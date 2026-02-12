#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "gximage/gximage.h"

#if _TODO
#include "GxVideoFile.h"
#include "Color.h"
#endif

//-----------------------------------------------------------------------
// Global data release
//-----------------------------------------------------------------------
/*--- Playback HDAL definition: begin---*/
PB_HD_INFO          g_PbHdInfo = {0};
PPB_HD_INFO         g_pPbHdInfo = &g_PbHdInfo;
PB_HD_DEC_INFO      g_HdDecInfo = {0};
PB_HD_COM_BUF       g_hd_file_buf = {0};
PB_HD_COM_BUF       g_hd_raw_buf = {0};
PB_HD_COM_BUF       g_hd_tmp1buf = {0};
PB_HD_COM_BUF       g_hd_tmp2buf = {0};
PB_HD_COM_BUF       g_hd_tmp3buf = {0};
PB_HD_COM_BUF       g_hd_exif_buf = {0}; //only 64KB
/*--- Playback HDAL definition: end---*/
PLAYMODE_INFO       gMenuPlayInfo = {0};
#if _TODO
JPG_DEC_INFO        gDecOneJPGInfo = {0};
#endif
PLAY_SCALE_INFO     gPlayScaleInfo = {0};
GXVIDEO_INFO        g_PBVideoInfo = {0};
PLAY_OBJ            g_PBObj = {0};
PLAY_CMMD_SINGLE    gPBCmdObjPlaySingle = {0};
PLAY_CMMD_BROWSER   gPBCmdObjPlayBrowser = {0};
PLAY_CMMD_USERJPEG  gPBCmdObjPlayUserJPEG = {0};
PLAY_CMMD_SPECFILE  gPBCmdObjPlaySpecFile = {0};
PB_EXIF_ORIENTATION gPB_EXIFOri = {0};
PLAY_CMMD_CROPSAVE  gPBCmdObjCropSave = {0};
PB_SCREEN_CTRL_TYPE gScreenControlType = PB_FLUSH_SCREEN;
//#NT#2013/04/18#Ben Wang -begin
//#NT#Add callback for decoding image and video files.
PB_DECIMG_CB        g_fpDecImageCB = NULL;
PB_DECVIDEO_CB      g_fpDecVideoCB = NULL;
//#NT#2013/04/18#Ben Wang -end
//#NT#2013/07/02#Scottie -begin
//#NT#Add callback for doing something before trigger next (DspSrv)
PB_CFG4NEXT_CB      g_fpCfg4NextCB = NULL;
//#NT#2013/07/02#Scottie -end
//#NT#2012/09/10#Scottie -begin
//#NT#Add to set the default settings of PB
PB_SETTING g_PBSetting = {
	PB_DISPDIR_HORIZONTAL,
	PB_SHOW_THUMBNAIL_IN_THE_SAME_TIME,
	PB_ENABLE_KEEP_ASPECT_RATIO | PB_ENABLE_SPEEDUP_SCREENNAIL,
	TRUE,
	0,
	NULL,
	//#NT#2013/02/20#Lincy Lin -begin
	//#NT#Support Filelist plugin
	NULL,
	//#NT#2013/02/20#Lincy Lin -end
};
//#NT#2012/09/10#Scottie -end
//#NT#2012/11/13#Scottie -begin
//#NT#Support Dual display
PB_DISPLAY_INFO     g_PBDispInfo = {0};
//#NT#2012/11/13#Scottie -end
PB_USER_EDIT        g_PBUserEdit = {0};

BOOL   g_bExifExist = FALSE;
BOOL   g_bPBSaveOverWrite = FALSE;
UINT8  gucPlayReqtyQuality;
UINT8  gucPlayTskWorking = FALSE;
UINT8  gucZoomUpdateNewFB = FALSE;
UINT8  gusCurrPlayStatus;
UINT8  gucRotatePrimary = FALSE;
UINT8  gucCurrPlayZoomCmmd;
UINT16 gusPlayResizeNewWidth, gusPlayResizeNewHeight;
UINT32 g_uiPBBufStart, g_uiPBBufEnd;
UINT32 guiPlayAllBufSize, guiPlayMaxFileBufSize;
UINT32 guiPlayFileBuf, guiPlayRawBuf;
UINT32 g_uiPBFBBuf, g_uiPBTmpBuf, g_uiPBIMETmpBuf;
UINT32 guiPlayIMETmpBufSize;
UINT32 g_uiPBRotateDir;
UINT32 guiUserJPGOutWidth, guiUserJPGOutHeight;
UINT32 guiUserJPGOutStartX, guiUserJPGOutStartY;
UINT16 gusPlayFileFormat = PB_SUPPORT_FMT;
UINT16 gusPlayThisFileFormat;
UINT32 guiCurrPlayCmmd;
UINT32 guiOnPlaybackMode = FALSE;
UINT16 gusPlayZoomUserLeftUp_X, gusPlayZoomUserLeftUp_Y, gusPlayZoomUserRightDown_X, gusPlayZoomUserRightDown_Y;
UINT32 g_uiMAXPanelSize = 1920*1080; //Default. This is equal for real display size.
UINT32 g_uiMaxFileSize = 0;
UINT32 g_uiMaxRawSize = 0;
UINT32 g_uiMaxDecodeW = 0;
UINT32 g_uiMaxDecodeH = 0;
UINT32 g_uiExifOrientation;
UINT32 g_uiPbDebugMsg = FALSE;


PB_BRC_INFO g_stPBBRCInfo = {
	PB_BRC_DEFAULT_COMPRESS_RATIO,
	PB_BRC_DEFAULT_UPBOUND_RATIO,
	PB_BRC_DEFAULT_LOWBOUND_RATIO,
	PB_BRC_DEFAULT_LIMIT_CNT
};

PB_CAPTURESN_INFO g_stPBCSNInfo = {0};
UINT32 g_uiDefaultBGColor = COLOR_YUV_BLACK;

//#NT#2012/06/21#Lincy Lin -begin
//#NT#Porting disply Srv
UINT32 guiIsSuspendPB = FALSE;
//#NT#2012/06/21#Lincy Lin -end

HD_VIDEO_FRAME  gPlayFBImgBuf = {0};
HD_VIDEO_FRAME  gPBImgBuf[PB_DISP_SRC_COUNT];
HD_VIDEO_FRAME  *g_pDecodedImgBuf = &gPBImgBuf[PB_DISP_SRC_DEC];
HD_VIDEO_FRAME  *g_pPlayTmpImgBuf = &gPBImgBuf[PB_DISP_SRC_TMP];
HD_VIDEO_FRAME  *g_pPlayIMETmpImgBuf = &gPBImgBuf[PB_DISP_SRC_IMETMP];
HD_VIDEO_FRAME  *g_pThumb1ImgBuf = &gPBImgBuf[PB_DISP_SRC_THUMB1];
HD_VIDEO_FRAME  *g_pThumb2ImgBuf = &gPBImgBuf[PB_DISP_SRC_THUMB2];

UINT32 ThumbSrcW[PLAY_MAX_THUMB_NUM], ThumbSrcH[PLAY_MAX_THUMB_NUM];
UINT32 gSrcWidth, gSrcHeight;
UINT32 gPbPushSrcIdx = 0;

IRECT gSrcRegion = {0};
IRECT gDstRegion = {0};
URECT g_rcPbDetailWIN = {0};
//#NT#2012/11/21#Scottie -begin
//#NT#Support Dual Display for PB
// video output window (draw image in this specific area)
URECT g_PBVdoWIN[PBDISP_IDX_MAX] = {0};
URECT g_PB1stVdoRect[PBDISP_IDX_MAX] = {0};
URECT g_PBIdxView[PBDISP_IDX_MAX][PLAY_MAX_THUMB_NUM] = {0};
//#NT#2012/11/21#Scottie -end

PB_DISPTRIG_CB g_fpDispTrigCB = NULL;
PB_ONDRAW_CB g_fpOnDrawCB  = NULL;
BOOL gDualDisp = FALSE;
BOOL gScaleTwice = TRUE;


