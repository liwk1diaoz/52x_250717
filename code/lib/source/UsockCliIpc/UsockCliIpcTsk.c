#include "UsockCliIpc/UsockCliIpcAPI.h"

#if (USE_IPC)
#include <stdio.h>
#include "UsockCliIpcInt.h"
#include "UsockCliIpcID.h"
#include "UsockCliIpcMsg.h"
#include "UsockCliIpcTsk.h"
#include "UsockCliIpcAPI.h"

typedef INT32 (*USOCKETCLI_CMDID_FP)(void);

typedef struct{
    USOCKETCLI_CMDID CmdId;
    USOCKETCLI_CMDID_FP pFunc;
}USOCKETCLI_CMDID_SET;

#define USOCKCLIIPC_CMD_FP_MAX        sizeof(gCmdTbl)/sizeof(USOCKETCLI_CMDID_SET)


//Note: the array order should be the same as the enum of the command
//Ex: USOCKETCLI_CMDID_TEST = 2, so it should be put to the position of gCmdTbl[2], which is the 3rd item in the table
static const USOCKETCLI_CMDID_SET gCmdTbl[]={
{USOCKETCLI_CMDID_GET_VER_INFO, NULL},
{USOCKETCLI_CMDID_GET_BUILD_DATE, NULL},
{USOCKETCLI_CMDID_START, NULL},
{USOCKETCLI_CMDID_CONNECT, NULL},
{USOCKETCLI_CMDID_DISCONNECT, NULL},
{USOCKETCLI_CMDID_STOP, NULL},
{USOCKETCLI_CMDID_SEND, NULL},
{USOCKETCLI_CMDID_RCV, UsockCliIpc_CmdId_Receive},
{USOCKETCLI_CMDID_NOTIFY, UsockCliIpc_CmdId_Notify},
{USOCKETCLI_CMDID_SYSREQ_ACK, NULL},
};

void UsockCliIpc_Tsk(void)
{
    USOCKETCLI_MSG RcvMsg = {0};
    INT32 Ret_IpcResult;
    ER Ret_IpcMsg;

    kent_tsk();//kent_tsk() should be the first function to be called in a task function
    DBG_FUNC_BEGIN("[TSK]\r\n");

    set_flg(USOCKCLIIPC_FLG_ID, USOCKCLIIPC_FLGBIT_TSK_READY);

    while(1)
    {
        //1. receive ipc cmd from eCos
        Ret_IpcMsg = UsockCliIpc_Msg_WaitCmd(&RcvMsg);
        if(E_OK != Ret_IpcMsg)
        {
            DBG_ERR("UsockCliIpc_Msg_WaitCmd = %d\r\n", Ret_IpcMsg);
            break;
        }

        //2. error check
        DBG_IND("%s(): [TSK]Got RcvCmdId = %d\r\n", __func__, RcvMsg.CmdId);
        if(RcvMsg.CmdId >= USOCKCLIIPC_CMD_FP_MAX)
        {
            DBG_ERR("RcvCmdId(%d) should be 0~%d\r\n", RcvMsg.CmdId, USOCKCLIIPC_CMD_FP_MAX);
            break;
        }

        if(gCmdTbl[RcvMsg.CmdId].CmdId != RcvMsg.CmdId)
        {
            DBG_ERR("RcvCmdId = %d, TableCmdId = %d\r\n", RcvMsg.CmdId, gCmdTbl[RcvMsg.CmdId].CmdId);
            break;
        }

        //3. process the cmd
        if(USOCKETCLI_CMDID_SYSREQ_ACK == RcvMsg.CmdId)
        {//an ack of the system call request
            DBG_IND("[TSK]Got API Arg %d\r\n",RcvMsg.Arg);
            UsockCliIpc_Msg_SetAck(RcvMsg.Arg);
        }
        else
        {//call the corresponding function
            if(gCmdTbl[RcvMsg.CmdId].pFunc)
            {
                Ret_IpcResult = gCmdTbl[RcvMsg.CmdId].pFunc();
                DBG_IND("[TSK]Ret_IpcResult = 0x%X\r\n", Ret_IpcResult);
            }
            else
            {
                Ret_IpcResult = USOCKETCLI_RET_NO_FUNC;
                DBG_WRN("[TSK]Null Cmd pFunc\r\n");
            }

            //send the msg of the return value of FileSys API
            Ret_IpcMsg = UsockCliIpc_Msg_AckCmd(Ret_IpcResult);
            if(E_OK != Ret_IpcMsg)
            {
                DBG_ERR("UsockCliIpc_Msg_WaitCmd = %d\r\n", Ret_IpcMsg);
                break;
            }
        }
    }

    DBG_FUNC_END("[TSK]\r\n");
    ext_tsk();
}



#endif
