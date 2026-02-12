/*
    Media plug-in for H.265

    Media plug-in H.265 re-order functions

    @file       MP_H265ReOrder.c
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

/** \addtogroup */
//@{

#include "mp_h265_reorder.h"
#ifdef __KERNEL__
#include <linux/module.h>
#endif

#define __MODULE__          h265order
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int h265order_debug_level = NVT_DBG_WRN;
#ifdef __KERNEL__
module_param_named(debug_level_h265order, h265order_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_h265order, "mp_h265_reorder debug level");
#else
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#endif

//-------------------------------------------------------------------------------------------------
// Decode
//static H265_IMG_DATA        g_H265DecImg[H265DEC_IMG_BUF_NUM];          // the image buffer address for H.265 decoding
static H265DEC_IMG_QUEUE    g_H265DecImgQueue[H265DEC_GOP_30];         // the image queue for H.265 decoding//coverity 60478
static UINT32               g_uiH265DecGopNum = H265DEC_GOP_MAX;        // Frame number in one GOP
static UINT32               g_uiH265DecGopType = H265DEC_GOP_IPONLY;    // GOP type
static UINT32               g_uiH265DecNextImgToQueue = 0;              // next image number for adding to queue

//------------------------------------
// Decode Order  0 1 2 3 4 5 6 7 8 ...
// Decode Type   I B B P B B P B B ...
// Display Order 1 2 0 4 5 3 7 8 6 ...
// Display Type  B B I B B P B B P ...
//------------------------------------
// re-order array for displaying
static UINT32   g_H265DecImgOrderGop15[H265DEC_GOP_15] = {
	1, 2, 0, 4, 5, 3, 7, 8, 6, 10, 11, 9, 13, 14, 12
};

// decode type
//static UINT32   g_H265DecImgTypeGop15[H265DEC_GOP_15] = {
//    I_SLICE, B_SLICE, B_SLICE, P_SLICE, B_SLICE, B_SLICE, P_SLICE, B_SLICE, B_SLICE, P_SLICE, B_SLICE, B_SLICE, P_SLICE, B_SLICE, B_SLICE};

//#NT#2013/02/25#Calvin Chang#Support four interlaced B/B fields reorder case -begin
// re-order array for encoding
static UINT32   g_H265DecImgOrderGop30_4IBB[H265DEC_GOP_30] = {
	2,  3,  4,  5,  0,  1,  8,  9, 10, 11,  6,  7,
	14, 15, 16, 17, 12, 13, 20, 21, 22, 23, 18, 19,
	26, 27, 28, 29, 24, 25
};
//#NT#2013/02/25#Calvin Chang -end


//-------------------------------------------------------------------------------------------------
//
// Set frame number in one GOP for H.265 decoding
//
//-------------------------------------------------------------------------------------------------
void MP_H265Dec_SetGopNum(UINT32 uiGopNum)
{
	g_uiH265DecGopNum = uiGopNum;
}

//-------------------------------------------------------------------------------------------------
//
// Get frame number in one GOP for H.265 decoding
//
//-------------------------------------------------------------------------------------------------
UINT32 MP_H265Dec_GetGopNum(void)
{
	return g_uiH265DecGopNum;
}

//-------------------------------------------------------------------------------------------------
//
// Set GOP type for H.265 decoding
//
//-------------------------------------------------------------------------------------------------
void MP_H265Dec_SetGopType(UINT32 uiGopType)
{
	g_uiH265DecGopType = uiGopType;
}

//-------------------------------------------------------------------------------------------------
//
// Get GOP type for H.265 decoding
//
//-------------------------------------------------------------------------------------------------
UINT32 MP_H265Dec_GetGopType(void)
{
	return g_uiH265DecGopType;
}

//-------------------------------------------------------------------------------------------------
//
// H.265 decode display image queue initialization
//
//-------------------------------------------------------------------------------------------------
void MP_H265Dec_ImgQueueInit(UINT32 uiGopType)
{
	UINT32  i;

	for (i = 0; i < g_uiH265DecGopNum; i++) {
		g_H265DecImgQueue[i].ImgData.uiYAddr    = 0;
		g_H265DecImgQueue[i].ImgData.uiUVAddr   = 0;
		g_H265DecImgQueue[i].uiFrameType        = 0;
		g_H265DecImgQueue[i].bIsReady           = FALSE;
	}

	// reset order information of image queue
	if (uiGopType == H265DEC_GOP_IPONLY) { // I, P only
		for (i = 0; i < g_uiH265DecGopNum; i++) {
			g_H265DecImgQueue[i].uiOrder = i;
		}
	}
	//#NT#2013/02/25#Calvin Chang#Support four interlaced B/B fields reorder case -begin
	else if (uiGopType == H265DEC_GOP_IPB_1ST30_4BB) {
		if (g_uiH265DecGopNum != H265DEC_GOP_30) {
			DBG_ERR("With 4BB fields, but the number in 1 GOP is not 30!\r\n");
		}

		for (i = 0; i < g_uiH265DecGopNum; i++) {
			g_H265DecImgQueue[i].uiOrder = g_H265DecImgOrderGop30_4IBB[i];
		}
	}
	//#NT#2013/02/25#Calvin Chang -end
	else { // I, P, B
		if ((g_uiH265DecGopNum != H265DEC_GOP_15) && (g_uiH265DecGopNum != H265DEC_GOP_12)) {
			DBG_ERR("With B frame, but frame number in 1 GOP is not 15 or 12!\r\n");
		}

		for (i = 0; i < g_uiH265DecGopNum; i++) {
			g_H265DecImgQueue[i].uiOrder = g_H265DecImgOrderGop15[i];
		}
	}

	g_uiH265DecNextImgToQueue = 0;
}

//-------------------------------------------------------------------------------------------------
//
// Add image to queue for decoding
//
//-------------------------------------------------------------------------------------------------
//#NT#2012/05/08#Calvin Chang#support Fowrad/Backward mechanism -begin
UINT32 MP_H265Dec_AddImgToQueue(PH265_IMG_DATA pImgData, UINT32 uiFrameType, UINT32 uiFrameNum)
//UINT32 MP_H265Dec_AddImgToQueue(PH265_IMG_DATA pImgData, UINT32 uiFrameType)
//#NT#2012/05/08#Calvin Chang -end
{
	//#NT#2012/05/08#Calvin Chang#support Fowrad/Backward mechanism -begin
	g_uiH265DecNextImgToQueue = (uiFrameNum % g_uiH265DecGopNum);
	//#NT#2012/05/08#Calvin Chang -end

	if (g_H265DecImgQueue[g_uiH265DecNextImgToQueue].bIsReady) {
		DBG_ERR("Add [%d] to queue, but was ready!\r\n", g_uiH265DecNextImgToQueue);
		return 0;
	}

	g_H265DecImgQueue[g_uiH265DecNextImgToQueue].ImgData.uiYAddr    = pImgData->uiYAddr;
	g_H265DecImgQueue[g_uiH265DecNextImgToQueue].ImgData.uiUVAddr   = pImgData->uiUVAddr;
	g_H265DecImgQueue[g_uiH265DecNextImgToQueue].uiFrameType        = uiFrameType;
	g_H265DecImgQueue[g_uiH265DecNextImgToQueue].bIsReady           = TRUE;

#if 1
	switch (uiFrameType) {
	case MP_VDODEC_B_SLICE:
		DBG_IND("Add [%d] to queue, Y=0x%X, UV=0x%X, type=%d (B)\r\n", g_uiH265DecNextImgToQueue, pImgData->uiYAddr, pImgData->uiUVAddr, uiFrameType);
		break;

	case MP_VDODEC_P_SLICE:
		DBG_IND("Add [%d] to queue, Y=0x%X, UV=0x%X, type=%d (P)\r\n", g_uiH265DecNextImgToQueue, pImgData->uiYAddr, pImgData->uiUVAddr, uiFrameType);
		break;

	default:
		DBG_IND("Add [%d] to queue, Y=0x%X, UV=0x%X, type=%d (I)\r\n", g_uiH265DecNextImgToQueue, pImgData->uiYAddr, pImgData->uiUVAddr, uiFrameType);
		break;
	}
#else
	DBG_IND("Add [%d] to queue, Y=0x%X, UV=0x%X, type=%d\r\n", g_uiH265DecNextImgToQueue, pImgData->uiYAddr, pImgData->uiUVAddr, uiFrameType);
#endif


	//#NT#2012/05/08#Calvin Chang#support Fowrad/Backward mechanism -begin
#if 0 // new solution no use
	if (g_uiH265DecNextImgToQueue == (g_uiH265DecGopNum - 1)) {
		g_uiH265DecNextImgToQueue = 0;
	} else {
		g_uiH265DecNextImgToQueue++;
	}
#endif
	//#NT#2012/05/08#Calvin Chang -end

	return g_uiH265DecNextImgToQueue;
}

//-------------------------------------------------------------------------------------------------
//
// Get image address from queue by order
//
//-------------------------------------------------------------------------------------------------
#if 0
void MP_H265Dec_GetImgFromQueue(PH265_IMG_DATA pImgData, UINT32 uiOrder)
{
	UINT32  uiYAddr, uiUVAddr;

	uiYAddr  = g_H265DecImgQueue[uiOrder].ImgData.uiYAddr;
	uiUVAddr = g_H265DecImgQueue[uiOrder].ImgData.uiUVAddr;

	if (uiYAddr == 0) {
		DBG_ERR("Get [%d] from queue, but was empty!\r\n", uiOrder);
	} else if (uiYAddr == H265DEC_FAKE_B_ADDR) {
		g_H265DecImgQueue[uiOrder].ImgData.uiYAddr  = 0;
		g_H265DecImgQueue[uiOrder].ImgData.uiUVAddr = 0;
		g_H265DecImgQueue[uiOrder].bIsReady = FALSE;
	}

	pImgData->uiYAddr  = uiYAddr;
	pImgData->uiUVAddr = uiUVAddr;
}
#endif

//-------------------------------------------------------------------------------------------------
//
// Release image from queue after displaying
//
//-------------------------------------------------------------------------------------------------
UINT32 MP_H265Dec_FreeImgFromQueue(UINT32 uiOrder)
{
	if (g_H265DecImgQueue[uiOrder].ImgData.uiYAddr == 0) {
		DBG_ERR("Free [%d] from queue, but was empty!\r\n", uiOrder);
		return 0;
	}

	DBG_IND("Free [%d] from queue, Y=0x%X, UV=0x%X\r\n", uiOrder, g_H265DecImgQueue[uiOrder].ImgData.uiYAddr, g_H265DecImgQueue[uiOrder].ImgData.uiUVAddr);

	g_H265DecImgQueue[uiOrder].ImgData.uiYAddr  = 0;
	g_H265DecImgQueue[uiOrder].ImgData.uiUVAddr = 0;
	g_H265DecImgQueue[uiOrder].bIsReady = FALSE;

	return uiOrder;
}

//-------------------------------------------------------------------------------------------------
//
// Get image number by re-order
//
//-------------------------------------------------------------------------------------------------
static UINT32 MP_H265Dec_GetReOrderNum(UINT32 uiFrameNum)
{
	UINT32  uiOrder;

	uiOrder = g_H265DecImgQueue[uiFrameNum % g_uiH265DecGopNum].uiOrder;

	DBG_IND("Re-order: in %d, out %d\r\n", uiFrameNum, uiOrder);

	return uiOrder;
}

//-------------------------------------------------------------------------------------------------
//
// Get re-order image information from image queue
//
//-------------------------------------------------------------------------------------------------
void MP_H265Dec_GetReOrderImg(UINT32 uiFrameNum, PH265DEC_IMG_QUEUE pImgQueueData)
{
	UINT32  uiReOrder;

	// get re-order number
	uiReOrder = MP_H265Dec_GetReOrderNum(uiFrameNum);

	pImgQueueData->ImgData.uiYAddr  = g_H265DecImgQueue[uiReOrder].ImgData.uiYAddr;
	pImgQueueData->ImgData.uiUVAddr = g_H265DecImgQueue[uiReOrder].ImgData.uiUVAddr;
	pImgQueueData->uiFrameType      = g_H265DecImgQueue[uiReOrder].uiFrameType;
	pImgQueueData->uiOrder          = uiReOrder;
	pImgQueueData->bIsReady         = g_H265DecImgQueue[uiReOrder].bIsReady;

	if (pImgQueueData->ImgData.uiYAddr == 0) {
		DBG_ERR("Get [%d] from queue, but was empty!\r\n", uiReOrder);
	}

#if 1
	switch (pImgQueueData->uiFrameType) {
	case MP_VDODEC_B_SLICE:
		DBG_IND("Get [%d], order=%d, Y=0x%X, UV=0x%X, type=%d (B)\r\n", uiFrameNum, uiReOrder, pImgQueueData->ImgData.uiYAddr, pImgQueueData->ImgData.uiUVAddr, pImgQueueData->uiFrameType);
		break;

	case MP_VDODEC_P_SLICE:
		DBG_IND("Get [%d], order=%d, Y=0x%X, UV=0x%X, type=%d (P)\r\n", uiFrameNum, uiReOrder, pImgQueueData->ImgData.uiYAddr, pImgQueueData->ImgData.uiUVAddr, pImgQueueData->uiFrameType);
		break;

	default:
		DBG_IND("Get [%d], order=%d, Y=0x%X, UV=0x%X, type=%d (I)\r\n", uiFrameNum, uiReOrder, pImgQueueData->ImgData.uiYAddr, pImgQueueData->ImgData.uiUVAddr, pImgQueueData->uiFrameType);
		break;
	}
#else
	DBG_IND("Get [%d], order=%d, Y=0x%X, UV=0x%X, type=%d\r\n", uiFrameNum, uiReOrder, pImgQueueData->ImgData.uiYAddr, pImgQueueData->ImgData.uiUVAddr, pImgQueueData->uiFrameType);
#endif
}

//#NT#2012/05/10#Calvin Chang#support Fowrad/Backward mechanism -begin
UINT32 MP_H265Dec_Flush_ImgQueue(UINT32 uiFrameNum)
{
	UINT32  uiReOrder, i;

	// get re-order number
	uiReOrder = MP_H265Dec_GetReOrderNum(uiFrameNum);

	if (g_H265DecImgQueue[uiReOrder].bIsReady == FALSE) {
		DBG_IND("Current display frame %d is not ready!\r\n", uiFrameNum);
		return 0;
	}

	for (i = 0; i < g_uiH265DecGopNum; i++) {
		if (i == uiReOrder) { // Keep current image
			continue;
		}

		g_H265DecImgQueue[i].bIsReady = FALSE;
	}

	return uiReOrder;
}

void MP_H265Dec_Reset_ImgQueue_Order(UINT32 uiGopType)
{
	UINT32 i;

	if (uiGopType == H265DEC_GOP_IPONLY) { // I, P only
		for (i = 0; i < g_uiH265DecGopNum; i++) {
			g_H265DecImgQueue[i].uiOrder = i;
		}
	}
	//#NT#2013/02/25#Calvin Chang#Support four interlaced B/B fields reorder case -begin
	else if (uiGopType == H265DEC_GOP_IPB_1ST30_4BB) {
		if (g_uiH265DecGopNum != H265DEC_GOP_30) {
			DBG_ERR("With 4BB fields, but the number in 1 GOP is not 30!\r\n");
		}

		for (i = 0; i < g_uiH265DecGopNum; i++) {
			g_H265DecImgQueue[i].uiOrder = g_H265DecImgOrderGop30_4IBB[i];
		}
	}
	//#NT#2013/02/25#Calvin Chang -end
	else { // I, P, B
		if ((g_uiH265DecGopNum != H265DEC_GOP_15) && (g_uiH265DecGopNum != H265DEC_GOP_12)) {
			DBG_ERR("With B frame, but frame number in 1 GOP is not 15 or 12!\r\n");
		}

		for (i = 0; i < g_uiH265DecGopNum; i++) {
			g_H265DecImgQueue[i].uiOrder = g_H265DecImgOrderGop15[i];
		}
	}
}
//#NT#2012/05/10#Calvin Chang -end

//@}
