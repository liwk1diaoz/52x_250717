#include "UsockCliIpc/UsockCliIpcAPI.h"
#if (USE_IPC)
#include "NvtIpcAPI.h"
#include "UsockCliIpcInt.h"
#include "UsockCliIpcID.h"
#include "UsockCliIpcMsg.h"
#include "UsockCliIpcTsk.h"

#define USOCKCLIIPC_INVALID_MSGID NVTIPC_MSG_QUEUE_NUM+1
#define USOCKCLIIPC_SEND_TARGET NVTIPC_SENDTO_CORE2

//global variables
static NVTIPC_U32 gIPC_MsgId  = USOCKCLIIPC_INVALID_MSGID;
static UINT32 gMsgAck = 0;

#ifdef USOCKETCLI_SIM
static NVTIPC_U32 gIPC_MsgId_uITRON = 1;
static NVTIPC_U32 gIPC_MsgId_eCos = 2;
#endif //#ifdef USOCKETCLI_SIM

ER UsockCliIpc_Msg_Init()
{
    NVTIPC_I32 Ret_NvtIpc;
    DBG_FUNC_BEGIN("[MSG]\r\n");

    //get ipc msg id
#ifdef USOCKETCLI_SIM
    DBG_ERR("============================================\r\n");
    DBG_ERR("============= UsockCliIpc simulation =============\r\n");
    DBG_ERR("============================================\r\n");
    Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok("USOCKCLIIPC uITRON"));
#else
    Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(USOCKETCLI_TOKEN_PATH));
#endif

    if(Ret_NvtIpc < 0)
    {
        DBG_ERR("NvtIPC_MsgGet = %d\r\n", Ret_NvtIpc);
        return E_SYS;
    }
    gIPC_MsgId = (NVTIPC_U32)Ret_NvtIpc;
    DBG_IND("[MSG]Got MsgId(%d)\r\n", gIPC_MsgId);

#ifdef USOCKETCLI_SIM
    if(gIPC_MsgId != gIPC_MsgId_uITRON)
    {
        DBG_ERR("Simulation environment error\n");
        return E_SYS;
    }
#endif

    DBG_FUNC_END("[MSG]\r\n");
    return E_OK;
}

ER UsockCliIpc_Msg_Uninit()
{
    NVTIPC_I32 Ret_NvtIpc;
    DBG_FUNC_BEGIN("[MSG]\r\n");

#ifdef USOCKETCLI_SIM
    gIPC_MsgId = gIPC_MsgId_uITRON;
#endif

    Ret_NvtIpc = NvtIPC_MsgRel(gIPC_MsgId);
    if(Ret_NvtIpc < 0)
    {
        DBG_ERR("NvtIPC_MsgRel = %d\r\n", Ret_NvtIpc);
        return E_SYS;
    }
    DBG_IND("[MSG]MsgId(%d) released\r\n", gIPC_MsgId);
    gIPC_MsgId = USOCKCLIIPC_INVALID_MSGID;

    DBG_FUNC_END("[MSG]\r\n");
    return E_OK;
}

ER UsockCliIpc_Msg_WaitCmd(USOCKETCLI_MSG *pRcvMsg)
{
    NVTIPC_I32  Ret_NvtIpc;
    DBG_FUNC_BEGIN("[MSG]\r\n");

#ifdef USOCKETCLI_SIM
    gIPC_MsgId = gIPC_MsgId_uITRON;
#endif

    Ret_NvtIpc = NvtIPC_MsgRcv(gIPC_MsgId, pRcvMsg, sizeof(USOCKETCLI_MSG));
    if(Ret_NvtIpc < 0)
    {
        DBG_ERR("NvtIPC_MsgRcv = %d\r\n", Ret_NvtIpc);
        return E_SYS;
    }

    DBG_FUNC_END("[MSG]\r\n");
    return E_OK;
}

ER UsockCliIpc_Msg_AckCmd(INT32 AckValue)
{
    USOCKETCLI_MSG IpcMsg = {0};
    NVTIPC_I32  Ret_NvtIpc;

    if(USOCKCLIIPC_INVALID_MSGID == gIPC_MsgId)
    {
        DBG_ERR("USOCKCLIIPC_INVALID_MSGID\n");
        return E_SYS;
    }

    IpcMsg.CmdId = USOCKETCLI_CMDID_SYSREQ_ACK;
    IpcMsg.Arg = 0;

    #ifdef USOCKETCLI_SIM
    gIPC_MsgId = gIPC_MsgId_eCos;
    #endif

    Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId, USOCKCLIIPC_SEND_TARGET, &IpcMsg, sizeof(IpcMsg));
    if(Ret_NvtIpc < 0)
    {
        DBG_ERR("NvtIPC_MsgSnd = %d\r\n", Ret_NvtIpc);
        return E_SYS;
    }

    return E_OK;
}

ER UsockCliIpc_Msg_SysCallReq(CHAR* szCmd)
{
    NVTIPC_SYS_MSG SysMsg;
    NVTIPC_I32 Ret_NvtIpc;
    DBG_FUNC_BEGIN("[MSG]\r\n");

    DBG_IND("[MSG]szCmd = %s\r\n", szCmd);

    //before sending system call request, clear the ack flag
    clr_flg(USOCKCLIIPC_FLG_ID, USOCKCLIIPC_FLGBIT_SYSREQ_GOTACK);

    //send syscall request ipc to FileSysEcos to close
    SysMsg.sysCmdID = NVTIPC_SYSCMD_SYSCALL_REQ;
    SysMsg.DataAddr = (NVTIPC_U32)szCmd;
    SysMsg.DataSize = (NVTIPC_U32)strlen(szCmd)+1;

#ifdef USOCKETCLI_SIM
    USOCKETECOS_CmdLine(szCmd);
    Ret_NvtIpc = 0;
#else
    Ret_NvtIpc = NvtIPC_MsgSnd(NVTIPC_SYS_QUEUE_ID, USOCKCLIIPC_SEND_TARGET, &SysMsg, sizeof(SysMsg));
#endif

    if(Ret_NvtIpc < 0)
    {
        DBG_ERR("NvtIPC_MsgSnd(SYSCALL_REQ) = %d\r\n", Ret_NvtIpc);\
        return E_SYS;
    }

    DBG_FUNC_END("[MSG]\r\n");
    return E_OK;
}
ER UsockCliIpc_Msg_SendCmd(USOCKETCLI_CMDID CmdId, int *pOutRet)
{
    USOCKETCLI_MSG IpcMsg = {0};
    NVTIPC_I32  Ret_NvtIpc;

    if(USOCKCLIIPC_INVALID_MSGID == gIPC_MsgId)
    {
        DBG_ERR("USOCKCLIIPC_INVALID_MSGID\n");
        return E_SYS;
    }

    IpcMsg.CmdId = CmdId;
    IpcMsg.Arg = 0;

    #ifdef USOCKETCLI_SIM
    gIPC_MsgId = gIPC_MsgId_eCos;
    #endif
    DBG_IND("Send CmdId = %d\r\n", CmdId);
    Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId, USOCKCLIIPC_SEND_TARGET, &IpcMsg, sizeof(IpcMsg));
    if(Ret_NvtIpc < 0)
    {
        DBG_ERR("NvtIPC_MsgSnd = %d\r\n", Ret_NvtIpc);
        return E_SYS;
    }

    *pOutRet = UsockCliIpc_Msg_WaitAck();
    return E_OK;
}

UINT32 UsockCliIpc_Msg_WaitAck(void)
{
    FLGPTN FlgPtn;

    wai_flg(&FlgPtn, USOCKCLIIPC_FLG_ID, USOCKCLIIPC_FLGBIT_SYSREQ_GOTACK, TWF_ORW|TWF_CLR);

    return gMsgAck;
}

void UsockCliIpc_Msg_SetAck(INT32 msgAck)
{
    gMsgAck = msgAck;
    set_flg(USOCKCLIIPC_FLG_ID, USOCKCLIIPC_FLGBIT_SYSREQ_GOTACK);
}
#endif
