MODULE_NAME = libimageapp_common
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
KFLOW_DIR = $(NVT_HDAL_DIR)/drivers/k_flow
RTOS_OUTPUT_DIR = $(LIBRARY_DIR)/output
OUTPUT_DIR = $(RTOS_OUTPUT_DIR)

.PHONY: modules modules_install clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
# INC FLAGs
EXTRA_INCLUDE += \
	-I$(VOS_DRIVER_DIR)/include \
	-I$(NVT_VOS_DIR)/include \
	-I$(NVT_DRIVER_DIR)/include \
	-I$(NVT_HDAL_DIR)/include \
	-I$(KFLOW_DIR)/include \
	-I../../../include \
	-I$(NVT_HDAL_DIR)/vendor/media/include

#--------- ENVIRONMENT SETTING --------------------
WARNING	= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	= -I. -O2 -fPIC -ffunction-sections -fdata-sections -D__LINUX_USER__
C_PREDEFINED	= -D_NVT_CONSOLE_ -D__LINUX520
C_CFLAGS	= $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE) $(C_PREDEFINED)
LD_FLAGS	= -fPIC -shared -L$(NVT_HDAL_DIR)/vendor/output -lvendor_media
#--------- END OF ENVIRONMENT SETTING -------------
SRC = \
      ImageApp_Common.c \
      ImageApp_Common_CB.c \
      ImageApp_Common_Param.c \
      ImageApp_Common_Utilities.c \
      ImageApp_Common_VerInfo.c \
      iacomm_audiocap.c \
      iacomm_audiodec.c \
      iacomm_audioenc.c \
      iacomm_audioout.c \
      iacomm_bsmux.c \
      iacomm_fileout.c \
      iacomm_live555.c \
      iacomm_lviewnvt.c \
      iacomm_queue.c \
      iacomm_videocap.c \
      iacomm_videodec.c \
      iacomm_videoenc.c \
      iacomm_videoout.c \
      iacomm_videoprocess.c \
      nvtrtspd.c

OBJ = $(SRC:.c=.o)

ifeq ("$(wildcard *.c */*.c)","")
all:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
else
all: $(OBJ)
	@echo Creating library $(MODULE_NAME) ...
	@$(CC) -o $(MODULE_NAME).so $(OBJ) $(LD_FLAGS)
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
	@cp -avf *.so* *.a $(OUTPUT_DIR)
###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
OUTPUT_NAME = $(MODULE_NAME).a
# INC FLAGs
EXTRA_INCLUDE += \
	-I$(RTOS_LIB_DIR)/include \
	-I$(RTOS_KERNEL_DIR)/portable/GCC/ARM_CA9  \
	-I$(RTOS_KERNEL_DIR)/include \
	-I$(RTOS_KERNEL_DIR)/include/private \
	-I$(RTOS_CURR_DEMO_DIR)/include \
	-I$(VOS_DRIVER_DIR)/include \
	-I$(NVT_VOS_DIR)/include \
	-I$(NVT_DRIVER_DIR)/include \
	-I$(NVT_HDAL_DIR)/include \
	-I$(KFLOW_DIR)/include \
	-I../../../include \
	-I$(NVT_HDAL_DIR)/vendor/media/include

C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -DDEBUG -Wno-format

SRC = \
      ImageApp_Common.c \
      ImageApp_Common_CB.c \
      ImageApp_Common_Param.c \
      ImageApp_Common_Utilities.c \
      ImageApp_Common_VerInfo.c \
      iacomm_audiocap.c \
      iacomm_audiodec.c \
      iacomm_audioenc.c \
      iacomm_audioout.c \
      iacomm_bsmux.c \
      iacomm_fileout.c \
      iacomm_live555.c \
      iacomm_lviewnvt.c \
      iacomm_queue.c \
      iacomm_videocap.c \
      iacomm_videodec.c \
      iacomm_videoenc.c \
      iacomm_videoout.c \
      iacomm_videoprocess.c \
      nvtrtspd.c

copy = if [ ! -z "$(1)" -a "$(1)" != " " ]; then cp -avf $(1) $(2); fi

OBJ = $(SRC:.c=.o)

ifeq ("$(wildcard *.c */*.c */*/*.c)","")
all:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
else
all : $(OUTPUT_NAME)

%.o:%.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

$(OUTPUT_NAME): $(OBJ)
	@echo Creating library $* ...
	@$(AR) rcsD $(OUTPUT_NAME) $(OBJ)
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --arc-sha1 $@

clean:
	@rm -f $(OBJ) *.o *.a *.so*
endif

install:
	@mkdir -p $(RTOS_OUTPUT_DIR)
	@cp -avf *.a $(RTOS_OUTPUT_DIR)

endif
