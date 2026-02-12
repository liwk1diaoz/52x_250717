#include "ethcam_test.h"

#include "EthCamAppCmd.h"
#include "EthCam/EthCamSocket.h"
#include "EthCamAppSocket.h"
#include "EthCamAppNetwork.h"
#include <comm/hwclock.h>
#include "kwrap/flag.h"
#include "kwrap/type.h"
#include "Utility/SwTimer.h"
#include "kwrap/task.h"
#include "kwrap/semaphore.h"
#include "HfsNvt/hfs.h"

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          EthCamAppCmd
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////


ID ETHCAM_CMD_SND_FLG_ID = 0;
ID ETHCAM_CMD_RCV_FLG_ID = 0;
THREAD_HANDLE ETHCAM_CMD_SND_TSK_ID;
THREAD_HANDLE ETHCAM_CMD_RCV_TSK_ID;

ID ETHCAM_CMD_SNDDATA1_SEM_ID = 0;
ID ETHCAM_CMD_SNDDATA2_SEM_ID = 0;
ID ETHCAM_CMD_SNDCMD_SEM_ID = 0;

ID  ETHCAM_WIFICB_VDOFRM_SEM_ID = 0;
ID  ETHCAM_DISP_DATA_SEM_ID = 0;
extern UINT32 EthCamHB1[ETHCAM_PATH_ID_MAX], EthCamHB2;
ETHCAM_TX_SYS_INFO sEthCamTxSysInfo[ETHCAM_PATH_ID_MAX]={0};//for Rx
ETHCAM_TX_CODEC_SRCTYPE sEthCamCodecSrctype[ETHCAM_PATH_ID_MAX]={0};//for Rx
static BOOL g_bEthCamCmdOpened = FALSE;
static ETHCAM_XML_RESULT *g_pEthCamCmdRstTbl=NULL;
ETHCAM_TX_DEC_INFO sEthCamTxDecInfo[ETHCAM_PATH_ID_MAX]={0};
ETHCAM_TX_REC_INFO sEthCamTxRecInfo[ETHCAM_PATH_ID_MAX]={0};
INT32 g_SocketCmd_Status[ETHCAM_PATH_ID_MAX]={0};

ETHCAM_FWUD sEthCamFwUd={0};


ER EthCamCmdTsk_Open(void)
{
	if (g_bEthCamCmdOpened==1) {
		return E_SYS;
	}
	EthCamCmdHandler_InstallID();


	// clear flag first
	clr_flg(ETHCAM_CMD_SND_FLG_ID, 0xFFFFFFFF);
	clr_flg(ETHCAM_CMD_RCV_FLG_ID, 0xFFFFFFFF);

	vos_task_resume(ETHCAM_CMD_SND_TSK_ID);

	vos_task_resume(ETHCAM_CMD_RCV_TSK_ID);

	g_bEthCamCmdOpened=1;
	return E_OK;
}

ER EthCamCmdTsk_Close(void)
{
	FLGPTN FlgPtn;

	if (g_bEthCamCmdOpened==0) {
		return E_SYS;
	}

	// stop task
	set_flg(ETHCAM_CMD_SND_FLG_ID, FLG_ETHCAM_CMD_STOP);
	wai_flg(&FlgPtn, ETHCAM_CMD_SND_FLG_ID, FLG_ETHCAM_CMD_IDLE, TWF_ORW | TWF_CLR);

	set_flg(ETHCAM_CMD_RCV_FLG_ID, FLG_ETHCAM_CMD_STOP);
	wai_flg(&FlgPtn, ETHCAM_CMD_RCV_FLG_ID, FLG_ETHCAM_CMD_IDLE, TWF_ORW | TWF_CLR);


	EthCamCmdHandler_UnInstallID();

	EthsockCliIpc_UnInstallID(1);

	g_bEthCamCmdOpened=0;
	return E_OK;
}


void EthCamCmd_SndData1Lock(void)
{
    DBG_IND("SndTsk Lock\r\n");
    SEM_WAIT(ETHCAM_CMD_SNDDATA1_SEM_ID);
}
void EthCamCmd_SndData1Unlock(void)
{
    DBG_IND("SndTsk Unlock\r\n");
    SEM_SIGNAL(ETHCAM_CMD_SNDDATA1_SEM_ID);
}
void EthCamCmd_SndData2Lock(void)
{
    DBG_IND("SndTsk Lock\r\n");
    SEM_WAIT(ETHCAM_CMD_SNDDATA2_SEM_ID);
}
void EthCamCmd_SndData2Unlock(void)
{
    DBG_IND("SndTsk Unlock\r\n");
    SEM_SIGNAL(ETHCAM_CMD_SNDDATA2_SEM_ID);
}
void EthCamCmd_SndCmdLock(void)
{
    DBG_IND("SndTsk Lock\r\n");
    SEM_WAIT(ETHCAM_CMD_SNDCMD_SEM_ID);
}
void EthCamCmd_SndCmdUnlock(void)
{
    DBG_IND("SndTsk Unlock\r\n");
    SEM_SIGNAL(ETHCAM_CMD_SNDCMD_SEM_ID);
}

INT32 EthCamCmd_Send(ETHCAM_PATH_ID path_id, char* addr, int* size)
{
	INT32 result=0;
	static UINT32 errCnt=0;

	EthCamCmd_SndCmdLock();
	result=EthCamSocket_Send(path_id, ETHSOCKETCLI_CMD, addr, size);
	EthCamCmd_SndCmdUnlock();
	if(result!=0){
		if(errCnt % 20 == 0){
			DBG_ERR("[%d]result=%d, size=%d\r\n",path_id,result,*size);
		}
		errCnt++;
	}else{
		errCnt=0;
	}

	return result;
}
THREAD_RETTYPE EthCamCmdSnd_Tsk(void)
{
	FLGPTN  FlgPtn;
	UINT32 GetFrmNum=1;
	DBG_IND("EthCamCmdSnd_Tsk() start\r\n");

	THREAD_ENTRY();

	while (ETHCAM_CMD_SND_TSK_ID) {
		set_flg(ETHCAM_CMD_SND_FLG_ID, FLG_ETHCAM_CMD_IDLE);
		PROFILE_TASK_IDLE();
		wai_flg(&FlgPtn, ETHCAM_CMD_SND_FLG_ID, FLG_ETHCAM_CMD_STOP|FLG_ETHCAM_CMD_SND|FLG_ETHCAM_CMD_GETFRM, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		clr_flg(ETHCAM_CMD_SND_FLG_ID, FLG_ETHCAM_CMD_IDLE);
		if (FlgPtn & FLG_ETHCAM_CMD_STOP){
			break;
		}

		if (FlgPtn & FLG_ETHCAM_CMD_SND) {
			DBG_IND("FLG_ETHCAM_CMD_SND\r\n");
			EthCamCmd_Send(sEthCamSendCmdInfo.path_id, (char*)sEthCamSendCmdInfo.ParserBuf, (int*)&sEthCamSendCmdInfo.BufSize);
		}
		if (FlgPtn & FLG_ETHCAM_CMD_GETFRM) {
			DBG_IND("FLG_ETHCAM_CMD_GETFRM\r\n");
			if(sEthCamTxSysInfo[0].PullModeEn){
				static UINT32 EthCamPreHB1[ETHCAM_PATH_ID_MAX]={0};
				if(EthCamNet_GetEthLinkStatus(ETHCAM_PATH_ID_1)==ETHCAM_LINK_UP && EthCamNet_GetPrevEthLinkStatus(ETHCAM_PATH_ID_1)==ETHCAM_LINK_UP){
					if(EthCamHB1[ETHCAM_PATH_ID_1]>2 && EthCamPreHB1[ETHCAM_PATH_ID_1]!=EthCamHB1[ETHCAM_PATH_ID_1]){// 4 sec
						DBG_WRN("reSend Strm Start[0]\r\n");
						EthCamPreHB1[ETHCAM_PATH_ID_1]=EthCamHB1[ETHCAM_PATH_ID_1];
						EthCam_SendXMLCmd(ETHCAM_PATH_ID_1, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_TX_STREAM_START, 0);
					}
					//uiTime = Perf_GetCurrent();
					if(socketCliEthData1_IsConn(ETHCAM_PATH_ID_1)){
						EthCam_SendXMLCmd(ETHCAM_PATH_ID_1, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_GET_FRAME, GetFrmNum);
					}
					//DBG_DUMP("t1: %dus\r\n", Perf_GetCurrent() - uiTime);
				}
				if(EthCamNet_GetEthLinkStatus(ETHCAM_PATH_ID_2)==ETHCAM_LINK_UP && EthCamNet_GetPrevEthLinkStatus(ETHCAM_PATH_ID_2)==ETHCAM_LINK_UP){
					if(EthCamHB1[ETHCAM_PATH_ID_2]>2 && EthCamPreHB1[ETHCAM_PATH_ID_2]!=EthCamHB1[ETHCAM_PATH_ID_2]){// 4 sec
						DBG_WRN("reSend Strm Start[1]\r\n");
						EthCamPreHB1[ETHCAM_PATH_ID_2]=EthCamHB1[ETHCAM_PATH_ID_2];
						EthCam_SendXMLCmd(ETHCAM_PATH_ID_2, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_TX_STREAM_START, 0);
					}
					if(socketCliEthData1_IsConn(ETHCAM_PATH_ID_2)){
						EthCam_SendXMLCmd(ETHCAM_PATH_ID_2, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_GET_FRAME, GetFrmNum);
					}
				}
			}
		}
	}
	//ext_tsk();
	THREAD_RETURN(0);
}

INT32 EthCamData2_Send(char* addr, int* size)
{
	INT32 result=0;
	static UINT32 errCnt=0;
	ETHCAM_PATH_ID path_id=ETHCAM_PATH_ID_1;
	EthCamCmd_SndData2Lock();

	result=EthCamSocket_Send(path_id, ETHSOCKETCLI_DATA2, addr, size);
 	EthCamCmd_SndData2Unlock();
	if(result!=0){
		if(errCnt % 20 == 0){
			DBG_ERR("result=%d\r\n",result);
		}
		errCnt++;
	}

	return result;
}
THREAD_RETTYPE EthCamCmdRcvHandler_Tsk(void)
{
	FLGPTN  FlgPtn;

	DBG_IND("EthCamCmdRcvHandler_Tsk() start\r\n");

	THREAD_ENTRY();

	while (ETHCAM_CMD_RCV_TSK_ID) {
		set_flg(ETHCAM_CMD_RCV_FLG_ID, FLG_ETHCAM_CMD_IDLE);
		PROFILE_TASK_IDLE();
		wai_flg(&FlgPtn, ETHCAM_CMD_RCV_FLG_ID, FLG_ETHCAM_CMD_STOP|FLG_ETHCAM_CMD_RCV|FLG_ETHCAM_CMD_SETIP, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		clr_flg(ETHCAM_CMD_RCV_FLG_ID, FLG_ETHCAM_CMD_IDLE);
		if (FlgPtn & FLG_ETHCAM_CMD_STOP){
			break;
		}
		if (FlgPtn & FLG_ETHCAM_CMD_RCV) {
		}
		if (FlgPtn & FLG_ETHCAM_CMD_SETIP) {
		}
	}
	THREAD_RETURN(0);
}
void EthCamCmdHandler_InstallID(void)
{
	OS_CONFIG_SEMPHORE(ETHCAM_CMD_SNDDATA1_SEM_ID, 0, 1, 1);
	OS_CONFIG_SEMPHORE(ETHCAM_CMD_SNDDATA2_SEM_ID, 0, 1, 1);
	OS_CONFIG_SEMPHORE(ETHCAM_CMD_SNDCMD_SEM_ID, 0, 1, 1);

	OS_CONFIG_FLAG(ETHCAM_CMD_SND_FLG_ID);
	OS_CONFIG_FLAG(ETHCAM_CMD_RCV_FLG_ID);
	ETHCAM_CMD_SND_TSK_ID=vos_task_create(EthCamCmdSnd_Tsk,  0, "EthCamCmdSnd_Tsk",   PRI_ETHCAM_CMD_SND,	STKSIZE_ETHCAM_CMD_SND);
	ETHCAM_CMD_RCV_TSK_ID=vos_task_create(EthCamCmdRcvHandler_Tsk,  0, "EthCamCmdRcvHandler_Tsk",   PRI_ETHCAM_CMD_RCV,	STKSIZE_ETHCAM_CMD_RCV);

	OS_CONFIG_SEMPHORE(ETHCAM_WIFICB_VDOFRM_SEM_ID, 0, 1, 1);

	OS_CONFIG_SEMPHORE(ETHCAM_DISP_DATA_SEM_ID, 0, 1, 1);
}
void EthCamCmdHandler_UnInstallID(void)
{
	SEM_DESTROY(ETHCAM_CMD_SNDDATA1_SEM_ID);
	SEM_DESTROY(ETHCAM_CMD_SNDDATA2_SEM_ID);
	SEM_DESTROY(ETHCAM_CMD_SNDCMD_SEM_ID);
	rel_flg(ETHCAM_CMD_SND_FLG_ID);
	rel_flg(ETHCAM_CMD_RCV_FLG_ID);

	vos_task_destroy(ETHCAM_CMD_SND_TSK_ID);
	vos_task_destroy(ETHCAM_CMD_RCV_TSK_ID);

	SEM_DESTROY(ETHCAM_DISP_DATA_SEM_ID);
}
static void EthCamCmd_TimeOutTimerCB1(UINT32 uiEvent)
{
	EthCamCmd_Done(ETHCAM_PATH_ID_1, ETHCAM_CMD_TIMEOUT, ETHCAM_RET_TIMEOUT);
}
static void EthCamCmd_TimeOutTimerCB2(UINT32 uiEvent)
{
	EthCamCmd_Done(ETHCAM_PATH_ID_2, ETHCAM_CMD_TIMEOUT, ETHCAM_RET_TIMEOUT);
}
UINT32 XML_snprintf(char **buf, UINT32 *size, const char *fmt, ...)
{
	UINT32 len = 0;
	va_list marker;

	va_start(marker, fmt);       // Initialize variable arguments.

	#if _TODO
	if (marker)
	#endif
	{
		len = vsnprintf(*buf, *size, fmt, marker);
	}

	va_end(marker);                // Reset variable arguments.

	*buf += len;
	*size = *size - len;
	return len;
}
void XML_DefaultFormat(UINT32 cmd, UINT32 ret, HFS_U32 bufAddr, HFS_U32 *bufSize, char *mimeType)
{
	UINT32 bufflen = *bufSize - 512;
	char  *buf = (char *)bufAddr;
	UINT32 xmlBufSize = *bufSize;

	XML_STRCPY(mimeType, "text/xml", CYG_HFS_MIMETYPE_MAXLEN);

	XML_snprintf(&buf, &xmlBufSize, DEF_XML_HEAD);
	XML_snprintf(&buf, &xmlBufSize, DEF_XML_RET, cmd, ret);
	*bufSize = (HFS_U32)(buf) - bufAddr;

	if (*bufSize > bufflen) {
		DBG_ERR("no buffer\r\n");
		*bufSize = bufflen;
	}
}

char pSendXMLCmdBuf[128] = {0};
INT32 EthCam_SendXMLCmd(ETHCAM_PATH_ID path_id, ETHCAM_PORT_TYPE port_type, UINT32 cmd,UINT32 par)
{
	INT32 result=0;
	UINT32 len =0;

	CHAR* dest_ip[ETHCAM_PATH_ID_MAX]={SocketInfo[0].ip,SocketInfo[1].ip};
	UINT32 socket_port[ETHCAM_PATH_ID_MAX][ETHCAM_PORT_DATA_MAX]={{SocketInfo[0].port[2], SocketInfo[0].port[0], SocketInfo[0].port[1]},
																{SocketInfo[1].port[2], SocketInfo[1].port[0], SocketInfo[1].port[1]}};
	SWTIMER_ID   timer_id;
	BOOL         isOpenTimerOK = TRUE;
	SWTIMER_CB EventHandler;

	sprintf(pSendXMLCmdBuf, "%s:%d/?custom=1&cmd=%d&par=%d", dest_ip[path_id], (int)socket_port[path_id][port_type], (int)cmd, (int)par);
	len=strlen(pSendXMLCmdBuf)+1;
	DBG_IND("len=%d, cmd=%s\r\n",len,pSendXMLCmdBuf);
	EthCamCmd_ClrFlg(path_id, (ETHCAM_CMD_DONE|ETHCAM_CMD_TIMEOUT));
	if(path_id==ETHCAM_PATH_ID_1){
		EventHandler=(SWTIMER_CB)EthCamCmd_TimeOutTimerCB1;
	}else{
		EventHandler=(SWTIMER_CB)EthCamCmd_TimeOutTimerCB2;
	}
	if (SwTimer_Open(&timer_id, EventHandler) != E_OK) {
		DBG_ERR("open timer fail\r\n");
		isOpenTimerOK = FALSE;
	} else {
		SwTimer_Cfg(timer_id, 3000 /*ms*/, SWTIMER_MODE_FREE_RUN);
		SwTimer_Start(timer_id);
	}


	result= EthCamCmd_Send(path_id, (char *)pSendXMLCmdBuf, (int*)&len);
	//DBG_DUMP("result=%d\r\n",result);

	if(result==0){

		result =EthCamCmd_WaitFinish(path_id, (ETHCAM_CMD_DONE|ETHCAM_CMD_TIMEOUT));

		if(ETHCAM_RET_TIMEOUT == result){
			DBG_ERR("cmd Timeout!cmd=%s\r\n",pSendXMLCmdBuf);
		}else if(ETHCAM_RET_CONTINUE == result){
			DBG_WRN("cmd CONTINUE\r\n");
		}else{
			//DBG_DUMP("cmd=%d  OK\r\n",cmd);
		}
	}else{
		DBG_ERR("result=%d, %s\r\n",result,pSendXMLCmdBuf);
	}
	if (isOpenTimerOK) {
		SwTimer_Stop(timer_id);
		SwTimer_Close(timer_id);
	}
	return result;
}
INT32 EthCam_SendXMLData(ETHCAM_PATH_ID path_id, UINT8* addr, UINT32 size)
{
	UINT32 result=0;
	EthCamCmd_ClrFlg(path_id, ETHCAM_CMD_DONE);
	//timer_pausePlay(g_EthCamCmd_TimerID, TIMER_STATE_PLAY);

	result= EthCamCmd_Send(path_id, (char *)addr, (int*)&size);
	if(result==0){

		result=EthCamCmd_WaitFinish(path_id, (ETHCAM_CMD_DONE|ETHCAM_CMD_TIMEOUT));
		if(ETHCAM_RET_TIMEOUT == result){
			DBG_ERR("cmd Timeout!\r\n");
		}
	}else{
		DBG_ERR("result=%d\r\n",result);
	}
	//timer_pausePlay(g_EthCamCmd_TimerID, TIMER_STATE_PAUSE);

	return result;
}

INT32 EthCam_SendXMLStatusCB(ETHCAM_PATH_ID path_id, ETHCAM_PORT_TYPE port_type, UINT32 cmd, UINT32 status)
{
	static char bufArry[128];
	static UINT32 len = 0;
	INT32 result = E_OK;
	char *buf = bufArry;
	UINT32 sendlen = 0;
	UINT32 bufSize = 128;
	memset(bufArry, 0, sizeof(bufArry));
	CHAR* dest_ip[ETHCAM_PATH_ID_MAX]={SocketInfo[0].ip,SocketInfo[1].ip};
	UINT32 socket_port[ETHCAM_PATH_ID_MAX][ETHCAM_PORT_DATA_MAX]={{SocketInfo[0].port[2], SocketInfo[0].port[0], SocketInfo[0].port[1]},
																{SocketInfo[1].port[2], SocketInfo[1].port[0], SocketInfo[1].port[1]}};

	len = XML_snprintf(&buf, &bufSize, "%s:%d<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" ,dest_ip[path_id], socket_port[path_id][port_type]);
	len = XML_snprintf(&buf, &bufSize, DEF_XML_RET, cmd, status);

	len = buf - bufArry;
	sendlen = len;

	if (len < 128) {
		DBG_IND(" %s %d\r\n", bufArry, sendlen);
		result = EthCamCmd_Send(path_id, (char*)bufArry, (int *)&len);
		if (sendlen != len) {
			result = E_SYS;
			DBG_ERR("sent %d error,should %d\r\n", len, sendlen);
		}
	} else {
		DBG_ERR("len %d err\r\n", len);
		result = E_SYS;
	}

	return result;

}

INT32 EthCam_SendXMLStatus(ETHCAM_PATH_ID path_id, ETHCAM_PORT_TYPE port_type, UINT32 cmd, UINT32 status)
{
	static char bufArry[128];
	static UINT32 len = 0;
	INT32 result = E_OK;
	char *buf = bufArry;
	UINT32 sendlen = 0;
	UINT32 bufSize = 128;
	memset(bufArry, 0, sizeof(bufArry));
	CHAR* dest_ip[ETHCAM_PATH_ID_MAX]={SocketInfo[0].ip,SocketInfo[1].ip};
	UINT32 socket_port[ETHCAM_PATH_ID_MAX][ETHCAM_PORT_DATA_MAX]={{SocketInfo[0].port[2], SocketInfo[0].port[0], SocketInfo[0].port[1]},
																{SocketInfo[1].port[2], SocketInfo[1].port[0], SocketInfo[1].port[1]}};

	len = XML_snprintf(&buf, &bufSize, "%s:%d<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" ,dest_ip[path_id], socket_port[path_id][port_type]);
	len = XML_snprintf(&buf, &bufSize, DEF_XML_RET, cmd, status);

	len = buf - bufArry;
	sendlen = len;

	if (len < 128) {
		DBG_IND(" %s %d\r\n", bufArry, sendlen);
		result = EthCamCmd_Send(path_id, (char*)bufArry, (int *)&len);
		if (sendlen != len) {
			result = E_SYS;
			DBG_ERR("sent %d error,should %d\r\n", len, sendlen);
		}
	} else {
		DBG_ERR("len %d err\r\n", len);
		result = E_SYS;
	}

	return result;

}
INT32 EthCam_SendXMLStr(ETHCAM_PATH_ID path_id, ETHCAM_PORT_TYPE port_type, UINT32 cmd, CHAR* str)
{
	static char bufArry[256];
	static UINT32 len = 0;
	INT32 result = E_OK;
	char *buf = bufArry;
	UINT32 sendlen = 0;
	UINT32 bufSize = 256;
	memset(bufArry, 0, sizeof(bufArry));
	CHAR* dest_ip[ETHCAM_PATH_ID_MAX]={SocketInfo[0].ip,SocketInfo[1].ip};
	UINT32 socket_port[ETHCAM_PATH_ID_MAX][ETHCAM_PORT_DATA_MAX]={{SocketInfo[0].port[2], SocketInfo[0].port[0], SocketInfo[0].port[1]},
																{SocketInfo[1].port[2], SocketInfo[1].port[0], SocketInfo[1].port[1]}};

	len = XML_snprintf(&buf, &bufSize, "%s:%d<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" ,dest_ip[path_id], socket_port[path_id][port_type]);
	len = XML_snprintf(&buf, &bufSize, DEF_XML_STR, cmd, 0, str);

	len = buf - bufArry;
	sendlen = len;

	EthCamCmd_ClrFlg(path_id, (ETHCAM_CMD_DONE|ETHCAM_CMD_TIMEOUT));

	if (len < 256) {

		DBG_IND(" %s %d\r\n", bufArry, sendlen);
		result = EthCamCmd_Send(path_id, (char*)bufArry, (int *)&len);
		if (sendlen != len) {
			result = E_SYS;
			DBG_ERR("sent %d error,should %d\r\n", len, sendlen);
		}
	} else {
		DBG_ERR("len %d err\r\n", len);
		result = E_SYS;
	}

	return result;

}
INT32 EthCam_SendXMLValue(ETHCAM_PATH_ID path_id, ETHCAM_PORT_TYPE port_type, UINT32 cmd, UINT64 value)
{
	static char bufArry[256];
	static UINT32 len = 0;
	INT32 result = E_OK;
	char *buf = bufArry;
	UINT32 sendlen = 0;
	UINT32 bufSize = 256;
	memset(bufArry, 0, sizeof(bufArry));
	CHAR* dest_ip[ETHCAM_PATH_ID_MAX]={SocketInfo[0].ip,SocketInfo[1].ip};
	UINT32 socket_port[ETHCAM_PATH_ID_MAX][ETHCAM_PORT_DATA_MAX]={{SocketInfo[0].port[2], SocketInfo[0].port[0], SocketInfo[0].port[1]},
																{SocketInfo[1].port[2], SocketInfo[1].port[0], SocketInfo[1].port[1]}};

	len = XML_snprintf(&buf, &bufSize, "%s:%d<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" ,dest_ip[path_id], socket_port[path_id][port_type]);
	len = XML_snprintf(&buf, &bufSize, DEF_XML_VALUE, cmd, 0, value);

	len = buf - bufArry;
	sendlen = len;

	EthCamCmd_ClrFlg(path_id, (ETHCAM_CMD_DONE|ETHCAM_CMD_TIMEOUT));

	if (len < 256) {
		DBG_IND(" %s %d\r\n", bufArry, sendlen);
		result = EthCamCmd_Send(path_id, (char*)bufArry, (int *)&len);
		if (sendlen != len) {
			result = E_SYS;
			DBG_ERR("sent %d error,should %d\r\n", len, sendlen);
		}
	} else {
		DBG_ERR("len %d err\r\n", len);
		result = E_SYS;
	}

	return result;

}
INT32 EthCamApp_CheckIfEthCamMode(void)
{
	return TRUE;
}

void EthCamCmd_Init(void)
{
	EthCamCmd_SetExecTable(Cmd_ethcam);
	EthCamCmd_SetResultTable(EthCamXMLResultTbl);

	EthCamCmd_SetDefautCB((UINT32)XML_DefaultFormat);
	//EthCamCmd_SetEventHandle((UINT32)Ux_PostEvent);
	EthCamCmd_SetAppStartupChecker((UINT32)EthCamApp_CheckIfEthCamMode);

	EthCamCmd_ReceiveCmd(TRUE);
}
//XML parser
int EthCamCmdXML_skip_first_line(char *xml_buf){

    ////start code
    int xml_offset=0;

    if(*xml_buf == '<' &&
       *(xml_buf+1) == '?' &&
       *(xml_buf+2) == 'x' &&
       *(xml_buf+3) == 'm' &&
       *(xml_buf+4) == 'l'
       ){
        xml_offset ++;
        while(1){
            if(*(xml_buf+xml_offset)=='>'){
                xml_offset ++;
                break;
            }
            xml_offset ++;
        }
    }
    else{
        DBG_ERR("xml parser error, first char not '<' (%s) \r\n",xml_buf);
        return -1;
    }
    return xml_offset;
}

int EthCamCmdXML_remove_escape(char *xml_buf){

    int offset=0;
    while (1){
        if(*(xml_buf+offset)==0x0d || *(xml_buf+offset)==0x0a || *(xml_buf+offset)==' '){
            offset++;
            continue;
        }
        break;
    }
     return offset;
}
UINT32 EthCamCmdXML_get_tag_name(char *xml_buf, char *output){

    UINT32 offset=0;

    while(1){
        if(*(xml_buf+offset)!='>'){
            offset++;
            if(strlen(xml_buf) < offset){
                DBG_ERR("get not find tag '>' error! xml_buf=%s\r\n",xml_buf);
                return -1;
            }
            continue;
        }
        break;
    }
    if(*xml_buf == '<' && *(xml_buf+1) == '/'){
        strncpy(output,xml_buf+2,offset-2);
        output[offset -2]='\0';

    }
    else if(*xml_buf == '<'){
        strncpy(output,xml_buf+1,offset-1);
        output[offset -1]='\0';
    }
    else{
       DBG_ERR("parsing tag name error first buf=%x xml_buf=%s\r\n",*xml_buf,xml_buf);
       return -1;
    }
    if(strstr(output," ")){
       //////there is a attribute, need to_do!!!
    }
    offset++; ///skip '>'
    return offset;
}

UINT32 EthCamCmdXML_get_tag_value(char *xml_buf, char *output){

    UINT32 offset=0;
    while(1){
        if(*(xml_buf+offset)!='<'){
            offset++;
            if(strlen(xml_buf) < offset){
                DBG_ERR("get not find tag '<' error! xml_buf=%s\r\n",xml_buf);
                return -1;
            }
            continue;
        }
        break;
    }
    strncpy(output,xml_buf,offset);
    output[offset]='\0';

    return offset;
}
INT32 EthCamCmdXML_parsing_leaf(char *xml_buf, char *name, char *value){

    INT32 offset=0;
    INT32 tmp_offset=0;
    tmp_offset = EthCamCmdXML_get_tag_value(xml_buf, value);
    if(tmp_offset< 0){
        DBG_ERR("EthCamCmdXML_get_tag_value error  tmp_offset=%d\r\n",tmp_offset);
        return -1;
    }
    offset += tmp_offset;
    tmp_offset = EthCamCmdXML_get_tag_name(xml_buf+offset,name);//get end flag
    if(tmp_offset< 0){
        DBG_ERR("EthCamCmdXML_get_tag_name error  tmp_offset=%d\r\n",tmp_offset);
        return -1;
    }
    offset += tmp_offset;
    tmp_offset = EthCamCmdXML_remove_escape(xml_buf+offset);// remove \r \n
    if(tmp_offset< 0){
        DBG_ERR("EthCamCmdXML_remove_escape error  tmp_offset=%d\r\n",tmp_offset);
        return -1;
    }
    offset += tmp_offset;
    return offset;
}
INT32 EthCamCmdXML_parsing_default_format(char *xml_buf, void * output)
{
    char current_name[64]={0};
    char current_value[128]={0};
    INT32 offset =0;
    INT32 tmp_offset=0;
    INT32 function_flag=0;
    INT32 pass_flag=0;

    //ETHCAM_XML_DEFAULT_FORMAT *output_data = (ETHCAM_XML_DEFAULT_FORMAT *)output;
    ETHCAM_XML_DEFAULT_FORMAT output_data={0};
    ETHCAM_XML_CB_REGISTER *reg = (ETHCAM_XML_CB_REGISTER *)output;
    memset(&output_data, 0, sizeof(ETHCAM_XML_DEFAULT_FORMAT));
    if(reg){
        output_data.path_id=reg->path_id;
        output_data.port_type=reg->port_type;
    }


    tmp_offset = EthCamCmdXML_get_tag_name(xml_buf+offset,current_name);

    if(tmp_offset < 0){
        DBG_ERR("get_tag_name error offset=%d\r\n",offset);
        return -1;
    }

    offset += tmp_offset;

    while(1){
        if(!strcmp(current_name, "Function")){
            function_flag++;
            if(function_flag > 1){
                /////xml parsing finish
                break;
            }
            tmp_offset = EthCamCmdXML_remove_escape(xml_buf+offset);// remove \r \n
            if(tmp_offset< 0){
                DBG_ERR("EthCamCmdXML_remove_escape error Function tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
        }
        else if(!strcmp(current_name,"Cmd")){
            tmp_offset = EthCamCmdXML_parsing_leaf(xml_buf+ offset, current_name, current_value);
            if(tmp_offset<0){
                DBG_ERR("EthCamCmdXML_parsing_leaf error tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }

            offset += tmp_offset;
            output_data.cmd = atoi(current_value);
        }
        else if(!strcmp(current_name,"Status")){
            tmp_offset = EthCamCmdXML_parsing_leaf(xml_buf+ offset, current_name, current_value);
            if(tmp_offset<0){
                DBG_ERR("EthCamCmdXML_parsing_leaf error tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }

            offset += tmp_offset;
            output_data.status = atoi(current_value);
            g_SocketCmd_Status[reg->path_id]=output_data.status;
            if(pass_flag==0){
                pass_flag=1;
            }
            else{
                DBG_ERR("EthCamCmdXML pass_flag error pass_flag=%d\r\n",pass_flag);
                return -1;
            }
        }
        else{
            DBG_ERR("tag name not support (%s)\r\n",current_name);
            return -1;
        }
        tmp_offset = EthCamCmdXML_get_tag_name(xml_buf+offset,current_name);
        if(tmp_offset< 0){
            DBG_ERR("EthCamCmdXML_get_tag_name error Function tmp_offset=%d\r\n",tmp_offset);
            return -1;
        }
        offset += tmp_offset;
        if(pass_flag == 1){
            ////get data finish , send data to CB function
            ////next current_name is List, it is the last data
            if(!strcmp(current_name,"Function")){
                if(reg && reg->EthCamXML_data_CB){
	                reg->EthCamXML_data_CB(1,(void *)&output_data);
                }
            }
            else{
                if(reg && reg->EthCamXML_data_CB){
	                reg->EthCamXML_data_CB(0,(void *)&output_data);
                }
            }
            pass_flag = 0;
            DBG_IND("cmd=%d, status=%d\r\n",output_data.cmd, output_data.status);
            memset(&output_data, 0, sizeof(output_data));
        }
    }
    return output_data.status;
}


INT32 EthCamCmdXML_parsing_value_result(char *xml_buf, void * output)
{
    char current_name[64]={0};
    char current_value[128]={0};
    INT32 offset =0;
    INT32 tmp_offset=0;
    INT32 function_flag=0;
    INT32 pass_flag=0;
    //ETHCAM_XML_VALUE_RESULT *output_data = (ETHCAM_XML_VALUE_RESULT *)output;
    ETHCAM_XML_VALUE_RESULT output_data={0};
    ETHCAM_XML_CB_REGISTER *reg = (ETHCAM_XML_CB_REGISTER *)output;
    memset(&output_data, 0, sizeof(ETHCAM_XML_VALUE_RESULT));
    if(reg){
        output_data.path_id=reg->path_id;
        output_data.port_type=reg->port_type;
    }

    tmp_offset = EthCamCmdXML_get_tag_name(xml_buf+offset,current_name);

    if(tmp_offset < 0){
        DBG_ERR("get_tag_name error offset=%d\r\n",offset);
        return -1;
    }

    offset += tmp_offset;

    while(1){
        if(!strcmp(current_name, "Function")){
            function_flag++;
            if(function_flag > 1){
                /////xml parsing finish
                break;
            }
            tmp_offset = EthCamCmdXML_remove_escape(xml_buf+offset);// remove \r \n
            if(tmp_offset< 0){
                DBG_ERR("EthCamCmdXML_remove_escape error Function tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
        }
        else if(!strcmp(current_name,"Cmd")){
            tmp_offset = EthCamCmdXML_parsing_leaf(xml_buf+ offset, current_name, current_value);
            if(tmp_offset<0){
                DBG_ERR("EthCamCmdXML_parsing_leaf error tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
            output_data.cmd = atoi(current_value);
        }
        else if(!strcmp(current_name,"Status")){
            tmp_offset = EthCamCmdXML_parsing_leaf(xml_buf+ offset, current_name, current_value);
            if(tmp_offset<0){
                DBG_ERR("EthCamCmdXML_parsing_leaf error tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
            output_data.status = atoi(current_value);
            g_SocketCmd_Status[reg->path_id]=output_data.status;
        }
        else if(!strcmp(current_name,"Value")){
            tmp_offset = EthCamCmdXML_parsing_leaf(xml_buf+ offset, current_name, current_value);
            if(tmp_offset<0){
                DBG_ERR("EthCamCmdXML_parsing_leaf error tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
            output_data.value= atoi(current_value);
            if(pass_flag==0){
                pass_flag=1;
            }
            else{
                DBG_ERR("EthCamCmdXML pass_flag error pass_flag=%d\r\n",pass_flag);
                return -1;
            }
        }
        else{
            DBG_ERR("tag name not support (%s)\r\n",current_name);
            return -1;
        }
        tmp_offset = EthCamCmdXML_get_tag_name(xml_buf+offset,current_name);
        if(tmp_offset< 0){
            DBG_ERR("EthCamCmdXML_get_tag_name error Function tmp_offset=%d\r\n",tmp_offset);
            return -1;
        }
        offset += tmp_offset;
        if(pass_flag == 1){
            ////get data finish , send data to CB function
            ////next current_name is List, it is the last data
            if(!strcmp(current_name,"Function")){
                if(reg && reg->EthCamXML_data_CB){
                	reg->EthCamXML_data_CB(1,(void *)&output_data);
                }
            }
            else{
                if(reg && reg->EthCamXML_data_CB){
                	reg->EthCamXML_data_CB(0,(void *)&output_data);
                }
            }
            pass_flag = 0;
            DBG_IND("cmd=%d, status=%d, value=%d\r\n",output_data.cmd, output_data.status,output_data.value);
            memset(&output_data, 0, sizeof(output_data));
        }
    }
    return 0;
}
INT32 EthCamCmdXML_parsing_string_result(char *xml_buf, void * output)
{
    char current_name[64]={0};
    char current_value[128]={0};
    INT32 offset =0;
    INT32 tmp_offset=0;
    INT32 function_flag=0;
    INT32 pass_flag=0;
    //ETHCAM_XML_VALUE_RESULT *output_data = (ETHCAM_XML_VALUE_RESULT *)output;
    ETHCAM_XML_STRING_RESULT output_data={0};
    ETHCAM_XML_CB_REGISTER *reg = (ETHCAM_XML_CB_REGISTER *)output;
    memset(&output_data, 0, sizeof(ETHCAM_XML_STRING_RESULT));
    if(reg){
        output_data.path_id=reg->path_id;
        output_data.port_type=reg->port_type;
    }

    tmp_offset = EthCamCmdXML_get_tag_name(xml_buf+offset,current_name);

    if(tmp_offset < 0){
        DBG_ERR("get_tag_name error offset=%d\r\n",offset);
        return -1;
    }

    offset += tmp_offset;

    while(1){
        if(!strcmp(current_name, "Function")){
            function_flag++;
            if(function_flag > 1){
                /////xml parsing finish
                break;
            }
            tmp_offset = EthCamCmdXML_remove_escape(xml_buf+offset);// remove \r \n
            if(tmp_offset< 0){
                DBG_ERR("EthCamCmdXML_remove_escape error Function tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
        }
        else if(!strcmp(current_name,"Cmd")){
            tmp_offset = EthCamCmdXML_parsing_leaf(xml_buf+ offset, current_name, current_value);
            if(tmp_offset<0){
                DBG_ERR("EthCamCmdXML_parsing_leaf error tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
            output_data.cmd = atoi(current_value);
        }
        else if(!strcmp(current_name,"Status")){
            tmp_offset = EthCamCmdXML_parsing_leaf(xml_buf+ offset, current_name, current_value);
            if(tmp_offset<0){
                DBG_ERR("EthCamCmdXML_parsing_leaf error tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
            output_data.status = atoi(current_value);
            g_SocketCmd_Status[reg->path_id]=output_data.status;
        }
        else if(!strcmp(current_name,"String")){
            tmp_offset = EthCamCmdXML_parsing_leaf(xml_buf+ offset, current_name, current_value);
            if(tmp_offset<0){
                DBG_ERR("EthCamCmdXML_parsing_leaf error tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
            strncpy(output_data.string,current_value,sizeof(output_data.string));
            output_data.string[sizeof(output_data.string)-1]='\0';
            if(pass_flag==0){
                pass_flag=1;
            }
            else{
                DBG_ERR("EthCamCmdXML pass_flag error pass_flag=%d\r\n",pass_flag);
                return -1;
            }
        }
        else{
            DBG_ERR("tag name not support (%s)\r\n",current_name);
            return -1;
        }
        tmp_offset = EthCamCmdXML_get_tag_name(xml_buf+offset,current_name);
        if(tmp_offset< 0){
            DBG_ERR("EthCamCmdXML_get_tag_name error Function tmp_offset=%d\r\n",tmp_offset);
            return -1;
        }
        offset += tmp_offset;
        if(pass_flag == 1){
            ////get data finish , send data to CB function
            ////next current_name is List, it is the last data
            if(!strcmp(current_name,"Function")){
                if(reg && reg->EthCamXML_data_CB){
                	reg->EthCamXML_data_CB(1,(void *)&output_data);
                }
            }
            else{
                if(reg && reg->EthCamXML_data_CB){
                	reg->EthCamXML_data_CB(0,(void *)&output_data);
                }
            }
            pass_flag = 0;
            DBG_IND("cmd=%d, status=%d, string=%s\r\n",output_data.cmd, output_data.status,output_data.string);
            memset(&output_data, 0, sizeof(output_data));
        }
    }
    return 0;
}
static INT32 EthCamCmdXML_parsing_list(char *xml_buf, void * reg_data){

    char current_name[64]={0};
    char current_value[128]={0};
    INT32 offset =0;
    INT32 tmp_offset=0;
    INT32 list_flag=0;
    INT32 total_count=0;

    ETHCAM_XML_CB_REGISTER *reg = (ETHCAM_XML_CB_REGISTER *)reg_data;
    ETHCAM_XML_LIST output_data={0};
    if(reg){
        output_data.path_id=reg->path_id;
        output_data.port_type=reg->port_type;
    }

    tmp_offset = EthCamCmdXML_get_tag_name(xml_buf+offset,current_name);
    if(tmp_offset < 0){
        DBG_ERR("get_tag_name error offset=%d\r\n",offset);
        return -1;
    }
    offset += tmp_offset;
    while(1){
        if(!strcmp(current_name, "LIST")){
            list_flag++;
            if(list_flag > 1){
                /////xml parsing finish
                break;
            }
            tmp_offset = EthCamCmdXML_remove_escape(xml_buf+offset);// remove \r \n
            if(tmp_offset< 0){
                DBG_ERR("EthCamCmdXML_remove_escape error Function tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
        }
        else if(!strcmp(current_name,"Cmd")){
            tmp_offset = EthCamCmdXML_parsing_leaf(xml_buf+ offset, current_name, current_value);
            if(tmp_offset<0){
                DBG_ERR("EthCamCmdXML_parsing_leaf error tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
        }
        else if(!strcmp(current_name, "ITEM")){
            tmp_offset = EthCamCmdXML_parsing_leaf(xml_buf+ offset, current_name, current_value);
            if(tmp_offset< 0){
                DBG_ERR("EthCamCmdXML_remove_escape error Function tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
            output_data.item[total_count] = atoi(current_value);
            DBG_IND("item=%d, total_count=%d\r\n",output_data.item[total_count],total_count);
            total_count++;
            output_data.total_item_cnt=total_count;
        }
        else{
            DBG_ERR("tag name not support (%s)\r\n",current_name);
            return -1;
        }
        tmp_offset = EthCamCmdXML_get_tag_name(xml_buf+offset,current_name);
        if(tmp_offset< 0){
            DBG_ERR("EthCamCmdXML_get_tag_name error Function tmp_offset=%d\r\n",tmp_offset);
            return -1;
        }
        offset += tmp_offset;
    }
    if(list_flag > 1){
        ////get data finish , send data to CB function
        ////next current_name is List, it is the last data
        if(!strcmp(current_name,"LIST")){
            if(reg && reg->EthCamXML_data_CB){
                reg->EthCamXML_data_CB(1,(void *)&output_data);
            }
        }
        else{
            if(reg && reg->EthCamXML_data_CB){
                reg->EthCamXML_data_CB(0,(void *)&output_data);
            }
        }
        list_flag = 0;
        memset(&output_data, 0, sizeof(output_data));
    }

    return 0;
}
INT32 EthCamCmdXML_GetCmdId(char *xml_buf)
{
    if(strlen(xml_buf)<=0){
        DBG_ERR("EthCam xml buf len error len=%d\r\n",strlen(xml_buf));
        return -1;
    }
    INT32 offset=0;
    INT32 tmp_offset=0;
    tmp_offset = EthCamCmdXML_skip_first_line(xml_buf);
    if(tmp_offset < 0){
        DBG_ERR("skip_first_line parsing error offset=%d\r\n",offset);
        return -1;
    }
    offset = offset + tmp_offset;
    tmp_offset = EthCamCmdXML_remove_escape(xml_buf+offset);
    if(tmp_offset < 0){
        DBG_ERR("EthCamCmdXML_remove_escape parsing error offset=%d\r\n",offset);
        return -1;
    }
    offset = offset + tmp_offset;


    char current_name[64]={0};
    char current_value[128]={0};
    INT32 function_flag=0;

    xml_buf=xml_buf+offset;
    tmp_offset = EthCamCmdXML_get_tag_name(xml_buf,current_name);

    if(tmp_offset < 0){
        DBG_ERR("get_tag_name error offset=%d\r\n",offset);
        return -1;
    }

    offset =0;
    offset += tmp_offset;
    while(1){
        if(!strcmp(current_name, "Function")){
            function_flag++;
            if(function_flag > 1){
                /////xml parsing finish
                break;
            }
            tmp_offset = EthCamCmdXML_remove_escape(xml_buf+offset);// remove \r \n
            if(tmp_offset< 0){
                DBG_ERR("EthCamCmdXML_remove_escape error Function tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
        }
        else if(!strcmp(current_name, "LIST")){
            function_flag++;
            if(function_flag > 1){
                /////xml parsing finish
                break;
            }
            tmp_offset = EthCamCmdXML_remove_escape(xml_buf+offset);// remove \r \n
            if(tmp_offset< 0){
                DBG_ERR("EthCamCmdXML_remove_escape error Function tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
        }
        else if(!strcmp(current_name,"Cmd")){
            tmp_offset = EthCamCmdXML_parsing_leaf(xml_buf+ offset, current_name, current_value);
            if(tmp_offset<0){
                DBG_ERR("EthCamCmdXML_parsing_leaf error tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
            //output_data->cmd = atoi(current_value);
            return atoi(current_value);
        }
        else if(!strcmp(current_name,"Status")){
            tmp_offset = EthCamCmdXML_parsing_leaf(xml_buf+ offset, current_name, current_value);
            if(tmp_offset<0){
                DBG_ERR("EthCamCmdXML_parsing_leaf error tmp_offset=%d\r\n",tmp_offset);
                return -1;
            }
            offset += tmp_offset;
            //output_data->status = atoi(current_value);
            DBG_DUMP("status=%d\r\n",atoi(current_value));
        }
        else{
            DBG_ERR("tag name not support (%s)\r\n",current_name);
            return -1;
        }
        tmp_offset = EthCamCmdXML_get_tag_name(xml_buf+offset,current_name);
        if(tmp_offset< 0){
            DBG_ERR("EthCamCmdXML_get_tag_name error Function tmp_offset=%d\r\n",tmp_offset);
            return -1;
        }
        offset += tmp_offset;
    }

    return -1;
}
INT32 EthCamCmdXML_parser(INT32 cmd_id,char *xml_buf ,void* output)
{
    if(cmd_id < 0){
        DBG_ERR("EthCam cmd_id error cmd_id=%d \r\n",cmd_id);
        return -1;
    }
    if(strlen(xml_buf)<=0){
        DBG_ERR("EthCam xml buf len error len=%d\r\n",strlen(xml_buf));
        return -1;
    }
    INT32 offset=0;
    INT32 tmp_offset=0;
    tmp_offset = EthCamCmdXML_skip_first_line(xml_buf);
    if(tmp_offset < 0){
        DBG_ERR("skip_first_line parsing error offset=%d\r\n",offset);
        return -1;
    }
    offset = offset + tmp_offset;
    tmp_offset = EthCamCmdXML_remove_escape(xml_buf+offset);
    if(tmp_offset < 0){
        DBG_ERR("EthCamCmdXML_remove_escape parsing error offset=%d\r\n",offset);
        return -1;
    }

    offset = offset + tmp_offset;

    INT32 i = 0;
    UINT32 result_type=0;
    ETHCAM_XML_RESULT *EthCamXMLResultTbl=EthCamCmd_GetResultTable();
    while (EthCamXMLResultTbl[i].cmd != 0) {
        if (cmd_id == EthCamXMLResultTbl[i].cmd) {
            result_type=EthCamXMLResultTbl[i].result_type;
            break;
        }
        i++;
    }
    DBG_IND("result_type=%d\r\n",result_type);

    INT32 ret =0;
    //switch (cmd_id){
    switch (result_type){

        //case ETHCAM_CMD_STARTUP:
        case ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT:
            ret = EthCamCmdXML_parsing_default_format(xml_buf+offset, output);
            if(ret < 0){
                DBG_ERR("DEFAULT_FORMAT %d error ret =%d\r\n",cmd_id,ret);
            }
            break;
        case ETHCAM_XML_RESULT_TYPE_VALUE_RESULT:
            ret = EthCamCmdXML_parsing_value_result(xml_buf+offset, output);
            if(ret < 0){
                DBG_ERR("VALUE_RESULT %d error ret =%d\r\n",cmd_id,ret);
            }
            break;
        case ETHCAM_XML_RESULT_TYPE_LIST:
            ret = EthCamCmdXML_parsing_list(xml_buf+offset, output);
            if(ret < 0){
                DBG_ERR("LIST %d error ret =%d\r\n",cmd_id,ret);
            }
            break;
        case ETHCAM_XML_RESULT_TYPE_STRING_RESULT:
            ret = EthCamCmdXML_parsing_string_result(xml_buf+offset, output);
            if(ret < 0){
                DBG_ERR("STRING_RESULT %d error ret =%d\r\n",cmd_id,ret);
            }
            break;
        default:
            DBG_ERR("EthCam cmd %d not support!\r\n",cmd_id);
            ret=ETHCAM_RET_CMD_NOT_FOUND;
            return ret;
    }
    if(ret < 0){
        //DBG_DUMP("xml_buf=%s\r\n",xml_buf);
    }
    return ret;
}

void EthCam_GetDest(char *path,  ETHCAM_PATH_ID *path_id, ETHCAM_PORT_TYPE *port_type)
{
	char dest_ip[16];
	UINT32 dest_port = 80;
	UINT i ,j;
	//CHAR* ip_base[ETHCAM_PATH_ID_MAX]={gEthCamTxIP[0].ip,gEthCamTxIP[1].ip};
	CHAR* ip_base[ETHCAM_PATH_ID_MAX]={SocketInfo[0].ip,SocketInfo[1].ip};

	//UINT32 port_base[ETHCAM_PATH_ID_MAX][ETHCAM_PORT_DATA_MAX]={ETH_CMD_SOCKET_PORT, ETH_DATA1_SOCKET_PORT, ETH_DATA2_SOCKET_PORT};
	UINT32 port_base[ETHCAM_PATH_ID_MAX][ETHCAM_PORT_DATA_MAX]={{SocketInfo[0].port[2], SocketInfo[0].port[0], SocketInfo[0].port[1]},
																{SocketInfo[1].port[2], SocketInfo[1].port[0], SocketInfo[1].port[1]}};

	UINT32 port_type_base[ETHCAM_PATH_ID_MAX][ETHCAM_PORT_DATA_MAX]={{ETHCAM_PORT_DEFAULT, ETHCAM_PORT_DATA1, ETHCAM_PORT_DATA2},
																{ETHCAM_PORT_DEFAULT, ETHCAM_PORT_DATA1, ETHCAM_PORT_DATA2}};


	sscanf(path, "%15[^:]:%99d", dest_ip, (int *)&dest_port);
	DBG_IND("dest_ip=%s, dest_port=%d\r\n",dest_ip,dest_port);

	if(strcmp(dest_ip, ip_base[0])==0){
		*path_id=ETHCAM_PATH_ID_1;
	}else{
		*path_id=ETHCAM_PATH_ID_2;
	}
	for(i=0;i<ETHCAM_PATH_ID_MAX;i++){
		for(j=0;j<ETHCAM_PORT_DATA_MAX;j++){
			if(dest_port==port_base[i][j]){
				*port_type=port_type_base[i][j];
				break;
			}
		}
	}
}

//======================XML Cmd Result CB Satrt======================
void EthCam_Startup_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_DEFAULT_FORMAT *output=(ETHCAM_XML_DEFAULT_FORMAT*)output_data;
	DBG_IND("bEnd=%d, cmd=%d, status=%d\r\n",bEnd,output->cmd, output->status);
	if(output->status == ETHCAM_RET_OK){
		DBG_DUMP("Startup OK\r\n");
		sEthCamTxDecInfo[output->path_id].bStarupOK=1;
	}
}
void EthCam_MovieTBR_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_VALUE_RESULT *output=(ETHCAM_XML_VALUE_RESULT*)output_data;
	DBG_IND("bEnd=%d, cmd=%d, status=%d, value=%d\r\n",bEnd,output->cmd, output->status,output->value);
	DBG_IND("Tx TBR=%d\r\n",output->value);
	DBG_IND("path_id=%d, port_type=%d\r\n",output->path_id,output->port_type);

	sEthCamTxDecInfo[output->path_id].Tbr=output->value;
}
void EthCam_MovieFps_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_VALUE_RESULT *output=(ETHCAM_XML_VALUE_RESULT*)output_data;
	DBG_IND("bEnd=%d, cmd=%d, status=%d, value=%d\r\n",bEnd,output->cmd, output->status,output->value);
	DBG_IND("Tx Fps=%d\r\n",output->value);
	sEthCamTxDecInfo[output->path_id].Fps=output->value;
}
void EthCam_MovieGOP_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_VALUE_RESULT *output=(ETHCAM_XML_VALUE_RESULT*)output_data;
	DBG_IND("bEnd=%d, cmd=%d, status=%d, value=%d\r\n",bEnd,output->cmd, output->status,output->value);
	DBG_IND("Tx GOP=%d\r\n",output->value);
	sEthCamTxDecInfo[output->path_id].Gop=output->value;
}
void EthCam_MovieCodec_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_VALUE_RESULT *output=(ETHCAM_XML_VALUE_RESULT*)output_data;
	DBG_IND("bEnd=%d, cmd=%d, status=%d, value=%d\r\n",bEnd,output->cmd, output->status,output->value);
	DBG_IND("Tx Codec=%d\r\n",output->value);
	sEthCamTxDecInfo[output->path_id].Codec=output->value;
}

void EthCam_MovieRecSize_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_LIST *output=(ETHCAM_XML_LIST*)output_data;
	DBG_IND("bEnd=%d, cmd=%d, total_item_cnt=%d\r\n",bEnd,output->cmd, output->total_item_cnt);
	if(output->total_item_cnt==2){
		DBG_IND("Tx Width=%d, Height=%d\r\n",output->item[0], output->item[1]);
		sEthCamTxDecInfo[output->path_id].Width=output->item[0];
		sEthCamTxDecInfo[output->path_id].Height=output->item[1];
	}
}
/*
	H264_NALU_TYPE_SPS = 7,
	H264_NALU_TYPE_PPS = 8,

	H265_NALU_TYPE_VPS = 32,
	H265_NALU_TYPE_SPS = 33,
	H265_NALU_TYPE_PPS = 34,
*/
void EthCam_MovieSPS_PPS_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_LIST *output=(ETHCAM_XML_LIST*)output_data;
	UINT32 i;
	DBG_IND("bEnd=%d, cmd=%d, total_item_cnt=%d\r\n",bEnd,output->cmd, output->total_item_cnt);
	#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	if(output->port_type == ETHCAM_PORT_DATA1){
		sEthCamTxRecInfo[output->path_id].DescSize=output->total_item_cnt;
		//memcpy(sEthCamTxInfo.Desc,output->item,  output->total_item_cnt);
		DBG_IND("Rec Tx SPS[%d]= {",output->total_item_cnt);
		sEthCamTxRecInfo[output->path_id].SPSSize=0;
		sEthCamTxRecInfo[output->path_id].PPSSize=0;
		sEthCamTxRecInfo[output->path_id].VPSSize=0;
		if(sEthCamTxRecInfo[output->path_id].codec == MEDIAVIDENC_H264){
			for(i=0;i<output->total_item_cnt;i++){
				sEthCamTxRecInfo[output->path_id].Desc[i]=output->item[i];
				if(sEthCamTxRecInfo[output->path_id].SPSSize==0 && output->item[i] ==0  && output->item[i+1] ==0 && output->item[i+2] ==0 && output->item[i+3] ==1 && (output->item[i+4] & 0x1F) == 0x8){
					sEthCamTxRecInfo[output->path_id].SPSSize=i;
				}
				DBG_IND("0x%x, ",output->item[i]);
			}
			DBG_IND("}\r\n");
			sEthCamTxRecInfo[output->path_id].PPSSize=output->total_item_cnt-sEthCamTxRecInfo[output->path_id].SPSSize;
		}else{
			for(i=0;i<output->total_item_cnt;i++){
				sEthCamTxRecInfo[output->path_id].Desc[i]=output->item[i];
				if(sEthCamTxRecInfo[output->path_id].VPSSize==0 && output->item[i] ==0  && output->item[i+1] ==0 && output->item[i+2] ==0 && output->item[i+3] ==1 && (output->item[i+4] >>0x1) == 33){
					sEthCamTxRecInfo[output->path_id].VPSSize=i;
				}
				if(sEthCamTxRecInfo[output->path_id].SPSSize==0 && output->item[i] ==0  && output->item[i+1] ==0 && output->item[i+2] ==0 && output->item[i+3] ==1 && (output->item[i+4] >>0x1) == 34){
					sEthCamTxRecInfo[output->path_id].SPSSize=i-sEthCamTxRecInfo[output->path_id].VPSSize;
				}
				DBG_IND("0x%x, ",output->item[i]);
			}
			DBG_IND("}\r\n");
			sEthCamTxRecInfo[output->path_id].PPSSize=output->total_item_cnt-sEthCamTxRecInfo[output->path_id].VPSSize-sEthCamTxRecInfo[output->path_id].SPSSize;
		}
		DBG_DUMP("Rec SPSSize=%d, PPSSize=%d, VPSSize=%d\r\n",sEthCamTxRecInfo[output->path_id].SPSSize, sEthCamTxRecInfo[output->path_id].PPSSize,sEthCamTxRecInfo[output->path_id].VPSSize);

	}else if(output->port_type == ETHCAM_PORT_DATA2){
		sEthCamTxDecInfo[output->path_id].DescSize=output->total_item_cnt;
		//memcpy(sEthCamTxInfo.Desc,output->item,  output->total_item_cnt);
		DBG_IND("Dec Tx SPS[%d]= {",output->total_item_cnt);
		sEthCamTxDecInfo[output->path_id].SPSSize=0;
		sEthCamTxDecInfo[output->path_id].PPSSize=0;
		sEthCamTxDecInfo[output->path_id].VPSSize=0;
		if(sEthCamTxDecInfo[output->path_id].Codec == MEDIAVIDENC_H264){
			for(i=0;i<output->total_item_cnt;i++){
				sEthCamTxDecInfo[output->path_id].Desc[i]=output->item[i];
				if(sEthCamTxDecInfo[output->path_id].SPSSize==0 && output->item[i] ==0  && output->item[i+1] ==0 && output->item[i+2] ==0 && output->item[i+3] ==1 && (output->item[i+4] & 0x1F) == 0x8){
					sEthCamTxDecInfo[output->path_id].SPSSize=i;
				}
				DBG_IND("0x%x, ",output->item[i]);
			}
			DBG_IND("}\r\n");
			sEthCamTxDecInfo[output->path_id].PPSSize=output->total_item_cnt-sEthCamTxDecInfo[output->path_id].SPSSize;
		}else{
			for(i=0;i<output->total_item_cnt;i++){
				sEthCamTxDecInfo[output->path_id].Desc[i]=output->item[i];
				if(sEthCamTxDecInfo[output->path_id].VPSSize==0 && output->item[i] ==0  && output->item[i+1] ==0 && output->item[i+2] ==0 && output->item[i+3] ==1 && (output->item[i+4] >>0x1) == 33){
					sEthCamTxDecInfo[output->path_id].VPSSize=i;
				}
				if(sEthCamTxDecInfo[output->path_id].SPSSize==0 && output->item[i] ==0  && output->item[i+1] ==0 && output->item[i+2] ==0 && output->item[i+3] ==1 && (output->item[i+4] >>0x1) == 34){
					sEthCamTxDecInfo[output->path_id].SPSSize=i-sEthCamTxDecInfo[output->path_id].VPSSize;
				}
				DBG_IND("0x%x, ",output->item[i]);
			}
			DBG_IND("}\r\n");
			sEthCamTxDecInfo[output->path_id].PPSSize=output->total_item_cnt-sEthCamTxDecInfo[output->path_id].VPSSize-sEthCamTxDecInfo[output->path_id].SPSSize;
		}
		DBG_DUMP("Dec SPSSize=%d, PPSSize=%d, VPSSize=%d\r\n",sEthCamTxDecInfo[output->path_id].SPSSize, sEthCamTxDecInfo[output->path_id].PPSSize,sEthCamTxDecInfo[output->path_id].VPSSize);

	}
	#else
	DBG_IND("path_id= %d, port_type=%d\r\n",output->path_id,output->port_type);

	sEthCamTxRecInfo[output->path_id].DescSize=output->total_item_cnt;
	//memcpy(sEthCamTxInfo.Desc,output->item,  output->total_item_cnt);
	DBG_IND("Rec Tx SPS[%d]= {",output->total_item_cnt);
	sEthCamTxRecInfo[output->path_id].SPSSize=0;
	sEthCamTxRecInfo[output->path_id].PPSSize=0;
	sEthCamTxRecInfo[output->path_id].VPSSize=0;
	//if(sEthCamTxRecInfo[output->path_id].codec == MEDIAVIDENC_H264){
	if(sEthCamTxRecInfo[output->path_id].codec == HD_CODEC_TYPE_H264){
		for(i=0;i<output->total_item_cnt;i++){
			sEthCamTxRecInfo[output->path_id].Desc[i]=output->item[i];
				if(sEthCamTxRecInfo[output->path_id].SPSSize==0 && output->item[i] ==0  && output->item[i+1] ==0 && output->item[i+2] ==0 && output->item[i+3] ==1 && (output->item[i+4] & 0x1F) == 0x8){
					sEthCamTxRecInfo[output->path_id].SPSSize=i;
				}
			DBG_IND("0x%x, ",output->item[i]);
		}
		DBG_IND("}\r\n");
		sEthCamTxRecInfo[output->path_id].PPSSize=output->total_item_cnt-sEthCamTxRecInfo[output->path_id].SPSSize;
	}else{
		for(i=0;i<output->total_item_cnt;i++){
			sEthCamTxRecInfo[output->path_id].Desc[i]=output->item[i];
			if(sEthCamTxRecInfo[output->path_id].VPSSize==0 && output->item[i] ==0  && output->item[i+1] ==0 && output->item[i+2] ==0 && output->item[i+3] ==1 && (output->item[i+4] >>0x1) == 33){
				sEthCamTxRecInfo[output->path_id].VPSSize=i;
			}
			if(sEthCamTxRecInfo[output->path_id].SPSSize==0 && output->item[i] ==0  && output->item[i+1] ==0 && output->item[i+2] ==0 && output->item[i+3] ==1 && (output->item[i+4] >>0x1) == 34){
				sEthCamTxRecInfo[output->path_id].SPSSize=i-sEthCamTxRecInfo[output->path_id].VPSSize;
			}
			DBG_IND("0x%x, ",output->item[i]);
		}
		DBG_IND("}\r\n");
		sEthCamTxRecInfo[output->path_id].PPSSize=output->total_item_cnt-sEthCamTxRecInfo[output->path_id].VPSSize-sEthCamTxRecInfo[output->path_id].SPSSize;
	}
	DBG_DUMP("Rec Codec=%d, SPSSize=%d, PPSSize=%d, VPSSize=%d\r\n",sEthCamTxRecInfo[output->path_id].codec ,sEthCamTxRecInfo[output->path_id].SPSSize, sEthCamTxRecInfo[output->path_id].PPSSize,sEthCamTxRecInfo[output->path_id].VPSSize);

	sEthCamTxDecInfo[output->path_id].DescSize=output->total_item_cnt;
	//memcpy(sEthCamTxInfo.Desc,output->item,  output->total_item_cnt);
	DBG_IND("Dec Tx SPS[%d]= {",output->total_item_cnt);
	sEthCamTxDecInfo[output->path_id].SPSSize=0;
	sEthCamTxDecInfo[output->path_id].PPSSize=0;
	sEthCamTxDecInfo[output->path_id].VPSSize=0;
	//if(sEthCamTxDecInfo[output->path_id].Codec == MEDIAVIDENC_H264){
	if(sEthCamTxDecInfo[output->path_id].Codec == HD_CODEC_TYPE_H264){
		for(i=0;i<output->total_item_cnt;i++){
			sEthCamTxDecInfo[output->path_id].Desc[i]=output->item[i];
			if(sEthCamTxDecInfo[output->path_id].SPSSize==0 && output->item[i] ==0  && output->item[i+1] ==0 && output->item[i+2] ==0 && output->item[i+3] ==1 && (output->item[i+4] & 0x1F) == 0x8){
				sEthCamTxDecInfo[output->path_id].SPSSize=i;
			}
			DBG_IND("0x%x, ",output->item[i]);
		}
		DBG_IND("}\r\n");
		sEthCamTxDecInfo[output->path_id].PPSSize=output->total_item_cnt-sEthCamTxDecInfo[output->path_id].SPSSize;
	}else{
		for(i=0;i<output->total_item_cnt;i++){
			sEthCamTxDecInfo[output->path_id].Desc[i]=output->item[i];
			if(sEthCamTxDecInfo[output->path_id].VPSSize==0 && output->item[i] ==0  && output->item[i+1] ==0 && output->item[i+2] ==0 && output->item[i+3] ==1 && (output->item[i+4] >>0x1) == 33){
				sEthCamTxDecInfo[output->path_id].VPSSize=i;
			}
			if(sEthCamTxDecInfo[output->path_id].SPSSize==0 && output->item[i] ==0  && output->item[i+1] ==0 && output->item[i+2] ==0 && output->item[i+3] ==1 && (output->item[i+4] >>0x1) == 34){
				sEthCamTxDecInfo[output->path_id].SPSSize=i-sEthCamTxDecInfo[output->path_id].VPSSize;
			}
			DBG_IND("0x%x, ",output->item[i]);
		}
		DBG_IND("}\r\n");
		sEthCamTxDecInfo[output->path_id].PPSSize=output->total_item_cnt-sEthCamTxDecInfo[output->path_id].VPSSize-sEthCamTxDecInfo[output->path_id].SPSSize;
	}
	DBG_DUMP("Dec Codec=%d, SPSSize=%d, PPSSize=%d, VPSSize=%d\r\n",sEthCamTxDecInfo[output->path_id].Codec,sEthCamTxDecInfo[output->path_id].SPSSize, sEthCamTxDecInfo[output->path_id].PPSSize,sEthCamTxDecInfo[output->path_id].VPSSize);
	#endif
}
void EthCam_MovieThumb_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_DEFAULT_FORMAT *output=(ETHCAM_XML_DEFAULT_FORMAT*)output_data;
	DBG_DUMP("bEnd=%d, cmd=%d, status=%d\r\n",bEnd,output->cmd, output->status);
}

void EthCam_MovieRawEncode_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_DEFAULT_FORMAT *output=(ETHCAM_XML_DEFAULT_FORMAT*)output_data;
	DBG_IND("bEnd=%d, cmd=%d, status=%d\r\n",bEnd,output->cmd, output->status);
	if(output->status == ETHCAM_RET_OK){
		DBG_IND("RawEncode OK\r\n");
 	}else{
		DBG_ERR("RawEncode Fail\r\n");
 	}
}
void EthCam_MovieRecInfo_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_LIST *output=(ETHCAM_XML_LIST*)output_data;
	UINT32 i;
	DBG_IND("MovieRecInfo_CB bEnd=%d, cmd=%d, total_item_cnt=%d\r\n",bEnd,output->cmd, output->total_item_cnt);
	UINT32 *pBuf;
	pBuf=(UINT32 *)&sEthCamTxRecInfo[output->path_id];
	for(i=0;i<output->total_item_cnt;i++){
		pBuf[i]=output->item[i];
	}
	sEthCamTxRecInfo[output->path_id].codec= (sEthCamTxRecInfo[output->path_id].codec == MEDIAVIDENC_H265)?HD_CODEC_TYPE_H265:HD_CODEC_TYPE_H264;

	DBG_DUMP("[%d]RecInfo, ar=%d, aud_codec=%d, codec=%d, gop=%d, vfr=%d\r\n",output->path_id,sEthCamTxRecInfo[output->path_id].ar,sEthCamTxRecInfo[output->path_id].aud_codec,sEthCamTxRecInfo[output->path_id].codec,sEthCamTxRecInfo[output->path_id].gop,sEthCamTxRecInfo[output->path_id].vfr);
	DBG_DUMP("[%d]height=%d, rec_format=%d, rec_mode=%d, tbr=%d, width=%d\r\n",output->path_id,sEthCamTxRecInfo[output->path_id].height,sEthCamTxRecInfo[output->path_id].rec_format,sEthCamTxRecInfo[output->path_id].rec_mode,sEthCamTxRecInfo[output->path_id].tbr,sEthCamTxRecInfo[output->path_id].width);
}
void EthCam_MovieDecInfo_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_LIST *output=(ETHCAM_XML_LIST*)output_data;
	UINT32 i;
	DBG_DUMP("MovieDecInfo_CB bEnd=%d, cmd=%d, total_item_cnt=%d\r\n",bEnd,output->cmd, output->total_item_cnt);
	UINT32 *pBuf;
	pBuf=(UINT32 *)&sEthCamTxDecInfo[output->path_id];
	for(i=1;i<output->total_item_cnt;i++){
		pBuf[i]=output->item[i];
	}
	sEthCamTxDecInfo[output->path_id].Codec= (sEthCamTxDecInfo[output->path_id].Codec == MEDIAVIDENC_H265)?HD_CODEC_TYPE_H265:HD_CODEC_TYPE_H264;
	DBG_DUMP("[%d]DecInfo, Tbr=%d, Fps=%d, Width=%d, Height=%d, Gop=%d, Codec=%d\r\n",output->path_id,sEthCamTxDecInfo[output->path_id].Tbr,sEthCamTxDecInfo[output->path_id].Fps,sEthCamTxDecInfo[output->path_id].Width,sEthCamTxDecInfo[output->path_id].Height,sEthCamTxDecInfo[output->path_id].Gop,sEthCamTxDecInfo[output->path_id].Codec);
}

void EthCam_TxFwUpdate_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_DEFAULT_FORMAT *output=(ETHCAM_XML_DEFAULT_FORMAT*)output_data;
	DBG_DUMP("bEnd=%d, cmd=%d, status=%d\r\n",bEnd,output->cmd, output->status);
	if(output->cmd == ETHCAM_CMD_TX_FWUPDATE_START ){
		if(output->status == ETHCAM_RET_FW_OK){
			DBG_DUMP("TxFwUpdate OK\r\n");
		}else if(output->status != ETHCAM_RET_CONTINUE){
			DBG_ERR("TxFwUpdate status=%d\r\n",output->status);
		}
	}
}
void EthCam_SyncTime_CB(INT32 bEnd, void *output_data)
{
	//ETHCAM_XML_DEFAULT_FORMAT *output=(ETHCAM_XML_DEFAULT_FORMAT*)output_data;
	//DBG_IND("cmd=%d, status=%d\r\n",output->cmd, output->status);
}
void EthCam_SyncMenuSetting_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_DEFAULT_FORMAT *output=(ETHCAM_XML_DEFAULT_FORMAT*)output_data;
	DBG_DUMP("cmd=%d, status=%d\r\n",output->cmd, output->status);
}
void EthCam_GetFrame_CB(INT32 bEnd, void *output_data)
{
	//ETHCAM_XML_VALUE_RESULT *output=(ETHCAM_XML_VALUE_RESULT*)output_data;
	//DBG_IND("path_id=%d, cmd=%d, status=%d\r\n",output->path_id,output->cmd, output->status);
}
void EthCam_TxIOStatus_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_VALUE_RESULT *output=(ETHCAM_XML_VALUE_RESULT*)output_data;
	DBG_DUMP("TxIOStatus cmd=%d, path_id=%d, status=%d, value=%d\r\n",output->cmd, output->path_id, output->status,output->value);
}
void EthCam_TxFWVersion_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_STRING_RESULT *output=(ETHCAM_XML_STRING_RESULT*)output_data;
	DBG_DUMP("TxFWVersion cmd=%d, path_id=%d, status=%d, string=%s\r\n",output->cmd, output->path_id,output->status,output->string);
}
void EthCam_SetTxIPReset_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_DEFAULT_FORMAT *output=(ETHCAM_XML_DEFAULT_FORMAT*)output_data;
	DBG_DUMP("cmd=%d, status=%d\r\n",output->cmd, output->status);
}

void EthCam_SetTxSysInfo_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_DEFAULT_FORMAT *output=(ETHCAM_XML_DEFAULT_FORMAT*)output_data;
	DBG_DUMP("SetTxSysInfo cmd=%d, path_id=%d, status=%d\r\n",output->cmd, output->path_id,output->status);
	if(output->status== sizeof(ETHCAM_TX_SYS_INFO)){
		DBG_DUMP("SetTxSysInfo cmd OK\r\n");
		sEthCamTxSysInfo[output->path_id].bCmdOK=1;
	}else if(output->status== ETHCAM_RET_CONTINUE){
	}else{
		DBG_ERR("SetTxSysInfo cmd FAIL!!!\r\n");
		sEthCamTxSysInfo[output->path_id].bCmdOK=0;
	}
}
void EthCam_SetTxCodecSrctype_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_VALUE_RESULT *output=(ETHCAM_XML_VALUE_RESULT*)output_data;
	DBG_DUMP("SetTxCodecSrctype cmd=%d, path_id=%d, status=%d, value=%d\r\n",output->cmd, output->path_id, output->status,output->value);
	if(output->value== sizeof(ETHCAM_TX_SYS_INFO)){
		DBG_DUMP("SetTxCodecSrctype cmd OK\r\n");
		sEthCamCodecSrctype[output->path_id].bCmdOK=1;
	}else{
		DBG_ERR("SetTxCodecSrctype cmd FAIL!!!\r\n");
		sEthCamCodecSrctype[output->path_id].bCmdOK=0;
	}
}

void EthCam_MovieStreamStart_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_DEFAULT_FORMAT *output=(ETHCAM_XML_DEFAULT_FORMAT*)output_data;
	DBG_DUMP("cmd=%d, status=%d\r\n",output->cmd, output->status);
}
void EthCam_MovieStreamStop_CB(INT32 bEnd, void *output_data)
{
	ETHCAM_XML_DEFAULT_FORMAT *output=(ETHCAM_XML_DEFAULT_FORMAT*)output_data;
	DBG_DUMP("cmd=%d, status=%d\r\n",output->cmd, output->status);
}

//======================XML Cmd Result CB End======================



void EthCamCmd_SetResultTable(ETHCAM_XML_RESULT *pAppCmdTbl)
{
	g_pEthCamCmdRstTbl = pAppCmdTbl;
}
ETHCAM_XML_RESULT *EthCamCmd_GetResultTable(void)
{
	if(g_pEthCamCmdRstTbl==NULL){
		DBG_ERR("Tbl NULL\r\n");
		return NULL;
	}
	return g_pEthCamCmdRstTbl;
}

ETHCAM_CMD_BEGIN(ethcam)
ETHCAM_CMD_END()


ETHCAM_XML_RESULT EthCamXMLResultTbl[]={
	{ETHCAM_CMD_STARTUP, 						ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		EthCam_Startup_CB},
	{ETHCAM_CMD_GET_TX_MOVIE_TBR, 				ETHCAM_XML_RESULT_TYPE_VALUE_RESULT, 			EthCam_MovieTBR_CB},
	{ETHCAM_CMD_GET_TX_MOVIE_REC_SIZE, 		ETHCAM_XML_RESULT_TYPE_LIST, 					EthCam_MovieRecSize_CB},
	{ETHCAM_CMD_GET_TX_MOVIE_FPS, 				ETHCAM_XML_RESULT_TYPE_VALUE_RESULT, 			EthCam_MovieFps_CB},
	{ETHCAM_CMD_GET_TX_MOVIE_GOP, 			ETHCAM_XML_RESULT_TYPE_VALUE_RESULT, 			EthCam_MovieGOP_CB},
	{ETHCAM_CMD_GET_TX_MOVIE_CODEC, 			ETHCAM_XML_RESULT_TYPE_VALUE_RESULT, 			EthCam_MovieCodec_CB},
	{ETHCAM_CMD_GET_TX_SPS_PPS, 				ETHCAM_XML_RESULT_TYPE_LIST, 					EthCam_MovieSPS_PPS_CB},
	{ETHCAM_CMD_TX_STREAM_START, 				ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		EthCam_MovieStreamStart_CB},
	{ETHCAM_CMD_TX_STREAM_STOP, 				ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		EthCam_MovieStreamStop_CB},
	{ETHCAM_CMD_GET_TX_MOVIE_THUMB, 			ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		EthCam_MovieThumb_CB},
	{ETHCAM_CMD_GET_TX_MOVIE_RAW_ENCODE, 	ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		EthCam_MovieRawEncode_CB},
	{ETHCAM_CMD_GET_TX_REC_INFO, 				ETHCAM_XML_RESULT_TYPE_LIST, 					EthCam_MovieRecInfo_CB},
	{ETHCAM_CMD_TX_RTSP_START, 				ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		NULL},
	{ETHCAM_CMD_SYNC_TIME, 					ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		EthCam_SyncTime_CB},
	{ETHCAM_CMD_SYNC_MENU_SETTING, 			ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		EthCam_SyncMenuSetting_CB},
	{ETHCAM_CMD_TX_RESET_I, 					ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		NULL},
	{ETHCAM_CMD_GET_TX_DEC_INFO, 				ETHCAM_XML_RESULT_TYPE_LIST, 					EthCam_MovieDecInfo_CB},
	{ETHCAM_CMD_TX_FWUPDATE_FWSEND, 			ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		EthCam_TxFwUpdate_CB},
	{ETHCAM_CMD_TX_FWUPDATE_START, 			ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		EthCam_TxFwUpdate_CB},
	{ETHCAM_CMD_TX_DISP_CROP, 					ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		NULL},
	{ETHCAM_CMD_GET_FRAME, 					ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		EthCam_GetFrame_CB},
	{ETHCAM_CMD_TX_RESET_QUEUE, 				ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		NULL},
	{ETHCAM_CMD_TX_POWEROFF, 					ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		NULL},
	{ETHCAM_CMD_TX_IO_STATUS, 					ETHCAM_XML_RESULT_TYPE_VALUE_RESULT, 			EthCam_TxIOStatus_CB},
	{ETHCAM_CMD_TX_FW_VERSION, 				ETHCAM_XML_RESULT_TYPE_STRING_RESULT, 			EthCam_TxFWVersion_CB},
	{ETHCAM_CMD_SET_TX_IP_RESET, 				ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		EthCam_SetTxIPReset_CB},
	{ETHCAM_CMD_SET_TX_SYS_INFO, 				ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		EthCam_SetTxSysInfo_CB},
	{ETHCAM_CMD_DUMP_TX_BS_INFO, 				ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		NULL},
	{ETHCAM_CMD_CHANGE_MODE, 					ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		NULL},
	{ETHCAM_CMD_IPERF_TEST, 					ETHCAM_XML_RESULT_TYPE_DEFAULT_FORMAT, 		NULL},
	{ETHCAM_CMD_SET_TX_CODEC_SRCTYPE, 		ETHCAM_XML_RESULT_TYPE_VALUE_RESULT, 			EthCam_SetTxCodecSrctype_CB},
};
