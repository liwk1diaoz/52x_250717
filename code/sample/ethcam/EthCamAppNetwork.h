#ifndef _ETHCAMAPPNET_H_
#define _ETHCAMAPPNET_H_
#include "EthCam/EthCamSocket.h"

//#define ETHCAM_LINK_DOWN          (0x01)          //< link down
//#define ETHCAM_LINK_UP           	 (0x02)          //< link up

extern ETHCAM_SOCKET_INFO SocketInfo[];
extern void EthCamNet_EthernetLinkDet(void);
extern UINT32 EthCamNet_GetEthLinkStatus(ETHCAM_PATH_ID path_id);
extern UINT32 EthCamNet_GetPrevEthLinkStatus(ETHCAM_PATH_ID path_id);
extern void EthCamNet_SetPrevEthLinkStatus(ETHCAM_PATH_ID path_id, UINT32 LinkStatus);
extern void EthCamNet_EthLinkStatusNotify(char *cmd);
extern void EthCamNet_SrvCliConnIPAddrNofity(char *cmd);
extern void EthCamNet_SetTxIPAddr(void);
extern void EthCamNet_SetTxHB(UINT32 value);
extern UINT32 EthCamNet_GetTxHB(void);
extern UINT32 EthCamNet_GetEthHub_LinkStatus(ETHCAM_PATH_ID path_id);
extern void EthCamNet_EthHubLinkDet(void);
extern UINT32 ipstr2int(char* ip_addr);
extern void EthCamNet_Ethboot(void);
extern void EthCamNet_EnableEthernetLinkDet(void);
extern void EthCamNet_DisableEthernetLinkDet(void);
extern void EthCamNet_EthernetLinkDown(void);

#endif //_ETHCAMAPPNET_H_
