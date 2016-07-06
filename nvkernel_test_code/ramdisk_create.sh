#!/bin/bash -x
#

#script to create and mount a pmfs
#requires size as input


if [[ x$1 == x ]];
then
	echo You have specify correct pmfs size in GB
	exit 1
fi


mkdir /mnt/ramdisk; chmod 777 /mnt/ramdisk

if mount | grep /mnt/ramdisk > /dev/null; then
	echo "/mnt/pmfs already mounted"
else
	sudo mount -t tmpfs -o size=$1M tmpfs /mnt/ramdisk
fi
exit




mkdir /mnt/pmfs; chmod 777 /mnt/pmfs

if mount | grep /mnt/pmfs > /dev/null; then
	echo "/mnt/pmfs already mounted"
else
	sudo mount -t mntfs -o size=$1M mntfs /mnt/pmfs
fi

exit



