#include "FsLinuxBuf.h"
#include "FsLinuxDebug.h"
#include "FsLinuxID.h"
#include "FsLinuxIpc.h"
#include "FsLinuxLock.h"
#include "FsLinuxTsk.h"
#include "FsLinuxUtil.h"

FSLINUX_CMDID g_Async_CmdId;
FileSys_CB g_Async_CB;

void FsLinux_Tsk_WaitForAsyncCmd(FSLINUX_CMDID CmdId, FileSys_CB CB, CHAR Drive)
{
#if FSLINUX_USE_IPC
	FSLINUX_CTRL_OBJ *pCtrlObj;

	DBG_IND("Async CmdId = %d, CB = 0x%X, Drive = %c\r\n", CmdId, CB, Drive);

	pCtrlObj = FsLinux_GetCtrlObjByDrive(Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		return;
	}

	g_Async_CmdId = CmdId;
	g_Async_CB = CB;
	set_flg(pCtrlObj->FlagID, FSLINUX_COMMON_FLGBIT_TSK_START_WAIT);

	DBG_ERR("Async not support\r\n");
#endif
}

void FsLinux_Tsk_WaitTskReady(void)
{
#if FSLINUX_USE_IPC
	FLGPTN FlgPtn;
	wai_flg(&FlgPtn, FSLINUX_COMMON_FLG_ID, FSLINUX_COMMON_FLGBIT_TSK_READY, TWF_ORW);
#endif
}

#if FSLINUX_USE_IPC
void FsLinux_Tsk(void)
{
	FSLINUX_IPCMSG RcvMsg = {0};
	FSLINUX_DRIVE_INFO *pDrvInfo;
	FSLINUX_CTRL_OBJ *pCtrlObj;

	THREAD_ENTRY(); //should be the first function to be called in a task function
	DBG_FUNC_BEGIN("[TSK]\r\n");

	if (E_OK != FsLinux_Ipc_Init()) {
		DBG_ERR("FsLinux_Ipc_Init\r\n");
		goto L_FsLinux_Tsk_END;
	}

	if (E_OK != FsLinux_Ipc_SysCallReq(FSLINUX_TOKEN_PATH" "FSLINUX_INTERFACE_VER" &")) {
		DBG_ERR("FsLinux_Ipc_SysCallReq\r\n");
		goto L_FsLinux_Tsk_END;
	}

	if (E_OK != FsLinux_Ipc_WaitMsg(&RcvMsg)) {
		DBG_ERR("FsLinux_Ipc_WaitAck\r\n");
		goto L_FsLinux_Tsk_END;
	}

	if (RcvMsg.CmdId != FSLINUX_CMDID_ACKPID) {
		DBG_ERR("SysCallReq ack id not match\r\n");
		goto L_FsLinux_Tsk_END;
	}

	clr_flg(FSLINUX_COMMON_FLG_ID, FSLINUX_COMMON_FLGBIT_TSK_EXITED);
	set_flg(FSLINUX_COMMON_FLG_ID, FSLINUX_COMMON_FLGBIT_TSK_READY);

	while (1) {
		PROFILE_TASK_IDLE();
		if (E_OK != FsLinux_Ipc_WaitMsg(&RcvMsg)) {
			DBG_ERR("FsLinux_Ipc_WaitAck failed\r\n");
			break;//break the while loop
		}
		PROFILE_TASK_BUSY();

		DBG_IND("[TSK]RcvMsg.CmdId = %d\r\n", RcvMsg.CmdId);

		pDrvInfo = FsLinux_Buf_GetDrvInfo(RcvMsg.Drive);
		if (NULL == pDrvInfo) {
			DBG_ERR("GetDrvInfo failed\r\n");
			break;
		}

		pCtrlObj = FsLinux_GetCtrlObjByDrive(RcvMsg.Drive);
		if (NULL == pCtrlObj) {
			DBG_ERR("Get CtrlObj failed\r\n");
			break;
		}

		if (FSLINUX_CMDID_GETDISKINFO == RcvMsg.CmdId) {
			set_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_IPC_INFO_RCVED);
		} else {
			memcpy(&(pDrvInfo->RcvIpc), &RcvMsg, sizeof(FSLINUX_IPCMSG));
			set_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_IPC_RECEIVED);
		}

		if (FSLINUX_CMDID_IPC_DOWN == RcvMsg.CmdId) {
			DBG_IND("[TSK]Got IPC Done Cmd\r\n");
			break;
		}
	}

	if (E_OK != FsLinux_Ipc_Uninit()) {
		DBG_ERR("FsLinux_Ipc_Uninit\r\n");
		goto L_FsLinux_Tsk_END;
	}

L_FsLinux_Tsk_END:

	clr_flg(FSLINUX_COMMON_FLG_ID, FSLINUX_COMMON_FLGBIT_TSK_READY);
	set_flg(FSLINUX_COMMON_FLG_ID, FSLINUX_COMMON_FLGBIT_TSK_EXITED);

	DBG_FUNC_END("[TSK]\r\n");
	THREAD_RETURN(0);
}
#endif

