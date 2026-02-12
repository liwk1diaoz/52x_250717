#include "PrjInc.h"

#if HEIC_FUNC == ENABLE

#include "PrjCfg.h"
#include "UIAppMovie_SampleCode_HEIC.h"
#include "heifnvt/nvt_wrapper.h"
#if USE_DCF
	#include "DCF.h"
#endif

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UiAppMovie
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
#include "ImageApp/ImageApp_Common.h"
#include "NamingRule/NameRule_Custom.h"
#include <kwrap/flag.h>

#define HEIC_VENC_PORT                  5
#define HEIC_VENC_WIDTH              1920
#define HEIC_VENC_HEIGHT             1080
#define HEIC_VENC_QUALITY              35

#define PRI_MOVIE_SAMPLE_HEIC          20
#define STKSIZE_MOVIESAMPLE_HEIC     4096

#define FLG_HEIC_TSK_ESC       0x20000000
#define FLG_HEIC_RAWENC        0x80000000

static THREAD_HANDLE movie_sample_heic_tsk_id;
static UINT32 movie_sample_heic_tsk_run, is_movie_sample_heic_tsk_running;

static HD_PATH_ID heic_venc_p = 0;
static ID heic_flag_id = 0;

typedef struct {
	QUEUE_BUFFER_ELEMENT q;             // should be the first element of this structure
	HD_PATH_ID           path;
} HEIC_JOB_ELEMENT;

static QUEUE_BUFFER_QUEUE heic_job_queue_clear = {0};
static QUEUE_BUFFER_QUEUE heic_job_queue_ready = {0};
static HEIC_JOB_ELEMENT   heic_job_element[4];

extern void MovieExe_UserEventCb(UINT32 id, MOVIE_USER_CB_EVENT event_id, UINT32 value);

static HD_RESULT _MovieSample_HEIC_Open_VEnc_Path(void)
{
	HD_RESULT hd_ret;
	HD_IN_ID  heic_venc_i = HD_VIDEOENC_IN (0, HEIC_VENC_PORT);
	HD_OUT_ID heic_venc_o = HD_VIDEOENC_OUT(0, HEIC_VENC_PORT);

	// open venc path
	{
		if ((hd_ret = hd_videoenc_open(heic_venc_i, heic_venc_o, &heic_venc_p)) != HD_OK) {
			DBG_ERR("hd_videoenc_open fail(%d)\n", hd_ret);
		}
	}

	// config dram id
	{
		HD_VIDEOENC_FUNC_CONFIG heic_venc_func_cfg = {0};
		heic_venc_func_cfg.ddr_id  = DDR_ID0;
		heic_venc_func_cfg.in_func = 0;
		if ((hd_ret = hd_videoenc_set(heic_venc_p, HD_VIDEOENC_PARAM_FUNC_CONFIG, &heic_venc_func_cfg)) != HD_OK) {
			DBG_ERR("set HD_VIDEOENC_PARAM_FUNC_CONFIG fail(%d)\r\n", hd_ret);
		}
	}

	// set venc cfg
	{
		HD_VIDEOENC_PATH_CONFIG heic_venc_path_cfg = {0};
		heic_venc_path_cfg.max_mem.codec_type          = HD_CODEC_TYPE_H265;
		heic_venc_path_cfg.max_mem.max_dim.w           = HEIC_VENC_WIDTH;
		heic_venc_path_cfg.max_mem.max_dim.h           = HEIC_VENC_HEIGHT;
		heic_venc_path_cfg.max_mem.bitrate             = 1 * 1024 * 1024 * 8;
		heic_venc_path_cfg.max_mem.enc_buf_ms          = 1500;
		heic_venc_path_cfg.max_mem.svc_layer           = HD_SVC_2X;
		heic_venc_path_cfg.max_mem.ltr                 = FALSE;
		heic_venc_path_cfg.max_mem.rotate              = FALSE;
		heic_venc_path_cfg.max_mem.source_output       = FALSE;
		heic_venc_path_cfg.isp_id                      = 0;
		if ((hd_ret = hd_videoenc_set(heic_venc_p, HD_VIDEOENC_PARAM_PATH_CONFIG, &heic_venc_path_cfg)) != HD_OK) {
			DBG_ERR("set HD_VIDEOENC_PARAM_PATH_CONFIG fail(%d)\r\n", hd_ret);
		}
	}

	// set venc in param
	{
		HD_VIDEOENC_IN  heic_venc_in_param = {0};
		heic_venc_in_param.dim.w                       = HEIC_VENC_WIDTH;
		heic_venc_in_param.dim.h                       = HEIC_VENC_HEIGHT;
		heic_venc_in_param.pxl_fmt                     = HD_VIDEO_PXLFMT_YUV420;
		heic_venc_in_param.dir                         = HD_VIDEO_DIR_NONE;
		heic_venc_in_param.frc                         = HD_VIDEO_FRC_RATIO(1, 1);
		if ((hd_ret = hd_videoenc_set(heic_venc_p, HD_VIDEOENC_PARAM_IN, &heic_venc_in_param)) != HD_OK) {
			DBG_ERR("set HD_VIDEOENC_PARAM_IN fail(%d)\r\n", hd_ret);
		}
	}

	// set venc out param
	{
		HD_VIDEOENC_OUT2 heic_venc_out_param = {0};
		heic_venc_out_param.codec_type                 = HD_CODEC_TYPE_H265;
		heic_venc_out_param.h26x.gop_num               = 30;
		heic_venc_out_param.h26x.source_output         = DISABLE;
		heic_venc_out_param.h26x.profile               = HD_H265E_MAIN_PROFILE;
		heic_venc_out_param.h26x.level_idc             = HD_H265E_LEVEL_5;
		heic_venc_out_param.h26x.svc_layer             = HD_SVC_2X;
		heic_venc_out_param.h26x.entropy_mode          = HD_H265E_CABAC_CODING;
		if ((hd_ret = hd_videoenc_set(heic_venc_p, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &heic_venc_out_param)) != HD_OK) {
			DBG_ERR("set HD_VIDEOENC_PARAM_OUT_ENC_PARAM2 fail(%d)\r\n", hd_ret);
		}
	}

	// set venc rc param
	{
		HD_H26XENC_RATE_CONTROL2 heic_venc_rc_param = {0};
		heic_venc_rc_param.rc_mode                     = HD_RC_MODE_CBR;
		heic_venc_rc_param.cbr.bitrate                 = 1 * 1024 * 1024 * 8;
		heic_venc_rc_param.cbr.frame_rate_base         = 30;
		heic_venc_rc_param.cbr.frame_rate_incr         = 1;
		heic_venc_rc_param.cbr.init_i_qp               = HEIC_VENC_QUALITY;
		heic_venc_rc_param.cbr.max_i_qp                = 45;
		heic_venc_rc_param.cbr.min_i_qp                = 10;
		heic_venc_rc_param.cbr.init_p_qp               = HEIC_VENC_QUALITY;
		heic_venc_rc_param.cbr.max_p_qp                = 45;
		heic_venc_rc_param.cbr.min_p_qp                = 10;
		heic_venc_rc_param.cbr.static_time             = 4;
		heic_venc_rc_param.cbr.ip_weight               = 0;
		heic_venc_rc_param.cbr.key_p_period            = 0;
		heic_venc_rc_param.cbr.kp_weight               = 0;
		heic_venc_rc_param.cbr.p2_weight               = 0;
		heic_venc_rc_param.cbr.p3_weight               = 0;
		heic_venc_rc_param.cbr.lt_weight               = 0;
		heic_venc_rc_param.cbr.motion_aq_str           = 0;
		heic_venc_rc_param.cbr.max_frame_size          = 0;
		if ((hd_ret = hd_videoenc_set(heic_venc_p, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2, &heic_venc_rc_param)) != HD_OK) {
			DBG_ERR("set HD_VIDEOENC_PARAM_OUT_RATE_CONTROL2 fail(%d)\r\n", hd_ret);
		}
	}

	// set venc long start code
	{
		VENDOR_VIDEOENC_LONG_START_CODE lsc = {0};
		lsc.long_start_code_en = ENABLE;
		if ((hd_ret = vendor_videoenc_set(heic_venc_p, VENDOR_VIDEOENC_PARAM_OUT_LONG_START_CODE, &lsc)) != HD_OK) {
			DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_LONG_START_CODE) fail(%d)\n", hd_ret);
		}
	}

	// set venc base quality mode
	{
		VENDOR_VIDEOENC_QUALITY_BASE_MODE qbm = {0};
		qbm.quality_base_en = TRUE;
		if ((hd_ret = vendor_videoenc_set(heic_venc_p, VENDOR_VIDEOENC_PARAM_OUT_QUALITY_BASE, &qbm)) != HD_OK) {
			DBG_ERR("vendor_videoenc_set(VENDOR_VIDEOENC_PARAM_OUT_QUALITY_BASE) fail(%d)\n", hd_ret);
		}
	}

	{
		HD_H26XENC_VUI vui_param = {0};

		hd_ret = hd_videoenc_get(heic_venc_p, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param);
		if (hd_ret != HD_OK) {
			DBG_ERR("hd_videoenc_get(HD_VIDEOENC_PARAM_OUT_VUI) fail(%d)\n", hd_ret);
			return E_SYS;
		}

		vui_param.vui_en = TRUE;
		vui_param.color_range = 1; /* full range, makes color transform like jpg */

		if ((hd_ret = hd_videoenc_set(heic_venc_p, HD_VIDEOENC_PARAM_OUT_VUI, &vui_param)) != HD_OK) {
			DBG_ERR("hd_videoenc_set(HD_VIDEOENC_PARAM_OUT_VUI) fail(%d)\n", hd_ret);
		}
	}

	return hd_ret;
}

static HD_RESULT _MovieSample_HEIC_Close_VEnc_Path(void)
{
	HD_RESULT hd_ret = HD_ERR_NG;

	// close venc path
	{
		if ((hd_ret = hd_videoenc_close(heic_venc_p)) != HD_OK) {
			DBG_ERR("hd_videoenc_close fail(%d)\n", hd_ret);
		}
		heic_venc_p = 0;
	}
	return hd_ret;
}

static THREAD_RETTYPE MovieSample_HEIC_Tsk(void)
{
	HD_PATH_ID yuv_path;
	HD_VIDEO_FRAME yuv_frame = {0};
	HD_VIDEOENC_BS h265_bs = {0};
	//HD_VIDEOENC_BUFINFO venc_bs_buf = {0};
	//UINT32 venc_bs_buf_va = 0;
	HD_RESULT hd_ret;
	FLGPTN uiFlag;
	UINT32 i;
	char fullpath[80];
	HEIC_JOB_ELEMENT *pJob = NULL;
	#if (MOVIE_SAMPLE_HEIC_WITH_JPG == FALSE)
	UINT32 captured = FALSE;
	#endif

	THREAD_ENTRY();

	is_movie_sample_heic_tsk_running = TRUE;

	// init job queue
	memset(&heic_job_queue_clear, 0, sizeof(heic_job_queue_clear));
	memset(&heic_job_queue_ready, 0, sizeof(heic_job_queue_ready));
	for (i = 0; i < 4; i++) {
		QUEUE_EnQueueToTail(&heic_job_queue_clear, (QUEUE_BUFFER_ELEMENT *)&heic_job_element[i]);
	}

	while (movie_sample_heic_tsk_run) {
		vos_flag_wait(&uiFlag, heic_flag_id, (FLG_HEIC_RAWENC | FLG_HEIC_TSK_ESC), TWF_ORW | TWF_CLR);

		// break and terminate task if receive ESC flag
		if (uiFlag & FLG_HEIC_TSK_ESC) {
			vos_flag_clr(heic_flag_id, (FLG_HEIC_RAWENC | FLG_HEIC_TSK_ESC));
			break;
		}

		while ((pJob = (HEIC_JOB_ELEMENT *)QUEUE_DeQueueFromHead(&heic_job_queue_ready)) != NULL) {

			// init status
			#if (MOVIE_SAMPLE_HEIC_WITH_JPG == FALSE)
			captured = FALSE;
			#endif

			// get YUV src path
			yuv_path = ImageApp_MovieMulti_GetVprcPort(pJob->path, IAMOVIE_VPRC_EX_MAIN);

			// pull out YUV frame
			if ((hd_ret = hd_videoproc_pull_out_buf(yuv_path, &yuv_frame, 500)) != HD_OK) {
				DBG_ERR("hd_videoproc_pull_out_buf fail(%d)\r\n", hd_ret);
				goto SKIP_THIS_JOB;
			}

			// start venc path
			if (MovieSample_HEIC_VEnc_Start() != HD_OK) {
				DBG_ERR("MovieSample_HEIC_VEnc_Start fail(%d)\r\n", hd_ret);
				goto RELEASE_YUV_BLOCK;
			}

			// push YUV frame to venc
			if ((hd_ret = hd_videoenc_push_in_buf(heic_venc_p, &yuv_frame, NULL, 0)) != HD_OK) {
				DBG_ERR("hd_videoenc_push_in_buf fail(%d)\r\n", hd_ret);
				goto STOP_VENC;
			}

			// pull out H265 bitstream
			if ((hd_ret = hd_videoenc_pull_out_buf(heic_venc_p, &h265_bs, -1)) != HD_OK) {
				DBG_ERR("hd_videoenc_pull_out_buf fail(%d)\r\n", hd_ret);
				goto STOP_VENC;
			}

#if USE_FILEDB
			UINT32 port = ImageApp_MovieMulti_Recid2BsPort(pJob->path);
			MovieExe_UserEventCb(pJob->path, MOVIE_USER_CB_EVENT_FILENAMING_SNAPSHOT_CB, (UINT32)filename);
			NameRule_getCustom_byid(port)->SetInfo(MEDIANAMING_SETINFO_SETFILETYPE, MEDIA_FILEFORMAT_JPG, 0, 0);
			NH_Custom_BuildFullPath_GiveFileName(port, NMC_SECONDTYPE_PHOTO, filename, fullpath);
#elif USE_DCF
			//MovieExe_UserEventCb(pJob->path, MOVIE_USER_CB_EVENT_FILENAMING_SNAPSHOT_CB, (UINT32)fullpath);
			UINT32 nextFolderID = 0, nextFileID = 0;

		    if (DCF_GetDBInfo(DCF_INFO_IS_9999)) {
				DBG_ERR("Exceed max dcf file!\r\n");
				fullpath[0] = '\0';
			} else {
				DCF_GetNextID(&nextFolderID,&nextFileID);
				DCF_MakeObjPath(nextFolderID, nextFileID,  DCF_FILE_TYPE_ASF, fullpath);
				DCF_AddDBfile(fullpath);
				DBG_DUMP("%s added to DCF\r\n", fullpath);
			}
#else
			DBG_ERR("file name not given\n");
			goto STOP_VENC;
#endif

			// convert file path for linux
#if defined(__FREERTOS)	
			/* do nothing */
#else
			// get filename
			char filename[80];
			char *pch;
			UINT32 i, j;
			
			strncpy(filename, fullpath, sizeof(filename)-1);
			strncpy(fullpath, HEIC_STORAGE_ROOT, sizeof(filename)-1);
			i = 3;
			j = strlen(fullpath);
			while(filename[i]) {
				fullpath[j] = (filename[i] == '\\') ? '/' : filename[i];
				i++;
				j++;
			}
			pch = strstr(fullpath, ".JPG");
			if (pch != NULL) {
				strncpy(pch, HEIC_TMP_EXT_NAME, strlen(HEIC_TMP_EXT_NAME));
			}
#endif

			// save file
			if (heif_create_heic(&h265_bs, fullpath) == TRUE) {
				
#if defined(__FREERTOS)	
				/* do nothing */
#else			
				// force sync
				system("sync");
#endif
				
				#if (MOVIE_SAMPLE_HEIC_WITH_JPG == FALSE)
				captured = TRUE;
				#endif
				DBG_DUMP("Create %s\n", fullpath);
			}

			// release H265 bitstream
			if ((hd_ret = hd_videoenc_release_out_buf(heic_venc_p, &h265_bs)) != HD_OK) {
				DBG_ERR("hd_videoenc_release_out_buf fail(%d)\r\n", hd_ret);
			}

STOP_VENC:
			// stop venc path
			MovieSample_HEIC_VEnc_Stop();

RELEASE_YUV_BLOCK:
			// release YUV frame
			if ((hd_ret = hd_videoproc_release_out_buf(yuv_path, &yuv_frame)) != HD_OK) {
				DBG_ERR("hd_videoproc_release_out_buf fail(%d)\r\n", hd_ret);
			}

SKIP_THIS_JOB:
			// callback status
			#if (MOVIE_SAMPLE_HEIC_WITH_JPG == FALSE)
			if (captured == TRUE) {
				MovieExe_UserEventCb(pJob->path, MOVIE_USER_CB_EVENT_SNAPSHOT_OK, pJob->path);
				MovieExe_UserEventCb(pJob->path, MOVIE_USER_CB_EVENT_CLOSE_FILE_COMPLETED, (UINT32)"jpg");  // since file extension is heic, but callback "jpg" for project check
			} else {
				MovieExe_UserEventCb(pJob->path, MOVIE_USER_CB_ERROR_SNAPSHOT_ERR, pJob->path);
			}
			#endif

			// release job queue
			QUEUE_EnQueueToTail(&heic_job_queue_clear, (QUEUE_BUFFER_ELEMENT *)pJob);
		}
	}
	is_movie_sample_heic_tsk_running = FALSE;

	THREAD_RETURN(0);
}

ER MovieSample_HEIC_InstallID(void)
{
	ER ret = E_OK;
	T_CFLG cflg;

	// create flag
	{
		memset(&cflg, 0, sizeof(T_CFLG));
		if (vos_flag_create(&heic_flag_id, &cflg, "HEIC_FLG_ID") != E_OK) {
			DBG_ERR("HEIC_FLG_ID fail\r\n");
			ret = E_SYS;
		}
	}

	// open venc path
	{
		_MovieSample_HEIC_Open_VEnc_Path();
	}

	// create task
	{
		movie_sample_heic_tsk_run = FALSE;
		is_movie_sample_heic_tsk_running = FALSE;

		if ((movie_sample_heic_tsk_id = vos_task_create(MovieSample_HEIC_Tsk, 0, "MovieSampleHEICTsk", PRI_MOVIE_SAMPLE_HEIC, STKSIZE_MOVIESAMPLE_HEIC)) == 0) {
			DBG_ERR("MovieSampleHEICTsk create failed.\r\n");
        	ret = E_SYS;
		} else {
			movie_sample_heic_tsk_run = TRUE;
	    	vos_task_resume(movie_sample_heic_tsk_id);
		}
	}
	return ret;
}

ER MovieSample_HEIC_UninstallID(void)
{
	ER ret = E_OK;
	UINT32 delay_cnt;

	// destroy task
	{
		delay_cnt = 50;
		movie_sample_heic_tsk_run = FALSE;
		vos_flag_set(heic_flag_id, FLG_HEIC_TSK_ESC);
		while (is_movie_sample_heic_tsk_running && delay_cnt) {
			vos_util_delay_ms(10);
			delay_cnt --;
		}

		if (is_movie_sample_heic_tsk_running) {
			DBG_DUMP("Destroy MovieSampleHEICTsk\r\n");
			vos_task_destroy(movie_sample_heic_tsk_id);
		}
	}

	// close venc path
	{
		_MovieSample_HEIC_Close_VEnc_Path();
	}

	// destroy flag
	{
		if (vos_flag_destroy(heic_flag_id) != E_OK) {
			DBG_ERR("HEIC_FLG_ID destroy failed.\r\n");
			ret = E_SYS;
		}
		heic_flag_id = 0;
	}
	return ret;
}

HD_RESULT MovieSample_HEIC_VEnc_Start(void)
{
	HD_RESULT hd_ret = HD_ERR_NG;

	if ((hd_ret = hd_videoenc_start(heic_venc_p)) != HD_OK) {
		DBG_ERR("hd_videoenc_start fail(%d)\r\n", hd_ret);
	}
	return hd_ret;
}

HD_RESULT MovieSample_HEIC_VEnc_Stop(void)
{
	HD_RESULT hd_ret = HD_ERR_NG;

	if ((hd_ret = hd_videoenc_stop(heic_venc_p)) != HD_OK) {
		DBG_ERR("hd_videoenc_stop fail(%d)\r\n", hd_ret);
	}
	return hd_ret;
}

ER MovieSample_HEIC_Trigger(MOVIE_CFG_REC_ID id)
{
	ER ret = E_SYS;;
	HEIC_JOB_ELEMENT *pJob = NULL;

	if ((pJob = (HEIC_JOB_ELEMENT *)QUEUE_DeQueueFromHead(&heic_job_queue_clear)) != NULL) {
		pJob->path = id;
		QUEUE_EnQueueToTail(&heic_job_queue_ready, (QUEUE_BUFFER_ELEMENT *)pJob);
		ret = E_OK;
	}
	vos_flag_set(heic_flag_id, FLG_HEIC_RAWENC);

	return ret;
}

#endif
