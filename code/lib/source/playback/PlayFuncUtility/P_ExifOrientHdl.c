#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    According to EXIF Orientation tag to auto-rotate image
    This is internal API.

    @return TRUE: done; FALSE: fail
*/
BOOL PB_EXIFOrientationHandle(void)
{
	UINT32 uiOrientation;

	if (g_bExifExist) {
		uiOrientation = g_uiExifOrientation;
	} else {
		uiOrientation = JPEG_EXIF_ORI_DEFAULT;
	}
	if ((uiOrientation == JPEG_EXIF_ORI_DEFAULT) ||
		(uiOrientation == JPEG_EXIF_ORI_FLIP_HORIZONTAL) ||
		(uiOrientation == JPEG_EXIF_ORI_FLIP_VERTICAL) ||
		(uiOrientation == JPEG_EXIF_ORI_TRANSPOSE) ||
		(uiOrientation == JPEG_EXIF_ORI_TRANSVERSE)) {
		if (g_PBSetting.DispDirection != PB_DISPDIR_VERTICAL) {
			return FALSE;
		}
	}

	if (g_PBSetting.DispDirection == PB_DISPDIR_VERTICAL) {
		switch (uiOrientation) {
		case JPEG_EXIF_ORI_DEFAULT:
			g_uiExifOrientation = JPEG_EXIF_ORI_ROTATE_270;
			g_uiPBRotateDir = PLAY_ROTATE_DIR_270;
			break;

		case JPEG_EXIF_ORI_ROTATE_270:
			g_uiExifOrientation = JPEG_EXIF_ORI_ROTATE_180;
			g_uiPBRotateDir = PLAY_ROTATE_DIR_180;
			break;

		case JPEG_EXIF_ORI_ROTATE_180:
			g_uiExifOrientation = JPEG_EXIF_ORI_ROTATE_90;
			g_uiPBRotateDir = PLAY_ROTATE_DIR_90;
			break;

		default:
			return FALSE;
		}

		gMenuPlayInfo.RotateDir = PLAY_ROTATE_DIR_UNKNOWN;
	} else {
		switch (uiOrientation) {
		case JPEG_EXIF_ORI_ROTATE_270:
			g_uiPBRotateDir = PLAY_ROTATE_DIR_270;
			break;

		case JPEG_EXIF_ORI_ROTATE_180:
			g_uiPBRotateDir = PLAY_ROTATE_DIR_180;
			break;

		case JPEG_EXIF_ORI_ROTATE_90:
			g_uiPBRotateDir = PLAY_ROTATE_DIR_90;
			break;
		}
	}

	// Do Rotate function.
	PB_RotateDisplayHandle(FALSE);

	return TRUE;
}