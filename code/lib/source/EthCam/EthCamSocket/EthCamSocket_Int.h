#ifndef _ETHCAM_SOCKETINT_H
#define _ETHCAM_SOCKETINT_H
#include "kwrap/type.h"
#include "kwrap/task.h"

#include "EthCam/EthsockIpcAPI.h"
#include "EthCam/EthsockCliIpcAPI.h"

#include "kwrap/util.h"
#define Delay_DelayMs(ms) vos_util_delay_ms(ms)

#define FLG_ETHCAMSOCKETCLI_STOP     		FLGPTN_BIT(0)
#define FLG_ETHCAMSOCKETCLI_IDLE     		FLGPTN_BIT(1)
#define FLG_ETHCAMSOCKETCLI_RECONN       	FLGPTN_BIT(2)

typedef void (*ETHCAMSOCKET_RECV_PROCESS_CB)(char* addr , int size);
typedef void (*ETHCAMSOCKET_CLI_RECV_PROCESS_CB)(ETHCAM_PATH_ID path_id, char* addr , int size);
typedef void (*ETHCAMSOCKET_NOTIFY_PROCESS_CB)(int status, int parm);
typedef void (*ETHCAMSOCKET_CLI_NOTIFY_PROCESS_CB)(ETHCAM_PATH_ID path_id, int status, int parm);

typedef void (*ETHCAMSOCKET_CMD_SEND_DATA_SIZE_CB)(int size, int total_size);

void EthCamSocket_Data1Notify(int status, int parm);
int EthCamSocket_Data1Recv(char* addr, int size);
void EthCamSocket_Data2Notify(int status, int parm);
int EthCamSocket_Data2Recv(char* addr, int size);
void EthCamSocket_CmdNotify(int status, int parm);
int EthCamSocket_CmdRecv(char* addr, int size);

void EthCamSocketCli_Data1Notify(int path_id , int status, int parm);
int EthCamSocketCli_Data1Recv(int path_id , char* addr, int size);
void EthCamSocketCli_Data2Notify(int path_id , int status, int parm);
int EthCamSocketCli_Data2Recv(int path_id , char* addr, int size);
void EthCamSocketCli_CmdNotify(int path_id , int status, int parm);
int EthCamSocketCli_CmdRecv(int path_id , char* addr, int size);

#if 0//Tx
int EthCamSocket_DataSend(ETHSOCKIPC_ID id, char* addr, int *size);
int EthCamSocket_CmdSend(char* addr, int *size);
#endif
int EthCamSocketCli_DataSend(ETHCAM_PATH_ID path_id, ETHSOCKIPCCLI_ID id, char* addr, int *size);
int EthCamSocketCli_CmdSend(ETHCAM_PATH_ID path_id, char* addr, int *size);
int EthCamSocketCli_HandleTskOpen(void);
int EthCamSocketCli_HandleTskClose(void);
THREAD_RETTYPE EthCamSocketCli_HandleTsk(void);


#endif //_ETHCAM_SOCKETINT_H
