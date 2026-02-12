#include "GxGfx/GxGfx.h"
#include "DC.h"
#include "DC_Blt.h"
#include "GxGfx_int.h"
#include "GxPixel.h"
#include "GxGrph.h"

#define FORCE_SW_8565   0
#if !FORCE_SW_8565
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
//  include
//--------------------------------------------------------------------------------------



//modify from _DC_FillRect_INDEX_sw
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_PK2 format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by _pDestDC
RESULT _DC_FillRect_RGB565_sw(DC *_pDestDC, const IRECT *pDestRect,
							  UINT32 uiRop, COLOR_ITEM uiColor, const MAPPING_ITEM *pTable)
{
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT DestRect;
//#if USE_FAST_BLT //fast path (optimized by dword read/write)
#if 0 //fast path (optimized by dword read/write)
	UINT32 DestSHIFT, DestMASK;
#endif

	UINT32 rop = uiRop & ROP_MASK;

	RECT_SetRect(&DestRect, pDestRect);

	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_RGB565_PK :
		break;
	case PXLFMT_RGBA5654_PK :
		break;
	case PXLFMT_RGBA5658_PK :
		break;
	default: {
			DBG_ERR("_GxGfx_FillRect - ERROR! Not support this pDestDC format.\r\n");
			return GX_ERROR_FORMAT;
		}
	}

#if 0
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
#endif

//#if USE_FAST_BLT //fast path (optimized by dword read/write)
#if 0 //fast path (optimized by dword read/write)
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
			iDestPitch = (pDestDC->blk[3].uiWidth) << DestSHIFT;
			break;
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
				register dmask = DestMASK;
				register INT32 DestBPP = 1 << DestSHIFT;

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
			UINT32 rv = pTable[((UINT32)uiColor) & DestMASK];
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
				register dmask = DestMASK;
				register INT32 DestBPP = 1 << DestSHIFT;

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

// ROP_DESTNOT //////////////////////////////////////////////////////////////////////////////////

		if ((uiRop == ROP_DESTNOT)) {
			register INT32 w = (DestRect.w);
			register INT32 i;

			for (j = 0; j < h; j++) {
				register UINT32 d_dword;
				register UINT32 *pDestAddrDW;
				register INT32 di;
				INT32 iDestBits;
				register dmask = DestMASK;
				register INT32 DestBPP = 1 << DestSHIFT;

				//2.read dst_dword from dst line data
				iDestBits = (iDestBase + iDestOffset);
				pDestAddrDW = pDestBaseDW + (iDestBits >> 5);
				di = iDestBits & 0x01f; //pixel shift bits in dword

				//if(di)  //remove this line if current ROP need read DEST value
				{
					d_dword = (*pDestAddrDW);
				}

				for (i = 0; i < w; i++) {
					register UINT32 dm;
					register UINT32 dv, rv;

					//3.get dst pixel data from dst_buf
					dm = (dmask << di);
					dv = (d_dword & dm) >> di;

					//4.do ROP with src and dst pixel data, and put to result pixel data
					rv = (0xff ^ dv) & dmask;

					//5.put result pixel data to dst_dword
					d_dword = (d_dword & ~dm) | ((rv << di) & dm);

					di += DestBPP;
					if (di == 32) {
						//6.write dst_dword to dst_ptr
						(*pDestAddrDW) = d_dword;

						//read dst_dword from dst_ptr
						di = 0;
						pDestAddrDW++;

						d_dword = (*pDestAddrDW);  //add this line if current ROP need read DEST value
					}
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


//modify from _DC_BitBlt_INDEX
//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_PK2 format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_PK2 format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_BitBlt_RGB565(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
						 UINT32 uiRop, UINT32 uiParam)
{

#if (FORCE_SW_8565 == 1) //some project only use SW without any HW
		return _DC_BitBlt_RGB565_sw(_pDestDC, pDestRect, _pSrcDC, pSrcRect, uiRop, uiParam);
#else
	{

		DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
		DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
		IRECT DestRect;
		IRECT SrcRect;
		ER ERcode = 0;
		UINT32 cparam[1];
	    UINT32 rop = uiRop & ROP_MASK;

		RECT_SetRect(&DestRect, pDestRect);
		RECT_SetRect(&SrcRect, pSrcRect);

		//check rect zero
		if (_DC_IS_ZERO_RGB565(pDestDC, &DestRect)) {
			return GX_OK;    //skip this draw
		}
		if (_DC_IS_ZERO_RGB565(pSrcDC, &SrcRect)) {
			return GX_OK;    //skip this draw
		}

		switch (rop) {
		case ROP_COPY:
			cparam[0] = 0;
			break;
		case ROP_KEY:
			cparam[0] = COLOR_RGB565_GET_COLOR(uiParam);
			break;
		case ROP_DESTFILL:
			_pSrcDC = _pDestDC; //_pSrcDC=0, let _pSrcDC equal to _pDestDC
			//pDestRect = pSrcRect;   //pDestRect=0, let pDestRect equal to pSrcRect
			cparam[0] = _Repeat2Words(COLOR_RGB565_GET_COLOR(uiParam));
			break;
		default :
			return GX_NOTSUPPORT_ROP;
#if 0
		case ROP_BLEND:
#if 0
			param = ((uiRop & SRC_ALPHA_MASK) >> SRC_ALPHA_SHIFT);
			if (param == 7) {
				AOP = GRPH_CMD_A_COPY;
				cparam[0] = 0;
			} else {
				AOP = GRPH_CMD_BLENDING;
				param = ucWTable[param];
				cparam[0] = param;
				bSwap = TRUE;
			}
#else
			AOP = GRPH_CMD_PLANE_BLENDING;
			cparam[0] = 0;
#endif
			break;
#endif
		}
//  RGB operation

		_grph_setImage1(_pSrcDC, 0, &SrcRect);
		_grph_setImage2(_pDestDC, 0, &DestRect);

        if ((rop == ROP_COPY)||(rop == ROP_KEY)||(rop == ROP_BLEND)){
        	VF_GFX_COPY         param ={0};
	        HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};

            vir_meminfo.va = (void *)(_DC_bltSrcOffset);
            ERcode = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
        	if ( ERcode != HD_OK) {
        		DBG_ERR("_DC_bltSrcOffset map fail %d\n",ERcode);
        		return GX_OUTOF_MEMORY;
        	}
        	memset(&param, 0, sizeof(VF_GFX_COPY));
        	ERcode = vf_init(&param.src_img, _DC_bltSrcWidth, _DC_bltSrcHeight, HD_VIDEO_PXLFMT_RGB565, _DC_bltSrcPitch,vir_meminfo.pa, _DC_bltSrcPitch*_DC_bltSrcHeight);
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
        	ERcode = vf_init(&param.dst_img, _DC_bltDestWidth, _DC_bltDestHeight, HD_VIDEO_PXLFMT_RGB565, _DC_bltDestPitch, vir_meminfo.pa, _DC_bltDestPitch*_DC_bltDestHeight);
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
	        HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};
            vir_meminfo.va = (void *)(_DC_bltDestOffset);
            ERcode = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
        	if ( ERcode != HD_OK) {
        		DBG_ERR("_DC_bltDestOffset map fail %d\n",ERcode);
        		return GX_OUTOF_MEMORY;
			}
            memset(&param, 0, sizeof(VF_GFX_DRAW_RECT));
        	ERcode = vf_init(&param.dst_img, _DC_bltDestWidth, _DC_bltDestHeight, HD_VIDEO_PXLFMT_RGB565, _DC_bltDestPitch, vir_meminfo.pa, _DC_bltDestPitch*_DC_bltDestHeight);
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

/*
HW todo???

#define ROP_BLEND               0x14000000 //(Support YUV format only)  sub-type: SRC_ALPHA***

GxGfx_SetImageBlend(BlendOp, SrcFactor, DestFactor, OutMask);
BlendOp = BLENDOP_ADD;
SrcFactor = BLEND_SRCALPHA;
DestFactor = BLEND_INVSRCALPHA;
OutMask = OUTMASK_ALPHA|OUTMASK_RED|OUTMASK_GREEN|OUTMASK_BLUE;

BlendOp = BLENDOP_ADD;
SrcFactor = BLEND_BLENDFACTOR;
DestFactor = BLEND_INVBLENDFACTOR;
OutMask = OUTMASK_ALPHA|OUTMASK_RED|OUTMASK_GREEN|OUTMASK_BLUE;

GxGfx_SetImageStroke(ROP_BLEND, BlendFactor);
BlendFactor = 0;

//D3DRS_SRCBLEND
//D3DRS_DESTBLEND
#define BLEND_ZERO            1 //Blend factor is (0, 0, 0, 0).
#define BLEND_ONE             2 //Blend factor is (1, 1, 1, 1).

#define BLEND_SRCCOLOR        3 //Blend factor is (Rs, Gs, Bs, As).
#define BLEND_INVSRCCOLOR     4 //Blend factor is (1 - Rs, 1 - Gs, 1 - Bs, 1 - As)
#define BLEND_SRCALPHA        5 //Blend factor is (As, As, As, As)
#define BLEND_INVSRCALPHA     6 //Blend factor is (1 - As, 1 - As, 1 - As, 1 - As)

#define BLEND_DESTALPHA       7 //Blend factor is (Ad, Ad, Ad, Ad)
#define BLEND_INVDESTALPHA    8 //Blend factor is (1 - Ad, 1 - Ad, 1 - Ad, 1 - Ad).
#define BLEND_DESTCOLOR       9 //Blend factor is (Rd, Gd, Bd, Ad)
#define BLEND_INVDESTCOLOR    10//Blend factor is (1 - Rd, 1 - Gd, 1 - Bd, 1 - Ad)

//BLENDFACTOR = 0x000000ff in image stroke param
#define BLEND_BLENDFACTOR     14//Blend factor is (Ft, Ft, Ft, Ft)
#define BLEND_INVBLENDFACTOR  15//Blend factor is (1 - Ft, 1 - Ft, 1 - Ft, 1 - Ft)

//CONSTCOLOR = 0xaabbggrr in image stroke param
#define BLEND_CONSTCOLOR      16//Blend factor is (Rc, Gc, Bc, Ac)
#define BLEND_INVCONSTCOLOR   17//Blend factor is (1 - Rc, 1 - Gc, 1 - Bc, 1 - Ac)
#define BLEND_CONSTALPHA      18//Blend factor is (Ac, Ac, Ac, Ac)
#define BLEND_INVCONSTALPHA   19//Blend factor is (1 - Ac, 1 - Ac, 1 - Ac, 1 - Ac).

//D3DRS_BLENDOP
#define BLENDOP_ADD           1
#define BLENDOP_SUBTRACT      2
#define BLENDOP_REVSUBTRACT   3
#define BLENDOP_MIN           4
#define BLENDOP_MAX           5

//D3DRS_COLORWRITE
#define OUTMASK_RED             0x01
#define OUTMASK_GREEN           0x02
#define OUTMASK_BLUE            0x04
#define OUTMASK_ALPHA           0x08

//image rop - subtype for ROP_BLEND
#define SRC_ALPHA_012           0x00000000 //OUT= 0.88 x A + 0.12 x B
#define SRC_ALPHA_025           0x00010000 //OUT= 0.75 x A + 0.25 x B
#define SRC_ALPHA_038           0x00020000 //OUT= 0.62 x A + 0.38 x B
#define SRC_ALPHA_050           0x00030000 //OUT= 0.50 x A + 0.50 x B
#define SRC_ALPHA_062           0x00040000 //OUT= 0.38 x A + 0.62 x B
#define SRC_ALPHA_075           0x00050000 //OUT= 0.25 x A + 0.75 x B
#define SRC_ALPHA_088           0x00060000 //OUT= 0.50 x A + 0.88 x B
#define SRC_ALPHA_100           0x00070000 //OUT= B
#define SRC_ALPHA_MASK          0x00070000
#define SRC_ALPHA_SHIFT         16

*/

static UINT32 RGB565_OpAlphaBlending(UINT32 SrcValue, UINT32 DestValue)
{
	UINT32 OutValue;
	CVALUE r1, g1, b1, a1;
	CVALUE r2, g2, b2, a2;
	CVALUE r3, g3, b3, a3;

	//================================
	//pixel op: blend
	OutValue = 0;

	a1 = COLOR_RGB565_GET_A(SrcValue);
	r1 = COLOR_RGB565_GET_R(SrcValue);
	g1 = COLOR_RGB565_GET_G(SrcValue);
	b1 = COLOR_RGB565_GET_B(SrcValue);
	a2 = COLOR_RGB565_GET_A(DestValue);
	r2 = COLOR_RGB565_GET_R(DestValue);
	g2 = COLOR_RGB565_GET_G(DestValue);
	b2 = COLOR_RGB565_GET_B(DestValue);
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
	OutValue = COLOR_MAKE_RGB565(r3, g3, b3, a3);
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
RESULT _DC_BitBlt_RGB565_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							UINT32 uiRop, UINT32 uiParam)
{
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT DestRect;
	IRECT SrcRect;

	UINT32 rop = uiRop & ROP_MASK;
	UINT32 colorkey = 0;

	if (rop == ROP_DESTFILL) {
		return _DC_FillRect_RGB565_sw(_pDestDC, pSrcRect, uiRop, uiParam, 0);
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
	if (_DC_IS_ZERO_RGB565(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}
	if (_DC_IS_ZERO_RGB565(pSrcDC, &SrcRect)) {
		return GX_OK;    //skip this draw
	}
	/*
	    if( (uiRop == ROP_COPY) && (pTable==0) &&
	        (pDestDC->uiPxlfmt == pSrcDC->uiPxlfmt) &&
	        (pDestDC->uiType == TYPE_FB || pDestDC->uiType == TYPE_BITMAP) &&
	        (pSrcDC->uiType == TYPE_FB || pSrcDC->uiType == TYPE_BITMAP))
	    {
	        UINT8 SrcCheckWidthBitOffset = (UINT8)( (SrcRect.w << SrcSHIFT) & DATA_BITOFFSETMASK );
	        UINT8 DestCheckWidthBitOffset = (UINT8)( (DestRect.w << DestSHIFT) & DATA_BITOFFSETMASK );

	        if((SrcRect.x==0) && !SrcCheckWidthBitOffset &&
	            (DestRect.x==0) && !DestCheckWidthBitOffset)
	        {
	            //fast path for copy between INDEX/INDEX format with the same size

	            UINT8* pDestAddr = (UINT8*)(pDestDC->blk[3].uiOffset) + pDestDC->blk[3].uiPitch*DestRect.y;
	            UINT8* pSrcAddr = (UINT8*)(pSrcDC->blk[3].uiOffset) + pSrcDC->blk[3].uiPitch*SrcRect.y;
	            UINT32 uiWidth = (SrcRect.w  << SrcSHIFT) >> DATA_BIT;
	            //use HW path
	            if( _DC_RectMemCopy(
	                    pDestAddr,
	                    (INT16)pDestDC->blk[3].uiPitch,
	                    pSrcAddr,
	                    (INT16)pSrcDC->blk[3].uiPitch,
	                    (UINT16)uiWidth, (UINT16)SrcRect.h) != GX_OK)
	            {
	                //use SW path
	                return _RectMemCopy(
	                    pDestAddr,
	                    (INT16)pDestDC->blk[3].uiPitch,
	                    pSrcAddr,
	                    (INT16)pSrcDC->blk[3].uiPitch,
	                    (UINT16)uiWidth, (UINT16)SrcRect.h);
	            }
	            return GX_OK;
	        }
	    }
	*/
//#if USE_FAST_BLT //fast path (optimized by dword read/write)
#if 0 //fast path (optimized by dword read/write)
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
#if (GX_HW == GX_HW_NT964XX)
			//NT96433, NT96210 and Before
			iSrcPitch = (pSrcDC->blk[3].uiWidth) << SrcSHIFT;
#else
			//NT96630, NT96450 and After
			if (SrcSHIFT == 3) {
				iSrcPitch = GxGfx_CalcDCPitchSize(pSrcDC->blk[3].uiWidth, TYPE_ICON, pSrcDC->uiPxlfmt) << SrcSHIFT;
			} else {
				iSrcPitch = (pSrcDC->blk[3].uiWidth) << SrcSHIFT;
			}
#endif
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
#if (GX_HW == GX_HW_NT964XX)
			//NT96433, NT96210 and Before
			iDestPitch = (pDestDC->blk[3].uiWidth) << DestSHIFT;
#else
			//NT96630, NT96450 and After
			if (DestSHIFT == 3) {
				iDestPitch = GxGfx_CalcDCPitchSize(pDestDC->blk[3].uiWidth, TYPE_ICON, pDestDC->uiPxlfmt) << DestSHIFT;
			} else {
				iDestPitch = (pDestDC->blk[3].uiWidth) << DestSHIFT;
			}
#endif
			break;
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

// ROP_DESTKEY //////////////////////////////////////////////////////////////////////////////////

		else if ((uiRop == ROP_DESTKEY) && (!pTable)) {
			for (j = 0; j < h; j++) {
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
					d_dword = (*pDestAddrDW);
				}

				for (i = 0; i < w; i++) {
					register UINT32 sm, dm;
					register UINT32 sv, dv, rv;
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
					dv = (d_dword & dm) >> di;

					//4.do ROP with src and dst pixel data, and put to result pixel data
					rv = sv;
					if (dv != colorkey) {
						//5.put result pixel data to dst_dword
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

						d_dword = (*pDestAddrDW);  //add this line if current ROP need read DEST value
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

// ROP_DESTKEY + Mapping //////////////////////////////////////////////////////////////////////////////////

		else if ((uiRop == ROP_DESTKEY) && (pTable)) {
			for (j = 0; j < h; j++) {
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
					d_dword = (*pDestAddrDW);
				}

				for (i = 0; i < w; i++) {
					register UINT32 sm, dm;
					register UINT32 sv, dv, rv;
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
					dv = (d_dword & dm) >> di;

					//4.do ROP with src and dst pixel data, and put to result pixel data
					rv = pTable[sv];
					if (dv != colorkey) {
						//5.put result pixel data to dst_dword
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

						d_dword = (*pDestAddrDW);  //add this line if current ROP need read DEST value
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

// ROP other //////////////////////////////////////////////////////////////////////////////////

		else {
			for (j = 0; j < h; j++) {
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
					d_dword = (*pDestAddrDW);
				}

				for (i = 0; i < w; i++) {
					register UINT32 sm, dm;
					register UINT32 sv, dv, rv;
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
					dv = (d_dword & dm) >> di;

					//4.do ROP with src and dst pixel data, and put to result pixel data
					if (rop == ROP_NOT) {
						rv = 0xFF ^ sv; //not
					} else if (rop == ROP_AND) {
						rv = dv & sv; //and
					} else if (rop == ROP_OR) {
						rv = dv | sv; //or
					} else if (rop == ROP_XOR) {
						rv = dv ^ sv; //xor
					}

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

						d_dword = (*pDestAddrDW);  //add this line if current ROP need read DEST value
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

	}

#else //slow path

	//RGB565 -> RGB565
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
				BlendValue = RGB565_OpAlphaBlending(SrcValue, DestValue);
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

#endif //path

}


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
	UINT32 DestAlpha;

	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_RGB565_PK :
		DestAlpha = 0;
		break;
	case PXLFMT_RGBA5658_PK :
		DestAlpha = 8;
		break;
	case PXLFMT_RGBA5654_PK :
		DestAlpha = 4;
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pDestDC format. %x\r\n", pDestDC->uiPxlfmt);
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
	if (_DC_IS_ZERO_RGB565(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}
	if (DestAlpha) {
		if (_DC_IS_ZERO_ALPHA(pDestDC, &DestRect)) {
			return GX_OK;    //skip this draw
		}
	}
	//INDEX1 -> PXLFMT_RGB565_PK
	//INDEX1 -> PXLFMT_RGBA5658_PK
	//INDEX1 -> PXLFMT_RGBA5654_PK
	//INDEX2 -> PXLFMT_RGB565_PK
	//INDEX2 -> PXLFMT_RGBA5658_PK
	//INDEX2 -> PXLFMT_RGBA5654_PK
	//INDEX4 -> PXLFMT_RGB565_PK
	//INDEX4 -> PXLFMT_RGBA5658_PK
	//INDEX4 -> PXLFMT_RGBA5654_PK
	//INDEX8 -> PXLFMT_RGB565_PK
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
						DestValue = 0x00ffffff;
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
				if (COLOR_RGB565_GET_COLOR(SrcValue) == COLOR_RGB565_GET_COLOR(colorkey)) {
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
RESULT _DC_RotateBlt_RGB565(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
							UINT32 uiRop, UINT32 uiParam)
{
#if (FORCE_SW_8565 == 1)
	DBG_ERR("Not support RGB565 Rotate!\r\n");
    return GX_NOTSUPPORT_ROP;
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
// RGB operation
	_grph_setImage1(_pSrcDC, 0, &SrcRect);
	_grph_setImage2(_pDestDC, 0, &DestRect);

    {
    	VF_GFX_ROTATE         param ={0};
		HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};

		vir_meminfo.va = (void *)(_DC_bltSrcOffset);
		ERcode = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
		if ( ERcode != HD_OK) {
			DBG_ERR("_DC_bltSrcOffset map fail %d\n",ERcode);
			return GX_OUTOF_MEMORY;
		}

    	memset(&param, 0, sizeof(VF_GFX_ROTATE));
    	ERcode = vf_init(&param.src_img, _DC_bltSrcWidth, _DC_bltSrcHeight, HD_VIDEO_PXLFMT_RGB565, _DC_bltSrcPitch, vir_meminfo.pa, _DC_bltSrcPitch*_DC_bltSrcHeight);
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
    	ERcode = vf_init(&param.dst_img, _DC_bltDestWidth, _DC_bltDestHeight, HD_VIDEO_PXLFMT_RGB565, _DC_bltDestPitch, vir_meminfo.pa, _DC_bltDestPitch*_DC_bltDestHeight);
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
    	if(ERcode){
    		DBG_ERR("fail to rotate %d\n",ERcode);
    		return ERcode;
    	}

    }

	return ERcode;
#endif
}

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_RGB565 format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_FontBlt_INDEX2RGB565_sw(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
								   const MAPPING_ITEM *pTable)
{
	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	IRECT DestRect;
	IRECT SrcRect;

	UINT32 DestAlpha;

	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_RGB565_PK :
		DestAlpha = 0;
		break;
	case PXLFMT_RGBA5658_PK :
		DestAlpha = 8;
		break;
	case PXLFMT_RGBA5654_PK :
		DestAlpha = 4;
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
	if (_DC_IS_ZERO_RGB565(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}
	if (DestAlpha) {
		if (_DC_IS_ZERO_ALPHA(pDestDC, &DestRect)) {
			return GX_OK;    //skip this draw
		}
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
		#if 0
		MAPPING_ITEM BackColor;
		MAPPING_ITEM FaceColor;
		MAPPING_ITEM ShadowColor;
		MAPPING_ITEM LineColor;
		BackColor = pTable[0];
		FaceColor = pTable[1];
		ShadowColor = pTable[2];
		LineColor = pTable[3];
		#endif
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
				#if 0
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
				#else
				DestValue= pTable[SrcValue];
				#endif
				if (DestValue != (COLOR_TRANSPARENT&0x0000ffff)) {
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
//condition : _pDestDC is PXLFMT_P1_PK2 format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_IDX format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_StretchBlt_INDEX2RGB565_sw
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
	const PALETTE_ITEM *pColorTable = pc->pColorTable;
	const MAPPING_ITEM *pIndexTable = pc->pIndexTable;

	PIXEL_2D DestPx = {0};
	PIXEL_2D SrcPx = {0};
	UINT32 rop = pc->uiRop & ROP_MASK;
	UINT32 colorkey;
	UINT32 DestAlpha;


	int sw, sh, dw, dh;

	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_RGB565_PK :
		DestAlpha = 0;
		break;
	case PXLFMT_RGBA5658_PK :
		DestAlpha = 8;
		break;
	case PXLFMT_RGBA5654_PK :
		DestAlpha = 4;
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
	if (_DC_IS_ZERO_RGB565(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}
	if (DestAlpha) {
		if (_DC_IS_ZERO_ALPHA(pDestDC, &DestRect)) {
			return GX_OK;    //skip this draw
		}
	}

	sw = SrcRect.w;
	sh = SrcRect.h;
	dw = DestRect.w;
	dh = DestRect.h;

	//INDEX1 -> PXLFMT_RGB565_PK
	//INDEX1 -> PXLFMT_RGBA5658_PK
	//INDEX1 -> PXLFMT_RGBA5654_PK
	//INDEX2 -> PXLFMT_RGB565_PK
	//INDEX2 -> PXLFMT_RGBA5658_PK
	//INDEX2 -> PXLFMT_RGBA5654_PK
	//INDEX4 -> PXLFMT_RGB565_PK
	//INDEX4 -> PXLFMT_RGBA5658_PK
	//INDEX4 -> PXLFMT_RGBA5654_PK
	//INDEX8 -> PXLFMT_RGB565_PK
	//INDEX8 -> PXLFMT_RGBA5658_PK
	//INDEX8 -> PXLFMT_RGBA5654_PK
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
						DestValue = 0x00ffffff;
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
						SrcValue = 0x00ffffff;
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
						SrcValue = 0x00ffffff;
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
				AccValue_A += COLOR_RGB565_GET_A(SrcValue) * (dw - xE);
				AccValue_R += COLOR_RGB565_GET_R(SrcValue) * (dw - xE);
				AccValue_G += COLOR_RGB565_GET_G(SrcValue) * (dw - xE);
				AccValue_B += COLOR_RGB565_GET_B(SrcValue) * (dw - xE);
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

						AccValue_A += COLOR_RGB565_GET_A(SrcValue) * dw;
						AccValue_R += COLOR_RGB565_GET_R(SrcValue) * dw;
						AccValue_G += COLOR_RGB565_GET_G(SrcValue) * dw;
						AccValue_B += COLOR_RGB565_GET_B(SrcValue) * dw;
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

							AccValue_A += COLOR_RGB565_GET_A(SrcValue) * dw;
							AccValue_R += COLOR_RGB565_GET_R(SrcValue) * dw;
							AccValue_G += COLOR_RGB565_GET_G(SrcValue) * dw;
							AccValue_B += COLOR_RGB565_GET_B(SrcValue) * dw;
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

					AccValue_A += COLOR_RGB565_GET_A(SrcValue) * xE;
					AccValue_R += COLOR_RGB565_GET_R(SrcValue) * xE;
					AccValue_G += COLOR_RGB565_GET_G(SrcValue) * xE;
					AccValue_B += COLOR_RGB565_GET_B(SrcValue) * xE;
					AccCnt += xE;
				} else {
					bLoad = 0;
				}

				//copy
				AccValue = COLOR_MAKE_RGB565(
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

//condition : _pDestDC is not null
//condition : _pDestDC is PXLFMT_P1_PK2 format
//condition : _pSrcDC is not null
//condition : _pSrcDC is PXLFMT_P1_PK2 format
//condition : pDestRect is not null
//condition : pDestRect is validated and already clipped by pSrcRect
//condition : pSrcRect is not null
//condition : pSrcRect is validated and already clipped by pDestRect
RESULT _DC_StretchBlt_RGB565
(DC *_pDestDC, const IRECT *pDestRect, const DC *_pSrcDC, const IRECT *pSrcRect,
 DC_CTRL *pc)
{
#if (FORCE_SW_8565 == 1) //some project only use SW without any HW

    return _DC_StretchBlt_RGB565_sw(_pDestDC,pDestRect,_pSrcDC,pSrcRect,pc);

#else

	DISPLAY_CONTEXT *pSrcDC = (DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;

	IRECT DestRect;
	IRECT SrcRect;

	//UINT32 colorkey;
	//UINT32 rop = pc->uiRop & ROP_MASK;
	UINT32 DestAlpha;
	UINT32 SrcAlpha;

	RESULT ERcode;

	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_RGB565_PK :
		DestAlpha = 0;
		break;
	case PXLFMT_RGBA5658_PK :
		DestAlpha = 8;
		break;
	case PXLFMT_RGBA5654_PK :
		DestAlpha = 4;
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pDestDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}
	switch (pSrcDC->uiPxlfmt) {
	case PXLFMT_RGB565_PK :
		SrcAlpha = 0;
		break;
	case PXLFMT_RGBA5658_PK :
		SrcAlpha = 8;
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pSrcDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}

	RECT_SetRect(&DestRect, pDestRect);
	RECT_SetRect(&SrcRect, pSrcRect);

	//check rect zero
	if (_DC_IS_ZERO_RGB565(pSrcDC, &SrcRect)) {
		return GX_OK;    //skip this draw
	}
	if (SrcAlpha) {
		if (_DC_IS_ZERO_ALPHA(pSrcDC, &SrcRect)) {
			return GX_OK;    //skip this draw
		}
	}
	//check rect zero
	if (_DC_IS_ZERO_RGB565(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}
	if (DestAlpha) {
		if (_DC_IS_ZERO_ALPHA(pDestDC, &DestRect)) {
			return GX_OK;    //skip this draw
		}
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
// for RGB operation

		//fmt: 3=INDEX, 1=YUVPACK, 2=YUVPlaner, 0=ARGB
		_ime_setImage1(_pSrcDC, 0, &SrcRect);
		_ime_setImage2(_pDestDC, 0, &DestRect);


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
    	ERcode = vf_init(&param.src_img, _DC_SclSrcWidth, _DC_SclSrcHeight, HD_VIDEO_PXLFMT_RGB565, _DC_SclSrcPitch,vir_meminfo.pa, _DC_SclSrcPitch*_DC_SclSrcHeight);
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
    	ERcode = vf_init(&param.dst_img, _DC_SclDestWidth, _DC_SclDestHeight, HD_VIDEO_PXLFMT_RGB565, _DC_SclDestPitch, vir_meminfo.pa, _DC_SclDestPitch*_DC_SclDestHeight);
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
    	if(ERcode){
    		DBG_ERR("fail to scale %d\n",ERcode);
    		return ERcode;
    	}

    	}

// alph operation

		//fmt: 3=INDEX, 1=YUVPACK, 2=YUVPlaner, 0=ARGB
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


    }

	return ERcode;
#endif
}


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
	UINT32 DestAlpha;
	UINT32 SrcAlpha;


	int sw, sh, dw, dh;

	switch (pDestDC->uiPxlfmt) {
	case PXLFMT_RGB565_PK :
		DestAlpha = 0;
		break;
	case PXLFMT_RGBA5658_PK :
		DestAlpha = 8;
		break;
	case PXLFMT_RGBA5654_PK :
		DestAlpha = 4;
		break;
	default: {
			DBG_ERR("_GxGfx_BitBlt - ERROR! Not support this pDestDC format.\r\n");
			return GX_CANNOT_CONVERT;
		}
	}
	switch (pSrcDC->uiPxlfmt) {
	case PXLFMT_RGB565_PK :
		SrcAlpha = 0;
		break;
	case PXLFMT_RGBA5658_PK :
		SrcAlpha = 8;
		break;
	case PXLFMT_RGBA5654_PK :
		SrcAlpha = 4;
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
	if (_DC_IS_ZERO_RGB565(pSrcDC, &SrcRect)) {
		return GX_OK;    //skip this draw
	}
	if (SrcAlpha) {
		if (_DC_IS_ZERO_ALPHA(pSrcDC, &SrcRect)) {
			return GX_OK;    //skip this draw
		}
	}
	//check rect zero
	if (_DC_IS_ZERO_RGB565(pDestDC, &DestRect)) {
		return GX_OK;    //skip this draw
	}
	if (DestAlpha) {
		if (_DC_IS_ZERO_ALPHA(pDestDC, &DestRect)) {
			return GX_OK;    //skip this draw
		}
	}

	sw = SrcRect.w;
	sh = SrcRect.h;
	dw = DestRect.w;
	dh = DestRect.h;

	//PXLFMT_RGB565_PK -> PXLFMT_RGB565_PK
	//PXLFMT_RGB565_PK -> PXLFMT_RGBA5658_PK
	//PXLFMT_RGB565_PK -> PXLFMT_RGBA5654_PK
	//PXLFMT_RGBA5658_PK -> PXLFMT_RGB565_PK
	//PXLFMT_RGBA5658_PK -> PXLFMT_RGBA5658_PK
	//PXLFMT_RGBA5658_PK -> PXLFMT_RGBA5654_PK
	//PXLFMT_RGBA5654_PK -> PXLFMT_RGB565_PK
	//PXLFMT_RGBA5654_PK -> PXLFMT_RGBA5658_PK
	//PXLFMT_RGBA5654_PK -> PXLFMT_RGBA5654_PK
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
				AccValue_A += COLOR_RGB565_GET_A(SrcValue) * (dw - xE);
				AccValue_R += COLOR_RGB565_GET_R(SrcValue) * (dw - xE);
				AccValue_G += COLOR_RGB565_GET_G(SrcValue) * (dw - xE);
				AccValue_B += COLOR_RGB565_GET_B(SrcValue) * (dw - xE);
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

						AccValue_A += COLOR_RGB565_GET_A(SrcValue) * dw;
						AccValue_R += COLOR_RGB565_GET_R(SrcValue) * dw;
						AccValue_G += COLOR_RGB565_GET_G(SrcValue) * dw;
						AccValue_B += COLOR_RGB565_GET_B(SrcValue) * dw;
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

							AccValue_A += COLOR_RGB565_GET_A(SrcValue) * dw;
							AccValue_R += COLOR_RGB565_GET_R(SrcValue) * dw;
							AccValue_G += COLOR_RGB565_GET_G(SrcValue) * dw;
							AccValue_B += COLOR_RGB565_GET_B(SrcValue) * dw;
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

					AccValue_A += COLOR_RGB565_GET_A(SrcValue) * xE;
					AccValue_R += COLOR_RGB565_GET_R(SrcValue) * xE;
					AccValue_G += COLOR_RGB565_GET_G(SrcValue) * xE;
					AccValue_B += COLOR_RGB565_GET_B(SrcValue) * xE;
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
					UINT32 dst_a = COLOR_RGB565_GET_A(DestValue);
					UINT32 dst_r = COLOR_RGB565_GET_R(DestValue);
					UINT32 dst_g = COLOR_RGB565_GET_G(DestValue);
					UINT32 dst_b = COLOR_RGB565_GET_B(DestValue);
					if (dst_a == 0) {
						dst_a = src_a;
						dst_r = src_a * src_r / 255;
						dst_g = src_a * src_g / 255;
						dst_b = src_a * src_b / 255;
					} else if (dst_a == 255) {
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
					AccValue = COLOR_MAKE_RGB565(
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
					UINT32 dst_a = COLOR_RGB565_GET_A(DestValue);
					UINT32 dst_r = COLOR_RGB565_GET_R(DestValue) * dst_a / 255;
					UINT32 dst_g = COLOR_RGB565_GET_G(DestValue) * dst_a / 255;
					UINT32 dst_b = COLOR_RGB565_GET_B(DestValue) * dst_a / 255;
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
					AccValue = COLOR_MAKE_RGB565(
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
				AccValue = COLOR_MAKE_RGB565(
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



//--------------------------------------------------------------------------------------
//  PIXEL_2D
//--------------------------------------------------------------------------------------
typedef struct _PIXEL_2D_RGB565 {
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
} PIXEL_2D_RGB565;

static void PIXEL_2D_RGB565_Begin(PIXEL_2D *_p, IRECT *pRect)
{
	PIXEL_2D_RGB565 *p = (PIXEL_2D_RGB565 *)_p;

	switch (p->pDC->uiType) {
	case TYPE_FB:
		p->uiPitch = p->pDC->blk[0].uiPitch;
		break;
	case TYPE_BITMAP:
		p->uiPitch = p->pDC->blk[0].uiPitch;
		break;
	case TYPE_ICON:
		p->uiPitch = p->pDC->blk[0].uiPitch;
		break;
	}

	p->pRect = pRect;

	p->uiOffset =
		p->pRect->y * (p->uiPitch) + (p->pRect->x << 1);

	p->x = 0;
	p->y = 0;
}
static void PIXEL_2D_RGB565_BeginLine(PIXEL_2D *_p)
{
	PIXEL_2D_RGB565 *p = (PIXEL_2D_RGB565 *)_p;

	p->pLine = ((DATA_PACK *)(p->pDC->blk[0].uiOffset)) +
			   (p->uiOffset);
	p->x = 0; //fix
}
static void PIXEL_2D_RGB565_BeginPixel(PIXEL_2D *_p)
{
	PIXEL_2D_RGB565 *p = (PIXEL_2D_RGB565 *)_p;

	p->pData = p->pLine + (p->x << 1); //get first byte
}
static UINT32 PIXEL_2D_RGB565_Read(PIXEL_2D *_p)
{
	PIXEL_2D_RGB565 *p = (PIXEL_2D_RGB565 *)_p;
	UINT32 pdata = 0;
	pdata |= ((*(UINT8 *)(p->pData)) << 0);
	pdata |= ((*(UINT8 *)(p->pData + 1)) << 8);
	return pdata;
}
static void PIXEL_2D_RGB565_Write(PIXEL_2D *_p, UINT32 value)
{
	PIXEL_2D_RGB565 *p = (PIXEL_2D_RGB565 *)_p;

	*(p->pData) = (DATA_PACK)((value & 0x000000ff) >> 0);
	*(p->pData + 1) = (DATA_PACK)((value & 0x0000ff00) >> 8);
}
static void PIXEL_2D_RGB565_EndPixel(PIXEL_2D *_p, BOOL bForceFlush, BOOL bNext)
{
	PIXEL_2D_RGB565 *p = (PIXEL_2D_RGB565 *)_p;

	if (bNext) {
		p->x++;
	}
}
static void PIXEL_2D_RGB565_EndLine(PIXEL_2D *_p)
{
	PIXEL_2D_RGB565 *p = (PIXEL_2D_RGB565 *)_p;

	p->uiOffset += (p->uiPitch);
	p->x = 0;
	p->y++;
}
static void PIXEL_2D_RGB565_End(PIXEL_2D *p)
{
}
static void PIXEL_2D_RGB565_WriteOffset(PIXEL_2D *_p, UINT32 value, INT32 dx, INT32 dy)
{
	PIXEL_2D_RGB565 *p = (PIXEL_2D_RGB565 *)_p;

	//NOTE: boundary checking is ignored
	*(UINT16 *)(p->pData + (p->uiPitch)*dy + (dx << 1)) = (UINT16)(value & 0x0000ffff);
}
static void PIXEL_2D_RGB565_GotoXY(PIXEL_2D *_p, int x, int y)
{
	PIXEL_2D_RGB565 *p = (PIXEL_2D_RGB565 *)_p;

	p->uiOffset =
		p->pRect->y * (p->uiPitch) + (p->pRect->x << 1);
	p->uiOffset += (p->uiPitch) * y;
	p->pLine = ((DATA_PACK *)(p->pDC->blk[0].uiOffset)) +
			   (p->uiOffset);
	p->x = x;
	p->y = y;
}

static UINT8 *PIXEL_2D_RGB565_LineBuf(struct _PIXEL_2D *_p, int y)
{
	PIXEL_2D_RGB565 *p = (PIXEL_2D_RGB565 *)_p;

	//plane 0, line y
	return ((DATA_PACK *)(p->pDC->blk[3].uiOffset)) +
		   ((y) * (p->uiPitch));
}
static UINT8 *PIXEL_2D_RGB565_PlaneBuf(struct _PIXEL_2D *_p, int i)
{
	PIXEL_2D_RGB565 *p = (PIXEL_2D_RGB565 *)_p;

	//plane i, line 0
	return ((DATA_PACK *)(p->pDC->blk[3].uiOffset));
}


PIXEL_2D _gPixel2D_RGB565 = {
	PIXEL_2D_RGB565_Begin,
	PIXEL_2D_RGB565_BeginLine,
	PIXEL_2D_RGB565_BeginPixel,
	PIXEL_2D_RGB565_Read,
	PIXEL_2D_RGB565_Write,
	PIXEL_2D_RGB565_EndPixel,
	PIXEL_2D_RGB565_EndLine,
	PIXEL_2D_RGB565_End,
	PIXEL_2D_RGB565_WriteOffset,
	PIXEL_2D_RGB565_GotoXY,
	PIXEL_2D_RGB565_LineBuf,
	PIXEL_2D_RGB565_PlaneBuf,
};






typedef struct _PIXEL_2D_RGBA5658 {
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

	INT32 uiOffset2;
	INT32 uiPitch2;

	DATA_PACK *pLine2;
	DATA_PACK *pData2;
} PIXEL_2D_RGBA5658;

static void PIXEL_2D_RGBA5658_Begin(PIXEL_2D *_p, IRECT *pRect)
{
	PIXEL_2D_RGBA5658 *p = (PIXEL_2D_RGBA5658 *)_p;

	switch (p->pDC->uiType) {
	case TYPE_FB:
		p->uiPitch = p->pDC->blk[0].uiPitch;
		p->uiPitch2 = p->pDC->blk[3].uiPitch;
		break;
	case TYPE_BITMAP:
		p->uiPitch = p->pDC->blk[0].uiPitch;
		p->uiPitch2 = p->pDC->blk[3].uiPitch;
		break;
	case TYPE_ICON:
		p->uiPitch = p->pDC->blk[0].uiPitch;
		p->uiPitch2 = p->pDC->blk[3].uiPitch;
		break;
	}

	p->pRect = pRect;

	p->uiOffset =
		p->pRect->y * (p->uiPitch) + (p->pRect->x << 1);
	p->uiOffset2 =
		p->pRect->y * (p->uiPitch2) + p->pRect->x;

	p->x = 0;
	p->y = 0;
}
static void PIXEL_2D_RGBA5658_BeginLine(PIXEL_2D *_p)
{
	PIXEL_2D_RGBA5658 *p = (PIXEL_2D_RGBA5658 *)_p;

	p->pLine = ((DATA_PACK *)(p->pDC->blk[0].uiOffset)) +
			   (p->uiOffset);
	p->pLine2 = ((DATA_PACK *)(p->pDC->blk[3].uiOffset)) +
				(p->uiOffset2);
	p->x = 0; //fix
}
static void PIXEL_2D_RGBA5658_BeginPixel(PIXEL_2D *_p)
{
	PIXEL_2D_RGBA5658 *p = (PIXEL_2D_RGBA5658 *)_p;

	p->pData = p->pLine + (p->x << 1); //get first byte
	p->pData2 = p->pLine2 + p->x;  //get first byte
}
static UINT32 PIXEL_2D_RGBA5658_Read(PIXEL_2D *_p)
{
	PIXEL_2D_RGBA5658 *p = (PIXEL_2D_RGBA5658 *)_p;
	UINT32 pdata = 0;
	pdata |= ((*(UINT8 *)(p->pData)) << 0);
	pdata |= ((*(UINT8 *)(p->pData + 1)) << 8);
	pdata |= (((*(p->pData2)) & 0x000000ff) << 16);
	return pdata;
}
static void PIXEL_2D_RGBA5658_Write(PIXEL_2D *_p, UINT32 value)
{
	PIXEL_2D_RGBA5658 *p = (PIXEL_2D_RGBA5658 *)_p;

	*(p->pData) = (DATA_PACK)((value & 0x000000ff) >> 0);
	*(p->pData + 1) = (DATA_PACK)((value & 0x0000ff00) >> 8);
	*(p->pData2) = (DATA_PACK)((value & 0x00ff0000) >> 16);
}
static void PIXEL_2D_RGBA5658_EndPixel(PIXEL_2D *_p, BOOL bForceFlush, BOOL bNext)
{
	PIXEL_2D_RGBA5658 *p = (PIXEL_2D_RGBA5658 *)_p;

	if (bNext) {
		p->x++;
	}
}
static void PIXEL_2D_RGBA5658_EndLine(PIXEL_2D *_p)
{
	PIXEL_2D_RGBA5658 *p = (PIXEL_2D_RGBA5658 *)_p;

	p->uiOffset += (p->uiPitch);
	p->uiOffset2 += (p->uiPitch2);
	p->x = 0;
	p->y++;
}
static void PIXEL_2D_RGBA5658_End(PIXEL_2D *p)
{
}
static void PIXEL_2D_RGBA5658_WriteOffset(PIXEL_2D *_p, UINT32 value, INT32 dx, INT32 dy)
{
	PIXEL_2D_RGBA5658 *p = (PIXEL_2D_RGBA5658 *)_p;

	//NOTE: boundary checking is ignored
	*(UINT16 *)(p->pData + (p->uiPitch)*dy + (dx << 1)) = (UINT16)(value & 0x0000ffff);
	*(p->pData2 + (p->uiPitch2)*dy + dx) = (DATA_PACK)((value & 0x00ff0000) >> 16);
}
static void PIXEL_2D_RGBA5658_GotoXY(PIXEL_2D *_p, int x, int y)
{
	PIXEL_2D_RGBA5658 *p = (PIXEL_2D_RGBA5658 *)_p;

	p->uiOffset =
		p->pRect->y * (p->uiPitch) + (p->pRect->x << 1);
	p->uiOffset2 =
		p->pRect->y * (p->uiPitch2) + p->pRect->x;
	p->uiOffset += (p->uiPitch) * y;
	p->uiOffset2 += (p->uiPitch2) * y;
	p->pLine = ((DATA_PACK *)(p->pDC->blk[0].uiOffset)) +
			   (p->uiOffset);
	p->pLine2 = ((DATA_PACK *)(p->pDC->blk[3].uiOffset)) +
				(p->uiOffset2);
	p->x = x;
	p->y = y;
}
//for coverity,open it,but not verify
static UINT8 *PIXEL_2D_RGBA5658_LineBuf(struct _PIXEL_2D *_p, int y)
{
	PIXEL_2D_RGBA5658 *p = (PIXEL_2D_RGBA5658 *)_p;

	//plane 0, line y
	return ((DATA_PACK *)(p->pDC->blk[3].uiOffset)) +
		   ((y) * (p->uiPitch));
}
static UINT8 *PIXEL_2D_RGBA5658_PlaneBuf(struct _PIXEL_2D *_p, int i)
{
	PIXEL_2D_RGBA5658 *p = (PIXEL_2D_RGBA5658 *)_p;

	//plane i, line 0
	return ((DATA_PACK *)(p->pDC->blk[3].uiOffset));
}


PIXEL_2D _gPixel2D_RGBA5658 = {
	PIXEL_2D_RGBA5658_Begin,
	PIXEL_2D_RGBA5658_BeginLine,
	PIXEL_2D_RGBA5658_BeginPixel,
	PIXEL_2D_RGBA5658_Read,
	PIXEL_2D_RGBA5658_Write,
	PIXEL_2D_RGBA5658_EndPixel,
	PIXEL_2D_RGBA5658_EndLine,
	PIXEL_2D_RGBA5658_End,
	PIXEL_2D_RGBA5658_WriteOffset,
	PIXEL_2D_RGBA5658_GotoXY,
	PIXEL_2D_RGBA5658_LineBuf,
	PIXEL_2D_RGBA5658_PlaneBuf,
};



typedef struct _PIXEL_2D_RGBA5654 {
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

	INT32 uiOffset2;
	INT32 uiPitch2;

	DATA_PACK *pLine2;
	DATA_PACK *pData2;
} PIXEL_2D_RGBA5654;

static void PIXEL_2D_RGBA5654_Begin(PIXEL_2D *_p, IRECT *pRect)
{
	PIXEL_2D_RGBA5654 *p = (PIXEL_2D_RGBA5654 *)_p;

	switch (p->pDC->uiType) {
	case TYPE_FB:
		p->uiPitch = p->pDC->blk[0].uiPitch;
		p->uiPitch2 = p->pDC->blk[3].uiPitch;
		break;
	case TYPE_BITMAP:
		p->uiPitch = p->pDC->blk[0].uiPitch;
		p->uiPitch2 = p->pDC->blk[3].uiPitch;
		break;
	case TYPE_ICON:
		p->uiPitch = p->pDC->blk[0].uiPitch;
		p->uiPitch2 = p->pDC->blk[3].uiPitch;
		break;
	}

	p->pRect = pRect;

	p->uiOffset =
		p->pRect->y * (p->uiPitch) + (p->pRect->x << 1);
	p->uiOffset2 =
		p->pRect->y * (p->uiPitch2) + (p->pRect->x >> 1);

	p->x = 0;
	p->y = 0;
}
static void PIXEL_2D_RGBA5654_BeginLine(PIXEL_2D *_p)
{
	PIXEL_2D_RGBA5654 *p = (PIXEL_2D_RGBA5654 *)_p;

	p->pLine = ((DATA_PACK *)(p->pDC->blk[0].uiOffset)) +
			   (p->uiOffset);
	p->pLine2 = ((DATA_PACK *)(p->pDC->blk[3].uiOffset)) +
				(p->uiOffset2);
	p->x = 0; //fix
}
static void PIXEL_2D_RGBA5654_BeginPixel(PIXEL_2D *_p)
{
	PIXEL_2D_RGBA5654 *p = (PIXEL_2D_RGBA5654 *)_p;

	p->pData = p->pLine + (p->x << 1); //get first byte
	p->pData2 = p->pLine2 + (p->x >> 1); //get first byte
}
static UINT32 PIXEL_2D_RGBA5654_Read(PIXEL_2D *_p)
{
	PIXEL_2D_RGBA5654 *p = (PIXEL_2D_RGBA5654 *)_p;
	UINT32 pdata = 0;
	pdata |= ((*(UINT8 *)(p->pData)) << 0);
	pdata |= ((*(UINT8 *)(p->pData + 1)) << 8);
	if (p->x & 1) {
		pdata |= ((((*(p->pData2)) & 0xf0) >> 4) << 20);
	} else {
		pdata |= ((((*(p->pData2)) & 0x0f) >> 0) << 20);
	}
	return pdata;
}
static void PIXEL_2D_RGBA5654_Write(PIXEL_2D *_p, UINT32 value)
{
	PIXEL_2D_RGBA5654 *p = (PIXEL_2D_RGBA5654 *)_p;

	*(p->pData) = (DATA_PACK)((value & 0x000000ff) >> 0);
	*(p->pData + 1) = (DATA_PACK)((value & 0x0000ff00) >> 8);
	if (p->x & 1) {
		*(p->pData2) |= (DATA_PACK)(((value & 0x00f00000) >> 20) << 4);
	} else {
		*(p->pData2) |= (DATA_PACK)(((value & 0x00f00000) >> 20) << 0);
	}
}
static void PIXEL_2D_RGBA5654_EndPixel(PIXEL_2D *_p, BOOL bForceFlush, BOOL bNext)
{
	PIXEL_2D_RGBA5654 *p = (PIXEL_2D_RGBA5654 *)_p;

	if (bNext) {
		p->x++;
	}
}
static void PIXEL_2D_RGBA5654_EndLine(PIXEL_2D *_p)
{
	PIXEL_2D_RGBA5654 *p = (PIXEL_2D_RGBA5654 *)_p;

	p->uiOffset += (p->uiPitch);
	p->uiOffset2 += (p->uiPitch2);
	p->x = 0;
	p->y++;
}
static void PIXEL_2D_RGBA5654_End(PIXEL_2D *p)
{
}
static void PIXEL_2D_RGBA5654_WriteOffset(PIXEL_2D *_p, UINT32 value, INT32 dx, INT32 dy)
{
	PIXEL_2D_RGBA5654 *p = (PIXEL_2D_RGBA5654 *)_p;

	//NOTE: boundary checking is ignored
	*(UINT16 *)(p->pData + (p->uiPitch)*dy + (dx << 1)) = (UINT16)(value & 0x0000ffff);
	if (p->x & 1) {
		*(p->pData2 + (p->uiPitch2)*dy + (dx >> 1)) |= (DATA_PACK)(((value & 0x00f00000) >> 20) << 4);
	} else {
		*(p->pData2 + (p->uiPitch2)*dy + (dx >> 1)) |= (DATA_PACK)(((value & 0x00f00000) >> 20) << 0);
	}
}
static void PIXEL_2D_RGBA5654_GotoXY(PIXEL_2D *_p, int x, int y)
{
	PIXEL_2D_RGBA5654 *p = (PIXEL_2D_RGBA5654 *)_p;

	p->uiOffset =
		p->pRect->y * (p->uiPitch) + (p->pRect->x << 1);
	p->uiOffset2 =
		p->pRect->y * (p->uiPitch2) + (p->pRect->x >> 1);
	p->uiOffset += (p->uiPitch) * y;
	p->uiOffset2 += (p->uiPitch2) * y;
	p->pLine = ((DATA_PACK *)(p->pDC->blk[0].uiOffset)) +
			   (p->uiOffset);
	p->pLine2 = ((DATA_PACK *)(p->pDC->blk[3].uiOffset)) +
				(p->uiOffset2);
	p->x = x;
	p->y = y;
}
//for coverity,open it,but not verify
static UINT8 *PIXEL_2D_RGBA5654_LineBuf(struct _PIXEL_2D *_p, int y)
{
	PIXEL_2D_RGBA5654 *p = (PIXEL_2D_RGBA5654 *)_p;

	//plane 0, line y
	return ((DATA_PACK *)(p->pDC->blk[3].uiOffset)) +
		   ((y) * (p->uiPitch));
}
static UINT8 *PIXEL_2D_RGBA5654_PlaneBuf(struct _PIXEL_2D *_p, int i)
{
	PIXEL_2D_RGBA5654 *p = (PIXEL_2D_RGBA5654 *)_p;

	//plane i, line 0
	return ((DATA_PACK *)(p->pDC->blk[3].uiOffset));
}

PIXEL_2D _gPixel2D_RGBA5654 = {
	PIXEL_2D_RGBA5654_Begin,
	PIXEL_2D_RGBA5654_BeginLine,
	PIXEL_2D_RGBA5654_BeginPixel,
	PIXEL_2D_RGBA5654_Read,
	PIXEL_2D_RGBA5654_Write,
	PIXEL_2D_RGBA5654_EndPixel,
	PIXEL_2D_RGBA5654_EndLine,
	PIXEL_2D_RGBA5654_End,
	PIXEL_2D_RGBA5654_WriteOffset,
	PIXEL_2D_RGBA5654_GotoXY,
	PIXEL_2D_RGBA5654_LineBuf,
	PIXEL_2D_RGBA5654_PlaneBuf,
};

#if (GX_SUPPPORT_YUV ==ENABLE)

//condition : _pDestDC is not null
//condition : _pSrcDC is not null
//condition : _pDestDC is PXLFMT_RGB888 format and _pSrcDC is PXLFMT_RGB888_PK/PXLFMT_RGBD8888_PK format
//        or, _pDestDC is PXLFMT_RGB888_PK/PXLFMT_RGBD8888_PK format and _pSrcDC is PXLFMT_RGB888 format
RESULT _DC_ConvetBlt_RGBToRGB_sw(DC *_pDestDC, const DC *_pSrcDC)
{
	const DISPLAY_CONTEXT *pSrcDC = (const DISPLAY_CONTEXT *)_pSrcDC;
	DISPLAY_CONTEXT *pDestDC = (DISPLAY_CONTEXT *)_pDestDC;
	register LVALUE i, j;
	LVALUE w, h;
	UINT8 *rgbpk_buf;
	UINT8 *r_buf;
	UINT8 *g_buf;
	UINT8 *b_buf;
	LVALUE rgbpk_pitch;
	LVALUE r_pitch;
	LVALUE g_pitch;
	LVALUE b_pitch;

	if ((pSrcDC->uiPxlfmt == PXLFMT_RGB888) && (pDestDC->uiPxlfmt == PXLFMT_RGB888_PK)) {
		w = pSrcDC->size.w;
		h = pSrcDC->size.h;
		rgbpk_pitch = pDestDC->blk[3].uiPitch;
		rgbpk_buf = (UINT8 *)(pDestDC->blk[3].uiOffset);
		/*
		        //reverse scan order of y
		        rgbpk_buf += (rgbpk_pitch*(h-1));
		        rgbpk_pitch = -rgbpk_pitch;
		*/
		r_pitch = pSrcDC->blk[0].uiPitch;
		r_buf = (UINT8 *)(pSrcDC->blk[0].uiOffset);
		g_pitch = pSrcDC->blk[1].uiPitch;
		g_buf = (UINT8 *)(pSrcDC->blk[1].uiOffset);
		b_pitch = pSrcDC->blk[2].uiPitch;
		b_buf = (UINT8 *)(pSrcDC->blk[2].uiOffset);

		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
//              UINT8* rgbpk_pk = rgbpk_buf;
				UINT8 *rgbpk_pk = &(rgbpk_buf[i * 3]);
				rgbpk_pk[0] = b_buf[i];
				rgbpk_pk[1] = g_buf[i];
				rgbpk_pk[2] = r_buf[i];
				//rgbpk_pk += 3;
			}
			rgbpk_buf += rgbpk_pitch;
			r_buf += r_pitch;
			g_buf += g_pitch;
			b_buf += b_pitch;
		}
		return GX_OK;
	}

	if ((pSrcDC->uiPxlfmt == PXLFMT_RGB888) && (pDestDC->uiPxlfmt == PXLFMT_RGBD8888_PK)) {
		w = pSrcDC->size.w;
		h = pSrcDC->size.h;
		rgbpk_pitch = pDestDC->blk[3].uiPitch;
		rgbpk_buf = (UINT8 *)(pDestDC->blk[3].uiOffset);
		/*
		        //reverse scan order of y
		        rgbpk_buf += (rgbpk_pitch*(h-1));
		        rgbpk_pitch = -rgbpk_pitch;
		*/
		r_pitch = pSrcDC->blk[0].uiPitch;
		r_buf = (UINT8 *)(pSrcDC->blk[0].uiOffset);
		g_pitch = pSrcDC->blk[1].uiPitch;
		g_buf = (UINT8 *)(pSrcDC->blk[1].uiOffset);
		b_pitch = pSrcDC->blk[2].uiPitch;
		b_buf = (UINT8 *)(pSrcDC->blk[2].uiOffset);

		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
//              UINT8* rgbpk_pk = rgbpk_buf;
				UINT8 *rgbpk_pk = &(rgbpk_buf[i * 4]);
				rgbpk_pk[0] = b_buf[i];
				rgbpk_pk[1] = g_buf[i];
				rgbpk_pk[2] = r_buf[i];
				//rgbpk_pk += 3;
			}
			rgbpk_buf += rgbpk_pitch;
			r_buf += r_pitch;
			g_buf += g_pitch;
			b_buf += b_pitch;
		}
		return GX_OK;
	}

	else if ((pSrcDC->uiPxlfmt == PXLFMT_RGB888_PK) && (pDestDC->uiPxlfmt == PXLFMT_RGB888)) {
		w = pSrcDC->size.w;
		h = pSrcDC->size.h;
		rgbpk_pitch = pSrcDC->blk[3].uiPitch;
		rgbpk_buf = (UINT8 *)(pSrcDC->blk[3].uiOffset);
		/*      //reverse scan order of y
		        rgbpk_buf += (rgbpk_pitch*(h-1));
		        rgbpk_pitch = -rgbpk_pitch;
		*/
		r_pitch = pDestDC->blk[0].uiPitch;
		r_buf = (UINT8 *)(pDestDC->blk[0].uiOffset);
		g_pitch = pDestDC->blk[1].uiPitch;
		g_buf = (UINT8 *)(pDestDC->blk[1].uiOffset);
		b_pitch = pDestDC->blk[2].uiPitch;
		b_buf = (UINT8 *)(pDestDC->blk[2].uiOffset);

		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
//              UINT8* rgbpk_pk = rgbpk_buf;
				UINT8 *rgbpk_pk = &(rgbpk_buf[i * 3]);
				b_buf[i] = rgbpk_pk[0];
				g_buf[i] = rgbpk_pk[1];
				r_buf[i] = rgbpk_pk[2];
				//rgbpk_pk += 3;
			}
			rgbpk_buf += rgbpk_pitch;
			r_buf += r_pitch;
			g_buf += g_pitch;
			b_buf += b_pitch;
		}
		return GX_OK;
	}

	else if ((pSrcDC->uiPxlfmt == PXLFMT_RGBD8888_PK) && (pDestDC->uiPxlfmt == PXLFMT_RGB888)) {
		w = pSrcDC->size.w;
		h = pSrcDC->size.h;
		rgbpk_pitch = pSrcDC->blk[3].uiPitch;
		rgbpk_buf = (UINT8 *)(pSrcDC->blk[3].uiOffset);
		/*      //reverse scan order of y
		        rgbpk_buf += (rgbpk_pitch*(h-1));
		        rgbpk_pitch = -rgbpk_pitch;
		*/
		r_pitch = pDestDC->blk[0].uiPitch;
		r_buf = (UINT8 *)(pDestDC->blk[0].uiOffset);
		g_pitch = pDestDC->blk[1].uiPitch;
		g_buf = (UINT8 *)(pDestDC->blk[1].uiOffset);
		b_pitch = pDestDC->blk[2].uiPitch;
		b_buf = (UINT8 *)(pDestDC->blk[2].uiOffset);

		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
//              UINT8* rgbpk_pk = rgbpk_buf;
				UINT8 *rgbpk_pk = &(rgbpk_buf[i * 4]);
				b_buf[i] = rgbpk_pk[0];
				g_buf[i] = rgbpk_pk[1];
				r_buf[i] = rgbpk_pk[2];
				//rgbpk_pk += 3;
			}
			rgbpk_buf += rgbpk_pitch;
			r_buf += r_pitch;
			g_buf += g_pitch;
			b_buf += b_pitch;
		}
		return GX_OK;
	}

	DBG_ERR("_GxGfx_Convert - ERROR! Not support with this pSrcDC/pDestDC format.\r\n");
	return GX_CANNOT_CONVERT;   //not support
}
#endif

