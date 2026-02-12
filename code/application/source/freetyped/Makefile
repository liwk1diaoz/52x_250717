include $(NVT_PRJCFG_MODEL)
#--------- ENVIRONMENT SETTING --------------------
INCLUDES	= -I$(INCLUDE_DIR) -I../lib/include -I$(INCLUDE_DIR)/freetype2
WARNING		= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes
COMPILE_OPTS	= $(INCLUDES) -I. -O2 -fPIC -ffunction-sections -fdata-sections
CPPFLAGS	=
CFLAGS		= $(PLATFORM_CFLAGS) $(PRJCFG_CFLAGS)
C_FLAGS		= $(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS) $(WARNING)
#LD_FLAGS        = -L$(LIBRARY_DIR) -Wl,-rpath-link=$(LIBRARY_DIR) -lpthread -lnvtuctrl -lnvtipc -lfreetype -lm -lnvtdevinfo -lnvtuctrl -lnvtcodec
LD_FLAGS        = -L$(LIBRARY_DIR) -Wl,-rpath-link=$(LIBRARY_DIR) -lpthread -lnvtuctrl -lnvtipc -lfreetype -lm -lnvtuctrl -lnvtcodec
#--------- END OF ENVIRONMENT SETTING -------------

#--------- Compiling -------------------
BIN = freetyped
SRC = freetyped.c
HEADER = $(shell find . -name "*.h")

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
	$(NVT_HOST)-gcc -o $@ $(OBJ) $(LD_FLAGS)
	$(NVT_HOST)-nm -n $@ > $@.sym
	$(NVT_HOST)-strip $@
	$(NVT_HOST)-objcopy -R .comment -R .note.ABI-tag -R .gnu.version $@

%.o:%.c $(HEADER)
	$(NVT_HOST)-gcc $(C_FLAGS) -c $< -o $@

clean:
	rm -vf $(BIN) $(OBJ) $(BIN).sym *.o *.a *.so*
endif

install:
	@echo ">>>>>>>>>>>>>>>>>>> $@ >>>>>>>>>>>>>>>>>>>"
	@mkdir -p $(APP_DIR)/output
	@cp -avf $(BIN) $(APP_DIR)/output
	@cp -avf $(BIN) ${ROOTFS_DIR}/rootfs/bin/
