#include "ethcam_test.h"
#include <kwrap/stdio.h>

#include "EthCamAppNetwork.h"


#include "EthCamAppCmd.h"
#include "EthCam/EthCamSocket.h"
#include "EthCam/EthsockIpcAPI.h"
#include "EthCam/ethsocket.h"
#include "EthCamAppSocket.h"
#include "BackgroundObj.h"
#include "Utility/SwTimer.h"

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          EthCamAppNetwork
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////
#define NET_IP_ETH0            "192.168.0.3"
#define NET_SRVIP_ETH0         "192.168.0.3"
#define NET_LEASE_START_ETH0   "192.168.0.12"
#define NET_LEASE_END_ETH0     	"192.168.0.12"
#define UDP_PORT 2222
#define FIX_IP_MAX_LEN              16

static UINT32 g_bEthCamEthLinkStatus[ETHCAM_PATH_ID_MAX]={ETHCAM_LINK_DOWN,ETHCAM_LINK_DOWN};
static UINT32 g_bEthCamPrevEthLinkStatus[ETHCAM_PATH_ID_MAX]={ETHCAM_LINK_DOWN,ETHCAM_LINK_DOWN};
int SX_TIMER_ETHCAM_DATARECVDET_ID = -1;


UINT32 EthCamHB1[ETHCAM_PATH_ID_MAX]={0}, EthCamHB2 = 0;
int SX_TIMER_ETHCAM_ETHERNETLINKDET_LINKDET_ID = -1;
UINT32 gEthCamDhcpSrvConnIpAddr=0;//[ETHCAM_MAX_TX_NUM] = {0}; //for DHCP lease is 1,keep Mac of IP

ETHCAM_SOCKET_INFO SocketInfo[]={{"192.168.0.12", 8887,8888,8899}};
UINT32 ipstr2int(char* ip_addr)
{
	UINT32 nRet = 0;
	UINT16 i = 0;
	CHAR chBuf[16] = "";

	memcpy(chBuf, ip_addr ,15);
	//DBG_DUMP("ip_addr=%s\r\n",ip_addr);

	CHAR* szBuf = strtok(chBuf,".");
	//DBG_DUMP("szBuf=%s\r\n",szBuf);
	while(NULL != szBuf)
	{
		nRet += atoi(szBuf)<<((3-i)*8);
		szBuf = strtok(NULL,".");
		i++;
	}
	nRet=((nRet & 0xFF)<< 24) + ((nRet & 0xFF00)<< 8) + ((nRet & 0xFF0000)>>8) + ((nRet & 0xFF000000)>> 24);
	return nRet;
}
#if 1
int udpsocket_recv(char *addr, int size)
{
	char cmdname[64];
	unsigned int CliIPAddr=0;
	unsigned int CliMacAddr[2]={0};
	char chCliIPAddr[FIX_IP_MAX_LEN]={0};
	char chCliMacAddr[20]={0};

	sscanf_s(addr, "%s %d %d %d", cmdname, 50, &CliIPAddr, &CliMacAddr[0], &CliMacAddr[1]);
	if(strcmp(cmdname,"cliconnipnotify")==0){
		snprintf(chCliIPAddr, FIX_IP_MAX_LEN, "%d.%d.%d.%d", (CliIPAddr & 0xFF), (CliIPAddr >> 8) & 0xFF, (CliIPAddr >> 16) & 0xFF, (CliIPAddr >> 24) & 0xFF);
		snprintf(chCliMacAddr, 20, "%02x:%02x:%02x:%02x:%02x:%02x", (CliMacAddr[0] & 0xFF), (CliMacAddr[0] >> 8) & 0xFF, (CliMacAddr[0] >> 16) & 0xFF, (CliMacAddr[0] >> 24) & 0xFF , (CliMacAddr[1]) & 0xFF ,(CliMacAddr[1] >> 8) & 0xFF);
		DBG_DUMP("Get=%s ,CliIPAddr=0x%x, %s, CliMac=%s\r\n", cmdname, CliIPAddr,chCliIPAddr, chCliMacAddr);

		//strcpy(cmdname, "");
		//snprintf(cmdname, sizeof(cmdname) - 1, "cliconnipnotify %d %d %d",CliIPAddr,CliMacAddr[0],CliMacAddr[1]);
		if(gEthCamDhcpSrvConnIpAddr==0){
			strcpy(SocketInfo[0].ip, chCliIPAddr);
			strcpy(SocketInfo[0].mac, chCliMacAddr);
			strcpy(cmdname, "");
			snprintf(cmdname, sizeof(cmdname) - 1, "SrvCliConnIPLink %d %d",(int)ipstr2int(SocketInfo[0].ip), (int)ETHCAM_LINK_UP);
			EthCamNet_EthLinkStatusNotify(cmdname);
			gEthCamDhcpSrvConnIpAddr=CliIPAddr;
		}
	}
	return 1;
}
void udpsocket_notify(int status, int parm)
{
	switch (status) {
	case CYG_ETHSOCKET_UDP_STATUS_CLIENT_REQUEST: {
		}
		break;
	}
}
void udpsocket_open(void)
{
	//get client static ip
	ethsocket_install_obj  udpsocket_obj = {0};
	DBG_DUMP("open udp usocket\r\n");
	udpsocket_obj.notify = udpsocket_notify;
	udpsocket_obj.recv = udpsocket_recv;
	udpsocket_obj.portNum = UDP_PORT;
	udpsocket_obj.threadPriority = 8;
	udpsocket_obj.sockbufSize = 1024; //default 8192
	ethsocket_udp_install(&udpsocket_obj);
	ethsocket_udp_open();
}
#endif
SWTIMER_ID   g_EthLinkDetTimer_Id;
UINT32         g_isEthLinkDetTimerOpenOK = TRUE;
static void EthCamNet_EnableEthernetLinkDet_TimerCB(UINT32 uiEvent)
{
	EthCamNet_EthernetLinkDet();
}

void EthCamNet_EnableEthernetLinkDet(void)
{
	if (SwTimer_Open(&g_EthLinkDetTimer_Id, EthCamNet_EnableEthernetLinkDet_TimerCB) != E_OK) {
		DBG_ERR("open timer fail\r\n");
		g_isEthLinkDetTimerOpenOK = FALSE;
	} else {
		SwTimer_Cfg(g_EthLinkDetTimer_Id, 500 /*ms*/, SWTIMER_MODE_FREE_RUN);
		SwTimer_Start(g_EthLinkDetTimer_Id);
	}
}
void EthCamNet_DisableEthernetLinkDet(void)
{
	if(g_isEthLinkDetTimerOpenOK){
		SwTimer_Stop(g_EthLinkDetTimer_Id);
		SwTimer_Close(g_EthLinkDetTimer_Id);
	}
}
void EthCamNet_EthernetLinkDown(void)
{
	char ipccmd[40]={0};
	ethsocket_udp_close();
	snprintf(ipccmd, sizeof(ipccmd) - 1, "ethlinknotify %d %d",(int)gEthCamDhcpSrvConnIpAddr, (int)ETHCAM_LINK_DOWN);
	EthCamNet_EthLinkStatusNotify(ipccmd);
	gEthCamDhcpSrvConnIpAddr=0;
	socketCliEthData_DestroyAllBuff(0);
}
void EthCamNet_EthernetLinkDet(void)
{
	int LinkSta = ETHCAM_LINK_DOWN;
	static int PrvLinkSta = ETHCAM_LINK_DOWN;
	int h_eth0;
	char eth0_info[10] = {0};
	char ipccmd[40]={0};

	if ((h_eth0 = open("/sys/class/net/eth0/operstate", O_RDONLY)) < 0) {
		DBG_ERR("check eth0 operstate fail\r\n");
	} else {
		read(h_eth0, eth0_info, sizeof(eth0_info));
		close(h_eth0);
		LinkSta = (eth0_info[0] == 'u') ? ETHCAM_LINK_UP : ETHCAM_LINK_DOWN;
	}
	if(PrvLinkSta != LinkSta){
		if(LinkSta==ETHCAM_LINK_UP){
			DBG_DUMP("eth0 LinkUp\r\n");
		}else{
			DBG_DUMP("eth0 LinkDown\r\n");
		}
		PrvLinkSta = LinkSta;
		if(LinkSta== ETHCAM_LINK_UP){
			udpsocket_open();
			//DBG_DUMP("client_ip=0x%x\r\n",client_ip);
			char udp_cmd[50]={0};
			int udp_cmdsize=sizeof(udp_cmd);
			udp_cmdsize=snprintf(udp_cmd, sizeof(udp_cmd) - 1, "cliconncheck");
			DBG_DUMP("udp cmd=%s, sz=%d\r\n",udp_cmd, udp_cmdsize);
			ethsocket_udp_sendto(NET_LEASE_START_ETH0, UDP_PORT, udp_cmd, &udp_cmdsize);
		}else{
			ethsocket_udp_close();
			snprintf(ipccmd, sizeof(ipccmd) - 1, "ethlinknotify %d %d",(int)gEthCamDhcpSrvConnIpAddr, (int)LinkSta);
			EthCamNet_EthLinkStatusNotify(ipccmd);
			gEthCamDhcpSrvConnIpAddr=0;
		}
	}
	if(LinkSta==ETHCAM_LINK_UP && gEthCamDhcpSrvConnIpAddr==0){
		char udp_cmd[50]={0};
		int udp_cmdsize=sizeof(udp_cmd);
		udp_cmdsize=snprintf(udp_cmd, sizeof(udp_cmd) - 1, "cliconncheck");
		//DBG_DUMP("udp cmd=%s, sz=%d\r\n",udp_cmd, udp_cmdsize);
		ethsocket_udp_sendto(NET_LEASE_START_ETH0, UDP_PORT, udp_cmd, &udp_cmdsize);
	}
}
UINT32 EthCamNet_GetEthLinkStatus(ETHCAM_PATH_ID path_id)
{
	return g_bEthCamEthLinkStatus[path_id];
}
UINT32 EthCamNet_GetPrevEthLinkStatus(ETHCAM_PATH_ID path_id)
{
	return g_bEthCamPrevEthLinkStatus[path_id];
}
void EthCamNet_SetPrevEthLinkStatus(ETHCAM_PATH_ID path_id, UINT32 LinkStatus)
{
	g_bEthCamPrevEthLinkStatus[path_id]=LinkStatus;
}
void EthCamNet_EthLinkStatusNotify(char *cmd)
{
	char cmdname[40];
	UINT32 path_id_fr_eth=ETHCAM_PATH_ID_MAX;
	UINT32 TxIPAddr=0;
	CHAR chTxIPAddr[ETHCAM_SOCKETCLI_IP_MAX_LEN];
	UINT32 status;
	UINT32 path_id;
	UINT16 i=0;
	UINT32 AllPathLinkStatus=0;
	ETHCAM_SOCKET_INFO EthSocketInfo[ETHCAM_PATH_ID_MAX];
	sscanf_s(cmd, "%s %d %d", cmdname, 40, &TxIPAddr, &status);
	snprintf(chTxIPAddr, ETHCAM_SOCKETCLI_IP_MAX_LEN, "%d.%d.%d.%d", (int)(TxIPAddr & 0xFF), (int)(TxIPAddr >> 8) & 0xFF, (int)(TxIPAddr >> 16) & 0xFF, (int)(TxIPAddr >> 24) & 0xFF);

	DBG_DUMP("Get = %s ,status=%d, TxIPAddr=0x%x, %s\r\n", cmdname, status,TxIPAddr,chTxIPAddr);


	if(TxIPAddr==0){
		//DBG_ERR("Tx No IP!!\r\n");
		return;
	}

	for(i=0;i<ETHCAM_PATH_ID_MAX;i++){
		if(strcmp(chTxIPAddr,SocketInfo[i].ip)==0){
			path_id_fr_eth=i;
			break;
		}
	}
	DBG_DUMP("path_id_fr_eth=%d\r\n", path_id_fr_eth);

	if(path_id_fr_eth>=ETHCAM_PATH_ID_MAX){
		DBG_ERR("path id error %d\r\n", path_id_fr_eth);
		return;
	}
	if(strcmp(SocketInfo[i].ip,"0.0.0.0")==0){
		DBG_ERR("SocketInfo[%d].ip error ,ip=%s\r\n",i, SocketInfo[i].ip);
		return;
	}

	memcpy(&EthSocketInfo[0], &SocketInfo[0],sizeof(ETHCAM_SOCKET_INFO)*1);
	path_id=path_id_fr_eth;

	g_bEthCamPrevEthLinkStatus[path_id]=g_bEthCamEthLinkStatus[path_id];
	if(status==ETHCAM_LINK_UP){
		DBG_DUMP("link up\r\n");
		g_bEthCamEthLinkStatus[path_id]=ETHCAM_LINK_UP;
		EthCamSocket_SetEthLinkStatus(path_id, ETHCAM_LINK_UP);
		EthCamSocketCli_SetSvrInfo(EthSocketInfo, 1);
		EthCamSocketCli_ReConnect(path_id, 0 ,0);
		//BKG_PostEvent(NVTEVT_BKW_ETHCAM_SOCKETCLI_CMD_OPEN);
		BackgroundExeEvt(NVTEVT_BKW_ETHCAM_SOCKETCLI_CMD_OPEN);

		EthCamHB1[path_id] = 0;
		EthCamHB2 = 0;

	}else if(status==ETHCAM_LINK_DOWN){
		DBG_DUMP("link down\r\n");
		g_bEthCamEthLinkStatus[path_id]=ETHCAM_LINK_DOWN;
		EthCamSocket_SetEthLinkStatus(path_id, ETHCAM_LINK_DOWN);

		//clear ip, for Tx unplug and power off
		if(strcmp(cmdname,"EthHubLink")==0){
			//DBG_WRN("clear IP addr\r\n");
			strcpy(SocketInfo[path_id].ip, "0.0.0.0");
			memset(SocketInfo[path_id].mac,0,ETHCAM_SOCKETCLI_MAC_MAX_LEN);
		}
		for(i=0;i<ETHCAM_PATH_ID_MAX;i++){
			if(g_bEthCamEthLinkStatus[i]==ETHCAM_LINK_UP){
				AllPathLinkStatus++;
			}
		}
		//for unplug and ethcam cmd not finish
		EthCamCmd_Done(path_id, 0xFFFFFFFF, ETHCAM_CMD_TERMINATE);

		socketCliEthData1_SetRecv(path_id, 0);
		socketCliEthData2_SetRecv(path_id, 0);
		EthCamSocketCli_ReConnect(path_id, 0, 0);
		EthCamSocketCli_Close(path_id, ETHSOCKIPCCLI_ID_0);
		EthCamSocketCli_Close(path_id, ETHSOCKIPCCLI_ID_1);
		EthCamSocketCli_Close(path_id, ETHSOCKIPCCLI_ID_2);

		if(AllPathLinkStatus==0){
			EthCamHB1[path_id] = 0;
			EthCamHB2 = 0;
		}
		g_bEthCamPrevEthLinkStatus[path_id]=ETHCAM_LINK_DOWN;
	}
}
void EthCamNet_SrvCliConnIPAddrNofity(char *cmd)
{
	char cmdname[50];
	UINT32 CliIPAddr=0;
	UINT32 CliMacAddr[2]={0};
	CHAR chCliIPAddr[ETHCAM_SOCKETCLI_IP_MAX_LEN]={0};
	CHAR chCliMacAddr[ETHCAM_SOCKETCLI_MAC_MAX_LEN]={0};

	sscanf_s(cmd, "%s %d %d %d", cmdname, 50, &CliIPAddr, &CliMacAddr[0], &CliMacAddr[1]);
	snprintf(chCliIPAddr, ETHCAM_SOCKETCLI_IP_MAX_LEN, "%d.%d.%d.%d", (int)(CliIPAddr & 0xFF), (int)(CliIPAddr >> 8) & 0xFF, (int)(CliIPAddr >> 16) & 0xFF, (int)(CliIPAddr >> 24) & 0xFF);
	snprintf(chCliMacAddr, ETHCAM_SOCKETCLI_MAC_MAX_LEN, "%02x:%02x:%02x:%02x:%02x:%02x", (int)(CliMacAddr[0] & 0xFF), (int)(CliMacAddr[0] >> 8) & 0xFF, (int)(CliMacAddr[0] >> 16) & 0xFF, (int)(CliMacAddr[0] >> 24) & 0xFF , (int)(CliMacAddr[1]) & 0xFF ,(int)(CliMacAddr[1] >> 8) & 0xFF);
	DBG_DUMP("Get=%s ,CliIPAddr=0x%x, %s, CliMac=%s\r\n", cmdname, CliIPAddr,chCliIPAddr, chCliMacAddr);


	strcpy(SocketInfo[0].ip, chCliIPAddr);
	strcpy(SocketInfo[0].mac, chCliMacAddr);
	DBG_DUMP("SocketInfo[0].ip=%s\r\n",SocketInfo[0].ip);
	snprintf(cmdname, sizeof(cmdname) - 1, "SrvCliConnIPLink %d %d",(int)ipstr2int(SocketInfo[0].ip), (int)ETHCAM_LINK_UP);
	EthCamNet_EthLinkStatusNotify(cmdname);
}
