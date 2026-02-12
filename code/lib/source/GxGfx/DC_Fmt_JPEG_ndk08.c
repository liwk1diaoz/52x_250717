
#include "GxGfx/GxGfx.h"
#include "GxGfx_int.h"
#include "DC.h"
#include "GxImageJPG.h"

//for JPEG func
#include "SysKer.h"
#include <JpgDec.h>

BOOL Jpeg_DecodeInfo(const UINT8 *JpegBitStream, UINT32 uiBitStreamSize, UINT16 *pW,  UINT16 *pH, UINT16 *pFMT)
{
	INT32      r;
	JPGHEAD_DEC_CFG     param;

	//memset(&param, 0x00, sizeof(JPGHEAD_DEC_CFG));

	DBG_IND("JPG src=%08x\r\n", (UINT32)JpegBitStream);
	DBG_IND("JPG size=%08x\r\n", uiBitStreamSize);

	param.inbuf = (UINT8 *)(UINT32)JpegBitStream;
	param.rstinterval = 0;
	// get primary-image Info in Exif-JPEG
	r = (UINT32)Jpg_ParseHeader(&param, DEC_PRIMARY);
	if (r) {
		DBG_ERR("JPG header err=%08x\r\n", r);
		return FALSE;
	}

	*pW = param.imagewidth;
	*pH = param.imageheight;

#if (GX_SUPPORT_DCFMT_UVPACK)
	if (param.fileformat == JPG_FMT_YUV211) {
		*pFMT = PXLFMT_YUV422_PK;
	} else if (param.fileformat == JPG_FMT_YUV420) {
		*pFMT = PXLFMT_YUV420_PK;
	}
#else
	if (param.fileformat == JPG_FMT_YUV211) {   // 0x21->YUV422
		*pFMT = PXLFMT_YUV422;
	} else if (param.fileformat == JPG_FMT_YUV420) { // 0x22->YUV420
		*pFMT = PXLFMT_YUV420;
	}
	//else if(param.fileformat==JPG_FMT_YUV444) // 0x11->YUV444
	//{
	//  *pFMT = PXLFMT_YUV444;
	//}
#endif
	DBG_IND("JPG w,h,fmt=%d,%d,%08x\r\n", *pW, *pH, (UINT32)*pFMT);

	return TRUE;
}

//condition : pBuf must 32 byte alignment
BOOL Jpeg_DecodeImage(const UINT8 *JpegBitStream, UINT32 uiBitStreamSize, UINT8 *pBuf, UINT32 uiBufSize)
{
	ER r;
	JPGHEAD_DEC_CFG     param;
	JPG_DEC_INFO         info;

	//memset(&param, 0x00, sizeof(JPGHEAD_DEC_CFG));
	DBG_IND("JPG src=%08x\r\n", (UINT32)JpegBitStream);
	DBG_IND("JPG dst=%08x\r\n", (UINT32)pBuf);
	DBG_IND("JPG size=%08x\r\n", uiBitStreamSize);
	DBG_IND("JPG data=%02x-%02x-%02x-%02x\r\n", JpegBitStream[0], JpegBitStream[1], JpegBitStream[2], JpegBitStream[3]);
	DBG_IND("JPG data2=%02x-%02x-%02x-%02x\r\n", JpegBitStream[4], JpegBitStream[5], JpegBitStream[6], JpegBitStream[7]);
	info.JPGFileSize        = uiBitStreamSize;
	info.jdcfg_p            = &param;
	info.pSrcAddr           = (UINT8 *)(UINT32)JpegBitStream;
	info.pDstAddr           = (UINT8 *)pBuf;
	info.pJPGScalarHandler  = 0;
	info.DecodeType      = DEC_PRIMARY;
	info.JPGFileSize        = uiBitStreamSize;
	info.bEnableTimeOut     = FALSE;
	info.bEnableCrop        = FALSE;

	r = Jpg_DecOneJPG(&info);
	if (r != E_OK) {
		DBG_ERR("JPG err=%08x\r\n", r);
		return FALSE;
	}

	return TRUE;
}

BOOL Jpeg_GetImageAddr(UINT16 w, UINT16 h, UINT16 fmt, UINT8 *pBuf, UINT32 *uiOutAddrY, UINT32 *uiOutAddrCb, UINT32 *uiOutAddrCr)
{
	(*uiOutAddrY) = (UINT32)pBuf;
	DBG_IND("JPG dst Y=%08x\r\n", (UINT32)(*uiOutAddrY));
	(*uiOutAddrCb) = (*uiOutAddrY) + w * h;
	DBG_IND("JPG dst CB=%08x\r\n", (UINT32)(*uiOutAddrCb));
#if (GX_SUPPORT_DCFMT_UVPACK)
	if (fmt == PXLFMT_YUV422_PK) {
		(*uiOutAddrCr) = (*uiOutAddrCb) + 1;
	} else if (fmt == PXLFMT_YUV420_PK) {
		(*uiOutAddrCr) = (*uiOutAddrCb) + 1;
	}
#else
	if (fmt == PXLFMT_YUV422) {   // 0x21->YUV422
		(*uiOutAddrCr) = (*uiOutAddrCb) + w * h / 2;
	} else if (fmt == PXLFMT_YUV420) { // 0x22->YUV420
		(*uiOutAddrCr) = (*uiOutAddrCb) + w * h / 4;
	}
#endif
	DBG_IND("JPG dst CR=%08x\r\n", (UINT32)(*uiOutAddrCr));

	return TRUE;
}

