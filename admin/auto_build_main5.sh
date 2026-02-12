#/bin/sh
CONF_JSON_LIST=(
	"novatek_na51055_ipc_jason/novatek_na51055_linux_and_fastboot_sdk_uclibc.json"
	"novatek_na51055_ipc_jason/novatek_na51055_linux_sdk_uclibc.json"
	"novatek_na51055_cardv_rtos_sdk_v1.00.json"
#	"novatek_na51055_cardv_linux_sdk_v1.25.json"
)

#set -x
SDK_MAKER_GIT="http://git.novatek.com.tw/scm/ivot_bsp/ivot_sdk_maker.git"
SDK_MAKER_DIR="ivot_sdk_maker"
SDK_MAKER_OUTPUT_DIR="${SDK_MAKER_DIR}/output"
# na51055_dual_sdk
ORI_SDK_NAME=`echo $PWD | awk -F'/' '{print $(NF-2)}'`;
# na51055
ORI_CODE_NAME=`echo ${ORI_SDK_NAME} | sed 's/\(.*\)_\(.*\)_\(.*\)/\1/g'`
# dual or linux or rtos
ORI_SDK_TYPE=`echo ${ORI_SDK_NAME} | sed 's/\(.*\)_\(.*\)_\(.*\)/\2/g'`
# na51055_linux_sdk
SDK_NAME_LINUX="${ORI_CODE_NAME}_linux_sdk"
# na51055_rtos_sdk
SDK_NAME_RTOS="${ORI_CODE_NAME}_rtos_sdk"

function show_info() {
	echo "sdk name: ${ORI_SDK_NAME}"
	echo "code name: ${ORI_CODE_NAME}"
	echo "sdk type: ${ORI_SDK_TYPE}"
	echo "linux sdk name ${SDK_NAME_LINUX}"
	echo "rtos sdk name ${SDK_NAME_RTOS}"
}

function setup_sdk_maker_output {
	rm -rf ${SDK_MAKER_OUTPUT_DIR}
	mkdir -p ${SDK_MAKER_OUTPUT_DIR}
	# check if dual sdk
	if [ ${ORI_SDK_TYPE} == "dual" ]; then
		echo "dual sdk detected"
		if [ ${1} == "Linux" ]; then
			echo "make sdk1 as Linux"
			mkdir -p ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}
			cp ${SDK_DIR} ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX} -rf
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/base/linux-BSP ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/BSP
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/base/linux-code ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/code
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/base/u-boot ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/BSP/u-boot
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/base/vos ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/code/vos
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/base/hdal ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/code/hdal
			rm -rf ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/base
			if [ ${2} != ${SDK_NAME_LINUX} ]; then
				mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX} ${SDK_MAKER_OUTPUT_DIR}/${2}
			fi
		elif [ ${1} == "rtos" ]; then
			echo "make sdk1 as rtos"
			mkdir -p ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}
			cp ${SDK_DIR} ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS} -rf
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_LINUX} ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/base/rtos-BSP ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/BSP
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/base/rtos-code ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/code
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/base/u-boot ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/BSP/u-boot
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/base/vos ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/code/vos
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/base/hdal ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/code/hdal
			rm -rf ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/base
			if [ ${2} != ${SDK_NAME_RTOS} ]; then
				mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX} ${SDK_MAKER_OUTPUT_DIR}/${2}
			fi
		fi

		if [ -z ${3} ]; then
			return
		fi

		if [ ${3} == "Linux" ]; then
			echo "make sdk2 as Linux"
			mkdir -p ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}
			cp ${SDK_DIR} ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX} -rf
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/base/linux-BSP ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/BSP
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/base/linux-code ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/code
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/base/u-boot ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/BSP/u-boot
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/base/vos ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/code/vos
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/base/hdal ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/code/hdal
			rm -rf ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX}/${SDK_NAME_LINUX}/base
			if [ ${4} != ${SDK_NAME_LINUX} ]; then
				mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_LINUX} ${SDK_MAKER_OUTPUT_DIR}/${4}
			fi
		elif [ ${3} == "rtos" ]; then
			echo "make sdk2 as rtos"
			mkdir -p ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}
			cp ${SDK_DIR} ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS} -rf
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_LINUX} ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/base/rtos-BSP ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/BSP
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/base/rtos-code ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/code
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/base/u-boot ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/BSP/u-boot
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/base/vos ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/code/vos
			mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/base/hdal ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/code/hdal
			rm -rf ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS}/${SDK_NAME_RTOS}/base
			if [ ${4} != ${SDK_NAME_RTOS} ]; then
				mv ${SDK_MAKER_OUTPUT_DIR}/${SDK_NAME_RTOS} ${SDK_MAKER_OUTPUT_DIR}/${4}
			fi
		fi
	else
		echo "single sdk detected"
		mkdir -p ${SDK_MAKER_OUTPUT_DIR}/${2}
		cp ${SDK_DIR} ${SDK_MAKER_OUTPUT_DIR}/${2} -rf
	fi
}

function init_env() {
	export SDK_DIR=`pwd`/..
	export SDK_DIR=`realpath ${SDK_DIR}`
	# switch to /tmp/09221547-45e18e20837116035d603eca1de99d80
	cd ../..
	# clone into /tmp/09221547-45e18e20837116035d603eca1de99d80/ivot_sdk_maker
	rm -rf ${SDK_MAKER_DIR}
	git clone --depth 1 --single-branch ${SDK_MAKER_GIT} ${SDK_MAKER_DIR}
}

show_info
init_env
for n in "${CONF_JSON_LIST[@]}"
do
	SDK1_PRJ=`cat ${SDK_MAKER_DIR}/conf/${n} | python3 -c "import os, sys, json; print(os.path.splitext(os.path.basename(json.load(sys.stdin)['project_name']))[0])"`
	SDK1_DIR=`cat ${SDK_MAKER_DIR}/conf/${n} | python3 -c "import os, sys, json; print(os.path.splitext(os.path.basename(json.load(sys.stdin)['manifest_xml']))[0])"`
	SDK2_PRJ=`cat ${SDK_MAKER_DIR}/conf/${n} | python3 -c "import os, sys, json; conf=json.load(sys.stdin); name='' if 'sdk2' not in conf else os.path.splitext(os.path.basename(conf['sdk2']['project_name']))[0]; print(name)"`
	SDK2_DIR=`cat ${SDK_MAKER_DIR}/conf/${n} | python3 -c "import os, sys, json; conf=json.load(sys.stdin); name='' if 'sdk2' not in conf else os.path.splitext(os.path.basename(conf['sdk2']['manifest_xml']))[0]; print(name)"`
	setup_sdk_maker_output ${SDK1_PRJ} ${SDK1_DIR} ${SDK2_PRJ} ${SDK2_DIR}
	pushd  ${SDK_MAKER_DIR}
	python3 sdk_maker.py -j conf/${n} -e
	if [ $? -ne 0 ]; then
		exit -1;
	fi;
	popd
done

