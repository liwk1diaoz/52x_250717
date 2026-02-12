#if defined(__ECOS)
#include <cyg/nvtipc/NvtIpcAPI.h>
#else
#include <stdlib.h>
#include <string.h>
#include <nvtipc.h>
//#include <nvtmem.h>
#include <pthread.h>
#include <unistd.h>
#endif
#if defined(__ECOS)
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_cache.h>
#include <cyg/kernel/kapi.h>
#include <stdlib.h>
#elif defined(__FREERTOS)
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#include <kwrap/semaphore.h>
#include <kwrap/task.h>
#endif
#include "ethsocket_int.h"
#include "EthCam/ethsocket_ipc.h"
#include "EthCam/ethsocket.h"

#define ETHSOCKET_SEND_TARGET               NVTIPC_SENDTO_CORE1
#define ETHSOCKET_INVALID_MSGID             (NVTIPC_MSG_QUEUE_NUM + 1)
#define ETHSOCKET_INVALID_PARAMADDR         0x0

#if (__USE_IPC)
#define ipc_getCacheAddr(addr)        	NvtIPC_GetCacheAddr(addr)
#define ipc_getNonCacheAddr(addr)   	NvtIPC_GetNonCacheAddr(addr)
#define ipc_flushCache(addr, size)   	NvtIPC_FlushCache(addr, size)
#else
#define ipc_getCacheAddr(addr)        addr
#define ipc_getNonCacheAddr(addr)   addr
#define ipc_flushCache(addr, size)
#endif


#ifdef ETHSOCKET_SIM
static NVTIPC_U32   gIPC_MsgId_uITRON = 1;
static NVTIPC_U32   gIPC_MsgId_eCos = 2;
#endif //#ifdef ETHSOCKET_SIM

int ethsocket_ipc_wait_ack(unsigned int id);
void ethsocket_ipc_set_ack(unsigned int id, int retValue);
int ethsocket_ipc_send_ack(unsigned int id, int AckValue);
int ethsocket_ipc_wait_cmd(unsigned int id, ETHSOCKET_MSG *pRcvMsg);
int ethsocket_ipc_init(unsigned int id, ETHSOCKET_OPEN_OBJ *pObj);
int ethsocket_ipc_uninit(unsigned int id);
int ethsocket_ipc_exeapi(unsigned int id, ETHSOCKET_CMDID CmdId, int *pOutRet);


/* ================================================================= */
/* Thread stacks, etc.
 */
#define ETHSOCKETIPC_THREAD_PRIORITY           4 //5
#define ETHSOCKETIPC_THREAD_STACK_SIZE         (4 * 1024)
#define ETHSOCKETIPC_BUFFER_SIZE               512
// flag define

#define CYG_FLG_ETHSOCKETIPC_START          0x01
#define CYG_FLG_ETHSOCKETIPC_IDLE           0x02
#define CYG_FLG_ETHSOCKETIPC_ACK            0x04
#if (__USE_IPC)

#if defined(__ECOS)
static cyg_uint8 ethsocket_ipc_stacks[ETHSOCKIPC_MAX_NUM][ETHSOCKETIPC_BUFFER_SIZE + ETHSOCKETIPC_THREAD_STACK_SIZE] = {0};
static cyg_handle_t ethsocket_ipc_thread[ETHSOCKIPC_MAX_NUM];
static cyg_thread   ethsocket_ipc_thread_object[ETHSOCKIPC_MAX_NUM];
static cyg_flag_t ethsocket_ipc_flag[ETHSOCKIPC_MAX_NUM];
#else
static pthread_t ethsocket_ipc_thread[ETHSOCKIPC_MAX_NUM];
static volatile int ethsocket_ipc_flag[ETHSOCKIPC_MAX_NUM];
#endif
#if defined(__ECOS)
static void ethsocket_ipc_receive_thread_0(cyg_addrword_t arg);
static void ethsocket_ipc_receive_thread_1(cyg_addrword_t arg);
static void ethsocket_ipc_receive_thread_2(cyg_addrword_t arg);
static void ethsocket_ipc_receive_thread(ETHSOCKIPC_ID id, cyg_addrword_t arg);
#else
static void* ethsocket_ipc_receive_thread_0(void* arg);
static void* ethsocket_ipc_receive_thread_1(void* arg);
static void* ethsocket_ipc_receive_thread_2(void* arg);
static void* ethsocket_ipc_receive_thread(ETHSOCKIPC_ID id, void* arg);
#endif
#endif
//int id =ETHSOCKIPC_ID_0;
#if (__USE_IPC)
static NVTIPC_U32   gIPC_MsgId[ETHSOCKIPC_MAX_NUM] = {ETHSOCKET_INVALID_MSGID, ETHSOCKET_INVALID_MSGID, ETHSOCKET_INVALID_MSGID};
#endif
static unsigned int gRcvParamAddr[ETHSOCKIPC_MAX_NUM] = {ETHSOCKET_INVALID_PARAMADDR, ETHSOCKET_INVALID_PARAMADDR, ETHSOCKET_INVALID_PARAMADDR}; //Local for eCos, non-cached address
static unsigned int gRcvParamSize[ETHSOCKIPC_MAX_NUM] = {ETHSOCKET_INVALID_PARAMADDR, ETHSOCKET_INVALID_PARAMADDR, ETHSOCKET_INVALID_PARAMADDR}; //Local for eCos, non-cached address
static unsigned int gSndParamAddr[ETHSOCKIPC_MAX_NUM] = {ETHSOCKET_INVALID_PARAMADDR, ETHSOCKET_INVALID_PARAMADDR, ETHSOCKET_INVALID_PARAMADDR}; //Local for eCos, non-cached address
static unsigned int gSndParamSize[ETHSOCKIPC_MAX_NUM] = {ETHSOCKET_INVALID_PARAMADDR, ETHSOCKET_INVALID_PARAMADDR, ETHSOCKET_INVALID_PARAMADDR}; //Local for eCos, non-cached address
static bool         gIsOpened[ETHSOCKIPC_MAX_NUM] = {false, false , false};

#if (__USE_IPC)
static unsigned int gRetValue[ETHSOCKIPC_MAX_NUM] = {0};
#if defined(__ECOS)
static cyg_thread_entry_t *gThread[ETHSOCKIPC_MAX_NUM] = {ethsocket_ipc_receive_thread_0, ethsocket_ipc_receive_thread_1, ethsocket_ipc_receive_thread_2};
#else
static void *gThread[ETHSOCKIPC_MAX_NUM] = {ethsocket_ipc_receive_thread_0, ethsocket_ipc_receive_thread_1, ethsocket_ipc_receive_thread_2};
#endif
#endif
typedef int (*ETHSOCKET_CMDID_FP)(ETHSOCKIPC_ID id);

typedef struct {
	ETHSOCKET_CMDID CmdId;
	ETHSOCKET_CMDID_FP pFunc;
} ETHSOCKET_CMDID_SET;

static int ethsocket_ipc_CmdId_Test(ETHSOCKIPC_ID id);
static int ethsocket_ipc_CmdId_Open(ETHSOCKIPC_ID id);
static int ethsocket_ipc_CmdId_Close(ETHSOCKIPC_ID id);
static int ethsocket_ipc_CmdId_Send(ETHSOCKIPC_ID id);
static int ethsocket_ipc_CmdId_Udp_Open(ETHSOCKIPC_ID id);
static int ethsocket_ipc_CmdId_Udp_Close(ETHSOCKIPC_ID id);
static int ethsocket_ipc_CmdId_Udp_Send(ETHSOCKIPC_ID id);
static int ethsocket_ipc_CmdId_Udp_Sendto(ETHSOCKIPC_ID id);
static void ethsocket_ipc_notify(ETHSOCKIPC_ID id, int status, int parm);
static int ethsocket_ipc_recv(ETHSOCKIPC_ID id, char *addr, int size);
static void ethsocket_ipc_udp_notify(int status, int parm);
static int ethsocket_ipc_udp_recv(char *addr, int size);
extern void EthsockIpc_HandleCmd(ETHSOCKET_CMDID CmdId, unsigned int id);

static const ETHSOCKET_CMDID_SET gCmdFpTbl[ETHSOCKET_CMDID_MAX] = {
	{ETHSOCKET_CMDID_GET_VER_INFO,          NULL                                },
	{ETHSOCKET_CMDID_GET_BUILD_DATE,        NULL                                },
	{ETHSOCKET_CMDID_TEST,                  ethsocket_ipc_CmdId_Test            },
	{ETHSOCKET_CMDID_OPEN,                  ethsocket_ipc_CmdId_Open            },
	{ETHSOCKET_CMDID_CLOSE,                 ethsocket_ipc_CmdId_Close           },
	{ETHSOCKET_CMDID_SEND,                  ethsocket_ipc_CmdId_Send            },
	{ETHSOCKET_CMDID_RCV,                   NULL                                },
	{ETHSOCKET_CMDID_NOTIFY,                NULL                                },
	{ETHSOCKET_CMDID_UDP_OPEN,              ethsocket_ipc_CmdId_Udp_Open        },
	{ETHSOCKET_CMDID_UDP_CLOSE,             ethsocket_ipc_CmdId_Udp_Close       },
	{ETHSOCKET_CMDID_UDP_SEND,              ethsocket_ipc_CmdId_Udp_Send        },
	{ETHSOCKET_CMDID_UDP_RCV,               NULL                                },
	{ETHSOCKET_CMDID_UDP_NOTIFY,            NULL                                },
	{ETHSOCKET_CMDID_SYSREQ_ACK,            NULL                                },
	{ETHSOCKET_CMDID_UNINIT,                NULL                                },
	{ETHSOCKET_CMDID_UDP_SENDTO,            ethsocket_ipc_CmdId_Udp_Sendto      },
};

//private functions
static int ETHSOCKETECOS_Open(unsigned int id, ETHSOCKET_OPEN_OBJ *pObj);
static int ETHSOCKETECOS_Close(unsigned int id);

//-------------------------------------------------------------------------
//General global variables
//-------------------------------------------------------------------------


unsigned int ethsocket_ipc_GetSndAddr(ETHSOCKIPC_ID id)
{

	if (*(unsigned int *)(gSndParamAddr[id]) == ETHSOCKETIPC_BUF_TAG) {
		//ETHSOCKET_PRINT(id,"%x %x \r\n",gSndParamAddr[id],*(unsigned int *)(gSndParamAddr[id]));
		return (gSndParamAddr[id] + ETHSOCKETIPC_BUF_CHK_SIZE);
	} else {
		ETHSOCKET_ERR(id, "buf chk fail\r\n");
		return ETHSOCKET_RET_ERR;
	}
}

unsigned int ethsocket_ipc_GetRcvAddr(ETHSOCKIPC_ID id)
{

	if (*(unsigned int *)(gRcvParamAddr[id]) == ETHSOCKETIPC_BUF_TAG) {
		//ETHSOCKET_PRINT(id,"%x %x \r\n",gRcvParamAddr[id],*(unsigned int *)(gRcvParamAddr[id]));
		return (gRcvParamAddr[id] + ETHSOCKETIPC_BUF_CHK_SIZE);
	} else {
		ETHSOCKET_ERR(id, "buf chk fail\r\n");
		return ETHSOCKET_RET_ERR;
	}
}

static void ethsocket_ipc_notify_0(int status, int parm)
{
	ethsocket_ipc_notify(ETHSOCKIPC_ID_0, status, parm);
}

static void ethsocket_ipc_notify_1(int status, int parm, int obj_index)
{
	ethsocket_ipc_notify(obj_index, status, parm);
}

static void ethsocket_ipc_notify_2(int status, int parm, int obj_index)
{
	ethsocket_ipc_notify(obj_index, status, parm);
}

static int ethsocket_ipc_recv_0(char *addr, int size)
{
	return ethsocket_ipc_recv(ETHSOCKIPC_ID_0, addr, size);
}

static int ethsocket_ipc_recv_1(char *addr, int size, int obj_index)
{
	return ethsocket_ipc_recv(obj_index, addr, size);
}

static int ethsocket_ipc_recv_2(char *addr, int size, int obj_index)
{
	return ethsocket_ipc_recv(obj_index, addr, size);
}

static void ethsocket_ipc_notify(ETHSOCKIPC_ID id, int status, int parm)
{
	ETHSOCKET_PARAM_PARAM *pParam = (ETHSOCKET_PARAM_PARAM *)ethsocket_ipc_GetSndAddr(id);
	int result = 0;
	int Ret_EthsocketApi = 0;

	if ((int)pParam == ETHSOCKET_RET_ERR) {
		ETHSOCKET_ERR(id, "pParam err\r\n");
		return ;
	}
	pParam->param1 = (int)status;
	pParam->param2 = (int)parm;

	ETHSOCKET_PRINT(id, "(%d, %d)\r\n", pParam->param1, pParam->param2);

	result = ethsocket_ipc_exeapi(id, ETHSOCKET_CMDID_NOTIFY, &Ret_EthsocketApi);

	if ((result != ETHSOCKET_RET_OK) || (Ret_EthsocketApi != 0)) {
		ETHSOCKET_PRINT(id, "snd CB fail\r\n");
	}
}

static int ethsocket_ipc_recv(ETHSOCKIPC_ID id, char *addr, int size)
{
	ETHSOCKET_TRANSFER_PARAM *pParam = (ETHSOCKET_TRANSFER_PARAM *)ethsocket_ipc_GetSndAddr(id);
	int result = 0;
	int Ret_EthsocketApi = 0;

	ETHSOCKET_PRINT(id, "%x %d\r\n  ", addr, size);

	if ((int)pParam == ETHSOCKET_RET_ERR) {
		ETHSOCKET_ERR(id, "pParam err\r\n");
		return ETHSOCKET_RET_ERR;
	}

	pParam->size = (int)size;

	if (size > ETHSOCKIPC_TRANSFER_BUF_SIZE) {
		ETHSOCKET_ERR(id, "size %d > %d\r\n", size, ETHSOCKIPC_TRANSFER_BUF_SIZE);
		return ETHSOCKET_RET_ERR;
	}

	ETHSOCKET_PRINT(id, "%x %d cp to buf\r\n  ", addr, size);

	memcpy((void *)pParam->buf, addr, size);

	result = ethsocket_ipc_exeapi(id, ETHSOCKET_CMDID_RCV, &Ret_EthsocketApi);

	if ((result != ETHSOCKET_RET_OK) || (Ret_EthsocketApi != 0)) {
		ETHSOCKET_PRINT(id, "snd CB fail\r\n");
	}

	return result;
}

static void ethsocket_ipc_udp_notify(int status, int parm)
{
	unsigned int id = ETHSOCKIPC_ID_0; //only 1
	ETHSOCKET_PARAM_PARAM *pParam = (ETHSOCKET_PARAM_PARAM *)ethsocket_ipc_GetSndAddr(id);
	int result = 0;
	int Ret_EthsocketApi = 0;

	if ((int)pParam == ETHSOCKET_RET_ERR) {
		ETHSOCKET_ERR(id, "pParam err\r\n");
		return ;
	}

	pParam->param1 = (int)status;
	pParam->param2 = (int)parm;

	ETHSOCKET_PRINT(id, "(%d, %d)\r\n", pParam->param1, pParam->param2);

	result = ethsocket_ipc_exeapi(ETHSOCKIPC_ID_0, ETHSOCKET_CMDID_UDP_NOTIFY, &Ret_EthsocketApi);

	if ((result != ETHSOCKET_RET_OK) || (Ret_EthsocketApi != 0)) {
		ETHSOCKET_PRINT(id, "snd CB fail\r\n");
	}

}

static int ethsocket_ipc_udp_recv(char *addr, int size)
{
	unsigned int id = ETHSOCKIPC_ID_0; //only 1
	ETHSOCKET_TRANSFER_PARAM *pParam = (ETHSOCKET_TRANSFER_PARAM *)ethsocket_ipc_GetSndAddr(id);
	int result = 0;
	int Ret_EthsocketApi = 0;

	ETHSOCKET_PRINT(id, "%x %d\r\n  ", addr, size);

	if ((int)pParam == ETHSOCKET_RET_ERR) {
		ETHSOCKET_ERR(id, "pParam err\r\n");
		return ETHSOCKET_RET_ERR;
	}

	pParam->size = (int)size;
	if (size > ETHSOCKIPC_TRANSFER_BUF_SIZE) {
		ETHSOCKET_ERR(id, "size %d > %d\r\n", size, ETHSOCKIPC_TRANSFER_BUF_SIZE);
		return ETHSOCKET_RET_ERR;
	} else {
		memcpy(pParam->buf, addr, size);
	}



	result = ethsocket_ipc_exeapi(id, ETHSOCKET_CMDID_UDP_RCV, &Ret_EthsocketApi);

	if ((result != ETHSOCKET_RET_OK) || (Ret_EthsocketApi != 0)) {
		ETHSOCKET_PRINT(id, "snd CB fail\r\n");
	}

	return result;
}
#if (__USE_IPC)
#if defined(__ECOS)
static void ethsocket_ipc_receive_thread_0(cyg_addrword_t arg)
{
	ethsocket_ipc_receive_thread(ETHSOCKIPC_ID_0, arg);
}

static void ethsocket_ipc_receive_thread_1(cyg_addrword_t arg)
{
	ethsocket_ipc_receive_thread(ETHSOCKIPC_ID_1, arg);
}
static void ethsocket_ipc_receive_thread_2(cyg_addrword_t arg)
{
	ethsocket_ipc_receive_thread(ETHSOCKIPC_ID_2, arg);
}

static void ethsocket_ipc_receive_thread(ETHSOCKIPC_ID id, cyg_addrword_t arg)
#else
static void* ethsocket_ipc_receive_thread_0(void* arg)
{
	return ethsocket_ipc_receive_thread(ETHSOCKIPC_ID_0, arg);
}

static void* ethsocket_ipc_receive_thread_1(void* arg)
{
	return ethsocket_ipc_receive_thread(ETHSOCKIPC_ID_1, arg);
}
static void* ethsocket_ipc_receive_thread_2(void* arg)
{
	return ethsocket_ipc_receive_thread(ETHSOCKIPC_ID_2, arg);
}

static void *ethsocket_ipc_receive_thread(ETHSOCKIPC_ID id, void *arg)
#endif
{
	ETHSOCKET_MSG RcvMsg={0};
	int Ret_ExeResult = ETHSOCKET_RET_ERR;
	int Ret_IpcMsg=0;

	cyg_flag_maskbits(&ethsocket_ipc_flag[id], 0);
	while (1) {
		//1. receive ipc cmd from uItron
		memset(&RcvMsg, 0, sizeof(ETHSOCKET_MSG));
		Ret_IpcMsg = ethsocket_ipc_wait_cmd(id, &RcvMsg);
		if (ETHSOCKET_RET_OK != Ret_IpcMsg) {
			ETHSOCKET_ERR(id, "ethsocket_ipc_waitcmd = %d\r\n", Ret_IpcMsg);
			continue;
		}

		//2. error check
		ETHSOCKET_PRINT(id, "Got RcvCmdId = %d\r\n", RcvMsg.CmdId);
		if (RcvMsg.CmdId >= ETHSOCKET_CMDID_MAX || RcvMsg.CmdId < 0) {
			ETHSOCKET_ERR(id, "RcvCmdId(%d) should be 0~%d\r\n", RcvMsg.CmdId, ETHSOCKET_CMDID_MAX);
			continue;
		}

		if (gCmdFpTbl[RcvMsg.CmdId].CmdId != RcvMsg.CmdId) {
			ETHSOCKET_ERR(id, "RcvCmdId = %d, TableCmdId = %d\r\n", RcvMsg.CmdId, gCmdFpTbl[RcvMsg.CmdId].CmdId);
			continue;
		}

		//3. process the cmd
		if (RcvMsg.CmdId == ETHSOCKET_CMDID_SYSREQ_ACK) {
			ethsocket_ipc_set_ack(id, RcvMsg.Arg);
		} else if (RcvMsg.CmdId == ETHSOCKET_CMDID_UNINIT) {
			ETHSOCKETECOS_Close(id);
			return DUMMY_NULL;
		} else {
			if (gCmdFpTbl[RcvMsg.CmdId].pFunc) {
				Ret_ExeResult = gCmdFpTbl[RcvMsg.CmdId].pFunc(id);
				ETHSOCKET_PRINT(id, "[TSK]Ret_ExeResult = 0x%X\r\n", Ret_ExeResult);
			} else {
				Ret_ExeResult = ETHSOCKET_RET_NO_FUNC;
				ETHSOCKET_ERR(id, "[TSK]Null Cmd pFunc id %d\r\n", RcvMsg.CmdId);
				//ethsocket_ipc_CmdId_Test();
			}

			//send the msg of the return value of FileSys API
			Ret_IpcMsg = ethsocket_ipc_send_ack(id, Ret_ExeResult);
			if (ETHSOCKET_RET_OK != Ret_IpcMsg) {
				ETHSOCKET_ERR(id, "ethsocket_ipc_Msg_WaitCmd = %d\r\n", Ret_IpcMsg);
				continue;
			}
		}
	}

	return DUMMY_NULL;
}
#endif
static int ethsocket_ipc_CmdId_Test(ETHSOCKIPC_ID id)
{
#if 0
	ETHSOCKET_PARAM_PARAM *pParam = (ETHSOCKET_PARAM_PARAM *)ethsocket_ipc_GetRcvAddr(id);

	ETHSOCKET_PRINT(id, "(0x%X, 0x%X, 0x%X)\r\n", pParam->param1, pParam->param2, pParam->param3);
#endif

	return ETHSOCKET_RET_OK;
}

static int ethsocket_ipc_CmdId_Open(ETHSOCKIPC_ID id)
{
	ETHSOCKET_PARAM_PARAM *pParam = (ETHSOCKET_PARAM_PARAM *)ethsocket_ipc_GetRcvAddr(id);
	if (id == ETHSOCKIPC_ID_0) {
		ethsocket_install_obj *pInstallObj = (ethsocket_install_obj *)pParam;
		if (pInstallObj->notify) {
			pInstallObj->notify = ethsocket_ipc_notify_0;
		}
		if (pInstallObj->recv) {
			pInstallObj->recv = ethsocket_ipc_recv_0;
		}

		ethsocket_install(pInstallObj);

		ethsocket_open();

		ETHSOCKET_PRINT(id, "(portNum:%d, threadPriority:%d, sockbufSize:%d,client_socket:%d)\r\n", pInstallObj->portNum, pInstallObj->threadPriority, pInstallObj->sockbufSize, pInstallObj->client_socket);

	} else if (id == ETHSOCKIPC_ID_1) {
		ethsocket_install_obj_multi *pInstallObj = (ethsocket_install_obj_multi *)pParam;
		unsigned int multi_id = 0;
		if (pInstallObj->notify) {
			pInstallObj->notify = ethsocket_ipc_notify_1;
		}
		if (pInstallObj->recv) {
			pInstallObj->recv = ethsocket_ipc_recv_1;
		}

		multi_id = ethsocket_install_multi(id , pInstallObj);

		if (multi_id != id) {
			ETHSOCKET_ERR(id, "open id not match %d %d\n", multi_id, id);
		}

		ethsocket_open_multi(multi_id);

		ETHSOCKET_PRINT(id, "multi_id %d portNum:%d, threadPriority:%d, sockbufSize:%d\r\n", multi_id, pInstallObj->portNum, pInstallObj->threadPriority, pInstallObj->sockbufSize);

	} else if (id == ETHSOCKIPC_ID_2) {
		ethsocket_install_obj_multi *pInstallObj = (ethsocket_install_obj_multi *)pParam;
		unsigned int multi_id = 0;
		if (pInstallObj->notify) {
			pInstallObj->notify = ethsocket_ipc_notify_2;
		}
		if (pInstallObj->recv) {
			pInstallObj->recv = ethsocket_ipc_recv_2;
		}

		multi_id = ethsocket_install_multi(id ,pInstallObj);

		if (multi_id != id) {
			ETHSOCKET_ERR(id, "open id not match %d %d\n", multi_id, id);
		}

		ethsocket_open_multi(multi_id);

		ETHSOCKET_PRINT(id, "multi_id %d portNum:%d, threadPriority:%d, sockbufSize:%d\r\n", multi_id, pInstallObj->portNum, pInstallObj->threadPriority, pInstallObj->sockbufSize);

	}
	return ETHSOCKET_RET_OK;
}

static int ethsocket_ipc_CmdId_Close(ETHSOCKIPC_ID id)
{
	ethsocket_ipc_set_ack(id, ETHSOCKET_RET_FORCE_ACK);
	if (id == ETHSOCKIPC_ID_0) {
		ethsocket_close();
		ethsocket_uninstall();
	} else {
		ethsocket_close_multi(id);
		ethsocket_uninstall_multi(id);
	}
	return ETHSOCKET_RET_OK;

}
static int ethsocket_ipc_CmdId_Send(ETHSOCKIPC_ID id)
{
	ETHSOCKET_TRANSFER_PARAM *pTransParam;// = (ETHSOCKET_TRANSFER_PARAM *)ethsocket_ipc_GetRcvAddr(id);
	ETHSOCKET_SEND_PARAM *pSendParam;// = (ETHSOCKET_SEND_PARAM *)ethsocket_ipc_GetRcvAddr(id);

	if (id == ETHSOCKIPC_ID_0) {
		pSendParam = (ETHSOCKET_SEND_PARAM *)ethsocket_ipc_GetRcvAddr(id);
		pSendParam->buf=(char *)(ethsocket_ipc_GetRcvAddr(id) + sizeof(ETHSOCKET_SEND_PARAM));
		return ethsocket_send(pSendParam->buf, &(pSendParam->size));
	}else if (id == ETHSOCKIPC_ID_1) {
		pSendParam = (ETHSOCKET_SEND_PARAM *)ethsocket_ipc_GetRcvAddr(id);
		pSendParam->buf=(char *)(ethsocket_ipc_GetRcvAddr(id) + sizeof(ETHSOCKET_SEND_PARAM));
		return ethsocket_send_multi(pSendParam->buf, &(pSendParam->size), id);
	} else {
		pTransParam = (ETHSOCKET_TRANSFER_PARAM *)ethsocket_ipc_GetRcvAddr(id);
		return ethsocket_send_multi(pTransParam->buf, &(pTransParam->size), id);
	}
}

static int ethsocket_ipc_CmdId_Udp_Open(ETHSOCKIPC_ID id)
{
	ETHSOCKET_PARAM_PARAM *pParam = (ETHSOCKET_PARAM_PARAM *)ethsocket_ipc_GetRcvAddr(id);
	ethsocket_install_obj *pInstallObj = (ethsocket_install_obj *)pParam;

	if (pInstallObj->notify) {
		pInstallObj->notify = ethsocket_ipc_udp_notify;
	}
	if (pInstallObj->recv) {
		pInstallObj->recv = ethsocket_ipc_udp_recv;
	}

	ETHSOCKET_PRINT(id, "(portNum:%d, threadPriority:%d, sockbufSize:%d,client_socket:%d)\r\n", pInstallObj->portNum, pInstallObj->threadPriority, pInstallObj->sockbufSize, pInstallObj->client_socket);

	ethsocket_udp_install(pInstallObj);

	ethsocket_udp_open();

	return ETHSOCKET_RET_OK;

}

static int ethsocket_ipc_CmdId_Udp_Close(ETHSOCKIPC_ID id)
{
	ethsocket_ipc_set_ack(id, ETHSOCKET_RET_FORCE_ACK);
	ethsocket_udp_uninstall();
	ethsocket_udp_close();
	return ETHSOCKET_RET_OK;
}

static int ethsocket_ipc_CmdId_Udp_Send(ETHSOCKIPC_ID id)
{
	ETHSOCKET_TRANSFER_PARAM *pParam = (ETHSOCKET_TRANSFER_PARAM *)ethsocket_ipc_GetRcvAddr(id);

	return ethsocket_udp_send(pParam->buf, &(pParam->size));
}

static int ethsocket_ipc_CmdId_Udp_Sendto(ETHSOCKIPC_ID id)
{
	ETHSOCKET_SENDTO_PARAM *pParam = (ETHSOCKET_SENDTO_PARAM *)ethsocket_ipc_GetRcvAddr(id);

	return ethsocket_udp_sendto(pParam->dest_ip, pParam->dest_port, pParam->buf, &(pParam->size));
}

int ethsocket_ipc_init(unsigned int id, ETHSOCKET_OPEN_OBJ *pObj)
{

	//set the Receive parameter address for IPC
	//gRcvParamAddr[id] = (NvtIPC_GetNonCacheAddr(pObj->RcvParamAddr)) ;
	gRcvParamAddr[id] = (ipc_getNonCacheAddr(pObj->RcvParamAddr)) ;
	if (ETHSOCKET_INVALID_PARAMADDR == gRcvParamAddr[id]) {
		ETHSOCKET_ERR(id, "gRcvParamAddr[id] = 0x%X\n", gRcvParamAddr[id]);
		return ETHSOCKET_RET_ERR;
	}
	gRcvParamSize[id] = pObj->RcvParamSize;
	if (0 == gRcvParamSize[id]) {
		ETHSOCKET_ERR(id, "gRcvParamSize[id] = 0x%X\n", gRcvParamSize[id]);
		return ETHSOCKET_RET_ERR;
	}

	//set the Send parameter address for IPC
	//gSndParamAddr[id] = (NvtIPC_GetNonCacheAddr(pObj->SndParamAddr)) ;
	gSndParamAddr[id] = (ipc_getNonCacheAddr(pObj->SndParamAddr)) ;
	if (ETHSOCKET_INVALID_PARAMADDR == gSndParamAddr[id]) {
		ETHSOCKET_ERR(id, "gSndParamAddr[id] = 0x%X\n", gSndParamAddr[id]);
		return ETHSOCKET_RET_ERR;
	}

	gSndParamSize[id] = pObj->SndParamSize;
	if (0 == gSndParamSize[id]) {
		ETHSOCKET_ERR(id, "gSndParamSize[id] = 0x%X\n", gSndParamSize[id]);
		return ETHSOCKET_RET_ERR;
	}
#if (__USE_IPC)
	NVTIPC_I32 Ret_NvtIpc=0;

#ifdef ETHSOCKET_SIM
	Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok("WIFIIPC eCos"));
#else

	if (gIPC_MsgId[id] == ETHSOCKET_INVALID_MSGID) {

		if (id == ETHSOCKIPC_ID_0) {
			Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(ETHSOCKET_TOKEN_PATH0));
		} else if (id == ETHSOCKIPC_ID_1) {
			Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(ETHSOCKET_TOKEN_PATH1));
		} else if (id == ETHSOCKIPC_ID_2) {
			Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(ETHSOCKET_TOKEN_PATH2));
		}

	} else {
		Ret_NvtIpc = gIPC_MsgId[id];
	}
#endif

	if (Ret_NvtIpc < 0) {
		ETHSOCKET_ERR(id, "NvtIPC_MsgGet = %d\n", Ret_NvtIpc);
		return ETHSOCKET_RET_ERR;
	}

	gIPC_MsgId[id] = (NVTIPC_U32)Ret_NvtIpc;
	ETHSOCKET_PRINT(id, "gIPC_MsgId[id] = %d\n", gIPC_MsgId[id]);

#ifdef ETHSOCKET_SIM
	if (gIPC_MsgId[id] != gIPC_MsgId_eCos) {
		ETHSOCKET_ERR(id, "Simulation environment error\n");
		return ETHSOCKET_RET_ERR;
	}
#endif

	cyg_flag_init(&ethsocket_ipc_flag[id]);

#if defined(__ECOS)
	/* Create a main thread for receive message */
	cyg_thread_create(ETHSOCKETIPC_THREAD_PRIORITY, //ethsocket_ipc_obj.threadPriority,
					  gThread[id],
					  0,
					  "ethcam socket ipc",
					  &ethsocket_ipc_stacks[id][0],
					  sizeof(ethsocket_ipc_stacks[id]),
					  &ethsocket_ipc_thread[id],
					  &ethsocket_ipc_thread_object[id]
					 );

	cyg_thread_resume(ethsocket_ipc_thread[id]);  /* Start it */
#else
	if(pthread_create(&ethsocket_ipc_thread[id], NULL , gThread[id] , NULL)){
            printf("usocket can't create usocket_thread\r\n");
	}
#endif
#endif
	return ETHSOCKET_RET_OK;
}

int ethsocket_ipc_uninit(unsigned int id)
{
#if (__USE_IPC)
	NVTIPC_I32 Ret_NvtIpc;
	cyg_flag_destroy(&ethsocket_ipc_flag[id]);

#ifdef ETHSOCKET_SIM
	gIPC_MsgId[id] = gIPC_MsgId_eCos;
#endif

	Ret_NvtIpc = NvtIPC_MsgRel(gIPC_MsgId[id]);
	if (Ret_NvtIpc < 0) {
		ETHSOCKET_ERR(id, "NvtIPC_MsgRel = %d\n", Ret_NvtIpc);
		return ETHSOCKET_RET_ERR;
	}

	gIPC_MsgId[id] = ETHSOCKET_INVALID_MSGID;
#endif
	gRcvParamAddr[id] = ETHSOCKET_INVALID_PARAMADDR;

#if defined(__ECOS)
	cyg_thread_exit();
#endif

	return ETHSOCKET_RET_OK;
}

int ethsocket_ipc_wait_ack(unsigned int id)
{
#if (__USE_IPC)
	cyg_flag_waitms(&ethsocket_ipc_flag[id], CYG_FLG_ETHSOCKETIPC_ACK, CYG_FLAG_WAITMODE_OR | CYG_FLAG_WAITMODE_CLR);
	return gRetValue[id];
#else
	return 0;
#endif
}

void ethsocket_ipc_set_ack(unsigned int id, int retValue)
{
#if (__USE_IPC)
	gRetValue[id] = retValue;
	cyg_flag_setbits(&ethsocket_ipc_flag[id], CYG_FLG_ETHSOCKETIPC_ACK);
#endif
}

int ethsocket_ipc_exeapi(unsigned int id, ETHSOCKET_CMDID CmdId, int *pOutRet)
{
#if (__USE_IPC)
	ETHSOCKET_MSG IpcMsg = {0};
	NVTIPC_I32  Ret_NvtIpc;

	if (ETHSOCKET_INVALID_MSGID == gIPC_MsgId[id]) {
		ETHSOCKET_ERR(id, "ETHSOCKET_INVALID_MSGID\n");
		return ETHSOCKET_RET_ERR;
	}

	IpcMsg.CmdId = CmdId;
	IpcMsg.Arg = 0;

#ifdef ETHSOCKET_SIM
	gIPC_MsgId[id] = gIPC_MsgId_uITRON;
#endif

	ETHSOCKET_PRINT(id, "Send CmdId = %d\n", CmdId);
	Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId[id], ETHSOCKET_SEND_TARGET, &IpcMsg, sizeof(IpcMsg));
	if (Ret_NvtIpc < 0) {
		ETHSOCKET_ERR(id, "NvtIPC_MsgSnd = %d\n", Ret_NvtIpc);
		return ETHSOCKET_RET_ERR;
	}
	*pOutRet = ethsocket_ipc_wait_ack(id);
#else
	EthsockIpc_HandleCmd(CmdId,  id);
#endif
	return ETHSOCKET_RET_OK;
}

//Ack the API caller with Ack CmdId and Ack Arg (if no Arg to be sent, please set Arg to 0)
int ethsocket_ipc_send_ack(unsigned int id, int AckValue)
{
#if (__USE_IPC)
	ETHSOCKET_MSG IpcMsg = {0};
	NVTIPC_I32  Ret_NvtIpc;

	if (ETHSOCKET_INVALID_MSGID == gIPC_MsgId[id]) {
		ETHSOCKET_ERR(id, "ETHSOCKET_INVALID_MSGID\n");
		return ETHSOCKET_RET_ERR;
	}

	IpcMsg.CmdId = ETHSOCKET_CMDID_SYSREQ_ACK;
	IpcMsg.Arg = AckValue;

#ifdef ETHSOCKET_SIM
	gIPC_MsgId[id] = gIPC_MsgId_uITRON;
#endif

	Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId[id], ETHSOCKET_SEND_TARGET, &IpcMsg, sizeof(IpcMsg));
	if (Ret_NvtIpc < 0) {
		ETHSOCKET_ERR(id, "NvtIPC_MsgSnd = %d\n", Ret_NvtIpc);
		return ETHSOCKET_RET_ERR;
	}
	#endif
	return ETHSOCKET_RET_OK;
}

int ethsocket_ipc_wait_cmd(unsigned int id, ETHSOCKET_MSG *pRcvMsg)
{
#if (__USE_IPC)
	NVTIPC_I32  Ret_NvtIpc;
 #ifdef ETHSOCKET_SIM
	gIPC_MsgId[id] = gIPC_MsgId_eCos;
#endif
 	Ret_NvtIpc = NvtIPC_MsgRcv(gIPC_MsgId[id], pRcvMsg, sizeof(ETHSOCKET_MSG));
	if (Ret_NvtIpc < 0) {
		ETHSOCKET_ERR(id, "NvtIPC_MsgRcv = %d\r\n", Ret_NvtIpc);
		return ETHSOCKET_RET_ERR;
	}
	#endif
	return ETHSOCKET_RET_OK;
}

//==========================================================================

#define ETHSOCKET_MACRO_START                   do {
#define ETHSOCKET_MACRO_END                     } while (0)

#define ETHSOCKET_API_RET(id,RetValue)          \
	ETHSOCKET_MACRO_START                       \
	ethsocket_ipc_send_ack(id,RetValue);        \
	return RetValue;                            \
	ETHSOCKET_MACRO_END
#define ETHSOCKET_API_RET_UNINIT(id,RetValue)   \
	ETHSOCKET_MACRO_START                       \
	ethsocket_ipc_send_ack(id,RetValue);        \
	ethsocket_ipc_uninit(id);                   \
	return RetValue;                            \
	ETHSOCKET_MACRO_END
//-------------------------------------------------------------------------
//Implementation of ETHSOCKETECOS API
//-------------------------------------------------------------------------
static int ETHSOCKETECOS_Open(unsigned int id, ETHSOCKET_OPEN_OBJ *pObj)
{
	ETHSOCKET_PRINT(id, "\n");

	if (gIsOpened[id]) {
		ETHSOCKET_ERR(id, "is opened\n");
		ETHSOCKET_API_RET(id, ETHSOCKET_RET_OPENED);
	}

	if (NULL == pObj) {
		ETHSOCKET_ERR(id, "pObj = 0x%X\n", pObj);
		ETHSOCKET_API_RET(id, ETHSOCKET_RET_ERR);
	}

	//init the IPC communication
	if (ETHSOCKET_RET_ERR == ethsocket_ipc_init(id, pObj)) {
		ETHSOCKET_ERR(id, "ethsocket_ipc_init failed\n");
		ETHSOCKET_API_RET(id, ETHSOCKET_RET_ERR);
	}
	gIsOpened[id] = true;
	ETHSOCKET_API_RET(id, ETHSOCKET_RET_OK);
}

static int ETHSOCKETECOS_Close(unsigned int id)
{
	ETHSOCKET_PRINT(id, "\n");

	gIsOpened[id] = false;

	ETHSOCKET_API_RET_UNINIT(id, ETHSOCKET_RET_OK);
}


#if defined(__ECOS)
void ETHSOCKETECOS_CmdLine(char *szCmd)
{
	char *delim = " ";
	char *pToken;

	ETHSOCKET_PRINT(3, "szCmd = %s\n", szCmd);

	pToken = strtok(szCmd, delim);
	while (pToken != NULL) {
		if (!strcmp(pToken, "-open")) {
			ETHSOCKET_OPEN_OBJ OpenObj;
			ETHSOCKIPC_ID id = 0;

			//get receive memory address
			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKET_ERR(id, "get rcv address fail\n");
				break;
			}
			OpenObj.RcvParamAddr = atoi(pToken);

			//get receive memory size
			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKET_ERR(id, "get rcv size fail\n");
				break;
			}
			OpenObj.RcvParamSize= atoi(pToken);

			//get send memory address
			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKET_ERR(id, "get snd address fail\n");
				break;
			}
			OpenObj.SndParamAddr = atoi(pToken);

			//get send memory size
			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKET_ERR(id, "get snd size fail\n");
				break;
			}
			OpenObj.SndParamSize= atoi(pToken);

			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKET_ERR(id, "get ver key fail\n");
				break;
			}
			if (atoi(pToken) != ETHSOCKET_VER_KEY) {
				ETHSOCKET_ERR(id, "ver key mismatch %d %d\n", ETHSOCKET_VER_KEY, atoi(pToken));
				break;
			}
			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKET_ERR(id, "get id fail\n");
				break;
			}

			id = atoi(pToken);
			//open ETHSOCKETECOS
			ETHSOCKETECOS_Open(id, &OpenObj);
			break;
		} else if (!strcmp(pToken, "-close")) {
			ETHSOCKIPC_ID id = 0;
			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKET_ERR(id, "get id fail\n");
				break;
			}
			id = atoi(pToken);
			ETHSOCKETECOS_Close(id);
			break;
		}

		pToken = strtok(NULL, delim);
	}
}
#else
int ETHSOCKETECOS_CmdLine(char *szCmd, char *szRcvAddr, char *szSndAddr)
{
	char *delim = " ";
	char *pToken;

	//ETHSOCKET_PRINT(3, "szCmd = %s\n", szCmd);
	printf("szCmd = %s\n", szCmd);

	pToken = strtok(szCmd, delim);
	while (pToken != NULL) {
		if (!strcmp(pToken, "-open")) {
			ETHSOCKET_OPEN_OBJ OpenObj;
			ETHSOCKIPC_ID id = 0;

			//get receive memory address
			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKET_ERR(id, "get rcv address fail\n");
				break;
			}
			OpenObj.RcvParamAddr = atoi(pToken);

			//get receive memory size
			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKET_ERR(id, "get rcv size fail\n");
				break;
			}
			OpenObj.RcvParamSize= atoi(pToken);

			//get send memory address
			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKET_ERR(id, "get snd address fail\n");
				break;
			}
			OpenObj.SndParamAddr = atoi(pToken);

			//get send memory size
			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKET_ERR(id, "get snd size fail\n");
				break;
			}
			OpenObj.SndParamSize= atoi(pToken);

			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKET_ERR(id, "get ver key fail\n");
				break;
			}
			if (atoi(pToken) != ETHSOCKET_VER_KEY) {
				ETHSOCKET_ERR(id, "ver key mismatch %d %d\n", ETHSOCKET_VER_KEY, atoi(pToken));
				break;
			}
			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKET_ERR(id, "get id fail\n");
				break;
			}

			id = atoi(pToken);
			//open ETHSOCKETECOS
	            return ETHSOCKETECOS_Open(id, &OpenObj);
		} else if (!strcmp(pToken, "-close")) {
			ETHSOCKIPC_ID id = 0;
			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKET_ERR(id, "get id fail\n");
				break;
			}
			id = atoi(pToken);
			ETHSOCKETECOS_Close(id);
			break;
		}

		pToken = strtok(NULL, delim);
	}
	return ETHSOCKET_RET_ERR;
}

int ETHSOCKETECOS_HandleCmd(ETHSOCKET_CMDID CmdId, unsigned int id)
{
	int Ret_ExeResult = ETHSOCKET_RET_ERR;
	if(CmdId == ETHSOCKET_CMDID_UNINIT) {
		ETHSOCKETECOS_Close(id);
		Ret_ExeResult = ETHSOCKET_RET_OK;
	}else if (gCmdFpTbl[CmdId].pFunc) {
		Ret_ExeResult= gCmdFpTbl[CmdId].pFunc(id);
	}else{
		Ret_ExeResult =ETHSOCKET_RET_NO_FUNC;
	}
	return Ret_ExeResult;
}

#endif
// -------------------------------------------------------------------------


#if defined(__LINUX_660)
int main(int argc, char **argv)
{
	if (argc < 4) {
		ETHSOCKET_ERR(id, "ethsocket:: incorrect main argc(%d)\n", argc);
		return -1;
	}
	ETHSOCKET_PRINT(id, "szCmd = %s ; szRcvAddr is %s ; szSndAddr is %s\n", argv[1], argv[2], argv[3]);

	if (ETHSOCKET_RET_OK != ETHSOCKETECOS_CmdLine(argv[1], argv[2], argv[3])) {
		ETHSOCKET_ERR(id, "ETHSOCKETECOS_CmdLinel() fail\n");
		return -1;
	}

	ethsocket_ipc_receive_thread(NULL);

	return 0;
}
#endif

