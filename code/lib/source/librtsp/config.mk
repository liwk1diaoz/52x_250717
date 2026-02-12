#only select one platform 
PLATFORM_ARM_LINUX_3_3_UCLIB=y
PLATFORM_ARM_LINUX_3_3_GLIBC=n
PLATFORM_ARM_LINUX_2_6_28_GLIBC=n
PLATFORM_ARM_LINUX_2_6_14_GLIBC=n
PLATFORM_X86=n


ifeq ($(PLATFORM_ARM_LINUX_3_3_UCLIB),y)
USR_SRC=/usr/src/arm-linux-3.3
LINUX_SRC=$(USR_SRC)/linux-3.3-fa
export LINUX_SRC
endif

ifeq ($(PLATFORM_ARM_LINUX_3_3_GLIBC),y)
USR_SRC=/usr/src/arm-linux-3.3
LINUX_SRC=$(USR_SRC)/linux-3.3-fa
export LINUX_SRC
endif

ifeq ($(PLATFORM_ARM_LINUX_2_6_28_GLIBC),y)
KERN_26=y
KERN_SUB_28=y
USR_SRC=/usr/src/arm-linux-2.6.28
LINUX_SRC=$(USR_SRC)/linux-2.6.28-fa
include $(LINUX_SRC)/.config
export KERN_26 LINUX_SRC
endif

ifeq ($(PLATFORM_ARM_LINUX_2_6_14_GLIBC),y)
KERN_26=y
KERN_SUB_14=y
USR_SRC=/usr/src/arm-linux-2.6
LINUX_SRC=$(USR_SRC)/linux-2.6.14-fa
export KERN_26 LINUX_SRC
endif

ifeq ($(PLATFORM_X86),n)
include $(LINUX_SRC)/.config
endif
#OUTPUT_DIR=../simple_ipc_1.1/lib

