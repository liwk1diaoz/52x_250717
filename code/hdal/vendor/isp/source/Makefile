OUTPUT_DIR = $(NVT_HDAL_DIR)/vendor/output
SUBDIRS := $(dir $(shell find . -maxdepth 2 -name Makefile))
SUBDIRS := $(shell echo $(SUBDIRS) | sed -e 's/\.\///g';)

.PHONY: all clean install $(SUBDIRS)

all: $(SUBDIRS)

###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
uclibc=$(shell echo $(CROSS_COMPILE)|grep uclib)
ifeq ($(uclibc),)
    ENV_LIBC=glibc
else
    ENV_LIBC=uclibc
endif
###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
ENV_LIBC=rtos
endif

$(SUBDIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS) $(NVT_MULTI_CORES_FLAG);

install: $(SUBDIRS)
ifneq ("$(wildcard prebuilt/$(ENV_LIBC)/*.a)","")
	@cp -avf prebuilt/$(ENV_LIBC)/*.* $(OUTPUT_DIR)
endif

clean: $(SUBDIRS)
	@rm output -rf
