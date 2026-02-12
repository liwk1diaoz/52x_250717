#include "ethcam_test.h"

#include <comm/hwclock.h>

#include "EthCam/EthsockCliIpcAPI.h"
#include "EthCam/EthCamSocket.h"
#include "EthCamAppSocket.h"
#include "EthCamAppNetwork.h"
#include "EthCamAppCmd.h"
#include "BackgroundObj.h"

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UIBKGObj
#define __DBGLVL__          THIS_DBGLVL // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

static UINT32 BackgroundEthCamSocketCliCmdOpen(void);
static UINT32 BackgroundEthCamSocketCliDataOpen(void);
static UINT32 BackgroundEthCamTxStreamStart(void);
static UINT32 BackgroundEthCamTxDecInfo(void);
static UINT32 BackgroundEthCamTxRecInfo(void);
static UINT32 BackgroundEthCamTxDispOpenSocketStreamStart(void);
static UINT32 BackgroundEthCamTxDispOpenSocketStream(void);
static UINT32 BackgroundEthCamTxRecOpenSocketStreamStart(void);
static UINT32 BackgroundEthCamSyncTime(void);
static UINT32 BackgroundEthCamSetTxSysInfo(void);


BKG_JOB_ENTRY gBackgroundExtFuncTable[] = {

	{NVTEVT_BKW_ETHCAM_SOCKETCLI_CMD_OPEN,    		BackgroundEthCamSocketCliCmdOpen},
	{NVTEVT_BKW_ETHCAM_SOCKETCLI_DATA_OPEN,    	BackgroundEthCamSocketCliDataOpen},
	{NVTEVT_BKW_TX_STREAM_START,    				BackgroundEthCamTxStreamStart},


	{NVTEVT_BKW_GET_ETHCAM_TX_DECINFO,    					BackgroundEthCamTxDecInfo},
	{NVTEVT_BKW_ETHCAM_SOCKETCLI_DISP_DATA_OPEN_START,    	BackgroundEthCamTxDispOpenSocketStreamStart},
	{NVTEVT_BKW_ETHCAM_SOCKETCLI_DISP_DATA_OPEN,    	BackgroundEthCamTxDispOpenSocketStream},
	{NVTEVT_BKW_GET_ETHCAM_TX_RECINFO,    					BackgroundEthCamTxRecInfo},
	{NVTEVT_BKW_ETHCAM_SOCKETCLI_REC_DATA_OPEN_START,    	BackgroundEthCamTxRecOpenSocketStreamStart},
	{NVTEVT_BKW_ETHCAM_SYNC_TIME,    						BackgroundEthCamSyncTime},

	{NVTEVT_BKW_ETHCAM_SET_TX_SYSINFO,    BackgroundEthCamSetTxSysInfo},

	{0,                             0},
};
UINT32 BackgroundExeEvt(NVTEVT evt)
{
	int i = 0;
	UINT32 ret =0;

	while (gBackgroundExtFuncTable[i].event != NVTEVT_NULL) {
		if (evt == gBackgroundExtFuncTable[i].event) {
			ret = gBackgroundExtFuncTable[i].pfn();
			break;
		}
		i++;
	}
	return ret;
}


static UINT32 BackgroundEthCamSocketCliCmdOpen(void)
{
	UINT32 i;

	for (i=0; i<1; i++){
		//DBG_ERR("LinkStatus[%d]=%d, %d\r\n",i,EthCamNet_GetEthLinkStatus(i),EthCamNet_GetPrevEthLinkStatus(i));
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			socketCliEthCmd_Open(i);
		}
	}
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamSocketCliDataOpen(void)
{
	UINT32 i;
	for (i=0; i<1; i++){
		if(socketCliEthCmd_IsConn(i) && sEthCamTxDecInfo[i].bStarupOK && EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			socketCliEthData_Open(i, ETHSOCKIPCCLI_ID_1);
#endif
			socketCliEthData_Open(i, ETHSOCKIPCCLI_ID_0);
		}
	}
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamTxDecInfo(void)
{
	ETHCAM_PORT_TYPE port_type=ETHCAM_PORT_DATA1;
	UINT32 i;//, j=0;
	for (i=0; i<1; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			port_type =ETHCAM_PORT_DATA2;
			#else
			port_type =ETHCAM_PORT_DATA1;
			#endif

			EthCam_SendXMLCmd(i, port_type ,ETHCAM_CMD_GET_TX_DEC_INFO, 0);
			DBG_DUMP("i=%d, dec_info, fps=%d, tbr=%d,w=%d, h=%d, GOP=%d, Codec=%d\r\n",i,sEthCamTxDecInfo[i].Fps,sEthCamTxDecInfo[i].Tbr,sEthCamTxDecInfo[i].Width ,sEthCamTxDecInfo[i].Height,sEthCamTxDecInfo[i].Gop,sEthCamTxDecInfo[i].Codec);
			EthCam_SendXMLCmd(i, port_type ,ETHCAM_CMD_GET_TX_SPS_PPS, 0);


			EthCamSocketCli_SetDescSize(i, ETHSOCKIPCCLI_ID_1,  sEthCamTxDecInfo[i].DescSize);
			sEthCamTxDecInfo[i].bStarupOK=1;

			#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			socketCliEthData2_ConfigRecvBuf(i);
			#else
			socketCliEthData1_ConfigRecvBuf(i);
			#endif
		}
	}
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamTxRecInfo(void)
{
	UINT32 i;
	UINT32 addrPa=0, sizePa=0;
	for (i=0; i<1; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_GET_TX_REC_INFO, 0);

			EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_GET_TX_SPS_PPS, 0);
			EthCamSocketCli_SetDescSize(i, ETHSOCKIPCCLI_ID_0,  sEthCamTxRecInfo[i].DescSize);

			DBG_DUMP("rec_format[%d]=%d\r\n",i,sEthCamTxRecInfo[i].rec_format);
			#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			socketCliEthData1_ConfigRecvBuf(i);
			#endif
			socketCliEthData1_GetBsBufInfo(i, &addrPa, &sizePa);

		}
	}
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamTxDispOpenSocketStreamStart(void)
{
	UINT32 i;
	for (i=0; i<1; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			socketCliEthData_Open(i, ETHSOCKIPCCLI_ID_1);
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA2 ,ETHCAM_CMD_TX_STREAM_START, 0);
#else
			socketCliEthData_Open(i, ETHSOCKIPCCLI_ID_0);
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_TX_STREAM_START, 0);
#endif
		}
	}
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamTxDispOpenSocketStream(void)
{
	UINT32 i;
	for (i=0; i<1; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			socketCliEthData_Open(i, ETHSOCKIPCCLI_ID_1);
#else
			socketCliEthData_Open(i, ETHSOCKIPCCLI_ID_0);
#endif
		}
	}
	return NVTRET_OK;
}

static UINT32 BackgroundEthCamTxRecOpenSocketStreamStart(void)
{
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	UINT32 i;
	for (i=0; i<1; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			socketCliEthData_Open(i, ETHSOCKIPCCLI_ID_0);
			EthCam_SendXMLCmd(i ,ETHCAM_PORT_DATA1 ,ETHCAM_CMD_TX_STREAM_START, 0);
			EthCamNet_SetPrevEthLinkStatus(i, ETHCAM_LINK_UP);
		}
	}
#endif
	return NVTRET_OK;
}


static UINT32 BackgroundEthCamTxStreamStart(void)
{
	UINT32 i;
	for (i=0; i<1; i++){
		//DBG_WRN("GetEthLinkStatus[%d]=%d, GetPrevEthLinkStatus[%d]=%d\r\n",i,EthCamNet_GetEthLinkStatus(i),i,EthCamNet_GetPrevEthLinkStatus(i));
		if(socketCliEthData1_IsConn(i) && EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			DBG_DUMP("TxStream Open path id=%d\r\n",i);
#if(ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA2 ,ETHCAM_CMD_TX_STREAM_START, 0);
#endif
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_TX_STREAM_START, 0);
			EthCamNet_SetPrevEthLinkStatus(i, ETHCAM_LINK_UP);
		}else{
		}
	}

	return NVTRET_OK;
}
static UINT32 BackgroundEthCamSyncTime(void)
{
	UINT32 i;
	for (i=0; i<1; i++){
		if(EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			//sync time
			#if 0
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SYNC_TIME, 0);
			struct tm Curr_DateTime ={0};
			Curr_DateTime = hwclock_get_time(TIME_ID_CURRENT);
			EthCam_SendXMLData(i, (UINT8 *)&Curr_DateTime, sizeof(Curr_DateTime));
			#endif
		}
	}
	return NVTRET_OK;
}
static UINT32 BackgroundEthCamSetTxSysInfo(void)
{
	UINT32 i;
	UINT32 TxVdoEncBufSec=3; //default=3
	for (i=0; i<1; i++){
		if(socketCliEthCmd_IsConn(i) && EthCamNet_GetEthLinkStatus(i)==ETHCAM_LINK_UP && (EthCamNet_GetEthLinkStatus(i) != EthCamNet_GetPrevEthLinkStatus(i))){
			sEthCamTxSysInfo[i].bCmdOK=0;
			EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SET_TX_SYS_INFO, TxVdoEncBufSec);
			EthCam_SendXMLData(i, (UINT8 *)&sEthCamTxSysInfo[i], sizeof(ETHCAM_TX_SYS_INFO));
			DBG_DUMP("bCmdOK[%d]=%d\r\n",i,sEthCamTxSysInfo[i].bCmdOK);

			if(sEthCamTxSysInfo[i].bCmdOK==1){
				EthCam_SendXMLCmd(i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SET_TX_CODEC_SRCTYPE, 0);
				EthCam_SendXMLData(i, (UINT8 *)&sEthCamCodecSrctype[i], sizeof(ETHCAM_TX_CODEC_SRCTYPE));
			}
		}
	}
	return NVTRET_OK;
}


