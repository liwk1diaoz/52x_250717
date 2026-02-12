#!/bin/bash
#############################################################################
function getNvtCfgVal () { 
	#cat $LINUX_BUILD_TOP/admin/configs/config_master.json | python -c "import json,sys;sys.stdout.write(json.dumps(json.load(sys.stdin)$1))";
	if [ -f $LINUX_BUILD_TOP/admin/.nvt_cfg ]; then
		config_file=`cat $LINUX_BUILD_TOP/admin/.nvt_cfg`
		if [ -f $config_file ]; then
			cat $config_file | python -c "import json,sys;json_data=json.load(sys.stdin);print(json_data$1)";
		else
			exit 1;
		fi
	else
		exit 1;
	fi
}
#############################################################################
#	Json setting.
#############################################################################
pushd $LINUX_BUILD_TOP/admin/configs
total_json_cfg_name=`ls config_*`
popd

if [ ! -z "$1" ]; then
	echo -e "\e[1;33m\rConfig = $LINUX_BUILD_TOP/admin/configs/$1 \r\e[0m"
	echo $LINUX_BUILD_TOP/admin/configs/$1 > $LINUX_BUILD_TOP/admin/.nvt_cfg
else
	if [ -f $LINUX_BUILD_TOP/admin/.nvt_cfg ]; then
		echo -e "\e[1;33m\rIs this config? y/n (`cat $LINUX_BUILD_TOP/admin/.nvt_cfg`)\r\e[0m"
		read answer
	fi
	if [ "$answer" != "y" ]; then
		echo -e "\e[1;33m\rPlease select your config?\r\e[0m"
		SELECTION_LIST=" "
		while [ 1 ]; do
			i=0
			for n in $total_json_cfg_name
			do
				found=0
				if [ ! -z $SELECTION_LIST ]; then
					if [ $SELECTION_LIST -eq $i ]; then
						echo -e "\e[1;44m$i". "$n\e[0m";
						found=1
					fi
				fi
				if [ $found -ne 1 ]; then
					echo $i". "$n

				fi
				i=$(($i+1))
			done
			echo $i". Exit"
			read answer;
			# checking for illegal and exit
			if [ $answer -ge 0 ] && [ $answer -lt  $i ]; then
				SELECTION_LIST=$answer
			elif [ $answer -eq $i ]; then
				break
			else
				continue



			fi

		done
		if [ ! -z $SELECTION_LIST ]; then
			i=0
			for n in $total_json_cfg_name
			do
				if [ $SELECTION_LIST -eq $i  ]; then
					echo $LINUX_BUILD_TOP/admin/configs/$n > $LINUX_BUILD_TOP/admin/.nvt_cfg
					break;
				fi
				i=$(($i+1))
			done









		fi











	fi
fi

SDK_PACK_COMP_NAME="`getNvtCfgVal "['nvt_packcfg']['bsp_name']"`_`date +%Y%m%d`"
if [ ! -z "`getNvtCfgVal "['nvt_packcfg']['customer']"`" ]; then
	SDK_PACK_COMP_NAME="${SDK_PACK_COMP_NAME}_`getNvtCfgVal "['nvt_packcfg']['customer']"`"
fi

if [ ! -z "`getNvtCfgVal "['nvt_packcfg']['rev']"`" ]; then
	SDK_PACK_COMP_NAME="${SDK_PACK_COMP_NAME}_`getNvtCfgVal "['nvt_packcfg']['rev']"`"
fi

if [ ! -z "`echo ${CROSS_COMPILE} | grep uclibc`" ]; then
	SDK_PACK_COMP_NAME_LIBVER="${SDK_PACK_COMP_NAME}.02"
else





	SDK_PACK_COMP_NAME_LIBVER="${SDK_PACK_COMP_NAME}.01"


fi

SDK_PREV_PACK_COMP_NAME="`getNvtCfgVal "['nvt_packcfg']['prev_tag']"`"
SDK_LINUX_SRC_RELEASE="`getNvtCfgVal "['nvt_packcfg']['release_linux_src']"`"
SDK_LINUX_SRC_RELEASE_APP_EXCLUDE_LIST="`getNvtCfgVal "['nvt_packcfg']['release_linux_app_exclude_list']"`"
SDK_LINUX_SRC_RELEASE_APP_LIST="`getNvtCfgVal "['nvt_packcfg']['release_linux_app_list']"`"
SDK_LINUX_SRC_RELEASE_LIB_REMOVE_LIST="`getNvtCfgVal "['nvt_packcfg']['release_linux_lib_remove_list']"`"
SDK_LINUX_SRC_RELEASE_LIB_EXCLUDE_LIST="`getNvtCfgVal "['nvt_packcfg']['release_linux_lib_exclude_list']"`"
SDK_LINUX_SRC_RELEASE_LIB_LIST="`getNvtCfgVal "['nvt_packcfg']['release_linux_lib_list']"`"
SDK_LINUX_SRC_RELEASE_SUPPLEMENT_LIST="`getNvtCfgVal "['nvt_packcfg']['release_linux_supplement_list']"`"
SDK_LINUX_SRC_RELEASE_SUPPLEMENT_REMOVE_LIST="`getNvtCfgVal "['nvt_packcfg']['release_linux_supplement_remove_list']"`"
SDK_ITRON_SRC_RELEASE_SCRIPT="`getNvtCfgVal "['nvt_packcfg']['release_itron_script']"`"
SDK_MULTI_MODEL="`getNvtCfgVal "['nvt_packcfg']['multi_model']"`"
SDK_EXCLUDE_BIN_LIST="`getNvtCfgVal "['nvt_packcfg']['exclude_bin_list']"`"
