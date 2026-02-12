#--------- ENVIRONMENT SETTING --------------------
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
HDAL_SAMPLE_DIR = $(NVT_HDAL_DIR)/samples
# INCs
VOS_INC_PATH = $(VOS_DRIVER_DIR)/include
HDAL_INC_PATH = $(NVT_HDAL_DIR)/include
HDAL_LIB_PATH = $(NVT_HDAL_DIR)/output

INCLUDES = -I$(LIBRARY_DIR)/include -I$(HDAL_INC_PATH) -I$(VOS_INC_PATH) -I$(NVT_HDAL_DIR)/vendor/isp/include -I$(NVT_HDAL_DIR)/vendor/media/include
WARNING = -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes
COMPILE_OPTS = $(INCLUDES) -I. -O2 -fPIC -ffunction-sections -fdata-sections
CPPFLAGS =
CFLAGS = $(PLATFORM_CFLAGS) $(PRJCFG_CFLAGS) -D__LINUX520 -D__LINUX -D_NETWORK_ON_CPU1_
C_FLAGS = $(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS) $(WARNING)
LD_FLAGS = -L$(LIBRARY_DIR)/output -L$(NVT_HDAL_DIR)/output -lstdc++ -lpthread -lm -lrt -lnvtlive555 -lhdal -lvendor_isp -lvendor_media -L$(NVT_HDAL_DIR)/vendor/output
#--------- END OF ENVIRONMENT SETTING -------------
DEP_LIBRARIES := nvt@lib@nvtlive555
#--------- Compiling -------------------
BIN = pq_video_rtsp
SRC = nvtrtspd.c pq_video_rtsp.c
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
	$(CC) -o $@ $(OBJ) $(LD_FLAGS)
	$(NM) -n $@ > $@.sym
	$(STRIP) $@
	$(OBJCOPY) -R .comment -R .note.ABI-tag -R .gnu.version $@

%.o: %.c $(HEADER)
	$(CC) $(C_FLAGS) -c $< -o $@

clean:
	rm -vf $(BIN) $(OBJ) $(BIN).sym *.o *.a *.so*
endif

install:
	@echo ">>>>>>>>>>>>>>>>>>> $@ >>>>>>>>>>>>>>>>>>>"
	@mkdir -p $(APP_DIR)/output
	@cp -avf $(BIN) $(APP_DIR)/output
	@cp -avf ${BIN} ${ROOTFS_DIR}/rootfs/bin/
