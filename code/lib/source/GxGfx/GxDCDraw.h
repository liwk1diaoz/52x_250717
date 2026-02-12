
//    Graphics

/*
    DC : Drawing Canvas object, a bitmap use as a drawing target/source

    BRUSH: Draw brush, a property-set to help DC drawing

    VAR: Draw variable, a variable array to help DC drawing custom items
*/

#ifndef _GX_DCDRAW_H_
#define _GX_DCDRAW_H_

#include "GxGfx/GxGfx.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------------
//  DC
//--------------------------------------------------------------------------------------

//<struct>

typedef struct _DC_CTRL {
	const IRECT *pClipRect;
	UINT32 uiRop;
	UINT32 uiParam;
	const PALETTE_ITEM *pColorTable;
	const MAPPING_ITEM *pIndexTable;
}
DC_CTRL;


//<function>

//dc (low level) - not support xform/scaling
void _GxGfx_SetClip(const IRECT *pClipRect);
void _GxGfx_SetImageROP(UINT32 uiRop, UINT32 uiParam, const PALETTE_ITEM *pColorTable, const MAPPING_ITEM *pIndexTable);

RESULT _GxGfx_Pixel(DC *pDestDC, const IPOINT *pPt, COLOR_ITEM uiColor);
RESULT _GxGfx_Line(DC *pDestDC, const IPOINT *pStartPt, const IPOINT *pEndPt, UINT32 uiLineStyle, COLOR_ITEM uiColor);
RESULT _GxGfx_Ellipse(DC *pDestDC, const IRECT *pRect, const IPOINT *pRadius, UINT32 uiLineStyle, COLOR_ITEM uiColor, BOOL bFill);
#define _GxGfx_FillEllipse(pDestDC, pRect, pRadius, uiColor)    \
	_GxGfx_Ellipse(pDestDC, pRect, pRadius, 0, uiColor, TRUE)
#define _GxGfx_FrameEllipse(pDestDC, pRect, pRadius, uiLineStyle, uiColor)    \
	_GxGfx_Ellipse(pDestDC, pRect, pRadius, uiLineStyle, uiColor, FALSE)
RESULT _GxGfx_Rect(DC *pDestDC, const IRECT *pDestRect, UINT32 uiLineStyle, COLOR_ITEM uiColor, BOOL bFill);
#define _GxGfx_FillRect(pDestDC, pRect, uiColor)    \
	_GxGfx_Rect(pDestDC, pRect, 0, uiColor, TRUE)
#define _GxGfx_FrameRect(pDestDC, pRect, uiLineStyle, uiColor)    \
	_GxGfx_Rect(pDestDC, pRect, uiLineStyle, uiColor, FALSE)
RESULT _GxGfx_BitBlt(DC *pDestDC, const IPOINT *pDestPt, const DC *pSrcDC, const IRECT *pSrcRect);
RESULT _GxGfx_StretchBlt(DC *pDestDC, const IRECT *pDestRect, const DC *pSrcDC, const IRECT *pSrcRect);
RESULT _GxGfx_FontBitBlt(DC *pDestDC, const IPOINT *pDestPt, const DC *pSrcDC, const IRECT *pSrcRect);

#ifdef __cplusplus
} //extern "C"
#endif

#endif //_GX_DCDRAW_H_
