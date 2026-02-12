#include "ImageApp_Common_int.h"


/// ========== Cross file variables ==========
UINT32 live555_support_rtcp = TRUE;
#if (RTSP_FIXED_STREAM0 == DISABLE)
UINT32 live555_rtsp_fixed_stream0 = FALSE;
#else
UINT32 live555_rtsp_fixed_stream0 = TRUE;
#endif

/// ========== Noncross file variables ==========


/// ========== Set parameter area ==========
static ER _ImageApp_Common_SetParam_RtspGroup(UINT32 id, UINT32 param, UINT32 value)
{
	ER ret = E_SYS;

	switch (param) {
	case IACOMMON_PARAM_SUPPORT_RTCP: {
			if (is_iacomm_rtsp_start == TRUE) {
				DBG_ERR("Cannot set IACOMMON_PARAM_SUPPORT_RTCP since rtsp is started\r\n");
			} else {
				live555_support_rtcp = value ? TRUE : FALSE;
				ImageApp_Common_DUMP("Set live555 rtcp = %d\r\n", live555_support_rtcp);
				ret = E_OK;
			}
			break;
		}

	case IACOMMON_PARAM_RTSP_FIXED_STREAM0: {
			if (is_iacomm_rtsp_start == TRUE) {
				DBG_ERR("Cannot set IACOMMON_PARAM_RTSP_FIXED_STREAM0 since rtsp is started\r\n");
			} else {
				live555_rtsp_fixed_stream0 = value ? TRUE : FALSE;
				ImageApp_Common_DUMP("Set live555 fixed_stream0 = %d\r\n", live555_rtsp_fixed_stream0);
				ret = E_OK;
			}
			break;
		}
	}
	return ret;
}

static ER _ImageApp_Common_SetParam_LviewNvtGroup(UINT32 id, UINT32 param, UINT32 value)
{
	ER ret = E_NOSPT;

#if (IACOMMON_BRANCH != IACOMMON_BR_1_25)
	switch (param) {
	case IACOMMON_PARAM_LVIEWNVT_PORTNUM: {
			if (is_iacomm_rtsp_start == TRUE) {
				DBG_ERR("Cannot set IACOMMON_PARAM_LVIEWNVT_PORTNUM since rtsp is started\r\n");
			} else {
				iacomm_LviewNvt_portNum = value;
				ImageApp_Common_DUMP("Set LviewNvt portNum = %d\r\n", iacomm_LviewNvt_portNum);
				ret = E_OK;
			}
			break;
		}

	case IACOMMON_PARAM_LVIEWNVT_THREADPRIORITY: {
			if (is_iacomm_rtsp_start == TRUE) {
				DBG_ERR("Cannot set IACOMMON_PARAM_LVIEWNVT_THREADPRIORITY since rtsp is started\r\n");
			} else {
				iacomm_LviewNvt_threadPriority = value;
				ImageApp_Common_DUMP("Set LviewNvt threadPriority = %d\r\n", iacomm_LviewNvt_threadPriority);
				ret = E_OK;
			}
			break;
		}

	case IACOMMON_PARAM_LVIEWNVT_FRAMERATE: {
			if (is_iacomm_rtsp_start == TRUE) {
				DBG_ERR("Cannot set IACOMMON_PARAM_LVIEWNVT_FRAMERATE since rtsp is started\r\n");
			} else {
				iacomm_LviewNvt_frameRate = value;
				ImageApp_Common_DUMP("Set LviewNvt frameRate = %d\r\n", iacomm_LviewNvt_frameRate);
				ret = E_OK;
			}
			break;
		}

	case IACOMMON_PARAM_LVIEWNVT_SOCKBUFSIZE: {
			if (is_iacomm_rtsp_start == TRUE) {
				DBG_ERR("Cannot set IACOMMON_PARAM_LVIEWNVT_SOCKBUFSIZE since rtsp is started\r\n");
			} else {
				iacomm_LviewNvt_sockbufSize = value;
				ImageApp_Common_DUMP("Set LviewNvt sockbufSize = %d\r\n", iacomm_LviewNvt_sockbufSize);
				ret = E_OK;
			}
			break;
		}

	case IACOMMON_PARAM_LVIEWNVT_TIMEOUTCNT: {
			if (is_iacomm_rtsp_start == TRUE) {
				DBG_ERR("Cannot set IACOMMON_PARAM_LVIEWNVT_TIMEOUTCNT since rtsp is started\r\n");
			} else {
				iacomm_LviewNvt_timeoutCnt = value;
				ImageApp_Common_DUMP("Set LviewNvt timeoutCnt = %d\r\n", iacomm_LviewNvt_timeoutCnt);
				ret = E_OK;
			}
			break;
		}

	case IACOMMON_PARAM_LVIEWNVT_TOS: {
			if (is_iacomm_rtsp_start == TRUE) {
				DBG_ERR("Cannot set IACOMMON_PARAM_LVIEWNVT_TOS since rtsp is started\r\n");
			} else {
				iacomm_LviewNvt_tos = value;
				ImageApp_Common_DUMP("Set LviewNvt tos = %d\r\n", iacomm_LviewNvt_tos);
				ret = E_OK;
			}
			break;
		}
	}
#endif
	return ret;
}

static ER _ImageApp_Common_SetParam_MiscGroup(UINT32 id, UINT32 param, UINT32 value)
{
	ER ret = E_SYS;

	switch (param) {
	case IACOMMON_PARAM_MISC_DEBUG_MSG_EN: {
			g_ia_common_trace_on = value ? TRUE : FALSE;
			DBG_DUMP("Set IACOMMON_PARAM_MISC_DEBUG_MSG_EN = %s\r\n", g_ia_common_trace_on ? "ON" : "OFF");
			ret = E_OK;
			break;
		}

	case IACOMMON_PARAM_MISC_KEEP_SEM_FLAG: {
			iacomm_keep_sem_flag = value ? TRUE : FALSE;
			DBG_DUMP("Set IACOMMON_PARAM_MISC_KEEP_SEM_FLAG = %s\r\n", iacomm_keep_sem_flag ? "TRUE" : "FALSE");
			ret = E_OK;
			break;
		}

	case IACOMMON_PARAM_MISC_USE_EXTERNAL_STREAMING: {
			if (is_iacomm_init == FALSE) {
				iacomm_use_external_streaming = value ? TRUE : FALSE;
				DBG_DUMP("Set IACOMMON_PARAM_MISC_USE_EXTERNAL_STREAMING = %d\r\n", iacomm_use_external_streaming);
				ret = E_OK;
			} else {
				DBG_ERR("Cannot set IACOMMON_PARAM_MISC_USE_EXTERNAL_STREAMING after ImageApp_Common_Init()\r\n");
			}
			break;
		}
	}
	return ret;
}

ER ImageApp_Common_SetParam(UINT32 id, UINT32 param, UINT32 value)
{
	ER ret;

	if (param >= IACOMMON_PARAM_RTSP_GROUP_BEGIN && param < IACOMMON_PARAM_RTSP_GROUP_END) {
		ret = _ImageApp_Common_SetParam_RtspGroup(id, param, value);
	} else if (param >= IACOMMON_PARAM_LVIEWNVT_GROUP_BEGIN && param < IACOMMON_PARAM_LVIEWNVT_GROUP_END) {
		ret = _ImageApp_Common_SetParam_LviewNvtGroup(id, param, value);
	} else if (param >= IACOMMON_PARAM_MISC_GROUP_BEGIN && param < IACOMMON_PARAM_MISC_GROUP_END) {
		ret = _ImageApp_Common_SetParam_MiscGroup(id, param, value);
	} else {
		DBG_WRN("Paramter id %x not matched.\r\n", param);
		ret = E_SYS;
	}
	return ret;
}

/// ========== Get parameter area ==========
static ER _ImageApp_Common_GetParam_RtspGroup(UINT32 id, UINT32 param, UINT32* value)
{
	ER ret = E_NOSPT;

	switch (param) {
	case IACOMMON_PARAM_SUPPORT_RTCP: {
			*value = live555_support_rtcp;
			ret = E_OK;
			break;
		}

	case IACOMMON_PARAM_RTSP_FIXED_STREAM0: {
			*value = live555_rtsp_fixed_stream0;
			ret = E_OK;
			break;
		}
	}
	return ret;
}

static ER _ImageApp_Common_GetParam_LviewNvtGroup(UINT32 id, UINT32 param, UINT32* value)
{
	ER ret = E_NOSPT;

#if (IACOMMON_BRANCH != IACOMMON_BR_1_25)
	switch (param) {
	case IACOMMON_PARAM_LVIEWNVT_PORTNUM: {
			*value = iacomm_LviewNvt_portNum;
			ret = E_OK;
			break;
		}

	case IACOMMON_PARAM_LVIEWNVT_THREADPRIORITY: {
			*value = iacomm_LviewNvt_threadPriority;
			ret = E_OK;
			break;
		}

	case IACOMMON_PARAM_LVIEWNVT_FRAMERATE: {
			*value = iacomm_LviewNvt_frameRate;
			ret = E_OK;
			break;
		}

	case IACOMMON_PARAM_LVIEWNVT_SOCKBUFSIZE: {
			*value = iacomm_LviewNvt_sockbufSize;
			ret = E_OK;
			break;
		}

	case IACOMMON_PARAM_LVIEWNVT_TIMEOUTCNT: {
			*value = iacomm_LviewNvt_timeoutCnt;
			ret = E_OK;
			break;
		}

	case IACOMMON_PARAM_LVIEWNVT_TOS: {
			*value = iacomm_LviewNvt_tos;
			ret = E_OK;
			break;
		}
	}
#endif
	return ret;
}

static ER _ImageApp_Common_GetParam_MiscGroup(UINT32 id, UINT32 param, UINT32* value)
{
	ER ret = E_NOSPT;

	switch (param) {
	case IACOMMON_PARAM_MISC_DEBUG_MSG_EN: {
			*value = g_ia_common_trace_on;
			ret = E_OK;
			break;
		}

	case IACOMMON_PARAM_MISC_KEEP_SEM_FLAG: {
			*value = iacomm_keep_sem_flag;
			ret = E_OK;
			break;
		}

	case IACOMMON_PARAM_MISC_USE_EXTERNAL_STREAMING: {
			*value = iacomm_use_external_streaming;
			ret = E_OK;
			break;
		}
	}
	return ret;
}

ER ImageApp_Common_GetParam(UINT32 id, UINT32 param, UINT32* value)
{
	ER ret;

	if (value == NULL) {
		return E_SYS;
	}

	if (param >= IACOMMON_PARAM_RTSP_GROUP_BEGIN && param < IACOMMON_PARAM_RTSP_GROUP_END) {
		ret = _ImageApp_Common_GetParam_RtspGroup(id, param, value);
	} else if (param >= IACOMMON_PARAM_LVIEWNVT_GROUP_BEGIN && param < IACOMMON_PARAM_LVIEWNVT_GROUP_END) {
		ret = _ImageApp_Common_GetParam_LviewNvtGroup(id, param, value);
	} else if (param >= IACOMMON_PARAM_MISC_GROUP_BEGIN && param < IACOMMON_PARAM_MISC_GROUP_END) {
		ret = _ImageApp_Common_GetParam_MiscGroup(id, param, value);
	} else {
		DBG_WRN("Paramter id %x not matched.\r\n", param);
		ret = E_SYS;
	}
	return ret;
}

