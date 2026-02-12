#include "GxVideo.h"
#include "GxVideo_int.h"
#include "Dx.h"
#include "DxDisplay.h"
#include "DxApi.h"
#include "hdal.h"
#include "kwrap/error_no.h"
#include "vendor_videoout.h"

#ifndef LAYER_NUM
#define _DD(id)                     (((id)>>4)&0x0f)
#define _DL(id)                     ((id)&0x0f)
#define _LayerID(dev,lyr)           ((((dev)&0x0f)<<4)|((lyr)&0x0f))

//DISPLAY layer id
#define LAYER_OSD1                  0x00
#define LAYER_OSD2                  0x01
#define LAYER_VDO1                  0x02
#define LAYER_VDO2                  0x03
#define LAYER_OUTPUT                0x04    //output = mix ( osd1, osd2, vdo1, vdo2 )
#define LAYER_FACE                  0x05    //face tracing frame
#define LAYER_ALL                   0x0F

#define LAYER_NUM                   4       //total input layer
#define LAYER_MAX                   (LAYER_NUM+1)
#endif

#if defined(_BSP_NA51000_)
#define DEVICE_COUNT	2
#else
#define DEVICE_COUNT	1
#endif

static HD_PATH_ID video_out_ctrl[2] = {0};
static HD_PATH_ID video_out_path[2] = {0};

static BOOL   bForceDetIns	= 0;
static BOOL   bForceDetDir	= 0;

static UINT32 g_DispDev[2] = {0}; //0:Lock, Non-0:Unlcok
static BOOL   g_DispRotate[2] = {FALSE, FALSE};
static BOOL   g_DispState[2] = {DISPLAY_ACTIVE_STOP, DISPLAY_ACTIVE_STOP};
//static UINT32 g_DispMode[2] = {0xffffffff, 0xffffffff}; //mode
#if 0
static USIZE	g_DispWinSize[2] = {0};	//current size
static ISIZE g_oldsize[2] = {0};
static ISIZE g_newsize[2] = {0};
#endif

GX_CALLBACK_PTR g_fpDispCB = 0;

void GxVideo_RegCB(GX_CALLBACK_PTR fpDispCB)
{
    DBG_FUNC_BEGIN("\r\n");
	g_fpDispCB = fpDispCB;
}


//init
// ->default mode
// ->default window en
// ->default size?
// ->lock
void GxVideo_InitDevice(UINT32 DevID)
{
    DBG_FUNC_BEGIN("\r\n");
}

//exit
// assert(under lock)
// ->xxx
void GxVideo_ExitDevice(UINT32 DevID)
{
    DBG_FUNC_BEGIN("\r\n");
}


void GxVideo_DumpInfo(void)
{
    DBG_FUNC_BEGIN("\r\n");
}

/*
open
 assert(under lock)
 -> set device ENABLE = TRUE
 ---> open device by current mode
 -> get new size, resize buffer/window
 -> unlock
*/
INT32 GxVideo_OpenDevice(UINT32 DevID, UINT32 NewDevObj, UINT32 mode)
{
    HD_RESULT ret = 0;
	HD_VIDEOOUT_MODE videoout_mode ={0};
	UINT32 cDevID = _DD(DevID);

#if (DEVICE_COUNT < 2)
	if (cDevID >= 1) {
		DBG_ERR("DevID%d no support\r\n", cDevID);
		return HD_ERR_NOT_SUPPORT;
	}
#endif

    DBG_FUNC_BEGIN("\r\n");

	if (Dx_Open((DX_HANDLE)NewDevObj) != DX_OK) { //here will update current size to INFOBUF
		DBG_ERR("Dx_Open fail\r\n");
		return E_PAR;
	}

    if((ret = hd_videoout_init()) != HD_OK){
        return ret;
    }
    if (DevID == 0) {
    	ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &video_out_ctrl[cDevID]); //open this for device control
    	if(ret!=HD_OK)
    		return ret;

    	memset((void *)&videoout_mode,0,sizeof(HD_VIDEOOUT_MODE));
    	videoout_mode.output_type = HD_COMMON_VIDEO_OUT_LCD;
    	videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
    	videoout_mode.output_mode.lcd = HD_VIDEOOUT_LCD_0;
    	ret = hd_videoout_set(video_out_ctrl[cDevID], HD_VIDEOOUT_PARAM_MODE, &videoout_mode);
    	if(ret!=HD_OK){
    		return ret;
    	}
        if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &video_out_path[cDevID])) != HD_OK){
            return ret;
        }

    } else {
        DBG_ERR("not sup\r\n");
        return E_NOSPT;
    }

    if(ret==0) {
	    g_DispDev[_DD(DevID)] = NewDevObj;
    }
    return ret;
}

/*
close
 assert(under unlock)
 -> lock
 -> get old size
 -> set device ENABLE = FALSE
 ---> close device
*/
INT32 GxVideo_CloseDevice(UINT32 DevID)
{
    HD_RESULT ret=0;
	UINT32 cDevID = _DD(DevID);

#if (DEVICE_COUNT < 2)
	if (cDevID >= 1) {
		DBG_ERR("DevID%d no support\r\n", cDevID);
		return HD_ERR_NOT_SUPPORT;
	}
#endif

    DBG_FUNC_BEGIN("\r\n");

    ret = hd_videoout_stop(video_out_path[cDevID]);
    if ((ret!=HD_OK) && (ret!=HD_ERR_NOT_START)) {
        DBG_ERR("hd_videoout_stop failed, ret=%d\r\n", ret);
        return ret;
    }

    ret = hd_videoout_close(video_out_path[cDevID]);
    if ((ret!=HD_OK) && (ret!=HD_ERR_NOT_START)) {
        DBG_ERR("hd_videoout_close failed, ret=%d\r\n", ret);
        return ret;
    }
    ret = hd_videoout_uninit();

	ret = Dx_Close((DX_HANDLE)g_DispDev[_DD(DevID)]);
	if (DX_OK != ret && DX_NOT_OPEN != ret) {
		DBG_ERR("Dx_Close fail, err: %d\r\n", ret);
	}

    if(ret == 0) {
	    g_DispDev[_DD(DevID)] = 0;
    }

    return ret;
}

UINT32	GxVideo_GetDevice(UINT32 DevID)
{
	DBG_FUNC_BEGIN("\r\n");
    return 0;
}

ISIZE GxVideo_GetDeviceSize(UINT32 DevID)
{
	ISIZE sz ={0};
	HD_RESULT ret = HD_OK;
    HD_VIDEOOUT_SYSCAPS video_out_syscaps;
	UINT32 cDevID = _DD(DevID);

#if (DEVICE_COUNT < 2)
	if (cDevID >= 1) {
		DBG_ERR("DevID%d no support\r\n", cDevID);
		return sz;
	}
#endif

	DBG_FUNC_BEGIN("\r\n");

	ret = hd_videoout_get(video_out_ctrl[cDevID], HD_VIDEOOUT_PARAM_SYSCAPS, &video_out_syscaps);
	if (ret != HD_OK) {
		DBG_ERR("get vout cap fail\r\n");
        return sz;
	}

	if(g_DispRotate[cDevID]) {
        sz.w = video_out_syscaps.output_dim.h;
        sz.h = video_out_syscaps.output_dim.w;
	} else {
        sz.w = video_out_syscaps.output_dim.w;
        sz.h = video_out_syscaps.output_dim.h;
	}

	return sz;
}

USIZE GxVideo_GetDeviceAspect(UINT32 DevID)
{
	USIZE sz = {0}, sz2 = {0};
	UINT32 cDevID;

	DBG_FUNC_BEGIN("\r\n");

	cDevID = _DD(DevID);
	Dx_Getcaps((DX_HANDLE)g_DispDev[cDevID], DISPLAY_CAPS_ASPECT, (UINT32)&sz); //query aspect
	if(g_DispRotate[cDevID]) {
		sz2.w = sz.h;
		sz2.h = sz.w;
	} else {
		sz2.w = sz.w;
		sz2.h = sz.h;
	}
	DBG_IND("devasp=(%d,%d)\r\n",sz2.w,sz2.h);
	return sz2;

}

UINT32 GxVideo_QueryDeviceLastMode(UINT32 NewDevObj)
{
	UINT32 mode = 0; //fix for CID 44482

	DBG_FUNC_BEGIN("\r\n");

	if (Dx_GetState((DX_HANDLE)NewDevObj, DRVDISP_STATE_LASTMODE, &mode) != DX_OK) { //query mode
		DBG_ERR("- query fail!\r\n");
		return mode;
	}
	return mode;
}
void GxVideo_ConfigDeviceFirstMode(UINT32 NewDevObj, UINT32 mode)
{
	DBG_FUNC_BEGIN("\r\n");

	Dx_Setconfig((DX_HANDLE)NewDevObj, DISPLAY_CFG_MODE, mode);
}
ISIZE GxVideo_QueryDeviceModeSize(UINT32 DevID, UINT32 NewDevObj, UINT32 mode)
{
	return GxVideo_GetDeviceSize(DevID);
}

/*
wakeup
 assert(under lock)
 -> set DRVDISP_CTRL_SLEEP = 1 //device off
 ---> open ide
 -> set DRVDISP_CTRL_SLEEP = 0
 ---> open device by current mode
 -> unlock
sleep
 assert(under unlock)
 -> lock
 -> set DRVDISP_CTRL_SLEEP = 1 //device off
 ---> close device
 -> set DRVDISP_CTRL_SLEEP = 2 //device off & ide off
 ---> close ide
mode
 -> close
 -> set mode
 -> open
*/
void GxVideo_SetDeviceCtrl(UINT32 DevID, UINT32 data, UINT32 value)
{
	UINT32 cDevID;
	UINT32 uiDxState = 0;
	cDevID = _DD(DevID);

    DBG_IND("DevID %x data %d value %d\r\n",DevID,data,value);

	if (g_DispDev[cDevID] == 0) {
		switch (data) {
		case DISPLAY_DEVCTRL_SWAPXY:
			g_DispRotate[cDevID] = value;
			break;
		default:
			DBG_WRN("skipped! devctrl=0x%02X (locked)\r\n", data);
			break;
		}
		return;
	}

	switch (data) {
	case DISPLAY_DEVCTRL_SWAPXY:
		g_DispRotate[cDevID] = value;
		if(g_DispRotate[cDevID]) {
			DBG_DUMP("GxVideo: DOUT%d enable rotate!\r\n", cDevID+1);
		}
		break;
	case DISPLAY_DEVCTRL_SLEEP: {
			UINT32 sleep = value;
			DBG_IND("GxVideo_SetDeviceCtrl SLEEP=%08x\r\n", sleep);

			if (Dx_GetState((DX_HANDLE)(g_DispDev[cDevID]), DRVDISP_STATE_SLEEP, &uiDxState) != DX_OK) {
				return;
			}
			if (uiDxState != sleep) {
				Dx_Control((DX_HANDLE)(g_DispDev[cDevID]), DRVDISP_CTRL_SLEEP, sleep, 0); //set sleep
			}
		}
		break;
	case DISPLAY_DEVCTRL_BACKLIGHT: {
			UINT32 state = value;
			if (Dx_Getcaps((DX_HANDLE)(g_DispDev[cDevID]), DISPLAY_CAPS_BASE, 0) & DISPLAY_BF_BACKLIGHT) {
				DBG_IND("GxVideo_SetDeviceCtrl LIGHT=%08x\r\n", state);

				if (Dx_GetState((DX_HANDLE)(g_DispDev[cDevID]), DRVDISP_STATE_BACKLIGHT, &uiDxState) != DX_OK) {
					return;
				}
				if (uiDxState != state) {
					Dx_SetState((DX_HANDLE)(g_DispDev[cDevID]), DRVDISP_STATE_BACKLIGHT, state);
				}
			}
		}
		break;
	case DISPLAY_DEVCTRL_BRIGHTLVL: {
			UINT32 bright = value;
			if (Dx_Getcaps((DX_HANDLE)(g_DispDev[cDevID]), DISPLAY_CAPS_BASE, 0) & DISPLAY_BF_BACKLIGHT) {
				DBG_IND("GxVideo_SetDeviceCtrl LEVEL=%08x\r\n", bright);

				if (Dx_GetState((DX_HANDLE)(g_DispDev[cDevID]), DRVDISP_STATE_BRIGHTLVL, &uiDxState) != DX_OK) {
					return;
				}
				if (uiDxState != bright) {
					Dx_SetState((DX_HANDLE)(g_DispDev[cDevID]), DRVDISP_STATE_BRIGHTLVL, bright);
				}
			}
		}
		break;
	case DISPLAY_DEVCTRL_DIRECT: {
			UINT32 dir = value;
			Dx_SetState((DX_HANDLE)(g_DispDev[cDevID]), DRVDISP_STATE_DIRECT, dir);
		}
		break;
	case DISPLAY_DEVCTRL_FORCEDETINS:
		bForceDetIns = (value != 0);
		break;
	case DISPLAY_DEVCTRL_FORCEDETDIR:
		bForceDetDir = (value != 0);
		break;
	case DISPLAY_DEVCTRL_ACTIVE:
		DBG_IND("GxVideo_SetDeviceCtrl new ACTIVE=%08x\r\n", value);
		DBG_IND("GxVideo_SetDeviceCtrl cur ACTIVE=%08x\r\n", g_DispState[cDevID]);
		if (value == DISPLAY_ACTIVE_STOP) {
			if (g_DispState[cDevID] == DISPLAY_ACTIVE_PLAY) {
				g_DispState[cDevID] = value;
				DBG_IND("new ACTIVE=%08x ok\r\n", value);
			} else if (g_DispState[cDevID] == DISPLAY_ACTIVE_PAUSE) {
				g_DispState[cDevID] = value;
				DBG_IND("new ACTIVE=%08x ok\r\n", value);
			} else {
				DBG_IND("ignore\r\n");
			}
		}
		if (value == DISPLAY_ACTIVE_PLAY) {
			if (g_DispState[cDevID] == DISPLAY_ACTIVE_PAUSE) {
				g_DispState[cDevID] = value;
				DBG_IND("new ACTIVE=%08x ok\r\n", value);
			} else {
				DBG_IND("ignore\r\n");
			}
		}
		if (value == DISPLAY_ACTIVE_PAUSE) {
			if (g_DispState[cDevID] == DISPLAY_ACTIVE_PLAY) {
				g_DispState[cDevID] = value;
				DBG_IND("new ACTIVE=%08x ok\r\n", value);
			} else {
				DBG_IND("ignore\r\n");
			}
		}
		break;
	default:
		DBG_ERR("devctrl=0x%02X not support!\r\n", data);
		break;
	}
}

UINT32 GxVideo_GetDeviceCtrl(UINT32 DevID, UINT32 data)
{
	UINT32 getv = 0;
	UINT32 cDevID;

    DBG_IND("DevID %x data %d \r\n",DevID,data);

	cDevID = _DD(DevID);

	if (g_DispDev[cDevID] == 0) {
		DBG_WRN("skipped! devctrl=0x%02X (locked)\r\n", data);
		return 0;
	}

	switch (data) {
	case DISPLAY_DEVCTRL_PATH:
		getv = video_out_path[cDevID];
		break;
	case DISPLAY_DEVCTRL_CTRLPATH:
		getv = video_out_ctrl[cDevID];
		break;
	case DISPLAY_DEVCTRL_SWAPXY:
		getv = g_DispRotate[cDevID];
		break;
	case DISPLAY_DEVCTRL_MODE:
		//fix for CID 42915 - begin
		if (Dx_GetState((DX_HANDLE)(g_DispDev[cDevID]), DRVDISP_STATE_MODE, &getv) != DX_OK) {
			return 0;
		}
		//fix for CID 42915 - end
		break;
	case DISPLAY_DEVCTRL_SLEEP:
		if (Dx_GetState((DX_HANDLE)(g_DispDev[cDevID]), DRVDISP_STATE_SLEEP, &getv) != DX_OK) {
			return 0;
		}
		break;
	case DISPLAY_DEVCTRL_BACKLIGHT:
		if (Dx_Getcaps((DX_HANDLE)(g_DispDev[cDevID]), DISPLAY_CAPS_BASE, 0) & DISPLAY_BF_BACKLIGHT) {
			if (Dx_GetState((DX_HANDLE)(g_DispDev[cDevID]), DRVDISP_STATE_BACKLIGHT, &getv) != DX_OK) {
				return 0;
			}
		}
		break;
	case DISPLAY_DEVCTRL_BRIGHTLVL:
		if (Dx_Getcaps((DX_HANDLE)(g_DispDev[cDevID]), DISPLAY_CAPS_BASE, 0) & DISPLAY_BF_BACKLIGHT) {
			if (Dx_GetState((DX_HANDLE)(g_DispDev[cDevID]), DRVDISP_STATE_BRIGHTLVL, &getv) != DX_OK) {
				return 0;
			}
		}
		break;
	case DISPLAY_DEVCTRL_DIRECT:
			if (Dx_Getcaps((DX_HANDLE)g_DispDev[cDevID], DISPLAY_CAPS_BASE, 0) & DISPLAY_BF_DETDIR) {
				getv = Dx_Getcaps((DX_HANDLE)g_DispDev[cDevID], DISPLAY_CAPS_DIRECT, 0);	//detect current plug status
			}
		break;
	case DISPLAY_DEVCTRL_FORCEDETINS:
		getv = bForceDetIns;
		break;
	case DISPLAY_DEVCTRL_FORCEDETDIR:
		getv = bForceDetDir;
		break;
	case DISPLAY_DEVCTRL_ACTIVE:
		getv = g_DispState[cDevID];
		break;
	default:
		DBG_ERR("devctrl=0x%02X not support!\r\n", data);
		break;
	}
	return getv;
}

static BOOL bLastDet[3] = {FALSE, FALSE, FALSE};
static BOOL bLastPlug[3] = {FALSE, FALSE, FALSE};

/**
  Detect video is plugging in or unplugged

  Detect video is plugging in or unplugged.

  @param void
  @return void
*/
void GxVideo_DetInsert(UINT32 DevID, UINT32 context)
{

	BOOL bCurDet, bCurPlug;
	bCurDet = FALSE;

	if (Dx_Getcaps((DX_HANDLE)DevID, DISPLAY_CAPS_BASE, 0) & DISPLAY_BF_DETPLUG) {
		bCurDet = Dx_Getcaps((DX_HANDLE)DevID, DISPLAY_CAPS_PLUG, 0);	 //detect current plug status
	}

	bCurPlug = (BOOL)(bCurDet & bLastDet[context]); //if det is TRUE twice => plug is TRUE

	if ((bCurPlug != bLastPlug[context]) || (bForceDetIns)) { //if plug is changed
		if (bForceDetIns) {
			DBG_IND("force detect ins\r\n");
		}
		// video is plugging in
		if (bCurPlug == TRUE) {
			DBG_IND("Video plug!\r\n");
			if (g_fpDispCB) {
				g_fpDispCB(DISPLAY_CB_PLUG, DevID, 0);
			}
		}
		// video is unplugged
		else {
			DBG_IND("Video unplug!\r\n");
			if (g_fpDispCB) {
				g_fpDispCB(DISPLAY_CB_UNPLUG, DevID, 0);
			}
		}
		bForceDetIns = 0;
	}

	bLastDet[context] = bCurDet; //save det
	bLastPlug[context] = bCurPlug;//save plug
}

static UINT32 bLastDetDir = 0xffffffff;
/**
  Detect video is plugging in or unplugged

  Detect video is plugging in or unplugged.

  @param void
  @return void
*/
void GxVideo_DetDir(UINT32 DevID, UINT32 context)
{
	UINT32 bCurDetDir;
	bCurDetDir = 0x0;

	if (Dx_Getcaps((DX_HANDLE)DevID, DISPLAY_CAPS_BASE, 0) & DISPLAY_BF_DETDIR) {
		bCurDetDir = Dx_Getcaps((DX_HANDLE)DevID, DISPLAY_CAPS_DIRECT, 0);	  //detect current plug status
	}

	if ((bCurDetDir != bLastDetDir) || (bForceDetDir)) {
		if (bForceDetDir) {
			DBG_IND("force detect dir\r\n");
		}
		DBG_IND("device change dir!\r\n");
		if (g_fpDispCB) {
			g_fpDispCB(DISPLAY_CB_DIR, DevID, bCurDetDir);
		}
		bForceDetDir = 0;
	}
	bLastDetDir = bCurDetDir;
}

