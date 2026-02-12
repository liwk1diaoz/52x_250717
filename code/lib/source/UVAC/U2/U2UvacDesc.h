/*
    Copyright   Novatek Microelectronics Corp. 2006.  All rights reserved.

    @file       UvacDesc.h
    @ingroup    mIUSB

    @brief      The header file for USB Video Class descriptor.

    @note       Nothing.
*/

#ifndef _U2UVACDESC_H
#define _U2UVACDESC_H

#include    "U2UvacVideoTsk.h"
#include    "UVAC.h"




//new
#define UVAC_IAD_LENGTH                                 0x08
#define UVAC_IAD_STR_INDEX                              0           // iFunction
#define UVAC_AUD_TYPE_FMT_I_BASE_LEN                    8
#define UVAC_AUD_TYPE_FMT_II_BASE_LEN                   9
#define UVAC_AUD_TYPE_FMT_III_BASE_LEN                  8
#define UVAC_VID_EP_PACK_SIZE_MAX_CNT                   3
#define UVAC_IF_WINUSB_EP_CNT                           1
#define UVAC_IF_VC_EP_CNT                               1           //fail to set 0 interrupt
#define UVAC_IF_AC_EP_CNT                               0
#define UVAC_IF_VS_EP_CNT                               1
#define UVAC_IF_AS_EP_CNT                               1
#define UVAC_WINUSB_IF_CLASS_CODE                       0xFF        // bInterface class
#define UVAC_INTF_IDX_UNKNOWN                           0xFF
#define UVAC_IF_VS_CNT                                  1
#define UVAC_IF_AS_CNT                                  1
#define UVAC_VS_COLOR_LENGTH                            0x6

// Maximum number of bits pre second kbits/sec
#define UVAC_AUD_MAX_BPS_16K                 0x200    //16K16Bit2Channel=512k
#define UVAC_AUD_MAX_BPS_32K                 0x400    //32K16Bit2Channel=1024k
#define UVAC_AUD_MAX_BPS_48K                 0x600    //48K16Bit2Channel=1536k
#define UVAC_AUD_SAMPLES_PER_FRAME           0x400    //1024 //// Number of PCM audio samples in one encoded audio frame
#define UVAC_AUD_EP_CLS_SPEC_ATTR            0x01     //sampling frequency
#define UVAC_SAMPLES_PER_FRAME               0x400    //1024
#define UVAC_MPEG_FMT_LENGTH                 0x08
#define UAC_DATA_FMT_TAG                    UAC_DATA_FMT_TYPE_II_MPEG //UAC_DATA_FMT_TYPE_MPEG2EXT //UAC_DATA_FMT_TYPE_AC3  //UAC_DATA_FMT_TYPE_MPEG
#define UAC_MPEG_CAP                        0x0040   //MPEG 2/D6 //0x00
#define UAC_MPEG_FEATURE                    0x00


#define UVAC_FIRST_IF_IDX                               0
#define UVAC_CONFIG_DESC_LEN_OFS                        2
#define UVAC_IF_VC_BASE_LEN                             0x0C
#define UVAC_INTR_EP_MAX_PACKET_SIZE                    64
#define UVC_VS_HW_TRIGGER_DISABLE                       0x00        // HW trigger supported
#define UVC_VS_HW_TRIGGER_ENABLE                        0x00        // HW trigger supported
#define UVC_VS_HW_TRIGGER_USAGE_INIT                    0x00        // HW trigger usage
#define UVC_VS_HW_TRIGGER_USAGE_GENARAL                 0x00        // HW trigger usage
#define UVC_VS_FRM_VARIABLE_SIZE                        0x01
#define UVC_VSFRM_FIRST_INDEX                           0x01        // first frame index
#define UVC_VSFRM_CAP_FIX_FRMRATE                       0x02        // Fix frame rate
#define UVC_VSFRM_CAP_NONE                              0x00        // none
#define USB_UVC_VS_FRAME_LENGTH                         0x26
//#define UVC_HS_BULKIN_BLKSIZE                           0x0200

typedef enum {
	UVC_TERMINAL_ID_NULL = 0,      //no terminal
	UVC_TERMINAL_ID_ITT,           //camera-input terminal
	UVC_TERMINAL_ID_OT,            //output terminal
	UVC_TERMINAL_ID_SEL_UNIT,      //selector unit
	UVC_TERMINAL_ID_PROC_UNIT,     //processing unit
	UVC_TERMINAL_ID_EXTE_UNIT,     //extension unit
} UVC_TERMINAL_ID;

typedef enum {
	UAC_TERMINAL_ID_NULL = 0,      //no terminal
	UAC_TERMINAL_ID_IT,            //input terminal
	UAC_TERMINAL_ID_OT,            //output terminal
	UAC_TERMINAL_ID_FU,            //feature unit
} UAC_TERMINAL_ID;


#if 0
#define UAC_TYPE_III                        DISABLE // ENABLE   //
#define UAC_TYPE_II                         DISABLE //ENABLE   //
#if UAC_TYPE_III
#define USB_UAC_TYPE_III_FMT_LENGTH         0x0B     //8+1*3
#define USB_UAC_TYPE_III_CH                 0x02     //must set to be 2
#define USB_UAC_TYPE_III_SUBFRM             0x02     //must set to be 2
#define USB_UAC_TYPE_III_SUBFRM_BITRESO     0x08     //bit-resolution of a subframe
#define USB_UAC_TYPE_X_FMT_LENGTH           USB_UAC_TYPE_III_FMT_LENGTH
#define USB_UAC_MPEG_FMT_LENGTH             0
#define UAC_DATA_FMT_TAG                    UAC_DATA_FMT_TYPE_III_MPEG2_NOEXT //UAC_DATA_FMT_TYPE_MPEG2EXT //UAC_DATA_FMT_TYPE_AC3  //UAC_DATA_FMT_TYPE_MPEG

#elif UAC_TYPE_II
#define USB_UAC_TYPE_III_FMT_LENGTH         0x0B     //8+1*3
#define USB_UAC_TYPE_III_CH                 0x02     //must set to be 2
#define USB_UAC_TYPE_III_SUBFRM             0x02     //must set to be 2
#define USB_UAC_TYPE_III_SUBFRM_BITRESO     0x08     //bit-resolution of a subframe

#define USB_UAC_TYPE_II_FMT_LENGTH          0x0C     //9+1*3
#define USB_UAC_TYPE_X_FMT_LENGTH           USB_UAC_TYPE_II_FMT_LENGTH
// Maximum number of bits pre second kbits/sec
#define USB_UAC_MAX_BPS_16K                 0x200    //16K16Bit2Channel=512k
#define USB_UAC_MAX_BPS_32K                 0x400    //32K16Bit2Channel=1024k
#define USB_UAC_MAX_BPS_48K                 0x600    //48K16Bit2Channel=1536k
#define USB_UAC_SAMPLES_PER_FRAME           0x400    //1024
#define USB_UAC_MPEG_FMT_LENGTH             0x08
#define USB_UAC_MPEG_CAP                    0x0040   //MPEG 2/D6 //0x00
#define USB_UAC_MPEG_FEATURE                0x00
#define UAC_DATA_FMT_TAG                    UAC_DATA_FMT_TYPE_II_MPEG //UAC_DATA_FMT_TYPE_MPEG2EXT //UAC_DATA_FMT_TYPE_AC3  //UAC_DATA_FMT_TYPE_MPEG
#else
#define USB_UAC_TYPE_X_FMT_LENGTH           USB_UAC_TYPE_I_FMT_LENGTH
#define USB_UAC_MPEG_FMT_LENGTH             0
#define UAC_DATA_FMT_TAG                    UAC_DATA_FMT_TYPE_I_PCM
#endif
#endif

//----------------------------------------------------


#define UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE        64
#define UVAC_CFGA_INTF0_ALT0_EP1_MAX_HS_PACKET_SIZE     512
#define UVAC_IF_CFGA_IF0_NUMBER_EP                      1

typedef enum _UVAC_IF_NUMBER_ {
	UVAC_IF_IF0_NUMBER = 0,
	UVAC_IF_IF1_NUMBER,
	UVAC_IF_IF2_NUMBER,
	UVAC_IF_IF3_NUMBER,
	UVAC_IF_MAX_NUMBER,
} UVAC_IF_NUMBER;

#define UVAC_VENDOR_ID                   0x0603             /* vendor id NVT(0x0603)            */
//#define UVAC_PRODUCT_ID                  0x8612             /* product id NVT                   */
#define UVAC_PRODUCT_ID                  0x8613             /* product id NVT                   */

#define UVAC_SPEC_RELEASE_NUMBER         0x0200            /* spec. release number             */
#define UVAC_DEV_MAX_PACKET_SIZE0        64                 /* max packet size for endpoint 0   */
#define UVAC_DEV_NUM_CONFIG              1                  /* number of possible configuration */
#define UVAC_DEV_RELEASE_NUMBER          0x0010             /* device release number            */

#define UVAC_CFGA_CFG_VALUE              1               /* configuration value              */
#define UVAC_CFGA_CFG_IDX                0               /* configuration string id          */
#define UVAC_CFGA_CFG_ATTRIBUES          0xC0            /* characteristics                  */
#define UVAC_CFGA_MAX_POWER              50              /* maximum power in 160mA??         */
#define UVAC_IF_ALT0                     0
#define UVAC_IF_IDX                      0


#define UVC_HS_BULKIN_BLKSIZE_HBYTE      0x02
#define UVC_HS_BULKIN_BLKSIZE_LBYTE      0x00
#define UVC_FS_BULKIN_BLKSIZE_HBYTE      0x00
#define UVC_FS_BULKIN_BLKSIZE_LBYTE      0x40
#define UVC_EP_INTR_HSINTERVAL           0x0B   //2**(12-1)*125us=125ms

#define UVC_HS_BULKIN_BLKSIZE            ((UVC_HS_BULKIN_BLKSIZE_HBYTE << 8) | UVC_HS_BULKIN_BLKSIZE_LBYTE)  //512
#define UVC_HS_BULKIN_MAXPKTSIZE         ((UVC_HS_BULKIN_BLKSIZE_HBYTE << 8) | UVC_HS_BULKIN_BLKSIZE_LBYTE)  //512
#define UVC_FS_BULKIN_BLKSIZE            ((UVC_FS_BULKIN_BLKSIZE_HBYTE << 8) | UVC_FS_BULKIN_BLKSIZE_LBYTE)  //64
#define UVC_FS_BULKIN_MAXPKTSIZE         ((UVC_FS_BULKIN_BLKSIZE_HBYTE << 8) | UVC_FS_BULKIN_BLKSIZE_LBYTE)  //64

#define USB_PCC_IAD_IF_COUNT                2   //control interface, video-stream interface

//Descriptor length
#define USB_UVC_IAD_LENGTH          0x08

//IAD type
#define USB_IAD_DESCRIPTOR_TYPE     0x0B

//--------------------------------------------------------
//For UVC descriptor definition
#define USB_UVC_DEV_CLASS                   0xEF        // class code
#define USB_UVC_DEV_SUBCLASS                0x02        // sub class code
#define USB_UVC_DEV_PROTOCOL                0x01        // protocol code

// ID
#define USB_UVC_ID1                         0x01
#define USB_UVC_ID2                         0x02
#define USB_UVC_ID3                         0x03
#define USB_UVC_ID4                         0x04
#define USB_UVC_ID5                         0x05
#define USB_UVC_ID6                         0x06
#define USB_UVC_ID7                         0x07
#define USB_UVC_ID8                         0x08
#define USB_UVC_ID9                         0x09

//Extention Unit
#define USB_UVC_XU_GUID1                    0xc987a729
#define USB_UVC_XU_GUID2                    0x456959d3
#define USB_UVC_XU_GUID3                    0x08ff6784
#define USB_UVC_XU_GUID4                    0xe819fc49

#define USB_UVC_XU_ID                       USB_UVC_ID5 //UVAC_1UVC_1UAC
//#define USB_UVC_XU_ID                       USB_UVC_ID6 //UVAC_2UVC_2UAC
#define UVC_XU_CONTROL_NUMS                 0x10         // bNumControls
#define UVC_XU_CONTROLSIZE                  0x02         // bControlSize

#define UVC_XU_BMCONTROLS                   0xFFFF

//#if (UVAC_UVC_UAC_DEV_SET == UVAC_1UVC_1UAC)
#define USB_UVC_XU_LENGTH                   (0x19 + UVC_XU_CONTROLSIZE)
#define UVC_XU_INPUT_PINS                   0x01        // bNrInPins
#define UVC_XU_STR_INDEX                        0x00        // iExtension
//#else
//#define USB_UVC_XU_LENGTH                   0
//#endif

//


//IAD
//lilyttt : test configuration fail
#define USB_PCC_IAD_FIRST_VID_IF                0
#define USB_PCC_IAD_SECOND_VID_IF               4


#define USB_PCC_IAD_IF_COUNT_AUD            1
//#define UVAC_IAD_STR_INDEX               0           // iFunction

//#define UVAC_IAD_STR_INDEX2              1           // iFunction


//#define USB_PCC_UVC_NUMBER_IF               2*(UVAC_VID_DEV_CNT + UVAC_AUD_DEV_CNT)   // number of interfaces
#define USB_PCC_UVC_NUMBER_IF               2*(UVAC_VID_DEV_CNT_MAX + UVAC_AUD_DEV_CNT_MAX)   // number of interfaces

//#if (UVAC_UVC_UAC_DEV_SET == UVAC_1UVC_1UAC)  //lilyttt : test configuration fail
#define USB_PCC_IAD_FIRST_IF_UAC            2 //2:audio-control-1, 3:audio-stream-1;
//#else
//#define USB_PCC_IAD_FIRST_IF_UAC            4 //4:audio-control-1, 5:audio-stream-1;
//#endif

#define USB_PCC_IAD_SECOND_IF_UAC           6 //6:audio-control-1, 7:audio-stream-1
#if 1 //(ENABLE == UVC_METHOD3)
#define UVAC_IF_UVC_M3_NUMBER               8
#endif


#define UVAC_IF_ALT1                     1
//#NT#2012/12/07#Lily Kao -begin
//#NT#IsoIn Txf
#define UVAC_IF_ALT2                     2
#define UVAC_IF_ALT3                     3
#define UVAC_IF_ALT4                     4
//#NT#2012/12/07#Lily Kao -end

//Define fields of [Class-spec. VC interface] descriptor
#define USB_UVC_RELEASE_NUMBER              0x0100       //0x0210 //// UVC version :0x0110 is fail to connect to the pc (win8/7/vista)
#define USB_UVC_110_RELEASE_NUMBER          0x0110       // UVC version 1.1
#define USB_UVC_150_RELEASE_NUMBER          0x0150       // UVC version 1.5
#if 1
#define UVC_VC_CLOCK_FREQUENCY              0x01E00000   // dwClockFrequency of  :30MHz
#else
#define UVC_VC_CLOCK_FREQUENCY              0x005B8D80   // dwClockFrequency of  :6MHz
#endif
#define UVC_VC_VIDEOSTREAM_TOTAL            1            // baInterfaceNr (total VS interface numbers)
#define UVC_VC_VIDEOSTREAM_FIRSTNUM         (USB_PCC_IAD_FIRST_VID_IF+1)        //1   // bInCollection (Number of first streaming interfaces)
#define UVC_VC_VIDEOSTREAM_SECONDNUM        (USB_PCC_IAD_SECOND_VID_IF+1)        //1   // bInCollection (Number of first streaming interfaces)

//Define fields of [Camera input terminal] descriptor
#define UVC_IT_ASSOC_TERMINAL               0x00         // bAssocTerminal
#define UVC_IT_ITERMINAL_STR_INDEX              0x00         // iTerminal
#define UVC_IT_OBJFOCAL_LEN_MIN             0x0000       // wObjectiveFocalLengthMin
#define UVC_IT_OBJFOCAL_LEN_MAX             0x0000       // wObjectiveFocalLengthMax
#define UVC_IT_OBJFOCAL_LEN                 0x0000       // wOcularFocalLength
#define UVC_IT_CTRL_SIZE                    0x02         // bControlSize
#define UVC_IT_CONTROLS                     0x000A       // bmControls

//Define fields of [Output terminal] descriptor
#define UVC_OT_ASSOC_TERMINAL               0x00         // bAssocTerminal
#define UVC_OT_ITERMINAL_STR_INDEX              0x00         // iTerminal

//Define fields of [Select unit] descriptor
#define UVC_SU_PIN_NUMBERS                  0x01         // input pin numbers
#define UVC_SU_SELECT_STR_INDEX                 0x00         // index of string descriptor (unused)

//Define fields of [Processing unit] descriptor
#define UVC_PU_MAX_MULTIPLIER               0x0000       // wMaxMultiplier
#define UVC_PU_CTRL_SIZE                    0x03         // bControlSize for UVC 1.1 and 1.5
#define UVC_100_PU_CTRL_SIZE                0x02         // bControlSize for UVC 1.0
#define UVC_PU_CONTROLS                     0x175F       // bmControls
#define UVC_PU_PROCESS_STR_INDEX             0x00         // index of string descriptor (unused)
#define UVC_PU_VID_STANDARDS                0x00         // bitmap of all analog video standards supported by the Processing Unit

//Define fields of [Standard VS interface] descriptor
#define UVC_VS_IF0_NUMBER_EP                0x01
#define UVC_VS_INTERFACE_STR_INDEX              0x00         // index of string descriptor (unused)
#define UVC_VS_IF0_NUMBER_EP                0x01


//Define fields of [Class-spec VS input header] descriptor
#define USB_UVC_FORMAT_NUM                  0x01
#define USB_UVC_TOTAL_VS_LENGTH             (USB_UVC_VS_INPUT_HEADER_LENGTH+\
		USB_UVC_VS_FORMAT_LENGTH+\
		USB_UVC_VS_FRAME_LENGTH+\
		USB_UVC_VS_FRAME_LENGTH)
//#NT#2011/01/05#Hideo Lin -begin
//#NT#For PCC maximum size setting (add one more USB_UVC_VS_FRAME_LENGTH for PCC 720p)
#define USB_UVC_TOTAL_VS_LENGTH_HD          (USB_UVC_VS_INPUT_HEADER_LENGTH+\
		USB_UVC_VS_FORMAT_LENGTH+\
		USB_UVC_VS_FRAME_LENGTH+\
		USB_UVC_VS_FRAME_LENGTH+\
		USB_UVC_VS_FRAME_LENGTH)
//#NT#2011/01/05#Hideo Lin -end
//#NT#2012/12/07#Lily Kao -begin
//#NT#IsoIn Txf
//#define UVC_VS_BMINFO                      0x01        // dynamic format change supported
#define UVC_VS_BMINFO                      0x00        // No dynamic format change supported
//#NT#2012/12/07#Lily Kao -end

#define UVC_VS_STILL_CAPTURE_NONE          0            // method of still capture supported
#define UVC_VS_STILL_CAPTURE_M1            0x01         // method of still capture supported
#define UVC_VS_STILL_CAPTURE_M2            0x02         // method of still capture supported
#define UVC_VS_STILL_CAPTURE_M3            0x03        // method of still capture supported
#if 1 //(ENABLE == UVC_METHOD3)
#define UVC_VS_STILL_CAPTURE               UVC_VS_STILL_CAPTURE_M3
#define UVC_VS_STILL_FRM_MAX_SIZE          0x300000     //3M  //0x600000     //6M
#define UVC_VS_STILL_FRM_MIN_SIZE          0x4000       //16k
#define UVC_VS_STILL_FRM_DEF_SIZE          0x100000     //1M
#define UVC_VS_STILL_DEFAULT_FRM_IDX       0x01
#else
#define UVC_VS_STILL_CAPTURE               UVC_VS_STILL_CAPTURE_NONE        // method of still capture supported
#endif


#define UVC_VS_HW_TRIGGER                   0x00        // HW trigger supported
#define UVC_VS_HW_TRIGGER_USAGE             0x00        // HW trigger usage
#define UVC_VS_CTRL_SIZE                    0x01        // bControlSize
//#NT#2012/12/07#Lily Kao -begin
//#NT#IsoIn Txf
#define UVC_VS_CONTROLS                     0x04 //D02-Compression Quality 0x00        // bmControls
//#NT#2012/12/07#Lily Kao -end

//Define fields of [Class spec VS Format ] descriptor
#define UVC_VSFMT_FIRSTINDEX                0x01        // format index (one-based)
#define UVC_VSFMT_FRAMENUM1                 0x02        // Number of frame descriptors
#define UVC_VSFMT_FRAMENUM1_HD              0x08        // Number of frame descriptors for HD size supporting
#define UVC_VSFMT_BMINFO                    0x00        // fix size samples
#define UVC_VSFMT_DEF_FRAMEINDEX            0x01        // default frame index
#define UVC_VSFMT_ASPECTRATIO_X             0x00        // aspect ratio X
#define UVC_VSFMT_ASPECTRATIO_Y             0x00        // aspect ratio Y
#define UVC_VSFMT_INTERLACE                 0x00        // bmInterlaceFlags
#define UVC_VSFMT_COPYPROTECT               0x01        // bCopyProtect

//Define fields of [Class spec Frame ] descriptor
#define UVC_VSFRM_INDEX1                    0x01        // frame index
#define UVC_VSFRM_INDEX1_CAP                0x03        // D0: still image  D1:Fix frame rate
#define UVC_VSFRM_INDEX2                    0x02        // frame index
#define UVC_VSFRM_INDEX2_CAP                0x03        // D0: still image  D1:Fix frame rate
//#NT#2010/08/20#Hideo Lin -begin
//#NT#For PCC 720p test
#define UVC_VSFRM_INDEX3                    0x03        // frame index
#define UVC_VSFRM_INDEX3_CAP                0x03        // D0: still image  D1:Fix frame rate
#define UVC_VSFRM_INDEX4                    0x04        // frame index
#define UVC_VSFRM_INDEX4_CAP                0x03        // D0: still image  D1:Fix frame rate
#define UVC_VSFRM_INDEX5                    0x05        // frame index
#define UVC_VSFRM_INDEX5_CAP                0x03        // D0: still image  D1:Fix frame rate
#define UVC_VSFRM_INDEX6                    0x06        // frame index
#define UVC_VSFRM_INDEX6_CAP                0x03        // D0: still image  D1:Fix frame rate
#define UVC_VSFRM_INDEX7                    0x07        // frame index
#define UVC_VSFRM_INDEX7_CAP                0x03        // D0: still image  D1:Fix frame rate
#define UVC_VSFRM_INDEX8                    0x08        // frame index
#define UVC_VSFRM_INDEX8_CAP                0x03        // D0: still image  D1:Fix frame rate

#define UVC_VSFRM_INDEX9                    0x09        // frame index
#define UVC_VSFRM_INDEX10                   0x0A        // frame index
#define UVC_VSFRM_INDEX11                   0x0B        // frame index

//MPJG
#define UVC_VSFRM_BITRATE_MIN               0x02000000  //32M  bps =  4M  byte
#define UVC_VSFRM_BITRATE_MAX               0x10000000  //256M bps = 32M byte
//#define UVC_VSFRM_MAX_FBSIZE                0x00080000  //512k byte ==> ring buffer size associated
#define UVC_VSFRM_MAX_FBSIZE                0x00300000  //3M  //UVC_H264ENC_RATECTRL_MAX

#define UVC_VSFRM_INTERVAL_TYPE             0x00

//0x0011CD4C/8.57~8fps,0x000F4241/9.9~10fps,0x000CB736/11.9~12fps,0x000A2C2B/14.99~15fps,0x0007A120/20fps,0x00051615/30fps
#define UVC_VSFRM_FRAMEINTERVAL_DEF         0x00051615  //30fps, 333333
#define UVC_VSFRM_FRAMEINTERVAL_60FPS       0x00028B0A  //60fps, 166666
#if 0
#define UVC_VSFRM_FRAMEINTERVAL_MAX         0x0011CD4C  //8fps
#define UVC_VSFRM_FRAMEINTERVAL_MIN         0x00051615  //30fps
#define UVC_VSFRM_FRAMEINTERVAL_STEP        0x00028B0B
#else
#define UVC_VSFRM_FRAMEINTERVAL_MAX         UVC_VSFRM_FRAMEINTERVAL_DEF
#define UVC_VSFRM_FRAMEINTERVAL_MIN         UVC_VSFRM_FRAMEINTERVAL_DEF //UVC_VSFRM_FRAMEINTERVAL_60FPS
#define UVC_VSFRM_FRAMEINTERVAL_STEP        0x00028B0B
#endif



#define USB_UVC_QVGA_WIDTH                  320
#define USB_UVC_QVGA_HEIGHT                 240
#define USB_UVC_VGA_WIDTH                   640
#define USB_UVC_VGA_HEIGHT                  480
#define USB_UVC_FULL_HD_WIDTH               1920
#define USB_UVC_FULL_HD_HEIGHT              1080
#define USB_UVC_HD_WIDTH                    1280
#define USB_UVC_HD_HEIGHT                   720
#define USB_UVC_1024x768_WIDTH              1024
#define USB_UVC_1024x768_HEIGHT             768
#define USB_UVC_1024x576_WIDTH              1024
#define USB_UVC_1024x576_HEIGHT             576
#define USB_UVC_800x600_WIDTH               800
#define USB_UVC_800x600_HEIGHT              600
#define USB_UVC_800x480_WIDTH               800
#define USB_UVC_800x480_HEIGHT              480
#define USB_UVC_640x360_WIDTH               640
#define USB_UVC_640x360_HEIGHT              360

#define UVC_USB_MAX_ENDPOINT_IDX            15   //660 usb driver support 16(UVC_USB_MAX_ENDPOINT_IDX+1) endpoints
#define UVC_USB_FIFO_UNIT_SIZE              0x200
typedef enum _UVC_USB_DEDICATED_DMA_FIFO_ {
	UVC_USB_DEDICATED_DMA_FIFO_CNT_1 = 1,
	UVC_USB_DEDICATED_DMA_FIFO_CNT_2,
	UVC_USB_DEDICATED_DMA_FIFO_CNT_3,
	UVC_USB_DEDICATED_DMA_FIFO_CNT_MAX  //4
} UVC_USB_DEDICATED_DMA_FIFO;
typedef enum _UVC_USB_NONDEDICATED_DMA_FIFO_ {
	UVC_USB_NONDEDICATED_DMA_FIFO_CNT_1 = 1,
	UVC_USB_NONDEDICATED_DMA_FIFO_CNT_2,
	UVC_USB_NONDEDICATED_DMA_FIFO_CNT_3,
	UVC_USB_NONDEDICATED_DMA_FIFO_CNT_MAX  //4
} UVC_USB_NONDEDICATED_DMA_FIFO;

#define UVC_ISOIN_HS_PACKET_SIZE            (UVC_USB_FIFO_UNIT_SIZE * UVC_USB_DEDICATED_DMA_FIFO_CNT_2)
#define UVC_ISOIN_CAPIMG_BANDWIDTH          BLKNUM_SINGLE
#define UVC_ISOIN_BANDWIDTH                 BLKNUM_SINGLE   //BLKNUM_DOUBLE  //BLKNUM_TRIPLE //BLKNUM_SINGLE //BLKNUM_DOUBLE
#define UVC_USB_UVC_FIFO_PINGPONG           DISABLE         //ENABLE  //DISABLE  //
#define UVC_USB_UAC_FIFO_PINGPONG           DISABLE         //ENABLE  //DISABLE  //


#define UVC_ISOIN_FS_PACKET_SIZE            510  // < 0x800

#define USB_HS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE        (UVC_ISOIN_HS_PACKET_SIZE | ((UVC_ISOIN_BANDWIDTH-1)<<11))
#define USB_FS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE        (UVC_ISOIN_FS_PACKET_SIZE | ((UVC_ISOIN_BANDWIDTH-1)<<11))//(0x03FC)  //1020

#define UVC_ISOIN_AUD_BANDWIDTH             BLKNUM_SINGLE //BLKNUM_DOUBLE //BLKNUM_SINGLE
#define UVC_ISOIN_AUD_HS_PACKET_SIZE       (UVC_USB_FIFO_UNIT_SIZE * UVC_USB_DEDICATED_DMA_FIFO_CNT_1)  //    0x200  //0x40  //
#define UVC_ISOIN_AUD_FS_PACKET_SIZE        0x40

#define USB_UVC_VS_INPUT_HEADER_LENGTH_NEW  (0x0D + UVC_VS_CTRL_SIZE)

#define USB_UVC_TOTAL_VS_LENGTH_FB_H264     (USB_UVC_VS_INPUT_HEADER_LENGTH+\
		USB_UVC_VS_FBH264_FORMAT_LENGTH+\
		USB_UVC_VS_FBH264_FRAME_LENGTH*UVC_VSFMT_FRAMENUM1_HD)

//DIFF V1,V2 configuration //lower priority
#if 0
#define UVC_DIFF_V1V2_CONFIG                ENABLE
#if UVC_DIFF_V1V2_CONFIG
#define UVC_ISOIN_HS_PACKET_SIZE_V2         (UVC_USB_FIFO_UNIT_SIZE * UVC_USB_DEDICATED_DMA_FIFO_CNT_2)
#define UVC_ISOIN_BANDWIDTH_V2             BLKNUM_SINGLE   //BLKNUM_DOUBLE  //BLKNUM_TRIPLE //BLKNUM_SINGLE //BLKNUM_DOUBLE
#define USB_HS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE_V2        (UVC_ISOIN_HS_PACKET_SIZE_V2 | ((UVC_ISOIN_BANDWIDTH_V2-1)<<11))
#else
#define UVC_ISOIN_HS_PACKET_SIZE_V2        UVC_ISOIN_HS_PACKET_SIZE
#define UVC_ISOIN_BANDWIDTH_V2             UVC_ISOIN_BANDWIDTH
#define USB_HS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE_V2        USB_HS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE
#endif
#endif
//DIFF V1,V2 configuration >



#if 1 //(ENABLE == UVC_METHOD3)
#define USB_UVC_TOTAL_VS_LENGTH_FB_H264_MJPG_NEW   (USB_UVC_TOTAL_VS_LENGTH_FB_H264 + \
		USB_UVC_VS_COLOR_LENGTH * (USB_UVC_FORMAT_NUM2-1) + \
		USB_UVC_VS_MJPG_FORMAT_LENGTH + \
		USB_UVC_VS_MJPG_FRAME_LENGTH*UVC_VSFMT_FRAMENUM_MJPG +\
		UVC_M3_CAP_FRAME_DESC_LENGTH)


#else
#define USB_UVC_TOTAL_VS_LENGTH_FB_H264_MJPG_NEW   (USB_UVC_TOTAL_VS_LENGTH_FB_H264 + \
		USB_UVC_VS_COLOR_LENGTH * (USB_UVC_FORMAT_NUM2-1) + \
		USB_UVC_VS_MJPG_FORMAT_LENGTH + \
		USB_UVC_VS_MJPG_FRAME_LENGTH*UVC_VSFMT_FRAMENUM_MJPG)
#endif

#define UVC_IT_INPUT_CTRL_SIZE              0x03     //UVC 1.5 use fixed size, 3

#define USB_UVC_VC_TOTAL_LENGTH             (USB_UVC_VC_IF_LENGTH+USB_UVC_IT_LENGTH+USB_UVC_OT_LENGTH+USB_UVC_SU_LENGTH+USB_UVC_PU_LENGTH+USB_UVC_XU_LENGTH)

#define UVC_COLOR_PRIMARIES                 0x00
#define UVC_COLOR_TRANSFER                  0x00
#define UVC_COLOR_MATRIX                    0x00

#define IsoLen2UVC2UAC_INTR                 ENABLE //DISABLE  //

#define USB_UAC_TOTAL_LENGTH                (USB_UAC_IAD_LENGTH +\
		USB_UAC_AC_IF_LENGTH +\
		USB_UAC_CS_AC_IF_LENGTH +\
		USB_UAC_IT_LENGTH +\
		USB_UAC_OT_LENGTH +\
		USB_UAC_FU_LENGTH +\
		USB_UAC_AS_LENGTH +\
		USB_UAC_AS_LENGTH +\
		USB_UAC_AS_CS_LENGTH +\
		USB_UAC_TYPE_I_FMT_LENGTH +\
		USB_UAC_STAND_ENDPOINT_LENGTH +\
		USB_UAC_CS_ENDPOINT_GENERAL_LENGTH +\
		USB_UAC_AS_LENGTH +\
		USB_UAC_AS_CS_LENGTH +\
		USB_UAC_TYPE_I_FMT_LENGTH +\
		USB_UAC_STAND_ENDPOINT_LENGTH +\
		USB_UAC_CS_ENDPOINT_GENERAL_LENGTH +\
		USB_UAC_AS_LENGTH +\
		USB_UAC_AS_CS_LENGTH +\
		USB_UAC_TYPE_I_FMT_LENGTH +\
		USB_UAC_STAND_ENDPOINT_LENGTH +\
		USB_UAC_CS_ENDPOINT_GENERAL_LENGTH )


#define USB_UVC_TOTAL_LENGTH_FB_H264        (USB_CFG_LENGTH+\
		USB_UVC_IAD_LENGTH+\
		USB_IF_LENGTH+\
		USB_UVC_VC_IF_LENGTH+\
		USB_UVC_IT_LENGTH+\
		USB_UVC_OT_LENGTH+\
		USB_UVC_SU_LENGTH+\
		USB_UVC_PU_LENGTH+\
		USB_BULK_EP_LENGTH+\
		USB_UVC_EP_LENGTH+\
		USB_IF_LENGTH+\
		USB_BULK_EP_LENGTH+\
		USB_UVC_VS_INPUT_HEADER_LENGTH+\
		USB_UVC_VS_FBH264_FORMAT_LENGTH+\
		USB_UVC_VS_FBH264_FRAME_LENGTH*UVC_VSFMT_FRAMENUM1_HD+\
		USB_UVC_VS_COLOR_LENGTH)



#if (IsoLen2UVC2UAC_INTR == ENABLE)
#define IntrLen                             (USB_BULK_EP_LENGTH + USB_UVC_EP_LENGTH)
#else
#define IntrLen                              0
#endif

#define SingleUVCS                          (USB_UVC_IAD_LENGTH+\
		USB_IF_LENGTH+\
		USB_UVC_VC_IF_LENGTH+\
		USB_UVC_IT_LENGTH+\
		USB_UVC_OT_LENGTH+\
		USB_UVC_SU_LENGTH+\
		USB_UVC_PU_LENGTH+\
		USB_IF_LENGTH+\
		USB_UVC_VS_INPUT_HEADER_LENGTH+\
		USB_UVC_VS_FBH264_FORMAT_LENGTH+\
		(USB_UVC_VS_FBH264_FRAME_LENGTH*UVC_VSFMT_FRAMENUM1_HD)+\
		USB_UVC_VS_COLOR_LENGTH+\
		USB_UVC_VS_MJPG_FORMAT_LENGTH+\
		(USB_UVC_VS_MJPG_FRAME_LENGTH*UVC_VSFMT_FRAMENUM_MJPG) +\
		USB_UVC_VS_COLOR_LENGTH+\
		3*(USB_IF_LENGTH + USB_BULK_EP_LENGTH))


#if 1 //(ENABLE == UVC_METHOD3)
#define UVC_M3_CAP_FRAME_DESC_LENGTH         (10+(4*UVC_M3_IMG_SIZE_PATT_CNT)-4+UVC_M3_IMG_COMPRESS_CNT)
#define UVC_M3_IMG_SIZE_PATT_CNT             3
#define UVC_M3_IMG_14M_WIDTH                 4320
#define UVC_M3_IMG_14M_HEIGHT                3240
#define UVC_M3_IMG_12M_WIDTH                 4320
#define UVC_M3_IMG_12M_HEIGHT                2880
#define UVC_M3_IMG_8M_WIDTH                  3264
#define UVC_M3_IMG_8M_HEIGHT                 2448
#define UVC_M3_IMG_COMPRESS_CNT              0

#define IsoLenUVCSUAC                      ( USB_CFG_LENGTH +\
		(SingleUVCS * UVAC_VID_DEV_CNT_MAX) +\
		(IntrLen * UVAC_VID_DEV_CNT_MAX) +\
		(UVAC_AUD_DEV_CNT_MAX * USB_UAC_TOTAL_LENGTH) +\
		USB_BULK_EP_LENGTH + UVC_M3_CAP_FRAME_DESC_LENGTH +\
		(USB_UVC_XU_LENGTH * UVAC_VID_DEV_CNT_MAX))

#else

#define IsoLenUVCSUAC                      ( USB_CFG_LENGTH +\
		(SingleUVCS * UVAC_VID_DEV_CNT) +\
		(IntrLen * UVAC_VID_DEV_CNT) +\
		(UVAC_AUD_DEV_CNT * USB_UAC_TOTAL_LENGTH) +\
		(USB_UVC_XU_LENGTH * UVAC_VID_DEV_CNT))

#endif

#define USB_UVC_FORMAT_NUM2                 0x02
#define UVC_VSFMT_SECONDINDEX               0x02
#define USB_UVC_PROBE_LENGTH                26    // UVC1.0 => 26
#define USB_UVC_110_PROBE_LENGTH            34    // UVC1.1 => 34
#define USB_UAC_PROBE_LENGTH                2

extern USB_DEVICE_DESC g_U2UVACDevDesc;
extern const USB_DEVICE_DESC g_U2UVACDevQualiDesc;
extern UINT8 g_U2UVACFSConfigDescFBH264[];
extern UINT8 g_U2UVACHSConfigDescFBH264[]; //=> replaced with "gpUvacHSConfigDesc"
extern UINT8 g_U2UVACFSOtherConfigDescFBH264[], g_U2UVACHSOtherConfigDescFBH264[];
extern const UINT8 U2UvacStrDesc0[];
extern const UINT8 U2UvacStrDesc1[], U2UvacStrDesc2[];
extern const UINT16 U2UvacStrDesc3[];
extern const UINT16 U2UvacStrDesc4[];

extern UINT16 U2UVAC_MakeHSConfigDesc(UINT8 *pUvacHSConfigDesc, UINT16 maxLen);

//=============================


//Video Interface Class Code
#define CC_VIDEO                            0x0E

//Video Interface Subclass Codes
#define SC_UNDEFINED                        0x00
#define SC_VIDEOCONTROL                     0x01
#define SC_VIDEOSTREAMING                   0x02
#define SC_VIDEO_INTERFACE_COLLECTION       0x03

//Video Interface Protocol Codes
#define PC_PROTOCOL_UNDEFINED               0x00
#define PC_PROTOCOL_15                      0x01   //0x01  :0x01 is fail in win8

//Video Class-Specific Descriptor Types
#define CS_UNDEFINED                        0x20
#define CS_DEVICE                           0x21
#define CS_CONFIGURATION                    0x22
#define CS_STRING                           0x23
#define CS_INTERFACE                        0x24
#define CS_ENDPOINT                         0x25

//Video Class-Specific VC Interface Descriptor Subtypes
#define VC_DESCRIPTOR_UNDEFINED             0x00
#define VC_HEADER                           0x01
#define VC_INPUT_TERMINAL                   0x02
#define VC_OUTPUT_TERMINAL                  0x03
#define VC_SELECTOR_UNIT                    0x04
#define VC_PROCESSING_UNIT                  0x05
#define VC_EXTENSION_UNIT                   0x06

//Video Class-Specific VS Interface Descriptor Subtypes
#define VS_UNDEFINED                        0x00
#define VS_INPUT_HEADER                     0x01
#define VS_OUTPUT_HEADER                    0x02
#define VS_STILL_IMAGE_FRAME                0x03
#define VS_FORMAT_UNCOMPRESSED              0x04
#define VS_FRAME_UNCOMPRESSED               0x05
#define VS_FORMAT_MJPEG                     0x06
#define VS_FRAME_MJPEG                      0x07
#define VS_FORMAT_MPEG2TS                   0x0A
#define VS_FORMAT_DV                        0x0C
#define VS_COLORFORMAT                      0x0D
#define VS_FORMAT_FRAME_BASED               0x10
#define VS_FRAME_FRAME_BASED                0x11
#define VS_FORMAT_STREAM_BASED              0x12
#define VS_FORMAT_H264                              0x13  //VS_FORMAT_UNCOMPRESSED  //
#define VS_FRAME_H264                               0x14  //VS_FRAME_UNCOMPRESSED   //
#define VS_FORMAT_H264_SIMULCAST                    0x15
#define UVC_BITS_PER_PIXEL                          24
//UVC_VSFRM_INDEX2=0x02=>gUvcVidResoAry[1]1280X720(internal reso-tbl without update by project
#define UVC_VSFMT_DEF_FRM_IDX                       UVC_VSFRM_INDEX1
#define UVC_VSFMT_PREV_FRM_IDX                      0xFF
#define UVC_MAX_CODEC_CONFIG_DELAY                  0x01
#define UVC_SUPP_SLIDE_MODE                         0x00
#define UVC_SUPP_SYNC_FRM_TYPE                      0x21
#define UVC_RESO_SCALING                            0x00
#define UVC_RESERVED1                               0x00
#define UVC_SUPP_RATE_CONTROL_MODE                  0x0D

#define UVC_MAX_BYTE_PERSEC_ONE_RESO_NOSCALE        0x006C //(108,000 MB/sec);a unit -> 1000 MB/s
#define UVC_MAX_BYTE_PERSEC_TWO_RESO_NOSCALE        0x00
#define UVC_MAX_BYTE_PERSEC_THREE_RESO_NOSCALE      0x00
#define UVC_MAX_BYTE_PERSEC_FOUR_RESO_NOSCALE       0x00

#define UVC_MAX_BYTE_PERSEC_ONE_RESO_TEMPSCALE        0x006C //(108,000 MB/sec);a unit -> 1000 MB/s
#define UVC_MAX_BYTE_PERSEC_TWO_RESO_TEMPSCALE        0x00
#define UVC_MAX_BYTE_PERSEC_THREE_RESO_TEMPSCALE      0x00
#define UVC_MAX_BYTE_PERSEC_FOUR_RESO_TEMPSCALE       0x00

#define UVC_MAX_BYTE_PERSEC_ONE_RESO_TEMPQUALSCALE        0x01   //a unit -> 1000 MB/s
#define UVC_MAX_BYTE_PERSEC_TWO_RESO_TEMPQUALSCALE        0x00
#define UVC_MAX_BYTE_PERSEC_THREE_RESO_TEMPQUALSCALE      0x00
#define UVC_MAX_BYTE_PERSEC_FOUR_RESO_TEMPQUALSCALE       0x00

#define UVC_MAX_BYTE_PERSEC_ONE_RESO_TEMPSPATSCALE        0x01   //a unit -> 1000 MB/s
#define UVC_MAX_BYTE_PERSEC_TWO_RESO_TEMPSPATSCALE        0x00
#define UVC_MAX_BYTE_PERSEC_THREE_RESO_TEMPSPATSCALE      0x00
#define UVC_MAX_BYTE_PERSEC_FOUR_RESO_TEMPSPATSCALE       0x00

#define UVC_MAX_BYTE_PERSEC_ONE_RESO_FULLSCALE        0x01   //a unit -> 1000 MB/s
#define UVC_MAX_BYTE_PERSEC_TWO_RESO_FULLSCALE        0x00
#define UVC_MAX_BYTE_PERSEC_THREE_RESO_FULLSCALE      0x00
#define UVC_MAX_BYTE_PERSEC_FOUR_RESO_FULLSCALE       0x00


//Video Class-Specific Endpoint Descriptor Subtypes
#define EP_UNDEFINED                        0x00
#define EP_GENERAL                          0x01
#define EP_ENDPOINT                         0x02
#define EP_INTERRUPT                        0x03

//Video Class-Specific Request Codes
#define RC_UNDEFINED                        0x00

//Control Selector Codes
#define VC_CONTROL_UNDEFINED                0x00
#define VC_VIDEO_POWER_MODE_CONTROL         0x01
#define VC_REQUEST_ERROR_CODE_CONTROL       0x02

//Terminal Control Selectors
#define TE_CONTROL_UNDEFINED                0x00

//Selector Unit Control Selectors
#define SU_CONTROL_UNDEFINED                0x00
#define SU_INPUT_SELECT_CONTROL             0x01

//Camera Terminal Control Selectors
#define CT_CONTROL_UNDEFINED                0x00
#define CT_SCANNING_MODE_CONTROL            0x01
#define CT_AE_MODE_CONTROL                  0x02
#define CT_AE_PRIORITY_CONTROL              0x03
#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL 0x04
#define CT_EXPOSURE_TIME_RELATIVE_CONTROL 0x05
#define CT_FOCUS_ABSOLUTE_CONTROL           0x06
#define CT_FOCUS_RELATIVE_CONTROL           0x07
#define CT_FOCUS_AUTO_CONTROL               0x08
#define CT_IRIS_ABSOLUTE_CONTROL            0x09
#define CT_IRIS_RELATIVE_CONTROL            0x0A
#define CT_ZOOM_ABSOLUTE_CONTROL            0x0B
#define CT_ZOOM_RELATIVE_CONTROL            0x0C
#define CT_PANTILT_ABSOLUTE_CONTROL         0x0D
#define CT_PANTILT_RELATIVE_CONTROL         0x0E
#define CT_ROLL_ABSOLUTE_CONTROL            0x0F
#define CT_ROLL_RELATIVE_CONTROL            0x10
#define CT_PRIVACY_CONTROL                  0x11

//Processing Unit Control Selectors
#define PU_CONTROL_UNDEFINED                0x00
#define PU_BACKLIGHT_COMPENSATION_CONTROL 0x01
#define PU_BRIGHTNESS_CONTROL               0x02
#define PU_CONTRAST_CONTROL                 0x03
#define PU_GAIN_CONTROL                     0x04
#define PU_POWER_LINE_FREQUENCY_CONTROL 0x05
#define PU_HUE_CONTROL                      0x06
#define PU_SATURATION_CONTROL               0x07
#define PU_SHARPNESS_CONTROL                0x08
#define PU_GAMMA_CONTROL                    0x09
#define PU_WHITE_BALANCE_TEMPERATURE_CONTROL 0x0A
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 0x0B
#define PU_WHITE_BALANCE_COMPONENT_CONTROL 0x0C
#define PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL 0x0D
#define PU_DIGITAL_MULTIPLIER_CONTROL       0x0E
#define PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL 0x0F
#define PU_HUE_AUTO_CONTROL                 0x10
#define PU_ANALOG_VIDEO_STANDARD_CONTROL 0x11
#define PU_ANALOG_LOCK_STATUS_CONTROL       0x12

//Extension Unit Control Selectors
#define XU_CONTROL_UNDEFINED                0x00

//VideoStreaming Interface Control Selectors
#define VS_CONTROL_UNDEFINED                0x00
#define VS_PROBE_CONTROL                    0x01
#define VS_COMMIT_CONTROL                   0x02
#define VS_STILL_PROBE_CONTROL              0x03
#define VS_STILL_COMMIT_CONTROL             0x04
#define VS_STILL_IMAGE_TRIGGER_CONTROL      0x05
#define VS_STREAM_ERROR_CODE_CONTROL        0x06
#define VS_GENERATE_KEY_FRAME_CONTROL       0x07
#define VS_UPDATE_FRAME_SEGMENT_CONTROL 0x08
#define VS_SYNCH_DELAY_CONTROL              0x09

//USB Terminal Types
#define TT_VENDOR_SPECIFIC                  0x0100
#define TT_STREAMING                        0x0101

//Input terminal type
#define ITT_VENDOR_SPECIFIC                 0x0200
#define ITT_CAMERA                          0x0201
#define ITT_MEDIA_TRANSPORT_INPUT           0x0202

//Output terminal type
#define OTT_VENDOR_SPECIFIC                 0x0300
#define OTT_DISPLAY                         0x0301
#define OTT_MEDIA_TRANSPORT_OUTPUT          0x0302

//External Terminal Types
#define EXTERNAL_VENDOR_SPECIFIC            0x0400
#define COMPOSITE_CONNECTOR                 0x0401
#define SVIDEO_CONNECTOR                    0x0402
#define COMPONENT_CONNECTOR                 0x0403


//Descriptor length
#define USB_UVC_VC_IF_LENGTH        0x0D
#define USB_UVC_IT_LENGTH           0x12  //UVC 1.5 use fixed size, 18
#define USB_UVC_OT_LENGTH           0x09
#define USB_UVC_SU_LENGTH           0x07



#define USB_UVC_PU_LENGTH           0x0B  // UVC1.0 => use fixed bControlSize, 2
#define USB_UVC_110_PU_LENGTH       0x0D  // UVC1.1 => use fixed bControlSize, 3

#define _UVC_SAVE_YUV_SRCDATA_             0
#define _UVC_TXF_H264_DESC_TWICE_          1

#define PAYLOAD_LEN                         0x0C         //Payload length




#define USB_UVC_EP_LENGTH           0x05

#if 0
#define UVAC_VID_H264_ONLY                      DISABLE  //ENABLE DISABLE
#if UVAC_VID_H264_ONLY
#define USB_UVC_VID_FORMAT_CNT                  1   //Only h264
#else
#define USB_UVC_VID_FORMAT_CNT                  2   //both h264, mjpg
#endif
#define USB_UVC_VS_INPUT_HEADER_LENGTH       (0x0D + USB_UVC_VID_FORMAT_CNT)
#endif
#define USB_UVC_VS_INPUT_HEADER_LENGTH      0x0E //for single video format count
#define USB_UVC_VS_FORMAT_LENGTH    0x0B
#define USB_UVC_VS_FRAME_LENGTH     0x26
#define USB_UVC_VS_FRAME_LENGTH_DISCRETE     0x1A
#define USB_UVC_VS_COLOR_LENGTH     0x6


#define USB_UVC_VS_FBH264_FORMAT_LENGTH           0x1C    //28
#define USB_UVC_VS_FBH264_BASE_FRAME_LENGTH       0x1A    //26
#define USB_UVC_VS_FBH264_NUM_FRM_INTVL           7
#define USB_UVC_VS_FBH264_FRAME_LENGTH            0x26 //(USB_UVC_VS_FBH264_BASE_FRAME_LENGTH+4*3)    //26+4*7=54
#define USB_UVC_VS_FBH264_MIN_BIT_RATE            0x02000000 //32M bps =  4M byte
#define USB_UVC_VS_FBH264_MAX_BIT_RATE            0x10000000 //256 bps = 32M byte
#define USB_UVC_VS_FBH264_FRAME_INTERVAL_TYPE     7 //1  //0=continuous, 1=discrete
#define USB_UVC_VS_FBH264_BYTES_PER_LINE          0x00000000
#define USB_UVC_VS_FBH264_CAPABILITIES            0x00   //0x03 // D0: still image  D1:Fix frame rate  //0x00

#define USB_UVC_VS_FBH264_FRAME_INTERVAL_1        0x00051615 //30fps
#define USB_UVC_VS_FBH264_FRAME_INTERVAL_2        0x00065B9A //24fps
#define USB_UVC_VS_FBH264_FRAME_INTERVAL_3        0x0007A120 //20fps
#define USB_UVC_VS_FBH264_FRAME_INTERVAL_4        0x000A2C2A //15fps
#define USB_UVC_VS_FBH264_FRAME_INTERVAL_5        0x000F4240 //10fps
#define USB_UVC_VS_FBH264_FRAME_INTERVAL_6        0x001E8480 //5fps
#define USB_UVC_VS_FBH264_FRAME_INTERVAL_7        0x00028B0A //60fps
#define USB_UVC_VS_FBH264_DEF_FRAME_INTERVAL      USB_UVC_VS_FBH264_FRAME_INTERVAL_1 //USB_UVC_VS_FBH264_FRAME_INTERVAL_6 //


#define USB_UVC_VS_MJPG_FORMAT_LENGTH             0x0B
#define USB_UVC_VS_MJPG_FRAME_LENGTH              0x26
#define UVC_VSFMT_FRAMENUM_MJPG                   0x08        // Number of frame descriptors for MJPG resolution

#define USB_UVC_VS_YUV_FORMAT_LENGTH             0x1B

//Payload header
#define PAYLOAD_FID                         0x01  //Frame ID
#define PAYLOAD_EOF                         0x02  //End of Frame
#define PAYLOAD_PTS                         0x04  //Presentation Time
#define PAYLOAD_SCR                         0x08  //Source Clock Reference
#define PAYLOAD_RSD                         0x10  //Reserved
#define PAYLOAD_STI                         0x20  //Still image
#define PAYLOAD_ERR                         0x40  //Error
#define PAYLOAD_EOH                         0x80  //End of header

#define UVC_STILL_PROBE_COMMIT_LEN                  11 //0x0B
#define UVC_STILLIMG_TRIG_REQ_LEN                   1
//UVC Still Image Trigger Control
#define UVC_STILLIMG_TRIG_CTRL_NORMAL               0
#define UVC_STILLIMG_TRIG_CTRL_TRANSMIT             1
#define UVC_STILLIMG_TRIG_CTRL_TRANSMIT_BULK        2
#define UVC_STILLIMG_TRIG_CTRL_ABORT_TRANSMIT       3

typedef struct _UVC_STILL_PROBE_COMMIT {
	UINT8   bFormatIndex;
	UINT8   bFrameIndex;
	UINT8   bCompressionIndex;
	UINT32  dwMaxVideoFrameSize;
	UINT32  dwMaxPayloadTransferSize;
} UVC_STILL_PROBE_COMMIT, *PUVC_STILL_PROBE_COMMIT;

typedef struct _UVC_PROBE_COMMIT {
	UINT16  bmHint;
	UINT8   bFormatIndex;
	UINT8   bFrameIndex;
	UINT32  dwFrameInterval;
	UINT16  wKeyFrameRate;
	UINT16  wPFrameRate;
	UINT16  wCompQuality;
	UINT16  wCompWindowSize;
	UINT16  wDelay;
	UINT32  dwMaxVideoFrameSize;
	UINT32  dwMaxPayloadTransferSize;
	UINT32  dwClockFrequency;
	UINT8   bmFramingInfo;
	UINT8   bPreferredVersion;
	UINT8   bMinVersion;
	UINT8   bMaxVersion;
	UINT8   bUsage;
	UINT8   bBitDepthLuma;
	UINT8   bmSettings;
	UINT8   bMaxNumberOfRefFramesPlus1;
	UINT16  bmRateControlModes;
	UINT16  bmLayoutPerStream[4];
} UVC_PROBE_COMMIT, *PUVC_PROBE_COMMIT;

typedef struct _UVC_INFO {
	UINT8   bFormatIndex;
	UINT8   bFrameIndex;
	UINT32  dwFrameInterval;
} UVC_INFO;

typedef struct _UVC_PAYLOAD_HEADER {
	UINT8   bHeaderLength;
	UINT8   bmHeaderInfo;
} UVC_PAYLOAD_HEADER;

//UAC Configuration Values
#define USB_UAC_RELEASE_NUMBER              0x0100

//Audio Interface Class Code
#define CC_AUDIO                            0x01
//Audio Interface Subclass Codes
#define SC_AUDIOUNDEFINED                   0x00
#define SC_AUDIOCONTROL                     0x01
#define SC_AUDIOSTREAMING                   0x02
#define SC_MIDISTREAMING                    0x03

//Audio Class-Specific AC Interface Descriptor Subtypes
#define UAC_CS_DESCRIPTOR_UNDEFINED             0x00
#define UAC_CS_HEADER                           0x01
#define UAC_CS_INPUT_TERMINAL                   0x02
#define UAC_CS_OUTPUT_TERMINAL                  0x03
#define UAC_CS_MIXER_UNIT                       0x04
#define UAC_CS_SELECTOR_UNIT                    0x05
#define UAC_CS_FEATURE_UNIT                     0x06
#define UAC_CS_PROCESSING_UNIT                  0x07
#define UAC_CS_EXTENSION_UNIT                   0x08


//UAC Terminal Types
#define UAC_TERMINAL_UNDEFINED              0x100
#define UAC_TERMINAL_STREAMING              0x101
#define UAC_TERMINAL_VENDOR_SPEC            0x1FF



//Audio Input Terminal Type
#define UAC_IT_UNDEFINED                    0x0200
#define UAC_IT_MICROPHONE                   0x0201
#define UAC_IT_DESKTOP_MICROPHONE           0x0202
#define UAC_IT_PERSONAL_MICROPHONE          0x0203
#define UAC_IT_OMNI_DIR_MICROPHONE          0x0204
#define UAC_IT_MICROPHONE_ARRAY             0x0205
#define UAC_IT_PROC_MICROPHONE_ARRAY        0x0206

//Audio Output Terminal Type
#define UAC_OT_UNDEFINED                   0x0300
#define UAC_OT_SPEAKER                     0x0301
#define UAC_OT_HEADPHONES                  0x0302
#define UAC_OT_HEAD_MOUNTED_DISPLAY_AUDIO  0x0303
#define UAC_OT_DESKTOP_SPEAKER             0x0304
#define UAC_OT_ROOM_SPEAKER                0x0305
#define UAC_OT_COMMUNICATION_SPEAKER       0x0306
#define UAC_OT_LOW_FREQ_EFFECTS_SPEAKER    0x0307

#define UAC_IT_ASSOCIATION                  0
#if 1
#define UAC_IT_OUT_CHANNEL_CNT              2 //2 for stereo, 1 for right or left but the HW only right channel workable
#define UAC_IT_OUT_CHANNEL_CONFIG           0x0003  //left-front,right-front
#else
#define UAC_IT_OUT_CHANNEL_CNT              1 //2 for stereo, 1 for right or left but the HW only right channel workable
#define UAC_IT_OUT_CHANNEL_CONFIG           0x0  //Mono sets no position bits.
#endif
#define UAC_IT_OUT_CHANNEL_STR_IDX          0
#define UAC_IT_STR_IDX                      0
#define UAC_IT_ID                           USB_UVC_ID1 //USB_UVC_ID6


#define UAC_OT_ASSOCIATION                  0
#define UAC_OT_STR_IDX                      0
#define UAC_OT_SRC_ID                       UAC_FU_UNIT_ID   //USB_UVC_ID5
#define UAC_OT_ID                           USB_UVC_ID3


#define UAC_FU_SRC_ID                       UAC_IT_ID
#define UAC_FU_CTRL_SIZE                    1
#define UAC_FU_STR_IDX                      0
#define UAC_FU_CTRL_CONFIG                  0x03   //Mute,Volume
//#if (UVAC_UVC_UAC_DEV_SET == UVAC_1UVC_1UAC)
#define UAC_FU_UNIT_ID                      USB_UVC_ID6
//#else
//#define UAC_FU_UNIT_ID                      USB_UVC_ID5
//#endif

#define UAC_AC_AUDIOSTREAM_TOTAL            1

#define USB_UAC_IAD_LENGTH                  0x08
#define USB_UAC_AC_IF_LENGTH                0x09
#define USB_UAC_CS_AC_IF_LENGTH             0x09
#define USB_UAC_IT_LENGTH                   0x0C
#define USB_UAC_OT_LENGTH                   0x09
#define USB_UAC_FU_LENGTH                   0x08

#define USB_UAC_AS_LENGTH                   0x09


#define USB_UAC_AS_CS_LENGTH                0x07
#define USB_UAC_TYPE_I_FMT_LENGTH           0x0B
#define USB_UAC_STAND_ENDPOINT_LENGTH       0x09
#define USB_UAC_CS_ENDPOINT_GENERAL_LENGTH  0x07
#define USB_UAC_CS_ENDPOINT_LENGTH          0x07


#define USB_UAC_AC_TOTAL_LENGTH             (USB_UAC_AC_IF_LENGTH+USB_UAC_IT_LENGTH+USB_UAC_OT_LENGTH+USB_UAC_FU_LENGTH) //0x26=38

#define UVAC_IF_IF0_UAC_NUMBER           2  //usb audio control interface number
#define UVAC_IF_IF0_UAS_NUMBER           3  //usb audio stream interface number
#define UVAC_IF_IF0_UAC_NUMBER_EP0       0  //usb audio control end point number
#define UVAC_IF_IF0_UAS_NUMBER_EP0       0  //usb audio stream interface number
#define UVAC_IF_IF0_UAS_NUMBER_EP1       1  //usb audio stream interface number

// Audio Class-Specific AS Interface Descriptor Subtypes
#define UAC_CS_AS_UNDEFINED                 0x00
#define UAC_CS_AS_GENERAL                   0x01
#define UAC_CS_FORMAT_TYPE                  0x02
#define UAC_CS_FORMAT_SPECIFIC              0x03
#define UAC_CS_AS_OUT_TID                   UAC_OT_ID    //7
#define UAC_CS_AS_DELAY                     0xFF

//Audio Data Format Type I Codes */
#define UAC_DATA_FMT_TYPE_I_UNDEFINED               0x0000
#define UAC_DATA_FMT_TYPE_I_PCM                     0x0001
#define UAC_DATA_FMT_TYPE_I_PCM8                    0x0002
#define UAC_DATA_FMT_TYPE_I_IEEE_FLOAT              0x0003
#define UAC_DATA_FMT_TYPE_I_ALAW                    0x0004
#define UAC_DATA_FMT_TYPE_I_MULAW                   0x0005
#define UAC_DATA_FMT_TYPE_MPEG                      0x1001   //MPEG
#define UAC_DATA_FMT_TYPE_AC3                       0x1002   //AC-3
#define UAC_DATA_FMT_TYPE_MPEG2NOEXT                0x2003   //MPEG2-NoExt
#define UAC_DATA_FMT_TYPE_MPEG2EXT                  0x2004   //MPEG2-Ext

//Audio Data Format Type II Codes */
#define UAC_DATA_FMT_TYPE_II_UNDEFINED            0x0000
#define UAC_DATA_FMT_TYPE_II_MPEG                 0x1001
#define UAC_DATA_FMT_TYPE_II_AC3                  0x1002

//Audio Data Format Type III Codes */
#define UAC_DATA_FMT_TYPE_III_UNDEFINED           0x2000
#define UAC_DATA_FMT_TYPE_III_AC3                 0x2001
#define UAC_DATA_FMT_TYPE_III_MPEG1_LAYER1        0x2002
#define UAC_DATA_FMT_TYPE_III_MPEG2_NOEXT         0x2003
#define UAC_DATA_FMT_TYPE_III_MPEG2_EXT           0x2004
#define UAC_DATA_FMT_TYPE_III_MPEG2_LAYER1_LS     0x2005


//Format Type Codes
#define UAC_FMT_TYPE_UNDEFINED               0x0
#define UAC_FMT_TYPE_I                       0x1
#define UAC_FMT_TYPE_II                      0x2
#define UAC_FMT_TYPE_III                     0x3
#define UAC_EXT_FMT_TYPE_I                   0x81
#define UAC_EXT_FMT_TYPE_II                  0x82
#define UAC_EXT_FMT_TYPE_III                 0x83

//Audio setting
#define UAC_BYTE_EACH_SUBFRAME               0x02
#define UAC_BIT_PER_SECOND                   0x10
#define UAC_FREQ_TYPE_CNT                    0x01
#define UAC_FREQUENCY_08K                    0x001F40   //8k
#define UAC_FREQUENCY_16K                    0x003E80   //16k
#define UAC_FREQUENCY_32K                    0x007D00   //32k
#define UAC_FREQUENCY_44K                    0x00ABE0   //44k
#define UAC_FREQUENCY_48K                    0x00BB80   //48k

#define UAC_NUM_PHYSICAL_CHANNEL             0x02

#define UAC_FREQUENCY_DEF                    UAC_FREQUENCY_48K
#define UAC_FREQUENCY_MIN                    UAC_FREQUENCY_48K
#define UAC_FREQUENCY_MAX                    UAC_FREQUENCY_48K

#define USB_UAC_POLL_INTERVAL                0x04


#define USB_UAC_MAX_PACKET_68                0x400 //0x200 //0x0044  //68bytes
#define USB_UAC_MAX_PACKET_100               0x400 //0x200 //0x0064  //100bytes
#define USB_UAC_MAX_PACKET_132               0x400 //0x200 //0x0084  //132bytes


#define USB_UAC_ENDPOINT_ATTR                0x0D  //0x05  //ISO, Async
#define USB_UAC_EP_REFRESH                   0x00
#define USB_UAC_EP_SYNC_ADDR                 0x00

#define USB_UAC_ISO_DATA_ENDPOINT_ATTR       0x00
#define USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAYUNIT  0x01
#define USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY      333


#define UAC_IF_ALT0                           0
#define UAC_IF_ALT1                           1
#define UAC_IF_ALT2                           2
#define UAC_IF_ALT3                           3

#define UAC_FU_CONTROL_MUTE                   0x01
#define UAC_FU_CONTROL_VOLUME                 0x02

#define UAC_VOL_MAX                           0x64
#define UAC_VOL_MIN                           0x01
#define UAC_VOL_RES                           0x01

#define CDC_COMM_IN_EP          USB_EP6
#define CDC_DATA_IN_EP          USB_EP7
#define CDC_DATA_OUT_EP         USB_EP8

#define CDC_COMM2_IN_EP          USB_EP11
#define CDC_DATA2_IN_EP          USB_EP12
#define CDC_DATA2_OUT_EP         USB_EP13

#define CDC_INTERFACE_NUM             2

#define USB_CDC_DEV_CLASS                  2              /* class code                       */
#define USB_CDC_DEV_SUBCLASS               0              /* sub class code                   */
#define USB_CDC_DEV_PROTOCOL               0              /* protocol code                    */

//Communation Interface Class Code
#define CC_CDC_COMM                         2
//Data Interface Class Code
#define CC_CDC_DATA                         0x0A
//CDC Interface Subclass Codes
#define USB_CDC_IF_SUBCLASS_NONE           0
#define USB_CDC_IF_SUBCLASS_ACM            2
/* bInterfaceProtocol */
#define USB_CDC_IF_PROTOCOL_NONE           0
#define USB_CDC_IF_PROTOCOL_AT_CMD         1

#define USB_CDC_IF_COMM_NUMBER_EP          0

#define USB_CDC_IF_DATA_NUMBER_EP      2

#define USB_2ND_CDC                    DISABLE

#define USB_CDC_DATA_EP_IN_MAX_PACKET_SIZE         0x0200
#define USB_CDC_DATA_EP_OUT_MAX_PACKET_SIZE        0x0200
#define USB_CDC_COMM_EP_INTR_PACKET_SIZE         64

#define USB_EP_CDC_INTERRUPT_HSINTERVAL      0xFF   //2**(255-1)*125us=125ms

//Functional header
#define USB_CDC_FUNCTIONAL_HEADER_LENGTH        5
#define USB_CS_SUBTYPE_HEADER                   0x00  /**< CDC class-specific Header functional descriptor. */
#define USB_CDC_SPEC_VER_MAJOR                  1
#define USB_CDC_SPEC_VER_MINOR                  0x10
//Functional ACM
#define USB_CDC_FUNCTIONAL_ACM_LENGTH           4
#define USB_CS_SUBTYPE_ACM                      0x02  /**< CDC class-specific Abstract Control Model functional descriptor. */
#define USB_CDC_ACM_CAPABILITIES                0x06
//Functional Union
#define USB_CDC_FUNCTIONAL_UNION_LENGTH         5
#define USB_CS_SUBTYPE_UNION                    0x06  /**< CDC class-specific Union functional descriptor. */
#define USB_CDC_MASTER_IF_NUM                   0
#define USB_CDC_SLAVE_IF_NUM                    1

#define SIDC_BULKIN_EP         USB_EP11
#define SIDC_BULKOUT_EP        USB_EP12
#define SIDC_INTRIN_EP         USB_EP13


#define SIDC_INTERFACE_NUM                 1

#define SIDC_IF_NUMBER_EP                  3
#define SIDC_EP_IN_MAX_PACKET_SIZE         512//0x0200
#define SIDC_EP_OUT_MAX_PACKET_SIZE        512//0x0200
#define SIDC_EP_INTR_PACKET_SIZE           64
#define SIDC_EP_INTERRUPT_INTERVAL         0x0B   //2**(12-1)*125us=125ms
#define SIDC_EP_BULKIN_BLKSIZE             512
#define SIDC_EP_BULKOUT_BLKSIZE            512
#define SIDC_EP_INTRIN_BLKSIZE             512

/*---- bInterfaceClass ----*/
#define SIDC_IF_CLASS                      6
/*---- bInterfaceSubClass ----*/
#define SIDC_IF_SUBCLASS                   1
/*---- bInterfaceProtocol ----*/
#define SIDC_IF_PROTOCOL                   1
/*---- interface string descriptor ----*/
#define SIDC_IF_IDX                        0

#define mLowByte(u16)                    ((UINT8)(u16&0xFF))
#define mHighByte(u16)                   ((UINT8)(u16 >> 8))


#define DFU_INTERFACE_NUM                 1
#define DFU_IF_NUMBER_EP                  0

/*---- bInterfaceClass ----*/
#define DFU_IF_CLASS                      0xFE
/*---- bInterfaceSubClass ----*/
#define DFU_IF_SUBCLASS                   1
/*---- bInterfaceProtocol ----*/
#define DFU_IF_PROTOCOL                   1
/*---- interface string descriptor ----*/
#define DFU_IF_IDX                        4

#define DFU_FUNC_LENGTH              0x09    /* DFU Functional descriptor length */
#define DFU_DESCRIPTOR_TYPE          0x21   /* DFU descriptor               */
//#define DFU_ATTRIBUTE                0x9//(BIT0|BIT3) // Download capable:Yes; Upload: No; bitManifestationTolerant: No; bitWillDeatch: Yes.
//#define DFU_DETACH_TIMEOUT           0xFFFF
#define DFU_TRANSFER_SIZE            64
#define DFU_REV_BCD                  0x0110      // Revision of 1.1

/* USB DFU Class Request Types */
#define DFU_DEATCH                      0x00
#define DFU_DNLOAD                      0x01
#define DFU_UPLOAD                      0x02
#define DFU_GETSTATUS                   0x03
#define DFU_CLRSTATUS                   0x04
#define DFU_GETSTATE                    0x05
#define DFU_ABORT                       0x06



#define HID_INTERFACE_NUM                 1
#define HID_IF_NUMBER_EP                  1

/*---- bInterfaceClass ----*/
#define HID_IF_CLASS                      0x03
/*---- bInterfaceSubClass ----*/
#define HID_IF_SUBCLASS                   0
/*---- bInterfaceProtocol ----*/
#define HID_IF_PROTOCOL                   0
/*---- interface string descriptor ----*/
#define HID_IF_IDX                        0

#define HID_FUNC_LENGTH              0x09    /* HID Functional descriptor length */

/* USB HID Class Request Types */
#define HID_GET_REPORT                0x01
#define HID_GET_IDLE                  0x02
#define HID_GET_PROTOCOL              0x03
#define HID_SET_REPORT                0x09
#define HID_SET_IDLE                  0x0A
#define HID_SET_PROTOCOL              0x0B

#define HID_INTRIN_EP                 USB_EP6
#define HID_INTROUT_EP                USB_EP4
#define HID_EP_INTR_PACKET_SIZE       512//64
#define HID_EP_INTERRUPT_INTERVAL     3

/* USB HID Class Request Types */
#define MSDC_IN_EP                    USB_EP7
#define MSDC_OUT_EP                   USB_EP8

#define USB_FS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE        64
#define USB_FS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE        64

#define USB_HS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE        0x0200
#define USB_HS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE        0x0200

#define USB_SS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE        0x0400
#define USB_SS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE        0x0400

/* number of string descriptor */
#define USB_STRG_STR_NUM_STRING             4
#define USB_STRG_IF_CFGA_IF0_NUMBER_EP      2
#define USB_STRG_IF_IF0_NUMBER              0
#define USB_STRG_CFG_A_IF0_ALT_MAX          1
#define USB_STRG_CFGA_NUMBER_IF             1

/* device descriptor */
#define USB_SSSTRG_DEV_RELEASE_NUMBER       0x0310         /* spec. release number             */
#define USB_HSSTRG_DEV_RELEASE_NUMBER       0x0210         /* spec. release number             */
#define USB_FSSTRG_DEV_RELEASE_NUMBER       0x0110         /* spec. release number             */
#define USB2_HSSTRG_DEV_RELEASE_NUMBER      0x0200         /* spec. release number             */
#define USB_STRG_DEV_CLASS                  0              /* class code                       */
#define USB_STRG_DEV_SUBCLASS               0              /* sub class code                   */
#define USB_STRG_DEV_PROTOCOL               0              /* protocol code                    */
#define USB_STRG_DEV_MAX_PACKET_SIZE0       0x40           /* max packet size for endpoint 0   */
#define USB_STRG_VENDOR_ID                  0x0D64         /* vendor id                        */
#define USB_STRG_PRODUCT_ID                 0x6680         /* product id                       */
#define USB_STRG_DEV_RELASE_NUMBER          0x0100         /* device release number            */
#define USB_STRG_DEV_NUM_CONFIG             1              /* number of possible configuration */

/*
    configuration descriptor
*/
#define USB_SS_STRG_CFGA_TOTAL_LENGTH       (USB_CFG_LENGTH + (USB_IF_LENGTH * 1) + ((USB_EP_LENGTH+USB_SSEP_COMPANISON_LENGTH) * 2))
#define USB_STRG_CFGA_TOTAL_LENGTH          ((USB_EP_LENGTH * 2))
/* total length                     */
#define USB_STRG_CFGA_NUMBER_IF             1                   /* number of interfaces             */
#define USB_STRG_CFGA_CFG_VALUE             1                   /* configuration value              */
#define USB_STRG_CFGA_CFG_IDX               0                   /* configuration string id          */

#define USB_STRG_CFGA_CFG_ATTRIBUES_IDX     0x07                /* Attribute index in configuration */
#define USB_STRG_CFGA_CFG_ATTRIBUES_SELF    0xC0                /* characteristics: Self Powered    */
#define USB_STRG_CFGA_CFG_ATTRIBUES_BUS     0x80                /* characteristics: Bus Powered     */

#define USB_STRG_CFGA_MAX_POWER_IDX         0x08                /* maximum power index in configuration*/
#define USB_SS_STRG_CFGA_MAX_POWER_SELF     0x0D                /* 104mA maximum power. Unit in 8mA */
#define USB_STRG_CFGA_MAX_POWER_SELF        0x32                /* 100mA maximum power. Unit in 2mA */
#define USB_STRG_CFGA_MAX_POWER_BUS         0xFA                /* 500mA maximum power. Unit in 2mA */


/*
    Interface Descriptor
*/

/* interface number */
/*---- alternate setting */
#define USB_STRG_IF_ALT0                    0
/* number of endpoint */
/* b_interface_class */
#define USB_STRG_IF_CLASS_STORAGE           8
/* b_interface_subclass */
#define USB_STRG_IF_SUBCLASS_SCSI           6
/* b_interface_protocol */
#define USB_STRG_IF_PROTOCOL_BULK_ONLY      0x50
#define USB_STRG_IF_IDX                     0

/*
    Binary device Object Store (BOS) Descriptor
*/
#define USB_STRG_BOS_LENGTH                 5
#define USB_SS_STRG_BOS_TOTAL_LENGTH        22
#define USB_HS_STRG_BOS_TOTAL_LENGTH        12
#define USB_SS_STRG_BOS_DEVCAP_NUMBER       2
#define USB_HS_STRG_BOS_DEVCAP_NUMBER       1

/* BOS descriptor*/
#define USB_DEVICE_CAPABILITY_DESCRIPTOR_TYPE 0x10
#define USB_DEVICE_CAPABILITY_PLATFORM 0x05
#define UVC_BOS_DESCRIPTOR_LENGTH 0x21

#endif
