#! /bin/bash
#
# TALPA test script
#
# Copyright (C) 2008-2011 Sophos Limited, Oxford, England.
#
# This program is free software; you can redistribute it and/or modify it under the terms of the
# GNU General Public License Version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program; if not,
# write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#

. ${srcdir}/tlp-cleanup.sh

if test "$interceptor_module" = "syscall"; then
    tlp_insmod ../talpa_syscallhook.${ko}
elif test "$interceptor_module" = "vfshook"; then
    tlp_insmod ../talpa_syscallhook.${ko} hook_mask=mu
else
    exit 77
fi

tlp_insmod ../talpa_syscallhookprobe.${ko}
if test $? -ne 0; then
    exit 1
fi
rmmod talpa_syscallhookprobe

tlp_insmod modules/tlp-syscalltable.${ko}
if test $? -ne 0; then
    exit 1
fi

tlp_insmod ../talpa_syscallhookprobe.${ko} 2>/dev/null
if test $? -eq 0; then
    exit 1
fi

exit 0
