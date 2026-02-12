
#include "GxGfx/GxGfx.h"
#include "DC.h"
#include "Brush.h"
#include "TextRender.h"
#include "TextString.h"
#include "GxDCDraw.h"
#include "GxGfx_int.h"
#include "DC_Blt.h"

#define GX_TEST_FONTEFFECT  0
#define FONTEFFECT_DEF      FONTEFFECT_OUTLINE

MAPPING_ITEM _gDC_TextMapping[4];

//====================
//  API
//====================
RESULT GxGfx_EscImage(DC *pDestDC, LVALUE x, LVALUE y, IVALUE id_image);
RESULT _GxGfx_DummyImage(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2);

//--------------------------------------------------------------------------------------
//  DC draw char and esc-image
//--------------------------------------------------------------------------------------

RESULT GxGfx_GetFontSize(ISIZE *pFontSize)
{
	RESULT r = GX_OK;
	IMAGE_TABLE *pImageTable;
	IVALUE i;
	GXGFX_ASSERT(pFontSize);
	if (!pFontSize) {
		return GX_NULL_POINTER;
	}
	pImageTable = (IMAGE_TABLE *)GxGfx_Get(BR_TEXTFONT);
	SIZE_Set(pFontSize, 0, 0);
	if (pImageTable == 0) {
		return GX_NO_FONT;
	}

	i = 0;
	do {
		ISIZE TmpSz;
		r = IMAGE_GetSizeFromTable(pImageTable, i, &TmpSz);
		if (r == GX_OK) {
			pFontSize->h = TmpSz.h;
		}
		i++;
	} while ((i < 256) && (pFontSize->h == 0));

	pFontSize->w = pFontSize->h + pFontSize->h; //max-width = 2*h
	return r;
}

RESULT GxGfx_GetFontCharSize(IVALUE id, ISIZE *pCharSize) //single char
{
	IMAGE_TABLE *pImageTable;
	GXGFX_ASSERT(pCharSize);
	if (!pCharSize) {
		return GX_NULL_POINTER;
	}
	SIZE_Set(pCharSize, 0, 0);
	pImageTable = (IMAGE_TABLE *)GxGfx_Get(BR_TEXTFONT);
	if (!pImageTable) {
		return GX_NO_FONT;
	}
	return IMAGE_GetSizeFromTable(pImageTable, id, pCharSize);
}

//condition : pTable != null
//condition : id is in the range of table
RESULT STRING_GetLengthFromTable(const STRING_TABLE *pTable, IVALUE id, INT16 *pStringLen)
{
	RESULT r = GX_OK;
	GXGFX_ASSERT(pTable);
	GXGFX_ASSERT(pStringLen);
	r = _STR_GetLen(pTable, id, pStringLen);
	if (r != GX_OK) {
		*pStringLen = 0;
	}
	return r;
}

//condition : pTable != null
//condition : id is in the range of table
const TCHAR *STRING_LoadFromTable(const STRING_TABLE *pTable, IVALUE id)
{
	RESULT r = GX_OK;
	const TCHAR *str = 0;
	GXGFX_ASSERT(pTable);
	r = _STR_MakeStr(pTable, id, &str);
	if (r != GX_OK) {
		str = 0;
	}
	return str;
}

static void _GxGfx_TextPosShift(IPOINT *pPt)
{
	UINT32 iFontEffect;
	if (!pPt) {
		return;
	}
	iFontEffect = GxGfx_Get(BR_TEXTFONTSTYLE) & DSF_FE_MASK;
#if (GX_TEST_FONTEFFECT)
	iFontEffect = FONTEFFECT_DEF;
#endif
	if (iFontEffect == FONTEFFECT_NONE) {
	} else if (iFontEffect == FONTEFFECT_SHADOW) {
	} else if (iFontEffect == FONTEFFECT_SHADOW2) {
	} else if (iFontEffect == FONTEFFECT_OUTLINE) {
		pPt->x += 1;
		pPt->y += 1;
	} else if (iFontEffect == FONTEFFECT_OUTLINE2) {
		pPt->x += 2;
		pPt->y += 1;
	}
}

static RESULT _GxGfx_FontEffect(DC *_pDestDC, const DC *_pSrcDC, UINT32 iFontEffect)
{
	BOOL bINDEX;

	DBG_FUNC_BEGIN("\r\n");
	if (!_pDestDC) {
		return GX_NULL_POINTER;
	}
	if (!_pSrcDC) {
		return GX_NULL_POINTER;
	}
	//check format
	bINDEX = ((_pSrcDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
	if (!bINDEX) {
		return GX_ERROR_FORMAT;
	}
	bINDEX = ((_pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
	if (!bINDEX) {
		return GX_ERROR_FORMAT;
	}

	return _DC_FontEffectBlt_INDEX_sw(_pDestDC, _pSrcDC, iFontEffect);
}

static void _GxGfx_TextRectShrink(IRECT *pRect)
{
	UINT32 iFontEffect;
	if (!pRect) {
		return;
	}
	iFontEffect = GxGfx_Get(BR_TEXTFONTSTYLE) & DSF_FE_MASK;
#if (GX_TEST_FONTEFFECT)
	iFontEffect = FONTEFFECT_DEF;
#endif
	if (iFontEffect == FONTEFFECT_NONE) {
	} else if (iFontEffect == FONTEFFECT_SHADOW) {
		pRect->w -= 1;
		pRect->h -= 1;
	} else if (iFontEffect == FONTEFFECT_SHADOW2) {
		pRect->w -= 2;
		pRect->h -= 2;
	} else if (iFontEffect == FONTEFFECT_OUTLINE) {
		pRect->w -= 2;
		pRect->h -= 2;
		pRect->x += 1;
		pRect->y += 1;
	} else if (iFontEffect == FONTEFFECT_OUTLINE2) {
		pRect->w -= 4;
		pRect->h -= 2;
		pRect->x += 2;
		pRect->y += 1;
	}
}

static RESULT _GxGfx_TextChar(DC *pDestDC, LVALUE x, LVALUE y, IVALUE id) //single char
{
	RESULT r = GX_OK;
	IPOINT Pt;
	IMAGE_TABLE *pImageTable;
	UINT32 iFontEffect;
	DC ImageDC;
	DC *pSrcDC;
	DC TempDC;
	//int effect;
	IPOINT dv;
	BOOL bINDEX;
	BOOL bPKRGB2;
	BOOL bPKRGB3;
	BOOL bPKRGB4;
	BOOL bPKRGB;
	BOOL bTemp;
    PALETTE_ITEM* pColorTable;
	UINT32 colorTansport = 0;
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}

	bINDEX = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
	bPKRGB2 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK2);
	bPKRGB3 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK3);
	bPKRGB4 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK4);
	bPKRGB = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK);
	if (!bINDEX && !bPKRGB2 && !bPKRGB && !bPKRGB3 && !bPKRGB4) {
		DBG_ERR("_GxGfx_TextChar - ERROR! Not support this pDestDC format.\r\n");
		return GX_ERROR_FORMAT;
	}

	pImageTable = (IMAGE_TABLE *)GxGfx_Get(BR_TEXTFONT);
	//pMapping = (MAPPING_ITEM*)GxGfx_Get(BR_TEXTMAPPING);
	r = _ICON_MakeDC(&ImageDC, pImageTable, id);
	if (r != GX_OK) {
		_GxGfx_DummyImage(pDestDC, x, y, x + 16 - 1, y + 24 - 1); // for draw dummy character by _GxGfx_DummyImage()
		return r;
	}

	iFontEffect = GxGfx_Get(BR_TEXTFONTSTYLE) & DSF_FE_MASK;
#if (GX_TEST_FONTEFFECT)
	iFontEffect = FONTEFFECT_DEF;
#endif

	//effect = DSF_FE(iFontEffect);
	dv = _DC_GetFontEffectExtend(iFontEffect);

    pColorTable = (PALETTE_ITEM*)GxGfx_Get(BR_TEXTCOLORMAPPING);

    if(!pColorTable)
    {
		if(bPKRGB2) //565
			colorTansport = (COLOR_TRANSPARENT&0x0000ffff);
		else if(bPKRGB3) //4444
			colorTansport = (COLOR_TRANSPARENT&0x00000fff);
		else if(bPKRGB3)//1555
			colorTansport = (COLOR_TRANSPARENT&0x00007fff);
		else
			colorTansport = COLOR_TRANSPARENT;
	_gDC_TextMapping[0] = colorTansport;    //always cut by colorkey
	_gDC_TextMapping[1] = (MAPPING_ITEM)GxGfx_Get(BR_TEXTFORECOLOR1);
	_gDC_TextMapping[2] = (MAPPING_ITEM)GxGfx_Get(BR_TEXTFORECOLOR2);
	_gDC_TextMapping[3] = (MAPPING_ITEM)GxGfx_Get(BR_TEXTFORECOLOR3);
    }
	pSrcDC =  &ImageDC;
	bTemp = FALSE;

	if (iFontEffect == FONTEFFECT_NONE) {
	} else {
		//alloc buffer for font effect
		r = GxGfx_PushStackDC(&TempDC,
							  TYPE_BITMAP,
							  PXLFMT_INDEX8,
							  //pDestDC->uiPxlfmt,
							  ImageDC.size.w + dv.x,
							  ImageDC.size.h + dv.y);
		//make font effect
		if (r == GX_OK) {
			bTemp = TRUE;
			r = _GxGfx_FontEffect(&TempDC, &ImageDC, iFontEffect);
			if (r == GX_OK) {
				pSrcDC = (DC *)&TempDC;
			}
		}
		if (r != GX_OK) {
			DBG_WRN("GxGfx_Char - WARNING! Failed to make font effect. Ignore effect.\r\n");
		}
	}
	POINT_Set(&Pt, x, y);

    if(pColorTable) {
        _GxGfx_SetImageROP(ROP_FONT, 0, pColorTable, 0);
    } else {
        _GxGfx_SetImageROP(ROP_FONT, 0, (PALETTE_ITEM*)_gDC_TextMapping, 0);
    }
    //draw font with src-key OP, colorkey =0
    r = _GxGfx_FontBitBlt(pDestDC, &Pt, pSrcDC, 0);

	if (bTemp) {
		//free buffer for font effect
		GxGfx_PopStackDC(pSrcDC);
	}

	GxGfx_MoveTo(pDestDC, x + pSrcDC->size.w, y + pSrcDC->size.h);

	return r;
}



static RESULT _GxGfx_EscImage(DC *pDestDC, LVALUE x, LVALUE y, IVALUE id) //single esc-image
{
	RESULT r = GX_OK;
	IPOINT Pt;
	DC ImageDC;
	IMAGE_TABLE *pImageTable;
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}

	pImageTable = (IMAGE_TABLE *)GxGfx_Get(BR_IMAGETABLE);
	r = _ICON_MakeDC(&ImageDC, pImageTable, id);
	if (r != GX_OK) {
		_GxGfx_DummyImage(pDestDC, x, y, x + 16 - 1, y + 24 - 1); // for draw dummy character by _GxGfx_DummyImage()
		return r;
	}

	POINT_Set(&Pt, x, y);
	//draw icon with src-key OP, colorkey = BR_IMAGEPARAM
	_GxGfx_SetImageROP(ROP_KEY, GxGfx_Get(BR_IMAGEPARAM), (PALETTE_ITEM *)GxGfx_Get(BR_IMAGEPALETTE), (MAPPING_ITEM *)GxGfx_Get(BR_IMAGEMAPPING));
	_GxGfx_BitBlt(pDestDC, &Pt, &ImageDC, 0);

	GxGfx_MoveTo(pDestDC, x + ImageDC.size.w, y + ImageDC.size.h);

	return GX_OK;
}

RESULT GxGfx_Char(DC *pDestDC, LVALUE x, LVALUE y, IVALUE id) //single char
{
	IPOINT Pt;
	IRECT cw;
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	POINT_Set(&Pt, x, y);
	_GxGfx_TextPosShift(&Pt);
	_DC_XFORM_POINT(pDestDC, &Pt);
	cw = GxGfx_GetWindow(pDestDC);
	//_DC_SCALE_RECT(pDestDC, &cw);
	_DC_XFORM2_RECT(pDestDC, &cw);
	_GxGfx_SetClip(&cw);

	return _GxGfx_TextChar(pDestDC, Pt.x, Pt.y, id);
}

RESULT GxGfx_EscImage(DC *pDestDC, LVALUE x, LVALUE y, IVALUE id) //single esc-image
{
	IPOINT Pt;
	IRECT cw;
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	POINT_Set(&Pt, x, y);
	_DC_XFORM_POINT(pDestDC, &Pt);
	cw = GxGfx_GetWindow(pDestDC);
	//_DC_SCALE_RECT(pDestDC, &cw);
	_DC_XFORM2_RECT(pDestDC, &cw);
	_GxGfx_SetClip(&cw);

	//draw icon with src-key OP, colorkey = BR_IMAGEPARAM
	return _GxGfx_EscImage(pDestDC, Pt.x, Pt.y, id);
}



//--------------------------------------------------------------------------------------
//  DC draw Text
//--------------------------------------------------------------------------------------

#include <stdio.h>
//#include <stdlib.h>
#include <stdarg.h>
//#include <string.h>

RESULT _DC_sprintf(DC *pDestDC, TCHAR *pszDest, const TCHAR *pszSrc, ...);
/*
-----------------------------------------------------------------------------------------------
    * %[ctrl] : support variable function arguments
        if it is a %[fmt] => sprintf to pszDest
        if it is a %[cmd] => encode to pszDest as stack array parameter $[cmd][n]
    * ^[ctrl][n] : support ESC array arguments
        if it is a ^[fmt][n] => sprintf to pszDest
        if it is a ^[cmd][n] => bypass (keep ^[cmd][n] and copy to pszDest)
    * $[ctrl][n] : (not support)
-----------------------------------------------------------------------------------------------
*/


/*
-----------------------------------------------------------------------------------------------
    * %[ctrl] : (not support)
    * $[ctrl][n] : support stack array parameter
        if it is a $[cmd][n] => decode and issue this command
    * ^[ctrl][n] : support ESC array parameter
        if it is a ^[cmd][n] => decode and issue this command
-----------------------------------------------------------------------------------------------
*/






//-----------------------------------------------------------------------------

/*
image:table
image:pid
image:id
text:table
text:pid
text:id

image:mapping
image:keycolor
text:font
text:fontstyle
text:scale
text:forecolor1
text:forecolor2
text:forecolor3

text:layout
text:align
text:lineheight
text:letterspace
text:indentspace
*/

#ifndef __LINT__
//#define FMT_DECLARE(symbol, string)     CHAR_TYPE FMT_##symbol[] = { string }
#define FMT_DECLARE(symbol)             CHAR_TYPE FMT_##symbol[]
#endif
#define FMT(symbol)                     ((const TCHAR*)(FMT_##symbol))
#define LEN(symbol)                     ((sizeof(FMT_##symbol)/sizeof(CHAR_TYPE))-1)

/*
//--------------------------------------------
FMT_DECLARE(STAGE1, _T(" 0123456789.+-*plh"));
FMT_DECLARE(STAGE2, _T("%cdiouxXfs"));
FMT_DECLARE(PTEXTID, _T("text:pid"));
FMT_DECLARE(TEXTID, _T("text:id"));
FMT_DECLARE(TEXTSTRINGTABLE, _T("text:table")); //BR_TEXTSTRINGTABLE
FMT_DECLARE(TEXT_CMD, _T("text:"));
FMT_DECLARE(IMAGE_CMD, _T("image:"));
//--------------------------------------------
FMT_DECLARE(PIMAGEID, _T("image:pid"));
FMT_DECLARE(IMAGEID, _T("image:id"));
FMT_DECLARE(IMAGEMAPPING, _T("image:mapping"));     //BR_IMAGEMAPPING
FMT_DECLARE(IMAGEKEYCOLOR, _T("image:keycolor"));    //BR_IMAGEPARAM <---- ! BR_IMAGEROP = ROP_KEY
FMT_DECLARE(IMAGETABLE, _T("image:table"));   //BR_IMAGETABLE
FMT_DECLARE(TEXTFONT, _T("text:font"));        //BR_TEXTFONT
FMT_DECLARE(TEXTFONTSTYLE, _T("text:fontstyle"));   //BR_TEXTFONTSTYLE
FMT_DECLARE(TEXTFONTSIZE, _T("text:fontsize"));       //BR_TEXTFONTSIZE
FMT_DECLARE(TEXTFORECOLOR1, _T("text:forecolor1"));  //BR_TEXTFORECOLOR1
FMT_DECLARE(TEXTFORECOLOR2, _T("text:forecolor2"));  //BR_TEXTFORECOLOR2
FMT_DECLARE(TEXTFORECOLOR3, _T("text:forecolor3"));  //BR_TEXTFORECOLOR3
//--------------------------------------------
FMT_DECLARE(TEXTLAYOUT, _T("text:layout"));      //BR_TEXTLAYOUT
FMT_DECLARE(TEXTALIGN, _T("text:align"));       //BR_TEXTALIGN
FMT_DECLARE(TEXTLINEHEIGHT, _T("text:lineheight"));  //BR_TEXTLINEHEIGHT
FMT_DECLARE(TEXTLETTERSPACE, _T("text:letterspace")); //BR_TEXTLETTERSPACE
FMT_DECLARE(TEXTINDENTSPACE, _T("text:indentspace")); //BR_TEXTINDENTSPACE
//--------------------------------------------
*/

// !!! Convert Ascii String to Wide String by this converter:
// !!! http://rishida.net/tools/conversion/
//--------------------------------------------
FMT_DECLARE(STAGE1) =          {0x0020, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x002E, 0x002B, 0x002D, 0x002A, 0x0070, 0x006C, 0x0068, 0x0000};
FMT_DECLARE(STAGE2) =          {0x0069, 0x006F, 0x0075, 0x0078, 0x0058, 0x0066, 0x0073, 0x0000};
FMT_DECLARE(PTEXTID) =         {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x0070, 0x0069, 0x0064, 0x0000};
FMT_DECLARE(TEXTID) =          {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x0069, 0x0064, 0x0000};
FMT_DECLARE(TEXTSTRINGTABLE) = {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x0074, 0x0061, 0x0062, 0x006C, 0x0065, 0x0000}; //BR_TEXTSTRINGTABLE
FMT_DECLARE(TEXT_CMD) =        {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x0000};
FMT_DECLARE(IMAGE_CMD) =       {0x0069, 0x006D, 0x0061, 0x0067, 0x0065, 0x003A, 0x0000};
//--------------------------------------------
FMT_DECLARE(PIMAGEID) =        {0x0069, 0x006D, 0x0061, 0x0067, 0x0065, 0x003A, 0x0070, 0x0069, 0x0064, 0x0000};
FMT_DECLARE(IMAGEID) =         {0x0069, 0x006D, 0x0061, 0x0067, 0x0065, 0x003A, 0x0069, 0x0064, 0x0000};
FMT_DECLARE(IMAGEMAPPING) =    {0x0069, 0x006D, 0x0061, 0x0067, 0x0065, 0x003A, 0x006D, 0x0061, 0x0070, 0x0070, 0x0069, 0x006E, 0x0067, 0x0000}; //BR_IMAGEMAPPING
FMT_DECLARE(IMAGEKEYCOLOR) =   {0x0069, 0x006D, 0x0061, 0x0067, 0x0065, 0x003A, 0x006B, 0x0065, 0x0079, 0x0063, 0x006F, 0x006C, 0x006F, 0x0072, 0x0000}; //BR_IMAGEPARAM <---- ! BR_IMAGEROP = ROP_KEY
FMT_DECLARE(IMAGETABLE) =      {0x0069, 0x006D, 0x0061, 0x0067, 0x0065, 0x003A, 0x0074, 0x0061, 0x0062, 0x006C, 0x0065, 0x0000}; //BR_IMAGETABLE
FMT_DECLARE(TEXTFONT) =        {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x0066, 0x006F, 0x006E, 0x0074, 0x0000}; //BR_TEXTFONT
FMT_DECLARE(TEXTFONTSTYLE) =   {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x0066, 0x006F, 0x006E, 0x0074, 0x0073, 0x0074, 0x0079, 0x006C, 0x0065, 0x0000}; //BR_TEXTFONTSTYLE
FMT_DECLARE(TEXTFONTSIZE) =    {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x0066, 0x006F, 0x006E, 0x0074, 0x0073, 0x0069, 0x007A, 0x0065, 0x0000}; //BR_TEXTFONTSIZE
FMT_DECLARE(TEXTFORECOLOR1) =  {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x0066, 0x006F, 0x0072, 0x0065, 0x0063, 0x006F, 0x006C, 0x006F, 0x0072, 0x0031, 0x0000}; //BR_TEXTFORECOLOR1
FMT_DECLARE(TEXTFORECOLOR2) =  {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x0066, 0x006F, 0x0072, 0x0065, 0x0063, 0x006F, 0x006C, 0x006F, 0x0072, 0x0032, 0x0000}; //BR_TEXTFORECOLOR2
FMT_DECLARE(TEXTFORECOLOR3) =  {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x0066, 0x006F, 0x0072, 0x0065, 0x0063, 0x006F, 0x006C, 0x006F, 0x0072, 0x0033, 0x0000}; //BR_TEXTFORECOLOR3
//--------------------------------------------
FMT_DECLARE(TEXTLAYOUT) =      {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x006C, 0x0061, 0x0079, 0x006F, 0x0075, 0x0074, 0x0000}; //BR_TEXTLAYOUT
FMT_DECLARE(TEXTALIGN) =       {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x0061, 0x006C, 0x0069, 0x0067, 0x006E, 0x0000}; //BR_TEXTALIGN
FMT_DECLARE(TEXTLINEHEIGHT) =  {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x006C, 0x0069, 0x006E, 0x0065, 0x0068, 0x0065, 0x0069, 0x0067, 0x0068, 0x0074, 0x0000}; //BR_TEXTLINEHEIGHT
FMT_DECLARE(TEXTLETTERSPACE) = {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x006C, 0x0065, 0x0074, 0x0074, 0x0065, 0x0072, 0x0073, 0x0070, 0x0061, 0x0063, 0x0065, 0x0000}; //BR_TEXTLETTERSPACE
FMT_DECLARE(TEXTINDENTSPACE) = {0x0074, 0x0065, 0x0078, 0x0074, 0x003A, 0x0069, 0x006E, 0x0064, 0x0065, 0x006E, 0x0074, 0x0073, 0x0070, 0x0061, 0x0063, 0x0065, 0x0000}; //BR_TEXTINDENTSPACE
//--------------------------------------------

#define SYMBOL_STACK(fmt, i)  0x0024, FMT_##fmt, (i)   //$
#define SYMBOL_PARAM(fmt, i)  0x0025, FMT_##fmt, (i)   //%
#define SYMBOL_STATE(fmt, i)  0x005E, FMT_##fmt, (i)   //^
#define WCHAR_money     0x0024, 0x0024
#define WCHAR_percent   0x0025, 0x0025
#define WCHAR_power     0x005E, 0x005E


//-----------------------------------------------------------------------------



BRUSH _gBRUSH_old;
//BRUSH _gBRUSH_text;

static INT16 _String_DecodeNumber(const TCHAR *pszSrc, INT16 *pNumber)
{
	INT16 num = -1;
	BOOL bIsNumber = TRUE;
	INT16 iCount;
	TCHAR_VALUE c;

	iCount = 0;

	c = _String_GetChar(pszSrc, iCount);

	while (c && bIsNumber) {
		INT16 cn = (INT16)(c - '0');
		if ((cn >= 0) && (cn <= 9)) {
			if (num == -1) {
				num = cn;
			} else {
				num = num * 10 + cn;
			}
		} else {
			bIsNumber = 0;
		}

		if (bIsNumber) {
			iCount++;
			c = _String_GetChar(pszSrc, iCount);
		}
	}

	*pNumber = num;

	return iCount;
}


#define CTRL_VALUE_LONG     0x0001
#define CTRL_VALUE_INT      0x0002
#define CTRL_VALUE_FLOAT    0x0004
#define CTRL_VALUE_DOUBLE   0x0008
#define CTRL_VALUE_TEXT     0x0010
#define CTRL_VALUE_TEXTID   0x0020
#define CTRL_VALUE_TEXTDB   0x0040
#define CTRL_VALUE_IMAGEID  0x0080
#define CTRL_VALUE_CPAR     0x0100
#define CTRL_VALUE_CESC     0x0200
#define CTRL_VALUE_CMD      0x1000  //ctrl = command
#define CTRL_VALUE_FMT      0x2000  //ctrl = conversion format
#define CTRL_VALUE_PTR      0x8000

typedef struct _VAR_CMD {
	UINT16 uiFlag;
	UINT16 uiCommand;
	UINT32 uiValue;
} VAR_CMD;

static INT16 _String_DecodeCommandWithEscValue(const TCHAR *pszSrc, VAR_CMD *pEscCmd)
{
	INT16 i = 0; //read offset of pszSrc
	BOOL nPointerType;
	INT16 iCount;
	INT16 iState;
	TCHAR_VALUE c;
	const TCHAR *pCurSrc;

	nPointerType = FALSE;

	i++;
	pCurSrc = _String_GetConstCharPtr(pszSrc, i);

	c = _String_GetChar(pCurSrc, 0);
	if (c == 'p') {
		nPointerType = TRUE;
		i++;
		pCurSrc = _String_GetConstCharPtr(pszSrc, i);
	}

	iCount = 0;
	if (!_String_N_Compare(pCurSrc, FMT(IMAGEID), LEN(IMAGEID))) {
		pEscCmd->uiFlag |= (CTRL_VALUE_FMT | CTRL_VALUE_IMAGEID);
		iCount +=  LEN(IMAGEID);
	} else if (!_String_N_Compare(pCurSrc, FMT(PIMAGEID), LEN(PIMAGEID))) {
		pEscCmd->uiFlag |= (CTRL_VALUE_FMT | CTRL_VALUE_IMAGEID | CTRL_VALUE_PTR);
		iCount +=  LEN(PIMAGEID);
	} else if (!_String_N_Compare(pCurSrc, FMT(IMAGETABLE), LEN(IMAGETABLE))) {
		pEscCmd->uiFlag |= CTRL_VALUE_CMD;
		pEscCmd->uiCommand = BR_IMAGETABLE;
		iCount +=  LEN(IMAGETABLE);
	} else if (!_String_N_Compare(pCurSrc, FMT(IMAGEMAPPING), LEN(IMAGEMAPPING))) {
		pEscCmd->uiFlag |= CTRL_VALUE_CMD;
		pEscCmd->uiCommand = BR_IMAGEMAPPING;
		iCount +=  LEN(IMAGEMAPPING);
	} else if (!_String_N_Compare(pCurSrc, FMT(TEXTFONT), LEN(TEXTFONT))) {
		pEscCmd->uiFlag |= CTRL_VALUE_CMD;
		pEscCmd->uiCommand = BR_TEXTFONT;
		iCount +=  LEN(TEXTFONT);
	} else if (!_String_N_Compare(pCurSrc, FMT(TEXTFONTSTYLE), LEN(TEXTFONTSTYLE))) {
		pEscCmd->uiFlag |= CTRL_VALUE_CMD;
		pEscCmd->uiCommand = BR_TEXTFONTSTYLE;
		iCount +=  LEN(TEXTFONTSTYLE);
	} else if (!_String_N_Compare(pCurSrc, FMT(TEXTFORECOLOR1), LEN(TEXTFORECOLOR1))) {
		pEscCmd->uiFlag |= CTRL_VALUE_CMD;
		pEscCmd->uiCommand = BR_TEXTFORECOLOR1;
		iCount +=  LEN(TEXTFORECOLOR1);
	} else if (!_String_N_Compare(pCurSrc, FMT(TEXTFORECOLOR2), LEN(TEXTFORECOLOR2))) {
		pEscCmd->uiFlag |= CTRL_VALUE_CMD;
		pEscCmd->uiCommand = BR_TEXTFORECOLOR2;
		iCount +=  LEN(TEXTFORECOLOR2);
	} else if (!_String_N_Compare(pCurSrc, FMT(TEXTFORECOLOR3), LEN(TEXTFORECOLOR3))) {
		pEscCmd->uiFlag |= CTRL_VALUE_CMD;
		pEscCmd->uiCommand = BR_TEXTFORECOLOR3;
		iCount +=  LEN(TEXTFORECOLOR3);
	} else if (!_String_N_Compare(pCurSrc, FMT(TEXTLAYOUT), LEN(TEXTLAYOUT))) {
		pEscCmd->uiFlag |= CTRL_VALUE_CMD;
		pEscCmd->uiCommand = BR_TEXTLAYOUT;
		iCount +=  LEN(TEXTLAYOUT);
	} else if (!_String_N_Compare(pCurSrc, FMT(TEXTALIGN), LEN(TEXTALIGN))) {
		pEscCmd->uiFlag |= CTRL_VALUE_CMD;
		pEscCmd->uiCommand = BR_TEXTALIGN;
		iCount +=  LEN(TEXTALIGN);
	} else if (!_String_N_Compare(pCurSrc, FMT(TEXTLETTERSPACE), LEN(TEXTLETTERSPACE))) {
		pEscCmd->uiFlag |= CTRL_VALUE_CMD;
		pEscCmd->uiCommand = BR_TEXTLETTERSPACE;
		iCount +=  LEN(TEXTLETTERSPACE);
	} else if (!_String_N_Compare(pCurSrc, FMT(TEXTINDENTSPACE), LEN(TEXTINDENTSPACE))) {
		pEscCmd->uiFlag |= CTRL_VALUE_CMD;
		pEscCmd->uiCommand = BR_TEXTINDENTSPACE;
		iCount +=  LEN(TEXTINDENTSPACE);
	} else if (!_String_N_Compare(pCurSrc, FMT(TEXTLINEHEIGHT), LEN(TEXTLINEHEIGHT))) {
		pEscCmd->uiFlag |= CTRL_VALUE_CMD;
		pEscCmd->uiCommand = BR_TEXTLINEHEIGHT;
		iCount +=  LEN(TEXTLINEHEIGHT);
	} else {
		return 0;
	}

	i += iCount;
	pCurSrc = _String_GetConstCharPtr(pszSrc, i);
	iCount = _String_DecodeNumber(pCurSrc, &iState);

	if (iState == -1) {
		return 0;
	}

	i += iCount; //pCurSrc = _String_GetConstCharPtr(pszSrc, i);

	if (nPointerType) {
		pEscCmd->uiValue = *(UINT32 *)VAR_GETVALUE(iState);
	} else {
		pEscCmd->uiValue = (UINT32)VAR_GETVALUE(iState);
	}

	return i;
}

///////////////////////////////////////////////////////////////////////////////

static void _TextRender_Begin(TextRender *pRender, UINT32 ScreenObj)
{
	pRender->ScreenObj = ScreenObj;

	//Save state (keep initial state)
	GxGfx_GetAll(&_gBRUSH_old);
//    _gBRUSH_text = _gBRUSH_old;
}
static void _TextRender_End(TextRender *pRender)
{
	//Load state (restore to initial state)
	GxGfx_SetAll(&_gBRUSH_old);
}

static UINT16 _TextRender_ParseStringCmd(const TCHAR *pszDest, INT16 i, ESCCMD *pCmd)
{
	VAR_CMD EscCmd;
	const TCHAR *pCurSrc;
	TCHAR_VALUE c;

	pCurSrc = _String_GetConstCharPtr(pszDest, i);
	EscCmd.uiFlag = 0;
	c = _String_GetChar(pCurSrc, 0);

	EscCmd.uiValue = 0; //set initial value
	EscCmd.uiCommand = 0; //set initial value

	if ((c == '^') || (c == '$')) {
		INT16 iCount;
		if (c == '^') {
			VAR_SETTYPE(VARTYPE_STATE);    //'^'
		} else {
			VAR_SETTYPE(VARTYPE_STACK);    //'$'
		}

		iCount = _String_DecodeCommandWithEscValue(pCurSrc, &EscCmd);

		i += iCount;
	}

	pCmd->bDecodeImage = FALSE;
	pCmd->bEscIcon = FALSE;
	pCmd->value = 0;
	if (EscCmd.uiFlag == (CTRL_VALUE_CMD)) {
		pCmd->value = (BVALUE)(EscCmd.uiValue);
		//kkk
		GxGfx_Set((UINT8)EscCmd.uiCommand, pCmd->value);
	} else if ((EscCmd.uiFlag & (CTRL_VALUE_FMT | CTRL_VALUE_IMAGEID)) == (CTRL_VALUE_FMT | CTRL_VALUE_IMAGEID)) {
		//draw esc icon ==>id by pImageTable

		if (EscCmd.uiFlag & CTRL_VALUE_PTR) {
			pCmd->id = *(IVALUE *)EscCmd.uiValue; //piid
		} else {
			pCmd->id = (IVALUE)EscCmd.uiValue; //iid
		}

		pCmd->bDecodeImage = TRUE;
		pCmd->bEscIcon = TRUE;
	} else { //EscCmd.uiFlag == 0 (normal char)
		//draw char icon ==>uiFontID by pFontTable

		pCmd->id = c;

		i++;

		pCmd->bDecodeImage = TRUE;
	}

	return i;
}

static RESULT _TextRender_GetCharSize(IVALUE id, ISIZE *pCharSize, BOOL bEscIcon)
{
	if (bEscIcon == TRUE) {
		return GxGfx_GetImageIDSize(id, pCharSize);
	} else {
		return GxGfx_GetFontCharSize(id, pCharSize);
	}
}

static RESULT _TextRender_DrawChar(UINT32 ScreenObj, LVALUE x, LVALUE y, IVALUE id, BOOL bEscIcon)
{
	DC *pDestDC = (DC *)ScreenObj;
	IPOINT Pt;
	IRECT cw;

	POINT_Set(&Pt, x, y);
	_DC_XFORM_POINT(pDestDC, &Pt);
	cw = GxGfx_GetWindow(pDestDC);
	//_DC_SCALE_RECT(pDestDC, &cw);
	_DC_XFORM2_RECT(pDestDC, &cw);
	_GxGfx_SetClip(&cw);

	if (bEscIcon == TRUE) {
		//esc icon
		return _GxGfx_EscImage(pDestDC, Pt.x, Pt.y, id);
	} else {
		//text icon
		_GxGfx_TextPosShift(&Pt);
		return _GxGfx_TextChar(pDestDC, Pt.x, Pt.y, id);
	}
}

TextRender _gDC_Text_render = {
	_TextRender_Begin,
	_TextRender_End,
	_TextRender_ParseStringCmd,
	_TextRender_GetCharSize,
	_TextRender_DrawChar,
};

///////////////////////////////////////////////////////////////////////////////

IPOINT GxGfx_GetTextLastLoc(void)
{
	return TextFormat_GetLastLoc();
}
ISIZE GxGfx_GetTextLastSize(void)
{
	return TextFormat_GetLastSize();
}

#ifndef __LINT__
//CHAR_TYPE _gSTR_NULL[] = { _T("0") };
CHAR_TYPE _gSTR_NULL[] = { 0x0000 };
#endif

static const TCHAR *_GxGfx_DummyString(void)
{
	return (const TCHAR *)_gSTR_NULL;
}


//single-line, no ESC command
RESULT GxGfx_Text(DC *pDestDC, LVALUE x, LVALUE y, const TCHAR *pszSrc)
{
	RESULT r = GX_OK;
	IMAGE_TABLE *pFontTable;

	TextFormat TF = {0};
	IRECT DestRect;
	IPOINT DestPt;
	UINT32 uiFlag;

	DBG_FUNC_BEGIN("\r\n");
	if (pszSrc == 0) {
		DBG_WRN("GxGfx_Text - WARNING! String is null.\r\n");
		pszSrc = _GxGfx_DummyString();
	}
	if (pDestDC != 0) {
		GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	}
	pFontTable = (IMAGE_TABLE *)GxGfx_Get(BR_TEXTFONT);
	//[TODO]
	//what is default font? (return FONT_DEF)
	if (!pFontTable) {
		DBG_ERR("GxGfx_Text - ERROR! Font is null.\r\n");
		return GX_NO_FONT;
	}

#if (GX_SUPPORT_WCHAR)
	_XString_N_ConvertToWString((WCHAR *)_gDC_Text_instr, pszSrc, _gGfxStringBuf_Size);
	pszSrc = (TCHAR *)_gDC_Text_instr;
#endif

	//prepare output rect box
	POINT_Set(&DestPt, x, y);
	_GxGfx_TextPosShift(&DestPt);
	RECT_SetPoint(&DestRect, &DestPt);
	RECT_SetWH(&DestRect, 0, 0);

	TextFormat_Init(&TF, &_gDC_Text_render, (UINT32)pDestDC, &DestRect);
	uiFlag = GxGfx_Get(BR_TEXTALIGN);
	uiFlag &= ~(ALIGN_H_MASK | ALIGN_V_MASK); //clear h,v align flag
	uiFlag |= (ALIGN_LEFT | ALIGN_TOP); //always align to (left,top)
	r = TextFormat_DoSingleLineByAlign(&TF, pszSrc, uiFlag);

	TextFormat_Exit(&TF);

	//record last move
	if (pDestDC) {
		ISIZE textSize = TextFormat_GetLastSize();
		r = GxGfx_MoveTo(pDestDC, x, y);
		if (r == GX_OK) {
			r = GxGfx_MoveTo(pDestDC, x + textSize.w - 1, y + textSize.h - 1);
		}
	}

	if (r != GX_OK) {
		return r;
	}

	return GX_OK;
}

//multi-line layout to rect, support ESC command
RESULT GxGfx_TextInRect(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2, const TCHAR *pszSrc)
{
	RESULT r;
	IMAGE_TABLE *pFontTable;
	IRECT DestRect;

	TextFormat TF = {0};
	UINT32 uiFlag;
	UINT32 uiLayout;
	BOOL bINDEX;
	BOOL bPKRGB2;
	BOOL bPKRGB3;
	BOOL bPKRGB4;
	BOOL bPKRGB;

	DBG_FUNC_BEGIN("\r\n");
	if (pszSrc == 0) {
		DBG_WRN("GxGfx_TextInRect - WARNING! String is null.\r\n");
		pszSrc = _GxGfx_DummyString();
	}
	if (pDestDC != 0) {
		GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	} else {
		DBG_ERR("no pDestDC\r\n");
		return GX_NULL_POINTER;
	}

	if (x1 == x2) {
		return GX_EMPTY_RECT;
	}
	if (y1 == y2) {
		return GX_EMPTY_RECT;
	}

	pFontTable = (IMAGE_TABLE *)GxGfx_Get(BR_TEXTFONT);
	//[TODO]
	//what is default font? (return FONT_DEF)
	if (!pFontTable) {
		DBG_ERR("GxGfx_TextInRect - ERROR! Font is null.\r\n");
		return GX_NO_FONT;
	}

#if (GX_SUPPORT_WCHAR)
	_XString_N_ConvertToWString((WCHAR *)_gDC_Text_instr, pszSrc, _gGfxStringBuf_Size);
	pszSrc = (TCHAR *)_gDC_Text_instr;
#endif

	//prepare output rect box
	RECT_SetX1Y1X2Y2(&DestRect, x1, y1, x2, y2);
	_GxGfx_TextRectShrink(&DestRect);

	//check format
	bINDEX = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_IDX);
	bPKRGB2 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK2);
	bPKRGB3 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK3);
	bPKRGB4 = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK4);
	bPKRGB = ((pDestDC->uiPxlfmt & PXLFMT_PMODE) == PXLFMT_P1_PK);

	uiLayout = GxGfx_Get(BR_TEXTLAYOUT);
	uiFlag = GxGfx_Get(BR_TEXTALIGN);
	if (uiLayout & LAYOUT_LINEBREAK) { //auto line-break
		if (uiFlag & STREAMALIGN_MASK) { //stream is backward order -> Ex: Hebrew
			//reverse order before auto line-break layout
			_String_MakeReverse((TCHAR *)_gDC_Text_instr);
		}
		TextFormat_Init(&TF, &_gDC_Text_render, (UINT32)pDestDC, &DestRect);
		r = TextFormat_DoMultiLineByAlign(&TF, pszSrc, uiFlag);
		TextFormat_Exit(&TF);
	} else if (uiLayout & LAYOUT_LINEWRAP) { //no line-break, auto line-warp
		DC TempDC;
		BOOL bDrawWrap = FALSE;
		ISIZE textSize = {0};
		IRECT TempRect;

		memset(&TempDC, 0x00, sizeof(DC));

		TextFormat_Init(&TF, &_gDC_Text_render, (UINT32)0, &DestRect);
		r = TextFormat_DoSingleLine_EvalSize(&TF, pszSrc, &textSize);
		if (r != GX_OK) {
			DBG_WRN("EvalSize fail.%x\r\n", r);
		}
		TextFormat_Exit(&TF);
		RECT_Set(&TempRect, 0, 0, textSize.w, textSize.h);

		if ((textSize.w > DestRect.w) && (bINDEX || bPKRGB2 || bPKRGB || bPKRGB3 || bPKRGB4)) {
			//alloc buffer for warp effect
			r = GxGfx_PushStackDC(&TempDC,
								  TYPE_BITMAP,
								  pDestDC->uiPxlfmt,
								  textSize.w,
								  textSize.h);

			if (r != GX_OK) {
				DBG_WRN("GxGfx_TextInRect - WARNING! Failed to make warp effect. Ignore effect.\r\n");
			} else {
				bDrawWrap = TRUE;
			}
		}

		if (bDrawWrap) {
			//draw with warp effect
			IRECT cw;

			UINT32 vAlign;
			vAlign = (uiFlag & ALIGN_V_MASK);
			if (vAlign == ALIGN_BOTTOM) {
				DestRect.y += (DestRect.h - textSize.h);
			} else if (vAlign == ALIGN_MIDDLE) {
				DestRect.y += (DestRect.h - textSize.h) >> 1;
			}
			DestRect.h = textSize.h;

			cw = GxGfx_GetWindow(&TempDC);
			//_DC_SCALE_RECT(&TempDC, &cw);
			_DC_XFORM2_RECT(pDestDC, &cw);
			_GxGfx_SetClip(&cw);
			//clear temp DC
			if (uiLayout & FILTER_LINEAR) {
				if (bINDEX) {
					//fill background with back color for making correct bilinear-interpolation image
					MAPPING_ITEM c1;
					c1 = (MAPPING_ITEM)GxGfx_Get(BR_TEXTFORECOLOR2); //back color
					_GxGfx_FillRect(&TempDC, &TempRect, c1);
				}
#if (GX_SUPPORT_DCFMT_RGB565)
				if (bPKRGB2) {
					//fill background with transparent color
					_GxGfx_FillRect(&TempDC, &TempRect, 0x00000000);
				}
#endif
				if (bPKRGB) {
					DBG_ERR("need clear transparent color\r\n");
				}
			} else {
				_GxGfx_FillRect(&TempDC, &TempRect, 0);
			}

			//draw to temp DC
			TextFormat_Init(&TF, &_gDC_Text_render, (UINT32)&TempDC, &TempRect);
			uiFlag &= ~(ALIGN_H_MASK | ALIGN_V_MASK); //clear h,v align flag
			uiFlag |= (ALIGN_LEFT | ALIGN_TOP); //always align to (left,top)
			r = TextFormat_DoSingleLineByAlign(&TF, pszSrc, uiFlag);
			TextFormat_Exit(&TF);

			cw = GxGfx_GetWindow(pDestDC);
			//_DC_SCALE_RECT(pDestDC, &cw);
			_DC_XFORM2_RECT(pDestDC, &cw);
			_GxGfx_SetClip(&cw);
			if (uiLayout & FILTER_LINEAR) {
				if (bINDEX) {
					_GxGfx_SetImageROP(ROP_KEY | FILTER_LINEAR, GxGfx_Get(BR_TEXTFORECOLOR2), 0, 0);
					//DestRect.w <<= 1;
				}
#if (GX_SUPPORT_DCFMT_RGB565)
				if (bPKRGB2) {
					//color key with transparent color
					_GxGfx_SetImageROP(ROP_KEY | FILTER_LINEAR, 0x00000000, 0, 0);
				}
#endif
				if (bPKRGB) {
					DBG_ERR("need to set color key with transparent color\r\n");
				}
			} else {
				_GxGfx_SetImageROP(ROP_KEY, 0, 0, 0);
			}
			//scale temp DC to dest DC, with src-key OP, colorkey =0
			_GxGfx_StretchBlt(pDestDC, &DestRect, &TempDC, 0);

			//free buffer for warp effect
			GxGfx_PopStackDC(&TempDC);

			GxGfx_MoveTo(pDestDC, DestRect.x, DestRect.y);
			GxGfx_MoveTo(pDestDC, DestRect.x + DestRect.w - 1, DestRect.y + DestRect.h - 1);
			return r;
		} else {
			//draw without warp effect

			TextFormat_Init(&TF, &_gDC_Text_render, (UINT32)pDestDC, &DestRect);
			r = TextFormat_DoSingleLineByAlign(&TF, pszSrc, uiFlag);
			TextFormat_Exit(&TF);
		}
	} else { //no line-break and no line-warp
		TextFormat_Init(&TF, &_gDC_Text_render, (UINT32)pDestDC, &DestRect);
		r = TextFormat_DoSingleLineByAlign(&TF, pszSrc, uiFlag);
		TextFormat_Exit(&TF);
	}

	//record last move

	IPOINT textLoc = TextFormat_GetLastLoc();
	ISIZE textSize = TextFormat_GetLastSize();
	GxGfx_MoveTo(pDestDC, textLoc.x, textLoc.y);
	GxGfx_MoveTo(pDestDC, textLoc.x + textSize.w - 1, textLoc.y + textSize.h - 1);

	return r;
}

static INT16 _String_DecodeCtrlCommand(const TCHAR *pszSrc, UINT16 *pCtrl)
{
	INT16 iCount;

	iCount = 0;

	if ((!_String_N_Compare(pszSrc, FMT(PIMAGEID), LEN(PIMAGEID)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(PIMAGEID);
	} else if ((!_String_N_Compare(pszSrc, FMT(IMAGEID), LEN(IMAGEID)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(IMAGEID);
	} else if ((!_String_N_Compare(pszSrc, FMT(IMAGEMAPPING), LEN(IMAGEMAPPING)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(IMAGEMAPPING);
	} else if ((!_String_N_Compare(pszSrc, FMT(IMAGEKEYCOLOR), LEN(IMAGEKEYCOLOR)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(IMAGEKEYCOLOR);
	} else if ((!_String_N_Compare(pszSrc, FMT(IMAGETABLE), LEN(IMAGETABLE)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(IMAGETABLE);
	} else if ((!_String_N_Compare(pszSrc, FMT(TEXTFONT), LEN(TEXTFONT)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(TEXTFONT);
	} else if ((!_String_N_Compare(pszSrc, FMT(TEXTFONTSTYLE), LEN(TEXTFONTSTYLE)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(TEXTFONTSTYLE);
	} else if ((!_String_N_Compare(pszSrc, FMT(TEXTFONTSIZE), LEN(TEXTFONTSIZE)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(TEXTFONTSIZE);
	} else if ((!_String_N_Compare(pszSrc, FMT(TEXTFORECOLOR1), LEN(TEXTFORECOLOR1)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(TEXTFORECOLOR1);
	} else if ((!_String_N_Compare(pszSrc, FMT(TEXTFORECOLOR2), LEN(TEXTFORECOLOR2)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(TEXTFORECOLOR2);
	} else if ((!_String_N_Compare(pszSrc, FMT(TEXTFORECOLOR3), LEN(TEXTFORECOLOR3)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(TEXTFORECOLOR3);
	} else if ((!_String_N_Compare(pszSrc, FMT(TEXTLAYOUT), LEN(TEXTLAYOUT)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(TEXTLAYOUT);
	} else if ((!_String_N_Compare(pszSrc, FMT(TEXTALIGN), LEN(TEXTALIGN)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(TEXTALIGN);
	} else if ((!_String_N_Compare(pszSrc, FMT(TEXTLINEHEIGHT), LEN(TEXTLINEHEIGHT)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(TEXTLINEHEIGHT);
	} else if ((!_String_N_Compare(pszSrc, FMT(TEXTLETTERSPACE), LEN(TEXTLETTERSPACE)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(TEXTLETTERSPACE);
	} else if ((!_String_N_Compare(pszSrc, FMT(TEXTINDENTSPACE), LEN(TEXTINDENTSPACE)))) {
		//it is a %[ctrl]
		*pCtrl |= CTRL_VALUE_CMD;
		iCount += LEN(TEXTINDENTSPACE);
	}
	/*    else if( (!_String_N_Compare(pszSrc, FMT(TEXTSTRINGTABLE), LEN(TEXTSTRINGTABLE))) )
	    {
	        //it is a %[ctrl]
	        *pCtrl |= CTRL_VALUE_CMD;
	        iCount += LEN(TEXTSTRINGTABLE);
	    }*/

	return iCount;
}

static INT16 _String_DecodeCtrlFormat(const TCHAR *pszSrc, UINT16 *pCtrl)
{
	int fmt_stage;
	INT16 iCount;
	//const TCHAR* pszToken;
	const TCHAR *ts1;
	const TCHAR *ts2;
	TCHAR_VALUE c;

	iCount = 0;
	//pszToken = pszSrc;

	//0: end, 1: check modifier, 2: check format
	fmt_stage = 1; //start to check modifier
	ts1 = NULL;
	ts2 = NULL;

	c = _String_GetChar(pszSrc, iCount);

	while (c && fmt_stage) {
		if (fmt_stage == 1) { //1: check modifier
			if ((iCount == 0) && (c == 'p')) {
				//NOTE: we will support "pointer to value" for following types:
				//c,d,i,o,u,x,X,f : value format ( like variable format symbol of printf() )
				//s: string format ( like string format symbol of printf() )
				//sid : string id
				//iid : icon id
				*pCtrl |= CTRL_VALUE_PTR;
			}
			ts1 = _String_FindChar(FMT(STAGE1), c);
			if (!ts1) {
				fmt_stage = 2; //start to check format
			}
		}
		if (fmt_stage == 2) { //2: check format
			ts2 = _String_FindChar(FMT(STAGE2), c);
			if (!ts2) {
				if (c == 't') {
					if (!_String_N_Compare(pszSrc, FMT(TEXT_CMD), LEN(TEXT_CMD))) {
						if (!_String_N_Compare(pszSrc, FMT(TEXTSTRINGTABLE), LEN(TEXTSTRINGTABLE))) {
							*pCtrl |= CTRL_VALUE_FMT;
							iCount += LEN(TEXTSTRINGTABLE);
							return iCount;
						} else if (!_String_N_Compare(pszSrc, FMT(PTEXTID), LEN(PTEXTID))) {
							*pCtrl |= CTRL_VALUE_FMT;
							*pCtrl |= CTRL_VALUE_PTR;
							iCount += LEN(PTEXTID);
							return iCount;
						} else if (!_String_N_Compare(pszSrc, FMT(TEXTID), LEN(TEXTID))) {
							*pCtrl |= CTRL_VALUE_FMT;
							iCount += LEN(TEXTID);
							return iCount;
						}
					}
				}

				fmt_stage = 0; //invalid, end
			} else {
				BOOL bIgnore = FALSE;
				if (c == 'i') {
					if (!_String_N_Compare(pszSrc, FMT(IMAGE_CMD), LEN(IMAGE_CMD))) {
						//ignore, this is an "image:" command
						bIgnore = TRUE;
					}
				}

				if (!bIgnore) {
					*pCtrl |= CTRL_VALUE_FMT;
					iCount++;
				}

				return iCount;
			}
		}

		iCount++;
		c = _String_GetChar(pszSrc, iCount);

	}

	//invalid
	iCount = 0;

	return iCount;
}

static INT16 _String_DecodeCtrlFormatClass(const TCHAR *pszSrc, INT16 iLen, UINT16 *pCtrl)
{
	const TCHAR *pHeadChar;
	TCHAR_VALUE tcEscChar;
	TCHAR_VALUE tcEndChar;
	INT16 iLen2;

	pHeadChar = _String_GetConstCharPtr(pszSrc, 1); //pointer to first char


	if (!_String_N_Compare(pHeadChar, FMT(PTEXTID), LEN(PTEXTID))) {
		//psid
		*pCtrl |= CTRL_VALUE_TEXTID;
	} else if (!_String_N_Compare(pHeadChar, FMT(TEXTID), LEN(TEXTID))) {
		//sid
		*pCtrl |= CTRL_VALUE_TEXTID;
	} else if (!_String_N_Compare(pHeadChar, FMT(TEXTSTRINGTABLE), LEN(TEXTSTRINGTABLE))) {
		//sdb
		*pCtrl |= CTRL_VALUE_TEXTDB;
	} else { //printf() format
		tcEscChar = _String_GetChar(pszSrc, 0); //Esc char: '%' or '^'
		iLen2 = iLen - 1;
		tcEndChar = _String_GetChar(pszSrc, iLen2); //last char

		//check if it is %[fmt], just convert [fmt] format value to string.
		switch (tcEndChar) {
		case '%':
			if ((iLen == 2) && (tcEscChar == '%'))
				//%%
			{
				*pCtrl |= CTRL_VALUE_CPAR;
			}
			break;
		case '^':
			if ((iLen == 2) && (tcEscChar == '^'))
				//^^
			{
				*pCtrl |= CTRL_VALUE_CESC;
			}
			break;
		case 'c':
		case 'd':
		case 'i':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
			//c,d,i,o,u,x,X : value format ( like variable format symbol of printf() )
			iLen2 = iLen - 2;
			tcEndChar = _String_GetChar(pszSrc, iLen2); //2nd last char
			if (tcEndChar == 'l') {
				*pCtrl |= CTRL_VALUE_LONG;
			}
			//%plx or %lx
			else {
				*pCtrl |= CTRL_VALUE_INT;
			}
			//%px or %x
			break;
		case 'f':
			//f : value format ( like variable format symbol of printf() )
			iLen2 = iLen - 3;
			tcEndChar = _String_GetChar(pszSrc, iLen2); //2nd last char
			if (tcEndChar == 'l') {
				*pCtrl |= CTRL_VALUE_DOUBLE;
			}
			//%plf or %lf
			else {
				*pCtrl |= CTRL_VALUE_FLOAT;
			}
			//%pf or %f
			break;
		case 's':
			//s: string format ( like string format symbol of printf() )
			if ((iLen == 2) || ((iLen == 3) && ((*pCtrl) & CTRL_VALUE_PTR)))
				//%ps or %s
			{
				*pCtrl |= CTRL_VALUE_TEXT;
			}
			break;
		}
	}
	return 0;
}

#if (GX_SUPPORT_WCHAR)
TCHAR _gDC_Text_decode_fmt[PRINTF_BUFFER_SIZE];
TCHAR _gDC_Text_decode_dest[PRINTF_BUFFER_SIZE];
#endif

static INT16 _String_DecodeFormatWithParamValue
(DC *pDestDC, TCHAR *pszDest, TCHAR *pszFmt, UINT16 uiFlag, va_list marker)
{
	//=================================================================
	// formatting value by class of [fmt]
	//=================================================================
	long arglong;
	int  argint;
	float argfloat;
	double argdouble;
	IVALUE TextID;
	const STRING_TABLE *pStringTable;
	const TCHAR *pText;
	TCHAR *pDestBuf;
	const TCHAR *pSrcBuf;
	const TCHAR *pszSrc;

	if (uiFlag & CTRL_VALUE_PTR) {
		_String_SetChar(pszFmt, 1, '%');//replace 'p' by '%'
		pSrcBuf = _String_GetConstCharPtr(pszFmt, 1);
	} else {
		pSrcBuf = pszFmt;
	}
	pDestBuf = pszDest;

	_String_Resize(pDestBuf, 0);

	pszSrc = 0;
#ifndef __LINT__
//TODO: remove unknown pclint error
	switch (uiFlag) {
	case CTRL_VALUE_TEXTID :
		pStringTable = (const STRING_TABLE *)GxGfx_Get(BR_TEXTSTRINGTABLE);
		TextID = (IVALUE)va_arg(marker, long);
		pszSrc = (pStringTable == 0) ? "(0)" : STRING_LoadFromTable(pStringTable, TextID);
		break;
	case CTRL_VALUE_TEXTID | CTRL_VALUE_PTR :
		pStringTable = (const STRING_TABLE *)GxGfx_Get(BR_TEXTSTRINGTABLE);
		TextID = (IVALUE) * va_arg(marker, long *);
		pszSrc = (pStringTable == 0) ? "(0)" : STRING_LoadFromTable(pStringTable, TextID);
		break;
	case CTRL_VALUE_TEXT :
		pText = (const TCHAR *) va_arg(marker, long);
		pszSrc = (pText == 0) ? "(0)" : pText;
		break;
	case CTRL_VALUE_TEXT | CTRL_VALUE_PTR :
		pText = (const TCHAR *)(* va_arg(marker, long *));
		pszSrc = (pText == 0) ? "(0)" : pText;
		break;
	}
#endif

	if (pszSrc)
#if (GX_SUPPORT_WCHAR)
	{
		_XString_N_ConvertToWString((WCHAR *)pDestBuf, pszSrc, _gGfxStringBuf_Size);
		return 1;
	}
#else
	{
		sprintf(pDestBuf, "%s", pszSrc);
		return 1;
	}
#endif

#if (GX_SUPPORT_WCHAR)
	_WString_N_ConvertToTString(_gDC_Text_decode_fmt, (WCHAR *)pSrcBuf, PRINTF_BUFFER_SIZE);
	pSrcBuf = (TCHAR *)_gDC_Text_decode_fmt;
	pDestBuf = (TCHAR *)_gDC_Text_decode_dest;
#endif

	_String_Resize(pDestBuf, 0);

#ifndef __LINT__
//TODO: remove unknown pclint error
	switch (uiFlag) {
	case CTRL_VALUE_CPAR:
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, "%c", '%');
		break;
	case CTRL_VALUE_LONG :
		arglong = va_arg(marker, long);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, arglong);
		break;
	case CTRL_VALUE_LONG | CTRL_VALUE_PTR :
		arglong = * va_arg(marker, long *);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, arglong);
		break;
	case CTRL_VALUE_INT :
		argint = va_arg(marker, int);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, argint);
		break;
	case CTRL_VALUE_INT | CTRL_VALUE_PTR :
		argint = * va_arg(marker, int *);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, argint);
		break;
	case CTRL_VALUE_DOUBLE :
		argdouble = va_arg(marker, double);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, argdouble);
		break;
	case CTRL_VALUE_DOUBLE | CTRL_VALUE_PTR :
		argdouble = * va_arg(marker, double *);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, argdouble);
		break;
	case CTRL_VALUE_FLOAT :
		argfloat = (float) va_arg(marker, double);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, argfloat);
		break;
	case CTRL_VALUE_FLOAT | CTRL_VALUE_PTR :
		argfloat = * va_arg(marker, float *);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, argfloat);
		break;
	case CTRL_VALUE_TEXTDB :
		pStringTable = (STRING_TABLE *) va_arg(marker, long);
		//kkk
		GxGfx_Set(BR_TEXTSTRINGTABLE, pStringTable);

		break;
	default :
		return 0;
	}
#endif

#if (GX_SUPPORT_WCHAR)
	_TString_N_ConvertToWString((WCHAR *)pszDest, _gDC_Text_decode_dest, PRINTF_BUFFER_SIZE);
#endif

	return 1;
}

static INT16 _String_DecodeFormatWithEscValue
(DC *pDestDC, TCHAR *pszDest, TCHAR *pszFmt, UINT16 uiFlag, INT16 iState)
{
	//=================================================================
	// formatting value by class of [fmt]
	//=================================================================
	long arglong;
	int  argint;
	float argfloat;
	double argdouble;
	IVALUE TextID;
	const STRING_TABLE *pStringTable;
	const TCHAR *pText;
	UINT32 u32;
	const TCHAR *pszSrc;

	TCHAR *pDestBuf;
	const TCHAR *pSrcBuf;

	_String_SetChar(pszFmt, 0, '%');   //replace '^' by '%'
	if (uiFlag & CTRL_VALUE_PTR) {
		_String_SetChar(pszFmt, 1, '%');//replace 'p' by '%'
		pSrcBuf = _String_GetConstCharPtr(pszFmt, 1);
	} else {
		pSrcBuf = pszFmt;
	}
	pDestBuf = pszDest;

	VAR_SETTYPE(VARTYPE_STATE);

	_String_Resize(pDestBuf, 0);

	pszSrc = 0;
	switch (uiFlag) {
	case CTRL_VALUE_TEXTID :
		//kkk
		pStringTable = (const STRING_TABLE *)GxGfx_Get(BR_TEXTSTRINGTABLE);
		TextID = (IVALUE)VAR_GETVALUE(iState);
		pszSrc = (pStringTable == 0) ? "(0)" : STRING_LoadFromTable(pStringTable, TextID);
		break;
	case CTRL_VALUE_TEXTID | CTRL_VALUE_PTR :
		//kkk
		pStringTable = (const STRING_TABLE *)GxGfx_Get(BR_TEXTSTRINGTABLE);
		TextID = *(IVALUE *)VAR_GETVALUE(iState);
		pszSrc = (pStringTable == 0) ? "(0)" : STRING_LoadFromTable(pStringTable, TextID);
		break;
	case CTRL_VALUE_TEXT :
		pText = (TCHAR *)VAR_GETVALUE(iState);
		pszSrc = (pText == 0) ? "(0)" : pText;
		break;
	case CTRL_VALUE_TEXT | CTRL_VALUE_PTR :
		pText = *(const TCHAR **)VAR_GETVALUE(iState);
		pszSrc = (pText == 0) ? "(0)" : pText;
		break;
	}

	if (pszSrc)
#if (GX_SUPPORT_WCHAR)
	{
		_XString_N_ConvertToWString((WCHAR *)pDestBuf, pszSrc, _gGfxStringBuf_Size);
		return 1;
	}
#else
	{
		sprintf(pDestBuf, "%s", pszSrc);
		return 1;
	}
#endif

#if (GX_SUPPORT_WCHAR)
	_WString_N_ConvertToTString(_gDC_Text_decode_fmt, (WCHAR *)pSrcBuf, PRINTF_BUFFER_SIZE);
	pSrcBuf = (TCHAR *)_gDC_Text_decode_fmt;
	pDestBuf = (TCHAR *)_gDC_Text_decode_dest;
#endif

	_String_Resize(pDestBuf, 0);

	switch (uiFlag) {
	case CTRL_VALUE_CESC :
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, "%c", '^');
		break;
	case CTRL_VALUE_LONG :
		arglong = (long)VAR_GETVALUE(iState);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, arglong);
		break;
	case CTRL_VALUE_LONG | CTRL_VALUE_PTR :
		arglong = *(long *)VAR_GETVALUE(iState);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, arglong);
		break;
	case CTRL_VALUE_INT :
		argint = (int)VAR_GETVALUE(iState);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, argint);
		break;
	case CTRL_VALUE_INT | CTRL_VALUE_PTR :
		argint = *(int *)VAR_GETVALUE(iState);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, argint);
		break;
	case CTRL_VALUE_DOUBLE :
		argdouble = *(double *)VAR_GETVALUE(iState);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, argdouble);
		break;
	case CTRL_VALUE_DOUBLE | CTRL_VALUE_PTR :
		argdouble = *(double *)VAR_GETVALUE(iState);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, argdouble);
		break;
	case CTRL_VALUE_FLOAT :
		u32 = (UINT32)VAR_GETVALUE(iState);
		argfloat = *(float *)&u32;
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, argfloat);
		break;
	case CTRL_VALUE_FLOAT | CTRL_VALUE_PTR :
		argfloat = *(float *)VAR_GETVALUE(iState);
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, pSrcBuf, argfloat);
		break;
	case CTRL_VALUE_TEXTDB :
		pStringTable = (STRING_TABLE *)VAR_GETVALUE(iState);
		//kkk
		GxGfx_Set(BR_TEXTSTRINGTABLE, pStringTable);

		break;
	default :
		return 0;
	}

#if (GX_SUPPORT_WCHAR)
	_TString_N_ConvertToWString((WCHAR *)pszDest, _gDC_Text_decode_dest, PRINTF_BUFFER_SIZE);
#endif

	return 1;
}

static INT16 _String_EncodeCommandWithNumber
(TCHAR *pszDest, TCHAR *pszCmd, INT16 iNum)
{
	INT16 iCount, iCmdLen;
	TCHAR *pDestBuf;

	iCount = 0;
	iCmdLen = _String_GetLength(pszCmd);

//#if (GX_SUPPORT_WCHAR)
	_String_N_Copy(pszDest, pszCmd, iCmdLen);
	/*#else
	    sprintf(pszDest, "%s", pszCmd);
	#endif*/

	iCount += iCmdLen;
	pszDest = _String_GetCharPtr(pszDest, iCount);

#if (GX_SUPPORT_WCHAR)
	pDestBuf = (TCHAR *)_gDC_Text_decode_dest;
#else
	pDestBuf = pszDest;
#endif

	if ((iNum != -1) && (iNum < VARARRAY_SIZE)) {
		//convert to stack symbol
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, "%d", iNum);
	} else {
		//out of stack size
		snprintf(pDestBuf, PRINTF_BUFFER_SIZE, "(0)");
	}

#if (GX_SUPPORT_WCHAR)
	_TString_N_ConvertToWString((WCHAR *)pszDest, _gDC_Text_decode_dest, PRINTF_BUFFER_SIZE);
#endif

	return iCount;
}


static RESULT _GxGfx_vsprintf(DC *pDestDC, TCHAR *pszDest, const TCHAR *pszSrc, va_list marker)
{
	const TCHAR *pCurSrc;
	TCHAR *pCurBuf;
	TCHAR *pszBuf;
	TCHAR *pszOut;
	INT16 iSrc;   //read offset of pszSrc
	INT16 iBuf;   //write offset of pszBuf
	INT16 iOut;   //write offset of pszOut
	INT16 iDest;  //write offset of pszDest

	INT16 iCount;

	TCHAR_VALUE c;
	int fmt_type; //0:invalid, 1:'%', 2:'^'
	BOOL bCtrlFmt;
	BOOL bCtrlCmd;
	UINT16 uiCtrlFlag;
	INT16 iStack; // '$' stack index
	INT16 iState; // '^' state index

	//Save state (keep initial state)
	//kkk
	GxGfx_GetAll(&_gBRUSH_old);
//    _gBRUSH_text = _gBRUSH_old;

	iStack = 0;

	pszOut = (TCHAR *)_gDC_Text_prnstr;

	pszBuf = (TCHAR *)_gDC_Text_bufstr;

#if (GX_SUPPORT_WCHAR)
	AddWStringHeader((WCHAR *)pszDest);
	pszDest = (TCHAR *)SeekWString((WCHAR *)pszDest, FMT_WCHAR_SIZE);
#endif
	iDest = 0;
	_String_Resize(pszDest, 0);

#if (GX_SUPPORT_WCHAR)
	_XString_N_ConvertToWString((WCHAR *)_gDC_Text_instr, pszSrc, _gGfxStringBuf_Size);
	pszSrc = (TCHAR *)_gDC_Text_instr;
#endif
	iSrc = 0;
	pCurSrc = _String_GetConstCharPtr(pszSrc, iSrc);
	c = _String_GetChar(pCurSrc, 0);

	while (c) {
		fmt_type = 0;

		//=================================================================
		// find prefix is '%' or '^' from src string
		//=================================================================
		if ((c != '%') &&
			(c != '^')) {
			//normal character:

			//copy it to output string
			_String_SetChar(pszDest, iDest, c);
			iSrc++;
			pCurSrc = _String_GetConstCharPtr(pszSrc, iSrc);
			iDest++;
		} else { //c == '%' or '^'
			//prefix character:

			UINT16 uiFlag = 0;

			bCtrlCmd = 1;
			bCtrlFmt = 0;
			iState = 0;

			iBuf = 0;
			_String_Resize(pszBuf, 0);

			iOut = 0;
			_String_Resize(pszOut, 0);

			//=================================================================
			// decode prefix char
			//=================================================================
			if (c == '%') {
				fmt_type = 1;    //'%'
			} else {
				fmt_type = 2;    //'^'
			}

			//copy to buf string
			_String_N_Copy(pszBuf, pCurSrc, 1);
			iSrc++;
			pCurSrc = _String_GetConstCharPtr(pszSrc, iSrc);
			iBuf++;

			if (bCtrlCmd) {
				//=================================================================
				// decode %[cmd] and ^[cmd]
				//=================================================================
				uiCtrlFlag = 0;
				iCount = _String_DecodeCtrlCommand(pCurSrc, &uiCtrlFlag);
				if (uiCtrlFlag & CTRL_VALUE_CMD) {
					//copy to buf string
					pCurBuf = _String_GetCharPtr(pszBuf, iBuf);
					_String_N_Copy(pCurBuf, pCurSrc, iCount);
					iSrc += iCount;
					pCurSrc = _String_GetConstCharPtr(pszSrc, iSrc);
					iBuf += iCount;
				} else {
					bCtrlCmd = 0;
					//it is not a valid %[ctrl] or ^[ctrl][n]
				}
			}

			if ((fmt_type == 1) && bCtrlCmd) {
				/*if (marker == 0) { //if va_list is not exist
					bCtrlCmd = 0;    //ignore %[cmd]
				}*/
			}

			if ((fmt_type == 1) && bCtrlCmd) { //'%'
				//=================================================================
				// encode %[cmd] to $[cmd][n],  n=iStack
				//=================================================================
				UINT32 value;

				_String_SetChar(pszBuf, 0, '$');

				value = va_arg(marker, UINT32);
				/*
				                VAR_SETTYPE(VARTYPE_STACK);

				                if(iStack < VARARRAY_SIZE)
				                {
				                    VAR_SETVALUE(iStack, value);
				                    //convert to stack symbol
				                    sprintf(pszOut, "%s%d", pszBuf, iStack);
				                    iStack ++;
				                }
				                else
				                {
				                    //out of stack size
				                    sprintf(pszOut, "%s(0)", pszBuf);
				                }*/
				_String_EncodeCommandWithNumber(pszOut, pszBuf, iStack);

				VAR_SETTYPE(VARTYPE_STACK);
				if ((iStack >= 0) && (iStack < VARARRAY_SIZE)) {
					VAR_SETVALUE(iStack, value);
					iStack ++;
				} else {
					//out of stack size
				}
			}

			if ((fmt_type == 2) && bCtrlCmd) { //'^'
				//=================================================================
				// decode [n] of ^[cmd],  n=iState
				//=================================================================
				iState = -1;
				iCount = _String_DecodeNumber(pCurSrc, &iState);
				//=================================================================
				// encode ^[cmd][n] to ^[cmd][n] (keep)
				//=================================================================
				/*                if((iState != -1) && (iState < VARARRAY_SIZE))
				                {
				                    iSrc += iCount; pCurSrc = _String_GetConstCharPtr(pszSrc, iSrc);
				                    sprintf(pszOut, "%s%d", pszBuf, iState);
				                }
				                else
				                {
				                    //it is not a valid [n]
				                    sprintf(pszOut, "%s(0)", pszBuf);
				                }*/
				_String_EncodeCommandWithNumber(pszOut, pszBuf, iState);

				if ((iState >= 0) && (iState < VARARRAY_SIZE)) {
					iSrc += iCount;
					pCurSrc = _String_GetConstCharPtr(pszSrc, iSrc);
				} else {
					//it is not a valid [n]
				}
			}

			bCtrlFmt = 0;

			if (!bCtrlCmd) {
				//=================================================================
				// give-up to decode [cmd]
				//=================================================================
                //coverity[DEADCODE]:not real dead code,because line 1711
				#if 0
				if (iBuf > 1) {
					//restore all chars to buffer, except '%'
					iSrc -= (iBuf - 1);
					pCurSrc = _String_GetConstCharPtr(pszSrc, iSrc);
					iBuf = 1;
					_String_Resize(pszBuf, iBuf);
					_String_Resize(pszOut, 0);
				}
                #endif
				//=================================================================
				// start to decode [fmt]
				//=================================================================
				bCtrlFmt = 1;
			}

			if (bCtrlFmt) {
				//=================================================================
				// decode [fmt]
				//=================================================================

				iCount = _String_DecodeCtrlFormat(pCurSrc, &uiCtrlFlag);
				if (uiCtrlFlag & CTRL_VALUE_FMT) {
					//copy to buf string
					pCurBuf = _String_GetCharPtr(pszBuf, iBuf);
					_String_N_Copy(pCurBuf, pCurSrc, iCount);
					iSrc += iCount;
					pCurSrc = _String_GetConstCharPtr(pszSrc, iSrc);
					iBuf += iCount;
					uiFlag |= (uiCtrlFlag & ~CTRL_VALUE_FMT);
				} else {
					bCtrlFmt = 0;    //it is not a %[ctrl]
				}

			}

			if (bCtrlFmt) {
				//=================================================================
				// classify [fmt]
				//=================================================================

				_String_Resize(pszOut, 0);

				_String_DecodeCtrlFormatClass(pszBuf, iBuf, &uiFlag);
			}

			if ((fmt_type == 2) && bCtrlFmt) {
				//=================================================================
				// decode [iState] of ^
				//=================================================================
				iState = -1;
				iCount = _String_DecodeNumber(pCurSrc, &iState);
				if ((iState != -1) && (iState < VARARRAY_SIZE)) {
					iSrc += iCount;
					pCurSrc = _String_GetConstCharPtr(pszSrc, iSrc);
					//skip to output this "number"
				} else {
					bCtrlFmt = 0;    //it is not a valid [n]
				}
			}

			if ((fmt_type == 1) && bCtrlFmt) {
				/*if (marker == 0) { //if va_list is not exist, ignore %[fmt]
					bCtrlFmt = 0;
				}*/
			}

			if ((fmt_type == 1) && bCtrlFmt) {
				//=================================================================
				// formatting value to string by class of %[fmt]
				//=================================================================
				if (!_String_DecodeFormatWithParamValue
					(pDestDC, pszOut, pszBuf, uiFlag, marker)) {
					bCtrlFmt = 0;    //it is not a valid %[fmt]
				}
			}

			if ((fmt_type == 2) && bCtrlFmt) {
				//=================================================================
				// formatting value to string by class of ^[fmt]
				//=================================================================
				if (!_String_DecodeFormatWithEscValue
					(pDestDC, pszOut, pszBuf, uiFlag, iState)) {
					bCtrlFmt = 0;    //it is not a valid ^[fmt]
				}
			}

			if (!bCtrlFmt && !bCtrlCmd) {
				//=================================================================
				// give-up to find %[cmd]/^[cmd] and %[fmt]/^[fmt]
				//=================================================================
				if (iBuf > 1) {
					//restore all chars to buffer, except '%'
					iSrc -= (iBuf - 1);
					pCurSrc = _String_GetConstCharPtr(pszSrc, iSrc);
					iBuf = 1;
					_String_Resize(pszBuf, iBuf);
					_String_Resize(pszOut, 0);
				}
			}

			//=================================================================
			// append formatted string to dest string
			//=================================================================
			iOut = _String_GetLength(pszOut);
			pCurBuf = _String_GetCharPtr(pszDest, iDest);
			_String_N_Copy(pCurBuf, pszOut, iOut);
			iDest += iOut;
		}

		c = _String_GetChar(pCurSrc, 0);
	}

	//Load state (restore to initial state)
	//kkk
	GxGfx_SetAll(&_gBRUSH_old);

	return GX_OK;
}
#if 0 //not use now
static RESULT _GxGfx_sprintf(DC *pDestDC, TCHAR *pszDest, const TCHAR *pszSrc, ...)
{
	va_list marker;

	va_start(marker, pszSrc);       // Initialize variable arguments.

	_GxGfx_vsprintf(pDestDC, pszDest, pszSrc, marker);

	va_end(marker);                // Reset variable arguments.

	return GX_OK;
}
#endif




//DC_Text = _DC_sprintf(support %[fmt] only) + _DC_Text()



//single-line, no ESC command
RESULT GxGfx_TextPrint(DC *pDestDC, LVALUE x, LVALUE y, const TCHAR *pszSrc, ...)
{
	va_list marker;
	TCHAR *pszDest;

	IMAGE_TABLE *pFontTable;

	DBG_FUNC_BEGIN("\r\n");
	if (pszSrc == 0) {
		DBG_WRN("GxGfx_TextPrint - WARNING! String is null.\r\n");
		pszSrc = _GxGfx_DummyString();
	}
	if (pDestDC != 0) {
		GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	}
	pFontTable = (IMAGE_TABLE *)GxGfx_Get(BR_TEXTFONT);
	//[TODO]
	//what is default font? (return FONT_DEF)
	if (!pFontTable) {
		DBG_ERR("GxGfx_TextPrint - ERROR! Font is null.\r\n");
		return GX_NO_FONT;
	}

	pszDest = (TCHAR *)_gDC_Text_outstr;
	_String_N_Fill(pszDest, 0x00, _gGfxStringBuf_Size);

	va_start(marker, pszSrc);       // Initialize variable arguments.

	_GxGfx_vsprintf(pDestDC, pszDest, pszSrc, marker);

	va_end(marker);                // Reset variable arguments.

	return GxGfx_Text(pDestDC, x, y, pszDest);
}

//DC_StyleText = _DC_sprintf() + _DC_TextRect()

//multi-line layout to rect, support ESC command
RESULT GxGfx_TextPrintInRect(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2, const TCHAR *pszSrc, ...)
{
	va_list marker;
	TCHAR *pszDest;
	IMAGE_TABLE *pFontTable;

	DBG_FUNC_BEGIN("\r\n");
	if (pszSrc == 0) {
		DBG_WRN("GxGfx_TextPrintInRect - WARNING! String is null.\r\n");
		pszSrc = _GxGfx_DummyString();
	}
	if (pDestDC != 0) {
		GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	}
	if (x1 == x2) {
		return GX_EMPTY_RECT;
	}
	if (y1 == y2) {
		return GX_EMPTY_RECT;
	}

	pFontTable = (IMAGE_TABLE *)GxGfx_Get(BR_TEXTFONT);
	//[TODO]
	//what is default font? (return FONT_DEF)
	if (!pFontTable) {
		DBG_ERR("GxGfx_TextPrintInRect - ERROR! Font is null.\r\n");
		return GX_NO_FONT;
	}

	pszDest = (TCHAR *)_gDC_Text_outstr;
	_String_N_Fill(pszDest, 0x00, _gGfxStringBuf_Size);

	va_start(marker, pszSrc);       // Initialize variable arguments.

	_GxGfx_vsprintf(pDestDC, pszDest, pszSrc, marker);

	va_end(marker);                // Reset variable arguments.

	return GxGfx_TextInRect(pDestDC, x1, y1, x2, y2, pszDest);
}

//get text content
//if pStringTable == 0, cannot get string by id, return NULL string
const TCHAR *GxGfx_GetStringID(IVALUE id)
{
	const STRING_TABLE *pTable = (const STRING_TABLE *)GxGfx_Get(BR_TEXTSTRINGTABLE);
	return STRING_LoadFromTable(pTable, id);
}

