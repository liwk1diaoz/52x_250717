MODULE_NAME = hd_uvc_cam_ct_pu
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
HDAL_SAMPLE_DIR = $(NVT_HDAL_DIR)/samples
RTOS_OUTPUT_DIR = $(HDAL_SAMPLE_DIR)/output
# INCs
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include
HDAL_INC_PATH = $(NVT_HDAL_DIR)/include
HDAL_LIB_PATH = $(NVT_HDAL_DIR)/output
MEDIA_INC_PATH = $(NVT_HDAL_DIR)/vendor/media/include
ISP_INC_PATH = $(NVT_HDAL_DIR)/vendor/isp/include

EXTRA_LIB_DIR += \
	-L$(NVT_HDAL_DIR)/vendor/media/drivers/output/ \
	-L$(NVT_HDAL_DIR)/vendor/output \
	-L$(NVT_HDAL_DIR)/output \
        -L$(LIBRARY_DIR)/output \
	-L$(NVT_VOS_DIR)/output \


# INC FLAGs
EXTRA_INCLUDE += -I$(HDAL_INC_PATH) -I$(VOS_INC_PATH) -I$(MEDIA_INC_PATH) -I$(ISP_INC_PATH)\
	-I$(NVT_VOS_DIR)/include \
	-I$(NVT_HDAL_DIR)/include \
	-I$(LIBRARY_DIR)/include \
	-I$(KDRV_DIR)/include/comm \
	-I$(KDRV_DIR)/include \
        -I$(NVT_HDAL_DIR)/drivers/k_driver/include/comm \
        -I$(NVT_HDAL_DIR)/drivers/k_driver/include/ \

.PHONY: all clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
WARNING		= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Wno-unused-variable
COMPILE_OPTS	=  -I. -O2 -fPIC -ffunction-sections -fdata-sections
C_CFLAGS	= $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE)


# code/lib
EXTRA_LIB += \
	-l:libfdt.a \
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
	-l:libuvac.a \
	-l:libusb2dev.a \
	-l:libutility.a \

# hdal, vendor
EXTRA_LIB += \
	-l:libhdal.a \
	-l:libvendor_media.a \
	-l:libvendor_isp.a \
	
# vos
EXTRA_LIB += \
	-l:libvos.a \

LD_FLAGS	= -L$(HDAL_LIB_PATH) -L$(EXTRA_LIB_DIR) -lpthread $(EXTRA_LIB)
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = $(MODULE_NAME)
SRC = uvc_cam_ct_pu.c  uvc_ct.c uvc_pu.c

OBJ = $(SRC:.c=.o)

#ifeq ("$(wildcard *.c */*.c)","")
#all:
#	@echo "nothing to be done for '$(OUTPUT_NAME)'"
#clean:
#	@echo "nothing to be done for '$(OUTPUT_NAME)'"
#else
all: $(LIB_NAME)

$(LIB_NAME): $(OBJ)
	@echo Creating $@...
	@$(CC) -o $@ $(OBJ) $(LD_FLAGS)
	@$(NM) -n $@ > $@.sym
#	@$(STRIP) $@
	@$(OBJCOPY) -R .comment -R .note.ABI-tag -R .gnu.version $@

%.o: %.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

clean:
	@rm -f $(LIB_NAME) $(OBJ) $(LIB_NAME).sym *.o *.a *.so*
#endif

install:
	@cp -avf $(LIB_NAME) $(ROOTFS_DIR)/rootfs/usr/bin

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
#--------- ENVIRONMENT SETTING --------------------

endif
