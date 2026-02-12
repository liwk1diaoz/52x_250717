#!/bin/bash
if [ -f ${LINUX_BUILD_TOP}/admin/json_parser.sh ]; then
	source ${LINUX_BUILD_TOP}/admin/json_parser.sh $1
	if [ $? -eq 1 ]; then
		echo -e "\e[1;31mStop packing, we can't find json config file.\e[0m"
		exit 0
	fi
else
	echo -e "\e[1;31mStop packing, we can't find necessary procedures.\e[0m"
fi
#############################################################################
#${SDK_PACK_COMP_NAME}.tar.bz2
#	|
#	--------NVT_NA51023_BSP
#			|
#			--------bin
#			|
#			--------toolchain
#			|
#			--------NA51023_BSP
#					|
#					--------packages
#############################################################################
CPU1_TYPE=""
CPU2_TYPE=""
SDK_FOLDER_NAME="NVT_NA51023_BSP"
SDK_SUB_FOLDER_NAME="NA51023_BSP"
SDK_ROOT=${LINUX_BUILD_TOP}/${SDK_FOLDER_NAME}
SDK_TMP=${LINUX_BUILD_TOP}/${RANDOM}${RANDOM}
SDK_BIN_PATH=${SDK_ROOT}/bin
SDK_SRC_PATH=${SDK_ROOT}/${SDK_SUB_FOLDER_NAME}
SDK_SRC_PACK_PATH=${SDK_ROOT}/${SDK_SUB_FOLDER_NAME}/packages
SDK_TOOLCHAIN_PATH=${SDK_ROOT}/toolchain
UITRON_TOOLCHAIN_PATH="/opt/im/mipsel-sde-elf-4.4.3"
SDK_MAKE_LIST="uitron linux modules supplement uboot busybox library sample"
APP_PACK_NAME="`cat ${NVT_PRJCFG_MODEL_CFG}  | grep NVT_CFG_APP | grep -v NVT_CFG_APP_EXTERNAL | awk -F'=' '{print $NF}'`"
APP_EXT_REMOVE_LIST=""
SUPPLEMENT_EXCLUDE=""
#############################################################################

function setupCpuType () {
        if [ -z "$SDK_MULTI_MODEL" ]; then
                CPU1_TYPE=`cat $NVT_PRJCFG_MODEL_CFG  | grep "CPU1_TYPE" | grep "=" | awk -F'=' '{print $NF}' | tr -d ' ' | fromdos`;
                CPU2_TYPE=`cat $NVT_PRJCFG_MODEL_CFG  | grep "CPU2_TYPE" | grep "=" | awk -F'=' '{print $NF}' | tr -d ' ' | fromdos`;
        else
                for multi_model_list in $SDK_MULTI_MODEL
                do
                        nvt_prjcfg_model_cfg_dir=`dirname $NVT_PRJCFG_MODEL_CFG`
                        cpu1_type=`cat $nvt_prjcfg_model_cfg_dir/$multi_model_list  | grep "CPU1_TYPE" | grep "=" | awk -F'=' '{print $NF}' | tr -d ' ' | fromdos`;
                        cpu2_type=`cat $nvt_prjcfg_model_cfg_dir/$multi_model_list  | grep "CPU2_TYPE" | grep "=" | awk -F'=' '{print $NF}' | tr -d ' ' | fromdos`;
                        CPU1_TYPE="$cpu1_type $CPU1_TYPE"
                        CPU2_TYPE="$cpu2_type $CPU2_TYPE"
                done
        fi
}

function checkCpuType(){
        if [ "$1" == "CPU1" ]; then
                if [ -z "$SDK_MULTI_MODEL" ]; then
                        if [ "$2" == "$CPU1_TYPE" ]; then
                                echo "yes"
                                return
                        else
                                echo "no"
                                return
                        fi
                else
                        for n in $CPU1_TYPE
                        do
                                if [ "$2" == "$n" ]; then
                                        echo "yes"
                                        return
                                fi
                        done
                fi
        elif [ "$1" == "CPU2" ]; then
                if [ -z "$SDK_MULTI_MODEL" ]; then
                        if [ "$2" == "$CPU2_TYPE" ]; then
                                echo "yes"
                                return
                        else
                                echo "no"
                                return
                        fi
                else
                        for n in $CPU2_TYPE
                        do
                                if [ "$2" == "$n" ]; then
                                        echo "yes"
                                        return
                                fi
                        done
                fi
        fi

        echo "no"
}

echo "Starting to pack process"
if [ -z ${LINUX_BUILD_TOP} ]; then
	echo -e "\r\nERROR :Please source build/envsetup.sh in NA51023 firstly\r\n"
	exit 1;
fi

echo "########################################################"
echo -e "\e[1;44mGIT ID revision:\e[0m"
	repo forall -c 'echo -e "\e[1;34m$REPO_PROJECT\e[0m"; git log HEAD -n 1; echo -e "===============================================\n"'
echo -e "\e[1;44mSDK file name: ${SDK_PACK_COMP_NAME_LIBVER}.tar.bz2\e[0m"
if [ -z "$SDK_MULTI_MODEL" ]; then
	echo -e "\e[1;44mModel Config: `echo ${NVT_PRJCFG_MODEL_CFG} | awk -F/ '{print $NF}'`\e[0m" 
else
	# Support for multi model packing
	for n in $SDK_MULTI_MODEL
	do
		if [ ! -f `dirname $NVT_PRJCFG_MODEL_CFG`/$n ]; then
			echo "ERROR: Multi model setting error"
			exit 1;
		fi
	done
	echo -e "\e[1;44mModel Config: `echo $SDK_MULTI_MODEL`\e[0m" 
fi
tmp=`echo "$CROSS_COMPILE" | awk -F/ '{for(i=2;i<NF;i++) print $i;}'`;
for n in $tmp; do if [ ! -z "`echo $n | grep linux`" ]; then ret="$n"; fi; done;
echo -e "\e[1;44mToolchain: \c"; echo -e "$ret \e[0m"
echo "########################################################"

echo "checking if all target configs/cfg_* folders are available"
if [ -z "$SDK_MULTI_MODEL" ]; then
	MODEL=`basename ${NVT_PRJCFG_MODEL_CFG}`;
	MODEL=${MODEL%.txt}
	MODEL=${MODEL#ModelConfig_}
	if [ ! -d ${LINUX_BUILD_TOP}/configs/cfg_${MODEL} ]; then
		echo "${LINUX_BUILD_TOP}/configs/cfg_${MODEL} is not available!!!"
		exit -1
	fi
else
	for multi_model_list in $SDK_MULTI_MODEL
	do
		MODEL=${multi_model_list%.txt}
		MODEL=${MODEL#ModelConfig_}
		if [ ! -d ${LINUX_BUILD_TOP}/configs/cfg_${MODEL} ]; then
			echo "${LINUX_BUILD_TOP}/configs/cfg_${MODEL} is not available!!!"
			exit 1
		fi
	done
fi
echo "all target configs/cfg_* folders are available"

echo -e "\e[1;33m\rAre you sure to pack? (y/n)\r\e[0m"
read answer
if [ $answer != "y" ]; then
	rm -rf ${SDK_TMP};
	exit 0;
fi

set -e
if [ -d $LINUX_BUILD_TOP/.git ]; then
	#############################################################################################
	# codebase sync init.
	#############################################################################################
	cp -rf ${LINUX_BUILD_TOP}/admin $LINUX_BUILD_TOP/.git/
	repo forall -c "git reset --hard;"
	cp -rvf $LINUX_BUILD_TOP/.git/admin/* ${LINUX_BUILD_TOP}/admin/
	rm -rf $LINUX_BUILD_TOP/.git/admin
	git clean -xfd -e admin -e build
	echo -e "$SDK_PACK_COMP_NAME_LIBVER\n" > ${LINUX_BUILD_TOP}/admin/ChangeList
	echo "===========================================================================" >> ${LINUX_BUILD_TOP}/admin/ChangeList
	repo forall -c 'echo -e "\t$REPO_PROJECT\t\t" `git log --pretty=format:"%H" -1`' >> ${LINUX_BUILD_TOP}/admin/ChangeList
	echo -e "===========================================================================\n\n" >> ${LINUX_BUILD_TOP}/admin/ChangeList
	if [ ! -z "$SDK_PREV_PACK_COMP_NAME" ]; then
		echo $SDK_PREV_PACK_COMP_NAME"-"$SDK_PACK_COMP_NAME":" >> ${LINUX_BUILD_TOP}/admin/ChangeList
		git log $SDK_PREV_PACK_COMP_NAME...HEAD >> ${LINUX_BUILD_TOP}/admin/ChangeList
	fi
	make clean
	mkdir -p ${SDK_TMP}
	rm -rf ${SDK_ROOT} *.tar.bz2
	mkdir -p ${SDK_SRC_PACK_PATH}

	#############################################################################################
	# Toolchain
	#############################################################################################
	echo -e "\e[1;42m>>>>> copy toolchain\e[0m"
	mkdir -p ${SDK_TOOLCHAIN_PATH}
	cd ${SDK_TOOLCHAIN_PATH}
	cp -af ${CROSS_TOOLCHAIN_PATH} ./
	chmod 755 -R `echo ${CROSS_TOOLCHAIN_PATH} | awk -F/ '{print $NF}'`
	tar -jcf `echo ${CROSS_TOOLCHAIN_PATH} | awk -F/ '{print $NF}'`.tar.bz2 `echo ${CROSS_TOOLCHAIN_PATH} | awk -F/ '{print $NF}'`
	rm -rf `echo ${CROSS_TOOLCHAIN_PATH} | awk -F/ '{print $NF}'`
	cp -af ${UITRON_TOOLCHAIN_PATH} ./
	chmod 755 -R `echo ${UITRON_TOOLCHAIN_PATH} | awk -F/ '{print $NF}'`
	tar -jcf `echo ${UITRON_TOOLCHAIN_PATH} | awk -F/ '{print $NF}'`.tar.bz2 `echo ${UITRON_TOOLCHAIN_PATH} | awk -F/ '{print $NF}'`
	rm -rf `echo ${UITRON_TOOLCHAIN_PATH} | awk -F/ '{print $NF}'`
	cd -

	#############################################################################################
	# Source code
	#############################################################################################
	echo -e "\e[1;42m>>>>> source code packing\e[0m"
	echo "SDK Version:" > ${SDK_ROOT}/version
	echo -e "\t${SDK_PACK_COMP_NAME_LIBVER}.tar.bz2\n" >> ${SDK_ROOT}/version
	echo "VCS:" >> ${SDK_ROOT}/version
	repo forall -c 'echo -e "\t$REPO_PROJECT\t\t" `git log --pretty=format:"%H" -1`' >> ${SDK_ROOT}/version
	# build 
	echo -e "\e[1;42m>>>>> copy build folder\e[0m"
	mkdir ${SDK_TMP}/build
	cp -rf build/. ${SDK_TMP}/build/
	pushd ${SDK_TMP}
	if [ ! -z `echo ${CROSS_COMPILE} | grep uclibc` ]; then
		sed -i '/add_lunch_toolchain_combo arm-ca53-linux-gnueabihf-4.9-2017.05/d' build/envsetup.sh
	else
		sed -i '/add_lunch_toolchain_combo arm-ca53-linux-uclibcgnueabihf-4.9-2017.02/d' build/envsetup.sh
	fi
	tar -jcf ${SDK_SRC_PACK_PATH}/build.tar.bz2 build --exclude ".git" --exclude ".svn"
	rm -rf ${SDK_TMP}/build/
	popd

	# configs
	mkdir ${SDK_TMP}/configs
	cp -rf configs/. ${SDK_TMP}/configs/
	pushd ${SDK_TMP}
	pushd configs
	if [ -z "$SDK_MULTI_MODEL" ]; then
		MODEL=`echo $NVT_PRJCFG_MODEL_CFG | awk -F'/' '{print $NF}' | awk -F'.' '{print $1}' | awk -F'ModelConfig_' '{print $NF}'`;
		for n in `ls -d */ | cut -f1 -d'/'`
		do
			if [ "$n" != "cfg_$MODEL" ]; then
				rm -rf $n;
			fi
		done
	else
		model_list="`echo $SDK_MULTI_MODEL | sed -e "s/ModelConfig_//g" | sed -e "s/.txt//g"`"
		for n in `ls -d */ | cut -f1 -d'/'`
		do
			parse_model="`echo $n | sed -e "s/cfg_//g"`"
			if [ -z "`echo $model_list | grep -w $parse_model`" ]; then
				rm -rf $n;
			fi
		done
	fi
	popd
	echo -e "\e[1;42m>>>>> copy configs folder\e[0m"
	tar -jcf ${SDK_SRC_PACK_PATH}/configs.tar.bz2 configs --exclude ".git" --exclude ".svn"
	rm -rf ${SDK_TMP}/configs
	popd

	# u-boot
	echo -e "\e[1;42m>>>>> copy u-boot folder\e[0m"
	tar -jcf ${SDK_SRC_PACK_PATH}/u-boot.tar.bz2 u-boot --exclude ".git" --exclude ".svn"

	# Copy Linux related
	echo -e "\e[1;42m>>>>> copy linux related folders\e[0m"
	setupCpuType

	# Single model
	if [ `checkCpuType "CPU2" "CPU2_LINUX"` == "yes" ]; then
		# sample
		tar -jcf ${SDK_SRC_PACK_PATH}/sample.tar.bz2 sample --exclude ".git" --exclude ".svn"
		# busybox
		echo -e "\e[1;42m>>>>> copy busybox folder\e[0m"
		tar -jcf ${SDK_SRC_PACK_PATH}/busybox.tar.bz2 busybox --exclude ".git" --exclude ".svn"
		# linux-kernel
		echo -e "\e[1;42m>>>>> copy linux-kernel folder\e[0m"
		tar -jcf ${SDK_SRC_PACK_PATH}/linux-kernel.tar.bz2 linux-kernel --exclude ".git" --exclude ".svn"
		# tools
		echo -e "\e[1;42m>>>>> copy tools folder\e[0m"
		tar -jcf ${SDK_SRC_PACK_PATH}/tools.tar.bz2 tools --exclude ".git" --exclude ".svn"
	fi

	# make all
	echo -e "\e[1;42m>>>>> make all\e[0m"
	if [ -z "$SDK_MULTI_MODEL" ]; then
		if [[ ( `checkCpuType "CPU1" "CPU1_ECOS"` == "yes" || `checkCpuType "CPU1" "CPU1_UITRON"` == "yes" ) && ( `checkCpuType "CPU2" "CPU2_NONE"` == "yes" || `checkCpuType "CPU2" "CPU2_ECOS"` == "yes" ) ]]; then
			# ecos only
			source build/envsetup.sh
			make all
			make pack

			echo -e "\e[1;42m>>>>> copy BIN folder\e[0m"
			mkdir ${SDK_TMP}/output/
			cp -arfv ${OUTPUT_DIR}/* ${SDK_TMP}/output/
			mkdir -p ${SDK_BIN_PATH}
			cp -arfv ${SDK_TMP}/output/* ${SDK_BIN_PATH}
			rm -rf ${SDK_TMP}/output/
		else
			# Single model (linux + itron)
			source build/envsetup.sh
			for n in ${SDK_MAKE_LIST}
			do
				make $n
			done

			# Not all applications need to be build, we will buit it separately
			echo -e "\e[1;42m>>>>> make app\e[0m"
			pushd ${APP_DIR}
			for n in ${APP_PACK_NAME}
			do
				pushd $n
				make; make install;
				popd
			done
			pushd external
			make; make install;
			make clean;
			popd
			popd

			# root-fs
			echo -e "\e[1;42m>>>>> make rootfs\e[0m"
			make rootfs

			# pack
			echo -e "\e[1;42m>>>>> make pack\e[0m"
			make pack
			echo -e "\e[1;42m>>>>> copy BIN folder\e[0m"
			mkdir ${SDK_TMP}/output/
			cp -arfv ${OUTPUT_DIR}/* ${SDK_TMP}/output/
			mkdir -p ${SDK_BIN_PATH}
			cp -arfv ${SDK_TMP}/output/* ${SDK_BIN_PATH}
			rm -rf ${SDK_TMP}/output/
		fi
	else
		# Multi models
		for multi_model_list in $SDK_MULTI_MODEL
		do
			sed -i 's/ModelConfig_.*/'"$multi_model_list"'/g' $BUILD_DIR/.nvt_default
			source build/envsetup.sh
			cpu1_type=`cat $NVT_PRJCFG_MODEL_CFG  | grep "CPU1_TYPE" | grep "=" | awk -F'=' '{print $NF}' | tr -d ' ' | fromdos`;
			cpu2_type=`cat $NVT_PRJCFG_MODEL_CFG  | grep "CPU2_TYPE" | grep "=" | awk -F'=' '{print $NF}' | tr -d ' ' | fromdos`;
			if [[ ( "$cpu1_type" == "CPU1_ECOS" || "$cpu1_type" == "CPU1_UITRON" ) && ( "$cpu2_type" == "CPU2_NONE" || "$cpu2_type" == "CPU2_ECOS" ) ]]; then
				# ecos only
				make clean
				make all
				make pack
			else
				APP_PACK_NAME="`cat ${NVT_PRJCFG_MODEL_CFG}  | grep NVT_CFG_APP | grep -v NVT_CFG_APP_EXTERNAL | awk -F'=' '{print $NF}'`"
				make uitron_clean;
				make uboot_clean;
				make linux_clean;
				make rootfs_clean;
				for n in ${SDK_MAKE_LIST}
				do
					make $n
				done

				# Not all applications need to be build, we will buit it separately
				echo -e "\e[1;42m>>>>> make app\e[0m"
				pushd ${APP_DIR}
				for n in ${APP_PACK_NAME}
				do
					pushd $n
					make; make install;
					popd
				done
				pushd external
				make; make install;
				make clean;
				popd
				popd

				# root-fs
				echo -e "\e[1;42m>>>>> make rootfs\e[0m"
				make rootfs
			fi
			#if multi_model_list is not set in exclude list, copy its bin files
			if [ -z "`echo $SDK_EXCLUDE_BIN_LIST | grep $multi_model_list`" ]; then
				echo -e "\e[1;42m>>>>> copy BIN folder\e[0m"
				bin_name="`echo $multi_model_list | sed 's/.txt//g'`";
				mkdir -p ${SDK_TMP}/output/$bin_name
				cp -arfv ${OUTPUT_DIR}/* ${SDK_TMP}/output/$bin_name
				mkdir -p ${SDK_BIN_PATH}
				cp -avf ${SDK_TMP}/output/* ${SDK_BIN_PATH}
				rm -rf ${SDK_TMP}/output/
			fi
		done
	fi

	echo -e "\e[1;42m>>>>> copy root-fs folder\e[0m"

	if [ `checkCpuType "CPU2" "CPU2_LINUX"` == "yes" ]; then
		if [ -z "$SDK_MULTI_MODEL" ]; then
			pushd root-fs; ETC_LINK=`readlink rootfs/etc| awk -F'/' '{print $NF}'`; make clean;
			pushd rootfs/etc_Model
			ETC_LINK_EXCLUDE=`ls -I$ETC_LINK`
			ETC_LINK_EXCLUDE_RESULT=""; for n in $ETC_LINK_EXCLUDE; do ETC_LINK_EXCLUDE_RESULT="--exclude rootfs/etc_Model/$n $ETC_LINK_EXCLUDE_RESULT"; done
			popd
			popd
		else
			pushd root-fs; make clean;
			pushd rootfs/etc_Model
			etc_link=""
			for n in $SDK_MULTI_MODEL
			do
				modelcfg="`dirname $NVT_PRJCFG_MODEL_CFG`/$n"
				NVT_ROOTFS_ETC="`cat $modelcfg | grep NVT_ROOTFS_ETC | awk -F'=' '{print $NF}'`"
				if [ ! -z "$NVT_ROOTFS_ETC" ]; then
					etc_link="-Ietc_$NVT_ROOTFS_ETC $etc_link"
				else
					NVT_ROOTFS_ETC="`echo $n | awk -F'ModelConfig_' '{print $NF}' | awk -F'.txt' '{print $1}'`";
					etc_link="-Ietc_$NVT_ROOTFS_ETC $etc_link"
				fi
			done
			ETC_LINK_EXCLUDE=`ls $etc_link`
			ETC_LINK_EXCLUDE_RESULT=""; for n in $ETC_LINK_EXCLUDE; do ETC_LINK_EXCLUDE_RESULT="--exclude rootfs/etc_Model/$n $ETC_LINK_EXCLUDE_RESULT"; done
			popd
			popd
		fi
		tar -jcf ${SDK_SRC_PACK_PATH}/root-fs.tar.bz2 root-fs --exclude ".git" --exclude ".svn" $ETC_LINK_EXCLUDE_RESULT
	fi
	# include
	# This folder will contain some header files need to be released after compiling all
	echo -e "\e[1;42m>>>>> copy include folder\e[0m"
	if [ "$SDK_LINUX_SRC_RELEASE" != "on" ]; then
		tar -jcf ${SDK_SRC_PACK_PATH}/include.tar.bz2 include --exclude ".git" --exclude ".svn" --exclude "protected" --exclude "nvtlive555.h"
	else
		tar -jcf ${SDK_SRC_PACK_PATH}/include.tar.bz2 include --exclude ".git" --exclude ".svn" --exclude "nvtlive555.h"
	fi
	if [ `checkCpuType "CPU2" "CPU2_LINUX"` == "yes" ]; then
		# application
		echo -e "\e[1;42m>>>>> copy application folder\e[0m"
		# Reorg app pack list
		if [ ! -z "$SDK_MULTI_MODEL" ]; then
			APP_PACK_NAME=""
			for n in $SDK_MULTI_MODEL
			do
				modelcfg="`dirname $NVT_PRJCFG_MODEL_CFG`/$n"
				app_pack_name="`cat ${modelcfg}  | grep NVT_CFG_APP | grep -v NVT_CFG_APP_EXTERNAL | awk -F'=' '{print $NF}'`"
				for m in $app_pack_name
				do
					if [ -z "`echo $APP_PACK_NAME | grep $m`" ]; then
						APP_PACK_NAME="$m $APP_PACK_NAME"
					fi
				done
			done
		fi

		mkdir ${SDK_TMP}/application
		cd application
		cp -rf ${APP_PACK_NAME} Makefile external ${SDK_TMP}/application
		for n in ${APP_EXT_REMOVE_LIST}
		do
			rm -rf ${SDK_TMP}/application/external/$n
		done
		cd ${SDK_TMP}
		# ONVIF related handling
		if [ -d application/onvif ]; then pushd application/onvif; rm -rf HttpCgi  ItriOnvifSysLib  MakeRule NetCtrl  Obj  Onvif  PlatformSystem itri.pack  lib  setting; popd; fi

		if [ "$SDK_LINUX_SRC_RELEASE" != "on" ]; then
			for n in ${APP_PACK_NAME}
			do
				cmp_ret="`if [ "$m" != "" ]; then for m in $SDK_LINUX_SRC_RELEASE_APP_LIST; do if [ "$m" == "$n" ]; then echo true; break; fi; done; fi`"

				# Search for release code list
				#if [ -z "`echo $SDK_LINUX_SRC_RELEASE_APP_LIST | grep $n`" ]; then
				if [ "${cmp_ret}" != "true" ]; then
					# bin only can't clean build
					echo ------------------------------------------------------------
					echo $n clean source
					find application/$n -name "*.c" -exec rm {} \; -o -name "*.h" -exec rm {} \; -o -name "*.cpp" -exec rm {} \; -o -name "*.hh" -exec rm {} \; -o -name "*.o" -exec rm {} \; -o -name "*.sym" -exec rm {} \;
				else
					# release code can clean build
					echo ------------------------------------------------------------
					echo $n clean build
					popd application/$n; make clean; pushd;
				fi
			done
		else
			# Linux release source code
			for n in $SDK_LINUX_SRC_RELEASE_APP_EXCLUDE_LIST
			do
				find application/$n -name "*.c" -exec rm {} \; -o -name "*.h" -exec rm {} \; -o -name "*.cpp" -exec rm {} \; -o -name "*.hh" -exec rm {} \; -o -name "*.o" -exec rm {} \; -o -name "*.sym" -exec rm {} \;
			done
			find application -name "*.o" -exec rm {} \;
		fi
		tar -jcf ${SDK_SRC_PACK_PATH}/application.tar.bz2 application --exclude ".git" --exclude ".svn"
		cd ${LINUX_BUILD_TOP}
		rm -rf ${SDK_TMP}/application

		# lib
		echo -e "\e[1;42m>>>>> copy lib folder\e[0m"
		mkdir ${SDK_TMP}/lib
		pushd lib/external; make clean; popd
		cd lib
		cp -rf * ${SDK_TMP}/lib
		rm -rf ${SDK_TMP}/lib/engines
		rm -rf ${SDK_TMP}/lib/pkgconfig
		cd ${SDK_TMP}
		pushd lib; LIB_PACK_NAME=`ls -d */ | cut -f1 -d'/'`; popd
		if [ "$SDK_LINUX_SRC_RELEASE" != "on" ]; then
			for n in $LIB_PACK_NAME
			do
				cmp_ret="`if [ "$m" != "" ]; then for m in $SDK_LINUX_SRC_RELEASE_LIB_LIST; do if [ "$m" == "$n" ]; then echo true; break; fi; done; fi`"

				# Search for release code list
				#if [ -z "`echo $SDK_LINUX_SRC_RELEASE_LIB_LIST | grep $n`" ]; then
				if [ "${cmp_ret}" != "true" ]; then
					find lib/$n -path "*/external" -prune -o -name "*.c" -exec rm {} \; -o -name "*.S" -exec rm {} \; -o -name "*.h" -exec rm {} \; -o -name "*.o" -exec rm {} \; -o -name "*.sym" -exec rm {} \; -o -name "*.cpp" -exec rm {} \;
				else
					pushd lib/$n; make clean; popd;
				fi
			done
		else
			# Linux release source code
			for n in $SDK_LINUX_SRC_RELEASE_LIB_REMOVE_LIST
			do
				rm -rf lib/$n;
			done
			for n in $SDK_LINUX_SRC_RELEASE_LIB_EXCLUDE_LIST
			do
				find lib/$n -path "*/external" -prune -o -name "*.c" -exec rm {} \; -o -name "*.S" -exec rm {} \; -o -name "*.h" -exec rm {} \; -o -name "*.o" -exec rm {} \; -o -name "*.sym" -exec rm {} \; -o -name "*.cpp" -exec rm {} \;
			done
			find lib -name "*.o" -exec rm {} \;
		fi
		tar -jcf ${SDK_SRC_PACK_PATH}/lib.tar.bz2 lib --exclude ".git" --exclude ".svn"
		cd ${LINUX_BUILD_TOP}
		rm -rf ${SDK_TMP}/lib

		# linux-supplement
		echo -e "\e[1;42m>>>>> copy linux-supplement folder\e[0m"
		cp -rf linux-supplement ${SDK_TMP}/linux-supplement
		cd ${SDK_TMP}

		for n in $SDK_LINUX_SRC_RELEASE_SUPPLEMENT_REMOVE_LIST
		do
			rm -rf linux-supplement/$n;
		done

		if [ "$SDK_LINUX_SRC_RELEASE" != "on" ]; then
			pushd linux-supplement; SUPPLEMENT_PACK_NAME=`ls -d */ | cut -f1 -d'/'`; popd
			for n in $SUPPLEMENT_PACK_NAME
			do
				cmp_ret="`if [ "$m" != "" ]; then for m in $SDK_LINUX_SRC_RELEASE_SUPPLEMENT_LIST; do if [ "$m" == "$n" ]; then echo true; break; fi; done; fi`"
				#if [ -z "`echo $SDK_LINUX_SRC_RELEASE_SUPPLEMENT_LIST | grep $n`" ]; then
				if [ "${cmp_ret}" != "true" ]; then
					find linux-supplement/$n -name "*.c" -exec rm {} \; -o -name "*.h" -exec rm {} \; -o -name "*.cmd" -exec rm {} \; -o -name "modules.order" -exec rm {} \; -o -name "Module.symvers" -exec rm {} \; -o -name "build.sh" -exec rm {} \;
				else
					find linux-supplement/$n -name "*.cmd" -exec rm {} \; -o -name "modules.order" -exec rm {} \; -o -name "Module.symvers" -exec rm {} \; -o -name "build.sh" -exec rm {} \;
				fi
			done
		else
			find linux-supplement -name nvt_isp -prune -o -name "*.o" -exec rm {} \;
		fi

		# The net drivers should be removed because not all source code are nvt copyright.
		find linux-supplement/net -name synopsys -prune -o -name "*.c" -exec rm {} \; -o -name "*.h" -exec rm {} \; -o -name "*.cmd" -exec rm {} \; -o -name "modules.order" -exec rm {} \; -o -name "Module.symvers" -exec rm {} \; -o -name "build.sh" -exec rm {} \;

		# The isp drivers should be removed because consist of confidential source
		make -C linux-supplement/misc/nvt_isp setup

		for n in ${SUPPLEMENT_EXCLUDE}
		do
			rm -rf linux-supplement/$n
		done
		tar -jcf ${SDK_SRC_PACK_PATH}/linux-supplement.tar.bz2 . --exclude ".git" --exclude ".svn"
		cd ${LINUX_BUILD_TOP}
		rm -rf ${SDK_TMP}/linux-supplement
	fi

	echo -e "\e[1;42m>>>>> copy uitron folder\e[0m"
	cp -rf uitron ${SDK_TMP}/uitron
	cd ${SDK_TMP}/uitron
	bash $SDK_ITRON_SRC_RELEASE_SCRIPT "$SDK_MULTI_MODEL"
	cp uitron.tar.bz2 ${SDK_SRC_PACK_PATH}/uitron.tar.bz2

	echo -e "\e[1;42m>>>>> copy ecos folder\e[0m"
	if [ `checkCpuType "CPU2" "CPU2_ECOS"` == "yes" ]; then
		cd ${LINUX_BUILD_TOP}
		pushd ecos-kit/MakeCommon
		make clean
		popd
		tar -jcf ${SDK_SRC_PACK_PATH}/ecos-kit.tar.bz2 ecos-kit --exclude ".git" --exclude ".svn"
	else
		echo "cpu2 is not ecos. no need to copy ecos folder"
	fi

	echo -e "\e[1;42m>>>>> copy other files\e[0m"
	cd ${LINUX_BUILD_TOP}
	cp -avf Makefile ${SDK_SRC_PATH}
	cp -avf ${LINUX_BUILD_TOP}/admin/sdk.unpack ${SDK_ROOT}
	pushd ${SDK_ROOT}
	find . -type f ! -name nvt.md5 -exec md5sum {} \; > nvt.md5
	popd

	# Pack with toolchain
	tar -jcf ${SDK_PACK_COMP_NAME_LIBVER}.tar.bz2 ${SDK_FOLDER_NAME}

	# Pack without toolchain
	rm -rf ${SDK_TOOLCHAIN_PATH}
	pushd ${SDK_ROOT}
	find . -type f ! -name nvt.md5 -exec md5sum {} \; > nvt.md5
	popd
	tar -jcf LE_${SDK_PACK_COMP_NAME_LIBVER}.tar.bz2 ${SDK_FOLDER_NAME}

	rm -rf ${SDK_TMP}
	rm -rf ${SDK_ROOT}

	echo "########################################################"
	echo -e "\e[1;44mGIT ID revision:\e[0m"
	repo forall -c 'echo -e "\t\e[1;44m$REPO_PROJECT\t\t `git log --pretty=format:"%H" -1`\e[0m"'
	echo -e "\e[1;44mSDK file name: ${SDK_PACK_COMP_NAME_LIBVER}.tar.bz2\e[0m"
	echo -e "\e[1;44mModel Config: ${NVT_PRJCFG_MODEL_CFG}\e[0m"
	if [ ! -z `echo ${CROSS_COMPILE} | grep uclibc` ]; then
		echo -e "\e[1;44mToolchain: ucibc\e[0m"
	else
		echo -e "\e[1;44mToolchain: glibc\e[0m"
	fi
	echo "########################################################"
	echo ">>>>> With toolchain version"
	tar -tjvf ${SDK_PACK_COMP_NAME_LIBVER}.tar.bz2

	echo ">>>>> Without toolchain version"
	tar -tjvf LE_${SDK_PACK_COMP_NAME_LIBVER}.tar.bz2
else
	echo -e "\e[1;42m>>>>> Please execute git clone firstly\e[0m"
fi
