include $(NVT_PRJCFG_MODEL_CFG)

MULTI_CORES ?= $(shell grep -c ^processor /proc/cpuinfo)
EXTERNAL := ${APP_DIR}/external
SHELL = /bin/bash
INSTALL_DIR = __install

ifeq ($(NVT_SDIO_WIFI), NVT_SDIO_WIFI_RTK)
ifeq ($(WIFI_RTK_MDL), WIFI_RTK_MDL_8821CS)
    _HOSTAPD_ = hostapd-8821cs_8_O_8.x_rtw-17-g894b400ab.20210716
    _WPA_SUPPLICANT_ = wpa_supplicant-8821cs_8_O_8.x_rtw-17-g894b400ab.20210716
else
    _HOSTAPD_ = hostapd-8189ftv_8_O_8.x_rtw_r31832.20190226
    _WPA_SUPPLICANT_ = wpa_supplicant-8189ftv_8_O_8.x_rtw_r31832.20190226
endif
else ifeq ($(NVT_SDIO_WIFI),  NVT_SDIO_WIFI_BRCM)
ifeq ($(WIFI_BRCM_MDL), WIFI_BRCM_MDL_43456c5_ampk6256c5)
    _HOSTAPD_ = hostapd-2.5
    _WPA_SUPPLICANT_ = wpa_supplicant-2.5
else
    _HOSTAPD_ = hostapd-2.5
    _WPA_SUPPLICANT_ = wpa_supplicant-2.5
endif
else
ifeq ($(WIFI_RTK_MDL), WIFI_RTK_MDL_8821CS)
    _HOSTAPD_ = hostapd-8821cs_8_O_8.x_rtw-17-g894b400ab.20210716
    _WPA_SUPPLICANT_ = wpa_supplicant-8821cs_8_O_8.x_rtw-17-g894b400ab.2021071
else
    _HOSTAPD_ = hostapd-8189ftv_8_O_8.x_rtw_r31832.20190226
   _WPA_SUPPLICANT_ = wpa_supplicant-8189ftv_8_O_8.x_rtw_r31832.20190226
endif
endif 

NTP  := ntp-4.2.8p4
ISCSI:= open-iscsi-2.0-873
TZCODE := tzcode
IPERF := iperf-2.0.5
IPERF3 := iperf-3.0.12
WIRELESS_TOOL := wireless_tools.30.rtl
HOSTAPD := $(_HOSTAPD_)
WPA_SUPPLICANT := $(_WPA_SUPPLICANT_)
QUECTEL_CM := quectel-CM
ZTE_CONNECT := zte_connect
MEMTESTER := memtester-4.3.0
DHD_PRIV := dhd_priv
NVT_REMOTE_FW := nvt_remote_fw
RTL8188FTV := rtl8188ftv
ANDROID_SIMG2IMG := android-simg2img-1.1.4
RNG_TOOLS := rng-tools
SYSUTILS := sysfsutils-2.1.0
UDEV := udev-070
LIBICONV := libiconv-1.16
DOSFSTOOLS := dosfstools-4.1
RTK_BT_FW_DL_TOOL := rtk_bt_fw_dl_tool

unexport CC
unexport CPP
unexport CXX
unexport LD
unexport AR
unexport NM
unexport STRIP
unexport OBJCOPY
unexport OBJDUMP
unexport RANLIB

AR="${CROSS_COMPILE}ar"
CC="${CROSS_COMPILE}gcc"
CXX="${CROSS_COMPILE}g++"
CPP="${CROSS_COMPILE}gcc"
RANLIB="${CROSS_COMPILE}ranlib"

check_exist = if [ ! -e $(1)/.nvt_finish ]; then rm -rf $(1); tar -jxvf $(1).tar.bz2; fi
check_exist_cp_bin = if [ -e $(INSTALL_DIR)/$(1) ]; then mkdir -p ${ROOTFS_DIR}/rootfs/$(1); cp -arfv $(INSTALL_DIR)/$(1)/* ${ROOTFS_DIR}/rootfs/$(1); fi
check_exist_cp_lib = if [ -e $(INSTALL_DIR)/$(1) ]; then cp -arfv $(INSTALL_DIR)/$(1)/* ${ROOTFS_DIR}/rootfs/$(1); fi
check_exist_cp_etc = if [ -e $(INSTALL_DIR)/$(1) ]; then mkdir -p ${ROOTFS_DIR}/rootfs/$(1); cp -narv $(INSTALL_DIR)/$(1)/* ${ROOTFS_DIR}/rootfs/$(1); fi
check_exist_lib_rm = if [ -e $(INSTALL_DIR)/lib/$(1) ]; then rm -rf $(INSTALL_DIR)/lib/$(1); fi


.PHONY: $(NVT_CFG_APP_EXTERNAL)
all: $(NVT_CFG_APP_EXTERNAL)


ifneq ($(findstring RTK,$(BT_MDL)),)
ifeq ($(findstring ${RTK_BT_FW_DL_TOOL},$(NVT_CFG_APP_EXTERNAL)),)

all: ${RTK_BT_FW_DL_TOOL}
.PHONY: ${RTK_BT_FW_DL_TOOL}
NVT_CFG_APP_EXTERNAL += " ${RTK_BT_FW_DL_TOOL}"

endif
endif

list:
	@echo $(NVT_CFG_APP_EXTERNAL)

init:
	@if [ ! -e "${EXTERNAL}/${INSTALL_DIR}" ]; then \
		mkdir ${EXTERNAL}/${INSTALL_DIR}; \
	fi
	@if [ -z "${CROSS_COMPILE}" ]; then \
		echo "Please execute \"source build/envsetup.sh\" firstly.  Stop."; exit 1; \
	fi

libiconv: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_exist, ${LIBICONV})
	@cd ${LIBICONV}; ./configure --prefix=${EXTERNAL}/${INSTALL_DIR} --host=${NVT_HOST}; make -j${MULTI_CORES}; make install
	@touch ${LIBICONV}/.nvt_finish
  
dosfstools: init libiconv
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_exist, ${DOSFSTOOLS})
	@cd ${DOSFSTOOLS}; ./configure --prefix=${EXTERNAL}/${INSTALL_DIR} --host=${NVT_HOST} --without-udev LDFLAGS="-L${EXTERNAL}/${INSTALL_DIR}/lib -liconv" CFLAGS="-I${EXTERNAL}/${INSTALL_DIR}/include"; make -j${MULTI_CORES}; make install
	@rm -rf ${EXTERNAL}/${INSTALL_DIR}/include
	@touch ${DOSFSTOOLS}/.nvt_finish

ntp: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	@if [ ! -f ${NTP}/.nvt_finish ]; then \
		$(call check_exist, ${NTP}); \
		cd ${NTP}; ./configure CPPFLAGS="-I${EXTERNAL}/${INSTALL_DIR}/include -I${INCLUDE_DIR}" LDFLAGS="-L${LIBRARY_DIR} -L${EXTERNAL}/${INSTALL_DIR}/lib" --without-crypto --with-yielding-select=yes --prefix=${EXTERNAL}/${INSTALL_DIR} --host=${NVT_HOST}; make -j$(MULTI_CORES); \
	fi
	@cd ${NTP}; make install;
	@cd ${EXTERNAL}/${INSTALL_DIR}/bin;${CROSS_COMPILE}strip ntpdate;rm ntpd;rm ntpdc;rm ntp-keygen;rm ntpq;rm ntptime;rm ntptrace;rm ntp-wait;rm sntp;rm tickadj;rm update-leap;
	@touch ${NTP}/.nvt_finish

iscsi: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	@if [ ! -f ${ISCSI}/.nvt_finish ]; then \
		$(call check_exist, ${ISCSI}); \
		cd ${ISCSI}/utils/open-isns; ./configure --host=${NVT_HOST} --with-security=no; \
		cd ${EXTERNAL}/${ISCSI}; make -j$(MULTI_CORES) CC=${CROSS_COMPILE}gcc; mkdir -p ${EXTERNAL}/${INSTALL_DIR}/etc/iscsi; \
	fi
	@cd ${ISCSI}; make install DESTDIR=${EXTERNAL}/${INSTALL_DIR};
	@cd ${EXTERNAL}/${INSTALL_DIR}/sbin; rm iscsi-iname; rm iscsi_discovery;
	@touch ${ISCSI}/.nvt_finish

tzcode: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	@cd ${TZCODE}; make -j$(MULTI_CORES) CC=${CC} NM=${NM} STRIP=${STRIP} OBJCOPY=${OBJCOPY}; make install;

iperf-2: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	@if [ ! -f ${IPERF}/.nvt_finish ]; then \
		$(call check_exist, ${IPERF}); \
		cd ${IPERF}; ./configure  --host=${NVT_HOST} --prefix=${EXTERNAL}/${INSTALL_DIR}; make -j$(MULTI_CORES); \
	fi
	@cd ${IPERF}; make install;
	@touch ${IPERF}/.nvt_finish

iperf-3: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	@mkdir -p ${EXTERNAL}/${INSTALL_DIR}/usr/bin ${EXTERNAL}/${INSTALL_DIR}/lib;
	@if [ ! -f ${IPERF3}/.nvt_finish ]; then \
                $(call check_exist, ${IPERF3}); \
                cd ${IPERF3}; ./configure  --host=${NVT_HOST} --build=i686-linux --prefix=${EXTERNAL}/${INSTALL_DIR}; make -j$(MULTI_CORES); \
        fi
	@cp -r ${IPERF3}/src/.libs/iperf3 ${EXTERNAL}/${INSTALL_DIR}/usr/bin/
	@cp -r ${IPERF3}/src/.libs/libiperf.so* ${EXTERNAL}/${INSTALL_DIR}/lib/
	@touch ${IPERF3}/.nvt_finish

wireless_tool: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	@if [ ! -f ${WIRELESS_TOOL}/.nvt_finish ]; then \
		$(call check_exist, ${WIRELESS_TOOL}); \
		cd ${WIRELESS_TOOL}; make -j$(MULTI_CORES) PREFIX=${EXTERNAL}/${INSTALL_DIR} CC=${CC} AR=${AR}; \
	fi
	@cd ${WIRELESS_TOOL}; make PREFIX=${EXTERNAL}/${INSTALL_DIR} CC=${CC} AR=${AR} install-bin install-dynamic;
	@touch ${WIRELESS_TOOL}/.nvt_finish

wpa_supplicant: init
ifeq ($(NVT_SDIO_WIFI), NVT_SDIO_WIFI_BRCM)
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling BRCM WIFI>>>>>>>>>>>>>>>>>>>"
	@if [ ! -f ${WPA_SUPPLICANT}/.nvt_finish ]; then \
		$(call check_exist, ${WPA_SUPPLICANT}); \
		cd ${WPA_SUPPLICANT}/wpa_supplicant; make CC=${CC} LD=${LD} AR=${AR}; \
	fi
	rm -rf ${EXTERNAL}/${INSTALL_DIR}/tmp/
	cd ${WPA_SUPPLICANT}/wpa_supplicant; make DESTDIR=${EXTERNAL}/${INSTALL_DIR}/tmp BINDIR=/bin install;
	
	cp -arfv ${EXTERNAL}/${INSTALL_DIR}/tmp/* ${EXTERNAL}/${INSTALL_DIR}/
	@touch ${WPA_SUPPLICANT}/.nvt_finish	
else ifeq ($(NVT_SDIO_WIFI), NVT_SDIO_WIFI_RTK)
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling RTK WIFI>>>>>>>>>>>>>>>>>>>"
	mkdir -p ${EXTERNAL}/${INSTALL_DIR}/tmp/usr/bin/librtw/
	mkdir -p ${EXTERNAL}/${INSTALL_DIR}/bin/
	@if [ ! -f ${WPA_SUPPLICANT}/.nvt_finish ]; then \
		$(call check_exist, ${WPA_SUPPLICANT}); \
		cd ${EXTERNAL}/${WPA_SUPPLICANT}/librtw; make -j$(MULTI_CORES) CC=${CC} LD=${LD} DESTDIR=${EXTERNAL}/${INSTALL_DIR}/tmp BINDIR=/usr/bin all; \
		cd ${EXTERNAL}/${WPA_SUPPLICANT}/wpa_supplicant; make CC=${CC} LD=${LD} DESTDIR=${EXTERNAL}/${INSTALL_DIR}/tmp BINDIR=/usr/bin wpa_cli; \
		cd ${EXTERNAL}/${WPA_SUPPLICANT}/wpa_supplicant; make CC=${CC} LD=${LD} DESTDIR=${EXTERNAL}/${INSTALL_DIR}/tmp BINDIR=/usr/bin wpa_supplicant; \
	fi

	cp ${WPA_SUPPLICANT}/wpa_supplicant/wpa_cli ${EXTERNAL}/${INSTALL_DIR}/bin/
	cp ${WPA_SUPPLICANT}/wpa_supplicant/wpa_supplicant ${EXTERNAL}/${INSTALL_DIR}/bin/
	
	cp -arfv ${EXTERNAL}/${INSTALL_DIR}/tmp/* ${EXTERNAL}/${INSTALL_DIR}/
	@touch ${WPA_SUPPLICANT}/.nvt_finish
endif

hostapd: init
ifeq ($(NVT_SDIO_WIFI), NVT_SDIO_WIFI_BRCM)
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling BRCM WIFI>>>>>>>>>>>>>>>>>>>"
	@if [ ! -f ${HOSTAPD}/.nvt_finish ]; then \
		$(call check_exist, ${HOSTAPD}); \
		cd ${HOSTAPD}/hostapd; make CC=${CC} LD=${LD}; \
	fi
	rm -rf ${EXTERNAL}/${INSTALL_DIR}/tmp/
	cd ${HOSTAPD}/hostapd; make DESTDIR=${EXTERNAL}/${INSTALL_DIR}/tmp BINDIR=/usr/bin install;	
	
	cp -arfv ${EXTERNAL}/${INSTALL_DIR}/tmp/* ${EXTERNAL}/${INSTALL_DIR}/
	@touch ${HOSTAPD}/.nvt_finish	
else ifeq ($(NVT_SDIO_WIFI), NVT_SDIO_WIFI_RTK)
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling RTK WIFI>>>>>>>>>>>>>>>>>>>"
	mkdir -p ${EXTERNAL}/${INSTALL_DIR}/tmp/usr/bin/librtw/
	mkdir -p ${EXTERNAL}/${INSTALL_DIR}/bin/
	@if [ ! -f ${HOSTAPD}/.nvt_finish ]; then \
		$(call check_exist, ${HOSTAPD}); \
		cd ${EXTERNAL}/${HOSTAPD}/librtw; make -j$(MULTI_CORES) CC=${CC} LD=${LD} DESTDIR=${EXTERNAL}/${INSTALL_DIR}/tmp BINDIR=/usr/bin all; \
		cd ${EXTERNAL}/${HOSTAPD}/hostapd; make -j$(MULTI_CORES) CC=${CC} LD=${LD} DESTDIR=${EXTERNAL}/${INSTALL_DIR}/tmp BINDIR=/usr/bin hostapd hostapd_cli; \
		cd ${EXTERNAL}/${HOSTAPD}/wpa_supplicant; make -j$(MULTI_CORES) CC=${CC} LD=${LD} DESTDIR=${EXTERNAL}/${INSTALL_DIR}/tmp BINDIR=/usr/bin wpa_cli; \
	fi

	cp ${HOSTAPD}/wpa_supplicant/wpa_cli ${EXTERNAL}/${INSTALL_DIR}/bin/
	cp ${HOSTAPD}/hostapd/hostapd ${EXTERNAL}/${INSTALL_DIR}/bin/
	cp ${HOSTAPD}/hostapd/hostapd_cli ${EXTERNAL}/${INSTALL_DIR}/bin/
	
	cp -arfv ${EXTERNAL}/${INSTALL_DIR}/tmp/* ${EXTERNAL}/${INSTALL_DIR}/
	@touch ${HOSTAPD}/.nvt_finish
endif

quectel-CM: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_exist, ${QUECTEL_CM})
	@cd ${QUECTEL_CM}; make -j$(MULTI_CORES) CC=${CC} NM=${NM} STRIP=${STRIP} OBJCOPY=${OBJCOPY}; make install;

memtester: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_exist, ${MEMTESTER})
	@mkdir -p ${EXTERNAL}/${INSTALL_DIR}/bin;
	@cd ${MEMTESTER}; echo "${CC} -O2 -DPOSIX -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64 -DTEST_NARROW_WRITES -c" > conf-cc; echo "${CC} -s" > conf-ld; make -j$(MULTI_CORES);
	@cd ${MEMTESTER}; make DESTDIR=${EXTERNAL}/${INSTALL_DIR} install;
	@touch ${MEMTESTER}/.nvt_finish

dhd_priv: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	@if [ ! -f ${DHD_PRIV}/.nvt_finish ]; then \
		$(call check_exist, ${DHD_PRIV}); \
		cd ${DHD_PRIV}; make -j$(MULTI_CORES) CC=${CC} LD=${LD}; \
	fi
	@mkdir -p ${EXTERNAL}/${INSTALL_DIR}/usr/local/bin;
	@cp -r ${DHD_PRIV}/dhd_priv ${EXTERNAL}/${INSTALL_DIR}/usr/local/bin;
	@touch ${DHD_PRIV}/.nvt_finish

nvt_remote_fw: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	@if [ ! -f ${NVT_REMOTE_FW}/.nvt_finish ]; then \
		$(call check_exist, ${NVT_REMOTE_FW}); \
		cd ${NVT_REMOTE_FW}; make -j$(MULTI_CORES) CC=${CC} LD=${LD}; \
	fi
	@mkdir -p ${EXTERNAL}/${INSTALL_DIR}/usr/local/bin;
	@cp -r ${NVT_REMOTE_FW}/nvtfwcmd.out ${EXTERNAL}/${INSTALL_DIR}/usr/local/bin;
	@cp -r ${NVT_REMOTE_FW}/nvtfwlog_server.out ${EXTERNAL}/${INSTALL_DIR}/usr/local/bin;
	@touch ${DHD_PRIV}/.nvt_finish

rtl8188ftv: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	$(call check_exist, ${RTL8188FTV})
	@mkdir -p ${EXTERNAL}/${INSTALL_DIR}/usr/bin ${EXTERNAL}/${INSTALL_DIR}/lib;
	@cd ${RTL8188FTV}; make -j$(MULTI_CORES);
	@cp -r ${RTL8188FTV}/8188fu.ko ${EXTERNAL}/${INSTALL_DIR}/lib/;
	@cd ${RTL8188FTV}/wpa_supplicant_0.8/wpa_supplicant;make CC=${CC};
	@cp -r ${RTL8188FTV}/wpa_supplicant_0.8/wpa_supplicant/wpa_supplicant  ${RTL8188FTV}/wpa_supplicant_0.8/wpa_supplicant/wpa_passphrase ${RTL8188FTV}/wpa_supplicant_0.8/wpa_supplicant/wpa_cli ${EXTERNAL}/${INSTALL_DIR}/usr/bin/;
	@touch ${RTL8188FTV}/.nvt_finish

android-simg2img: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	@if [ ! -f ${ANDROID_SIMG2IMG}/.nvt_finish ]; then \
		$(call check_exist, ${ANDROID_SIMG2IMG}); \
		cd ${ANDROID_SIMG2IMG}; make -j$(MULTI_CORES) PREFIX=${EXTERNAL}/${INSTALL_DIR} CXX=${CXX} DEP_CXX=${CXX} LD=${LD} AR=${AR} RANLIB=${RANLIB}; \
	fi
	@cd ${ANDROID_SIMG2IMG}; make install PREFIX=${EXTERNAL}/${INSTALL_DIR} CXX=${CXX} DEP_CXX=${CXX} LD=${LD} AR=${AR} RANLIB=${RANLIB};
	@touch ${ANDROID_SIMG2IMG}/.nvt_finish

sysutils: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	@if [ ! -f ${SYSUTILS}/.nvt_finish ]; then \
		$(call check_exist, ${SYSUTILS}); \
		cd ${SYSUTILS}; ./configure --host=${NVT_HOST} --prefix=${EXTERNAL}/${INSTALL_DIR};\
	fi
	@cd ${SYSUTILS}; make; make install;
	@touch ${SYSUTILS}/.nvt_finish

rng_tools: init sysutils
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	@if [ ! -f ${RNG_TOOLS}/.nvt_finish ]; then \
		$(call check_exist, ${RNG_TOOLS}); \
		cd ${RNG_TOOLS}; ./configure CPPFLAGS="-I${EXTERNAL}/${INSTALL_DIR}/include -I${INCLUDE_DIR}" LDFLAGS="-L${LIBRARY_DIR}/external/__install/lib -L${EXTERNAL}/${INSTALL_DIR}/lib" --host=${NVT_HOST} --prefix=${EXTERNAL}/${INSTALL_DIR} --without-libgcrypt --without-nistbeacon; \
	fi
	@cd ${RNG_TOOLS}; make -j$(MULTI_CORES); make install;
	@touch ${RNG_TOOLS}/.nvt_finish

udev: init
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	if [ ! -f ${UDEV}/.nvt_finish ]; then \
		$(call check_exist, ${UDEV}); \
		cd ${UDEV}; make -j$(MULTI_CORES) CROSS_COMPILE=${CROSS_COMPILE} PREFIX=${EXTERNAL}/${INSTALL_DIR} CXX=${CXX}; \
	fi
	@cd ${UDEV}; make install-bin DESTDIR=${EXTERNAL}/${INSTALL_DIR}; if [ -d ${ROOTFS_DIR}/rootfs/etc ]; then make install-config DESTDIR=${ROOTFS_DIR}/rootfs; fi;
	@touch ${UDEV}/.nvt_finish

rtk_bt_fw_dl_tool:
	@echo ">>>>>>>>>>>>>>>>>>> $@ compiling >>>>>>>>>>>>>>>>>>>"
	cd ${RTK_BT_FW_DL_TOOL}; \
	${MAKE} -j$(MULTI_CORES) CROSS_COMPILE=${CROSS_COMPILE}; \
	cp rtk_hciattach/rtk_hciattach ${ROOTFS_DIR}/rootfs/usr/bin/ ;

install:
	@echo ">>>>>>>>>>>>>>>>>>> $@ >>>>>>>>>>>>>>>>>>>"
	@$(call check_exist_cp_bin,bin)
	@$(call check_exist_cp_bin,sbin)
	@$(call check_exist_cp_bin,usr/bin)
	@$(call check_exist_cp_bin,usr/sbin)
	@$(call check_exist_cp_bin,usr/local/bin)
	@$(call check_exist_cp_lib,lib)
	@$(call check_exist_cp_lib,include)

clean:
	@echo ">>>>>>>>>>>>>>>>>>> Remove >>>>>>>>>>>>>>>>>>>"
	@rm -rf ${EXTERNAL}/${INSTALL_DIR} ${CURL} ${NTP} ${ISCSI} ${IPERF} ${WIRELESS_TOOL} ${_WPA_SUPPLICANT_} ${_HOSTAPD_} ${QUECTEL_CM} ${ZTE_CONNECT} ${MEMTESTER} ${DHD_PRIV} ${NVT_REMOTE_FW} ${RTL8188FTV} ${ANDROID_SIMG2IMG} ${SYSUTILS} ${RNG_TOOLS} ${IPERF3} ${UDEV} ${LIBICONV} ${DOSFSTOOLS}
	@cd ${RTK_BT_FW_DL_TOOL}; ${MAKE} clean; rm -f .nvt_finish
