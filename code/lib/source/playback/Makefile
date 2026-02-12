MODULE_NAME = libplayback
# DIRs
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
KFLOW_DIR =$(NVT_HDAL_DIR)/drivers/k_flow
RTOS_OUTPUT_DIR = $(LIBRARY_DIR)/output
OUTPUT_DIR = $(RTOS_OUTPUT_DIR)

.PHONY: modules modules_install clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
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
	-I$(KDRV_DIR)/include \
	-I$(KDRV_DIR)/include/plat \
	-I$(KFLOW_DIR)/include \
	-I../../include \
	-I../../include/PBXFileList \
	-I./

#--------- ENVIRONMENT SETTING --------------------
WARNING	= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	= -I. -O2 -fPIC -ffunction-sections -fdata-sections -D__LINUX_USER__
C_CFLAGS	= $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(WARNING) $(EXTRA_INCLUDE) $(C_PREDEFINED)
LD_FLAGS	= -fPIC -shared
#--------- END OF ENVIRONMENT SETTING -------------
SRC = \
    PlaybackID.c \
    PlayVerInfo.c \
    PlayAPI/P_Close.c \
    PlayAPI/P_GetCmd.c \
    PlayAPI/P_Open.c \
    PlayAPI/P_SetCmd.c \
    PlayAPI/P_WaitCmmdFinish.c \
    PlayAPI/API_GetFileInfo/P_GetCurDirId.c \
    PlayAPI/API_GetFileInfo/P_GetCurFileFormat.c \
    PlayAPI/API_GetFileInfo/P_GetCurFileNameId.c \
    PlayAPI/API_GetFileInfo/P_GetCurFileWithMemo.c \
    PlayAPI/API_GetFileInfo/P_GetCurImageInfo.c \
    PlayAPI/API_GetPlayInfo/P_GetPlayByDate.c \
    PlayAPI/API_GetPlayInfo/P_QuasiLoadFile.c \
    PlayAPI/API_SetPlayCommand/P_DisplaySetting.c \
    PlayAPI/API_SetPlayCommand/P_EditImage.c \
    PlayAPI/API_SetPlayCommand/P_OpenReadFile.c \
    PlayAPI/API_SetPlayCommand/P_PlayBrowser.c \
    PlayAPI/API_SetPlayCommand/P_PlaySingle.c \
    PlayAPI/API_SetPlayCommand/P_PlaySpecFile.c \
    PlayAPI/API_SetPlayCommand/P_PlayZoom.c \
    PlayAPI/API_SetPlayCommand/P_SetCaptureScreen.c \
    PlayAPI/API_SetPlayCommand/P_SetPlaybackObject.c \
    PlayAPI/API_SetPlayCommand/P_SetPlayByDate.c \
    PlayAPI/API_SetPlayCommand/P_SetScreenControl.c \
    PlayAPI/API_SetPlayCommand/P_SetVideoFrame.c \
    PlayAPI/API_SetPlayCommand/P_SystemSetting.c \
    PlayFuncByFileSys/P_File_GetFileStatus.c \
    PlayFuncByFileSys/P_File_ParseFileFormat.c \
    PlayFuncByFileSys/P_File_ReadFile.c \
    PlayFuncByFileSys/P_File_WaitInitial.c \
    PlayFuncByFileSys/P_File_WriteFile.c \
    PlayFuncData2Display/P_CopyImage2Display.c \
    PlayFuncData2Display/P_MoveImage2Display.c \
    PlayFuncData2Display/P_ReScaleRotate2Display.c \
    PlayFuncData2Display/P_Rotate2Display.c \
    PlayFuncData2Display/P_Single2Display.c \
    PlayFuncData2Display/P_Thumb2Display.c \
    PlayFuncData2Display/P_DispSrv.c \
    PlayFuncHWsetting/P_Decode_Start.c \
    PlayFuncHWsetting/P_SysInit.c \
    PlayFuncHWsetting/P_SysUnInit.c \
    PlayFuncHWsetting/P_Timer_Pause.c \
    PlayFuncHWsetting/P_Timer_Start.c \
    PlayFuncHWsetting/P_Timer_TimeOutHdl.c \
    PlayFuncHWsetting/P_Timer_Wait.c \
    PlayFuncUtility/P_DecodeOnePic.c \
    PlayFuncUtility/P_ExifOrientHdl.c \
    PlayFuncUtility/P_GetFileBuf_DefaultSize.c \
    PlayFuncUtility/P_GetFileBuf_FullSize.c \
    PlayFuncUtility/P_GetImageRatioTrans.c \
    PlayFuncUtility/P_InitThumbOffset.c \
    PlayFuncUtility/P_JpgBRCHandle.c \
    PlayFuncUtility/P_JPGDecode_ScaleHdl.c \
    PlayFuncUtility/P_PBMemoryAlloc.c \
    PlayFuncUtility/P_RotatePrimaryImg.c \
    PlayFuncUtility/P_Scale.c \
    PlayFuncUtility/P_SetFinishFlag.c \
    PlayFuncUtility/P_SetFrameBuffer.c \
    PlayFuncUtility/P_SetPlayCmmd.c \
    PlayFuncUtility/P_HdalUtility.c \
    PlayFuncZoom/P_ZoomInOut.c \
    PlayFuncZoom/P_ZoomPan.c \
    PlayFuncZoom/P_ZoomUserSetting.c \
    PlayMainHdl/P_Hdl_Browser.c \
    PlayMainHdl/P_Hdl_CaptureScreen.c \
    PlayMainHdl/P_Hdl_CropSave.c \
    PlayMainHdl/P_Hdl_DisplayImage.c \
    PlayMainHdl/P_Hdl_EditImage.c \
    PlayMainHdl/P_Hdl_EditUserEffect.c \
    PlayMainHdl/P_Hdl_Init.c \
    PlayMainHdl/P_Hdl_OpenFile.c \
    PlayMainHdl/P_Hdl_PlaySpecFile.c \
    PlayMainHdl/P_Hdl_ReQty.c \
    PlayMainHdl/P_Hdl_ReSize.c \
    PlayMainHdl/P_Hdl_RotateDisplay.c \
    PlayMainHdl/P_Hdl_SaveCurrRawBuf.c \
    PlayMainHdl/P_Hdl_Single.c \
    PlayMainHdl/P_Hdl_UpdateEXIF.c \
    PlayMainHdl/P_Hdl_Zoom.c \
    PlayMainTask/PlaybackTsk.c \
    PlayMainTask/PlaySysData.c

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
OUTPUT_NAME = lib$(MODULE_NAME).a

# This line should be removed later
EXTRA_INCLUDE += -I$(NVT_HDAL_DIR)/drivers/k_driver/source/kdrv_videocodec/include

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
	-I$(KDRV_DIR)/include \
	-I$(KDRV_DIR)/include/plat \
	-I$(KFLOW_DIR)/include \
	-I../../include \
	-I../../include/PBXFileList \
	-I./

C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE) -DDEBUG -Wno-format

SRC = \
    PlaybackID.c \
    PlayAPI/P_Close.c \
    PlayAPI/P_GetCmd.c \
    PlayAPI/P_Open.c \
    PlayAPI/P_SetCmd.c \
    PlayAPI/P_WaitCmmdFinish.c \
    PlayAPI/API_GetFileInfo/P_GetCurDirId.c \
    PlayAPI/API_GetFileInfo/P_GetCurFileFormat.c \
    PlayAPI/API_GetFileInfo/P_GetCurFileNameId.c \
    PlayAPI/API_GetFileInfo/P_GetCurFileWithMemo.c \
    PlayAPI/API_GetFileInfo/P_GetCurImageInfo.c \
    PlayAPI/API_GetPlayInfo/P_GetPlayByDate.c \
    PlayAPI/API_GetPlayInfo/P_QuasiLoadFile.c \
    PlayAPI/API_SetPlayCommand/P_DisplaySetting.c \
    PlayAPI/API_SetPlayCommand/P_EditImage.c \
    PlayAPI/API_SetPlayCommand/P_OpenReadFile.c \
    PlayAPI/API_SetPlayCommand/P_PlayBrowser.c \
    PlayAPI/API_SetPlayCommand/P_PlaySingle.c \
    PlayAPI/API_SetPlayCommand/P_PlaySpecFile.c \
    PlayAPI/API_SetPlayCommand/P_PlayZoom.c \
    PlayAPI/API_SetPlayCommand/P_SetCaptureScreen.c \
    PlayAPI/API_SetPlayCommand/P_SetPlaybackObject.c \
    PlayAPI/API_SetPlayCommand/P_SetPlayByDate.c \
    PlayAPI/API_SetPlayCommand/P_SetScreenControl.c \
    PlayAPI/API_SetPlayCommand/P_SetVideoFrame.c \
    PlayAPI/API_SetPlayCommand/P_SystemSetting.c \
    PlayFuncByFileSys/P_File_GetFileStatus.c \
    PlayFuncByFileSys/P_File_ParseFileFormat.c \
    PlayFuncByFileSys/P_File_ReadFile.c \
    PlayFuncByFileSys/P_File_WaitInitial.c \
    PlayFuncByFileSys/P_File_WriteFile.c \
    PlayFuncData2Display/P_CopyImage2Display.c \
    PlayFuncData2Display/P_MoveImage2Display.c \
    PlayFuncData2Display/P_ReScaleRotate2Display.c \
    PlayFuncData2Display/P_Rotate2Display.c \
    PlayFuncData2Display/P_Single2Display.c \
    PlayFuncData2Display/P_Thumb2Display.c \
    PlayFuncData2Display/P_DispSrv.c \
    PlayFuncHWsetting/P_Decode_Start.c \
    PlayFuncHWsetting/P_SysInit.c \
    PlayFuncHWsetting/P_SysUnInit.c \
    PlayFuncHWsetting/P_Timer_Pause.c \
    PlayFuncHWsetting/P_Timer_Start.c \
    PlayFuncHWsetting/P_Timer_TimeOutHdl.c \
    PlayFuncHWsetting/P_Timer_Wait.c \
    PlayFuncUtility/P_DecodeOnePic.c \
    PlayFuncUtility/P_ExifOrientHdl.c \
    PlayFuncUtility/P_GetFileBuf_DefaultSize.c \
    PlayFuncUtility/P_GetFileBuf_FullSize.c \
    PlayFuncUtility/P_GetImageRatioTrans.c \
    PlayFuncUtility/P_InitThumbOffset.c \
    PlayFuncUtility/P_JpgBRCHandle.c \
    PlayFuncUtility/P_JPGDecode_ScaleHdl.c \
    PlayFuncUtility/P_PBMemoryAlloc.c \
    PlayFuncUtility/P_RotatePrimaryImg.c \
    PlayFuncUtility/P_Scale.c \
    PlayFuncUtility/P_SetFinishFlag.c \
    PlayFuncUtility/P_SetFrameBuffer.c \
    PlayFuncUtility/P_SetPlayCmmd.c \
    PlayFuncUtility/P_HdalUtility.c \
    PlayFuncZoom/P_ZoomInOut.c \
    PlayFuncZoom/P_ZoomPan.c \
    PlayFuncZoom/P_ZoomUserSetting.c \
    PlayMainHdl/P_Hdl_Browser.c \
    PlayMainHdl/P_Hdl_CaptureScreen.c \
    PlayMainHdl/P_Hdl_CropSave.c \
    PlayMainHdl/P_Hdl_DisplayImage.c \
    PlayMainHdl/P_Hdl_EditImage.c \
    PlayMainHdl/P_Hdl_EditUserEffect.c \
    PlayMainHdl/P_Hdl_Init.c \
    PlayMainHdl/P_Hdl_OpenFile.c \
    PlayMainHdl/P_Hdl_PlaySpecFile.c \
    PlayMainHdl/P_Hdl_ReQty.c \
    PlayMainHdl/P_Hdl_ReSize.c \
    PlayMainHdl/P_Hdl_RotateDisplay.c \
    PlayMainHdl/P_Hdl_SaveCurrRawBuf.c \
    PlayMainHdl/P_Hdl_Single.c \
    PlayMainHdl/P_Hdl_UpdateEXIF.c \
    PlayMainHdl/P_Hdl_Zoom.c \
    PlayMainTask/PlaybackTsk.c \
    PlayMainTask/PlaySysData.c
    
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
