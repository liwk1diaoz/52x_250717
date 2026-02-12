include $(NVT_PRJCFG_MODEL_CFG)
#--------- ENVIRONMENT SETTING --------------------
INCLUDES	= -I$(INCLUDE_DIR)
WARNING		= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes
COMPILE_OPTS	= $(INCLUDES) -I. -fPIC -ffunction-sections -fdata-sections -g
CPPFLAGS	= 
CFLAGS		= $(PLATFORM_CFLAGS) $(PRJCFG_CFLAGS) -I$(KERNELDIR)
C_FLAGS		= $(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS) $(WARNING)
LD_FLAGS	= -L$(LIBRARY_DIR) 
#--------- END OF ENVIRONMENT SETTING -------------
DEP_LIBRARIES := 
#--------- Compiling -------------------
BIN = nvt_perf
SRC = nvt_perf.c

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
	@mkdir -p $(APP_DIR)/output
	@cp -avf $(BIN) $(APP_DIR)/output
	@cp -avf ${BIN} $(ROOTFS_DIR)/rootfs/usr/bin
