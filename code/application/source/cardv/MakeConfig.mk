# get model name from $(NVT_PRJCFG_MODEL_CFG)
# e.g: ./configs/rtos/cfg_CARDV_EVB/ModelConfig.mk -> CARDV_EVB
MODEL = $(shell echo $(NVT_PRJCFG_MODEL_CFG) | sed 's/.*\/configs\/Linux\/cfg_\([^\/]*\)\/ModelConfig.mk/\1/g')
#######################################################################################
#--------- PACKAGE SELECTION ---------------------------------------------------------#
#######################################################################################

PACKAGE_HEIC     ?= 0

#######################################################################################
#--------- DO NOT EDIT ---------------------------------------------------------------#
#######################################################################################
# rtos-main.c always be compilied at last to update build date
MAIN_C = ./SrcCode/System/app-main.c

# compiler options
WARNING	= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror -Wformat
COMPILE_OPTS =  -I. -O2 -fPIC -ffunction-sections -fdata-sections

# use COLLECT2 instead of ld (https://gcc.gnu.org/onlinedocs/gccint/Collect2.html)
COLLECT2         = $(shell $(CC) $(PLATFORM_CFLAGS) -print-prog-name=collect2)

# necessary objects for user application
CRTBEGIN_OBJ = $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ = $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=crtend.o)
CRTN_OBJ = $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=crtn.o)
CRT1_OBJ = $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=crt1.o)
CRTI_OBJ = $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=crti.o)
# do not adjust the order in CRT_OBJ_BEGIN and CRT_OBJ_END
CRT_OBJ_BEGIN = $(CRT1_OBJ) $(CRTI_OBJ) $(CRTBEGIN_OBJ)
CRT_OBJ_END = --no-as-needed $(CRTEND_OBJ) $(CRTN_OBJ)
# DYNAMIC LINKER
uclibc=$(shell echo $(CROSS_COMPILE)|grep uclib)
ifneq ($(uclibc),)
    DYNAMIC_LINKER = /lib/ld-uClibc.so.0
else
    DYNAMIC_LINKER = /lib/ld-linux-armhf.so.3
endif

# replace string on lds
LDS_REPLACE_DEFAULT  = \
	-e's/$$START_UP/$(BOARD_RTOS_ADDR)/g' \
	-e's/$$CODE_INFO/$(CODE_INFO)/g' \
	-e's/$$BIN_INFO/$(BIN_INFO)/g' \
	-e's/$$LDS_EXTERN/$(LDS_EXTERN)/g' \
	-e's/$$MODEL/$(MODEL)/g' \
	-e's/$$SENSOR1/libnvt_$(SENSOR1)/g' \
	-e's/$$SENSOR2/libnvt_$(SENSOR2)/g' \
	-e's/$$SENSOR3/libnvt_$(SENSOR3)/g' \
	-e's/$$SENSOR4/libnvt_$(SENSOR4)/g' \
	-e's/$$LCD1/lib$(LCD1)/g' \
	-e's/$$LCD2/lib$(LCD2)/g' \

#--------- END OF DO NOT EDIT ---------------------------------------------------------

#######################################################################################
#--------- ENVIRONMENT SETTING -------------------------------------------------------#
#######################################################################################
# FLAGs for Compiler, Assembler
C_PREDEFINED = \
	-D_NVT_CONSOLE_ \
	-D_MODEL_$(MODEL)_ \
	-D_BIN_NAME_='"$(BIN_NAME)"' \
	-D_BIN_NAME_T_='"$(BIN_NAME_T)"' \
	-D_BOARD_DRAM_SIZE_=$(BOARD_DRAM_SIZE) \
	-D_BOARD_FDT_ADDR_=$(BOARD_FDT_ADDR) \
	-D_BOARD_SHMEM_ADDR_=$(BOARD_SHMEM_ADDR) \
	-D_BOARD_RTOS_ADDR_=$(BOARD_RTOS_ADDR) \
	-D_$(EMBMEM)_ \
	-D_EMBMEM_BLK_SIZE_=$(EMBMEM_BLK_SIZE) \
	-D_$(FW_TYPE)_ \
	-D_$(UI_STYLE)_ \
	-D_$(LCD1)_ \
	-D_$(SENSOR1)_ \
	-D_SENSOR1_$(SENSOR1)_ \
	-D_$(SENSOR2)_ \
	-D_SENSOR2_$(SENSOR2)_ \
	-D_$(SENSOR3)_ \
	-D_SENSOR3_$(SENSOR3)_ \
	-D_$(SENSOR4)_ \
	-D_SENSOR4_$(SENSOR4)_ \
	-D_SEN1_="$(subst sen_,,$(SENSOR1))" \
	-D_SEN2_="$(subst sen_,,$(SENSOR2))" \
	-D_SEN3_="$(subst sen_,,$(SENSOR3))" \
	-D_SEN4_="$(subst sen_,,$(SENSOR4))" \
	-D_$(NVT_ROOTFS_TYPE)_ \
	-D_$(TOUCH)_ \
	-D_$(NVT_ETHREARCAM)_\
	-D_$(NVT_ETHREARCAM_CAPS_COUNT)_\
	-D_$(NVT_SDIO_WIFI)_\
	-D_PACKAGE_HEIC_=$(PACKAGE_HEIC) \
