#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

#if _TODO
#include "GxImageFile.h"
#endif

INT32 PB_JpgBRCHandler(HD_VIDEO_FRAME *pSrcImgBuf, PMEM_RANGE pDstBitstream)
{
	#if _TODO
	INT32 iErrChk = E_OK;
	UINT32 uiCompressRatio, uiUpBoundRatio, uiLowBoundRatio, uiTargetSize, uiAvailableSize;

	uiCompressRatio = g_stPBBRCInfo.uiCompressRatio;
	uiUpBoundRatio = g_stPBBRCInfo.uiUpBoundRatio;
	uiLowBoundRatio = g_stPBBRCInfo.uiLowBoundRatio;
	uiAvailableSize = pDstBitstream->Size;

	if (pSrcImgBuf->PxlFmt == GX_IMAGE_PIXEL_FMT_YUV420_PLANAR || pSrcImgBuf->PxlFmt == GX_IMAGE_PIXEL_FMT_YUV420_PACKED) {
		uiTargetSize = pSrcImgBuf->Width * pSrcImgBuf->Height * 3 * uiCompressRatio / 100 / 2;
	} else { //422 or default
		uiTargetSize = pSrcImgBuf->Width * pSrcImgBuf->Height * 2 * uiCompressRatio / 100;
	}
	DBG_MSG("uiAvailableSize=%d, uiTargetSize=%d\r\n", uiAvailableSize, uiTargetSize);

	if (uiTargetSize * (100 + uiUpBoundRatio) / 100 > uiAvailableSize) {
		uiTargetSize = uiAvailableSize * 100 / (100 + uiUpBoundRatio);
		uiCompressRatio = GxImgFile_CalcCompressedRatio(pSrcImgBuf, uiTargetSize);
		DBG_WRN("Mem insufficient! Use remaining buffer size(%d) to be the target size.\r\n", uiAvailableSize);
	}
	DBG_MSG("Original CompressRatio=%d, Revised CompressRatio=%d\r\n", g_stPBBRCInfo.uiCompressRatio, uiCompressRatio);

	GxImgFile_Config(IMGENC_CFG_BRC_UPPER_BOUND_RATIO, uiUpBoundRatio);
	GxImgFile_Config(IMGENC_CFG_BRC_LOWER_BOUND_RATIO, uiLowBoundRatio);
	GxImgFile_Config(IMGENC_CFG_BRC_MAX_RETRY_CNT, g_stPBBRCInfo.uiReEncCnt);


	iErrChk = GxImgFile_EncodeByBitrate(pSrcImgBuf, pDstBitstream, uiCompressRatio);

	return iErrChk;
	#else
	return E_OK;
	#endif
}
