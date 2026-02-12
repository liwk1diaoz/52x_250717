#include <stdio.h>
#include "LviewNvtID.h"
#include "LviewNvtInt.h"
#include "LviewNvtNonIpc.h"
#include "LviewNvt/lviewd.h"


///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          LviewNvtNonIpc
#define __DBGLVL__          2
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"


#if LVIEW_USE_IPC == DISABLE
void LviewNvt_NonIpc_Open(LVIEWNVT_DAEMON_INFO   *pOpen)
{
	cyg_lviewd_install_obj  lviewObj = {0};

	// register getJPG function for getting liveview JPG.
	lviewObj.getJpg = (cyg_lviewd_getJpg *)pOpen->getJpg;
	// noify some status of HTTP server
	lviewObj.notify = (cyg_lviewd_notify *)pOpen->serverEvent;
	// register HW memcpy function for speed up memory copy
	lviewObj.hwmem_memcpy = NULL;
	// set port number
	lviewObj.portNum = pOpen->portNum;
	// set http live view server thread priority
	lviewObj.threadPriority = pOpen->threadPriority;
	// set the maximum JPG size that http live view server to support
	//lviewObj.maxJpgSize = maxJpgSize;
	// live view streaming frame rate
	lviewObj.frameRate = pOpen->frameRate;
	// socket buffer size
	lviewObj.is_ssl    = pOpen->is_ssl;

	lviewObj.sockbufSize = pOpen->sockbufSize;

	lviewObj.timeoutCnt = pOpen->timeoutCnt;
	// type of service
	lviewObj.tos = pOpen->tos;
    // push mode
	lviewObj.is_push_mode = pOpen->is_push_mode;
	// reserved parameter , no use now, just fill NULL.
	lviewObj.arg = NULL;
	// install the CB functions and set some parameters
	cyg_lviewd_install(&lviewObj);
	// start the http live view server
	cyg_lviewd_startup();
}


void LviewNvt_NonIpc_Close(void)
{
	return cyg_lviewd_stop();
}

void LviewNvt_NonIpc_PushFrame(LVIEWNVT_FRAME_INFO *frame_info)
{
	cyg_lviewd_frame_info  cyg_frame_info;

	cyg_frame_info.addr = frame_info->addr;
	cyg_frame_info.size = frame_info->size;
	return cyg_lviewd_push_frame(&cyg_frame_info);
}

#endif

