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

. ${srcdir}/tlp-cleanup.sh

tlp_insmod modules/tlp-filesysteminfo.${ko}
./chkfsinfo /dev/hda /mnt ext2 4
if test $? -eq 77; then
    ./chkfsinfo /dev/sda /mnt ext2 4
    if test $? -eq 77; then
        ./chkfsinfo /dev/vda /mnt ext2 4
        if test $? -eq 77; then
            ./chkfsinfo/dev/md0 /mnt ext2 4
            if test $? -eq 77; then
                ./chkfsinfo `find /dev/ide/ -name "disc" | head -n 1` /mnt ext2 4
                if test $? -eq 77; then
                    ./chkfsinfo `find /dev/scsi/ -name "disc" | head -n 1` /mnt ext2 4
                fi
            fi
        fi
    fi
fi

exit $?
