#include <stdio.h>
#include <string.h>
#include "kwrap/type.h"
#include "kwrap/flag.h"

#include "EthCam/EthCamSocket.h"
#include "EthCamSocket_Int.h"
#include "EthCam/EthsockIpcAPI.h"
#include "EthCam/EthsockCliIpcAPI.h"
//#include "Delay.h"
#include "Utility/SwTimer.h"


#define __MODULE__          EthCamSocket
#define __DBGLVL__         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>


//#define ETH_DATA1_SOCKET_PORT  8887
//#define ETH_DATA2_SOCKET_PORT  8888
//#define ETH_CMD_SOCKET_PORT  8899

#define H264_NALU_TYPE_IDR      5
#define H264_NALU_TYPE_SLICE   1
#define H264_NALU_TYPE_SPS     7
#define H264_NALU_TYPE_PPS     8
#define H264_START_CODE_I		0x88
#define H264_START_CODE_P     	0x9A

#define H265_NALU_TYPE_IDR      19
#define H265_NALU_TYPE_SLICE   1
#define H265_NALU_TYPE_VPS     32
#define H265_NALU_TYPE_SPS     33
#define H265_NALU_TYPE_PPS     34

#define SOCKETCLI_RECONNECT_MAX_COUNT 30

#define SOCKETCLI_DATA_PORT_MAX  2

//for ts
//#define TS_VIDEOPES_HEADERLENGTH  14  //(9+PTS 5bytes)
//#define TS_AUDIOPES_HEADERLENGTH  14  //(9+PTS 5bytes)
//#define TS_VIDEO_264_NAL_LENGTH    6
//#define TS_VIDEO_265_NAL_LENGTH    7

#define TS_VIDEO_RESERVED_SIZE 24 //align 4

static int socketcli_reconnect_count[ETHCAM_PATH_ID_MAX][ETHCAM_PORT_DATA_MAX] = {0};
static UINT32 socketcli_reconnect_pathid[ETHCAM_PATH_ID_MAX]={0};
static UINT32 socketcli_reconnect_socketid[ETHCAM_PATH_ID_MAX][ETHCAM_PORT_DATA_MAX]={0};
static SWTIMER_ID  g_EthCamSocketCli_ReConnecTimerId=SWTIMER_NUM;

#if 1//Tx
static int g_ethData1_tx_sockHandle=-1;
static int g_ethData2_tx_sockHandle=-1;
static int g_ethCmd_tx_sockHandle=-1;
static ETHCAMSOCKET_RECV_PROCESS_CB  g_EthCamSocket_Data1RecvCb = NULL;
static ETHCAMSOCKET_RECV_PROCESS_CB  g_EthCamSocket_Data2RecvCb = NULL;
static ETHCAMSOCKET_RECV_PROCESS_CB  g_EthCamSocket_CmdRecvCb = NULL;

static ETHCAMSOCKET_NOTIFY_PROCESS_CB  g_EthCamSocket_Data1NotifyCb = NULL;
static ETHCAMSOCKET_NOTIFY_PROCESS_CB  g_EthCamSocket_Data2NotifyCb = NULL;
static ETHCAMSOCKET_NOTIFY_PROCESS_CB  g_EthCamSocket_CmdNotifyCb = NULL;

static ETHCAM_SOCKET_INFO g_EthCamSocket_Info={0};

#endif

static INT32 g_ethData1_rx_sockCliHandle[ETHCAM_PATH_ID_MAX] = {0};
static INT32 g_ethData2_rx_sockCliHandle[ETHCAM_PATH_ID_MAX] = {0};
static INT32 g_ethCmd_rx_sockCliHandle[ETHCAM_PATH_ID_MAX] = {0};

static UINT32 g_SocketCliData_RawEncodeAddr[ETHCAM_PATH_ID_MAX][SOCKETCLI_DATA_PORT_MAX]={0};
static UINT32 g_SocketCliData_RawEncodeSize[ETHCAM_PATH_ID_MAX][SOCKETCLI_DATA_PORT_MAX]={0};
static UINT32 g_SocketCliData_BsBufTotalAddr[ETHCAM_PATH_ID_MAX][SOCKETCLI_DATA_PORT_MAX]={0}; //tbl Buf+BsFrmBuf
static UINT32 g_SocketCliData_BsBufTotalSize[ETHCAM_PATH_ID_MAX][SOCKETCLI_DATA_PORT_MAX]={0};
static UINT32 g_SocketCliData_BsQueueMax[ETHCAM_PATH_ID_MAX][SOCKETCLI_DATA_PORT_MAX]={0};

static ETHCAMSOCKET_CLI_RECV_PROCESS_CB  g_EthCamSocketCli_Data1RecvCb[ETHCAM_PATH_ID_MAX] = {NULL};
static ETHCAMSOCKET_CLI_RECV_PROCESS_CB  g_EthCamSocketCli_Data2RecvCb[ETHCAM_PATH_ID_MAX] = {NULL};
static ETHCAMSOCKET_CLI_RECV_PROCESS_CB  g_EthCamSocketCli_CmdRecvCb[ETHCAM_PATH_ID_MAX] = {NULL};

static ETHCAMSOCKET_CLI_NOTIFY_PROCESS_CB  g_EthCamSocketCli_Data1NotifyCb[ETHCAM_PATH_ID_MAX] = {NULL};
static ETHCAMSOCKET_CLI_NOTIFY_PROCESS_CB  g_EthCamSocketCli_Data2NotifyCb[ETHCAM_PATH_ID_MAX] = {NULL};
static ETHCAMSOCKET_CLI_NOTIFY_PROCESS_CB  g_EthCamSocketCli_CmdNotifyCb[ETHCAM_PATH_ID_MAX] = {NULL};

static ETHSOCKCLIIPC_OPEN EthsockCliIpcData_Open[ETHCAM_PATH_ID_MAX][SOCKETCLI_DATA_PORT_MAX] = {0};
static ETHSOCKCLIIPC_OPEN EthsockCliIpcCmd_Open[ETHCAM_PATH_ID_MAX] = {0};

static UINT32 g_bEthLinkStatus[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_bEthHubLinkSta[ETHCAM_PATH_ID_MAX]={0};

static BOOL g_EthCamSocketCli_Data1SocketClose[ETHCAM_PATH_ID_MAX]={0};
static BOOL g_EthCamSocketCli_Data2SocketClose[ETHCAM_PATH_ID_MAX]={0};
static BOOL g_EthCamSocketCli_CmdSocketClose[ETHCAM_PATH_ID_MAX]={0};

static BOOL g_EthCamSocketCli_Data1NeedResetI[ETHCAM_PATH_ID_MAX]={0};
static BOOL g_EthCamSocketCli_Data2NeedResetI[ETHCAM_PATH_ID_MAX]={0};
static ETHCAMSOCKET_CMD_SEND_DATA_SIZE_CB g_EthCamSocketCli_CmdSendSizeCb[ETHCAM_PATH_ID_MAX] = {NULL};

static UINT32 g_EthCamSocketCli_Data1DescSize[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_EthCamSocketCli_Data2DescSize[ETHCAM_PATH_ID_MAX]={0};

static ETHCAM_SOCKET_INFO g_EthCamSocketCli_SvrInfo[ETHCAM_PATH_ID_MAX]={0};

ID ETHCAMSOCKETCLI_HANDLE_FLG_ID = 0;
THREAD_HANDLE ETHCAMSOCKETCLI_HANDLE_TSK_ID = 0;
#define PRI_ETHCAM_ETHCAMSOCKETCLI_HANDLE               	10
#define STKSIZE_ETHCAMSOCKETCLI_HANDLE        			4096
static BOOL g_bEthCamSocketCliTskOpened = 0;
static UINT32 g_IsHandleTskRunning=0;

#if 1
void EthCamSocket_Data1Notify(int status, int parm)
{
    //DBG_IND("status = %d, parm = %d, obj_index = %d\r\n", status, parm, obj_index);
#if 0
    if(status == CYG_ETHSOCKET_STATUS_CLIENT_CONNECT){
        DBG_DUMP("EthCamSocket_Data1Notify Connect OK\r\n");
	}
    else if(status == CYG_ETHSOCKET_STATUS_CLIENT_REQUEST){
    }
    else if(status == CYG_ETHSOCKET_STATUS_CLIENT_DISCONNECT){
        DBG_DUMP("EthCamSocket_Data1Notify DISConnect !!\r\n");
	}
    else{
        DBG_ERR("^GUnknown status = %d, parm = %d\r\n", status, parm);
    }
#endif
    if(g_EthCamSocket_Data1NotifyCb){
        g_EthCamSocket_Data1NotifyCb(status ,parm);
    }

}

int EthCamSocket_Data1Recv(char* addr, int size)
{
	if (addr && (size > 0)) {

		//addr[size] = '\0';
		//DBG_DUMP("EthCamSocket_Data1 size=%d, addr=%s\r\n",  size,addr);
		if(g_EthCamSocket_Data1RecvCb){
			g_EthCamSocket_Data1RecvCb(addr, size);
		}
	}
	return 0;
}
void EthCamSocket_Data2Notify(int status, int parm)
{
    //DBG_IND("status = %d, parm = %d, obj_index = %d\r\n", status, parm, obj_index);
#if 0
    if(status == CYG_ETHSOCKET_STATUS_CLIENT_CONNECT){
        DBG_DUMP("EthCamSocket_Data2Notify Connect OK\r\n");
	}
    else if(status == CYG_ETHSOCKET_STATUS_CLIENT_REQUEST){
    }
    else if(status == CYG_ETHSOCKET_STATUS_CLIENT_DISCONNECT){
        DBG_DUMP("EthCamSocket_Data2Notify DISConnect !!\r\n");
	}
    else{
        DBG_ERR("^GUnknown status = %d, parm = %d\r\n", status, parm);
    }
#endif
    if(g_EthCamSocket_Data2NotifyCb){
        g_EthCamSocket_Data2NotifyCb(status ,parm);
    }

}

int EthCamSocket_Data2Recv(char* addr, int size)
{
	if (addr && (size > 0)) {

		//addr[size] = '\0';
		//DBG_DUMP("EthCamSocket_Data2 size=%d, addr=%s\r\n",  size,addr);
		if(g_EthCamSocket_Data2RecvCb){
			g_EthCamSocket_Data2RecvCb(addr, size);
		}
	}
	return 0;
}
void EthCamSocket_CmdNotify(int status, int parm)
{
    //DBG_IND("status = %d, parm = %d, obj_index = %d\r\n", status, parm, obj_index);
#if 0
    if(status == CYG_ETHSOCKET_STATUS_CLIENT_CONNECT){
        DBG_DUMP("EthCamSocket_CmdNotify Connect OK\r\n");
	}
    else if(status == CYG_ETHSOCKET_STATUS_CLIENT_REQUEST){
    }
    else if(status == CYG_ETHSOCKET_STATUS_CLIENT_DISCONNECT){
        DBG_DUMP("EthCamSocket_CmdNotify DISConnect !!\r\n");
	}
    else{
        DBG_ERR("^GUnknown status = %d, parm = %d\r\n", status, parm);
    }
#endif
    if(g_EthCamSocket_CmdNotifyCb){
        g_EthCamSocket_CmdNotifyCb(status ,parm);
    }

}

int EthCamSocket_CmdRecv(char* addr, int size)
{
	if (addr && (size > 0)) {

		//addr[size] = '\0';
		//DBG_DUMP("EthCamSocket_Cmd size=%d, addr=%s\r\n",  size,addr);
		if(g_EthCamSocket_CmdRecvCb){
			g_EthCamSocket_CmdRecvCb(addr, size);
		}
	}
	return 0;
}
int EthCamSocket_DataSend(ETHSOCKIPC_ID id, char* addr, int *size)
{
	ER Result=EthsockIpcMulti_Send(id, addr, size);
	if (E_OK !=Result){
		//DBG_ERR("Result=%d\r\n", Result);
		return -1;
	}else{
		return 0;
	}
}
int EthCamSocket_CmdSend(char* addr, int *size)
{
	ER Result=EthsockIpcMulti_Send(ETHSOCKIPC_ID_2, addr, size);
	if (E_OK !=Result){
		//DBG_ERR("Result=%d\r\n", Result);
		return -1;
	}else{
		return 0;
	}
}

void EthCamSocket_Open(ETHSOCKIPC_ID id, ETHSOCKIPC_OPEN *pOpen)
{
	ethsocket_install_obj  ethsocketObj = {0};
	ER rt;
	CHAR ip_base[ETHCAM_SOCKETCLI_IP_MAX_LEN]={0};
	UINT32 port_base[3]={0};

	if(g_EthCamSocket_Info.ip){
		strcpy(ip_base, g_EthCamSocket_Info.ip);
	}else{
		DBG_ERR("ip error\r\n");
		return ;
	}
	//DBG_DUMP("ip=%s\r\n",ip_base);

	if(g_EthCamSocket_Info.port){
		memcpy(port_base, g_EthCamSocket_Info.port, sizeof(g_EthCamSocket_Info.port));
	}else{
		DBG_ERR("port error\r\n");
		return ;
	}
	//DBG_DUMP("port[0]=%d, port[1]=%d, port[2]=%d\r\n",port_base[0],port_base[1],port_base[2]);

	if(id == ETHSOCKIPC_ID_0){
		// open
		ETHSOCKIPC_OPEN EthsockIpcOpen = {0};
		memcpy(&EthsockIpcOpen, pOpen, sizeof(ETHSOCKIPC_OPEN));
		if (E_OK !=(rt = EthsockIpcMulti_Open(id, &EthsockIpcOpen))) {
			if(rt != E_OBJ){
				DBG_ERR("EthsockIpc_Open fail\r\n");
			}
			return;
		}

		// init
		ethsocketObj.notify = EthCamSocket_Data1Notify;
		ethsocketObj.recv = EthCamSocket_Data1Recv;
		ethsocketObj.portNum = port_base[0];//ETH_DATA1_SOCKET_PORT;
		ethsocketObj.sockbufSize =  6*40960;//1024;
		ethsocketObj.threadPriority = 4;//6;
		if ((g_ethData1_tx_sockHandle = EthsockIpcMulti_Init(id, &ethsocketObj)) != E_OK) {
			DBG_ERR("id =%d, EthsockIpc_Init fail\r\n",id);
			return;
		}
		DBG_IND("g_ethData1_tx_sockHandle=%d\r\n",g_ethData1_tx_sockHandle);
	}else if(id == ETHSOCKIPC_ID_1){
		// open
		ETHSOCKIPC_OPEN EthsockIpcOpen = {0};
		memcpy(&EthsockIpcOpen, pOpen, sizeof(ETHSOCKIPC_OPEN));
		if (E_OK !=(rt = EthsockIpcMulti_Open(id, &EthsockIpcOpen))) {
			if(rt != E_OBJ){
				DBG_ERR("EthsockIpc_Open fail\r\n");
			}
			return;
		}

		// init
		ethsocketObj.notify = EthCamSocket_Data2Notify;
		ethsocketObj.recv = EthCamSocket_Data2Recv;
		ethsocketObj.portNum = port_base[1];//ETH_DATA2_SOCKET_PORT;
		ethsocketObj.sockbufSize =  6*40960;//1024;
		ethsocketObj.threadPriority = 4;//6;
		if ((g_ethData2_tx_sockHandle = EthsockIpcMulti_Init(id, &ethsocketObj)) != E_OK) {
			DBG_ERR("id =%d, EthsockIpc_Init fail\r\n",id);
			return;
		}
		DBG_IND("g_ethData2_tx_sockHandle=%d\r\n",g_ethData2_tx_sockHandle);
	}else if(id == ETHSOCKIPC_ID_2){
		// open
		ETHSOCKIPC_OPEN EthsockIpcOpen = {0};
		memcpy(&EthsockIpcOpen, pOpen, sizeof(ETHSOCKIPC_OPEN));
		if (E_OK !=(rt = EthsockIpcMulti_Open(id, &EthsockIpcOpen))) {
			if(rt != E_OBJ){
				DBG_ERR("EthsockIpc_Open fail\r\n");
			}
			return;
		}

		// init
		ethsocketObj.notify = EthCamSocket_CmdNotify;
		ethsocketObj.recv = EthCamSocket_CmdRecv;
		ethsocketObj.portNum = port_base[2];//ETH_CMD_SOCKET_PORT;
		ethsocketObj.sockbufSize =  4*40960;//1024;
		ethsocketObj.threadPriority = 6;
		if ((g_ethCmd_tx_sockHandle = EthsockIpcMulti_Init(id, &ethsocketObj)) != E_OK) {
			DBG_ERR("id =%d, EthsockIpc_Init fail\r\n",id);
			return;
		}
		DBG_IND("g_ethCmd_tx_sockHandle=%d\r\n",g_ethCmd_tx_sockHandle);
	}
}
void EthCamSocket_Close(void)
{
	if(g_ethData1_tx_sockHandle != -1){
		DBG_DUMP("EthCamSocket_Close Data1\r\n");
		EthsockIpcMulti_Uninit(ETHSOCKIPC_ID_0);
		EthsockIpcMulti_Close(ETHSOCKIPC_ID_0);
		g_ethData1_tx_sockHandle=-1;
	}
	if(g_ethData2_tx_sockHandle != -1){
		DBG_DUMP("EthCamSocket_Close Data2\r\n");
		EthsockIpcMulti_Uninit(ETHSOCKIPC_ID_1);
		EthsockIpcMulti_Close(ETHSOCKIPC_ID_1);
		g_ethData2_tx_sockHandle=-1;
	}
	if(g_ethCmd_tx_sockHandle != -1){
		DBG_DUMP("EthCamSocket_Close Cmd\r\n");
		EthsockIpcMulti_Uninit(ETHSOCKIPC_ID_2);
		EthsockIpcMulti_Close(ETHSOCKIPC_ID_2);
		g_ethCmd_tx_sockHandle=-1;
	}
}
void EthCamSocket_SetDataRecvCB(ETHSOCKIPC_ID id, UINT32 value)
{
	if(id< ETHSOCKIPC_ID_0 || id>ETHSOCKIPC_ID_1){
		DBG_ERR("id out of range ,id=%d\r\n", id);
		return;
	}

	if (value != 0) {
		if(id== ETHSOCKIPC_ID_0 ){
			g_EthCamSocket_Data1RecvCb = (ETHCAMSOCKET_RECV_PROCESS_CB) value;
		}else{
			g_EthCamSocket_Data2RecvCb = (ETHCAMSOCKET_RECV_PROCESS_CB) value;
		}
	}
}
void EthCamSocket_SetCmdRecvCB(UINT32 value)
{
	if (value != 0) {
		g_EthCamSocket_CmdRecvCb = (ETHCAMSOCKET_RECV_PROCESS_CB) value;
	}
}
void EthCamSocket_SetDataNotifyCB(ETHSOCKIPC_ID id, UINT32 value)
{
	if(id< ETHSOCKIPC_ID_0 || id>ETHSOCKIPC_ID_1){
		DBG_ERR("id out of range ,id=%d\r\n", id);
		return;
	}

	if (value != 0) {
		if(id== ETHSOCKIPC_ID_0 ){
			g_EthCamSocket_Data1NotifyCb = (ETHCAMSOCKET_NOTIFY_PROCESS_CB) value;
		}else{
			g_EthCamSocket_Data2NotifyCb = (ETHCAMSOCKET_NOTIFY_PROCESS_CB) value;
		}
	}
}
void EthCamSocket_SetCmdNotifyCB(UINT32 value)
{
	if (value != 0) {
		g_EthCamSocket_CmdNotifyCb = (ETHCAMSOCKET_NOTIFY_PROCESS_CB) value;
	}
}

void EthCamSocket_SetInfo(ETHCAM_SOCKET_INFO* info)
{
	memcpy((void*)&g_EthCamSocket_Info, (void*)info, sizeof(ETHCAM_SOCKET_INFO));
}
#endif


//======================Rx Start======================
void EthCamSocketCli_Data1Notify(int path_id , int status, int parm)
{
#if 0
	switch (status) {
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_REQUEST: {
			//DBG_DUMP("Notify EthCli REQUEST\r\n");
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_CONNECT: {
			DBG_DUMP("Notify EthCamSocketCli_Data1 connect OK\r\n");
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_DISCONNECT: {
			DBG_ERR("EthCamSocketCli_Data1 disconnect!!!\r\n");
			break;
		}
	}
#else
	switch (status) {
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_SOCKET_CLOSE: {
			g_EthCamSocketCli_Data1SocketClose[path_id]=1;
			break;
		}
	}

	if(g_EthCamSocketCli_Data1NotifyCb[path_id]){
		g_EthCamSocketCli_Data1NotifyCb[path_id](path_id, status ,parm);
	}
#endif
}

int EthCamSocketCli_Data1Recv(int path_id , char* addr, int size)
{
	if(addr && (size>0))
	{
		#if 0
		//DBG_DUMP("D1 size=%d, addr=%s\r\n",  size,addr);
		//DBG_DUMP("D1 size=%d, 0x%x, [0]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",  size, addr, addr[0],addr[1],addr[2],addr[3],addr[4]);
		//DBG_DUMP("D1 size=%d, %d\r\n",  size, addr);
		//addr[size]='\0';
		//size_get+=size;
		//DBG_DUMP("size=%d\r\n",  size);
		if(addr[0] ==0 && addr[1] ==0 && addr[2] ==0 && addr[3] ==1){
			if(((addr[4])& 0x1F) == H264_NALU_TYPE_IDR){
				//DBG_DUMP("I OK\r\n");
				//g_queue_idx++;
				//g_SocketCliData1_BsFrameCnt++;
				//eth_is_i_frame = 1;
				//eth_i_cnt ++;
		//if(g_EthCamSocketCli_Data1RecvCb){
		//	CHKPNT;
		//	g_EthCamSocketCli_Data1RecvCb(addr, size);
		//}

			}else if(((addr[4])& 0x1F) == H264_NALU_TYPE_SLICE){
				//DBG_DUMP("P OK\r\n");
				//g_queue_idx++;
				//g_SocketCliData1_BsFrameCnt++;
				//eth_is_i_frame = 0;
			}else if(((addr[4])& 0x1F) == H264_NALU_TYPE_SPS){
				//DBG_DUMP("SPS OK\r\n");
			}else if(((addr[4])& 0x1F) == H264_NALU_TYPE_PPS){
				//DBG_DUMP("PPS OK\r\n");
			}else if(((addr[4])==0xFF)){
				DBG_DUMP("Thumb OK\r\n");
				DBG_DUMP("data[5]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",addr[5],addr[6],addr[size-5],addr[size-4],addr[size-3],addr[size-2],addr[size-1],addr[size]);
		//if(g_EthCamSocketCli_Data1RecvCb){
		//	CHKPNT;
		//	g_EthCamSocketCli_Data1RecvCb(addr, size);
		//}

			}else if(((addr[4])==0xFE)){
				//DBG_DUMP("PIM OK\r\n");
				//DBG_DUMP("data[5]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",*(g_psocket_cli + g_bs_size_start +5),*(g_psocket_cli + g_bs_size_start +6),*(g_psocket_cli + g_bs_size_start +g_bs_size_total-5),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-4),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-3),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-2),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-1),*(g_psocket_cli + g_bs_size_start + g_bs_size_total));
			}else{
				//DBG_DUMP("Check FAIL\r\n");
			}
		}
		#endif
		//dma_flushReadCache((UINT32)addr, (UINT32)size);
		//addr =(char*)dma_getNonCacheAddr((UINT32)addr);
		UINT32 DescOffset=TS_VIDEO_RESERVED_SIZE;
		UINT32 DescOffset_Desc=0; //offset + Desc Size
		//static UINT32 cnt=0;
		//DBG_DUMP("D1 size=%d, [0]=0x%x, %x, %x, %x, %x, %x\r\n",  size, addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);

		//if(path_id==1 && cnt<10){
			//cnt++;
			//DBG_DUMP("1ResetI=%d, addr[0]=0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",g_EthCamSocketCli_Data1NeedResetI[path_id],addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7],addr[8],addr[9],addr[10],addr[11],addr[12],addr[13]);
		//}

		if(g_EthCamSocketCli_Data1NeedResetI[path_id]){
			if(g_EthCamSocketCli_Data1DescSize[path_id]%4){
				DescOffset+=4-(g_EthCamSocketCli_Data1DescSize[path_id]%4);  //for H265 I Frame Bs Align 4, for VdoDec
			}
			DescOffset_Desc=DescOffset+g_EthCamSocketCli_Data1DescSize[path_id];
			//DBG_DUMP("D1 DescOffset_Desc=%d, [DescOffset_Desc+0]=0x%x, %x, %x, %x, %x, %x\r\n",  DescOffset_Desc, addr[DescOffset_Desc+0],addr[DescOffset_Desc+1],addr[DescOffset_Desc+2],addr[DescOffset_Desc+3],addr[DescOffset_Desc+4],addr[DescOffset_Desc+5]);
			//DBG_DUMP("11D1 size=%d, [0]=0x%x, %x, %x, %x, %x, %x, %x, %x, %x\r\n",  size, addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7],addr[8]);
			#if 0
			if(addr[0+DescOffset] ==0 && addr[1+DescOffset] ==0 && addr[2+DescOffset] ==0 && addr[3+DescOffset] ==1 && ((addr[4+DescOffset])>>1) == H265_NALU_TYPE_VPS){
				addr+=DescOffset;
				size-=DescOffset;
				g_EthCamSocketCli_Data1NeedResetI[path_id]=0;
			}else if(addr[0] ==0 && addr[1] ==0 && addr[2] ==0 && addr[3] ==1 && ((addr[4])& 0x1F) == H264_NALU_TYPE_IDR){
				g_EthCamSocketCli_Data1NeedResetI[path_id]=0;
			}else{
				return 0;
			}
			#else
			if(addr[0+DescOffset] ==0 && addr[1+DescOffset] ==0 && addr[2+DescOffset] ==0 && addr[3+DescOffset] ==1 && (((addr[4+DescOffset])>>1)&0x3F) == H265_NALU_TYPE_VPS){
				addr+=DescOffset;//vps addr
				size-=DescOffset;
				g_EthCamSocketCli_Data1NeedResetI[path_id]=0;
			}else if(addr[0+DescOffset_Desc] ==0 && addr[1+DescOffset_Desc] ==0 && addr[2+DescOffset_Desc] ==0 && addr[3+DescOffset_Desc] ==1 && ((addr[4+DescOffset_Desc])& 0x1F) == H264_NALU_TYPE_IDR && addr[5+DescOffset_Desc]==H264_START_CODE_I){
				addr+=DescOffset;//sps addr
				size-=DescOffset;
				g_EthCamSocketCli_Data1NeedResetI[path_id]=0;
			}else{
				return 0;
			}
			#endif
		}else{
			if(addr[0+DescOffset] ==0 && addr[1+DescOffset] ==0 && addr[2+DescOffset] ==0 && addr[3+DescOffset] ==1 && ((((addr[4+DescOffset])>>1)&0x3F) == H265_NALU_TYPE_SLICE
				|| (((addr[4+DescOffset])& 0x1F) == H264_NALU_TYPE_SLICE  && addr[5+DescOffset]==H264_START_CODE_P))){
				addr+=DescOffset;//P frame addr
				size-=DescOffset;
			}else{
				if(g_EthCamSocketCli_Data1DescSize[path_id]%4){
					DescOffset+=4-(g_EthCamSocketCli_Data1DescSize[path_id]%4);  //for H265 I Frame Bs Align 4, for VdoDec
				}
				DescOffset_Desc=DescOffset+g_EthCamSocketCli_Data1DescSize[path_id];

				#if 0
				if(addr[0+DescOffset] ==0 && addr[1+DescOffset] ==0 && addr[2+DescOffset] ==0 && addr[3+DescOffset] ==1 && ((addr[4+DescOffset])>>1) == H265_NALU_TYPE_VPS){
					addr+=DescOffset;
					size-=DescOffset;
				}
				#else
			//DBG_DUMP("12D1 Desc=%d, %d, size=%d, [0]=0x%x, %x, %x, %x, %x, %x, %x, %x, %x\r\n", DescOffset_Desc, DescOffset, size, addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7],addr[8]);
				if(addr[0+DescOffset] ==0 && addr[1+DescOffset] ==0 && addr[2+DescOffset] ==0 && addr[3+DescOffset] ==1 && (((addr[4+DescOffset])>>1)&0x3F) == H265_NALU_TYPE_VPS){
					addr+=DescOffset;//vps addr
					size-=DescOffset;
				}else if(addr[0+DescOffset_Desc] ==0 && addr[1+DescOffset_Desc] ==0 && addr[2+DescOffset_Desc] ==0 && addr[3+DescOffset_Desc] ==1 && ((addr[4+DescOffset_Desc])& 0x1F) == H264_NALU_TYPE_IDR && addr[5+DescOffset_Desc]==H264_START_CODE_I){
					addr+=DescOffset;//sps addr
					size-=DescOffset;
				}
			//DBG_DUMP("13D1 size=%d, [0]=0x%x, %x, %x, %x, %x, %x, %x, %x, %x\r\n",  size, addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7],addr[8]);

				#endif
			}
		}
		//DBG_DUMP("addr[0]=0x%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7],addr[8],addr[9],addr[10],addr[11],addr[12],addr[13]);
		if(g_EthCamSocketCli_Data1RecvCb[path_id]){
			g_EthCamSocketCli_Data1RecvCb[path_id](path_id, addr, size);
		}
	}
	return 0;
}
void EthCamSocketCli_Data2Notify(int path_id , int status, int parm)
{
#if 0
	switch (status) {
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_REQUEST: {
			//DBG_DUMP("Notify EthCli REQUEST\r\n");
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_CONNECT: {
			DBG_DUMP("Notify EthCamSocketCli_Data2 connect OK\r\n");
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_DISCONNECT: {
			DBG_ERR("EthCamSocketCli_Data2 disconnect!!!\r\n");
			break;
		}
	}
#else
	switch (status) {
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_SOCKET_CLOSE: {
			g_EthCamSocketCli_Data2SocketClose[path_id]=1;
			break;
		}
	}

	if(g_EthCamSocketCli_Data2NotifyCb[path_id]){
		g_EthCamSocketCli_Data2NotifyCb[path_id](path_id, status ,parm);
	}
#endif
}

int EthCamSocketCli_Data2Recv(int path_id , char* addr, int size)
{
	if(addr && (size>0))
	{
		#if 0
		//DBG_DUMP("D1 size=%d, addr=%s\r\n",  size,addr);
		//DBG_DUMP("D1 size=%d, 0x%x, [0]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",  size, addr, addr[0],addr[1],addr[2],addr[3],addr[4]);
		//DBG_DUMP("D1 size=%d, %d\r\n",  size, addr);
		//addr[size]='\0';
		//size_get+=size;
		//DBG_DUMP("size=%d\r\n",  size);
		if(addr[0] ==0 && addr[1] ==0 && addr[2] ==0 && addr[3] ==1){
			if(((addr[4])& 0x1F) == H264_NALU_TYPE_IDR){
				//DBG_DUMP("I OK\r\n");
				//g_queue_idx++;
				//g_SocketCliData1_BsFrameCnt++;
				//eth_is_i_frame = 1;
				//eth_i_cnt ++;
		//if(g_EthCamSocketCli_Data1RecvCb){
		//	CHKPNT;
		//	g_EthCamSocketCli_Data1RecvCb(addr, size);
		//}

			}else if(((addr[4])& 0x1F) == H264_NALU_TYPE_SLICE){
				//DBG_DUMP("P OK\r\n");
				//g_queue_idx++;
				//g_SocketCliData1_BsFrameCnt++;
				//eth_is_i_frame = 0;
			}else if(((addr[4])& 0x1F) == H264_NALU_TYPE_SPS){
				//DBG_DUMP("SPS OK\r\n");
			}else if(((addr[4])& 0x1F) == H264_NALU_TYPE_PPS){
				//DBG_DUMP("PPS OK\r\n");
			}else if(((addr[4])==0xFF)){
				DBG_DUMP("Thumb OK\r\n");
				DBG_DUMP("data[5]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",addr[5],addr[6],addr[size-5],addr[size-4],addr[size-3],addr[size-2],addr[size-1],addr[size]);
		//if(g_EthCamSocketCli_Data1RecvCb){
		//	CHKPNT;
		//	g_EthCamSocketCli_Data1RecvCb(addr, size);
		//}

			}else if(((addr[4])==0xFE)){
				//DBG_DUMP("PIM OK\r\n");
				//DBG_DUMP("data[5]=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\r\n",*(g_psocket_cli + g_bs_size_start +5),*(g_psocket_cli + g_bs_size_start +6),*(g_psocket_cli + g_bs_size_start +g_bs_size_total-5),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-4),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-3),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-2),*(g_psocket_cli + g_bs_size_start + g_bs_size_total-1),*(g_psocket_cli + g_bs_size_start + g_bs_size_total));
			}else{
				//DBG_DUMP("Check FAIL\r\n");
			}
		}
		#endif
		//addr =(char*)dma_getNonCacheAddr((UINT32)addr);
		UINT32 DescOffset=TS_VIDEO_RESERVED_SIZE; //Align 4 , addr offset
		UINT32 DescOffset_Desc=0; //offset + Desc Size
		//DBG_DUMP("D2 size=%d, [0]=0x%x, %x, %x, %x, %x, %x\r\n",  size, addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);

		if(g_EthCamSocketCli_Data2NeedResetI[path_id]){
			if(g_EthCamSocketCli_Data2DescSize[path_id]%4){
				DescOffset+=4-(g_EthCamSocketCli_Data2DescSize[path_id]%4);  //for H265 I Frame Bs Align 4, for VdoDec
			}
			//DescOffset+=TS_VIDEO_RESERVED_SIZE;
			DescOffset_Desc=DescOffset+g_EthCamSocketCli_Data2DescSize[path_id];
			//DBG_DUMP("D2 DescOffset_Desc=%d, [DescOffset_Desc+0]=0x%x, %x, %x, %x, %x, %x\r\n",  DescOffset_Desc, addr[DescOffset_Desc+0],addr[DescOffset_Desc+1],addr[DescOffset_Desc+2],addr[DescOffset_Desc+3],addr[DescOffset_Desc+4],addr[DescOffset_Desc+5]);

			#if 0
			if(addr[0+DescOffset] ==0 && addr[1+DescOffset] ==0 && addr[2+DescOffset] ==0 && addr[3+DescOffset] ==1 && ((addr[4+DescOffset])>>1) == H265_NALU_TYPE_VPS){
				addr+=DescOffset;
				size-=DescOffset;
				g_EthCamSocketCli_Data2NeedResetI[path_id]=0;
			}else if(addr[0] ==0 && addr[1] ==0 && addr[2] ==0 && addr[3] ==1 && ((addr[4])& 0x1F) == H264_NALU_TYPE_IDR){
				g_EthCamSocketCli_Data2NeedResetI[path_id]=0;
			}else{
				return 0;
			}
			#else
			if(addr[0+DescOffset] ==0 && addr[1+DescOffset] ==0 && addr[2+DescOffset] ==0 && addr[3+DescOffset] ==1 && (((addr[4+DescOffset])>>1)&0x3F) == H265_NALU_TYPE_VPS){
				addr+=DescOffset;//VPS addr
				size-=DescOffset;
				g_EthCamSocketCli_Data2NeedResetI[path_id]=0;
			}else if(addr[0+DescOffset_Desc] ==0 && addr[1+DescOffset_Desc] ==0 && addr[2+DescOffset_Desc] ==0 && addr[3+DescOffset_Desc] ==1 && ((addr[4+DescOffset_Desc])& 0x1F) == H264_NALU_TYPE_IDR && addr[5+DescOffset_Desc]==H264_START_CODE_I){
				addr+=DescOffset;//SPS addr
				size-=DescOffset;
				g_EthCamSocketCli_Data2NeedResetI[path_id]=0;
			}else{
				return 0;
			}
			#endif

		}else{
			if(addr[0+DescOffset] ==0 && addr[1+DescOffset] ==0 && addr[2+DescOffset] ==0 && addr[3+DescOffset] ==1 && ((((addr[4+DescOffset])>>1)&0x3F) == H265_NALU_TYPE_SLICE
				|| (((addr[4+DescOffset])& 0x1F) == H264_NALU_TYPE_SLICE  && addr[5+DescOffset]==H264_START_CODE_P))){
				addr+=DescOffset;//P frame addr
				size-=DescOffset;
			}else{
				if(g_EthCamSocketCli_Data2DescSize[path_id]%4){
					DescOffset+=4-(g_EthCamSocketCli_Data2DescSize[path_id]%4);  //for H265 I Frame Bs Align 4, for VdoDec
				}
				//DescOffset+=TS_VIDEO_RESERVED_SIZE;
				DescOffset_Desc=DescOffset+g_EthCamSocketCli_Data2DescSize[path_id];
				#if 0
				if(addr[0+DescOffset] ==0 && addr[1+DescOffset] ==0 && addr[2+DescOffset] ==0 && addr[3+DescOffset] ==1 && ((addr[4+DescOffset])>>1) == H265_NALU_TYPE_VPS){
					addr+=DescOffset;
					size-=DescOffset;
				}
				#else
				if(addr[0+DescOffset] ==0 && addr[1+DescOffset] ==0 && addr[2+DescOffset] ==0 && addr[3+DescOffset] ==1 && (((addr[4+DescOffset])>>1)&0x3F) == H265_NALU_TYPE_VPS){
					addr+=DescOffset;//VPS addr
					size-=DescOffset;
				}else if(addr[0+DescOffset_Desc] ==0 && addr[1+DescOffset_Desc] ==0 && addr[2+DescOffset_Desc] ==0 && addr[3+DescOffset_Desc] ==1 && ((addr[4+DescOffset_Desc])& 0x1F) == H264_NALU_TYPE_IDR && addr[5+DescOffset_Desc]==H264_START_CODE_I){
					addr+=DescOffset;//SPS addr
					size-=DescOffset;
				}
				#endif
			}
		}

		if(g_EthCamSocketCli_Data2RecvCb[path_id]){
			g_EthCamSocketCli_Data2RecvCb[path_id](path_id, addr, size);
		}
	}
	return 0;
}
void EthCamSocketCli_CmdNotify(int path_id , int status, int parm)
{
#if 0
	switch (status) {
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_REQUEST: {
			//DBG_DUMP("Notify EthCli REQUEST\r\n");
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_CONNECT: {
			DBG_DUMP("Notify EthCamSocketCli_Cmd connect OK\r\n");
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_DISCONNECT: {
			DBG_ERR("EthCamSocketCli_Cmd disconnect!!!\r\n");
			break;
		}
	}
#else
	switch (status) {
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_SOCKET_CLOSE: {
			g_EthCamSocketCli_CmdSocketClose[path_id]=1;
			break;
		}
	}

	if(g_EthCamSocketCli_CmdNotifyCb[path_id]){
		g_EthCamSocketCli_CmdNotifyCb[path_id](path_id, status ,parm);
	}
#endif
}

int EthCamSocketCli_CmdRecv(int path_id , char* addr, int size)
{
	if(addr && (size>0))
	{
		if(g_EthCamSocketCli_CmdRecvCb[path_id]){
			g_EthCamSocketCli_CmdRecvCb[path_id](path_id, addr, size);
		}
	}
	return 0;
}
static void EthCamSocketCli_ReConnectTimerCB(UINT32 uiEvent)
{
	set_flg(ETHCAMSOCKETCLI_HANDLE_FLG_ID, FLG_ETHCAMSOCKETCLI_RECONN);
}

void EthCamSocketCli_ReConnect(ETHCAM_PATH_ID path_id ,UINT32 socket_id, UINT32 bStart)
{
	if (!SOCKETCLI_RECONNECT_MAX_COUNT) {
		return;
	}
	socketcli_reconnect_pathid[path_id]=1;
	socketcli_reconnect_socketid[path_id][socket_id]=1;

	if (g_bEthLinkStatus[path_id]==ETHCAM_LINK_UP && bStart && (g_ethData1_rx_sockCliHandle[path_id]==0
		|| g_ethData2_rx_sockCliHandle[path_id]==0
		|| g_ethCmd_rx_sockCliHandle[path_id]==0)) {
		DBG_WRN("Start SocketCli Reconnect Timer[%d][%d]\r\n",path_id,socket_id);
		if (SOCKETCLI_RECONNECT_MAX_COUNT && socketcli_reconnect_count[path_id][socket_id] >= SOCKETCLI_RECONNECT_MAX_COUNT) {
			DBG_ERR("socketcli connect aborts retry\r\n");
			socketcli_reconnect_count[path_id][socket_id]=0;
			if(g_EthCamSocketCli_ReConnecTimerId != SWTIMER_NUM){
				SwTimer_Stop(g_EthCamSocketCli_ReConnecTimerId);
				SwTimer_Close(g_EthCamSocketCli_ReConnecTimerId);
				g_EthCamSocketCli_ReConnecTimerId=SWTIMER_NUM;
			}
		} else {
			socketcli_reconnect_count[path_id][socket_id]++;
			DBG_WRN("[%d][%d]socketcli Reconnect %d, TimerId=%d\r\n", path_id,socket_id,socketcli_reconnect_count[path_id][socket_id],g_EthCamSocketCli_ReConnecTimerId);

			if(g_EthCamSocketCli_ReConnecTimerId==SWTIMER_NUM){
				if (SwTimer_Open(&g_EthCamSocketCli_ReConnecTimerId, EthCamSocketCli_ReConnectTimerCB) != E_OK) {
					DBG_ERR("open timer fail\r\n");
				} else {
					DBG_IND("ReConnect open timer OK\r\n");
					SwTimer_Cfg(g_EthCamSocketCli_ReConnecTimerId, 500 /*ms*/, SWTIMER_MODE_FREE_RUN);
					SwTimer_Start(g_EthCamSocketCli_ReConnecTimerId);
				}
			}else{
				if(socket_id == ETHSOCKIPCCLI_ID_2){
					EthCamSocketCli_Open(path_id,  socket_id, &EthsockCliIpcCmd_Open[path_id]);
				}else{
					EthCamSocketCli_Open(path_id,  socket_id, &EthsockCliIpcData_Open[path_id][socket_id]);
				}
			}
		}
	} else {
		UINT32 i, j;
		UINT32 AllPathLinkStatus=0;
		DBG_IND("Stop SocketCli Reconnect Timer[%d][%d]\r\n",path_id,socket_id);
		socketcli_reconnect_pathid[path_id]=0;
		socketcli_reconnect_socketid[path_id][socket_id]=0;
		for(i=0;i<ETHCAM_PATH_ID_MAX;i++){
			for(j=0;j<ETHCAM_PORT_DATA_MAX;j++){
				if(socketcli_reconnect_pathid[i]==0 && socketcli_reconnect_socketid[i][j]==0){
					AllPathLinkStatus++;
				}
			}
		}
		DBG_IND("AllPathLinkStatus=%d\r\n",AllPathLinkStatus);

		if(g_EthCamSocketCli_ReConnecTimerId != SWTIMER_NUM && (AllPathLinkStatus== (ETHCAM_PATH_ID_MAX) * (ETHCAM_PORT_DATA_MAX))){
			DBG_DUMP("ReConnect stop timer OK\r\n");
			SwTimer_Stop(g_EthCamSocketCli_ReConnecTimerId);
			SwTimer_Close(g_EthCamSocketCli_ReConnecTimerId);
			g_EthCamSocketCli_ReConnecTimerId=SWTIMER_NUM;
		}
		socketcli_reconnect_count[path_id][socket_id]=0;
	}
}
UINT32 EthCamSocketCli_ReConnectIsStart(ETHCAM_PATH_ID path_id)
{
	//DBG_DUMP("count=%d\r\n",socketcli_reconnect_count);
	//return socketcli_reconnect_count[path_id];
	return 0;
}
void EthCamSocketCli_Open(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, ETHSOCKCLIIPC_OPEN *pOpen)//UINT32 IpcMemAddr, UINT32 IpcMemSize)
{
	ethsocketcli_install_obj  ethsocketcliObj = {0};
	CHAR ip_base[ETHCAM_PATH_ID_MAX][ETHCAM_SOCKETCLI_IP_MAX_LEN]={0};
	UINT32 port_base[ETHCAM_PATH_ID_MAX][3]={0};
	DBG_IND("Open[%d][%d]\r\n",path_id,id);

	if(g_EthCamSocketCli_SvrInfo[path_id].ip){
		strcpy(ip_base[path_id], g_EthCamSocketCli_SvrInfo[path_id].ip);
	}else{
		DBG_ERR("ip error, path_id=%d\r\n",path_id);
		return ;
	}
	//DBG_DUMP("ip=%s , path_id=%d\r\n",ip_base[path_id],path_id);

	if(g_EthCamSocketCli_SvrInfo[path_id].port){
		memcpy(port_base[path_id], g_EthCamSocketCli_SvrInfo[path_id].port, sizeof(g_EthCamSocketCli_SvrInfo[path_id].port));
	}else{
		DBG_ERR("port error, path_id=%d\r\n",path_id);
		return ;
	}
	EthCamSocketCli_HandleTskOpen();

	//DBG_DUMP("path_id=%d, port[0]=%d, port[1]=%d, port[2]=%d\r\n",path_id,port_base[path_id][0],port_base[path_id][1],port_base[path_id][2]);
	if(id== ETHSOCKIPCCLI_ID_0){
		g_EthCamSocketCli_Data1NeedResetI[path_id]=0;
		if(g_ethData1_rx_sockCliHandle[path_id]){
			DBG_WRN("[%d]EthCamSocketCli Data1 already open!\r\n",path_id);
		}else{
			memcpy(&EthsockCliIpcData_Open[path_id][id], pOpen, sizeof(ETHSOCKCLIIPC_OPEN));

			if (E_OK != EthsockCliIpc_Open(path_id ,id, &EthsockCliIpcData_Open[path_id][id])) {
				DBG_ERR("[%d]EthsockCliIpc_Open fail\r\n",path_id);
				return ;
			}

			// init
			ethsocketcliObj.notify = EthCamSocketCli_Data1Notify;
			ethsocketcliObj.recv = EthCamSocketCli_Data1Recv;
			sprintf(ethsocketcliObj.svrIP, "%s", ip_base[path_id]);
			DBG_DUMP("[%d][%d]conn ip=%s\r\n", path_id,id,ethsocketcliObj.svrIP);
			ethsocketcliObj.portNum = port_base[path_id][0];
			ethsocketcliObj.sockbufSize = 2*1024*1024;//6*40960;//1024;
			ethsocketcliObj.timeout = 5;
			ethsocketcliObj.threadPriority = 2;//4;//6;//8;//4;//4;//6;
			if ((g_ethData1_rx_sockCliHandle[path_id] = EthsockCliIpc_Connect(path_id, id, &ethsocketcliObj)) == 0) {
				DBG_ERR("EthsockCliIpc_Connect fail[%d][%d] ,Handle=%d\r\n",path_id,id,g_ethData1_rx_sockCliHandle[path_id]);
				//return ;
			}
			DBG_IND("g_ethData1_rx_sockCliHandle=%d\r\n",g_ethData1_rx_sockCliHandle[path_id]);
			if(g_ethData1_rx_sockCliHandle[path_id]==-2){//Timeout
				g_ethData1_rx_sockCliHandle[path_id]=0;
				EthCamSocketCli_ReConnect(path_id, id, 1);
				//ReconnId[id]=1;
			}else if(g_ethData1_rx_sockCliHandle[path_id]==-3){//Connection refused
				g_ethData1_rx_sockCliHandle[path_id]=0;
				EthCamSocketCli_ReConnect(path_id, id, 1);
				//ReconnId[id]=1;
			//}else if(g_ethData1_rx_sockCliHandle[path_id] != 0){
			}else if(g_ethData1_rx_sockCliHandle[path_id] > 0){
				DBG_IND("EthCamSocketCli_Data1 Connect OK\r\n");
				EthsockCliIpc_Start(path_id, id);
				g_EthCamSocketCli_Data1NeedResetI[path_id]=1;
				g_EthCamSocketCli_Data1SocketClose[path_id]=0;
				//if(socketcli_reconnect_pathid==path_id && socketcli_reconnect_socketid==id){
				if(socketcli_reconnect_pathid[path_id] && socketcli_reconnect_socketid[path_id][id]){
					EthCamSocketCli_ReConnect(path_id, id, 0);
				}
				if(g_EthCamSocketCli_Data1NotifyCb[path_id]){
					g_EthCamSocketCli_Data1NotifyCb[path_id](path_id, CYG_ETHSOCKETCLI_STATUS_CLIENT_CONNECT ,0);
				}
			}else if(g_ethData1_rx_sockCliHandle[path_id] == 0){
				EthsockCliIpc_Close(path_id ,id);
			}
		}
	}else if(id == ETHSOCKIPCCLI_ID_1){
		g_EthCamSocketCli_Data2NeedResetI[path_id]=0;
		if(g_ethData2_rx_sockCliHandle[path_id]){
			DBG_WRN("[%d]EthCamSocketCli Data2 already open!\r\n",path_id);
		}else{
			memcpy(&EthsockCliIpcData_Open[path_id][id], pOpen, sizeof(ETHSOCKCLIIPC_OPEN));

			if (E_OK != EthsockCliIpc_Open(path_id ,id, &EthsockCliIpcData_Open[path_id][id])) {
				DBG_ERR("EthsockCliIpc_Open fail\r\n");
				return ;
			}

			// init
			ethsocketcliObj.notify = EthCamSocketCli_Data2Notify;
			ethsocketcliObj.recv = EthCamSocketCli_Data2Recv;
			sprintf(ethsocketcliObj.svrIP, "%s", ip_base[path_id]);
			DBG_DUMP("[%d][%d]conn ip=%s\r\n", path_id,id,ethsocketcliObj.svrIP);
			ethsocketcliObj.portNum = port_base[path_id][1];
			ethsocketcliObj.sockbufSize = 2*1024*1024;//6*40960;//1024;
			ethsocketcliObj.timeout = 5;
			ethsocketcliObj.threadPriority = 4;//6;//8;//4;//6;
			if ((g_ethData2_rx_sockCliHandle[path_id] = EthsockCliIpc_Connect(path_id, id, &ethsocketcliObj)) == 0) {
				DBG_ERR("EthsockCliIpc_Connect fail[%d][%d] ,Handle=%d\r\n",path_id,id,g_ethData2_rx_sockCliHandle[path_id]);
				//return ;
			}
			DBG_IND("g_ethData2_rx_sockCliHandle=%d\r\n",g_ethData2_rx_sockCliHandle[path_id]);
			if(g_ethData2_rx_sockCliHandle[path_id]==-2){//Timeout
				g_ethData2_rx_sockCliHandle[path_id]=0;
				EthCamSocketCli_ReConnect(path_id, id, 1);
				//ReconnId[id]=1;
			}else if(g_ethData2_rx_sockCliHandle[path_id]==-3){//Connection refused
				g_ethData2_rx_sockCliHandle[path_id]=0;
				EthCamSocketCli_ReConnect(path_id, id, 1);
				//ReconnId[id]=1;
			//}else if(g_ethData2_rx_sockCliHandle[path_id] != 0){
			}else if(g_ethData2_rx_sockCliHandle[path_id] > 0){
				DBG_IND("EthCamSocketCli_Data2 Connect OK\r\n");
				EthsockCliIpc_Start(path_id, id);
				g_EthCamSocketCli_Data2NeedResetI[path_id]=1;
				g_EthCamSocketCli_Data2SocketClose[path_id]=0;
				//if(socketcli_reconnect_pathid==path_id && socketcli_reconnect_socketid==id){
				if(socketcli_reconnect_pathid[path_id] && socketcli_reconnect_socketid[path_id][id]){
					EthCamSocketCli_ReConnect(path_id, id, 0);
				}
				if(g_EthCamSocketCli_Data2NotifyCb[path_id]){
					g_EthCamSocketCli_Data2NotifyCb[path_id](path_id, CYG_ETHSOCKETCLI_STATUS_CLIENT_CONNECT ,0);
				}
			}else if(g_ethData2_rx_sockCliHandle[path_id] == 0){
				EthsockCliIpc_Close(path_id ,id);
			}
		}
	}else if(id == ETHSOCKIPCCLI_ID_2){
		if(g_ethCmd_rx_sockCliHandle[path_id]){
			DBG_WRN("[%d]EthCamSocketCli Cmd already open!\r\n",path_id);
		}else{
			memcpy(&EthsockCliIpcCmd_Open[path_id], pOpen, sizeof(ETHSOCKCLIIPC_OPEN));

			if (E_OK != EthsockCliIpc_Open(path_id ,id, &EthsockCliIpcCmd_Open[path_id])) {
				DBG_ERR("EthsockCliIpc_Open fail\r\n");
				return ;
			}

			// init
			ethsocketcliObj.notify = EthCamSocketCli_CmdNotify;
			ethsocketcliObj.recv = EthCamSocketCli_CmdRecv;
			sprintf(ethsocketcliObj.svrIP, "%s", ip_base[path_id]);
			DBG_DUMP("[%d][%d]conn ip=%s\r\n",path_id,id, ethsocketcliObj.svrIP);
			ethsocketcliObj.portNum = port_base[path_id][2];
			ethsocketcliObj.sockbufSize = 4*40960;//1024;
			ethsocketcliObj.timeout = 5;
			ethsocketcliObj.threadPriority = 10;//7;
			if ((g_ethCmd_rx_sockCliHandle[path_id] = EthsockCliIpc_Connect(path_id, id, &ethsocketcliObj)) == 0) {
				DBG_ERR("EthsockCliIpc_Connect fail[%d][%d] ,Handle=%d\r\n",path_id,id,g_ethCmd_rx_sockCliHandle[path_id]);
				//return ;
			}
			DBG_IND("ethCmdCliHandle[%d]=%d\r\n",path_id,g_ethCmd_rx_sockCliHandle[path_id]);
			if(g_ethCmd_rx_sockCliHandle[path_id]==-2){//Timeout
				g_ethCmd_rx_sockCliHandle[path_id]=0;
				EthCamSocketCli_ReConnect(path_id, id, 1);
				//ReconnId[id]=1;
			}else if(g_ethCmd_rx_sockCliHandle[path_id]==-3){//Connection refused
				DBG_DUMP("Conn refused [%d][%d]\r\n",path_id,id);
				g_ethCmd_rx_sockCliHandle[path_id]=0;
				Delay_DelayMs(10);
				EthCamSocketCli_ReConnect(path_id, id, 1);
				//ReconnId[id]=1;
			//}else if(g_ethCmd_rx_sockCliHandle[path_id] != 0){
			}else if(g_ethCmd_rx_sockCliHandle[path_id] > 0){
				DBG_IND("EthCamSocketCli_Cmd Connect OK\r\n");
				EthsockCliIpc_Start(path_id, id);
				g_EthCamSocketCli_CmdSocketClose[path_id]=0;
				//if(socketcli_reconnect_pathid==path_id && socketcli_reconnect_socketid==id){
				if(socketcli_reconnect_pathid[path_id] && socketcli_reconnect_socketid[path_id][id]){
					EthCamSocketCli_ReConnect(path_id, id, 0);
				}
				if(g_EthCamSocketCli_CmdNotifyCb[path_id]){
					g_EthCamSocketCli_CmdNotifyCb[path_id](path_id, CYG_ETHSOCKETCLI_STATUS_CLIENT_CONNECT ,0);
				}
			}else if(g_ethCmd_rx_sockCliHandle[path_id] == 0){
				EthsockCliIpc_Close(path_id ,id);
			}
		}
	}
}
void EthCamSocketCli_Close(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id)
{
	if(id== ETHSOCKIPCCLI_ID_0 && g_ethData1_rx_sockCliHandle[path_id]){
		EthsockCliIpc_Stop(path_id, ETHSOCKIPCCLI_ID_0);//stop socket thread
		EthsockCliIpc_Disconnect(path_id, ETHSOCKIPCCLI_ID_0, (INT32*)&g_ethData1_rx_sockCliHandle[path_id]);//close socket, free g_ethData_rx_usockCliHandle
		DBG_IND("EthCamSocketCliData1 Close OK\r\n");
		g_ethData1_rx_sockCliHandle[path_id]=0;
		memset(&EthsockCliIpcData_Open[path_id][ETHSOCKIPCCLI_ID_0], 0, sizeof(ETHSOCKCLIIPC_OPEN));
		EthsockCliIpc_Close(path_id ,ETHSOCKIPCCLI_ID_0);//close ipc thread
		if(g_EthCamSocketCli_Data1NotifyCb[path_id]){
			g_EthCamSocketCli_Data1NotifyCb[path_id](path_id, CYG_ETHSOCKETCLI_STATUS_CLIENT_DISCONNECT ,0);
		}
	}
	if(id== ETHSOCKIPCCLI_ID_1 && g_ethData2_rx_sockCliHandle[path_id]){
		EthsockCliIpc_Stop(path_id, ETHSOCKIPCCLI_ID_1);
		EthsockCliIpc_Disconnect(path_id, ETHSOCKIPCCLI_ID_1, (INT32*)&g_ethData2_rx_sockCliHandle[path_id]);//free g_ethData_rx_usockCliHandle
		DBG_IND("EthCamSocketCliData2 Close OK\r\n");
		g_ethData2_rx_sockCliHandle[path_id]=0;
		memset(&EthsockCliIpcData_Open[path_id][ETHSOCKIPCCLI_ID_1], 0, sizeof(ETHSOCKCLIIPC_OPEN));
		EthsockCliIpc_Close(path_id ,ETHSOCKIPCCLI_ID_1);
		if(g_EthCamSocketCli_Data2NotifyCb[path_id]){
			g_EthCamSocketCli_Data2NotifyCb[path_id](path_id, CYG_ETHSOCKETCLI_STATUS_CLIENT_DISCONNECT ,0);
		}
	}
	if(id== ETHSOCKIPCCLI_ID_2 && g_ethCmd_rx_sockCliHandle[path_id]){
		EthsockCliIpc_Stop(path_id, ETHSOCKIPCCLI_ID_2);
		EthsockCliIpc_Disconnect(path_id, ETHSOCKIPCCLI_ID_2, (INT32*)&g_ethCmd_rx_sockCliHandle[path_id]);//free g_ethCmd_rx_usockCliHandle
		DBG_IND("EthCamSocketCliCmd Close OK\r\n");
		g_ethCmd_rx_sockCliHandle[path_id]=0;
		memset(&EthsockCliIpcCmd_Open[path_id], 0, sizeof(ETHSOCKCLIIPC_OPEN));
		EthsockCliIpc_Close(path_id ,ETHSOCKIPCCLI_ID_2);
		if(g_EthCamSocketCli_CmdNotifyCb[path_id]){
			g_EthCamSocketCli_CmdNotifyCb[path_id](path_id, CYG_ETHSOCKETCLI_STATUS_CLIENT_DISCONNECT ,0);
		}
	}
	EthCamSocketCli_HandleTskClose();
	if(g_EthCamSocketCli_ReConnecTimerId != SWTIMER_NUM){
		SwTimer_Stop(g_EthCamSocketCli_ReConnecTimerId);
		SwTimer_Close(g_EthCamSocketCli_ReConnecTimerId);
		g_EthCamSocketCli_ReConnecTimerId=SWTIMER_NUM;
	}
}
int EthCamSocketCli_DataSend(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, char* addr, int *size)
{
	ER Result;
	if(id< ETHSOCKIPCCLI_ID_0 || id>ETHSOCKIPCCLI_ID_1){
		DBG_ERR("id out of range ,id=%d\r\n", id);
		return -1;
	}
	if(id == ETHSOCKIPCCLI_ID_0)
	{
		Result=EthsockCliIpc_Send(path_id, ETHSOCKIPCCLI_ID_0 ,g_ethData1_rx_sockCliHandle[path_id], addr, size);
		if (E_OK !=Result){
			//DBG_ERR("Result=%d\r\n", Result);
			return -1;
		}else{
			return 0;
		}
	}else{
		Result=EthsockCliIpc_Send(path_id, ETHSOCKIPCCLI_ID_1 ,g_ethData2_rx_sockCliHandle[path_id], addr, size);
		if (E_OK !=Result){
			//DBG_ERR("Result=%d\r\n", Result);
			return -1;
		}else{
			return 0;
		}
	}
}
int EthCamSocketCli_CmdSend(ETHCAM_PATH_ID path_id, char* addr, int *size)
{
	ER Result;
	Result=EthsockCliIpc_Send(path_id, ETHSOCKIPCCLI_ID_2 ,g_ethCmd_rx_sockCliHandle[path_id], addr, size);
	if (E_OK !=Result){
		//DBG_ERR("Result=%d\r\n", Result);
		return -1;
	}else{
		return 0;
	}
}
void  EthCamSocketCli_DataSetRawEncodeBuff(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, ETHCAM_SOCKET_BUF_OBJ *BufObj)
{
	if(BufObj->ParamAddr== 0){
		DBG_ERR("ParamAddr=0!!\r\n");
		return;
	}
	if(BufObj->ParamSize==0){
		DBG_ERR("ParamSize=0!!\r\n");
		return;
	}
	if(BufObj->ParamSize==0){
		DBG_ERR("ParamSize=0!!\r\n");
		return;
	}
	if(id< ETHSOCKIPCCLI_ID_0 || id>ETHSOCKIPCCLI_ID_1){
		DBG_ERR("id out of range ,id=%d\r\n", id);
		return;
	}
	g_SocketCliData_RawEncodeAddr[path_id][id]=BufObj->ParamAddr;
	g_SocketCliData_RawEncodeSize[path_id][id]=BufObj->ParamSize;
}
ETHCAM_SOCKET_BUF_OBJ  EthCamSocketCli_DataGetRawEncodeBufObj(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id)
{
	ETHCAM_SOCKET_BUF_OBJ BufObj={0};
	if(id< ETHSOCKIPCCLI_ID_0 || id>ETHSOCKIPCCLI_ID_1){
		DBG_ERR("id out of range ,id=%d\r\n", id);
		return BufObj;
	}

	if(g_SocketCliData_RawEncodeAddr[path_id][id]== 0){
		DBG_ERR("RawEncodeAddr=0!!\r\n");
		return BufObj;
	}
	if(g_SocketCliData_RawEncodeSize[path_id][id]==0){
		DBG_ERR("RawEncodeSize=0!!\r\n");
		return BufObj;
	}

	BufObj.ParamAddr=g_SocketCliData_RawEncodeAddr[path_id][id];
	BufObj.ParamSize=g_SocketCliData_RawEncodeSize[path_id][id];
	return BufObj;
}
void  EthCamSocketCli_DataSetBsBuff(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, ETHCAM_SOCKET_BUF_OBJ *BufObj)
{
	if(id< ETHSOCKIPCCLI_ID_0 || id>ETHSOCKIPCCLI_ID_1){
		DBG_ERR("id out of range ,id=%d\r\n", id);
		return;
	}

	if(BufObj->ParamAddr== 0){
		DBG_ERR("ParamAddr=0!!\r\n");
		return;
	}
	if(BufObj->ParamSize==0){
		DBG_ERR("ParamSize=0!!\r\n");
		return;
	}

	g_SocketCliData_BsBufTotalAddr[path_id][id]=BufObj->ParamAddr;
	g_SocketCliData_BsBufTotalSize[path_id][id]=BufObj->ParamSize;
}
ETHCAM_SOCKET_BUF_OBJ  EthCamSocketCli_DataGetBsBufObj(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id)
{
	ETHCAM_SOCKET_BUF_OBJ BufObj={0};
	if(id< ETHSOCKIPCCLI_ID_0 || id>ETHSOCKIPCCLI_ID_1){
		DBG_ERR("id out of range ,id=%d\r\n", id);
		return BufObj;
	}

	if(g_SocketCliData_BsBufTotalAddr[path_id][id]== 0){
		DBG_ERR("BsBufTotalAddr=0!!\r\n");
		return BufObj;
	}
	if(g_SocketCliData_BsBufTotalSize[path_id][id]==0){
		DBG_ERR("BsBufTotalSize=0!!\r\n");
		return BufObj;
	}
	BufObj.ParamAddr=g_SocketCliData_BsBufTotalAddr[path_id][id];
	BufObj.ParamSize=g_SocketCliData_BsBufTotalSize[path_id][id];
	return BufObj;
}
void  EthCamSocketCli_DataSetBsQueueMax(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id,UINT32 BsQMax)
{
	if(id< ETHSOCKIPCCLI_ID_0 || id>ETHSOCKIPCCLI_ID_1){
		DBG_ERR("id out of range ,id=%d\r\n", id);
		return;
	}

	g_SocketCliData_BsQueueMax[path_id][id]=BsQMax;
}
UINT32  EthCamSocketCli_DataGetBsQueueMax(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id)
{
	if(id< ETHSOCKIPCCLI_ID_0 || id>ETHSOCKIPCCLI_ID_1){
		DBG_ERR("id out of range ,id=%d\r\n", id);
		return 0;
	}

	return g_SocketCliData_BsQueueMax[path_id][id];
}
void EthCamSocketCli_SetDataRecvCB(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, UINT32 value)
{
	if(id< ETHSOCKIPCCLI_ID_0 || id>ETHSOCKIPCCLI_ID_1){
		DBG_ERR("id out of range ,id=%d\r\n", id);
		return;
	}

	if (value != 0) {
		if(id== ETHSOCKIPCCLI_ID_0 ){
			g_EthCamSocketCli_Data1RecvCb[path_id] = (ETHCAMSOCKET_CLI_RECV_PROCESS_CB) value;
		}else{
			g_EthCamSocketCli_Data2RecvCb[path_id] = (ETHCAMSOCKET_CLI_RECV_PROCESS_CB) value;
		}
	}
}
void EthCamSocketCli_SetCmdRecvCB(ETHCAM_PATH_ID path_id, UINT32 value)
{
	if (value != 0) {
		g_EthCamSocketCli_CmdRecvCb[path_id] = (ETHCAMSOCKET_CLI_RECV_PROCESS_CB) value;
	}
}
void EthCamSocketCli_SetDataNotifyCB(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, UINT32 value)
{
	if(id< ETHSOCKIPCCLI_ID_0 || id>ETHSOCKIPCCLI_ID_1){
		DBG_ERR("id out of range ,id=%d\r\n", id);
		return;
	}

	if (value != 0) {
		if(id== ETHSOCKIPCCLI_ID_0 ){
			g_EthCamSocketCli_Data1NotifyCb[path_id] = (ETHCAMSOCKET_CLI_NOTIFY_PROCESS_CB) value;
		}else{
			g_EthCamSocketCli_Data2NotifyCb[path_id] = (ETHCAMSOCKET_CLI_NOTIFY_PROCESS_CB) value;
		}
	}
}
void EthCamSocketCli_SetCmdNotifyCB(ETHCAM_PATH_ID path_id, UINT32 value)
{
	if (value != 0) {
		g_EthCamSocketCli_CmdNotifyCb[path_id] = (ETHCAMSOCKET_CLI_NOTIFY_PROCESS_CB) value;
	}
}
void EthCamSocketCli_SetCmdSendSizeCB(ETHCAM_PATH_ID path_id, UINT32 value)
{
	//if (value != 0) {
		g_EthCamSocketCli_CmdSendSizeCb[path_id] = (ETHCAMSOCKET_CMD_SEND_DATA_SIZE_CB) value;
	//}
}
void EthCamSocketCli_SetSvrInfo(ETHCAM_SOCKET_INFO* info, UINT32 num_info)
{
	UINT16 i=0;
	for(i=0;i<num_info;i++){
		memcpy((void*)&g_EthCamSocketCli_SvrInfo[i], (void*)&info[i], sizeof(ETHCAM_SOCKET_INFO));
	}
}

//#define MAX_DATA_SIZE (230 * 1024)
#define MAX_DATA_SIZE  (600*1024)
#define MAX_CMD_SIZE (1 * 1024)
#define MAX_RETRY_CNT (10000)
int EthCamSocket_Send(ETHCAM_PATH_ID path_id, ETHSOCKET_ID id, char* addr, int *size)
{
	INT32 result=-1;
	UINT32 retryCnt=0;
	UINT32 retryCntMax=MAX_RETRY_CNT;

	INT32 total = 0;
	INT32 MaxSize=MAX_DATA_SIZE;
	UINT32 DelayTime=1;
	if(id == ETHSOCKET_CMD){
		MaxSize=MAX_CMD_SIZE;
		DelayTime=1;//15;
	}
	if( id ==ETHSOCKETCLI_CMD){
		MaxSize=MAX_CMD_SIZE;
		DelayTime=15;
	}

	INT32 bytesleft = (*size > MaxSize) ? MaxSize : *size;

	do {
		if(g_bEthLinkStatus[path_id]==ETHCAM_LINK_DOWN){
			DBG_ERR("not conn ,path_id=%d\r\n", path_id);
			return -1;
		}

		if(g_bEthHubLinkSta[path_id]==0){
			DBG_ERR("hub not conn ,path_id=%d\r\n", path_id);
			return -1;
		}

		if(id ==ETHSOCKETCLI_CMD && (g_ethCmd_rx_sockCliHandle[path_id]==0|| g_EthCamSocketCli_CmdSocketClose[path_id]==1)){
			DBG_ERR("cmd socket close ,path_id=%d, %d, %d\r\n", path_id, g_ethCmd_rx_sockCliHandle[path_id],g_EthCamSocketCli_CmdSocketClose[path_id]);
			return -1;
		}

		if(id ==ETHSOCKETCLI_DATA1){
			if(g_ethData1_rx_sockCliHandle[path_id]==0|| g_EthCamSocketCli_Data1SocketClose[path_id]==1){
				return -1;
			}
		}
		if(id ==ETHSOCKETCLI_DATA2){
			if(g_ethData2_rx_sockCliHandle[path_id]==0||g_EthCamSocketCli_Data2SocketClose[path_id]==1){
				return -1;
			}
		}
		//Tx
		if(id ==ETHSOCKET_CMD && (g_ethCmd_tx_sockHandle==-1)){
			return -1;
		}

		if(id ==ETHSOCKET_DATA1){
			if(g_ethData1_tx_sockHandle==-1){
				DBG_ERR("[%d]g_ethData1_tx_sockHandle -1\r\n", path_id);
				return -1;
			}
		}
		if(id ==ETHSOCKET_DATA2){
			if(g_ethData2_tx_sockHandle==-1){
				DBG_ERR("[%d]g_ethData2_tx_sockHandle -1\r\n", path_id);
				return -1;
			}
		}
		if(id>=ETHSOCKET_DATA1 && id <= ETHSOCKET_CMD){
			switch(id){
				case ETHSOCKET_DATA1:
				default:
					result = (INT32)EthCamSocket_DataSend(ETHSOCKIPC_ID_0, (char *)(addr + total), (int*)&bytesleft);
					break;
				case ETHSOCKET_DATA2:
					result = (INT32)EthCamSocket_DataSend(ETHSOCKIPC_ID_1, (char *)(addr + total), (int*)&bytesleft);
					break;
				case ETHSOCKET_CMD:
					result = (INT32)EthCamSocket_CmdSend((char *)(addr + total), (int*)&bytesleft);
					break;
			}
		}else if(id>=ETHSOCKETCLI_DATA1 && id <= ETHSOCKETCLI_CMD){
			switch(id){
				case ETHSOCKETCLI_DATA1:
				default:
					result = (INT32)EthCamSocketCli_DataSend(path_id, ETHSOCKIPCCLI_ID_0, (char *)(addr + total), (int*)&bytesleft);
					break;
				case ETHSOCKETCLI_DATA2:
					result = (INT32)EthCamSocketCli_DataSend(path_id, ETHSOCKIPCCLI_ID_1, (char *)(addr + total), (int*)&bytesleft);
					break;
				case ETHSOCKETCLI_CMD:
					result = (INT32)EthCamSocketCli_CmdSend(path_id, (char *)(addr + total), (int*)&bytesleft);
					break;
			}
		}else{
			DBG_ERR("id out of range ,id=%d\r\n", id);
			return result;
		}
		if (bytesleft == 0 || result ==-1) {
			//if((retryCnt % 200) ==0){
			if(retryCnt && (retryCnt % 10) ==0){
				//DBG_DUMP("-->Sent fail[%d][%d][%d][%d]\r\n",id,retryCnt,*size, total);
				DBG_DUMP("reSend P=%d, RSCnt=%d, TolSz=%d, SSz=%d\r\n",id,retryCnt,*size, total);
			}
			if(retryCnt >retryCntMax){//MAX_RETRY_CNT){
				//DBG_DUMP("Sent exceed MAX %d\r\n",retryCntMax);
				break;
			}
			retryCnt++;
		}

		total += bytesleft;
		if(id ==ETHSOCKETCLI_CMD){
			if(g_EthCamSocketCli_CmdSendSizeCb[path_id]){
				g_EthCamSocketCli_CmdSendSizeCb[path_id](total, *size);
			}
		}
		bytesleft = ((*size) > (MaxSize + total))? MaxSize : (*size - total);
		if (total < *size) {
			if(DelayTime){
				Delay_DelayMs(DelayTime);
			}
		}
	} while (total < *size );
	return result;
}
void EthCamSocket_SetEthLinkStatus(ETHCAM_PATH_ID path_id, UINT32 bLink)
{
	g_bEthLinkStatus[path_id]=bLink;
}
UINT32 EthCamSocket_GetEthLinkStatus(ETHCAM_PATH_ID path_id)
{
	return g_bEthLinkStatus[path_id];
}
void EthCamSocket_SetEthHubLinkSta(ETHCAM_PATH_ID path_id, UINT32 bLink)
{
	g_bEthHubLinkSta[path_id]=bLink;
}

void EthCamSocketCli_SetDescSize(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, UINT32 Size)
{
	if(id< ETHSOCKIPCCLI_ID_0 || id>ETHSOCKIPCCLI_ID_1){
		DBG_ERR("id out of range ,id=%d\r\n", id);
		return;
	}

	if (Size != 0) {
		if(id== ETHSOCKIPCCLI_ID_0 ){
			g_EthCamSocketCli_Data1DescSize[path_id] = Size;
		}else{
			g_EthCamSocketCli_Data2DescSize[path_id] = Size;
		}
	}
}
UINT32 EthCamSocket_Checksum(UINT8 *buf, UINT32 len)
{
	UINT32 checksum = 0;
	UINT32 i;
	for(i=0;i<len;i++){
		checksum+= *(buf+i);
	}
	return checksum;
}

THREAD_RETTYPE EthCamSocketCli_HandleTsk(void)
{
	FLGPTN       uiFlag = 0;
	UINT32       i, j;
	//kent_tsk();
	THREAD_ENTRY();

	DBG_FUNC_BEGIN("\r\n");
	g_IsHandleTskRunning=1;
	while (ETHCAMSOCKETCLI_HANDLE_TSK_ID) {
		set_flg(ETHCAMSOCKETCLI_HANDLE_FLG_ID, FLG_ETHCAMSOCKETCLI_IDLE);
		PROFILE_TASK_IDLE();
		wai_flg(&uiFlag, ETHCAMSOCKETCLI_HANDLE_FLG_ID, FLG_ETHCAMSOCKETCLI_STOP|FLG_ETHCAMSOCKETCLI_RECONN, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		clr_flg(ETHCAMSOCKETCLI_HANDLE_FLG_ID, FLG_ETHCAMSOCKETCLI_IDLE);
		if (uiFlag & FLG_ETHCAMSOCKETCLI_STOP){
			set_flg(ETHCAMSOCKETCLI_HANDLE_FLG_ID, FLG_ETHCAMSOCKETCLI_IDLE);
			break;
		}
		if (uiFlag & FLG_ETHCAMSOCKETCLI_RECONN) {
			//DBG_DUMP("FLG_ETHCAMSOCKETCLI_RECONN\r\n");

			for(i=0;i<ETHCAM_PATH_ID_MAX;i++){
				for(j=0;j<ETHCAM_PORT_DATA_MAX;j++){
					//DBG_DUMP("[%d][%d] p=%d, s=%d ,cnt=%d\r\n",i,j,socketcli_reconnect_pathid[i] ,socketcli_reconnect_socketid[i][j], socketcli_reconnect_count[i][j]);

					if(socketcli_reconnect_pathid[i] && socketcli_reconnect_socketid[i][j] && socketcli_reconnect_count[i][j]){
						DBG_DUMP("EthCamSocketCli_HandleTsk RECONN[%d][%d][%d]\r\n",i,j,socketcli_reconnect_count[i][j]);
						EthCamSocketCli_ReConnect(i, j, 1);
					}
				}
			}
		}
	}
	g_IsHandleTskRunning=0;
	DBG_FUNC_END("\r\n");
	//ext_tsk();
	THREAD_RETURN(0);

}
void EthCamSocketCli_InstallID(void)
{
	OS_CONFIG_FLAG(ETHCAMSOCKETCLI_HANDLE_FLG_ID);
	//OS_CONFIG_TASK(ETHCAMSOCKETCLI_HANDLE_TSK_ID,    PRI_ETHCAM_ETHCAMSOCKETCLI_HANDLE,   STKSIZE_ETHCAMSOCKETCLI_HANDLE,   EthCamSocketCli_HandleTsk);
}
void EthCamSocketCli_UnInstallID(void)
{
	rel_flg(ETHCAMSOCKETCLI_HANDLE_FLG_ID);
	//OS_CONFIG_TASK(ETHCAMSOCKETCLI_HANDLE_TSK_ID,    PRI_ETHCAM_ETHCAMSOCKETCLI_HANDLE,   STKSIZE_ETHCAMSOCKETCLI_HANDLE,   EthCamSocketCli_HandleTsk);
}

int EthCamSocketCli_HandleTskOpen(void)
{
	if (g_bEthCamSocketCliTskOpened==1) {
		return -1;
	}
	EthCamSocketCli_InstallID();
	clr_flg(ETHCAMSOCKETCLI_HANDLE_FLG_ID, 0xFFFFFFFF);


	//sta_tsk(ETHCAMSOCKETCLI_HANDLE_TSK_ID, 0);
	ETHCAMSOCKETCLI_HANDLE_TSK_ID=vos_task_create(EthCamSocketCli_HandleTsk,  0, "EthCamSCliHTsk",   PRI_ETHCAM_ETHCAMSOCKETCLI_HANDLE,	STKSIZE_ETHCAMSOCKETCLI_HANDLE);
	vos_task_resume(ETHCAMSOCKETCLI_HANDLE_TSK_ID);

	g_bEthCamSocketCliTskOpened=1;
	return 0;
}
int EthCamSocketCli_HandleTskClose(void)
{
	FLGPTN FlgPtn;
	UINT32 delay_cnt;

	if (g_bEthCamSocketCliTskOpened==0) {
		return -1;
	}

	// stop task
	//set_flg(ETHCAMSOCKETCLI_HANDLE_FLG_ID, FLG_ETHCAMSOCKETCLI_STOP);
	vos_flag_set(ETHCAMSOCKETCLI_HANDLE_FLG_ID, FLG_ETHCAMSOCKETCLI_STOP);
	//wai_flg(&FlgPtn, ETHCAMSOCKETCLI_HANDLE_FLG_ID, FLG_ETHCAMSOCKETCLI_IDLE, TWF_ORW | TWF_CLR);
	vos_flag_wait_timeout(&FlgPtn, ETHCAMSOCKETCLI_HANDLE_FLG_ID, FLG_ETHCAMSOCKETCLI_IDLE, TWF_ORW | TWF_CLR, vos_util_msec_to_tick(1000));


	delay_cnt = 50;
	while (g_IsHandleTskRunning && delay_cnt) {
		vos_util_delay_ms(10);
		delay_cnt --;
	}
	//ter_tsk(ETHCAMSOCKETCLI_HANDLE_TSK_ID);
	if(g_IsHandleTskRunning){
		vos_task_destroy(ETHCAMSOCKETCLI_HANDLE_TSK_ID);
	}
	ETHCAMSOCKETCLI_HANDLE_TSK_ID=0;
	EthCamSocketCli_UnInstallID();

	g_bEthCamSocketCliTskOpened=0;
	return 0;
}


