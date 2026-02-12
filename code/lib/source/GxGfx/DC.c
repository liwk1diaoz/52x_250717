
#include "GxGfx/GxGfx.h"
#include "DC.h"
#include "GxGfx_int.h"

//====================
//  API
//====================
//dc attach & detach
RESULT GxGfx_AttachDC(DC *_pDC,
					  UINT16 uiType, UINT16 uiPxlfmt,
					  UINT32 uiWidth, UINT32 uiHeight,
					  UINT32 uiPitch, UINT8 *pBuf0, UINT8 *pBuf1, UINT8 *pBuf2);
void   GxGfx_DetachDC(DC *_pDC);

//dc size and rect
ISIZE GxGfx_GetSize(const DC *pDC);
IRECT GxGfx_GetRect(const DC *pDC);
//dc origin point
RESULT GxGfx_SetOrigin(DC *pDC, LVALUE x, LVALUE y);
IPOINT GxGfx_GetOrigin(const DC *pDC);
//dc clipping window
RESULT GxGfx_SetWindow(DC *pDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2);
IRECT GxGfx_GetWindow(const DC *pDC);
//dc move
RESULT GxGfx_MoveTo(DC *pDestDC, LVALUE x, LVALUE y);
RESULT GxGfx_MoveBack(DC *pDestDC);
IPOINT GxGfx_GetPos(const DC *pDestDC);
ISIZE GxGfx_GetLastMove(const DC *pDestDC);

//--------------------------------------------------------------------------------------
//  move
//--------------------------------------------------------------------------------------
RESULT GxGfx_MoveTo(DC *pDestDC, LVALUE x, LVALUE y)
{
	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	pDestDC->lastPt = pDestDC->thisPt;
	POINT_Set(&(pDestDC->thisPt), x, y);
	return GX_OK;
}
RESULT GxGfx_MoveBack(DC *pDestDC)
{
	IPOINT tmpPt;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	tmpPt = pDestDC->lastPt;
	pDestDC->lastPt = pDestDC->thisPt;
	pDestDC->thisPt = tmpPt;
	return GX_OK;
}
IPOINT GxGfx_GetPos(const DC *pDestDC)
{
	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		IPOINT thisPt;
		POINT_Set(&thisPt, 0, 0);
		return thisPt;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	return pDestDC->thisPt;
}
ISIZE GxGfx_GetLastMove(const DC *pDestDC)
{
	ISIZE MoveSz;
	DBG_FUNC_BEGIN("\r\n");
	if (!pDestDC) {
		SIZE_Set(&MoveSz, 0, 0);
		return MoveSz;
	}
	GXGFX_ASSERT((pDestDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);
	MoveSz.w = pDestDC->thisPt.x - pDestDC->lastPt.x;
	MoveSz.h = pDestDC->thisPt.y - pDestDC->lastPt.y;
	return MoveSz;
}


//--------------------------------------------------------------------------------------
//  DC
//--------------------------------------------------------------------------------------

UINT32 GxGfx_GetPixelSize(UINT16 uiPxlfmt, UINT8 iCn)
{
	DBG_FUNC_BEGIN("\r\n");
	switch (uiPxlfmt) {
	case PXLFMT_INDEX1:
	case PXLFMT_INDEX2:
	case PXLFMT_INDEX4:
	case PXLFMT_INDEX8:
		return 1;
#if (GX_SUPPPORT_YUV ==ENABLE)
	case PXLFMT_YUV444:
	case PXLFMT_YUV422:
	case PXLFMT_YUV421:
	case PXLFMT_YUV420:
	case PXLFMT_RGB888:
		return 1;
#if (GX_SUPPORT_DCFMT_UVPACK)
	case PXLFMT_YUV422_PK:
	case PXLFMT_YUV421_PK:
	case PXLFMT_YUV420_PK:
		if (iCn == 0) { //Y
			return 1;
		} else if (iCn == 1) { //UV
			return 1;
		} else {
			return 0;
		}
#endif
#endif
	case PXLFMT_RGB888_PK:
		return 3;
	case PXLFMT_RGBD8888_PK:
	case PXLFMT_RGBA8888_PK:
		return 4;

#if (GX_SUPPORT_DCFMT_RGB444)
	case PXLFMT_RGBA4444_PK:
		return 2;
	case PXLFMT_RGBA5551_PK:
		return 2;
#endif
#if (GX_SUPPORT_DCFMT_RGB565)
	case PXLFMT_RGB565_PK:
		return 2;
	case PXLFMT_RGBA5658_PK:
	case PXLFMT_RGBA5654_PK:
		if (iCn == 0) { //RGB
			return 2;
		} else if (iCn == 3) { //Alpha
			return 1;
		} else {
			return 0;
		}
#endif
	}
	return 0;
}

UINT32 GxGfx_CalcDCPitchSize(UINT32 w, UINT16 uiType, UINT16 uiPxlfmt)
{
	UINT32 uiPitch = 0;
	BOOL bICON = (uiType == TYPE_ICON);
	UINT8 uiAlignByte = (UINT8)((bICON) ? 1 : 4);
	DBG_FUNC_BEGIN("\r\n");
	switch (uiPxlfmt) {
	case PXLFMT_INDEX1:
		if ((uiAlignByte == 1) && (w < 8)) {
			w = 8;
		}
		uiPitch = MAKE_ALIGN(w >> 3, uiAlignByte);
		break;
	case PXLFMT_INDEX2:
		if ((uiAlignByte == 1) && (w < 4)) {
			w = 4;
		}
		uiPitch = MAKE_ALIGN(w >> 2, uiAlignByte);
		break;
	case PXLFMT_INDEX4:
		if ((uiAlignByte == 1) && (w < 2)) {
			w = 2;
		}
		uiPitch = MAKE_ALIGN(w >> 1, uiAlignByte);
		break;
	case PXLFMT_INDEX8:
		uiPitch = MAKE_ALIGN(w, 4);
		break;
#if (GX_SUPPPORT_YUV ==ENABLE)
	case PXLFMT_YUV444:
		uiPitch = MAKE_ALIGN(w, 4);
		break;
	case PXLFMT_YUV422:
		uiPitch = MAKE_ALIGN(w, 4);
		break;
	case PXLFMT_YUV421:
		uiPitch = MAKE_ALIGN(w, 4);
		break;
	case PXLFMT_YUV420:
		uiPitch = MAKE_ALIGN(w, 4);
		break;
	case PXLFMT_RGB888:
		uiPitch = MAKE_ALIGN(w, 4);
		break;
#if (GX_SUPPORT_DCFMT_UVPACK)
	case PXLFMT_YUV422_PK:
	case PXLFMT_YUV421_PK:
	case PXLFMT_YUV420_PK:
		uiPitch = MAKE_ALIGN(w, 4);
		break;
#endif
#endif
	case PXLFMT_RGB888_PK:
		uiPitch = MAKE_ALIGN(w * 3, 4);
		break;
	case PXLFMT_RGBD8888_PK:
		uiPitch = MAKE_ALIGN(w * 4, 4);
		break;
	case PXLFMT_RGBA8888_PK:
		uiPitch = MAKE_ALIGN(w * 4, 4);
		break;
#if (GX_SUPPORT_DCFMT_RGB444)
	case PXLFMT_RGBA4444_PK:
		uiPitch = MAKE_ALIGN(w * 2, 4);
		break;
	case PXLFMT_RGBA5551_PK:
		uiPitch = MAKE_ALIGN(w * 2, 4);
		break;
#endif

#if (GX_SUPPORT_DCFMT_RGB565)
	case PXLFMT_RGB565_PK:
		uiPitch = MAKE_ALIGN(w * 2, 4);
		break;
	case PXLFMT_RGBA5658_PK:
	case PXLFMT_RGBA5654_PK:
		uiPitch = MAKE_ALIGN(w * 2, 4);
		break;
#endif

	}
	return uiPitch;
}

UINT32 GxGfx_CalcDCMemSize(UINT32 w, UINT32 h, UINT16 uiType, UINT16 uiPxlfmt)
{
	UINT32 uiPitch = GxGfx_CalcDCPitchSize(w, uiType, uiPxlfmt);
	DBG_IND("uiPitch  %d\r\n", uiPitch);
	DBG_FUNC_BEGIN("\r\n");
	switch (uiPxlfmt) {
	case PXLFMT_INDEX1:
	case PXLFMT_INDEX2:
	case PXLFMT_INDEX4:
	case PXLFMT_INDEX8:
		return uiPitch * h;
#if (GX_SUPPPORT_YUV ==ENABLE)
	case PXLFMT_YUV444:
		return uiPitch * h * 3 + 64; //reserved for address alignment (for JPG HW)
	case PXLFMT_YUV422:
		return uiPitch * h * 2 + 64; //reserved for address alignment (for JPG HW)
	case PXLFMT_YUV421:
		return uiPitch * h * 2 + 64; //reserved for address alignment (for JPG HW)
	case PXLFMT_YUV420:
		return (uiPitch * h * 3 / 2) + 64; //reserved for address alignment (for JPG HW)
	case PXLFMT_RGB888:
		return uiPitch * h * 3;
#if (GX_SUPPORT_DCFMT_UVPACK)
	case PXLFMT_YUV422_PK:
		return uiPitch * h * 2;
	case PXLFMT_YUV421_PK:
		return uiPitch * h * 2;
	case PXLFMT_YUV420_PK:
		return (uiPitch * h * 3 / 2);
#endif
#endif
	case PXLFMT_RGB888_PK:
	case PXLFMT_RGBD8888_PK:
	case PXLFMT_RGBA8888_PK:
	case PXLFMT_RGB565_PK:
#if (GX_SUPPORT_DCFMT_RGB444)
	case PXLFMT_RGBA4444_PK:
	case PXLFMT_RGBA5551_PK:
#endif
		return uiPitch * h;
#if (GX_SUPPORT_DCFMT_RGB565)
	case PXLFMT_RGBA5658_PK:
		return uiPitch * h + GxGfx_CalcDCPitchSize(w, uiType, PXLFMT_INDEX8) * h;
	case PXLFMT_RGBA5654_PK:
		return uiPitch * h + GxGfx_CalcDCPitchSize(w, uiType, PXLFMT_INDEX4) * h;
#endif
	}
	return 0;
}
#if 0 //not use now
static RESULT _DC_Lock(DC *_pDC, UINT8 uiLockFmt)
{
	DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;
	//UINT16 uiDevFmt;
	DBG_FUNC_BEGIN("\r\n");
	if (!pDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	if (pDC->uiType == TYPE_NULL) {
		return GX_ERROR_TYPE;
	}

	if (pDC->uiFlag & DCFLAG_LOCK) {
		return GX_ERROR_LOCK;
	}

	if (pDC->uiPxlfmt != uiLockFmt) {
		//uiDevFmt = (UINT16)(pDC->uiReserved[1]);

		//check support format convert
		switch (uiLockFmt) {
		case PXLFMT_YUV444:
		case PXLFMT_YUV422:
		case PXLFMT_YUV421:
		case PXLFMT_YUV420:
			break;

		case PXLFMT_RGB888:
			break;

		case PXLFMT_RGB888_PK:
		case PXLFMT_RGBD8888_PK:
#if (GX_SUPPORT_DCFMT_RGB565)
		case PXLFMT_RGB565_PK:
#endif
			break;
		}
//        pTempBuf = new mem;
//        convert pSurfaceBuf(Pxlfmt) to pTempBuf(uiPxlfmt);
//        pBuf = pTempBuf;
	} else {
//        pTexture = (LPDIRECT3DTEXTURE9)(pImage->uiReserved[0]);

		switch (uiLockFmt) {
		case PXLFMT_YUV444:
		case PXLFMT_YUV422:
		case PXLFMT_YUV421:
		case PXLFMT_YUV420:
			break;

		case PXLFMT_RGB888:
			break;

		case PXLFMT_RGB888_PK:
		case PXLFMT_RGBD8888_PK:
#if (GX_SUPPORT_DCFMT_RGB565)
		case PXLFMT_RGB565_PK:
#endif
			/*
			            //block 0
			            pDC->blk[0].uiWidth = pDC->uiWidth;
			            pDC->blk[0].uiHeight = pDC->uiHeight;
			            pDC->blk[0].uiPitch = d3dlr.Pitch;
			            pDC->blk[0].uiOffset = (UINT32)d3dlr.pBits;
			*/
			break;
		}
	}

	pDC->uiReserved[1] |= (uiLockFmt << 16);
	pDC->uiFlag |= DCFLAG_LOCK;
	return GX_OK;
}

static RESULT _DC_Unlock(DC *_pDC)
{
	//UINT16 uiDevFmt;
	UINT16 uiLockFmt;
	DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;
	DBG_FUNC_BEGIN("\r\n");
	if (!pDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	if (pDC->uiType == TYPE_NULL) {
		return GX_ERROR_TYPE;
	}

	if (!(pDC->uiFlag & DCFLAG_LOCK)) {
		return GX_ERROR_LOCK;
	}

	uiLockFmt = (UINT16)((pDC->uiReserved[1] >> 16));

	if (pDC->uiPxlfmt != uiLockFmt) {
		//uiDevFmt = (UINT16)(pDC->uiReserved[1] & 0x0000ffff);

		//check support format convert
		switch (uiLockFmt) {
		case PXLFMT_YUV444:
		case PXLFMT_YUV422:
		case PXLFMT_YUV421:
		case PXLFMT_YUV420:
			break;

		case PXLFMT_RGB888:
			break;

		case PXLFMT_RGB888_PK:
		case PXLFMT_RGBD8888_PK:
#if (GX_SUPPORT_DCFMT_RGB565)
		case PXLFMT_RGB565_PK:
#endif
			//TODO:
			//convert RGB bitmap to YUV by IME.
			break;
		}
//        convert pTempBuf(USER_EDIT_FORMAT) to pSurfaceBuf(Pxlfmt);
//        free mem (pTempBuf);
//        pTempBuf = 0;
	} else {
//        pTexture = (LPDIRECT3DTEXTURE9)(pDC->uiReserved[0]);

		switch (uiLockFmt) {
		case PXLFMT_YUV444:
		case PXLFMT_YUV422:
		case PXLFMT_YUV421:
		case PXLFMT_YUV420:
			break;

		case PXLFMT_RGB888:
			break;

		case PXLFMT_RGB888_PK:
		case PXLFMT_RGBD8888_PK:
#if (GX_SUPPORT_DCFMT_RGB565)
		case PXLFMT_RGB565_PK:
#endif

			//block 0
			pDC->blk[0].uiWidth = 0;
			pDC->blk[0].uiHeight = 0;
			pDC->blk[0].uiPitch = 0;
			pDC->blk[0].uiOffset = 0;

			break;
		}
	}

	pDC->uiReserved[1] &= ~0xffff0000;
	pDC->uiFlag &= ~DCFLAG_LOCK;
	return GX_OK;
}
#endif
RESULT GxGfx_CheckDCTypeFmt(UINT16 uiType, UINT16 uiPxlfmt)
{
	RESULT r = GX_OK;

	DBG_FUNC_BEGIN("\r\n");
	switch (uiType) {
	case TYPE_NULL:
	case TYPE_FB:
	case TYPE_BITMAP:
	case TYPE_ICON:
	case TYPE_JPEG:
		break;
	default:
		DBG_ERR("GxGfx_CheckTypeFmt - ERROR! DC type is invalid.\r\n");
		r = GX_ERROR_TYPE;
		break;
	}

	switch (uiPxlfmt) {
	case PXLFMT_INDEX1:
	case PXLFMT_INDEX2:
	case PXLFMT_INDEX4:
	case PXLFMT_INDEX8:
		switch (uiType) {
		case TYPE_NULL:
		case TYPE_FB:
		case TYPE_BITMAP:
		case TYPE_ICON:
			break;
		default:
			DBG_ERR("GxGfx_CheckTypeFmt - ERROR! DC type/format is not supported.\r\n");
			r = GX_ERROR_TYPE;
			break;
		}
		break;
#if (GX_SUPPPORT_YUV ==ENABLE)
	case PXLFMT_YUV444:
	case PXLFMT_YUV422:
	case PXLFMT_YUV421:
	case PXLFMT_YUV420:
		//NOTE: not support TYPE_ICON
		switch (uiType) {
		case TYPE_NULL:
		case TYPE_FB:
		case TYPE_BITMAP:
		case TYPE_JPEG:
			break;
		default:
			DBG_ERR("GxGfx_CheckTypeFmt - ERROR! DC type/format is not supported.\r\n");
			r = GX_ERROR_TYPE;
			break;
		}
		break;
	case PXLFMT_RGB888:
		//NOTE: not support TYPE_ICON
		switch (uiType) {
		case TYPE_NULL:
		case TYPE_FB:
		case TYPE_BITMAP:
			break;
		default:
			DBG_ERR("GxGfx_CheckTypeFmt - ERROR! DC type/format is not supported.\r\n");
			r = GX_ERROR_TYPE;
			break;
		}
		break;
#endif
	case PXLFMT_RGB888_PK:
	case PXLFMT_RGBD8888_PK:
		//NOTE: not support TYPE_FB,TYPE_ICON
		switch (uiType) {
		case TYPE_NULL:
		case TYPE_BITMAP:
			break;
		default:
			DBG_ERR("GxGfx_CheckTypeFmt - ERROR! DC type/format is not supported.\r\n");
			r = GX_ERROR_TYPE;
			break;
		}
		break;
	case PXLFMT_RGBA8888_PK:
#if (GX_SUPPORT_DCFMT_RGB444)
	case PXLFMT_RGBA4444_PK:
	case PXLFMT_RGBA5551_PK:
#endif
		switch (uiType) {
		case TYPE_NULL:
		case TYPE_FB:
		case TYPE_BITMAP:
		case TYPE_ICON:
			break;
		default:
			DBG_ERR("GxGfx_CheckTypeFmt - ERROR! DC type/format is not supported.\r\n");
			r = GX_ERROR_TYPE;
			break;
		}
		break;
#if (GX_SUPPORT_DCFMT_RGB565)
	case PXLFMT_RGB565_PK:
	case PXLFMT_RGBA5658_PK:
		//case PXLFMT_RGBA5654_PK: // 96660 not support
		switch (uiType) {
		case TYPE_NULL:
		case TYPE_FB:
		case TYPE_BITMAP:
		case TYPE_ICON:
			break;
		default:
			DBG_ERR("GxGfx_CheckTypeFmt - ERROR! DC type/format is not supported.\r\n");
			r = GX_ERROR_TYPE;
			break;
		}
		break;
#endif
#if (GX_SUPPORT_DCFMT_UVPACK)
	case PXLFMT_YUV422_PK:
	case PXLFMT_YUV421_PK:
	case PXLFMT_YUV420_PK:
		//NOTE: not support TYPE_ICON
		switch (uiType) {
		case TYPE_NULL:
		case TYPE_FB:
		case TYPE_BITMAP:
		case TYPE_JPEG:
			break;
		default:
			DBG_ERR("GxGfx_CheckTypeFmt - ERROR! DC type/format is not supported.\r\n");
			r = GX_ERROR_TYPE;
			break;
		}
		break;
#endif
	default:
		DBG_ERR("GxGfx_CheckTypeFmt - ERROR! DC format is invalid. 0x%x \r\n", uiPxlfmt);
		r = GX_ERROR_FORMAT;
		break;
	}

	return r;
}

RESULT GxGfx_AttachDC(DC *_pDC,
					  UINT16 uiType, UINT16 uiPxlfmt,
					  UINT32 uiWidth, UINT32 uiHeight,
					  UINT32 uiPitch, UINT8 *pBuf0, UINT8 *pBuf1, UINT8 *pBuf2)
{
	DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;
	UINT32 uiOffset;
	RESULT r;
#if (GX_SUPPPORT_YUV ==ENABLE)
	BOOL bCalcBuf = FALSE;
#endif
	UINT8 *pBuf = pBuf0;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDC) {
		return GX_NULL_POINTER;
	}

	if (!pBuf0) {
		return GX_NULL_POINTER;
	}
#if (GX_SUPPPORT_YUV ==ENABLE)
	if ((!pBuf1) || (!pBuf2)) {
		bCalcBuf = TRUE;
	}
#endif

	r = GxGfx_CheckDCTypeFmt(uiType, uiPxlfmt);
	if (r != GX_OK) {
		return r;
	}

	if (uiPitch == 0) { //auto calculate minimal pitch
		uiPitch = GxGfx_CalcDCPitchSize(uiWidth, uiType, uiPxlfmt);
	}

	switch (uiPxlfmt) {
	case PXLFMT_INDEX1:
	case PXLFMT_INDEX2:
	case PXLFMT_INDEX4:
	case PXLFMT_INDEX8:
		//check zero
		if ((uiWidth == 0) || (uiHeight == 0) || (uiPitch == 0)) {
			return GX_ERROR_SIZEZERO;
		}

		if ((uiType == TYPE_FB)
			|| (uiType == TYPE_BITMAP)) {
			if ((uiPitch & 0x00000003)) { //pitch must be 4 bytes alignment.
				return GX_ERROR_SIZEALIGN;
			}
		}
		break;
#if (GX_SUPPPORT_YUV ==ENABLE)
	case PXLFMT_YUV444:
	case PXLFMT_YUV422:
	case PXLFMT_YUV421:
	case PXLFMT_YUV420:
	case PXLFMT_RGB888:
#if (GX_SUPPORT_DCFMT_UVPACK)
	case PXLFMT_YUV422_PK:
	case PXLFMT_YUV421_PK:
	case PXLFMT_YUV420_PK:
#endif
#endif
	case PXLFMT_RGB888_PK:
	case PXLFMT_RGBD8888_PK:
	case PXLFMT_RGBA8888_PK:
#if (GX_SUPPORT_DCFMT_RGB565)
	case PXLFMT_RGB565_PK:
	case PXLFMT_RGBA5658_PK:
	case PXLFMT_RGBA5654_PK:
#endif
#if (GX_SUPPORT_DCFMT_RGB444)
	case PXLFMT_RGBA4444_PK:
	case PXLFMT_RGBA5551_PK:
#endif
		//check zero
		if ((uiWidth == 0) || (uiHeight == 0) || (uiPitch == 0)) {
			return GX_ERROR_SIZEZERO;
		}

		if (uiType == TYPE_JPEG) {
			break;
		}
#if (GX_SUPPPORT_YUV ==ENABLE)
		//check limit
		if (uiPxlfmt == PXLFMT_YUV420) {
			if ((uiWidth & 0x00000003)) {
				return GX_ERROR_SIZEALIGN;
			}
			if ((uiHeight & 0x00000001)) { //2 line pair for subsample U to V (for YUV420)
				return GX_ERROR_SIZEALIGN;
			}
		} else if (uiPxlfmt == PXLFMT_YUV421) {
			if ((uiWidth & 0x00000003)) {
				return GX_ERROR_SIZEALIGN;
			}
			if ((uiHeight & 0x00000001)) { //2 line pair for subsample U to V (for YUV421)
				return GX_ERROR_SIZEALIGN;
			}
		} else if ((uiPxlfmt == PXLFMT_YUV444) || (uiPxlfmt == PXLFMT_YUV422)) {
			if ((uiWidth & 0x00000001)) {
				return GX_ERROR_SIZEALIGN;
			}
		}
#if (GX_SUPPORT_DCFMT_UVPACK)
		if (uiPxlfmt == PXLFMT_YUV420_PK) {
			if ((uiWidth & 0x00000003)) {
				return GX_ERROR_SIZEALIGN;
			}
			if ((uiHeight & 0x00000001)) { //2 line pair for subsample U to V (for YUV420)
				return GX_ERROR_SIZEALIGN;
			}
		} else if (uiPxlfmt == PXLFMT_YUV421_PK) {
			if ((uiWidth & 0x00000003)) {
				return GX_ERROR_SIZEALIGN;
			}
			if ((uiHeight & 0x00000001)) { //2 line pair for subsample U to V (for YUV421)
				return GX_ERROR_SIZEALIGN;
			}
		} else if (uiPxlfmt == PXLFMT_YUV422_PK) {
			if ((uiWidth & 0x00000001)) {
				return GX_ERROR_SIZEALIGN;
			}
		}
#endif
#endif
		if ((uiType == TYPE_FB)
			|| (uiType == TYPE_BITMAP)) {
			if ((uiPitch & 0x00000003)) { //pitch must be 4 bytes alignment.
				return GX_ERROR_SIZEALIGN;
			}
		}
		break;
	}

	//clear whole dc
	memset(pDC, 0, sizeof(DISPLAY_CONTEXT));

	pDC->uiType = uiType;
	pDC->uiPxlfmt = uiPxlfmt;

	SIZE_Set(&(pDC->size), uiWidth, uiHeight);
	RECT_Set(&(pDC->winRect), 0, 0, uiWidth, uiHeight);

	pDC->uiFlag = 0;

	POINT_Set(&(pDC->lastPt), 0, 0);
	POINT_Set(&(pDC->thisPt), 0, 0);

	pDC->uiReserved[0] = 0;
	pDC->uiReserved[1] = 0;
	pDC->uiReserved[2] = 0;
	pDC->uiReserved[3] = 0;

	SIZE_Set(&(pDC->virSize), uiWidth, uiHeight); //equal to real buffer size
	POINT_Set(&(pDC->shiftVec), 0, 0); //0
	POINT_Set(&(pDC->scaleVec), 0x00010000, 0x00010000); //1.0

	uiOffset = (unsigned int)pBuf;

	switch (uiPxlfmt) {
	case PXLFMT_INDEX1:
	case PXLFMT_INDEX2:
	case PXLFMT_INDEX4:
	case PXLFMT_INDEX8:
		//block 3
		pDC->blk[3].uiWidth = uiWidth;
		pDC->blk[3].uiHeight = uiHeight;
		pDC->blk[3].uiPitch = uiPitch;
		pDC->blk[3].uiOffset = uiOffset;
		break;
#if (GX_SUPPPORT_YUV ==ENABLE)
	case PXLFMT_YUV444:
	case PXLFMT_YUV422:
	case PXLFMT_YUV421:
	case PXLFMT_YUV420:

		uiOffset = MAKE_ALIGN(uiOffset, 32); //keep 32 bytes alignment (for JPG HW)

		//block 0
		pDC->blk[0].uiWidth = uiWidth;
		pDC->blk[0].uiHeight = uiHeight;
		pDC->blk[0].uiPitch = uiPitch;
		pDC->blk[0].uiOffset = uiOffset;

		switch (pDC->uiPxlfmt) {
		case PXLFMT_YUV444:
			break;
		case PXLFMT_YUV422:
			uiWidth >>= 1;
			uiPitch >>= 1;
			break;
		case PXLFMT_YUV421:
			uiHeight >>= 1;;
			break;
		case PXLFMT_YUV420:
			uiWidth >>= 1;
			uiPitch >>= 1;
			uiHeight >>= 1;
			break;
		}
		if (bCalcBuf) {
			uiOffset += pDC->blk[0].uiHeight * pDC->blk[0].uiPitch;
		} else {
			uiOffset = (UINT32)pBuf1;
		}

		uiOffset = MAKE_ALIGN(uiOffset, 32); //keep 32 bytes alignment (for JPG HW)

		//block 1
		pDC->blk[1].uiWidth = uiWidth;
		pDC->blk[1].uiHeight = uiHeight;
		pDC->blk[1].uiPitch = uiPitch;
		pDC->blk[1].uiOffset = uiOffset;

		if (bCalcBuf) {
			uiOffset += pDC->blk[1].uiHeight * pDC->blk[1].uiPitch;
		} else {
			uiOffset = (UINT32)pBuf1;
		}

		uiOffset = MAKE_ALIGN(uiOffset, 32); //keep 32 bytes alignment (for JPG HW)

		//block 2
		pDC->blk[2].uiWidth = uiWidth;
		pDC->blk[2].uiHeight = uiHeight;
		pDC->blk[2].uiPitch = uiPitch;
		pDC->blk[2].uiOffset = uiOffset;

		//block 3
		pDC->blk[3].uiWidth = 0;
		pDC->blk[3].uiHeight = 0;
		pDC->blk[3].uiPitch = 0;
		pDC->blk[3].uiOffset = 0;

		break;

#if (GX_SUPPORT_DCFMT_UVPACK)
	case PXLFMT_YUV422_PK:
	case PXLFMT_YUV421_PK:
	case PXLFMT_YUV420_PK:

		//block 0
		pDC->blk[0].uiWidth = uiWidth;
		pDC->blk[0].uiHeight = uiHeight;
		pDC->blk[0].uiPitch = uiPitch;
		pDC->blk[0].uiOffset = uiOffset;

		switch (pDC->uiPxlfmt) {
		case PXLFMT_YUV422_PK:
			//uiWidth >>= 1;
			break;
		case PXLFMT_YUV421_PK:
			uiHeight >>= 1;
			break;
		case PXLFMT_YUV420_PK:
			//uiWidth >>= 1;
			uiHeight >>= 1;
			break;
		}
		if (bCalcBuf) {
			uiOffset += pDC->blk[0].uiHeight * pDC->blk[0].uiPitch;
		} else {
			uiOffset = (UINT32)pBuf1;
		}

		//block 1
		pDC->blk[1].uiWidth = uiWidth;
		pDC->blk[1].uiHeight = uiHeight;
		pDC->blk[1].uiPitch = uiPitch;
		pDC->blk[1].uiOffset = uiOffset;

		//block 2 (same as block 1)
		pDC->blk[2].uiWidth = uiWidth;
		pDC->blk[2].uiHeight = uiHeight;
		pDC->blk[2].uiPitch = uiPitch;
		pDC->blk[2].uiOffset = uiOffset;

		//block 3
		pDC->blk[3].uiWidth = 0;
		pDC->blk[3].uiHeight = 0;
		pDC->blk[3].uiPitch = 0;
		pDC->blk[3].uiOffset = 0;

		break;
#endif
	case PXLFMT_RGB888:
		//block 0
		pDC->blk[0].uiWidth = uiWidth;
		pDC->blk[0].uiHeight = uiHeight;
		pDC->blk[0].uiPitch = uiPitch;
		pDC->blk[0].uiOffset = uiOffset;

		if (bCalcBuf) {
			uiOffset += pDC->blk[0].uiHeight * pDC->blk[0].uiPitch;
		} else {
			uiOffset = (UINT32)pBuf1;
		}

		//block 1
		pDC->blk[1].uiWidth = uiWidth;
		pDC->blk[1].uiHeight = uiHeight;
		pDC->blk[1].uiPitch = uiPitch;
		pDC->blk[1].uiOffset = uiOffset;

		if (bCalcBuf) {
			uiOffset += pDC->blk[0].uiHeight * pDC->blk[0].uiPitch;
		} else {
			uiOffset = (UINT32)pBuf1;
		}

		//block 2
		pDC->blk[2].uiWidth = uiWidth;
		pDC->blk[2].uiHeight = uiHeight;
		pDC->blk[2].uiPitch = uiPitch;
		pDC->blk[2].uiOffset = uiOffset;

		//block 3
		pDC->blk[3].uiWidth = 0;
		pDC->blk[3].uiHeight = 0;
		pDC->blk[3].uiPitch = 0;
		pDC->blk[3].uiOffset = 0;

		break;
#endif

	case PXLFMT_RGB888_PK:
	case PXLFMT_RGBD8888_PK:
	case PXLFMT_RGBA8888_PK:

		//block 0
		pDC->blk[0].uiWidth = 0;
		pDC->blk[0].uiHeight = 0;
		pDC->blk[0].uiPitch = 0;
		pDC->blk[0].uiOffset = 0;

		//block 1
		pDC->blk[1].uiWidth = 0;
		pDC->blk[1].uiHeight = 0;
		pDC->blk[1].uiPitch = 0;
		pDC->blk[1].uiOffset = 0;

		//block 2
		pDC->blk[2].uiWidth = 0;
		pDC->blk[2].uiHeight = 0;
		pDC->blk[2].uiPitch = 0;
		pDC->blk[2].uiOffset = 0;

		//block 3 (RGB)
		pDC->blk[3].uiWidth = uiWidth;
		pDC->blk[3].uiHeight = uiHeight;
		pDC->blk[3].uiPitch = uiPitch;
		pDC->blk[3].uiOffset = uiOffset;

		break;

#if (GX_SUPPORT_DCFMT_RGB565)
	case PXLFMT_RGB565_PK:

		//block 0 (RGB)
		pDC->blk[0].uiWidth = uiWidth;
		pDC->blk[0].uiHeight = uiHeight;
		pDC->blk[0].uiPitch = uiPitch;
		pDC->blk[0].uiOffset = uiOffset;

		//block 1
		pDC->blk[1].uiWidth = 0;
		pDC->blk[1].uiHeight = 0;
		pDC->blk[1].uiPitch = 0;
		pDC->blk[1].uiOffset = 0;

		//block 2
		pDC->blk[2].uiWidth = 0;
		pDC->blk[2].uiHeight = 0;
		pDC->blk[2].uiPitch = 0;
		pDC->blk[2].uiOffset = 0;

		//block 3
		pDC->blk[3].uiWidth = 0;
		pDC->blk[3].uiHeight = 0;
		pDC->blk[3].uiPitch = 0;
		pDC->blk[3].uiOffset = 0;

		break;

	case PXLFMT_RGBA5658_PK:
	case PXLFMT_RGBA5654_PK:

		//block 0 (RGB)
		pDC->blk[0].uiWidth = uiWidth;
		pDC->blk[0].uiHeight = uiHeight;
		pDC->blk[0].uiPitch = uiPitch;
		pDC->blk[0].uiOffset = uiOffset;

		//block 1
		pDC->blk[1].uiWidth = 0;
		pDC->blk[1].uiHeight = 0;
		pDC->blk[1].uiPitch = 0;
		pDC->blk[1].uiOffset = 0;

		//block 2
		pDC->blk[2].uiWidth = 0;
		pDC->blk[2].uiHeight = 0;
		pDC->blk[2].uiPitch = 0;
		pDC->blk[2].uiOffset = 0;

		//block 3 (Alpha)
		pDC->blk[3].uiWidth = uiWidth;
		pDC->blk[3].uiHeight = uiHeight;
		if (uiPxlfmt == PXLFMT_RGBA5658_PK) {
			pDC->blk[3].uiPitch = GxGfx_CalcDCPitchSize(uiWidth, uiType, PXLFMT_INDEX8);
		} else {
			pDC->blk[3].uiPitch = GxGfx_CalcDCPitchSize(uiWidth, uiType, PXLFMT_INDEX4);
		}
		pDC->blk[3].uiOffset = uiOffset + uiPitch * uiHeight;
		break;

#endif
#if (GX_SUPPORT_DCFMT_RGB444)
	case PXLFMT_RGBA4444_PK:
	case PXLFMT_RGBA5551_PK:

		//block 0
		pDC->blk[0].uiWidth = 0;
		pDC->blk[0].uiHeight = 0;
		pDC->blk[0].uiPitch = 0;
		pDC->blk[0].uiOffset = 0;

		//block 1
		pDC->blk[1].uiWidth = 0;
		pDC->blk[1].uiHeight = 0;
		pDC->blk[1].uiPitch = 0;
		pDC->blk[1].uiOffset = 0;

		//block 2
		pDC->blk[2].uiWidth = 0;
		pDC->blk[2].uiHeight = 0;
		pDC->blk[2].uiPitch = 0;
		pDC->blk[2].uiOffset = 0;

		//block 3 (RGB)
		pDC->blk[3].uiWidth = uiWidth;
		pDC->blk[3].uiHeight = uiHeight;
		pDC->blk[3].uiPitch = uiPitch;
		pDC->blk[3].uiOffset = uiOffset;

		break;

#endif


	}

	pDC->uiFlag &= ~DCFLAG_SIGN_MASK;
	pDC->uiFlag |= DCFLAG_SIGN;

	DBG_IND("uiFlag %x \r\n", pDC->uiFlag);
	DBG_IND("uiType %x \r\n", pDC->uiType);
	DBG_IND("uiPxlfmt %x \r\n", pDC->uiPxlfmt);
	DBG_IND("size w %d h %d\r\n", pDC->size.w, pDC->size.h);
	DBG_IND("blk 0 w %d h %d off %x pitch %d\r\n", pDC->blk[0].uiWidth, pDC->blk[0].uiHeight, pDC->blk[0].uiOffset, pDC->blk[0].uiPitch);
	DBG_IND("blk 3 w %d h %d off %x pitch %d\r\n", pDC->blk[3].uiWidth, pDC->blk[3].uiHeight, pDC->blk[3].uiOffset, pDC->blk[3].uiPitch);



	return GX_OK;
}

void   GxGfx_DetachDC(DC *_pDC)
{
	DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDC) {
		return;
	}
	GXGFX_ASSERT((pDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	pDC->uiType = TYPE_NULL; //clear

	pDC->blk[0].uiOffset = 0;
	pDC->blk[1].uiOffset = 0;
	pDC->blk[2].uiOffset = 0;
	pDC->blk[3].uiOffset = 0;

	pDC->uiFlag = 0; //clear

	//clear whole dc
	memset(pDC, 0, sizeof(DISPLAY_CONTEXT));
}

ISIZE GxGfx_GetSize(const DC *_pDC)
{
	const DISPLAY_CONTEXT *pDC = (const DISPLAY_CONTEXT *)_pDC;
	ISIZE sz;
	SIZE_Set(&sz, 0, 0);

	DBG_FUNC_BEGIN("\r\n");
	if (!pDC) {
		return sz;
	}
	GXGFX_ASSERT((pDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	sz = pDC->size;
	return sz;
}

RESULT GxGfx_GetAddr(const DC *_pDC, UINT32 *pPlan1, UINT32 *pPlan2, UINT32 *pPlan3)
{
	const DISPLAY_CONTEXT *pDC = (const DISPLAY_CONTEXT *)_pDC;
	RESULT r = GX_OK;

	if (!pDC) {
		return GX_NULL_POINTER;
	}

	switch (pDC->uiPxlfmt) {
	case PXLFMT_INDEX1:
	case PXLFMT_INDEX2:
	case PXLFMT_INDEX4:
	case PXLFMT_INDEX8:
		//block 3
		if (pPlan1) {
			*pPlan1 = pDC->blk[3].uiOffset;
		}
		break;
#if (GX_SUPPPORT_YUV ==ENABLE)
	case PXLFMT_YUV444:
	case PXLFMT_YUV422:
	case PXLFMT_YUV421:
	case PXLFMT_YUV420:
	case PXLFMT_RGB888:
#if (GX_SUPPORT_DCFMT_UVPACK)
	case PXLFMT_YUV422_PK:
	case PXLFMT_YUV421_PK:
	case PXLFMT_YUV420_PK:
#endif
		if (pPlan1) {
			*pPlan1 = pDC->blk[0].uiOffset;
		}
		if (pPlan2) {
			*pPlan2 = pDC->blk[1].uiOffset;
		}
		if (pPlan3) {
			*pPlan3 = pDC->blk[2].uiOffset;
		}
		break;
#endif
	case PXLFMT_RGB888_PK:
	case PXLFMT_RGBD8888_PK:
	case PXLFMT_RGBA8888_PK:
		if (pPlan1) {
			*pPlan1 = pDC->blk[3].uiOffset;
		}
		break;
#if (GX_SUPPORT_DCFMT_RGB444)
	case PXLFMT_RGBA4444_PK:
	case PXLFMT_RGBA5551_PK:
		if (pPlan1) {
			*pPlan1 = pDC->blk[3].uiOffset;
		}
		break;
#endif
#if (GX_SUPPORT_DCFMT_RGB565)
	case PXLFMT_RGB565_PK:
		if (pPlan1) {
			*pPlan1 = pDC->blk[0].uiOffset;
		}
		break;
	case PXLFMT_RGBA5658_PK:
	case PXLFMT_RGBA5654_PK:
		if (pPlan1) {
			*pPlan1 = pDC->blk[0].uiOffset;
		}
		//block 3 (Alpha)
		if (pPlan2) {
			*pPlan2 = pDC->blk[3].uiOffset;
		}
		break;
#endif
	default:
		r = GX_ERROR_FORMAT;
		break;
	}

	return r;

}

RESULT GxGfx_GetLineOffset(const DC *_pDC, UINT32 *pPlan1, UINT32 *pPlan2, UINT32 *pPlan3)
{
	const DISPLAY_CONTEXT *pDC = (const DISPLAY_CONTEXT *)_pDC;
	RESULT r = GX_OK;

	if (!pDC) {
		return GX_NULL_POINTER;
	}

	switch (pDC->uiPxlfmt) {
	case PXLFMT_INDEX1:
	case PXLFMT_INDEX2:
	case PXLFMT_INDEX4:
	case PXLFMT_INDEX8:
		//block 3
		if (pPlan1) {
			*pPlan1 = pDC->blk[3].uiPitch;
		}
		break;
#if (GX_SUPPPORT_YUV ==ENABLE)
	case PXLFMT_YUV444:
	case PXLFMT_YUV422:
	case PXLFMT_YUV421:
	case PXLFMT_YUV420:
	case PXLFMT_RGB888:
#if (GX_SUPPORT_DCFMT_UVPACK)
	case PXLFMT_YUV422_PK:
	case PXLFMT_YUV421_PK:
	case PXLFMT_YUV420_PK:
#endif
		if (pPlan1) {
			*pPlan1 = pDC->blk[0].uiPitch;
		}
		if (pPlan2) {
			*pPlan2 = pDC->blk[1].uiPitch;
		}
		if (pPlan3) {
			*pPlan3 = pDC->blk[2].uiPitch;
		}
		break;
#endif
	case PXLFMT_RGB888_PK:
	case PXLFMT_RGBD8888_PK:
	case PXLFMT_RGBA8888_PK:
#if (GX_SUPPORT_DCFMT_RGB444)
	case PXLFMT_RGBA4444_PK:
	case PXLFMT_RGBA5551_PK:
#endif
		if (pPlan1) {
			*pPlan1 = pDC->blk[3].uiPitch;
		}
		break;
#if (GX_SUPPORT_DCFMT_RGB565)
	case PXLFMT_RGB565_PK:
		if (pPlan1) {
			*pPlan1 = pDC->blk[0].uiPitch;
		}
		break;
	case PXLFMT_RGBA5658_PK:
	case PXLFMT_RGBA5654_PK:
		if (pPlan1) {
			*pPlan1 = pDC->blk[0].uiPitch;
		}
		//block 3 (Alpha)
		if (pPlan2) {
			*pPlan2 = pDC->blk[3].uiPitch;
		}
		break;
#endif
	default:
		r = GX_ERROR_FORMAT;
		break;
	}
	return r;

}
RESULT GxGfx_SetCoord(DC *_pDC, LVALUE vw, LVALUE vh)
{
	DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;
	ISIZE virSize;
	IRECT realRect;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	if (vw == 0) {
		return GX_EMPTY_RECT;
	}
	if (vh == 0) {
		return GX_EMPTY_RECT;
	}

	SIZE_Set(&(virSize), vw, vh);
	RECT_Set(&(realRect), 0, 0, pDC->size.w, pDC->size.h);
	pDC->virSize = virSize;
	pDC->shiftVec.x = realRect.x;
	pDC->shiftVec.y = realRect.y;
	pDC->scaleVec.x = (realRect.w << 16) / virSize.w;
	pDC->scaleVec.y = (realRect.h << 16) / virSize.h;

	return GX_OK;
}
RESULT GxGfx_SetCoordEx(DC *_pDC, LVALUE vw, LVALUE vh, LVALUE rx, LVALUE ry, LVALUE rw, LVALUE rh)
{
	DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;
	ISIZE virSize;
	IRECT realRect;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	if (vw == 0) {
		return GX_EMPTY_RECT;
	}
	if (vh == 0) {
		return GX_EMPTY_RECT;
	}
	if (rw == 0) {
		return GX_EMPTY_RECT;
	}
	if (rh == 0) {
		return GX_EMPTY_RECT;
	}

	SIZE_Set(&(virSize), vw, vh);
	RECT_Set(&(realRect), rx, ry, rw, rh);
	pDC->virSize = virSize;
	pDC->shiftVec.x = realRect.x;
	pDC->shiftVec.y = realRect.y;
	pDC->scaleVec.x = (realRect.w << 16) / virSize.w;
	pDC->scaleVec.y = (realRect.h << 16) / virSize.h;

	return GX_OK;
}

IRECT _DC_GetRect(const DC *_pDC)
{
	const DISPLAY_CONTEXT *pDC = (const DISPLAY_CONTEXT *)_pDC;
	IRECT DestRect;
	RECT_Set(&DestRect, 0, 0, pDC->virSize.w, pDC->virSize.h);
	return DestRect;
}
IRECT _DC_GetRealRect(const DC *_pDC)
{
	const DISPLAY_CONTEXT *pDC = (const DISPLAY_CONTEXT *)_pDC;
	IRECT DestRect;
	RECT_Set(&DestRect, 0, 0, pDC->size.w, pDC->size.h);
	return DestRect;
}

static ISIZE nullSize = {0, 0};
static IRECT nullRect = {0, 0, 0, 0};
ISIZE GxGfx_GetCoord(const DC *_pDC)
{
	const DISPLAY_CONTEXT *pDC = (const DISPLAY_CONTEXT *)_pDC;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDC) {
		return nullSize;
	}
	GXGFX_ASSERT((pDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	return pDC->virSize;
}
IRECT GxGfx_GetCoordEx(const DC *_pDC)
{
	const DISPLAY_CONTEXT *pDC = (const DISPLAY_CONTEXT *)_pDC;
	IRECT DestRect;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDC) {
		return nullRect;
	}
	GXGFX_ASSERT((pDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	DestRect = _DC_GetRect(_pDC);
	_DC_XFORM_RECT(_pDC, &DestRect);

	return DestRect;
}

RESULT GxGfx_SetOrigin(DC *_pDC, LVALUE x1, LVALUE y1)
{
	DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	POINT_Set(&(pDC->origPt), x1, y1);

	return GX_OK;
}

IPOINT GxGfx_GetOrigin(const DC *_pDC)
{
	const DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;
	IPOINT Pt;
	DBG_FUNC_BEGIN("\r\n");
	if (!pDC) {
		POINT_Set(&Pt, 0, 0);
		return Pt;
	}
	GXGFX_ASSERT((pDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	return pDC->origPt;
}

//shift+scale
void _DC_XFORM_POINT(const DC *_pDC, IPOINT *pPt)
{
	const DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;
	IPOINT Ot;
	if (!pDC) {
		return;
	}
	//(1st phase) xfm src -> world
	POINT_SetPoint(&Ot, &(_DC(pDC)->origPt));
	POINT_MovePoint(pPt, &Ot);
	//(2nd phase) xfm world -> dest
	if ((pDC->scaleVec.x == 0x00010000) && (pDC->scaleVec.y == 0x00010000)) {
		return;
	}
	pPt->x = pDC->shiftVec.x + ((pPt->x * pDC->scaleVec.x) >> 16);
	pPt->y = pDC->shiftVec.y + ((pPt->y * pDC->scaleVec.y) >> 16);
}
//shift+scale
void _DC_XFORM_RECT(const DC *_pDC, IRECT *pRect)
{
	const DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;
	IPOINT Ot;
	if (!pDC) {
		return;
	}
	//(1st phase) xfm src -> world
	POINT_SetPoint(&Ot, &(_DC(pDC)->origPt));
	RECT_MovePoint(pRect, &Ot);
	//(2nd phase) xfm world -> dest
	if ((pDC->scaleVec.x == 0x00010000) && (pDC->scaleVec.y == 0x00010000)) {
		return;
	}
	pRect->x = pDC->shiftVec.x + ((pRect->x * pDC->scaleVec.x) >> 16);
	pRect->y = pDC->shiftVec.y + ((pRect->y * pDC->scaleVec.y) >> 16);
	pRect->w = ((pRect->w * pDC->scaleVec.x) >> 16);
	pRect->h = ((pRect->h * pDC->scaleVec.y) >> 16);
}
//shift+scale (but not apply orig shift)
void _DC_XFORM2_RECT(const DC *_pDC, IRECT *pRect)
{
	const DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;
	if (!pDC) {
		return;
	}
	//(2nd phase) xfm world -> dest
	if ((pDC->scaleVec.x == 0x00010000) && (pDC->scaleVec.y == 0x00010000)) {
		return;
	}
	pRect->x = pDC->shiftVec.x + ((pRect->x * pDC->scaleVec.x) >> 16);
	pRect->y = pDC->shiftVec.y + ((pRect->y * pDC->scaleVec.y) >> 16);
	pRect->w = ((pRect->w * pDC->scaleVec.x) >> 16);
	pRect->h = ((pRect->h * pDC->scaleVec.y) >> 16);
}
//scale only
void _DC_SCALE_POINT(const DC *_pDC, IPOINT *pPt)
{
	const DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;
	if (!pDC) {
		return;
	}
	if ((pDC->scaleVec.x == 0x00010000) && (pDC->scaleVec.y == 0x00010000)) {
		return;
	}
	pPt->x = (pPt->x * pDC->scaleVec.x) >> 16;
	pPt->y = (pPt->y * pDC->scaleVec.y) >> 16;
}
//scale only
void _DC_SCALE_RECT(const DC *_pDC, IRECT *pRect)
{
	const DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;
	if (!pDC) {
		return;
	}
	if ((pDC->scaleVec.x == 0x00010000) && (pDC->scaleVec.y == 0x00010000)) {
		return;
	}
	pRect->x = (pRect->x * pDC->scaleVec.x) >> 16;
	pRect->y = (pRect->y * pDC->scaleVec.y) >> 16;
	pRect->w = (pRect->w * pDC->scaleVec.x) >> 16;
	pRect->h = (pRect->h * pDC->scaleVec.y) >> 16;
}
//scale only
void _DC_INVSCALE_POINT(const DC *_pDC, IPOINT *pPt)
{
	const DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;
	if (!pDC) {
		return;
	}
	if ((pDC->scaleVec.x == 0x00010000) && (pDC->scaleVec.y == 0x00010000)) {
		return;
	}
	pPt->x = (pPt->x << 16) / pDC->scaleVec.x;
	pPt->y = (pPt->y << 16) / pDC->scaleVec.y;
}
//scale only
void _DC_INVSCALE_RECT(const DC *_pDC, IRECT *pRect)
{
	const DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;
	if (!pDC) {
		return;
	}
	if ((pDC->scaleVec.x == 0x00010000) && (pDC->scaleVec.y == 0x00010000)) {
		return;
	}
	pRect->x = (pRect->x << 16) / pDC->scaleVec.x;
	pRect->y = (pRect->y << 16) / pDC->scaleVec.y;
	pRect->w = (pRect->w << 16) / pDC->scaleVec.x;
	pRect->h = (pRect->h << 16) / pDC->scaleVec.y;
}


RESULT GxGfx_SetWindow(DC *_pDC, LVALUE x1, LVALUE y1, LVALUE x2, LVALUE y2)
{
	DISPLAY_CONTEXT *pDC = (DISPLAY_CONTEXT *)_pDC;
	IRECT DestClipRect;
	IRECT FullRect;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDC) {
		return GX_NULL_POINTER;
	}
	GXGFX_ASSERT((pDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	/*
	    if((x1 == 0) && (y1 == 0) && (x2 == 0) && (y2 == 0))
	    {
	        RECT_Set(&DestClipRect, 0, 0, pDC->size.w, pDC->size.h);
	        pDC->winRect = DestClipRect;
	        return GX_OK;
	    }
	*/
	RECT_SetX1Y1X2Y2(&DestClipRect, x1, y1, x2, y2);
	FullRect = _DC_GetRect(_pDC);

	//interection ClipRect with full Rect
	if (DestClipRect.x < 0) {
		DestClipRect.w -= -DestClipRect.x;
		DestClipRect.x = 0;
	}
	if (DestClipRect.x + DestClipRect.w > FullRect.w) {
		DestClipRect.w = FullRect.w - DestClipRect.x;
	}
	if (DestClipRect.y < 0) {
		DestClipRect.h -= -DestClipRect.y;
		DestClipRect.y = 0;
	}
	if (DestClipRect.y + DestClipRect.h > FullRect.h) {
		DestClipRect.h = FullRect.h - DestClipRect.y;
	}

	pDC->winRect = DestClipRect;

	return GX_OK;
}

IRECT GxGfx_GetWindow(const DC *_pDC)
{
	const DISPLAY_CONTEXT *pDC = (const DISPLAY_CONTEXT *)_pDC;
	IRECT ClipRect;

	DBG_FUNC_BEGIN("\r\n");
	if (!pDC) {
		RECT_Set(&ClipRect, 0, 0, 0, 0);
		return ClipRect;
	}
	GXGFX_ASSERT((pDC->uiFlag & DCFLAG_SIGN_MASK) == DCFLAG_SIGN);

	return pDC->winRect;
}

RESULT _DC_CalcPlaneRect(const DC *_pDC, UINT8 ucPlane, const IRECT *pRect, IRECT *pPlaneRect)
{
	const DISPLAY_CONTEXT *pDC = (const DISPLAY_CONTEXT *)_pDC;
	//const PXLBLK* pBlk;

	if (pRect == 0) {
		return GX_NULL_POINTER;
	}

	if (ucPlane >= 4) {
		return GX_OUTOF_PLANE;
	}

	//pBlk = &(pDC->blk[ucPlane]);
	RECT_SetRect(pPlaneRect, pRect);

	if (ucPlane > 0) {
		switch (pDC->uiPxlfmt) {
		case PXLFMT_INDEX4:
			pPlaneRect->x >>= 1;
			pPlaneRect->w >>= 1;
			break;
		case PXLFMT_INDEX8:
			break;
#if (GX_SUPPPORT_YUV ==ENABLE)

		case PXLFMT_YUV444:
			break;
		case PXLFMT_YUV422:
			pPlaneRect->x >>= 1;
			pPlaneRect->w >>= 1;
			break;
		case PXLFMT_YUV421:
			pPlaneRect->y >>= 1;
			pPlaneRect->h >>= 1;
			break;
		case PXLFMT_YUV420:
			pPlaneRect->x >>= 1;
			pPlaneRect->w >>= 1;
			pPlaneRect->y >>= 1;
			pPlaneRect->h >>= 1;
			break;

#if (GX_SUPPORT_DCFMT_UVPACK)
		case PXLFMT_YUV422_PK:
			//pPlaneRect->x >>= 1;
			//pPlaneRect->w >>= 1;
			break;
		case PXLFMT_YUV421_PK:
			pPlaneRect->y >>= 1;
			pPlaneRect->h >>= 1;
			break;
		case PXLFMT_YUV420_PK:
			//pPlaneRect->x >>= 1;
			//pPlaneRect->w >>= 1;
			pPlaneRect->y >>= 1;
			pPlaneRect->h >>= 1;
			break;
#endif

		case PXLFMT_RGB888:
			break;
#endif
		case PXLFMT_RGB888_PK:
		case PXLFMT_RGBD8888_PK:
		case PXLFMT_RGBA8888_PK:
#if (GX_SUPPORT_DCFMT_RGB444)
		case PXLFMT_RGBA4444_PK:
		case PXLFMT_RGBA5551_PK:
#endif
			break;

#if (GX_SUPPORT_DCFMT_RGB565)
		case PXLFMT_RGB565_PK:
		case PXLFMT_RGBA5658_PK:
		case PXLFMT_RGBA5654_PK:
			break;
#endif

		default:
			pPlaneRect->x = 0;
			pPlaneRect->w = 0;
			pPlaneRect->y = 0;
			pPlaneRect->h = 0;
			break;
		}
	}

	return GX_OK;
}

UINT32 _DC_CalcPlaneOffset(const DC *_pDC, UINT8 ucPlane, const IRECT *pPlaneRect)
{
	const DISPLAY_CONTEXT *pDC = (const DISPLAY_CONTEXT *)_pDC;
	const PXLBLK *pBlk;
	UINT32 uiOffset;

	if (pPlaneRect == 0) {
		return 0;
	}

	if (ucPlane >= 4) {
		return 0;
	}

	pBlk = &(pDC->blk[ucPlane]);
	uiOffset = pBlk->uiOffset + pPlaneRect->x * GxGfx_GetPixelSize(_pDC->uiPxlfmt, ucPlane) + pPlaneRect->y * pBlk->uiPitch;

	return uiOffset;
}

void RECT_CalcInterection(const IRECT *pSrcRectA, const IRECT *pSrcRectB, IRECT *pOutRect)
{
	if (pSrcRectA->x < pSrcRectB->x) {
		pOutRect->x = pSrcRectB->x;
		if (pSrcRectA->x + pSrcRectA->w > pSrcRectB->x + pSrcRectB->w) {
			pOutRect->w = pSrcRectB->w;
		} else {
			pOutRect->w = pSrcRectA->w - (pSrcRectB->x - pSrcRectA->x);
		}
	} else {
		pOutRect->x = pSrcRectA->x;
		if (pSrcRectB->x + pSrcRectB->w > pSrcRectA->x + pSrcRectA->w) {
			pOutRect->w = pSrcRectA->w;
		} else {
			pOutRect->w = pSrcRectB->w - (pSrcRectA->x - pSrcRectB->x);
		}
	}
	if (pOutRect->w < 0) {
		pOutRect->w = 0;
	}

	if (pSrcRectA->y < pSrcRectB->y) {
		pOutRect->y = pSrcRectB->y;
		if (pSrcRectA->y + pSrcRectA->h > pSrcRectB->y + pSrcRectB->h) {
			pOutRect->h = pSrcRectB->h;
		} else {
			pOutRect->h = pSrcRectA->h - (pSrcRectB->y - pSrcRectA->y);
		}
	} else {
		pOutRect->y = pSrcRectA->y;
		if (pSrcRectB->y + pSrcRectB->h > pSrcRectA->y + pSrcRectA->h) {
			pOutRect->h = pSrcRectA->h;
		} else {
			pOutRect->h = pSrcRectB->h - (pSrcRectA->y - pSrcRectB->y);
		}
	}
	if (pOutRect->h < 0) {
		pOutRect->h = 0;
	}

}


RESULT GxGfx_PushStackDC(DC *pDC,
						 UINT16 uiType, UINT16 uiPxlfmt,
						 UINT32 uiWidth, UINT32 uiHeight)
{
	RESULT r;
	UINT32 *pBuf = 0;

	DBG_FUNC_BEGIN("\r\n");

	if (!pDC) {
		return GX_NULL_POINTER;
	}

	//validate type and format
	r = GxGfx_CheckDCTypeFmt(uiType, uiPxlfmt);
	if (r != GX_OK) {
		return r;
	}

	//alloc memory as buffer
	pBuf = GxGfx_PushStack(GxGfx_CalcDCMemSize(uiWidth, uiHeight, uiType, uiPxlfmt));
	if (!pBuf) {
		return GX_NULL_BUF;
	}

	//create DC at buffer
	return GxGfx_AttachDC((DC *)pDC,
						  uiType, uiPxlfmt, uiWidth, uiHeight, 0, (UINT8 *)pBuf, 0, 0);
}

void GxGfx_PopStackDC(DC *pDC)
{
	DBG_FUNC_BEGIN("\r\n");

	GxGfx_DetachDC(pDC);

	GxGfx_PopStack();
}

