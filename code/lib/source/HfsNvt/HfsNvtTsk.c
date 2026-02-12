#include <stdio.h>
#include <string.h>

#include "HfsNvtInt.h"
#include "HfsNvtID.h"
#include "HfsNvtTsk.h"
#include "HfsNvtIpc.h"
#include "HfsNvt/hfs.h"
//#include "dma_protected.h"


///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          HfsNvtTsk
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#include "kwrap/debug.h"

#if !HFS_USE_IPC && !defined _NETWORK_OFF_
static void ecos_hfs_open(HFSNVT_OPEN *pOpen);
static void ecos_hfs_close(void);
#endif

static HFS_OPS g_hfsOpObj =
{
	#ifdef _NETWORK_OFF_
		NULL,
		NULL,
	#else
	    #if HFS_USE_IPC
	    HfsNvt_Ipc_Open,
	    HfsNvt_Ipc_Close,
	    #else
	    ecos_hfs_open,
	    ecos_hfs_close,
	    #endif
	#endif
};

static HFS_OPS *pHfsOp = &g_hfsOpObj;

#define CFG_HFSNVT_INIT_KEY   MAKEFOURCC('H','F','S','I')

#define HFSNVT_NEED_BUFSIZE   70*1024 // 65k for send , 5k for msg buffer

HFSNVT_CTRL m_HfsNvt_Ctrl = {0};


HFSNVT_CTRL *xHfsNvt_GetCtrl(void)
{
	return &m_HfsNvt_Ctrl;
}

static HFSNVT_WR xHfsNvt_Wrn(HFSNVT_WR wr)
{
	if (wr == HFSNVT_WR_OK) {
		return wr;
	}
	DBG_WRN("%d\r\n", wr);
	return wr;
}

#if !HFS_USE_IPC && !defined _NETWORK_OFF_
static void ecos_hfs_open(HFSNVT_OPEN *pOpen)
{
	cyg_hfs_open_obj  hfsObj = {0};

	DBG_IND("\r\n");
	// register callback function
	hfsObj.notify        = (cyg_hfs_notify *)pOpen->serverEvent;
	hfsObj.getCustomData = (cyg_hfs_getCustomData *)pOpen->getCustomData;
	hfsObj.putCustomData = (cyg_hfs_putCustomData *)pOpen->putCustomData;
	hfsObj.check_pwd     = (cyg_hfs_check_password *)pOpen->check_pwd;
	hfsObj.uploadResultCb = (cyg_hfs_upload_result_cb *)pOpen->uploadResultCb;
	hfsObj.deleteResultCb = (cyg_hfs_delete_result_cb *)pOpen->deleteResultCb;
	hfsObj.headerCb       = (cyg_hfs_header_cb *)pOpen->headerCb;
	hfsObj.downloadResultCb = (cyg_hfs_download_result_cb *)pOpen->downloadResultCb;
	// set port number
	hfsObj.portNum        = pOpen->portNum;
	// set thread priority
	hfsObj.threadPriority = pOpen->threadPriority;
	hfsObj.sockbufSize    = pOpen->sockbufSize;
	hfsObj.tos            = pOpen->tos;
	hfsObj.maxClientNum   = pOpen->maxClientNum;
	hfsObj.timeoutCnt     = pOpen->timeoutCnt;
	hfsObj.httpsPortNum   = pOpen->httpsPortNum;
	// set HFS root dir path
	strncpy(hfsObj.rootdir, pOpen->rootdir, CYG_HFS_ROOT_DIR_MAXLEN);
	hfsObj.rootdir[CYG_HFS_ROOT_DIR_MAXLEN] = 0;
	strncpy(hfsObj.key, pOpen->key, CYG_HFS_KEY_MAXLEN);
	hfsObj.rootdir[CYG_HFS_KEY_MAXLEN] = 0;
	strncpy(hfsObj.customStr, pOpen->customStr, HFS_CUSTOM_STR_MAXLEN);
	hfsObj.rootdir[HFS_CUSTOM_STR_MAXLEN] = 0;
	// start HFS
	cyg_hfs_open(&hfsObj);
}

static void ecos_hfs_close(void)
{
    cyg_hfs_close();
}
#endif


ER HfsNvt_Open(HFSNVT_OPEN *pOpen)
{
	HFSNVT_CTRL *pCtrl;

	DBG_FUNC_BEGIN("\r\n");
	if (!pOpen) {
		DBG_ERR("pOpen is NULL\r\n");
		return E_PAR;
	}
	#if 0
	if (!pOpen->sharedMemAddr) {
		DBG_ERR("sharedMemAddr is NULL\r\n");
		return E_NOMEM;
	}
	if (dma_isCacheAddr(pOpen->sharedMemAddr)) {
		DBG_ERR("sharedMemAddr 0x%x should be non-cache Address\r\n", (int)pOpen->sharedMemAddr);
		return E_PAR;
	}
	if (pOpen->sharedMemSize < HFSNVT_NEED_BUFSIZE) {
		DBG_ERR("sharedMemSize 0x%x < need 0x%x\r\n", pOpen->sharedMemSize, HFSNVT_NEED_BUFSIZE);
		return E_NOMEM;
	}
	#endif
	pCtrl = xHfsNvt_GetCtrl();
	if (pCtrl->uiInitKey == CFG_HFSNVT_INIT_KEY) {
		xHfsNvt_Wrn(HFSNVT_WR_ALREADY_OPEN);
		return E_OK;
	}
	HfsNvt_InstallID();
	SEM_WAIT(HFSNVT_SEM_ID);
	memset(pCtrl, 0, sizeof(HFSNVT_CTRL));
	pOpen->rootdir[HFS_ROOT_DIR_MAXLEN] = 0;
	pOpen->key[HFS_KEY_MAXLEN] = 0;
	pOpen->clientQueryStr[HFS_USER_QUERY_MAXLEN] = 0;
	pCtrl->Open = *pOpen;
	pCtrl->FlagID = HFSNVT_FLG_ID;
	pCtrl->uiInitKey = CFG_HFSNVT_INIT_KEY;

	if (pHfsOp->Open)
		pHfsOp->Open(pOpen);
	SEM_SIGNAL(HFSNVT_SEM_ID);
	DBG_FUNC_END("\r\n");
	return E_OK;
}
ER HfsNvt_Close(void)
{
	HFSNVT_CTRL *pCtrl = xHfsNvt_GetCtrl();

	DBG_FUNC_BEGIN("\r\n");
	SEM_WAIT(HFSNVT_SEM_ID);
	if (pCtrl->uiInitKey != CFG_HFSNVT_INIT_KEY) {
		xHfsNvt_Wrn(HFSNVT_WR_NOT_OPEN);
		SEM_SIGNAL(HFSNVT_SEM_ID);
		return E_OK;
	}
	if (pHfsOp->Close)
		pHfsOp->Close();
	pCtrl->uiInitKey = 0;
	SEM_SIGNAL(HFSNVT_SEM_ID);
	HfsNvt_UnInstallID();
	DBG_FUNC_END("\r\n");
	return E_OK;
}


