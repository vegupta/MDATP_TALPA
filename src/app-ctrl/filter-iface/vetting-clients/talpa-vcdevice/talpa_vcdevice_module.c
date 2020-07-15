/*
 * talpa_vcdevice_module.c
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

#define TALPA_SUBSYS "vcdevice"
#include "common/talpa.h"
#include "components/filter-iface/vetting-clients/device_driver_vc_impl/device_driver_vetting_client.h"
#include "app_ctrl/iportability_app_ctrl.h"
#include "app_ctrl/icore_app_ctrl.h"


#ifdef TALPA_ID
const char talpa_id[] = "$TALPA_ID:" TALPA_ID;
#endif

#ifdef TALPA_VERSION
const char talpa_version[] = "$TALPA_VERSION:" TALPA_VERSION;
#endif

static DeviceDriverVettingClient*   mClient;


static int __init talpa_vcdevice_init(void)
{
    IConfigurator* config;
    int ret;


    /* Create a new client */
    mClient = newDeviceDriverVettingClient(TALPA_Core()->vettingServer());
    if ( !mClient )
    {
        err("Failed to create client!");
        return -ENOMEM;
    }

    /* Expose the configuration */
    config = TALPA_Portability()->configurator();
    ret = config->attach(config->object, ECG_FilterInterfaces, &mClient->i_IConfigurable);
    if ( ret != 0 )
    {
        err("Failed to attach configuration!");
        mClient->delete(mClient);
        return ret;
    }

    dbg("Ready");
    return 0;
}

static void __exit talpa_vcdevice_exit(void)
{
    IConfigurator* config;


    config = TALPA_Portability()->configurator();
    config->detach(config->object, &mClient->i_IConfigurable);

    mClient->delete(mClient);

    dbg("Unloaded");
    return;
}

/*
 *
 * Module information.
 *
 */
MODULE_AUTHOR("Sophos Limited");
MODULE_DESCRIPTION("TALPA Filesystem Interceptor Device Driver Vetting Client Module");
MODULE_LICENSE("GPL");
#if defined TALPA_VERSION && defined MODULE_VERSION
MODULE_VERSION(TALPA_VERSION);
#endif


module_init(talpa_vcdevice_init);
module_exit(talpa_vcdevice_exit);


/*
 * End of talpa_vcdevice_module.c
 */
