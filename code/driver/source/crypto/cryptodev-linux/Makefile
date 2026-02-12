
.PHONY: modules modules_install clean

###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#
# Since version 1.6 the asynchronous mode has been
# disabled by default. To re-enable it uncomment the
# corresponding CFLAG.
#
CRYPTODEV_CFLAGS ?= #-DENABLE_ASYNC
KBUILD_CFLAGS += -I$(src) $(CRYPTODEV_CFLAGS)
#KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
KERNEL_DIR ?= $(KERNELDIR)
VERSION = 1.10.1

prefix ?= /usr/local
includedir = $(prefix)/include

cryptodev-objs = ioctl.o main.o cryptlib.o authenc.o zc.o util.o

obj-m += cryptodev.o

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
		install -m644 -b -D $(MODNAME) $(NVT_MOD_INSTALL)/lib/modules/$(KERVER)/hdal/$(MODPATH)/$(MODNAME); \
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

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)

modules:
	@echo "nothing to be done for cryptodev"

clean:
	@echo "nothing to be done for cryptodev"

modules_install:
	@echo "nothing to be done for cryptodev"

endif
