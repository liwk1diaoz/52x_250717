MODULE_NAME = libasound_module_pcm_anr
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
RTOS_OUTPUT_DIR = $(LIBRARY_DIR)/output
OUTPUT_DIR = $(RTOS_OUTPUT_DIR)/alsa-lib

.PHONY: modules modules_install clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
# INC FLAGs
EXTRA_INCLUDE += \
	-I$(NVT_VOS_DIR)/include \
	-I$(TOOLS_DIR)/__install/usr/include \
	-I$(LIBRARY_DIR)/include \
	
#--------- ENVIRONMENT SETTING --------------------
WARNING	= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	= -g -fPIC -DPIC -fno-short-enums -fno-common -nostdlib
C_PREDEFINED	=
C_CFLAGS	= $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE) $(C_PREDEFINED) -L$(TOOLS_DIR)/__install/usr/lib
LD_FLAGS	= -fPIC -DPIC -Wl,--no-undefined -shared -export-dynamic -L$(TOOLS_DIR)/__install/usr/lib -lasound -L$(LIBRARY_DIR)/output -lanr -lgcc_s -L$(HDAL_LIB_PATH) -lhdal
#--------- END OF ENVIRONMENT SETTING -------------
DEP_LIBRARIES := nvt@lib@anr
SRC += pcm_anr.c

OBJ = $(SRC:.c=.o)

ALSA_LIB := alsa-lib-1.1.9
TOOL_PATH := $(TOOLS_DIR)/$(ALSA_LIB)
ifeq ("$(wildcard $(TOOL_PATH))","")
all:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
	
install:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
else
ifeq ("$(wildcard *.c */*.c)","")
all:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
else
all: $(OBJ)
	@echo Creating library $(MODULE_NAME) ...
	@$(CC) -o $(MODULE_NAME).so $(OBJ) $(LD_FLAGS)

%.o:%.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) $< -c -o $@

clean:
	@rm -f *.a *.so*
endif

install:
	@mkdir -p $(OUTPUT_DIR)
	@cp -avf *.so* $(OUTPUT_DIR)
endif
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
	-I$(LIBRARY_DIR)/include \
	-I$(KDRV_DIR)/include \
	-I$(KDRV_DIR)/include/plat \
	-I$(KDRV_DIR)/source/include
	
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -DDEBUG -Wno-format -D__section_name__=$(MODULE_NAME)

SRC = \

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
