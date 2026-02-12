MODULE_NAME = libusockipc
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
RTOS_OUTPUT_DIR = $(LIBRARY_DIR)/output
OUTPUT_DIR = $(RTOS_OUTPUT_DIR)
RTOS_KERNEL_DIR = $(KERNELDIR)/lib/FreeRTOS
RTOS_CURR_DEMO_DIR = $(KERNELDIR)/demos/novatek/na51055
RTOS_POSIX_DIR = $(KERNELDIR)/lib/FreeRTOS-Plus-POSIX
RTOS_POSIX_SRC_DIR = $(RTOS_POSIX_DIR)/source
RTOS_LWIP_DIR = $(KERNELDIR)/lib/third_party/lwip/src
RTOS_LWIP_SRC_DIR = $(RTOS_LWIP_DIR)

# INC FLAGs
EXTRA_INCLUDE += \
	-I$(RTOS_LIB_DIR)/include \
	-I$(RTOS_KERNEL_DIR)/portable/GCC/ARM_CA9  \
	-I$(RTOS_KERNEL_DIR)/include \
	-I$(RTOS_KERNEL_DIR)/include/private \
	-I$(RTOS_CURR_DEMO_DIR)/include \
	-I$(VOS_DRIVER_DIR)/include \
	-I$(NVT_VOS_DIR)/include \
	-I$(RTOS_POSIX_DIR)/include \
	-I$(RTOS_POSIX_DIR)/include/portable/novatek \
	-I$(RTOS_LWIP_DIR)/include \
	-I$(RTOS_LWIP_SRC_DIR)/portable \
	-I$(RTOS_LWIP_SRC_DIR)/portable/arch \
	-I$(RTOS_LWIP_SRC_DIR)/include/posix \
	-I../../include \
	-I./UsockIpc

.PHONY: modules modules_install clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
WARNING	= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	= -I. -O2 -fPIC -ffunction-sections -fdata-sections -D__LINUX_USER__
C_CFLAGS	= $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE)
LD_FLAGS	= -fPIC -shared
#--------- END OF ENVIRONMENT SETTING -------------
SRC = \
		usocket/src/usocket.c \
		usocket/src/usocketudp.c \
		UsockIpcID.c \
		UsockIpcMsg.c \
		UsockIpcAPI.c \
		UsockIpcTsk.c \
		UsockIpcVerInfo.c \

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
C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -DDEBUG -Wno-format -DCONFIG_SUPPORT_AUTH=0 -DCONFIG_SUPPORT_CGI=0 -DHFS_TODO=0

SRC = \
        usocket/src/usocket.c \
        usocket/src/usocketudp.c \
		usocket/src/usocket_multi.c \
		UsockIpcID.c \
		UsockIpcMsg.c \
		UsockIpcAPI.c \
		UsockIpcTsk.c \
		UsockIpcVerInfo.c \

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
