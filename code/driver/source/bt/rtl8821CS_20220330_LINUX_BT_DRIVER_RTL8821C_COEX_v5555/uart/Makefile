# IMPORTANT!! do not change module name 
MODULE_NAME = hci_uart

obj-m           += ${MODULE_NAME}.o
$(MODULE_NAME)-objs := hci_ldisc.o hci_h4.o hci_rtk_h5.o rtk_coex.o

ifeq ($(KERNELRELEASE),)

PWD := $(shell pwd)
KERVER ?= $(NVT_LINUX_VER)
KDIR ?= $(KERNELDIR)
MDIR ?= $(KERNELDIR)/_install_modules/lib/modules/$(KERVER)/extra
MODPATH := $(shell echo $(PWD) | awk -F'source/' '{print $$NF}')
MODNAME := $(shell echo $(obj-m:.o=.ko))

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

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
	else \
		rm -rf .tmp_versions Module.symvers modules.order $(MODULE_NAME).o $(MODULE_NAME).mod.o `find . -type f -name "*.mod.c" -o -name ".*.cmd" -o -name "*.ko" -o -name "modules.order" -o -name *~`; \
	fi
	
.PHONY: modules modules_install clean
endif