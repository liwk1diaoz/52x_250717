MODULE_NAME = libaac
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
RTOS_OUTPUT_DIR = $(LIBRARY_DIR)/output
OUTPUT_DIR = $(RTOS_OUTPUT_DIR)
HDAL_LIB_PATH = $(NVT_HDAL_DIR)/output

.PHONY: modules modules_install clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
# INC FLAGs
EXTRA_INCLUDE += \
	-I$(VOS_DRIVER_DIR)/include \
	-I$(NVT_VOS_DIR)/include \
	-I$(LIBRARY_DIR)/include \
	-I$(LIBRARY_DIR)/include/aac \
	
#--------- ENVIRONMENT SETTING --------------------
WARNING	= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	= -I. -O2 -fPIC -ffunction-sections -fdata-sections -D__LINUX_USER__
C_PREDEFINED	= -D_NVT_CONSOLE_
C_CFLAGS	= $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE) $(C_PREDEFINED)
LD_FLAGS	= -fPIC -shared -L$(HDAL_LIB_PATH) -lhdal
LD_FLAGS += -march=armv7-a -DARMV7 -nostdlib
LD_FLAGS += -fno-common 
LD_FLAGS += -fno-short-enums
#--------- END OF ENVIRONMENT SETTING -------------
SRC = \

uclibc=$(shell echo $(CROSS_COMPILE)|grep uclib)
ifeq ($(uclibc),)
    ENV_LIBC=glibc
else
    ENV_LIBC=uclibc
endif

OBJ = \
	$(SRC:.c=.o)

ifeq ("$(wildcard *.o */*.o)","")
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
	@rm -f *.a *.so*
	@rm -rf $(OUTPUT_DIR)
endif

install:
	# @mkdir -p $(OUTPUT_DIR)
	# @cp -avf *.so* *.a $(OUTPUT_DIR)
ifneq ("$(wildcard prebuild/$(ENV_LIBC)/*.a)","")
	@cp -avf prebuild/$(ENV_LIBC)/*.* $(OUTPUT_DIR)
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

