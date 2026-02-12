#ifndef _PRJCFG_DVCAM_H_
#define _PRJCFG_DVCAM_H_
#include "kwrap/type.h"

//==============================================================================
//   MODE and Major Feature (Maintain these in \Kernel, \System, \Mode folder)
//==============================================================================
#define PLAY_MODE                       ENABLE
#define PHOTO_MODE                      ENABLE
#define MOVIE_MODE                      ENABLE
#define USB_MODE                        ENABLE
#define SLEEP_MODE                      DISABLE
#define ENG_MODE                        DISABLE
#define MAIN_MODE                       ENABLE
#define IPCAM_MODE                      DISABLE
#define UPDFW_MODE                      ENABLE

#define IPCAM_FUNC                      DISABLE
#define STAMP_FUNC                      DISABLE
#define UI_FUNC                         ENABLE
#if !defined(_NVT_SDIO_WIFI_NONE_) || !defined(_NVT_USB_WIFI_NONE_)
#define WIFI_FUNC                       ENABLE
#else
#define WIFI_FUNC                       ENABLE
#endif

//==============================================================================
//   Ethcam related setting
//==============================================================================
#if (defined(_NVT_ETHREARCAM_TX_) || defined(_NVT_ETHREARCAM_RX_))
#if defined(_NVT_ETHREARCAM_CAPS_COUNT_2_)
#define ETH_REARCAM_CAPS_COUNT              	2
#define ETH_REARCAM_CLONE_FOR_DISPLAY       DISABLE
#else
#define ETH_REARCAM_CAPS_COUNT              1
#define ETH_REARCAM_CLONE_FOR_DISPLAY       ENABLE
#endif
#endif

//==============================================================================
//   GENERAL CONFIG
//==============================================================================
//..............................................................................
#if defined(_FW_PARTIAL_LOAD_ON_)
#else
#endif

//..............................................................................
// FW version and name
#define FW_UPDATE_NAME                  "A:\\FW98529Z.bin"
#define FW_VERSION_RELEASE              DISABLE //DISABLE
#define FW_VERSION_NUM                  "REGULAR_0402_001"

//..............................................................................
/**
     The definition of EmbMem is the storage without any card detection,
     which means the end-user cannot take that storage out. The EmbMem can also
     be a combination of partition such as partition 0 is spi-uITRON,
     partition 1 is ram-FAT these depend on what you designed on DxStorage_Mem.c
     Normally, in SPI case, we design the ram-disk as internal FAT

*/
// share info memory on fdt
#define SHMEM_PATH "/nvt_memory_cfg/shmem"

// Internal Storage Mapping to Storage Object
#define STRG_OBJ_FW_FDT   STRG_OBJ_FW_RSV1
#define STRG_OBJ_FW_APP   STRG_OBJ_FW_RSV2
#define STRG_OBJ_FW_UBOOT STRG_OBJ_FW_RSV3
#define STRG_OBJ_FW_RTOS  STRG_OBJ_FW_RSV4
#define STRG_OBJ_FW_PST   STRG_OBJ_PSTORE1
// Internal Storage Mapping to DxStorage
#define USER_DX_TYPE_EMBMEM_LOADER      DX_TYPE_EMBMEM0 //STRG_OBJ_FW_RSV1
#define USER_DX_TYPE_EMBMEM_MODELEXT    DX_TYPE_EMBMEM1 //STRG_OBJ_FW_RSV2
#define USER_DX_TYPE_EMBMEM_UITRON      DX_TYPE_EMBMEM2 //STRG_OBJ_FW_RSV3
#define USER_DX_TYPE_EMBMEM_UBOOT       DX_TYPE_EMBMEM3 //STRG_OBJ_FW_RSV4
#define USER_DX_TYPE_EMBMEM_LINUX       DX_TYPE_EMBMEM4 //STRG_OBJ_FW_RSV5
#define USER_DX_TYPE_EMBMEM_ECOS        DX_TYPE_EMBMEM4 //STRG_OBJ_FW_RSV5
#define USER_DX_TYPE_EMBMEM_DSP         DX_TYPE_EMBMEM5 //STRG_OBJ_FW_RSV6
#define USER_DX_TYPE_EMBMEM_DSP2        DX_TYPE_EMBMEM6 //STRG_OBJ_FW_RSV7
#define USER_DX_TYPE_EMBMEM_PSTORE      DX_TYPE_EMBMEM7 //STRG_OBJ_PSTORE
#define USER_DX_TYPE_EMBMEM_FAT         DX_TYPE_EMBMEM8 //STRG_OBJ_FAT1

//==============================================================================
//   SYSTEM FUNCTIONS
//==============================================================================
//..............................................................................
// Boot
#define POWERON_PERF                    DISABLE  //enable performance benchmark
#define POWERON_TRACE                   DISABLE  //stop and wait for user enter cmd: "dsc boot"
#define POWERON_FAST_BOOT               DISABLE
#define POWERON_FAST_BOOT_MSG           ENABLE   // disable boot msg for fast boot, but hard to debug
#define POWERON_FAST_CPU2_BOOT          DISABLE
#define POWERON_FAST_RECORD             DISABLE
#define POWERON_FAST_WIFI               DISABLE //NOTE: need to enable POWERON_FAST_CPU2_BOOT too
#define POWERON_WAIT_FS_READY           DISABLE
#define WAITPHOTO_FUNCTION              DISABLE
#define POWERONLOGO_FUNCTION            ENABLE
#define POWEROFFLOGO_FUNCTION           DISABLE
#if ((POWERON_FAST_BOOT == DISABLE) || (POWERON_FAST_RECORD == DISABLE))
#define POWERONSOUND_FUNCTION           ENABLE
#else
#define POWERONSOUND_FUNCTION           DISABLE
#endif
#define POWEROFFSOUND_FUNCTION          DISABLE
#define _LOGO_                          _LOGO_NOVATEK_
#if 1//(defined(_LCDTYPE_ILI9341_IF8B_) || defined(_LCDTYPE_TG078UW006A0_DSI_)||defined(_LCDTYPE_UT35067A0_ILI9488_DSI_))
#define LOGO_DISP_LAYER                 LAYER_VDO1 // VDO2 logo has no rotation
#else
#define LOGO_DISP_LAYER                 LAYER_VDO2
#endif
#define LENSERROFF_FUNCTION             DISABLE

#if (POWERON_PERF == ENABLE)
#define TM_BOOT_ARRAY(n)                static TIMEMARK tmbuf[(n)] = {0}
#define TM_BOOT_INIT(n)                 TM_Init(tmbuf, n)
#define TM_BOOT_BEGIN(a,b)              TM_Begin(a,b)
#define TM_BOOT_END(a,b)                TM_End(a,b)
#else
#define TM_BOOT_ARRAY(n)
#define TM_BOOT_INIT(n)
#define TM_BOOT_BEGIN(a,b)
#define TM_BOOT_END(a,b)
#endif

//#NT#2016/07/12#Niven Cho -begin
//#NT#LINUX_LOAD_BY_UBOOT
//#NT#2016/07/12#Niven Cho -end

//..............................................................................
// External Card Config (for SDIO device)
#if !defined(_CARD1_NONE_)
#define CARD1_FUNC					DISABLE
#else
#define CARD1_FUNC					DISABLE
#endif

#if !defined(_CARD3_NONE_)
#define CARD3_FUNC					DISABLE
#else
#define CARD3_FUNC					DISABLE
#endif

//..............................................................................
// Storage Config
#define FWS_FUNC                        DISABLE  //enable it, if require load fw for start DSP or update fw for eCos

#define PST_FUNC                        ENABLE  //enable it, if require write pstore

#define FS_FUNC                         ENABLE  //enable it, if require write file

//#NT#2016/12/14#Niven Cho -begin
//#NT#EMMC_AS_PSTORE
// only for (_CPU2_TYPE_==_CPU2_LINUX_ && _INTERSTORAGE_ == _INTERSTORAGE_EMMC_)
#define PST_DEV_ID 4 //used for gxstrg to mount linux emmc-pstore partition
#define PST_FS_DRIVE "E:\\"
//#NT#2016/12/14#Niven Cho -end


#if defined(_DSP1_FREERTOS_)
#if (FWS_FUNC != DISABLE)
	#warning Not enable FWS_FUNC for DSP?
	#warning Please enable your "FWS_FUNC" config in header "PrjCfg_XXXX.h"
	#error (see above)
#endif
#endif

//..............................................................................
// Input Config
#define INPUT_FUNC                      ENABLE
//key
#define POWERKEY_FUNCTION               ENABLE //Auto Detect
#define NORMALKEY_FUNCTION              ENABLE //Auto Detect
#define STATUSKEY_FUNCTION              DISABLE //Auto Detect
#define MODEKEY_FUNCTION                DISABLE //Auto Detect

//..............................................................................
// Output Config
#define OUTPUT_FUNC                     DISABLE
//LED
#define LED_FUNCTION                    DISABLE //Auto Detect

//..............................................................................
// Power-Saving Config
#define PWR_FUNC                        ENABLE
//battery level
#define PWRLEVEL_FUNCTION               DISABLE //Auto Detect
//auto power-off when battery level = empty
#define EMPTYPWROFF_FUNCTION            DISABLE
//auto sleep
#define AUTOSLEEP_FUNCTION              ENABLE //Auto Detect
//auto power-off
#define AUTOPWROFF_FUNCTION             ENABLE //Auto Detect

//..............................................................................
// Date-Time Config
#define DEF_YEAR                        2020
#define DEF_MONTH                       1
#define DEF_DAY                         1
#define MIN_YEAR                        2000
#define MAX_YEAR                        2050


//..............................................................................
// Sensor Config (for 1,2,4 devices)


#if 0
#define SENSOR_CAPS_COUNT               2
#define SENSOR_CAPS_MASK                (SENSOR_1 | SENSOR_2)
#else
#define SENSOR_CAPS_COUNT               1
#define SENSOR_CAPS_MASK                SENSOR_1
#endif


#define SENSOR_MAP_1                    SENSOR_ID_1
#define SENSOR_MAP_2                    SENSOR_ID_2


#define SENSOR_GP_FUNC                  DISABLE

// Sensor group
#define SENSOR_GP_1                     SENSOR_MULTI_ID_NONE /*ex: SENSOR_MULTI_ID_1|SENSOR_MULTI_ID_2*/
#define SENSOR_GP_2                     SENSOR_MULTI_ID_NONE /*ex: SENSOR_MULTI_ID_3|SENSOR_MULTI_ID_4*/
// Sensor sub group

#define SENSOR_SUBGP_1                  SENSOR_MULTI_ID_NONE
#define SENSOR_SUBGP_2                  SENSOR_MULTI_ID_NONE



// Sensor Insert Mask
#if 0
#define SENSOR_INSERT_MASK              (SENSOR_2) // support insert sensor
#else
#define SENSOR_INSERT_MASK              (0) // if NOT support insert sensor, please choose this
#endif

#define SENSOR_DEFAULT_ATTACH_MASK      (SENSOR_CAPS_MASK & ~(SENSOR_INSERT_MASK)) // hw default attach sensors

#define SENSOR_INSERT_FUNCTION          ((SENSOR_CAPS_MASK == SENSOR_DEFAULT_ATTACH_MASK)?(DISABLE):(ENABLE)) //Auto Detect (Dynamic attach)

#define SENSOR_DEFAULT_DISPLAY_MASK     (SENSOR_1)


//..............................................................................
// Flashlight Config

#define FLASHLIGHT_FUNCTION             DISABLE //DISABLE


//..............................................................................
// Lens Config
#define LENS_FUNCTION                   DISABLE
#define LENS_FOCAL_LENGTH               2880    // unit:um


//..............................................................................
// Display Config (for LCD, LCD2, TV and HDMI device)

#define LCD2_FUNC_USERSTATUS            DISABLE //use LCD2 as status display. (not for video display)

//NOTE: LCD1 output from DOUT1 only
#if defined(_Disp_VIRTUAL_LCD1_OFF_)
#define LCD1_FUNC                       DISABLE
#define LCD1_MASK                       0x00
#else
#define LCD1_FUNC                       ENABLE
#define LCD1_MASK                       0x01
#endif
//NOTE: TV output from DOUT1 or DOUT2
#if defined(_TV_OFF_)
#define TV_FUNC                         DISABLE
#define TV_MASK                         0x00
#else
#define TV_FUNC                         DISABLE
#define TV_MASK                         0x02
#endif
//NOTE: HDMI output from DOUT1 or DOUT2
#if defined(_HDMI_OFF_)
#define HDMI_FUNC                       DISABLE
#define HDMI_MASK                       0x00
#else
#define HDMI_FUNC                       DISABLE
#define HDMI_MASK                       0x04
#endif

#if((LCD1_FUNC == DISABLE) || (TV_FUNC == DISABLE) || (HDMI_FUNC == DISABLE))
#define DISPLAY_FUNC                    ENABLE
#define DISPLAY2_FUNC                   DISABLE
#define DISPLAY_CAPS_COUNT              1 //1: only single display, 2: allow single display or dual display
#define DISPLAY_CAPS_MASK               (LCD1_MASK|TV_MASK)//|HDMI_MASK|LCD2_MASK
#define DUALDISP_FUNC                   ((DISPLAY_CAPS_COUNT == 2)?DISABLE:DISABLE)

#define DISPLAY_DEFAULT_ATTACH_MASK     DISPLAY_1 // hw default attach display

#if defined(_Disp_VIRTUAL_LCD1_OFF_)
#define DISPLAY_DEFAULT_ATTACH_DEVICE   TV_MASK //LCD1_MASK or HDMI_MASK or TV_MASK
#else
#define DISPLAY_DEFAULT_ATTACH_DEVICE   LCD1_MASK //LCD1_MASK or HDMI_MASK or TV_MASK
#endif
#define DISPLAY2_DEFAULT_ATTACH_DEVICE  0 //LCD2_MASK or TV_MASK or HDMI_MASK

//LCD related
#define LCD_AUTORESET_FUNCTION          DISABLE //Auto Reset - LCD register
#define LCD_AUTOBACKLIGHT_FUNCTION      DISABLE //Auto Adjust Backlight - with AE LV
#define LCD_ROTATE_FUNCTION             DISABLE //Auto Detect - LCD rotate (enable to support LCD body mirror or flip)
//TV related
#define TV_INSERT_FUNCTION              TV_FUNC //Auto Detect - TV cable insert
//HDMI related
#define HDMI_INSERT_FUNCTION            HDMI_FUNC //Auto Detect - HDMI cable insert
//LCD2 related
#define LCD2_INSERT_FUNCTION            LCD2_FUNC //Auto Detect - LCD2 cable insert
#else
#define DISPLAY_FUNC                    ENABLE
#define DISPLAY2_FUNC                   DISABLE
#define DISPLAY_CAPS_COUNT              0
#define DISPLAY_CAPS_MASK               0
#define DUALDISP_FUNC                   0

#define DISPLAY_DEFAULT_ATTACH_MASK     0 // hw default attach display

#define DISPLAY2_DEFAULT_ATTACH_DEVICE  0 //LCD2_MASK or TV_MASK or HDMI_MASK

//LCD related
#define LCD_AUTORESET_FUNCTION          DISABLE //Auto Reset - LCD register
#define LCD_AUTOBACKLIGHT_FUNCTION      DISABLE //Auto Adjust Backlight - with AE LV
#define LCD_ROTATE_FUNCTION             DISABLE //Auto Detect - LCD rotate (enable to support LCD body mirror or flip)
//TV related
#define TV_INSERT_FUNCTION              TV_FUNC //Auto Detect - TV cable insert
//HDMI related
#define HDMI_INSERT_FUNCTION            HDMI_FUNC //Auto Detect - HDMI cable insert
//LCD2 related
#define LCD2_INSERT_FUNCTION            LCD2_FUNC //Auto Detect - LCD2 cable insert
#endif

// Display Config (for UI window, FD window)

// OSD size and format
#define DISPLAY_OSD_W                   320//640
#define DISPLAY_OSD_H                   240
#define DISPLAY_OSD_FMT                 PXLFMT_INDEX8 // 256 colors
//#define DISPLAY_OSD_FMT                 PXLFMT_RGBA5658_PK // 16bpp-colors

//UI tool layout size setting
#define TOOL_LAYOUT_W                   320//640
#define TOOL_LAYOUT_H                   240//480
#define OSD_USE_DOUBLE_BUFFER           ENABLE //use double buffer
#if 1//(defined(_Disp_IF8B_LCD1_ILI9341_))
#define OSD_USE_ROTATE_BUFFER           DISABLE  //use rotate buffer (enable to support LCD with stripe-type subpixel)
#else
#define OSD_USE_ROTATE_BUFFER           DISABLE //use rotate buffer (enable to support LCD with stripe-type subpixel)
#endif

#define OSD2_FUNC						DISABLE
#define OSD2_USE_DOUBLE_BUFFER          DISABLE //use double buffer
#define VDO_BM_FUNC                     DISABLE


// Display Config (for UI background, App quickview, photo frame, ...)

// VDO size and format
#define DISPLAY_VDO_W                   640
#define DISPLAY_VDO_H                   480
#define DISPLAY_VDO_FMT                 DISP_PXLFMT_YUV422_PK

#define VDO_USE_DOUBLE_BUFFER           DISABLE //use double buffer
#define VDO2_USE_DOUBLE_BUFFER          DISABLE
#define VDO_USE_PHOTO_SOURCE            DISABLE  //display photo video streaming
#if 1//(defined(_Disp_IF8B_LCD1_ILI9341_))
#define VDO_ROTATE_DIR                  HD_VIDEO_DIR_ROTATE_0
#define VDO_USE_ROTATE_BUFFER           DISABLE  //use rotate buffer (enable to support LCD with stripe-type subpixel)
#else
#define VDO_ROTATE_DIR                  0
#define VDO_USE_ROTATE_BUFFER           DISABLE //use rotate buffer (enable to support LCD with stripe-type subpixel)
#endif

#define VDO_USE_COPY_BUFFER             DISABLE //use copy buffer to avoid black between UI mode

#define VDO2_USE_ROTATE_BUFFER          DISABLE //use rotate buffer (enable to support LCD with stripe-type subpixel)

#if (DISPLAY2_FUNC == DISABLE)
//#define VDOOUT_MASK     				(DISPLAY_1) // app's video will output to which display
//#define VDOOUT_MASK     				(DISPLAY_2) // app's video will output to which display
#define VDOOUT_MASK     				(DISPLAY_1|DISPLAY_2) // app's video will output to which display
#else
#define VDOOUT_MASK     				(DISPLAY_1) // app's video will output to which display
#endif

#define LCD_PREVIEW_VDO_FMT             GX_IMAGE_PIXEL_FMT_YUV420_PACKED
#define LCD2_PREVIEW_VDO_FMT            GX_IMAGE_PIXEL_FMT_YUV420_PACKED
#define TV_PREVIEW_VDO_FMT              GX_IMAGE_PIXEL_FMT_YUV420_PACKED
#define HDMI_PREVIEW_VDO_FMT            GX_IMAGE_PIXEL_FMT_YUV420_PACKED

// Display Config (for DispSrv; Photo App, Movie App and Play App)
// VDO1 size and format (for LCD device)
//LCD1:
#if defined(_Disp_VIRTUAL_LCD1_OFF_)
#define LCDMODE                         DISABLE
#elif defined(_Disp_IF8B_LCD1_AUCN01_)
#define LCDMODE                         DISP_LCDMODE_RGBD320 //DISP_LCDMODE_YUV640
#elif defined(_Disp_IF8B_LCD1_PW35P00_HX8238D_) || \
      defined(_Disp_IF8B_LCD1_ILI9341_)
#define LCDMODE                         0//DISP_LCDMODE_RGB_SERIAL
#else
#error "Unknown _LCD_"
#endif

#define LCD2MODE                        DISABLE

// VDO1 size and format (for TV device)
//TV: DISP_TVMODE_PAL mode
#define TVMODE                          DISP_TVMODE_NTSC

// VDO1 size and format (for HDMI device)
//HDMI: DISP_HDMIMODE_1920X1080I60 mode
//#define HDMIMODE                        DISP_HDMIMODE_1920X1080I60
#define HDMIMODE                        DISP_HDMIMODE_MAX


#define DUALCAM_PIP_BEHIND_FLIP         DISABLE  //Flip 2nd cam image in PIP view. Need extra 1/2 VDO buffer



//..............................................................................
// Audio Config
#define AUDIO_FUNC						ENABLE
//..............................................................................
// External Audio Config (for LineOut device)

//LINE related

//..............................................................................
// External Storage Config
#define SDINSERT_FUNCTION               ENABLE //Auto Detect
#define SDHOTPLUG_FUNCTION              DISABLE
//..............................................................................
// FILEDB Config
#define USE_FILEDB                      ENABLE //DISABLE
#define FILEDB_CARDV_ROOT               "Novatek"   // the max length should not exceeds 16
// DCF Config
#define USE_DCF                         DISABLE
#define DCF_SUPPORT_FORMAT              (DCF_FILE_TYPE_JPG|DCF_FILE_TYPE_AVI|DCF_FILE_TYPE_MOV|DCF_FILE_TYPE_MP4)
//..............................................................................
// EXIF Config
#define USE_EXIF                        DISABLE
// End string must be '\0', total char must be 31
#define EXIF_MAKER_NAME                 "DvCam"
// End string must be '\0', total char must be 31
#define EXIF_MODEL_NAME                 "96675"
// End string must be '\0', total char must be 31
#define EXIF_IMAGE_DESCROPTOIN          "NOVATEK CAMERA"

#define EXIF_SOFTWARE_VERSION           FW_VERSION_NUM    // <= 20 char (TagSoftVerLen)

//..............................................................................
// Movie Maker/Model name Config
// End string must be '\0', total char must plus 1
#define MOVAPP_COMP_MANU_STRING         "NOVATEK"
// End string must be '\0', total char must plus 1
#define MOVAPP_COMP_NAME_STRING         "DEMO1"
//..............................................................................
// Usb Config

#define USBINSERT_FUNCTION              ENABLE //Auto Detect

#define USB_VID                         0x0603
#define USB_PID_PCCAM                   0x8612 // not support pc cam
#define USB_PID_WRITE                   0x8614
#define USB_PID_PRINT                   0x8613
#define USB_PID_MSDC                    0x8611

#define USB_PRODUCT_REVISION            '1', '.', '0', '0'
#define USB_VENDER_DESC_STRING          'N', 0x00,'O', 0x00,'V', 0x00,'A', 0x00,'T', 0x00,'E', 0x00,'K', 0x00, 0x20, 0x00,0x00, 0x00 // NULL
#define USB_VENDER_DESC_STRING_LEN      0x09
#define USB_PRODUCT_DESC_STRING         'D', 0x00,'E', 0x00,'M', 0x00,'O', 0x00,'1', 0x00, 0x20, 0x00, 0x00, 0x00 // NULL
#define USB_PRODUCT_DESC_STRING_LEN     0x07
#define USB_PRODUCT_STRING              'N','v','t','-','D','S','C'
#define USB_SIDC_DESC_STRING            'D', 0x00,'E', 0x00,'M', 0x00,'O', 0x00,'1', 0x00, 0x20, 0x00, 0x00, 0x00 // NULL
#define USB_SIDC_DESC_STRING_LEN        0x07
#define USB_MTP_FRIENDNAME_STRING       'D','E','M','O','1',0x00  //maximum length is 253 bytes
#define USB_MTP_FRIENDNAME_STRING_LEN   0x06                      //maximum length is 253 bytes
#define USB_VENDER_STRING               'N','O','V','A','T','E','K'
#define USB_VENDER_SIDC_DESC_STRING     'N', 0x00,'O', 0x00,'V', 0x00,'A', 0x00,'T', 0x00,'E', 0x00,'K', 0x00, 0x20, 0x00,0x00, 0x00 // NULL
#define USB_VENDER_SIDC_DESC_STRING_LEN 0x09
#define USB_SERIAL_NUM_STRING           '5', '1', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '1', '0', '0'

//..............................................................................
// Other Config

#define GSENSOR_FUNCTION                DISABLE  //G-sensor
#define GPS_FUNCTION                    DISABLE

#define SHOW_ADC_ON_SCREEN              DISABLE
#define CALIBRATION_FUNC                DISABLE  // Enable/Disable Calibration
#define WATERLOGO_FUNCTION              DISABLE  // Enable/Disable waterlogo function

// Important Notice:
//    If enable MULTILINGUAL_FUNCTION, remember to update font table and make sure the font height is not exceed the buffer size!!
#define MULTILINGUAL_FUNCTION           DISABLE  // Enable/Disable Multi-Lingual movie stamp function

#define TIMELAPSE_LPR_FUNCTION          DISABLE   // Enable/Disable Timelapse Low Power Record function

#define MOVIE_MULTISTAMP_FUNC           DISABLE
//---------------------------------------------------------------------------
// For Video Debug Info
//---------------------------------------------------------------------------
#define _MOVIE_IQLOG_                   DISABLE     // Enable/Disable Video IQLOG, Spark Chou

//==============================================================================
//   PLAYBACK FUNCTIONS
//==============================================================================
#define SLIDESHOW_MUSIC_OPTION          DISABLE //longer no maintain
// copy card to card function
#define COPYCARD2CARD_FUNCTION          DISABLE

//==============================================================================
//   PHOTO FUNCTIONS
//==============================================================================

#define ASSIST_BEAM_FUNC                DISABLE
#define GDC_FUNC                        DISABLE
#define AF_BEAM_EV_THRESHOLD            540
#define _FD_FUNC_                       DISABLE
#define _TSR_FUNC_                      DISABLE
#define MAX_FDNUM                       10
#define PIP_VIEW_FUNC                   DISABLE // PipView (DISABLE: display 2 sensor by PipView, DISABLE: display user selected sensor)
#define PIP_VIEW_FASTSWITCH             DISABLE // display single sensor method (DISABLE: always display by Pipview, DISABLE: display by direct path)
#define PIP_VIEW_LR                     DISABLE //(DISABLE: display 2 sensor by PipView in left, right view, side by side, DISABLE: normal pipview)

#define SBS_VIEW_FUNC                   DISABLE // SbsView (DISABLE: display 2 sensor by SbsView, DISABLE: display user selected sensor)
#define SHDR_FUNC                       DISABLE // DISABLE
#define WDR_FUNC                        ENABLE
#define DEFOG_FUNC                      DISABLE // DISABLE

#if !defined(_Gyro_None_)
#define RSC_FUNC                        DISABLE
#else
#define RSC_FUNC                        DISABLE
#endif
//..............................................................................
#define _SCREENNAIL_SIZE_               _SCREENNAIL_SIZE_VGA_
#define _QUICKVIEW_SIZE_                _QUICKVIEW_SIZE_SUIT_FOR_DISP

//..............................................................................
// Photo Frame Pstore Version Config
#define PF_PSTORE_VER                   0x00000105

//==============================================================================
//   MOVIE FUNCTIONS
//==============================================================================
#if !defined(_Gyro_None_)
#define MOVIE_RSC                       DISABLE
#else
#define MOVIE_RSC                       DISABLE
#endif
#define MOVIE_PIM_CAPTURE               DISABLE // picture in movie method 1 (capture full resolution image)
#define MOVIE_GDC_FUNC                  DISABLE
#define MOVIE_SMEAR_R_FUNC              DISABLE
#define MOVIE_ENSURE_SD_CLASS4          DISABLE
#define MOVIE_ENSURE_SD_32KCLUSTER      DISABLE
#define MOVIE_MULTI_RECORD_FUNC         DISABLE
#define MOVIE_REC_YUVMERGE              DISABLE // slowly timelapse record mode, frame period >= 1S
#define MOVIE_AUTOREC_ACPLUG            DISABLE
#define MOVIE_NET_USE_STREAM_1          DISABLE
#define MOVIE_UVAC_FUNC     			DISABLE
#define MOVIE_DSP_FUNC					DISABLE

#define MOVIE_MAPPING_MULTIREC			DISABLE
#define MOVIE_DIRECT_FUNC               ENABLE  // direct mode function
#define MOVIE_EIS                       ENABLE
#define MOVIE_EIS_GYRO_LOG              ENABLE
#define MOVIE_YUV_COMPRESS              ENABLE

#define MOVIE_DIS                       DISABLE
#define MOVIE_FD_FUNC_                  DISABLE
#define MOVIE_FD_DRAW_VIDEO             DISABLE
#define MOVIE_BC_FUNC                   DISABLE
#define MOVIE_TD_FUNC                   DISABLE // Depend on MOVIE_BC_FUNC
#define MOVIE_TD_DRAW_VIDEO             DISABLE
#define MOVIE_TWD_FUNC                  DISABLE // Depend on MOVIE_BC_FUNC
#define MOVIE_TWD_DRAW_VIDEO            DISABLE
#define MOVIE_TZD_FUNC                  DISABLE // Depend on MOVIE_BC_FUNC
#define MOVIE_TZD_DRAW_VIDEO            DISABLE
#define MOVIE_ODT_FUNC                  DISABLE
#define MOVIE_ODT_DRAW_VIDEO            DISABLE

#define UCTRL_APP_MOVIE_FEATURE_SETGET  DISABLE
#define MEDIA_VASTREAM                  DISABLE
#define IR_CUT_FUNC                     DISABLE
#define LOGFILE_FUNC                    DISABLE
#define _ADAS_FUNC_                     DISABLE
//..............................................................................
// ADAS subsetting (The following sub-functions are available only if _ADAS_FUNC_ = DISABLE
#define _AUTOVP_FUNC_                   DISABLE
#define _SNG_FUNC_                      DISABLE
#define _DRAW_LDFCINFO_ON_OSD_          DISABLE
#define _DRAW_SNG_ROI_                  DISABLE
#define _DRAW_LDWSINFO_ON_VIDEO_        DISABLE     // This function is only valid if data path is D2D!!

#define MOVIE_TSR_FUNC_                 DISABLE
#define MOVIE_TSR_DRAW_VIDEO            DISABLE
#define TSR_MODEL_IN_PSTORE             DISABLE

#define CROP_SOURCE_W                   3840
#define CROP_SOURCE_H                   2160


//==============================================================================
//   MOVIE PLAY FUNCTIONS
//==============================================================================
#define MOVIE_PLAY_VIEW_FUNC            DISABLE // MoviePlayView


//==============================================================================
//   USBCAM FUNCTIONS
//==============================================================================
#define UVAC_MODE_2_PATH                DISABLE
#if (UVAC_MODE_2_PATH==DISABLE)
#undef MOVIE_MULTI_RECORD_FUNC
#define MOVIE_MULTI_RECORD_FUNC         DISABLE
#endif
//==============================================================================
//   USBCHARGE FUNCTIONS
//==============================================================================
#define USB_CHARGE_FUNCTION             DISABLE
#define USB_CHARGE_VERIFY               DISABLE

//==============================================================================
//   USBDISK FUNCTIONS
//==============================================================================

//==============================================================================
//   MSDCVENDOR_NVT, is required for PC tools to transmit commands and data via USB / NET
//==============================================================================
#define MSDCVENDOR_NVT                  DISABLE  //via USB, depend on USBINSERT_FUNCTION and _USB3_INT_CORE1_
#define MSDCVENDOR_UPDFW                DISABLE //enable it will cause disalbing MSDC and MTP/PTP/UAVC Mode, MSDCVENDOR_NVT must enable
#define MSDCVENDOR_IDENTITY_FILE        "A:\\MSDCNVT"
#if (defined(_MODEL_DVCAM1_EVB_) || defined(_MODEL_DVCAM2_EVB_))
#define MSDCVENDOR_NVT_EXAM             DISABLE  //only for novatek internal use
#else
#define MSDCVENDOR_NVT_EXAM             DISABLE //for customer, disable it to reduce memory
#endif
//==============================================================================
//   USBPRINT FUNCTIONS
//==============================================================================

//==============================================================================
//   USBPRINT FUNCTIONS
//==============================================================================

//==============================================================================
//   UI FUNCTIONS
//==============================================================================
//..............................................................................
// UI Resource Storage Config
#define _LANG_STORE_                    _INSIDECODE_
#define _JPG_STORE_                     _INSIDECODE_
#define SPECIAL_KEY_RESET_FUNC          DISABLE
#define EXTRA_MENU                      DISABLE

//==============================================================================
//   IPCam FUNCTION
//==============================================================================
//..............................................................................

///////FD param
#define IPCAM_FD_FUNC                   DISABLE
#define MAX_FDNUM                       10        //set face detection max face number
//////MD param
#define IPCAM_MD_FUNC                   DISABLE
//////VQA param
#define IPCAM_VQA_FUNC                  DISABLE

/////AUDIO param
#define IPCAM_AUDIO_IN_FUNC             DISABLE
#define IPCAM_AUDIO_OUT_FUNC            DISABLE
#define IPCAM_AEC_FUNC  				DISABLE
#define AUDIO_FUNC_AAC                  DISABLE


//==============================================================================
//   NET FUNCTIONS
//==============================================================================
#if (!defined(_NVT_SDIO_WIFI_NONE_) || !defined(_NVT_USB_WIFI_NONE_))
#define _NETWORK_DEVICE_
#define _NETWORK_ON_WIFI_
#endif
#if !defined(_NVT_USB_4G_NONE_)
#define _NETWORK_DEVICE_
#define _NETWORK_ON_4G_
#endif
#if !defined(_NVT_ETHERNET_NONE_)
#define _NETWORK_DEVICE_
#define _NETWORK_ON_ETH_
#endif

#define MAC_APPEN_SSID                  ENABLE
#define ACL_TIME                        15000 //minmum 15 sec
#define WIFI_APP_MANUFACTURER           "NVT"
#define WIFI_APP_MODLE                  "CarDV_WiFi"
#define ONVIF_PROFILE_S                 DISABLE  //Enable it to cause NO CARD DETECTED.
#define ECOS_POWER_OFF_FLOW             DISABLE

//#NT#2016/10/21#YongChang Qui -begin
//#NT#wifi modules on EVB can't be powered off by default
#define WIFI_POWER_OFF_FLOW             DISABLE //Enabled only if ECOS_POWER_OFF_FLOW == DISABLE and DrvWiFi_PowerOff() actually powers off wifi module
//#NT#2016/10/21#YongChang Qui -end

#define HFS_FUNC                        ENABLE

#define HTTP_LIVEVIEW_FUNC              DISABLE
#if (WIFI_FUNC==DISABLE)
#define WIFI_AP_FUNC                    DISABLE
#else
#define WIFI_AP_FUNC                    ENABLE
#endif
#define WIFI_FTP_FUNC                   DISABLE

#define HTTP_MJPG_W_4                   640
#define HTTP_MJPG_H_3                   480

#define HTTP_MJPG_W_16                  640
#define HTTP_MJPG_H_9                   360

#define AUTO_WIFI                       DISABLE

#define HTTP_LIVEVIEW                   1
#define RTSP_LIVEVIEW                   2
#define MOVIE_LIVEVIEW                  RTSP_LIVEVIEW

#if(WIFI_AP_FUNC==DISABLE)
#undef HTTP_LIVEVIEW_FUNC
#define HTTP_LIVEVIEW_FUNC              DISABLE
#define MOVIE_MULTI_RECORD_FUNC         DISABLE


#undef HTTP_MJPG_W_4
#undef HTTP_MJPG_H_3
#undef HTTP_MJPG_W_16
#undef HTTP_MJPG_H_9
#define HTTP_MJPG_W_4                   640
#define HTTP_MJPG_H_3                   480
#define HTTP_MJPG_W_16                  640
#define HTTP_MJPG_H_9                   360

#undef HFS_FUNC
#define HFS_FUNC                        DISABLE
#endif

#define WIFI_APP_MANUFACTURER           "NVT"
#define WIFI_APP_MODLE                  "CarDV_WiFi"
#define WIFI_APP_DOWNLOAD_URL           "http://115.29.201.46:8020/download/filedesc.xml"


#if defined(_CPU2_LINUX_)
#define UCTRL_FUNC                      DISABLE
#else
#define UCTRL_FUNC                      DISABLE
#endif
#define ONVIF_FUNC                      DISABLE  //CarDV-Linux is no using of ONVIF


#define WIFI_FINALCAM_APP_STYLE         DISABLE
#define WIFI_BG_COLOR                   14

#define WIFI_STA_RECONNECT_MAX_COUNT 5
#define WIFI_STA_RECONNECT_INTERVAL 300

#define YOUKU_SDK                       DISABLE
#define IOT_P2P_FUNC                    DISABLE
#define NETWORK_4G_MODULE               DISABLE

#if (YOUKU_SDK == DISABLE)
#define WIFI_MOVIE_FOLDER               "MOVIE_WIFI"
#define CUT_MOVIE_FOLDER                "MOVIE_CUT"
//the following two functions should be exclusive
#define SAVE_DUAL_STREAM_FUNC           DISABLE
#define CUT_MOVIE_FUNC                  DISABLE
#endif

#define MOVIE_OT_FUNC                   DISABLE
#define MOVIE_OT_DRAW_VIDEO             DISABLE



#define CURL_FUNC                       DISABLE

#define _DDD_FUNC_                      DISABLE
#define MOVIE_DDD_FUNC                  DISABLE
#define MOVIE_DDD_FD_FUNC               DISABLE
#define MOVIE_DDD_DRAW_VIDEO            DISABLE
#define MOVIE_DDD_SAVE_LOG              DISABLE
#define DDD_MODEL_IN_PSTORE             DISABLE

#define MOVIE_FTG_FUNC                  DISABLE
#define MOVIE_FTG_DRAW_VIDEO            DISABLE

//#NT#2017/04/28#KCHong -begin
//#NT# CNN Object detection
#define _OD_FUNC_                       DISABLE
//#NT#2017/04/28#KCHong -end

#define USERLOG_FUNC                    DISABLE


// fileSize align function
#define FILESIZE_ALIGN_FUNC             DISABLE
#define PHOTO_FILESIZE_ALIGN_VALUE      2*1024*1024   //  2MB
#define FS_ALIGN_RESERVED_SIZE          (2 * MOVIE_FILESIZE_ALIGN_VALUE)

#define FS_SWITCH_STRG_FUNC             DISABLE

// mount sd-2 as "B:\\"
// To mount sd-2 as "B:\\" please do the following steps
// 1. set .int_id_dst[CC_CORE_MIPS1].Bit.bInt_ID_SDIO2 to TRUE
// 2. enable FS_DX_TYPE_DRIVE_B as DX_TYPE_CARD2
// 3. enable FS_MULTI_STRG_FUNC
// 4. NETWORK = NETWORK_NONE (Both uITRON and eCos)
#define FS_MULTI_STRG_FUNC              DISABLE
//MULTI_DRIVE
#if defined(_EMBMEM_EMMC_) && (FS_MULTI_STRG_FUNC==DISABLE)
#define FS_DX_TYPE_DRIVE_A              USER_DX_TYPE_EMBMEM_FAT
#else
#define FS_DX_TYPE_DRIVE_A              DX_TYPE_CARD1
#endif
#define FS_DX_TYPE_DRIVE_B              USER_DX_TYPE_EMBMEM_FAT //FS_MULTI_STRG_FUNC must enable for FS_DX_TYPE_DRIVE_B

#define _3DNROUT_FUNC                   DISABLE

#define RTC_FUNC                        DISABLE

#define DISPLAY_FAST_SWITCH DISABLE //fast swicth display device
#define DISPLAY_OSD2_FMT DISP_PXLFMT_INDEX8 // 256 colors
#define SPK_MASK 0x01
#define LOUT_MASK 0x02
#define LINE_INSERT_FUNCTION DISABLE //Auto Detect - LINE cable insert (audio out)
#define USE_DPOF DISABLE
#define BT_FUNC DISABLE
#define ECOS_DBG_MSG_FORCE_UART1_DIRECT DISABLE
#define USOCKET_CLIENT DISABLE
#define MOVIE_DISDSP_FUNC DISABLE
#define WIFI_UI_VER_1_0 0
#define WIFI_UI_VER_2_0 1
#define WIFI_UI_FLOW_VER WIFI_UI_VER_1_0
#define LOG_DBGINFO_IN_GPS_SECTION DISABLE
#define MOVIE_TD_SAVE_LOG DISABLE
#define MOVIE_SCD_FUNC DISABLE
#define UVC_MULTIMEDIA_FUNC DISABLE
#if (UVC_MULTIMEDIA_FUNC == DISABLE)
#undef MOVIE_UVAC_FUNC
#define MOVIE_UVAC_FUNC					DISABLE // it needs to enable MOVIE_UVAC_FUNC if UVC_MULTIMEDIA_FUNC was enabled
#endif
#define FORCE_UNPLUG_RTSP DISABLE
#define IQTOOL_FUNC DISABLE
#define GXGFX_ADVANCED_STRING_FUNC DISABLE
#define SMALL_CLONE_MOVIE DISABLE
#define CUSTOM_NAMING_RULE DISABLE
#define LINUX_CVHOST_FUNC DISABLE
#define AUDIO_PLAY_TEST DISABLE
#define FTG_SNAPSHOT_FUN DISABLE
#define FTG_MAX_SNAPSHOT_NUM 5
#define FTG_OVERRALL_MAX_BUF_SIZE 250*1024
#define FTG_FACE_MAX_BUF_SIZE 100*1024
#define FTG_MAX_SNAPSHOT_SIZE 1024*1024*25
#define FTG_MAX_FACE_PIC_NUM 3
#define RTSP_AUDIO_ENABLE DISABLE
#define AUDIO_CMD_UCTRL_ENABLE DISABLE
#define FACE_GRADE_TEST DISABLE
#define IPCAM_UVC_FUNC DISABLE
#define BOOT_RESET_MENU_INFO			ENABLE
#define VIDEOSPRITEIPC DISABLE
#define IPCAM_FD_FUNC DISABLE
//#define IPCAM_DIS_FUNC DISABLE
#define IPCAM_DIS_MV_FUNC			DISABLE //Digital Image Stabilzier: calculate motion vector
#define IPCAM_DIS_COMP_FUNC			DISABLE //DIS compensation: if set IPCAM_DIS_COMP_FUNC = enable, it should set IPCAM_DIS_MV_FUNC = TRUE, too
#define HOT_PLUG_REMAIN_SIZE 0
#define _CVHOST_EX_                     DISABLE
#define _CVHOST_EX_ENDSP_               DISABLE
#define _NVTCV_EX_                      DISABLE
#define _NVTCV_EX_ENDSP_                DISABLE
#define MOVIE_MAX_CHANNEL				16
#define LINUX_MSDC DISABLE //DISABLE
#define STEREO_FUNC                     DISABLE
#define THUMB_STAMP_FUNC				DISABLE
#define STAMP_PRIVATE_POOL				ENABLE
#define VIDEOSPRITE_STAMP_FUNC ENABLE
#define VDOENC_MULTI_TEMPORARY_LAYER 	DISABLE 	  	//Multi Temporary Layer
#define YUV_TV_RANGE 				    DISABLE  	//YUV color space in tv range
#define VIDEO_FUNC_MJPG            		DISABLE
#define VIDEO_FUNC_H264            		DISABLE
#define VIDEO_FUNC_H265            		DISABLE
#define STOP_REC_BK  DISABLE
#define NMEDIAPLAY_FUNC                 DISABLE     // switch SMediaPlay and NMediaPlay flow, use NMediaPlay when DISABLE to be set.
#define USER_PACK_UPDATE				DISABLE
#define WAV_PLAY_FUNC					ENABLE
#define MOVIE_IME_CROP					ENABLE
#define PLAY_FULL_DISP					DISABLE
#define FSCK_FUNC                       ENABLE    //fsck is for disk checking (only FAT format)
#define GDC_POWER_DOWN               DISABLE
#define LIVESTREAM_FUNC                 ENABLE
#if (LIVESTREAM_FUNC == ENABLE)
#define _QRCODE_FUNC_                  ENABLE
#else
#define _QRCODE_FUNC_                  DISABLE
#endif

/*******************************************************************************************
 * LVGL UI Style config
 ******************************************************************************************/
#if defined(_UI_STYLE_LVGL_)

#ifdef DISPLAY_OSD_W
#undef DISPLAY_OSD_W
#endif

#ifdef DISPLAY_OSD_H
#undef DISPLAY_OSD_H
#endif

#ifdef DISPLAY_OSD_FMT
#undef DISPLAY_OSD_FMT
#endif

#include "lv_conf.h"

/*******************************************************************************************
 * OSD res is determined by lvgl library (LV_HOR_RES_MAX / LV_VER_RES_MAX in the lv_conf.h)
 ******************************************************************************************/
#define DISPLAY_OSD_W_PADDING           0
#define DISPLAY_OSD_H_PADDING           0
#define DISPLAY_OSD_W_OFFSET            (DISPLAY_OSD_W_PADDING / 2)
#define DISPLAY_OSD_H_OFFSET            (DISPLAY_OSD_H_PADDING / 2)
#define DISPLAY_OSD_W                   (LV_HOR_RES_MAX + DISPLAY_OSD_W_PADDING)
#define DISPLAY_OSD_H                   (LV_VER_RES_MAX + DISPLAY_OSD_H_PADDING)

#if (DISPLAY_OSD_W_OFFSET > DISPLAY_OSD_W_PADDING) || (DISPLAY_OSD_H_OFFSET > DISPLAY_OSD_H_PADDING)
#error "DISPLAY_OSD_OFFSET must be smaller than DISPLAY_OSD_PADDING !"
#endif

/*******************************************************************************************
 * OSD format is determined by lvgl library (LV_COLOR_DEPTH in the lv_conf.h)
 ******************************************************************************************/
#if LV_COLOR_DEPTH == 8
	#define DISPLAY_OSD_FMT                 PXLFMT_INDEX8
	#define DISPLAY_HDAL_OSD_FMT            HD_VIDEO_PXLFMT_I8
#elif LV_COLOR_DEPTH == 24
	#define DISPLAY_OSD_FMT                 PXLFMT_RGBA5658_PK
	#define DISPLAY_HDAL_OSD_FMT            HD_VIDEO_PXLFMT_ARGB8565
#elif LV_COLOR_DEPTH == 32
	#define DISPLAY_OSD_FMT                 PXLFMT_RGBA8888_PK
	#define DISPLAY_HDAL_OSD_FMT            HD_VIDEO_PXLFMT_ARGB8888
#else
	#error "LV_COLOR_DEPTH in the lv_conf.h is currently not supported"
#endif

#define OSD_SCREEN_SIZE        (DISPLAY_OSD_W * DISPLAY_OSD_H * HD_VIDEO_PXLFMT_BPP(DISPLAY_HDAL_OSD_FMT) / 8)
#define UI_LAYOUT_SIZE         (LV_HOR_RES_MAX * LV_VER_RES_MAX * HD_VIDEO_PXLFMT_BPP(DISPLAY_HDAL_OSD_FMT) / 8)

/*******************************************************************************************
 * Movie stamp user config
 ******************************************************************************************/
#define LV_USER_CFG_STAMP_FONT_ID_XXL			LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_128_1BPP
#define LV_USER_CFG_STAMP_FONT_ID_XL			LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_64_1BPP
#define LV_USER_CFG_STAMP_FONT_ID_LARGE 		LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_48_1BPP
#define LV_USER_CFG_STAMP_FONT_ID_MEDIUM		LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_32_1BPP
#define LV_USER_CFG_STAMP_FONT_ID_SMALL			LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP

#define LV_USER_CFG_STAMP_LETTER_SPACE			0
#define	LV_USER_CFG_STAMP_TEXT_ALIGN			LV_ALIGN_CENTER
#define	LV_USER_CFG_STAMP_RADIUS				5
#define	LV_USER_CFG_STAMP_BORDER_WIDTH			3
#define	LV_USER_CFG_STAMP_EXT_WIDTH				10
#define	LV_USER_CFG_STAMP_EXT_HEIGHT			10

/* please always fill ARGB8888, alpha channel will be discarded if output format is yuv (Photo UIDateImprint) */
#define LV_USER_CFG_STAMP_COLOR_TEXT			0xFFFFFFFF	/* text color */
#define LV_USER_CFG_STAMP_COLOR_BACKGROUND		0XFF000088	/* background color */
#define LV_USER_CFG_STAMP_COLOR_FRAME			0xFF880088	/* frame color */

/*******************************************************************************************
 * Enable LVGL rotate depend on VDO_USE_ROTATE_BUFFER (LV_USE_GPU_NVT_DMA2D in the lv_conf.h should be enabled)
 ******************************************************************************************/
#if VDO_USE_ROTATE_BUFFER == ENABLE
	#if LV_USE_GPU_NVT_DMA2D
		#define LV_USER_CFG_USE_ROTATE_BUFFER 1
	#else
		#error "LV_USE_GPU_NVT_DMA2D in the lv_conf.h must be enabled when VDO_USE_ROTATE_BUFFER = ENABLE"
	#endif
#else
	#define LV_USER_CFG_USE_ROTATE_BUFFER 0
#endif

#define LV_USER_CFG_USE_TWO_BUFFER	0

#endif

#define ETH_POWER_DOWN ENABLE

#endif //_PRJCFG_DVCAM_H_
