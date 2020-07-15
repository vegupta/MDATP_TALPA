
/* Build by configure */
#include "autoconf.h"

#include <linux/version.h>
#include <linux/types.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
#include <linux/posix_types.h>
#endif

#ifdef HAVE_KCONFIG
#include <linux/kconfig.h>
#endif

#ifdef TALPA_HAS_ASM_GENERIC_FCNTL
#include <asm-generic/fcntl.h>
#endif

#include <linux/fcntl.h>

#ifndef KBUILD_BASENAME
# define KBUILD_BASENAME
#endif

#include <linux/fs.h>

#ifdef TALPA_HAS_STRUCT_FILENAME
#define TALPA_FILENAME_T struct filename
#else /* ! TALPA_HAS_STRUCT_FILENAME */
#define TALPA_FILENAME_T char
#endif /* TALPA_HAS_STRUCT_FILENAME */

void testfunc();

// Deliberately not called
void testfunc()
{
    char* x = "A";
    TALPA_FILENAME_T* y = getname(x);
}
