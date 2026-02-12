/*++

Copyright (c) 2006  Novatek Microelectronics Corporation

Module Name:

    UsbUVCDesc.c

Abstract:

    The Descriptor of USB Video Class device.

Environment:

    For nt96611

Notes:

  Copyright (c) 2006 Novatek Microelectronics Corporation.  All Rights Reserved.

--*/

#include "U2UvacDesc.h"
#include "U2UvacVideoTsk.h"
#include "UVAC.h"
#include "U2UvacDbg.h"
#include "U2UvacIsoInTsk.h"
#include "umsd.h"

#define YUV_BIT_RATE(w, h, fps) (w*h*2*fps*8)
#define YUV_FRAME_SIZE(w, h) (w*h*2)

UINT8 gU2UvacIntfIdx_WinUsb = UVAC_INTF_IDX_UNKNOWN;
UINT8 gU2UvacIntfIdx_VC[UVAC_VID_DEV_CNT_MAX] = {UVAC_INTF_IDX_UNKNOWN};
UINT8 gU2UvacIntfIdx_VS[UVAC_VID_DEV_CNT_MAX] = {UVAC_INTF_IDX_UNKNOWN};
UINT8 gU2UvacIntfIdx_AC[UVAC_AUD_DEV_CNT_MAX] = {UVAC_INTF_IDX_UNKNOWN};
UINT8 gU2UvacIntfIdx_AS[UVAC_AUD_DEV_CNT_MAX] = {UVAC_INTF_IDX_UNKNOWN};
UINT8 gU2UvacIntfIdx_vidAryCurrIdx = 0; //one video control map single video stream
UINT8 gU2UvacIntfIdx_audAryCurrIdx = 0; //one audio control map single audio stream
UINT8 gU2UvacIntfIdx_CDC_COMM[CDC_COM_MAX_NUM] = {UVAC_INTF_IDX_UNKNOWN};
UINT8 gU2UvacIntfIdx_CDC_DATA[CDC_COM_MAX_NUM] = {UVAC_INTF_IDX_UNKNOWN};

extern UINT32 gU2UvacAudSampleRate[UVAC_AUD_SAMPLE_RATE_MAX_CNT];
extern UINT32 gU2UvcVidResoCnt;
extern UVAC_VID_RESO gU2UvcVidResoAry[UVAC_VID_RESO_MAX_CNT];
extern UINT32 gU2UvcStrmMinBitRate;
extern UINT32 gU2UvcStrmMaxBitRate;
extern UINT32 gU2UvcMJPGMaxTBR;
extern UINT32 gU2UvcCapM3Enable;
extern BOOL gU2UvacWinIntrfEnable;
extern UINT32 gU2UvacChannel;
extern const USB_EP U2UVAC_USB_EP[UVAC_TXF_QUE_MAX];
extern const USB_EP U2UVAC_USB_INTR_EP[UVAC_EP_INTR_MAX];
extern const USB_EP U2UVAC_USB_RX_EP[UVAC_RXF_QUE_MAX];
extern UINT8 gU2UvacAudFmtType;
extern UVAC_VIDEO_FORMAT_TYPE gU2UvcVideoFmtType;
extern BOOL gU2UvacCdcEnabled;
UINT8 gU2UvacIntfIdx_SIDC = UVAC_INTF_IDX_UNKNOWN;
#if 0//gU2UvacMtpEnabled
extern BOOL gU2UvacMtpEnabled;
#endif
extern UINT32 gU2UacChNum;
extern UINT32 gU2UacITOutChCfg;
extern UVAC_VID_RESO_ARY gUvcYuvFrmInfo;
extern UVAC_VID_RESO_ARY gUvcMjpgFrmInfo[UVAC_VID_DEV_CNT_MAX];
extern UVAC_VID_RESO_ARY gUvcH264FrmInfo[UVAC_VID_DEV_CNT_MAX];
UINT8 gUvacIntfIdx_DFU = UVAC_INTF_IDX_UNKNOWN;
extern UVAC_DFU_INFO g_dfu_info;
UINT8 gUvacIntfIdx_HID = UVAC_INTF_IDX_UNKNOWN;
extern UVAC_HID_INFO g_hid_info;
extern BOOL gDisableUac;
extern BOOL gU2UvacUacRxEnabled;
extern UINT16 gUacMaxPacketSize;
extern UINT8 gUacInterval;
UINT8 h264_fmt_index[UVAC_VID_DEV_CNT_MAX] = {0};
UINT8 mjpeg_fmt_index[UVAC_VID_DEV_CNT_MAX] = {0};
UINT8 yuv_fmt_index = 0;
extern UINT32 ct_controls;
extern UINT32 pu_controls;

UINT8 eu_unit_id[2] = {UVC_TERMINAL_ID_EXTE_UNIT, UVC_TERMINAL_ID_EXTE_UNIT};

UVAC_EU_DESC eu_desc[2] = {0};
UINT32 gUvacUvcVer = UVAC_UVC_VER_100;

extern UINT8 uvc_string_idx;
extern UINT8 uac_string_idx;
extern UINT8 dfu_string_idx;
extern UINT8 hid_string_idx;


extern UVAC_MSDC_INFO g_msdc_info;
extern USB_GENERIC_CB g_u2uvac_msdc_device_evt_cb;
extern FP g_u2uvac_msdc_class_req_cb;
/*---- string descriptor index0 is LANGID ----*/
_ALIGNED(4) const UINT8 U2UvacStrDesc0[] = {
	4,                                  /* size of String Descriptor        */
	USB_STRING_DESCRIPTOR_TYPE,         /* String Descriptor type           */
	0x09, 0x04                          /*  Primary/Sub LANGID              */
};

/*---- USB Device Manufacturer string descriptor ----*/
_ALIGNED(4) const UINT8 U2UvacStrDesc1[] = {
	0x14,                              // 10: size of String Descriptor = 20 bytes
	0x03,                              // 03: String Descriptor type
	'N', 0x00, 'O', 0x00, 'V', 0x00, 'A', 0x00, 'T', 0x00, 'E', 0x00, 'K', 0x00, 0x20, 0x00, 0x00, 0x00
};

/*---- USB Device Product string descriptor ----*/
_ALIGNED(4) const UINT8 U2UvacStrDesc2[] = {
	0x14,                               // 16: size of String Descriptor = 20 bytes
	0x03,                               // 03: String Descriptor type
	'N', 0x00, 'T', 0x00, '9', 0x00, '6', 0x00, '5', 0x00, '1', 0x00, '0', 0x00, 0x20, 0x00, 0x00, 0x00
};

/*---- USB Device SerialNumber string descriptor ----*/
_ALIGNED(4) const UINT16 U2UvacStrDesc3[] = {
	0x0320,                                // 20: size of String Descriptor = 32 bytes
	// 03: String Descriptor type
	'9', '6', '5', '1', '0',            // 96510-00000-001
	'0', '0', '0', '0', '0',
	'0', '0', '1',  0,   0
};


/*---- USB Device Product string descriptor ----*/
_ALIGNED(4) const UINT16 U2UvacStrDesc4[] = {
	0x0314,                               // 14: size of String Descriptor = 20 bytes
	// 03: String Descriptor type
	'U', 'S', 'B', ' ', 'I', 'm', 'a',     // USB Camera
	'g', 'e'
};

UINT16 gU2UvacVidEPMaxPackSize[UVAC_VID_EP_PACK_SIZE_MAX_CNT] = {
	(UVC_USB_FIFO_UNIT_SIZE * UVC_USB_DEDICATED_DMA_FIFO_CNT_1),//0x0080
	(UVC_USB_FIFO_UNIT_SIZE * UVC_USB_DEDICATED_DMA_FIFO_CNT_2),//0x0100
	((UVC_USB_FIFO_UNIT_SIZE * UVC_USB_DEDICATED_DMA_FIFO_CNT_2) | ((BLKNUM_DOUBLE - 1) << 11)) //0x0400
};


#define UVAC_H264_GUIDFMT_LEN       16
UINT8 gU2UvacH264GuidFormat[UVAC_H264_GUIDFMT_LEN] = {0x48, 0x32, 0x36, 0x34, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};

#define UVAC_YUV_GUIDFMT_LEN       16
UINT8 gUvacYUVGuidFormat[UVAC_YUV_GUIDFMT_LEN] = {0x59, 0x55, 0x59, 0x32, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};

// ----------------------------------------------------------------------------
//                        UVC device
// ----------------------------------------------------------------------------

_ALIGNED(4)  USB_DEVICE_DESC g_U2UVACDevDesc = {
	//---- device descriptor ----
	USB_DEV_LENGTH,                     /* descriptor size                  */
	USB_DEVICE_DESCRIPTOR_TYPE,         /* descriptor type                  */
	UVAC_SPEC_RELEASE_NUMBER,         /* spec. release number             */
	USB_UVC_DEV_CLASS,                  /* class code                       */
	USB_UVC_DEV_SUBCLASS,               /* sub class code                   */
	USB_UVC_DEV_PROTOCOL,               /* protocol code                    */
	UVAC_DEV_MAX_PACKET_SIZE0,       /* max packet size for endpoint 0   */
	UVAC_VENDOR_ID,                  /* vendor id                        */
	UVAC_PRODUCT_ID,                 /* product id                       */
	UVAC_DEV_RELEASE_NUMBER,          /* device release number            */
	1,                                  /* manifacturer string id           */
	2,                                  /* product string id                */
	3,                                  /* serial number string id          */
	UVAC_DEV_NUM_CONFIG              /* number of possible configuration */
};

_ALIGNED(4) const USB_DEVICE_DESC g_U2UVACDevQualiDesc = {
	USB_DEV_QUALI_LENGTH,
	USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,
	UVAC_SPEC_RELEASE_NUMBER,    /* spec. release number             */
	USB_UVC_DEV_CLASS,             /* class code                       */
	USB_UVC_DEV_SUBCLASS,          /* sub class code                   */
	USB_UVC_DEV_PROTOCOL,          /* protocol code                    */
	UVAC_DEV_MAX_PACKET_SIZE0,  /* max packet size for endpoint 0   */
	UVAC_DEV_NUM_CONFIG,        /* number of possible configuration */
	0                              /*reserved*/
};

_ALIGNED(4) UINT8 g_U2UVACFSConfigDescFBH264[USB_UVC_TOTAL_LENGTH_FB_H264] = {
	//----- [Configuration Descriptor]----- (Length 9)
	USB_CFG_LENGTH,                     // descriptor size
	USB_CONFIGURATION_DESCRIPTOR_TYPE,  // descriptor type
	(USB_UVC_TOTAL_LENGTH_FB_H264) & 0xff,   // total length
	(USB_UVC_TOTAL_LENGTH_FB_H264) >> 8,
	USB_PCC_UVC_NUMBER_IF,              // number of interface
	UVAC_CFGA_CFG_VALUE,             // configuration value
	UVAC_CFGA_CFG_IDX,               // configuration string id
	UVAC_CFGA_CFG_ATTRIBUES,         // characteristics
	UVAC_CFGA_MAX_POWER,             // maximum power in 2mA

	//----- [   IAD Descriptor  ]----- (Length 8)
	USB_UVC_IAD_LENGTH,                 // descriptor size
	USB_IAD_DESCRIPTOR_TYPE,            // descriptor type
	USB_PCC_IAD_FIRST_VID_IF,               // bFirstInterface
	USB_PCC_IAD_IF_COUNT,               // Interface counts
	CC_VIDEO,                           // bFunction class
	SC_VIDEO_INTERFACE_COLLECTION,      // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,              // bFunction Protocol
	UVAC_IAD_STR_INDEX,              // iFunction

	//----- [ Standard VC interface Descriptor ]----- (Length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	UVAC_IF_IF0_NUMBER,              // interface number
	UVAC_IF_ALT0,                    // intfAltSet setting
	UVAC_IF_CFGA_IF0_NUMBER_EP,      // number of endpoint
	CC_VIDEO,                           // bFunction class
	SC_VIDEOCONTROL,                    // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,              // bFunction Protocol
	UVAC_IF_IDX,                     // interface string id

	//----- [Class-spec. VC interface Descriptor ] ----- (length 13)
	USB_UVC_VC_IF_LENGTH,               // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_HEADER,                          // descriptor Subtype
	USB_UVC_RELEASE_NUMBER  & 0xff,     // UVC version
	USB_UVC_RELEASE_NUMBER >> 8,
	USB_UVC_VC_TOTAL_LENGTH & 0xff,     // Total Size of class-spec. descriptor
	USB_UVC_VC_TOTAL_LENGTH >> 8,
	UVC_VC_CLOCK_FREQUENCY & 0xff,      // dwClockFrequency 6MHz (Use of this field has been deprecated)
	(UVC_VC_CLOCK_FREQUENCY >> 8) & 0xff,
	(UVC_VC_CLOCK_FREQUENCY >> 16) & 0xff,
	(UVC_VC_CLOCK_FREQUENCY >> 24) & 0xff,
	UVC_VC_VIDEOSTREAM_TOTAL,           // bInCollection (Number of streaming interfaces)
	UVC_VC_VIDEOSTREAM_FIRSTNUM,        // VS I/F 1 belongs to the VC I/F

	//----- [Camera terminal Descriptor ] ----- (length 17)
	USB_UVC_IT_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_INPUT_TERMINAL,                  // descriptor Subtype
	USB_UVC_ID1,                        // ID of this terminal
	ITT_CAMERA & 0xff,                  // terminal type
	ITT_CAMERA >> 8,
	UVC_IT_ASSOC_TERMINAL,              // no association
	UVC_IT_ITERMINAL_STR_INDEX,             // index of string descriptor (unused)
	UVC_IT_OBJFOCAL_LEN_MIN  & 0xff,    // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN_MIN >> 8,
	UVC_IT_OBJFOCAL_LEN_MAX  & 0xff,    // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN_MAX >> 8,
	UVC_IT_OBJFOCAL_LEN & 0xff,         // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN >> 8,
	UVC_IT_INPUT_CTRL_SIZE,                   // bmControls
	UVC_IT_CONTROLS  & 0xff,            // no controls supported
	UVC_IT_CONTROLS >> 8,
	UVC_IT_CONTROLS >> 16,

	//----- [Output terminal Descriptor ] ----- (length 9)
	USB_UVC_OT_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_OUTPUT_TERMINAL,                 // descriptor Subtype
	USB_UVC_ID2,                        // ID of this terminal
	TT_STREAMING & 0xff,                // terminal type
	TT_STREAMING >> 8,
	UVC_OT_ASSOC_TERMINAL,              // no association
	USB_UVC_ID4,                        // bSourceID
	UVC_OT_ITERMINAL_STR_INDEX,             // index of string descriptor (unused)

	//----- [Select unit Descriptor ] ----- (length 7)
	USB_UVC_SU_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_SELECTOR_UNIT,                   // descriptor Subtype
	USB_UVC_ID3,                        // ID of this terminal
	UVC_SU_PIN_NUMBERS,                 // number of this input pin
	USB_UVC_ID1,                        // Source input 1
	UVC_SU_SELECT_STR_INDEX,                // unused

	//----- [Processing unit Descriptor ] ----- (length 11)
	USB_UVC_PU_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_PROCESSING_UNIT,                 // descriptor Subtype
	USB_UVC_ID4,                        // ID of this unit
	USB_UVC_ID3,                        // Connect to ID x
	UVC_PU_MAX_MULTIPLIER & 0xff,       // unused
	UVC_PU_MAX_MULTIPLIER >> 8,         // unused
	UVC_100_PU_CTRL_SIZE,               // control size
	UVC_PU_CONTROLS  & 0xff,            // no controls supported
	UVC_PU_CONTROLS >> 8,
	UVC_PU_PROCESS_STR_INDEX,            // unused

	//----- [Standard interrupt EP Descriptor ] ----- (length 7)
	USB_BULK_EP_LENGTH,                 // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,       // descriptor type
	USB_EP_EP3I_ADDRESS,                // EP address
	USB_EP_ATR_INTERRUPT,               // transfer type
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff, // max packet size
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,   // max packet size
	USB_EP_INT_INTERVAL,                // interval

	//----- [Class -spec interrupt EP Descriptor ] ----- (length 5)
	USB_UVC_EP_LENGTH,                  // descriptor size
	CS_ENDPOINT,                        // descriptor type
	EP_INTERRUPT,                       // descriptor subtype
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff, // max packet size
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,   // max packet size

	//----- [Standard VS interface Descriptor] ----- (length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(UVAC_IF_IF0_NUMBER + 1),        // interface number
	UVAC_IF_ALT0,                    // intfAltSet setting
	UVC_VS_IF0_NUMBER_EP,
	CC_VIDEO,                           // interface class
	SC_VIDEOSTREAMING,                  // interface subclass
	PC_PROTOCOL_UNDEFINED,              // interface protocol
	UVC_VS_INTERFACE_STR_INDEX,             // interface string id

	//----- [Standard Iso EP Descriptor ] ----- (length 7)
	USB_BULK_EP_LENGTH,                 // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,       // descriptor type
	USB_EP_EP1I_ADDRESS,                // endpoint address
	//#NT#2012/12/07#Lily Kao -begin
	//#NT#IsoIn Txf
	USB_EP_ATR_ISO,                    // transfer mode
	USB_FS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff,    // max packet size
	USB_FS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,    // max packet size
	USB_EP_ISO_INTERVAL,

	//----- [Class spec VS Header input Descriptor ] ----- (length 14)
	USB_UVC_VS_INPUT_HEADER_LENGTH,     // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_INPUT_HEADER,                    // descriptor subtype
	USB_UVC_FORMAT_NUM,                 // how many format descriptors follow
	(USB_UVC_TOTAL_VS_LENGTH_FB_H264) & 0xff, // Total size of VS header + format +frame
	(USB_UVC_TOTAL_VS_LENGTH_FB_H264) >> 8,
	USB_EP_EP1I_ADDRESS,                // Data endpoint address
	UVC_VS_BMINFO,                      // dynamic format change supported (bmInfo)
	USB_UVC_ID2,                        // Ouput terminal ID
	UVC_VS_STILL_CAPTURE,               // StillCapture Method (1,2,3)
	UVC_VS_HW_TRIGGER,                  // HW trigger supported
	UVC_VS_HW_TRIGGER_USAGE,            // HW trigger usage
	UVC_VS_CTRL_SIZE,                   // size of the bmaControls filed
	UVC_VS_CONTROLS,                    // bmaControls

	//----- [Class spec VS Format Descriptor ] -----  (length 11)
	//FRAME-BASED FORMAT
	USB_UVC_VS_FBH264_FORMAT_LENGTH,           // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FORMAT_FRAME_BASED,                    // descriptor subtype  //VS_FORMAT_H264_SIMULCAST
	UVC_VSFMT_FIRSTINDEX,               // First format descriptor
	UVC_VSFMT_FRAMENUM1_HD,             // frame descriptor numbers
	//16 byte GUID
	0x48,
	0x32,
	0x36,
	0x34,
	0x00,
	0x00,
	0x10,
	0x00,
	0x80,
	0x00,
	0x00,
	0xAA,
	0x00,
	0x38,
	0x9B,
	0x71,

	UVC_BITS_PER_PIXEL,             //bBitsPerPixel
	UVC_VSFMT_DEF_FRM_IDX,               /* Default frame index is 1=>1280x720 */
	0x00,                           /* bAspecRatioX */
	0x00,                           /* bAspecRatioY */
	0x00,                           /* bmInterlaceFlags */
	0x00,                           /* bCopyProtect: duplication unrestricted */
	0x01,                           /* bVariableSize */

	//----- [Class spec VS Frame 1280x720 Descriptor ] ----- (length 38)
	//USB_UVC_HD_WIDTH  ; FRAME-BASED FRAME
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX1,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_HD_WIDTH & 0xff,            // wWidth
	USB_UVC_HD_WIDTH >> 8,              // wWidth
	USB_UVC_HD_HEIGHT & 0xff,           // wHeight
	USB_UVC_HD_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 1280x768 Descriptor ] ----- (length 38)
	//USB_UVC_1024x768_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX2,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_1024x768_WIDTH & 0xff,            // wWidth
	USB_UVC_1024x768_WIDTH >> 8,              // wWidth
	USB_UVC_1024x768_HEIGHT & 0xff,           // wHeight
	USB_UVC_1024x768_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 640x480 Descriptor ] ----- (length 38)
	//USB_UVC_VGA_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX3,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_VGA_WIDTH & 0xff,            // wWidth
	USB_UVC_VGA_WIDTH >> 8,              // wWidth
	USB_UVC_VGA_HEIGHT & 0xff,           // wHeight
	USB_UVC_VGA_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 320x240 Descriptor ] ----- (length 38)
	//USB_UVC_QVGA_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX4,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_QVGA_WIDTH & 0xff,            // wWidth
	USB_UVC_QVGA_WIDTH >> 8,              // wWidth
	USB_UVC_QVGA_HEIGHT & 0xff,           // wHeight
	USB_UVC_QVGA_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 1024x576 Descriptor ] ----- (length 38)
	//USB_UVC_1024x576_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX5,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_1024x576_WIDTH & 0xff,            // wWidth
	USB_UVC_1024x576_WIDTH >> 8,              // wWidth
	USB_UVC_1024x576_HEIGHT & 0xff,           // wHeight
	USB_UVC_1024x576_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 800x600 Descriptor ] ----- (length 38)
	//USB_UVC_800x600_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX6,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_800x600_WIDTH & 0xff,            // wWidth
	USB_UVC_800x600_WIDTH >> 8,              // wWidth
	USB_UVC_800x600_HEIGHT & 0xff,           // wHeight
	USB_UVC_800x600_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 800x480 Descriptor ] ----- (length 38)
	//USB_UVC_800x480_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX7,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_800x480_WIDTH & 0xff,            // wWidth
	USB_UVC_800x480_WIDTH >> 8,              // wWidth
	USB_UVC_800x480_HEIGHT & 0xff,           // wHeight
	USB_UVC_800x480_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 640x360 Descriptor ] ----- (length 38)
	//USB_UVC_640x360_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX8,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_640x360_WIDTH & 0xff,            // wWidth
	USB_UVC_640x360_WIDTH >> 8,              // wWidth
	USB_UVC_640x360_HEIGHT & 0xff,           // wHeight
	USB_UVC_640x360_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Color matching Descriptor ] ----- (length 6)
	USB_UVC_VS_COLOR_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_COLORFORMAT,                     // descriptor subtype
	UVC_COLOR_PRIMARIES,
	UVC_COLOR_TRANSFER,
	UVC_COLOR_MATRIX
};
_ALIGNED(4) UINT8 g_U2UVACHSConfigDescFBH264[IsoLenUVCSUAC] = {
	//----- [Configuration Descriptor]----- (Length 9)
	USB_CFG_LENGTH,                     // descriptor size
	USB_CONFIGURATION_DESCRIPTOR_TYPE,  // descriptor type
	IsoLenUVCSUAC & 0xff,
	IsoLenUVCSUAC >> 0x08,
	USB_PCC_UVC_NUMBER_IF,              // number of interface
	UVAC_CFGA_CFG_VALUE,             // configuration value
	UVAC_CFGA_CFG_IDX,               // configuration string id
	UVAC_CFGA_CFG_ATTRIBUES,         // characteristics
	UVAC_CFGA_MAX_POWER,             // maximum power in 2mA

	//----- [   IAD Descriptor  ]----- (Length 8)
	USB_UVC_IAD_LENGTH,                 // descriptor size
	USB_IAD_DESCRIPTOR_TYPE,            // descriptor type
	USB_PCC_IAD_FIRST_VID_IF,               // bFirstInterface
	USB_PCC_IAD_IF_COUNT,           // Interface counts
	CC_VIDEO,                           // bFunction class
	SC_VIDEO_INTERFACE_COLLECTION,      // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,              // bFunction Protocol
	UVAC_IAD_STR_INDEX,              // iFunction

	//----- [ Standard VC interface Descriptor ]----- (Length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	USB_PCC_IAD_FIRST_VID_IF,              // interface number
	UVAC_IF_ALT0,                    // intfAltSet setting
	UVAC_IF_CFGA_IF0_NUMBER_EP,      // number of endpoint
	CC_VIDEO,                           // bFunction class
	SC_VIDEOCONTROL,                    // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,                     // bFunction Protocol
	UVAC_IF_IDX,                     // interface string id

	//----- [Class-spec. VC interface Descriptor ] ----- (length 13)
	USB_UVC_VC_IF_LENGTH,               // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_HEADER,                          // descriptor Subtype
	USB_UVC_RELEASE_NUMBER  & 0xff,     // UVC version
	USB_UVC_RELEASE_NUMBER >> 8,
	USB_UVC_VC_TOTAL_LENGTH & 0xff,     // Total Size of class-spec. descriptor
	USB_UVC_VC_TOTAL_LENGTH >> 8,
	UVC_VC_CLOCK_FREQUENCY & 0xff,      // dwClockFrequency 6MHz (Use of this field has been deprecated)
	(UVC_VC_CLOCK_FREQUENCY >> 8) & 0xff,
	(UVC_VC_CLOCK_FREQUENCY >> 16) & 0xff,
	(UVC_VC_CLOCK_FREQUENCY >> 24) & 0xff,
	UVC_VC_VIDEOSTREAM_TOTAL,           // bInCollection (Number of streaming interfaces)
	UVC_VC_VIDEOSTREAM_FIRSTNUM,        // VS I/F 1 belongs to the VC I/F

	//----- [Camera Input terminal Descriptor ] ----- (length 17)
	USB_UVC_IT_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_INPUT_TERMINAL,                  // descriptor Subtype
	USB_UVC_ID1,                        // ID of this terminal
	ITT_CAMERA & 0xff,                  // terminal type
	ITT_CAMERA >> 8,
	UVC_IT_ASSOC_TERMINAL,              // no association
	UVC_IT_ITERMINAL_STR_INDEX,             // index of string descriptor (unused)
	UVC_IT_OBJFOCAL_LEN_MIN  & 0xff,    // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN_MIN >> 8,
	UVC_IT_OBJFOCAL_LEN_MAX  & 0xff,    // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN_MAX >> 8,
	UVC_IT_OBJFOCAL_LEN & 0xff,         // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN >> 8,
	UVC_IT_INPUT_CTRL_SIZE,                   // bmControls
	UVC_IT_CONTROLS  & 0xff,            // no controls supported
	UVC_IT_CONTROLS >> 8,

	//----- [Output terminal Descriptor ] ----- (length 9)
	USB_UVC_OT_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_OUTPUT_TERMINAL,                 // descriptor Subtype
	USB_UVC_ID2,                        // ID of this terminal
	TT_STREAMING & 0xff,                // terminal type
	TT_STREAMING >> 8,
	UVC_OT_ASSOC_TERMINAL,              // no association
	USB_UVC_ID4,                        // bSourceID
	UVC_OT_ITERMINAL_STR_INDEX,             // index of string descriptor (unused)

	//----- [Select unit Descriptor ] ----- (length 7)
	USB_UVC_SU_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_SELECTOR_UNIT,                   // descriptor Subtype
	USB_UVC_ID3,                        // ID of this terminal
	UVC_SU_PIN_NUMBERS,                 // number of this input pin
	USB_UVC_ID1,                        // Source input 1
	UVC_SU_SELECT_STR_INDEX,                // unused

	//----- [Processing unit Descriptor ] ----- (length 11)
	USB_UVC_PU_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_PROCESSING_UNIT,                 // descriptor Subtype
	USB_UVC_ID4,                        // ID of this unit
	USB_UVC_ID3,                        // Connect to ID x
	UVC_PU_MAX_MULTIPLIER & 0xff,       // unused
	UVC_PU_MAX_MULTIPLIER >> 8,         // unused
	UVC_PU_CTRL_SIZE,                   // control size
	UVC_PU_CONTROLS  & 0xff,            // no controls supported
	UVC_PU_CONTROLS >> 8,
	UVC_PU_PROCESS_STR_INDEX,            // unused

#if 1//(UVAC_UVC_UAC_DEV_SET == UVAC_1UVC_1UAC)
	//----- [Extension unit Descriptor ] ----- (length )
	USB_UVC_XU_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_EXTENSION_UNIT,                  // descriptor Subtype
	USB_UVC_XU_ID,                        // ID of this unit
	USB_UVC_XU_GUID1 & 0xFF,            // GUID = // {c987a729-59d3-4569-8467-ff0849fc19e8}  16 bytes
	(USB_UVC_XU_GUID1 >> 8) & 0xFF,
	(USB_UVC_XU_GUID1 >> 16) & 0xFF,
	(USB_UVC_XU_GUID1 >> 24) & 0xFF,
	USB_UVC_XU_GUID2 & 0xFF,
	(USB_UVC_XU_GUID2 >> 8) & 0xFF,
	(USB_UVC_XU_GUID2 >> 16) & 0xFF,
	(USB_UVC_XU_GUID2 >> 24) & 0xFF,
	USB_UVC_XU_GUID3 & 0xFF,
	(USB_UVC_XU_GUID3 >> 8) & 0xFF,
	(USB_UVC_XU_GUID3 >> 16) & 0xFF,
	(USB_UVC_XU_GUID3 >> 24) & 0xFF,
	USB_UVC_XU_GUID4 & 0xFF,
	(USB_UVC_XU_GUID4 >> 8) & 0xFF,
	(USB_UVC_XU_GUID4 >> 16) & 0xFF,
	(USB_UVC_XU_GUID4 >> 24) & 0xFF,
	UVC_XU_CONTROL_NUMS,                // number of controls
	UVC_XU_INPUT_PINS,                  // number of input pins
	USB_UVC_ID4,                        // baSourceID: Processing unit
	UVC_XU_CONTROLSIZE,                 // bControlSize: unit: in bytes
	UVC_XU_BMCONTROLS & 0xFF,           // bmControls: set 1 to info vendor-spec
	(UVC_XU_BMCONTROLS >> 8) & 0xFF,    // bmControls: set 1 to info vendor-spec
	UVC_XU_STR_INDEX,                       // index of string descriptor (unused)
#endif

#if (IsoLen2UVC2UAC_INTR == ENABLE)
	//----- [Standard interrupt EP Descriptor ] ----- (length 7)
	USB_BULK_EP_LENGTH,                 // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,       // descriptor type
	USB_EP_EP5I_ADDRESS,                // EP address
	USB_EP_ATR_INTERRUPT,               // transfer type
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff, // max packet size
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,   // max packet size
	USB_EP_INT_INTERVAL,                // interval

	//----- [Class -spec interrupt EP Descriptor ] ----- (length 5)
	USB_UVC_EP_LENGTH,                  // descriptor size
	CS_ENDPOINT,                        // descriptor type
	EP_INTERRUPT,                       // descriptor subtype
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff, // max packet size
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,   // max packet size
#endif

	//----- [Standard VS interface Descriptor] ----- (length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_FIRST_VID_IF + 1),        // interface number
	UVAC_IF_ALT0,                    // intfAltSet setting
	0,                                  // number of endpoint
	CC_VIDEO,                           // interface class
	SC_VIDEOSTREAMING,                  // interface subclass
	PC_PROTOCOL_UNDEFINED,                     // interface protocol
	UVC_VS_INTERFACE_STR_INDEX,             // interface string id

	//----- [Class spec VS Header input Descriptor ] ----- (length 14)
	USB_UVC_VS_INPUT_HEADER_LENGTH,//USB_UVC_VS_INPUT_HEADER_LENGTH,     // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_INPUT_HEADER,                    // descriptor subtype
	USB_UVC_FORMAT_NUM2,                 // how many format descriptors follow
	(USB_UVC_TOTAL_VS_LENGTH_FB_H264_MJPG_NEW) & 0xff, // Total size of VS header + format +frame
	(USB_UVC_TOTAL_VS_LENGTH_FB_H264_MJPG_NEW) >> 8,
	USB_EP_EP1I_ADDRESS,
	UVC_VS_BMINFO,                      // dynamic format change supported (bmInfo)
	USB_UVC_ID2,                        // Ouput terminal ID
	UVC_VS_STILL_CAPTURE,               // StillCapture Method (1,2,3)
	UVC_VS_HW_TRIGGER,                  // HW trigger supported
	UVC_VS_HW_TRIGGER_USAGE,            // HW trigger usage
	UVC_VS_CTRL_SIZE,                   // size of the bmaControls filed
	UVC_VS_CONTROLS,                    // bmaControls
	UVC_VS_CONTROLS,                    // bmaControls

	//----- [Class spec VS Format Descriptor ] -----  (length 28)
	//FRAME-BASED FORMAT
	USB_UVC_VS_FBH264_FORMAT_LENGTH,           // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FORMAT_FRAME_BASED,                    // descriptor subtype  //VS_FORMAT_H264_SIMULCAST
	UVC_VSFMT_FIRSTINDEX,               // First format descriptor
	UVC_VSFMT_FRAMENUM1_HD,             // frame descriptor numbers
	//16 byte GUID
	0x48,
	0x32,
	0x36,
	0x34,
	0x00,
	0x00,
	0x10,
	0x00,
	0x80,
	0x00,
	0x00,
	0xAA,
	0x00,
	0x38,
	0x9B,
	0x71,

	UVC_BITS_PER_PIXEL,             //bBitsPerPixel
	UVC_VSFMT_DEF_FRAMEINDEX,               /* Default frame index is 1=>1280x720 */
	UVC_VSFMT_ASPECTRATIO_X,                           /* bAspecRatioX */
	UVC_VSFMT_ASPECTRATIO_Y,                           /* bAspecRatioY */
	UVC_VSFMT_INTERLACE,                           /* bmInterlaceFlags */
	UVC_VSFMT_COPYPROTECT,                           /* bCopyProtect: duplication unrestricted */
	0x01,                           /* bVariableSize */

	//----- [Class spec VS Frame 1280x720 Descriptor ] ----- (length 38)
	//USB_UVC_HD_WIDTH  ; FRAME-BASED FRAME
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX1,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_HD_WIDTH & 0xff,            // wWidth
	USB_UVC_HD_WIDTH >> 8,              // wWidth
	USB_UVC_HD_HEIGHT & 0xff,           // wHeight
	USB_UVC_HD_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate


	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 1280x768 Descriptor ] ----- (length 38)
	//USB_UVC_1024x768_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX2,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_1024x768_WIDTH & 0xff,            // wWidth
	USB_UVC_1024x768_WIDTH >> 8,              // wWidth
	USB_UVC_1024x768_HEIGHT & 0xff,           // wHeight
	USB_UVC_1024x768_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,


	//----- [Class spec VS Frame 640x480 Descriptor ] ----- (length 38)
	//USB_UVC_VGA_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX3,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_VGA_WIDTH & 0xff,            // wWidth
	USB_UVC_VGA_WIDTH >> 8,              // wWidth
	USB_UVC_VGA_HEIGHT & 0xff,           // wHeight
	USB_UVC_VGA_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 320x240 Descriptor ] ----- (length 38)
	//USB_UVC_QVGA_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX4,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_QVGA_WIDTH & 0xff,            // wWidth
	USB_UVC_QVGA_WIDTH >> 8,              // wWidth
	USB_UVC_QVGA_HEIGHT & 0xff,           // wHeight
	USB_UVC_QVGA_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 1024x576 Descriptor ] ----- (length 38)
	//USB_UVC_1024x576_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX5,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_1024x576_WIDTH & 0xff,            // wWidth
	USB_UVC_1024x576_WIDTH >> 8,              // wWidth
	USB_UVC_1024x576_HEIGHT & 0xff,           // wHeight
	USB_UVC_1024x576_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 800x600 Descriptor ] ----- (length 38)
	//USB_UVC_800x600_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX6,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_800x600_WIDTH & 0xff,            // wWidth
	USB_UVC_800x600_WIDTH >> 8,              // wWidth
	USB_UVC_800x600_HEIGHT & 0xff,           // wHeight
	USB_UVC_800x600_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 800x480 Descriptor ] ----- (length 38)
	//USB_UVC_800x480_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX7,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_800x480_WIDTH & 0xff,            // wWidth
	USB_UVC_800x480_WIDTH >> 8,              // wWidth
	USB_UVC_800x480_HEIGHT & 0xff,           // wHeight
	USB_UVC_800x480_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 1920x1080 Descriptor ] ----- (length 38)
	//USB_UVC_1920x1080_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX8,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_FULL_HD_WIDTH & 0xff,            // wWidth
	USB_UVC_FULL_HD_WIDTH >> 8,              // wWidth
	USB_UVC_FULL_HD_HEIGHT & 0xff,           // wHeight
	USB_UVC_FULL_HD_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate
	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)

	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine

	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Color matching Descriptor ] ----- (length 6)
	USB_UVC_VS_COLOR_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_COLORFORMAT,                     // descriptor subtype
	UVC_COLOR_PRIMARIES,
	UVC_COLOR_TRANSFER,
	UVC_COLOR_MATRIX,

	//----- [Class spec VS Format Descriptor ] -----  (length 11)
	//MJPG
	USB_UVC_VS_MJPG_FORMAT_LENGTH,           // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FORMAT_MJPEG,                    // descriptor subtype
	UVC_VSFMT_SECONDINDEX,//UVC_VSFMT_FIRSTINDEX,//UVC_VSFMT_SECONDINDEX,               // First format descriptor
	UVC_VSFMT_FRAMENUM_MJPG,             // frame descriptor numbers
	UVC_VSFMT_BMINFO,                   // fix size samples
	UVC_VSFMT_DEF_FRAMEINDEX,           // default frame index
	UVC_VSFMT_ASPECTRATIO_X,            // Aspect ratio X
	UVC_VSFMT_ASPECTRATIO_Y,            // Aspect ratio Y
	UVC_VSFMT_INTERLACE,                // bmInterlaceFlags
	UVC_VSFMT_COPYPROTECT,              // bCopyProtect

	//----- [Class spec VS Frame 1280x720 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX1,                   // frame index
	UVC_VSFRM_INDEX1_CAP,               // frame capabilities
	USB_UVC_HD_WIDTH & 0xff,            // Frame width
	USB_UVC_HD_WIDTH >> 8,              // Frame width
	USB_UVC_HD_HEIGHT & 0xff,           // Frame height
	USB_UVC_HD_HEIGHT >> 8,             // Frame height
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,


	//----- [Class spec VS Frame 1024x768 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX2,                   // frame index
	UVC_VSFRM_INDEX2_CAP,               // frame capabilities
	USB_UVC_1024x768_WIDTH & 0xff,            // Frame width
	USB_UVC_1024x768_WIDTH >> 8,              // Frame width
	USB_UVC_1024x768_HEIGHT & 0xff,           // Frame height
	USB_UVC_1024x768_HEIGHT >> 8,             // Frame height
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 640x480 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX3,                   // frame index
	UVC_VSFRM_INDEX3_CAP,               // frame capabilities
	USB_UVC_VGA_WIDTH & 0xff,           //Frame width
	USB_UVC_VGA_WIDTH >> 8,             //Frame width
	USB_UVC_VGA_HEIGHT & 0xff,          //Frame height
	USB_UVC_VGA_HEIGHT >> 8,            //Frame height
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 320x240 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX4,                   // frame index
	UVC_VSFRM_INDEX4_CAP,               // frame capabilities
	USB_UVC_QVGA_WIDTH & 0xff,          // Frame width
	USB_UVC_QVGA_WIDTH >> 8,            // Frame width
	USB_UVC_QVGA_HEIGHT & 0xff,         // Frame height
	USB_UVC_QVGA_HEIGHT >> 8,           // Frame height
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 1024x576 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX5,                   // frame index
	UVC_VSFRM_INDEX5_CAP,               // frame capabilities
	USB_UVC_1024x576_WIDTH & 0xff,          // Frame width
	USB_UVC_1024x576_WIDTH >> 8,            // Frame width
	USB_UVC_1024x576_HEIGHT & 0xff,         // Frame height
	USB_UVC_1024x576_HEIGHT >> 8,           // Frame height
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // ( Default frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // ( Min frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // (Max frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 800x600 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX6,                   // frame index
	UVC_VSFRM_INDEX6_CAP,               // frame capabilities
	USB_UVC_800x600_WIDTH & 0xff,          // Frame width
	USB_UVC_800x600_WIDTH >> 8,            // Frame width
	USB_UVC_800x600_HEIGHT & 0xff,         // Frame height
	USB_UVC_800x600_HEIGHT >> 8,           // Frame height
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // ( Default frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // ( Min frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // (Max frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 800x480 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX7,                   // frame index
	UVC_VSFRM_INDEX7_CAP,               // frame capabilities
	USB_UVC_800x480_WIDTH & 0xff,          // Frame width
	USB_UVC_800x480_WIDTH >> 8,            // Frame width
	USB_UVC_800x480_HEIGHT & 0xff,         // Frame height
	USB_UVC_800x480_HEIGHT >> 8,           // Frame height
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // ( Default frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // ( Min frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // (Max frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 640x360 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX8,                   // frame index
	UVC_VSFRM_INDEX8_CAP,               // frame capabilities
	USB_UVC_FULL_HD_WIDTH & 0xff,            // wWidth
	USB_UVC_FULL_HD_WIDTH >> 8,              // wWidth
	USB_UVC_FULL_HD_HEIGHT & 0xff,           // wHeight
	USB_UVC_FULL_HD_HEIGHT >> 8,             // wHeight
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // ( Default frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // ( Min frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // (Max frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec Still Image Frame 4320x3240 Descriptor ] ----- (length 38)
	//6M: 3264x2448
#if 1 //(ENABLE == UVC_METHOD3)
	UVC_M3_CAP_FRAME_DESC_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_STILL_IMAGE_FRAME,                     // descriptor subtype
	USB_EP_EP7I_ADDRESS,                   //EP
	UVC_M3_IMG_SIZE_PATT_CNT,               // number of image pattern
	UVC_M3_IMG_14M_WIDTH & 0xff,            // wWidth
	UVC_M3_IMG_14M_WIDTH >> 8,              // wWidth
	UVC_M3_IMG_14M_HEIGHT & 0xff,           // wHeight
	UVC_M3_IMG_14M_HEIGHT >> 8,             // wHeight
	UVC_M3_IMG_12M_WIDTH & 0xff,            // wWidth
	UVC_M3_IMG_12M_WIDTH >> 8,              // wWidth
	UVC_M3_IMG_12M_HEIGHT & 0xff,           // wHeight
	UVC_M3_IMG_12M_HEIGHT >> 8,             // wHeight
	UVC_M3_IMG_8M_WIDTH & 0xff,            // wWidth
	UVC_M3_IMG_8M_WIDTH >> 8,              // wWidth
	UVC_M3_IMG_8M_HEIGHT & 0xff,           // wHeight
	UVC_M3_IMG_8M_HEIGHT >> 8,             // wHeight
	UVC_M3_IMG_COMPRESS_CNT,
#endif

	//----- [Color matching Descriptor ] ----- (length 6)
	USB_UVC_VS_COLOR_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_COLORFORMAT,                     // descriptor subtype
	UVC_COLOR_PRIMARIES,
	UVC_COLOR_TRANSFER,
	UVC_COLOR_MATRIX,

#if 1 //(ENABLE == UVC_METHOD3)
	//----- [Standard BULK EP Descriptor ] ----- (length 7)
	// 0x07, 0x05, 0x87, 0x02, 0x00, 0x02, 0x00
	//---- EP7 IN ----
	USB_BULK_EP_LENGTH,                 /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,       /* descriptor type      */
	USB_EP_EP7I_ADDRESS,                /* endpoint address     */
	USB_EP_ATR_BULK,                    /* character address    */
	UVC_HS_BULKIN_BLKSIZE_LBYTE,        /* max packet size      */
	UVC_HS_BULKIN_BLKSIZE_HBYTE,        /* max packet size      */
	USB_EP_BULK_INTERVAL,
#endif

	//----- [Standard VS interface Descriptor] ----- (length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_FIRST_VID_IF + 1),        // interface number
	UVAC_IF_ALT1,                    // intfAltSet setting
	UVC_VS_IF0_NUMBER_EP,               // number of endpoint
	CC_VIDEO,                           // interface class
	SC_VIDEOSTREAMING,                  // interface subclass
	PC_PROTOCOL_UNDEFINED,              // interface protocol
	UVC_VS_INTERFACE_STR_INDEX,             // interface string id

	//----- [Standard Iso EP Descriptor ] ----- (length 7)
	USB_BULK_EP_LENGTH,                 // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,       // descriptor type
	USB_EP_EP1I_ADDRESS,                // endpoint address
	USB_EP_ATR_ISO | USB_EP_ATR_ASYNC,  // attributes
	0x80,    // max packet size
	0x00,     // max packet size
	USB_EP_ISO_INTERVAL,               // interval

	//----- [Standard VS interface Descriptor] ----- (length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_FIRST_VID_IF + 1),        // interface number
	UVAC_IF_ALT2,                    // intfAltSet setting
	UVC_VS_IF0_NUMBER_EP,               // number of endpoint
	CC_VIDEO,                           // interface class
	SC_VIDEOSTREAMING,                  // interface subclass
	PC_PROTOCOL_UNDEFINED,              // interface protocol
	UVC_VS_INTERFACE_STR_INDEX,             // interface string id

	//----- [Standard Iso EP Descriptor ] ----- (length 7)
	USB_BULK_EP_LENGTH,                 // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,       // descriptor type
	USB_EP_EP1I_ADDRESS,                // endpoint address
	USB_EP_ATR_ISO | USB_EP_ATR_ASYNC,  // attributes
	0x00,    // max packet size
	0x01,     // max packet size
	USB_EP_ISO_INTERVAL,

	//----- [Standard VS interface Descriptor] ----- (length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_FIRST_VID_IF + 1),        // interface number
	UVAC_IF_ALT3,                    // intfAltSet setting
	UVC_VS_IF0_NUMBER_EP,               // number of endpoint
	CC_VIDEO,                           // interface class
	SC_VIDEOSTREAMING,                  // interface subclass
	PC_PROTOCOL_UNDEFINED,              // interface protocol
	UVC_VS_INTERFACE_STR_INDEX,             // interface string id

	//----- [Standard Iso EP Descriptor ] ----- (length 7)
	USB_BULK_EP_LENGTH,                 // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,       // descriptor type
	USB_EP_EP1I_ADDRESS,                // endpoint address
	USB_EP_ATR_ISO | USB_EP_ATR_ASYNC,  // attributes
	USB_HS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff,    // max packet size
	USB_HS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 0x08,     // max packet size
	USB_EP_ISO_INTERVAL,                // interval


#if 1 //(UVAC_UVC_UAC_DEV_SET > UVAC_1UVC_1UAC)
	//----- [   IAD Descriptor  ]----- (Length 8)
	USB_UVC_IAD_LENGTH,                 // descriptor size
	USB_IAD_DESCRIPTOR_TYPE,            // descriptor type
	USB_PCC_IAD_SECOND_VID_IF,
	USB_PCC_IAD_IF_COUNT,               // Interface counts
	CC_VIDEO,                           // bFunction class
	SC_VIDEO_INTERFACE_COLLECTION,      // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,              // bFunction Protocol
	UVAC_IAD_STR_INDEX + 2,            // iFunction

	//----- [ Standard VC interface Descriptor ]----- (Length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	USB_PCC_IAD_SECOND_VID_IF,         // interface number
	UVAC_IF_ALT0,                    // intfAltSet setting
	UVAC_IF_CFGA_IF0_NUMBER_EP,      // number of endpoint
	CC_VIDEO,                           // bFunction class
	SC_VIDEOCONTROL,                    // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,                     // bFunction Protocol
	UVAC_IF_IDX + 2,                   // interface string id

	//----- [Class-spec. VC interface Descriptor ] ----- (length 13)
	USB_UVC_VC_IF_LENGTH,               // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_HEADER,                          // descriptor Subtype
	USB_UVC_RELEASE_NUMBER  & 0xff,     // UVC version
	USB_UVC_RELEASE_NUMBER >> 8,
	USB_UVC_VC_TOTAL_LENGTH & 0xff,     // Total Size of class-spec. descriptor
	USB_UVC_VC_TOTAL_LENGTH >> 8,
	UVC_VC_CLOCK_FREQUENCY & 0xff,      // dwClockFrequency 6MHz (Use of this field has been deprecated)
	(UVC_VC_CLOCK_FREQUENCY >> 8) & 0xff,
	(UVC_VC_CLOCK_FREQUENCY >> 16) & 0xff,
	(UVC_VC_CLOCK_FREQUENCY >> 24) & 0xff,
	UVC_VC_VIDEOSTREAM_TOTAL,           // bInCollection (Number of streaming interfaces)
	UVC_VC_VIDEOSTREAM_SECONDNUM,        // VS I/F 1 belongs to the VC I/F

	//----- [Camera Input terminal Descriptor ] ----- (length 17)
	USB_UVC_IT_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_INPUT_TERMINAL,                  // descriptor Subtype
	USB_UVC_ID1,                        // ID of this terminal
	ITT_CAMERA & 0xff,                  // terminal type
	ITT_CAMERA >> 8,
	UVC_IT_ASSOC_TERMINAL,              // no association
	UVC_IT_ITERMINAL_STR_INDEX,             // index of string descriptor (unused)
	UVC_IT_OBJFOCAL_LEN_MIN  & 0xff,    // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN_MIN >> 8,
	UVC_IT_OBJFOCAL_LEN_MAX  & 0xff,    // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN_MAX >> 8,
	UVC_IT_OBJFOCAL_LEN & 0xff,         // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN >> 8,
	UVC_IT_INPUT_CTRL_SIZE,                   // bmControls
	UVC_IT_CONTROLS  & 0xff,            // no controls supported
	UVC_IT_CONTROLS >> 8,

	//----- [Output terminal Descriptor ] ----- (length 9)
	USB_UVC_OT_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_OUTPUT_TERMINAL,                 // descriptor Subtype
	USB_UVC_ID2,                        // ID of this terminal
	TT_STREAMING & 0xff,                // terminal type
	TT_STREAMING >> 8,
	UVC_OT_ASSOC_TERMINAL,              // no association
	USB_UVC_ID4,                        // bSourceID
	UVC_OT_ITERMINAL_STR_INDEX,             // index of string descriptor (unused)

	//----- [Select unit Descriptor ] ----- (length 7)
	USB_UVC_SU_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_SELECTOR_UNIT,                   // descriptor Subtype
	USB_UVC_ID3,                        // ID of this terminal
	UVC_SU_PIN_NUMBERS,                 // number of this input pin
	USB_UVC_ID1,                        // Source input 1
	UVC_SU_SELECT_STR_INDEX,                // unused

	//----- [Processing unit Descriptor ] ----- (length 11)
	USB_UVC_PU_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_PROCESSING_UNIT,                 // descriptor Subtype
	USB_UVC_ID4,                        // ID of this unit
	USB_UVC_ID3,                        // Connect to ID x
	UVC_PU_MAX_MULTIPLIER & 0xff,       // unused
	UVC_PU_MAX_MULTIPLIER >> 8,         // unused
	UVC_PU_CTRL_SIZE,                   // control size
	UVC_PU_CONTROLS  & 0xff,            // no controls supported
	UVC_PU_CONTROLS >> 8,
	UVC_PU_PROCESS_STR_INDEX,            // unused

#if 1//(UVAC_UVC_UAC_DEV_SET == UVAC_1UVC_1UAC)
	//----- [Extension unit Descriptor ] ----- (length )
	USB_UVC_XU_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_EXTENSION_UNIT,                  // descriptor Subtype
	USB_UVC_XU_ID,                        // ID of this unit
	USB_UVC_XU_GUID1 & 0xFF,            // GUID = // {c987a729-59d3-4569-8467-ff0849fc19e8}  16 bytes
	(USB_UVC_XU_GUID1 >> 8) & 0xFF,
	(USB_UVC_XU_GUID1 >> 16) & 0xFF,
	(USB_UVC_XU_GUID1 >> 24) & 0xFF,
	USB_UVC_XU_GUID2 & 0xFF,
	(USB_UVC_XU_GUID2 >> 8) & 0xFF,
	(USB_UVC_XU_GUID2 >> 16) & 0xFF,
	(USB_UVC_XU_GUID2 >> 24) & 0xFF,
	USB_UVC_XU_GUID3 & 0xFF,
	(USB_UVC_XU_GUID3 >> 8) & 0xFF,
	(USB_UVC_XU_GUID3 >> 16) & 0xFF,
	(USB_UVC_XU_GUID3 >> 24) & 0xFF,
	USB_UVC_XU_GUID4 & 0xFF,
	(USB_UVC_XU_GUID4 >> 8) & 0xFF,
	(USB_UVC_XU_GUID4 >> 16) & 0xFF,
	(USB_UVC_XU_GUID4 >> 24) & 0xFF,
	UVC_XU_CONTROL_NUMS,                // number of controls
	UVC_XU_INPUT_PINS,                  // number of input pins
	USB_UVC_ID4,                        // baSourceID: Processing unit
	UVC_XU_CONTROLSIZE,                 // bControlSize: unit: in bytes
	UVC_XU_BMCONTROLS & 0xFF,           // bmControls: set 1 to info vendor-spec
	(UVC_XU_BMCONTROLS >> 8) & 0xFF,    // bmControls: set 1 to info vendor-spec
	UVC_XU_STR_INDEX,                       // index of string descriptor (unused)
#endif

#if (IsoLen2UVC2UAC_INTR == ENABLE)
	//----- [Standard interrupt EP Descriptor ] ----- (length 7)
	USB_BULK_EP_LENGTH,                 // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,       // descriptor type
	USB_EP_EP6I_ADDRESS,                // EP address
	USB_EP_ATR_INTERRUPT,               // transfer type
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff, // max packet size
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,   // max packet size
	USB_EP_INT_INTERVAL,                // interval

	//----- [Class -spec interrupt EP Descriptor ] ----- (length 5)
	USB_UVC_EP_LENGTH,                  // descriptor size
	CS_ENDPOINT,                        // descriptor type
	EP_INTERRUPT,                       // descriptor subtype
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff, // max packet size
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,   // max packet size
#endif

	//----- [Standard VS interface Descriptor] ----- (length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_SECOND_VID_IF + 1),        // interface number
	UVAC_IF_ALT0,                    // intfAltSet setting
	0,                                  // number of endpoint
	CC_VIDEO,                           // interface class
	SC_VIDEOSTREAMING,                  // interface subclass
	PC_PROTOCOL_UNDEFINED,                     // interface protocol
	UVC_VS_INTERFACE_STR_INDEX,             // interface string id

	//----- [Class spec VS Header input Descriptor ] ----- (length 14)
	USB_UVC_VS_INPUT_HEADER_LENGTH,     // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_INPUT_HEADER,                    // descriptor subtype
	USB_UVC_FORMAT_NUM2,                 // how many format descriptors follow
	(USB_UVC_TOTAL_VS_LENGTH_FB_H264_MJPG_NEW) & 0xff, // Total size of VS header + format +frame
	(USB_UVC_TOTAL_VS_LENGTH_FB_H264_MJPG_NEW) >> 8,
	USB_EP_EP2I_ADDRESS,                // Data endpoint address
	UVC_VS_BMINFO,                      // dynamic format change supported (bmInfo)
	USB_UVC_ID2,                        // Ouput terminal ID
	UVC_VS_STILL_CAPTURE,               // StillCapture Method (1,2,3)
	UVC_VS_HW_TRIGGER,                  // HW trigger supported
	UVC_VS_HW_TRIGGER_USAGE,            // HW trigger usage
	UVC_VS_CTRL_SIZE,                   // size of the bmaControls filed
	UVC_VS_CONTROLS,                    // bmaControls
	UVC_VS_CONTROLS,                    // bmaControls

	//----- [Class spec VS Format Descriptor ] -----  (length 11)
	//MJPG
	USB_UVC_VS_MJPG_FORMAT_LENGTH,           // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FORMAT_MJPEG,                    // descriptor subtype
	UVC_VSFMT_FIRSTINDEX,//UVC_VSFMT_SECONDINDEX,               // First format descriptor
	UVC_VSFMT_FRAMENUM_MJPG,             // frame descriptor numbers
	UVC_VSFMT_BMINFO,                   // fix size samples
	UVC_VSFRM_INDEX3,//UVC_VSFRM_INDEX4,//UVC_VSFMT_DEF_FRAMEINDEX,           // default frame index
	UVC_VSFMT_ASPECTRATIO_X,            // Aspect ratio X
	UVC_VSFMT_ASPECTRATIO_Y,            // Aspect ratio Y
	UVC_VSFMT_INTERLACE,                // bmInterlaceFlags
	UVC_VSFMT_COPYPROTECT,              // bCopyProtect

	//----- [Class spec VS Frame 1280x720 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX1,                   // frame index
	UVC_VSFRM_INDEX1_CAP,               // frame capabilities
	USB_UVC_HD_WIDTH & 0xff,            // Frame width
	USB_UVC_HD_WIDTH >> 8,              // Frame width
	USB_UVC_HD_HEIGHT & 0xff,           // Frame height
	USB_UVC_HD_HEIGHT >> 8,             // Frame height
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,


	//----- [Class spec VS Frame 1024x768 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX2,                   // frame index
	UVC_VSFRM_INDEX2_CAP,               // frame capabilities
	USB_UVC_1024x768_WIDTH & 0xff,            // Frame width
	USB_UVC_1024x768_WIDTH >> 8,              // Frame width
	USB_UVC_1024x768_HEIGHT & 0xff,           // Frame height
	USB_UVC_1024x768_HEIGHT >> 8,             // Frame height
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 640x480 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX3,                   // frame index
	UVC_VSFRM_INDEX3_CAP,               // frame capabilities
	USB_UVC_VGA_WIDTH & 0xff,           //Frame width
	USB_UVC_VGA_WIDTH >> 8,             //Frame width
	USB_UVC_VGA_HEIGHT & 0xff,          //Frame height
	USB_UVC_VGA_HEIGHT >> 8,            //Frame height
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 320x240 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX4,                   // frame index
	UVC_VSFRM_INDEX4_CAP,               // frame capabilities
	USB_UVC_QVGA_WIDTH & 0xff,          // Frame width
	USB_UVC_QVGA_WIDTH >> 8,            // Frame width
	USB_UVC_QVGA_HEIGHT & 0xff,         // Frame height
	USB_UVC_QVGA_HEIGHT >> 8,           // Frame height
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 1024x576 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX5,                   // frame index
	UVC_VSFRM_INDEX5_CAP,               // frame capabilities
	USB_UVC_1024x576_WIDTH & 0xff,          // Frame width
	USB_UVC_1024x576_WIDTH >> 8,            // Frame width
	USB_UVC_1024x576_HEIGHT & 0xff,         // Frame height
	USB_UVC_1024x576_HEIGHT >> 8,           // Frame height
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // ( Default frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // ( Min frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // (Max frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 800x600 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX6,                   // frame index
	UVC_VSFRM_INDEX6_CAP,               // frame capabilities
	USB_UVC_800x600_WIDTH & 0xff,          // Frame width
	USB_UVC_800x600_WIDTH >> 8,            // Frame width
	USB_UVC_800x600_HEIGHT & 0xff,         // Frame height
	USB_UVC_800x600_HEIGHT >> 8,           // Frame height
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // ( Default frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // ( Min frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // (Max frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 800x480 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX7,                   // frame index
	UVC_VSFRM_INDEX7_CAP,               // frame capabilities
	USB_UVC_800x480_WIDTH & 0xff,          // Frame width
	USB_UVC_800x480_WIDTH >> 8,            // Frame width
	USB_UVC_800x480_HEIGHT & 0xff,         // Frame height
	USB_UVC_800x480_HEIGHT >> 8,           // Frame height
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // ( Default frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // ( Min frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // (Max frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 640x360 Descriptor ] ----- (length 38)
	USB_UVC_VS_MJPG_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_MJPEG,                     // descriptor subtype
	UVC_VSFRM_INDEX8,                   // frame index
	UVC_VSFRM_INDEX8_CAP,               // frame capabilities
	USB_UVC_FULL_HD_WIDTH & 0xff,            // wWidth
	USB_UVC_FULL_HD_WIDTH >> 8,              // wWidth
	USB_UVC_FULL_HD_HEIGHT & 0xff,           // wHeight
	USB_UVC_FULL_HD_HEIGHT >> 8,             // wHeight
	UVC_VSFRM_BITRATE_MIN & 0xff,       // Min bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MIN >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MIN >> 24) & 0xff,
	UVC_VSFRM_BITRATE_MAX & 0xff,       // Max bit rate in bits/s    [0:31]
	(UVC_VSFRM_BITRATE_MAX >> 8) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 16) & 0xff,
	(UVC_VSFRM_BITRATE_MAX >> 24) & 0xff,
	UVC_VSFRM_MAX_FBSIZE & 0xff,        // dwMaxVideoFrameBufSize [0:31]
	(UVC_VSFRM_MAX_FBSIZE >> 8) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 16) & 0xff,
	(UVC_VSFRM_MAX_FBSIZE >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // ( Default frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // ( Min frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // (Max frame interval 666666ns /15fps)  100ns/unit
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Color matching Descriptor ] ----- (length 6)
	USB_UVC_VS_COLOR_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_COLORFORMAT,                     // descriptor subtype
	UVC_COLOR_PRIMARIES,
	UVC_COLOR_TRANSFER,
	UVC_COLOR_MATRIX,

	//----- [Class spec VS Format Descriptor ] -----  (length 28)
	//FRAME-BASED FORMAT
	USB_UVC_VS_FBH264_FORMAT_LENGTH,           // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FORMAT_FRAME_BASED,                    // descriptor subtype  //VS_FORMAT_H264_SIMULCAST
	UVC_VSFMT_SECONDINDEX,//UVC_VSFMT_FIRSTINDEX,               // First format descriptor
	UVC_VSFMT_FRAMENUM1_HD,             // frame descriptor numbers
	//16 byte GUID
	0x48,
	0x32,
	0x36,
	0x34,
	0x00,
	0x00,
	0x10,
	0x00,
	0x80,
	0x00,
	0x00,
	0xAA,
	0x00,
	0x38,
	0x9B,
	0x71,

	UVC_BITS_PER_PIXEL,             //bBitsPerPixel
	UVC_VSFMT_DEF_FRAMEINDEX,               /* Default frame index is 1=>1280x720 */
	UVC_VSFMT_ASPECTRATIO_X,                           /* bAspecRatioX */
	UVC_VSFMT_ASPECTRATIO_Y,                           /* bAspecRatioY */
	UVC_VSFMT_INTERLACE,                           /* bmInterlaceFlags */
	UVC_VSFMT_COPYPROTECT,                           /* bCopyProtect: duplication unrestricted */
	0x01,                           /* bVariableSize */

	//----- [Class spec VS Frame 1280x720 Descriptor ] ----- (length 38)
	//USB_UVC_HD_WIDTH  ; FRAME-BASED FRAME
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX1,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_HD_WIDTH & 0xff,            // wWidth
	USB_UVC_HD_WIDTH >> 8,              // wWidth
	USB_UVC_HD_HEIGHT & 0xff,           // wHeight
	USB_UVC_HD_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate


	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 1280x768 Descriptor ] ----- (length 38)
	//USB_UVC_1024x768_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX2,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_1024x768_WIDTH & 0xff,            // wWidth
	USB_UVC_1024x768_WIDTH >> 8,              // wWidth
	USB_UVC_1024x768_HEIGHT & 0xff,           // wHeight
	USB_UVC_1024x768_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,


	//----- [Class spec VS Frame 640x480 Descriptor ] ----- (length 38)
	//USB_UVC_VGA_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX3,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_VGA_WIDTH & 0xff,            // wWidth
	USB_UVC_VGA_WIDTH >> 8,              // wWidth
	USB_UVC_VGA_HEIGHT & 0xff,           // wHeight
	USB_UVC_VGA_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 320x240 Descriptor ] ----- (length 38)
	//USB_UVC_QVGA_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX4,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_QVGA_WIDTH & 0xff,            // wWidth
	USB_UVC_QVGA_WIDTH >> 8,              // wWidth
	USB_UVC_QVGA_HEIGHT & 0xff,           // wHeight
	USB_UVC_QVGA_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 1024x576 Descriptor ] ----- (length 38)
	//USB_UVC_1024x576_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX5,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_1024x576_WIDTH & 0xff,            // wWidth
	USB_UVC_1024x576_WIDTH >> 8,              // wWidth
	USB_UVC_1024x576_HEIGHT & 0xff,           // wHeight
	USB_UVC_1024x576_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 800x600 Descriptor ] ----- (length 38)
	//USB_UVC_800x600_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX6,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_800x600_WIDTH & 0xff,            // wWidth
	USB_UVC_800x600_WIDTH >> 8,              // wWidth
	USB_UVC_800x600_HEIGHT & 0xff,           // wHeight
	USB_UVC_800x600_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 800x480 Descriptor ] ----- (length 38)
	//USB_UVC_800x480_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX7,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_800x480_WIDTH & 0xff,            // wWidth
	USB_UVC_800x480_WIDTH >> 8,              // wWidth
	USB_UVC_800x480_HEIGHT & 0xff,           // wHeight
	USB_UVC_800x480_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 1920x1080 Descriptor ] ----- (length 38)
	//USB_UVC_1920x1080_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX8,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_FULL_HD_WIDTH & 0xff,            // wWidth
	USB_UVC_FULL_HD_WIDTH >> 8,              // wWidth
	USB_UVC_FULL_HD_HEIGHT & 0xff,           // wHeight
	USB_UVC_FULL_HD_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate
	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)

	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine

	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Color matching Descriptor ] ----- (length 6)
	USB_UVC_VS_COLOR_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_COLORFORMAT,                     // descriptor subtype
	UVC_COLOR_PRIMARIES,
	UVC_COLOR_TRANSFER,
	UVC_COLOR_MATRIX,

	//----- [Standard VS interface Descriptor] ----- (length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_SECOND_VID_IF + 1),        // interface number
	UVAC_IF_ALT1,                    // intfAltSet setting
	UVC_VS_IF0_NUMBER_EP,               // number of endpoint
	CC_VIDEO,                           // interface class
	SC_VIDEOSTREAMING,                  // interface subclass
	PC_PROTOCOL_UNDEFINED,              // interface protocol
	UVC_VS_INTERFACE_STR_INDEX,             // interface string id

	//----- [Standard Iso EP Descriptor ] ----- (length 7)
	USB_BULK_EP_LENGTH,                 // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,       // descriptor type
	USB_EP_EP2I_ADDRESS,                // endpoint address
	USB_EP_ATR_ISO | USB_EP_ATR_ASYNC,  // attributes
	0x80,    // max packet size
	0x00,     // max packet size
	USB_EP_ISO_INTERVAL,               // interval

	//----- [Standard VS interface Descriptor] ----- (length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_SECOND_VID_IF + 1),        // interface number
	UVAC_IF_ALT2,                    // intfAltSet setting
	UVC_VS_IF0_NUMBER_EP,               // number of endpoint
	CC_VIDEO,                           // interface class
	SC_VIDEOSTREAMING,                  // interface subclass
	PC_PROTOCOL_UNDEFINED,              // interface protocol
	UVC_VS_INTERFACE_STR_INDEX,             // interface string id

	//----- [Standard Iso EP Descriptor ] ----- (length 7)
	USB_BULK_EP_LENGTH,                 // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,       // descriptor type
	USB_EP_EP2I_ADDRESS,                // endpoint address
	USB_EP_ATR_ISO | USB_EP_ATR_ASYNC,  // attributes
	0x00,    // max packet size
	0x01,     // max packet size
	USB_EP_ISO_INTERVAL,

	//----- [Standard VS interface Descriptor] ----- (length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_SECOND_VID_IF + 1),        // interface number
	UVAC_IF_ALT3,                    // intfAltSet setting
	UVC_VS_IF0_NUMBER_EP,               // number of endpoint
	CC_VIDEO,                           // interface class
	SC_VIDEOSTREAMING,                  // interface subclass
	PC_PROTOCOL_UNDEFINED,              // interface protocol
	UVC_VS_INTERFACE_STR_INDEX,             // interface string id

	//----- [Standard Iso EP Descriptor ] ----- (length 7)
	USB_BULK_EP_LENGTH,                 // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,       // descriptor type
	USB_EP_EP2I_ADDRESS,                // endpoint address
	USB_EP_ATR_ISO | USB_EP_ATR_ASYNC,  // attributes
	USB_HS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff,    // max packet size
	USB_HS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 0x08,     // max packet size
	USB_EP_ISO_INTERVAL,               // interval
#endif

	//audio1 uac1,uas1
	//--- Audio Configuration ---
	//----- [   IAD Descriptor  ]----- (Length 8)
	USB_UAC_IAD_LENGTH,                 // descriptor size
	USB_IAD_DESCRIPTOR_TYPE,            // descriptor type
	USB_PCC_IAD_FIRST_IF_UAC,           // bFirstInterface
	USB_PCC_IAD_IF_COUNT, //USB_PCC_IAD_IF_COUNT_AUD,           // Interface counts
	CC_AUDIO,                           // bFunction class
	SC_AUDIOSTREAMING,                  // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,              // bFunction Protocol
	UVAC_IAD_STR_INDEX,              // iFunction

	//----- [ Standard AC interface Descriptor ]----- (Length 9)
	USB_UAC_AC_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	USB_PCC_IAD_FIRST_IF_UAC,           // bFirstInterface
	UVAC_IF_ALT0,                    // intfAltSet setting
	UVAC_IF_IF0_UAC_NUMBER_EP0,      // number of endpoint
	CC_AUDIO,                           // bFunction class
	SC_AUDIOCONTROL,                    // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,                     // bFunction Protocol
	UVAC_IF_IDX,                     // interface string id

	//----- [Class-spec. AC interface Descriptor ] ----- (length 9)
	USB_UAC_CS_AC_IF_LENGTH,               // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_HEADER,                          // descriptor Subtype
	USB_UAC_RELEASE_NUMBER  & 0xff,     // UVC version
	USB_UAC_RELEASE_NUMBER >> 8,
	USB_UAC_AC_TOTAL_LENGTH & 0xff,     // Total Size of class-spec. descriptor
	USB_UAC_AC_TOTAL_LENGTH >> 8,
	UAC_AC_AUDIOSTREAM_TOTAL,           // bInCollection (Number of streaming interfaces)
	(USB_PCC_IAD_FIRST_IF_UAC + 1),    //first interface number

	//----- [Class-spec. AC Input Terminal interface Descriptor ] ----- (length 12)
	USB_UAC_IT_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_INPUT_TERMINAL,                  // descriptor Subtype
	UAC_IT_ID,                          // bTerminalID
	UAC_IT_MICROPHONE & 0xff,                  // wTerminalType
	UAC_IT_MICROPHONE >> 8,                  // wTerminalType
	UAC_IT_ASSOCIATION,                 // bAssocTerminal
	UAC_IT_OUT_CHANNEL_CNT,             // logical output channels
	UAC_IT_OUT_CHANNEL_CONFIG & 0xff,           // channel config.,Spatial location
	UAC_IT_OUT_CHANNEL_CONFIG >> 8,     // channel config.,Spatial location
	UAC_IT_OUT_CHANNEL_STR_IDX,         // Index of a string descriptor, describing the name of the first logical channel.
	UAC_IT_STR_IDX,                     // Index of a string descriptor, describing the Input Terminal.

	//----- [Class-spec. AC Output Terminal interface Descriptor ] ----- (length 9)
	USB_UAC_OT_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_OUTPUT_TERMINAL,                 // descriptor Subtype
	UAC_OT_ID,                          // bTerminalID
	UAC_TERMINAL_STREAMING & 0xff,             // wTerminalType
	UAC_TERMINAL_STREAMING >> 8,             // wTerminalType
	UAC_OT_ASSOCIATION,                 // bAssocTerminal
	UAC_OT_SRC_ID,                      // ID of the Unit or Terminal to which this Terminal is connected.
	UAC_OT_STR_IDX,                     // Index of a string descriptor, describing the Input Terminal.

	//----- [Class-spec. AC Feature Unit interface Descriptor ] ----- (length 8)
	USB_UAC_FU_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_FEATURE_UNIT,                    // descriptor Subtype
	UAC_FU_UNIT_ID,                     // bUnitID
	UAC_FU_SRC_ID,                      // bSourceID
	UAC_FU_CTRL_SIZE,                   // Size in bytes of an element of the bmaControls() array
	UAC_FU_CTRL_CONFIG,                 //
	UAC_FU_STR_IDX,                     // Index of a string descriptor, describing the Input Terminal.

	//----- [ Standard AS interface Descriptor ]----- (Length 9)
	USB_UAC_AS_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_FIRST_IF_UAC + 1),     // interface number
	UAC_IF_ALT0,                        // intfAltSet setting
	UVAC_IF_IF0_UAS_NUMBER_EP0,         // number of endpoint
	CC_AUDIO,                           // bFunction class
	SC_AUDIOSTREAMING,                  // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,                     // bFunction Protocol
	UVAC_IF_IDX,                        // interface string id

	//----- [ Standard AS interface Descriptor ]----- (Length 9)
	USB_UAC_AS_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_FIRST_IF_UAC + 1),     // interface number,index of thi interface 3
	UAC_IF_ALT1,                        // intfAltSet setting
	UVAC_IF_IF0_UAS_NUMBER_EP1,         // number of endpoint
	CC_AUDIO,                           // bFunction class
	SC_AUDIOSTREAMING,                  // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,                     // bFunction Protocol
	UVAC_IF_IDX,                     // interface string id

	//----- [Class-spec. AS General interface Descriptor ] ----- (length 7)
	USB_UAC_AS_CS_LENGTH,               // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_AS_GENERAL,                  // descriptor Subtype
	UAC_OT_ID,                         // Output Terminal ID
	UAC_CS_AS_DELAY,                    // Delay introduced by the data path. Expressed in number of frames.
	UAC_DATA_FMT_TYPE_I_PCM & 0xff,                   // Audio Format
	UAC_DATA_FMT_TYPE_I_PCM >> 8,             // Audio Format

	//----- [Type I Format Type Descriptor ] ----- (length 11)
	USB_UAC_TYPE_I_FMT_LENGTH,          // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_FORMAT_TYPE,                     // descriptor Subtype
	UAC_FMT_TYPE_I,                          // Output Terminal ID
	UAC_NUM_PHYSICAL_CHANNEL,           //channel number
	UAC_BYTE_EACH_SUBFRAME,
	UAC_BIT_PER_SECOND,
	UAC_FREQ_TYPE_CNT,
	UAC_FREQUENCY_16K & 0xff,
	(UAC_FREQUENCY_16K >> 8) & 0xff,
	(UAC_FREQUENCY_16K >> 16) & 0xff,

	//----- [ Standard Audio Endpoint Descriptor ]----- (Length 9)
	USB_UAC_STAND_ENDPOINT_LENGTH,                      // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,      // descriptor type
	USB_EP_EP3I_ADDRESS,//USB_EP_EP2I_ADDRESS,          // endpoint address
	USB_UAC_ENDPOINT_ATTR,                    // attribute
	USB_UAC_MAX_PACKET_68 & 0xff,      // byte per packet
	(USB_UAC_MAX_PACKET_68 >> 8) & 0xff,      // byte per packet
	USB_UAC_POLL_INTERVAL,
	USB_UAC_EP_REFRESH,
	USB_UAC_EP_SYNC_ADDR,

	//----- [Class-spec. Endpoint General Descriptor ] ----- (length 7)
	USB_UAC_CS_ENDPOINT_GENERAL_LENGTH, // descriptor size
	CS_ENDPOINT,                        // descriptor type
	UAC_CS_AS_GENERAL,                   // descriptor Subtype
	0x01,                               // bmAttributes (Sampling Frequency 1)
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAYUNIT,                           // Output Terminal ID
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY & 0xff,                    // Delay introduced by the data path. Expressed in number of frames.
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY >> 8,                    // Delay introduced by the data path. Expressed in number of frames.

	//----- [ Standard AS interface Descriptor ]----- (Length 9)
	USB_UAC_AS_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_FIRST_IF_UAC + 1),     // interface number,index of thi interface 3
	UAC_IF_ALT2,                       // intfAltSet setting
	UVAC_IF_IF0_UAS_NUMBER_EP1,        // number of endpoint
	CC_AUDIO,                           // bFunction class
	SC_AUDIOSTREAMING,                  // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,                     // bFunction Protocol
	UVAC_IF_IDX,                     // interface string id

	//----- [Class-spec. AS General interface Descriptor ] ----- (length 7)
	USB_UAC_AS_CS_LENGTH,               // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_AS_GENERAL,                  // descriptor Subtype
	UAC_OT_ID,                          // Output Terminal ID
	UAC_CS_AS_DELAY,                    // Delay introduced by the data path. Expressed in number of frames.
	UAC_DATA_FMT_TYPE_I_PCM & 0xff,                   // Audio Format
	UAC_DATA_FMT_TYPE_I_PCM >> 8,             // Audio Format

	//----- [Type I Format Type Descriptor ] ----- (length 11)??
	USB_UAC_TYPE_I_FMT_LENGTH,          // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_FORMAT_TYPE,                     // descriptor Subtype
	UAC_FMT_TYPE_I,                          // Output Terminal ID
	UAC_NUM_PHYSICAL_CHANNEL,           //channel number
	UAC_BYTE_EACH_SUBFRAME,
	UAC_BIT_PER_SECOND,
	UAC_FREQ_TYPE_CNT,
	UAC_FREQUENCY_32K & 0xff,
	(UAC_FREQUENCY_32K >> 8) & 0xff,
	(UAC_FREQUENCY_32K >> 16) & 0xff,

	//----- [ Standard Audio Endpoint Descriptor ]----- (Length 9)
	USB_UAC_STAND_ENDPOINT_LENGTH,                      // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,      // descriptor type
	USB_EP_EP3I_ADDRESS,//USB_EP_EP2I_ADDRESS,          // endpoint address
	USB_UAC_ENDPOINT_ATTR,                    // attribute
	USB_UAC_MAX_PACKET_100 & 0xff,      // byte per packet
	(USB_UAC_MAX_PACKET_100 >> 8) & 0xff,      // byte per packet
	USB_UAC_POLL_INTERVAL,
	USB_UAC_EP_REFRESH,
	USB_UAC_EP_SYNC_ADDR,

	//----- [Class-spec. Endpoint General Descriptor ] ----- (length 7)
	USB_UAC_CS_ENDPOINT_GENERAL_LENGTH, // descriptor size
	CS_ENDPOINT,                        // descriptor type
	UAC_CS_AS_GENERAL,                   // descriptor Subtype
	0x01,                               // bmAttributes (Sampling Frequency 1)
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAYUNIT,                           // Output Terminal ID
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY & 0xff,                      // Delay introduced by the data path. Expressed in number of frames.
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY >> 8,                    // Delay introduced by the data path. Expressed in number of frames.

	//----- [ Standard AS interface Descriptor ]----- (Length 9)
	USB_UAC_AS_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_FIRST_IF_UAC + 1),    // interface number,index of thi interface 3
	UAC_IF_ALT3,                     // intfAltSet setting
	UVAC_IF_IF0_UAS_NUMBER_EP1,      // number of endpoint
	CC_AUDIO,                           // bFunction class
	SC_AUDIOSTREAMING,                  // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,                     // bFunction Protocol
	UVAC_IF_IDX,                     // interface string id

	//----- [Class-spec. AS General interface Descriptor ] ----- (length 7)
	USB_UAC_AS_CS_LENGTH,               // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_AS_GENERAL,                  // descriptor Subtype
	UAC_OT_ID,                          // Output Terminal ID
	UAC_CS_AS_DELAY,                    // Delay introduced by the data path. Expressed in number of frames.
	UAC_DATA_FMT_TYPE_I_PCM & 0xff,                  // Audio Format
	UAC_DATA_FMT_TYPE_I_PCM >> 8,             // Audio Format

	//----- [Type I Format Type Descriptor ] ----- (length 11)
	USB_UAC_TYPE_I_FMT_LENGTH,          // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_FORMAT_TYPE,                     // descriptor Subtype
	UAC_FMT_TYPE_I,                          // Output Terminal ID
	UAC_NUM_PHYSICAL_CHANNEL,           //channel number
	UAC_BYTE_EACH_SUBFRAME,
	UAC_BIT_PER_SECOND,
	UAC_FREQ_TYPE_CNT,
	UAC_FREQUENCY_48K & 0xff,
	(UAC_FREQUENCY_48K >> 8) & 0xff,
	(UAC_FREQUENCY_48K >> 16) & 0xff,

	//----- [ Standard Audio Endpoint Descriptor ]----- (Length 9)
	USB_UAC_STAND_ENDPOINT_LENGTH,                      // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,      // descriptor type
	USB_EP_EP3I_ADDRESS,//USB_EP_EP2I_ADDRESS,          // endpoint address
	USB_UAC_ENDPOINT_ATTR,                    // attribute
	USB_UAC_MAX_PACKET_132 & 0xff,      // byte per packet
	(USB_UAC_MAX_PACKET_132 >> 8) & 0xff,      // byte per packet
	USB_UAC_POLL_INTERVAL,
	USB_UAC_EP_REFRESH,
	USB_UAC_EP_SYNC_ADDR,

	//----- [Class-spec. Endpoint General Descriptor ] ----- (length 7)
	USB_UAC_CS_ENDPOINT_GENERAL_LENGTH, // descriptor size
	CS_ENDPOINT,                        // descriptor type
	UAC_CS_AS_GENERAL,                   // descriptor Subtype
	0x01,                               // bmAttributes (Sampling Frequency 1)
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAYUNIT,                           // Output Terminal ID
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY & 0xff,                    // Delay introduced by the data path. Expressed in number of frames.

#if 1//(UVAC_UVC_UAC_DEV_SET == UVAC_1UVC_1UAC)
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY >> 8
#else
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY >> 8,

	//audio2 uac2,uas2
	//--- Audio Configuration ---
	//----- [   IAD Descriptor  ]----- (Length 8)
	USB_UAC_IAD_LENGTH,                 // descriptor size
	USB_IAD_DESCRIPTOR_TYPE,            // descriptor type
	USB_PCC_IAD_SECOND_IF_UAC,          // bFirstInterface
	USB_PCC_IAD_IF_COUNT, //USB_PCC_IAD_IF_COUNT_AUD,           // Interface counts
	CC_AUDIO,                           // bFunction class
	SC_AUDIOSTREAMING,                  // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,              // bFunction Protocol
	UVAC_IAD_STR_INDEX,              // iFunction

	//----- [ Standard AC interface Descriptor ]----- (Length 9)
	USB_UAC_AC_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	USB_PCC_IAD_SECOND_IF_UAC,          // interface number
	UVAC_IF_ALT0,                    // intfAltSet setting
	UVAC_IF_IF0_UAC_NUMBER_EP0,      // number of endpoint
	CC_AUDIO,                           // bFunction class
	SC_AUDIOCONTROL,                    // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,                     // bFunction Protocol
	UVAC_IF_IDX,                     // interface string id

	//----- [Class-spec. AC interface Descriptor ] ----- (length 9)
	USB_UAC_CS_AC_IF_LENGTH,               // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_HEADER,                          // descriptor Subtype
	USB_UAC_RELEASE_NUMBER  & 0xff,     // UVC version
	USB_UAC_RELEASE_NUMBER >> 8,
	USB_UAC_AC_TOTAL_LENGTH & 0xff,     // Total Size of class-spec. descriptor
	USB_UAC_AC_TOTAL_LENGTH >> 8,
	UAC_AC_AUDIOSTREAM_TOTAL,           // bInCollection (Number of streaming interfaces)
	(USB_PCC_IAD_SECOND_IF_UAC + 1),    //first interface number

	//----- [Class-spec. AC Input Terminal interface Descriptor ] ----- (length 12)
	USB_UAC_IT_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_INPUT_TERMINAL,                  // descriptor Subtype
	UAC_IT_ID,                          // bTerminalID
	UAC_IT_MICROPHONE & 0xff,                  // wTerminalType
	UAC_IT_MICROPHONE >> 8,                  // wTerminalType
	UAC_IT_ASSOCIATION,                 // bAssocTerminal
	UAC_IT_OUT_CHANNEL_CNT,             // logical output channels
	UAC_IT_OUT_CHANNEL_CONFIG & 0xff,           // channel config.,Spatial location
	UAC_IT_OUT_CHANNEL_CONFIG >> 8,     // channel config.,Spatial location
	UAC_IT_OUT_CHANNEL_STR_IDX,         // Index of a string descriptor, describing the name of the first logical channel.
	UAC_IT_STR_IDX,                     // Index of a string descriptor, describing the Input Terminal.

	//----- [Class-spec. AC Output Terminal interface Descriptor ] ----- (length 9)
	USB_UAC_OT_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_OUTPUT_TERMINAL,                 // descriptor Subtype
	UAC_OT_ID,                          // bTerminalID
	UAC_TERMINAL_STREAMING & 0xff,             // wTerminalType
	UAC_TERMINAL_STREAMING >> 8,             // wTerminalType
	UAC_OT_ASSOCIATION,                 // bAssocTerminal
	UAC_OT_SRC_ID,                      // ID of the Unit or Terminal to which this Terminal is connected.
	UAC_OT_STR_IDX,                     // Index of a string descriptor, describing the Input Terminal.

	//----- [Class-spec. AC Feature Unit interface Descriptor ] ----- (length 8)
	USB_UAC_FU_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_FEATURE_UNIT,                    // descriptor Subtype
	UAC_FU_UNIT_ID,                     // bUnitID
	UAC_FU_SRC_ID,                      // bSourceID
	UAC_FU_CTRL_SIZE,                   // Size in bytes of an element of the bmaControls() array
	UAC_FU_CTRL_CONFIG,                 //
	UAC_FU_STR_IDX,                     // Index of a string descriptor, describing the Input Terminal.

	//----- [ Standard AS interface Descriptor ]----- (Length 9)
	USB_UAC_AS_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_SECOND_IF_UAC + 1),//7,//UVAC_IF_IF0_UAS_NUMBER,          // interface number  3
	UAC_IF_ALT0,                        // intfAltSet setting
	UVAC_IF_IF0_UAS_NUMBER_EP0,      // number of endpoint
	CC_AUDIO,                           // bFunction class
	SC_AUDIOSTREAMING,                  // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,                     // bFunction Protocol
	UVAC_IF_IDX,                     // interface string id

	//----- [ Standard AS interface Descriptor ]----- (Length 9)
	USB_UAC_AS_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_SECOND_IF_UAC + 1),   //7,//5,//UVAC_IF_IF0_UAS_NUMBER,          // interface number,index of thi interface 3
	UAC_IF_ALT1,                     // intfAltSet setting
	UVAC_IF_IF0_UAS_NUMBER_EP1,      // number of endpoint
	CC_AUDIO,                           // bFunction class
	SC_AUDIOSTREAMING,                  // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,                     // bFunction Protocol
	UVAC_IF_IDX,                     // interface string id

	//----- [Class-spec. AS General interface Descriptor ] ----- (length 7)
	USB_UAC_AS_CS_LENGTH,               // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_AS_GENERAL,                  // descriptor Subtype
	UAC_OT_ID,                         // Output Terminal ID
	UAC_CS_AS_DELAY,                    // Delay introduced by the data path. Expressed in number of frames.
	UAC_DATA_FMT_TYPE_I_PCM & 0xff,                   // Audio Format
	UAC_DATA_FMT_TYPE_I_PCM >> 8,             // Audio Format

	//----- [Type I Format Type Descriptor ] ----- (length 11)
	USB_UAC_TYPE_I_FMT_LENGTH,          // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_FORMAT_TYPE,                     // descriptor Subtype
	UAC_FMT_TYPE_I,                          // Output Terminal ID
	UAC_NUM_PHYSICAL_CHANNEL,           //channel number
	UAC_BYTE_EACH_SUBFRAME,
	UAC_BIT_PER_SECOND,
	UAC_FREQ_TYPE_CNT,
	UAC_FREQUENCY_16K & 0xff,
	(UAC_FREQUENCY_16K >> 8) & 0xff,
	(UAC_FREQUENCY_16K >> 16) & 0xff,

	//----- [ Standard Audio Endpoint Descriptor ]----- (Length 9)
	USB_UAC_STAND_ENDPOINT_LENGTH,                      // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,      // descriptor type
	USB_EP_EP4I_ADDRESS,//USB_EP_EP5I_ADDRESS,//USB_EP_EP2I_ADDRESS,          // endpoint address
	USB_UAC_ENDPOINT_ATTR,                    // attribute
	USB_UAC_MAX_PACKET_68 & 0xff,      // byte per packet
	(USB_UAC_MAX_PACKET_68 >> 8) & 0xff,      // byte per packet
	USB_UAC_POLL_INTERVAL,
	USB_UAC_EP_REFRESH,
	USB_UAC_EP_SYNC_ADDR,

	//----- [Class-spec. Endpoint General Descriptor ] ----- (length 7)
	USB_UAC_CS_ENDPOINT_GENERAL_LENGTH, // descriptor size
	CS_ENDPOINT,                        // descriptor type
	UAC_CS_AS_GENERAL,                   // descriptor Subtype
	0x01,                               // bmAttributes (Sampling Frequency 1)
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAYUNIT,                           // Output Terminal ID
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY & 0xff,                    // Delay introduced by the data path. Expressed in number of frames.
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY >> 8,                    // Delay introduced by the data path. Expressed in number of frames.

	//----- [ Standard AS interface Descriptor ]----- (Length 9)
	USB_UAC_AS_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_SECOND_IF_UAC + 1), //7,//5,//UVAC_IF_IF0_UAS_NUMBER,          // interface number,index of thi interface 3
	UAC_IF_ALT2,                    // intfAltSet setting
	UVAC_IF_IF0_UAS_NUMBER_EP1,      // number of endpoint
	CC_AUDIO,                           // bFunction class
	SC_AUDIOSTREAMING,                  // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,                     // bFunction Protocol
	UVAC_IF_IDX,                     // interface string id

	//----- [Class-spec. AS General interface Descriptor ] ----- (length 7)
	USB_UAC_AS_CS_LENGTH,               // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_AS_GENERAL,                  // descriptor Subtype
	UAC_OT_ID,                          // Output Terminal ID
	UAC_CS_AS_DELAY,                    // Delay introduced by the data path. Expressed in number of frames.
	UAC_DATA_FMT_TYPE_I_PCM & 0xff,                   // Audio Format
	UAC_DATA_FMT_TYPE_I_PCM >> 8,             // Audio Format

	//----- [Type I Format Type Descriptor ] ----- (length 11)??
	USB_UAC_TYPE_I_FMT_LENGTH,          // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_FORMAT_TYPE,                     // descriptor Subtype
	UAC_FMT_TYPE_I,                          // Output Terminal ID
	UAC_NUM_PHYSICAL_CHANNEL,           //channel number
	UAC_BYTE_EACH_SUBFRAME,
	UAC_BIT_PER_SECOND,
	UAC_FREQ_TYPE_CNT,
	UAC_FREQUENCY_32K & 0xff,
	(UAC_FREQUENCY_32K >> 8) & 0xff,
	(UAC_FREQUENCY_32K >> 16) & 0xff,

	//----- [ Standard Audio Endpoint Descriptor ]----- (Length 9)
	USB_UAC_STAND_ENDPOINT_LENGTH,                      // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,      // descriptor type
	USB_EP_EP4I_ADDRESS,//USB_EP_EP5I_ADDRESS,//USB_EP_EP2I_ADDRESS,          // endpoint address
	USB_UAC_ENDPOINT_ATTR,                    // attribute
	USB_UAC_MAX_PACKET_100 & 0xff,      // byte per packet
	(USB_UAC_MAX_PACKET_100 >> 8) & 0xff,      // byte per packet
	USB_UAC_POLL_INTERVAL,
	USB_UAC_EP_REFRESH,
	USB_UAC_EP_SYNC_ADDR,

	//----- [Class-spec. Endpoint General Descriptor ] ----- (length 7)
	USB_UAC_CS_ENDPOINT_GENERAL_LENGTH, // descriptor size
	CS_ENDPOINT,                        // descriptor type
	UAC_CS_AS_GENERAL,                   // descriptor Subtype
	0x01,                               // bmAttributes (Sampling Frequency 1)
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAYUNIT,                           // Output Terminal ID
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY & 0xff,                      // Delay introduced by the data path. Expressed in number of frames.
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY >> 8,                    // Delay introduced by the data path. Expressed in number of frames.

	//----- [ Standard AS interface Descriptor ]----- (Length 9)
	USB_UAC_AS_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(USB_PCC_IAD_SECOND_IF_UAC + 1),//7,//5,//UVAC_IF_IF0_UAS_NUMBER,          // interface number,index of thi interface 3
	UAC_IF_ALT3,                     // intfAltSet setting
	UVAC_IF_IF0_UAS_NUMBER_EP1,      // number of endpoint
	CC_AUDIO,                           // bFunction class
	SC_AUDIOSTREAMING,                  // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,                     // bFunction Protocol
	UVAC_IF_IDX,                     // interface string id

	//----- [Class-spec. AS General interface Descriptor ] ----- (length 7)
	USB_UAC_AS_CS_LENGTH,               // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_AS_GENERAL,                  // descriptor Subtype
	UAC_OT_ID,                          // Output Terminal ID
	UAC_CS_AS_DELAY,                    // Delay introduced by the data path. Expressed in number of frames.
	UAC_DATA_FMT_TYPE_I_PCM & 0xff,                  // Audio Format
	UAC_DATA_FMT_TYPE_I_PCM >> 8,             // Audio Format

	//----- [Type I Format Type Descriptor ] ----- (length 11)
	USB_UAC_TYPE_I_FMT_LENGTH,          // descriptor size
	CS_INTERFACE,                       // descriptor type
	UAC_CS_FORMAT_TYPE,                     // descriptor Subtype
	UAC_FMT_TYPE_I,                          // Output Terminal ID
	UAC_NUM_PHYSICAL_CHANNEL,           //channel number
	UAC_BYTE_EACH_SUBFRAME,
	UAC_BIT_PER_SECOND,
	UAC_FREQ_TYPE_CNT,
	UAC_FREQUENCY_48K & 0xff,
	(UAC_FREQUENCY_48K >> 8) & 0xff,
	(UAC_FREQUENCY_48K >> 16) & 0xff,

	//----- [ Standard Audio Endpoint Descriptor ]----- (Length 9)
	USB_UAC_STAND_ENDPOINT_LENGTH,                      // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,      // descriptor type
	USB_EP_EP4I_ADDRESS,//USB_EP_EP5I_ADDRESS,//USB_EP_EP2I_ADDRESS,          // endpoint address
	USB_UAC_ENDPOINT_ATTR,                    // attribute
	USB_UAC_MAX_PACKET_132 & 0xff,      // byte per packet
	(USB_UAC_MAX_PACKET_132 >> 8) & 0xff,      // byte per packet
	USB_UAC_POLL_INTERVAL,
	USB_UAC_EP_REFRESH,
	USB_UAC_EP_SYNC_ADDR,

	//----- [Class-spec. Endpoint General Descriptor ] ----- (length 7)
	USB_UAC_CS_ENDPOINT_GENERAL_LENGTH, // descriptor size
	CS_ENDPOINT,                        // descriptor type
	UAC_CS_AS_GENERAL,                   // descriptor Subtype
	0x01,                               // bmAttributes (Sampling Frequency 1)
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAYUNIT,                           // Output Terminal ID
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY & 0xff,                    // Delay introduced by the data path. Expressed in number of frames.
	USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY >> 8
#endif

};
_ALIGNED(4) UINT8 g_U2UVACFSOtherConfigDescFBH264[USB_UVC_TOTAL_LENGTH_FB_H264] = {
	//----- [Configuration Descriptor]----- (Length 9)
	USB_CFG_LENGTH,                     // descriptor size
	USB_CONFIGURATION_DESCRIPTOR_TYPE,  // descriptor type
	(USB_UVC_TOTAL_LENGTH_FB_H264) & 0xff,   // total length
	(USB_UVC_TOTAL_LENGTH_FB_H264) >> 8,
	USB_PCC_UVC_NUMBER_IF,              // number of interface
	UVAC_CFGA_CFG_VALUE,             // configuration value
	UVAC_CFGA_CFG_IDX,               // configuration string id
	UVAC_CFGA_CFG_ATTRIBUES,         // characteristics
	UVAC_CFGA_MAX_POWER,             // maximum power in 2mA

	//----- [   IAD Descriptor  ]----- (Length 8)
	USB_UVC_IAD_LENGTH,                 // descriptor size
	USB_IAD_DESCRIPTOR_TYPE,            // descriptor type
	USB_PCC_IAD_FIRST_VID_IF,               // bFirstInterface
	USB_PCC_IAD_IF_COUNT,               // Interface counts
	CC_VIDEO,                           // bFunction class
	SC_VIDEO_INTERFACE_COLLECTION,      // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,              // bFunction Protocol
	UVAC_IAD_STR_INDEX,              //iFunction

	//----- [ Standard VC interface Descriptor ]----- (Length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	UVAC_IF_IF0_NUMBER,              // interface number
	UVAC_IF_ALT0,                    // intfAltSet setting
	UVAC_IF_CFGA_IF0_NUMBER_EP,      // number of endpoint
	CC_VIDEO,                           // bFunction class
	SC_VIDEOCONTROL,                    // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,              // bFunction Protocol
	UVAC_IF_IDX,                     // interface string id

	//----- [Class-spec. VC interface Descriptor ] ----- (length 13)
	USB_UVC_VC_IF_LENGTH,               // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_HEADER,                          // descriptor Subtype
	USB_UVC_RELEASE_NUMBER  & 0xff,     // UVC version
	USB_UVC_RELEASE_NUMBER >> 8,
	USB_UVC_VC_TOTAL_LENGTH & 0xff,     // Total Size of class-spec. descriptor
	USB_UVC_VC_TOTAL_LENGTH >> 8,
	UVC_VC_CLOCK_FREQUENCY & 0xff,      // dwClockFrequency 6MHz (Use of this field has been deprecated)
	(UVC_VC_CLOCK_FREQUENCY >> 8) & 0xff,
	(UVC_VC_CLOCK_FREQUENCY >> 16) & 0xff,
	(UVC_VC_CLOCK_FREQUENCY >> 24) & 0xff,
	UVC_VC_VIDEOSTREAM_TOTAL,           // bInCollection (Number of streaming interfaces)
	UVC_VC_VIDEOSTREAM_FIRSTNUM,        // VS I/F 1 belongs to the VC I/F

	//----- [Camera terminal Descriptor ] ----- (length 17)
	USB_UVC_IT_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_INPUT_TERMINAL,                  // descriptor Subtype
	USB_UVC_ID1,                        // ID of this terminal
	ITT_CAMERA & 0xff,                  // terminal type
	ITT_CAMERA >> 8,
	UVC_IT_ASSOC_TERMINAL,              // no association
	UVC_IT_ITERMINAL_STR_INDEX,             // index of string descriptor (unused)
	UVC_IT_OBJFOCAL_LEN_MIN  & 0xff,    // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN_MIN >> 8,
	UVC_IT_OBJFOCAL_LEN_MAX  & 0xff,    // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN_MAX >> 8,
	UVC_IT_OBJFOCAL_LEN & 0xff,         // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN >> 8,
	UVC_IT_INPUT_CTRL_SIZE,                   //bmControls
	UVC_IT_CONTROLS  & 0xff,            //no controls supported
	UVC_IT_CONTROLS >> 8,
	UVC_IT_CONTROLS >> 16,

	//----- [Output terminal Descriptor ] ----- (length 9)
	USB_UVC_OT_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_OUTPUT_TERMINAL,                 // descriptor Subtype
	USB_UVC_ID2,                        // ID of this terminal
	TT_STREAMING & 0xff,                // terminal type
	TT_STREAMING >> 8,
	UVC_OT_ASSOC_TERMINAL,              // no association
	USB_UVC_ID4,                        // bSourceID
	UVC_OT_ITERMINAL_STR_INDEX,             // index of string descriptor (unused)

	//----- [Select unit Descriptor ] ----- (length 7)
	USB_UVC_SU_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_SELECTOR_UNIT,                   // descriptor Subtype
	USB_UVC_ID3,                        // ID of this terminal
	UVC_SU_PIN_NUMBERS,                 // number of this input pin
	USB_UVC_ID1,                        // Source input 1
	UVC_SU_SELECT_STR_INDEX,                // unused

	//----- [Processing unit Descriptor ] ----- (length 11)
	USB_UVC_PU_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_PROCESSING_UNIT,                 // descriptor Subtype
	USB_UVC_ID4,                        // ID of this unit
	USB_UVC_ID3,                        // Connect to ID x
	UVC_PU_MAX_MULTIPLIER & 0xff,       // unused
	UVC_PU_MAX_MULTIPLIER >> 8,         // unused
	UVC_PU_CTRL_SIZE,                   // control size
	UVC_PU_CONTROLS  & 0xff,            // no controls supported
	UVC_PU_CONTROLS >> 8,
	UVC_PU_PROCESS_STR_INDEX,            // unused

	//----- [Standard interrupt EP Descriptor ] ----- (length 7)
	USB_BULK_EP_LENGTH,                 // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,       // descriptor type
	USB_EP_EP3I_ADDRESS,                // EP address
	USB_EP_ATR_INTERRUPT,               // transfer type
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff, // max packet size
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,   // max packet size
	USB_EP_INT_INTERVAL,                // interval

	//----- [Class -spec interrupt EP Descriptor ] ----- (length 5)
	USB_UVC_EP_LENGTH,                  // descriptor size
	CS_ENDPOINT,                        // descriptor type
	EP_INTERRUPT,                       // descriptor subtype
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff, // max packet size
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,   // max packet size

	//----- [Standard VS interface Descriptor] ----- (length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(UVAC_IF_IF0_NUMBER + 1),        // interface number
	UVAC_IF_ALT0,                    // intfAltSet setting
	UVC_VS_IF0_NUMBER_EP,               // number of endpoint
	CC_VIDEO,                           // interface class
	SC_VIDEOSTREAMING,                  // interface subclass
	PC_PROTOCOL_UNDEFINED,              // interface protocol
	UVC_VS_INTERFACE_STR_INDEX,             // interface string id

	//----- [Standard Iso EP Descriptor ] ----- (length 7)
	USB_BULK_EP_LENGTH,                 // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,       // descriptor type
	USB_EP_EP1I_ADDRESS,                // endpoint address
	//#NT#2012/12/07#Lily Kao -begin
	//#NT#IsoIn Txf
	USB_EP_ATR_ISO,                    // transfer mode
	USB_HS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff,    // max packet size
	USB_HS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,    // max packet size
	USB_EP_ISO_INTERVAL,


	//----- [Class spec VS Header input Descriptor ] ----- (length 14)
	USB_UVC_VS_INPUT_HEADER_LENGTH,     // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_INPUT_HEADER,                    // descriptor subtype
	USB_UVC_FORMAT_NUM,                 // how many format descriptors follow
	(USB_UVC_TOTAL_VS_LENGTH_FB_H264) & 0xff, // Total size of VS header + format +frame
	(USB_UVC_TOTAL_VS_LENGTH_FB_H264) >> 8,
	USB_EP_EP1I_ADDRESS,                // Data endpoint address
	UVC_VS_BMINFO,                      // dynamic format change supported (bmInfo)
	USB_UVC_ID2,                        // Ouput terminal ID
	UVC_VS_STILL_CAPTURE,               // StillCapture Method (1,2,3)
	UVC_VS_HW_TRIGGER,                  // HW trigger supported
	UVC_VS_HW_TRIGGER_USAGE,            // HW trigger usage
	UVC_VS_CTRL_SIZE,                   // size of the bmaControls filed
	UVC_VS_CONTROLS,                    // bmaControls

	//----- [Class spec VS Format Descriptor ] -----  (length 28)
	//FRAME-BASED FORMAT
	USB_UVC_VS_FBH264_FORMAT_LENGTH,           // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FORMAT_FRAME_BASED,                    // descriptor subtype  //VS_FORMAT_H264_SIMULCAST
	UVC_VSFMT_FIRSTINDEX,               // First format descriptor
	UVC_VSFMT_FRAMENUM1_HD,             // frame descriptor numbers
	//16 byte GUID
	0x48,
	0x32,
	0x36,
	0x34,
	0x00,
	0x00,
	0x10,
	0x00,
	0x80,
	0x00,
	0x00,
	0xAA,
	0x00,
	0x38,
	0x9B,
	0x71,

	UVC_BITS_PER_PIXEL,             //bBitsPerPixel
	UVC_VSFMT_DEF_FRM_IDX,               /* Default frame index is 1=>1280x720 */
	0x00,                           /* bAspecRatioX */
	0x00,                           /* bAspecRatioY */
	0x00,                           /* bmInterlaceFlags */
	0x00,                           /* bCopyProtect: duplication unrestricted */
	0x01,                           /* bVariableSize */

	//----- [Class spec VS Frame 1280x720 Descriptor ] ----- (length 38)
	//USB_UVC_HD_WIDTH  ; FRAME-BASED FRAME
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX1,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_HD_WIDTH & 0xff,            // wWidth
	USB_UVC_HD_WIDTH >> 8,              // wWidth
	USB_UVC_HD_HEIGHT & 0xff,           // wHeight
	USB_UVC_HD_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

//----- [Class spec VS Frame 1280x768 Descriptor ] ----- (length 38)
	//USB_UVC_1024x768_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX2,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_1024x768_WIDTH & 0xff,            // wWidth
	USB_UVC_1024x768_WIDTH >> 8,              // wWidth
	USB_UVC_1024x768_HEIGHT & 0xff,           // wHeight
	USB_UVC_1024x768_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 640x480 Descriptor ] ----- (length 38)
	//USB_UVC_VGA_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX3,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_VGA_WIDTH & 0xff,            // wWidth
	USB_UVC_VGA_WIDTH >> 8,              // wWidth
	USB_UVC_VGA_HEIGHT & 0xff,           // wHeight
	USB_UVC_VGA_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 320x240 Descriptor ] ----- (length 38)
	//USB_UVC_QVGA_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX4,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_QVGA_WIDTH & 0xff,            // wWidth
	USB_UVC_QVGA_WIDTH >> 8,              // wWidth
	USB_UVC_QVGA_HEIGHT & 0xff,           // wHeight
	USB_UVC_QVGA_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 1024x576 Descriptor ] ----- (length 38)
	//USB_UVC_1024x576_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX5,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_1024x576_WIDTH & 0xff,            // wWidth
	USB_UVC_1024x576_WIDTH >> 8,              // wWidth
	USB_UVC_1024x576_HEIGHT & 0xff,           // wHeight
	USB_UVC_1024x576_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 800x600 Descriptor ] ----- (length 38)
	//USB_UVC_800x600_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX6,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_800x600_WIDTH & 0xff,            // wWidth
	USB_UVC_800x600_WIDTH >> 8,              // wWidth
	USB_UVC_800x600_HEIGHT & 0xff,           // wHeight
	USB_UVC_800x600_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 800x480 Descriptor ] ----- (length 38)
	//USB_UVC_800x480_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX7,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_800x480_WIDTH & 0xff,            // wWidth
	USB_UVC_800x480_WIDTH >> 8,              // wWidth
	USB_UVC_800x480_HEIGHT & 0xff,           // wHeight
	USB_UVC_800x480_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 640x360 Descriptor ] ----- (length 38)
	//USB_UVC_640x360_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX8,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_640x360_WIDTH & 0xff,            // wWidth
	USB_UVC_640x360_WIDTH >> 8,              // wWidth
	USB_UVC_640x360_HEIGHT & 0xff,           // wHeight
	USB_UVC_640x360_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Color matching Descriptor ] ----- (length 6)
	USB_UVC_VS_COLOR_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_COLORFORMAT,                     // descriptor subtype
	UVC_COLOR_PRIMARIES,
	UVC_COLOR_TRANSFER,
	UVC_COLOR_MATRIX
};
_ALIGNED(4) UINT8 g_U2UVACHSOtherConfigDescFBH264[USB_UVC_TOTAL_LENGTH_FB_H264] = {
	//----- [Configuration Descriptor]----- (Length 9)
	USB_CFG_LENGTH,                     // descriptor size
	USB_OTHER_SPEED_CONFIGURATION,
	(USB_UVC_TOTAL_LENGTH_FB_H264) & 0xff,   // total length
	(USB_UVC_TOTAL_LENGTH_FB_H264) >> 8,
	USB_PCC_UVC_NUMBER_IF,              // number of interface
	UVAC_CFGA_CFG_VALUE,             // configuration value
	UVAC_CFGA_CFG_IDX,               // configuration string id
	UVAC_CFGA_CFG_ATTRIBUES,         // characteristics
	UVAC_CFGA_MAX_POWER,             // maximum power in 2mA

	//----- [   IAD Descriptor  ]----- (Length 8)
	USB_UVC_IAD_LENGTH,                 // descriptor size
	USB_IAD_DESCRIPTOR_TYPE,            // descriptor type
	USB_PCC_IAD_FIRST_VID_IF,               // bFirstInterface
	USB_PCC_IAD_IF_COUNT,               // Interface counts
	CC_VIDEO,                           // bFunction class
	SC_VIDEO_INTERFACE_COLLECTION,      // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,              // bFunction Protocol
	UVAC_IAD_STR_INDEX,              //iFunction

	//----- [ Standard VC interface Descriptor ]----- (Length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	UVAC_IF_IF0_NUMBER,              // interface number
	UVAC_IF_ALT0,                    // intfAltSet setting
	UVAC_IF_CFGA_IF0_NUMBER_EP,      // number of endpoint
	CC_VIDEO,                           // bFunction class
	SC_VIDEOCONTROL,                    // bFunction Subclass
	PC_PROTOCOL_UNDEFINED,              // bFunction Protocol
	UVAC_IF_IDX,                     // interface string id

	//----- [Class-spec. VC interface Descriptor ] ----- (length 13)
	USB_UVC_VC_IF_LENGTH,               // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_HEADER,                          // descriptor Subtype
	USB_UVC_RELEASE_NUMBER  & 0xff,     // UVC version
	USB_UVC_RELEASE_NUMBER >> 8,
	USB_UVC_VC_TOTAL_LENGTH & 0xff,     // Total Size of class-spec. descriptor
	USB_UVC_VC_TOTAL_LENGTH >> 8,
	UVC_VC_CLOCK_FREQUENCY & 0xff,      // dwClockFrequency 6MHz (Use of this field has been deprecated)
	(UVC_VC_CLOCK_FREQUENCY >> 8) & 0xff,
	(UVC_VC_CLOCK_FREQUENCY >> 16) & 0xff,
	(UVC_VC_CLOCK_FREQUENCY >> 24) & 0xff,
	UVC_VC_VIDEOSTREAM_TOTAL,           // bInCollection (Number of streaming interfaces)
	UVC_VC_VIDEOSTREAM_FIRSTNUM,        // VS I/F 1 belongs to the VC I/F

	//----- [Camera terminal Descriptor ] ----- (length 17)
	USB_UVC_IT_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_INPUT_TERMINAL,                  // descriptor Subtype
	USB_UVC_ID1,                        // ID of this terminal
	ITT_CAMERA & 0xff,                  // terminal type
	ITT_CAMERA >> 8,
	UVC_IT_ASSOC_TERMINAL,              // no association
	UVC_IT_ITERMINAL_STR_INDEX,             // index of string descriptor (unused)
	UVC_IT_OBJFOCAL_LEN_MIN  & 0xff,    // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN_MIN >> 8,
	UVC_IT_OBJFOCAL_LEN_MAX  & 0xff,    // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN_MAX >> 8,
	UVC_IT_OBJFOCAL_LEN & 0xff,         // no optical zoom supported
	UVC_IT_OBJFOCAL_LEN >> 8,
	UVC_IT_INPUT_CTRL_SIZE,                   //bmControls
	UVC_IT_CONTROLS  & 0xff,            //no controls supported
	UVC_IT_CONTROLS >> 8,

	//----- [Output terminal Descriptor ] ----- (length 9)
	USB_UVC_OT_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_OUTPUT_TERMINAL,                 // descriptor Subtype
	USB_UVC_ID2,                        // ID of this terminal
	TT_STREAMING & 0xff,                // terminal type
	TT_STREAMING >> 8,
	UVC_OT_ASSOC_TERMINAL,              // no association
	USB_UVC_ID4,                        // bSourceID
	UVC_OT_ITERMINAL_STR_INDEX,             // index of string descriptor (unused)

	//----- [Select unit Descriptor ] ----- (length 7)
	USB_UVC_SU_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_SELECTOR_UNIT,                   // descriptor Subtype
	USB_UVC_ID3,                        // ID of this terminal
	UVC_SU_PIN_NUMBERS,                 // number of this input pin
	USB_UVC_ID1,                        // Source input 1
	UVC_SU_SELECT_STR_INDEX,                // unused

	//----- [Processing unit Descriptor ] ----- (length 11)
	USB_UVC_PU_LENGTH,                  // descriptor size
	CS_INTERFACE,                       // descriptor type
	VC_PROCESSING_UNIT,                 // descriptor Subtype
	USB_UVC_ID4,                        // ID of this unit
	USB_UVC_ID3,                        // Connect to ID x
	UVC_PU_MAX_MULTIPLIER & 0xff,       // unused
	UVC_PU_MAX_MULTIPLIER >> 8,         // unused
	UVC_PU_CTRL_SIZE,                   // control size
	UVC_PU_CONTROLS  & 0xff,            // no controls supported
	UVC_PU_CONTROLS >> 8,
	UVC_PU_PROCESS_STR_INDEX,            // unused

	//----- [Standard interrupt EP Descriptor ] ----- (length 7)
	USB_BULK_EP_LENGTH,                 // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,       // descriptor type
	USB_EP_EP3I_ADDRESS,                // EP address
	USB_EP_ATR_INTERRUPT,               // transfer type
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff, // max packet size
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,   // max packet size
	USB_EP_INT_INTERVAL,                // interval

	//----- [Class -spec interrupt EP Descriptor ] ----- (length 5)
	USB_UVC_EP_LENGTH,                  // descriptor size
	CS_ENDPOINT,                        // descriptor type
	EP_INTERRUPT,                       // descriptor subtype
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff, // max packet size
	UVAC_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,   // max packet size

	//----- [Standard VS interface Descriptor] ----- (length 9)
	USB_IF_LENGTH,                      // descriptor size
	USB_INTERFACE_DESCRIPTOR_TYPE,      // descriptor type
	(UVAC_IF_IF0_NUMBER + 1),        // interface number
	UVAC_IF_ALT0,                    // intfAltSet setting
	UVC_VS_IF0_NUMBER_EP,               // number of endpoint
	CC_VIDEO,                           // interface class
	SC_VIDEOSTREAMING,                  // interface subclass
	PC_PROTOCOL_UNDEFINED,              // interface protocol
	UVC_VS_INTERFACE_STR_INDEX,             // interface string id

	//----- [Standard Iso EP Descriptor ] ----- (length 7)
	USB_BULK_EP_LENGTH,                 // descriptor size
	USB_ENDPOINT_DESCRIPTOR_TYPE,       // descriptor type
	USB_EP_EP1I_ADDRESS,                // endpoint address

	//#NT#2012/12/07#Lily Kao -begin
	//#NT#IsoIn Txf
	USB_EP_ATR_ISO,                    // transfer mode
	USB_FS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff,    // max packet size
	USB_FS_ISOIN_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,    // max packet size
	USB_EP_ISO_INTERVAL,
	//#NT#2012/12/07#Lily Kao -end

	//----- [Class spec VS Header input Descriptor ] ----- (length 14)
	USB_UVC_VS_INPUT_HEADER_LENGTH,     // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_INPUT_HEADER,                    // descriptor subtype
	USB_UVC_FORMAT_NUM,                 // how many format descriptors follow
	(USB_UVC_TOTAL_VS_LENGTH_FB_H264) & 0xff, // Total size of VS header + format +frame
	(USB_UVC_TOTAL_VS_LENGTH_FB_H264) >> 8,
	USB_EP_EP1I_ADDRESS,                // Data endpoint address
	UVC_VS_BMINFO,                      // dynamic format change supported (bmInfo)
	USB_UVC_ID2,                        // Ouput terminal ID
	UVC_VS_STILL_CAPTURE,               // StillCapture Method (1,2,3)
	UVC_VS_HW_TRIGGER,                  // HW trigger supported
	UVC_VS_HW_TRIGGER_USAGE,            // HW trigger usage
	UVC_VS_CTRL_SIZE,                   // size of the bmaControls filed
	UVC_VS_CONTROLS,                    // bmaControls

	//----- [Class spec VS Format Descriptor ] -----  (length 28)
	//----- [Class spec VS Format Descriptor ] -----  (length 11)
	//FRAME-BASED FORMAT
	USB_UVC_VS_FBH264_FORMAT_LENGTH,           // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FORMAT_FRAME_BASED,                    // descriptor subtype  //VS_FORMAT_H264_SIMULCAST
	UVC_VSFMT_FIRSTINDEX,               // First format descriptor
	UVC_VSFMT_FRAMENUM1_HD,             // frame descriptor numbers
	//16 byte GUID
	0x48,
	0x32,
	0x36,
	0x34,
	0x00,
	0x00,
	0x10,
	0x00,
	0x80,
	0x00,
	0x00,
	0xAA,
	0x00,
	0x38,
	0x9B,
	0x71,

	UVC_BITS_PER_PIXEL,             //bBitsPerPixel
	UVC_VSFMT_DEF_FRM_IDX,               /* Default frame index is 1=>1280x720 */
	0x00,                           /* bAspecRatioX */
	0x00,                           /* bAspecRatioY */
	0x00,                           /* bmInterlaceFlags */
	0x00,                           /* bCopyProtect: duplication unrestricted */
	0x01,                           /* bVariableSize */

	//----- [Class spec VS Frame 1280x720 Descriptor ] ----- (length 38)
	//USB_UVC_HD_WIDTH  ; FRAME-BASED FRAME
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX1,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_HD_WIDTH & 0xff,            // wWidth
	USB_UVC_HD_WIDTH >> 8,              // wWidth
	USB_UVC_HD_HEIGHT & 0xff,           // wHeight
	USB_UVC_HD_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate


	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 1280x768 Descriptor ] ----- (length 38)
	//USB_UVC_1024x768_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX2,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_1024x768_WIDTH & 0xff,            // wWidth
	USB_UVC_1024x768_WIDTH >> 8,              // wWidth
	USB_UVC_1024x768_HEIGHT & 0xff,           // wHeight
	USB_UVC_1024x768_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 640x480 Descriptor ] ----- (length 38)
	//USB_UVC_VGA_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX3,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_VGA_WIDTH & 0xff,            // wWidth
	USB_UVC_VGA_WIDTH >> 8,              // wWidth
	USB_UVC_VGA_HEIGHT & 0xff,           // wHeight
	USB_UVC_VGA_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 320x240 Descriptor ] ----- (length 38)
	//USB_UVC_QVGA_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX4,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_QVGA_WIDTH & 0xff,            // wWidth
	USB_UVC_QVGA_WIDTH >> 8,              // wWidth
	USB_UVC_QVGA_HEIGHT & 0xff,           // wHeight
	USB_UVC_QVGA_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 1024x576 Descriptor ] ----- (length 38)
	//USB_UVC_1024x576_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX5,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_1024x576_WIDTH & 0xff,            // wWidth
	USB_UVC_1024x576_WIDTH >> 8,              // wWidth
	USB_UVC_1024x576_HEIGHT & 0xff,           // wHeight
	USB_UVC_1024x576_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 800x600 Descriptor ] ----- (length 38)
	//USB_UVC_800x600_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX6,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_800x600_WIDTH & 0xff,            // wWidth
	USB_UVC_800x600_WIDTH >> 8,              // wWidth
	USB_UVC_800x600_HEIGHT & 0xff,           // wHeight
	USB_UVC_800x600_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,


	//----- [Class spec VS Frame 800x480 Descriptor ] ----- (length 38)
	//USB_UVC_800x480_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX7,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_800x480_WIDTH & 0xff,            // wWidth
	USB_UVC_800x480_WIDTH >> 8,              // wWidth
	USB_UVC_800x480_HEIGHT & 0xff,           // wHeight
	USB_UVC_800x480_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Class spec VS Frame 640x360 Descriptor ] ----- (length 38)
	//USB_UVC_640x360_WIDTH
	USB_UVC_VS_FBH264_FRAME_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_FRAME_FRAME_BASED,                     // descriptor subtype
	UVC_VSFRM_INDEX8,               // Index of Frame Descriptor
	USB_UVC_VS_FBH264_CAPABILITIES,     // bmCapabilities
	USB_UVC_640x360_WIDTH & 0xff,            // wWidth
	USB_UVC_640x360_WIDTH >> 8,              // wWidth
	USB_UVC_640x360_HEIGHT & 0xff,           // wHeight
	USB_UVC_640x360_HEIGHT >> 8,             // wHeight

	USB_UVC_VS_FBH264_MIN_BIT_RATE & 0xff,        //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 8) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 16) & 0xff, //dwMinBitRate
	(USB_UVC_VS_FBH264_MIN_BIT_RATE >> 24) & 0xff, //dwMinBitRate

	USB_UVC_VS_FBH264_MAX_BIT_RATE & 0xff,        //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 8) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 16) & 0xff, //dwMaxBitRate
	(USB_UVC_VS_FBH264_MAX_BIT_RATE >> 24) & 0xff, //dwMaxBitRate

	UVC_VSFRM_FRAMEINTERVAL_DEF & 0xff,      // Default frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_DEF >> 24) & 0xff,
	UVC_VSFRM_INTERVAL_TYPE,                 // interval type (continuous frame interval)
	USB_UVC_VS_FBH264_BYTES_PER_LINE & 0xff,        //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16) & 0xff, //dwBytesPerLine
	(USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24) & 0xff, //dwBytesPerLine
	UVC_VSFRM_FRAMEINTERVAL_MIN & 0xff,      // Min frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MIN >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_MAX & 0xff,      // Max frame interval 666666ns /15fps
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_MAX >> 24) & 0xff,
	UVC_VSFRM_FRAMEINTERVAL_STEP & 0xff,      // frame interval step
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 8) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 16) & 0xff,
	(UVC_VSFRM_FRAMEINTERVAL_STEP >> 24) & 0xff,

	//----- [Color matching Descriptor ] ----- (length 6)
	USB_UVC_VS_COLOR_LENGTH,            // descriptor size
	CS_INTERFACE,                       // descriptor type
	VS_COLORFORMAT,                     // descriptor subtype
	UVC_COLOR_PRIMARIES,
	UVC_COLOR_TRANSFER,
	UVC_COLOR_MATRIX
};
#ifdef __LINUX_USER__
_ALIGNED(64) static UINT8 gUVCHSStrgConfigDesc[USB_STRG_CFGA_TOTAL_LENGTH] = {
	//----- cfg A interface0 ----
	//USB_INTERFACE_DESCRIPTOR, 9 bytes for Mass Storage
	// 0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,

	//USB_ENDPOINT_DESCRIPTOR, ep1, bulk-in, packet size=64 bytes
	// 0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00
	//---- EP1 IN ----
	USB_EP_LENGTH,                          /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,       /* descriptor type      */
	(USB_EP_IN_ADDRESS | MSDC_IN_EP), /* Msdc_Open Runtime determined *//* endpoint address     */
	USB_EP_ATR_BULK,                    /* character address    */
	USB_HS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff,  /* max packet size      */
	USB_HS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,    /* max packet size      */
	USB_EP_BULK_INTERVAL,

	//USB_ENDPOINT_DESCRIPTOR, ep2, bulk-out, packet size=64 bytes
	//0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,
	//---- EP2 OUT ----
	USB_EP_LENGTH,                          /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,       /* descriptor type      */
	(USB_EP_OUT_ADDRESS | MSDC_OUT_EP), /* Msdc_Open Runtime determined *//* endpoint address     */
	USB_EP_ATR_BULK,                    /* character address    */
	USB_HS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE & 0xff,  /* max packet size      */
	USB_HS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE >> 8,    /* max packet size      */
	USB_EP_BULK_INTERVAL
};
#endif

#if (_UVC_DBG_LVL_ == _UVC_DBG_ALL_)
static void UVAC_DbgDmpConfigDesc(UINT8 *pUvacHSConfigDesc, UINT16 descLen)
{
	UINT16 i = 0;
	UINT8 *ptr = pUvacHSConfigDesc;
	DBG_DUMP("%s ==> %d\r\n", __func__, descLen);
	if ((0 == ptr) || (0 == descLen)) {
		DBG_ERR("Input NULL\r\n");
		return;
	}
	while (i < descLen) {
		if (0 == (i % 16)) {
			DBG_DUMP("\r\n[%8d]", i);
		}
		DBG_DUMP("  0x%x", *pUvacHSConfigDesc++);
		i++;
	}
	DBG_DUMP("<===========\r\n");

}
#else
static void UVAC_DbgDmpConfigDesc(UINT8 *pUvacHSConfigDesc, UINT16 descLen) {}
#endif
static UINT32 UVAC_GetFrmRateInterval(UINT32 frmRate)
{
	UINT32 frameInterval = UVC_VSFRM_FRAMEINTERVAL_DEF;
	if (frmRate) {
		frameInterval = 10000000 / frmRate;
	}
	return frameInterval;
}

//Make Standard Video-Stream or Audio-Stream Interface Descriptor
static UINT16 UVAC_MakeStrmIntfDesc(UINT8 *ptr, UINT8 intfNum, UINT8 intfAltSet, UINT8 numOfEP, UINT8 funcClass)
{
	UINT8 *pCurr = ptr;
	DBG_IND("intfNum=0x%x, intfAltSet=0x%x, numOfEP=0x%x, funcClass=0x%x\r\n", intfNum, intfAltSet, numOfEP, funcClass);
	*pCurr++ = USB_IF_LENGTH;
	*pCurr++ = USB_INTERFACE_DESCRIPTOR_TYPE;
	*pCurr++ = intfNum;
	*pCurr++ = intfAltSet;
	*pCurr++ = numOfEP;
	*pCurr++ = funcClass;
	if (CC_VIDEO == funcClass) {
		*pCurr++ = SC_VIDEOSTREAMING;
		if (gUvacUvcVer == UVAC_UVC_VER_150) {
		*pCurr++ = PC_PROTOCOL_15;
		} else {
			*pCurr++ = PC_PROTOCOL_UNDEFINED;
		}
		*pCurr++ = UVC_VS_INTERFACE_STR_INDEX;
	} else if (CC_AUDIO == funcClass) {
		*pCurr++ = SC_AUDIOSTREAMING;
		*pCurr++ = PC_PROTOCOL_UNDEFINED;
		*pCurr++ = UVAC_IF_IDX;
	} else {
		DBG_ERR("Unknown Function Class=0x%x\r\n", funcClass);
		*pCurr++ = 0;
	}
	return USB_IF_LENGTH;
}
//Class-specific AS General interface Descriptor
static UINT16 UVAC_MakeClsSpecAudStrmIntfDesc(UINT8 *ptr, UINT8 audFmtType)
{
	UINT8 *pCurr = ptr;
	UINT16 audDataFmtCodec = UAC_DATA_FMT_TYPE_I_PCM;
	*pCurr++ = USB_UAC_AS_CS_LENGTH;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = UAC_CS_AS_GENERAL;
	*pCurr++ = UAC_TERMINAL_ID_OT;
	*pCurr++ = UAC_CS_AS_DELAY;
	if (UAC_FMT_TYPE_I == audFmtType) {
		audDataFmtCodec = UAC_DATA_FMT_TYPE_I_PCM;
	} else if (UAC_FMT_TYPE_II == audFmtType) {
		audDataFmtCodec = UAC_DATA_FMT_TYPE_II_MPEG;
	} else if (UAC_FMT_TYPE_III == audFmtType) {
		audDataFmtCodec = UAC_DATA_FMT_TYPE_III_MPEG2_NOEXT;
	} else {
		DBG_ERR("Unknow audFmtType=0x%x\r\n", audFmtType);
	}
	*pCurr++ = (UINT8)(audDataFmtCodec & 0xFF);
	*pCurr++ = (UINT8)((audDataFmtCodec >> 8) & 0xFF);

	return USB_UAC_AS_CS_LENGTH;
}
static UINT16 UVAC_MakeAudTypeFmtDesc(UINT8 *ptr, UINT8 audFmtType, UINT8 sampleRateIdx)
{
	UINT8 *pCurr = ptr;
	UINT8 tmpLen = 0;
	//UINT8 *pTmp;
	UINT32 tmpVal = 0;

	if (UAC_FMT_TYPE_I == audFmtType) {
		tmpLen = UVAC_AUD_TYPE_FMT_I_BASE_LEN;
		*pCurr++ = 0;
		*pCurr++ = CS_INTERFACE;
		*pCurr++ = UAC_CS_FORMAT_TYPE;
		*pCurr++ = audFmtType;
		*pCurr++ = gU2UacChNum;
		*pCurr++ = UAC_BYTE_EACH_SUBFRAME;
		*pCurr++ = UAC_BIT_PER_SECOND;
		*pCurr++ = UAC_FREQ_TYPE_CNT;
		//pTmp = pCurr;
	} else if (UAC_FMT_TYPE_II == audFmtType) {
		tmpLen = UVAC_AUD_TYPE_FMT_II_BASE_LEN;
		*pCurr++ = 0;
		*pCurr++ = CS_INTERFACE;
		*pCurr++ = UAC_CS_FORMAT_TYPE;
		*pCurr++ = audFmtType;
		*pCurr++ = (UINT8)UVAC_AUD_MAX_BPS_48K;
		*pCurr++ = UVAC_AUD_MAX_BPS_48K >> 8;
		*pCurr++ = (UINT8)UVAC_SAMPLES_PER_FRAME;
		*pCurr++ = UVAC_SAMPLES_PER_FRAME >> 8;
		*pCurr++ = UAC_FREQ_TYPE_CNT;
		//pTmp = pCurr;
	} else if (UAC_FMT_TYPE_III == audFmtType) {
		tmpLen = UVAC_AUD_TYPE_FMT_III_BASE_LEN;
		*pCurr++ = 0;
		*pCurr++ = CS_INTERFACE;
		*pCurr++ = UAC_CS_FORMAT_TYPE;
		*pCurr++ = audFmtType;
		*pCurr++ = gU2UacChNum;
		*pCurr++ = UAC_BYTE_EACH_SUBFRAME;
		*pCurr++ = UAC_BIT_PER_SECOND;//USB_UAC_TYPE_III_SUBFRM_BITRESO;
		*pCurr++ = UAC_FREQ_TYPE_CNT;
		//pTmp = pCurr;
	}
	DBG_IND("+Aud[%d] SampleRate=%d\r\n", sampleRateIdx, tmpVal);
	tmpVal = gU2UvacAudSampleRate[sampleRateIdx];
	DBG_IND("-Aud[%d] SampleRate=%d\r\n", sampleRateIdx, tmpVal);
	if (tmpVal) {
		*pCurr++ = tmpVal;
		*pCurr++ = tmpVal >> 8;
		*pCurr++ = tmpVal >> 16;
	} else {
		DBG_ERR("Aud SampleRate %d NULL\r\n", sampleRateIdx);
	}
	tmpLen += 3;
	if (UAC_FMT_TYPE_II == audFmtType) {
		tmpLen += UVAC_MPEG_FMT_LENGTH;
		*pCurr++ = UVAC_MPEG_FMT_LENGTH;
		*pCurr++ = CS_INTERFACE;
		*pCurr++ = UAC_CS_FORMAT_SPECIFIC;
		*pCurr++ = CS_INTERFACE;
		*pCurr++ = (UINT8)UAC_DATA_FMT_TYPE_II_MPEG;       //UAC_DATA_FMT_TYPE_MPEG2NOEXT
		*pCurr++ = (UINT8)UAC_DATA_FMT_TYPE_II_MPEG >> 8;  //UAC_DATA_FMT_TYPE_MPEG2NOEXT >> 8;
		*pCurr++ = UAC_MPEG_CAP;
		*pCurr++ = UAC_MPEG_CAP >> 8;
		*pCurr++ = UAC_MPEG_FEATURE;
	}
	*ptr = tmpLen;
	return tmpLen;
}
static UINT16 UVAC_MakeEPDesc(UINT8 *ptr, UINT8 epAddr, UINT8 attribute, UINT16 packetSize, UINT8 intval)
{
	UINT8 *pCurr = ptr;
	DBG_IND("epAddr=0x%x, attribute=0x%x, packetSize=0x%x, intval=0x%x\r\n", epAddr, attribute, packetSize, intval);
	*pCurr++ = USB_BULK_EP_LENGTH;
	*pCurr++ = USB_ENDPOINT_DESCRIPTOR_TYPE;
	*pCurr++ = epAddr;
	*pCurr++ = attribute;
	*pCurr++ = packetSize;
	*pCurr++ = packetSize >> 8;
	*pCurr++ = intval;
	return USB_BULK_EP_LENGTH;
}
static UINT16 UVAC_MakeAudEPDesc(UINT8 *ptr, UINT8 epAddr, UINT8 attribute, UINT16 packetSize, UINT8 intval)
{
	UINT8 *pCurr = ptr;
	UINT16 tmpVal = UVAC_MakeEPDesc(pCurr, epAddr, attribute, packetSize, intval);
	*pCurr = (*pCurr + 2);
	pCurr += tmpVal;
	*pCurr++ = USB_UAC_EP_REFRESH;
	*pCurr++ = USB_UAC_EP_SYNC_ADDR;
	tmpVal += 2;
	return tmpVal;
}
//Class-spec. Endpoint General Descriptor
static UINT16 UVAC_MakeClsSpecAudEPDesc(UINT8 *ptr)
{
	UINT8 *pCurr = ptr;
	*pCurr ++ = USB_UAC_CS_ENDPOINT_GENERAL_LENGTH;
	*pCurr ++ = CS_ENDPOINT;
	*pCurr ++ = UAC_CS_AS_GENERAL;
	*pCurr ++ = UVAC_AUD_EP_CLS_SPEC_ATTR;
	*pCurr ++ = USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAYUNIT;
	*pCurr ++ = (UINT8)USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY;
	*pCurr ++ = USB_UAC_ISO_DATA_ENDPOINT_LOCKDELAY >> 8;
	return USB_UAC_CS_ENDPOINT_GENERAL_LENGTH;
}
//Make IAD Descriptor for video or audio
static UINT16 UVAC_MakeIADDesc(UINT8 *ptr, UINT8 firstIntfId, UINT8 intfCnt, UINT8 funcClass)
{
	UINT8 *pCurr = ptr;
	DBG_IND("ptr=0x%x, intf=%d, intfCnt=%d, funcClass=0x%x\r\n", ptr, firstIntfId, intfCnt, funcClass);
	*pCurr++ = UVAC_IAD_LENGTH;
	*pCurr++ = USB_IAD_DESCRIPTOR_TYPE;
	*pCurr++ = firstIntfId;
	*pCurr++ = intfCnt;
	*pCurr++ = funcClass;
	if (CC_VIDEO == funcClass) {
		*pCurr++ = SC_VIDEO_INTERFACE_COLLECTION;
	} else if (CC_AUDIO == funcClass) {
		*pCurr++ = SC_AUDIOSTREAMING;
	} else if (CC_CDC_COMM == funcClass) {
		*pCurr++ = USB_CDC_DEV_CLASS;
	} else if (SIDC_IF_CLASS == funcClass) {
		*pCurr++ = SIDC_IF_SUBCLASS;
	} else if (USB_STRG_IF_CLASS_STORAGE == funcClass) {
		*pCurr++ = USB_STRG_IF_SUBCLASS_SCSI;
	} else {
		DBG_ERR("Unknown Function Class=0x%x\r\n", funcClass);
		*pCurr++ = 0;
	}
	if (SIDC_IF_CLASS == funcClass) {
		*pCurr++ = SIDC_IF_PROTOCOL;
	} else {
		*pCurr++ = PC_PROTOCOL_UNDEFINED;
	}
	*pCurr++ = UVAC_IAD_STR_INDEX;
	return UVAC_IAD_LENGTH;
}
//Make Standard Control Interface Descriptor for video or audio
static UINT16 UVAC_MakeStandIntrfDesc(UINT8 *ptr, UINT8 firstIntfId, UINT8 funcClass, UINT8 numEP)
{
	UINT8 *pCurr = ptr;
	DBG_IND("ptr=0x%x, intf=%d, funcClass=0x%x\r\n", ptr, firstIntfId, funcClass);
	*pCurr++ = USB_IF_LENGTH;
	*pCurr++ = USB_INTERFACE_DESCRIPTOR_TYPE;
	*pCurr++ = firstIntfId;
	*pCurr++ = UVAC_IF_ALT0;
	*pCurr++ = numEP;
	*pCurr++ = funcClass;
	if (CC_VIDEO == funcClass) {
		*pCurr++ = SC_VIDEOCONTROL;
		if (gUvacUvcVer == UVAC_UVC_VER_150) {
			*pCurr++ = PC_PROTOCOL_15;
		} else {
		*pCurr++ = PC_PROTOCOL_UNDEFINED;
		}
		*pCurr++ = uvc_string_idx;
	} else if (CC_AUDIO == funcClass) {
		*pCurr++ = SC_AUDIOCONTROL;
		*pCurr++ = PC_PROTOCOL_UNDEFINED;
		*pCurr++ = uac_string_idx;
	} else if (CC_CDC_COMM == funcClass) {
		*pCurr++ = USB_CDC_IF_SUBCLASS_ACM;
		*pCurr++ = USB_CDC_IF_PROTOCOL_AT_CMD;
		*pCurr++ = UVAC_IF_IDX;
	} else if (CC_CDC_DATA == funcClass) {
		*pCurr++ = USB_CDC_IF_SUBCLASS_NONE;
		*pCurr++ = PC_PROTOCOL_UNDEFINED;
		*pCurr++ = UVAC_IF_IDX;
	} else if (SIDC_IF_CLASS == funcClass) {
		*pCurr++ = SIDC_IF_SUBCLASS;
		*pCurr++ = SIDC_IF_PROTOCOL;
		*pCurr++ = SIDC_IF_IDX;
	} else if (DFU_IF_CLASS == funcClass) {
		*pCurr++ = DFU_IF_SUBCLASS;
		*pCurr++ = DFU_IF_PROTOCOL;
		*pCurr++ = dfu_string_idx;
	} else if (HID_IF_CLASS == funcClass) {
		*pCurr++ = HID_IF_SUBCLASS;
		*pCurr++ = HID_IF_PROTOCOL;
		*pCurr++ = hid_string_idx;
	} else if (USB_STRG_IF_CLASS_STORAGE == funcClass) {
		*pCurr++ = USB_STRG_IF_SUBCLASS_SCSI;
		*pCurr++ = USB_STRG_IF_PROTOCOL_BULK_ONLY;
		*pCurr++ = USB_STRG_IF_IDX;
	} else {
		DBG_WRN("Special or Unknown Function Class=0x%x\r\n", funcClass);
		*pCurr++ = 0;
		*pCurr++ = PC_PROTOCOL_UNDEFINED;
		*pCurr++ = UVAC_IF_IDX;
	}
	return USB_IF_LENGTH;
}
//Make Audio Configuration Descriptor
static UINT16 UVAC_MakeAudioConfigDesc(UINT8 *ptr, UINT8 *pIntfNum, UINT8 strmIntfCnt, UINT8 epAddr, UINT8 audFmtType)
{
	UINT8 firstIntfId = *pIntfNum;
	UINT16 tmpLen = 0, tmpVal = 0;

	UINT8 *pTmp;
	UINT8 *pCurr = ptr;
	UINT8 altSet = 0;
	UINT32 UacPacketSize = 0;
	UINT8 interval;
	DBG_IND("ptr=0x%x, 1stIntf=%d, strmIntfCnt=%d, ep=0x%x\r\n", ptr, firstIntfId, strmIntfCnt, epAddr);

	//IAD Descriptor of Audio configuration
	tmpVal = UVAC_MakeIADDesc(pCurr, firstIntfId, (strmIntfCnt + 1), CC_AUDIO);
	pCurr += tmpVal;

	//Standard AC interface Descriptor
	gU2UvacIntfIdx_AC[gU2UvacIntfIdx_audAryCurrIdx] = firstIntfId;
	DBG_IND("AudCtrl[%d] Intf=%d\r\n", gU2UvacIntfIdx_audAryCurrIdx, gU2UvacIntfIdx_AC[gU2UvacIntfIdx_audAryCurrIdx]);
	tmpVal = UVAC_MakeStandIntrfDesc(pCurr, firstIntfId, CC_AUDIO, UVAC_IF_AC_EP_CNT);
	pCurr += tmpVal;

	//Class-Specific AC interface Descriptor
	tmpLen = USB_IF_LENGTH;
	*pCurr++ = USB_IF_LENGTH;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = UAC_CS_HEADER;
	*pCurr++ = (UINT8)USB_UAC_RELEASE_NUMBER;
	*pCurr++ = USB_UAC_RELEASE_NUMBER >> 8;
	pTmp = pCurr;                               //point to the total length of class-specific interface descriptor
	*pCurr++ = 0;
	*pCurr++ = 0;
	*pCurr++ = strmIntfCnt;
	firstIntfId ++;
	*pCurr++ = firstIntfId;

	//Class-specific AC Input Terminal interface Descriptor
	tmpLen += USB_UAC_IT_LENGTH;
	*pCurr++ = USB_UAC_IT_LENGTH;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = UAC_CS_INPUT_TERMINAL;
	*pCurr++ = UAC_TERMINAL_ID_IT;
	*pCurr++ = (UINT8)UAC_IT_MICROPHONE;
	*pCurr++ = UAC_IT_MICROPHONE >> 8;
	*pCurr++ = UAC_TERMINAL_ID_NULL;
	*pCurr++ = gU2UacChNum;
	*pCurr++ = gU2UacITOutChCfg;
	*pCurr++ = gU2UacITOutChCfg >> 8;
	*pCurr++ = UAC_IT_OUT_CHANNEL_STR_IDX;
	*pCurr++ = UAC_IT_STR_IDX;

	//Class-specific AC Output Terminal interface Descriptor
	tmpLen += USB_UAC_OT_LENGTH;
	*pCurr++ = USB_UAC_OT_LENGTH;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = UAC_CS_OUTPUT_TERMINAL;
	*pCurr++ = UAC_TERMINAL_ID_OT;
	*pCurr++ = (UINT8)UAC_TERMINAL_STREAMING;
	*pCurr++ = UAC_TERMINAL_STREAMING >> 8;
	*pCurr++ = UAC_TERMINAL_ID_NULL;
	*pCurr++ = UAC_TERMINAL_ID_FU;
	*pCurr++ = UAC_OT_STR_IDX;

	//Class-specific AC Feature Unit interface Descriptor
	tmpLen += USB_UAC_FU_LENGTH;
	*pCurr++ = USB_UAC_FU_LENGTH;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = UAC_CS_FEATURE_UNIT;
	*pCurr++ = UAC_TERMINAL_ID_FU;
	*pCurr++ = UAC_TERMINAL_ID_IT;
	*pCurr++ = UAC_FU_CTRL_SIZE;
	*pCurr++ = UAC_FU_CTRL_CONFIG;//lilyttt??
	*pCurr++ = UAC_FU_STR_IDX;

	//Update the total length of class-specific interface descriptor
	*pTmp++  = tmpLen;
	*pTmp    = tmpLen >> 8;

	//Standard AS interface Descriptor, ALT=0, EP-Cnt=0
	gU2UvacIntfIdx_AS[gU2UvacIntfIdx_audAryCurrIdx] = firstIntfId;
	DBG_IND("AudStrm[%d] Intf=%d\r\n", gU2UvacIntfIdx_audAryCurrIdx, gU2UvacIntfIdx_AS[gUvacIntfIdx_audAryCurrIdx]);
	gU2UvacIntfIdx_audAryCurrIdx++;
	altSet = 0;
	tmpVal = UVAC_MakeStrmIntfDesc(pCurr, firstIntfId, altSet, 0, CC_AUDIO);
	pCurr += tmpVal;

	//Standard AS interface Descriptor, ALT=1, EP-Cnt=1
	altSet++;
	tmpVal = UVAC_MakeStrmIntfDesc(pCurr, firstIntfId, altSet, 1, CC_AUDIO);
	pCurr += tmpVal;

	//Class-specific AS General interface Descriptor
	tmpVal = UVAC_MakeClsSpecAudStrmIntfDesc(pCurr, audFmtType);
	pCurr += tmpVal;

	//Audio Type(x) Format Type Descriptor
	tmpVal = UVAC_MakeAudTypeFmtDesc(pCurr, audFmtType, (altSet - 1));
	pCurr += tmpVal;

	//Standard Audio Endpoint Descriptor Descriptor
	if (gUacMaxPacketSize){
		UacPacketSize = gUacMaxPacketSize;
	} else {
		UacPacketSize = gU2UvacAudSampleRate[altSet - 1] * gU2UacChNum * UAC_BIT_PER_SECOND / 8 / 1000; //Audio bit rate(byte per ms).
		UacPacketSize += 4;	//fix for compatibility with MacOS
		if (UacPacketSize < 36) {
			UacPacketSize = 36;
		}
	}
	if (gUacInterval) {
		interval = gUacInterval;
	} else {
		interval = USB_UAC_POLL_INTERVAL;
	}
	tmpVal = UVAC_MakeAudEPDesc(pCurr, epAddr, USB_UAC_ENDPOINT_ATTR, UacPacketSize, interval);
	pCurr += tmpVal;

	//Class-spec. Endpoint General Descriptor
	tmpVal = UVAC_MakeClsSpecAudEPDesc(pCurr);
	pCurr += tmpVal;

	*pIntfNum = firstIntfId;

	return (pCurr - ptr);
}

static UINT16 UVAC_MakeClsSpecAudOStrmIntfDesc(UINT8 *ptr, UINT8 audFmtType)
{
	UINT8 *pCurr = ptr;
	UINT16 audDataFmtCodec = UAC_DATA_FMT_TYPE_I_PCM;
	*pCurr++ = USB_UAC_AS_CS_LENGTH;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = UAC_CS_AS_GENERAL;
	*pCurr++ = UAC_TERMINAL_ID_IT;
	*pCurr++ = UAC_CS_AS_DELAY;
	if (UAC_FMT_TYPE_I == audFmtType) {
		audDataFmtCodec = UAC_DATA_FMT_TYPE_I_PCM;
	} else if (UAC_FMT_TYPE_II == audFmtType) {
		audDataFmtCodec = UAC_DATA_FMT_TYPE_II_MPEG;
	} else if (UAC_FMT_TYPE_III == audFmtType) {
		audDataFmtCodec = UAC_DATA_FMT_TYPE_III_MPEG2_NOEXT;
	} else {
		DBG_ERR("Unknow audFmtType=0x%x\r\n", audFmtType);
	}
	*pCurr++ = (UINT8)(audDataFmtCodec & 0xFF);
	*pCurr++ = (UINT8)((audDataFmtCodec >> 8) & 0xFF);

	return USB_UAC_AS_CS_LENGTH;
}


//Make Audio Configuration Descriptor
static UINT16 UVAC_MakeAudioOutConfigDesc(UINT8 *ptr, UINT8 *pIntfNum, UINT8 strmIntfCnt, UINT8 epAddr, UINT8 audFmtType)
{
	UINT8 firstIntfId = *pIntfNum;
	UINT16 tmpLen = 0, tmpVal = 0;

	UINT8 *pTmp;
	UINT8 *pCurr = ptr;
	UINT8 altSet = 0;
	UINT32 UacPacketSize = 0;
	DBG_IND("ptr=0x%x, 1stIntf=%d, strmIntfCnt=%d, ep=0x%x\r\n", ptr, firstIntfId, strmIntfCnt, epAddr);

	//IAD Descriptor of Audio configuration
	tmpVal = UVAC_MakeIADDesc(pCurr, firstIntfId, (strmIntfCnt + 1), CC_AUDIO);
	pCurr += tmpVal;

	//Standard AC interface Descriptor
	gU2UvacIntfIdx_AC[UVAC_AUD_DEV_CNT_3] = firstIntfId;

	DBG_IND("AudCtrl[%d] Intf=%d\r\n", UVAC_AUD_DEV_CNT_3, gU2UvacIntfIdx_AC[UVAC_AUD_DEV_CNT_3]);
	tmpVal = UVAC_MakeStandIntrfDesc(pCurr, firstIntfId, CC_AUDIO, UVAC_IF_AC_EP_CNT);
	pCurr += tmpVal;

	//Class-Specific AC interface Descriptor
	tmpLen = USB_IF_LENGTH;
	*pCurr++ = USB_IF_LENGTH;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = UAC_CS_HEADER;
	*pCurr++ = (UINT8)USB_UAC_RELEASE_NUMBER;
	*pCurr++ = USB_UAC_RELEASE_NUMBER >> 8;
	pTmp = pCurr;                               //point to the total length of class-specific interface descriptor
	*pCurr++ = 0;
	*pCurr++ = 0;
	*pCurr++ = strmIntfCnt;
	firstIntfId ++;
	*pCurr++ = firstIntfId;

	//Class-specific AC Input Terminal interface Descriptor
	tmpLen += USB_UAC_IT_LENGTH;
	*pCurr++ = USB_UAC_IT_LENGTH;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = UAC_CS_INPUT_TERMINAL;
	*pCurr++ = UAC_TERMINAL_ID_IT;
	//*pCurr++ = (UINT8)UAC_IT_MICROPHONE;
	*pCurr++ = (UINT8)UAC_TERMINAL_STREAMING;
	*pCurr++ = UAC_TERMINAL_STREAMING >> 8;
	*pCurr++ = UAC_TERMINAL_ID_NULL;
	*pCurr++ = gU2UacChNum;
	*pCurr++ = gU2UacITOutChCfg;
	*pCurr++ = gU2UacITOutChCfg >> 8;
	*pCurr++ = UAC_IT_OUT_CHANNEL_STR_IDX;
	*pCurr++ = UAC_IT_STR_IDX;

	//Class-specific AC Output Terminal interface Descriptor
	tmpLen += USB_UAC_OT_LENGTH;
	*pCurr++ = USB_UAC_OT_LENGTH;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = UAC_CS_OUTPUT_TERMINAL;
	*pCurr++ = UAC_TERMINAL_ID_OT;
	*pCurr++ = (UINT8)UAC_OT_SPEAKER;
	//*pCurr++ = (UINT8)UAC_TERMINAL_STREAMING;
	*pCurr++ = UAC_OT_SPEAKER >> 8;
	*pCurr++ = UAC_TERMINAL_ID_NULL;
	*pCurr++ = UAC_TERMINAL_ID_FU;
	*pCurr++ = UAC_OT_STR_IDX;

	//Class-specific AC Feature Unit interface Descriptor
	tmpLen += USB_UAC_FU_LENGTH;
	*pCurr++ = USB_UAC_FU_LENGTH;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = UAC_CS_FEATURE_UNIT;
	*pCurr++ = UAC_TERMINAL_ID_FU;
	*pCurr++ = UAC_TERMINAL_ID_IT;
	*pCurr++ = UAC_FU_CTRL_SIZE;
	*pCurr++ = 1;//lilyttt??
	*pCurr++ = UAC_FU_STR_IDX;

	//Update the total length of class-specific interface descriptor
	*pTmp++  = tmpLen;
	*pTmp    = tmpLen >> 8;

	//Standard AS interface Descriptor, ALT=0, EP-Cnt=0
	gU2UvacIntfIdx_AS[UVAC_AUD_DEV_CNT_3] = firstIntfId;

	DBG_IND("AudStrm[%d] Intf=%d\r\n", UVAC_AUD_DEV_CNT_3, gU2UvacIntfIdx_AS[UVAC_AUD_DEV_CNT_3]);
	altSet = 0;
	tmpVal = UVAC_MakeStrmIntfDesc(pCurr, firstIntfId, altSet, 0, CC_AUDIO);
	pCurr += tmpVal;

	//Standard AS interface Descriptor, ALT=1, EP-Cnt=1
	altSet++;
	tmpVal = UVAC_MakeStrmIntfDesc(pCurr, firstIntfId, altSet, 1, CC_AUDIO);
	pCurr += tmpVal;

	//Class-specific AS General interface Descriptor
	tmpVal = UVAC_MakeClsSpecAudOStrmIntfDesc(pCurr, audFmtType);
	pCurr += tmpVal;

	//Audio Type(x) Format Type Descriptor
	tmpVal = UVAC_MakeAudTypeFmtDesc(pCurr, audFmtType, (altSet - 1));
	pCurr += tmpVal;

	//Standard Audio Endpoint Descriptor Descriptor
	UacPacketSize = gU2UvacAudSampleRate[altSet - 1] * gU2UacChNum * UAC_BIT_PER_SECOND / 8 / 1000; //Audio bit rate(byte per ms).

	tmpVal = UVAC_MakeAudEPDesc(pCurr, epAddr, USB_UAC_ENDPOINT_ATTR, UacPacketSize, USB_UAC_POLL_INTERVAL);
	pCurr += tmpVal;

	//Class-spec. Endpoint General Descriptor
	tmpVal = UVAC_MakeClsSpecAudEPDesc(pCurr);
	pCurr += tmpVal;

	*pIntfNum = firstIntfId;

	return (pCurr - ptr);
}

static UINT16 UVAC_MakeColorMatch(UINT8 *ptr)
{
	UINT8 *pCurr = ptr;
	*pCurr++ = UVAC_VS_COLOR_LENGTH;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = VS_COLORFORMAT;
	*pCurr++ = UVC_COLOR_PRIMARIES;
	*pCurr++ = UVC_COLOR_TRANSFER;
	*pCurr++ = UVC_COLOR_MATRIX;
	return UVAC_VS_COLOR_LENGTH;
}
//Make CDC Configuration Descriptor
static UINT16 UVAC_MakeCdcConfigDesc(CDC_COM_ID ComID, UINT8 *ptr, UINT8 *pIntfNum)
{
	UINT8 firstIntfId = *pIntfNum;
	UINT16 tmpVal = 0;
	UINT8 *pCurr = ptr;
	UINT32 DataInEP, DataOutEP;
#if USB_CDC_IF_COMM_NUMBER_EP
	UINT32 CommInEP;
#endif

	if (CDC_COM_1ST == ComID) {
#if USB_CDC_IF_COMM_NUMBER_EP
		CommInEP = CDC_COMM_IN_EP;
#endif
		DataInEP = CDC_DATA_IN_EP;
		DataOutEP = CDC_DATA_OUT_EP;
	} else {
#if USB_CDC_IF_COMM_NUMBER_EP
		CommInEP = CDC_COMM2_IN_EP;
#endif
		DataInEP = CDC_DATA2_IN_EP;
		DataOutEP = CDC_DATA2_OUT_EP;
	}
	DBG_IND("ptr=0x%x, 1stIntf=%d\r\n", ptr, firstIntfId);
	//IAD Descriptor of CDC configuration
	tmpVal = UVAC_MakeIADDesc(pCurr, firstIntfId, CDC_INTERFACE_NUM, CC_CDC_COMM);
	pCurr += tmpVal;

	gU2UvacIntfIdx_CDC_COMM[ComID] = firstIntfId;
	tmpVal = UVAC_MakeStandIntrfDesc(pCurr, firstIntfId, CC_CDC_COMM, USB_CDC_IF_COMM_NUMBER_EP);
	pCurr += tmpVal;

	//----- Functional Header ----
	*pCurr++ = USB_CDC_FUNCTIONAL_HEADER_LENGTH;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = USB_CS_SUBTYPE_HEADER;
	*pCurr++ = USB_CDC_SPEC_VER_MINOR;
	*pCurr++ = USB_CDC_SPEC_VER_MAJOR;

	//----- Functional ACM ----
	*pCurr++ = USB_CDC_FUNCTIONAL_ACM_LENGTH;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = USB_CS_SUBTYPE_ACM;
	*pCurr++ = USB_CDC_ACM_CAPABILITIES;

	//----- Functional Union ----
	*pCurr++ = USB_CDC_FUNCTIONAL_UNION_LENGTH;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = USB_CS_SUBTYPE_UNION;
	*pCurr++ = firstIntfId;//USB_CDC_MASTER_IF_NUM;
	*pCurr++ = firstIntfId + 1;//USB_CDC_SLAVE_IF_NUM;

#if USB_CDC_IF_COMM_NUMBER_EP
	//------Notification Endpoint
	*pCurr++ = USB_BULK_EP_LENGTH;
	*pCurr++ = USB_ENDPOINT_DESCRIPTOR_TYPE;

	*pCurr++ = (USB_EP_IN_ADDRESS | CommInEP);
	*pCurr++ = USB_EP_ATR_INTERRUPT;
	*pCurr++ = USB_CDC_COMM_EP_INTR_PACKET_SIZE & 0xff;
	*pCurr++ = USB_CDC_COMM_EP_INTR_PACKET_SIZE >> 8;
	*pCurr++ = USB_EP_CDC_INTERRUPT_HSINTERVAL;
#endif

	firstIntfId++;

	gU2UvacIntfIdx_CDC_DATA[ComID] = firstIntfId;
	tmpVal = UVAC_MakeStandIntrfDesc(pCurr, firstIntfId, CC_CDC_DATA, USB_CDC_IF_DATA_NUMBER_EP);
	pCurr += tmpVal;

	*pCurr++ = USB_BULK_EP_LENGTH;
	*pCurr++ = USB_ENDPOINT_DESCRIPTOR_TYPE;
	*pCurr++ = (USB_EP_IN_ADDRESS | DataInEP);
	*pCurr++ = USB_EP_ATR_BULK;
	*pCurr++ = USB_CDC_DATA_EP_IN_MAX_PACKET_SIZE & 0xff;
	*pCurr++ = USB_CDC_DATA_EP_IN_MAX_PACKET_SIZE >> 8;
	*pCurr++ = USB_EP_BULK_INTERVAL;

	*pCurr++ = USB_BULK_EP_LENGTH;
	*pCurr++ = USB_ENDPOINT_DESCRIPTOR_TYPE;
	*pCurr++ = (USB_EP_OUT_ADDRESS | DataOutEP);
	*pCurr++ = USB_EP_ATR_BULK;
	*pCurr++ = USB_CDC_DATA_EP_OUT_MAX_PACKET_SIZE & 0xff;
	*pCurr++ = USB_CDC_DATA_EP_OUT_MAX_PACKET_SIZE >> 8;
	*pCurr++ = USB_EP_BULK_INTERVAL;

	*pIntfNum = firstIntfId;

	return (pCurr - ptr);
}
//Make DFU Configuration Descriptor
static UINT16 UVAC_MakeDfuConfigDesc(UINT8 *ptr, UINT8 IntfNum)
{
	UINT16 tmpVal = 0;
	UINT8 *pCurr = ptr;
	DBG_IND("ptr=0x%x, 1stIntf=%d\r\n", ptr, IntfNum);

	gUvacIntfIdx_DFU = IntfNum;

	tmpVal = UVAC_MakeStandIntrfDesc(pCurr, IntfNum, DFU_IF_CLASS, DFU_IF_NUMBER_EP);
	pCurr += tmpVal;

	//----- [DFU FUNCTIONAL Descriptor] ----- (length 9)
	*pCurr++ = DFU_FUNC_LENGTH;
	*pCurr++ = DFU_DESCRIPTOR_TYPE;
	*pCurr++ = g_dfu_info.attributes;//DFU_ATTRIBUTE;
	*pCurr++ = g_dfu_info.detach_time_out & 0xff;
	*pCurr++ = g_dfu_info.detach_time_out >> 8;
	if (g_dfu_info.transfer_size > DFU_TRANSFER_SIZE) {
		*pCurr++ = g_dfu_info.transfer_size & 0xff;
		*pCurr++ = g_dfu_info.transfer_size >> 8;
	} else {
		*pCurr++ = DFU_TRANSFER_SIZE & 0xff;
		*pCurr++ = DFU_TRANSFER_SIZE >> 8;
	}
	*pCurr++ = DFU_REV_BCD & 0xff;
	*pCurr++ = DFU_REV_BCD >> 8;

	return (pCurr - ptr);
}
//Make HID Configuration Descriptor
static UINT16 UVAC_MakeHidConfigDesc(UINT8 *ptr, UINT8 IntfNum)
{
	UINT16 tmpVal = 0;
	UINT8 *pCurr = ptr;
	UINT8 ep_num = HID_IF_NUMBER_EP;
	DBG_IND("ptr=0x%x, 1stIntf=%d\r\n", ptr, IntfNum);

	gUvacIntfIdx_HID = IntfNum;

	if (g_hid_info.intr_out) {
		ep_num++;
	}
	tmpVal = UVAC_MakeStandIntrfDesc(pCurr, IntfNum, HID_IF_CLASS, ep_num);
	pCurr += tmpVal;

	//----- [HID Descriptor] -----
	*pCurr++ = g_hid_info.hid_desc.bLength;
	*pCurr++ = g_hid_info.hid_desc.bHidDescType;
	*pCurr++ = g_hid_info.hid_desc.bcdHID & 0xff;
	*pCurr++ = g_hid_info.hid_desc.bcdHID >> 8;
	*pCurr++ = g_hid_info.hid_desc.bCountryCode;
	*pCurr++ = g_hid_info.hid_desc.bNumDescriptors;
	*pCurr++ = g_hid_info.hid_desc.bDescriptorType;
	*pCurr++ = g_hid_info.hid_desc.wDescriptorLength & 0xff;
	*pCurr++ = g_hid_info.hid_desc.wDescriptorLength >> 8;
	if (g_hid_info.hid_desc.bNumDescriptors > 1) {
		UINT8 i;
		UINT8 *p_temp = g_hid_info.hid_desc.p_desc;

		for (i = 0; i < g_hid_info.hid_desc.bNumDescriptors - 1; i++) {
			*pCurr++ = *p_temp++;
			*pCurr++ = *p_temp;
			p_temp++;
			*pCurr++ = *p_temp;
			p_temp++;
		}
	}
	//USB_ENDPOINT_DESCRIPTOR,  interrupt-in
	//---- EP3 Interrupt ----
	*pCurr++ = USB_BULK_EP_LENGTH,                                     /* descriptor size      */
	*pCurr++ = USB_ENDPOINT_DESCRIPTOR_TYPE,                           /* descriptor type      */
	*pCurr++ = (USB_EP_IN_ADDRESS | HID_INTRIN_EP),                                  /* endpoint address     */
	*pCurr++ = USB_EP_ATR_INTERRUPT,                                   /* character address    */
	*pCurr++ = HID_EP_INTR_PACKET_SIZE & 0xff;                        /* max packet size      */
	*pCurr++ = HID_EP_INTR_PACKET_SIZE >> 8;                          /* max packet size      */
	*pCurr++ = g_hid_info.intr_interval;                             /*USB_EP_SIDC_INTERRUPT_INTERVAL*/

	if (g_hid_info.intr_out) {
		*pCurr++ = USB_BULK_EP_LENGTH,                                     /* descriptor size      */
		*pCurr++ = USB_ENDPOINT_DESCRIPTOR_TYPE,                           /* descriptor type      */
		*pCurr++ = (USB_EP_OUT_ADDRESS | HID_INTROUT_EP),                   /* endpoint address     */
		*pCurr++ = USB_EP_ATR_INTERRUPT,                                   /* character address    */
		*pCurr++ = HID_EP_INTR_PACKET_SIZE & 0xff;                        /* max packet size      */
		*pCurr++ = HID_EP_INTR_PACKET_SIZE >> 8;                          /* max packet size      */
		*pCurr++ = g_hid_info.intr_interval;                             /*USB_EP_SIDC_INTERRUPT_INTERVAL*/
	}
	return (pCurr - ptr);
}
#ifdef __LINUX_USER__
static UINT16 UVAC_MakeMsdcConfigDesc(UINT8 *ptr, UINT8 IntfNum)
{
	UINT16 tmpVal = 0;
	UINT8 *pCurr = ptr;
	UINT8 ep_num = 2;

	DBG_IND("ptr=0x%x, 1stIntf=%d\r\n", ptr, IntfNum);

	tmpVal = UVAC_MakeStandIntrfDesc(pCurr, IntfNum, USB_STRG_IF_CLASS_STORAGE, ep_num);
	pCurr += tmpVal;

	memcpy((void*)pCurr, (const void*)gUVCHSStrgConfigDesc, USB_STRG_CFGA_TOTAL_LENGTH);

	pCurr += USB_STRG_CFGA_TOTAL_LENGTH;

	return (pCurr - ptr);
}
#endif
static UINT16 UVAC_MakeFrmDesc(UINT8 *ptr, UINT8 subType, UVAC_VID_RESO_ARY *p_frm_info)
{
	UINT32 frmRate = 0;
	UINT8 *pCurr = ptr;
	UINT32 i = 0, j = 0;
	UINT16 tmpLen = 0, tmpWidth = 0, tmpHeight = 0;
	UINT8  frmIdx = UVC_VSFRM_FIRST_INDEX;
	UINT8  frmDescSize = 0;
	UINT32 frmFpsCnt = 0, tmpFps = 0;
	UVAC_VID_RESO *pVidResAry;

	for (i = 0; i < p_frm_info->aryCnt; i++) {
		pVidResAry = p_frm_info->pVidResAry + i;
		tmpWidth = (UINT16)pVidResAry->width;
		tmpHeight = (UINT16)pVidResAry->height;
		frmFpsCnt = pVidResAry->fpsCnt;
		tmpFps = pVidResAry->fps[0];
		DbgMsg_UVCIO(("Add Type[0x%x]--VidReso[%d]=%d, %d, fps=%d, %d\r\n", subType, i, tmpWidth, tmpHeight, tmpFps, gU2UvcVidResoAry[i].fpsCnt));
		frmDescSize = USB_UVC_VS_FRAME_LENGTH_DISCRETE;
		if (frmFpsCnt > UVAC_VID_RESO_FPS_MAX_CNT) {
			frmFpsCnt = UVAC_VID_RESO_FPS_MAX_CNT;
		} else if (0 == frmFpsCnt) {
			frmFpsCnt = 1;
			DBG_ERR("NULL FPSCnt[%d]\r\n", i);
		}
		frmDescSize += (frmFpsCnt * 4);
		tmpLen += frmDescSize;
		*pCurr++ = frmDescSize;
		*pCurr++ = CS_INTERFACE;
		*pCurr++ = subType;
		*pCurr++ = frmIdx++;
		*pCurr++ = UVC_VSFRM_CAP_FIX_FRMRATE;
		*pCurr++ = tmpWidth;
		*pCurr++ = tmpWidth >> 8;
		*pCurr++ = tmpHeight;
		*pCurr++ = tmpHeight >> 8;
		if (1 == frmFpsCnt) {
			*pCurr++ = gU2UvcStrmMaxBitRate;
			*pCurr++ = gU2UvcStrmMaxBitRate >> 8;
			*pCurr++ = gU2UvcStrmMaxBitRate >> 16;
			*pCurr++ = gU2UvcStrmMaxBitRate >> 24;
		} else {
			*pCurr++ = gU2UvcStrmMinBitRate;
			*pCurr++ = gU2UvcStrmMinBitRate >> 8;
			*pCurr++ = gU2UvcStrmMinBitRate >> 16;
			*pCurr++ = gU2UvcStrmMinBitRate >> 24;
		}
		*pCurr++ = gU2UvcStrmMaxBitRate;
		*pCurr++ = gU2UvcStrmMaxBitRate >> 8;
		*pCurr++ = gU2UvcStrmMaxBitRate >> 16;
		*pCurr++ = gU2UvcStrmMaxBitRate >> 24;
		if (VS_FRAME_MJPEG == subType) {
			UINT32 UvcMaxTBR = tmpWidth*tmpHeight*2;
			*pCurr++ = UvcMaxTBR;
			*pCurr++ = UvcMaxTBR >> 8;
			*pCurr++ = UvcMaxTBR >> 16;
			*pCurr++ = UvcMaxTBR >> 24;
		}
		frmRate = UVAC_GetFrmRateInterval(tmpFps);
		*pCurr++ = (UINT8)frmRate;
		*pCurr++ = (UINT8)(frmRate >> 8);
		*pCurr++ = (UINT8)(frmRate >> 16);
		*pCurr++ = (UINT8)(frmRate >> 24);
		*pCurr++ = frmFpsCnt;
		if (VS_FRAME_FRAME_BASED == subType) {
			*pCurr++ = USB_UVC_VS_FBH264_BYTES_PER_LINE;
			*pCurr++ = USB_UVC_VS_FBH264_BYTES_PER_LINE >> 8;
			*pCurr++ = USB_UVC_VS_FBH264_BYTES_PER_LINE >> 16;
			*pCurr++ = USB_UVC_VS_FBH264_BYTES_PER_LINE >> 24;
		}
		*pCurr++ = (UINT8)frmRate;
		*pCurr++ = (UINT8)(frmRate >> 8);
		*pCurr++ = (UINT8)(frmRate >> 16);
		*pCurr++ = (UINT8)(frmRate >> 24);
		DbgMsg_UVC(("   intrval[0]=0x%x\r\n", frmRate));
		frmFpsCnt--;
		j = 1;
		while (frmFpsCnt) {
			tmpFps = pVidResAry->fps[j];
			if (0 == tmpFps) {
				DBG_ERR("VidReso[%d].fps[%d] NULL, w=%d, h=%d!!\r\n", i, j, tmpWidth, tmpHeight);
				DBG_ERR("VidReso[%d].fps[%d] NULL, w=%d, h=%d!!\r\n", i, j, tmpWidth, tmpHeight);
			}
			frmRate = UVAC_GetFrmRateInterval(tmpFps);
			*pCurr++ = (UINT8)frmRate;
			*pCurr++ = (UINT8)(frmRate >> 8);
			*pCurr++ = (UINT8)(frmRate >> 16);
			*pCurr++ = (UINT8)(frmRate >> 24);
			DbgMsg_UVC(("   intrval[%d]=0x%x\r\n", j, frmRate));
			frmFpsCnt--;
			j++;
		}
	}
	//DBG_IND("LastFrmIdx=%d, ResoCnt=%d, len=%d\r\n", frmIdx, gUvcVidResoCnt, tmpLen);

	return tmpLen;
}
static UINT16 UVAC_MakeYuvFrmDesc(UINT8 *ptr, UVAC_VID_RESO_ARY *p_frm_info)
{
	UINT32 frmRate = 0;
	UINT8 *pCurr = ptr;
	UINT32 i;
	UINT16 tmpLen = 0, tmpWidth = 0, tmpHeight = 0;
	UINT8  frmIdx = UVC_VSFRM_FIRST_INDEX;
	UINT8  frmDescSize = 0;
	UINT32 frmFpsCnt;
	UINT8 subType = VS_FRAME_UNCOMPRESSED;
	UINT32 k, min_bit_rate, max_bit_rate, max_frm_buf_size;
	UVAC_VID_RESO *pVidResAry;
	UINT32 min_fps, max_fps;

	for (i = 0; i < p_frm_info->aryCnt; i++) {
		pVidResAry = p_frm_info->pVidResAry + i;
		tmpWidth = (UINT16)pVidResAry->width;
		tmpHeight = (UINT16)pVidResAry->height;
		frmFpsCnt = pVidResAry->fpsCnt;
		if (frmFpsCnt > UVAC_VID_RESO_FPS_MAX_CNT) {
			frmFpsCnt = UVAC_VID_RESO_FPS_MAX_CNT;
		} else if (0 == frmFpsCnt) {
			frmFpsCnt = 1;
			DBG_ERR("NULL FPSCnt[%d]\r\n", i);
		}
		min_fps = max_fps = pVidResAry->fps[0];
		for (k = 0; k < frmFpsCnt; k++) {
			if (pVidResAry->fps[k]) {
				if (pVidResAry->fps[k] < min_fps) {
					min_fps = pVidResAry->fps[k];
				}
				if (pVidResAry->fps[k] > max_fps) {
					max_fps = pVidResAry->fps[k];
				}
			} else {
				break;
			}
		}
		if (tmpWidth == 0 || tmpHeight == 0 || min_fps == 0 || min_fps == 0) {
			DBG_ERR("Frm_idx[%d] WxH(%d,%d) fps[0]=%d  error\r\n",i, tmpWidth,tmpHeight,pVidResAry->fps[0]);
			break;
		}
		DbgMsg_UVCIO(("Add Type[0x%x]--VidReso[%d]=%d, %d, fpscnt=%d\r\n", subType, i, tmpWidth, tmpHeight, frmFpsCnt));
		frmDescSize = USB_UVC_VS_FRAME_LENGTH_DISCRETE;

		frmDescSize += (frmFpsCnt * 4);
		tmpLen += frmDescSize;
		*pCurr++ = frmDescSize;
		*pCurr++ = CS_INTERFACE;
		*pCurr++ = subType;
		*pCurr++ = frmIdx++;
		*pCurr++ = 0;//bmCapabilities;
		*pCurr++ = tmpWidth;
		*pCurr++ = tmpWidth >> 8;
		*pCurr++ = tmpHeight;
		*pCurr++ = tmpHeight >> 8;

		min_bit_rate = YUV_BIT_RATE(tmpWidth, tmpWidth, min_fps);
		max_bit_rate = YUV_BIT_RATE(tmpWidth, tmpWidth, max_fps);
		max_frm_buf_size = YUV_FRAME_SIZE(tmpWidth, tmpWidth);

		*pCurr++ = min_bit_rate;
		*pCurr++ = min_bit_rate >> 8;
		*pCurr++ = min_bit_rate >> 16;
		*pCurr++ = min_bit_rate >> 24;

		*pCurr++ = max_bit_rate;
		*pCurr++ = max_bit_rate >> 8;
		*pCurr++ = max_bit_rate >> 16;
		*pCurr++ = max_bit_rate >> 24;

		*pCurr++ = max_frm_buf_size;
		*pCurr++ = max_frm_buf_size >> 8;
		*pCurr++ = max_frm_buf_size >> 16;
		*pCurr++ = max_frm_buf_size >> 24;


		frmRate = UVAC_GetFrmRateInterval(pVidResAry->fps[0]);

		*pCurr++ = (UINT8)frmRate;
		*pCurr++ = (UINT8)(frmRate >> 8);
		*pCurr++ = (UINT8)(frmRate >> 16);
		*pCurr++ = (UINT8)(frmRate >> 24);

		*pCurr++ = frmFpsCnt;

		for (k = 0; k < frmFpsCnt; k++) {
			frmRate = UVAC_GetFrmRateInterval(pVidResAry->fps[k]);
			DbgMsg_UVC(("Frm_idx[%d] WxH(%d,%d) Interval=%d (100ns)\r\n",i, tmpWidth,tmpHeight,frmRate));
			*pCurr++ = (UINT8)frmRate;
			*pCurr++ = (UINT8)(frmRate >> 8);
			*pCurr++ = (UINT8)(frmRate >> 16);
			*pCurr++ = (UINT8)(frmRate >> 24);
		}
	}
	DbgMsg_UVCIO(("LastFrmIdx=%d, ResoCnt=%d, len=%d\r\n", frmIdx, p_frm_info->aryCnt, tmpLen));

	return tmpLen;
}
static UINT16 UVAC_MakeVidConfigDesc(UINT8 *ptr, UINT8 *pIntfNum, UINT8 strmIntfCnt, UINT8 epAddr, UINT8 intrEpAddr)
{
	UINT8 *pCurr = ptr;
	UINT8 intfAltSet = 0;
	UINT16 tmpLen = 0;
	UINT8 *pTmp = 0;
	UINT16 tmpVal = 0;
	UINT8 tmpV8 = 0;
	UINT32 i = 0;
	UINT8 format_index = UVC_VSFMT_FIRSTINDEX;
	UINT32 uvc_ver;
	UINT32 pu_len;
	UINT8 pu_ctrolsize;
	UINT32 vidDevIdx = UVAC_VID_DEV_CNT_1;

	if (gUvacUvcVer == UVAC_UVC_VER_100) {
		uvc_ver = USB_UVC_RELEASE_NUMBER;
		pu_len  = USB_UVC_PU_LENGTH;
		pu_ctrolsize = UVC_100_PU_CTRL_SIZE;
		if (pu_controls & 0x00FF0000) {
		//force using 3 byte control size by user
			pu_len++;
			pu_ctrolsize++;
		}
	} else if (gUvacUvcVer == UVAC_UVC_VER_110) {
		uvc_ver = USB_UVC_110_RELEASE_NUMBER;
		pu_len  = USB_UVC_110_PU_LENGTH;
		pu_ctrolsize = UVC_PU_CTRL_SIZE;
	} else {
		uvc_ver = USB_UVC_150_RELEASE_NUMBER;
		pu_len  = USB_UVC_110_PU_LENGTH;
		pu_ctrolsize = UVC_PU_CTRL_SIZE;
	}


	DbgMsg_UVCIO(("+MakeVidConfigDesc:0x%x, intf=%d, alt=%d\r\n", ptr, *pIntfNum, intfAltSet));

	//Interface Association Descriptor of USB_Video_Class
	tmpVal = UVAC_MakeIADDesc(pCurr, *pIntfNum, (strmIntfCnt + 1), CC_VIDEO);
	pCurr += tmpVal;

	//Standard Video Control Interface Descriptor
	gU2UvacIntfIdx_VC[gU2UvacIntfIdx_vidAryCurrIdx] = *pIntfNum;
	DbgMsg_UVC(("VidCtrl[%d] Intf=%d  gUvcVideoFmtType=%d  gUvcYuvFrmInfo.aryCnt=%d\r\n", gU2UvacIntfIdx_vidAryCurrIdx, gU2UvacIntfIdx_VC[gU2UvacIntfIdx_vidAryCurrIdx], gU2UvcVideoFmtType, gUvcYuvFrmInfo.aryCnt));
	tmpVal = UVAC_MakeStandIntrfDesc(pCurr, *pIntfNum, CC_VIDEO, UVAC_IF_VC_EP_CNT);
	pCurr += tmpVal;

	//Class-Specific Video Control Interface Descriptor
	tmpLen = (UVAC_IF_VC_BASE_LEN + UVC_VC_VIDEOSTREAM_TOTAL);
	*pCurr++ = tmpLen;                        //13=12+n
	*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
	*pCurr++ = VC_HEADER;                       //VC_HEADER descriptor subtype
	*pCurr++ = (UINT8)uvc_ver;          //Video Device Class Spec. release number
	*pCurr++ = (UINT8)(uvc_ver >> 8);   //number of endpoints, excludeing EP0
	pTmp = pCurr;                               //keep for updating total length of class-specific video control interface
	*pCurr++ = 0;                               //Total length of class-specific video control interface
	*pCurr++ = 0;
	*pCurr++ = (UINT8)UVC_VC_CLOCK_FREQUENCY;   //dwClockFrequency 30MHz
	*pCurr++ = (UINT8)(UVC_VC_CLOCK_FREQUENCY >> 8);
	*pCurr++ = (UINT8)(UVC_VC_CLOCK_FREQUENCY >> 16);
	*pCurr++ = (UINT8)(UVC_VC_CLOCK_FREQUENCY >> 24);
	*pCurr++ = UVC_VC_VIDEOSTREAM_TOTAL;        //How many video stream interface in this IAD
	*pCurr++ = (*pIntfNum + 1);                 //Interface number/idx of the first video stream

	//Camera/Input Terminal Descriptor
	tmpLen += USB_UVC_IT_LENGTH;
	*pCurr++ = USB_UVC_IT_LENGTH;               //17=15+n
	*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
	*pCurr++ = VC_INPUT_TERMINAL;               //VC_INPUT_TERMINAL descriptor subtype
	*pCurr++ = UVC_TERMINAL_ID_ITT;            //terminal ID, non-zero, unique
	*pCurr++ = (UINT8)ITT_CAMERA;                      //Terminal Type
	*pCurr++ = (UINT8)(ITT_CAMERA >> 8);
	*pCurr++ = UVC_TERMINAL_ID_NULL;           //ID of associated output terminal
	*pCurr++ = UVC_IT_ITERMINAL_STR_INDEX;      //index of a string descriptor described this camera terminal
	*pCurr++ = UVC_IT_OBJFOCAL_LEN_MIN;         //wObjectiveFocalLengthMin
	*pCurr++ = UVC_IT_OBJFOCAL_LEN_MIN >> 8;    //wObjectiveFocalLengthMin
	*pCurr++ = UVC_IT_OBJFOCAL_LEN_MAX;         //wObjectiveFocalLengthMax
	*pCurr++ = UVC_IT_OBJFOCAL_LEN_MAX >> 8;    //wObjectiveFocalLengthMax
	*pCurr++ = UVC_IT_OBJFOCAL_LEN;             //wObjectiveFocalLength
	*pCurr++ = UVC_IT_OBJFOCAL_LEN >> 8;        //wObjectiveFocalLength
	*pCurr++ = UVC_IT_INPUT_CTRL_SIZE;
	*pCurr++ = (UINT8)ct_controls;
	*pCurr++ = (UINT8)(ct_controls >> 8);
	*pCurr++ = (UINT8)(ct_controls >> 16);

	//Output Terminal Descriptor
	tmpLen += USB_UVC_OT_LENGTH;
	*pCurr++ = USB_UVC_OT_LENGTH;               //9
	*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
	*pCurr++ = VC_OUTPUT_TERMINAL;              //VC_OUTPUT_TERMINAL descriptor subtype
	*pCurr++ = UVC_TERMINAL_ID_OT;             //terminal ID, non-zero, unique
	*pCurr++ = (UINT8)TT_STREAMING;                    //Terminal Type ~ bTerminalLink of Class-spec VS Header input Descriptor
	*pCurr++ = (UINT8)(TT_STREAMING >> 8);
	*pCurr++ = UVC_TERMINAL_ID_NULL;           //ID of input terminal that the output terminal is associated
	*pCurr++ = UVC_TERMINAL_ID_PROC_UNIT;      //bSourceID, what the terminal is connected to
	*pCurr++ = UVC_OT_ITERMINAL_STR_INDEX;      //index of a string descriptor described this output terminal

	//Select Unit Descriptor
	tmpLen += USB_UVC_SU_LENGTH;
	*pCurr++ = USB_UVC_SU_LENGTH;               //7
	*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
	*pCurr++ = VC_SELECTOR_UNIT;                //VC_SELECTOR_UNIT descriptor subtype
	*pCurr++ = UVC_TERMINAL_ID_SEL_UNIT;       //terminal ID, non-zero, unique
	*pCurr++ = UVC_SU_PIN_NUMBERS;              //Number of input pin
	*pCurr++ = UVC_TERMINAL_ID_ITT;            //bSourceID, What terminal the first input pin of the ternimal is connected.
	*pCurr++ = UVC_SU_SELECT_STR_INDEX;         //index of a string descriptor described this selector unit

	//Processing unit Descriptor
	tmpLen += pu_len;
	*pCurr++ = pu_len;               //11
	*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
	*pCurr++ = VC_PROCESSING_UNIT;              //VC_PROCESSING_UNIT descriptor subtype
	*pCurr++ = UVC_TERMINAL_ID_PROC_UNIT;      //terminal ID, non-zero, unique
	*pCurr++ = UVC_TERMINAL_ID_SEL_UNIT;       //bSourceID, What terminal the terminal is connected to
	*pCurr++ = UVC_PU_MAX_MULTIPLIER;           //maximum digital magnification
	*pCurr++ = UVC_PU_MAX_MULTIPLIER >> 8;
	*pCurr++ = pu_ctrolsize;                    //size of bmControls
	*pCurr++ = (UINT8)pu_controls;
	*pCurr++ = (UINT8)(pu_controls >> 8);
	if (pu_ctrolsize > UVC_100_PU_CTRL_SIZE) {
		*pCurr++ = (UINT8)(pu_controls >> 16);
	}
	*pCurr++ = UVC_PU_PROCESS_STR_INDEX;         //index of a string descriptor described this processing unit
	if (gUvacUvcVer >= UVAC_UVC_VER_110) {
		*pCurr++ = UVC_PU_VID_STANDARDS;
	}

	if (eu_desc[0].bLength) {
		tmpLen += eu_desc[0].bLength;
		*pCurr++ = eu_desc[0].bLength;
		*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
		*pCurr++ = VC_EXTENSION_UNIT;               //VC_EXTENSION_UNIT descriptor subtype
		eu_unit_id[0] = eu_desc[0].bUnitID;
		*pCurr++ = eu_unit_id[0];
		for (i = 0; i < 16; i++) {
			*pCurr++ = eu_desc[0].guidExtensionCode[i];
		}
		*pCurr++ = eu_desc[0].bNumControls;            //bytes size of the bmControls field
		*pCurr++ = eu_desc[0].bNrInPins;               //number of input pins
		if (eu_desc[0].baSourceID == NULL || eu_desc[0].baSourceID[0] == 0) { //set to default
			*pCurr++ = UVC_TERMINAL_ID_PROC_UNIT; //bSourceID, What terminal the terminal is connected to
		} else {
			for (i = 0; i < eu_desc[0].bNrInPins; i++) {
				*pCurr++ = eu_desc[0].baSourceID[i];       //bSourceID, What terminal the terminal is connected to
			}
		}
		*pCurr++ = eu_desc[0].bControlSize;
		for (i = 0; i < eu_desc[0].bControlSize; i++) {
			*pCurr++ = eu_desc[0].bmControls[i];
		}
		*pCurr++ = eu_desc[0].iExtension;
	} else {
		//Extension unit Descriptor
		tmpLen += USB_UVC_XU_LENGTH;
		*pCurr++ = USB_UVC_XU_LENGTH;               //27
		*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
		*pCurr++ = VC_EXTENSION_UNIT;               //VC_EXTENSION_UNIT descriptor subtype
		//*pCurr++ = UVC_TERMINAL_ID_EXTE_UNIT;      //terminal ID, non-zero, unique
		*pCurr++ = eu_unit_id[0];
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID1);               //vendor-specific guid code to indentify the extension unit
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID1 >> 8);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID1 >> 16);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID1 >> 24);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID2);                //vendor-specific guid code to indentify the extension unit
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID2 >> 8);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID2 >> 16);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID2 >> 24);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID3);                //vendor-specific guid code to indentify the extension unit
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID3 >> 8);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID3 >> 16);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID3 >> 24);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID4);                //vendor-specific guid code to indentify the extension unit
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID4 >> 8);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID4 >> 16);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID4 >> 24);
		*pCurr++ = UVC_XU_CONTROL_NUMS;             //bytes size of the bmControls field
		*pCurr++ = UVC_XU_INPUT_PINS;               //number of iinput pins
		*pCurr++ = UVC_TERMINAL_ID_PROC_UNIT;      //bSourceID, What terminal the terminal is connected to
		*pCurr++ = UVC_XU_CONTROLSIZE;
		*pCurr++ = UVC_XU_BMCONTROLS & 0xff;
		*pCurr++ = (UVC_XU_BMCONTROLS >> 8) & 0xff;
		*pCurr++ = UVC_XU_STR_INDEX;
	}

	if (eu_desc[1].bLength) {
		tmpLen += eu_desc[1].bLength;
		*pCurr++ = eu_desc[1].bLength;
		*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
		*pCurr++ = VC_EXTENSION_UNIT;               //VC_EXTENSION_UNIT descriptor subtype
		eu_unit_id[1] = eu_desc[1].bUnitID;
		*pCurr++ = eu_unit_id[1];
		for (i = 0; i < 16; i++) {
			*pCurr++ = eu_desc[1].guidExtensionCode[i];
		}
		*pCurr++ = eu_desc[1].bNumControls;            //bytes size of the bmControls field
		*pCurr++ = eu_desc[1].bNrInPins;               //number of input pins
		if (eu_desc[1].baSourceID == NULL || eu_desc[1].baSourceID[0] == 0) { //set to default
			*pCurr++ = UVC_TERMINAL_ID_PROC_UNIT; //bSourceID, What terminal the terminal is connected to
		} else {
			for (i = 0; i < eu_desc[1].bNrInPins; i++) {
				*pCurr++ = eu_desc[1].baSourceID[i];       //bSourceID, What terminal the terminal is connected to
			}
		}
		*pCurr++ = eu_desc[1].bControlSize;
		for (i = 0; i < eu_desc[1].bControlSize; i++) {
			*pCurr++ = eu_desc[1].bmControls[i];
		}
		*pCurr++ = eu_desc[1].iExtension;
	}

	DbgMsg_UVC(("Class-spec. VC interface Descriptor len=0x%x, pTmp=0x%x\r\n", tmpLen, pTmp));
	*pTmp++ = tmpLen;
	*pTmp = tmpLen >> 8;

	//Standard interrupt EP Descriptor
	DbgMsg_UVC(("Add interrupt EP=0x%x\r\n", intrEpAddr));
	tmpVal = UVAC_MakeEPDesc(pCurr, intrEpAddr, USB_EP_ATR_INTERRUPT, UVAC_INTR_EP_MAX_PACKET_SIZE, USB_EP_INT_INTERVAL);
	pCurr += tmpVal;

	//Class -spec interrupt EP Descriptor
	*pCurr++ = USB_UVC_EP_LENGTH;
	*pCurr++ = CS_ENDPOINT;
	*pCurr++ = EP_INTERRUPT;
	*pCurr++ = UVAC_INTR_EP_MAX_PACKET_SIZE;
	*pCurr++ = UVAC_INTR_EP_MAX_PACKET_SIZE >> 8;

	*pIntfNum = *pIntfNum + 1;                                  //next interface number/idx
	//Standard VS interface Descriptor, ALT0, NumOfEP=0
	gU2UvacIntfIdx_VS[gU2UvacIntfIdx_vidAryCurrIdx] = *pIntfNum;
	DBG_IND("VidStrm[%d] Intf=%d\r\n", gU2UvacIntfIdx_vidAryCurrIdx, gU2UvacIntfIdx_VS[gU2UvacIntfIdx_vidAryCurrIdx]);
	gU2UvacIntfIdx_vidAryCurrIdx++;
	intfAltSet = 0;
	tmpV8 = gU2UvcCapM3Enable ? UVAC_IF_VS_EP_CNT : 0;
	tmpVal = UVAC_MakeStrmIntfDesc(pCurr, *pIntfNum, intfAltSet, tmpV8, CC_VIDEO);
	pCurr += tmpVal;
	intfAltSet++;  //1

	//Class-Specific VS Header input Descriptor
	//V1-EP1
	tmpLen = USB_UVC_VS_INPUT_HEADER_LENGTH;

	//H264 and MJPEG -> compression quality
	//YUV -> uncompression quality

	if (gUvcYuvFrmInfo.aryCnt) {
		tmpLen++;
	}
	*pCurr++ = tmpLen;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = VS_INPUT_HEADER;

	if (gUvcYuvFrmInfo.aryCnt) {
		*pCurr++ = 2;    //video format count, H264 and MJPEG
	} else {
		*pCurr++ = 1;    //video format count, only H264 or only MJPEG
	}
	pTmp = pCurr;          //keep for updating total length of the Class-Specific VS Header input Descriptor
	*pCurr++ = 0;          //total length of the Class-Specific VS Header input Descriptor
	*pCurr++ = 0;
	*pCurr++ = epAddr;
	*pCurr++ = UVC_VS_BMINFO;
	*pCurr++ = UVC_TERMINAL_ID_OT;
	if (gU2UvcCapM3Enable) {
		*pCurr++ = UVC_VS_STILL_CAPTURE_M3;
		*pCurr++ = UVC_VS_HW_TRIGGER_ENABLE;
		*pCurr++ = UVC_VS_HW_TRIGGER_USAGE_GENARAL;
	} else {
		*pCurr++ = UVC_VS_STILL_CAPTURE_NONE;
		*pCurr++ = UVC_VS_HW_TRIGGER_DISABLE;
		*pCurr++ = UVC_VS_HW_TRIGGER_USAGE_INIT;
	}
	*pCurr++ = UVC_VS_CTRL_SIZE;
	*pCurr++ = UVC_VS_CONTROLS;
	if (gUvcYuvFrmInfo.aryCnt) {
		*pCurr++ = 0; //bmControls
	}

	if (UVAC_VIDEO_FORMAT_H264_ONLY != gU2UvcVideoFmtType) {
		//Class-Specific VS Format Descriptor (MJPG)
		tmpLen += USB_UVC_VS_MJPG_FORMAT_LENGTH;
		*pCurr++ = USB_UVC_VS_MJPG_FORMAT_LENGTH;
		*pCurr++ = CS_INTERFACE;
		*pCurr++ = VS_FORMAT_MJPEG;
		*pCurr++ = format_index;   //index of this format descriptor
		mjpeg_fmt_index[vidDevIdx] = format_index;
		format_index++;
		*pCurr++ = gUvcMjpgFrmInfo[vidDevIdx].aryCnt;              //number of frame descriptor => how many resolutions
		DbgMsg_UVC(("Reso-Cnt=%d of VS[%d]-MJPG\r\n", gUvcMjpgFrmInfo[vidDevIdx].aryCnt, *pIntfNum));
		*pCurr++ = UVC_VSFMT_BMINFO;
		if (gUvcMjpgFrmInfo[vidDevIdx].bDefaultFrameIndex > gUvcMjpgFrmInfo[vidDevIdx].aryCnt|| gUvcMjpgFrmInfo[vidDevIdx].bDefaultFrameIndex == 0) {
			*pCurr++ = UVC_VSFMT_DEF_FRAMEINDEX;
			DBG_ERR("mjpg[%d] bDefaultFrameIndex(%d), aryCnt(%d)!\r\n", vidDevIdx, gUvcMjpgFrmInfo[vidDevIdx].bDefaultFrameIndex, gUvcMjpgFrmInfo[vidDevIdx].aryCnt);
		} else {
			*pCurr++ = (UINT8)gUvcMjpgFrmInfo[vidDevIdx].bDefaultFrameIndex;
		}
		*pCurr++ = UVC_VSFMT_ASPECTRATIO_X;
		*pCurr++ = UVC_VSFMT_ASPECTRATIO_Y;
		*pCurr++ = UVC_VSFMT_INTERLACE;
		*pCurr++ = UVC_VSFMT_COPYPROTECT;

		//Class-Specific VS Frame Descriptor(MJPG)
		tmpVal = UVAC_MakeFrmDesc(pCurr, VS_FRAME_MJPEG, &gUvcMjpgFrmInfo[vidDevIdx]);
		pCurr += tmpVal;
		tmpLen += tmpVal;

		//Color matching Descriptor
		tmpVal = UVAC_MakeColorMatch(pCurr);
		pCurr += tmpVal;
		tmpLen += tmpVal;
	}

	if (UVAC_VIDEO_FORMAT_MJPEG_ONLY != gU2UvcVideoFmtType) {
		//Class-Specific VS Format Descriptor(H264)
		tmpLen += USB_UVC_VS_FBH264_FORMAT_LENGTH;
		*pCurr++ = USB_UVC_VS_FBH264_FORMAT_LENGTH;
		*pCurr++ = CS_INTERFACE;
		*pCurr++ = VS_FORMAT_FRAME_BASED;
		*pCurr++ = format_index;   //index of this format descriptor
		h264_fmt_index[vidDevIdx] = format_index;
		format_index++;
		*pCurr++ = gUvcH264FrmInfo[vidDevIdx].aryCnt;              //number of frame descriptor => how many resolutions
		DbgMsg_UVC(("Reso-Cnt=%d of VS[%d]-H264\r\n", gUvcH264FrmInfo[vidDevIdx].aryCnt, *pIntfNum));
		for (i = 0; i < UVAC_H264_GUIDFMT_LEN; i++) {
			*pCurr++ = gU2UvacH264GuidFormat[i];
		}
		*pCurr++ = UVC_BITS_PER_PIXEL;
		if (gUvcH264FrmInfo[vidDevIdx].bDefaultFrameIndex > gUvcH264FrmInfo[vidDevIdx].aryCnt || gUvcH264FrmInfo[vidDevIdx].bDefaultFrameIndex == 0) {
			*pCurr++ = UVC_VSFMT_DEF_FRAMEINDEX;
			DBG_ERR("H264[%d] def_frm_idx(%d), aryCnt(%d)!\r\n", vidDevIdx, gUvcH264FrmInfo[vidDevIdx].bDefaultFrameIndex, gUvcH264FrmInfo[vidDevIdx].aryCnt);
		} else {
			*pCurr++ = (UINT8)gUvcH264FrmInfo[vidDevIdx].bDefaultFrameIndex;
		}

		*pCurr++ = UVC_VSFMT_ASPECTRATIO_X;
		*pCurr++ = UVC_VSFMT_ASPECTRATIO_Y;
		*pCurr++ = UVC_VSFMT_INTERLACE;
		*pCurr++ = UVC_VSFMT_COPYPROTECT;
		*pCurr++ = UVC_VS_FRM_VARIABLE_SIZE;

		//Class-Specific VS Frame Descriptor(H264)
		tmpVal = UVAC_MakeFrmDesc(pCurr, VS_FRAME_FRAME_BASED, &gUvcH264FrmInfo[vidDevIdx]);
		pCurr += tmpVal;
		tmpLen += tmpVal;

		//Color matching Descriptor
		tmpVal = UVAC_MakeColorMatch(pCurr);
		pCurr += tmpVal;
		tmpLen += tmpVal;
	}


	if (gUvcYuvFrmInfo.aryCnt) {
		DbgMsg_UVC(("YuvFrmInfo num=%d\r\n", gUvcYuvFrmInfo.aryCnt));

		//Class-Specific VS Format Descriptor(YUV)
		tmpLen += USB_UVC_VS_YUV_FORMAT_LENGTH;
		*pCurr++ = USB_UVC_VS_YUV_FORMAT_LENGTH;
		*pCurr++ = CS_INTERFACE;
		*pCurr++ = VS_FORMAT_UNCOMPRESSED;
		*pCurr++ = format_index;   //index of this format descriptor
		yuv_fmt_index = format_index;
		format_index++;
		*pCurr++ = gUvcYuvFrmInfo.aryCnt;      //number of frame descriptor => how many resolutions
		for (i = 0; i < UVAC_YUV_GUIDFMT_LEN; i++) {
			*pCurr++ = gUvacYUVGuidFormat[i];
		}
		*pCurr++ = 0x10;      //bBitsPerPixel
		*pCurr++ = gUvcYuvFrmInfo.bDefaultFrameIndex;
		*pCurr++ = UVC_VSFMT_ASPECTRATIO_X;
		*pCurr++ = UVC_VSFMT_ASPECTRATIO_Y;
		*pCurr++ = UVC_VSFMT_INTERLACE;
		*pCurr++ = 0; //bCopyProtect

		//Class-Specific VS Frame Descriptor(H264)
		tmpVal = UVAC_MakeYuvFrmDesc(pCurr, &gUvcYuvFrmInfo);
		pCurr += tmpVal;
		tmpLen += tmpVal;

		//Color matching Descriptor
		tmpVal = UVAC_MakeColorMatch(pCurr);
		pCurr += tmpVal;
		tmpLen += tmpVal;

	}
	//Updating total length of the Class-Specific VS Header input Descriptor
	*pTmp++ = tmpLen; //More add:USB_UVC_VS_COLOR_LENGTH+USB_BULK_EP_LENGTH
	*pTmp = tmpLen >> 8;

	if ((gU2UvcMJPGMaxTBR > UVAC_MJPG_TBR_MAX) || (gU2UvacChannel > UVAC_CHANNEL_1V1A)) {
		UINT16 packetSize;
		//Standard VS interface Descriptor, ALT(1+i), NumOfEP=1
		tmpVal = UVAC_MakeStrmIntfDesc(pCurr, *pIntfNum, intfAltSet, UVAC_IF_VS_EP_CNT, CC_VIDEO);
		pCurr += tmpVal;
		//high bandwidth, [12:11] 01 for extra 1 packet and 10 for extra 2 packet.
		packetSize = gU2UvcIsoInHsPacketSize | (gU2UvcIsoInBandWidth - 1) << 11;
		//Standard Iso EP Descriptor
		tmpVal = UVAC_MakeEPDesc(pCurr, epAddr, (USB_EP_ATR_ISO | USB_EP_ATR_ASYNC), packetSize, USB_EP_ISO_INTERVAL);

		pCurr += tmpVal;
	} else {
		for (i = 0; i < UVAC_VID_EP_PACK_SIZE_MAX_CNT; i++) {
			//Standard VS interface Descriptor, ALT(1+i), NumOfEP=1
			tmpVal = UVAC_MakeStrmIntfDesc(pCurr, *pIntfNum, intfAltSet, UVAC_IF_VS_EP_CNT, CC_VIDEO);
			pCurr += tmpVal;
			intfAltSet++;
			//Standard Iso EP Descriptor
			tmpVal = UVAC_MakeEPDesc(pCurr, epAddr, (USB_EP_ATR_ISO | USB_EP_ATR_ASYNC), gU2UvacVidEPMaxPackSize[i], USB_EP_ISO_INTERVAL);

			pCurr += tmpVal;
		}
	}
	DbgMsg_UVCIO(("-MakeVidConfigDesc:sAddr=0x%x, endAddr=0x%x, intf=%d, alt=%d\r\n", ptr, pCurr, *pIntfNum, intfAltSet));
	return (pCurr - ptr);
}
static UINT16 UVAC_MakeVidConfigDesc2(UINT8 *ptr, UINT8 *pIntfNum, UINT8 strmIntfCnt, UINT8 epAddr, UINT8 intrEpAddr)
{
	UINT8 *pCurr = ptr;
	UINT8 intfAltSet = 0;
	UINT16 tmpLen = 0;
	UINT8 *pTmp = 0;
	UINT16 tmpVal = 0;
	UINT8 tmpV8 = 0;
	UINT32 i = 0;
	UINT8 format_index = UVC_VSFMT_FIRSTINDEX;
	UINT32 uvc_ver;
	UINT32 pu_len;
	UINT8 pu_ctrolsize;
	UINT32 vidDevIdx = UVAC_VID_DEV_CNT_2;

	if (gUvacUvcVer == UVAC_UVC_VER_100) {
		uvc_ver = USB_UVC_RELEASE_NUMBER;
		pu_len  = USB_UVC_PU_LENGTH;
		pu_ctrolsize = UVC_100_PU_CTRL_SIZE;
		if (pu_controls & 0x00FF0000) {
		//force using 3 byte control size by user
			pu_len++;
			pu_ctrolsize++;
		}
	} else if (gUvacUvcVer == UVAC_UVC_VER_110) {
		uvc_ver = USB_UVC_110_RELEASE_NUMBER;
		pu_len  = USB_UVC_110_PU_LENGTH;
		pu_ctrolsize = UVC_PU_CTRL_SIZE;
	} else {
		uvc_ver = USB_UVC_150_RELEASE_NUMBER;
		pu_len  = USB_UVC_110_PU_LENGTH;
		pu_ctrolsize = UVC_PU_CTRL_SIZE;
	}


	DbgMsg_UVCIO(("+MakeVidConfigDesc2:0x%x, intf=%d, alt=%d\r\n", ptr, *pIntfNum, intfAltSet));

	//Interface Association Descriptor of USB_Video_Class
	tmpVal = UVAC_MakeIADDesc(pCurr, *pIntfNum, (strmIntfCnt + 1), CC_VIDEO);
	pCurr += tmpVal;

	//Standard Video Control Interface Descriptor
	gU2UvacIntfIdx_VC[gU2UvacIntfIdx_vidAryCurrIdx] = *pIntfNum;
	DbgMsg_UVC(("VidCtrl[%d] Intf=%d\r\n", gU2UvacIntfIdx_vidAryCurrIdx, gU2UvacIntfIdx_VC[gU2UvacIntfIdx_vidAryCurrIdx]));
	tmpVal = UVAC_MakeStandIntrfDesc(pCurr, *pIntfNum, CC_VIDEO, UVAC_IF_VC_EP_CNT);
	pCurr += tmpVal;

	//Class-Specific Video Control Interface Descriptor
	tmpLen = (UVAC_IF_VC_BASE_LEN + UVC_VC_VIDEOSTREAM_TOTAL);
	*pCurr++ = tmpLen;                        //13=12+n
	*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
	*pCurr++ = VC_HEADER;                       //VC_HEADER descriptor subtype
	*pCurr++ = (UINT8)uvc_ver;          //Video Device Class Spec. release number
	*pCurr++ = (UINT8)(uvc_ver >> 8);   //number of endpoints, excludeing EP0
	pTmp = pCurr;                               //keep for updating total length of class-specific video control interface
	*pCurr++ = 0;                               //Total length of class-specific video control interface
	*pCurr++ = 0;
	*pCurr++ = (UINT8)UVC_VC_CLOCK_FREQUENCY;   //dwClockFrequency 30MHz
	*pCurr++ = (UINT8)(UVC_VC_CLOCK_FREQUENCY >> 8);
	*pCurr++ = (UINT8)(UVC_VC_CLOCK_FREQUENCY >> 16);
	*pCurr++ = (UINT8)(UVC_VC_CLOCK_FREQUENCY >> 24);
	*pCurr++ = UVC_VC_VIDEOSTREAM_TOTAL;        //How many video stream interface in this IAD
	*pCurr++ = (*pIntfNum + 1);                 //Interface number/idx of the first video stream

	//Camera/Input Terminal Descriptor
	tmpLen += USB_UVC_IT_LENGTH;
	*pCurr++ = USB_UVC_IT_LENGTH;               //17=15+n
	*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
	*pCurr++ = VC_INPUT_TERMINAL;               //VC_INPUT_TERMINAL descriptor subtype
	*pCurr++ = UVC_TERMINAL_ID_ITT;            //terminal ID, non-zero, unique
	*pCurr++ = (UINT8)ITT_CAMERA;                      //Terminal Type
	*pCurr++ = (UINT8)(ITT_CAMERA >> 8);
	*pCurr++ = UVC_TERMINAL_ID_NULL;           //ID of associated output terminal
	*pCurr++ = UVC_IT_ITERMINAL_STR_INDEX;      //index of a string descriptor described this camera terminal
	*pCurr++ = UVC_IT_OBJFOCAL_LEN_MIN;         //wObjectiveFocalLengthMin
	*pCurr++ = UVC_IT_OBJFOCAL_LEN_MIN >> 8;    //wObjectiveFocalLengthMin
	*pCurr++ = UVC_IT_OBJFOCAL_LEN_MAX;         //wObjectiveFocalLengthMax
	*pCurr++ = UVC_IT_OBJFOCAL_LEN_MAX >> 8;    //wObjectiveFocalLengthMax
	*pCurr++ = UVC_IT_OBJFOCAL_LEN;             //wObjectiveFocalLength
	*pCurr++ = UVC_IT_OBJFOCAL_LEN >> 8;        //wObjectiveFocalLength
	*pCurr++ = UVC_IT_INPUT_CTRL_SIZE;
	*pCurr++ = (UINT8)ct_controls;
	*pCurr++ = (UINT8)(ct_controls >> 8);
	*pCurr++ = (UINT8)(ct_controls >> 16);

	//Output Terminal Descriptor
	tmpLen += USB_UVC_OT_LENGTH;
	*pCurr++ = USB_UVC_OT_LENGTH;               //9
	*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
	*pCurr++ = VC_OUTPUT_TERMINAL;              //VC_OUTPUT_TERMINAL descriptor subtype
	*pCurr++ = UVC_TERMINAL_ID_OT;             //terminal ID, non-zero, unique
	*pCurr++ = (UINT8)TT_STREAMING;                    //Terminal Type ~ bTerminalLink of Class-spec VS Header input Descriptor
	*pCurr++ = (UINT8)(TT_STREAMING >> 8);
	*pCurr++ = UVC_TERMINAL_ID_NULL;           //ID of input terminal that the output terminal is associated
	*pCurr++ = UVC_TERMINAL_ID_PROC_UNIT;      //bSourceID, what the terminal is connected to
	*pCurr++ = UVC_OT_ITERMINAL_STR_INDEX;      //index of a string descriptor described this output terminal

	//Select Unit Descriptor
	tmpLen += USB_UVC_SU_LENGTH;
	*pCurr++ = USB_UVC_SU_LENGTH;               //7
	*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
	*pCurr++ = VC_SELECTOR_UNIT;                //VC_SELECTOR_UNIT descriptor subtype
	*pCurr++ = UVC_TERMINAL_ID_SEL_UNIT;       //terminal ID, non-zero, unique
	*pCurr++ = UVC_SU_PIN_NUMBERS;              //Number of input pin
	*pCurr++ = UVC_TERMINAL_ID_ITT;            //bSourceID, What terminal the first input pin of the ternimal is connected.
	*pCurr++ = UVC_SU_SELECT_STR_INDEX;         //index of a string descriptor described this selector unit

	//Processing unit Descriptor
	tmpLen += pu_len;
	*pCurr++ = pu_len;               //11
	*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
	*pCurr++ = VC_PROCESSING_UNIT;              //VC_PROCESSING_UNIT descriptor subtype
	*pCurr++ = UVC_TERMINAL_ID_PROC_UNIT;      //terminal ID, non-zero, unique
	*pCurr++ = UVC_TERMINAL_ID_SEL_UNIT;       //bSourceID, What terminal the terminal is connected to
	*pCurr++ = UVC_PU_MAX_MULTIPLIER;           //maximum digital magnification
	*pCurr++ = UVC_PU_MAX_MULTIPLIER >> 8;
	*pCurr++ = pu_ctrolsize;                    //size of bmControls
	*pCurr++ = (UINT8)pu_controls;
	*pCurr++ = (UINT8)(pu_controls >> 8);
	if (pu_ctrolsize > UVC_100_PU_CTRL_SIZE) {
		*pCurr++ = (UINT8)(pu_controls >> 16);
	}
	*pCurr++ = UVC_PU_PROCESS_STR_INDEX;         //index of a string descriptor described this processing unit
	if (gUvacUvcVer >= UVAC_UVC_VER_110) {
		*pCurr++ = UVC_PU_VID_STANDARDS;
	}

	if (eu_desc[0].bLength) {
		tmpLen += eu_desc[0].bLength;
		*pCurr++ = eu_desc[0].bLength;
		*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
		*pCurr++ = VC_EXTENSION_UNIT;               //VC_EXTENSION_UNIT descriptor subtype
		eu_unit_id[0] = eu_desc[0].bUnitID;
		*pCurr++ = eu_unit_id[0];
		for (i = 0; i < 16; i++) {
			*pCurr++ = eu_desc[0].guidExtensionCode[i];
		}
		*pCurr++ = eu_desc[0].bNumControls;            //bytes size of the bmControls field
		*pCurr++ = eu_desc[0].bNrInPins;               //number of input pins
		if (eu_desc[0].baSourceID == NULL || eu_desc[0].baSourceID[0] == 0) { //set to default
			*pCurr++ = UVC_TERMINAL_ID_PROC_UNIT; //bSourceID, What terminal the terminal is connected to
		} else {
			for (i = 0; i < eu_desc[0].bNrInPins; i++) {
				*pCurr++ = eu_desc[0].baSourceID[i];       //bSourceID, What terminal the terminal is connected to
			}
		}
		*pCurr++ = eu_desc[0].bControlSize;
		for (i = 0; i < eu_desc[0].bControlSize; i++) {
			*pCurr++ = eu_desc[0].bmControls[i];
		}
		*pCurr++ = eu_desc[0].iExtension;
	} else {
		//Extension unit Descriptor
		tmpLen += USB_UVC_XU_LENGTH;
		*pCurr++ = USB_UVC_XU_LENGTH;               //27
		*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
		*pCurr++ = VC_EXTENSION_UNIT;               //VC_EXTENSION_UNIT descriptor subtype
		//*pCurr++ = UVC_TERMINAL_ID_EXTE_UNIT;      //terminal ID, non-zero, unique
		*pCurr++ = eu_unit_id[0];
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID1);               //vendor-specific guid code to indentify the extension unit
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID1 >> 8);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID1 >> 16);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID1 >> 24);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID2);                //vendor-specific guid code to indentify the extension unit
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID2 >> 8);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID2 >> 16);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID2 >> 24);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID3);                //vendor-specific guid code to indentify the extension unit
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID3 >> 8);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID3 >> 16);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID3 >> 24);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID4);                //vendor-specific guid code to indentify the extension unit
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID4 >> 8);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID4 >> 16);
		*pCurr++ = (UINT8)(USB_UVC_XU_GUID4 >> 24);
		*pCurr++ = UVC_XU_CONTROL_NUMS;             //bytes size of the bmControls field
		*pCurr++ = UVC_XU_INPUT_PINS;               //number of iinput pins
		*pCurr++ = UVC_TERMINAL_ID_PROC_UNIT;      //bSourceID, What terminal the terminal is connected to
		*pCurr++ = UVC_XU_CONTROLSIZE;
		*pCurr++ = UVC_XU_BMCONTROLS & 0xff;
		*pCurr++ = (UVC_XU_BMCONTROLS >> 8) & 0xff;
		*pCurr++ = UVC_XU_STR_INDEX;
	}

	if (eu_desc[1].bLength) {
		tmpLen += eu_desc[1].bLength;
		*pCurr++ = eu_desc[1].bLength;
		*pCurr++ = CS_INTERFACE;                    //CS_INTERFACE descriptor type
		*pCurr++ = VC_EXTENSION_UNIT;               //VC_EXTENSION_UNIT descriptor subtype
		eu_unit_id[1] = eu_desc[1].bUnitID;
		*pCurr++ = eu_unit_id[1];
		for (i = 0; i < 16; i++) {
			*pCurr++ = eu_desc[1].guidExtensionCode[i];
		}
		*pCurr++ = eu_desc[1].bNumControls;            //bytes size of the bmControls field
		*pCurr++ = eu_desc[1].bNrInPins;               //number of input pins
		if (eu_desc[1].baSourceID == NULL || eu_desc[1].baSourceID[0] == 0) { //set to default
			*pCurr++ = UVC_TERMINAL_ID_PROC_UNIT; //bSourceID, What terminal the terminal is connected to
		} else {
			for (i = 0; i < eu_desc[1].bNrInPins; i++) {
				*pCurr++ = eu_desc[1].baSourceID[i];       //bSourceID, What terminal the terminal is connected to
			}
		}
		*pCurr++ = eu_desc[1].bControlSize;
		for (i = 0; i < eu_desc[1].bControlSize; i++) {
			*pCurr++ = eu_desc[1].bmControls[i];
		}
		*pCurr++ = eu_desc[1].iExtension;
	}

	DbgMsg_UVC(("Class-spec. VC interface Descriptor len=0x%x, pTmp=0x%x\r\n", tmpLen, pTmp));
	*pTmp++ = tmpLen;
	*pTmp = tmpLen >> 8;

	//Standard interrupt EP Descriptor
	DbgMsg_UVC(("Add interrupt EP=0x%x\r\n", intrEpAddr));
	tmpVal = UVAC_MakeEPDesc(pCurr, intrEpAddr, USB_EP_ATR_INTERRUPT, UVAC_INTR_EP_MAX_PACKET_SIZE, USB_EP_INT_INTERVAL);
	pCurr += tmpVal;

	//Class -spec interrupt EP Descriptor
	*pCurr++ = USB_UVC_EP_LENGTH;
	*pCurr++ = CS_ENDPOINT;
	*pCurr++ = EP_INTERRUPT;
	*pCurr++ = UVAC_INTR_EP_MAX_PACKET_SIZE;
	*pCurr++ = UVAC_INTR_EP_MAX_PACKET_SIZE >> 8;

	*pIntfNum = *pIntfNum + 1;                                  //next interface number/idx
	//Standard VS interface Descriptor, ALT0, NumOfEP=0
	gU2UvacIntfIdx_VS[gU2UvacIntfIdx_vidAryCurrIdx] = *pIntfNum;
	DBG_IND("VidStrm[%d] Intf=%d\r\n", gU2UvacIntfIdx_vidAryCurrIdx, gU2UvacIntfIdx_VS[gU2UvacIntfIdx_vidAryCurrIdx]);
	gU2UvacIntfIdx_vidAryCurrIdx++;
	intfAltSet = 0;
	tmpV8 = gU2UvcCapM3Enable ? UVAC_IF_VS_EP_CNT : 0;
	tmpVal = UVAC_MakeStrmIntfDesc(pCurr, *pIntfNum, intfAltSet, tmpV8, CC_VIDEO);
	pCurr += tmpVal;
	intfAltSet++;  //1

	//Class-Specific VS Header input Descriptor
	//V1-EP1
	tmpLen = USB_UVC_VS_INPUT_HEADER_LENGTH;

	*pCurr++ = tmpLen;
	*pCurr++ = CS_INTERFACE;
	*pCurr++ = VS_INPUT_HEADER;

	//H264 and MJPEG -> compression quality
	*pCurr++ = 1;    //video format count, only H264 or only MJPEG

	pTmp = pCurr;          //keep for updating total length of the Class-Specific VS Header input Descriptor
	*pCurr++ = 0;          //total length of the Class-Specific VS Header input Descriptor
	*pCurr++ = 0;
	*pCurr++ = epAddr;
	*pCurr++ = UVC_VS_BMINFO;
	*pCurr++ = UVC_TERMINAL_ID_OT;
	if (gU2UvcCapM3Enable) {
		*pCurr++ = UVC_VS_STILL_CAPTURE_M3;
		*pCurr++ = UVC_VS_HW_TRIGGER_ENABLE;
		*pCurr++ = UVC_VS_HW_TRIGGER_USAGE_GENARAL;
	} else {
		*pCurr++ = UVC_VS_STILL_CAPTURE_NONE;
		*pCurr++ = UVC_VS_HW_TRIGGER_DISABLE;
		*pCurr++ = UVC_VS_HW_TRIGGER_USAGE_INIT;
	}
	*pCurr++ = UVC_VS_CTRL_SIZE;
	*pCurr++ = UVC_VS_CONTROLS;

	if (gUvcMjpgFrmInfo[vidDevIdx].aryCnt) {
		//Class-Specific VS Format Descriptor (MJPG)
		tmpLen += USB_UVC_VS_MJPG_FORMAT_LENGTH;
		*pCurr++ = USB_UVC_VS_MJPG_FORMAT_LENGTH;
		*pCurr++ = CS_INTERFACE;
		*pCurr++ = VS_FORMAT_MJPEG;
		*pCurr++ = format_index;   //index of this format descriptor
		mjpeg_fmt_index[vidDevIdx] = format_index;
		format_index++;
		*pCurr++ = gUvcMjpgFrmInfo[vidDevIdx].aryCnt;              //number of frame descriptor => how many resolutions
		DbgMsg_UVC(("Reso-Cnt=%d of VS[%d]-MJPG\r\n", gUvcMjpgFrmInfo[vidDevIdx].aryCnt, *pIntfNum));
		*pCurr++ = UVC_VSFMT_BMINFO;
		if (gUvcMjpgFrmInfo[vidDevIdx].bDefaultFrameIndex > gUvcMjpgFrmInfo[vidDevIdx].aryCnt|| gUvcMjpgFrmInfo[vidDevIdx].bDefaultFrameIndex == 0) {
			*pCurr++ = UVC_VSFMT_DEF_FRAMEINDEX;
			DBG_ERR("mjpg[%d] bDefaultFrameIndex(%d), aryCnt(%d)!\r\n", vidDevIdx, gUvcMjpgFrmInfo[vidDevIdx].bDefaultFrameIndex, gUvcMjpgFrmInfo[vidDevIdx].aryCnt);
		} else {
			*pCurr++ = (UINT8)gUvcMjpgFrmInfo[vidDevIdx].bDefaultFrameIndex;
		}
		*pCurr++ = UVC_VSFMT_ASPECTRATIO_X;
		*pCurr++ = UVC_VSFMT_ASPECTRATIO_Y;
		*pCurr++ = UVC_VSFMT_INTERLACE;
		*pCurr++ = UVC_VSFMT_COPYPROTECT;

		//Class-Specific VS Frame Descriptor(MJPG)
		tmpVal = UVAC_MakeFrmDesc(pCurr, VS_FRAME_MJPEG, &gUvcMjpgFrmInfo[vidDevIdx]);
		pCurr += tmpVal;
		tmpLen += tmpVal;

		//Color matching Descriptor
		tmpVal = UVAC_MakeColorMatch(pCurr);
		pCurr += tmpVal;
		tmpLen += tmpVal;
	}

	if (gUvcH264FrmInfo[vidDevIdx].aryCnt) {
		//Class-Specific VS Format Descriptor(H264)
		tmpLen += USB_UVC_VS_FBH264_FORMAT_LENGTH;
		*pCurr++ = USB_UVC_VS_FBH264_FORMAT_LENGTH;
		*pCurr++ = CS_INTERFACE;
		*pCurr++ = VS_FORMAT_FRAME_BASED;
		*pCurr++ = format_index;   //index of this format descriptor
		h264_fmt_index[UVAC_VID_DEV_CNT_2] = format_index;
		format_index++;
		*pCurr++ = gUvcH264FrmInfo[vidDevIdx].aryCnt;              //number of frame descriptor => how many resolutions
		DbgMsg_UVC(("Reso-Cnt=%d of VS[%d]-H264\r\n", gUvcH264FrmInfo[vidDevIdx].aryCnt, *pIntfNum));
		for (i = 0; i < UVAC_H264_GUIDFMT_LEN; i++) {
			*pCurr++ = gU2UvacH264GuidFormat[i];
		}
		*pCurr++ = UVC_BITS_PER_PIXEL;
		if (gUvcH264FrmInfo[vidDevIdx].bDefaultFrameIndex > gUvcH264FrmInfo[vidDevIdx].aryCnt || gUvcH264FrmInfo[vidDevIdx].bDefaultFrameIndex == 0) {
			*pCurr++ = UVC_VSFMT_DEF_FRAMEINDEX;
			DBG_ERR("H264[%d] def_frm_idx(%d), aryCnt(%d)!\r\n", vidDevIdx, gUvcH264FrmInfo[vidDevIdx].bDefaultFrameIndex, gUvcH264FrmInfo[vidDevIdx].aryCnt);
		} else {
			*pCurr++ = (UINT8)gUvcH264FrmInfo[vidDevIdx].bDefaultFrameIndex;
		}

		*pCurr++ = UVC_VSFMT_ASPECTRATIO_X;
		*pCurr++ = UVC_VSFMT_ASPECTRATIO_Y;
		*pCurr++ = UVC_VSFMT_INTERLACE;
		*pCurr++ = UVC_VSFMT_COPYPROTECT;
		*pCurr++ = UVC_VS_FRM_VARIABLE_SIZE;

		//Class-Specific VS Frame Descriptor(H264)
		tmpVal = UVAC_MakeFrmDesc(pCurr, VS_FRAME_FRAME_BASED, &gUvcH264FrmInfo[vidDevIdx]);
		pCurr += tmpVal;
		tmpLen += tmpVal;

		//Color matching Descriptor
		tmpVal = UVAC_MakeColorMatch(pCurr);
		pCurr += tmpVal;
		tmpLen += tmpVal;
	}


	//Updating total length of the Class-Specific VS Header input Descriptor
	*pTmp++ = tmpLen; //More add:USB_UVC_VS_COLOR_LENGTH+USB_BULK_EP_LENGTH
	*pTmp = tmpLen >> 8;

	//VID2 only support 512x1(4MB/sec)
	{
		UINT16 packetSize;
		//Standard VS interface Descriptor, ALT(1+i), NumOfEP=1
		tmpVal = UVAC_MakeStrmIntfDesc(pCurr, *pIntfNum, intfAltSet, UVAC_IF_VS_EP_CNT, CC_VIDEO);
		pCurr += tmpVal;
		//high bandwidth, [12:11] 01 for extra 1 packet and 10 for extra 2 packet.
		//packetSize = UVC_USB_FIFO_UNIT_SIZE | (BLKNUM_SINGLE - 1) << 11;
		packetSize = gU2UvcIsoInHsPacketSize | (gU2UvcIsoInBandWidth - 1) << 11;
		//Standard Iso EP Descriptor
		tmpVal = UVAC_MakeEPDesc(pCurr, epAddr, (USB_EP_ATR_ISO | USB_EP_ATR_ASYNC), packetSize, USB_EP_ISO_INTERVAL);

		pCurr += tmpVal;
	}
	DbgMsg_UVCIO(("-MakeVidConfigDesc2:sAddr=0x%x, endAddr=0x%x, intf=%d, alt=%d\r\n", ptr, pCurr, *pIntfNum, intfAltSet));
	return (pCurr - ptr);
}
UINT16 U2UVAC_MakeHSConfigDesc(UINT8 *pUvacHSConfigDesc, UINT16 maxLen)
{
	UINT8 *pCurr = pUvacHSConfigDesc;
	UINT8 intfNum = UVAC_FIRST_IF_IDX;
	UINT8 totalIntf;// = gU2UvacWinIntrfEnable ? (4 * gU2UvacChannel + 1) : (4 * gU2UvacChannel); //vc+vs+ac+as
	UINT16 descLen = 0;
	UINT8 tmpV8 = 0, tmpIntrEP = 0;
	UINT16 tmpLen = 0;
	UINT32 cdc_max;

#if USB_2ND_CDC
	cdc_max = CDC_COM_2ND;
#else
	cdc_max = CDC_COM_1ST;
#endif

	if (gU2UvacChannel == UVAC_CHANNEL_1V1A) {
		totalIntf = 4;
	} else if (gU2UvacChannel == UVAC_CHANNEL_2V2A) {
		totalIntf = 8;
	} else if (gU2UvacChannel == UVAC_CHANNEL_2V1A) {
		totalIntf = 6;
	} else {
		DBG_ERR("UVAC channel(%x) invalid!\r\n", (unsigned int)gU2UvacChannel);
		return 0;
	}
	if (gDisableUac) {
		if (gU2UvacChannel == UVAC_CHANNEL_2V2A) {
			totalIntf -= 4;
		} else {
			totalIntf -= 2;
		}
	}
	DbgMsg_UVCIO((" inPtr=0x%x, totalIntf=%d, maxLen=0x%x +\r\n", pUvacHSConfigDesc, totalIntf, maxLen));
	if (pUvacHSConfigDesc && maxLen) {
		//Standard Configuration Descriptor
		*pCurr++ = USB_CFG_LENGTH;
		*pCurr++ = USB_CONFIGURATION_DESCRIPTOR_TYPE;
		*pCurr++ = 0;                               //Total Description length
		*pCurr++ = 0;
		if (gU2UvacCdcEnabled) {
			UINT32 ComID;
			for (ComID = CDC_COM_1ST; ComID <= cdc_max; ComID++) {
				totalIntf += CDC_INTERFACE_NUM;
			}
		}
		#if 0//gU2UvacMtpEnabled
		if (gU2UvacMtpEnabled) {
			totalIntf += SIDC_INTERFACE_NUM;
		}
		#endif
		if (g_dfu_info.en) {
			totalIntf += DFU_INTERFACE_NUM;
		}
		if (g_hid_info.en) {
			totalIntf += HID_INTERFACE_NUM;
		}
		if (gU2UvacUacRxEnabled) {
			totalIntf += 2;
		}
		if (g_msdc_info.en) {
			totalIntf += 1;
		}
		*pCurr++ = totalIntf;
		*pCurr++ = UVAC_CFGA_CFG_VALUE;
		*pCurr++ = UVAC_CFGA_CFG_IDX;
		*pCurr++ = UVAC_CFGA_CFG_ATTRIBUES;
		*pCurr++ = UVAC_CFGA_MAX_POWER;

		DBG_IND("+Vid intfNum=%d\r\n", intfNum);
		tmpV8 = U2UVAC_USB_EP[UVAC_TXF_QUE_V1];
		if (USB_EP_MAX > tmpV8) {
			tmpV8 |= USB_EP_IN_ADDRESS;
			DBG_IND("Add EP=0x%x for V1\r\n", tmpV8);
		} else {
			DBG_ERR("V1-EP=%d > USB_EP_MAX=%d\r\n", tmpV8, USB_EP_MAX);
		}
		tmpIntrEP = U2UVAC_USB_INTR_EP[UVAC_EP_INTR_V1];
		if (USB_EP_MAX > tmpIntrEP) {
			tmpIntrEP |= USB_EP_IN_ADDRESS;
			DBG_IND("Add Intr-EP=0x%x for V1\r\n", tmpIntrEP);
		} else {
			DBG_ERR("V1-Intr-EP=%d > USB_EP_MAX=%d\r\n", tmpIntrEP, USB_EP_MAX);
		}
		tmpLen = UVAC_MakeVidConfigDesc(pCurr, &intfNum, UVAC_IF_VS_CNT, tmpV8, tmpIntrEP);
		DBG_IND("-Vid intfNum=%d\r\n", intfNum);
		pCurr += tmpLen;

		if (gDisableUac == FALSE) {
			//Audio Configuration Descriptor of A1
			intfNum++;
			tmpV8 = U2UVAC_USB_EP[UVAC_TXF_QUE_A1] | USB_EP_IN_ADDRESS;
			DBG_IND("+Aud intfNum=%d\r\n", intfNum);
			tmpLen = UVAC_MakeAudioConfigDesc(pCurr, &intfNum, UVAC_IF_AS_CNT, tmpV8, gU2UvacAudFmtType);
			DBG_IND("-Aud intfNum=%d\r\n", intfNum);
			pCurr += tmpLen;
		}
		if (gU2UvacChannel > UVAC_CHANNEL_1V1A) {
			//Video Configuration Descriptor of V2
			intfNum++;
			DBG_IND("+Vid intfNum=%d\r\n", intfNum);
			tmpV8 = U2UVAC_USB_EP[UVAC_TXF_QUE_V2] | USB_EP_IN_ADDRESS;
			if (USB_EP_MAX < U2UVAC_USB_EP[UVAC_TXF_QUE_V2]) {
				DBG_ERR("V2-EP=0x%x > USB_EP_MAX=%d\r\n", tmpV8, USB_EP_MAX);
			}
			tmpIntrEP = U2UVAC_USB_INTR_EP[UVAC_EP_INTR_V2];
			if (USB_EP_MAX > tmpIntrEP) {
				tmpIntrEP |= USB_EP_IN_ADDRESS;
				DBG_IND("Add Intr-EP=0x%x for V2\r\n", tmpIntrEP);
			} else {
				DBG_ERR("V2-Intr-EP=%d > USB_EP_MAX=%d\r\n", tmpIntrEP, USB_EP_MAX);
			}
			tmpLen = UVAC_MakeVidConfigDesc2(pCurr, &intfNum, UVAC_IF_VS_CNT, tmpV8, tmpIntrEP);
			DBG_IND("-Vid intfNum=%d\r\n", intfNum);
			pCurr += tmpLen;
			if (gDisableUac == FALSE && gU2UvacChannel == UVAC_CHANNEL_2V2A) {
				//The whole Audio Configuration of A2
				intfNum++;
				tmpV8 = U2UVAC_USB_EP[UVAC_TXF_QUE_A2] | USB_EP_IN_ADDRESS;
				DBG_IND("+Aud intfNum=%d\r\n", intfNum);
				tmpLen = UVAC_MakeAudioConfigDesc(pCurr, &intfNum, UVAC_IF_AS_CNT, tmpV8, gU2UvacAudFmtType);
				DBG_IND("-Aud intfNum=%d\r\n", intfNum);
				pCurr += tmpLen;
			}
		}

		if (gU2UvacUacRxEnabled) {
			//The whole Audio Configuration of audio out (receive)
			intfNum++;
			tmpV8 = U2UVAC_USB_RX_EP[UVAC_RXF_QUE_A1] | USB_EP_OUT_ADDRESS;
			tmpLen = UVAC_MakeAudioOutConfigDesc(pCurr, &intfNum, UVAC_IF_AS_CNT, tmpV8, gU2UvacAudFmtType);
			pCurr += tmpLen;
		}

		//Standard Interface Descriptor for WinUSB
		if (gU2UvacWinIntrfEnable) {
			intfNum++;
			gU2UvacIntfIdx_WinUsb = intfNum;
			tmpLen = UVAC_MakeStandIntrfDesc(pCurr, intfNum, UVAC_WINUSB_IF_CLASS_CODE, UVAC_IF_WINUSB_EP_CNT);
			pCurr += tmpLen;

			tmpV8 = U2UVAC_USB_EP[UVAC_TXF_QUE_IMG];
			tmpV8 |= USB_EP_IN_ADDRESS;
			tmpLen = UVAC_MakeEPDesc(pCurr, tmpV8, USB_EP_ATR_BULK, gU2UvacVidEPMaxPackSize[0], USB_EP_BULK_INTERVAL);
			pCurr += tmpLen;
		}

		if (gU2UvacCdcEnabled) {
			UINT32 ComID;
			for (ComID = CDC_COM_1ST; ComID <= cdc_max; ComID++) {
				intfNum++;
				DBG_IND("+CDC intfNum=%d\r\n", intfNum);
				tmpLen = UVAC_MakeCdcConfigDesc(ComID, pCurr, &intfNum);
				DBG_IND("-CDC intfNum=%d, tmpLen=%d\r\n", intfNum, tmpLen);
				pCurr += tmpLen;
			}
		}
#ifdef __LINUX_USER__
		if (g_msdc_info.en) {
			PMSDC_OBJ p_msdc_object;

			intfNum++;
			tmpLen = UVAC_MakeMsdcConfigDesc(pCurr, intfNum);
			pCurr += tmpLen;

			p_msdc_object = Msdc_getObject(MSDC_ID_USB20);
			p_msdc_object->SetConfig(USBMSDC_CONFIG_ID_INTERFACE_IDX, intfNum);
			p_msdc_object->SetConfig(USBMSDC_CONFIG_ID_RESERVED, MSDC_IN_EP);
			g_u2uvac_msdc_device_evt_cb = (USB_GENERIC_CB)p_msdc_object->GetConfig(USBMSDC_CONFIG_ID_EVT_CB);
			g_u2uvac_msdc_class_req_cb = (FP)p_msdc_object->GetConfig(USBMSDC_CONFIG_ID_CLASS_CB);
		}
#endif
		if (g_hid_info.en) {
			intfNum++;
			DBG_IND("HID intfNum=%d\r\n", intfNum);
			tmpLen = UVAC_MakeHidConfigDesc(pCurr, intfNum);
			pCurr += tmpLen;
		}
		if (g_dfu_info.en) {
			intfNum++;
			DBG_IND("DFU intfNum=%d\r\n", intfNum);
			tmpLen = UVAC_MakeDfuConfigDesc(pCurr, intfNum);
			pCurr += tmpLen;
		}
		#if 0//gU2UvacMtpEnabled
		if (gU2UvacMtpEnabled) {
			intfNum++;
			DBG_IND("SIDC intfNum=%d\r\n", intfNum);
			tmpLen = UVAC_MakeSidcConfigDesc(pCurr, intfNum);
			pCurr += tmpLen;
		}
		#endif
		//Fill the total length of the whole descriptor
		descLen = pCurr - pUvacHSConfigDesc;
		pCurr = pUvacHSConfigDesc + UVAC_CONFIG_DESC_LEN_OFS;
		*pCurr++ = (UINT8)(descLen & 0xff);
		*pCurr   = (UINT8)(descLen >> 0x08);

		if (maxLen < descLen) {
			DBG_ERR("allocateLen=0x%x < needLen=0x%x\r\n", maxLen, descLen);
			DBG_ERR("allocateLen=0x%x < needLen=0x%x\r\n", maxLen, descLen);
			DBG_ERR("allocateLen=0x%x < needLen=0x%x\r\n", maxLen, descLen);
		}
		UVAC_DbgDmpConfigDesc(pUvacHSConfigDesc, descLen);

#if (_UVC_DBG_LVL_ >= _UVC_DBG_CHK_)
		if (intfNum >= totalIntf) {
			DBG_ERR("lastIntf=%d, totalIntf=%d\r\n", intfNum, totalIntf);
		}
#endif
	} else {
		DBG_ERR("Any one, 0x%x, 0x%x NULL\r\n", (unsigned int)pUvacHSConfigDesc, maxLen);
	}

	DbgMsg_UVC(("maxLen=%d, currLen=%d, totalIntf=%d -\r\n", maxLen, descLen, totalIntf));
	return descLen;
}
