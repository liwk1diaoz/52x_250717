#include <stdio.h>

#include "EthCam/EthsockIpcAPI.h"

#include "EthsockIpcInt.h"
#include "EthsockIpcID.h"
#include "EthsockIpcMsg.h"
#include "EthsockIpcTsk.h"

THREAD_RETTYPE EthsockIpc_Tsk(ETHSOCKIPC_ID id);

//Note: the array order should be the same as the enum of the command
//Ex: ETHSOCKET_CMDID_TEST = 2, so it should be put to the position of gEthSocketCmdTbl[2], which is the 3rd item in the table
const ETHSOCKET_CMDID_SET gEthSocketCmdTbl[ETHSOCKET_CMDID_MAX] = {
	{ETHSOCKET_CMDID_GET_VER_INFO, NULL},
	{ETHSOCKET_CMDID_GET_BUILD_DATE, NULL},
	{ETHSOCKET_CMDID_TEST, NULL},
	{ETHSOCKET_CMDID_OPEN, NULL},
	{ETHSOCKET_CMDID_CLOSE, NULL},
	{ETHSOCKET_CMDID_SEND, NULL},
	{ETHSOCKET_CMDID_RCV, EthsockIpc_CmdId_Receive},
	{ETHSOCKET_CMDID_NOTIFY, EthsockIpc_CmdId_Notify},
	{ETHSOCKET_CMDID_UDP_OPEN, NULL},
	{ETHSOCKET_CMDID_UDP_CLOSE, NULL},
	{ETHSOCKET_CMDID_UDP_SEND, NULL},
	//{ETHSOCKET_CMDID_UDP_RCV, EthsockIpc_CmdId_Udp_Receive},
	//{ETHSOCKET_CMDID_UDP_NOTIFY, EthsockIpc_CmdId_Udp_Notify},
	{ETHSOCKET_CMDID_SYSREQ_ACK, NULL},
};

THREAD_RETTYPE EthsockIpc_Tsk(ETHSOCKIPC_ID id)
{
	THREAD_ENTRY();
#if (USE_IPC)

	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);
	ETHSOCKET_MSG RcvMsg = {0};
	INT32 Ret_IpcResult;
	ER Ret_IpcMsg;

	kent_tsk();//kent_tsk() should be the first function to be called in a task function
	DBG_FUNC_BEGIN("[TSK]\r\n");

	set_flg(p_ctrl->flag_id, ETHSOCKIPC_FLGBIT_TSK_READY);

	while (1) {
		//1. receive ipc cmd from eCos
		Ret_IpcMsg = EthsockIpc_Msg_WaitCmd(id, &RcvMsg);
		if (E_OK != Ret_IpcMsg) {
			DBG_ERR("%d EthsockIpc_Msg_WaitCmd = %d\r\n", id, Ret_IpcMsg);
			break;
		}

		//2. error check
		DBG_IND("[TSK]%d Got RcvCmdId = %d\r\n", id, RcvMsg.CmdId);
		if (RcvMsg.CmdId >= ETHSOCKET_CMDID_MAX) {
			DBG_ERR("%d RcvCmdId(%d) should be 0~%d\r\n", id, RcvMsg.CmdId, ETHSOCKET_CMDID_MAX);
			break;
		}

		if (gEthSocketCmdTbl[RcvMsg.CmdId].CmdId != RcvMsg.CmdId) {
			DBG_ERR("%d RcvCmdId = %d, TableCmdId = %d\r\n", id, RcvMsg.CmdId, gEthSocketCmdTbl[RcvMsg.CmdId].CmdId);
			break;
		}

		//3. process the cmd
		if (ETHSOCKET_CMDID_SYSREQ_ACK == RcvMsg.CmdId) {
			//an ack of the system call request
			DBG_IND("[TSK]%d Got API Arg %d\r\n", id, RcvMsg.Arg);
			EthsockIpc_Msg_SetAck(id, RcvMsg.Arg);
		} else {
			//call the corresponding function
			if (gEthSocketCmdTbl[RcvMsg.CmdId].pFunc) {
				Ret_IpcResult = gEthSocketCmdTbl[RcvMsg.CmdId].pFunc(id);
				DBG_IND("[TSK]%d Ret_IpcResult = 0x%X\r\n", id, Ret_IpcResult);
			} else {
				Ret_IpcResult = ETHSOCKET_RET_NO_FUNC;
				DBG_WRN("[TSK%d ]Null Cmd pFunc\r\n", id);
			}

			//send the msg of the return value of FileSys API
			Ret_IpcMsg = EthsockIpc_Msg_AckCmd(id, Ret_IpcResult);
			if (E_OK != Ret_IpcMsg) {
				DBG_ERR("%d EthsockIpc_Msg_WaitCmd = %d\r\n", id, Ret_IpcMsg);
				break;
			}
		}
	}

	DBG_FUNC_END("[TSK]\r\n");
#endif
	THREAD_RETURN(0);
}

void EthsockIpc_Tsk_0(void)
{
	EthsockIpc_Tsk(ETHSOCKIPC_ID_0);
}
void EthsockIpc_Tsk_1(void)
{
	EthsockIpc_Tsk(ETHSOCKIPC_ID_1);
}
void EthsockIpc_Tsk_2(void)
{
	EthsockIpc_Tsk(ETHSOCKIPC_ID_2);
}
void EthsockIpc_HandleCmd(ETHSOCKET_CMDID CmdId, unsigned int id)
{
#if(USE_IPC==0)
	if (gEthSocketCmdTbl[CmdId].pFunc) {
		gEthSocketCmdTbl[CmdId].pFunc( id);
	}
#endif
}

