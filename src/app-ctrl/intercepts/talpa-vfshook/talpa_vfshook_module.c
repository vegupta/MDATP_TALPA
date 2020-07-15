/*
 * talpa_vfshook_module.c
 *
 * TALPA Filesystem Interceptor
 *
 * Copyright (C) 2004-2011 Sophos Limited, Oxford, England.
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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <asm/errno.h>

#define TALPA_SUBSYS "vfshook"
#include "common/talpa.h"
#include "components/intercepts/vfshook_impl/vfshook_interceptor.h"
#include "app_ctrl/iportability_app_ctrl.h"
#include "app_ctrl/icore_app_ctrl.h"


#ifdef TALPA_ID
const char talpa_id[] = "$TALPA_ID:" TALPA_ID;
#endif

#ifdef TALPA_VERSION
const char talpa_version[] = "$TALPA_VERSION:" TALPA_VERSION;
#endif

static VFSHookInterceptor*    mIntercept;


static int __init talpa_vfshook_init(void)
{
    IInterceptProcessor*    target;
    IConfigurator*          config;
    int ret;


    /*
     * Create a new interceptor!
     */
    mIntercept = newVFSHookInterceptor();
    if ( !mIntercept )
    {
        err("Failed to create interceptor!");
        return -ENOMEM;
    }

    /*
     * Set the InterceptProcessor that will be targetted by the Interceptor.
     */
    target = TALPA_Core()->interceptProcessor();
    if ( !target )
    {
        err("Failed to obtain intercept processor!");
        mIntercept->delete(mIntercept);
        return -ENOENT;
    }

    /*
     * Expose the Interceptor's configuration.
     */
    config = TALPA_Portability()->configurator();
    ret = config->attach(config->object, ECG_Interceptor, &mIntercept->i_IConfigurable);
    if ( ret != 0 )
    {
        err("Failed to attach configuration!");
        mIntercept->delete(mIntercept);
        return ret;
    }

    mIntercept->i_IInterceptor.addInterceptProcessor(mIntercept, target);

    dbg("Ready");
    return 0;

}

static void __exit talpa_vfshook_exit(void)
{
    IConfigurator* config;


    config = TALPA_Portability()->configurator();
    config->detach(config->object, &mIntercept->i_IConfigurable);

    mIntercept->delete(mIntercept);
    dbg("Unloaded");
    return;
}

/*
 *
 * Module information.
 *
 */
MODULE_AUTHOR("Sophos Limited");
MODULE_DESCRIPTION("TALPA Filesystem Interceptor VFS Hook Intercept Module");
MODULE_LICENSE("GPL");
#if defined TALPA_VERSION && defined MODULE_VERSION
MODULE_VERSION(TALPA_VERSION);
#endif


module_init(talpa_vfshook_init);
module_exit(talpa_vfshook_exit);


/*
 * End of talpa_vfshook_module.c
 */
