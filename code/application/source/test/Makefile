MODULE_NAME = app-main

include $(NVT_PRJCFG_MODEL_CFG)
.PHONY: all clean install

#######################################################################################
#--------- PACKAGE SELECTION ---------------------------------------------------------#
#######################################################################################
# 0: disable, 1: enable
PACKAGE_SAMPLES  = 1
PACKAGE_CMDSYS   = 1

#######################################################################################
#--------- DO NOT EDIT ---------------------------------------------------------------#
#######################################################################################
# rtos-main.c always be compilied at last to update build date
MAIN_C = ./app-main.c
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

#--------- END OF DO NOT EDIT ---------------------------------------------------------

#######################################################################################
#--------- ENVIRONMENT SETTING -------------------------------------------------------#
#######################################################################################
# DIRs
SYSROOT_DIR     = $(shell $(CC) $(PLATFORM_CFLAGS) -print-sysroot)
GCC_LIB_DIR     = $(dir $(shell $(CC) $(PLATFORM_CFLAGS) -print-libgcc-file-name))
SYSROOT_LIB_DIR = $(SYSROOT_DIR)/usr/lib
STDC_LIB_DIR    = $(dir $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=libstdc++.a))
LIBC_LIB_DIR    = $(dir $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=libc.a))
HDAL_SAMPLE_DIR = $(NVT_HDAL_DIR)/samples
NVT_TOOLS_DIR = $(BUILD_DIR)/nvt-tools
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
EXT_DIR = $(NVT_HDAL_DIR)/ext_devices
KFLOW_DIR = $(NVT_HDAL_DIR)/drivers/k_flow
OUTPUT_DIR = ./output
INSTALL_DIR = ../../output

# LIB_DEPENDENCY to collect magic symbol and check if needing link again and
LIB_DEPENDENCY = \
	$(wildcard $(NVT_HDAL_DIR)/output/*.a) \
	$(wildcard $(NVT_HDAL_DIR)/vendor/output/*.a) \
	$(wildcard $(NVT_HDAL_DIR)/vendor/isp/drivers/output/*.a) \
	$(wildcard $(NVT_HDAL_DIR)/vendor/media/drivers/output/*.a) \
	$(wildcard $(HDAL_SAMPLE_DIR)/output/*.a) \
	$(wildcard $(LIBRARY_DIR)/output/*.a) \
	$(wildcard $(VOS_DRIVER_DIR)/output/*.a) \
	$(wildcard $(NVT_VOS_DIR)/output/*.a) \
	$(wildcard $(NVT_DRIVER_DIR)/output/*.a) \
	$(wildcard $(KDRV_DIR)/output/*.a) \
	$(wildcard $(EXT_DIR)/panel/output/*.a) \
	$(wildcard $(EXT_DIR)/sensor/output/*.a) \
	$(wildcard $(KFLOW_DIR)/output/*.a) \
	$(wildcard $(EXT_DIR)/audio/output/*.a) \

#LIB DIRs for C_LDFLAGS
EXTRA_LIB_DIR += \
	-L$(LIBC_LIB_DIR) \
	-L$(GCC_LIB_DIR) \
	-L$(STDC_LIB_DIR) \
	-L$(SYSROOT_LIB_DIR) \
	-L$(NVT_HDAL_DIR)/output \
	-L$(NVT_HDAL_DIR)/vendor/output \
	-L$(NVT_HDAL_DIR)/vendor/media/drivers/output/ \
	-L$(NVT_HDAL_DIR)/vendor/isp/drivers/output/ \
	-L$(KDRV_DIR)/output \
	-L$(EXT_DIR)/panel/output \
	-L$(EXT_DIR)/sensor/output \
	-L$(EXT_DIR)/audio/output \
	-L$(KFLOW_DIR)/output \
	-L$(HDAL_SAMPLE_DIR)/output \
	-L$(LIBRARY_DIR)/output \
	-L$(VOS_DRIVER_DIR)/output \
	-L$(NVT_VOS_DIR)/output \
	-L$(NVT_DRIVER_DIR)/output \
	-L$(GCC_LIB_DIR) \
	-L$(STDC_LIB_DIR) \

# FLAGs for Compiler, Assembler
C_PREDEFINED = \
	-D_NVT_CONSOLE_ \
	-D_MODEL_$(MODEL)_ \
	-D_BIN_NAME_='"$(BIN_NAME)"' \
	-D_BIN_NAME_T_='"$(BIN_NAME_T)"' \
	-D_BOARD_FDT_ADDR_=$(BOARD_FDT_ADDR) \
	-D_BOARD_SHMEM_ADDR_=$(BOARD_SHMEM_ADDR) \
	-D_BOARD_RTOS_ADDR_=$(BOARD_RTOS_ADDR) \
	-D_$(EMBMEM)_ \
	-D_EMBMEM_BLK_SIZE_=$(EMBMEM_BLK_SIZE) \
	-D_$(FW_TYPE)_ \
	-D_$(LCD1)_ \
	-D_$(SENSOR1)_ \
	-D_$(SENSOR2)_ \
	-D_SEN1_="$(subst sen_,,$(SENSOR1))" \
	-D_SEN2_="$(subst sen_,,$(SENSOR2))" \
	-D_$(NVT_ROOTFS_TYPE)_ \

C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) $(C_PREDEFINED) $(COMPILE_OPTS) $(WARNING) -D__LINUX
C_CXXFLAGS = $(PLATFORM_CXXFLAGS) $(EXTRA_INCLUDE) $(C_PREDEFINED) $(COMPILE_OPTS) $(WARNING)
C_AFLAGS = $(PLATFORM_AFLAGS) $(EXTRA_INCLUDE)

# CRT_OBJ_BEGIN must come first in link queue, and CRT_OBJ_END must in the last place
C_LDFLAGS = \
	--sysroot=$(SYSROOT_DIR) \
	--eh-frame-hdr \
	-dynamic-linker $(DYNAMIC_LINKER) \
	-X \
	-m armelf_linux_eabi \
	-T $(OUTPUT_DIR)/$(LDSCRIPT) \
	-Map $(MAP_NAME) \
	--start-group \
	$(CRT_OBJ_BEGIN) \
	$(EXTRA_LIB_DIR) \
	$(EXTRA_LIB) \
	$(CRT_OBJ_END) \
	--end-group

# FILEs
LDSCRIPT = $(MODULE_NAME).lds
LDS_EXTERN = extern.lds
OUTPUT_NAME = $(OUTPUT_DIR)/app-test
IMG_NAME = $(OUTPUT_DIR)/$(MODULE_NAME).img
MAP_NAME = $(OUTPUT_DIR)/$(MODULE_NAME).map
SYM_NAME = $(OUTPUT_DIR)/$(MODULE_NAME).sym
DASM_NAME = $(OUTPUT_DIR)/$(MODULE_NAME).asm

# LDS
LDS_REPLACE = \
	$(LDS_REPLACE_DEFAULT) \

#--------- END OF ENVIRONMENT SETTING -------------------------------------------------

#######################################################################################
#--------- INCs FOR C_CFLAGS ---------------------------------------------------------#
#######################################################################################
# public includes
EXTRA_INCLUDE += \
	-I$(NVT_VOS_DIR)/include \
	-I$(NVT_HDAL_DIR)/include \
	-I$(NVT_HDAL_DIR)/vendor/isp/include \
	-I$(NVT_HDAL_DIR)/vendor/isp/drivers/include \
	-I$(LIBRARY_DIR)/include \
	-I$(KDRV_DIR)/include/plat \
	-I$(KDRV_DIR)/include/comm \
	-I$(KDRV_DIR)/include \
	-I$(EXT_DIR)/panel/display_panel/include \
	-I$(EXT_DIR)/sensor/sen_common \
	-I$(EXT_DIR)/audio/aud_common/include \
	-I$(KFLOW_DIR)/include \
	-I$(NVT_DRIVER_DIR)/include \
	-I$(APP_DIR)/include \
	-I$(KDRV_DIR)/source/kdrv_gfx2d/kdrv_affine/include \
	-I$(NVT_HDAL_DIR)/vendor/media/include \

# application local includes
EXTRA_INCLUDE += \
	-I. \
	-I./system \
	-I./configs \
	-I./flow \

#--------- END OF INCs FOR C_CFLAGS ---------------------------------------------------

#######################################################################################
#--------- LINKING LIBs FOR C_LDFLAGS ------------------------------------------------#
#######################################################################################

# kernel, vos, libc, others necessary
EXTRA_LIB = \
	-lgcc \
	--as-needed \
	-lgcc_s \
	--no-as-needed \
	-lc -lgcc \
	--as-needed \
	-lgcc_s \
	--no-as-needed \
	-lrt \
	-lm \
	-lstdc++ \
	-lpthread \

# vos
EXTRA_LIB += \
	-l:libvos.a \

# hdal, vendor
EXTRA_LIB += \
	-l:libhdal.a \
	-l:libvendor_media.a \

# code/lib
EXTRA_LIB += \
	-l:libfdt.a \
	-l:libutility.a \
	-l:libSxTimer.a \
	-l:libHfsNvt.a \
	-l:libLviewNvt.a \
	-l:libfwsrv.a \
	-l:libzlib.a \
	-l:libstrg.a \
	-l:libFileSys.a \
	-l:libFsLinux.a \
	-l:libfiledb.a \
	-l:libfileout.a \
	-l:libavfile.a \
	-l:libbsmux.a \
	-l:libmsdcnvt.a \
	-l:libumsd.a \
	-l:libusb2dev.a \
	-l:libuvac.a \
	-l:libtse.a \


#--------- END of LINKING LIBs FOR C_LDFLAGS ------------------------------------------

#######################################################################################
#--------- SOURCEs FOR APPLICATION ---------------------------------------------------#
#######################################################################################
ASM = \
	./startup/code_info.S \

# system
SRC = \
	./startup/bin_info.c \
	./startup/startup.c \
	./system/sys_filesys.c \
	./system/sys_card.c \
	./system/sys_nvtmpp.c \
	./system/sys_mempool.c \
	./system/sys_fwload.c \

CPP_SRC = \

# test item
SRC += \
	./items/test_template.c \
	./items/test_hfs.c \
	./items/test_lviewd.c \
	./items/test_timer.c \
	./items/test_fwsrv.c \
	./items/test_filedb.c \
	./items/test_fileout.c \
	./items/msdcnvt/test_msdcnvt.c \
	./items/msdcnvt/MsdcNvtCb_CustomSi.c \
	./items/msdcnvt/MsdcNvtCb_Adj.c \
	./items/msdcnvt/MsdcNvtCb_UpdFw.c \
	./items/msdcnvt/MsdcNvtCbCom.c \
	./items/msdcnvt/MsdcNvtCb.c \
	./items/uvac/test_uvac.c \

#--------- END OF SOURCEs FOR APPLICATION ---------------------------------------------

#######################################################################################
#--------- COMPILING AND LINKING -----------------------------------------------------#
#######################################################################################
OBJ = $(SRC:.c=.o) $(ASM:.S=.o) $(CPP_SRC:.cpp=.o)

all: $(OUTPUT_NAME)

$(LDS_EXTERN): $(OBJ) $(LIB_DEPENDENCY) $(LDSCRIPT) $(MAIN_C)
	@echo generate $@ ... && \
	echo "EXTERN(" > $@  && \
	$(OBJDUMP) -h $(LIB_DEPENDENCY) | grep "\.version\.info" | sed 's/[^.]*\.version\.info\.[^\.]*\.\([^\ ]*\).*/\1_LIBRARY_VERSION_INFO/g' >> $@
ifeq ($(PACKAGE_CMDSYS),1)
	@$(OBJDUMP) -h $(LIB_DEPENDENCY) | grep "\.cmdsys\.table" | sed 's/[^.]*\.cmdsys\.table\.\([^\ ]*\).*/\1_cmdsys_main/g' >> $@
endif
ifeq ($(PACKAGE_SAMPLES),1)
	@$(OBJDUMP) -h $(LIB_DEPENDENCY) | grep "\.examsys\.table" | sed 's/[^.]*\.examsys\.table\.\([^\ ]*\).*/\1_examsys_main/g' >> $@
endif
	@echo ")" >> $@

$(IMG_NAME): $(LDS_EXTERN)
	@echo Compiling $(MAIN_C) && \
	$(CC) $(C_CFLAGS) -c $(MAIN_C) -o $(MAIN_C:.c=.o) && \
	echo Creating $@... && \
	mkdir -p $(OUTPUT_DIR) && \
	sed $(LDS_REPLACE_DEFAULT) $(LDSCRIPT) > $(OUTPUT_DIR)/$(LDSCRIPT).in && \
	cpp -nostdinc -x assembler-with-cpp -C -P -E $(C_PREDEFINED) $(OUTPUT_DIR)/$(LDSCRIPT).in > $(OUTPUT_DIR)/$(LDSCRIPT) && \
	rm $(OUTPUT_DIR)/$(LDSCRIPT).in && \
	$(COLLECT2) -o $@ $(OBJ) $(MAIN_C:.c=.o) $(C_LDFLAGS) && \
	$(NM) -n $@ > $(SYM_NAME)

$(OUTPUT_NAME): $(IMG_NAME)
	@echo Creating executable $@ ... && \
	$(STRIP) $< && \
	$(OBJCOPY) -R .comment -R .note.ABI-tag -R .gnu.version $< $@

%.o: %.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

%.o: %.cpp
	@echo Compiling $<
	@$(CXX) $(C_CXXFLAGS) -c $< -o $@

%.o: %.S
	@echo Assembling $<
	@$(CC) $(C_AFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ) $(MAIN_C:.c=.o) $(LDS_EXTERN) $(OUTPUT_DIR)

install: $(OUTPUT_NAME)
	@mkdir -p $(INSTALL_DIR)
	@cp -avf $(OUTPUT_NAME) $(INSTALL_DIR)
	@cp -avf $(OUTPUT_NAME) $(ROOTFS_DIR)/rootfs/usr/bin

dasm: $(IMG_NAME)
	@echo Disassembly $< to $(DASM_NAME)... \
	&& $(OBJDUMP) -D $(IMG_NAME) > $(DASM_NAME)
