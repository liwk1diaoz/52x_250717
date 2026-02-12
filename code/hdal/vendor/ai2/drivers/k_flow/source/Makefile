current_mk_abspathname := $(abspath $(lastword $(MAKEFILE_LIST)))
current_mk_absdir := $(dir $(current_mk_abspathname))../../../../ai_sdk_env.mk
ifneq ("$(wildcard $(current_mk_absdir))", "")
include $(current_mk_absdir)
endif

NVT_KDRV_PATH=$(NVT_HDAL_DIR)/drivers/k_driver
NVT_KFLOW_PATH=$(NVT_HDAL_DIR)/drivers/k_flow

ifeq ("$(wildcard $(OWN_NVT_AI2_PATH))", "")
NVT_AI_KDRV_PATH=$(NVT_HDAL_DIR)/vendor/ai/drivers/k_driver
NVT_AI_KFLOW_PATH=$(NVT_HDAL_DIR)/vendor/ai2/drivers/k_flow
else
NVT_AI_KFLOW_PATH=$(NVT_AI2_KFLOW_PATH)
endif

VG_MODULE_INC = $(NVT_KDRV_PATH)/source/include
VOS_MODULE_INC=$(NVT_VOS_DIR)/drivers/include

EXTRA_CFLAGS += -I$(VOS_MODULE_INC)
EXTRA_CFLAGS += -I$(NVT_HDAL_DIR)/include
EXTRA_CFLAGS += -I$(NVT_KDRV_PATH)/include -I$(NVT_KFLOW_PATH)/include -I$(VG_MODULE_INC) -I$(VG_MODULE_INC)/common -I$(VG_MODULE_INC)/util
EXTRA_CFLAGS += -I$(NVT_AI_KDRV_PATH)/include -I$(NVT_AI_KFLOW_PATH)/include -I$(NVT_KDRV_PATH)/source/kdrv_builtin/ai
EXTRA_CFLAGS += -Werror -Wno-enum-compare -Wno-date-time -D__LINUX


MODULE_NAME = kflow_ai
OBJ := \
	kflow_ai_net/kflow_ai_net_flow.o \
	kflow_ai_net/kflow_ai_net_queue.o \
	kflow_ai_net/kflow_ai_core.o \
	kflow_ai_net/kflow_ai_core_tsk.o \
	kflow_ai_net/kflow_ai_core_callback.o \
	kflow_ai_net/linux/kflow_ai_net_ioctl.o \
	kflow_ai_net/linux/kflow_ai_net_proc.o \
	kflow_ai_net/linux/kflow_ai_net_platform.o \
	kflow_ai_net/debug_util/graph_debug_core.o \
	kflow_ai_net/debug_util/graph_debug_buffer.o \
	kflow_ai_net/debug_util/graph_debug_schedule.o \
	kflow_ai_net/debug_util/graph_debug_log.o \
	kflow_cnn/kflow_ai_cnn.o \
	kflow_nue/kflow_ai_nue.o \
	kflow_nue2/kflow_ai_nue2.o \
	kflow_cpu/kflow_ai_cpu.o \
	kflow_cpu/kflow_ai_cpu_callback.o \
	kflow_cpu/kflow_cpu_platform.o \
	kflow_dsp/kflow_ai_dsp.o \
	kflow_dsp/kflow_ai_dsp_callback.o \
	kflow_dsp/kflow_dsp_platform.o \

obj-m += $(MODULE_NAME).o
obj-m := $(sort $(obj-m))

###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
# To build modules outside of the kernel tree, we run "make"
# in the kernel source tree; the Makefile these then includes this
# Makefile once again.
# This conditional selects whether we are being included from the
# kernel Makefile or not.
ifeq ($(CONFIG_FUNCTION_TRACER),y)
$(MODULE_NAME)-objs := $(OBJ)
else
$(MODULE_NAME)-objs := $(OBJ:.o=_no_tracer.o)
EXTRA_CFLAGS += $(shell for n in $(OBJ:.o=); do if [ -f $(src)/$$n.c ]; then ln -sf $$(basename $${n}.c) $(src)/$${n}_no_tracer.c; else rm -f $(src)/$${n}_no_tracer.c; fi done)
endif

ifeq ($(KERNELRELEASE),)
PWD := $(shell pwd)
KERVER ?= $(NVT_LINUX_VER)
KDIR ?= $(KERNELDIR)
MDIR ?= $(KERNELDIR)/_install_modules/lib/modules/$(KERVER)/hdal
#MODPATH := $(shell echo $(PWD) | awk -F'source/' '{print $$NF}')
MODPATH := kflow_ai
MODNAME := $(shell echo $(obj-m:.o=.ko))
# variables for clean object
RM_TRACER_OBJ = $(foreach n, $(OBJ:.o=), $(if $(wildcard $(n).c),$(if $(wildcard $(n).o),$(n).o,),))
RM_NO_TRACER_OBJ = $(foreach n, $(OBJ:.o=), $(if $(wildcard $(n).c),$(if $(wildcard $(n)_no_tracer.o),$(n)_no_tracer.o,),))
export KBUILD_EXTRA_SYMBOLS += $(shell find $(NVT_KDRV_PATH) -name Module.symvers)
export KBUILD_EXTRA_SYMBOLS += $(shell find $(NVT_AI_KDRV_PATH) -name Module.symvers)
export KBUILD_EXTRA_SYMBOLS += $(shell find $(NVT_AI_KFLOW_PATH) -name Module.symvers)

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) $(NVT_KGCOV) modules
	@$(MAKE) -C ./netflowsample -j$(JOBS)
	
modules_install:
	@rm -f $(MDIR)/$(MODPATH)/$(MODNAME); \
	install -m644 -b -D $(MODNAME) ${MDIR}/$(MODPATH)/$(MODNAME); \
	cd $(KDIR)/_install_modules/lib/modules/$(KERVER)/; depmod -b $(KDIR)/_install_modules/ -a $(KERVER)
	@if [ -z $(NVT_MOD_INSTALL) ]; then \
		rm -f $(MDIR)/$(MODPATH)/$(MODNAME); \
		install -m644 -b -D $(MODNAME) ${MDIR}/$(MODPATH)/$(MODNAME); \
		cd $(KDIR)/_install_modules/lib/modules/$(KERVER)/; depmod -b $(KDIR)/_install_modules/ -a $(KERVER); \
	else \
		mkdir -p $(NVT_MOD_INSTALL)/lib/modules/$(KERVER); \
		install -m644 -b -D $(MODNAME) $(NVT_MOD_INSTALL)/lib/modules/$(KERVER)/hdal/$(MODPATH)/$(MODNAME); \
	fi
	@$(MAKE) -C ./netflowsample modules_install
	
clean:
	@rm -rf $(MODULE_NAME).* .$(MODULE_NAME).* $(OBJ:.o=_no_tracer.c) Module.symvers modules.order .tmp_versions
	@rm -f `find . -type f -name ".*.cmd" -o -name ".*.d"`
	@rm -f $(RM_TRACER_OBJ) $(RM_NO_TRACER_OBJ)
	@$(MAKE) -C ./netflowsample clean
	@rm -rf netflowsample.ko
codesize:
	@echo $(MODULE_NAME)
	@$(OBJDUMP) -t $(MODULE_NAME).o > $(MODULE_NAME).sym && \
	$(BUILD_DIR)/nvt-tools/nvt-ld-op -j $(MODULE_NAME).sym
	@$(MAKE) -C $(NVT_KFLOW_PATH) codesize
.PHONY: modules modules_install clean

endif
###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
OUTPUT_NAME = lib$(MODULE_NAME).a
RTOS_OUTPUT_DIR = $(NVT_KDRV_PATH)/output
KDRV_DIR_OUTPUT = $(NVT_KDRV_PATH)/output
RTOS_KERNEL_DIR = $(KERNELDIR)/lib/FreeRTOS
RTOS_CURR_DEMO_DIR = $(KERNELDIR)/demos/novatek/na51055
RTOS_POSIX_DIR = $(KERNELDIR)/lib/FreeRTOS-Plus-POSIX
RTOS_POSIX_SRC_DIR = $(RTOS_POSIX_DIR)/source

EXTRA_INCLUDE += -I$(RTOS_KERNEL_DIR)/portable/GCC/ARM_CA9
EXTRA_INCLUDE += -I$(RTOS_KERNEL_DIR)/include
EXTRA_INCLUDE += -I$(RTOS_KERNEL_DIR)/include/private
EXTRA_INCLUDE += -I$(RTOS_CURR_DEMO_DIR)/include
EXTRA_INCLUDE += -I$(RTOS_POSIX_DIR)/include
EXTRA_INCLUDE += -I$(RTOS_POSIX_DIR)/include/portable/novatek
EXTRA_INCLUDE += -I$(NVT_KDRV_PATH)/include/rtos_na51055
EXTRA_INCLUDE += -I$(NVT_AI_KDRV_PATH)/include
EXTRA_INCLUDE += -I$(NVT_AI_KFLOW_PATH)/include
EXTRA_INCLUDE += -I$(VOS_MODULE_INC)
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/include
EXTRA_INCLUDE += -I$(NVT_KDRV_PATH)/include
EXTRA_INCLUDE += -I$(NVT_KFLOW_PATH)/include
EXTRA_INCLUDE += -I$(VG_MODULE_INC)
EXTRA_INCLUDE += -I$(VG_MODULE_INC)/common
EXTRA_INCLUDE += -I$(VG_MODULE_INC)/util

PLATFORM_CFLAGS += -D__FREERTOS_KFLOW

C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE)
SRC = \
	kflow_ai_net/kflow_ai_net_flow.o \
	kflow_ai_net/kflow_ai_net_queue.o \
	kflow_ai_net/kflow_ai_core.o \
	kflow_ai_net/kflow_ai_core_tsk.o \
	kflow_ai_net/kflow_ai_core_callback.o \
	kflow_ai_net/rtos/kflow_ai_net_ioctl.o \
	kflow_ai_net/rtos/kflow_ai_net_proc.o \
	kflow_ai_net/rtos/kflow_ai_net_platform.o \
	kflow_ai_net/debug_util/graph_debug_core.o \
	kflow_ai_net/debug_util/graph_debug_buffer.o \
	kflow_ai_net/debug_util/graph_debug_schedule.o \
	kflow_ai_net/debug_util/graph_debug_log.o \
	kflow_cnn/kflow_ai_cnn.o \
	kflow_nue/kflow_ai_nue.o \
	kflow_nue2/kflow_ai_nue2.o \
	kflow_cpu/kflow_ai_cpu.o \
	kflow_cpu/kflow_ai_cpu_callback.o \
	kflow_cpu/kflow_cpu_platform.o \
	kflow_dsp/kflow_ai_dsp.o \
	kflow_dsp/kflow_ai_dsp_callback.o \
	kflow_dsp/kflow_dsp_platform.o \

	OBJ = $(SRC:.c=.o)
ifeq ("$(wildcard *.c */*.c */*/*.c)","")
modules:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
else
modules : $(OUTPUT_NAME)

%.o:%.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

$(OUTPUT_NAME): $(OBJ)
	@echo Creating library $* ...
	@$(AR) rcsD $(OUTPUT_NAME) $(OBJ)
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --arc-sha1 $@

clean:
	@rm -f $(OBJ) *.o *.a *.so*
endif

modules_install:
	@mkdir -p $(RTOS_OUTPUT_DIR)
	@cp -avf *.a $(RTOS_OUTPUT_DIR)
	@cp -avf *.a $(KDRV_DIR_OUTPUT)
endif
