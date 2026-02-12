/* =================================================================
 *
 *      ethsocket_cli.h
 *
 *      A simple ethcam socket client
 *
 * =================================================================
 */
#if defined(__ECOS)
#include <cyg/infra/cyg_trac.h>        /* tracing macros */
#include <cyg/infra/cyg_ass.h>         /* assertion macros */
#endif


#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>

#include "kwrap/task.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/type.h"
#include <kwrap/util.h>


#include "EthCam/ethsocket_cli.h"
#include "EthCam/ethsocket_cli_ipc.h"

//#include "ethsocket_cli_int.h"

#if defined(__ECOS)
#include <network.h>
#include <arpa/inet.h>
#include <cyg/ethsocket_cli/ethsocket_cli.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
//#include <UsockIpc/usocket.h>
#include <netdb.h>
#include <sys/ioctl.h>
//#include <pthread.h>
#endif

//#if defined(__USE_IPC)
//#include "ethsocket_cli_ipc.h"
//#endif


#define ETHSOCKETCLI_SERVER_PORT            6666
#define ETHSOCKETCLI_THREAD_PRIORITY        6
#define ETHSOCKETCLI_THREAD_STACK_SIZE      5120
#if defined(__USE_IPC)
#define ETHSOCKETCLI_BUFFER_SIZE        	ETHSOCKCLIIPC_TRANSFER_BUF_SIZE
#else
#define ETHSOCKETCLI_BUFFER_SIZE          ETHSOCKCLIIPC_TRANSFER_BUF_SIZE //48*1024 //8*1024
#endif
/* ================================================================= */

#if 0
#if defined(__ECOS)
#define ETHSOCKETCLI_DIAG diag_printf
#else
#define ETHSOCKETCLI_DIAG printf
#endif
#else
#define ETHSOCKETCLI_DIAG(...)
#endif
#define CHKPNT  printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)


/* ================================================================= */
/* socket address and file descriptor.
 */

#define CYGNUM_MAX_PORT_NUM             (65535)

#define ETHSOCKETCLI_SOCK_BUF_SIZE      (8*1024)

#define CYGNUM_SELECT_TIMEOUT_USEC      (100000)//(500000)

#define ETHSOCKETCLI_IDLE_TIME          (1)  // retry 20

#define ETHSOCKETCLI_TIMEOUT            (20)

// flag define
#define CYG_FLG_ETHSOCKETCLI_START      (0x01)
#define CYG_FLG_ETHSOCKETCLI_IDLE       (0x80)
#define MAX_CLIENT_NUM                  (3)
#define MAX_PATH_NUM                  (2)

#define STKSIZE_ETHCAM_CLI_PROC           4096

//ethsocket_cli_obj gethsocket_cli_obj[MAX_CLIENT_NUM] = {0};
ethsocket_cli_obj gethsocket_cli_obj[MAX_PATH_NUM][MAX_CLIENT_NUM] =
							{
								{{0, 0,     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,      0, 0, 0, -1},{0, 0,     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,      0, 0, 0, -1},{0, 0,     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,      0, 0, 0, -1}},
								{{0, 0,     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,      0, 0, 0, -1},{0, 0,     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,      0, 0, 0, -1},{0, 0,     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,      0, 0, 0, -1}}
							};

#if defined(__ECOS)
static cyg_flag_t ethsocket_cli_flag[MAX_PATH_NUM][MAX_CLIENT_NUM];
#else
//static volatile int ethsocket_cli_flag[MAX_PATH_NUM][MAX_CLIENT_NUM];
ID ethsocket_cli_flag[MAX_PATH_NUM][MAX_CLIENT_NUM] = {0};
#endif


static bool isStartClient[MAX_PATH_NUM][MAX_CLIENT_NUM] = {false};
static bool isStopClient[MAX_PATH_NUM][MAX_CLIENT_NUM] = {false};
static int fdmax[MAX_PATH_NUM][MAX_CLIENT_NUM];
static fd_set master[MAX_PATH_NUM][MAX_CLIENT_NUM];  // master file descriptor list

static ethsocket_cli_id_mapping proc_id_tbl[MAX_PATH_NUM*MAX_CLIENT_NUM]={{0,0} ,{0,1} ,{0,2},{1,0},{1,1},{1,2}}; //{{ETHCAM_PATH_ID_1,ETHSOCKIPCCLI_ID_0} ,{ETHCAM_PATH_ID_1,ETHSOCKIPCCLI_ID_1} ,{ETHCAM_PATH_ID_1,ETHSOCKIPCCLI_ID_2},{ETHCAM_PATH_ID_2,ETHSOCKIPCCLI_ID_0},{ETHCAM_PATH_ID_2,ETHSOCKIPCCLI_ID_1},{ETHCAM_PATH_ID_2,ETHSOCKIPCCLI_ID_2}};


/* ================================================================= */
/* Thread stacks, etc.
 */

#if defined(__ECOS)
static cyg_uint8 ethsocket_cli_stacks[MAX_PATH_NUM][MAX_CLIENT_NUM][ETHSOCKETCLI_BUFFER_SIZE + ETHSOCKETCLI_THREAD_STACK_SIZE] = {0};
#endif

#if defined(__ECOS)
static cyg_handle_t ethsocket_cli_thread[MAX_PATH_NUM][MAX_CLIENT_NUM];
static cyg_thread   ethsocket_cli_thread_object[MAX_PATH_NUM][MAX_CLIENT_NUM];
#else
//static pthread_t    ethsocket_cli_thread[MAX_PATH_NUM][MAX_CLIENT_NUM];
static THREAD_HANDLE ethsocket_cli_thread[MAX_PATH_NUM][MAX_CLIENT_NUM];;

#endif
#if defined(__ECOS)
static char ethsocket_cli_thread_name[MAX_PATH_NUM][MAX_CLIENT_NUM][20]={{{"ethsocketCli0_p0"}, {"ethsocketCli1_p0"}, {"ethsocketCli2_p0"}},
																		{{"ethsocketCli0_p1"}, {"ethsocketCli1_p1"}, {"ethsocketCli2_p1"}}
																		};
#endif
static int ethsocket_cli_mapping_id[MAX_PATH_NUM][MAX_CLIENT_NUM]={{-1, -1, -1}, {-1, -1, -1}};

static int recvall(int s, char* buf, int len, int isJustOne, int obj_index, int path_id);
static int sendall(int s, char* buf, int *len, int obj_index);
//extern void ethsocket_cli_ipc_notify(ethsocket_cli_obj* obj, int status, int parm);
//extern int ethsocket_cli_ipc_recv(ethsocket_cli_obj* obj, char* addr, int size);

ethsocket_cli_obj *ethsocket_cli_get_newobj(int path_id, int obj_index)
{
	#if 0 //for only ETHSOCKIPCCLI_ID_0 + ETHSOCKIPCCLI_ID_2,no ETHSOCKIPCCLI_ID_1
	int i=0;
	for (i=0; i<MAX_CLIENT_NUM; i++) {
		ETHSOCKETCLI_DIAG("gethsocket_cli_obj[%d].connect_socket %x\n",i,gethsocket_cli_obj[i].connect_socket);

		//if (gethsocket_cli_obj[i].connect_socket== 0) {
		if (gethsocket_cli_obj[i].connect_socket== -1) {
			ETHSOCKETCLI_DIAG("get %d gethsocket_cli_obj %x\n",i, &gethsocket_cli_obj[i]);
			return &gethsocket_cli_obj[i];
		}
	}
	printf("err,no empty obj\n");
	#else
	if (gethsocket_cli_obj[path_id][obj_index].connect_socket== -1) {
		return &gethsocket_cli_obj[path_id][obj_index];
	}else{
		printf("err,no empty obj\n");
		printf("socket[%d][%d]=%d\n",path_id,obj_index,gethsocket_cli_obj[path_id][obj_index].connect_socket);
	}
	#endif
	return NULL;
}

/* ================================================================= */
/* Main processing function                                          */
/*                                                                   */
char request[MAX_PATH_NUM][MAX_CLIENT_NUM][ETHSOCKETCLI_BUFFER_SIZE]={0};
extern unsigned int g_SocketCliData2_RecvAddr[MAX_PATH_NUM];
extern unsigned int g_SocketCliData2_RecvSize[MAX_PATH_NUM];
extern unsigned char *g_SocketCliData2_BsFrmBufAddr[MAX_PATH_NUM];
extern unsigned int g_SocketCliData2_BsSize[MAX_PATH_NUM];

extern unsigned int gSndParamSize[MAX_PATH_NUM][MAX_CLIENT_NUM];
extern unsigned int g_SocketCliData1_RecvAddr[MAX_PATH_NUM];
extern unsigned int g_SocketCliData1_RecvSize[MAX_PATH_NUM];
extern unsigned char *g_SocketCliData1_BsFrmBufAddr[MAX_PATH_NUM];
extern unsigned int g_SocketCliData1_BsSize[MAX_PATH_NUM];

extern unsigned int g_SocketCliData_DebugAddr[MAX_PATH_NUM][MAX_CLIENT_NUM];
extern unsigned int g_SocketCliData_DebugSize[MAX_PATH_NUM][MAX_CLIENT_NUM];
extern unsigned int g_SocketCliData_DebugRecvSize[MAX_PATH_NUM][MAX_CLIENT_NUM];


extern unsigned int g_SocketCliData_DebugReceiveEn[MAX_PATH_NUM][MAX_CLIENT_NUM];


// this function will return true, if the connection is closed.
static cyg_bool ethsocket_cli_receive(int path_id, int obj_index, int cur_socket, ethsocket_cli_obj *pethsocket_cli_obj)
{

	//char request[ETHSOCKETCLI_BUFFER_SIZE]={0};
	int  ret = 1;
	char* addr_debug;

	ETHSOCKETCLI_DIAG("ethsocket_cli_receive ,obj_index=%d\r\n",obj_index);

	if(obj_index==-1)
	{
        	//perror("recv ,obj_index -1");
        	printf("recv ,obj_index -1\r\n");
        	return false;
	}
	if(cur_socket != pethsocket_cli_obj->connect_socket){
        	//perror("recv ,cur_socket");
        	printf("recv ,cur_socket\r\n");
    		return false;
	}
#if 1
	if(obj_index==1){
		//ret = recvall(cur_socket,request[path_id][obj_index], sizeof(request[path_id][obj_index]),true ,obj_index, path_id);

		if(g_SocketCliData2_BsSize[path_id]==0 || g_SocketCliData2_BsFrmBufAddr[path_id]==NULL){

			if(gSndParamSize[path_id][obj_index]<= g_SocketCliData2_RecvSize[path_id]){
				printf("[%d][%d]D2ERR: recv size =%d, %d, bsSz=%d, BsFrmBufAddr=%d\r\n",path_id,obj_index,gSndParamSize[path_id][obj_index], g_SocketCliData2_RecvSize[path_id], g_SocketCliData2_BsSize[path_id], ((g_SocketCliData2_BsFrmBufAddr[path_id]==NULL)? 0: 1));
				g_SocketCliData_DebugReceiveEn[path_id][obj_index] =1;
				ret=-2;
			}else{
				ret = recvall(cur_socket,(char*)(g_SocketCliData2_RecvAddr[path_id]+g_SocketCliData2_RecvSize[path_id]), (gSndParamSize[path_id][obj_index] - g_SocketCliData2_RecvSize[path_id] ),true ,obj_index, path_id);
				addr_debug =(char*)(g_SocketCliData2_RecvAddr[path_id]+g_SocketCliData2_RecvSize[path_id]);
			}

		}else{
			if( g_SocketCliData2_BsSize[path_id]<= g_SocketCliData2_RecvSize[path_id]){
				printf("[%d][%d]D2ERR: recv size =0x%x, %d, %d\r\n",path_id,obj_index,g_SocketCliData2_BsFrmBufAddr[path_id],g_SocketCliData2_BsSize[path_id], g_SocketCliData2_RecvSize[path_id]);
				g_SocketCliData_DebugReceiveEn[path_id][obj_index] =1;
				ret=-2;
			}else{
				ret = recvall(cur_socket,(char*)(g_SocketCliData2_BsFrmBufAddr[path_id]+g_SocketCliData2_RecvSize[path_id]), (g_SocketCliData2_BsSize[path_id] - g_SocketCliData2_RecvSize[path_id]),true ,obj_index, path_id);
				addr_debug =(char*)(g_SocketCliData2_BsFrmBufAddr[path_id]+g_SocketCliData2_RecvSize[path_id]);
			}
		}
		if(ret>0){
			if(g_SocketCliData_DebugAddr[path_id][obj_index] && g_SocketCliData_DebugSize[path_id][obj_index]){

				if(g_SocketCliData_DebugSize[path_id][obj_index] < (ret + g_SocketCliData_DebugRecvSize[path_id][obj_index])){
					g_SocketCliData_DebugRecvSize[path_id][obj_index]=0;
				}

				memcpy((char*)(g_SocketCliData_DebugAddr[path_id][obj_index]+g_SocketCliData_DebugRecvSize[path_id][obj_index]), (char*)addr_debug, ret);
				g_SocketCliData_DebugRecvSize[path_id][obj_index]+=ret;
			}
		}
	}else if(obj_index==0){

		if(g_SocketCliData1_BsSize[path_id]==0 || g_SocketCliData1_BsFrmBufAddr[path_id]==NULL){

			if(gSndParamSize[path_id][obj_index]<= g_SocketCliData1_RecvSize[path_id]){
				printf("[%d][%d]D1ERR: recv size =%d, %d, bsSz=%d, BsFrmBufAddr=%d\r\n",path_id,obj_index,gSndParamSize[path_id][obj_index], g_SocketCliData1_RecvSize[path_id], g_SocketCliData1_BsSize[path_id], ((g_SocketCliData1_BsFrmBufAddr[path_id]==NULL)? 0: 1));
				g_SocketCliData_DebugReceiveEn[path_id][obj_index] =1;
				ret=-2;
			}else{
				ret = recvall(cur_socket,(char*)(g_SocketCliData1_RecvAddr[path_id]+g_SocketCliData1_RecvSize[path_id]), (gSndParamSize[path_id][obj_index] - g_SocketCliData1_RecvSize[path_id] ),true ,obj_index, path_id);
				addr_debug =(char*)(g_SocketCliData1_RecvAddr[path_id]+g_SocketCliData1_RecvSize[path_id]);
			}
		}else{
			if( g_SocketCliData1_BsSize[path_id]<= g_SocketCliData1_RecvSize[path_id]){
				printf("[%d][%d]D1ERR: recv size =0x%x, %d, %d\r\n",path_id,obj_index,g_SocketCliData1_BsFrmBufAddr[path_id],g_SocketCliData1_BsSize[path_id], g_SocketCliData1_RecvSize[path_id]);
				g_SocketCliData_DebugReceiveEn[path_id][obj_index] =1;
				ret=-2;
			}else{
				ret = recvall(cur_socket,(char*)(g_SocketCliData1_BsFrmBufAddr[path_id]+g_SocketCliData1_RecvSize[path_id]), (g_SocketCliData1_BsSize[path_id] - g_SocketCliData1_RecvSize[path_id]),true ,obj_index, path_id);
				addr_debug =(char*)(g_SocketCliData1_BsFrmBufAddr[path_id]+g_SocketCliData1_RecvSize[path_id]);
			}
		}
		if(ret>0){
			if(g_SocketCliData_DebugAddr[path_id][obj_index] && g_SocketCliData_DebugSize[path_id][obj_index]){

				if(g_SocketCliData_DebugSize[path_id][obj_index] < (ret + g_SocketCliData_DebugRecvSize[path_id][obj_index])){
					g_SocketCliData_DebugRecvSize[path_id][obj_index]=0;
				}

				memcpy((char*)(g_SocketCliData_DebugAddr[path_id][obj_index]+g_SocketCliData_DebugRecvSize[path_id][obj_index]), (char*)addr_debug, ret);
				g_SocketCliData_DebugRecvSize[path_id][obj_index]+=ret;
			}
		}
	}else{
		ret = recvall(cur_socket,request[path_id][obj_index], sizeof(request[path_id][obj_index]),true ,obj_index, path_id);
	}

	if(g_SocketCliData_DebugAddr[path_id][obj_index] && g_SocketCliData_DebugSize[path_id][obj_index]){
		if(g_SocketCliData_DebugReceiveEn[path_id][obj_index]){
			//extern INT32 socketCliEthData_DebugDump(UINT32 path_id,UINT32 id);
			//socketCliEthData_DebugDump(path_id, obj_index);
		}
	}
#else
	ret = recvall(cur_socket,request[path_id][obj_index], sizeof(request[path_id][obj_index]),true ,obj_index, path_id);
#endif

	if (ret == 0) {
		//perror("server disconnect!!\r\n");
		printf("server disconnect!![%d][%d]\r\n",path_id,obj_index);

		return true;
	}
	if (ret == -1) {
		//perror("recv time out");
		printf("recv time out[%d][%d]\r\n",path_id,obj_index);
		return false;
	} else {
		if (pethsocket_cli_obj->recv) {
			//ethsocket_cli_ipc_recv(pethsocket_cli_obj, request, ret);
			pethsocket_cli_obj->recv(obj_index, request[path_id][obj_index], ret);
		}
		#if 0
		if(g_SocketCliData_DebugReceiveEn[path_id][obj_index]){
			printf("[%d][%d]DebugReceiveEn poweroff\r\n",path_id,obj_index);
			g_SocketCliData_DebugReceiveEn[path_id][obj_index]=0;
			extern void System_OnPowerPostExit(void);
			System_OnPowerPostExit();
			while(1);
		}
		#endif

		return false;
	}

}
/* ================================================================= */
/* Initialization thread
 *
 * Optionally delay for a time before getting the network
 * running. Then create and connet to server.Enter client receive
 * mode.
 */

#if defined(__ECOS)
static void ethsocket_cli_proc(cyg_addrword_t arg)
#else
//static void* ethsocket_cli_proc(void* arg)
static THREAD_RETTYPE ethsocket_cli_proc(void* arg)
//static void* ethsocket_cli_proc(cyg_addrword_t arg)
#endif
{
	//fd_set master;  // master file descriptor list
	fd_set readfds; // temp file descriptor list for select()
	//int fdmax =0,i;
	int i;//, j;
	int ret=-1;
	struct timeval tv;
	ethsocket_cli_obj* pethsocket_cli_obj =0;
#if 1
/*
    	#if defined(__ECOS)
		int obj_index = (int) arg;
    	#else
		int obj_index = (int) *arg;
    	#endif
*/
	int path_id=0;
	int obj_index=0;
    	#if defined(__ECOS)
		int mapping_id = (int) arg;
    	#else
		int* mapping_id_arg = (int *) arg;
		//int mapping_id = (int) arg;
		int mapping_id = (int) *mapping_id_arg;

    	#endif
	THREAD_ENTRY();

	path_id=proc_id_tbl[mapping_id].path_id;
	obj_index	=proc_id_tbl[mapping_id].socket_id;
#else
	int obj_index=0;
	int path_id=0;
	int j;
	for (j=0; j<MAX_PATH_NUM; j++) {
		for (i=0; i<MAX_CLIENT_NUM; i++) {
			ETHSOCKETCLI_DIAG("gethsocket_cli_obj[%d].connect_socket %x\n",i,gethsocket_cli_obj[j][i].connect_socket);
			//if (gethsocket_cli_obj[i].connect_socket== 0) {
			if (&gethsocket_cli_obj[j][i]== (ethsocket_cli_obj *) arg) {
				ETHSOCKETCLI_DIAG("get %d, %d gethsocket_cli_obj %x\n",j,i, &gethsocket_cli_obj[j][i]);
				path_id=j;
				obj_index=i;
				ret=0;
				break;
			}
		}
	}
	if(ret==-1){
		printf("err, get path id fail !!\n");
	}
#endif
	ETHSOCKETCLI_DIAG("ethsocket_cli_proc, mapping_id=%d, path_id=%d,obj_index=%d\r\n",mapping_id, path_id,obj_index);

	// clear the set
	//FD_ZERO(&master[path_id][obj_index]);
	//FD_ZERO(&readfds);

#if 0
	// add socket to the set
	for (i=0; i < MAX_CLIENT_NUM; i++) {
		if (gethsocket_cli_obj[i].connect_socket) {
			ETHSOCKETCLI_DIAG("FD_SET %d\n",gethsocket_cli_obj[i].connect_socket);
			FD_SET(gethsocket_cli_obj[i].connect_socket, &master);
			if (gethsocket_cli_obj[i].connect_socket>fdmax) {
				fdmax = gethsocket_cli_obj[i].connect_socket;
			}
		}
	}
#else
	//FD_SET(gethsocket_cli_obj[path_id][obj_index].connect_socket, &master[path_id][obj_index]);
	fdmax[path_id][obj_index] = gethsocket_cli_obj[path_id][obj_index].connect_socket;
#endif
	// set timeout to 100 ms
	tv.tv_sec = 0;
	tv.tv_usec = 100000;//5000;
	//cyg_flag_maskbits(&ethsocket_cli_flag[path_id][obj_index], 0);
	//cyg_flag_setbits(&ethsocket_cli_flag[path_id][obj_index], CYG_FLG_ETHSOCKETCLI_START);
	vos_flag_clr(ethsocket_cli_flag[path_id][obj_index], 0xFFFFFFFF);
	vos_flag_set(ethsocket_cli_flag[path_id][obj_index], CYG_FLG_ETHSOCKETCLI_START);
	while (!isStopClient[path_id][obj_index] && ethsocket_cli_thread[path_id][obj_index]) {
		// copy it
		FD_ZERO(&master[path_id][obj_index]);
		FD_ZERO(&readfds);
		if(gethsocket_cli_obj[path_id][obj_index].connect_socket==-1){
			printf("err: [%d][%d]connect_socket -1!!!",path_id,obj_index);
			goto PROC_END;
		}else{
			FD_SET(gethsocket_cli_obj[path_id][obj_index].connect_socket, &master[path_id][obj_index]);
		}
		tv.tv_sec = 0;
		tv.tv_usec = 100000;//5000;
		readfds = master[path_id][obj_index];
		ret = select(fdmax[path_id][obj_index] + 1, &readfds, NULL, NULL, &tv);
		switch(ret) {
		case -1: {
				perror("select");
				break;
			}
		case 0: {
				if (isStopClient[path_id][obj_index]) {
					//ETHSOCKETCLI_DIAG("select timeout %x\n",pethsocket_cli_obj);
				#if 0
					int j = 0;
					for (j = 0; j < MAX_CLIENT_NUM; j++) {
						if (gethsocket_cli_obj[j].connect_socket) {
							ethsocket_cli_disconnect(&gethsocket_cli_obj[j]);
							ethsocket_cli_uninstall(&gethsocket_cli_obj[j]);
							FD_CLR(gethsocket_cli_obj[j].connect_socket, &master);
							break;
						}
					}
				#else
				ETHSOCKETCLI_DIAG("connect_socket obj_index=%d, connect_socket=%d\n",obj_index,gethsocket_cli_obj[path_id][obj_index].connect_socket);
				if(gethsocket_cli_obj[path_id][obj_index].connect_socket>=0){
					FD_CLR(gethsocket_cli_obj[path_id][obj_index].connect_socket, &master[path_id][obj_index]);
				}
				ethsocket_cli_disconnect(&gethsocket_cli_obj[path_id][obj_index]);
				ethsocket_cli_uninstall(path_id, obj_index, &gethsocket_cli_obj[path_id][obj_index]);
				//FD_CLR(gethsocket_cli_obj[obj_index].connect_socket, &master[obj_index]);
				#endif
				}else{
					continue;
				}
				break;
			}
		default: {
				// run throuth the existing connections looking for data to read
				ETHSOCKETCLI_DIAG("obj_index=%d, fdmax %d readfds %d\n",obj_index,fdmax[path_id][obj_index],readfds);
				//for (i = 0; i <= fdmax[path_id][obj_index]; i++) {
				{
					{
						i= fdmax[path_id][obj_index];
						// one data coming
						if (FD_ISSET(i, &readfds)) {

							pethsocket_cli_obj = &gethsocket_cli_obj[path_id][obj_index];
							if(i== gethsocket_cli_obj[path_id][obj_index].connect_socket)
							{
								ETHSOCKETCLI_DIAG("handle data from server %d  %x notify %x\n", i, pethsocket_cli_obj, pethsocket_cli_obj->notify);
								// client has request
								if (pethsocket_cli_obj && pethsocket_cli_obj->notify) {
									//ethsocket_cli_ipc_notify(pethsocket_cli_obj,CYG_ETHSOCKETCLI_STATUS_CLIENT_REQUEST, 0);
									//pethsocket_cli_obj->notify(obj_index,CYG_ETHSOCKETCLI_STATUS_CLIENT_REQUEST, 0);
								}
								ret = ethsocket_cli_receive(path_id, obj_index, i, pethsocket_cli_obj);

								// if connection close
								if (ret) {
									printf("server conn close[%d][%d]\n",path_id,obj_index);
									ethsocket_cli_disconnect(pethsocket_cli_obj);
									ETHSOCKETCLI_DIAG("FD_CLR %d\n",i);
									FD_CLR(i, &master[path_id][obj_index]);
									if (pethsocket_cli_obj && pethsocket_cli_obj->notify) {
										//ethsocket_cli_ipc_notify(pethsocket_cli_obj,CYG_ETHSOCKETCLI_STATUS_CLIENT_DISCONNECT, 0);
										ETHSOCKETCLI_DIAG("eCos index=%d DISCONN",obj_index);
										//pethsocket_cli_obj->notify(obj_index,CYG_ETHSOCKETCLI_STATUS_CLIENT_DISCONNECT, 0);
										pethsocket_cli_obj->notify(obj_index,CYG_ETHSOCKETCLI_STATUS_CLIENT_SOCKET_CLOSE, 0);
									}
									ethsocket_cli_uninstall(path_id, obj_index ,pethsocket_cli_obj);
									printf("server conn close end\n");
									goto PROC_END;
								}
							}
						}
					}
				}
			}
		}
	}
	PROC_END:
	// process would stop,close all the opened sockets
	for (i = 0; i <= fdmax[path_id][obj_index]; i++) {
		if (FD_ISSET(i, &master[path_id][obj_index])) {
			close(i);
			ETHSOCKETCLI_DIAG("close socket %d\n",i);
		}
	}
	gethsocket_cli_obj[path_id][obj_index].connect_socket=-1;

	//cyg_flag_setbits(&ethsocket_cli_flag[path_id][obj_index], CYG_FLG_ETHSOCKETCLI_IDLE);
	vos_flag_set(ethsocket_cli_flag[path_id][obj_index],CYG_FLG_ETHSOCKETCLI_IDLE);
	printf("ethsocket_cli_proc close\n");

	THREAD_RETURN(0);
	//return DUMMY_NULL;

}

int ethsocket_cli_connect(int obj_index, ethsocket_cli_obj *pethsocket_cli_obj)
{
	int err = 0, flag;
	struct addrinfo *res;
	char   portNumStr[10];
	int    portNum = pethsocket_cli_obj->portNum;
	int    sock_buf_size = ETHSOCKETCLI_SOCK_BUF_SIZE;
	struct addrinfo hints;
	char   *ipStr=pethsocket_cli_obj->svrIP;
	int     new_socket = 0;
	fd_set  myset;

	ETHSOCKETCLI_DIAG("ethsocket_cli_connect\n");

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	sprintf(portNumStr, "%d", portNum);
	ETHSOCKETCLI_DIAG("portNum %d ip %s\r\n", portNum, ipStr);
	if ((err = getaddrinfo(ipStr, portNumStr, &hints, &res)) != 0) {
		printf("getaddrinfo: %s\n", gai_strerror(err));
		return -1;
	}

	/* Create socket.
	*/
	ETHSOCKETCLI_DIAG("create socket\r\n");
	new_socket = socket( res->ai_family, res->ai_socktype, res->ai_protocol );

	ETHSOCKETCLI_DIAG("new_socket = %d\r\n",new_socket);
	if(new_socket<0){
		perror("socket");
	}
	CYG_ASSERT(new_socket >= 0, "Socket create failed");

	/* Disable the Nagle (TCP No Delay) Algorithm */
	#if 0
	flag = 1;
	err = setsockopt(new_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
	if (err == -1) {
		printf("Couldn't setsockopt(TCP_NODELAY)\n");
	}
	#endif
	#if 1
	sock_buf_size = pethsocket_cli_obj->sockbufSize;
	flag = 1;
	err = setsockopt( new_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_REUSEADDR)\n");
	}

	#if defined(__ECOS)
	err = setsockopt( new_socket, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
	#else
	#if defined (SO_REUSEPORT)
	err = setsockopt( new_socket, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
	#endif
	#endif
	if (err == -1) {
		printf("Couldn't setsockopt(SO_REUSEPORT)\n");
	}

	err = setsockopt(new_socket , SOL_SOCKET, SO_SNDBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_SNDBUF)\n");
	}

	//cat /proc/sys/net/ipv4/tcp_rmem
	//\BSP\root-fs\rootfs\etc_Model\etc_CARDV_ETHCAM_RX_EVB\sysctl.conf
	//net.ipv4.tcp_rmem=4096 453376 453376
	err = setsockopt(new_socket , SOL_SOCKET, SO_RCVBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_RCVBUF)\n");
	}
	#endif

	/* Set the socket for non-blocking mode. */
	int yes = 1;
	if (ioctl(new_socket, FIONBIO, &yes) != 0) {
		/* Error */
		perror("errno\r\n");
	}
	ETHSOCKETCLI_DIAG("non-block connect port %d\r\n",portNum);

	err = connect(new_socket, res->ai_addr, res->ai_addrlen);
	ETHSOCKETCLI_DIAG("connect result %d\r\n",err);

	if (err < 0) {
		if (errno == EINPROGRESS) {
			//printf("EINPROGRESS in connect() - selecting\n");
			do {
				struct timeval tv;
				int result = 0;
				int valopt;
				int lon = 0 ;
				tv.tv_sec = pethsocket_cli_obj->timeout;
				tv.tv_usec = 0;

				FD_ZERO(&myset);
				FD_SET(new_socket, &myset);
				result = select(new_socket + 1, NULL, &myset, NULL, &tv);
				ETHSOCKETCLI_DIAG("select result %d\r\n",result);

				if (result < 0 && errno != EINTR) {
					perror("select\n");
					break;
				} else if (result > 0) {
					//ETHSOCKETCLI_DIAG("Error in delayed connection result %d \r\n",result);
					err = 0;
					// Socket selected for write
					lon = sizeof(int);
					if (getsockopt(new_socket, SOL_SOCKET, SO_ERROR, (void*)(&valopt), (socklen_t *)&lon) < 0) {
						ETHSOCKETCLI_DIAG("Error in getsockopt() %d\n", errno);
						perror("getsockopt 1 \r\n");
						err = -1;
						break;
					}
					// Check the value returned...
					if (valopt) {
						printf("Error in delayed connection() %d - %s\n", valopt, strerror(valopt));
						//perror("getsockopt 2\r\n");
						err = -3;
						break;
					} else {
						ETHSOCKETCLI_DIAG("connect\n");
						err = 0;
						break;
					}
				} else {
					printf("Timeout! result %d\n",result);
					err = -2;
					break;
				}
			} while (1);
		} else {
			perror("errno\r\n");
		}
	}

	if (err == 0) {
		//printf("new_socket=%d\n",new_socket);
		pethsocket_cli_obj->connect_socket = new_socket;
	} else {
		printf("close new_socket=%d\n",new_socket);
		//pethsocket_cli_obj->connect_socket = 0;
		pethsocket_cli_obj->connect_socket = -1;
		close(new_socket);
	}

	freeaddrinfo(res);   // free the linked list
	ETHSOCKETCLI_DIAG("ethsocket_cli_connect end err %d\r\n",err);
	return err;
}

void ethsocket_cli_disconnect(ethsocket_cli_obj *pethsocket_cli_obj)
{
	if (pethsocket_cli_obj) {
		ETHSOCKETCLI_DIAG("ethsocket_cli_disconnect, close socket %d\n", pethsocket_cli_obj->connect_socket);
		if(pethsocket_cli_obj->connect_socket>=0){
			close(pethsocket_cli_obj->connect_socket);
		}
		//pethsocket_cli_obj->connect_socket = 0 ;
		pethsocket_cli_obj->connect_socket = -1 ;
		ETHSOCKETCLI_DIAG("pethsocket_cli_obj %x disconn\r\n", pethsocket_cli_obj);
	}
}

/* ================================================================= */
/* System initializer
 *
 * This is called from the static constructor in init.cxx. It spawns
 * the main client thread and makes it ready to run. It can also be
 * called explicitly by the application if the auto start option is
 * disabled.
 */

__externC void ethsocket_cli_start(int path_id, int obj_index)
{
	T_CFLG cflg ;
	char name[40]={0};
	ETHSOCKETCLI_DIAG("ethsocket_cli_start , obj_index=%d\r\n",obj_index);

	if(path_id >=MAX_PATH_NUM){
		printf("error: ethsocket_cli_start path_id out of range! %d\r\n",path_id);
	}
	if(obj_index >=MAX_CLIENT_NUM){
		printf("error: ethsocket_cli_start obj_index out of range! %d\r\n",obj_index);
	}
	if (!isStartClient[path_id][obj_index]) {
		isStopClient[path_id][obj_index] = false;
		//cyg_flag_init(&ethsocket_cli_flag[path_id][obj_index]);
		snprintf(name, sizeof(name) - 1, "ethsoCliFlag_%d_%d",path_id,obj_index);
		vos_flag_create(&ethsocket_cli_flag[path_id][obj_index], &cflg, name);
		vos_flag_clr(ethsocket_cli_flag[path_id][obj_index], 0xFFFFFFFF);
		ETHSOCKETCLI_DIAG("ethsocket_cli_start create thread\r\n");
		ETHSOCKETCLI_DIAG("ethsocket_cli_start, get path_id = %d, obj_index=%d\n",path_id,obj_index);

		int i ;
		for (i=0; i<MAX_PATH_NUM* MAX_CLIENT_NUM; i++) {
			if (proc_id_tbl[i].path_id == path_id && proc_id_tbl[i].socket_id ==obj_index ) {
				ETHSOCKETCLI_DIAG("get mapping_id = %d\n",i);
				ethsocket_cli_mapping_id[path_id][obj_index]=i;
				break;
			}
		}
		if(ethsocket_cli_mapping_id[path_id][obj_index] == -1){
			printf("error, get mapping id fail !!\n");
		}

#if defined(__ECOS)
		cyg_thread_create(  gethsocket_cli_obj[path_id][obj_index].threadPriority,
							ethsocket_cli_proc,
							ethsocket_cli_mapping_id[path_id][obj_index],//obj_index,
							ethsocket_cli_thread_name[path_id][obj_index],//"ethsocketCli",
							&ethsocket_cli_stacks[path_id][obj_index][0],
							sizeof(ethsocket_cli_stacks[path_id][obj_index]),
							&ethsocket_cli_thread[path_id][obj_index],
							&ethsocket_cli_thread_object[path_id][obj_index]
 			);
		ETHSOCKETCLI_DIAG("ethsocket_cli_start resume thread\r\n");
		cyg_thread_resume(ethsocket_cli_thread[path_id][obj_index]);
#else
		snprintf(name, sizeof(name) - 1, "ethsoCliPrc_%d_%d",path_id,obj_index);

		ethsocket_cli_thread[path_id][obj_index]=vos_task_create(ethsocket_cli_proc,  (void *)&ethsocket_cli_mapping_id[path_id][obj_index], name,   gethsocket_cli_obj[path_id][obj_index].threadPriority,	STKSIZE_ETHCAM_CLI_PROC);
		vos_task_resume(ethsocket_cli_thread[path_id][obj_index]);
		//pthread_create(&ethsocket_cli_thread[path_id][obj_index], NULL , ethsocket_cli_proc , (void *)&ethsocket_cli_mapping_id);
		//pthread_create(&ethsocket_cli_thread[path_id][obj_index], NULL , ethsocket_cli_proc , (cyg_addrword_t)&obj_index);
#endif
		isStartClient[path_id][obj_index] = true;
	}
}

__externC void ethsocket_cli_stop(int path_id, int obj_index)
{
	FLGPTN uiFlag;
	int delay_cnt;
	int i;
    	printf("ethsocket_cli_stop , path_id=%d, obj_index=%d\r\n",path_id,obj_index);
	if (isStartClient[path_id][obj_index]) {
		isStopClient[path_id][obj_index] = true;
		//cyg_flag_maskbits(&ethsocket_cli_flag[path_id][obj_index], ~CYG_FLG_ETHSOCKETCLI_START);
		vos_flag_clr(ethsocket_cli_flag[path_id][obj_index], CYG_FLG_ETHSOCKETCLI_START);
		printf("ethsocket_cli_stop wait idle begin\r\n");
		//cyg_flag_wait(&ethsocket_cli_flag[path_id][obj_index], CYG_FLG_ETHSOCKETCLI_IDLE, CYG_FLAG_WAITMODE_AND);
		vos_flag_wait(&uiFlag, ethsocket_cli_flag[path_id][obj_index], CYG_FLG_ETHSOCKETCLI_IDLE, TWF_ORW);
		delay_cnt = 50;
		while ((gethsocket_cli_obj[path_id][obj_index].connect_socket != -1) && delay_cnt) {
			vos_util_delay_ms(10);
			delay_cnt --;
		}
		printf("ethsocket_cli_stop wait idle end\r\n");
		printf("ethsocket_cli_stop ->suspend thread\r\n");
		#if defined(__ECOS)
		cyg_thread_suspend(ethsocket_cli_thread[path_id][obj_index]);
		ETHSOCKETCLI_DIAG("ethsocket_cli_stop ->delete thread\r\n");
		cyg_thread_delete(ethsocket_cli_thread[path_id][obj_index]);
		#else
		//pthread_join(ethsocket_cli_thread[path_id][obj_index], NULL);
		if(gethsocket_cli_obj[path_id][obj_index].connect_socket != -1){
			printf("Destroy EthCamCliTsk[%d][%d]\r\n",path_id,obj_index);
			for (i = 0; i <= fdmax[path_id][obj_index]; i++) {
				if (FD_ISSET(i, &master[path_id][obj_index])) {
					close(i);
					printf("close socket %d\n",i);
				}
			}
			gethsocket_cli_obj[path_id][obj_index].connect_socket=-1;
			vos_task_destroy(ethsocket_cli_thread[path_id][obj_index]);
		}
		#endif
		//cyg_flag_destroy(&ethsocket_cli_flag[path_id][obj_index]);
		vos_flag_destroy(ethsocket_cli_flag[path_id][obj_index]);
		isStartClient[path_id][obj_index] = false;
		printf("ethsocket_cli_stop ->end\r\n");
	}
}

__externC int ethsocket_cli_send(int path_id, int obj_index, char* addr, int* size)
{
	int ret = -1;

	ETHSOCKETCLI_DIAG("ethsocket_cli_send %d\r\n", gethsocket_cli_obj[path_id][obj_index].connect_socket);
	if (gethsocket_cli_obj[path_id][obj_index].connect_socket) {
		ETHSOCKETCLI_DIAG("send %s %d client_socket %d\n", addr, *size, gethsocket_cli_obj[path_id][obj_index].connect_socket);

        	ETHSOCKETCLI_DIAG("ethsocket_cli_send ,obj_index=%d\r\n",obj_index);
		if (*size) {
			ret = sendall(gethsocket_cli_obj[path_id][obj_index].connect_socket, addr, size,obj_index);
		}
	} else {
		*size = 0;
	}
	return ret;
}

__externC void ethsocket_cli_install(ethsocket_cli_obj*  pethsocket_cli_obj, ethsocket_cli_obj*  pObj)
{
	//pethsocket_cli_obj->connect_socket = 0;
	pethsocket_cli_obj->connect_socket = -1;
	pethsocket_cli_obj->notify = pObj->notify;
	pethsocket_cli_obj->recv = pObj->recv;

	strcpy(pethsocket_cli_obj->svrIP, pObj->svrIP);

	if (!strlen(pObj->svrIP)) {
		printf("no svrIP: %s\n", pObj->svrIP);
	}

	// check if port number valid
	if ((!pObj->portNum) || (pObj->portNum > CYGNUM_MAX_PORT_NUM)) {
		pethsocket_cli_obj->portNum = ETHSOCKETCLI_SERVER_PORT;
		printf("error Port, set default port %d\r\n", ETHSOCKETCLI_SERVER_PORT);
	} else {
		pethsocket_cli_obj->portNum = pObj->portNum;
	}
  	if (!pObj->threadPriority)
    	{
        	pethsocket_cli_obj->threadPriority = ETHSOCKETCLI_THREAD_PRIORITY;
        	printf("error threadPriority, set default %d\r\n",ETHSOCKETCLI_THREAD_PRIORITY);
    	}
    	else
    	{
        	pethsocket_cli_obj->threadPriority = pObj->threadPriority;
    	}

	if (!pObj->sockbufSize) {
		pethsocket_cli_obj->sockbufSize = ETHSOCKETCLI_SOCK_BUF_SIZE;
		printf("error sockSendbufSize %d, set default %d \r\n", pObj->sockbufSize, ETHSOCKETCLI_SOCK_BUF_SIZE);
	} else {
		pethsocket_cli_obj->sockbufSize = pObj->sockbufSize;
	}

	if (!pObj->timeout) {
		pethsocket_cli_obj->timeout = ETHSOCKETCLI_TIMEOUT;
		printf("error timeout %d, set default %d \r\n", pObj->timeout, ETHSOCKETCLI_TIMEOUT);
	} else {
		pethsocket_cli_obj->timeout = pObj->timeout;
	}
}

__externC void ethsocket_cli_uninstall(int path_id, int obj_index, ethsocket_cli_obj*  pethsocket_cli_obj)
{
	ETHSOCKETCLI_DIAG("pethsocket_cli_obj %x uninstall\r\n", pethsocket_cli_obj);

	if(pethsocket_cli_obj){
		memset(pethsocket_cli_obj, 0, sizeof(ethsocket_cli_obj));
	}
	gethsocket_cli_obj[path_id][obj_index].connect_socket=-1;
}

static int sendall(int s, char* buf, int *len, int obj_index)
{
	int total = 0;
	int bytesleft = *len;
	int n = -1;

	ETHSOCKETCLI_DIAG("sendall[%d] %d bytes\n",obj_index,*len);
	while (total < *len) {
		n = send(s, buf+total, bytesleft, 0);
		//ETHSOCKETCLI_DIAG("send %d bytes\n",n);
		if (n == -1) {
			break;
		}
		total += n;
		bytesleft -= n;
	}
	*len = total;
	return (n==-1) ? -1 : 0;
}

static int recvall(int s, char* buf, int len, int isJustOne, int obj_index , int path_id)
{
	int total = 0;
	int bytesleft = len;
	int n = -1;
	int ret;
	int idletime;

	ETHSOCKETCLI_DIAG("recvall buf = 0x%x, len = %d bytes\n",(int)buf,len);
	while (total < len && (!isStopClient[path_id][obj_index])) {
		n = recv(s, buf+total, bytesleft, 0);
		if ( n == 0) {
			// client close the socket
			ETHSOCKETCLI_DIAG("recvall len = 0!!\n");
			return 0;
		}
		if ( n < 0 ) {
			if ( errno == EAGAIN ) {
				idletime = 0;
				while (!isStopClient[path_id][obj_index]) {
					/* wait idletime seconds for progress */
					fd_set rs;
					struct timeval tv;

					FD_ZERO( &rs );
					FD_SET( s, &rs );
					tv.tv_sec = 0;
					tv.tv_usec = CYGNUM_SELECT_TIMEOUT_USEC;
					ret = select( s + 1, &rs, NULL, NULL, &tv );
					if (ret == 0) {
						idletime++;
						//ETHSOCKETCLI_DIAG("idletime=%d\r\n",idletime);
						printf("idletime=%d\r\n",idletime);
						if (idletime >= ETHSOCKETCLI_IDLE_TIME) {
							perror("select\r\n");
							return -1;
						} else {
							continue;
						}
					} else if (ret <0) {
						perror("select\r\n");
						return -1;
					}
					if (FD_ISSET( s, &rs )) {
						//ETHSOCKETCLI_DIAG("recvall FD_ISSET\r\n");
						n = 0;
						break;
					}
				}
			}
		}
		if (n == -1) {
			break;
		}
		total += n;
		bytesleft -= n;
		if (isJustOne && total) {
			break;
		}
	}
	ETHSOCKETCLI_DIAG("recvall end, total=%d\n",total);
	return (n<0) ? -1 : total;
}
/* ----------------------------------------------------------------- */
/* end of ethsocket_cli.c                                            */

