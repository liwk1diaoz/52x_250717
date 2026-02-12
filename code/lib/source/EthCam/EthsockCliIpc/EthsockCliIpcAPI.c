#include <stdio.h>
#include <string.h>
#include "EthCam/EthsockCliIpcAPI.h"
#include "EthsockCliIpcInt.h"
#include "EthsockCliIpcID.h"
#include "EthsockCliIpcMsg.h"
#include "EthsockCliIpcTsk.h"
#include "EthCam/EthCamSocket.h"

#if (USE_IPC)
#include "dma_protected.h"
#include "NvtIpcAPI.h"
#endif

#if (USE_IPC)
#define ipc_getPhyAddr(addr)        NvtIPC_GetPhyAddr(addr)
#else
#define ipc_getPhyAddr(addr)        addr
#endif

#define ETHSOCKETCLI_VER_KEY              20190506

ER EthsockCliIpc_Test(int param1, int param2, int param3);

#define ETHSOCKCLIIPC_INVALID_PARAMADDR   (0)

static ETHSOCKIPCCLI_CTRL EthsockCliObj[MAX_ETH_PATH_NUM][MAX_ETHSOCKETCLI_NUM] = {0};
//static BOOL bIsOpened = FALSE;
static UINT32 gParamSndAddr[MAX_ETH_PATH_NUM][MAX_ETHSOCKETCLI_NUM] ;//parameter address
static UINT32 gParamSndSize[MAX_ETH_PATH_NUM][MAX_ETHSOCKETCLI_NUM] ;//parameter size
static UINT32 gParamRcvAddr[MAX_ETH_PATH_NUM][MAX_ETHSOCKETCLI_NUM] ;//parameter address
static UINT32 gParamRcvSize[MAX_ETH_PATH_NUM][MAX_ETHSOCKETCLI_NUM] ;//parameter size
static ethsocket_cli_recv *pRecvCB[MAX_ETH_PATH_NUM][MAX_ETHSOCKETCLI_NUM]  = {0};
static ethsocket_cli_notify *pNotifyCB[MAX_ETH_PATH_NUM][MAX_ETHSOCKETCLI_NUM]  = {0};
static char ProcIdTbl[MAX_ETH_PATH_NUM][MAX_ETHSOCKETCLI_NUM][20]={{ETHSOCKETCLI_TOKEN_scoket0_p0, ETHSOCKETCLI_TOKEN_scoket1_p0, ETHSOCKETCLI_TOKEN_scoket2_p0},{ETHSOCKETCLI_TOKEN_scoket0_p1, ETHSOCKETCLI_TOKEN_scoket1_p1, ETHSOCKETCLI_TOKEN_scoket2_p1}};

ETHSOCKIPCCLI_CTRL *EthsockCliIpc_GetCtrl(UINT32  path_id, ETHSOCKIPCCLI_ID id)
{
	return &EthsockCliObj[path_id][id];
}

static void EthsockCliIpc_lock(UINT32 path_id, ETHSOCKIPCCLI_ID id)
{
	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(path_id, id);
	//wai_sem(ETHSOCKCLIIPC_SEM_ID);
	SEM_WAIT(p_ctrl->sem_id);
}

static void EthsockCliIpc_unlock(UINT32  path_id, ETHSOCKIPCCLI_ID id)
{
	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(path_id, id);
	//sig_sem(ETHSOCKCLIIPC_SEM_ID);
	SEM_SIGNAL(p_ctrl->sem_id);
}

static UINT32 EthsockCliIpc_getSndAddr(UINT32  path_id, ETHSOCKIPCCLI_ID id)
{
	if (*(UINT32 *)(gParamSndAddr[path_id][id]) == ETHSOCKETCLIIPC_BUF_TAG) {
		//DBG_IND("%x %x \r\n",gParamSndAddr,*(UINT32 *)(gParamSndAddr));
		return (gParamSndAddr[path_id][id] + ETHSOCKETCLIIPC_BUF_CHK_SIZE);
    } else {
		DBG_ERR("%d %d buf chk fail %x  %x\r\n", path_id, id, gParamRcvAddr[path_id][id], *(UINT32 *)(gParamRcvAddr[path_id][id]));
		return ETHSOCKCLIIPC_INVALID_PARAMADDR;
	}
}

static UINT32 EthsockCliIpc_getRcvAddr(UINT32  path_id, ETHSOCKIPCCLI_ID id)
{
    if (*(UINT32 *)(gParamRcvAddr[path_id][id]) == ETHSOCKETCLIIPC_BUF_TAG) {
		//DBG_IND("%x %x \r\n",gParamRcvAddr,*(UINT32 *)(gParamRcvAddr));
		return (gParamRcvAddr[path_id][id] + ETHSOCKETCLIIPC_BUF_CHK_SIZE);
	} else {
		DBG_ERR("%d %d buf chk fail %x  %x\r\n", path_id ,id, gParamRcvAddr[path_id][id], *(UINT32 *)(gParamRcvAddr[path_id][id]));
		return ETHSOCKCLIIPC_INVALID_PARAMADDR;
	}
}

ER EthsockCliIpc_Open(UINT32  path_id, ETHSOCKIPCCLI_ID id, ETHSOCKCLIIPC_OPEN *pOpen)
{
	//AR szCmd[50] = {0};
	CHAR szCmd[128] = {0};
#if (USE_IPC)
	FLGPTN FlgPtn;
#endif
	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(path_id, id);
 	ETHCAM_SOCKET_BUF_OBJ BsBufObj={0};
	ETHCAM_SOCKET_BUF_OBJ RawEncodeBufObj={0};
	UINT32 BsQueueMax={0};

	DBG_FUNC_BEGIN("[API]\r\n");
	if (!pOpen->sharedSendMemAddr) {
		DBG_ERR("sharedSendMemAddr[%d][%d] is NULL\r\n",path_id, id);
		return E_NOMEM;
	}
	if (!pOpen->sharedRecvMemAddr) {
		DBG_ERR("sharedRecvMemAddr[%d][%d] is NULL\r\n",path_id, id);
		return E_NOMEM;
	}
#if defined __UITRON || defined __ECOS
	if (dma_isCacheAddr(pOpen->sharedSendMemAddr)) {
		DBG_ERR("sharedSendMemAddr 0x%x should be non-cache Address\r\n",(int)pOpen->sharedSendMemAddr);
		return E_PAR;
	}
	if (dma_isCacheAddr(pOpen->sharedRecvMemAddr)) {
		DBG_ERR("sharedRecvMemAddr 0x%x should be non-cache Address\r\n",(int)pOpen->sharedRecvMemAddr);
		return E_PAR;
	}
#endif
	if (pOpen->sharedSendMemSize < ETHSOCKCLIIPC_PARAM_BUF_SIZE/2) {
		DBG_ERR("sharedSendMemSize 0x%x < need 0x%x\r\n", pOpen->sharedSendMemSize,ETHSOCKCLIIPC_PARAM_BUF_SIZE/2);
		return E_PAR;
	}
	if (pOpen->sharedRecvMemSize < ETHSOCKCLIIPC_PARAM_BUF_SIZE/2) {
		DBG_ERR("sharedRecvMemSize 0x%x < need 0x%x\r\n", pOpen->sharedRecvMemSize,ETHSOCKCLIIPC_PARAM_BUF_SIZE/2);
		return E_PAR;
	}

	switch (id) {
	case ETHSOCKIPCCLI_ID_0:
		p_ctrl->task_id = ETHSOCKCLIIPC_TSK_ID_0[path_id];
		p_ctrl->sem_id = ETHSOCKCLIIPC_SEM_ID_0[path_id];
		p_ctrl->flag_id = ETHSOCKCLIIPC_FLG_ID_0[path_id];
		p_ctrl->token_path = ProcIdTbl[path_id][id];//ETHSOCKETCLI_TOKEN_PATH0;
		BsBufObj=EthCamSocketCli_DataGetBsBufObj(path_id, id);
		RawEncodeBufObj=EthCamSocketCli_DataGetRawEncodeBufObj(path_id, id);
		BsQueueMax=EthCamSocketCli_DataGetBsQueueMax(path_id, id);
		break;
#if (MAX_ETHSOCKETCLI_NUM >= 2)
	case ETHSOCKIPCCLI_ID_1:
		p_ctrl->task_id = ETHSOCKCLIIPC_TSK_ID_1[path_id];
		p_ctrl->sem_id = ETHSOCKCLIIPC_SEM_ID_1[path_id];
		p_ctrl->flag_id = ETHSOCKCLIIPC_FLG_ID_1[path_id];
		p_ctrl->token_path = ProcIdTbl[path_id][id];//ETHSOCKETCLI_TOKEN_PATH1;
		BsBufObj=EthCamSocketCli_DataGetBsBufObj(path_id, id);
		BsQueueMax=EthCamSocketCli_DataGetBsQueueMax(path_id, id);
		break;
	case ETHSOCKIPCCLI_ID_2:
		p_ctrl->task_id = ETHSOCKCLIIPC_TSK_ID_2[path_id];
		p_ctrl->sem_id = ETHSOCKCLIIPC_SEM_ID_2[path_id];
		p_ctrl->flag_id = ETHSOCKCLIIPC_FLG_ID_2[path_id];
		p_ctrl->token_path = ProcIdTbl[path_id][id];//ETHSOCKETCLI_TOKEN_PATH2;
		break;
#endif
	default:
		DBG_ERR("not sup path_id=%d, id=%d\r\n",path_id,  id);
		return E_PAR;
	}

	//error check
	if (!p_ctrl->sem_id) {
		DBG_ERR("path_id=%d, ID %d not installed\r\n",path_id,id);
		return E_SYS;
	}
    //----------lock----------
    EthsockCliIpc_lock(path_id, id);
	//if (bIsOpened) {
	if (p_ctrl->b_opened) {
		DBG_WRN("%d ,%d is opened\r\n",path_id,id);
		EthsockCliIpc_unlock(path_id, id);
		return E_OK;
	}

	if (E_OK != EthsockCliIpc_Msg_Init(path_id, id)) {
		DBG_ERR("%d EthsockCliIpc_Msg_Init ERR\r\n",id);
		EthsockCliIpc_unlock(path_id, id);
		return E_SYS;
	}

	gParamSndAddr[path_id][id] = pOpen->sharedSendMemAddr;
	*(UINT32 *)gParamSndAddr[path_id][id] = ETHSOCKETCLIIPC_BUF_TAG;
	gParamSndSize[path_id][id] = pOpen->sharedSendMemSize;
	//gParamRcvAddr = pOpen->sharedMemAddr + (ETHSOCKCLIIPC_PARAM_BUF_SIZE / 2) ;
	gParamRcvAddr[path_id][id] = pOpen->sharedRecvMemAddr ;
	*(UINT32 *)gParamRcvAddr[path_id][id] = ETHSOCKETCLIIPC_BUF_TAG;
	gParamRcvSize[path_id][id] = pOpen->sharedRecvMemSize;

	//DBG_DUMP("id=%d, gParamSndAddr=0x%x, gParamRcvAddr=0x%x\r\n",id, gParamSndAddr[id],gParamRcvAddr[id] );
	//DBGH(BsBufObj.ParamAddr);
	//DBGH(BsBufObj.ParamSize);
	//DBGH(RawEncodeBufObj.ParamAddr);
	//DBGH(RawEncodeBufObj.ParamSize);


	//DBG_ERR("path_id=%d ,socket_id=%d open\r\n",path_id,id);
#if (USE_IPC)

	//create the receive task and wait it ready
	clr_flg(p_ctrl->flag_id, ETHSOCKCLIIPC_FLGBIT_TSK_READY);
	//sta_tsk(p_ctrl->task_id, 0);
	vos_task_resume(p_ctrl->task_id);

	wai_flg(&FlgPtn, p_ctrl->flag_id, ETHSOCKCLIIPC_FLGBIT_TSK_READY, TWF_ORW);
#endif
	//send syscall request ipc to ecos to open and init ethsocket
	snprintf(szCmd, sizeof(szCmd), "%s -open %d %d %d %d %d %d %d %d %d %d %d %d &", "ethsockcliipc", (unsigned int)ipc_getPhyAddr(gParamSndAddr[path_id][id]), (unsigned int)gParamSndSize[path_id][id], (unsigned int)ipc_getPhyAddr(gParamRcvAddr[path_id][id]),  (unsigned int)gParamRcvSize[path_id][id], (unsigned int)ipc_getPhyAddr(BsBufObj.ParamAddr),BsBufObj.ParamSize,(unsigned int)ipc_getPhyAddr(RawEncodeBufObj.ParamAddr),RawEncodeBufObj.ParamSize, (unsigned int)BsQueueMax, ETHSOCKETCLI_VER_KEY ,(unsigned int)path_id, id);
	//DBG_DUMP("szCmd=%s\r\n",szCmd);
	if (E_OK != EthsockCliIpc_Msg_SysCallReq(path_id, id, szCmd)) {
		DBG_ERR("[%d][%d]EthsockCliIpc_Msg_SysCallReq(%s) ERR\n", path_id,id,szCmd);
		if (E_OK != EthsockCliIpc_Msg_Uninit(path_id, id)) {
			DBG_ERR("EthsockCliIpc_Msg_Uninit ERR\r\n");
			EthsockCliIpc_unlock(path_id, id);
			return E_SYS;
		}
		EthsockCliIpc_unlock(path_id, id);
		return E_SYS;
	}

	if (ETHSOCKETCLI_RET_OK != EthsockCliIpc_Msg_WaitAck(path_id, id)) {
		DBG_ERR("[%d][%d] Open failed.\r\n",path_id, id);
		if (E_OK != EthsockCliIpc_Msg_Uninit(path_id, id)) {
			DBG_ERR("EthsockCliIpc_Msg_Uninit ERR\r\n");
			EthsockCliIpc_unlock(path_id, id);
			return E_SYS;
		}
		EthsockCliIpc_unlock(path_id, id);
		return E_SYS;
	}

	p_ctrl->b_opened = TRUE;

	EthsockCliIpc_unlock(path_id ,id);
    //----------unlock----------

	DBG_FUNC_END("[API]\r\n");
	return E_OK;
}

ER EthsockCliIpc_Close(UINT32  path_id, ETHSOCKIPCCLI_ID id)
{
	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(path_id, id);
	int iRet_EthsockApi = 0;
	DBG_FUNC_BEGIN("[API]\r\n");

	//----------lock----------
	EthsockCliIpc_lock(path_id, id);

	//if (FALSE == bIsOpened) {
	if (FALSE == p_ctrl->b_opened) {
		DBG_WRN("%d is closed\r\n",id);
		EthsockCliIpc_unlock(path_id, id);
		return E_OK;
	}

	if (E_OK != EthsockCliIpc_Msg_SendCmd(path_id, id,ETHSOCKETCLI_CMDID_UNINIT, &iRet_EthsockApi) || ETHSOCKETCLI_RET_OK != iRet_EthsockApi) {
		DBG_ERR("%d EthsockCliIpc_Msg_SendCmd(ETHSOCKETCLI_CMDID_UNINIT) ERR\r\n",id);
		EthsockCliIpc_unlock(path_id, id);
		return E_SYS;
	}

#if (USE_IPC)
	//ter_tsk(ETHSOCKCLIIPC_TSK_ID);
	//ter_tsk(p_ctrl->task_id);
	vos_task_destroy(p_ctrl->task_id);
#endif
	if (E_OK != EthsockCliIpc_Msg_Uninit(path_id, id)) {
		DBG_ERR("EthsockCliIpc_Msg_Uninit ERR\r\n");
		EthsockCliIpc_unlock(path_id, id);
		return E_SYS;
	}

	//bIsOpened = FALSE;
	p_ctrl->b_opened = FALSE;
	EthsockCliIpc_unlock(path_id, id);
	//----------unlock----------

	DBG_FUNC_END("[API]\r\n");
	return E_OK;
}

ER EthsockCliIpc_Start(UINT32  path_id, ETHSOCKIPCCLI_ID id)
{
	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(path_id, id);
	int iRet_EthsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return ETHSOCKETCLI_RET_NOT_OPEN;
	}

	EthsockCliIpc_lock(path_id, id);
	result = EthsockCliIpc_Msg_SendCmd(path_id, id, ETHSOCKETCLI_CMDID_START, &iRet_EthsockApi);
	EthsockCliIpc_unlock(path_id, id);

	if (result == E_OK) {
		return iRet_EthsockApi;
	} else {
		return ETHSOCKETCLI_RET_ERR;
	}
}

INT32 EthsockCliIpc_Connect(UINT32  path_id, ETHSOCKIPCCLI_ID id, ethsocketcli_install_obj*  pObj)
{
	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(path_id, id);
	//ETHSOCKETCLI_PARAM_PARAM *EthsockApiParam = 0;
	ethsocketcli_install_obj *EthsockApiParam = 0;
	int iRet_EthsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return 0;
	}
	if (!pObj) {
		return 0;
	}

	EthsockCliIpc_lock(path_id, id);
	EthsockApiParam = (ethsocketcli_install_obj *)EthsockCliIpc_getSndAddr(path_id, id);
	memcpy(EthsockApiParam, pObj, sizeof(ethsocketcli_install_obj));
	if (pObj->notify) {
		pNotifyCB[path_id][id] = pObj->notify;
	}
	if (pObj->recv) {
		pRecvCB[path_id][id] = pObj->recv;
	}
	//ethsocket_cli_get_newobj
	//ethsocket_cli_install
	//ethsocket_cli_connect
	result = EthsockCliIpc_Msg_SendCmd(path_id, id, ETHSOCKETCLI_CMDID_CONNECT, &iRet_EthsockApi);
	EthsockCliIpc_unlock(path_id, id);

	if (result == E_OK) {
		return (INT32)iRet_EthsockApi;
	} else {
		return 0;
	}
}

ER EthsockCliIpc_Send(UINT32  path_id, ETHSOCKIPCCLI_ID id, INT32 handle, char* addr, int* size)
{
	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(path_id, id);
	ETHSOCKETCLI_TRANSFER_PARAM *EthsockApiParam = 0;
	int iRet_EthsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return ETHSOCKET_RET_NOT_OPEN;
	}

	if (!handle) {
		DBG_ERR("no handle %d\r\n",handle);
		return ETHSOCKETCLI_RET_ERR_PAR;
	}

	if (*size > ETHSOCKCLIIPC_TRANSFER_BUF_SIZE) {
		DBG_ERR("size %d > IPC buffer %d\r\n", *size, ETHSOCKCLIIPC_TRANSFER_BUF_SIZE);
		return ETHSOCKETCLI_RET_ERR;
	}

	EthsockCliIpc_lock(path_id, id);
	EthsockApiParam = (ETHSOCKETCLI_TRANSFER_PARAM *)EthsockCliIpc_getSndAddr(path_id, id);
	//EthsockApiParam->obj = handle;
	EthsockApiParam->size = *size;
	memcpy(EthsockApiParam->buf, addr, *size);
	result = EthsockCliIpc_Msg_SendCmd(path_id, id, ETHSOCKETCLI_CMDID_SEND, &iRet_EthsockApi);
	*size = EthsockApiParam->size;
	EthsockCliIpc_unlock(path_id, id);

	if (result == E_OK) {
		return iRet_EthsockApi;
	} else {
		return ETHSOCKETCLI_RET_ERR;
	}
}

ER EthsockCliIpc_Disconnect(UINT32  path_id, ETHSOCKIPCCLI_ID id, INT32 *handle)
{
	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(path_id, id);
	ETHSOCKETCLI_PARAM_PARAM *EthsockApiParam = 0;
	int iRet_EthsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return ETHSOCKET_RET_NOT_OPEN;
	}

	if (!handle) {
		return ETHSOCKETCLI_RET_ERR_PAR;
	}

	EthsockCliIpc_lock(path_id, id);
	EthsockApiParam = (ETHSOCKETCLI_PARAM_PARAM *)EthsockCliIpc_getSndAddr(path_id, id);
	EthsockApiParam->param1 = *handle;
	result = EthsockCliIpc_Msg_SendCmd(path_id, id, ETHSOCKETCLI_CMDID_DISCONNECT, &iRet_EthsockApi);
	*handle = 0;
	pNotifyCB[path_id][id] = 0;
	pRecvCB[path_id][id] = 0;

	EthsockCliIpc_unlock(path_id, id);

	if (result == E_OK) {
		return iRet_EthsockApi;
	} else {
		return ETHSOCKETCLI_RET_ERR;
	}
}

ER EthsockCliIpc_Stop(UINT32  path_id, ETHSOCKIPCCLI_ID id)
{
	ETHSOCKIPCCLI_CTRL *p_ctrl = EthsockCliIpc_GetCtrl(path_id, id);
	//ETHSOCKETCLI_PARAM_PARAM *EthsockApiParam = 0;
	int iRet_EthsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return ETHSOCKETCLI_RET_NOT_OPEN;
	}
	EthsockCliIpc_lock(path_id, id);
	//EthsockApiParam = (ETHSOCKETCLI_PARAM_PARAM *)EthsockCliIpc_getSndAddr(id);
	//EthsockApiParam->param1 = handle;
	result = EthsockCliIpc_Msg_SendCmd(path_id, id, ETHSOCKETCLI_CMDID_STOP, &iRet_EthsockApi);
	EthsockCliIpc_unlock(path_id, id);

	if (result == E_OK) {
		return iRet_EthsockApi;
	} else {
		return ETHSOCKETCLI_RET_ERR;
	}
}

INT32 EthsockCliIpc_CmdId_Notify(UINT32  path_id, ETHSOCKIPCCLI_ID id)
{
	ETHSOCKETCLI_PARAM_PARAM *EthsockApiParam = (ETHSOCKETCLI_PARAM_PARAM *)EthsockCliIpc_getRcvAddr(path_id, id);

	if(EthsockApiParam ==NULL){
		DBG_ERR("param null[%d]\r\n",id);
		return ETHSOCKETCLI_RET_ERR;
	}

	if (pNotifyCB[path_id][id]) {
		//pNotifyCB[id](EthsockApiParam->param2, EthsockApiParam->param3);
		pNotifyCB[path_id][id](path_id, EthsockApiParam->param1, EthsockApiParam->param2);
		return ETHSOCKETCLI_RET_OK;
	} else {
		DBG_ERR("no pRecvCB[%d]\r\n",id);
		return ETHSOCKETCLI_RET_ERR;
	}
}

INT32 EthsockCliIpc_CmdId_Receive(UINT32  path_id, ETHSOCKIPCCLI_ID id)
{
	ETHSOCKETCLI_RECV_PARAM * EthsockApiRecvParam;
	ETHSOCKETCLI_TRANSFER_PARAM * EthsockApiTransParam;
	if(id== ETHSOCKIPCCLI_ID_0 || id== ETHSOCKIPCCLI_ID_1){
		EthsockApiRecvParam = (ETHSOCKETCLI_RECV_PARAM *)EthsockCliIpc_getRcvAddr(path_id, id);
		if (pRecvCB[path_id][id]) {
			pRecvCB[path_id][id](path_id, EthsockApiRecvParam->buf, EthsockApiRecvParam->size);
			return ETHSOCKETCLI_RET_OK;
		} else {
			DBG_ERR("no pRecvCB[%d][%d]\r\n",path_id,id);
			return ETHSOCKETCLI_RET_ERR;
		}

	}else{
		EthsockApiTransParam = (ETHSOCKETCLI_TRANSFER_PARAM *)EthsockCliIpc_getRcvAddr(path_id, id);
		if (pRecvCB[path_id][id]) {
			pRecvCB[path_id][id](path_id, EthsockApiTransParam->buf, EthsockApiTransParam->size);
			return ETHSOCKETCLI_RET_OK;
		} else {
			DBG_ERR("no pRecvCB[%d][%d]\r\n",path_id,id);
			return ETHSOCKETCLI_RET_ERR;
		}
	}
}

