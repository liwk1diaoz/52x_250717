#include <stdio.h>
#include <string.h>
#include "EthCam/EthsockIpcAPI.h"
#include "EthsockIpcInt.h"
#include "EthsockIpcID.h"
#include "EthsockIpcMsg.h"
#include "EthsockIpcTsk.h"
#if (USE_IPC)
#include "dma_protected.h"
#include "NvtIpcAPI.h"
#include "HwMem.h"
#endif

#if (USE_IPC)
#define ipc_getPhyAddr(addr)        NvtIPC_GetPhyAddr(addr)
#else
#define ipc_getPhyAddr(addr)        addr
#endif

ER EthsockIpc_Test(ETHSOCKIPC_ID id, int param1, int param2, int param3);

#define ETHSOCKIPC_INVALID_PARAMADDR   (0)

static ETHSOCKIPC_CTRL EthsockObj[MAX_ETHSOCKET_NUM] = {0};
static UINT32 gParamSndAddr[MAX_ETHSOCKET_NUM];//parameter address
static UINT32 gParamSndSize[MAX_ETHSOCKET_NUM];//parameter size
static UINT32 gParamRcvAddr[MAX_ETHSOCKET_NUM];//parameter address
static UINT32 gParamRcvSize[MAX_ETHSOCKET_NUM];//parameter size

static ethsocket_recv *pRecvCB[MAX_ETHSOCKET_NUM] = {0};
static ethsocket_notify *pNotifyCB[MAX_ETHSOCKET_NUM] = {0};
//static ethsocket_recv *pUdpRecvCB = 0;
//static ethsocket_notify *pUdpNotifyCB = 0;

ETHSOCKIPC_CTRL *EthsockIpc_GetCtrl(ETHSOCKIPC_ID id)
{
	return &EthsockObj[id];
}

static void EthsockIpc_lock(ETHSOCKIPC_ID id)
{
	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);
	SEM_WAIT(p_ctrl->sem_id);
}

static void EthsockIpc_unlock(ETHSOCKIPC_ID id)
{
	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);
	SEM_SIGNAL(p_ctrl->sem_id);
}

static UINT32 EthsockIpc_getSndAddr(ETHSOCKIPC_ID id)
{
	if (*(UINT32 *)(gParamSndAddr[id]) == ETHSOCKETIPC_BUF_TAG) {
		//DBG_IND("%x %x \r\n",gParamSndAddr[id],*(UINT32 *)(gParamSndAddr[id]));
		return (gParamSndAddr[id] + ETHSOCKETIPC_BUF_CHK_SIZE);
	} else {
		DBG_ERR("%d buf chk fail %x  %x\r\n", id, gParamSndAddr[id], *(UINT32 *)(gParamSndAddr[id]));
		return ETHSOCKIPC_INVALID_PARAMADDR;
	}
}
static UINT32 EthsockIpc_getRcvAddr(ETHSOCKIPC_ID id)
{

	if (*(UINT32 *)(gParamRcvAddr[id]) == ETHSOCKETIPC_BUF_TAG) {
		//DBG_IND("%x %x \r\n",gParamRcvAddr[id],*(UINT32 *)(gParamRcvAddr[id]));
		return (gParamRcvAddr[id] + ETHSOCKETIPC_BUF_CHK_SIZE);
	} else {
		DBG_ERR("%d buf chk fail %x  %x\r\n", id, gParamRcvAddr[id], *(UINT32 *)(gParamRcvAddr[id]));
		return ETHSOCKIPC_INVALID_PARAMADDR;
	}
}


ER EthsockIpcMulti_Open(ETHSOCKIPC_ID id, ETHSOCKIPC_OPEN *pOpen)
{
#if 1//(USE_IPC)
	//CHAR szCmd[64] = {0};
	CHAR szCmd[128] = {0};
	//FLGPTN FlgPtn;
	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);


	DBG_FUNC_BEGIN("[API]\r\n");


	if (!pOpen->sharedSendMemAddr) {
		DBG_ERR("%d sharedSendMemAddr is NULL\r\n", id);
		return E_NOMEM;
	}
	if (!pOpen->sharedRecvMemAddr) {
		DBG_ERR("%d sharedRecvMemAddr is NULL\r\n", id);
		return E_NOMEM;
	}
#if defined __UITRON || defined __ECOS
	if (dma_isCacheAddr(pOpen->sharedSendMemAddr)) {
		DBG_ERR("%d sharedSendMemAddr 0x%x should be non-cache Address\r\n", id, (int)pOpen->sharedSendMemAddr);
		return E_PAR;
	}
	if (dma_isCacheAddr(pOpen->sharedRecvMemAddr)) {
		DBG_ERR("%d sharedRecvMemAddr 0x%x should be non-cache Address\r\n", id, (int)pOpen->sharedRecvMemAddr);
		return E_PAR;
	}
#endif
	if (pOpen->sharedSendMemSize < ETHSOCKIPC_PARAM_BUF_SIZE/2) {
		DBG_ERR("%d sharedSendMemSize 0x%x < need 0x%x\r\n", id, pOpen->sharedSendMemSize, ETHSOCKIPC_PARAM_BUF_SIZE/2);
		return E_PAR;
	}
	if (pOpen->sharedRecvMemSize < ETHSOCKIPC_PARAM_BUF_SIZE/2) {
		DBG_ERR("%d sharedRecvMemSize 0x%x < need 0x%x\r\n", id, pOpen->sharedRecvMemSize, ETHSOCKIPC_PARAM_BUF_SIZE/2);
		return E_PAR;
	}

	//init control obj
	switch (id) {
	case ETHSOCKIPC_ID_0:
		p_ctrl->task_id = ETHSOCKIPC_TSK_ID_0;
		p_ctrl->sem_id = ETHSOCKIPC_SEM_ID_0;
		p_ctrl->flag_id = ETHSOCKIPC_FLG_ID_0;
		p_ctrl->token_path = ETHSOCKET_TOKEN_PATH0;
		break;
#if (MAX_ETHSOCKET_NUM >= 2)
	case ETHSOCKIPC_ID_1:
		p_ctrl->task_id = ETHSOCKIPC_TSK_ID_1;
		p_ctrl->sem_id = ETHSOCKIPC_SEM_ID_1;
		p_ctrl->flag_id = ETHSOCKIPC_FLG_ID_1;
		p_ctrl->token_path = ETHSOCKET_TOKEN_PATH1;
		break;
	case ETHSOCKIPC_ID_2:
		p_ctrl->task_id = ETHSOCKIPC_TSK_ID_2;
		p_ctrl->sem_id = ETHSOCKIPC_SEM_ID_2;
		p_ctrl->flag_id = ETHSOCKIPC_FLG_ID_2;
		p_ctrl->token_path = ETHSOCKET_TOKEN_PATH2;
		break;

#endif
	default:
		DBG_ERR("not sup id%d\r\n", id);
		return E_PAR;
	}

	//error check
	if (!p_ctrl->sem_id) {
		DBG_ERR("ID %d not installed\r\n", id);
		return E_SYS;
	}

	//----------lock----------
	EthsockIpc_lock(id);

	if (p_ctrl->b_opened) {
		DBG_WRN("%d is opened\r\n", id);
		EthsockIpc_unlock(id);
		return E_OBJ;
		//return E_OK;
	}

	if (E_OK != EthsockIpc_Msg_Init(id)) {
		DBG_ERR("%d EthsockIpc_Msg_Init ERR\r\n", id);
		EthsockIpc_unlock(id);
		return E_SYS;
	}

	gParamSndAddr[id] = pOpen->sharedSendMemAddr;
	*(UINT32 *)gParamSndAddr[id] = ETHSOCKETIPC_BUF_TAG;
	gParamSndSize[id] =pOpen->sharedSendMemSize;

	//gParamRcvAddr[id] = pOpen->sharedMemAddr + (ETHSOCKIPC_PARAM_BUF_SIZE / 2) ;
	gParamRcvAddr[id] = pOpen->sharedRecvMemAddr ;
	*(UINT32 *)gParamRcvAddr[id] = ETHSOCKETIPC_BUF_TAG;
	gParamRcvSize[id] =pOpen->sharedRecvMemSize;

	//DBG_DUMP("id=%d, gParamSndAddr=0x%x, gParamRcvAddr=0x%x\r\n",id, gParamSndAddr[id],gParamRcvAddr[id] );
#if (USE_IPC)

	//create the receive task and wait it ready
	clr_flg(p_ctrl->flag_id, ETHSOCKIPC_FLGBIT_TSK_READY);
	sta_tsk(p_ctrl->task_id, 0);
	wai_flg(&FlgPtn, p_ctrl->flag_id, ETHSOCKIPC_FLGBIT_TSK_READY, TWF_ORW);
#endif
	//send syscall request ipc to ecos to open and init ethsocket
	//snprintf(szCmd, sizeof(szCmd), "%s -open %d %d %d %d %d %d &", p_ctrl->token_path, NvtIPC_GetPhyAddr(gParamSndAddr[id]), gParamSndSize[id]  ,NvtIPC_GetPhyAddr(gParamRcvAddr[id]), gParamRcvSize[id], ETHSOCKET_VER_KEY, id);
	snprintf(szCmd, sizeof(szCmd), "%s -open %d %d %d %d %d %d &", "ethsockipc", (unsigned int)ipc_getPhyAddr(gParamSndAddr[id]), (unsigned int)gParamSndSize[id]  ,(unsigned int)ipc_getPhyAddr(gParamRcvAddr[id]), (unsigned int)gParamRcvSize[id], ETHSOCKET_VER_KEY, id);
	if (E_OK != EthsockIpc_Msg_SysCallReq(id, szCmd)) {
		DBG_ERR("%d EthsockIpc_Msg_SysCallReq(%s) ERR\n", id, szCmd);
		EthsockIpc_unlock(id);
		return E_SYS;
	}

	if (ETHSOCKET_RET_OK != EthsockIpc_Msg_WaitAck(id)) {
		DBG_ERR("%d Open failed.\r\n", id);
		EthsockIpc_unlock(id);
		return E_SYS;
	}

	p_ctrl->b_opened = TRUE;

	EthsockIpc_unlock(id);
	//----------unlock----------

	DBG_FUNC_END("[API]\r\n");
#endif

	return E_OK;
}

ER EthsockIpcMulti_Close(ETHSOCKIPC_ID id)
{
#if 1//(USE_IPC)
	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);
	int iRet_EthsockApi = 0;
	DBG_FUNC_BEGIN("[API]\r\n");

	//----------lock----------
	EthsockIpc_lock(id);

	if (FALSE == p_ctrl->b_opened) {
		DBG_WRN("%d is closed\r\n", id);
		EthsockIpc_unlock(id);
		return E_OK;
	}

	if (E_OK != EthsockIpc_Msg_SendCmd(id, ETHSOCKET_CMDID_UNINIT, &iRet_EthsockApi) || ETHSOCKET_RET_OK != iRet_EthsockApi) {
		DBG_ERR("%d EthsockIpc_Msg_SendCmd(ETHSOCKET_CMDID_UNINIT) ERR\r\n", id);
		EthsockIpc_unlock(id);
		return E_SYS;
	}
#if (USE_IPC)

	ter_tsk(p_ctrl->task_id);
#endif
	if (E_OK != EthsockIpc_Msg_Uninit(id)) {
		DBG_ERR("%d EthsockIpc_Msg_Uninit ERR\r\n", id);
		EthsockIpc_unlock(id);
		return E_SYS;
	}

	p_ctrl->b_opened = FALSE;

	EthsockIpc_unlock(id);
	//----------unlock----------

	DBG_FUNC_END("[API]\r\n");
#endif
	return E_OK;
}
#if (USE_IPC)

ER EthsockIpc_Test(ETHSOCKIPC_ID id, int param1, int param2, int param3)
{

	ETHSOCKET_PARAM_PARAM *EthsockApiParam = 0;
	int iRet_EthsockApi = 0;
	int result = 0;

	EthsockIpc_lock(id);
	EthsockApiParam = (ETHSOCKET_PARAM_PARAM *)EthsockIpc_getSndAddr(id);
	EthsockApiParam->param1 = (int)param1;
	EthsockApiParam->param2 = (int)param2;
	EthsockApiParam->param3 = (int)param3;
	//DBG_IND("param1 = 0x%X, param2 = 0x%X, param3 = 0x%X\r\n", EthsockApiParam->param1, EthsockApiParam->param2, EthsockApiParam->param3);
	dma_flushWriteCache((UINT32)EthsockApiParam, sizeof(ETHSOCKET_PARAM_PARAM));
	result = EthsockIpc_Msg_SendCmd(id, ETHSOCKET_CMDID_TEST, &iRet_EthsockApi);
	EthsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_EthsockApi;
	} else {
		return ETHSOCKET_RET_ERR;
	}
}
#endif
ER EthsockIpcMulti_Init(ETHSOCKIPC_ID id, ethsocket_install_obj  *pObj)
{
#if 1//(USE_IPC)
	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);
	ETHSOCKET_PARAM_PARAM *EthsockApiParam = 0;
	int iRet_EthsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		DBG_ERR("not open %d\r\n", id);
		return ETHSOCKET_RET_NOT_OPEN;
	}

	if (!pObj) {
		return ETHSOCKET_RET_ERR_PAR;
	}

	EthsockIpc_lock(id);
	EthsockApiParam = (ETHSOCKET_PARAM_PARAM *)EthsockIpc_getSndAddr(id);
	memcpy(EthsockApiParam, pObj, sizeof(ethsocket_install_obj));
	if (pObj->notify) {
		pNotifyCB[id] = pObj->notify;
	}
	if (pObj->recv) {
		pRecvCB[id] = pObj->recv;
	}

	result = EthsockIpc_Msg_SendCmd(id, ETHSOCKET_CMDID_OPEN, &iRet_EthsockApi);
	EthsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_EthsockApi;
	} else {
		return ETHSOCKET_RET_ERR;
	}
#elif (defined (__ECOS))

	ethsocket_install(pObj);
	ethsocket_open();
	return 0;
#else
	return 0;
#endif
}


ER EthsockIpcMulti_Send(ETHSOCKIPC_ID id, char *addr, int *size)
{
#if 1//(USE_IPC)
	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);
	ETHSOCKET_TRANSFER_PARAM *EthsockApiTransParam = 0;
	ETHSOCKET_SEND_PARAM *EthsockApiSendParam = 0;
	int iRet_EthsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return ETHSOCKET_RET_NOT_OPEN;
	}

	if(id== ETHSOCKIPC_ID_0 || id== ETHSOCKIPC_ID_1){
		if (*size > (int )gParamSndSize[id]) {
			DBG_ERR("%d size %d > IPC buffer %d\r\n", id, *size, gParamSndSize[id]);
			return ETHSOCKET_RET_ERR;
		}
		EthsockIpc_lock(id);
		EthsockApiSendParam = (ETHSOCKET_SEND_PARAM *)EthsockIpc_getSndAddr(id);
		EthsockApiSendParam->size = *size;

		EthsockApiSendParam->buf=(char*)(EthsockIpc_getSndAddr(id)+ sizeof(ETHSOCKET_SEND_PARAM));

		//hwmem_open();
		//hwmem_memcpy((UINT32)EthsockApiSendParam->buf, (UINT32)(addr), (UINT32)(*size));
		//hwmem_close();
		memcpy((UINT8*)EthsockApiSendParam->buf, (UINT8*)(addr), (UINT32)(*size));

		//DBG_DUMP("size=%d\r\n",*size);
		result = EthsockIpc_Msg_SendCmd(id, ETHSOCKET_CMDID_SEND, &iRet_EthsockApi);
		*size = EthsockApiSendParam->size;
		EthsockIpc_unlock(id);

	}else{
		if (*size > ETHSOCKIPC_TRANSFER_BUF_SIZE) {
			DBG_ERR("%d size %d > IPC buffer %d\r\n", id, *size, ETHSOCKIPC_TRANSFER_BUF_SIZE);
			return ETHSOCKET_RET_ERR;
		}

		EthsockIpc_lock(id);
		EthsockApiTransParam = (ETHSOCKET_TRANSFER_PARAM *)EthsockIpc_getSndAddr(id);
		EthsockApiTransParam->size = *size;

		memset(EthsockApiTransParam->buf, 0, ETHSOCKIPC_TRANSFER_BUF_SIZE);
		memcpy(EthsockApiTransParam->buf, addr, *size);
		//hwmem_open();
		//hwmem_memcpy((UINT32)EthsockApiTransParam->buf, (UINT32)(addr), (UINT32)(*size));
		//hwmem_close();


		result = EthsockIpc_Msg_SendCmd(id, ETHSOCKET_CMDID_SEND, &iRet_EthsockApi);

		*size = EthsockApiTransParam->size;
		EthsockIpc_unlock(id);

	}


	if (result == E_OK) {
		return iRet_EthsockApi;
	} else {
		return ETHSOCKET_RET_ERR;
	}
#elif (defined (__ECOS))
	return ethsocket_send(addr, size);
#else
	return 0;
#endif
}


ER EthsockIpcMulti_Uninit(ETHSOCKIPC_ID id)
{
#if 1//(USE_IPC)
	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);
	int iRet_EthsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return ETHSOCKET_RET_NOT_OPEN;
	}

	EthsockIpc_lock(id);
	result = EthsockIpc_Msg_SendCmd(id, ETHSOCKET_CMDID_CLOSE, &iRet_EthsockApi);
	EthsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_EthsockApi;
	} else {
		return ETHSOCKET_RET_ERR;
	}
#elif (defined (__ECOS))

	ethsocket_close();
	return E_OK;
#else
	return 0;
#endif
}

#if 1//(USE_IPC)

INT32 EthsockIpc_CmdId_Notify(ETHSOCKIPC_ID id)
{
	ETHSOCKET_PARAM_PARAM *EthsockApiParam = (ETHSOCKET_PARAM_PARAM *)EthsockIpc_getRcvAddr(id);

	if (pNotifyCB[id]) {
		pNotifyCB[id](EthsockApiParam->param1, EthsockApiParam->param2);
		return ETHSOCKET_RET_OK;
	} else {
		DBG_ERR("no pNotifyCB[%d]\r\n", id);
		return ETHSOCKET_RET_ERR;
	}
}

INT32 EthsockIpc_CmdId_Receive(ETHSOCKIPC_ID id)
{
	ETHSOCKET_TRANSFER_PARAM *EthsockApiParam = (ETHSOCKET_TRANSFER_PARAM *)EthsockIpc_getRcvAddr(id);

	if (pRecvCB[id]) {
		pRecvCB[id](EthsockApiParam->buf, EthsockApiParam->size);
		return ETHSOCKET_RET_OK;
	} else {
		DBG_ERR("no pRecvCB[%d]\r\n", id);
		return ETHSOCKET_RET_ERR;
	}

}
#endif

//////////////////////////EthsockIpc_UDP///////////////////////////////////////////////

ER EthsockIpc_Udp_Init(ethsocket_install_obj  *pObj)
{
#if (USE_IPC)
	UINT32 id = ETHSOCKIPC_ID_0;//only support one udp socket
	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);
	ETHSOCKET_PARAM_PARAM *EthsockApiParam = 0;
	int iRet_EthsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return ETHSOCKET_RET_NOT_OPEN;
	}

	if (!pObj) {
		return ETHSOCKET_RET_ERR_PAR;
	}

	EthsockIpc_lock(id);
	EthsockApiParam = (ETHSOCKET_PARAM_PARAM *)EthsockIpc_getSndAddr(id);

	memcpy(EthsockApiParam, pObj, sizeof(ethsocket_install_obj));

	if (pObj->notify) {
		pUdpNotifyCB = pObj->notify;
	}
	if (pObj->recv) {
		pUdpRecvCB = pObj->recv;
	}

	result = EthsockIpc_Msg_SendCmd(id, ETHSOCKET_CMDID_UDP_OPEN, &iRet_EthsockApi);
	EthsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_EthsockApi;
	} else {
		return ETHSOCKET_RET_ERR;
	}
#else
	ethsocket_udp_install(pObj);
	ethsocket_udp_open();
	return 0;
#endif

}

ER EthsockIpc_Udp_Send(char *addr, int *size)
{
#if (USE_IPC)
	UINT32 id = ETHSOCKIPC_ID_0;//only support one udp socket
	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);
	ETHSOCKET_TRANSFER_PARAM *EthsockApiParam = 0;
	int iRet_EthsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return ETHSOCKET_RET_NOT_OPEN;
	}

	if (*size > ETHSOCKIPC_TRANSFER_BUF_SIZE) {
		DBG_ERR("%d size %d > IPC buffer %d\r\n", id, *size, ETHSOCKIPC_TRANSFER_BUF_SIZE);
		return ETHSOCKET_RET_ERR;
	}

	EthsockIpc_lock(id);
	EthsockApiParam = (ETHSOCKET_TRANSFER_PARAM *)EthsockIpc_getSndAddr(id);
	EthsockApiParam->size = *size;
	memcpy(EthsockApiParam->buf, addr, *size);
	result = EthsockIpc_Msg_SendCmd(id, ETHSOCKET_CMDID_UDP_SEND, &iRet_EthsockApi);
	*size = EthsockApiParam->size;
	EthsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_EthsockApi;
	} else {
		return ETHSOCKET_RET_ERR;
	}
#else

	return ethsocket_udp_send(addr, size);
#endif

}

ER EthsockIpc_Udp_Sendto(char *dest_ip, int dest_port, char *buf, int *size)
{
#if (USE_IPC)
	UINT32 id = ETHSOCKIPC_ID_0;//only support one udp socket
	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);
	ETHSOCKET_SENDTO_PARAM *EthsockApiParam = 0;
	int iRet_EthsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return ETHSOCKET_RET_NOT_OPEN;
	}

	if (*size > ETHSOCKIPC_TRANSFER_BUF_SIZE) {
		DBG_ERR("%d size %d > IPC buffer %d\r\n", id, *size, ETHSOCKIPC_TRANSFER_BUF_SIZE);
		return ETHSOCKET_RET_ERR;
	}

	EthsockIpc_lock(id);
	EthsockApiParam = (ETHSOCKET_SENDTO_PARAM *)EthsockIpc_getSndAddr(id);
	memset(EthsockApiParam->dest_ip, 0, ETHSOCKIPC_IP_BUF);
	strncpy(EthsockApiParam->dest_ip, dest_ip, ETHSOCKIPC_IP_BUF - 1);
	EthsockApiParam->dest_port = dest_port;
	EthsockApiParam->size = *size;
	memcpy(EthsockApiParam->buf, buf, *size);
	result = EthsockIpc_Msg_SendCmd(id, ETHSOCKET_CMDID_UDP_SENDTO, &iRet_EthsockApi);
	*size = EthsockApiParam->size;
	EthsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_EthsockApi;
	} else {
		return ETHSOCKET_RET_ERR;
	}
#else

	return ethsocket_udp_sendto(dest_ip, dest_port, buf, size);;
#endif

}

ER EthsockIpc_Udp_Uninit(void)
{
#if (USE_IPC)
	UINT32 id = ETHSOCKIPC_ID_0;//only support one udp socket
	ETHSOCKIPC_CTRL *p_ctrl = EthsockIpc_GetCtrl(id);
	int iRet_EthsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return ETHSOCKET_RET_NOT_OPEN;
	}

	EthsockIpc_lock(id);
	result = EthsockIpc_Msg_SendCmd(id, ETHSOCKET_CMDID_UDP_CLOSE, &iRet_EthsockApi);
	EthsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_EthsockApi;
	} else {
		return ETHSOCKET_RET_ERR;
	}
#else

	ethsocket_udp_close();
	return E_OK;
#endif
}

#if (USE_IPC)

INT32 EthsockIpc_CmdId_Udp_Notify(ETHSOCKIPC_ID id)
{
	ETHSOCKET_PARAM_PARAM *EthsockApiParam = (ETHSOCKET_PARAM_PARAM *)EthsockIpc_getRcvAddr(id);

	if (pUdpNotifyCB) {
		pUdpNotifyCB(EthsockApiParam->param1, EthsockApiParam->param2);
		return ETHSOCKET_RET_OK;
	} else {
		DBG_ERR("no pNotifyCB[%d]\r\n", id);
		return ETHSOCKET_RET_ERR;
	}
}


INT32 EthsockIpc_CmdId_Udp_Receive(ETHSOCKIPC_ID id)
{

	ETHSOCKET_TRANSFER_PARAM *EthsockApiParam = (ETHSOCKET_TRANSFER_PARAM *)EthsockIpc_getRcvAddr(id);

	if (pUdpRecvCB) {
		pUdpRecvCB(EthsockApiParam->buf, EthsockApiParam->size);
		return ETHSOCKET_RET_OK;
	} else {
		DBG_ERR("no pRecvCB[%d]\r\n", id);
		return ETHSOCKET_RET_ERR;
	}

}
#endif

