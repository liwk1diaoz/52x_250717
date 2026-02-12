#include <stdio.h>
#include <string.h>
#include "kwrap/type.h"
#include "EthsockCliIpcInt.h"
#include "EthsockCliIpcID.h"
#include "EthsockCliIpcMsg.h"
#include "EthsockCliIpcTsk.h"

#if (USE_IPC)
#include "NvtIpcAPI.h"
#endif
#include "nvtipc.h"
#include "Utility/SwTimer.h"

#define ETHSOCKCLIIPC_INVALID_MSGID     (NVTIPC_MSG_QUEUE_NUM + 1)
#define ETHSOCKCLIIPC_SEND_TARGET        NVTIPC_SENDTO_CORE2

#if (USE_IPC)
//global variables
static NVTIPC_U32 gIPC_MsgId[MAX_ETH_PATH_NUM][MAX_ETHSOCKETCLI_NUM]  = {ETHSOCKCLIIPC_INVALID_MSGID};//,ETHSOCKCLIIPC_INVALID_MSGID,ETHSOCKCLIIPC_INVALID_MSGID};
static UINT32 gMsgAck[MAX_ETH_PATH_NUM][MAX_ETHSOCKETCLI_NUM] = {0};
#endif

#ifdef ETHSOCKETCLI_SIM
static NVTIPC_U32 gIPC_MsgId_uITRON = 1;
static NVTIPC_U32 gIPC_MsgId_eCos = 2;
#endif //#ifdef ETHSOCKETCLI_SIM

ER EthsockCliIpc_Msg_Init(UINT32 path_id, ETHSOCKIPCCLI_ID id)
{
#if (USE_IPC)
	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(path_id, id);
	NVTIPC_I32 Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

	//get ipc msg id
#ifdef ETHSOCKETCLI_SIM
	DBG_ERR("====================================================\r\n");
	DBG_ERR("============= EthsockCliIpc simulation =============\r\n");
	DBG_ERR("====================================================\r\n");
	Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok("ETHSOCKCLIIPC uITRON"));
#else
	//Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(ETHSOCKETCLI_TOKEN_PATH0));
	Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(p_ctrl->token_path));
#endif

	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgGet = %d\r\n", id, Ret_NvtIpc);
		return E_SYS;
	}
	gIPC_MsgId[path_id][id] = (NVTIPC_U32)Ret_NvtIpc;
	DBG_IND("[MSG]%d Got MsgId(%d)\r\n", id, gIPC_MsgId[path_id][id]);

#ifdef ETHSOCKETCLI_SIM
	if (gIPC_MsgId[path_id][id] != gIPC_MsgId_uITRON) {
		DBG_ERR("%d %d Simulation environment error\n",path_id, id);
		return E_SYS;
	}
#endif

	DBG_FUNC_END("[MSG]\r\n");
#endif
	return E_OK;
}

ER EthsockCliIpc_Msg_Uninit(UINT32 path_id, ETHSOCKIPCCLI_ID id)
{
#if (USE_IPC)
	NVTIPC_I32 Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

#ifdef ETHSOCKETCLI_SIM
	gIPC_MsgId[path_id][id] = gIPC_MsgId_uITRON;
#endif

	Ret_NvtIpc = NvtIPC_MsgRel(gIPC_MsgId[path_id][id]);
	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgRel = %d\r\n", id, Ret_NvtIpc);
		return E_SYS;
	}
	DBG_IND("[MSG]%d MsgId(%d) released\r\n", id, gIPC_MsgId[path_id][id]);
	gIPC_MsgId[path_id][id] = ETHSOCKCLIIPC_INVALID_MSGID;

	DBG_FUNC_END("[MSG]\r\n");
#endif
	return E_OK;
}

ER EthsockCliIpc_Msg_WaitCmd(UINT32 path_id, ETHSOCKIPCCLI_ID id, ETHSOCKETCLI_MSG *pRcvMsg)
{
#if (USE_IPC)

	NVTIPC_I32  Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

#ifdef ETHSOCKETCLI_SIM
	gIPC_MsgId[path_id][id] = gIPC_MsgId_uITRON;
#endif

	Ret_NvtIpc = NvtIPC_MsgRcv(gIPC_MsgId[path_id][id], pRcvMsg, sizeof(ETHSOCKETCLI_MSG));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d NvtIPC_MsgRcv = %d\r\n", id, Ret_NvtIpc);
		return E_SYS;
	}

	DBG_FUNC_END("[MSG]\r\n");
#endif
	return E_OK;
}

ER EthsockCliIpc_Msg_AckCmd(UINT32 path_id, ETHSOCKIPCCLI_ID id, INT32 AckValue)
{
#if (USE_IPC)
	ETHSOCKETCLI_MSG IpcMsg = {0};
	NVTIPC_I32  Ret_NvtIpc;

	if (ETHSOCKCLIIPC_INVALID_MSGID == gIPC_MsgId[path_id][id]) {
		DBG_ERR("%d ETHSOCKCLIIPC_INVALID_MSGID\n",id);
		return E_SYS;
	}

	IpcMsg.CmdId = ETHSOCKETCLI_CMDID_SYSREQ_ACK;
	IpcMsg.Arg = 0;

#ifdef ETHSOCKETCLI_SIM
	gIPC_MsgId[path_id][id] = gIPC_MsgId_eCos;
#endif

	Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId[path_id][id], ETHSOCKCLIIPC_SEND_TARGET, &IpcMsg, sizeof(IpcMsg));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("%d %d NvtIPC_MsgSnd = %d\r\n", path_id, id, Ret_NvtIpc);
		return E_SYS;
	}
#endif
	return E_OK;
}

ER EthsockCliIpc_Msg_SysCallReq(UINT32 path_id, ETHSOCKIPCCLI_ID id, CHAR* szCmd)
{
	NVTIPC_I32 Ret_NvtIpc;
#if (USE_IPC)
	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(path_id, id);
	NVTIPC_SYS_MSG SysMsg;
	DBG_FUNC_BEGIN("[MSG]\r\n");

	DBG_IND("[MSG]%d %d szCmd = %s\r\n",path_id, id, szCmd);

	//before sending system call request, clear the ack flag
	//clr_flg(ETHSOCKCLIIPC_FLG_ID_0, ETHSOCKCLIIPC_FLGBIT_SYSREQ_GOTACK);
	clr_flg(p_ctrl->flag_id, ETHSOCKCLIIPC_FLGBIT_SYSREQ_GOTACK);

	//send syscall request ipc to FileSysEcos to close
	SysMsg.sysCmdID = NVTIPC_SYSCMD_SYSCALL_REQ;
	SysMsg.DataAddr = (NVTIPC_U32)szCmd;
	SysMsg.DataSize = (NVTIPC_U32)strlen(szCmd) + 1;

#ifdef ETHSOCKETCLI_SIM
	ETHSOCKETECOS_CmdLine(szCmd);
	Ret_NvtIpc = 0;
#else
	Ret_NvtIpc = NvtIPC_MsgSnd(NVTIPC_SYS_QUEUE_ID, ETHSOCKCLIIPC_SEND_TARGET, &SysMsg, sizeof(SysMsg));
#endif

	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgSnd(SYSCALL_REQ) = %d %d %d\r\n",path_id, id, Ret_NvtIpc);
		return E_SYS;
	}

	DBG_FUNC_END("[MSG]\r\n");
#else
	Ret_NvtIpc = ETHSOCKETCLIECOS_CmdLine(szCmd, NULL,NULL);
	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgSnd(SYSCALL_REQ) = %d %d %d\r\n",path_id, id, Ret_NvtIpc);
		return E_SYS;
	}
#endif
	return E_OK;
}

ER EthsockCliIpc_Msg_SendCmd(UINT32 path_id, ETHSOCKIPCCLI_ID id, ETHSOCKETCLI_CMDID CmdId, int *pOutRet)
{
#if (USE_IPC)

	ETHSOCKETCLI_MSG IpcMsg = {0};
	NVTIPC_I32  Ret_NvtIpc;

	if (ETHSOCKCLIIPC_INVALID_MSGID == gIPC_MsgId[path_id][id]) {
		DBG_ERR("ETHSOCKCLIIPC_INVALID_MSGID\n",id);
		return E_SYS;
	}

	IpcMsg.CmdId = CmdId;
	IpcMsg.Arg = 0;

#ifdef ETHSOCKETCLI_SIM
	gIPC_MsgId[path_id][id] = gIPC_MsgId_eCos;
#endif
	DBG_IND("d Send CmdId = %d\r\n", id, CmdId);
	if ((Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId[path_id][id], ETHSOCKCLIIPC_SEND_TARGET, &IpcMsg, sizeof(IpcMsg))) < 0) {
		DBG_ERR("NvtIPC_MsgSnd = %d\r\n", Ret_NvtIpc);
		return E_SYS;
	}

	*pOutRet = EthsockCliIpc_Msg_WaitAck(path_id, id);
#else
	*pOutRet =ETHSOCKETCLIECOS_HandleCmd(CmdId, path_id, id);
#endif
	return E_OK;
}
#if (USE_IPC)
static void EthsockCliIpc_Msg_TimerCb0_p0(UINT32 uiEvent)
{
	DBG_ERR("timeout!\r\n");

	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(0, ETHSOCKIPCCLI_ID_0);
	set_flg(p_ctrl->flag_id, ETHSOCKCLIIPC_FLGBIT_SYSREQ_TIMEOUT);
}
static void EthsockCliIpc_Msg_TimerCb1_p0(UINT32 uiEvent)
{
	DBG_ERR("timeout!\r\n");

	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(0, ETHSOCKIPCCLI_ID_1);
	set_flg(p_ctrl->flag_id, ETHSOCKCLIIPC_FLGBIT_SYSREQ_TIMEOUT);
}
static void EthsockCliIpc_Msg_TimerCb2_p0(UINT32 uiEvent)
{
	DBG_ERR("timeout!\r\n");

	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(0,  ETHSOCKIPCCLI_ID_2);
	set_flg(p_ctrl->flag_id, ETHSOCKCLIIPC_FLGBIT_SYSREQ_TIMEOUT);
}
static void EthsockCliIpc_Msg_TimerCb0_p1(UINT32 uiEvent)
{
	DBG_ERR("timeout!\r\n");

	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(0, ETHSOCKIPCCLI_ID_0);
	set_flg(p_ctrl->flag_id, ETHSOCKCLIIPC_FLGBIT_SYSREQ_TIMEOUT);
}
static void EthsockCliIpc_Msg_TimerCb1_p1(UINT32 uiEvent)
{
	DBG_ERR("timeout!\r\n");

	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(0, ETHSOCKIPCCLI_ID_1);
	set_flg(p_ctrl->flag_id, ETHSOCKCLIIPC_FLGBIT_SYSREQ_TIMEOUT);
}
static void EthsockCliIpc_Msg_TimerCb2_p1(UINT32 uiEvent)
{
	DBG_ERR("timeout!\r\n");

	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(0,  ETHSOCKIPCCLI_ID_2);
	set_flg(p_ctrl->flag_id, ETHSOCKCLIIPC_FLGBIT_SYSREQ_TIMEOUT);
}
static SWTIMER_CB gCliIpcMsgTimerCb[2][ETHSOCKIPCCLI_MAX_NUM] = {
	{EthsockCliIpc_Msg_TimerCb0_p0, EthsockCliIpc_Msg_TimerCb1_p0, EthsockCliIpc_Msg_TimerCb2_p0},
	{EthsockCliIpc_Msg_TimerCb0_p1, EthsockCliIpc_Msg_TimerCb1_p1, EthsockCliIpc_Msg_TimerCb2_p1}
};
#endif
UINT32 EthsockCliIpc_Msg_WaitAck(UINT32 path_id, ETHSOCKIPCCLI_ID id)
{
#if (USE_IPC)

	FLGPTN FlgPtn;
	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(path_id, id);
	SWTIMER_ID   timer_id;
	BOOL         isOpenTimerOK = TRUE;
	SWTIMER_CB EventHandler;

	EventHandler=(SWTIMER_CB)gCliIpcMsgTimerCb[path_id][id];
	if (SwTimer_Open(&timer_id, EventHandler) != E_OK) {
		DBG_ERR("open timer fail\r\n");
		isOpenTimerOK = FALSE;
	}
	if(isOpenTimerOK){
		SwTimer_Cfg(timer_id, 30000, SWTIMER_MODE_ONE_SHOT);
		SwTimer_Start(timer_id);
	}

	wai_flg(&FlgPtn, p_ctrl->flag_id, ETHSOCKCLIIPC_FLGBIT_SYSREQ_GOTACK|ETHSOCKCLIIPC_FLGBIT_SYSREQ_TIMEOUT, TWF_ORW|TWF_CLR);

	if (isOpenTimerOK) {
		SwTimer_Stop(timer_id);
		SwTimer_Close(timer_id);
	}

	if (FlgPtn & ETHSOCKCLIIPC_FLGBIT_SYSREQ_TIMEOUT) {
		return E_SYS;
	}

	return gMsgAck[path_id][id];
#else
	return E_OK;
#endif
}

void EthsockCliIpc_Msg_SetAck(UINT32 path_id, ETHSOCKIPCCLI_ID id, INT32 msgAck)
{
#if (USE_IPC)
	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(path_id, id);
	gMsgAck[path_id][id] = msgAck;
	set_flg(p_ctrl->flag_id, ETHSOCKCLIIPC_FLGBIT_SYSREQ_GOTACK);
#endif
}

