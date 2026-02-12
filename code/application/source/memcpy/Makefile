include $(NVT_PRJCFG_MODEL_CFG)
#--------- ENVIRONMENT SETTING --------------------
INCLUDES	= -I$(INCLUDE_DIR)
WARNING		= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes
COMPILE_OPTS	= $(INCLUDES) -I. -fPIC -ffunction-sections -fdata-sections -g
CPPFLAGS	= 
CFLAGS		= $(PLATFORM_CFLAGS) $(PRJCFG_CFLAGS)
C_FLAGS		= $(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS) $(WARNING)
LD_FLAGS	= -L$(LIBRARY_DIR)/output -lnvtlibc
#--------- END OF ENVIRONMENT SETTING -------------
DEP_LIBRARIES := 
#--------- Compiling -------------------
BIN = memcpy
BIN2 = memcpy_multi_process
SRC = memcpy.c
SRC2 = memcpy_multi_process.c

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

$(BIN2): $(OBJ2)
	$(CC) -o $@ $(OBJ2) $(LD_FLAGS)

%.o:%.c
	$(CC) $(C_FLAGS) -c $< -o $@

clean:
	rm -vf $(BIN) $(OBJ) $(BIN).sym *.o *.a *.so* $(BIN2)
endif
install:
	@echo ">>>>>>>>>>>>>>>>>>> $@ >>>>>>>>>>>>>>>>>>>"
	@mkdir -p $(APP_DIR)/output
	@cp -avf $(BIN) $(BIN2) $(APP_DIR)/output
	@cp -avf ${BIN} $(BIN2) $(ROOTFS_DIR)/rootfs/usr/bin
