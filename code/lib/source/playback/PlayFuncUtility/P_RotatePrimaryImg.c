#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    Rotate primary image
    This is internal API.

    @param[in] uiRotateDir:
    @return void
*/

//#NT#2011/10/17#Lincy Lin -begin
//#NT#650GxImage

UINT32 PB_GetGxImgRotateDir(UINT32 uiRotateDir)
{
#if _TODO
	UINT32 GxRotateDir;
	switch (uiRotateDir) {
	case PLAY_ROTATE_DIR_90:
		GxRotateDir = VDO_DIR_ROTATE_90;
		break;
	case PLAY_ROTATE_DIR_270:
		GxRotateDir = VDO_DIR_ROTATE_270;
		break;
	case PLAY_ROTATE_DIR_180:
		GxRotateDir = VDO_DIR_ROTATE_180;
		break;

	case PLAY_ROTATE_DIR_VER:
		GxRotateDir = GX_IMAGE_ROTATE_VTC;
		break;
	case PLAY_ROTATE_DIR_HOR:
	default:
		GxRotateDir = GX_IMAGE_ROTATE_HRZ;
		break;
	}
	default:
		GxRotateDir = VDO_DIR_NONE;
		break;
	}
	return GxRotateDir;
#else
   return 0;
#endif
}

void PB_RotatePrimaryHandle(UINT32 uiRotateDir)
{
#if _TODO
	UINT32 GxRotateDir;
	UINT32 StartAddr, EndAddr = 0;

	GxRotateDir = PB_GetGxImgRotateDir(uiRotateDir);
	//gximg_get_buf_addr(g_pDecodedImgBuf, &StartAddr, &EndAddr);
	// do rotate
	gximg_self_rotate(g_pDecodedImgBuf, EndAddr, guiPlayFileBuf - EndAddr, GxRotateDir);
#endif	
}
//#NT#2011/10/17#Lincy Lin -end

