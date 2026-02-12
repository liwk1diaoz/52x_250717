MODULE_NAME = hd_cmdsys

.PHONY: all clean install

###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#######################################################################################
#--------- DO NOT EDIT ---------------------------------------------------------------#
#######################################################################################

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

C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) $(C_PREDEFINED) $(COMPILE_OPTS) $(WARNING)
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
LDSCRIPT = main.lds
LDS_EXTERN = extern.lds
OUTPUT_NAME = $(OUTPUT_DIR)/$(MODULE_NAME)
IMG_NAME = $(OUTPUT_DIR)/$(MODULE_NAME).img
MAP_NAME = $(OUTPUT_DIR)/$(MODULE_NAME).map
SYM_NAME = $(OUTPUT_DIR)/$(MODULE_NAME).sym
DASM_NAME = $(OUTPUT_DIR)/$(MODULE_NAME).asm

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

#--------- END of LINKING LIBs FOR C_LDFLAGS ------------------------------------------

#######################################################################################
#--------- SOURCEs FOR APPLICATION ---------------------------------------------------#
#######################################################################################
# system
SRC = \
	./main.c \

#--------- END OF SOURCEs FOR APPLICATION ---------------------------------------------

#######################################################################################
#--------- COMPILING AND LINKING -----------------------------------------------------#
#######################################################################################
OBJ = $(SRC:.c=.o) $(ASM:.S=.o) $(CPP_SRC:.cpp=.o)

all: $(OUTPUT_NAME)

$(LDS_EXTERN): $(OBJ) $(LIB_DEPENDENCY) $(LDSCRIPT)
	@echo generate $@ ... && \
	echo "EXTERN(" > $@  && \
	$(OBJDUMP) -h $(LIB_DEPENDENCY) | grep "\.version\.info" | sed 's/[^.]*\.version\.info\.[^\.]*\.\([^\ ]*\).*/\1_LIBRARY_VERSION_INFO/g' >> $@
	@$(OBJDUMP) -h $(LIB_DEPENDENCY) | grep "\.cmdsys\.table" | sed 's/[^.]*\.cmdsys\.table\.\([^\ ]*\).*/\1_cmdsys_main/g' >> $@
	@$(OBJDUMP) -h $(LIB_DEPENDENCY) | grep "\.examsys\.table" | sed 's/[^.]*\.examsys\.table\.\([^\ ]*\).*/\1_examsys_main/g' >> $@
	@echo ")" >> $@

$(IMG_NAME): $(LDS_EXTERN)
	@echo Creating $@... && \
	mkdir -p $(OUTPUT_DIR) && \
	cp $(LDSCRIPT) $(OUTPUT_DIR)/$(LDSCRIPT).in && \
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

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
#--------- ENVIRONMENT SETTING --------------------
all:
	 @echo "nothing to be done for $(MODULE_NAME)"

install:
	 @echo "nothing to be done for $(MODULE_NAME)"

clean:
	 @echo "nothing to be done for $(MODULE_NAME)"

endif
