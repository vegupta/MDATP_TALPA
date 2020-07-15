/*
* linux_glue.c
*
* TALPA Filesystem Interceptor
*
* Copyright (C) 2004-2017 Sophos Limited, Oxford, England.
*
* This program is free software; you can redistribute it and/or modify it under the terms of the
* GNU General Public License Version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
* even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program; if not,
* write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*
*/
#include <linux/kernel.h>
#include <linux/version.h>

#include <asm/atomic.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>

#include <linux/dcache.h>

#if defined TALPA_VFSMOUNT_LOCK_BRLOCK || defined TALPA_VFSMOUNT_LG_BRLOCK
#include <linux/lglock.h>
#endif

#include "platforms/linux/glue.h"
#include "platforms/linux/log.h"
#include "platforms/linux/vfs_mount.h"

/* TALPA_MNT_NAMESPACE defined in vfs_mount.h */
#ifdef TALPA_MNT_NAMESPACE
#include <linux/nsproxy.h>
#endif

#if defined(TALPA_DPATH_PATH) && LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
#define TALPA_D_DNAME_DIRECT_DPATH
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) && (!defined TALPA_HAS_DPATH)
/**
 * d_path - return the path of a dentry
 * @dentry: dentry to report
 * @vfsmnt: vfsmnt to which the dentry belongs
 * @root: root dentry
 * @rootmnt: vfsmnt to which the root dentry belongs
 * @buffer: buffer to return value in
 * @buflen: buffer length
 *
 * Convert a dentry into an ASCII path name. If the entry has been deleted
 * the string " (deleted)" is appended. Note that this is ambiguous.
 *
 * Returns the buffer or an error code if the path was too long.
 *
 * "buflen" should be positive. Caller holds the dcache_lock.
 */
static char * __talpa_d_path( struct dentry *dentry, struct vfsmount *vfsmnt,
            struct dentry *root, struct vfsmount *rootmnt,
            char *buffer, int buflen)
{
    char * end = buffer+buflen;
    char * retval;
    int namelen;
    unsigned m_seq = 1;

    *--end = '\0';
    buflen--;
    if (!IS_ROOT(dentry) && d_unhashed(dentry))
    {
        buflen -= 10;
        end -= 10;
        if (buflen < 0)
            goto Elong;
        memcpy(end, " (deleted)", 10);
    }

    if (buflen < 1)
        goto Elong;
    /* Get '/' right */
    retval = end-1;
    *retval = '/';

    for (;;) {
        struct dentry * parent;

        if (dentry == root && vfsmnt == rootmnt)
            break;
        if (dentry == vfsmnt->mnt_root || IS_ROOT(dentry))
        {
            /* Global root? */
            talpa_vfsmount_lock(&m_seq);
            if (vfsmnt->mnt_parent == vfsmnt) {
                talpa_vfsmount_unlock(&m_seq);
                goto global_root;
            }
            dentry = vfsmnt->mnt_mountpoint;
            vfsmnt = vfsmnt->mnt_parent;
            talpa_vfsmount_unlock(&m_seq);
            continue;
        }
        parent = dentry->d_parent;
        prefetch(parent);
        namelen = dentry->d_name.len;
        buflen -= namelen + 1;
        if (buflen < 0)
            goto Elong;
        end -= namelen;
        memcpy(end, dentry->d_name.name, namelen);
        *--end = '/';
        retval = end;
        dentry = parent;
    }

    return retval;

global_root:
    namelen = dentry->d_name.len;
    buflen -= namelen;
    if (buflen < 0)
        goto Elong;
    retval -= namelen-1;    /* hit the slash */
    memcpy(retval, dentry->d_name.name, namelen);
    return retval;
Elong:
    return ERR_PTR(-ENAMETOOLONG);
}
#endif /* >= 2.6.0 */


char* talpa__d_namespace_path( struct dentry *dentry, struct vfsmount *vfsmnt,
            struct dentry *root, struct vfsmount *rootmnt,
            char *buffer, int buflen, bool* nonRootNamespaceOut, bool* inProcessNamespaceOut)
{
    char* path;

#ifdef TALPA_MNT_NAMESPACE
    struct vfsmount *ns_root_mnt = NULL;
    struct dentry *ns_root_dentry;
    unsigned m_seq = 1;
    struct vfsmount *process_root_mnt;
    struct dentry *process_root_dentry;
    struct task_struct* proc;

    proc = current;
    if ( likely(proc->fs != NULL) )
    {
        talpa_proc_fs_lock(&proc->fs->lock);
        process_root_mnt = mntget(talpa_task_root_mnt(proc));
        process_root_dentry = dget(talpa_task_root_dentry(proc));
        talpa_proc_fs_unlock(&proc->fs->lock);

        if (inProcessNamespaceOut)
        {
            if ( likely( proc->nsproxy != NULL ) )
            {
                *inProcessNamespaceOut = (getNamespaceInfo(vfsmnt) == proc->nsproxy->mnt_ns);
            }
            else
            {
                *inProcessNamespaceOut = false;
            }
        }

        talpa_vfsmount_lock(&m_seq);
        if (getNamespaceInfo(process_root_mnt))
        {
            ns_root_mnt = mntget(getNamespaceRoot(process_root_mnt));
        }
        talpa_vfsmount_unlock(&m_seq);

        dbg("root dentry %s dentry=%p vfsmnt=%p",process_root_dentry->d_name.name,process_root_dentry,process_root_mnt);

        if(ns_root_mnt)
        {
            ns_root_dentry = dget(ns_root_mnt->mnt_root);
            dbg("ns_root_dentry %s dentry=%p vfsmnt=%p",ns_root_dentry->d_name.name,ns_root_dentry,ns_root_mnt);

            path = talpa__d_path(dentry, vfsmnt, ns_root_dentry, ns_root_mnt,
                buffer, buflen, NULL);
            dbg("talpa__d_namespace_path: found d_namespace_path: %s", path);

            if (nonRootNamespaceOut)
            {
                *nonRootNamespaceOut = (ns_root_mnt != rootmnt);
            }
            dput(ns_root_dentry);

            mntput(ns_root_mnt);

        }
        else
        {
            path = talpa__d_path(dentry, vfsmnt, process_root_dentry, process_root_mnt,
                buffer, buflen, nonRootNamespaceOut);
            dbg("talpa__d_namespace_path: found non namespace path: %s", path);

        }
        dput(process_root_dentry);
        mntput(process_root_mnt);

    }
    else
    {
        dbg("proc->fs == NULL, cannot get namespace path");

        if (inProcessNamespaceOut)
        {
            *inProcessNamespaceOut = false;
        }
#else /* !TALPA_MNT_NAMESPACE */
    {
        if (inProcessNamespaceOut)
        {
            *inProcessNamespaceOut = true;
        }
#endif

        path = talpa__d_path(dentry, vfsmnt, root, rootmnt,
                buffer, buflen, nonRootNamespaceOut);
        dbg("talpa__d_namespace_path: found non namespace path: %s", path);
    }

    return path;
}

#ifdef TALPA_MNT_NAMESPACE
static void debugPathWalk( struct dentry *dentry, struct vfsmount *vfsmnt,
            struct dentry *root, struct vfsmount *rootmnt)
{
    /* Debug - try and evaluate roots */
    int count = 50;
    struct dentry *td = dentry;
    struct vfsmount *temp_vfsmnt = vfsmnt;

    err("DEBUG: expected root node %s dentry=%p vfsmnt=%p",root->d_name.name,root,rootmnt);
    while (td != root || temp_vfsmnt != rootmnt)
    {
        count--;
        if (count == 0)
        {
            err("DEBUG: reached count limit!");
            break;
        }
        err("DEBUG: examining %s %p",td->d_name.name,td);
        if (td == temp_vfsmnt->mnt_root || IS_ROOT(td))
        {
            struct vfsmount *temp2_vfsmnt = getParent(temp_vfsmnt);
            if (td == temp_vfsmnt->mnt_root)
            {
                err("DEBUG: found root dentry td == temp_vfsmnt->mnt_root %p",td);
            }
            if (IS_ROOT(td))
            {
                err("DEBUG: found root dentry IS_ROOT(td) %p",td);
            }
            err("DEBUG: going to parent: %p -> %p",temp_vfsmnt,temp2_vfsmnt);
            if (temp_vfsmnt != temp2_vfsmnt)
            {
                td = getVfsMountPoint(temp_vfsmnt);
                err("DEBUG: going to mountpoint %s dentry=%p", td->d_name.name, td);
                temp_vfsmnt = temp2_vfsmnt;
            }
            else
            {
                err("DEBUG: Got to temp_vfsmnt = temp2_vfsmnt!");
            }
        }
        if (td == td->d_parent)
        {
            err("DEBUG: td == td->d_parent");
            break;
        }
        else if (td->d_parent == NULL)
        {
            err("DEBUG: td->d_parent == NULL");
            break;
        }
        td = td->d_parent;
    }
    err("DEBUG: actual root node %s dentry=%p vfsmnt=%p",td->d_name.name,td,temp_vfsmnt);
    if (td == root && temp_vfsmnt == rootmnt)
    {
        err("DEBUG: Found the correct root");
    }
    else
    {
        err("DEBUG: Found wrong root td=%p (name=%s) vfsmnt=%p",
            td,
            td->d_name.name,
            temp_vfsmnt);
        err("DEBUG: Expected       root=%p (name=%s) vfsmnt=%p",
            root,
            root->d_name.name,
            rootmnt);
    }
}
#endif /* TALPA_MNT_NAMESPACE */

char* talpa__d_path( struct dentry *dentry, struct vfsmount *vfsmnt,
            struct dentry *root, struct vfsmount *rootmnt,
            char *buffer, int buflen, bool* nonRootNamespaceOut)
{
    char* path;

    /* Get the function pointer for the real __d_path if we're going to call it. */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0) || (defined TALPA_HAS_DPATH)

#   if defined TALPA_DPATH_SLES11
    typedef char *(*d_path_func)(const struct path *, struct path *, char *, int, int);
#   elif defined TALPA_DPATH_PATH
    typedef char *(*d_path_func)(const struct path *, struct path *, char *, int);
#   elif defined TALPA_DPATH_SUSE103
    typedef char *(*d_path_func)(struct dentry *, struct vfsmount *, struct dentry *, struct vfsmount *, char *buffer, int buflen, int flags);
#   else
    typedef char *(*d_path_func)(struct dentry *, struct vfsmount *, struct dentry *, struct vfsmount *, char *buffer, int buflen);
#   endif


#   if defined TALPA_HAS_DPATH_ADDR
    d_path_func kernel_d_path = (d_path_func)talpa_get_symbol("__d_path", (void *)TALPA_DPATH_ADDR);
#   else
    d_path_func kernel_d_path = &__d_path;
#   endif

#   if defined TALPA_DPATH_SLES11 || defined TALPA_DPATH_PATH
    struct path pathPath;
    struct path rootPath;
#   endif
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0) || (defined TALPA_HAS_DPATH) */

    if (nonRootNamespaceOut != NULL)
    {
        *nonRootNamespaceOut = false;
    }

#if defined HOLD_DCACHE_LOCK_WHILE_CALLING_D_PATH
    spin_lock(&dcache_lock);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0) || (defined TALPA_HAS_DPATH)
    /* Calling the real __d_path */
#   if defined TALPA_DPATH_SLES11 || defined TALPA_DPATH_PATH
    pathPath.dentry = dentry;
    pathPath.mnt = vfsmnt;
    rootPath.dentry = root;
    rootPath.mnt = rootmnt;
#   endif

#   if defined TALPA_D_DNAME_DIRECT_DPATH
    if (dentry->d_op && dentry->d_op->d_dname)
    {
        path = d_path(&pathPath, buffer, buflen);
        if ( unlikely( IS_ERR(path) != 0 ) )
        {
            critical("talpa__d_path: d_path returned an error: %ld",PTR_ERR(path));
            path = NULL;
        }
        if ( NULL != path )
        {
            return path;
        }
    }
#   endif /* TALPA_D_DNAME_DIRECT_DPATH */

#   if defined TALPA_DPATH_SLES11
    path = kernel_d_path(&pathPath, &rootPath, buffer, buflen, 0);
#   elif defined TALPA_DPATH_PATH
    path = kernel_d_path(&pathPath, &rootPath, buffer, buflen);
#   elif defined TALPA_DPATH_SUSE103
    path = kernel_d_path(dentry, vfsmnt, root, rootmnt, buffer, buflen, 0);
#   else
    path = kernel_d_path(dentry, vfsmnt, root, rootmnt, buffer, buflen);
#   endif
#else
    /* Call our own version */
    path = __talpa_d_path(dentry, vfsmnt, root, rootmnt, buffer, buflen);
#endif

#if defined HOLD_DCACHE_LOCK_WHILE_CALLING_D_PATH
    spin_unlock(&dcache_lock);
#endif

    if ( unlikely( IS_ERR(path) != 0 ) )
    {
        critical("talpa__d_path: kernel__d_path returned an error: %ld",PTR_ERR(path));
        path = NULL;
    }
    else if ( unlikely( NULL == path ) )
    {
#ifdef TALPA_D_DNAME_DIRECT_DPATH
        /* only use this as a fall-back, it will only return the relative path from a chroot
         * Use this in cases where kernel_d_path fails to return a valid path for bind mounts
         * in newer kernel in a systemd environment */
        path = d_path(&pathPath, buffer, buflen);
        if ( unlikely( IS_ERR(path) != 0 ) )
        {
            critical("talpa__d_path: kernel_d_path returned an error: %ld",PTR_ERR(path));
            path = NULL;
        }

        if (dentry->d_op && dentry->d_op->d_dname)
        {
            dbg("    dpath=%s",path);
            err("dpath=%s - dentry has d_op and d_dname=%p",path,dentry->d_op->d_dname);
        }
#endif
        if ( NULL == path )
        {
            if (!IS_ROOT(dentry) && d_unhashed(dentry)) {
                dbg("talpa__d_path: kernel_d_path returned NULL for deleted file");
                dbg("    basename=%s",dentry->d_name.name);
            }
            else
            {
                info("talpa__d_path: kernel_d_path returned NULL for non-deleted file");
                info("    basename=%s",dentry->d_name.name);
            }
        }
        else
        {
            if (!IS_ROOT(dentry) && d_unhashed(dentry))
            {
                dbg("    talpa__d_path: kernel_d_path returned NULL but d_path returned path %s for deleted file",path);
            }
            else
            {
                /* the systemd / containers / bind mount case.
                 *
                 * Now so common that we don't want to even debug log it
                 * dbg("    talpa__d_path: kernel_d_path returned NULL but d_path returned path %s for non-deleted file",path);
                 */
#ifdef TALPA_MNT_NAMESPACE
                if (NULL != getNamespaceInfo(vfsmnt) && (!S_ISDIR(dentry->d_inode->i_mode)))
                {
                    /* we're in a namespace/container */
                    if (nonRootNamespaceOut != NULL)
                    {
                        *nonRootNamespaceOut = true;
                    }
                }

                if (false)
                {
                    debugPathWalk(dentry, vfsmnt, root, rootmnt);
                }
#endif

            }
        }
    }

    return path;
}

/*
 * tasklist_lock un-export handling
 */
#ifdef TALPA_HIDDEN_TASKLIST_LOCK
void talpa_tasklist_lock(void)
{
    rwlock_t* talpa_tasklist_lock_addr = (rwlock_t *)talpa_get_symbol("tasklist_lock", (void *)TALPA_TASKLIST_LOCK_ADDR);

    read_lock(talpa_tasklist_lock_addr);
}

void talpa_tasklist_unlock(void)
{
    rwlock_t* talpa_tasklist_lock_addr = (rwlock_t *)talpa_get_symbol("tasklist_lock", (void *)TALPA_TASKLIST_LOCK_ADDR);

    read_unlock(talpa_tasklist_lock_addr);
}
#endif

#ifdef  TALPA_VFSMOUNT_LG_BRLOCK
DEFINE_BRLOCK(vfsmount_lock);
#elif defined TALPA_VFSMOUNT_LOCK_BRLOCK
DECLARE_BRLOCK(vfsmount_lock);
#endif

/*
 * hidden vfsmnt_lock handling
 */
void talpa_vfsmount_lock(unsigned* m_seq)
{
#if defined TALPA_USE_VFSMOUNT_LOCK
#   if defined TALPA_VFSMOUNT_LG_BRLOCK
    br_read_lock(&vfsmount_lock);
#   elif defined TALPA_VFSMOUNT_LOCK_BRLOCK
    br_read_lock(vfsmount_lock);
#   else
    spinlock_t* talpa_vfsmount_lock_addr = (spinlock_t *)talpa_get_symbol("vfmount_lock", (void *)TALPA_VFSMOUNT_LOCK_ADDR);

    spin_lock(talpa_vfsmount_lock_addr);
#   endif
#elif defined TALPA_USE_MOUNT_LOCK
    seqlock_t* mount_lock_addr = (seqlock_t *)talpa_get_symbol("mount_lock", (void *)TALPA_MOUNT_LOCK_ADDR);
    read_seqbegin_or_lock(mount_lock_addr,m_seq);
#else
    // On 2.4 we don't have vfsmount_lock - we use dcache_lock instead
    spin_lock(&dcache_lock);
#endif

}

bool talpa_vfsmount_unlock(unsigned* m_seq)
{
#if defined  TALPA_USE_VFSMOUNT_LOCK
#   if defined TALPA_VFSMOUNT_LG_BRLOCK
    br_read_unlock(&vfsmount_lock);
#   elif defined TALPA_VFSMOUNT_LOCK_BRLOCK
    br_read_unlock(vfsmount_lock);
#   else
    spinlock_t* talpa_vfsmount_lock_addr = (spinlock_t *)talpa_get_symbol("vfmount_lock", (void *)TALPA_VFSMOUNT_LOCK_ADDR);


    spin_unlock(talpa_vfsmount_lock_addr);
#   endif
#elif defined TALPA_USE_MOUNT_LOCK
    seqlock_t* mount_lock_addr = (seqlock_t *)talpa_get_symbol("mount_lock", (void *)TALPA_MOUNT_LOCK_ADDR);
    if (need_seqretry(mount_lock_addr, *m_seq)) {
       *m_seq = 1;
       return true;
    }
    done_seqretry(mount_lock_addr, *m_seq);
#else
    // On 2.4 we don't have vfsmount_lock - we use dcache_lock instead
    spin_unlock(&dcache_lock);
#endif
    return false;
}

#ifndef TALPA_PUTNAME_EXPORTED
void talpa_putname(TALPA_FILENAME_T* filename)
{
    /*
     * Uses putname if available, or final_putname - but they come from the same define
     */
    typedef void(*putname_func)(TALPA_FILENAME_T *);
    putname_func putname = (putname_func)talpa_get_symbol("putname", (void *)TALPA_PUTNAME_ADDRESS);
    putname(filename);
}
#endif

#ifndef TALPA_GETNAME_EXPORTED
TALPA_FILENAME_T * talpa_getname(const char __user * filename )
{
    /*
     * Uses putname if available, or final_putname - but they come from the same define
     */
    typedef TALPA_FILENAME_T * (*getname_func)(const char __user * filename);
    getname_func getname = (getname_func)talpa_get_symbol("getname", (void *)TALPA_GETNAME_ADDRESS);
    return(getname(filename));
}
#endif

#ifdef TALPA_HANDLE_RELATIVE_PATH_IN_MOUNT
# ifndef TALPA_SYSTEM_GET_FS_ROOT_AND_PWD
void talpa_get_fs_root_and_pwd(
                struct fs_struct *fs,
                struct path *root,
                struct path *pwd)
{
    spin_lock(&fs->lock);
    *root = fs->root;
    path_get(root);
    *pwd = fs->pwd;
    path_get(pwd);
    spin_unlock(&fs->lock);
}
# endif
#endif /* TALPA_HANDLE_RELATIVE_PATH_IN_MOUNT */

void* getUtsNamespace(struct task_struct* process)
{
#ifdef TALPA_MNT_NAMESPACE
    if ( likely(process->nsproxy != NULL) )
    {
        return (void*) process->nsproxy->uts_ns;
    }
#endif
    return NULL;
}

/*
* End of linux_glue.c
*/

