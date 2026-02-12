/*
    Copyright   Novatek Microelectronics Corp. 2004.  All rights reserved.

    @file       jpgenc.c
    @ingroup    mIAVJPEG

    @brief      Jpeg encoder

*/
#if 0
#include <stdio.h>
#include <string.h>

#include "SysKer.h"
#include "jpeg.h"
#include "JpgEnc.h"
#include "pll.h"
#include "rtc.h"
//#include "ExifHeader.h"
//#include "VendorEXIF.h"
#include "JpgHeader.h"
#include "Utility.h"
#include "BinaryFormat.h"
#include "jpgint.h"
#include "jpeg_lmt.h"
#endif
#include "jpg_int.h"
#include "../../include/jpg_enc.h"
#include "../brc/jpeg_brc.h"
#include "../../include/jpeg.h"
#include "../jpeg_platform.h"

#if defined __FREERTOS
#include "top.h"
#else
#include <plat/top.h>
#endif

#define JPEGENC_DEBUG   0
#define PERF_TEST   0
#define Q_CHECK         1
//#NT#2011/07/05#Ben Wang -begin
//#NT#Add ring buffer mechanism for JPEG bitstream

/** \addtogroup mIAVJPEG
*/
//@{

#if 0//dedicated design for NT96220
#define RING_BUFFER_FLOW    1
#endif

#define hi_byte(data)            UINT16_HIBYTE(data)     ///< Refer to UINT16_HiByte().
#define lo_byte(data)            UINT16_LOBYTE(data)     ///< Refer to UINT16_LoByte().
#define MAKE_ALIGN(s, bytes)    ((((UINT32)(s))+((bytes)-1)) & ~((UINT32)((bytes)-1)))
#define ROUND_ALIGN(s, bytes)    (((UINT32)(s)) & ~((UINT32)((bytes)-1)))

#define  DEFAULT_BRC_STD_QTABLE_QUALITY   50//grucStdLuxQ[] is equal to Quality=50
//#NT#2011/08/09#Ben Wang -begin
//#NT#Add adaptive BRC mechanism
#define ADAPTIVE_STD_Q_TABLE_FLOW    1
#define ADAPTIVE_ADJUST_VALUE        10
#define ADAPTIVE_MAX_STD_Q_QUALITY   DEFAULT_BRC_STD_QTABLE_QUALITY//50
#define ADAPTIVE_MIN_STD_Q_QUALITY   5
//#NT#2011/08/09#Ben Wang -end
#define BRC2_MIN_Q_QUALITY   5
#define BRC2_MAX_Q_QUALITY   99
#define BRC2_STEP_Q_QUALITY   2

//#define  EXIF_HEAD_SIZE   sizeof(JPGAPP1Header) + MakeNoteLen_UnInit + MakeNoteLen_DEBUGBUFSIZE
//#NT#2011/06/17#Ben Wang -begin
//#NT#refine UserJpeg API
#define  DEFAULT_TARGET_FRAME_SIZE  (100*1024)
#define  FRAME_SIZE_MAX_RATE        120//% it's an experimental value, needed to fine tune by case
//#NT#2011/08/15#Ben Wang -begin
//#NT#Refine API for the boundary of target size
#define  DEFAULT_BRC_UPPER_BOUND    105//%
#define  DEFAULT_BRC_LOWER_BOUND    95//%
//#NT#2011/08/15#Ben Wang -end
//#NT#2011/07/06#Ben Wang -begin
//#NT#modified Q value for testing
#define  CONSTANT_Q_VALUE   1000
//#NT#2011/07/06#Ben Wang -end
//#define  DEFAULT_BRC_QF  500//128
#define  DEFAULT_MAX_BRC_RETRY_CNT    1
//#NT#2011/08/12#Ben Wang -begin
//#NT#Make weighting mechanism valid only when QF is decaying.
#define  BRC_WEIGHTING_VALID_IN_DECAY_ONLY  1
//#NT#2011/08/12#Ben Wang -end

#define  JPEG_MIN_BUF_LENGTH        64
//#NT#2011/06/17#Ben Wang -end
//#NT#2011/08/09#Ben Wang -begin
//#NT#Add max valid frame size limitation
#define  DEFAULT_MAX_VALID_FRAME_SIZE_RATIO         200
#define  MAX_VALID_FRAME_SIZE_RATIO_UPPER_LIMIT     200
#define  MAX_VALID_FRAME_SIZE_RATIO_LOWER_LIMIT     100

//#NT#2012/05/08#Ben Wang -begin
//#NT#Patch for JPEG HW limitation
#define JPG_MIN_BSLENGTH        (256)
#define JPG_MAX_BSLENGTH        (32*1024*1024-1)
#define BS_START_ADDR_ALIGNMENT  0x4
//#NT#2012/05/08#Ben Wang -end

//BRC control
//#define DEFAULT_BRC_QF   500

//#NT#2011/08/09#Ben Wang -end
//#NT#2010/08/27#Ben Wang -begin
//#Place Hidden image after primary image
typedef struct {
	UINT32    header;
	UINT32    offset;
	UINT32    size;
} HIDDEN_IMAGE_INFO;
//#NT#2010/08/27#Ben Wang -end

//#NT#2011/07/18#HH Chuang -begin
typedef enum {
	JPG_BRC_STATE_DIS,          //< disabled
	JPG_BRC_STATE_EN,           //< enabled
	JPG_BRC_STATE_EST_PLANAR,   //< state to estimate standalone planar
	JPG_BRC_STATE_EST_FULL,     //< state to estimate full (Y/U/V) image

	ENUM_DUMMY4WORD(JPG_BRC_STATE)
} JPG_BRC_STATE;
//#NT#2011/07/18#HH Chuang -end

//For GPS Info. Reserved for Future.

//add MovMjpg
UINT32 Jpg_Swap(UINT32 value);

//#Add for checking system time if it is set..
BOOL    g_bIsSysTimeSet = TRUE;

UINT32    g_uiQuality = 0;
UINT8   *g_pQTable;

JPG_YUV_FORMAT gYUVFormat   = JPG_FMT_YUV422;
//#NT#2011/07/18#HH Chuang -begin
//static BOOL g_bJpegBrcEn = FALSE;
UINT32 g_uiBrcQF[KDRV_VDOENC_ID_MAX+1] = {[0 ... KDRV_VDOENC_ID_MAX]=DEFAULT_BRC_QF};
static UINT32 g_uiBrcMaxReTryCnt = DEFAULT_MAX_BRC_RETRY_CNT;
//static UINT32 g_uiBrcBsLen = 0;
static JPG_BRC_STATE g_brcState = JPG_BRC_STATE_DIS;
static UINT32 g_uiBrcYuvMsk = 0;        // record Y/U/V processed record when g_brcState = JPG_BRC_STATE_EST_PLANAR
static UINT8   ucBrcStdQTableY[DQT_SIZE], ucBrcStdQTableUV[DQT_SIZE];
UINT32  g_uiBrcStdQTableQuality[KDRV_VDOENC_ID_MAX+1] = {[0 ... KDRV_VDOENC_ID_MAX]=DEFAULT_BRC_STD_QTABLE_QUALITY};
UINT32  g_uiMinQTableQuality[KDRV_VDOENC_ID_MAX+1] = {0};
UINT32  g_uiMaxQTableQuality[KDRV_VDOENC_ID_MAX+1] = {0};
JPGBRC_PARAM g_brcParam[KDRV_VDOENC_ID_MAX+1];
static BOOL  m_IsASyncEncOpened = FALSE;
static MEM_RANGE g_header = {0};
//static UINT32 *m_puiJPGBSBufSize;
//static ER JpegBrcGenQtable(JPG_BRC_CHANNEL yuvChannel);
//#NT#2011/07/18#HH Chuang -end

static UINT8            QTableY[64];
static UINT8            QTableUV[64];

//for kdrv
KDRV_JPEG_INFO  kdrv_info;
JPGHEAD_DEC_CFG      jpegdec_Cfg;

//for BRC contrl
UINT32 g_uiBrcTargetSize[KDRV_VDOENC_ID_MAX+1] = {0};     // target frame size
UINT32 g_uiBrcCurrTarget_Rate[KDRV_VDOENC_ID_MAX+1] = {0};     // Current target size

//for VBR mode control
UINT32 g_uiVBR[KDRV_VDOENC_ID_MAX+1] = {[0 ... KDRV_VDOENC_ID_MAX]=VBR_NORAMAL};
UINT32 g_uiVBRmodechange[KDRV_VDOENC_ID_MAX+1] = {0};
UINT32 g_uiVBR_QUALITY[KDRV_VDOENC_ID_MAX+1] = {[0 ... KDRV_VDOENC_ID_MAX]=DEFAULT_BRC_STD_QTABLE_QUALITY};

extern UINT32 g_uiVBR_Q[KDRV_VDOENC_ID_MAX+1];
extern UINT32 g_uiDC_Qavg[KDRV_VDOENC_ID_MAX+1];
extern UINT32 g_TotalFrameID[KDRV_VDOENC_ID_MAX+1];
extern UINT32 g_reEnc[KDRV_VDOENC_ID_MAX+1];

UINT32 g_jpeg_targetrate[KDRV_VDOENC_ID_MAX+1] = {0};
UINT32 g_jpeg_quality[KDRV_VDOENC_ID_MAX+1] = {0};

static void JpegTransStdQTable(UINT32 quality, UINT8 *p_q_tbl_y, UINT8 *p_q_tbl_uv)
{
	UINT32  uiQuantFactor, i;
	UINT32  uiCoff_Y, uiCoff_UV = 0;
#if JPEGENC_DEBUG
	DBG_DUMP("^BJpegTransStdQTable>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>=%3d\r\n",quality);
#endif
	if (quality == 0) {
		memcpy(p_q_tbl_y, (UINT8 *)(JPGHeader.mark_dqt.q_tbl_0th.q_data), DQT_SIZE);
		memcpy(p_q_tbl_uv, (UINT8 *)(JPGHeader.mark_dqt.q_tbl_1th.q_data), DQT_SIZE);
	} else {
		if (quality >= 100) {
			memset((UINT8 *) p_q_tbl_y, 1, DQT_SIZE);   //also copy new q table to JPG header.
			memset((UINT8 *) p_q_tbl_uv, 1, DQT_SIZE);
		} else {
			if (quality < 50) {
				uiQuantFactor = (5000 / quality);
			} else {
				uiQuantFactor = 200 - (quality << 1);
			}
			//Reason: less caculate for UV Q table.

			for (i = 0; i < DQT_SIZE; i++) {
				uiCoff_Y = (grucStdLuxQ[i] * uiQuantFactor + 50) / 100;

				if (uiCoff_Y < 1) {
					uiCoff_Y = 1;
				}

				if (uiCoff_Y > 255) {
					uiCoff_Y = 255;
				}
				if (i < 16) {
					uiCoff_UV = ((grucStdChrQ[i] * uiQuantFactor) + 50) / 100;

					if (uiCoff_UV < 1) {
						uiCoff_UV = 1;
					}

					if (uiCoff_UV > 255) {
						uiCoff_UV = 255;
					}
				} else {
					//Since grucStdChrQ[i] is the same value after i>16, we don't caculate the value again.
				}
				*p_q_tbl_y++  = (UINT8) uiCoff_Y;    //also copy new q table to JPG header.
				*p_q_tbl_uv++ = (UINT8) uiCoff_UV;
				//DBG_DUMP("^BY=%3d, UV=%3d\r\n",uiCoff_Y, uiCoff_UV);
			}
		}
	}
}

UINT32 Jpg_Swap(UINT32 value)
{
	UINT32 returnV = 0;

	returnV = ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24);
	return returnV;
}

ER jpg_enc_one_img(PJPG_ENC_CFG p_jpg_param, UINT32 quality)
{
	ER E_Ret = E_OK;
	UINT32 JPGFileSize = 0;
	UINT32 HeaderSize, BufSize;
	UINT8  QTableY[DQT_SIZE], QTableUV[DQT_SIZE];
	UINT32  uiBrcReTryCnt = 0;

	BufSize = *p_jpg_param->p_bs_buf_size;
	jpg_async_enc_open();
BRC_ENCODE:
	if (p_jpg_param->brc_en == TRUE) {
		jpg_async_set_brc_qtable(g_uiBrcQF[0], QTableY, QTableUV);
		if (BufSize < g_brcParam[0].ubound_byte) {
			DBG_ERR("p_bs_buf_size should NOT be smaller than UBoundByte in BRC mode!\r\n");
			jpg_async_enc_close();
			return E_PAR;
		}
	} else {
		jpg_async_set_qtable(quality, QTableY, QTableUV);
	}
	HeaderSize = jpg_async_enc_setheader(p_jpg_param, QTableY, QTableUV);
	if (HeaderSize > 0 && BufSize > HeaderSize) {
		E_Ret = jpg_async_enc_start(p_jpg_param->bs_addr + HeaderSize, BufSize - HeaderSize, p_jpg_param->image_addr[0], p_jpg_param->image_addr[1]);
		if (E_OK == E_Ret) {
			E_Ret = jpg_async_enc_waitdone(&JPGFileSize);
		}
		*p_jpg_param->p_bs_buf_size = JPGFileSize;
	}
	if (p_jpg_param->brc_en == TRUE) {
		UINT32  uiBrcBsLen;

		uiBrcBsLen = jpeg_get_bssize();
		DBG_MSG("BrcQF = %3d, BSLength = %d\r\n", (int)g_uiBrcQF[0], (int)uiBrcBsLen);
		if ((g_uiBrcQF[0] == JPGBRC_QUALITY_MIN && uiBrcBsLen < g_brcParam[0].lbound_byte) ||
			(g_uiBrcQF[0] == JPGBRC_QUALITY_MAX && uiBrcBsLen > g_brcParam[0].ubound_byte)) {
			DBG_ERR("Reach BRC limitation!\r\n");
			if (E_OK == E_Ret) {
				E_Ret = E_OBJ;
			}
		} else if (uiBrcBsLen > g_brcParam[0].ubound_byte || uiBrcBsLen < g_brcParam[0].lbound_byte) {
			if (uiBrcReTryCnt < g_uiBrcMaxReTryCnt) {
				uiBrcReTryCnt++;
				if (p_jpg_param->yuv_fmt == JPG_FMT_YUV420) {
					jpg_get_brc_qf(0,JPGBRC_COMBINED_YUV420, &g_uiBrcQF[0], uiBrcBsLen);
					goto BRC_ENCODE;
				} else if (p_jpg_param->yuv_fmt == JPG_FMT_YUV422) {
					jpg_get_brc_qf(0,JPGBRC_COMBINED_YUV422, &g_uiBrcQF[0], uiBrcBsLen);
					goto BRC_ENCODE;
				} else {
					DBG_ERR(" Only YUV420/422 support BRC\r\n");
				}
			} else {
				DBG_ERR(" Reach BRC re-try count limitation(%d)!\r\n", (int)(g_uiBrcMaxReTryCnt));
				if (E_OK == E_Ret) {
					E_Ret = E_OBJ;
				}
			}
		}
	}
	jpg_async_enc_close();
	return E_Ret;
}

/**
    Set Q-Table for encode.

    @param quality desired quality value (0~99)
    @param QTable Q-table being calculated per quality value
*/
//#NT#2010/06/03#Scottie -begin
//#NT#Support "Bit Rate Control" mechanism for Playback re-encoding
/**
    Set Q-Table for encode.

    @param quality desired quality value (0~99)
    @param QTable Q-table being calculated per quality value
*/
void jpg_async_set_qtable(UINT32 quality, UINT8 *p_q_tbl_y, UINT8 *p_q_tbl_uv)
{
	JpegTransStdQTable(quality, p_q_tbl_y, p_q_tbl_uv);
	jpeg_set_hwqtable(p_q_tbl_y, p_q_tbl_uv);
}
//#NT#2010/06/03#Scottie -end

//#NT#2010/07/26#Daniel -begin
//#Add for JPEG HW Bit Rate Control
void jpg_async_set_brc_qtable(UINT32 qf, UINT8 *p_q_tbl_y, UINT8 *p_q_tbl_uv)
{
#if JPEG_BRC2

#else
#if JPEGENC_DEBUG
	DBG_ERR("<<<<<<<<<<<<<<< jpg_async_set_brc_qtable>>>>>>>>>>>>>>>>>>>>>>>>>> %d\r\n",(int)qf);
#endif
	//jpgbrc_gen_qtable_y(qf, grucStdLuxQ, p_q_tbl_y);
	//JpgBrc_GenQUVtable(qf, grucStdChrQ, p_q_tbl_uv);
	jpgbrc_gen_qtable_y(qf, ucBrcStdQTableY, p_q_tbl_y);
	jpgbrc_gen_qtable_uv(qf, ucBrcStdQTableUV, p_q_tbl_uv);
#if JPEGENC_DEBUG
	DBG_ERR("===========================================QQQQQQQQQQQQQQQQQQ =%d\r\n",(int)(*p_q_tbl_y));
#endif
	//Set Q table
	jpeg_set_hwqtable(p_q_tbl_y, p_q_tbl_uv);
#endif
}
#if 1


ER jpg_set_bitrate_ctrl(PJPG_BRC_CFG p_brc_cfg)
{
	if (p_brc_cfg->initial_qf > 512 || p_brc_cfg->initial_qf < 1) {
		DBG_ERR("Initial QF should be in the range of 1~512.\r\n");
		return E_PAR;
	} else if (g_brcParam[0].lbound_byte > g_brcParam[0].target_byte || g_brcParam[0].ubound_byte < g_brcParam[0].target_byte) {
		DBG_ERR("Target size setting error!\r\n");
		return E_PAR;
	}

	//g_bJpegBrcEn = p_brc_cfg->bBrcEn;
	g_brcState = JPG_BRC_STATE_EN;
	g_uiBrcYuvMsk = 0;
	g_uiBrcQF[0] = p_brc_cfg->initial_qf;
	g_brcParam[0].width = p_brc_cfg->width;
	g_brcParam[0].height = p_brc_cfg->height;
	g_brcParam[0].target_byte = p_brc_cfg->target_size;
	g_brcParam[0].lbound_byte = p_brc_cfg->lowbound_size;
	g_brcParam[0].ubound_byte = p_brc_cfg->upbound_size;
	g_uiBrcMaxReTryCnt = p_brc_cfg->max_retry;
	//g_uiBrcBsLen = 0;
	DBG_IND("WxH=%dx%d, TBR = %d(boundary%d~%d)\r\n", (int)(g_brcParam[0].width), (int)(g_brcParam[0].height), (int)(g_brcParam[0].target_byte), (int)(g_brcParam[0].lbound_byte), (int)(g_brcParam[0].ubound_byte));
	//init standard Q table
	//g_uiBrcStdQTableQuality = DEFAULT_BRC_STD_QTABLE_QUALITY;
	DBG_ERR("Checking below api, using channel 0 first\r\n");
	JpegTransStdQTable(g_uiBrcStdQTableQuality[0], ucBrcStdQTableY, ucBrcStdQTableUV);
	return E_OK;
}
//#NT#2011/08/15#Ben Wang -end
//static UINT8   ucQMatrix[64], ucQUVMatrix[64];


#if 0
/*
    JPEG translate channel format

    JPEG translate channel format

    @param[in] yuvChannel    JPG_CHANNEL (in user jpeg layer)

    @return JPEG h/w format
*/
static JPG_BRC_CHANNEL JpegTransChannelFormat(JPG_CHANNEL yuvChannel)
{
	if (yuvChannel == JPG_CHANNEL_ALL) {
		return JPG_BRC_ALL;
	} else if (yuvChannel == JPG_CHANNEL_Y) {
		return JPG_BRC_Y;
	} else if (yuvChannel == JPG_CHANNEL_U) {
		return JPG_BRC_U;
	} else { // if (yuvChannel == JPG_CHANNEL_V)
		return JPG_BRC_V;
	}
}
/*
    JPEG adjust dimension

    JPEG adjust dimension for JPEG_YUV_FORMAT_100

    @retur void
*/
static void JpegAdjustDim(JPG_YUV_FORMAT yuvFormat, UINT32 *puiWidth, UINT32 *puiHeight)
{
	if (yuvFormat == JPG_FMT_YUV420) {
		*puiWidth >>= 1;
		*puiHeight >>= 1;
	} else if (yuvFormat == JPG_FMT_YUV422 || (yuvFormat == JPG_FMT_YUV211)) {
		*puiWidth >>= 1;
	} else { //if (yuvFormat == JPG_FMT_YUV211V)
		*puiHeight >>= 1;
	}
}
#endif

void jpg_get_brc_qf(UINT32 id,JPG_YUV_FORMAT yuv_fmt, UINT32 *p_qf, UINT32 bs_size)
{
#if 0
	DBG_DUMP("[jpg_get_brc_qf] TEST\r\n");
	JPEGBSCtrlDebugLevel(BRC_DEBUG_LEVEL_FULL);
	g_brcParam.UBoundByte = 0x300000;
	g_brcParam.TargetByte = 0x200000;
	g_brcParam.LBoundByte = 0x100000;
	DBG_DUMP("[jpg_get_brc_qf] UBound 0x%08X\r\n", g_brcParam.UBoundByte);
	DBG_DUMP("[jpg_get_brc_qf] Target 0x%08X\r\n", g_brcParam.TargetByte);
	DBG_DUMP("[jpg_get_brc_qf] LBound 0x%08X\r\n", g_brcParam.LBoundByte);
#endif

	//if ((bs_size > g_brcParam.UBoundByte) || (bs_size < g_brcParam.LBoundByte))
	{
		//#NT#2011/08/09#Ben Wang -begin
		//#NT#Add adaptive BRC mechanism
		JPGBRC_INFO brcInfo;

		if (yuv_fmt == JPG_FMT_YUV211) {
			brcInfo.input_raw = JPGBRC_COMBINED_YUV422;
		} else if (yuv_fmt == JPG_FMT_YUV420) {
			brcInfo.input_raw = JPGBRC_COMBINED_YUV420;
		} else {
			brcInfo.input_raw = JPGBRC_COMBINED_YUV420;
			DBG_ERR("yuv_fmt(%d) not supported\r\n", (int)(yuv_fmt));
		}
		brcInfo.current_qf = *p_qf;
		brcInfo.current_rate = bs_size;
		brcInfo.pbrc_param = &g_brcParam[id];
		*p_qf = jpgbrc_predict_quality(&brcInfo);
#if JPEG_BRC2
        if(bs_size > brcInfo.pbrc_param->ubound_byte)
       	{
            g_uiBrcStdQTableQuality[id]= g_uiBrcStdQTableQuality[id]-BRC2_STEP_Q_QUALITY;
			if (g_uiBrcStdQTableQuality[id] < BRC2_MIN_Q_QUALITY) {
			    g_uiBrcStdQTableQuality[id] = BRC2_MIN_Q_QUALITY;
		    }
       	}

        if(bs_size < brcInfo.pbrc_param->lbound_byte)
       	{
            g_uiBrcStdQTableQuality[id]= g_uiBrcStdQTableQuality[id]+BRC2_STEP_Q_QUALITY;
			if (g_uiBrcStdQTableQuality[id] > BRC2_MAX_Q_QUALITY) {
			     g_uiBrcStdQTableQuality[id] = BRC2_MAX_Q_QUALITY;
		    }
       	}

#if Q_CHECK
        if(g_uiMinQTableQuality[id]) {
			if(g_uiBrcStdQTableQuality[id]<g_uiMinQTableQuality[id])
			{
				g_uiBrcStdQTableQuality[id] = g_uiMinQTableQuality[id];
				DBG_IND(" lower than min Q : set Q to min Q.  [Q] = (%d) \r\n", (int)(g_uiBrcStdQTableQuality[id]));
			}
        }

		if(g_uiMaxQTableQuality[id]) {
			if(g_uiBrcStdQTableQuality[id]>g_uiMaxQTableQuality[id])
			{
				g_uiBrcStdQTableQuality[id] = g_uiMaxQTableQuality[id];
				DBG_IND(" Larger than max Q : set Q to max Q.  [Q] = (%d) \r\n", (int)(g_uiBrcStdQTableQuality[id]));
			}
		}
#endif
        JpegTransStdQTable(g_uiBrcStdQTableQuality[id], ucBrcStdQTableY, ucBrcStdQTableUV);
#else
#if ADAPTIVE_STD_Q_TABLE_FLOW
		if (brcInfo.brc_result == JPGBRC_RESULT_OVERFLOW) {
			UINT32 uiPrevStdQuality;

			uiPrevStdQuality = g_uiBrcStdQTableQuality[id];
			//DBG_ERR("Next QF overflow\r\n");
			if (g_uiBrcStdQTableQuality[id] >= ADAPTIVE_ADJUST_VALUE) {
				g_uiBrcStdQTableQuality[id] -= ADAPTIVE_ADJUST_VALUE;
			}
			if (g_uiBrcStdQTableQuality[id] < ADAPTIVE_MIN_STD_Q_QUALITY) {
				g_uiBrcStdQTableQuality[id] = ADAPTIVE_MIN_STD_Q_QUALITY;
			}
#if Q_CHECK
            if(g_uiMinQTableQuality[id]) {
	    		if(g_uiBrcStdQTableQuality[id]<g_uiMinQTableQuality[id])
	    		{
	    			g_uiBrcStdQTableQuality[id] = g_uiMinQTableQuality[id];
	    		}
            }
#endif
			JpegTransStdQTable(g_uiBrcStdQTableQuality[id], ucBrcStdQTableY, ucBrcStdQTableUV);

			DBG_IND("^BQF overflow,StdQ %d->%d\r\n", (int)(uiPrevStdQuality), (int)(g_uiBrcStdQTableQuality[id]));
		} else if (brcInfo.brc_result == JPGBRC_RESULT_UNDERFLOW) {
			UINT32 uiPrevStdQuality;

			uiPrevStdQuality = g_uiBrcStdQTableQuality[id];
			//DBG_ERR("Next QF underflow\r\n");
			g_uiBrcStdQTableQuality[id] += ADAPTIVE_ADJUST_VALUE;
			if (g_uiBrcStdQTableQuality[id] > ADAPTIVE_MAX_STD_Q_QUALITY) {
				g_uiBrcStdQTableQuality[id] = ADAPTIVE_MAX_STD_Q_QUALITY;
			}
#if Q_CHECK
            if(g_uiMaxQTableQuality[id]) {
	            if(g_uiBrcStdQTableQuality[id]>g_uiMaxQTableQuality[id])
	    		{
	    			g_uiBrcStdQTableQuality[id] = g_uiMaxQTableQuality[id];
	    		}
            }
#endif
			JpegTransStdQTable(g_uiBrcStdQTableQuality[id], ucBrcStdQTableY, ucBrcStdQTableUV);

			DBG_IND("^YQF underflow,StdQ %d->%d\r\n", (int)(uiPrevStdQuality), (int)(g_uiBrcStdQTableQuality[id]));

		}
#endif
#endif
//        *p_qf = JPEGBRC_predictQuality(input_raw, *p_qf, bs_size, pBRCParam);
		//#NT#2011/08/09#Ben Wang -end
		if (((brcInfo.input_raw == (UINT32)JPGBRC_COMBINED_YUV420) || (brcInfo.input_raw == (UINT32)JPGBRC_COMBINED_YUV422) || (brcInfo.input_raw == (UINT32)JPGBRC_YUV100))
			&& (*p_qf == 1)) {
			DBG_MSG("[jpg_get_brc_qf] QF Reach Minimum Value (1)\r\n");
		}
	}
	//DBG_MSG("[jpg_get_brc_qf] BS Size 0x%08X\r\n", bs_size);
}
#endif

void jpg_enc_setdata(UINT32 id, JPG_ENC_DATA_SET attribute, UINT32 value)
{
	switch (attribute) {

	case JPG_TARGET_RATE:
			g_jpeg_targetrate[id] = value;
			g_uiBrcCurrTarget_Rate[id] = 0;
#if JPEGENC_DEBUG
	DBG_ERR("<<<<<<<<<<<<<<< jpg_enc_setdata JPG_TARGET_RATE>>>>>>>>>>>>>>>>>>>>>>>>>> 0x%x\r\n",(unsigned int)value);
#endif

		break;
	case JPG_VBR_QUALITY:
			g_jpeg_quality[id] = value;
#if JPEGENC_DEBUG
	DBG_ERR("<<<<<<<<<<<<<<< jpg_enc_setdata JPG_VBR_QUALITY>>>>>>>>>>>>>>>>>>>>>>>>>> %d\r\n",(int)value);
#endif

		break;
	case JPG_BRC_STD_QTABLE_QUALITY:
		if (value > 0 && value <= 100) {
			DBG_ERR("Setting below with channel 0 first, need check");
			g_uiBrcStdQTableQuality[id] = value;
			DBG_MSG("jpg_enc_setdata> JPG_BRC_STD_QTABLE_QUALITY = %d\r\n", (int)(g_uiBrcStdQTableQuality[id]));
		} else {
			DBG_ERR("value error!");
		}
		break;

	default:
		DBG_ERR("attribute error");
		break;
	}

}

ER jpg_async_enc_open(void)
{
	ER E_Ret;
	if (m_IsASyncEncOpened) {
		//DBG_WRN("Wait for JPEG ready. \r\n");
	}
	E_Ret = jpeg_open();
	m_IsASyncEncOpened = TRUE;
	return E_Ret;
}
UINT32 jpg_async_enc_setheader(PJPG_ENC_CFG p_jpg_param, UINT8 *p_q_tbl_y, UINT8 *p_q_tbl_uv)
{
	UINT8   *JPG_BS_Buf_Addr, *q_tbl_y, *q_tbl_uv;
	UINT32  img_width, img_height;
    UINT8   *sos_mark;
	//UINT8   *dht_mark;
	TAG_DRI  tag_dri;
	TAG_SOS  tag_sos;
	UINT8   *pBuf;

	JPG_YUV_FORMAT  yuv_fmt = p_jpg_param->yuv_fmt;
#if 0
	if (FALSE == m_IsASyncEncOpened) {
		DBG_ERR("jpg_async_enc_open NOT opened yet! \r\n");
		return 0;
	}
#endif
	if (p_q_tbl_y == NULL || p_q_tbl_uv == NULL) {
		DBG_ERR("PARAMETER ERROR\r\n");
		return 0;
	}
	g_header.addr = p_jpg_param->bs_addr;
	JPG_BS_Buf_Addr = (UINT8 *)(p_jpg_param->bs_addr);
	img_width  = p_jpg_param->width;
	img_height = p_jpg_param->height;

	if (p_jpg_param->header_form == JPG_HEADER_MOV) {
		memcpy((void *)JPG_BS_Buf_Addr, (void *) &MOVMJPGHeader,  sizeof(MOVMJPGHeader)); //Copy  JPG Header. Is it correct??
		//Set JPG header image size.
		((MOVMJPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.img_width[0] = hi_byte(img_width);
		((MOVMJPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.img_width[1] = lo_byte(img_width);
		((MOVMJPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.img_height[0] = hi_byte(img_height);
		((MOVMJPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.img_height[1] = lo_byte(img_height);
		q_tbl_y = (UINT8 *)(((MOVMJPG_HEADER *)(JPG_BS_Buf_Addr))->mark_dqt.q_tbl_0th.q_data);
		q_tbl_uv = (UINT8 *)(((MOVMJPG_HEADER *)(JPG_BS_Buf_Addr))->mark_dqt.q_tbl_1th.q_data);
		g_header.size = sizeof(MOVMJPG_HEADER);
	} else if (p_jpg_param->header_form == JPG_HEADER_QV5 || p_jpg_param->header_form == JPG_HEADER_QV5_NO_HUFFTABLE) {
		memcpy((void *)JPG_BS_Buf_Addr, (void *) &QV5AVIHeader,  sizeof(QV50AVI_HEADER)); //Copy  JPG Header. Is it correct??
		g_header.size = sizeof(QV50AVI_HEADER);
		//Add huffman table to JPEG header
		if (p_jpg_param->header_form == JPG_HEADER_QV5) {
			UINT8   *pTemp;
			//memcpy((void *)JPG_BS_Buf_Addr, (void *) &QV5AVIHeader, sizeof(QV50AVI_HEADER) - sizeof(TAG_SOS));
			pTemp = JPG_BS_Buf_Addr + sizeof(QV50AVI_HEADER) - sizeof(TAG_SOS);
			memcpy((void *)pTemp, (void *) &JPGHeader.mark_dht, sizeof(TAG_DHT));
			pTemp = JPG_BS_Buf_Addr + sizeof(QV50AVI_HEADER) - sizeof(TAG_SOS) + sizeof(TAG_DHT);
			memcpy((void *)pTemp, (void *) &JPGHeader.mark_sos, sizeof(TAG_SOS));
			g_header.size += sizeof(TAG_DHT);
		}

		//Set JPG header image size.
		((QV50AVI_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.img_width[0] = hi_byte(img_width);
		((QV50AVI_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.img_width[1] = lo_byte(img_width);
		((QV50AVI_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.img_height[0] = hi_byte(img_height);
		((QV50AVI_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.img_height[1] = lo_byte(img_height);
		q_tbl_y = (UINT8 *)(((QV50AVI_HEADER *)(JPG_BS_Buf_Addr))->mark_dqt.q_tbl_0th.q_data);
		q_tbl_uv = (UINT8 *)(((QV50AVI_HEADER *)(JPG_BS_Buf_Addr))->mark_dqt.q_tbl_1th.q_data);
	} else { //default setting is JPG_HEADER_STD
		memcpy((void *)JPG_BS_Buf_Addr, (void *) &JPGHeader, JPG_HEADER_SIZE);

		//Set JPG header image size.
		((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.img_width[0] = hi_byte(img_width);
		((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.img_width[1] = lo_byte(img_width);
		((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.img_height[0] = hi_byte(img_height);
		((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.img_height[1] = lo_byte(img_height);
		q_tbl_y  = (UINT8 *)(((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_dqt.q_tbl_0th.q_data);
		q_tbl_uv = (UINT8 *)(((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_dqt.q_tbl_1th.q_data);
		g_header.size = sizeof(JPG_HEADER) - sizeof(TAG_DRI);

	    //insert DRI jpeg tag info when restart interval enabled
	    if (p_jpg_param->interval > 0)
	    {
			sos_mark = (UINT8 *)(&((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sos);
			memcpy((UINT8 *)(&tag_sos), sos_mark, sizeof(TAG_SOS));
			tag_dri.tag_id[0] = 0xFF;
			tag_dri.tag_id[1] = 0xDD;
			tag_dri.tag_length[0] = 0x00;
			tag_dri.tag_length[1] = 0x04;
			tag_dri.interval = ((p_jpg_param->interval & 0xff00) >> 8 | (p_jpg_param->interval & 0x00ff) << 8);

			memcpy(sos_mark, (UINT8 *)(&tag_dri), sizeof(TAG_DRI));
#if 0 //For coverity
			memcpy((UINT8 *)(sos_mark+sizeof(TAG_DRI)), (UINT8 *)(&tag_sos), sizeof(TAG_SOS));
#else
			//TAG_DRI size = 6
			pBuf = JPG_BS_Buf_Addr+JPG_HEADER_SIZE-8;
			//TAG_SOS size = 14
			memcpy((UINT8 *)pBuf, (UINT8 *)(&tag_sos), 14);
#endif
		    g_header.size += sizeof(TAG_DRI);
	    }

		if (yuv_fmt == JPG_FMT_YUV100) {
            //only 1 channel
			((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sos.tag_length[1] = 0x8;
			((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sos.component_nums = 0x1;
			//sos_mark = (UINT8 *)(&((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sos);

#if 0 //For coverity
			memcpy((UINT8 *)(sos_mark+7), (UINT8 *)(sos_mark+11), 3);
#else
			pBuf = JPG_BS_Buf_Addr+JPG_HEADER_SIZE-7;
			memcpy((UINT8 *)pBuf, (UINT8 *)(pBuf+4), 3);
#endif
			//DBG_INFO("g_header.size -= 6 \r\n");
			g_header.size -= 4;

			//dht_mark = (UINT8 *)(&((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_dht);
#if 0 //For coverity
			memcpy((UINT8 *)(dht_mark-6), dht_mark, (sizeof(TAG_DHT) + sizeof(TAG_SOS)-4));
#else
			//pBuf = dht_mark-6
			pBuf = JPG_BS_Buf_Addr+JPG_HEADER_SIZE-sizeof(TAG_SOS)-sizeof(TAG_DHT)-6;
			memmove((UINT8 *)pBuf, (UINT8 *)(pBuf+6), (sizeof(TAG_DHT) + sizeof(TAG_SOS)-4));
#endif
			g_header.size -= 6;
		}
	}

	memcpy(q_tbl_y, p_q_tbl_y, DQT_SIZE);
	memcpy(q_tbl_uv, p_q_tbl_uv, DQT_SIZE);

	//Use JPEG_YUV_FORMAT_xxx macro instead of JPG_HW_FMT_xxx for jpeg_setFileFormat()
	if (yuv_fmt == JPG_FMT_YUV422 || yuv_fmt == JPG_FMT_YUV211) {
		//default is 422... so we don't setting it again
		//[Hint] The DCF YUV422 foramt is equal to our YUV211 format.
		jpeg_set_format(img_width, img_height, JPEG_YUV_FORMAT_211);
	} else if (yuv_fmt == JPG_FMT_YUV420) {
		if (p_jpg_param->header_form == JPG_HEADER_MOV) {
			((MOVMJPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.component1.sample_hv_rate = 0x22; // YUV422-> 21
		} else if (p_jpg_param->header_form == JPG_HEADER_QV5 || p_jpg_param->header_form == JPG_HEADER_QV5_NO_HUFFTABLE) {
			((QV50AVI_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.component1.sample_hv_rate = 0x22; // YUV422-> 21
		} else {
			((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.component1.sample_hv_rate = 0x22;   // YUV422-> 21
		}
		jpeg_set_format(img_width, img_height, JPEG_YUV_FORMAT_411);
	} else if (yuv_fmt == JPG_FMT_YUV100) {

		jpeg_set_format(img_width, img_height, JPEG_YUV_FORMAT_100);
		((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.tag_length[1] = 0xB;
		((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.component_nums = 1;
		((JPG_HEADER *)(JPG_BS_Buf_Addr))->mark_sof.component1.sample_hv_rate = 0x11;   // YUV100-> 11

		//DBG_INFO("yuv_fmt == JPG_FMT_YUV100! \r\n");

	} else {
		DBG_ERR("PARAMETER ERROR\r\n");
		return 0;    //Parameter Error. No JPG output file format.
	}

	jpeg_set_imglineoffset(p_jpg_param->lineoffset[0], p_jpg_param->lineoffset[1], 0);

	if (p_jpg_param->header_form != JPG_HEADER_QV5_NO_HUFFTABLE) {
		jpeg_enc_set_hufftable((UINT16 *)gstd_enc_lum_actbl, (UINT16 *)gstd_enc_lum_dctbl,
						   (UINT16 *)gstd_enc_chr_actbl, (UINT16 *)gstd_enc_chr_dctbl);
	}

	//Support bitstream output enable/disable
	if (p_jpg_param->bs_out_dis) {
		jpeg_set_bsoutput(0);    // disable bitstream output
	} else {
		jpeg_set_bsoutput(1);    // enable bitstream output
	}

	return g_header.size;


}
ER jpg_async_enc_start(UINT32 bs_bufaddr, UINT32 bs_bufsize, UINT32 img_y_addr, UINT32 img_uv_addr)
{
#if 0
	if (FALSE == m_IsASyncEncOpened) {
		DBG_ERR("jpg_async_enc_open NOT opened yet! \r\n");
		return E_SYS;
	}
#endif
	jpeg_set_bsstartaddr(bs_bufaddr, (UINT32)bs_bufsize);
	jpeg_set_enableint(JPEG_INT_FRAMEEND | JPEG_INT_BUFEND | JPEG_INT_SLICEDONE);
	jpeg_set_imgstartaddr(img_y_addr, img_uv_addr, 0);
	jpeg_set_startencode(bs_bufaddr,bs_bufsize);
	return E_OK;
}

ER jpg_async_enc_waitdone(UINT32 *p_jpg_size)
{
	UINT32  EReturn, IntrStatus = 0;
#if 0
	if (FALSE == m_IsASyncEncOpened) {
		DBG_ERR("jpg_async_enc_open NOT opened yet! \r\n");
		return E_SYS;
	}
#endif
	IntrStatus = jpeg_waitdone();
	EReturn = E_OK;

	if ((IntrStatus & JPEG_INT_FRAMEEND) == 0) {
		if (IntrStatus & JPEG_INT_BUFEND) { // Buffer is too small
			EReturn = E_NOMEM;
BUF_SMALL:
			DBG_MSG("Encode BS Buffer is too small. Re-asign BS buffer.\r\n");
			jpeg_set_enableint(JPEG_INT_FRAMEEND | JPEG_INT_BUFEND | JPEG_INT_SLICEDONE);
			jpeg_set_bsoutput(0);
			jpeg_set_bsstartaddr(jpeg_get_bsstartaddr(), jpeg_get_bscurraddr() - jpeg_get_bsstartaddr());
			if (jpeg_waitdone() & JPEG_INT_FRAMEEND) {
				DBG_MSG("Frame end.\r\n");
			} else {
				DBG_MSG("Still Buffer End\r\n");
				goto BUF_SMALL;
			}
		} else {
			EReturn = E_SYS;
			DBG_ERR("Error Status(0x%X)\r\n", (unsigned int)(IntrStatus));
		}
	}
	jpeg_set_endencode();
	//*m_puiJPGBSBufSize = jpeg_get_bssize() + JPG_HEADER_SIZE;
	*p_jpg_size = jpeg_get_bssize() + g_header.size;
	if (g_header.size == sizeof(MOVMJPG_HEADER)) {
		UINT32   thisJpgSize, swapJpgSize;
		MOVMJPG_HEADER *pThisHeader;

		pThisHeader = (MOVMJPG_HEADER *)g_header.addr;
		thisJpgSize = *p_jpg_size;
		swapJpgSize = Jpg_Swap(thisJpgSize);//FFD9-FFD8+2
		pThisHeader->MarkCME1.FieldSize = swapJpgSize;
		swapJpgSize = Jpg_Swap(thisJpgSize + 10); //next FFD8 - FFD8

		pThisHeader->MarkCME1.PaddedSize = swapJpgSize;//next FFD8 - FFD8
		pThisHeader->MarkCME1.Offset2Next = swapJpgSize;
	}

	return EReturn;
}
ER jpg_async_enc_close(void)
{
#if 0
	if (FALSE == m_IsASyncEncOpened) {
		DBG_ERR("jpg_async_enc_open NOT opened yet! \r\n");
		return E_SYS;
	}
#endif
	m_IsASyncEncOpened = FALSE;
	return jpeg_close();
}



void jpeg_set_property(KDRV_JPEG_INFO *p_jpeg_info)
{
    memcpy(&kdrv_info, p_jpeg_info, sizeof(KDRV_JPEG_INFO));
}

ER jpeg_trigger(JPEG_CODEC_MODE codec_mode, void *p_param, KDRV_CALLBACK_FUNC *p_cb_func)
{
    KDRV_VDOENC_PARAM *p_enc_param;
	KDRV_VDODEC_PARAM *p_dec_param;
	//BOOL    is_block_mode = FALSE;
	UINT32  HeaderSize;
	JPG_ENC_CFG     jpgCmpParam;
//	JPG_HEADER_ER jpghdr_errcode = JPG_HEADER_ER_OK;

	UINT32 MCU_W;
	UINT32 MCU_H;
	UINT32 MCU_W_NOTALIGN = 0;
	UINT32 MCU_H_NOTALIGN = 0;

	UINT32 id = 0;

	ER  ret = E_OK;

    if (codec_mode == JPEG_CODEC_MODE_ENC)
    {
        p_enc_param = (KDRV_VDOENC_PARAM *)p_param;

        memset((void *)&jpgCmpParam, 0, sizeof(JPG_ENC_CFG));
        //printk("jpeg_enc_set_hufftable\r\n");
        //Below move to jpg_async_enc_setheader
        //jpeg_enc_set_hufftable((UINT16 *)gstd_enc_lum_actbl, (UINT16 *)gstd_enc_lum_dctbl,
    	//					   (UINT16 *)gstd_enc_chr_actbl, (UINT16 *)gstd_enc_chr_dctbl);

		id = p_enc_param->temproal_id;
        //p_enc_param->vbr_mode = 1;   //for test
		//if (p_enc_param->target_rate) {g_jpeg_targetrate[id]

        //Set min quality value
		if(p_enc_param->min_quality)
		{
			g_uiMinQTableQuality[id] = p_enc_param->min_quality;
			DBG_IND("g_uiMinQTableQuality= 0x%x\r\n",(unsigned int)g_uiMinQTableQuality[id]);
		}

        //Set max quality value
		if(p_enc_param->max_quality)
		{
            g_uiMaxQTableQuality[id] = p_enc_param->max_quality;
            DBG_IND("g_uiMaxQTableQuality= 0x%x\r\n",(unsigned int)g_uiMaxQTableQuality[id]);
		}

		if (g_uiBrcCurrTarget_Rate[id] != g_jpeg_targetrate[id]) {           //initial setting targetrate

  		    if(!g_reEnc[id])
			    p_enc_param->target_rate = p_enc_param->target_rate >>3;

			//BRC path
			if (p_enc_param->target_rate != g_uiBrcCurrTarget_Rate[id]) {
                //still image using default setting
                if(p_enc_param->quality){
					g_uiBrcStdQTableQuality[id] = p_enc_param->quality;
                }

#if Q_CHECK
            if(g_uiMinQTableQuality[id]) {
    			if(g_uiBrcStdQTableQuality[id]<g_uiMinQTableQuality[id])
    			{
        			DBG_WRN(" quality (%d) is lower than min_quality (%d)\r\n", (int)(g_uiBrcStdQTableQuality[id]),(int)(g_uiMinQTableQuality[id]));
    				g_uiBrcStdQTableQuality[id] = g_uiMinQTableQuality[id];
    			}
            }

    		if(g_uiMaxQTableQuality[id]) {
    			if(g_uiBrcStdQTableQuality[id]>g_uiMaxQTableQuality[id])
    			{
                    DBG_WRN(" quality (%d) is larger than max_quality (%d)\r\n", (int)(g_uiBrcStdQTableQuality[id]),(int)(g_uiMaxQTableQuality[id]));
    				g_uiBrcStdQTableQuality[id] = g_uiMaxQTableQuality[id];
    			}
    		}
#endif

				JpegTransStdQTable(g_uiBrcStdQTableQuality[id], ucBrcStdQTableY, ucBrcStdQTableUV);
				//jpg_async_set_brc_qtable(g_uiBrcQF, ucBrcStdQTableY, ucBrcStdQTableUV);

				if (p_enc_param->frame_rate)
					g_uiBrcTargetSize[id] = p_enc_param->target_rate/p_enc_param->frame_rate;
				else
					g_uiBrcTargetSize[id] = p_enc_param->target_rate/30;

				g_uiBrcCurrTarget_Rate[id] = p_enc_param->target_rate;

				g_brcParam[id].width = p_enc_param->encode_width;
	        	g_brcParam[id].height = p_enc_param->encode_height;
	        	g_brcParam[id].target_byte = g_uiBrcTargetSize[id];
				//g_uiBrcTargetSize +- 15%
				g_brcParam[id].ubound_byte = g_uiBrcTargetSize[id]+2*(g_uiBrcTargetSize[id]/20);
				g_brcParam[id].lbound_byte = g_uiBrcTargetSize[id]-2*(g_uiBrcTargetSize[id]/20);

				//printk("jpeg brc target rate = 0x%x bytes\r\n",(unsigned int)g_uiBrcCurrTarget_Rate[id]);
                                //printk("jpeg frame target = 0x%x bytes\r\n",(unsigned int)g_uiBrcTargetSize[id]);
				//printk("jpeg ubound_byte = 0x%x bytes\r\n",(unsigned int)g_brcParam[id].ubound_byte);
				//printk("jpeg lbound_byte = 0x%x bytes\r\n",(unsigned int)g_brcParam[id].lbound_byte);

                //initial
				if(p_enc_param->vbr_mode)
				{
					g_uiVBR_QUALITY[id] = p_enc_param->quality;
					//printk("Init VBR mode [%d] (fix Q) Q = 0x%x\r\n",(int)id,(unsigned int)p_enc_param->quality);
					//(fix quality)
#if JPEG_BRC2

#if Q_CHECK
					if(g_uiMinQTableQuality[id]) {
						if(g_uiVBR_QUALITY[id]<g_uiMinQTableQuality[id])
						{
    						DBG_WRN(" quality (%d) is lower than min_quality (%d)\r\n", (int)(g_uiVBR_QUALITY[id]),(int)(g_uiMinQTableQuality[id]));
							g_uiVBR_QUALITY[id] = g_uiMinQTableQuality[id];
						}
					}

					if(g_uiMaxQTableQuality[id]) {
						if(g_uiVBR_QUALITY[id]>g_uiMaxQTableQuality[id])
						{
    						DBG_WRN(" quality (%d) is larger than max_quality (%d)\r\n", (int)(g_uiVBR_QUALITY[id]),(int)(g_uiMaxQTableQuality[id]));
							g_uiVBR_QUALITY[id] = g_uiMaxQTableQuality[id];
						}
					}
#endif
                    jpg_async_set_qtable(g_uiVBR_QUALITY[id], QTableY, QTableUV);
#else
					//jpg_async_set_qtable(p_enc_param->quality, QTableY, QTableUV);
					JpegTransStdQTable(g_uiVBR_QUALITY[id], ucBrcStdQTableY, ucBrcStdQTableUV);
					g_uiBrcQF[id] = DEFAULT_BRC_VBR_QUALITY;
					jpg_async_set_brc_qtable(g_uiBrcQF[id], QTableY, QTableUV);
#endif
					g_uiVBR[id] = VBR_NORAMAL;
					g_TotalFrameID[id] = 0;
					g_uiVBR_Q[id] = jpeg_get_DCQvalue()&0xFF;
#if JPEGENC_DEBUG
					printk("[VBR]Init ------ g_uiVBR_Q Q = 0x%x \r\n",(unsigned int)g_uiVBR_Q[id]);
#endif
				}
			}

			if(p_enc_param->vbr_mode)
			{
				if(g_uiVBR[id] == VBR_NORAMAL)  //VBR
				{
					if(g_uiVBRmodechange[id])
					{
#if JPEGENC_DEBUG
						printk("CBR to VBR(%d), g_uiVBRmodechange Q = 0x%x \r\n",(int)id,(unsigned int)g_uiVBR_QUALITY[id]);
#endif
						g_uiVBRmodechange[id] = 0;
						//jpg_async_set_qtable(p_enc_param->quality, QTableY, QTableUV);
						//jpg_async_set_qtable(g_uiVBR_QUALITY[id], QTableY, QTableUV);
						//JpegTransStdQTable(g_uiVBR_QUALITY[id], ucBrcStdQTableY, ucBrcStdQTableUV);
						//g_uiBrcQF[id] = DEFAULT_BRC_VBR_QUALITY;
						//jpg_async_set_brc_qtable(g_uiBrcQF[id], QTableY, QTableUV);
					}
					// If no changed , use def init value
					jpg_async_set_qtable(g_uiVBR_QUALITY[id], QTableY, QTableUV);
#if JPEG_BRC2
					g_uiBrcStdQTableQuality[id] = g_uiVBR_QUALITY[id];
#endif
				}
				else // CBR
				{
				    if(g_uiVBRmodechange[id])
					{
#if JPEGENC_DEBUG
					    printk("VBR to CBR(%d), init Q = 0x%x\r\n",(int)id, (unsigned int)g_uiBrcStdQTableQuality[id]);
#endif
					    //printk("g_uiVBR_QUALITY Q = 0x%x\r\n",(unsigned int)g_uiVBR_QUALITY[id]);
						g_uiBrcStdQTableQuality[id] = g_uiVBR_QUALITY[id];
					    JpegTransStdQTable(g_uiBrcStdQTableQuality[id], ucBrcStdQTableY, ucBrcStdQTableUV);
						g_uiVBRmodechange[id] = 0;

						g_uiBrcQF[id] = DEFAULT_BRC_VBR_QUALITY;
						//jpg_async_set_brc_qtable(g_uiBrcQF[id], QTableY, QTableUV);
						//jpg_async_set_brc_qtable(DEFAULT_BRC_QF, QTableY, QTableUV);
						//g_uiBrcQF[id] = DEFAULT_BRC_QF;
					//}else {
					//	jpg_async_set_brc_qtable(g_uiBrcQF[id], QTableY, QTableUV);
					}
#if JPEG_BRC2
                    jpg_async_set_qtable(g_uiBrcStdQTableQuality[id], QTableY, QTableUV);
#else
					jpg_async_set_brc_qtable(g_uiBrcQF[id], QTableY, QTableUV);
#endif
				}
				//g_uiDC_Qavg[id] = jpeg_get_DCQvalue()&0xFF;
//#if JPEGENC_DEBUG
//				printk("[VBR] g_uiDC_Qavg Q = 0x%x \r\n",(unsigned int)g_uiDC_Qavg[id]);
//#endif

			}
			else
			{
#if Q_CHECK
		        if(g_uiMinQTableQuality[id]) {
					if(g_uiBrcStdQTableQuality[id]<g_uiMinQTableQuality[id])
					{
						g_uiBrcStdQTableQuality[id] = g_uiMinQTableQuality[id];
						DBG_IND(" lower than min Q : set Q to min Q.  [Q] = (%d) \r\n", (int)(g_uiBrcStdQTableQuality[id]));
					}
		        }

				if(g_uiMaxQTableQuality[id]) {
					if(g_uiBrcStdQTableQuality[id]>g_uiMaxQTableQuality[id])
					{
						g_uiBrcStdQTableQuality[id] = g_uiMaxQTableQuality[id];
						DBG_IND(" Larger than max Q : set Q to max Q.  [Q] = (%d) \r\n", (int)(g_uiBrcStdQTableQuality[id]));
					}
				}
#endif

#if JPEG_BRC2
				//printk("[CBR] new Q = %d \r\n",(unsigned int)g_uiBrcStdQTableQuality[id]);
				jpg_async_set_qtable(g_uiBrcStdQTableQuality[id], QTableY, QTableUV);

#else
     			//printk("[CBR] g_uiBrcQF = 0x%x \r\n",(unsigned int)g_uiBrcQF[id]);
				jpg_async_set_brc_qtable(g_uiBrcQF[id], QTableY, QTableUV);
#endif
			}
		} else {
			//Normal encode (fix quality)
			jpg_async_set_qtable(p_enc_param->quality, QTableY, QTableUV);
			g_uiBrcTargetSize[id] = 0;
		}

		//printk("pEncRegSet->uiQuality = %d\r\n", (int)(p_enc_param->quality));
		//jpg_async_set_qtable(p_enc_param->quality, QTableY, QTableUV);

        // Set format
		//move to jpg_async_enc_setheader
		if (p_enc_param->in_fmt == KDRV_JPEGYUV_FORMAT_420) {
			//printk("KDRV_JPEGYUV_FORMAT_420\r\n");
			jpgCmpParam.yuv_fmt = JPG_FMT_YUV420;
			if (p_enc_param->fmt_trans_en)
			{
				jpgCmpParam.yuv_fmt = JPG_FMT_YUV211;
				//printk("420 trans to 211\r\n");
			}
		} else if (p_enc_param->in_fmt == KDRV_JPEGYUV_FORMAT_422) {
			//printk("KDRV_JPEGYUV_FORMAT_422\r\n");
			jpgCmpParam.yuv_fmt = JPG_FMT_YUV211;

		} else if (p_enc_param->in_fmt == KDRV_JPEGYUV_FORMAT_100) {
			//printk("KDRV_JPEGYUV_FORMAT_100\r\n");
			jpgCmpParam.yuv_fmt = JPG_FMT_YUV100;
		} else {
			DBG_ERR("No support format [0x%x]\r\n",jpgCmpParam.yuv_fmt);
		}
		//printk("W = %d  H = %d\r\n", (int)(p_enc_param->encode_width), (int)(p_enc_param->encode_height));

    	jpgCmpParam.width = p_enc_param->encode_width;
    	jpgCmpParam.height= p_enc_param->encode_height;
    	jpgCmpParam.bs_addr = p_enc_param->bs_addr_1;
    	jpgCmpParam.lineoffset[0] = p_enc_param->y_line_offset;
    	jpgCmpParam.lineoffset[1] = p_enc_param->c_line_offset;
		jpgCmpParam.interval =  p_enc_param->retstart_interval;

		//frame type = IDR
		p_enc_param->frm_type = 3;

//		jpeg_close();
//		jpg_async_enc_open(); // need this ??
		//if (p_enc_param->target_rate)
		//	HeaderSize = jpg_async_enc_setheader(&jpgCmpParam, ucBrcStdQTableY, ucBrcStdQTableUV);
		//else
			HeaderSize = jpg_async_enc_setheader(&jpgCmpParam, QTableY, QTableUV);

		//printk("HeaderSize = %d\r\n", (int)(HeaderSize));

		p_enc_param->svc_hdr_size = HeaderSize;

		//printk("EncBuf.uiYBuf = 0x%x\r\n", (unsigned int)(p_enc_param->y_addr));
		//printk("EncBuf.uiUVBuf = 0x%x\r\n", (unsigned int)(p_enc_param->c_addr));
		//printk("EncBuf.uiYLineOffset = 0x%x\r\n", (unsigned int)(p_enc_param->y_line_offset));
		//printk("EncBuf.uiUVLineOffset = 0x%x\r\n", (unsigned int)(p_enc_param->c_line_offset));
		jpeg_set_cropdisable();

		// Set source data address
		jpeg_set_imgstartaddr(p_enc_param->y_addr, p_enc_param->c_addr, 0);

		// Set line offset
		jpeg_set_imglineoffset(p_enc_param->y_line_offset, p_enc_param->c_line_offset, 0);

		// Set trans format enable or not
		if (p_enc_param->fmt_trans_en) {
			jpeg_set_fmttransenable();
		} else {
			jpeg_set_fmttransdisable();
		}
		// Set restart marker, jpeg_open() will disable restart marker
		if (p_enc_param->retstart_interval) {
			//printk("kdrv_info.retstart_interval = 0x%x\r\n", (unsigned int)(p_enc_param->retstart_interval));
			// Set restart marker MCU number
			jpeg_set_restartinterval(p_enc_param->retstart_interval);
			// Enable restart marker function
			jpeg_set_restartenable(TRUE);
		}
		//printk("p_enc_param->uiBsAddr = 0x%x\r\n", (unsigned int)(p_enc_param->bs_addr_1));
		//printk("p_enc_param->uiBsSize = 0x%x\r\n", (unsigned int)(p_enc_param->bs_size_1));
		//keep bs size @bs_size_2
		p_enc_param->bs_size_2 = p_enc_param->bs_size_1;
		jpeg_set_bsstartaddr(p_enc_param->bs_addr_1 + HeaderSize, p_enc_param->bs_size_1 - HeaderSize);

		// Enable frame end and buffer end interrupt
		jpeg_set_enableint(JPEG_INT_FRAMEEND | JPEG_INT_BUFEND);
		//printk("Start to encode\r\n");
		// Enable newimageDMA path		
		if (nvt_get_chip_id() == CHIP_NA51084){
			jpeg_set_newimgdma(TRUE);
		}

		// Start to encode
		//Need flush origin define header size (all header size)
		jpeg_set_startencode(p_enc_param->bs_addr_1,sizeof(JPG_HEADER));
#if 0
		// Wait for encode done
		while (1) {
			UINT32  int_sts;

			int_sts = jpeg_waitdone();

			// Frame end
			if (int_sts & JPEG_INT_FRAMEEND) {
				jpeg_set_endencode();
				break;
			}
		}
#endif
		//printk("encode done\r\n");
	}else {

		p_dec_param = (KDRV_VDODEC_PARAM *)p_param;
#if 0
		memset((void *)&jpegdec_Cfg, 0, sizeof(JPGHEAD_DEC_CFG));
		p_dec_param->errorcode = 0;

		jpegdec_Cfg.inbuf = (UINT8 *)p_dec_param->jpeg_hdr_addr;
		DBG_IND("p_dec_param->jpeg_hdr_addr = 0x%x\r\n", (unsigned int)(p_dec_param->jpeg_hdr_addr));

		jpghdr_errcode = jpg_parse_header(&jpegdec_Cfg, DEC_PRIMARY, NULL);

		if (jpghdr_errcode != JPG_HEADER_ER_OK) {

			p_dec_param->errorcode = jpghdr_errcode;
			DBG_ERR("Parese JPEG header error!\r\n");
			return FALSE;
		}
#endif
		DBG_IND("jpegdec_Cfg.headerlen = 0x%x\r\n", (unsigned int)(jpegdec_Cfg.headerlen));
		//BitStream addr = start addr + header size
		p_dec_param->bs_addr = p_dec_param->jpeg_hdr_addr +jpegdec_Cfg.headerlen;

		// Set Huffman table
		jpeg_set_decode_hufftabhw(jpegdec_Cfg.p_huff_dc0th, jpegdec_Cfg.p_huff_dc1th, jpegdec_Cfg.p_huff_ac0th, jpegdec_Cfg.p_huff_ac1th);

		#if 0
			printk("\r\n[pQTable_Y] \r\n");
			for (i = 0; i < 64; i++) {
				printk("0x%x ", (unsigned int)(*(JpegCfg.pQTabY + i)));
			}

			printk("\r\n[pQTable_UV] \r\n");
			for (i = 0; i < 64; i++) {
				printk("0x%x ", (unsigned int)(*(JpegCfg.pQTabUV + i)));
			}
		#endif

		// Set Q table
		jpeg_set_hwqtable(jpegdec_Cfg.p_q_tbl_y, jpegdec_Cfg.p_q_tbl_uv);

		if (jpegdec_Cfg.fileformat == JPG_FMT_YUV420) {
			MCU_W = 16;
			MCU_H = 16;
		} else{ //KDRV_JPEGYUV_FORMAT_422
			MCU_W = 16;
			MCU_H = 8;
		}

		if(jpegdec_Cfg.imagewidth%MCU_W) {
			MCU_W_NOTALIGN = 1;
			jpegdec_Cfg.imagewidth = MAKE_ALIGN(jpegdec_Cfg.imagewidth, MCU_W);
		}else {
			MCU_W_NOTALIGN = 0;
		}

		if(jpegdec_Cfg.imageheight%MCU_H){
			MCU_H_NOTALIGN = 1;
			jpegdec_Cfg.imageheight = MAKE_ALIGN(jpegdec_Cfg.imageheight, MCU_H);
		} else {
			MCU_H_NOTALIGN = 0;
		}

		// Set format (Original width and height)
		// Set format
		if (jpegdec_Cfg.fileformat == JPG_FMT_YUV420) {
			DBG_IND("JPG_FMT_YUV420\r\n");
			p_dec_param->yuv_fmt = KDRV_VDODEC_YUV420;
			jpeg_set_format(jpegdec_Cfg.imagewidth, jpegdec_Cfg.imageheight, JPEG_YUV_FORMAT_411);
		} else{ //KDRV_JPEGYUV_FORMAT_422
			DBG_IND("JPG_FMT_YUV422\r\n");
			p_dec_param->yuv_fmt = KDRV_VDODEC_YUV422;
			jpeg_set_format(jpegdec_Cfg.imagewidth, jpegdec_Cfg.imageheight, JPEG_YUV_FORMAT_211);
		}
		//return width/height
		//if(MCU_W_NOTALIGN)
		//{
		//	p_dec_param->uiWidth = jpegdec_Cfg.imagewidth - MCU_W; //remove dummy 1 MCU
		//} else {
		//	p_dec_param->uiWidth = jpegdec_Cfg.imagewidth;
		//}
		p_dec_param->uiWidth = jpegdec_Cfg.ori_imagewidth;

		//if(MCU_H_NOTALIGN)
		//{
		//	p_dec_param->uiHeight = jpegdec_Cfg.imageheight - MCU_H;
		//	DBG_ERR(" No align p_dec_param->uiHeight= 0x%x\r\n", (unsigned int)(p_dec_param->uiHeight));
		//} else {
		//	p_dec_param->uiHeight = jpegdec_Cfg.imageheight;
		p_dec_param->uiHeight = jpegdec_Cfg.ori_imageheight;
		//}

		// Set output data address
		jpeg_set_imgstartaddr(p_dec_param->y_addr, p_dec_param->c_addr, 0);


		if (jpegdec_Cfg.rstinterval) {
			DBG_IND("kdrv_info.retstart_interval = 0x%x\r\n", (unsigned int)(jpegdec_Cfg.rstinterval));
			// Set restart marker MCU number
			jpeg_set_restartinterval(jpegdec_Cfg.rstinterval);
			// Enable restart marker function
			jpeg_set_restartenable(TRUE);
		}

		if (MCU_W_NOTALIGN ||MCU_H_NOTALIGN) {
			// Set crop information
			jpeg_set_crop(0, 0,
						 MAKE_ALIGN(jpegdec_Cfg.ori_imagewidth , MCU_W),
						 MAKE_ALIGN(jpegdec_Cfg.ori_imageheight , MCU_H));

			// Enable cropping
			jpeg_set_cropenable();
	    } else {
			jpeg_set_cropdisable();
		}

        DBG_IND("p_dec_param->uiBsAddr = 0x%x\r\n", (unsigned int)(p_dec_param->bs_addr));
		DBG_IND("p_dec_param->uiBsSize = 0x%x\r\n", (unsigned int)(p_dec_param->bs_size));

		jpeg_set_bsstartaddr(p_dec_param->bs_addr, p_dec_param->bs_size);

		if ((jpegdec_Cfg.fileformat == JPG_FMT_YUV420) || (jpegdec_Cfg.fileformat == JPG_FMT_YUV211)) {
			// Set line offset (same with Width, need export ?)
			if(MCU_W_NOTALIGN ||MCU_H_NOTALIGN)
			{
				ret = jpeg_set_imglineoffset(MAKE_ALIGN(jpegdec_Cfg.ori_imagewidth , MCU_W), MAKE_ALIGN(jpegdec_Cfg.ori_imagewidth , MCU_W), 0);
			} else {
				ret = jpeg_set_imglineoffset(jpegdec_Cfg.imagewidth, jpegdec_Cfg.imagewidth, 0);
			}

			if (ret != E_OK) {
				p_dec_param->errorcode = 0xFF01; //LineOffset error
			}
		}

		// Enable frame end and buffer end interrupt
		jpeg_set_enableint(JPEG_INT_FRAMEEND | JPEG_INT_BUFEND);
		//printk("Start to decode\r\n");
		// Start to encode
		jpeg_set_startdecode();
#if 0
		// Wait for decode done
		while (1) {
			UINT32  uiIntSts;

			uiIntSts = jpeg_waitdone();

			// Decode error
			if (uiIntSts & JPEG_INT_DECERR) {
				printk(("Decode ERROR!\r\n"));
			}

			// Buffer end
			if (uiIntSts & JPEG_INT_BUFEND) {
				printk("JPEG_INT_BUFEND    need modify\r\n");
				// Must handle buffer end status or frame end won't be issued if buffer size = bit stream length
				// Interrupt will be disabled in ISR, need to enable again
				jpeg_set_enableint(JPEG_INT_FRAMEEND | JPEG_INT_BUFEND);

				// Only one buffer mode
				jpeg_set_bsstartaddr(p_dec_param->bs_addr, p_dec_param->bs_size);

			}

			// Frame end
			if (uiIntSts & JPEG_INT_FRAMEEND) {
				jpeg_set_enddecode();
				break;
			}
		}
#endif
		//printk("decode done\r\n");
	}

	return ret;
}


#if defined __UITRON || defined __ECOS

#elif defined __KERNEL__
EXPORT_SYMBOL(jpg_enc_one_img);
EXPORT_SYMBOL(jpg_async_enc_open);
EXPORT_SYMBOL(jpg_async_enc_setheader);
EXPORT_SYMBOL(jpg_async_enc_start);
EXPORT_SYMBOL(jpg_async_enc_waitdone);
EXPORT_SYMBOL(jpg_async_enc_close);
EXPORT_SYMBOL(jpg_async_set_brc_qtable);
EXPORT_SYMBOL(jpg_async_set_qtable);
EXPORT_SYMBOL(jpg_set_bitrate_ctrl);
EXPORT_SYMBOL(jpg_get_brc_qf);
EXPORT_SYMBOL(jpg_enc_setdata);

#else
#endif

//@}
