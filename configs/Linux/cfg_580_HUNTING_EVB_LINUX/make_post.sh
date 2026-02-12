#!/bin/bash
# global settings
KEEP_SENSOR_LIST=(sen_imx290 sen_sc401ai sen_os02k10 sen_os05a10 sen_gc4653 sen_imx415)
KEEP_ISP_CFG=(isp_imx290_0.cfg isp_gc4653_0.cfg isp_os05a10_0.cfg isp_sc401ai_0.cfg isp_imx415_0.cfg)
KEEP_ISP_BIN=(lut2d_table.bin dpc_table.bin ecs_table.bin ecs_table_ir.bin)
KEEP_LCD_LIST=(disp_if8b_lcd1_pw35p00_hx8238d)

# install appfs: 1. copy isp_xxx.cfg 2.copy isp bin
pushd ${NVT_HDAL_DIR}/vendor/isp/configs/
ISP_CFG_DIR=${ROOTFS_DIR}/rootfs/etc/app/isp

mkdir -p ${ISP_CFG_DIR}
for n in "${KEEP_ISP_CFG[@]}"
do
	cp cfg/${n} ${ISP_CFG_DIR}
done
for n in "${KEEP_ISP_BIN[@]}"
do
	cp bin/${n} ${ISP_CFG_DIR}
done

popd

# remove sensor ko and install sensor cfg to /etc/app/sensor
KEEP_SENSOR_DIR=${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/keep_sensores
SENSOR_CFG_DIR=${ROOTFS_DIR}/rootfs/etc/app/sensor
mkdir -p ${SENSOR_CFG_DIR}
mkdir ${KEEP_SENSOR_DIR}
for n in "${KEEP_SENSOR_LIST[@]}"
do
	cp ${NVT_HDAL_DIR}/ext_devices/sensor/configs/cfg/${n}.cfg ${SENSOR_CFG_DIR}
	mv ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/${n} ${KEEP_SENSOR_DIR}
done
rm -rf  ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/sen_*
mv ${KEEP_SENSOR_DIR}/* ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal
rmdir ${KEEP_SENSOR_DIR}

# remove unused misc
rm -rf ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/dummy
rm -rf ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/dummy2
rm -rf ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/comm/dummy
rm -rf ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/extra/msdcnvt
rm -rf ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/extra/sample
rm -rf ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/vos/dummy
