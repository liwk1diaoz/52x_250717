KDRV_DIR=$(NVT_HDAL_DIR)/drivers/k_driver
KFLOW_DIR=$(NVT_HDAL_DIR)/drivers/k_flow
SUBDIRS := $(dir $(shell find . -name Makefile))
SUBDIRS := $(shell echo $(SUBDIRS) | sed -e 's/\.\///g';)
# The current directory is passed to sub-makes as argument
PWD := $(shell pwd)
.PHONY: modules modules_install clean $(SUBDIRS)
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
# To build modules outside of the kernel tree, we run "make"
# in the kernel source tree; the Makefile these then includes this
# Makefile once again.
# This conditional selects whether we are being included from the
# kernel Makefile or not.
ifeq ($(KERNELRELEASE),)
export KBUILD_EXTRA_SYMBOLS = $(shell find $(VOS_DRIVER_DIR) -name Module.symvers) $(shell find $(KDRV_DIR) -name Module.symvers) $(shell find $(KFLOW_DIR) -name Module.symvers)

modules:
	@$(MAKE) -C $(KERNELDIR) M=$(PWD) modules $(NVT_MULTI_CORES_FLAG);

modules_install:
	@if [ -z $(NVT_MOD_INSTALL) ]; then \
		$(MAKE) -C $(KERNELDIR) M=$(PWD) INSTALL_MOD_PATH=./_install_modules/ INSTALL_MOD_DIR=hdal modules_install; \
	else \
		$(MAKE) -C $(KERNELDIR) M=$(PWD) INSTALL_MOD_PATH=$(NVT_MOD_INSTALL) INSTALL_MOD_DIR=hdal modules_install; \
	fi

clean:
	@rm -rf Module.symvers modules.order .tmp_versions
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n clean $(NVT_MULTI_CORES_FLAG); \
	done

else
# called from kernel build system: just declare what our modules are
obj-m += \
	sen_off/ \
	sen_ar0221/ \
	sen_ar0237/ \
	sen_ar0237ir/ \
	sen_ar0144/ \
	sen_gc030a/ \
	sen_gc2053/ \
	sen_gc2053_slave/ \
	sen_gc2083/ \
	sen_gc2093/ \
	sen_gc2375/ \
	sen_gc3003/ \
	sen_gc4c33/ \
	sen_gc4023/ \
	sen_gc4653/ \
	sen_gc4653_slave/ \
	sen_gc4663/ \
	sen_gc5035/ \
	sen_gc5603_slave/ \
	sen_gc8034/ \
	sen_ov12d/ \
	sen_ov13b10/ \
	sen_ov2715/ \
	sen_ov2718/ \
	sen_ov2775/ \
	sen_ov2736ir/ \
	sen_ov2740/ \
	sen_ov48b/ \
	sen_ov12895/ \
	sen_os02d10/ \
	sen_os02g10/ \
	sen_os02k10/ \
	sen_os02k10_slave/ \
	sen_os02h10/ \
	sen_os03b10/ \
	sen_os04a10/ \
	sen_os04a10_slave/ \
	sen_os04b10/ \
	sen_os04c10/ \
	sen_os04e10/ \
	sen_os05a10/ \
	sen_os05b10/ \
	sen_os08a10/ \
	sen_os08b10/ \
	sen_os12d40/ \
	sen_ps5268/ \
	sen_f35/ \
	sen_f37/ \
	sen_f37p/ \
	sen_f51/ \
	sen_f53/ \
	sen_imx178/ \
	sen_imx265/ \
	sen_imx307/ \
	sen_imx307_slave/ \
	sen_imx230/ \
	sen_imx258/ \
	sen_imx290/ \
	sen_imx305/ \
	sen_imx317/ \
	sen_imx322/ \
	sen_imx326/ \
	sen_imx327/ \
	sen_imx334/ \
	sen_imx335/ \
	sen_imx347/ \
	sen_imx385/ \
	sen_imx415/ \
	sen_imx464/ \
	sen_imx485/ \
	sen_imx577/ \
	sen_imx586/ \
	sen_imx662/ \
	sen_imx675/ \
	sen_imx678/ \
	sen_jinglin_ti_module/ \
	sen_jinglin_ti_raw_module/ \
	sen_k305p_slave/ \
	sen_q03p/ \
	sen_s5k231y/ \
	sen_s5k4h7/ \
	sen_sc200ai/ \
	sen_sc230ai/ \
	sen_sc301iot/ \
	sen_sc301iot_slave/ \
	sen_sc302iot/ \
	sen_sc400ai/ \
	sen_sc401ai/ \
	sen_sc430ai/ \
	sen_sc450ai/ \
	sen_sc450ai_slave/ \
	sen_sc500ai/ \
	sen_sc501ai_slave/ \
	sen_sc500cs/ \
	sen_sc530ai/ \
	sen_sc630ai/ \
	sen_sc632ai/ \
	sen_sc830ai/ \
	sen_sc850sl/ \
	sen_sc2210/ \
	sen_sc223a/ \
	sen_sc223a_slave/ \
	sen_sc2232h/ \
	sen_sc2239/ \
	sen_sc2310/ \
	sen_sc2335/ \
	sen_sc233a/ \
	sen_sc2335_slave/ \
	sen_sc2336/ \
	sen_sc2336_slave/ \
	sen_sc233a/ \
	sen_sc3332/ \
	sen_sc3335/ \
	sen_sc3336/ \
	sen_sc3338/ \
	sen_sc3338_slave/ \
	sen_sc4210/ \
	sen_sc4238/ \
	sen_sc4335/ \
	sen_sc4336/ \
	sen_sc5239/ \
	sen_sc5239_slave/ \
	sen_sc8238/ \
	sen_sc910gs/ \
	sen_vd55g0/ \

endif

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)

modules: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@  modules $(NVT_MULTI_CORES_FLAG)

modules_install:
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n modules_install; \
	done

clean:
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n clean; \
	done
	@rm output -rf
endif
