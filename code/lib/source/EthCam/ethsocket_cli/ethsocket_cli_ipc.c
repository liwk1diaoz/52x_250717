#if defined(__ECOS)
#include <cyg/nvtipc/NvtIpcAPI.h>
#else
#include <stdlib.h>
#include <string.h>
#include <nvtipc.h>
//#include <comm/nvtmem.h>
#include <pthread.h>
#include <unistd.h>
#endif
#include "ethsocket_cli_int.h"
#include "EthCam/ethsocket_cli_ipc.h"
#include "EthCam/ethsocket_cli.h"
#if defined(__ECOS)
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_cache.h>
#include <cyg/kernel/kapi.h>
#include <stdlib.h>
#endif

//#ifndef __USE_IPC
//#define __USE_IPC              0
//#endif

#if (__USE_IPC)
#define ipc_getCacheAddr(addr)        	NvtIPC_GetCacheAddr(addr)
#define ipc_getNonCacheAddr(addr)   	NvtIPC_GetNonCacheAddr(addr)
#define ipc_flushCache(addr, size)   	NvtIPC_FlushCache(addr, size)
#else
#define ipc_getCacheAddr(addr)        addr
#define ipc_getNonCacheAddr(addr)   addr
#define ipc_flushCache(addr, size)
#endif

#define ETHSOCKETCLI_VER_KEY              20190506
#define ETHSOCKETCLI_SEND_TARGET          NVTIPC_SENDTO_CORE1
#define ETHSOCKETCLI_INVALID_MSGID        NVTIPC_MSG_QUEUE_NUM+1
#define ETHSOCKETCLI_INVALID_PARAMADDR    0x0

static NVTIPC_U32   gIPC_MsgId[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {ETHSOCKETCLI_INVALID_MSGID,ETHSOCKETCLI_INVALID_MSGID,ETHSOCKETCLI_INVALID_MSGID,
																			ETHSOCKETCLI_INVALID_MSGID,ETHSOCKETCLI_INVALID_MSGID,ETHSOCKETCLI_INVALID_MSGID};

#ifdef ETHSOCKETCLI_SIM
static NVTIPC_U32   gIPC_MsgId_uITRON = 1;
static NVTIPC_U32   gIPC_MsgId_eCos = 2;
#endif //#ifdef ETHSOCKETCLI_SIM

int ethsocket_cli_ipc_wait_ack(unsigned int path_id, unsigned int id);
void ethsocket_cli_ipc_set_ack(unsigned int path_id, unsigned int id, int retValue);
int ethsocket_cli_ipc_send_ack(unsigned int path_id, unsigned int id, int AckValue);
int ethsocket_cli_ipc_wait_cmd(unsigned int path_id, unsigned int id, ETHSOCKETCLI_MSG *pRcvMsg);
int ethsocket_cli_ipc_init(unsigned int path_id, unsigned int id, ETHSOCKETCLI_OPEN_OBJ* pObj);
int ethsocket_cli_ipc_uninit(unsigned int path_id, unsigned int id);
int ethsocket_cli_ipc_exeapi(unsigned int path_id, unsigned int id, ETHSOCKETCLI_CMDID CmdId, int *pOutRet);
void ethsocket_cli_ipc_init_data1_bs_buf(ETHCAM_PATH_ID path_id);
void ethsocket_cli_ipc_init_data2_bs_buf(ETHCAM_PATH_ID path_id);

#if (__USE_IPC)
#if defined(__ECOS)
static void ethsocket_cli_ipc_receive_thread_0_p0(cyg_addrword_t arg);
static void ethsocket_cli_ipc_receive_thread_1_p0(cyg_addrword_t arg);
static void ethsocket_cli_ipc_receive_thread_2_p0(cyg_addrword_t arg);
static void ethsocket_cli_ipc_receive_thread_0_p1(cyg_addrword_t arg);
static void ethsocket_cli_ipc_receive_thread_1_p1(cyg_addrword_t arg);
static void ethsocket_cli_ipc_receive_thread_2_p1(cyg_addrword_t arg);
static void ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, cyg_addrword_t arg);
#else
static void* ethsocket_cli_ipc_receive_thread_0_p0(void* arg);
static void* ethsocket_cli_ipc_receive_thread_1_p0(void* arg);
static void* ethsocket_cli_ipc_receive_thread_2_p0(void* arg);
static void* ethsocket_cli_ipc_receive_thread_0_p1(void* arg);
static void* ethsocket_cli_ipc_receive_thread_1_p1(void* arg);
static void* ethsocket_cli_ipc_receive_thread_2_p1(void* arg);
static void* ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, void* arg);
#endif
#endif
static void ethsocket_cli_ipc_notify_0_p0(int obj_index, int status, int parm);
static void ethsocket_cli_ipc_notify_1_p0(int obj_index, int status, int parm);
static void ethsocket_cli_ipc_notify_2_p0(int obj_index, int status, int parm);
static void ethsocket_cli_ipc_notify_0_p1(int obj_index, int status, int parm);
static void ethsocket_cli_ipc_notify_1_p1(int obj_index, int status, int parm);
static void ethsocket_cli_ipc_notify_2_p1(int obj_index, int status, int parm);

static void ethsocket_cli_ipc_recv_0_p0(int obj_index,  char* addr, int size);
static void ethsocket_cli_ipc_recv_1_p0(int obj_index,  char* addr, int size);
static void ethsocket_cli_ipc_recv_2_p0(int obj_index,  char* addr, int size);
static void ethsocket_cli_ipc_recv_0_p1(int obj_index,  char* addr, int size);
static void ethsocket_cli_ipc_recv_1_p1(int obj_index,  char* addr, int size);
static void ethsocket_cli_ipc_recv_2_p1(int obj_index,  char* addr, int size);

extern void EthsockCliIpc_HandleCmd(ETHSOCKETCLI_CMDID CmdId, unsigned int path_id, unsigned int id);

/* ================================================================= */
/* Thread stacks, etc.
 */
#define ETHSOCKETCLIIPC_THREAD_PRIORITY        5
#define ETHSOCKETCLIIPC_THREAD_STACK_SIZE      (4 * 1024)
#define ETHSOCKETCLIIPC_BUFFER_SIZE            512
// flag define

#define CYG_FLG_ETHSOCKETCLIIPC_START       0x01
#define CYG_FLG_ETHSOCKETCLIIPC_IDLE        0x02
#define CYG_FLG_ETHSOCKETCLIIPC_ACK         0x04
#if (__USE_IPC)
#if defined(__ECOS)
static cyg_uint8 ethsocket_cli_ipc_stacks[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM][ETHSOCKETCLIIPC_BUFFER_SIZE + ETHSOCKETCLIIPC_THREAD_STACK_SIZE] = {0};
static cyg_handle_t ethsocket_cli_ipc_thread[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM]={0};
static cyg_thread   ethsocket_cli_ipc_thread_object[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM];
static cyg_flag_t ethsocket_cli_ipc_flag[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM]={0};
//static cyg_mutex_t ethsocket_cli_ipc_mutex;
#else
static pthread_t ethsocket_cli_ipc_thread[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM];
static volatile int ethsocket_cli_ipc_flag[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM];
#endif

static unsigned int gRetValue[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {0};
#if defined(__ECOS)
static cyg_thread_entry_t *gThread[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {
	{ethsocket_cli_ipc_receive_thread_0_p0, ethsocket_cli_ipc_receive_thread_1_p0, ethsocket_cli_ipc_receive_thread_2_p0},
	{ethsocket_cli_ipc_receive_thread_0_p1, ethsocket_cli_ipc_receive_thread_1_p1, ethsocket_cli_ipc_receive_thread_2_p1}
};
static char ethsocket_cli_ipc_thread_name[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM][20]={{{"ethsockCliIpc0_p0"}, {"ethsockCliIpc1_p0"}, {"ethsockCliIpc2_p0"}},
																		{{"ethsockCliIpc0_p1"}, {"ethsockCliIpc1_p1"}, {"ethsockCliIpc2_p1"}}
																		};

#else
static void *gThread[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {
	{ethsocket_cli_ipc_receive_thread_0_p0, ethsocket_cli_ipc_receive_thread_1_p0, ethsocket_cli_ipc_receive_thread_2_p0},
	{ethsocket_cli_ipc_receive_thread_0_p1, ethsocket_cli_ipc_receive_thread_1_p1, ethsocket_cli_ipc_receive_thread_2_p1}
};
#endif
#endif
static ethsocket_cli_notify *gNotify[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {
	{ethsocket_cli_ipc_notify_0_p0, ethsocket_cli_ipc_notify_1_p0, ethsocket_cli_ipc_notify_2_p0},
	{ethsocket_cli_ipc_notify_0_p1, ethsocket_cli_ipc_notify_1_p1, ethsocket_cli_ipc_notify_2_p1}
};
static ethsocket_cli_recv *gRecv[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {
	{ethsocket_cli_ipc_recv_0_p0, ethsocket_cli_ipc_recv_1_p0, ethsocket_cli_ipc_recv_2_p0},
	{ethsocket_cli_ipc_recv_0_p1, ethsocket_cli_ipc_recv_1_p1, ethsocket_cli_ipc_recv_2_p1}
};

static unsigned int gRcvParamAddr[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {ETHSOCKETCLI_INVALID_PARAMADDR};//Local for eCos, non-cached address
static unsigned int gRcvParamSize[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {0};
static unsigned int gSndParamAddr[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {ETHSOCKETCLI_INVALID_PARAMADDR};//Local for eCos, non-cached address
unsigned int gSndParamSize[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {0};

static unsigned int g_SocketCliData_BsBufTotalAddr[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {ETHSOCKETCLI_INVALID_PARAMADDR};//Local for eCos, non-cached address
static unsigned int g_SocketCliData_BsBufTotalSize[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {0};
static unsigned int g_SocketCliData_RawEncodeAddr[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {ETHSOCKETCLI_INVALID_PARAMADDR};//Local for eCos, non-cached address
static unsigned int g_SocketCliData_RawEncodeSize[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {0};
static unsigned int g_SocketCliData_BsQueueMax[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM]  = {0};

static unsigned int *g_SocketCliData1_BsBufMapTbl[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData1_BsAddr[ETHCAM_PATH_ID_MAX];
static unsigned int g_SocketCliData1_BsQueueIdx[ETHCAM_PATH_ID_MAX]={0};
unsigned int g_SocketCliData1_RecvAddr[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData1_BsFrameCnt[ETHCAM_PATH_ID_MAX]={0};

static unsigned int *g_SocketCliData2_BsBufMapTbl[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData2_BsAddr[ETHCAM_PATH_ID_MAX];
static unsigned int g_SocketCliData2_BsQueueIdx[ETHCAM_PATH_ID_MAX]={0};
unsigned int g_SocketCliData2_RecvAddr[ETHCAM_PATH_ID_MAX]={0};
#if (__USE_IPC)
static char g_ProcIdTbl[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM][20]={{ETHSOCKETCLI_TOKEN_scoket0_p0, ETHSOCKETCLI_TOKEN_scoket1_p0, ETHSOCKETCLI_TOKEN_scoket2_p0},{ETHSOCKETCLI_TOKEN_scoket0_p1, ETHSOCKETCLI_TOKEN_scoket1_p1, ETHSOCKETCLI_TOKEN_scoket2_p1}};
#endif

unsigned int g_SocketCliData_DebugAddr[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {ETHSOCKETCLI_INVALID_PARAMADDR};//Local for eCos, non-cached address
unsigned int g_SocketCliData_DebugSize[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {0};
unsigned int g_SocketCliData_DebugRecvSize[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {0};

unsigned int g_SocketCliData_DebugReceiveEn[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM]={0};

typedef int (*ETHSOCKETCLI_CMDID_FP)(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id);

typedef struct{
	ETHSOCKETCLI_CMDID CmdId;
	ETHSOCKETCLI_CMDID_FP pFunc;
} ETHSOCKETCLI_CMDID_SET;

#define ETHSOCKETCLI_CMD_FP_MAX        (sizeof(gCmdFpTbl) / sizeof(ETHSOCKETCLI_CMDID_SET))

static int ethsocket_cli_ipc_CmdId_Start(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id);
static int ethsocket_cli_ipc_CmdId_Connect(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id);
static int ethsocket_cli_ipc_CmdId_Disconnect(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id);
static int ethsocket_cli_ipc_CmdId_Stop(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id);
static int ethsocket_cli_ipc_CmdId_Send(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id);
void ethsocket_cli_ipc_notify(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id,int status, int parm);
int ethsocket_cli_ipc_recv(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id ,char* addr, int size);

static const ETHSOCKETCLI_CMDID_SET gCmdFpTbl[]= {
	{ETHSOCKETCLI_CMDID_GET_VER_INFO,       NULL                                },
	{ETHSOCKETCLI_CMDID_GET_BUILD_DATE,     NULL                                },
	{ETHSOCKETCLI_CMDID_START,              ethsocket_cli_ipc_CmdId_Start       },
	{ETHSOCKETCLI_CMDID_CONNECT,            ethsocket_cli_ipc_CmdId_Connect     },
	{ETHSOCKETCLI_CMDID_DISCONNECT,         ethsocket_cli_ipc_CmdId_Disconnect  },
	{ETHSOCKETCLI_CMDID_STOP,               ethsocket_cli_ipc_CmdId_Stop        },
	{ETHSOCKETCLI_CMDID_SEND,               ethsocket_cli_ipc_CmdId_Send        },
	{ETHSOCKETCLI_CMDID_RCV,                NULL                                },
	{ETHSOCKETCLI_CMDID_NOTIFY,             NULL                                },
	{ETHSOCKETCLI_CMDID_SYSREQ_ACK,         NULL                                },
	{ETHSOCKETCLI_CMDID_UNINIT,             NULL                                },
};
//private functions
static int ETHSOCKETCLIECOS_Open(unsigned int path_id, unsigned int id, ETHSOCKETCLI_OPEN_OBJ* pObj);
static int ETHSOCKETCLIECOS_Close(unsigned int path_id, unsigned int id);

//-------------------------------------------------------------------------
//General global variables
//-------------------------------------------------------------------------
static bool             gIsOpened[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM] = {false};

static unsigned int ethsocket_cli_ipc_GetSndAddr(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id)
{
	if (*(unsigned int *)(gSndParamAddr[path_id][id]) == ETHSOCKETCLIIPC_BUF_TAG) {
		//ETHSOCKETCLI_PRINT("%x %x \r\n",gSndParamAddr,*(unsigned int *)(gSndParamAddr));
		return (gSndParamAddr[path_id][id] + ETHSOCKETCLIIPC_BUF_CHK_SIZE);
	} else {
		ETHSOCKETCLI_ERR(path_id, id,"buf chk fail\r\n");
		return ETHSOCKETCLI_RET_ERR;
	}
}

static unsigned int ethsocket_cli_ipc_GetRcvAddr(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id)
{
	if (*(unsigned int *)(gRcvParamAddr[path_id][id]) == ETHSOCKETCLIIPC_BUF_TAG) {
		//ETHSOCKETCLI_PRINT("%x %x \r\n",gRcvParamAddr,*(unsigned int *)(gRcvParamAddr));
		return (gRcvParamAddr[path_id][id] + ETHSOCKETCLIIPC_BUF_CHK_SIZE);
	} else {
		ETHSOCKETCLI_ERR(path_id, id,"buf chk fail\r\n");
		return ETHSOCKETCLI_RET_ERR;
	}
}
static void ethsocket_cli_ipc_notify_0_p0(int obj_index, int status, int parm)
{
	ethsocket_cli_ipc_notify(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_0, status, parm);
}

static void ethsocket_cli_ipc_notify_1_p0(int obj_index, int status, int parm)
{
	ethsocket_cli_ipc_notify(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_1, status, parm);
}

static void ethsocket_cli_ipc_notify_2_p0(int obj_index, int status, int parm)
{
	ethsocket_cli_ipc_notify(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_2, status, parm);
}
static void ethsocket_cli_ipc_notify_0_p1(int obj_index, int status, int parm)
{
	ethsocket_cli_ipc_notify(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_0, status, parm);
}

static void ethsocket_cli_ipc_notify_1_p1(int obj_index, int status, int parm)
{
	ethsocket_cli_ipc_notify(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_1, status, parm);
}

static void ethsocket_cli_ipc_notify_2_p1(int obj_index, int status, int parm)
{
	ethsocket_cli_ipc_notify(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_2, status, parm);
}

void ethsocket_cli_ipc_notify(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, int status, int parm)
{
	ETHSOCKETCLI_PARAM_PARAM *pParam = (ETHSOCKETCLI_PARAM_PARAM *)ethsocket_cli_ipc_GetSndAddr(path_id, id);
	int result =0;
	int Ret_EthsocketApi = 0;

	if (pParam == NULL) {
		ETHSOCKETCLI_ERR(path_id,id,"pParam NULL\r\n");
		return;
	}
	if ((int)pParam == ETHSOCKETCLI_RET_ERR) {
		ETHSOCKETCLI_ERR(path_id,id,"pParam err\r\n");
		return;
	}

	//pParam->param1 = (int)obj;
	//pParam->param2 = (int)status;
	//pParam->param3 = (int)parm;
	pParam->param1 = (int)status;
	pParam->param2 = (int)parm;

	ETHSOCKETCLI_PRINT(path_id,id, "(%x, %d, %d)\r\n",pParam->param1, pParam->param2, pParam->param3);

	ipc_flushCache((NVTIPC_U32)pParam, (NVTIPC_U32)sizeof(ETHSOCKETCLI_PARAM_PARAM));
	result = ethsocket_cli_ipc_exeapi(path_id, id, ETHSOCKETCLI_CMDID_NOTIFY, &Ret_EthsocketApi);

	if ((result!=ETHSOCKETCLI_RET_OK) || (Ret_EthsocketApi != 0)) {
		ETHSOCKETCLI_PRINT(path_id,id, "snd CB fail\r\n");
	}
}
unsigned int ethsocket_cli_ipc_Checksum(unsigned char *buf, unsigned int len)
{
	unsigned int i;
	unsigned int checksum = 0;

	for(i=0;i<len;i++){
		checksum+= *(buf+i);
	}

	//ETHSOCKETCLI_PRINT(path_id,id,"Checksum -> 0x%x\r\n",checksum);
	return checksum;
}

#define H264_TYPE_IDR      5
#define H264_TYPE_SLICE   1
#define H264_TYPE_SPS     7
#define H264_TYPE_PPS     8

#define H264_START_CODE_I		0x88
#define H264_START_CODE_P     	0x9A

#define H265_TYPE_SLICE 1
#define H265_TYPE_IDR   19
#define H265_TYPE_VPS  32
#define H265_TYPE_SPS  33
#define H265_TYPE_PPS  34


#define HEAD_TYPE_THUMB     0xff
#define HEAD_TYPE_RAW_ENCODE     0xfe
#define HEAD_TYPE_LONGCNT_STAMP     0xfc

#define LONGCNT_STAMP_EN 2
#define LONGCNT_STAMP_LENGTH 16




//for ts
//#define TS_VIDEOPES_HEADERLENGTH  14  //(9+PTS 5bytes)
//#define TS_AUDIOPES_HEADERLENGTH  14  //(9+PTS 5bytes)
//#define TS_VIDEO_264_NAL_LENGTH    6
//#define TS_VIDEO_265_NAL_LENGTH    7

#define TS_VIDEO_RESERVED_SIZE 24 //align 4
//#define ETHCAM_AUDCAP_RESERVED_SIZE (4*1024)
#define  ETHCAM_AUDCAP_FRAME_SIZE  (2*1024-TS_VIDEO_RESERVED_SIZE)

#define ERR_SLOW_RECV_OVER_FLOW 1
#define ERR_SLOW_BS_OVER_FLOW 2
#define ERR_SLOW_CHECKSUM_ERR 3

unsigned int g_SocketCliData1_RecvSize[ETHCAM_PATH_ID_MAX]={0};
unsigned int g_SocketCliData1_BsSize[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData1_BsStartPos[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData1_IdxInGOP[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData1_checkSumHead[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData1_checkSumTail[ETHCAM_PATH_ID_MAX]={0};

static unsigned int g_SocketCliData1_LongConterLow[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData1_LongConterHigh[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData1_LongConterStampEn[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData1_DescSize[ETHCAM_PATH_ID_MAX]={0};

static unsigned int g_SocketCliData1_BsSizeOverflow[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData1_ErrCnt[ETHCAM_PATH_ID_MAX]={0};

unsigned char *g_SocketCliData1_BsFrmBufAddr[ETHCAM_PATH_ID_MAX]={0};
unsigned int g_SocketCliData1_LongConterByteBK[ETHCAM_PATH_ID_MAX][LONGCNT_STAMP_LENGTH]={0};

//static unsigned int g_SocketCliData1_H265Desc[ETHCAM_PATH_ID_MAX][128]={0};


unsigned int g_SocketCliData2_RecvSize[ETHCAM_PATH_ID_MAX]={0};
unsigned int g_SocketCliData2_BsSize[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData2_BsStartPos[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData2_IdxInGOP[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData2_checkSumHead[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData2_checkSumTail[ETHCAM_PATH_ID_MAX]={0};

static unsigned int g_SocketCliData2_LongConterLow[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData2_LongConterHigh[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData2_LongConterStampEn[ETHCAM_PATH_ID_MAX]={0};
static unsigned int g_SocketCliData2_DescSize[ETHCAM_PATH_ID_MAX]={0};
unsigned char *g_SocketCliData2_BsFrmBufAddr[ETHCAM_PATH_ID_MAX]={0};
unsigned int g_SocketCliData2_LongConterByteBK[ETHCAM_PATH_ID_MAX][LONGCNT_STAMP_LENGTH]={0};


#define ETHCAM_FRM_HEADER_SIZE_PART1 (24)//(28)
#define ETHCAM_FRM_HEADER_SIZE_PART2 (14)//(28)
#define ETHCAM_FRM_HEADER_SIZE (ETHCAM_FRM_HEADER_SIZE_PART1 + ETHCAM_FRM_HEADER_SIZE_PART2)
#define ETHCAM_PREVHEADER_MAX_FRMNUM 60//30//15   //backup max frame number
static unsigned char g_Data1_Tx_PrevHeader[ETHCAM_PATH_ID_MAX][ETHCAM_PREVHEADER_MAX_FRMNUM][ETHCAM_FRM_HEADER_SIZE]={0};
static unsigned char g_Data2_Tx_PrevHeader[ETHCAM_PATH_ID_MAX][ETHCAM_PREVHEADER_MAX_FRMNUM][ETHCAM_FRM_HEADER_SIZE]={0};

static unsigned int g_Data1_Tx_PrevHeaderBufIdx=0;
static unsigned int g_Data2_Tx_PrevHeaderBufIdx=0;
//static unsigned int g_EthCamTxData1_FrameIndex=0;
//static unsigned int g_EthCamTxData1_Prev_FrameIndex=0;
//static unsigned int g_EthCamTxData2_FrameIndex=0;
//static unsigned int g_EthCamTxData2_Prev_FrameIndex=0;

unsigned int ethsocket_cli_ipc_Data1_DumpBsInfo(unsigned int pathid)
{
	unsigned int i, j;
	printf("Data1, BufIdx=%d, FrmIdx[%d]=%d\r\n", g_Data1_Tx_PrevHeaderBufIdx, pathid, g_SocketCliData1_IdxInGOP[pathid]);
	for(i=0;i<ETHCAM_PREVHEADER_MAX_FRMNUM;i++){
		printf("[%d] ", i);
		for(j=0;j<ETHCAM_FRM_HEADER_SIZE;j++){

			printf("%02x, ",  g_Data1_Tx_PrevHeader[pathid][i][j]);
		}
		printf("\r\n");
	}

	return 0;
}
unsigned int ethsocket_cli_ipc_Data2_DumpBsInfo(unsigned int pathid)
{
	unsigned int i, j;
	printf("Data2, BufIdx=%d, FrmIdx[%d]=%d\r\n", g_Data2_Tx_PrevHeaderBufIdx, pathid, g_SocketCliData2_IdxInGOP[pathid]);

	for(i=0;i<ETHCAM_PREVHEADER_MAX_FRMNUM;i++){
		printf("[%d] ", i);
		for(j=0;j<ETHCAM_FRM_HEADER_SIZE;j++){

			printf("%02x, ",  g_Data2_Tx_PrevHeader[pathid][i][j]);
		}
		printf("\r\n");
	}

	return 0;
}




static unsigned char* ethsocket_cli_ipc_Data1_GetBsAlignAddr(unsigned int path_id ,unsigned int uiWantSize)
{
	unsigned char *pOutAddr[ETHCAM_PATH_ID_MAX] = {NULL};
	static unsigned int uiSize[ETHCAM_PATH_ID_MAX] = {0};

	if(g_SocketCliData1_BsQueueIdx[path_id] ==0 && g_SocketCliData1_BsBufMapTbl[path_id][0]==0){
	}else{
		g_SocketCliData1_BsQueueIdx[path_id]++;
	}
	if(g_SocketCliData1_BsQueueIdx[path_id]>=1){
		uiSize[path_id]= uiSize[path_id] + g_SocketCliData1_BsBufMapTbl[path_id][g_SocketCliData1_BsQueueIdx[path_id]-1];
	}else{
		uiSize[path_id]=0;
	}
	while (uiSize[path_id] % 4 != 0)
	{
		uiSize[path_id] += 1;
	}
	if(g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_0] < (uiWantSize + uiSize[path_id])){
		//printf("BsSize  reset!!\r\n");
		uiSize[path_id]=0;
		g_SocketCliData1_BsQueueIdx[path_id]=0;
	}
	if(g_SocketCliData1_BsQueueIdx[path_id] >= g_SocketCliData_BsQueueMax[path_id][ETHSOCKIPCCLI_ID_0]){
		//printf("BsQueueIdx reset!!\r\n");
		uiSize[path_id]=0;
		g_SocketCliData1_BsQueueIdx[path_id]=0;
	}
	pOutAddr[path_id] = (unsigned char* )(g_SocketCliData1_BsAddr[path_id]+ uiSize[path_id]);
	if (pOutAddr[path_id]) {
		g_SocketCliData1_BsBufMapTbl[path_id][g_SocketCliData1_BsQueueIdx[path_id]] = uiWantSize;
	}
	//printf("inSz=%d, Size=%d, pAddr=0x%x, endAddr=0x%x\r\n",uiWantSize,uiSize[path_id],pOutAddr[path_id],g_SocketCliData1_BsQueueIdx[path_id]);
	return pOutAddr[path_id];
}
void ethsocket_cli_ipc_Data1_RecvResetParam(ETHCAM_PATH_ID path_id)
{
	g_SocketCliData1_RecvSize[path_id]=0;
	g_SocketCliData1_BsSize[path_id]=0;
	g_SocketCliData1_BsStartPos[path_id]=0;
	g_SocketCliData1_BsQueueIdx[path_id]=0;
	g_SocketCliData1_LongConterStampEn[path_id]=0;
	g_SocketCliData1_BsFrameCnt[path_id]=0;
	g_SocketCliData1_DescSize[path_id]=0;
	g_SocketCliData1_BsSizeOverflow[path_id]=0;
	g_SocketCliData1_ErrCnt[path_id]=0;

	g_SocketCliData1_BsFrmBufAddr[path_id]=NULL;
	unsigned int i ;
	for(i=0;i<LONGCNT_STAMP_LENGTH;i++){
		g_SocketCliData1_LongConterByteBK[path_id][i]=0;
	}
	g_SocketCliData_DebugRecvSize[path_id][0]=0;

}
#if 0 // perf test
#define TIMER0_COUNTER_REG          0xC0040108
#define ipc_getMmioAddr(addr)       (addr)
static unsigned int drv_getSysTick(void)
{
    volatile unsigned int* p1 = (volatile unsigned int*)ipc_getMmioAddr(TIMER0_COUNTER_REG);
    return *p1;

}
unsigned int time1;
#endif
void ethsocket_cli_ipc_Data1_lock(void)
{
	return;
    //cyg_mutex_lock(&ethsocket_cli_ipc_mutex);
}
void ethsocket_cli_ipc_Data1_unlock(void)
{
	return;
    //cyg_mutex_unlock(&ethsocket_cli_ipc_mutex);
}
void ethsocket_cli_ipc_Data1_ParseBsFrame(unsigned int path_id, char* addr, int size)
{
	unsigned int i;
	unsigned char* pRecvAddr=NULL;
	unsigned int bPushData=0;
	static unsigned int bChkData[ETHCAM_PATH_ID_MAX]={0};
	int result =0;
	int Ret_EthsocketApi = 0;
	static int errCnt=0;
	static unsigned int Prev_BsSize[ETHCAM_PATH_ID_MAX]={0};
	static unsigned int Prev_IdxInGOP[ETHCAM_PATH_ID_MAX]={0};
	static unsigned int Prev_StartPos[ETHCAM_PATH_ID_MAX]={0};

	unsigned char *uiBsFrmBufAddr=NULL;
	unsigned int  uiFrmBuf=0;

	//diag_printf("eeD1_sz[%d]=%d\r\n",path_id,size);
	//return;
	ethsocket_cli_ipc_Data1_lock();

	if(g_SocketCliData1_RecvAddr[path_id]==0){
		ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"RecvAddr no buffer!\r\n");
		ethsocket_cli_ipc_Data1_unlock();

		return;
	}

#if 0  //if 0 for new method
	if((g_SocketCliData1_RecvSize[path_id]+ size) >=gSndParamSize[path_id][ETHSOCKIPCCLI_ID_0]){
		if(g_SocketCliData1_ErrCnt[path_id] % 300 == 0){
			unsigned char* addr_tmp= (unsigned char*)g_SocketCliData1_RecvAddr[path_id];
			ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"over flow Recv Size=%d, wantSize=%d, size=%d\r\n",g_SocketCliData1_RecvSize[path_id],gSndParamSize[path_id][ETHSOCKIPCCLI_ID_0],size);
			ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"BsCnt=%d, chkD=%d, oldPos=%d, oldIdx=%d, oldBsSz=%d,  startPos=%d, idx=%d, BsSz=%d, DescSize=%d\r\n",g_SocketCliData1_BsFrameCnt[path_id],bChkData[path_id], Prev_StartPos[path_id], Prev_IdxInGOP[path_id],Prev_BsSize[path_id],g_SocketCliData1_BsStartPos[path_id],g_SocketCliData1_IdxInGOP[path_id],g_SocketCliData1_BsSize[path_id],g_SocketCliData1_DescSize[path_id]);

			//ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"i+0=0x%x,%x,%x,%x,%x,%x,%x,%x,%x,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",pRecvAddr[0] ,pRecvAddr[1],pRecvAddr[2],pRecvAddr[3],pRecvAddr[4],pRecvAddr[5],pRecvAddr[6],pRecvAddr[7],pRecvAddr[8],pRecvAddr[9],pRecvAddr[10],pRecvAddr[11],pRecvAddr[12],pRecvAddr[13],pRecvAddr[14],pRecvAddr[15],pRecvAddr[16],pRecvAddr[17],pRecvAddr[18],pRecvAddr[19],pRecvAddr[20],pRecvAddr[21],pRecvAddr[22],pRecvAddr[23],pRecvAddr[24]);
			ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"i+0=0x%x,%x,%x,%x,%x,%x,%x,%x,%x,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",addr_tmp[0] ,addr_tmp[1],addr_tmp[2],addr_tmp[3],addr_tmp[4],addr_tmp[5],addr_tmp[6],addr_tmp[7],addr_tmp[8],addr_tmp[9],addr_tmp[10],addr_tmp[11],addr_tmp[12],addr_tmp[13],addr_tmp[14],addr_tmp[15],addr_tmp[16],addr_tmp[17],addr_tmp[18],addr_tmp[19],addr_tmp[20],addr_tmp[21],addr_tmp[22],addr_tmp[23],addr_tmp[24]);


			ETHSOCKETCLI_PARAM_PARAM *pNotifyParam = (ETHSOCKETCLI_PARAM_PARAM *)ethsocket_cli_ipc_GetSndAddr(path_id, ETHSOCKIPCCLI_ID_0);
			pNotifyParam->param1 = (int)CYG_ETHSOCKETCLI_STATUS_CLIENT_SLOW;
			pNotifyParam->param2 = ERR_SLOW_RECV_OVER_FLOW;
			ipc_flushCache((NVTIPC_U32)pNotifyParam, (NVTIPC_U32)sizeof(ETHSOCKETCLI_PARAM_PARAM));
			result = ethsocket_cli_ipc_exeapi(path_id, ETHSOCKIPCCLI_ID_0, ETHSOCKETCLI_CMDID_NOTIFY, &Ret_EthsocketApi);
			if ((result != ETHSOCKETCLI_RET_OK) || (Ret_EthsocketApi!=0)) {
				ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"snd Notify fail\r\n");
			}
		}
		g_SocketCliData1_RecvSize[path_id]=0;
		g_SocketCliData1_ErrCnt[path_id]++;

#if 0//debug, write to card
		ETHSOCKETCLI_RECV_PARAM *pParam = (ETHSOCKETCLI_RECV_PARAM *)ethsocket_cli_ipc_GetSndAddr(ETHSOCKIPCCLI_ID_0);

		if(bFirstErr){
			bFirstErr=0;
			pParam->size = (int)(g_SocketCliData1_BsSize);
			pParam->buf=(char *)(g_SocketCliData1_RecvAddr);
			result = ethsocket_cli_ipc_exeapi(ETHSOCKIPCCLI_ID_0, ETHSOCKETCLI_CMDID_RCV, &Ret_EthsocketApi);
			if ((result != ETHSOCKETCLI_RET_OK) || (Ret_EthsocketApi!=0)) {
				ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"snd CB fail\r\n");
			}
		}
		memcpy((char*)(g_SocketCliData1_RecvAddr), (char*)addr, size);
		pParam->size = (int)(size);
		pParam->buf=(char *)(g_SocketCliData1_RecvAddr);

		result = ethsocket_cli_ipc_exeapi(ETHSOCKIPCCLI_ID_0, ETHSOCKETCLI_CMDID_RCV, &Ret_EthsocketApi);
		if ((result != ETHSOCKETCLI_RET_OK) || (Ret_EthsocketApi!=0)) {
			ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"snd CB fail\r\n");
		}
#endif
		ethsocket_cli_ipc_Data1_unlock();

		return;
	}
#endif
	if(g_SocketCliData1_BsSizeOverflow[path_id] && (g_SocketCliData1_BsSize[path_id] >=gSndParamSize[path_id][ETHSOCKIPCCLI_ID_0])){
		if(g_SocketCliData1_ErrCnt[path_id] % 300 == 0){
			ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"BsSizeOverflow!\r\n");
		}
		g_SocketCliData1_ErrCnt[path_id]++;
		//ethsocket_cli_ipc_Data1_RecvResetParam(path_id);
		return;
	}
	//memcpy((char*)(g_SocketCliData1_RecvAddr[path_id]+g_SocketCliData1_RecvSize[path_id]), (char*)addr, size);
	g_SocketCliData1_RecvSize[path_id]+=size;
	//pRecvAddr = (unsigned char*)g_SocketCliData1_RecvAddr[path_id];
	if(g_SocketCliData1_BsFrmBufAddr[path_id]==NULL){
		pRecvAddr = (unsigned char*)g_SocketCliData1_RecvAddr[path_id];
	}else{
		pRecvAddr = g_SocketCliData1_BsFrmBufAddr[path_id];
	}

#if 0
	if((pRecvAddr[i+0] ==0 && pRecvAddr[i+1] ==0 && pRecvAddr[i+2] ==0 && pRecvAddr[i+3] ==1) &&
		((pRecvAddr[i+4]& 0x1F) == H264_NALU_TYPE_IDR)){

		//diag_printf("eeD1_I[%d]\r\n",path_id);

		if(g_SocketCliData1_RecvSize[path_id] != 0){
			//jump to next data
			memmove((unsigned char *)pRecvAddr, (unsigned char *)(pRecvAddr + g_SocketCliData1_BsSize[path_id]), g_SocketCliData1_RecvSize[path_id]);
			//diag_printf("move 0x%x, 0x%x, %d\r\n",pRecvAddr, pRecvAddr + g_SocketCliData1_BsSize + g_SocketCliData1_BsStartPos, g_SocketCliData1_RecvSize);
		}
		g_SocketCliData1_BsSize[path_id]=0;

		ethsocket_cli_ipc_Data1_unlock();

		return;
	}
#endif
	#if 0
	//if(g_SocketCliData1_RecvSize>=MAX_I_FRAME_SZIE){
	if(g_SocketCliData1_RecvSize>=gSndParamSize[ETHSOCKIPCCLI_ID_0]){
		ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"over flow CliSocket_size_get=%d, size=%d\r\n",g_SocketCliData1_RecvSize,size);
		return;
	}
	#endif
	while(1)
	{
		//diag_printf("pRecvAddr=0x%x, %x, %x, %x, %x, %x, %x, %x\r\n",  pRecvAddr[0] ,  pRecvAddr[0+1] ,  pRecvAddr[0+2] ,  pRecvAddr[0+3] ,  pRecvAddr[0+4] ,  pRecvAddr[0+5] ,  pRecvAddr[0+6] ,  pRecvAddr[0+7]);
		for(i=0;(i<g_SocketCliData1_RecvSize[path_id]) && (g_SocketCliData1_BsSize[path_id] ==0);i++)
		{
			if(g_SocketCliData1_BsSize[path_id] ==0 && ((i+6) < g_SocketCliData1_RecvSize[path_id]) && (pRecvAddr[i+0] ==0 && pRecvAddr[i+1] ==0 && pRecvAddr[i+2] ==0 && pRecvAddr[i+3] ==1)  &&
				(
				((pRecvAddr[i+4]& 0x1F) == H264_TYPE_IDR && pRecvAddr[i+5] == H264_START_CODE_I)
				||((pRecvAddr[i+4]& 0x1F) == H264_TYPE_SLICE  && pRecvAddr[i+5] == H264_START_CODE_P)
				//||(pRecvAddr[i+4]& 0x1F) == H264_TYPE_SPS
				||((g_SocketCliData1_RecvSize[path_id] > (5 +2)) && i>3 && ((pRecvAddr[i+4]& 0x1F) == H264_TYPE_PPS) && ((pRecvAddr[i-2]+ (pRecvAddr[i-1]<<8))>0) )

				//||((pRecvAddr[i+4]>> 0x1) == H265_TYPE_IDR )
				||( (((pRecvAddr[i+4]>> 0x1)&0x3F) == H265_TYPE_SLICE)  )
				//||(pRecvAddr[i+4]>> 0x1) == H265_TYPE_SPS
				//||(pRecvAddr[i+4]>> 0x1) == H265_TYPE_PPS
				||( (((pRecvAddr[i+4]>> 0x1)&0x3F) == H265_TYPE_VPS) && ((((pRecvAddr[i+4] & 0x01)<<6) | (pRecvAddr[i+5] >>3) ) ==0) && ((pRecvAddr[i+4] & 0x80) ==0) )//for IDR, and Layer id =0, forbidden bit=0

				||(pRecvAddr[i+4] == HEAD_TYPE_THUMB && pRecvAddr[i+5] == 0xFF && pRecvAddr[i+6] == 0xD8)
				||(pRecvAddr[i+4] == HEAD_TYPE_RAW_ENCODE && pRecvAddr[i+5] == 0xFF && pRecvAddr[i+6] == 0xD8))){
				//Prev_BsSize[path_id]=g_SocketCliData1_BsSize[path_id];
				//g_bs_size_total=pdata[i-4]+ (pdata[i-3]<<8)+ (pdata[i-2]<<16) + (pdata[i-1]<<24);
				if((g_SocketCliData1_RecvSize[path_id] >= (5 +8)) && (i>=8) &&  (pRecvAddr[i-1]==0 || pRecvAddr[i-1]==LONGCNT_STAMP_EN)  && (pRecvAddr[i-5]>0 && pRecvAddr[i-5]<=180) && (pRecvAddr[i-2]==0) && ((unsigned int)(pRecvAddr[i-8]+ (pRecvAddr[i-7]<<8)+ (pRecvAddr[i-6]<<16)) <g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_0]) &&
					((pRecvAddr[i+4]& 0x1F) == H264_TYPE_IDR
					||(pRecvAddr[i+4]& 0x1F) == H264_TYPE_SLICE)  ){  //5: 0 0 0 1 xx, 8: header frame siz(3) +fr idx(1) + chksum(2) + nouse(1) + longc_en(1)
					//g_SocketCliData1_BsSize=pRecvAddr[i-4]+ (pRecvAddr[i-3]<<8)+ (pRecvAddr[i-2]<<16);
					g_SocketCliData1_BsSize[path_id]=pRecvAddr[i-8]+ (pRecvAddr[i-7]<<8)+ (pRecvAddr[i-6]<<16);
					g_SocketCliData1_checkSumHead[path_id]=pRecvAddr[i-4];
					g_SocketCliData1_checkSumTail[path_id]=pRecvAddr[i-3];
					g_SocketCliData1_LongConterStampEn[path_id]=pRecvAddr[i-1];
					if(g_SocketCliData1_LongConterStampEn[path_id]==LONGCNT_STAMP_EN && (i>=16)){
						g_SocketCliData1_LongConterLow[path_id]=(pRecvAddr[i-12]<<24)+ (pRecvAddr[i-11]<<16)+ (pRecvAddr[i-10]<<8) + (pRecvAddr[i-9]);
						g_SocketCliData1_LongConterHigh[path_id]=(pRecvAddr[i-16]<<24)+ (pRecvAddr[i-15]<<16)+ (pRecvAddr[i-14]<<8) + (pRecvAddr[i-13]);
					}else{
						g_SocketCliData1_LongConterLow[path_id]=0;
						g_SocketCliData1_LongConterHigh[path_id]=0;
					}
					//diag_printf("longcnt[%d]=%d, %d\r\n",path_id,g_SocketCliData1_LongConterHigh[path_id],g_SocketCliData1_LongConterLow[path_id]);
					//diag_printf("pRecvAddr[i+4]=0x%x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x\r\n",  pRecvAddr[i+4], pRecvAddr[i+3] ,  pRecvAddr[i+2] ,  pRecvAddr[i+1], pRecvAddr[i], pRecvAddr[i-1] ,  pRecvAddr[i-2] ,  pRecvAddr[i-3] ,  pRecvAddr[i-4] ,  pRecvAddr[i-5] ,  pRecvAddr[i-6] ,  pRecvAddr[i-7] ,  pRecvAddr[i-8]);
					//diag_printf("pRecvAddr[i-9]=0x%x, %x, %x, %x, %x, %x, %x, %x\r\n",  pRecvAddr[i-9] ,  pRecvAddr[i-10] ,  pRecvAddr[i-11] ,  pRecvAddr[i-12] ,  pRecvAddr[i-13] ,  pRecvAddr[i-14] ,  pRecvAddr[i-15] ,  pRecvAddr[i-16]);
					if((pRecvAddr[i+4]& 0x1F) == H264_TYPE_IDR){
						bChkData[path_id]=0x21;
					}else{
						bChkData[path_id]=0x22;
					}
				}else if((g_SocketCliData1_RecvSize[path_id] >= (5 +8)) && (i>=8) &&  (pRecvAddr[i-1]==0 || pRecvAddr[i-1]==LONGCNT_STAMP_EN) && ((unsigned int)(pRecvAddr[i-8]+ (pRecvAddr[i-7]<<8)+ (pRecvAddr[i-6]<<16)) <g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_0]) &&
					(( (((pRecvAddr[i+4]>> 0x1)&0x3F) == H265_TYPE_VPS) && (pRecvAddr[i-2]>0) && (pRecvAddr[i-5]==1))//H265_TYPE_IDR
					||( (((pRecvAddr[i+4]>> 0x1)&0x3F) == H265_TYPE_SLICE) && (pRecvAddr[i-2]==0) &&  (pRecvAddr[i-5]>1 && pRecvAddr[i-5]<=180)))  ){  //5: 0 0 0 1 xx, 8: header frame siz(3) +fr idx(1) + chksum(2) + nouse(1) + longc_en(1)
					//g_SocketCliData1_BsSize=pRecvAddr[i-4]+ (pRecvAddr[i-3]<<8)+ (pRecvAddr[i-2]<<16);
					g_SocketCliData1_BsSize[path_id]=pRecvAddr[i-8]+ (pRecvAddr[i-7]<<8)+ (pRecvAddr[i-6]<<16);
					g_SocketCliData1_checkSumHead[path_id]=pRecvAddr[i-4];
					g_SocketCliData1_checkSumTail[path_id]=pRecvAddr[i-3];
					g_SocketCliData1_LongConterStampEn[path_id]=pRecvAddr[i-1];
					g_SocketCliData1_DescSize[path_id]=pRecvAddr[i-2];
					if(g_SocketCliData1_LongConterStampEn[path_id]==LONGCNT_STAMP_EN && (i>=16)){
						g_SocketCliData1_LongConterLow[path_id]=(pRecvAddr[i-12]<<24)+ (pRecvAddr[i-11]<<16)+ (pRecvAddr[i-10]<<8) + (pRecvAddr[i-9]);
						g_SocketCliData1_LongConterHigh[path_id]=(pRecvAddr[i-16]<<24)+ (pRecvAddr[i-15]<<16)+ (pRecvAddr[i-14]<<8) + (pRecvAddr[i-13]);
					}else{
						g_SocketCliData1_LongConterLow[path_id]=0;
						g_SocketCliData1_LongConterHigh[path_id]=0;
					}
					//diag_printf("longcnt[%d]=%d, %d\r\n",path_id,g_SocketCliData1_LongConterHigh[path_id],g_SocketCliData1_LongConterLow[path_id]);
					//diag_printf("pRecvAddr[i+4]=0x%x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x\r\n",  pRecvAddr[i+4], pRecvAddr[i+3] ,  pRecvAddr[i+2] ,  pRecvAddr[i+1], pRecvAddr[i], pRecvAddr[i-1] ,  pRecvAddr[i-2] ,  pRecvAddr[i-3] ,  pRecvAddr[i-4] ,  pRecvAddr[i-5] ,  pRecvAddr[i-6] ,  pRecvAddr[i-7] ,  pRecvAddr[i-8]);
					//diag_printf("pRecvAddr[i-9]=0x%x, %x, %x, %x, %x, %x, %x, %x\r\n",  pRecvAddr[i-9] ,  pRecvAddr[i-10] ,  pRecvAddr[i-11] ,  pRecvAddr[i-12] ,  pRecvAddr[i-13] ,  pRecvAddr[i-14] ,  pRecvAddr[i-15] ,  pRecvAddr[i-16]);
					bChkData[path_id]=2;
					if( ((pRecvAddr[i+4]>> 0x1)&0x3F) == H265_TYPE_VPS  ){
						bChkData[path_id]=0x41;
					}else{
						bChkData[path_id]=0x42;
					}

				}else if((g_SocketCliData1_RecvSize[path_id] >= (5 + 4)) && (i>=4) && (pRecvAddr[i+4] == HEAD_TYPE_THUMB || pRecvAddr[i+4] == HEAD_TYPE_RAW_ENCODE) ){
					g_SocketCliData1_BsSize[path_id]=pRecvAddr[i-4]+ (pRecvAddr[i-3]<<8)+ (pRecvAddr[i-2]<<16);
					bChkData[path_id]=3;
				//}else if (( (pRecvAddr[i+4]& 0x1F) == H264_TYPE_SPS  ||(pRecvAddr[i+4]& 0x1F) == H264_TYPE_PPS)  && (g_SocketCliData1_RecvSize[path_id] >= (5 +2)) && (i>=2)){//sps, pps
				}else if ((g_SocketCliData1_RecvSize[path_id] >= (5 +2)) && (i>=2) && ((pRecvAddr[i+4]& 0x1F) == H264_TYPE_PPS)  ){//sps, pps
					//diag_printf("PPS i=%d, R[i+4]=0x%x\r\n",i,pRecvAddr[i+4]);
					bChkData[path_id]=4;

					unsigned int j;
					unsigned int IsGetSPS=0;
					for(j=0;j<i ;j++){
						if(pRecvAddr[j+0]==0 &&pRecvAddr[j+1]==0 &&pRecvAddr[j+2]==0 &&pRecvAddr[j+3]==1 && ((pRecvAddr[j+4]& 0x1F) == H264_TYPE_SPS)){
							IsGetSPS=1;
							g_SocketCliData1_DescSize[path_id]=pRecvAddr[j-2]+ (pRecvAddr[j-1]<<8);
							break;
						}
					}
					if(IsGetSPS==0){
						//diag_printf("==========Fail Get SPS============\r\n");
						continue;
					}
					#if 0
					g_SocketCliData1_BsSize[path_id]=pRecvAddr[i-2]+ (pRecvAddr[i-1]<<8);
					bChkData[path_id]=4;
					if((pRecvAddr[i+4]& 0x1F) == H264_TYPE_SPS){
						g_SocketCliData1_DescSize[path_id]=g_SocketCliData1_BsSize[path_id];
					}else{
						g_SocketCliData1_DescSize[path_id] +=g_SocketCliData1_BsSize[path_id];
					}
					#else
					g_SocketCliData1_BsSize[path_id]=pRecvAddr[i-2]+ (pRecvAddr[i-1]<<8);
					g_SocketCliData1_DescSize[path_id] +=g_SocketCliData1_BsSize[path_id];
					#endif
				}
				//else if (( (pRecvAddr[i+4]>> 0x1) == H265_TYPE_SPS  ||(pRecvAddr[i+4]>> 0x1) == H265_TYPE_PPS ||(pRecvAddr[i+4]>> 0x1) == H265_TYPE_VPS)  && (g_SocketCliData1_RecvSize[path_id] >= (5 +2)) && (i>=2)){//sps, pps, vps
				//	g_SocketCliData1_BsSize[path_id]=pRecvAddr[i-2]+ (pRecvAddr[i-1]<<8);
				//	bChkData=3;
				//}


				//if(g_SocketCliData1_BsSize[path_id] >=gSndParamSize[path_id][ETHSOCKIPCCLI_ID_0]){
				if(g_SocketCliData1_BsSize[path_id] >=g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_0] ){

					//ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"Frm Size Error!! sz=%d\r\n",g_SocketCliData1_BsSize[path_id]);
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"Bs Size overflow i=%d oldidx=%d, oldBsSz=%d, wanted[%d]=%d, but now=%d ,RecvSz=%d,bChkData=%d,DescSize=%d\r\n",i,Prev_IdxInGOP[path_id],Prev_BsSize[path_id],path_id,gSndParamSize[path_id][ETHSOCKIPCCLI_ID_0],g_SocketCliData1_BsSize[path_id], g_SocketCliData1_RecvSize[path_id],bChkData[path_id],g_SocketCliData1_DescSize[path_id]);
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"i+0=0x%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",pRecvAddr[i+0] ,pRecvAddr[i+1],pRecvAddr[i+2],pRecvAddr[i+3],pRecvAddr[i+4],pRecvAddr[i+5],pRecvAddr[i+6],pRecvAddr[i+7],pRecvAddr[i+8]);
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"i-8=0x%x,%x,%x,%x,%x,%x,%x,%x\r\n",pRecvAddr[i-8] ,pRecvAddr[i-7],pRecvAddr[i-6],pRecvAddr[i-5],pRecvAddr[i-4],pRecvAddr[i-3],pRecvAddr[i-2],pRecvAddr[i-1]);

					ETHSOCKETCLI_PARAM_PARAM *pNotifyParam = (ETHSOCKETCLI_PARAM_PARAM *)ethsocket_cli_ipc_GetSndAddr(path_id, ETHSOCKIPCCLI_ID_0);
					pNotifyParam->param1 = (int)CYG_ETHSOCKETCLI_STATUS_CLIENT_SLOW;
					pNotifyParam->param2 = ERR_SLOW_BS_OVER_FLOW;
					ipc_flushCache((NVTIPC_U32)pNotifyParam, (NVTIPC_U32)sizeof(ETHSOCKETCLI_PARAM_PARAM));
					result = ethsocket_cli_ipc_exeapi(path_id, ETHSOCKIPCCLI_ID_0, ETHSOCKETCLI_CMDID_NOTIFY, &Ret_EthsocketApi);
					if ((result != ETHSOCKETCLI_RET_OK) || (Ret_EthsocketApi!=0)) {
						ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"snd Notify fail\r\n");
					}
					ethsocket_cli_ipc_Data1_unlock();
					g_SocketCliData1_BsSizeOverflow[path_id]=1;
					//ethsocket_cli_ipc_Data1_RecvResetParam(path_id);

					return;
				}
				if(g_SocketCliData1_BsSize[path_id]){
					g_SocketCliData1_BsStartPos[path_id]=i;//4;//size_get;//remove size header
					//g_SocketCliData1_IdxInGOP=pRecvAddr[i-1];
					if(i>=5){
						g_SocketCliData1_IdxInGOP[path_id]=pRecvAddr[i-5];
					}
					if(pRecvAddr[i+4] == HEAD_TYPE_THUMB || pRecvAddr[i+4] == HEAD_TYPE_RAW_ENCODE){
						g_SocketCliData1_BsSize[path_id]+=5; //start code
						//diag_printf("user_id=%d, start=%d, size_total=%d\r\n",g_SocketCliData1_IdxInGOP,g_SocketCliData1_BsStartPos,g_SocketCliData1_BsSize);
					}else{
						//diag_printf("frm_idx=%d, start=%d, size_total=%d\r\n",g_SocketCliData1_IdxInGOP[path_id],g_SocketCliData1_BsStartPos[path_id],g_SocketCliData1_BsSize[path_id]);
					}
					bChkData[path_id]|=0x10;
					#if 1 //new method to reduce memcpy time
					//unsigned char *uiBsFrmBufAddr=NULL;
					unsigned int LongCounterOffset=0;
					unsigned int DescOffset=0;  //for H265 I Frame Bs Align 4, for VdoDec

					unsigned int AudCapOffset=0;
					#if 0
					g_SocketCliData1_AudCapEn[path_id]=0;
					g_SocketCliData1_AudCapTotalSize[path_id]=0;

					if(g_SocketCliData1_RecvSize[path_id] >=ETHCAM_AUDCAP_FRAME_SIZE){
						//unsigned char* addr_tmp=pRecvAddr+ g_SocketCliData1_BsStartPos[path_id] +g_SocketCliData1_BsSize[path_id];
						unsigned char* addr_tmp=pRecvAddr;
						//unsigned int i_tmp=g_SocketCliData1_BsStartPos[path_id] ;
						//diag_printf(" StartPos=%d, BsSize=%d,RecvSize=%d,  i+0=0x%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n", g_SocketCliData1_BsStartPos[path_id] ,g_SocketCliData1_BsSize[path_id],g_SocketCliData1_RecvSize[path_id],pRecvAddr[i_tmp+0] ,pRecvAddr[i_tmp+1],pRecvAddr[i_tmp+2],pRecvAddr[i_tmp+3],pRecvAddr[i_tmp+4],pRecvAddr[i_tmp+5],pRecvAddr[i_tmp+6],pRecvAddr[i_tmp+7],pRecvAddr[i_tmp+8]);
						//diag_printf("StartPos=%d, BsSize=%d,RecvSize=%d,  addr_tmp[0]=0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",g_SocketCliData1_BsStartPos[path_id] ,g_SocketCliData1_BsSize[path_id],g_SocketCliData1_RecvSize[path_id],addr_tmp[0] ,addr_tmp[1],addr_tmp[2],addr_tmp[3],addr_tmp[4],addr_tmp[5],addr_tmp[6],addr_tmp[7],addr_tmp[8],addr_tmp[9],addr_tmp[10],addr_tmp[11],addr_tmp[12],addr_tmp[13],addr_tmp[14]);

						if((addr_tmp[0]> 0)&&((addr_tmp[1] + (addr_tmp[2]<<8))>0) && addr_tmp[3+0]==0 && addr_tmp[3+1]==0 && addr_tmp[3+2]==0 && addr_tmp[3+3]==1
							&& addr_tmp[3+4]==HEAD_TYPE_AUDCAP && addr_tmp[3+5]==0xff){ //[6]=0xff for AAC
							g_SocketCliData1_AudCapEn[path_id]=1;
							g_SocketCliData1_AudCapTotalSize[path_id]=ETHCAM_AUDCAP_FRAME_SIZE;//addr_tmp[7] + (addr_tmp[8]<<8);
							//diag_printf("AudCap !\r\n");
						}
					}
					#endif
					if(((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_IDR && (*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +5)) == H264_START_CODE_I) ||
						((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_SLICE && (*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +5)) == H264_START_CODE_P)){

						//printf("2244 %d, %d\r\n",g_SocketCliData1_IdxInGOP[path_id],g_SocketCliData1_BsSize[path_id]);
						if(g_SocketCliData1_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
							LongCounterOffset=16; //(8+5) and align 4
						}else{
							LongCounterOffset=0;
						}
						if(((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_IDR)){
							//printf("22[%d]h264 DescSize=%d, bsSz=%d\r\n",path_id,g_SocketCliData1_DescSize[path_id],g_SocketCliData1_BsSize[path_id]);
							if(g_SocketCliData1_DescSize[path_id] == 0){
								ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"h264 DescSize 0!!\r\n");
								return;
							}
							if(g_SocketCliData1_DescSize[path_id]%4){
								DescOffset=4-(g_SocketCliData1_DescSize[path_id]%4);
							}
							DescOffset +=g_SocketCliData1_DescSize[path_id];//add offset for sps + pps
							DescOffset+=TS_VIDEO_RESERVED_SIZE;
						}else{
							DescOffset=TS_VIDEO_RESERVED_SIZE;
						}
						//if(g_SocketCliData1_AudCapEn[path_id]==1 && g_SocketCliData1_AudCapTotalSize[path_id]){
						//	AudCapOffset=g_SocketCliData1_AudCapTotalSize[path_id];
						//}else{
						//	AudCapOffset=0;
						//}
						#if 0
						if(((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_IDR)){
							if(g_SocketCliData1_DescSize[path_id] == 0){
								if(errCnt % 300 == 0){
									ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"h264 DescSize 0!!\r\n");
								}
								errCnt++;
								return;
							}
							if(g_SocketCliData1_DescSize[path_id]%4){
								DescOffset=4-(g_SocketCliData1_DescSize[path_id]%4);
							}
							DescOffset +=g_SocketCliData1_DescSize[path_id];//add offset for sps + pps
							DescOffset+=TS_VIDEO_RESERVED_SIZE;
						}else{
							DescOffset=TS_VIDEO_RESERVED_SIZE;
						}
						#endif
						//DescOffset=0;
						//uiBsFrmBufAddr =ethsocket_cli_ipc_Data1_GetBsAlignAddr(path_id, (DescOffset+g_SocketCliData1_BsSize[path_id]+LongCounterOffset+AudCapOffset));
						//uiBsFrmBufAddr =ethsocket_cli_ipc_Data1_GetBsAlignAddr(path_id, (DescOffset+g_SocketCliData1_RecvSize[path_id]+LongCounterOffset+AudCapOffset));
						uiFrmBuf = (g_SocketCliData1_BsSize[path_id] > g_SocketCliData1_RecvSize[path_id] ) ? g_SocketCliData1_BsSize[path_id] : g_SocketCliData1_RecvSize[path_id] ;
						uiBsFrmBufAddr =ethsocket_cli_ipc_Data1_GetBsAlignAddr(path_id, (DescOffset+uiFrmBuf+LongCounterOffset+AudCapOffset));

						g_SocketCliData1_BsFrmBufAddr[path_id]=uiBsFrmBufAddr+DescOffset;
						memcpy((unsigned char*)(uiBsFrmBufAddr+DescOffset), (unsigned char*)(pRecvAddr+ g_SocketCliData1_BsStartPos[path_id]), (g_SocketCliData1_RecvSize[path_id]- g_SocketCliData1_BsStartPos[path_id]));
						//if(AudCapOffset){
						//	memcpy((unsigned char*)(uiBsFrmBufAddr+DescOffset+g_SocketCliData1_BsSize[path_id]), (unsigned char*)(pRecvAddr), AudCapOffset);
						//}

						memset((unsigned char*)uiBsFrmBufAddr, 0 ,DescOffset);

						g_SocketCliData1_RecvSize[path_id]-= g_SocketCliData1_BsStartPos[path_id];
						g_SocketCliData1_BsStartPos[path_id]=0;
						pRecvAddr = g_SocketCliData1_BsFrmBufAddr[path_id];


					}else if((((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)>> 0x1)&0x3F) == H265_TYPE_VPS )
						||(((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)>> 0x1)&0x3F) == H265_TYPE_SLICE)
					){

						//unsigned char *uiBsFrmBufAddr=NULL;
						//unsigned int LongCounterOffset=0;
						//unsigned int DescOffset=0;  //for H265 I Frame Bs Align 4, for VdoDec
						if(g_SocketCliData1_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
							LongCounterOffset=16; //(8+5) and align 4
						}else{
							LongCounterOffset=0;
						}
						if(((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)>> 0x1) == H265_TYPE_VPS )){
							if(g_SocketCliData1_DescSize[path_id] == 0){
								if(errCnt % 300 == 0){
									ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"h265 DescSize 0!!\r\n");
								}
								errCnt++;
								return;
							}
							if(g_SocketCliData1_DescSize[path_id]%4){
								DescOffset=4-(g_SocketCliData1_DescSize[path_id]%4);
							}
							DescOffset+=TS_VIDEO_RESERVED_SIZE;
						}else{
							DescOffset=TS_VIDEO_RESERVED_SIZE;
						}
						//unsigned int   uiTime =drv_getSysTick();

						//unsigned char*  addr_tmp= (unsigned char*)pRecvAddr;
						//diag_printf("11 i+0=%d,bssz=%d, recsz=%d ,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",g_SocketCliData2_IdxInGOP[path_id],g_SocketCliData2_BsSize[path_id] , g_SocketCliData2_RecvSize[path_id],addr_tmp[0] ,addr_tmp[1],addr_tmp[2],addr_tmp[3],addr_tmp[4],addr_tmp[5],addr_tmp[6],addr_tmp[7],addr_tmp[8],addr_tmp[9],addr_tmp[10],addr_tmp[11],addr_tmp[12],addr_tmp[13],addr_tmp[14],addr_tmp[15],addr_tmp[16],addr_tmp[17],addr_tmp[18],addr_tmp[19],addr_tmp[20],addr_tmp[21],addr_tmp[22],addr_tmp[23],addr_tmp[24]);

						//uiBsFrmBufAddr =ethsocket_cli_ipc_Data1_GetBsAlignAddr(path_id, (DescOffset+g_SocketCliData1_BsSize[path_id]+LongCounterOffset));
						//uiBsFrmBufAddr =ethsocket_cli_ipc_Data1_GetBsAlignAddr(path_id, (DescOffset+g_SocketCliData1_RecvSize[path_id]+LongCounterOffset));
						uiFrmBuf = (g_SocketCliData1_BsSize[path_id] > g_SocketCliData1_RecvSize[path_id] ) ? g_SocketCliData1_BsSize[path_id] : g_SocketCliData1_RecvSize[path_id] ;
						uiBsFrmBufAddr =ethsocket_cli_ipc_Data1_GetBsAlignAddr(path_id, (DescOffset+uiFrmBuf+LongCounterOffset));


						g_SocketCliData1_BsFrmBufAddr[path_id]=uiBsFrmBufAddr+DescOffset;

						//if((DescOffset+g_SocketCliData1_BsSize[path_id]+LongCounterOffset)  < (g_SocketCliData1_RecvSize[path_id]- g_SocketCliData1_BsStartPos[path_id])){
							//printf("warning bsSz=0x%x, ofs=%d, recvSz=0x%x, %d, ChkData=0x%x\r\n", g_SocketCliData1_BsSize[path_id],DescOffset, g_SocketCliData1_RecvSize[path_id],g_SocketCliData1_BsStartPos[path_id],bChkData[path_id]);
						//}

						memcpy((unsigned char*)(uiBsFrmBufAddr+DescOffset), (unsigned char*)(pRecvAddr+ g_SocketCliData1_BsStartPos[path_id]), (g_SocketCliData1_RecvSize[path_id]- g_SocketCliData1_BsStartPos[path_id]));

						if(AudCapOffset){
							memcpy((unsigned char*)(uiBsFrmBufAddr+DescOffset+g_SocketCliData1_BsSize[path_id]), (unsigned char*)(pRecvAddr), AudCapOffset);
						}

						memset((unsigned char*)uiBsFrmBufAddr, 0 ,DescOffset);

						g_SocketCliData1_RecvSize[path_id]-= g_SocketCliData1_BsStartPos[path_id];
						g_SocketCliData1_BsStartPos[path_id]=0;
						pRecvAddr = g_SocketCliData1_BsFrmBufAddr[path_id];

						//addr_tmp= (unsigned char*)g_SocketCliData2_BsFrmBufAddr;
						//diag_printf("22 i+0=%d,bssz=%d, recsz=%d ,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",g_SocketCliData2_IdxInGOP[path_id],g_SocketCliData2_BsSize[path_id] , g_SocketCliData2_RecvSize[path_id],addr_tmp[0] ,addr_tmp[1],addr_tmp[2],addr_tmp[3],addr_tmp[4],addr_tmp[5],addr_tmp[6],addr_tmp[7],addr_tmp[8],addr_tmp[9],addr_tmp[10],addr_tmp[11],addr_tmp[12],addr_tmp[13],addr_tmp[14],addr_tmp[15],addr_tmp[16],addr_tmp[17],addr_tmp[18],addr_tmp[19],addr_tmp[20],addr_tmp[21],addr_tmp[22],addr_tmp[23],addr_tmp[24]);

						//diag_printf("t1: %d us\r\n", drv_getSysTick() - uiTime);

						//g_CliSocket_size_frm[g_BsFrmQueueIdx]=g_bs_size_total;

					}
					#endif
					break;
				}
				//break;
			}
		}
		if(g_SocketCliData1_BsSize[path_id] >0 && (g_SocketCliData1_BsSize[path_id] + g_SocketCliData1_BsStartPos[path_id]) <= (g_SocketCliData1_RecvSize[path_id])){
			unsigned char *uiBsFrmBufAddr=NULL;
			unsigned int LongCounterOffset=0;
			unsigned int DescOffset=0;  //for H265 I Frame Bs Align 4, for VdoDec

			//if(g_queue_idx>=H264_QUEUE_MAX)
			//	g_queue_idx=0;
			bPushData=0;
			//I or P Frame
			if(((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_IDR && (*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +5)) == H264_START_CODE_I) ||
				((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_SLICE && (*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +5)) == H264_START_CODE_P)){

				#if 0
				if(g_SocketCliData1_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					LongCounterOffset=16; //(8+5) and align 4
				}else{
					LongCounterOffset=0;
				}

				if(((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_IDR)){
					if(g_SocketCliData1_DescSize[path_id] == 0){
						ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"h264 DescSize 0!!\r\n");
						return;
					}
					if(g_SocketCliData1_DescSize[path_id]%4){
						DescOffset=4-(g_SocketCliData1_DescSize[path_id]%4);
					}
					DescOffset +=g_SocketCliData1_DescSize[path_id];//add offset for sps + pps
					DescOffset+=TS_VIDEO_RESERVED_SIZE;
				}else{
					DescOffset=TS_VIDEO_RESERVED_SIZE;
				}
				//uiBsFrmBufAddr =ethsocket_cli_ipc_Data1_GetBsAlignAddr(path_id, g_SocketCliData1_BsSize[path_id]);
				//uiBsFrmBufAddr =ethsocket_cli_ipc_Data1_GetBsAlignAddr(path_id, (g_SocketCliData1_BsSize[path_id]+LongCounterOffset));
				uiBsFrmBufAddr =ethsocket_cli_ipc_Data1_GetBsAlignAddr(path_id, (DescOffset+g_SocketCliData1_BsSize[path_id]+LongCounterOffset));

				memcpy((unsigned char*)(uiBsFrmBufAddr+DescOffset), (unsigned char*)(pRecvAddr + g_SocketCliData1_BsStartPos[path_id]), g_SocketCliData1_BsSize[path_id]);
				//g_CliSocket_size_frm[g_BsFrmQueueIdx]=g_bs_size_total;
				memset((unsigned char*)uiBsFrmBufAddr, 0 ,DescOffset);
				if(g_SocketCliData1_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					unsigned int sz_tmp=g_SocketCliData1_BsSize[path_id]+DescOffset;
					uiBsFrmBufAddr[0+sz_tmp]=0;
					uiBsFrmBufAddr[1+sz_tmp]=0;
					uiBsFrmBufAddr[2+sz_tmp]=0;
					uiBsFrmBufAddr[3+sz_tmp]=1;
					uiBsFrmBufAddr[4+sz_tmp]=HEAD_TYPE_LONGCNT_STAMP;
					uiBsFrmBufAddr[5+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0xff000000)>>24;
					uiBsFrmBufAddr[6+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0x00ff0000)>>16;
					uiBsFrmBufAddr[7+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0x0000ff00)>>8;
					uiBsFrmBufAddr[8+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0x000000ff)>>0;
					uiBsFrmBufAddr[9+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0xff000000)>>24;
					uiBsFrmBufAddr[10+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0x00ff0000)>>16;
					uiBsFrmBufAddr[11+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0x0000ff00)>>8;
					uiBsFrmBufAddr[12+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0x000000ff)>>0;

					uiBsFrmBufAddr[13+sz_tmp]=0;
					uiBsFrmBufAddr[14+sz_tmp]=0;
					uiBsFrmBufAddr[15+sz_tmp]=0;
				}
				#else
				if(g_SocketCliData1_BsFrmBufAddr[path_id] == NULL){
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"h264 g_SocketCliData1_BsFrmBufAddr 0!!\r\n");
					return;
				}

				if(g_SocketCliData1_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					LongCounterOffset=16; //(8+5) and align 4
				}else{
					LongCounterOffset=0;
				}
				if(((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_IDR)){
					if(g_SocketCliData1_DescSize[path_id] == 0){
						if(errCnt % 300 == 0){
							ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"h264 DescSize 0!!\r\n");
						}
						errCnt++;
						return;
					}
					if(g_SocketCliData1_DescSize[path_id]%4){
						DescOffset=4-(g_SocketCliData1_DescSize[path_id]%4);
					}
					DescOffset +=g_SocketCliData1_DescSize[path_id];//add offset for sps + pps
					DescOffset+=TS_VIDEO_RESERVED_SIZE;
				}else{
					DescOffset=TS_VIDEO_RESERVED_SIZE;
				}

				uiBsFrmBufAddr=g_SocketCliData1_BsFrmBufAddr[path_id]-DescOffset;
				if(g_SocketCliData1_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					unsigned int sz_tmp=g_SocketCliData1_BsSize[path_id]+DescOffset;
					g_SocketCliData1_LongConterByteBK[path_id][0]=uiBsFrmBufAddr[0+sz_tmp]; //recive size over than g_SocketCliData1_BsSize, so backup data, the next frame data before set longcounter data
					g_SocketCliData1_LongConterByteBK[path_id][1]=uiBsFrmBufAddr[1+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][2]=uiBsFrmBufAddr[2+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][3]=uiBsFrmBufAddr[3+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][4]=uiBsFrmBufAddr[4+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][5]=uiBsFrmBufAddr[5+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][6]=uiBsFrmBufAddr[6+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][7]=uiBsFrmBufAddr[7+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][8]=uiBsFrmBufAddr[8+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][9]=uiBsFrmBufAddr[9+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][10]=uiBsFrmBufAddr[10+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][11]=uiBsFrmBufAddr[11+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][12]=uiBsFrmBufAddr[12+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][13]=uiBsFrmBufAddr[13+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][14]=uiBsFrmBufAddr[14+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][15]=uiBsFrmBufAddr[15+sz_tmp];
					uiBsFrmBufAddr[0+sz_tmp]=0;
					uiBsFrmBufAddr[1+sz_tmp]=0;
					uiBsFrmBufAddr[2+sz_tmp]=0;
					uiBsFrmBufAddr[3+sz_tmp]=1;
					uiBsFrmBufAddr[4+sz_tmp]=HEAD_TYPE_LONGCNT_STAMP;
					uiBsFrmBufAddr[5+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0xff000000)>>24;
					uiBsFrmBufAddr[6+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0x00ff0000)>>16;
					uiBsFrmBufAddr[7+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0x0000ff00)>>8;
					uiBsFrmBufAddr[8+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0x000000ff)>>0;
					uiBsFrmBufAddr[9+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0xff000000)>>24;
					uiBsFrmBufAddr[10+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0x00ff0000)>>16;
					uiBsFrmBufAddr[11+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0x0000ff00)>>8;
					uiBsFrmBufAddr[12+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0x000000ff)>>0;

					uiBsFrmBufAddr[13+sz_tmp]=0;
					uiBsFrmBufAddr[14+sz_tmp]=0;
					uiBsFrmBufAddr[15+sz_tmp]=0;
				}
				#endif
				bPushData=1;
				g_SocketCliData1_BsFrameCnt[path_id]++;

			}else if((((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)>> 0x1)&0x3F) == H265_TYPE_VPS )
				||(((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)>> 0x1)&0x3F) == H265_TYPE_SLICE)
				){
				#if 0
				if(g_SocketCliData1_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					LongCounterOffset=16; //(8+5) and align 4
				}else{
					LongCounterOffset=0;
				}
				if((((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)>> 0x1)&0x3F) == H265_TYPE_VPS )){
					if(g_SocketCliData1_DescSize[path_id] == 0){
						ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"h265 DescSize 0!!\r\n");
						return;
					}
					if(g_SocketCliData1_DescSize[path_id]%4){
						DescOffset=4-(g_SocketCliData1_DescSize[path_id]%4);
					}
					DescOffset+=TS_VIDEO_RESERVED_SIZE;
				}else{
					DescOffset=TS_VIDEO_RESERVED_SIZE;
				}

				//uiBsFrmBufAddr =ethsocket_cli_ipc_Data1_GetBsAlignAddr(path_id, g_SocketCliData1_BsSize[path_id]);
				//uiBsFrmBufAddr =ethsocket_cli_ipc_Data1_GetBsAlignAddr(path_id, (g_SocketCliData1_BsSize[path_id]+LongCounterOffset));
				uiBsFrmBufAddr =ethsocket_cli_ipc_Data1_GetBsAlignAddr(path_id, (DescOffset+g_SocketCliData1_BsSize[path_id]+LongCounterOffset));

				memcpy((unsigned char*)(uiBsFrmBufAddr+DescOffset), (unsigned char*)(pRecvAddr + g_SocketCliData1_BsStartPos[path_id]), g_SocketCliData1_BsSize[path_id]);
				//g_CliSocket_size_frm[g_BsFrmQueueIdx]=g_bs_size_total;
				memset((unsigned char*)uiBsFrmBufAddr, 0 ,DescOffset);
				if(g_SocketCliData1_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					unsigned int sz_tmp=g_SocketCliData1_BsSize[path_id]+DescOffset;
					uiBsFrmBufAddr[0+sz_tmp]=0;
					uiBsFrmBufAddr[1+sz_tmp]=0;
					uiBsFrmBufAddr[2+sz_tmp]=0;
					uiBsFrmBufAddr[3+sz_tmp]=1;
					uiBsFrmBufAddr[4+sz_tmp]=HEAD_TYPE_LONGCNT_STAMP;
					uiBsFrmBufAddr[5+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0xff000000)>>24;
					uiBsFrmBufAddr[6+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0x00ff0000)>>16;
					uiBsFrmBufAddr[7+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0x0000ff00)>>8;
					uiBsFrmBufAddr[8+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0x000000ff)>>0;
					uiBsFrmBufAddr[9+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0xff000000)>>24;
					uiBsFrmBufAddr[10+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0x00ff0000)>>16;
					uiBsFrmBufAddr[11+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0x0000ff00)>>8;
					uiBsFrmBufAddr[12+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0x000000ff)>>0;

					uiBsFrmBufAddr[13+sz_tmp]=0;
					uiBsFrmBufAddr[14+sz_tmp]=0;
					uiBsFrmBufAddr[15+sz_tmp]=0;
				}
				#else
				if(g_SocketCliData1_BsFrmBufAddr[path_id] == NULL){
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"h265 g_SocketCliData1_BsFrmBufAddr 0!!\r\n");
					return;
				}

				if(g_SocketCliData1_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					LongCounterOffset=16; //(8+5) and align 4
				}else{
					LongCounterOffset=0;
				}
				if((((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)>> 0x1)&0x3F) == H265_TYPE_VPS )){
					if(g_SocketCliData1_DescSize[path_id] == 0){
						if(errCnt % 300 == 0){
							ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"h265 DescSize 0!!\r\n");
						}
						errCnt++;
						return;
					}
					if(g_SocketCliData1_DescSize[path_id]%4){
						DescOffset=4-(g_SocketCliData1_DescSize[path_id]%4);
					}
					DescOffset+=TS_VIDEO_RESERVED_SIZE;
				}else{
					DescOffset=TS_VIDEO_RESERVED_SIZE;
				}

				uiBsFrmBufAddr=g_SocketCliData1_BsFrmBufAddr[path_id]-DescOffset;
				if(g_SocketCliData1_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					unsigned int sz_tmp=g_SocketCliData1_BsSize[path_id]+DescOffset;
					g_SocketCliData1_LongConterByteBK[path_id][0]=uiBsFrmBufAddr[0+sz_tmp]; //recive size over than g_SocketCliData1_BsSize, so backup data, the next frame data before set longcounter data
					g_SocketCliData1_LongConterByteBK[path_id][1]=uiBsFrmBufAddr[1+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][2]=uiBsFrmBufAddr[2+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][3]=uiBsFrmBufAddr[3+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][4]=uiBsFrmBufAddr[4+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][5]=uiBsFrmBufAddr[5+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][6]=uiBsFrmBufAddr[6+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][7]=uiBsFrmBufAddr[7+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][8]=uiBsFrmBufAddr[8+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][9]=uiBsFrmBufAddr[9+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][10]=uiBsFrmBufAddr[10+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][11]=uiBsFrmBufAddr[11+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][12]=uiBsFrmBufAddr[12+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][13]=uiBsFrmBufAddr[13+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][14]=uiBsFrmBufAddr[14+sz_tmp];
					g_SocketCliData1_LongConterByteBK[path_id][15]=uiBsFrmBufAddr[15+sz_tmp];
					uiBsFrmBufAddr[0+sz_tmp]=0;
					uiBsFrmBufAddr[1+sz_tmp]=0;
					uiBsFrmBufAddr[2+sz_tmp]=0;
					uiBsFrmBufAddr[3+sz_tmp]=1;
					uiBsFrmBufAddr[4+sz_tmp]=HEAD_TYPE_LONGCNT_STAMP;
					uiBsFrmBufAddr[5+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0xff000000)>>24;
					uiBsFrmBufAddr[6+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0x00ff0000)>>16;
					uiBsFrmBufAddr[7+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0x0000ff00)>>8;
					uiBsFrmBufAddr[8+sz_tmp]=(g_SocketCliData1_LongConterHigh[path_id]  & 0x000000ff)>>0;
					uiBsFrmBufAddr[9+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0xff000000)>>24;
					uiBsFrmBufAddr[10+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0x00ff0000)>>16;
					uiBsFrmBufAddr[11+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0x0000ff00)>>8;
					uiBsFrmBufAddr[12+sz_tmp]=(g_SocketCliData1_LongConterLow[path_id] & 0x000000ff)>>0;

					uiBsFrmBufAddr[13+sz_tmp]=0;
					uiBsFrmBufAddr[14+sz_tmp]=0;
					uiBsFrmBufAddr[15+sz_tmp]=0;
				}
				#endif
				bPushData=1;
				g_SocketCliData1_BsFrameCnt[path_id]++;
			}else if((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)==HEAD_TYPE_THUMB) ||
			(*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)==HEAD_TYPE_RAW_ENCODE)){
				if(g_SocketCliData_RawEncodeAddr[path_id][ETHSOCKIPCCLI_ID_0] !=0 && g_SocketCliData_RawEncodeSize[path_id][ETHSOCKIPCCLI_ID_0] !=0){
					//keep header 0 0 0 1 0xfe/0xff for uitron
					//memcpy((unsigned char*)g_SocketCliData1_RawEncodeAddr, (unsigned char*)(pRecvAddr + g_SocketCliData1_BsStartPos+5), (g_SocketCliData1_BsSize-5));
					memcpy((unsigned char*)g_SocketCliData_RawEncodeAddr[path_id][ETHSOCKIPCCLI_ID_0], (unsigned char*)(pRecvAddr + g_SocketCliData1_BsStartPos[path_id]), (g_SocketCliData1_BsSize[path_id]));
					//g_SocketCliData1_RawEncodeSize=g_SocketCliData1_BsSize-5;
					g_SocketCliData_RawEncodeSize[path_id][ETHSOCKIPCCLI_ID_0]=g_SocketCliData1_BsSize[path_id];
					if (*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4) == 0xFF) {
						bPushData = 2;
					} else if (*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4) == 0xFE) {
						bPushData = 3;
					}
				}else{
					if (ETHSOCKETCLI_INVALID_PARAMADDR == g_SocketCliData_RawEncodeAddr[path_id][ETHSOCKIPCCLI_ID_0]) {
						ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"g_SocketCliData_RawEncodeAddr = 0x%X\n", g_SocketCliData_RawEncodeAddr[path_id][ETHSOCKIPCCLI_ID_0]);
					}
					if (0 == g_SocketCliData_RawEncodeSize[path_id][ETHSOCKIPCCLI_ID_0]) {
						ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"g_SocketCliData_RawEncodeSize = 0x%X\n", g_SocketCliData_RawEncodeSize[path_id][ETHSOCKIPCCLI_ID_0]);
					}
				}
			}
			#if 1
			if((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_IDR){
				//diag_printf("eD1_I[%d]\r\n",path_id);
				//diag_printf("eD1_I\r\n");
				//g_queue_idx++;
				//g_SocketCliData1_BsFrameCnt++;
				//eth_is_i_frame = 1;
				//eth_i_cnt ++;
				//printf("DELAY\r\n");
				//HAL_DELAY_US(10000);

			}else if((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_SLICE){
				//DBG_DUMP("P OK\r\n");
				//diag_printf("eD1_P\r\n");
				//g_queue_idx++;
				//g_SocketCliData1_BsFrameCnt++;
				//eth_is_i_frame = 0;
			}else if((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_SPS){
				//DBG_DUMP("SPS OK\r\n");
			}else if((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_PPS){
				//DBG_DUMP("PPS OK\r\n");

			}else if(((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)>> 0x1)&0x3F) == H265_TYPE_VPS){
				//diag_printf("eD1_I[%d]\r\n",path_id);
				//diag_printf("eD1_I\r\n");
				//g_queue_idx++;
				//g_SocketCliData1_BsFrameCnt++;
				//eth_is_i_frame = 1;
				//eth_i_cnt ++;
				//printf("DELAY\r\n");
				//HAL_DELAY_US(10000);
			}else if(((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)>> 0x1)&0x3F) == H265_TYPE_SLICE){
				//diag_printf("eD1_P[%d]\r\n",path_id);
				//diag_printf("eD1_P\r\n");
				//g_queue_idx++;
				//g_SocketCliData1_BsFrameCnt++;
				//eth_is_i_frame = 0;
			}else if((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)==HEAD_TYPE_THUMB)){
				//DBG_DUMP("Thumb OK\r\n");
				//DBG_DUMP("data[5]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",*(g_psocket_cli + g_bs_size_start +5),*(g_psocket_cli + g_bs_size_start +6),*(g_psocket_cli + g_bs_size_start +g_bs_size_total-5),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-4),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-3),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-2),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-1),*(g_psocket_cli + g_bs_size_start + g_bs_size_total));
			}else if((*(pRecvAddr + g_SocketCliData1_BsStartPos[path_id] +4)==HEAD_TYPE_RAW_ENCODE)){
				//DBG_DUMP("PIM OK\r\n");
				//DBG_DUMP("data[5]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",*(g_psocket_cli + g_bs_size_start +5),*(g_psocket_cli + g_bs_size_start +6),*(g_psocket_cli + g_bs_size_start +g_bs_size_total-5),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-4),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-3),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-2),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-1),*(g_psocket_cli + g_bs_size_start + g_bs_size_total));
			}else{
				//DBG_DUMP("Check FAIL\r\n");
			}
			#endif

			if(bPushData==1 ){
				//push data to VdoDec
				ETHSOCKETCLI_RECV_PARAM *pParam = (ETHSOCKETCLI_RECV_PARAM *)ethsocket_cli_ipc_GetSndAddr(path_id, ETHSOCKIPCCLI_ID_0);
				unsigned int checkSumHead=0;
				unsigned int checkSumTail=0;
				unsigned int checksum = 0;
				unsigned int checksumLen = 64;

				//pParam->obj = 0;
				pParam->size = (int)(g_SocketCliData1_BsSize[path_id]+ LongCounterOffset+DescOffset);
				//pParam->buf=(char *)(uiBsFrmBufAddr);
				pParam->buf=(char *)(ipc_getNonCacheAddr((unsigned int)uiBsFrmBufAddr));

				//diag_printf("eCosD1Frm Size=%d, Addr=0x%x\r\n",pParam->size, pParam->buf);
				//if(((*(pRecvAddr + g_SocketCliData1_BsStartPos +4)& 0x1F) == H264_NALU_TYPE_IDR) ||
				//((*(pRecvAddr + g_SocketCliData1_BsStartPos +4)& 0x1F) == H264_NALU_TYPE_SLICE)){
				//}
				//diag_printf("I/P size=%d, 0x%x, [0]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",  pParam->size, pParam->buf,pParam->buf[0],pParam->buf[1],pParam->buf[2],pParam->buf[3],pParam->buf[4]);
				ipc_flushCache((NVTIPC_U32)uiBsFrmBufAddr, (NVTIPC_U32)(g_SocketCliData1_BsSize[path_id]+ LongCounterOffset+DescOffset));
				ipc_flushCache((NVTIPC_U32)pParam, (NVTIPC_U32)sizeof(ETHSOCKETCLI_RECV_PARAM));
				//make checksum
				if(g_SocketCliData1_BsSize[path_id]>checksumLen){
					checksum=0;
					for(i=0;i<checksumLen;i++){
						checksum+= *(uiBsFrmBufAddr+DescOffset+i);
					}
					checkSumHead=checksum & 0xff;//ethsocket_cli_ipc_Checksum((unsigned char*)uiBsFrmBufAddr, 64) & 0xff;
					checksum=0;
					for(i=(g_SocketCliData1_BsSize[path_id]-checksumLen) ;i<g_SocketCliData1_BsSize[path_id];i++){
						checksum+= *(uiBsFrmBufAddr+DescOffset+i);
					}
					checkSumTail=checksum & 0xff;//ethsocket_cli_ipc_Checksum((unsigned char*)(uiBsFrmBufAddr+ g_SocketCliData2_BsSize-1-64), 64) & 0xff;
				}
				if(checkSumHead== g_SocketCliData1_checkSumHead[path_id]
					&& checkSumTail== g_SocketCliData1_checkSumTail[path_id]){
					result = ethsocket_cli_ipc_exeapi(path_id, ETHSOCKIPCCLI_ID_0, ETHSOCKETCLI_CMDID_RCV, &Ret_EthsocketApi);
				}else{
					unsigned int sz_tmp=g_SocketCliData1_BsSize[path_id];
					//unsigned char* addr_tmp=uiBsFrmBufAddr;
					//unsigned int j=0;
					i=DescOffset;
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"chksum error, BsCnt=%d, chkD=%d, oldPos=%d, oldIdx=%d, oldBsSz=%d,  startPos=%d, idx=%d, BsSz=%d, tx chk=0x%x, 0x%x, now chk=0x%x, 0x%x, RecvSz=%d, DescOfset=%d, DescSize=%d\r\n",g_SocketCliData1_BsFrameCnt[path_id],bChkData[path_id], Prev_StartPos[path_id], Prev_IdxInGOP[path_id],Prev_BsSize[path_id],g_SocketCliData1_BsStartPos[path_id],g_SocketCliData1_IdxInGOP[path_id],g_SocketCliData1_BsSize[path_id],g_SocketCliData1_checkSumHead[path_id],g_SocketCliData1_checkSumTail[path_id],checkSumHead,checkSumTail, g_SocketCliData1_RecvSize[path_id],DescOffset,g_SocketCliData1_DescSize[path_id]);
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"Bs i+0=0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",uiBsFrmBufAddr[i+0] ,uiBsFrmBufAddr[i+1],uiBsFrmBufAddr[i+2],uiBsFrmBufAddr[i+3],uiBsFrmBufAddr[i+4],uiBsFrmBufAddr[i+5],uiBsFrmBufAddr[i+6],uiBsFrmBufAddr[i+7],uiBsFrmBufAddr[i+8],uiBsFrmBufAddr[i+9] ,uiBsFrmBufAddr[i+10],uiBsFrmBufAddr[i+11],uiBsFrmBufAddr[i+12],uiBsFrmBufAddr[i+13],uiBsFrmBufAddr[i+14],uiBsFrmBufAddr[i+15]);

					i=g_SocketCliData1_BsStartPos[path_id];
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"Recv i+0=0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",pRecvAddr[i+0] ,pRecvAddr[i+1],pRecvAddr[i+2],pRecvAddr[i+3],pRecvAddr[i+4],pRecvAddr[i+5],pRecvAddr[i+6],pRecvAddr[i+7],pRecvAddr[i+8],pRecvAddr[i+9] ,pRecvAddr[i+10],pRecvAddr[i+11],pRecvAddr[i+12],pRecvAddr[i+13],pRecvAddr[i+14],pRecvAddr[i+15]);
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"Recv i-16=0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",pRecvAddr[i-16] ,pRecvAddr[i-15] ,pRecvAddr[i-14] ,pRecvAddr[i-13] ,pRecvAddr[i-12] ,pRecvAddr[i-11] ,pRecvAddr[i-10] ,pRecvAddr[i-9] ,pRecvAddr[i-8] ,pRecvAddr[i-7],pRecvAddr[i-6],pRecvAddr[i-5],pRecvAddr[i-4],pRecvAddr[i-3],pRecvAddr[i-2],pRecvAddr[i-1]);
					i=g_SocketCliData1_BsSize[path_id];
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"totalsize+0=0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",pRecvAddr[i+0] ,pRecvAddr[i+1],pRecvAddr[i+2],pRecvAddr[i+3],pRecvAddr[i+4],pRecvAddr[i+5],pRecvAddr[i+6],pRecvAddr[i+7],pRecvAddr[i+8],pRecvAddr[i+9] ,pRecvAddr[i+10],pRecvAddr[i+11],pRecvAddr[i+12],pRecvAddr[i+13],pRecvAddr[i+14],pRecvAddr[i+15]);

					if(checkSumTail!= g_SocketCliData1_checkSumTail[path_id]){
						ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"End 0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",uiBsFrmBufAddr[sz_tmp-10],uiBsFrmBufAddr[sz_tmp-9],uiBsFrmBufAddr[sz_tmp-8],uiBsFrmBufAddr[sz_tmp-7],uiBsFrmBufAddr[sz_tmp-6],uiBsFrmBufAddr[sz_tmp-5],uiBsFrmBufAddr[sz_tmp-4],uiBsFrmBufAddr[sz_tmp-3],uiBsFrmBufAddr[sz_tmp-2],uiBsFrmBufAddr[sz_tmp-1]);
					}
					if(checkSumHead!= g_SocketCliData1_checkSumHead[path_id]){
						ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"Start 0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",uiBsFrmBufAddr[0],uiBsFrmBufAddr[1],uiBsFrmBufAddr[2],uiBsFrmBufAddr[3],uiBsFrmBufAddr[4],uiBsFrmBufAddr[5],uiBsFrmBufAddr[6],uiBsFrmBufAddr[7],uiBsFrmBufAddr[8],uiBsFrmBufAddr[9]);
					}
					ETHSOCKETCLI_PARAM_PARAM *pNotifyParam = (ETHSOCKETCLI_PARAM_PARAM *)ethsocket_cli_ipc_GetSndAddr(path_id, ETHSOCKIPCCLI_ID_0);
					pNotifyParam->param1 = (int)CYG_ETHSOCKETCLI_STATUS_CLIENT_SLOW;
					pNotifyParam->param2 = ERR_SLOW_CHECKSUM_ERR;//g_SocketCliData1_IdxInGOP[path_id];
					ipc_flushCache((NVTIPC_U32)pNotifyParam, (NVTIPC_U32)sizeof(ETHSOCKETCLI_PARAM_PARAM));
					result = ethsocket_cli_ipc_exeapi(path_id, ETHSOCKIPCCLI_ID_0, ETHSOCKETCLI_CMDID_NOTIFY, &Ret_EthsocketApi);
				}
				Prev_BsSize[path_id]=g_SocketCliData1_BsSize[path_id];
				Prev_IdxInGOP[path_id]=g_SocketCliData1_IdxInGOP[path_id];
				Prev_StartPos[path_id]=g_SocketCliData1_BsStartPos[path_id];

				if ((result != ETHSOCKETCLI_RET_OK) || (Ret_EthsocketApi!=0)) {
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"snd CB fail\r\n");
				}

			#if 0
				ISF_PORT         *pDist;
				ISF_VIDEO_STREAM_BUF    *pVidStrmBuf;
				g_SocketCliData1_InIsfData = gISF_EthCam_Pull_InData1;

				pVidStrmBuf = (ISF_VIDEO_STREAM_BUF *)&g_SocketCliData1_InIsfData.Desc;
				pVidStrmBuf->flag     = MAKEFOURCC('V', 'S', 'T', 'M');
				//pVidStrmBuf->DataAddr = (UINT32)pCliSocket_H264_data[g_queue_idx-1];//pVdoBs->BsAddr;
				pVidStrmBuf->DataAddr = (UINT32)uiBsFrmBufAddr;//g_EthBsFrmAddr[g_queue_idx-1];//pVdoBs->BsAddr;
				pVidStrmBuf->DataSize = g_SocketCliData1_BsBufMapTbl[g_SocketCliData1_BsQueueIdx];//g_CliSocket_size_frm[g_queue_idx-1];//pVdoBs->BsSize;
				//pVidStrmBuf->CodecType = MEDIAVIDENC_H264;                                          //codec type
				pVidStrmBuf->CodecType = sEthCamTxDecInfo.Codec;                                          //codec type
				//pVidStrmBuf->Resv[0]  = (UINT32)&(SPS_Addr[0]);                                     //sps addr
				pVidStrmBuf->Resv[0]  = (UINT32)&(sEthCamTxDecInfo.Desc[0]);         //sps addr
				//pVidStrmBuf->Resv[1]  = 28;                                                         //sps size
				pVidStrmBuf->Resv[1]  = sEthCamTxDecInfo.SPSSize;                         //sps size
				//pVidStrmBuf->Resv[2]  = (UINT32)&(SPS_Addr[28]);                                    //pps addr
				pVidStrmBuf->Resv[2]  = (UINT32)&(sEthCamTxDecInfo.Desc[sEthCamTxDecInfo.SPSSize]);                                    //pps addr
				//pVidStrmBuf->Resv[3]  = 8;
				pVidStrmBuf->Resv[3]  = sEthCamTxDecInfo.PPSSize;                                                          //pps size
				pVidStrmBuf->Resv[4]  = 0;                                                          //vps addr
				pVidStrmBuf->Resv[5]  = 0;                                                          //vps size
				pVidStrmBuf->Resv[6]  = (eth_is_i_frame) ? 3 : 0;                                   //FrameType (IDR = 3, P = 0)
				pVidStrmBuf->Resv[7]  = ((eth_i_cnt % 4) == 0 && eth_is_i_frame) ? TRUE : FALSE;    //IsIDR2Cut
				pVidStrmBuf->Resv[8]  = 0;                                                          //SVC size
				pVidStrmBuf->Resv[9]  = 0;                                                          //Temporal Id
				pVidStrmBuf->Resv[10] = (eth_is_i_frame) ? TRUE : FALSE;                            //IsKey
				pVidStrmBuf->Resv[12] = g_SocketCliData1_BsFrameCnt-1;//pVdoBs->FrmIdx;
				pVidStrmBuf->Resv[13] = 0;//pVdoBs->bIsEOF;  //have next I Frame ?
				g_SocketCliData1_InIsfData.TimeStamp =0;
				pDist = ImageUnit_In(&ISF_Demux, ImageApp_MovieMulti_GetEthCamDemuxInPort(_CFG_ETHCAM_ID_1));
				if (ImageUnit_IsAllowPush(pDist)) {
					//DBG_DUMP("Push\r\n");
					ImageUnit_PushData(pDist, &g_SocketCliData1_InIsfData, 0);
				}
			#endif
			} else if (bPushData == 2 || bPushData == 3) {
				ETHSOCKETCLI_RECV_PARAM *pParam = (ETHSOCKETCLI_RECV_PARAM *)ethsocket_cli_ipc_GetSndAddr(path_id, ETHSOCKIPCCLI_ID_0);
				//pParam->obj = 0;
				pParam->size = (int)(g_SocketCliData_RawEncodeSize[path_id][ETHSOCKIPCCLI_ID_0]);
				//pParam->buf=(char *)(g_SocketCliData_RawEncodeAddr[path_id][ETHSOCKIPCCLI_ID_0]);
				pParam->buf=(char *)(ipc_getNonCacheAddr(g_SocketCliData_RawEncodeAddr[path_id][ETHSOCKIPCCLI_ID_0]));
				//unsigned char* pBuf=(unsigned char*)g_SocketCliData_RawEncodeAddr[ETHSOCKIPCCLI_ID_0];
				//diag_printf("eCosRawEn Size=%d, Addr=%d\r\n",pParam->size, pParam->buf);

				//diag_printf("eCosPr BsCnt=%d, chkD=%d, oldPos=%d, oldIdx=%d, oldBsSz=%d,  startPos=%d, idx=%d, BsSz=%d, RecvSz=%d\r\n",g_SocketCliData1_BsFrameCnt[path_id],bChkData, Prev_StartPos[path_id], Prev_IdxInGOP[path_id],Prev_BsSize[path_id],g_SocketCliData1_BsStartPos[path_id],g_SocketCliData1_IdxInGOP[path_id],g_SocketCliData1_BsSize[path_id], g_SocketCliData1_RecvSize[path_id]);
				//diag_printf("eCosPr i+0=0x%x,%x,%x,%x,%x,%x,%x,%x,%x,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",pRecvAddr[0] ,pRecvAddr[1],pRecvAddr[2],pRecvAddr[3],pRecvAddr[4],pRecvAddr[5],pRecvAddr[6],pRecvAddr[7],pRecvAddr[8],pRecvAddr[9],pRecvAddr[10],pRecvAddr[11],pRecvAddr[12],pRecvAddr[13],pRecvAddr[14],pRecvAddr[15],pRecvAddr[16],pRecvAddr[17],pRecvAddr[18],pRecvAddr[19],pRecvAddr[20],pRecvAddr[21],pRecvAddr[22],pRecvAddr[23],pRecvAddr[24]);

				//diag_printf("data[3]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",pBuf[3],pBuf[4],pBuf[5],pBuf[6],pBuf[pParam->size-3],pBuf[pParam->size-2],pBuf[pParam->size-1]);
				ipc_flushCache((NVTIPC_U32)g_SocketCliData_RawEncodeAddr[path_id][ETHSOCKIPCCLI_ID_0], (NVTIPC_U32)g_SocketCliData_RawEncodeSize[path_id][ETHSOCKIPCCLI_ID_0]);
				ipc_flushCache((NVTIPC_U32)pParam, (NVTIPC_U32)sizeof(ETHSOCKETCLI_RECV_PARAM));

				result = ethsocket_cli_ipc_exeapi(path_id, ETHSOCKIPCCLI_ID_0, ETHSOCKETCLI_CMDID_RCV, &Ret_EthsocketApi);

				if ((result != ETHSOCKETCLI_RET_OK) || (Ret_EthsocketApi!=0)) {
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"snd CB fail\r\n");
				}

			#if 0
				ISF_PORT *pDist;
				ISF_VIDEO_STREAM_BUF *pVidStrmBuf;
				g_SocketCliData1_InIsfData = gISF_EthCam_Pull_InData1;

				pVidStrmBuf = (ISF_VIDEO_STREAM_BUF *)&g_SocketCliData1_InIsfData.Desc;
				if (bPushData == 2) {
					pVidStrmBuf->flag = MAKEFOURCC('T', 'H', 'U', 'M');
				} else {
					pVidStrmBuf->flag = MAKEFOURCC('J', 'S', 'T', 'M');
				}
				pVidStrmBuf->DataAddr = g_SocketCliData1_RawEncodeAddr;
				pVidStrmBuf->DataSize = g_SocketCliData1_RawEncodeSize;
				pVidStrmBuf->Resv[0]  = 4;                                     //user_id
				g_SocketCliData1_InIsfData.TimeStamp = 0;
				pDist = ImageUnit_In(&ISF_Demux, ImageApp_MovieMulti_GetEthCamDemuxInPort(_CFG_ETHCAM_ID_1));
				if (ImageUnit_IsAllowPush(pDist)) {
					//DBG_DUMP("Push thumb\r\n");
					ImageUnit_PushData(pDist, &g_SocketCliData1_InIsfData, 0);
				}
			#endif
			}
			#if 1
			if(1){//bPushData == 1){// || bPushData == 2 || bPushData == 3){
				memset(&g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][0], 0, ETHCAM_FRM_HEADER_SIZE);
				for(i=ETHCAM_FRM_HEADER_SIZE_PART1;i<ETHCAM_FRM_HEADER_SIZE && uiBsFrmBufAddr;i++){
					g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][i]=uiBsFrmBufAddr[i-ETHCAM_FRM_HEADER_SIZE_PART1+DescOffset];//pParam->buf[i-ETHCAM_FRM_HEADER_SIZE_PART1+DescOffset];
				}

				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][0]=((g_SocketCliData1_LongConterHigh[path_id] ) & 0xff000000)>>24;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][1]=((g_SocketCliData1_LongConterHigh[path_id] ) & 0x00ff0000)>>16;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][2]=((g_SocketCliData1_LongConterHigh[path_id] ) & 0x0000ff00)>>8;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][3]=(g_SocketCliData1_LongConterHigh[path_id] ) & 0x000000ff;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][4]=(g_SocketCliData1_LongConterLow[path_id] & 0xff000000)>>24;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][5]=(g_SocketCliData1_LongConterLow[path_id]  & 0x00ff0000)>>16;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][6]=(g_SocketCliData1_LongConterLow[path_id]  & 0x0000ff00)>>8;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][7]=g_SocketCliData1_LongConterLow[path_id]  & 0x000000ff;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][8]=g_SocketCliData1_BsSize[path_id] & 0xff;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][9]=(g_SocketCliData1_BsSize[path_id] & 0xff00) >> 8;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][10]=(g_SocketCliData1_BsSize[path_id] & 0xff0000) >> 16;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][11]=g_SocketCliData1_IdxInGOP[path_id]  & 0xff;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][12]=g_SocketCliData1_checkSumHead[path_id]  & 0xff;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][13]=g_SocketCliData1_checkSumTail[path_id]  & 0xff;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][14]=g_SocketCliData1_DescSize[path_id]  & 0xff;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][15]=g_SocketCliData1_LongConterStampEn[path_id]  & 0xff;


				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][16]=(uiBsFrmBufAddr != NULL) ? ((((unsigned int )uiBsFrmBufAddr) & 0xff000000)>>24): 0;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][17]=(uiBsFrmBufAddr != NULL) ? ((((unsigned int )uiBsFrmBufAddr ) & 0x00ff0000)>>16): 0;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][18]=(uiBsFrmBufAddr != NULL) ? ((((unsigned int )uiBsFrmBufAddr ) & 0x0000ff00)>>8): 0;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][19]=(uiBsFrmBufAddr != NULL) ? (((unsigned int )uiBsFrmBufAddr ) & 0x000000ff): 0;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][20]=LongCounterOffset & 0xff;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][21]=DescOffset & 0xff;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][22]=bChkData[path_id]& 0xff;
				g_Data1_Tx_PrevHeader[path_id][g_Data1_Tx_PrevHeaderBufIdx][23]=bPushData & 0xff;


				g_Data1_Tx_PrevHeaderBufIdx++;
				g_Data1_Tx_PrevHeaderBufIdx= (g_Data1_Tx_PrevHeaderBufIdx >=ETHCAM_PREVHEADER_MAX_FRMNUM) ? 0 : g_Data1_Tx_PrevHeaderBufIdx;
			}
			#endif

			//diag_printf("2size_total=%d, size_get=%d, start=%d\r\n",g_SocketCliData1_BsSize, g_SocketCliData1_RecvSize,g_SocketCliData1_BsStartPos);
			//reset
			g_SocketCliData1_RecvSize[path_id] = g_SocketCliData1_RecvSize[path_id] -(g_SocketCliData1_BsSize[path_id] + g_SocketCliData1_BsStartPos[path_id]);
			//diag_printf("3size_total=%d, size_get=%d, start=%d\r\n",g_SocketCliData1_BsSize, g_SocketCliData1_RecvSize,g_SocketCliData1_BsStartPos);
			//diag_printf("3size_get=%d\r\n", g_SocketCliData1_RecvSize);
			#if 0
			if(g_SocketCliData1_RecvSize[path_id] != 0){
				//jump to next data//remove sended frame data
				memmove((unsigned char *)pRecvAddr, (unsigned char *)(pRecvAddr + g_SocketCliData1_BsSize[path_id] + g_SocketCliData1_BsStartPos[path_id]), g_SocketCliData1_RecvSize[path_id]);
				//diag_printf("move 0x%x, 0x%x, %d\r\n",pRecvAddr, pRecvAddr + g_SocketCliData1_BsSize + g_SocketCliData1_BsStartPos, g_SocketCliData1_RecvSize);
			}
			#else
			if(g_SocketCliData1_RecvSize[path_id] != 0){
				//jump to next data//remove sended frame data
				if(g_SocketCliData1_BsFrmBufAddr[path_id]==NULL){  //for h264, sps/pps
					memmove((unsigned char *)pRecvAddr, (unsigned char *)(pRecvAddr + g_SocketCliData1_BsSize[path_id] + g_SocketCliData1_BsStartPos[path_id]), g_SocketCliData1_RecvSize[path_id]);
					//printf("move 0x%x, 0x%x, %d\r\n",pRecvAddr, pRecvAddr + g_SocketCliData1_BsSize + g_SocketCliData1_BsStartPos, g_SocketCliData1_RecvSize);
				}else
				{
					memcpy((unsigned char*)(g_SocketCliData1_RecvAddr[path_id]),(unsigned char *)(pRecvAddr + g_SocketCliData1_BsSize[path_id] + g_SocketCliData1_BsStartPos[path_id]), g_SocketCliData1_RecvSize[path_id]);
					pRecvAddr=(unsigned char*)(g_SocketCliData1_RecvAddr[path_id]);
					pRecvAddr[0]=g_SocketCliData1_LongConterByteBK[path_id][0];
					pRecvAddr[1]=g_SocketCliData1_LongConterByteBK[path_id][1];
					pRecvAddr[2]=g_SocketCliData1_LongConterByteBK[path_id][2];
					pRecvAddr[3]=g_SocketCliData1_LongConterByteBK[path_id][3];
					pRecvAddr[4]=g_SocketCliData1_LongConterByteBK[path_id][4];
					pRecvAddr[5]=g_SocketCliData1_LongConterByteBK[path_id][5];
					pRecvAddr[6]=g_SocketCliData1_LongConterByteBK[path_id][6];
					pRecvAddr[7]=g_SocketCliData1_LongConterByteBK[path_id][7];
					pRecvAddr[8]=g_SocketCliData1_LongConterByteBK[path_id][8];
					pRecvAddr[9]=g_SocketCliData1_LongConterByteBK[path_id][9];
					pRecvAddr[10]=g_SocketCliData1_LongConterByteBK[path_id][10];
					pRecvAddr[11]=g_SocketCliData1_LongConterByteBK[path_id][11];
					pRecvAddr[12]=g_SocketCliData1_LongConterByteBK[path_id][12];
					pRecvAddr[13]=g_SocketCliData1_LongConterByteBK[path_id][13];
					pRecvAddr[14]=g_SocketCliData1_LongConterByteBK[path_id][14];
					pRecvAddr[15]=g_SocketCliData1_LongConterByteBK[path_id][15];
				}
				//diag_printf("t2: %d us\r\n", drv_getSysTick() - uiTime2);
			}
			#endif
			g_SocketCliData1_BsSize[path_id]=0;
			g_SocketCliData1_BsStartPos[path_id] =0;

			g_SocketCliData1_LongConterLow[path_id]=0;
			g_SocketCliData1_LongConterHigh[path_id]=0;
			g_SocketCliData1_LongConterStampEn[path_id]=0;
			if(bPushData==1){
				g_SocketCliData1_DescSize[path_id]=0;
			}
			uiBsFrmBufAddr =NULL;
			g_SocketCliData1_BsFrmBufAddr[path_id]=NULL;

			bChkData[path_id]=0;
		}else{
			//DBG_DUMP("else size_total=%d, size_get=%d\r\n",g_bs_size_total, g_CliSocket_size_get);
			break;
		}
	}
	ethsocket_cli_ipc_Data1_unlock();

}


static unsigned char* ethsocket_cli_ipc_Data2_GetBsAlignAddr(unsigned int path_id ,unsigned int uiWantSize)
{
	unsigned char *pOutAddr[ETHCAM_PATH_ID_MAX] = {NULL};
	static unsigned int uiSize[ETHCAM_PATH_ID_MAX] = {0};

	if(g_SocketCliData2_BsQueueIdx[path_id] ==0 && g_SocketCliData2_BsBufMapTbl[path_id][0]==0){
	}else{
		g_SocketCliData2_BsQueueIdx[path_id]++;
	}
	if(g_SocketCliData2_BsQueueIdx[path_id]>=1){
		uiSize[path_id]= uiSize[path_id] + g_SocketCliData2_BsBufMapTbl[path_id][g_SocketCliData2_BsQueueIdx[path_id]-1];
	}else{
		uiSize[path_id]=0;
	}
	while (uiSize[path_id] % 4 != 0)
	{
		uiSize[path_id] += 1;
	}
	if(g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_1] < (uiWantSize + uiSize[path_id])){
		uiSize[path_id]=0;
		g_SocketCliData2_BsQueueIdx[path_id]=0;
	}
	if(g_SocketCliData2_BsQueueIdx[path_id] >= g_SocketCliData_BsQueueMax[path_id][ETHSOCKIPCCLI_ID_1]){
		uiSize[path_id]=0;
		g_SocketCliData2_BsQueueIdx[path_id]=0;
	}
	pOutAddr[path_id] = (unsigned char* )(g_SocketCliData2_BsAddr[path_id]+ uiSize[path_id]);
	if (pOutAddr[path_id]) {
		g_SocketCliData2_BsBufMapTbl[path_id][g_SocketCliData2_BsQueueIdx[path_id]] = uiWantSize;
	}
	//DBG_DUMP("inSz=%d, Size=%d, pAddr=0x%x, endAddr=0x%x, Idx=%d\r\n",uiWantSize,uiSize,pOutAddr,pOutAddr+uiWantSize,g_BsFrmQueueIdx);
	return pOutAddr[path_id];
}
void ethsocket_cli_ipc_Data2_RecvResetParam(ETHCAM_PATH_ID path_id)
{
	g_SocketCliData2_RecvSize[path_id]=0;
	g_SocketCliData2_BsSize[path_id]=0;
	g_SocketCliData2_BsStartPos[path_id]=0;
	g_SocketCliData2_BsQueueIdx[path_id]=0;
	g_SocketCliData2_LongConterStampEn[path_id]=0;
	g_SocketCliData2_DescSize[path_id]=0;
	g_SocketCliData2_BsFrmBufAddr[path_id]=NULL;
	unsigned int i ;
	for(i=0;i<LONGCNT_STAMP_LENGTH;i++){
		g_SocketCliData2_LongConterByteBK[path_id][i]=0;
	}
	g_SocketCliData_DebugRecvSize[path_id][1]=0;
}
void ethsocket_cli_ipc_Data2_ParseBsFrame(unsigned int path_id, char* addr, int size)
{
	unsigned int i ;
	unsigned char* pRecvAddr=NULL;
	unsigned int bPushData=0;
	int result =0;
	int Ret_EthsocketApi = 0;
	static int errCnt=0;
	unsigned char *uiBsFrmBufAddr=NULL;
	unsigned int uiFrmBuf=0;
	static unsigned int bChkData[ETHCAM_PATH_ID_MAX]={0};

	//diag_printf("eeD2_sz[%d]=%d\r\n",path_id,size);
	//return;

	if(g_SocketCliData2_RecvAddr[path_id]==0){
		ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"RecvAddr no buffer!\r\n");
		return;
	}
	#if 0 //for new method
	if((g_SocketCliData2_RecvSize[path_id]+ size)>=gSndParamSize[path_id][ETHSOCKIPCCLI_ID_1]){
		if(errCnt % 300 == 0){
			ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"RecvSize over flow wanted[%d]=%d, but now=%d\r\n",path_id,gSndParamSize[path_id][ETHSOCKIPCCLI_ID_1],g_SocketCliData2_RecvSize[path_id]);
		}
		errCnt++;
		g_SocketCliData2_RecvSize[path_id]=0;
		return;
	}
	#endif
	//memcpy((char*)(g_SocketCliData2_RecvAddr[path_id]+g_SocketCliData2_RecvSize[path_id]), (char*)addr, size);
	g_SocketCliData2_RecvSize[path_id]+=size;
	//pRecvAddr = (unsigned char*)g_SocketCliData2_RecvAddr[path_id];
	if(g_SocketCliData2_BsFrmBufAddr[path_id]==NULL){
		pRecvAddr = (unsigned char*)g_SocketCliData2_RecvAddr[path_id];
	}else{
		pRecvAddr = g_SocketCliData2_BsFrmBufAddr[path_id];
	}
	if(g_SocketCliData_DebugReceiveEn[path_id][ETHSOCKIPCCLI_ID_1]){
		unsigned char*  addr_tmp= (unsigned char*)pRecvAddr;
		printf("11[%d]DbgRecv i+0=%d,bssz=%d, recsz=%d ,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",path_id,g_SocketCliData2_IdxInGOP[path_id],g_SocketCliData2_BsSize[path_id] , g_SocketCliData2_RecvSize[path_id],addr_tmp[0] ,addr_tmp[1],addr_tmp[2],addr_tmp[3],addr_tmp[4],addr_tmp[5],addr_tmp[6],addr_tmp[7],addr_tmp[8],addr_tmp[9],addr_tmp[10],addr_tmp[11],addr_tmp[12],addr_tmp[13],addr_tmp[14],addr_tmp[15],addr_tmp[16],addr_tmp[17],addr_tmp[18],addr_tmp[19],addr_tmp[20],addr_tmp[21],addr_tmp[22],addr_tmp[23],addr_tmp[24]);
		printf("bChkData=0x%x, size=%d\r\n",bChkData[path_id],size);
	}

	while(1)
	{
		//DBG_DUMP("psocket_data=0x%x, %x, %x, %x, %x, %x, %x, %x\r\n",  g_psocket_cli[0] ,  g_psocket_cli[0+1] ,  g_psocket_cli[0+2] ,  g_psocket_cli[0+3] ,  g_psocket_cli[0+4] ,  g_psocket_cli[0+5] ,  g_psocket_cli[0+6] ,  g_psocket_cli[0+7]);
		for(i=0;(i<g_SocketCliData2_RecvSize[path_id]) && (g_SocketCliData2_BsSize[path_id] ==0);i++)
		{
			if(g_SocketCliData2_BsSize[path_id] ==0 && ((i+6) < g_SocketCliData2_RecvSize[path_id]) && (pRecvAddr[i+0] ==0 && pRecvAddr[i+1] ==0 && pRecvAddr[i+2] ==0 && pRecvAddr[i+3] ==1) &&
				(
				((pRecvAddr[i+4]& 0x1F) == H264_TYPE_IDR && pRecvAddr[i+5] == H264_START_CODE_I)
				||((pRecvAddr[i+4]& 0x1F) == H264_TYPE_SLICE  && pRecvAddr[i+5] == H264_START_CODE_P)
				//||(pRecvAddr[i+4]& 0x1F) == H264_TYPE_SPS
				//||(((pRecvAddr[i+4]& 0x1F) == H264_TYPE_PPS) && ((pRecvAddr[i-2]+ (pRecvAddr[i-1]<<8))>0))
				||(i>3 && (g_SocketCliData2_RecvSize[path_id] > (5 +2)) && ((pRecvAddr[i+4]& 0x1F) == H264_TYPE_PPS) && ((pRecvAddr[i-2]+ (pRecvAddr[i-1]<<8))>0) )

				//||((pRecvAddr[i+4]>> 0x1) == H265_TYPE_IDR )
				||( (((pRecvAddr[i+4]>> 0x1)&0x3F) == H265_TYPE_SLICE)  )
				//||(pRecvAddr[i+4]>> 0x1) == H265_TYPE_SPS
				//||(pRecvAddr[i+4]>> 0x1) == H265_TYPE_PPS
				||( (((pRecvAddr[i+4]>> 0x1)&0x3F) == H265_TYPE_VPS) && ((((pRecvAddr[i+4] & 0x01)<<6) | (pRecvAddr[i+5] >>3) ) ==0) && ((pRecvAddr[i+4] & 0x80) ==0) )//for IDR, and Layer id =0, forbidden bit=0

				//||(pRecvAddr[i+4] == HEAD_TYPE_THUMB && pRecvAddr[i+5] == 0xFF && pRecvAddr[i+6] == 0xD8)
				//||(pRecvAddr[i+4] == HEAD_TYPE_RAW_ENCODE && pRecvAddr[i+5] == 0xFF && pRecvAddr[i+6] == 0xD8)
				)){
				//printf("i=%d, pRecvAddr=0x%x, %d\r\n", i, pRecvAddr[i-6], (pRecvAddr[i-5] + (((pRecvAddr[i-6] & 0xf0)>>4)<<8)));

				//if( (pRecvAddr[i-1]==0 || pRecvAddr[i-1]==LONGCNT_STAMP_EN)  && (pRecvAddr[i-5]>0 && pRecvAddr[i-5]<=180) && (pRecvAddr[i-2]==0) && ((unsigned int)(pRecvAddr[i-8]+ (pRecvAddr[i-7]<<8)+ (pRecvAddr[i-6]<<16)) <gSndParamSize[path_id][ETHSOCKIPCCLI_ID_1]) &&
				//if( (pRecvAddr[i-1]==0 || pRecvAddr[i-1]==LONGCNT_STAMP_EN)  && ((pRecvAddr[i-5] + (((pRecvAddr[i-6] & 0xf0)>>4)<<8)) >1) && (pRecvAddr[i-2]==0) && ((unsigned int)(pRecvAddr[i-8]+ (pRecvAddr[i-7]<<8)+ (pRecvAddr[i-6]<<16)) <gSndParamSize[path_id][ETHSOCKIPCCLI_ID_1]) &&
				if((i>=8) && (pRecvAddr[i-1]==0 || pRecvAddr[i-1]==LONGCNT_STAMP_EN)  && ((unsigned int)(pRecvAddr[i-8]+ (pRecvAddr[i-7]<<8)+ ((pRecvAddr[i-6] & 0x0f)<<16)) <g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_1]) &&
					(  ((pRecvAddr[i+4]& 0x1F) == H264_TYPE_IDR   && (pRecvAddr[i-5] == 1) && (pRecvAddr[i-2]==0)  )
					|| (((pRecvAddr[i+4]& 0x1F) == H264_TYPE_SLICE)  && ((pRecvAddr[i-5] + (((pRecvAddr[i-6] & 0xf0)>>4)<<8)) >1) && (pRecvAddr[i-2]==0) )) && (g_SocketCliData2_RecvSize[path_id] >=(5+ 8)) ){
					//g_SocketCliData2_BsSize[path_id]=pRecvAddr[i-8]+ (pRecvAddr[i-7]<<8)+ (pRecvAddr[i-6]<<16);
					g_SocketCliData2_BsSize[path_id]=pRecvAddr[i-8]+ (pRecvAddr[i-7]<<8)+ ((pRecvAddr[i-6] & 0x0f)<<16);
					g_SocketCliData2_checkSumHead[path_id]=pRecvAddr[i-4];
					g_SocketCliData2_checkSumTail[path_id]=pRecvAddr[i-3];
					g_SocketCliData2_LongConterStampEn[path_id]=pRecvAddr[i-1];
					if(g_SocketCliData2_LongConterStampEn[path_id]==LONGCNT_STAMP_EN && (i>=16)){
						g_SocketCliData2_LongConterLow[path_id]=(pRecvAddr[i-12]<<24)+ (pRecvAddr[i-11]<<16)+ (pRecvAddr[i-10]<<8) + (pRecvAddr[i-9]);
						g_SocketCliData2_LongConterHigh[path_id]=(pRecvAddr[i-16]<<24)+ (pRecvAddr[i-15]<<16)+ (pRecvAddr[i-14]<<8) + (pRecvAddr[i-13]);
					}else{
						g_SocketCliData2_LongConterLow[path_id]=0;
						g_SocketCliData2_LongConterHigh[path_id]=0;
					}
					bChkData[path_id]=1;
					//printf("h264\r\n");

				//}else if( (pRecvAddr[i-1]==0 || pRecvAddr[i-1]==LONGCNT_STAMP_EN) && ((unsigned int)(pRecvAddr[i-8]+ (pRecvAddr[i-7]<<8)+ (pRecvAddr[i-6]<<16)) <gSndParamSize[path_id][ETHSOCKIPCCLI_ID_1]) &&
				}else if((i>=8) && (pRecvAddr[i-1]==0 || pRecvAddr[i-1]==LONGCNT_STAMP_EN) && ((unsigned int)(pRecvAddr[i-8]+ (pRecvAddr[i-7]<<8)+ ((pRecvAddr[i-6] & 0x0f)<<16)) <g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_1]) &&
					(( (((pRecvAddr[i+4]>> 0x1)&0x3F) == H265_TYPE_VPS) && (pRecvAddr[i-2]>0) && (pRecvAddr[i-5]==1))//H265_TYPE_IDR
					//||( (((pRecvAddr[i+4]>> 0x1)&0x3F) == H265_TYPE_SLICE) && (pRecvAddr[i-2]==0) &&  (pRecvAddr[i-5]>1 && pRecvAddr[i-5]<=180)))  && (g_SocketCliData2_RecvSize[path_id] >= (5 +8)) && (i>=8)){  //5: 0 0 0 1 xx, 8: header frame siz(3) +fr idx(1) + chksum(2) + nouse(1) + longc_en(1)
					||( (((pRecvAddr[i+4]>> 0x1)&0x3F) == H265_TYPE_SLICE) && (pRecvAddr[i-2]==0) &&  ((pRecvAddr[i-5] + (((pRecvAddr[i-6] & 0xf0)>>4)<<8)) >1)   ))  && (g_SocketCliData2_RecvSize[path_id] >= (5 +8)) ){  //5: 0 0 0 1 xx, 8: header frame siz(3) +fr idx(1) + chksum(2) + nouse(1) + longc_en(1)

					g_SocketCliData2_BsSize[path_id]=pRecvAddr[i-8]+ (pRecvAddr[i-7]<<8)+ ((pRecvAddr[i-6] & 0x0f)<<16);

					g_SocketCliData2_checkSumHead[path_id]=pRecvAddr[i-4];
					g_SocketCliData2_checkSumTail[path_id]=pRecvAddr[i-3];
					g_SocketCliData2_LongConterStampEn[path_id]=pRecvAddr[i-1];
					g_SocketCliData2_DescSize[path_id]=pRecvAddr[i-2];
					if(g_SocketCliData2_LongConterStampEn[path_id]==LONGCNT_STAMP_EN && (i>=16)){
						g_SocketCliData2_LongConterLow[path_id]=(pRecvAddr[i-12]<<24)+ (pRecvAddr[i-11]<<16)+ (pRecvAddr[i-10]<<8) + (pRecvAddr[i-9]);
						g_SocketCliData2_LongConterHigh[path_id]=(pRecvAddr[i-16]<<24)+ (pRecvAddr[i-15]<<16)+ (pRecvAddr[i-14]<<8) + (pRecvAddr[i-13]);
					}else{
						g_SocketCliData2_LongConterLow[path_id]=0;
						g_SocketCliData2_LongConterHigh[path_id]=0;
					}
					bChkData[path_id]=2;
					//printf("h265, sz=%d\r\n",g_SocketCliData2_BsSize[path_id]);

				//}else if((pRecvAddr[i+4] == HEAD_TYPE_THUMB || pRecvAddr[i+4] == HEAD_TYPE_RAW_ENCODE) && (g_SocketCliData2_RecvSize[path_id] >= (5 + 4)) && (i>=4)){
				//	g_SocketCliData2_BsSize[path_id]=pRecvAddr[i-4]+ (pRecvAddr[i-3]<<8)+ (pRecvAddr[i-2]<<16);
				//	bChkData[path_id]=3;
				}else if ((i>=2) && ((pRecvAddr[i+4]& 0x1F) == H264_TYPE_PPS)  && (g_SocketCliData2_RecvSize[path_id] >= (5 +2)) ){//sps, pps // Recv Sz= 5 +2 -> header  5 + size 2 byte
					//diag_printf("PPS i=%d, R[i+4]=0x%x\r\n",i,pRecvAddr[i+4]);
					bChkData[path_id]=4;

					unsigned int j;
					unsigned int IsGetSPS=0;
					for(j=0;j<i ;j++){
						if(pRecvAddr[j+0]==0 &&pRecvAddr[j+1]==0 &&pRecvAddr[j+2]==0 &&pRecvAddr[j+3]==1 && ((pRecvAddr[j+4]& 0x1F) == H264_TYPE_SPS)){
							IsGetSPS=1;
							g_SocketCliData2_DescSize[path_id]=pRecvAddr[j-2]+ (pRecvAddr[j-1]<<8);
							break;
						}
					}
					if(IsGetSPS==0){
						//diag_printf("==========Fail Get SPS============\r\n");
						continue;
					}
					g_SocketCliData2_BsSize[path_id]=pRecvAddr[i-2]+ (pRecvAddr[i-1]<<8);
					g_SocketCliData2_DescSize[path_id] +=g_SocketCliData2_BsSize[path_id];
					bChkData[path_id]=3;
				}

				if(g_SocketCliData_DebugReceiveEn[path_id][ETHSOCKIPCCLI_ID_1]){
					unsigned char*  addr_tmp= (unsigned char*)pRecvAddr;
					printf("22[%d]DbgRecv i=%d, i+0=%d,bssz=%d, recsz=%d ,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",path_id,i,g_SocketCliData2_IdxInGOP[path_id],g_SocketCliData2_BsSize[path_id] , g_SocketCliData2_RecvSize[path_id],addr_tmp[0] ,addr_tmp[1],addr_tmp[2],addr_tmp[3],addr_tmp[4],addr_tmp[5],addr_tmp[6],addr_tmp[7],addr_tmp[8],addr_tmp[9],addr_tmp[10],addr_tmp[11],addr_tmp[12],addr_tmp[13],addr_tmp[14],addr_tmp[15],addr_tmp[16],addr_tmp[17],addr_tmp[18],addr_tmp[19],addr_tmp[20],addr_tmp[21],addr_tmp[22],addr_tmp[23],addr_tmp[24]);
					printf("bChkData=0x%x, size=%d\r\n",bChkData[path_id],size);
				}

				//if(g_SocketCliData2_BsSize[path_id] >=gSndParamSize[path_id][ETHSOCKIPCCLI_ID_1]){
				if(g_SocketCliData2_BsSize[path_id] >=g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_1]){
					//ETHSOCKETCLI_ERR(path_id, ETHSOCKIPCCLI_ID_1,"Frm Size Error!! sz=%d\r\n",g_SocketCliData2_BsSize[path_id]);
					ETHSOCKETCLI_ERR(path_id, ETHSOCKIPCCLI_ID_1,"BsSize over flow ,wanted[%d]=%d, but now=%d, bsTotalBufSz=%d\r\n",path_id,gSndParamSize[path_id][ETHSOCKIPCCLI_ID_1],g_SocketCliData2_BsSize[path_id],g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_1]);

				}
				if(g_SocketCliData2_BsSize[path_id]){
					g_SocketCliData2_BsStartPos[path_id]=i;//4;//size_get;//remove size header
					//g_SocketCliData2_IdxInGOP=pRecvAddr[i-1];
					//g_SocketCliData2_IdxInGOP[path_id]=pRecvAddr[i-5];
					if(i>=6){
						g_SocketCliData2_IdxInGOP[path_id]=pRecvAddr[i-5] + (((pRecvAddr[i-6] & 0xf0)>>4)<<8);
					}
					//printf("chk=0x%x, idx=%d, 0x%x, 0x%x, 0x%x\r\n", bChkData[path_id], g_SocketCliData2_IdxInGOP[path_id], pRecvAddr[i-5] , (((pRecvAddr[i-6] & 0xf0)>>4)<<8), pRecvAddr[i-6]);
					if(pRecvAddr[i+4] == HEAD_TYPE_THUMB || pRecvAddr[i+4] == HEAD_TYPE_RAW_ENCODE){
						g_SocketCliData2_BsSize[path_id]+=5; //start code
						//diag_printf("user_id=%d, start=%d, size_total=%d\r\n",g_SocketCliData1_IdxInGOP,g_SocketCliData1_BsStartPos,g_SocketCliData1_BsSize);
					}else{
						//diag_printf("frm_idx=%d, start=%d, size_total=%d\r\n",g_SocketCliData1_IdxInGOP[path_id],g_SocketCliData1_BsStartPos[path_id],g_SocketCliData1_BsSize[path_id]);
					}
					bChkData[path_id]|=0x10;
					//printf("11 bChkData=0x%x\r\n",bChkData[path_id]);

					#if 1 //new method to reduce memcpy time
					//unsigned char *uiBsFrmBufAddr=NULL;
					unsigned int LongCounterOffset=0;
					unsigned int DescOffset=0;  //for H265 I Frame Bs Align 4, for VdoDec

					if(((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_IDR && (*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +5)) == H264_START_CODE_I) ||
						((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_SLICE && (*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +5)) == H264_START_CODE_P)){

					//diag_printf("2244 %d, %d\r\n",g_SocketCliData2_IdxInGOP[path_id],g_SocketCliData2_BsSize[path_id]);
						if(g_SocketCliData2_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
							LongCounterOffset=16;
						}else{
							LongCounterOffset=0;
						}
						if(((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_IDR)){
							if(g_SocketCliData2_DescSize[path_id] == 0){
								if(errCnt % 300 == 0){
									ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"h264 DescSize 0!!\r\n");
								}
								errCnt++;
								return;
							}
							if(g_SocketCliData2_DescSize[path_id]%4){
								DescOffset=4-(g_SocketCliData2_DescSize[path_id]%4);
							}
							DescOffset +=g_SocketCliData2_DescSize[path_id];//add offset for sps + pps
							DescOffset+=TS_VIDEO_RESERVED_SIZE;
						}else{
							DescOffset=TS_VIDEO_RESERVED_SIZE;
						}

						//uiBsFrmBufAddr =ethsocket_cli_ipc_Data2_GetBsAlignAddr(path_id, (DescOffset+g_SocketCliData2_BsSize[path_id]+LongCounterOffset));
						//uiBsFrmBufAddr =ethsocket_cli_ipc_Data2_GetBsAlignAddr(path_id, (DescOffset+g_SocketCliData2_RecvSize[path_id]+LongCounterOffset));
						uiFrmBuf = (g_SocketCliData2_BsSize[path_id] > g_SocketCliData2_RecvSize[path_id] ) ? g_SocketCliData2_BsSize[path_id] : g_SocketCliData2_RecvSize[path_id] ;
						uiBsFrmBufAddr =ethsocket_cli_ipc_Data2_GetBsAlignAddr(path_id, (DescOffset+uiFrmBuf+LongCounterOffset));

						g_SocketCliData2_BsFrmBufAddr[path_id]=uiBsFrmBufAddr+DescOffset;
						memcpy((unsigned char*)(uiBsFrmBufAddr+DescOffset), (unsigned char*)(pRecvAddr+ g_SocketCliData2_BsStartPos[path_id]), (g_SocketCliData2_RecvSize[path_id]- g_SocketCliData2_BsStartPos[path_id]));
						g_SocketCliData2_RecvSize[path_id]-= g_SocketCliData2_BsStartPos[path_id];
						g_SocketCliData2_BsStartPos[path_id]=0;
						pRecvAddr = g_SocketCliData2_BsFrmBufAddr[path_id];

						memset((unsigned char*)uiBsFrmBufAddr, 0 ,DescOffset);
						bChkData[path_id]|=0x20;
					}else if((((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)>> 0x1)&0x3F) == H265_TYPE_VPS ) ||
						(((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)>> 0x1)&0x3F) == H265_TYPE_SLICE)){

						//unsigned char *uiBsFrmBufAddr=NULL;
						//unsigned int LongCounterOffset=0;
						//unsigned int DescOffset=0;  //for H265 I Frame Bs Align 4, for VdoDec
						if(g_SocketCliData2_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
							LongCounterOffset=16; //(8+5) and align 4
						}else{
							LongCounterOffset=0;
						}
						if(((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)>> 0x1) == H265_TYPE_VPS )){
							if(g_SocketCliData2_DescSize[path_id] == 0){
								if(errCnt % 300 == 0){
									ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"h265 DescSize 0!!\r\n");
								}
								errCnt++;
								return;
							}
							if(g_SocketCliData2_DescSize[path_id]%4){
								DescOffset=4-(g_SocketCliData2_DescSize[path_id]%4);
							}
							DescOffset+=TS_VIDEO_RESERVED_SIZE;
						}else{
							DescOffset=TS_VIDEO_RESERVED_SIZE;
						}
						//unsigned int   uiTime =drv_getSysTick();

						//unsigned char*  addr_tmp= (unsigned char*)pRecvAddr;
						//diag_printf("11 i+0=%d,bssz=%d, recsz=%d ,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",g_SocketCliData2_IdxInGOP[path_id],g_SocketCliData2_BsSize[path_id] , g_SocketCliData2_RecvSize[path_id],addr_tmp[0] ,addr_tmp[1],addr_tmp[2],addr_tmp[3],addr_tmp[4],addr_tmp[5],addr_tmp[6],addr_tmp[7],addr_tmp[8],addr_tmp[9],addr_tmp[10],addr_tmp[11],addr_tmp[12],addr_tmp[13],addr_tmp[14],addr_tmp[15],addr_tmp[16],addr_tmp[17],addr_tmp[18],addr_tmp[19],addr_tmp[20],addr_tmp[21],addr_tmp[22],addr_tmp[23],addr_tmp[24]);

						//uiBsFrmBufAddr =ethsocket_cli_ipc_Data2_GetBsAlignAddr(path_id, (DescOffset+g_SocketCliData2_BsSize[path_id]+LongCounterOffset));
						//uiBsFrmBufAddr =ethsocket_cli_ipc_Data2_GetBsAlignAddr(path_id, (DescOffset+g_SocketCliData2_RecvSize[path_id]+LongCounterOffset));
						uiFrmBuf = (g_SocketCliData2_BsSize[path_id] > g_SocketCliData2_RecvSize[path_id] ) ? g_SocketCliData2_BsSize[path_id] : g_SocketCliData2_RecvSize[path_id] ;
						uiBsFrmBufAddr =ethsocket_cli_ipc_Data2_GetBsAlignAddr(path_id, (DescOffset+uiFrmBuf+LongCounterOffset));

						//diag_printf("2uiBsFrmBufAddr=%x, recvSz=%d, bsSz=%d, BsQIdx=%d\r\n",uiBsFrmBufAddr,g_SocketCliData2_RecvSize[path_id],g_SocketCliData2_BsSize[path_id],g_SocketCliData2_BsQueueIdx[path_id]);

						g_SocketCliData2_BsFrmBufAddr[path_id]=uiBsFrmBufAddr+DescOffset;
						memcpy((unsigned char*)(uiBsFrmBufAddr+DescOffset), (unsigned char*)(pRecvAddr+ g_SocketCliData2_BsStartPos[path_id]), (g_SocketCliData2_RecvSize[path_id]- g_SocketCliData2_BsStartPos[path_id]));

						g_SocketCliData2_RecvSize[path_id]-= g_SocketCliData2_BsStartPos[path_id];
						g_SocketCliData2_BsStartPos[path_id]=0;
						pRecvAddr = g_SocketCliData2_BsFrmBufAddr[path_id];

						//addr_tmp= (unsigned char*)g_SocketCliData2_BsFrmBufAddr;
						//diag_printf("22 i+0=%d,bssz=%d, recsz=%d ,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",g_SocketCliData2_IdxInGOP[path_id],g_SocketCliData2_BsSize[path_id] , g_SocketCliData2_RecvSize[path_id],addr_tmp[0] ,addr_tmp[1],addr_tmp[2],addr_tmp[3],addr_tmp[4],addr_tmp[5],addr_tmp[6],addr_tmp[7],addr_tmp[8],addr_tmp[9],addr_tmp[10],addr_tmp[11],addr_tmp[12],addr_tmp[13],addr_tmp[14],addr_tmp[15],addr_tmp[16],addr_tmp[17],addr_tmp[18],addr_tmp[19],addr_tmp[20],addr_tmp[21],addr_tmp[22],addr_tmp[23],addr_tmp[24]);

						//diag_printf("t1: %d us\r\n", drv_getSysTick() - uiTime);

						//g_CliSocket_size_frm[g_BsFrmQueueIdx]=g_bs_size_total;
						memset((unsigned char*)uiBsFrmBufAddr, 0 ,DescOffset);
						bChkData[path_id]|=0x40;

					}
					#endif


					break;
				}
				//break;
			}
		}
		if(g_SocketCliData2_BsSize[path_id] >0 && (g_SocketCliData2_BsSize[path_id] + g_SocketCliData2_BsStartPos[path_id]) <= (g_SocketCliData2_RecvSize[path_id])){
			unsigned char *uiBsFrmBufAddr=NULL;
			unsigned int LongCounterOffset=0;
			unsigned int DescOffset=0;  //for H265 I Frame Bs Align 4, for VdoDec

			//if(g_queue_idx>=H264_QUEUE_MAX)
			//	g_queue_idx=0;
			bPushData=0;
			//I or P Frame
			if(((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_IDR && (*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +5)) == H264_START_CODE_I) ||
				((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_SLICE && (*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +5)) == H264_START_CODE_P)){

				#if 0
				if(g_SocketCliData2_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					LongCounterOffset=16;
				}else{
					LongCounterOffset=0;
				}
				if(((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_IDR)){
					if(g_SocketCliData2_DescSize[path_id] == 0){
						if(errCnt % 300 == 0){
							ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"DescSize 0!!\r\n");
						}
						errCnt++;
						return;
					}
					if(g_SocketCliData2_DescSize[path_id]%4){
						DescOffset=4-(g_SocketCliData2_DescSize[path_id]%4);
					}
					DescOffset +=g_SocketCliData2_DescSize[path_id];//add offset for sps + pps
					DescOffset+=TS_VIDEO_RESERVED_SIZE;
				}else{
					DescOffset=TS_VIDEO_RESERVED_SIZE;
				}
				//diag_printf("DescSize=%d, %d\r\n", g_SocketCliData2_DescSize[path_id],DescOffset);

				//uiBsFrmBufAddr =ethsocket_cli_ipc_Data2_GetBsAlignAddr(path_id, g_SocketCliData2_BsSize[path_id]);
				uiBsFrmBufAddr =ethsocket_cli_ipc_Data2_GetBsAlignAddr(path_id, (DescOffset+g_SocketCliData2_BsSize[path_id]+LongCounterOffset));


				//memcpy((unsigned char*)uiBsFrmBufAddr, (unsigned char*)(pRecvAddr + g_SocketCliData2_BsStartPos[path_id]), g_SocketCliData2_BsSize[path_id]);
				memcpy((unsigned char*)(uiBsFrmBufAddr+DescOffset), (unsigned char*)(pRecvAddr + g_SocketCliData2_BsStartPos[path_id]), g_SocketCliData2_BsSize[path_id]);

				//NvtIPC_FlushCache(addr, size);
				//g_CliSocket_size_frm[g_BsFrmQueueIdx]=g_bs_size_total;
				memset((unsigned char*)uiBsFrmBufAddr, 0 ,DescOffset);

				if(g_SocketCliData2_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					unsigned int sz_tmp=g_SocketCliData2_BsSize[path_id]+DescOffset;
					uiBsFrmBufAddr[0+sz_tmp]=0;
					uiBsFrmBufAddr[1+sz_tmp]=0;
					uiBsFrmBufAddr[2+sz_tmp]=0;
					uiBsFrmBufAddr[3+sz_tmp]=1;
					uiBsFrmBufAddr[4+sz_tmp]=HEAD_TYPE_LONGCNT_STAMP;
					uiBsFrmBufAddr[5+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0xff000000)>>24;
					uiBsFrmBufAddr[6+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0x00ff0000)>>16;
					uiBsFrmBufAddr[7+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0x0000ff00)>>8;
					uiBsFrmBufAddr[8+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0x000000ff)>>0;
					uiBsFrmBufAddr[9+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0xff000000)>>24;
					uiBsFrmBufAddr[10+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0x00ff0000)>>16;
					uiBsFrmBufAddr[11+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0x0000ff00)>>8;
					uiBsFrmBufAddr[12+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0x000000ff)>>0;

					uiBsFrmBufAddr[13+sz_tmp]=0;
					uiBsFrmBufAddr[14+sz_tmp]=0;
					uiBsFrmBufAddr[15+sz_tmp]=0;
				}
				#else
				if(g_SocketCliData2_BsFrmBufAddr[path_id] == NULL){
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"h264 g_SocketCliData2_BsFrmBufAddr 0!!\r\n");
					return;
				}
				if(g_SocketCliData2_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					LongCounterOffset=16;
				}else{
					LongCounterOffset=0;
				}
				if(((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_IDR)){
					if(g_SocketCliData2_DescSize[path_id] == 0){
						if(errCnt % 300 == 0){
							ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"DescSize 0!!\r\n");
						}
						errCnt++;
						return;
					}
					if(g_SocketCliData2_DescSize[path_id]%4){
						DescOffset=4-(g_SocketCliData2_DescSize[path_id]%4);
					}
					DescOffset +=g_SocketCliData2_DescSize[path_id];//add offset for sps + pps
					DescOffset+=TS_VIDEO_RESERVED_SIZE;
				}else{
					DescOffset=TS_VIDEO_RESERVED_SIZE;
				}
				uiBsFrmBufAddr=g_SocketCliData2_BsFrmBufAddr[path_id]-DescOffset;


				if(g_SocketCliData2_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					unsigned int sz_tmp=g_SocketCliData2_BsSize[path_id]+DescOffset;
					g_SocketCliData2_LongConterByteBK[path_id][0]=uiBsFrmBufAddr[0+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][1]=uiBsFrmBufAddr[1+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][2]=uiBsFrmBufAddr[2+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][3]=uiBsFrmBufAddr[3+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][4]=uiBsFrmBufAddr[4+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][5]=uiBsFrmBufAddr[5+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][6]=uiBsFrmBufAddr[6+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][7]=uiBsFrmBufAddr[7+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][8]=uiBsFrmBufAddr[8+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][9]=uiBsFrmBufAddr[9+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][10]=uiBsFrmBufAddr[10+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][11]=uiBsFrmBufAddr[11+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][12]=uiBsFrmBufAddr[12+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][13]=uiBsFrmBufAddr[13+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][14]=uiBsFrmBufAddr[14+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][15]=uiBsFrmBufAddr[15+sz_tmp];

					uiBsFrmBufAddr[0+sz_tmp]=0;
					uiBsFrmBufAddr[1+sz_tmp]=0;
					uiBsFrmBufAddr[2+sz_tmp]=0;
					uiBsFrmBufAddr[3+sz_tmp]=1;
					uiBsFrmBufAddr[4+sz_tmp]=HEAD_TYPE_LONGCNT_STAMP;
					uiBsFrmBufAddr[5+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0xff000000)>>24;
					uiBsFrmBufAddr[6+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0x00ff0000)>>16;
					uiBsFrmBufAddr[7+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0x0000ff00)>>8;
					uiBsFrmBufAddr[8+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0x000000ff)>>0;
					uiBsFrmBufAddr[9+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0xff000000)>>24;
					uiBsFrmBufAddr[10+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0x00ff0000)>>16;
					uiBsFrmBufAddr[11+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0x0000ff00)>>8;
					uiBsFrmBufAddr[12+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0x000000ff)>>0;

					uiBsFrmBufAddr[13+sz_tmp]=0;
					uiBsFrmBufAddr[14+sz_tmp]=0;
					uiBsFrmBufAddr[15+sz_tmp]=0;
				}
				#endif
				bPushData=1;
			}else if((((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)>> 0x1)&0x3F) == H265_TYPE_VPS ) ||
				(((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)>> 0x1)&0x3F) == H265_TYPE_SLICE)){
				#if 0
				if(g_SocketCliData2_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					LongCounterOffset=16; //(8+5) and align 4
				}else{
					LongCounterOffset=0;
				}
				if(((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)>> 0x1) == H265_TYPE_VPS )){

					if(g_SocketCliData2_DescSize[path_id]%4){
						DescOffset=4-(g_SocketCliData2_DescSize[path_id]%4);
					}
					DescOffset+=TS_VIDEO_RESERVED_SIZE;
				}else{
					DescOffset=TS_VIDEO_RESERVED_SIZE;
				}

				uiBsFrmBufAddr =ethsocket_cli_ipc_Data2_GetBsAlignAddr(path_id, (DescOffset+g_SocketCliData2_BsSize[path_id]+LongCounterOffset));

				memcpy((unsigned char*)(uiBsFrmBufAddr+DescOffset), (unsigned char*)(pRecvAddr + g_SocketCliData2_BsStartPos[path_id]), g_SocketCliData2_BsSize[path_id]);
				//g_CliSocket_size_frm[g_BsFrmQueueIdx]=g_bs_size_total;
				memset((unsigned char*)uiBsFrmBufAddr, 0 ,DescOffset);

				if(g_SocketCliData2_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					unsigned int sz_tmp=g_SocketCliData2_BsSize[path_id]+DescOffset;
					uiBsFrmBufAddr[0+sz_tmp]=0;
					uiBsFrmBufAddr[1+sz_tmp]=0;
					uiBsFrmBufAddr[2+sz_tmp]=0;
					uiBsFrmBufAddr[3+sz_tmp]=1;
					uiBsFrmBufAddr[4+sz_tmp]=HEAD_TYPE_LONGCNT_STAMP;
					uiBsFrmBufAddr[5+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0xff000000)>>24;
					uiBsFrmBufAddr[6+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0x00ff0000)>>16;
					uiBsFrmBufAddr[7+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0x0000ff00)>>8;
					uiBsFrmBufAddr[8+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0x000000ff)>>0;
					uiBsFrmBufAddr[9+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0xff000000)>>24;
					uiBsFrmBufAddr[10+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0x00ff0000)>>16;
					uiBsFrmBufAddr[11+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0x0000ff00)>>8;
					uiBsFrmBufAddr[12+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0x000000ff)>>0;

					uiBsFrmBufAddr[13+sz_tmp]=0;
					uiBsFrmBufAddr[14+sz_tmp]=0;
					uiBsFrmBufAddr[15+sz_tmp]=0;
				}
				#else
				if(g_SocketCliData2_BsFrmBufAddr[path_id] == NULL){
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"h265 g_SocketCliData2_BsFrmBufAddr 0!!\r\n");
					return;
				}
				if(g_SocketCliData2_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					LongCounterOffset=16; //(8+5) and align 4
				}else{
					LongCounterOffset=0;
				}
				if(((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)>> 0x1) == H265_TYPE_VPS )){
					if(g_SocketCliData2_DescSize[path_id] == 0){
						if(errCnt % 300 == 0){
							ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"h265 DescSize 0!!\r\n");
						}
						errCnt++;
						return;
					}
					if(g_SocketCliData2_DescSize[path_id]%4){
						DescOffset=4-(g_SocketCliData2_DescSize[path_id]%4);
					}
					DescOffset+=TS_VIDEO_RESERVED_SIZE;
				}else{
					DescOffset=TS_VIDEO_RESERVED_SIZE;
				}

				uiBsFrmBufAddr=g_SocketCliData2_BsFrmBufAddr[path_id]-DescOffset;

				if(g_SocketCliData2_LongConterStampEn[path_id]==LONGCNT_STAMP_EN){
					unsigned int sz_tmp=g_SocketCliData2_BsSize[path_id]+DescOffset;
					g_SocketCliData2_LongConterByteBK[path_id][0]=uiBsFrmBufAddr[0+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][1]=uiBsFrmBufAddr[1+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][2]=uiBsFrmBufAddr[2+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][3]=uiBsFrmBufAddr[3+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][4]=uiBsFrmBufAddr[4+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][5]=uiBsFrmBufAddr[5+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][6]=uiBsFrmBufAddr[6+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][7]=uiBsFrmBufAddr[7+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][8]=uiBsFrmBufAddr[8+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][9]=uiBsFrmBufAddr[9+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][10]=uiBsFrmBufAddr[10+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][11]=uiBsFrmBufAddr[11+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][12]=uiBsFrmBufAddr[12+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][13]=uiBsFrmBufAddr[13+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][14]=uiBsFrmBufAddr[14+sz_tmp];
					g_SocketCliData2_LongConterByteBK[path_id][15]=uiBsFrmBufAddr[15+sz_tmp];
					uiBsFrmBufAddr[0+sz_tmp]=0;
					uiBsFrmBufAddr[1+sz_tmp]=0;
					uiBsFrmBufAddr[2+sz_tmp]=0;
					uiBsFrmBufAddr[3+sz_tmp]=1;
					uiBsFrmBufAddr[4+sz_tmp]=HEAD_TYPE_LONGCNT_STAMP;
					uiBsFrmBufAddr[5+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0xff000000)>>24;
					uiBsFrmBufAddr[6+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0x00ff0000)>>16;
					uiBsFrmBufAddr[7+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0x0000ff00)>>8;
					uiBsFrmBufAddr[8+sz_tmp]=(g_SocketCliData2_LongConterHigh[path_id]  & 0x000000ff)>>0;
					uiBsFrmBufAddr[9+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0xff000000)>>24;
					uiBsFrmBufAddr[10+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0x00ff0000)>>16;
					uiBsFrmBufAddr[11+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0x0000ff00)>>8;
					uiBsFrmBufAddr[12+sz_tmp]=(g_SocketCliData2_LongConterLow[path_id] & 0x000000ff)>>0;

					uiBsFrmBufAddr[13+sz_tmp]=0;
					uiBsFrmBufAddr[14+sz_tmp]=0;
					uiBsFrmBufAddr[15+sz_tmp]=0;
				}
				#endif
				bPushData=1;
			}
			#if 0
			else if((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)==HEAD_TYPE_THUMB) ||
			(*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)==HEAD_TYPE_RAW_ENCODE)){
				if(g_SocketCliData_RawEncodeAddr[path_id][ETHSOCKIPCCLI_ID_1] !=0 && g_SocketCliData_RawEncodeSize[path_id][ETHSOCKIPCCLI_ID_1] !=0){
					//keep header 0 0 0 1 0xfe/0xff for uitron
					//memcpy((unsigned char*)g_SocketCliData1_RawEncodeAddr, (unsigned char*)(pRecvAddr + g_SocketCliData1_BsStartPos+5), (g_SocketCliData1_BsSize-5));
					memcpy((unsigned char*)g_SocketCliData_RawEncodeAddr[path_id][ETHSOCKIPCCLI_ID_1], (unsigned char*)(pRecvAddr + g_SocketCliData2_BsStartPos[path_id]), (g_SocketCliData2_BsSize[path_id]));
					//g_SocketCliData2_RawEncodeSize=g_SocketCliData2_BsSize-5;
					g_SocketCliData_RawEncodeSize[path_id][ETHSOCKIPCCLI_ID_1]=g_SocketCliData2_BsSize[path_id];
					if (*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4) == 0xFF) {
						bPushData = 2;
					} else if (*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4) == 0xFE) {
						bPushData = 3;
					}
				}else{
					if (ETHSOCKETCLI_INVALID_PARAMADDR == g_SocketCliData_RawEncodeAddr[path_id][ETHSOCKIPCCLI_ID_1]) {
						ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"g_SocketCliData_RawEncodeAddr = 0x%X\n", g_SocketCliData_RawEncodeAddr[path_id][ETHSOCKIPCCLI_ID_1]);
					}
					if (0 == g_SocketCliData_RawEncodeSize[path_id][ETHSOCKIPCCLI_ID_1]) {
						ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"g_SocketCliData_RawEncodeSize = 0x%X\n", g_SocketCliData_RawEncodeSize[path_id][ETHSOCKIPCCLI_ID_1]);
					}
				}
			}
			#endif
			#if 1
			if((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_IDR){
				//diag_printf("eD2_I[%d]\r\n",path_id);
				//diag_printf("eD2_I\r\n");
				//g_queue_idx++;
				//g_SocketCliData1_BsFrameCnt++;
				//eth_is_i_frame = 1;
				//eth_i_cnt ++;
			}else if((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_SLICE){
				//diag_printf("eD2_P\r\n");
				//g_queue_idx++;
				//g_SocketCliData1_BsFrameCnt++;
				//eth_is_i_frame = 0;
			}else if((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_SPS){
				//diag_printf("SPS OK\r\n");
			}else if((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)& 0x1F) == H264_TYPE_PPS){
				//diag_printf("PPS OK\r\n");

			}else if(((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)>> 0x1)&0x3F) == H265_TYPE_VPS){
				//diag_printf("eD2_I[%d], sz=%d\r\n",path_id,g_SocketCliData2_BsSize[path_id]);
				//diag_printf("eD1_I\r\n");
				//g_queue_idx++;
				//g_SocketCliData1_BsFrameCnt++;
				//eth_is_i_frame = 1;
				//eth_i_cnt ++;
				//printf("DELAY\r\n");
				//HAL_DELAY_US(10000);
			}else if(((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)>> 0x1)&0x3F) == H265_TYPE_SLICE){
				//diag_printf("eD2_P[%d], sz=%d\r\n",path_id,g_SocketCliData2_BsSize[path_id]);
				//diag_printf("eD1_P\r\n");
				//g_queue_idx++;
				//g_SocketCliData1_BsFrameCnt++;
				//eth_is_i_frame = 0;
			}else if((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)==HEAD_TYPE_THUMB)){
				//DBG_DUMP("Thumb OK\r\n");
				//DBG_DUMP("data[5]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",*(g_psocket_cli + g_bs_size_start +5),*(g_psocket_cli + g_bs_size_start +6),*(g_psocket_cli + g_bs_size_start +g_bs_size_total-5),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-4),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-3),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-2),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-1),*(g_psocket_cli + g_bs_size_start + g_bs_size_total));
			}else if((*(pRecvAddr + g_SocketCliData2_BsStartPos[path_id] +4)==HEAD_TYPE_RAW_ENCODE)){
				//DBG_DUMP("PIM OK\r\n");
				//DBG_DUMP("data[5]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",*(g_psocket_cli + g_bs_size_start +5),*(g_psocket_cli + g_bs_size_start +6),*(g_psocket_cli + g_bs_size_start +g_bs_size_total-5),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-4),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-3),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-2),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-1),*(g_psocket_cli + g_bs_size_start + g_bs_size_total));
			}else{
				//DBG_DUMP("Check FAIL\r\n");
			}
			#endif

			if(bPushData==1 ){
				//push data to VdoDec
				ETHSOCKETCLI_RECV_PARAM *pParam = (ETHSOCKETCLI_RECV_PARAM *)ethsocket_cli_ipc_GetSndAddr(path_id, ETHSOCKIPCCLI_ID_1);
				unsigned int checkSumHead=0;
				unsigned int checkSumTail=0;
				unsigned int checksum = 0;
				unsigned int checksumLen = 64;
				//pParam->obj = 0;
				pParam->size = (int)(g_SocketCliData2_BsSize[path_id]+ LongCounterOffset+DescOffset);
				//pParam->buf=(char *)(uiBsFrmBufAddr);
				pParam->buf=(char *)(ipc_getNonCacheAddr((unsigned int)uiBsFrmBufAddr));

				//diag_printf("eCosD2Frm Size=%d, Addr=0x%x\r\n",pParam->size, pParam->buf);
				//if(((*(pRecvAddr + g_SocketCliData1_BsStartPos +4)& 0x1F) == H264_NALU_TYPE_IDR) ||
				//((*(pRecvAddr + g_SocketCliData1_BsStartPos +4)& 0x1F) == H264_NALU_TYPE_SLICE)){
				//}
				//diag_printf("I/P size=%d, 0x%x, [0]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",  pParam->size, pParam->buf,pParam->buf[0],pParam->buf[1],pParam->buf[2],pParam->buf[3],pParam->buf[4]);
				ipc_flushCache((NVTIPC_U32)uiBsFrmBufAddr, (NVTIPC_U32)(g_SocketCliData2_BsSize[path_id]+LongCounterOffset+DescOffset));
				ipc_flushCache((NVTIPC_U32)pParam, (NVTIPC_U32)sizeof(ETHSOCKETCLI_RECV_PARAM));

				if(g_SocketCliData2_BsSize[path_id]>checksumLen){
					checksum=0;
					for(i=0;i<checksumLen;i++){
						checksum+= *(uiBsFrmBufAddr+DescOffset+i);
					}
					checkSumHead=checksum & 0xff;//ethsocket_cli_ipc_Checksum((unsigned char*)uiBsFrmBufAddr, 64) & 0xff;
					checksum=0;
					for(i=(g_SocketCliData2_BsSize[path_id]-checksumLen) ;i<g_SocketCliData2_BsSize[path_id];i++){
						checksum+= *(uiBsFrmBufAddr+DescOffset+i);
					}
					checkSumTail=checksum & 0xff;//ethsocket_cli_ipc_Checksum((unsigned char*)(uiBsFrmBufAddr+ g_SocketCliData2_BsSize-1-64), 64) & 0xff;
				}
				if(checkSumHead== g_SocketCliData2_checkSumHead[path_id]
					&& checkSumTail== g_SocketCliData2_checkSumTail[path_id]){
					//diag_printf("push[%d], sz=%d\r\n",path_id,g_SocketCliData2_BsSize[path_id]);

					result = ethsocket_cli_ipc_exeapi(path_id, ETHSOCKIPCCLI_ID_1, ETHSOCKETCLI_CMDID_RCV, &Ret_EthsocketApi);
				}else{
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"chksum error, %d, %d, 0x%x, 0x%x, 0x%x, 0x%x\r\n",g_SocketCliData2_IdxInGOP[path_id],g_SocketCliData2_BsSize[path_id],g_SocketCliData2_checkSumHead[path_id],g_SocketCliData2_checkSumTail[path_id],checkSumHead,checkSumTail);
					//ETHSOCKETCLI_PARAM_PARAM *pNotifyParam = (ETHSOCKETCLI_PARAM_PARAM *)ethsocket_cli_ipc_GetSndAddr(ETHSOCKIPCCLI_ID_1);
					//pNotifyParam->param1 = (int)CYG_ETHSOCKETCLI_STATUS_CLIENT_SLOW;
					//pNotifyParam->param2 = g_SocketCliData2_IdxInGOP;
					//ipc_flushCache((NVTIPC_U32)pNotifyParam, (NVTIPC_U32)sizeof(ETHSOCKETCLI_PARAM_PARAM));
					//result = ethsocket_cli_ipc_exeapi(ETHSOCKIPCCLI_ID_1, ETHSOCKETCLI_CMDID_NOTIFY, &Ret_EthsocketApi);
				}

				if ((result != ETHSOCKETCLI_RET_OK) || (Ret_EthsocketApi!=0)) {
					ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"snd CB fail\r\n");
				}
			}

			if(1){//bPushData == 1){// || bPushData == 2 || bPushData == 3){
				memset(&g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][0], 0, ETHCAM_FRM_HEADER_SIZE);

				for(i=ETHCAM_FRM_HEADER_SIZE_PART1;i<ETHCAM_FRM_HEADER_SIZE && uiBsFrmBufAddr;i++){
					g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][i]=uiBsFrmBufAddr[i-ETHCAM_FRM_HEADER_SIZE_PART1+DescOffset];//pParam->buf[i-ETHCAM_FRM_HEADER_SIZE_PART1+DescOffset];
				}
				#if 1
				//unsigned char*  addr_tmp= (unsigned char*)&g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][ETHCAM_FRM_HEADER_SIZE_PART1];
				//diag_printf("11 i+0=%d,bssz=%d, recsz=%d ,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",g_SocketCliData2_IdxInGOP[path_id],g_SocketCliData2_BsSize[path_id] , g_SocketCliData2_RecvSize[path_id],addr_tmp[0] ,addr_tmp[1],addr_tmp[2],addr_tmp[3],addr_tmp[4],addr_tmp[5],addr_tmp[6],addr_tmp[7],addr_tmp[8],addr_tmp[9],addr_tmp[10],addr_tmp[11],addr_tmp[12],addr_tmp[13],addr_tmp[14],addr_tmp[15],addr_tmp[16],addr_tmp[17],addr_tmp[18],addr_tmp[19],addr_tmp[20],addr_tmp[21],addr_tmp[22],addr_tmp[23],addr_tmp[24]);

				//addr_tmp= (unsigned char*)(uiBsFrmBufAddr +DescOffset);
				//diag_printf("22 i+0=%d,bssz=%d, recsz=%d ,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",g_SocketCliData2_IdxInGOP[path_id],g_SocketCliData2_BsSize[path_id] , g_SocketCliData2_RecvSize[path_id],addr_tmp[0] ,addr_tmp[1],addr_tmp[2],addr_tmp[3],addr_tmp[4],addr_tmp[5],addr_tmp[6],addr_tmp[7],addr_tmp[8],addr_tmp[9],addr_tmp[10],addr_tmp[11],addr_tmp[12],addr_tmp[13],addr_tmp[14],addr_tmp[15],addr_tmp[16],addr_tmp[17],addr_tmp[18],addr_tmp[19],addr_tmp[20],addr_tmp[21],addr_tmp[22],addr_tmp[23],addr_tmp[24]);

				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][0]=((g_SocketCliData2_LongConterHigh[path_id] ) & 0xff000000)>>24;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][1]=((g_SocketCliData2_LongConterHigh[path_id] ) & 0x00ff0000)>>16;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][2]=((g_SocketCliData2_LongConterHigh[path_id] ) & 0x0000ff00)>>8;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][3]=(g_SocketCliData2_LongConterHigh[path_id]  ) & 0x000000ff;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][4]=(g_SocketCliData2_LongConterLow[path_id] & 0xff000000)>>24;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][5]=(g_SocketCliData2_LongConterLow[path_id]  & 0x00ff0000)>>16;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][6]=(g_SocketCliData2_LongConterLow[path_id]  & 0x0000ff00)>>8;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][7]=g_SocketCliData2_LongConterLow[path_id]  & 0x000000ff;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][8]=g_SocketCliData2_BsSize[path_id] & 0xff;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][9]=(g_SocketCliData2_BsSize[path_id] & 0xff00) >> 8;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][10]=(g_SocketCliData2_BsSize[path_id] & 0xff0000) >> 16;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][11]=g_SocketCliData2_IdxInGOP[path_id]  & 0xff;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][12]=g_SocketCliData2_checkSumHead[path_id]  & 0xff;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][13]=g_SocketCliData2_checkSumTail[path_id]  & 0xff;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][14]=g_SocketCliData2_DescSize[path_id]  & 0xff;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][15]=g_SocketCliData2_LongConterStampEn[path_id]  & 0xff;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][16]=(uiBsFrmBufAddr != NULL) ? ((((unsigned int )uiBsFrmBufAddr) & 0xff000000)>>24) : 0 ;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][17]=(uiBsFrmBufAddr != NULL) ? ((((unsigned int )uiBsFrmBufAddr ) & 0x00ff0000)>>16) : 0 ;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][18]=(uiBsFrmBufAddr != NULL) ? ((((unsigned int )uiBsFrmBufAddr ) & 0x0000ff00)>>8) : 0 ;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][19]=(uiBsFrmBufAddr != NULL) ? (((unsigned int )uiBsFrmBufAddr ) & 0x000000ff) : 0 ;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][20]=LongCounterOffset & 0xff;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][21]=DescOffset & 0xff;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][22]=bChkData[path_id]& 0xff;
				g_Data2_Tx_PrevHeader[path_id][g_Data2_Tx_PrevHeaderBufIdx][23]=bPushData & 0xff;



				g_Data2_Tx_PrevHeaderBufIdx++;
				g_Data2_Tx_PrevHeaderBufIdx= (g_Data2_Tx_PrevHeaderBufIdx >=ETHCAM_PREVHEADER_MAX_FRMNUM) ? 0 : g_Data2_Tx_PrevHeaderBufIdx;
				#endif

			}

			//diag_printf("2size_total=%d, size_get=%d, start=%d\r\n",g_SocketCliData1_BsSize, g_SocketCliData1_RecvSize,g_SocketCliData1_BsStartPos);
			//reset
			g_SocketCliData2_RecvSize[path_id] = g_SocketCliData2_RecvSize[path_id] -(g_SocketCliData2_BsSize[path_id] + g_SocketCliData2_BsStartPos[path_id]);
			//diag_printf("3size_total=%d, size_get=%d, start=%d\r\n",g_SocketCliData1_BsSize, g_SocketCliData1_RecvSize,g_SocketCliData1_BsStartPos);
			//diag_printf("3size_get=%d\r\n", g_SocketCliData1_RecvSize);

			if(g_SocketCliData2_RecvSize[path_id] != 0){
				//jump to next data
				//memmove((unsigned char *)pRecvAddr, (unsigned char *)(pRecvAddr + g_SocketCliData2_BsSize[path_id] + g_SocketCliData2_BsStartPos[path_id]), g_SocketCliData2_RecvSize[path_id]);
				//diag_printf("move 0x%x, 0x%x, %d\r\n",pRecvAddr, pRecvAddr + g_SocketCliData1_BsSize + g_SocketCliData1_BsStartPos, g_SocketCliData1_RecvSize);

				//jump to next data
				if(g_SocketCliData2_BsFrmBufAddr[path_id]==NULL){
					memmove((unsigned char *)pRecvAddr, (unsigned char *)(pRecvAddr + g_SocketCliData2_BsSize[path_id] + g_SocketCliData2_BsStartPos[path_id]), g_SocketCliData2_RecvSize[path_id]);
				}else
				{
					memcpy((unsigned char*)(g_SocketCliData2_RecvAddr[path_id]),(unsigned char *)(pRecvAddr + g_SocketCliData2_BsSize[path_id] + g_SocketCliData2_BsStartPos[path_id]), g_SocketCliData2_RecvSize[path_id]);
					pRecvAddr=(unsigned char*)(g_SocketCliData2_RecvAddr[path_id]);
					pRecvAddr[0]=g_SocketCliData2_LongConterByteBK[path_id][0];
					pRecvAddr[1]=g_SocketCliData2_LongConterByteBK[path_id][1];
					pRecvAddr[2]=g_SocketCliData2_LongConterByteBK[path_id][2];
					pRecvAddr[3]=g_SocketCliData2_LongConterByteBK[path_id][3];
					pRecvAddr[4]=g_SocketCliData2_LongConterByteBK[path_id][4];
					pRecvAddr[5]=g_SocketCliData2_LongConterByteBK[path_id][5];
					pRecvAddr[6]=g_SocketCliData2_LongConterByteBK[path_id][6];
					pRecvAddr[7]=g_SocketCliData2_LongConterByteBK[path_id][7];
					pRecvAddr[8]=g_SocketCliData2_LongConterByteBK[path_id][8];
					pRecvAddr[9]=g_SocketCliData2_LongConterByteBK[path_id][9];
					pRecvAddr[10]=g_SocketCliData2_LongConterByteBK[path_id][10];
					pRecvAddr[11]=g_SocketCliData2_LongConterByteBK[path_id][11];
					pRecvAddr[12]=g_SocketCliData2_LongConterByteBK[path_id][12];
					pRecvAddr[13]=g_SocketCliData2_LongConterByteBK[path_id][13];
					pRecvAddr[14]=g_SocketCliData2_LongConterByteBK[path_id][14];
					pRecvAddr[15]=g_SocketCliData2_LongConterByteBK[path_id][15];
				}
			}
			g_SocketCliData2_BsSize[path_id]=0;
			g_SocketCliData2_BsStartPos[path_id] =0;

			g_SocketCliData2_LongConterLow[path_id]=0;
			g_SocketCliData2_LongConterHigh[path_id]=0;
			g_SocketCliData2_LongConterStampEn[path_id]=0;
			uiBsFrmBufAddr =NULL;
			g_SocketCliData2_BsFrmBufAddr[path_id]=NULL;
			if(bPushData==1 ){
				g_SocketCliData2_DescSize[path_id]=0;
			}
			bChkData[path_id]=0;
		}else{
			//DBG_DUMP("else size_total=%d, size_get=%d\r\n",g_bs_size_total, g_CliSocket_size_get);
			break;
		}
	}
}

static void ethsocket_cli_ipc_recv_0_p0(int obj_index,  char* addr, int size)
{
	ethsocket_cli_ipc_recv(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_0,  addr, size);
}

static void ethsocket_cli_ipc_recv_1_p0(int obj_index,  char* addr, int size)
{
	ethsocket_cli_ipc_recv(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_1,  addr, size);
}

static void ethsocket_cli_ipc_recv_2_p0(int obj_index,  char* addr, int size)
{
	ethsocket_cli_ipc_recv(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_2,  addr, size);
}
static void ethsocket_cli_ipc_recv_0_p1(int obj_index,  char* addr, int size)
{
	ethsocket_cli_ipc_recv(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_0,  addr, size);
}

static void ethsocket_cli_ipc_recv_1_p1(int obj_index,  char* addr, int size)
{
	ethsocket_cli_ipc_recv(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_1,  addr, size);
}

static void ethsocket_cli_ipc_recv_2_p1(int obj_index,  char* addr, int size)
{
	ethsocket_cli_ipc_recv(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_2,  addr, size);
}

int ethsocket_cli_ipc_recv(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, char* addr, int size)
{
	int result =0;
#if 1
	if(id== ETHSOCKIPCCLI_ID_0){
		ethsocket_cli_ipc_Data1_ParseBsFrame(path_id, addr,size);
	}else if(id== ETHSOCKIPCCLI_ID_1){
		ethsocket_cli_ipc_Data2_ParseBsFrame(path_id, addr,size);
	}else{
		ETHSOCKETCLI_TRANSFER_PARAM *pParam = (ETHSOCKETCLI_TRANSFER_PARAM *)ethsocket_cli_ipc_GetSndAddr(path_id, id);
		int Ret_EthsocketApi = 0;

		ETHSOCKETCLI_PRINT(path_id,id,"%x %d\r\n  ",addr, size);

		if ((int)pParam == ETHSOCKETCLI_RET_ERR) {
			ETHSOCKETCLI_ERR(path_id,id,"pParam err\r\n");
			return ETHSOCKETCLI_RET_ERR;
		}
		//pParam->obj = (int)obj;
		pParam->size = (int)size;

		if (size > ETHSOCKCLIIPC_TRANSFER_BUF_SIZE) {
			ETHSOCKETCLI_ERR(path_id,id,"size %d > %d\r\n",size, ETHSOCKCLIIPC_TRANSFER_BUF_SIZE);
			return ETHSOCKETCLI_RET_ERR;
		} else {
			memcpy(pParam->buf,addr,size);
		}
		ipc_flushCache((NVTIPC_U32)pParam, (NVTIPC_U32)sizeof(ETHSOCKETCLI_TRANSFER_PARAM));

		result = ethsocket_cli_ipc_exeapi(path_id, id, ETHSOCKETCLI_CMDID_RCV, &Ret_EthsocketApi);

		if ((result != ETHSOCKETCLI_RET_OK) || (Ret_EthsocketApi!=0)) {
			ETHSOCKETCLI_PRINT(path_id,id,"snd CB fail\r\n");
		}
	}
	return result;
#endif
}
#if (__USE_IPC)
#if defined(__ECOS)
static void ethsocket_cli_ipc_receive_thread_0_p0(cyg_addrword_t arg)
{
	ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_0, arg);
}

static void ethsocket_cli_ipc_receive_thread_1_p0(cyg_addrword_t arg)
{
	ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_1, arg);
}

static void ethsocket_cli_ipc_receive_thread_2_p0(cyg_addrword_t arg)
{
	ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_2, arg);
}
static void ethsocket_cli_ipc_receive_thread_0_p1(cyg_addrword_t arg)
{
	ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_0, arg);
}

static void ethsocket_cli_ipc_receive_thread_1_p1(cyg_addrword_t arg)
{
	ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_1, arg);
}

static void ethsocket_cli_ipc_receive_thread_2_p1(cyg_addrword_t arg)
{
	ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_2, arg);
}

static void ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, cyg_addrword_t arg)
#else
static void* ethsocket_cli_ipc_receive_thread_0_p0(void* arg)
{
	return ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_0, arg);
}

static void* ethsocket_cli_ipc_receive_thread_1_p0(void* arg)
{
	return ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_1, arg);
}

static void* ethsocket_cli_ipc_receive_thread_2_p0(void* arg)
{
	return ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID_1, ETHSOCKIPCCLI_ID_2, arg);
}
static void* ethsocket_cli_ipc_receive_thread_0_p1(void* arg)
{
	return ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_0, arg);
}

static void* ethsocket_cli_ipc_receive_thread_1_p1(void* arg)
{
	return ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_1, arg);
}

static void* ethsocket_cli_ipc_receive_thread_2_p1(void* arg)
{
	return ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID_2, ETHSOCKIPCCLI_ID_2, arg);
}
static void* ethsocket_cli_ipc_receive_thread(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, void* arg)
#endif
{

	ETHSOCKETCLI_MSG RcvMsg;
	int Ret_ExeResult = ETHSOCKETCLI_RET_ERR;
	int Ret_IpcMsg;

	cyg_flag_maskbits(&ethsocket_cli_ipc_flag[path_id][id], 0);
	while (1) {
		//1. receive ipc cmd from uItron
		Ret_IpcMsg = ethsocket_cli_ipc_wait_cmd(path_id,id,&RcvMsg);
		if (ETHSOCKETCLI_RET_OK != Ret_IpcMsg) {
			ETHSOCKETCLI_ERR(path_id,id, "ethsocket_cli_ipc_waitcmd = %d\r\n", Ret_IpcMsg);
			continue;
		}

		//2. error check
		ETHSOCKETCLI_PRINT(path_id,id, "Got RcvCmdId = %d\r\n", RcvMsg.CmdId);
		if (RcvMsg.CmdId >= ETHSOCKETCLI_CMD_FP_MAX || RcvMsg.CmdId < 0) {
			ETHSOCKETCLI_ERR(path_id,id, "RcvCmdId(%d) should be 0~%d\r\n", RcvMsg.CmdId, ETHSOCKETCLI_CMD_FP_MAX);
			continue;
		}

		if (gCmdFpTbl[RcvMsg.CmdId].CmdId != RcvMsg.CmdId) {
			ETHSOCKETCLI_ERR(path_id,id, "RcvCmdId = %d, TableCmdId = %d\r\n", RcvMsg.CmdId, gCmdFpTbl[RcvMsg.CmdId].CmdId);
			continue;
		}

		//3. process the cmd
		if (RcvMsg.CmdId == ETHSOCKETCLI_CMDID_SYSREQ_ACK) {
			ethsocket_cli_ipc_set_ack(path_id,id,RcvMsg.Arg);
		} else if(RcvMsg.CmdId == ETHSOCKETCLI_CMDID_UNINIT) {
			ETHSOCKETCLIECOS_Close(path_id,id);
			return DUMMY_NULL;
		} else {
			if (gCmdFpTbl[RcvMsg.CmdId].pFunc) {
				Ret_ExeResult = gCmdFpTbl[RcvMsg.CmdId].pFunc(path_id, id);
				ETHSOCKETCLI_PRINT(path_id,id,"[TSK]Ret_ExeResult = 0x%X\r\n", Ret_ExeResult);
			} else {
				Ret_ExeResult = ETHSOCKETCLI_RET_NO_FUNC;
				ETHSOCKETCLI_ERR(path_id,id,"[TSK]Null Cmd pFunc id %d\r\n", RcvMsg.CmdId);
			}

			//send the msg of the return value of FileSys API
			Ret_IpcMsg = ethsocket_cli_ipc_send_ack(path_id, id, Ret_ExeResult);
			if (ETHSOCKETCLI_RET_OK != Ret_IpcMsg) {
				ETHSOCKETCLI_ERR(path_id,id,"ethsocket_cli_ipc_Msg_WaitCmd = %d\r\n", Ret_IpcMsg);
				continue;
			}
		}
	}
	return DUMMY_NULL;
}
#endif
static int ethsocket_cli_ipc_CmdId_Start(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id)
{
	ethsocket_cli_start(path_id, id);
	return ETHSOCKETCLI_RET_OK;
}

static int ethsocket_cli_ipc_CmdId_Connect(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id)
{
	//ETHSOCKETCLI_PARAM_PARAM *pParam = (ETHSOCKETCLI_PARAM_PARAM *)ethsocket_cli_ipc_GetRcvAddr();
	//ethsocket_cli_obj *pInstallObj = (ethsocket_cli_obj*)pParam;
	ethsocket_cli_obj *pInstallObj = (ethsocket_cli_obj*)ethsocket_cli_ipc_GetRcvAddr(path_id, id);

	ethsocket_cli_obj *pNewObj = 0;
	int result = 0;

	ETHSOCKETCLI_PRINT(path_id,id, "portNum:%d, sockbufSize:%d,timeout=%d,client_socket:%d,threadPriority=%d\r\n", pInstallObj->portNum, pInstallObj->sockbufSize,pInstallObj->timeout, pInstallObj->connect_socket,pInstallObj->threadPriority);

	pNewObj = ethsocket_cli_get_newobj(path_id, id);
	if (pNewObj) {
		if (pInstallObj->notify) {
			pInstallObj->notify = gNotify[path_id][id];
		}
		if (pInstallObj->recv) {
			pInstallObj->recv = gRecv[path_id][id];
		}
		ethsocket_cli_install(pNewObj, pInstallObj);
		result = ethsocket_cli_connect(id, pNewObj);
		ETHSOCKETCLI_PRINT(path_id,id,"result=%d\r\n",result);

		if (result == 0) {
			return (int)pNewObj;
		}else if(result == -2){//timeout
			return -2;
		}else if(result == -3){//Connection refused
			return -3;
		}else {
			ETHSOCKETCLI_ERR(path_id,id,"connect fail, result=%d\r\n",result);
			ethsocket_cli_uninstall(path_id, id, pNewObj);
			return 0;
		}
	} else {
		ETHSOCKETCLI_ERR(path_id,id,"get_newobj fail\r\n");
		return 0;
	}
}

static int ethsocket_cli_ipc_CmdId_Disconnect(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id)
{
#if 1
	ETHSOCKETCLI_PARAM_PARAM *pParam = (ETHSOCKETCLI_PARAM_PARAM *)ethsocket_cli_ipc_GetRcvAddr(path_id, id);
	ethsocket_cli_obj *pInstallObj = (ethsocket_cli_obj*)pParam->param1;

	if (pInstallObj) {
		ethsocket_cli_disconnect(pInstallObj);
		//ethsocket_cli_uninstall(pInstallObj);
		return ETHSOCKETCLI_RET_OK;
	} else {
		return ETHSOCKETCLI_RET_ERR;
	}
#else
		return ETHSOCKETCLI_RET_OK;
#endif
}

static int ethsocket_cli_ipc_CmdId_Stop(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id)
{
	ethsocket_cli_ipc_set_ack(path_id, id, ETHSOCKETVLI_RET_FORCE_ACK);

	ethsocket_cli_stop(path_id, id);
	return ETHSOCKETCLI_RET_OK;
}

static int ethsocket_cli_ipc_CmdId_Send(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id)
{
	ETHSOCKETCLI_TRANSFER_PARAM *pParam = (ETHSOCKETCLI_TRANSFER_PARAM *)ethsocket_cli_ipc_GetRcvAddr(path_id, id);

	return ethsocket_cli_send(path_id, id , pParam->buf, &(pParam->size));
}
void ethsocket_cli_ipc_init_data1_bs_buf(ETHCAM_PATH_ID path_id)
{
	if (ETHSOCKETCLI_INVALID_PARAMADDR == g_SocketCliData_BsBufTotalAddr[path_id][ETHSOCKIPCCLI_ID_0]) {
		ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"g_SocketCliData_BsBufTotalAddr = 0x%X\n", g_SocketCliData_BsBufTotalAddr[path_id][ETHSOCKIPCCLI_ID_0]);
	}
	if (0 == g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_0]) {
		ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"g_SocketCliData_BsBufTotalSize = 0x%X\n", g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_0]);
	}
	if (0 == g_SocketCliData_BsQueueMax[path_id][ETHSOCKIPCCLI_ID_0]) {
		ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_0,"g_SocketCliData_BsQueueMax = 0x%X\n", g_SocketCliData_BsQueueMax[path_id][ETHSOCKIPCCLI_ID_0]);
	}

	g_SocketCliData1_BsBufMapTbl[path_id] = (unsigned int *)g_SocketCliData_BsBufTotalAddr[path_id][ETHSOCKIPCCLI_ID_0];
	ETHSOCKETCLI_PRINT(path_id,ETHSOCKIPCCLI_ID_0,"g_EthBsFrmBufMapTbl=0x%x, g_EthBsFrmTotalAddr=0x%x, end=0x%x\r\n",g_SocketCliData1_BsBufMapTbl[path_id],g_SocketCliData_BsBufTotalAddr[path_id][ETHSOCKIPCCLI_ID_0],g_SocketCliData_BsBufTotalAddr[path_id][ETHSOCKIPCCLI_ID_0]+g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_0]);

	memset((char*)g_SocketCliData_BsBufTotalAddr[path_id][ETHSOCKIPCCLI_ID_0], 0, g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_0]);
	g_SocketCliData1_BsAddr[path_id] = (g_SocketCliData_BsBufTotalAddr[path_id][ETHSOCKIPCCLI_ID_0] + sizeof(unsigned int )*(g_SocketCliData_BsQueueMax[path_id][ETHSOCKIPCCLI_ID_0]));
	g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_0]-= (sizeof(unsigned int )*g_SocketCliData_BsQueueMax[path_id][ETHSOCKIPCCLI_ID_0]);
	//DBG_DUMP("1EthBsFrmAddr=0x%x, EthBsFrmBufTotalSize=0x%x, end=0x%x\r\n",g_EthBsFrmAddr,g_EthBsFrmBufTotalSize,g_EthBsFrmAddr+g_EthBsFrmBufTotalSize);
	while (g_SocketCliData1_BsAddr[path_id] % 4 != 0)
	{
		g_SocketCliData1_BsAddr[path_id] += 1;
	}
	g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_0]-=4;
	ETHSOCKETCLI_PRINT(path_id,ETHSOCKIPCCLI_ID_0,"2EthBsFrmAddr=0x%x, EthBsFrmBufTotalSize=0x%x, end=0x%x\r\n",g_SocketCliData1_BsAddr[path_id],g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_0],g_SocketCliData1_BsAddr[path_id]+g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_0]);
}
void ethsocket_cli_ipc_init_data2_bs_buf(ETHCAM_PATH_ID path_id)
{
	if (ETHSOCKETCLI_INVALID_PARAMADDR == g_SocketCliData_BsBufTotalAddr[path_id][ETHSOCKIPCCLI_ID_1]) {
		ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"g_SocketCliData_BsBufTotalAddr = 0x%X\n", g_SocketCliData_BsBufTotalAddr[path_id][ETHSOCKIPCCLI_ID_1]);
	}
	if (0 == g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_1]) {
		ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"g_SocketCliData_BsBufTotalSize = 0x%X\n", g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_1]);
	}
	if (0 == g_SocketCliData_BsQueueMax[path_id][ETHSOCKIPCCLI_ID_1]) {
		ETHSOCKETCLI_ERR(path_id,ETHSOCKIPCCLI_ID_1,"g_SocketCliData_BsQueueMax = 0x%X\n", g_SocketCliData_BsQueueMax[path_id][ETHSOCKIPCCLI_ID_1]);
	}

	g_SocketCliData2_BsBufMapTbl[path_id] = (unsigned int *)g_SocketCliData_BsBufTotalAddr[path_id][ETHSOCKIPCCLI_ID_1];
	ETHSOCKETCLI_PRINT(path_id,ETHSOCKIPCCLI_ID_1,"g_EthBsFrmBufMapTbl=0x%x, g_EthBsFrmTotalAddr=0x%x, end=0x%x\r\n",g_SocketCliData2_BsBufMapTbl[path_id],g_SocketCliData_BsBufTotalAddr[path_id][ETHSOCKIPCCLI_ID_1],g_SocketCliData_BsBufTotalAddr[path_id][ETHSOCKIPCCLI_ID_1]+g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_1]);

	memset((char*)g_SocketCliData_BsBufTotalAddr[path_id][ETHSOCKIPCCLI_ID_1], 0, g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_1]);
	g_SocketCliData2_BsAddr[path_id] = (g_SocketCliData_BsBufTotalAddr[path_id][ETHSOCKIPCCLI_ID_1] + sizeof(unsigned int )*(g_SocketCliData_BsQueueMax[path_id][ETHSOCKIPCCLI_ID_1]));
	g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_1]-= (sizeof(unsigned int )*g_SocketCliData_BsQueueMax[path_id][ETHSOCKIPCCLI_ID_1]);
	//DBG_DUMP("1EthBsFrmAddr=0x%x, EthBsFrmBufTotalSize=0x%x, end=0x%x\r\n",g_EthBsFrmAddr,g_EthBsFrmBufTotalSize,g_EthBsFrmAddr+g_EthBsFrmBufTotalSize);
	while (g_SocketCliData2_BsAddr[path_id] % 4 != 0)
	{
		g_SocketCliData2_BsAddr[path_id] += 1;
	}
	g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_1]-=4;
	ETHSOCKETCLI_PRINT(path_id,ETHSOCKIPCCLI_ID_1,"2EthBsFrmAddr=0x%x, EthBsFrmBufTotalSize=0x%x, end=0x%x\r\n",g_SocketCliData2_BsAddr[path_id],g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_1],g_SocketCliData2_BsAddr[path_id]+g_SocketCliData_BsBufTotalSize[path_id][ETHSOCKIPCCLI_ID_1]);
}

int ethsocket_cli_ipc_init(unsigned int path_id, unsigned int id, ETHSOCKETCLI_OPEN_OBJ* pObj)
{

	//set the Receive parameter address for IPC
	gRcvParamAddr[path_id][id] = (ipc_getNonCacheAddr(pObj->RcvParamAddr)) ;
	if (ETHSOCKETCLI_INVALID_PARAMADDR == gRcvParamAddr[path_id][id]) {
		ETHSOCKETCLI_ERR(path_id,id,"gRcvParamAddr = 0x%X\n", gRcvParamAddr[path_id][id]);
		return ETHSOCKETCLI_RET_ERR;
	}

	gRcvParamSize[path_id][id]= pObj->RcvParamSize;
	if (0 == gRcvParamSize[path_id][id]) {
		ETHSOCKETCLI_ERR(path_id, id,"gRcvParamSize= 0x%X\n", gRcvParamSize[path_id][id]);
		return ETHSOCKETCLI_RET_ERR;
	}

	//set the Send parameter address for IPC
	//gSndParamAddr[id] = (NvtIPC_GetNonCacheAddr(pObj->SndParamAddr)) ;
	gSndParamAddr[path_id][id] = (ipc_getCacheAddr(pObj->SndParamAddr)) ;
	if (ETHSOCKETCLI_INVALID_PARAMADDR == gSndParamAddr[path_id][id]) {
		ETHSOCKETCLI_ERR(path_id,id,"gSndParamAddr = 0x%X\n", gSndParamAddr[path_id][id]);
		return ETHSOCKETCLI_RET_ERR;
	}

	gSndParamSize[path_id][id] = pObj->SndParamSize;
	if (0 == gSndParamSize[path_id][id]) {
		ETHSOCKETCLI_ERR(path_id,id,"gSndParamSize = 0x%X\n", gSndParamSize[path_id][id]);
		return ETHSOCKETCLI_RET_ERR;
	}

	//g_SocketCliData_BsBufTotalAddr[id] = (NvtIPC_GetNonCacheAddr(pObj->BsBufAddr)) ;
	g_SocketCliData_BsBufTotalAddr[path_id][id] = (ipc_getCacheAddr(pObj->BsBufAddr)) ;
	if (ETHSOCKETCLI_INVALID_PARAMADDR == g_SocketCliData_BsBufTotalAddr[id]) {
		ETHSOCKETCLI_PRINT(path_id,id,"g_SocketCliData1_BsBufTotalAddr = 0x%X\n", g_SocketCliData_BsBufTotalAddr[path_id][id]);
	}

	g_SocketCliData_BsBufTotalSize[path_id][id] = pObj->BsBufSize;
	if (0 == g_SocketCliData_BsBufTotalSize[path_id][id]) {
		ETHSOCKETCLI_PRINT(path_id,id,"g_SocketCliData1_BsBufTotalSize = 0x%X\n", g_SocketCliData_BsBufTotalSize[path_id][id]);
	}

	//set the Send parameter address for IPC
	//g_SocketCliData_RawEncodeAddr[id] = (NvtIPC_GetNonCacheAddr(pObj->RawEncodeAddr)) ;
	g_SocketCliData_RawEncodeAddr[path_id][id] = (ipc_getCacheAddr(pObj->RawEncodeAddr)) ;
	if (ETHSOCKETCLI_INVALID_PARAMADDR == g_SocketCliData_RawEncodeAddr[path_id][id]) {
		ETHSOCKETCLI_PRINT(path_id,id,"g_SocketCliData1_RawEncodeAddr = 0x%X\n", g_SocketCliData_RawEncodeAddr[path_id][id]);
	}

	g_SocketCliData_RawEncodeSize[path_id][id] = pObj->RawEncodeSize;
	if (0 == g_SocketCliData_RawEncodeSize[path_id][id]) {
		ETHSOCKETCLI_PRINT(path_id,id,"g_SocketCliData_RawEncodeSize = 0x%X\n", g_SocketCliData_RawEncodeSize[path_id][id]);
	}

	g_SocketCliData_BsQueueMax[path_id][id]= pObj->BsQueueMax;
	if (0 == g_SocketCliData_BsQueueMax[path_id][id]) {
		ETHSOCKETCLI_PRINT(path_id,id,"g_SocketCliData1_BsQueueMax = 0x%X\n", g_SocketCliData_BsQueueMax[path_id][id]);
	}

	ETHSOCKETCLI_PRINT(path_id,id,"gRcvParamAddr=%d, gRcvParamSize=%d, gSndParamAddr=%d, gSndParamSize=%d\n", gRcvParamAddr[path_id][id],gRcvParamSize[path_id][id],gSndParamAddr[path_id][id],gSndParamSize[path_id][id]);
	ETHSOCKETCLI_PRINT(path_id,id,"g_SocketCliData1_BsBufTotalAddr=%d, g_SocketCliData1_BsBufTotalSize=%d, g_SocketCliData_RawEncodeAddr=%d, g_SocketCliData_RawEncodeSize=%d\n", g_SocketCliData_BsBufTotalAddr[path_id][id],g_SocketCliData_BsBufTotalSize[path_id][id],g_SocketCliData_RawEncodeAddr[path_id][id],g_SocketCliData_RawEncodeSize[path_id][id]);
	ETHSOCKETCLI_PRINT(path_id,id,"g_SocketCliData1_BsQueueMax=%d\n", g_SocketCliData_BsQueueMax[path_id][id]);

	if(id == ETHSOCKIPCCLI_ID_0){
		ethsocket_cli_ipc_init_data1_bs_buf(path_id);
		g_SocketCliData1_RecvAddr[path_id]=(ethsocket_cli_ipc_GetSndAddr(path_id, id) + sizeof(ETHSOCKETCLI_RECV_PARAM));
		ETHSOCKETCLI_PRINT(path_id,id, "g_SocketCliData1_RecvAddr=%d\n", g_SocketCliData1_RecvAddr[path_id]);

		ethsocket_cli_ipc_Data1_RecvResetParam(path_id);
	}else if(id ==ETHSOCKIPCCLI_ID_1){
		ethsocket_cli_ipc_init_data2_bs_buf(path_id);
		g_SocketCliData2_RecvAddr[path_id]=(ethsocket_cli_ipc_GetSndAddr(path_id, id) + sizeof(ETHSOCKETCLI_RECV_PARAM));
		ETHSOCKETCLI_PRINT(path_id,id, "g_SocketCliData2_RecvAddr=%d\n", g_SocketCliData2_RecvAddr[path_id]);

		ethsocket_cli_ipc_Data2_RecvResetParam(path_id);
	}
#if (__USE_IPC)
	NVTIPC_I32 Ret_NvtIpc;

	#if 0//def ETHSOCKETCLI_SIM
	Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok("ETHSOCKETCLIIPC eCos"));
	#else
	if (gIPC_MsgId[path_id][id] == ETHSOCKETCLI_INVALID_MSGID) {
		//Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(ETHSOCKETCLI_TOKEN_PATH));
		if (id == ETHSOCKIPCCLI_ID_0) {
			//Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(ETHSOCKETCLI_TOKEN_PATH0));
			Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(g_ProcIdTbl[path_id][id]));
		}else if (id == ETHSOCKIPCCLI_ID_1) {
			//Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(ETHSOCKETCLI_TOKEN_PATH1));
			Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(g_ProcIdTbl[path_id][id]));
		} else {
			//Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(ETHSOCKETCLI_TOKEN_PATH2));
			Ret_NvtIpc = NvtIPC_MsgGet(NvtIPC_Ftok(g_ProcIdTbl[path_id][id]));
		}

	} else {
		Ret_NvtIpc = gIPC_MsgId[path_id][id];
	}
    #endif

	if (Ret_NvtIpc < 0) {
		ETHSOCKETCLI_ERR(path_id,id, "NvtIPC_MsgGet = %d\n", Ret_NvtIpc);
		return ETHSOCKETCLI_RET_ERR;
	}

	gIPC_MsgId[path_id][id] = (NVTIPC_U32)Ret_NvtIpc;
	ETHSOCKETCLI_PRINT(path_id,id, "gIPC_MsgId = %d, Ftok =0x%x\n", gIPC_MsgId[path_id][id], NvtIPC_Ftok(g_ProcIdTbl[path_id][id]));

	#if 0//def ETHSOCKETCLI_SIM
	if (gIPC_MsgId[path_id][id] != gIPC_MsgId_eCos) {
		ETHSOCKETCLI_ERR(path_id,id, "Simulation environment error\n");
		return ETHSOCKETCLI_RET_ERR;
	}
	#endif

	//Error Handle
	ETHSOCKETCLI_PARAM_PARAM *pRcvParam = (ETHSOCKETCLI_PARAM_PARAM *)ethsocket_cli_ipc_GetRcvAddr(path_id, id);
	if ((int)pRcvParam == ETHSOCKETCLI_RET_ERR) {
		ETHSOCKETCLI_ERR(path_id,id,"pRcvParam error\n");
		return ETHSOCKETCLI_RET_ERR;
	}

	ETHSOCKETCLI_RECV_PARAM *pSndParam = (ETHSOCKETCLI_RECV_PARAM *)ethsocket_cli_ipc_GetSndAddr(path_id, id);
	if ((int)pSndParam == ETHSOCKETCLI_RET_ERR) {
		ETHSOCKETCLI_ERR(path_id,id,"pSndParam error\n");
		return ETHSOCKETCLI_RET_ERR;
	}

	cyg_flag_init(&ethsocket_cli_ipc_flag[path_id][id]);


	#if defined(__ECOS)
	/* Create a main thread for receive message */
	cyg_thread_create(  ETHSOCKETCLIIPC_THREAD_PRIORITY,//5,
						gThread[path_id][id],
						0,
						ethsocket_cli_ipc_thread_name[path_id][id],//"ethcam socket ipc",
						&ethsocket_cli_ipc_stacks[path_id][id][0],
						sizeof(ethsocket_cli_ipc_stacks[path_id][id]),
						&ethsocket_cli_ipc_thread[path_id][id],
						&ethsocket_cli_ipc_thread_object[path_id][id]
		);
	cyg_thread_resume(ethsocket_cli_ipc_thread[path_id][id]);  /* Start it */

	//cyg_mutex_init(&ethsocket_cli_ipc_mutex);

	#else
	if(pthread_create(&ethsocket_cli_ipc_thread[path_id][id], NULL , gThread[path_id][id] , NULL)){
            printf("usocket can't create usocket_thread\r\n");
	}
	#endif
#endif
	return ETHSOCKETCLI_RET_OK;
}

int ethsocket_cli_ipc_uninit(unsigned int path_id, unsigned int id)
{
#if (__USE_IPC)
	NVTIPC_I32 Ret_NvtIpc;
	#if defined(__ECOS)
	if(ethsocket_cli_ipc_thread[path_id][id]){
		cyg_thread_suspend(ethsocket_cli_ipc_thread[path_id][id]);
		cyg_thread_delete(ethsocket_cli_ipc_thread[path_id][id]);
	}
	#endif
	cyg_flag_destroy(&ethsocket_cli_ipc_flag[path_id][id]);

	#ifdef ETHSOCKETCLI_SIM
	gIPC_MsgId[path_id][id] = gIPC_MsgId_eCos;
	#endif

	Ret_NvtIpc = NvtIPC_MsgRel(gIPC_MsgId[path_id][id]);
	if (Ret_NvtIpc < 0) {
		ETHSOCKETCLI_ERR(path_id,id ,"NvtIPC_MsgRel = %d\n", Ret_NvtIpc);
		return ETHSOCKETCLI_RET_ERR;
	}
#endif
	gIPC_MsgId[path_id][id] = ETHSOCKETCLI_INVALID_MSGID;
	gRcvParamAddr[path_id][id] = ETHSOCKETCLI_INVALID_PARAMADDR;

	return ETHSOCKETCLI_RET_OK;
}

int ethsocket_cli_ipc_wait_ack(unsigned int path_id, unsigned int id)
{
#if (__USE_IPC)
	cyg_flag_wait(&ethsocket_cli_ipc_flag[path_id][id], CYG_FLG_ETHSOCKETCLIIPC_ACK, CYG_FLAG_WAITMODE_OR|CYG_FLAG_WAITMODE_CLR);
	return gRetValue[path_id][id];
#else
	return 0;
#endif
}

void ethsocket_cli_ipc_set_ack(unsigned int path_id, unsigned int id, int retValue)
{
#if (__USE_IPC)
	gRetValue[path_id][id]= retValue;
	cyg_flag_setbits(&ethsocket_cli_ipc_flag[path_id][id], CYG_FLG_ETHSOCKETCLIIPC_ACK);
#endif
}

int ethsocket_cli_ipc_exeapi(unsigned int path_id, unsigned int id, ETHSOCKETCLI_CMDID CmdId, int *pOutRet)
{
#if (__USE_IPC)
	ETHSOCKETCLI_MSG IpcMsg = {0};
	NVTIPC_I32  Ret_NvtIpc;

	if (ETHSOCKETCLI_INVALID_MSGID == gIPC_MsgId[path_id][id]) {
		ETHSOCKETCLI_ERR(path_id,id, "ETHSOCKETCLI_INVALID_MSGID\n");
		return ETHSOCKETCLI_RET_ERR;
	}

	IpcMsg.CmdId = CmdId;
	IpcMsg.Arg = 0;

	#ifdef ETHSOCKETCLI_SIM
	gIPC_MsgId[id] = gIPC_MsgId_uITRON;
	#endif

	ETHSOCKETCLI_PRINT(path_id,id, "Send CmdId = %d\n", CmdId);
	if ((Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId[path_id][id], ETHSOCKETCLI_SEND_TARGET, &IpcMsg, sizeof(IpcMsg))) < 0) {
		ETHSOCKETCLI_ERR(path_id,id, "NvtIPC_MsgSnd = %d\n", Ret_NvtIpc);
		return ETHSOCKETCLI_RET_ERR;
	}

	*pOutRet = ethsocket_cli_ipc_wait_ack(path_id, id);
#else
	EthsockCliIpc_HandleCmd(CmdId, path_id, id);
#endif
	return ETHSOCKETCLI_RET_OK;
}

//Ack the API caller with Ack CmdId and Ack Arg (if no Arg to be sent, please set Arg to 0)
int ethsocket_cli_ipc_send_ack(unsigned int path_id, unsigned int id, int AckValue)
{
#if (__USE_IPC)
	ETHSOCKETCLI_MSG IpcMsg = {0};
	NVTIPC_I32  Ret_NvtIpc;

	if (ETHSOCKETCLI_INVALID_MSGID == gIPC_MsgId[path_id][id]) {
		ETHSOCKETCLI_ERR(path_id,id, "ETHSOCKETCLI_INVALID_MSGID\n");
		return ETHSOCKETCLI_RET_ERR;
	}

	IpcMsg.CmdId = ETHSOCKETCLI_CMDID_SYSREQ_ACK;
	IpcMsg.Arg = AckValue;

	#ifdef ETHSOCKETCLI_SIM
	gIPC_MsgId[path_id][id] = gIPC_MsgId_uITRON;
	#endif

	ETHSOCKETCLI_PRINT(path_id,id, "send ack %d\n", AckValue);
	if ((Ret_NvtIpc = NvtIPC_MsgSnd(gIPC_MsgId[path_id][id], ETHSOCKETCLI_SEND_TARGET, &IpcMsg, sizeof(IpcMsg))) < 0) {
		ETHSOCKETCLI_ERR(path_id,id, "NvtIPC_MsgSnd = %d\n", Ret_NvtIpc);
		return ETHSOCKETCLI_RET_ERR;
	}
#endif
	return ETHSOCKETCLI_RET_OK;
}

int ethsocket_cli_ipc_wait_cmd(unsigned int path_id, unsigned int id, ETHSOCKETCLI_MSG *pRcvMsg)
{
#if (__USE_IPC)

	NVTIPC_I32  Ret_NvtIpc;

#ifdef ETHSOCKETCLI_SIM
	gIPC_MsgId[path_id][id] = gIPC_MsgId_eCos;
#endif

	if ((Ret_NvtIpc = NvtIPC_MsgRcv(gIPC_MsgId[path_id][id], pRcvMsg, sizeof(ETHSOCKETCLI_MSG))) < 0) {
		ETHSOCKETCLI_ERR(path_id,id, "NvtIPC_MsgRcv = %d\r\n", Ret_NvtIpc);
		return ETHSOCKETCLI_RET_ERR;
	}
#endif
	return ETHSOCKETCLI_RET_OK;
}

//==========================================================================
#define ETHSOCKETCLI_MACRO_START do {
#define ETHSOCKETCLI_MACRO_END   } while (0)

#define ETHSOCKETCLI_API_RET(path_id,id,RetValue)         \
ETHSOCKETCLI_MACRO_START                       \
	ethsocket_cli_ipc_send_ack(path_id,id,RetValue);     \
	return RetValue;                    \
ETHSOCKETCLI_MACRO_END
#define ETHSOCKETCLI_API_RET_UNINIT(path_id,id,RetValue)  \
ETHSOCKETCLI_MACRO_START                       \
	ethsocket_cli_ipc_send_ack(path_id,id,RetValue);     \
	ethsocket_cli_ipc_uninit(path_id,id);                 \
	return RetValue;                    \
ETHSOCKETCLI_MACRO_END
//-------------------------------------------------------------------------
//Implementation of ETHSOCKETECOS API
//-------------------------------------------------------------------------
static int ETHSOCKETCLIECOS_Open(unsigned int path_id, unsigned int id, ETHSOCKETCLI_OPEN_OBJ* pObj)
{
	ETHSOCKETCLI_PRINT(path_id,id, "\n");

	if (gIsOpened[path_id][id]) {
		ETHSOCKETCLI_ERR(path_id,id, "is opened\n");
		ETHSOCKETCLI_API_RET(path_id, id, ETHSOCKETCLI_RET_OPENED);
	}

	if (NULL == pObj) {
		ETHSOCKETCLI_ERR(path_id,id, "pObj = 0x%X\n", pObj);
		ETHSOCKETCLI_API_RET(path_id, id, ETHSOCKETCLI_RET_ERR);
	}

	//init the IPC communication
	if (ETHSOCKETCLI_RET_ERR == ethsocket_cli_ipc_init(path_id, id, pObj)) {
		ETHSOCKETCLI_ERR(path_id,id, "ethsocket_cli_ipc_init failed\n");
		//ETHSOCKETCLI_API_RET(path_id, id, ETHSOCKETCLI_RET_ERR);
		ETHSOCKETCLI_API_RET_UNINIT(path_id, id, ETHSOCKETCLI_RET_ERR);
	}
	gIsOpened[path_id][id] = true;
	ETHSOCKETCLI_API_RET(path_id, id, ETHSOCKETCLI_RET_OK);
}

static int ETHSOCKETCLIECOS_Close(unsigned int path_id, unsigned int id)
{
	ETHSOCKETCLI_PRINT(path_id,id, "\n");

	gIsOpened[path_id][id]   = false;

	ETHSOCKETCLI_API_RET_UNINIT(path_id, id, ETHSOCKETCLI_RET_OK);
}

#if defined(__ECOS)
void ETHSOCKETCLIECOS_CmdLine(char *szCmd)
{
	char *delim = " ";
	char *pToken;

	ETHSOCKETCLI_PRINT(3, 3, "szCmd = %s\n", szCmd);
	//diag_printf("szCmd = %s\n", szCmd);
	pToken = strtok(szCmd, delim);
	while (pToken != NULL) {
		if (!strcmp(pToken, "-open")) {
			ETHSOCKETCLI_OPEN_OBJ OpenObj;
			ETHSOCKIPCCLI_ID id = 0;
			ETHCAM_PATH_ID path_id = 0;

			//get receive memory address
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get rcv address fail\n");
				break;
			}
			OpenObj.RcvParamAddr = atoi(pToken);

			//get receive memory size
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get rcv size fail\n");
				break;
			}
			OpenObj.RcvParamSize= atoi(pToken);

			//get send memory address
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get snd address fail\n");
				break;
			}
			OpenObj.SndParamAddr = atoi(pToken);

			//get send memory size
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get snd size fail\n");
				break;
			}
			OpenObj.SndParamSize= atoi(pToken);

			//get Bs memory address
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get Bs address fail\n");
				break;
			}
			OpenObj.BsBufAddr = atoi(pToken);

			//get Bs memory size
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get Bs size fail\n");
				break;
			}
			OpenObj.BsBufSize= atoi(pToken);

			//get RawEncode memory address
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get RawEncode address fail\n");
				break;
			}
			OpenObj.RawEncodeAddr = atoi(pToken);

			//get RawEncode memory size
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get RawEncode size fail\n");
				break;
			}
			OpenObj.RawEncodeSize= atoi(pToken);

			//get BsQueueMax
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get BsQueueMax fail\n");
				break;
			}
			OpenObj.BsQueueMax= atoi(pToken);

			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get ver key fail\n");
				break;
			}
			if (atoi(pToken) != ETHSOCKETCLI_VER_KEY) {
				ETHSOCKETCLI_ERR(path_id,id, "ver key mismatch %d %d\n", ETHSOCKETCLI_VER_KEY, atoi(pToken));
				break;
			}

			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKETCLI_ERR(path_id,id, "get path_id fail\n");
				break;
			}

			path_id = atoi(pToken);

			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKETCLI_ERR(path_id,id, "get id fail\n");
				break;
			}

			id = atoi(pToken);

			//diag_printf("id =%d, RcvParamAddr=0x%x, RcvParamSize=%d, SndParamAddr=0x%x, SndParamSize=%d",id, OpenObj.RcvParamAddr,OpenObj.RcvParamSize,OpenObj.SndParamAddr ,OpenObj.SndParamSize);
			//diag_printf("path_id =%d, id=%d\r\n",path_id,id);

	            //open ETHSOCKETECOS
			ETHSOCKETCLIECOS_Open(path_id, id, &OpenObj);
			break;
		} else if(!strcmp(pToken, "-close")) {
			ETHSOCKIPCCLI_ID id = 0;
			ETHCAM_PATH_ID path_id = 0;

			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKETCLI_ERR(path_id,id, "get id fail\n");
				break;
			}
			path_id = atoi(pToken);
			id = atoi(pToken);
			ETHSOCKETCLIECOS_Close(path_id, id);

			break;
		}

		pToken = strtok(NULL, delim);
	}
}
#else
int ETHSOCKETCLIECOS_CmdLine(char *szCmd, char *szRcvAddr, char *szSndAddr)
{
	char *delim = " ";
	char *pToken;

	ETHSOCKETCLI_PRINT(3, 3, "szCmd = %s\n", szCmd);
	printf("szCmd = %s\n", szCmd);
	pToken = strtok(szCmd, delim);
	while (pToken != NULL) {
		if (!strcmp(pToken, "-open")) {
			ETHSOCKETCLI_OPEN_OBJ OpenObj;
			ETHSOCKIPCCLI_ID id = 0;
			ETHCAM_PATH_ID path_id = 0;

			//get receive memory address
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get rcv address fail\n");
				break;
			}
			OpenObj.RcvParamAddr = atoi(pToken);

			//get receive memory size
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get rcv size fail\n");
				break;
			}
			OpenObj.RcvParamSize= atoi(pToken);

			//get send memory address
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get snd address fail\n");
				break;
			}
			OpenObj.SndParamAddr = atoi(pToken);

			//get send memory size
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get snd size fail\n");
				break;
			}
			OpenObj.SndParamSize= atoi(pToken);

			//get Bs memory address
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get Bs address fail\n");
				break;
			}
			OpenObj.BsBufAddr = atoi(pToken);

			//get Bs memory size
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get Bs size fail\n");
				break;
			}
			OpenObj.BsBufSize= atoi(pToken);

			//get RawEncode memory address
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get RawEncode address fail\n");
				break;
			}
			OpenObj.RawEncodeAddr = atoi(pToken);

			//get RawEncode memory size
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get RawEncode size fail\n");
				break;
			}
			OpenObj.RawEncodeSize= atoi(pToken);

			//get BsQueueMax
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get BsQueueMax fail\n");
				break;
			}
			OpenObj.BsQueueMax= atoi(pToken);

			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get ver key fail\n");
				break;
			}
			if (atoi(pToken) != ETHSOCKETCLI_VER_KEY) {
				ETHSOCKETCLI_ERR(path_id,id, "ver key mismatch %d %d\n", ETHSOCKETCLI_VER_KEY, atoi(pToken));
				break;
			}

			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKETCLI_ERR(path_id,id, "get path_id fail\n");
				break;
			}

			path_id = atoi(pToken);

			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKETCLI_ERR(path_id,id, "get id fail\n");
				break;
			}

			id = atoi(pToken);

			//printf("id =%d, RcvParamAddr=0x%x, RcvParamSize=%d, SndParamAddr=0x%x, SndParamSize=%d",id, OpenObj.RcvParamAddr,OpenObj.RcvParamSize,OpenObj.SndParamAddr ,OpenObj.SndParamSize);
			//printf("path_id =%d, id=%d\r\n",path_id,id);

	            //open ETHSOCKETECOS
	            return ETHSOCKETCLIECOS_Open(path_id,id, &OpenObj);
		} else if(!strcmp(szCmd, "-close")) {
				ETHSOCKIPCCLI_ID id = 0;
				ETHCAM_PATH_ID path_id = 0;

				pToken = strtok(NULL, delim);
				if (NULL == pToken) {
					ETHSOCKETCLI_ERR(path_id,id, "get id fail\n");
					break;
				}
				path_id = atoi(pToken);
				id = atoi(pToken);
			return ETHSOCKETCLIECOS_Close(path_id,id);
		}else if(!strcmp(pToken, "-decerr")) {
			ETHSOCKIPCCLI_ID id = 0;
			ETHCAM_PATH_ID path_id = 0;

			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKETCLI_ERR(path_id,id, "get path_id fail\n");
				break;
			}
			path_id = atoi(pToken);

			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKETCLI_ERR(path_id,id, "get id fail\n");
				break;
			}
			id = atoi(pToken);

			printf("path_id =%d, id=%d\r\n",path_id,id);

			printf("DebugRecvSize[%d][%d]=%d\r\n",path_id,id,g_SocketCliData_DebugRecvSize[path_id][id]);

			ethsocket_cli_ipc_Data1_DumpBsInfo(path_id);
			ethsocket_cli_ipc_Data2_DumpBsInfo(path_id);
			return ETHSOCKETCLI_RET_OK;
		} else if(!strcmp(pToken, "-debug")) {
			ETHSOCKIPCCLI_ID id = 0;
			ETHCAM_PATH_ID path_id = 0;

			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKETCLI_ERR(path_id,id, "get path_id fail\n");
				break;
			}
			path_id = atoi(pToken);

			pToken = strtok(NULL, delim);
			if (NULL == pToken) {
				ETHSOCKETCLI_ERR(path_id,id, "get id fail\n");
				break;
			}
			id = atoi(pToken);

			printf("path_id =%d, id=%d\r\n",path_id,id);


			//get Debug memory address
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get Debug address fail\n");
				break;
			}
			g_SocketCliData_DebugAddr[path_id][id]= atoi(pToken);
			//g_SocketCliData_DebugAddr[path_id][id]= NvtIPC_GetCacheAddr(g_SocketCliData_DebugAddr[path_id][id]);


			//get Debug memory size
			if ((pToken = strtok(NULL, delim)) == NULL) {
				ETHSOCKETCLI_ERR(path_id,id, "get Debug size fail\n");
				break;
			}
			g_SocketCliData_DebugSize[path_id][id]= atoi(pToken);

			printf("DebugAddr[%d][%d]=0x%x, DebugSize[%d][%d]=%d\r\n",path_id,id,g_SocketCliData_DebugAddr[path_id][id],path_id,id,g_SocketCliData_DebugSize[path_id][id]);
			return ETHSOCKETCLI_RET_OK;
		}
		pToken = strtok(NULL, delim);
	}

	return ETHSOCKETCLI_RET_ERR;
}

int ETHSOCKETCLIECOS_HandleCmd(ETHSOCKETCLI_CMDID CmdId, unsigned int path_id, unsigned int id)
{
	int Ret_ExeResult = ETHSOCKETCLI_RET_ERR;
	if(CmdId == ETHSOCKETCLI_CMDID_UNINIT) {
		ETHSOCKETCLIECOS_Close(path_id,id);
		Ret_ExeResult = ETHSOCKETCLI_RET_OK;
	}else if (gCmdFpTbl[CmdId].pFunc) {
		Ret_ExeResult= gCmdFpTbl[CmdId].pFunc(path_id, id);
	}else{
		Ret_ExeResult =ETHSOCKETCLI_RET_NO_FUNC;
	}
	return Ret_ExeResult;
}
#endif
// -------------------------------------------------------------------------


#if defined(__LINUX_660)
int main(int argc, char **argv)
{
	if (argc < 4) {
		ETHSOCKETCLI_ERR(path_id,id,"ethsocket:: incorrect main argc(%d)\n", argc);
		return -1;
	}
	ETHSOCKETCLI_PRINT(path_id,id,"szCmd = %s ; szRcvAddr is %s ; szSndAddr is %s\n", argv[1], argv[2], argv[3]);

	if (ETHSOCKETCLI_RET_OK != ETHSOCKETCLIECOS_CmdLine(argv[1], argv[2], argv[3])) {
		ETHSOCKETCLI_ERR(path_id,id,"ETHSOCKETCLIECOS_CmdLinel() fail\n");
		return -1;
	}

	ethsocket_cli_ipc_receive_thread(NULL);

	return 0;
}
#endif

