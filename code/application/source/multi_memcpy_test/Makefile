include $(NVT_PRJCFG_MODEL_CFG)
#--------- ENVIRONMENT SETTING --------------------
INCLUDES	= -I$(INCLUDE_DIR)
WARNING		= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes
COMPILE_OPTS	= $(INCLUDES) -I. -fPIC -ffunction-sections -fdata-sections -g
CPPFLAGS	= 
CFLAGS		= $(PLATFORM_CFLAGS) $(PRJCFG_CFLAGS)
C_FLAGS		= $(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS) $(WARNING)
LD_FLAGS	= -L$(LIBRARY_DIR)/output -lpthread -lnvtlibc
#--------- END OF ENVIRONMENT SETTING -------------
DEP_LIBRARIES := 
#--------- Compiling -------------------
BIN = multi_memcpy_test
SRC = test.c bas_task.c

BIN2 = multi_memcpy_test_6M
SRC2 = test_6m.c bas_task.c

OBJ = $(SRC:.c=.o)
OBJ2 = $(SRC2:.c=.o)

.PHONY: all clean install

ifeq ("$(wildcard *.c */*.c)","")
all:
	@echo ">>> Skip"
clean:
	@echo ">>> Skip"
else

all: $(BIN) $(BIN2)

$(BIN): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LD_FLAGS)
	@#$(NM) -n $@ > $@.sym
	@#$(STRIP) $@
	@#$(OBJCOPY) -R .comment -R .note.ABI-tag -R .gnu.version $@

$(BIN2): $(OBJ2)
	$(CC) -o $@ $(OBJ2) $(LD_FLAGS)

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
	@cp -avf $(BIN2) $(APP_DIR)/output
	@cp -avf ${BIN2} $(ROOTFS_DIR)/rootfs/usr/bin
