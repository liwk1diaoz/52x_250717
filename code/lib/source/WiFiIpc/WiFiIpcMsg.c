#include "Type.h"
#include "NvtIpcAPI.h"
#include "WiFiIpcInt.h"
#include "WiFiIpcID.h"
#include "WiFiIpcMsg.h"
#include "WiFiIpcTsk.h"

#if defined(_NETWORK_ON_CPU2_)

#define WIFIIPC_INVALID_MSGID NVTIPC_MSG_QUEUE_NUM+1
#define WIFIIPC_SEND_TARGET NVTIPC_SENDTO_CORE2

//global variables
static NVTIPC_U32 gIPC_MsgId  = WIFIIPC_INVALID_MSGID;
static UINT32 gMsgAck = 0;

#ifdef NVTWIFI_SIM
static NVTIPC_U32 gIPC_MsgId_uITRON = 1;
static NVTIPC_U32 gIPC_MsgId_eCos = 2;
#endif //#ifdef NVTWIFI_SIM

ER WiFiIpc_Msg_Init()
{
	NVTIPC_I32 Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

	//get ipc msg id
#ifdef NVTWIFI_SIM
	DBG_ERR("============================================\r\n");
	DBG_ERR("============= WiFiIpc simulation =============\r\n");
	DBG_ERR("============================================\r\n");
	Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok("WIFIIPC uITRON"));
#else
	Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(NVTWIFI_TOKEN_PATH));
#endif

	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgGet = %d\r\n", Ret_NvtIpc);
		return E_SYS;
	}
	gIPC_MsgId = (NVTIPC_U32)Ret_NvtIpc;
	DBG_IND("[MSG]Got MsgId(%d)\r\n", gIPC_MsgId);

#ifdef NVTWIFI_SIM
	if (gIPC_MsgId != gIPC_MsgId_uITRON) {
		DBG_ERR("Simulation environment error\n");
		return E_SYS;
	}
#endif

	DBG_FUNC_END("[MSG]\r\n");
	return E_OK;
}

ER WiFiIpc_Msg_Uninit()
{
	NVTIPC_I32 Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

#ifdef NVTWIFI_SIM
	gIPC_MsgId = gIPC_MsgId_uITRON;
#endif

	Ret_NvtIpc = NvtIPC_MsgRel(gIPC_MsgId);
	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgRel = %d\r\n", Ret_NvtIpc);
		return E_SYS;
	}
	DBG_IND("[MSG]MsgId(%d) released\r\n", gIPC_MsgId);
	gIPC_MsgId = WIFIIPC_INVALID_MSGID;

	DBG_FUNC_END("[MSG]\r\n");
	return E_OK;
}

ER WiFiIpc_Msg_WaitCmd(NVTWIFI_MSG *pRcvMsg)
{
	NVTIPC_I32  Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

#ifdef NVTWIFI_SIM
	gIPC_MsgId = gIPC_MsgId_uITRON;
#endif

	Ret_NvtIpc = NvtIPC_MsgRcv(gIPC_MsgId, pRcvMsg, sizeof(NVTWIFI_MSG));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgRcv = %d\r\n", Ret_NvtIpc);
		return E_SYS;
	}

	DBG_FUNC_END("[MSG]\r\n");
	return E_OK;
}

ER WiFiIpc_Msg_AckCmd(INT32 AckValue)
{
	NVTWIFI_MSG IpcMsg = {0};
	NVTIPC_I32  Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");


	IpcMsg.CmdId = NVTWIFI_CMDID_SYSREQ_ACK;
	IpcMsg.Arg = AckValue;
#ifdef NVTWIFI_SIM
	gIPC_MsgId = gIPC_MsgId_eCos;
#endif

	Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId, WIFIIPC_SEND_TARGET, &IpcMsg, sizeof(IpcMsg));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgSnd = %d\r\n", Ret_NvtIpc);
		return E_SYS;
	}

	DBG_FUNC_END("[MSG]\r\n");
	return E_OK;
}

ER WiFiIpc_Msg_SysCallReq(CHAR *szCmd)
{
	NVTIPC_SYS_MSG SysMsg;
	NVTIPC_I32 Ret_NvtIpc;
	DBG_FUNC_BEGIN("[MSG]\r\n");

	DBG_IND("[MSG]szCmd = %s\r\n", szCmd);

	//before sending system call request, clear the ack flag
	clr_flg(WIFIIPC_FLG_ID, WIFIIPC_FLGBIT_SYSREQ_GOTACK);

	//send syscall request ipc to FileSysEcos to close
	SysMsg.sysCmdID = NVTIPC_SYSCMD_SYSCALL_REQ;
	SysMsg.DataAddr = (NVTIPC_U32)szCmd;
	SysMsg.DataSize = (NVTIPC_U32)strlen(szCmd) + 1;

#ifdef NVTWIFI_SIM
	NvtWifiECOS_CmdLine(szCmd);
	Ret_NvtIpc = 0;
#else
	Ret_NvtIpc = NvtIPC_MsgSnd(NVTIPC_SYS_QUEUE_ID, WIFIIPC_SEND_TARGET, &SysMsg, sizeof(SysMsg));
#endif

	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgSnd(SYSCALL_REQ) = %d\r\n", Ret_NvtIpc);
		\
		return E_SYS;
	}

	DBG_FUNC_END("[MSG]\r\n");
	return E_OK;
}
ER WiFiIpc_Msg_SendCmd(NVTWIFI_CMDID CmdId, int *pOutRet)
{
	NVTWIFI_MSG IpcMsg = {0};
	NVTIPC_I32  Ret_NvtIpc;

	if (WIFIIPC_INVALID_MSGID == gIPC_MsgId) {
		DBG_ERR("WIFIIPC_INVALID_MSGID\n");
		return E_SYS;
	}

	IpcMsg.CmdId = CmdId;
	IpcMsg.Arg = 0;

#ifdef NVTWIFI_SIM
	gIPC_MsgId = gIPC_MsgId_eCos;
#endif
	DBG_IND("Send CmdId = %d\r\n", CmdId);
	Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId, WIFIIPC_SEND_TARGET, &IpcMsg, sizeof(IpcMsg));
	if (Ret_NvtIpc < 0) {
		DBG_ERR("NvtIPC_MsgSnd = %d\r\n", Ret_NvtIpc);
		return E_SYS;
	}

	*pOutRet = WiFiIpc_Msg_WaitAck();
	return E_OK;
}

UINT32 WiFiIpc_Msg_WaitAck(void)
{
	FLGPTN FlgPtn;

	wai_flg(&FlgPtn, WIFIIPC_FLG_ID, WIFIIPC_FLGBIT_SYSREQ_GOTACK, TWF_ORW | TWF_CLR);

	return gMsgAck;
}

void WiFiIpc_Msg_SetAck(INT32 msgAck)
{
	gMsgAck = msgAck;
	set_flg(WIFIIPC_FLG_ID, WIFIIPC_FLGBIT_SYSREQ_GOTACK);
}

#endif
