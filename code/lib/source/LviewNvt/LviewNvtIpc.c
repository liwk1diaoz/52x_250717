#include <stdio.h>
#include "Type.h"
#include "timer.h"
#include "LviewNvtID.h"
#include "LviewNvtInt.h"
#include "LviewNvtIpc.h"
#include "LviewNvtAPI.h"
#include "LviewNvtTsk.h"
#include "NvtIpcAPI.h"
#include "lviewd.h"
#include "dma_protected.h"
#include "CoreInfo.h"

#if LVIEW_USE_IPC



#define IPC_MSGGET()                 NvtIPC_MsgGet(NvtIPC_Ftok(LVIEWD_IPC_TOKEN_PATH))
#define IPC_MSGREL(msqid)            NvtIPC_MsgRel(msqid)
#define IPC_MSGSND(msqid,pMsg,msgsz) NvtIPC_MsgSnd(msqid,NVTIPC_SENDTO_CORE2,pMsg,msgsz)
#define IPC_MSGRCV(msqid,pMsg,msgsz) NvtIPC_MsgRcv(msqid,pMsg,msgsz)


///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          LviewNvtIpc
#define __DBGLVL__          2
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"


static ER xLviewNvt_SendIPC(LVIEWD_IPC_CMD ipc)
{
	LVIEWD_IPC_MSG msg = {0};
	msg.mtype = LVIEWD_IPC_MSG_TYPE_C2S;
	msg.uiIPC = ipc;
	LVIEWD_CTRL *pCtrl = xLviewNvt_GetCtrl();

	if (IPC_MSGSND(pCtrl->msqid, &msg, sizeof(msg)) < 0) {
		DBG_ERR("msgsnd");
		return E_SYS;
	}
	return E_OK;
}

void LviewNvt_Ipc_Open(LVIEWNVT_DAEMON_INFO   *pOpen)
{
    FLGPTN          uiFlag;
    LVIEWD_CTRL*    pCtrl = xLviewNvt_GetCtrl();
	CHAR            startDaemonCmd[128];
	UINT32          size;

    pCtrl->msqid = IPC_MSGGET();
    if (pCtrl->msqid < 0)
    {
        DBG_ERR("get msq fail\r\n",pCtrl->msqid);
    }

	// set up server start up command
	size = sizeof(startDaemonCmd);
	snprintf(startDaemonCmd, size, "lviewd -p %d -s %d -f %d -t %d -m %x %x -tos %d -to %d -ssl %d -pm %d -v 0x%x &",
			 pOpen->portNum,
			 pOpen->sockbufSize,
			 pOpen->frameRate,
			 pOpen->threadPriority,
			 dma_getPhyAddr(pOpen->bitstream_mem_range.Addr),
			 pOpen->bitstream_mem_range.Size,
			 pOpen->tos,
			 pOpen->timeoutCnt,
			 pOpen->is_ssl,
			 pOpen->is_push_mode,
			 LVIEWD_INTERFACE_VER);
	startDaemonCmd[size - 1] = 0;

	sta_tsk(LVIEWD_RCV_TSK_ID, 0);
	// start cpu2 lviewd app
    {
        INT32            ret;
        NVTIPC_SYS_MSG   sysMsg;

        sysMsg.sysCmdID = NVTIPC_SYSCMD_SYSCALL_REQ;
        sysMsg.DataAddr = (UINT32)&startDaemonCmd[0];
        sysMsg.DataSize = strlen(startDaemonCmd)+1;
        if ((ret = IPC_MSGSND(NVTIPC_SYS_QUEUE_ID, &sysMsg, sizeof(sysMsg))) < 0)
        {
            DBG_ERR("NvtIPC_MsgSnd %d\r\n",ret);
            return;
        }
    }
    // wait hfs started
    DBG_IND("wait hfs start\r\n");
    wai_flg(&uiFlag, pCtrl->FlagID, FLG_LVIEWD_STARTED, TWF_ORW);
	DBG_IND("hfs started\r\n");
}

void LviewNvt_Ipc_Close(void)
{
    FLGPTN          uiFlag;
    LVIEWD_CTRL*    pCtrl = xLviewNvt_GetCtrl();

    xLviewNvt_SendIPC(LVIEWD_IPC_CLOSE_SERVER);
	// wait server exit.
    wai_flg(&uiFlag, pCtrl->FlagID, FLG_LVIEWD_STOPPED, TWF_ORW|TWF_CLR);
    ter_tsk(LVIEWD_RCV_TSK_ID);
    if (IPC_MSGREL(pCtrl->msqid) != NVTIPC_ER_OK)
		DBG_ERR("NvtIPC_MsgRel\r\n");

}


void LviewNvt_Ipc_PushFrame(LVIEWNVT_FRAME_INFO *frame_info)
{
	LVIEWD_IPC_PUSH_FRAME_MSG Msg;
	LVIEWD_CTRL*    pCtrl = xLviewNvt_GetCtrl();

	Msg.mtype = LVIEWD_IPC_MSG_TYPE_C2S;
	Msg.uiIPC = LVIEWD_IPC_PUSH_FRAME;
	Msg.addr = dma_getPhyAddr(frame_info->addr);
	Msg.size = frame_info->size;
	DBG_IND("addr=0x%x,size=0x%x \r\n", Msg.addr,Msg.size);

	if (IPC_MSGSND(pCtrl->msqid, &Msg, sizeof(Msg)) < 0) {
		DBG_ERR("msgsnd\r\n");
	}
}

void LviewNvt_Ipc_RcvTsk(void)
{
	LVIEWD_IPC_MSG   msg;
	LVIEWD_CTRL *pCtrl;
	LVIEWNVT_DAEMON_INFO *pOpen;
	BOOL bContinue = 1;
	UINT32       frameCount = 0;
	INT32        ret;

	kent_tsk();
	DBG_FUNC_BEGIN("\r\n");
	pCtrl = xLviewNvt_GetCtrl();
	pOpen = &pCtrl->Open;

	while (bContinue) {
		PROFILE_TASK_IDLE();
		if ((ret = IPC_MSGRCV(pCtrl->msqid, &msg, sizeof(msg))) < 0) {
			DBG_ERR("msgrcv %d\r\n", ret);
			bContinue = 0;
			continue;
		}
		PROFILE_TASK_BUSY();
		DBG_IND("ipc_cmd=%d\r\n", msg.uiIPC);
		switch (msg.uiIPC) {
		case LVIEWD_IPC_SERVER_STARTED:
			set_flg(pCtrl->FlagID, FLG_LVIEWD_STARTED);
			break;
		case LVIEWD_IPC_NOTIFY_CLIENT:
			if (pOpen->serverEvent) {
				pOpen->serverEvent(msg.parm1);
			}
			break;
		case LVIEWD_IPC_GET_JPG: {
				UINT32 jpgAddr = 0, jpgSize = 0, ret = FALSE;
				LVIEWD_IPC_GETJPG_MSG Msg;

				if (pOpen->getJpg) {
					ret = pOpen->getJpg(&jpgAddr, &jpgSize);
				}
				Msg.mtype = LVIEWD_IPC_MSG_TYPE_C2S;
				Msg.uiIPC = LVIEWD_IPC_GET_JPG_ACK;
				if (ret && dma_isCacheAddr(jpgAddr)) {
					DBG_ERR("jpgAddr 0x%x should be non-cache Address\r\n", jpgAddr);
					jpgAddr = 0;
					jpgSize = 0;
				}
				Msg.jpgAddr = dma_getPhyAddr(jpgAddr);
				Msg.jpgSize = jpgSize;
				if (IPC_MSGSND(pCtrl->msqid, &Msg, LVIEWD_IPC_MSGSZ) < 0) {
					DBG_ERR("msgsnd\r\n");
					break;
				}
				frameCount++;
#if LVIEW_DEBUG_LATENCY
				DBG_DUMP("frameCount=%06d    ,\ttime=%d ms\r\n", frameCount, timer_getCurrentCount(timer_getSysTimerID()) / 1000);
#endif
				if ((frameCount % 1800) == 0) {
					DBG_DUMP("frameCount=%d \r\n", frameCount);
				}
			}
			break;
		case LVIEWD_IPC_CLOSE_SERVER_ACK:
			bContinue = 0;
			LVIEWD_IPC_MSG  Msg;
			Msg.mtype = LVIEWD_IPC_MSG_TYPE_C2S;
			Msg.uiIPC = LVIEWD_IPC_CLOSE_FINISH;
			if (IPC_MSGSND(pCtrl->msqid, &Msg, sizeof(LVIEWD_IPC_MSG)) < 0) {
				DBG_ERR("msgsnd\r\n");
			}
			break;
		default:
			DBG_ERR("ipc_cmd=%d\r\n", msg.uiIPC);
			break;
		}
	}
	set_flg(pCtrl->FlagID, FLG_LVIEWD_STOPPED);
	DBG_FUNC_END("\r\n");
	ext_tsk();
}
#endif



