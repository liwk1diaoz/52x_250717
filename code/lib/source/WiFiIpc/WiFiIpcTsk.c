#include <stdio.h>
#include "WiFiIpcInt.h"
#include "WiFiIpcID.h"
#include "WiFiIpcMsg.h"
#include "WiFiIpcTsk.h"
#include "Debug.h"

#if defined(_NETWORK_ON_CPU2_)

typedef INT32(*NVTWIFI_CMDID_FP)(void);

typedef struct {
	NVTWIFI_CMDID CmdId;
	NVTWIFI_CMDID_FP pFunc;
} NVTWIFI_CMDID_SET;

#define WIFIIPC_CMD_FP_MAX        sizeof(gCmdTbl)/sizeof(NVTWIFI_CMDID_SET)


//Note: the array order should be the same as the enum of the command
//Ex: NVTWIFI_CMDID_OPEN = 2, so it should be put to the position of gCmdTbl[2], which is the 3rd item in the table
static const NVTWIFI_CMDID_SET gCmdTbl[] = {
	{NVTWIFI_CMDID_GET_VER_INFO, NULL},
	{NVTWIFI_CMDID_GET_BUILD_DATE, NULL},
	{NVTWIFI_CMDID_TEST, NULL},
	{NVTWIFI_CMDID_INIT, NULL},
	{NVTWIFI_CMDID_INTF_IS_UP, NULL},
	{NVTWIFI_CMDID_INTF_UP, NULL},
	{NVTWIFI_CMDID_INTF_DOWN, NULL},
	{NVTWIFI_CMDID_INTF_CONFIG, NULL},
	{NVTWIFI_CMDID_INTF_DELETE_ADDR, NULL},
	{NVTWIFI_CMDID_CONFIG, NULL},
	{NVTWIFI_CMDID_GET_MAC, NULL},
	{NVTWIFI_CMDID_SET_MAC, NULL},
	{NVTWIFI_CMDID_RUN_CMD, NULL},
	{NVTWIFI_CMDID_REG_STACB, NULL},
	{NVTWIFI_CMDID_REG_LINKCB, NULL},
	{NVTWIFI_CMDID_REG_WSCCB, NULL},
	{NVTWIFI_CMDID_REG_WSC_FLASHCB, NULL},
	{NVTWIFI_CMDID_REG_P2PCB, NULL},
	{NVTWIFI_CMDID_STA_CB, WiFiIpc_CmdId_StaCB},
	{NVTWIFI_CMDID_LINK_CB, WiFiIpc_CmdId_LinkCB},
	{NVTWIFI_CMDID_WSCCB, WiFiIpc_CmdId_WSCCB},
	{NVTWIFI_CMDID_WSC_FLASHCB, WiFiIpc_CmdId_WSC_FlashCB},
	{NVTWIFI_CMDID_P2PCB, WiFiIpc_CmdId_P2PCB},
	{NVTWIFI_CMDID_SITE_REQ, NULL},
	{NVTWIFI_CMDID_SITE_RET, NULL},
	{NVTWIFI_CMDID_P2P_REQ, NULL},
	{NVTWIFI_CMDID_P2P_RET, NULL},
	{NVTWIFI_CMDID_P2P_PROVISION, NULL},
	{NVTWIFI_CMDID_WSCD_CREATE, NULL},
	{NVTWIFI_CMDID_WSCD_REINIT, NULL},
	{NVTWIFI_CMDID_WSCD_STOP, NULL},
	{NVTWIFI_CMDID_WSCD_STATUS, NULL},
	{NVTWIFI_CMDID_GEN_PIN, NULL},
	{NVTWIFI_CMDID_THREAD_INFO_BY_NAME, NULL},
	{NVTWIFI_CMDID_IGNORE_DOWN_UP, NULL},
	{NVTWIFI_CMDID_SYSREQ_ACK, NULL},
};

void WiFiIpc_Tsk(void)
{
	NVTWIFI_MSG RcvMsg = {0};
	INT32 Ret_IpcResult = NVTWIFI_RET_ERR;
	ER Ret_IpcMsg;

	kent_tsk();//kent_tsk() should be the first function to be called in a task function
	DBG_FUNC_BEGIN("[TSK]\r\n");

	set_flg(WIFIIPC_FLG_ID, WIFIIPC_FLGBIT_TSK_READY);

	while (1) {
		PROFILE_TASK_IDLE();
		//1. receive ipc cmd from eCos
		Ret_IpcMsg = WiFiIpc_Msg_WaitCmd(&RcvMsg);
		if (E_OK != Ret_IpcMsg) {
			DBG_ERR("WiFiIpc_Msg_WaitCmd = %d\r\n", Ret_IpcMsg);
			break;
		}
		PROFILE_TASK_BUSY();

		//2. error check
		DBG_IND("^M%s(): [TSK]Got RcvCmdId = %d\r\n", __func__, RcvMsg.CmdId);
		if (RcvMsg.CmdId >= WIFIIPC_CMD_FP_MAX) {
			DBG_ERR("RcvCmdId(%d) should be 0~%d\r\n", RcvMsg.CmdId, WIFIIPC_CMD_FP_MAX);
			break;
		}

		if (gCmdTbl[RcvMsg.CmdId].CmdId != RcvMsg.CmdId) {
			DBG_ERR("RcvCmdId = %d, TableCmdId = %d\r\n", RcvMsg.CmdId, gCmdTbl[RcvMsg.CmdId].CmdId);
			break;
		}

		//3. process the cmd
		if (NVTWIFI_CMDID_SYSREQ_ACK == RcvMsg.CmdId) {
			//an ack of the system call request
			DBG_IND("[TSK]Got API Arg %d\r\n", RcvMsg.Arg);
			WiFiIpc_Msg_SetAck(RcvMsg.Arg);
		} else {
			//call the corresponding function
			if (gCmdTbl[RcvMsg.CmdId].pFunc) {
				Ret_IpcResult = gCmdTbl[RcvMsg.CmdId].pFunc();
				DBG_IND("[TSK]Ret_IpcResult = 0x%X\r\n", Ret_IpcResult);
			} else {
				Ret_IpcResult = NVTWIFI_RET_ERR;
				DBG_WRN("[TSK]Null Cmd pFunc\r\n");
			}

			//send the msg of the return value of WiFiIpc API
			Ret_IpcMsg = WiFiIpc_Msg_AckCmd(Ret_IpcResult);
			if (E_OK != Ret_IpcMsg) {
				DBG_ERR("WiFiIpc_Msg_WaitCmd = %d\r\n", Ret_IpcMsg);
				break;
			}
		}
	}

	DBG_FUNC_END("[TSK]\r\n");
	ext_tsk();
}

#endif

