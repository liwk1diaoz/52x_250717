/*
    Media Video Recording Task.

    This file is the task of media video recorder.

    @file       MediaVideoRecTsk.c
    @ingroup    mIAPPMEDIAREC
    @note       add h264 saving
    @version    V1.02.006
    @date       2011/05/10

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/
#include "comm/timer.h"
#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/platform.h"
#include "kwrap/stdio.h"
#include <kwrap/cpu.h>
#include "sxcmd_wrapper.h"
#include "comm/hwclock.h"
#include "kflow_common/type_vdo.h"
#include "isf_vdoenc_internal.h"
#include "video_codec_h264.h"
#include "video_codec_h265.h"

#include "nmediarec_api.h"
#include "nmediarec_vdoenc_api.h"
#include "nmediarec_vdoenc_tsk.h"
#include "nmediarec_vdotrig_tsk.h"
#include "nmediarec_vdoenc_platform.h"

#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/vdoenc_builtin.h"

#include "jpg_header.h"   // for JPG_FMT_YUV420 define
#include "h265_def.h" // for tile rule
#include "kdrv_videoenc/kdrv_videoenc_lmt.h"
#include "kdrv_videoenc/kdrv_videoenc_jpeg_lmt.h"

//#include <plat-na51055/top.h>   // TODO: linux should include this, but there's no CHIP_NA51084 yet...
#include "rtos_na51055/top.h"     // TODO: include RTOS .h  first

#define Perf_GetCurrent(x)  hwclock_get_counter(x)

#if !defined(__LINUX)
#include "kwrap/cmdsys.h"
#endif

#if defined(__LINUX)
#include <linux/vmalloc.h>
#elif defined(__FREERTOS)
#include <malloc.h>
#endif

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          nmediarec_vdoenc
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int nmediarec_vdoenc_debug_level = NVT_DBG_WRN;
module_param_named(debug_level_nmediarec_vdoenc, nmediarec_vdoenc_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_nmediarec_vdoenc, "nmediarec_vdoenc debug level");
///////////////////////////////////////////////////////////////////////////////


#define NMR_VDOTRIG_TASK_PRIORITY               2
#define NMR_VDOTRIG_STACK_SIZE                  4096  // since video callback to draw something, extend larger

#define NMR_VDOENC_BUF_MIN_MS                   1500  //for liveview
#define NMR_VDOENC_BUF_MAX_MS             		6000  //for car dv
#define NMR_VDOENC_BUF_RESERVED_MS         		1000  //reserved length of enc buf
#define NMR_VDOENC_BUF_RESERVED_BYTES      		4  	  //enc buf leave 4 bytes to avoid md data overwritten by hw codec garbage

#if NMR_VDOENC_DYNAMIC_CONTEXT
NMR_VDOENC_OBJ                                  *gNMRVdoEncObj = NULL;
extern NMR_VDOTRIG_OBJ                          *gNMRVdoTrigObj;
#else
NMR_VDOENC_OBJ                                  gNMRVdoEncObj[NMR_VDOENC_MAX_PATH] = {0};
extern NMR_VDOTRIG_OBJ                          gNMRVdoTrigObj[];
#endif

THREAD_HANDLE                                   NMR_VDOTRIG_D2DTSK_ID_H26X = 0;
THREAD_HANDLE                                   NMR_VDOTRIG_D2DTSK_ID_H26X_SNAPSHOT = 0;
THREAD_HANDLE                                   NMR_VDOTRIG_D2DTSK_ID_JPEG = 0;
#ifdef VDOENC_LL
THREAD_HANDLE                                   NMR_VDOTRIG_D2DTSK_ID_WRAPBS= 0;
#endif
ID                                              FLG_ID_NMR_VDOTRIG_H26X = 0;
ID                                              FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT = 0;
ID                                              FLG_ID_NMR_VDOTRIG_JPEG = 0;
#ifdef VDOENC_LL
ID                                              FLG_ID_NMR_VDOTRIG_WRAPBS = 0;
SEM_HANDLE                                      NMR_VDOENC_TRIG_SEM_ID[NMR_VDOENC_MAX_PATH] = {0};
#endif
//SEM_HANDLE                                      NMR_VDOENC_SEM_ID[NMR_VDOENC_MAX_PATH] = {0};

#ifdef __KERNEL__
SEM_HANDLE                                      NMR_VDOENC_COMMON_SEM_ID = {0};
SEM_HANDLE                                      NMR_VDOCODEC_H26X_SEM_ID = {0};
SEM_HANDLE                                      NMR_VDOCODEC_JPEG_SEM_ID = {0};
#else
SEM_HANDLE                                      NMR_VDOENC_COMMON_SEM_ID = 0;
SEM_HANDLE                                      NMR_VDOCODEC_H26X_SEM_ID = 0;
SEM_HANDLE                                      NMR_VDOCODEC_JPEG_SEM_ID = 0;
#endif

SEM_HANDLE                                      NMR_VDOENC_YUV_SEM_ID[NMR_VDOENC_MAX_PATH] = {0};

UINT8                                           gNMRVdoEncOpened_H26X = FALSE;
UINT8                                           gNMRVdoEncOpened_JPEG = FALSE;
#ifdef VDOENC_LL
UINT8                                           gNMRVdoEncOpened_WrapBS = FALSE;
#endif
UINT32                                          gNMRVdoEncPathCount = 0;
static volatile UINT32                          gNMRVdoEncTimerID[NMR_VDOENC_MAX_PATH] = {0};
static volatile UINT32                          gNMRVdoEncFpsTimerID[NMR_VDOENC_MAX_PATH] = {0};
UINT32                                          gNMRVdoEncFirstPathID = NMR_VDOENC_MAX_PATH;
UINT32                                          gNMRVdoEncJpgYuvFormat[NMR_VDOENC_MAX_PATH] = {VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420, VDO_PXLFMT_YUV420};
#ifdef __KERNEL__
UINT32                                          gNMRVdoEnc1stStart[BUILTIN_VDOENC_PATH_ID_MAX] = {1, 1, 1, 1, 1, 1};
#endif

UINT32 _nmr_vdoenc_query_context_size(void)
{
#if NMR_VDOENC_DYNAMIC_CONTEXT
	return (PATH_MAX_COUNT * sizeof(NMR_VDOENC_OBJ));
#else
	return 0;
#endif
}

void _nmr_vdoenc_assign_context(UINT32 addr)
{
#if NMR_VDOENC_DYNAMIC_CONTEXT
	gNMRVdoEncObj = (NMR_VDOENC_OBJ *)addr;
#endif
	return;
}

void _nmr_vdoenc_free_context(void)
{
#if NMR_VDOENC_DYNAMIC_CONTEXT
	gNMRVdoEncObj = NULL;
#endif
	return;
}

BOOL _nmr_vdoenc_is_init(void)
{
#if NMR_VDOENC_DYNAMIC_CONTEXT
	return (gNMRVdoEncObj)? TRUE:FALSE;
#else
	return TRUE;
#endif
}

static void _NMR_VdoEnc_FpsTimerHdl0(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(0);
}

static void _NMR_VdoEnc_FpsTimerHdl1(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(1);
}

static void _NMR_VdoEnc_FpsTimerHdl2(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(2);
}

static void _NMR_VdoEnc_FpsTimerHdl3(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(3);
}

static void _NMR_VdoEnc_FpsTimerHdl4(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(4);
}

static void _NMR_VdoEnc_FpsTimerHdl5(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(5);
}

static void _NMR_VdoEnc_FpsTimerHdl6(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(6);
}

static void _NMR_VdoEnc_FpsTimerHdl7(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(7);
}

static void _NMR_VdoEnc_FpsTimerHdl8(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(8);
}

static void _NMR_VdoEnc_FpsTimerHdl9(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(9);
}

static void _NMR_VdoEnc_FpsTimerHdl10(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(10);
}

static void _NMR_VdoEnc_FpsTimerHdl11(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(11);
}

static void _NMR_VdoEnc_FpsTimerHdl12(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(12);
}

static void _NMR_VdoEnc_FpsTimerHdl13(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(13);
}

static void _NMR_VdoEnc_FpsTimerHdl14(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(14);
}

static void _NMR_VdoEnc_FpsTimerHdl15(UINT32 nouse)
{
	NMR_VdoEnc_FpsTimerTrigger(15);
}

static ER _NMR_VdoEnc_OpenFpsTimer(UINT32 pathID)
{
	switch (pathID) {
	case 0:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl0) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 1:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl1) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 2:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl2) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 3:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl3) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 4:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl4) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 5:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl5) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 6:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl6) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 7:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl7) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 8:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl8) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 9:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl9) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 10:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl10) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 11:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl11) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 12:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl12) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 13:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl13) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 14:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl14) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 15:
		if (timer_open((TIMER_ID *)&gNMRVdoEncFpsTimerID[pathID], _NMR_VdoEnc_FpsTimerHdl15) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	default:
		DBG_ERR("[VDOENC] set timer invalid path id = %d\r\n", pathID);
		return E_PAR;
	}

	timer_cfg(gNMRVdoEncFpsTimerID[pathID], 1000000, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);

	return E_OK;
}

static void _NMR_VdoEnc_CloseFpsTimer(UINT32 pathID)
{
	if (gNMRVdoEncFpsTimerID[pathID]) {
		timer_close(gNMRVdoEncFpsTimerID[pathID]);
		gNMRVdoEncFpsTimerID[pathID] = 0;
	}
}

static void _NMR_VdoEnc_TimerHdl0(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(0);
}

static void _NMR_VdoEnc_TimerHdl1(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(1);
}

static void _NMR_VdoEnc_TimerHdl2(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(2);
}

static void _NMR_VdoEnc_TimerHdl3(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(3);
}

static void _NMR_VdoEnc_TimerHdl4(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(4);
}

static void _NMR_VdoEnc_TimerHdl5(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(5);
}

static void _NMR_VdoEnc_TimerHdl6(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(6);
}

static void _NMR_VdoEnc_TimerHdl7(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(7);
}

static void _NMR_VdoEnc_TimerHdl8(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(8);
}

static void _NMR_VdoEnc_TimerHdl9(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(9);
}

static void _NMR_VdoEnc_TimerHdl10(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(10);
}

static void _NMR_VdoEnc_TimerHdl11(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(11);
}

static void _NMR_VdoEnc_TimerHdl12(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(12);
}

static void _NMR_VdoEnc_TimerHdl13(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(13);
}

static void _NMR_VdoEnc_TimerHdl14(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(14);
}

static void _NMR_VdoEnc_TimerHdl15(UINT32 nouse)
{
	NMR_VdoEnc_TimerTrigger(15);
}


static ER _NMR_VdoEnc_OpenTimer(UINT32 pathID)
{
	switch (pathID) {
	case 0:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl0) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 1:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl1) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 2:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl2) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 3:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl3) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 4:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl4) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 5:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl5) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 6:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl6) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 7:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl7) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 8:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl8) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 9:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl9) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 10:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl10) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 11:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl11) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 12:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl12) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 13:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl13) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 14:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl14) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	case 15:
		if (timer_open((TIMER_ID *)&gNMRVdoEncTimerID[pathID], _NMR_VdoEnc_TimerHdl15) != E_OK) {
			DBG_ERR("[VDOENC][%d] Video encode trigger timer open failed!\r\n", pathID);
			return E_SYS;
		}
		break;

	default:
		DBG_ERR("[VDOENC] set timer invalid path id = %d\r\n", pathID);
		return E_PAR;
	}

	return E_OK;
}

static void _NMR_VdoEnc_SetEncTriggerTimer(UINT32 pathID)
{
	if (gNMRVdoEncObj[pathID].InitInfo.frame_rate == 0){
		DBG_ERR("[VDOENC][%d] set timer invalid framerate = 0\r\n", pathID);
		return;
	}
	//init timer rate
	gNMRVdoEncObj[pathID].uiTimerRate = gNMRVdoEncObj[pathID].InitInfo.frame_rate/1000;

	if (gNMRVdoEncObj[pathID].uiTimerRate == 0) {
		DBG_ERR("[VDOENC][%d] TimerRate = 0 ? (frame_rate = %u.%03u)\r\n", pathID, gNMRVdoEncObj[pathID].InitInfo.frame_rate/1000, gNMRVdoEncObj[pathID].InitInfo.frame_rate%1000);
		return;
	}

	if (_NMR_VdoEnc_OpenTimer(pathID) != E_OK) {
		DBG_ERR("[VDOENC][%d] open timer failed\r\n", pathID);
		return;
	}

	if ((gNMRVdoEncObj[pathID].uiRecFormat == MEDIAREC_TIMELAPSE) || (gNMRVdoEncObj[pathID].uiTimelapseTime > 0)) {
		timer_cfg(gNMRVdoEncTimerID[pathID], 1000 * gNMRVdoEncObj[pathID].uiTimelapseTime, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
		DBG_IND("[VDOENC][%d] timelapse enc timer = %d us!!!!\r\n", pathID, 1000 * gNMRVdoEncObj[pathID].uiTimelapseTime);
	} else { // normal mode
		//gNMRVdoEncObj[pathID].iTimerInterval = 1000000 / gNMRVdoEncObj[pathID].InitInfo.uiFrameRate;
		gNMRVdoEncObj[pathID].iTimerInterval = 1000000 / gNMRVdoEncObj[pathID].uiTimerRate;
		gNMRVdoEncObj[pathID].uiLastTime64 = 0;
		gNMRVdoEncObj[pathID].uiTimerStatus = NMR_VDOENC_TIMER_STATUS_NORMAL;
		timer_cfg(gNMRVdoEncTimerID[pathID], gNMRVdoEncObj[pathID].iTimerInterval, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
		DBG_IND("[VDOENC][%d] normal enc timer = %d us!!!!\r\n", pathID, 1000000 / gNMRVdoEncObj[pathID].uiTimerRate);
	}
}

static void _NMR_VdoEnc_UpdateEncTriggerTimer(UINT32 pathID, UINT32 tr)
{
	gNMRVdoEncObj[pathID].uiTimerRate = tr;

	gNMRVdoEncObj[pathID].iTimerInterval = 1000000 / gNMRVdoEncObj[pathID].uiTimerRate;
	gNMRVdoEncObj[pathID].uiLastTime64 = 0;
	gNMRVdoEncObj[pathID].uiTimerStatus = NMR_VDOENC_TIMER_STATUS_NORMAL;

	timer_reload(gNMRVdoEncTimerID[pathID], gNMRVdoEncObj[pathID].iTimerInterval);
	DBG_IND("[VDOENC][%d] normal enc timer = %d us!!!!\r\n", pathID, gNMRVdoEncObj[pathID].iTimerInterval);
}

static void _NMR_VdoEnc_UpdateEncTriggerTimer2(UINT32 pathID, UINT32 tr)
{

	gNMRVdoEncObj[pathID].iTimerInterval = tr;
	gNMRVdoEncObj[pathID].uiLastTime64 = 0;
	gNMRVdoEncObj[pathID].uiTimerStatus = NMR_VDOENC_TIMER_STATUS_RESET;
	gNMRVdoEncObj[pathID].uiTimerRate = 1000000 / gNMRVdoEncObj[pathID].iTimerInterval;

	DBG_IND("[VDOENC][%d] change enc timer = %d us!!!!\r\n", pathID, gNMRVdoEncObj[pathID].iTimerInterval);
}

static void _NMR_VdoEnc_CloseTriggerTimer(UINT32 pathID)
{
		//DBG_ERR("VdoEnc.out[%d] -- stop Enc timer\r\n", pathID);
	timer_close(gNMRVdoEncTimerID[pathID]);
	gNMRVdoEncTimerID[pathID] = 0;
}

//#NT#2012/04/06#Hideo Lin -end

static UINT32 _NMR_VdoEnc_Get_H265_GDC_MIN_W_1_TILE(void)
{
	UINT32 chip_id = nvt_get_chip_id();

	switch(chip_id) {
		case CHIP_NA51055:   return H265_GDC_MIN_W_1_TILE;        //520
		case CHIP_NA51084:   return H265_GDC_MIN_W_1_TILE_528;    //528
		default:
			DBG_ERR("nvt_get_chip_id() = 0x%08x ?\r\n", (unsigned int)chip_id);
			return 0;
	}

	return 0;
}

static UINT32 _NMR_VdoEnc_Get_H265_D2D_MIN_W_1_TILE(void)
{
	UINT32 chip_id = nvt_get_chip_id();

	switch(chip_id) {
		case CHIP_NA51055:   return H265_D2D_MIN_W_1_TILE;        //520
		case CHIP_NA51084:   return H265_D2D_MIN_W_1_TILE_528;    //528
		default:
			DBG_ERR("nvt_get_chip_id() = 0x%08x ?\r\n", (unsigned int)chip_id);
			return 0;
	}

	return 0;
}

static INT32 _NMR_VdoEnc_Get_HW_Limit(MP_VDOENC_HW_LMT *p_hw_limit)
{
	UINT32 chip_id = nvt_get_chip_id();

	switch(chip_id) {
		case CHIP_NA51055: //520
			p_hw_limit->h264e.min_width  = H264E_WIDTH_MIN;
			p_hw_limit->h264e.min_height = H264E_HEIGHT_MIN;
			p_hw_limit->h264e.max_width  = H264E_WIDTH_MAX;
			p_hw_limit->h264e.max_height = H264E_HEIGHT_MAX;
			p_hw_limit->h265e.min_width  = H265E_WIDTH_MIN;
			p_hw_limit->h265e.min_height = H265E_HEIGHT_MIN;
			p_hw_limit->h265e.max_width  = H265E_WIDTH_MAX;
			p_hw_limit->h265e.max_height = H265E_HEIGHT_MAX;
			p_hw_limit->jpege.min_width  = 16;
			p_hw_limit->jpege.min_height = 16;
			p_hw_limit->jpege.max_width  = JPEG_W_MAX;
			p_hw_limit->jpege.max_height = JPEG_H_MAX;

			p_hw_limit->h264e_rotate_max_height = H264E_ROTATE_MAX_HEIGHT;
			break;

		case CHIP_NA51084: //528
			p_hw_limit->h264e.min_width  = H264E_WIDTH_MIN_528;
			p_hw_limit->h264e.min_height = H264E_HEIGHT_MIN_528;
			p_hw_limit->h264e.max_width  = H264E_WIDTH_MAX_528;
			p_hw_limit->h264e.max_height = H264E_HEIGHT_MAX_528;
			p_hw_limit->h265e.min_width  = H265E_WIDTH_MIN_528;
			p_hw_limit->h265e.min_height = H265E_HEIGHT_MIN_528;
			p_hw_limit->h265e.max_width  = H265E_WIDTH_MAX_528;
			p_hw_limit->h265e.max_height = H265E_HEIGHT_MAX_528;
			p_hw_limit->jpege.min_width  = 16;
			p_hw_limit->jpege.min_height = 16;
			p_hw_limit->jpege.max_width  = JPEG_W_MAX;
			p_hw_limit->jpege.max_height = JPEG_H_MAX;

			p_hw_limit->h264e_rotate_max_height = H264E_ROTATE_MAX_HEIGHT_528;
			break;

		default:
			DBG_ERR("nvt_get_chip_id() = 0x%08x ?\r\n", (unsigned int)chip_id);
			return (-1);
	}
	return 0;
}

static UINT32 _NMR_VdoEnc_GetCodecSize(UINT32 pathID)
{
	MP_VDOENC_MEM_INFO memInfo = {0};
	UINT32 uiWidth = 0, uiHeight = 0, codecSize = 0;

	if (gNMRVdoEncObj[pathID].InitInfo.width == 0 || gNMRVdoEncObj[pathID].InitInfo.height == 0) {
		DBG_ERR("[VDOENC][%d] Get Video Codec Size error, width = %d, height = %d\r\n", pathID, gNMRVdoEncObj[pathID].InitInfo.width, gNMRVdoEncObj[pathID].InitInfo.height);
		return 0;
	}

	if (gNMRVdoEncObj[pathID].pEncoder == NULL) {
		DBG_ERR("[VDOENC][%d] Get codec size, but codec = %d not support\r\n", pathID, gNMRVdoEncObj[pathID].uiVidCodec);
		return 0;
	}

	uiWidth = ALIGN_CEIL_16(((gNMRVdoEncObj[pathID].InitInfo.rotate == 1) || (gNMRVdoEncObj[pathID].InitInfo.rotate == 2)) ? gNMRVdoEncObj[pathID].InitInfo.height : gNMRVdoEncObj[pathID].InitInfo.width);
	uiHeight = ((gNMRVdoEncObj[pathID].InitInfo.rotate == 1) || (gNMRVdoEncObj[pathID].InitInfo.rotate == 2)) ? gNMRVdoEncObj[pathID].InitInfo.width : gNMRVdoEncObj[pathID].InitInfo.height;

	if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265) {
		memInfo.width = uiWidth;
		memInfo.height = uiHeight;
		memInfo.svc_layer = gNMRVdoEncObj[pathID].InitInfo.e_svc;
		memInfo.ltr_interval = gNMRVdoEncObj[pathID].InitInfo.ltr_interval;
		memInfo.d2d_mode_en = gNMRVdoEncObj[pathID].InitInfo.bD2dEn;
		memInfo.gdc_mode_en = (g_vdoenc_gdc_first)? 1:0;
		memInfo.colmv_en = gNMRVdoEncObj[pathID].InitInfo.colmv_en;
		memInfo.comm_recfrm_en = gNMRVdoEncObj[pathID].InitInfo.comm_recfrm_en;
		memInfo.gdr_i_frm_en = gNMRVdoEncObj[pathID].InitInfo.bGDRIFrm;

		if (memInfo.d2d_mode_en) {
			if (g_vdoenc_gdc_first) {
				memInfo.tile_mode_en = (memInfo.width <  _NMR_VdoEnc_Get_H265_GDC_MIN_W_1_TILE())? 0:1;  // for LowLatency, we MUST tile=1  for width <  1280 for GDC first
			} else {
				memInfo.tile_mode_en = (memInfo.width <= _NMR_VdoEnc_Get_H265_D2D_MIN_W_1_TILE())? 0:1;  // for LowLatency, we MUST tile=1  for width <= 2048 for LL first
			}
		} else{
#if defined(CONFIG_NVT_SMALL_HDAL)
			memInfo.quality_level = KDRV_VDOENC_QUALITY_BASE;
#else
			if (gNMRVdoEncObj[pathID].bQualityBase) {
				memInfo.quality_level = KDRV_VDOENC_QUALITY_BASE;
			} else {
				memInfo.quality_level = KDRV_VDOENC_QUALITY_MAIN;
			}
#endif
			// set info for vdoenc_builtin dump dtsi
			gNMRVdoEncObj[pathID].uiVidFastbootGdc = memInfo.gdc_mode_en;
			gNMRVdoEncObj[pathID].uiVidFastbootQualityLv = memInfo.quality_level;
		}
		if ((MP_H265Enc_getVideoObject((MP_VDOENC_ID)pathID)) && (MP_H265Enc_getVideoObject((MP_VDOENC_ID)pathID)->GetInfo)) {
			(MP_H265Enc_getVideoObject((MP_VDOENC_ID)pathID)->GetInfo)(VDOENC_GET_MEM_SIZE, (UINT32 *)&memInfo, 0, 0);
		}
		codecSize = memInfo.size;
	} else if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264) {
		memInfo.width = uiWidth;
		memInfo.height = uiHeight;
		memInfo.svc_layer = gNMRVdoEncObj[pathID].InitInfo.e_svc;
		memInfo.ltr_interval = gNMRVdoEncObj[pathID].InitInfo.ltr_interval;
		memInfo.tile_mode_en = 0;  // H264 won't reference this parameter.
		memInfo.gdc_mode_en  = 0;  // H264 won't reference this parameter.
		memInfo.d2d_mode_en = gNMRVdoEncObj[pathID].InitInfo.bD2dEn;
		memInfo.colmv_en = gNMRVdoEncObj[pathID].InitInfo.colmv_en;
		memInfo.comm_recfrm_en = gNMRVdoEncObj[pathID].InitInfo.comm_recfrm_en;
		memInfo.gdr_i_frm_en = gNMRVdoEncObj[pathID].InitInfo.bGDRIFrm;

		if ((MP_H264Enc_getVideoObject((MP_VDOENC_ID)pathID)) && (MP_H264Enc_getVideoObject((MP_VDOENC_ID)pathID)->GetInfo)) {
			(MP_H264Enc_getVideoObject((MP_VDOENC_ID)pathID)->GetInfo)(VDOENC_GET_MEM_SIZE, (UINT32 *)&memInfo, 0, 0);
		}
		codecSize = memInfo.size;

		// set info for vdoenc_builtin dump dtsi
		gNMRVdoEncObj[pathID].uiVidFastbootGdc = memInfo.gdc_mode_en;
		gNMRVdoEncObj[pathID].uiVidFastbootQualityLv = 0;
	} else {
		codecSize = 0;
	}

	return codecSize;
}

static UINT32 _NMR_VdoEnc_GetEncBufSize(UINT32 pathID)
{
	UINT32 sizepersec = 0, totalSize = 0;

	if ((gNMRVdoEncObj[pathID].uiRecFormat == MEDIAREC_LIVEVIEW) || (gNMRVdoEncObj[pathID].uiTimelapseTime == 0)) {
		if (gNMRVdoEncObj[pathID].MemInfo.uiEncBufMs == 0)
			gNMRVdoEncObj[pathID].MemInfo.uiEncBufMs = NMR_VDOENC_BUF_MIN_MS;
	} else {
		if (gNMRVdoEncObj[pathID].MemInfo.uiEncBufMs == 0)
			gNMRVdoEncObj[pathID].MemInfo.uiEncBufMs = NMR_VDOENC_BUF_MAX_MS;
	}

	sizepersec = NMR_VdoEnc_GetBytesPerSecond(pathID);
#ifdef __KERNEL__
	totalSize = (UINT32)div_u64((UINT64)sizepersec * (UINT64)gNMRVdoEncObj[pathID].MemInfo.uiEncBufMs, 1000);
	if (pathID < BUILTIN_VDOENC_PATH_ID_MAX) {
		BOOL bBuiltinEn = 0;
		UINT32 builtin_codectype = 0;
		VdoEnc_Builtin_GetParam(pathID, BUILTIN_VDOENC_INIT_PARAM_ENC_EN, &bBuiltinEn);
		VdoEnc_Builtin_GetParam(pathID, BUILTIN_VDOENC_INIT_PARAM_CODEC, &builtin_codectype);
		if (kdrv_builtin_is_fastboot() && bBuiltinEn && gNMRVdoEnc1stStart[pathID] && builtin_codectype == BUILTIN_VDOENC_MJPEG) {
			VdoEnc_Builtin_GetParam(pathID, BUILTIN_VDOENC_INIT_PARAM_JPEG_MAX_MEM_SIZE, &totalSize);
		}
	}
#else
    totalSize = (UINT64)sizepersec * (UINT64)gNMRVdoEncObj[pathID].MemInfo.uiEncBufMs/1000;
#endif
	return totalSize;
}

static UINT32 _NMR_VdoEnc_GetSnapshotSize(UINT32 pathID)
{
	UINT32 snapshotSize = 0;

#if (!NMR_VDOENC_SRCOUT_USE_REF_BUF)
	if (gNMRVdoEncObj[pathID].bAllocSnapshotBuf){
		if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265) {
			snapshotSize = ALIGN_CEIL_64(gNMRVdoEncObj[pathID].InitInfo.width)*ALIGN_CEIL_64(gNMRVdoEncObj[pathID].InitInfo.height)*3/2;
		} else if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264){
			if ((gNMRVdoEncObj[pathID].InitInfo.rotate == 0) || (gNMRVdoEncObj[pathID].InitInfo.rotate == 3) )
				snapshotSize = ALIGN_CEIL_32(gNMRVdoEncObj[pathID].InitInfo.width)*ALIGN_CEIL_16(gNMRVdoEncObj[pathID].InitInfo.height)*3/2;
			else
				snapshotSize = ALIGN_CEIL_32(gNMRVdoEncObj[pathID].InitInfo.width)*ALIGN_CEIL_32(gNMRVdoEncObj[pathID].InitInfo.height)*3/2;
		}
	}
#endif

	return snapshotSize;
}

static UINT32 _NMR_VdoEnc_GetReconBufSize(UINT32 pathID)
{
	MP_VDOENC_MEM_INFO memInfo = {0};
	UINT32 uiWidth = 0, uiHeight = 0, recFrmSize = 0;

	if (gNMRVdoEncObj[pathID].InitInfo.width == 0 || gNMRVdoEncObj[pathID].InitInfo.height == 0) {
		DBG_ERR("[VDOENC][%d] Get Video Codec Size error, width = %d, height = %d\r\n", pathID, gNMRVdoEncObj[pathID].InitInfo.width, gNMRVdoEncObj[pathID].InitInfo.height);
		return 0;
	}

	if (gNMRVdoEncObj[pathID].pEncoder == NULL) {
		DBG_ERR("[VDOENC][%d] Get codec size, but codec = %d not support\r\n", pathID, gNMRVdoEncObj[pathID].uiVidCodec);
		return 0;
	}

	if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_MJPG) {
		DBG_ERR("[VDOENC][%d] MJPG codec has no reconstruct buffer\r\n", pathID);
		return 0;
	}

	uiWidth = ALIGN_CEIL_16((gNMRVdoEncObj[pathID].InitInfo.rotate != 0) ? gNMRVdoEncObj[pathID].InitInfo.height : gNMRVdoEncObj[pathID].InitInfo.width);
	uiHeight = (gNMRVdoEncObj[pathID].InitInfo.rotate != 0) ? gNMRVdoEncObj[pathID].InitInfo.width : gNMRVdoEncObj[pathID].InitInfo.height;

	memInfo.width = uiWidth;
	memInfo.height = uiHeight;

	if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265) {
		(MP_H265Enc_getVideoObject((MP_VDOENC_ID)pathID)->GetInfo)(VDOENC_GET_RECONFRM_SIZE, (UINT32 *)&memInfo, 0, 0);
		recFrmSize = memInfo.recfrm_size;
	} else if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264) {
		(MP_H264Enc_getVideoObject((MP_VDOENC_ID)pathID)->GetInfo)(VDOENC_GET_RECONFRM_SIZE, (UINT32 *)&memInfo, 0, 0);
		recFrmSize = memInfo.recfrm_size;
	} else {
		recFrmSize = 0;
	}

	return recFrmSize;
}

static UINT32 _NMR_VdoEnc_GetReconBufNum(UINT32 pathID)
{
	MP_VDOENC_MEM_INFO memInfo = {0};
	UINT32 uiFrmNum = 0;

	if (gNMRVdoEncObj[pathID].InitInfo.width == 0 || gNMRVdoEncObj[pathID].InitInfo.height == 0) {
		DBG_ERR("[VDOENC][%d] Get Video Codec Size error, width = %d, height = %d\r\n", pathID, gNMRVdoEncObj[pathID].InitInfo.width, gNMRVdoEncObj[pathID].InitInfo.height);
		return 0;
	}

	if (gNMRVdoEncObj[pathID].pEncoder == NULL) {
		DBG_ERR("[VDOENC][%d] Get codec size, but codec = %d not support\r\n", pathID, gNMRVdoEncObj[pathID].uiVidCodec);
		return 0;
	}

	memInfo.svc_layer = gNMRVdoEncObj[pathID].InitInfo.e_svc;
	memInfo.ltr_interval = gNMRVdoEncObj[pathID].InitInfo.ltr_interval;

	if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265) {
		(MP_H265Enc_getVideoObject((MP_VDOENC_ID)pathID)->GetInfo)(VDOENC_GET_RECONFRM_NUM, (UINT32 *)&memInfo, 0, 0);
		uiFrmNum = memInfo.recfrm_num;
	} else if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264) {
		(MP_H264Enc_getVideoObject((MP_VDOENC_ID)pathID)->GetInfo)(VDOENC_GET_RECONFRM_NUM, (UINT32 *)&memInfo, 0, 0);
		uiFrmNum = memInfo.recfrm_num;
	} else {
		uiFrmNum = 0;
	}

	return uiFrmNum;
}


/*
    Set memory for saving video bitstream.

    Set memory for saving video bitstream.

    @param[in] addr, memory to save video bitstream
    @param[in] size, size of usage memory
    @return
        -b E_OK   valid memory setting
        -b E_SYS  not enough memory size
*/
static ER _NMR_VdoEnc_SetSaveBuf(UINT32 pathID, UINT32 addr, UINT32 size)
{
	UINT32 sizePerSec, snapshotSize, codecSize, encBufSize;

	//if (gNMRVdoEncObj[pathID].MemInfo.uiEncBufReservedMs == 0) {
	//	gNMRVdoEncObj[pathID].MemInfo.uiEncBufReservedMs = NMR_VDOENC_BUF_RESERVED_MS;
	//}

	if (gNMRVdoEncObj[pathID].bCommSrcOut) {
		snapshotSize = 0;
	} else {
		snapshotSize = _NMR_VdoEnc_GetSnapshotSize(pathID);
	}

	// using for dump fastboot dtsi
	gNMRVdoEncObj[pathID].uiVidFastbootSnapshotSize = snapshotSize;

	codecSize = _NMR_VdoEnc_GetCodecSize(pathID);
	gNMRVdoEncObj[pathID].MemInfo.uiVCodecAddr = addr;
	gNMRVdoEncObj[pathID].MemInfo.uiVCodecSize = codecSize;
	addr += codecSize;
	size -= (codecSize + NMR_VDOENC_MD_MAP_MAX_SIZE + snapshotSize);

	sizePerSec = NMR_VdoEnc_GetBytesPerSecond(pathID);
#if 0 // no need to check if bs_buffer >= 1 sec. For example, user may set enc_buf_ms = 0.11 sec , and min_i_ratio = 0.036 sec
	if (sizePerSec >= size){
		DBG_ERR("[VDOENC][%d] enc buf size(%d) <= byterate(%d)\r\n", pathID, size, sizePerSec);
		return E_PAR;
	}
#endif
	if (addr % 4 != 0){
		DBG_ERR("[VDOENC][%d] enc buf addr(0x%08x) need to be word aligned\r\n", pathID, addr);
		return E_PAR;
	}

	if (gNMRVdoEncObj[pathID].MemInfo.uiEncBufReservedMs > gNMRVdoEncObj[pathID].MemInfo.uiEncBufMs){
		DBG_ERR("[VDOENC][%d] enc buf length (%d ms) is too small\r\n", pathID, gNMRVdoEncObj[pathID].MemInfo.uiEncBufMs);
		return E_PAR;
	}

	gNMRVdoEncObj[pathID].MemInfo.uiBSStart = addr;
	gNMRVdoEncObj[pathID].MemInfo.uiBSEnd = addr + size;
	if (gNMRVdoEncObj[pathID].MemInfo.uiEncBufReservedMs < gNMRVdoEncObj[pathID].MemInfo.uiEncBufMs) {
#ifdef __KERNEL__
		gNMRVdoEncObj[pathID].MemInfo.uiBSMax = addr + size - (UINT32)div_u64((UINT64)sizePerSec * (UINT64)gNMRVdoEncObj[pathID].MemInfo.uiEncBufReservedMs, 1000);
#else
		gNMRVdoEncObj[pathID].MemInfo.uiBSMax = addr + size - ((UINT64)sizePerSec * (UINT64)gNMRVdoEncObj[pathID].MemInfo.uiEncBufReservedMs/1000);
#endif
	} else {
		gNMRVdoEncObj[pathID].MemInfo.uiBSMax = gNMRVdoEncObj[pathID].MemInfo.uiBSStart;
	}

#if defined(_BSP_NA51000_)
	gNMRVdoEncObj[pathID].MdInfo.p_md_bitmap = (UINT8 *) gNMRVdoEncObj[pathID].MemInfo.uiBSEnd;
	memset((void *)gNMRVdoEncObj[pathID].MdInfo.p_md_bitmap, 0, NMR_VDOENC_MD_MAP_MAX_SIZE);
#endif
	if (gNMRVdoEncObj[pathID].MemInfo.uiMinIRatio == 0) {
		gNMRVdoEncObj[pathID].MemInfo.uiMinIRatio = NMR_VDOENC_MIN_I_RATIO;
	}

	if (gNMRVdoEncObj[pathID].MemInfo.uiMinPRatio == 0) {
		gNMRVdoEncObj[pathID].MemInfo.uiMinPRatio = NMR_VDOENC_MIN_P_RATIO;
	}

	gNMRVdoEncObj[pathID].MemInfo.uiBSEnd -= NMR_VDOENC_BUF_RESERVED_BYTES;

#ifdef __KERNEL__
	if (kdrv_builtin_is_fastboot() && pathID < BUILTIN_VDOENC_PATH_ID_MAX && gNMRVdoEnc1stStart[pathID]) {
		BOOL bBuiltinEn = 0;
		UINT32 builtin_codectype = 0;
		UINT32 builtin_bs_start = 0, builtin_bs_end = 0;
		VdoEnc_Builtin_GetParam(pathID, BUILTIN_VDOENC_INIT_PARAM_ENC_EN, &bBuiltinEn);
		VdoEnc_Builtin_GetParam(pathID, BUILTIN_VDOENC_INIT_PARAM_CODEC, &builtin_codectype);
		if (kdrv_builtin_is_fastboot() && bBuiltinEn && builtin_codectype != BUILTIN_VDOENC_MJPEG) {
			VdoEnc_Builtin_GetParam(pathID, BUILTIN_VDOENC_INIT_PARAM_BS_START, &builtin_bs_start);
			VdoEnc_Builtin_GetParam(pathID, BUILTIN_VDOENC_INIT_PARAM_BS_END, &builtin_bs_end);
			if (builtin_bs_start != gNMRVdoEncObj[pathID].MemInfo.uiBSStart) {
				DBG_ERR("[VDOENC][%d] builtin bs start(0x%08x) differ to hdal bs start(0x%08x)\r\n", pathID, builtin_bs_start, gNMRVdoEncObj[pathID].MemInfo.uiBSStart);
				return E_SYS;
			}
			if (builtin_bs_end != gNMRVdoEncObj[pathID].MemInfo.uiBSEnd) {
				DBG_ERR("[VDOENC][%d] builtin bs end(0x%08x) differ to hdal bs end(0x%08x)\r\n", pathID, builtin_bs_end, gNMRVdoEncObj[pathID].MemInfo.uiBSEnd);
				return E_SYS;
			}
		}
	}
#endif

	if (gNMRVdoEncObj[pathID].MemInfo.uiBSMax > gNMRVdoEncObj[pathID].MemInfo.uiBSEnd) {
		gNMRVdoEncObj[pathID].MemInfo.uiBSMax = gNMRVdoEncObj[pathID].MemInfo.uiBSEnd;
	}

	encBufSize = gNMRVdoEncObj[pathID].MemInfo.uiBSEnd - gNMRVdoEncObj[pathID].MemInfo.uiBSStart;
#ifdef __KERNEL__
	gNMRVdoEncObj[pathID].MemInfo.uiMinISize = (UINT32)div_u64((UINT64)sizePerSec * (UINT64)gNMRVdoEncObj[pathID].MemInfo.uiMinIRatio, 100);
	gNMRVdoEncObj[pathID].MemInfo.uiMinPSize = (UINT32)div_u64((UINT64)sizePerSec * (UINT64)gNMRVdoEncObj[pathID].MemInfo.uiMinPRatio, 100);
#else
	gNMRVdoEncObj[pathID].MemInfo.uiMinISize = ((UINT64)sizePerSec * (UINT64)gNMRVdoEncObj[pathID].MemInfo.uiMinIRatio/100);
	gNMRVdoEncObj[pathID].MemInfo.uiMinPSize = ((UINT64)sizePerSec * (UINT64)gNMRVdoEncObj[pathID].MemInfo.uiMinPRatio/100);
#endif
	if (gNMRVdoEncObj[pathID].MemInfo.uiMinISize > encBufSize) {
		gNMRVdoEncObj[pathID].MemInfo.uiMinISize = encBufSize;
	}
	if (gNMRVdoEncObj[pathID].MemInfo.uiMinPSize > encBufSize) {
		gNMRVdoEncObj[pathID].MemInfo.uiMinPSize = encBufSize;
	}

	if (!gNMRVdoEncObj[pathID].bCommSrcOut) {
		addr += (encBufSize + NMR_VDOENC_BUF_RESERVED_BYTES);
		size = snapshotSize;
		gNMRVdoEncObj[pathID].MemInfo.uiSnapshotAddr = addr;
		gNMRVdoEncObj[pathID].MemInfo.uiSnapshotSize = size;
	}

	return E_OK;
}

static ER _NMR_VdoEnc_SetReconFrmBuf(UINT32 pathID, UINT32 addr, UINT32 size, UINT32 idx)
{
	gNMRVdoEncObj[pathID].MemInfo.uiRecBuffAddr[idx] = addr;
	gNMRVdoEncObj[pathID].MemInfo.uiRecBuffSize      = size;
	gNMRVdoEncObj[pathID].MemInfo.uiRecBuffNum       = _NMR_VdoEnc_GetReconBufNum(pathID);

	return E_OK;
}

static ER _NMR_VdoEnc_SetSnapShotBuf(UINT32 pathID, UINT32 addr, UINT32 size)
{
	gNMRVdoEncObj[pathID].MemInfo.uiSnapshotAddr = addr;
	gNMRVdoEncObj[pathID].MemInfo.uiSnapshotSize = size;
	return E_OK;
}

static ER _NMR_VdoEnc_Open_H26X(void)
{
	ER r = E_OK;

	if (gNMRVdoEncOpened_H26X) {
		DBG_WRN("H26X already opened...\r\n");
		return E_OK;
	}
	r = NMR_VdoTrig_Start_H26X();
	gNMRVdoEncOpened_H26X = TRUE;
	return r;
}

static ER _NMR_VdoEnc_Open_JPEG(void)
{
	ER r = E_OK;

	if (gNMRVdoEncOpened_JPEG) {
		DBG_WRN("JPEG already opened...\r\n");
		return E_OK;
	}
	r = NMR_VdoTrig_Start_JPEG();
	gNMRVdoEncOpened_JPEG = TRUE;
	return r;
}

#ifdef VDOENC_LL
static ER _NMR_VdoEnc_Open_Wrap(void)
{
	ER r = E_OK;

	if (gNMRVdoEncOpened_WrapBS) {
		DBG_WRN("Wrap BS already opened...\r\n");
		return E_OK;
	}
	r = NMR_VdoTrig_Start_WrapBS();
	gNMRVdoEncOpened_WrapBS = TRUE;
	return r;

}
#endif

static ER _NMR_VdoEnc_Close_H26X(void)
{
	if (gNMRVdoEncOpened_H26X == FALSE) {
		DBG_WRN("H26X already closed...\r\n");
		return E_OK;
	}

#if MP_VDOENC_SHOW_MSG
	DBG_IND("[VDOENC] close h26x enc task\r\n");
#endif


	NMR_VdoTrig_Close_H26X();
	gNMRVdoEncOpened_H26X = FALSE;

	return E_OK;
}

static ER _NMR_VdoEnc_Close_JPEG(void)
{
	if (gNMRVdoEncOpened_JPEG == FALSE) {
		DBG_WRN("JPEG already closed...\r\n");
		return E_OK;
	}

#if MP_VDOENC_SHOW_MSG
	DBG_IND("[VDOENC] close jpeg enc task\r\n");
#endif

	NMR_VdoTrig_Close_JPEG();
	gNMRVdoEncOpened_JPEG = FALSE;

	return E_OK;
}

#ifdef VDOENC_LL
static ER _NMR_VdoEnc_Close_WrapBS(void)
{
	if (gNMRVdoEncOpened_WrapBS== FALSE) {
		DBG_WRN("Wrap BS already closed...\r\n");
		return E_OK;
	}

#if MP_VDOENC_SHOW_MSG
	DBG_IND("[VDOENC] close Wrap BS task\r\n");
#endif


	NMR_VdoTrig_Close_WrapBS();
	gNMRVdoEncOpened_WrapBS = FALSE;

	return E_OK;
}

void _NMR_VdoEnc_OSD_MASK(UINT32 pathID, UINT32 isDo)
{
	UINT32 osdtime = 0;

	if (isDo) {
		osdtime = Perf_GetCurrent();
		_isf_vdoenc_do_input_mask(&isf_vdoenc, ISF_IN(pathID), (void *) gNMRVdoEncObj[pathID].pYuvSrc, NULL, FALSE);
		_isf_vdoenc_do_input_osd(&isf_vdoenc, ISF_IN(pathID), (void *) gNMRVdoEncObj[pathID].pYuvSrc, NULL, FALSE);
		gNMRVdoEncObj[pathID].uiEncTopTime[NMI_VDOENC_TOP_OSD_INPUT_TIME] = Perf_GetCurrent() - osdtime;
		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_OSD) {
			DBG_DUMP("[VDOTRIG][%d] do_osd %d us\r\n", pathID, gNMRVdoEncObj[pathID].uiEncTopTime[NMI_VDOENC_TOP_OSD_INPUT_TIME]);
		}
	} else {
		osdtime = Perf_GetCurrent();
		_isf_vdoenc_finish_input_mask(&isf_vdoenc, ISF_IN(pathID));
		_isf_vdoenc_finish_input_osd(&isf_vdoenc, ISF_IN(pathID));
		gNMRVdoEncObj[pathID].uiEncTopTime[NMI_VDOENC_TOP_OSD_FINISH_TIME] = Perf_GetCurrent() - osdtime;
		if (gNMRVdoEncObj[pathID].uiMsgLevel & NMR_VDOENC_MSG_OSD) {
			DBG_DUMP("[VDOTRIG][%d] finish_osd %d us\r\n", pathID, Perf_GetCurrent() - gNMRVdoEncObj[pathID].uiEncTopTime[NMI_VDOENC_TOP_OSD_FINISH_TIME]);
		}
	}
}
#endif

#ifdef __KERNEL__
static BOOL _nmr_vdoenc_compare_builtin_rc(UINT32 pathID, BUILTIN_VDOENC_RC rc_mode, VOID *rc_param)
{
	VDOENC_BUILTIN_RC_PARAM builtin_rc_param = {0};

	VdoEnc_Builtin_GetParam(pathID, BUILTIN_VDOENC_INIT_PARAM_RATE_CONTROL, (UINT32 *)&builtin_rc_param);
	if (rc_mode == BUILTIN_VDOENC_RC_CBR) {
		MP_VDOENC_CBR_INFO *cbr_info = (MP_VDOENC_CBR_INFO *)rc_param;

		if (builtin_rc_param.uiEncId != pathID) {
			DBG_IND("[%d] buitlin cbr uiEncId %d != pathID %d", pathID, builtin_rc_param.uiEncId, pathID);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiRCMode != rc_mode) {
			DBG_IND("[%d] buitlin cbr uiRCMode %d != rc_mode %d", pathID, builtin_rc_param.uiRCMode, rc_mode);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiInitIQp != cbr_info->init_i_qp) {
			DBG_IND("[%d] buitlin cbr uiInitIQp %d != init_i_qp %d", pathID, builtin_rc_param.uiInitIQp, cbr_info->init_i_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMinIQp != cbr_info->min_i_qp) {
			DBG_IND("[%d] buitlin cbr uiMinIQp %d != min_i_qp %d", pathID, builtin_rc_param.uiMinIQp, cbr_info->min_i_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMaxIQp != cbr_info->max_i_qp) {
			DBG_IND("[%d] buitlin cbr uiMaxIQp %d != max_i_qp %d", pathID, builtin_rc_param.uiMaxIQp, cbr_info->max_i_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiInitPQp != cbr_info->init_p_qp) {
			DBG_IND("[%d] buitlin cbr uiInitPQp %d != init_p_qp %d", pathID, builtin_rc_param.uiInitPQp, cbr_info->init_p_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMinPQp != cbr_info->min_p_qp) {
			DBG_IND("[%d] buitlin cbr uiMinPQp %d != min_p_qp %d", pathID, builtin_rc_param.uiMinPQp, cbr_info->min_p_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMaxPQp != cbr_info->max_p_qp) {
			DBG_IND("[%d] buitlin cbr uiMaxPQp %d != max_p_qp %d", pathID, builtin_rc_param.uiMaxPQp, cbr_info->max_p_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiBitRate != (cbr_info->byte_rate*8)) {
			DBG_IND("[%d] buitlin cbr uiBitRate %d != bit_rate %d", pathID, builtin_rc_param.uiBitRate, cbr_info->byte_rate*8);
			goto DIFF_RC;
		}
		if ((builtin_rc_param.uiFrameRateBase) != cbr_info->frame_rate) {
			DBG_IND("[%d] buitlin cbr uiFrameRateBase %d != frame_rate %d", pathID, builtin_rc_param.uiFrameRateBase, cbr_info->frame_rate);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiGOP != cbr_info->gop) {
			DBG_IND("[%d] buitlin cbr uiGOP %d != gop %d", pathID, builtin_rc_param.uiGOP, cbr_info->gop);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiStaticTime != cbr_info->static_time) {
			DBG_IND("[%d] buitlin cbr uiStaticTime %d != static_time %d", pathID, builtin_rc_param.uiStaticTime, cbr_info->static_time);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iIPWeight != cbr_info->ip_weight) {
			DBG_IND("[%d] buitlin cbr iIPWeight %d != ip_weight %d", pathID, builtin_rc_param.iIPWeight, cbr_info->ip_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiKeyPPeriod != cbr_info->key_p_period) {
			DBG_IND("[%d] buitlin cbr uiKeyPPeriod %d != key_p_period %d", pathID, builtin_rc_param.uiKeyPPeriod, cbr_info->key_p_period);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iKPWeight != cbr_info->kp_weight) {
			DBG_IND("[%d] buitlin cbr iKPWeight %d != kp_weight %d", pathID, builtin_rc_param.iKPWeight, cbr_info->kp_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iP2Weight != cbr_info->p2_weight) {
			DBG_IND("[%d] buitlin cbr iP2Weight %d != p2_weight %d", pathID, builtin_rc_param.iP2Weight, cbr_info->p2_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iP3Weight != cbr_info->p3_weight) {
			DBG_IND("[%d] buitlin cbr iP3Weight %d != p3_weight %d", pathID, builtin_rc_param.iP3Weight, cbr_info->p3_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iLTWeight != cbr_info->lt_weight) {
			DBG_IND("[%d] buitlin cbr iLTWeight %d != lt_weight %d", pathID, builtin_rc_param.iLTWeight, cbr_info->lt_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iMotionAQStrength != cbr_info->motion_aq_str) {
			DBG_IND("[%d] buitlin cbr iMotionAQStrength %d != motion_aq_str %d", pathID, builtin_rc_param.iMotionAQStrength, cbr_info->motion_aq_str);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiSvcBAMode != cbr_info->svc_weight_mode) {
			DBG_IND("[%d] buitlin cbr uiSvcBAMode %d != svc_weight_mode %d", pathID, builtin_rc_param.uiSvcBAMode, cbr_info->svc_weight_mode);
			goto DIFF_RC;
		}
	}

	if (rc_mode == BUILTIN_VDOENC_RC_EVBR) {
		MP_VDOENC_EVBR_INFO *evbr_info = (MP_VDOENC_EVBR_INFO *)rc_param;

		if (builtin_rc_param.uiEncId != pathID) {
			DBG_IND("[%d] buitlin evbr uiEncId %d != pathID %d", pathID, builtin_rc_param.uiEncId, pathID);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiRCMode != rc_mode) {
			DBG_IND("[%d] buitlin evbr uiRCMode %d != rc_mode %d", pathID, builtin_rc_param.uiRCMode, rc_mode);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiInitIQp != evbr_info->init_i_qp) {
			DBG_IND("[%d] buitlin evbr uiInitIQp %d != init_i_qp %d", pathID, builtin_rc_param.uiInitIQp, evbr_info->init_i_qp);
			goto DIFF_RC;
		}

		if (builtin_rc_param.uiMinIQp != evbr_info->min_i_qp) {
			DBG_IND("[%d] buitlin evbr uiMinIQp %d != min_i_qp %d", pathID, builtin_rc_param.uiMinIQp, evbr_info->min_i_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMaxIQp != evbr_info->max_i_qp) {
			DBG_IND("[%d] buitlin evbr uiMaxIQp %d != max_i_qp %d", pathID, builtin_rc_param.uiMaxIQp, evbr_info->max_i_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiInitPQp != evbr_info->init_p_qp) {
			DBG_IND("[%d] buitlin evbr uiInitPQp %d != init_p_qp %d", pathID, builtin_rc_param.uiInitPQp, evbr_info->init_p_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMinPQp != evbr_info->min_p_qp) {
			DBG_IND("[%d] buitlin evbr uiMinPQp %d != min_p_qp %d", pathID, builtin_rc_param.uiMinPQp, evbr_info->min_p_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMaxPQp != evbr_info->max_p_qp) {
			DBG_IND("[%d] buitlin evbr uiMaxPQp %d != max_p_qp %d", pathID, builtin_rc_param.uiMaxPQp, evbr_info->max_p_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiBitRate != (evbr_info->byte_rate*8)) {
			DBG_IND("[%d] buitlin evbr uiBitRate %d != bit_rate %d", pathID, builtin_rc_param.uiBitRate, evbr_info->byte_rate*8);
			goto DIFF_RC;
		}
		if ((builtin_rc_param.uiFrameRateBase) != evbr_info->frame_rate) {
			DBG_IND("[%d] buitlin evbr uiFrameRateBase %d != frame_rate %d", pathID, builtin_rc_param.uiFrameRateBase, evbr_info->frame_rate);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiGOP != evbr_info->gop) {
			DBG_IND("[%d] buitlin evbr uiGOP %d != gop %d", pathID, builtin_rc_param.uiGOP, evbr_info->gop);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiStaticTime != evbr_info->static_time) {
			DBG_IND("[%d] buitlin evbr uiStaticTime %d != static_time %d", pathID, builtin_rc_param.uiStaticTime, evbr_info->static_time);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iIPWeight != evbr_info->ip_weight) {
			DBG_IND("[%d] buitlin evbr iIPWeight %d != ip_weight %d", pathID, builtin_rc_param.iIPWeight, evbr_info->ip_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iKPWeight != evbr_info->kp_weight) {
			DBG_IND("[%d] buitlin evbr iKPWeight %d != kp_weight %d", pathID, builtin_rc_param.iKPWeight, evbr_info->kp_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiKeyPPeriod != evbr_info->key_p_period) {
			DBG_IND("[%d] buitlin evbr uiKeyPPeriod %d != key_p_period %d", pathID, builtin_rc_param.uiKeyPPeriod, evbr_info->key_p_period);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iMotionAQStrength != evbr_info->motion_aq_st) {
			DBG_IND("[%d] buitlin evbr iMotionAQStrength %d != motion_aq_st %d", pathID, builtin_rc_param.iMotionAQStrength, evbr_info->motion_aq_st);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiStillFrameCnd != evbr_info->still_frm_cnd) {
			DBG_IND("[%d] buitlin evbr uiStillFrameCnd %d != still_frm_cnd %d", pathID, builtin_rc_param.uiStillFrameCnd, evbr_info->still_frm_cnd);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMotionRatioThd != evbr_info->motion_ratio_thd) {
			DBG_IND("[%d] buitlin evbr uiMotionRatioThd %d != motion_ratio_thd %d", pathID, builtin_rc_param.uiMotionRatioThd, evbr_info->motion_ratio_thd);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiIPsnrCnd != evbr_info->i_psnr_cnd) {
			DBG_IND("[%d] buitlin evbr uiIPsnrCnd %d != i_psnr_cnd %d", pathID, builtin_rc_param.uiIPsnrCnd, evbr_info->i_psnr_cnd);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiPPsnrCnd != evbr_info->p_psnr_cnd) {
			DBG_IND("[%d] buitlin evbr uiPPsnrCnd %d != p_psnr_cnd %d", pathID, builtin_rc_param.uiPPsnrCnd, evbr_info->p_psnr_cnd);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiKeyPPsnrCnd != evbr_info->kp_psnr_cnd) {
			DBG_IND("[%d] buitlin evbr uiKeyPPsnrCnd %d != kp_psnr_cnd %d", pathID, builtin_rc_param.uiKeyPPsnrCnd, evbr_info->kp_psnr_cnd);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMinStillIQp != evbr_info->min_p_qp) {
			DBG_IND("[%d] buitlin evbr uiMinStillIQp %d != min_p_qp %d", pathID, builtin_rc_param.uiMinStillIQp, evbr_info->min_p_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iP2Weight != evbr_info->p2_weight) {
			DBG_IND("[%d] buitlin evbr iP2Weight %d != p2_weight %d", pathID, builtin_rc_param.iP2Weight, evbr_info->p2_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iP3Weight != evbr_info->p3_weight) {
			DBG_IND("[%d] buitlin evbr iP3Weight %d != p3_weight %d", pathID, builtin_rc_param.iP3Weight, evbr_info->p3_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iLTWeight != evbr_info->lt_weight) {
			DBG_IND("[%d] buitlin evbr iLTWeight %d != lt_weight %d", pathID, builtin_rc_param.iLTWeight, evbr_info->lt_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiSvcBAMode != evbr_info->svc_weight_mode) {
			DBG_IND("[%d] buitlin evbr uiSvcBAMode %d != svc_weight_mode %d", pathID, builtin_rc_param.uiSvcBAMode, evbr_info->svc_weight_mode);
			goto DIFF_RC;
		}
	}

	if (rc_mode == BUILTIN_VDOENC_RC_VBR) {
		MP_VDOENC_VBR_INFO *vbr_info = (MP_VDOENC_VBR_INFO *)rc_param;

		if (builtin_rc_param.uiEncId != pathID) {
			DBG_IND("[%d] buitlin vbr uiEncId %d != pathID %d", pathID, builtin_rc_param.uiEncId, pathID);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiRCMode != rc_mode) {
			DBG_IND("[%d] buitlin vbr uiRCMode %d != rc_mode %d", pathID, builtin_rc_param.uiRCMode, rc_mode);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiInitIQp != vbr_info->init_i_qp) {
			DBG_IND("[%d] buitlin vbr uiInitIQp %d != init_i_qp %d", pathID, builtin_rc_param.uiInitIQp, vbr_info->init_i_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMinIQp != vbr_info->min_i_qp) {
			DBG_IND("[%d] buitlin vbr uiMinIQp %d != min_i_qp %d", pathID, builtin_rc_param.uiMinIQp, vbr_info->min_i_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMaxIQp != vbr_info->max_i_qp) {
			DBG_IND("[%d] buitlin vbr uiMaxIQp %d != max_i_qp %d", pathID, builtin_rc_param.uiMaxIQp, vbr_info->max_i_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiInitPQp != vbr_info->init_p_qp) {
			DBG_IND("[%d] buitlin vbr uiInitPQp %d != init_p_qp %d", pathID, builtin_rc_param.uiInitPQp, vbr_info->init_p_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMinPQp != vbr_info->min_p_qp) {
			DBG_IND("[%d] buitlin vbr uiMinPQp %d != min_p_qp %d", pathID, builtin_rc_param.uiMinPQp, vbr_info->min_p_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMaxPQp != vbr_info->max_p_qp) {
			DBG_IND("[%d] buitlin vbr uiMaxPQp %d != max_p_qp %d", pathID, builtin_rc_param.uiMaxPQp, vbr_info->max_p_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiBitRate != (vbr_info->byte_rate*8)) {
			DBG_IND("[%d] buitlin vbr uiBitRate %d != bit_rate %d", pathID, builtin_rc_param.uiBitRate, (vbr_info->byte_rate*8));
			goto DIFF_RC;
		}
		if ((builtin_rc_param.uiFrameRateBase) != vbr_info->frame_rate) {
			DBG_IND("[%d] buitlin vbr uiFrameRateBase %d != frame_rate %d", pathID, builtin_rc_param.uiFrameRateBase, vbr_info->frame_rate);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiGOP != vbr_info->gop) {
			DBG_IND("[%d] buitlin vbr uiGOP %d != gop %d", pathID, builtin_rc_param.uiGOP, vbr_info->gop);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiStaticTime != vbr_info->static_time) {
			DBG_IND("[%d] buitlin vbr uiStaticTime %d != static_time %d", pathID, builtin_rc_param.uiStaticTime, vbr_info->static_time);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iIPWeight != vbr_info->ip_weight) {
			DBG_IND("[%d] buitlin vbr iIPWeight %d != ip_weight %d", pathID, builtin_rc_param.iIPWeight, vbr_info->ip_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiChangePos != vbr_info->change_pos) {
			DBG_IND("[%d] buitlin vbr uiChangePos %d != change_pos %d", pathID, builtin_rc_param.uiChangePos, vbr_info->change_pos);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiKeyPPeriod != vbr_info->key_p_period) {
			DBG_IND("[%d] buitlin vbr uiKeyPPeriod %d != key_p_period %d", pathID, builtin_rc_param.uiKeyPPeriod, vbr_info->key_p_period);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iKPWeight != vbr_info->kp_weight) {
			DBG_IND("[%d] buitlin vbr iKPWeight %d != kp_weight %d", pathID, builtin_rc_param.iKPWeight, vbr_info->kp_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iP2Weight != vbr_info->p2_weight) {
			DBG_IND("[%d] buitlin vbr iP2Weight %d != p2_weight %d", pathID, builtin_rc_param.iP2Weight, vbr_info->p2_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iP3Weight != vbr_info->p3_weight) {
			DBG_IND("[%d] buitlin vbr iP3Weight %d != p3_weight %d", pathID, builtin_rc_param.iP3Weight, vbr_info->p3_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iLTWeight != vbr_info->lt_weight) {
			DBG_IND("[%d] buitlin vbr iLTWeight %d != lt_weight %d", pathID, builtin_rc_param.iLTWeight, vbr_info->lt_weight);
			goto DIFF_RC;
		}
		if (builtin_rc_param.iMotionAQStrength != vbr_info->motion_aq_str) {
			DBG_IND("[%d] buitlin vbr iMotionAQStrength %d != motion_aq_str %d", pathID, builtin_rc_param.iMotionAQStrength, vbr_info->motion_aq_str);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiSvcBAMode != vbr_info->svc_weight_mode) {
			DBG_IND("[%d] buitlin vbr uiSvcBAMode %d != svc_weight_mode %d", pathID, builtin_rc_param.uiSvcBAMode, vbr_info->svc_weight_mode);
			goto DIFF_RC;
		}
	}

	if (rc_mode == BUILTIN_VDOENC_RC_FIXQP) {
		MP_VDOENC_FIXQP_INFO *fixqp_info = (MP_VDOENC_FIXQP_INFO *)rc_param;

		if (builtin_rc_param.uiEncId != pathID) {
			DBG_IND("[%d] buitlin fixqp uiEncId %d != pathID %d", pathID, builtin_rc_param.uiEncId, pathID);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiRCMode != rc_mode) {
			DBG_IND("[%d] buitlin fixqp uiRCMode %d != rc_mode %d", pathID, builtin_rc_param.uiRCMode, rc_mode);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMinIQp != fixqp_info->fix_i_qp) {
			DBG_IND("[%d] buitlin fixqp uiMinIQp %d != fix_i_qp %d", pathID, builtin_rc_param.uiMinIQp, fixqp_info->fix_i_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiFixIQp != fixqp_info->fix_i_qp) {
			DBG_IND("[%d] buitlin fixqp uiFixIQp %d != fix_i_qp %d", pathID, builtin_rc_param.uiFixIQp, fixqp_info->fix_i_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMaxIQp != fixqp_info->fix_i_qp) {
			DBG_IND("[%d] buitlin fixqp uiMaxIQp %d != fix_i_qp %d", pathID, builtin_rc_param.uiMaxIQp, fixqp_info->fix_i_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMinPQp != fixqp_info->fix_p_qp) {
			DBG_IND("[%d] buitlin fixqp uiMinPQp %d != fix_p_qp %d", pathID, builtin_rc_param.uiMinPQp, fixqp_info->fix_p_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiFixPQp != fixqp_info->fix_p_qp) {
			DBG_IND("[%d] buitlin fixqp uiFixPQp %d != fix_p_qp %d", pathID, builtin_rc_param.uiFixPQp, fixqp_info->fix_p_qp);
			goto DIFF_RC;
		}
		if (builtin_rc_param.uiMaxPQp != fixqp_info->fix_p_qp) {
			DBG_IND("[%d] buitlin fixqp uiMaxPQp %d != fix_p_qp %d", pathID, builtin_rc_param.uiMaxPQp, fixqp_info->fix_p_qp);
			goto DIFF_RC;
		}
		if ((builtin_rc_param.uiFrameRateBase) != fixqp_info->frame_rate) {
			DBG_IND("[%d] buitlin fixqp uiFrameRateBase %d != frame_rate %d", pathID, builtin_rc_param.uiFrameRateBase, fixqp_info->frame_rate);
			goto DIFF_RC;
		}
	}
	return 0;

DIFF_RC:
	return 1;
}
#endif

void NMR_VdoEnc_FpsTimerTrigger(UINT32 pathID)
{
	if (gNMRVdoEncObj[pathID].uiPreEncInCount == 0) {
		gNMRVdoEncObj[pathID].uiPreEncInCount = gNMRVdoEncObj[pathID].uiEncInCount;
		gNMRVdoEncObj[pathID].uiPreEncErrCount = gNMRVdoEncObj[pathID].uiEncErrCount;
		gNMRVdoEncObj[pathID].uiPreEncDropCount = gNMRVdoEncObj[pathID].uiEncDropCount;
	} else {
		DBG_DUMP("[VDOENC][%d] fps = %lld, err = %lld, drop = %lld\r\n", pathID, gNMRVdoEncObj[pathID].uiEncInCount - gNMRVdoEncObj[pathID].uiPreEncInCount, gNMRVdoEncObj[pathID].uiEncErrCount - gNMRVdoEncObj[pathID].uiPreEncErrCount, gNMRVdoEncObj[pathID].uiEncDropCount - gNMRVdoEncObj[pathID].uiPreEncDropCount);
		gNMRVdoEncObj[pathID].uiPreEncInCount = gNMRVdoEncObj[pathID].uiEncInCount;
		gNMRVdoEncObj[pathID].uiPreEncErrCount = gNMRVdoEncObj[pathID].uiEncErrCount;
		gNMRVdoEncObj[pathID].uiPreEncDropCount = gNMRVdoEncObj[pathID].uiEncDropCount;
	}
}

void NMR_VdoEnc_TimerTrigger(UINT32 pathID)
{
	if ((gNMRVdoEncObj[pathID].uiRecFormat == MEDIAREC_TIMELAPSE) || (gNMRVdoEncObj[pathID].uiTimelapseTime > 0)) {
		gNMRVdoEncObj[pathID].bTimelapseTrigger = TRUE;
	} else if (gNMRVdoEncObj[pathID].uiTriggerMode == NMI_VDOENC_TRIGGER_NOTIFY) {
		if (gNMRVdoEncObj[pathID].CallBackFunc) {
			INT32 ret = (INT32)(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_NOTIFY_IPL, pathID);

			if((ret != 0) && ((ret & 0xff000000)==0xf0000000)) { // ((ret & 0xff000000)==0xf0000000) => make sure ret is not a negative number
				//calibration before change IPL period
				ret &= ~0xf0000000; //remove "ADJUST SIGN"
				timer_reload(gNMRVdoEncTimerID[pathID], gNMRVdoEncObj[pathID].iTimerInterval-ret);
				gNMRVdoEncObj[pathID].uiTimerStatus = NMR_VDOENC_TIMER_STATUS_RESET;
				DBG_IND("[VDOENC][%d] adjust enc timer = %d-%d us!!!!\r\n", pathID, gNMRVdoEncObj[pathID].iTimerInterval, ret);
			} else if((gNMRVdoEncObj[pathID].uiTimerStatus == NMR_VDOENC_TIMER_STATUS_NORMAL) && (ret != 0)) {
				//calibration under current IPL period
				timer_reload(gNMRVdoEncTimerID[pathID], gNMRVdoEncObj[pathID].iTimerInterval-ret);
				gNMRVdoEncObj[pathID].uiTimerStatus = NMR_VDOENC_TIMER_STATUS_RESET2;
				DBG_IND("[VDOENC][%d] align enc timer = %d-%d us!!!!\r\n", pathID, gNMRVdoEncObj[pathID].iTimerInterval, ret);
			} else if((gNMRVdoEncObj[pathID].uiTimerStatus == NMR_VDOENC_TIMER_STATUS_RESET) || (gNMRVdoEncObj[pathID].uiTimerStatus == NMR_VDOENC_TIMER_STATUS_RESET2)) {
				//change IPL period
				timer_reload(gNMRVdoEncTimerID[pathID], gNMRVdoEncObj[pathID].iTimerInterval);
				gNMRVdoEncObj[pathID].uiTimerStatus = NMR_VDOENC_TIMER_STATUS_NORMAL;
				DBG_IND("[VDOENC][%d] reset enc timer = %d us!!!!\r\n", pathID, gNMRVdoEncObj[pathID].iTimerInterval);
			} else if(ret!= 0) {
				DBG_IND("[VDOENC][%d] ignore = %d us!!!!\r\n", pathID, ret);
			}
		}
	} else {
		//#if 1
		if (gNMRVdoEncObj[pathID].bTimerTrigComp) {
		if (gNMRVdoEncObj[pathID].bStart) {
			if (gNMRVdoEncObj[pathID].uiTimerStatus == NMR_VDOENC_TIMER_STATUS_RESET) {
				INT32 iOffset = 0;
				INT32 iResetInterval = 0;
				UINT32 uiCurrentTime = Perf_GetCurrent();
				if (gNMRVdoEncObj[pathID].uiLastTime64 == 0) {
					gNMRVdoEncObj[pathID].uiCurrentTime64 = uiCurrentTime;
					gNMRVdoEncObj[pathID].uiLastTime64 = uiCurrentTime;
					gNMRVdoEncObj[pathID].uiLastClock = uiCurrentTime;
				} else {
					if (uiCurrentTime > gNMRVdoEncObj[pathID].uiLastClock) {
						gNMRVdoEncObj[pathID].uiCurrentTime64 += (uiCurrentTime - gNMRVdoEncObj[pathID].uiLastClock);
						gNMRVdoEncObj[pathID].uiLastTime64 += 1000000;
						gNMRVdoEncObj[pathID].uiLastClock = uiCurrentTime;
						iOffset = gNMRVdoEncObj[pathID].uiCurrentTime64 - gNMRVdoEncObj[pathID].uiLastTime64;
					} else {
						//DBG_DUMP("[VDOENC][%d] reset time = %ld us, Offset = %d us\r\n", pathID, uiCurrentTime, iOffset);
						gNMRVdoEncObj[pathID].uiCurrentTime64 += (uiCurrentTime + (0xFFFFFFFF - gNMRVdoEncObj[pathID].uiLastClock));
						gNMRVdoEncObj[pathID].uiLastTime64 += 1000000;
						gNMRVdoEncObj[pathID].uiLastClock = uiCurrentTime;
						iOffset = gNMRVdoEncObj[pathID].uiCurrentTime64 - gNMRVdoEncObj[pathID].uiLastTime64;
					}
				}

				//DBG_DUMP("[VDOENC][%d] cur time = %lld us, Offset = %d us\r\n", pathID, gNMRVdoEncObj[pathID].uiCurrentTime64, iOffset);

				if (iOffset != 0) {
					iResetInterval = gNMRVdoEncObj[pathID].iTimerInterval - iOffset;
					if (iResetInterval > 4000) {
						gNMRVdoEncObj[pathID].uiTimerStatus = NMR_VDOENC_TIMER_STATUS_ALREADY_RESET;
						gNMRVdoEncObj[pathID].uiTimerResetCount = 0;
						timer_reload(gNMRVdoEncTimerID[pathID], iResetInterval);
					} else {
						//delay over interval, just trigger one more to catch up
						gNMRVdoEncObj[pathID].uiTimerStatus = NMR_VDOENC_TIMER_STATUS_NORMAL;
						NMR_VdoTrig_PutJob(pathID);
						NMR_VdoTrig_TrigEncode(pathID);
					}
				} else {
					gNMRVdoEncObj[pathID].uiTimerStatus = NMR_VDOENC_TIMER_STATUS_NORMAL;
				}
			} else if (gNMRVdoEncObj[pathID].uiTimerStatus == NMR_VDOENC_TIMER_STATUS_ALREADY_RESET) {
				gNMRVdoEncObj[pathID].uiTimerResetCount++;
				if (gNMRVdoEncObj[pathID].uiTimerResetCount == 2) {
					//DBG_DUMP("[VDOENC][%d] already reset time = %d us\r\n", pathID, Perf_GetCurrent());
					gNMRVdoEncObj[pathID].uiTimerStatus = NMR_VDOENC_TIMER_STATUS_NORMAL;
					timer_reload(gNMRVdoEncTimerID[pathID], gNMRVdoEncObj[pathID].iTimerInterval);
				}
			}
		}
		}
		//#endif

		NMR_VdoTrig_PutJob(pathID);
		NMR_VdoTrig_TrigEncode(pathID);
	}
}

void NMR_VdoEnc_EncInfo(UINT32 pathID)
{
	char codec_name[0x10];
    char rc_name[0x10];
	char enc_status[0x10];
	UINT32 yuv_in_queue = NMR_VdoTrig_GetYuvCount(pathID);
	UINT32 bs_in_queue  = NMR_VdoTrig_GetBSCount(pathID);

	if (pathID >= NMR_VDOENC_MAX_PATH) {
		DBG_ERR("[VDOENC] show enc info, invalid path index = %d\r\n", pathID);
		return;
	}

	if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_MJPG) {
		strcpy(codec_name, "MJPG");
	} else if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264) {
		strcpy(codec_name, "H264");
	} else if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265) {
		strcpy(codec_name, "H265");
	} else {
		strcpy(codec_name, "NULL");
	}

	if (gNMRVdoEncObj[pathID].CbrInfo.enable) {
		strcpy(rc_name, "CBR");
	} else if (gNMRVdoEncObj[pathID].EVbrInfo.enable) {
		strcpy(rc_name, "EVBR");
	} else if (gNMRVdoEncObj[pathID].VbrInfo.enable) {
		strcpy(rc_name, "VBR");
	} else if (gNMRVdoEncObj[pathID].FixQpInfo.enable) {
		strcpy(rc_name, "FixQP");
	} else {
		strcpy(rc_name, "NULL");
	}

	if (gNMRVdoTrigObj[pathID].uiEncStatus == NMR_VDOENC_ENC_STATUS_IDLE) {
		strcpy(enc_status, "IDLE");
	} else if (gNMRVdoTrigObj[pathID].uiEncStatus == NMR_VDOENC_ENC_STATUS_CHECK_VALID) {
		strcpy(enc_status, "CHK_VLD");
	} else if (gNMRVdoTrigObj[pathID].uiEncStatus == NMR_VDOENC_ENC_STATUS_ENCODE_TRIG) {
		strcpy(enc_status, "TRIG_ENC");
	} else if (gNMRVdoTrigObj[pathID].uiEncStatus == NMR_VDOENC_ENC_STATUS_ENCODE_OK_DONE) {
		strcpy(enc_status, "ENC_OK");
	} else if (gNMRVdoTrigObj[pathID].uiEncStatus == NMR_VDOENC_ENC_STATUS_ENCODE_FAIL_DONE) {
		strcpy(enc_status, "ENC_FAIL");
	} else if (gNMRVdoTrigObj[pathID].uiEncStatus == NMR_VDOENC_ENC_STATUS_BS_CB_DONE) {
		strcpy(enc_status, "CB_DONE");
	} else {
		strcpy(enc_status, "NULL");
	}

	if (gNMRVdoEncObj[pathID].uiVidCodec != NMEDIAREC_ENC_MJPG &&
		gNMRVdoEncObj[pathID].uiVidCodec != NMEDIAREC_ENC_H264 &&
		gNMRVdoEncObj[pathID].uiVidCodec != NMEDIAREC_ENC_H265) {
		DBG_DUMP("[VDOENC][%d] No Enc\r\n", pathID);
	} else {
#ifdef __KERNEL__
		DBG_DUMP("[VDOENC][%d] Codec = %s, RC Mode = %s, W = %d, H = %d, BitRate = %d, Fps = %u.%03u, Gop = %d, ChrmQPIdx = %d, SecChrmQPIdx = %d, SVC = %d, IQP = (%d, %d, %d), PQP = (%d, %d, %d), Static = %d, Weight = %d, RowRc = (%d, %u, %u, %u, %u, %u, %u, %u, %u) , SmartRoi = %d, LTR = (%d, %d), DB = (%d, %d, %d), VUI = (%d, %d, %d, %d, %d, %d, %d, %d, %d), CommRec = (%d, 0x%08x, 0x%08x, 0x%08x, %d), CommSrcOut = (%d, %d, 0x%08x, %d), 3DNR Callback = 0x%08x, ISP Ratio Callback = 0x%08x, AQ = (%d, %d, %d, %d, %d), Rotate = %d, GDR = (%d, %d, %d, %d, %d, %d), MAQDiff = (%d, %d, %d, %d), SLICE = (%d, %d), QPMap = (%d, %08x), SEIChkSum = %d, LowPower = %d, CROP = (%d, %d, %d), Enc(Drop, In, Out, YQue, BQue, Re-Enc, Err) = (%lld, %lld, %lld, %u, %u, %lld, %lld), Status = %s, Trig = %d\r\n",
			pathID,
			codec_name,
			rc_name,
			gNMRVdoEncObj[pathID].InitInfo.width,
			gNMRVdoEncObj[pathID].InitInfo.height,
			gNMRVdoEncObj[pathID].InitInfo.byte_rate*8,
			gNMRVdoEncObj[pathID].InitInfo.frame_rate/1000,
			gNMRVdoEncObj[pathID].InitInfo.frame_rate%1000,
			gNMRVdoEncObj[pathID].InitInfo.gop,
			gNMRVdoEncObj[pathID].InitInfo.chrm_qp_idx,
			gNMRVdoEncObj[pathID].InitInfo.sec_chrm_qp_idx,
			gNMRVdoEncObj[pathID].InitInfo.e_svc,
			gNMRVdoEncObj[pathID].InitInfo.init_i_qp,
			gNMRVdoEncObj[pathID].InitInfo.min_i_qp,
			gNMRVdoEncObj[pathID].InitInfo.max_i_qp,
			gNMRVdoEncObj[pathID].InitInfo.init_p_qp,
			gNMRVdoEncObj[pathID].InitInfo.min_p_qp,
			gNMRVdoEncObj[pathID].InitInfo.max_p_qp,
			gNMRVdoEncObj[pathID].InitInfo.static_time,
			gNMRVdoEncObj[pathID].InitInfo.ip_weight,
			gNMRVdoEncObj[pathID].RowRcInfo.enable,
			gNMRVdoEncObj[pathID].RowRcInfo.i_qp_range,
			gNMRVdoEncObj[pathID].RowRcInfo.i_qp_step,
			gNMRVdoEncObj[pathID].RowRcInfo.min_i_qp,
			gNMRVdoEncObj[pathID].RowRcInfo.max_i_qp,
			gNMRVdoEncObj[pathID].RowRcInfo.p_qp_range,
			gNMRVdoEncObj[pathID].RowRcInfo.p_qp_step,
			gNMRVdoEncObj[pathID].RowRcInfo.min_p_qp,
			gNMRVdoEncObj[pathID].RowRcInfo.max_p_qp,
			gNMRVdoEncObj[pathID].bStartSmartRoi,
			gNMRVdoEncObj[pathID].InitInfo.ltr_interval,
			gNMRVdoEncObj[pathID].InitInfo.ltr_pre_ref,
			gNMRVdoEncObj[pathID].InitInfo.disable_db,
			gNMRVdoEncObj[pathID].InitInfo.db_alpha,
			gNMRVdoEncObj[pathID].InitInfo.db_beta,
			gNMRVdoEncObj[pathID].InitInfo.bVUIEn,
			gNMRVdoEncObj[pathID].InitInfo.sar_width,
			gNMRVdoEncObj[pathID].InitInfo.sar_height,
			gNMRVdoEncObj[pathID].InitInfo.matrix_coef,
			gNMRVdoEncObj[pathID].InitInfo.transfer_characteristics,
			gNMRVdoEncObj[pathID].InitInfo.colour_primaries,
			gNMRVdoEncObj[pathID].InitInfo.video_format,
			gNMRVdoEncObj[pathID].InitInfo.color_range,
			gNMRVdoEncObj[pathID].InitInfo.time_present_flag,
			gNMRVdoEncObj[pathID].InitInfo.comm_recfrm_en,
			gNMRVdoEncObj[pathID].MemInfo.uiRecBuffAddr[0],
			gNMRVdoEncObj[pathID].MemInfo.uiRecBuffAddr[1],
			gNMRVdoEncObj[pathID].MemInfo.uiRecBuffAddr[2],
			gNMRVdoEncObj[pathID].MemInfo.uiRecBuffSize,
			gNMRVdoEncObj[pathID].bCommSrcOut,
			gNMRVdoEncObj[pathID].bCommSrcOutNoJPGEnc,
			gNMRVdoEncObj[pathID].MemInfo.uiSnapshotAddr,
			gNMRVdoEncObj[pathID].MemInfo.uiSnapshotSize,
			(UINT32)gNMRVdoEncObj[pathID].v3dnrCb.VdoEnc_3DNRDo,
			(UINT32)gNMRVdoEncObj[pathID].ispCb.vdoenc_isp_cb,
			gNMRVdoEncObj[pathID].AqInfo.enable,
			gNMRVdoEncObj[pathID].AqInfo.i_str,
			gNMRVdoEncObj[pathID].AqInfo.p_str,
			gNMRVdoEncObj[pathID].AqInfo.max_delta_qp,
			gNMRVdoEncObj[pathID].AqInfo.min_delta_qp,
			gNMRVdoEncObj[pathID].InitInfo.rotate,
			gNMRVdoEncObj[pathID].GdrInfo.enable,
			gNMRVdoEncObj[pathID].GdrInfo.period,
			gNMRVdoEncObj[pathID].GdrInfo.number,
			gNMRVdoEncObj[pathID].GdrInfo.enable_gdr_i_frm,
			gNMRVdoEncObj[pathID].GdrInfo.enable_gdr_qp,
			gNMRVdoEncObj[pathID].GdrInfo.gdr_qp,
			gNMRVdoEncObj[pathID].MaqDiffInfo.enable,
			gNMRVdoEncObj[pathID].MaqDiffInfo.str,
			gNMRVdoEncObj[pathID].MaqDiffInfo.start_idx,
			gNMRVdoEncObj[pathID].MaqDiffInfo.end_idx,
			gNMRVdoEncObj[pathID].SliceSplitInfo.enable,
			gNMRVdoEncObj[pathID].SliceSplitInfo.slice_row_num,
			gNMRVdoEncObj[pathID].QpMapInfo.enable,
			(UINT32)gNMRVdoEncObj[pathID].QpMapInfo.qp_map_addr,
			gNMRVdoEncObj[pathID].bSEIBsChkSum,
			gNMRVdoEncObj[pathID].bLowPower,
			gNMRVdoEncObj[pathID].InitInfo.frm_crop_info.frm_crop_enable,
			gNMRVdoEncObj[pathID].InitInfo.frm_crop_info.display_w,
			gNMRVdoEncObj[pathID].InitInfo.frm_crop_info.display_h,
			gNMRVdoEncObj[pathID].uiEncDropCount,
			gNMRVdoEncObj[pathID].uiEncInCount,
			gNMRVdoEncObj[pathID].uiEncOutCount,
			yuv_in_queue,
			bs_in_queue,
			gNMRVdoEncObj[pathID].uiEncReCount,
			gNMRVdoEncObj[pathID].uiEncErrCount,
			enc_status,
			gNMRVdoEncObj[pathID].uiTriggerMode
			);
#else // RTOS
		DBG_DUMP("[VDOENC][%d] Codec = %s, RC Mode = %s, W = %d, H = %d, BitRate = %d, Fps = %u.%03u, Gop = %d, ChrmQPIdx = %d, SecChrmQPIdx = %d, SVC = %d, IQP = (%d, %d, %d), PQP = (%d, %d, %d), Static = %d, Weight = %d, RowRc = (%d, %u, %u, %u, %u, %u, %u, %u, %u)",
			pathID,
			codec_name,
			rc_name,
			gNMRVdoEncObj[pathID].InitInfo.width,
			gNMRVdoEncObj[pathID].InitInfo.height,
			gNMRVdoEncObj[pathID].InitInfo.byte_rate*8,
			gNMRVdoEncObj[pathID].InitInfo.frame_rate/1000,
			gNMRVdoEncObj[pathID].InitInfo.frame_rate%1000,
			gNMRVdoEncObj[pathID].InitInfo.gop,
			gNMRVdoEncObj[pathID].InitInfo.chrm_qp_idx,
			gNMRVdoEncObj[pathID].InitInfo.sec_chrm_qp_idx,
			gNMRVdoEncObj[pathID].InitInfo.e_svc,
			gNMRVdoEncObj[pathID].InitInfo.init_i_qp,
			gNMRVdoEncObj[pathID].InitInfo.min_i_qp,
			gNMRVdoEncObj[pathID].InitInfo.max_i_qp,
			gNMRVdoEncObj[pathID].InitInfo.init_p_qp,
			gNMRVdoEncObj[pathID].InitInfo.min_p_qp,
			gNMRVdoEncObj[pathID].InitInfo.max_p_qp,
			gNMRVdoEncObj[pathID].InitInfo.static_time,
			gNMRVdoEncObj[pathID].InitInfo.ip_weight,
			gNMRVdoEncObj[pathID].RowRcInfo.enable,
			gNMRVdoEncObj[pathID].RowRcInfo.i_qp_range,
			gNMRVdoEncObj[pathID].RowRcInfo.i_qp_step,
			gNMRVdoEncObj[pathID].RowRcInfo.min_i_qp,
			gNMRVdoEncObj[pathID].RowRcInfo.max_i_qp,
			gNMRVdoEncObj[pathID].RowRcInfo.p_qp_range,
			gNMRVdoEncObj[pathID].RowRcInfo.p_qp_step,
			gNMRVdoEncObj[pathID].RowRcInfo.min_p_qp,
			gNMRVdoEncObj[pathID].RowRcInfo.max_p_qp
			);

		DBG_DUMP(" , SmartRoi = %d, LTR = (%d, %d), DB = (%d, %d, %d), VUI = (%d, %d, %d, %d, %d, %d, %d, %d, %d), 3DNR Callback = 0x%08x, ISP Ratio Callback = 0x%08x, AQ = (%d, %d, %d, %d, %d), Rotate = %d, GDR = (%d, %d, %d, %d, %d, %d)",
			gNMRVdoEncObj[pathID].bStartSmartRoi,
			gNMRVdoEncObj[pathID].InitInfo.ltr_interval,
			gNMRVdoEncObj[pathID].InitInfo.ltr_pre_ref,
			gNMRVdoEncObj[pathID].InitInfo.disable_db,
			gNMRVdoEncObj[pathID].InitInfo.db_alpha,
			gNMRVdoEncObj[pathID].InitInfo.db_beta,
			gNMRVdoEncObj[pathID].InitInfo.bVUIEn,
			gNMRVdoEncObj[pathID].InitInfo.sar_width,
			gNMRVdoEncObj[pathID].InitInfo.sar_height,
			gNMRVdoEncObj[pathID].InitInfo.matrix_coef,
			gNMRVdoEncObj[pathID].InitInfo.transfer_characteristics,
			gNMRVdoEncObj[pathID].InitInfo.colour_primaries,
			gNMRVdoEncObj[pathID].InitInfo.video_format,
			gNMRVdoEncObj[pathID].InitInfo.color_range,
			gNMRVdoEncObj[pathID].InitInfo.time_present_flag,
			(UINT32)gNMRVdoEncObj[pathID].v3dnrCb.VdoEnc_3DNRDo,
			(UINT32)gNMRVdoEncObj[pathID].ispCb.vdoenc_isp_cb,
			gNMRVdoEncObj[pathID].AqInfo.enable,
			gNMRVdoEncObj[pathID].AqInfo.i_str,
			gNMRVdoEncObj[pathID].AqInfo.p_str,
			gNMRVdoEncObj[pathID].AqInfo.max_delta_qp,
			gNMRVdoEncObj[pathID].AqInfo.min_delta_qp,
			gNMRVdoEncObj[pathID].InitInfo.rotate,
			gNMRVdoEncObj[pathID].GdrInfo.enable,
			gNMRVdoEncObj[pathID].GdrInfo.period,
			gNMRVdoEncObj[pathID].GdrInfo.number,
			gNMRVdoEncObj[pathID].GdrInfo.enable_gdr_i_frm,
			gNMRVdoEncObj[pathID].GdrInfo.enable_gdr_qp,
			gNMRVdoEncObj[pathID].GdrInfo.gdr_qp
			);

		DBG_DUMP(", MAQDiff = (%d, %d, %d, %d), CommRec = (%d, 0x%08x, 0x%08x, 0x%08x, %d), CommSrcOut = (%d, %d, 0x%08x, %d)",
			gNMRVdoEncObj[pathID].MaqDiffInfo.enable,
			gNMRVdoEncObj[pathID].MaqDiffInfo.str,
			gNMRVdoEncObj[pathID].MaqDiffInfo.start_idx,
			gNMRVdoEncObj[pathID].MaqDiffInfo.end_idx,
			gNMRVdoEncObj[pathID].InitInfo.comm_recfrm_en,
			(UINT32)gNMRVdoEncObj[pathID].MemInfo.uiRecBuffAddr[0],
			(UINT32)gNMRVdoEncObj[pathID].MemInfo.uiRecBuffAddr[1],
			(UINT32)gNMRVdoEncObj[pathID].MemInfo.uiRecBuffAddr[2],
			gNMRVdoEncObj[pathID].MemInfo.uiRecBuffSize,
			gNMRVdoEncObj[pathID].bCommSrcOut,
			gNMRVdoEncObj[pathID].bCommSrcOutNoJPGEnc,
			(UINT32)gNMRVdoEncObj[pathID].MemInfo.uiSnapshotAddr,
			gNMRVdoEncObj[pathID].MemInfo.uiSnapshotSize);

		DBG_DUMP(", SLICE = (%d, %d), QPMap = (%d, %08x), SEIChkSum = %d, LowPower = %d, CROP = (%d, %d, %d), Enc(Drop, In, Out, YQue, BQue, Re-Enc, Err) = (%lld, %lld, %lld, %u, %u, %lld, %lld), Status = %s, Trig = %d\r\n",
			gNMRVdoEncObj[pathID].SliceSplitInfo.enable,
			gNMRVdoEncObj[pathID].SliceSplitInfo.slice_row_num,
			gNMRVdoEncObj[pathID].QpMapInfo.enable,
			(UINT32)gNMRVdoEncObj[pathID].QpMapInfo.qp_map_addr,
			gNMRVdoEncObj[pathID].bSEIBsChkSum,
			gNMRVdoEncObj[pathID].bLowPower,
			gNMRVdoEncObj[pathID].InitInfo.frm_crop_info.frm_crop_enable,
			gNMRVdoEncObj[pathID].InitInfo.frm_crop_info.display_w,
			gNMRVdoEncObj[pathID].InitInfo.frm_crop_info.display_h,
			gNMRVdoEncObj[pathID].uiEncDropCount,
			gNMRVdoEncObj[pathID].uiEncInCount,
			gNMRVdoEncObj[pathID].uiEncOutCount,
			yuv_in_queue,
			bs_in_queue,
			gNMRVdoEncObj[pathID].uiEncReCount,
			gNMRVdoEncObj[pathID].uiEncErrCount,
			enc_status,
			gNMRVdoEncObj[pathID].uiTriggerMode
			);
#endif // #ifdef __KERNEL__
	}
}

void NMR_VdoEnc_Top(int (*dump)(const char *fmt, ...))
{
	char codec_name[0x10];
	UINT32 i = 0;
	unsigned long flags = 0;
	UINT32 pathID = 0;
	NMR_VDOENC_TOPINFO **topinfo = NULL;

#if defined(__LINUX)
	topinfo = (NMR_VDOENC_TOPINFO **)vmalloc(sizeof(NMR_VDOENC_TOPINFO *) * NMR_VDOENC_MAX_PATH);
	for (i = 0; i < NMR_VDOENC_MAX_PATH; i++) {
		topinfo[i] = (NMR_VDOENC_TOPINFO *)vmalloc(sizeof(NMR_VDOENC_TOPINFO) * NMI_VDOENC_TOP_MAX);
	}
#elif defined(__FREERTOS)
	topinfo = (NMR_VDOENC_TOPINFO **)malloc(sizeof(NMR_VDOENC_TOPINFO *) * NMR_VDOENC_MAX_PATH);
	for (i = 0; i < NMR_VDOENC_MAX_PATH; i++) {
		topinfo[i] = (NMR_VDOENC_TOPINFO *)malloc(sizeof(NMR_VDOENC_TOPINFO) * NMI_VDOENC_TOP_MAX);
	}
#endif
	for (pathID = 0; pathID < NMR_VDOENC_MAX_PATH; pathID++) {
		if (gNMRVdoEncObj[pathID].bStart == FALSE) {
			continue;
		}
		for (i = 0; i < NMI_VDOENC_TOP_MAX; i++) {
			NMR_VdoEnc_Lock_cpu(&flags);
			topinfo[pathID][i].uiCur[0] = gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiCur[0];
			topinfo[pathID][i].uiCur[1] = gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiCur[1];
			topinfo[pathID][i].uiCur[2] = gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiCur[2];
			topinfo[pathID][i].uiCur[3] = gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiCur[3];
			topinfo[pathID][i].uiMin = gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiMin;
			topinfo[pathID][i].uiMax = gNMRVdoEncObj[pathID].uiEncTopInfo[i].uiMax;
			NMR_VdoEnc_Unlock_cpu(&flags);
		}
	}

	for (pathID = 0; pathID < NMR_VDOENC_MAX_PATH; pathID++) {
		if (gNMRVdoEncObj[pathID].bStart == FALSE) {
			continue;
		}
		if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_MJPG) {
			strcpy(codec_name, "MJPG");
		} else if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264) {
			strcpy(codec_name, "H264");
		} else if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265) {
			strcpy(codec_name, "H265");
		} else {
			strcpy(codec_name, "NULL");
		}

		if (gNMRVdoEncObj[pathID].uiVidCodec != NMEDIAREC_ENC_MJPG &&
			gNMRVdoEncObj[pathID].uiVidCodec != NMEDIAREC_ENC_H264 &&
			gNMRVdoEncObj[pathID].uiVidCodec != NMEDIAREC_ENC_H265) {
			dump("[VDOENC][%d] No Enc\r\n", pathID);
			continue;
		} else {
			UINT32 top_avg[NMI_VDOENC_TOP_MAX] = {0};
			UINT32 top_sum[NMI_VDOENC_TOP_MAX] = {0};

			for (i = 0; i < 4; i++) {
				top_sum[NMI_VDOENC_TOP_MD_TIME] += topinfo[pathID][NMI_VDOENC_TOP_MD_TIME].uiCur[i];
				top_sum[NMI_VDOENC_TOP_SW_TIME] += topinfo[pathID][NMI_VDOENC_TOP_SW_TIME].uiCur[i];
				top_sum[NMI_VDOENC_TOP_HW_TIME] += topinfo[pathID][NMI_VDOENC_TOP_HW_TIME].uiCur[i];
				top_sum[NMI_VDOENC_TOP_OSD_INPUT_TIME] += topinfo[pathID][NMI_VDOENC_TOP_OSD_INPUT_TIME].uiCur[i];
				top_sum[NMI_VDOENC_TOP_OSD_FINISH_TIME] += topinfo[pathID][NMI_VDOENC_TOP_OSD_FINISH_TIME].uiCur[i];
			}
			top_avg[NMI_VDOENC_TOP_MD_TIME] = top_sum[NMI_VDOENC_TOP_MD_TIME] >> 2;
			top_avg[NMI_VDOENC_TOP_SW_TIME] = top_sum[NMI_VDOENC_TOP_SW_TIME] >> 2;
			top_avg[NMI_VDOENC_TOP_HW_TIME] = top_sum[NMI_VDOENC_TOP_HW_TIME] >> 2;
			top_avg[NMI_VDOENC_TOP_OSD_INPUT_TIME] = top_sum[NMI_VDOENC_TOP_OSD_INPUT_TIME] >> 2;
			top_avg[NMI_VDOENC_TOP_OSD_FINISH_TIME] = top_sum[NMI_VDOENC_TOP_OSD_FINISH_TIME] >> 2;

			for (i = 0; i < 4; i++) {
				dump("[VDOENC][%d] TOP Codec = %s,  md = (cur %d, min %d, max %d, avg %d)us, sw = (cur %d, min %d, max %d, avg %d)us, hw = (cur %d, min %d, max %d, avg %d)us, do_osd = (cur %d, min %d, max %d, avg %d)us, finish_osd = (cur %d, min %d, max %d, avg %d)us\r\n",
					pathID,
					codec_name,
					topinfo[pathID][NMI_VDOENC_TOP_MD_TIME].uiCur[i],
					topinfo[pathID][NMI_VDOENC_TOP_MD_TIME].uiMin,
					topinfo[pathID][NMI_VDOENC_TOP_MD_TIME].uiMax,
					top_avg[NMI_VDOENC_TOP_MD_TIME],
					topinfo[pathID][NMI_VDOENC_TOP_SW_TIME].uiCur[i],
					topinfo[pathID][NMI_VDOENC_TOP_SW_TIME].uiMin,
					topinfo[pathID][NMI_VDOENC_TOP_SW_TIME].uiMax,
					top_avg[NMI_VDOENC_TOP_SW_TIME],
					topinfo[pathID][NMI_VDOENC_TOP_HW_TIME].uiCur[i],
					topinfo[pathID][NMI_VDOENC_TOP_HW_TIME].uiMin,
					topinfo[pathID][NMI_VDOENC_TOP_HW_TIME].uiMax,
					top_avg[NMI_VDOENC_TOP_HW_TIME],
					topinfo[pathID][NMI_VDOENC_TOP_OSD_INPUT_TIME].uiCur[i],
					topinfo[pathID][NMI_VDOENC_TOP_OSD_INPUT_TIME].uiMin,
					topinfo[pathID][NMI_VDOENC_TOP_OSD_INPUT_TIME].uiMax,
					top_avg[NMI_VDOENC_TOP_OSD_INPUT_TIME],
					topinfo[pathID][NMI_VDOENC_TOP_OSD_FINISH_TIME].uiCur[i],
					topinfo[pathID][NMI_VDOENC_TOP_OSD_FINISH_TIME].uiMin,
					topinfo[pathID][NMI_VDOENC_TOP_OSD_FINISH_TIME].uiMax,
					top_avg[NMI_VDOENC_TOP_OSD_FINISH_TIME]
					);
			}
		}
	}

#if defined(__LINUX)
	for (i = 0; i < NMR_VDOENC_MAX_PATH; i++) {
		vfree(topinfo[i]);
	}
	vfree(topinfo);
#elif defined(__FREERTOS)
	for (i = 0; i < NMR_VDOENC_MAX_PATH; i++) {
		free(topinfo[i]);
	}
	free(topinfo);
#endif
}

void NMR_VdoEnc_GetAQ(UINT32 pathID, MP_VDOENC_AQ_INFO *pAqInfo)
{
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);
	if (pVidEnc->GetInfo && pAqInfo) {
		(pVidEnc->GetInfo)(VDOENC_GET_AQ, (UINT32 *)&gNMRVdoEncObj[pathID].AqInfo, 0, 0);
		memcpy((void *)pAqInfo, (void *)&gNMRVdoEncObj[pathID].AqInfo, sizeof(MP_VDOENC_AQ_INFO));
	} else {
		DBG_ERR("[VDOENC][%d] GetAQ video encoder or info is NULL\r\n", pathID);
	}
}

void NMR_VdoEnc_GetGDR(UINT32 pathID, MP_VDOENC_GDR_INFO *pGdrInfo)
{
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);
	if (pVidEnc->GetInfo && pGdrInfo) {
		(pVidEnc->GetInfo)(VDOENC_GET_GDR, (UINT32 *)&gNMRVdoEncObj[pathID].GdrInfo, 0, 0);
		memcpy((void *)pGdrInfo, (void *)&gNMRVdoEncObj[pathID].GdrInfo, sizeof(MP_VDOENC_GDR_INFO));
	} else {
		DBG_ERR("[VDOENC][%d] GetGDR video encoder or info is NULL\r\n", pathID);
	}
}

UINT32 NMR_VdoEnc_GetBytesPerSecond(UINT32 pathID)
{
	UINT32 bytePerSec;
	UINT32 TBR, wid, hei;//frame rate

	TBR = gNMRVdoEncObj[pathID].InitInfo.byte_rate;
	wid = gNMRVdoEncObj[pathID].InitInfo.width;
	hei = gNMRVdoEncObj[pathID].InitInfo.height;

	//DBG_DUMP("[VDOENC][%d] fr = %d, TBR = %d, wid = %d, hei = %d\r\n", pathID, fr, TBR, wid, hei);

	if (TBR) {
		bytePerSec = TBR;//2016/11/25 + TBR/30 ;
	} else {
		bytePerSec = wid * hei / 5;
		gNMRVdoEncObj[pathID].InitInfo.byte_rate = bytePerSec;
	}
	//DBG_DUMP("[REC]GetVideoByte:wid=%d, hei=%d!\r\n", wid , hei);
	return bytePerSec;
}

void NMR_VdoEnc_GetCBR(UINT32 pathID, MP_VDOENC_CBR_INFO *pCbrInfo)
{
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);

	if (pVidEnc->GetInfo && pCbrInfo) {
		(pVidEnc->GetInfo)(VDOENC_GET_CBR, (UINT32 *)&gNMRVdoEncObj[pathID].CbrInfo, 0, 0);
		memcpy((void *)pCbrInfo, (void *)&gNMRVdoEncObj[pathID].CbrInfo, sizeof(MP_VDOENC_CBR_INFO));
	} else {
		DBG_ERR("[VDOENC][%d] GetCBR video encoder or info is NULL\r\n", pathID);
	}
}

void NMR_VdoEnc_GetDesc(UINT32 pathID, UINT32 *pAddr, UINT32 *pSize)
{
	UINT32 descAdr, descSze;

	descAdr = gNMRVdoEncObj[pathID].uiDescAddr;
	descSze = gNMRVdoEncObj[pathID].uiDescSize;
	*pAddr = descAdr;
	*pSize = descSze;
}

void NMR_VdoEnc_GetFixQP(UINT32 pathID, MP_VDOENC_FIXQP_INFO *pFixQpInfo)
{
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);

	if (pVidEnc->GetInfo && pFixQpInfo) {
		(pVidEnc->GetInfo)(VDOENC_GET_FIXQP, (UINT32 *)&gNMRVdoEncObj[pathID].FixQpInfo, 0, 0);
		memcpy((void *)pFixQpInfo, (void *)&gNMRVdoEncObj[pathID].FixQpInfo, sizeof(MP_VDOENC_FIXQP_INFO));
	} else {
		DBG_ERR("[VDOENC][%d] GetFIXQP video encoder or info is NULL\r\n", pathID);
	}
}

void NMR_VdoEnc_GetEVBR(UINT32 pathID, MP_VDOENC_EVBR_INFO *pEVbrInfo)
{
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);

	if (pVidEnc->GetInfo && pEVbrInfo) {
		(pVidEnc->GetInfo)(VDOENC_GET_EVBR, (UINT32 *)&gNMRVdoEncObj[pathID].EVbrInfo, 0, 0);
		memcpy((void *)pEVbrInfo, (void *)&gNMRVdoEncObj[pathID].EVbrInfo, sizeof(MP_VDOENC_EVBR_INFO));
	} else {
		DBG_ERR("[VDOENC][%d] GetEVBR video encoder or info is NULL\r\n", pathID);
	}
}

void NMR_VdoEnc_GetRC(UINT32 pathID, MP_VDOENC_RC_INFO *pRcInfo)
{
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);

	if (pVidEnc->GetInfo && pRcInfo) {
		(pVidEnc->GetInfo)(VDOENC_GET_RC, (UINT32 *)pRcInfo, 0, 0);
	} else {
		DBG_ERR("[VDOENC][%d] GetRC video encoder or Info is NULL\r\n", pathID);
	}
}

void NMR_VdoEnc_GetRCStr(UINT32 pathID, char *Buf, UINT32 BufLen)
{
	MP_VDOENC_RC_INFO RcInfo = {0};
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);

	if (pVidEnc->GetInfo && Buf) {
		(pVidEnc->GetInfo)(VDOENC_GET_RC, (UINT32 *)&RcInfo, 0, 0);
		snprintf(Buf, BufLen, "%d/%d/%d %d/%d/%d",
			RcInfo.uiSliceType,
			RcInfo.uiFrameRate,
			RcInfo.uiGOPSize,
			RcInfo.uiQp,
			RcInfo.uiStaticFrame,
			RcInfo.uiFrameByte);
	} else {
		DBG_ERR("[VDOENC][%d] GetRCStr video encoder or Buf is NULL\r\n", pathID);
	}
}

void NMR_VdoEnc_GetVBR(UINT32 pathID, MP_VDOENC_VBR_INFO *pVbrInfo)
{
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);

	if (pVidEnc->GetInfo && pVbrInfo) {
		(pVidEnc->GetInfo)(VDOENC_GET_VBR, (UINT32 *)&gNMRVdoEncObj[pathID].VbrInfo, 0, 0);
		memcpy((void *)pVbrInfo, (void *)&gNMRVdoEncObj[pathID].VbrInfo, sizeof(MP_VDOENC_VBR_INFO));
	} else {
		DBG_ERR("[VDOENC][%d] GetVBR video encoder or info is NULL\r\n", pathID);
	}
}

UINT32 NMR_VdoEnc_GetVidInputPathNum(void)
{
	//video input path number
	return gNMRVdoEncPathCount;
}

void NMR_VdoEnc_FpsInfo(UINT32 pathID, UINT32 Enable)
{
	if (Enable) {
		_NMR_VdoEnc_OpenFpsTimer(pathID);
	} else {
		_NMR_VdoEnc_CloseFpsTimer(pathID);
	}
}

void NMR_VdoEnc_RcInfo(UINT32 pathID, UINT32 Enable)
{
	MP_VDOENC_ENCODER *pVidEnc = NULL;

	if (pathID >= NMR_VDOENC_MAX_PATH) {
		DBG_ERR("[VDOENC] show rc info, invalid path index = %d\r\n", pathID);
		return;
	}

	pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);

	if (pVidEnc->SetInfo) {
		(pVidEnc->SetInfo)(VDOENC_SET_DUMPINFO, Enable, pathID, 0);
	} else {
		DBG_ERR("[VDOENC][%d] SetDumpInfo video encoder is NULL\r\n", pathID);
	}
}

void NMR_VdoEnc_SetAQ(UINT32 pathID, MP_VDOENC_AQ_INFO *pAqInfo)
{
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);

	if (!pAqInfo){
		DBG_ERR("[VDOENC][%d] Set AQ Info is NULL\r\n", pathID);
		return;
	}

	DBG_DUMP("[VDOENC][%d] Set AQ Enable = %d, I_Str = %d, P_Str = %d, MaxAq = %d, MinAq = %d\r\n",
			pathID,
			pAqInfo->enable,
			pAqInfo->i_str,
			pAqInfo->p_str,
			pAqInfo->max_delta_qp,
			pAqInfo->min_delta_qp);

	memcpy((void *)&gNMRVdoEncObj[pathID].AqInfo, (void *)pAqInfo, sizeof(MP_VDOENC_AQ_INFO));

	if (pVidEnc->SetInfo && pAqInfo) {
		(pVidEnc->SetInfo)(VDOENC_SET_AQ, (UINT32)&gNMRVdoEncObj[pathID].AqInfo, 0, 0);
	}
}

void NMR_VdoEnc_SetGDR(UINT32 pathID, MP_VDOENC_GDR_INFO *pGdrInfo)
{
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);

	if (!pGdrInfo){
		DBG_ERR("[VDOENC][%d] Set GDR Info is NULL\r\n", pathID);
		return;
	}

	DBG_DUMP("[VDOENC][%d] Set GDR Enable = %d, Period = %d, RowNumber = %d\r\n",
			pathID,
			pGdrInfo->enable,
			pGdrInfo->period,
			pGdrInfo->number);

	memcpy((void *)&gNMRVdoEncObj[pathID].GdrInfo, (void *)pGdrInfo, sizeof(MP_VDOENC_GDR_INFO));

	if (pVidEnc->SetInfo && pGdrInfo) {
		(pVidEnc->SetInfo)(VDOENC_SET_GDR, (UINT32)&gNMRVdoEncObj[pathID].GdrInfo, 0, 0);
	}
}

void NMR_VdoEnc_SetSliceSplit(UINT32 pathID, MP_VDOENC_SLICESPLIT_INFO *pSliceSplitInfo)
{
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);

	if (!pSliceSplitInfo){
		DBG_ERR("[VDOENC][%d] Set SliceSplit Info is NULL\r\n", pathID);
		return;
	}

	DBG_DUMP("[VDOENC][%d] Set SliceSplit Enable = %d, SliceRowNumber = %d\r\n",
			pathID,
			pSliceSplitInfo->enable,
			pSliceSplitInfo->slice_row_num);

	memcpy((void *)&gNMRVdoEncObj[pathID].SliceSplitInfo, (void *)pSliceSplitInfo, sizeof(MP_VDOENC_SLICESPLIT_INFO));

	if (pVidEnc->SetInfo && pSliceSplitInfo) {
		(pVidEnc->SetInfo)(VDOENC_SET_SLICESPLIT, (UINT32)&gNMRVdoEncObj[pathID].SliceSplitInfo, 0, 0);
	}
}

void NMR_VdoEnc_SetCBR(UINT32 pathID, MP_VDOENC_CBR_INFO *pCbrInfo)
{
	if (!pCbrInfo){
		DBG_ERR("[VDOENC][%d] Set CBR Info is NULL\r\n", pathID);
		return;
	}

#if MP_VDOENC_SHOW_MSG
	if (gNMRVdoEncObj[pathID].bStart && pCbrInfo->enable) {
		DBG_IND("[VDOENC][%d] Set CBR Enable = %d, StaticTime = %d, ByteRate = %d, Fr = %u.%03u, GOP = %d, IQp = (%d, %d, %d), PQp = (%d, %d, %d), IPWei = %d, KP_Prd = %d, KP_Wei = %d, P2_Wei = %d, P3_Wei = %d, LT_Wei = %d, mas = %d, max_size = %d\r\n",
				 pathID,
				 pCbrInfo->enable,
				 pCbrInfo->static_time,
				 pCbrInfo->byte_rate,
				 (pCbrInfo->frame_rate/1000),
				 (pCbrInfo->frame_rate%1000),
				 pCbrInfo->gop,
				 pCbrInfo->init_i_qp,
				 pCbrInfo->min_i_qp,
				 pCbrInfo->max_i_qp,
				 pCbrInfo->init_p_qp,
				 pCbrInfo->min_p_qp,
				 pCbrInfo->max_p_qp,
				 pCbrInfo->ip_weight,
				 pCbrInfo->key_p_period,
				 pCbrInfo->kp_weight,
				 pCbrInfo->p2_weight,
				 pCbrInfo->p3_weight,
				 pCbrInfo->lt_weight,
				 pCbrInfo->motion_aq_str,
				 pCbrInfo->max_frame_size);
	}
#endif
	{
		UINT32 svc_weight_mode = gNMRVdoEncObj[pathID].CbrInfo.svc_weight_mode; // backup
		UINT32 br_tolerance = gNMRVdoEncObj[pathID].CbrInfo.br_tolerance; // backup
		UINT32 encode_frame_rate = gNMRVdoEncObj[pathID].CbrInfo.encode_frame_rate; // backup
		UINT32 src_frame_rate = gNMRVdoEncObj[pathID].CbrInfo.src_frame_rate; // backup
		memcpy((void *)&gNMRVdoEncObj[pathID].CbrInfo, (void *)pCbrInfo, sizeof(MP_VDOENC_CBR_INFO));
		gNMRVdoEncObj[pathID].CbrInfo.svc_weight_mode = svc_weight_mode; // restore
		gNMRVdoEncObj[pathID].CbrInfo.br_tolerance = br_tolerance; // restore
		gNMRVdoEncObj[pathID].CbrInfo.encode_frame_rate = encode_frame_rate; // restore
		gNMRVdoEncObj[pathID].CbrInfo.src_frame_rate = src_frame_rate; // restore
	}
	if (gNMRVdoEncObj[pathID].CbrInfo.enable) {
		gNMRVdoEncObj[pathID].EVbrInfo.enable = 0;
		gNMRVdoEncObj[pathID].VbrInfo.enable = 0;
		gNMRVdoEncObj[pathID].FixQpInfo.enable = 0;

		if (gNMRVdoEncObj[pathID].bStart &&
			(/*gNMRVdoEncObj[pathID].InitInfo.gop != gNMRVdoEncObj[pathID].CbrInfo.gop ||*/
			gNMRVdoEncObj[pathID].InitInfo.frame_rate != gNMRVdoEncObj[pathID].CbrInfo.frame_rate)) {
			gNMRVdoEncObj[pathID].bResetIFrame = TRUE;
		}

		//gNMRVdoEncObj[pathID].InitInfo.gop            = gNMRVdoEncObj[pathID].CbrInfo.gop;   // maybe encode GOP != rc GOP now, "K-customer" want to do dynamic gop via resetI by themself
		gNMRVdoEncObj[pathID].InitInfo.frame_rate     = gNMRVdoEncObj[pathID].CbrInfo.frame_rate;
		gNMRVdoEncObj[pathID].InitInfo.byte_rate      = gNMRVdoEncObj[pathID].CbrInfo.byte_rate;
		gNMRVdoEncObj[pathID].InitInfo.init_i_qp      = gNMRVdoEncObj[pathID].CbrInfo.init_i_qp;
		gNMRVdoEncObj[pathID].InitInfo.min_i_qp       = gNMRVdoEncObj[pathID].CbrInfo.min_i_qp;
		gNMRVdoEncObj[pathID].InitInfo.max_i_qp       = gNMRVdoEncObj[pathID].CbrInfo.max_i_qp;
		gNMRVdoEncObj[pathID].InitInfo.init_p_qp      = gNMRVdoEncObj[pathID].CbrInfo.init_p_qp;
		gNMRVdoEncObj[pathID].InitInfo.min_p_qp       = gNMRVdoEncObj[pathID].CbrInfo.min_p_qp;
		gNMRVdoEncObj[pathID].InitInfo.max_p_qp       = gNMRVdoEncObj[pathID].CbrInfo.max_p_qp;
		gNMRVdoEncObj[pathID].InitInfo.static_time    = gNMRVdoEncObj[pathID].CbrInfo.static_time;
		gNMRVdoEncObj[pathID].InitInfo.ip_weight      = gNMRVdoEncObj[pathID].CbrInfo.ip_weight;

#ifdef __KERNEL__
		// JIRA: IVOT_N12006_CO-137, for fastboot dynamic byterate setting
		VdoEnc_Builtin_SetParam(pathID, BUILTIN_VDOENC_INIT_PARAM_DYNAMIC_BYTERATE, gNMRVdoEncObj[pathID].CbrInfo.byte_rate);
#endif

	}

	gNMRVdoEncObj[pathID].uiSetRCMode = NMR_VDOENC_RC_CBR;
}

void NMR_VdoEnc_SetDesc(UINT32 pathID)
{
	MP_VDOENC_BUF buf = {0};
	NMI_VDOENC_BS_INFO vidBSinfo = {0};
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);

	if ((pVidEnc) && (pVidEnc->GetInfo)) {
		(pVidEnc->GetInfo)(VDOENC_GET_DESC, (UINT32 *)&buf, 0, 0);
	}
	gNMRVdoEncObj[pathID].uiDescAddr = buf.addr;
	gNMRVdoEncObj[pathID].uiDescSize = buf.size;//mov no need to align to 4, avi align itself
	vidBSinfo.PathID = pathID;
	vidBSinfo.Addr = buf.addr;
	vidBSinfo.Size = buf.size;
	if (gNMRVdoEncObj[pathID].CallBackFunc) {
		(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_INFO_READY, (UINT32) &vidBSinfo);
	}
}

void NMR_VdoEnc_SetEVBR(UINT32 pathID, MP_VDOENC_EVBR_INFO *pEVbrInfo)
{
	if (!pEVbrInfo){
		DBG_ERR("[VDOENC][%d] Set EVBR Info is NULL\r\n", pathID);
		return;
	}

#if MP_VDOENC_SHOW_MSG
	if (gNMRVdoEncObj[pathID].bStart && pEVbrInfo->enable) {
		DBG_IND("[VDOENC][%d] Set EVBR Enable = %d, StaticTime = %d, ByteRate = %d, Fr = %u.%03u, GOP = %d, KeyPPeriod = %d, IQp = (%d, %d, %d), PQp = (%d, %d, %d), IPWei = %d, KPWei = %d, MotionAQ = %d, StillFr = %d, MotionR = %d, Psnr = (%d, %d, %d), P2_Wei = %d, P3_Wei = %d, LT_Wei = %d, max_size = %d\r\n",
				pathID,
				pEVbrInfo->enable,
				pEVbrInfo->static_time,
				pEVbrInfo->byte_rate,
				(pEVbrInfo->frame_rate/1000),
				(pEVbrInfo->frame_rate%1000),
				pEVbrInfo->gop,
				pEVbrInfo->key_p_period,
				pEVbrInfo->init_i_qp,
				pEVbrInfo->min_i_qp,
				pEVbrInfo->max_i_qp,
				pEVbrInfo->init_p_qp,
				pEVbrInfo->min_p_qp,
				pEVbrInfo->max_p_qp,
				pEVbrInfo->ip_weight,
				pEVbrInfo->kp_weight,
				pEVbrInfo->motion_aq_st,
				pEVbrInfo->still_frm_cnd,
				pEVbrInfo->motion_ratio_thd,
				pEVbrInfo->i_psnr_cnd,
				pEVbrInfo->p_psnr_cnd,
				pEVbrInfo->kp_psnr_cnd,
				pEVbrInfo->p2_weight,
				pEVbrInfo->p3_weight,
				pEVbrInfo->lt_weight,
				pEVbrInfo->max_frame_size);
	}
#endif
	{
		UINT32 svc_weight_mode = gNMRVdoEncObj[pathID].EVbrInfo.svc_weight_mode; // backup
		UINT32 br_tolerance = gNMRVdoEncObj[pathID].EVbrInfo.br_tolerance; // backup
		memcpy((void *)&gNMRVdoEncObj[pathID].EVbrInfo, (void *)pEVbrInfo, sizeof(MP_VDOENC_EVBR_INFO));
		gNMRVdoEncObj[pathID].EVbrInfo.svc_weight_mode = svc_weight_mode; // restore
		gNMRVdoEncObj[pathID].EVbrInfo.br_tolerance = br_tolerance; // restore
	}
	if (gNMRVdoEncObj[pathID].EVbrInfo.enable) {
		gNMRVdoEncObj[pathID].CbrInfo.enable = 0;
		gNMRVdoEncObj[pathID].VbrInfo.enable = 0;
		gNMRVdoEncObj[pathID].FixQpInfo.enable = 0;

		if (gNMRVdoEncObj[pathID].bStart &&
			(/*gNMRVdoEncObj[pathID].InitInfo.gop != gNMRVdoEncObj[pathID].EVbrInfo.gop ||*/
			gNMRVdoEncObj[pathID].InitInfo.frame_rate != gNMRVdoEncObj[pathID].EVbrInfo.frame_rate)) {
			gNMRVdoEncObj[pathID].bResetIFrame = TRUE;
		}

		//gNMRVdoEncObj[pathID].InitInfo.gop         = gNMRVdoEncObj[pathID].EVbrInfo.gop;   // maybe encode GOP != rc GOP now, "K-customer" want to do dynamic gop via resetI by themself
		gNMRVdoEncObj[pathID].InitInfo.frame_rate  = gNMRVdoEncObj[pathID].EVbrInfo.frame_rate;
		gNMRVdoEncObj[pathID].InitInfo.byte_rate   = gNMRVdoEncObj[pathID].EVbrInfo.byte_rate;
		gNMRVdoEncObj[pathID].InitInfo.init_i_qp   = gNMRVdoEncObj[pathID].EVbrInfo.init_i_qp;
		gNMRVdoEncObj[pathID].InitInfo.min_i_qp    = gNMRVdoEncObj[pathID].EVbrInfo.min_i_qp;
		gNMRVdoEncObj[pathID].InitInfo.max_i_qp    = gNMRVdoEncObj[pathID].EVbrInfo.max_i_qp;
		gNMRVdoEncObj[pathID].InitInfo.init_p_qp   = gNMRVdoEncObj[pathID].EVbrInfo.init_p_qp;
		gNMRVdoEncObj[pathID].InitInfo.min_p_qp    = gNMRVdoEncObj[pathID].EVbrInfo.min_p_qp;
		gNMRVdoEncObj[pathID].InitInfo.max_p_qp    = gNMRVdoEncObj[pathID].EVbrInfo.max_p_qp;
		gNMRVdoEncObj[pathID].InitInfo.static_time = gNMRVdoEncObj[pathID].EVbrInfo.static_time;
		gNMRVdoEncObj[pathID].InitInfo.ip_weight   = gNMRVdoEncObj[pathID].EVbrInfo.ip_weight;

#ifdef __KERNEL__
		// JIRA: IVOT_N12006_CO-137, for fastboot dynamic byterate setting
		VdoEnc_Builtin_SetParam(pathID, BUILTIN_VDOENC_INIT_PARAM_DYNAMIC_BYTERATE, gNMRVdoEncObj[pathID].EVbrInfo.byte_rate);
#endif
	}

	gNMRVdoEncObj[pathID].uiSetRCMode = NMR_VDOENC_RC_EVBR;
}

/*
    Set encoding parameters to video codec library.

    Set encoding parameters to video codec library before encoding one video frame.

    @param void

    @return void
*/
void NMR_VdoEnc_SetEncodeParam(UINT32 pathID)
{
	MP_VDOENC_ENCODER *pVidEnc = &gNMRVdoTrigObj[pathID].Encoder;
	UINT32 i = 0;

	if (gNMRVdoEncObj[pathID].InitInfo.width == 0 || gNMRVdoEncObj[pathID].InitInfo.height == 0) {
		DBG_ERR("[VDOENC][%d] SetEncodeParam invalid param, wid = %d, hei = %d\r\n",
				pathID, gNMRVdoEncObj[pathID].InitInfo.width, gNMRVdoEncObj[pathID].InitInfo.height);
		return;
	}
	gNMRVdoEncObj[pathID].InitInfo.bFBCEn = 1; //JIRA : IVOT_N01002_CO-686. Set h26x as always RecCompress, and source_out require to alloc extra buffer.
	gNMRVdoEncObj[pathID].InitInfo.user_qp_en = 0; //tmp for md, // 0 : 16x16, 1 : 32x32, 2 : 64x64
	gNMRVdoEncObj[pathID].InitInfo.buf_addr = gNMRVdoEncObj[pathID].MemInfo.uiVCodecAddr;
	gNMRVdoEncObj[pathID].InitInfo.buf_size = gNMRVdoEncObj[pathID].MemInfo.uiVCodecSize;
	//reconstruct buffer
	for (i = 0; i < gNMRVdoEncObj[pathID].MemInfo.uiRecBuffNum; i++) {
		gNMRVdoEncObj[pathID].InitInfo.recfrm_addr[i] = gNMRVdoEncObj[pathID].MemInfo.uiRecBuffAddr[i];
	}
	gNMRVdoEncObj[pathID].InitInfo.recfrm_size = gNMRVdoEncObj[pathID].MemInfo.uiRecBuffSize;
	gNMRVdoEncObj[pathID].InitInfo.recfrm_num = gNMRVdoEncObj[pathID].MemInfo.uiRecBuffNum;

	if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_MJPG) {
		switch (gNMRVdoEncJpgYuvFormat[pathID]) {
			case VDO_PXLFMT_Y8:
				gNMRVdoEncObj[pathID].InitInfo.jpeg_yuv_format = JPG_FMT_YUV100;
				break;
			case VDO_PXLFMT_YUV420:
				gNMRVdoEncObj[pathID].InitInfo.jpeg_yuv_format = JPG_FMT_YUV420;
				break;
			case VDO_PXLFMT_YUV422:
				gNMRVdoEncObj[pathID].InitInfo.jpeg_yuv_format = JPG_FMT_YUV422;
				break;
			default:
				gNMRVdoEncObj[pathID].InitInfo.jpeg_yuv_format = JPG_FMT_YUV420;
				DBG_ERR("unknown YuvFormat(%d) !! set as default YUV420 !!\r\n", gNMRVdoEncJpgYuvFormat[pathID]);
				break;
		}
	}
#if defined(_BSP_NA51000_)
	if (gNMRVdoEncObj[pathID].uiRecFormat == MEDIAREC_LIVEVIEW) {
#elif defined(_BSP_NA51055_)
	if (1) {    // [520] doesn't support (fast_search = 1) => will encode fail with encode error interrupt(0x00000200)
#endif
		gNMRVdoEncObj[pathID].InitInfo.fast_search = 0;
		gNMRVdoEncObj[pathID].InitInfo.project_mode = 0;
	} else {
		gNMRVdoEncObj[pathID].InitInfo.fast_search = 1;
		gNMRVdoEncObj[pathID].InitInfo.project_mode = 1;
	}
	if (gNMRVdoEncObj[pathID].bTvRange) {
		gNMRVdoEncObj[pathID].InitInfo.color_range = 0;
	} else {
		gNMRVdoEncObj[pathID].InitInfo.color_range = 1;
	}

	// [520] param
	if ((gNMRVdoEncObj[pathID].InitInfo.rotate == 1) || (gNMRVdoEncObj[pathID].InitInfo.rotate == 2)) {
		// rotate 90/270, we check height
		if (gNMRVdoEncObj[pathID].InitInfo.bD2dEn) {
			if (g_vdoenc_gdc_first) {
				gNMRVdoEncObj[pathID].InitInfo.bTileEn = (gNMRVdoEncObj[pathID].InitInfo.height <  _NMR_VdoEnc_Get_H265_GDC_MIN_W_1_TILE())? 0:1;  // LowLatency = ON, we MUST tile = 1 for height <  1280 for GDC first
			} else {
				gNMRVdoEncObj[pathID].InitInfo.bTileEn = (gNMRVdoEncObj[pathID].InitInfo.height <= _NMR_VdoEnc_Get_H265_D2D_MIN_W_1_TILE())? 0:1;  // LowLatency = ON, we MUST tile = 1 for height <= 2048 for LL first
			}
		}else{
#if defined(CONFIG_NVT_SMALL_HDAL)
			gNMRVdoEncObj[pathID].InitInfo.quality_level = KDRV_VDOENC_QUALITY_BASE;
#else
			if (gNMRVdoEncObj[pathID].bQualityBase) {
				gNMRVdoEncObj[pathID].InitInfo.quality_level = KDRV_VDOENC_QUALITY_BASE;
			} else {
				gNMRVdoEncObj[pathID].InitInfo.quality_level = KDRV_VDOENC_QUALITY_MAIN;
			}
#endif
		}
	}else{
		// rotate  0/180, we check width
		if (gNMRVdoEncObj[pathID].InitInfo.bD2dEn) {
			if (g_vdoenc_gdc_first) {
				gNMRVdoEncObj[pathID].InitInfo.bTileEn = (gNMRVdoEncObj[pathID].InitInfo.width <  _NMR_VdoEnc_Get_H265_GDC_MIN_W_1_TILE())? 0:1;   // LowLatency = ON, we MUST tile = 1 for width <  1280 for GDC first
			} else {
				gNMRVdoEncObj[pathID].InitInfo.bTileEn = (gNMRVdoEncObj[pathID].InitInfo.width <= _NMR_VdoEnc_Get_H265_D2D_MIN_W_1_TILE())? 0:1;   // LowLatency = ON, we MUST tile = 1 for width <= 2048 for LL first
			}
		}else{
#if defined(CONFIG_NVT_SMALL_HDAL)
			gNMRVdoEncObj[pathID].InitInfo.quality_level = KDRV_VDOENC_QUALITY_BASE;
#else
			if (gNMRVdoEncObj[pathID].bQualityBase) {
				gNMRVdoEncObj[pathID].InitInfo.quality_level = KDRV_VDOENC_QUALITY_BASE;
			} else {
				gNMRVdoEncObj[pathID].InitInfo.quality_level = KDRV_VDOENC_QUALITY_MAIN;
			}

#endif

		}
	}

	//gNMRVdoEncObj[pathID].InitInfo.bD2dEn          = gNMRVdoEncObj[pathID].InitInfo.bD2dEn;
	gNMRVdoEncObj[pathID].InitInfo.gdc_mode_en     = g_vdoenc_gdc_first;
	if (gNMRVdoEncObj[pathID].uiEncPaddingMode == NMR_VDOENC_PADDING_MODE_ZERO) { // [IVOT_N12010_CO-640] vendor command for setting padding mode
		gNMRVdoEncObj[pathID].InitInfo.hw_padding_en   = 1;   // Padding zero mode(0 : normal, 1 : use 0 for padding ) => [IVOT_N00025-518] modify to padding zero with hardcode.
	} else if (gNMRVdoEncObj[pathID].uiEncPaddingMode == NMR_VDOENC_PADDING_MODE_COPY) {
		gNMRVdoEncObj[pathID].InitInfo.hw_padding_en   = 0;
	}
	gNMRVdoEncObj[pathID].InitInfo.sao_en          = 1;
	gNMRVdoEncObj[pathID].InitInfo.sao_luma_flag   = 1;
	gNMRVdoEncObj[pathID].InitInfo.sao_chroma_flag = 1;
#ifdef __KERNEL__
	if (kdrv_builtin_is_fastboot() && pathID < BUILTIN_VDOENC_PATH_ID_MAX) {
		BOOL bBuiltinEn = 0;
		VdoEnc_Builtin_GetParam(pathID, BUILTIN_VDOENC_INIT_PARAM_ENC_EN, &bBuiltinEn);
		gNMRVdoEncObj[pathID].InitInfo.builtin_init = bBuiltinEn ? 1 : 0;
	} else {
		gNMRVdoEncObj[pathID].InitInfo.builtin_init = 0;
	}
#endif
	if (pVidEnc->SetInfo == 0) {
		DBG_ERR("[VDOENC][%d] SetEncodeParam need media encode SetInfo function\r\n", pathID);
		return;
	}

	//initialize
	(pVidEnc->Initialize)(&gNMRVdoEncObj[pathID].InitInfo);  // [520] have to Init before any set()...so move up

	if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_MJPG) {
		(pVidEnc->SetInfo)(VDOENC_SET_INIT, (UINT32)&gNMRVdoTrigObj[pathID].EncParam, 0, 0);  // JPG driver require kflow to call kdrv_set(INIT) by passing struct KDRV_VDOENC_PARAM, which is same as kdrv_trigger
	}

	(pVidEnc->SetInfo)(VDOENC_SET_AQ, (UINT32)&gNMRVdoEncObj[pathID].AqInfo, 0, 0);
	(pVidEnc->SetInfo)(VDOENC_SET_GDR, (UINT32)&gNMRVdoEncObj[pathID].GdrInfo, 0, 0);
	(pVidEnc->SetInfo)(VDOENC_SET_SLICESPLIT, (UINT32)&gNMRVdoEncObj[pathID].SliceSplitInfo, 0, 0);
	(pVidEnc->SetInfo)(VDOENC_SET_QPMAP, (UINT32)&gNMRVdoEncObj[pathID].QpMapInfo, 0, 0);
	(pVidEnc->SetInfo)(VDOENC_SET_ROWRC, (UINT32)&gNMRVdoEncObj[pathID].RowRcInfo, 0, 0);

	if (gNMRVdoTrigObj[pathID].rdo_en) {
		(pVidEnc->SetInfo)(VDOENC_SET_RDO, (UINT32)&gNMRVdoTrigObj[pathID].rdo_info, 0, 0);
		gNMRVdoTrigObj[pathID].rdo_en = FALSE;
	}

	if (gNMRVdoTrigObj[pathID].jnd_en) {
		(pVidEnc->SetInfo)(VDOENC_SET_JND, (UINT32)&gNMRVdoTrigObj[pathID].jnd_info, 0, 0);
		gNMRVdoTrigObj[pathID].jnd_en = FALSE;
	}

#ifdef __KERNEL__
	if (kdrv_builtin_is_fastboot() && pathID < BUILTIN_VDOENC_PATH_ID_MAX && gNMRVdoEnc1stStart[pathID]) {
		if (gNMRVdoEncObj[pathID].uiVidCodec != NMEDIAREC_ENC_MJPG) {
			BOOL bSetRC = FALSE;

			if (gNMRVdoEncObj[pathID].CbrInfo.enable) {
				bSetRC = _nmr_vdoenc_compare_builtin_rc(pathID, BUILTIN_VDOENC_RC_CBR, (VOID *)&gNMRVdoEncObj[pathID].CbrInfo);
				if (bSetRC) {
					(pVidEnc->SetInfo)(VDOENC_SET_CBR, (UINT32)&gNMRVdoEncObj[pathID].CbrInfo, pathID, 0);
				}
			}
			if (gNMRVdoEncObj[pathID].EVbrInfo.enable) {
				bSetRC = _nmr_vdoenc_compare_builtin_rc(pathID, BUILTIN_VDOENC_RC_EVBR, (VOID *)&gNMRVdoEncObj[pathID].EVbrInfo);
				if (bSetRC) {
					(pVidEnc->SetInfo)(VDOENC_SET_EVBR, (UINT32)&gNMRVdoEncObj[pathID].EVbrInfo, pathID, 0);
				}
			}
			if (gNMRVdoEncObj[pathID].VbrInfo.enable) {
				bSetRC = _nmr_vdoenc_compare_builtin_rc(pathID, BUILTIN_VDOENC_RC_VBR, (VOID *)&gNMRVdoEncObj[pathID].VbrInfo);
				if (bSetRC) {
					(pVidEnc->SetInfo)(VDOENC_SET_VBR, (UINT32)&gNMRVdoEncObj[pathID].VbrInfo, pathID, 0);
				}
			}
			if (gNMRVdoEncObj[pathID].FixQpInfo.enable) {
				bSetRC = _nmr_vdoenc_compare_builtin_rc(pathID, BUILTIN_VDOENC_RC_FIXQP, (VOID *)&gNMRVdoEncObj[pathID].FixQpInfo);
				if (bSetRC) {
					(pVidEnc->SetInfo)(VDOENC_SET_FIXQP, (UINT32)&gNMRVdoEncObj[pathID].FixQpInfo, pathID, 0);
				}
			}
		}
	} else {
#endif


	if (gNMRVdoEncObj[pathID].CbrInfo.enable)
		(pVidEnc->SetInfo)(VDOENC_SET_CBR, (UINT32)&gNMRVdoEncObj[pathID].CbrInfo, pathID, 0);
	if (gNMRVdoEncObj[pathID].EVbrInfo.enable)
		(pVidEnc->SetInfo)(VDOENC_SET_EVBR, (UINT32)&gNMRVdoEncObj[pathID].EVbrInfo, pathID, 0);
	if (gNMRVdoEncObj[pathID].VbrInfo.enable)
		(pVidEnc->SetInfo)(VDOENC_SET_VBR, (UINT32)&gNMRVdoEncObj[pathID].VbrInfo, pathID, 0);
	if (gNMRVdoEncObj[pathID].FixQpInfo.enable)
		(pVidEnc->SetInfo)(VDOENC_SET_FIXQP, (UINT32)&gNMRVdoEncObj[pathID].FixQpInfo, pathID, 0);
#ifdef __KERNEL__
	}
#endif
	//gNMRVdoEncObj[pathID].uiSetRCMode = 0;

	// 3DNR CB setting
	if (gNMRVdoEncObj[pathID].v3dnrCb.VdoEnc_3DNRDo) {
		(pVidEnc->SetInfo)(VDOENC_SET_3DNRCB, (UINT32)&gNMRVdoEncObj[pathID].v3dnrCb, 0, 0);
	}

	// ISP ratio CB setting
	if (gNMRVdoEncObj[pathID].ispCb.vdoenc_isp_cb) {
		(pVidEnc->SetInfo)(VDOENC_SET_ISPCB, (UINT32)&gNMRVdoEncObj[pathID].ispCb, 0, 0);
	}

	//set ROI
	(pVidEnc->SetInfo)(VDOENC_SET_ROI, (UINT32)&gNMRVdoEncObj[pathID].RoiInfo, 0, 0);

	//set long start code
	(pVidEnc->SetInfo)(VDOENC_SET_LSC, (UINT32)&gNMRVdoEncObj[pathID].uiLongStartCode, 0, 0);

	//set bs sei check sum
	(pVidEnc->SetInfo)(VDOENC_SET_SEI_BS_CHKSUM, (UINT32)&gNMRVdoEncObj[pathID].bSEIBsChkSum, 0, 0);

	//set low power
	(pVidEnc->SetInfo)(VDOENC_SET_LPM, (UINT32)&gNMRVdoEncObj[pathID].bLowPower, 0, 0);

	// MAQ Diff setting
	if (gNMRVdoEncObj[pathID].MaqDiffInfo.enable) {
		(pVidEnc->SetInfo)(VDOENC_SET_MAQDIFF, (UINT32)&gNMRVdoEncObj[pathID].MaqDiffInfo, 0, 0);
	}

#ifdef VDOENC_LL
	// OSD and Mask CB setting
	if (gNMRVdoEncObj[pathID].osdmaskCb.vdoenc_osdmask_cb) {
		(pVidEnc->SetInfo)(VDOENC_SET_OSDMASKCB, (UINT32)&gNMRVdoEncObj[pathID].osdmaskCb, 0, 0);
	}
#endif

	// remove if JPEG re-trigger ready
	if (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_MJPG && gNMRVdoEncObj[pathID].MemInfo.bBSRingMode)
	{
		DBG_WRN("[VDOENC][%d] JPEG re-trigger not ready, force BS ring mode disable\r\n", pathID);
		gNMRVdoEncObj[pathID].MemInfo.bBSRingMode = 0;
	}

	gNMRVdoTrigObj[pathID].EncParam.nxt_frm_type = NMR_VDOENC_IDR_SLICE;
}

void NMR_VdoEnc_SetFixQP(UINT32 pathID, MP_VDOENC_FIXQP_INFO *pFixQpInfo)
{
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);

	if (!pFixQpInfo){
		DBG_ERR("[VDOENC][%d] Set FIXQP Info is NULL\r\n", pathID);
		return;
	}

#if MP_VDOENC_SHOW_MSG
	if (gNMRVdoEncObj[pathID].bStart && pFixQpInfo->enable) {
		DBG_IND("[VDOENC][%d] Set FIXQP Enable = %d, IFixQP = %d, PFixQP = %d\r\n",
				 pathID,
				 pFixQpInfo->enable,
				 pFixQpInfo->fix_i_qp,
				 pFixQpInfo->fix_p_qp);
	}
#endif

	memcpy((void *)&gNMRVdoEncObj[pathID].FixQpInfo, (void *)pFixQpInfo, sizeof(MP_VDOENC_FIXQP_INFO));

	if (gNMRVdoEncObj[pathID].FixQpInfo.enable) {
		gNMRVdoEncObj[pathID].CbrInfo.enable = 0;
		gNMRVdoEncObj[pathID].EVbrInfo.enable = 0;
		gNMRVdoEncObj[pathID].VbrInfo.enable = 0;

		gNMRVdoEncObj[pathID].InitInfo.frame_rate = gNMRVdoEncObj[pathID].FixQpInfo.frame_rate; // Let UART print Fps , but no use in fact
	}

	if (pVidEnc->SetInfo) {
		(pVidEnc->SetInfo)(VDOENC_SET_FIXQP, (UINT32)&gNMRVdoEncObj[pathID].FixQpInfo, 0, 0);
	}
}

void NMR_VdoEnc_SetROI(UINT32 pathID, MP_VDOENC_ROI_INFO *pRoiInfo)
{
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);

	if (!pRoiInfo){
		DBG_ERR("[VDOENC][%d] Set ROI Info is NULL\r\n", pathID);
		return;
	}

	if (pRoiInfo){
		memcpy((void *)&gNMRVdoEncObj[pathID].RoiInfo, (void *)pRoiInfo, sizeof(MP_VDOENC_ROI_INFO));
	}

	if (pVidEnc->SetInfo) {
		(pVidEnc->SetInfo)(VDOENC_SET_ROI, (UINT32)&gNMRVdoEncObj[pathID].RoiInfo, 0, 0);
	}
}

void NMR_VdoEnc_SetSmartROI(UINT32 pathID, UINT32 Enable)
{
	if (pathID >= NMR_VDOENC_MAX_PATH) {
		DBG_ERR("[VDOENC] smart roi, invalid path index = %d\r\n", pathID);
		return;
	}

	DBG_DUMP("[VDOENC][%d] Set SmartRoi = %d\r\n", pathID, Enable);
	gNMRVdoEncObj[pathID].bStartSmartRoi = Enable;
}

void NMR_VdoEnc_SetVBR(UINT32 pathID, MP_VDOENC_VBR_INFO *pVbrInfo)
{
	if (!pVbrInfo){
		DBG_ERR("[VDOENC][%d] Set VBR Info is NULL\r\n", pathID);
		return;
	}

#if MP_VDOENC_SHOW_MSG
	if (gNMRVdoEncObj[pathID].bStart && pVbrInfo->enable) {
		DBG_IND("[VDOENC][%d] Set VBR Enable = %d, StaticTime = %d, ByteRate = %d, Fr = %u.%03u, GOP = %d, IQp = (%d, %d, %d), PQp = (%d, %d, %d), IPWei = %d, ChangePos = %d, KP_Prd = %d, KP_Wei = %d, P2_Wei = %d, P3_Wei = %d, LT_Wei = %d, mas = %d, max_size = %d\r\n",
				pathID,
				pVbrInfo->enable,
				pVbrInfo->static_time,
				pVbrInfo->byte_rate,
				(pVbrInfo->frame_rate/1000),
				(pVbrInfo->frame_rate%1000),
				pVbrInfo->gop,
				pVbrInfo->init_i_qp,
				pVbrInfo->min_i_qp,
				pVbrInfo->max_i_qp,
				pVbrInfo->init_p_qp,
				pVbrInfo->min_p_qp,
				pVbrInfo->max_p_qp,
				pVbrInfo->ip_weight,
				pVbrInfo->change_pos,
				pVbrInfo->key_p_period,
				pVbrInfo->kp_weight,
				pVbrInfo->p2_weight,
				pVbrInfo->p3_weight,
				pVbrInfo->lt_weight,
				pVbrInfo->motion_aq_str,
				pVbrInfo->max_frame_size);
	}
#endif
	{
		UINT32 vbr_policy = gNMRVdoEncObj[pathID].VbrInfo.policy; // backup
		UINT32 svc_weight_mode = gNMRVdoEncObj[pathID].VbrInfo.svc_weight_mode; // backup
		UINT32 br_tolerance = gNMRVdoEncObj[pathID].VbrInfo.br_tolerance; // backup
		memcpy((void *)&gNMRVdoEncObj[pathID].VbrInfo, (void *)pVbrInfo, sizeof(MP_VDOENC_VBR_INFO)); // copy all other VBR settings from user
		gNMRVdoEncObj[pathID].VbrInfo.policy = vbr_policy; // restore
		gNMRVdoEncObj[pathID].VbrInfo.svc_weight_mode = svc_weight_mode; // restore
		gNMRVdoEncObj[pathID].VbrInfo.br_tolerance = br_tolerance; // restore
	}
	if (gNMRVdoEncObj[pathID].VbrInfo.enable) {
		gNMRVdoEncObj[pathID].CbrInfo.enable = 0;
		gNMRVdoEncObj[pathID].EVbrInfo.enable = 0;
		gNMRVdoEncObj[pathID].FixQpInfo.enable = 0;

		if (gNMRVdoEncObj[pathID].bStart &&
			(/*gNMRVdoEncObj[pathID].InitInfo.gop != gNMRVdoEncObj[pathID].VbrInfo.gop ||*/
			gNMRVdoEncObj[pathID].InitInfo.frame_rate != gNMRVdoEncObj[pathID].VbrInfo.frame_rate)) {
			gNMRVdoEncObj[pathID].bResetIFrame = TRUE;
		}

		//gNMRVdoEncObj[pathID].InitInfo.gop            = gNMRVdoEncObj[pathID].VbrInfo.gop;  // maybe encode GOP != rc GOP now, "K-customer" want to do dynamic gop via resetI by themself
		gNMRVdoEncObj[pathID].InitInfo.frame_rate     = gNMRVdoEncObj[pathID].VbrInfo.frame_rate;
		gNMRVdoEncObj[pathID].InitInfo.byte_rate      = gNMRVdoEncObj[pathID].VbrInfo.byte_rate;
		gNMRVdoEncObj[pathID].InitInfo.init_i_qp      = gNMRVdoEncObj[pathID].VbrInfo.init_i_qp;
		gNMRVdoEncObj[pathID].InitInfo.min_i_qp       = gNMRVdoEncObj[pathID].VbrInfo.min_i_qp;
		gNMRVdoEncObj[pathID].InitInfo.max_i_qp       = gNMRVdoEncObj[pathID].VbrInfo.max_i_qp;
		gNMRVdoEncObj[pathID].InitInfo.init_p_qp      = gNMRVdoEncObj[pathID].VbrInfo.init_p_qp;
		gNMRVdoEncObj[pathID].InitInfo.min_p_qp       = gNMRVdoEncObj[pathID].VbrInfo.min_p_qp;
		gNMRVdoEncObj[pathID].InitInfo.max_p_qp       = gNMRVdoEncObj[pathID].VbrInfo.max_p_qp;
		gNMRVdoEncObj[pathID].InitInfo.static_time    = gNMRVdoEncObj[pathID].VbrInfo.static_time;
		gNMRVdoEncObj[pathID].InitInfo.ip_weight      = gNMRVdoEncObj[pathID].VbrInfo.ip_weight;

#ifdef __KERNEL__
		// JIRA: IVOT_N12006_CO-137, for fastboot dynamic byterate setting
		VdoEnc_Builtin_SetParam(pathID, BUILTIN_VDOENC_INIT_PARAM_DYNAMIC_BYTERATE, gNMRVdoEncObj[pathID].VbrInfo.byte_rate);
#endif
	}

	gNMRVdoEncObj[pathID].uiSetRCMode = NMR_VDOENC_RC_VBR;
}

void NMR_VdoEnc_SetRowRc(UINT32 pathID, MP_VDOENC_ROWRC_INFO *pRowRcInfo)
{
	MP_VDOENC_ENCODER *pVidEnc = NMR_VdoTrig_GetVidEncoder(pathID);

	if (!pRowRcInfo){
		DBG_ERR("[VDOENC][%d] Set RowRc Info is NULL\r\n", pathID);
		return;
	}

#if MP_VDOENC_SHOW_MSG
	if (gNMRVdoEncObj[pathID].bStart && pRowRcInfo->enable) {
		DBG_DUMP("[VDOENC][%d] set RowRC, en = %u, (range, step, min, max)=> I = (%u, %u, %u, %u), P = (%u, %u, %u, %u)\r\n",
			pathID,
			pRowRcInfo->enable,
			pRowRcInfo->i_qp_range,
			pRowRcInfo->i_qp_step,
			pRowRcInfo->min_i_qp,
			pRowRcInfo->max_i_qp,
			pRowRcInfo->p_qp_range,
			pRowRcInfo->p_qp_step,
			pRowRcInfo->min_p_qp,
			pRowRcInfo->max_p_qp);
	}
#endif

	memcpy((void *)&gNMRVdoEncObj[pathID].RowRcInfo, (void *)pRowRcInfo, sizeof(MP_VDOENC_ROWRC_INFO));


	if (pVidEnc->SetInfo) {
		(pVidEnc->SetInfo)(VDOENC_SET_ROWRC, (UINT32)&gNMRVdoEncObj[pathID].RowRcInfo, 0, 0);
	}
}

#ifdef __KERNEL__
void NMR_VdoEnc_GetBuiltinBsData(UINT32 pathID)
{
	NMR_VDOTRIG_OBJ 			*pVidTrigObj;
	NMI_VDOENC_BS_INFO          vidBSinfo = {0};
	UINT32						IsIDR2Cut = 0;
	BOOL						bBuiltinEn = 0;
	UINT32             total_size = 0;

	pVidTrigObj =  &(gNMRVdoTrigObj[pathID]);
	VdoEnc_Builtin_GetParam(pathID, BUILTIN_VDOENC_INIT_PARAM_ENC_EN, &bBuiltinEn);

	if (VdoEnc_Builtin_HowManyInBSQ(pathID) == 0) {
		return;
	}

	if (kdrv_builtin_is_fastboot() && bBuiltinEn) {
		VDOENC_BUILTIN_BS_INFO builtinBSinfo = {0};

		while (VdoEnc_Builtin_HowManyInBSQ(pathID) > 0) {
			if (gNMRVdoEncObj[pathID].CallBackFunc) {
				vidBSinfo.PathID = pathID;
				vidBSinfo.BufID = 0;/*check streamReadyCB*/;
				vidBSinfo.Occupied = 1;

				if (pVidTrigObj->SyncFrameN != 0 && pVidTrigObj->FrameCount % pVidTrigObj->SyncFrameN == 0) {
					IsIDR2Cut = 1;
				}

				VdoEnc_Builtin_GetBS(pathID, &builtinBSinfo);

				vidBSinfo.IsKey = builtinBSinfo.isKeyFrame;
				if (vidBSinfo.IsKey) {
					vidBSinfo.Addr = builtinBSinfo.Addr + gNMRVdoEncObj[pathID].uiDescSize;
					vidBSinfo.Size = builtinBSinfo.Size - gNMRVdoEncObj[pathID].uiDescSize;
				} else {
					vidBSinfo.Addr = builtinBSinfo.Addr;
					vidBSinfo.Size = builtinBSinfo.Size;
				}
				vidBSinfo.IsIDR2Cut = IsIDR2Cut;
				vidBSinfo.RawYAddr	 = 0;//pVidEncParam->raw_y_addr;
				vidBSinfo.SVCSize	 = 0;//pVidEncParam->svc_hdr_size;
				vidBSinfo.TimeStamp = (UINT64)builtinBSinfo.timestamp;
				vidBSinfo.TemporalId = builtinBSinfo.temproal_id;
				vidBSinfo.FrameType  = builtinBSinfo.frm_type;
				vidBSinfo.uiBaseQP	 = builtinBSinfo.base_qp;
				vidBSinfo.nalu_info_addr = ALIGN_CEIL_64(builtinBSinfo.Addr + builtinBSinfo.Size); // this must sync with following

				if ((gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H264) || (gNMRVdoEncObj[pathID].uiVidCodec == NMEDIAREC_ENC_H265))
				{
					UINT32 idx = 0;
					UINT32 bs_addr = 0;
					UINT32 *nalu_len_addr = (UINT32*)builtinBSinfo.nalu_size_addr;
					UINT32 nalu_num = builtinBSinfo.nalu_num;

					total_size = 0;
					// nalu_num
					*((UINT32 *)(ALIGN_CEIL_64(builtinBSinfo.Addr + builtinBSinfo.Size) + total_size)) = nalu_num;
					total_size += sizeof(UINT32);

					// nalu addr/size
					if (gNMRVdoEncObj[pathID].CallBackFunc) {
						bs_addr = (gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_GET_BS_START_PA, (UINT32) &vidBSinfo);
					}

					for (idx=0; idx<nalu_num; idx++) {
						// nalu_addr
						*((UINT32 *)(ALIGN_CEIL_64(builtinBSinfo.Addr + builtinBSinfo.Size) + total_size)) = bs_addr;
						total_size += sizeof(UINT32);

						// nalu_size
						if ((builtinBSinfo.isKeyFrame) && (idx==0)) {
							*((UINT32 *)(ALIGN_CEIL_64(builtinBSinfo.Addr + builtinBSinfo.Size) + total_size)) = (nalu_len_addr[idx] - gNMRVdoEncObj[pathID].uiDescSize);
							bs_addr += (nalu_len_addr[idx] - gNMRVdoEncObj[pathID].uiDescSize); // update bs_addr
						}else{
							*((UINT32 *)(ALIGN_CEIL_64(builtinBSinfo.Addr + builtinBSinfo.Size) + total_size)) = nalu_len_addr[idx];
							bs_addr += nalu_len_addr[idx]; // update bs_addr
						}
						total_size += sizeof(UINT32);
					}
					VdoEnc_Builtin_SetParam(pathID, BUILTIN_VDOENC_INIT_PARAM_FREE_MEM, builtinBSinfo.nalu_size_addr);

					total_size = ALIGN_CEIL_64(total_size);

					// flush cache
#if 1  // don't need to flush, because we let nalu_addr be 64x , and size be 64x  => (Update) with BS_QUICK_ROLLBACK maybe wrong, so enable again
					vos_cpu_dcache_sync((UINT32)(ALIGN_CEIL_64(builtinBSinfo.Addr + builtinBSinfo.Size)), total_size, VOS_DMA_TO_DEVICE);
#endif
				} else {
					UINT32 bs_addr = 0;

					total_size = 0;
					if (gNMRVdoEncObj[pathID].CallBackFunc) {
						bs_addr = (gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_GET_BS_START_PA, (UINT32) &vidBSinfo);
					}

					// nalu_num
					*((UINT32 *)(ALIGN_CEIL_64(builtinBSinfo.Addr + builtinBSinfo.Size) + total_size)) = 1;
					total_size += sizeof(UINT32);

					// nalu_addr
					*((UINT32 *)(ALIGN_CEIL_64(builtinBSinfo.Addr + builtinBSinfo.Size) + total_size)) = bs_addr;
					total_size += sizeof(UINT32);

					// nalu_size
					*((UINT32 *)(ALIGN_CEIL_64(builtinBSinfo.Addr + builtinBSinfo.Size) + total_size)) = vidBSinfo.Size;
					total_size += sizeof(UINT32);

					total_size = ALIGN_CEIL_64(total_size);

					// flush cache
#if 1  // don't need to flush, because we let nalu_addr be 64x , and size be 64x  => (Update) with BS_QUICK_ROLLBACK maybe wrong, so enable again
					vos_cpu_dcache_sync((UINT32)(ALIGN_CEIL_64(builtinBSinfo.Addr + builtinBSinfo.Size)), total_size, VOS_DMA_TO_DEVICE);
#endif
				}

				NMR_VdoTrig_LockBS(pathID, vidBSinfo.Addr, vidBSinfo.Size);
				(gNMRVdoEncObj[pathID].CallBackFunc)(NMI_VDOENC_EVENT_BS_CB, (UINT32) &vidBSinfo); // the do_probe REL will do on StreamReadyCB, with (Occupy = 0 & Addr !=0)

				pVidTrigObj->FrameCount++;
				gNMRVdoEncObj[pathID].uiEncOutCount++;
				if (builtinBSinfo.re_encode_en) {
					gNMRVdoEncObj[pathID].uiEncReCount++;
				}
			}
		}
		pVidTrigObj->BsNow = ALIGN_CEIL_64(ALIGN_CEIL_64(builtinBSinfo.Addr + builtinBSinfo.Size) + total_size);

		if (pVidTrigObj->BsNow >= pVidTrigObj->BsMax && pVidTrigObj->WantRollback == FALSE) {
			pVidTrigObj->WantRollback = TRUE;
		}

		if (pVidTrigObj->WantRollback) {
			pVidTrigObj->BsNow = pVidTrigObj->BsStart;
			pVidTrigObj->WantRollback = FALSE;
		}
	}
}
#endif

void NMR_VdoEnc_ShowMsg(UINT32 pathID, UINT32 msgLevel)
{
	if (pathID >= NMR_VDOENC_MAX_PATH) {
		DBG_ERR("[VDOENC] showmsg, invalid path index = %d\r\n", pathID);
		return;
	}
	gNMRVdoEncObj[pathID].uiMsgLevel = msgLevel;

}

/**
    @name   NMediaRec VdoEnc NMI API
*/
static ER NMI_VdoEnc_Open(UINT32 Id, UINT32 *pContext)
{
	ER r = E_OK;

#ifdef VDOENC_LL
	gNMRVdoEncObj[Id].osdmaskCb.id = Id;
	gNMRVdoEncObj[Id].osdmaskCb.vdoenc_osdmask_cb = (MEDIAREC_OSDMASKCB) _NMR_VdoEnc_OSD_MASK;
#endif

	gNMRVdoEncPathCount++;
	if (gNMRVdoEncPathCount == 1) {
		//create enc & trig tsk first (h26x)
		if ((r = _NMR_VdoEnc_Open_H26X()) != E_OK) {
			goto _OPEN_H26X_FAIL;
		}
		NMR_VdoTrig_InitJobQ_H26X();
		NMR_VdoTrig_InitJobQ_H26X_Snapshot();

		//create enc & trig tsk first (jpeg)
		if ((r = _NMR_VdoEnc_Open_JPEG()) != E_OK) {
			goto _OPEN_JPEG_FAIL;
		}
		NMR_VdoTrig_InitJobQ_JPEG();

#ifdef VDOENC_LL
		if ((r = _NMR_VdoEnc_Open_Wrap()) != E_OK) {
			goto _OPEN_WRAP_FAIL;
		}
		NMR_VdoTrig_InitJobQ_WrapBS();
#endif
	}
	return E_OK;

#ifdef VDOENC_LL
_OPEN_WRAP_FAIL:
	_NMR_VdoEnc_Close_JPEG();
#endif
_OPEN_JPEG_FAIL:
	_NMR_VdoEnc_Close_H26X();
_OPEN_H26X_FAIL:
	return r;
}

static ER NMI_VdoEnc_SetParam(UINT32 Id, UINT32 Param, UINT32 Value)
{
	switch (Param) {
	case NMI_VDOENC_PARAM_UNLOCK_BS_ADDR:
		NMR_VdoTrig_UnlockBS(Id, Value);
		_NMR_VdoTrig_YUV_TMOUT_ResourceUnlock(Id);
		break;

	case NMI_VDOENC_PARAM_CANCEL_H26X_TASK:
		NMR_VdoTrig_CancelH26xTask();
		break;

	case NMI_VDOENC_PARAM_ROI:
	{
		if (Value != 0){
			memcpy((void *)&gNMRVdoEncObj[Id].RoiInfo, (void *)Value, sizeof(MP_VDOENC_ROI_INFO));
			if (gNMRVdoEncObj[Id].bStart == TRUE && gNMRVdoTrigObj[Id].Encoder.SetInfo != 0) {
				(gNMRVdoTrigObj[Id].Encoder.SetInfo)(VDOENC_SET_ROI, Value, 0, 0);
			}
		} else {
			DBG_ERR("[VDOENC][%d] null ROI info\r\n", Id);
		}
		break;
	}

	case NMI_VDOENC_PARAM_SMART_ROI:
	{
		MP_VDOENC_SMART_ROI_INFO* pSmartROI = (MP_VDOENC_SMART_ROI_INFO*) Value;
		if (gNMRVdoEncObj[Id].bWaitSmartRoi == TRUE && pSmartROI != 0){
            NMR_VdoTrig_PutSmartRoi(Id, pSmartROI);
			NMR_VdoTrig_TrigEncode(Id);
		} else {
			DBG_ERR("[VDOENC][%d] invalid operation WaitSmartRoi = %d, SmartROI info = 0x%08x\r\n", Id, gNMRVdoEncObj[Id].bWaitSmartRoi, (UINT32)pSmartROI);
		}
		break;
	}

	case NMI_VDOENC_PARAM_DROP_FRAME:
		gNMRVdoEncObj[Id].uiEncDropCount++;
		break;

	case NMI_VDOENC_PARAM_SKIP_FRAME:
		gNMRVdoEncObj[Id].uiSkipFrame = Value;
		break;

	case NMI_VDOENC_PARAM_ENCODER_OBJ:
		gNMRVdoEncObj[Id].pEncoder = (MP_VDOENC_ENCODER *)Value;
		break;

	case NMI_VDOENC_PARAM_MEM_RANGE:
		if (Value != 0) {
			NMI_VDOENC_MEM_RANGE *pMem = (NMI_VDOENC_MEM_RANGE *) Value;
			if (pMem->Addr == 0 || pMem->Size == 0) {
				gNMRVdoEncObj[Id].MemInfo.uiBSStart = 0;
				DBG_ERR("[VDOENC][%d] invalid mem range, addr = 0x%08x, size = %d\r\n", Id, pMem->Addr, pMem->Size);
				return E_PAR;
			}
			if (_NMR_VdoEnc_SetSaveBuf(Id, pMem->Addr, pMem->Size) != E_OK) {
				DBG_ERR("[VDOENC][%d] set mem range error\r\n", Id);
				return E_PAR;
			}
		} else {
			DBG_ERR("[VDOENC][%d] Set Mem range invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_MAX_MEM_INFO:
	{
		UINT32 uiSnapShotSize, uiCodecsize, uiEncsize;
		NMI_VDOENC_MAX_MEM_INFO* pMaxMemInfo = (NMI_VDOENC_MAX_MEM_INFO*) Value;
		// backup gNMRVdoEncObj   // JIRA: IVOT_N01002_CO-403 , don't use full NMR_VDOENC_OBJ to backup, this will cause stack size too large....so only backup required variables
		UINT32 uiVidCodec    = gNMRVdoEncObj[Id].uiVidCodec;
		UINT32 uiWidth       = gNMRVdoEncObj[Id].InitInfo.width;
		UINT32 uiHeight      = gNMRVdoEncObj[Id].InitInfo.height;
		UINT32 uiByteRate    = gNMRVdoEncObj[Id].InitInfo.byte_rate;
		UINT32 uiEncBufMs    = gNMRVdoEncObj[Id].MemInfo.uiEncBufMs;
		UINT32 uiRecFormat   = gNMRVdoEncObj[Id].uiRecFormat;
		UINT32 uiSVC         = gNMRVdoEncObj[Id].InitInfo.e_svc;
		UINT32 uiLTRInterval = gNMRVdoEncObj[Id].InitInfo.ltr_interval;
		UINT32 uiRotate      = gNMRVdoEncObj[Id].InitInfo.rotate;
		BOOL bAllocSnapshotBuf = gNMRVdoEncObj[Id].bAllocSnapshotBuf;

		// assign temp value to gNMRVdoEncObj to calculate memory info
		gNMRVdoEncObj[Id].uiVidCodec = pMaxMemInfo->uiCodec;
		gNMRVdoEncObj[Id].InitInfo.width = pMaxMemInfo->uiWidth;
		gNMRVdoEncObj[Id].InitInfo.height = pMaxMemInfo->uiHeight;
		gNMRVdoEncObj[Id].InitInfo.byte_rate = pMaxMemInfo->uiTargetByterate;
		gNMRVdoEncObj[Id].MemInfo.uiEncBufMs = pMaxMemInfo->uiEncBufMs;
		gNMRVdoEncObj[Id].uiRecFormat = pMaxMemInfo->uiRecFormat;
		gNMRVdoEncObj[Id].InitInfo.e_svc = pMaxMemInfo->uiSVCLayer;
		gNMRVdoEncObj[Id].InitInfo.ltr_interval = pMaxMemInfo->uiLTRInterval;
		gNMRVdoEncObj[Id].InitInfo.rotate = pMaxMemInfo->uiRotate;
		gNMRVdoEncObj[Id].bAllocSnapshotBuf = pMaxMemInfo->bAllocSnapshotBuf;
		gNMRVdoEncObj[Id].InitInfo.comm_recfrm_en = pMaxMemInfo->bCommRecFrmEn;

		// snapshot size
		if (gNMRVdoEncObj[Id].uiVidCodec == NMEDIAREC_ENC_H264) {
			gNMRVdoEncObj[Id].uiVidCodec = NMEDIAREC_ENC_H265; // (calculate MAX_MEM by H265, because H265 need larger memory than h264)
		}
		uiSnapShotSize = _NMR_VdoEnc_GetSnapshotSize(Id);
		gNMRVdoEncObj[Id].uiVidCodec = pMaxMemInfo->uiCodec;

		if (gNMRVdoEncObj[Id].uiVidCodec == NMEDIAREC_ENC_MJPG) {
			// if MJPG ... just calculate it
			uiCodecsize = _NMR_VdoEnc_GetCodecSize(Id);
		} else {
			// if H264/H265 ... calculate both and use larger one (for max memory way....h264/h265 need memory is difference for settings, sometimes h264 need more, sometime h265 need more)
			// also we have to try (bD2dEn = 0/1) + (rotate = 0/1) , get MAX one
			UINT32 uiH264Need = 0, uiH265Need = 0;
			UINT32 uiCodecsize_max = 0;
			UINT32 backup_codec = gNMRVdoEncObj[Id].uiVidCodec;
			UINT32 bD2dEn = 0, rotate = 0;
			UINT32 rotate_max_idx = (pMaxMemInfo->uiRotate)? 1:0;  // IVOT_N01004_CO-113 : maybe rotate cause H265 DO NOT need tile (for example, 2592x1944(need tile) -> 1944x2592(no tile),  rotate will need less memory than original, so we have to check both rotate on/off, pick max one.

			if (pMaxMemInfo->bFitWorkMemory == TRUE) {
				uiCodecsize = _NMR_VdoEnc_GetCodecSize(Id);
			} else {
				for (bD2dEn = 0; bD2dEn <= 1; bD2dEn++) {
					if (gNMRVdoEncObj[Id].InitInfo.width > H265E_D2D_MAX_W_528 && bD2dEn == 1) {
						continue;
					}
					for (rotate = 0; rotate <= rotate_max_idx; rotate ++) {
						gNMRVdoEncObj[Id].InitInfo.bD2dEn = bD2dEn;
						gNMRVdoEncObj[Id].InitInfo.rotate = rotate;
						gNMRVdoEncObj[Id].uiVidCodec = NMEDIAREC_ENC_H264;
						uiH264Need = _NMR_VdoEnc_GetCodecSize(Id);
						gNMRVdoEncObj[Id].uiVidCodec = NMEDIAREC_ENC_H265;
						uiH265Need = _NMR_VdoEnc_GetCodecSize(Id);

						uiCodecsize_max = (uiCodecsize_max < uiH264Need)? uiH264Need:uiCodecsize_max;
						uiCodecsize_max = (uiCodecsize_max < uiH265Need)? uiH265Need:uiCodecsize_max;
						DBG_IND("[VDOENC][%d] bD2dEn(%u) rotate(%u), H264 need = %u, H265 need = %u\r\n", Id, bD2dEn, rotate, uiH264Need, uiH265Need);
					}
				}

				uiCodecsize = uiCodecsize_max;
			}
			DBG_IND("[VDOENC][%d] uiCodecsize = %u\r\n", Id, uiCodecsize);
			gNMRVdoEncObj[Id].uiVidCodec = backup_codec;
		}

		uiEncsize = _NMR_VdoEnc_GetEncBufSize(Id);
#if MP_VDOENC_SHOW_MSG
#if 0
		DBG_DUMP("[VDOENC][%d] Set max alloc size, codec = %d, w = %d, h = %d, byterate = %d, recformat = %d, rotate = %d, svc = %d, ltr = %d, snapshot size = %d, codec size = %d, enc buf size = %d\r\n",
#else
		(&isf_vdoenc)->p_base->do_trace(&isf_vdoenc, ISF_OUT(Id), ISF_OP_PARAM_VDOBS,"[VDOENC][%d] Set max alloc size, codec = %d, w = %d, h = %d, byterate = %d, recformat = %d, rotate = %d, svc = %d, ltr = %d, snapshot size = %d, codec size = %d, enc buf size = %d\r\n",
#endif
				Id,
				gNMRVdoEncObj[Id].uiVidCodec,
				gNMRVdoEncObj[Id].InitInfo.width,
				gNMRVdoEncObj[Id].InitInfo.height,
				gNMRVdoEncObj[Id].InitInfo.byte_rate,
				gNMRVdoEncObj[Id].uiRecFormat,
				pMaxMemInfo->uiRotate,
				gNMRVdoEncObj[Id].InitInfo.e_svc,
				gNMRVdoEncObj[Id].InitInfo.ltr_interval,
				uiSnapShotSize,
				uiCodecsize,
				uiEncsize);
#endif
		//recover gNMRVdoEncObj
		gNMRVdoEncObj[Id].uiVidCodec = uiVidCodec;
		gNMRVdoEncObj[Id].InitInfo.width = uiWidth;
		gNMRVdoEncObj[Id].InitInfo.height = uiHeight;
		gNMRVdoEncObj[Id].InitInfo.byte_rate = uiByteRate;
		gNMRVdoEncObj[Id].MemInfo.uiEncBufMs = uiEncBufMs;
		gNMRVdoEncObj[Id].uiRecFormat = uiRecFormat;
		gNMRVdoEncObj[Id].InitInfo.e_svc = uiSVC;
		gNMRVdoEncObj[Id].InitInfo.ltr_interval = uiLTRInterval;
		gNMRVdoEncObj[Id].InitInfo.rotate = uiRotate;
		gNMRVdoEncObj[Id].bAllocSnapshotBuf = bAllocSnapshotBuf;

		pMaxMemInfo->uiSnapShotSize = uiSnapShotSize;
		pMaxMemInfo->uiCodecsize = uiCodecsize /*+ 0x2000*/ + NMR_VDOENC_MD_MAP_MAX_SIZE; // +0x2000 is added by JIRA:NA51023-552 for rotate... but now HDAL already check rotate on/off to pick max one, so remove it !
		pMaxMemInfo->uiEncsize = uiEncsize;
#ifdef __KERNEL__
		if (kdrv_builtin_is_fastboot() && Id < BUILTIN_VDOENC_PATH_ID_MAX && gNMRVdoEnc1stStart[Id]) {
			BOOL bBuiltinEn = 0;
			UINT32 builtin_codectype = 0;
			VdoEnc_Builtin_GetParam(Id, BUILTIN_VDOENC_INIT_PARAM_ENC_EN, &bBuiltinEn);
			VdoEnc_Builtin_GetParam(Id, BUILTIN_VDOENC_INIT_PARAM_CODEC, &builtin_codectype);
			if (kdrv_builtin_is_fastboot() && bBuiltinEn && builtin_codectype == BUILTIN_VDOENC_MJPEG) {
				pMaxMemInfo->uiCodecsize = 0;
				pMaxMemInfo->uiSnapShotSize = 0;
			}
		}
#endif
		break;
	}

	case NMI_VDOENC_PARAM_ALLOC_SNAPSHOT_BUF:
		gNMRVdoEncObj[Id].bAllocSnapshotBuf = (BOOL) Value;
		break;

	case NMI_VDOENC_PARAM_SNAPSHOT:
		gNMRVdoEncObj[Id].bSnapshot = (BOOL) Value;
		break;

	case NMI_VDOENC_PARAM_DIS:
		gNMRVdoEncObj[Id].bDis = (BOOL) Value;
		break;

	case NMI_VDOENC_PARAM_TV_RANGE:
		gNMRVdoEncObj[Id].bTvRange= (BOOL) Value;
		break;

	case NMI_VDOENC_PARAM_START_TIMER_BY_MANUAL:
#if MP_VDOENC_SHOW_MSG
		if (Value) {
			DBG_DUMP("[VDOENC][%d] Start Timer by Manual = %d\r\n", Id, Value);
		}
#endif
		gNMRVdoEncObj[Id].bStartTimerByManual = (BOOL) Value;
		break;

	case NMI_VDOENC_PARAM_START_TIMER:
		if (gNMRVdoEncObj[Id].bStartTimerByManual) {
#if MP_VDOENC_SHOW_MSG
			DBG_DUMP("[VDOENC][%d] Start Timer\r\n", Id);
#endif
			_NMR_VdoEnc_SetEncTriggerTimer(Id);
		}
		break;

	case NMI_VDOENC_PARAM_STOP_TIMER:
		if (gNMRVdoEncObj[Id].bStartTimerByManual) {
#if MP_VDOENC_SHOW_MSG
			DBG_DUMP("[VDOENC][%d] Stop Timer\r\n", Id);
#endif
			_NMR_VdoEnc_CloseTriggerTimer(Id);
		}
		break;

	case NMI_VDOENC_PARAM_START_SMART_ROI:
#if MP_VDOENC_SHOW_MSG
		if (Value != (UINT32)gNMRVdoEncObj[Id].bStartSmartRoi) {
			DBG_DUMP("[VDOENC][%d] Start Smart Roi = %d\r\n", Id, Value);
		}
#endif
		gNMRVdoEncObj[Id].bStartSmartRoi = (BOOL) Value;
		break;

	case NMI_VDOENC_PARAM_WAIT_SMART_ROI:
#if MP_VDOENC_SHOW_MSG
		DBG_DUMP("[VDOENC][%d] Wait Smart Roi = %d\r\n", Id, Value);
#endif
		gNMRVdoEncObj[Id].bWaitSmartRoi = (BOOL) Value;
		break;

	case NMI_VDOENC_PARAM_TRIGGER_MODE:
		gNMRVdoEncObj[Id].uiTriggerMode = Value;
		break;

	case NMI_VDOENC_PARAM_RESET_IFRAME:
		DBG_IND("[VDOENC][%d] set Reset I frame = %d, time = %u us\r\n", Id, Value, Perf_GetCurrent());
		gNMRVdoEncObj[Id].bResetIFrame = (BOOL) Value;
		break;

    case NMI_VDOENC_PARAM_RESET_SEC:
		if (gNMRVdoEncFirstPathID < PATH_MAX_COUNT) {
			gNMRVdoEncObj[gNMRVdoEncFirstPathID].bResetSec = (BOOL) Value;
		} else {
			DBG_ERR("[VDOENC] Reset sec err, callback sec info path = %d\r\n", gNMRVdoEncFirstPathID);
		}
		break;

	case NMI_VDOENC_PARAM_RECFORMAT:
		gNMRVdoEncObj[Id].uiRecFormat = Value;
		break;

	case NMI_VDOENC_PARAM_FILETYPE:
		gNMRVdoEncObj[Id].uiFileType = Value;
		break;

	case NMI_VDOENC_PARAM_BS_RESERVED_SIZE:
		gNMRVdoEncObj[Id].uiBsReservedSize = Value;
		break;

	case NMI_VDOENC_PARAM_WIDTH:
		if (gNMRVdoEncObj[Id].bStart == TRUE) {
			DBG_WRN("[VDOENC][%d] set width = %d when running\r\n", Id, Value);
		}
		gNMRVdoEncObj[Id].InitInfo.width = Value;
		break;

	case NMI_VDOENC_PARAM_HEIGHT:
		if (gNMRVdoEncObj[Id].bStart == TRUE) {
			DBG_WRN("[VDOENC][%d] set height = %d when running\r\n", Id, Value);
		}
		gNMRVdoEncObj[Id].InitInfo.height = Value;
		break;

	case NMI_VDOENC_PARAM_FRAMERATE:
		gNMRVdoEncObj[Id].InitInfo.frame_rate = Value;
		break;

	case NMI_VDOENC_PARAM_TIMERRATE_IMM:
		_NMR_VdoEnc_UpdateEncTriggerTimer(Id, Value);
		break;

	case NMI_VDOENC_PARAM_TIMERRATE2_IMM:
		_NMR_VdoEnc_UpdateEncTriggerTimer2(Id, Value);
		break;

    case NMI_VDOENC_PARAM_ENCBUF_MS:
        gNMRVdoEncObj[Id].MemInfo.uiEncBufMs = Value;
        break;

    case NMI_VDOENC_PARAM_ENCBUF_RESERVED_MS:
        gNMRVdoEncObj[Id].MemInfo.uiEncBufReservedMs = Value;
        break;

	case NMI_VDOENC_PARAM_CODEC:
		if (gNMRVdoEncObj[Id].bStart == FALSE)
			gNMRVdoEncObj[Id].uiVidCodec = Value;
		else
			DBG_ERR("[VDOENC][%d] invalid operation, cannot change codec when this path is running\r\n", Id);
		break;

	case NMI_VDOENC_PARAM_PROFILE:
		gNMRVdoEncObj[Id].InitInfo.e_profile = (MP_VDOENC_PROFILE)Value;
		break;

	case NMI_VDOENC_PARAM_GOPTYPE:
		gNMRVdoEncObj[Id].uiGopType = Value;
		break;

	case NMI_VDOENC_PARAM_INITQP:
		gNMRVdoEncObj[Id].InitInfo.init_i_qp = Value;
		gNMRVdoEncObj[Id].InitInfo.init_p_qp = Value;
		break;

	case NMI_VDOENC_PARAM_MINQP:
		if (Value <= MP_VDOENC_QP_MAX) {
			gNMRVdoEncObj[Id].InitInfo.min_i_qp = Value;
			gNMRVdoEncObj[Id].InitInfo.min_p_qp = Value;
		} else {
			DBG_ERR("[VDOENC][%d] Set MinQP invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_MAXQP:
		if (Value <= MP_VDOENC_QP_MAX) {
			gNMRVdoEncObj[Id].InitInfo.max_i_qp = Value;
			gNMRVdoEncObj[Id].InitInfo.max_p_qp = Value;
		} else {
			DBG_ERR("[VDOENC][%d] Set MaxQP invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_CBRINFO:
		NMR_VdoEnc_SetCBR(Id, (MP_VDOENC_CBR_INFO *)Value);
		break;

	case NMI_VDOENC_PARAM_EVBRINFO:
		NMR_VdoEnc_SetEVBR(Id, (MP_VDOENC_EVBR_INFO *)Value);
		break;

	case NMI_VDOENC_PARAM_VBRINFO:
		NMR_VdoEnc_SetVBR(Id, (MP_VDOENC_VBR_INFO *)Value);
		break;

	case NMI_VDOENC_PARAM_FIXQPINFO:
		NMR_VdoEnc_SetFixQP(Id, (MP_VDOENC_FIXQP_INFO *)Value);
		break;

	case NMI_VDOENC_PARAM_ROWRC:
		if (Value != 0) {
			memcpy((void *)&gNMRVdoEncObj[Id].RowRcInfo, (void *)Value, sizeof(MP_VDOENC_ROWRC_INFO));
			if (gNMRVdoEncObj[Id].bStart == TRUE && gNMRVdoTrigObj[Id].Encoder.SetInfo != 0) {
				(gNMRVdoTrigObj[Id].Encoder.SetInfo)(VDOENC_SET_ROWRC, Value, 0, 0);
			}
		} else {
			DBG_ERR("[VDOENC][%d] Set ROWRC invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_AQINFO:
		if (Value != 0) {
			memcpy((void *)&gNMRVdoEncObj[Id].AqInfo, (void *) Value, sizeof(MP_VDOENC_AQ_INFO));
			if (gNMRVdoEncObj[Id].bStart == TRUE && gNMRVdoTrigObj[Id].Encoder.SetInfo != 0) {
				(gNMRVdoTrigObj[Id].Encoder.SetInfo)(VDOENC_SET_AQ, Value, 0, 0);
			}
		} else {
			DBG_ERR("[VDOENC][%d] Set AQ invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_GDR:
		if (Value != 0) {
			memcpy((void *)&gNMRVdoEncObj[Id].GdrInfo, (void *) Value, sizeof(MP_VDOENC_GDR_INFO));
			gNMRVdoEncObj[Id].InitInfo.bGDRIFrm = gNMRVdoEncObj[Id].GdrInfo.enable_gdr_i_frm;
			if (gNMRVdoEncObj[Id].bStart == TRUE && gNMRVdoTrigObj[Id].Encoder.SetInfo != 0) {
				(gNMRVdoTrigObj[Id].Encoder.SetInfo)(VDOENC_SET_GDR, Value, 0, 0);
			}
		} else {
			DBG_ERR("[VDOENC][%d] Set GDR invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_USR_QP_MAP:
		if (Value != 0) {
			memcpy((void *)&gNMRVdoEncObj[Id].QpMapInfo, (void *) Value, sizeof(MP_VDOENC_QPMAP_INFO));
			if (gNMRVdoEncObj[Id].bStart == TRUE && gNMRVdoTrigObj[Id].Encoder.SetInfo != 0) {
				(gNMRVdoTrigObj[Id].Encoder.SetInfo)(VDOENC_SET_QPMAP, Value, 0, 0);
			}
		} else {
			DBG_ERR("[VDOENC][%d] Set QP Map invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_SLICE_SPLIT:
		if (Value != 0) {
			memcpy((void *)&gNMRVdoEncObj[Id].SliceSplitInfo, (void *) Value, sizeof(MP_VDOENC_SLICESPLIT_INFO));
			if (gNMRVdoEncObj[Id].bStart == TRUE && gNMRVdoTrigObj[Id].Encoder.SetInfo != 0) {
				(gNMRVdoTrigObj[Id].Encoder.SetInfo)(VDOENC_SET_SLICESPLIT, Value, 0, 0);
			}
		} else {
			DBG_ERR("[VDOENC][%d] Set SliceSplit invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_3DNR_CB:
		gNMRVdoEncObj[Id].v3dnrCb.id = Id;
		gNMRVdoEncObj[Id].v3dnrCb.VdoEnc_3DNRDo = (MEDIAREC_3DNRCB) Value;
		break;

	case NMI_VDOENC_PARAM_ISP_CB:
		gNMRVdoEncObj[Id].ispCb.id = Id;
		gNMRVdoEncObj[Id].ispCb.vdoenc_isp_cb = (MEDIAREC_ISPCB) Value;
		break;

	case NMI_VDOENC_PARAM_IMM_CB:
		gNMRVdoEncObj[Id].ImmProc = (NMI_VDOENC_IMM_CB *) Value;
		break;

	case NMI_VDOENC_REG_CB:
		gNMRVdoEncObj[Id].CallBackFunc = (NMI_VDOENC_CB *) Value;
		break;

	case NMI_VDOENC_PARAM_GOPNUM:
		gNMRVdoEncObj[Id].InitInfo.gop = Value;
		break;

	case NMI_VDOENC_PARAM_CHRM_QP_IDX:
		gNMRVdoEncObj[Id].InitInfo.chrm_qp_idx = Value;
		break;

	case NMI_VDOENC_PARAM_SEC_CHRM_QP_IDX:
		gNMRVdoEncObj[Id].InitInfo.sec_chrm_qp_idx = Value;
		break;

	case NMI_VDOENC_PARAM_GOPNUM_ONSTART:
		gNMRVdoEncObj[Id].InitInfo.gop = Value;
		if (gNMRVdoEncObj[Id].bStart == TRUE && gNMRVdoTrigObj[Id].Encoder.SetInfo != 0) {
			(gNMRVdoTrigObj[Id].Encoder.SetInfo)(VDOENC_SET_GOPNUM, (UINT32)&Value, 0, 0);
		}
		break;

	case NMI_VDOENC_PARAM_FRAME_INTERVAL:
		gNMRVdoEncObj[Id].uiFrameInterval = Value;
		break;

	case NMI_VDOENC_PARAM_TARGETRATE:
		gNMRVdoEncObj[Id].InitInfo.byte_rate = Value;
		break;

	case NMI_VDOENC_PARAM_DAR:
		gNMRVdoEncObj[Id].InitInfo.e_dar = Value;
		break;

	case NMI_VDOENC_PARAM_SVC:
		gNMRVdoEncObj[Id].InitInfo.e_svc = Value;
		break;

	case NMI_VDOENC_PARAM_LTR:
		if (Value != 0) {
			MP_VDOENC_LTR_INFO *pInfo = (MP_VDOENC_LTR_INFO *)Value;
			gNMRVdoEncObj[Id].InitInfo.ltr_interval = pInfo->uiLTRInterval;
			gNMRVdoEncObj[Id].InitInfo.ltr_pre_ref = pInfo->uiLTRPreRef;
		} else {
			DBG_ERR("[VDOENC][%d] Set LTR invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_MULTI_TEMPORARY_LAYER:
		gNMRVdoEncObj[Id].InitInfo.multi_layer= Value;
		break;

	case NMI_VDOENC_PARAM_USRQP:
		gNMRVdoEncObj[Id].InitInfo.user_qp_en = Value;
		break;

	case NMI_VDOENC_PARAM_ROTATE:
		gNMRVdoEncObj[Id].InitInfo.rotate = Value;
		break;

	case NMI_VDOENC_PARAM_TIMELAPSE_TIME:
		if (Value > 3600000) { //1HR
			DBG_ERR("[VDOENC][%d] Timelapse time exceeds max 1hr, Change to 1 sec one shot\r\n", Id);
			Value = 1000;//1sec
		}
		#if 0  // now timelapse_time =0 means Normal TimerMode.    timelapse_time >0 means Timelapse TimerMode
		if (Value < 1000 / 30) {
			Value = 1000 / 30;    // minimum value = 1/30 sec
		}
		#endif
		gNMRVdoEncObj[Id].uiTimelapseTime = Value;
		break;

	case NMI_VDOENC_PARAM_MIN_I_RATIO:
		gNMRVdoEncObj[Id].MemInfo.uiMinIRatio = Value;
		break;

	case NMI_VDOENC_PARAM_MIN_P_RATIO:
		gNMRVdoEncObj[Id].MemInfo.uiMinPRatio = Value;
		break;

	case NMI_VDOENC_PARAM_MAX_FRAME_QUEUE:
		if (Value <= NMR_VDOTRIG_BSQ_MAX) {
			gNMRVdoEncObj[Id].uiMaxFrameQueue = Value;
		} else {
			DBG_ERR("[VDOENC][%d] max queue count(%d) > %d\r\n", Id, Value, NMR_VDOTRIG_BSQ_MAX);
		}
		break;

	case NMI_VDOENC_PARAM_BUILTIN_MAX_FRAME_QUEUE:
		if (Value <= NMR_VDOTRIG_BSQ_MAX) {
			gNMRVdoEncObj[Id].uiMaxFrameQueue = NMR_VDOTRIG_BSQ_MAX;
		} else {
			gNMRVdoEncObj[Id].uiMaxFrameQueue = Value;
		}
		break;

	case NMI_VDOENC_PARAM_LEVEL_IDC:
		gNMRVdoEncObj[Id].InitInfo.level_idc = Value;
		break;

	case NMI_VDOENC_PARAM_ENTROPY:
		gNMRVdoEncObj[Id].InitInfo.e_entropy = Value;
		break;

	case NMI_VDOENC_PARAM_GRAY:
		gNMRVdoEncObj[Id].InitInfo.gray_en = Value;
		break;

	case NMI_VDOENC_PARAM_DEBLOCK:
		if (Value != 0) {
			MP_VDOENC_INIT *pInfo = (MP_VDOENC_INIT *)Value;
			gNMRVdoEncObj[Id].InitInfo.disable_db = pInfo->disable_db;
			gNMRVdoEncObj[Id].InitInfo.db_alpha   = pInfo->db_alpha;
			gNMRVdoEncObj[Id].InitInfo.db_beta    = pInfo->db_beta;
		} else {
			DBG_ERR("[VDOENC][%d] Set DEBLOCK invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_VUI:
		if (Value != 0) {
			MP_VDOENC_INIT *pInfo = (MP_VDOENC_INIT *)Value;
			gNMRVdoEncObj[Id].InitInfo.bVUIEn                   = pInfo->bVUIEn;
			gNMRVdoEncObj[Id].InitInfo.sar_width                = pInfo->sar_width;
			gNMRVdoEncObj[Id].InitInfo.sar_height               = pInfo->sar_height;
			gNMRVdoEncObj[Id].InitInfo.matrix_coef              = pInfo->matrix_coef;
			gNMRVdoEncObj[Id].InitInfo.transfer_characteristics = pInfo->transfer_characteristics;
			gNMRVdoEncObj[Id].InitInfo.colour_primaries         = pInfo->colour_primaries;
			gNMRVdoEncObj[Id].InitInfo.video_format             = pInfo->video_format;
			gNMRVdoEncObj[Id].InitInfo.time_present_flag        = pInfo->time_present_flag;
			gNMRVdoEncObj[Id].bTvRange                          = (pInfo->color_range==0)? 1:0; // equal to set NMI_VDOENC_PARAM_TV_RANGE, (TvRange = 0, ColorRange = 1) / (TvRange = 1, ColorRange = 0)
		} else {
			DBG_ERR("[VDOENC][%d] Set VUI invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_SKIP_LOFF_CHECK:
		gNMRVdoEncObj[Id].bSkipLoffCheck = Value;
		break;

	case NMI_VDOENC_PARAM_IMGFMT:
		gNMRVdoEncJpgYuvFormat[Id] = Value;  // should be VDO_PXLFMT_YUV420 or VDO_PXLFMT_YUV422
		break;

	case NMI_VDOENC_PARAM_JPG_QUALITY:
		gNMRVdoTrigObj[Id].EncParam.quality = Value;  // jpeg encode quality
		break;

	case NMI_VDOENC_PARAM_JPG_RESTART_INTERVAL:
		gNMRVdoTrigObj[Id].EncParam.retstart_interval = Value;  // jpeg encode restart interval
		break;

	case NMI_VDOENC_PARAM_JPG_BITRATE:
		gNMRVdoTrigObj[Id].EncParam.target_rate = Value;  // jpeg encode bitrate
		break;

	case NMI_VDOENC_PARAM_JPG_FRAMERATE:
		gNMRVdoTrigObj[Id].EncParam.frame_rate = Value;  // jpeg encode framerate
		break;

	case NMI_VDOENC_PARAM_JPG_VBR_MODE:
		gNMRVdoTrigObj[Id].EncParam.vbr_mode = Value;  // jpeg encode rc mode, 0: CBR , 1: VBR
		break;

	case NMI_VDOENC_PARAM_H26X_VBR_POLICY:
		gNMRVdoEncObj[Id].VbrInfo.policy = Value;    // h26x vbr policy, 0: original , 1: hisi-like
		if (gNMRVdoEncObj[Id].uiSetRCMode == 0) {
			gNMRVdoEncObj[Id].uiSetRCMode = NMR_VDOENC_RC_VBR;
		}
		break;

	case NMI_VDOENC_PARAM_LOWLATENCY:
		gNMRVdoEncObj[Id].InitInfo.bD2dEn = Value;
		break;

	case NMI_VDOENC_PARAM_RDO:
		if (Value != 0) {
			memcpy((void *)&gNMRVdoTrigObj[Id].rdo_info, (void *) Value, sizeof(MP_VDOENC_RDO_INFO));
			gNMRVdoTrigObj[Id].rdo_en = TRUE;
			if (gNMRVdoEncObj[Id].bStart == TRUE && gNMRVdoTrigObj[Id].Encoder.SetInfo != 0) {
				(gNMRVdoTrigObj[Id].Encoder.SetInfo)(VDOENC_SET_RDO, Value, 0, 0);
			}
		} else {
			DBG_ERR("[VDOENC][%d] Set RDO invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_JND:
		if (Value != 0) {
			memcpy((void *)&gNMRVdoTrigObj[Id].jnd_info, (void *) Value, sizeof(MP_VDOENC_JND_INFO));
			gNMRVdoTrigObj[Id].jnd_en = TRUE;
			if (gNMRVdoEncObj[Id].bStart == TRUE && gNMRVdoTrigObj[Id].Encoder.SetInfo != 0) {
				(gNMRVdoTrigObj[Id].Encoder.SetInfo)(VDOENC_SET_JND, Value, 0, 0);
			}
		} else {
			DBG_ERR("[VDOENC][%d] Set JND invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_BSQ_MEM:
		if (Value != 0) {
			gNMRVdoEncObj[Id].uiBSQAddr = Value;
		} else {
			DBG_ERR("[VDOENC][%d] Set BSQ Mem invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_JPG_YUV_TRAN_ENABLE:
		gNMRVdoTrigObj[Id].EncParam.fmt_trans_en = Value;
		break;

	case NMI_VDOENC_PARAM_JPG_RC_MIN_QUALITY:
		gNMRVdoTrigObj[Id].EncParam.min_quality = Value;
		break;

	case NMI_VDOENC_PARAM_JPG_RC_MAX_QUALITY:
		gNMRVdoTrigObj[Id].EncParam.max_quality = Value;
		break;

	case NMI_VDOENC_PARAM_H26X_SRCOUT_JOB:
		{
			MP_VDOENC_YUV_SRCOUT *p_srcout = (MP_VDOENC_YUV_SRCOUT *)Value;
			if (TRUE != NMR_VdoTrig_PutSrcOut(p_srcout->path_id, p_srcout)) {
				return E_SYS;
			}
			NMR_VdoTrig_PutJob_H26X_Snapshot(p_srcout->path_id);
			NMR_VdoTrig_TrigSnapshotEncode();
		}
		break;

	case NMI_VDOENC_PARAM_FASTBOOT_CODEC:
		if (gNMRVdoEncObj[Id].bStart == FALSE)
			gNMRVdoEncObj[Id].uiVidFastbootCodec = Value;
		else
			DBG_ERR("[VDOENC][%d] invalid operation, cannot change codec when this path is running\r\n", Id);
		break;

	case NMI_VDOENC_PARAM_COLMV_ENABLE:
		gNMRVdoEncObj[Id].InitInfo.colmv_en = Value;
		break;

	case NMI_VDOENC_PARAM_COMM_RECFRM_ENABLE:
		gNMRVdoEncObj[Id].InitInfo.comm_recfrm_en = Value;
		break;

	case NMI_VDOENC_PARAM_RECONFRM_RANGE:
		if (Value != 0) {
			NMI_VDOENC_RECBUF_RANGE *pRecbuf = (NMI_VDOENC_RECBUF_RANGE *) Value;
			if (pRecbuf->Addr == 0 || pRecbuf->Size == 0) {
				gNMRVdoEncObj[Id].MemInfo.uiBSStart = 0;
				DBG_ERR("[VDOENC][%d] invalid mem range, addr = 0x%08x, size = %d\r\n", Id, pRecbuf->Addr, pRecbuf->Size);
				return E_PAR;
			}
			if (_NMR_VdoEnc_SetReconFrmBuf(Id, pRecbuf->Addr, pRecbuf->Size, pRecbuf->Idx) != E_OK) {
				DBG_ERR("[VDOENC][%d] set reconstruct frame range error\r\n", Id);
			}
		} else {
			DBG_ERR("[VDOENC][%d] Set reconstruct frame range invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_LONG_START_CODE:
		gNMRVdoEncObj[Id].uiLongStartCode = Value;
		break;

	case NMI_VDOENC_PARAM_H26X_SVC_WEIGHT_MODE:
		if (gNMRVdoEncObj[Id].uiSetRCMode == NMR_VDOENC_RC_CBR) {
			gNMRVdoEncObj[Id].CbrInfo.svc_weight_mode = Value;    // h26x svc weight mode, 0: original , 1: svc weight mode
		} else if (gNMRVdoEncObj[Id].uiSetRCMode == NMR_VDOENC_RC_EVBR) {
			gNMRVdoEncObj[Id].EVbrInfo.svc_weight_mode = Value;    // h26x svc weight mode, 0: original , 1: svc weight mode
		} else if (gNMRVdoEncObj[Id].uiSetRCMode == NMR_VDOENC_RC_VBR) {
			gNMRVdoEncObj[Id].VbrInfo.svc_weight_mode = Value;    // h26x svc weight mode, 0: original , 1: svc weight mode
		} else if (gNMRVdoEncObj[Id].uiSetRCMode == 0) {
			DBG_WRN("[VDOENC][%d] RC mode is 0, please set RC mode before set svc weight mode\r\n", Id);
		} else {
			DBG_WRN("[VDOENC][%d] RC mode %d not support svc weight mode\r\n", Id, gNMRVdoEncObj[Id].uiSetRCMode);
		}
		break;

	case NMI_VDOENC_PARAM_BS_QUICK_ROLLBACK:
		gNMRVdoEncObj[Id].bBsQuickRollback = Value;
		break;

	case NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_DEV:
		gNMRVdoEncObj[Id].uiVidFastbootVprcSrcDev = Value;
		break;

	case NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_PATH:
		gNMRVdoEncObj[Id].uiVidFastbootVprcSrcPath = Value;
		break;

	case NMI_VDOENC_PARAM_QUALITY_BASE_MODE_ENABLE:
		gNMRVdoEncObj[Id].bQualityBase = Value;
		break;

	case NMI_VDOENC_PARAM_HW_PADDING_MODE:
		gNMRVdoEncObj[Id].uiEncPaddingMode = Value;
		break;

	case NMI_VDOENC_PARAM_FASTBOOT_ISP_ID:
		gNMRVdoEncObj[Id].uiVidFastbootISPId = Value;
		break;

	case NMI_VDOENC_PARAM_BR_TOLERANCE:
		// bitrate tolerance, range: 0~6
		gNMRVdoEncObj[Id].CbrInfo.br_tolerance = Value;
		gNMRVdoEncObj[Id].VbrInfo.br_tolerance = Value;
		gNMRVdoEncObj[Id].EVbrInfo.br_tolerance = Value;
		break;

	case NMI_VDOENC_PARAM_COMM_SRCOUT_ENABLE:
		gNMRVdoEncObj[Id].bCommSrcOut = (BOOL) Value;
		break;

	case NMI_VDOENC_PARAM_SNAPSHOT_RANGE:
		if (Value != 0) {
			NMI_VDOENC_MEM_RANGE *pMem = (NMI_VDOENC_MEM_RANGE *) Value;
			if (pMem->Addr == 0 || pMem->Size == 0) {
				gNMRVdoEncObj[Id].MemInfo.uiSnapshotAddr = 0;
				DBG_ERR("[VDOENC][%d] invalid snapshot mem range, addr = 0x%08x, size = %d\r\n", Id, pMem->Addr, pMem->Size);
				return E_PAR;
			}
			if (_NMR_VdoEnc_SetSnapShotBuf(Id, pMem->Addr, pMem->Size) != E_OK) {
				DBG_ERR("[VDOENC][%d] set snapshot mem range error\r\n", Id);
				return E_PAR;
			}
		} else {
			DBG_ERR("[VDOENC][%d] Set snapshot Mem range invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_COMM_SRCOUT_NO_JPG_ENC:
		gNMRVdoEncObj[Id].bCommSrcOutNoJPGEnc = (BOOL) Value;
		break;

	case NMI_VDOENC_PARAM_TIMER_TRIG_COMP_ENABLE:
		gNMRVdoEncObj[Id].bTimerTrigComp = (BOOL) Value;
		break;

	case NMI_VDOENC_REQ_TARGET_I:
		if (Value != 0) {
			NMI_VDOENC_REQ_TARGET_I_INFO *pInfo = (NMI_VDOENC_REQ_TARGET_I_INFO *)Value;
			gNMRVdoEncObj[Id].bResetIFrameByTimeStamp = pInfo->enable;
			gNMRVdoEncObj[Id].uiTargetTimeStamp = pInfo->target_timestamp;
		} else {
			DBG_ERR("[VDOENC][%d] Set REQ_TARGET_I invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_H26X_SEI_CHKSUM:
		gNMRVdoEncObj[Id].bSEIBsChkSum = Value;
		break;

	case NMI_VDOENC_PARAM_H26X_LOW_POWER:
		gNMRVdoEncObj[Id].bLowPower = Value;
		break;

	case NMI_VDOENC_PARAM_H26X_CROP:
		if (Value != 0) {
			memcpy((void *)&gNMRVdoEncObj[Id].InitInfo.frm_crop_info, (void *) Value, sizeof(MP_VDOENC_FRM_CROP_INFO));
		} else {
			DBG_ERR("[VDOENC][%d] Set H26X_CROP invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_H26X_MAQ_DIFF_ENABLE:
		gNMRVdoEncObj[Id].MaqDiffInfo.enable = Value;
		break;

	case NMI_VDOENC_PARAM_H26X_MAQ_DIFF_STR:
		gNMRVdoEncObj[Id].MaqDiffInfo.str = Value;
		break;

	case NMI_VDOENC_PARAM_H26X_MAQ_DIFF_START_IDX:
		gNMRVdoEncObj[Id].MaqDiffInfo.start_idx = Value;
		break;

	case NMI_VDOENC_PARAM_H26X_MAQ_DIFF_END_IDX:
		gNMRVdoEncObj[Id].MaqDiffInfo.end_idx = Value;
		break;

	case NMI_VDOENC_PARAM_BS_RING:
		gNMRVdoEncObj[Id].MemInfo.bBSRingMode = (BOOL)Value;
		break;

	case NMI_VDOENC_PARAM_JPG_JFIF_ENABLE:
		gNMRVdoTrigObj[Id].EncParam.jpeg_jfif_en = Value;
		break;

	case NMI_VDOENC_PARAM_H26X_SKIP_FRM:
		if (Value != 0) {
			NMI_VDOENC_H26X_SKIP_FRM_INFO *pInfo = (NMI_VDOENC_H26X_SKIP_FRM_INFO *)Value;
			gNMRVdoEncObj[Id].bSkipFrm = pInfo->enable;
			gNMRVdoEncObj[Id].uiSkipFrmTargetFr = pInfo->target_fr;
			gNMRVdoEncObj[Id].uiSkipFrmInputCnt = pInfo->input_frm_cnt;

			if (gNMRVdoEncObj[Id].uiSetRCMode == NMR_VDOENC_RC_CBR) {
				gNMRVdoEncObj[Id].CbrInfo.encode_frame_rate = pInfo->target_fr;
				gNMRVdoEncObj[Id].CbrInfo.src_frame_rate = pInfo->input_frm_cnt;
			} else if (gNMRVdoEncObj[Id].uiSetRCMode == NMR_VDOENC_RC_EVBR) {
//				gNMRVdoEncObj[Id].EVbrInfo.encode_frame_rate = pInfo->target_fr;
//				gNMRVdoEncObj[Id].EVbrInfo.src_frame_rate = pInfo->input_frm_cnt;
			} else if (gNMRVdoEncObj[Id].uiSetRCMode == NMR_VDOENC_RC_VBR) {
//				gNMRVdoEncObj[Id].VbrInfo.encode_frame_rate = pInfo->target_fr;
//				gNMRVdoEncObj[Id].VbrInfo.src_frame_rate = pInfo->input_frm_cnt;
			} else if (gNMRVdoEncObj[Id].uiSetRCMode == 0) {
				DBG_WRN("[VDOENC][%d] RC mode is 0, please set RC mode before set H26X_SKIP_FRM\r\n", Id);
			} else {
				DBG_WRN("[VDOENC][%d] RC mode %d not support H26X_SKIP_FRM rate control\r\n", Id, gNMRVdoEncObj[Id].uiSetRCMode);
			}
		} else {
			DBG_ERR("[VDOENC][%d] Set H26X_SKIP_FRM invalid value = %d\r\n", Id, Value);
		}
		break;

	case NMI_VDOENC_PARAM_META_ALLOC:
		if (Value != 0) {
			gNMRVdoEncObj[Id].pMetaAllocInfo = (NMI_VDOENC_META_ALLOC_INFO*)Value;
		} else {
			DBG_ERR("[VDOENC][%d] Set META_ALLOC invalid value = %d\r\n", Id, Value);
		}
		break;

	default:
		DBG_ERR("[VDOENC][%d] Set invalid param = %d\r\n", Id, Param);
		return E_PAR;
	}
	return E_OK;
}

static ER NMI_VdoEnc_GetParam(UINT32 Id, UINT32 Param, UINT32 *pValue)
{
	switch (Param) {
	case NMI_VDOENC_PARAM_HW_LIMIT:
		if (pValue) {
			MP_VDOENC_HW_LMT *p_hw_limit = (MP_VDOENC_HW_LMT *)pValue;
			_NMR_VdoEnc_Get_HW_Limit(p_hw_limit);
		}
		break;

	case NMI_VDOENC_PARAM_ROI:
		*pValue = (UINT32) &gNMRVdoEncObj[Id].RoiInfo;
		break;

	case NMI_VDOENC_PARAM_ALLOC_SIZE: {
#ifdef __KERNEL__
			BOOL bBuiltinEn = 0;
			UINT32 builtin_codectype = 0;
#endif
			if (gNMRVdoEncObj[Id].bCommSrcOut) {
				gNMRVdoEncObj[Id].uiSnapShotSize = 0;
			} else {
				gNMRVdoEncObj[Id].uiSnapShotSize = _NMR_VdoEnc_GetSnapshotSize(Id);
			}
			gNMRVdoEncObj[Id].uiCodecSize = _NMR_VdoEnc_GetCodecSize(Id);
			gNMRVdoEncObj[Id].uiEncSize = _NMR_VdoEnc_GetEncBufSize(Id);
#ifdef __KERNEL__
			if (kdrv_builtin_is_fastboot() && Id < BUILTIN_VDOENC_PATH_ID_MAX && gNMRVdoEnc1stStart[Id]) {
				VdoEnc_Builtin_GetParam(Id, BUILTIN_VDOENC_INIT_PARAM_ENC_EN, &bBuiltinEn);
				VdoEnc_Builtin_GetParam(Id, BUILTIN_VDOENC_INIT_PARAM_CODEC, &builtin_codectype);

				if (bBuiltinEn && builtin_codectype == BUILTIN_VDOENC_MJPEG) {
					*pValue = gNMRVdoEncObj[Id].uiSnapShotSize + gNMRVdoEncObj[Id].uiCodecSize + gNMRVdoEncObj[Id].uiEncSize;
				} else {
					*pValue = gNMRVdoEncObj[Id].uiSnapShotSize + gNMRVdoEncObj[Id].uiCodecSize + gNMRVdoEncObj[Id].uiEncSize + NMR_VDOENC_MD_MAP_MAX_SIZE;
				}
			} else
#endif
			*pValue = gNMRVdoEncObj[Id].uiSnapShotSize + gNMRVdoEncObj[Id].uiCodecSize + gNMRVdoEncObj[Id].uiEncSize + NMR_VDOENC_MD_MAP_MAX_SIZE;
			break;
		}

	case NMI_VDOENC_PARAM_RECON_FRAME_SIZE:
		*pValue = _NMR_VdoEnc_GetReconBufSize(Id);
		break;

	case NMI_VDOENC_PARAM_RECON_FRAME_NUM:
		*pValue = _NMR_VdoEnc_GetReconBufNum(Id);
		break;

	case NMI_VDOENC_PARAM_ENCBUF_ADDR:
		*pValue = gNMRVdoEncObj[Id].MemInfo.uiBSStart;
		break;

	case NMI_VDOENC_PARAM_ENCBUF_SIZE:
		*pValue = gNMRVdoEncObj[Id].MemInfo.uiBSEnd - gNMRVdoEncObj[Id].MemInfo.uiBSStart;
		break;

	case NMI_VDOENC_PARAM_ALLOC_SNAPSHOT_BUF:
		*pValue = gNMRVdoEncObj[Id].bAllocSnapshotBuf;
		break;

	case NMI_VDOENC_PARAM_DIS:
		*pValue = gNMRVdoEncObj[Id].bDis;
		break;

	case NMI_VDOENC_PARAM_TV_RANGE:
		*pValue = gNMRVdoEncObj[Id].bTvRange;
		break;

	case NMI_VDOENC_PARAM_START_TIMER_BY_MANUAL:
		*pValue = gNMRVdoEncObj[Id].bStartTimerByManual;
		break;

	case NMI_VDOENC_PARAM_START_SMART_ROI:
		*pValue = gNMRVdoEncObj[Id].bStartSmartRoi;
		break;

	case NMI_VDOENC_PARAM_WAIT_SMART_ROI:
		*pValue = gNMRVdoEncObj[Id].bWaitSmartRoi;
		break;

	case NMI_VDOENC_PARAM_TRIGGER_MODE:
		*pValue = gNMRVdoEncObj[Id].uiTriggerMode;
		break;

	case NMI_VDOENC_PARAM_RECFORMAT:
		*pValue = gNMRVdoEncObj[Id].uiRecFormat;
		break;

	case NMI_VDOENC_PARAM_FILETYPE:
		*pValue = gNMRVdoEncObj[Id].uiFileType;
		break;

	case NMI_VDOENC_PARAM_BS_RESERVED_SIZE:
		*pValue = gNMRVdoEncObj[Id].uiBsReservedSize;
		break;

	case NMI_VDOENC_PARAM_WIDTH:
		*pValue = gNMRVdoEncObj[Id].InitInfo.width;
		break;

	case NMI_VDOENC_PARAM_HEIGHT:
		*pValue = gNMRVdoEncObj[Id].InitInfo.height;
		break;

	case NMI_VDOENC_PARAM_FRAMERATE:
		*pValue = gNMRVdoEncObj[Id].InitInfo.frame_rate;
		break;

    case NMI_VDOENC_PARAM_ENCBUF_MS:
        *pValue = gNMRVdoEncObj[Id].MemInfo.uiEncBufMs;
        break;

    case NMI_VDOENC_PARAM_ENCBUF_RESERVED_MS:
        *pValue = gNMRVdoEncObj[Id].MemInfo.uiEncBufReservedMs;
        break;

	case NMI_VDOENC_PARAM_CODEC:
		*pValue = gNMRVdoEncObj[Id].uiVidCodec;
		break;

	case NMI_VDOENC_PARAM_PROFILE:
		*pValue = gNMRVdoEncObj[Id].InitInfo.e_profile;
		break;

	case NMI_VDOENC_PARAM_GOPTYPE:
		*pValue = gNMRVdoEncObj[Id].uiGopType;
		break;

	case NMI_VDOENC_PARAM_INITQP:
		*pValue = gNMRVdoEncObj[Id].InitInfo.init_i_qp;
		break;

	case NMI_VDOENC_PARAM_MINQP:
		*pValue = gNMRVdoEncObj[Id].InitInfo.min_i_qp;
		break;

	case NMI_VDOENC_PARAM_MAXQP:
		*pValue = gNMRVdoEncObj[Id].InitInfo.max_i_qp;
		break;

	case NMI_VDOENC_PARAM_TARGETRATE:
		*pValue = gNMRVdoEncObj[Id].InitInfo.byte_rate;
		break;

	case NMI_VDOENC_PARAM_DAR:
		*pValue = gNMRVdoEncObj[Id].InitInfo.e_dar;
		break;

	case NMI_VDOENC_PARAM_SVC:
		*pValue = gNMRVdoEncObj[Id].InitInfo.e_svc;
		break;

	case NMI_VDOENC_PARAM_LTR:
		if (pValue) {
			MP_VDOENC_LTR_INFO *pInfo = (MP_VDOENC_LTR_INFO *)pValue;
			pInfo->uiLTRInterval = gNMRVdoEncObj[Id].InitInfo.ltr_interval;
			pInfo->uiLTRPreRef = gNMRVdoEncObj[Id].InitInfo.ltr_pre_ref;
		}
		break;

	case NMI_VDOENC_PARAM_AQINFO:
		if (pValue) {
			MP_VDOENC_AQ_INFO *p_aq_info = (MP_VDOENC_AQ_INFO *)pValue;
			if ((MP_H265Enc_getVideoObject((MP_VDOENC_ID)Id)) && (MP_H265Enc_getVideoObject((MP_VDOENC_ID)Id)->GetInfo)) {
				(MP_H265Enc_getVideoObject((MP_VDOENC_ID)Id)->GetInfo)(VDOENC_GET_AQ, (UINT32 *)p_aq_info, 0, 0);
			}
		}
		break;

	case NMI_VDOENC_PARAM_ROWRC:
		if (pValue) {
			MP_VDOENC_ROWRC_INFO *p_rowrc_info = (MP_VDOENC_ROWRC_INFO *)pValue;
			if ((MP_H265Enc_getVideoObject((MP_VDOENC_ID)Id)) && (MP_H265Enc_getVideoObject((MP_VDOENC_ID)Id)->GetInfo)) {
				(MP_H265Enc_getVideoObject((MP_VDOENC_ID)Id)->GetInfo)(VDOENC_GET_ROWRC, (UINT32 *)p_rowrc_info, 0, 0);
			}
		}
		break;

	case NMI_VDOENC_PARAM_MULTI_TEMPORARY_LAYER:
		*pValue = gNMRVdoEncObj[Id].InitInfo.multi_layer;
		break;

	case NMI_VDOENC_PARAM_USRQP:
		*pValue = gNMRVdoEncObj[Id].InitInfo.user_qp_en;
		break;

	case NMI_VDOENC_PARAM_ROTATE:
		*pValue = gNMRVdoEncObj[Id].InitInfo.rotate;
		break;

	case NMI_VDOENC_PARAM_IMGFMT:
		*pValue = gNMRVdoEncJpgYuvFormat[Id];
		break;

	case NMI_VDOENC_PARAM_JPG_QUALITY:
		*pValue = gNMRVdoTrigObj[Id].EncParam.quality;  // jpeg encode quality
		break;

	case NMI_VDOENC_PARAM_TIMERRATE_IMM:
		*pValue = gNMRVdoEncObj[Id].iTimerInterval;
		break;

	case NMI_VDOENC_PARAM_TIMERRATE2_IMM:
		*pValue = gNMRVdoEncObj[Id].iTimerInterval;
		break;

	case NMI_VDOENC_PARAM_IS_ENCODING:
		*pValue = (gNMRVdoTrigObj[Id].uiEncStatus == NMR_VDOENC_ENC_STATUS_ENCODE_TRIG)? 1:0;
		break;

	case NMI_VDOENC_PARAM_PARTIAL_BS_ADDR:
		*pValue = gNMRVdoTrigObj[Id].BsNow;
		break;

	case NMI_VDOENC_PARAM_DESC_LEN:
		*pValue = gNMRVdoEncObj[Id].uiDescSize;
		break;

	case NMI_VDOENC_PARAM_TRIGBSQ_SIZE:
		*pValue = sizeof(NMI_VDOENC_MEM_RANGE);
		break;

	case NMI_VDOENC_PARAM_JPG_YUV_TRAN_ENABLE:
		*pValue = gNMRVdoTrigObj[Id].EncParam.fmt_trans_en;
		break;

	case NMI_VDOENC_PARAM_JPG_RC_MIN_QUALITY:
		*pValue = gNMRVdoTrigObj[Id].EncParam.min_quality;
		break;

	case NMI_VDOENC_PARAM_JPG_RC_MAX_QUALITY:
		*pValue = gNMRVdoTrigObj[Id].EncParam.max_quality;
		break;

	case NMI_VDOENC_PARAM_SNAPSHOT_SIZE:
		*pValue = _NMR_VdoEnc_GetSnapshotSize(Id);
		break;

	case NMI_VDOENC_PARAM_JPG_JFIF_ENABLE:
		*pValue = gNMRVdoTrigObj[Id].EncParam.jpeg_jfif_en;
		break;

	default:
		DBG_ERR("[VDOENC][%d] Get invalid param = %d\r\n", Id, Param);
		return E_PAR;
	}
	return E_OK;
}

static ER NMI_VdoEnc_Action(UINT32 Id, UINT32 Action)
{
	ER status = E_SYS;
    UINT32 i = 0;
	MP_VDOENC_ENCODER *pVidEnc = NULL;

	switch (Action) {
	case NMI_VDOENC_ACTION_START:
#if MP_VDOENC_SHOW_MSG
#if 0
		DBG_DUMP("[VDOENC][%d] Action = %d, Codec = %d, Mode = %d, Size = (%d, %d), Fr = %d, Trig = %d, Alloc size = (%d, %d, %d), Enc Buf = (%d ms, %d ms, 0x%08x, 0x%08x, 0x%08x, %d), Min Size(I, P) = (%d, %d)\r\n",
#else
		(&isf_vdoenc)->p_base->do_trace(&isf_vdoenc, ISF_OUT(Id), ISF_OP_STATE, "[VDOENC][%d] Action = %d, Codec = %d, Mode = %d, Time = %d, Size = (%d, %d), Fr = %u.%03u, Trig = %d, Alloc size = (%d, %d, %d), Enc Buf = (%d ms, %d ms, 0x%08x, 0x%08x, 0x%08x, %d), Min Size(I, P) = (%d, %d)\r\n",
#endif
				Id,
				Action,
				gNMRVdoEncObj[Id].uiVidCodec,
				gNMRVdoEncObj[Id].uiRecFormat,
				gNMRVdoEncObj[Id].uiTimelapseTime,
				gNMRVdoEncObj[Id].InitInfo.width,
				gNMRVdoEncObj[Id].InitInfo.height,
				(gNMRVdoEncObj[Id].InitInfo.frame_rate/1000),
				(gNMRVdoEncObj[Id].InitInfo.frame_rate%1000),
				gNMRVdoEncObj[Id].uiTriggerMode,
				gNMRVdoEncObj[Id].uiSnapShotSize,
				gNMRVdoEncObj[Id].uiCodecSize,
				gNMRVdoEncObj[Id].uiEncSize,
				gNMRVdoEncObj[Id].MemInfo.uiEncBufMs,
				gNMRVdoEncObj[Id].MemInfo.uiEncBufReservedMs,
				gNMRVdoEncObj[Id].MemInfo.uiBSStart,
				gNMRVdoEncObj[Id].MemInfo.uiBSEnd,
				gNMRVdoEncObj[Id].MemInfo.uiBSMax,
				gNMRVdoEncObj[Id].MemInfo.uiBSEnd - gNMRVdoEncObj[Id].MemInfo.uiBSStart,
				gNMRVdoEncObj[Id].MemInfo.uiMinISize,
				gNMRVdoEncObj[Id].MemInfo.uiMinPSize
				);
#endif

		if (gNMRVdoEncObj[Id].bStart == FALSE) {
			if (gNMRVdoEncObj[Id].uiRecFormat == 0)	{
				DBG_ERR("[VDOENC][%d] Invalid rec format = %d\r\n", Id, gNMRVdoEncObj[Id].uiRecFormat);
				return E_SYS;
			}
			NMR_VdoTrig_CreatePluginObject(Id);
			NMR_VdoEnc_SetEncodeParam(Id);
			NMR_VdoEnc_SetDesc(Id);

			// Configure Video Stream Object
			//_NMR_VdoEnc_InitVtrigObj(Id);//create then init
			NMR_VdoTrig_InitParam(Id);
			if (gNMRVdoEncObj[Id].uiTriggerMode == NMI_VDOENC_TRIGGER_TIMER || gNMRVdoEncObj[Id].uiTriggerMode == NMI_VDOENC_TRIGGER_NOTIFY) {
				if (gNMRVdoEncObj[Id].bStartTimerByManual != TRUE) {
					_NMR_VdoEnc_SetEncTriggerTimer(Id);
				}
			}
            if (gNMRVdoEncFirstPathID == NMR_VDOENC_MAX_PATH && gNMRVdoEncObj[Id].uiRecFormat != MEDIAREC_LIVEVIEW) {
                gNMRVdoEncFirstPathID = Id;
            }
			gNMRVdoEncObj[Id].bTimelapseTrigger = TRUE;
			gNMRVdoEncObj[Id].bSnapshot = FALSE;
			gNMRVdoEncObj[Id].bResetEncoder = FALSE;
			gNMRVdoEncObj[Id].uiPreEncDropCount = 0;
			gNMRVdoEncObj[Id].uiPreEncInCount = 0;
			gNMRVdoEncObj[Id].uiPreEncErrCount = 0;
			gNMRVdoEncObj[Id].uiEncDropCount = 0;
			gNMRVdoEncObj[Id].uiEncInCount = 0;
			gNMRVdoEncObj[Id].uiEncOutCount = 0;
			gNMRVdoEncObj[Id].uiEncReCount = 0;
			gNMRVdoEncObj[Id].uiEncErrCount = 0;
			gNMRVdoEncObj[Id].bStart = TRUE;
			status = E_OK;
		} else {
			DBG_ERR("[VDOENC][%d] invalid operation, start again before stopping", Id);
			status = E_SYS;
		}
		break;

	case NMI_VDOENC_ACTION_STOP: {
			while (gNMRVdoEncObj[Id].bStart == FALSE && i < 10) {
				DELAY_M_SEC(100);
				i++;
			}
			if (gNMRVdoEncObj[Id].bStart == TRUE) {
				gNMRVdoEncObj[Id].bStart = FALSE;
				_NMR_VdoTrig_YUV_TMOUT_ResourceUnlock(Id);  // let push_in(blocking mode) auto-unlock, give up & release YUV (because gNMRVdoEncObj[Id].bStart = FALSE  , so it will NOT try again)

#if MP_VDOENC_SHOW_MSG
#if 0
				DBG_DUMP("[VDOENC][%d] Action = %d\r\n", Id, Action);
#else
				(&isf_vdoenc)->p_base->do_trace(&isf_vdoenc, ISF_OUT(Id), ISF_OP_STATE, "[VDOENC][%d] Action = %d\r\n", Id, Action);
#endif
#endif
				pVidEnc = NMR_VdoTrig_GetVidEncoder(Id);
				if (gNMRVdoEncObj[Id].uiTriggerMode == NMI_VDOENC_TRIGGER_TIMER || gNMRVdoEncObj[Id].uiTriggerMode == NMI_VDOENC_TRIGGER_NOTIFY) {
					_NMR_VdoEnc_CloseTriggerTimer(Id);
				}

				_NMR_VdoEnc_CloseFpsTimer(Id);

	            if (gNMRVdoEncFirstPathID == Id)
	            {
	                for (i = 0; i < PATH_MAX_COUNT; i++) {
	                    if (gNMRVdoEncObj[i].bStart == TRUE && gNMRVdoEncObj[i].uiRecFormat != MEDIAREC_LIVEVIEW) {
	                        gNMRVdoEncFirstPathID = i;
	                        break;
	                    }
	                }

	                if (i == PATH_MAX_COUNT)
	                    gNMRVdoEncFirstPathID = NMR_VDOENC_MAX_PATH;
	            }

				NMR_VdoTrig_Stop(Id);

				// wait video task idle, then close H26x Driver
				NMR_VdoTrig_WaitTskIdle(Id);
				if ((pVidEnc) && (pVidEnc->Close)) {
					(pVidEnc->Close)();
				}
#ifdef __KERNEL__
				gNMRVdoEnc1stStart[Id] = 0;
#endif
				status = E_OK;
			} else {
				DBG_DUMP("[VDOENC][%d] Invalid Action = %d, not start yet\r\n", Id, Action);
				status = E_SYS;
			}
			break;
		}

	default:
		DBG_ERR("[VDOENC][%d] Invalid action = %d\r\n", Id, Action);
		break;
	}

	return status;
}

static ER NMI_VdoEnc_In(UINT32 Id, UINT32 *pBuf)
{
	DBG_MSG("[VDOENC][%d] Input data\r\n", Id);

	if ((gNMRVdoEncObj[Id].uiRecFormat == MEDIAREC_TIMELAPSE) || (gNMRVdoEncObj[Id].uiTimelapseTime > 0)) {
		if (gNMRVdoEncObj[Id].bTimelapseTrigger != TRUE) {
			return E_RLWAI;
		} else {
			gNMRVdoEncObj[Id].bTimelapseTrigger = FALSE;
			if (NMR_VdoTrig_PutYuv(Id, (MP_VDOENC_YUV_SRC *) pBuf) == FALSE) {
				return E_SYS;
			}
			NMR_VdoTrig_PutJob(Id);
			NMR_VdoTrig_TrigEncode(Id);
		}
	} else {
		if (NMR_VdoTrig_PutYuv(Id, (MP_VDOENC_YUV_SRC *) pBuf) == FALSE) {
			return E_SYS;
		}
	}

	if (gNMRVdoEncObj[Id].uiTriggerMode == NMI_VDOENC_TRIGGER_DIRECT || gNMRVdoEncObj[Id].uiTriggerMode == NMI_VDOENC_TRIGGER_NOTIFY) {
		NMR_VdoTrig_PutJob(Id);
		NMR_VdoTrig_TrigEncode(Id);
	}
	return E_OK;
}

static ER NMI_VdoEnc_Close(UINT32 Id)
{
	gNMRVdoEncPathCount--;

	if (gNMRVdoEncPathCount == 0) {
		_NMR_VdoEnc_Close_H26X();
		_NMR_VdoEnc_Close_JPEG();
#ifdef VDOENC_LL
		_NMR_VdoEnc_Close_WrapBS();
#endif
        gNMRVdoEncFirstPathID = NMR_VDOENC_MAX_PATH;
	}
	return E_OK;
}

NMI_UNIT NMI_VdoEnc = {
	.Name           = "VdoEnc",
	.Open           = NMI_VdoEnc_Open,
	.SetParam       = NMI_VdoEnc_SetParam,
	.GetParam       = NMI_VdoEnc_GetParam,
	.Action         = NMI_VdoEnc_Action,
	.In             = NMI_VdoEnc_In,
	.Close          = NMI_VdoEnc_Close,
};

void nmr_vdoenc_install_id(void)
{
    UINT32 i = 0;
#if defined __UITRON || defined __ECOS
	OS_CONFIG_TASK(NMR_VDOTRIG_D2DTSK_ID_H26X,          NMR_VDOTRIG_TASK_PRIORITY,    NMR_VDOTRIG_STACK_SIZE,      NMR_VdoTrig_D2DTsk_H26X);
	OS_CONFIG_TASK(NMR_VDOTRIG_D2DTSK_ID_H26X_SNAPSHOT, NMR_VDOTRIG_TASK_PRIORITY,    NMR_VDOTRIG_STACK_SIZE,      NMR_VdoTrig_D2DTsk_H26X_Snapshot);
	OS_CONFIG_TASK(NMR_VDOTRIG_D2DTSK_ID_JPEG,          NMR_VDOTRIG_TASK_PRIORITY,    NMR_VDOTRIG_STACK_SIZE,      NMR_VdoTrig_D2DTsk_JPEG);
#ifdef VDOENC_LL
	OS_CONFIG_TASK(NMR_VDOTRIG_D2DTSK_ID_WRAPBS,        NMR_VDOTRIG_TASK_PRIORITY,    NMR_VDOTRIG_STACK_SIZE,      NMR_VdoTrig_D2DTsk_WrapBS);
#endif
#endif

	OS_CONFIG_FLAG(FLG_ID_NMR_VDOTRIG_H26X);
	OS_CONFIG_FLAG(FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT);
	OS_CONFIG_FLAG(FLG_ID_NMR_VDOTRIG_JPEG);
#ifdef VDOENC_LL
	OS_CONFIG_FLAG(FLG_ID_NMR_VDOTRIG_WRAPBS);
#endif

    for (i = 0; i < PATH_MAX_COUNT; i++) {
        //OS_CONFIG_SEMPHORE(NMR_VDOENC_SEM_ID[i], 0, 1, 1);
#ifdef VDOENC_LL
        OS_CONFIG_SEMPHORE(NMR_VDOENC_TRIG_SEM_ID[i], 0, 1, 1);
#endif
        OS_CONFIG_SEMPHORE(NMR_VDOENC_YUV_SEM_ID[i], 0, 0, 0);
    }

    OS_CONFIG_SEMPHORE(NMR_VDOENC_COMMON_SEM_ID, 0, 1, 1);
    //OS_CONFIG_SEMPHORE(NMR_VDOCODEC_H26X_SEM_ID, 0, 1, 1);
    //OS_CONFIG_SEMPHORE(NMR_VDOCODEC_JPEG_SEM_ID, 0, 1, 1);
}

void nmr_vdocodec_install_id(void)
{
    OS_CONFIG_SEMPHORE(NMR_VDOCODEC_H26X_SEM_ID, 0, 1, 1);
    OS_CONFIG_SEMPHORE(NMR_VDOCODEC_JPEG_SEM_ID, 0, 1, 1);
}

void nmr_vdoenc_uninstall_id(void)
{
    UINT32 i = 0;

    rel_flg(FLG_ID_NMR_VDOTRIG_H26X);
    rel_flg(FLG_ID_NMR_VDOTRIG_H26X_SNAPSHOT);
    rel_flg(FLG_ID_NMR_VDOTRIG_JPEG);
#ifdef VDOENC_LL
    rel_flg(FLG_ID_NMR_VDOTRIG_WRAPBS);
#endif

    for (i = 0; i < PATH_MAX_COUNT; i++) {
        //SEM_DESTROY(NMR_VDOENC_SEM_ID[i]);
#ifdef VDOENC_LL
        SEM_DESTROY(NMR_VDOENC_TRIG_SEM_ID[i]);
#endif
        SEM_DESTROY(NMR_VDOENC_YUV_SEM_ID[i]);
    }
    SEM_DESTROY(NMR_VDOENC_COMMON_SEM_ID);
    //SEM_DESTROY(NMR_VDOCODEC_H26X_SEM_ID);
    //SEM_DESTROY(NMR_VDOCODEC_JPEG_SEM_ID);
}

void nmr_vdocodec_uninstall_id(void)
{
    SEM_DESTROY(NMR_VDOCODEC_H26X_SEM_ID);
    SEM_DESTROY(NMR_VDOCODEC_JPEG_SEM_ID);
}

void VdoCodec_H26x_Lock(void)
{
    SEM_WAIT(NMR_VDOCODEC_H26X_SEM_ID);
}

void VdoCodec_H26x_UnLock(void)
{
    SEM_SIGNAL(NMR_VDOCODEC_H26X_SEM_ID);
}

void VdoCodec_JPEG_Lock(void)
{
    SEM_WAIT(NMR_VDOCODEC_JPEG_SEM_ID);
}

void VdoCodec_JPEG_UnLock(void)
{
    SEM_SIGNAL(NMR_VDOCODEC_JPEG_SEM_ID);
}

static BOOL Cmd_VdoEnc_ReEncTest(CHAR *strCmd)
{
	UINT32 id = 0, MaxMs = 0, SizePerSec = 0, EncBufSize = 0;
	sscanf_s(strCmd, "%d %d", &id, &MaxMs);
	DBG_DUMP("[VDOENC][%d] ReEncTest MaxMs = %d\r\n", id, MaxMs);

	if (MaxMs > gNMRVdoEncObj[id].MemInfo.uiEncBufMs) {
		DBG_ERR("[VDOENC][%d] ReEncTest fail, MaxMs(%d) > Ori EncBufMs(%d)\r\n", id, MaxMs, gNMRVdoEncObj[id].MemInfo.uiEncBufMs);
		return FALSE;
	}

	SizePerSec = NMR_VdoEnc_GetBytesPerSecond(id);
#ifdef __KERNEL__
	gNMRVdoEncObj[id].MemInfo.uiBSEnd = gNMRVdoEncObj[id].MemInfo.uiBSStart + (UINT32)div_u64((UINT64)SizePerSec * (UINT64)MaxMs, 1000) - NMR_VDOENC_BUF_RESERVED_BYTES;
#else
	gNMRVdoEncObj[id].MemInfo.uiBSEnd = gNMRVdoEncObj[id].MemInfo.uiBSStart + ((UINT64)SizePerSec * (UINT64)MaxMs/1000) - NMR_VDOENC_BUF_RESERVED_BYTES;
#endif
	gNMRVdoEncObj[id].MemInfo.uiBSMax = gNMRVdoEncObj[id].MemInfo.uiBSEnd;

	EncBufSize = gNMRVdoEncObj[id].MemInfo.uiBSEnd - gNMRVdoEncObj[id].MemInfo.uiBSStart;
#ifdef __KERNEL__
	gNMRVdoEncObj[id].MemInfo.uiMinISize = (UINT32)div_u64((UINT64)SizePerSec * (UINT64)gNMRVdoEncObj[id].MemInfo.uiMinIRatio,100);
	gNMRVdoEncObj[id].MemInfo.uiMinPSize = (UINT32)div_u64((UINT64)SizePerSec * (UINT64)gNMRVdoEncObj[id].MemInfo.uiMinPRatio,100);
#else
	gNMRVdoEncObj[id].MemInfo.uiMinISize = ((UINT64)SizePerSec * (UINT64)gNMRVdoEncObj[id].MemInfo.uiMinIRatio/100);
	gNMRVdoEncObj[id].MemInfo.uiMinPSize = ((UINT64)SizePerSec * (UINT64)gNMRVdoEncObj[id].MemInfo.uiMinPRatio/100);
#endif
	if (gNMRVdoEncObj[id].MemInfo.uiMinISize > EncBufSize) {
		gNMRVdoEncObj[id].MemInfo.uiMinISize = EncBufSize;
	}
	if (gNMRVdoEncObj[id].MemInfo.uiMinPSize > EncBufSize) {
		gNMRVdoEncObj[id].MemInfo.uiMinPSize = EncBufSize;
	}

	NMI_VdoEnc_Action(id, NMI_VDOENC_ACTION_STOP);
	NMI_VdoEnc_Action(id, NMI_VDOENC_ACTION_START);
	return TRUE;
}

static BOOL Cmd_VdoEnc_ResetIFrmame(CHAR *strCmd)
{
	UINT32 id = 0;
	sscanf_s(strCmd, "%d", &id);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (id >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", id);
		return FALSE;
	}
#endif
	DBG_DUMP("[VDOENC][%d] ResetIFrame\r\n", id);
	gNMRVdoEncObj[id].bResetIFrame = TRUE;
	return TRUE;
}

static BOOL Cmd_VdoEnc_DumpMem(CHAR *strCmd)
{
	UINT32 uiAddr = 0, uiSize = 0;
	sscanf_s(strCmd, "%x %x", &uiAddr, &uiSize);
	debug_dumpmem(uiAddr, uiSize);
	return TRUE;
}

static BOOL Cmd_VdoEnc_EncInfo(CHAR *strCmd)
{
	UINT32 uiPathID = 0;
	sscanf_s(strCmd, "%d", &uiPathID);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	NMR_VdoEnc_EncInfo(uiPathID);
	return TRUE;
}

static BOOL Cmd_VdoEnc_FpsInfo(CHAR *strCmd)
{
	UINT32 uiPathID = 0, uiEnable = 0;
	sscanf_s(strCmd, "%d %d", &uiPathID, &uiEnable);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	NMR_VdoEnc_FpsInfo(uiPathID, uiEnable);
	return TRUE;
}

static BOOL Cmd_VdoEnc_RcInfo(CHAR *strCmd)
{
	UINT32 uiPathID = 0, uiEnable = 0;
	sscanf_s(strCmd, "%d %d", &uiPathID, &uiEnable);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	NMR_VdoEnc_RcInfo(uiPathID, uiEnable);
	return TRUE;
}

static BOOL Cmd_VdoEnc_SetAQ(CHAR *strCmd)
{
	UINT32 uiPathID = 0;
	MP_VDOENC_AQ_INFO AqInfo = {0};

	sscanf_s(strCmd, "%d %d %d %d %d %d",
			 &uiPathID,
			 &AqInfo.enable,
			 &AqInfo.i_str,
			 &AqInfo.p_str,
			 &AqInfo.max_delta_qp,
			 &AqInfo.min_delta_qp);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	NMR_VdoEnc_SetAQ(uiPathID, &AqInfo);
	return TRUE;
}

static BOOL Cmd_VdoEnc_SetGDR(CHAR *strCmd)
{
	UINT32 uiPathID = 0;
	MP_VDOENC_GDR_INFO GdrInfo = {0};

	sscanf_s(strCmd, "%d %d %d %d",
			 &uiPathID,
			 &GdrInfo.enable,
			 &GdrInfo.period,
			 &GdrInfo.number);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	NMR_VdoEnc_SetGDR(uiPathID, &GdrInfo);
	return TRUE;
}

static BOOL Cmd_VdoEnc_SetSliceSplit(CHAR *strCmd)
{
	UINT32 uiPathID = 0;
	MP_VDOENC_SLICESPLIT_INFO SliceSplitInfo = {0};

	sscanf_s(strCmd, "%d %d %d",
			 &uiPathID,
			 &SliceSplitInfo.enable,
			 &SliceSplitInfo.slice_row_num);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	NMR_VdoEnc_SetSliceSplit(uiPathID, &SliceSplitInfo);
	return TRUE;
}

static BOOL Cmd_VdoEnc_SetCBR(CHAR *strCmd)
{
	UINT32 uiPathID = 0;
	MP_VDOENC_CBR_INFO CbrInfo = {0};

	sscanf_s(strCmd, "%d %d %d %d %d %d %d %d %d %d %d %d %d",
			 &uiPathID,
			 &CbrInfo.enable,
			 &CbrInfo.static_time,
			 &CbrInfo.byte_rate,
			 &CbrInfo.frame_rate,
			 &CbrInfo.gop,
			 &CbrInfo.init_i_qp,
			 &CbrInfo.min_i_qp,
			 &CbrInfo.max_i_qp,
			 &CbrInfo.init_p_qp,
			 &CbrInfo.min_p_qp,
			 &CbrInfo.max_p_qp,
			 &CbrInfo.ip_weight);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	NMR_VdoEnc_SetCBR(uiPathID, &CbrInfo);
	return TRUE;
}

static BOOL Cmd_VdoEnc_SetEVBR(CHAR *strCmd)
{
	UINT32 uiPathID = 0;
	MP_VDOENC_EVBR_INFO EVbrInfo = {0};

	sscanf_s(strCmd, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
			&uiPathID,
			&EVbrInfo.enable,
			&EVbrInfo.static_time,
			&EVbrInfo.byte_rate,
			&EVbrInfo.frame_rate,
			&EVbrInfo.gop,
			&EVbrInfo.key_p_period,
			&EVbrInfo.init_i_qp,
			&EVbrInfo.min_i_qp,
			&EVbrInfo.max_i_qp,
			&EVbrInfo.init_p_qp,
			&EVbrInfo.min_p_qp,
			&EVbrInfo.max_p_qp,
			&EVbrInfo.ip_weight,
			&EVbrInfo.kp_weight,
			&EVbrInfo.motion_aq_st,
			&EVbrInfo.still_frm_cnd,
			&EVbrInfo.motion_ratio_thd,
			&EVbrInfo.i_psnr_cnd,
			&EVbrInfo.p_psnr_cnd,
			&EVbrInfo.kp_psnr_cnd);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	NMR_VdoEnc_SetEVBR(uiPathID, &EVbrInfo);

	return TRUE;
}

static BOOL Cmd_VdoEnc_SetFixQP(CHAR *strCmd)
{
	UINT32 uiPathID = 0;
	MP_VDOENC_FIXQP_INFO FixQpInfo = {0};

	sscanf_s(strCmd, "%d %d %d %d",
			 &uiPathID,
			 &FixQpInfo.enable,
			 &FixQpInfo.fix_i_qp,
			 &FixQpInfo.fix_p_qp);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	NMR_VdoEnc_SetFixQP(uiPathID, &FixQpInfo);

	return TRUE;
}

static BOOL Cmd_VdoEnc_SetROI(CHAR *strCmd)
{
	UINT32 uiPathID = 0, uiIndex = 0;
	MP_VDOENC_ROI_INFO RoiInfo = {0};

	sscanf_s(strCmd, "%d %d %d %d %d %d %d %d %d",
			&uiPathID,
			&uiIndex,
			&RoiInfo.st_roi[uiIndex].enable,
			&RoiInfo.st_roi[uiIndex].qp,
			&RoiInfo.st_roi[uiIndex].qp_mode,
			&RoiInfo.st_roi[uiIndex].coord_X,
			&RoiInfo.st_roi[uiIndex].coord_Y,
			&RoiInfo.st_roi[uiIndex].width,
			&RoiInfo.st_roi[uiIndex].height);

	DBG_DUMP("[VDOENC][%d] Set ROI Index = %d, Enable = %d, QP = %d, QPMode = %d, Coord_X = %d, Coord_Y = %d, Width = %d, Height = %d\r\n",
			uiPathID,
			uiIndex,
			RoiInfo.st_roi[uiIndex].enable,
			RoiInfo.st_roi[uiIndex].qp,
			RoiInfo.st_roi[uiIndex].qp_mode,
			RoiInfo.st_roi[uiIndex].coord_X,
			RoiInfo.st_roi[uiIndex].coord_Y,
			RoiInfo.st_roi[uiIndex].width,
			RoiInfo.st_roi[uiIndex].height);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	NMR_VdoEnc_SetROI(uiPathID, &RoiInfo);

	return TRUE;
}

static BOOL Cmd_VdoEnc_SetSmartROI(CHAR *strCmd)
{
	UINT32 uiPathID = 0, uiEnable = 0;
	sscanf_s(strCmd, "%d %d", &uiPathID, &uiEnable);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	NMR_VdoEnc_SetSmartROI(uiPathID, uiEnable);
	return TRUE;
}

static BOOL Cmd_VdoEnc_SetVBR(CHAR *strCmd)
{
	UINT32 uiPathID = 0;
	MP_VDOENC_VBR_INFO VbrInfo = {0};

	sscanf_s(strCmd, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d",
			 &uiPathID,
			 &VbrInfo.enable,
			 &VbrInfo.static_time,
			 &VbrInfo.byte_rate,
			 &VbrInfo.frame_rate,
			 &VbrInfo.gop,
			 &VbrInfo.init_i_qp,
			 &VbrInfo.min_i_qp,
			 &VbrInfo.max_i_qp,
			 &VbrInfo.init_p_qp,
			 &VbrInfo.min_p_qp,
			 &VbrInfo.max_p_qp,
			 &VbrInfo.ip_weight,
			 &VbrInfo.change_pos);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	NMR_VdoEnc_SetVBR(uiPathID, &VbrInfo);

	return TRUE;
}

static BOOL Cmd_VdoEnc_ShowMsg(CHAR *strCmd)
{
	UINT32 uiPathID = 0, uiMsgLevel = 0;
	sscanf_s(strCmd, "%d %d", &uiPathID, &uiMsgLevel);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	NMR_VdoEnc_ShowMsg(uiPathID, uiMsgLevel);
	return TRUE;
}

static BOOL Cmd_VdoEnc_SkipFrame(CHAR *strCmd)
{
	UINT32 uiPathID = 0, uiSkipFrame = 0;
	sscanf_s(strCmd, "%d %d", &uiPathID, &uiSkipFrame);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	DBG_DUMP("[VDOENC][%d] SkipFrame = %d\r\n", uiPathID, uiSkipFrame);
	gNMRVdoEncObj[uiPathID].uiSkipFrame = uiSkipFrame;
	return TRUE;
}

static BOOL Cmd_VdoEnc_ResetEncoder(CHAR *strCmd)
{
	UINT32 uiPathID = 0;
	sscanf_s(strCmd, "%d", &uiPathID);
#if NMR_VDOENC_DYNAMIC_CONTEXT
	if (uiPathID >= PATH_MAX_COUNT) {
		DBG_ERR("[VDOENC] invalid path index = %d\r\n", uiPathID);
		return FALSE;
	}
#endif
	gNMRVdoEncObj[uiPathID].bResetEncoder = TRUE;
	return TRUE;
}

#ifdef __KERNEL__
static BOOL Cmd_VdoEnc_Fastboot_EncInfo(CHAR *strCmd)
{
	UINT32 uiFastbootParam = 0;
	int 			 fd, len;
	char			 tmp_buf[128]="/mnt/sd/nvt-na51055-fastboot-venc.dtsi";
	UINT32 i= 0;
	MP_VDOENC_CBR_INFO FastbootCBR = {0};
	MP_VDOENC_EVBR_INFO FastbootEVBR = {0};
	MP_VDOENC_VBR_INFO FastbootVBR = {0};
	MP_VDOENC_FIXQP_INFO FastbootFIXQP = {0};

	sscanf(strCmd, "%s", tmp_buf);

	fd = vos_file_open(tmp_buf, O_CREAT|O_WRONLY|O_SYNC, 0);
	if ((VOS_FILE)(-1) == fd) {
		printk("open %s failure\r\n", tmp_buf);
		return FALSE;
	}
	len = snprintf(tmp_buf, sizeof(tmp_buf), "&fastboot {\r\n\tvenc {\r\n");
	vos_file_write(fd, (void *)tmp_buf, len);

	for (i = 0; i < BUILTIN_VDOENC_PATH_ID_MAX; i++) {
		if (gNMRVdoEncObj[i].bStart) {
			len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tvenc%d {\r\n", i);
			vos_file_write(fd, (void *)tmp_buf, len);

			len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tenable = <1>;\r\n");
			vos_file_write(fd, (void *)tmp_buf, len);

			if (gNMRVdoEncObj[i].uiVidFastbootCodec == NMEDIAREC_ENC_H265) {
				uiFastbootParam = 0;
			} else if (gNMRVdoEncObj[i].uiVidFastbootCodec == NMEDIAREC_ENC_H264) {
				uiFastbootParam = 1;
			} else if (gNMRVdoEncObj[i].uiVidFastbootCodec == NMEDIAREC_ENC_MJPG) {
				uiFastbootParam = 2;
			}
			len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tcodectype = <%d>;\r\n", uiFastbootParam);
			vos_file_write(fd, (void *)tmp_buf, len);

			uiFastbootParam = gNMRVdoEncObj[i].uiVidFastbootVprcSrcDev;
			len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tvprc_src_dev = <%d>;\r\n", uiFastbootParam);
			vos_file_write(fd, (void *)tmp_buf, len);

			uiFastbootParam = gNMRVdoEncObj[i].uiVidFastbootVprcSrcPath;
			len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tvprc_src_path = <%d>;\r\n", uiFastbootParam);
			vos_file_write(fd, (void *)tmp_buf, len);

			uiFastbootParam = gNMRVdoEncObj[i].InitInfo.byte_rate;
			len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tinit_byte_rate = <%d>;\r\n", uiFastbootParam);
			vos_file_write(fd, (void *)tmp_buf, len);

			if (gNMRVdoEncObj[i].uiVidFastbootCodec != NMEDIAREC_ENC_MJPG) {
				uiFastbootParam = gNMRVdoEncObj[i].InitInfo.frame_rate/1000;
				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tframerate = <%d>;\r\n", uiFastbootParam);
				vos_file_write(fd, (void *)tmp_buf, len);

				VdoEnc_Builtin_GetParam(0, BUILTIN_VDOENC_INIT_PARAM_SEC, &uiFastbootParam);
				if (uiFastbootParam == 0) {
					uiFastbootParam = 3;
				}
				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tsec = <%d>;\r\n", uiFastbootParam);
				vos_file_write(fd, (void *)tmp_buf, len);

				uiFastbootParam = gNMRVdoEncObj[i].InitInfo.gop;
				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tgop = <%d>;\r\n", uiFastbootParam);
				vos_file_write(fd, (void *)tmp_buf, len);

				if (gNMRVdoEncObj[i].CbrInfo.enable) {
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\trc = <1>;\r\n");
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.init_i_qp = gNMRVdoEncObj[i].CbrInfo.init_i_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tinit_i_qp = <%d>;\r\n", FastbootCBR.init_i_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.min_i_qp = gNMRVdoEncObj[i].CbrInfo.min_i_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmin_i_qp = <%d>;\r\n", FastbootCBR.min_i_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.max_i_qp = gNMRVdoEncObj[i].CbrInfo.max_i_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmax_i_qp = <%d>;\r\n", FastbootCBR.max_i_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.init_p_qp = gNMRVdoEncObj[i].CbrInfo.init_p_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tinit_p_qp = <%d>;\r\n", FastbootCBR.init_p_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.min_p_qp = gNMRVdoEncObj[i].CbrInfo.min_p_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmin_p_qp = <%d>;\r\n", FastbootCBR.min_p_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.max_p_qp = gNMRVdoEncObj[i].CbrInfo.max_p_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmax_p_qp = <%d>;\r\n", FastbootCBR.max_p_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.byte_rate = gNMRVdoEncObj[i].CbrInfo.byte_rate;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\trc_byte_rate = <%d>;\r\n", FastbootCBR.byte_rate);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.static_time = gNMRVdoEncObj[i].CbrInfo.static_time;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tstatic_time = <%d>;\r\n", FastbootCBR.static_time);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.ip_weight = gNMRVdoEncObj[i].CbrInfo.ip_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tip_weight = <0x%x>;\r\n", FastbootCBR.ip_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.key_p_period = gNMRVdoEncObj[i].CbrInfo.key_p_period;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tkey_p_period = <%d>;\r\n", FastbootCBR.key_p_period);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.kp_weight = gNMRVdoEncObj[i].CbrInfo.kp_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tkp_weight = <0x%x>;\r\n", FastbootCBR.kp_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.p2_weight = gNMRVdoEncObj[i].CbrInfo.p2_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tp2_weight = <0x%x>;\r\n", FastbootCBR.p2_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.p3_weight = gNMRVdoEncObj[i].CbrInfo.p3_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tp3_weight = <0x%x>;\r\n", FastbootCBR.p3_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.lt_weight = gNMRVdoEncObj[i].CbrInfo.lt_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tlt_weight = <0x%x>;\r\n", FastbootCBR.lt_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.motion_aq_str = gNMRVdoEncObj[i].CbrInfo.motion_aq_str;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmotion_aq_str = <0x%x>;\r\n", FastbootCBR.motion_aq_str);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootCBR.svc_weight_mode = gNMRVdoEncObj[i].CbrInfo.svc_weight_mode;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tsvc_weight_mode = <0x%x>;\r\n", FastbootCBR.svc_weight_mode);
					vos_file_write(fd, (void *)tmp_buf, len);
				} else if (gNMRVdoEncObj[i].EVbrInfo.enable) {
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\trc = <5>;\r\n");
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.init_i_qp = gNMRVdoEncObj[i].EVbrInfo.init_i_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tinit_i_qp = <%d>;\r\n", FastbootEVBR.init_i_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.min_i_qp = gNMRVdoEncObj[i].EVbrInfo.min_i_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmin_i_qp = <%d>;\r\n", FastbootEVBR.min_i_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.max_i_qp = gNMRVdoEncObj[i].EVbrInfo.max_i_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmax_i_qp = <%d>;\r\n", FastbootEVBR.max_i_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.init_p_qp = gNMRVdoEncObj[i].EVbrInfo.init_p_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tinit_p_qp = <%d>;\r\n", FastbootEVBR.init_p_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.min_p_qp = gNMRVdoEncObj[i].EVbrInfo.min_p_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmin_p_qp = <%d>;\r\n", FastbootEVBR.min_p_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.max_p_qp = gNMRVdoEncObj[i].EVbrInfo.max_p_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmax_p_qp = <%d>;\r\n", FastbootEVBR.max_p_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.byte_rate = gNMRVdoEncObj[i].EVbrInfo.byte_rate;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\trc_byte_rate = <%d>;\r\n", FastbootEVBR.byte_rate);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.static_time = gNMRVdoEncObj[i].EVbrInfo.static_time;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tstatic_time = <%d>;\r\n", FastbootEVBR.static_time);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.ip_weight = gNMRVdoEncObj[i].EVbrInfo.ip_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tip_weight = <0x%x>;\r\n", FastbootEVBR.ip_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.kp_weight = gNMRVdoEncObj[i].EVbrInfo.kp_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tkp_weight = <0x%x>;\r\n", FastbootEVBR.kp_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.key_p_period = gNMRVdoEncObj[i].EVbrInfo.key_p_period;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tkey_p_period = <%d>;\r\n", FastbootEVBR.key_p_period);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.motion_aq_st = gNMRVdoEncObj[i].EVbrInfo.motion_aq_st;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmotion_aq_str = <0x%x>;\r\n", FastbootEVBR.motion_aq_st);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.still_frm_cnd = gNMRVdoEncObj[i].EVbrInfo.still_frm_cnd;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tstill_frm_cnd = <%d>;\r\n", FastbootEVBR.still_frm_cnd);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.motion_ratio_thd = gNMRVdoEncObj[i].EVbrInfo.motion_ratio_thd;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmotion_ratio_thd = <%d>;\r\n", FastbootEVBR.motion_ratio_thd);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.i_psnr_cnd = gNMRVdoEncObj[i].EVbrInfo.i_psnr_cnd;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\ti_psnr_cnd = <%d>;\r\n", FastbootEVBR.i_psnr_cnd);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.p_psnr_cnd = gNMRVdoEncObj[i].EVbrInfo.p_psnr_cnd;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tp_psnr_cnd = <%d>;\r\n", FastbootEVBR.p_psnr_cnd);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.kp_psnr_cnd = gNMRVdoEncObj[i].EVbrInfo.kp_psnr_cnd;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tkp_psnr_cnd = <%d>;\r\n", FastbootEVBR.kp_psnr_cnd);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.p2_weight = gNMRVdoEncObj[i].EVbrInfo.p2_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tp2_weight = <0x%x>;\r\n", FastbootEVBR.p2_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.p3_weight = gNMRVdoEncObj[i].EVbrInfo.p3_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tp3_weight = <0x%x>;\r\n", FastbootEVBR.p3_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.lt_weight = gNMRVdoEncObj[i].EVbrInfo.lt_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tlt_weight = <0x%x>;\r\n", FastbootEVBR.lt_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootEVBR.svc_weight_mode = gNMRVdoEncObj[i].EVbrInfo.svc_weight_mode;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tsvc_weight_mode = <0x%x>;\r\n", FastbootEVBR.svc_weight_mode);
					vos_file_write(fd, (void *)tmp_buf, len);
				} else if (gNMRVdoEncObj[i].VbrInfo.enable) {
					if (gNMRVdoEncObj[i].VbrInfo.policy == 1) {
						len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\trc = <3>;\r\n");
						vos_file_write(fd, (void *)tmp_buf, len);
					} else {
						len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\trc = <2>;\r\n");
						vos_file_write(fd, (void *)tmp_buf, len);
					}

					FastbootVBR.init_i_qp = gNMRVdoEncObj[i].VbrInfo.init_i_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tinit_i_qp = <%d>;\r\n", FastbootVBR.init_i_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.min_i_qp = gNMRVdoEncObj[i].VbrInfo.min_i_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmin_i_qp = <%d>;\r\n", FastbootVBR.min_i_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.max_i_qp = gNMRVdoEncObj[i].VbrInfo.max_i_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmax_i_qp = <%d>;\r\n", FastbootVBR.max_i_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.init_p_qp = gNMRVdoEncObj[i].VbrInfo.init_p_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tinit_p_qp = <%d>;\r\n", FastbootVBR.init_p_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.min_p_qp = gNMRVdoEncObj[i].VbrInfo.min_p_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmin_p_qp = <%d>;\r\n", FastbootVBR.min_p_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.max_p_qp = gNMRVdoEncObj[i].VbrInfo.max_p_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmax_p_qp = <%d>;\r\n", FastbootVBR.max_p_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.byte_rate = gNMRVdoEncObj[i].VbrInfo.byte_rate;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\trc_byte_rate = <%d>;\r\n", FastbootVBR.byte_rate);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.static_time = gNMRVdoEncObj[i].VbrInfo.static_time;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tstatic_time = <%d>;\r\n", FastbootVBR.static_time);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.ip_weight = gNMRVdoEncObj[i].VbrInfo.ip_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tip_weight = <0x%x>;\r\n", FastbootVBR.ip_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.change_pos = gNMRVdoEncObj[i].VbrInfo.change_pos;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tchange_pos = <%d>;\r\n", FastbootVBR.change_pos);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.key_p_period = gNMRVdoEncObj[i].VbrInfo.key_p_period;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tkey_p_period = <%d>;\r\n", FastbootVBR.key_p_period);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.kp_weight = gNMRVdoEncObj[i].VbrInfo.kp_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tkp_weight = <0x%x>;\r\n", FastbootVBR.kp_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.p2_weight = gNMRVdoEncObj[i].VbrInfo.p2_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tp2_weight = <0x%x>;\r\n", FastbootVBR.p2_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.p3_weight = gNMRVdoEncObj[i].VbrInfo.p3_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tp3_weight = <0x%x>;\r\n", FastbootVBR.p3_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.lt_weight = gNMRVdoEncObj[i].VbrInfo.lt_weight;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tlt_weight = <0x%x>;\r\n", FastbootVBR.lt_weight);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.motion_aq_str = gNMRVdoEncObj[i].VbrInfo.motion_aq_str;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmotion_aq_str = <0x%x>;\r\n", FastbootVBR.motion_aq_str);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootVBR.svc_weight_mode = gNMRVdoEncObj[i].VbrInfo.svc_weight_mode;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tsvc_weight_mode = <0x%x>;\r\n", FastbootVBR.svc_weight_mode);
					vos_file_write(fd, (void *)tmp_buf, len);
				} else if (gNMRVdoEncObj[i].FixQpInfo.enable) {
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\trc = <4>;\r\n");
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootFIXQP.fix_i_qp = gNMRVdoEncObj[i].FixQpInfo.fix_i_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tinit_i_qp = <%d>;\r\n", FastbootFIXQP.fix_i_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootFIXQP.fix_i_qp = gNMRVdoEncObj[i].FixQpInfo.fix_i_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmin_i_qp = <%d>;\r\n", FastbootFIXQP.fix_i_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootFIXQP.fix_i_qp = gNMRVdoEncObj[i].FixQpInfo.fix_i_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmax_i_qp = <%d>;\r\n", FastbootFIXQP.fix_i_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootFIXQP.fix_p_qp = gNMRVdoEncObj[i].FixQpInfo.fix_p_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tinit_p_qp = <%d>;\r\n", FastbootFIXQP.fix_p_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootFIXQP.fix_p_qp = gNMRVdoEncObj[i].FixQpInfo.fix_p_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmin_p_qp = <%d>;\r\n", FastbootFIXQP.fix_p_qp);
					vos_file_write(fd, (void *)tmp_buf, len);

					FastbootFIXQP.fix_p_qp = gNMRVdoEncObj[i].FixQpInfo.fix_p_qp;
					len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tmax_p_qp = <%d>;\r\n", FastbootFIXQP.fix_p_qp);
					vos_file_write(fd, (void *)tmp_buf, len);
				}

				uiFastbootParam = gNMRVdoEncObj[i].InitInfo.e_svc;
				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tsvc_layer = <%d>;\r\n", uiFastbootParam);
				vos_file_write(fd, (void *)tmp_buf, len);

				uiFastbootParam = gNMRVdoEncObj[i].InitInfo.ltr_interval;
				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tltr_interval = <%d>;\r\n", uiFastbootParam);
				vos_file_write(fd, (void *)tmp_buf, len);

				uiFastbootParam = gNMRVdoEncObj[i].InitInfo.bD2dEn;
				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\td2d = <%d>;\r\n", uiFastbootParam);
				vos_file_write(fd, (void *)tmp_buf, len);

				uiFastbootParam = gNMRVdoEncObj[i].uiVidFastbootGdc;
				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tgdc = <%d>;\r\n", uiFastbootParam);
				vos_file_write(fd, (void *)tmp_buf, len);

				uiFastbootParam = gNMRVdoEncObj[i].InitInfo.colmv_en;
				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tcolmv = <%d>;\r\n", uiFastbootParam);
				vos_file_write(fd, (void *)tmp_buf, len);

				uiFastbootParam = gNMRVdoEncObj[i].uiVidFastbootQualityLv;
				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tqualitylv = <%d>;\r\n", uiFastbootParam);
				vos_file_write(fd, (void *)tmp_buf, len);

				uiFastbootParam = gNMRVdoEncObj[i].uiVidFastbootISPId;
				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tisp_id = <%d>;\r\n", uiFastbootParam);
				vos_file_write(fd, (void *)tmp_buf, len);

				uiFastbootParam = gNMRVdoEncObj[i].InitInfo.rotate;
				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\th26x_rotate = <%d>;\r\n", uiFastbootParam);
				vos_file_write(fd, (void *)tmp_buf, len);

				uiFastbootParam = gNMRVdoEncObj[i].MemInfo.uiSnapshotSize;
				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tsrcout_size = <%d>;\r\n", uiFastbootParam);
				vos_file_write(fd, (void *)tmp_buf, len);

				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t};\r\n");
				vos_file_write(fd, (void *)tmp_buf, len);

				len = snprintf(tmp_buf, sizeof(tmp_buf), "\r\n");
				vos_file_write(fd, (void *)tmp_buf, len);
			} else {
				uiFastbootParam = gNMRVdoTrigObj[i].EncParam.quality;
				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tjpeg_quality = <%d>;\r\n", uiFastbootParam);
				vos_file_write(fd, (void *)tmp_buf, len);

				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t\tjpeg_fps = <%d>;\r\n", 5);
				vos_file_write(fd, (void *)tmp_buf, len);

				len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\t};\r\n");
				vos_file_write(fd, (void *)tmp_buf, len);

				len = snprintf(tmp_buf, sizeof(tmp_buf), "\r\n");
				vos_file_write(fd, (void *)tmp_buf, len);
			}
		}
	}
	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t};\r\n};");
	vos_file_write(fd, (void *)tmp_buf, len);

	vos_file_close(fd);

	return TRUE;
}
#endif

SXCMD_BEGIN(vdoenc,                 "NMI vdo enc module")
SXCMD_ITEM("dumpmem %s",            Cmd_VdoEnc_DumpMem,              "dump mem (Param: StartAddr Size => dumpmem 0xf0a10000 0x260)")
SXCMD_ITEM("encinfo %s",            Cmd_VdoEnc_EncInfo,              "enc info (Param: PathId => encinfo 0)")
SXCMD_ITEM("fpsinfo %s",            Cmd_VdoEnc_FpsInfo,              "fps info (Param: PathId Enable=> fpsinfo 0 1)")
SXCMD_ITEM("rcinfo %s",             Cmd_VdoEnc_RcInfo,               "rc info (Param: PathId Enable => rcinfo 0 1)")
SXCMD_ITEM("reenctest %s",          Cmd_VdoEnc_ReEncTest,            "re-encode test (Param: PathId MaxMs => reenctest 0 100)")
SXCMD_ITEM("reseti %s",				Cmd_VdoEnc_ResetIFrmame,		 "reset i frame (Param: PathId => reseti 0)")
SXCMD_ITEM("setaq %s",              Cmd_VdoEnc_SetAQ,                "set aq (Param: PathId Enable I_Str P_Str MaxAq MinAq) => setaq 0 1 3 3 8 -6")
SXCMD_ITEM("setgdr %s",             Cmd_VdoEnc_SetGDR,               "set gdr (Param: PathId Enable Period RowNumber) => setgdr 0 1 30 20")
SXCMD_ITEM("setslicesplit %s",      Cmd_VdoEnc_SetSliceSplit,        "set slicesplit (Param: PathId Enable SliceRowNumber) => setslicesplit 0 1 20")
SXCMD_ITEM("setcbr %s",             Cmd_VdoEnc_SetCBR,               "set cbr (Param: PathId Enable StaticTime ByteRate FrameRate GOP InitIQp MinIQp MaxIQp InitPQp MinPQp MaxPQp IPWeight => setcbr 0 1 4 524288 30 15 26 10 40 26 10 40 0)")
SXCMD_ITEM("setevbr %s",            Cmd_VdoEnc_SetEVBR,              "set evbr (Param: PathId Enable StaticTime ByteRate FrameRate GOP KeyPPeriod InitIQp MinIQp MaxIQp InitPQp MinPQp MaxPQp IPWeight KeyPWeight MotionAQStrength StillFrameCnd MotionRatioThd IPsnrCnd PPsnrCnd KeyPPsnrCnd=> setevbr 0 1 4 524288 25 50 0 26 15 45 26 15 45 0 0 -6 150 30 0 0 0)")
SXCMD_ITEM("setfixqp %s", 			Cmd_VdoEnc_SetFixQP,			 "set fix QP (Param: PathId Enable IFixQP PFixQP => setfixqp 0 1 26 26)")
SXCMD_ITEM("setroi %s",				Cmd_VdoEnc_SetROI,			 	 "set roi (Param: PathId RoiIndex Enable QP QPMode Coord_X Coord_Y Width Height)")
SXCMD_ITEM("setsmartroi %s",		Cmd_VdoEnc_SetSmartROI,			 "set smart roi (Param: PathId Enable) => setsmartroi 0 1")
SXCMD_ITEM("setvbr %s",             Cmd_VdoEnc_SetVBR,               "set vbr (Param: PathId Enable StaticTime ByteRate FrameRate GOP InitIQp MinIQp MaxIQp InitPQp MinPQp MaxPQp IPWeight ChangePos => setvbr 0 1 4 524288 25 50 26 15 45 26 15 45 0 0)")
SXCMD_ITEM("showmsg %s",		    Cmd_VdoEnc_ShowMsg, 		     "show msg (Param: PathId Level(0:Disable, 1:Enc time, 2:Input info, 4:Output info, 8:Process time, 16:Md info, 32:SmartRoi, 64:Ring Buf, 128:Drop Frame, 256:YUV_TMOUT) => showmsg 0 1)")
SXCMD_ITEM("skipframe %s",		    Cmd_VdoEnc_SkipFrame, 		     "skip frame (Param: PathId SkipFrameCount => skipframe 0 200)")
SXCMD_ITEM("resetencoder %s",       Cmd_VdoEnc_ResetEncoder,         "reset encoder (Param: PathId => resetencoder 0)")
#ifdef __KERNEL__
SXCMD_ITEM("dump_fastboot  %",    Cmd_VdoEnc_Fastboot_EncInfo,     "fastboot enc info (Param: PathId => fastbootencinfo 0)")
#endif
SXCMD_END()


void NMR_VdoEnc_AddUnit(void)
{
	NMI_AddUnit(&NMI_VdoEnc);
	sxcmd_addtable(vdoenc);
}

int _isf_vdoenc_cmd_vdoenc_showhelp(void)
{
	UINT32 cmd_num = SXCMD_NUM(vdoenc);
	UINT32 loop=1;

	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "vdoenc");
	DBG_DUMP("=====================================================================\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		DBG_DUMP("%15s : %s\r\n", vdoenc[loop].p_name, vdoenc[loop].p_desc);
	}

	return 0;
}

int _isf_vdoenc_cmd_vdoenc(char* sub_cmd_name, char *cmd_args)
{
	UINT32 cmd_num = SXCMD_NUM(vdoenc);
	UINT32 loop=1;
	BOOL ret=FALSE;

	if (strncmp(sub_cmd_name, "?", 2) == 0) {
		_isf_vdoenc_cmd_vdoenc_showhelp();
		return 0;
	}

	if (FALSE == _nmr_vdoenc_is_init()) {
		DBG_ERR(" NOT init yet, could not use debug command !!\r\n");
		return -1;
	}

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(sub_cmd_name, vdoenc[loop].p_name, strlen(sub_cmd_name)) == 0) {
			ret = vdoenc[loop].p_func(cmd_args);
			return ret;
		}
	}

	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n  Usage :\r\n");
		_isf_vdoenc_cmd_vdoenc_showhelp();
		return -1;
	}


	return 0;
}

#ifndef __KERNEL__
MAINFUNC_ENTRY(vdoenc, argc, argv)
{
	char sub_cmd_name[32] = {0};
	char cmd_args[256]    = {0};
	UINT8 i = 0;

	if (argc == 1) {
		_isf_vdoenc_cmd_vdoenc_showhelp();
		return 0; // only "vdoenc"
	}

	// setup sub_cmd ( showmsg, encinfo, fpsinfo, ... )
	if (argc >= 2) {
		strncpy(sub_cmd_name, argv[1], sizeof(sub_cmd_name)-1);
		sub_cmd_name[sizeof(sub_cmd_name)-1] = '\0';
	}

	// setup command params ( 0 2 1920 1080 ... )
	if (argc >=3) {
		for (i = 2 ; i < argc ; i++) {
			strncat(cmd_args, argv[i], 20);
			strncat(cmd_args, " ", 20);
		}
	}

	//DBG_ERR("subcmd = %s, args = %s\r\n", sub_cmd_name, cmd_args);

	_isf_vdoenc_cmd_vdoenc(sub_cmd_name, cmd_args);

	return 0;
}
#endif


