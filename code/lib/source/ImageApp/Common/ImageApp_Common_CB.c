#include "ImageApp_Common_int.h"

///// Cross file variables
IACommonUserEventCb *g_ia_common_usercb = NULL;
IACommonRtspParseUrlCb *g_ia_common_rtsp_parse_url_cb = NULL;
///// Noncross file variables


ER ImageApp_Common_RegUserCB(IACommonUserEventCb *fncb)
{
	g_ia_common_usercb = fncb;

	return E_OK;
}

ER ImageApp_Common_RegRtspParseUrlCB(IACommonRtspParseUrlCb *fncb)
{
	g_ia_common_rtsp_parse_url_cb = fncb;

	return E_OK;
}

