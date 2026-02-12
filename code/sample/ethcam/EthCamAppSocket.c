#include "ethcam_test.h"

#include "EthCamAppCmd.h"
#include "EthCamAppSocket.h"
#include "EthCam/EthCamSocket.h"
#include <comm/hwclock.h>
#include "EthCamAppNetwork.h"
#include <kwrap/stdio.h>
#include "BackgroundObj.h"
#include "hd_common.h"

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          EthCamAppSocket
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

#define TBR_SIZE_RATIO  170//220//240

ETHCAM_SENDCMD_INFO sEthCamSendCmdInfo={0};

#define POOL_CNT_ETHSOCKCLI_IPC       4*1
#define POOL_SIZE_ETHSOCKCLI_IPC 	 	(48*1024 + 8)//(2*1024+8)

#define MovieExe_GetEthcamEncBufSec(x)    3
#define MovieExe_GetEmrRollbackSec()    3

static UINT8 g_sharedMemAddr[POOL_SIZE_ETHSOCKCLI_IPC*POOL_CNT_ETHSOCKCLI_IPC];

extern UINT32 EthCamHB1[ETHCAM_PATH_ID_MAX], EthCamHB2;
//extern MOVIE_RECODE_FILE_OPTION gMovie_Rec_Option;

static UINT32 g_SocketCliData1_RecvAddr[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData1_BsBufTotalAddr[ETHCAM_PATH_ID_MAX]={0}; //tbl Buf+BsFrmBuf
static UINT32 g_SocketCliData1_BsBufTotalSize[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData1_BsQueueMax[ETHCAM_PATH_ID_MAX]={0};

UINT32 g_SocketCliData1_RawEncodeAddr[ETHCAM_PATH_ID_MAX]={0};

static UINT32 g_SocketCliData1_BsFrameCnt[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData1_RecBsFrameCnt[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData1_eth_i_cnt[ETHCAM_PATH_ID_MAX]={0};
static UINT32 g_SocketCliData1_LongCntTxRxOffset[ETHCAM_PATH_ID_MAX]={0};

static ETHCAM_ADDR_INFO g_SocketCliData1_RecvPoolAddr[ETHCAM_PATH_ID_MAX][1]={0};
static ETHCAM_ADDR_INFO g_SocketCliData1_RawEncodePoolAddr[ETHCAM_PATH_ID_MAX][1]={0};
static ETHCAM_ADDR_INFO g_SocketCliData1_BsPoolAddr[ETHCAM_PATH_ID_MAX][1]={0};


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

		if (g_SocketCliData1_RecvPoolAddr[i][0].pool_va != 0) {
			ret = hd_common_mem_free((UINT32)g_SocketCliData1_RecvPoolAddr[i][0].pool_pa, (void *)g_SocketCliData1_RecvPoolAddr[i][0].pool_va);
			if (ret != HD_OK) {
				DBG_ERR("FileIn release blk failed! (%d)\r\n", ret);
				break;
			}
			g_SocketCliData1_RecvPoolAddr[i][0].pool_va = 0;
			g_SocketCliData1_RecvPoolAddr[i][0].pool_pa = 0;
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
			DBG_ERR("alloc fail ret=%d, size 0x%x, ddr %d\r\n", ret, blk_size, ddr_id);
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
			DBG_ERR("alloc fail ret=%d, size 0x%x, ddr %d\r\n", ret,blk_size, ddr_id);
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
void socketCliEthData1_GetBsBufInfo(ETHCAM_PATH_ID path_id, UINT32* Addr, UINT32* Size)
{
	*Addr=g_SocketCliData1_BsPoolAddr[path_id][0].pool_pa;
	*Size=g_SocketCliData1_BsBufTotalSize[path_id];
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
	UINT32 JpgCompRatio = 10; // JPEG compression ratio
	UINT32 MaxRawEncodeSize = sEthCamTxRecInfo[path_id].width * sEthCamTxRecInfo[path_id].height * 3 / (2 * JpgCompRatio);
	//MaxRawEncodeSize*= 13/10;
	MaxRawEncodeSize= (MaxRawEncodeSize*13)/10;
	DBG_IND("MaxRawEncodeSize=%d\r\n",MaxRawEncodeSize);
	if((g_SocketCliData1_RawEncodeAddr[path_id]=socketCliEthData1_GetRawEncodeBufAddr(path_id, MaxRawEncodeSize))==0){
		return -1;
	}
	//TBR* (EMR RollbackSec + 3) + MapTbl= FPS* (EMR RollbackSec + 3)
	g_SocketCliData1_BsBufTotalSize[path_id] = (MovieExe_GetEthcamEncBufSec(path_id) + MovieExe_GetEmrRollbackSec()) * sEthCamTxRecInfo[path_id].tbr + sizeof(UINT32) * (MovieExe_GetEthcamEncBufSec(path_id) + MovieExe_GetEmrRollbackSec()) * sEthCamTxRecInfo[path_id].vfr;
	if (g_SocketCliData1_BsBufTotalSize[path_id]==0) {
		DBG_ERR("EthBsFrmBufTotalSize[%d] =0 !!\r\n",path_id);
	}
	if((g_SocketCliData1_BsBufTotalAddr[path_id]=socketCliEthData1_GetBsBufAddr(path_id, g_SocketCliData1_BsBufTotalSize[path_id]))==0){
		DBG_ERR("BsBuf fail, want buf size=%d\r\n",g_SocketCliData1_BsBufTotalSize[path_id]);
		return -1;
	}
	DBG_IND("g_SocketCliData1_BsBufTotalAddr[%d]=0x%x, g_SocketCliData1_BsBufTotalSize[%d]=%d\r\n",path_id,g_SocketCliData1_BsBufTotalAddr[path_id],path_id,g_SocketCliData1_BsBufTotalSize[path_id]);

	ETHCAM_SOCKET_BUF_OBJ BufObj={0};
	BufObj.ParamAddr=g_SocketCliData1_BsBufTotalAddr[path_id];
	BufObj.ParamSize=g_SocketCliData1_BsBufTotalSize[path_id];
	EthCamSocketCli_DataSetBsBuff(path_id, ETHSOCKIPCCLI_ID_0, &BufObj);

	BufObj.ParamAddr=g_SocketCliData1_RawEncodeAddr[path_id];
	BufObj.ParamSize=MaxRawEncodeSize;
	EthCamSocketCli_DataSetRawEncodeBuff(path_id, ETHSOCKIPCCLI_ID_0, &BufObj);
	g_SocketCliData1_BsQueueMax[path_id]=((MovieExe_GetEthcamEncBufSec(path_id)+ MovieExe_GetEmrRollbackSec())* sEthCamTxRecInfo[path_id].vfr);
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
		if((g_SocketCliData2_RecvAddr[path_id]=socketCliEthData2_GetRecvBufAddr(path_id, (sEthCamTxDecInfo[path_id].Tbr*100/TBR_SIZE_RATIO)))==0){
			return -1;
		}
	}else{
		if((g_SocketCliData2_RecvAddr[path_id]=socketCliEthData2_GetRecvBufAddr(path_id, MAX_I_FRAME_SZIE))==0){
			return -1;
		}
	}
	//TBR* (EMR RollbackSec + 3) + MapTbl= FPS* (EMR RollbackSec + 3)
	//g_SocketCliData2_BsBufTotalSize[path_id]  = (MovieExe_GetEthcamEncBufSec(path_id)+ MovieExe_GetEmrRollbackSec())*sEthCamTxDecInfo[path_id].Tbr+ sizeof(UINT32)*(MovieExe_GetEthcamEncBufSec(path_id)+ MovieExe_GetEmrRollbackSec())* sEthCamTxDecInfo[path_id].Fps;
	g_SocketCliData2_BsBufTotalSize[path_id]  = (1)*sEthCamTxDecInfo[path_id].Tbr+ sizeof(UINT32)*(1)* sEthCamTxDecInfo[path_id].Fps;
	if(g_SocketCliData2_BsBufTotalSize[path_id]==0){
		DBG_ERR("EthBsFrmBufTotalSize =0 !!\r\n");
	}
	if((g_SocketCliData2_BsBufTotalAddr[path_id]=socketCliEthData2_GetBsBufAddr(path_id, g_SocketCliData2_BsBufTotalSize[path_id]))==0){
		DBG_ERR("BsBuf fail, want buf size=%d\r\n",g_SocketCliData2_BsBufTotalSize[path_id]);
		return -1;
	}
	ETHCAM_SOCKET_BUF_OBJ BufObj={0};
	BufObj.ParamAddr=g_SocketCliData2_BsBufTotalAddr[path_id];
	BufObj.ParamSize=g_SocketCliData2_BsBufTotalSize[path_id];
	EthCamSocketCli_DataSetBsBuff(path_id, ETHSOCKIPCCLI_ID_1, &BufObj);

	//g_SocketCliData2_BsQueueMax[path_id]=((MovieExe_GetEthcamEncBufSec(path_id)+ MovieExe_GetEmrRollbackSec())* sEthCamTxDecInfo[path_id].Fps);
	g_SocketCliData2_BsQueueMax[path_id]=((1)* sEthCamTxDecInfo[path_id].Fps);
	EthCamSocketCli_DataSetBsQueueMax(path_id, ETHSOCKIPCCLI_ID_1, g_SocketCliData2_BsQueueMax[path_id]);
#endif
	return 0;
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
		EthsockCliIpcOpen[path_id][id].sharedSendMemAddr=(UINT32)&g_sharedMemAddr[(POOL_CNT_ETHSOCKCLI_IPC/1)*path_id*POOL_SIZE_ETHSOCKCLI_IPC]  ;
		EthsockCliIpcOpen[path_id][id].sharedSendMemSize=POOL_SIZE_ETHSOCKCLI_IPC;
		if(sEthCamTxRecInfo[path_id].tbr!=0 && sEthCamTxDecInfo[path_id].bStarupOK==1){
			g_SocketCliData1_RecvAddr[path_id]=socketCliEthData1_GetRecvBufAddr(path_id, (sEthCamTxRecInfo[path_id].tbr *100/TBR_SIZE_RATIO));
			EthsockCliIpcOpen[path_id][id].sharedRecvMemSize=(sEthCamTxRecInfo[path_id].tbr*100/TBR_SIZE_RATIO);
		}else{
			g_SocketCliData1_RecvAddr[path_id]=socketCliEthData1_GetRecvBufAddr(path_id, MAX_I_FRAME_SZIE);
			EthsockCliIpcOpen[path_id][id].sharedRecvMemSize=MAX_I_FRAME_SZIE;
		}
		EthsockCliIpcOpen[path_id][id].sharedRecvMemAddr=g_SocketCliData1_RecvAddr[path_id];
		//DBG_DUMP("sEthCamTxRecInfo.tbr=%d, sharedRecvMemSize%d\r\n", sEthCamTxRecInfo.tbr,EthsockCliIpcOpen[id].sharedRecvMemSize );
		DBG_IND("Cli[%d] D1 SendMemAddr=0x%x, RecvMemAddr=0x%x\r\n",path_id,EthsockCliIpcOpen[path_id][id].sharedSendMemAddr,EthsockCliIpcOpen[path_id][id].sharedRecvMemAddr);
		EthCamSocketCli_Open(path_id,  id, &EthsockCliIpcOpen[path_id][id]);
	}else if(id== ETHSOCKIPCCLI_ID_1){
#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
		if(socketCliEthData2_ConfigRecvBuf(path_id)==-1){
			return;
		}
	    	EthCamSocketCli_SetDataRecvCB(path_id, ETHSOCKIPCCLI_ID_1, (UINT32)&socketCliEthData2_RecvCB);
		EthCamSocketCli_SetDataNotifyCB(path_id, ETHSOCKIPCCLI_ID_1, (UINT32)&socketCliEthData2_NotifyCB);
		socketCliEthData2_RecvResetParam(path_id);
		// open
		EthsockCliIpcOpen[path_id][id].sharedSendMemAddr=(UINT32)&g_sharedMemAddr[(POOL_CNT_ETHSOCKCLI_IPC/1)*path_id*POOL_SIZE_ETHSOCKCLI_IPC +POOL_SIZE_ETHSOCKCLI_IPC];
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
#endif
	}
}
void socketCliEthCmd_Open(ETHCAM_PATH_ID path_id)
{
	ETHSOCKCLIIPC_OPEN EthsockCliIpcOpen[ETHCAM_PATH_ID_MAX] = {0};

    	EthCamSocketCli_SetCmdRecvCB(path_id, (UINT32)&socketCliEthCmd_RecvCB);
    	EthCamSocketCli_SetCmdNotifyCB(path_id, (UINT32)&socketCliEthCmd_NotifyCB);
	EthsockCliIpcOpen[path_id].sharedSendMemAddr=(UINT32)&g_sharedMemAddr[(POOL_CNT_ETHSOCKCLI_IPC/1)*path_id*POOL_SIZE_ETHSOCKCLI_IPC +2*POOL_SIZE_ETHSOCKCLI_IPC];
	EthsockCliIpcOpen[path_id].sharedSendMemSize=POOL_SIZE_ETHSOCKCLI_IPC;

	EthsockCliIpcOpen[path_id].sharedRecvMemAddr=(UINT32)&g_sharedMemAddr[(POOL_CNT_ETHSOCKCLI_IPC/1)*path_id*POOL_SIZE_ETHSOCKCLI_IPC +3*POOL_SIZE_ETHSOCKCLI_IPC];
	EthsockCliIpcOpen[path_id].sharedRecvMemSize=POOL_SIZE_ETHSOCKCLI_IPC;
	DBG_IND("Cli[%d] Cmd SendMemAddr=0x%x, RecvMemAddr=0x%x\r\n",path_id,EthsockCliIpcOpen[path_id].sharedSendMemAddr,EthsockCliIpcOpen[path_id].sharedRecvMemAddr);
	EthCamSocketCli_ReConnect(path_id, ETHSOCKIPCCLI_ID_2, 0);
	EthCamSocketCli_Open(path_id, ETHSOCKIPCCLI_ID_2, &EthsockCliIpcOpen[path_id]);
}
FILE *g_EthcamD1Fp=NULL;
CHAR g_EthcamD1_Otput[40] = "/mnt/sd/ethcamD1.265";
void socketCliEthData1_RecvFileClose(void)
{
	if(g_EthcamD1Fp){
		printf("close file =%s\n", g_EthcamD1_Otput);
		fflush(g_EthcamD1Fp);
		fclose(g_EthcamD1Fp);
		g_EthcamD1Fp=NULL;
	}
}
void socketCliEthData1_RecvCB(ETHCAM_PATH_ID path_id, char* addr, int size)
{
	//static UINT32 eth_i_cnt = 0;
	UINT32 eth_is_i_frame = 0;
	UINT16 bPushData=0;
	UINT32 DescSize=0;
	//UINT16 i;

	//if(sEthCamTxRecInfo[path_id].codec == MEDIAVIDENC_H264){
	if(sEthCamTxRecInfo[path_id].codec == HD_CODEC_TYPE_H264){
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
			//DBG_DUMP("D1_I[%d]=%d,%d\r\n",path_id,g_SocketCliData1_BsFrameCnt[path_id],size);
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
			//BKG_PostEvent(NVTEVT_BKW_ETHCAM_RAW_ENCODE_RESULT);
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
		}


	if (bPushData == 1){

		//UINT32 g_addrPa = 0;
		UINT32 g_addrVa = 0;
		g_addrVa=(UINT32)addr;
		//g_addrPa=g_SocketCliData1_BsPoolAddr[path_id][0].pool_pa+ g_addrVa-g_SocketCliData1_BsPoolAddr[path_id][0].pool_va;
		if (eth_is_i_frame) {    // I frame
		} else {                // P frame
		}
		//DBG_DUMP("[%d]eth_is_i_frame=%d\r\n",path_id,eth_is_i_frame);
#if 1
		//static FILE *fp=NULL;
		char *buf =(char *)g_addrVa;
		static UINT32  FileCnt=0;


		if((g_SocketCliData1_BsFrameCnt[path_id] % 1800) == 1 ){

			FileCnt = (FileCnt+1)%30;
			snprintf(g_EthcamD1_Otput, sizeof(g_EthcamD1_Otput) - 1, "/mnt/sd/ethcamD1_%d.265",(int)FileCnt);

			if(g_EthcamD1Fp==NULL){
				printf("open file =%s\n", g_EthcamD1_Otput);
				g_EthcamD1Fp = fopen(g_EthcamD1_Otput, "wb");
				if (g_EthcamD1Fp) {
					fwrite(buf, size, 1, g_EthcamD1Fp);
				}
			}
		}else if((g_SocketCliData1_BsFrameCnt[path_id] % 1800) == 0 ){
			socketCliEthData1_RecvFileClose();
		}else{
			if (g_EthcamD1Fp == NULL) {
				printf("failed to open %s\n", g_EthcamD1_Otput);
				return;
			}else{
				fwrite(buf, size, 1, g_EthcamD1Fp);
			}
		}
#endif
		if(socketCliEthData1_IsRecv(path_id)==0)
		{
			socketCliEthData1_SetRecv(path_id, 1);
		}
		EthCamHB1[path_id] = 0;
	} else if (bPushData == 3) {
	}
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
			g_IsSocketCliData1_Conn[path_id]=1;
#if ((ETH_REARCAM_CLONE_FOR_DISPLAY == DISABLE) )
			EthCam_SendXMLCmd(path_id, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_TX_STREAM_START, 0);
#endif
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_DISCONNECT: {
			DBG_ERR("[%d]disconnect!!!\r\n",path_id);
			g_IsSocketCliData1_Conn[path_id]=0;
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_SLOW: {
			DBG_ERR("[%d]Error!!!parm=%d\r\n",path_id,parm);
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
FILE *g_EthcamD2Fp=NULL;
void socketCliEthData2_RecvCB(ETHCAM_PATH_ID path_id, char* addr, int size)
{
#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	//static UINT32 eth_i_cnt = 0;
	UINT32 eth_is_i_frame = 0;
	UINT16 bPushData=0;
	UINT32 DescSize=0;
	if(sEthCamTxDecInfo[path_id].Codec == HD_CODEC_TYPE_H264){
		DescSize=sEthCamTxDecInfo[path_id].DescSize;
	}
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
	if(bPushData==1){

		if(socketCliEthData2_IsRecv(path_id)==0)
		{
			socketCliEthData2_SetRecv(path_id, 1);
		}
		if (eth_is_i_frame) {    // I frame
		} else {                // P frame
		}
		//UINT32 g_addrPa = 0;
		//UINT32 g_addrVa = 0;
		//g_addrVa=(UINT32)addr;
		//g_addrPa=g_SocketCliData2_BsPoolAddr[path_id][0].pool_pa+ g_addrVa-g_SocketCliData2_BsPoolAddr[path_id][0].pool_va;

#if 0
		//static FILE *fp=NULL;
		char *buf =(char *)g_addrVa;
		CHAR bs_output[20]       = "/mnt/sd/ethcamD2.265";
		if(g_SocketCliData2_BsFrameCnt[path_id]<= 300){

			if(g_EthcamD2Fp==NULL){
				printf("open file =%s\n", bs_output);
				g_EthcamD2Fp = fopen(bs_output, "wb");
			}
			if (g_EthcamD2Fp == NULL) {
				printf("failed to open %s\n", bs_output);
				return;
			}else{


				fwrite(buf, size, 1, g_EthcamD2Fp);
				//fclose(g_EthcamFp);
			}
		}else{
			if(g_EthcamD2Fp){
				printf("close file =%s\n", bs_output);
				fflush(g_EthcamD2Fp);
				fclose(g_EthcamD2Fp);
				g_EthcamD2Fp=NULL;
			}
		}
#endif

		EthCamHB2 = 0;
	}
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
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
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

#if 1
static  CHAR g_SocketCmd_XMLCmdAddr[ETHCAM_PATH_ID_MAX][2048];
static  CHAR g_SocketCmd_ParserBuf[ETHCAM_PATH_ID_MAX][2048] = {0};
static  UINT32 g_SocketCmd_ResultType[ETHCAM_PATH_ID_MAX]={0};
static  INT32 g_SocketCmd_CmdRet[ETHCAM_PATH_ID_MAX]={0};
static  UINT32 g_SocketCmd_RecvSize[ETHCAM_PATH_ID_MAX]={0};
static  INT32 g_SocketCmd_Cmd[ETHCAM_PATH_ID_MAX]={0};
static  ETHCAM_XML_CB_REGISTER g_SocketCmd_OutputCb[ETHCAM_PATH_ID_MAX]={0};
#endif
extern INT32 g_SocketCmd_Status[ETHCAM_PATH_ID_MAX];
void socketEthCmd_NotifyCB(int status, int parm)
{
    if(status == CYG_ETHSOCKET_STATUS_CLIENT_CONNECT){
        DBG_DUMP("socketEthCmd_NotifyCB Connect OK\r\n");
	}
    else if(status == CYG_ETHSOCKET_STATUS_CLIENT_REQUEST){
    }
    else if(status == CYG_ETHSOCKET_STATUS_CLIENT_DISCONNECT){
        DBG_DUMP("socketEthCmd_NotifyCB DISConnect !!\r\n");
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
	#if 1
	if(addr && (size>0))
	{
		addr[size]='\0';
		//DBG_DUMP("path_id=%d, size=%d, addr[0]=%s\r\n", path_id, size, addr);
		char   *pch;
		char mimeType[64];//just a dummy buffer for using EthCamCmd_GetData
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

				memcpy(g_SocketCmd_XMLCmdAddr[path_id]+g_SocketCmd_RecvSize[path_id], addr, size);

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
		sscanf(addr, "%15[^:]:%99d", dest_ip, (int *)&dest_port);
		if(dest_port !=SocketInfo[0].port[0] && dest_port !=SocketInfo[0].port[1] && dest_port !=SocketInfo[0].port[2]){
			DBG_ERR("port error! %d\r\n",  cdest_port);
			return;
		}

		sprintf(cdest_port,"%d",(int)dest_port);
		CmdBufShiftSize=strlen(dest_ip)+ strlen(cdest_port)+1; //+1 for :
		memcpy(g_SocketCmd_XMLCmdAddr[path_id], addr+CmdBufShiftSize, size);

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
					if(memcmp((g_SocketCmd_XMLCmdAddr[path_id]+size-1-CmdBufShiftSize-strlen("</LIST>")), "</LIST>",strlen("</LIST>"))!=0){//list data not finish
						g_SocketCmd_RecvSize[path_id]=size-CmdBufShiftSize;
						g_SocketCmd_CmdRet[path_id]=ETHCAM_RET_CONTINUE;
						//EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_CONTINUE);
						return;
					}
				}
				g_SocketCmd_Status[path_id]=0xff;
				g_SocketCmd_CmdRet[path_id]=EthCamCmdXML_parser(g_SocketCmd_Cmd[path_id],g_SocketCmd_XMLCmdAddr[path_id],&g_SocketCmd_OutputCb[path_id]);

				if(g_SocketCmd_CmdRet[path_id]==0){
					EthCamCmd_Done(path_id, ETHCAM_CMD_DONE, ETHCAM_RET_OK);
					if(g_SocketCmd_Status[path_id]==ETHCAM_RET_OK &&
						(g_SocketCmd_ResultType[path_id] == ETHCAM_XML_RESULT_TYPE_VALUE_RESULT || g_SocketCmd_ResultType[path_id] == ETHCAM_XML_RESULT_TYPE_STRING_RESULT)){
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

	}
	#endif
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
			UINT32 AllPathLinkStatus=0;
			UINT16 i=0;

			EthCamSocketCli_ReConnect(path_id, 0, 0);

			for(i=0;i<1;i++){
				if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP){
					AllPathLinkStatus++;
				}
			}
			g_IsSocketCliCmdConn[path_id]=1;

			sEthCamTxSysInfo[0].PullModeEn=0;
			#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			sEthCamTxSysInfo[path_id].CloneDisplayPathEn=1;
			#else
			sEthCamTxSysInfo[path_id].CloneDisplayPathEn=0;
			#endif
			sEthCamCodecSrctype[path_id].VCodec=MOVIE_CODEC_H265;//UI_GetData(FL_MOVIE_CODEC);
			sEthCamCodecSrctype[path_id].Srctype=ETHCAM_TX_SYS_SRCTYPE_57;
			sEthCamCodecSrctype[path_id].bCmdOK=0;
			DBG_DUMP("[%d]PullModeEn=%d, CloneDisplayPathEn=%d, VCode=%d, Srctype=%d\r\n",path_id, sEthCamTxSysInfo[path_id].PullModeEn,sEthCamTxSysInfo[path_id].CloneDisplayPathEn,sEthCamCodecSrctype[path_id].VCodec,sEthCamCodecSrctype[path_id].Srctype);

			EthCamCmdTsk_Open();
			BackgroundExeEvt(NVTEVT_BKW_ETHCAM_SET_TX_SYSINFO);

			BackgroundExeEvt(NVTEVT_BKW_GET_ETHCAM_TX_RECINFO);
			BackgroundExeEvt(NVTEVT_BKW_GET_ETHCAM_TX_DECINFO);
			BackgroundExeEvt(NVTEVT_BKW_ETHCAM_SOCKETCLI_DISP_DATA_OPEN);

			BackgroundExeEvt(NVTEVT_BKW_ETHCAM_SYNC_TIME);
			BackgroundExeEvt(NVTEVT_BKW_ETHCAM_SOCKETCLI_REC_DATA_OPEN_START);
			break;
		}
	case CYG_ETHSOCKETCLI_STATUS_CLIENT_DISCONNECT: {
			DBG_ERR("[%d] disconnect!!!\r\n",path_id);
			g_IsSocketCliCmdConn[path_id]=0;
			break;
		}
	}
}
void socketCliEthCmd_SendSizeCB(int size, int total_size)
{
	DBG_IND("%d, %d, Send %d Percent\r\n",size ,total_size ,size*100/total_size);
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
