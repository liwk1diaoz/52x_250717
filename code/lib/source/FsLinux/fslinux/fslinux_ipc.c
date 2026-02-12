#include "fslinux_cmd.h"
#include "fslinux_debug.h"
#include "fslinux_ipc.h"

#if FSLINUX_USE_IPC
#define INVALID_MSGID (-1)

static int g_MsgId = INVALID_MSGID;//default invalid

int fslinux_ipc_Init(void)
{
    int key;

#if !defined(_BSP_NA50902_)
    // map dram1 all memory
    NvtIPC_mmap(NVTIPC_MEM_TYPE_NONCACHE,NVTIPC_MEM_ADDR_DRAM1_ALL_FLAG,0);
#endif

    //init NvtIPC
    key = NvtIPC_Ftok(FSLINUX_TOKEN_PATH);
    if(key <= 0)
    {
        DBG_ERR("NvtIPC_Ftok failed\r\n");
        return -1;
    }

    g_MsgId = NvtIPC_MsgGet(key);
    if(g_MsgId < 0)
    {
        DBG_ERR("NvtIPC_MsgGet failed\r\n");
        return -1;
    }
    DBG_IND("g_MsgId = %d\r\n", g_MsgId);

#ifdef FSLINUX_SIM
    if(g_MsgId != SIM_MSGID_DAEMON)
    {
        DBG_ERR("SIM environment ERR. g_MsgId should be %d\r\n", SIM_MSGID_DAEMON);
        NvtIPC_MsgRel(g_MsgId);
        g_MsgId = INVALID_MSGID;
        return -1;
    }
#endif

    return 0;
}

void fslinux_ipc_Uninit(void)
{
    if(INVALID_MSGID == g_MsgId)
        return;//skip

#ifdef FSLINUX_SIM
    g_MsgId = SIM_MSGID_DAEMON;
#endif

    NvtIPC_MsgRel(g_MsgId);
    g_MsgId = INVALID_MSGID;
}

int fslinux_ipc_WaitCmd(FSLINUX_IPCMSG *pRcvMsg)
{
    DBG_IND("begin\r\n");
    if(INVALID_MSGID == g_MsgId)
    {
        DBG_ERR("g_MsgId is invalid\r\n");
        return -1;
    }

#ifdef FSLINUX_SIM
    g_MsgId = SIM_MSGID_DAEMON;
#endif

    if(NvtIPC_MsgRcv(g_MsgId, pRcvMsg, sizeof(FSLINUX_IPCMSG)) < 0)
    {
        DBG_ERR("NvtIPC_MsgRcv failed\r\n");
        return -1;
    }

    DBG_IND("Drive %c, CmdId %d\r\n", pRcvMsg->Drive, pRcvMsg->CmdId);
    if(pRcvMsg->Drive < FSLINUX_DRIVE_NAME_FIRST || pRcvMsg->Drive > FSLINUX_DRIVE_NAME_LAST)
    {
        DBG_ERR("Drive(%c) is invalid. (Shoule be '%c'~'%c')\r\n", pRcvMsg->Drive, FSLINUX_DRIVE_NAME_FIRST, FSLINUX_DRIVE_NAME_LAST);
        return -1;
    }

    DBG_IND("end\r\n");
    return 0;
}

int fslinux_ipc_AckCmd(FSLINUX_IPCMSG *pSendMsg)
{
    DBG_IND("begin %c %d\r\n", pSendMsg->Drive, pSendMsg->CmdId);
    if(INVALID_MSGID == g_MsgId)
    {
        DBG_ERR("g_MsgId is invalid\r\n");
        return -1;
    }

#ifdef FSLINUX_SIM
    g_MsgId = SIM_MSGID_TESTER;
#endif

    if(NvtIPC_MsgSnd(g_MsgId, FSLINUX_SEND_TARGET, pSendMsg, sizeof(FSLINUX_IPCMSG)) < 0)
    {
        DBG_ERR("NvtIPC_MsgSnd(%d) failed\r\n", pSendMsg->CmdId);
        return -1;
    }
    DBG_IND("end %c %d\r\n", pSendMsg->Drive,  pSendMsg->CmdId);
    return 0;
}
#endif