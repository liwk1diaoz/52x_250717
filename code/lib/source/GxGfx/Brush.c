
#include "GxGfx/GxGfx.h"
#include "DC.h"
#include "Brush.h"

#include <string.h> //for memset

//====================
//  API
//====================
//var
void   VAR_SetV(UINT8 nVarId, BVALUE nValue);
//#define VAR_Set(n, v)   VAR_SetV(n, (BVALUE)(v))
BVALUE VAR_GetV(UINT8 nVarId);
//#define VAR_Get(n)      VAR_GetV(n)
//brush
void   BR_SetP(UINT8 uiProperty, BVALUE uiValue);
//#define BR_Set(n, p)  BR_SetP(n, (BVALUE)(p))
BVALUE BR_GetP(UINT8 uiProperty);
//#define BR_Get(n)     BR_GetP(n)
//dc brush
void   GxGfx_GetAll(BRUSH *pBrush);
void   GxGfx_SetAll(const BRUSH *pBrush);
//dc move
RESULT GxGfx_MoveTo(DC *pDestDC, LVALUE x, LVALUE y);
RESULT GxGfx_MoveBack(DC *pDestDC);
IPOINT GxGfx_GetPos(const DC *pDestDC);
ISIZE GxGfx_GetLastMove(const DC *pDestDC);


//--------------------------------------------------------------------------------------
//  var
//--------------------------------------------------------------------------------------
UINT8 _gVAR_index = VARTYPE_STATE;
BVALUE _gVAR_array[2][VARARRAY_SIZE];

void    _VAR_Init(void)
{
	memset(_gVAR_array, 0, sizeof(_gVAR_array));
	VAR_SETTYPE(VARTYPE_STATE);
}

void    VAR_SetV(UINT8 nVarId, BVALUE nValue)
{
	if (nVarId > (VAR_STATE_NUM)) {
		return;
	}

	VAR_SETTYPE(VARTYPE_STATE);
	VAR_SETVALUE(nVarId, nValue);
}

BVALUE  VAR_GetV(UINT8 nVarId)
{
	if (nVarId > (VAR_STATE_NUM)) {
		return 0;
	}

	VAR_SETTYPE(VARTYPE_STATE);
	return VAR_GETVALUE(nVarId);
}


//--------------------------------------------------------------------------------------
//  brush
//--------------------------------------------------------------------------------------

BRUSH _gBRUSH;

void  _BR_Init(void)
{
	memset(&_gBRUSH, 0, sizeof(BRUSH));
}

void  BR_SetP(UINT8 uiProperty, BVALUE uiValue)
{
	if (uiProperty >= (BR_STATE_NUM)) {
		return;
	} else {
		_gBRUSH.uiState[uiProperty] = uiValue;
	}
}

BVALUE BR_GetP(UINT8 uiProperty)
{
	if (uiProperty >= (BR_STATE_NUM)) {
		return 0;
	} else {
		return _gBRUSH.uiState[uiProperty];
	}
}

void   GxGfx_GetAll(BRUSH *pBrush)
{
	if (!pBrush) {
		return;
	}
	*pBrush = _gBRUSH;
}
void   GxGfx_SetAll(const BRUSH *pBrush)
{
	if (!pBrush) {
		return;
	}
	_gBRUSH = *pBrush;
}


//--------------------------------------------------------------------------------------
//  macro function
//--------------------------------------------------------------------------------------
//shape brush
void GxGfx_SetShapeStroke(BVALUE uiLineStyle, BVALUE uiFillStyle)
{
	GxGfx_Set(BR_SHAPELINESTYLE, uiLineStyle);
	GxGfx_Set(BR_SHAPEFILLSTYLE, uiFillStyle);
}
void GxGfx_SetShapeColor(BVALUE uiForeColor, BVALUE uiBackColor, const MAPPING_ITEM *pTable)
{
	GxGfx_Set(BR_SHAPEFORECOLOR, uiForeColor);
	GxGfx_Set(BR_SHAPEBACKCOLOR, uiBackColor);
	GxGfx_Set(BR_SHAPEMAPPING, pTable);
}
void GxGfx_SetShapeLayout(BVALUE uiLayout, BVALUE uiAlignment)
{
	GxGfx_Set(BR_SHAPELAYOUT, uiLayout);
	GxGfx_Set(BR_SHAPEALIGN, uiAlignment);
}
//image brush
void GxGfx_SetImageStroke(BVALUE uiRop, BVALUE uiParam)
{
	GxGfx_Set(BR_IMAGEROP, uiRop);
	GxGfx_Set(BR_IMAGEPARAM, uiParam);
}
void GxGfx_SetImageColor(const PALETTE_ITEM *pPalette, const MAPPING_ITEM *pMapping)
{
	GxGfx_Set(BR_IMAGEPALETTE, pPalette);
	GxGfx_Set(BR_IMAGEMAPPING, pMapping);
}
void GxGfx_SetImageLayout(BVALUE uiLayout, BVALUE uiAlignment)
{
	GxGfx_Set(BR_IMAGELAYOUT, uiLayout);
	GxGfx_Set(BR_IMAGEALIGN, uiAlignment);
}
void GxGfx_SetImageTable(const IMAGE_TABLE *pTable) //NOTE: pTable is need word alignment
{
	GxGfx_Set(BR_IMAGETABLE, pTable);
}
//text brush
void GxGfx_SetTextStroke(const FONT *pFont, BVALUE uiFontStyle, BVALUE uiScale)
{
	GxGfx_Set(BR_TEXTFONT, pFont);
	GxGfx_Set(BR_TEXTFONTSTYLE, uiFontStyle);
	GxGfx_Set(BR_TEXTFONTSIZE, uiScale);
}
void GxGfx_SetTextColor(BVALUE uiForeColor1, BVALUE uiForeColor2, BVALUE uiForeColor3)
{
	GxGfx_Set(BR_TEXTFORECOLOR1, uiForeColor1);
	GxGfx_Set(BR_TEXTFORECOLOR2, uiForeColor2);
	GxGfx_Set(BR_TEXTFORECOLOR3, uiForeColor3);
}
void GxGfx_SetTextColorMapping(const PALETTE_ITEM* pColorTabl)
{
    GxGfx_Set(BR_TEXTCOLORMAPPING, pColorTabl);
}
PALETTE_ITEM* GxGfx_GetTextColorMapping(void)
{
    return (PALETTE_ITEM*)GxGfx_Get(BR_TEXTCOLORMAPPING);
}
void GxGfx_SetTextLayout(BVALUE uiLayout, BVALUE uiAlignment, BVALUE lh, BVALUE cs, BVALUE is)
{
	GxGfx_Set(BR_TEXTLAYOUT, uiLayout);
	GxGfx_Set(BR_TEXTALIGN, uiAlignment);
	GxGfx_Set(BR_TEXTLINEHEIGHT, lh);
	GxGfx_Set(BR_TEXTLETTERSPACE, cs);
	GxGfx_Set(BR_TEXTINDENTSPACE, is);
}
void GxGfx_SetStringTable(const STRING_TABLE *pTable)  //NOTE: pTable is need word alignment
{
	GxGfx_Set(BR_TEXTSTRINGTABLE, pTable);
}

void GxGfx_SetAllDefault(void)
{
	//draw shape
	GxGfx_SetShapeStroke
	(SHAPELINESTYLE_DEFAULT, SHAPEFILLSTYLE_DEFAULT);
	GxGfx_SetShapeColor
	(SHAPEFORECOLOR_DEFAULT, SHAPEBACKCOLOR_DEFAULT, SHAPEMAPPING_DEFAULT);
	GxGfx_SetShapeLayout
	(SHAPELAYOUT_DEFAULT, SHAPEALIGN_DEFAULT);
	//draw image
	GxGfx_SetImageStroke
	(IMAGEROP_DEFAULT, IMAGEPARAM_DEFAULT);
	GxGfx_SetImageColor
	(IMAGEPALETTE_DEFAULT, IMAGEMAPPING_DEFAULT);
	GxGfx_SetImageLayout
	(IMAGELAYOUT_DEFAULT, IMAGEALIGN_DEFAULT);
	GxGfx_SetImageTable
	(IMAGETABLE_DEFAULT);
	//draw text
	GxGfx_SetTextStroke
	(TEXTFONT_DEFAULT, TEXTFONTSTYLE_DEFAULT, TEXTFONTSIZE_DEFAULT);
	GxGfx_SetTextColor
	(TEXTFORECOLOR1_DEFAULT, TEXTFORECOLOR2_DEFAULT, TEXTFORECOLOR3_DEFAULT);
	GxGfx_SetTextLayout
	(TEXTLAYOUT_DEFAULT, TEXTALIGN_DEFAULT, TEXTLINEHEIGHT_DEFAULT, TEXTLETTERSPACE_DEFAULT, TEXTINDENTSPACE_DEFAULT);
	GxGfx_SetStringTable
	(TEXTSTRINGTABLE_DEFAULT);
}

