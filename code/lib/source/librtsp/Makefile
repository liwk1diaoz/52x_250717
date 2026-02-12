include $(NVT_PRJCFG_MODEL_CFG)
#--------- ENVIRONMENT SETTING --------------------
INCLUDES        = -I$(LIBRARY_DIR)/include -I$(NVT_HDAL_DIR)/include
WARNING         = -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes
COMPILE_OPTS    = $(INCLUDES) -I. -O2 -fPIC -ffunction-sections -fdata-sections -DGLIB
CPPFLAGS        =
CFLAGS          = $(PLATFORM_CFLAGS) $(PRJCFG_CFLAGS)
C_FLAGS         = $(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS) $(WARNING) -fcommon
LD_FLAGS        = -fPIC -lpthread -L$(LIBRARY_DIR)/output
#--------- END OF ENVIRONMENT SETTING -------------
DEP_LIBRARIES :=

#--------- Compiling --------------------
LIB_NAME=librtsp
INC = librtsp.h
INC_PROTECT =
SRC = rtp-h264.c rtp-h265.c rtp-mp4.c rtp-jpeg.c rtp-mpa.c rtp-aac.c rtp-amr.c rtp-rawaudio.c rtp-g711.c rtp-g726.c \
			tcp.c rtp.c rtsp.c http.c live.c session.c spook.c rtspman.c http-auth.c rtsp-md5.c \
			stream.c event.c pmsg.c priv.c log.c gm_memory.c sdp_param_encode.c rtspd_api.c
HEADER = $(shell find . -name "*.h")

copy = if [ ! -z "$(1)" -a "$(1)" != " " ]; then cp -avf $(1) $(2); fi

OBJ = $(SRC:.c=.o)

SHELL = /bin/bash
MULTI_CORES = $(shell grep -c ^processor /proc/cpuinfo)

ifeq ("$(wildcard *.c */*.c)","")
all:
	@echo ">>> Skip"
clean:
	@echo ">>> Skip"
install_header:
	@echo ">>> Skip"
else

.PHONY: librtsp
all: librtsp

librtsp: $(OBJ)
	$(AR) rcs $(LIB_NAME).a $(OBJ)

%.o:%.c $(HEADER)
	$(CC) $(C_FLAGS) $< -c

clean:
	rm -vf $(OBJ) *.o *.a *.so*

install_header:
	@$(call copy, $(INC), $(LIBRARY_DIR)/include)
	@$(call copy, $(INC_PROTECT), $(LIBRARY_DIR)/include/protected)
endif

install: install_header
	@mkdir -p $(LIBRARY_DIR)/output
	@cp -avf *.a $(LIBRARY_DIR)/output
