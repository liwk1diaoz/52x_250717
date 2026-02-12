# To build modules outside of the kernel tree, we run "make"
# in the kernel source tree; the Makefile these then includes this
# Makefile once again.
# This conditional selects whether we are being included from the
# kernel Makefile or not.

include $(NVT_PRJCFG_MODEL_CFG)

ifeq ($(BT_HCI_IF), BT_HCI_IF_UART)
	BT_HCI_IF_DIR=uart
endif

ifeq ($(BT_MDL), BT_MDL_RTK_8821CS)
	BT_MDL_DIR=rtl8821CS_20220330_LINUX_BT_DRIVER_RTL8821C_COEX_v5555
endif

ifneq ($(BT_MDL_DIR),)
	ifneq ($(BT_HCI_IF_DIR),)	
		BT_DIR=bt/$(BT_MDL_DIR)/$(BT_HCI_IF_DIR)
	endif		
endif

ifeq ($(KERNELRELEASE),)
	export KBUILD_EXTRA_SYMBOLS = $(shell find $(NVT_HDAL_DIR)/drivers -name Module.symvers)
	# SUBDIRS := $(dir $(wildcard */*/Makefile */*/makefile */Makefile))
	SUBDIRS := $(dir $(shell find . -name Makefile))
	SUBDIRS := $(shell echo $(SUBDIRS) | sed -e 's/\.\///g';)
	# The current directory is passed to sub-makes as argument
	PWD := $(shell pwd)

modules:
	@$(MAKE) -C $(KERNELDIR) M=$(PWD) modules $(NVT_MULTI_CORES_FLAG);

modules_install:
	@if [ -z $(NVT_MOD_INSTALL) ]; then \
		$(MAKE) -C $(KERNELDIR) M=$(PWD) INSTALL_MOD_PATH=./_install_modules/ INSTALL_MOD_DIR=extra modules_install; \
	else \
		$(MAKE) -C $(KERNELDIR) M=$(PWD) INSTALL_MOD_PATH=$(NVT_MOD_INSTALL) INSTALL_MOD_DIR=extra modules_install; \
	fi

	@if [ ! -z $(BT_DIR)  ]; then \
			mkdir -p ${ROOTFS_DIR}/rootfs/usr/share/btscripts; \
			cp -ar $(BT_DIR)/scripts/* ${ROOTFS_DIR}/rootfs/usr/share/btscripts/; \
			cp -ar bt/init.d/* ${ROOTFS_DIR}/rootfs/etc/init.d/; \
	fi

clean:
	@for n in $(SUBDIRS); do \
		SRC="`find $$n -name  *.c ! -name *.mod.c`"; \
		if [ "$$SRC" ]; then \
			rm -rf .tmp_versions Module.symvers modules.order `find $$n -type f -name *.mod.c -o -name .*.cmd -o -name *.o -o -name *.o.d -o -name *.ko -o -name modules.order -o -name *~ -o -name .Makefile.swp -o -name Module.symvers`; \
		else \
			echo ">>> Skip"; \
		fi \
	done

.PHONY: modules modules_install clean

else

ifeq ($(NVT_SDIO_WIFI), NVT_SDIO_WIFI_BRCM)
	NET_DIR = bcmdhd.100.10.545.x
else
ifeq ($(WIFI_RTK_MDL), WIFI_RTK_MDL_8189)
	NET_DIR = rtl8189fs
else ifeq ($(WIFI_RTK_MDL), WIFI_RTK_MDL_8821CS)
	NET_DIR = rtl8821CS_WiFi_linux_v5.14.2-28-g6011b0372.20220328_COEX20210319-5555
endif
endif

# called from kernel build system: just declare what our modules are
ifneq ($(NVT_SDIO_WIFI), NVT_SDIO_WIFI_NONE)
obj-m += net/$(NET_DIR)/
endif

ifneq ($(BT_DIR),)
	obj-m += $(BT_DIR)/
endif

obj-m += \
	sample/fake/ \
	crypto/cryptodev-linux/ \
	msdcnvt/msdcnvt/ \
	msdcnvt/msdcnvt_adj/ \
	msdcnvt/msdcnvt_custom_si/ \
	touch/touch_gt911/ \
	touch/touch_sj10/ \
	touch/touch_c300/ \
	touch/touch2_c300/ \
	lcd_backlight_pwm/
	
#obj-$(CONFIG_HAVE_HW_BREAKPOINT) += debug/nvt_data_breakpoint/

endif
