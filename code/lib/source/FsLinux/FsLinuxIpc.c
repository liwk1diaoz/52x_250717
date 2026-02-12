#include "FsLinuxBuf.h"
#include "FsLinuxDebug.h"
#include "FsLinuxID.h"
#include "FsLinuxIpc.h"

#if FSLINUX_USE_IPC
#define FSLINUX_INVALID_MSGID NVTIPC_MSG_QUEUE_NUM+1
#define FSLINUX_SEND_TARGET NVTIPC_SENDTO_CORE2

//global variables
static NVTIPC_U32 gIPC_MsgId  = FSLINUX_INVALID_MSGID;

ER FsLinux_Ipc_Init()
{
	NVTIPC_I32 Ret_NvtIpc;
	DBG_FUNC_BEGIN("[IPC]\r\n");

	//get ipc msg id
	Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(FSLINUX_TOKEN_PATH));

	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgGet = %d\r\n", Ret_NvtIpc);
		return E_SYS;
	}
	gIPC_MsgId = (NVTIPC_U32)Ret_NvtIpc;
	DBG_IND("[IPC]Got MsgId(%d)\r\n", gIPC_MsgId);

	DBG_FUNC_END("[IPC]\r\n");
	return E_OK;
}

ER FsLinux_Ipc_Uninit()
{
	NVTIPC_I32 Ret_NvtIpc;
	DBG_FUNC_BEGIN("[IPC]\r\n");

	Ret_NvtIpc = NvtIPC_MsgRel(gIPC_MsgId);
	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgRel = %d\r\n", Ret_NvtIpc);
		return E_SYS;
	}
	DBG_IND("[IPC]MsgId(%d) released\r\n", gIPC_MsgId);

	gIPC_MsgId = FSLINUX_INVALID_MSGID;

	DBG_FUNC_END("[IPC]\r\n");
	return E_OK;
}

ER FsLinux_Ipc_WaitMsg(FSLINUX_IPCMSG *pRcvMsg)
{
	NVTIPC_I32  Ret_NvtIpc;
	DBG_FUNC_BEGIN("[IPC]\r\n");

	Ret_NvtIpc = NvtIPC_MsgRcv(gIPC_MsgId, pRcvMsg, sizeof(FSLINUX_IPCMSG));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgRcv = %d\r\n", Ret_NvtIpc);
		return E_SYS;
	}

	DBG_FUNC_END("[IPC]\r\n");
	return E_OK;
}

ER FsLinux_Ipc_PostMsg(FSLINUX_IPCMSG *pPostMsg)
{
	NVTIPC_I32  Ret_NvtIpc;
	DBG_FUNC_BEGIN("[IPC]\r\n");

	Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId, FSLINUX_SEND_TARGET, pPostMsg, sizeof(FSLINUX_IPCMSG));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgSnd = %d\r\n", Ret_NvtIpc);
		return E_SYS;
	}

	DBG_FUNC_END("[IPC]\r\n");
	return E_OK;
}
#endif
ER FsLinux_Ipc_SendMsgWaitAck(const FSLINUX_IPCMSG *pSendMsg)
{
#if FSLINUX_USE_IPC
	NVTIPC_I32  Ret_NvtIpc;
	FSLINUX_DRIVE_INFO *pDrvInfo;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FLGPTN FlgPtn;

	DBG_FUNC_BEGIN("[IPC]\r\n");

	pDrvInfo = FsLinux_Buf_GetDrvInfo(pSendMsg->Drive);
	if (NULL == pDrvInfo) {
		DBG_ERR("GetDrvInfo failed\r\n");
		return E_SYS;
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(pSendMsg->Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		return E_SYS;
	}

	clr_flg(pCtrlObj->FlagID, FSLINUX_FLGBIT_IPC_RECEIVED);

	Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId, FSLINUX_SEND_TARGET, (void *)pSendMsg, sizeof(FSLINUX_IPCMSG));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgSnd = %d\r\n", Ret_NvtIpc);
		return E_SYS;
	}

	wai_flg(&FlgPtn, pCtrlObj->FlagID, FSLINUX_FLGBIT_IPC_RECEIVED, TWF_ORW);

	if (pSendMsg->CmdId != pDrvInfo->RcvIpc.CmdId) {
		DBG_ERR("Cmd ack not match. Send(%d), Ack(%d)\r\n", pSendMsg->CmdId, pDrvInfo->RcvIpc.CmdId);
		return E_SYS;
	}

	DBG_FUNC_END("[IPC]\r\n");
#else
	FSLINUX_DRIVE_INFO *pDrvInfo;
	INT32 Ret;

	DBG_FUNC_BEGIN("[CMD]\r\n");

	pDrvInfo = FsLinux_Buf_GetDrvInfo(pSendMsg->Drive);
	if (NULL == pDrvInfo) {
		DBG_ERR("GetDrvInfo failed\r\n");
		return E_SYS;
	}

	Ret = fslinux_cmd_DoCmd(pSendMsg);
	if (Ret < 0) {
		DBG_ERR("DoCmd = %d\r\n", Ret);
		return E_SYS;
	}

	DBG_FUNC_END("[CMD]\r\n");
#endif
	return E_OK;
}

ER FsLinux_Ipc_ReqInfoWaitAck(const FSLINUX_IPCMSG *pSendMsg)
{
#if FSLINUX_USE_IPC
	NVTIPC_I32  Ret_NvtIpc;
	FSLINUX_CTRL_OBJ *pCtrlObj;
	FLGPTN FlgPtn;

	DBG_FUNC_BEGIN("[IPC]\r\n");

	if (pSendMsg->CmdId != FSLINUX_CMDID_GETDISKINFO) {
		DBG_ERR("Invalid CmdId %d\r\n", pSendMsg->CmdId);
		return E_PAR;
	}

	pCtrlObj = FsLinux_GetCtrlObjByDrive(pSendMsg->Drive);
	if (NULL == pCtrlObj) {
		DBG_ERR("Get CtrlObj failed\r\n");
		return E_SYS;
	}

	Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId, FSLINUX_SEND_TARGET, (void *)pSendMsg, sizeof(FSLINUX_IPCMSG));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgSnd = %d\r\n", Ret_NvtIpc);
		return E_SYS;
	}

	wai_flg(&FlgPtn, pCtrlObj->FlagID, FSLINUX_FLGBIT_IPC_INFO_RCVED, TWF_ORW | TWF_CLR);

	DBG_FUNC_END("[IPC]\r\n");
#else
	INT32 Ret;

	DBG_FUNC_BEGIN("[CMD]\r\n");

	if (pSendMsg->CmdId != FSLINUX_CMDID_GETDISKINFO) {
		DBG_ERR("Invalid CmdId %d\r\n", pSendMsg->CmdId);
		return E_PAR;
	}

	Ret = fslinux_cmd_DoCmd(pSendMsg);
	if (Ret < 0) {
		DBG_ERR("DoCmd = %d\r\n", Ret);
		return E_SYS;
	}

	DBG_FUNC_END("[CMD]\r\n");
#endif
	return E_OK;
}
#if FSLINUX_USE_IPC
ER FsLinux_Ipc_SysCallReq(CHAR *szCmd)
{
	NVTIPC_SYS_MSG SysMsg;
	NVTIPC_I32 Ret_NvtIpc;
	DBG_FUNC_BEGIN("[IPC]\r\n");

	DBG_IND("[IPC]szCmd = %s\r\n", szCmd);

	//send syscall request ipc to FileSysEcos to close
	SysMsg.sysCmdID = NVTIPC_SYSCMD_SYSCALL_REQ;
	SysMsg.DataAddr = (NVTIPC_U32)szCmd;
	SysMsg.DataSize = (NVTIPC_U32)strlen(szCmd) + 1;

	Ret_NvtIpc = NvtIPC_MsgSnd(NVTIPC_SYS_QUEUE_ID, FSLINUX_SEND_TARGET, &SysMsg, sizeof(SysMsg));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgSnd(SYSCALL_REQ) = %d\r\n", Ret_NvtIpc);
		return E_SYS;
	}

	DBG_FUNC_END("[IPC]\r\n");
	return E_OK;
}
#endif

