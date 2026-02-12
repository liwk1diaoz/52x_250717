#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
//#include "Color.h"

/*
    Show all images in thumbnail mode.
    This is internal API.

    @param[in] SrcWidth
    @param[in] SrcHeight
    @param[in] index
    @return void
*/
void PB_Thumb2Display(UINT32 SrcWidth, UINT32 SrcHeight, UINT32 index)
{
#if _TODO
	UINT8  ucOrientation;
	UINT32 uiIdx, uiTmpW, uiTmpH, uiTmpBufSZ;
	//HD_VIDEO_FRAME *pTmpThumbImg;

	ThumbSrcW[index] = SrcWidth;
	ThumbSrcH[index] = SrcHeight;

	/* Thumb img is overlap with play tmp*/
	uiIdx = (index & 0x1);
	pTmpThumbImg = &gPBImgBuf[PB_DISP_SRC_THUMB1 + uiIdx];
//#NT#2016/03/25#Scottie -begin
//#NT#Modify for better image quality (for HDMI)
	if (SrcWidth > SrcHeight) {
		if (SrcWidth > DCF_THUMB_PIXEL_H) {
			uiTmpW = DCF_THUMB_PIXEL_H;
			uiTmpH = DCF_THUMB_PIXEL_V;
		} else {
			uiTmpW = SrcWidth;
			uiTmpH = SrcHeight;
		}
	} else {
		if (SrcWidth > DCF_THUMB_PIXEL_H) {
			uiTmpW = DCF_THUMB_PIXEL_V;
			uiTmpH = DCF_THUMB_PIXEL_H;
		} else {
			uiTmpW = SrcHeight;
			uiTmpH = SrcWidth;
		}
	}
	uiTmpBufSZ = uiTmpW * uiTmpH * 2;

//#NT#2016/05/10#Scottie#[0100493] -begin
//#NT#Correct the pixel format for Video/JPEG
//    GxImg_InitBuf(pTmpThumbImg, uiTmpW, uiTmpH, GX_IMAGE_PIXEL_FMT_YUV422, GXIMAGE_LINEOFFSET_ALIGN(8), g_uiPBTmpBuf+(uiTmpBufSZ*uiIdx), uiTmpBufSZ);
	//gximg_init_buf(pTmpThumbImg, uiTmpW, uiTmpH, g_pDecodedImgBuf->pxlfmt, GXIMG_LINEOFFSET_ALIGN(8), g_uiPBTmpBuf + (uiTmpBufSZ * uiIdx), uiTmpBufSZ);
//#NT#2016/05/10#Scottie#[0100493] -end
//#NT#2016/03/25#Scottie -end

	gPbPushSrcIdx = PB_DISP_SRC_DEC;
	if (g_bExifExist) {
		ucOrientation = g_uiExifOrientation;
	} else {
		ucOrientation = JPEG_EXIF_ORI_DEFAULT;
	}

	// Below is rotate case , not support yet
	if (ucOrientation != JPEG_EXIF_ORI_DEFAULT) {

		//UINT32 uiRotateDir;
		switch (ucOrientation) {
		case JPEG_EXIF_ORI_ROTATE_270:
			//uiRotateDir = HD_VIDEO_DIR_ROTATE_270;
			break;

		case JPEG_EXIF_ORI_ROTATE_180:
			//uiRotateDir = HD_VIDEO_DIR_ROTATE_180;
			break;

		case JPEG_EXIF_ORI_ROTATE_90:
			//uiRotateDir = HD_VIDEO_DIR_ROTATE_90;
			break;

		default:
			DBG_ERR("error rotate dir %d\r\n", ucOrientation);
			return;
		}
		//gximg_rotate_data(g_pDecodedImgBuf, g_uiPBIMETmpBuf, guiPlayIMETmpBufSize, uiRotateDir, g_pPlayIMETmpImgBuf);
		gPbPushSrcIdx = PB_DISP_SRC_IMETMP;
	}
//#NT#2016/03/25#Scottie -begin
//#NT#Remove for better image quality (for HDMI)
#if 0
	else {
		if (SrcWidth > 320) {
			// There isn't thumbnail image.
			// 1. Scale primary image to 320x240

			// Normal, Width > Height
			if (SrcWidth > SrcHeight) {
				GxImg_InitBuf(g_pPlayIMETmpImgBuf, 320, 240, GX_IMAGE_PIXEL_FMT_YUV422, GXIMAGE_LINEOFFSET_ALIGN(8), g_uiPBIMETmpBuf, 320 * 240 * 2);
			} else {
				GxImg_InitBuf(g_pPlayIMETmpImgBuf, 240, 320, GX_IMAGE_PIXEL_FMT_YUV422, GXIMAGE_LINEOFFSET_ALIGN(8), g_uiPBIMETmpBuf, 320 * 240 * 2);
			}
			GxImg_FillData(g_pPlayIMETmpImgBuf, NULL, COLOR_YUV_BLACK);
			GxImg_ScaleDataFine(g_pDecodedImgBuf, REGION_MATCH_IMG, g_pPlayIMETmpImgBuf, REGION_MATCH_IMG);
			gPbPushSrcIdx = PB_DISP_SRC_IMETMP;
		}
	}
#endif
//#NT#2016/03/25#Scottie -end
	if (gPbPushSrcIdx == PB_DISP_SRC_DEC) {
		//gximg_scale_data(g_pDecodedImgBuf, GXIMG_REGION_MATCH_IMG, pTmpThumbImg, GXIMG_REGION_MATCH_IMG);
	} else {
		//gximg_scale_data(g_pPlayIMETmpImgBuf, GXIMG_REGION_MATCH_IMG, pTmpThumbImg, GXIMG_REGION_MATCH_IMG);
	}
	gPbPushSrcIdx = PB_DISP_SRC_THUMB1 + uiIdx;
#endif	
}

