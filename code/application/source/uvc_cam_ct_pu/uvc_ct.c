#include "hd_type.h"
#include "uvc_ct.h"
#include "hdal.h"
#include "hd_debug.h"
#include "UVAC.h"

extern void Set_CT_Zoom_Absolute(UINT32 indx);
UINT32 ct_zoom_indx = 0;

static BOOL xUvac_CT_Zoom_Absolute(UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = TRUE;

	switch(request)
	{
		case GET_LEN:
			*pDataLen = 2;	//must be 2 according to UVC spec
			pData[0] = 2;
			pData[1] = 0;
			break;
		case GET_INFO:
			*pDataLen = 1;	//must be 1 according to UVC spec
			pData[0] = SUPPORT_GET_REQUEST | SUPPORT_SET_REQUEST;
			break;
		case GET_MIN:
			*pDataLen = 2;
			pData[0] = 0;
			pData[1] = 0;
			break;
		case GET_MAX:
			*pDataLen = 2;
			pData[0] = 0x28;	//40
			pData[1] = 0;
			break;
		case GET_RES:
			*pDataLen = 2;
			pData[0] = 1;
			pData[1] = 0;
			break;
		case GET_DEF:
			*pDataLen = 2;
			pData[0] = 0;		//0
			pData[1] = 0;
			break;
		case GET_CUR:
			printf("%s: GET_CUR\r\n", __func__);
			*pDataLen = 2;
			pData[0] = ct_zoom_indx;
			pData[1] = 0;
			break;
		case SET_CUR:
			printf("%s: SET_CUR pData=0x%X 0x%X =%d\r\n", __func__, pData[1], pData[0], (pData[1]<<8)+pData[0]);
			ct_zoom_indx = *pData;		// 0 ~ 40->depending on max dzoom factor
			Set_CT_Zoom_Absolute(ct_zoom_indx);
			break;
		default:
			ret = FALSE;
			break;
	}
	return ret;
}

BOOL xUvacCT_CB(UINT32 CS, UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = FALSE;

	printf("%s: CS=0x%x, request = 0x%X,  pData=0x%x, len=%d\r\n", __func__, CS, request, pData, *pDataLen);
	switch (CS){
		case CT_SCANNING_MODE_CONTROL:
		case CT_AE_MODE_CONTROL:
		case CT_AE_PRIORITY_CONTROL:
		case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
		case CT_EXPOSURE_TIME_RELATIVE_CONTROL:
		case CT_FOCUS_ABSOLUTE_CONTROL:
		case CT_FOCUS_RELATIVE_CONTROL:
		case CT_FOCUS_AUTO_CONTROL:
		case CT_IRIS_ABSOLUTE_CONTROL:
		case CT_IRIS_RELATIVE_CONTROL:
		case CT_ZOOM_ABSOLUTE_CONTROL:
			ret = xUvac_CT_Zoom_Absolute(request, pData, pDataLen);
			break;
		case CT_ZOOM_RELATIVE_CONTROL:
		case CT_PANTILT_ABSOLUTE_CONTROL:
		case CT_PANTILT_RELATIVE_CONTROL:
		case CT_ROLL_ABSOLUTE_CONTROL:
		case CT_ROLL_RELATIVE_CONTROL:
		case CT_PRIVACY_CONTROL:
			//not supported now
			break;
		default:
			break;
	}

	if (ret && request == SET_CUR) {
		*pDataLen = 0;
	}
	return ret;
}

