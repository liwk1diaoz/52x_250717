/**
    Copyright   Novatek Microelectronics Corp. 2004.  All rights reserved.

    @file       UVAC.h
    @ingroup    mISYSUVAC

    @brief      USB PCC device class API header.
                USB PCC device class API header.

*/

#ifndef _UVAC_H
#define _UVAC_H

/*-----------------------------------
    Includes
-----------------------------------*/
#include "kwrap/type.h"


#define UVAC_VID_RESO_MAX_CNT               16
#define UVAC_VID_RESO_FPS_MAX_CNT           3

#define UVAC_AUD_SAMPLE_RATE_MAX_CNT        3

#define SET_CUR                             0x01
#define GET_CUR                             0x81
#define GET_MIN                             0x82
#define GET_MAX                             0x83
#define GET_RES                             0x84
#define GET_LEN                             0x85
#define GET_INFO                            0x86
#define GET_DEF                             0x87

#define ERROR_CODE_WRONG_STATE        0x2
#define ERROR_CODE_OUT_OF_RANGE       0x4
#define ERROR_CODE_INVALID_CONTROL    0x6
#define ERROR_CODE_INVALID_REQUEST    0x7

/**
    @addtogroup mISYSUVAC
*/
//@{

/**
    Video format for UVC, UV packed only.
*/
typedef enum _UVAC_VIDEO_FORMAT {
	UVAC_VIDEO_FORMAT_H264,                ///< H264 format with YCbCr 420, UV packed.
	UVAC_VIDEO_FORMAT_MJPG,                ///< MJPEG format with YCbCr 422, UV packed.
	UVAC_VIDEO_FORMAT_YUV,                 ///< YUV format.
	ENUM_DUMMY4WORD(UVAC_VIDEO_FORMAT)
} UVAC_VIDEO_FORMAT;

/**
    UVC version.
*/
typedef enum {
	UVAC_UVC_VER_100 = 0,  // UVC 1.0
	UVAC_UVC_VER_110,      // UVC 1.1
	UVAC_UVC_VER_150,      // UVC 1.5
} UVAC_UVC_VER;
/**
    Video Resolution.

    @note For video resolution of UVAC_VID_RESO_ARY
*/
typedef struct _UVAC_VID_RESO_ {
	UINT32 width;                            ///< Width.
	UINT32 height;                           ///< Height.
	UINT32 fpsCnt;                           ///< The number of fps .
	UINT32 fps[UVAC_VID_RESO_FPS_MAX_CNT];   ///< FPS array.
} UVAC_VID_RESO, *PUVAC_VID_RESO;

/**
    Video Resolution array.

    This structure is used in UVAC_ConfigVidReso(..) to set supported video resolution, and
    it must be set before UVAC_Open() is called.
*/
typedef struct _UVAC_VID_RESO_ARY_ {
	UINT32 aryCnt;                          ///< The number of video resolution array.
	UVAC_VID_RESO *pVidResAry;              ///< Video resolution.
	UINT8 bDefaultFrameIndex;               ///< bDefaultFrameIndex, referring to the SPEC of Video Format Descriptor.
} UVAC_VID_RESO_ARY, *PUVAC_VID_RESO_ARY;

/**
    Audio sample array.

    This structure is used for the configuration of UVAC_CONFIG_AUD_SAMPLERATE to set supported audio resolution, and
    it must be set before UVAC_Open() is called.
*/
typedef struct _UVAC_AUD_SAMPLERATE_ARY_ {
	UINT32 aryCnt;                          ///< The number of audio resolution array.
	UINT32 *pAudSampleRateAry;              ///< Audio resolution.
} UVAC_AUD_SAMPLERATE_ARY, *PUVAC_AUD_SAMPLERATE_ARY;

/**
    Structure used for UVAC Device Vendor Request Callback API.

    This structure is used in the callback prototype UVAC_VENDOR_REQ_CB.
*/
typedef struct _UVAC_VENDOR_PARAM {
	BOOL      bHostToDevice;      ///< Data direction.
	///< - @b TRUE: USB Host sent data to USB Device.
	///< - @b FALSE: USB Host will get data from USB Device.
	UINT8     uiReguest;          ///< The bRequest field in USB Device Requests.
	UINT16    uiValue;            ///< The wValue field in USB Device Requests.
	UINT16    uiIndex;            ///< The wIndex field in USB Device Requests.
	UINT32    uiDataAddr;         ///< Data address.
	UINT32    uiDataSize;         ///< Data size, in byte, maximun is 64. If there is no data to transfer, this should be zero.
} UVAC_VENDOR_PARAM, *PUVAC_VENDOR_PARAM;

/**
    COM ID for CDC.

    It supports dual COM port.
*/
typedef enum _CDC_COM_ID {
	CDC_COM_1ST = 0,                       ///< The first COM ID.
	CDC_COM_2ND,                           ///< The second COM ID.
	CDC_COM_MAX_NUM,
	ENUM_DUMMY4WORD(CDC_COM_ID)
} CDC_COM_ID;

/**
    Callback function prototype for PSTN subclass request.
*/
typedef BOOL (*UVAC_CDC_PSTN_REQUEST_CB)(CDC_COM_ID ComID, UINT8 PstnRequestCode, UINT8 *pData, UINT32 *pDataLen);


/**
    Callback function prototype for UVAC Device Vendor Request.
    @return
     - @UINT32 0:  Ok.
     - @UINT32 1,2,...: TBD.
*/
typedef UINT32(*UVAC_VENDOR_REQ_CB)(PUVAC_VENDOR_PARAM pParam);

/**
    Callback function prototype for WINUSB.
*/
typedef UINT8(*UVAC_WINUSBCLSREQCB)(UINT32 ControlCode, UINT8 CS, UINT8 *pData, UINT32 *pDataLen);


/**
    Callback function prototype for Unit

    @param[in] CS Control Selector.
    @param[in] request The field of bRequest in UVC SPEC.
    @param[in] pData The address of data received from USB Host or data will be sent to USB Host.
    @param[in,out] pDataLen
			[in] The size of data from USB Host
			[out] The size of data needed for USB Host. If there is no data for USB Host, set zero before returning.
    The bRequest field indicates which attribute the request is manipulating.
    @return
     - @TRUE: command OK.
     - @FALSE: Driver will send STALL to USB Host
*/
typedef BOOL (*UVAC_UNIT_CB)(UINT32 CS, UINT8 request, UINT8 *pData, UINT32 *pDataLen);
/**
    Callback function prototype for HW memory copy.
*/
typedef VOID(*UVAC_HWCOPYCB)(UINT32 uiDst, UINT32 uiSrc, UINT32 uiSize);


/**
    String Descriptor
*/
typedef  struct _UVAC_STRING_DESC {
	UINT8       bLength;                ///< size of String Descriptor
	UINT8       bDescriptorType;        ///< String Descriptor type
	UINT8       bString[254];           ///< UNICODE stirng
} UVAC_STRING_DESC;

/**
    UVC Extension Unit Descriptor
*/
typedef  struct _UVAC_EU_DESC {
	UINT8 bLength;                ///< Size of String Descriptor.
	UINT8 bDescriptorType;        ///< CS_INTERFACE descriptor type.
	UINT8 bDescriptorSubtype;     ///< VC_EXTENSION_UNIT descriptor subtype.
	UINT8 bUnitID;                ///< Unit ID, set 0 for default value.
	UINT8 guidExtensionCode[16];  ///< GUID.
	UINT8 bNumControls;           ///< Number of controls in this extension unit.
	UINT8 bNrInPins;              ///< Number of Input Pins of this Unit.
	UINT8 *baSourceID;            ///< Pointer to a static Source ID array, set 0 for default value.
	UINT8 bControlSize;           ///< Size of the bmControls field.
	UINT8 *bmControls;            ///< Pointer to static bmControl array.
	UINT8 iExtension;             ///< Index of a string descriptor that describes this extension unit.
	UVAC_UNIT_CB eu_cb;           ///< Extension unit callback
} UVAC_EU_DESC;


typedef enum _UVAC_DFU_EVENT {
	UVAC_DFU_REQUEST,              ///< Callback when the host issue a DFU class requess.
	UVAC_DFU_USB_RESET,            ///< Callback when the host issue a USB reset.
	ENUM_DUMMY4WORD(UVAC_DFU_EVENT)
} UVAC_DFU_EVENT;

/**
    Callback function prototype for DFU.

	@param[in] event Callback type, referring to UVAC_DFU_EVENT.
    @param[in] request The field of bRequest in DFU Class-Specific Requests, only valid while event is UVAC_DFU_REQUEST.
    @param[in] value The field of wValue in DFU Class-Specific Requests, only valid while event is UVAC_DFU_REQUEST.
    @param[in] p_data The address of data received from USB Host or data will be sent to USB Host, only valid while event is UVAC_DFU_REQUEST.
    @param[in,out] p_dataLen Only valid while event is UVAC_DFU_REQUEST.
			[in] The size of data from USB Host
			[out] The size of data needed for USB Host. If there is no data for USB Host, set zero before returning.

    @return
     - @TRUE: command OK.
     - @FALSE: Driver will send STALL to USB Host
*/
typedef BOOL (*UVAC_DFU_CB)(UVAC_DFU_EVENT event, UINT8 request, UINT16 value, UINT8 *p_data, UINT32 *p_dataLen);

typedef struct _UVAC_DFU_INFO {
	BOOL              en;              ///< Enable DFU function or not
	UVAC_DFU_CB       cb;              ///< DFU callback function, referring to UVAC_DFU_CB.
	UVAC_STRING_DESC *p_vendor_string; ///< DFU vendor string descriptor ptr
	UINT16            detach_time_out; ///< wDetachTimeOut, in milliseconds, that the device will wait after receipt of the DFU_DETACH request.
	UINT16            transfer_size;   ///< wTransferSize, maximum number of bytes that the device can accept per control-write transaction.
	MEM_RANGE         download_buf;    ///< buffer for DFU_DNLOAD.
	UINT8             attributes;      ///< bmAttributes, refer to "DFU Functional Descriptor" in DFU SPEC.
} UVAC_DFU_INFO;


/**
    Callback function prototype for HID.

    @param[in] request The field of bRequest in HID Class-Specific Requests.
    @param[in] value The field of wValue in HID Class-Specific Requests.
    @param[in] p_data The address of data received from USB Host or data will be sent to USB Host.
    @param[in,out] p_dataLen
   				[in] The size of data from USB Host
   				[out] The size of data needed for USB Host.

    @return
     - @TRUE: command OK.
     - @FALSE: Driver will send STALL to USB Host
*/
typedef BOOL (*UVAC_HID_CB)(UINT8 request, UINT16 value, UINT8 *p_data, UINT32 *p_dataLen);

/**
    UVC Extension Unit Descriptor
*/
typedef _PACKED_BEGIN struct _UVAC_HID_DESC {
	UINT8   bLength;              ///< The total size of the HID descriptor.
	UINT8   bHidDescType;         ///< Constant name specifying type of HID descriptor.
	UINT16  bcdHID;               ///< The HID Class Specification release.
	UINT8   bCountryCode;         ///< Country code of the localized hardware.
	UINT8   bNumDescriptors;      ///< The number of class descriptors.
	UINT8   bDescriptorType;      ///< Constant name identifying type of class descriptor.
	UINT16  wDescriptorLength;    ///< The total size of the Report descriptor.
	UINT8  *p_desc;               ///< Reserved for optional descriptors.
} _PACKED_END UVAC_HID_DESC;

/**
    HID information
*/
typedef struct _UVAC_HID_INFO {
	BOOL              en;              ///< Enable HID function or not
	UVAC_HID_CB       cb;              ///< HID callback function for class request, referring to UVAC_HID_CB.
	UVAC_HID_DESC     hid_desc;        ///< HID descriptor.
	UINT8             *p_report_desc;  ///< Pointer to a report descriptor which should be a static array.
	BOOL              intr_out;        ///< Enable Interrupt out endpoint.
	MEM_RANGE         data_stage_buf;  ///< Buffer for receiving data from control transfer, only used when the data length greater than 64.
	UVAC_STRING_DESC *p_vendor_string; ///< Vendor string descriptor ptr
	UINT8             intr_interval;   ///< The bInterval in Endpoint Descritptor
} UVAC_HID_INFO;

/**
	MSDC information
*/
typedef struct _UVAC_MSDC_INFO {
	BOOL              en;              ///< Enable MSDC function or not
} UVAC_MSDC_INFO;


/**
    MSOS20 information
*/
typedef struct _UVAC_MSOS20_INFO {
	BOOL              en;                ///< Enable Microsoft OS 2.0 descriptor or not
	UINT8             *p_msos20_desc;    ///< Pointer to a descriptor which should be a static array.
	UINT32            msos20_desc_size;  ///< Size of Microsoft OS 2.0 descriptor.
} UVAC_MSOS20_INFO;

/**
    Device Descriptor
*/
typedef struct _UVAC_VEND_DEV_DESC {
	UINT16              VID;                 ///< Customized USB Vendor ID
	UINT16              PID;                 ///< Customized USB Product ID
	UVAC_STRING_DESC    *pManuStringDesc;    ///< USB Manufacturer String Descriptor ptr
	UVAC_STRING_DESC    *pProdStringDesc;    ///< USB Product String Descriptor ptr
	UVAC_STRING_DESC    *pSerialStringDesc;  ///< USB Serial Number String Descriptor ptr
	UVAC_VENDOR_REQ_CB  fpVendReqCB;         ///< Obsolete.
	UVAC_VENDOR_REQ_CB  fpIQVendReqCB;       ///< Obsolete.
} UVAC_VEND_DEV_DESC, *PUVAC_VEND_DEV_DESC;

/**
    UVAC channel.

    @note For UVAC_INFO
*/
typedef enum {
	UVAC_CHANNEL_1V1A = 1,      ///< One video and one audio channel.
	UVAC_CHANNEL_2V2A,          ///< Two video and two audio channel.
	UVAC_CHANNEL_2V1A,          ///< Two video and one audio channel.
	UVAC_CHANNEL_MAX,
} UVAC_CHANNEL;

/**
    Audio format

    @note For UVAC_CONFIG_AUD_FMT_TYPE.
*/
typedef enum _UVAC_AUD_FMT_TYPE {
	UVAC_AUD_FMT_TYPE_PCM,      ///< Audio type is PCM.
	UVAC_AUD_FMT_TYPE_AAC,      ///< Audio type is AAC.
	UVAC_AUD_FMT_TYPE_MAX,
} UVAC_AUD_FMT_TYPE;

/**
    Stream path.
*/
typedef enum _UVAC_STRM_PATH {
	UVAC_STRM_VID,   ///< 1st video bitstream
	UVAC_STRM_AUD,   ///< 1st audio bitstream
	UVAC_STRM_VID2,  ///< 2nd video bitstream
	UVAC_STRM_AUD2,  ///< 2nd audio bitstream
	UVAC_STRM_AUDRX,///< audio bitstream
	ENUM_DUMMY4WORD(UVAC_STRM_PATH)
} UVAC_STRM_PATH, *PUVAC_STRM_PATH;

/**
    Stream format.

    @note For UVAC_SetEachStrmInfo.
*/
typedef struct _UVAC_STRM_FRM {
	UINT32              addr;       ///< Address of stream.
	UINT32              size;       ///< Size of stream.
	UVAC_STRM_PATH      path;       ///< Stream path.
	UINT8               *pStrmHdr;  ///< VS header address.
	UINT32              strmHdrSize;///< VS header size.
	UINT32              va;         ///< Virtual address of stream, for YUV frame only.
	UINT64              timestamp;  ///< Timestamp.
} UVAC_STRM_FRM, *PUVAC_STRM_FRM;

/**
    Stream information.

    Request from USB Host.
*/
typedef struct _UVAC_STRM_INFO {
	UINT8               *pStrmHdr;      ///< H264 header address.
	UINT32              strmHdrSize;    ///< H264 header size.
	UVAC_STRM_PATH      strmPath;       ///< Stream path.
	UINT32              strmWidth;      ///< Stream width.
	UINT32              strmHeight;     ///< Stream height.
	UINT32              strmFps;        ///< Frame rate.
	UINT32              strmTBR;        ///< Target bitrate (Byte/Sec).
	UVAC_VIDEO_FORMAT   strmCodec;      ///< Stream format.
	UINT32              strmResoIdx;    ///< Index of video resolution.
	BOOL                isStrmOn;       ///< Indicate if the stream is active.
	BOOL                isAudStrmOn;    ///< Indicate if the audio stream is active.
} UVAC_STRM_INFO, *PUVAC_STRM_INFO;

/**
    Supported video format.

    @note For UVAC_CONFIG_VIDEO_FORMAT.
*/
typedef enum _UVAC_VIDEO_FORMAT_TYPE {
	UVAC_VIDEO_FORMAT_H264_MJPEG,          ///< Support H264 and MJPEG.
	UVAC_VIDEO_FORMAT_H264_ONLY,           ///< Support H264 only.
	UVAC_VIDEO_FORMAT_MJPEG_ONLY,          ///< Support MJPEG only.
	ENUM_DUMMY4WORD(UVAC_VIDEO_FORMAT_TYPE)
} UVAC_VIDEO_FORMAT_TYPE;

/**
    Video Stream buffer information.

    Set video stream buffer for internal converting VA.
*/
typedef struct _UVAC_VID_BUF_INFO {
	UINT32      vid_buf_pa;        ///< Address of video stream buffer
	UINT32      vid_buf_size;      ///< Size of video stream buffer.
	UINT32      vid2_buf_pa;       ///< Address of senond video stream buffer
	UINT32      vid2_buf_size;     ///< Size of senond video stream buffer
} UVAC_VID_BUF_INFO, *PUVAC_VID_BUF_INFO;

/**
    UVAC configuration identifier.

    This definition is used in UVAC_SetConfig() to assgin new configuration to specified ID function.
*/
typedef enum _UVAC_CONFIG_ID {
	UVAC_CONFIG_VEND_DEV_DESC,              ///< Pointer to the vendor device descriptor, refering to #_UVAC_VEND_DEV_DESC.
	UVAC_CONFIG_AUD_SAMPLERATE,             ///< Pointer to audio sample array, refering to #_UVAC_AUD_SAMPLERATE_ARY_.
	UVAC_CONFIG_AUD_FMT_TYPE,               ///< Audio format, refering to #_UVAC_AUD_FMT_TYPE
	UVAC_CONFIG_H264_TARGET_SIZE,           ///< H.264 stream target size(Byte/Sec). Default value is 0x100000.
	UVAC_CONFIG_MJPG_TARGET_SIZE,           ///< Motion JPEG stream target size(Byte/Sec). Default value is 0x300000.
	UVAC_CONFIG_VENDOR_CALLBACK,            ///< The callback function for USB Vendor Request
	UVAC_CONFIG_VENDOR_IQ_CALLBACK,         ///< The callback function for USB Vendor IQ Request
	UVAC_CONFIG_MFK_REC2LIVEVIEW,           ///< Suspend sending stream to USB host.
	UVAC_CONFIG_MFK_LIVEVIEW2REC,           ///< Resume  sending stream to USB host.
	UVAC_CONFIG_UVAC_CAP_M3,                ///< Enable UVC capture method 3.
	UVAC_CONFIG_EU_VENDCMDCB_START,         ///< The callback function for USB Vendor Command of Extention Unit, idx0 no use
	UVAC_CONFIG_EU_VENDCMDCB_ID01,          ///< Extension unit callback with CS=1.
	UVAC_CONFIG_EU_VENDCMDCB_ID02,          ///< Extension unit callback with CS=2.
	UVAC_CONFIG_EU_VENDCMDCB_ID03,          ///< Extension unit callback with CS=3.
	UVAC_CONFIG_EU_VENDCMDCB_ID04,          ///< Extension unit callback with CS=4.
	UVAC_CONFIG_EU_VENDCMDCB_ID05,          ///< Extension unit callback with CS=5.
	UVAC_CONFIG_EU_VENDCMDCB_ID06,          ///< Extension unit callback with CS=6.
	UVAC_CONFIG_EU_VENDCMDCB_ID07,          ///< Extension unit callback with CS=7.
	UVAC_CONFIG_EU_VENDCMDCB_ID08,          ///< Extension unit callback with CS=8.
	UVAC_CONFIG_EU_VENDCMDCB_END,
	UVAC_CONFIG_WINUSB_ENABLE,              ///< Enable WINUSB.
	UVAC_CONFIG_WINUSB_CB,                  ///< The callback function for WINUSB
	UVAC_CONFIG_VIDEO_FORMAT_TYPE,          ///< Supported video format by UVC device.
	UVAC_CONFIG_CDC_ENABLE,                 ///< Enable CDC ADM. It'll emulate dual COM port at USB host.
	UVAC_CONFIG_CDC_PSTN_REQUEST_CB,        ///< The callback function for CDC PSTN subclass specific request, REFERING TO #UVAC_CDC_PSTN_REQUEST_CB.
	UVAC_CONFIG_VID_BUF_INFO,               ///< Set video stream buffer information, referring to UVAC_VID_BUF_INFO
	UVAC_CONFIG_XU_CTRL,                    ///< The callback function for UVC Extension Unit.
	UVAC_CONFIG_MAX_FRAME_SIZE,				///< Max video frame size, the default is 800KB.
    UVAC_CONFIG_AUD_CHANNEL_NUM,			///< Audio channel number.
    UVAC_CONFIG_HW_COPY_CB,					///< The callback function for HW memory copy, referring to UVAC_HWCOPYCB.
	UVAC_CONFIG_CT_CB,                      ///< UVC Camera Terminal capability.
    UVAC_CONFIG_PU_CB,                      ///< UVC Processing Unit capability.
    UVAC_CONFIG_YUV_FRM_INFO,               ///< Set YUV frame type info, referring to UVAC_VID_RESO_ARY.
    UVAC_CONFIG_DFU_INFO,                   ///< Set DFU function, referring to UVAC_DFU_INFO.
    UVAC_CONFIG_MAX_UVC_DEV_NUM,            ///< Max UVC device num.
    UVAC_CONFIG_DISABLE_UAC,                ///< Set TRUE to disable UAC.
    UVAC_CONFIG_EU_DESC,                    ///< Customized UVC EU descriptor, referring to UVAC_EU_DESC.
    UVAC_CONFIG_UVC_STRING,                 ///< Set string for UVC, referring to UVAC_STRING_DESC.
    UVAC_CONFIG_UAC_STRING,                 ///< Set string for UAC, referring to UVAC_STRING_DESC.
    UVAC_CONFIG_HID_INFO,                   ///< Set HID function, referring to UVAC_HID_INFO.
    UVAC_CONFIG_UAC_RX_ENABLE,              ///< UAC RX enable.
    UVAC_CONFIG_UAC_RX_BLK_SIZE,            ///< UAC RX block size.
    UVAC_CONFIG_UAC_RX_BUF_SIZE,            ///< UAC RX buffer size.
    UVAC_CONFIG_AUD_START_CB,               ///< UAC start callback.
    UVAC_CONFIG_VID_USER_DATA_SIZE,         ///< The size of user data prior to video stream which should contain SPS/PPS.
    UVAC_CONFIG_MJPG_DEF_FRM_IDX,           ///< Set bDefaultFrameIndex for MJPEG, referring to the SPEC of Video Format Descriptor.
    UVAC_CONFIG_H264_DEF_FRM_IDX,           ///< Set bDefaultFrameIndex for H264, referring to the SPEC of Video Format Descriptor.
    UVAC_CONFIG_MSDC_INFO,                  ///< Set MSDC function, referring to UVAC_MSDC_INFO.
    UVAC_CONFIG_EU_DESC_2ND,                ///< Customized 2nd UVC EU descriptor, referring to UVAC_EU_DESC.
	UVAC_CONFIG_UAC_PACKETSIZE,             ///< Set wMaxPacketSize field for UAC endpoint descriptor.
	UVAC_CONFIG_UAC_INTERVAL,               ///< Set bInterval field for UAC endpoint descriptor.
	UVAC_CONFIG_CT_CONTROLS,                ///< Set bmControls field for UVC Camera Terminal.
	UVAC_CONFIG_PU_CONTROLS,                ///< Set bmControls field for UVC Processing Unit.
	UVAC_CONFIG_UVC_VER,                    ///< UVC version, refering to UVAC_UVC_VER.
	UVAC_CONFIG_MSOS20_INFO,                ///< Set Microsoft OS 2.0 descriptor info, referring to UVAC_MSOS20_INFO.
	UVAC_CONFIG_UVC_MJPG_FRM_INFO,         ///< Set MJPG frame info for UVC, referring to UVAC_VID_RESO_ARY.
	UVAC_CONFIG_UVC_H264_FRM_INFO,         ///< Set H264 frame info for UVC, referring to UVAC_VID_RESO_ARY.
	UVAC_CONFIG_UVC2_MJPG_FRM_INFO,         ///< Set MJPG frame info for UVC2, referring to UVAC_VID_RESO_ARY.
	UVAC_CONFIG_UVC2_H264_FRM_INFO,         ///< Set H264 frame info for UVC2, referring to UVAC_VID_RESO_ARY.
	UVAC_CONFIG_AUD_CHANNEL_CONFIG,         ///< UAC channel config.
	UVAC_CONFIG_CDC2_ENABLE,
	ENUM_DUMMY4WORD(UVAC_CONFIG_ID)
} UVAC_CONFIG_ID;


/**
    Video device count.

    @note For UVAC_STARTVIDEOCB and UVAC_STOPVIDEOCB.
*/
typedef enum {
	UVAC_VID_DEV_CNT_1 = 0,
	UVAC_VID_DEV_CNT_2,
	UVAC_VID_DEV_CNT_MAX
} UVAC_VID_DEV_CNT;

typedef enum _UVAC_TYPE {
	UVAC_TYPE_U3 = 0,
	UVAC_TYPE_U2,
	ENUM_DUMMY4WORD(UVAC_TYPE)
} UVAC_TYPE;

typedef enum {
	UVAC_AUD_DEV_CNT_1 = 0,
	UVAC_AUD_DEV_CNT_2,
	UVAC_AUD_DEV_CNT_3,
	UVAC_AUD_DEV_CNT_MAX
} UVAC_AUD_DEV_CNT;

/**
    Callback function prototype for the fpStartVideoCB of UVAC_INFO.
*/
typedef UINT32(*UVAC_STARTVIDEOCB)(UVAC_VID_DEV_CNT vidDevIdx, UVAC_STRM_INFO *pStrmInfo);

/**
    Callback function prototype for the fpStopVideoCB of UVAC_INFO.
*/
typedef void (*UVAC_STOPVIDEOCB)(UVAC_VID_DEV_CNT vidDevIdx);

/**
    Callback function prototype for the fpSetVolCB of UVAC_INFO.
*/
typedef UINT32(*UVAC_SETVOLCB)(UINT32 volume);  ///< Callback for starting video.

/**
    Callback function prototype for the UAC callback.
*/
typedef UINT32(*UVAC_STARTAUDIOCB)(UVAC_AUD_DEV_CNT aud_dev_idx, UVAC_STRM_INFO *strm_info);

/**
    Information needed for UVAC module.

    This structure is used in UVAC_Open() to specify the UVAC task needed information for normal working
    and user customization callback.
*/
typedef struct _UVAC_INFO_ {
	UINT32              UvacMemAdr;                      ///< Buffer address for USB operation
	UINT32              UvacMemSize;                     ///< Buffer size for USB operation
	UVAC_CHANNEL        channel;                         ///< The number of UVAC channel, refering to UVAC_CHANNEL.
	UVAC_STARTVIDEOCB   fpStartVideoCB;                  ///< Callback function to start video
	UVAC_STOPVIDEOCB    fpStopVideoCB;                   ///< Callback function to stop video buffer
	UVAC_SETVOLCB		fpSetVolCB;						 ///< Callback function to audio record volume
	UINT32              hwPayload[UVAC_VID_DEV_CNT_MAX]; ///< Obsolete
	UVAC_STRM_INFO      strmInfo;                        ///< Obsolete
	UINT16              uiStrgCardLock;                  ///< Obsolete
} UVAC_INFO;

/**
    Callback functions prototype that UVAC task used.

    These definitions are used in the structure UVAC_INFO and would be used during UVAC_Open() to specify the
    UVAC_Task needed information.
*/
//@{
typedef UINT8(*UVAC_EUVENDCMDCB)(UINT32 ControlCode, UINT8 CS, UINT8 *pData, UINT32 *pDataLen);
//@}


/**
    Close UVAC module.
*/
extern void UVAC_Close(void);

/**
    Get needed buffer size for UVAC module.

    Double the size for UVAC_CHANNEL_2V2A or UVAC_CHANNEL_2V1A.
*/
extern UINT32 UVAC_GetNeedMemSize(void);

/**
    Set UVAC configuration

    Assign new configuration of the specified ConfigID.

    @param[in] ConfigID         Configuration identifier
    @param[in] Value            Configuration context for ConfigID
*/
extern void UVAC_SetConfig(UVAC_CONFIG_ID ConfigID, UINT32 Value);

/**
    Set the stream of a frame information to UVAC lib.
*/
extern void UVAC_SetEachStrmInfo(PUVAC_STRM_FRM pStrmFrm);

/**
    Set video resolution.
*/
extern ER UVAC_ConfigVidReso(PUVAC_VID_RESO pVidReso, UINT32 cnt);

/**
    Open UVAC(USB VIDEO/AUDIO-Device-Class) module.

    @param[in] pClassInfo Information needed for opening UVAC Task. The user must prepare all the information needed.

    @return
     - @b E_OK:  The UVAC Task open done and success.
     - @b E_SYS: Buffer allocation failed.
     - @b E_PAR: Parameter error.
*/
extern UINT32   UVAC_Open(UVAC_INFO *pClassInfo);


/**
    Read data from USB host via CDC class.
    This API doesn't return until there is any data coming from USB host or CDC_AbortRead() is invoked.

    @param[in] ComID   COM ID.
    @param[in] pBuf   Buffer pointer.
    @param[in,out] pBufSize  Input length to read, output actual transfered length.

    @return
        - @b E_OK:  Success.
        - @b E_SYS: Failed
        - @b E_RSATR: UVAC_AbortCdcRead() is invoked by another task.
*/
extern INT32 UVAC_ReadCdcData(CDC_COM_ID ComID, void *p_buf, UINT32 buffer_size, INT32 timeout);
/**
    Abort the function of UVAC_ReadCdcData().

    @param[in] ComID   COM ID.
*/
extern void UVAC_AbortCdcRead(CDC_COM_ID ComID);

/**
    Write data to USB host via CDC class.

    This function is a blocking API and will return after data is sent to USB host.

    @param[in] ComID   COM ID.
    @param[in] pBuf  Buffer pointer
    @param[in,out] pBufSize  Input length to read, output actual transfered length. Valid length is 0x1 to 0x7fffff.

    @return
        - @b E_OK: Start to transfer.
        - @b E_SYS: Failed.
*/
extern INT32 UVAC_WriteCdcData(CDC_COM_ID ComID, void *p_buf, UINT32 buffer_size, INT32 timeout);

/**
    Write data to USB host via HID class.

    @param[in] p_buf  Buffer pointer
    @param[in] buffer_size  Input length to write. Valid length is 0x1 to 0x7fffff.
    @param[in] timeout  wait ms, -1 for blocking, 0 for non-blocking

    @return
        - @b INT32: positive value means acturally written size, negative value means USB engine busy or error code.
*/
extern INT32 UVAC_WriteHidData(void *p_buf, UINT32 buffer_size, INT32 timeout);

/**
    Read data from USB host via HID class.

    @param[in] p_buf  Buffer pointer
    @param[in] buffer_size  Input length to write. Valid length is 0x1 to 0x7fffff.
    @param[in] timeout  wait ms, -1 for blocking, 0 for non-blocking

    @return
        - @b INT32: positive value means acturally read size, negative value means USB engine busy or error code.
*/

extern INT32 UVAC_ReadHidData(void *p_buf, UINT32 buffer_size, INT32 timeout);

/**
    Pull stream from UVAC.

    @return
        - @b E_OK: Success.
        - @b E_SYS: Failed.
*/
extern ER UVAC_PullOutStrm(PUVAC_STRM_FRM pStrmFrm, INT32 wait_ms);

/**
    Release pulled stream.

    @return
        - @b E_OK: Success.
        - @b E_SYS: Failed.
*/
extern ER UVAC_ReleaseOutStrm(PUVAC_STRM_FRM pStrmFrm);

/**
    Wait for the stream frame is ready for releasing.

    @return
        - @b E_OK: Success.
        - @b E_SYS: Failed.
*/
extern ER UVAC_WaitStrmDone(UVAC_STRM_PATH path);

//@}

#endif  // _UVAC_H

