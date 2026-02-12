#include "PrjCfg.h"
#include <kwrap/stdio.h>

#include "EthCamAppNetwork.h"

#include "WiFiIpc/nvtwifi.h"


#include "UIAppWiFiCmd.h"

#include "UsockIpc/UsockIpcAPI.h"
//#include "SysCfg.h"

#include "UIAppNetwork.h"
#include "EthCamAppCmd.h"
#include "SysMain.h"
#include "SysCommon.h"
#include "UIAppNetwork.h"
#include "EthCam/EthCamSocket.h"
#include "EthCam/EthsockIpcAPI.h"
#include "EthCam/ethsocket.h"
#include "EthCamAppSocket.h"
#include "UIApp/Background/UIBackgroundObj.h"
#include "UIApp/Movie/UIAppMovie.h"
//#include "ImageApp_MovieMulti.h"
#include "Mode/UIModeUpdFw.h"
#include "UIControl/UIControlWnd.h"
#include "UIWnd/UIFlow.h"
//#include "NvtIpcAPI.h"
#include "EthCam/mdcmdio.h"
#include "io/gpio.h"
//#include "EthCam/EthsockCliIpcAPI.h"
#include "Utility/SwTimer.h"

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          EthCamAppNetwork
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////
#if (defined(_NVT_ETHREARCAM_TX_))
#define NET_IP_ETH0            "192.168.0.12"
#else
#define NET_IP_ETH0            "192.168.0.3"
#endif
#define NET_SRVIP_ETH0         "192.168.0.3"
#define NET_LEASE_START_ETH0   "192.168.0.12"
#define NET_LEASE_END_ETH0     	"192.168.0.13"
#define UDP_PORT 2222
#define FIX_IP_MAX_LEN              16

#define ETHCAM_CLR_BIT(reg, bit)   ((reg) &= ~(1 << (bit)))
#define ETHCAM_SET_BIT(reg, bit)   ((reg) |= (1 << (bit)))

#if (defined(_NVT_ETHREARCAM_TX_) || defined(_NVT_ETHREARCAM_RX_))
static UINT32 g_bEthCamEthLinkStatus[ETHCAM_PATH_ID_MAX]={ETHCAM_LINK_DOWN,ETHCAM_LINK_DOWN};
static UINT32 g_bEthCamPrevEthLinkStatus[ETHCAM_PATH_ID_MAX]={ETHCAM_LINK_DOWN,ETHCAM_LINK_DOWN};
int SX_TIMER_ETHCAM_DATARECVDET_ID = -1;

#if defined(_NVT_ETHREARCAM_TX_)
UINT32 EthCamTxHB = 0;
#endif
int SX_TIMER_ETHCAM_ETHERNETLINKDET_LINKDET_ID = -1;

#if (defined(_NVT_ETHREARCAM_RX_))
UINT32 EthCamHB1[ETHCAM_PATH_ID_MAX]={0}, EthCamHB2 = 0;
#if (ETH_REARCAM_CAPS_COUNT>=2)
#define ETHHUB_LINK_STATUS_REG		1
#define ETHHUB_LINK_STATUS_REG_BIT	2
#define ETHHUB_LINK_AN_COMPLETE_BIT	5
#define ETHHUB_LINK_CTRL_REG			0
#define ETHHUB_LINK_CTRL_REG_ISOLATE_BIT 10
int SX_TIMER_ETHCAM_ETHHUB_LINKDET_ID = -1;
UINT32  EthCamEthHub_LinkStatus[ETHCAM_PATH_ID_MAX]={0};
UINT32  EthCamEthHub_PrevLinkStatus[ETHCAM_PATH_ID_MAX]={0};
UINT32  EthCamEthHub_ANStatus[ETHCAM_PATH_ID_MAX]={0};
UINT16 ETHCAM_ETHHUB_PHY_PORT[ETHCAM_PATH_ID_MAX]={0, 2};//, 5, 6, 7};
static BOOL g_isChkPortReady = FALSE;
static UINT32 g_ChkPortReadyIPAddr = 0;
static UINT32 g_PrevSrvCliConnCliMacAddr[2]={0};
static UINT32 g_SrvCliConnCliMacAddrCnt=0;
static SWTIMER_ID  g_EthHubChkPortReadyTimeID=SWTIMER_NUM;
static SWTIMER_ID  g_EthHubPortReadySendCmdTimeID=SWTIMER_NUM;
static UINT32  g_isEthHubPortReadySendCmdTimerOpen=0;
static UINT32 g_isEthHubPortReadyCmdAck = 0;
static UINT32  g_EthHubChkPortReadyTimerRunning=0;
static BOOL g_bLastTxDet     = FALSE;
static BOOL g_bLastTxStatus  = FALSE;
static UINT32 g_isEthCamIPConflict=0;
#endif
UINT32 gEthCamDhcpSrvConnIpAddr=0;//[ETHCAM_MAX_TX_NUM] = {0}; //for DHCP lease is 1,keep Mac of IP
BOOL g_bEthCamApp_IperfTest=0;
#endif
#if (defined(_NVT_ETHREARCAM_TX_))
//extern ER efuse_get_unique_id(UINT32 * id_L, UINT32 * id_H);
//extern ER   eth_setConfig(ETH_CONFIG_ID configID, UINT32 uiConfig);
//extern ER   eth_getConfig(ETH_CONFIG_ID configID, UINT32 *pConfigContext);
ETHCAM_SOCKET_INFO SocketInfo[ETHCAM_PATH_ID_MAX]={{"192.168.0.12", 8887,8888,8899},{"192.168.0.13",8887,8888,8899}};
static UINT32 g_IsSocketOpenBKGTsk=0;
#else
#if (ETH_REARCAM_CAPS_COUNT>=2)
//HCAM_SOCKET_INFO SocketInfo[]={{"0.0.0.0", 8887,8888,8899},{"0.0.0.0",8887,8888,8899}};
ETHCAM_SOCKET_INFO SocketInfo[]={{"0.0.0.0", 8887,8888,8899,"00:00:00:00:00:00"},{"0.0.0.0",8887,8888,8899,"00:00:00:00:00:00"}};
#else
ETHCAM_SOCKET_INFO SocketInfo[]={{"192.168.0.12", 8887,8888,8899}};
#endif
#endif
//extern void ethernetif_set_mac_address(char *mac_address);
UINT32 ipstr2int(char* ip_addr)
{
	UINT32 nRet = 0;
	UINT16 i = 0;
	char chBuf[16] = {0};
	char *p_chBuf= chBuf;

	strncpy(chBuf,ip_addr,16);
	//DBG_DUMP("ip_addr=%s, chBuf=%s\r\n",ip_addr, chBuf);

	char* szBuf = strsep(&p_chBuf,".");
	//DBG_DUMP("szBuf=%s\r\n",szBuf);
	while(NULL != szBuf)
	{
	    nRet += atoi(szBuf)<<((3-i)*8);
	    szBuf = strsep(&p_chBuf,".");
	    //DBG_DUMP("i=%d, szBuf=%s, nRet=0x%x\r\n",i, szBuf,nRet);
	    i++;
	}
	nRet=((nRet & 0xFF)<< 24) + ((nRet & 0xFF00)<< 8) + ((nRet & 0xFF0000)>>8) + ((nRet & 0xFF000000)>> 24);
	return nRet;
}
BOOL EthCamNet_IsIPConflict(void)
{
#if (defined(_NVT_ETHREARCAM_RX_) && (ETH_REARCAM_CAPS_COUNT>=2))
	return g_isEthCamIPConflict;
#else
	return 0;
#endif
}

void EthCamNet_EthHubPortIsolate(UINT32 port_id, BOOL port_isolate)
{
#if (ETHCAM_CHECK_PORTREADY == ENABLE)
#if (defined(MDC_GPIO) && defined(MDIO_GPIO))
	UINT32  read_data;
	UINT32  status;
	if(port_id>1){
		DBG_ERR("port_id=%d out of range!\r\n",port_id);
		return;
	}
	smiInit(MDC_GPIO, MDIO_GPIO);
	smiRead(ETHCAM_ETHHUB_PHY_PORT[port_id], ETHHUB_LINK_CTRL_REG, &read_data);
	//DBG_DUMP("read_data=0x%x\r\n",read_data);
	status= (read_data==0xffff) ?  0: ((read_data & (1<< ETHHUB_LINK_CTRL_REG_ISOLATE_BIT)) >> ETHHUB_LINK_CTRL_REG_ISOLATE_BIT);
	//DBG_DUMP("[%d]read_data=0x%x, now_sta=%d, want_sta=%d\r\n",port_id,read_data,status,port_isolate);
	if(status == (UINT32)port_isolate){
		return;
	}
	if(port_isolate){
		DBG_DUMP("[%d]ISOLATE ON\r\n",port_id);
		ETHCAM_SET_BIT(read_data, ETHHUB_LINK_CTRL_REG_ISOLATE_BIT);
	}else{
		DBG_DUMP("[%d]ISOLATE OFF\r\n",port_id);
		ETHCAM_CLR_BIT(read_data, ETHHUB_LINK_CTRL_REG_ISOLATE_BIT);
	}
	//DBG_DUMP("read_data BIT=0x%x\r\n",read_data);
	smiWrite(ETHCAM_ETHHUB_PHY_PORT[port_id], ETHHUB_LINK_CTRL_REG, read_data);
#endif
#endif
}
char g_udp_cmd[50]={0};
int g_udp_cmdsize=50;
int udpsocket_recv(char *addr, int size)
{
	if (addr && (size > 0)) {
#if (defined(_NVT_ETHREARCAM_RX_))
	char cmdname[64];
	unsigned int CliIPAddr=0;
	unsigned int CliMacAddr[2]={0};
	char chCliIPAddr[FIX_IP_MAX_LEN]={0};
	char chCliMacAddr[20]={0};

	//sscanf_s(addr, "%s %d %d %d", cmdname, 50, &CliIPAddr, &CliMacAddr[0], &CliMacAddr[1]);
	sscanf_s(addr, "%s %d %d %d", cmdname, sizeof(cmdname)-1, &CliIPAddr, &CliMacAddr[0], &CliMacAddr[1]);

	//DBG_DUMP("udpsocket_recv Get=%s \r\n", addr);
	//DBG_DUMP("udpsocket_recv cmdname=%s \r\n", cmdname);

	if(strcmp(cmdname,"cliconnipnotify")==0){
		snprintf(chCliIPAddr, FIX_IP_MAX_LEN, "%d.%d.%d.%d", (CliIPAddr & 0xFF), (CliIPAddr >> 8) & 0xFF, (CliIPAddr >> 16) & 0xFF, (CliIPAddr >> 24) & 0xFF);
		snprintf(chCliMacAddr, 20, "%02x:%02x:%02x:%02x:%02x:%02x", (CliMacAddr[0] & 0xFF), (CliMacAddr[0] >> 8) & 0xFF, (CliMacAddr[0] >> 16) & 0xFF, (CliMacAddr[0] >> 24) & 0xFF , (CliMacAddr[1]) & 0xFF ,(CliMacAddr[1] >> 8) & 0xFF);
		DBG_DUMP("udp_recv Get=%s ,CliIPAddr=0x%x, %s, CliMac=%s\r\n", cmdname, CliIPAddr,chCliIPAddr, chCliMacAddr);

		strcpy(cmdname, "");
		snprintf(cmdname, sizeof(cmdname) - 1, "cliconnipnotify udp %d %d %d",CliIPAddr,CliMacAddr[0],CliMacAddr[1]);
		#if(ETH_REARCAM_CAPS_COUNT>=2)
			//EthCam_SrvCliConnIPNofity(cmdname);
			EthCamNet_SrvCliConnIPAddrNofity(cmdname);
		#else
		if(gEthCamDhcpSrvConnIpAddr==0){
			strcpy(SocketInfo[0].ip, chCliIPAddr);
			strcpy(SocketInfo[0].mac, chCliMacAddr);
			strcpy(cmdname, "");
			snprintf(cmdname, sizeof(cmdname) - 1, "SrvCliConnIPLink %d %d",ipstr2int(SocketInfo[0].ip), ETHCAM_LINK_UP);
			EthCamNet_EthLinkStatusNotify(cmdname);
			gEthCamDhcpSrvConnIpAddr=CliIPAddr;
		}
		#endif
	}
#endif
#if (defined(_NVT_ETHREARCAM_TX_))
	char cmdname[64];
	sscanf_s(addr, "%s", cmdname, 50);
	if(strcmp(cmdname,"cliconncheck")==0){
		DBG_DUMP("Get=%s\r\n", cmdname);
		if(g_udp_cmdsize){
			DBG_DUMP("udp cmd=%s, sz=%d\r\n",g_udp_cmd, g_udp_cmdsize);
			ethsocket_udp_sendto(NET_SRVIP_ETH0, UDP_PORT, g_udp_cmd, &g_udp_cmdsize);
		}
	}
#endif
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
bool EthCamNet_DhcpLeaseCheck(void)
{
	bool is_get_client_ip=0;
#if(ETH_REARCAM_CAPS_COUNT==1)
	int h_lease;
	char lease_info[128] = {0};
	unsigned int client_ip=(unsigned int)ipstr2int(NET_LEASE_START_ETH0);//0xc0a8000c;
	unsigned int bytes_read, i;

	//DBG_DUMP("client_ip=0x%x\r\n",client_ip);
	if ((h_lease = open("/var/lib/misc/udhcpd.leases", O_RDONLY)) < 0) {
		DBG_WRN("open leases fail\r\n");
	} else {
		bytes_read=read(h_lease, lease_info, sizeof(lease_info));
		//DBG_DUMP("bytes_read=%d\r\n",bytes_read);
		close(h_lease);
		for(i=0;i<bytes_read;i++){
			if (memcmp((void *)&lease_info[i], (void *)&client_ip, 4) == 0) {
				DBG_DUMP("lease_info[%d]=0x%x,0x%x,0x%x,0x%x\r\n",i,lease_info[i],lease_info[i+1],lease_info[i+2],lease_info[i+3]);
				gEthCamDhcpSrvConnIpAddr=client_ip;
				is_get_client_ip=1;
				break;
			}
		}
	}
#endif
	return is_get_client_ip;
}
UINT32 gbEthCamEthLinkStatus=ETHCAM_LINK_DOWN;
UINT32 gEthCamDhcpCliOpened = false;
UINT32 gEthCamDhcpCliIP = 0;
unsigned int ethlinkstatusget(void)
{
	return gbEthCamEthLinkStatus;
}
void ethlinkstatusset(unsigned int LinkStatus)
{
	gbEthCamEthLinkStatus=LinkStatus;
}
void ethlinkstatusnofity(unsigned int sts)
{
	//DBG_DUMP("ethlinkstatusnofity=================================sts=%d\n",sts);

	unsigned int ethcam_tx_ipaddr=0xc00a8c0;//192.168.0.12
	char ipccmd[40];
	#if 0
	ethlinkstatusset(sts);
	if(sts==ETHCAM_LINK_UP){
		if(gEthCamDhcpCliOpened && gEthCamDhcpCliIP==0){//DHCP
			DBG_DUMP("Dhcp start but NO IP Get!\r\n"); //wait ethcamsettxip cmd
			return;
		}else if(gEthCamDhcpCliOpened==0 && gEthCamDhcpCliIP==0){//static
			if(gEthCamDhcpCliFailIP==true){
				init_eth0();//retry
			}else{
				DBG_DUMP("NO IP Get!\r\n"); //wait ethcamsettxip cmd
			}
			return;
		}else{
			if(gEthCamDhcpCliOpened==0){
			}
		}
	}else{
		if(gEthCamDhcpCliOpened){
		}
	}
	ethcam_tx_ipaddr=gEthCamDhcpCliIP;
	#endif
	snprintf(ipccmd, sizeof(ipccmd) - 1, "ethlinknotify %d %d",ethcam_tx_ipaddr, sts);
	//EthCamNet_EthLinkStatusNotify("ethlinknotify 201369792 2");
	//DBG_DUMP("ipccmd=%s\n",ipccmd);
	EthCamNet_EthLinkStatusNotify(ipccmd);
}

void EthCamNet_EthernetLinkDet(void)
{
	int LinkSta = ETHCAM_LINK_DOWN;
	static int PrvLinkSta = ETHCAM_LINK_DOWN;
	int h_eth0;
	char eth0_info[10] = {0};
#if (defined(_NVT_ETHREARCAM_RX_))
	char ipccmd[40]={0};
	bool is_get_client_ip=0;
#endif

	if ((h_eth0 = open("/sys/class/net/eth0/operstate", O_RDONLY)) < 0) {
		DBG_ERR("check eth0 operstate fail\r\n");
	} else {
		read(h_eth0, eth0_info, sizeof(eth0_info));
		close(h_eth0);
		LinkSta = (eth0_info[0] == 'u') ? ETHCAM_LINK_UP : ETHCAM_LINK_DOWN;
	}
	//DBG_DUMP("LinkSta=%d\r\n",LinkSta);

	if(PrvLinkSta != LinkSta){
		if(LinkSta==ETHCAM_LINK_UP){
			DBG_DUMP("eth0 LinkUp\r\n");
		}else{
			DBG_DUMP("eth0 LinkDown\r\n");
		}
		PrvLinkSta = LinkSta;
#if (defined(_NVT_ETHREARCAM_RX_))
		if(LinkSta== ETHCAM_LINK_UP){
			udpsocket_open();
			//DBG_DUMP("client_ip=0x%x\r\n",client_ip);
			is_get_client_ip=EthCamNet_DhcpLeaseCheck();
			if(is_get_client_ip==0){
				char udp_cmd[50]={0};
				int udp_cmdsize=sizeof(udp_cmd);
				udp_cmdsize=snprintf(udp_cmd, sizeof(udp_cmd) - 1, "cliconncheck");
				DBG_DUMP("udp cmd=%s, sz=%d\r\n",udp_cmd, udp_cmdsize);
				ethsocket_udp_sendto(NET_LEASE_START_ETH0, UDP_PORT, udp_cmd, &udp_cmdsize);
				#if(ETH_REARCAM_CAPS_COUNT>=2)
				ethsocket_udp_sendto(NET_LEASE_END_ETH0, UDP_PORT, udp_cmd, &udp_cmdsize);
				if(SxTimer_GetFuncActive(SX_TIMER_ETHCAM_ETHHUB_LINKDET_ID)==0){
					//DBG_DUMP("Active ETHHUB Timer\r\n");
					SxTimer_SetFuncActive(SX_TIMER_ETHCAM_ETHHUB_LINKDET_ID, TRUE);
				}
				#endif
			}else{
				//snprintf(ipccmd, sizeof(ipccmd) - 1, "ethlinknotify %d %d",201369792, LinkSta);
				snprintf(ipccmd, sizeof(ipccmd) - 1, "ethlinknotify %d %d",gEthCamDhcpSrvConnIpAddr, LinkSta);
				EthCamNet_EthLinkStatusNotify(ipccmd);
			}
		}else{
			ethsocket_udp_close();
			snprintf(ipccmd, sizeof(ipccmd) - 1, "ethlinknotify %d %d",gEthCamDhcpSrvConnIpAddr, LinkSta);
			EthCamNet_EthLinkStatusNotify(ipccmd);
			gEthCamDhcpSrvConnIpAddr=0;
		}
#elif (defined(_NVT_ETHREARCAM_TX_))
		if(LinkSta== ETHCAM_LINK_UP){
			EthCam_UdpCheckConn(0);
		}
		ethlinkstatusnofity(LinkSta);
#endif
	}
	#if (defined(_NVT_ETHREARCAM_RX_))
	if(LinkSta==ETHCAM_LINK_UP &&
		#if (ETH_REARCAM_CAPS_COUNT==1)
		gEthCamDhcpSrvConnIpAddr==0
		#else
		(ipstr2int(SocketInfo[0].ip)==0 ||ipstr2int(SocketInfo[1].ip)==0)
		#endif
	){
		is_get_client_ip=EthCamNet_DhcpLeaseCheck();
		if(is_get_client_ip==0){
			char udp_cmd[50]={0};
			int udp_cmdsize=sizeof(udp_cmd);
			udp_cmdsize=snprintf(udp_cmd, sizeof(udp_cmd) - 1, "cliconncheck");
			//DBG_DUMP("udp cmd=%s, sz=%d\r\n",udp_cmd, udp_cmdsize);
			#if (ETH_REARCAM_CAPS_COUNT==1)
				ethsocket_udp_sendto(NET_LEASE_START_ETH0, UDP_PORT, udp_cmd, &udp_cmdsize);
			#else
			UINT16 i;
			for(i=0;i<ETH_REARCAM_CAPS_COUNT;i++){
				//DBG_DUMP("Hub_LinkStatus[%d]=%d\r\n",i,EthCamEthHub_LinkStatus[i]);
				if(EthCamEthHub_LinkStatus[i] && (g_bEthCamEthLinkStatus[i] !=  (EthCamEthHub_LinkStatus[i]+1))){
					if(strcmp(SocketInfo[0].ip,NET_LEASE_START_ETH0)!=0 && strcmp(SocketInfo[1].ip,NET_LEASE_START_ETH0)!=0){

						ethsocket_udp_sendto(NET_LEASE_START_ETH0, UDP_PORT, udp_cmd, &udp_cmdsize);
					}
					if(strcmp(SocketInfo[0].ip,NET_LEASE_END_ETH0)!=0 && strcmp(SocketInfo[1].ip,NET_LEASE_END_ETH0)!=0){

						ethsocket_udp_sendto(NET_LEASE_END_ETH0, UDP_PORT, udp_cmd, &udp_cmdsize);
					}
				}
			}
			#endif
		}else{
			//snprintf(ipccmd, sizeof(ipccmd) - 1, "ethlinknotify %d %d",201369792, LinkSta);
			snprintf(ipccmd, sizeof(ipccmd) - 1, "ethlinknotify %d %d",gEthCamDhcpSrvConnIpAddr, LinkSta);
			EthCamNet_EthLinkStatusNotify(ipccmd);
		}
	}
	#endif
}
ER eth_get_mac_address(UINT32 *pConfigContext)
{
	int h_eth0;
	char eth0_info[12] = {0};

	if (pConfigContext == NULL) {
		DBG_ERR("input mac addr buffer is NULL\r\n");
		return E_SYS;
	}
	if ((h_eth0 = open("/sys/class/net/eth0/address", O_RDONLY)) < 0) {
		DBG_ERR("get eth0 mac fail\r\n");
	} else {
		read(h_eth0, eth0_info, sizeof(eth0_info));
		printf("eth0_mac=%x%x:%x%x:%x%x:%x%x:%x%x:%x%x\r\n",eth0_info[0],eth0_info[1],eth0_info[2],eth0_info[3],eth0_info[4],eth0_info[5],eth0_info[6],eth0_info[7],eth0_info[8],eth0_info[9],eth0_info[10],eth0_info[11]);
		close(h_eth0);
	}

	memcpy((void *)pConfigContext, eth0_info, sizeof(eth0_info));
	return E_OK;

}
void EthCam_UdpCheckConn(UINT32 bDhcpIP)
{
	//send static ip info to server
	udpsocket_open();

	char udp_cmd[50]={0};
	int udp_cmdsize=50;
	unsigned int macaddr[2]={0};
	char mac_addr[6]={0x56,0x78,0x87,0x65,0x43,0x21};
	//eth_get_mac_address("eth0", mac_addr);
	//eth_getConfig(ETH_CONFIG_ID_MAC_ADDR, (UINT32 *)mac_addr);
	eth_get_mac_address((UINT32 *)mac_addr);

	macaddr[0]=(mac_addr[0] & 0xFF)+ ((mac_addr[1] & 0xFF)<<8 )+ ((mac_addr[2] & 0xFF)<<16)+ ((mac_addr[3] & 0xFF)<<24);
	macaddr[1]=(mac_addr[4] & 0xFF)+ ((mac_addr[5]  & 0xFF)<<8 );


	gEthCamDhcpCliIP=ipstr2int("192.168.0.12");

	memset(udp_cmd, 0 ,sizeof(udp_cmd));
	if(0){//bDhcpIP){
		udp_cmdsize=snprintf(udp_cmd, sizeof(udp_cmd) - 1, "cliconnipnotify dhcpsrv %d %d %d",gEthCamDhcpCliIP,macaddr[0],macaddr[1]);
	}else{
		udp_cmdsize=snprintf(udp_cmd, sizeof(udp_cmd) - 1, "cliconnipnotify %d %d %d",gEthCamDhcpCliIP,macaddr[0],macaddr[1]);
	}
	printf("udp cmd=%s, sz=%d\r\n",udp_cmd, udp_cmdsize);
	strcpy(g_udp_cmd, udp_cmd);
	g_udp_cmdsize=udp_cmdsize;
	ethsocket_udp_sendto(NET_SRVIP_ETH0, UDP_PORT, udp_cmd, &udp_cmdsize);
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
UINT32 EthCamNet_IsSocketOpenBKG(void)
{
#if (defined(_NVT_ETHREARCAM_TX_))
	return g_IsSocketOpenBKGTsk;
#else
	return 0;
#endif
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
#if (defined(_NVT_ETHREARCAM_RX_))
	UINT32 AllPathLinkStatus=0;
#endif
	ETHCAM_SOCKET_INFO EthSocketInfo[ETHCAM_PATH_ID_MAX];
	sscanf_s(cmd, "%s %d %d", cmdname, 40, &TxIPAddr, &status);
	snprintf(chTxIPAddr, ETHCAM_SOCKETCLI_IP_MAX_LEN, "%d.%d.%d.%d", (TxIPAddr & 0xFF), (TxIPAddr >> 8) & 0xFF, (TxIPAddr >> 16) & 0xFF, (TxIPAddr >> 24) & 0xFF);

	DBG_DUMP("Get = %s ,status=%d, TxIPAddr=0x%x, %s\r\n", cmdname, status,TxIPAddr,chTxIPAddr);

	if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_UPDFW && System_GetState(SYS_STATE_CURRSUBMODE) == SYS_SUBMODE_UPDFW) {
		//DBG_WRN("fw update return\r\n");
		SxTimer_SetFuncActive(SX_TIMER_ETHCAM_ETHERNETLINKDET_LINKDET_ID, FALSE);
		return;
	}


#if (defined(_NVT_ETHREARCAM_RX_) && ETH_REARCAM_CAPS_COUNT>=2)
	if(SxTimer_GetFuncActive(SX_TIMER_ETHCAM_ETHHUB_LINKDET_ID)==0){
		//DBG_DUMP("Active ETHHUB Timer\r\n");
		SxTimer_SetFuncActive(SX_TIMER_ETHCAM_ETHHUB_LINKDET_ID, TRUE);
	}
#endif

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

	#if (defined(_NVT_ETHREARCAM_TX_))
		SysSetFlag(FL_ETHCAM_TX_IP_ADDR,TxIPAddr);
		memcpy(&EthSocketInfo[0], &SocketInfo[path_id_fr_eth],sizeof(ETHCAM_SOCKET_INFO));
		path_id=0; //path_id always 0 for Tx
	#else
		memcpy(&EthSocketInfo[0], &SocketInfo[0],sizeof(ETHCAM_SOCKET_INFO)*ETH_REARCAM_CAPS_COUNT);
		path_id=path_id_fr_eth;
	#endif

	g_bEthCamPrevEthLinkStatus[path_id]=g_bEthCamEthLinkStatus[path_id];
	if(status==ETHCAM_LINK_UP){
		DBG_DUMP("link up\r\n");
		g_bEthCamEthLinkStatus[path_id]=ETHCAM_LINK_UP;
		EthCamSocket_SetEthLinkStatus(path_id, ETHCAM_LINK_UP);
		#if (defined(_NVT_ETHREARCAM_TX_))
		EthCamSocket_SetInfo(&EthSocketInfo[0]);
		EthCamSocket_SetEthHubLinkSta(0, 1);
		#if 0
		UINT16 WaitCnt=0;
		while((BKG_GetExecFuncTable() && BKG_GetExecFuncTable()->event==0) && WaitCnt++<20){
		    Delay_DelayMs(50);
		}
		BKG_PostEvent(NVTEVT_BKW_ETHCAM_SOCKET_OPEN);
		#else
		if((BKG_GetExecFuncTable() && BKG_GetExecFuncTable()->event==0)){
			g_IsSocketOpenBKGTsk=0;
			socketEthCmd_Open();
		}else{
			g_IsSocketOpenBKGTsk=1;
			BKG_PostEvent(NVTEVT_BKW_ETHCAM_SOCKET_OPEN);
		}
		#endif
		EthCamTxHB = 0;
		#else
		#if(ETH_REARCAM_CAPS_COUNT==1)
		EthCamSocket_SetEthHubLinkSta(0, 1);
		#endif
		EthCamSocketCli_SetSvrInfo(EthSocketInfo, ETH_REARCAM_CAPS_COUNT);
		if ((System_IsModeChgClose()==0 && System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_MOVIE) || System_GetState(SYS_STATE_NEXTMODE) == PRIMARY_MODE_MOVIE) {
			EthCamSocketCli_ReConnect(path_id, 0 ,0);
			BKG_PostEvent(NVTEVT_BKW_ETHCAM_SOCKETCLI_CMD_OPEN);

			EthCamHB1[path_id] = 0;
			EthCamHB2 = 0;
			SxTimer_SetFuncActive(SX_TIMER_ETHCAM_DATARECVDET_ID, TRUE);
		}else{
			DBG_ERR("NOT movie mode!!\r\n");
		}
		#endif
	}else if(status==ETHCAM_LINK_DOWN){
		DBG_DUMP("link down\r\n");
		g_bEthCamEthLinkStatus[path_id]=ETHCAM_LINK_DOWN;
		EthCamSocket_SetEthLinkStatus(path_id, ETHCAM_LINK_DOWN);

#if (defined(_NVT_ETHREARCAM_TX_))
		EthCamTxHB = 0;
		SxTimer_SetFuncActive(SX_TIMER_ETHCAM_DATARECVDET_ID, FALSE);

		//stop TX Stream
		if(ImageApp_MovieMulti_IsStreamRunning(_CFG_REC_ID_1 | ETHCAM_TX_MAGIC_KEY)){
			MovieExe_EthCamTxStop(_CFG_REC_ID_1);
		}
		if(ImageApp_MovieMulti_IsStreamRunning(_CFG_CLONE_ID_1 | ETHCAM_TX_MAGIC_KEY)){
			MovieExe_EthCamTxStop(_CFG_CLONE_ID_1);
		}
		MovieExe_EthCamRecId1_ResetBsQ();

		//EthCamSocket_Close();
		socketEth_Close();
#else
		//clear ip, for Tx unplug and power off
		if(strcmp(cmdname,"EthHubLink")==0){
			//DBG_WRN("clear IP addr\r\n");
			strcpy(SocketInfo[path_id].ip, "0.0.0.0");
			memset(SocketInfo[path_id].mac,0,ETHCAM_SOCKETCLI_MAC_MAX_LEN);
		}
#if (ETH_REARCAM_CAPS_COUNT>=2)
		g_PrevSrvCliConnCliMacAddr[0]=0;
		g_PrevSrvCliConnCliMacAddr[1]=0;
		g_SrvCliConnCliMacAddrCnt=0;
#endif
		EthCamNet_EthHubChkPortReadyTimerClose();
		EthCamNet_EthHubPortReadySendCmdTimerClose();
		if (System_GetState(SYS_STATE_CURRSUBMODE) == SYS_SUBMODE_UPDFW) {
			SxTimer_SetFuncActive(SX_TIMER_ETHCAM_ETHERNETLINKDET_LINKDET_ID, FALSE);
			#if 0

			#if defined(_UI_STYLE_LVGL_)
			UIFlowWaitMomentAPI_Set_Text_Param param = (UIFlowWaitMomentAPI_Set_Text_Param){.string_id = LV_PLUGIN_STRING_ID_STRID_ETHCAM_UDFW_FINISH};
			lv_user_task_handler_lock();
			lv_plugin_scr_open(UIFlowWaitMoment, &param);
			lv_user_task_handler_unlock();
			#else
			Ux_OpenWindow(&UIFlowWndWaitMomentCtrl, 1, UIFlowWndWaitMoment_StatusTXT_Msg_STRID_ETHCAM_UDFW_FINISH);
			#endif

			Delay_DelayMs(1000);

			#if defined(_UI_STYLE_LVGL_)
			lv_user_task_handler_lock();
			lv_plugin_scr_close(UIFlowWaitMoment, NULL);
			lv_user_task_handler_unlock();
			#else
			Ux_CloseWindow(&UIFlowWndWaitMomentCtrl, 0);
			#endif

			System_ChangeSubMode(SYS_SUBMODE_NORMAL);
			Ux_SendEvent(0, NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_MOVIE);
			#endif
		}
		for(i=0;i<ETHCAM_PATH_ID_MAX;i++){
			if(g_bEthCamEthLinkStatus[i]==ETHCAM_LINK_UP){
				AllPathLinkStatus++;
			}
		}
		if(AllPathLinkStatus==0){
			EthCamCmd_GetFrameTimerEn(0);
			#if (ETH_REARCAM_CAPS_COUNT>=2)
			g_isChkPortReady=0;
			g_ChkPortReadyIPAddr=0;
			#endif
		}
		//for unplug and ethcam cmd not finish
		EthCamCmd_Done(path_id, 0xFFFFFFFF, ETHCAM_CMD_TERMINATE);

		socketCliEthData1_SetRecv(path_id, 0);
		socketCliEthData2_SetRecv(path_id, 0);
		if (System_GetState(SYS_STATE_CURRMODE) == PRIMARY_MODE_MOVIE) {
			if(ImageApp_MovieMulti_IsStreamRunning(_CFG_ETHCAM_ID_1+ path_id)){
			#if (ETHCAM_EIS ==ENABLE)
				if (SysGetFlag(FL_MOVIE_EIS) == EIS_ON) {
					ImageApp_MovieMulti_TranscodeStop(_CFG_ETHCAM_ID_1 +path_id);
				}else{
					ImageApp_MovieMulti_RecStop(_CFG_ETHCAM_ID_1 + path_id);
				}
			#else
				ImageApp_MovieMulti_RecStop(_CFG_ETHCAM_ID_1+ path_id);
			#endif
			}
			ImageApp_MovieMulti_EthCamLinkForDisp((_CFG_ETHCAM_ID_1 + path_id), DISABLE, TRUE);
			MovieExe_EthCam_ChgDispCB(UI_GetData(FL_DUAL_CAM));
		}
		EthCamSocketCli_ReConnect(path_id, 0, 0);
		EthCamSocketCli_Close(path_id, ETHSOCKIPCCLI_ID_0);
		EthCamSocketCli_Close(path_id, ETHSOCKIPCCLI_ID_1);
		EthCamSocketCli_Close(path_id, ETHSOCKIPCCLI_ID_2);

		if(AllPathLinkStatus==0){
			//SxTimer_SetFuncActive(SX_TIMER_ETHCAM_ETHERNETLINKDET_LINKDET_ID, FALSE);
			SxTimer_SetFuncActive(SX_TIMER_ETHCAM_DATARECVDET_ID, FALSE);
			EthCamHB1[path_id] = 0;
			EthCamHB2 = 0;
			#if (ETH_REARCAM_CAPS_COUNT>=2)
			EthCamNet_EthHubPortIsolate(0, 0);
			EthCamNet_EthHubPortIsolate(1, 1);
			EthCamNet_DetConnInit();
			#endif
		}
		if (System_GetState(SYS_STATE_CURRSUBMODE) == SYS_SUBMODE_UPDFW) {

			#if defined(_UI_STYLE_LVGL_)
			UIFlowWaitMomentAPI_Set_Text_Param param = (UIFlowWaitMomentAPI_Set_Text_Param){.string_id = LV_PLUGIN_STRING_ID_STRID_ETHCAM_UDFW_FINISH};
			lv_user_task_handler_lock();
			lv_plugin_scr_open(UIFlowWaitMoment, &param);
			lv_user_task_handler_unlock();
			#else
			Ux_OpenWindow(&UIFlowWndWaitMomentCtrl, 1, UIFlowWndWaitMoment_StatusTXT_Msg_STRID_ETHCAM_UDFW_FINISH);
			#endif

			Delay_DelayMs(1000);

			#if defined(_UI_STYLE_LVGL_)
			lv_user_task_handler_lock();
			lv_plugin_scr_close(UIFlowWaitMoment, NULL);
			lv_user_task_handler_unlock();
			#else
			Ux_CloseWindow(&UIFlowWndWaitMomentCtrl, 0);
			#endif


			System_ChangeSubMode(SYS_SUBMODE_NORMAL);
			Ux_SendEvent(0, NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_MOVIE);
			SxTimer_SetFuncActive(SX_TIMER_ETHCAM_ETHERNETLINKDET_LINKDET_ID, TRUE);
		}
		g_bEthCamPrevEthLinkStatus[path_id]=ETHCAM_LINK_DOWN;
#endif
	}
}
#if (defined(_NVT_ETHREARCAM_TX_))
void EthCamNet_SetTxHB(UINT32 value)
{
	EthCamTxHB=value;
}
UINT32 EthCamNet_GetTxHB(void)
{
	return EthCamTxHB;
}
#endif
void EthCamNet_LinkDetStreamRestart(UINT32 pathid)
{
	CHAR cmd[40]={0};
	snprintf(cmd, 40, "EthRestart %d %d", ipstr2int(SocketInfo[pathid].ip),ETHCAM_LINK_DOWN);
	EthCamNet_EthLinkStatusNotify(cmd);
	snprintf(cmd, 40, "EthRestart %d %d", ipstr2int(SocketInfo[pathid].ip),ETHCAM_LINK_UP);
	EthCamNet_EthLinkStatusNotify(cmd);
}
void EthCamNet_DataRecvDet(void)
{
#if (defined(_NVT_ETHREARCAM_RX_))
	UINT32 j;
	//CHAR cmd[40]={0};
	UINT32 HBReset=0;

	for (j=0; j<ETH_REARCAM_CAPS_COUNT; j++){
#if (ETH_REARCAM_CLONE_FOR_DISPLAY == DISABLE)
		//DBG_DUMP("EthCamHB1=%d\r\n", EthCamHB1);
		EthCamHB1[j] ++;
		if (EthCamHB1[j] && !(EthCamHB1[j] % 5)) {
#else
		//DBG_DUMP("EthCamHB1/2=%d/%d\r\n", EthCamHB1[j], EthCamHB2);
#if (ETH_REARCAM_CAPS_COUNT>=2)
		EthCamHB1[j] ++;
#endif
		EthCamHB2 ++;
		if ((EthCamHB1[j] && !(EthCamHB1[j] % 5)) || (EthCamHB2 && !(EthCamHB2 % 5))) {
#endif
			if(g_bEthCamEthLinkStatus[j]==ETHCAM_LINK_UP){
				DBG_WRN("EthCamHBActive HB1[%d]=%d, HB2=%d\r\n", j,EthCamHB1[j],EthCamHB2);
				EthCamNet_LinkDetStreamRestart(j);
				//EthCamNet_EthLinkStatusNotify("EthRestart 201369792 1");
				//EthCamNet_EthLinkStatusNotify("EthRestart 201369792 2");
			}
			//if (kchk_flg(ETHCAM_CMD_SND_FLG_ID, FLG_ETHCAM_CMD_SND) == FLG_ETHCAM_CMD_SND){
			//}
		}
#if (ETH_REARCAM_CLONE_FOR_DISPLAY == DISABLE)
		if (EthCamHB1[j] > 15) {
#else
		if (EthCamHB1[j] > 15 || EthCamHB2 > 15) {
#endif
			//SxTimer_SetFuncActive(SX_TIMER_ETHCAM_DATARECVDET_ID, FALSE);
			EthCamHB1[j] = 0;
			EthCamHB2 = 0;
		}
	}
	for(j=0;j<ETH_REARCAM_CAPS_COUNT;j++){
		if(EthCamHB1[j]==0){
			HBReset++;
		}
	}
#if (ETH_REARCAM_CLONE_FOR_DISPLAY == DISABLE)
	if(HBReset==ETH_REARCAM_CAPS_COUNT){
#else
	if(HBReset==ETH_REARCAM_CAPS_COUNT || EthCamHB2==0){
#endif
		SxTimer_SetFuncActive(SX_TIMER_ETHCAM_DATARECVDET_ID, FALSE);
	}
#endif
//#if (defined(_NVT_ETHREARCAM_TX_) && ETH_REARCAM_CAPS_COUNT>=2)
#if (defined(_NVT_ETHREARCAM_TX_))
	if(sEthCamSysInfo.PullModeEn){
		EthCamTxHB ++;
		if(EthCamTxHB >=6){// 3 sec
			//hCamNet_EthLinkStatusNotify("EthLinkDown 0 1");
			if(ImageApp_MovieMulti_IsStreamRunning(_CFG_REC_ID_1 | ETHCAM_TX_MAGIC_KEY)){
				DBG_WRN("NoHB Stop Stream\r\n");

				MovieExe_EthCamTxStop(_CFG_REC_ID_1);
				EthCamTxHB=0;
			}
		}
	}
	EthCamNet_CheckVDCount();
#endif
}
#if (defined(_NVT_ETHREARCAM_RX_))
#if (ETH_REARCAM_CAPS_COUNT>=2)
static void EthCamNet_EthHubPortReadySendCmdTimer_TimeOutCB(UINT32 uiEvent)
{
	//DBG_DUMP("ReadySendCmdTimer_TimeOut\r\n");
	if(g_isEthHubPortReadyCmdAck==0){
		BKG_PostEvent(NVTEVT_BKW_ETHCAM_CHECKPORT_READY);
	}
}
static void EthCamNet_EthHubChkPortReadyTimer_TimeOutCB(UINT32 uiEvent)
{
	//DBG_DUMP("ReadyTimer_TimeOut\r\n");
	g_EthHubChkPortReadyTimerRunning=0;
}
#endif
void EthCamNet_EthHubPortReadySendCmdTimerOpen(void)
{
#if (ETH_REARCAM_CAPS_COUNT>=2)
	SWTIMER_CB EventHandler;
	if(g_isEthHubPortReadySendCmdTimerOpen){
		DBG_WRN("timer already open\r\n");
		return;
	}
	EventHandler=(SWTIMER_CB)EthCamNet_EthHubPortReadySendCmdTimer_TimeOutCB;
	if (SwTimer_Open(&g_EthHubPortReadySendCmdTimeID, EventHandler) != E_OK) {
		DBG_ERR("open timer fail\r\n");
	} else {
		SwTimer_Cfg(g_EthHubPortReadySendCmdTimeID, 200 /*ms*/, SWTIMER_MODE_ONE_SHOT);
		SwTimer_Start(g_EthHubPortReadySendCmdTimeID);
		g_isEthHubPortReadySendCmdTimerOpen=1;
	}
	//DBG_DUMP("g_EthHubPortReadySendCmdTimeID=%d\r\n",g_EthHubPortReadySendCmdTimeID);
#endif
}
void EthCamNet_EthHubPortReadySendCmdTimerClose(void)
{
#if (ETH_REARCAM_CAPS_COUNT>=2)
	if(g_isEthHubPortReadySendCmdTimerOpen==0){
		return;
	}
	if(g_EthHubPortReadySendCmdTimeID !=SWTIMER_NUM){
		//DBG_DUMP("Port Ready close timer\r\n");

		SwTimer_Stop(g_EthHubPortReadySendCmdTimeID);
		SwTimer_Close(g_EthHubPortReadySendCmdTimeID);
	}
	g_EthHubPortReadySendCmdTimeID=SWTIMER_NUM;
	g_isEthHubPortReadySendCmdTimerOpen=0;
#endif
}
void EthCamNet_EthHubChkPortReadyTimerOpen(void)
{
#if (ETH_REARCAM_CAPS_COUNT>=2)
	SWTIMER_CB EventHandler;
	EventHandler=(SWTIMER_CB)EthCamNet_EthHubChkPortReadyTimer_TimeOutCB;
	if (SwTimer_Open(&g_EthHubChkPortReadyTimeID, EventHandler) != E_OK) {
		DBG_ERR("open timer fail\r\n");
	} else {
		SwTimer_Cfg(g_EthHubChkPortReadyTimeID, 300 /*ms*/, SWTIMER_MODE_ONE_SHOT);
		SwTimer_Start(g_EthHubChkPortReadyTimeID);
		g_EthHubChkPortReadyTimerRunning=1;
	}
	//DBG_DUMP("g_EthHubChkPortReadyTimeID=%d\r\n",g_EthHubChkPortReadyTimeID);
#endif
}
void EthCamNet_EthHubChkPortReadyTimerClose(void)
{
#if (ETH_REARCAM_CAPS_COUNT>=2)
	if(g_EthHubChkPortReadyTimeID !=SWTIMER_NUM){
		DBG_DUMP("Port Ready close timer\r\n");

		SwTimer_Stop(g_EthHubChkPortReadyTimeID);
		SwTimer_Close(g_EthHubChkPortReadyTimeID);
	}
	g_EthHubChkPortReadyTimeID=SWTIMER_NUM;
#endif
}
void EthCamNet_EthHubChkPortReady(void)
{
#if (ETH_REARCAM_CAPS_COUNT>=2)
#if (ETHCAM_CHECK_PORTREADY == ENABLE)
	EthCamNet_EthHubLinkDet();
	if(EthCamEthHub_LinkStatus[0] && EthCamEthHub_LinkStatus[1]){
		if(ipstr2int(SocketInfo[0].ip) &&  g_isChkPortReady==0){
		EthCamNet_EthHubPortIsolate(1, 0);
		g_isEthHubPortReadyCmdAck=0;
		g_ChkPortReadyIPAddr=((ipstr2int(SocketInfo[0].ip) == 201369792) ? 218147008: 201369792);
		AppBKW_SetData(BKW_ETHCAM_CHECK_PORT_READY_IP,g_ChkPortReadyIPAddr );
		BKG_PostEvent(NVTEVT_BKW_ETHCAM_CHECKPORT_READY);
		g_isChkPortReady=1;
		}
	}else if(EthCamEthHub_LinkStatus[1] && g_isChkPortReady==0){

		EthCamNet_EthHubPortIsolate(1, 0);
		g_isEthHubPortReadyCmdAck=0;

		AppBKW_SetData(BKW_ETHCAM_CHECK_PORT_READY_IP,0 );
		BKG_PostEvent(NVTEVT_BKW_ETHCAM_CHECKPORT_READY);

		g_isChkPortReady=1;
	}
#endif
#endif
}
void EthCamNet_SrvCliConnIPAddrNofity(char *cmd)
{
	char cmdname[50]={0};
	char cmdname2[50]={0};
	UINT32 CliIPAddr=0;
	UINT32 CliMacAddr[2]={0};
	#if (ETHCAM_CHECK_PORTREADY == ENABLE)
	static char prev_cmdname[50]={0};
	static char prev_cmdname2[50]={0};
	#endif
	CHAR chCliIPAddr[ETHCAM_SOCKETCLI_IP_MAX_LEN]={0};
	CHAR chCliMacAddr[ETHCAM_SOCKETCLI_MAC_MAX_LEN]={0};

	sscanf_s(cmd, "%s %s %d %d %d", cmdname, sizeof(cmdname)-1, cmdname2, sizeof(cmdname2)-1, &CliIPAddr, &CliMacAddr[0], &CliMacAddr[1]);
	snprintf(chCliIPAddr, ETHCAM_SOCKETCLI_IP_MAX_LEN, "%d.%d.%d.%d", (CliIPAddr & 0xFF), (CliIPAddr >> 8) & 0xFF, (CliIPAddr >> 16) & 0xFF, (CliIPAddr >> 24) & 0xFF);
	snprintf(chCliMacAddr, ETHCAM_SOCKETCLI_MAC_MAX_LEN, "%02x:%02x:%02x:%02x:%02x:%02x", (CliMacAddr[0] & 0xFF), (CliMacAddr[0] >> 8) & 0xFF, (CliMacAddr[0] >> 16) & 0xFF, (CliMacAddr[0] >> 24) & 0xFF , (CliMacAddr[1]) & 0xFF ,(CliMacAddr[1] >> 8) & 0xFF);
	DBG_DUMP("Srv Get=%s ,cmd2=%s, CliIPAddr=0x%x, IP=%s, CliMac=%s\r\n", cmdname, cmdname2, CliIPAddr,chCliIPAddr, chCliMacAddr);

	#if (ETH_REARCAM_CAPS_COUNT>=2)
	if((g_PrevSrvCliConnCliMacAddr[0]!= CliMacAddr[0]) || (g_PrevSrvCliConnCliMacAddr[1]!= CliMacAddr[1])){
		g_SrvCliConnCliMacAddrCnt++;
	}
#if (ETHCAM_CHECK_PORTREADY == ENABLE)
	DBG_DUMP("CliConCnt=%d\r\n",g_SrvCliConnCliMacAddrCnt);
	g_PrevSrvCliConnCliMacAddr[0]= CliMacAddr[0];
	g_PrevSrvCliConnCliMacAddr[1]= CliMacAddr[1];

	if((strcmp(cmdname,"cliconnipnotify")==0)
		&& (strcmp(cmdname2,"dhcpsrv")==0)
		&& g_isChkPortReady==0){
		DBG_DUMP("isChkPortReady=0 return\r\n");
		g_isEthHubPortReadyCmdAck=0;
		AppBKW_SetData(BKW_ETHCAM_CHECK_PORT_READY_IP, 0);
		while(BKG_GetExecFuncTable()->event==0){
			Delay_DelayMs(5);
		}
		BKG_PostEvent(NVTEVT_BKW_ETHCAM_CHECKPORT_READY);
		return;
	}
	if((strcmp(cmdname,"cliconnipnotify")==0) && (strcmp(cmdname2,"udp")==0)){
		g_isEthHubPortReadyCmdAck=1;
	}
	strcpy(prev_cmdname, cmdname);
	strcpy(prev_cmdname2, cmdname2);
#endif
	UINT16  i=0;
	for(i=0;i<ETHCAM_PATH_ID_MAX;i++){
		if(strcmp(SocketInfo[i].mac,chCliMacAddr)==0){
			//DBG_ERR("Mac Addr Already Exist!! SocketInfo[%d].mac=%s\r\n",i, SocketInfo[i].mac);
			return;
		}

		if(strcmp(SocketInfo[i].ip,chCliIPAddr)==0){
			//DBG_WRN("IP Already Exist!! SocketInfo[%d].ip=%s\r\n",i, SocketInfo[i].ip);
			if(strcmp(SocketInfo[i].mac,chCliMacAddr)!=0 ){
				DBG_ERR("[%d]IP conflict!! old=%s, new=%s\r\n",i, SocketInfo[i].mac, chCliMacAddr);
				if(socketCliEthCmd_IsConn(ETHCAM_PATH_ID_1+i)){
					EthCam_SendXMLCmd(ETHCAM_PATH_ID_1+i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_SET_TX_IP_RESET, 0);
					if(EthCam_SendXMLCmd(ETHCAM_PATH_ID_1+i, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_TX_POWEROFF, 0)==ETHCAM_RET_ERR){
						g_isEthCamIPConflict=1;
					}
				}else{
					g_isEthCamIPConflict=1;
				}
			}
			return;
		}
	}

	EthCamNet_EthHubLinkDet();
	for(i=0;i<ETHCAM_PATH_ID_MAX;i++){
		//DBG_DUMP("EthHub_LinkStatus[%d]=%d, EthLinkStatus[%d]=%d\r\n", i,EthCamEthHub_LinkStatus[i],i,g_bEthCamEthLinkStatus[i]);
		//AN is OK, but not linkup
		if(EthCamEthHub_LinkStatus[i]==0 && EthCamEthHub_ANStatus[i]){
			EthCamNet_EthHubLinkDet();
		}
#if (ETHCAM_CHECK_PORTREADY == ENABLE)
		if(EthCamEthHub_LinkStatus[0] && EthCamEthHub_LinkStatus[1]){
			if(!(ipstr2int(SocketInfo[0].ip)==0 && ipstr2int(SocketInfo[1].ip)==0)){
				EthCamNet_EthHubPortIsolate(1, 0);
			}
		}else{
			EthCamNet_EthHubPortIsolate(1, 0);
		}
		EthCamNet_EthHubLinkDet();
#endif
		if(EthCamEthHub_LinkStatus[i] && (g_bEthCamEthLinkStatus[i] !=  (EthCamEthHub_LinkStatus[i]+1))){
			if(ipstr2int(SocketInfo[i].ip)==0){
				#if (ETHCAM_CHECK_PORTREADY == ENABLE)
				DBG_DUMP("SrvConn %d,%d,%d\r\n",EthCamEthHub_LinkStatus[0] ,EthCamEthHub_LinkStatus[1],g_isChkPortReady);
				if(EthCamEthHub_LinkStatus[0] && EthCamEthHub_LinkStatus[1] ){
					DBG_DUMP("EthHub ALL UP\r\n");
					if(ipstr2int(SocketInfo[0].ip)==0 && ipstr2int(SocketInfo[1].ip)==0 && g_isChkPortReady==0){
						EthCamNet_EthHubPortIsolate(1, 0);
						g_isEthHubPortReadyCmdAck=0;
						g_ChkPortReadyIPAddr=((ipstr2int(chCliIPAddr) == 201369792) ? 218147008: 201369792);
						AppBKW_SetData(BKW_ETHCAM_CHECK_PORT_READY_IP,g_ChkPortReadyIPAddr );
						while(BKG_GetExecFuncTable()->event==0){
							Delay_DelayMs(5);
						}
						BKG_PostEvent(NVTEVT_BKW_ETHCAM_CHECKPORT_READY);
						g_isChkPortReady=1;
					}
				}else if(EthCamEthHub_LinkStatus[1] && g_isChkPortReady==0){

					g_isEthHubPortReadyCmdAck=0;
					g_ChkPortReadyIPAddr=((ipstr2int(chCliIPAddr) == 201369792) ? 218147008: 201369792);
					AppBKW_SetData(BKW_ETHCAM_CHECK_PORT_READY_IP,g_ChkPortReadyIPAddr );
					while(BKG_GetExecFuncTable()->event==0){
						Delay_DelayMs(5);
					}
					BKG_PostEvent(NVTEVT_BKW_ETHCAM_CHECKPORT_READY);
					g_isChkPortReady=1;
				}else{
					g_isChkPortReady=1;
				}
				#endif
				strcpy(SocketInfo[i].ip, chCliIPAddr);
				strcpy(SocketInfo[i].mac, chCliMacAddr);
				DBG_DUMP("Srv SocketInfo[%d].ip=%s\r\n",i,SocketInfo[i].ip);
				snprintf(cmdname, sizeof(cmdname) - 1, "SrvCliConnIPLink %d %d",ipstr2int(SocketInfo[i].ip), (EthCamEthHub_LinkStatus[i] == 1) ? ETHCAM_LINK_UP : ETHCAM_LINK_DOWN);
				EthCamNet_EthLinkStatusNotify(cmdname);
				break;
			}
		}
#if 0
		else{
			if(strcmp(SocketInfo[i].ip, chCliIPAddr)==0){
				DBG_ERR("IP Exist !!SocketInfo[%d].ip=%s\r\n",i,SocketInfo[i].ip);
				break;
			}
		}
#endif
	}
	#else
	strcpy(SocketInfo[0].ip, chCliIPAddr);
	strcpy(SocketInfo[0].mac, chCliMacAddr);
	DBG_DUMP("SocketInfo[0].ip=%s\r\n",SocketInfo[0].ip);
	snprintf(cmdname, sizeof(cmdname) - 1, "SrvCliConnIPLink %d %d",ipstr2int(SocketInfo[0].ip), ETHCAM_LINK_UP);
	EthCamNet_EthLinkStatusNotify(cmdname);
	#endif
}
#if (ETH_REARCAM_CAPS_COUNT>=2)

UINT32 EthCamNet_GetEthHub_LinkStatus(ETHCAM_PATH_ID path_id)
{
	return EthCamEthHub_LinkStatus[path_id];
}

void EthCamNet_EthHubLinkDetInit(void)
{
	smiInit(MDC_GPIO, MDIO_GPIO);
}
void EthCamNet_DetConnInit(void)
{
	g_bLastTxDet     = FALSE;
	g_bLastTxStatus  = FALSE;
}
static BOOL EthCamNet_DetConn(BOOL ConnSta)
{
	BOOL        bConn=0;
#if (ETHCAM_CHECK_PORTREADY == ENABLE)
	BOOL        bCurTxDet, bCurTxStatus;
	bCurTxDet = ConnSta;
	//DBG_DUMP("^BTx DetPlug > lastDet = %d, currDet = %d\r\n", g_bLastTxDet, bCurTxDet);
	bCurTxStatus = (BOOL)(bCurTxDet & g_bLastTxDet);
	//DBG_DUMP("^BTx DetPlug > last = %d, curr = %d\r\n", g_bLastTxStatus, bCurTxStatus);
	if (bCurTxStatus != g_bLastTxStatus) {
		if (bCurTxStatus == TRUE) {
			bConn= 1;
		}else {
			//DBG_DUMP("^BTx Unplug\r\n");
			bConn= 0;
		}
	}else if ((g_bLastTxDet == TRUE) && (bCurTxDet == FALSE)) {
		//DBG_DUMP("^BTx Fast Unplug 2\r\n");
		bConn= 0;
		bCurTxDet = FALSE;
		bCurTxStatus = (BOOL)(bCurTxDet & g_bLastTxDet);
	}
	g_bLastTxDet     = bCurTxDet;
	g_bLastTxStatus  = bCurTxStatus;
#endif
	return bConn;
}
extern UINT32 g_TestEthHubForceLink[2];
void EthCamNet_EthHubLinkDet(void)
{
	UINT32  read_data[ETHCAM_PATH_ID_MAX]={0};
	UINT16  i, j;
	char cmd[40];

	for(i=0;i<ETHCAM_PATH_ID_MAX;i++){
		smiRead(ETHCAM_ETHHUB_PHY_PORT[i], ETHHUB_LINK_STATUS_REG, &read_data[i]);
		//read_data[i]=0x782d;
		EthCamEthHub_LinkStatus[i]= (read_data[i]==0xffff) ?  0: ((read_data[i] & (1<< ETHHUB_LINK_STATUS_REG_BIT)) >> ETHHUB_LINK_STATUS_REG_BIT);
		if(g_TestEthHubForceLink[i]==1 || g_TestEthHubForceLink[i]==0){
			//DBG_WRN("ForceLink[%d]=%d\r\n",i,g_TestEthHubForceLink[i]);
			EthCamEthHub_LinkStatus[i]=g_TestEthHubForceLink[i];
		}
		EthCamSocket_SetEthHubLinkSta(i, EthCamEthHub_LinkStatus[i]);
		EthCamEthHub_ANStatus[i]= (read_data[i]==0xffff) ?  0: ((read_data[i] & (1<< ETHHUB_LINK_AN_COMPLETE_BIT)) >> ETHHUB_LINK_AN_COMPLETE_BIT);
		//DBG_DUMP("port[%d]=0x%x, sta=%d, pre_sta=%d, EthLinkStatus=%d, AN=%d\r\n",ETHCAM_ETHHUB_PHY_PORT[i],read_data[i],EthCamEthHub_LinkStatus[i], EthCamEthHub_PrevLinkStatus[i], g_bEthCamEthLinkStatus[i],EthCamEthHub_ANStatus[i]);
		if(EthCamEthHub_PrevLinkStatus[i] !=EthCamEthHub_LinkStatus[i]){
#if 1
			for (j=0; j<ETH_REARCAM_CAPS_COUNT; j++){
				if(ImageApp_MovieMulti_IsStreamRunning(_CFG_ETHCAM_ID_1+j)){
					DBG_ERR("STOPREC!!!\r\n");
					BKG_PostEvent(NVTEVT_BKW_STOPREC_PROCESS);
					break;
				}
			}
			//DBG_DUMP("HubLinkDet, EthCamEthHub_LinkStatus=%d, %d, LinkStatus=%d, %d ,%d, %d\r\n",EthCamEthHub_LinkStatus[0],EthCamEthHub_LinkStatus[1], EthCamNet_GetEthLinkStatus(0),EthCamNet_GetEthLinkStatus(1), socketCliEthData1_IsRecv(0), socketCliEthData1_IsRecv(1));

			//stop stream for exist path for hot plugin
			if(EthCamEthHub_LinkStatus[i]){
				for (j=0; j<ETH_REARCAM_CAPS_COUNT; j++){
					if(EthCamNet_GetEthLinkStatus(j)==ETHCAM_LINK_UP){
						if(EthCamNet_GetPrevEthLinkStatus(j)==ETHCAM_LINK_UP && socketCliEthData1_IsRecv(j)){//start stream is OK
							DBG_WRN("stop path id=%d\r\n",j);
							EthCamCmd_GetFrameTimerEn(0);
							EthCam_SendXMLCmd(j, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_TX_STREAM_STOP, 0);
							socketCliEthData1_SetRecv(j, 0);
						}else{ //connecting, stream not start
							DBG_WRN("path id[%d] is connecting\r\n",j);
						}
					}
				}
			}


#endif
			if(g_bEthCamEthLinkStatus[i] !=  (EthCamEthHub_LinkStatus[i]+1)){
				//DBG_ERR("SocketInfo[%d].ip=0x%x\r\n",i,ipstr2int(SocketInfo[i].ip));
				if(ipstr2int(SocketInfo[i].ip)){
					if(EthCamEthHub_LinkStatus[i]){
						SxTimer_SetFuncActive(SX_TIMER_ETHCAM_ETHERNETLINKDET_LINKDET_ID, TRUE);
					}
					snprintf(cmd, sizeof(cmd) - 1, "EthHubLink %d %d",ipstr2int(SocketInfo[i].ip), (EthCamEthHub_LinkStatus[i] == 1) ? ETHCAM_LINK_UP : ETHCAM_LINK_DOWN);
					EthCamNet_EthLinkStatusNotify(cmd);
				}else{
					//DBG_ERR("SocketInfo[%d].ip=0x%x, HubSta=%d\r\n",i,ipstr2int(SocketInfo[i].ip),EthCamEthHub_LinkStatus[i]);
				}
				//EthCamNet_NotifyEthHubConn(i, (EthCamEthHub_LinkStatus[i] == 1) ? ETHCAM_LINK_UP : ETHCAM_LINK_DOWN);
			}
			EthCamEthHub_PrevLinkStatus[i]=EthCamEthHub_LinkStatus[i];
		}
		//DBG_DUMP("port[%d]=0x%x, sta=%d, EthLinkStatus=%d\r\n",ETHCAM_ETHHUB_PHY_PORT[i],read_data[i],EthCamEthHub_LinkStatus[i], g_bEthCamEthLinkStatus[i]);
	}
	//DBG_DUMP("LinkStatus=%d, %d, %d, %d\r\n",EthCamEthHub_LinkStatus[0], EthCamEthHub_LinkStatus[1] ,g_bEthCamPrevEthLinkStatus[1], g_bEthCamEthLinkStatus[1] );
	if(EthCamEthHub_LinkStatus[0]==0 && EthCamEthHub_LinkStatus[1]  && g_bEthCamPrevEthLinkStatus[1]==ETHCAM_LINK_DOWN && (g_bEthCamEthLinkStatus[1] !=  (EthCamEthHub_LinkStatus[1]+1))){
		 if(g_isChkPortReady==0 && EthCamNet_DetConn(EthCamEthHub_LinkStatus[1])){
			DBG_DUMP("singleTx2\r\n");
			EthCamNet_EthHubPortIsolate(1, 0);
			g_isEthHubPortReadyCmdAck=0;
			AppBKW_SetData(BKW_ETHCAM_CHECK_PORT_READY_IP,0);
			BKG_PostEvent(NVTEVT_BKW_ETHCAM_CHECKPORT_READY);
			g_isChkPortReady=1;
		}
	}
}
#endif
#endif
void EthCamNet_IperfTest(void)
{
#if (defined(_NVT_ETHREARCAM_RX_))
	EthCam_SendXMLCmd(ETHCAM_PATH_ID_1, ETHCAM_PORT_DATA1 ,ETHCAM_CMD_TX_STREAM_STOP, 0);
#if (ETH_REARCAM_CLONE_FOR_DISPLAY == ENABLE)
	EthCam_SendXMLCmd(ETHCAM_PATH_ID_1, ETHCAM_PORT_DATA2 ,ETHCAM_CMD_TX_STREAM_STOP, 0);
#endif

	System_ChangeSubMode(SYS_SUBMODE_UPDFW);
	Ux_SendEvent(0, NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_UPDFW);

#if defined(_UI_STYLE_LVGL_)
	lv_user_task_handler_lock();
	lv_plugin_scr_close(UIFlowMovie, NULL);
	lv_user_task_handler_unlock();
#else
	Ux_CloseWindow(&UIFlowWndMovieCtrl, 0);
#endif


#if defined(_UI_STYLE_LVGL_)
	uint32_t string_id = LV_PLUGIN_STRING_ID_STRID_PLEASE_WAIT;
	lv_user_task_handler_lock();
	lv_plugin_scr_open(UIFlowMovie, &string_id);
	lv_user_task_handler_unlock();
#else
	Ux_OpenWindow(&UIFlowWndWaitMomentCtrl, 1, UIFlowWndWaitMoment_StatusTXT_Msg_STRID_PLEASE_WAIT);
#endif

	system("iperf3 -s -D");
	EthCam_SendXMLCmd(ETHCAM_PATH_ID_1, ETHCAM_PORT_DEFAULT ,ETHCAM_CMD_IPERF_TEST, 0);
	g_bEthCamApp_IperfTest=1;
#endif
}

#endif
//extern void init_eth0(void);
void EthCamNet_SetTxIPAddr(void)
{
#if 0//(defined(_NVT_ETHREARCAM_TX_) && (ETH_REARCAM_CAPS_COUNT>=2))

	DBG_DUMP("FL_ETHCAM_TX_IP_ADDR=0x%x, %d.%d.%d.%d\r\n",SysGetFlag(FL_ETHCAM_TX_IP_ADDR), (SysGetFlag(FL_ETHCAM_TX_IP_ADDR) & 0xFF), (SysGetFlag(FL_ETHCAM_TX_IP_ADDR) >> 8) & 0xFF, (SysGetFlag(FL_ETHCAM_TX_IP_ADDR) >> 16) & 0xFF, (SysGetFlag(FL_ETHCAM_TX_IP_ADDR) >> 24) & 0xFF);
	static BOOL bSendIpAddrSuccess=0;

	if(bSendIpAddrSuccess==0){
		bSendIpAddrSuccess=1;
		gEthCamDhcpCliIP=SysGetFlag(FL_ETHCAM_TX_IP_ADDR);
		init_eth0();
	}
#endif
}

void EthCamNet_SetSysWdtReset(UINT32 uMS)
{
#if 0//(defined(_NVT_ETHREARCAM_TX_))
	UINT32 uiUserData =  MAKEFOURCC('E','C','T','X');

	wdt_open();
	wdt_setConfig(WDT_CONFIG_ID_USERDATA,uiUserData);
	DBG_DUMP("SET USERDATA=0x%x\r\n",uiUserData);

	wdt_setConfig(WDT_CONFIG_ID_MODE,WDT_MODE_RESET);
	wdt_setConfig(WDT_CONFIG_ID_TIMEOUT,uMS);	//ms
	wdt_trigger();
	wdt_enable();
	DBG_ERR("[WDT] EthCamNet SysWdtReset when after %d ms\r\n",uMS);
#endif
}

void EthCamNet_CheckVDCount(void)
{
#if 0//(defined(_NVT_ETHREARCAM_TX_))
	return;
	IPL_CUR_FRAMECNT_INFO frm_info;

	frm_info.id = IPL_ID_1;
	frm_info.pnext = NULL;
	IPL_GetCmd(IPL_GET_IPL_CUR_FRAMECNT_INFOR, (void *)&frm_info);
	//DBG_DUMP("frame_cnt=%d\r\n",frm_info.frame_cnt);
	//DBG_DUMP("dirbuf_err_cnt=%d\r\n",frm_info.dirbuf_err_cnt);

	g_CheckVDCnt=frm_info.frame_cnt;

	if(g_CheckVDCnt == g_PrevCheckAECnt) {
		g_WDTCnt++;
	}
	else{
		g_PrevCheckAECnt = g_CheckVDCnt;
		g_WDTCnt=0;
	}
	if (g_WDTCnt >3)
	{
		g_WDTCnt= 0;
		if(g_isWDTReset == FALSE)
		{
			EthCamNet_SetSysWdtReset(500);
			g_isWDTReset = TRUE;
		}
	}
#endif
}

void EthCamNet_PortReadyCheck(char* cmd)
{
	#if (ETH_REARCAM_CAPS_COUNT >=2)
	printf("EthCam_PortReadyCheck\r\n");
	char cmdname[64];
	UINT32 ip_addr;
	sscanf_s(cmd, "%s %d", cmdname, sizeof(cmdname), &ip_addr);
	printf("cmdname=%s, ip_addr=%d\r\n",cmdname, ip_addr);

	char udp_cmd[50]={0};
	int udp_cmdsize=50;
	udp_cmdsize=snprintf(udp_cmd, sizeof(udp_cmd) - 1, "cliconncheck");
	//printf("udp cmd=%s, sz=%d\r\n",udp_cmd, udp_cmdsize);
	if(ip_addr==0){
		ethsocket_udp_sendto(NET_LEASE_START_ETH0, UDP_PORT, udp_cmd, &udp_cmdsize);
		ethsocket_udp_sendto(NET_LEASE_END_ETH0, UDP_PORT, udp_cmd, &udp_cmdsize);
	}else{
		char chCliIPAddr[FIX_IP_MAX_LEN]={0};
		snprintf(chCliIPAddr, FIX_IP_MAX_LEN, "%d.%d.%d.%d", (ip_addr & 0xFF), (ip_addr >> 8) & 0xFF, (ip_addr >> 16) & 0xFF, (ip_addr >> 24) & 0xFF);
		//printf("ip=%s\r\n",chCliIPAddr);
		ethsocket_udp_sendto(chCliIPAddr, UDP_PORT, udp_cmd, &udp_cmdsize);
	}
	#endif
}

void EthCamNet_Ethboot(void)
{
	system("udpsvd -vE 0 69 tftpd /mnt/sd/ethcam &");
}

#if (defined(_NVT_ETHREARCAM_TX_))
UINT32 g_ChipId_h=0;
UINT32 g_ChipId_l=0;
int eth_gen_mac_address(char *mac_address)
{
#if(defined(_NVT_ETHREARCAM_TX_) && ETH_REARCAM_CAPS_COUNT>=2)

	int i, j=0, k=0;
	for ( i = 0; i < (6); i++ ){
		if(i<2){
			k++;
			if(i==0){
				mac_address[i]= (g_ChipId_h>>((2-k)*8)) & 0xFE;
			}else{
				mac_address[i]= (g_ChipId_h>>((2-k)*8)) & 0xFF;
			}
		}else{
			j++;
			mac_address[i]=(g_ChipId_l>>((4-j)*8)) & 0xFF;
		}
	}
#endif
	return true;
}
int eth_set_mac_address( const char *interface, char *mac_address )
{
#if 0
	eth_setConfig(ETH_CONFIG_ID_MAC_ADDR, (UINT32)mac_address);//eth driver
	ethernetif_set_mac_address(mac_address);//lwip

	DBG_DUMP( "Mac addr %02x:%02x:%02x:%02x:%02x:%02x\n",
	             mac_address[0],
	             mac_address[1],
	             mac_address[2],
	             mac_address[3],
	             mac_address[4],
	             mac_address[5] );
#endif
	return true;
}
void ethcam_get_chipid(void)
{
#if 0
	UINT64 uid ;
	g_ChipId_h=0, g_ChipId_l=0;

	if((uid =avl_get_uniqueid(void)) > 0){
		DBG_DUMP("unique ID[0x%08x][0x%08x] success\r\n", g_ChipId_h, g_ChipId_l);
		g_ChipId_h = (uid >> 32) & 0xFFFFFFFF;
		g_ChipId_l = uid & 0xFFFFFFFF;
	}else{
	#if (defined(_MODEL_580_CARDV_ETHCAM_TX_EVB_))
		g_ChipId_h=0x12345678;
		g_ChipId_l=0x87654321;
	#else
		DBG_ERR("unique ID[0x%08x][0x%08x] error\r\n", g_ChipId_h, g_ChipId_l);
		return;
	#endif
	}
	DBG_DUMP("ChipId_h=0x%x, ChipId_l=0x%x\r\n",  g_ChipId_h, g_ChipId_l);
#endif
}
int eth_set_bypass_phy_rst(UINT32 bypass)
{
#if 0
	eth_setConfig(ETH_PHY_RST_BYPASS, bypass);
#endif
	return true;
}
#if 0
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
extern bool gEthCamDhcpCliFailIP;
void EthCamNet_LwIP_DHCP(struct netif *pnetif, UINT8 dhcp_state)
{
#if LWIP_DHCP
	//ip4_addr_t ipaddr;
	//ip4_addr_t netmask;
	//ip4_addr_t gw;
	UINT32 IPaddress;
	UINT8 iptab[4];
	UINT8 DHCP_state;
#ifdef USE_LCD
	UINT8 iptxt[20];
#endif

	int mscnt = 0;
	//struct netif *pnetif = NULL;

	DHCP_state = dhcp_state;

	//if(idx > 1)
	//	idx = 1;
	//pnetif = &xnetif[idx];
	if(DHCP_state == 0){
		pnetif->ip_addr.addr = 0;
		pnetif->netmask.addr = 0;
		pnetif->gw.addr = 0;
	}

	for (;;)
	{
		//DBG_DUMP("========DHCP_state:%d============\n\r",DHCP_state);
		switch (DHCP_state)
		{
			case DHCP_START:
			{
				dhcp_start(pnetif);
				IPaddress = 0;
				DHCP_state = DHCP_WAIT_ADDRESS;
#ifdef USE_LCD
				DBG_DUMP("     Looking for    \r\n");
				DBG_DUMP("     DHCP server    \r\n");
				DBG_DUMP("     please wait... \r\n");
#endif
			}
			break;

			case DHCP_WAIT_ADDRESS:
			{
				/* Read the new IP address */
				IPaddress = pnetif->ip_addr.addr;

				if (IPaddress!=0)
				{
					//coverity[assigned_value]
					DHCP_state = DHCP_ADDRESS_ASSIGNED;

					/* Stop DHCP */
					dhcp_stop(pnetif);

					iptab[0] = (uint8_t)(IPaddress >> 24);
					iptab[1] = (uint8_t)(IPaddress >> 16);
					iptab[2] = (uint8_t)(IPaddress >> 8);
					iptab[3] = (uint8_t)(IPaddress);
#ifdef USE_LCD
					sprintf((char*)iptxt, "  %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]);

					/* Display the IP address */
					DBG_DUMP("IP address assigned \r\n");
					DBG_DUMP("  by a DHCP server  \r\n");
					DBG_DUMP("iptxt=%s\r\n"iptxt);
#endif
					DBG_DUMP("DHCP Get IP address : %d.%d.%d.%d\r\n", iptab[3], iptab[2], iptab[1], iptab[0]);
					DBG_DUMP("netmask : %d.%d.%d.%d\r\n", (pnetif->netmask.addr)&0xff, (pnetif->netmask.addr>>8)&0xff, (pnetif->netmask.addr>>16)&0xff, (pnetif->netmask.addr>>24)&0xff);
					DBG_DUMP("gw : %d.%d.%d.%d\r\n", (pnetif->gw.addr)&0xff, (pnetif->gw.addr>>8)&0xff, (pnetif->gw.addr>>16)&0xff, (pnetif->gw.addr>>24)&0xff);
					gEthCamDhcpCliFailIP=0;
					return;
				}
				else
				{
					//DBG_DUMP("waiting for IP\n");
					DBG_DUMP("##DHCP Client get-ip:cnt=%d\n",mscnt/DHCP_FINE_TIMER_MSECS);

					/* DHCP timeout */
					gEthCamDhcpCliFailIP=1;
					#if 0
					if (0)//(pnetif->dhcp->tries > MAX_DHCP_TRIES)
					{
						//coverity[assigned_value]
						DHCP_state = DHCP_TIMEOUT;

						/* Stop DHCP */
						dhcp_stop(pnetif);

						/* Static address used */
						IP4_ADDR(&ipaddr, IP_ADDR0 ,IP_ADDR1 , IP_ADDR2 , IP_ADDR3 );
						IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
						IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
						netif_set_addr(pnetif, &ipaddr , &netmask, &gw);

						iptab[0] = IP_ADDR3;
						iptab[1] = IP_ADDR2;
						iptab[2] = IP_ADDR1;
						iptab[3] = IP_ADDR0;
#ifdef USE_LCD
						LCD_DisplayStringLine(Line7, (uint8_t*)"    DHCP timeout    ");

						sprintf((char*)iptxt, "  %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]);

						LCD_ClearLine(Line4);
						LCD_ClearLine(Line5);
						LCD_ClearLine(Line6);
						LCD_DisplayStringLine(Line8, (uint8_t*)"  Static IP address   ");
						LCD_DisplayStringLine(Line9, iptxt);
#endif
						DBG_DUMP("\n\rDHCP timeout");
						DBG_DUMP("\n\rStatic IP address : %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]);

						return;
					}else
					#endif
					{
						vos_task_delay_ms(DHCP_FINE_TIMER_MSECS);
						dhcp_fine_tmr();
						mscnt += DHCP_FINE_TIMER_MSECS;
						if (mscnt >= DHCP_COARSE_TIMER_SECS*1000)
						{
							dhcp_coarse_tmr();
							mscnt = 0;
						}
					}
				}
			}
			break;
			case DHCP_RELEASE_IP:
				DBG_DUMP("\n\rLwIP_DHCP: Release ip");
				//dhcp_release_unicast(pnetif);
				return;
			case DHCP_STOP:
				DBG_DUMP("\n\rLwIP_DHCP: dhcp stop.");
				dhcp_stop(pnetif);
				return;
			default:
				break;
		}
	}
#endif
}
UINT8* EthCamNet_LwIP_GetIP(struct netif *pnetif)
{
	return (UINT8 *) &(pnetif->ip_addr);
}
#endif
ID ETHCAM_INIT_ETH_FLG_ID = 0;
THREAD_HANDLE ETHCAM_INIT_ETH_TSK_ID;
#define PRI_ETHCAM_INIT_ETH                10// 6
#define STKSIZE_ETHCAM_INIT_ETH          4096

#define FLG_ETHCAM_INIT_ETH_START            			FLGPTN_BIT(0)// 1
#define FLG_ETHCAM_INIT_ETH_STOP					FLGPTN_BIT(1)// 2
#define FLG_ETHCAM_INIT_ETH_IDLE					FLGPTN_BIT(2)// 4

static ETHCAM_INIT_ETH_CB g_fpInitEthCb = NULL;

static BOOL g_bEthCamNetInitEthOpened = FALSE;

void EthCamNet_RegInitEthCB(ETHCAM_INIT_ETH_CB fpInitEthCb)
{
	g_fpInitEthCb  = fpInitEthCb;
}

void EthCamNet_TrigInitEth(void)
{
	vos_flag_set(ETHCAM_INIT_ETH_FLG_ID, FLG_ETHCAM_INIT_ETH_START);
}

THREAD_RETTYPE EthCamNet_InitEth_Tsk(void)
{
	FLGPTN  FlgPtn;
	DBG_DUMP("EthCamInitEth_Tsk() start\r\n");
	THREAD_ENTRY();

	while (ETHCAM_INIT_ETH_TSK_ID) {
		set_flg(ETHCAM_INIT_ETH_FLG_ID, FLG_ETHCAM_INIT_ETH_IDLE);
		PROFILE_TASK_IDLE();
		wai_flg(&FlgPtn, ETHCAM_INIT_ETH_FLG_ID, FLG_ETHCAM_INIT_ETH_STOP|FLG_ETHCAM_INIT_ETH_START, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		clr_flg(ETHCAM_INIT_ETH_FLG_ID, FLG_ETHCAM_INIT_ETH_IDLE);
		if (FlgPtn & FLG_ETHCAM_INIT_ETH_STOP){
			break;
		}

		if (FlgPtn & FLG_ETHCAM_INIT_ETH_START) {
			DBG_DUMP("FLG_ETHCAM_INIT_ETH_START\r\n");
			if(g_fpInitEthCb){
				g_fpInitEthCb();
			}
			break;
		}
	}
	THREAD_RETURN(0);
}


void EthCamNet_InitEth_Open(void)
{
	if(g_bEthCamNetInitEthOpened){
		return;
	}
	OS_CONFIG_FLAG(ETHCAM_INIT_ETH_FLG_ID);
	ETHCAM_INIT_ETH_TSK_ID=vos_task_create(EthCamNet_InitEth_Tsk,  0, "EthCamNet_InitEth_Tsk",   PRI_ETHCAM_INIT_ETH,	STKSIZE_ETHCAM_INIT_ETH);
	vos_task_resume(ETHCAM_INIT_ETH_TSK_ID);
	g_bEthCamNetInitEthOpened=1;
}
void EthCamNet_InitEth_Close(void)
{
	if(g_bEthCamNetInitEthOpened==0){
		return;
	}
	g_bEthCamNetInitEthOpened=0;
	rel_flg(ETHCAM_INIT_ETH_FLG_ID);
	//vos_task_destroy(ETHCAM_INIT_ETH_TSK_ID);
}
#endif
