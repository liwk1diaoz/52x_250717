MODULE_NAME = ethcam_test
include $(NVT_PRJCFG_MODEL_CFG)
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
HDAL_SAMPLE_DIR = $(NVT_HDAL_DIR)/samples
RTOS_OUTPUT_DIR = $(HDAL_SAMPLE_DIR)/output
HDAL_LIB_PATH = $(NVT_HDAL_DIR)/output
# INCs
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include
HDAL_INC_PATH = $(NVT_HDAL_DIR)/include
LIBRARY_INC_PATH = $(LIBRARY_DIR)/include
AAC_LIB_PATH = 
# INC FLAGs
EXTRA_INCLUDE += -I$(HDAL_INC_PATH) -I$(VOS_INC_PATH) -I$(NVT_HDAL_DIR)/vendor/isp/include  -I$(LIBRARY_DIR)/include
.PHONY: all clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
WARNING		= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror -Xlinker -no-enum-size-warning
COMPILE_OPTS	=  -I. -O2 -fPIC -ffunction-sections -fdata-sections -D__LINUX
C_CFLAGS	= $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE)
LD_FLAGS	= -L$(LIBRARY_DIR)/output -L$(NVT_VOS_DIR)/output -lvos -lstdc++ -lutility -lnvtuser -lethcamsocket -lethsocketcli -lethsockcliipc -lethsocket -lethsockipc -L$(HDAL_LIB_PATH) -lhdal -lgcc_s
LD_FLAGS += -fno-short-enums
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = $(MODULE_NAME)
SRC = ethcam_test.c \
	  BackgroundObj.c \
	  EthCamAppNetwork.c \
	  EthCamAppSocket.c \
	  EthCamAppCmd.c \
	  EthCamCmdParser.c \
	  EthCamCmdParserID.c



OBJ = $(SRC:.c=.o)

ifeq ("$(wildcard *.c */*.c)","")
all:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
else
all: $(LIB_NAME) $(DTB)

#Because kernel .dts depend on .dtsi inclusion, they have to be preprocessed first with
#the C preprocessor (cpp). The dtc tool can convert between .dts and .dtb:
$(DTB): $(DTS)
	cpp -nostdinc -I include -undef -x assembler-with-cpp $< > $<.tmp \
	&& dtc -O dtb -b 0 -o $@ $<.tmp
	rm $<.tmp

$(LIB_NAME): $(OBJ)
	@echo Creating $@...
	@$(CC) -o $@ $(OBJ) $(LD_FLAGS)
	@$(NM) -n $@ > $@.sym
	@$(STRIP) $@
	@$(OBJCOPY) -R .comment -R .note.ABI-tag -R .gnu.version $@

%.o: %.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

clean:
	@rm -f $(LIB_NAME) $(OBJ) $(LIB_NAME).sym *.o *.a *.so* $(DTB)
endif

install:
	@cp -avf $(LIB_NAME) $(ROOTFS_DIR)/rootfs/usr/bin

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
#--------- ENVIRONMENT SETTING --------------------
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE)
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = lib$(MODULE_NAME).a
SRC = ir_rc_test.c


OBJ = $(SRC:.c=.o)

all: $(LIB_NAME) $(DTB)

#Because kernel .dts depend on .dtsi inclusion, they have to be preprocessed first with
#the C preprocessor (cpp). The dtc tool can convert between .dts and .dtb:
$(DTB): $(DTS)
	cpp -nostdinc -I include -undef -x assembler-with-cpp $< > $<.tmp \
	&& dtc -O dtb -b 0 -o $@ $<.tmp
	rm $<.tmp

$(LIB_NAME): $(OBJ)
	@echo Creating $@...
	@$(AR) rcsD $@ $(OBJ)
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --arc-sha1 $@

%.o: %.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

clean:
	@rm -f $(LIB_NAME) $(OBJ) $(LIB_NAME).sym *.o *.a *.so* $(DTB)

install:
	@mkdir -p $(RTOS_OUTPUT_DIR)
	@cp -avf $(LIB_NAME) $(RTOS_OUTPUT_DIR)

endif
