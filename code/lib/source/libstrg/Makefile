MODULE_NAME = libstrg
# DIRs
OUTPUT_DIR = $(LIBRARY_DIR)/output
# INCs
LIB_INC_PATH = $(LIBRARY_DIR)/include/libfdt
# INC FLAGs
EXTRA_INCLUDE += -I$(LIB_INC_PATH)
.PHONY: all clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
WARNING	= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Wno-sign-compare
COMPILE_OPTS	= -I. -O2 -fPIC -ffunction-sections -fdata-sections
C_CFLAGS	= \
	$(PLATFORM_CFLAGS) \
	$(PRJCFG_CFLAGS) \
	-D_NETWORK_ON_CPU1_ \
	-I$(LIBRARY_DIR)/include \
	-I$(NVT_VOS_DIR)/include \
	-I$(LIBRARY_DIR)/include/libfdt \
	-D_LARGEFILE_SOURCE \
	-D_FILE_OFFSET_BITS=64 \

LD_FLAGS	= -fPIC -shared -lpthread
#--------- END OF ENVIRONMENT SETTING -------------
SRC = \
	strg.c \
	nand_global.c \
	nand_firmware1.c \
	nand_firmware2.c \
	nand_firmware3.c \
	nand_firmware4.c \
	nand_firmware5.c \
	nand_firmware6.c \
	nand_firmware7.c \
	nand_firmware8.c \
	sdio_global.c \
	sdio_fat1.c \
	emmc_global.c \
	emmc_fat1.c \

OBJ = $(SRC:.c=.o)

ifeq ("$(wildcard *.c */*.c)","")
all:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
else
all: $(OBJ)
	@echo Creating library $(MODULE_NAME) ...
#	@$(CC) -o $(MODULE_NAME).so $(OBJ) $(LD_FLAGS)
	@$(AR) rcs $(MODULE_NAME).a $(OBJ)

%.o:%.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) $< -c -o $@

clean:
	@rm -f $(OBJ) *.o *.a *.so*
	@rm -rf $(OUTPUT_DIR)
endif

install:
	@mkdir -p $(OUTPUT_DIR)
	@cp -avf *.a $(OUTPUT_DIR)

###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
#--------- ENVIRONMENT SETTING --------------------
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE)
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = $(MODULE_NAME).a
SRC = \

OBJ = $(SRC:.c=.o)

all: $(LIB_NAME)

$(LIB_NAME): $(OBJ)
	@echo Creating $@...
	@$(AR) rcsD $@ $(OBJ)
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --arc-sha1 $@

%.o: %.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

clean:
	@rm -f $(LIB_NAME) $(OBJ) $(LIB_NAME).sym *.o *.a *.so*
	@rm -rf $(OUTPUT_DIR)

install:
	@mkdir -p $(OUTPUT_DIR)
	@cp -avf $(LIB_NAME) $(OUTPUT_DIR)
endif
