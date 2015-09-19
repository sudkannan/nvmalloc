#!/bin/bash -x
   #

#script to create and mount a pmfs
#requires size as input


if [[ x$1 == x ]];
   then
      echo You have specify correct pmfs size in GB
      exit 1
   fi



 mkdir /mnt/pmfs; chmod 777 /mnt/pmfs

if mount | grep /mnt/pmfs > /dev/null; then
	echo "/mnt/pmfs already mounted"
else
	 sudo mount -t tmpfs -o size=$1M tmpfs /mnt/pmfs
fi

