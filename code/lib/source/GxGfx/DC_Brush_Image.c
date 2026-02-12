
#include "GxGfx/GxGfx.h"
#include "DC.h"
#include "GxDCDraw.h"
#include "GxGfx_int.h"
//#include "GxImageJPG.h"

//====================
//  API
//====================
RESULT IMAGE_LoadFromTable(IMAGE *pImage, const IMAGE_TABLE *pTable, IVALUE id);
RESULT IMAGE_GetSizeFromTable
(const IMAGE_TABLE *pTable, IVALUE id, ISIZE *pImageSize);
ISIZE IMAGE_GetSize(const IMAGE *pImage);
RESULT IMAGE_GetActiveRect(const IMAGE *pImage, IRECT *pRect);
RESULT IMAGE_SetActiveRect(IMAGE *pImage, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2);
//dc draw image
RESULT GxGfx_Image(DC *pDestDC, LVALUE x, LVALUE y, const IMAGE *pImage);
RESULT GxGfx_ImageScale(DC *pDestDC, LVALUE x, LVALUE y, LVALUE w, LVALUE h, const IMAGE *pImage);         //scaling to rect
//dc draw image in rect
RESULT GxGfx_ImageInRect(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2, const IMAGE *pImage);        //scaling to rect
RESULT GxGfx_ImageScaleInRect(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2, LVALUE w, LVALUE h, const IMAGE *pImage);
//get icon content
const IMAGE *GxGfx_GetImageID(IVALUE id);//if pTable == 0, cannot get image by id, return NULL image
const IMAGE *GxGfx_GetImageIDEx(IVALUE id, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2);

RESULT _GxGfx_DummyImage(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2);


RESULT IMAGE_LoadFromTable(IMAGE *pImage, const IMAGE_TABLE *pTable, IVALUE id)
{
	RESULT r = GX_OK;
	DC *pSrcDC;
	ISIZE imageSize;

	GXGFX_ASSERT(pImage);
	GXGFX_ASSERT(pTable);

	if (pImage == 0) {
		return GX_NULL_POINTER;
	}

	((DISPLAY_CONTEXT *) & (pImage->dc))->uiType = TYPE_NULL;
	RECT_Set(&(pImage->rect), 0, 0, 0, 0);

	pSrcDC = &(pImage->dc);
	/*if(_ICON_GetBpp(pTable, id) == 24)
	{
	    r = _JPEG_MakeDC(pSrcDC, pTable, id);
	    if(r == GX_OK)
	    {
	        _JPEG_GetSize(pTable, id, &imageSize);
	        IMAGE_SetActiveRect(pImage, 0, 0, imageSize.w-1, imageSize.h-1);
	        pImage->uiPixelSize = 0;
	        pImage->pDynamicBuf = 0;
	    }
	}
	else*/
	{
		r = _ICON_MakeDC(pSrcDC, pTable, id);
		if (r == GX_OK) {
			_ICON_GetSize(pTable, id, &imageSize);
			IMAGE_SetActiveRect(pImage, 0, 0, imageSize.w - 1, imageSize.h - 1);
			pImage->uiPixelSize = 0;
			pImage->pDynamicBuf = 0;
		}
	}
	return r;
}

RESULT IMAGE_GetSizeFromTable(const IMAGE_TABLE *pTable, IVALUE id, ISIZE *pImageSize)
{
	RESULT r = GX_OK;

	GXGFX_ASSERT(pTable);
	GXGFX_ASSERT(pImageSize);
	/*if(_ICON_GetBpp(pTable, id) == 24)
	    r = _JPEG_GetSize(pTable, id, pImageSize);
	else*/
	r = _ICON_GetSize(pTable, id, pImageSize);
	if (r != GX_OK) {
		SIZE_Set(pImageSize, 0, 0);
	}
	return r;
}
#if 0
RESULT IMAGE_LoadFromJPG(IMAGE *pImage, UINT32 uiJPGData, UINT32 uiJPGSize)
{
	UINT16 w = 640, h = 240, fmt = PXLFMT_YUV422;
	GXGFX_ASSERT(pImage);
	GXGFX_ASSERT(uiJPGData);
	GXGFX_ASSERT(uiJPGSize);
	if (pImage == 0) {
		return GX_NULL_POINTER;
	}
	pImage->dc.uiType = TYPE_NULL;
	RECT_Set(&(pImage->rect), 0, 0, 0, 0);
	pImage->uiPixelSize = 0;
	pImage->pDynamicBuf = 0;
#if (GX_SUPPPORT_YUV ==ENABLE)
	//parsing w,h from JPG bitstream (optional)
	Jpeg_DecodeInfo((UINT8 *)uiJPGData, uiJPGSize, &w,  &h, &fmt); //fmt must be PXLFMT_YUV422 or PXLFMT_YUV420
#endif
	//create image DC
	GxGfx_AttachDC(&(pImage->dc), TYPE_JPEG, (UINT16)fmt, w, h, uiJPGSize, (UINT8 *)uiJPGData, 0, 0);
	//set active rect of image
	IMAGE_SetActiveRect(pImage, 0, 0, w - 1, h - 1);
	return GX_OK;
}

RESULT IMAGE_GetSizeFromJPG(UINT32 uiJPGData, UINT32 uiJPGSize, ISIZE *pImageSize)
{
	UINT16 w, h, fmt;
	//RESULT r = GX_OK;

	GXGFX_ASSERT(uiJPGData);
	GXGFX_ASSERT(uiJPGSize);
	SIZE_Set(pImageSize, 0, 0);
	w = 0, h = 0;
	//parsing w,h from JPG bitstream (optional)
	Jpeg_DecodeInfo((UINT8 *)uiJPGData, uiJPGSize, &w,  &h, &fmt); //fmt must be PXLFMT_YUV422 or PXLFMT_YUV420
	if (w == 0 && h == 0) {
		return GX_UNKNOWN_FORMAT;
	}
	SIZE_Set(pImageSize, w, h);
	return GX_OK;
}
#endif
RESULT IMAGE_SetActiveRect(IMAGE *pImage, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2)
{
	GXGFX_ASSERT(pImage);
	if (pImage == 0) {
		return GX_NULL_POINTER;
	}
	RECT_SetX1Y1X2Y2(&(pImage->rect), x1, y1, x2, y2);
	return GX_OK;
}

RESULT IMAGE_GetActiveRect(const IMAGE *pImage, IRECT *pRect)
{
	GXGFX_ASSERT(pImage);
	GXGFX_ASSERT(pRect);
	if (pImage == 0) {
		return GX_NULL_POINTER;
	}
	if (pRect == 0) {
		return GX_NULL_POINTER;
	}
	RECT_Set(pRect, 0, 0, 0, 0);
	*pRect = pImage->rect;
	return GX_OK;
}

ISIZE IMAGE_GetSize(const IMAGE *pImage)
{
	ISIZE imageSize;
	GXGFX_ASSERT(pImage);
	SIZE_Set(&imageSize, 0, 0);
	if (pImage == 0) {
		return imageSize;
	}
	SIZE_Set(&imageSize, pImage->dc.size.w, pImage->dc.size.h);
	return imageSize;
}


//--------------------------------------------------------------------------------------
//  DC draw image
//--------------------------------------------------------------------------------------
/*
-----------------------------------------------------------------------------------------------

Support style properties:

#define DS_IMAGETABLE       //support by DC_LoadIconTable
#define DS_IMAGEMAPPING     //support by DC_LoadColorMapping
#define DS_IMAGELAYOUT
#define DS_IMAGEOFFSET      //fixed = (0,0)
#define DS_IMAGESCALE       //support (calc from Rect)
#define DS_IMAGEALIGN       //fixed = IMAGEALIGN_LEFT|IMAGEALIGN_TOP
#define DS_IMAGEREPEAT
#define DS_IMAGEEFFECT      //support by DC_LoadImageEffect
#define DS_IMAGPARAM        //support by DC_LoadImageEffect

-----------------------------------------------------------------------------------------------
*/

/*
[TODO]

-----------------------------------------------------------------------------------------------
effect      formula                 AOP
-----------------------------------------------------------------------------------------------
NORMAL      //c = a;                //AOP 0: BITBLT_OP_COPY
SELECT      //c = (a==v)?a:b;       //AOP 3: BITBLT_OP_SRCKEY  //SelectBlt
MASK        //c = (b==v)?a:b;       //AOP 3: BITBLT_OP_DESTKEY //MaskBlt
OFFSET      //c = a (+/-) (b>>v);   //AOP 1 or AOP 2: BITBLT_OP_OFFSET
-----------------------------------------------------------------------------------------------
OPACITY     //c = a*v+b*(1-v)       //AOP 13: BITBLT_OP_BLEND
-----------------------------------------------------------------------------------------------
MULTIPLE    //c = a*b               //AOP 15
SCREEN      //c = 1-(1-a)*(1-b)     //INVERT(MULTIPLE(INVERT(a), INVERT(b)))
OVERLAY     //c = (b>0.5)?(2*(1-a)*(1-b)):(2*a*b)
SOFTLIGHT   //c = (a>0.5)?((2*a-1)*(b^(-2)-b)+b):((2*a-1)*(b-b^2)+b)
HARDLIGHT   //c = (a>0.5)?(2*(1-a)*(1-b)):(2*a*b)
LINEARLIGHT //c = b+2*a-1
PINLIGHT    //c = if(b<(2*a-1)):(2*a-1), if(b>(2*a)):(2*a), else:b
VIVIDLIGHT  //c = (a>0.5)?(b/(2*(1-a)):(1-(1-b)/2*a)
MARDMIX     //c = (a>(1-b))?1:0
-----------------------------------------------------------------------------------------------
COLORDODGE  //c = b/(1-a)
COLORBURN   //c = 1-(1-b)/a
LINEARDODGE //c = a+b
LINEARBURN  //c = a+b-1
-----------------------------------------------------------------------------------------------
DARKEN      //c = (a<b)?a:b         //N/A
LIGHTEN     //c = (a>=b)?a:b        //N/A
DIFFERENCE  //c = abs(a-b)          //AOP 2 with Bit[31]=1
EXCLUSIVE   //c = a+b-2*a*b         //N/A...equal to AOP 7?
-----------------------------------------------------------------------------------------------
HUE         //c.h|c.s|c.y = a.h|b.s|b.y
SATURATION  //c.h|c.s|c.y = b.h|a.s|b.y
COLOR       //c.h|c.s|c.y = a.h|a.s|b.y
LUMINOSITY  //c.h|c.s|c.y = b.h|b.s|a.y
-----------------------------------------------------------------------------------------------
ADD         //c = a+b               //AOP 2: BITBLT_OP_OFFSET
SUBTRUCT    //c = a-b               //AOP 2: BITBLT_OP_OFFSET
INVERT      //c = 1-a               //AOP 11 with text=255:
-----------------------------------------------------------------------------------------------

//GOP
DC_GeometryBlt(MIRROR)
DC_GeometryBlt(FLIP)
DC_GeometryBlt(ROTATE_90C)
DC_GeometryBlt(ROTATE_90CC)
DC_GeometryBlt(ROTATE_180)

//Gamma and HSV
DC_AdjustBlt(CURVE)
DC_AdjustBlt(COLORBALANCE)
DC_AdjustBlt(BRIGHTNESSCONTRAST)
DC_AdjustBlt(HUESATURATION)
*/


RESULT GxGfx_GetImageIDSize(IVALUE id, ISIZE *pImageSize)
{
	IMAGE_TABLE *pImageTable;
	GXGFX_ASSERT(pImageSize);
	if (!pImageSize) {
		return GX_NULL_POINTER;
	}
	SIZE_Set(pImageSize, 0, 0);
	pImageTable = (IMAGE_TABLE *)GxGfx_Get(BR_IMAGETABLE);
	if (!pImageTable) {
		return GX_NO_IMAGETABLE;
	}
	return IMAGE_GetSizeFromTable(pImageTable, id, pImageSize);
}

RESULT _GxGfx_DummyImage(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2)
{
	IRECT DestRect;
	IPOINT Pt1, Pt2;

	if (!pDestDC) {
		return GX_OK;
	}
	//draw
	RECT_SetX1Y1X2Y2(&DestRect, x1, y1, x2, y2);
	_GxGfx_FrameRect(pDestDC, &DestRect, SHAPELINESTYLE_DEFAULT, 1);
	POINT_Set(&Pt1, x1, y1);
	POINT_Set(&Pt2, x2, y2);
	_GxGfx_Line(pDestDC, &Pt1, &Pt2, SHAPELINESTYLE_DEFAULT, 1);
	POINT_Set(&Pt1, x1, y2);
	POINT_Set(&Pt2, x2, y1);
	_GxGfx_Line(pDestDC, &Pt1, &Pt2, SHAPELINESTYLE_DEFAULT, 1);
	return GX_OK;
}

RESULT GxGfx_Image(DC *pDestDC, LVALUE x, LVALUE y, const IMAGE *pImage)
{
	RESULT r;
	const DC *pSrcDC;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	if (!pImage) {
		IPOINT Pt;
		IRECT cw;

		POINT_Set(&Pt, x, y);
		_DC_XFORM_POINT(pDestDC, &Pt);
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);

		DBG_WRN("GxGfx_Image - WARNING! Image is null.\r\n");
		// draw unknown image
		_GxGfx_DummyImage(pDestDC, Pt.x, Pt.y, Pt.x + 32 - 1, Pt.y + 32 - 1);
		return GX_NULL_POINTER;
	}

	pSrcDC = &(pImage->dc);
	if (((DISPLAY_CONTEXT *)pSrcDC)->uiType == TYPE_NULL) {
		return GX_OK;
	}
	GXGFX_ASSERT((pSrcDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	{
		IPOINT Pt;
		PALETTE_ITEM *pColorTable;
		MAPPING_ITEM *pIndexTable;
		UINT32 uiRop, uiParam;
		IRECT cw;
		pColorTable = (PALETTE_ITEM *)GxGfx_Get(BR_IMAGEPALETTE);
		pIndexTable = (MAPPING_ITEM *)GxGfx_Get(BR_IMAGEMAPPING);

		uiRop = GxGfx_Get(BR_IMAGEROP);
		uiParam = GxGfx_Get(BR_IMAGEPARAM);

		POINT_Set(&Pt, x, y);
		_DC_XFORM_POINT(pDestDC, &Pt);
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);
		_GxGfx_SetImageROP(uiRop, uiParam, pColorTable, pIndexTable);

		if ((pImage->rect.x == 0) &&
			(pImage->rect.y == 0) &&
			(pImage->rect.w == 0) &&
			(pImage->rect.h == 0)) {
			if (uiRop == ROP_DESTFILL) {
				IRECT DestRect;
				RECT_Set(&DestRect, x, y, pSrcDC->size.w, pSrcDC->size.h);
				_DC_XFORM_RECT(pDestDC, &DestRect);
				r = _GxGfx_BitBlt(pDestDC, 0, pDestDC, &DestRect);
			} else {
				r = _GxGfx_BitBlt(pDestDC, &Pt, pSrcDC, 0);
			}
		} else {
			if (uiRop == ROP_DESTFILL) {
				IRECT DestRect;
				RECT_Set(&DestRect, x, y, pImage->rect.w, pImage->rect.h);
				_DC_XFORM_RECT(pDestDC, &DestRect);
				r = _GxGfx_BitBlt(pDestDC, 0, pDestDC, &DestRect);
			} else {
				r = _GxGfx_BitBlt(pDestDC, &Pt, pSrcDC, &(pImage->rect));
			}
		}
		GxGfx_MoveTo(pDestDC, x + pSrcDC->size.w, y + pSrcDC->size.h);
	}
	return r;
}

RESULT GxGfx_ImageInRect(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2, const IMAGE *pImage)   //put to rect
{
	UINT32 uiFlag, hAlign, vAlign;
	IRECT DestRect;
	IPOINT DestPt;
	ISIZE szImage;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	if (x1 == x2) {
		return GX_EMPTY_RECT;
	}
	if (y1 == y2) {
		return GX_EMPTY_RECT;
	}
	if (!pImage) {
		IPOINT Pt;
		IRECT cw;

		POINT_Set(&Pt, x1, y1);
		_DC_XFORM_POINT(pDestDC, &Pt);
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);

		DBG_WRN("GxGfx_Image - WARNING! Image is null.\r\n");
		// draw unknown image
		_GxGfx_DummyImage(pDestDC, Pt.x, Pt.y, Pt.x + (x2 - x1), Pt.y + (y2 - y1));
		return GX_NULL_POINTER;
	}

	uiFlag = GxGfx_Get(BR_IMAGEALIGN);
	RECT_SetX1Y1X2Y2(&DestRect, x1, y1, x2, y2);

	szImage = IMAGE_GetSize(pImage);
	_DC_INVSCALE_POINT(pDestDC, (IPOINT *)&szImage);

	hAlign = (uiFlag & ALIGN_H_MASK);
	if (hAlign == ALIGN_RIGHT) {
		DestPt.x = DestRect.x + DestRect.w - szImage.w;
	} else if (hAlign == ALIGN_CENTER) {
		DestPt.x = DestRect.x + ((DestRect.w - szImage.w) >> 1);
	} else { //if(hAlign == ALIGN_LEFT)
		DestPt.x = DestRect.x;
	}

	vAlign = (uiFlag & ALIGN_V_MASK);
	if (vAlign == ALIGN_BOTTOM) {
		DestPt.y = DestRect.y + DestRect.h - szImage.h;
	} else if (vAlign == ALIGN_MIDDLE) {
		DestPt.y = DestRect.y + ((DestRect.h - szImage.h) >> 1);
	} else { //if(vAlign == ALIGN_TOP)
		DestPt.y = DestRect.y;
	}

	return GxGfx_Image(pDestDC, DestPt.x, DestPt.y, pImage);
}


RESULT GxGfx_ImageScale(DC *pDestDC, LVALUE x, LVALUE y, LVALUE w, LVALUE h, const IMAGE *pImage)     //scaling to rect
{
	RESULT r;
	const DC *pSrcDC;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	if ((w == 0) || (h == 0)) {
		return GX_OK;    //ignore
	}
	if (!pImage) {
		IPOINT Pt;
		IRECT cw;

		POINT_Set(&Pt, x, y);
		_DC_XFORM_POINT(pDestDC, &Pt);
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);

		DBG_WRN("GxGfx_Image - WARNING! Image is null.\r\n");
		// draw unknown image
		_GxGfx_DummyImage(pDestDC, Pt.x, Pt.y, Pt.x + 32 - 1, Pt.y + 32 - 1);
		return GX_NULL_POINTER;
	}

	if ((pImage->rect.w == 0)
		|| (pImage->rect.h == 0)) {
		return GX_OK;    //ignore
	}

	pSrcDC = &(pImage->dc);
	if (((DISPLAY_CONTEXT *)pSrcDC)->uiType == TYPE_NULL) {
		return GX_OK;
	}
	GXGFX_ASSERT((pSrcDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	{
		IRECT DestRect;
		IRECT cw;
		PALETTE_ITEM *pColorTable;
		MAPPING_ITEM *pIndexTable;
		UINT32 uiRop, uiParam;
		pColorTable = (PALETTE_ITEM *)GxGfx_Get(BR_IMAGEPALETTE);
		pIndexTable = (MAPPING_ITEM *)GxGfx_Get(BR_IMAGEMAPPING);
		uiRop = GxGfx_Get(BR_IMAGEROP);
		uiParam = GxGfx_Get(BR_IMAGEPARAM);

		RECT_Set(&DestRect, x, y, w, h);
		_DC_XFORM_RECT(pDestDC, &DestRect);
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);
		_GxGfx_SetImageROP(uiRop, uiParam, pColorTable, pIndexTable);
		r = _GxGfx_StretchBlt(pDestDC, &DestRect, pSrcDC, &(pImage->rect));
		GxGfx_MoveTo(pDestDC, x + DestRect.w, y + DestRect.h);
	}
	return r;
}

RESULT GxGfx_ImageScaleInRect(DC *pDestDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2, LVALUE w, LVALUE h, const IMAGE *pImage)
{
	UINT32 uiFlag, hAlign, vAlign;
	IRECT DestRect;
	IPOINT DestPt;
	ISIZE szImage;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	if (x1 == x2) {
		return GX_EMPTY_RECT;
	}
	if (y1 == y2) {
		return GX_EMPTY_RECT;
	}
	if ((w == 0) || (h == 0)) {
		return GX_OK;    //ignore
	}
	if (!pImage) {
		IPOINT Pt;
		IRECT cw;

		POINT_Set(&Pt, x1, y1);
		_DC_XFORM_POINT(pDestDC, &Pt);
		cw = GxGfx_GetWindow(pDestDC);
		//_DC_SCALE_RECT(pDestDC, &cw);
		_DC_XFORM2_RECT(pDestDC, &cw);
		_GxGfx_SetClip(&cw);

		DBG_WRN("GxGfx_Image - WARNING! Image is null.\r\n");
		// draw unknown image
		_GxGfx_DummyImage(pDestDC, Pt.x, Pt.y, Pt.x + (x2 - x1), Pt.y + (y2 - y1));
		return GX_NULL_POINTER;
	}

	if ((pImage->rect.w == 0)
		|| (pImage->rect.h == 0)) {
		return GX_OK;    //ignore
	}

	uiFlag = GxGfx_Get(BR_IMAGEALIGN);
	RECT_SetX1Y1X2Y2(&DestRect, x1, y1, x2, y2);

	if (w < 0) {
		w = -w;
	}
	if (h < 0) {
		h = -h;
	}
	SIZE_Set(&szImage, w, h);

	hAlign = (uiFlag & ALIGN_H_MASK);
	if (hAlign == ALIGN_RIGHT) {
		DestPt.x = DestRect.x + DestRect.w - szImage.w;
	} else if (hAlign == ALIGN_CENTER) {
		DestPt.x = DestRect.x + ((DestRect.w - szImage.w) >> 1);
	} else { //if(hAlign == ALIGN_LEFT)
		DestPt.x = DestRect.x;
	}

	vAlign = (uiFlag & ALIGN_V_MASK);
	if (vAlign == ALIGN_BOTTOM) {
		DestPt.y = DestRect.y + DestRect.h - szImage.h;
	} else if (vAlign == ALIGN_MIDDLE) {
		DestPt.y = DestRect.y + ((DestRect.h - szImage.h) >> 1);
	} else { //if(vAlign == ALIGN_TOP)
		DestPt.y = DestRect.y;
	}

	return GxGfx_ImageScale(pDestDC, DestPt.x, DestPt.y, szImage.w, szImage.h, pImage);
}




IMAGE _gIMAGE_tmp;
//get icon content
//if pImageTable == 0, cannot get image by id, return NULL image
const IMAGE  *GxGfx_GetImageID(IVALUE id)
{
	RESULT r;
	const IMAGE_TABLE *pTable = (const IMAGE_TABLE *)GxGfx_Get(BR_IMAGETABLE);

	r = IMAGE_LoadFromTable(&_gIMAGE_tmp, pTable, id);
	if (r != GX_OK) {
		return 0;
	}
	return &_gIMAGE_tmp;
}

const IMAGE  *GxGfx_GetImageIDEx(IVALUE id, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2)
{
	RESULT r;
	const IMAGE_TABLE *pTable = (const IMAGE_TABLE *)GxGfx_Get(BR_IMAGETABLE);

	r = IMAGE_LoadFromTable(&_gIMAGE_tmp, pTable, id);
	if (r != GX_OK) {
		return 0;
	}
	IMAGE_SetActiveRect(&_gIMAGE_tmp, x1, y1, x2, y2);
	return &_gIMAGE_tmp;
}

