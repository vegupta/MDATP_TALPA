/*
 * personality_factoryimpl.h
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
#ifndef H_PERSONALITYFACTORYIMPL
#define H_PERSONALITYFACTORYIMPL

#include "personality/ipersonality_factory.h"


typedef struct tag_LinuxPersonalityFactoryImpl
{
    IPersonalityFactory         i_IPersonalityFactory;
    void                        (*delete)(struct tag_LinuxPersonalityFactoryImpl* object);
} LinuxPersonalityFactoryImpl;

/*
 * Object Creators.
 */
LinuxPersonalityFactoryImpl* newLinuxPersonalityFactoryImpl(void);


#endif

/*
 * End of personality_factoryimpl.h
 */

