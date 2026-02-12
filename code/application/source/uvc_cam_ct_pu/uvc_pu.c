#include "hd_type.h"
#include "uvc_pu.h"
#include "hdal.h"
#include "hd_debug.h"
#include "UVAC.h"
#include "ae_ui.h"
#include "vendor_isp.h"

static BOOL xUvac_PU_BlCompensation(UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = TRUE;
	static UINT8 init = 0;
	//AE_TUNING_UICMD info_cmd = {0};
	//IPCAM_STREAM_INFO *pStreamInfo = UIAppIPCam_get_streamInfo();
	//UINT32 iUVACPath = UIAppIPCam_get_UVACPath();
	//UINT8 sensor_id = pStreamInfo[iUVACPath].sensor;

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
			pData[0] = AE_EV_N4;
			pData[1] = 0;
			break;
		case GET_MAX:
			*pDataLen = 2;
			pData[0] = AE_EV_P4;
			pData[1] = 0;
			break;
		case GET_RES:
			*pDataLen = 2;
			pData[0] = 1;
			pData[1] = 0;
			break;
		case GET_DEF:
			*pDataLen = 2;
			pData[0] = AE_EV_00;
			pData[1] = 0;
			break;
		case GET_CUR:
			printf("%s: GET_CUR\r\n", __func__);
			//*pDataLen = 2;
			//if (!init){
			//	AETuning_Get_Cmd(sensor_id, AE_GET_UICMD, (UINT32)&info_cmd, sizeof(AE_TUNING_UICMD));
			//	info_cmd.ui_cmd.ev_comp = AE_EV_00;
			//	AETuning_Set_Cmd(sensor_id, AE_SET_UICMD, (UINT32)&info_cmd, sizeof(AE_TUNING_UICMD));
			//	init++;	//init done
			//}
			//AETuning_Get_Cmd(sensor_id, AE_GET_UICMD, (UINT32)&info_cmd, sizeof(AE_TUNING_UICMD));
			//pData[0] = info_cmd.ui_cmd.ev_comp;
			//pData[1] = 0;
			break;
		case SET_CUR:
			printf("%s: SET_CUR\r\n", __func__);
                        printf("%s: SET_CUR pData=0x%X 0x%X =%d\r\n", __func__, pData[1], pData[0], (pData[1]<<8)+pData[0]);
			//set backlight compensation
			//AETuning_Get_Cmd(sensor_id, AE_GET_UICMD, (UINT32)&info_cmd, sizeof(AE_TUNING_UICMD));
			//info_cmd.ui_cmd.ev_comp = pData[0];
			//AETuning_Set_Cmd(sensor_id, AE_SET_UICMD, (UINT32)&info_cmd, sizeof(AE_TUNING_UICMD));
			break;
		default:
			ret = FALSE;
			break;
	}
	return ret;
}

static BOOL xUvac_PU_Brightness(UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = TRUE;
    IQT_BRIGHTNESS_LV brightness_lv = {0};
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
			pData[0] = 0xC8;	//200
			pData[1] = 0;
			break;
		case GET_RES:
			*pDataLen = 2;
			pData[0] = 1;
			pData[1] = 0;
			break;
		case GET_DEF:
			*pDataLen = 2;
			pData[0] = 0x64;	//100
			pData[1] = 0;
			break;
		case GET_CUR:
			printf("%s: GET_CUR\r\n", __func__);
			*pDataLen = 2;
			brightness_lv.id = 0;
			vendor_isp_get_iq(IQT_ITEM_BRIGHTNESS_LV, &brightness_lv);
			pData[0] = brightness_lv.lv;
			pData[1] = 0;
			break;
		case SET_CUR:
            printf("%s: SET_CUR pData=0x%X 0x%X =%d\r\n", __func__, pData[1], pData[0], (pData[1]<<8)+pData[0]);
            brightness_lv.id = 0;
            brightness_lv.lv = pData[0];          // 0 ~ 40->depending on max dzoom factor
            printf("%s: brightness_lv.lv 0x%X\r\n", __func__, brightness_lv.lv);
            vendor_isp_set_iq(IQT_ITEM_BRIGHTNESS_LV, &brightness_lv);
			break;
		default:
			ret = FALSE;
			break;
	}
	return ret;
}

static BOOL xUvac_PU_Contrast(UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = TRUE;
	IQT_CONTRAST_LV contrast_lv = {0};
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
			pData[0] = 0xC8;	//200
			pData[1] = 0;
			break;
		case GET_RES:
			*pDataLen = 2;
			pData[0] = 1;
			pData[1] = 0;
			break;
		case GET_DEF:
			*pDataLen = 2;
			pData[0] = 0x64;	//100
			pData[1] = 0;
			break;
		case GET_CUR:
			printf("%s: GET_CUR\r\n", __func__);
			*pDataLen = 2;
			contrast_lv.id = 0;
			vendor_isp_get_iq(IQT_ITEM_CONTRAST_LV, &contrast_lv);
			pData[0] = contrast_lv.lv;
			pData[1] = 0;
			break;
		case SET_CUR:
            printf("%s: SET_CUR pData=0x%X 0x%X =%d\r\n", __func__, pData[1], pData[0], (pData[1]<<8)+pData[0]);
            contrast_lv.id = 0;
            contrast_lv.lv = pData[0];          // 0 ~ 40->depending on max dzoom factor
            printf("%s: contrast_lv.lv 0x%X\r\n", __func__, contrast_lv.lv);
            vendor_isp_set_iq(IQT_ITEM_CONTRAST_LV, &contrast_lv);

			break;
		default:
			ret = FALSE;
			break;
	}
	return ret;
}

static BOOL xUvac_PU_PowerFreq(UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = TRUE;
	static UINT8 init = 0;
	//AE_TUNING_CURVEGEN info_curve = {0};
	//IPCAM_STREAM_INFO *pStreamInfo = UIAppIPCam_get_streamInfo();
	//UINT32 iUVACPath = UIAppIPCam_get_UVACPath();
	//UINT8 sensor_id = pStreamInfo[iUVACPath].sensor;

	switch(request)
	{
		case GET_LEN:
			*pDataLen = 1;	//must be 1 according to UVC spec
			pData[0] = 2;
			pData[1] = 0;
			break;
		case GET_INFO:
			*pDataLen = 1;	//must be 1 according to UVC spec
			pData[0] = SUPPORT_GET_REQUEST | SUPPORT_SET_REQUEST;
			break;
		case GET_MIN:
			*pDataLen = 1;
			pData[0] = 0;		//0:Disabled, 1:50Hz, 2:60Hz
			break;
		case GET_MAX:
			*pDataLen = 1;
			pData[0] = 2;		//0:Disabled, 1:50Hz, 2:60Hz
			break;
		case GET_RES:
			*pDataLen = 1;
			pData[0] = 1;
			break;
		case GET_DEF:
			*pDataLen = 1;
			pData[0] = 2;		//0:Disabled, 1:50Hz, 2:60Hz
			break;
		case GET_CUR:
			printf("%s: GET_CUR\r\n", __func__);
			//*pDataLen = 1;
			//AETuning_Get_Cmd(sensor_id, AE_GET_CURVEGEN, (UINT32)&info_curve, sizeof(AE_TUNING_CURVEGEN));
			//if (info_curve.curvegen.freq == AE_FLICKER_50HZ){
			//	pData[0] = 1;	//0:Disabled, 1:50Hz, 2:60Hz
			//} else {
			//	pData[0] = 2;	//0:Disabled, 1:50Hz, 2:60Hz
			//}
			break;
		case SET_CUR:
			printf("%s: SET_CUR\r\n", __func__);
                        printf("%s: SET_CUR pData=0x%X 0x%X =%d\r\n", __func__, pData[1], pData[0], (pData[1]<<8)+pData[0]);
			//set power frequency
			//if (!init){
			//	//Windows host send "Disable power freqency" when enumaeration, ignore
			//	init++;	//init done
			//} else {
			//	AETuning_Get_Cmd(sensor_id, AE_GET_CURVEGEN, (UINT32)&info_curve, sizeof(AE_TUNING_CURVEGEN));
			//	if (pData[0] == 0x01){	//50Hz
			//		info_curve.curvegen.freq = AE_FLICKER_50HZ;
			//	} else {	//disabled or 60Hz
			//		info_curve.curvegen.freq = AE_FLICKER_60HZ;
			//	}
			//	AETuning_Set_Cmd(sensor_id, AE_SET_CURVEGEN, (UINT32)&info_curve, sizeof(AE_TUNING_CURVEGEN));
			//}
			break;
		default:
			ret = FALSE;
			break;
	}
	return ret;
}

static BOOL xUvac_PU_Hue(UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = TRUE;
	IQT_HUE_SHIFT hue_shift = {0};
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
			pData[0] = 0x68;	//360
			pData[1] = 0x01;
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
			hue_shift.id = 0;
			vendor_isp_get_iq(IQT_ITEM_HUE_SHIFT, &hue_shift);
			pData[0] = hue_shift.hue_shift;
			pData[1] = 0;
			break;
		case SET_CUR:
            printf("%s: SET_CUR pData=0x%X 0x%X =%d\r\n", __func__, pData[1], pData[0], (pData[1]<<8)+pData[0]);
            hue_shift.id = 0;
            hue_shift.hue_shift = pData[0];          // 0 ~ 40->depending on max dzoom factor
            printf("%s: brightness_lv.lv 0x%X\r\n", __func__, hue_shift.hue_shift);
            vendor_isp_set_iq(IQT_ITEM_HUE_SHIFT, &hue_shift);
			break;
		default:
			ret = FALSE;
			break;
	}
	return ret;
}

static BOOL xUvac_PU_Saturation(UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = TRUE;
	IQT_SATURATION_LV saturation_lv = {0};
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
			pData[0] = 0xC8;	//200
			pData[1] = 0;
			break;
		case GET_RES:
			*pDataLen = 2;
			pData[0] = 1;
			pData[1] = 0;
			break;
		case GET_DEF:
			*pDataLen = 2;
			pData[0] = 0x64;	//100
			pData[1] = 0;
			break;
		case GET_CUR:
			printf("%s: GET_CUR\r\n", __func__);
			*pDataLen = 2;
			saturation_lv.id = 0;
			vendor_isp_get_iq(IQT_ITEM_SATURATION_LV, &saturation_lv);
			pData[0] = saturation_lv.lv;
			pData[1] = 0;
			break;
		case SET_CUR:
            printf("%s: SET_CUR pData=0x%X 0x%X =%d\r\n", __func__, pData[1], pData[0], (pData[1]<<8)+pData[0]);
            saturation_lv.id = 0;
            saturation_lv.lv = pData[0];          // 0 ~ 40->depending on max dzoom factor
            printf("%s: saturation_lv.lv 0x%X\r\n", __func__, saturation_lv.lv);
            vendor_isp_set_iq(IQT_ITEM_SATURATION_LV, &saturation_lv);
			break;
		default:
			ret = FALSE;
			break;
	}
	return ret;
}

static BOOL xUvac_PU_Sharpness(UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = TRUE;
	IQT_SHARPNESS_LV sharpness_lv = {0};
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
			pData[0] = 0xC8;	//200
			pData[1] = 0;
			break;
		case GET_RES:
			*pDataLen = 2;
			pData[0] = 1;
			pData[1] = 0;
			break;
		case GET_DEF:
			*pDataLen = 2;
			pData[0] = 0x64;	//100
			pData[1] = 0;
			break;
		case GET_CUR:
			printf("%s: GET_CUR\r\n", __func__);
			*pDataLen = 2;
			sharpness_lv.id = 0;
			vendor_isp_get_iq(IQT_ITEM_SHARPNESS_LV, &sharpness_lv);
			pData[0] = sharpness_lv.lv;
			pData[1] = 0;
			break;
		case SET_CUR:
            printf("%s: SET_CUR pData=0x%X 0x%X =%d\r\n", __func__, pData[1], pData[0], (pData[1]<<8)+pData[0]);
            sharpness_lv.id = 0;
            sharpness_lv.lv = pData[0];          // 0 ~ 40->depending on max dzoom factor
            printf("%s: sharpness_lv.lv 0x%X\r\n", __func__, sharpness_lv.lv);
            vendor_isp_set_iq(IQT_ITEM_SHARPNESS_LV, &sharpness_lv);
			break;
		default:
			ret = FALSE;
			break;
	}
	return ret;
}

BOOL xUvacPU_CB(UINT32 CS, UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = FALSE;

	printf("%s: CS=0x%x, request = 0x%X,  pData=0x%x, len=%d\r\n", __func__, CS, request, pData, *pDataLen);
	switch (CS){
	case PU_BACKLIGHT_COMPENSATION_CONTROL:
		ret = xUvac_PU_BlCompensation(request, pData, pDataLen);
		break;
	case PU_BRIGHTNESS_CONTROL:
		ret = xUvac_PU_Brightness(request, pData, pDataLen);
		break;
	case PU_CONTRAST_CONTROL:
		ret = xUvac_PU_Contrast(request, pData, pDataLen);
		break;
	case PU_GAIN_CONTROL:
		//not supported now
		break;
	case PU_POWER_LINE_FREQUENCY_CONTROL:
		ret = xUvac_PU_PowerFreq(request, pData, pDataLen);
		break;
	case PU_HUE_CONTROL:
		ret = xUvac_PU_Hue(request, pData, pDataLen);
		break;
	case PU_SATURATION_CONTROL:
		ret = xUvac_PU_Saturation(request, pData, pDataLen);
		break;
	case PU_SHARPNESS_CONTROL:
		ret = xUvac_PU_Sharpness(request, pData, pDataLen);
		break;
	case PU_GAMMA_CONTROL:
	case PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
	case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
	case PU_WHITE_BALANCE_COMPONENT_CONTROL:
	case PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL:
	case PU_DIGITAL_MULTIPLIER_CONTROL:
	case PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL:
	case PU_HUE_AUTO_CONTROL:
	case PU_ANALOG_VIDEO_STANDARD_CONTROL:
	case PU_ANALOG_LOCK_STATUS_CONTROL:
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

