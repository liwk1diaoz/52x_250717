#include <stdio.h>
#include <string.h>
#include "kwrap/type.h"

#include "EthCam/EthsockIpcAPI.h"
#include "EthsockIpcInt.h"
#include "EthsockIpcID.h"
#include "EthsockIpcMsg.h"
#include "EthsockIpcTsk.h"

#if (USE_IPC)
#include "NvtIpcAPI.h"
#endif


#define ETHSOCKIPC_INVALID_MSGID              (NVTIPC_MSG_QUEUE_NUM + 1)
#define ETHSOCKIPC_SEND_TARGET                 NVTIPC_SENDTO_CORE2

#if (USE_IPC)
//global variables
static NVTIPC_U32 gIPC_MsgId[MAX_ETHSOCKET_NUM]  = {ETHSOCKIPC_INVALID_MSGID, ETHSOCKIPC_INVALID_MSGID, ETHSOCKIPC_INVALID_MSGID};
static INT32 gMsgAck[MAX_ETHSOCKET_NUM] = {0};
#endif

#ifdef ETHSOCKET_SIM
static NVTIPC_U32 gIPC_MsgId_uITRON = 1;
static NVTIPC_U32 gIPC_MsgId_eCos = 2;
#endif //#ifdef ETHSOCKET_SIM

ER EthsockIpc_Msg_Init(ETHSOCKIPC_ID id)
{
#if (USE_IPC)
	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);
	NVTIPC_I32 Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

	//get ipc msg id
#ifdef ETHSOCKET_SIM
	DBG_ERR("============================================\r\n");
	DBG_ERR("=========== EthsockIpc simulation ==========\r\n");
	DBG_ERR("============================================\r\n");
	Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok("ETHSOCKIPC uITRON"));
#else
	Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(p_ctrl->token_path));
#endif

	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgGet = %d\r\n", id, Ret_NvtIpc);
		return E_SYS;
	}
	gIPC_MsgId[id] = (NVTIPC_U32)Ret_NvtIpc;
	DBG_IND("[MSG]%d Got MsgId(%d)\r\n", id, gIPC_MsgId[id]);

#ifdef ETHSOCKET_SIM
	if (gIPC_MsgId[id] != gIPC_MsgId_uITRON) {
		DBG_ERR("%d Simulation environment error\n", id);
		return E_SYS;
	}
#endif

	DBG_FUNC_END("[MSG]\r\n");
#endif
	return E_OK;
}

ER EthsockIpc_Msg_Uninit(ETHSOCKIPC_ID id)
{
#if (USE_IPC)

	NVTIPC_I32 Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

#ifdef ETHSOCKET_SIM
	gIPC_MsgId[id] = gIPC_MsgId_uITRON;
#endif

	Ret_NvtIpc = NvtIPC_MsgRel(gIPC_MsgId[id]);
	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgRel = %d\r\n", id, Ret_NvtIpc);
		return E_SYS;
	}
	DBG_IND("[MSG]%d MsgId(%d) released\r\n", id, gIPC_MsgId[id]);
	gIPC_MsgId[id] = ETHSOCKIPC_INVALID_MSGID;

	DBG_FUNC_END("[MSG]\r\n");
#endif
	return E_OK;
}

ER EthsockIpc_Msg_WaitCmd(ETHSOCKIPC_ID id, ETHSOCKET_MSG *pRcvMsg)
{
#if (USE_IPC)

	NVTIPC_I32  Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

#ifdef ETHSOCKET_SIM
	gIPC_MsgId[id] = gIPC_MsgId_uITRON;
#endif

	Ret_NvtIpc = NvtIPC_MsgRcv(gIPC_MsgId[id], pRcvMsg, sizeof(ETHSOCKET_MSG));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgRcv = %d\r\n", id, Ret_NvtIpc);
		return E_SYS;
	}

	DBG_FUNC_END("[MSG]\r\n");
#endif
	return E_OK;
}

ER EthsockIpc_Msg_AckCmd(ETHSOCKIPC_ID id, INT32 AckValue)
{
#if (USE_IPC)
	ETHSOCKET_MSG IpcMsg = {0};
	NVTIPC_I32  Ret_NvtIpc;

	if (ETHSOCKIPC_INVALID_MSGID == gIPC_MsgId[id]) {
		DBG_ERR("%d ETHSOCKIPC_INVALID_MSGID\n", id);
		return E_SYS;
	}

	IpcMsg.CmdId = ETHSOCKET_CMDID_SYSREQ_ACK;
	IpcMsg.Arg = 0;

#ifdef ETHSOCKET_SIM
	gIPC_MsgId[id] = gIPC_MsgId_eCos;
#endif

	Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId[id], ETHSOCKIPC_SEND_TARGET, &IpcMsg, sizeof(IpcMsg));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgSnd = %d\r\n", id, Ret_NvtIpc);
		return E_SYS;
	}
#endif
	return E_OK;
}

ER EthsockIpc_Msg_SysCallReq(ETHSOCKIPC_ID id, CHAR *szCmd)
{
	int Ret_NvtIpc;

#if (USE_IPC)

	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);
	NVTIPC_SYS_MSG SysMsg;
	//NVTIPC_I32 Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

	DBG_IND("[MSG]%d szCmd = %s\r\n", id, szCmd);

	//before sending system call request, clear the ack flag
	clr_flg(p_ctrl->flag_id, ETHSOCKIPC_FLGBIT_SYSREQ_GOTACK);

	//send syscall request ipc to FileSysEcos to close
	SysMsg.sysCmdID = NVTIPC_SYSCMD_SYSCALL_REQ;
	SysMsg.DataAddr = (NVTIPC_U32)szCmd;
	SysMsg.DataSize = (NVTIPC_U32)strlen(szCmd) + 1;

#ifdef ETHSOCKET_SIM
	ETHSOCKETECOS_CmdLine(szCmd);
	Ret_NvtIpc = 0;
#else
	Ret_NvtIpc = NvtIPC_MsgSnd(NVTIPC_SYS_QUEUE_ID, ETHSOCKIPC_SEND_TARGET, &SysMsg, sizeof(SysMsg));
#endif

	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgSnd(SYSCALL_REQ) = %d\r\n", id, Ret_NvtIpc);
		return E_SYS;
	}

	DBG_FUNC_END("[MSG]\r\n");
#else
	Ret_NvtIpc = ETHSOCKETECOS_CmdLine(szCmd, NULL,NULL);
	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgSnd(SYSCALL_REQ) = %d %d\r\n", id, Ret_NvtIpc);
		return E_SYS;
	}
#endif
	return E_OK;
}
ER EthsockIpc_Msg_SendCmd(ETHSOCKIPC_ID id, ETHSOCKET_CMDID CmdId, int *pOutRet)
{
#if (USE_IPC)
	ETHSOCKET_MSG IpcMsg = {0};
	NVTIPC_I32  Ret_NvtIpc;

	if (ETHSOCKIPC_INVALID_MSGID == gIPC_MsgId[id]) {
		DBG_ERR("%d ETHSOCKIPC_INVALID_MSGID\n", id);
		return E_SYS;
	}

	IpcMsg.CmdId = CmdId;
	IpcMsg.Arg = 0;

#ifdef ETHSOCKET_SIM
	gIPC_MsgId[id] = gIPC_MsgId_eCos;
#endif
	DBG_IND("%d Send CmdId = %d\r\n", id, CmdId);
	Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId[id], ETHSOCKIPC_SEND_TARGET, &IpcMsg, sizeof(IpcMsg));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgSnd = %d\r\n", id, Ret_NvtIpc);
		return E_SYS;
	}

	*pOutRet = EthsockIpc_Msg_WaitAck(id);
#else
	*pOutRet =ETHSOCKETECOS_HandleCmd(CmdId,  id);

#endif
	return E_OK;
}

INT32 EthsockIpc_Msg_WaitAck(ETHSOCKIPC_ID id)
{
#if (USE_IPC)

	FLGPTN FlgPtn;
	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);

	wai_flg(&FlgPtn, p_ctrl->flag_id, ETHSOCKIPC_FLGBIT_SYSREQ_GOTACK, TWF_ORW | TWF_CLR);

	return gMsgAck[id];
#else
	return E_OK;
#endif
}

void EthsockIpc_Msg_SetAck(ETHSOCKIPC_ID id, INT32 msgAck)
{
#if (USE_IPC)
	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);
	gMsgAck[id] = msgAck;
	set_flg(p_ctrl->flag_id, ETHSOCKIPC_FLGBIT_SYSREQ_GOTACK);
#endif
}

