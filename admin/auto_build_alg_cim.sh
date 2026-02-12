#/bin/sh
#!!!!! the combination must NVT_PRJCFG_CFG=Linux come first, followed by rtos !!!!!
MODEL_BUILD_LIST=("lunch Linux cfg_IPCAM1_RAMDISK_EVB arm-ca9-linux-gnueabihf-6.5"
		"lunch rtos cfg_TEST_EVB gcc-6.5-newlib-2.4-2019.11-arm-ca9-eabihf"
		"lunch rtos cfg_DEMO_EVB gcc-6.5-newlib-2.4-2019.11-arm-ca9-eabihf"
		"lunch rtos cfg_CARDV_EVB gcc-6.5-newlib-2.4-2019.11-arm-ca9-eabihf")
#PREBUILD_IMAGE_GIT_BRANCH_NAME="prebuild/na51055_linux_4.9_master"
#PREBUILD_IMAGE_GIT_FILEPATH="prebuild_hdr_na51055_linux_sdk_2019_12_17_10_12_18.tar.bz2"
PREBUILD_IMAGE_GIT_BRANCH_NAME="prebuild/na51055_linux_4.19_master"
PREBUILD_IMAGE_GIT_FILEPATH="prebuild_hdr_na51055_linux_sdk_2020_11_16_17_23_28.tar.bz2"
PREBUILD_IMAGE_GIT_PATH="http://git.novatek.com.tw/scm/ivot_bsp/ivot_prebuild_hdr.git"
PREBUILD_IMAGE_SSH_PATH="ssh://git.novatek.com.tw:7999/ivot_bsp/ivot_prebuild_hdr.git"

DATE=`date +"%Y_%m_%d_%H_%M_%S"`

function show_msg() {
	echo -e "\e[1;44m[`echo $NVT_PRJCFG_MODEL_CFG | awk -F'/' '{print $NF}'`]\e[0m $1"
}

function show_msg_err() {
	echo -e "$1" >&2;
}

function show_model_to_stderr() {
	MODEL=`echo $NVT_PRJCFG_MODEL_CFG | awk -F'/' '{print $(NF-1)}'`;
	echo "============================================================================================" >&2;
	echo "Current model is: $MODEL, $CROSS_TOOLCHAIN_PATH" >&2;
	echo "============================================================================================" >&2;
}

function init_env() {
	# Sync submodule source code
	export cur_dir=`pwd`
	echo $cur_dir
	export AUTOBUILD_OUTPUT_DIR=$cur_dir/../../autobuild_output
	mkdir -p $AUTOBUILD_OUTPUT_DIR
	cd $cur_dir/../
#	export MULTI_CORES=1
	source build/envsetup.sh
	export PREBUILD_PATH="$LINUX_BUILD_TOP/prebuild"
	export PREBUILD_IMAGE_PATH="$PREBUILD_PATH/$PREBUILD_IMAGE_GIT_FILEPATH"
	${MODEL_BUILD_LIST[0]}
}

function build_app_ext() {
	cd $APP_DIR/external/
	make clean
	make install
}

function build_lib_ext() {
	cd $LIBRARY_DIR/external/
	make clean
	make
	make install
}

function build_ker() {
	cd $LINUX_BUILD_TOP
	# use linux_config_gcov to change kernel config for tracer on
	sed -i 's/CONFIG_GCOV_KERNEL=y/CONFIG_TRACING_SUPPORT=y\\nCONFIG_FTRACE=y\\nCONFIG_HAVE_FUNCTION_TRACER=y\\nCONFIG_FUNCTION_TRACER=y/g' build/Makefile
	make linux_config_gcov
	make linux_clean; make linux
	make modules
}

function copy_prebuild() {
	show_msg "Copy prebuild lib/external begin"
	rm -rf $PREBUILD_PATH
	show_msg "Starting to clone prebuild header"
	#git clone ${PREBUILD_IMAGE_GIT_PATH} -b ${PREBUILD_IMAGE_GIT_BRANCH_NAME} ${PREBUILD_PATH}
	#git clone --depth 1 -b ${PREBUILD_IMAGE_GIT_BRANCH_NAME} --single-branch ${PREBUILD_IMAGE_GIT_PATH} ${PREBUILD_PATH}
	git clone --depth 1 -b ${PREBUILD_IMAGE_GIT_BRANCH_NAME} --single-branch ${PREBUILD_IMAGE_SSH_PATH} ${PREBUILD_PATH}
	cd ${PREBUILD_PATH}
	git fetch --unshallow
	cd -
	#git clone -b ${PREBUILD_IMAGE_GIT_BRANCH_NAME} --single-branch ${PREBUILD_IMAGE_GIT_PATH} ${PREBUILD_PATH}
	if [ ! -f $PREBUILD_IMAGE_PATH ]; then
		show_msg "Without this file: $PREBUILD_IMAGE_PATH"
		show_msg_err "Without this file: $PREBUILD_IMAGE_PATH"
		exit -1;
	fi
	tar -jxf $PREBUILD_IMAGE_PATH -C $PREBUILD_PATH
	ret=$?
	if [ $ret != 0 ]; then
		show_msg "Decompress this file: $PREBUILD_IMAGE_PATH error"
		exit -1;
	fi
	# To Modify the symbolic link
	rm -rf ${PREBUILD_PATH}/BSP/linux-kernel/arch/arm/plat-novatek/include/plat
	ln -s ${PREBUILD_PATH}/BSP/linux-kernel/arch/arm/plat-novatek/include/plat-na51055 ${PREBUILD_PATH}/BSP/linux-kernel/arch/arm/plat-novatek/include/plat
	cp -arfv $PREBUILD_PATH/BSP/root-fs/* $ROOTFS_DIR/
	rm -rf $ROOTFS_DIR/bin $OUTPUT_DIR
	#cd $PREBUILD_PATH/lib/external/
	#for n in `find . -maxdepth 1 -type d | awk -F'./' '{print $2}'`
	#do
	#	cp -rf $n $LIBRARY_DIR/external
	#done
	#cd $LIBRARY_DIR/external; make install;
	#show_msg "Copy prebuild lib/external finish"
#	show_msg "Copy prebuild app/external begin"
#	cd $PREBUILD_PATH/application/external/
#	for n in `find . -maxdepth 1 -type d | grep -v ./__install | awk -F'./' '{print $2}'`
#	do
#			cp -rf $n $APP_DIR/external
#	done
#	cd $APP_DIR/external; make install;
#	show_msg "Copy prebuild app/external finish"
}

function build_auto() {
	show_msg "Start to lib/app build"
	cd $APP_DIR
	cp Makefile Makefile.auto
	# Regenerate build list: nvt@app and nvt@lib only
	build_list=`cat Makefile.auto | grep "BUILD_LIST :"`
	new_build_list=""; for n in $build_list; do if [ -z "`echo $n | grep nvt@ext`" ]; then new_build_list="$new_build_list ""$n"; fi done
	sed -i '/nvt@ext@lib@%).*nvt@ext@app/d' Makefile.auto
	sed -i "/LIB_EXTLIST :=/a ${new_build_list}" Makefile.auto
	# Build nvt app/lib only (Remove external opensource build)
	make -f Makefile.auto || exit "$?";
	rm Makefile.auto
}

function build_code() {
	show_msg "Build Modules for CIM"
	cd $LINUX_BUILD_TOP/
	make vos
	make alg
	make hdal
if [[ $NVT_PRJCFG_CFG == "rtos" ]]; then
	make driver
	make library
	pushd $APP_DIR/
	make cim
	make clean
	popd
fi
	make hdal_clean
	make vos_clean
}

function build_driver() {
	show_msg "Build driver"
	cd $LINUX_BUILD_TOP
	make driver
}

function prebuild() {
	make busybox
	build_ker
	PROJECT_NAME=`echo $LINUX_BUILD_TOP | awk -F'/' '{print $NF}' | tr -d ' ' | fromdos`
	tar -jcvf $OUTPUT_DIR/prebuild_hdr_${PROJECT_NAME}_${DATE}.tar.bz2 BSP/.
}

function remove_ignore() {
	for n in $IGNORE_LIST
	do
		cd $LINUX_BUILD_TOP
		rm -rf $n;
	done
}

function dump_server_info() {
	hostname
	git config -l
	git --version
}

init_env > /dev/null

case $1 in
	prebuild)
		show_msg "Start prebuild"
		prebuild
		show_msg "Finish prebuild"
	;;
	*)
		show_msg "=================================================================================================================================="
		show_msg "Start auto-build"
		show_msg "=================================================================================================================================="
		dump_server_info
		cd $LINUX_BUILD_TOP;
		copy_prebuild
		if [ $? -ne 0 ]; then exit -1; fi;
		for n in "${MODEL_BUILD_LIST[@]}"
		do
			cd $LINUX_BUILD_TOP;
			rm -rf output
			rm -rf root-fs/bin/
			show_msg "=================================================================================================================================="
			echo $n;
			show_msg "=================================================================================================================================="
			$n;
			# do not move chk_unix to out of loop, because symbolic has not been created yet.
			make chk_unix
			show_model_to_stderr
			mkdir -p $LINUX_BUILD_TOP/BIN/$MODEL
			if [[ $NVT_PRJCFG_CFG == "Linux" ]]; then
				export KERNELDIR=${PREBUILD_PATH}/BSP/linux-kernel
			fi
			build_code;
			if [ $? -ne 0 ]; then exit -1; fi;
			cd $LINUX_BUILD_TOP;
			if [ ! -z "${gVERSION}" ]; then repo forall -c 'ret=`git log -1 | grep ${gVERSION}`; if [ ! -z "$ret" ]; then git log -1; fi;' > BIN/$MODEL/git_log.txt; fi;
			if [ ! -z "${git_user_email}" ]; then
				touch BIN/$MODEL/`echo $git_user_email`;
				if [ ! -z "${gVERSION}" ]; then echo $gVERSION >> BIN/$MODEL/`echo $git_user_email`; fi
				if [ ! -z "${gTIMESTAMP}" ]; then echo $gTIMESTAMP >> BIN/$MODEL/`echo $git_user_email`; fi
				if [ ! -z "${git_repository_name}" ]; then echo $git_repository_name >> BIN/$MODEL/`echo $git_user_email`; fi
				if [ ! -z "${git_project_name}" ]; then echo $git_project_name >> BIN/$MODEL/`echo $git_user_email`; fi
				if [ ! -z "${git_user_name}" ]; then echo $git_user_name >> BIN/$MODEL/`echo $git_user_email`; fi
			fi
		done
		tree BIN > BIN/filelist
		cp -arfv BIN/* $AUTOBUILD_OUTPUT_DIR

		show_msg "=================================================================================================================================="
		show_msg "Finish auto-build"
		show_msg "=================================================================================================================================="
	;;
esac
