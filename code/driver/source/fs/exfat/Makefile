obj-m  += exfat.o
exfat-objs := exfat_core.o exfat_super.o exfat_api.o exfat_blkdev.o exfat_cache.o \
			   exfat_data.o exfat_bitmap.o exfat_nls.o exfat_oal.o exfat_upcase.o
ccflags-y += -I$(NVT_DRIVER_DIR)/include

ifeq ($(KERNELRELEASE),)
PWD := $(shell pwd)
KERVER ?= $(NVT_LINUX_VER)
KDIR ?= $(KERNELDIR)
MDIR ?= $(KERNELDIR)/_install_modules/lib/modules/$(KERVER)/extra
MODPATH := $(shell echo $(PWD) | awk -F'linux-driver/' '{print $$NF}')
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

