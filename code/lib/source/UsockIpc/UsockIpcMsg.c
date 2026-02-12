#include "UsockIpc/UsockIpcAPI.h"
#if (USE_IPC)
#include "NvtIpcAPI.h"
#include "UsockIpcInt.h"
#include "UsockIpcID.h"
#include "UsockIpcMsg.h"
#include "UsockIpcTsk.h"

#define USOCKIPC_INVALID_MSGID NVTIPC_MSG_QUEUE_NUM+1
#define USOCKIPC_SEND_TARGET NVTIPC_SENDTO_CORE2

//global variables
static NVTIPC_U32 gIPC_MsgId[MAX_USOCKET_NUM]  = {USOCKIPC_INVALID_MSGID,USOCKIPC_INVALID_MSGID};
static UINT32 gMsgAck[MAX_USOCKET_NUM] = {0};

#ifdef USOCKET_SIM
static NVTIPC_U32 gIPC_MsgId_uITRON = 1;
static NVTIPC_U32 gIPC_MsgId_eCos = 2;
#endif //#ifdef USOCKET_SIM

ER UsockIpc_Msg_Init(USOCKIPC_ID id)
{
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);
	NVTIPC_I32 Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

	//get ipc msg id
#ifdef USOCKET_SIM
	DBG_ERR("============================================\r\n");
	DBG_ERR("============= UsockIpc simulation =============\r\n");
	DBG_ERR("============================================\r\n");
	Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok("USOCKIPC uITRON"));
#else
	Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(p_ctrl->token_path));
#endif

	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgGet = %d\r\n",id, Ret_NvtIpc);
		return E_SYS;
	}
	gIPC_MsgId[id] = (NVTIPC_U32)Ret_NvtIpc;
	DBG_IND("[MSG]%d Got MsgId(%d)\r\n",id, gIPC_MsgId[id]);

#ifdef USOCKET_SIM
	if (gIPC_MsgId[id] != gIPC_MsgId_uITRON) {
		DBG_ERR("%d Simulation environment error\n",id);
		return E_SYS;
	}
#endif

	DBG_FUNC_END("[MSG]\r\n");
	return E_OK;
}

ER UsockIpc_Msg_Uninit(USOCKIPC_ID id)
{
	NVTIPC_I32 Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

#ifdef USOCKET_SIM
	gIPC_MsgId[id] = gIPC_MsgId_uITRON;
#endif

	Ret_NvtIpc = NvtIPC_MsgRel(gIPC_MsgId[id]);
	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgRel = %d\r\n",id, Ret_NvtIpc);
		return E_SYS;
	}
	DBG_IND("[MSG]%d MsgId(%d) released\r\n",id, gIPC_MsgId[id]);
	gIPC_MsgId[id] = USOCKIPC_INVALID_MSGID;

	DBG_FUNC_END("[MSG]\r\n");
	return E_OK;
}

ER UsockIpc_Msg_WaitCmd(USOCKIPC_ID id,USOCKET_MSG *pRcvMsg)
{
	NVTIPC_I32  Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

#ifdef USOCKET_SIM
	gIPC_MsgId[id] = gIPC_MsgId_uITRON;
#endif

	Ret_NvtIpc = NvtIPC_MsgRcv(gIPC_MsgId[id], pRcvMsg, sizeof(USOCKET_MSG));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgRcv = %d\r\n",id, Ret_NvtIpc);
		return E_SYS;
	}

	DBG_FUNC_END("[MSG]\r\n");
	return E_OK;
}

ER UsockIpc_Msg_AckCmd(USOCKIPC_ID id,INT32 AckValue)
{
	USOCKET_MSG IpcMsg = {0};
	NVTIPC_I32  Ret_NvtIpc;

	if (USOCKIPC_INVALID_MSGID == gIPC_MsgId[id]) {
		DBG_ERR("%d USOCKIPC_INVALID_MSGID\n",id);
		return E_SYS;
	}

	IpcMsg.CmdId = USOCKET_CMDID_SYSREQ_ACK;
	IpcMsg.Arg = 0;

#ifdef USOCKET_SIM
	gIPC_MsgId[id] = gIPC_MsgId_eCos;
#endif

	Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId[id], USOCKIPC_SEND_TARGET, &IpcMsg, sizeof(IpcMsg));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgSnd = %d\r\n",id, Ret_NvtIpc);
		return E_SYS;
	}

	return E_OK;
}

ER UsockIpc_Msg_SysCallReq(USOCKIPC_ID id,CHAR *szCmd)
{
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);
	NVTIPC_SYS_MSG SysMsg;
	NVTIPC_I32 Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

	DBG_IND("[MSG]%d szCmd = %s\r\n",id, szCmd);

	//before sending system call request, clear the ack flag
	clr_flg(p_ctrl->flag_id, USOCKIPC_FLGBIT_SYSREQ_GOTACK);

	//send syscall request ipc to FileSysEcos to close
	SysMsg.sysCmdID = NVTIPC_SYSCMD_SYSCALL_REQ;
	SysMsg.DataAddr = (NVTIPC_U32)szCmd;
	SysMsg.DataSize = (NVTIPC_U32)strlen(szCmd) + 1;

#ifdef USOCKET_SIM
	USOCKETECOS_CmdLine(szCmd);
	Ret_NvtIpc = 0;
#else
	Ret_NvtIpc = NvtIPC_MsgSnd(NVTIPC_SYS_QUEUE_ID, USOCKIPC_SEND_TARGET, &SysMsg, sizeof(SysMsg));
#endif

	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgSnd(SYSCALL_REQ) = %d\r\n",id, Ret_NvtIpc);
		return E_SYS;
	}

	DBG_FUNC_END("[MSG]\r\n");
	return E_OK;
}
ER UsockIpc_Msg_SendCmd(USOCKIPC_ID id,USOCKET_CMDID CmdId, int *pOutRet)
{
	USOCKET_MSG IpcMsg = {0};
	NVTIPC_I32  Ret_NvtIpc;

	if (USOCKIPC_INVALID_MSGID == gIPC_MsgId[id]) {
		DBG_ERR("%d USOCKIPC_INVALID_MSGID\n",id);
		return E_SYS;
	}

	IpcMsg.CmdId = CmdId;
	IpcMsg.Arg = 0;

#ifdef USOCKET_SIM
	gIPC_MsgId[id] = gIPC_MsgId_eCos;
#endif
	DBG_IND("%d Send CmdId = %d\r\n",id, CmdId);
	Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId[id], USOCKIPC_SEND_TARGET, &IpcMsg, sizeof(IpcMsg));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgSnd = %d\r\n",id, Ret_NvtIpc);
		return E_SYS;
	}

	*pOutRet = UsockIpc_Msg_WaitAck(id);
	return E_OK;
}

UINT32 UsockIpc_Msg_WaitAck(USOCKIPC_ID id)
{
	FLGPTN FlgPtn;
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);

	wai_flg(&FlgPtn, p_ctrl->flag_id, USOCKIPC_FLGBIT_SYSREQ_GOTACK, TWF_ORW | TWF_CLR);

	return gMsgAck[id];
}

void UsockIpc_Msg_SetAck(USOCKIPC_ID id,INT32 msgAck)
{
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);
	gMsgAck[id] = msgAck;
	set_flg(p_ctrl->flag_id, USOCKIPC_FLGBIT_SYSREQ_GOTACK);
}
#endif
