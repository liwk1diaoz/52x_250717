MODULE_NAME = fake
KDRV_INC_PATH=$(NVT_HDAL_DIR)/drivers/k_driver
KFLOW_INC_PATH=$(NVT_HDAL_DIR)/drivers/k_flow
C_CFLAGS  += -D__LINUX
EXTRA_INCLUDE += -I$(KDRV_INC_PATH)/include/ -I$(KFLOW_INC_PATH)/include/
EXTRA_CFLAGS += $(C_CFLAGS) $(EXTRA_INCLUDE) -Wno-date-time -D__SOC_680_PLATFORM__ -I$(src)/include -DDEBUG
KBUILD_EXTRA_SYMBOLS = $(shell find $(NVT_HDAL_DIR)/drivers -name Module.symvers)

obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-objs := \
	fake_main.o

ifeq ($(KERNELRELEASE),)
PWD := $(shell pwd)
KERVER ?= $(NVT_LINUX_VER)
KDIR ?= $(KERNELDIR)
MDIR ?= $(KERNELDIR)/_install_modules/lib/modules/$(KERVER)/extra
MODPATH := $(shell echo $(PWD) | awk -F'source/' '{print $$NF}')
MODNAME := $(shell echo $(obj-m:.o=.ko))

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

modules_install:
	@if [ -z $(NVT_MOD_INSTALL) ]; then \
		rm -f $(MDIR)/$(MODPATH)/$(MODNAME); \
		install -m644 -b -D $(MODNAME) ${MDIR}/$(MODPATH)/$(MODNAME); \
		cd $(KDIR)/_install_modules/lib/modules/$(KERVER)/; depmod -b $(KDIR)/_install_modules/ -a $(KERVER); \
	else \
		mkdir -p $(NVT_MOD_INSTALL)/lib/modules/$(KERVER); \
		install -m644 -b -D $(MODNAME) $(NVT_MOD_INSTALL)/lib/modules/$(KERVER)/extra/$(MODPATH)/$(MODNAME); \
	fi

clean:
	@SRC="`find . -name  "*.c" ! -name "*.mod.c"`"; \
	if [ "$$SRC" ]; then \
		rm -rf .tmp_versions Module.symvers modules.order `find . -type f -name "*.mod.c" -o -name ".*.cmd" -o -name "*.o" -o -name "*.ko" -o -name "modules.order" -o -name *~`; \
		echo ">>> Clean"; \
	else \
		echo ">>> Skip"; \
	fi \

.PHONY: modules modules_install clean
endif
