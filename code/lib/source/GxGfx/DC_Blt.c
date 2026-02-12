
#include "GxGfx/GxGfx.h"
#include "DC.h"
#include "DC_Blt.h"
#include "GxGfx_int.h"
#include "GxPixel.h"
#include "GxDCDraw.h"

#define COLOR_YUV_EMPTY         COLOR_YUV_BLACK
#define COLOR_RGB_EMPTY         COLOR_RGB_BLACK
//#define COLOR_YUV_SOLID         COLOR_YUV_WHITE
//#define COLOR_RGB_SOLID         COLOR_RGB_WHITE


//====================
//  API
//====================
//dc clear/copy/convert
RESULT GxGfx_Clear(DC *pDestDC);
RESULT GxGfx_Copy(DC *pDestDC, const DC *pSrcDC);
RESULT GxGfx_Convert(DC *pDestDC, const DC *pSrcDC, const void *pConvertTable);


//--------------------------------------------------------------------------------------
//  low level
//--------------------------------------------------------------------------------------
DC_CTRL _gxgfx_ctrl = {0, 0, 0, 0};

//dc (low level) - not support xform/scaling
void _GxGfx_SetClip(const IRECT *pClipRect)
{
	_gxgfx_ctrl.pClipRect = pClipRect;
}
void _GxGfx_SetImageROP(UINT32 uiRop, UINT32 uiParam, const PALETTE_ITEM *pColorTable, const MAPPING_ITEM *pIndexTable)
{
	_gxgfx_ctrl.uiRop = uiRop;
	_gxgfx_ctrl.uiParam = uiParam;
	_gxgfx_ctrl.pColorTable = pColorTable;
	_gxgfx_ctrl.pIndexTable = pIndexTable;
}

//--------------------------------------------------------------------------------------
//  PIXEL_2D
//--------------------------------------------------------------------------------------
extern PIXEL_2D _gPixel2D_INDEXn;
extern PIXEL_2D _gPixel2D_INDEX8;
extern PIXEL_2D _gPixel2D_RGB888;
#if (GX_SUPPORT_DCFMT_RGB565)
extern PIXEL_2D _gPixel2D_RGB565;
extern PIXEL_2D _gPixel2D_RGBA5658;
extern PIXEL_2D _gPixel2D_RGBA5654;
#endif
extern PIXEL_2D _gPixel2D_RGBA8888;
#if (GX_SUPPORT_DCFMT_RGB444)
extern PIXEL_2D _gPixel2D_RGBA4444;
extern PIXEL_2D _gPixel2D_RGBA5551;
#endif
BOOL PIXEL_2D_Init(PIXEL_2D *p, DC *_pDC)
{
	DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;

	switch (pDC->uiPxlfmt) {
	case PXLFMT_INDEX1 :
	case PXLFMT_INDEX2 :
	case PXLFMT_INDEX4 :
		*p = _gPixel2D_INDEXn;
		break;
	case PXLFMT_INDEX8 :
		*p = _gPixel2D_INDEX8;
		break;
#if (GX_SUPPPORT_YUV ==ENABLE)
	case PXLFMT_RGB888 :
	case PXLFMT_YUV444 :
	case PXLFMT_YUV422 :
	case PXLFMT_YUV421 :
	case PXLFMT_YUV420 :
		*p = _gPixel2D_RGB888;
		break;
#if (GX_SUPPORT_DCFMT_UVPACK)
	case PXLFMT_YUV422_PK:
	case PXLFMT_YUV421_PK:
	case PXLFMT_YUV420_PK:
		*p = _gPixel2D_RGB888;
		break;
#endif
#endif

#if (GX_SUPPORT_DCFMT_RGB565)
	case PXLFMT_RGB565_PK :
		*p = _gPixel2D_RGB565;
		break;
	case PXLFMT_RGBA5658_PK :
		*p = _gPixel2D_RGBA5658;
		break;
	case PXLFMT_RGBA5654_PK :
		*p = _gPixel2D_RGBA5654;
		break;
#endif
	case PXLFMT_RGBA8888_PK :
		*p = _gPixel2D_RGBA8888;
		break;
#if (GX_SUPPORT_DCFMT_RGB444)
	case PXLFMT_RGBA4444_PK :
		*p = _gPixel2D_RGBA4444;
		break;
	case PXLFMT_RGBA5551_PK :
		*p = _gPixel2D_RGBA5551;
		break;
#endif
	default:
		return FALSE;
	}
	p->pDC = _pDC;
	return TRUE;
}


//--------------------------------------------------------------------------------------
//  DC clear
//--------------------------------------------------------------------------------------


RESULT GxGfx_Clear(DC *_pDestDC)
{
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT DestRect;
	BOOL bINDEX;
	BOOL bPKRGB, bPKRGB2, bPKRGB3, bPKRGB4;

	UINT32 uiClearColor;
	IRECT cw;
	UINT32 fs;
	RESULT r;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	//GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	if ((pDestDC->uiFlag & DCFLAG_SIGN_MASK) != DCFLAG_SIGN) {
		//DBG_ERR("GxGfx_Clear - ERROR! Not init DC.\r\n");
		return GX_ERROR_INITDEVICE;
	}
	if (_pDestDC->uiType == TYPE_JPEG) {
		DBG_ERR("GxGfx_Clear - ERROR! Not support format.\r\n");
		return GX_ERROR_FORMAT;
	}

	bINDEX = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
	bPKRGB = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK);
	bPKRGB2 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK2);
	bPKRGB3 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK3);
	bPKRGB4 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK4);

	if (bINDEX) {
		uiClearColor = 0;
	} else if (bPKRGB) {
		uiClearColor = COLOR_RGB_EMPTY;
	}
#if (GX_SUPPORT_DCFMT_RGB565)
	else if (bPKRGB2) {
		uiClearColor = COLOR_RGBA_2_RGB565(COLOR_RGB_EMPTY);
	}
#endif
#if (GX_SUPPORT_DCFMT_RGB444)
	else if (bPKRGB3) {
		uiClearColor = COLOR_RGBA_2_ARGB4444(COLOR_RGB_EMPTY);
	} else if (bPKRGB4) {
		uiClearColor = COLOR_RGBA_2_ARGB1555(COLOR_RGB_EMPTY);
	}
#endif

	else {
		DBG_ERR("GxGfx_Clear - ERROR! Not support format.\r\n");
		return GX_ERROR_FORMAT;
	}

	DestRect = _DC_GetRealRect(_pDestDC);

	fs = GxGfx_Get(BR_SHAPEFILLSTYLE);
	GxGfx_Set(BR_SHAPEFILLSTYLE, FILLSTYLE_FILL); //fill all channel

	cw = GxGfx_GetWindow(_pDestDC);
	_DC_XFORM2_RECT(_pDestDC, &cw);
	_GxGfx_SetClip(0); //disable
	r = _GxGfx_FillRect(_pDestDC, &DestRect, uiClearColor);
	_GxGfx_SetClip(&cw); //restore

	GxGfx_Set(BR_SHAPEFILLSTYLE, fs); //restore
	return r;
}

RESULT GxGfx_Copy(DC *_pDestDC, const DC *_pSrcDC)
{
	RESULT r = GX_OK;
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT SrcRect;
	IRECT DestRect;
	BOOL bINDEX;
	BOOL bPKRGB2;
	BOOL bPKRGB3;
	BOOL bPKRGB4;
	BOOL bPKRGB;
	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	if (!pSrcDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pSrcDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	if ((pSrcDC->uiType == TYPE_JPEG)
		|| (pDestDC->uiType == TYPE_JPEG)) {
		DBG_ERR("GxGfx_Copy - ERROR! Not support copy pSrcDC/pDestDC with JPEG type.\r\n");
		return GX_ERROR_FORMAT;
	}
	if (pDestDC->uiPxlfmt != pSrcDC->uiPxlfmt) {
		DBG_ERR("GxGfx_Copy - ERROR! Format is different between pSrcDC and pDestDC.\r\n");
		return GX_ERROR_FORMAT;
	}
	SrcRect = _DC_GetRealRect(_pSrcDC);
	DestRect = _DC_GetRealRect(_pDestDC);

	if ((DestRect.w != SrcRect.w) || (DestRect.h != SrcRect.h)) {
		DBG_ERR("GxGfx_Copy - ERROR! Size is different between pSrcDC and pDestDC.\r\n");
		return GX_ERROR_SIZEDIFF;
	}

	bINDEX = ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
	bPKRGB2 = ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK2);
	bPKRGB3 = ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK3);
	bPKRGB4 = ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK4);
	bPKRGB = ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK);
	if (bINDEX) {
		//INDEX path
		if ((pDestDC->uiPxlfmt == PXLFMT_INDEX8)) {
			r = _DC_BitBlt_INDEX
				(_pDestDC, &DestRect, _pSrcDC, &SrcRect, ROP_COPY, 0);
		} else {
			r = _DC_BitBlt_INDEX_sw
				(_pDestDC, &DestRect, _pSrcDC, &SrcRect, ROP_COPY, 0, 0);
		}
	}
#if (GX_SUPPORT_DCFMT_RGB565)
	else if (bPKRGB2) {
		//INDEX path
		r = _DC_BitBlt_RGB565
			(_pDestDC, &DestRect, _pSrcDC, &SrcRect, ROP_COPY, 0);
		if (pDestDC->uiPxlfmt == PXLFMT_RGBA5658_PK) {
			r = _DC_BitBlt_INDEX
				(_pDestDC, &DestRect, _pSrcDC, &SrcRect, ROP_COPY, 0);
		}
	}
#endif
#if (GX_SUPPORT_DCFMT_RGB444)
	else if (bPKRGB3) {
		r = _DC_BitBlt_ARGB4444
			(_pDestDC, &DestRect, _pSrcDC, &SrcRect, ROP_COPY, 0);
	} else if (bPKRGB4) {
		r = _DC_BitBlt_ARGB1555
			(_pDestDC, &DestRect, _pSrcDC, &SrcRect, ROP_COPY, 0);
	}
#endif
	else if ((bPKRGB) && (pSrcDC->uiPxlfmt == PXLFMT_RGBA8888_PK) && (pDestDC->uiPxlfmt == PXLFMT_RGBA8888_PK)) {
		r = _DC_BitBlt_ARGB8888
			(_pDestDC, &DestRect, _pSrcDC, &SrcRect, ROP_COPY, 0);
	} else {
		DBG_ERR("Not support pSrcDC format 0x%x pDestDC format0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
		return GX_ERROR_FORMAT;
	}

	if (r != GX_OK) {
		DBG_ERR("GxGfx_Copy - ERROR! General error.%d\r\n",r);
	}

	return r;
}

RESULT GxGfx_CopyEx(DC *_pDestDC, const DC *_pSrcDC, UINT32 uiRop, UINT32 uiParam)
{
	RESULT r = GX_OK;
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT SrcRect;
	IRECT DestRect;
	BOOL bINDEX;
	BOOL bPKRGB2;
	BOOL bPKRGB3;
	BOOL bPKRGB4;
	BOOL bPKRGB;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	if (!pSrcDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pSrcDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	if ((pSrcDC->uiType == TYPE_JPEG)
		|| (pDestDC->uiType == TYPE_JPEG)) {
		DBG_ERR("GxGfx_CopyEx - ERROR! Not support copy pSrcDC/pDestDC with JPEG type.\r\n");
		return GX_ERROR_FORMAT;
	}
	SrcRect = _DC_GetRealRect(_pSrcDC);
	DestRect = _DC_GetRealRect(_pDestDC);

	bINDEX = ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
	bPKRGB2 = ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK2);
	bPKRGB3 = ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK3);
	bPKRGB4 = ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK4);
	bPKRGB = ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK);

	if (uiRop & ROP_ROTATE) {
		BOOL bMatchFormat = ((pDestDC->uiPxlfmt == pSrcDC->uiPxlfmt)
#if (GX_SUPPPORT_YUV ==ENABLE)
#if (GX_SUPPORT_DCFMT_UVPACK)
							 || ((pDestDC->uiPxlfmt == PXLFMT_YUV422_PK) && (pSrcDC->uiPxlfmt == PXLFMT_YUV421_PK))
							 || ((pDestDC->uiPxlfmt == PXLFMT_YUV421_PK) && (pSrcDC->uiPxlfmt == PXLFMT_YUV422_PK))

#else
							 || ((pDestDC->uiPxlfmt == PXLFMT_YUV422) && (pSrcDC->uiPxlfmt == PXLFMT_YUV421))
							 || ((pDestDC->uiPxlfmt == PXLFMT_YUV421) && (pSrcDC->uiPxlfmt == PXLFMT_YUV422))
#endif
#endif
							);
		BOOL bMatchSize = (((DestRect.w == SrcRect.w) && (DestRect.h == SrcRect.h))
						   || ((DestRect.w == SrcRect.h) && (DestRect.h == SrcRect.w))
						  );

		if (!bMatchFormat || !bMatchSize) {
			DBG_ERR("GxGfx_CopyEx - ERROR! Size or format is different between pSrcDC and pDestDC.\r\n");
			return GX_ERROR_SIZEDIFF;
		}

		//rotate case
		if ((bINDEX) && (pDestDC->uiPxlfmt == PXLFMT_INDEX8)) {
			//INDEX path
			r = _DC_RotateBlt_INDEX
				(_pDestDC, &DestRect, _pSrcDC, &SrcRect, uiRop, uiParam);

		}
#if (GX_SUPPORT_DCFMT_RGB565)
		else if ((bPKRGB2) && (pDestDC->uiPxlfmt == PXLFMT_RGBA5658_PK)) {
			r = _DC_RotateBlt_RGB565
				(_pDestDC, &DestRect, _pSrcDC, &SrcRect, uiRop, uiParam);
			if (r == GX_OK) {
				r = _DC_RotateBlt_INDEX
					(_pDestDC, &DestRect, _pSrcDC, &SrcRect, uiRop, uiParam);
			}
		}
#endif
		else if ((bPKRGB) && (pDestDC->uiPxlfmt == PXLFMT_RGBA8888_PK)) {
			r = _DC_RotateBlt_ARGB8888
				(_pDestDC, &DestRect, _pSrcDC, &SrcRect, uiRop, uiParam);

		}
#if (GX_SUPPORT_DCFMT_RGB444)
		else if ((bPKRGB3) && (pDestDC->uiPxlfmt == PXLFMT_RGBA4444_PK)) {
			r = _DC_RotateBlt_ARGB4444
				(_pDestDC, &DestRect, _pSrcDC, &SrcRect, uiRop, uiParam);

		} else if ((bPKRGB4) && (pDestDC->uiPxlfmt == PXLFMT_RGBA5551_PK)) {
			r = _DC_RotateBlt_ARGB1555
				(_pDestDC, &DestRect, _pSrcDC, &SrcRect, uiRop, uiParam);

		}
#endif
		else {
			DBG_ERR("Not support rotate pSrcDC format 0x%x pDestDC format0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
			return GX_ERROR_FORMAT;
		}

	} else {
		BOOL bMatchFormat = (pDestDC->uiPxlfmt == pSrcDC->uiPxlfmt);
		BOOL bMatchSize = ((DestRect.w == SrcRect.w) && (DestRect.h == SrcRect.h));

		if (!bMatchFormat || !bMatchSize) {
			DBG_ERR("GxGfx_CopyEx - ERROR! Size or format is different between pSrcDC and pDestDC.\r\n");
			return GX_ERROR_SIZEDIFF;
		}

		bINDEX = ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
		if (bINDEX) {
			//INDEX path
			if ((pDestDC->uiPxlfmt == PXLFMT_INDEX8)) {
				r = _DC_BitBlt_INDEX
					(_pDestDC, &DestRect, _pSrcDC, &SrcRect, uiRop, uiParam);
			} else {
				r = _DC_BitBlt_INDEX_sw
					(_pDestDC, &DestRect, _pSrcDC, &SrcRect,  uiRop, uiParam, 0);
			}
		}
#if (GX_SUPPORT_DCFMT_RGB565)
		else if (bPKRGB2) {
			r = _DC_BitBlt_RGB565
				(_pDestDC, &DestRect, _pSrcDC, &SrcRect, uiRop, uiParam);
			if (pDestDC->uiPxlfmt == PXLFMT_RGBA5658_PK) {
				r = _DC_BitBlt_INDEX
					(_pDestDC, &DestRect, _pSrcDC, &SrcRect, uiRop, uiParam);
			}
		}
#endif
#if (GX_SUPPORT_DCFMT_RGB444)
		else if ((bPKRGB3) && (pSrcDC->uiPxlfmt == PXLFMT_RGBA4444_PK) && (pDestDC->uiPxlfmt == PXLFMT_RGBA4444_PK)) {
			r = _DC_BitBlt_ARGB4444
				(_pDestDC, &DestRect, _pSrcDC, &SrcRect, uiRop, uiParam);
		} else if ((bPKRGB4) && (pSrcDC->uiPxlfmt == PXLFMT_RGBA5551_PK) && (pDestDC->uiPxlfmt == PXLFMT_RGBA5551_PK)) {
			r = _DC_BitBlt_ARGB1555
				(_pDestDC, &DestRect, _pSrcDC, &SrcRect, uiRop, uiParam);
		}
#endif
		else if ((bPKRGB) && (pSrcDC->uiPxlfmt == PXLFMT_RGBA8888_PK) && (pDestDC->uiPxlfmt == PXLFMT_RGBA8888_PK)) {
			r = _DC_BitBlt_ARGB8888
				(_pDestDC, &DestRect, _pSrcDC, &SrcRect, uiRop, uiParam);
		} else {
			DBG_ERR("Not support pSrcDC format 0x%x pDestDC format0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
			return GX_ERROR_FORMAT;
		}

	}
	if (r != GX_OK) {
		DBG_ERR("GxGfx_CopyEx - ERROR! General error. 0x%x\r\n", r);
	}

	return r;
}

RESULT GxGfx_Convert(DC *_pDestDC, const DC *_pSrcDC,
					 const void *pConvertTable)
{
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT SrcRect;
	IRECT DestRect;
	BOOL bINDEX;
	//const PALETTE_ITEM* pTable;
	IRECT cw;
	RESULT r;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	if (!pSrcDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pSrcDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	//check format
	bINDEX = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
	if (bINDEX) {
		DBG_ERR("GxGfx_Convert - ERROR! Not support convert to pDestDC with INDEX format.\r\n");
		return GX_ERROR_FORMAT;
	}

	SrcRect = _DC_GetRealRect(_pSrcDC);
	DestRect = _DC_GetRealRect(_pDestDC);

	//pTable = (const PALETTE_ITEM*)pConvertTable;
#if (GX_SUPPPORT_YUV ==ENABLE)
	if ((pSrcDC->uiPxlfmt == PXLFMT_RGB888) && (pDestDC->uiPxlfmt == PXLFMT_RGB888_PK)) {
		DBG_IND("GxGfx_Convert - Convert from pSrcDC:RGB888 format to pDestDC:RGB888_PK format.\r\n");

		//RGB888 -> RGB888_PK
		if (bSizeDiff) {
			DBG_ERR("GxGfx_Convert - ERROR! Size if different between pSrcDC and pDestDC.\r\n");
			return GX_ERROR_SIZEDIFF;
		}
		return _DC_ConvetBlt_RGBToRGB_sw(_pDestDC, _pSrcDC);
	}
	if ((pSrcDC->uiPxlfmt == PXLFMT_RGB888) && (pDestDC->uiPxlfmt == PXLFMT_RGBD8888_PK)) {
		DBG_IND("GxGfx_Convert - Convert from pSrcDC:RGB888 format to pDestDC:RGBD8888_PK format.\r\n");

		//RGB888 -> RGBD8888_PK
		if (bSizeDiff) {
			DBG_ERR("GxGfx_Convert - ERROR! Size if different between pSrcDC and pDestDC.\r\n");
			return GX_ERROR_SIZEDIFF;
		}
		return _DC_ConvetBlt_RGBToRGB_sw(_pDestDC, _pSrcDC);
	}
	if ((pSrcDC->uiPxlfmt == PXLFMT_RGB888_PK) && (pDestDC->uiPxlfmt == PXLFMT_RGB888)) {
		DBG_IND("GxGfx_Convert - Convert from pSrcDC:RGB888_PK format to pDestDC:RGB888 format.\r\n");

		//RGB888_PK -> RGB888
		if (bSizeDiff) {
			DBG_ERR("GxGfx_Convert - ERROR! Size if different between pSrcDC and pDestDC.\r\n");
			return GX_ERROR_SIZEDIFF;
		}
		return _DC_ConvetBlt_RGBToRGB_sw(_pDestDC, _pSrcDC);
	}
	if ((pSrcDC->uiPxlfmt == PXLFMT_RGBD8888_PK) && (pDestDC->uiPxlfmt == PXLFMT_RGB888)) {
		DBG_IND("GxGfx_Convert - Convert from pSrcDC:RGBD8888_PK format to pDestDC:RGB888 format.\r\n");

		//RGBD8888_PK -> RGB888
		if (bSizeDiff) {
			DBG_ERR("GxGfx_Convert - ERROR! Size if different between pSrcDC and pDestDC.\r\n");
			return GX_ERROR_SIZEDIFF;
		}
		return _DC_ConvetBlt_RGBToRGB_sw(_pDestDC, _pSrcDC);
	}
#endif
	//RGB -> YUV
	//YUV -> RGB
	//YUV -> YUV
	DBG_IND("GxGfx_Convert - Convert from pSrcDC:RGB format to pDestDC:YUV format.\r\n");
	DBG_IND("                     or, Convert from pSrcDC:YUV format to pDestDC:RGB format.\r\n");
	_GxGfx_SetImageROP(ROP_COPY | FILTER_LINEAR, 0, pConvertTable, 0);
	cw = GxGfx_GetWindow(_pDestDC);
	_DC_XFORM2_RECT(_pDestDC, &cw);
	_GxGfx_SetClip(0); //disable
	r = _GxGfx_StretchBlt(_pDestDC, &DestRect, _pSrcDC, &SrcRect);
	_GxGfx_SetClip(&cw); //restore
	return r;
}


//--------------------------------------------------------------------------------------
//  DC bitblt
//--------------------------------------------------------------------------------------

static BOOL _DC_CalcBitBltRect
(const IRECT *pSrcRect, const IPOINT *pDestPt, const IRECT *pDestClipRect,
 IRECT *pSrcReadRect, IRECT *pDestWriteRect)
{
	LVALUE SW, SH;
	LVALUE CL, CR, DX, DDX;
	LVALUE CT, CB, DY, DDY;

	SW = pSrcRect->w;
	CL = pDestClipRect->x;
	CR = CL + pDestClipRect->w;
	DX = pDestPt->x;

	if (pDestPt->x >= CR) {
		return FALSE;
	}

	if (pDestPt->x + SW <= CL) {
		return FALSE;
	}

	if (pDestPt->x + SW > CR) {
		pSrcReadRect->w = (CR - pDestPt->x);
	} else {
		pSrcReadRect->w = SW;
	}
	if (pDestPt->x < CL) {
		DDX = CL - pDestPt->x;
		pSrcReadRect->x = pSrcRect->x + DDX;
		pSrcReadRect->w = pSrcRect->w - DDX;
		DX = CL;
	}

	SH = pSrcRect->h;
	CT = pDestClipRect->y;
	CB = CT + pDestClipRect->h;
	DY = pDestPt->y;

	if (pDestPt->y >= CB) {
		return FALSE;
	}

	if (pDestPt->y + SH <= CT) {
		return FALSE;
	}

	if (pDestPt->y + SH > CB) {
		pSrcReadRect->h = (CB - pDestPt->y);
	} else {
		pSrcReadRect->h = SH;
	}
	if (pDestPt->y < CT) {
		DDY = CT - pDestPt->y;
		pSrcReadRect->y = pSrcRect->y + DDY;
		pSrcReadRect->h = pSrcRect->h - DDY;
		DY = CT;
	}

	RECT_Set(pDestWriteRect,
			 DX,
			 DY,
			 pSrcReadRect->w,
			 pSrcReadRect->h);

	return TRUE;
}

static BOOL _DC_CalcStretchBltRect
(const IRECT *pSrcRect, const IRECT *pDestRect, const IRECT *pDestClipRect,
 IRECT *pSrcReadRect, IRECT *pDestWriteRect)
{
	LVALUE SW, SH;
	LVALUE CL, CR, DX, DW;
	LVALUE CT, CB, DY, DH;
	LVALUE dstx_add, dstx_sub;
	LVALUE dsty_add, dsty_sub;

	SW = pSrcRect->w;
	CL = pDestClipRect->x;
	CR = CL + pDestClipRect->w;
	DX = pDestRect->x;
	DW = pDestRect->w;

	if (DX >= CR) {
		return FALSE;
	}

	if (DX + DW <= CL) {
		return FALSE;
	}

	dstx_sub = 0;
	if (DX + DW > CR) { // and (DX < CR)
		dstx_sub = (DX + DW) - CR;
	}
	dstx_add = 0;
	if (DX < CL) { // and (DX + DW > CL)
		dstx_add = CL - DX;
	}
	pDestWriteRect->x += dstx_add;
	pDestWriteRect->w -= (dstx_add + dstx_sub);
	pSrcReadRect->x += dstx_add * SW / DW;
	pSrcReadRect->w -= (dstx_add + dstx_sub) * SW / DW;

	SH = pSrcRect->h;
	CT = pDestClipRect->y;
	CB = CT + pDestClipRect->h;
	DY = pDestRect->y;
	DH = pDestRect->h;

	if (DY >= CB) {
		return FALSE;
	}

	if (DY + DW <= CT) {
		return FALSE;
	}

	dsty_sub = 0;
	if (DY + DH > CB) { // and (DY < CB)
		dsty_sub = (DY + DH) - CB;
	}
	dsty_add = 0;
	if (DY < CT) { // and (DY + DH > CT)
		dsty_add = CT - DY;
	}
	pDestWriteRect->y += dsty_add;
	pDestWriteRect->h -= (dsty_add + dsty_sub);
	pSrcReadRect->y += dsty_add * SH / DH;
	pSrcReadRect->h -= (dsty_add + dsty_sub) * SH / DH;

	return TRUE;
}



RESULT _GxGfx_BitBlt(DC *_pDestDC, const IPOINT *pDestPt, const DC *_pSrcDC, const IRECT *pSrcRect)
{
	DISPLAY_CONTEXT *pSrcDC;
	DISPLAY_CONTEXT *pDestDC;
	IRECT DestRect;
	IRECT DestClipRect;
	IPOINT DestPt;
	IRECT SrcRect;
#if 0
	BOOL bINDEX;
	BOOL bPKRGB2;
	BOOL bPKRGB;
#endif
	UINT32 rop;
	RESULT r = GX_OK;
	UINT32 SrcTypeGroup, DestTypeGroup;

	DBG_FUNC_BEGIN("\r\n");
	rop = _gxgfx_ctrl.uiRop & ROP_MASK;
	if (!_pDestDC) {
		return GX_NULL_POINTER;
	}
	if (!_pSrcDC) {
		_pSrcDC = _pDestDC;
	}
	pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	pDestDC = (DISPLAY_CONTEXT *)_pDestDC;

	//check type
	if (_pDestDC->uiType == TYPE_JPEG) {
		DBG_ERR("Not support BitBlt pDestDC with JPEG type.\r\n");
		return GX_ERROR_FORMAT;
	}
	if (_pSrcDC->uiType == TYPE_JPEG) {
		DBG_ERR("Not support BitBlt pSrcDC with JPEG type.\r\n");
		return GX_ERROR_FORMAT;
	}

#if 0
	//check format
	bINDEX = ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
	bPKRGB2 = ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK2);
	bPKRGB = ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK);
#endif
	//Clipping Window
	if (_gxgfx_ctrl.pClipRect) {
		DestClipRect = *_gxgfx_ctrl.pClipRect; //custom rect
	} else {
		DestClipRect = _DC_GetRealRect(_pDestDC); //full DC
	}

	if (rop == ROP_DESTFILL) {
		//SrcRect
		_DC_PREPARE_RECT(_pSrcDC, pSrcRect, &SrcRect);

		//DestPt
		DestPt = RECT_GetPoint(&SrcRect);
	} else {
		//SrcRect
		_DC_PREPARE_RECT(_pSrcDC, pSrcRect, &SrcRect);

		//DestPt
		_DC_PREPARE_POINT(_pDestDC, pDestPt, &DestPt);
	}

	if (!_DC_CalcBitBltRect(&SrcRect, &DestPt, &DestClipRect, &SrcRect, &DestRect)) {
		//do not have any intersection, cancel the blt action!
		return GX_OK;
	}

	SrcTypeGroup =  pSrcDC->uiPxlfmt & PXLFMT_PMODE;
	DestTypeGroup =  pDestDC->uiPxlfmt & PXLFMT_PMODE;


	switch (SrcTypeGroup) {
	case PXLFMT_P1_IDX: {
			switch (DestTypeGroup) {
			case PXLFMT_P1_IDX: {
					switch (rop) {
					case ROP_DESTFILL :
					case ROP_COPY :
					case ROP_KEY :
						break;
					default:
						DBG_ERR("Not support ROP 0x%x.\r\n", rop);
						return GX_NOTSUPPORT_ROP;
					}
					if ((pDestDC->uiPxlfmt == PXLFMT_INDEX8) && (pSrcDC->uiPxlfmt == PXLFMT_INDEX8) && (_gxgfx_ctrl.pIndexTable == 0)) {
                        #if GX_ICON_SWCOPY
                        if(pSrcDC->uiType==TYPE_ICON){
    						r = _DC_BitBlt_INDEX_sw
    							(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.uiRop, _gxgfx_ctrl.uiParam,0);

                        } else
                        #endif
                        {
						r = _DC_BitBlt_INDEX
							(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.uiRop, _gxgfx_ctrl.uiParam);
                        }
					} else {
						r = _DC_BitBlt_INDEX_sw
							(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.uiRop, _gxgfx_ctrl.uiParam, _gxgfx_ctrl.pIndexTable);
					}
				}
				break;
#if (GX_SUPPORT_DCFMT_RGB565)
			case PXLFMT_P1_PK2: {
					if (_gxgfx_ctrl.pColorTable == 0) {
						DBG_ERR("Not support BitBlt if not provide Palette for converting.\r\n");
						return GX_NO_PALETTE;
					}

					switch (rop) {
					case ROP_COPY:
					case ROP_KEY:
					case ROP_DESTFILL:
						r = _DC_BitBlt_INDEX2RGB565_sw
							(_pDestDC, &DestRect, _pSrcDC, &SrcRect, &_gxgfx_ctrl);
						break;
					default :
						DBG_ERR("Not support ROP 0x%x.\r\n", rop);
						return GX_NOTSUPPORT_ROP;
					}
				}
				break;
#endif
			case PXLFMT_P1_PK: {
					if (_gxgfx_ctrl.pColorTable == 0) {
						DBG_ERR("Not support BitBlt if not provide Palette for converting.\r\n");
						return GX_NO_PALETTE;
					}

					switch (rop) {
					case ROP_COPY:
					case ROP_KEY:
					case ROP_DESTFILL:
						r = _DC_BitBlt_INDEX2RGB888_sw
							(_pDestDC, &DestRect, _pSrcDC, &SrcRect, &_gxgfx_ctrl);
						break;
					default :
						DBG_ERR("Not support ROP 0x%x.\r\n", rop);
						return GX_NOTSUPPORT_ROP;
					}
				}
				break;
			case PXLFMT_P1_PK3: {
					if (_gxgfx_ctrl.pColorTable == 0) {
						DBG_ERR("Not support BitBlt if not provide Palette for converting.\r\n");
						return GX_NO_PALETTE;
					}

					switch (rop) {
					case ROP_COPY:
					case ROP_KEY:
					case ROP_DESTFILL:
						r = _DC_BitBlt_INDEX2RGB444_sw
							(_pDestDC, &DestRect, _pSrcDC, &SrcRect, &_gxgfx_ctrl);
						break;
					default :
						DBG_ERR("Not support ROP 0x%x.\r\n", rop);
						return GX_NOTSUPPORT_ROP;
					}
				}
				break;
			case PXLFMT_P1_PK4: {
					if (_gxgfx_ctrl.pColorTable == 0) {
						DBG_ERR("Not support BitBlt if not provide Palette for converting.\r\n");
						return GX_NO_PALETTE;
					}

					switch (rop) {
					case ROP_COPY:
					case ROP_KEY:
					case ROP_DESTFILL:
						r = _DC_BitBlt_INDEX2RGB555_sw
							(_pDestDC, &DestRect, _pSrcDC, &SrcRect, &_gxgfx_ctrl);
						break;
					default :
						DBG_ERR("Not support ROP 0x%x.\r\n", rop);
						return GX_NOTSUPPORT_ROP;
					}
				}
				break;
			default: {
					DBG_ERR("Not support fmt pSrcDC 0x%x to pDestDC 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
					return GX_ERROR_FORMAT;
				}
			}
		}
		break;
#if (GX_SUPPORT_DCFMT_RGB565)
	case PXLFMT_P1_PK2: {
			switch (DestTypeGroup) {
			case PXLFMT_P1_PK2: {
					switch (rop) {
					case ROP_DESTFILL : {
							UINT32 fs = GxGfx_Get(BR_SHAPEFILLSTYLE) & (FILLSTYLE_FILLCOLOR | FILLSTYLE_FILLALPHA);
							UINT32 color = COLOR_RGB565_GET_COLOR(_gxgfx_ctrl.uiParam);
							UINT32 alpha = COLOR_RGB565_GET_A(_gxgfx_ctrl.uiParam);

							DBG_MSG("blt rgba5658 = 0x%08x\r\n", _gxgfx_ctrl.uiParam);
							DBG_MSG("blt c = 0x%04x\r\n", color);
							DBG_MSG("blt a = 0x%04x\r\n", alpha);
							DBG_MSG("AAAA %d,%d,%d,%d\r\n", DestRect.x, DestRect.y, DestRect.x + DestRect.w - 1, DestRect.y + DestRect.h - 1);
							{
								if ((fs == 0) || (fs == FILLSTYLE_FILLCOLOR)) {
									r = _DC_BitBlt_RGB565
										(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.uiRop, color);
								}
                                if (pDestDC->uiPxlfmt == PXLFMT_RGBA5658_PK) {
									if ((fs == 0) || (fs == FILLSTYLE_FILLALPHA)) {
										r = _DC_BitBlt_INDEX
											(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.uiRop, alpha);
								    }
                                }
							}
						}
						break;
					case ROP_COPY :
					case ROP_KEY :
					case ROP_BLEND: {
							switch (rop) {
							case ROP_COPY :
							case ROP_KEY :
							case ROP_BLEND : {
                                #if GX_ICON_SWCOPY
                                if(pSrcDC->uiType==TYPE_ICON){
                                	r = _DC_BitBlt_RGB565_sw
                                		(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.uiRop, _gxgfx_ctrl.uiParam);

                                } else
                                #endif
                                {
								r = _DC_BitBlt_RGB565
									(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.uiRop, _gxgfx_ctrl.uiParam);
                                }

                                }
								break;
							}
							if (pDestDC->uiPxlfmt == PXLFMT_RGBA5658_PK) {
								BOOL bDoAlpha = FALSE;
								BOOL bDoAlphaMul = FALSE;
								DC TempDC;
								const DC *_pSrcDC2 = _pSrcDC;

								switch (rop) {
								case ROP_COPY :
									bDoAlpha = TRUE;  //must copy alpha
								case ROP_BLEND :  //ARBG856 blending ,not bDoalpha,blending is the same as ARBG8888_RGB only
									if (((_gxgfx_ctrl.uiRop & ROP_SUB_MASK) << 8) == ROP_MUL_CNST_ALPHA) {
										//alloc buffer for alpha pre-multiply
										r = GxGfx_PushStackDC(&TempDC,
															  TYPE_BITMAP,
															  _pSrcDC->uiPxlfmt,
															  _pSrcDC->size.w,
															  _pSrcDC->size.h);

										if (r != GX_OK) {
											DBG_WRN("GxGfx_Image - WARNING! Failed to alpha multiply. Ignore effect.\r\n");
										} else {
											bDoAlphaMul = TRUE;
											bDoAlpha = TRUE;  //for	ROP_BLEND|ROP_MUL_CNST_ALPHA
										}

										if (bDoAlphaMul) {
											//do alpha pre-multiply
											IRECT SrcRect2, DestRect2;
											SrcRect2 = DestRect2 = _DC_GetRect(_pSrcDC);
											r = _DC_BitBlt_INDEX
												(&TempDC, &DestRect2, _pSrcDC, &SrcRect2,
												 ROP_MUL_CNST_ALPHA, (_gxgfx_ctrl.uiRop & 0x000000ff));
											_pSrcDC2 = &TempDC;
											_gxgfx_ctrl.uiRop &= 0xff000000; //clear flag
										}
									}
									break;
								}
								if (bDoAlpha) {
                                    #if GX_ICON_SWCOPY
                                    if(pSrcDC->uiType==TYPE_ICON){
                                        //do nothing, _DC_BitBlt_RGB565_sw incldue alpha
                                    } else
                                    #endif
                                    {
    									r = _DC_BitBlt_INDEX
    										(_pDestDC, &DestRect, _pSrcDC2, &SrcRect, _gxgfx_ctrl.uiRop, _gxgfx_ctrl.uiParam);
								    }
                                }
								if (bDoAlphaMul) {
									//free buffer for alpha pre-multiply
									GxGfx_PopStackDC(&TempDC);
								}
							}
						}
						break;
					default:
						DBG_ERR("Not support ROP 0x%x.\r\n", rop);
						return GX_NOTSUPPORT_ROP;
					}
				}
				break;
			case PXLFMT_P1_PK: {
					DBG_ERR("Todo fmt pSrcDC 0x%x to pDestDC 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
					return GX_ERROR_FORMAT;
				}
				break;
			default: {
					DBG_ERR("Not support fmt pSrcDC 0x%x to pDestDC 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
					return GX_ERROR_FORMAT;
				}
			}
		}
		break;
#endif
#if (GX_SUPPORT_DCFMT_RGB444)
	case PXLFMT_P1_PK3: {
			switch (DestTypeGroup) {
			case PXLFMT_P1_PK3: {
					if ((pDestDC->uiPxlfmt == PXLFMT_RGBA4444_PK) && (pSrcDC->uiPxlfmt == PXLFMT_RGBA4444_PK)) {
						r = _DC_BitBlt_ARGB4444
							(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.uiRop, _gxgfx_ctrl.uiParam);

						switch (rop) {
						case ROP_COPY :
						case ROP_KEY :
						case ROP_BLEND:

							if (((_gxgfx_ctrl.uiRop & ROP_SUB_MASK) << 8) == ROP_MUL_CNST_ALPHA) {
								r = _DC_BitBlt_ARGB4444
									(_pDestDC, &DestRect, _pSrcDC, &SrcRect, ROP_MUL_CNST_ALPHA, (_gxgfx_ctrl.uiRop & 0x000000ff));

							}
						}
					} else {
						DBG_ERR("Not support this ROP with fmt pSrcDC 0x%x to pDestDC 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
						return GX_NOTSUPPORT_ROP;
					}
				}
				break;
			default: {
					DBG_ERR("Not support fmt pSrcDC 0x%x to pDestDC 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
					return GX_ERROR_FORMAT;
				}
			}
		}
		break;
	case PXLFMT_P1_PK4: {
			switch (DestTypeGroup) {
			case PXLFMT_P1_PK4: {
					if ((pDestDC->uiPxlfmt == PXLFMT_RGBA5551_PK) && (pSrcDC->uiPxlfmt == PXLFMT_RGBA5551_PK)) {
						r = _DC_BitBlt_ARGB1555
							(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.uiRop, _gxgfx_ctrl.uiParam);

						switch (rop) {
						case ROP_COPY :
						case ROP_KEY :
							if (((_gxgfx_ctrl.uiRop & ROP_SUB_MASK) << 8) == ROP_MUL_CNST_ALPHA) {
								r = _DC_BitBlt_ARGB1555
									(_pDestDC, &DestRect, _pSrcDC, &SrcRect, ROP_MUL_CNST_ALPHA, (_gxgfx_ctrl.uiRop & 0x000000ff));

							}
						}
					} else {
						DBG_ERR("Not support this ROP with fmt pSrcDC 0x%x to pDestDC 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
						return GX_NOTSUPPORT_ROP;
					}
				}
				break;
			default: {
					DBG_ERR("Not support fmt pSrcDC 0x%x to pDestDC 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
					return GX_ERROR_FORMAT;
				}
			}
		}
		break;
#endif
	case PXLFMT_P1_PK: {
			switch (DestTypeGroup) {
			case PXLFMT_P1_PK: {
					if ((pDestDC->uiPxlfmt == PXLFMT_RGBA8888_PK) && (pSrcDC->uiPxlfmt == PXLFMT_RGBA8888_PK)) {

                        #if GX_ICON_SWCOPY
                        if(pSrcDC->uiType==TYPE_ICON){
                        	r = _DC_BitBlt_ARGB8888_sw
                        		(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.uiRop, _gxgfx_ctrl.uiParam,0);

                        } else
                        #endif
                        {
                        	r = _DC_BitBlt_ARGB8888
                        		(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.uiRop, _gxgfx_ctrl.uiParam);
                        }
						switch (rop) {
						case ROP_COPY :
						case ROP_KEY :
						case ROP_BLEND:
							if (((_gxgfx_ctrl.uiRop & ROP_SUB_MASK) << 8) == ROP_MUL_CNST_ALPHA) {
								r = _DC_BitBlt_ARGB8888
									(_pDestDC, &DestRect, _pSrcDC, &SrcRect, ROP_MUL_CNST_ALPHA, (_gxgfx_ctrl.uiRop & 0x000000ff));

							}
						}
					} else {
						DBG_ERR("Not support this ROP with fmt pSrcDC 0x%x to pDestDC 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
						return GX_NOTSUPPORT_ROP;
					}
				}
				break;
			default: {
					DBG_ERR("Not support fmt pSrcDC 0x%x to pDestDC 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
					return GX_ERROR_FORMAT;
				}
			}
		}
		break;
	default: {
			DBG_ERR("Not support fmt pSrcDC 0x%x to pDestDC 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
			return GX_ERROR_FORMAT;
		}
	}
	if (r != GX_OK) {
		DBG_ERR("General error. 0x%x pSrcDC format 0x%x pDestDC format0x%x. type0x%x rop0x%x\r\n", r, pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt,pSrcDC->uiType,rop);
	}

	return r;
}


RESULT _GxGfx_StretchBlt(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect)
{
	RESULT r = GX_OK;
	const DISPLAY_CONTEXT *pSrcDC = (const DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;

	IRECT DestRect;
	IRECT SrcRect;
	IRECT DestClipRect;
	BOOL bINDEX;
	BOOL bPKRGB2;
	BOOL bPKRGB3;
	BOOL bPKRGB4;
	BOOL bPKRGB;
	UINT32 rop;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}

	if (!pSrcDC) {
		pSrcDC = pDestDC;
	}

	if ((_pDestDC->uiType == TYPE_JPEG)) {
		DBG_ERR("Not support pDestDC with JPEG type.\r\n");
		return GX_ERROR_FORMAT;
	}
	rop = _gxgfx_ctrl.uiRop & ROP_MASK;
	//check format
	bINDEX = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
	bPKRGB2 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK2);
	bPKRGB3 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK3);
	bPKRGB4 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK4);
	bPKRGB = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK);

	//DestRect
	_DC_PREPARE_RECT(_pDestDC, pDestRect, &DestRect);
	//SrcRect
	_DC_PREPARE_RECT(_pSrcDC, pSrcRect, &SrcRect);

	//Clipping Window
	if (_gxgfx_ctrl.pClipRect) {
		DestClipRect = *_gxgfx_ctrl.pClipRect; //custom rect
	} else {
		DestClipRect = _DC_GetRealRect(_pDestDC); //full DC
	}

	if (!_DC_CalcStretchBltRect(&SrcRect, &DestRect, &DestClipRect, &SrcRect, &DestRect)) {
		//do not have any intersection, cancel the blt action!
		return GX_OK;
	}

	if (bINDEX) {
		//index to index
		if ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) != PXLFMT_P1_IDX) {
			DBG_ERR("Not support with this pSrcDC format.0x%x\r\n", pSrcDC->uiPxlfmt);
			return GX_ERROR_FORMAT;
		}
		switch (rop) {
		case ROP_COPY :

			if (((_gxgfx_ctrl.pIndexTable == 0) && (pSrcDC->uiPxlfmt  == PXLFMT_INDEX4) && (pDestDC->uiPxlfmt  == PXLFMT_INDEX4)) ||
				((_gxgfx_ctrl.pIndexTable == 0) && (pSrcDC->uiPxlfmt  == PXLFMT_INDEX8) && (pDestDC->uiPxlfmt  == PXLFMT_INDEX8))) {
				r = _DC_StretchBlt_INDEX(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
										 _gxgfx_ctrl.uiRop, _gxgfx_ctrl.uiParam, _gxgfx_ctrl.pIndexTable);
			} else {
				r = _DC_StretchBlt_INDEX_sw(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
											_gxgfx_ctrl.uiRop, _gxgfx_ctrl.uiParam, _gxgfx_ctrl.pIndexTable);
			}
			break;
		case ROP_DESTFILL :
		case ROP_KEY :
			r = _DC_StretchBlt_INDEX_sw(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
										_gxgfx_ctrl.uiRop, _gxgfx_ctrl.uiParam, _gxgfx_ctrl.pIndexTable);
			break;
		default:
			DBG_ERR("Not support ROP 0x%x.\r\n", rop);
			return GX_NOTSUPPORT_ROP;
		}
	}  // dest index end
#if (GX_SUPPORT_DCFMT_RGB565)
	else if (bPKRGB2) {
		//index to RGB
		if ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX) {
			switch (rop) {
			case ROP_DESTFILL :
			case ROP_COPY :
			case ROP_KEY :
				break;
			default:
				DBG_ERR("Not support ROP 0x%x.\r\n", rop);
				return GX_NOTSUPPORT_ROP;
			}
			if (_gxgfx_ctrl.pColorTable == 0) {
				DBG_ERR("Not support StretchBlt if not provide Palette for converting.\r\n");
				return GX_NO_PALETTE;
			}
			r = _DC_StretchBlt_INDEX2RGB565_sw(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
											   &_gxgfx_ctrl);
		} else //RGB to RGB
			if ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK2) {
				switch (rop) {
				case ROP_COPY :
					if (pSrcDC->uiPxlfmt  == pDestDC->uiPxlfmt) {
						r = _DC_StretchBlt_RGB565(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
												  &_gxgfx_ctrl);
					} else {
						r = _DC_StretchBlt_RGB565_sw(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
													 &_gxgfx_ctrl);
					}

					break;
				case ROP_DESTFILL :
				case ROP_KEY :
					r = _DC_StretchBlt_RGB565_sw(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
												 &_gxgfx_ctrl);
					break;
				default:
					DBG_ERR("Not support ROP 0x%x.\r\n", rop);
					return GX_NOTSUPPORT_ROP;
				}
			} else {
				DBG_ERR("Not support with this pSrcDC format.0x%x\r\n", pSrcDC->uiPxlfmt);
				return GX_ERROR_FORMAT;
			}
	}
#endif
	else if (bPKRGB) {
		//index to ARGB8888_PK
		if ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX) {
			switch (rop) {
			case ROP_DESTFILL :
			case ROP_COPY :
			case ROP_KEY :
				break;
			default:
				DBG_ERR("Not support ROP 0x%x.\r\n", rop);
				return GX_NOTSUPPORT_ROP;
			}
			if (_gxgfx_ctrl.pColorTable == 0) {
				DBG_ERR("Not support StretchBlt if not provide Palette for converting.\r\n");
				return GX_NO_PALETTE;
			}
			r = _DC_StretchBlt_INDEX2RGBA8888_sw(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
												 &_gxgfx_ctrl);
		} else //RGB to RGB
			if ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK) {
				if ((pSrcDC->uiPxlfmt  == PXLFMT_RGBA8888_PK) && (pDestDC->uiPxlfmt  == PXLFMT_RGBA8888_PK)) {
					switch (rop) {
					case ROP_COPY :
					case ROP_DESTFILL :
						r = _DC_StretchBlt_ARGB8888(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
													&_gxgfx_ctrl);
						break;
					case ROP_KEY :
						r = _DC_StretchBlt_ARGB8888_sw(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
													   &_gxgfx_ctrl);
						break;
					default:
						DBG_ERR("Not support ROP 0x%x.\r\n", rop);
						return GX_NOTSUPPORT_ROP;
					}
				} else {
					DBG_ERR("Not support pSrcDC format 0x%x pDestDC format 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
					return GX_ERROR_FORMAT;
				}

			} else {
				DBG_ERR("Not support pSrcDC format 0x%x pDestDC format0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
				return GX_ERROR_FORMAT;
			}
	}
#if (GX_SUPPORT_DCFMT_RGB444)
	else if (bPKRGB3) {
		if ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX) {
			switch (rop) {
			case ROP_DESTFILL :
			case ROP_COPY :
			case ROP_KEY :
				break;
			default:
				DBG_ERR("Not support ROP 0x%x.\r\n", rop);
				return GX_NOTSUPPORT_ROP;
			}
			if (_gxgfx_ctrl.pColorTable == 0) {
				DBG_ERR("Not support StretchBlt if not provide Palette for converting.\r\n");
				return GX_NO_PALETTE;
			}
			r = _DC_StretchBlt_INDEX2RGBA4444_sw(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
												 &_gxgfx_ctrl);
		} else //RGB to RGB
			if ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK3) {
				if ((pSrcDC->uiPxlfmt  == PXLFMT_RGBA4444_PK) && (pDestDC->uiPxlfmt  == PXLFMT_RGBA4444_PK)) {
					switch (rop) {
					case ROP_COPY :
					case ROP_DESTFILL :
						r = _DC_StretchBlt_ARGB4444(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
													&_gxgfx_ctrl);
						break;
					case ROP_KEY :
						r = _DC_StretchBlt_ARGB4444_sw(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
													   &_gxgfx_ctrl);
						break;
					default:
						DBG_ERR("Not support ROP 0x%x.\r\n", rop);
						return GX_NOTSUPPORT_ROP;
					}
				} else {
					DBG_ERR("Not support pSrcDC format 0x%x pDestDC format 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
					return GX_ERROR_FORMAT;
				}

			} else {
				DBG_ERR("Not support pSrcDC format 0x%x pDestDC format0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
				return GX_ERROR_FORMAT;
			}
	} else if (bPKRGB4) {
		if ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX) {
			switch (rop) {
			case ROP_DESTFILL :
			case ROP_COPY :
			case ROP_KEY :
				break;
			default:
				DBG_ERR("Not support ROP 0x%x.\r\n", rop);
				return GX_NOTSUPPORT_ROP;
			}
			if (_gxgfx_ctrl.pColorTable == 0) {
				DBG_ERR("Not support StretchBlt if not provide Palette for converting.\r\n");
				return GX_NO_PALETTE;
			}
			r = _DC_StretchBlt_INDEX2RGBA5551_sw(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
												 &_gxgfx_ctrl);
		} else //RGB to RGB
			if ((pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK4) {
				if ((pSrcDC->uiPxlfmt  == PXLFMT_RGBA5551_PK) && (pDestDC->uiPxlfmt  == PXLFMT_RGBA5551_PK)) {
					switch (rop) {
					case ROP_COPY :
					case ROP_DESTFILL :
						r = _DC_StretchBlt_ARGB1555(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
													&_gxgfx_ctrl);
						break;
					case ROP_KEY :
						r = _DC_StretchBlt_ARGB1555_sw(_pDestDC, &DestRect, _pSrcDC, &SrcRect,
													   &_gxgfx_ctrl);
						break;
					default:
						DBG_ERR("Not support ROP 0x%x.\r\n", rop);
						return GX_NOTSUPPORT_ROP;
					}
				} else {
					DBG_ERR("Not support pSrcDC format 0x%x pDestDC format 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
					return GX_ERROR_FORMAT;
				}

			} else {
				DBG_ERR("Not support pSrcDC format 0x%x pDestDC format0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
				return GX_ERROR_FORMAT;
			}
	}
#endif
	else { //P3 to P3
		DBG_ERR("Not support pSrcDC format 0x%x pDestDC format0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
		return GX_ERROR_FORMAT;
	}

	if (r != GX_OK) {
		DBG_ERR("General error. 0x%x\r\n", r);
	}

	return r;
}
RESULT _GxGfx_FontBitBlt(DC *_pDestDC, const IPOINT *pDestPt, const DC *_pSrcDC, const IRECT *pSrcRect)
{
	DISPLAY_CONTEXT *pSrcDC;
	DISPLAY_CONTEXT *pDestDC;
	IRECT DestRect;
	IRECT DestClipRect;
	IPOINT DestPt;
	IRECT SrcRect;
	RESULT r = GX_OK;
	UINT32 SrcTypeGroup, DestTypeGroup;

	DBG_FUNC_BEGIN("\r\n");

	if ((_gxgfx_ctrl.uiRop & ROP_MASK) != ROP_FONT) {
		return GX_NOTSUPPORT_ROP;
	}

	if (_gxgfx_ctrl.pColorTable == 0) {
		DBG_ERR("Not support if not provide Palette for converting.\r\n");
		return GX_NO_PALETTE;
	}

	if (!_pDestDC) {
		return GX_NULL_POINTER;
	}
	if (!_pSrcDC) {
		_pSrcDC = _pDestDC;
	}

	pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	pDestDC = (DISPLAY_CONTEXT *)_pDestDC;


	//Clipping Window
	if (_gxgfx_ctrl.pClipRect) {
		DestClipRect = *_gxgfx_ctrl.pClipRect; //custom rect
	} else {
		DestClipRect = _DC_GetRealRect(_pDestDC); //full DC
	}

	{
		//SrcRect
		_DC_PREPARE_RECT(_pSrcDC, pSrcRect, &SrcRect);

		//DestPt
		_DC_PREPARE_POINT(_pDestDC, pDestPt, &DestPt);
	}

	if (!_DC_CalcBitBltRect(&SrcRect, &DestPt, &DestClipRect, &SrcRect, &DestRect)) {
		//do not have any intersection, cancel the blt action!
		return GX_OK;
	}

	SrcTypeGroup =  pSrcDC->uiPxlfmt & PXLFMT_PMODE;
	DestTypeGroup =  pDestDC->uiPxlfmt & PXLFMT_PMODE;

	if (SrcTypeGroup == PXLFMT_P1_IDX) {
		switch (DestTypeGroup) {
		case PXLFMT_P1_IDX: {
				r = _DC_FontBlt_INDEX_sw
					(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.pColorTable);
			}
			break;
		case PXLFMT_P1_PK2: {
				r = _DC_FontBlt_INDEX2RGB565_sw
					(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.pColorTable);
			}
			break;
		case PXLFMT_P1_PK: {
				r = _DC_FontBlt_INDEX2ARGB8888_sw
					(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.pColorTable);
			}
			break;
		case PXLFMT_P1_PK3: {
				r = _DC_FontBlt_INDEX2ARGB4444_sw
					(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.pColorTable);
			}
			break;
		case PXLFMT_P1_PK4: {
				r = _DC_FontBlt_INDEX2ARGB1555_sw
					(_pDestDC, &DestRect, _pSrcDC, &SrcRect, _gxgfx_ctrl.pColorTable);
			}
			break;
		default: {
				DBG_ERR("Not support fmt pSrcDC 0x%x to pDestDC 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
				return GX_ERROR_FORMAT;
			}
		}
	} else {
		DBG_ERR("Not support fmt pSrcDC 0x%x to pDestDC 0x%x.\r\n", pSrcDC->uiPxlfmt, pDestDC->uiPxlfmt);
		return GX_ERROR_FORMAT;
	}

	if (r != GX_OK) {
		DBG_ERR("General error. 0x%x\r\n", r);
	}

	return r;
}

