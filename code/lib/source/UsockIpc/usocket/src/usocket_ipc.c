#if defined(__ECOS)
#include <cyg/nvtipc/NvtIpcAPI.h>
#else
#include <stdlib.h>
#include <string.h>
#include <nvtipc.h>
#include <nvtmem.h>
#include <pthread.h>
#include <unistd.h>
#endif
#include "usocket_int.h"
#include "usocket_ipc.h"
#include "usocket.h"
#if defined(__ECOS)
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_cache.h>
#include <cyg/kernel/kapi.h>
#include <stdlib.h>
#endif

#define USOCKET_SEND_TARGET          NVTIPC_SENDTO_CORE1
#define USOCKET_INVALID_MSGID        (NVTIPC_MSG_QUEUE_NUM+1)
#define USOCKET_INVALID_PARAMADDR    0x0

#ifdef USOCKET_SIM
static NVTIPC_U32   gIPC_MsgId_uITRON = 1;
static NVTIPC_U32   gIPC_MsgId_eCos = 2;
#endif //#ifdef USOCKET_SIM

static int usocket_ipc_wait_ack(unsigned int id);
static void usocket_ipc_set_ack(unsigned int id,int retValue);
static int usocket_ipc_send_ack(unsigned int id,int AckValue);
static int usocket_ipc_wait_cmd(unsigned int id,USOCKET_MSG *pRcvMsg);
static int usocket_ipc_init(unsigned int id,USOCKET_OPEN_OBJ* pObj);
static int usocket_ipc_uninit(unsigned int id);
static int usocket_ipc_exeapi(unsigned int id,USOCKET_CMDID CmdId, int *pOutRet);




/* ================================================================= */
/* Thread stacks, etc.
 */
#define USOCKET_THREAD_PRIORITY   6
#define USOCKET_THREAD_STACK_SIZE (4*1024)
#define USOCKET_BUFFER_SIZE       512
// flag define

#define CYG_FLG_USOCKETIPC_START       0x01
#define CYG_FLG_USOCKETIPC_IDLE        0x02
#define CYG_FLG_USOCKETIPC_ACK         0x04


#if defined(__ECOS)
static cyg_uint8 usocket_ipc_stacks[USOCKIPC_MAX_NUM][USOCKET_BUFFER_SIZE+
                              USOCKET_THREAD_STACK_SIZE] = {0};
static cyg_handle_t usocket_ipc_thread[USOCKIPC_MAX_NUM];
static cyg_thread   usocket_ipc_thread_object[USOCKIPC_MAX_NUM];
static cyg_flag_t usocket_ipc_flag[USOCKIPC_MAX_NUM];
#else
static volatile int usocket_ipc_flag;
#endif
static void usocket_ipc_receive_thread_0(cyg_addrword_t arg);
static void usocket_ipc_receive_thread_1(cyg_addrword_t arg);
static void usocket_ipc_receive_thread(USOCKIPC_ID id,cyg_addrword_t arg);

//int id =USOCKIPC_ID_0;
static NVTIPC_U32   gIPC_MsgId[USOCKIPC_MAX_NUM] = {USOCKET_INVALID_MSGID,USOCKET_INVALID_MSGID};
static unsigned int gRcvParamAddr[USOCKIPC_MAX_NUM] = {USOCKET_INVALID_PARAMADDR,USOCKET_INVALID_PARAMADDR};//Local for eCos, non-cached address
static unsigned int gSndParamAddr[USOCKIPC_MAX_NUM] = {USOCKET_INVALID_PARAMADDR,USOCKET_INVALID_PARAMADDR};//Local for eCos, non-cached address
static unsigned int gRetValue[USOCKIPC_MAX_NUM] = {0};
static bool         gIsOpened[USOCKIPC_MAX_NUM] = {false,false};
static cyg_thread_entry_t *gThread[USOCKIPC_MAX_NUM] = {usocket_ipc_receive_thread_0,usocket_ipc_receive_thread_1};


typedef int (*USOCKET_CMDID_FP)(USOCKIPC_ID id);

typedef struct{
    USOCKET_CMDID CmdId;
    USOCKET_CMDID_FP pFunc;
}USOCKET_CMDID_SET;

static int usocket_ipc_CmdId_Test(USOCKIPC_ID id);
static int usocket_ipc_CmdId_Open(USOCKIPC_ID id);
static int usocket_ipc_CmdId_Close(USOCKIPC_ID id);
static int usocket_ipc_CmdId_Send(USOCKIPC_ID id);
static int usocket_ipc_CmdId_Udp_Open(USOCKIPC_ID id);
static int usocket_ipc_CmdId_Udp_Close(USOCKIPC_ID id);
static int usocket_ipc_CmdId_Udp_Send(USOCKIPC_ID id);
static int usocket_ipc_CmdId_Udp_Sendto(USOCKIPC_ID id);
static void usocket_ipc_notify(USOCKIPC_ID id,int status,int parm);
static int usocket_ipc_recv(USOCKIPC_ID id,char* addr, int size);
static void usocket_ipc_udp_notify(int status,int parm);
static int usocket_ipc_udp_recv(char* addr, int size);

static const USOCKET_CMDID_SET gCmdFpTbl[USOCKET_CMDID_MAX]={
{USOCKET_CMDID_GET_VER_INFO, NULL},
{USOCKET_CMDID_GET_BUILD_DATE, NULL},
{USOCKET_CMDID_TEST, usocket_ipc_CmdId_Test},
{USOCKET_CMDID_OPEN, usocket_ipc_CmdId_Open},
{USOCKET_CMDID_CLOSE, usocket_ipc_CmdId_Close},
{USOCKET_CMDID_SEND, usocket_ipc_CmdId_Send},
{USOCKET_CMDID_RCV, NULL},
{USOCKET_CMDID_NOTIFY, NULL},
{USOCKET_CMDID_UDP_OPEN, usocket_ipc_CmdId_Udp_Open},
{USOCKET_CMDID_UDP_CLOSE, usocket_ipc_CmdId_Udp_Close},
{USOCKET_CMDID_UDP_SEND, usocket_ipc_CmdId_Udp_Send},
{USOCKET_CMDID_UDP_RCV, NULL},
{USOCKET_CMDID_UDP_NOTIFY, NULL},
{USOCKET_CMDID_SYSREQ_ACK, NULL},
{USOCKET_CMDID_UNINIT, NULL},
{USOCKET_CMDID_UDP_SENDTO, usocket_ipc_CmdId_Udp_Sendto},

};


//private functions
static int USOCKETECOS_Open(unsigned int id,USOCKET_OPEN_OBJ* pObj);
static int USOCKETECOS_Close(unsigned int id);

//-------------------------------------------------------------------------
//General global variables
//-------------------------------------------------------------------------


unsigned int usocket_ipc_GetSndAddr(USOCKIPC_ID id)
{

    if(*(unsigned int *)(gSndParamAddr[id]) == USOCKETIPC_BUF_TAG)
    {
        //USOCKET_PRINT(id,"%x %x \r\n",gSndParamAddr[id],*(unsigned int *)(gSndParamAddr[id]));
        return (gSndParamAddr[id]+USOCKETIPC_BUF_CHK_SIZE);
    }
    else
    {
        USOCKET_ERR(id,"buf chk fail\r\n");
        return USOCKET_RET_ERR;
    }
}
unsigned int usocket_ipc_GetRcvAddr(USOCKIPC_ID id)
{

    if(*(unsigned int *)(gRcvParamAddr[id]) == USOCKETIPC_BUF_TAG)
    {
        //USOCKET_PRINT(id,"%x %x \r\n",gRcvParamAddr[id],*(unsigned int *)(gRcvParamAddr[id]));
        return (gRcvParamAddr[id]+USOCKETIPC_BUF_CHK_SIZE);
    }
    else
    {
        USOCKET_ERR(id,"buf chk fail\r\n");
        return USOCKET_RET_ERR;
    }
}

static void usocket_ipc_notify_0(int status,int parm)
{
    usocket_ipc_notify(USOCKIPC_ID_0,status,parm);
}
static void usocket_ipc_notify_1(int status,int parm,int obj_index)
{
    usocket_ipc_notify(obj_index,status,parm);
}
static int usocket_ipc_recv_0(char* addr, int size)
{
    return usocket_ipc_recv(USOCKIPC_ID_0,addr,size);
}
static int usocket_ipc_recv_1(char* addr, int size,int obj_index)
{
    return usocket_ipc_recv(obj_index,addr,size);
}
static void usocket_ipc_notify(USOCKIPC_ID id,int status,int parm)
{
    USOCKET_PARAM_PARAM *pParam = (USOCKET_PARAM_PARAM *)usocket_ipc_GetSndAddr(id);
    int result =0;
    int Ret_UsocketApi = 0;

    if((int)pParam==USOCKET_RET_ERR)
    {
        USOCKET_ERR(id,"pParam err\r\n");
        return ;
    }
    pParam->param1 = (int)status;
    pParam->param2 = (int)parm;

    USOCKET_PRINT(id,"(%d, %d)\r\n",pParam->param1, pParam->param2);

    result = usocket_ipc_exeapi(id,USOCKET_CMDID_NOTIFY,&Ret_UsocketApi);

    if((result!=USOCKET_RET_OK)||(Ret_UsocketApi!=0))
    {
        USOCKET_PRINT(id,"snd CB fail\r\n");
    }

}

static int usocket_ipc_recv(USOCKIPC_ID id,char* addr, int size)
{
    USOCKET_TRANSFER_PARAM *pParam = (USOCKET_TRANSFER_PARAM *)usocket_ipc_GetSndAddr(id);
    int result =0;
    int Ret_UsocketApi = 0;

    USOCKET_PRINT(id,"%x %d\r\n  ",addr,size);

    if((int)pParam==USOCKET_RET_ERR)
    {
        USOCKET_ERR(id,"pParam err\r\n");
        return USOCKET_RET_ERR;
    }

    pParam->size = (int)size;

    if(size > USOCKIPC_TRANSFER_BUF_SIZE)
    {
        USOCKET_ERR(id,"size %d > %d\r\n",size,USOCKIPC_TRANSFER_BUF_SIZE);
        return USOCKET_RET_ERR;
    }

    USOCKET_PRINT(id,"%x %d cp to buf\r\n  ",addr,size);

    memcpy((void *)pParam->buf,addr,size);

    result = usocket_ipc_exeapi(id,USOCKET_CMDID_RCV,&Ret_UsocketApi);

    if((result!=USOCKET_RET_OK)||(Ret_UsocketApi!=0))
    {
        USOCKET_PRINT(id,"snd CB fail\r\n");
    }

    return result;
}

static void usocket_ipc_udp_notify(int status,int parm)
{
    unsigned int id = USOCKIPC_ID_0; //only 1
    USOCKET_PARAM_PARAM *pParam = (USOCKET_PARAM_PARAM *)usocket_ipc_GetSndAddr(id);
    int result =0;
    int Ret_UsocketApi = 0;

    if((int)pParam==USOCKET_RET_ERR)
    {
        USOCKET_ERR(id,"pParam err\r\n");
        return ;
    }

    pParam->param1 = (int)status;
    pParam->param2 = (int)parm;

    USOCKET_PRINT(id,"(%d, %d)\r\n",pParam->param1, pParam->param2);

    result = usocket_ipc_exeapi(USOCKIPC_ID_0,USOCKET_CMDID_UDP_NOTIFY,&Ret_UsocketApi);

    if((result!=USOCKET_RET_OK)||(Ret_UsocketApi!=0))
    {
        USOCKET_PRINT(id,"snd CB fail\r\n");
    }

}

static int usocket_ipc_udp_recv(char* addr, int size)
{
    unsigned int id = USOCKIPC_ID_0; //only 1
    USOCKET_TRANSFER_PARAM *pParam = (USOCKET_TRANSFER_PARAM *)usocket_ipc_GetSndAddr(id);
    int result =0;
    int Ret_UsocketApi = 0;

    USOCKET_PRINT(id,"%x %d\r\n  ",addr,size);

    if((int)pParam==USOCKET_RET_ERR)
    {
        USOCKET_ERR(id,"pParam err\r\n");
        return USOCKET_RET_ERR;
    }

    pParam->size = (int)size;
    if(size > USOCKIPC_TRANSFER_BUF_SIZE)
    {
        USOCKET_ERR(id,"size %d > %d\r\n",size,USOCKIPC_TRANSFER_BUF_SIZE);
        return USOCKET_RET_ERR;
    }
    else
    {
        memcpy(pParam->buf,addr,size);
    }



    result = usocket_ipc_exeapi(id,USOCKET_CMDID_UDP_RCV,&Ret_UsocketApi);

    if((result!=USOCKET_RET_OK)||(Ret_UsocketApi!=0))
    {
        USOCKET_PRINT(id,"snd CB fail\r\n");
    }

    return result;
}



#if defined(__ECOS)
static void usocket_ipc_receive_thread_0(cyg_addrword_t arg)
{
    usocket_ipc_receive_thread(USOCKIPC_ID_0,arg);
}
static void usocket_ipc_receive_thread_1(cyg_addrword_t arg)
{
    usocket_ipc_receive_thread(USOCKIPC_ID_1,arg);
}
static void usocket_ipc_receive_thread(USOCKIPC_ID id,cyg_addrword_t arg)
#else
static void* usocket_ipc_receive_thread(void* arg)
#endif
{
    USOCKET_MSG RcvMsg;
    int Ret_ExeResult = USOCKET_RET_ERR;
    int Ret_IpcMsg;

    cyg_flag_maskbits(&usocket_ipc_flag[id], 0);
    while(1)
    {
        //1. receive ipc cmd from uItron
        Ret_IpcMsg = usocket_ipc_wait_cmd(id,&RcvMsg);
        if(USOCKET_RET_OK != Ret_IpcMsg)
        {
            USOCKET_ERR(id,"usocket_ipc_waitcmd = %d\r\n", Ret_IpcMsg);
            continue;
        }

        //2. error check
        USOCKET_PRINT(id,"Got RcvCmdId = %d\r\n",RcvMsg.CmdId);
        if(RcvMsg.CmdId >= USOCKET_CMDID_MAX || RcvMsg.CmdId < 0)
        {
            USOCKET_ERR(id,"RcvCmdId(%d) should be 0~%d\r\n", RcvMsg.CmdId, USOCKET_CMDID_MAX);
            continue;
        }

        if(gCmdFpTbl[RcvMsg.CmdId].CmdId != RcvMsg.CmdId)
        {
            USOCKET_ERR(id,"RcvCmdId = %d, TableCmdId = %d\r\n", RcvMsg.CmdId, gCmdFpTbl[RcvMsg.CmdId].CmdId);
            continue;
        }

        //3. process the cmd
        if(RcvMsg.CmdId==USOCKET_CMDID_SYSREQ_ACK)
        {
            usocket_ipc_set_ack(id,RcvMsg.Arg);
        }
        else if(RcvMsg.CmdId==USOCKET_CMDID_UNINIT){
            USOCKETECOS_Close(id);
            return DUMMY_NULL;
        }
        else
        {
            if(gCmdFpTbl[RcvMsg.CmdId].pFunc)
            {
                Ret_ExeResult = gCmdFpTbl[RcvMsg.CmdId].pFunc(id);
                USOCKET_PRINT(id,"[TSK]Ret_ExeResult = 0x%X\r\n", Ret_ExeResult);
            }
            else
            {
                Ret_ExeResult = USOCKET_RET_NO_FUNC;
                USOCKET_ERR(id,"[TSK]Null Cmd pFunc id %d\r\n",RcvMsg.CmdId);
                //usocket_ipc_CmdId_Test();
            }

            //send the msg of the return value of FileSys API
            Ret_IpcMsg = usocket_ipc_send_ack(id,Ret_ExeResult);
            if(USOCKET_RET_OK != Ret_IpcMsg)
            {
                USOCKET_ERR(id,"usocket_ipc_Msg_WaitCmd = %d\r\n", Ret_IpcMsg);
                continue;
            }
        }
    }

    return DUMMY_NULL;
}

static int usocket_ipc_CmdId_Test(USOCKIPC_ID id)
{
    #if 0
    USOCKET_PARAM_PARAM *pParam = (USOCKET_PARAM_PARAM *)usocket_ipc_GetRcvAddr(id);

    USOCKET_PRINT(id,"(0x%X, 0x%X, 0x%X)\r\n",pParam->param1, pParam->param2, pParam->param3);
    #endif

    return USOCKET_RET_OK;
}
static int usocket_ipc_CmdId_Open(USOCKIPC_ID id)
{
    USOCKET_PARAM_PARAM *pParam = (USOCKET_PARAM_PARAM *)usocket_ipc_GetRcvAddr(id);
    if(id ==USOCKIPC_ID_0) {
        usocket_install_obj *pInstallObj = (usocket_install_obj*)pParam;
        if(pInstallObj->notify)
            pInstallObj->notify = usocket_ipc_notify_0;
        if(pInstallObj->recv)
            pInstallObj->recv = usocket_ipc_recv_0;

        usocket_install(pInstallObj);

        usocket_open();

        USOCKET_PRINT(id,"(portNum:%d, threadPriority:%d, sockbufSize:%d,client_socket:%d)\r\n",pInstallObj->portNum,pInstallObj->threadPriority,pInstallObj->sockbufSize,pInstallObj->client_socket);

    } else if(id ==USOCKIPC_ID_1) {
        usocket_install_obj_multi *pInstallObj = (usocket_install_obj_multi*)pParam;
        unsigned int multi_id =0;
        if(pInstallObj->notify)
            pInstallObj->notify = usocket_ipc_notify_1;
        if(pInstallObj->recv)
            pInstallObj->recv = usocket_ipc_recv_1;

        multi_id = usocket_install_multi(pInstallObj);

        if(multi_id!=id)
            USOCKET_ERR(id,"open id not match %d %d\n",multi_id,id);

        usocket_open_multi(multi_id);

        USOCKET_PRINT(id,"multi_id %d portNum:%d, threadPriority:%d, sockbufSize:%d\r\n",multi_id,pInstallObj->portNum,pInstallObj->threadPriority,pInstallObj->sockbufSize);

    }


    return USOCKET_RET_OK;

}
static int usocket_ipc_CmdId_Close(USOCKIPC_ID id)
{
    usocket_ipc_set_ack(id,USOCKET_RET_FORCE_ACK);
    if(id==USOCKIPC_ID_0){
        usocket_uninstall();
        usocket_close();
    } else {
        usocket_uninstall_multi(id);
        usocket_close_multi(id);
    }
    return USOCKET_RET_OK;

}
static int usocket_ipc_CmdId_Send(USOCKIPC_ID id)
{
    USOCKET_TRANSFER_PARAM *pParam = (USOCKET_TRANSFER_PARAM *)usocket_ipc_GetRcvAddr(id);
    if(id==USOCKIPC_ID_0) {
        return usocket_send(pParam->buf,&(pParam->size));
    } else {
        return usocket_send_multi(pParam->buf,&(pParam->size),id);
    }

}

static int usocket_ipc_CmdId_Udp_Open(USOCKIPC_ID id)
{
    USOCKET_PARAM_PARAM *pParam = (USOCKET_PARAM_PARAM *)usocket_ipc_GetRcvAddr(id);
    usocket_install_obj *pInstallObj = (usocket_install_obj*)pParam;

    if(pInstallObj->notify)
        pInstallObj->notify = usocket_ipc_udp_notify;
    if(pInstallObj->recv)
        pInstallObj->recv = usocket_ipc_udp_recv;

    USOCKET_PRINT(id,"(portNum:%d, threadPriority:%d, sockbufSize:%d,client_socket:%d)\r\n",pInstallObj->portNum,pInstallObj->threadPriority,pInstallObj->sockbufSize,pInstallObj->client_socket);

    usocket_udp_install(pInstallObj);

    usocket_udp_open();

    return USOCKET_RET_OK;

}
static int usocket_ipc_CmdId_Udp_Close(USOCKIPC_ID id)
{
    usocket_ipc_set_ack(id,USOCKET_RET_FORCE_ACK);
    usocket_udp_uninstall();
    usocket_udp_close();
    return USOCKET_RET_OK;
}
static int usocket_ipc_CmdId_Udp_Send(USOCKIPC_ID id)
{
    USOCKET_TRANSFER_PARAM *pParam = (USOCKET_TRANSFER_PARAM *)usocket_ipc_GetRcvAddr(id);

    return usocket_udp_send(pParam->buf,&(pParam->size));
}
static int usocket_ipc_CmdId_Udp_Sendto(USOCKIPC_ID id)
{
    USOCKET_SENDTO_PARAM *pParam = (USOCKET_SENDTO_PARAM *)usocket_ipc_GetRcvAddr(id);

    return usocket_udp_sendto(pParam->dest_ip,pParam->dest_port,pParam->buf,&(pParam->size));
}

int usocket_ipc_init(unsigned int id,USOCKET_OPEN_OBJ* pObj)
{
    NVTIPC_I32 Ret_NvtIpc;

    //set the Receive parameter address for IPC
    gRcvParamAddr[id] = (ipc_getNonCacheAddr(pObj->RcvParamAddr)) ;
    if(USOCKET_INVALID_PARAMADDR == gRcvParamAddr[id])
    {
        USOCKET_ERR(id,"gRcvParamAddr[id] = 0x%X\n", gRcvParamAddr[id]);
        return USOCKET_RET_ERR;
    }

    //set the Send parameter address for IPC
    gSndParamAddr[id] = (ipc_getNonCacheAddr(pObj->SndParamAddr)) ;
    if(USOCKET_INVALID_PARAMADDR == gSndParamAddr[id])
    {
        USOCKET_ERR(id,"gSndParamAddr[id] = 0x%X\n", gSndParamAddr[id]);
        return USOCKET_RET_ERR;
    }

    #ifdef USOCKET_SIM
    Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok("WIFIIPC eCos"));
    #else

    if(gIPC_MsgId[id] == USOCKET_INVALID_MSGID){

        if(id ==USOCKIPC_ID_0)
            Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(USOCKET_TOKEN_PATH0));
        else
            Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(USOCKET_TOKEN_PATH1));

    } else {
        Ret_NvtIpc = gIPC_MsgId[id];
    }
    #endif

    if(Ret_NvtIpc < 0)
    {
        USOCKET_ERR(id,"NvtIPC_MsgGet = %d\n", Ret_NvtIpc);
        return USOCKET_RET_ERR;
    }

    gIPC_MsgId[id] = (NVTIPC_U32)Ret_NvtIpc;
    USOCKET_PRINT(id,"gIPC_MsgId[id] = %d\n", gIPC_MsgId[id]);

    #ifdef USOCKET_SIM
    if(gIPC_MsgId[id] != gIPC_MsgId_eCos)
    {
        USOCKET_ERR(id,"Simulation environment error\n");
        return USOCKET_RET_ERR;
    }
    #endif

    cyg_flag_init(&usocket_ipc_flag[id]);

    #if defined(__ECOS)
    /* Create a main thread for receive message */
    cyg_thread_create( 5,//usocket_ipc_obj.threadPriority,
                       gThread[id],
                       0,
                       "user socket ipc",
                       &usocket_ipc_stacks[id][0],
                       sizeof(usocket_ipc_stacks[id]),
                       &usocket_ipc_thread[id],
                       &usocket_ipc_thread_object[id]
        );

    cyg_thread_resume(usocket_ipc_thread[id]);  /* Start it */
    #endif
    return USOCKET_RET_OK;
}

int usocket_ipc_uninit(unsigned int id)
{
    NVTIPC_I32 Ret_NvtIpc;
    cyg_flag_destroy(&usocket_ipc_flag[id]);

    #ifdef USOCKET_SIM
    gIPC_MsgId[id] = gIPC_MsgId_eCos;
    #endif

    Ret_NvtIpc = NvtIPC_MsgRel(gIPC_MsgId[id]);
    if(Ret_NvtIpc < 0)
    {
        USOCKET_ERR(id,"NvtIPC_MsgRel = %d\n", Ret_NvtIpc);
        return USOCKET_RET_ERR;
    }

    gIPC_MsgId[id] = USOCKET_INVALID_MSGID;
    gRcvParamAddr[id] = USOCKET_INVALID_PARAMADDR;

    #if defined(__ECOS)
    cyg_thread_exit();
    #endif

    return USOCKET_RET_OK;
}

int usocket_ipc_wait_ack(unsigned int id)
{
    cyg_flag_wait(&usocket_ipc_flag[id],CYG_FLG_USOCKETIPC_ACK,CYG_FLAG_WAITMODE_OR|CYG_FLAG_WAITMODE_CLR);
    return gRetValue[id];
}

void usocket_ipc_set_ack(unsigned int id,int retValue)
{
    gRetValue[id] = retValue;
    cyg_flag_setbits(&usocket_ipc_flag[id], CYG_FLG_USOCKETIPC_ACK);
}

int usocket_ipc_exeapi(unsigned int id,USOCKET_CMDID CmdId, int *pOutRet)
{
    USOCKET_MSG IpcMsg = {0};
    NVTIPC_I32  Ret_NvtIpc;

    if(USOCKET_INVALID_MSGID == gIPC_MsgId[id])
    {
        USOCKET_ERR(id,"USOCKET_INVALID_MSGID\n");
        return USOCKET_RET_ERR;
    }

    IpcMsg.CmdId = CmdId;
    IpcMsg.Arg = 0;

    #ifdef USOCKET_SIM
    gIPC_MsgId[id] = gIPC_MsgId_uITRON;
    #endif

    USOCKET_PRINT(id,"Send CmdId = %d\n", CmdId);
    Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId[id], USOCKET_SEND_TARGET, &IpcMsg, sizeof(IpcMsg));
    if(Ret_NvtIpc < 0)
    {
        USOCKET_ERR(id,"NvtIPC_MsgSnd = %d\n", Ret_NvtIpc);
        return USOCKET_RET_ERR;
    }
    *pOutRet = usocket_ipc_wait_ack(id);

    return USOCKET_RET_OK;
}

//Ack the API caller with Ack CmdId and Ack Arg (if no Arg to be sent, please set Arg to 0)
int usocket_ipc_send_ack(unsigned int id,int AckValue)
{
    USOCKET_MSG IpcMsg = {0};
    NVTIPC_I32  Ret_NvtIpc;

    if(USOCKET_INVALID_MSGID == gIPC_MsgId[id])
    {
        USOCKET_ERR(id,"USOCKET_INVALID_MSGID\n");
        return USOCKET_RET_ERR;
    }

    IpcMsg.CmdId = USOCKET_CMDID_SYSREQ_ACK;
    IpcMsg.Arg = AckValue;

    #ifdef USOCKET_SIM
    gIPC_MsgId[id] = gIPC_MsgId_uITRON;
    #endif

    Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId[id], USOCKET_SEND_TARGET, &IpcMsg, sizeof(IpcMsg));
    if(Ret_NvtIpc < 0)
    {
        USOCKET_ERR(id,"NvtIPC_MsgSnd = %d\n", Ret_NvtIpc);
        return USOCKET_RET_ERR;
    }

    return USOCKET_RET_OK;
}
int usocket_ipc_wait_cmd(unsigned int id,USOCKET_MSG *pRcvMsg)
{
    NVTIPC_I32  Ret_NvtIpc;

#ifdef USOCKET_SIM
    gIPC_MsgId[id] = gIPC_MsgId_eCos;
#endif

    Ret_NvtIpc = NvtIPC_MsgRcv(gIPC_MsgId[id], pRcvMsg, sizeof(USOCKET_MSG));
    if(Ret_NvtIpc < 0)
    {
        USOCKET_ERR(id,"NvtIPC_MsgRcv = %d\r\n", Ret_NvtIpc);
        return USOCKET_RET_ERR;
    }

    return USOCKET_RET_OK;
}


//==========================================================================

#define USOCKET_MACRO_START do {
#define USOCKET_MACRO_END   } while (0)

#define USOCKET_API_RET(id,RetValue)         \
USOCKET_MACRO_START                       \
    usocket_ipc_send_ack(id,RetValue);     \
    return RetValue;                    \
USOCKET_MACRO_END
#define USOCKET_API_RET_UNINIT(id,RetValue)  \
USOCKET_MACRO_START                       \
    usocket_ipc_send_ack(id,RetValue);     \
    usocket_ipc_uninit(id);                 \
    return RetValue;                    \
USOCKET_MACRO_END
//-------------------------------------------------------------------------
//Implementation of USOCKETECOS API
//-------------------------------------------------------------------------
static int USOCKETECOS_Open(unsigned int id,USOCKET_OPEN_OBJ* pObj)
{
    USOCKET_PRINT(id,"\n");

    if(gIsOpened[id])
    {
        USOCKET_ERR(id,"is opened\n");
        USOCKET_API_RET(id,USOCKET_RET_OPENED);
    }

    if(NULL == pObj)
    {
        USOCKET_ERR(id,"pObj = 0x%X\n", pObj);
        USOCKET_API_RET(id,USOCKET_RET_ERR);
    }

    //init the IPC communication
    if(USOCKET_RET_ERR == usocket_ipc_init(id,pObj))
    {
        USOCKET_ERR(id,"usocket_ipc_init failed\n");
        USOCKET_API_RET(id,USOCKET_RET_ERR);
    }
    gIsOpened[id] = true;
    USOCKET_API_RET(id,USOCKET_RET_OK);
}

static int USOCKETECOS_Close(unsigned int id)
{
    USOCKET_PRINT(id,"\n");

    gIsOpened[id]   = false;

    USOCKET_API_RET_UNINIT(id,USOCKET_RET_OK);
}


#if defined(__ECOS)
void USOCKETECOS_CmdLine(char *szCmd)
{
    char *delim = " ";
    char *pToken;

    USOCKET_PRINT(3,"szCmd = %s\n", szCmd);

    pToken = strtok(szCmd, delim);
    while (pToken != NULL)
    {
        if(!strcmp(pToken, "-open"))
        {
            USOCKET_OPEN_OBJ OpenObj;
            USOCKIPC_ID id=0;

            //get receive memory address
            pToken = strtok(NULL, delim);
            if(NULL == pToken)
            {
                USOCKET_ERR(id,"get rcv address fail\n");
                break;
            }
            OpenObj.RcvParamAddr = atoi(pToken);

            //get send memory address
            pToken = strtok(NULL, delim);
            if(NULL == pToken)
            {
                USOCKET_ERR(id,"get snd address fail\n");
                break;
            }
            OpenObj.SndParamAddr = atoi(pToken);

            pToken = strtok(NULL, delim);
            if(NULL == pToken)
            {
                USOCKET_ERR(id,"get ver key fail\n");
                break;
            }
            if(atoi(pToken)!=USOCKET_VER_KEY)
            {
                USOCKET_ERR(id,"ver key mismatch %d %d\n",USOCKET_VER_KEY,atoi(pToken));
                break;
            }
            pToken = strtok(NULL, delim);
            if(NULL == pToken)
            {
                USOCKET_ERR(id,"get id fail\n");
                break;
            }

            id = atoi(pToken);
            //open USOCKETECOS
            USOCKETECOS_Open(id,&OpenObj);
            break;
        }
        else if(!strcmp(pToken, "-close"))
        {
            USOCKIPC_ID id=0;
            pToken = strtok(NULL, delim);
            if(NULL == pToken)
            {
                USOCKET_ERR(id,"get id fail\n");
                break;
            }
            id = atoi(pToken);
            USOCKETECOS_Close(id);
            break;
        }

        pToken = strtok(NULL, delim);
    }
}
#else
int USOCKETECOS_CmdLine(char *szCmd, char *szRcvAddr, char *szSndAddr)
{
    USOCKET_PRINT(id,"szCmd = %s\n", szCmd);

    if(!strcmp(szCmd, "-open"))
    {
        USOCKET_OPEN_OBJ OpenObj;

        OpenObj.RcvParamAddr = atoi(szRcvAddr);
        OpenObj.SndParamAddr = atoi(szSndAddr);

        //open USOCKETECOS
        return USOCKETECOS_Open(id,&OpenObj);
    }
    else if(!strcmp(szCmd, "-close"))
    {
        return USOCKETECOS_Close(id);
    }

    return USOCKET_RET_ERR;
}
#endif
// -------------------------------------------------------------------------


#if defined(__LINUX_660)
int main(int argc, char **argv)
{
    if(argc < 4){
        USOCKET_ERR(id,"usocket:: incorrect main argc(%d)\n", argc);
        return -1;
    }
    USOCKET_PRINT(id,"szCmd = %s ; szRcvAddr is %s ; szSndAddr is %s\n", argv[1], argv[2], argv[3]);

    if(USOCKET_RET_OK != USOCKETECOS_CmdLine(argv[1], argv[2], argv[3])){
        USOCKET_ERR(id,"USOCKETECOS_CmdLinel() fail\n");
        return -1;
    }

    usocket_ipc_receive_thread(NULL);

    return 0;
}
#endif
