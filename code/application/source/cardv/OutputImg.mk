MODULE_NAME = app-main

.PHONY: all clean install

#######################################################################################
#--------- ENVIRONMENT SETTING -------------------------------------------------------#
#######################################################################################

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

C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) $(C_PREDEFINED) $(COMPILE_OPTS) $(WARNING) -Wno-format
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
OUTPUT_NAME = $(OUTPUT_DIR)/cardv
IMG_NAME = $(OUTPUT_DIR)/$(MODULE_NAME).img
MAP_NAME = $(OUTPUT_DIR)/$(MODULE_NAME).map
SYM_NAME = $(OUTPUT_DIR)/$(MODULE_NAME).sym
DASM_NAME = $(OUTPUT_DIR)/$(MODULE_NAME).asm

# LDS
LDS_REPLACE = \
	$(LDS_REPLACE_DEFAULT) \

#--------- END OF ENVIRONMENT SETTING -------------------------------------------------

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
	
	
ifeq (${BT_STACK}, BT_STACK_BLUEZ)	

EXTLIB = ${LIBRARY_DIR}/external
PREFIX := usr
BLUEZ := bluez-5.64

EXTRA_LIB_DIR += \
	-L$(EXTLIB)/__install/${PREFIX}/lib \
	
	
EXTRA_LIB += \
	-ldbus-1 \
	-ltinfo \
	-licalvcal \
	-lical \
	-lglib-2.0 \
	-lgio-2.0 \
	-lgmodule-2.0 \
	-lgobject-2.0 \
	-lffi \
	-lmount \
	-lz \
	-lblkid \
	-luuid \
	-liconv \
	-lreadline \
	-lresolv \
	
endif


# vos
EXTRA_LIB += \
	-l:libvos.a \

# hdal, vendor
EXTRA_LIB += \
	-l:libhdal.a \
	-l:libvendor_media.a \
	-l:libvendor_isp.a \
	-l:libvendor_eis.a \
	-l:libprebuilt_isp.a \

# code/lib
EXTRA_LIB += \
	-l:libSxTimer.a \
	-l:libgxgfx.a \
	-l:libnvtuser.a \
	-l:libuicontrol.a \
	-l:libvcontrol.a \
	-l:libappcontrol.a \
	-l:libvf_gfx.a \
	-l:libutility.a \
	-l:libfdt.a \
	-l:libWiFiIpc.a \
	-l:libfiledb.a \
	-l:libDCF.a \
	-l:libFileSys.a \
	-l:libNameRule.a \
	-l:libFsLinux.a \
	-l:libio.a \
	-l:libstrg.a \
	-l:libfileout.a \
	-l:libavfile.a \
	-l:libbsmux.a \
	-l:libnvtlive555.a \
	-l:libLviewNvt.a \
	-l:libgximagefile.a \
	-l:libexif.a \
	-l:libsizeconvert.a \
	-l:libimageapp_common.a \
	-l:libimageapp_moviemulti.a \
	-l:libimageapp_usbmovie.a \
	-l:libFontConv.a \
  	-l:libimageapp_photo.a \
	-l:libmsdcnvt.a \
	-l:libumsd.a \
	-l:libusb2dev.a \
	-l:libimageapp_play.a \
	-l:libplayback.a \
	-l:libpbxfilelist_filedb.a \
	-l:libpbxfilelist_dcf.a \
	-l:libGxVideoFile.a \
	-l:libpbxfile.a \
	-l:libfilein.a \
	-l:libbsdemux.a \
	-l:libimageapp_movieplay.a \
	-l:libusockipc.a \
	-l:libusockcliipc.a \
	-l:libHfsNvt.a \
	-l:libuvac.a \
	-l:libfwsrv.a \
	-l:libsw_md.a \
	-l:libPStore.a \
	-l:libethcamsocket.a \
	-l:libethsocketcli.a \
	-l:libethsockcliipc.a \
	-l:libethsocket.a \
	-l:libethcamsmi.a \
	-l:libethsockipc.a \
	-l:libtse.a \
	-l:liblogfile.a \
	-l:libnvtqrcode.a \
	-l:libzbar.a \
	-liconv \
	-l:libnvtrtmpclient.a \
	-l:libpushrtmp.a \
	-l:libssl.a \
	-l:libcrypto.a \
	-l:libzlib.a \

# fix compiling error such as "undefined reference to `dlclose'"
glibc=$(shell echo $(CROSS_COMPILE)|grep gnueabihf)	
ifneq ($(glibc),)
EXTRA_LIB += \
	-ldl
endif

ifeq ($(PACKAGE_HEIC),1)

EXTRA_LIB += \
	-lheif_static \
	-lheif_writer_static \
	-lheifpp

endif

ifeq "$(UI_STYLE)" "UI_STYLE_LVGL"
EXTRA_LIB += \
	-llvgl
endif

#--------- END of LINKING LIBs FOR C_LDFLAGS ------------------------------------------

#######################################################################################
#--------- DEVICE TREE CONFIGURATION -------------------------------------------------#
#######################################################################################
DTB_SENSOR = $(DTS_SENSOR:.dts=.dtb)
DTB_APP = $(DTS_APP:.dts=.dtb)
DTB = $(DTB_SENSOR) $(DTB_APP)

SENSOR_CFG_DIR = $(NVT_HDAL_DIR)/ext_devices/sensor/configs
AD_CFG_DIR = $(NVT_HDAL_DIR)/ext_devices/ad/configs
MOTOR_CFG_DIR = $(NVT_HDAL_DIR)/ext_devices/motor/configs
ISP_CFG_DIR = $(NVT_HDAL_DIR)/vendor/isp/configs

DTB_INCLUDE = \
	-I$(SENSOR_CFG_DIR)/dtsi \
	-I$(AD_CFG_DIR)/dtsi \
	-I$(MOTOR_CFG_DIR)/dtsi \
	-I$(ISP_CFG_DIR)/dtsi \
	-I$(KERNELDIR)/include \
	-I$(KERNELDIR)/arch/arm/boot/dts/include \
	-I$(CONFIG_DIR)/include \
	-I$(LINUX_BUILD_TOP)/base/linux-BSP/linux-kernel/include

#--------- END OF DEVICE TREE CONFIGURATION -------------------------------------------

#######################################################################################
#--------- APP FILESYS --- (Linux Only) ----------------------------------------------#
#######################################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
APPFS_NAME = appfs
APPFS_DIR = $(OUTPUT_DIR)/$(APPFS_NAME)
SENSOR_APPFS_DIR = $(APPFS_DIR)/sensor
MOTOR_APPFS_DIR = $(APPFS_DIR)/motor
ISP_APPFS_DIR = $(APPFS_DIR)/isp

SENSOR_CFG_SRC = $(wildcard $(SENSOR_CFG_DIR)/cfg/*.cfg)
AD_CFG_SRC = $(wildcard $(AD_CFG_DIR)/cfg/*.cfg)
MOTOR_CFG_SRC = $(wildcard $(MOTOR_CFG_DIR)/cfg/*.cfg)
ISP_CFG_SRC = $(wildcard $(ISP_CFG_DIR)/cfg/*.cfg) $(wildcard $(ISP_CFG_DIR)/bin/*.bin)

OUTPUT_NAND = $(OUTPUT_DIR)/$(APPFS_NAME).cardv.ubifs.nand.bin
OUTPUT_NOR = $(OUTPUT_DIR)/$(APPFS_NAME).cardv.jffs2.nor.bin
OUTPUT_EMMC = $(OUTPUT_DIR)/$(APPFS_NAME).cardv.ext4.emmc.bin

ifeq ($(EMBMEM),EMBMEM_SPI_NAND)
OUTPUT_APPFS = $(OUTPUT_NAND)
else ifeq ($(EMBMEM),EMBMEM_SPI_NOR)
OUTPUT_APPFS = $(OUTPUT_NOR)
else ifeq ($(EMBMEM),EMBMEM_EMMC)
OUTPUT_APPFS = $(OUTPUT_EMMC)
endif

# disable python tool generate __pycache__ directory
export PYTHONDONTWRITEBYTECODE=1
# MTD configs
NVT_FS_MTD_CFG_DEFAULT := ${ROOTFS_DIR}/mtd_cfg.txt
NVT_FS_MTD_CFG := ${CONFIG_DIR}/cfg_gen/mtd_cfg.txt
-include $(NVT_FS_MTD_CFG_DEFAULT)
-include $(NVT_FS_MTD_CFG)
# UBIFS app partition label name
PARTITION_LABEL_UBIFS = app
# system fdt name
DTBNAME = nvt-na51055-evb.dtb
endif
#--------- END OF APP FILESYS ---------------------------------------------------------


#######################################################################################
#--------- COMPILING AND LINKING -----------------------------------------------------#
#######################################################################################
OBJ = $(SRC:.c=.o) $(ASM:.S=.o) $(CPP_SRC:.cpp=.o) $(DTS_SENSOR:.dts=.o)

all: $(OUTPUT_NAME) $(DTS_SENSOR:.dts=.dtb) $(OUTPUT_APPFS)

$(APPFS_DIR): $(DTS_APP:.dts=.dtb)
	@echo creating $@
	@mkdir -p $@ && \
	cp -arf -t $@ $^

$(SENSOR_APPFS_DIR): $(SENSOR_CFG_SRC)
	@echo creating $@
	@mkdir -p $@ && \
	cp -arf -t $@ $^

$(MOTOR_APPFS_DIR): $(MOTOR_CFG_SRC)
	@echo creating $@
	@mkdir -p $@ && \
	cp -arf -t $@ $^

$(OUTPUT_NAND) : $(APPFS_DIR) $(SENSOR_APPFS_DIR) $(MOTOR_APPFS_DIR)
	@echo "creating $@ ";
ifeq ($(EMBMEM), EMBMEM_SPI_NAND)
	@$(ROOTFS_DIR)/verify_leb.py -f $(dir $(NVT_PRJCFG_MODEL_CFG))$(DTBNAME) -l $(PARTITION_LABEL_UBIFS) -b $(ROOTFS_UBI_APP_MAX_LEB_COUNT)
endif
	@mkfs.ubifs --squash-uids -x $(ROOTFS_UBI_COMPRESS_MODE) -m $(ROOTFS_UBI_PAGE_SIZE) -e $(ROOTFS_UBI_ERASE_BLK_SIZE) -c $(ROOTFS_UBI_APP_MAX_LEB_COUNT) -o $(OUTPUT_NAND).raw -r $(OUTPUT_DIR)/$(APPFS_NAME);
	@echo -e "[ubifs]\nmode=ubi\nimage=$(OUTPUT_NAND).raw\nvol_id=0" > $(OUTPUT_DIR)/ubinize.cfg;
	@echo -e "vol_size=$(shell echo $(ROOTFS_UBI_APP_MAX_LEB_COUNT)\*$(shell echo $(ROOTFS_UBI_ERASE_BLK_SIZE))|bc)" >> $(OUTPUT_DIR)/ubinize.cfg;
	@echo -e "vol_type=dynamic\nvol_name=app" >> $(OUTPUT_DIR)/ubinize.cfg;
	@ubinize -m $(ROOTFS_UBI_PAGE_SIZE) -p $(ROOTFS_UBI_BLK_SIZE) -s $(ROOTFS_UBI_PAGE_SIZE) -o $(OUTPUT_NAND).tmp.raw $(OUTPUT_DIR)/ubinize.cfg;
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=$(OUTPUT_NAND).tmp.raw --packsum-dst=$(OUTPUT_NAND) --packsum-type=0x9;

$(OUTPUT_NOR) : $(APPFS_DIR) $(SENSOR_APPFS_DIR) $(MOTOR_APPFS_DIR)
	@echo "creating $@ ";
	@mkfs.jffs2 -n -s $(ROOTFS_JFFS2_PAGE_SIZE) -e $(ROOTFS_JFFS2_BLK_SIZE) -r $(OUTPUT_DIR)/$(APPFS_NAME) -U -X $(ROOTFS_JFFS2_COMPRESS_MODE) -p $(ROOTFS_JFFS2_APP_NOR_SIZE) -o $(OUTPUT_NOR).raw;
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=$(OUTPUT_NOR).raw --packsum-dst=$(OUTPUT_NOR) --packsum-type=0x9;

$(OUTPUT_EMMC) : $(APPFS_DIR) $(SENSOR_APPFS_DIR) $(MOTOR_APPFS_DIR)
	@echo "creating $@ ";
	@make_ext4fs -s -l $(ROOTFS_EXT4_APP_SIZE) $@.raw $(OUTPUT_DIR)/$(APPFS_NAME);
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=$@.raw --packsum-dst=$@ --packsum-type=0x9; \
	mv $@.raw $(LINUX_BUILD_TOP)/output/raw;

$(LDS_EXTERN): $(OBJ) $(LIB_DEPENDENCY) $(LDSCRIPT) $(MAIN_C)
	@echo generate $@ ... && \
	echo "EXTERN(" > $@  && \
	$(OBJDUMP) -h $(LIB_DEPENDENCY) | grep "\.version\.info" | sed 's/[^.]*\.version\.info\.[^\.]*\.\([^\ ]*\).*/\1_LIBRARY_VERSION_INFO/g' >> $@
	@$(OBJDUMP) -h $(LIB_DEPENDENCY) | grep "\.cmdsys\.table" | sed 's/[^.]*\.cmdsys\.table\.\([^\ ]*\).*/\1_cmdsys_main/g' >> $@
	@$(OBJDUMP) -h $(LIB_DEPENDENCY) | grep "\.examsys\.table" | sed 's/[^.]*\.examsys\.table\.\([^\ ]*\).*/\1_examsys_main/g' >> $@
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
# inline warning should be ignored with some LVGL static inline functions(e.g. lv_color_mix_with_alpha)
%/fbdev.o: %/fbdev.c
	@echo Compiling $< , skip inline warning
	@$(CC) $(C_CFLAGS) -Wno-inline -c $< -o $@
	
%/shell.o: %/shell.c
	@echo Compiling $< , skip strict prototypes
	@$(CC) $(C_CFLAGS) -DVERSION=5.64 -Wno-strict-prototypes -c $< -o $@

%.o: %.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

%.o: %.cpp
	@echo Compiling $<
	@$(CXX) $(C_CXXFLAGS) -c $< -o $@

%.o: %.S
	@echo Assembling $<
	@$(CC) $(C_AFLAGS) -c $< -o $@

%.dtb: %.dts
	@echo Compiling $<
	@cpp -nostdinc $(DTB_INCLUDE) -undef -x assembler-with-cpp $(C_PREDEFINED) $< > $<.tmp \
	&& dtc -O dtb -b 0 -o $@ $<.tmp \
	&& rm $<.tmp

%.o: %.dtb
	@echo Objectizing $< && \
	touch $<.tmp.c && \
	$(CC) $(C_CFLAGS) -c $<.tmp.c -o $@ && \
	$(OBJCOPY) --add-section .sensor=$< $@ && \
	rm $<.tmp.c

# cim is for coverity check
cim: $(DTB) $(OBJ)

clean:
	@rm -rf $(OBJ) $(MAIN_C:.c=.o) $(LDS_EXTERN) $(DTB) $(OUTPUT_DIR)

install: $(OUTPUT_NAME) $(OUTPUT_APPFS)
	@mkdir -p $(INSTALL_DIR)
	@cp -avf $(OUTPUT_NAME) $(INSTALL_DIR)
	@cp -avf $(OUTPUT_NAME) $(ROOTFS_DIR)/rootfs/usr/bin
	@cp -avf $(OUTPUT_APPFS) $(LINUX_BUILD_TOP)/output

dasm: $(IMG_NAME)
	@echo Disassembly $< to $(DASM_NAME)... \
	&& $(OBJDUMP) -D $(IMG_NAME) > $(DASM_NAME)

# codesize statistics
codesize: $(OUTPUT_NAME)
	@cd $(dir $(SYM_NAME)) && \
	$(BUILD_DIR)/nvt-tools/nvt-ld-op -r -s $(notdir $(SYM_NAME)) && \
	$(BUILD_DIR)/nvt-tools/nvt-ld-op -a $(notdir $(MAP_NAME)) && \
	echo see $(dir $(SYM_NAME))*.txt for the report
