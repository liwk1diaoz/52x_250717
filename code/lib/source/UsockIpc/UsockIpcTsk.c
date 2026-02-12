#include "UsockIpc/UsockIpcAPI.h"

#if (USE_IPC)
#include <stdio.h>
#include "UsockIpcInt.h"
#include "UsockIpcID.h"
#include "UsockIpcMsg.h"
#include "UsockIpcTsk.h"

void UsockIpc_Tsk(USOCKIPC_ID id);

//Note: the array order should be the same as the enum of the command
//Ex: USOCKET_CMDID_TEST = 2, so it should be put to the position of gCmdTbl[2], which is the 3rd item in the table
const USOCKET_CMDID_SET gCmdTbl[USOCKET_CMDID_MAX] = {
	{USOCKET_CMDID_GET_VER_INFO, NULL},
	{USOCKET_CMDID_GET_BUILD_DATE, NULL},
	{USOCKET_CMDID_TEST, NULL},
	{USOCKET_CMDID_OPEN, NULL},
	{USOCKET_CMDID_CLOSE, NULL},
	{USOCKET_CMDID_SEND, NULL},
	{USOCKET_CMDID_RCV, UsockIpc_CmdId_Receive},
	{USOCKET_CMDID_NOTIFY, UsockIpc_CmdId_Notify},
	{USOCKET_CMDID_UDP_OPEN, NULL},
	{USOCKET_CMDID_UDP_CLOSE, NULL},
	{USOCKET_CMDID_UDP_SEND, NULL},
	{USOCKET_CMDID_UDP_RCV, UsockIpc_CmdId_Udp_Receive},
	{USOCKET_CMDID_UDP_NOTIFY, UsockIpc_CmdId_Udp_Notify},
	{USOCKET_CMDID_SYSREQ_ACK, NULL},
};

void UsockIpc_Tsk(USOCKIPC_ID id)
{
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);
	USOCKET_MSG RcvMsg = {0};
	INT32 Ret_IpcResult;
	ER Ret_IpcMsg;

	kent_tsk();//kent_tsk() should be the first function to be called in a task function
	DBG_FUNC_BEGIN("[TSK]\r\n");

	set_flg(p_ctrl->flag_id, USOCKIPC_FLGBIT_TSK_READY);

	while (1) {
		//1. receive ipc cmd from eCos
		Ret_IpcMsg = UsockIpc_Msg_WaitCmd(id,&RcvMsg);
		if (E_OK != Ret_IpcMsg) {
			DBG_ERR("%d UsockIpc_Msg_WaitCmd = %d\r\n",id, Ret_IpcMsg);
			break;
		}

		//2. error check
		DBG_IND("[TSK]%d Got RcvCmdId = %d\r\n",id,RcvMsg.CmdId);
		if (RcvMsg.CmdId >= USOCKET_CMDID_MAX) {
			DBG_ERR("%d RcvCmdId(%d) should be 0~%d\r\n",id, RcvMsg.CmdId, USOCKET_CMDID_MAX);
			break;
		}

		if (gCmdTbl[RcvMsg.CmdId].CmdId != RcvMsg.CmdId) {
			DBG_ERR("%d RcvCmdId = %d, TableCmdId = %d\r\n",id, RcvMsg.CmdId, gCmdTbl[RcvMsg.CmdId].CmdId);
			break;
		}

		//3. process the cmd
		if (USOCKET_CMDID_SYSREQ_ACK == RcvMsg.CmdId) {
			//an ack of the system call request
			DBG_IND("[TSK]%d Got API Arg %d\r\n",id, RcvMsg.Arg);
			UsockIpc_Msg_SetAck(id,RcvMsg.Arg);
		} else {
			//call the corresponding function
			if (gCmdTbl[RcvMsg.CmdId].pFunc) {
				Ret_IpcResult = gCmdTbl[RcvMsg.CmdId].pFunc(id);
				DBG_IND("[TSK]%d Ret_IpcResult = 0x%X\r\n",id, Ret_IpcResult);
			} else {
				Ret_IpcResult = USOCKET_RET_NO_FUNC;
				DBG_WRN("[TSK%d ]Null Cmd pFunc\r\n",id);
			}

			//send the msg of the return value of FileSys API
			Ret_IpcMsg = UsockIpc_Msg_AckCmd(id,Ret_IpcResult);
			if (E_OK != Ret_IpcMsg) {
				DBG_ERR("%d UsockIpc_Msg_WaitCmd = %d\r\n",id, Ret_IpcMsg);
				break;
			}
		}
	}

	DBG_FUNC_END("[TSK]\r\n");
	ext_tsk();
}

void UsockIpc_Tsk_0(void)
{
    UsockIpc_Tsk(USOCKIPC_ID_0);
}
void UsockIpc_Tsk_1(void)
{
    UsockIpc_Tsk(USOCKIPC_ID_1);
}
#endif
