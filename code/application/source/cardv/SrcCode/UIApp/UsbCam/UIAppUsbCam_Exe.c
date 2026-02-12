#include "AppControl/AppControl.h"
#include "PrjCfg.h"
#include "UIApp/AppLib.h"
#include "GxUSB.h"
#include "UIAppUsbCam.h"
#include "ImageApp/ImageApp_UsbMovie.h"
#include "vendor_isp.h"
#include "SysSensor.h"

#define UVAC_CT_PU_FUNC          ENABLE

//#NT#2022/9/15#Philex Lin - begin
// add CT/PU config
#if UVAC_CT_PU_FUNC
#define UVCCAM_CUSTOMER_NORMAL	0
#define UVCCAM_CUSTOMER_MSFT	1
#define UVCCAM_CUSTOMER_OPTION	UVCCAM_CUSTOMER_MSFT

#if (UVCCAM_CUSTOMER_OPTION == UVCCAM_CUSTOMER_MSFT)
// PU control
#define PU_BRIGHTNESS_EN                            ENABLE
#define PU_CONTRAST_EN                              ENABLE
#define PU_HUE_EN                                   DISABLE
#define PU_SATURATION_EN                            ENABLE
#define PU_SHARPNESS_EN                             ENABLE
#define PU_GAMMA_EN                                 DISABLE
#define PU_WB_TEMP_EN                               ENABLE
#define PU_BACKLIGHT_COMP_EN                        DISABLE
#define PU_GAIN_EN                                  DISABLE
#define PU_POWERLINE_FREQ_EN                        ENABLE
#define PU_WB_AUTO_EN                               ENABLE

// CT control
#define CT_SCANNING_MODE_EN                         DISABLE
#define CT_AE_MODE_EN                               ENABLE
#define CT_AE_PRIORITY_EN                           DISABLE
#define CT_EXPOSURE_TIME_ABSOLUTE_EN                ENABLE
#define CT_EXPOSURE_TIME_RELATIVE_EN                DISABLE
#define CT_FOCUS_RELATIVE_EN                        DISABLE
#define CT_ZOOM_ABSOLUTE_EN                         DISABLE
#define CT_ZOOM_RELATIVE_EN                         DISABLE
#define CT_PANTILT_ABSOLUTE_EN                      DISABLE
#define CT_PANTILT_RELATIVE_EN                      DISABLE
#define CT_FOCUS_ABSOLUTE_EN                        DISABLE
#define CT_AUTO_FOCUS_EN                            DISABLE
#else
// PU control
#define PU_BRIGHTNESS_EN                            ENABLE
#define PU_CONTRAST_EN                              ENABLE
#define PU_HUE_EN                                   ENABLE
#define PU_SATURATION_EN                            ENABLE
#define PU_SHARPNESS_EN                             ENABLE
#define PU_GAMMA_EN                                 DISABLE
#define PU_WB_TEMP_EN                               ENABLE
#define PU_BACKLIGHT_COMP_EN                        ENABLE
#define PU_GAIN_EN                                  DISABLE
#define PU_POWERLINE_FREQ_EN                        ENABLE
#define PU_WB_AUTO_EN                               DISABLE

// CT control
#define CT_SCANNING_MODE_EN                         DISABLE
#define CT_AE_MODE_EN                               DISABLE
#define CT_AE_PRIORITY_EN                           DISABLE
#define CT_EXPOSURE_TIME_ABSOLUTE_EN                DISABLE
#define CT_EXPOSURE_TIME_RELATIVE_EN                DISABLE
#define CT_FOCUS_RELATIVE_EN                        DISABLE
#define CT_ZOOM_ABSOLUTE_EN                         DISABLE
#define CT_ZOOM_RELATIVE_EN                         DISABLE
#define CT_PANTILT_ABSOLUTE_EN                      DISABLE
#define CT_PANTILT_RELATIVE_EN                      DISABLE
#define CT_FOCUS_ABSOLUTE_EN                        DISABLE
#define CT_AUTO_FOCUS_EN                            DISABLE
#endif

#if PU_BRIGHTNESS_EN
#define PU_BRIGHTNESS_MSK                           0x0001
#else
#define PU_BRIGHTNESS_MSK                           0x0000
#endif

#if PU_CONTRAST_EN
#define PU_CONTRAST_MSK                             0x0002
#else
#define PU_CONTRAST_MSK                             0x0000
#endif

#if PU_HUE_EN
#define PU_HUE_MSK                                  0x0004
#else
#define PU_HUE_MSK                                  0x0000
#endif

#if PU_SATURATION_EN
#define PU_SATURATION_MSK                           0x0008
#else
#define PU_SATURATION_MSK                           0x0000
#endif

#if PU_SHARPNESS_EN
#define PU_SHARPNESS_MSK                            0x0010
#else
#define PU_SHARPNESS_MSK                            0x0000
#endif

#if PU_GAMMA_EN
#define PU_GAMMA_MSK                                0x0020
#else
#define PU_GAMMA_MSK                                0x0000
#endif

#if PU_WB_TEMP_EN
#define PU_WB_TEMP_MSK                              0x0040
#else
#define PU_WB_TEMP_MSK                              0x0000
#endif

#if PU_BACKLIGHT_COMP_EN
#define PU_BACKLIGHT_COMP_MSK                       0x0100
#else
#define PU_BACKLIGHT_COMP_MSK                       0x0000
#endif

#if PU_GAIN_EN
#define PU_GAIN_MSK                                 0x0200
#else
#define PU_GAIN_MSK                                 0x0000
#endif

#if PU_POWERLINE_FREQ_EN
#define PU_POWERLINE_FREQ_MSK                       0x0400
#else
#define PU_POWERLINE_FREQ_MSK                       0x0000
#endif

#if PU_WB_AUTO_EN
#define PU_WB_AUTO_MSK                              0x1000
#else
#define PU_WB_AUTO_MSK                              0x0000
#endif

#if CT_SCANNING_MODE_EN
#define CT_SCANNING_MODE_MSK                        0x0001
#else
#define CT_SCANNING_MODE_MSK                        0x0000
#endif

#if CT_AE_MODE_EN
#define CT_AE_MODE_MSK                              0x0002
#else
#define CT_AE_MODE_MSK                              0x0000
#endif

#if CT_AE_PRIORITY_EN
#define CT_AE_PRIORITY_MSK                          0x0004
#else
#define CT_AE_PRIORITY_MSK                          0x0000
#endif

#if CT_EXPOSURE_TIME_ABSOLUTE_EN
#define CT_EXPOSURE_TIME_ABSOLUTE_MSK               0x0008
#else
#define CT_EXPOSURE_TIME_ABSOLUTE_MSK               0x0000
#endif

#if CT_EXPOSURE_TIME_RELATIVE_EN
#define CT_EXPOSURE_TIME_RELATIVE_MSK               0x0010
#else
#define CT_EXPOSURE_TIME_RELATIVE_MSK               0x0000
#endif

#if CT_FOCUS_ABSOLUTE_EN
#define CT_FOCUS_ABSOLUTE_MSK                       0x0020
#else
#define CT_FOCUS_ABSOLUTE_MSK                       0x0000
#endif

#if CT_FOCUS_RELATIVE_EN
#define CT_FOCUS_RELATIVE_MSK                       0x0040
#else
#define CT_FOCUS_RELATIVE_MSK                       0x0000
#endif

#if CT_ZOOM_ABSOLUTE_EN
#define CT_ZOOM_ABSOLUTE_MSK                        0x0200
#else
#define CT_ZOOM_ABSOLUTE_MSK                        0x0000
#endif

#if CT_ZOOM_RELATIVE_EN
#define CT_ZOOM_RELATIVE_MSK                        0x0400
#else
#define CT_ZOOM_RELATIVE_MSK                        0x0000
#endif

#if CT_PANTILT_ABSOLUTE_EN
#define CT_PANTILT_ABSOLUTE_MSK                     0x0800
#else
#define CT_PANTILT_ABSOLUTE_MSK                     0x0000
#endif

#if CT_PANTILT_RELATIVE_EN
#define CT_PANTILT_RELATIVE_MSK                     0x1000
#else
#define CT_PANTILT_RELATIVE_MSK                     0x0000
#endif

#if CT_AUTO_FOCUS_EN
#define CT_AUTO_FOCUS_MSK                           0x20000
#else
#define CT_AUTO_FOCUS_MSK                           0x00000
#endif


UINT32 uvc_ct_controls = (CT_SCANNING_MODE_MSK|CT_AE_MODE_MSK|CT_AE_PRIORITY_MSK|CT_EXPOSURE_TIME_ABSOLUTE_MSK|
                          CT_EXPOSURE_TIME_RELATIVE_MSK|CT_FOCUS_ABSOLUTE_MSK|CT_FOCUS_RELATIVE_MSK|
                          CT_ZOOM_ABSOLUTE_MSK|CT_ZOOM_RELATIVE_MSK|CT_PANTILT_ABSOLUTE_MSK|CT_AUTO_FOCUS_MSK);
UINT32 uvc_pu_controls = (PU_BRIGHTNESS_MSK|PU_CONTRAST_MSK|PU_HUE_MSK|PU_SATURATION_MSK|
                          PU_SHARPNESS_MSK|PU_GAMMA_MSK|PU_WB_TEMP_MSK|PU_BACKLIGHT_COMP_MSK|
                          PU_GAIN_MSK|PU_POWERLINE_FREQ_MSK|PU_WB_AUTO_MSK);
//#NT#2022/9/15#Philex Lin - end

// Define Bits Containing Capabilities of the control for the Get_Info request (section 4.1.2 in the UVC spec 1.1)
#define SUPPORT_GET_REQUEST                 0x01
#define SUPPORT_SET_REQUEST                 0x02
#define DISABLED_DUE_TO_AUTOMATIC_MODE      0x04
#define AUTOUPDATE_CONTROL                  0x08
#define ASNCHRONOUS_CONTROL                 0x10
#define RESERVED_BIT5                       0x20
#define RESERVED_BIT6                       0x40
#define RESERVED_BIT7                       0x80

//Camera Terminal Control Selectors
#define CT_CONTROL_UNDEFINED                0x00
#define CT_SCANNING_MODE_CONTROL            0x01
#define CT_AE_MODE_CONTROL                  0x02
#define CT_AE_PRIORITY_CONTROL              0x03
#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL 0x04
#define CT_EXPOSURE_TIME_RELATIVE_CONTROL 0x05
#define CT_FOCUS_ABSOLUTE_CONTROL           0x06
#define CT_FOCUS_RELATIVE_CONTROL           0x07
#define CT_FOCUS_AUTO_CONTROL               0x08
#define CT_IRIS_ABSOLUTE_CONTROL            0x09
#define CT_IRIS_RELATIVE_CONTROL            0x0A
#define CT_ZOOM_ABSOLUTE_CONTROL            0x0B
#define CT_ZOOM_RELATIVE_CONTROL            0x0C
#define CT_PANTILT_ABSOLUTE_CONTROL         0x0D
#define CT_PANTILT_RELATIVE_CONTROL         0x0E
#define CT_ROLL_ABSOLUTE_CONTROL            0x0F
#define CT_ROLL_RELATIVE_CONTROL            0x10
#define CT_PRIVACY_CONTROL                  0x11

//Processing Unit Control Selectors
#define PU_CONTROL_UNDEFINED                0x00
#define PU_BACKLIGHT_COMPENSATION_CONTROL 0x01
#define PU_BRIGHTNESS_CONTROL               0x02
#define PU_CONTRAST_CONTROL                 0x03
#define PU_GAIN_CONTROL                     0x04
#define PU_POWER_LINE_FREQUENCY_CONTROL 0x05
#define PU_HUE_CONTROL                      0x06
#define PU_SATURATION_CONTROL               0x07
#define PU_SHARPNESS_CONTROL                0x08
#define PU_GAMMA_CONTROL                    0x09
#define PU_WHITE_BALANCE_TEMPERATURE_CONTROL 0x0A
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 0x0B
#define PU_WHITE_BALANCE_COMPONENT_CONTROL 0x0C
#define PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL 0x0D
#define PU_DIGITAL_MULTIPLIER_CONTROL       0x0E
#define PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL 0x0F
#define PU_HUE_AUTO_CONTROL                 0x10
#define PU_ANALOG_VIDEO_STANDARD_CONTROL 0x11
#define PU_ANALOG_LOCK_STATUS_CONTROL       0x12
#endif

#if 1

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UIApp_UVAC
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////

#define UVAC_VENDCMD_CURCMD                         0x01
#define UVAC_VENDCMD_VERSION                        0x02

#define UVAC_PRJ_SET_LIVEVIEW_SAVEFILE          0  //1V + 1F
#define UVAC_PRJ_SET_LIVEVIEW_1V_ONLY           1  //1V only
#define UVAC_PRJ_SET_LIVEVIEW_2V                2  //2V

#define UVAC_MAX_WIDTH		1920
#define UVAC_MAX_HEIGHT		1080
#define UVAC_MAX_FRAMERATE	30


#if (1)//(DISABLE == UVAC_MODE_2_PATH) //only support 1V
#define UVAC_PRJ_SET_THIS           UVAC_PRJ_SET_LIVEVIEW_1V_ONLY
#else
#define UVAC_PRJ_SET_THIS           UVAC_PRJ_SET_LIVEVIEW_2V
//#define UVAC_PRJ_SET_THIS           UVAC_PRJ_SET_LIVEVIEW_SAVEFILE
//#define UVAC_PRJ_SET_THIS           UVAC_PRJ_SET_LIVEVIEW_1V_ONLY
#endif


#if (0)//(UVAC_PRJ_SET_THIS == UVAC_PRJ_SET_LIVEVIEW_SAVEFILE)
//Single Video liveview and movie-recording a file
#define UVAC_2PATH                  DISABLE
#define UVAC_SAVE_FILE              ENABLE
#define UVAC_TEST_FOR_CPU_EXCEPTION_MFK         DISABLE
#elif (UVAC_PRJ_SET_THIS == UVAC_PRJ_SET_LIVEVIEW_1V_ONLY)
//Single Video liveview only
#define UVAC_2PATH                  DISABLE
#define UVAC_SAVE_FILE              DISABLE
#define UVAC_TEST_FOR_CPU_EXCEPTION_MFK         ENABLE  //ENABLE DISABLE
#else
//Two Video liveview
#define UVAC_2PATH                  ENABLE
#define UVAC_SAVE_FILE              DISABLE
#define UVAC_TEST_FOR_CPU_EXCEPTION_MFK         ENABLE  //ENABLE DISABLE
#endif

#define UVAC_WINUSB_INTF            DISABLE   //ENABLE    DISABLE


UVAC_VEND_DEV_DESC gUIUvacDevDesc = {0};
UINT8 gUIUvacVendCmdVer[8] = {'1', '.', '0', '8', '.', '0', '0', '7'};

typedef struct _VEND_CMD_ {
	UINT32 cmd;
	UINT8 data[36]; //<= 60
} VEND_CMD, *PVEND_CMD;

typedef struct {
	UINT32  uiWidth;
	UINT32  uiHeight;
	UINT32  uiVidFrameRate;
	UINT32  uiH264TBR;
	UINT32  uiMJPEGTBR;
} UVC_TARGET_SIZE_PARAM;

static UVC_TARGET_SIZE_PARAM g_UvcVidSizeTable[] = {
	{1920,  1080,   30,     1800 * 1024,  3 * 1800 * 1024},
	{1280,  720,    30,      800 * 1024,   4 * 800 * 1024},
};

#if UVAC_2PATH
static UVC_TARGET_SIZE_PARAM g_Uvc2VidSizeTable[] = {
	{1920,  1080,   30,     1800 * 1024,  3 * 1800 * 1024},
	{1280,  720,    30,      800 * 1024,   4 * 800 * 1024},
};
#endif

/**
* 1.NVT_UI_UVAC_RESO_CNT < UVAC_VID_RESO_MAX_CNT
* 2.Discrete fps:
*       The values of fps[] must be in a decreasing order and 0 shall be put to the last one.
*       The element, fpsCnt, must be correct for the values in fps[]
* 3.Must be a subset of g_MovieRecSizeTable[] for MFK-available.
*
*/
static UVAC_VID_RESO gUIUvacVidReso[NVT_UI_UVAC_RESO_CNT] = {
	{1920,  1080,   1,      30,      0,      0},        //16:9
	{1280,   720,   1,      30,      0,      0},        //16:9
};

#if UVAC_2PATH
static UVAC_VID_RESO gUvc2MjpgReso[] = {
	{ 1920, 1080,   1,      30,      0,      0},
	{ 1280,  720,   1,      30,      0,      0},
};
static UVAC_VID_RESO gUvc2H264Reso[] = {
	{ 1920, 1080,   1,      30,      0,      0},
	{ 1280,  720,   1,      30,      0,      0},
};
#endif

//NVT_UI_UVAC_AUD_SAMPLERATE_CNT <= UVAC_AUD_SAMPLE_RATE_MAX_CNT
UINT32 gUIUvacAudSampleRate[NVT_UI_UVAC_AUD_SAMPLERATE_CNT] = {
	NVT_UI_FREQUENCY_32K
};

_ALIGNED(64) static UINT16 m_UVACSerialStrDesc3[] = {
	0x0320,                             // 20: size of String Descriptor = 32 bytes
	// 03: String Descriptor type
	'9', '8', '5', '2', '0',            // 98520-00000-001 (default)
	'0', '0', '0', '0', '0',
	'0', '0', '1', '0', '0'
};

_ALIGNED(64) const static UINT8 m_UVACManuStrDesc[] = {
	USB_VENDER_DESC_STRING_LEN * 2 + 2, // size of String Descriptor = 6 bytes
	0x03,                       // 03: String Descriptor type
	USB_VENDER_DESC_STRING
};

_ALIGNED(64) const static UINT8 m_UVACProdStrDesc[] = {
	USB_PRODUCT_DESC_STRING_LEN * 2 + 2, // size of String Descriptor = 6 bytes
	0x03,                       // 03: String Descriptor type
	USB_PRODUCT_DESC_STRING
};

#if UVAC_CT_PU_FUNC
static BOOL xUvacCT_CB(UINT32 CS, UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = FALSE;

	#if (THIS_DBGLVL>=5)
	DBG_DUMP("%s: CS=0x%x, request = 0x%X,  pData=0x%x, len=%d\r\n", __func__, CS, request, pData, *pDataLen);
	if (SET_CUR == request) {
		DumpMemory((UINT32) pData, *pDataLen, 16);
	}
	#endif
	switch (CS){
	case CT_AE_MODE_CONTROL://0x02:
		//ret = TRUE;//if supported
		break;
	case CT_AE_PRIORITY_CONTROL://0x03:
		//ret = TRUE;//if supported
		break;
	//...
	default:
		break;
	}

	if (ret && request == SET_CUR) {
		*pDataLen = 0;
	}
	return ret;
}
static BOOL xUvac_PU_Brightness(UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = TRUE;

	//just a sample for range:0~200   default:100
	switch(request)
    {
    	case GET_INFO:
    		*pDataLen = 1;
            pData[0] = SUPPORT_GET_REQUEST | SUPPORT_SET_REQUEST;
            break;
        case GET_MIN:
        	*pDataLen = 2;
	        pData[0] = 0;
	        pData[1] = 0;
        	break;
        case GET_MAX:
        	*pDataLen = 2;
	        pData[0] = 0xC8;//200
	        pData[1] = 0;
        	break;
        case GET_RES:
            *pDataLen = 2;
	        pData[0] = 1;
	        pData[1] = 0;
            break;
        case GET_DEF:
            *pDataLen = 2;
	        pData[0] = 0x64;//100
	        pData[1] = 0;
            break;
        case GET_CUR:
            *pDataLen = 2;
            //return current xxx value for USB Host
	        pData[0] = 0x6;//just a hard code sample
	        pData[1] = 0;
            break;
        case SET_CUR:
			DBG_DUMP("%s: pData=0x%X 0x%X =%d\r\n", __func__, pData[1], pData[0], (pData[1]<<8)+pData[0] );
			//set brightness
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

	//just a sample for range:0~200   default:100
	switch(request)
    {
    	case GET_INFO:
    		*pDataLen = 1;
            pData[0] = SUPPORT_GET_REQUEST | SUPPORT_SET_REQUEST;
            break;
        case GET_MIN:
        	*pDataLen = 2;
	        pData[0] = 0;
	        pData[1] = 0;
        	break;
        case GET_MAX:
        	*pDataLen = 2;
	        pData[0] = 0xC8;//200
	        pData[1] = 0;
        	break;
        case GET_RES:
            *pDataLen = 2;
	        pData[0] = 1;
	        pData[1] = 0;
            break;
        case GET_DEF:
            *pDataLen = 2;
	        pData[0] = 0x64;//100
	        pData[1] = 0;
            break;
        case GET_CUR:
            *pDataLen = 2;
            //return current xxx value for USB Host
	        pData[0] = 0x6;//just a hard code sample
	        pData[1] = 0;
            break;
        case SET_CUR:
			DBG_DUMP("%s: pData=0x%X 0x%X =%d\r\n", __func__, pData[1], pData[0], (pData[1]<<8)+pData[0] );
			//set contrast
            break;
        default:
			ret = FALSE;
            break;
    }
    return ret;
}
static BOOL xUvac_PU_PowerLineFreq(UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = TRUE;

	switch(request)
    {
    	case GET_INFO:
    		*pDataLen = 1;
            pData[0] = SUPPORT_GET_REQUEST | SUPPORT_SET_REQUEST;
            break;
        case GET_DEF:
            *pDataLen = 1;
	        pData[0] = 2;

            break;
        case GET_CUR:
            *pDataLen = 1;
            //return current xxx value for USB Host
	        pData[0] = 0x2;//just a hard code sample
            break;
        case SET_CUR:
			DBG_DUMP("%s: pData[0]=0x%X \r\n", __func__, pData[0]);
			//set contrast
            break;
        default:
			ret = FALSE;
            break;
    }
    return ret;
}
static BOOL xUvacPU_CB(UINT32 CS, UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = FALSE;

	#if (THIS_DBGLVL>=5)
	DBG_DUMP("%s: CS=0x%x, request = 0x%X,  pData=0x%x, len=%d\r\n", __func__, CS, request, pData, *pDataLen);
	if (SET_CUR == request) {
		DumpMemory((UINT32) pData, *pDataLen, 16);
	}
	#endif
	switch (CS){
	case PU_BACKLIGHT_COMPENSATION_CONTROL://0x01
		//ret = TRUE;//if supported
		break;
	case PU_BRIGHTNESS_CONTROL://0x02:
		ret = xUvac_PU_Brightness(request, pData, pDataLen);
		break;
	case PU_CONTRAST_CONTROL://0x03:
		ret = xUvac_PU_Contrast(request, pData, pDataLen);
		break;
	case PU_POWER_LINE_FREQUENCY_CONTROL:
		ret = xUvac_PU_PowerLineFreq(request, pData, pDataLen);
		break;
	//...
	default:
		break;
	}

	if (ret && request == SET_CUR) {
		*pDataLen = 0;
	}
	return ret;
}
#endif

//======= end movie mode related setting  ======
//0:OK
//1...:TBD
static UINT32 xUvacVendReqCB(PUVAC_VENDOR_PARAM pParam)
{
	if (pParam) {
		DBG_IND("bHostToDevice       = 0x%x\r\n", pParam->bHostToDevice);
		DBG_IND("uiReguest       = 0x%x\r\n",     pParam->uiReguest);
		DBG_IND("uiValue       = 0x%x\r\n",       pParam->uiValue);
		DBG_IND("uiIndex       = 0x%x\r\n",       pParam->uiIndex);
		DBG_IND("uiDataAddr       = 0x%x\r\n",    pParam->uiDataAddr);
		DBG_IND("uiDataSize       = 0x%x\r\n",    pParam->uiDataSize);
	} else {
		DBG_ERR(" Input parameter NULL\r\n");
	}
	return 0;
}

//0:OK
//1...:TBD
static UINT32 xUvacVendReqIQCB(PUVAC_VENDOR_PARAM pParam)
{
	if (pParam) {
		DBG_IND("bHostToDevice       = 0x%x\r\n", pParam->bHostToDevice);
		DBG_IND("uiReguest       = 0x%x\r\n",     pParam->uiReguest);
		DBG_IND("uiValue       = 0x%x\r\n",       pParam->uiValue);
		DBG_IND("uiIndex       = 0x%x\r\n",       pParam->uiIndex);
		DBG_IND("uiDataAddr       = 0x%x\r\n",    pParam->uiDataAddr);
		DBG_IND("uiDataSize       = 0x%x\r\n",    pParam->uiDataSize);
	} else {
		DBG_ERR(" Input parameter NULL\r\n");
	}
	return 0;
}

static void xUSBMakerInit_UVAC(UVAC_VEND_DEV_DESC *pUVACDevDesc)
{
	pUVACDevDesc->pManuStringDesc = (UVAC_STRING_DESC *)m_UVACManuStrDesc;
	pUVACDevDesc->pProdStringDesc = (UVAC_STRING_DESC *)m_UVACProdStrDesc;

	pUVACDevDesc->pSerialStringDesc = (UVAC_STRING_DESC *)m_UVACSerialStrDesc3;

	pUVACDevDesc->VID = USB_VID;
	pUVACDevDesc->PID = USB_PID_PCCAM;

	pUVACDevDesc->fpVendReqCB = xUvacVendReqCB;
	pUVACDevDesc->fpIQVendReqCB = xUvacVendReqIQCB;

}

void xUvac_ConfigMovieSizeCB(PNVT_USBMOVIE_VID_RESO pVidReso)
{
	UINT32 tbr;

	if (pVidReso) {
		if (pVidReso->dev_id == 0) {
			if (pVidReso->codec == UVAC_VIDEO_FORMAT_H264) {
				tbr = g_UvcVidSizeTable[pVidReso->ResoIdx].uiH264TBR;
			} else {
				tbr = g_UvcVidSizeTable[pVidReso->ResoIdx].uiMJPEGTBR;
			}
			DBGD(tbr);
			pVidReso->tbr = tbr;
		}
#if UVAC_2PATH
		else {
			if (pVidReso->codec == UVAC_VIDEO_FORMAT_H264) {
				tbr = g_Uvc2VidSizeTable[pVidReso->ResoIdx].uiH264TBR;
			} else {
				tbr = g_Uvc2VidSizeTable[pVidReso->ResoIdx].uiMJPEGTBR;
			}
			DBGD(tbr);
			pVidReso->tbr = tbr;
		}
#endif
	}
}

UINT8 xUvacWinUSBCB(UINT32 ControlCode, UINT8 CS, UINT8 *pDataAddr, UINT32 *pDataLen)
{
	DBG_IND("xUvacWinUSBCB ctrl=0x%x, cs=0x%x, pData=0x%x, len=%d\r\n", ControlCode, CS, pDataAddr, *pDataLen);
	return TRUE;
}

// sample code for UVC extension unit
#if 0
static BOOL xUvac_EU_ID_01_CB(UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	switch (request){
		case GET_LEN:
			pData[0] = 0x02;
			pData[1] = 0x00;
			*pDataLen = 2;		//must be 2 according to UVC spec
			return TRUE;
		case GET_INFO:
			pData[0] = SUPPORT_GET_REQUEST | SUPPORT_SET_REQUEST;
			*pDataLen = 1;		//must be 1 according to UVC spec
			return TRUE;
		case GET_MIN:
			pData[0] = 0x00;
			pData[1] = 0x00;
			*pDataLen = 2;
			return TRUE;
		case GET_MAX:
			pData[0] = 0xFF;
			pData[1] = 0xFF;
			*pDataLen = 2;
			return TRUE;
		case GET_RES:
			pData[0] = 0x01;
			pData[1] = 0x00;
			*pDataLen = 2;
			return TRUE;
		case GET_DEF:
			pData[0] = 0x00;
			pData[1] = 0x00;
			*pDataLen = 2;
			return TRUE;
		case GET_CUR:
			pData[0] = 0x02;
			pData[1] = 0x00;
			*pDataLen = 0x02;
			return TRUE;
		case SET_CUR:
			// TODO
			return TRUE;
	}
	DBG_ERR("set raw_seq not support!\r\n");
	return FALSE;
}

static BOOL xUvac_EU_ID_04_CB(UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	switch (request){
		case GET_LEN:
			pData[0] = 0x32;
			pData[1] = 0x00;
			*pDataLen = 2;		//must be 2 according to UVC spec
			return TRUE;
		case GET_INFO:
			pData[0] = SUPPORT_GET_REQUEST | SUPPORT_SET_REQUEST;
			*pDataLen = 1;		//must be 1 according to UVC spec
			return TRUE;
		case GET_MIN:
			pData[0] = 0x00;
			pData[1] = 0x00;
			*pDataLen = 2;
			return TRUE;
		case GET_MAX:
			pData[0] = 0xFF;
			pData[1] = 0xFF;
			*pDataLen = 2;
			return TRUE;
		case GET_RES:
			pData[0] = 0x01;
			pData[1] = 0x00;
			*pDataLen = 2;
			return TRUE;
		case GET_DEF:
			pData[0] = 0x00;
			pData[1] = 0x00;
			*pDataLen = 2;
			return TRUE;
		case GET_CUR:
			pData[0] = 0x32;
			pData[1] = 0x00;
			*pDataLen = 0x32;
			return TRUE;
		case SET_CUR:
			{
			DBG_DUMP("%d: ", *pDataLen);
			UINT32 i;
			for (i = 0; i < *pDataLen; i++) {
				DBG_DUMP("%02x ", pData[i]);
			}
			DBG_DUMP("\r\n");
			}
			return TRUE;
	}
	DBG_ERR("set raw_seq not support!\r\n");
	return FALSE;
}

BOOL xUvacEU_CB(UINT32 CS, UINT8 request, UINT8 *pData, UINT32 *pDataLen)
{
	BOOL ret = FALSE;

	if (request == SET_CUR){
		DBG_DUMP("%s: SET_CUR, CS=0x%02X, request = 0x%X,  pData=0x%X, len=%d\r\n", __func__, CS, request, pData, *pDataLen);
	}
	switch (CS){
		case EU_CONTROL_ID_01:
			ret = xUvac_EU_ID_01_CB(request, pData, pDataLen);
			break;
		case EU_CONTROL_ID_02:
		case EU_CONTROL_ID_03:
			break;
		case EU_CONTROL_ID_04:
			ret = xUvac_EU_ID_04_CB(request, pData, pDataLen);
			break;
		case EU_CONTROL_ID_05:
		case EU_CONTROL_ID_06:
		case EU_CONTROL_ID_07:
		case EU_CONTROL_ID_08:
		case EU_CONTROL_ID_09:
		case EU_CONTROL_ID_10:
		case EU_CONTROL_ID_11:
		case EU_CONTROL_ID_12:
		case EU_CONTROL_ID_13:
		case EU_CONTROL_ID_14:
		case EU_CONTROL_ID_15:
		case EU_CONTROL_ID_16:
			//not supported now
			break;
		default:
			break;
	}

	if (request != SET_CUR) {
		DBG_DUMP("%s: CS=0x%02X, request = 0x%X,  pData=0x%X, len=%d\r\n", __func__, CS, request, pData, *pDataLen);
	}

	if (ret && request == SET_CUR){
		*pDataLen = 0;
	}
	return ret;
}
#endif

INT32 UVACExe_OnOpen(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	UVAC_VID_RESO_ARY uvacVidResoAry = {0};
	UVAC_AUD_SAMPLERATE_ARY uvacAudSampleRateAry = {0};
	USBMOVIE_SENSOR_INFO sen_cfg = {0};

	DBG_IND(" ++\r\n");
	Ux_DefaultEvent(pCtrl, NVTEVT_EXE_OPEN, paramNum, paramArray); //=>System_OnSensorAttach

	if (GxUSB_GetIsUSBPlug()) {

		UsbMovie_CommPoolInit();

		ImageApp_UsbMovie_Config(USBMOVIE_CFG_SET_MOVIE_SIZE_CB, (UINT32)xUvac_ConfigMovieSizeCB);

#if UVAC_2PATH
		DBG_IND("UVAC_2PATH Enable, Set Channel:%d\r\n", (UINT32)UVAC_CHANNEL_2V2A);
		ImageApp_UsbMovie_Config(USBMOVIE_CFG_CHANNEL, UVAC_CHANNEL_2V2A);
#else
		DBG_IND("UVAC_2PATH Disable, Set Channel:%d\r\n", (UINT32)UVAC_CHANNEL_1V1A);
		ImageApp_UsbMovie_Config(USBMOVIE_CFG_CHANNEL, UVAC_CHANNEL_1V1A);
#endif

		uvacVidResoAry.aryCnt = NVT_UI_UVAC_RESO_CNT;//MOVIE_SIZE_ID_MAX;
		uvacVidResoAry.pVidResAry = &gUIUvacVidReso[0];
#if NVT_UI_UVAC_RESO_INTERNAL
		DBG_DUMP("Use UVAC internal resolution table\r\n");
#else
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_VID_RESO_ARY, (UINT32)&uvacVidResoAry);
		DBG_IND("Vid Reso:%d\r\n", uvacVidResoAry.aryCnt);
#endif

#if UVAC_2PATH
		uvacVidResoAry.aryCnt = sizeof(gUvc2MjpgReso)/sizeof(UVAC_VID_RESO);
		uvacVidResoAry.pVidResAry = &gUvc2MjpgReso[0];
		uvacVidResoAry.bDefaultFrameIndex = 1;
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_UVC2_MJPG_FRM_INFO, (UINT32)&uvacVidResoAry);

		uvacVidResoAry.aryCnt = sizeof(gUvc2H264Reso)/sizeof(UVAC_VID_RESO);
		uvacVidResoAry.pVidResAry = &gUvc2H264Reso[0];
		uvacVidResoAry.bDefaultFrameIndex = 1;
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_UVC2_H264_FRM_INFO, (UINT32)&uvacVidResoAry);
#endif

		uvacAudSampleRateAry.aryCnt = NVT_UI_UVAC_AUD_SAMPLERATE_CNT;
		uvacAudSampleRateAry.pAudSampleRateAry = &gUIUvacAudSampleRate[0];
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_AUD_SAMPLERATE_ARY, (UINT32)&uvacAudSampleRateAry);
		DBG_IND("Aud SampleRate:%d\r\n", uvacAudSampleRateAry.aryCnt);

		xUSBMakerInit_UVAC(&gUIUvacDevDesc);
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_UVAC_VEND_DEV_DESC, (UINT32)&gUIUvacDevDesc);
		DBG_IND("VID=0x%x, PID=0x%x\r\n", gUIUvacDevDesc.VID, gUIUvacDevDesc.PID);

		// enable to register UVC extension unit callback
		#if 0
		DBG_IND("VendCb:0x%x\r\n", (UINT32)xUvacEU_CB);
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_EU_CTRL, (UINT32)xUvacEU_CB);
		#endif

		//ImageUnit_SetParam(ISF_CTRL, UVAC_PARAM_EU_CTRL, (UINT32)0x01);

#if UVAC_CT_PU_FUNC
              //#NT#2022/9/15#Philex Lin - begin
              // add CT/PU config
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_CT_CONTROLS, uvc_ct_controls);
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_PU_CONTROLS, uvc_pu_controls);
		 //#NT#2022/9/15#Philex Lin - end
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_CT_CB, (UINT32)xUvacCT_CB);
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_PU_CB, (UINT32)xUvacPU_CB);
#endif

#if UVAC_WINUSB_INTF
		DBG_IND("Enable WinUSB\r\n");
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_WINUSB_ENABLE, TRUE);
		DBG_IND("WinUSBCb:0x%x\r\n", (UINT32)xUvacWinUSBCB);
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_WINUSB_CB, (UINT32)xUvacWinUSBCB);
#else
		DBG_IND("DISABLE WinUSB\r\n");
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_WINUSB_ENABLE, FALSE);
#endif

		//xUvac_AE_SetUIInfo(AE_UI_EV, AE_EV_00);
		//DBG_IND("Set Parameter for MFK\r\n");
		//xUvac_InitSetMFK();

		// get sensor info
		sen_cfg.rec_id = 0;
		System_GetSensorInfo(0, SENSOR_DRV_CFG, &(sen_cfg.vcap_cfg));
		System_GetSensorInfo(0, SENSOR_SENOUT_FMT, &(sen_cfg.senout_pxlfmt));
		System_GetSensorInfo(0, SENSOR_CAPOUT_FMT, &(sen_cfg.capout_pxlfmt));
		System_GetSensorInfo(0, SENSOR_DATA_LANE, &(sen_cfg.data_lane));
		System_GetSensorInfo(0, SENSOR_AE_PATH, &(sen_cfg.ae_path));
		System_GetSensorInfo(0, SENSOR_AWB_PATH, &(sen_cfg.awb_path));
		System_GetSensorInfo(0, SENSOR_IQ_PATH, &(sen_cfg.iq_path));
		ImageApp_UsbMovie_Config(USBMOVIE_CFG_SENSOR_INFO, (UINT32)&sen_cfg);
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_VCAP_FUNC, (HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB));
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_VPROC_FUNC, (HD_VIDEOPROC_FUNC_WDR | HD_VIDEOPROC_FUNC_DEFOG | HD_VIDEOPROC_FUNC_COLORNR | HD_VIDEOPROC_FUNC_3DNR));

#if UVAC_2PATH
		sen_cfg.rec_id = 1;
		System_GetSensorInfo(1, SENSOR_DRV_CFG, &(sen_cfg.vcap_cfg));
		System_GetSensorInfo(1, SENSOR_SENOUT_FMT, &(sen_cfg.senout_pxlfmt));
		System_GetSensorInfo(1, SENSOR_CAPOUT_FMT, &(sen_cfg.capout_pxlfmt));
		System_GetSensorInfo(1, SENSOR_DATA_LANE, &(sen_cfg.data_lane));
		System_GetSensorInfo(1, SENSOR_AE_PATH, &(sen_cfg.ae_path));
		System_GetSensorInfo(1, SENSOR_AWB_PATH, &(sen_cfg.awb_path));
		System_GetSensorInfo(1, SENSOR_IQ_PATH, &(sen_cfg.iq_path));
		ImageApp_UsbMovie_Config(USBMOVIE_CFG_SENSOR_INFO, (UINT32)&sen_cfg);
		ImageApp_UsbMovie_SetParam(1, UVAC_PARAM_VCAP_FUNC, (HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB));
		ImageApp_UsbMovie_SetParam(1, UVAC_PARAM_VPROC_FUNC, (HD_VIDEOPROC_FUNC_WDR | HD_VIDEOPROC_FUNC_DEFOG | HD_VIDEOPROC_FUNC_COLORNR | HD_VIDEOPROC_FUNC_3DNR));
#endif

#if (SENSOR_SIEPATGEN == ENABLE)
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_VCAP_PAT_GEN, HD_VIDEOCAP_SEN_PAT_COLORBAR);
#endif

		// set image mirror/flip (HD_VIDEO_DIR_NONE / HD_VIDEO_DIR_MIRRORX / HD_VIDEO_DIR_MIRRORY / HD_VIDEO_DIR_MIRRORXY)
		ImageApp_UsbMovie_SetParam(0, UVAC_PARAM_IPL_MIRROR, HD_VIDEO_DIR_NONE);

		DBG_IND("+ImageApp_UsbMovie_Open\r\n");
		ImageApp_UsbMovie_Open();
		DBG_IND("-ImageApp_UsbMovie_Open\r\n");

	} else {
		DBG_ERR("USB NOT connected!\r\n");
	}
	return NVTEVT_CONSUME;
}
INT32 UVACExe_OnClose(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	DBG_IND(" +\r\n");

	ImageApp_UsbMovie_Close();

	Ux_DefaultEvent(pCtrl, NVTEVT_EXE_CLOSE, paramNum, paramArray);
	DBG_IND(" ---\r\n");

	return NVTEVT_CONSUME;
}
#else
INT32 UVACExe_OnOpen(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	return NVTEVT_CONSUME;
}
INT32 UVACExe_OnClose(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray)
{
	return NVTEVT_CONSUME;
}
#endif

////////////////////////////////////////////////////////////
EVENT_ENTRY CustomUSBPCCObjCmdMap[] = {
	{NVTEVT_EXE_OPEN,               UVACExe_OnOpen},
	{NVTEVT_EXE_CLOSE,              UVACExe_OnClose},
	{NVTEVT_NULL,                   0},  //End of Command Map
};

CREATE_APP(CustomUSBPCCObj, APP_SETUP)

