#include <stdio.h>
#include "EthsockCliIpcInt.h"
#include "EthsockCliIpcID.h"
#include "EthsockCliIpcMsg.h"
#include "EthsockCliIpcTsk.h"
#include "EthCam/EthsockCliIpcAPI.h"
#include "EthCam/EthCamSocket.h"

typedef INT32 (*ETHSOCKETCLI_CMDID_FP)(UINT32 path_id, ETHSOCKIPCCLI_ID id);

typedef struct{
	ETHSOCKETCLI_CMDID CmdId;
	ETHSOCKETCLI_CMDID_FP pFunc;
} ETHSOCKETCLI_CMDID_SET;

#define ETHSOCKCLIIPC_CMD_FP_MAX        (sizeof(gCmdTbl) / sizeof(ETHSOCKETCLI_CMDID_SET))

//Note: the array order should be the same as the enum of the command
//Ex: ETHSOCKETCLI_CMDID_TEST = 2, so it should be put to the position of gCmdTbl[2], which is the 3rd item in the table
static const ETHSOCKETCLI_CMDID_SET gCmdTbl[]={
	{ETHSOCKETCLI_CMDID_GET_VER_INFO,       NULL                            },
	{ETHSOCKETCLI_CMDID_GET_BUILD_DATE,     NULL                            },
	{ETHSOCKETCLI_CMDID_START,              NULL                            },
	{ETHSOCKETCLI_CMDID_CONNECT,            NULL                            },
	{ETHSOCKETCLI_CMDID_DISCONNECT,         NULL                            },
	{ETHSOCKETCLI_CMDID_STOP,               NULL                            },
	{ETHSOCKETCLI_CMDID_SEND,               NULL                            },
	{ETHSOCKETCLI_CMDID_RCV,                EthsockCliIpc_CmdId_Receive     },
	{ETHSOCKETCLI_CMDID_NOTIFY,             EthsockCliIpc_CmdId_Notify      },
	{ETHSOCKETCLI_CMDID_SYSREQ_ACK,         NULL                            },
};
THREAD_RETTYPE EthsockCliIpc_Tsk(UINT32 path_id, ETHSOCKIPCCLI_ID id);

THREAD_RETTYPE EthsockCliIpc_Tsk(UINT32 path_id, ETHSOCKIPCCLI_ID id)
{

	THREAD_ENTRY();
#if (USE_IPC)

	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(path_id, id);

	ETHSOCKETCLI_MSG RcvMsg = {0};
	INT32 Ret_IpcResult;
	ER Ret_IpcMsg;

	DBG_FUNC_BEGIN("[TSK]\r\n");

	//set_flg(ETHSOCKCLIIPC_FLG_ID_0, ETHSOCKCLIIPC_FLGBIT_TSK_READY);
	set_flg(p_ctrl->flag_id, ETHSOCKCLIIPC_FLGBIT_TSK_READY);

	while (1) {
		//1. receive ipc cmd from eCos
		Ret_IpcMsg = EthsockCliIpc_Msg_WaitCmd(path_id, id, &RcvMsg);
		if (E_OK != Ret_IpcMsg) {
			DBG_ERR("EthsockCliIpc_Msg_WaitCmd = %d\r\n", Ret_IpcMsg);
			break;
		}

		//2. error check
		DBG_IND("%s(): [TSK]%d %d Got RcvCmdId = %d\r\n", __func__, path_id, id, RcvMsg.CmdId);
		if (RcvMsg.CmdId >= ETHSOCKCLIIPC_CMD_FP_MAX) {
			DBG_ERR("%d %d RcvCmdId(%d) should be 0~%d\r\n",path_id, id, RcvMsg.CmdId, ETHSOCKCLIIPC_CMD_FP_MAX);
			break;
		}

		if (gCmdTbl[RcvMsg.CmdId].CmdId != RcvMsg.CmdId) {
			DBG_ERR("%d %d RcvCmdId = %d, TableCmdId = %d\r\n", path_id, id, RcvMsg.CmdId, gCmdTbl[RcvMsg.CmdId].CmdId);
			break;
		}

		//3. process the cmd
		if (ETHSOCKETCLI_CMDID_SYSREQ_ACK == RcvMsg.CmdId) {    //an ack of the system call request
			DBG_IND("[TSK]%d %d Got API Arg %d\r\n",path_id, id, RcvMsg.Arg);
			EthsockCliIpc_Msg_SetAck(path_id, id, RcvMsg.Arg);
		} else {                                                //call the corresponding function
			if (gCmdTbl[RcvMsg.CmdId].pFunc) {
				Ret_IpcResult = gCmdTbl[RcvMsg.CmdId].pFunc(path_id, id);
				DBG_IND("[TSK]%d %d Ret_IpcResult = 0x%X\r\n", path_id, id, Ret_IpcResult);
			} else {
				Ret_IpcResult = ETHSOCKETCLI_RET_NO_FUNC;
				DBG_WRN("[TSK %d %d]Null Cmd pFunc\r\n", path_id, id);
			}

			//send the msg of the return value of FileSys API
			Ret_IpcMsg = EthsockCliIpc_Msg_AckCmd(path_id, id, Ret_IpcResult);
			if (E_OK != Ret_IpcMsg) {
				DBG_ERR("%d %dEthsockCliIpc_Msg_WaitCmd = %d\r\n",path_id, id, Ret_IpcMsg);
				break;
			}
		}
	}

	DBG_FUNC_END("[TSK]\r\n");
#endif
	THREAD_RETURN(0);
}
void EthsockCliIpc_Tsk_Path0_0(void)
{
	EthsockCliIpc_Tsk(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_0);
}
void EthsockCliIpc_Tsk_Path0_1(void)
{
	EthsockCliIpc_Tsk(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_1);
}
void EthsockCliIpc_Tsk_Path0_2(void)
{
	EthsockCliIpc_Tsk(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_2);
}
void EthsockCliIpc_Tsk_Path1_0(void)
{
	EthsockCliIpc_Tsk(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_0);
}
void EthsockCliIpc_Tsk_Path1_1(void)
{
	EthsockCliIpc_Tsk(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_1);
}
void EthsockCliIpc_Tsk_Path1_2(void)
{
	EthsockCliIpc_Tsk(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_2);
}
void EthsockCliIpc_HandleCmd(ETHSOCKETCLI_CMDID CmdId, unsigned int path_id, unsigned int id)
{
#if(USE_IPC==0)
	if (gCmdTbl[CmdId].pFunc) {
		gCmdTbl[CmdId].pFunc(path_id, id);
	}
#endif
}
