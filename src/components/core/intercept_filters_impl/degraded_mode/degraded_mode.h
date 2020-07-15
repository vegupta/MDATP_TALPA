/*
 * degraded_mode.h
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
#ifndef H_DEGRADEDMODEPROCESSOR
#define H_DEGRADEDMODEPROCESSOR

#include <asm/atomic.h>

#include "common/locking.h"
#include "intercept_filters/iintercept_filter.h"
#include "configurator/iconfigurable.h"

/*
 * Configuration structures
 */


#define DMD_CFGDATASIZE      (16)

typedef struct {
    char    name[DMD_CFGDATASIZE];
    char    value[DMD_CFGDATASIZE];
} DegrModeConfigData;


typedef struct tag_DegradedModeProcessor
{
    IInterceptFilter          i_IInterceptFilter;
    IConfigurable             i_IConfigurable;
    void                      (*delete)(struct tag_DegradedModeProcessor* object);
    bool                      mEnabled;

    talpa_simple_lock_t       mLock;
    atomic_t                  mThreshold;
    bool                      mActive;

    talpa_mutex_t             mConfigSerialize;

    PODConfigurationElement   mConfig[4];
    DegrModeConfigData        mStateConfigData;
    DegrModeConfigData        mThresholdConfigData;
    DegrModeConfigData        mActiveConfigData;
} DegradedModeProcessor;

/*
 * Object Creators.
 */
DegradedModeProcessor* newDegradedModeProcessor(void);



#endif

/*
 * End of degraded_mode.h
 */

