#include "ImageApp_Common_int.h"

/// ========== Cross file variables ==========

/// ========== Noncross file variables ==========

#if (IACOMMON_BRANCH != IACOMMON_BR_1_25)
static int _getJpg(int *jpgAddr, int *jpgSize)
{
	*jpgAddr = 0;
	*jpgSize = 0;
	return TRUE;
}

static void _notifyStatus(int status)
{
	switch (status) {
	// HTTP client has request coming in
	case CYG_LVIEW_STATUS_CLIENT_REQUEST:
		ImageApp_Common_DUMP("client request %lld ms\r\n", hd_gettime_us() / 1000);
		break;
	// HTTP server send JPG data start
	case CYG_LVIEW_STATUS_SERVER_RESPONSE_START:
		//ImageApp_Common_DUMP("response start, time= %05d ms\r\n", hd_gettime_us()/1000);
		break;
	// HTTP server send JPG data end
	case CYG_LVIEW_STATUS_SERVER_RESPONSE_END:
		//ImageApp_Common_DUMP("se time= %05d ms\r\n", hd_gettime_us() / 1000);
		break;
	case CYG_LVIEW_STATUS_CLIENT_DISCONNECT:
		ImageApp_Common_DUMP("client disconnect %lld ms\r\n", hd_gettime_us() / 1000);
		break;
	}
}
#endif

ER lviewnvt_start_daemon(LVIEWNVT_DAEMON_INFO *p_open)
{
#if (IACOMMON_BRANCH != IACOMMON_BR_1_25)
	if (p_open == NULL) {
		DBG_ERR("p_open==NULL\r\n");
		return E_SYS;
	}

	p_open->getJpg = (LVIEWD_GETJPGCB)_getJpg;
	p_open->serverEvent = (LVIEWD_SERVER_EVENT_CB)_notifyStatus;

	return LviewNvt_StartDaemon(p_open);
#else
	return E_NOSPT;
#endif
}

ER lviewnvt_stop_daemon(void)
{
#if (IACOMMON_BRANCH != IACOMMON_BR_1_25)
	return LviewNvt_StopDaemon();
#else
	return E_NOSPT;
#endif
}

ER lviewnvt_push_frame(LVIEWNVT_FRAME_INFO *frame_info)
{
#if (IACOMMON_BRANCH != IACOMMON_BR_1_25)
	return LviewNvt_PushFrame(frame_info);
#else
	return E_NOSPT;
#endif
}

