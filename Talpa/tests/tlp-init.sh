#! /bin/bash
#
# TALPA test script
#
# Copyright (C) 2004-2011 Sophos Limited, Oxford, England.
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

mknod /dev/talpa-test c 60 0 2>/dev/null
chmod a+rw /dev/talpa-test
if [ -d /sys/kernel/security ]; then
    if grep -q securityfs /proc/filesystems; then
        mount none -t securityfs /sys/kernel/security
    fi
fi

echo "************************************************"
echo "***                                          ***"
echo "*** PLEASE OPEN A SEPARATE KERNEL LOG VIEWER ***"
echo "***      (and watch the talpa output)        ***"
echo "***                                          ***"
echo "************************************************"

