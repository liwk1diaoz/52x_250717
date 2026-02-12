/////////////////////////////////////////////////////////////////
/*
    Copyright (c) 2004~  Novatek Microelectronics Corporation

    @file UvacVideoTsk.h the pc-cam device firmware

    @hardware: Novatek 96610

    @version

    @date

*//////////////////////////////////////////////////////////////////
#ifndef _U2UVACVIDEOTSK_H
#define _U2UVACVIDEOTSK_H
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/util.h"
#include "kwrap/perf.h"
#include <kwrap/cpu.h>
#include <kwrap/verinfo.h>
#include "UVAC.h"
#include "../uvac_compat.h"
#include "U2UvacIsoInTsk.h"
#include <stdio.h>
#include <string.h>

#define UVAC_MEM_STORE_PERIOD       6         //second
#define UVAC_H264_TBR_MIN           0x00180000
#define UVAC_H264_TBR_DEF           0x00200000
#define UVAC_H264_TBR_MAX           0x00300000
#define UVAC_MJPG_TBR_MIN           0x00180000
#define UVAC_MJPG_TBR_DEF           0x00200000
#define UVAC_MJPG_TBR_MAX           0x00300000
#define UVAC_MEM_DESC_SIZE          0x00001000    //4k
#define UVAC_MAX_PAYLOAD_FRAME_SIZE (800*1024)    //800k
#define UVAC_AUD_RX_BUF_SIZE        0x00014000
#define UVAC_AUD_RX_BLK_SIZE        0x00001000

#define UVAC_VID_INFO_QUE_MAX_CNT   0x40
#define UVAC_VID_INFO_QUE_MAX_IDX  (UVAC_VID_INFO_QUE_MAX_CNT - 1)
#define UVAC_VID_I_FRM_MARK         0x65
#define UVAC_VID_I_FRM_MARK_POS     0x04

#define UVAC_VID_RESO_INTERNAL_TBL_CNT      5

#define UVC_FRMRATE_5               5
#define UVC_FRMRATE_8               8
#define UVC_FRMRATE_10              10
#define UVC_FRMRATE_12              12
#define UVC_FRMRATE_15              15
#define UVC_FRMRATE_20              20
#define UVC_FRMRATE_25              25
#define UVC_FRMRATE_30              30
#define UVC_FRMRATE_60              60

#define  UVAC_OUT_SIZE_1280_720     UVC_VSFRM_INDEX1
#define  UVAC_OUT_SIZE_1024_768     UVC_VSFRM_INDEX2
#define  UVAC_OUT_SIZE_640_480      UVC_VSFRM_INDEX3
#define  UVAC_OUT_SIZE_320_240      UVC_VSFRM_INDEX4
#define  UVAC_OUT_SIZE_1024_576     UVC_VSFRM_INDEX5
#define  UVAC_OUT_SIZE_800_600      UVC_VSFRM_INDEX6
#define  UVAC_OUT_SIZE_800_480      UVC_VSFRM_INDEX7
#define  UVAC_OUT_SIZE_1920_1080    UVC_VSFRM_INDEX8
#define  UVAC_OUT_SIZE_160_120      UVC_VSFRM_INDEX9       //not in configuration descriptor
#define  UVAC_OUT_SIZE_128_96       UVC_VSFRM_INDEX10      //not in configuration descriptor
#define  UVAC_OUT_SIZE_352_288      UVC_VSFRM_INDEX11

#define FLGUVAC_START              FLGPTN_BIT(0)  //0x00000001
#define FLGUVAC_STOP               FLGPTN_BIT(1)  //0x00000002
///#define FLGUVAC_EP1TX0BYTE         FLGPTN_BIT(2)  //0x00000004
#define FLGUVAC_OUT0               FLGPTN_BIT(3)  //0x00000008
//#define FLGUVAC_IN1                FLGPTN_BIT(7)  //0x00000080
#define FLGUVAC_RDY                FLGPTN_BIT(8)  //0x00000100
#define FLGUVAC_ISOIN_TXF          FLGPTN_BIT(12) //0x00001000
#define FLGUVAC_ISOIN_STOP         FLGPTN_BIT(13) //0x00002000
#define FLGUVAC_ISOIN_START        FLGPTN_BIT(14) //0x00004000
#define FLGUVAC_START2             FLGPTN_BIT(15) //0x00008000
#define FLGUVAC_STOP2              FLGPTN_BIT(9)  //0x00000200
#define FLGUVAC_RDY2               FLGPTN_BIT(10) //0x00000400
//#define FLGUVAC_ISOIN_READY        FLGPTN_BIT(11) //0x00000800
#define FLGUVAC_HID_DATA_OUT       FLGPTN_BIT(11) //0x00010000
#define FLGUVAC_CDC_DATA_OUT       FLGPTN_BIT(16) //0x00010000
#define FLGUVAC_CDC_ABORT_READ     FLGPTN_BIT(17) //0x00020000
#define FLGUVAC_CDC_DATA2_OUT      FLGPTN_BIT(18) //0x00040000
#define FLGUVAC_CDC_ABORT_READ2    FLGPTN_BIT(19) //0x00080000

#define FLGUVAC_ISOIN_VDO_TXF      FLGPTN_BIT(20)
#define FLGUVAC_ISOIN_VDO_READY    FLGPTN_BIT(21)
#define FLGUVAC_ISOIN_AUD_TXF      FLGPTN_BIT(22)
#define FLGUVAC_ISOIN_AUD_READY    FLGPTN_BIT(23)
#define FLGUVAC_ISOIN_VDO2_TXF     FLGPTN_BIT(24)
#define FLGUVAC_ISOIN_VDO2_READY   FLGPTN_BIT(25)
#define FLGUVAC_ISOIN_AUD2_TXF     FLGPTN_BIT(26)
#define FLGUVAC_ISOIN_AUD2_READY   FLGPTN_BIT(27)

#define FLGUVAC_VIDEO_TXF          FLGPTN_BIT(28)
#define FLGUVAC_VIDEO_STOP         FLGPTN_BIT(29)
#define FLGUVAC_VIDEO2_TXF         FLGPTN_BIT(30)
#define FLGUVAC_VIDEO2_STOP        FLGPTN_BIT(31)

#define FLGUVAC_AUD_DATA_OUT        FLGPTN_BIT(0)  //0x00000001
#define FLGUVAC_AUD_EXIT            FLGPTN_BIT(1)  //0x00000002
#define FLGUVAC_AUD_IDLE            FLGPTN_BIT(2)  //0x00000004

#define FLGUVAC_AUD1_CHK_BUSY      FLGPTN_BIT(0)  //0x00000001
#define FLGUVAC_AUD2_CHK_BUSY      FLGPTN_BIT(1)  //0x00000002

#define FLGUVAC_FRM_V1             FLGPTN_BIT(0)
#define FLGUVAC_FRM_V2             FLGPTN_BIT(1)
#define FLGUVAC_FRM_A1             FLGPTN_BIT(2)
#define FLGUVAC_FRM_A2             FLGPTN_BIT(3)

#define UVAC_EU_VENDCMD_CNT     (UVAC_CONFIG_EU_VENDCMDCB_END - UVAC_CONFIG_EU_VENDCMDCB_START)

#define UVAC_AUD_RAWQ_MAX       (80)

#define wai_sem vos_sem_wait
#define sig_sem vos_sem_sig

typedef _PACKED_BEGIN struct {
	UINT32   uiBaudRateBPS; ///< Data terminal rate, in bits per second.
	UINT8    uiCharFormat;  ///< Stop bits.
	UINT8    uiParityType;  ///< Parity.
	UINT8    uiDataBits;    ///< Data bits (5, 6, 7, 8 or 16).
} _PACKED_END CDCLineCoding;

typedef enum _CDC_PSTN_REQUEST {
	REQ_SET_LINE_CODING         =    0x20,
	REQ_GET_LINE_CODING         =    0x21,
	REQ_SET_CONTROL_LINE_STATE  =    0x22,
	REQ_SEND_BREAK              =    0x23,
	ENUM_DUMMY4WORD(CDC_PSTN_REQUEST)
} CDC_PSTN_REQUEST;

typedef enum _UVAC_ACCESS_ {
	UVAC_ACCESS_GET,
	UVAC_ACCESS_SET,
} UVAC_ACCESS;

typedef struct _UVAC_PARAM {
	UINT32  OutSizeID;
	UINT32  FrameRate;
} UVAC_PARAM, *PUVAC_PARAM;

typedef struct _UVAC_VID_STRM_INFO {
	UINT32  addr[UVAC_VID_INFO_QUE_MAX_CNT];
	UINT32  size[UVAC_VID_INFO_QUE_MAX_CNT];
	UINT32  idxConsumer;
	UINT32  idxProducer;
	UVAC_STRM_INFO strmInfo;
	UINT64  timestamp;
} UVAC_VID_STRM_INFO, *PUVAC_VID_STRM_INFO;

typedef struct _UVC_PAYL_MAKE_COMP {
	UINT8 *pVidSrc;
	UINT8 *pDst;
	UINT32 vidSize;
	UINT32 toggleFid;
	UINT8 *pHdrAddr;
	UINT32 hdrSize;
	BOOL   bIsH264;
} UVC_PAYL_MAKE_COMP, *PUVC_PAYL_MAKE_COMP;

/**
    UVAC H264 Target Byte Rate Configuration

    This definition is used in UVAC_SetConfig() to set the value of UVAC_CONFIG_H264_TARGET_SIZE.
*/
typedef enum _UVAC_H264_TBR_ {
	UVAC_H264_TBR_1M,                      ///< Set H.264 TBR to be 1M bytes.
	UVAC_H264_TBR_1_5_M,                   ///< Set H.264 TBR to be 1.5M bytes.
	UVAC_H264_TBR_2M,                      ///< Set H.264 TBR to be 2M bytes.
	ENUM_DUMMY4WORD(UVAC_H264_TBR)
} UVAC_H264_TBR;

typedef struct {
	UINT32                      Front;              ///< Front pointer
	UINT32                      Rear;               ///< Rear pointer
	UINT32                      last_addr;          ///< Front pointer
	UINT32                      lock_addr;          ///< Front pointer
	MEM_RANGE                   queue[UVAC_AUD_RAWQ_MAX];
} UVAC_AUD_RAWQ, *PUVAC_AUD_RAWQ;


extern USB_EP_BLKNUM gU2UvcIsoInBandWidth;
extern UINT32 gU2UvcIsoInHsPacketSize;


/*-----------------------------------
    functions declare
-----------------------------------*/
void U2UVAC_SetCodec(UVAC_VIDEO_FORMAT CodecType, UINT32 vidIdx);
int U2UVAC_SetFrameRate(UINT16 uhFrameRate, UINT32 vidDevIdx);
int U2UVAC_SetImageSize(UINT16 uhSize, UINT32 vidDevIdx);
extern THREAD_DECLARE(U2UVAC_VideoTsk, arglist);
extern THREAD_DECLARE(U2UVAC_VideoTsk2, arglist);
extern THREAD_DECLARE(U2UVAC_AudioRxTsk, arglist);
void U2UVC_MakePayloadFmt(UVAC_VID_DEV_CNT  thisVidIdx, UVAC_TXF_INFO *pTxfInfo);

extern void U2UVAC_Close(void);
extern UINT32 U2UVAC_GetNeedMemSize(void);
extern void U2UVAC_SetConfig(UVAC_CONFIG_ID ConfigID, UINT32 Value);
extern void U2UVAC_SetEachStrmInfo(PUVAC_STRM_FRM pStrmFrm);
extern void U2UVC_SetTestImg(UINT32 imgAddr, UINT32 imgSize);
extern ER U2UVAC_ConfigVidReso(PUVAC_VID_RESO pVidReso, UINT32 cnt);
extern UINT32 U2UVAC_Open(UVAC_INFO *pClassInfo);
extern INT32 U2UVAC_ReadCdcData(CDC_COM_ID ComID, void *p_buf, UINT32 buffer_size, INT32 timeout);
extern void U2UVAC_AbortCdcRead(CDC_COM_ID ComID);
extern INT32 U2UVAC_WriteCdcData(CDC_COM_ID ComID, void *p_buf, UINT32 buffer_size, INT32 timeout);
extern UINT32 U2UVAC_GetCdcDataCount(CDC_COM_ID ComID);

extern ER U2UVAC_PullOutStrm(PUVAC_STRM_FRM pStrmFrm, INT32 wait_ms);
extern ER U2UVAC_ReleaseOutStrm(PUVAC_STRM_FRM pStrmFrm);

#endif  // _UVACVIDEOTSK_H

