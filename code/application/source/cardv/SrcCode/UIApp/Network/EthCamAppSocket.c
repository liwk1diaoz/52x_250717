#include "PrjCfg.h"

#if (defined(_NVT_ETHREARCAM_TX_) || defined(_NVT_ETHREARCAM_RX_))
//#include "NVTToolCommand.h"
//#include "UIAppWiFiCmd.h"
#include "WiFiIpc/nvtwifi.h"

#include "UsockIpc/UsockIpcAPI.h"
//#include "nvtmpp.h"
//#include "SysCfg.h"
#include "UIApp/Network/WifiAppXML.h"
//#include "UIAppPhoto.h"
//#include "ImageUnit_VdoDec.h"
#include "UIApp/Movie/UIAppMovie.h"
#include "UIAppNetwork.h"
#include "EthCamAppCmd.h"
#include "EthCamAppSocket.h"
//#include "ImageApp_MovieCommon.h"
#include "ImageApp/ImageApp_MovieMulti.h"
#include "System/SysMain.h"
#include "System/SysCommon.h"
#include "EthCam/EthCamSocket.h"
//#include "ImageUnit_Demux.h"
#include "Mode/UIModeUpdFw.h"
//#include "movieinterface_def.h"
#include "Mode/UIModeWifi.h"
#include <comm/hwclock.h>
#include "EthCamAppNetwork.h"
#include <kwrap/stdio.h>
#include "UIWnd/UIFlow.h"
#include "EthCam/ethcam_type.h"
#include "UIApp/Network/WifiAppCmd.h"
#include "vendor_type.h"

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          EthCamAppSocket
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

#define TBR_SIZE_RATIO 100//130// 1000//130//220//240

#define ETHCAM_DEBUG_SIZE 	0x200000

ETHCAM_SENDCMD_INFO sEthCamSendCmdInfo={0};

#if(defined(_NVT_ETHREARCAM_TX_))
extern int SX_TIMER_ETHCAM_DATARECVDET_ID;
//static NVTMPP_VB_POOL g_SocketData1_Tx_SendPool[1]= {NVTMPP_VB_INVALID_POOL};
static ETHCAM_ADDR_INFO g_SocketData1_Tx_SendPoolAddr[1]={0};
//static UINT32 g_SocketData1_Tx_SendAddr=0;

//static NVTMPP_VB_POOL g_SocketData1_Tx_RawEncodePool[1]= {NVTMPP_VB_INVALID_POOL};
static ETHCAM_ADDR_INFO g_SocketData1_Tx_RawEncodePoolAddr[1]={0};
static UINT32 g_SocketData1_Tx_RawEncodeAddr=0;
static ETHCAM_ADDR_INFO g_SocketData1_Tx_ThumbPoolAddr={0};
static UINT32 g_SocketData1_Tx_ThumbAddr=0;
static UINT32 g_IsSocketData1_Conn=0;

static UINT32 g_IsSocketCmdOpen=0;
static UINT32 g_IsSocketData1Open=0;
static UINT32 g_IsSocketData2Open=0;

#if 1//(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
//static NVTMPP_VB_POOL g_SocketData2_Tx_SendPool[1]= {NVTMPP_VB_INVALID_POOL};
static ETHCAM_ADDR_INFO g_SocketData2_Tx_SendPoolAddr[1]={0};
//static UINT32 g_SocketData2_Tx_SendAddr=0;
#endif
BOOL socketEthCmd_IsOpen(void)
{
	return g_IsSocketCmdOpen;
}
BOOL socketEthData1_IsConn(void)
{
	return g_IsSocketData1_Conn;
}
UINT32 socketEthData1_GetSendBufAddr(UINT32 blk_size)
{
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	CHAR pool_name[20] ={0};
	if(g_SocketData1_Tx_SendPoolAddr[0].pool_va == 0)  {

            sprintf(pool_name,"EthSocket_SendD1");

		ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, blk_size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", blk_size, ddr_id);
			return 0;
		}
		DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		g_SocketData1_Tx_SendPoolAddr[0].pool_va=(UINT32)va;
		g_SocketData1_Tx_SendPoolAddr[0].pool_pa=(UINT32)pa;
		memset(va, 0, blk_size);
	}

	if(g_SocketData1_Tx_SendPoolAddr[0].pool_va == 0)
		DBG_ERR("get buf addr err\r\n");
	return g_SocketData1_Tx_SendPoolAddr[0].pool_va;
}
void socketEthData1_DestroySendBuff(void)
{
	UINT32 ret;

	if (g_SocketData1_Tx_SendPoolAddr[0].pool_va != 0) {
		ret = hd_common_mem_free((UINT32)g_SocketData1_Tx_SendPoolAddr[0].pool_pa, (void *)g_SocketData1_Tx_SendPoolAddr[0].pool_va);
		if (ret != HD_OK) {
			DBG_ERR("release blk failed! (%d)\r\n", ret);
		}
		g_SocketData1_Tx_SendPoolAddr[0].pool_va = 0;
		g_SocketData1_Tx_SendPoolAddr[0].pool_pa = 0;
	}
}
UINT32 socketEthData1_GetRawEncodeBufAddr(UINT32 blk_size)
{
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	CHAR pool_name[20] ={0};
	if(g_SocketData1_Tx_RawEncodePoolAddr[0].pool_va == 0)  {

            sprintf(pool_name,"socketCli_RawEncode");

		ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, blk_size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", blk_size, ddr_id);
			return 0;
		}
		DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		g_SocketData1_Tx_RawEncodePoolAddr[0].pool_va=(UINT32)va;
		g_SocketData1_Tx_RawEncodePoolAddr[0].pool_pa=(UINT32)pa;
		memset(va, 0, blk_size);
	}

	if(g_SocketData1_Tx_RawEncodePoolAddr[0].pool_va == 0)
		DBG_ERR("get buf addr err\r\n");
	return g_SocketData1_Tx_RawEncodePoolAddr[0].pool_va;
}
void socketEthData1_DestroyRawEncodeBuff(void)
{
	UINT32 ret;

	if (g_SocketData1_Tx_RawEncodePoolAddr[0].pool_va != 0) {
		ret = hd_common_mem_free((UINT32)g_SocketData1_Tx_RawEncodePoolAddr[0].pool_pa, (void *)g_SocketData1_Tx_RawEncodePoolAddr[0].pool_va);
		if (ret != HD_OK) {
			DBG_ERR("release blk failed! (%d)\r\n", ret);
		}
		g_SocketData1_Tx_RawEncodePoolAddr[0].pool_va = 0;
		g_SocketData1_Tx_RawEncodePoolAddr[0].pool_pa = 0;
	}
}
UINT32 socketEthData1_GetThumbBufAddr(UINT32 blk_size)
{
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	CHAR pool_name[20] ={0};
	if(g_SocketData1_Tx_ThumbPoolAddr.pool_va == 0)  {

            sprintf(pool_name,"socketCli_Thumb");

		ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, blk_size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", blk_size, ddr_id);
			return 0;
		}
		DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		g_SocketData1_Tx_ThumbPoolAddr.pool_va=(UINT32)va;
		g_SocketData1_Tx_ThumbPoolAddr.pool_pa=(UINT32)pa;
		memset(va, 0, blk_size);
	}

	if(g_SocketData1_Tx_ThumbPoolAddr.pool_va == 0)
		DBG_ERR("get buf addr err\r\n");
	return g_SocketData1_Tx_ThumbPoolAddr.pool_va;
}
void socketEthData1_DestroyThumbBuff(void)
{
	UINT32 ret;

	if (g_SocketData1_Tx_ThumbPoolAddr.pool_va != 0) {
		ret = hd_common_mem_free((UINT32)g_SocketData1_Tx_ThumbPoolAddr.pool_pa, (void *)g_SocketData1_Tx_ThumbPoolAddr.pool_va);
		if (ret != HD_OK) {
			DBG_ERR("release blk failed! (%d)\r\n", ret);
		}
		g_SocketData1_Tx_ThumbPoolAddr.pool_va = 0;
		g_SocketData1_Tx_ThumbPoolAddr.pool_pa = 0;
	}
}
UINT32 socketEthData2_GetSendBufAddr(UINT32 blk_size)
{
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	CHAR pool_name[20] ={0};
	if(g_SocketData2_Tx_SendPoolAddr[0].pool_va == 0)  {

            sprintf(pool_name,"EthSocket_SendD2");

		ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, blk_size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", blk_size, ddr_id);
			return 0;
		}
		DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		g_SocketData2_Tx_SendPoolAddr[0].pool_va=(UINT32)va;
		g_SocketData2_Tx_SendPoolAddr[0].pool_pa=(UINT32)pa;
		memset(va, 0, blk_size);
	}

	if(g_SocketData2_Tx_SendPoolAddr[0].pool_va == 0)
		DBG_ERR("get buf addr err\r\n");
	return g_SocketData2_Tx_SendPoolAddr[0].pool_va;
}
void socketEthData2_DestroySendBuff(void)
{
	UINT32 ret;

	if (g_SocketData2_Tx_SendPoolAddr[0].pool_va != 0) {
		ret = hd_common_mem_free((UINT32)g_SocketData2_Tx_SendPoolAddr[0].pool_pa, (void *)g_SocketData2_Tx_SendPoolAddr[0].pool_va);
		if (ret != HD_OK) {
			DBG_ERR("release blk failed! (%d)\r\n", ret);
		}
		g_SocketData2_Tx_SendPoolAddr[0].pool_va = 0;
		g_SocketData2_Tx_SendPoolAddr[0].pool_pa = 0;
	}
}
void socketEthData_DestroyAllBuff(void)
{
	socketEthData1_DestroySendBuff();
	socketEthData2_DestroySendBuff();
	socketEthData1_DestroyRawEncodeBuff();
	socketEthData1_DestroyThumbBuff();
}
void socketEthData1_RecvCB(char* addr, int size)
{
}
void socketEthData2_RecvCB(char* addr, int size)
{
}
void socketEthData1_NotifyCB(int status, int parm)
{
    if(status == CYG_ETHSOCKET_STATUS_CLIENT_CONNECT){
        DBG_DUMP("socketEthData1_NotifyCB Connect OK\r\n");
        g_IsSocketData1_Conn=1;
    }
    else if(status == CYG_ETHSOCKET_STATUS_CLIENT_REQUEST){
    }
    else if(status == CYG_ETHSOCKET_STATUS_CLIENT_DISCONNECT){
        DBG_DUMP("socketEthData1_NotifyCB DISConnect !!\r\n");
        g_IsSocketData1_Conn=0;
        g_IsSocketData2Open=0;
        SxTimer_SetFuncActive(SX_TIMER_ETHCAM_DATARECVDET_ID, FALSE);
    }
    else{
        DBG_ERR("^GUnknown status = %d, parm = %d\r\n", status, parm);
    }
}
void socketEthData2_NotifyCB(int status, int parm)
{
    if(status == CYG_ETHSOCKET_STATUS_CLIENT_CONNECT){
        DBG_DUMP("socketEthData2_NotifyCB Connect OK\r\n");
    }
    else if(status == CYG_ETHSOCKET_STATUS_CLIENT_REQUEST){
    }
    else if(status == CYG_ETHSOCKET_STATUS_CLIENT_DISCONNECT){
        DBG_DUMP("socketEthData2_NotifyCB DISConnect !!\r\n");
		g_IsSocketData2Open=0;
    }
    else{
        DBG_ERR("^GUnknown status = %d, parm = %d\r\n", status, parm);
    }
}
#define POOL_CNT_ETHSOCK_IPC       4
#define POOL_SIZE_ETHSOCK_IPC      ALIGN_CEIL_64((2*1024 +8))
static UINT8 g_sharedMemAddr[POOL_CNT_ETHSOCK_IPC*POOL_SIZE_ETHSOCK_IPC];

void socketEthData_Open(ETHSOCKIPC_ID id)
{
	//EthCamCmdTsk_Open();
	ETHSOCKIPC_OPEN EthsockIpcOpen[2] = {0};
	if(id< ETHSOCKIPC_ID_0 || id>ETHSOCKIPC_ID_1){
		DBG_ERR("id out of range ,id=%d\r\n", id);
	}
	//DBG_DUMP("socketEthData_Open ,id=%d\r\n",id);
	if(id== ETHSOCKIPC_ID_0 && g_IsSocketData1Open==0){
	    	EthCamSocket_SetDataRecvCB(ETHSOCKIPC_ID_0, (UINT32)&socketEthData1_RecvCB);
	    	EthCamSocket_SetDataNotifyCB(ETHSOCKIPC_ID_0, (UINT32)&socketEthData1_NotifyCB);
		UINT32 JpgCompRatio = 10; // JPEG compression ratio
		UINT32 MaxRawEncodeSize=MovieExe_GetWidth(_CFG_REC_ID_1)*MovieExe_GetHeight(_CFG_REC_ID_1)*3/(2*JpgCompRatio);
		MaxRawEncodeSize*= 13/10;
		DBG_IND("MaxRawEncodeSize=%d\r\n",MaxRawEncodeSize);
		g_SocketData1_Tx_RawEncodeAddr=socketEthData1_GetRawEncodeBufAddr(MaxRawEncodeSize);
		ImageApp_MovieMulti_EthCamTxRecId1_SetRawEncodeBuff(g_SocketData1_Tx_RawEncodeAddr, MaxRawEncodeSize);

		UINT32 MaxThumbSize=640*480*3/(2*JpgCompRatio);
		MaxThumbSize*= 15/10;
		g_SocketData1_Tx_ThumbAddr=socketEthData1_GetThumbBufAddr(MaxThumbSize);
		ImageApp_MovieMulti_EthCamTxRecId1_SetThumbBuff(g_SocketData1_Tx_ThumbAddr, MaxThumbSize);

		// open
		EthsockIpcOpen[id].sharedSendMemAddr=socketEthData1_GetSendBufAddr((MovieExe_GetTBR(_CFG_REC_ID_1)*100/TBR_SIZE_RATIO));
		EthsockIpcOpen[id].sharedSendMemSize=(MovieExe_GetTBR(_CFG_REC_ID_1)*100/TBR_SIZE_RATIO);
		//EthsockIpcOpen[id].sharedRecvMemAddr=OS_GetMempoolAddr(POOL_ID_ETHSOCK_IPC) ;
		EthsockIpcOpen[id].sharedRecvMemAddr=(UINT32)&g_sharedMemAddr[0]  ;

		EthsockIpcOpen[id].sharedRecvMemSize=POOL_SIZE_ETHSOCK_IPC;

		DBG_IND("D1 SendMemAddr=0x%x, RecvMemAddr=0x%x\r\n",EthsockIpcOpen[id].sharedSendMemAddr,EthsockIpcOpen[id].sharedRecvMemAddr);
		EthCamSocket_Open(id, &EthsockIpcOpen[id]);
		g_IsSocketData1Open=1;
	}else if(id== ETHSOCKIPC_ID_1 && g_IsSocketData2Open==0){
	    	EthCamSocket_SetDataRecvCB(ETHSOCKIPC_ID_1, (UINT32)&socketEthData2_RecvCB);
	    	EthCamSocket_SetDataNotifyCB(ETHSOCKIPC_ID_1, (UINT32)&socketEthData2_NotifyCB);

		// open
		EthsockIpcOpen[id].sharedSendMemAddr=socketEthData2_GetSendBufAddr((MovieExe_GetTBR(_CFG_CLONE_ID_1)*100/TBR_SIZE_RATIO));
		EthsockIpcOpen[id].sharedSendMemSize=(MovieExe_GetTBR(_CFG_CLONE_ID_1)*100/TBR_SIZE_RATIO);

		DBG_DUMP("UI_GetData(FL_MOVIE_SIZE)=%d, TBR=%d, sharedSendMemSize=%d\r\n", UI_GetData(FL_MOVIE_SIZE), MovieExe_GetTBR(_CFG_CLONE_ID_1), EthsockIpcOpen[id].sharedSendMemSize);
		//fflush(stdout);

		//EthsockIpcOpen[id].sharedRecvMemAddr=OS_GetMempoolAddr(POOL_ID_ETHSOCK_IPC)+POOL_SIZE_ETHSOCK_IPC;
		EthsockIpcOpen[id].sharedRecvMemAddr=(UINT32)&g_sharedMemAddr[POOL_SIZE_ETHSOCK_IPC]  ;
		EthsockIpcOpen[id].sharedRecvMemSize=POOL_SIZE_ETHSOCK_IPC;
		DBG_DUMP("sharedRecvMemSize=%d\r\n", EthsockIpcOpen[id].sharedRecvMemSize);
		//fflush(stdout);
		DBG_IND("D2 SendMemAddr=0x%x, RecvMemAddr=0x%x\r\n",EthsockIpcOpen[id].sharedSendMemAddr,EthsockIpcOpen[id].sharedRecvMemAddr);
		EthCamSocket_Open(id, &EthsockIpcOpen[id]);
		g_IsSocketData2Open=1;
	}
}
void socketEthCmd_Open(void)
{
	ETHSOCKIPC_OPEN EthsockIpcOpen = {0};
	EthCamCmdTsk_Open();
	if(g_IsSocketCmdOpen==0){
	    	EthCamSocket_SetCmdRecvCB((UINT32)&socketEthCmd_RecvCB);
	    	EthCamSocket_SetCmdNotifyCB((UINT32)&socketEthCmd_NotifyCB);

		//EthsockIpcOpen.sharedSendMemAddr=OS_GetMempoolAddr(POOL_ID_ETHSOCK_IPC)+2*POOL_SIZE_ETHSOCK_IPC;
		EthsockIpcOpen.sharedSendMemAddr=(UINT32)&g_sharedMemAddr[2*POOL_SIZE_ETHSOCK_IPC]  ;
		EthsockIpcOpen.sharedSendMemSize=POOL_SIZE_ETHSOCK_IPC;

		//EthsockIpcOpen.sharedRecvMemAddr=OS_GetMempoolAddr(POOL_ID_ETHSOCK_IPC)+3*POOL_SIZE_ETHSOCK_IPC;
		EthsockIpcOpen.sharedRecvMemAddr=(UINT32)&g_sharedMemAddr[3*POOL_SIZE_ETHSOCK_IPC]  ;
		EthsockIpcOpen.sharedRecvMemSize=POOL_SIZE_ETHSOCK_IPC;

		EthCamSocket_Open(ETHSOCKIPC_ID_2, &EthsockIpcOpen);
		g_IsSocketCmdOpen=1;
	}
}
void socketEth_Close(void)
{
	g_IsSocketCmdOpen=0;
	g_IsSocketData1Open=0;
	g_IsSocketData2Open=0;
	EthCamSocket_Close();
}

#endif
#if(defined(_NVT_ETHREARCAM_RX_))
#include "EthCam/ethsocket_cli_ipc.h"

#define POOL_CNT_ETHSOCKCLI_IPC       4*ETH_REARCAM_CAPS_COUNT
#define POOL_SIZE_ETHSOCKCLI_IPC 	 	(48*1024 + 8)//(2*1024+8)
static UINT8 g_sharedMemAddr[POOL_SIZE_ETHSOCKCLI_IPC*POOL_CNT_ETHSOCKCLI_IPC];

extern UINT32 EthCamHB1[ETHCAM_PATH_ID_MAX], EthCamHB2;
extern MOVIE_RECODE_FILE_OPTION gMovie_Rec_Option;
extern int SX_TIMER_ETHCAM_ETHERNETLINKDET_LINKDET_ID;

static UINT32 g_SocketCliData1_RecvAddr[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData1_BsBufTotalAddr[ETHCAM_PATH_ID_MAX]={0}; //tbl Buf+BsFrmBuf
static UINT32 g_SocketCliData1_BsBufTotalSize[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData1_BsQueueMax[ETHCAM_PATH_ID_MAX]={0};

UINT32 g_SocketCliData1_RawEncodeAddr[ETHCAM_PATH_ID_MAX]={0};

static UINT32 g_SocketCliData1_BsFrameCnt[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData1_RecBsFrameCnt[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData1_eth_i_cnt[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData1_LongCntTxRxOffset[ETHCAM_PATH_ID_MAX]={0};

//static NVTMPP_VB_POOL g_SocketCliData1_RecvPool[ETHCAM_PATH_ID_MAX][1]= {NVTMPP_VB_INVALID_POOL, NVTMPP_VB_INVALID_POOL};
//static ETHCAM_ADDR_INFO g_SocketCliData1_RecvPool[ETHCAM_PATH_ID_MAX][1]= {0};
static ETHCAM_ADDR_INFO g_SocketCliData1_RecvPoolAddr[ETHCAM_PATH_ID_MAX][1]={0};
//static NVTMPP_VB_POOL g_SocketCliData1_RawEncodePool[ETHCAM_PATH_ID_MAX][1]= {NVTMPP_VB_INVALID_POOL, NVTMPP_VB_INVALID_POOL};
//static ETHCAM_ADDR_INFO g_SocketCliData1_RawEncodePool[ETHCAM_PATH_ID_MAX][1]= {0};
static ETHCAM_ADDR_INFO g_SocketCliData1_RawEncodePoolAddr[ETHCAM_PATH_ID_MAX][1]={0};
//NVTMPP_VB_POOL g_SocketCliData1_BsPool[ETHCAM_PATH_ID_MAX][1]= {NVTMPP_VB_INVALID_POOL, NVTMPP_VB_INVALID_POOL};
//ETHCAM_ADDR_INFO g_SocketCliData1_BsPool[ETHCAM_PATH_ID_MAX][1]= {0};
static ETHCAM_ADDR_INFO g_SocketCliData1_BsPoolAddr[ETHCAM_PATH_ID_MAX][1]={0};

static UINT32 g_bsocketCliEthData1_AllowPull[ETHCAM_PATH_ID_MAX]={1, 1};


#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
static UINT32 g_SocketCliData2_RecvAddr[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData2_BsBufTotalAddr[ETHCAM_PATH_ID_MAX]={0}; //tbl Buf+BsFrmBuf
static UINT32 g_SocketCliData2_BsBufTotalSize[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData2_BsQueueMax[ETHCAM_PATH_ID_MAX]={0};

UINT32 g_SocketCliData2_RawEncodeAddr[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData2_BsFrameCnt[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData2_eth_i_cnt[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData2_LongCntTxRxOffset[ETHCAM_PATH_ID_MAX]={0};

static ETHCAM_ADDR_INFO g_SocketCliData2_RecvPoolAddr[ETHCAM_PATH_ID_MAX][1]={0};
static ETHCAM_ADDR_INFO g_SocketCliData2_BsPoolAddr[ETHCAM_PATH_ID_MAX][1]={0};
#endif

#if (ETHCAM_DECODE_ERR_DBG== ENABLE)
static UINT32 g_SocketCliData_DebugAddr[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM]={0};
#endif
static ETHCAM_ADDR_INFO g_SocketCliData_DebugPoolAddr[ETHCAM_PATH_ID_MAX][ETHSOCKIPCCLI_MAX_NUM]={0};


static UINT32 g_IsSocketCliData1_RecvData[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_IsSocketCliData1_Conn[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_IsSocketCliCmdConn[ETHCAM_PATH_ID_MAX]={0};

static UINT32 g_IsSocketCliData2_RecvData[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_IsSocketCliData2_Conn[ETHCAM_PATH_ID_MAX]={0};
UINT32  socketCliEthData1_GetBsFrameCnt(ETHCAM_PATH_ID path_id)
{
	return g_SocketCliData1_BsFrameCnt[path_id];
}
void  socketCliEthData1_SetRecv(ETHCAM_PATH_ID path_id, UINT32 bRecvData)
{
	g_IsSocketCliData1_RecvData[path_id]=bRecvData;
}
UINT32  socketCliEthData1_IsRecv(ETHCAM_PATH_ID path_id)
{
	return g_IsSocketCliData1_RecvData[path_id];
}
void  socketCliEthData2_SetRecv(ETHCAM_PATH_ID path_id, UINT32 bRecvData)
{
	g_IsSocketCliData2_RecvData[path_id]=bRecvData;
}
UINT32  socketCliEthData2_IsRecv(ETHCAM_PATH_ID path_id)
{
	return g_IsSocketCliData2_RecvData[path_id];
}

void  socketCliEthData1_SetAllowPull(ETHCAM_PATH_ID path_id, UINT32 bAllowPull)
{
	g_bsocketCliEthData1_AllowPull[path_id]=bAllowPull;
}
UINT32  socketCliEthData1_IsAllowPull(ETHCAM_PATH_ID path_id)
{
	return g_bsocketCliEthData1_AllowPull[path_id];
}

UINT32 socketCliEthData1_GetRecvBufAddr(ETHCAM_PATH_ID path_id, UINT32 blk_size)
{
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	CHAR pool_name[20] ={0};
	if(g_SocketCliData1_RecvPoolAddr[path_id][0].pool_va == 0)  {

            sprintf(pool_name,"socketCli_RecvD1_%d",path_id);

		ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, blk_size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", blk_size, ddr_id);
			return 0;
		}
		DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		g_SocketCliData1_RecvPoolAddr[path_id][0].pool_va=(UINT32)va;
		g_SocketCliData1_RecvPoolAddr[path_id][0].pool_pa=(UINT32)pa;
		memset(va, 0, blk_size);
	}

	if(g_SocketCliData1_RecvPoolAddr[path_id][0].pool_va == 0)
		DBG_ERR("get buf addr err\r\n");
	return g_SocketCliData1_RecvPoolAddr[path_id][0].pool_va;

}
void socketCliEthData1_DestroyRecvBuff(ETHCAM_PATH_ID path_id)
{

	UINT32 i, ret;
	for (i=0;i<1;i++) {

		if (g_SocketCliData1_RecvPoolAddr[path_id][i].pool_va != 0) {
			ret = hd_common_mem_free((UINT32)g_SocketCliData1_RecvPoolAddr[path_id][i].pool_pa, (void *)g_SocketCliData1_RecvPoolAddr[path_id][i].pool_va);
			if (ret != HD_OK) {
				DBG_ERR("FileIn release blk failed! (%d)\r\n", ret);
				break;
			}
			g_SocketCliData1_RecvPoolAddr[path_id][i].pool_va = 0;
			g_SocketCliData1_RecvPoolAddr[path_id][i].pool_pa = 0;
		}
	}
}
UINT32 socketCliEthData1_GetRawEncodeBufAddr(ETHCAM_PATH_ID path_id, UINT32 blk_size)
{

	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
        CHAR pool_name[30] ={0};
	if(g_SocketCliData1_RawEncodePoolAddr[path_id][0].pool_va == 0)  {

            sprintf(pool_name,"socketCli_RawEncode_%d",path_id);

		ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, blk_size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", blk_size, ddr_id);
			return 0;
		}
		DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		g_SocketCliData1_RawEncodePoolAddr[path_id][0].pool_va=(UINT32)va;
		g_SocketCliData1_RawEncodePoolAddr[path_id][0].pool_pa=(UINT32)pa;
		memset(va, 0, blk_size);
	}

	if(g_SocketCliData1_RawEncodePoolAddr[path_id][0].pool_va == 0)
		DBG_ERR("get buf addr err\r\n");
	return g_SocketCliData1_RawEncodePoolAddr[path_id][0].pool_va;
}
void socketCliEthData1_DestroyRawEncodeBuff(ETHCAM_PATH_ID path_id)
{
	UINT32 i, ret;
	for (i=0;i<1;i++) {

		if (g_SocketCliData1_RawEncodePoolAddr[path_id][i].pool_va != 0) {
			ret = hd_common_mem_free((UINT32)g_SocketCliData1_RawEncodePoolAddr[path_id][i].pool_pa, (void *)g_SocketCliData1_RawEncodePoolAddr[path_id][i].pool_va);
			if (ret != HD_OK) {
				DBG_ERR("FileIn release blk failed! (%d)\r\n", ret);
				break;
			}
			g_SocketCliData1_RawEncodePoolAddr[path_id][i].pool_va = 0;
			g_SocketCliData1_RawEncodePoolAddr[path_id][i].pool_pa = 0;
		}
	}
}
UINT32 socketCliEthData1_GetBsBufAddr(ETHCAM_PATH_ID path_id, UINT32 blk_size)
{
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	CHAR pool_name[20] ={0};
	if(g_SocketCliData1_BsPoolAddr[path_id][0].pool_va == 0)  {

            sprintf(pool_name,"socketCli_BsD1_%d",path_id);

		ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, blk_size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", blk_size, ddr_id);
			return 0;
		}
		DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		g_SocketCliData1_BsPoolAddr[path_id][0].pool_va=(UINT32)va;
		g_SocketCliData1_BsPoolAddr[path_id][0].pool_pa=(UINT32)pa;
		memset(va, 0, blk_size);
	}

	if(g_SocketCliData1_BsPoolAddr[path_id][0].pool_va == 0)
		DBG_ERR("get buf addr err\r\n");
	return g_SocketCliData1_BsPoolAddr[path_id][0].pool_va;
}
INT32 socketCliEthData1_GetBsBufInfo(ETHCAM_PATH_ID path_id, UINT32* Addr, UINT32* Size)
{
	if(g_SocketCliData1_BsBufTotalSize[path_id]==0){
		DBG_ERR("buffer size=0!!\r\n");
		return 0;
	}
	if(g_SocketCliData1_BsPoolAddr[path_id][0].pool_pa==0){
		DBG_ERR("pa addr=0!!\r\n");
		return 0;
	}
	*Addr=g_SocketCliData1_BsPoolAddr[path_id][0].pool_pa;
	*Size=g_SocketCliData1_BsBufTotalSize[path_id];
	return 1;
}
void socketCliEthData1_DestroyBsBuff(ETHCAM_PATH_ID path_id)
{
	UINT32 i, ret;
	for (i=0;i<1;i++) {

		if (g_SocketCliData1_BsPoolAddr[path_id][i].pool_va != 0) {
			ret = hd_common_mem_free((UINT32)g_SocketCliData1_BsPoolAddr[path_id][i].pool_pa, (void *)g_SocketCliData1_BsPoolAddr[path_id][i].pool_va);
			if (ret != HD_OK) {
				DBG_ERR("FileIn release blk failed! (%d)\r\n", ret);
				break;
			}
			g_SocketCliData1_BsPoolAddr[path_id][i].pool_va = 0;
			g_SocketCliData1_BsPoolAddr[path_id][i].pool_pa = 0;
		}
	}
}
INT32 socketCliEthData1_ConfigRecvBuf(ETHCAM_PATH_ID path_id)
{
#if 0
	if(sEthCamTxDecInfo.Tbr !=0 && sEthCamTxDecInfo.bStarupOK==1){
		g_SocketCliData1_RecvAddr=socketCliEthData1_GetRecvBufAddr((sEthCamTxDecInfo.Tbr*100/285));
	}else{
		g_SocketCliData1_RecvAddr=socketCliEthData1_GetRecvBufAddr(MAX_I_FRAME_SZIE);
	}
#endif
	UINT32 JpgCompRatio = 10; // JPEG compression ratio
	UINT32 MaxRawEncodeSize = sEthCamTxRecInfo[path_id].width * sEthCamTxRecInfo[path_id].height * 3 / (2 * JpgCompRatio);
	//MaxRawEncodeSize*= 13/10;
	MaxRawEncodeSize= (MaxRawEncodeSize*13)/10;
	DBG_IND("MaxRawEncodeSize=%d\r\n",MaxRawEncodeSize);
	if((g_SocketCliData1_RawEncodeAddr[path_id]=socketCliEthData1_GetRawEncodeBufAddr(path_id, MaxRawEncodeSize))==NVTEVT_CONSUME){
		return -1;
	}
	//TBR* (EMR RollbackSec + 3) + MapTbl= FPS* (EMR RollbackSec + 3)
	g_SocketCliData1_BsBufTotalSize[path_id] = (MovieExe_GetEthcamEncBufSec(path_id) + MovieExe_GetEmrRollbackSec()) * sEthCamTxRecInfo[path_id].tbr + sizeof(UINT32) * (MovieExe_GetEthcamEncBufSec(path_id) + MovieExe_GetEmrRollbackSec()) * sEthCamTxRecInfo[path_id].vfr*3;
	if (g_SocketCliData1_BsBufTotalSize[path_id]==0) {
		DBG_ERR("EthBsFrmBufTotalSize[%d] =0 !!\r\n",path_id);
	}
	if((g_SocketCliData1_BsBufTotalAddr[path_id]=socketCliEthData1_GetBsBufAddr(path_id, g_SocketCliData1_BsBufTotalSize[path_id]))==NVTEVT_CONSUME){
		DBG_ERR("BsBuf fail, want buf size=%d\r\n",g_SocketCliData1_BsBufTotalSize[path_id]);
		return -1;
	}
	DBG_IND("g_SocketCliData1_BsBufTotalAddr[%d]=0x%x, g_SocketCliData1_BsBufTotalSize[%d]=%d\r\n",path_id,g_SocketCliData1_BsBufTotalAddr[path_id],path_id,g_SocketCliData1_BsBufTotalSize[path_id]);

#if 0
	g_SocketCliData1_BsBufMapTbl = (UINT32*)g_SocketCliData1_BsBufTotalAddr;
	//DBG_DUMP("g_EthBsFrmBufMapTbl=0x%x, g_EthBsFrmTotalAddr=0x%x, end=0x%x\r\n",g_EthBsFrmBufMapTbl,g_EthBsFrmTotalAddr,g_EthBsFrmTotalAddr+g_EthBsFrmBufTotalSize);

	memset((UINT8*)g_SocketCliData1_BsBufTotalAddr, 0, g_SocketCliData1_BsBufTotalSize);
	g_SocketCliData1_BsAddr = (g_SocketCliData1_BsBufTotalAddr + sizeof(UINT32)*(3+ MovieExe_GetEmrRollbackSec())* MovieExe_GetFps(_CFG_REC_ID_1));
	g_SocketCliData1_BsBufTotalSize-= (sizeof(UINT32)*(3+ MovieExe_GetEmrRollbackSec())* MovieExe_GetFps(_CFG_REC_ID_1));
	//DBG_DUMP("1EthBsFrmAddr=0x%x, EthBsFrmBufTotalSize=0x%x, end=0x%x\r\n",g_EthBsFrmAddr,g_EthBsFrmBufTotalSize,g_EthBsFrmAddr+g_EthBsFrmBufTotalSize);
	while (g_SocketCliData1_BsAddr % 4 != 0)
	{
		g_SocketCliData1_BsAddr += 1;
	}
	g_SocketCliData1_BsBufTotalSize-=4;
	//DBG_DUMP("2EthBsFrmAddr=0x%x, EthBsFrmBufTotalSize=0x%x, end=0x%x\r\n",g_EthBsFrmAddr,g_EthBsFrmBufTotalSize,g_EthBsFrmAddr+g_EthBsFrmBufTotalSize);
#endif
	ETHCAM_SOCKET_BUF_OBJ BufObj={0};
	BufObj.ParamAddr=g_SocketCliData1_BsBufTotalAddr[path_id];
	BufObj.ParamSize=g_SocketCliData1_BsBufTotalSize[path_id];
	EthCamSocketCli_DataSetBsBuff(path_id, ETHSOCKIPCCLI_ID_0, &BufObj);

	BufObj.ParamAddr=g_SocketCliData1_RawEncodeAddr[path_id];
	BufObj.ParamSize=MaxRawEncodeSize;
	EthCamSocketCli_DataSetRawEncodeBuff(path_id, ETHSOCKIPCCLI_ID_0, &BufObj);
	g_SocketCliData1_BsQueueMax[path_id]=((MovieExe_GetEthcamEncBufSec(path_id)+ MovieExe_GetEmrRollbackSec())* sEthCamTxRecInfo[path_id].vfr*3);
	EthCamSocketCli_DataSetBsQueueMax(path_id, ETHSOCKIPCCLI_ID_0, g_SocketCliData1_BsQueueMax[path_id]);
	return 0;
}


UINT32 socketCliEthData2_GetRecvBufAddr(ETHCAM_PATH_ID path_id, UINT32 blk_size)
{
#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)

	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	CHAR pool_name[20] ={0};
	if(g_SocketCliData2_RecvPoolAddr[path_id][0].pool_va == 0)  {

            sprintf(pool_name,"socketCli_RecvD2_%d",path_id);

		ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, blk_size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", blk_size, ddr_id);
			return 0;
		}
		DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		g_SocketCliData2_RecvPoolAddr[path_id][0].pool_va=(UINT32)va;
		g_SocketCliData2_RecvPoolAddr[path_id][0].pool_pa=(UINT32)pa;
		memset(va, 0, blk_size);
	}

	if(g_SocketCliData2_RecvPoolAddr[path_id][0].pool_va == 0)
		DBG_ERR("get buf addr err\r\n");
	return g_SocketCliData2_RecvPoolAddr[path_id][0].pool_va;
#else
	return 0;
#endif
}
void socketCliEthData2_DestroyRecvBuff(ETHCAM_PATH_ID path_id)
{
#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	UINT32 i, ret;
	for (i=0;i<1;i++) {

		if (g_SocketCliData2_RecvPoolAddr[path_id][i].pool_va != 0) {
			ret = hd_common_mem_free((UINT32)g_SocketCliData2_RecvPoolAddr[path_id][i].pool_pa, (void *)g_SocketCliData2_RecvPoolAddr[path_id][i].pool_va);
			if (ret != HD_OK) {
				DBG_ERR("FileIn release blk failed! (%d)\r\n", ret);
				break;
			}
			g_SocketCliData2_RecvPoolAddr[path_id][i].pool_va = 0;
			g_SocketCliData2_RecvPoolAddr[path_id][i].pool_pa = 0;
		}
	}
#endif
}

UINT32 socketCliEthData2_GetBsBufAddr(ETHCAM_PATH_ID path_id, UINT32 blk_size)
{
#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	CHAR pool_name[20] ={0};
	if(g_SocketCliData2_BsPoolAddr[path_id][0].pool_va == 0)  {

            sprintf(pool_name,"socketCli_BsD2_%d",path_id);

		ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, blk_size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", blk_size, ddr_id);
			return 0;
		}
		DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		g_SocketCliData2_BsPoolAddr[path_id][0].pool_va=(UINT32)va;
		g_SocketCliData2_BsPoolAddr[path_id][0].pool_pa=(UINT32)pa;
		memset(va, 0, blk_size);
	}

	if(g_SocketCliData2_BsPoolAddr[path_id][0].pool_va == 0)
		DBG_ERR("get buf addr err\r\n");
	return g_SocketCliData2_BsPoolAddr[path_id][0].pool_va;
#else
	return 0;
#endif
}
void socketCliEthData2_DestroyBsBuff(ETHCAM_PATH_ID path_id)
{
#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	UINT32 i, ret;
	for (i=0;i<1;i++) {

		if (g_SocketCliData2_BsPoolAddr[path_id][i].pool_va != 0) {
			ret = hd_common_mem_free((UINT32)g_SocketCliData2_BsPoolAddr[path_id][i].pool_pa, (void *)g_SocketCliData2_BsPoolAddr[path_id][i].pool_va);
			if (ret != HD_OK) {
				DBG_ERR("FileIn release blk failed! (%d)\r\n", ret);
				break;
			}
			g_SocketCliData2_BsPoolAddr[path_id][i].pool_va = 0;
			g_SocketCliData2_BsPoolAddr[path_id][i].pool_pa = 0;
		}
	}
#endif
}
INT32 socketCliEthData2_ConfigRecvBuf(ETHCAM_PATH_ID path_id)
{
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	if(sEthCamTxDecInfo[path_id].Tbr !=0 && sEthCamTxDecInfo[path_id].bStarupOK==1){
		if((g_SocketCliData2_RecvAddr[path_id]=socketCliEthData2_GetRecvBufAddr(path_id, (sEthCamTxDecInfo[path_id].Tbr*100/TBR_SIZE_RATIO)))==NVTEVT_CONSUME){
			return -1;
		}
	}else{
		if((g_SocketCliData2_RecvAddr[path_id]=socketCliEthData2_GetRecvBufAddr(path_id, MAX_I_FRAME_SZIE))==NVTEVT_CONSUME){
			return -1;
		}
	}
	//TBR* (EMR RollbackSec + 3) + MapTbl= FPS* (EMR RollbackSec + 3)
	//g_SocketCliData2_BsBufTotalSize[path_id]  = (MovieExe_GetEthcamEncBufSec(path_id)+ MovieExe_GetEmrRollbackSec())*sEthCamTxDecInfo[path_id].Tbr+ sizeof(UINT32)*(MovieExe_GetEthcamEncBufSec(path_id)+ MovieExe_GetEmrRollbackSec())* sEthCamTxDecInfo[path_id].Fps;
	//g_SocketCliData2_BsBufTotalSize[path_id]  = (1)*sEthCamTxDecInfo[path_id].Tbr+ sizeof(UINT32)*(1)* sEthCamTxDecInfo[path_id].Fps;
	g_SocketCliData2_BsBufTotalSize[path_id]  = (6)*sEthCamTxDecInfo[path_id].Tbr+ sizeof(UINT32)*(6)* sEthCamTxDecInfo[path_id].Fps;
	if(g_SocketCliData2_BsBufTotalSize[path_id]==0){
		DBG_ERR("EthBsFrmBufTotalSize =0 !!\r\n");
	}
	if((g_SocketCliData2_BsBufTotalAddr[path_id]=socketCliEthData2_GetBsBufAddr(path_id, g_SocketCliData2_BsBufTotalSize[path_id]))==NVTEVT_CONSUME){
		DBG_ERR("BsBuf fail, want buf size=%d\r\n",g_SocketCliData2_BsBufTotalSize[path_id]);
		return -1;
	}
#if 0
	g_SocketCliData2_BsBufMapTbl = (UINT32*)g_SocketCliData2_BsBufTotalAddr;
	//DBG_DUMP("g_EthBsFrmBufMapTbl=0x%x, g_EthBsFrmTotalAddr=0x%x, end=0x%x\r\n",g_EthBsFrmBufMapTbl,g_EthBsFrmTotalAddr,g_EthBsFrmTotalAddr+g_EthBsFrmBufTotalSize);

	memset((UINT8*)g_SocketCliData2_BsBufTotalAddr, 0, g_SocketCliData2_BsBufTotalSize);
	g_SocketCliData2_BsAddr = (g_SocketCliData2_BsBufTotalAddr + sizeof(UINT32)*(3+ MovieExe_GetEmrRollbackSec())* MovieExe_GetFps(_CFG_CLONE_ID_1));
	g_SocketCliData2_BsBufTotalSize-= (sizeof(UINT32)*(3+ MovieExe_GetEmrRollbackSec())* MovieExe_GetFps(_CFG_CLONE_ID_1));
	//DBG_DUMP("1EthBsFrmAddr=0x%x, EthBsFrmBufTotalSize=0x%x, end=0x%x\r\n",g_EthBsFrmAddr,g_EthBsFrmBufTotalSize,g_EthBsFrmAddr+g_EthBsFrmBufTotalSize);
	while (g_SocketCliData2_BsAddr % 4 != 0)
	{
		g_SocketCliData2_BsAddr += 1;
	}
	g_SocketCliData2_BsBufTotalSize-=4;
	//DBG_DUMP("2EthBsFrmAddr=0x%x, EthBsFrmBufTotalSize=0x%x, end=0x%x\r\n",g_EthBsFrmAddr,g_EthBsFrmBufTotalSize,g_EthBsFrmAddr+g_EthBsFrmBufTotalSize);
#endif
	ETHCAM_SOCKET_BUF_OBJ BufObj={0};
	BufObj.ParamAddr=g_SocketCliData2_BsBufTotalAddr[path_id];
	BufObj.ParamSize=g_SocketCliData2_BsBufTotalSize[path_id];
	EthCamSocketCli_DataSetBsBuff(path_id, ETHSOCKIPCCLI_ID_1, &BufObj);

	//g_SocketCliData2_BsQueueMax[path_id]=((MovieExe_GetEthcamEncBufSec(path_id)+ MovieExe_GetEmrRollbackSec())* sEthCamTxDecInfo[path_id].Fps);
	//g_SocketCliData2_BsQueueMax[path_id]=((1)* sEthCamTxDecInfo[path_id].Fps);
	g_SocketCliData2_BsQueueMax[path_id]=((6)* sEthCamTxDecInfo[path_id].Fps);
	EthCamSocketCli_DataSetBsQueueMax(path_id, ETHSOCKIPCCLI_ID_1, g_SocketCliData2_BsQueueMax[path_id]);
#endif
	return 0;
}
UINT32 socketCliEthData_GetDebugBufAddr(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, UINT32 blk_size)
{
	void                 *va;
	UINT32               pa;
	ER                   ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	CHAR pool_name[30] ={0};
	if(g_SocketCliData_DebugPoolAddr[path_id][id].pool_va == 0)  {

            sprintf(pool_name,"socketCli_Debug_%d_%d",path_id,id);

		ret = hd_common_mem_alloc(pool_name, &pa, (void **)&va, blk_size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", blk_size, ddr_id);
			return 0;
		}
		DBG_IND("pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		g_SocketCliData_DebugPoolAddr[path_id][id].pool_va=(UINT32)va;
		g_SocketCliData_DebugPoolAddr[path_id][id].pool_pa=(UINT32)pa;
		memset(va, 0, blk_size);
	}

	if(g_SocketCliData_DebugPoolAddr[path_id][id].pool_va == 0)
		DBG_ERR("get buf addr err\r\n");
	return g_SocketCliData_DebugPoolAddr[path_id][id].pool_va;

}
void socketCliEthData_DestroyDebugBuff(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id)
{
	UINT32 ret;
	if (g_SocketCliData_DebugPoolAddr[path_id][id].pool_va != 0) {
		ret = hd_common_mem_free((UINT32)g_SocketCliData_DebugPoolAddr[path_id][id].pool_pa, (void *)g_SocketCliData_DebugPoolAddr[path_id][id].pool_va);
		if (ret != HD_OK) {
			DBG_ERR("FileIn release blk failed! (%d)\r\n", ret);
		}
		g_SocketCliData_DebugPoolAddr[path_id][id].pool_va = 0;
		g_SocketCliData_DebugPoolAddr[path_id][id].pool_pa = 0;
	}
}

void socketCliEthData_DestroyAllBuff(ETHCAM_PATH_ID path_id)
{
	socketCliEthData1_DestroyRecvBuff(path_id);
	socketCliEthData1_DestroyRawEncodeBuff(path_id);
	socketCliEthData1_DestroyBsBuff(path_id);
#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	socketCliEthData2_DestroyRecvBuff(path_id);
	socketCliEthData2_DestroyBsBuff(path_id);
#endif
	socketCliEthData_DestroyDebugBuff(path_id, ETHSOCKIPCCLI_ID_0);
	socketCliEthData_DestroyDebugBuff(path_id, ETHSOCKIPCCLI_ID_1);
}
INT32 socketCliEthData_DebugSet(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id)
{
	//return 0;

#if (ETHCAM_DECODE_ERR_DBG== ENABLE)
	CHAR ipccmd[64];
	g_SocketCliData_DebugAddr[path_id][id]=socketCliEthData_GetDebugBufAddr(path_id, id, ETHCAM_DEBUG_SIZE);

	snprintf(ipccmd, sizeof(ipccmd) - 1, "ethsockcliipc -debug %d %d %d %d",path_id, id, g_SocketCliData_DebugAddr[path_id][id], ETHCAM_DEBUG_SIZE);


	INT32 Ret_NvtIpc = ETHSOCKETCLIECOS_CmdLine(ipccmd, NULL,NULL);
	if (Ret_NvtIpc < 0) {
		DBG_ERR("[%d][%d]ETHSOCKETCLIECOS_CmdLine(%s)\r\n", path_id,id,ipccmd);
		return E_SYS;
	}

#endif
	return 0;
}
INT32 socketCliEthData_DebugDump(UINT32 path_id ,UINT32 id)
{
#if (ETHCAM_DECODE_ERR_DBG== ENABLE)
	char file_name[50] = {0};
	FST_FILE  FileHdl;
	UINT32 FileSize = ETHCAM_DEBUG_SIZE;
	//#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	//UINT32 id=1;
	//#else
	//UINT32 id=0;
	//#endif
	DBG_DUMP("[%d][%d]EthCamDebug\r\n",path_id, id);

 	sprintf(file_name, "A:\\EthCamDebug_%d_%d.dat", path_id,id);

	FileHdl = FileSys_OpenFile(file_name, FST_OPEN_WRITE | FST_CREATE_ALWAYS);
	FileSys_WriteFile(FileHdl, (UINT8 *)g_SocketCliData_DebugAddr[path_id][id], &FileSize, 0, NULL);
	FileSys_CloseFile(FileHdl);
	DBG_DUMP("[%d][%d]finish write EthCamDebug to file\r\n",path_id, id);
#endif
	return 0;
}

void socketCliEthData_Open(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id)
{
	//EthCamCmdTsk_Open();
	ETHSOCKCLIIPC_OPEN EthsockCliIpcOpen[ETHCAM_PATH_ID_MAX][2] = {0};
	if(id< ETHSOCKIPCCLI_ID_0 || id>ETHSOCKIPCCLI_ID_1){
		DBG_ERR("id out of range ,id=%d\r\n", id);
	}
	if(path_id>=ETHCAM_PATH_ID_MAX){
		DBG_ERR("path_id out of range ,path_id=%d\r\n", path_id);
	}

	if(id== ETHSOCKIPCCLI_ID_0){
		if(socketCliEthData1_ConfigRecvBuf(path_id)==-1){
			return;
		}
	    	EthCamSocketCli_SetDataRecvCB(path_id, ETHSOCKIPCCLI_ID_0, (UINT32)&socketCliEthData1_RecvCB);
		EthCamSocketCli_SetDataNotifyCB(path_id, ETHSOCKIPCCLI_ID_0, (UINT32)&socketCliEthData1_NotifyCB);
		socketCliEthData1_RecvResetParam(path_id);
		// open
		//EthsockCliIpcOpen[path_id][id].sharedSendMemAddr=OS_GetMempoolAddr(POOL_ID_ETHSOCKCLI_IPC)+(POOL_CNT_ETHSOCKCLI_IPC/ETH_REARCAM_CAPS_COUNT)*path_id*POOL_SIZE_ETHSOCKCLI_IPC  ;
		EthsockCliIpcOpen[path_id][id].sharedSendMemAddr=(UINT32)&g_sharedMemAddr[(POOL_CNT_ETHSOCKCLI_IPC/ETH_REARCAM_CAPS_COUNT)*path_id*POOL_SIZE_ETHSOCKCLI_IPC]  ;
		EthsockCliIpcOpen[path_id][id].sharedSendMemSize=POOL_SIZE_ETHSOCKCLI_IPC;
		if(sEthCamTxRecInfo[path_id].tbr!=0 && sEthCamTxDecInfo[path_id].bStarupOK==1){
			//EthsockCliIpcOpen[path_id][id].sharedRecvMemSize=(sEthCamTxRecInfo[path_id].tbr*100/TBR_SIZE_RATIO);
			EthsockCliIpcOpen[path_id][id].sharedRecvMemSize=(((sEthCamTxRecInfo[path_id].tbr *100/TBR_SIZE_RATIO) < POOL_SIZE_ETHSOCKCLI_IPC*2) ? POOL_SIZE_ETHSOCKCLI_IPC*2: (sEthCamTxRecInfo[path_id].tbr *100/TBR_SIZE_RATIO));
			//g_SocketCliData1_RecvAddr[path_id]=socketCliEthData1_GetRecvBufAddr(path_id, (sEthCamTxRecInfo[path_id].tbr *100/TBR_SIZE_RATIO));
			g_SocketCliData1_RecvAddr[path_id]=socketCliEthData1_GetRecvBufAddr(path_id, EthsockCliIpcOpen[path_id][id].sharedRecvMemSize);
		}else{
			g_SocketCliData1_RecvAddr[path_id]=socketCliEthData1_GetRecvBufAddr(path_id, MAX_I_FRAME_SZIE);
			EthsockCliIpcOpen[path_id][id].sharedRecvMemSize=MAX_I_FRAME_SZIE;
		}
		EthsockCliIpcOpen[path_id][id].sharedRecvMemAddr=g_SocketCliData1_RecvAddr[path_id];
		//DBG_DUMP("sEthCamTxRecInfo.tbr=%d, sharedRecvMemSize%d\r\n", sEthCamTxRecInfo.tbr,EthsockCliIpcOpen[id].sharedRecvMemSize );
		DBG_IND("Cli[%d] D1 SendMemAddr=0x%x, RecvMemAddr=0x%x\r\n",path_id,EthsockCliIpcOpen[path_id][id].sharedSendMemAddr,EthsockCliIpcOpen[path_id][id].sharedRecvMemAddr);
		EthCamSocketCli_Open(path_id,  id, &EthsockCliIpcOpen[path_id][id]);
		#if (ETH_REARCAM_CLONE_FOR_DISPLAY == DISABLE)
		socketCliEthData_DebugSet(path_id, id);
		#endif
	}else if(id== ETHSOCKIPCCLI_ID_1){
#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
		if(socketCliEthData2_ConfigRecvBuf(path_id)==-1){
			return;
		}
	    	EthCamSocketCli_SetDataRecvCB(path_id, ETHSOCKIPCCLI_ID_1, (UINT32)&socketCliEthData2_RecvCB);
		EthCamSocketCli_SetDataNotifyCB(path_id, ETHSOCKIPCCLI_ID_1, (UINT32)&socketCliEthData2_NotifyCB);
		socketCliEthData2_RecvResetParam(path_id);
		// open
		//EthsockCliIpcOpen[path_id][id].sharedSendMemAddr=OS_GetMempoolAddr(POOL_ID_ETHSOCKCLI_IPC)+(POOL_CNT_ETHSOCKCLI_IPC/ETH_REARCAM_CAPS_COUNT)*path_id*POOL_SIZE_ETHSOCKCLI_IPC +POOL_SIZE_ETHSOCKCLI_IPC;
		EthsockCliIpcOpen[path_id][id].sharedSendMemAddr=(UINT32)&g_sharedMemAddr[(POOL_CNT_ETHSOCKCLI_IPC/ETH_REARCAM_CAPS_COUNT)*path_id*POOL_SIZE_ETHSOCKCLI_IPC +POOL_SIZE_ETHSOCKCLI_IPC];
		EthsockCliIpcOpen[path_id][id].sharedSendMemSize=POOL_SIZE_ETHSOCKCLI_IPC;
		if(sEthCamTxDecInfo[path_id].Tbr !=0 && sEthCamTxDecInfo[path_id].bStarupOK==1){
			g_SocketCliData2_RecvAddr[path_id]=socketCliEthData2_GetRecvBufAddr(path_id, (sEthCamTxDecInfo[path_id].Tbr*100/TBR_SIZE_RATIO));
			EthsockCliIpcOpen[path_id][id].sharedRecvMemSize=(sEthCamTxDecInfo[path_id].Tbr*100/TBR_SIZE_RATIO);
		}else{
			g_SocketCliData2_RecvAddr[path_id]=socketCliEthData2_GetRecvBufAddr(path_id, MAX_I_FRAME_SZIE);
			EthsockCliIpcOpen[path_id][id].sharedRecvMemSize=MAX_I_FRAME_SZIE;
		}
		//DBG_DUMP("sEthCamTxDecInfo.Tbr=%d, sharedRecvMemSize%d\r\n", sEthCamTxDecInfo.Tbr,EthsockCliIpcOpen[id].sharedRecvMemSize );
		EthsockCliIpcOpen[path_id][id].sharedRecvMemAddr=g_SocketCliData2_RecvAddr[path_id];
		DBG_IND("Cli[%d] D2 SendMemAddr=0x%x, RecvMemAddr=0x%x\r\n",path_id,EthsockCliIpcOpen[path_id][id].sharedSendMemAddr,EthsockCliIpcOpen[path_id][id].sharedRecvMemAddr);
		EthCamSocketCli_Open(path_id, id, &EthsockCliIpcOpen[path_id][id]);
		socketCliEthData_DebugSet(path_id, id);
#endif
	}
}
void socketCliEthCmd_Open(ETHCAM_PATH_ID path_id)
{
	ETHSOCKCLIIPC_OPEN EthsockCliIpcOpen[ETHCAM_PATH_ID_MAX] = {0};

    	EthCamSocketCli_SetCmdRecvCB(path_id, (UINT32)&socketCliEthCmd_RecvCB);
    	EthCamSocketCli_SetCmdNotifyCB(path_id, (UINT32)&socketCliEthCmd_NotifyCB);
	//EthsockCliIpcOpen[path_id].sharedSendMemAddr=OS_GetMempoolAddr(POOL_ID_ETHSOCKCLI_IPC)+(POOL_CNT_ETHSOCKCLI_IPC/ETH_REARCAM_CAPS_COUNT)*path_id*POOL_SIZE_ETHSOCKCLI_IPC +2*POOL_SIZE_ETHSOCKCLI_IPC;
	EthsockCliIpcOpen[path_id].sharedSendMemAddr=(UINT32)&g_sharedMemAddr[(POOL_CNT_ETHSOCKCLI_IPC/ETH_REARCAM_CAPS_COUNT)*path_id*POOL_SIZE_ETHSOCKCLI_IPC +2*POOL_SIZE_ETHSOCKCLI_IPC];
	EthsockCliIpcOpen[path_id].sharedSendMemSize=POOL_SIZE_ETHSOCKCLI_IPC;

	//EthsockCliIpcOpen[path_id].sharedRecvMemAddr=OS_GetMempoolAddr(POOL_ID_ETHSOCKCLI_IPC)+(POOL_CNT_ETHSOCKCLI_IPC/ETH_REARCAM_CAPS_COUNT)*path_id*POOL_SIZE_ETHSOCKCLI_IPC +3*POOL_SIZE_ETHSOCKCLI_IPC;
	EthsockCliIpcOpen[path_id].sharedRecvMemAddr=(UINT32)&g_sharedMemAddr[(POOL_CNT_ETHSOCKCLI_IPC/ETH_REARCAM_CAPS_COUNT)*path_id*POOL_SIZE_ETHSOCKCLI_IPC +3*POOL_SIZE_ETHSOCKCLI_IPC];
	EthsockCliIpcOpen[path_id].sharedRecvMemSize=POOL_SIZE_ETHSOCKCLI_IPC;
	DBG_IND("Cli[%d] Cmd SendMemAddr=0x%x, RecvMemAddr=0x%x\r\n",path_id,EthsockCliIpcOpen[path_id].sharedSendMemAddr,EthsockCliIpcOpen[path_id].sharedRecvMemAddr);
	EthCamSocketCli_ReConnect(path_id, ETHSOCKIPCCLI_ID_2, 0);
	EthCamSocketCli_Open(path_id, ETHSOCKIPCCLI_ID_2, &EthsockCliIpcOpen[path_id]);
}
UINT32 g_testData1_Addr;
UINT32 g_testData1_Size;
UINT64 HwClock_DiffLongCounter(UINT64 time_start, UINT64 time_end)
{
	UINT32 time_start_sec = 0;
	UINT32 time_start_usec = 0;
	UINT32 time_end_sec=0;
	UINT32 time_end_usec =0;
	INT32  diff_time_sec =0 ;
	INT32  diff_time_usec = 0;
	UINT64 diff_time;

	time_start_sec = (time_start >> 32) & 0xFFFFFFFF;
	time_start_usec = time_start & 0xFFFFFFFF;
	time_end_sec = (time_end >> 32) & 0xFFFFFFFF;
	time_end_usec = time_end & 0xFFFFFFFF;
	diff_time_sec = (INT32)time_end_sec - (INT32)time_start_sec;
	diff_time_usec = (INT32)time_end_usec - (INT32)time_start_usec;
	diff_time = (INT64)diff_time_sec * 1000000 + diff_time_usec;
	return diff_time;
}
UINT64 HwClock_GetLongCounter(void)
{
	return hd_gettime_us();
}

void socketCliEthData1_RecvCB(ETHCAM_PATH_ID path_id, char* addr, int size)
{
	//static UINT32 eth_i_cnt = 0;
	UINT32 eth_is_i_frame = 0;
	UINT16 bPushData=0;
	UINT32 DescSize=0;
	//UINT16 i;

	if(sEthCamTxRecInfo[path_id].codec == HD_CODEC_TYPE_H264){//MEDIAVIDENC_H264){
		DescSize=sEthCamTxRecInfo[path_id].DescSize;
	}

	//static BOOL bFirstErr=0;
	//DBG_DUMP("D1 size=%d, 0x%x\r\n",  size, addr);
	//if(addr % 4){
	//	DBG_DUMP("addr%4!=0\r\n");
	//}
	if(addr[0] ==0 && addr[1] ==0 && addr[2] ==0 && addr[3] ==1){
		if(((addr[4])& 0x1F) == H264_NALU_TYPE_IDR  && addr[5] == H264_START_CODE_I){
			//DBG_DUMP("D1_I[%d] OK\r\n",path_id);
			//DBG_DUMP("D1_I[%d]=%d,%d\r\n",path_id,g_SocketCliData1_BsFrameCnt[path_id],size);
			g_SocketCliData1_BsFrameCnt[path_id]++;
			eth_is_i_frame = 1;
			g_SocketCliData1_eth_i_cnt[path_id] ++;
			bPushData=1;
		}else if(((addr[4])& 0x1F) == H264_NALU_TYPE_SLICE && addr[5] == H264_START_CODE_P){
			//DBG_DUMP("D1_P[%d] OK\r\n",path_id);
			//DBG_DUMP("D1_P[%d]=%d,%d\r\n",path_id,g_SocketCliData1_BsFrameCnt[path_id],size);
			g_SocketCliData1_BsFrameCnt[path_id]++;
			eth_is_i_frame = 0;
			bPushData=1;
		}else if((((addr[4])>>1)&0x3F) == H265_NALU_TYPE_VPS){
			//DBG_DUMP("D1_I[%d] OK\r\n",path_id);
			//DBG_DUMP("D1_h265_I[%d]=%d,%d\r\n",path_id,g_SocketCliData1_BsFrameCnt[path_id],size);
			g_SocketCliData1_BsFrameCnt[path_id]++;
			eth_is_i_frame = 1;
			g_SocketCliData1_eth_i_cnt[path_id] ++;
			bPushData=1;
		}else if((((addr[4])>>1)&0x3F) == H265_NALU_TYPE_SLICE){
			//DBG_DUMP("D1_P[%d] OK\r\n",path_id);
			//DBG_DUMP("D1_P[%d]=%d,%d\r\n",path_id,g_SocketCliData1_BsFrameCnt[path_id],size);
			g_SocketCliData1_BsFrameCnt[path_id]++;
			eth_is_i_frame = 0;
			bPushData=1;

		}else if(((addr[4])==HEAD_TYPE_THUMB)){
			DBG_DUMP("D1_RecvCB Thumb OK, FRcnt[%d]=%d, sz=%d\r\n",path_id,g_SocketCliData1_BsFrameCnt[path_id],size);
			//g_testData1_RawEncodeAddr=(UINT32)addr;
			//g_testData1_RawEncodeSize=(UINT32)size;
			//set_flg(ETHCAM_CMD_RCV_FLG_ID, FLG_ETHCAM_CMD_RCV);
			bPushData = 2;
			//DBG_DUMP("data[4]=0x%x, %x, %x, %x, %x, %x, %x, %x\r\n",addr[4],addr[5],addr[6],addr[7],addr[8],addr[size-3],addr[size-2],addr[size-1]);
		}else if(((addr[4])==HEAD_TYPE_RAW_ENCODE)){
			DBG_DUMP("D1_RecvCB PIM OK, FRcnt[%d]=%d\r\n",path_id,g_SocketCliData1_BsFrameCnt[path_id]);
			bPushData = 3;
			//DBG_DUMP("data[4]=0x%x, %x, %x, %x, %x, %x, %x, %x\r\n",addr[4],addr[5],addr[6],addr[7],addr[8],addr[size-3],addr[size-2],addr[size-1]);
			BKG_PostEvent(NVTEVT_BKW_ETHCAM_RAW_ENCODE_RESULT);
		}else{
			//DBG_DUMP("Check FAIL\r\n");
		}

	}else if(addr[0+DescSize] ==0 && addr[1+DescSize] ==0 && addr[2+DescSize] ==0 && addr[3+DescSize] ==1){
		if(((addr[4+DescSize])& 0x1F) == H264_NALU_TYPE_IDR && addr[5+DescSize] == H264_START_CODE_I){
			g_SocketCliData1_BsFrameCnt[path_id]++;
			eth_is_i_frame = 1;
			g_SocketCliData1_eth_i_cnt[path_id] ++;
			bPushData=1;
		}
	}//else{
		if(bPushData){
		//g_testData1_Addr=(UINT32)addr;
		//g_testData1_Size=(UINT32)size;

		//set_flg(ETHCAM_CMD_RCV_FLG_ID, FLG_ETHCAM_CMD_RCV);
		//vos_flag_set(ETHCAM_CMD_RCV_FLG_ID, FLG_ETHCAM_CMD_RCV);
		}
#if 0//debug, write to card
		//static BOOL bFirstErr=0;

		//bFirstErr=1;
		g_testData1_Addr=(UINT32)addr;
		g_testData1_Size=(UINT32)size;

		set_flg(ETHCAM_CMD_RCV_FLG_ID, FLG_ETHCAM_CMD_RCV);
				char path[30];
				sprintf(path, "A:\\RxData.bin");
				DBG_DUMP("=========RxData: %s\r\n", path);

				static  FST_FILE fhdl = 0;
				UINT32      fileSize = 0;
				static UINT32 uiRecvSize=0;
				UINT32      TotalfileSize = 10*1024*1024;
				if(fhdl==0 && (uiRecvSize < TotalfileSize)){
					fhdl = FileSys_OpenFile(path, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
					FileSys_SeekFile(fhdl, 0, FST_SEEK_SET);
				}else if(fhdl && (uiRecvSize < TotalfileSize)){
					FileSys_SeekFile(fhdl, 0, FST_SEEK_END);
				}
				fileSize = size;
				if(fhdl && (uiRecvSize < TotalfileSize)){
					FileSys_WriteFile(fhdl, (UINT8 *)addr, &fileSize, 0, NULL);
					uiRecvSize+=size;
				}
				if(fhdl && (uiRecvSize>=TotalfileSize)){
					DBG_DUMP("Write Finish!\r\n");
					FileSys_CloseFile(fhdl);
					fhdl = 0;
				}
//#else
				static UINT32 uiRecvSize=0;
				UINT32      TotalfileSize = 8*1024*1024;

				hwmem_open();
				hwmem_memcpy((UINT32)(g_Test_Addr+uiRecvSize), (UINT32)addr, (UINT32)size);
				hwmem_close();
				if((uiRecvSize + size) < TotalfileSize){
					uiRecvSize+=size;
				}else{
					FST_FILE fhdl = 0;
					char path[30];
					sprintf(path, "A:\\RxData.bin");
					fhdl = FileSys_OpenFile(path, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
					FileSys_SeekFile(fhdl, 0, FST_SEEK_SET);
					FileSys_WriteFile(fhdl, (UINT8 *)g_Test_Addr, &uiRecvSize, 0, NULL);
					FileSys_CloseFile(fhdl);
					DBG_DUMP("Write Finish!\r\n");
				}
#endif
	//}

	if (bPushData == 1){
		UINT32 LongCounterSizeOffset=LONGCNT_STAMP_OFFSET;
		UINT32 StreamSize = size-LONGCNT_STAMP_OFFSET;
		UINT32 LongConterLow[ETHCAM_PATH_ID_MAX]={0};
		UINT32 LongConterHigh[ETHCAM_PATH_ID_MAX]={0};
		UINT64 LongConter[ETHCAM_PATH_ID_MAX]={0};
		static UINT32 LongCntTxRxOffset[ETHCAM_PATH_ID_MAX]={0};
		//DBG_DUMP("D1longcnt=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",addr[StreamSize+0],addr[StreamSize+1],addr[StreamSize+2],addr[StreamSize+3],addr[StreamSize+4],addr[StreamSize+5],addr[StreamSize+6],addr[StreamSize+7],addr[StreamSize+8],addr[StreamSize+9],addr[StreamSize+10],addr[StreamSize+11],addr[StreamSize+12]);
		if(addr[StreamSize+0] ==0 && addr[StreamSize+1] ==0 && addr[StreamSize+2] ==0 && addr[StreamSize+3] ==1 && addr[StreamSize+4]==HEAD_TYPE_LONGCNT_STAMP){
			LongConterHigh[path_id]=(addr[StreamSize+5]<<24)+ (addr[StreamSize+6]<<16)+ (addr[StreamSize+7]<<8) + (addr[StreamSize+8]);
			LongConterLow[path_id]=(addr[StreamSize+9]<<24)+ (addr[StreamSize+10]<<16)+ (addr[StreamSize+11]<<8) + (addr[StreamSize+12]);
			LongConter[path_id]=(((UINT64)LongConterHigh[path_id]<<32) | LongConterLow[path_id]);
			if(g_SocketCliData1_BsFrameCnt[path_id]> 10 && g_SocketCliData1_BsFrameCnt[path_id] <= 60){
				LongCntTxRxOffset[path_id]+=HwClock_DiffLongCounter(LongConter[path_id], HwClock_GetLongCounter());
				//DBG_DUMP("11D1_LongCntTxRxOffset[%d]=%d\r\n",path_id,LongCntTxRxOffset[path_id]);

				if(g_SocketCliData1_BsFrameCnt[path_id] == 60){
					//DBG_DUMP("22D1_LongCntTxRxOffset[%d]=%d\r\n",path_id,LongCntTxRxOffset[path_id]);
					g_SocketCliData1_LongCntTxRxOffset[path_id]=LongCntTxRxOffset[path_id]/(60-10);
				}
			}else{
				LongCntTxRxOffset[path_id] =0;
			}
			//DBG_DUMP("D1_LongCntHigh[%d]=(%d,%d), %d, (%d,%d), diff=%d, %d\r\n",path_id,LongConterHigh[path_id],LongConterLow[path_id],(UINT32)LongConter[path_id],(UINT32)((HwClock_GetLongCounter() >> 32) & 0xFFFFFFFF),(UINT32)(HwClock_GetLongCounter() & 0xFFFFFFFF), (UINT32)HwClock_DiffLongCounter(LongConter[path_id], HwClock_GetLongCounter())/1000,(UINT32)g_SocketCliData1_LongCntTxRxOffset[path_id]/1000);
		}else{
			LongCounterSizeOffset=0;
		}




		UINT32 g_addrPa = 0;
		UINT32 g_addrVa = 0;
		g_addrVa=(UINT32)addr;
		g_addrPa=g_SocketCliData1_BsPoolAddr[path_id][0].pool_pa+ g_addrVa-g_SocketCliData1_BsPoolAddr[path_id][0].pool_va;
		#if (ETHCAM_EIS ==ENABLE) || (ETH_REARCAM_CLONE_FOR_DISPLAY == DISABLE)
		UINT64 timestamp=0;
		#endif
		//parsing ethcam meta
		UINT32 ptr_offset= (size >= (int)LongCounterSizeOffset) ? ( size -LongCounterSizeOffset) : 0;
		UINT8*ptr= (UINT8*)(addr + ptr_offset);
		UINT32 MetaTotalSize=0;

		ETHCAM_META_HEADER* pEthCamMetaHeader = ptr ? ((ETHCAM_META_HEADER *)(ptr- sizeof(ETHCAM_META_HEADER))): NULL;
		#if (ETHCAM_EIS ==ENABLE)
		UINT32 gyro_meta_sign=GYRO_META_SIGN;
		UINT16 i=0;
		#endif
		//UINT32 lut_meta_sign=LUT_META_SIGN;
		if(pEthCamMetaHeader && pEthCamMetaHeader->sign==ETHCAM_META_SIGN){
			MetaTotalSize= pEthCamMetaHeader->data_size;
			#if (ETHCAM_EIS ==ENABLE)
			if(SysGetFlag(FL_MOVIE_EIS) == EIS_ON ){
				ptr = ptr ? (ptr -MetaTotalSize): NULL;
				for(i=0;i<MetaTotalSize;i++){
					if(memcmp((void*)&ptr[i], (void*)&gyro_meta_sign,sizeof(GYRO_META_SIGN))==0){
						VENDOR_COMM_DUMMY *gyro_meta = (VENDOR_COMM_DUMMY *)&ptr[i];
						VENDOR_COMM_DUMMY *lut_meta = gyro_meta ? ((VENDOR_COMM_DUMMY *)&ptr[i+ HD_VENDOR_META_HEADER_SIZE + gyro_meta->buffer_size]) : NULL;
						//DBG_DUMP("gyro_size=%d, lut_size=%d\r\n",gyro_meta->buffer_size, lut_meta->buffer_size);
						//DBG_DUMP("gyro_meta=0x%x, 0x%x, pEthCamMetaHeader=0x%x\n\r", gyro_meta,  lut_meta, pEthCamMetaHeader);
						if((i+ HD_VENDOR_META_HEADER_SIZE + gyro_meta->buffer_size) >= MetaTotalSize){
							DBG_ERR("out of range, i=%d, buffer_size=%d, MSz=%d\r\n",i, gyro_meta->buffer_size, MetaTotalSize);
							break;
						}
						if(lut_meta && lut_meta->sign == LUT_META_SIGN){
							//DBG_DUMP("[%d]gyro_size=%d, lut_size=%d\r\n",path_id,gyro_meta->buffer_size, lut_meta->buffer_size);

							#if (ETHCAM_EIS ==ENABLE)
							//DBG_DUMP("P p=%d\r\n", path_id);
							//fflush(stdout);
							if(SysGetFlag(FL_MOVIE_EIS) == EIS_ON ){
								timestamp= hd_gettime_us();
								MovieExe_EthCamRecId1_PutMetaQ(path_id, (UINT8*)&ptr[i], timestamp);
							}
							#endif
						}
						break;
					}
				}
			}
			#endif
		}
		//ptr = (UINT8*)(addr + ptr_offset-sizeof(ETHCAM_META_HEADER));
		//DBG_DUMP("ptr[0]=0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x ,sign=%x\r\n",ptr[0],ptr[1],ptr[2],ptr[3],ptr[4],ptr[5],ptr[6],ptr[7],   ptr[8],ptr[9],ptr[10],ptr[11],ptr[12], ETHCAM_META_SIGN);

		ptr = (UINT8*)(addr + ptr_offset-4-MetaTotalSize);
		//get nalu info
		//UINT32 ptr_offset= ( size -LongCounterSizeOffset-4);
		//UINT8*ptr= (UINT8*)(addr + ptr_offset);
		UINT16 k=0, j=0;
		UINT32 len, nalu_size=0, nalu_num=0, is_nalu_info=0;
		HD_BUFINFO *nalu_data;
		for(k=0;k<64;k++){

			if(*(ptr-k)>0 && *(ptr-k+1)==0 && *(ptr-k+2)==0 && *(ptr-k+3)==0){
                len=0;
                nalu_num= *(ptr-k);
                nalu_size = (1+nalu_num*2)*sizeof(UINT32);
                nalu_data = (HD_BUFINFO *)(((UINT32)(ptr-k))+sizeof(UINT32));
                for (j=0; j<nalu_num; j++) {
					if(nalu_data[j].phy_addr==0 || nalu_data[j].buf_size==0){
						len=0;
						break;
					}
					len += nalu_data[j].buf_size;
					//DBG_DUMP("[%d][%d]addr=0x%x, sz=%d, len=%d\r\n",k,j,nalu_data[j].phy_addr,nalu_data[j].buf_size, len);

					if(len > (UINT32)size){

						len=0;
						break;
					}
                }
                if((eth_is_i_frame==0 && len == (size -LongCounterSizeOffset - nalu_size))
						|| (eth_is_i_frame ==1  && len == (size -LongCounterSizeOffset - sEthCamTxRecInfo[path_id].DescSize- nalu_size))){
                	//DBG_DUMP("k=%d, nalu_num=%d, len=%d\r\n",k, nalu_num,len);
                	is_nalu_info=1;
                	break;
                }
			}
		}
		nalu_size = (is_nalu_info == 1)? nalu_size: 0;
		nalu_num = (is_nalu_info == 1)? nalu_num: 0;
		#if (ETH_REARCAM_CLONE_FOR_DISPLAY == DISABLE)

		HD_VIDEODEC_BS vdec_bs = {
			.sign          = MAKEFOURCC('V','S','T','M'),
			.p_next        = NULL,
			.vcodec_format =  sEthCamTxDecInfo[path_id].Codec ,//(UI_GetData(FL_MOVIE_CODEC) == MOVIE_CODEC_H265) ? HD_CODEC_TYPE_H265:HD_CODEC_TYPE_H264 ,//HD_CODEC_TYPE_H264,
			.count         = 0,
			.timestamp     = (timestamp !=0) ? timestamp: hd_gettime_us(),
			.ddr_id        = DDR_ID0,
			.phy_addr      = (g_addrPa+sEthCamTxDecInfo[path_id].VPSSize+sEthCamTxDecInfo[path_id].SPSSize+sEthCamTxDecInfo[path_id].PPSSize),//(UINT32)addr,//ethbuf_bs_pa[buf_idx] + header_offset,
			.size          = (size-LongCounterSizeOffset-(sEthCamTxDecInfo[path_id].VPSSize+sEthCamTxDecInfo[path_id].SPSSize+sEthCamTxDecInfo[path_id].PPSSize) - (MetaTotalSize ? MetaTotalSize :nalu_size)),
			.blk           = -2,
		};
		if (eth_is_i_frame==0) {    // I frame
			vdec_bs.phy_addr=g_addrPa;
			vdec_bs.size=(size-LongCounterSizeOffset-(MetaTotalSize ? MetaTotalSize :nalu_size));
		}

		if(ImageApp_MovieMulti_EthCamLinkForDispStatus(_CFG_ETHCAM_ID_1+ path_id)){ //VdoDec is running
			ImageApp_MovieMulti_EthCamLinkPushVdoBsToDecoder(_CFG_ETHCAM_ID_1+ path_id, &vdec_bs);
		}


		#if (ETHCAM_EIS ==ENABLE)
		if(SysGetFlag(FL_MOVIE_EIS) == EIS_ON ){
			HD_VIDEO_FRAME src_img[ETH_REARCAM_CAPS_COUNT] = {0};
			HD_RESULT hd_ret;
			j =path_id;
			i =_CFG_ETHCAM_ID_1+path_id;

			if ((hd_ret = ImageApp_MovieMulti_DispPullOut(i, &src_img[j], -1)) != HD_OK) {
				DBG_ERR("ethcam pull_out yuv error[%d]=%d\r\n", i-_CFG_ETHCAM_ID_1, hd_ret);
			}else{
				//get meta data
				MovieExe_EthCamRecId1_GetMetaQ(j, (UINT8*)&src_img[j]);

				//for vpe yuv continual address
				src_img[j].dim.h=ALIGN_CEIL_16(src_img[j].dim.h);
				src_img[j].ph[0]=ALIGN_CEIL_16(src_img[j].ph[0]);
				src_img[j].ph[1]=ALIGN_CEIL_16(src_img[j].ph[1]);


				//push in VPE
				if ((hd_ret = hd_videoproc_push_in_buf(ImageApp_MovieMulti_GetVprcInPort(i), &(src_img[j]), NULL, -1)) != HD_OK) {
					if(hd_ret !=HD_ERR_UNDERRUN){
						DBG_ERR("hd_videoproc_push_in_buf fail id=%d, port=0x%x, ret=%d\r\n", j,ImageApp_MovieMulti_GetVprcInPort(i),hd_ret);
					}
				}
				if (src_img[j].pw[0] || src_img[j].phy_addr[0]) {
					if ((hd_ret = ImageApp_MovieMulti_DispReleaseOut(i, &src_img[j])) != HD_OK) {
						DBG_ERR("pip release_out error\r\n", hd_ret);
					}
				}

			}
		}
		#endif
		#endif


		//UINT32 g_addrPa = 0;
		//UINT32 g_addrVa = 0;
		g_addrVa=(UINT32)addr;
		g_addrPa=g_SocketCliData1_BsPoolAddr[path_id][0].pool_pa+ g_addrVa-g_SocketCliData1_BsPoolAddr[path_id][0].pool_va;

		#if 1//(ETHCAM_EIS ==DISABLE)
		if(SysGetFlag(FL_MOVIE_EIS) == EIS_OFF ){
		HD_VIDEOENC_BS venc_bs = {0};
		venc_bs.sign          = MAKEFOURCC('V','S','T','M');
		venc_bs.vcodec_format = sEthCamTxRecInfo[path_id].codec;//HD_CODEC_TYPE_H265;
		venc_bs.timestamp     = hd_gettime_us();
		venc_bs.frame_type    = eth_is_i_frame ? HD_FRAME_TYPE_IDR : HD_FRAME_TYPE_P;


		if (eth_is_i_frame) {    // I frame
			venc_bs.frame_type    = HD_FRAME_TYPE_IDR;
			if(sEthCamTxRecInfo[path_id].codec == HD_CODEC_TYPE_H264){
				venc_bs.pack_num      = 3;
				memcpy((UINT8*)g_addrVa, (UINT8*)&sEthCamTxRecInfo[path_id].Desc[0],sEthCamTxRecInfo[path_id].DescSize);
				venc_bs.video_pack[0].pack_type.h264_type = H264_NALU_TYPE_SPS;
				//memcpy((UINT8*)g_addrVa, (UINT8*)&sEthCamTxRecInfo[path_id].Desc[0],sEthCamTxRecInfo[path_id].SPSSize);
				venc_bs.video_pack[0].phy_addr = g_addrPa;
				venc_bs.video_pack[0].size = sEthCamTxRecInfo[path_id].SPSSize;

				venc_bs.video_pack[1].pack_type.h264_type = H264_NALU_TYPE_PPS;
				//memcpy((UINT8*)(g_addrVa+sEthCamTxRecInfo[path_id].SPSSize), (UINT8*)&sEthCamTxRecInfo[path_id].Desc[sEthCamTxRecInfo[path_id].SPSSize],sEthCamTxRecInfo[path_id].PPSSize);
				venc_bs.video_pack[1].phy_addr = (g_addrPa+sEthCamTxRecInfo[path_id].SPSSize);
				venc_bs.video_pack[1].size = sEthCamTxRecInfo[path_id].PPSSize;

				venc_bs.video_pack[2].pack_type.h264_type = H264_NALU_TYPE_IDR;
				venc_bs.video_pack[2].phy_addr = (g_addrPa+sEthCamTxRecInfo[path_id].DescSize);
				venc_bs.video_pack[2].size = (size-LongCounterSizeOffset-sEthCamTxRecInfo[path_id].DescSize-(MetaTotalSize ? MetaTotalSize :nalu_size));

			}else{
				venc_bs.pack_num      = 4;
				venc_bs.video_pack[0].pack_type.h265_type = H265_NALU_TYPE_VPS;
				venc_bs.video_pack[0].phy_addr = g_addrPa;
				venc_bs.video_pack[0].size = sEthCamTxRecInfo[path_id].VPSSize;

				venc_bs.video_pack[1].pack_type.h265_type = H265_NALU_TYPE_SPS;
				venc_bs.video_pack[1].phy_addr = (g_addrPa+sEthCamTxRecInfo[path_id].VPSSize);
				venc_bs.video_pack[1].size = sEthCamTxRecInfo[path_id].SPSSize;

				venc_bs.video_pack[2].pack_type.h265_type = H265_NALU_TYPE_PPS;
				venc_bs.video_pack[2].phy_addr = (g_addrPa+sEthCamTxRecInfo[path_id].VPSSize + sEthCamTxRecInfo[path_id].SPSSize);
				venc_bs.video_pack[2].size = sEthCamTxRecInfo[path_id].PPSSize;

				venc_bs.video_pack[3].pack_type.h265_type = H265_NALU_TYPE_IDR;
				venc_bs.video_pack[3].phy_addr = (g_addrPa+sEthCamTxRecInfo[path_id].DescSize);
				venc_bs.video_pack[3].size = (size-LongCounterSizeOffset-sEthCamTxRecInfo[path_id].DescSize-(MetaTotalSize ? MetaTotalSize :nalu_size));
			}
		} else {                // P frame
			venc_bs.pack_num      = 1;
			venc_bs.frame_type    = HD_FRAME_TYPE_P;
			if(sEthCamTxRecInfo[path_id].codec == HD_CODEC_TYPE_H264){
				venc_bs.video_pack[0].pack_type.h264_type = H264_NALU_TYPE_SLICE;
			}else{
				venc_bs.video_pack[0].pack_type.h265_type = H265_NALU_TYPE_SLICE;
			}
			venc_bs.video_pack[0].phy_addr = g_addrPa;
				//venc_bs.video_pack[0].size = (size-LongCounterSizeOffset-nalu_size);
				venc_bs.video_pack[0].size = (size-LongCounterSizeOffset-(MetaTotalSize ? MetaTotalSize :nalu_size));
		}
		if(sEthCamTxRecInfo[path_id].codec == HD_CODEC_TYPE_H264){
			venc_bs.p_next = (HD_METADATA*)(g_addrPa + size -LongCounterSizeOffset- MetaTotalSize);
			addr[size -LongCounterSizeOffset+0]=1;
			addr[size -LongCounterSizeOffset+1]=0;
			addr[size -LongCounterSizeOffset+2]=0;
			addr[size -LongCounterSizeOffset+3]=0;
		}else{
			if(is_nalu_info==0 || nalu_num == 0){
				venc_bs.p_next = (HD_METADATA*)(g_addrPa + size -LongCounterSizeOffset- MetaTotalSize);
				addr[size -LongCounterSizeOffset- MetaTotalSize+0]=1;
				addr[size -LongCounterSizeOffset- MetaTotalSize+1]=0;
				addr[size -LongCounterSizeOffset- MetaTotalSize+2]=0;
				addr[size -LongCounterSizeOffset- MetaTotalSize+3]=0;
				//if(sEthCamTxRecInfo[path_id].width>2560){
				//	DBG_WRN("width over 2560 ,nalu info not correct, nalu_num=%d\r\n",nalu_num);
				//}
			}else{
				venc_bs.p_next = (HD_METADATA*)(g_addrPa + ptr_offset -k);
				nalu_data = (HD_BUFINFO *)(((UINT32)(ptr-k))+sizeof(UINT32));
				if (eth_is_i_frame) {
					nalu_data[0].phy_addr=(g_addrPa+sEthCamTxRecInfo[path_id].DescSize);
				}else{
					nalu_data[0].phy_addr=(g_addrPa);
				}
				for (j=1; j<nalu_num; j++) {
					nalu_data[j].phy_addr=nalu_data[j-1].phy_addr+ nalu_data[j-1].buf_size;
				}
				//DBG_DUMP("nalu_num=%d, phy_addr=0x%x, 0x%x, 0x%x\r\n",nalu_num, nalu_data[0].phy_addr, nalu_data[1].phy_addr, nalu_data[2].phy_addr);
			}
		}
		ImageApp_MovieMulti_EthCamLinkPushVdoBsToBsMux(_CFG_ETHCAM_ID_1 + path_id, &venc_bs);
		}
		#endif

		if(socketCliEthData1_IsRecv(path_id)==0)
		{
			socketCliEthData1_SetRecv(path_id, 1);
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == DISABLE)
			#if(ETH_REARCAM_CAPS_COUNT >=2)
			UI_SetData(FL_DUAL_CAM, DUALCAM_BOTH);
			//UI_SetData(FL_DUAL_CAM_MENU, DUALCAM_BOTH);
			#else
			UI_SetData(FL_DUAL_CAM, DUALCAM_BEHIND);
			//UI_SetData(FL_DUAL_CAM_MENU, DUALCAM_BEHIND);
			#endif
			MovieExe_EthCam_ChgDispCB(UI_GetData(FL_DUAL_CAM));
#endif
		}
		EthCamHB1[path_id] = 0;
	} else if (bPushData == 2 || bPushData == 3) {
		if(bPushData  == 3){
			HD_FILEOUT_BUF jpg_buf = {0};
			jpg_buf.sign = MAKEFOURCC('F','O','U','T');
			jpg_buf.type = MEDIA_FILEFORMAT_JPG;
			jpg_buf.fileop = HD_FILEOUT_FOP_SNAPSHOT;
			jpg_buf.event = HD_BSMUX_FEVENT_NORMAL;
			jpg_buf.addr = (UINT32)addr + 5;                                                             // note: fileout use va instead
			jpg_buf.size = size - 5;
			ImageApp_MovieMulti_EthCamLinkPushJpgToFileOut(_CFG_ETHCAM_ID_1 + path_id, &jpg_buf);
		}else{
			#if 1
			UINT32 g_addrPa = 0;
			UINT32 g_addrVa = 0;
			g_addrVa=(UINT32)(addr + 5);
			g_addrPa=g_SocketCliData1_RawEncodePoolAddr[path_id][0].pool_pa+ g_addrVa-g_SocketCliData1_RawEncodePoolAddr[path_id][0].pool_va;
			(path_id==0)?AppBKW_SetData(BKW_ETHCAM_TRIGGER_THUMB_PATHID_P0, 0):AppBKW_SetData(BKW_ETHCAM_TRIGGER_THUMB_PATHID_P1, 0);

			HD_BSMUX_PUT_DATA thumb_info = {
                            .type     = HD_BSMUX_PUT_DATA_TYPE_THUMB,
                            .phy_addr =g_addrPa,
                            .vir_addr = g_addrVa,
                            .size     = (size - 5),
			};
			ImageApp_MovieMulti_EthCamLinkPushThumbToBsMux((_CFG_ETHCAM_ID_1+path_id), &thumb_info, 0);
			#endif
		}
	}

#if 0

	if(bPushData==1 ){
		//push data to VdoDec
	#if 1
		//extern UINT8 SPS_Addr[];

		ISF_PORT         *pDist;
		ISF_VIDEO_STREAM_BUF    *pVidStrmBuf;

		UINT32 LongCounterSizeOffset=LONGCNT_STAMP_OFFSET;
		UINT32 StreamSize = size-LONGCNT_STAMP_OFFSET;
		UINT32 LongConterLow[ETHCAM_PATH_ID_MAX]={0};
		UINT32 LongConterHigh[ETHCAM_PATH_ID_MAX]={0};
		UINT64 LongConter[ETHCAM_PATH_ID_MAX]={0};
		static UINT32 LongCntTxRxOffset[ETHCAM_PATH_ID_MAX]={0};
		//DBG_DUMP("D1longcnt=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",addr[StreamSize+0],addr[StreamSize+1],addr[StreamSize+2],addr[StreamSize+3],addr[StreamSize+4],addr[StreamSize+5],addr[StreamSize+6],addr[StreamSize+7],addr[StreamSize+8],addr[StreamSize+9],addr[StreamSize+10],addr[StreamSize+11],addr[StreamSize+12]);
		if(addr[StreamSize+0] ==0 && addr[StreamSize+1] ==0 && addr[StreamSize+2] ==0 && addr[StreamSize+3] ==1 && addr[StreamSize+4]==HEAD_TYPE_LONGCNT_STAMP){
			LongConterHigh[path_id]=(addr[StreamSize+5]<<24)+ (addr[StreamSize+6]<<16)+ (addr[StreamSize+7]<<8) + (addr[StreamSize+8]);
			LongConterLow[path_id]=(addr[StreamSize+9]<<24)+ (addr[StreamSize+10]<<16)+ (addr[StreamSize+11]<<8) + (addr[StreamSize+12]);
			LongConter[path_id]=(((UINT64)LongConterHigh[path_id]<<32) | LongConterLow[path_id]);
			if(g_SocketCliData1_BsFrameCnt[path_id]> 10 && g_SocketCliData1_BsFrameCnt[path_id] <= 60){
				LongCntTxRxOffset[path_id]+=HwClock_DiffLongCounter(LongConter[path_id], HwClock_GetLongCounter());
				//DBG_DUMP("11D1_LongCntTxRxOffset[%d]=%d\r\n",path_id,LongCntTxRxOffset[path_id]);

				if(g_SocketCliData1_BsFrameCnt[path_id] == 60){
					//DBG_DUMP("22D1_LongCntTxRxOffset[%d]=%d\r\n",path_id,LongCntTxRxOffset[path_id]);
					g_SocketCliData1_LongCntTxRxOffset[path_id]=LongCntTxRxOffset[path_id]/(60-10);
				}
			}else{
				LongCntTxRxOffset[path_id] =0;
			}
			//DBG_DUMP("D1_LongCntHigh[%d]=(%d,%d), %d, (%d,%d), diff=%d, %d\r\n",path_id,LongConterHigh[path_id],LongConterLow[path_id],(UINT32)LongConter[path_id],(UINT32)((HwClock_GetLongCounter() >> 32) & 0xFFFFFFFF),(UINT32)(HwClock_GetLongCounter() & 0xFFFFFFFF), (UINT32)HwClock_DiffLongCounter(LongConter[path_id], HwClock_GetLongCounter())/1000,(UINT32)g_SocketCliData1_LongCntTxRxOffset[path_id]/1000);
		}else{
			LongCounterSizeOffset=0;
		}


		g_SocketCliData1_InIsfData[path_id] = gISF_EthCam_Pull_InData1;

#if 0
		if(socketCliEthData1_IsRecv(path_id)==0)
		{
			socketCliEthData1_SetRecv(path_id, 1);
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == DISABLE)
			#if(ETH_REARCAM_CAPS_COUNT >=2)
			UI_SetData(FL_DUAL_CAM, DUALCAM_BOTH);
			#else
			UI_SetData(FL_DUAL_CAM, DUALCAM_BEHIND);
			#endif
			MovieExe_UserProc_ChgCB(UI_GetData(FL_DUAL_CAM));
#endif
		}
#endif

		pVidStrmBuf = (ISF_VIDEO_STREAM_BUF *)&g_SocketCliData1_InIsfData[path_id].Desc;
		pVidStrmBuf->flag     = MAKEFOURCC('V', 'S', 'T', 'M');
		pVidStrmBuf->DataAddr = (UINT32)addr;//uiBsFrmBufAddr;//g_EthBsFrmAddr[g_queue_idx-1];//pVdoBs->BsAddr;
		pVidStrmBuf->DataSize = (UINT32)(size-LongCounterSizeOffset);//g_SocketCliData1_BsBufMapTbl[g_SocketCliData1_BsQueueIdx];//g_CliSocket_size_frm[g_queue_idx-1];//pVdoBs->BsSize;
		//pVidStrmBuf->CodecType = MEDIAVIDENC_H264;                                          //codec type
		pVidStrmBuf->CodecType = sEthCamTxRecInfo[path_id].codec;                                          //codec type
		//pVidStrmBuf->Resv[0]  = (UINT32)&(SPS_Addr[0]);                                     //sps addr
		if(sEthCamTxRecInfo[path_id].codec == MEDIAVIDENC_H264){
			pVidStrmBuf->Resv[0]  = (UINT32)&(sEthCamTxRecInfo[path_id].Desc[0]);         //sps addr
			//pVidStrmBuf->Resv[1]  = 28;                                                         //sps size
			pVidStrmBuf->Resv[1]  = sEthCamTxRecInfo[path_id].SPSSize;                         //sps size
			//pVidStrmBuf->Resv[2]  = (UINT32)&(SPS_Addr[28]);                                    //pps addr
			pVidStrmBuf->Resv[2]  = (UINT32)&(sEthCamTxRecInfo[path_id].Desc[sEthCamTxRecInfo[path_id].SPSSize]);                                    //pps addr
			//pVidStrmBuf->Resv[3]  = 8;
			pVidStrmBuf->Resv[3]  = sEthCamTxRecInfo[path_id].PPSSize;                                                          //pps size
			pVidStrmBuf->Resv[4]  = 0;                                                          //vps addr
			pVidStrmBuf->Resv[5]  = 0;                                                          //vps size
		}else{
			pVidStrmBuf->Resv[0]  = (UINT32)&(sEthCamTxRecInfo[path_id].Desc[sEthCamTxRecInfo[path_id].VPSSize]);         //sps addr
			//pVidStrmBuf->Resv[1]  = 28;                                                         //sps size
			pVidStrmBuf->Resv[1]  = sEthCamTxRecInfo[path_id].SPSSize;                         //sps size
			//pVidStrmBuf->Resv[2]  = (UINT32)&(SPS_Addr[28]);                                    //pps addr
			pVidStrmBuf->Resv[2]  = (UINT32)&(sEthCamTxRecInfo[path_id].Desc[sEthCamTxRecInfo[path_id].VPSSize+sEthCamTxRecInfo[path_id].SPSSize]);  //pps addr
			//pVidStrmBuf->Resv[3]  = 8;
			pVidStrmBuf->Resv[3]  = sEthCamTxRecInfo[path_id].PPSSize;                                                          //pps size
			pVidStrmBuf->Resv[4]  = (UINT32)&(sEthCamTxRecInfo[path_id].Desc[0]);                                         //vps addr
			pVidStrmBuf->Resv[5]  = sEthCamTxRecInfo[path_id].VPSSize;                                                          //vps size
		}

		UINT32 i_per_sec = sEthCamTxRecInfo[path_id].vfr / sEthCamTxRecInfo[path_id].gop;
		if (!i_per_sec) {
			i_per_sec = 1;
		}
		pVidStrmBuf->Resv[6]  = (eth_is_i_frame) ? 3 : 0;                                   //FrameType (IDR = 3, P = 0)
		pVidStrmBuf->Resv[7]  = (((g_SocketCliData1_eth_i_cnt[path_id] - 1) % i_per_sec) == 0 && eth_is_i_frame) ? TRUE : FALSE;    //IsIDR2Cut
		pVidStrmBuf->Resv[8]  = 0;                                                          //SVC size
		pVidStrmBuf->Resv[9]  = 0;                                                          //Temporal Id
		pVidStrmBuf->Resv[10] = (eth_is_i_frame) ? TRUE : FALSE;                            //IsKey
		pVidStrmBuf->Resv[12] = g_SocketCliData1_BsFrameCnt[path_id]-1;//pVdoBs->FrmIdx;
		pVidStrmBuf->Resv[13] = 0;//pVdoBs->bIsEOF;  //have next I Frame ?
		g_SocketCliData1_InIsfData[path_id].TimeStamp = HwClock_GetLongCounter();

		#if (ETH_REARCAM_CLONE_FOR_DISPLAY == DISABLE)
		pDist = ImageUnit_In(&ISF_VdoDec, ImageApp_MovieMulti_GetEthCamVdoDecInPort(_CFG_ETHCAM_ID_1 + path_id));
		if (ImageUnit_IsAllowPush(pDist)) {
			if(sEthCamTxRecInfo[path_id].codec == MEDIAVIDENC_H265 && eth_is_i_frame){
				pVidStrmBuf->DataAddr = (UINT32)(addr+sEthCamTxRecInfo[path_id].VPSSize+sEthCamTxRecInfo[path_id].SPSSize+sEthCamTxRecInfo[path_id].PPSSize);//uiBsFrmBufAddr;//g_EthBsFrmAddr[g_queue_idx-1];//pVdoBs->BsAddr;
				pVidStrmBuf->DataSize = (UINT32)(size-LongCounterSizeOffset-(sEthCamTxRecInfo[path_id].VPSSize+sEthCamTxRecInfo[path_id].SPSSize+sEthCamTxRecInfo[path_id].PPSSize));//g_SocketCliData1_BsBufMapTbl[g_SocketCliData1_BsQueueIdx];//g_CliSocket_size_frm[g_queue_idx-1];//pVdoBs->BsSize;
			}else if(sEthCamTxRecInfo[path_id].codec == MEDIAVIDENC_H264 && eth_is_i_frame){
				pVidStrmBuf->DataAddr = (UINT32)(addr+sEthCamTxRecInfo[path_id].SPSSize+sEthCamTxRecInfo[path_id].PPSSize);//uiBsFrmBufAddr;//g_EthBsFrmAddr[g_queue_idx-1];//pVdoBs->BsAddr;
				pVidStrmBuf->DataSize = (UINT32)(size-LongCounterSizeOffset-(sEthCamTxRecInfo[path_id].SPSSize+sEthCamTxRecInfo[path_id].PPSSize));//g_SocketCliData1_BsBufMapTbl[g_SocketCliData1_BsQueueIdx];//g_CliSocket_size_frm[g_queue_idx-1];//pVdoBs->BsSize;
			}
			ImageUnit_PushData(pDist, &(g_SocketCliData1_InIsfData[path_id]), 0);
		}
		#endif
		pDist = ImageUnit_In(&ISF_Demux, ImageApp_MovieMulti_GetEthCamDemuxInPort(_CFG_ETHCAM_ID_1 + path_id));
		if (ImageUnit_IsAllowPush(pDist)) {
			if(sEthCamTxRecInfo[path_id].codec == MEDIAVIDENC_H265 && eth_is_i_frame){
				pVidStrmBuf->DataAddr = (UINT32)addr;
				pVidStrmBuf->DataSize = (UINT32)(size-LongCounterSizeOffset);
			}else if(sEthCamTxRecInfo[path_id].codec == MEDIAVIDENC_H264 && eth_is_i_frame){

				if(sEthCamTxRecInfo[path_id].rec_format == _CFG_FILE_FORMAT_TS){
					memcpy(addr, sEthCamTxRecInfo[path_id].Desc, sEthCamTxRecInfo[path_id].DescSize);
					pVidStrmBuf->DataAddr = (UINT32)addr;
					pVidStrmBuf->DataSize = (UINT32)(size-LongCounterSizeOffset);
				}else{
					pVidStrmBuf->DataAddr = (UINT32)(addr+sEthCamTxRecInfo[path_id].SPSSize+sEthCamTxRecInfo[path_id].PPSSize);
					pVidStrmBuf->DataSize = (UINT32)(size-LongCounterSizeOffset-(sEthCamTxRecInfo[path_id].SPSSize+sEthCamTxRecInfo[path_id].PPSSize));
				}
			}
			ImageUnit_PushData(pDist, &(g_SocketCliData1_InIsfData[path_id]), 0);
		}
	#endif

		if(socketCliEthData1_IsRecv(path_id)==0)
		{
			socketCliEthData1_SetRecv(path_id, 1);
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == DISABLE)
			#if(ETH_REARCAM_CAPS_COUNT >=2)
			UI_SetData(FL_DUAL_CAM, DUALCAM_BOTH);
			#else
			UI_SetData(FL_DUAL_CAM, DUALCAM_BEHIND);
			#endif
			MovieExe_EthCam_ChgDispCB(UI_GetData(FL_DUAL_CAM));
#endif
		}
		EthCamHB1[path_id] = 0;
	} else if (bPushData == 2 || bPushData == 3) {
	#if 1
		ISF_PORT *pDist;
		ISF_VIDEO_STREAM_BUF *pVidStrmBuf;
		g_SocketCliData1_InIsfData[path_id] = gISF_EthCam_Pull_InData1;

		pVidStrmBuf = (ISF_VIDEO_STREAM_BUF *)&g_SocketCliData1_InIsfData[path_id].Desc;
		if (bPushData == 2) {
			pVidStrmBuf->flag = MAKEFOURCC('T', 'H', 'U', 'M');
		} else {
			pVidStrmBuf->flag = MAKEFOURCC('J', 'S', 'T', 'M');
		}
		pVidStrmBuf->DataAddr = (UINT32)&addr[5];
		pVidStrmBuf->DataSize = (UINT32)(size-5);
		pVidStrmBuf->Resv[0]  = ImageApp_MovieMulti_Recid2BsPort(_CFG_ETHCAM_ID_1 + path_id);                //user_id
		g_SocketCliData1_InIsfData[path_id].TimeStamp = HwClock_GetLongCounter();

		#if (ETH_REARCAM_CLONE_FOR_DISPLAY == DISABLE)
		pDist = ImageUnit_In(&ISF_VdoDec, ImageApp_MovieMulti_GetEthCamVdoDecInPort(_CFG_ETHCAM_ID_1 + path_id));
		if (ImageUnit_IsAllowPush(pDist)) {
			//DBG_DUMP("Push\r\n");
			ImageUnit_PushData(pDist, &(g_SocketCliData1_InIsfData[path_id]), 0);
		}
		#endif
		pDist = ImageUnit_In(&ISF_Demux, ImageApp_MovieMulti_GetEthCamDemuxInPort(_CFG_ETHCAM_ID_1 + path_id));
		if (ImageUnit_IsAllowPush(pDist)) {
			//DBG_DUMP("Push thumb\r\n");
			ImageUnit_PushData(pDist, &(g_SocketCliData1_InIsfData[path_id]), 0);
		}
	#endif
	}
#endif
}
void socketCliEthData1_NotifyCB(ETHCAM_PATH_ID path_id, int status, int parm)
{
	switch (status) {
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_REQUEST: {
			//DBG_DUMP("Notify EthCli REQUEST\r\n");
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_CONNECT: {
			DBG_DUMP("[%d]socketCliEthData1_NotifyCB connect OK\r\n",path_id);
			MovieExe_EthCamMetaInit(path_id);
			g_IsSocketCliData1_Conn[path_id]=1;
#if ((ETH_REARCAM_CLONE_FOR_DISPLAY == DISABLE) && (ETH_REARCAM_CAPS_COUNT==1))
			ImageApp_MovieMulti_EthCamLinkForDisp(_CFG_ETHCAM_ID_1+path_id, ENABLE, TRUE);
			EthCam_SendXMLCmd(path_id, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_TX_STREAM_START, 0);
#endif

			if (System_IsModeChgClose()==0 && System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_MOVIE) {
				Ux_PostEvent(NVTEVT_EXE_MOVIE_ETHCAMHOTPLUG, 1, path_id);
			}
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_DISCONNECT: {
			DBG_ERR("[%d]disconnect!!!\r\n",path_id);
			g_IsSocketCliData1_Conn[path_id]=0;
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_SLOW: {
			DBG_ERR("[%d]Error!!!parm=%d\r\n",path_id,parm);
			if(ImageApp_MovieMulti_IsStreamRunning(_CFG_ETHCAM_ID_1)
			#if(ETH_REARCAM_CAPS_COUNT >=2)
				|| ImageApp_MovieMulti_IsStreamRunning(_CFG_ETHCAM_ID_2)
			#endif
			){
				DBG_ERR("[%d]STOPREC!!!parm=%d\r\n",path_id,parm);
				//Ux_PostEvent(NVTEVT_CB_MOVIE_SLOW, 0);
				BKG_PostEvent(NVTEVT_BKW_STOPREC_PROCESS);
			}
			if(path_id>=ETHCAM_PATH_ID_1 && path_id< ETHCAM_PATH_ID_MAX){
				AppBKW_SetData(BKW_ETHCAM_DEC_ERR_PATHID, path_id);
				BKG_PostEvent(NVTEVT_BKW_ETHCAM_DEC_ERR);

				EthCam_SendXMLCmd(path_id, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_DUMP_TX_BS_INFO, 0);
			}
			break;
		}
	}
}
void socketCliEthData1_RecvResetParam(ETHCAM_PATH_ID path_id)
{
	g_SocketCliData1_BsFrameCnt[path_id]=0;
	g_SocketCliData1_RecBsFrameCnt[path_id]=0;
	g_SocketCliData1_eth_i_cnt[path_id]=0;
	g_SocketCliData1_LongCntTxRxOffset[path_id]=0;
	g_bsocketCliEthData1_AllowPull[path_id]=1;
}


#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
BOOL g_bsocketCliEthData2_AllowPush[ETHCAM_PATH_ID_MAX]={1, 1};
BOOL g_bsocketCliEthData2_ResetI[ETHCAM_PATH_ID_MAX]={0};
void  socketCliEthData2_SetResetI(ETHCAM_PATH_ID path_id, UINT32 Data)
{
	g_bsocketCliEthData2_ResetI[path_id]=Data;
}
void  socketCliEthData2_SetAllowPush(ETHCAM_PATH_ID path_id, UINT32 Data)
{
	g_bsocketCliEthData2_AllowPush[path_id]=Data;
}

#endif
void socketCliEthData2_RecvCB(ETHCAM_PATH_ID path_id, char* addr, int size)
{
#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	//static UINT32 eth_i_cnt = 0;
	UINT32 eth_is_i_frame = 0;
	UINT16 bPushData=0;
	UINT32 DescSize=0;
	if(sEthCamTxDecInfo[path_id].Codec == HD_CODEC_TYPE_H264){//MEDIAVIDENC_H264){
		DescSize=sEthCamTxDecInfo[path_id].DescSize;
	}
	//extern UINT8 SPS_Addr[];

	//DBG_DUMP("D2 size=%d, 0x%x\r\n",  size, addr);
	//if(addr % 4){
	//	DBG_DUMP("addr%4!=0\r\n");
	//}
	if((addr[0] ==0 && addr[1] ==0 && addr[2] ==0 && addr[3] ==1)){
		if(((addr[4])& 0x1F) == H264_NALU_TYPE_IDR  && addr[5] == H264_START_CODE_I){
			//DBG_DUMP("64D2_I[%d]=%d,%d\r\n",path_id,g_SocketCliData2_BsFrameCnt[path_id],size);
			if(g_bsocketCliEthData2_AllowPush[path_id]==1){
				g_bsocketCliEthData2_ResetI[path_id]=0;
			}
			g_SocketCliData2_BsFrameCnt[path_id]++;
			eth_is_i_frame = 1;
			g_SocketCliData2_eth_i_cnt[path_id] ++;
			bPushData=1;
			//g_TestH264Addr=(UINT32)addr;
			//g_TestH264Size=(UINT32)size;
			//EthCamCmdRcv_SetFlag();

		}else if(((addr[4])& 0x1F) == H264_NALU_TYPE_SLICE && addr[5] == H264_START_CODE_P){
			//DBG_DUMP("64D2_P[%d]=%d,%d\r\n",path_id,g_SocketCliData2_BsFrameCnt[path_id],size);
			if(g_bsocketCliEthData2_ResetI[path_id]){
				return;
			}
			g_SocketCliData2_BsFrameCnt[path_id]++;
			eth_is_i_frame = 0;
			bPushData=1;
		}else if((((addr[4])>>1)&0x3F) == H265_NALU_TYPE_VPS){
			//DBG_DUMP("65D2_I[%d]=%d,%d\r\n",path_id,g_SocketCliData2_BsFrameCnt[path_id],size);
			if(g_bsocketCliEthData2_AllowPush[path_id]==1){
				g_bsocketCliEthData2_ResetI[path_id]=0;
			}
			g_SocketCliData2_BsFrameCnt[path_id]++;
			eth_is_i_frame = 1;
			g_SocketCliData2_eth_i_cnt[path_id] ++;
			bPushData=1;
			//g_TestH264Addr=(UINT32)addr;
			//g_TestH264Size=(UINT32)size;
			//EthCamCmdRcv_SetFlag();

		}else if((((addr[4])>>1)&0x3F) == H265_NALU_TYPE_SLICE){
			//DBG_DUMP("65D2_P[%d]=%d,%d\r\n",path_id,g_SocketCliData2_BsFrameCnt[path_id],size);
			if(g_bsocketCliEthData2_ResetI[path_id]){
				return;
			}
			g_SocketCliData2_BsFrameCnt[path_id]++;
			eth_is_i_frame = 0;
			bPushData=1;

		}else if(((addr[4])==0xFF)){
			//DBG_DUMP("Thumb OK\r\n");
			bPushData = 2;
			//DBG_DUMP("data[5]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",addr[5],addr[6],addr[size-5],addr[size-4],addr[size-3],addr[size-2],addr[size-1],addr[size]);
		}else if(((addr[4])==0xFE)){
			//DBG_DUMP("PIM OK\r\n");
			bPushData = 3;
			//DBG_DUMP("data[5]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",addr[5],addr[6],addr[size-5],addr[size-4],addr[size-3],addr[size-2],addr[size-1],addr[size]);
		}else{
			//DBG_DUMP("Check FAIL\r\n");
		}
	}else if(addr[0+DescSize] ==0 && addr[1+DescSize] ==0 && addr[2+DescSize] ==0 && addr[3+DescSize] ==1){
		if(((addr[4+DescSize])& 0x1F) == H264_NALU_TYPE_IDR && addr[5+DescSize] == H264_START_CODE_I){
			if(g_bsocketCliEthData2_AllowPush[path_id]==1){
				g_bsocketCliEthData2_ResetI[path_id]=0;
			}
			g_SocketCliData2_BsFrameCnt[path_id]++;
			eth_is_i_frame = 1;
			g_SocketCliData2_eth_i_cnt[path_id] ++;
			bPushData=1;
		}
	}
	//return;
	if(bPushData==1 && g_bsocketCliEthData2_AllowPush[path_id]==1){
		UINT32 LongCounterSizeOffset=LONGCNT_STAMP_OFFSET;
		UINT32 StreamSize = size-LONGCNT_STAMP_OFFSET;
		UINT32 LongConterLow[ETHCAM_PATH_ID_MAX]={0};
		UINT32 LongConterHigh[ETHCAM_PATH_ID_MAX]={0};
		UINT64 LongConter[ETHCAM_PATH_ID_MAX]={0};
		static UINT32 LongCntTxRxOffset[ETHCAM_PATH_ID_MAX]={0};
		//DBG_DUMP("D2longcnt=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",addr[StreamSize+0],addr[StreamSize+1],addr[StreamSize+2],addr[StreamSize+3],addr[StreamSize+4],addr[StreamSize+5],addr[StreamSize+6],addr[StreamSize+7],addr[StreamSize+8],addr[StreamSize+9],addr[StreamSize+10],addr[StreamSize+11],addr[StreamSize+12]);
		if(addr[StreamSize+0] ==0 && addr[StreamSize+1] ==0 && addr[StreamSize+2] ==0 && addr[StreamSize+3] ==1 && addr[StreamSize+4]==HEAD_TYPE_LONGCNT_STAMP){
			LongConterHigh[path_id]=(addr[StreamSize+5]<<24)+ (addr[StreamSize+6]<<16)+ (addr[StreamSize+7]<<8) + (addr[StreamSize+8]);
			LongConterLow[path_id]=(addr[StreamSize+9]<<24)+ (addr[StreamSize+10]<<16)+ (addr[StreamSize+11]<<8) + (addr[StreamSize+12]);
			LongConter[path_id]=(((UINT64)LongConterHigh[path_id]<<32) | LongConterLow[path_id]);
			if(g_SocketCliData2_BsFrameCnt[path_id]> 10 && g_SocketCliData2_BsFrameCnt[path_id] <= 60){
				LongCntTxRxOffset[path_id]+=HwClock_DiffLongCounter(LongConter[path_id], HwClock_GetLongCounter());
				if(g_SocketCliData2_BsFrameCnt[path_id] == 60){
					g_SocketCliData2_LongCntTxRxOffset[path_id]=LongCntTxRxOffset[path_id]/(60-10);
				}
			}else{
				LongCntTxRxOffset[path_id] =0;
			}
			//DBG_DUMP("D2_LongCntHigh[%d]=(%d,%d), %d, (%d,%d), diff=%d, %d\r\n",path_id,LongConterHigh[path_id],LongConterLow[path_id],(UINT32)LongConter[path_id],(UINT32)((HwClock_GetLongCounter() >> 32) & 0xFFFFFFFF),(UINT32)(HwClock_GetLongCounter() & 0xFFFFFFFF), (UINT32)HwClock_DiffLongCounter(LongConter[path_id], HwClock_GetLongCounter())/1000,(UINT32)g_SocketCliData2_LongCntTxRxOffset[path_id]/1000);
		}else{
			LongCounterSizeOffset=0;
		}





		UINT32 g_addrPa = 0;
		UINT32 g_addrVa = 0;
		if(socketCliEthData2_IsRecv(path_id)==0)
		{
			socketCliEthData2_SetRecv(path_id, 1);
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			#if((ETH_REARCAM_CAPS_COUNT+SENSOR_CAPS_COUNT)  >2)
			UI_SetData(FL_DUAL_CAM, DUALCAM_BOTH);
			//UI_SetData(FL_DUAL_CAM_MENU, DUALCAM_BOTH);
			#else
			UI_SetData(FL_DUAL_CAM, DUALCAM_BEHIND);
			//UI_SetData(FL_DUAL_CAM_MENU, DUALCAM_BEHIND);
			#endif
			MovieExe_EthCam_ChgDispCB(UI_GetData(FL_DUAL_CAM));
#endif
		}

		g_addrVa=(UINT32)addr;
		g_addrPa=g_SocketCliData2_BsPoolAddr[path_id][0].pool_pa+ g_addrVa-g_SocketCliData2_BsPoolAddr[path_id][0].pool_va;

		HD_VIDEODEC_BS vdec_bs = {
			.sign          = MAKEFOURCC('V','S','T','M'),
			.p_next        = NULL,
			.vcodec_format =  sEthCamTxDecInfo[path_id].Codec ,//(UI_GetData(FL_MOVIE_CODEC) == MOVIE_CODEC_H265) ? HD_CODEC_TYPE_H265:HD_CODEC_TYPE_H264 ,//HD_CODEC_TYPE_H264,
			.count         = 0,
			.timestamp     = hd_gettime_us(),
			.ddr_id        = DDR_ID0,
			.phy_addr      = (g_addrPa+sEthCamTxDecInfo[path_id].VPSSize+sEthCamTxDecInfo[path_id].SPSSize+sEthCamTxDecInfo[path_id].PPSSize),//(UINT32)addr,//ethbuf_bs_pa[buf_idx] + header_offset,
			.size          = (size-LongCounterSizeOffset-(sEthCamTxDecInfo[path_id].VPSSize+sEthCamTxDecInfo[path_id].SPSSize+sEthCamTxDecInfo[path_id].PPSSize)),
			.blk           = -2,
		};
		if (eth_is_i_frame==0) {    // I frame
			vdec_bs.phy_addr=g_addrPa;
			vdec_bs.size=(size-LongCounterSizeOffset);
		}

		if(ImageApp_MovieMulti_EthCamLinkForDispStatus(_CFG_ETHCAM_ID_1+ path_id)){ //VdoDec is running
			ImageApp_MovieMulti_EthCamLinkPushVdoBsToDecoder(_CFG_ETHCAM_ID_1+ path_id, &vdec_bs);
		}
		EthCamHB2 = 0;
	}
#if 0
	if(bPushData==1 && g_bsocketCliEthData2_AllowPush[path_id]==1){
		if(socketCliEthData2_IsRecv(path_id)==0)
		{
			socketCliEthData2_SetRecv(path_id, 1);
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			#if((ETH_REARCAM_CAPS_COUNT+SENSOR_CAPS_COUNT)  >2)
			UI_SetData(FL_DUAL_CAM, DUALCAM_BOTH);
			#else
			UI_SetData(FL_DUAL_CAM, DUALCAM_BEHIND);
			#endif
			MovieExe_EthCam_ChgDispCB(UI_GetData(FL_DUAL_CAM));
#endif
		}

		//push data to VdoDec
	#if 1
		ISF_PORT         *pDist;
		ISF_VIDEO_STREAM_BUF    *pVidStrmBuf;

		UINT32 LongCounterSizeOffset=LONGCNT_STAMP_OFFSET;
		UINT32 StreamSize = size-LONGCNT_STAMP_OFFSET;
		UINT32 LongConterLow[ETHCAM_PATH_ID_MAX]={0};
		UINT32 LongConterHigh[ETHCAM_PATH_ID_MAX]={0};
		UINT64 LongConter[ETHCAM_PATH_ID_MAX]={0};
		static UINT32 LongCntTxRxOffset[ETHCAM_PATH_ID_MAX]={0};
		//DBG_DUMP("D2longcnt=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",addr[StreamSize+0],addr[StreamSize+1],addr[StreamSize+2],addr[StreamSize+3],addr[StreamSize+4],addr[StreamSize+5],addr[StreamSize+6],addr[StreamSize+7],addr[StreamSize+8],addr[StreamSize+9],addr[StreamSize+10],addr[StreamSize+11],addr[StreamSize+12]);
		if(addr[StreamSize+0] ==0 && addr[StreamSize+1] ==0 && addr[StreamSize+2] ==0 && addr[StreamSize+3] ==1 && addr[StreamSize+4]==HEAD_TYPE_LONGCNT_STAMP){
			LongConterHigh[path_id]=(addr[StreamSize+5]<<24)+ (addr[StreamSize+6]<<16)+ (addr[StreamSize+7]<<8) + (addr[StreamSize+8]);
			LongConterLow[path_id]=(addr[StreamSize+9]<<24)+ (addr[StreamSize+10]<<16)+ (addr[StreamSize+11]<<8) + (addr[StreamSize+12]);
			LongConter[path_id]=(((UINT64)LongConterHigh[path_id]<<32) | LongConterLow[path_id]);
			if(g_SocketCliData2_BsFrameCnt[path_id]> 10 && g_SocketCliData2_BsFrameCnt[path_id] <= 60){
				LongCntTxRxOffset[path_id]+=HwClock_DiffLongCounter(LongConter[path_id], HwClock_GetLongCounter());
				if(g_SocketCliData2_BsFrameCnt[path_id] == 60){
					g_SocketCliData2_LongCntTxRxOffset[path_id]=LongCntTxRxOffset[path_id]/(60-10);
				}
			}else{
				LongCntTxRxOffset[path_id] =0;
			}
			//DBG_DUMP("D2_LongCntHigh[%d]=(%d,%d), %d, (%d,%d), diff=%d, %d\r\n",path_id,LongConterHigh[path_id],LongConterLow[path_id],(UINT32)LongConter[path_id],(UINT32)((HwClock_GetLongCounter() >> 32) & 0xFFFFFFFF),(UINT32)(HwClock_GetLongCounter() & 0xFFFFFFFF), (UINT32)HwClock_DiffLongCounter(LongConter[path_id], HwClock_GetLongCounter())/1000,(UINT32)g_SocketCliData2_LongCntTxRxOffset[path_id]/1000);
		}else{
			LongCounterSizeOffset=0;
		}

		g_SocketCliData2_InIsfData = gISF_EthCam_Pull_InData2;

		pVidStrmBuf = (ISF_VIDEO_STREAM_BUF *)&g_SocketCliData2_InIsfData.Desc;
		pVidStrmBuf->flag     = MAKEFOURCC('V', 'S', 'T', 'M');
		//pVidStrmBuf->DataAddr = (UINT32)pCliSocket_H264_data[g_queue_idx-1];//pVdoBs->BsAddr;
		if(sEthCamTxDecInfo[path_id].Codec == MEDIAVIDENC_H265 && eth_is_i_frame){
			pVidStrmBuf->DataAddr = (UINT32)(addr+sEthCamTxDecInfo[path_id].VPSSize+sEthCamTxDecInfo[path_id].SPSSize+sEthCamTxDecInfo[path_id].PPSSize);//uiBsFrmBufAddr;//g_EthBsFrmAddr[g_queue_idx-1];//pVdoBs->BsAddr;
			pVidStrmBuf->DataSize = (UINT32)(size-LongCounterSizeOffset-(sEthCamTxDecInfo[path_id].VPSSize+sEthCamTxDecInfo[path_id].SPSSize+sEthCamTxDecInfo[path_id].PPSSize));//g_SocketCliData1_BsBufMapTbl[g_SocketCliData1_BsQueueIdx];//g_CliSocket_size_frm[g_queue_idx-1];//pVdoBs->BsSize;
		}else{
			pVidStrmBuf->DataAddr = (UINT32)addr;//uiBsFrmBufAddr;//g_EthBsFrmAddr[g_queue_idx-1];//pVdoBs->BsAddr;
			pVidStrmBuf->DataSize = (UINT32)(size-LongCounterSizeOffset);//g_SocketCliData1_BsBufMapTbl[g_SocketCliData1_BsQueueIdx];//g_CliSocket_size_frm[g_queue_idx-1];//pVdoBs->BsSize;
		}
		//pVidStrmBuf->CodecType = MEDIAVIDENC_H264;                                          //codec type
		pVidStrmBuf->CodecType = sEthCamTxDecInfo[path_id].Codec;                                          //codec type
		//pVidStrmBuf->Resv[0]  = (UINT32)&(SPS_Addr[0]);                                     //sps addr
		if(sEthCamTxDecInfo[path_id].Codec == MEDIAVIDENC_H264){
			pVidStrmBuf->Resv[0]  = (UINT32)&(sEthCamTxDecInfo[path_id].Desc[0]);         //sps addr
			//pVidStrmBuf->Resv[1]  = 28;                                                         //sps size
			pVidStrmBuf->Resv[1]  = sEthCamTxDecInfo[path_id].SPSSize;                         //sps size
			//pVidStrmBuf->Resv[2]  = (UINT32)&(SPS_Addr[28]);                                    //pps addr
			pVidStrmBuf->Resv[2]  = (UINT32)&(sEthCamTxDecInfo[path_id].Desc[sEthCamTxDecInfo[path_id].SPSSize]);  //pps addr
			//pVidStrmBuf->Resv[3]  = 8;
			pVidStrmBuf->Resv[3]  = sEthCamTxDecInfo[path_id].PPSSize;                                                          //pps size
			pVidStrmBuf->Resv[4]  = 0;                                                          //vps addr
			pVidStrmBuf->Resv[5]  = 0 ;                                                          //vps size
		}else{
			pVidStrmBuf->Resv[0]  = (UINT32)&(sEthCamTxDecInfo[path_id].Desc[sEthCamTxDecInfo[path_id].VPSSize]);         //sps addr
			//pVidStrmBuf->Resv[1]  = 28;                                                         //sps size
			pVidStrmBuf->Resv[1]  = sEthCamTxDecInfo[path_id].SPSSize;                         //sps size
			//pVidStrmBuf->Resv[2]  = (UINT32)&(SPS_Addr[28]);                                    //pps addr
			pVidStrmBuf->Resv[2]  = (UINT32)&(sEthCamTxDecInfo[path_id].Desc[sEthCamTxDecInfo[path_id].VPSSize+sEthCamTxDecInfo[path_id].SPSSize]);  //pps addr
			//pVidStrmBuf->Resv[3]  = 8;
			pVidStrmBuf->Resv[3]  = sEthCamTxDecInfo[path_id].PPSSize;                                                          //pps size
			pVidStrmBuf->Resv[4]  = (UINT32)&(sEthCamTxDecInfo[path_id].Desc[0]);                                                          //vps addr
			pVidStrmBuf->Resv[5]  = sEthCamTxDecInfo[path_id].VPSSize;                                                          //vps size
		}
		pVidStrmBuf->Resv[6]  = (eth_is_i_frame) ? 3 : 0;                                   //FrameType (IDR = 3, P = 0)
		pVidStrmBuf->Resv[7]  = ((g_SocketCliData2_eth_i_cnt[path_id] % 4) == 0 && eth_is_i_frame) ? TRUE : FALSE;    //IsIDR2Cut
		pVidStrmBuf->Resv[8]  = 0;                                                          //SVC size
		pVidStrmBuf->Resv[9]  = 0;                                                          //Temporal Id
		pVidStrmBuf->Resv[10] = (eth_is_i_frame) ? TRUE : FALSE;                            //IsKey
		pVidStrmBuf->Resv[12] = g_SocketCliData2_BsFrameCnt[path_id]-1;//pVdoBs->FrmIdx;
		pVidStrmBuf->Resv[13] = 0;//pVdoBs->bIsEOF;  //have next I Frame ?
		g_SocketCliData2_InIsfData.TimeStamp = HwClock_GetLongCounter();
		pDist = ImageUnit_In(&ISF_VdoDec, ImageApp_MovieMulti_GetEthCamVdoDecInPort(_CFG_ETHCAM_ID_1 + path_id));
		if (ImageUnit_IsAllowPush(pDist)) {
			//DBG_DUMP("Push\r\n");
			ImageUnit_PushData(pDist, &g_SocketCliData2_InIsfData, 0);
		}
	#endif
		EthCamHB2 = 0;
	} else if (bPushData == 2 || bPushData == 3) {
	#if 0
		ISF_PORT *pDist;
		ISF_VIDEO_STREAM_BUF *pVidStrmBuf;
		g_SocketCliData2_InIsfData = gISF_EthCam_Pull_InData1;

		pVidStrmBuf = (ISF_VIDEO_STREAM_BUF *)&g_SocketCliData2_InIsfData.Desc;
		if (bPushData == 2) {
			pVidStrmBuf->flag = MAKEFOURCC('T', 'H', 'U', 'M');
		} else {
			pVidStrmBuf->flag = MAKEFOURCC('J', 'S', 'T', 'M');
		}
		pVidStrmBuf->DataAddr = (UINT32)addr;//g_SocketCliData1_RawEncodeAddr;
		pVidStrmBuf->DataSize = (UINT32)size;//g_SocketCliData1_RawEncodeSize;
		pVidStrmBuf->Resv[0]  = 4;                                     //user_id
		g_SocketCliData2_InIsfData.TimeStamp = HwClock_GetLongCounter();
		pDist = ImageUnit_In(&ISF_Demux, ImageApp_MovieMulti_GetEthCamDemuxInPort(_CFG_ETHCAM_ID_1 + path_id));
		if (ImageUnit_IsAllowPush(pDist)) {
			//DBG_DUMP("Push thumb\r\n");
			ImageUnit_PushData(pDist, &g_SocketCliData2_InIsfData, 0);
		}
	#endif
	}
#endif
#endif
}
void socketCliEthData2_NotifyCB(ETHCAM_PATH_ID path_id, int status, int parm)
{
	switch (status) {
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_REQUEST: {
			//DBG_DUMP("Notify EthCli REQUEST\r\n");
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_CONNECT: {
			DBG_DUMP("[%d]Notify EthCamSocketCli_Data2 connect OK\r\n",path_id);
			g_IsSocketCliData2_Conn[path_id]=1;
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE && ETH_REARCAM_CAPS_COUNT ==1)
			ImageApp_MovieMulti_EthCamLinkForDisp(_CFG_ETHCAM_ID_1+path_id, ENABLE, TRUE);
			EthCam_SendXMLCmd(path_id, ETHCAM_PORT_DATA2 ,ETHCAM_CMD_TX_STREAM_START, 0);
#endif
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_DISCONNECT: {
			DBG_ERR("[%d]disconnect!!!\r\n",path_id);
			g_IsSocketCliData2_Conn[path_id]=0;
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_SLOW: {
			DBG_ERR("[%d]Error!!!parm=%d\r\n",path_id,parm);
			if(path_id>=ETHCAM_PATH_ID_1 && path_id< ETHCAM_PATH_ID_MAX){
				//EthCam_SendXMLCmd(path_id, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_DUMP_TX_BS_INFO, 0);
			}
			break;
		}
	}
}
void socketCliEthData2_RecvResetParam(ETHCAM_PATH_ID path_id)
{
#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	g_SocketCliData2_BsFrameCnt[path_id]=0;
	g_SocketCliData2_eth_i_cnt[path_id]=0;
	g_bsocketCliEthData2_AllowPush[path_id]=1;
	g_bsocketCliEthData2_ResetI[path_id]=0;
	g_SocketCliData2_LongCntTxRxOffset[path_id]=0;
#endif
}


#endif


static  CHAR g_SocketCmd_XMLCmdAddr[ETHCAM_PATH_ID_MAX][2048];
#if (defined(_NVT_ETHREARCAM_RX_))
static  CHAR g_SocketCmd_ParserBuf[ETHCAM_PATH_ID_MAX][2048] = {0};
static  UINT32 g_SocketCmd_ResultType[ETHCAM_PATH_ID_MAX]={0};
static  INT32 g_SocketCmd_CmdRet[ETHCAM_PATH_ID_MAX]={0};
static  UINT32 g_SocketCmd_RecvSize[ETHCAM_PATH_ID_MAX]={0};
static  INT32 g_SocketCmd_Cmd[ETHCAM_PATH_ID_MAX]={0};
static  ETHCAM_XML_CB_REGISTER g_SocketCmd_OutputCb[ETHCAM_PATH_ID_MAX]={0};
extern INT32 g_SocketCmd_Status[ETHCAM_PATH_ID_MAX];
#endif
#if (defined(_NVT_ETHREARCAM_TX_))
static  INT32 g_SocketCmd_CmdRet=0 ;
#endif
void socketEthCmd_NotifyCB(int status, int parm)
{
    if(status == CYG_ETHSOCKET_STATUS_CLIENT_CONNECT){
        DBG_DUMP("socketEthCmd_NotifyCB Connect OK\r\n");
		EthCamSocket_SetEthHubLinkSta(0, 1);
		if(EthCamNet_IsSocketOpenBKG()==0){
			if((BKG_GetExecFuncTable() && BKG_GetExecFuncTable()->event==0)){
				DBG_DUMP("BKG NOT READY!\r\n");
				set_flg(ETHCAM_CMD_RCV_FLG_ID,FLG_ETHCAM_CMD_INIT_SOCKET);
			}else{
				BKG_PostEvent(NVTEVT_BKW_ETHCAM_SOCKET_OPEN);
			}
		}
	}
    else if(status == CYG_ETHSOCKET_STATUS_CLIENT_REQUEST){
    }
    else if(status == CYG_ETHSOCKET_STATUS_CLIENT_DISCONNECT){
        DBG_DUMP("socketEthCmd_NotifyCB DISConnect !!\r\n");
#if(defined(_NVT_ETHREARCAM_TX_))
        g_IsSocketCmdOpen=0;
#endif
	}
    else{
        DBG_ERR("^GUnknown status = %d, parm = %d\r\n", status, parm);
    }
}
void socketEthCmd_RecvCB(char* addr, int size)
{
	socketCliEthCmd_RecvCB(0, addr, size);
}
void socketCliEthCmd_RecvCB(ETHCAM_PATH_ID path_id, char* addr, int size)
{
#if(defined(_NVT_ETHREARCAM_RX_))
	if(addr && (size>0))
	{
		addr[size]='\0';
		//DBG_DUMP("path_id=%d, size=%d, addr[0]=%s\r\n", path_id, size, addr);
#if 1
		char   *pch;
		//UINT32 cmd = 0;
		char mimeType[64];//just a dummy buffer for using EthCamCmd_GetData
		//INT32 EthCamCmdRet;
		UINT32 segmentCount = 0;
		UINT32 uiSendBufSize;
		static BOOL bFirst=1;

		char dest_ip[16]={0};
		UINT32 dest_port = 80;
		char cdest_port[5]={0};
		UINT32 CmdBufShiftSize;
		UINT32 i=0;
		if(g_SocketCmd_ResultType[path_id]==ETHCAM_XML_RESULT_TYPE_LIST && g_SocketCmd_CmdRet[path_id]==ETHCAM_RET_CONTINUE){
			if(memcmp((addr+size-1-strlen("</LIST>")), "</LIST>",strlen("</LIST>"))==0){
				//DBG_DUMP("CONTINUE XMLCmdAddr=%s\r\n",g_SocketCmd_XMLCmdAddr[path_id]);

				memcpy(g_SocketCmd_XMLCmdAddr[path_id]+g_SocketCmd_RecvSize[path_id], addr, size);
				//DBG_DUMP("CONTINUE size=%d, XMLCmdAddr=%s\r\n",size,g_SocketCmd_XMLCmdAddr[path_id]+g_SocketCmd_RecvSize[path_id]);

				g_SocketCmd_CmdRet[path_id]=EthCamCmdXML_parser(g_SocketCmd_Cmd[path_id],g_SocketCmd_XMLCmdAddr[path_id],&g_SocketCmd_OutputCb[path_id]);
				if(g_SocketCmd_CmdRet[path_id]==0){
					EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_OK);
				}else{
					DBG_ERR("EthCam Result error, EthCamCmdRet[%d]=%d\r\n",path_id,g_SocketCmd_CmdRet[path_id]);
					EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_ERR);
				}
			}else{
				DBG_ERR("cmd[%d]=%d  list result error!\r\n",path_id, g_SocketCmd_Cmd[path_id]);
				EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_ERR);
			}
			return;
		}
		//memcpy(g_SocketCmd_RecvAddr, addr, size);
		//192.168.0.12:8899<?xml version="1.0" encoding="UTF-8" ?>
		//sscanf(addr, "%15[^:]:%99d<%2046[^]", dest_ip, &dest_port, (g_SocketCmd_XMLCmdAddr+1));
		//g_SocketCmd_XMLCmdAddr[0]='<';
		sscanf(addr, "%15[^:]:%99d", dest_ip, &dest_port);
		//DBG_DUMP("addr=%s\r\n",  addr);
		if(dest_port !=SocketInfo[0].port[0] && dest_port !=SocketInfo[0].port[1] && dest_port !=SocketInfo[0].port[2]){
			DBG_ERR("[%d]port error! %d, sz=%d\r\n", path_id, cdest_port, size);
			return;
		}

		sprintf(cdest_port,"%d",dest_port);
		//DBG_DUMP("cdest_port=%s\r\n",  cdest_port);
		//DBG_DUMP("ip=%s, strlen(ip)=%d, port=%s\r\n",  dest_ip,strlen(dest_ip), cdest_port);
		CmdBufShiftSize=strlen(dest_ip)+ strlen(cdest_port)+1; //+1 for :
		//DBG_DUMP("CmdBufShiftSize=%d\r\n",  CmdBufShiftSize);
		memcpy(g_SocketCmd_XMLCmdAddr[path_id], addr+CmdBufShiftSize, size);
		//DBG_DUMP("dest_ip=%s, dest_port=%d, XMLcmd=%s\r\n",  dest_ip, dest_port, g_SocketCmd_XMLCmdAddr);
		//DBG_ERR("addr=%s, size=%d\r\n",  addr, size);

		//char pbuf_size[] = "TEST cmd ack";
		//UINT32 bufSize=strlen(pbuf_size)+1;
		//UserSocketCliEthCmd_Send(pbuf_size, (int *)&bufSize);
		if(bFirst){
			EthCamCmd_Init();
			bFirst=0;
		}

		//Get XML Cmd
		if (*g_SocketCmd_XMLCmdAddr[path_id] == ETHCAM_CMD_ROOT) {
			DBG_IND("cmd (%s)\r\n", g_SocketCmd_XMLCmdAddr[path_id]);
			pch = strchr((char *)g_SocketCmd_XMLCmdAddr[path_id], ETHCAM_CMD_CUSTOM_TAG);
			if (pch) {
				if (strncmp(pch + 1, CMD_STR, strlen(CMD_STR)) == 0) {
					sscanf_s(pch + 1 + strlen(CMD_STR), "%d", &g_SocketCmd_Cmd[path_id]);
					DBG_IND("Recv cmd=%d\r\n",g_SocketCmd_Cmd[path_id]);
					//to do
					//specail handle

				}

				*pch = 0;
				uiSendBufSize= sizeof(g_SocketCmd_ParserBuf[path_id]);
				g_SocketCmd_CmdRet[path_id] = EthCamCmd_GetData(path_id, (char *)g_SocketCmd_XMLCmdAddr[path_id], pch + 1, (UINT32)g_SocketCmd_ParserBuf[path_id], &uiSendBufSize, mimeType, segmentCount);
				DBG_IND("EthCamCmdRet[%d]=%d, len=%d(0x%X)\r\n", path_id,g_SocketCmd_CmdRet[path_id], uiSendBufSize, uiSendBufSize);
				if (ETHCAM_CMD_GETDATA_RETURN_OK == g_SocketCmd_CmdRet[path_id]) {
					DBG_IND("XML Return:\r\n%s\r\n", g_SocketCmd_ParserBuf[path_id]);
					//UserSocketCliEthCmd_Send(g_peth_cam_buf, &uiSendBufSize);
				}
			}
		}else{//Get XML Result
			g_SocketCmd_Cmd[path_id] = (INT32)EthCamCmdXML_GetCmdId(g_SocketCmd_XMLCmdAddr[path_id]);
			//DBG_DUMP("\r\ncmd=%d\r\n", g_SocketCmd_Cmd[path_id]);

			if(g_SocketCmd_Cmd[path_id]>0){
				//ETHCAM_XML_CB_REGISTER output_cb={0};
				memset(&g_SocketCmd_OutputCb[path_id],0,sizeof(ETHCAM_XML_CB_REGISTER));
				//UINT32 result_type=0;
				ETHCAM_XML_RESULT *EthCamXMLResultTbl=EthCamCmd_GetResultTable();
				EthCam_GetDest(addr, &g_SocketCmd_OutputCb[path_id].path_id, &g_SocketCmd_OutputCb[path_id].port_type);
				//DBG_DUMP("path_id=%d, port_type=%d\r\n", g_SocketCmd_OutputCb[path_id].path_id, g_SocketCmd_OutputCb[path_id].port_type);

				while (EthCamXMLResultTbl[i].cmd != 0) {
					if (g_SocketCmd_Cmd[path_id] == EthCamXMLResultTbl[i].cmd) {
						g_SocketCmd_OutputCb[path_id].EthCamXML_data_CB=EthCamXMLResultTbl[i].output_cb.EthCamXML_data_CB;
						g_SocketCmd_ResultType[path_id]=EthCamXMLResultTbl[i].result_type;
						break;
					}
					i++;
				}
				if(g_SocketCmd_ResultType[path_id]==0){
					DBG_ERR("result_type error\r\n");
				}
				//DBG_DUMP("result_type[%d]=%d, size=%d, %d, %d, XMLCmdAddr=%s\r\n",path_id,g_SocketCmd_ResultType[path_id],size,CmdBufShiftSize,strlen("</LIST>"),g_SocketCmd_XMLCmdAddr[path_id]+size-1-CmdBufShiftSize-strlen("</LIST>"));

				if(g_SocketCmd_ResultType[path_id]==ETHCAM_XML_RESULT_TYPE_LIST){
					if(memcmp((g_SocketCmd_XMLCmdAddr[path_id]+strlen(DEF_XML_HEAD)), "<Function>",strlen("<Function>"))==0){
				            INT32 ret =0;
				            ETHCAM_XML_CB_REGISTER OutputCb={NULL, g_SocketCmd_OutputCb[path_id].path_id,g_SocketCmd_OutputCb[path_id].port_type};
				            g_SocketCmd_Status[path_id]=0xff;
				            ret =EthCamCmdXML_parsing_default_format((g_SocketCmd_XMLCmdAddr[path_id]+strlen(DEF_XML_HEAD)), &OutputCb);
				            if(ret < 0){
				                DBG_ERR("DEFAULT_FORMAT %d error ret =%d\r\n",g_SocketCmd_Cmd[path_id] ,ret);
				                EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_ERR);
				                return;
				            }else{
							 if(g_SocketCmd_Status[path_id]==ETHCAM_RET_CONTINUE){
								EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_CONTINUE);
								return;
							 }
				            }
					}else if(memcmp((g_SocketCmd_XMLCmdAddr[path_id]+size-1-CmdBufShiftSize-strlen("</LIST>")), "</LIST>",strlen("</LIST>"))!=0){//list data not finish
						g_SocketCmd_RecvSize[path_id]=size-CmdBufShiftSize;
						g_SocketCmd_CmdRet[path_id]=ETHCAM_RET_CONTINUE;
						//EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_CONTINUE);
						return;
					}

				}
				g_SocketCmd_Status[path_id]=0xff;
				g_SocketCmd_CmdRet[path_id]=EthCamCmdXML_parser(g_SocketCmd_Cmd[path_id],g_SocketCmd_XMLCmdAddr[path_id],&g_SocketCmd_OutputCb[path_id]);

				if(g_SocketCmd_CmdRet[path_id]==0){
					//EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_OK);
					if(g_SocketCmd_Status[path_id]==ETHCAM_RET_OK &&
						(g_SocketCmd_ResultType[path_id] == ETHCAM_XML_RESULT_TYPE_VALUE_RESULT || g_SocketCmd_ResultType[path_id] == ETHCAM_XML_RESULT_TYPE_STRING_RESULT)){
						EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_OK);
						sEthCamSendCmdInfo.BufSize= sizeof(sEthCamSendCmdInfo.ParserBuf)- CmdBufShiftSize;
						XML_DefaultFormat(g_SocketCmd_Cmd[path_id], ETHCAM_RET_ACK, (UINT32)sEthCamSendCmdInfo.ParserBuf+CmdBufShiftSize, &sEthCamSendCmdInfo.BufSize, mimeType);
						sEthCamSendCmdInfo.path_id=path_id;
						for(i=0;i<strlen(dest_ip);i++){
							sEthCamSendCmdInfo.ParserBuf[i]=dest_ip[i];
						}
						sEthCamSendCmdInfo.ParserBuf[strlen(dest_ip)]=':';
						for(i=(strlen(dest_ip)+1);i<(strlen(dest_ip)+1+strlen(cdest_port));i++){
							sEthCamSendCmdInfo.ParserBuf[i]=cdest_port[i-(strlen(dest_ip)+1)];
						}
						sEthCamSendCmdInfo.BufSize += CmdBufShiftSize;
						//EthCamCmd_Send(path_id, (char*)sEthCamSendCmdInfo.ParserBuf, (int*)&sEthCamSendCmdInfo.BufSize);
						set_flg(ETHCAM_CMD_SND_FLG_ID,FLG_ETHCAM_CMD_SND);
					}else if(g_SocketCmd_Status[path_id]==ETHCAM_RET_CMD_NOT_FOUND){
						EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_CMD_NOT_FOUND);
					}else{
						EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_OK);
					}
				}else if(g_SocketCmd_CmdRet[path_id]==ETHCAM_RET_CONTINUE){
					EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_CONTINUE);
				}else if(g_SocketCmd_CmdRet[path_id]==ETHCAM_RET_CMD_NOT_FOUND){
					EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_CMD_NOT_FOUND);
				}else{
					EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_ERR);
					DBG_ERR("EthCam Result error, EthCamCmdRet[%d]=%d\r\n",path_id,g_SocketCmd_CmdRet[path_id]);
				}
			}
		}

#endif
	}
#else //===================== TX Start =====================

	if (addr && (size > 0)) {
	//return TRUE;

		addr[size] = '\0';
		//DBG_DUMP("size=%d, addr[0]=%s\r\n", size, addr);

#if 0
		DBG_DUMP("size=%d, addr[0]=0x%x, addr[1]=0x%x\r\n", size, addr[0], addr[1]);
		EthCamCmd_Rcv((ETHCAM_CMD_FMT*)addr);
#endif

		static char   *pch=0;
		static UINT32 cmd = 0;
		static ETHCAM_PATH_ID path_id;
		static ETHCAM_PORT_TYPE port_type;
		char mimeType[64] = {0};
		sprintf(mimeType, "text/xml");

		UINT32 segmentCount = 0;
		//UINT32 uiSendBufSize;
		static BOOL bFirst=1;
		char dest_ip[16]={0};
		UINT32 dest_port = 80;
		char cdest_port[5]={0};
		static UINT32 CmdBufShiftSize=0;
		UINT32 i=0;

		//memcpy(g_SocketCmd_RecvAddr, addr, size);
		if(ETHCAM_CMD_GETDATA_RETURN_CONTINUE == g_SocketCmd_CmdRet
			|| ETHCAM_CMD_GETDATA_RETURN_CONTI_NEED_ACKDATA== g_SocketCmd_CmdRet) {
			if (strncmp(addr, "192.168.", strlen("192.168.")) == 0) {
				//DBG_WRN("CONTINUE cmd=%d, size=%d, addr=%s\r\n",  cmd, size, addr);
				DBG_WRN("CONTINUE cmd=%d, size=%d\r\n",  cmd, size);
				return;
			}
			INT32 CmdRet=g_SocketCmd_CmdRet;
			DBG_IND("CONTINUE cmd=%d, size=%d\r\n",  cmd, size);
			segmentCount=1;
			if(ETHCAM_CMD_GETDATA_RETURN_CONTINUE == g_SocketCmd_CmdRet){
				g_SocketCmd_CmdRet = EthCamCmd_GetData(path_id, (char *)addr, pch + 1, (UINT32)(addr), (UINT32*)&size, mimeType, segmentCount);
			}else{
				memcpy(&sEthCamSendCmdInfo.ParserBuf[CmdBufShiftSize], addr, size);
				sEthCamSendCmdInfo.BufSize= sizeof(sEthCamSendCmdInfo.ParserBuf)- CmdBufShiftSize;
				g_SocketCmd_CmdRet = EthCamCmd_GetData(path_id, (char *)addr, pch + 1, (UINT32)(sEthCamSendCmdInfo.ParserBuf+CmdBufShiftSize), &sEthCamSendCmdInfo.BufSize, mimeType, segmentCount);
			}
			if(g_SocketCmd_CmdRet==ETHCAM_CMD_GETDATA_RETURN_OK){
				if(ETHCAM_CMD_GETDATA_RETURN_CONTINUE == CmdRet){
					EthCam_SendXMLStatusCB(path_id,port_type,cmd, ETHCAM_RET_OK);
				}else{
					sEthCamSendCmdInfo.BufSize+=CmdBufShiftSize;
					DBG_IND("XML Return:\r\n%s, size=%d,%d\r\n", sEthCamSendCmdInfo.ParserBuf, sEthCamSendCmdInfo.BufSize, strlen(sEthCamSendCmdInfo.ParserBuf));
					//EthCamCmd_Send(sEthCamSendCmdInfo.ParserBuf, (int *)&sEthCamSendCmdInfo.BufSize);
					set_flg(ETHCAM_CMD_SND_FLG_ID,FLG_ETHCAM_CMD_SND);
				}
			}else if(g_SocketCmd_CmdRet==ETHCAM_CMD_GETDATA_RETURN_ERROR){
				EthCam_SendXMLStatusCB(path_id,port_type,cmd, ETHCAM_RET_ERR);
			}
			return;
		}
		sscanf(addr, "%15[^:]:%99d", dest_ip, &dest_port);
		sprintf(cdest_port,"%d",dest_port);
		CmdBufShiftSize=strlen(dest_ip)+ strlen(cdest_port)+1; //+1 for :
		memcpy(g_SocketCmd_XMLCmdAddr[0], addr+CmdBufShiftSize, size);
		DBG_IND("dest_ip=%s, dest_port=%d, XMLcmd=%s\r\n",  dest_ip, dest_port, g_SocketCmd_XMLCmdAddr);

		DBG_IND("CmdBufShiftSize=%d\r\n",CmdBufShiftSize);
		EthCam_GetDest(addr, &path_id, &port_type);

		if(bFirst){
			EthCamCmd_Init();
			bFirst=0;
		}

		//Get XML Cmd
		if (*g_SocketCmd_XMLCmdAddr[0] == ETHCAM_CMD_ROOT) {
			//DBG_DUMP("XML cmd=%s\r\n", g_SocketCmd_XMLCmdAddr);
			pch = strchr((char *)g_SocketCmd_XMLCmdAddr[0], ETHCAM_CMD_CUSTOM_TAG);
			if (pch) {
				if (strncmp(pch + 1, CMD_STR, strlen(CMD_STR)) == 0) {
					sscanf_s(pch + 1 + strlen(CMD_STR), "%d", &cmd);
					if(cmd!=ETHCAM_CMD_GET_FRAME){
						DBG_DUMP("cmd Id=%d\r\n",cmd);
					}
					//to do
					//specail handle

				}
				//pch=?custom=1&cmd=9008&par=0
				*pch = 0;

				sEthCamSendCmdInfo.BufSize= sizeof(sEthCamSendCmdInfo.ParserBuf)- CmdBufShiftSize;
				DBG_IND("uiSendBufSize=(%d)\r\n", sEthCamSendCmdInfo.BufSize);
				DBG_IND("strlen(dest_ip)=(%d), strlen(cdest_port)=%d\r\n", strlen(dest_ip),strlen(cdest_port));

				g_SocketCmd_CmdRet = EthCamCmd_GetData(path_id, (char *)addr, pch + 1, (UINT32)(sEthCamSendCmdInfo.ParserBuf+CmdBufShiftSize), &sEthCamSendCmdInfo.BufSize, mimeType, segmentCount);
				DBG_IND("EthCamCmdRet=%d, len=%d\r\n", g_SocketCmd_CmdRet, sEthCamSendCmdInfo.BufSize);
				if (ETHCAM_CMD_GETDATA_RETURN_OK == g_SocketCmd_CmdRet
					|| ETHCAM_CMD_GETDATA_RETURN_CONTINUE == g_SocketCmd_CmdRet
					|| ETHCAM_CMD_GETDATA_RETURN_CONTI_NEED_ACKDATA== g_SocketCmd_CmdRet) {
					for(i=0;i<strlen(dest_ip);i++){
						sEthCamSendCmdInfo.ParserBuf[i]=dest_ip[i];
					}
					sEthCamSendCmdInfo.ParserBuf[strlen(dest_ip)]=':';
					for(i=(strlen(dest_ip)+1);i<(strlen(dest_ip)+1+strlen(cdest_port));i++){
						sEthCamSendCmdInfo.ParserBuf[i]=cdest_port[i-(strlen(dest_ip)+1)];
					//DBG_DUMP("cdest_port[%d]=%x\r\n", i,cdest_port[i]);
					}
					sEthCamSendCmdInfo.BufSize += CmdBufShiftSize;
					DBG_IND("XML Return:\r\n%s\r\n", sEthCamSendCmdInfo.ParserBuf);
					//EthCamCmd_Send(sEthCamSendCmdInfo.ParserBuf, (int *)&sEthCamSendCmdInfo.BufSize);
					set_flg(ETHCAM_CMD_SND_FLG_ID,FLG_ETHCAM_CMD_SND);
				}
			}
		}else{//Get XML Result
			EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_OK);
		}
	}
#endif//===================== TX End =====================
}
void socketCliEthCmd_NotifyCB(ETHCAM_PATH_ID path_id, int status, int parm)
{
	switch (status) {
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_SOCKET_CLOSE: {
			//DBG_ERR("[%d] close!!!\r\n",path_id);
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_REQUEST: {
			//DBG_DUMP("Notify EthCli REQUEST\r\n");
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_CONNECT: {
			DBG_DUMP("socketCliEthCmd_NotifyCB[%d] connect OK\r\n",path_id);
#if(defined(_NVT_ETHREARCAM_RX_))
			UINT32 AllPathLinkStatus=0;
			UINT16 i=0;

			EthCamSocketCli_ReConnect(path_id, 0, 0);

			for(i=0;i<ETH_REARCAM_CAPS_COUNT;i++){
				if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP){
					AllPathLinkStatus++;
				}
			}
			//DBG_DUMP("AllPathLinkStatus=%d\r\n",AllPathLinkStatus);

			if(AllPathLinkStatus==ETH_REARCAM_CAPS_COUNT){
				//SxTimer_SetFuncActive(SX_TIMER_ETHCAM_ETHERNETLINKDET_LINKDET_ID, FALSE);
				//ethsocket_udp_close();
			}

			g_IsSocketCliCmdConn[path_id]=1;

			if(EthCamNet_IsIPConflict()==1){
				DBG_WRN("IPConflict[%d] Reset IP\r\n",path_id);
				EthCam_SendXMLCmd(path_id, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SET_TX_IP_RESET, 0);
				EthCam_SendXMLCmd(path_id, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_TX_POWEROFF, 0);
				break;
			}
			#if (ETH_REARCAM_CAPS_COUNT>=2)
			for (i=0; i<ETH_REARCAM_CAPS_COUNT; i++){
					sEthCamTxSysInfo[i].PullModeEn=1;
			}
			#else
			sEthCamTxSysInfo[0].PullModeEn=0;
			#endif
			#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			sEthCamTxSysInfo[path_id].CloneDisplayPathEn=1;
			#else
			sEthCamTxSysInfo[path_id].CloneDisplayPathEn=0;
			#endif
			sEthCamCodecSrctype[path_id].VCodec=UI_GetData(FL_MOVIE_CODEC);;//MOVIE_CODEC_H264;//UI_GetData(FL_MOVIE_CODEC);
			sEthCamCodecSrctype[path_id].Srctype=ETHCAM_TX_SYS_SRCTYPE_57;
			sEthCamCodecSrctype[path_id].bCmdOK=0;
			DBG_DUMP("[%d]PullModeEn=%d, CloneDisplayPathEn=%d, VCode=%d, Srctype=%d\r\n",path_id, sEthCamTxSysInfo[path_id].PullModeEn,sEthCamTxSysInfo[path_id].CloneDisplayPathEn,sEthCamCodecSrctype[path_id].VCodec,sEthCamCodecSrctype[path_id].Srctype);

			#if (ETHCAM_EIS ==ENABLE)
			if(SysGetFlag(FL_MOVIE_EIS) == EIS_ON ){
				sEthCamTxEISInfo[path_id].Enable=1;
			}else{
				sEthCamTxEISInfo[path_id].Enable=0;
			}
			sEthCamTxEISInfo[path_id].SrcType= ETHCAM_TX_SYS_SRCTYPE_57;
			sEthCamTxEISInfo[path_id].Version=EIS_ETHCAM_VERSION;
			#else
			sEthCamTxEISInfo[path_id].Enable=0;
			#endif
#endif
			EthCamCmdTsk_Open();
			#if 0
			BKG_PostEvent(NVTEVT_BKW_ETHCAM_IPERF_TEST);
			#else
			if ((System_IsModeChgClose()==0 && System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_MOVIE) || System_GetState(SYS_STATE_NEXTMODE) == PRIMARY_MODE_MOVIE) {
					BKG_PostEvent(NVTEVT_BKW_ETHCAM_SET_TX_SYSINFO);

					#if (ETH_REARCAM_CAPS_COUNT>=2)
					BKG_PostEvent(NVTEVT_BKW_GET_ETHCAM_TX_INFO);
					BKG_PostEvent(NVTEVT_BKW_ETHCAM_SOCKETCLI_DATA_OPEN);
					BKG_PostEvent(NVTEVT_BKW_TX_STREAM_START);
					#else
					BKG_PostEvent(NVTEVT_BKW_GET_ETHCAM_TX_RECINFO);
					BKG_PostEvent(NVTEVT_BKW_GET_ETHCAM_TX_DECINFO);
					//BKG_PostEvent(NVTEVT_BKW_ETHCAM_SOCKETCLI_DISP_DATA_OPEN_START);
					BKG_PostEvent(NVTEVT_BKW_ETHCAM_SOCKETCLI_DISP_DATA_OPEN);

					//temp unmark
					//BKG_PostEvent(NVTEVT_BKW_ETHCAM_SYNC_TIME);
					//BKG_PostEvent(NVTEVT_BKW_ETHCAM_SOCKETCLI_REC_DATA_OPEN_START);
					#endif
			}
			#endif
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_DISCONNECT: {
			DBG_ERR("[%d] disconnect!!!\r\n",path_id);
#if(defined(_NVT_ETHREARCAM_RX_))
			g_IsSocketCliCmdConn[path_id]=0;
#endif
			break;
		}
	}
}
#if(defined(_NVT_ETHREARCAM_RX_))
#include "UIWnd/UIFlow.h"
void socketCliEthCmd_SendSizeCB(int size, int total_size)
{
	UINT32 percentage=(size*100/total_size);
	static UINT32 prev_size=0;
	if((prev_size*100/total_size)!= percentage &&  (percentage %10 )==0){
		DBG_DUMP("%d, %d, Send %d Percent\r\n",size ,total_size ,size*100/total_size);
	}
	if (System_GetState(SYS_STATE_NEXTSUBMODE) == SYS_SUBMODE_UPDFW
		&& System_GetState(SYS_STATE_CURRSUBMODE)  == SYS_SUBMODE_UPDFW) {

#if defined(_UI_STYLE_LVGL_)

		char buf[128] = {0};

		snprintf(buf, sizeof(buf), "%s: %d", lv_plugin_get_string(LV_PLUGIN_STRING_ID_STRID_ETHCAM_UDFW_SEND)->ptr, size*100/total_size);

		UIFlowWaitMomentAPI_Set_Text_Param param = {0};
		param.text = buf;
		param.size = strlen(buf);

		lv_user_task_handler_lock();
		UIFlowWaitMomentAPI_Set_Text(param);
		lv_user_task_handler_unlock();

#else
		static char g_StringTmpBuf[64] = {0};
	    	snprintf(g_StringTmpBuf, sizeof(g_StringTmpBuf), "Send FW to Tx: %d",size*100/total_size);
	    	UxState_SetItemData(&UIFlowWndWaitMoment_StatusTXT_MsgCtrl, 0, STATE_ITEM_STRID,  Txt_Pointer(g_StringTmpBuf));
#endif
	}
	prev_size= size;
}
BOOL socketCliEthCmd_IsConn(ETHCAM_PATH_ID path_id)
{
	return g_IsSocketCliCmdConn[path_id];
}
BOOL socketCliEthData1_IsConn(ETHCAM_PATH_ID path_id)
{
	return g_IsSocketCliData1_Conn[path_id];
}
BOOL socketCliEthData2_IsConn(ETHCAM_PATH_ID path_id)
{
	return g_IsSocketCliData2_Conn[path_id];
}
#endif
#endif
