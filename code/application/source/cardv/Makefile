include $(NVT_PRJCFG_MODEL_CFG)
include MakeConfig.mk

#######################################################################################
#--------- INCs FOR C_CFLAGS ---------------------------------------------------------#
#######################################################################################
# DIRs
SYSROOT_DIR     = $(shell $(CC) $(PLATFORM_CFLAGS) -print-sysroot)
GCC_LIB_DIR     = $(dir $(shell $(CC) $(PLATFORM_CFLAGS) -print-libgcc-file-name))
SYSROOT_LIB_DIR = $(SYSROOT_DIR)/usr/lib
STDC_LIB_DIR    = $(dir $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=libstdc++.a))
LIBC_LIB_DIR    = $(dir $(shell $(CC) $(PLATFORM_CFLAGS) -print-file-name=libc.a))
HDAL_SAMPLE_DIR = $(NVT_HDAL_DIR)/samples
NVT_TOOLS_DIR = $(BUILD_DIR)/nvt-tools
VOS_DRIVER_DIR = $(NVT_VOS_DIR)/drivers
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
EXT_DIR = $(NVT_HDAL_DIR)/ext_devices
KFLOW_DIR = $(NVT_HDAL_DIR)/drivers/k_flow
OUTPUT_DIR = ./output
INSTALL_DIR = ../../output

# public includes
EXTRA_INCLUDE += \
	-I$(NVT_VOS_DIR)/include \
	-I$(NVT_HDAL_DIR)/include \
	-I$(NVT_HDAL_DIR)/vendor/isp/include \
	-I$(NVT_HDAL_DIR)/vendor/isp/drivers/include \
	-I$(LIBRARY_DIR)/include \
	-I$(KDRV_DIR)/include/plat \
	-I$(KDRV_DIR)/include/comm \
	-I$(KDRV_DIR)/include \
	-I$(EXT_DIR)/panel/display_panel/include \
	-I$(EXT_DIR)/sensor/sen_common \
	-I$(EXT_DIR)/audio/aud_common/include \
	-I$(KFLOW_DIR)/include \
	-I$(NVT_DRIVER_DIR)/include \
	-I$(LIBRARY_DIR)/include \
	-I$(APP_DIR)/include \
	-I$(KDRV_DIR)/source/kdrv_gfx2d/kdrv_affine/include \
	-I$(NVT_HDAL_DIR)/vendor/media/include \
	-I$(NVT_HDAL_DIR)/vendor/common/include \
    -I$(NVT_HDAL_DIR)/ext_devices/gyro/gyro_comm/include \
# application local includes
EXTRA_INCLUDE += \
	-I./SrcCode \
	-I./SrcCode/Startup \
	-I./SrcCode/System \
	-I./SrcCode/Dx/include \
	-I./SrcCode/HAL/remote_24G \
	-I./SrcCode/Gx/include \
	-I./SrcCode/Compatible \

ifeq "$(UI_STYLE)" "UI_STYLE_LVGL"

EXTRA_INCLUDE += \
	-I./SrcCode/UIWnd/LVGL_SPORTCAM \

endif

#--------- END OF INCs FOR C_CFLAGS ---------------------------------------------------

#######################################################################################
#--------- SOURCEs FOR APPLICATION ---------------------------------------------------#
#######################################################################################
ASM = \
	./SrcCode/Startup/code_info.S \

# system
SRC = \
	./SrcCode/Startup/startup.c \
	./SrcCode/Startup/bin_info.c \
	./SrcCode/System/main.c \
	./SrcCode/System/sys_mempool.c \
	./SrcCode/System/sys_filesys.c \
	./SrcCode/System/sys_fdt.c \
	./SrcCode/System/SysMain.c \
	./SrcCode/System/SysMain_Exe.c \
	./SrcCode/System/SysMain_State.c \
	./SrcCode/System/SysLock.c \
	./SrcCode/System/SysMain_Flow_Init.c \
	./SrcCode/System/SysMain_Flow_Exit.c \
	./SrcCode/System/SysMain_Flow_Sleep.c \
	./SrcCode/System/SysMain_Flow_Mode.c \
	./SrcCode/System/SysMain_cmd.c \
	./SrcCode/System/SysInput_cmd.c \
	./SrcCode/System/SysInput_Exe.c \
	./SrcCode/System/SysInput_CB.c \
	./SrcCode/System/SysPower_CB.c \
	./SrcCode/System/SysPower_Exe.c \
	./SrcCode/System/SysPower_cmd.c \
	./SrcCode/System/SysVideo_Exe.c \
	./SrcCode/System/SysUsb_CB.c \
	./SrcCode/System/SysUsb_Exe.c \
	./SrcCode/System/SysAudio_Exe.c \
	./SrcCode/System/SysStrg_CB.c \
	./SrcCode/System/SysStrg_Exe.c \
	./SrcCode/System/SysSensor_Exe.c \
	./SrcCode/UIApp/UIAppObj.c \
	./SrcCode/UIApp/ExifVendor.c \
	./SrcCode/UIApp/Setup/UISetup_Exe.c \
	./SrcCode/UIApp/Movie/UIAppMovie_Exe.c \
	./SrcCode/UIApp/Movie/UIAppMovie_CommPoolInit.c \
	./SrcCode/UIApp/Movie/UIAppMovie_RecSetting.c \
	./SrcCode/UIApp/Movie/UIAppMovie_AlgFunc_MD.c \
	./SrcCode/UIApp/Movie/UIAppMovie_LiveStream.c \
	./SrcCode/UIApp/Movie/UIAppMovie_cmd.c \
	./SrcCode/UIApp/Movie/UIAppMovie_SetUserData.c \
	./SrcCode/UIApp/MovieStamp/MovieStamp.c \
	./SrcCode/UIApp/MovieStamp/MovieStampTsk.c \
	./SrcCode/UIApp/MovieStamp/MovieStampID.c \
	./SrcCode/UIApp/Photo/UIAppPhoto_Exe.c \
	./SrcCode/UIApp/Photo/UIAppPhoto_Param.c \
	./SrcCode/UIApp/Photo/UIAppPhoto_cmd.c \
	./SrcCode/UIApp/Photo/UIAppPhoto_CB.c \
	./SrcCode/UIApp/Photo/UIDateImprint.c \
	./SrcCode/UIApp/Photo/UIDateImprintID.c \
	./SrcCode/UIApp/Photo/UIStorageCheck.c \
	./SrcCode/UIApp/Play/UIAppPlay_Exe.c \
	./SrcCode/UIApp/Play/UIAppMoviePlay_Exe.c \
	./SrcCode/UIApp/Play/UIPlayComm.c \
	./SrcCode/UIApp/AppDisp_PBView.c \
	./SrcCode/UIApp/AppDisp_PipView.c \
	./SrcCode/UIApp/MovieUdtaVendor.c \
	./SrcCode/UIApp/Background/UIBackgroundObj.c \
	./SrcCode/UIApp/UsbMsdcCb/MsdcNvtCb_Adj.c \
	./SrcCode/UIApp/UsbMsdcCb/MsdcNvtCb_CustomSi.c \
	./SrcCode/UIApp/UsbMsdcCb/MsdcNvtCb_ISP.c \
	./SrcCode/UIApp/UsbMsdcCb/MsdcNvtCb_UpdFw.c \
	./SrcCode/UIApp/UsbMsdcCb/MsdcNvtCb.c \
	./SrcCode/UIApp/UsbCam/UIAppUsbCam_Exe.c \
	./SrcCode/UIApp/UsbCam/UIAppUsbCam_CommPoolInit.c \
	./SrcCode/UIApp/UsbDisk/UIAppUsbDisk_Exe.c \
	./SrcCode/UIApp/MovieStamp/BG_Watermark_640x480.c \
	./SrcCode/UIApp/MovieStamp/BG_Watermark_1920x1440.c \
	./SrcCode/UIApp/Network/NetFs.c \
	./SrcCode/UIApp/Network/UIAppNetwork.c \
	./SrcCode/UIApp/Network/UIAppNetwork_cmd.c \
	./SrcCode/UIApp/Network/UIAppWiFiCmd_Exe.c \
	./SrcCode/UIApp/Network/UIAppWiFiCmdMovie_Exe.c \
	./SrcCode/UIApp/Network/UIAppWiFiCmdPhoto_Exe.c \
	./SrcCode/UIApp/Network/WifiAppCmd.c \
	./SrcCode/UIApp/Network/WifiAppCmdMapping.c \
	./SrcCode/UIApp/Network/WifiAppXML.c \
	./SrcCode/UIApp/Network/EthCamAppCmd.c \
	./SrcCode/UIApp/Network/EthCamAppNetwork.c \
	./SrcCode/UIApp/Network/EthCamAppSocket.c \
	./SrcCode/UIApp/WifiCmdParser/WifiCmdParser.c \
	./SrcCode/UIApp/WifiCmdParser/WifiCmdParserID.c \
	./SrcCode/UIApp/EthCamCmdParser/EthCamCmdParser.c \
	./SrcCode/UIApp/EthCamCmdParser/EthCamCmdParserID.c \
	./SrcCode/UIApp/UsbCommand/UsbCmd.c \
	./SrcCode/UIApp/UsbCommand/UsbCmdTsk.c \
	./SrcCode/UIApp/lfqueue/lfqueue.c \
	./SrcCode/Mode/UIMode.c \
	./SrcCode/Mode/UIModeMain.c \
	./SrcCode/Mode/UIModeMovie.c \
	./SrcCode/Mode/UIModePhoto.c \
	./SrcCode/Mode/UIModePlayback.c \
	./SrcCode/Mode/UIModeUpdFw.c \
	./SrcCode/Mode/UIModeUsbCam.c \
	./SrcCode/Mode/UIModeUsbDisk.c \
	./SrcCode/Mode/UIModeUsbMenu.c \
	./SrcCode/Mode/UIModeUsbPTP.c \
	./SrcCode/Mode/UIMode_cmd.c \
	./SrcCode/Mode/UIModeWifi.c
	
ifeq ($(PACKAGE_HEIC),1)

SRC += \
	./SrcCode/UIApp/Movie/UIAppMovie_SampleCode_HEIC.c
	
endif

ifeq "$(UI_STYLE)" "UI_STYLE_LVGL"
SRC += \
	./SrcCode/UIApp/lv_user_font_conv/lv_user_font_conv.c \
	./SrcCode/UIApp/lv_user_font_conv/lv_user_font_conv_cmd.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIInfo/UIInfo.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIInfo/UIMovieMapping.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIInfo/UIPhotoMapping.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIInfo/DateTimeInfo.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/BG_Opening.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/UIResource.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/SoundData.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Images/images.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Languages/Language_DE.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Languages/Language_EN.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Languages/Language_ES.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Languages/Language_FR.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Languages/Language_IT.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Languages/Language_JP.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Languages/Language_PO.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Languages/Language_RU.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Languages/Language_SC.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Languages/Language_TC.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Fonts/notosans_black_16_1bpp.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Fonts/notosans_black_32_1bpp.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Fonts/notosans_black_48_1bpp.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Fonts/notosans_black_64_1bpp.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Fonts/notosans_black_128_1bpp.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Fonts/notosanscjkjp_black_16_1bpp.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Fonts/notosanscjksc_black_16_1bpp.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Fonts/notosanscjktc_black_16_1bpp.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Plugin/lv_plugin_common.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Plugin/lv_plugin_menu.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/Resource/Palette/palette.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowLVGL.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowUSB/UIFlowUSB.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowUSB/UIFlowUSBEventCallback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowUSBMenu/UIFlowUSBMenu.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowUSBMenu/UIFlowUSBMenuEventCallback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowWifiWait/UIFlowWifiWait.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowWifiWait/UIFlowWifiWaitEventCallback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowWifiLink/UIFlowWifiLink.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowWifiLink/UIFlowWifiLinkEventCallback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowWifiLinkOK/UIFlowWifiLinkOK.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowWifiLinkOK/UIFlowWifiLinkOKEventCallback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMovie/UIFlowMovie.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMovie/UIFlowMovieEventCallback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMovie/UIFlowMovieFuncs.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowPhoto/UIFlowPhoto.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowPhoto/UIFlowPhotoEventCallback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowPhoto/UIFlowPhotoFuncs.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowPhoto/UIFlowPhotoParams.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowWrnMsg/UIFlowWrnMsg.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowWrnMsg/UIFlowWrnMsgEventCallback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowWaitMoment/UIFlowWaitMoment.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowWaitMoment/UIFlowWaitMomentEventCallback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMenuCommonConfirm/UIFlowMenuCommonConfirm.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMenuCommonConfirm/UIFlowMenuCommonConfirmEventCallback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMenuCommonOption/UIFlowMenuCommonOption.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMenuCommonOption/UIFlowMenuCommonOptionEventCallback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMenuCommonItem/UIFlowMenuCommonItem.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMenuCommonItem/UIFlowMenuCommonItemEventCallback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowSetupDateTime/UIFlowSetupDateTime.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowSetupDateTime/UIFlowSetupDateTimeEventCallback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMenuCommonItem/TabMenu.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMenuCommonItem/MenuCommon.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMenuCommonItem/MenuMovie.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMenuCommonItem/MenuPhoto.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMenuCommonItem/MenuSetup.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMenuCommonItem/MenuPlayback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowMenuCommonItem/MenuMode.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowPlay/UIFlowPlay.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowPlay/UIFlowPlayEventCallback.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowPlay/UIFlowPlayFuncs.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowPlayThumb/UIFlowPlayThumb.c \
	./SrcCode/UIWnd/LVGL_SPORTCAM/UIFlowLVGL/UIFlowPlayThumb/UIFlowPlayThumbEventCallback.c \
	./SrcCode/lv_drivers/display/fbdev.c \
	
endif

ifeq "$(UI_STYLE)" "UI_STYLE_SPORTCAM"

SRC += \
	./SrcCode/UI/SysUICB.c \
	./SrcCode/UI/UIGraphicsID.c \
	./SrcCode/UI/UIGraphics.c \
	./SrcCode/UI/UIBuffer.c \
	./SrcCode/UI/UIView.c \
	./SrcCode/UI/UIDisplay.c \
	./SrcCode/UIWnd/SPORTCAM/UIInfo/UIInfo.c \
	./SrcCode/UIWnd/SPORTCAM/UIInfo/UIMovieMapping.c \
	./SrcCode/UIWnd/SPORTCAM/UIInfo/UIPhotoMapping.c \
	./SrcCode/UIWnd/SPORTCAM/UIInfo/DateTimeInfo.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/BG_Opening.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/UIResource.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_Font.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_Image.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_Palette.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_PaletteConfirm.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_PaletteOption.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_String_EN.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_String_DE.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_String_ES.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_String_PO.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_String_IT.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_String_FR.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_String_SC.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_String_TC.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_String_JP.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/DemoKit_String_RU.c \
	./SrcCode/UIWnd/SPORTCAM/Resource/SoundData.c \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowMovie/UIFlowWndMovie.c \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowMovie/UIFlowMovieFuncs.c \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowMovie/UIFlowMovieIcons.c \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowCommon/UIFlowWndUSB.c \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowCommon/UIFlowWndWaitMoment.c \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowCommon/UIFlowWndWrnMsg.c \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowPhoto/UIFlowWndPhoto.c  \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowPhoto/UIFlowPhotoFuncs.c  \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowPhoto/UIFlowPhotoIcons.c  \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowPhoto/UIFlowPhotoParams.c  \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowPlay/UIFlowCBSlideShow.c \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowPlay/UIFlowPlayFuncs.c \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowPlay/UIFlowPlayIcons.c \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowPlay/UIFlowWndPlay.c \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowPlay/UIFlowWndPlayMagnify.c \
	./SrcCode/UIWnd/SPORTCAM/UIFlow/UIFlowPlay/UIFlowWndPlayThumb.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuCommon/TabMenu.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuCommon/MenuMode.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuCommon/MenuCommon.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuCommon/MenuCommonConfirm.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuCommon/MenuCommonItem.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuCommon/MenuCommonOption.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuMovie/MenuMovie.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuPhoto/MenuPhoto.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuSetup/MenuSetup.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuSetup/UIMenuWndSetupDateTime.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuPlay/MenuPlayback.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuUSB/UIMenuWndUSB.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuWiFi/UIMenuWndWiFiMobileLinkOK.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuWiFi/UIMenuWndWiFiModuleLink.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuWiFi/UIMenuWndWiFiWait.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuWiFi/UIMenuWndWiFiLivestream.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuWiFi/UIMenuWndWiFiQRCodeScan.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl56x96.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl42x72.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl36x60.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl26x44.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl20x44.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl18x30.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl12x20.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl10x16.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont26x44.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont20x44.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont18x30.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont12x20.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont10x16.c 
endif

ifeq "$(UI_STYLE)" "UI_STYLE_SPORTCAM_TOUCH"
SRC += \
	./SrcCode/UI/SysUICB.c \
	./SrcCode/UI/UIGraphicsID.c \
	./SrcCode/UI/UIGraphics.c \
	./SrcCode/UI/UIBuffer.c \
	./SrcCode/UI/UIView.c \
	./SrcCode/UI/UIDisplay.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIInfo/UIInfo.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIInfo/UIMovieMapping.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIInfo/UIPhotoMapping.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIInfo/DateTimeInfo.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/BG_Opening.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/UIResource.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_Font.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_Image.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_Palette.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_PaletteConfirm.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_PaletteOption.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_String_EN.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_String_DE.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_String_ES.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_String_PO.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_String_IT.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_String_FR.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_String_SC.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_String_TC.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_String_JP.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/DemoKit_String_RU.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/Resource/SoundData.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowMovie/UIFlowWndMovie.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowMovie/UIFlowMovieFuncs.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowMovie/UIFlowMovieIcons.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowCommon/UIFlowWndUSB.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowCommon/UIFlowWndWaitMoment.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowCommon/UIFlowWndWrnMsg.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPhoto/UIFlowWndPhoto.c  \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPhoto/UIFlowPhotoFuncs.c  \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPhoto/UIFlowPhotoIcons.c  \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPhoto/UIFlowPhotoParams.c  \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPlay/UIFlowCBSlideShow.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPlay/UIFlowPlayFuncs.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPlay/UIFlowPlayIcons.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPlay/UIFlowWndPlay.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPlay/UIFlowWndPlayMagnify.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPlay/UIFlowWndPlayThumb.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuCommon/TabMenu.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuCommon/MenuMode.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuCommon/MenuCommon.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuCommon/MenuCommonConfirm.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuCommon/MenuCommonItem.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuCommon/MenuCommonOption.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuMovie/MenuMovie.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuPhoto/MenuPhoto.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuSetup/MenuSetup.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuSetup/UIMenuWndSetupDateTime.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuPlay/MenuPlayback.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuUSB/UIMenuWndUSB.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuWiFi/UIMenuWndWiFiMobileLinkOK.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuWiFi/UIMenuWndWiFiModuleLink.c \
	./SrcCode/UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuWiFi/UIMenuWndWiFiWait.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl56x96.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl42x72.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl36x60.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl26x44.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl20x44.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl18x30.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl12x20.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl10x16.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont26x44.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont20x44.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont18x30.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont12x20.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont10x16.c 	
endif

ifeq "$(UI_STYLE)" "UI_STYLE_ALEXA"
SRC += \
	./SrcCode/UI/SysUICB.c \
	./SrcCode/UI/UIGraphicsID.c \
	./SrcCode/UI/UIGraphics.c \
	./SrcCode/UI/UIBuffer.c \
	./SrcCode/UI/UIView.c \
	./SrcCode/UI/UIDisplay.c \
	./SrcCode/UIWnd/ALEXA/UIInfo/UIInfo.c \
	./SrcCode/UIWnd/ALEXA/UIInfo/UIMovieMapping.c \
	./SrcCode/UIWnd/ALEXA/UIInfo/UIPhotoMapping.c \
	./SrcCode/UIWnd/ALEXA/UIInfo/DateTimeInfo.c \
	./SrcCode/UIWnd/ALEXA/Resource/BG_Opening.c \
	./SrcCode/UIWnd/ALEXA/Resource/UIResource.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_Font.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_Image.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_Palette.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_PaletteConfirm.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_PaletteOption.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_String_EN.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_String_DE.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_String_ES.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_String_PO.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_String_IT.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_String_FR.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_String_SC.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_String_TC.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_String_JP.c \
	./SrcCode/UIWnd/ALEXA/Resource/DemoKit_String_RU.c \
	./SrcCode/UIWnd/ALEXA/Resource/SoundData.c \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowMovie/UIFlowWndMovie.c \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowMovie/UIFlowMovieFuncs.c \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowMovie/UIFlowMovieIcons.c \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowCommon/UIFlowWndUSB.c \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowCommon/UIFlowWndWaitMoment.c \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowCommon/UIFlowWndWrnMsg.c \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowPhoto/UIFlowWndPhoto.c  \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowPhoto/UIFlowPhotoFuncs.c  \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowPhoto/UIFlowPhotoIcons.c  \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowPhoto/UIFlowPhotoParams.c  \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowPlay/UIFlowCBSlideShow.c \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowPlay/UIFlowPlayFuncs.c \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowPlay/UIFlowPlayIcons.c \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowPlay/UIFlowWndPlay.c \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowPlay/UIFlowWndPlayMagnify.c \
	./SrcCode/UIWnd/ALEXA/UIFlow/UIFlowPlay/UIFlowWndPlayThumb.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuCommon/TabMenu.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuCommon/MenuMode.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuCommon/MenuCommon.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuCommon/MenuCommonConfirm.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuCommon/MenuCommonItem.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuCommon/MenuCommonOption.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuMovie/MenuMovie.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuPhoto/MenuPhoto.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuSetup/MenuSetup.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuSetup/UIMenuWndSetupDateTime.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuPlay/MenuPlayback.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuUSB/UIMenuWndUSB.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuWiFi/UIMenuWndWiFiMobileLinkOK.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuWiFi/UIMenuWndWiFiModuleLink.c \
	./SrcCode/UIWnd/ALEXA/UIMenu/UIMenuWiFi/UIMenuWndWiFiWait.c \
	./SrcCode/UIWnd/SPORTCAM/UIMenu/UIMenuWiFi/UIMenuWndWiFiWait.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl56x96.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl42x72.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl36x60.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl26x44.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl20x44.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl18x30.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl12x20.c \
	./SrcCode/UIApp/MovieStamp/DateStampFontTbl10x16.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont26x44.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont20x44.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont18x30.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont12x20.c \
	./SrcCode/UIApp/MovieStamp/DateStampFont10x16.c 
endif

# Dx
-include ./SrcCode/Dx/$(MODEL)/Source.mk
SRC += $(DX_SRC) \
	./SrcCode/Dx/DxVer.c \
	./SrcCode/Dx/DxCommon.c \

# HAL
SRC += \
	./SrcCode/HAL/remote_24G/hal_remote_main.c \
	./SrcCode/HAL/remote_24G/hal_remote_op.c \
	./SrcCode/HAL/remote_24G/hal_remote_uart.c \

# Gx
SRC += \
	./SrcCode/Gx/GxInput/Key/GxKey.c \
	./SrcCode/Gx/GxInput/Touch/Calibrate.c \
	./SrcCode/Gx/GxInput/Touch/Gesture.c \
	./SrcCode/Gx/GxInput/Touch/GxTouch.c \
	./SrcCode/Gx/GxInput/Touch/GxTouch_cmd.c \
	./SrcCode/Gx/GxPower/GxTime.c \
	./SrcCode/Gx/GxPower/GxPower.c \
	./SrcCode/Gx/GxPower/GxPower_cmd.c \
	./SrcCode/Gx/GxPower/GxPowerVerInfo.c \
	./SrcCode/Gx/GxSound/sxsound/PlaySoundAPI.c \
	./SrcCode/Gx/GxSound/sxsound/PlaySoundTsk.c \
	./SrcCode/Gx/GxSound/sxsound/PlaySoundUtil.c \
	./SrcCode/Gx/GxSound/GxSound.c \
	./SrcCode/Gx/GxSound/GxSoundID.c \
	./SrcCode/Gx/GxTimer/GxTimer.c \
	./SrcCode/Gx/GxTimer/GxTimer_cmd.c \
	./SrcCode/Gx/GxTimer/GxTimer_p1.c \
	./SrcCode/Gx/GxTimer/GxTimerID.c \
	./SrcCode/Gx/GxUSB/GxUSB.c \
	./SrcCode/Gx/GxVideo/GxVideo.c \
	./SrcCode/Gx/GxVideo/GxVideo_cmd.c \
	./SrcCode/Gx/GxVideo/GxVideoVer.c \
	./SrcCode/Gx/GxDisplay/GxDisplay.c \
	./SrcCode/Gx/GxDisplay/DL1_ndk08.c \
	./SrcCode/Gx/GxDisplay/GxDisplay_cmd.c \
	./SrcCode/Gx/GxDisplay/GxDisplayVer.c \
	./SrcCode/Gx/GxStrg/GxStrg.c \
	./SrcCode/Gx/GxStrg/GxStrgDevice.c \
	./SrcCode/Gx/GxStrg/GxStrgLnx.c \
	./SrcCode/Gx/GxStrg/GxStrg_p1.c \
	./SrcCode/Gx/GxStrg/GxStrgVerInfo.c \

ifeq (${BT_STACK}, BT_STACK_BLUEZ)

EXTLIB = EXTLIB = ${LIBRARY_DIR}/external
BLUEZ := bluez-5.64

SRC += \
	./SrcCode/UIApp/Bluez/adapter.c \
	./SrcCode/UIApp/Bluez/device.c \
	./SrcCode/UIApp/Bluez/device_manager.c \
	./SrcCode/UIApp/Bluez/connection.c \
	./SrcCode/UIApp/Bluez/proxy.c \
	./SrcCode/UIApp/Bluez/property_proxy.c \
	./SrcCode/UIApp/Bluez/gatt.c \
	./SrcCode/UIApp/Bluez/UIAppBluez.c \

EXTRA_INCLUDE += \
	-I${EXTLIB}/${BLUEZ}/ \
	-I${EXTLIB}/__install/usr/include/ \
	-I${EXTLIB}/__install/usr/include/dbus-1.0 \
	-I${EXTLIB}/__install/usr/lib/dbus-1.0/include/ \
	-I${EXTLIB}/__install/usr/include/glib-2.0/ \
	-I${EXTLIB}/__install/usr/lib/glib-2.0/include/ \
	-I${EXTLIB}/__install/usr/include/glib-2.0 \
	-I${EXTLIB}/__install/usr/include/glib-2.0/gio \

endif
	
CPP_SRC = \

#--------- END OF SOURCEs FOR APPLICATION ---------------------------------------------

#######################################################################################
#--------- include common parts of the makefile ---------------------------------------
#######################################################################################
include OutputImg.mk
