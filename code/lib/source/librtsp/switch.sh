#!/bin/bash

case "$1" in
"8126_28" )
	rm ./config.mk -f
	echo "KERN_26=y" > ./config.mk
	echo "KERN_SUB_28=y" >> ./config.mk
	echo "USR_SRC=/usr/src/arm-linux-2.6.28" >> ./config.mk
	echo "LINUX_SRC=\$(USR_SRC)/linux-2.6.28-fa" >> ./config.mk
	echo "include \$(LINUX_SRC)/.config" >> ./config.mk
	echo "OUTPUT_DIR=../simple_ipc/release" >> ./config.mk
	echo "export KERN_26 LINUX_SRC OUTPUT_DIR" >> ./config.mk
	echo "PLATFORM_X86=n" >> ./config.mk
	echo Prepare GM8126 rtsp library for Linux 2.6.28 Done!
;;
"8181_14" )
	rm ./config.mk -f
	echo "KERN_26=y" > ./config.mk
	echo "KERN_SUB_28=n" >> ./config.mk
	echo "USR_SRC=/usr/src/arm-linux-2.6" >> ./config.mk
	echo "LINUX_SRC=\$(USR_SRC)/linux-2.6.14-fa" >> ./config.mk
	echo "include \$(LINUX_SRC)/.config" >> ./config.mk
	echo "OUTPUT_DIR=../simple_dvr" >> ./config.mk
	echo "export KERN_26 LINUX_SRC OUTPUT_DIR" >> ./config.mk
	echo "PLATFORM_X86=n" >> ./config.mk
	echo Prepare GM8181 rtsp library for linux2.6.14 Done!
;;
* )
	echo "Usage $0 8126_28|8181_14."
	echo "8126_28: Prepare GM8181 for linux-2.6.28"
	echo "8181_14: Prepare GM8181 for linux-2.6.14"
;;
esac
