#!/bin/sh

MODULES=`lsmod | grep 8189fs`

if [ -z "$MODULES" ]; then
	modprobe 8189fs
else
	echo "8189fs already exist"
fi

TRY=0
while [ 1 ]
do
	WLAN0=`ifconfig -a | grep wlan0`

	if [ -z "$WLAN0" ]; then
		echo "TRY=$TRY"
		if [ $TRY -gt 5 ]; then
			rmmod 8189fs
			exit 255
		else
			TRY=$((${TRY}+1))
			sleep 1
		fi
	else
		echo "wlan0 found"
		exit 0
	fi
done
