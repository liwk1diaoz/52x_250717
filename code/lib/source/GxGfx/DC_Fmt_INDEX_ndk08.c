
#include "GxGfx/GxGfx.h"
#include "DC.h"
#include "DC_Blt.h"
#include "GxGfx_int.h"
#include "GxPixel.h"
#include "GxGrph.h"

#define FORCE_SW_I8  0
#define FORCE_SW_I8_SCALE 0
#define FORCE_SW_I8_ROT   0
#if !FORCE_SW_I8
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
RESULT _DC_FillRect_INDEX_sw(DC *_pDestDC, const IRECT *pDestRect,
							 UINT32 uiRop, COLOR_ITEM uiColor, const MAPPING_ITEM *pTable)
{
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT DestRect;
	UINT32 DestSHIFT, DestMASK;

	UINT32 rop = uiRop & ROP_MASK;

	DBG_FUNC_BEGIN("\r\n");

	RECT_SetRect(&DestRect, pDestRect);

	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_INDEX1 :
		DestSHIFT = 0;
		DestMASK = 0x00000001;
		break;
	case PXLFMT_INDEX2 :
		DestSHIFT = 1;
		DestMASK = 0x00000003;
		break;
	case PXLFMT_INDEX4 :
#if (GX_SUPPORT_DCFMT_RGB565)
	case PXLFMT_RGBA5654_PK :
#endif
		DestSHIFT = 2;
		DestMASK = 0x0000000f;
		break;
	case PXLFMT_INDEX8 :
#if (GX_SUPPORT_DCFMT_RGB565)
	case PXLFMT_RGBA5658_PK :
#endif
		DestSHIFT = 3;
		DestMASK = 0x000000ff;
		break;
	default: {
			DBG_ERR("_GxGfx_FillRect - ERROR! Not support this pDestDC format.\r\n");
			return GX_ERROR_FORMAT;
		}
	}

	if ((rop == ROP_DESTFILL) &&
		(!pTable && (pDestDC->uiType == TYPE_FB || pDestDC->uiType == TYPE_BITMAP))) {
		UINT8 DestCheckWidthBitOffset;
		DestCheckWidthBitOffset = (UINT8)((DestRect.w << DestSHIFT) & DATA_BITOFFSETMASK);

		if ((DestRect.x == 0) && !DestCheckWidthBitOffset) {
			//fast fill patch

			UINT8 ucByteValue = 0;
			int uiShift = 0;
			while (uiShift < 8) {
				ucByteValue |= (uiColor & DestMASK) << uiShift;
				uiShift += (1 << DestSHIFT);
			}

			return _RectMemSet(
					   (UINT8 *)(pDestDC->blk[3].uiOffset) + pDestDC->blk[3].uiPitch * DestRect.y,
					   (INT16)pDestDC->blk[3].uiPitch,
					   (UINT16)((DestRect.w << DestSHIFT) >> DATA_BIT),
					   (UINT16)DestRect.h,
					   ucByteValue);
		}
	}

#if USE_FAST_BLT //fast path (optimized by dword read/write)
	{
		UINT32 *pDestBaseDW;
		INT32 iDestOffset;
		INT32 iDestBase;
		INT32 iDestPitch;

		INT32 h = (DestRect.h);
		INT32 j;

		pDestBaseDW = (UINT32 *)(pDestDC->blk[3].uiOffset & (~0x03));
		iDestBase = (INT32)(pDestDC->blk[3].uiOffset & 0x03) << 3;
		switch (pDestDC->uiType) {
		case TYPE_FB:
		case TYPE_BITMAP:
			iDestPitch = (pDestDC->blk[3].uiPitch) << 3;
			break;
		case TYPE_ICON:
			//NT96630, NT96450 and After
			if (DestSHIFT == 3) {
				iDestPitch = GxGfx_CalcDCPitchSize(pDestDC->blk[3].uiWidth, TYPE_ICON, pDestDC->uiPxlfmt) << DestSHIFT;
			} else {
				iDestPitch = (pDestDC->blk[3].uiWidth) << DestSHIFT;
			}
			break;
		default:
			return GX_ERROR_TYPE;
		}
		iDestOffset = iDestPitch * DestRect.y + ((DestRect.x) << DestSHIFT);

// ROP_DESTFILL //////////////////////////////////////////////////////////////////////////////////

		if ((uiRop == ROP_DESTFILL) && (!pTable)) {
			register INT32 w = (DestRect.w);
			register INT32 i;

			//make constant dword of this color
			register UINT32 c_dword = 0;
			int uiShift = 0;
			register int uiPixelPerDWORD = 0;
			while (uiShift < 32) {
				c_dword |= (uiColor & DestMASK) << uiShift;
				uiShift += (1 << DestSHIFT);
				uiPixelPerDWORD++;
			}

			for (j = 0; j < h; j++) {
				register UINT32 d_dword;
				register UINT32 *pDestAddrDW;
				register INT32 di;
				INT32 iDestBits;
				register UINT32 dmask = DestMASK;
				register INT32 DestBPP = 1 << DestSHIFT;

				d_dword = 0;
				//2.read dst_dword from dst line data
				iDestBits = (iDestBase + iDestOffset);
				pDestAddrDW = pDestBaseDW + (iDestBits >> 5);
				di = iDestBits & 0x01f; //pixel shift bits in dword

				if (di) { //remove this line if current ROP need read DEST value
					d_dword = (*pDestAddrDW);
				}

				for (i = 0; i < w;) {
					register UINT32 dm;
					register UINT32 rv;

					if (di == 0) {
						if (i + uiPixelPerDWORD < w) {
							//write dword at once

							//6.write dst_dword to dst_ptr
							(*pDestAddrDW) = c_dword; //fill by constant dword

							//read dst_dword from dst_ptr
							pDestAddrDW++;
							i += uiPixelPerDWORD;

							continue;
						} else {
							d_dword = (*pDestAddrDW); //load last data
						}
					}

					//3.get dst pixel data from dst_buf
					dm = (dmask << di);

					//4.do ROP with src and dst pixel data, and put to result pixel data
					rv = ((UINT32)uiColor) & dmask;

					//5.put result pixel data to dst_dword
					d_dword = (d_dword & ~dm) | ((rv << di) & dm);

					di += DestBPP;
					if (di == 32) {
						//6.write dst_dword to dst_ptr
						(*pDestAddrDW) = d_dword;

						//read dst_dword from dst_ptr
						di = 0;
						pDestAddrDW++;

						//d_dword = (*pDestAddrDW);  //add this line if current ROP need read DEST value
					}

					i++;
				}

				if (di) {
					//force flush dst_dword at end of line
					(*pDestAddrDW) = d_dword;
				}

				//next dst line
				iDestOffset += iDestPitch;
			}
			return GX_OK;
		}

// ROP_DESTFILL + Mapping //////////////////////////////////////////////////////////////////////////////////

		if ((uiRop == ROP_DESTFILL) && (pTable)) {
			register INT32 w = (DestRect.w);
			register INT32 i;

			//make constant dword of this color
			register UINT32 c_dword = 0;
			int uiShift = 0;
			register int uiPixelPerDWORD = 0;
			register UINT32 rv = pTable[((UINT32)uiColor) & DestMASK];
			while (uiShift < 32) {
				c_dword |= (rv & DestMASK) << uiShift;
				uiShift += (1 << DestSHIFT);
				uiPixelPerDWORD++;
			}

			for (j = 0; j < h; j++) {
				register UINT32 d_dword;
				register UINT32 *pDestAddrDW;
				register INT32 di;
				INT32 iDestBits;
				register UINT32 dmask = DestMASK;
				register INT32 DestBPP = 1 << DestSHIFT;

				d_dword = 0;
				//2.read dst_dword from dst line data
				iDestBits = (iDestBase + iDestOffset);
				pDestAddrDW = pDestBaseDW + (iDestBits >> 5);
				di = iDestBits & 0x01f; //pixel shift bits in dword

				if (di) { //remove this line if current ROP need read DEST value
					d_dword = (*pDestAddrDW);
				}

				for (i = 0; i < w;) {
					register UINT32 dm;
					//register UINT32 rv;

					if (di == 0) {
						if (i + uiPixelPerDWORD < w) {
							//write dword at once

							//6.write dst_dword to dst_ptr
							(*pDestAddrDW) = c_dword; //fill by constant dword

							//read dst_dword from dst_ptr
							pDestAddrDW++;
							i += uiPixelPerDWORD;

							continue;
						} else {
							d_dword = (*pDestAddrDW); //load last data
						}
					}

					//3.get dst pixel data from dst_buf
					dm = (dmask << di);

					//4.do ROP with src and dst pixel data, and put to result pixel data
					rv = pTable[((UINT32)uiColor) & dmask];

					//5.put result pixel data to dst_dword
					d_dword = (d_dword & ~dm) | ((rv << di) & dm);

					di += DestBPP;
					if (di == 32) {
						//6.write dst_dword to dst_ptr
						(*pDestAddrDW) = d_dword;

						//read dst_dword from dst_ptr
						di = 0;
						pDestAddrDW++;

						//d_dword = (*pDestAddrDW);  //add this line if current ROP need read DEST value
					}

					i++;
				}

				if (di) {
					//force flush dst_dword at end of line
					(*pDestAddrDW) = d_dword;
				}

				//next dst line
				iDestOffset += iDestPitch;
			}
			return GX_OK;
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

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_BitBlt_INDEX(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
						UINT32 uiRop, UINT32 uiParam)
{
	UINT32 rop = uiRop & ROP_MASK;

#if (FORCE_SW_I8 == 1) //some project only use SW without any HW
	if (1)
#else
	if (pDestRect->w <= 2) //w<=2 ---> HW bug
#endif
	{
		return _DC_BitBlt_INDEX_sw(_pDestDC, pDestRect, _pSrcDC, pSrcRect, uiRop, uiParam, 0);
	} else {
		DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
		DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
		IRECT DestRect;
		IRECT SrcRect;
		UINT32 cparam[1];
		INT32 ERcode;
#if (FORCE_SW_I8 == 1)
		RESULT r;
		UINT32 AOP;
#endif

		RECT_SetRect(&DestRect, pDestRect);
		RECT_SetRect(&SrcRect, pSrcRect);

		//check rect zero
		if (_DC_IS_ZERO_INDEX(pDestDC, &DestRect)) {
			return GX_OK;    //skip this draw
		}
		if (_DC_IS_ZERO_INDEX(pSrcDC, &SrcRect)) {
			return GX_OK;    //skip this draw
		}

#if (FORCE_SW_I8 == 1)
		AOP = GRPH_AOP_A_COPY;
		switch (rop) {
		case ROP_COPY:
			AOP = GRPH_AOP_A_COPY;
			cparam[0] = 0;
			break;
		case ROP_KEY:
			AOP = GRPH_AOP_COLOR_EQ;
			cparam[0] = COLOR_YUVD_GET_Y(uiParam);
			break;
		case ROP_DESTFILL:
			AOP = GRPH_AOP_TEXT_COPY;
			_pSrcDC = _pDestDC; //_pSrcDC=0, let _pSrcDC equal to _pDestDC
			pDestRect = pSrcRect;   //pDestRect=0, let pDestRect equal to pSrcRect
			RECT_SetRect(&DestRect, pDestRect);
			cparam[0] = _Repeat4Bytes(COLOR_YUVD_GET_Y(uiParam));
			break;
		default :
			return GX_NOTSUPPORT_ROP;
		}

		//SW path
		_grph_setImage1_dummy(_pSrcDC, 3, &SrcRect);
		_grph_setImage2_dummy(_pDestDC, 3, &DestRect);
		//check GE HW limit
		r = _grph_chkImage1();
		if (r != GX_OK) {
			return r;
		}
		r = _grph_chkImage2();
		if (r != GX_OK) {
			return r;
		}
		ERcode = _sw_setAOP(0, GRPH_DST_2, AOP, cparam[0]);
#else
		switch (rop) {
		case ROP_COPY:
			cparam[0] = 0;
			break;
		case ROP_KEY:
			cparam[0] = COLOR_YUVD_GET_Y(uiParam);
			break;
		case ROP_DESTFILL:
			_pSrcDC = _pDestDC; //_pSrcDC=0, let _pSrcDC equal to _pDestDC
			pDestRect = pSrcRect;   //pDestRect=0, let pDestRect equal to pSrcRect
			RECT_SetRect(&DestRect, pDestRect);
			cparam[0] = _Repeat4Bytes(COLOR_YUVD_GET_Y(uiParam));
			break;
		default :
			return GX_NOTSUPPORT_ROP;
		}

		_grph_setImage1(_pSrcDC, 3, &SrcRect);
		_grph_setImage2(_pDestDC, 3, &DestRect);

        if ((rop == ROP_COPY)||(rop == ROP_KEY)){
        	VF_GFX_COPY         param ={0};
	        HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};

            vir_meminfo.va = (void *)(_DC_bltSrcOffset);
            ERcode = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
        	if ( ERcode != HD_OK) {
        		DBG_ERR("_DC_bltSrcOffset map fail %d\n",ERcode);
        		return GX_OUTOF_MEMORY;
        	}
        	memset(&param, 0, sizeof(VF_GFX_COPY));
        	ERcode = vf_init(&param.src_img, _DC_bltSrcWidth, _DC_bltSrcHeight, HD_VIDEO_PXLFMT_I8, _DC_bltSrcPitch, vir_meminfo.pa,_DC_bltSrcPitch*_DC_bltSrcHeight);
        	if(ERcode){
        		DBG_ERR("fail to init source image\n");
        		return ERcode;
        	}
            vir_meminfo.va = (void *)(_DC_bltDestOffset);
            ERcode = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
        	if ( ERcode != HD_OK) {
        		DBG_ERR("_DC_bltDestOffset map fail %d\n",ERcode);
        		return GX_OUTOF_MEMORY;
        	}
        	ERcode = vf_init(&param.dst_img, _DC_bltDestWidth, _DC_bltDestHeight, HD_VIDEO_PXLFMT_I8, _DC_bltDestPitch,vir_meminfo.pa, _DC_bltDestPitch*_DC_bltDestHeight);
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

            if(rop == ROP_COPY){
        	    ERcode = vf_gfx_copy(&param);
            } else if(rop == ROP_KEY) {
                ERcode = vf_gfx_I8_colorkey(&param);
            }
        }else if(rop == ROP_DESTFILL) {
            VF_GFX_DRAW_RECT    param;
	        HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};
            vir_meminfo.va = (void *)(_DC_bltDestOffset);
            ERcode = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
        	if ( ERcode != HD_OK) {
        		DBG_ERR("_DC_bltDestOffset map fail %d\n",ERcode);
        		return GX_OUTOF_MEMORY;
        	}
            memset(&param, 0, sizeof(VF_GFX_DRAW_RECT));
        	ERcode = vf_init(&param.dst_img, _DC_bltDestWidth, _DC_bltDestHeight, HD_VIDEO_PXLFMT_I8, _DC_bltDestPitch, vir_meminfo.pa, _DC_bltDestPitch*_DC_bltDestHeight);
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
#endif

		if (ERcode != 0) {
            DBG_ERR("fail %d\n",ERcode);
			return GX_DRAW_FAILED;
		}


		return GX_OK;
	}
}
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_BitBlt_INDEX_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
						   UINT32 uiRop, UINT32 uiParam, const MAPPING_ITEM *pTable)
{
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT DestRect;
	IRECT SrcRect;

	UINT32 SrcSHIFT, DestSHIFT;
	UINT32 SrcMASK, DestMASK;
	UINT32 rop = uiRop & ROP_MASK;
	UINT32 colorkey;

	DBG_FUNC_BEGIN("\r\n");

	if (rop == ROP_DESTFILL) {
		return _DC_FillRect_INDEX_sw(_pDestDC, pSrcRect, uiRop, uiParam, 0);
	}


	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_INDEX1 :
		DestSHIFT = 0;
		DestMASK = 0x00000001;
		break;
	case PXLFMT_INDEX2 :
		DestSHIFT = 1;
		DestMASK = 0x00000003;
		break;
	case PXLFMT_INDEX4 :
		DestSHIFT = 2;
		DestMASK = 0x0000000f;
		break;
	case PXLFMT_INDEX8 :
		DestSHIFT = 3;
		DestMASK = 0x000000ff;
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pDestDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}
	switch (pSrcDC->uiPxlfmt) {
	case PXLFMT_INDEX1 :
		SrcSHIFT = 0;
		SrcMASK = 0x00000001;
		break;
	case PXLFMT_INDEX2 :
		SrcSHIFT = 1;
		SrcMASK = 0x00000003;
		break;
	case PXLFMT_INDEX4 :
		SrcSHIFT = 2;
		SrcMASK = 0x0000000f;
		break;
	case PXLFMT_INDEX8 :
		SrcSHIFT = 3;
		SrcMASK = 0x000000ff;
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pSrcDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}
	if (SrcSHIFT > DestSHIFT) {
		DBG_ERR("_GxGfx_BitBlt - ERROR! Not support convert between pSrcDC/pDestDC format.\r\n");
		return GX_CANNOT_CONVERT;
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
	//check rect zero
	if (_DC_IS_ZERO_INDEX(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}
	if (_DC_IS_ZERO_INDEX(pSrcDC, &SrcRect)) {
		return GX_OK;    //skip this draw
	}

	if ((uiRop == ROP_COPY) && (pTable == 0) &&
		(pDestDC->uiPxlfmt == pSrcDC->uiPxlfmt) &&
		(pDestDC->uiType == TYPE_FB || pDestDC->uiType == TYPE_BITMAP) &&
		(pSrcDC->uiType == TYPE_FB || pSrcDC->uiType == TYPE_BITMAP)) {
		UINT8 SrcCheckWidthBitOffset = (UINT8)((SrcRect.w << SrcSHIFT) & DATA_BITOFFSETMASK);
		UINT8 DestCheckWidthBitOffset = (UINT8)((DestRect.w << DestSHIFT) & DATA_BITOFFSETMASK);

		if ((SrcRect.x == 0) && !SrcCheckWidthBitOffset &&
			(DestRect.x == 0) && !DestCheckWidthBitOffset) {
			//fast path for copy between INDEX/INDEX format with the same size

			UINT8 *pDestAddr = (UINT8 *)(pDestDC->blk[3].uiOffset) + pDestDC->blk[3].uiPitch * DestRect.y;
			UINT8 *pSrcAddr = (UINT8 *)(pSrcDC->blk[3].uiOffset) + pSrcDC->blk[3].uiPitch * SrcRect.y;
			UINT32 uiWidth = (SrcRect.w  << SrcSHIFT) >> DATA_BIT;
#if (FORCE_SW_I8 == 1)
			//use SW path
			return _RectMemCopy(
					   pDestAddr,
					   (INT16)pDestDC->blk[3].uiPitch,
					   pSrcAddr,
					   (INT16)pSrcDC->blk[3].uiPitch,
					   (UINT16)uiWidth, (UINT16)SrcRect.h);
#else
			//use HW path
			if (_DC_RectMemCopy(
					pDestAddr,
					(INT16)pDestDC->blk[3].uiPitch,
					pSrcAddr,
					(INT16)pSrcDC->blk[3].uiPitch,
					(UINT16)uiWidth, (UINT16)SrcRect.h) != GX_OK) {
				//use SW path
				return _RectMemCopy(
						   pDestAddr,
						   (INT16)pDestDC->blk[3].uiPitch,
						   pSrcAddr,
						   (INT16)pSrcDC->blk[3].uiPitch,
						   (UINT16)uiWidth, (UINT16)SrcRect.h);
			}
#endif
			return GX_OK;
		}
	}

#if USE_FAST_BLT //fast path (optimized by dword read/write)
	{
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

		pSrcBaseDW = (UINT32 *)(pSrcDC->blk[3].uiOffset & (~0x03));
		iSrcBase = (INT32)(pSrcDC->blk[3].uiOffset & 0x03) << 3;
		switch (pSrcDC->uiType) {
		case TYPE_FB:
		case TYPE_BITMAP:
			iSrcPitch = (pSrcDC->blk[3].uiPitch) << 3;
			break;
		case TYPE_ICON:
			if (SrcSHIFT == 3) {
				iSrcPitch = GxGfx_CalcDCPitchSize(pSrcDC->blk[3].uiWidth, TYPE_ICON, pSrcDC->uiPxlfmt) << SrcSHIFT;
			} else {
				iSrcPitch = (pSrcDC->blk[3].uiWidth) << SrcSHIFT;
			}
			break;
		default:
			return GX_ERROR_TYPE;
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
			if (DestSHIFT == 3) {
				iDestPitch = GxGfx_CalcDCPitchSize(pDestDC->blk[3].uiWidth, TYPE_ICON, pDestDC->uiPxlfmt) << DestSHIFT;
			} else {
				iDestPitch = (pDestDC->blk[3].uiWidth) << DestSHIFT;
			}
			break;
		default:
			return GX_ERROR_TYPE;
		}
		iDestOffset = iDestPitch * DestRect.y + ((DestRect.x) << DestSHIFT);

// ROP_COPY //////////////////////////////////////////////////////////////////////////////////

		if ((uiRop == ROP_COPY) && (!pTable)) {
			for (j = 0; j < h; j++) {
				register UINT32 s_dword, d_dword;
				register UINT32 *pDestAddrDW;
				register UINT32 *pSrcAddrDW;
				register INT32 di, si;
				INT32 iSrcBits;
				INT32 iDestBits;
				register UINT32 smask = SrcMASK;
				register UINT32 dmask = DestMASK;
				register INT32 DestBPP = 1 << DestSHIFT;
				register INT32 SrcBPP = 1 << SrcSHIFT;

				d_dword = 0;
				//1.read src_buf from src line data
				iSrcBits = (iSrcBase + iSrcOffset);
				pSrcAddrDW = pSrcBaseDW + (iSrcBits >> 5);
				si = iSrcBits & 0x01f; //pixel shift bits in dword
				s_dword = (*pSrcAddrDW);

				//2.read dst_dword from dst line data
				iDestBits = (iDestBase + iDestOffset);
				pDestAddrDW = pDestBaseDW + (iDestBits >> 5);
				di = iDestBits & 0x01f; //pixel shift bits in dword

				if (di) { //remove this line if current ROP need read DEST value
					d_dword = (*pDestAddrDW);
				}

				if ((w << SrcSHIFT) < 32) {
					d_dword = (*pDestAddrDW);  //add this line if current ROP need read DEST value
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
					dm = (dmask << di);
					//dv = (d_dword & dm) >> di;

					//4.do ROP with src and dst pixel data, and put to result pixel data
					rv = sv;

					//5.put result pixel data to dst_dword
					d_dword = (d_dword & ~dm) | ((rv << di) & dm);

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
						(*pDestAddrDW) = d_dword;

						//read dst_dword from dst_ptr
						di = 0;
						pDestAddrDW++;

						if (((w - i - 1) << SrcSHIFT) < 32) {
							d_dword = (*pDestAddrDW);  //add this line if current ROP need read DEST value
						}
					}
				}

				if (di) {
					//force flush dst_dword at end of line
					(*pDestAddrDW) = d_dword;
				}

				//next src line
				iSrcOffset += iSrcPitch;

				//next dst line
				iDestOffset += iDestPitch;
			}
			return GX_OK;
		}

// ROP_COPY + Mapping //////////////////////////////////////////////////////////////////////////////////

		else if ((uiRop == ROP_COPY) && (pTable)) {
			for (j = 0; j < h; j++) {
				register UINT32 s_dword, d_dword;
				register UINT32 *pDestAddrDW;
				register UINT32 *pSrcAddrDW;
				register INT32 di, si;
				INT32 iSrcBits;
				INT32 iDestBits;
				register UINT32 smask = SrcMASK;
				register UINT32 dmask = DestMASK;
				register INT32 DestBPP = 1 << DestSHIFT;
				register INT32 SrcBPP = 1 << SrcSHIFT;

				d_dword = 0;
				//1.read src_buf from src line data
				iSrcBits = (iSrcBase + iSrcOffset);
				pSrcAddrDW = pSrcBaseDW + (iSrcBits >> 5);
				si = iSrcBits & 0x01f; //pixel shift bits in dword
				s_dword = (*pSrcAddrDW);

				//2.read dst_dword from dst line data
				iDestBits = (iDestBase + iDestOffset);
				pDestAddrDW = pDestBaseDW + (iDestBits >> 5);
				di = iDestBits & 0x01f; //pixel shift bits in dword

				if (di) { //remove this line if current ROP need read DEST value
					d_dword = (*pDestAddrDW);
				}

				if ((w << SrcSHIFT) < 32) {
					d_dword = (*pDestAddrDW);  //add this line if current ROP need read DEST value
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
					dm = (dmask << di);
					//dv = (d_dword & dm) >> di;

					//4.do ROP with src and dst pixel data, and put to result pixel data
					rv = pTable[sv];

					//5.put result pixel data to dst_dword
					d_dword = (d_dword & ~dm) | ((rv << di) & dm);

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
						(*pDestAddrDW) = d_dword;

						//read dst_dword from dst_ptr
						di = 0;
						pDestAddrDW++;

						if (((w - i - 1) << SrcSHIFT) < 32) {
							d_dword = (*pDestAddrDW);  //add this line if current ROP need read DEST value
						}
					}
				}

				if (di) {
					//force flush dst_dword at end of line
					(*pDestAddrDW) = d_dword;
				}

				//next src line
				iSrcOffset += iSrcPitch;

				//next dst line
				iDestOffset += iDestPitch;
			}
			return GX_OK;
		}

// ROP_KEY //////////////////////////////////////////////////////////////////////////////////

		else if ((uiRop == ROP_KEY) && (!pTable)) {
			for (j = 0; j < h; j++) {
				int load = 0;
				int dirty = 0;

				register UINT32 s_dword, d_dword;
				register UINT32 *pDestAddrDW;
				register UINT32 *pSrcAddrDW;
				register INT32 di, si;
				INT32 iSrcBits;
				INT32 iDestBits;
				register UINT32 smask = SrcMASK;
				register UINT32 dmask = DestMASK;
				register INT32 DestBPP = 1 << DestSHIFT;
				register INT32 SrcBPP = 1 << SrcSHIFT;

				d_dword = 0;
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
					rv = sv;
					if (rv != colorkey) {
						//5.put result pixel data to dst_dword
						if (!load) {
							d_dword = (*pDestAddrDW);
							load = 1;
						}
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
				iSrcOffset += iSrcPitch;

				//next dst line
				iDestOffset += iDestPitch;
			}
			return GX_OK;
		}

// ROP_KEY + Mapping //////////////////////////////////////////////////////////////////////////////////

		else if ((uiRop == ROP_KEY) && (pTable)) {
			for (j = 0; j < h; j++) {
				int load = 0;
				int dirty = 0;

				register UINT32 s_dword, d_dword;
				register UINT32 *pDestAddrDW;
				register UINT32 *pSrcAddrDW;
				register INT32 di, si;
				INT32 iSrcBits;
				INT32 iDestBits;
				register UINT32 smask = SrcMASK;
				register UINT32 dmask = DestMASK;
				register INT32 DestBPP = 1 << DestSHIFT;
				register INT32 SrcBPP = 1 << SrcSHIFT;

				d_dword = 0;
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
					rv = pTable[sv];
					if (rv != colorkey) {
						//5.put result pixel data to dst_dword
						if (!load) {
							d_dword = (*pDestAddrDW);
							load = 1;
						}
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
				iSrcOffset += iSrcPitch;

				//next dst line
				iDestOffset += iDestPitch;
			}
			return GX_OK;
		}
	}

#else //slow path

	//INDEX2 -> INDEX2
	//INDEX2 -> INDEX4
	//INDEX2 -> INDEX8
	//INDEX4 -> INDEX4
	//INDEX4 -> INDEX8
	//INDEX8 -> INDEX8
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
					if (pTable) {
						DestValue = pTable[DestValue];    //replace
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
				if (pTable) {
					SrcValue = pTable[SrcValue];    //replace
				}
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

		return GX_OK;
	}



#endif //path

	DBG_ERR("NOTSUPPORT_ROP 0x%x\r\n", rop);
	return GX_NOTSUPPORT_ROP;
}


//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_RotateBlt_INDEX(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
						   UINT32 uiRop, UINT32 uiParam)
{
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT DestRect;
	IRECT SrcRect;
	UINT32 rop = uiRop & ROP_MASK;
#if (FORCE_SW_I8_ROT == 1)
#else
	ER ERcode;
	UINT32 GOP=0;
#endif

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

#if (FORCE_SW_I8_ROT == 1)
	DBG_ERR("NOT support rotate!\r\n");
#else
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
    		GOP = GR_RCW_90|HD_VIDEO_DIR_MIRRORXY;
    		break;
    	case GR_RCW_180|GR_MRR_X|GR_MRR_Y:
    		GOP = GR_RCW_180|HD_VIDEO_DIR_MIRRORXY;
    		break;
    	case GR_RCW_270|GR_MRR_X|GR_MRR_Y:
    		GOP = GR_RCW_270|HD_VIDEO_DIR_MIRRORXY;
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
        VF_GFX_ROTATE param={0};
        HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};

        vir_meminfo.va = (void *)(_DC_bltSrcOffset);
        ERcode = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
    	if ( ERcode != HD_OK) {
        	DBG_ERR("_DC_bltSrcOffset map fail %d\n",ERcode);
		    return GX_OUTOF_MEMORY;
    	}
    	//use gfx engine to mirror the image
    	memset(&param, 0, sizeof(VF_GFX_ROTATE));
    	ERcode = vf_init(&param.src_img, _DC_bltSrcWidth, _DC_bltSrcHeight, HD_VIDEO_PXLFMT_I8, _DC_bltSrcPitch, vir_meminfo.pa, _DC_bltSrcPitch*_DC_bltSrcHeight);
    	if(ERcode){
    		DBG_ERR("fail to init source image\n");
    		return ERcode;
    	}
        vir_meminfo.va = (void *)(_DC_bltDestOffset);
        ERcode = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
    	if ( ERcode != HD_OK) {
        	DBG_ERR("_DC_bltDestOffset map fail %d\n",ERcode);
    		return GX_OUTOF_MEMORY;
    	}
    	ERcode = vf_init(&param.dst_img, _DC_bltDestWidth, _DC_bltDestHeight, HD_VIDEO_PXLFMT_I8, _DC_bltDestPitch, vir_meminfo.pa, _DC_bltDestPitch*_DC_bltDestHeight);
    	if(ERcode){
    		DBG_ERR("fail to init dest image\n");
    		return ERcode;
    	}

    	param.src_region.x             = SrcRect.x;
    	param.src_region.y             = SrcRect.y;
    	param.src_region.w             = SrcRect.w;
    	param.src_region.h             = SrcRect.h;
    	param.dst_pos.x                = 0;
    	param.dst_pos.y                = 0;
    	param.angle                    = GOP;
        #if 0
        DBGD(param.src_region.x);
        DBGD(param.src_region.y);
        DBGD(param.src_region.w);
        DBGD(param.src_region.h);
        DBGH(param.angle);
        #endif
    	ERcode= vf_gfx_rotate(&param);
    	}

#endif

#if (FORCE_SW_I8_ROT == 1)
#else
	if (ERcode != E_OK) {
		return GX_DRAW_FAILED;
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
RESULT _DC_FontBlt_INDEX_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							const MAPPING_ITEM *pTable)
{
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;

	IRECT DestRect;
	IRECT SrcRect;

	UINT32 SrcSHIFT, DestSHIFT;
	UINT32 SrcMASK, DestMASK;

	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_INDEX1 :
		DestSHIFT = 0;
		DestMASK = 0x00000001;
		break;
	case PXLFMT_INDEX2 :
		DestSHIFT = 1;
		DestMASK = 0x00000003;
		break;
	case PXLFMT_INDEX4 :
		DestSHIFT = 2;
		DestMASK = 0x0000000f;
		break;
	case PXLFMT_INDEX8 :
		DestSHIFT = 3;
		DestMASK = 0x000000ff;
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pDestDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}
	switch (pSrcDC->uiPxlfmt) {
	case PXLFMT_INDEX1 :
		SrcSHIFT = 0;
		SrcMASK = 0x00000001;
		break;
	case PXLFMT_INDEX2 :
		SrcSHIFT = 1;
		SrcMASK = 0x00000003;
		break;
	case PXLFMT_INDEX4 :
		SrcSHIFT = 2;
		SrcMASK = 0x0000000f;
		break;
	case PXLFMT_INDEX8 :
		SrcSHIFT = 3;
		SrcMASK = 0x000000ff;
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pSrcDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}
	if (SrcSHIFT > DestSHIFT) {
		DBG_ERR("_GxGfx_BitBlt - ERROR! Not support convert between pSrcDC/pDestDC format.\r\n");
		return GX_CANNOT_CONVERT;
	}

	if (!pTable) {
		DBG_ERR("_GxGfx_BitBlt - ERROR! Need specify mapping for ROP_FONT.\r\n");
		return GX_CANNOT_CONVERT;
	}

	RECT_SetRect(&DestRect, pDestRect);
	RECT_SetRect(&SrcRect, pSrcRect);

#if USE_FAST_BLT //fast path

// ROP_KEY + Mapping //////////////////////////////////////////////////////////////////////////////////

	{
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
		default:
			return GX_ERROR_TYPE;
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
		default:
			return GX_ERROR_TYPE;
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
			register UINT32 smask = SrcMASK;
			register UINT32 dmask = DestMASK;
			register INT32 DestBPP = 1 << DestSHIFT;
			register INT32 SrcBPP = 1 << SrcSHIFT;

			d_dword = 0;
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
				rv = pTable[sv]; //mapping font index to user custom color

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
		//return GX_OK;
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
				if (DestValue != COLOR_TRANSPARENT) {
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


//FONTEFFECT_NONE
const IPOINT effect_tbl_0[] = {{0, 0}};
//FONTEFFECT_SHADOW
const IPOINT effect_tbl_1[] = {{1, 1}, {0, 0}};
//FONTEFFECT_SHADOW2
const IPOINT effect_tbl_2[] = {{2, 1}, {1, 1}, {0, 0}};
//FONTEFFECT_HIGHLIGHT
const IPOINT effect_tbl_3[] = {{ -1, -1}, {0, 0}};
//FONTEFFECT_HIGHLIGHT2
const IPOINT effect_tbl_4[] = {{ -2, -1}, { -1, -1}, {0, 0}};
//FONTEFFECT_OUTLINE
const IPOINT effect_tbl_5[] = {{ -1, -1}, {0, -1}, {1, -1}, { -1, 0}, {1, 0}, { -1, 1}, {0, 1}, {1, 1}, {0, 0}};
//FONTEFFECT_OUTLINE2
const IPOINT effect_tbl_6[] = {{ -2, -1}, { -1, -1}, {0, -1}, {1, -1}, {2, -1}, { -2, 0}, { -1, 0}, {1, 0}, {2, 0}, { -2, 1}, { -1, 1}, {0, 1}, {1, 1}, {2, 1}, {0, 0}};
const IPOINT *effect_tbl[] = {
	effect_tbl_0,
	effect_tbl_1,
	effect_tbl_2,
	effect_tbl_3,
	effect_tbl_4,
	effect_tbl_5,
	effect_tbl_6,
};

//extend size of x,y
const IPOINT ex_tbl[] = {
	{0, 0}, //FONTEFFECT_NONE
	{2, 2}, //FONTEFFECT_SHADOW
	{4, 2}, //FONTEFFECT_SHADOW2
	{2, 2}, //FONTEFFECT_HIGHLIGHT
	{4, 2}, //FONTEFFECT_HIGHLIGHT2
	{2, 2}, //FONTEFFECT_OUTLINE
	{4, 2}, //FONTEFFECT_OUTLINE2
};

IPOINT _DC_GetFontEffectExtend(UINT32 iFontEffect)
{
	int effect = DSF_FE(iFontEffect);
	if (iFontEffect & F_NOEXTEND) {
		effect = 0;
	}
	return ex_tbl[effect];
}

/*
TODO: font style
#define FONTSTYLE_ITALIC            0x00000001  //font style : italic
    -> dx/dy = 3/8 (字型的y每偏移8點, x偏移3點)
#define FONTSTYLE_BOLD              0x00000002  //font style : bold
    -> d_point/font_high = (1/8)-1 (字型大小每多出8點, 厚度多出1點)
#define FONTSTYLE_UNDERLINE         0x00000004  //font style : underline (use color index 3)
#define FONTSTYLE_DELETELINE        0x00000008  //font style : underline (use color index 3)
*/

const UINT8 _DC_italic_tbl[8] = {0, 0, 1, 1, 1, 2, 2, 2};

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_FontEffectBlt_INDEX_sw(DC *_pDestDC, const DC *_pSrcDC, UINT32 iFontEffect)
{
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	UINT8 SrcBPP = 0;
	UINT8 DestBPP = 0;
	LVALUE i, j;

	IRECT DestRect;
	IRECT SrcRect;

	PIXEL_2D DestPx = {0};
	PIXEL_2D SrcPx = {0};

	int effect = DSF_FE(iFontEffect);
	IPOINT dv = _DC_GetFontEffectExtend(iFontEffect); //vector to center point
	dv.x = dv.x / 2;
	dv.y = dv.y / 2;

	switch (pDestDC->uiPxlfmt) {
//    case PXLFMT_INDEX1 : DestBPP = 1; break;
//    case PXLFMT_INDEX2 : DestBPP = 2; break;
//    case PXLFMT_INDEX4 : DestBPP = 4; break;
	case PXLFMT_INDEX8 :
		DestBPP = 8;
		break;
	default:
		DBG_ERR("_GxGfx_FontEffect - ERROR! Not support make effect to this pDestDC.\r\n");
		return GX_CANNOT_CONVERT;
	}
	switch (pSrcDC->uiPxlfmt) {
	case PXLFMT_INDEX1 :
		SrcBPP = 1;
		break;
	case PXLFMT_INDEX2 :
		SrcBPP = 2;
		break;
	case PXLFMT_INDEX4 :
		SrcBPP = 4;
		break;
	case PXLFMT_INDEX8 :
		SrcBPP = 8;
		break;
	}
	if (SrcBPP > DestBPP) {
		DBG_ERR("_GxGfx_FontEffect - ERROR! Not support make effect to this pDestDC.\r\n");
		return GX_CANNOT_CONVERT;
	}

	RECT_Set(&DestRect, 0, 0, _pDestDC->size.w,  _pDestDC->size.h);
	RECT_Set(&SrcRect, 0, 0, _pSrcDC->size.w,  _pSrcDC->size.h);

	//INDEX2 -> INDEX2
	//INDEX2 -> INDEX4
	//INDEX2 -> INDEX8
	//INDEX4 -> INDEX4
	//INDEX4 -> INDEX8
	//INDEX8 -> INDEX8

	//clear
	PIXEL_2D_Init(&DestPx, _pDestDC);
	PIXEL_2D_Check(&DestPx);
	PIXEL_2D_Begin(&DestPx, &DestRect);
	for (j = 0; j < DestRect.h; j++) {
		PIXEL_2D_BeginLine(&DestPx);
		for (i = 0; i < DestRect.w; i++) {
			PIXEL_2D_BeginPixel(&DestPx);
			PIXEL_2D_Write(&DestPx, COLOR_INDEX_FONTBACK);
			PIXEL_2D_EndPixel(&DestPx, FALSE, TRUE);
		}
		PIXEL_2D_EndLine(&DestPx);
	}
	PIXEL_2D_End(&DestPx);

	//draw shadow
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

			PIXEL_2D_BeginPixel(&DestPx);
			PIXEL_2D_BeginPixel(&SrcPx);
			SrcValue = PIXEL_2D_Read(&SrcPx);
			if (SrcValue == COLOR_INDEX_FONTFACE) {
				const IPOINT *Pt = effect_tbl[effect];
				while (!((Pt->x == 0) && (Pt->y == 0))) {
					int dx = dv.x + Pt->x;
					int dy = dv.y + Pt->y;
					if ((dx >= 0) && (dx < DestRect.w) && (dy >= 0) && (dy < DestRect.h)) {
						PIXEL_2D_WriteOffset(&DestPx, COLOR_INDEX_FONTSHADOW, dx, dy);
					}
					Pt++;
				}
			}
			PIXEL_2D_EndPixel(&DestPx, FALSE, TRUE);
			PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE);
		}
		PIXEL_2D_EndLine(&DestPx);
		PIXEL_2D_EndLine(&SrcPx);
	}
	PIXEL_2D_End(&DestPx);
	PIXEL_2D_End(&SrcPx);

	//draw face
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

			PIXEL_2D_BeginPixel(&DestPx);
			PIXEL_2D_BeginPixel(&SrcPx);
			SrcValue = PIXEL_2D_Read(&SrcPx);
			if (SrcValue == COLOR_INDEX_FONTFACE) {
				int dx = dv.x;
				int dy = dv.y;
				PIXEL_2D_WriteOffset(&DestPx, COLOR_INDEX_FONTFACE, dx, dy);
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



//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_StretchBlt_INDEX
(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
 UINT32 uiRop, UINT32 uiParam, const MAPPING_ITEM *pTable)
{
    #if FORCE_SW_I8_SCALE
    return _DC_StretchBlt_INDEX_sw(_pDestDC,pDestRect,_pSrcDC,pSrcRect,uiRop,uiParam,pTable);
    #else

	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
    RESULT ERcode;
	UINT32 filter = uiRop & FILTER_MASK;
	UINT32 sclMethod = 0;
	IRECT DestRect;
	IRECT SrcRect;


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

	if ((filter == FILTER_NONE) || (filter == FILTER_PIXEL)) {
		sclMethod = HD_GFX_SCALE_QUALITY_NULL;
	}
	if (filter == FILTER_LINEAR)  {
		sclMethod = HD_GFX_SCALE_QUALITY_BILINEAR;
	}
    if (filter == FILTER_CUBIC)  {
		sclMethod = HD_GFX_SCALE_QUALITY_BICUBIC;
	}

	_ime_setImage1(_pSrcDC, 3, &SrcRect);
	_ime_setImage2(_pDestDC, 3, &DestRect);
    {
        VF_GFX_SCALE param={0};
        HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};

        vir_meminfo.va = (void *)(_DC_SclSrcOffsetY);
        ERcode = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
    	if ( ERcode != HD_OK) {
        	DBG_ERR("_DC_SclSrcOffsetY map fail %d\n",ERcode);
    		return GX_OUTOF_MEMORY;
    	}
    	//use gfx engine to mirror the image
    	memset(&param, 0, sizeof(VF_GFX_SCALE));
    	ERcode = vf_init(&param.src_img, _DC_SclSrcWidth, _DC_SclSrcHeight, HD_VIDEO_PXLFMT_I8, _DC_SclSrcPitch, vir_meminfo.pa, _DC_SclSrcPitch*_DC_SclSrcHeight);
    	if(ERcode){
    		DBG_ERR("fail to init source image\n");
    		return ERcode;
    	}
        vir_meminfo.va = (void *)(_DC_SclDestOffsetY);
        ERcode = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
    	if ( ERcode != HD_OK) {
        	DBG_ERR("_DC_SclDestOffsetY map fail %d\n",ERcode);
    		return GX_OUTOF_MEMORY;
    	}
    	ERcode = vf_init(&param.dst_img, _DC_SclDestWidth, _DC_SclDestHeight, HD_VIDEO_PXLFMT_I8, _DC_SclDestPitch, vir_meminfo.pa, _DC_SclDestPitch*_DC_SclDestHeight);
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
        return ERcode;
    #endif
}


/*

Scale up: Bi-linear interpolation
DWORD GetSubTexel( int x, int y )
{
const int h = (x & 0xff00) / 255;
const int i = (y & 0xff00) / 255;

x = x >> 16;
y = y >> 16;

const COLORREF cr1 = GetTexel( x + 0, y + 0 );
const COLORREF cr2 = GetTexel( x + 1, y + 0 );
const COLORREF cr3 = GetTexel( x + 1, y + 1 );
const COLORREF cr4 = GetTexel( x + 0, y + 1 );

const int a = (0x100 - h) * (0x100 - i);
const int b = (0x000 + h) * (0x100 - i);
const int c = (0x000 + h) * (0x000 + i);
const int d = 65536 - a - b - c;

const unsigned int R = 0x00ff0000 & (((cr1 >> 16) * a) + ((cr2 >> 16) * b) + ((cr3 >> 16) * c) + ((cr4 >> 16) * d));
const unsigned int G = 0xff000000 & (((cr1 & 0x00ff00) * a) + ((cr2 & 0x00ff00) * b) + ((cr3 & 0x00ff00) * c) + ((cr4 & 0x00ff00) * d));
const unsigned int B = 0x00ff0000 & (((cr1 & 0x0000ff) * a) + ((cr2 & 0x0000ff) * b) + ((cr3 & 0x0000ff) * c) + ((cr4 & 0x0000ff) * d));

return R|((G|B)>>16);
*/

/*
Scale down: Smooth Bresenham

void ScaleLine(PIXEL *Target, PIXEL *Source, int SrcWidth, int TgtWidth)
{
  int NumPixels = TgtWidth;
  int IntPart = SrcWidth / TgtWidth;
  int FractPart = SrcWidth % TgtWidth;
  int E = 0;

  while (NumPixels-- > 0) {

    *Target++ = *Source;

    Source += IntPart;

    E += FractPart;
    if (E >= TgtWidth) {
      E -= TgtWidth;
      Source++;
    }
  }
}

#define average(a, b)   (PIXEL)(( (int)(a) + (int)(b) ) >> 1)
void ScaleLineAvg(PIXEL *Target, PIXEL *Source, int SrcWidth, int TgtWidth)
{
  int NumPixels = TgtWidth;

  int Mid = TgtWidth / 2;

  int E = 0;

  PIXEL p;

  if (TgtWidth > SrcWidth)
    NumPixels--;

  while (NumPixels-- > 0) {

    p = *Source;
    if (E >= Mid)
     p = average(p, *(Source+1));
    *Target++ = p;

    E += SrcWidth;
    if (E >= TgtWidth) {
      E -= TgtWidth;
      Source++;
    }
  }

  if (TgtWidth > SrcWidth)
    *Target = *Source;
}

void ScaleRect(PIXEL *Target, PIXEL *Source, int SrcWidth, int SrcHeight,
               int TgtWidth, int TgtHeight)
{
  int NumPixels = TgtHeight;
  int IntPart = (SrcHeight / TgtHeight) * SrcWidth;
  int FractPart = SrcHeight % TgtHeight;
  int E = 0;
  PIXEL *PrevSource = NULL;
  while (NumPixels-- > 0) {
    if (Source == PrevSource) {
      memcpy(Target, Target-TgtWidth, TgtWidth*sizeof(*Target));
    } else {
      ScaleLine(Target, Source, SrcWidth, TgtWidth);
      PrevSource = Source;
    }
    Target += TgtWidth;
    Source += IntPart;
    E += FractPart;
    if (E >= TgtHeight) {
      E -= TgtHeight;
      Source += SrcWidth;
    }
  }
}
*/

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_IDX format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_StretchBlt_INDEX_sw
(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
 UINT32 uiRop, UINT32 uiParam, const MAPPING_ITEM *pTable)
{
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;

//  IRECT SrcPlaneRect;
//  IRECT DestPlaneRect;

//    LVALUE i,j;
	UINT16 n;
	UINT32 rop = uiRop & ROP_MASK;
	UINT32 colorkey = 0;
	UINT32 filter = uiRop & FILTER_MASK;

	IRECT DestRect;
	IRECT SrcRect;

	PIXEL_2D DestPx = {0};
	PIXEL_2D SrcPx = {0};

	UINT8 SrcBPP = 0;
	UINT8 DestBPP = 0;

	int sw, sh, dw, dh;

	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_INDEX1 :
		DestBPP = 1;
		break;
	case PXLFMT_INDEX2 :
		DestBPP = 2;
		break;
	case PXLFMT_INDEX4 :
		DestBPP = 4;
		break;
	case PXLFMT_INDEX8 :
		DestBPP = 8;
		break;
	}
	switch (pSrcDC->uiPxlfmt) {
	case PXLFMT_INDEX1 :
		SrcBPP = 1;
		break;
	case PXLFMT_INDEX2 :
		SrcBPP = 2;
		break;
	case PXLFMT_INDEX4 :
		SrcBPP = 4;
		break;
	case PXLFMT_INDEX8 :
		SrcBPP = 8;
		break;
	}
	if (SrcBPP > DestBPP) {
		DBG_ERR("_GxGfx_StretchBlt - ERROR! Not support convert between pSrcDC/pDestDC format.\r\n");
		return GX_CANNOT_CONVERT;
	}

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
	//check rect zero
	if (_DC_IS_ZERO_INDEX(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}
	if (_DC_IS_ZERO_INDEX(pSrcDC, &SrcRect)) {
		return GX_OK;    //skip this draw
	}

	sw = SrcRect.w;
	sh = SrcRect.h;
	dw = DestRect.w;
	dh = DestRect.h;

	if (!filter) {
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
					if (pTable) {
						DestValue = pTable[DestValue];    //replace
					}
					bWrite = TRUE;
					break;
				case ROP_KEY:
					if (pTable) {
						SrcValue = pTable[SrcValue];    //replace
					}
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
				UINT32 AccCnt = 0;
				PIXEL_2D_BeginPixel(&DestPx);
				DestValue = PIXEL_2D_Read(&DestPx);

				//frac part
				if (!bLoad) {
					PIXEL_2D_BeginPixel(&SrcPx);
					SrcValue = PIXEL_2D_Read(&SrcPx);
					switch (rop) {
					case ROP_COPY:
					case ROP_KEY:
						if (pTable) {
							SrcValue = pTable[SrcValue];    //replace
						}
						break;
					}
					PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next src pixel
					bLoad = 1;
				}
				AccValue += SrcValue * (dw - xE);
				AccCnt += (dw - xE);

				//int part
				xJump = xIntPart; //jump to offset
				if (xJump > 0) { //scale down
					for (n = 0; n < xJump - 1; n++) {
						PIXEL_2D_BeginPixel(&SrcPx);
						SrcValue = PIXEL_2D_Read(&SrcPx);
						switch (rop) {
						case ROP_COPY:
						case ROP_KEY:
							if (pTable) {
								SrcValue = pTable[SrcValue];    //replace
							}
							break;
						}
						PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next src pixel

						AccValue += SrcValue * dw;
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
							case ROP_KEY:
								if (pTable) {
									SrcValue = pTable[SrcValue];    //replace
								}
								break;
							}
							PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next src pixel

							AccValue += SrcValue * dw;
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
						if (pTable) {
							SrcValue = pTable[SrcValue];    //replace
						}
						break;
					}
					PIXEL_2D_EndPixel(&SrcPx, FALSE, TRUE); //next src pixel
					bLoad = 1;

					AccValue += SrcValue * xE;
					AccCnt += xE;
				} else {
					bLoad = 0;
				}

				//copy
				DestValue = AccValue / AccCnt;
				if (DestValue != colorkey) {
					PIXEL_2D_Write(&DestPx, DestValue);
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

	return GX_OK;
}

//--------------------------------------------------------------------------------------
//  PIXEL_2D
//--------------------------------------------------------------------------------------
typedef struct _PIXEL_2D_INDEXn {
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
	INT32 PackOffset;
	INT32 uiBitPitch;
	INT32 Limit;

	BOOL bLoad;
	BOOL bDirty;
	UINT8 BPP;
	DATA_PACK Mask;
	INT8 BitOffset;
	INT8 BitOffsetFinal;
#if USE_BIT_REVERSE
	BOOL bBitReverse;
#endif
	DATA_PACK PackData;
	DATA_PACK *pLine;
	DATA_PACK *pData;
} PIXEL_2D_INDEXn;


static void PIXEL_2D_INDEXn_Begin(PIXEL_2D *_p, IRECT *pRect)
{
	PIXEL_2D_INDEXn *p = (PIXEL_2D_INDEXn *)_p;

	switch (p->pDC->uiPxlfmt) {
	case PXLFMT_INDEX1 :
		p->BPP = 1;
		break;
	case PXLFMT_INDEX2 :
		p->BPP = 2;
		break;
	case PXLFMT_INDEX4 :
		p->BPP = 4;
		break;
	default:
		return;
	}
	switch (p->pDC->uiType) {
	case TYPE_FB:
		p->uiBitPitch = p->pDC->blk[3].uiPitch * 8;
#if USE_BIT_REVERSE
		p->bBitReverse = 1;
#endif
		break;
	case TYPE_BITMAP:
		p->uiBitPitch = p->pDC->blk[3].uiPitch * 8;
#if USE_BIT_REVERSE
		p->bBitReverse = 0;
#endif
		break;
	case TYPE_ICON:
		p->uiBitPitch = p->pDC->blk[3].uiWidth * p->BPP;
#if USE_BIT_REVERSE
		p->bBitReverse = 0;
#endif
		break;
	}
	p->Mask = (UINT8)((1 << (p->BPP)) - 1);

	p->pRect = pRect;

#if 1
	p->Limit = (p->uiBitPitch * p->pDC->size.h);
#endif
	p->PackOffset =
		p->pRect->y * p->uiBitPitch + p->pRect->x * p->BPP;

	p->bDirty = FALSE;
	p->bLoad = FALSE;
	p->x = 0;
	p->y = 0;
}
static void PIXEL_2D_INDEXn_BeginLine(PIXEL_2D *_p)
{
	PIXEL_2D_INDEXn *p = (PIXEL_2D_INDEXn *)_p;

	p->pLine = ((DATA_PACK *)(p->pDC->blk[3].uiOffset)) +
			   ((p->PackOffset) >> DATA_BIT);
	p->pData = p->pLine;  //get first byte
#if USE_BIT_REVERSE
	p->BitOffset = (INT8)(p->PackOffset & DATA_BITOFFSETMASK); //get first bit offset
	p->BitOffset = (INT8)(DATA_BITSIZE - p->BPP - p->BitOffset);
#else
	p->BitOffset = (INT8)(p->PackOffset & DATA_BITOFFSETMASK); //get first bit offset
#endif

#if 1
	if ((p->PackOffset >= 0) && (p->PackOffset < p->Limit)) {
		p->PackData = *(p->pData); //get first byte value
	} else {
		p->PackData = 0;
	}
#else
	p->PackData = *(p->pData); //get first byte value
#endif
	p->x = 0; //fix
}
static void PIXEL_2D_INDEXn_BeginPixel(PIXEL_2D *_p)
{
	PIXEL_2D_INDEXn *p = (PIXEL_2D_INDEXn *)_p;

#if USE_BIT_REVERSE
	p->BitOffsetFinal = p->BitOffset;
	if (p->bBitReverse) {
		p->BitOffsetFinal = (INT8)(DATA_BITSIZE - p->BPP - p->BitOffsetFinal);
	}
#else
	p->BitOffsetFinal = p->BitOffset;
#endif
}
static UINT32 PIXEL_2D_INDEXn_Read(PIXEL_2D *_p)
{
	PIXEL_2D_INDEXn *p = (PIXEL_2D_INDEXn *)_p;
	DATA_PACK value;

#if 1
	if (!((p->PackOffset >= 0) && (p->PackOffset < p->Limit))) {
		return 0;
	}
#endif
	value = (DATA_PACK)((p->PackData >> p->BitOffsetFinal) & p->Mask);          //get bit value

	return value;
}

static void PIXEL_2D_INDEXn_Write(PIXEL_2D *_p, UINT32 value)
{
	PIXEL_2D_INDEXn *p = (PIXEL_2D_INDEXn *)_p;

	value = (DATA_PACK)value;
	value &= p->Mask;  //clip value
#if 1
	if (!((p->PackOffset >= 0) && (p->PackOffset < p->Limit))) {
		return;
	}
#endif
	p->PackData &= ~(p->Mask << p->BitOffsetFinal);   //clear old bit value
	p->PackData |= (value << p->BitOffsetFinal);   //set new bit value

	p->bDirty = TRUE;
}
static void PIXEL_2D_INDEXn_EndPixel(PIXEL_2D *_p, BOOL bForceFlush, BOOL bNext)
{
	PIXEL_2D_INDEXn *p = (PIXEL_2D_INDEXn *)_p;

#if USE_BIT_REVERSE
	if (p->bDirty) {
		if ((p->BitOffset == 0) || (p->x == p->pRect->w - 1) || bForceFlush) {
			*(p->pData) = p->PackData; //put new byte data
			p->bDirty = FALSE;
		}
	}

	if (bNext) {
		if (p->BitOffset == 0) {
			p->pData ++;    //next byte
			p->PackData = *(p->pData); //get new byte data
			p->BitOffset = (INT8)(DATA_BITSIZE - p->BPP);    //reset bit offset
		} else {
			p->BitOffset -= (p->BPP);    //next bit offset
		}

		p->x++;
	}
#else
	if (p->bDirty) {
		if ((p->BitOffset == (DATA_BITSIZE - p->BPP)) || (p->x == p->pRect->w - 1) || bForceFlush) {
			*(p->pData) = p->PackData; //put new byte data
			p->bDirty = FALSE;
		}
	}

	if (bNext) {
		if (p->BitOffset == (DATA_BITSIZE - p->BPP)) {
			p->pData ++;    //next byte
			p->PackData = *(p->pData); //get new byte data
			p->BitOffset = (INT8) 0;
		} else {
			p->BitOffset += (p->BPP);
		}

		p->x++;
	}
#endif
}
static void PIXEL_2D_INDEXn_EndLine(PIXEL_2D *_p)
{
	PIXEL_2D_INDEXn *p = (PIXEL_2D_INDEXn *)_p;

	p->PackOffset += p->uiBitPitch;
	p->x = 0;
	p->y++;
}
static void PIXEL_2D_INDEXn_End(PIXEL_2D *_p)
{
}
static void PIXEL_2D_INDEXn_WriteOffset(PIXEL_2D *_p, UINT32 value, INT32 dx, INT32 dy)
{
	//NOTE: not support
}
static void PIXEL_2D_INDEXn_GotoXY(PIXEL_2D *_p, int x, int y)
{
	//NOTE: not support
}
static UINT8 *PIXEL_2D_INDEXn_LineBuf(struct _PIXEL_2D *p, int y)
{
	//NOTE: not support
	return 0;
}
static UINT8 *PIXEL_2D_INDEXn_PlaneBuf(struct _PIXEL_2D *p, int i)
{
	//NOTE: not support
	return 0;
}

PIXEL_2D _gPixel2D_INDEXn = {
	PIXEL_2D_INDEXn_Begin,
	PIXEL_2D_INDEXn_BeginLine,
	PIXEL_2D_INDEXn_BeginPixel,
	PIXEL_2D_INDEXn_Read,
	PIXEL_2D_INDEXn_Write,
	PIXEL_2D_INDEXn_EndPixel,
	PIXEL_2D_INDEXn_EndLine,
	PIXEL_2D_INDEXn_End,
	PIXEL_2D_INDEXn_WriteOffset,
	PIXEL_2D_INDEXn_GotoXY,
	PIXEL_2D_INDEXn_LineBuf,
	PIXEL_2D_INDEXn_PlaneBuf,
};


//--------------------------------------------------------------------------------------
//  PIXEL_2D
//--------------------------------------------------------------------------------------
typedef struct _PIXEL_2D_INDEX8 {
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
} PIXEL_2D_INDEX8;

static void PIXEL_2D_INDEX8_Begin(PIXEL_2D *_p, IRECT *pRect)
{
	PIXEL_2D_INDEX8 *p = (PIXEL_2D_INDEX8 *)_p;

	switch (p->pDC->uiType) {
	case TYPE_FB:
		p->uiPitch = p->pDC->blk[3].uiPitch;
		break;
	case TYPE_BITMAP:
		p->uiPitch = p->pDC->blk[3].uiPitch;
		break;
	case TYPE_ICON:
		p->uiPitch = p->pDC->blk[3].uiPitch;
		break;
	}

	p->pRect = pRect;

	p->uiOffset =
		p->pRect->y * (p->uiPitch) + p->pRect->x;

	p->x = 0;
	p->y = 0;
}
static void PIXEL_2D_INDEX8_BeginLine(PIXEL_2D *_p)
{
	PIXEL_2D_INDEX8 *p = (PIXEL_2D_INDEX8 *)_p;

	p->pLine = ((DATA_PACK *)(p->pDC->blk[3].uiOffset)) +
			   (p->uiOffset);
	p->x = 0; //fix
}
static void PIXEL_2D_INDEX8_BeginPixel(PIXEL_2D *_p)
{
	PIXEL_2D_INDEX8 *p = (PIXEL_2D_INDEX8 *)_p;

	p->pData = p->pLine + p->x;  //get first byte
}
static UINT32 PIXEL_2D_INDEX8_Read(PIXEL_2D *_p)
{
	PIXEL_2D_INDEX8 *p = (PIXEL_2D_INDEX8 *)_p;

	return (*(p->pData));
}
static void PIXEL_2D_INDEX8_Write(PIXEL_2D *_p, UINT32 value)
{
	PIXEL_2D_INDEX8 *p = (PIXEL_2D_INDEX8 *)_p;

	*(p->pData) = (DATA_PACK)value;
}
static void PIXEL_2D_INDEX8_EndPixel(PIXEL_2D *_p, BOOL bForceFlush, BOOL bNext)
{
	PIXEL_2D_INDEX8 *p = (PIXEL_2D_INDEX8 *)_p;

	if (bNext) {
		p->x++;
	}
}
static void PIXEL_2D_INDEX8_EndLine(PIXEL_2D *_p)
{
	PIXEL_2D_INDEX8 *p = (PIXEL_2D_INDEX8 *)_p;

	p->uiOffset += (p->uiPitch);
	p->x = 0;
	p->y++;
}
static void PIXEL_2D_INDEX8_End(PIXEL_2D *p)
{
}
static void PIXEL_2D_INDEX8_WriteOffset(PIXEL_2D *_p, UINT32 value, INT32 dx, INT32 dy)
{
	PIXEL_2D_INDEX8 *p = (PIXEL_2D_INDEX8 *)_p;

	//NOTE: boundary checking is ignored
	*(p->pData + (p->uiPitch)*dy + dx) = (DATA_PACK)value;
}
static void PIXEL_2D_INDEX8_GotoXY(PIXEL_2D *_p, int x, int y)
{
	PIXEL_2D_INDEX8 *p = (PIXEL_2D_INDEX8 *)_p;

	p->uiOffset =
		p->pRect->y * (p->uiPitch) + p->pRect->x;
	p->uiOffset += (p->uiPitch) * y;
	p->pLine = ((DATA_PACK *)(p->pDC->blk[3].uiOffset)) +
			   (p->uiOffset);
	p->x = x;
	p->y = y;
}
static UINT8 *PIXEL_2D_INDEX8_LineBuf(struct _PIXEL_2D *_p, int y)
{
	PIXEL_2D_INDEX8 *p = (PIXEL_2D_INDEX8 *)_p;

	//plane 0, line y
	return ((DATA_PACK *)(p->pDC->blk[3].uiOffset)) +
		   ((y) * (p->uiPitch));
}
static UINT8 *PIXEL_2D_INDEX8_PlaneBuf(struct _PIXEL_2D *_p, int i)
{
	PIXEL_2D_INDEX8 *p = (PIXEL_2D_INDEX8 *)_p;

	//plane i, line 0
	return ((DATA_PACK *)(p->pDC->blk[3].uiOffset));
}

PIXEL_2D _gPixel2D_INDEX8 = {
	PIXEL_2D_INDEX8_Begin,
	PIXEL_2D_INDEX8_BeginLine,
	PIXEL_2D_INDEX8_BeginPixel,
	PIXEL_2D_INDEX8_Read,
	PIXEL_2D_INDEX8_Write,
	PIXEL_2D_INDEX8_EndPixel,
	PIXEL_2D_INDEX8_EndLine,
	PIXEL_2D_INDEX8_End,
	PIXEL_2D_INDEX8_WriteOffset,
	PIXEL_2D_INDEX8_GotoXY,
	PIXEL_2D_INDEX8_LineBuf,
	PIXEL_2D_INDEX8_PlaneBuf,
};




