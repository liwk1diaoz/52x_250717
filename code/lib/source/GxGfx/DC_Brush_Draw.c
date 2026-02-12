
#include "GxGfx/GxGfx.h"
#include "DC.h"
#include "DC_Blt.h"
#include "GxDCDraw.h"
#include "GxGfx_int.h"
#include "GxPixel.h"

//====================
//  API
//====================
//dc draw shape
RESULT GxGfx_Point(DC *pDestDC, LVALUE x, LVALUE y);
RESULT GxGfx_LineTo(DC *pDestDC, LVALUE x, LVALUE y);
RESULT GxGfx_Line(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2);
RESULT GxGfx_FillRect(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2);
RESULT GxGfx_FrameRect(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2);
//RESULT GxGfx_InvertRect    (DC* pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2);
RESULT GxGfx_Rectangle(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2);
RESULT GxGfx_RectTo(DC *pDestDC, LVALUE x, LVALUE y);
RESULT GxGfx_Ellipse(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2);
RESULT GxGfx_RoundRect(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2, LVALUE rw, LVALUE rh);
//dc draw shape in rect
RESULT GxGfx_ShapeInRect(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2, UINT8 uiShapeType);


//--------------------------------------------------------------------------------------
//  POINT BRUSH
//--------------------------------------------------------------------------------------

extern Brush _gBrush;
static PIXEL_2D _gBrushPx;

void _DC_FillBrush_sw(DC *_pDestDC, LVALUE x, LVALUE y);
void _DC_Point_sw(DC *_pDestDC, LVALUE x, LVALUE y);

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
void _DC_SetBrush_LineWeight(DC *_pDestDC, UINT32 uiLineWeight)
{
	PIXEL_2D_Init(&_gBrushPx, _pDestDC);

	if (uiLineWeight == 0) {
		_gBrush.pfuncDraw = _DC_Point_sw;
	} else {
		_gBrush.pfuncDraw = _DC_FillBrush_sw;
	}
}


static const UINT8 brushCircle[31][31];
#if 0
static const UINT8 brushCircle_2[30][30];
#endif
static const UINT8 _DC_linestyle_num[8] = {
	1,
	2,
	5,
	7,
	9,
	12,
	4,
	7,
};
static const UINT8 _DC_linestyle_pattern[8][12] = {
	{1},                            //line style : ----------------
	{1, 0},                         //line style : . . . . . . . .
	{1, 1, 1, 1, 0},                //line style : -- -- -- -- --
	{1, 0, 1, 1, 1, 1, 0},          //line style : . -- . -- . -- .
	{1, 0, 1, 0, 1, 1, 1, 1, 0},    //line style : . . -- . . -- .
	{1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0}, //line style : . -- -- . -- --
	{1, 0, 0, 0},                   //line style : .   .   .   .
	{1, 1, 1, 1, 0, 0, 0},          //line style : --  --  --  --
};

static UINT32 _DC_Brush_count = 0;

//linestyle TODO:
// 1.work with line-weight issue
// 2.ellipse and roundrect border not continuous issue

//condition : _pDestDC is not null
//condition : pSrcBrush is not null
void _DC_Brush(DC *_pDestDC, LVALUE x, LVALUE y)
{
	//control line style
	UINT32 uiStyle = GET_LINESTYLE(_gBrush.uiLineStyle);//line-style
	UINT32 uiWeight = GET_LINEWEIGHT(_gBrush.uiLineStyle);//line-weight
	UINT32 uiIndex = _DC_Brush_count / ((uiWeight << 1) + 1);

	if (_DC_linestyle_pattern[uiStyle][uiIndex % _DC_linestyle_num[uiStyle]]) {
		//draw line with brush-type and line-weight
		(_gBrush.pfuncDraw)(_pDestDC, x, y);
	}
	_DC_Brush_count++;
}

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : pDestPt is already clipped by Rect of _pDestDC
void _DC_Point_sw(DC *_pDestDC, LVALUE x, LVALUE y)
{
	//PIXEL_2D DestPx;
	UINT32 DestValue;
	IRECT DestRect;
	IPOINT DestPt;

	POINT_Set(&DestPt, x, y);
	if (!POINT_IsInside4Points(&DestPt,
							   _gBrush.rx1, _gBrush.ry1, _gBrush.rx2, _gBrush.ry2)) {
		return;    //clip out
	}

	RECT_SetPoint(&DestRect, &DestPt);
	RECT_SetWH(&DestRect, 1, 1);

	PIXEL_2D_Begin(&_gBrushPx, &DestRect);
	PIXEL_2D_BeginLine(&_gBrushPx);
	PIXEL_2D_BeginPixel(&_gBrushPx);
	//================================
	//pixel op:
	DestValue = (UINT32)_gBrush.uiColor;    //set by uiColor
	//================================
	PIXEL_2D_Write(&_gBrushPx, DestValue);
	PIXEL_2D_EndPixel(&_gBrushPx, TRUE, FALSE);
}

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : pDestPt is not null
void _DC_FillBrush_sw(DC *_pDestDC, LVALUE x, LVALUE y)
{
	//DISPLAY_CONTEXT* pDestDC = (DISPLAY_CONTEXT*)_pDestDC;
	IRECT DestRect;
	LVALUE i, j;
	//PIXEL_2D DestPx;
	UINT32 uiBrushRadius;
	UINT32 uiBrushSize;
	UINT32 uiBrushOffset;
	UINT32 uiBrushShape;
	IPOINT DestPt;
	BOOL bSquare = FALSE;

	uiBrushSize = GET_LINEWEIGHT(_gBrush.uiLineStyle);
	if (uiBrushSize > 31) {
		uiBrushSize = 31;
	}
	uiBrushShape = GET_LINEBRUSH(_gBrush.uiLineStyle);
	if (uiBrushShape > 0) {
		bSquare = TRUE;
	}
	uiBrushSize++;
	uiBrushRadius = uiBrushSize / 2;
	uiBrushOffset = uiBrushRadius;

	if (!bSquare) { //circle
		POINT_Set(&DestPt, x - uiBrushOffset, y - uiBrushOffset);
		RECT_SetPoint(&DestRect, &DestPt);
		RECT_SetWH(&DestRect, uiBrushSize, uiBrushSize);

		if (uiBrushSize & 0x01) { //odd
			PIXEL_2D_Begin(&_gBrushPx, &DestRect);
			for (j = 0; j < DestRect.h; j++) {
				PIXEL_2D_BeginLine(&_gBrushPx);
				if (RANGE_IsInside(_gBrush.ry1, _gBrush.ry2, DestPt.y + j)) { //do clip
					for (i = 0; i < DestRect.w; i++) {
						UINT32 DestValue;
						PIXEL_2D_BeginPixel(&_gBrushPx);
						if (RANGE_IsInside(_gBrush.rx1, _gBrush.rx2, DestPt.x + i)) { //do clip
							LVALUE offi = 15 - (LVALUE)uiBrushOffset + i;
							LVALUE offj = 15 - (LVALUE)uiBrushOffset + j;
							if (offi < 0) {
								offi = 0;
							}
							if (offi > 31) {
								offi = 31;
							}
							if (offj < 0) {
								offj = 0;
							}
							if (offj > 31) {
								offj = 31;
							}
							if (brushCircle[offi][offj] <= uiBrushRadius) {
								//================================
								//pixel op: set
								DestValue = (UINT32)_gBrush.uiColor;    //set
								//================================
								PIXEL_2D_Write(&_gBrushPx, DestValue);
							}
						}
						PIXEL_2D_EndPixel(&_gBrushPx, FALSE, TRUE);
					}
				}
				PIXEL_2D_EndLine(&_gBrushPx);
			}
			PIXEL_2D_End(&_gBrushPx);
		} else { //even
			PIXEL_2D_Begin(&_gBrushPx, &DestRect);
			for (j = 0; j < DestRect.h; j++) {
				PIXEL_2D_BeginLine(&_gBrushPx);
				if (RANGE_IsInside(_gBrush.ry1, _gBrush.ry2, DestPt.y + j)) { //do clip
					for (i = 0; i < DestRect.w; i++) {
						UINT32 DestValue;
						PIXEL_2D_BeginPixel(&_gBrushPx);
						if (RANGE_IsInside(_gBrush.rx1, _gBrush.rx2, DestPt.x + i)) { //do clip
							LVALUE offi = 15 - (LVALUE)uiBrushOffset + i;
							LVALUE offj = 15 - (LVALUE)uiBrushOffset + j;
							if (offi < 0) {
								offi = 0;
							}
							if (offi > 30) {
								offi = 30;
							}
							if (offj < 0) {
								offj = 0;
							}
							if (offj > 30) {
								offj = 30;
							}
							if (brushCircle[offi][offj] <= uiBrushRadius) {
								//================================
								//pixel op: set
								DestValue = (UINT32)_gBrush.uiColor;    //set
								//================================
								PIXEL_2D_Write(&_gBrushPx, DestValue);
							}
						}
						PIXEL_2D_EndPixel(&_gBrushPx, FALSE, TRUE);
					}
				}
				PIXEL_2D_EndLine(&_gBrushPx);
			}
			PIXEL_2D_End(&_gBrushPx);
		}

	} else { //square
		POINT_Set(&DestPt, x - uiBrushOffset, y - uiBrushOffset);
		RECT_SetPoint(&DestRect, &DestPt);
		RECT_SetWH(&DestRect, uiBrushSize, uiBrushSize);

		PIXEL_2D_Begin(&_gBrushPx, &DestRect);
		for (j = 0; j < DestRect.h; j++) {
			PIXEL_2D_BeginLine(&_gBrushPx);
			if (RANGE_IsInside(_gBrush.ry1, _gBrush.ry2, DestPt.y + j)) { //do clip
				for (i = 0; i < DestRect.w; i++) {
					UINT32 DestValue;
					PIXEL_2D_BeginPixel(&_gBrushPx);
					if (RANGE_IsInside(_gBrush.rx1, _gBrush.rx2, DestPt.x + i)) { //do clip
						//================================
						//pixel op: set
						DestValue = (UINT32)_gBrush.uiColor;    //set
						//================================
						PIXEL_2D_Write(&_gBrushPx, DestValue);
					}
					PIXEL_2D_EndPixel(&_gBrushPx, FALSE, TRUE);
				}
			}
			PIXEL_2D_EndLine(&_gBrushPx);
		}
		PIXEL_2D_End(&_gBrushPx);
	}
}


//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : y,x1,x2 is validated and already clipped by _pDestDC
void _DC_LineHorizontal_sw(DC *_pDestDC, LVALUE y, LVALUE x1, LVALUE x2)
{
//    DISPLAY_CONTEXT* pDestDC = (DISPLAY_CONTEXT*)_pDestDC;
	IRECT DestRect;
	LVALUE i;

	RECT_Set(&DestRect, x1, y, x2 - x1 + 1, y);

	PIXEL_2D_Begin(&_gBrushPx, &DestRect);
	{
		PIXEL_2D_BeginLine(&_gBrushPx);
		for (i = 0; i < DestRect.w; i++) {
			UINT32 DestValue;

			PIXEL_2D_BeginPixel(&_gBrushPx);
			//================================
			DestValue = (UINT32)_gBrush.uiColor;    //set
			//================================
			PIXEL_2D_Write(&_gBrushPx, DestValue);
			PIXEL_2D_EndPixel(&_gBrushPx, FALSE, TRUE);
		}
		PIXEL_2D_EndLine(&_gBrushPx);
	}
	PIXEL_2D_End(&_gBrushPx);
}

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : x,y1,y2 is validated and already clipped by _pDestDC
void _DC_LineVertical_sw(DC *_pDestDC, LVALUE x, LVALUE y1, LVALUE y2)
{
//    DISPLAY_CONTEXT* pDestDC = (DISPLAY_CONTEXT*)_pDestDC;
	IRECT DestRect;
	LVALUE j;

	RECT_Set(&DestRect, x, y1, x, y2 - y1 + 1);

	PIXEL_2D_Begin(&_gBrushPx, &DestRect);
	for (j = 0; j < DestRect.h; j++) {
		PIXEL_2D_BeginLine(&_gBrushPx);
		{
			UINT32 DestValue;

			PIXEL_2D_BeginPixel(&_gBrushPx);
			//================================
			DestValue = (UINT32)_gBrush.uiColor;    //set
			//================================
			PIXEL_2D_Write(&_gBrushPx, DestValue);
			PIXEL_2D_EndPixel(&_gBrushPx, FALSE, TRUE);
		}
		PIXEL_2D_EndLine(&_gBrushPx);
	}
	PIXEL_2D_End(&_gBrushPx);
}


//--------------------------------------------------------------------------------------
//  draw
//--------------------------------------------------------------------------------------
static const UINT8 brushCircle[31][31] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0E, 0x0E, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0E, 0x0D, 0x0D, 0x0D, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0D, 0x0D, 0x0C, 0x0C, 0x0C, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0E, 0x0E, 0x0D, 0x0C, 0x0C, 0x0B, 0x0B, 0x0B, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0E, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0D, 0x0C, 0x0C, 0x0B, 0x0B, 0x0A, 0x0A, 0x09, 0x09, 0x09, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0B, 0x0A, 0x0A, 0x09, 0x09, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0C, 0x0B, 0x0A, 0x09, 0x09, 0x08, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0B, 0x0C, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF,
	0xFF, 0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x09, 0x08, 0x07, 0x07, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x08, 0x09, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF,
	0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x07, 0x06, 0x06, 0x05, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
	0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x07, 0x06, 0x05, 0x05, 0x04, 0x04, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
	0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x04, 0x03, 0x03, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
	0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x06, 0x05, 0x04, 0x03, 0x02, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x02, 0x01, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x02, 0x01, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x06, 0x05, 0x04, 0x03, 0x02, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x04, 0x03, 0x03, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
	0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x07, 0x06, 0x05, 0x05, 0x04, 0x04, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
	0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x07, 0x06, 0x06, 0x05, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
	0xFF, 0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x09, 0x08, 0x07, 0x07, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x08, 0x09, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF,
	0xFF, 0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0C, 0x0B, 0x0A, 0x09, 0x09, 0x08, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0B, 0x0C, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0B, 0x0A, 0x0A, 0x09, 0x09, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0D, 0x0C, 0x0C, 0x0B, 0x0B, 0x0A, 0x0A, 0x09, 0x09, 0x09, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0E, 0x0E, 0x0D, 0x0C, 0x0C, 0x0B, 0x0B, 0x0B, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0E, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0D, 0x0D, 0x0C, 0x0C, 0x0C, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0E, 0x0D, 0x0D, 0x0D, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0E, 0x0E, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

#if 0
static const UINT8 brushCircle_2[30][30] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0E, 0x0E, 0x0D, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0E, 0x0D, 0x0D, 0x0D, 0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0D, 0x0D, 0x0C, 0x0C, 0x0C, 0x0B, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0E, 0x0E, 0x0D, 0x0C, 0x0C, 0x0B, 0x0B, 0x0B, 0x0A, 0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0E, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0D, 0x0C, 0x0C, 0x0B, 0x0B, 0x0A, 0x0A, 0x09, 0x09, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0B, 0x0A, 0x0A, 0x09, 0x09, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0C, 0x0B, 0x0A, 0x09, 0x09, 0x08, 0x08, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0B, 0x0C, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF,
	0xFF, 0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x09, 0x08, 0x07, 0x07, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x08, 0x09, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF,
	0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x07, 0x06, 0x06, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
	0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x07, 0x06, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
	0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x04, 0x03, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
	0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x06, 0x05, 0x04, 0x03, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x06, 0x05, 0x04, 0x03, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x04, 0x03, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
	0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x07, 0x06, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
	0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x07, 0x06, 0x06, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
	0xFF, 0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x09, 0x08, 0x07, 0x07, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x08, 0x09, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF,
	0xFF, 0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0C, 0x0B, 0x0A, 0x09, 0x09, 0x08, 0x08, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0B, 0x0C, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0B, 0x0A, 0x0A, 0x09, 0x09, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0D, 0x0C, 0x0C, 0x0B, 0x0B, 0x0A, 0x0A, 0x09, 0x09, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0E, 0x0E, 0x0D, 0x0C, 0x0C, 0x0B, 0x0B, 0x0B, 0x0A, 0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0E, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0D, 0x0D, 0x0C, 0x0C, 0x0C, 0x0B, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0E, 0x0D, 0x0D, 0x0D, 0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0E, 0x0E, 0x0E, 0x0D, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
#endif

RESULT _GxGfx_Pixel(DC *pDestDC, const IPOINT *pPt, COLOR_ITEM uiColor)
{
	DBG_FUNC_BEGIN("\r\n");
	return _GxGfx_Line(pDestDC, pPt, pPt, GxGfx_Get(BR_SHAPELINESTYLE), uiColor);
}

RESULT GxGfx_Point(DC *pDestDC, LVALUE x, LVALUE y)
{
	RESULT r;
	IPOINT Pt;
	COLOR_ITEM fc;
	IRECT cw;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	POINT_Set(&Pt, x, y);
	_DC_XFORM_POINT(pDestDC, &Pt);
	fc = GxGfx_Get(BR_SHAPEFORECOLOR);
	{
		MAPPING_ITEM *pTable = (MAPPING_ITEM *)GxGfx_Get(BR_SHAPEMAPPING);
		_DC_MAPPING_COLOR(pDestDC, fc, pTable);
	}
	cw = GxGfx_GetWindow(pDestDC);
	//_DC_SCALE_RECT(pDestDC, &cw);
	_DC_XFORM2_RECT(pDestDC, &cw);
	_GxGfx_SetClip(&cw);
	r = _GxGfx_Pixel(pDestDC, &Pt, fc);

	GxGfx_MoveTo(pDestDC, x, y);
	return  r;
}

Brush _gBrush;

static void _DC_DummyBrush(DC *_pDestDC, LVALUE x, LVALUE y)
{
}

void _DC_SetBrush(DC *pDestDC, const IRECT *pClipRect,
				  UINT32 uiLineStyle, COLOR_ITEM uiColor)
{
	BOOL bINDEX;
	//BOOL bP3YUV;
	BOOL bPKRGB;
	BOOL bPKRGB2;
	BOOL bPKRGB3;
	BOOL bPKRGB4;

	_gBrush.uiLineStyle = uiLineStyle;
	_gBrush.uiColor = uiColor;

	_gBrush.rx1 = pClipRect->x;
	_gBrush.rx2 = pClipRect->x + pClipRect->w - 1;
	_gBrush.ry1 = pClipRect->y;
	_gBrush.ry2 = pClipRect->y + pClipRect->h - 1;

	_gBrush.uiDestType = 0; //null
	_gBrush.pfuncDraw = _DC_DummyBrush;

	if (!pDestDC) {
		return;
	}
	if (pDestDC->uiType == TYPE_JPEG) {
		return;
	}

	bINDEX = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
	bPKRGB = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK);

	bPKRGB2 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK2);

	bPKRGB3 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK3);
	bPKRGB4 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK4);

#if (GX_SUPPORT_DCFMT_RGB444)
	if (bINDEX  || bPKRGB2 || bPKRGB || bPKRGB3 || bPKRGB4)
#else
	if (bINDEX  || bPKRGB2 || bPKRGB)
#endif
	{
		_gBrush.uiDestType = 1;
		_DC_SetBrush_LineWeight(pDestDC, GET_LINEWEIGHT(_gBrush.uiLineStyle));
	} else {
		return; //not support,  skip
	}
}

RESULT _GxGfx_Line(DC *_pDestDC, const IPOINT *pStartPt, const IPOINT *pEndPt,
				   UINT32 uiLineStyle, COLOR_ITEM uiColor)
{
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	BOOL bINDEX;
	BOOL bPKRGB2;
	BOOL bPKRGB3;
	BOOL bPKRGB4;
	BOOL bPKRGB;
	IRECT DestClipRect;
	int x1, y1, x2, y2, x, y;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	if (_pDestDC->uiType == TYPE_JPEG) {
		DBG_ERR("_GxGfx_Line - ERROR! Not support with pDestDC:JPEG type.\r\n");
		return GX_ERROR_FORMAT;
	}
	if (!pStartPt) {
		return GX_NULL_POINTER;
	}
	if (!pEndPt) {
		return GX_NULL_POINTER;
	}
	bINDEX = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
	bPKRGB2 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK2);
	bPKRGB3 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK3);
	bPKRGB4 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK4);
	bPKRGB = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK);
#if (GX_SUPPORT_DCFMT_RGB444)
	if (!bINDEX && !bPKRGB2 && !bPKRGB && !bPKRGB3 && !bPKRGB4)
#else
	if (!bINDEX && !bPKRGB2 && !bPKRGB)
#endif
	{
		DBG_ERR("_GxGfx_Line - ERROR! Not support this pDestDC format. %x\r\n", pDestDC->uiPxlfmt);
		return GX_ERROR_FORMAT;
	}

	//Clipping Window
	if (_gxgfx_ctrl.pClipRect) {
		DestClipRect = *_gxgfx_ctrl.pClipRect; //custom rect
	} else {
		DestClipRect = _DC_GetRealRect(_pDestDC); //full DC
	}

	_DC_SetBrush(_pDestDC, &DestClipRect, uiLineStyle, uiColor);

	//draw line with Bresenham algorithm

	x1 = pStartPt->x;
	y1 = pStartPt->y;

	x2 = pEndPt->x;
	y2 = pEndPt->y;


#if 1
	{
		int stepx, stepy;
		int dy = y2 - y1;
		int dx = x2 - x1;
		int q;
		if (dy < 0) {
			dy = -dy;
			stepy = -1;
		} else {
			stepy = 1;
		}
		if (dx < 0) {
			dx = -dx;
			stepx = -1;
		} else {
			stepx = 1;
		}
		dx <<= 1;
		dy <<= 1;
		x = x1;
		y = y1;

		if (dx >= dy) {
			int fraction = dy - (dx >> 1);
			do {
				_DC_Brush(_pDestDC, x, y);
				q = (x == x2); //stop condition

				if (fraction >= 0) {
					y += stepy;
					fraction -= dx;
				}
				x += stepx;
				fraction += dy;
			} while (!q);
		} else {
			int fraction = dx - (dy >> 1);
			do {
				_DC_Brush(_pDestDC, x, y);
				q = (y == y2); //stop condition

				if (fraction >= 0) {
					x += stepx;
					fraction -= dy;
				}
				y += stepy;
				fraction += dx;
			} while (!q);
		}
	}
#else
	{
		int deltax, deltay;
		int xinc1, xinc2;
		int yinc1, yinc2;
		int den, num, numpixels, numadd, curpixel;

		deltax = _ABS(x2 - x1);         // The difference between the x's
		deltay = _ABS(y2 - y1);         // The difference between the y's
		x = x1;                         // Start x off at the first pixel
		y = y1;                         // Start y off at the first pixel

		if (x2 >= x1) {                 // The x-values are increasing
			xinc1 = 1;
			xinc2 = 1;
		} else {                        // The x-values are decreasing
			xinc1 = -1;
			xinc2 = -1;
		}

		if (y2 >= y1) {                 // The y-values are increasing
			yinc1 = 1;
			yinc2 = 1;
		} else {                        // The y-values are decreasing
			yinc1 = -1;
			yinc2 = -1;
		}

		if (deltax >= deltay) {         // There is at least one x-value for every y-value
			xinc1 = 0;                  // Don't change the x when numerator >= denominator
			yinc2 = 0;                  // Don't change the y for every iteration
			den = deltax;
			num = deltax >> 1;
			numadd = deltay;
			numpixels = deltax;         // There are more x-values than y-values
		} else {                        // There is at least one y-value for every x-value
			xinc2 = 0;                  // Don't change the x for every iteration
			yinc1 = 0;                  // Don't change the y when numerator >= denominator
			den = deltay;
			num = deltay >> 1;
			numadd = deltax;
			numpixels = deltay;         // There are more y-values than x-values
		}

		for (curpixel = 0; curpixel <= numpixels; curpixel++) {
			// Draw the current pixel
			_DC_Brush(_pDestDC, x, y);

			num += numadd;              // Increase the numerator by the top of the fraction
			if (num >= den) {           // Check if numerator >= denominator
				num -= den;             // Calculate the new numerator value
				x += xinc1;             // Change the x as appropriate
				y += yinc1;             // Change the y as appropriate
			}
			x += xinc2;                 // Change the x as appropriate
			y += yinc2;                 // Change the y as appropriate
		}
	}
#endif
	return GX_OK;
}

RESULT GxGfx_LineTo(DC *pDestDC, LVALUE x, LVALUE y)
{
	RESULT r;
	IPOINT Pt;
	IPOINT lastPt;
	COLOR_ITEM fc;
	IRECT cw;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	POINT_Set(&Pt, x, y);
	_DC_XFORM_POINT(pDestDC, &Pt);
	POINT_SetPoint(&lastPt, &(pDestDC->thisPt));
	_DC_XFORM_POINT(pDestDC, &lastPt);
	fc = GxGfx_Get(BR_SHAPEFORECOLOR);
	{
		MAPPING_ITEM *pTable = (MAPPING_ITEM *)GxGfx_Get(BR_SHAPEMAPPING);
		_DC_MAPPING_COLOR(pDestDC, fc, pTable);
	}
	cw = GxGfx_GetWindow(pDestDC);
	//_DC_SCALE_RECT(pDestDC, &cw);
	_DC_XFORM2_RECT(pDestDC, &cw);
	_GxGfx_SetClip(&cw);
	r = _GxGfx_Line(pDestDC, &lastPt, &Pt,
					GxGfx_Get(BR_SHAPELINESTYLE), fc);
	GxGfx_MoveTo(pDestDC, x, y);
	return r;
}

RESULT GxGfx_Line(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2)
{
	//RESULT r;
	IPOINT Pt;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	POINT_Set(&Pt, x1, y1);
	GxGfx_MoveTo(pDestDC, x1, y1);
	return GxGfx_LineTo(pDestDC, x2, y2);
}

RESULT GxGfx_RectTo(DC *pDestDC, LVALUE x, LVALUE y)
{
	UINT32 uiRectStyle = GxGfx_Get(BR_SHAPEFILLSTYLE);
	IPOINT Pt;
	IRECT DestRect;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	Pt = GxGfx_GetPos(pDestDC);
	RECT_SetX1Y1X2Y2(&DestRect, Pt.x, Pt.y, x, y);
	if (!(uiRectStyle & FILLSTYLE_EMPTY)) {
		COLOR_ITEM bc;
		IRECT cw;
		IRECT xfmRect = DestRect;
		_DC_XFORM_RECT(pDestDC, &xfmRect);
		bc = GxGfx_Get(BR_SHAPEBACKCOLOR);
		{
			MAPPING_ITEM *pTable = (MAPPING_ITEM *)GxGfx_Get(BR_SHAPEMAPPING);
			_DC_MAPPING_COLOR(pDestDC, bc, pTable);
		}
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);
		_GxGfx_FillRect(pDestDC, &xfmRect, bc);
	}
	if (!(uiRectStyle & FILLSTYLE_NOBORDER)) {
		COLOR_ITEM fc;
		IRECT xfmRect = DestRect;
		IRECT cw;
		_DC_XFORM_RECT(pDestDC, &xfmRect);
		fc = GxGfx_Get(BR_SHAPEFORECOLOR);
		{
			MAPPING_ITEM *pTable = (MAPPING_ITEM *)GxGfx_Get(BR_SHAPEMAPPING);
			_DC_MAPPING_COLOR(pDestDC, fc, pTable);
		}
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);
		_GxGfx_FrameRect(pDestDC, &xfmRect, GxGfx_Get(BR_SHAPELINESTYLE), fc);
	}
	GxGfx_MoveTo(pDestDC, Pt.x, Pt.y);
	GxGfx_MoveTo(pDestDC, x, y);
	return GX_OK;
}

RESULT GxGfx_FillRect(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2)
{
	IRECT DestRect;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	RECT_SetX1Y1X2Y2(&DestRect, x1, y1, x2, y2);
	{
		COLOR_ITEM bc;
		IRECT cw;
		IRECT xfmRect = DestRect;
		_DC_XFORM_RECT(pDestDC, &xfmRect);
		bc = GxGfx_Get(BR_SHAPEBACKCOLOR);
		{
			MAPPING_ITEM *pTable = (MAPPING_ITEM *)GxGfx_Get(BR_SHAPEMAPPING);
			_DC_MAPPING_COLOR(pDestDC, bc, pTable);
		}
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);
		_GxGfx_FillRect(pDestDC, &xfmRect, bc);
	}
	GxGfx_MoveTo(pDestDC, x1, y1);
	GxGfx_MoveTo(pDestDC, x2, y2);
	return GX_OK;
}

RESULT _GxGfx_Rect(DC *_pDestDC, const IRECT *pDestRect, UINT32 uiLineStyle, COLOR_ITEM uiColor, BOOL bFill)
{
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	BOOL bINDEX;
	BOOL bPKRGB2;
	BOOL bPKRGB3;
	BOOL bPKRGB4;
	BOOL bPKRGB;
	IPOINT Pt1, Pt2, Pt3, Pt4;
	RESULT r = GX_OK;
	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	if (_pDestDC->uiType == TYPE_JPEG) {
		DBG_ERR("_GxGfx_Rect - ERROR! Not support with pDestDC:JPEG type.\r\n");
		return GX_ERROR_FORMAT;
	}
	if (!pDestRect) {
		return GX_NULL_POINTER;
	}

	if (bFill) {
		_GxGfx_SetImageROP(ROP_DESTFILL, uiColor, 0, 0);
		r = _GxGfx_BitBlt(_pDestDC, 0, _pDestDC, pDestRect);
		if (r != GX_OK) {
			DBG_ERR("_GxGfx_Rect - ERROR! General error.\r\n");
		}
	} else {
		bINDEX = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
		bPKRGB2 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK2);
		bPKRGB3 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK3);
		bPKRGB4 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK4);
		bPKRGB = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK);
#if (GX_SUPPORT_DCFMT_RGB444)
		if (!bINDEX && !bPKRGB2 && !bPKRGB && !bPKRGB3 && !bPKRGB4)
#else
		if (!bINDEX && !bPKRGB2 && !bPKRGB)
#endif

		{
			DBG_ERR("_GxGfx_Rect - ERROR! Not support this pDestDC format.\r\n");
			return GX_ERROR_FORMAT;
		}
		POINT_Set(&Pt1, pDestRect->x, pDestRect->y);
		POINT_Set(&Pt2, pDestRect->x + pDestRect->w - 1, pDestRect->y);
		POINT_Set(&Pt3, pDestRect->x, pDestRect->y + pDestRect->h - 1);
		POINT_Set(&Pt4, pDestRect->x + pDestRect->w - 1, pDestRect->y + pDestRect->h - 1);
		_GxGfx_Line(_pDestDC, &Pt1, &Pt2, uiLineStyle, uiColor);
		_GxGfx_Line(_pDestDC, &Pt3, &Pt4, uiLineStyle, uiColor);
		_GxGfx_Line(_pDestDC, &Pt1, &Pt3, uiLineStyle, uiColor);
		_GxGfx_Line(_pDestDC, &Pt2, &Pt4, uiLineStyle, uiColor);
	}
	return r;
}

RESULT GxGfx_FrameRect(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2)
{
	IRECT DestRect;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	RECT_SetX1Y1X2Y2(&DestRect, x1, y1, x2, y2);
	{
		COLOR_ITEM fc;
		IRECT xfmRect = DestRect;
		IRECT cw;
		_DC_XFORM_RECT(pDestDC, &xfmRect);
		fc = GxGfx_Get(BR_SHAPEFORECOLOR);
		{
			MAPPING_ITEM *pTable = (MAPPING_ITEM *)GxGfx_Get(BR_SHAPEMAPPING);
			_DC_MAPPING_COLOR(pDestDC, fc, pTable);
		}
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);
		_GxGfx_FrameRect(pDestDC, &xfmRect, GxGfx_Get(BR_SHAPELINESTYLE), fc);
	}
	GxGfx_MoveTo(pDestDC, x1, y1);
	GxGfx_MoveTo(pDestDC, x2, y2);
	return GX_OK;
}
/*
RESULT GxGfx_InvertRect      (DC* pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2)
{
    IRECT DestRect;

    DBG_FUNC_BEGIN("\r\n");
    if(!pDestDC)
        return GX_NULL_POINTER;
    GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

    RECT_SetX1Y1X2Y2(&DestRect, x1, y1, x2, y2);
    {
        IRECT xfmRect = DestRect;
        IRECT cw;
        _DC_XFORM_RECT(pDestDC, &xfmRect);
        cw = GxGfx_GetWindow(pDestDC);
        //_DC_SCALE_RECT(pDestDC, &cw);
        _DC_XFORM2_RECT(pDestDC, &cw);
        _GxGfx_SetClip(&cw);
        _GxGfx_SetImageROP(ROP_DESTNOT, 0, 0, 0);
        _GxGfx_BitBlt(pDestDC, 0, pDestDC, &xfmRect);
    }
    GxGfx_MoveTo(pDestDC, x1, y1);
    GxGfx_MoveTo(pDestDC, x2, y2);
    return GX_OK;
}
*/
RESULT GxGfx_Rectangle(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2)
{
	UINT32 uiRectStyle = GxGfx_Get(BR_SHAPEFILLSTYLE);
	IRECT DestRect;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	RECT_SetX1Y1X2Y2(&DestRect, x1, y1, x2, y2);
	if (!(uiRectStyle & FILLSTYLE_EMPTY)) {
		COLOR_ITEM bc;
		IRECT xfmRect = DestRect;
		IRECT cw;
		_DC_XFORM_RECT(pDestDC, &xfmRect);
		bc = GxGfx_Get(BR_SHAPEBACKCOLOR);
		{
			MAPPING_ITEM *pTable = (MAPPING_ITEM *)GxGfx_Get(BR_SHAPEMAPPING);
			_DC_MAPPING_COLOR(pDestDC, bc, pTable);
		}
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);
		_GxGfx_FillRect(pDestDC, &xfmRect, bc);
	}
	if (!(uiRectStyle & FILLSTYLE_NOBORDER)) {
		COLOR_ITEM fc;
		IRECT xfmRect = DestRect;
		IRECT cw;
		_DC_XFORM_RECT(pDestDC, &xfmRect);
		fc = GxGfx_Get(BR_SHAPEFORECOLOR);
		{
			MAPPING_ITEM *pTable = (MAPPING_ITEM *)GxGfx_Get(BR_SHAPEMAPPING);
			_DC_MAPPING_COLOR(pDestDC, fc, pTable);
		}
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);
		_GxGfx_FrameRect(pDestDC, &xfmRect, GxGfx_Get(BR_SHAPELINESTYLE), fc);
	}
	GxGfx_MoveTo(pDestDC, x1, y1);
	GxGfx_MoveTo(pDestDC, x2, y2);
	return GX_OK;
}

typedef struct _Quadrant {
	IPOINT CenterPt;
	IPOINT ShiftPt;
	BOOL bEvenX;
	BOOL bEvenY;
	LVALUE sx, sy;
} Quadrant;

static void _DC_Quadrant4Points(DC *_pDestDC, const Quadrant *pQ)
{
	LVALUE y, x1, x2;
	y = pQ->CenterPt.y - pQ->ShiftPt.y;
	x1 = pQ->CenterPt.x - pQ->ShiftPt.x;
	x2 = pQ->CenterPt.x + pQ->ShiftPt.x + pQ->sx;
	if (pQ->bEvenX) { //even
		x2 --;
	}
	_DC_Brush(_pDestDC, x1, y); //point in quadrant 3
	_DC_Brush(_pDestDC, x2, y); //point in quadrant 4
	y = pQ->CenterPt.y + pQ->ShiftPt.y + pQ->sy;
	x1 = pQ->CenterPt.x - pQ->ShiftPt.x;
	x2 = pQ->CenterPt.x + pQ->ShiftPt.x + pQ->sx;
	if (pQ->bEvenX) { //even
		x2 --;
	}
	if (pQ->bEvenY) { //even
		y --;
	}
	_DC_Brush(_pDestDC, x1, y); //point in quadrant 2
	_DC_Brush(_pDestDC, x2, y); //point in quadrant 1
}

static void _DC_Quadrant2Lines(DC *_pDestDC, const Quadrant *pQ)
{
	LVALUE y, x1, x2;
	//line from quadrant 3 to quadrant 4
	y = pQ->CenterPt.y - pQ->ShiftPt.y;
	x1 = pQ->CenterPt.x - pQ->ShiftPt.x;
	x2 = pQ->CenterPt.x + pQ->ShiftPt.x + pQ->sx;
	if (pQ->bEvenX) { //even
		x2 --;
	}
	if (RANGE_IsInside(_gBrush.ry1, _gBrush.ry2, y)) {
//        if(RANGE_IsInside(_gBrush.rx1, _gBrush.rx2, x1) ||
//            RANGE_IsInside(_gBrush.rx1, _gBrush.rx2, x2))
		if (((x1 < _gBrush.rx1) && (x2 < _gBrush.rx1)) ||
			((x1 > _gBrush.rx2) && (x2 > _gBrush.rx2))) {
			//skip
		} else {
			x1 = RANGE_Clamp(_gBrush.rx1, _gBrush.rx2, x1);
			x2 = RANGE_Clamp(_gBrush.rx1, _gBrush.rx2, x2);
			_DC_LineHorizontal_sw(_pDestDC, y, x1, x2);
		}
	}
	//line from quadrant 2 to quadrant 1
	y = pQ->CenterPt.y + pQ->ShiftPt.y + pQ->sy;
	x1 = pQ->CenterPt.x - pQ->ShiftPt.x;
	x2 = pQ->CenterPt.x + pQ->ShiftPt.x + pQ->sx;
	if (pQ->bEvenX) { //even
		x2 --;
	}
	if (pQ->bEvenY) { //even
		y --;
	}
	if (RANGE_IsInside(_gBrush.ry1, _gBrush.ry2, y)) {
//        if(RANGE_IsInside(_gBrush.rx1, _gBrush.rx2, x1) ||
//            RANGE_IsInside(_gBrush.rx1, _gBrush.rx2, x2))
		if (((x1 < _gBrush.rx1) && (x2 < _gBrush.rx1)) ||
			((x1 > _gBrush.rx2) && (x2 > _gBrush.rx2))) {
			//skip
		} else {
			x1 = RANGE_Clamp(_gBrush.rx1, _gBrush.rx2, x1);
			x2 = RANGE_Clamp(_gBrush.rx1, _gBrush.rx2, x2);
			_DC_LineHorizontal_sw(_pDestDC, y, x1, x2);
		}
	}
}

//(dx, dy): shift vector to support drawing RoundRect
RESULT _GxGfx_Ellipse(DC *_pDestDC, const IRECT *pRect, const IPOINT *pRadius,
					  UINT32 uiLineStyle, COLOR_ITEM uiColor,
					  BOOL bFill)
{
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	BOOL bINDEX;
	BOOL bPKRGB2;
	BOOL bPKRGB3;
	BOOL bPKRGB4;
	BOOL bPKRGB;
	IRECT Rect;
	IPOINT cPt;
	//LVALUE rx1,ry1,rx2,ry2;
	IRECT DestClipRect;
	Quadrant q;
	LVALUE dx, dy;
	void (*pfnDrawQuadrant)(DC * _pDestDC, const Quadrant * pQ) = 0;

	//int CX, CY, XRadius, YRadius;
	int XRadius, YRadius;
	int X, Y;
	int XChange, YChange;
	int EllipseError;
	int TwoASquare, TwoBSquare;
	int StoppingX, StoppingY;

	BOOL bGapH, bGapV;
	LVALUE AX, BX, DY;
	LVALUE AY, BY, DX;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	if (_pDestDC->uiType == TYPE_JPEG) {
		DBG_ERR("_GxGfx_Ellipse - ERROR! Not support with pDestDC:JPEG type.\r\n");
		return GX_ERROR_FORMAT;
	}
	if (!pRect) {
		return GX_NULL_POINTER;
	}
	bINDEX = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
	bPKRGB2 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK2);
	bPKRGB3 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK3);
	bPKRGB4 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK4);
	bPKRGB = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK);
#if (GX_SUPPORT_DCFMT_RGB444)
	if (!bINDEX && !bPKRGB2 && !bPKRGB && !bPKRGB3 && !bPKRGB4)
#else
	if (!bINDEX && !bPKRGB2 && !bPKRGB)
#endif
	{
		DBG_ERR("_GxGfx_Ellipse - ERROR! Not support this pDestDC format.\r\n");
		return GX_ERROR_FORMAT;
	}

	//Clipping Window
	if (_gxgfx_ctrl.pClipRect) {
		DestClipRect = *_gxgfx_ctrl.pClipRect; //custom rect
	} else {
		DestClipRect = _DC_GetRealRect(_pDestDC); //full DC
	}
	if ((DestClipRect.w == 0) || (DestClipRect.h == 0)) {
		return GX_OK;
	}

	if ((pRect->w <= 1) && (pRect->h <= 1)) { //degraded to rectangle
		IRECT rt;
		RECT_Set(&rt, pRect->x, pRect->y, pRadius->x, pRadius->y);
		if (bFill) {
			_GxGfx_SetClip(&DestClipRect);
			return _GxGfx_FillRect(_pDestDC, &rt, uiColor);
		} else {
			return _GxGfx_FrameRect(_pDestDC, &rt, uiLineStyle, uiColor);
		}
	}

	if (bFill) {
		pfnDrawQuadrant = _DC_Quadrant2Lines;
	} else {
		pfnDrawQuadrant = _DC_Quadrant4Points;
	}
	_DC_SetBrush(_pDestDC, &DestClipRect, uiLineStyle, uiColor);

	Rect = *pRect;
	RECT_Normalize(&Rect);
	cPt = RECT_GetCenterPoint(&Rect);
	//CX = cPt.x; CY = cPt.y;
	dx = pRadius->x / 2;
	dy = pRadius->y / 2;
	POINT_Set(&q.CenterPt, cPt.x + dx, cPt.y + dy);
	XRadius = (pRect->w >> 1);
	YRadius = (pRect->h >> 1);
	q.bEvenX = !(pRect->w % 2);
	q.bEvenY = !(pRect->h % 2);
	q.sx = pRadius->x % 2;
	q.sy = pRadius->y % 2;

	//draw ellipse based on Bresenham algorithm (by John Kennedy)

	TwoASquare = 2 * XRadius * XRadius;
	TwoBSquare = 2 * YRadius * YRadius;

	AX = AY = 0;
	BX = 0;
	DX = DY = 0;

	if (pRect->w > pRect->h) { //horizontal case
		//start the 1st set of points
		X = XRadius;
		Y = 0;
		XChange = YRadius * YRadius * (1 - 2 * XRadius);
		YChange = XRadius * XRadius;
		EllipseError = 0;
		StoppingX = TwoBSquare * XRadius;
		StoppingY = 0;

		while (StoppingX >= StoppingY) {    //1st set of points, yw>1
			{
				POINT_Set(&q.ShiftPt, X + dx, Y + dy);
				(*pfnDrawQuadrant)(_pDestDC, &q);
			}
			Y++;
			StoppingY += TwoASquare;
			EllipseError += YChange;
			YChange += TwoASquare;
			if ((2 * EllipseError + XChange) > 0) {
				X--;
				StoppingX -= TwoBSquare;
				EllipseError += XChange;
				XChange += TwoBSquare;
			}
		}
		//1st point set is done

		bGapH = FALSE;
		if (!bFill) {
			Quadrant *pQ = &q;
			//LVALUE y, x;
			LVALUE y;
			POINT_Set(&q.ShiftPt, X + dx, Y + dy);
			y = pQ->CenterPt.y - pQ->ShiftPt.y;
			//x = pQ->CenterPt.x-pQ->ShiftPt.x;

			//detect horizontal gap
			if ((pQ->CenterPt.y - y) <= 2) {
				AX = X;
				DY = (pQ->CenterPt.y - y) - 1;
				bGapH = TRUE;
			}
		}

		//start the 2nd set of points
		X = 0;
		Y = YRadius;
		XChange = YRadius * YRadius;
		YChange = XRadius * XRadius * (1 - 2 * YRadius);
		EllipseError = 0;
		StoppingX = 0;
		StoppingY = TwoASquare * YRadius;

		while (StoppingX <= StoppingY) {    //2nd set of points, yw<1
			{
				POINT_Set(&q.ShiftPt, X + dx, Y + dy);
				(*pfnDrawQuadrant)(_pDestDC, &q);
			}
			X++;
			StoppingX += TwoBSquare;
			EllipseError += XChange;
			XChange += TwoBSquare;;
			if ((2 * EllipseError + YChange) > 0) {
				Y--;
				StoppingY -= TwoASquare;
				EllipseError += YChange;
				YChange += TwoASquare;
			}
		}
		//2nd point set is done

		//patch horizontal gap
		if (bGapH) {
			int i;
			BX = X;
			X = AX;
			for (i = 0; i < (AX - BX) + 1; i++) {
				{
					POINT_Set(&q.ShiftPt, X + dx, DY + dy);
					(*pfnDrawQuadrant)(_pDestDC, &q);
				}
				X--;
			}
		}

	} else { //veritcal case
		//start the 2nd set of points
		X = 0;
		Y = YRadius;
		XChange = YRadius * YRadius;
		YChange = XRadius * XRadius * (1 - 2 * YRadius);
		EllipseError = 0;
		StoppingX = 0;
		StoppingY = TwoASquare * YRadius;

		while (StoppingX <= StoppingY) {    //2nd set of points, yw<1
			{
				POINT_Set(&q.ShiftPt, X + dx, Y + dy);
				(*pfnDrawQuadrant)(_pDestDC, &q);
			}
			X++;
			StoppingX += TwoBSquare;
			EllipseError += XChange;
			XChange += TwoBSquare;;
			if ((2 * EllipseError + YChange) > 0) {
				Y--;
				StoppingY -= TwoASquare;
				EllipseError += YChange;
				YChange += TwoASquare;
			}
		}
		//2nd point set is done

		bGapV = FALSE;
		if (!bFill) {
			Quadrant *pQ = &q;
			//LVALUE y, x;
			LVALUE x;
			POINT_Set(&q.ShiftPt, X + dx, Y + dy);
			//y = pQ->CenterPt.y-pQ->ShiftPt.y;
			x = pQ->CenterPt.x - pQ->ShiftPt.x;

			//detect vertical gap
			if ((pQ->CenterPt.x - x) <= 2) {
				AY = Y;
				DX = (pQ->CenterPt.x - x) - 1;
				bGapV = TRUE;
			}
		}

		//start the 1st set of points
		X = XRadius;
		Y = 0;
		XChange = YRadius * YRadius * (1 - 2 * XRadius);
		YChange = XRadius * XRadius;
		EllipseError = 0;
		StoppingX = TwoBSquare * XRadius;
		StoppingY = 0;

		while (StoppingX >= StoppingY) {    //1st set of points, yw>1
			{
				POINT_Set(&q.ShiftPt, X + dx, Y + dy);
				(*pfnDrawQuadrant)(_pDestDC, &q);
			}
			Y++;
			StoppingY += TwoASquare;
			EllipseError += YChange;
			YChange += TwoASquare;
			if ((2 * EllipseError + XChange) > 0) {
				X--;
				StoppingX -= TwoBSquare;
				EllipseError += XChange;
				XChange += TwoBSquare;
			}
		}
		//1st point set is done

		//patch vertical gap
		if (bGapV) {
			int i;
			BY = Y;
			Y = AY;
			for (i = 0; i < (AY - BY) + 1; i++) {
				{
					POINT_Set(&q.ShiftPt, DX + dx, Y + dy);
					(*pfnDrawQuadrant)(_pDestDC, &q);
				}
				Y--;
			}
		}

	}
	return GX_OK;
}

RESULT GxGfx_Ellipse(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2)
{
	UINT32 uiRectStyle = GxGfx_Get(BR_SHAPEFILLSTYLE);
	IRECT DestRect;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	RECT_SetX1Y1X2Y2(&DestRect, x1, y1, x2, y2);
	if (!(uiRectStyle & FILLSTYLE_EMPTY)) {
		COLOR_ITEM bc;
		IRECT xfmRect = DestRect;
		IRECT cw;
		IPOINT rd;
		POINT_Set(&rd, 0, 0);
		_DC_XFORM_RECT(pDestDC, &xfmRect);
		bc = GxGfx_Get(BR_SHAPEBACKCOLOR);
		{
			MAPPING_ITEM *pTable = (MAPPING_ITEM *)GxGfx_Get(BR_SHAPEMAPPING);
			_DC_MAPPING_COLOR(pDestDC, bc, pTable);
		}
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);
		_GxGfx_FillEllipse(pDestDC, &xfmRect, &rd, bc);
	}
	if (!(uiRectStyle & FILLSTYLE_NOBORDER)) {
		COLOR_ITEM fc;
		IRECT xfmRect = DestRect;
		IRECT cw;
		IPOINT rd;
		POINT_Set(&rd, 0, 0);
		_DC_XFORM_RECT(pDestDC, &xfmRect);
		fc = GxGfx_Get(BR_SHAPEFORECOLOR);
		{
			MAPPING_ITEM *pTable = (MAPPING_ITEM *)GxGfx_Get(BR_SHAPEMAPPING);
			_DC_MAPPING_COLOR(pDestDC, fc, pTable);
		}
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);
		_GxGfx_FrameEllipse(pDestDC, &xfmRect, &rd, GxGfx_Get(BR_SHAPELINESTYLE), fc);
	}
	GxGfx_MoveTo(pDestDC, x1, y1);
	GxGfx_MoveTo(pDestDC, x2, y2);
	return GX_OK;
}

RESULT GxGfx_RoundRect(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2, LVALUE rw, LVALUE rh)
{
	UINT32 uiRectStyle = GxGfx_Get(BR_SHAPEFILLSTYLE);
	IRECT DestRect;
	LVALUE cx, cy;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	RECT_SetX1Y1X2Y2(&DestRect, x1, y1, x2, y2);
	if ((DestRect.w <= 2)  || (DestRect.h <= 2)) {
		return GxGfx_Rectangle(pDestDC, x1, y1, x2, y2);
	}
	if (rw < 0) {
		rw = -rw;
	}
	if (rh < 0) {
		rh = -rh;
	}
	rw = RANGE_Clamp(0, DestRect.w, rw);
	rh = RANGE_Clamp(0, DestRect.h, rh);
	if (!(uiRectStyle & FILLSTYLE_EMPTY)) {
		COLOR_ITEM bc;
		IRECT xfmRect = DestRect;
		IRECT cw;
		IRECT ellipseRect;
		IRECT fillRect;
		IPOINT rd;
		_DC_XFORM_RECT(pDestDC, &xfmRect);
		cx = (xfmRect.w - rw);
		cy = (xfmRect.h - rh);
		POINT_Set(&rd, cx, cy);
		bc = GxGfx_Get(BR_SHAPEBACKCOLOR);
		{
			MAPPING_ITEM *pTable = (MAPPING_ITEM *)GxGfx_Get(BR_SHAPEMAPPING);
			_DC_MAPPING_COLOR(pDestDC, bc, pTable);
		}
		//draw round part
		RECT_Set(&ellipseRect,
				 xfmRect.x,
				 xfmRect.y,
				 rw,
				 rh);
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);
		_GxGfx_FillEllipse(pDestDC, &ellipseRect, &rd, bc);
		//draw rect part
		RECT_Set(&fillRect,
				 xfmRect.x,
				 xfmRect.y + rh / 2,
				 xfmRect.w,
				 xfmRect.h - rh);
		_GxGfx_FillRect(pDestDC, &fillRect, bc);
	}
	if (!(uiRectStyle & FILLSTYLE_NOBORDER)) {
		COLOR_ITEM fc;
		IRECT xfmRect = DestRect;
		IRECT cw;
		IRECT ellipseRect;
		IPOINT border1StartPt;
		IPOINT border1EndPt;
		IPOINT border2StartPt;
		IPOINT border2EndPt;
		IPOINT rd;
		_DC_XFORM_RECT(pDestDC, &xfmRect);
		cx = (xfmRect.w - rw);
		cy = (xfmRect.h - rh);
		POINT_Set(&rd, cx, cy);
		//draw round part
		RECT_Set(&ellipseRect,
				 xfmRect.x,
				 xfmRect.y,
				 rw,
				 rh);
		fc = GxGfx_Get(BR_SHAPEFORECOLOR);
		{
			MAPPING_ITEM *pTable = (MAPPING_ITEM *)GxGfx_Get(BR_SHAPEMAPPING);
			_DC_MAPPING_COLOR(pDestDC, fc, pTable);
		}
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);
		_GxGfx_FrameEllipse(pDestDC, &ellipseRect, &rd, GxGfx_Get(BR_SHAPELINESTYLE), fc);
		//draw rect part
		POINT_Set(&border1StartPt, xfmRect.x, xfmRect.y + rh / 2);
		POINT_Set(&border1EndPt,   xfmRect.x, xfmRect.y + xfmRect.h - 1 - rh / 2);
		POINT_Set(&border2StartPt, xfmRect.x + xfmRect.w - 1, xfmRect.y + rh / 2);
		POINT_Set(&border2EndPt,   xfmRect.x + xfmRect.w - 1, xfmRect.y + xfmRect.h - 1 - rh / 2);
		_GxGfx_Line(pDestDC, &border1StartPt, &border1EndPt, GxGfx_Get(BR_SHAPELINESTYLE), fc);
		_GxGfx_Line(pDestDC, &border2StartPt, &border2EndPt, GxGfx_Get(BR_SHAPELINESTYLE), fc);
		POINT_Set(&border1StartPt, xfmRect.x + rw / 2, xfmRect.y);
		POINT_Set(&border1EndPt,   xfmRect.x + xfmRect.w - 1 - rw / 2, xfmRect.y);
		POINT_Set(&border2StartPt, xfmRect.x + rw / 2, xfmRect.y + xfmRect.h - 1);
		POINT_Set(&border2EndPt,   xfmRect.x + xfmRect.w - 1 - rw / 2, xfmRect.y + xfmRect.h - 1);
		_GxGfx_Line(pDestDC, &border1StartPt, &border1EndPt, GxGfx_Get(BR_SHAPELINESTYLE), fc);
		_GxGfx_Line(pDestDC, &border2StartPt, &border2EndPt, GxGfx_Get(BR_SHAPELINESTYLE), fc);
	}
	GxGfx_MoveTo(pDestDC, x1, y1);
	GxGfx_MoveTo(pDestDC, x2, y2);
	return GX_OK;
}

RESULT GxGfx_ConvertFont(DC *_pDestDC,  const DC *_pSrcDC, const PALETTE_ITEM *pColorTable)
{
    RESULT r=0;
	_GxGfx_SetImageROP(ROP_COPY, 0, pColorTable, 0);
	_GxGfx_SetClip(0); //disable
	r = _GxGfx_BitBlt(_pDestDC, 0, _pSrcDC, 0);   //convert INDEX8 to YUV444
    return r;
}

/*
//dc draw shape in rect
RESULT GxGfx_ShapeInRect        (DC* pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2, UINT8 uiShapeType)
{
    LVALUE dv=0;
    DBG_FUNC_BEGIN("\r\n");
    if(x1 == x2)
        return GX_EMPTY_RECT;
    if(y1 == y2)
        return GX_EMPTY_RECT;

    switch(uiShapeType)
    {
    case SHAPE_NONE : return GX_OK;
    case SHAPE_FILLRECT : return GxGfx_FillRect(pDestDC, x1, y1, x2, y2);
    case SHAPE_FRAMERECT : return GxGfx_FrameRect(pDestDC, x1, y1, x2, y2);
    case SHAPE_INVERTRECT : return GxGfx_InvertRect(pDestDC, x1, y1, x2, y2);
    case SHAPE_RECTANGLE : return GxGfx_Rectangle(pDestDC, x1, y1, x2, y2);
    case SHAPE_ELLIPSE : return GxGfx_Ellipse(pDestDC, x1, y1, x2, y2);
    case SHAPE_LINE_TOP : return GxGfx_Line(pDestDC, x1, y1, x2, y1);
    case SHAPE_LINE_BOTTOM : return GxGfx_Line(pDestDC, x1, y2, x2, y2);
    case SHAPE_LINE_LEFT : return GxGfx_Line(pDestDC, x1, y1, x1, y2);
    case SHAPE_LINE_RIGHT : return GxGfx_Line(pDestDC, x2, y1, x2, y2);
    case SHAPE_LINE_BACKSLASH : return GxGfx_Line(pDestDC, x1, y1, x2, y2); // '\'
    case SHAPE_LINE_SLASH : return GxGfx_Line(pDestDC, x2, y1, x1, y2); // '/'
    case SHAPE_ROUNDRECT_1 : dv = 1; break;
    case SHAPE_ROUNDRECT_2 : dv = 2; break;
    case SHAPE_ROUNDRECT_3 : dv = 3; break;
    case SHAPE_ROUNDRECT_4 : dv = 4; break;
    case SHAPE_ROUNDRECT_5 : dv = 5; break;
    case SHAPE_ROUNDRECT_6 : dv = 6; break;
    case SHAPE_ROUNDRECT_7 : dv = 7; break;
    case SHAPE_ROUNDRECT_8 : dv = 8; break;
    }
    if(dv)
    {
        LVALUE rw, rh;
        rw = (x2-x1+1)/dv; rh = (y2-y1+1)/dv;
        return GxGfx_RoundRect(pDestDC, x1, y1, x2, y2, rw, rh);
    }
    return GX_OK;
}
*/

#if 0

COLOR_ITEM _GxGfx_QueryPoint(DC *pDestDC, LVALUE x, LVALUE y)
{
}
RESULT _GxGfx_Point(DC *pDestDC, LVALUE x, LVALUE y, COLOR_ITEM color)
{
}
RESULT GxGfx_SetGradient(const IRECT *pRect)
{
}

RESULT GxGfx_FillAt(DC *pDestDC, LVALUE x, LVALUE y)
{
	COLOR_ITEM c = _GxGfx_QueryPoint(pDestDC, x, y);
	COLOR_ITEM seed = GxGfx_Get(BR_SHAPEFORECOLOR);
	int x1, x2, y1, y2;
	int i, j;
	if (c == seed) {
		//find y1
		c = seed;
		for (i = y; (i >= 0) & (c == seed); i--) {
			c = _GxGfx_QueryPoint(pDestDC, x, i);
		}
		y1 = i;
		//find y2
		c = seed;
		for (i = y; (i < 240) & (c == seed); i++) {
			c = _GxGfx_QueryPoint(pDestDC, x, i);
		}
		y2 = i;
		for (j = y1; j <= y2; j++) {
			//find x1
			c = seed;
			for (i = x; (i >= 0) & (c == seed); i--) {
				c = _GxGfx_QueryPoint(pDestDC, i, j);
			}
			x1 = i;
			//find x2
			c = seed;
			for (i = x; (i < 320) & (c == seed); i++) {
				c = _GxGfx_QueryPoint(pDestDC, i, j);
			}
			x2 = i;
			for (i = x1; i <= x2; i++) {
				_GxGfx_FillPoint(pDestDC, i, j);
			}
		}
	}
}

#define GRADIENT_MAKE(type, iBegin, iEnd)

#define GRADIENT_GET_TYPE(color)
#define GRADIENT_GET_BEGIN(color)
#define GRADIENT_GET_END(color)

#define GRADFILL_NONE       0
#define GRADFILL_LINEAR_X   1
#define GRADFILL_LINEAR_Y   2
#define GRADFILL_LINEAR     3
#define GRADFILL_RADIAL     4
#define GRADFILL_ANGLE      5
#define GRADFILL_REFLECTED  6
#define GRADFILL_DIAMOND    7

#endif

