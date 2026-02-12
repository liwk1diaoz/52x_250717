include $(NVT_PRJCFG_MODEL_CFG)
#--------- ENVIRONMENT SETTING --------------------
INCLUDES	= -I$(NVT_HDAL_DIR)/include -I$(NVT_HDAL_DIR)/vendor/media/include
WARNING		= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes
COMPILE_OPTS	= $(INCLUDES) -I. -fPIC -ffunction-sections -fdata-sections -g
CPPFLAGS	=
CFLAGS		= $(PLATFORM_CFLAGS) $(PRJCFG_CFLAGS)
C_FLAGS		= $(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS) $(WARNING)
LD_FLAGS	= -L$(LIBRARY_DIR)/output -L$(NVT_HDAL_DIR)/output -L$(NVT_HDAL_DIR)/vendor/output -lhdal -lvendor_media
#--------- END OF ENVIRONMENT SETTING -------------
DEP_LIBRARIES :=
#--------- Compiling -------------------
BIN = mem_hotplug
SRC = mem_hotplug.c

OBJ = $(SRC:.c=.o)

.PHONY: all clean install

ifeq ("$(wildcard *.c */*.c)","")
all:
	@echo ">>> Skip"
clean:
	@echo ">>> Skip"
else

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LD_FLAGS)
	@#$(NM) -n $@ > $@.sym
	@#$(STRIP) $@
	@#$(OBJCOPY) -R .comment -R .note.ABI-tag -R .gnu.version $@

%.o:%.c
	$(CC) $(C_FLAGS) -c $< -o $@

clean:
	rm -vf $(BIN) $(OBJ) $(BIN).sym *.o *.a *.so*
endif
install:
	@echo ">>>>>>>>>>>>>>>>>>> $@ >>>>>>>>>>>>>>>>>>>"
	@cp -avf $(BIN) $(ROOTFS_DIR)/rootfs/usr/bin

