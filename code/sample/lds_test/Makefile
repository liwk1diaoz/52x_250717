MODULE_NAME = lds_test
# INCs
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include
HDAL_INC_PATH = $(NVT_HDAL_DIR)/include
HDAL_LIB_PATH = $(NVT_HDAL_DIR)/output
# INC FLAGs
EXTRA_INCLUDE += -I$(HDAL_INC_PATH) -I$(VOS_INC_PATH)
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
WARNING		= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror -Wformat
COMPILE_OPTS	=  -I. -O2 -fPIC -ffunction-sections -fdata-sections
C_CFLAGS	= $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE)
#--------- END OF ENVIRONMENT SETTING -------------

MAP_NAME = $(MODULE_NAME).map
LIB_NAME = $(MODULE_NAME)
SRC = lds_test.c

OBJ = $(SRC:.c=.o)

.PHONY: all clean install

ifeq ("$(wildcard *.c */*.c)","")
all:
	@echo ">>> Skip"
clean:
	@echo ">>> Skip"
else

# use COLLECT2 instead of ld (https://gcc.gnu.org/onlinedocs/gccint/Collect2.html)
COLLECT2         = $(shell $(CC) $(PLATFORM_CFLAGS) -print-prog-name=collect2)
SYSROOT_PATH     = $(shell $(CC) $(PLATFORM_CFLAGS) -print-sysroot)
GCC_LIB_PATH     = $(dir $(shell $(CC) $(PLATFORM_CFLAGS) -print-libgcc-file-name))
SYSROOT_LIB_PATH = $(SYSROOT_PATH)/usr/lib
STDC_LIB_PATH    = $(dir $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=libstdc++.a))

CRTBEGIN_OBJ = $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ = $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=crtend.o)
CRTN_OBJ = $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=crtn.o)
CRT1_OBJ = $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=crt1.o)
CRTI_OBJ = $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=crti.o)
# do not adjust the order in CRT_OBJ_BEGIN and CRT_OBJ_END
CRT_OBJ_BEGIN = $(CRT1_OBJ) $(CRTI_OBJ) $(CRTBEGIN_OBJ)
CRT_OBJ_END = --no-as-needed $(CRTEND_OBJ) $(CRTN_OBJ)

all: $(LIB_NAME)

EXTRA_LIB_DIR = \
	-L $(GCC_LIB_PATH) \
	-L $(STDC_LIB_PATH) \
	-L $(SYSROOT_LIB_PATH) \
	-L $(HDAL_LIB_PATH) \

EXTRA_LIB = \
	--as-needed -lgcc_s \
	--no-as-needed \
	-lc \
	-lgcc \
	--as-needed -lgcc_s \
	-lm \
	-lstdc++ \
	-lpthread \
	-lhdal \

# CRT_OBJ_BEGIN must come first in link queue, and CRT_OBJ_END must in the last place
C_LDFLAGS = \
	--sysroot=$(SYSROOT_PATH) \
	-Map $(MAP_NAME) \
	--eh-frame-hdr \
	-dynamic-linker /lib/ld-linux-armhf.so.3 \
	-X \
	-m armelf_linux_eabi \
	--start-group \
	$(CRT_OBJ_BEGIN) \
	$(EXTRA_LIB_DIR) \
	$(EXTRA_LIB) \
	$(CRT_OBJ_END) \
	--end-group


$(LIB_NAME): $(OBJ)
	$(COLLECT2) -o $@ $(OBJ) -T lds_test.lds $(C_LDFLAGS)
	@$(NM) -n $@ > $@.sym
	@$(STRIP) $@
	@$(OBJCOPY) -R .comment -R .note.ABI-tag -R .gnu.version $@

%.o:%.c
	@$(CC) $(C_CFLAGS) -c $< -o $@

clean:
	@rm -f $(LIB_NAME) $(OBJ) $(LIB_NAME).sym $(LIB_NAME).map *.o *.a *.so*
endif

install:
	@cp -avf $(LIB_NAME) $(ROOTFS_DIR)/rootfs/usr/bin

endif
