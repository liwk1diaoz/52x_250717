/**
    @file       dal_jpegenc.c
    @ingroup	mICodec

    @brief      jpegenc device abstraction layer

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#include "../hal/jpg_int.h" //to do
#include "../../include/jpg_enc.h"


#define DAL_JPEGENC_BRC_QF              			500
#define DAL_JPEGENC_LOWER_BOUND_RATIO        		10
#define DAL_JPEGENC_UPPER_BOUND_RATIO        		10
#define DAL_JPEGENC_STD_QUALITY   					50			//grucStdLuxQ[] is equal to Quality=50
#define DAL_JPEGENC_MAX_BRC_WEIGHTING       		100
#define DAL_JPEGENC_BRC_WEIGHTING            		60

#define DAL_JPEGENC_ADAPTIVE_Q_TABLE_FLOW    		1
#define DAL_JPEGENC_ADAPTIVE_FINE_TUNE_VALUE 		10
#define DAL_JPEGENC_ADAPTIVE_MAX_Q_QUALITY      	DAL_JPEGENC_STD_QUALITY
#define DAL_JPEGENC_ADAPTIVE_MIN_Q_QUALITY      	1

#define DAL_JPEG_MAX_BSLENGTH       				(32*1024*1024-1) //hw spec limit


static UINT32  				gJpegEncStdQTableQuality[DAL_VDOENC_ID_MAX] = {DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY, DAL_JPEGENC_STD_QUALITY};
static JPG_BRC_CFG     		gJpegEncBrcCfg[DAL_VDOENC_ID_MAX];// = {0};
static DAL_VDOENC_INIT  	gJpegEncInit[DAL_VDOENC_ID_MAX];// = {0};
static UINT8  				gJpegEncQTableY[DAL_VDOENC_ID_MAX][64];// = {0};
static UINT8  				gJpegEncQTableUV[DAL_VDOENC_ID_MAX][64];// = {0};
static UINT32      			gJpegEncVidNum[DAL_VDOENC_ID_MAX] = {0};
static UINT32      			gJpegEncGopNum[DAL_VDOENC_ID_MAX] = {15, 15, 15, 15};
static UINT32 				gJpegEncTargetSize[DAL_VDOENC_ID_MAX] = {0};//0 means disabling BRC
static UINT32 				gJpegEncBrcQF[DAL_VDOENC_ID_MAX] = {DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF, DAL_JPEGENC_BRC_QF};
static UINT32 				gJpegEncLowerBoundSize[DAL_VDOENC_ID_MAX] = {0};
static UINT32 				gJpegEncUpperBoundSize[DAL_VDOENC_ID_MAX] = {0};
//static UINT32 				gJpegEncBrcWeighting[DAL_VDOENC_ID_MAX] = {DAL_JPEGENC_BRC_WEIGHTING, DAL_JPEGENC_BRC_WEIGHTING, DAL_JPEGENC_BRC_WEIGHTING, DAL_JPEGENC_BRC_WEIGHTING};

ER dal_jpegenc_init(DAL_VDOENC_ID id, DAL_VDOENC_INIT *pinit) {
	memcpy(&gJpegEncInit[id], pinit, sizeof(DAL_VDOENC_INIT));

	// target rate is based on 30fps
	if (gJpegEncInit[id].uiFrameRate) {
		gJpegEncTargetSize[id] = gJpegEncInit[id].uiByteRate / gJpegEncInit[id].uiFrameRate;
	} else {
		gJpegEncTargetSize[id] = gJpegEncInit[id].uiByteRate / 30;
	}

	if (gJpegEncTargetSize[id]) {
		gJpegEncBrcQF[id] = DAL_JPEGENC_BRC_QF;
		gJpegEncStdQTableQuality[id] = DAL_JPEGENC_STD_QUALITY;
		gJpegEncLowerBoundSize[id] = gJpegEncTargetSize[id] - gJpegEncTargetSize[id] * DAL_JPEGENC_LOWER_BOUND_RATIO / 100;
		gJpegEncUpperBoundSize[id] = gJpegEncTargetSize[id] + gJpegEncTargetSize[id] * DAL_JPEGENC_UPPER_BOUND_RATIO / 100;
		gJpegEncBrcCfg[id].initial_qf = gJpegEncBrcQF[id];
		gJpegEncBrcCfg[id].target_size = gJpegEncTargetSize[id];
		gJpegEncBrcCfg[id].lowbound_size = gJpegEncLowerBoundSize[id];
		gJpegEncBrcCfg[id].upbound_size = gJpegEncUpperBoundSize[id];
		gJpegEncBrcCfg[id].width = gJpegEncInit[id].uiWidth;
		gJpegEncBrcCfg[id].height = gJpegEncInit[id].uiHeight;
		gJpegEncBrcCfg[id].max_retry = 0;
	}

#if DAL_VDOENC_SHOW_MSG
	DBG_DUMP("[JPEGENC][%d] Init Codec Width = %d, Height = %d, BitRate = %d, FrameRate = %d, YuvFormat = %d\r\n",
			(int)id,
			(int)gJpegEncInit[id].uiWidth,
			(int)gJpegEncInit[id].uiHeight,
			(int)gJpegEncInit[id].uiByteRate*8,
			(int)gJpegEncInit[id].uiFrameRate,
			(int)gJpegEncInit[id].uiYuvFormat);
#endif

	return E_OK;
}

ER dal_jpegenc_getinfo(DAL_VDOENC_ID id, DAL_VDOENC_GET_ITEM item, UINT32 *pvalue) {
	ER resultV = E_OK;

	switch (item) {
	case DAL_VDOENC_GET_ISIFRAME:
		if (gJpegEncVidNum[id] % gJpegEncGopNum[id] == 0) {
			*pvalue = 1;
		} else {
			*pvalue = 0;
		}
		resultV = E_OK;
		break;
	default:
		resultV = E_PAR;
		break;
	}

	return resultV;
}

ER dal_jpegenc_setinfo(DAL_VDOENC_ID id, DAL_VDOENC_SET_ITEM item, UINT32 value) {
	return E_OK;
}

ER dal_jpegenc_encodeone(DAL_VDOENC_ID id, DAL_VDOENC_PARAM *pparam) {
	ER              ER_Code;
	UINT32          thisSize, outsize;
	//#NT#2012/09/14#Ben Wang -begin
	//#NT#Refine UserJpg
	JPG_ENC_CFG     jpgCmpParam;
	UINT32          HeaderSize, JPGFileSize = 0; //coverity 44579

	memset((void *)&jpgCmpParam, 0, sizeof(JPG_ENC_CFG));
	gJpegEncVidNum[id] += 1;
	jpg_async_enc_open();
	jpg_enc_setdata(id, JPG_BRC_STD_QTABLE_QUALITY, gJpegEncStdQTableQuality[id]);
	jpg_set_bitrate_ctrl(&gJpegEncBrcCfg[id]);

	if (pparam->uiQuality == 0) {
		pparam->uiQuality = 80;
	}

	if (gJpegEncTargetSize[id]) {
		jpg_async_set_brc_qtable(gJpegEncBrcQF[id], gJpegEncQTableY[id], gJpegEncQTableUV[id]);
	} else {
		jpg_async_set_qtable(pparam->uiQuality, gJpegEncQTableY[id], gJpegEncQTableUV[id]);
	}

	jpgCmpParam.image_addr[0] = pparam->uiYAddr;
	jpgCmpParam.image_addr[1] = pparam->uiUVAddr;
	//jpgCmpParam.image_addr[2] = gJpegEncInit[id].crAddr;
	jpgCmpParam.bs_addr = pparam->uiBsAddr; 		//Compress Bitstream location,

	if (pparam->uiBsEndAddr <= pparam->uiBsAddr) {
		DBG_ERR("[JPEGENC][%d] invalid param, enc bs start addr = 0x%08x, end addr = 0x%08x\n\r", (int)id, (unsigned int)pparam->uiBsAddr, (unsigned int)pparam->uiBsEndAddr);
		jpg_async_enc_close();
		return E_PAR;
	}

	outsize = pparam->uiBsEndAddr - pparam->uiBsAddr;
	if (outsize > DAL_JPEG_MAX_BSLENGTH) {
		outsize = DAL_JPEG_MAX_BSLENGTH;
	}
	jpgCmpParam.p_bs_buf_size = (UINT32 *)&outsize;
	//#NT#2012/06/22#Hideo Lin -begin
	//#NT#Modify for MJPEG movie

	jpgCmpParam.width  = gJpegEncInit[id].uiWidth;
	jpgCmpParam.height = gJpegEncInit[id].uiHeight;
	jpgCmpParam.yuv_fmt = gJpegEncInit[id].uiYuvFormat; //YUV format Calvin 20130917 //JPG_FMT_YUV420; // [TEST]: fixed temporary
	//#NT#2012/06/22#Hideo Lin -end

	jpgCmpParam.lineoffset[0] = pparam->uiYLineOffset;
	jpgCmpParam.lineoffset[1] = pparam->uiUVLineOffset;
	//DBG_IND("yline = %d, uv=%d!\r\n", (int)(jpgCmpParam.LineOffsetY), (int)(jpgCmpParam.LineOffsetCbCr));
	jpgCmpParam.header_form = JPG_HEADER_QV5;

	//if(gMovMjpgRecParam.bUserDataEmbed)
	//    ER_Code = JpegEncMovMjpgOnePic(&gMovMjpgRec_JPGParam, gMOVMJPG_QTable, JPEG_HDR_HUFFMANTBL_WITH);
	//else
	HeaderSize = jpg_async_enc_setheader(&jpgCmpParam, gJpegEncQTableY[id], gJpegEncQTableUV[id]);
	ER_Code = jpg_async_enc_start(jpgCmpParam.bs_addr + HeaderSize, outsize - HeaderSize, jpgCmpParam.image_addr[0], jpgCmpParam.image_addr[1]);
	if (E_OK == ER_Code) {
		ER_Code = jpg_async_enc_waitdone(&JPGFileSize);
	}

#if 0
	//DBG_DUMP("^MBS=%3dK,Q=%3d", JPGFileSize/1024,gJpegEncBrcQF[id]);
	//DBG_DUMP("Q[%d]QF[%3d],%dK->%dK",gJpegEncStdQTableQuality[id],gJpegEncBrcQF[id],JPGFileSize/1024,gJpegEncTargetSize[id]/1024);
	if (gJpegEncTargetSize[id] && (JPGFileSize > gJpegEncUpperBoundSize[id] || JPGFileSize < gJpegEncLowerBoundSize[id])) {
		//#NT#2015/11/11#Ben Wang#[0086711] -begin
		UINT32  uiNewQValue = 0, uiTargetQF;
#if DAL_JPEGENC_ADAPTIVE_Q_TABLE_FLOW
		if (JPGFileSize > gJpegEncUpperBoundSize[id] && gJpegEncBrcQF[id] == JPGBRC_QUALITY_MAX) {
			if (gJpegEncStdQTableQuality[id] >= DAL_JPEGENC_ADAPTIVE_FINE_TUNE_VALUE) {
				gJpegEncStdQTableQuality[id] -= DAL_JPEGENC_ADAPTIVE_FINE_TUNE_VALUE;
			}
			if (gJpegEncStdQTableQuality[id] < DAL_JPEGENC_ADAPTIVE_MIN_Q_QUALITY) {
				gJpegEncStdQTableQuality[id] = DAL_JPEGENC_ADAPTIVE_MIN_Q_QUALITY;
			}

			DBG_IND("StdQTable=>%d\r\n", (int)(gJpegEncStdQTableQuality[id]));
		} else if (JPGFileSize < gJpegEncLowerBoundSize[id] && gJpegEncBrcQF[id] == JPGBRC_QUALITY_MIN) {
			gJpegEncStdQTableQuality[id] += DAL_JPEGENC_ADAPTIVE_FINE_TUNE_VALUE;
			if (gJpegEncStdQTableQuality[id] > DAL_JPEGENC_ADAPTIVE_MAX_Q_QUALITY) {
				gJpegEncStdQTableQuality[id] = DAL_JPEGENC_ADAPTIVE_MAX_Q_QUALITY;
			}

			DBG_IND("StdQTable=>%d\r\n", (int)(gJpegEncStdQTableQuality[id]));
		} else
#endif
		{
			uiTargetQF = gJpegEncBrcQF[id];
			if (JPGFileSize > HeaderSize) { // fix coverity[14007] 2017/01/20
				if (jpgCmpParam.yuv_fmt == JPG_FMT_YUV420) {
					jpg_get_brc_qf(JPGBRC_COMBINED_YUV420, &uiTargetQF, (JPGFileSize - HeaderSize));
				} else if (jpgCmpParam.yuv_fmt == JPG_FMT_YUV100) { // support Y only pattern (Ben 2013/09/16)
					jpg_get_brc_qf(JPGBRC_YUV100, &uiTargetQF, (JPGFileSize - HeaderSize));
				} else {
					jpg_get_brc_qf(JPGBRC_COMBINED_YUV422, &uiTargetQF, (JPGFileSize - HeaderSize));
				}
			}

			if (uiTargetQF < gJpegEncBrcQF[id]) {
				uiNewQValue = gJpegEncBrcQF[id] - (gJpegEncBrcQF[id] - uiTargetQF) * gJpegEncBrcWeighting[id] / DAL_JPEGENC_MAX_BRC_WEIGHTING;

			} else {
				//#NT#2011/08/12#Ben Wang -begin
				//#NT#Make weighting mechanism valid only when QF is decaying.
#if 1//(BRC_WEIGHTING_VALID_IN_DECAY_ONLY)
				uiNewQValue = uiTargetQF;
#else
				uiNewQValue = gJpegEncBrcQF[id] + (uiTargetQF - gJpegEncBrcQF[id]) * gJpegEncBrcWeighting[id] / DAL_JPEGENC_MAX_BRC_WEIGHTING;
#endif
				//#NT#2011/08/12#Ben Wang -end
			}
			gJpegEncBrcQF[id] = uiNewQValue;
		}
		//#NT#2015/11/11#Ben Wang#[0086711] -end
		//DBG_DUMP("->%3d->%3d", uiNewQValue, uiTargetQF);
	}
#endif
	//DBG_DUMP("\r\n");
	//ER_Code = JpegEncOneVideoFrame(&jpgCmpParam, gJpegEncQTableY[id], JPEG_HDR_HUFFMANTBL_WITH);
	jpg_async_enc_close();

	if (ER_Code == E_OK)
		;//DBG_IND("JPEG OK\n\r");
	else {
		DBG_ERR("[JPEGENC][%d] Encode Failed\n\r", (int)(id));
	}

	DBG_IND("Jpeg size = 0x%X\n\r", (unsigned int)(JPGFileSize));

	thisSize = JPGFileSize;//ffD8 to FFD9(included)
	thisSize = (thisSize + 3) & 0xFFFFFFFC; //align to 4

	//pparam->thisSize = thisSize;
	pparam->uiBsSize = thisSize;
	//#NT#2012/09/14#Ben Wang -end
	return ER_Code;
}

ER dal_jpegenc_close(DAL_VDOENC_ID id) {
	DBG_DUMP("[JPEGENC][%d] close\r\n", (int)(id));
	gJpegEncVidNum[id] = 0;
	return E_OK;
}


#if defined __KERNEL__
EXPORT_SYMBOL(dal_jpegenc_init);
EXPORT_SYMBOL(dal_jpegenc_getinfo);
EXPORT_SYMBOL(dal_jpegenc_setinfo);
EXPORT_SYMBOL(dal_jpegenc_encodeone);
EXPORT_SYMBOL(dal_jpegenc_close);
#endif

