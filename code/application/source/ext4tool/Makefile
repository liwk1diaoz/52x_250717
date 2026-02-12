include $(NVT_PRJCFG_MODEL_CFG)
#--------- ENVIRONMENT SETTING --------------------
INCLUDES	= -I$(LIBRARY_DIR)/include
WARNING		= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes
COMPILE_OPTS	= $(INCLUDES) -I. -fPIC -ffunction-sections -fdata-sections -g
CPPFLAGS	=
CFLAGS		= $(PLATFORM_CFLAGS) $(PRJCFG_CFLAGS)
C_FLAGS		= $(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS) $(WARNING)
LD_FLAGS	= -L$(LIBRARY_DIR)/output -lnvtinfo
#--------- END OF ENVIRONMENT SETTING -------------
DEP_LIBRARIES :=
#--------- Compiling -------------------
BIN = mkfs.ext4
#SRC = hello_world.c

OBJ = $(SRC:.c=.o)

.PHONY: all clean install

all:
	@echo ">>> Skip"
clean:
	@echo ">>> Skip"

install:
	@echo ">>>>>>>>>>>>>>>>>>> $@ >>>>>>>>>>>>>>>>>>>"
	@mkdir -p $(APP_DIR)/output
	@cp -avf $(BIN) $(APP_DIR)/output
	@cp -af $(BIN) $(ROOTFS_DIR)/rootfs/bin/
	@cp -af *.a $(ROOTFS_DIR)/rootfs/lib/
