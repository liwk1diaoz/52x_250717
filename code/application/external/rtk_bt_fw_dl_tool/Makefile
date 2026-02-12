
include $(NVT_PRJCFG_MODEL_CFG)

FW_USB_DIR := firmware
FW_UART_DIR := firmware/rtlbt
EXTERNAL := ${APP_DIR}/external
INSTALL_DIR = __install

ifeq (${BT_HCI_IF}, BT_HCI_IF_UART)
	ifeq (${BT_MDL}, BT_MDL_RTK_8821CS) 
		FW_PREFIX := ${FW_UART_DIR}/rtl8821c
		FW_FILE := ${FW_PREFIX}_fw 
		CONFIG_FILE := ${FW_PREFIX}_config 
	endif
endif

FW_INSTALL_DIR := ${EXTERNAL}/${INSTALL_DIR}/lib/${FW_UART_DIR}

.PHONY: rtk_hciattach firmware

all: rtk_hciattach firmware 

rtk_hciattach:
	@cd rtk_hciattach ; make;

firmware:
	@mkdir -p ${FW_INSTALL_DIR};
	@echo "Copy $(FW_FILE) to $(FW_INSTALL_DIR)"
	@cp -a ${FW_FILE} ${FW_INSTALL_DIR}/;
	@echo "Copy $(CONFIG_FILE) to $(FW_INSTALL_DIR)"
	@cp -a ${CONFIG_FILE} ${FW_INSTALL_DIR}/;
	
clean:
	@cd rtk_hciattach ; make clean;
