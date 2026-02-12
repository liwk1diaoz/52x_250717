#/bin/sh
MODEL_BUILD_LIST=("lunch TestKit ModelConfig_EVB.txt mipsel-24kec-linux-uclibc-4.9-2017.07"
		"lunch EmuKit ModelConfig_EMU.txt mipsel-24kec-linux-uclibc-4.9-2017.07"
		"lunch DemoKit ModelConfig_DVCAM1_EVB.txt mipsel-24kec-linux-uclibc-4.9-2017.07"
		"lunch DemoKit ModelConfig_IPCAM1_EVB.txt mipsel-24kec-linux-uclibc-4.9-2017.07")
PREBUILD_PATH="/home/nvt02854/BitBucket/na51000_bsp/na51000_bsp"
IGNORE_LIST="application/live555/"

function show_msg() {
	echo -e "\e[1;44m[`echo $NVT_PRJCFG_MODEL_CFG | awk -F'/' '{print $NF}'`]\e[0m $1"
}

function init_env() {
	# Sync submodule source code
	git submodule init
	git submodule update
	export MULTI_CORES=4
	source build/envsetup.sh
	${MODEL_BUILD_LIST[0]}
	# Create install dir for build error
	mkdir $ROOTFS_DIR/rootfs/bin
	mkdir $ROOTFS_DIR/rootfs/etc
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

function chkunix() {
	ret=`${UITRON_DIR}/tools/recursive_dir -t lib/ --exclude-ext .o .a .bin .axf .7z .srec .ttl .png .sln .user .filters .vcxproj .sym .bat --exclude-dir .svn .git`
	if [ ! -z "$ret" ]; then
		echo -e "\e[1;31mlib UNIX file format check failed: $ret \e[0m" >&2;
		exit -1;
	fi
	ret=`${UITRON_DIR}/tools/recursive_dir -t application/ --exclude-ext .o .a .bin .axf .7z .srec .ttl .png .sln .user .filters .vcxproj .sym .bat --exclude-dir .svn .git`
	if [ ! -z "$ret" ]; then
		echo -e "\e[1;31mapplication UNIX file format check failed: $ret \e[0m" >&2;
		exit -1;
	fi
	ret=`${UITRON_DIR}/tools/recursive_dir -t linux-supplement/ --exclude-ext .o .a .bin .axf .7z .srec .ttl .png .sln .user .filters .vcxproj .sym .bat --exclude-dir .svn .git`
	if [ ! -z "$ret" ]; then
		echo -e "\e[1;31mlinux-supplement UNIX file format check failed: $ret \e[0m" >&2;
		exit -1;
	fi
	ret=`${UITRON_DIR}/tools/recursive_dir -t sample/ --exclude-ext .o .a .bin .axf .7z .srec .ttl .png .sln .user .filters .vcxproj .sym .bat --exclude-dir .svn .git`
	if [ ! -z "$ret" ]; then
		echo -e "\e[1;31msample UNIX file format check failed: $ret \e[0m" >&2;
		exit -1;
	fi
	cd ${UITRON_DIR}/MakeCommon; make chk_unix
	if [ $? -ne 0 ]; then
		echo -e "\e[1;31mitron UNIX file format check failed: $ret \e[0m" >&2;
		exit -1;
	fi
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
		chkunix
		copy_prebuild
		remove_ignore
		for n in "${MODEL_BUILD_LIST[@]}"
		do
			cd $LINUX_BUILD_TOP;
			$n;
			cd ${UITRON_DIR}/MakeCommon; make clean; make release -j4; if [ $? -ne 0 ]; then exit -1; fi;
			build_supplement;
			build_auto;
		done
		show_msg "=================================================================================================================================="
		show_msg "Finish auto-build"
		show_msg "=================================================================================================================================="
	;;
esac
