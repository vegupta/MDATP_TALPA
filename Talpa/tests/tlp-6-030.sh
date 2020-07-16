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

tlp_insmod modules/tlp-file.${ko}

tmpdir='/tmp/tlp-test'

mkdir -p ${tmpdir}/mnt1

dd if=/dev/zero of=${tmpdir}/fs1.img bs=1M count=4 >/dev/null 2>&1

mkfs='/sbin/mkfs.minix'
mkfs_args=4096
fs=minix
if [ ! -x "$mkfs" ]; then
    mkfs='/sbin/mkfs.vfat'
    mkfs_args=
    fs=vfat
    if [ ! -x "$mkfs" ]; then
        mkfs=''
    fi
fi

if [ "$mkfs" = "" ]; then
    exit 77
fi

${mkfs} ${tmpdir}/fs1.img ${mkfs_args} >/dev/null

# We can't be sure we'll have all we need to run this test so skip if anything fails
if ! mount -t $fs ${tmpdir}/fs1.img ${tmpdir}/mnt1 -o loop; then
    exit 77
fi

touch ${tmpdir}/mnt1/talpa-test-file

if ! ./tlp-6-030 ${tmpdir}/mnt1/talpa-test-file; then
    rc=$?
    umount ${tmpdir}/mnt1
    exit $rc
fi

if ! umount ${tmpdir}/mnt1; then
    exit $?
fi

exit 0
