#include "ImageApp_Common_int.h"

/// ========== Cross file variables ==========
BOOL g_ia_common_trace_on = TRUE;
ID IACOMMON_FLG_ID                    = 0;
ID IACOMMON_SEMID_QUE                 = 0;
ID IACOMMON_SEMID_LOCK_VIDEO          = 0;
ID IACOMMON_SEMID_PORT_STATUS         = 0;

UINT32 is_iacomm_init                 = FALSE;
UINT32 is_iacomm_rtsp_start           = FALSE;
UINT32 iacomm_streaming_mode          = IACOMMON_STREAMING_MODE_LIVE555;
UINT32 iacomm_keep_sem_flag           = FALSE;
UINT32 iacomm_reinited                = FALSE;
UINT32 iacomm_use_external_streaming  = FALSE;
#if (IACOMMON_BRANCH != IACOMMON_BR_1_25)
UINT32 iacomm_LviewNvt_portNum        = 8192;
UINT32 iacomm_LviewNvt_threadPriority = 6;
UINT32 iacomm_LviewNvt_frameRate      = 30;
UINT32 iacomm_LviewNvt_sockbufSize    = 102400;
UINT32 iacomm_LviewNvt_timeoutCnt     = 10;
UINT32 iacomm_LviewNvt_tos            = 0x0100;
#endif

/// ========== Noncross file variables ==========
static THREAD_HANDLE iacomm_rtsp_tsk_id;
static UINT32 is_iacomm_init_first_time = TRUE;

#define PRI_IACOMM_RTSP              7
#define STKSIZE_IACOMM_RTSP          4096

ER ImageApp_Common_Init(void)
{
	UINT32 i;
	ER ret = E_OK;
	T_CFLG cflg = {0};

	if (is_iacomm_init == TRUE) {
		DBG_WRN("ImageApp_Common is already init\r\n");
		return ret;
	}

	is_iacomm_init = TRUE;
	if (is_iacomm_init_first_time) {
		is_iacomm_init_first_time = FALSE;
	} else {
		iacomm_reinited = TRUE;
	}

	if(!IACOMMON_SEMID_QUE) {
		if ((ret = vos_sem_create(&IACOMMON_SEMID_QUE, 1, "SEMID_IACOMM_QUE")) != E_OK) {
			DBG_ERR("Create IACOMMON_SEMID_QUE fail(%d)\r\n", ret);
		}
	}
	if(!IACOMMON_SEMID_LOCK_VIDEO) {
		if ((ret = vos_sem_create(&IACOMMON_SEMID_LOCK_VIDEO, 1, "SEMID_IACOMM_LV")) != E_OK) {
			DBG_ERR("Create IACOMMON_SEMID_LOCK_VIDEO fail(%d)\r\n", ret);
		}
	}
	if(!IACOMMON_SEMID_PORT_STATUS) {
		if ((ret = vos_sem_create(&IACOMMON_SEMID_PORT_STATUS, 1, "SEMID_IACOMM_PORT")) != E_OK) {
			DBG_ERR("Create IACOMMON_SEMID_PORT_STATUS fail(%d)\r\n", ret);
		}
	}
	if(!IACOMMON_FLG_ID) {
		if ((ret = vos_flag_create(&IACOMMON_FLG_ID, &cflg, "IACOMMON_FLG")) != E_OK) {
			DBG_ERR("Create IACOMMON_FLG_ID fail(%d)\r\n", ret);
		} else {
			iacomm_flag_clr(IACOMMON_FLG_ID, FLGIACOMMON_RDY_FLAG);
	        iacomm_flag_set(IACOMMON_FLG_ID, FLGIACOMMON_FREE_FLAG);
			if (iacomm_use_external_streaming == FALSE) {
				for (i= 0; i < MAX_RTSP_PATH; i++) {
					live555_init(i);
					live555_reset_Aqueue(i);
					live555_reset_Vqueue(i);
				}
			}
		}
	}
	return ret;
}

ER ImageApp_Common_Uninit(void)
{
	ER ret = E_OK;

	if (is_iacomm_init == FALSE) {
		DBG_WRN("ImageApp_Common is already uninit\r\n");
		return ret;
	}

	if (iacomm_keep_sem_flag == FALSE) {
		if (IACOMMON_SEMID_QUE) {
			vos_sem_destroy(IACOMMON_SEMID_QUE);
			IACOMMON_SEMID_QUE = 0;
		}
		if (IACOMMON_SEMID_LOCK_VIDEO) {
			vos_sem_destroy(IACOMMON_SEMID_LOCK_VIDEO);
			IACOMMON_SEMID_LOCK_VIDEO = 0;
		}
		if (IACOMMON_SEMID_PORT_STATUS) {
			vos_sem_destroy(IACOMMON_SEMID_PORT_STATUS);
			IACOMMON_SEMID_PORT_STATUS = 0;
		}
		if (IACOMMON_FLG_ID) {
			if ((ret = vos_flag_destroy(IACOMMON_FLG_ID)) != E_OK) {
				DBG_ERR("Destroy IACOMMON_FLG_ID failed(%d)\r\n", ret);
			}
			IACOMMON_FLG_ID = 0;
		}
	} else {
		ImageApp_Common_DUMP("ImageApp_Common_Uninit but keep semaphores and flags\r\n");
	}

	// reset variables
	iacomm_use_external_streaming = FALSE;

	is_iacomm_init = FALSE;

	return ret;
}

THREAD_RETTYPE _ImageApp_Common_RtspTsk(void)
{
	NVTLIVE555_HDAL_CB hdal_cb;

	THREAD_ENTRY();

	hdal_cb.open_video = live555_on_open_video;
	hdal_cb.close_video = live555_on_close_video;
	hdal_cb.get_video_info = live555_on_get_video_info;
	hdal_cb.lock_video = live555_on_lock_video;
	hdal_cb.unlock_video = live555_on_unlock_video;
	hdal_cb.open_audio = live555_on_open_audio;
	hdal_cb.close_audio = live555_on_close_audio;
	hdal_cb.get_audio_info = live555_on_get_audio_info;
	hdal_cb.lock_audio = live555_on_lock_audio;
	hdal_cb.unlock_audio = live555_on_unlock_audio;
	nvtrtspd_init(&hdal_cb);
	nvtrtspd_open();

	THREAD_RETURN(0);
}

ER ImageApp_Common_RtspStart(UINT32 id)
{
	ER ret = E_OK;

	if (is_iacomm_rtsp_start == TRUE) {
		DBG_ERR("stsp is already started.\r\n");
		return E_SYS;
	}

	if (iacomm_use_external_streaming == TRUE) {
		DBG_ERR("IACOMMON_PARAM_MISC_USE_EXTERNAL_STREAMING is set\r\n");
		return E_SYS;
	}

	if (iacomm_streaming_mode == IACOMMON_STREAMING_MODE_LIVE555) {       // H.264 or H.265, use live555
		if ((iacomm_rtsp_tsk_id = vos_task_create(_ImageApp_Common_RtspTsk, 0, "IACommRtspTsk", PRI_IACOMM_RTSP, STKSIZE_IACOMM_RTSP)) == 0) {
			DBG_ERR("IACommRtspTsk create failed.\r\n");
			ret = E_SYS;
		} else {
			vos_task_resume(iacomm_rtsp_tsk_id);
		}
	} else {                        // MJPG, use LviewNvt
#if (IACOMMON_BRANCH != IACOMMON_BR_1_25)
		LVIEWNVT_DAEMON_INFO   lviewObj = {0};

		lviewObj.portNum = iacomm_LviewNvt_portNum;
		// set http live view server thread priority
		lviewObj.threadPriority = iacomm_LviewNvt_threadPriority;
		// set the maximum JPG size that http live view server to support
		//lviewObj.maxJpgSize = maxJpgSize;
		// live view streaming frame rate
		lviewObj.frameRate = iacomm_LviewNvt_frameRate;
		// socket buffer size
		lviewObj.sockbufSize = iacomm_LviewNvt_sockbufSize;
		//	// set time out 10*0.5 = 5 sec
		lviewObj.timeoutCnt  = iacomm_LviewNvt_timeoutCnt;
		// Set type of service as Maximize Throughput
		lviewObj.tos = iacomm_LviewNvt_tos;
		//lviewObj.bitstream_mem_range.addr = 0;
		//lviewObj.bitstream_mem_range.size = 0;
		// reserved parameter , no use now, just fill NULL.
		lviewObj.arg = NULL;
		// if want to use https
		lviewObj.is_ssl = 1;
		// if use push mode
		lviewObj.is_push_mode = 1;

		// start http daemon
		lviewnvt_start_daemon(&lviewObj);
#endif
	}
	is_iacomm_rtsp_start = TRUE;
	return ret;
}

ER ImageApp_Common_RtspStop(UINT32 id)
{
	if (is_iacomm_rtsp_start == FALSE) {
		DBG_ERR("stsp is already stoped.\r\n");
		return E_SYS;
	}

	if (iacomm_use_external_streaming == TRUE) {
		DBG_ERR("IACOMMON_PARAM_MISC_USE_EXTERNAL_STREAMING is set\r\n");
		return E_SYS;
	}

	if (iacomm_streaming_mode == IACOMMON_STREAMING_MODE_LIVE555) {       // H.264 or H.265, use live555
		nvtrtspd_close();
	} else {                        // MJPG, use LviewNvt
#if (IACOMMON_BRANCH != IACOMMON_BR_1_25)
		lviewnvt_stop_daemon();
#endif
	}
	is_iacomm_rtsp_start = FALSE;
	return E_OK;
}

UINT32 ImageApp_Common_IsRtspStart(UINT32 id)
{
	return is_iacomm_rtsp_start;
}

void QUEUE_EnQueueToTail(QUEUE_BUFFER_QUEUE *queue, QUEUE_BUFFER_ELEMENT *element)
{
	_QUEUE_EnQueueToTail(queue, element);
}

QUEUE_BUFFER_ELEMENT* QUEUE_DeQueueFromHead(QUEUE_BUFFER_QUEUE *queue)
{
	return _QUEUE_DeQueueFromHead(queue);
}

UINT32 QUEUE_GetQueueSize(QUEUE_BUFFER_QUEUE *queue)
{
	return _QUEUE_GetQueueSize(queue);
}

void _iacommp_set_streaming_mode(UINT32 mode)
{
#if (IACOMMON_BRANCH != IACOMMON_BR_1_25)
	iacomm_streaming_mode = mode;
#endif
}

ER iacomm_flag_clr(ID flgid, FLGPTN clrptn)
{
	ER ret = E_OK;

	if (is_iacomm_init == FALSE) {
		DBG_ERR("ImageApp_Common is not initialized\r\n");
	} else {
		ret = vos_flag_clr(flgid, clrptn);
	}
	return ret;
}

ER iacomm_flag_set(ID flgid, FLGPTN setptn)
{
	ER ret = E_OK;

	if (is_iacomm_init == FALSE) {
		DBG_ERR("ImageApp_Common is not initialized\r\n");
	} else {
		ret = vos_flag_set(flgid, setptn);
	}
	return ret;
}

ER iacomm_flag_wait(PFLGPTN p_flgptn, ID flgid, FLGPTN waiptn, UINT wfmode)
{
	ER ret = E_OK;

	if (is_iacomm_init == FALSE) {
		DBG_ERR("ImageApp_Common is not initialized\r\n");
	} else {
		ret = vos_flag_wait(p_flgptn, flgid, waiptn, wfmode);
	}
	return ret;
}

ER iacomm_flag_wait_timeout(PFLGPTN p_flgptn, ID flgid, FLGPTN waiptn, UINT wfmode, int timeout_tick)
{
	ER ret = E_OK;

	if (is_iacomm_init == FALSE) {
		DBG_ERR("ImageApp_Common is not initialized\r\n");
	} else {
		ret = vos_flag_wait_timeout(p_flgptn, flgid, waiptn, wfmode, timeout_tick);
	}
	return ret;
}

FLGPTN iacomm_flag_chk(ID flgid, FLGPTN chkptn)
{
	FLGPTN ret = 0;

	if (is_iacomm_init == FALSE) {
		DBG_ERR("ImageApp_Common is not initialized\r\n");
	} else {
		ret = vos_flag_chk(flgid, chkptn);
	}
	return ret;
}

