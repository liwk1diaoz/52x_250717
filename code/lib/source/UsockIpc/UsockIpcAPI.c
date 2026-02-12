#include <stdio.h>
#include "UsockIpc/UsockIpcAPI.h"
#include "UsockIpcInt.h"
#include "kwrap/error_no.h"

static usocket_recv *pRecvCB[MAX_USOCKET_NUM] = {0};
static usocket_notify *pNotifyCB[MAX_USOCKET_NUM] = {0};

#if (USE_IPC)
#include "UsockIpcID.h"
#include "UsockIpcMsg.h"
#include "UsockIpcTsk.h"
#include "dma_protected.h"
#include "NvtIpcAPI.h"

ER UsockIpc_Test(USOCKIPC_ID id,int param1, int param2, int param3);

#define ipc_isCacheAddr(addr)       NvtIPC_IsCacheAddr(addr)
#define ipc_getPhyAddr(addr)        NvtIPC_GetPhyAddr(addr)
#define ipc_getNonCacheAddr(addr)   NvtIPC_GetNonCacheAddr(addr)

#define USOCKIPC_INVALID_PARAMADDR   (0)

static USOCKIPC_CTRL UsockObj[MAX_USOCKET_NUM]={0};
static UINT32 gParamSndAddr[MAX_USOCKET_NUM];//parameter address
static UINT32 gParamRcvAddr[MAX_USOCKET_NUM];//parameter address

static usocket_recv *pUdpRecvCB = 0;
static usocket_notify *pUdpNotifyCB = 0;

USOCKIPC_CTRL *UsockIpc_GetCtrl(USOCKIPC_ID id)
{
    return &UsockObj[id];
}

static void UsockIpc_lock(USOCKIPC_ID id)
{
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);
	wai_sem(p_ctrl->sem_id);
}

static void UsockIpc_unlock(USOCKIPC_ID id)
{
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);
	sig_sem(p_ctrl->sem_id);
}

static UINT32 UsockIpc_getSndAddr(USOCKIPC_ID id)
{
	if (*(UINT32 *)(gParamSndAddr[id]) == USOCKETIPC_BUF_TAG) {
		//DBG_IND("%x %x \r\n",gParamSndAddr[id],*(UINT32 *)(gParamSndAddr[id]));
		return (gParamSndAddr[id] + USOCKETIPC_BUF_CHK_SIZE);
	} else {
		DBG_ERR("%d buf chk fail %x  %x\r\n",id,gParamSndAddr[id],*(UINT32 *)(gParamSndAddr[id]));
		return USOCKIPC_INVALID_PARAMADDR;
	}
}
static UINT32 UsockIpc_getRcvAddr(USOCKIPC_ID id)
{

	if (*(UINT32 *)(gParamRcvAddr[id]) == USOCKETIPC_BUF_TAG) {
		//DBG_IND("%x %x \r\n",gParamRcvAddr[id],*(UINT32 *)(gParamRcvAddr[id]));
		return (gParamRcvAddr[id] + USOCKETIPC_BUF_CHK_SIZE);
	} else {
		DBG_ERR("%d buf chk fail %x  %x\r\n",id,gParamRcvAddr[id],*(UINT32 *)(gParamRcvAddr[id]));
		return USOCKIPC_INVALID_PARAMADDR;
	}
}

#endif

ER UsockIpcMulti_Open(USOCKIPC_ID id,USOCKIPC_OPEN *pOpen)
{
#if (USE_IPC)
	CHAR szCmd[64] = {0};
	FLGPTN FlgPtn;
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);


	DBG_FUNC_BEGIN("[API]\r\n");


	if (!pOpen->sharedMemAddr) {
		DBG_ERR("%d sharedMemAddr is NULL\r\n",id);
		return E_NOMEM;
	}
	if (dma_isCacheAddr(pOpen->sharedMemAddr)) {
		DBG_ERR("%d sharedMemAddr 0x%x should be non-cache Address\r\n",id, (int)pOpen->sharedMemAddr);
		return E_PAR;
	}
	if (pOpen->sharedMemSize < USOCKIPC_PARAM_BUF_SIZE) {
		DBG_ERR("%d sharedMemSize 0x%x < need 0x%x\r\n",id, pOpen->sharedMemSize,USOCKIPC_PARAM_BUF_SIZE);
		return E_PAR;
	}

    //init control obj
    switch (id) {
		case USOCKIPC_ID_0:
			p_ctrl->task_id = USOCKIPC_TSK_ID_0;
			p_ctrl->sem_id = USOCKIPC_SEM_ID_0;
			p_ctrl->flag_id = USOCKIPC_FLG_ID_0;
            p_ctrl->token_path = USOCKET_TOKEN_PATH0;
			break;
#if (MAX_USOCKET_NUM >= 2)
		case USOCKIPC_ID_1:
			p_ctrl->task_id = USOCKIPC_TSK_ID_1;
			p_ctrl->sem_id = USOCKIPC_SEM_ID_1;
			p_ctrl->flag_id = USOCKIPC_FLG_ID_1;
            p_ctrl->token_path = USOCKET_TOKEN_PATH1;
			break;
#endif
        default:
            DBG_ERR("not sup id%d\r\n",id);
            return E_PAR;
    }

	//error check
	if (!p_ctrl->sem_id) {
		DBG_ERR("ID %d not installed\r\n",id);
		return E_SYS;
	}

	//----------lock----------
	UsockIpc_lock(id);

	if (p_ctrl->b_opened) {
		DBG_WRN("%d is opened\r\n",id);
		UsockIpc_unlock(id);
		return E_OK;
	}

	if (E_OK != UsockIpc_Msg_Init(id)) {
		DBG_ERR("%d UsockIpc_Msg_Init ERR\r\n",id);
		UsockIpc_unlock(id);
		return E_SYS;
	}

	gParamSndAddr[id] = pOpen->sharedMemAddr;
	*(UINT32 *)gParamSndAddr[id] = USOCKETIPC_BUF_TAG;
	gParamRcvAddr[id] = pOpen->sharedMemAddr + (USOCKIPC_PARAM_BUF_SIZE / 2) ;
	*(UINT32 *)gParamRcvAddr[id] = USOCKETIPC_BUF_TAG;

	//create the receive task and wait it ready
	clr_flg(p_ctrl->flag_id, USOCKIPC_FLGBIT_TSK_READY);
	sta_tsk(p_ctrl->task_id, 0);
	wai_flg(&FlgPtn, p_ctrl->flag_id, USOCKIPC_FLGBIT_TSK_READY, TWF_ORW);

	//send syscall request ipc to ecos to open and init usocket
    snprintf(szCmd, sizeof(szCmd), "%s -open %d %d %d %d &", p_ctrl->token_path, ipc_getPhyAddr(gParamSndAddr[id]),ipc_getPhyAddr(gParamRcvAddr[id]),USOCKET_VER_KEY,id);
	if (E_OK != UsockIpc_Msg_SysCallReq(id,szCmd)) {
		DBG_ERR("%d UsockIpc_Msg_SysCallReq(%s) ERR\n",id, szCmd);
		UsockIpc_unlock(id);
		return E_SYS;
	}

	if (USOCKET_RET_OK != UsockIpc_Msg_WaitAck(id)) {
		DBG_ERR("%d Open failed.\r\n",id);
		UsockIpc_unlock(id);
		return E_SYS;
	}

	p_ctrl->b_opened = TRUE;

	UsockIpc_unlock(id);
	//----------unlock----------

	DBG_FUNC_END("[API]\r\n");
#endif

	return E_OK;
}

ER UsockIpcMulti_Close(USOCKIPC_ID id)
{
#if (USE_IPC)
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);
	int iRet_UsockApi = 0;
	DBG_FUNC_BEGIN("[API]\r\n");

	//----------lock----------
	UsockIpc_lock(id);

	if (FALSE == p_ctrl->b_opened) {
		DBG_WRN("%d is closed\r\n",id);
		UsockIpc_unlock(id);
		return E_OK;
	}

	if (E_OK != UsockIpc_Msg_SendCmd(id,USOCKET_CMDID_UNINIT, &iRet_UsockApi) || USOCKET_RET_OK != iRet_UsockApi) {
		DBG_ERR("%d UsockIpc_Msg_SendCmd(USOCKET_CMDID_UNINIT) ERR\r\n",id);
		UsockIpc_unlock(id);
		return E_SYS;
	}

	ter_tsk(p_ctrl->task_id);

	if (E_OK != UsockIpc_Msg_Uninit(id)) {
		DBG_ERR("%d UsockIpc_Msg_Uninit ERR\r\n",id);
		UsockIpc_unlock(id);
		return E_SYS;
	}

	p_ctrl->b_opened = FALSE;

	UsockIpc_unlock(id);
	//----------unlock----------

	DBG_FUNC_END("[API]\r\n");
#endif
	return E_OK;
}
#if (USE_IPC)

ER UsockIpc_Test(USOCKIPC_ID id,int param1, int param2, int param3)
{

	USOCKET_PARAM_PARAM *UsockApiParam = 0;
	int iRet_UsockApi = 0;
	int result = 0;

	UsockIpc_lock(id);
	UsockApiParam = (USOCKET_PARAM_PARAM *)UsockIpc_getSndAddr(id);
	UsockApiParam->param1 = (int)param1;
	UsockApiParam->param2 = (int)param2;
	UsockApiParam->param3 = (int)param3;
	//DBG_IND("param1 = 0x%X, param2 = 0x%X, param3 = 0x%X\r\n", UsockApiParam->param1, UsockApiParam->param2, UsockApiParam->param3);
	dma_flushWriteCache((UINT32)UsockApiParam, sizeof(USOCKET_PARAM_PARAM));
	result = UsockIpc_Msg_SendCmd(id,USOCKET_CMDID_TEST, &iRet_UsockApi);
	UsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_UsockApi;
	} else {
		return USOCKET_RET_ERR;
	}
}
#endif

void usocket_multi_notify(int status,int parm,int obj_index)
{
    pNotifyCB[obj_index](status,parm);
}
int usocket_multi_recv(char* addr, int size,int obj_index)
{
    if((obj_index<MAX_USOCKET_NUM)&&(pRecvCB[obj_index])){
        return pRecvCB[obj_index](addr,size);
    }
    return E_NOSPT;
}

ER UsockIpcMulti_Init(USOCKIPC_ID id,usocket_install_obj  *pObj)
{
#if (USE_IPC)
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);
	USOCKET_PARAM_PARAM *UsockApiParam = 0;
	int iRet_UsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
        DBG_ERR("not open %d\r\n",id);
		return USOCKET_RET_NOT_OPEN;
	}

	if (!pObj) {
		return USOCKET_RET_ERR_PAR;
	}

	UsockIpc_lock(id);
	UsockApiParam = (USOCKET_PARAM_PARAM *)UsockIpc_getSndAddr(id);
	memcpy(UsockApiParam, pObj, sizeof(usocket_install_obj));
	if (pObj->notify) {
		pNotifyCB[id] = pObj->notify;
	}
	if (pObj->recv) {
		pRecvCB[id] = pObj->recv;
	}

	result = UsockIpc_Msg_SendCmd(id,USOCKET_CMDID_OPEN, &iRet_UsockApi);
	UsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_UsockApi;
	} else {
		return USOCKET_RET_ERR;
	}
#elif (defined (__ECOS)||defined (__FREERTOS))

    if(id==USOCKIPC_ID_0) {
        usocket_install(pObj);
        usocket_open();
    } else if (id<USOCKIPC_MAX_NUM) {
        static int multi_id =0;
        usocket_install_obj_multi MultiObj={0};
        if(pObj->notify){
            MultiObj.notify = usocket_multi_notify;
            pNotifyCB[id] = pObj->notify;
        }
        if(pObj->recv){
            MultiObj.recv = usocket_multi_recv;
            pRecvCB[id] = pObj->recv;
        }

        MultiObj.portNum = pObj->portNum;
        MultiObj.threadPriority = pObj->threadPriority;
        MultiObj.sockbufSize = pObj->sockbufSize;

        multi_id = usocket_install_multi(&MultiObj,id);

        if((multi_id!=(int)id)||(multi_id<0)) {
            DBG_ERR("open id not match %d %d \r\n",multi_id,id);
            return E_SYS;
        }
        usocket_open_multi(multi_id);

    } else {
        DBG_ERR("not sup %d\r\n",id);
        return E_NOSPT;
    }
    return 0;
#elif (defined (__LINUX_USER__))
    if(id==USOCKIPC_ID_0) {
        usocket_install(pObj);
        usocket_open();
    } else {
        DBG_ERR("not sup %d\r\n",id);
        return E_NOSPT;
    }

    return 0;
#else
    return 0;
#endif
}


ER UsockIpcMulti_Send(USOCKIPC_ID id,char *addr, int *size)
{
#if (USE_IPC)
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);
	USOCKET_TRANSFER_PARAM *UsockApiParam = 0;
	int iRet_UsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return USOCKET_RET_NOT_OPEN;
	}

	if (*size > USOCKIPC_TRANSFER_BUF_SIZE) {
		DBG_ERR("%d size %d > IPC buffer %d\r\n",id, *size, USOCKIPC_TRANSFER_BUF_SIZE);
		return USOCKET_RET_ERR;
	}

	UsockIpc_lock(id);
	UsockApiParam = (USOCKET_TRANSFER_PARAM *)UsockIpc_getSndAddr(id);
	UsockApiParam->size = *size;
	memcpy(UsockApiParam->buf, addr, *size);
	result = UsockIpc_Msg_SendCmd(id,USOCKET_CMDID_SEND, &iRet_UsockApi);
	*size = UsockApiParam->size;
	UsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_UsockApi;
	} else {
		return USOCKET_RET_ERR;
	}
#elif (defined (__ECOS)||defined (__FREERTOS))

    if(id==USOCKIPC_ID_0) {
        return usocket_send(addr,size);
    } else if (id<USOCKIPC_MAX_NUM) {
        return usocket_send_multi(addr,size,id);
    } else {
        DBG_ERR("not sup %d\r\n",id);
        return E_NOSPT;
    }
#elif (defined (__LINUX_USER__))
if(id==USOCKIPC_ID_0) {
        return usocket_send(addr,size);
    }  else {
        DBG_ERR("not sup %d\r\n",id);
        return E_NOSPT;
    }
    return 0;

#else
    return 0;
#endif
}


ER UsockIpcMulti_Uninit(USOCKIPC_ID id)
{
#if (USE_IPC)
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);
	int iRet_UsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return USOCKET_RET_NOT_OPEN;
	}

	UsockIpc_lock(id);
	result = UsockIpc_Msg_SendCmd(id,USOCKET_CMDID_CLOSE, &iRet_UsockApi);
	UsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_UsockApi;
	} else {
		return USOCKET_RET_ERR;
	}
#elif (defined (__ECOS)||defined (__FREERTOS))
    if(id==USOCKIPC_ID_0) {
        usocket_close();
    } else if (id<USOCKIPC_MAX_NUM) {
        usocket_close_multi(id);
    } else {
        DBG_ERR("not sup %d\r\n",id);
        return E_NOSPT;
    }
    return E_OK;
#elif (defined (__LINUX_USER__))
    if(id==USOCKIPC_ID_0) {
        usocket_close();
    } else {
        DBG_ERR("not sup %d\r\n",id);
        return E_NOSPT;
    }
    return 0;

#else
    return 0;
#endif
}

#if (USE_IPC)

INT32 UsockIpc_CmdId_Notify(USOCKIPC_ID id)
{
	USOCKET_PARAM_PARAM *UsockApiParam = (USOCKET_PARAM_PARAM *)UsockIpc_getRcvAddr(id);

	if (pNotifyCB[id]) {
		pNotifyCB[id](UsockApiParam->param1, UsockApiParam->param2);
		return USOCKET_RET_OK;
	} else {
		DBG_ERR("no pNotifyCB[%d]\r\n",id);
		return USOCKET_RET_ERR;
	}
}

INT32 UsockIpc_CmdId_Receive(USOCKIPC_ID id)
{
	USOCKET_TRANSFER_PARAM *UsockApiParam = (USOCKET_TRANSFER_PARAM *)UsockIpc_getRcvAddr(id);

	if (pRecvCB[id]) {
		pRecvCB[id](UsockApiParam->buf, UsockApiParam->size);
		return USOCKET_RET_OK;
	} else {
		DBG_ERR("no pRecvCB[%d]\r\n",id);
		return USOCKET_RET_ERR;
	}

}
#endif

//////////////////////////UsockIpc_UDP///////////////////////////////////////////////

ER UsockIpc_Udp_Init(usocket_install_obj  *pObj)
{
#if (USE_IPC)
    UINT32 id = USOCKIPC_ID_0;//only support one udp socket
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);
	USOCKET_PARAM_PARAM *UsockApiParam = 0;
	int iRet_UsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return USOCKET_RET_NOT_OPEN;
	}

	if (!pObj) {
		return USOCKET_RET_ERR_PAR;
	}

	UsockIpc_lock(id);
	UsockApiParam = (USOCKET_PARAM_PARAM *)UsockIpc_getSndAddr(id);

	memcpy(UsockApiParam, pObj, sizeof(usocket_install_obj));

	if (pObj->notify) {
		pUdpNotifyCB = pObj->notify;
	}
	if (pObj->recv) {
		pUdpRecvCB = pObj->recv;
	}

	result = UsockIpc_Msg_SendCmd(id,USOCKET_CMDID_UDP_OPEN, &iRet_UsockApi);
	UsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_UsockApi;
	} else {
		return USOCKET_RET_ERR;
	}
#else
    usocket_udp_install(pObj);
    usocket_udp_open();
    return 0;
#endif

}


ER UsockIpc_Udp_Send(char *addr, int *size)
{
#if (USE_IPC)
    UINT32 id = USOCKIPC_ID_0;//only support one udp socket
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);
	USOCKET_TRANSFER_PARAM *UsockApiParam = 0;
	int iRet_UsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return USOCKET_RET_NOT_OPEN;
	}

	if (*size > USOCKIPC_TRANSFER_BUF_SIZE) {
		DBG_ERR("%d size %d > IPC buffer %d\r\n",id, *size, USOCKIPC_TRANSFER_BUF_SIZE);
		return USOCKET_RET_ERR;
	}

	UsockIpc_lock(id);
	UsockApiParam = (USOCKET_TRANSFER_PARAM *)UsockIpc_getSndAddr(id);
	UsockApiParam->size = *size;
	memcpy(UsockApiParam->buf, addr, *size);
	result = UsockIpc_Msg_SendCmd(id,USOCKET_CMDID_UDP_SEND, &iRet_UsockApi);
	*size = UsockApiParam->size;
	UsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_UsockApi;
	} else {
		return USOCKET_RET_ERR;
	}
#else

    return usocket_udp_send(addr,size);
#endif

}

ER UsockIpc_Udp_Sendto(char *dest_ip, int dest_port, char *buf, int *size)
{
#if (USE_IPC)
    UINT32 id = USOCKIPC_ID_0;//only support one udp socket
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);
	USOCKET_SENDTO_PARAM *UsockApiParam = 0;
	int iRet_UsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return USOCKET_RET_NOT_OPEN;
	}

	if (*size > USOCKIPC_TRANSFER_BUF_SIZE) {
		DBG_ERR("%d size %d > IPC buffer %d\r\n",id, *size, USOCKIPC_TRANSFER_BUF_SIZE);
		return USOCKET_RET_ERR;
	}

	UsockIpc_lock(id);
	UsockApiParam = (USOCKET_SENDTO_PARAM *)UsockIpc_getSndAddr(id);
	memset(UsockApiParam->dest_ip, 0, USOCKIPC_IP_BUF);
	strncpy(UsockApiParam->dest_ip, dest_ip, USOCKIPC_IP_BUF - 1);
	UsockApiParam->dest_port = dest_port;
	UsockApiParam->size = *size;
	memcpy(UsockApiParam->buf, buf, *size);
	result = UsockIpc_Msg_SendCmd(id,USOCKET_CMDID_UDP_SENDTO, &iRet_UsockApi);
	*size = UsockApiParam->size;
	UsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_UsockApi;
	} else {
		return USOCKET_RET_ERR;
	}
#else

    return usocket_udp_sendto(dest_ip,dest_port,buf,size);;
#endif

}

ER UsockIpc_Udp_Uninit(void)
{
#if (USE_IPC)
    UINT32 id = USOCKIPC_ID_0;//only support one udp socket
	USOCKIPC_CTRL *p_ctrl = UsockIpc_GetCtrl(id);
	int iRet_UsockApi = 0;
	int result = 0;

	if (!p_ctrl->b_opened) {
		return USOCKET_RET_NOT_OPEN;
	}

	UsockIpc_lock(id);
	result = UsockIpc_Msg_SendCmd(id,USOCKET_CMDID_UDP_CLOSE, &iRet_UsockApi);
	UsockIpc_unlock(id);

	if (result == E_OK) {
		return iRet_UsockApi;
	} else {
		return USOCKET_RET_ERR;
	}
#else

    usocket_udp_close();
    return USOCKET_RET_OK;
#endif
}

#if (USE_IPC)

INT32 UsockIpc_CmdId_Udp_Notify(USOCKIPC_ID id)
{
	USOCKET_PARAM_PARAM *UsockApiParam = (USOCKET_PARAM_PARAM *)UsockIpc_getRcvAddr(id);

	if (pUdpNotifyCB) {
		pUdpNotifyCB(UsockApiParam->param1, UsockApiParam->param2);
		return USOCKET_RET_OK;
	} else {
		DBG_ERR("no pNotifyCB[%d]\r\n",id);
		return USOCKET_RET_ERR;
	}
}


INT32 UsockIpc_CmdId_Udp_Receive(USOCKIPC_ID id)
{

	USOCKET_TRANSFER_PARAM *UsockApiParam = (USOCKET_TRANSFER_PARAM *)UsockIpc_getRcvAddr(id);

	if (pUdpRecvCB) {
		pUdpRecvCB(UsockApiParam->buf, UsockApiParam->size);
		return USOCKET_RET_OK;
	} else {
		DBG_ERR("no pRecvCB[%d]\r\n",id);
		return USOCKET_RET_ERR;
	}

}

#endif
