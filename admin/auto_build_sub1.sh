#/bin/sh
##### cfg_528_FPGA must be at the end of list for special case #####
MODEL_BUILD_LIST=(
		"lunch Linux cfg_IPCAM1_NOR_EVB arm-ca9-linux-gnueabihf-6.5"
		"lunch Linux cfg_EMMC_RAMDISK_EVB arm-ca9-linux-uclibcgnueabihf-6.5"
		"lunch rtos cfg_RTOS_BOOT_LINUX_NOR_EVB gcc-6.5-newlib-2.4-2019.11-arm-ca9-eabihf"
		"lunch Linux cfg_RTOS_BOOT_LINUX_NOR_EVB arm-ca9-linux-uclibcgnueabihf-6.5"
)

function show_msg() {
	echo -e "\e[1;44m[`echo $NVT_PRJCFG_MODEL_CFG | awk -F'/' '{print $NF}'`]\e[0m $1"
}

function show_model_to_stderr() {
	MODEL=`echo $NVT_PRJCFG_MODEL_CFG | awk -F'/' '{print $(NF-1)}'`;
	echo "============================================================================================" >&2;
	echo "Current model is: $MODEL, $NVT_PRJCFG_CFG, $CROSS_TOOLCHAIN_PATH" >&2;
	echo "============================================================================================" >&2;
}

function init_env() {
	# Sync submodule source code
	export cur_dir=`pwd`
	export AUTOBUILD_OUTPUT_DIR=$cur_dir/../../autobuild_output
	mkdir -p $AUTOBUILD_OUTPUT_DIR
	cd $cur_dir/../
#	export MULTI_CORES=1
	source build/envsetup.sh
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
	make linux_clean; make linux
	make modules
}

function copy_prebuild() {
	show_msg "Copy prebuild lib/external begin"
	cd $PREBUILD_PATH/lib/external/
	for n in `find . -maxdepth 1 -type d | awk -F'./' '{print $2}'`
	do
		cp -rf $n $LIBRARY_DIR/external
	done
	cd $LIBRARY_DIR/external; make install;
	show_msg "Copy prebuild lib/external finish"
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

function build_supplement() {
	show_msg "Build supplement"
	cd $LINUX_BUILD_TOP/linux-supplement
	make KERNELDIR=$PREBUILD_PATH/linux-kernel KBUILD_OUTPUT=$PREBUILD_PATH/linux-kernel/OUTPUT modules
}

function prebuild() {
	build_ker
	build_lib_ext
	#build_app_ext
}

function remove_ignore() {
	for n in $IGNORE_LIST
	do
		cd $LINUX_BUILD_TOP
		rm -rf $n;
	done
}

function dump_server_info() {
	echo $LD_LIBRARY_PATH
	hostname
	git config -l
	git --version
	get_stuff_for_environment
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
		cd $LINUX_BUILD_TOP;
		if [ $? -ne 0 ]; then exit -1; fi;
		# show msg on log-xxxxxxxx-stdout.txt for code statistic server git git log.
		# because when autobuild failed, the git_log.txt will not be made.
		repo forall -c 'ret=`git log -1 | grep ${gVERSION}`; if [ ! -z "$ret" ]; then git log -1; fi;'
		for n in "${MODEL_BUILD_LIST[@]}"
		do
			cd $LINUX_BUILD_TOP;
			rm -rf output
			rm -rf root-fs/bin/
			make clean -j1
			show_msg "=================================================================================================================================="
			echo $n;
			dump_server_info
			show_msg "=================================================================================================================================="
			# special case for cfg_528_FPGA, its NVT_FPGA and NVT_EMULATION must ON and re-source envsetup.sh
			if [[ $n == *"cfg_528_FPGA"* ]]; then
				sed -i 's/^export NVT_FPGA=OFF.*/export NVT_FPGA=ON/g' build/envoption.sh
				sed -i 's/^export NVT_EMULATION=OFF.*/export NVT_EMULATION=ON/g' build/envoption.sh
				source build/envsetup.sh
			else
				sed -i 's/^export NVT_FPGA=ON.*/export NVT_FPGA=OFF/g' build/envoption.sh
				sed -i 's/^export NVT_EMULATION=ON.*/export NVT_EMULATION=OFF/g' build/envoption.sh
				source build/envsetup.sh
			fi
			$n;
			# do not move chk_unix to out of loop, because symbolic has not been created yet.
			make chk_unix
			show_model_to_stderr
			mkdir -p $LINUX_BUILD_TOP/BIN/$NVT_PRJCFG_CFG/$MODEL/symbol
			make all -j1
#			make sample
#			cd $NVT_HDAL_DIR;
#			make test_install;
			if [ $? -ne 0 ]; then exit -1; fi;
			# special case for RTOS_BOOT_LINUX, we have to cp rtos-main.bin and application.bin to top folder for Linux
			if [[ $MODEL == *"RTOS_BOOT_LINUX"* ]] && [[ $NVT_PRJCFG_CFG == "rtos" ]]; then
				cp output/rtos-main.bin output/application.bin .;
			fi
			cd $LINUX_BUILD_TOP;
			cp $OUTPUT_DIR/packed/*.bin BIN/$NVT_PRJCFG_CFG/$MODEL
			# !!! md5, .sym, .map, and list_bin.txt are for codebase statistics server !!!
			find BIN/$NVT_PRJCFG_CFG/$MODEL -name "*.bin" -exec sh -c 'x="{}"; md5sum "${x}" > "${x}.md5" ' \;
#			cp $NVT_HDAL_DIR/test_cases/output BIN/$NVT_PRJCFG_CFG/$MODEL -rf
			find base -name *.sym -exec cp {} BIN/$NVT_PRJCFG_CFG/$MODEL/symbol \;
			find base -name *.map -exec cp {} BIN/$NVT_PRJCFG_CFG/$MODEL/symbol \;
			find ${KERNELDIR}/ -name "System.map" -exec cp {} BIN/$NVT_PRJCFG_CFG/$MODEL/symbol/uImage.map \;
			find ${UBOOT_DIR}/ -name "System.map" -exec cp {} BIN/$NVT_PRJCFG_CFG/$MODEL/symbol/u-boot.map \;
			ls output/*.bin -al > BIN/$NVT_PRJCFG_CFG/$MODEL/list_bin.txt
			ls $LINUX_BUILD_TOP/BIN/
			repo forall -c 'ret=`git log -1 | grep ${gVERSION}`; if [ ! -z "$ret" ]; then git log -1; fi;' > BIN/$NVT_PRJCFG_CFG/$MODEL/git_log.txt
			touch BIN/$NVT_PRJCFG_CFG/$MODEL/`echo $git_user_email`
			echo $gVERSION >> BIN/$NVT_PRJCFG_CFG/$MODEL/`echo $git_user_email`
			echo $gTIMESTAMP >> BIN/$NVT_PRJCFG_CFG/$MODEL/`echo $git_user_email`
			echo $git_repository_name >> BIN/$NVT_PRJCFG_CFG/$MODEL/`echo $git_user_email`
			echo $git_project_name >> BIN/$NVT_PRJCFG_CFG/$MODEL/`echo $git_user_email`
			echo $git_user_name >> BIN/$NVT_PRJCFG_CFG/$MODEL/`echo $git_user_email`
		done
		#Debug purpose: Check if dir_1, dir_2 or dir_3 is in G:\BinaryCode\xxx project BIN directory.
		# Thus we can know which path of autobuild_output folder is correct.

		#echo $PWD
		#echo $LINUX_BUILD_TOP
		#mkdir -p $LINUX_BUILD_TOP/../autobuild_output/dir_1
		#mkdir -p $LINUX_BUILD_TOP/autobuild_output/dir_2
		#mkdir -p $LINUX_BUILD_TOP/admin/autobuild_output/
		echo $AUTOBUILD_OUTPUT_DIR
		tree BIN > BIN/filelist
		cp -arfv BIN/* $AUTOBUILD_OUTPUT_DIR

		show_msg "=================================================================================================================================="
		show_msg "Finish auto-build"
		show_msg "=================================================================================================================================="
	;;
esac
