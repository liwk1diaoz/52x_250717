#ifndef _DC_BLT_H_
#define _DC_BLT_H_

#include "GxGfx/GxGfx.h"
#include "GxDCDraw.h"
#include "GxGfx_int.h"

#define USE_FAST_BLT    1   //fast path (optimized by dword read/write) of bitblt
#define USE_BIT_REVERSE 0   //turn off this value if using IMAGE_TABLE format after UIDS v2.1.0 build 2008-05-07.1


extern DC_CTRL _gxgfx_ctrl;


//8 bits access
typedef UINT8 DATA_PACK;
#define DATA_BIT 3

//32 bits access
//typedef UINT32 DATA_PACK;
//#define DATA_BIT 5

#define DATA_BITSIZE        (1<<DATA_BIT)
#define DATA_BITOFFSETMASK  (DATA_BITSIZE-1)


#define _ABS(a)                     (((a)>0)?(a):-(a))
#define WORD_ALIGNMENT              (~0x00000003)
#define CHECK_WORD_ALIGNMENT(p)     ((((UINT32)(p)) & 0x00000003)==0)



#define _DC_PREPARE_POINT(_pDC, pInputPt, pCurrentPt)  \
	if(!(pInputPt))  \
	{   \
		POINT_Set((pCurrentPt), 0, 0);   \
	}   \
	else    \
	{   \
		POINT_SetPoint((pCurrentPt), (pInputPt));   \
	}

//        (pInputPt) = (pCurrentPt);

#define _DC_PREPARE_RECT(_pDC, pInputRect, pCurrentRect)  \
	if(!(pInputRect))  \
	{   \
		*(pCurrentRect) = _DC_GetRect(_pDC);   \
	}   \
	else    \
	{   \
		RECT_SetRect((pCurrentRect), (pInputRect));   \
	}

//        (pInputRect) = (pCurrentRect);


//condition : pDestAddr is not null
RESULT _RectMemSet(UINT8 *pDestAddr, INT16 iDestPitch,
				   UINT16 uiWidth, UINT16 uiHeight, UINT8 ucValue);

//condition : pDestAddr is not null
//condition : pSrcAddr is not null
RESULT _RectMemCopy(UINT8 *pDestAddr, INT16 iDestPitch, UINT8 *pSrcAddr, INT16 iSrcPitch,
					UINT16 uiWidth, UINT16 uiHeight);

//condition : uiWidth must be 4 bytes alignment
RESULT _DC_RectMemSet(UINT8 *pDestAddr, INT16 iDestPitch,
					  UINT16 uiWidth, UINT16 uiHeight, UINT8 ucValue);

//condition : pDestAddr is not null
//condition : pSrcAddr is not null
//condition : pDestAddr must be 4 bytes alignment
//condition : iDestPitch must be 4 bytes alignment
//condition : pSrcAddr must be 4 bytes alignment
//condition : iSrcPitch must be 4 bytes alignment
//condition : uiWidth must be 4 bytes alignment
RESULT _DC_RectMemCopy(UINT8 *pDestAddr, INT16 iDestPitch, UINT8 *pSrcAddr, INT16 iSrcPitch,
					   UINT16 uiWidth, UINT16 uiHeight);

//condition : pDestAddr is not null
//condition : pSrcAddr is not null
//condition : pDestAddr must be 4 bytes alignment
//condition : iDestPitch must be 4 bytes alignment
//condition : pSrcAddr must be 4 bytes alignment
//condition : iSrcPitch must be 4 bytes alignment
//condition : uiWidth must be 4 bytes alignment
RESULT _DC_RectMemAOP(UINT8 *pDestAddr, INT16 iDestPitch, UINT8 *pSrcAddr, INT16 iSrcPitch,
					  UINT16 uiWidth, UINT16 uiHeight, UINT32 rop, UINT32 Param);



typedef struct _Brush {
	//style
	UINT32 uiLineStyle;
	//color
	COLOR_ITEM uiColor;

	//dest
	int uiDestType;
	LVALUE rx1;
	LVALUE ry1;
	LVALUE rx2;
	LVALUE ry2;
	void (*pfuncDraw)(DC *_pDestDC, LVALUE x, LVALUE y);
} Brush;

//condition : pSrcBrush is not null
//condition : _pDestDC is not null
//condition : uiSize >= 1
void _DC_SetBrush(DC *_pDestDC, const IRECT *pClipRect,
				  UINT32 uiLineStyle, COLOR_ITEM uiColor);

//condition : _pDestDC is not null
//condition : pSrcBrush is not null
void _DC_Brush(DC *_pDestDC, LVALUE x, LVALUE y);

///////////////////////////////////////////////////////////////////////////////
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
void _DC_SetBrush_LineWeight(DC *_pDestDC, UINT32 uiLineWeight);

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : y,x1,x2 is validated and already clipped by _pDestDC
void _DC_LineHorizontal_sw(DC *_pDestDC, LVALUE y, LVALUE x1, LVALUE x2);

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : x,y1,y2 is validated and already clipped by _pDestDC
void _DC_LineVertical_sw(DC *_pDestDC, LVALUE x, LVALUE y1, LVALUE y2);


///////////////////////////////////////////////////////////////////////////////
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by _pDestDC
RESULT _DC_FillRect_INDEX_sw(DC *_pDestDC, const IRECT *pDestRect,
							 UINT32 uiRop, COLOR_ITEM uiColor, const MAPPING_ITEM *pTable);

#if (GX_SUPPORT_DCFMT_RGB565)
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_PK2 format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by _pDestDC
RESULT _DC_FillRect_RGB565_sw(DC *_pDestDC, const IRECT *pDestRect,
							  UINT32 uiRop, COLOR_ITEM uiColor, const MAPPING_ITEM *pTable);
#endif

///////////////////////////////////////////////////////////////////////////////
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_BitBlt_INDEX(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
						UINT32 uiRop, UINT32 uiParam);

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_BitBlt_INDEX_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
						   UINT32 uiRop, UINT32 uiParam, const MAPPING_ITEM *pTable);

RESULT _DC_BitBlt_ARGB8888(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
						   UINT32 uiRop, UINT32 uiParam);

RESULT _DC_BitBlt_ARGB8888_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							  UINT32 uiRop, UINT32 uiParam, const MAPPING_ITEM *pTable);
RESULT _DC_FillRect_ARGB8888_sw(DC *_pDestDC, const IRECT *pDestRect,
								UINT32 uiRop, COLOR_ITEM uiColor, const MAPPING_ITEM *pTable);
RESULT _DC_StretchBlt_ARGB8888(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							   DC_CTRL *pc);
RESULT _DC_StretchBlt_ARGB8888_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
								  DC_CTRL *pc);
RESULT _DC_BitBlt_INDEX2RGB888_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
								  DC_CTRL *pc);
RESULT _DC_RotateBlt_ARGB8888(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							  UINT32 uiRop, UINT32 uiParam);
RESULT _DC_FontBlt_INDEX2ARGB8888_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
									 const MAPPING_ITEM *pTable);
RESULT _DC_StretchBlt_INDEX2RGBA8888_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
										DC_CTRL *pc);

#if (GX_SUPPORT_DCFMT_RGB565)
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_PK2 format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_PK2 format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_BitBlt_RGB565(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
						 UINT32 uiRop, UINT32 uiParam);

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_RGB565 format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_RGB565 format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_BitBlt_RGB565_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							UINT32 uiRop, UINT32 uiParam);
#endif

///////////////////////////////////////////////////////////////////////////////
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_FontBlt_INDEX_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							const MAPPING_ITEM *pTable);

#if (GX_SUPPORT_DCFMT_RGB565)
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_RGB565 format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_FontBlt_INDEX2RGB565_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
								   const MAPPING_ITEM *pTable);
#endif


IPOINT _DC_GetFontEffectExtend(UINT32 iFontEffect);

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
RESULT _DC_FontEffectBlt_INDEX_sw(DC *_pDestDC, const DC *_pSrcDC, UINT32 iFontEffect);



///////////////////////////////////////////////////////////////////////////////
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_StretchBlt_INDEX(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							UINT32 uiRop, UINT32 uiParam, const MAPPING_ITEM *pTable);

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_StretchBlt_INDEX_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							   UINT32 uiRop, UINT32 uiParam, const MAPPING_ITEM *pTable);

#if (GX_SUPPORT_DCFMT_RGB565)
RESULT _DC_StretchBlt_RGB565(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							 DC_CTRL *pc);

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_PK2 format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_PK2 format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_StretchBlt_RGB565_sw
(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
 DC_CTRL *pc);
#endif

///////////////////////////////////////////////////////////////////////////////

RESULT _DC_RotateBlt_INDEX(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
						   UINT32 uiRop, UINT32 uiParam);
#if (GX_SUPPORT_DCFMT_RGB565)
RESULT _DC_RotateBlt_RGB565(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							UINT32 uiRop, UINT32 uiParam);
#endif

RESULT _DC_RotateBlt_YUV(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
						 UINT32 uiRop, UINT32 uiParam);

///////////////////////////////////////////////////////////////////////////////
#if (GX_SUPPORT_DCFMT_RGB565)
//modify from _DC_BitBlt_INDEX2YUV_sw
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_RGB565 format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_INDEX format
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
//condition : pTable is not null (for INDEX to YUV convert)
RESULT _DC_BitBlt_INDEX2RGB565_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
								  DC_CTRL *pc);
#endif

#if (GX_SUPPORT_DCFMT_RGB565)
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_PK2 format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_StretchBlt_INDEX2RGB565_sw
(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
 DC_CTRL *pc);
#endif

#if 0 //TODO
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_PK2 format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_StretchBlt_INDEX2YUV_sw
(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
 DC_CTRL *pc);
#endif

#if (GX_SUPPPORT_YUV ==ENABLE)
///////////////////////////////////////////////////////////////////////////////

//condition : _pDestDC is not null
//condition : _pSrcDC is not null
//condition : _pDestDC is PXLFMT_RGB888 format and _pSrcDC is PXLFMT_RGB888_PK/PXLFMT_RGBD8888_PK format
//        or, _pDestDC is PXLFMT_RGB888_PK/PXLFMT_RGBD8888_PK format and _pSrcDC is PXLFMT_RGB888 format
RESULT _DC_ConvetBlt_RGBToRGB_sw(DC *_pDestDC, const DC *_pSrcDC);
#endif
///////////////////////////////////////////////////////////////////////////////
#if (GX_SUPPORT_DCFMT_RGB444 ==ENABLE)

RESULT _DC_BitBlt_ARGB4444(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
						   UINT32 uiRop, UINT32 uiParam);

RESULT _DC_BitBlt_ARGB4444_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							  UINT32 uiRop, UINT32 uiParam, const MAPPING_ITEM *pTable);
RESULT _DC_FillRect_ARGB4444_sw(DC *_pDestDC, const IRECT *pDestRect,
								UINT32 uiRop, COLOR_ITEM uiColor, const MAPPING_ITEM *pTable);
RESULT _DC_StretchBlt_ARGB4444(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							   DC_CTRL *pc);
RESULT _DC_StretchBlt_ARGB4444_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
								  DC_CTRL *pc);
RESULT _DC_BitBlt_INDEX2RGB444_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
								  DC_CTRL *pc);
RESULT _DC_RotateBlt_ARGB4444(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							  UINT32 uiRop, UINT32 uiParam);
RESULT _DC_FontBlt_INDEX2ARGB4444_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
									 const MAPPING_ITEM *pTable);
RESULT _DC_StretchBlt_INDEX2RGBA4444_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
										DC_CTRL *pc);

RESULT _DC_BitBlt_ARGB1555(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
						   UINT32 uiRop, UINT32 uiParam);

RESULT _DC_BitBlt_ARGB1555_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							  UINT32 uiRop, UINT32 uiParam, const MAPPING_ITEM *pTable);
RESULT _DC_FillRect_ARGB1555_sw(DC *_pDestDC, const IRECT *pDestRect,
								UINT32 uiRop, COLOR_ITEM uiColor, const MAPPING_ITEM *pTable);
RESULT _DC_StretchBlt_ARGB1555(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							   DC_CTRL *pc);
RESULT _DC_StretchBlt_ARGB1555_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
								  DC_CTRL *pc);
RESULT _DC_BitBlt_INDEX2RGB555_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
								  DC_CTRL *pc);
RESULT _DC_RotateBlt_ARGB1555(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							  UINT32 uiRop, UINT32 uiParam);
RESULT _DC_FontBlt_INDEX2ARGB1555_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
									 const MAPPING_ITEM *pTable);
RESULT _DC_StretchBlt_INDEX2RGBA5551_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
										DC_CTRL *pc);

#endif
///////////////////////////////////////////////////////////////////////////////




//check rect or DC size is zero
#define _DC_IS_ZERO_INDEX(pDC, pRect) \
	( ((pRect)->w == 0) ||   \
	  ((pRect)->h == 0) ||  \
	  ((pDC)->blk[3].uiPitch == 0))

//check rect or DC size is zero
#define _DC_IS_ZERO_RGB_PK(pDC, pRect) \
	( ((pRect)->w == 0) ||   \
	  ((pRect)->h == 0) ||  \
	  ((pDC)->blk[3].uiPitch == 0))

//check rect or DC size is zero
#define _DC_IS_ZERO_ALPHA(pDC, pRect) \
	( ((pRect)->w == 0) ||   \
	  ((pRect)->h == 0) ||  \
	  ((pDC)->blk[3].uiPitch == 0))

//check rect or DC size is zero
#define _DC_IS_ZERO_RGB565(pDC, pRect) \
	( ((pRect)->w == 0) ||   \
	  ((pRect)->h == 0) ||  \
	  ((pDC)->blk[0].uiPitch == 0))

//check rect or DC size is zero
#define _DC_IS_ZERO_YUV(pDC, pRect) \
	( ((pRect)->w == 0) ||   \
	  ((pRect)->h == 0) ||  \
	  ((pDC)->blk[0].uiPitch == 0) || \
	  ((pDC)->blk[1].uiPitch == 0) || \
	  ((pDC)->blk[2].uiPitch == 0))

#endif //_DC_BLT_H_
