

#include "GxGfx/GxGfx.h"
#include "DC.h"
#include "DC_Blt.h"
#include "GxGfx_int.h"
#include "GxPixel.h"
#include "GxGrph.h"
#define FORCE_SW_I8   0
#include "vf_gfx.h"
#include <kwrap/cpu.h>

#if !FORCE_SW
#endif

//--------------------------------------------------------------------------------------
//  include
//--------------------------------------------------------------------------------------

/*

NOTE: GE ROP
0 :  A -> destination
1 :  A + (B >> SHF[1:0]) -> destination
2 :  A ¡V (B >> SHF[1:0])-> destination
3 :  A color key ( = ) with B -> destination
4 :  A color key ( < ) with B->destination
5 :  A & B -> destination
6 :  A | B -> destination
7 :  A ^ B -> destination
8 :  text -> destination
9 :  text & A -> destination
10 :  text | A -> destination
11 :  text ^ A -> destination
12 :  (text & A) | (~text & B) -> destination
13 :  Alpha blending : (A * WA) + (B * WB) -> destination
*/


UINT32 _Repeat4Bytes(UINT8 byte)
{
	UINT32 pattern;

	pattern = (UINT32)byte;
	pattern <<= 8;
	pattern |= (UINT32)byte;
	pattern <<= 8;
	pattern |= (UINT32)byte;
	pattern <<= 8;
	pattern |= (UINT32)byte;

	return pattern;
}

UINT32 _Repeat2Words(UINT16 word)
{
	UINT32 pattern;

	pattern = (UINT32)word;
	pattern <<= 16;
	pattern |= (UINT32)word;

	return pattern;
}


//--------------------------------------------------------------------------------------
//  DC Blt - Calculate Src/Dest by Rect
//--------------------------------------------------------------------------------------


UINT32 _DC_bltSrcOffset;
UINT32 _DC_bltSrcPitch;
UINT32 _DC_bltSrcWidth;
UINT32 _DC_bltSrcHeight;
UINT32 _DC_bltDestOffset;
UINT32 _DC_bltDestPitch;
UINT32 _DC_bltDestWidth;
UINT32 _DC_bltDestHeight;
UINT32 _DC_bltAlphaOffset;
UINT32 _DC_bltAlphaPitch;
UINT32 _DC_bltAlphaWidth;
UINT32 _DC_bltAlphaHeight;

void _grph_open_dummy(void) {}
void _grph_close_dummy(void) {}

void _grph_setImage1(const DC *_pSrcDC, UINT8 iCn, const IRECT *pSrcRect)
{
	const DISPLAY_CONTEXT *pSrcDC = (const DISPLAY_CONTEXT *)_pSrcDC;
	IRECT SrcPlaneRect;
	if (pSrcDC == 0) {
		_DC_bltSrcOffset = 0;
		_DC_bltSrcWidth = 0;
		_DC_bltSrcHeight = 0;
		_DC_bltSrcPitch = 0;
		return;
	}
	_DC_CalcPlaneRect(_pSrcDC, iCn, pSrcRect, &SrcPlaneRect);

	_DC_bltSrcOffset = _DC_CalcPlaneOffset(_pSrcDC, iCn, &SrcPlaneRect);    //NOTE: 4 byte alignment
	_DC_bltSrcWidth = SrcPlaneRect.w;// * GxGfx_GetPixelSize(_pSrcDC->uiPxlfmt, iCn); //NOTE: 4 byte alignment
	_DC_bltSrcHeight = SrcPlaneRect.h;
	_DC_bltSrcPitch = pSrcDC->blk[iCn].uiPitch; //NOTE: 4 byte alignment
	DBG_IND("_DC_bltSrcOffset %x _DC_bltSrcWidth  %d _DC_bltSrcHeight %d _DC_bltSrcPitch %d\r\n", _DC_bltSrcOffset, _DC_bltSrcWidth, _DC_bltSrcHeight, _DC_bltSrcPitch);

}

void _grph_setImage2(DC *_pDestDC, UINT8 iCn, const IRECT *pDestRect)
{
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT DestPlaneRect;
	if (pDestDC == 0) {
		_DC_bltDestOffset = 0;
		_DC_bltDestWidth = 0;
		_DC_bltDestHeight = 0;
		_DC_bltDestPitch = 0;
		return;
	}
	_DC_CalcPlaneRect(_pDestDC, iCn, pDestRect, &DestPlaneRect);

	_DC_bltDestOffset = _DC_CalcPlaneOffset(_pDestDC, iCn, &DestPlaneRect); //NOTE: 4 byte alignment
	_DC_bltDestWidth = DestPlaneRect.w;// * GxGfx_GetPixelSize(_pDestDC->uiPxlfmt, iCn); //NOTE: 4 byte alignment
	_DC_bltDestHeight = DestPlaneRect.h;
	_DC_bltDestPitch = pDestDC->blk[iCn].uiPitch;   //NOTE: 4 byte alignment

	DBG_IND("_DC_bltDestOffset %x _DC_bltDestWidth  %d _DC_bltDestHeight %d _DC_bltDestPitch %d\r\n", _DC_bltDestOffset, _DC_bltDestWidth, _DC_bltDestHeight, _DC_bltDestPitch);

}

void _grph_setImage3(const DC *_pSrcDC, UINT8 iCn, const IRECT *pSrcRect)
{
	const DISPLAY_CONTEXT *pSrcDC = (const DISPLAY_CONTEXT *)_pSrcDC;
	IRECT SrcPlaneRect;
	if (pSrcDC == 0) {
		_DC_bltAlphaOffset = 0;
		_DC_bltAlphaWidth = 0;
		_DC_bltAlphaHeight = 0;
		_DC_bltAlphaPitch = 0;
		return;
	}
	_DC_CalcPlaneRect(_pSrcDC, iCn, pSrcRect, &SrcPlaneRect);

	_DC_bltAlphaOffset = _DC_CalcPlaneOffset(_pSrcDC, iCn, &SrcPlaneRect);    //NOTE: 4 byte alignment
	_DC_bltAlphaWidth = SrcPlaneRect.w * GxGfx_GetPixelSize(_pSrcDC->uiPxlfmt, iCn); //NOTE: 4 byte alignment
	_DC_bltAlphaHeight = SrcPlaneRect.h;
	_DC_bltAlphaPitch = pSrcDC->blk[iCn].uiPitch; //NOTE: 4 byte alignment
}
#if !FORCE_SW
void grph_bindImageA(int image)
{
	if (image == 1) { //src
		memset(&(vGrphImages[0]), 0, sizeof(GRPH_IMG));
		vGrphImages[0].imgID =  GRPH_IMG_ID_A;       // source image
		vGrphImages[0].uiAddress = _DC_bltSrcOffset;
		vGrphImages[0].uiLineoffset = _DC_bltSrcPitch;
		vGrphImages[0].uiWidth = _DC_bltSrcWidth;
		vGrphImages[0].uiHeight = _DC_bltSrcHeight;
		vGrphImages[0].pInOutOP = NULL;
	} else if (image == 2) { //dest
		memset(&(vGrphImages[0]), 0, sizeof(GRPH_IMG));
		vGrphImages[0].imgID =  GRPH_IMG_ID_A;       // desination image
		vGrphImages[0].uiAddress = _DC_bltDestOffset;
		vGrphImages[0].uiLineoffset = _DC_bltDestPitch;
		vGrphImages[0].uiWidth = _DC_bltDestWidth;
		vGrphImages[0].uiHeight = _DC_bltDestHeight;
		vGrphImages[0].pInOutOP = NULL;
	}
}
void grph_bindImageB(int image)
{
	if (image == 1) {
		memset(&(vGrphImages[1]), 0, sizeof(GRPH_IMG));
		vGrphImages[1].imgID =  GRPH_IMG_ID_B;       // source image
		vGrphImages[1].uiAddress = _DC_bltSrcOffset;
		vGrphImages[1].uiLineoffset = _DC_bltSrcPitch;
		vGrphImages[1].uiWidth = _DC_bltSrcWidth;
		vGrphImages[1].uiHeight = _DC_bltSrcHeight;
		vGrphImages[1].pInOutOP = NULL;
	} else if (image == 2) { //dest
		memset(&(vGrphImages[1]), 0, sizeof(GRPH_IMG));
		vGrphImages[1].imgID =  GRPH_IMG_ID_B;       // desination image
		vGrphImages[1].uiAddress = _DC_bltDestOffset;
		vGrphImages[1].uiLineoffset = _DC_bltDestPitch;
		vGrphImages[1].uiWidth = _DC_bltDestWidth;
		vGrphImages[1].uiHeight = _DC_bltDestHeight;
		vGrphImages[1].pInOutOP = NULL;
	} else if (image == 3) { //source alpha
		memset(&(vGrphImages[1]), 0, sizeof(GRPH_IMG));
		vGrphImages[1].imgID =  GRPH_IMG_ID_B;       // desination image
		vGrphImages[1].uiAddress = _DC_bltAlphaOffset;
		vGrphImages[1].uiLineoffset = _DC_bltAlphaPitch;
		vGrphImages[1].uiWidth = _DC_bltAlphaWidth;
		vGrphImages[1].uiHeight = _DC_bltAlphaHeight;
		vGrphImages[1].pInOutOP = NULL;


	}
}
void grph_bindImageC(int image)
{
	if (image == 1) {
		memset(&(vGrphImages[2]), 0, sizeof(GRPH_IMG));
		vGrphImages[2].imgID =  GRPH_IMG_ID_C;       // source image
		vGrphImages[2].uiAddress = _DC_bltSrcOffset;
		vGrphImages[2].uiLineoffset = _DC_bltSrcPitch;
		vGrphImages[2].uiWidth = _DC_bltSrcWidth;
		vGrphImages[2].uiHeight = _DC_bltSrcHeight;
		vGrphImages[2].pInOutOP = NULL;
	} else if (image == 2) { //dest
		memset(&(vGrphImages[2]), 0, sizeof(GRPH_IMG));
		vGrphImages[2].imgID =  GRPH_IMG_ID_C;       // desination image
		vGrphImages[2].uiAddress = _DC_bltDestOffset;
		vGrphImages[2].uiLineoffset = _DC_bltDestPitch;
		vGrphImages[2].uiWidth = _DC_bltDestWidth;
		vGrphImages[2].uiHeight = _DC_bltDestHeight;
		vGrphImages[2].pInOutOP = NULL;

		DBG_IND("uiAddress 0x%x uiLineoffset %d W %d H %d\r\n", vGrphImages[2].uiAddress, vGrphImages[2].uiLineoffset, vGrphImages[2].uiWidth, vGrphImages[2].uiHeight);
	}
}
#else
void grph_bindImageA(int image)
{
}
void grph_bindImageB(int image)
{
}
void grph_bindImageC(int image)
{
}
#endif

void _grph_setImage1_dummy(const DC *_pSrcDC, UINT8 iCn, const IRECT *pSrcRect)
{
	const DISPLAY_CONTEXT *pSrcDC = (const DISPLAY_CONTEXT *)_pSrcDC;
	IRECT SrcPlaneRect;
	if (pSrcDC == 0) {
		return;
	}
	_DC_CalcPlaneRect(_pSrcDC, iCn, pSrcRect, &SrcPlaneRect);

	_DC_bltSrcOffset = _DC_CalcPlaneOffset(_pSrcDC, iCn, &SrcPlaneRect);    //NOTE: 4 byte alignment
	_DC_bltSrcWidth = SrcPlaneRect.w * GxGfx_GetPixelSize(_pSrcDC->uiPxlfmt, iCn); //NOTE: 4 byte alignment
	_DC_bltSrcHeight = SrcPlaneRect.h;
	_DC_bltSrcPitch = pSrcDC->blk[iCn].uiPitch; //NOTE: 4 byte alignment
}

void _grph_setImage2_dummy(DC *_pDestDC, UINT8 iCn, const IRECT *pDestRect)
{
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT DestPlaneRect;
	if (pDestDC == 0) {
		return;
	}
	_DC_CalcPlaneRect(_pDestDC, iCn, pDestRect, &DestPlaneRect);

	_DC_bltDestOffset = _DC_CalcPlaneOffset(_pDestDC, iCn, &DestPlaneRect); //NOTE: 4 byte alignment
	_DC_bltDestWidth = DestPlaneRect.w * GxGfx_GetPixelSize(_pDestDC->uiPxlfmt, iCn); //NOTE: 4 byte alignment
	_DC_bltDestHeight = DestPlaneRect.h;
	_DC_bltDestPitch = pDestDC->blk[iCn].uiPitch;   //NOTE: 4 byte alignment
}

RESULT _grph_chkImage1(void)
{
	/*

	//NT96610~NT96611 H/W limit:

	//check if is match WORD alignment limit
	if( !CHECK_WORD_ALIGNMENT(_DC_bltSrcOffset) )
	    return DC_ERROR_SIZEALIGN;
	//check if is match WORD alignment limit
	if( !CHECK_WORD_ALIGNMENT(_DC_bltSrcWidth) )
	    return DC_ERROR_SIZEALIGN;
	//check if is match WORD alignment limit
	if( !CHECK_WORD_ALIGNMENT(_DC_bltSrcPitch) )
	    return DC_ERROR_SIZEALIGN;

	*/
	return GX_OK;
}

RESULT _grph_chkImage2(void)
{
	/*

	//NT96610~NT96611 H/W limit:

	//check if is match WORD alignment limit
	if( !CHECK_WORD_ALIGNMENT(_DC_bltDestOffset) )
	    return DC_ERROR_SIZEALIGN;
	//check if is match WORD alignment limit
	if( !CHECK_WORD_ALIGNMENT(_DC_bltDestPitch) )
	    return DC_ERROR_SIZEALIGN;

	*/
	return GX_OK;
}

RESULT _grph_chkImage3(void)
{
	/*

	//NT96610~NT96611 H/W limit:

	//check if is match WORD alignment limit
	if( !CHECK_WORD_ALIGNMENT(_DC_bltDestOffset) )
	    return DC_ERROR_SIZEALIGN;
	//check if is match WORD alignment limit
	if( !CHECK_WORD_ALIGNMENT(_DC_bltDestPitch) )
	    return DC_ERROR_SIZEALIGN;

	*/
	return GX_OK;
}
#if GX_SUPPORT_ROTATE_ENG

void rotation_bindImageSrc(void)
{
	memset(&gRotationImageSRC, 0, sizeof(ROTATION_IMG));
	gRotationImageSRC.imgID =  ROTATION_IMG_ID_SRC;       // source image
	gRotationImageSRC.uiAddress = _DC_bltSrcOffset;
	gRotationImageSRC.uiLineoffset = _DC_bltSrcPitch;
	gRotationImageSRC.uiWidth = _DC_bltSrcWidth;
	gRotationImageSRC.uiHeight = _DC_bltSrcHeight;
	gRotationImageSRC.pNext = &gRotationImageDST;
	DBG_IND("_DC_bltSrcOffset  %x _DC_bltSrcPitch %d _DC_bltSrcWidth %d _DC_bltSrcHeight %d\r\n", _DC_bltSrcOffset, _DC_bltSrcPitch, _DC_bltSrcWidth, _DC_bltSrcHeight);
}

void rotation_bindImageDst(void)
{
	memset(&gRotationImageDST, 0, sizeof(ROTATION_IMG));
	gRotationImageDST.imgID =  ROTATION_IMG_ID_DST;       // desination image
	gRotationImageDST.uiAddress = _DC_bltDestOffset;
	gRotationImageDST.uiLineoffset = _DC_bltDestPitch;
	DBG_IND("_DC_bltDestOffset  %x _DC_bltDestPitch %d \r\n", _DC_bltDestOffset, _DC_bltDestPitch);

}
#endif


//--------------------------------------------------------------------------------------
//  DC StretchBlt - Calculate Src/Dest by Rect
//--------------------------------------------------------------------------------------


UINT32 _DC_SclSrcOffsetY;
UINT32 _DC_SclSrcOffsetU; //or UV
UINT32 _DC_SclSrcOffsetV;
UINT32 _DC_SclSrcPitch;
UINT32 _DC_SclSrcWidth;
UINT32 _DC_SclSrcHeight;
UINT32 _DC_SclDestOffsetY;
UINT32 _DC_SclDestOffsetU; //or UV
UINT32 _DC_SclDestOffsetV;
UINT32 _DC_SclDestPitch;
UINT32 _DC_SclDestWidth;  //NOTE: 4 byte alignment
UINT32 _DC_SclDestHeight;

//fmt: 0=A-RGB 2 planer(ARGB8565), 1=YUV_PK, 2=YUV Planer ,3=INDEX or ARGB_PK
void _ime_setImage1(const DC *_pSrcDC, int fmt, const IRECT *pSrcRect)
{
	const DISPLAY_CONTEXT *pSrcDC = (const DISPLAY_CONTEXT *)_pSrcDC;
	IRECT SrcPlaneRect;
	if (fmt == 0) { //RBG
		_DC_CalcPlaneRect(_pSrcDC, 0, pSrcRect, &SrcPlaneRect);  //plan rect = blk 0 src rect
		_DC_SclSrcOffsetY = _DC_CalcPlaneOffset(_pSrcDC, 0, &SrcPlaneRect); //blk 0 offset
		_DC_SclSrcWidth = SrcPlaneRect.w * GxGfx_GetPixelSize(_pSrcDC->uiPxlfmt, 0);
		_DC_SclSrcHeight = SrcPlaneRect.h;
		_DC_SclSrcPitch = pSrcDC->blk[0].uiPitch;  //RBG plan

		_DC_CalcPlaneRect(_pSrcDC, 3, pSrcRect, &SrcPlaneRect); //plan rect = src rect
		_DC_SclSrcOffsetU = _DC_CalcPlaneOffset(_pSrcDC, 3, &SrcPlaneRect); //blk 3 offset

		_DC_SclSrcOffsetV = 0;
	}
	if (fmt == 1) { //YUV_PK
		_DC_CalcPlaneRect(_pSrcDC, 0, pSrcRect, &SrcPlaneRect); //plan rect = blk 0 src rect
		_DC_SclSrcOffsetY = _DC_CalcPlaneOffset(_pSrcDC, 0, &SrcPlaneRect); //blk 0 offset
		_DC_SclSrcWidth = SrcPlaneRect.w * GxGfx_GetPixelSize(_pSrcDC->uiPxlfmt, 0);
		_DC_SclSrcHeight = SrcPlaneRect.h;
		_DC_SclSrcPitch = pSrcDC->blk[0].uiPitch;

		_DC_CalcPlaneRect(_pSrcDC, 1, pSrcRect, &SrcPlaneRect); //plan rect = blk 1 src rect
		_DC_SclSrcOffsetU = _DC_CalcPlaneOffset(_pSrcDC, 1, &SrcPlaneRect); //blk 1 offset

		_DC_SclSrcOffsetV = 0;
	}
	if (fmt == 2) { //YUV Planer
		_DC_CalcPlaneRect(_pSrcDC, 0, pSrcRect, &SrcPlaneRect); //plan rect = blk 0 src rect
		_DC_SclSrcOffsetY = _DC_CalcPlaneOffset(_pSrcDC, 0, &SrcPlaneRect); //blk 0 offset
		_DC_SclSrcWidth = SrcPlaneRect.w * GxGfx_GetPixelSize(_pSrcDC->uiPxlfmt, 0);
		_DC_SclSrcHeight = SrcPlaneRect.h;
		_DC_SclSrcPitch = pSrcDC->blk[0].uiPitch;

		_DC_CalcPlaneRect(_pSrcDC, 1, pSrcRect, &SrcPlaneRect); //plan rect = blk 1 src rect
		_DC_SclSrcOffsetU = _DC_CalcPlaneOffset(_pSrcDC, 1, &SrcPlaneRect); //blk 1 offset

		_DC_CalcPlaneRect(_pSrcDC, 2, pSrcRect, &SrcPlaneRect); //plan rect = blk 2 src rect
		_DC_SclSrcOffsetV = _DC_CalcPlaneOffset(_pSrcDC, 2, &SrcPlaneRect);//blk 2 offset
	}
	if (fmt == 3) { //index,ARGB_PK
		_DC_CalcPlaneRect(_pSrcDC, 3, pSrcRect, &SrcPlaneRect); //plan rect = blk 3 src rect
		_DC_SclSrcOffsetY = _DC_CalcPlaneOffset(_pSrcDC, 3, &SrcPlaneRect);
		_DC_SclSrcWidth = SrcPlaneRect.w * GxGfx_GetPixelSize(_pSrcDC->uiPxlfmt, 3);
		_DC_SclSrcHeight = SrcPlaneRect.h;
		_DC_SclSrcPitch = pSrcDC->blk[3].uiPitch;

		_DC_SclSrcOffsetU = 0;

		_DC_SclSrcOffsetV = 0;
	}

	DBG_IND("_DC_SclSrcOffsetY %x _DC_SclSrcWidth %d _DC_SclSrcHeight %d _DC_SclSrcPitch %d\r\n", _DC_SclSrcOffsetY, _DC_SclSrcWidth, _DC_SclSrcHeight, _DC_SclSrcPitch);

}

//fmt: 0=A-RGB 2 planer(ARGB8565), 1=YUV_PK, 2=YUV Planer ,3=INDEX or ARGB_PK
void _ime_setImage2(DC *_pDestDC, int fmt, const IRECT *pDestRect)
{
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT DestPlaneRect;
	if (fmt == 0) {
		_DC_CalcPlaneRect(_pDestDC, 0, pDestRect, &DestPlaneRect);
		_DC_SclDestOffsetY = _DC_CalcPlaneOffset(_pDestDC, 0, &DestPlaneRect);
		_DC_SclDestWidth = DestPlaneRect.w * GxGfx_GetPixelSize(_pDestDC->uiPxlfmt, 0);
		_DC_SclDestHeight = DestPlaneRect.h;
		_DC_SclDestPitch = pDestDC->blk[0].uiPitch;

		_DC_CalcPlaneRect(_pDestDC, 3, pDestRect, &DestPlaneRect);
		_DC_SclDestOffsetU = _DC_CalcPlaneOffset(_pDestDC, 3, &DestPlaneRect);

		_DC_SclDestOffsetV = 0;
	}
	if (fmt == 1) {
		_DC_CalcPlaneRect(_pDestDC, 0, pDestRect, &DestPlaneRect);
		_DC_SclDestOffsetY = _DC_CalcPlaneOffset(_pDestDC, 0, &DestPlaneRect);
		_DC_SclDestWidth = DestPlaneRect.w * GxGfx_GetPixelSize(_pDestDC->uiPxlfmt, 0);
		_DC_SclDestHeight = DestPlaneRect.h;
		_DC_SclDestPitch = pDestDC->blk[0].uiPitch;

		_DC_CalcPlaneRect(_pDestDC, 1, pDestRect, &DestPlaneRect);
		_DC_SclDestOffsetU = _DC_CalcPlaneOffset(_pDestDC, 1, &DestPlaneRect);

		_DC_SclDestOffsetV = 0;
	}
	if (fmt == 2) {
		_DC_CalcPlaneRect(_pDestDC, 0, pDestRect, &DestPlaneRect);
		_DC_SclDestOffsetY = _DC_CalcPlaneOffset(_pDestDC, 0, &DestPlaneRect);
		_DC_SclDestWidth = DestPlaneRect.w * GxGfx_GetPixelSize(_pDestDC->uiPxlfmt, 0);
		_DC_SclDestHeight = DestPlaneRect.h;
		_DC_SclDestPitch = pDestDC->blk[0].uiPitch;

		_DC_CalcPlaneRect(_pDestDC, 1, pDestRect, &DestPlaneRect);
		_DC_SclDestOffsetU = _DC_CalcPlaneOffset(_pDestDC, 1, &DestPlaneRect);

		_DC_CalcPlaneRect(_pDestDC, 2, pDestRect, &DestPlaneRect);
		_DC_SclDestOffsetV = _DC_CalcPlaneOffset(_pDestDC, 2, &DestPlaneRect);
	}
	if (fmt == 3) {
		_DC_CalcPlaneRect(_pDestDC, 3, pDestRect, &DestPlaneRect);
		_DC_SclDestWidth = DestPlaneRect.w * GxGfx_GetPixelSize(_pDestDC->uiPxlfmt, 3);
		_DC_SclDestHeight = DestPlaneRect.h;
		_DC_SclDestPitch = pDestDC->blk[3].uiPitch;

		_DC_SclDestOffsetY = _DC_CalcPlaneOffset(_pDestDC, 3, &DestPlaneRect);

		_DC_SclDestOffsetU = 0;

		_DC_SclDestOffsetV = 0;
	}

	DBG_IND("_DC_SclDestOffsetY %x _DC_SclDestWidth %d _DC_SclDestHeight %d _DC_SclDestPitch %d\r\n", _DC_SclDestOffsetY, _DC_SclDestWidth, _DC_SclDestHeight, _DC_SclDestPitch);

}

RESULT _ime_chkImage1(void)
{
#if 0
	if (!CHECK_WORD_ALIGNMENT(_DC_SclSrcOffsetY)) {
		DBG_ERR("_GxGfx_StretchBlt - ERROR! Not support bitblt from pSrcDC:YUV without src offset word alignment.\r\n");
		return GX_ERROR_XYALIGN;
	}
	//check if is match WORD alignment limit
	if (!CHECK_WORD_ALIGNMENT(_DC_SclSrcOffsetU)) {
		DBG_ERR("_GxGfx_StretchBlt - ERROR! Not support bitblt from pSrcDC:YUV without src offset word alignment.\r\n");
		return GX_ERROR_XYALIGN;
	}
	//check if is match WORD alignment limit
	if (!CHECK_WORD_ALIGNMENT(_DC_SclSrcOffsetV)) {
		DBG_ERR("_GxGfx_StretchBlt - ERROR! Not support bitblt from pSrcDC:YUV without src offset word alignment.\r\n");
		return GX_ERROR_XYALIGN;
	}
	//check if is match WORD alignment limit
	if (!CHECK_WORD_ALIGNMENT(_DC_SclSrcPitch)) {
		DBG_ERR("_GxGfx_StretchBlt - ERROR! Not support bitblt from pSrcDC:YUV without src pitch word alignment.\r\n");
		return GX_ERROR_SIZEALIGN;
	}
#endif
	return GX_OK;
}

RESULT _ime_chkImage2(void)
{
	/*
	    //check if is match WORD alignment limit
	    if( !CHECK_WORD_ALIGNMENT(_DC_SclDestOffsetY) )
	    {
	        DBG_ERR("_GxGfx_StretchBlt - ERROR! Not support bitblt to pDestDC:YUV without dest offset word alignment.\r\n");
	        return DC_ERROR_SIZEALIGN;
	    }
	*/
	return GX_OK;
}
#if !FORCE_SW
ISE_MODE_PRAM ise_op = {0};

//Cn: 0=Y, 1=U, 2=V
void ime_bindImageIN(int image, int Cn)
{
	if (image == 1) { //src
		if (Cn == 0) {
			ise_op.uiInWidth = _DC_SclSrcWidth;
			ise_op.uiInHeight = _DC_SclSrcHeight;
			ise_op.uiInLineoffset = _DC_SclSrcPitch;
			ise_op.uiInAddr = _DC_SclSrcOffsetY;
		}
		if (Cn == 1) {
			ise_op.uiInWidth = _DC_SclSrcWidth;
			ise_op.uiInHeight = _DC_SclSrcHeight;
			ise_op.uiInLineoffset = _DC_SclSrcPitch;
			ise_op.uiInAddr = _DC_SclSrcOffsetU;
		}
		if (Cn == 2) {
			ise_op.uiInWidth = _DC_SclSrcWidth;
			ise_op.uiInHeight = _DC_SclSrcHeight;
			ise_op.uiInLineoffset = _DC_SclSrcPitch;
			ise_op.uiInAddr = _DC_SclSrcOffsetV;
		}
	}

}
//Cn: 0=Y, 1=U, 2=V
void ime_bindImageOUT(int image, int Cn)
{
	if (image == 2) { //dest
		if (Cn == 0) {
			ise_op.uiOutWidth = _DC_SclDestWidth;
			ise_op.uiOutHeight = _DC_SclDestHeight;
			ise_op.uiOutLineoffset = _DC_SclDestPitch;
			ise_op.uiOutAddr = _DC_SclDestOffsetY;
		}
		if (Cn == 1) {
			ise_op.uiOutWidth = _DC_SclDestWidth;
			ise_op.uiOutHeight = _DC_SclDestHeight;
			ise_op.uiOutLineoffset = _DC_SclDestPitch;
			ise_op.uiOutAddr = _DC_SclDestOffsetU;
		}
		if (Cn == 2) {
			ise_op.uiOutWidth = _DC_SclDestWidth;
			ise_op.uiOutHeight = _DC_SclDestHeight;
			ise_op.uiOutLineoffset = _DC_SclDestPitch;
			ise_op.uiOutAddr = _DC_SclDestOffsetV;
		}
	}
}
#else
void ime_bindImageIN(int image, int Cn)
{
}
//Cn: 0=Y, 1=U, 2=V
void ime_bindImageOUT(int image, int Cn)
{
}
#endif
//--------------------------------------------------------------------------------------
//  DC Blt (SW)
//--------------------------------------------------------------------------------------




#define GRPH_CK_MSK                 0x000000FF  //7..0
#define GRPH_SHF_MSK                0x00000003  //1..0
#define GRPH_CK2_MSK                0x0000FF00  //7..0

// _sw_setAOP:
// uiProperty represents different properties depends on which AOP is using.
//      mode 1,2: property = SHF[1..0] of B
//      mode 3,4: property = ColorKey[7..0] to compare
//      mode 8,9,10,11,12: property = text[31..0]
//      mode 13: property= blending value of WA and WB.
//                          GRPH_BLD_WA_PER_2 = WA(1/2 )
//                          GRPH_BLD_WA_PER_2| GRPH_BLD_WA_PER_8 = WA(1/2+1/8) = WA(5/8)
//                          GRPH_BLD_WA_PER_2| GRPH_BLD_WA_PER_4 |GRPH_BLD_WB_PER_4 = WA(1/2+2/4), WB(1/4) = WA(3/4),WB(1/4)
ER _sw_setAOP(BOOL b16BitPrc, GRPH_DST_SEL dstSel, GRPH_AOP_MODE aopMode, UINT32 uiProperty)
{
#if 0
	UINT32 uiColorKey;
#endif
	switch (aopMode) {
#if 0
	case GRPH_AOP_COLOR_EQ: //this is ROP_KEY path
		uiColorKey = uiProperty & GRPH_CK_MSK;
		if (dstSel == GRPH_DST_2) {
#if 0 //fast path (optimized by dword read/write)
			//
			//    Damm, NOT really faster then slow path!!!
			//    it is too complex ...
			//
			register UINT32 i, j;
			UINT32 *pSrcBaseDW;
			INT32 iSrcPitch;
			UINT32 *pDestBaseDW;
			INT32 iDestPitch;
			INT32 iSrcBits;
			INT32 iDestBits;

			INT32 h = (_DC_bltSrcHeight);
			INT32 w = (_DC_bltSrcWidth);

			pSrcBaseDW = (UINT32 *)(_DC_bltSrcOffset & (~0x03));
			iSrcPitch = (_DC_bltSrcPitch) << 3;
			iSrcBits = (INT32)(_DC_bltSrcOffset & 0x03) << 3;

			pDestBaseDW = (UINT32 *)(_DC_bltDestOffset & (~0x03));
			iDestPitch = (_DC_bltDestPitch) << 3;
			iDestBits = (INT32)(_DC_bltDestOffset & 0x03) << 3;

			for (j = 0; j < h; j++) {
				int load = 0;
				int dirty = 0;
				register UINT32 s_dword, d_dword;
				register UINT32 *pDestAddrDW;
				register UINT32 *pSrcAddrDW;
				register INT32 di, si;

				//1.read src_buf from src line data
				pSrcAddrDW = pSrcBaseDW + (iSrcBits >> 5);
				si = iSrcBits & 0x01f; //pixel shift bits in dword
				s_dword = (*pSrcAddrDW);

				//2.read dst_dword from dst line data
				pDestAddrDW = pDestBaseDW + (iDestBits >> 5);
				di = iDestBits & 0x01f; //pixel shift bits in dword

				//if(di)  //remove this line if current ROP need read DEST value
				{
					//d_dword = (*pDestAddrDW);
				}

				for (i = 0; i < w; i++) {
					register UINT32 sm, dm;
					//register UINT32 sv,dv,rv;
					register UINT32 sv, rv;

					//3.get src pixel data from src_buf
					sm = (0x000000FF << si);
					sv = (s_dword & sm) >> si;

					//3.get dst pixel data from dst_buf
					dm = (0x000000FF << di);
					//dv = (d_dword & dm) >> di;

					//4.do ROP with src and dst pixel data, and put to result pixel data
					if (sv != uiColorKey) {
						rv = sv;

						if (!load) {
							d_dword = (*pDestAddrDW);
							load = 1;
						}

						//5.put result pixel data to dst_dword
						d_dword = (d_dword & ~dm) | (sv << di);
						dirty = 1;
					}

					si += 8;
					if (si == 32) {
						//read src_dword from src_ptr
						si = 0;
						pSrcAddrDW++;
						s_dword = (*pSrcAddrDW);
					}
					di += 8;
					if (di == 32) {
						//6.write dst_dword to dst_ptr
						if (dirty) {
							(*pDestAddrDW) = d_dword;
						}

						//read dst_dword from dst_ptr
						di = 0;
						pDestAddrDW++;

						//d_dword = (*pDestAddrDW);  //add this line if current ROP need read DEST value
						dirty = 0;
						load = 0;
					}
				}

				if (di) {
					//force flush dst_dword at end of line
					if (dirty) {
						(*pDestAddrDW) = d_dword;
					}
				}

				//next src line
				iSrcBits += iSrcPitch;

				//next dst line
				iDestBits += iDestPitch;
			}
#else
			if (b16BitPrc) {
				register UINT32 i, j;
				UINT16 *pSrc;
				UINT16 *pDst;
				pSrc = (UINT16 *)_DC_bltSrcOffset;
				pDst = (UINT16 *)_DC_bltDestOffset;
				for (j = 0; j < _DC_bltSrcHeight; j++) {
					for (i = 0; i < _DC_bltSrcWidth; i++) {
						if (pSrc[i] == uiColorKey) {
							//color key
						} else {
							//copy
							pDst[i] = pSrc[i];
						}
					}
					pSrc += (_DC_bltSrcPitch >> 1);
					pDst += (_DC_bltDestPitch >> 1);
				}
			} else {
				register UINT32 i, j;
				UINT8 *pSrc;
				UINT8 *pDst;
				pSrc = (UINT8 *)_DC_bltSrcOffset;
				pDst = (UINT8 *)_DC_bltDestOffset;
				for (j = 0; j < _DC_bltSrcHeight; j++) {
					for (i = 0; i < _DC_bltSrcWidth; i++) {
						if (pSrc[i] == uiColorKey) {
							//color key
						} else {
							//copy
							pDst[i] = pSrc[i];
						}
					}
					pSrc += _DC_bltSrcPitch;
					pDst += _DC_bltDestPitch;
				}
			}
#endif
		}
		break;
#endif
	case GRPH_AOP_A_COPY: //this is ROP_COPY path
		if (dstSel == GRPH_DST_2) {
			//if(!b16BitPrc)
			{
				register UINT32 i, j;
				UINT8 *pSrc;
				UINT8 *pDst;
				pSrc = (UINT8 *)_DC_bltSrcOffset;
				pDst = (UINT8 *)_DC_bltDestOffset;
				for (j = 0; j < _DC_bltSrcHeight; j++) {
					for (i = 0; i < _DC_bltSrcWidth; i++) {
						//copy
						pDst[i] = pSrc[i];
					}
					pSrc += _DC_bltSrcPitch;
					pDst += _DC_bltDestPitch;
				}
			}
		}
		break;
	case GRPH_AOP_COLOR_EQ: //this is ROP_KEY or ROP_DESTKEY path
		if (dstSel == GRPH_DST_2) {
			if (!b16BitPrc) {
				UINT32 uiColorKey = uiProperty & GRPH_CK_MSK;
				register UINT32 i, j;
				UINT8 *pSrc;
				UINT8 *pDst;
				pSrc = (UINT8 *)_DC_bltSrcOffset;
				pDst = (UINT8 *)_DC_bltDestOffset;
				for (j = 0; j < _DC_bltSrcHeight; j++) {
					for (i = 0; i < _DC_bltSrcWidth; i++) {
						if (pSrc[i] == uiColorKey) {
							//color key
						} else {
							//copy
							pDst[i] = pSrc[i];
						}
					}
					pSrc += _DC_bltSrcPitch;
					pDst += _DC_bltDestPitch;
				}
			} else {
				UINT32 uiColorKey = uiProperty & GRPH_CK_MSK;
				UINT32 uiColorKey2 = (uiProperty & GRPH_CK2_MSK) >> 8;
				register UINT32 i, j;
				UINT8 *pSrc;
				UINT8 *pDst;
				pSrc = (UINT8 *)_DC_bltSrcOffset;
				pDst = (UINT8 *)_DC_bltDestOffset;
				for (j = 0; j < _DC_bltSrcHeight; j++) {
					for (i = 0; i < _DC_bltSrcWidth; i++) {
						if (i % 2 == 0) {
							if (pSrc[i] == uiColorKey2) {
								//color key
							} else {
								//copy
								pDst[i] = pSrc[i];
							}
						} else {
							if (pSrc[i] == uiColorKey) {
								//color key
							} else {
								//copy
								pDst[i] = pSrc[i];
							}
						}
					}
					pSrc += _DC_bltSrcPitch;
					pDst += _DC_bltDestPitch;
				}
			}
		}
		break;
	case GRPH_AOP_TEXT_COPY: //this is ROP_DESTFILL path
		if (dstSel == GRPH_DST_2) {
			if (!b16BitPrc) {
				UINT32 uiColorText = uiProperty & GRPH_CK_MSK;
				register UINT32 i, j;
				UINT8 *pSrc;
				UINT8 *pDst;
				pSrc = (UINT8 *)_DC_bltSrcOffset;
				pDst = (UINT8 *)_DC_bltDestOffset;
				for (j = 0; j < _DC_bltSrcHeight; j++) {
					for (i = 0; i < _DC_bltSrcWidth; i++) {
						//text copy
						pDst[i] = ((UINT8)uiColorText);
					}
					pSrc += _DC_bltSrcPitch;
					pDst += _DC_bltDestPitch;
				}
			} else {
				UINT32 uiColorText = uiProperty & GRPH_CK_MSK;
				UINT32 uiColorText2 = (uiProperty & GRPH_CK2_MSK) >> 8;
				register UINT32 i, j;
				UINT8 *pSrc;
				UINT8 *pDst;
				pSrc = (UINT8 *)_DC_bltSrcOffset;
				pDst = (UINT8 *)_DC_bltDestOffset;
				for (j = 0; j < _DC_bltSrcHeight; j++) {
					for (i = 0; i < _DC_bltSrcWidth; i++) {
						//text copy
						if (i % 2 == 0) {
							pDst[i] = ((UINT8)uiColorText2);
						} else {
							pDst[i] = ((UINT8)uiColorText);
						}
					}
					pSrc += _DC_bltSrcPitch;
					pDst += _DC_bltDestPitch;
				}
			}
		}
		break;
	case GRPH_AOP_TEXT_XOR_A: //this is ROP_DESTNOT & ROP_NOT path
		if (dstSel == GRPH_DST_2) {
			if (!b16BitPrc) {
				UINT32 uiColorText = uiProperty & GRPH_CK_MSK;
				register UINT32 i, j;
				UINT8 *pSrc;
				UINT8 *pDst;
				pSrc = (UINT8 *)_DC_bltSrcOffset;
				pDst = (UINT8 *)_DC_bltDestOffset;
				for (j = 0; j < _DC_bltSrcHeight; j++) {
					for (i = 0; i < _DC_bltSrcWidth; i++) {
						//text xor
						pDst[i] = ((UINT8)uiColorText) ^ (pDst[i]);
					}
					pSrc += _DC_bltSrcPitch;
					pDst += _DC_bltDestPitch;
				}
			}
		}
		break;
	case GRPH_AOP_A_AND_B: //this is ROP_AND path
		if (dstSel == GRPH_DST_2) {
			if (!b16BitPrc) {
				register UINT32 i, j;
				UINT8 *pSrc;
				UINT8 *pDst;
				pSrc = (UINT8 *)_DC_bltSrcOffset;
				pDst = (UINT8 *)_DC_bltDestOffset;
				for (j = 0; j < _DC_bltSrcHeight; j++) {
					for (i = 0; i < _DC_bltSrcWidth; i++) {
						//and
						pDst[i] = (pDst[i]) & (pSrc[i]);
					}
					pSrc += _DC_bltSrcPitch;
					pDst += _DC_bltDestPitch;
				}
			}
		}
		break;
	case GRPH_AOP_A_OR_B: //this is ROP_OR path
		if (dstSel == GRPH_DST_2) {
			if (!b16BitPrc) {
				register UINT32 i, j;
				UINT8 *pSrc;
				UINT8 *pDst;
				pSrc = (UINT8 *)_DC_bltSrcOffset;
				pDst = (UINT8 *)_DC_bltDestOffset;
				for (j = 0; j < _DC_bltSrcHeight; j++) {
					for (i = 0; i < _DC_bltSrcWidth; i++) {
						//or
						pDst[i] = (pDst[i]) | (pSrc[i]);
					}
					pSrc += _DC_bltSrcPitch;
					pDst += _DC_bltDestPitch;
				}
			}
		}
		break;
	case GRPH_AOP_A_XOR_B: //this is ROP_XOR path
		if (dstSel == GRPH_DST_2) {
			if (!b16BitPrc) {
				register UINT32 i, j;
				UINT8 *pSrc;
				UINT8 *pDst;
				pSrc = (UINT8 *)_DC_bltSrcOffset;
				pDst = (UINT8 *)_DC_bltDestOffset;
				for (j = 0; j < _DC_bltSrcHeight; j++) {
					for (i = 0; i < _DC_bltSrcWidth; i++) {
						//xor
						pDst[i] = (pDst[i]) ^ (pSrc[i]);
					}
					pSrc += _DC_bltSrcPitch;
					pDst += _DC_bltDestPitch;
				}
			}
		}
		break;
	default:
		DBG_ERR("Not support this BitBlt ROP! %d\r\n", aopMode);
		break;
	}
	return E_OK;
}



//condition : pDestAddr is not null
//condition : pDestAddr must be 4 bytes alignment
//condition : iDestPitch must be 4 bytes alignment
//condition : uiWidth must be 4 bytes alignment
RESULT _DC_RectMemSet(UINT8 *pDestAddr, INT16 iDestPitch,
					  UINT16 uiWidth, UINT16 uiHeight, UINT8 ucValue)
{
#if (FORCE_SW == 1)
	DBG_ERR("Not support MemSet!\r\n");
	return GX_OK;
#else
	ER ERcode;
	/*

	//NT96610~NT96611 H/W limit:
	if(uiWidth==0 || uiHeight==0)
	    return GX_OK;
	if(!CHECK_WORD_ALIGNMENT(pDestAddr))
	    return GX_ERROR_XYALIGN;
	if(!CHECK_WORD_ALIGNMENT(iDestPitch)
	|| !CHECK_WORD_ALIGNMENT(uiWidth))
	    return GX_ERROR_SIZEALIGN;

	*/

	graph_open(GRPH_ID_1);

	memset(&grphRequest, 0, sizeof(grphRequest));
	//[set images]
	memset(&(vGrphImages[0]), 0, sizeof(GRPH_IMG));
	vGrphImages[0].imgID =  GRPH_IMG_ID_A;       // source image
	vGrphImages[0].uiAddress = (UINT32)pDestAddr;
	vGrphImages[0].uiLineoffset = iDestPitch;
	vGrphImages[0].uiWidth = uiWidth;
	vGrphImages[0].uiHeight = uiHeight;
	vGrphImages[0].pInOutOP = NULL;
	vGrphImages[0].pNext = 0;
	memset(&(vGrphImages[1]), 0, sizeof(GRPH_IMG));
	vGrphImages[1].imgID =  GRPH_IMG_ID_C;       // destination image
	vGrphImages[1].uiAddress = (UINT32)pDestAddr;
	vGrphImages[1].uiLineoffset = iDestPitch;
	vGrphImages[1].uiWidth = uiWidth;
	vGrphImages[1].uiHeight = uiHeight;
	vGrphImages[1].pInOutOP = NULL;
	vGrphImages[1].pNext = 0;
	grphRequest.pImageDescript = &(vGrphImages[0]);
	vGrphImages[0].pNext = &(vGrphImages[1]);
	vGrphImages[1].pNext = 0;
	//[set format]
	grphRequest.format = GRPH_FORMAT_8BITS;
	//[set operation]
	grphRequest.command = GRPH_CMD_TEXT_COPY;
	//[set parameter]
	grphRequest.pProperty = &(vGrphProperty[0]);
	vGrphProperty[0].id = GRPH_PROPERTY_ID_NORMAL;
	vGrphProperty[0].uiProperty = _Repeat4Bytes(ucValue);
	vGrphProperty[0].pNext = 0;
	//[do HW process]
	ERcode = graph_request(GRPH_ID_1, &grphRequest);

	graph_close(GRPH_ID_1);

	if (ERcode != E_OK) {
		return GX_DRAW_FAILED;
	}

	return GX_OK;
#endif
}


//condition : pDestAddr is not null
//condition : pSrcAddr is not null
//condition : pDestAddr must be 4 bytes alignment
//condition : iDestPitch must be 4 bytes alignment
//condition : pSrcAddr must be 4 bytes alignment
//condition : iSrcPitch must be 4 bytes alignment
//condition : uiWidth must be 4 bytes alignment
RESULT _DC_RectMemCopy(UINT8 *pDestAddr, INT16 iDestPitch, UINT8 *pSrcAddr, INT16 iSrcPitch,
					   UINT16 uiWidth, UINT16 uiHeight)
{
#if (FORCE_SW_I8 == 1)
	DBG_ERR("Not support MemCopy!\r\n");
	return GX_OK;
#else
   	VF_GFX_COPY         param ={0};
	ER ERcode;
    HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};

	if (uiWidth == 0 || uiHeight == 0) {
		return GX_OK;
	}

    vir_meminfo.va = (void *)(pSrcAddr);
    ERcode = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
	if ( ERcode != HD_OK) {
		DBG_ERR("_DC_bltSrcOffset map fail %d\n",ERcode);
		return GX_OUTOF_MEMORY;
	}
	memset(&param, 0, sizeof(VF_GFX_COPY));
	ERcode = vf_init(&param.src_img, uiWidth, uiHeight, HD_VIDEO_PXLFMT_I8, iSrcPitch, vir_meminfo.pa, iSrcPitch*uiHeight);
	if(ERcode){
		DBG_ERR("fail to init source image\n");
		return ERcode;
	}
    vir_meminfo.va = (void *)(pDestAddr);
    ERcode = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
	if ( ERcode != HD_OK) {
		DBG_ERR("_DC_bltSrcOffset map fail %d\n",ERcode);
		return GX_OUTOF_MEMORY;
	}

	ERcode = vf_init(&param.dst_img, uiWidth, uiHeight, HD_VIDEO_PXLFMT_I8, iDestPitch, vir_meminfo.pa, iDestPitch*uiHeight);
	if(ERcode){
		DBG_ERR("fail to init dest image\n");
		return ERcode;
	}
	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = uiWidth;
	param.src_region.h             = uiHeight;
	param.dst_pos.x                = 0;
	param.dst_pos.y                = 0;
	param.colorkey                 = 0;
	param.alpha                    = 255;

    return vf_gfx_copy(&param);
#endif
}



//--------------------------------------------------------------------------------------
//  Rect Blt (SW)
//--------------------------------------------------------------------------------------
#include <string.h>

//condition : pDestAddr is not null
RESULT _RectMemSet(UINT8 *pDestAddr, INT16 iDestPitch,
				   UINT16 uiWidth, UINT16 uiHeight, UINT8 ucValue)
{
	register UINT16 i;
	if (uiWidth == 0 || uiHeight == 0) {
		return GX_OK;
	}
	if ((iDestPitch == uiWidth)) {
		memset(pDestAddr, ucValue, ((UINT32)uiWidth)*uiHeight);
		return GX_OK;
	}
	for (i = 0; i < uiHeight; i++) {
		memset(pDestAddr, ucValue, uiWidth);
		pDestAddr += iDestPitch;
	}
	return GX_OK;
}


//condition : pDestAddr is not null
//condition : pSrcAddr is not null
RESULT _RectMemCopy(UINT8 *pDestAddr, INT16 iDestPitch, UINT8 *pSrcAddr, INT16 iSrcPitch,
					UINT16 uiWidth, UINT16 uiHeight)
{
	register UINT16 i;
	if (uiWidth == 0 || uiHeight == 0) {
		return GX_OK;
	}
	if ((iDestPitch == iSrcPitch) && (iDestPitch == uiWidth)) {
		memcpy(pDestAddr, pSrcAddr, ((UINT32)uiWidth)*uiHeight);
		return GX_OK;
	}
	for (i = 0; i < uiHeight; i++) {
		memcpy(pDestAddr, pSrcAddr, uiWidth);
		pSrcAddr += iSrcPitch;
		pDestAddr += iDestPitch;
	}
	return GX_OK;
}
#if !FORCE_SW

#if GX_SUPPORT_ROTATE_ENG
RESULT _grph_do_rotation(UINT32 cmd,UINT32 format)
{
	ER ERcode;

    DBG_IND("cmd %d format %d\r\n",cmd,format);
	rotation_open(ROTATION_ID_1);

	memset(&gRotationRequest, 0, sizeof(gRotationRequest));

	rotation_bindImageSrc();
	rotation_bindImageDst();

	gRotationRequest.command = cmd;
	gRotationRequest.format = format;
	gRotationRequest.pImageDescript = &gRotationImageSRC;

	ERcode = rotation_request(ROTATION_ID_1, &gRotationRequest);

	rotation_close(ROTATION_ID_1);

    return ERcode ;
}
#else
RESULT _grph_do_rotation(UINT32 cmd,UINT32 format)
{
	ER ERcode;

    DBG_IND("cmd %d format %d\r\n",cmd,format);
	graph_open(GRPH_ID_2);

	memset(&grphRequest, 0, sizeof(grphRequest));
	//[set images]
	ERcode = _grph_chkImage1();
	if (ERcode != GX_OK) {
		return ERcode;
	}
	ERcode = _grph_chkImage2();
	if (ERcode != GX_OK) {
		return ERcode;
	}
	grph_bindImageA(1); //A = source image
	grph_bindImageC(2); //C = desination image
	grphRequest.pImageDescript = &(vGrphImages[0]);
	vGrphImages[0].pNext = &(vGrphImages[2]);
	vGrphImages[2].pNext = 0;
	//[set format]
	grphRequest.format = format;
	//[set operation]
	grphRequest.command = cmd;
	//[set parameter]
	grphRequest.pProperty = 0;
	//[do HW process]

	ERcode = graph_request(GRPH_ID_2, &grphRequest);

	graph_close(GRPH_ID_2);

    return ERcode;
}
#endif

#else
RESULT _grph_do_rotation(UINT32 cmd,UINT32 format)
{
	DBG_ERR("Not support rotate!\r\n");
	return GX_OK;
}
#endif
