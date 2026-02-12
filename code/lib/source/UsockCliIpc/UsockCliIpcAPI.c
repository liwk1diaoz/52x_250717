#include <stdio.h>
#include "UsockCliIpc/UsockCliIpcAPI.h"
#include "UsockCliIpcInt.h"
#include "kwrap/error_no.h"
#if (USE_IPC)
#include "UsockCliIpcID.h"
#include "UsockCliIpcMsg.h"
#include "UsockCliIpcTsk.h"
#include "dma_protected.h"

#define USOCKETCLI_VER_KEY              20160519

ER UsockCliIpc_Test( int param1, int param2, int param3 );

#define ipc_getPhyAddr(addr)        ((addr) & 0x1FFFFFFF)
#define ipc_getNonCacheAddr(addr)   (((addr) & 0x1FFFFFFF)|0xA0000000)
#define USOCKCLIIPC_INVALID_PARAMADDR   (0)

static BOOL bIsOpened = FALSE;
static UINT32 gParamSndAddr;//parameter address
static UINT32 gParamRcvAddr;//parameter address

static usocket_cli_recv *pRecvCB = 0;
static usocket_cli_notify*pNotifyCB = 0;

static void UsockCliIpc_lock(void)
{
    wai_sem(USOCKCLIIPC_SEM_ID);
}

static void UsockCliIpc_unlock(void)
{
    sig_sem(USOCKCLIIPC_SEM_ID);
}

static UINT32 UsockCliIpc_getSndAddr(void)
{

    if(*(UINT32 *)(gParamSndAddr) == USOCKETCLIIPC_BUF_TAG)
    {
        //DBG_IND("%x %x \r\n",gParamSndAddr,*(UINT32 *)(gParamSndAddr));
        return (gParamSndAddr+USOCKETCLIIPC_BUF_CHK_SIZE);
    }
    else
    {
        DBG_ERR("buf chk fail\r\n");
        return USOCKCLIIPC_INVALID_PARAMADDR;
    }
}
static UINT32 UsockCliIpc_getRcvAddr(void)
{

    if(*(UINT32 *)(gParamRcvAddr) == USOCKETCLIIPC_BUF_TAG)
    {
        //DBG_IND("%x %x \r\n",gParamRcvAddr,*(UINT32 *)(gParamRcvAddr));
        return (gParamRcvAddr+USOCKETCLIIPC_BUF_CHK_SIZE);
    }
    else
    {
        DBG_ERR("buf chk fail\r\n");
        return USOCKCLIIPC_INVALID_PARAMADDR;
    }
}

#endif
ER UsockCliIpc_Open(USOCKCLIIPC_OPEN *pOpen)
{
#if (USE_IPC)
    CHAR szCmd[50] = {0};
    FLGPTN FlgPtn;
    DBG_FUNC_BEGIN("[API]\r\n");

    if (!pOpen->sharedMemAddr)
    {
        DBG_ERR("sharedMemAddr is NULL\r\n");
        return E_NOMEM;
    }
    if (dma_isCacheAddr(pOpen->sharedMemAddr))
    {
        DBG_ERR("sharedMemAddr 0x%x should be non-cache Address\r\n",(int)pOpen->sharedMemAddr);
        return E_PAR;
    }
    if (pOpen->sharedMemSize < USOCKCLIIPC_PARAM_BUF_SIZE)
    {
        DBG_ERR("sharedMemSize 0x%x < need 0x%x\r\n",pOpen->sharedMemSize,USOCKCLIIPC_PARAM_BUF_SIZE);
        return E_PAR;
    }


    //error check
    if (!USOCKCLIIPC_SEM_ID)
    {
        DBG_ERR("ID not installed\r\n");
        return E_SYS;
    }

    //----------lock----------
    UsockCliIpc_lock();

    if(bIsOpened)
    {
        DBG_WRN("is opened\r\n");
        UsockCliIpc_unlock();
        return E_OK;
    }

    if(E_OK != UsockCliIpc_Msg_Init())
    {
        DBG_ERR("UsockCliIpc_Msg_Init ERR\r\n");
        UsockCliIpc_unlock();
        return E_SYS;
    }

    gParamSndAddr = pOpen->sharedMemAddr;
    *(UINT32 *)gParamSndAddr = USOCKETCLIIPC_BUF_TAG;
    gParamRcvAddr = pOpen->sharedMemAddr+ (USOCKCLIIPC_PARAM_BUF_SIZE/2) ;
    *(UINT32 *)gParamRcvAddr = USOCKETCLIIPC_BUF_TAG;

    DBGH(gParamSndAddr);
    DBGH(gParamRcvAddr);

    //create the receive task and wait it ready
    clr_flg(USOCKCLIIPC_FLG_ID, USOCKCLIIPC_FLGBIT_TSK_READY);
    sta_tsk(USOCKCLIIPC_TSK_ID, 0);
    wai_flg(&FlgPtn, USOCKCLIIPC_FLG_ID, USOCKCLIIPC_FLGBIT_TSK_READY, TWF_ORW);

    //send syscall request ipc to ecos to open and init usocket
    snprintf(szCmd, sizeof(szCmd), "%s -open %d %d %d &", USOCKETCLI_TOKEN_PATH, ipc_getPhyAddr(gParamSndAddr),ipc_getPhyAddr(gParamRcvAddr),USOCKETCLI_VER_KEY);
    if(E_OK != UsockCliIpc_Msg_SysCallReq(szCmd))
    {
        DBG_ERR("UsockCliIpc_Msg_SysCallReq(%s) ERR\n", szCmd);
        UsockCliIpc_unlock();
        return E_SYS;
    }

    if(USOCKETCLI_RET_OK != UsockCliIpc_Msg_WaitAck())
    {
        DBG_ERR("Open failed.\r\n");
        UsockCliIpc_unlock();
        return E_SYS;
    }

    bIsOpened = TRUE;

    UsockCliIpc_unlock();
    //----------unlock----------

    DBG_FUNC_END("[API]\r\n");
#endif
    return E_OK;
}

ER UsockCliIpc_Close(void)
{
#if (USE_IPC)
    int iRet_UsockApi = 0;
    DBG_FUNC_BEGIN("[API]\r\n");

    //----------lock----------
    UsockCliIpc_lock();

    if(FALSE == bIsOpened)
    {
        DBG_WRN("is closed\r\n");
        UsockCliIpc_unlock();
        return E_OK;
    }

    if(E_OK != UsockCliIpc_Msg_SendCmd(USOCKETCLI_CMDID_UNINIT, &iRet_UsockApi) || USOCKETCLI_RET_OK != iRet_UsockApi)
    {
        DBG_ERR("UUsockCliIpc_Msg_SendCmd(USOCKETCLI_CMDID_UNINIT) ERR\r\n");
        UsockCliIpc_unlock();
        return E_SYS;
    }

    ter_tsk(USOCKCLIIPC_TSK_ID);

    if(E_OK != UsockCliIpc_Msg_Uninit())
    {
        DBG_ERR("UsockCliIpc_Msg_Uninit ERR\r\n");
        UsockCliIpc_unlock();
        return E_SYS;
    }

    bIsOpened = FALSE;

    UsockCliIpc_unlock();
    //----------unlock----------

    DBG_FUNC_END("[API]\r\n");
#endif
    return E_OK;
}

ER UsockCliIpc_Start(void)
{
#if (USE_IPC)

    int iRet_UsockApi = 0;
    int result = 0;

    CHKPNT;
    if(!bIsOpened)
        return USOCKETCLI_RET_NOT_OPEN;

    UsockCliIpc_lock();
    result = UsockCliIpc_Msg_SendCmd(USOCKETCLI_CMDID_START, &iRet_UsockApi);
    UsockCliIpc_unlock();

    if(result == E_OK)
        return iRet_UsockApi;
    else
        return USOCKETCLI_RET_ERR;
#else

    usocket_cli_start();
    return USOCKETCLI_RET_OK;

#endif
}

INT32 UsockCliIpc_Connect(usocket_cli_obj*  pObj)
{
#if (USE_IPC)

    USOCKETCLI_PARAM_PARAM *UsockApiParam = 0;
    int iRet_UsockApi = 0;
    int result = 0;

    if(!bIsOpened)
        return 0;

    if(!pObj)
        return 0;

    UsockCliIpc_lock();
    UsockApiParam = (USOCKETCLI_PARAM_PARAM *)UsockCliIpc_getSndAddr();
    memcpy(UsockApiParam,pObj,sizeof(usocket_cli_obj));
	if (pObj->notify) {
		pNotifyCB = pObj->notify;
	}
	if (pObj->recv) {
		pRecvCB = pObj->recv;
	}
    result = UsockCliIpc_Msg_SendCmd(USOCKETCLI_CMDID_CONNECT, &iRet_UsockApi);
    UsockCliIpc_unlock();

    if(result == E_OK)
        return (INT32)iRet_UsockApi;
    else
        return 0;
#else

    usocket_cli_obj *pNewObj = 0;
    int result = 0;


    pNewObj = usocket_cli_get_newobj();
    if(pNewObj)
    {
        usocket_cli_install(pNewObj,(usocket_cli_obj *)pObj);
        DBG_IND("(portNum:%d, sockbufSize:%d,client_socket:%d)\r\n",pObj->portNum,pObj->sockbufSize,pObj->connect_socket);

        result = usocket_cli_connect(pNewObj);
        if(result==0)
        {
            return (int)pNewObj;
        }
        else
        {
            usocket_cli_uninstall(pNewObj);
            return 0;
        }
    }
    else
    {
        return 0;
    }

#endif

}

ER UsockCliIpc_Send(INT32 handle,char* addr, int* size)
{
#if (USE_IPC)

    USOCKETCLI_TRANSFER_PARAM *UsockApiParam = 0;
    int iRet_UsockApi = 0;
    int result = 0;

    if(!bIsOpened)
        return USOCKETCLI_RET_NOT_OPEN;

    if(!handle)
    {
        DBG_ERR("no handle %d\r\n",handle);
        return USOCKETCLI_RET_ERR_PAR;
    }

    if(*size > USOCKCLIIPC_TRANSFER_BUF_SIZE)
    {
        DBG_ERR("size %d > IPC buffer %d\r\n",*size,USOCKCLIIPC_TRANSFER_BUF_SIZE);
        return USOCKETCLI_RET_ERR;
    }

    UsockCliIpc_lock();
    UsockApiParam = (USOCKETCLI_TRANSFER_PARAM *)UsockCliIpc_getSndAddr();
    UsockApiParam->obj = handle;
    UsockApiParam->size = *size;
    memcpy(UsockApiParam->buf,addr,*size);
    result = UsockCliIpc_Msg_SendCmd(USOCKETCLI_CMDID_SEND, &iRet_UsockApi);
    *size = UsockApiParam->size;
    UsockCliIpc_unlock();

    if(result == E_OK)
        return iRet_UsockApi;
    else
        return USOCKETCLI_RET_ERR;
#else
    return usocket_cli_send((usocket_cli_obj*)handle,addr,size);
#endif

}

ER UsockCliIpc_Disconnect(INT32 *handle)
{
#if (USE_IPC)

    USOCKETCLI_PARAM_PARAM *UsockApiParam = 0;
    int iRet_UsockApi = 0;
    int result = 0;

    if(!bIsOpened)
        return USOCKETCLI_RET_NOT_OPEN;

    if(!handle)
        return USOCKETCLI_RET_ERR_PAR;

    UsockCliIpc_lock();
    UsockApiParam = (USOCKETCLI_PARAM_PARAM *)UsockCliIpc_getSndAddr();
    UsockApiParam->param1 = *handle;
    result = UsockCliIpc_Msg_SendCmd(USOCKETCLI_CMDID_DISCONNECT, &iRet_UsockApi);
    *handle = 0;
    UsockCliIpc_unlock();

    if(result == E_OK)
        return iRet_UsockApi;
    else
        return USOCKETCLI_RET_ERR;
#else
    return E_OK;
#endif

}

ER UsockCliIpc_Stop(void)
{
#if (USE_IPC)

    int iRet_UsockApi = 0;
    int result = 0;

    if(!bIsOpened)
        return USOCKETCLI_RET_NOT_OPEN;

    UsockCliIpc_lock();
    result = UsockCliIpc_Msg_SendCmd(USOCKETCLI_CMDID_STOP, &iRet_UsockApi);
    UsockCliIpc_unlock();

    if(result == E_OK)
        return iRet_UsockApi;
    else
        return USOCKETCLI_RET_ERR;
#else
    usocket_cli_stop();
    return USOCKETCLI_RET_OK;
#endif

}

INT32 UsockCliIpc_CmdId_Notify(void)
{
#if (USE_IPC)

    USOCKETCLI_PARAM_PARAM *UsockApiParam = (USOCKETCLI_PARAM_PARAM *)UsockCliIpc_getRcvAddr();

    if(pNotifyCB)
    {
        pNotifyCB(UsockApiParam->param2, UsockApiParam->param3);
        return USOCKETCLI_RET_OK;
    }
    else
    {
        DBG_ERR("no pNotifyCB\r\n");
        return USOCKETCLI_RET_ERR;
    }
#else
    return E_OK;
#endif

}

INT32 UsockCliIpc_CmdId_Receive(void)
{
#if (USE_IPC)

    USOCKETCLI_TRANSFER_PARAM *UsockApiParam = (USOCKETCLI_TRANSFER_PARAM *)UsockCliIpc_getRcvAddr();

    if(pRecvCB)
    {
        pRecvCB(UsockApiParam->buf, UsockApiParam->size);
        return USOCKETCLI_RET_OK;
    }
    else
    {
        DBG_ERR("no pRecvCB\r\n");
        return USOCKETCLI_RET_ERR;
    }
#else
    return E_OK;
#endif

}

