
#include "GxGfx/GxGfx.h"
#include "DC.h"
#include "DC_Blt.h"
#include "GxGfx_int.h"
#include "GxPixel.h"
#include "GxGrph.h"

#define FORCE_SW_4444   1
#if !FORCE_SW_4444
#include "vf_gfx.h"
#include <kwrap/cpu.h>

extern UINT32 _DC_bltSrcOffset;
extern UINT32 _DC_bltSrcPitch;
extern UINT32 _DC_bltSrcWidth;
extern UINT32 _DC_bltSrcHeight;
extern UINT32 _DC_bltDestOffset;
extern UINT32 _DC_bltDestPitch;
extern UINT32 _DC_bltDestWidth;
extern UINT32 _DC_bltDestHeight;

extern UINT32 _DC_SclSrcOffsetY;
extern UINT32 _DC_SclSrcOffsetU; //or UV
extern UINT32 _DC_SclSrcOffsetV;
extern UINT32 _DC_SclSrcPitch;
extern UINT32 _DC_SclSrcWidth;
extern UINT32 _DC_SclSrcHeight;
extern UINT32 _DC_SclDestOffsetY;
extern UINT32 _DC_SclDestOffsetU; //or UV
extern UINT32 _DC_SclDestOffsetV;
extern UINT32 _DC_SclDestPitch;
extern UINT32 _DC_SclDestWidth;  //NOTE: 4 byte alignment
extern UINT32 _DC_SclDestHeight;
#endif

//--------------------------------------------------------------------------------------
//  DC Blt (INDEX format)
//--------------------------------------------------------------------------------------
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by _pDestDC
RESULT _DC_FillRect_ARGB4444_sw(DC *_pDestDC, const IRECT *pDestRect,
								UINT32 uiRop, COLOR_ITEM uiColor, const MAPPING_ITEM *pTable)
{
	//DISPLAY_CONTEXT* pDestDC = (DISPLAY_CONTEXT*)_pDestDC;
	IRECT DestRect;
	//UINT32 DestSHIFT, DestMASK;

	UINT32 rop = uiRop & ROP_MASK;

	RECT_SetRect(&DestRect, pDestRect);


#if 0//USE_FAST_BLT //fast path (optimized by dword read/write)
	{
		INT32 i, j;
		UINT32 *pBuf = 0;

		pBuf = (UINT32 *)pDestDC->blk[3].uiOffset;

		for (i = 0; i < DestRect.h; i++) {
			for (j = 0; j < DestRect.w; j++) {
				*((UINT32 *)pBuf + (i * DestRect.w + j)) = uiColor;
			}
			//debug_msg("addr %x %08X \r\n",((UINT32 *)pBuf+(i*DestRect.w+j)),*((UINT32 *)pBuf+(i*DestRect.w+j)));
		}
	}
#else //slow path
	{
		LVALUE i, j;
		PIXEL_2D DestPx = {0};
		PIXEL_2D_Init(&DestPx, _pDestDC);
		PIXEL_2D_Check(&DestPx);
		if (rop == ROP_DESTFILL) {
			PIXEL_2D_Begin(&DestPx, &DestRect);
			for (j = 0; j < DestRect.h; j++) {
				PIXEL_2D_BeginLine(&DestPx);
				for (i = 0; i < DestRect.w; i++) {
					UINT32 DestValue;

					PIXEL_2D_BeginPixel(&DestPx);
					//================================
					//pixel op: set
					DestValue = (UINT32)uiColor;    //set

					if (pTable) {
						DestValue = pTable[DestValue];    //replace
					}

					//================================
					PIXEL_2D_Write(&DestPx, DestValue);
					PIXEL_2D_EndPixel(&DestPx, FALSE, TRUE);
				}
				PIXEL_2D_EndLine(&DestPx);
			}
			PIXEL_2D_End(&DestPx);
		}

	}
#endif //path

	return GX_OK;
}


/*
//  AOP                     Meaning                         property value (n)
//  --------------------------------------------------------------------------------------------
//  GRPH_AOP_A_COPY         dest = A                        N/A
//  --------------------------------------------------------------------------------------------
//  GRPH_AOP_PLUS_SHF       dest = A + (B<<s)               Shift s[1..0]
//  GRPH_AOP_MINUS_SHF      dest = A - (B<<s)
//  GRPH_AOP_MINUS_SHF_ABS  dest = abs ( A + (B<<s) )
//  --------------------------------------------------------------------------------------------
//  GRPH_AOP_COLOR_EQ       dest = (c == B)?A:B             ColorKey c[7..0]
//  GRPH_AOP_COLOR_LE       dest = (c < B)?A:B
//  GRPH_AOP_COLOR_MRANDEQ  dest = (c >= B)?A:B
//  --------------------------------------------------------------------------------------------
//  GRPH_AOP_TEXT_COPY      dest = t                        Text t[31..0]
//  GRPH_AOP_TEXT_AND_A     dest = t & A
//  GRPH_AOP_TEXT_OR_A      dest = t | A
//  GRPH_AOP_TEXT_XOR_A     dest = t ^ A
//  GRPH_AOP_TEXT_AND_AB    dest = (t & A) | (~t & B)
//  --------------------------------------------------------------------------------------------
//  GRPH_AOP_BLENDING       dest = fa * A + fb * B          A Weighting wa[0..2]
//                                                              wa1 = 0x01, factor fa1 = 1/8
//                                                              wa2 = 0x02, factor fa2 = 1/4
//                                                              wa3 = 0x04, factor fa3 = 1/2
//                                                              wa = wa1 | wa2 | wa3
//                                                              A Factor fa = fa1 * fa2 * fa3
//                                                          B Weighting wb[3..5]
//                                                              wb1 = 0x01, factor fb1 = 1/8
//                                                              wb2 = 0x02, factor fb2 = 1/4
//                                                              wb3 = 0x04, factor fb3 = 1/2
//                                                              wb = wb1 | wb2 | wb3
//                                                              B Factor fb = fb1 * fb2 * fb3
//  --------------------------------------------------------------------------------------------
//  GRPH_AOP_MULTIPLY_DIV   dest = (A * B)>>d               Divide d[2..0]
//  --------------------------------------------------------------------------------------------
*/
#define COLOR_RGBA4444_GET_COLOR(c)   (COLOR_ITEM)(((c) & 0x00000fff) >> 0) ///< get RGB channel value of RGBA4444 color
#define COLOR_RGBA4444_GET_A(c)       (CVALUE)(((c) & 0x0000f000) >> 12) ///< get A channel value of RGBA4444 color
#define COLOR_RGBA4444_GET_B(c)       (CVALUE)(((c) & 0x0000000f) >> 0) ///< get B channel value of RGBA4444 color
#define COLOR_RGBA4444_GET_G(c)       (CVALUE)(((c) & 0x000000f0) >> 4) ///< get G channel value of RGBA4444 color
#define COLOR_RGBA4444_GET_R(c)       (CVALUE)(((c) & 0x00000f00) >> 8) ///< get R channel value of RGBA4444 color

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_BitBlt_ARGB4444(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
						   UINT32 uiRop, UINT32 uiParam)
{

#if (FORCE_SW_4444 == 1)
		return _DC_BitBlt_ARGB4444_sw(_pDestDC, pDestRect, _pSrcDC, pSrcRect, uiRop, uiParam, 0);
#else
	{
		DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
		DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
		IRECT DestRect;
		IRECT SrcRect;
		ER ERcode;
		UINT32 cparam[1];
	    UINT32 rop = uiRop & ROP_MASK;


		RECT_SetRect(&DestRect, pDestRect);
		RECT_SetRect(&SrcRect, pSrcRect);

		//check rect zero
		if (_DC_IS_ZERO_INDEX(pDestDC, &DestRect)) {
			return GX_OK;    //skip this draw
		}
		if (_DC_IS_ZERO_INDEX(pSrcDC, &SrcRect)) {
			return GX_OK;    //skip this draw
		}

		switch (rop) {
		case ROP_COPY:
			cparam[0] = 0;
			break;
		case ROP_KEY:
			cparam[0] = COLOR_RGBA4444_GET_COLOR(uiParam);
			break;
		case ROP_DESTFILL:
			_pSrcDC = _pDestDC; //_pSrcDC=0, let _pSrcDC equal to _pDestDC
			pDestRect = pSrcRect;   //pDestRect=0, let pDestRect equal to pSrcRect
			RECT_SetRect(&DestRect, pDestRect);
			cparam[0] = _Repeat2Words(COLOR_RGBA4444_GET_COLOR(uiParam));
			break;
        #if 0
		case ROP_MUL_CNST_ALPHA:
			AOP = GRPH_CMD_TEXT_MUL;
			cparam[0] = GRPH_TEXT_MULT_PROPTY(GRPH_TEXTMUL_BYTE, GRPH_TEXTMUL_UNSIGNED, uiParam, 4);
			break;
		case ROP_BLEND:
			AOP = GRPH_CMD_PLANE_BLENDING;
			cparam[0] = 0;
			break;
		case ROP_BLEND_CNST_ALPHA:
			AOP = GRPH_CMD_BLENDING;
			cparam[0] = GRPH_BLD_WA_WB_THR(uiParam, (256 - uiParam), 0xFF);
			break;
        #endif
		default :
			return GX_NOTSUPPORT_ROP;

		}


		_grph_setImage1(_pSrcDC, 3, &SrcRect);
		_grph_setImage2(_pDestDC, 3, &DestRect);

        if ((rop == ROP_COPY)||(rop == ROP_KEY)){
        	VF_GFX_COPY         param ={0};
        	memset(&param, 0, sizeof(VF_GFX_COPY));
        	ERcode = vf_init(&param.src_img, _DC_bltSrcWidth, _DC_bltSrcHeight, HD_VIDEO_PXLFMT_ARGB4444, _DC_bltSrcPitch, vos_cpu_get_phy_addr(_DC_bltSrcOffset), _DC_bltSrcPitch*_DC_bltSrcHeight);
        	if(ERcode){
        		DBG_ERR("fail to init source image\n");
        		return ERcode;
        	}
        	ERcode = vf_init(&param.dst_img, _DC_bltDestWidth, _DC_bltDestHeight, HD_VIDEO_PXLFMT_ARGB4444, _DC_bltDestPitch, vos_cpu_get_phy_addr(_DC_bltDestOffset), _DC_bltDestPitch*_DC_bltDestHeight);
        	if(ERcode){
        		DBG_ERR("fail to init dest image\n");
        		return ERcode;
        	}
        	param.src_region.x             = 0;
        	param.src_region.y             = 0;
        	param.src_region.w             = SrcRect.w;
        	param.src_region.h             = SrcRect.h;
        	param.dst_pos.x                = 0;
        	param.dst_pos.y                = 0;
        	param.colorkey                 = cparam[0];
        	param.alpha                    = 255;

        	ERcode = vf_gfx_copy(&param);

        }else if(rop == ROP_DESTFILL) {
            VF_GFX_DRAW_RECT    param;
            memset(&param, 0, sizeof(VF_GFX_DRAW_RECT));
        	ERcode = vf_init(&param.dst_img, _DC_bltDestWidth, _DC_bltDestHeight, HD_VIDEO_PXLFMT_ARGB4444, _DC_bltDestPitch, vos_cpu_get_phy_addr(_DC_bltDestOffset), _DC_bltDestPitch*_DC_bltDestHeight);
        	if(ERcode){
        		DBG_ERR("fail to init dest image\n");
    		    return ERcode;
        	}

            param.color                    = cparam[0];
        	param.rect.x                   = 0;
        	param.rect.y                   = 0;
        	param.rect.w                   = pDestRect->w;
        	param.rect.h                   = pDestRect->h;
        	param.type                     = HD_GFX_RECT_SOLID;
        	ERcode = vf_gfx_draw_rect(&param);

        }
		return ERcode;

	}
    #endif
}

static UINT32 RGBA4444_OpAlphaBlending(UINT32 SrcValue, UINT32 DestValue)
{
	UINT32 OutValue;
	CVALUE r1, g1, b1, a1;
	CVALUE r2, g2, b2, a2;
	CVALUE r3, g3, b3, a3;

	//================================
	//pixel op: blend
	OutValue = 0;

	a1 = COLOR_RGB444_GET_A(SrcValue);
	r1 = COLOR_RGB444_GET_R(SrcValue);
	g1 = COLOR_RGB444_GET_G(SrcValue);
	b1 = COLOR_RGB444_GET_B(SrcValue);
	a2 = COLOR_RGB444_GET_A(DestValue);
	r2 = COLOR_RGB444_GET_R(DestValue);
	g2 = COLOR_RGB444_GET_G(DestValue);
	b2 = COLOR_RGB444_GET_B(DestValue);
	if (a1 == 255) {
		a3 = a2; //not change
		r3 = r1;
		g3 = g1;
		b3 = b1;
	} else if (a1 == 0) {
		a3 = a2; //not change
		r3 = r2;
		g3 = g2;
		b3 = b2;
	} else {
		a3 = a2; //not change
		//BLEND_SRCALPHA, BLENDOP_ADD, BLEND_INVSRCALPHA
		r3 = (((UINT32)r1) * (a1) + ((UINT32)r2) * (255 - a1)) >> 8;
		g3 = (((UINT32)g1) * (a1) + ((UINT32)g2) * (255 - a1)) >> 8;
		b3 = (((UINT32)b1) * (a1) + ((UINT32)b2) * (255 - a1)) >> 8;
	}
	OutValue = COLOR_MAKE_ARGB4444(r3, g3, b3, a3);
	return OutValue;
}

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_BitBlt_ARGB4444_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							  UINT32 uiRop, UINT32 uiParam, const MAPPING_ITEM *pTable)
{
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT DestRect;
	IRECT SrcRect;

	//UINT32 SrcSHIFT, DestSHIFT;
	//UINT32 SrcMASK, DestMASK;
	UINT32 rop = uiRop & ROP_MASK;
	UINT32 colorkey=0;

	if (rop == ROP_DESTFILL) {
		return _DC_FillRect_ARGB4444_sw(_pDestDC, pSrcRect, uiRop, uiParam, 0);
	}


	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_RGBA4444_PK :
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pDestDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}
	switch (pSrcDC->uiPxlfmt) {
	case PXLFMT_RGBA4444_PK :
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pSrcDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}

	switch (rop) {
	case ROP_COPY:
		break;
	case ROP_KEY:
		colorkey = (UINT32)uiParam;
		break;
	case ROP_BLEND:
		break;
	default:
		return GX_NOTSUPPORT_ROP;
	}

	RECT_SetRect(&DestRect, pDestRect);
	RECT_SetRect(&SrcRect, pSrcRect);

	//check rect zero
	//check rect zero
	if (_DC_IS_ZERO_INDEX(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}
	if (_DC_IS_ZERO_INDEX(pSrcDC, &SrcRect)) {
		return GX_OK;    //skip this draw
	}


	//ARGB4444 -> ARGB4444
	if (rop == ROP_COPY) { //ROP_COPY
		PIXEL_2D DestPx = {0};
		PIXEL_2D SrcPx = {0};
		LVALUE i, j;
		PIXEL_2D_Init(&DestPx, _pDestDC);
		PIXEL_2D_Init(&SrcPx, (DC *)(UINT32)_pSrcDC);
		PIXEL_2D_Check(&DestPx);
		PIXEL_2D_Check(&SrcPx);
		PIXEL_2D_Begin(&DestPx, &DestRect);
		PIXEL_2D_Begin(&SrcPx, &SrcRect);
		for (j = 0; j < SrcRect.h; j++) {
			PIXEL_2D_BeginLine(&DestPx);
			PIXEL_2D_BeginLine(&SrcPx);
			for (i = 0; i < SrcRect.w; i++) {
				UINT32 SrcValue, DestValue;

				PIXEL_2D_BeginPixel(&DestPx);
				PIXEL_2D_BeginPixel(&SrcPx);
				SrcValue = PIXEL_2D_Read(&SrcPx);
				//================================
				//pixel op: copy
				DestValue = SrcValue; //copy

				//================================
				{
					PIXEL_2D_Write(&DestPx, DestValue);
				}
				PIXEL_2D_EndPixel(&DestPx, FALSE, TRUE);
				PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE);
			}
			PIXEL_2D_EndLine(&DestPx);
			PIXEL_2D_EndLine(&SrcPx);
		}
		PIXEL_2D_End(&DestPx);
		PIXEL_2D_End(&SrcPx);

	} else if (rop == ROP_BLEND) { //ROP_COPY
		PIXEL_2D DestPx = {0};
		PIXEL_2D SrcPx = {0};
		LVALUE i, j;
		PIXEL_2D_Init(&DestPx, _pDestDC);
		PIXEL_2D_Init(&SrcPx, (DC *)(UINT32)_pSrcDC);
		PIXEL_2D_Check(&DestPx);
		PIXEL_2D_Check(&SrcPx);
		PIXEL_2D_Begin(&DestPx, &DestRect);
		PIXEL_2D_Begin(&SrcPx, &SrcRect);
		for (j = 0; j < SrcRect.h; j++) {
			PIXEL_2D_BeginLine(&DestPx);
			PIXEL_2D_BeginLine(&SrcPx);
			for (i = 0; i < SrcRect.w; i++) {
				UINT32 SrcValue, DestValue, BlendValue;

				PIXEL_2D_BeginPixel(&DestPx);
				PIXEL_2D_BeginPixel(&SrcPx);
				SrcValue = PIXEL_2D_Read(&SrcPx);
				DestValue = PIXEL_2D_Read(&DestPx);
				//================================
				//pixel op: blend
				BlendValue = RGBA4444_OpAlphaBlending(SrcValue, DestValue);
				DestValue = BlendValue;

				//================================
				{
					PIXEL_2D_Write(&DestPx, DestValue);
				}
				PIXEL_2D_EndPixel(&DestPx, FALSE, TRUE);
				PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE);
			}
			PIXEL_2D_EndLine(&DestPx);
			PIXEL_2D_EndLine(&SrcPx);
		}
		PIXEL_2D_End(&DestPx);
		PIXEL_2D_End(&SrcPx);

	} else if (rop == ROP_KEY) { //ROP_KEY
		PIXEL_2D DestPx = {0};
		PIXEL_2D SrcPx = {0};
		LVALUE i, j;
		PIXEL_2D_Init(&DestPx, _pDestDC);
		PIXEL_2D_Init(&SrcPx, (DC *)(UINT32)_pSrcDC);
		PIXEL_2D_Check(&DestPx);
		PIXEL_2D_Check(&SrcPx);
		PIXEL_2D_Begin(&DestPx, &DestRect);
		PIXEL_2D_Begin(&SrcPx, &SrcRect);
		for (j = 0; j < SrcRect.h; j++) {
			PIXEL_2D_BeginLine(&DestPx);
			PIXEL_2D_BeginLine(&SrcPx);
			for (i = 0; i < SrcRect.w; i++) {
				UINT32 SrcValue, DestValue;

				PIXEL_2D_BeginPixel(&DestPx);
				PIXEL_2D_BeginPixel(&SrcPx);
				SrcValue = PIXEL_2D_Read(&SrcPx);
				//================================
				//pixel op: copy
				DestValue = SrcValue; //copy

				//================================
				if (SrcValue == colorkey) {
					//do color key
				} else {
					PIXEL_2D_Write(&DestPx, DestValue);
				}
				PIXEL_2D_EndPixel(&DestPx, FALSE, TRUE);
				PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE);
			}
			PIXEL_2D_EndLine(&DestPx);
			PIXEL_2D_EndLine(&SrcPx);
		}
		PIXEL_2D_End(&DestPx);
		PIXEL_2D_End(&SrcPx);

	}

	return GX_OK;

}

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_RGBA4444_PK format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_INDEX format
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
//condition : pTable is not null (for INDEX to RGB444 convert)
RESULT _DC_BitBlt_INDEX2RGB444_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
								  DC_CTRL *pc)
{
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT DestRect;
	IRECT SrcRect;

	UINT32 uiRop = pc->uiRop;
	UINT32 uiParam = pc->uiParam;
	const PALETTE_ITEM *pColorTable = pc->pColorTable;
	const MAPPING_ITEM *pIndexTable = pc->pIndexTable;
	UINT32 rop = uiRop & ROP_MASK;
	UINT32 colorkey;

	switch (pSrcDC->uiPxlfmt) {
	case PXLFMT_INDEX1 :
		break;
	case PXLFMT_INDEX2 :
		break;
	case PXLFMT_INDEX4 :
		break;
	case PXLFMT_INDEX8 :
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pSrcDC format. %x\r\n", pSrcDC->uiPxlfmt);
			return GX_CANNOT_CONVERT;
		}
	}

	colorkey = 0;
	switch (rop) {
	case ROP_COPY:
		break;
	case ROP_KEY:
	case ROP_DESTFILL:
		colorkey = (UINT32)uiParam;
		break;
	default:
		return GX_NOTSUPPORT_ROP;
	}

	RECT_SetRect(&DestRect, pDestRect);
	RECT_SetRect(&SrcRect, pSrcRect);

	//check rect zero
	if (_DC_IS_ZERO_INDEX(pSrcDC, &SrcRect)) {
		return GX_OK;    //skip this draw
	}
	if (_DC_IS_ZERO_RGB_PK(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}

	//INDEX1 -> PXLFMT_RGB444_PK
	//INDEX1 -> PXLFMT_RGBA5658_PK
	//INDEX1 -> PXLFMT_RGBA5654_PK
	//INDEX2 -> PXLFMT_RGB444_PK
	//INDEX2 -> PXLFMT_RGBA5658_PK
	//INDEX2 -> PXLFMT_RGBA5654_PK
	//INDEX4 -> PXLFMT_RGB444_PK
	//INDEX4 -> PXLFMT_RGBA5658_PK
	//INDEX4 -> PXLFMT_RGBA5654_PK
	//INDEX8 -> PXLFMT_RGB444_PK
	//INDEX8 -> PXLFMT_RGBA5658_PK
	//INDEX8 -> PXLFMT_RGBA5654_PK

	if (rop == ROP_COPY) { //ROP_COPY
		PIXEL_2D DestPx = {0};
		PIXEL_2D SrcPx = {0};
		LVALUE i, j;
		PIXEL_2D_Init(&DestPx, _pDestDC);
		PIXEL_2D_Init(&SrcPx, (DC *)(UINT32)_pSrcDC);
		PIXEL_2D_Check(&DestPx);
		PIXEL_2D_Check(&SrcPx);
		PIXEL_2D_Begin(&DestPx, &DestRect);
		PIXEL_2D_Begin(&SrcPx, &SrcRect);

		for (j = 0; j < SrcRect.h; j++) {
			PIXEL_2D_BeginLine(&DestPx);
			PIXEL_2D_BeginLine(&SrcPx);
			for (i = 0; i < SrcRect.w; i++) {
				UINT32 SrcValue, DestValue;

				PIXEL_2D_BeginPixel(&DestPx);
				PIXEL_2D_BeginPixel(&SrcPx);
				SrcValue = PIXEL_2D_Read(&SrcPx);
				//================================
				//pixel op: copy
				DestValue = SrcValue; //copy

				//================================
				{
					if (pIndexTable) {
						DestValue = pIndexTable[DestValue];    //replace
					}
					if (pColorTable) {
						DestValue = pColorTable[DestValue];    //replace
					} else {
						DestValue = 0x00000fff;
					}

					PIXEL_2D_Write(&DestPx, DestValue);
				}
				PIXEL_2D_EndPixel(&DestPx, FALSE, TRUE);
				PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE);
			}
			PIXEL_2D_EndLine(&DestPx);
			PIXEL_2D_EndLine(&SrcPx);
		}
		PIXEL_2D_End(&DestPx);
		PIXEL_2D_End(&SrcPx);

		return GX_OK;
	} else if (rop == ROP_KEY) { //ROP_KEY
		PIXEL_2D DestPx = {0};
		PIXEL_2D SrcPx = {0};
		LVALUE i, j;
		PIXEL_2D_Init(&DestPx, _pDestDC);
		PIXEL_2D_Init(&SrcPx, (DC *)(UINT32)_pSrcDC);
		PIXEL_2D_Check(&DestPx);
		PIXEL_2D_Check(&SrcPx);
		PIXEL_2D_Begin(&DestPx, &DestRect);
		PIXEL_2D_Begin(&SrcPx, &SrcRect);
		for (j = 0; j < SrcRect.h; j++) {
			PIXEL_2D_BeginLine(&DestPx);
			PIXEL_2D_BeginLine(&SrcPx);
			for (i = 0; i < SrcRect.w; i++) {
				UINT32 SrcValue, DestValue;

				PIXEL_2D_BeginPixel(&DestPx);
				PIXEL_2D_BeginPixel(&SrcPx);
				SrcValue = PIXEL_2D_Read(&SrcPx);
				if (pIndexTable) {
					SrcValue = pIndexTable[SrcValue];    //replace
				}
				if (pColorTable) {
					SrcValue = pColorTable[SrcValue];    //replace
				}
				//================================

				if (COLOR_RGBA4444_GET_COLOR(SrcValue) == COLOR_RGBA4444_GET_COLOR(colorkey)) {
					//do color key
				} else {
					//================================

					DestValue = SrcValue; //copy

					PIXEL_2D_Write(&DestPx, DestValue);
				}
				PIXEL_2D_EndPixel(&DestPx, FALSE, TRUE);
				PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE);
			}
			PIXEL_2D_EndLine(&DestPx);
			PIXEL_2D_EndLine(&SrcPx);
		}
		PIXEL_2D_End(&DestPx);
		PIXEL_2D_End(&SrcPx);

		return GX_OK;
	}

	return GX_OK;
}

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_RotateBlt_ARGB4444(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							  UINT32 uiRop, UINT32 uiParam)
{
#if (FORCE_SW_4444 == 1)
	DBG_ERR("NOT support rotate!\r\n");
    return GX_ERROR_PARAM;
#else

	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT DestRect;
	IRECT SrcRect;
	UINT32 rop = uiRop & ROP_MASK;
	ER ERcode;
	UINT32 GOP;

	RECT_SetRect(&DestRect, pDestRect);
	RECT_SetRect(&SrcRect, pSrcRect);

	//check rect zero
	if (_DC_IS_ZERO_INDEX(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}
	if (_DC_IS_ZERO_INDEX(pSrcDC, &SrcRect)) {
		return GX_OK;    //skip this draw
	}

	if (!(rop & GR_MASK)) {
		return GX_NOTSUPPORT_ROP;
	}

	rop = rop & GR_OP;

	switch (rop) {
	case GR_RCW_90:
		GOP = HD_VIDEO_DIR_ROTATE_90;
		break;
	case GR_RCW_180:
		GOP = HD_VIDEO_DIR_ROTATE_180;
		break;
	case GR_RCW_270:
		GOP = HD_VIDEO_DIR_ROTATE_270;
		break;
	case GR_RCW_90|GR_MRR_X:
		GOP = HD_VIDEO_DIR_ROTATE_90|HD_VIDEO_DIR_MIRRORX;
		break;
	case GR_RCW_180|GR_MRR_X:
		GOP = HD_VIDEO_DIR_ROTATE_180|HD_VIDEO_DIR_MIRRORX;
		break;
	case GR_RCW_270|GR_MRR_X:
		GOP = HD_VIDEO_DIR_ROTATE_270|HD_VIDEO_DIR_MIRRORX;
		break;
	case GR_RCW_90|GR_MRR_Y:
		GOP = HD_VIDEO_DIR_ROTATE_90|HD_VIDEO_DIR_MIRRORY;
		break;
	case GR_RCW_180|GR_MRR_Y:
		GOP = HD_VIDEO_DIR_ROTATE_180|HD_VIDEO_DIR_MIRRORY;
		break;
	case GR_RCW_270|GR_MRR_Y:
		GOP = HD_VIDEO_DIR_ROTATE_270|HD_VIDEO_DIR_MIRRORY;
		break;
	case GR_RCW_90|GR_MRR_X|GR_MRR_Y:
		GOP = HD_VIDEO_DIR_ROTATE_90|HD_VIDEO_DIR_MIRRORXY;
		break;
	case GR_RCW_180|GR_MRR_X|GR_MRR_Y:
		GOP = HD_VIDEO_DIR_ROTATE_180|HD_VIDEO_DIR_MIRRORXY;
		break;
	case GR_RCW_270|GR_MRR_X|GR_MRR_Y:
		GOP = HD_VIDEO_DIR_ROTATE_270|HD_VIDEO_DIR_MIRRORXY;
		break;
	case GR_RCW_360|GR_MRR_X:
		GOP = HD_VIDEO_DIR_MIRRORX;
		break;
	case GR_RCW_360|GR_MRR_Y:
		GOP = HD_VIDEO_DIR_MIRRORY;
		break;
	default:
		return GX_NOTSUPPORT_ROP;
	}
	_grph_setImage1(_pSrcDC, 3, &SrcRect);
	_grph_setImage2(_pDestDC, 3, &DestRect);

    {
    	VF_GFX_ROTATE         param ={0};
    	memset(&param, 0, sizeof(VF_GFX_ROTATE));
    	ERcode = vf_init(&param.src_img, _DC_bltSrcWidth, _DC_bltSrcHeight, HD_VIDEO_PXLFMT_ARGB4444, _DC_bltSrcPitch, vos_cpu_get_phy_addr(_DC_bltSrcOffset), _DC_bltSrcPitch*_DC_bltSrcHeight);
    	if(ERcode){
    		DBG_ERR("fail to init source image\n");
    		return ERcode;
    	}
    	ERcode = vf_init(&param.dst_img, _DC_bltDestWidth, _DC_bltDestHeight, HD_VIDEO_PXLFMT_ARGB4444, _DC_bltDestPitch, vos_cpu_get_phy_addr(_DC_bltDestOffset), _DC_bltDestPitch*_DC_bltDestHeight);
    	if(ERcode){
    		DBG_ERR("fail to init dest image\n");
    		return ERcode;
    	}
    	param.src_region.x             = 0;
    	param.src_region.y             = 0;
    	param.src_region.w             = SrcRect.w;
    	param.src_region.h             = SrcRect.h;
    	param.dst_pos.x                = 0;
    	param.dst_pos.y                = 0;
    	param.angle                    = GOP;

    	ERcode = vf_gfx_rotate(&param);

    }
	return ERcode;
#endif
}
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_FontBlt_INDEX2ARGB4444_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
									 const MAPPING_ITEM *pTable)
{
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT DestRect;
	IRECT SrcRect;

	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_RGBA4444_PK :
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pDestDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}
	switch (pSrcDC->uiPxlfmt) {
	case PXLFMT_INDEX1 :
		break;
	case PXLFMT_INDEX2 :
		break;
	case PXLFMT_INDEX4 :
		break;
	case PXLFMT_INDEX8 :
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pSrcDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}

	if (pTable == 0) { //using mapping table
		DBG_ERR("_GxGfx_BitBlt - ERROR! Need specify mapping for ROP_FONT.\r\n");
		return GX_CANNOT_CONVERT;
	}

	RECT_SetRect(&DestRect, pDestRect);
	RECT_SetRect(&SrcRect, pSrcRect);
	//check rect zero
	if (_DC_IS_ZERO_INDEX(pSrcDC, &SrcRect)) {
		return GX_OK;    //skip this draw
	}
	if (_DC_IS_ZERO_INDEX(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}
//#if USE_FAST_BLT //fast path
#if 0

// ROP_KEY + Mapping //////////////////////////////////////////////////////////////////////////////////

	{
		MAPPING_ITEM LocalTable[4];
		UINT32 *pSrcBaseDW;
		INT32 iSrcOffset;
		INT32 iSrcBase;
		INT32 iSrcPitch;
		UINT32 *pDestBaseDW;
		INT32 iDestOffset;
		INT32 iDestBase;
		INT32 iDestPitch;

		INT32 h = (SrcRect.h);
		INT32 j;
		INT32 w = (SrcRect.w);
		INT32 i;

		LocalTable[0] = pTable[0];
		LocalTable[1] = pTable[1];
		LocalTable[2] = pTable[2];
		LocalTable[3] = pTable[3];

		pSrcBaseDW = (UINT32 *)(pSrcDC->blk[3].uiOffset & (~0x03));
		iSrcBase = (INT32)(pSrcDC->blk[3].uiOffset & 0x03) << 3;
		switch (pSrcDC->uiType) {
		case TYPE_FB:
		case TYPE_BITMAP:
			iSrcPitch = (pSrcDC->blk[3].uiPitch) << 3;
			break;
		case TYPE_ICON:
			iSrcPitch = (pSrcDC->blk[3].uiWidth) << SrcSHIFT;
			break;
		}
		iSrcOffset = iSrcPitch * SrcRect.y + ((SrcRect.x) << SrcSHIFT);

		pDestBaseDW = (UINT32 *)(pDestDC->blk[3].uiOffset & (~0x03));
		iDestBase = (INT32)(pDestDC->blk[3].uiOffset & 0x03) << 3;
		switch (pDestDC->uiType) {
		case TYPE_FB:
		case TYPE_BITMAP:
			iDestPitch = (pDestDC->blk[3].uiPitch) << 3;
			break;
		case TYPE_ICON:
			iDestPitch = (pDestDC->blk[3].uiWidth) << DestSHIFT;
			break;
		}
		iDestOffset = iDestPitch * DestRect.y + ((DestRect.x) << DestSHIFT);

		for (j = 0; j < h; j++) {
			int load = 0;
			int dirty = 0;

			register UINT32 s_dword, d_dword;
			register UINT32 *pDestAddrDW;
			register UINT32 *pSrcAddrDW;
			register INT32 di, si;
			INT32 iSrcBits;
			INT32 iDestBits;
			register smask = SrcMASK;
			register dmask = DestMASK;
			register INT32 DestBPP = 1 << DestSHIFT;
			register INT32 SrcBPP = 1 << SrcSHIFT;

			//1.read src_buf from src line data
			iSrcBits = (iSrcBase + iSrcOffset);
			pSrcAddrDW = pSrcBaseDW + (iSrcBits >> 5);
			si = iSrcBits & 0x01f; //pixel shift bits in dword
			s_dword = (*pSrcAddrDW);

			//2.read dst_dword from dst line data
			iDestBits = (iDestBase + iDestOffset);
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
#if USE_BIT_REVERSE
				register INT32 si2;

				//3.get src pixel data from src_buf
				si2 = (si & ~0x07) | (8 - SrcBPP - (si & 0x07)); //reverse bit order in one byte
				sm = (smask << si2);
				sv = (s_dword & sm) >> si2;
#else
				//3.get src pixel data from src_buf
				sm = (smask << si);
				sv = (s_dword & sm) >> si;
#endif

				//3.get dst pixel data from dst_buf
				//dm = (dmask << di);
				//dv = (d_dword & dm) >> di;

				//4.do ROP with src and dst pixel data, and put to result pixel data
				rv = LocalTable[sv]; //mapping font index to user custom color

				if (rv != COLOR_TRANSPARENT) {
					if (!load) {
						d_dword = (*pDestAddrDW);
						load = 1;
					}
					//5.put result pixel data to dst_dword
					dm = (dmask << di);
					d_dword = (d_dword & ~dm) | ((rv << di) & dm);
					dirty = 1;
				}

				si += SrcBPP;
				if (si == 32) {
					//read src_dword from src_ptr
					si = 0;
					pSrcAddrDW++;
					s_dword = (*pSrcAddrDW);
				}
				di += DestBPP;
				if (di == 32) {
					//6.write dst_dword to dst_ptr
					if (dirty) {
						(*pDestAddrDW) = d_dword;
					}

					//read dst_dword from dst_ptr
					di = 0;
					pDestAddrDW++;

					//d_dword = (*pDestAddrDW);  //add this line if current ROP need read DEST value
					load = 0;
					dirty = 0;
				}
			}

			if (di) {
				//force flush dst_dword at end of line
				if (dirty) {
					(*pDestAddrDW) = d_dword;
				}
			}

			//next src line
			iSrcOffset += iSrcPitch;

			//next dst line
			iDestOffset += iDestPitch;
		}
		return GX_OK;
	}

#else //slow path
	{
		PIXEL_2D DestPx = {0};
		PIXEL_2D SrcPx = {0};
		LVALUE i, j;
		MAPPING_ITEM BackColor;
		MAPPING_ITEM FaceColor;
		MAPPING_ITEM ShadowColor;
		MAPPING_ITEM LineColor;
		BackColor = pTable[0];
		FaceColor = pTable[1];
		ShadowColor = pTable[2];
		LineColor = pTable[3];

		PIXEL_2D_Init(&DestPx, _pDestDC);
		PIXEL_2D_Init(&SrcPx, (DC *)(UINT32)_pSrcDC);
		PIXEL_2D_Check(&DestPx);
		PIXEL_2D_Check(&SrcPx);
		PIXEL_2D_Begin(&DestPx, &DestRect);
		PIXEL_2D_Begin(&SrcPx, &SrcRect);
		for (j = 0; j < SrcRect.h; j++) {
			PIXEL_2D_BeginLine(&DestPx);
			PIXEL_2D_BeginLine(&SrcPx);
			for (i = 0; i < SrcRect.w; i++) {
				//UINT32 SrcValue, DestValue;
				UINT32 SrcValue;
				MAPPING_ITEM DestValue = 0;

				PIXEL_2D_BeginPixel(&DestPx);
				PIXEL_2D_BeginPixel(&SrcPx);
				SrcValue = PIXEL_2D_Read(&SrcPx);
				//================================
				if (SrcValue == COLOR_INDEX_FONTBACK) {
					DestValue = BackColor;
				} else if (SrcValue == COLOR_INDEX_FONTFACE) { //face color
					DestValue = FaceColor;
				} else if (SrcValue == COLOR_INDEX_FONTSHADOW) { //shadow color
					//DestValue = PIXEL_2D_Read(&DestPx);
					//if((DestValue != ShadowColor) && (DestValue != FaceColor))
					DestValue = ShadowColor;
				} else if (SrcValue == COLOR_INDEX_FONTLINE) { //line color
					//DestValue = PIXEL_2D_Read(&DestPx);
					//if((DestValue != LineColor) && (DestValue != FaceColor))
					DestValue = LineColor;
				}

				if (DestValue != (COLOR_TRANSPARENT&0x00000fff)) {
					PIXEL_2D_Write(&DestPx, (UINT32)DestValue);
				}

				PIXEL_2D_EndPixel(&DestPx, FALSE, TRUE);
				PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE);
			}
			PIXEL_2D_EndLine(&DestPx);
			PIXEL_2D_EndLine(&SrcPx);
		}
		PIXEL_2D_End(&DestPx);
		PIXEL_2D_End(&SrcPx);
	}
#endif //path

	return GX_OK;
}

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_StretchBlt_ARGB4444(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							   DC_CTRL *pc)
{

#if (FORCE_SW_4444 == 1) //some project only use SW without any HW

	return _DC_StretchBlt_ARGB4444_sw(_pDestDC, pDestRect, _pSrcDC, pSrcRect, pc);

#else

	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;

	IRECT DestRect;
	IRECT SrcRect;
	RESULT ERcode;

	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_RGBA4444_PK :
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pDestDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}
	switch (pSrcDC->uiPxlfmt) {
	case PXLFMT_RGBA4444_PK :
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pSrcDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}

	RECT_SetRect(&DestRect, pDestRect);
	RECT_SetRect(&SrcRect, pSrcRect);
	//check rect zero
	if (_DC_IS_ZERO_INDEX(pSrcDC, &SrcRect)) {
		return GX_OK;    //skip this draw
	}
	if (_DC_IS_ZERO_INDEX(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}

	{
		UINT32 filter = pc->uiRop & FILTER_MASK;
		UINT32  sclMethod = HD_GFX_SCALE_QUALITY_NULL;

		if (pSrcDC->uiPxlfmt != pDestDC->uiPxlfmt) {
			return GX_ERROR_FORMAT;    //not support format
		}

		if ((filter == FILTER_NONE) || (filter == FILTER_PIXEL)) {
			sclMethod = HD_GFX_SCALE_QUALITY_NEAREST;
		}
		if ((filter == FILTER_LINEAR) || (filter == FILTER_CUBIC)) {
			sclMethod = HD_GFX_SCALE_QUALITY_BILINEAR;
		}

		//fmt: 3=INDEX, 1=YUVPACK, 2=YUVPlaner, 0=ARGB
		_ime_setImage1(_pSrcDC, 3, &SrcRect);
		_ime_setImage2(_pDestDC, 3, &DestRect);


        {
        VF_GFX_SCALE param={0};
    	//use gfx engine to mirror the image
    	memset(&param, 0, sizeof(VF_GFX_SCALE));
    	ERcode = vf_init(&param.src_img, _DC_SclSrcWidth, _DC_SclSrcHeight, HD_VIDEO_PXLFMT_ARGB4444, _DC_SclSrcPitch, vos_cpu_get_phy_addr(_DC_SclSrcOffsetY), _DC_SclSrcPitch*_DC_SclSrcHeight);
    	if(ERcode){
    		DBG_ERR("fail to init source image\n");
    		return ERcode;
    	}
    	ERcode = vf_init(&param.dst_img, _DC_SclDestWidth, _DC_SclDestHeight, HD_VIDEO_PXLFMT_ARGB4444, _DC_SclDestPitch, vos_cpu_get_phy_addr(_DC_SclDestOffsetY), _DC_SclDestPitch*_DC_SclDestHeight);
    	if(ERcode){
    		DBG_ERR("fail to init dest image\n");
    		return ERcode;
    	}

    	param.src_region.x  = 0;
    	param.src_region.y  = 0;
    	param.src_region.w  = SrcRect.w;
    	param.src_region.h  = SrcRect.h;
    	param.dst_region.x  = 0;
    	param.dst_region.y  = 0;
    	param.dst_region.w  = DestRect.w;
    	param.dst_region.h  = DestRect.h;
    	param.quality       = sclMethod;

    	ERcode = vf_gfx_scale(&param, 1);
    	}
    }

	return ERcode;
#endif
}

RESULT _DC_StretchBlt_ARGB4444_sw
(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
 DC_CTRL *pc)
{
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;

//  IRECT SrcPlaneRect;
//  IRECT DestPlaneRect;

//    LVALUE i,j;
	UINT16 n;
	UINT32 filter = pc->uiRop & FILTER_MASK;

	IRECT DestRect;
	IRECT SrcRect;

	PIXEL_2D DestPx = {0};
	PIXEL_2D SrcPx = {0};
	UINT32 rop = pc->uiRop & ROP_MASK;
	UINT32 colorkey;
	int sw, sh, dw, dh;

	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_RGBA4444_PK :
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pDestDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}
	switch (pSrcDC->uiPxlfmt) {
	case PXLFMT_RGBA4444_PK :
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pSrcDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}

	colorkey = 0;
	switch (rop) {
	case ROP_COPY:
		break;
	case ROP_KEY:
	case ROP_DESTFILL:
		colorkey = (UINT32)pc->uiParam;
		break;
	default:
		return GX_NOTSUPPORT_ROP;
	}

	RECT_SetRect(&DestRect, pDestRect);
	RECT_SetRect(&SrcRect, pSrcRect);

	//check rect zero
	if (_DC_IS_ZERO_INDEX(pSrcDC, &SrcRect)) {
		return GX_OK;    //skip this draw
	}
	if (_DC_IS_ZERO_INDEX(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}

	sw = SrcRect.w;
	sh = SrcRect.h;
	dw = DestRect.w;
	dh = DestRect.h;

#if 1
	if (!filter)
#endif
	{
		//stretch rect with Bresenham algorithm

		int yNumPixels = dh;
		//int yIntPart = (sh / dh) * sw;
		int yIntPart = sh / dh;
		int yFractPart = sh % dh;
		int yE = 0;
		int yJump = 0;

		int _xIntPart = sw / dw; //pre-calculate
		int _xFractPart = sw % dw; //pre-calculate

		PIXEL_2D_Init(&DestPx, _pDestDC);
		PIXEL_2D_Init(&SrcPx, (DC *)(UINT32)_pSrcDC);
		PIXEL_2D_Check(&DestPx);
		PIXEL_2D_Check(&SrcPx);
		PIXEL_2D_Begin(&DestPx, &DestRect);
		PIXEL_2D_Begin(&SrcPx, &SrcRect);
		while (yNumPixels-- > 0) {
			int xNumPixels = dw;
			int xIntPart = _xIntPart;
			int xFractPart = _xFractPart;
			int xE = 0;
			int xJump = 0;
			PIXEL_2D_BeginLine(&DestPx);
			PIXEL_2D_BeginLine(&SrcPx);
			while (xNumPixels-- > 0) {
				UINT32 SrcValue, DestValue;
				BOOL bWrite = FALSE;
				PIXEL_2D_BeginPixel(&DestPx);
				PIXEL_2D_BeginPixel(&SrcPx);
				SrcValue = PIXEL_2D_Read(&SrcPx);
				DestValue = PIXEL_2D_Read(&DestPx);
				switch (rop) {
				case ROP_COPY:
					DestValue = SrcValue; //copy
					bWrite = TRUE;
					break;
				case ROP_KEY:
					if (SrcValue == colorkey) {
						//do color key
					} else {
						DestValue = SrcValue; //copy
						bWrite = TRUE;
					}
					break;
				case ROP_DESTFILL:
					DestValue = colorkey; //set
					bWrite = TRUE;
					break;
				}
				if (bWrite) {
					PIXEL_2D_Write(&DestPx, DestValue);
				}
				PIXEL_2D_EndPixel(&DestPx, FALSE, TRUE); //next pixel

//            Source += xIntPart; //jump to offset
				xJump = xIntPart; //jump to offset

				xE += xFractPart;
				if (xE >= dw) {
					xE -= dw;
					xJump++;
				}

				if (xJump == 0) {
					// stay on current pixel
				} else if (xJump == 1) {
					PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next pixel
				} else { // >= 1
					PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next pixel
					for (n = 0; n < xJump - 1; n++) {
						//PIXEL_2D_BeginPixel(&SrcPx); //next pixel
						PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next pixel
					}
				}
			}
			PIXEL_2D_EndLine(&DestPx); //next line

//        Source += yIntPart; //jump to offset
			yJump = yIntPart; //jump to offset

			yE += yFractPart;
			if (yE >= dh) {
				yE -= dh;
				yJump++;
			}

			if (yJump == 0) {
				// stay on current line
			} else if (yJump == 1) {
				PIXEL_2D_EndLine(&SrcPx); //next line
			} else { // >= 1
				PIXEL_2D_EndLine(&SrcPx); //next line
				for (n = 0; n < yJump - 1; n++) {
					//PIXEL_2D_BeginLine(&SrcPx); //next line
					PIXEL_2D_EndLine(&SrcPx); //next line
				}
			}
		}
		PIXEL_2D_End(&DestPx);
		PIXEL_2D_End(&SrcPx);
	}
#if 1
	else { //filter (for wrap text) ... VERY SLOW
		//stretch rect with Bresenham algorithm

		int yNumPixels = dh;
		//int yIntPart = (sh / dh) * sw;
		int yIntPart = sh / dh;
		int yFractPart = sh % dh;
		int yE = 0;
		int yJump = 0;

		int _xIntPart = sw / dw; //pre-calculate
		int _xFractPart = sw % dw; //pre-calculate

		PIXEL_2D_Init(&DestPx, _pDestDC);
		PIXEL_2D_Init(&SrcPx, (DC *)(UINT32)_pSrcDC);
		PIXEL_2D_Check(&DestPx);
		PIXEL_2D_Check(&SrcPx);
		PIXEL_2D_Begin(&DestPx, &DestRect);
		PIXEL_2D_Begin(&SrcPx, &SrcRect);
		while (yNumPixels-- > 0) {
			int xNumPixels = dw;
			int xIntPart = _xIntPart;
			int xFractPart = _xFractPart;
			int xE = 0;
			int xJump = 0;
			BOOL bLoad = 0;
			UINT32 SrcValue, DestValue;
			PIXEL_2D_BeginLine(&DestPx);
			PIXEL_2D_BeginLine(&SrcPx);
			SrcValue = 0;

			while (xNumPixels-- > 0) {
				UINT32 AccValue = 0;
				UINT32 AccValue_A = 0;
				UINT32 AccValue_R = 0;
				UINT32 AccValue_G = 0;
				UINT32 AccValue_B = 0;
				UINT32 AccCnt = 0;
				PIXEL_2D_BeginPixel(&DestPx);
				DestValue = PIXEL_2D_Read(&DestPx);

				//frac part
				if (!bLoad) {
					PIXEL_2D_BeginPixel(&SrcPx);
					SrcValue = PIXEL_2D_Read(&SrcPx);
					switch (rop) {
					case ROP_COPY:
						break;
					case ROP_KEY:
						if (SrcValue == colorkey) {
							//do color key
							SrcValue = 0x00000000; //use empty color
						}
						break;
					}
					PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next src pixel
					bLoad = 1;
				}
				AccValue_A += COLOR_RGBA_GET_A(SrcValue) * (dw - xE);
				AccValue_R += COLOR_RGBA_GET_R(SrcValue) * (dw - xE);
				AccValue_G += COLOR_RGBA_GET_G(SrcValue) * (dw - xE);
				AccValue_B += COLOR_RGBA_GET_B(SrcValue) * (dw - xE);
				AccCnt += (dw - xE);

				//int part
				xJump = xIntPart; //jump to offset
				if (xJump > 0) { //scale down
					for (n = 0; n < xJump - 1; n++) {
						PIXEL_2D_BeginPixel(&SrcPx);
						SrcValue = PIXEL_2D_Read(&SrcPx);
						switch (rop) {
						case ROP_COPY:
							break;
						case ROP_KEY:
							if (SrcValue == colorkey) {
								//do color key
								SrcValue = 0x00000000; //use empty color
							}
							break;
						}
						PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next src pixel

						AccValue_A += COLOR_RGBA_GET_A(SrcValue) * dw;
						AccValue_R += COLOR_RGBA_GET_R(SrcValue) * dw;
						AccValue_G += COLOR_RGBA_GET_G(SrcValue) * dw;
						AccValue_B += COLOR_RGBA_GET_B(SrcValue) * dw;
						AccCnt += dw;
					}
				} else { //scale up
				}


				//frac part
				xJump = 0; //jump to offset
				{
					xE += xFractPart;
					if (xE >= dw) {
						if (dw < sw) {
							PIXEL_2D_BeginPixel(&SrcPx);
							SrcValue = PIXEL_2D_Read(&SrcPx);
							switch (rop) {
							case ROP_COPY:
								break;
							case ROP_KEY:
								if (SrcValue == colorkey) {
									//do color key
									SrcValue = 0x00000000; //use empty color
								}
								break;
							}
							PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next src pixel

							AccValue_A += COLOR_RGBA_GET_A(SrcValue) * dw;
							AccValue_R += COLOR_RGBA_GET_R(SrcValue) * dw;
							AccValue_G += COLOR_RGBA_GET_G(SrcValue) * dw;
							AccValue_B += COLOR_RGBA_GET_B(SrcValue) * dw;
							AccCnt += dw;
						}

						xE -= dw;
						xJump++;
					} else {
						// stay on current src pixel
					}
				}

				if (xE > 0) {
					PIXEL_2D_BeginPixel(&SrcPx);
					SrcValue = PIXEL_2D_Read(&SrcPx);
					switch (rop) {
					case ROP_COPY:
						break;
					case ROP_KEY:
						if (SrcValue == colorkey) {
							//do color key
							SrcValue = 0x00000000; //use empty color
						}
						break;
					}
					PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next src pixel
					bLoad = 1;

					AccValue_A += COLOR_RGBA_GET_A(SrcValue) * xE;
					AccValue_R += COLOR_RGBA_GET_R(SrcValue) * xE;
					AccValue_G += COLOR_RGBA_GET_G(SrcValue) * xE;
					AccValue_B += COLOR_RGBA_GET_B(SrcValue) * xE;
					AccCnt += xE;
				} else {
					bLoad = 0;
				}

				//copy
				if ((AccValue_A / AccCnt) > 0) {
					UINT32 src_a = AccValue_A / AccCnt;
					UINT32 src_r = AccValue_R / AccCnt;
					UINT32 src_g = AccValue_G / AccCnt;
					UINT32 src_b = AccValue_B / AccCnt;
					UINT32 dst_a = COLOR_RGBA_GET_A(DestValue);
					UINT32 dst_r = COLOR_RGBA_GET_R(DestValue);
					UINT32 dst_g = COLOR_RGBA_GET_G(DestValue);
					UINT32 dst_b = COLOR_RGBA_GET_B(DestValue);
					if (dst_a == 0) {
						dst_a = src_a;
						dst_r = src_a * src_r / 255;
						dst_g = src_a * src_g / 255;
						dst_b = src_a * src_b / 255;
					} else if (dst_a == 255) {
						//dst_a = dst_a;
						dst_r = ((255 - src_a) * dst_r + src_a * src_r) / 255;
						dst_g = ((255 - src_a) * dst_g + src_a * src_g) / 255;
						dst_b = ((255 - src_a) * dst_b + src_a * src_b) / 255;
					} else {
						{
							UINT32 new_a;

							//new_a = dst_a + src_a*(src_a-dst_a)/255;
							new_a = dst_a + src_a * (255 - dst_a) / 255;

							dst_r = ((255 - src_a) * dst_r + src_a * src_r) / 255;
							dst_g = ((255 - src_a) * dst_g + src_a * src_g) / 255;
							dst_b = ((255 - src_a) * dst_b + src_a * src_b) / 255;
							/*
							dst_r = dst_r*dst_a;
							dst_g = dst_g*dst_a;
							dst_b = dst_b*dst_a;

							//dst_r = ((255-new_a)*dst_r + new_a*src_r)/255;
							//dst_g = ((255-new_a)*dst_g + new_a*src_g)/255;
							//dst_b = ((255-new_a)*dst_b + new_a*src_b)/255;

							dst_r += src_a*src_r;
							dst_g += src_a*src_g;
							dst_b += src_a*src_b;

							dst_r /= new_a;
							dst_g /= new_a;
							dst_b /= new_a;
							*/

							dst_a = new_a;
						}
					}
					dst_a = RANGE_Clamp(0, 255, (INT32)dst_a);
					dst_r = RANGE_Clamp(0, 255, (INT32)dst_r);
					dst_g = RANGE_Clamp(0, 255, (INT32)dst_g);
					dst_b = RANGE_Clamp(0, 255, (INT32)dst_b);
					AccValue = COLOR_MAKE_ARGB4444(
								   dst_r,
								   dst_g,
								   dst_b,
								   dst_a);
					if (AccValue != DestValue) {
						PIXEL_2D_Write(&DestPx, AccValue);
					}
				}
#if 0
				if ((AccValue_A / AccCnt) > 0) {
					UINT32 src_a = AccValue_A / AccCnt;
					UINT32 src_r = AccValue_R / AccCnt;
					UINT32 src_g = AccValue_G / AccCnt;
					UINT32 src_b = AccValue_B / AccCnt;
					UINT32 dst_a = COLOR_RGB444_GET_A(DestValue);
					UINT32 dst_r = COLOR_RGB444_GET_R(DestValue) * dst_a / 255;
					UINT32 dst_g = COLOR_RGB444_GET_G(DestValue) * dst_a / 255;
					UINT32 dst_b = COLOR_RGB444_GET_B(DestValue) * dst_a / 255;
					/*if(dst_a == 0)
					{
					dst_a = src_a;
					dst_r = src_a*src_r/255;
					dst_g = src_a*src_g/255;
					dst_b = src_a*src_b/255;
					}*/
					dst_a = 255 - ((255 - dst_a) * (255 - src_a) / 255);
					dst_r = ((255 - src_a) * dst_r + src_a * src_r) / 255;
					dst_g = ((255 - src_a) * dst_g + src_a * src_g) / 255;
					dst_b = ((255 - src_a) * dst_b + src_a * src_b) / 255;
					dst_a = RANGE_Clamp(0, 255, dst_a);
					dst_r = RANGE_Clamp(0, 255, dst_r);
					dst_g = RANGE_Clamp(0, 255, dst_g);
					dst_b = RANGE_Clamp(0, 255, dst_b);
					AccValue = COLOR_MAKE_RGB444(
								   dst_r,
								   dst_g,
								   dst_b,
								   dst_a);
					if (AccValue != DestValue) {
						PIXEL_2D_Write(&DestPx, AccValue);
					}
				}
#endif
				/*
				AccValue = COLOR_MAKE_RGB444(
				    AccValue_R/AccCnt,
				    AccValue_G/AccCnt,
				    AccValue_B/AccCnt,
				    AccValue_A/AccCnt);
				if(AccValue != DestValue)
				{
				    PIXEL_2D_Write(&DestPx, AccValue);
				}
				*/
				PIXEL_2D_EndPixel(&DestPx, FALSE, TRUE); //next dest pixel
			}
			PIXEL_2D_EndLine(&DestPx); //next line

//        Source += yIntPart; //jump to offset
			yJump = yIntPart; //jump to offset

			yE += yFractPart;
			if (yE >= dh) {
				yE -= dh;
				yJump++;
			}

			if (yJump == 0) {
				// stay on current line
			} else if (yJump == 1) {
				PIXEL_2D_EndLine(&SrcPx); //next line
			} else { // >= 1
				PIXEL_2D_EndLine(&SrcPx); //next line
				for (n = 0; n < yJump - 1; n++) {
					//PIXEL_2D_BeginLine(&SrcPx); //next line
					PIXEL_2D_EndLine(&SrcPx); //next line
				}
			}
		}
		PIXEL_2D_End(&DestPx);
		PIXEL_2D_End(&SrcPx);
	}
#endif
	return GX_OK;
}
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_StretchBlt_INDEX2RGBA4444_sw
(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
 DC_CTRL *pc)
{
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;

	UINT16 n;
	UINT32 filter = pc->uiRop & FILTER_MASK;

	IRECT DestRect;
	IRECT SrcRect;
	const PALETTE_ITEM *pColorTable = pc->pColorTable;
	const MAPPING_ITEM *pIndexTable = pc->pIndexTable;

	PIXEL_2D DestPx = {0};
	PIXEL_2D SrcPx = {0};
	UINT32 rop = pc->uiRop & ROP_MASK;
	UINT32 colorkey;


	int sw, sh, dw, dh;


	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_RGBA4444_PK :
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pDestDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}
	switch (pSrcDC->uiPxlfmt) {
	case PXLFMT_INDEX1 :
		break;
	case PXLFMT_INDEX2 :
		break;
	case PXLFMT_INDEX4 :
		break;
	case PXLFMT_INDEX8 :
		break;
	}

	colorkey = 0;
	switch (rop) {
	case ROP_COPY:
		break;
	case ROP_KEY:
	case ROP_DESTFILL:
		colorkey = (UINT32)pc->uiParam;
		break;
	default:
		return GX_NOTSUPPORT_ROP;
	}

	RECT_SetRect(&DestRect, pDestRect);
	RECT_SetRect(&SrcRect, pSrcRect);

	//check rect zero
	if (_DC_IS_ZERO_INDEX(pSrcDC, &SrcRect)) {
		return GX_OK;    //skip this draw
	}
	if (_DC_IS_ZERO_INDEX(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}


	sw = SrcRect.w;
	sh = SrcRect.h;
	dw = DestRect.w;
	dh = DestRect.h;

#if 1
	if (!filter)
#endif
	{
		//stretch rect with Bresenham algorithm

		int yNumPixels = dh;
		//int yIntPart = (sh / dh) * sw;
		int yIntPart = sh / dh;
		int yFractPart = sh % dh;
		int yE = 0;
		int yJump = 0;

		int _xIntPart = sw / dw; //pre-calculate
		int _xFractPart = sw % dw; //pre-calculate

		PIXEL_2D_Init(&DestPx, _pDestDC);
		PIXEL_2D_Init(&SrcPx, (DC *)(UINT32)_pSrcDC);
		PIXEL_2D_Check(&DestPx);
		PIXEL_2D_Check(&SrcPx);
		PIXEL_2D_Begin(&DestPx, &DestRect);
		PIXEL_2D_Begin(&SrcPx, &SrcRect);
		while (yNumPixels-- > 0) {
			int xNumPixels = dw;
			int xIntPart = _xIntPart;
			int xFractPart = _xFractPart;
			int xE = 0;
			int xJump = 0;
			PIXEL_2D_BeginLine(&DestPx);
			PIXEL_2D_BeginLine(&SrcPx);
			while (xNumPixels-- > 0) {
				UINT32 SrcValue, DestValue;
				BOOL bWrite = FALSE;
				PIXEL_2D_BeginPixel(&DestPx);
				PIXEL_2D_BeginPixel(&SrcPx);
				SrcValue = PIXEL_2D_Read(&SrcPx);
				DestValue = PIXEL_2D_Read(&DestPx);
				switch (rop) {
				case ROP_COPY:
					DestValue = SrcValue; //copy
					if (pIndexTable) {
						DestValue = pIndexTable[DestValue];    //replace
					}
					if (pColorTable) {
						DestValue = pColorTable[DestValue];    //replace
					} else {
						DestValue = 0x00000fff;
					}
					bWrite = TRUE;
					break;
				case ROP_KEY:
					if (pIndexTable) {
						SrcValue = pIndexTable[SrcValue];    //replace
					}
					if (pColorTable) {
						SrcValue = pColorTable[SrcValue];    //replace
					} else {
						SrcValue = 0x00000fff;
					}

					if (SrcValue == colorkey) {
						//do color key
					} else {
						DestValue = SrcValue; //copy
						bWrite = TRUE;
					}
					break;
				case ROP_DESTFILL:
					SrcValue = colorkey;
					if (pIndexTable) {
						SrcValue = pIndexTable[SrcValue];    //replace
					}
					if (pColorTable) {
						SrcValue = pColorTable[SrcValue];    //replace
					} else {
						SrcValue = 0x00000fff;
					}
					DestValue = SrcValue; //set
					bWrite = TRUE;
					break;
				}
				if (bWrite) {
					PIXEL_2D_Write(&DestPx, DestValue);
				}
				PIXEL_2D_EndPixel(&DestPx, FALSE, TRUE); //next pixel

//            Source += xIntPart; //jump to offset
				xJump = xIntPart; //jump to offset

				xE += xFractPart;
				if (xE >= dw) {
					xE -= dw;
					xJump++;
				}

				if (xJump == 0) {
					// stay on current pixel
				} else if (xJump == 1) {
					PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next pixel
				} else { // >= 1
					PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next pixel
					for (n = 0; n < xJump - 1; n++) {
						//PIXEL_2D_BeginPixel(&SrcPx); //next pixel
						PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next pixel
					}
				}
			}
			PIXEL_2D_EndLine(&DestPx); //next line

//        Source += yIntPart; //jump to offset
			yJump = yIntPart; //jump to offset

			yE += yFractPart;
			if (yE >= dh) {
				yE -= dh;
				yJump++;
			}

			if (yJump == 0) {
				// stay on current line
			} else if (yJump == 1) {
				PIXEL_2D_EndLine(&SrcPx); //next line
			} else { // >= 1
				PIXEL_2D_EndLine(&SrcPx); //next line
				for (n = 0; n < yJump - 1; n++) {
					//PIXEL_2D_BeginLine(&SrcPx); //next line
					PIXEL_2D_EndLine(&SrcPx); //next line
				}
			}
		}
		PIXEL_2D_End(&DestPx);
		PIXEL_2D_End(&SrcPx);
	}
#if 1
	else { //filter (for wrap text) ... VERY SLOW
		//stretch rect with Bresenham algorithm

		int yNumPixels = dh;
		//int yIntPart = (sh / dh) * sw;
		int yIntPart = sh / dh;
		int yFractPart = sh % dh;
		int yE = 0;
		int yJump = 0;

		int _xIntPart = sw / dw; //pre-calculate
		int _xFractPart = sw % dw; //pre-calculate

		PIXEL_2D_Init(&DestPx, _pDestDC);
		PIXEL_2D_Init(&SrcPx, (DC *)(UINT32)_pSrcDC);
		PIXEL_2D_Check(&DestPx);
		PIXEL_2D_Check(&SrcPx);
		PIXEL_2D_Begin(&DestPx, &DestRect);
		PIXEL_2D_Begin(&SrcPx, &SrcRect);
		while (yNumPixels-- > 0) {
			int xNumPixels = dw;
			int xIntPart = _xIntPart;
			int xFractPart = _xFractPart;
			int xE = 0;
			int xJump = 0;
			BOOL bLoad = 0;
			UINT32 SrcValue, DestValue;
			PIXEL_2D_BeginLine(&DestPx);
			PIXEL_2D_BeginLine(&SrcPx);
			SrcValue = 0;

			while (xNumPixels-- > 0) {
				UINT32 AccValue = 0;
				UINT32 AccValue_A = 0;
				UINT32 AccValue_R = 0;
				UINT32 AccValue_G = 0;
				UINT32 AccValue_B = 0;
				UINT32 AccCnt = 0;
				PIXEL_2D_BeginPixel(&DestPx);
				DestValue = PIXEL_2D_Read(&DestPx);

				//frac part
				if (!bLoad) {
					PIXEL_2D_BeginPixel(&SrcPx);
					SrcValue = PIXEL_2D_Read(&SrcPx);
					switch (rop) {
					case ROP_COPY:
						if (pIndexTable) {
							SrcValue = pIndexTable[SrcValue];    //replace
						}
						if (pColorTable) {
							SrcValue = pColorTable[SrcValue];    //replace
						} else {
							SrcValue = 0x00ffffff;
						}
						break;
					case ROP_KEY:
						if (pIndexTable) {
							SrcValue = pIndexTable[SrcValue];    //replace
						}
						if (pColorTable) {
							SrcValue = pColorTable[SrcValue];    //replace
						} else {
							SrcValue = 0x00ffffff;
						}
						if (SrcValue == colorkey) {
							//do color key
							SrcValue = DestValue; //use BK color
						}
						break;
					}
					PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next src pixel
					bLoad = 1;
				}
				AccValue_A += COLOR_RGBA_GET_A(SrcValue) * (dw - xE);
				AccValue_R += COLOR_RGBA_GET_R(SrcValue) * (dw - xE);
				AccValue_G += COLOR_RGBA_GET_G(SrcValue) * (dw - xE);
				AccValue_B += COLOR_RGBA_GET_B(SrcValue) * (dw - xE);
				AccCnt += (dw - xE);

				//int part
				xJump = xIntPart; //jump to offset
				if (xJump > 0) { //scale down
					for (n = 0; n < xJump - 1; n++) {
						PIXEL_2D_BeginPixel(&SrcPx);
						SrcValue = PIXEL_2D_Read(&SrcPx);
						switch (rop) {
						case ROP_COPY:
							if (pIndexTable) {
								SrcValue = pIndexTable[SrcValue];    //replace
							}
							if (pColorTable) {
								SrcValue = pColorTable[SrcValue];    //replace
							} else {
								SrcValue = 0x00ffffff;
							}
							break;
						case ROP_KEY:
							if (pIndexTable) {
								SrcValue = pIndexTable[SrcValue];    //replace
							}
							if (pColorTable) {
								SrcValue = pColorTable[SrcValue];    //replace
							} else {
								SrcValue = 0x00ffffff;
							}
							if (SrcValue == colorkey) {
								//do color key
								SrcValue = DestValue; //use BK color
							}
							break;
						}
						PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next src pixel

						AccValue_A += COLOR_RGBA_GET_A(SrcValue) * dw;
						AccValue_R += COLOR_RGBA_GET_R(SrcValue) * dw;
						AccValue_G += COLOR_RGBA_GET_G(SrcValue) * dw;
						AccValue_B += COLOR_RGBA_GET_B(SrcValue) * dw;
						AccCnt += dw;
					}
				} else { //scale up
				}


				//frac part
				xJump = 0; //jump to offset
				{
					xE += xFractPart;
					if (xE >= dw) {
						if (dw < sw) {
							PIXEL_2D_BeginPixel(&SrcPx);
							SrcValue = PIXEL_2D_Read(&SrcPx);
							switch (rop) {
							case ROP_COPY:
								if (pIndexTable) {
									SrcValue = pIndexTable[SrcValue];    //replace
								}
								if (pColorTable) {
									SrcValue = pColorTable[SrcValue];    //replace
								} else {
									SrcValue = 0x00ffffff;
								}
								break;
							case ROP_KEY:
								if (pIndexTable) {
									SrcValue = pIndexTable[SrcValue];    //replace
								}
								if (pColorTable) {
									SrcValue = pColorTable[SrcValue];    //replace
								} else {
									SrcValue = 0x00ffffff;
								}
								if (SrcValue == colorkey) {
									//do color key
									SrcValue = DestValue; //use BK color
								}
								break;
							}
							PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next src pixel

							AccValue_A += COLOR_RGBA_GET_A(SrcValue) * dw;
							AccValue_R += COLOR_RGBA_GET_R(SrcValue) * dw;
							AccValue_G += COLOR_RGBA_GET_G(SrcValue) * dw;
							AccValue_B += COLOR_RGBA_GET_B(SrcValue) * dw;
							AccCnt += dw;
						}

						xE -= dw;
						xJump++;
					} else {
						// stay on current src pixel
					}
				}

				if (xE > 0) {
					PIXEL_2D_BeginPixel(&SrcPx);
					SrcValue = PIXEL_2D_Read(&SrcPx);
					switch (rop) {
					case ROP_COPY:
					case ROP_KEY:
						if (pIndexTable) {
							SrcValue = pIndexTable[SrcValue];    //replace
						}
						if (pColorTable) {
							SrcValue = pColorTable[SrcValue];    //replace
						} else {
							SrcValue = 0x00ffffff;
						}
						if (SrcValue == colorkey) {
							//do color key
							SrcValue = DestValue; //use BK color
						}
						break;
					}
					PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next src pixel
					bLoad = 1;

					AccValue_A += COLOR_RGBA_GET_A(SrcValue) * xE;
					AccValue_R += COLOR_RGBA_GET_R(SrcValue) * xE;
					AccValue_G += COLOR_RGBA_GET_G(SrcValue) * xE;
					AccValue_B += COLOR_RGBA_GET_B(SrcValue) * xE;
					AccCnt += xE;
				} else {
					bLoad = 0;
				}

				//copy
				AccValue = COLOR_MAKE_ARGB4444(
							   AccValue_R / AccCnt,
							   AccValue_G / AccCnt,
							   AccValue_B / AccCnt,
							   AccValue_A / AccCnt);
				if (AccValue != DestValue) {
					PIXEL_2D_Write(&DestPx, AccValue);
				}
				PIXEL_2D_EndPixel(&DestPx, FALSE, TRUE); //next dest pixel
			}
			PIXEL_2D_EndLine(&DestPx); //next line

//        Source += yIntPart; //jump to offset
			yJump = yIntPart; //jump to offset

			yE += yFractPart;
			if (yE >= dh) {
				yE -= dh;
				yJump++;
			}

			if (yJump == 0) {
				// stay on current line
			} else if (yJump == 1) {
				PIXEL_2D_EndLine(&SrcPx); //next line
			} else { // >= 1
				PIXEL_2D_EndLine(&SrcPx); //next line
				for (n = 0; n < yJump - 1; n++) {
					//PIXEL_2D_BeginLine(&SrcPx); //next line
					PIXEL_2D_EndLine(&SrcPx); //next line
				}
			}
		}
		PIXEL_2D_End(&DestPx);
		PIXEL_2D_End(&SrcPx);
	}
#endif
	return GX_OK;
}



//--------------------------------------------------------------------------------------
//  PIXEL_2D
//--------------------------------------------------------------------------------------
typedef struct _PIXEL_2D_RGBA4444 {
	void (*Begin)(struct _PIXEL_2D *p, IRECT *pRect);
	void (*BeginLine)(struct _PIXEL_2D *p);
	void (*BeginPixel)(struct _PIXEL_2D *p);
	UINT32(*Read)(struct _PIXEL_2D *p);
	void (*Write)(struct _PIXEL_2D *p, UINT32 value);
	void (*EndPixel)(struct _PIXEL_2D *p, BOOL bForceFlush, BOOL bNext);
	void (*EndLine)(struct _PIXEL_2D *p);
	void (*End)(struct _PIXEL_2D *p);
	void (*WriteOffset)(struct _PIXEL_2D *p, UINT32 value, INT32 dx, INT32 dy);
	void (*GotoXY)(struct _PIXEL_2D *p, int x, int y);
	UINT8 *(*LineBuf)(struct _PIXEL_2D *p, int y);
	UINT8 *(*PlaneBuf)(struct _PIXEL_2D *p, int i);

	DISPLAY_CONTEXT *pDC;
	IRECT *pRect;

	LVALUE x, y;
	INT32 uiOffset;
	INT32 uiPitch;

	DATA_PACK *pLine;
	DATA_PACK *pData;
} PIXEL_2D_RGBA4444;

static void PIXEL_2D_RGBA4444_Begin(PIXEL_2D *_p, IRECT *pRect)
{
	PIXEL_2D_RGBA4444 *p = (PIXEL_2D_RGBA4444 *)_p;

	p->uiPitch = p->pDC->blk[3].uiPitch;

	p->pRect = pRect;

	p->uiOffset =
		p->pRect->y * (p->uiPitch) + ((p->pRect->x) << 1);

	p->x = 0;
	p->y = 0;
}
static void PIXEL_2D_RGBA4444_BeginLine(PIXEL_2D *_p)
{
	PIXEL_2D_RGBA4444 *p = (PIXEL_2D_RGBA4444 *)_p;

	p->pLine = ((DATA_PACK *)(p->pDC->blk[3].uiOffset)) +
			   (p->uiOffset);
	p->x = 0; //fix

}
static void PIXEL_2D_RGBA4444_BeginPixel(PIXEL_2D *_p)
{
	PIXEL_2D_RGBA4444 *p = (PIXEL_2D_RGBA4444 *)_p;

	p->pData = p->pLine + ((p->x) << 1); //get first byte

}
static UINT32 PIXEL_2D_RGBA4444_Read(PIXEL_2D *_p)
{
	PIXEL_2D_RGBA4444 *p = (PIXEL_2D_RGBA4444 *)_p;
	UINT32 pdata = 0;

	pdata |= ((*(UINT8 *)(p->pData)) << 0);
	pdata |= ((*(UINT8 *)(p->pData + 1)) << 8);
	return pdata;
}
static void PIXEL_2D_RGBA4444_Write(PIXEL_2D *_p, UINT32 value)
{
	PIXEL_2D_RGBA4444 *p = (PIXEL_2D_RGBA4444 *)_p;

	*(p->pData) = (DATA_PACK)((value & 0x000000ff) >> 0);
	*(p->pData + 1) = (DATA_PACK)((value & 0x0000ff00) >> 8);
}
static void PIXEL_2D_RGBA4444_EndPixel(PIXEL_2D *_p, BOOL bForceFlush, BOOL bNext)
{
	PIXEL_2D_RGBA4444 *p = (PIXEL_2D_RGBA4444 *)_p;

	if (bNext) {
		p->x++;
	}
}
static void PIXEL_2D_RGBA4444_EndLine(PIXEL_2D *_p)
{
	PIXEL_2D_RGBA4444 *p = (PIXEL_2D_RGBA4444 *)_p;

	p->uiOffset += (p->uiPitch);
	p->x = 0;
	p->y++;
}
static void PIXEL_2D_RGBA4444_End(PIXEL_2D *p)
{
}
static void PIXEL_2D_RGBA4444_WriteOffset(PIXEL_2D *_p, UINT32 value, INT32 dx, INT32 dy)
{
	PIXEL_2D_RGBA4444 *p = (PIXEL_2D_RGBA4444 *)_p;

	INT32 offset = (p->uiPitch) * dy + (dx << 1);
	//NOTE: boundary checking is ignored
	*(p->pData + offset) = value;
}
static void PIXEL_2D_RGBA4444_GotoXY(PIXEL_2D *_p, int x, int y)
{
	PIXEL_2D_RGBA4444 *p = (PIXEL_2D_RGBA4444 *)_p;

	p->uiOffset =
		p->pRect->y * (p->uiPitch) + ((p->pRect->x) << 1);
	p->uiOffset += (p->uiPitch) * y;
	p->pLine = ((DATA_PACK *)(p->pDC->blk[3].uiOffset)) +
			   (p->uiOffset);
	p->x = x;
	p->y = y;
}
static UINT8 *PIXEL_2D_RGBA4444_LineBuf(struct _PIXEL_2D *_p, int y)
{
	PIXEL_2D_RGBA4444 *p = (PIXEL_2D_RGBA4444 *)_p;

	//plane 0, line y
	return ((DATA_PACK *)(p->pDC->blk[3].uiOffset)) +
		   ((y) * (p->uiPitch));
}
static UINT8 *PIXEL_2D_RGBA4444_PlaneBuf(struct _PIXEL_2D *_p, int i)
{
	PIXEL_2D_RGBA4444 *p = (PIXEL_2D_RGBA4444 *)_p;

	//plane i, line 0
	return ((DATA_PACK *)(p->pDC->blk[3].uiOffset));
}

PIXEL_2D _gPixel2D_RGBA4444 = {
	PIXEL_2D_RGBA4444_Begin,
	PIXEL_2D_RGBA4444_BeginLine,
	PIXEL_2D_RGBA4444_BeginPixel,
	PIXEL_2D_RGBA4444_Read,
	PIXEL_2D_RGBA4444_Write,
	PIXEL_2D_RGBA4444_EndPixel,
	PIXEL_2D_RGBA4444_EndLine,
	PIXEL_2D_RGBA4444_End,
	PIXEL_2D_RGBA4444_WriteOffset,
	PIXEL_2D_RGBA4444_GotoXY,
	PIXEL_2D_RGBA4444_LineBuf,
	PIXEL_2D_RGBA4444_PlaneBuf,
};

