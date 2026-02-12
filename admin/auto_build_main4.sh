#/bin/sh
##### cfg_528_FPGA must be at the end of list for special case #####
MODEL_BUILD_LIST=(
		"lunch rtos cfg_528_RTOS_BOOT_LINUX_EVB gcc-6.5-newlib-2.4-2019.11-arm-ca9-eabihf"
		"lunch Linux cfg_EMMC_EVB arm-ca9-linux-gnueabihf-6.5"
		"lunch Linux cfg_528_RTOS_BOOT_LINUX_EVB arm-ca9-linux-uclibcgnueabihf-6.5f"
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
	source build/envsetup.sh
}


function dump_server_info() {
	echo $LD_LIBRARY_PATH
	hostname
	git config -l
	git --version
	get_stuff_for_environment
}

function dump_server_info() {
	echo $LD_LIBRARY_PATH
	hostname
	git config -l
	git --version
	get_stuff_for_environment
}

function output_result() {
	cd $LINUX_BUILD_TOP;
	cp $OUTPUT_DIR/packed/*.bin BIN/$NVT_PRJCFG_CFG/$MODEL
	# !!! md5, .sym, .map, and list_bin.txt are for codebase statistics server !!!
	find BIN/$NVT_PRJCFG_CFG/$MODEL -name "*.bin" -exec sh -c 'x="{}"; md5sum "${x}" > "${x}.md5" ' \;
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
}

init_env > /dev/null

show_msg "=================================================================================================================================="
show_msg "Start auto-build"
show_msg "=================================================================================================================================="
cd $LINUX_BUILD_TOP;
if [ $? -ne 0 ]; then exit -1; fi;
# show msg on log-xxxxxxxx-stdout.txt for code statistic server git git log.
# because when autobuild failed, the git_log.txt will not be made.
repo forall -c 'ret=`git log -1 | grep ${gVERSION}`; if [ ! -z "$ret" ]; then git log -1; fi;'

show_msg "=================================================================================================================================="
echo $n;
dump_server_info
show_msg "=================================================================================================================================="
# use normal object(cfg_EMMC_EVB) to build fastboot model(cfg_528_RTOS_BOOT_LINUX_EVB)
cd $LINUX_BUILD_TOP;
lunch rtos cfg_528_RTOS_BOOT_LINUX_EVB gcc-6.5-newlib-2.4-2019.11-arm-ca9-eabihf
show_model_to_stderr
make all -j1
cp output/rtos-main.bin output/application.bin .
make clean -j1
lunch Linux cfg_EMMC_EVB arm-ca9-linux-gnueabihf-6.5
show_model_to_stderr
make all -j1
make publish
# for make clean vpe's o and sde'o, needing rollback their sources
pushd base/hdal/vendor/isp/drivers/source
git checkout sde
git checkout vpe
popd
pushd base/hdal/drivers/k_driver/source/kdrv_videoprocess
git checkout kdrv_vpe
git checkout kdrv_sde
popd
make clean -j1
lunch Linux cfg_528_RTOS_BOOT_LINUX_EVB arm-ca9-linux-uclibcgnueabihf-6.5
show_model_to_stderr
mkdir -p $LINUX_BUILD_TOP/BIN/$NVT_PRJCFG_CFG/$MODEL/symbol
make all -j1
output_result
pushd base/hdal
git checkout .
popd
pushd base/vos
git checkout .
popd
cd $LINUX_BUILD_TOP;
rm -rf output
rm -rf root-fs/bin/
make clean -j1

# use fastboot(cfg_528_RTOS_BOOT_LINUX_EMMC_EVB) object to build normal model(cfg_IPCAM1_RAMDISK_EVB)
cd $LINUX_BUILD_TOP;
lunch rtos cfg_528_RTOS_BOOT_LINUX_EMMC_EVB gcc-6.5-newlib-2.4-2019.11-arm-ca9-eabihf
make all -j1
cp output/rtos-main.bin output/application.bin .
make clean -j1
lunch Linux cfg_528_RTOS_BOOT_LINUX_EMMC_EVB arm-ca9-linux-uclibcgnueabihf-6.5
make all -j1
make publish
# for make clean vpe's o and sde'o, needing rollback their sources
pushd base/hdal/vendor/isp/drivers/source
git checkout sde
git checkout vpe
popd
pushd base/hdal/drivers/k_driver/source/kdrv_videoprocess
git checkout kdrv_vpe
git checkout kdrv_sde
popd
make clean -j1
lunch Linux cfg_IPCAM1_RAMDISK_EVB arm-ca9-linux-gnueabihf-6.5
show_model_to_stderr
mkdir -p $LINUX_BUILD_TOP/BIN/$NVT_PRJCFG_CFG/$MODEL/symbol
make all -j1
output_result
pushd base/hdal
git checkout .
popd
cd $LINUX_BUILD_TOP;
rm -rf output
rm -rf root-fs/bin/
make clean -j1

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

