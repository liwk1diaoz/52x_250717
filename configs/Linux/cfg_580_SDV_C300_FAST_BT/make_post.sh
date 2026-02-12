#!/bin/bash
# global settings
KEEP_SENSOR_LIST=(sen_imx335)
KEEP_LCD_LIST=(disp_ifdsi_lcd1_st7701_c300 disp_if8b_lcd1_st7789_c300)
SAMPLE_INSTALL_LIST=(rc_profile)

# install appfs
pushd ${NVT_HDAL_DIR}/samples/vendor_cfg
make install
popd

# install hd_video_record_with_fastboot sample
for n in "${SAMPLE_INSTALL_LIST[@]}"
do
	pushd ${NVT_HDAL_DIR}/samples/${n}
	make install
	popd
done

# remove sensor ko and install sensor cfg to /usr/local/etc/
KEEP_SENSOR_DIR=${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/keep_sensores
#SENSOR_CFG_DIR=${ROOTFS_DIR}/rootfs/usr/local/etc
SENSOR_CFG_DIR=${ROOTFS_DIR}/rootfs/etc/sensor
mkdir -p ${SENSOR_CFG_DIR}
mkdir ${KEEP_SENSOR_DIR}
for n in "${KEEP_SENSOR_LIST[@]}"
do
#	cp ${APP_DIR}/source/cardv/output/appfs/sensor/${n}.cfg ${SENSOR_CFG_DIR}
    cp ${APP_DIR}/source/cardv/output/appfs/sensor/sen_imx335_c300.cfg ${SENSOR_CFG_DIR}
	mv ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/${n} ${KEEP_SENSOR_DIR}
done
rm -rf  ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/sen_*
mv ${KEEP_SENSOR_DIR}/* ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal
rmdir ${KEEP_SENSOR_DIR}

# remove unused lcd ko
KEEP_LCD_DIR=${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/display_panel/keep_lcds
mkdir ${KEEP_LCD_DIR}
for n in "${KEEP_LCD_LIST[@]}"
do
	mv ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/display_panel/${n} ${KEEP_LCD_DIR}
done
rm -rf  ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/display_panel/disp_*
mv ${KEEP_LCD_DIR}/* ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/display_panel
rmdir ${KEEP_LCD_DIR}

# remove unused misc
rm -rf ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/dummy
rm -rf ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/dummy2
rm -rf ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/hdal/comm/dummy
rm -rf ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/extra/msdcnvt
rm -rf ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/extra/sample
rm -rf ${ROOTFS_DIR}/rootfs/lib/modules/${NVT_LINUX_VER}/vos/dummy
