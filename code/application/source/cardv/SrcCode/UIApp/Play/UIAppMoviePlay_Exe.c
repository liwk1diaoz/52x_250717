#include "PlaybackTsk.h"
#include "UIApp/Play/UIAppMoviePlay.h"
#include "PrjInc.h"
#include "NVTUserCommand.h"
#include "FileDB.h"
#include "PrjCfg.h"
#include "PBXFileList/PBXFileList_DCF.h"
#include "PBXFileList/PBXFileList_FileDB.h"
#include "ImageApp/ImageApp_MoviePlay.h"
#include "ImageApp/ImageApp_Play.h"
#include "SysMain.h"
#include "UIApp/AppDisp_PBView.h"
#include "sys_mempool.h"
#include "GxVideoFile.h"
#include <kwrap/debug.h>
#include "UIWnd/UIFlow.h"
#if (USE_DCF == ENABLE)
#include "DCF.h"
#endif


#if 0
#define MoviePlay_Open(x)                        SMediaPlay_Open((PSMEDIAPLAY_USEROBJ)x)
#define MoviePlay_WaitReady(x)                   SMediaPlay_WaitReady(x)
#define MoviePlay_Close(x)                       SMediaPlay_Close(x)
#define MoviePlay_Pause(x)                       SMediaPlay_PausePlay(x)
#define MoviePlay_GetMediaInfo(x)                SMediaPlay_GetMediaInfo(x)
#define MoviePlay_GetCurrFrame(x)                SMediaPlay_GetCurrVideoFrame(x)
#define MoviePlay_GetNowDisplayFrame(x,y,z)      SMediaPlay_GetNowDisplayFrame(x,y,z)
#define MoviePlay_StartPlay(x,y)                 SMediaPlay_StartPlay(x,y)
#define MoviePlay_FWDOneStep(x)                  SMediaPlay_VideoOneStepPlay(SMEDIAPLAY_DIR_FORWARD)
#define MoviePlay_BWDOneStep(x)                  SMediaPlay_VideoOneStepPlay(SMEDIAPLAY_DIR_BACKWARD)
#define MoviePlay_GetVideoFrameRate(x)           SMediaPlay_GetVideoFrameRate(x)
#define MoviePlay_GetTotalVideoFrame(x)          SMediaPlay_GetTotalVideoFrame(x)
#define MoviePlay_GetCreateModifyTime(x,y)       SMediaPlay_GetCreateModifyTime(x,y)
#define MoviePlay_GetThisVideoFrameOffset(x,y)   SMediaPlay_GetThisVideoFrameOffset(x,y)
#define MoviePlay_JumpVideoToSpecificFrame(x)    SMediaPlay_JumpVideoToSpecFramePlay(x)
#define MoviePlay_RestartTo1stFrame(x)           SMediaPlay_Restart1stFrmPlay(x)
#define MoviePlay_SetOneDispFrameEvent(x)
#define MoviePlay_Video_Step(x)
#define MoviePlay_WaitStepFinish(x)
#define MoviePlay_SetAudVolume(x)                SMediaPlay_SetAudVolume(x)
#define MoviePlay_GetCurrPlaySecs(x)             SMediaPlay_GetCurrPlaySecs(x)
#else
#define MoviePlay_Open(x)
#define MoviePlay_WaitReady(x)
#define MoviePlay_Close(x)
#define MoviePlay_Pause(x)
#define MoviePlay_GetMediaInfo(x)
#define MoviePlay_GetCurrFrame(x)
#define MoviePlay_GetNowDisplayFrame(x,y,z)
#define MoviePlay_StartPlay(x,y)
#define MoviePlay_FWDOneStep(x)
#define MoviePlay_BWDOneStep(x)
#define MoviePlay_GetVideoFrameRate(x)
#define MoviePlay_GetTotalVideoFrame(x)
#define MoviePlay_GetCreateModifyTime(x,y)
#define MoviePlay_GetThisVideoFrameOffset(x,y)
#define MoviePlay_JumpVideoToSpecificFrame(x)
#define MoviePlay_RestartTo1stFrame(x)
#define MoviePlay_SetOneDispFrameEvent(x)
#define MoviePlay_Video_Step(x)
#define MoviePlay_WaitStepFinish(x)
#define MoviePlay_SetAudVolume(x)
#define MoviePlay_GetCurrPlaySecs(x)
#endif

///////////////////////////////////////////////////////////////////////////////

//header
#define DBGINFO_BUFSIZE()	(0x200)

//RAW
#define VDO_RAW_BUFSIZE(w, h, pxlfmt)   (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NRX: RAW compress: Only support 12bit mode
#define RAW_COMPRESS_RATIO 59
#define VDO_NRX_BUFSIZE(w, h)           (ALIGN_CEIL_4(ALIGN_CEIL_64(w) * 12 / 8 * RAW_COMPRESS_RATIO / 100 * (h)))
//CA for AWB
#define VDO_CA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 3) << 1)
//LA for AE
#define VDO_LA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 1) << 1)

//YUV
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	(ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NVX: YUV compress
#define YUV_COMPRESS_RATIO 75
#define VDO_NVX_BUFSIZE(w, h, pxlfmt)	(VDO_YUV_BUFSIZE(w, h, pxlfmt) * YUV_COMPRESS_RATIO / 100)

///////////////////////////////////////////////////////////////////////////////

static UINT32         g_uiUIFlowWndPlayCurrenSpeed     = SMEDIAPLAY_SPEED_NORMAL;
static UINT32         g_uiUIFlowWndPlayCurrenDirection = SMEDIAPLAY_DIR_FORWARD;
static MOVIEPLAY_FILEPLAY_INFO gMovie_Play_Info         = {0};
static MOVIEPLAY_CFG_DISP_INFO gMovPlay_Disp_Info;
static MOVIEPLAY_AOUT_INFO_EX  g_aout_info_ex = {0};
GXVIDEO_INFO g_MovieInfo;

///////////////////////////////////////////////////////////////////////////////
static HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

static void _mem_pool_init(void)
{
	UINT32 i = 0;
	HD_VIDEO_FRAME hd_vdoframe_info = {0};
	ISIZE  disp_size = GxVideo_GetDeviceSize(DOUT1);

	// config common pool (decode)
	PB_GetParam(PBPRMID_INFO_IMG, (UINT32 *)&hd_vdoframe_info);

	mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
#if PLAY_THUMB_AND_MOVIE // fixed common pool size to 2560x1440 for playing thumbnail and movie together
	mem_cfg.pool_info[i].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_64(2560), ALIGN_CEIL_64(1440), HD_VIDEO_PXLFMT_YUV420);  // align to 64 for h265 raw buffer
#else
	mem_cfg.pool_info[i].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_64(hd_vdoframe_info.dim.w), ALIGN_CEIL_64(hd_vdoframe_info.dim.h), HD_VIDEO_PXLFMT_YUV420);  // align to 64 for h265 raw buffer
#endif
	/* ********************************************************
	 * IVOT_N00028-346
	 * 1.YUV blk cnt can be lowered to 3 from 5 (In general,only 3 blocked are needed. (SVC+1 or LTR +1))
	 * 2.remove unused blocks(HD_COMMON_MEM_USER_POOL_BEGIN)
	 * ********************************************************/
#if (_BOARD_DRAM_SIZE_ <= 0x04000000)
	mem_cfg.pool_info[i].blk_cnt = 3;
#else
	mem_cfg.pool_info[i].blk_cnt = 5;
#endif
	mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;
	// config common pool (scale & display)
	mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[i].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_16(disp_size.w), ALIGN_CEIL_16(disp_size.h), HD_VIDEO_PXLFMT_YUV420);  // align to 16 for rotation panel
	mem_cfg.pool_info[i].blk_cnt = 5;
	mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;
	// config common pool for filein bs block
	mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	#if (_BOARD_DRAM_SIZE_ == 0x04000000)
	mem_cfg.pool_info[i].blk_size = 16 * 1024 * 1024;
	#else
	mem_cfg.pool_info[i].blk_size = 20 * 1024 * 1024;  // default at least 20MBytes for filein bitstream block.
	#endif
	mem_cfg.pool_info[i].blk_cnt = 1;
	mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;
	// config common pool (main)
	mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[i].blk_size = 0x1000;
	mem_cfg.pool_info[i].blk_cnt = 1;
	mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;

#if PLAY_DEWARP == ENABLE
	mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[i].blk_size = ALIGN_CEIL_64(PB_MAX_VIDEO_W)*ALIGN_CEIL_64(PB_MAX_VIDEO_H)*3/2 + 1024;
	mem_cfg.pool_info[i].blk_cnt = 2;
	mem_cfg.pool_info[i].ddr_id = DDR_ID0;
	i++;
#endif
}

void UIAppMoviePlayExe_MoviePlayCB(UINT32 uiEventID, UINT32 p1, UINT32 p2, UINT32 p3)
{
	DBG_IND("MoviePlayCB event=%d\r\n", uiEventID);

	switch (uiEventID) {
	case MOVIEPLAY_EVENT_MEDIAINFO_READY:
		break;
	case MOVIEPLAY_EVENT_STOP:
		Ux_PostEvent(NVTEVT_CB_MOVIE_FINISH, 0);
		break;
	case MOVIEPLAY_EVENT_ONE_SECOND:
		DBG_DUMP("play sec: %d! (idx:%lu)\r\n", p1, p2);
		FlowPB_SetMovPlayTime(p1);
		Ux_PostEvent(NVTEVT_CB_MOVIE_ONE_SEC, 3, p1, p2, p3);
		break;
	case MOVIEPLAY_EVENT_ONE_DISPLAYFRAME:
#if PLAY_DEWARP == ENABLE
		PBView_DrawSingleView_Dewarp((HD_VIDEO_FRAME *)p1);
#else
		PBView_DrawSingleView((HD_VIDEO_FRAME *)p1);
#endif
		break;
	case MOVIEPLAY_EVENT_DECODE_ERR:
	{
		MOVIEPLAY_EVENT_DECODE_ERR_INFO* info = (MOVIEPLAY_EVENT_DECODE_ERR_INFO*)p1;
		if(info->ret == HD_ERR_BAD_DATA){
			DBG_DUMP("media stream has bad data(%s decode error), stop play\r\n", info->is_video == TRUE? "video" : "audio");
			Ux_PostEvent(NVTEVT_CB_MOVIE_FINISH, 0);
			Ux_PostEvent(NVTEVT_CB_MOVIE_ERR, 0);
		}
	}
		break;

	default:
		DBG_ERR("Invalid eventID=0x%x\r\n", uiEventID);
		break;
	}	
}

/* back to playback mode */
void UIAppMoviePlayExe_MoviePlay_ConfigErr(void)
{
	Ux_PostEvent(NVTEVT_CB_MOVIE_FINISH, 0);
	Ux_PostEvent(NVTEVT_CB_MOVIE_ERR, 0);
}

UINT32 MoviePlayExe_LibGetTotalSecs(void)
{
	PB_GetParam(PBPRMID_INFO_VDO, (UINT32 *)&g_MovieInfo);

	return (g_MovieInfo.uiToltalSecs);
}
UINT32 MoviePlayExe_LibGetEV(void)
{
	return 0;
}
UINT32 MoviePlayExe_LibGetVideoWidth(void)
{
	PB_GetParam(PBPRMID_INFO_VDO, (UINT32 *)&g_MovieInfo);

	return (g_MovieInfo.uiVidWidth);
}

BOOL MoviePlayExe_VDEC_Skip_SVC_Condition_CB(
		const MOVIEPLAY_VDEC_SKIP_SVC_FRAME_BS_INFO* bs_info,
		const void* user_data)
{
	/* display limitation */
	if(bs_info->fps > 60)
		return TRUE;
	/* codec limitation(4KP30) */
	else if((bs_info->fps > 30) && ((bs_info->width * bs_info->height) >= 3840 * 2160))
		return TRUE;
	else
		return FALSE;
}

INT32 MoviePlayExe_OpenPlay(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	FST_FILE       MovPlay_Filehdl         = NULL;

	DBG_IND("*************************************\n");
	DBG_IND("*** imageapp_movieplay open test code ***\n");
	DBG_IND("*************************************\n");

	if (paramNum > 0) {
		MovPlay_Filehdl = (FST_FILE)paramArray[0];
	} else {
		DBG_ERR("Need movie file handle\r\n");
		return NVTEVT_CONSUME;	
	}

    // Config max mem info
	_mem_pool_init();
	if(ImageApp_MoviePlay_Config(MOVIEPLAY_CONFIG_MEM_POOL_INFO, (UINT32)&mem_cfg) != ISF_OK){
		UIAppMoviePlayExe_MoviePlay_ConfigErr();
		return NVTEVT_CONSUME;
	}

	if (!MovPlay_Filehdl) {
		DBG_ERR("UIFlowWndPlay_OnKeySelect: Can't open Video file!\r\n");
		UIAppMoviePlayExe_MoviePlay_ConfigErr();
		return NVTEVT_CONSUME;
	}

#if 0 /* decrypt */

	/* **************************************************************************
	 * IMPORTANT!!
	 *
	 * MOVIEPLAY_CONFIG_DECRYPT_INFO should be configured before MOVIEPLAY_CONFIG_FILEPLAY_INFO
	 *
	 * **************************************************************************/
	UINT8 key[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	MOVIEPLAY_FILE_DECRYPT crypt_info = {
		.type    = MOVIEPLAY_DECRYPT_TYPE_CONTAINER | MOVIEPLAY_DECRYPT_TYPE_ALL_FRAME | MOVIEPLAY_DECRYPT_TYPE_AUDIO,
		.mode    = MOVIEPLAY_DECRYPT_MODE_AES256,
		.key     = key,
		.key_len = 32,
	};

	if(ImageApp_MoviePlay_Config(MOVIEPLAY_CONFIG_DECRYPT_INFO, (UINT32)&crypt_info) != ISF_OK){
		UIAppMoviePlayExe_MoviePlay_ConfigErr();
		return NVTEVT_CONSUME;
	}

#endif

	gMovie_Play_Info.fileplay_id = _CFG_FILEPLAY_ID_1;
	gMovie_Play_Info.file_handle = MovPlay_Filehdl;
	gMovie_Play_Info.curr_speed  = g_uiUIFlowWndPlayCurrenSpeed;
	gMovie_Play_Info.curr_direct = g_uiUIFlowWndPlayCurrenDirection;
	gMovie_Play_Info.event_cb    = UIAppMoviePlayExe_MoviePlayCB;
	/******************************************************************************
	 * IMPORTANT!!
	 * ts_demux_buf_size = 0 keeps default value for ts demux buffer,
	 * enlarge this value if ERR:TsReadLib_Parse1stVideo buffer size err occurred
	 ******************************************************************************/
//	gMovie_Play_Info.ts_demux_buf_size = 0;
	gMovie_Play_Info.ts_demux_buf_size = 0x120000;
	if(ImageApp_MoviePlay_Config(MOVIEPLAY_CONFIG_FILEPLAY_INFO, (UINT32) &gMovie_Play_Info) != ISF_OK){
		UIAppMoviePlayExe_MoviePlay_ConfigErr();
		return NVTEVT_CONSUME;
	}

	// config vout.
	gMovPlay_Disp_Info.vout_ctrl = GxVideo_GetDeviceCtrl(DOUT1,DISPLAY_DEVCTRL_CTRLPATH);
	gMovPlay_Disp_Info.vout_path = GxVideo_GetDeviceCtrl(DOUT1,DISPLAY_DEVCTRL_PATH);
	gMovPlay_Disp_Info.ratio = GxVideo_GetDeviceAspect(0);

	/* do not change queue depth */
	gMovPlay_Disp_Info.queue_depth = 2;

	if(ImageApp_MoviePlay_Config(MOVIEPLAY_CONFIG_DISP_INFO, (UINT32)&gMovPlay_Disp_Info) != ISF_OK){
		UIAppMoviePlayExe_MoviePlay_ConfigErr();
		return NVTEVT_CONSUME;
	}

	// config aout.
	g_aout_info_ex.drv_cfg.mono = HD_AUDIO_MONO_LEFT;
	g_aout_info_ex.drv_cfg.output = HD_AUDIOOUT_OUTPUT_LINE;
	g_aout_info_ex.pwr_en = TRUE;
	if(ImageApp_MoviePlay_Config(MOVIEPLAY_CONFIG_AUDOUT_INFO_EX, (UINT32)&g_aout_info_ex) != ISF_OK){
		UIAppMoviePlayExe_MoviePlay_ConfigErr();
		return NVTEVT_CONSUME;
	}

	{
		MOVIEPLAY_VDEC_SKIP_SVC_FRAME vdec_skip_svc_frame = {0};

		vdec_skip_svc_frame.enabled = 1;
		vdec_skip_svc_frame.user_data = NULL;
		vdec_skip_svc_frame.condition_callback = MoviePlayExe_VDEC_Skip_SVC_Condition_CB;

		if(ImageApp_MoviePlay_Config(MOVIEPLAY_CONFIG_VDEC_SKIP_SVC_FRAME, (UINT32)&vdec_skip_svc_frame) != ISF_OK){
			UIAppMoviePlayExe_MoviePlay_ConfigErr();
			return NVTEVT_CONSUME;
		}
	}

    if(ImageApp_MoviePlay_Open() != ISF_OK){
		UIAppMoviePlayExe_MoviePlay_ConfigErr();
		return NVTEVT_CONSUME;
    }

	PBView_videoout_task_create();

#if PLAY_DEWARP == ENABLE
	UIAppPlay_Dewarp_Init((HD_DIM){PB_MAX_VIDEO_W, PB_MAX_VIDEO_H}, NULL);
#endif

	return NVTEVT_CONSUME;
}

INT32 MoviePlayExe_ClosePlay(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
#if 0	
	MoviePlay_Pause();
	MoviePlay_Close();
#else
	ImageApp_MoviePlay_Close();
	PBView_videoout_task_destroy();

#if 0
	if (gMovie_Play_Info.file_handle != NULL)
		FileSys_CloseFile(gMovie_Play_Info.file_handle);
#endif	

	#if 0
	ImageApp_Play_Open();
	#endif
#endif

#if PLAY_DEWARP == ENABLE
	UIAppPlay_Dewarp_Uninit();
#endif

	return NVTEVT_CONSUME;
}

INT32 MoviePlayExe_PausePlay(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	ImageApp_MoviePlay_FilePlay_Pause();
	return NVTEVT_CONSUME;
}

INT32 MoviePlayExe_ResumePlay(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	ImageApp_MoviePlay_FilePlay_Resume();
	return NVTEVT_CONSUME;
}

INT32 MoviePlayExe_StartPlay(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
#if 0	
	UINT32 uiSpeedLevel, direction;

	if (paramNum != 2) {
		return NVTEVT_CONSUME;
	}

	uiSpeedLevel = paramArray[0];
	direction    = paramArray[1];

	MoviePlay_StartPlay(uiSpeedLevel, direction);
#else
	ImageApp_MoviePlay_Start();
#endif	

	return NVTEVT_CONSUME;
}

UINT32 MoviePlayExe_GetDataPlay(PLAYMOVIE_GET_DATATYPE dataType)
{
	UINT32 retV = 0;
	switch (dataType) {
#if (USE_DCF == ENABLE)
	case PLAYMOVIE_DIRID:
		retV = DCF_GetDBInfo(DCF_INFO_CUR_DIR_ID);
		break;
	case PLAYMOVIE_FILEID:
		retV = DCF_GetDBInfo(DCF_INFO_CUR_FILE_ID);
		break;
#endif
	case PLAYMOVIE_TOTAL_SECOND:
		retV = MoviePlayExe_LibGetTotalSecs();
		break;
	case PLAYMOVIE_EV:
		retV = MoviePlayExe_LibGetEV();
		break;
	case PLAYMOVIE_WIDTH:
		retV = MoviePlayExe_LibGetVideoWidth();
		break;
#if (USE_DCF == ENABLE)
	case PLAYMOVIE_LOCKSTATUS: {
			UINT32 CurIndex = 0, DirectoryID = 0, FilenameID = 0, uiThisFileFormat = 0;
			CHAR   filePath[DCF_FULL_FILE_PATH_LEN];
			BOOL   ReturnValue = FALSE;
			UINT8  uAttrib = 0;

			CurIndex = DCF_GetDBInfo(DCF_INFO_CUR_INDEX);
			DCF_GetObjInfo(CurIndex, &DirectoryID, &FilenameID, &uiThisFileFormat);
			DCF_GetObjPath(CurIndex, uiThisFileFormat, filePath);
			FileSys_GetAttrib(filePath, &uAttrib);
			if (M_IsReadOnly(uAttrib)) {
				ReturnValue = TRUE;
			} else {
				ReturnValue = FALSE;
			}
			retV = ReturnValue;
		}
		break;
	case PLAYMOVIE_FILENUM:
		retV = DCF_GetDBInfo(DCF_INFO_TOL_FILE_COUNT);
		break;
#endif
	case PLAYMOVIE_OPENED:
		retV = TRUE; // Always return true.
		break;
	case PLAYMOVIE_CURR_SECOND:
		//retV = MoviePlay_GetCurrPlaySecs();
		break;
	case PLAYMOVIE_CURR_FRAME:
		//retV = MoviePlay_GetCurrFrame();
		break;
	case PLAYMOVIE_TOTAL_FRAME:
		//retV = MoviePlay_GetTotalVideoFrame();
		break;

	default:
		DBG_IND("Unknown type=%d\r\n", dataType);
		break;
	}
	DBG_IND("-type=0x%x,v=0x%x\r\n", dataType, retV);
	return retV;
}

INT32 MoviePlayExe_SwitchFirstFrame(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	MoviePlay_RestartTo1stFrame();
	return NVTEVT_CONSUME;
}

INT32 MoviePlayExe_PlayStepFwd(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	MoviePlay_FWDOneStep();
	return NVTEVT_CONSUME;
}
INT32 MoviePlayExe_PlayStepBwd(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	MoviePlay_BWDOneStep();
	return NVTEVT_CONSUME;
}

INT32 MoviePlayExe_PlayFwd(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	return NVTEVT_CONSUME;
}
INT32 MoviePlayExe_PlayBwd(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	return NVTEVT_CONSUME;
}
INT32 MoviePlayExe_SwitchLastFrame(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	return NVTEVT_CONSUME;
}
void MoviePlayExe_SetDataPlay(PLAYMOVIE_SET_DATATYPE dataType, UINT32 data)
{
	return;
}

INT32 MoviePlayExe_ChangeSize(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	return NVTEVT_CONSUME;
}

INT32 MoviePlayExe_OnAudPlayVolume(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UINT32 uhSelect = 0;
	BOOL  bChangHWSetting = FALSE;

	if (paramNum == 1) { //save index only
		uhSelect = paramArray[0];
	} else if (paramNum == 2 && paramArray[1] == 1) { //save index and change HW setting
		uhSelect = paramArray[0];
		bChangHWSetting = TRUE;
	} else {
		DBG_ERR("paramNum error!\r\n");
		return NVTEVT_CONSUME;
	}
	DBG_IND("+-idx=%d,val=%d\r\n", uhSelect, GetMovieAudioVolumeValue(uhSelect));
	//if (UI_GetData(FL_MovieAudioPlayIndex) != uhSelect)
	{
		UI_SetData(FL_MovieAudioPlayIndex, uhSelect);
		if (bChangHWSetting) {
			ImageApp_MoviePlay_SetVolume(GetMovieAudioVolumeValue(uhSelect));
		}
	}
	return NVTEVT_CONSUME;
}


////////////////////////////////////////////////////////////

EVENT_ENTRY CustomMoviePlayObjCmdMap[] = {
	{NVTEVT_EXE_OPENPLAY,                   MoviePlayExe_OpenPlay},
	{NVTEVT_EXE_CLOSEPLAY,                  MoviePlayExe_ClosePlay},
	{NVTEVT_EXE_PAUSEPLAY,                  MoviePlayExe_PausePlay},
	{NVTEVT_EXE_RESUMEPLAY,                 MoviePlayExe_ResumePlay},
	{NVTEVT_EXE_STARTPLAY,                  MoviePlayExe_StartPlay},
	{NVTEVT_EXE_FWDPLAY,                    MoviePlayExe_PlayFwd},
	{NVTEVT_EXE_BWDPLAY,                    MoviePlayExe_PlayBwd},
	{NVTEVT_EXE_FWDSTEPPLAY,                MoviePlayExe_PlayStepFwd},
	{NVTEVT_EXE_BWDSTEPPLAY,                MoviePlayExe_PlayStepBwd},
	{NVTEVT_EXE_SWITCH_FIRSTFRAME,          MoviePlayExe_SwitchFirstFrame},
	{NVTEVT_EXE_SWITCH_LASTFRAME,           MoviePlayExe_SwitchLastFrame},
	{NVTEVT_EXE_CHANGESIZE,                 MoviePlayExe_ChangeSize},
	{NVTEVT_EXE_MOVIEAUDPLAYVOLUME,         MoviePlayExe_OnAudPlayVolume},
	{NVTEVT_NULL,                           0},  //End of Command Map
};

CREATE_APP(CustomMoviePlayObj, APP_SETUP)


