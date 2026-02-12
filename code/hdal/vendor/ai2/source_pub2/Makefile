current_mk_abspathname := $(abspath $(lastword $(MAKEFILE_LIST)))
current_mk_absdir := $(dir $(current_mk_abspathname))../../ai_sdk_env.mk
ifneq ("$(wildcard $(current_mk_absdir))", "")
include $(current_mk_absdir)
endif

CUR_PATH := $(shell pwd)

NVT_KDRV_PATH=$(NVT_HDAL_DIR)/drivers/k_driver
NVT_KFLOW_PATH=$(NVT_HDAL_DIR)/drivers/k_flow

ifeq ("$(wildcard $(OWN_NVT_AI2_PATH))", "")
NVT_AI_PATH=$(NVT_HDAL_DIR)/vendor/ai
NVT_AI2_PATH=$(NVT_HDAL_DIR)/vendor/ai2
NVT_AI_KDRV_PATH=$(NVT_AI_PATH)/drivers/k_driver
NVT_AI_KFLOW_PATH=$(NVT_AI2_PATH)/drivers/k_flow
else
NVT_AI_KFLOW_PATH=$(NVT_AI2_KFLOW_PATH)
endif

OUTPUT_DIR = $(NVT_HDAL_DIR)/vendor/output

INCLUDE_PATH += -I$(NVT_VOS_DIR)/drivers/include
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/include -I$(NVT_KDRV_PATH)/include -I$(NVT_KDRV_PATH)/source/include/ -I$(NVT_KFLOW_PATH)/include/
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/vendor/include -I$(NVT_AI2_PATH)/include -I$(NVT_AI2_PATH)/source_pub2/include
INCLUDE_PATH += -I$(NVT_AI2_PATH)/source/prebuilt/include/ -I$(NVT_AI2_PATH)/source/prebuilt/include/nvtnn -I$(NVT_AI_KDRV_PATH)/include -I$(NVT_AI_KFLOW_PATH)/include
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/vendor/cv/include -I$(NVT_HDAL_DIR)/vendor/isp/include
INCLUDE_PATH += -I$(NVT_HDAL_DIR)/vendor/dsp/include -I$(NVT_AI2_PATH)/source/include

uclibc=$(shell echo $(CROSS_COMPILE)|grep uclib)
nn_dli_cpu = yes

ifeq ($(nn_dli_cpu), yes)
# Path of ACL's header
INCLUDE_PATH += -I$(NVT_AI2_PATH)/source_pub2/vendor_ai_cpu/nn_dli_cpu/prebuilt/include/ComputeLibrary-21.11
endif

#COPS = $(INCLUDE_PATH) -D ARM_ASM -O3 -Wall -Werror -finline-functions -msoft-float -fPIC -Wno-aggressive-loop-optimizations
COPS = $(INCLUDE_PATH) -D ARM_ASM -O3 -Wall -Werror -finline-functions -fPIC -Wno-aggressive-loop-optimizations -mfpu=neon-fp16

ifeq ($(nn_dli_cpu), yes)
COPS += -D NN_DLI_CPU_ACL
# prebuilt ACL library
ifeq ($(uclibc),)
PREBUILD_DLI_CPU_LIB=$(NVT_AI2_PATH)/source_pub2/vendor_ai_cpu/nn_dli_cpu/prebuilt/lib/glibc
else
PREBUILD_DLI_CPU_LIB=$(NVT_AI2_PATH)/source_pub2/vendor_ai_cpu/nn_dli_cpu/prebuilt/lib/uclibc
endif # uclibc
endif

CC=$(CROSS_COMPILE)gcc
AS=$(CROSS_COMPILE)as
AR=$(CROSS_COMPILE)ar

CSRC += \
	vendor_ai_util.c \
	vendor_ai_cpu/vendor_ai_cpu.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn_eltwise.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn_concat.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn_slice.c \
	vendor_ai_cpu/vendor_ai_cpu_detout.c \
	vendor_ai_cpu/vendor_ai_cpu_nvtnn.c \
	vendor_ai_cpu/vendor_ai_cpu_postproc.c \
	vendor_ai_cpu/vendor_ai_cpu_prelu.c \
	vendor_ai_cpu/vendor_ai_cpu_priorbox.c \
	vendor_ai_cpu/vendor_ai_cpu_proposal.c \
	vendor_ai_cpu/vendor_ai_cpu_softmax.c \
	vendor_ai_cpu/vendor_ai_cpu_util.c \
	vendor_ai_dsp/vendor_ai_dsp.c \
	vendor_ai_dsp/vendor_ai_dsp_custnn.c \
	vendor_ai_dsp/vendor_ai_dsp_custnn_pool.c \
	vendor_ai_dsp/vendor_ai_dsp_nvtnn.c

CPPSRC += \
	vendor_ai_cpu/vendor_ai_cpu_custnn_pool.cpp

ifeq ($(nn_dli_cpu), yes)
CPPSRC += \
	vendor_ai_cpu/nn_dli_cpu/vendor_ai_dli_cpu.cpp \
	vendor_ai_cpu/nn_dli_cpu/vendor_ai_dli_cpu_elementwise.cpp \
	vendor_ai_cpu/nn_dli_cpu/vendor_ai_dli_cpu_resize.cpp \
	vendor_ai_cpu/nn_dli_cpu/vendor_ai_dli_cpu_softmax.cpp \
	vendor_ai_cpu/nn_dli_cpu/vendor_ai_dli_cpu_utils.cpp
endif

HDAL_STATIC_LIB = libvendor_ai2_pub2.a
HDAL_DYNAMIC_LIB = libvendor_ai2_pub2.so

COBJS = ${CSRC:%.c=%.o} ${CPPSRC:%.cpp=%.o}

TARGETS := $(HDAL_STATIC_LIB) $(HDAL_DYNAMIC_LIB)

.PHONY: $(TARGETS)
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
all: $(TARGETS)

$(HDAL_STATIC_LIB) : $(COBJS) $(SOBJS) 
	$(AR) -r $@ $(COBJS)
#	mkdir -p ./lib
#	cp $@ ./lib/

$(HDAL_DYNAMIC_LIB) : $(COBJS) $(SOBJS) 
	$(CC) -shared -s -o $@ $(COBJS)
#	mkdir -p ../lib
#	cp $@ ./lib/

%.o: %.c
	$(CC) -c $(COPS) $(CFLAGS) -I. -o $@ $<
	
%.o: %.cpp
	$(CC) -c $(COPS) $(CFLAGS) -I. -o $@ $<

install:
	cp *.so *.a $(OUTPUT_DIR)
ifeq ($(nn_dli_cpu), yes)
	cp $(PREBUILD_DLI_CPU_LIB)/*.so $(OUTPUT_DIR)
endif

clean :
	/bin/rm -f *.o
	/bin/rm -f */*.o
	/bin/rm -f */*/*.o
	/bin/rm -f $(HDAL_STATIC_LIB) $(HDAL_DYNAMIC_LIB)

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
#--------- ENVIRONMENT SETTING --------------------
# DIRs
RTOS_KERNEL_DIR = $(KERNELDIR)/lib/FreeRTOS
RTOS_LIB_DIR = $(KERNELDIR)/lib
RTOS_CURR_DEMO_DIR = $(KERNELDIR)/demos/novatek/na51055
RTOS_POSIX_DIR = $(KERNELDIR)/lib/FreeRTOS-Plus-POSIX
RTOS_POSIX_SRC_DIR = $(RTOS_POSIX_DIR)/source

#INCs for C_CFLAGS

EXTRA_INCLUDE += \
	-I$(RTOS_LIB_DIR) \
	-I$(RTOS_KERNEL_DIR)/portable/GCC/ARM_CA9  \
	-I$(RTOS_KERNEL_DIR)/include \
	-I$(RTOS_KERNEL_DIR)/include/private \
	-I$(RTOS_CURR_DEMO_DIR)/include \
	-I$(RTOS_POSIX_DIR)/include \
	-I$(RTOS_POSIX_DIR)/include/portable/novatek

C_CFLAGS = $(PLATFORM_CFLAGS) $(INCLUDE_PATH) $(EXTRA_INCLUDE) -Wno-format
C_CXXFLAGS = $(PLATFORM_CXXFLAGS) $(INCLUDE_PATH) $(EXTRA_INCLUDE) -Wno-format
#--------- END OF ENVIRONMENT SETTING -------------
MODULE_NAME = libvendor_ai2_pub2
LIB_NAME = $(MODULE_NAME).a
PREBUILD_LIB=$(NVT_HDAL_DIR)/vendor/ai/source/prebuilt/lib

SRC = \
	vendor_ai_util.c \
	vendor_ai_cpu/vendor_ai_cpu.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn_eltwise.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn_concat.c \
	vendor_ai_cpu/vendor_ai_cpu_custnn_slice.c \
	vendor_ai_cpu/vendor_ai_cpu_detout.c \
	vendor_ai_cpu/vendor_ai_cpu_nvtnn.c \
	vendor_ai_cpu/vendor_ai_cpu_postproc.c \
	vendor_ai_cpu/vendor_ai_cpu_prelu.c \
	vendor_ai_cpu/vendor_ai_cpu_priorbox.c \
	vendor_ai_cpu/vendor_ai_cpu_proposal.c \
	vendor_ai_cpu/vendor_ai_cpu_softmax.c \
	vendor_ai_cpu/vendor_ai_cpu_util.c \
	vendor_ai_dsp/vendor_ai_dsp.c \
	vendor_ai_dsp/vendor_ai_dsp_custnn.c \
	vendor_ai_dsp/vendor_ai_dsp_custnn_pool.c \
	vendor_ai_dsp/vendor_ai_dsp_nvtnn.c

PSRC += \
	vendor_ai_cpu/vendor_ai_cpu_custnn_pool.cpp
	
OBJ = $(SRC:.c=.o) $(PSRC:.cpp=.o)

all:
	@echo "nothing to be done for '$(LIB_NAME)'"

clean:
	@echo "nothing to be done for '$(LIB_NAME)'"

install:
	@echo "nothing to be done for '$(LIB_NAME)'"

endif
