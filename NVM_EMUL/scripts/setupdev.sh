#!/bin/bash

device_name="nvmemul"
device_major=256

device_module_name=$device_name".ko"
device_path="/dev/${device_name}"
device_module_path=`find .. -name ${device_module_name}`

function loaddev {
	if [ ! -c "$device_path" ]
	then 
	mknod $device_path c $device_major 0
	chmod a+wr $device_path
	fi
    /sbin/insmod $device_module_path
    lsmod | grep $device_name
}

function unloaddev {
    /sbin/rmmod $device_name   
}

if [ "$1" = "load" ] || [ "$1" = "l" ]
then
loaddev
fi

if [ "$1" = "unload" ] || [ "$1" = "u" ]
then
unloaddev
fi

if [ "$1" = "reload" ] || [ "$1" = "r" ]
then
unloaddev
loaddev
fi
