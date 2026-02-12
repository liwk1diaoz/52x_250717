/* =================================================================
 *
 *      ethsocket_multi.c
 *
 *      A simple ethcam socket
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
//#include <dirent.h>
#include "kwrap/task.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/type.h"
#include <kwrap/util.h>


#include "EthCam/ethsocket.h"
#include "EthCam/EthCamSocket.h"
//#include "ethsocket_int.h"

#if defined(__ECOS)
#include <network.h>
#include <arpa/inet.h>
#include <cyg/ethsocket/ethsocket.h>
#elif defined(__LINUX_USER__)
#include <sys/types.h>
#include <sys/socket.h>
#include <UsockIpc/usocket.h>
#include <netdb.h>
#include <sys/ioctl.h>
#elif defined(__FREERTOS)
#include <kwrap/type.h>
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#include <FreeRTOS_POSIX/dirent.h>
#include <kwrap/semaphore.h>
#include <kwrap/task.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#define SOMAXCONN   4
#else
#include <sys/types.h>
#include <sys/socket.h>
//#include <UsockIpc/usocket.h>
#include <netdb.h>
//#include <sys/ioctl.h>
#include <pthread.h>
#endif

#if defined(__FREERTOS)
#define IOCTL                  lwip_ioctl
#define SOCKET_CLOSE           lwip_close
#else
#define IOCTL                  ioctl
#define SOCKET_CLOSE           close
#endif

#ifndef __USE_IPC
#define __USE_IPC              0
#endif

#if (__USE_IPC)
#include "ethsocket_ipc.h"
#endif

#define ETHSOCKET_SERVER_PORT               3333
#define ETHSOCKET_THREAD_PRIORITY           6
#define ETHSOCKET_THREAD_STACK_SIZE         5120
#if (__USE_IPC)
#define ETHSOCKET_SERVER_BUFFER_SIZE        ETHSOCKIPC_TRANSFER_BUF_SIZE
#else
#define ETHSOCKET_SERVER_BUFFER_SIZE        (2 * 1024)
#endif

#define STKSIZE_ETHCAM_PROC           (8*1024)
#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)

/* ================================================================= */
#if 0
#if defined(__ECOS)
#define ETHSOCKET_DIAG                      diag_printf
#else
#define ETHSOCKET_DIAG                      printf
#endif
#else
#define ETHSOCKET_DIAG(...)
#endif


/* ================================================================= */
/* Server socket address and file descriptor.
 */

#define CYGNUM_MAX_PORT_NUM                 65535

#define ETHSOCKET_SOCK_BUF_SIZE             (8 * 1024)

#define CYGNUM_SELECT_TIMEOUT_USEC          500000

#define ETHSOCKET_SERVER_IDLE_TIME          1  // retry 20


// flag define

#define CYG_FLG_ETHSOCKET_START             0x01

#define CYG_FLG_ETHSOCKET_IDLE              0x80


// HTTP animation mode
//#define ANIMATION_MODE_SERVER_PUSH                      0
//#define ANIMATION_MODE_CLIENT_PULL_AUTO_REFRESH         1
//#define ANIMATION_MODE_CLIENT_PULL_MANUAL_REFRESH       2


#define MAX_ETHSOCKET_NUMBER                3

static struct addrinfo server_address[MAX_ETHSOCKET_NUMBER];

static int server_socket[MAX_ETHSOCKET_NUMBER] = { -1, -1, -1};

static ethsocket_install_obj_multi ethsocket_obj_list[MAX_ETHSOCKET_NUMBER] = {0};

//static char* g_Sendbuff = NULL;

#if defined(__ECOS)
static cyg_flag_t ethsocket_flag[MAX_ETHSOCKET_NUMBER];
#else
//static volatile int ethsocket_flag[MAX_ETHSOCKET_NUMBER];
static ID ethsocket_flag[MAX_ETHSOCKET_NUMBER];
#endif

static bool isServerStart[MAX_ETHSOCKET_NUMBER] = {false}, isStopServer[MAX_ETHSOCKET_NUMBER] = {false};
static int fdmax[MAX_ETHSOCKET_NUMBER];
static fd_set master[MAX_ETHSOCKET_NUMBER];  // master file descriptor list

/* ================================================================= */
/* Thread stacks, etc.
 */

#if defined(__ECOS)
static cyg_uint8 ethsocket_stacks[MAX_ETHSOCKET_NUMBER][ETHSOCKET_SERVER_BUFFER_SIZE + ETHSOCKET_THREAD_STACK_SIZE] = {0};
#endif

#if defined(__ECOS)
static cyg_handle_t ethsocket_thread[MAX_ETHSOCKET_NUMBER];
static cyg_thread   ethsocket_thread_object[MAX_ETHSOCKET_NUMBER];
static char ethsocket_thread_name[MAX_ETHSOCKET_NUMBER][15]={"ethsocketm0", "ethsocketm1", "ethsocketm2"};
#else
//static pthread_t    ethsocket_thread[MAX_ETHSOCKET_NUMBER];
static THREAD_HANDLE ethsocket_thread[MAX_ETHSOCKET_NUMBER];
#endif
int EthSocketDispIsEnable=0;

static int recvall_multi(int s, char *buf, int len, int isJustOne, int obj_index);
static int sendall_multi(int s, char *buf, int *len, int obj_index);

/* ================================================================= */
/* Main processing function                                          */
/*                                                                   */
/* Reads the HTTP header, look it up in the table and calls the      */
/* handler.                                                          */

// this function will return true, if the connection is closed.
static cyg_bool ethsocket_process_multi(int client_socket, struct sockaddr *client_address, int obj_index)
{
    #if _TODO
	int calen = sizeof(*client_address);
	char name[64];
	char port[10];
    #endif
	char request[ETHSOCKET_SERVER_BUFFER_SIZE];
	int  ret = 1;

    #if _TODO
	getnameinfo(client_address, calen, name, sizeof(name),
				port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
	ETHSOCKET_DIAG("Connection from %s[%s]\n", name, port);
    #endif
	//ret = recv(client_socket,request, sizeof(request), 0);
	//ethsocket_set_sock_nonblock(client_socket);
	memset(request, 0, ETHSOCKET_SERVER_BUFFER_SIZE);
	ret = recvall_multi(client_socket, request, sizeof(request), true, obj_index);

	// the connection is closed by client, so we close the client_socket
	if (ret == 0) {
		#if _TODO
		ETHSOCKET_DIAG("Connection closed by client %s[%s]\n", name, port);
		#else
		ETHSOCKET_DIAG("Connection closed by client %d\n",client_socket);
		#endif
		SOCKET_CLOSE(client_socket);
		return true;
	}
	if (ret == -1) {
		perror("recv time out");

		SOCKET_CLOSE(client_socket);
#if 0
		sprintf(request, "hello\r\n");
		len = strlen(request);
		ETHSOCKET_DIAG("send %s %d\n", request, len);
		ret = sendall(client_socket, request, &len);
#endif
		return true;
	} else {
#if 0
		char *pch = 0;
		int len = 0;
		ETHSOCKET_DIAG("ret %d\n", ret);

		pch = strchr(request, '\n');
		len = pch - request + 1;
		if (len > ret) {
			len = ret;
		}
		ETHSOCKET_DIAG("send back %s %d\n", request, len);
		//ETHSOCKET_DIAG("%x %x %x %x %x %x\n", request[0], request[1], request[2], request[3], request[4], request[5], request[6]);
		ret = sendall(client_socket, request, &len);
#else
		if (ethsocket_obj_list[obj_index].recv) {
			ethsocket_obj_list[obj_index].recv(request, ret, obj_index);
		}
#endif
		return false;
	}
}
#include <netinet/tcp.h> /* for TCP_NODELAY */
static void ethsocket_server_multi(int obj_index)
{
	int client_socket,  flag;
	#if 1//defined(__ECOS)
	int  sock_buf_size = ETHSOCKET_SOCK_BUF_SIZE;
	#endif
	struct sockaddr_in client_address;
	socklen_t calen = sizeof(client_address);
	fd_set readfds; // temp file descriptor list for select()
	int i;
	int ret;
	struct timeval tv;
	ethsocket_install_obj_multi *pethsocket_obj;

	pethsocket_obj = &ethsocket_obj_list[obj_index];

	#if 1//defined(__ECOS)
	sock_buf_size = pethsocket_obj->sockbufSize;
	ETHSOCKET_DIAG("ethsocket_server[%d] , sock_buf_size  =%d\r\n", obj_index, sock_buf_size);
	#endif

	// clear the set
	FD_ZERO(&master[obj_index]);
	FD_ZERO(&readfds);
	// add server socket to the set
	FD_SET(server_socket[obj_index], &master[obj_index]);

	fdmax[obj_index] = server_socket[obj_index];
	// set timeout to 100 ms
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	printf("ethsocket_server_multi[%d] , ethsocket_flag  =%d\r\n", obj_index, ethsocket_flag[obj_index]);
	fflush(stdout);

	//cyg_flag_maskbits(&ethsocket_flag[obj_index], 0);
	//cyg_flag_setbits(&ethsocket_flag[obj_index], CYG_FLG_ETHSOCKET_START);
	vos_flag_clr(ethsocket_flag[obj_index], 0xFFFFFFFF);
	vos_flag_set(ethsocket_flag[obj_index], CYG_FLG_ETHSOCKET_START);

	while (!isStopServer[obj_index]) {

#if 0
		FD_ZERO(&master[obj_index]);
		FD_ZERO(&readfds);
		// add server socket to the set
		FD_SET(fdmax[obj_index], &master[obj_index]);
#endif
		tv.tv_sec = 0;
		tv.tv_usec = 100000;

		// copy it
		readfds = master[obj_index];
		ret = select(fdmax[obj_index] + 1, &readfds, NULL, NULL, &tv);
		switch (ret) {
		case -1:
			perror("select");
			break;
		case 0:
			//ETHSOCKET_DIAG("select timeout\n");
			break;
		default:
			// run throuth the existing connections looking for data to read
			ETHSOCKET_DIAG("fdmax[%d] %d  readfds 0x%x\n", obj_index, fdmax[obj_index], readfds);
			for (i = 0; i <= fdmax[obj_index]; i++) {
				// one data coming
				if (FD_ISSET(i, &readfds)) {
					// handle new connection
					if (i == server_socket[obj_index]) {
						//if(obj_index==ETHSOCKIPC_ID_1){
						if(obj_index==1){
							EthSocketDispIsEnable=1;
						}
						ETHSOCKET_DIAG("accept[%d] begin %d\n", obj_index, i);
						client_socket = accept(server_socket[obj_index], (struct sockaddr *)&client_address, &calen);
						ETHSOCKET_DIAG("accept[%d] end %d, client_socket=%d\n", obj_index, i, client_socket);

						#if 1//defined(__ECOS)
						ret = setsockopt(client_socket, SOL_SOCKET, SO_SNDBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
						if (ret == -1) {
							printf("Couldn't setsockopt(SO_SNDBUF)[%d]\n", obj_index);
						}
						ret = setsockopt(client_socket, SOL_SOCKET, SO_RCVBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
						if (ret == -1) {
							printf("Couldn't setsockopt(SO_RCVBUF)[%d]\n", obj_index);
						}

						flag = 1;
						ret = setsockopt(client_socket, SOL_SOCKET, SO_OOBINLINE, (char *)&flag, sizeof(int));
						if (ret == -1) {
							printf("Couldn't setsockopt(SO_OOBINLINE)[%d]\n", obj_index);
						}
						#endif

						#if 1
						flag = 1;
						ret = setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
						if (ret == -1) {
							printf("Couldn't setsockopt(TCP_NODELAY)\n");
						}else{
							printf("multi(%d) setsockopt(TCP_NODELAY)\n",obj_index);
						}
						#else
							flag = 1;
							ret = setsockopt(client_socket, IPPROTO_TCP, TCP_NOPUSH, (char *)&flag, sizeof(flag));
							if (ret == -1) {
								printf("Couldn't setsockopt(TCP_NOPUSH)\n");
							}else{
								printf("multi(%d) setsockopt(TCP_NOPUSH)\n",obj_index);
							}
						#endif
						flag = 1;
						// set socket non-block
						if (IOCTL(client_socket, FIONBIO, &flag) != 0) {
							printf("%s: IOCTL errno = %d\r\n", __func__, errno);
						}

						ETHSOCKET_DIAG("client_address[%d] %x \r\n", obj_index, client_address.sin_addr.s_addr);

						// client has request
						if (pethsocket_obj->notify) {
							pethsocket_obj->notify(CYG_ETHSOCKET_STATUS_CLIENT_CONNECT, client_address.sin_addr.s_addr, obj_index);
						}

						if (client_socket != -1) {
							// add to master set
							ETHSOCKET_DIAG("FD_SET[%d] %d\n", obj_index, client_socket);
							FD_SET(client_socket, &master[obj_index]);
							// keep track of the max
							pethsocket_obj->client_socket = client_socket;
							if (client_socket > fdmax[obj_index]) {
								fdmax[obj_index] = client_socket;
							}

							ETHSOCKET_DIAG("client_socket[%d] %x %x master 0x%x ,fdmax 0x%x\n", obj_index, pethsocket_obj, pethsocket_obj->client_socket, master[obj_index], fdmax[obj_index]);

						}
					}
					// handle data from a client
					else {
						ETHSOCKET_DIAG("handle data from a client[%d] %d\n", obj_index, i);
						// client has request
						{
							if (pethsocket_obj->notify) {
								//pethsocket_obj->notify(CYG_ETHSOCKET_STATUS_CLIENT_REQUEST, 0, obj_index);
							}
							ret = ethsocket_process_multi(i, (struct sockaddr *)&client_address, obj_index);
							// if connection close
							if (ret) {
								ETHSOCKET_DIAG("FD_CLR %d\n", i);
								FD_CLR(i, &master[obj_index]);
								pethsocket_obj->client_socket = 0;
								ETHSOCKET_DIAG("disconnect %x %x\n", pethsocket_obj, pethsocket_obj->client_socket);

								if (pethsocket_obj->notify) {
									pethsocket_obj->notify(CYG_ETHSOCKET_STATUS_CLIENT_DISCONNECT, 0, obj_index);
								}

							}
						}
					}
				}
			}
		}
	}
	if(EthCamSocket_GetEthLinkStatus(0)==ETHCAM_LINK_DOWN){
		printf("[%d]ethsocket_server linkdown clis=%d\n",obj_index,pethsocket_obj->client_socket);
		if (FD_ISSET(pethsocket_obj->client_socket, &master[obj_index])) {
			FD_CLR(pethsocket_obj->client_socket, &master[obj_index]);
		}
	}
	// close all the opened sockets
	for (i = 0; i <= fdmax[obj_index]; i++) {
		if (FD_ISSET(i, &master[obj_index])) {
			SOCKET_CLOSE(i);
			ETHSOCKET_DIAG("close socket %d\n", i);
		}
	}
//ethsocket_server_exit:
	// reset the server socket value
	server_socket[obj_index] = -1;

#if 0
	// free allocated memory
	if (g_Sendbuff) {
		free(g_Sendbuff);
		ETHSOCKET_DIAG("free g_Sendbuff = 0x%x\r\n", g_Sendbuff);
		g_Sendbuff = NULL;
	}
#endif
	//cyg_flag_setbits(&ethsocket_flag[obj_index], CYG_FLG_ETHSOCKET_IDLE);
	vos_flag_set(ethsocket_flag[obj_index],CYG_FLG_ETHSOCKET_IDLE);
}

/* ================================================================= */
/* Initialization thread
 *
 * Optionally delay for a time before getting the network
 * running. Then create and bind the server socket and put it into
 * listen mode. Spawn any further server threads, then enter server
 * mode.
 */

#if defined(__ECOS)
static void ethsocket_init_multi(cyg_addrword_t arg)
#else
//static void *ethsocket_init_multi(void *arg)
static THREAD_RETTYPE ethsocket_init_multi(void *arg)
#endif
{
	int err = 0, flag;
#if (defined(__ECOS)|| defined(__FREERTOS))
	int obj_index = (int) arg;
#else
	int* obj_index_arg =  (int*) arg;
	int obj_index = (int) *obj_index_arg;

#endif

	struct addrinfo *res;
	char   portNumStr[10]={0};
	int    portNum = ethsocket_obj_list[obj_index].portNum;
	int    sock_buf_size = ETHSOCKET_SOCK_BUF_SIZE;

	//printf("ethsocket_init_multi[%d]\n", obj_index);
	//fflush(stdout);
	THREAD_ENTRY();

	memset(&server_address[obj_index], 0, sizeof(server_address[obj_index]));
	//server_address.ai_family = AF_UNSPEC; // user IPv4 or IPv6 whichever
	server_address[obj_index].ai_family = AF_INET;
	server_address[obj_index].ai_socktype = SOCK_STREAM;
	server_address[obj_index].ai_flags = AI_PASSIVE; // fill in my IP for me

	// check if port number valid
	if (portNum > CYGNUM_MAX_PORT_NUM) {
		printf("error port number %d\r\n", portNum);
		//return DUMMY_NULL;
		goto PROC_END;
	}

	sprintf(portNumStr, "%d", portNum);
	err = getaddrinfo(NULL, portNumStr, &server_address[obj_index], &res);
	if (err != 0) {
		#if _TODO
		printf("err_multi:getaddrinfo: %s\n", gai_strerror(err));
		#else
		printf("err_multi:getaddrinfo: %d\n", err);
		#endif
		//return DUMMY_NULL;
		goto PROC_END;
	}

	/* Get the network going. This is benign if the application has
	 * already done this.
	 */
	//ETHSOCKET_DIAG("init_all_network_interfaces\r\n");
	//init_all_network_interfaces();

	/* Create and bind the server socket.
	 */
	ETHSOCKET_DIAG("create socket[%d]\r\n", obj_index);
	server_socket[obj_index] = socket(res->ai_family, res->ai_socktype, res->ai_protocol);


	ETHSOCKET_DIAG("server_socket[%d] = %d\r\n", obj_index, server_socket[obj_index]);
	CYG_ASSERT(server_socket[obj_index] >= 0, "Socket create failed");

	/* Disable the Nagle (TCP No Delay) Algorithm */
#if 0
	flag = 1;
	err = setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
	if (err == -1) {
		printf("Couldn't setsockopt(TCP_NODELAY)\n");
	}
#endif
#if 1
	flag = 1;
	err = setsockopt(server_socket[obj_index], SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_REUSEADDR)[%d]\n", obj_index);
	}

#if defined(__ECOS)
	err = setsockopt(server_socket[obj_index], SOL_SOCKET, SO_REUSEPORT, (char *)&flag, sizeof(flag));
#else
    #if defined (SO_REUSEPORT)
	err = setsockopt(server_socket[obj_index], SOL_SOCKET, SO_REUSEPORT, (char *)&flag, sizeof(flag));
#endif
#endif
	if (err == -1) {
		printf("Couldn't setsockopt(SO_REUSEPORT)[%d]\n", obj_index);
	}

	#if 1//defined(__ECOS)

	sock_buf_size = ethsocket_obj_list[obj_index].sockbufSize;

	err = setsockopt(server_socket[obj_index], SOL_SOCKET, SO_SNDBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_SNDBUF)[%d]\n", obj_index);
	}

	err = setsockopt(server_socket[obj_index], SOL_SOCKET, SO_RCVBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_RCVBUF)[%d]\n", obj_index);
	}
	#endif

#endif

	ETHSOCKET_DIAG("bind %d\r\n", portNum);
	err = bind(server_socket[obj_index], res->ai_addr, res->ai_addrlen);
	CYG_ASSERT(err == 0, "bind() returned error");

	ETHSOCKET_DIAG("listen\r\n");
	err = listen(server_socket[obj_index], SOMAXCONN);
	CYG_ASSERT(err == 0, "listen() returned error");

	freeaddrinfo(res);   // free the linked list

	/* Now go be a server ourself.
	 */
	ethsocket_server_multi(obj_index);

PROC_END:
	THREAD_RETURN(0);

	//return DUMMY_NULL;
}

/* ================================================================= */
/* System initializer
 *
 * This is called from the static constructor in init.cxx. It spawns
 * the main server thread and makes it ready to run. It can also be
 * called explicitly by the application if the auto start option is
 * disabled.
 */
int open_multi_obj_index;
__externC void ethsocket_open_multi(int obj_index)
{
	int ret =0;
	T_CFLG cflg ;
	char name[40]={0};
	//printf("ethsocket_open_multi, obj_index=%d\n",obj_index);
	if (!isServerStart[obj_index]) {
		isStopServer[obj_index] = false;
		//cyg_flag_init(&ethsocket_flag[obj_index]);
		snprintf(name, sizeof(name) - 1, "ethsoFlagMul_%d",obj_index);
		vos_flag_create(&ethsocket_flag[obj_index], &cflg, name);
		vos_flag_clr(ethsocket_flag[obj_index], 0xFFFFFFFF);


		//printf("ethsocket_open_multi[%d] , ethsocket_flag  =%d\r\n", obj_index, ethsocket_flag[obj_index]);

		ETHSOCKET_DIAG("ethsocket_startup create thread\r\n");
#if defined(__ECOS)
		ret =cyg_thread_create(ethsocket_obj_list[obj_index].threadPriority,
						  ethsocket_init_multi,
						  obj_index,
						  ethsocket_thread_name[obj_index],//"user socket",
						  &ethsocket_stacks[obj_index][0],
						  sizeof(ethsocket_stacks[obj_index]),
						  &ethsocket_thread[obj_index],
						  &ethsocket_thread_object[obj_index]
						 );
		ETHSOCKET_DIAG("ethsocket_startup resume thread\r\n");
		cyg_thread_resume(ethsocket_thread[obj_index]);
#else
		//ret =pthread_create(&ethsocket_thread[obj_index], NULL, ethsocket_init_multi, (void *)obj_index);

		snprintf(name, sizeof(name) - 1, "ethsoPrcMul_%d",obj_index);
		open_multi_obj_index =obj_index;
		int* param = &open_multi_obj_index;


		//if((ethsocket_thread[obj_index]=vos_task_create(ethsocket_init_multi,  (void *)&obj_index, name,   ethsocket_obj_list[obj_index].threadPriority,	STKSIZE_ETHCAM_PROC))==0){
		if((ethsocket_thread[obj_index]=vos_task_create(ethsocket_init_multi,  param, name,   ethsocket_obj_list[obj_index].threadPriority,	STKSIZE_ETHCAM_PROC))==0){
			printf("ethsocket_open_multi[%d], vos_task_create fail\n", obj_index);
		}else{
			vos_task_resume(ethsocket_thread[obj_index]);
		}
#endif
		if (ret < 0) {
			printf("create feed_bs_thread failed, idx=%d\n",obj_index);
		} else {
			isServerStart[obj_index] = true;
		}
	}
}

__externC void ethsocket_uninstall_multi(int obj_index)
{
	memset(&ethsocket_obj_list[obj_index], 0, sizeof(ethsocket_install_obj_multi));
}

__externC void ethsocket_close_multi(int obj_index)
{
	FLGPTN uiFlag;
	int delay_cnt;
	int i;

	if (isServerStart[obj_index]) {
		//ethsocket_uninstall_multi(obj_index);
		isStopServer[obj_index] = true;
		//cyg_flag_maskbits(&ethsocket_flag[obj_index], ~CYG_FLG_ETHSOCKET_START);
		vos_flag_clr(ethsocket_flag[obj_index], CYG_FLG_ETHSOCKET_START);
		ETHSOCKET_DIAG("ethsocket_stop[%d] wait idle begin\r\n", obj_index);
		//cyg_flag_waitms(&ethsocket_flag[obj_index], CYG_FLG_ETHSOCKET_IDLE, CYG_FLAG_WAITMODE_AND);
		vos_flag_wait(&uiFlag, ethsocket_flag[obj_index], CYG_FLG_ETHSOCKET_IDLE, TWF_ORW);

		delay_cnt = 50;
		while ((server_socket[obj_index] != -1) && delay_cnt) {
			vos_util_delay_ms(10);
			delay_cnt --;
		}
		ETHSOCKET_DIAG("ethsocket_stop[%d] wait idle end\r\n", obj_index);
		ETHSOCKET_DIAG("ethsocket_stop[%d] ->suspend thread\r\n", obj_index);
#if defined(__ECOS)
		cyg_thread_suspend(ethsocket_thread[obj_index]);
		ETHSOCKET_DIAG("ethsocket_stop[%d] ->delete thread\r\n", obj_index);
		cyg_thread_delete(ethsocket_thread[obj_index]);
#else
		//pthread_join(ethsocket_thread[obj_index], NULL);
		if(server_socket[obj_index]!= -1){
			printf("Destroy EthCamMultiTsk\r\n");
			for (i = 0; i <= fdmax[obj_index]; i++) {
				if (FD_ISSET(i, &master[obj_index])) {
					SOCKET_CLOSE(i);
					ETHSOCKET_DIAG("close socket %d\n", i);
				}
			}
			server_socket[obj_index]=-1;
			vos_task_destroy(ethsocket_thread[obj_index]);
		}

#endif
		//cyg_flag_destroy(&ethsocket_flag[obj_index]);
		vos_flag_destroy(ethsocket_flag[obj_index]);

		isServerStart[obj_index] = false;
		ETHSOCKET_DIAG("ethsocket_stop[%d] ->end\r\n", obj_index);
	}
}


__externC int ethsocket_send_multi(char *buf, int *size, int obj_index)
{
	int ret = -1;
	int i = 0;
	int bSend=0;
	ethsocket_install_obj_multi *pethsocket_obj;
	pethsocket_obj = &ethsocket_obj_list[obj_index];
	ETHSOCKET_DIAG("ethsocket_send[%d] client_socket %d fdmax 0x%x\r\n", obj_index, pethsocket_obj->client_socket, fdmax[obj_index]);
	if ((buf) && (*size)) {
		ETHSOCKET_DIAG("send %s %d client_socket %d master %x\n", buf, *size, pethsocket_obj->client_socket, master[obj_index]);
		for (i = 0; i <= fdmax[obj_index]; i++) {
			//i=  fdmax[obj_index];
			if (FD_ISSET(i, &master[obj_index])) {
				if (i != server_socket[obj_index]) {
					ETHSOCKET_DIAG("send %s %d client_socket %d\n", buf, *size, i);
					//printf("ethsocket_send_multi[%d] %d bytes\n", obj_index, *size);
					ret = sendall_multi(i, buf, size, obj_index);
					bSend=1;
					if(ret!=0){
						//printf("error -1, ethsocket_send_multi[%d] %d bytes\n", obj_index, *size);
					}
					pethsocket_obj->client_socket = i;
				}
			}
		}
	}
	if(ret!=0 && bSend==0){
		//printf("error -1, ethsocket_send_multi[%d] %d bytes, bSend=%d\n", obj_index, *size, bSend);
		*size=0; //for uitron retry
	}
	return ret;
}

#if 0
__externC int ethsocket_recv(ethsocket_install_obj *pObj, char *addr, int *size)
{
	int ret = 0;
	ethsocket_install_obj *pethsocket_obj;
	pethsocket_obj = &ethsocket_obj;

	ETHSOCKET_DIAG("ethsocket_recv\r\n");
#if 0
	if (pethsocket_obj->client_socket) {
		ETHSOCKET_DIAG("recv %s %d client_socket %d\n", addr, *size, pethsocket_obj->client_socket);
		if (*size) {
			ret = recvall(pethsocket_obj->client_socket, addr, size, true);
		}
	}
#endif
	return ret;
}
#endif

__externC int ethsocket_install_multi(int obj_index, ethsocket_install_obj_multi  *pObj)
{
	#if 0//mark for ETHSOCKIPC_ID_0  + ETHSOCKIPC_ID_2, no ETHSOCKIPC_ID_1
	int i;
	int obj_index = -1;
	//i =1 in 96660,reserved 0 for origin ethsocket
	for (i = 1 ; i < MAX_ETHSOCKET_NUMBER; i++) {
		if (ethsocket_obj_list[i].occupy == 0) {
			obj_index = i;
			break;

		}
	}
	if (obj_index == -1) {
		printf("ethsocket can not create anymore, please kill other ethsocket first (MAX_NUM:%d)\r\n", MAX_ETHSOCKET_NUMBER);
		return -1;
	}
	#else
	if (ethsocket_obj_list[obj_index].occupy != 0) {
		printf("ethsocket can not create anymore, please kill other ethsocket first (MAX_NUM:%d)\r\n", MAX_ETHSOCKET_NUMBER);
		return -1;
	}
	#endif
	ethsocket_obj_list[obj_index].occupy = 1;
	ethsocket_obj_list[obj_index].client_socket = 0;
	ethsocket_obj_list[obj_index].notify = pObj->notify;
	ethsocket_obj_list[obj_index].recv = pObj->recv;
	if (!pObj->portNum) {
		ethsocket_obj_list[obj_index].portNum = ETHSOCKET_SERVER_PORT;
		printf("error Port, set default port %d\r\n", ETHSOCKET_SERVER_PORT);
	} else {
		ethsocket_obj_list[obj_index].portNum = pObj->portNum;
	}
	if (!pObj->threadPriority) {
		ethsocket_obj_list[obj_index].threadPriority = ETHSOCKET_THREAD_PRIORITY;
		printf("error threadPriority, set default %d\r\n", ETHSOCKET_THREAD_PRIORITY);
	} else {
		ethsocket_obj_list[obj_index].threadPriority = pObj->threadPriority;
	}
	if (!pObj->sockbufSize) {
		ethsocket_obj_list[obj_index].sockbufSize = ETHSOCKET_SOCK_BUF_SIZE;
		printf("error sockSendbufSize %d, set default %d \r\n", pObj->sockbufSize, ETHSOCKET_SOCK_BUF_SIZE);
	} else {
		ethsocket_obj_list[obj_index].sockbufSize = pObj->sockbufSize;
	}
	return obj_index;

}

__externC int ethsocket_get_obj_index(int port)
{

	if (port > 65535 || port <= 0) {
		printf("port error port=%d\r\n", port);
		return -1;
	}
	int i;
	for (i = 0; i < MAX_ETHSOCKET_NUMBER ; i++) {
		if (ethsocket_obj_list[i].occupy == 1 && ethsocket_obj_list[i].portNum == port) {
			return i;

		}
	}
	printf("can not find ethsocket object port=%d\r\n", port);
	return -1;
}


static int sendall_multi(int s, char *buf, int *len, int obj_index)
{
	int total = 0;
	int bytesleft = *len;
	int n = -1;


	ETHSOCKET_DIAG("sendall_multi[%d] %d bytes\n", obj_index, *len);
	while (total < *len) {
		n = send(s, buf + total, bytesleft, 0);
		//ETHSOCKET_DIAG("send %d bytes\n",n);
		//if(obj_index==1){
		//printf("send %d bytes\n",n);
		//fflush(stdout);
		//}
		if (n == -1) {
			//printf("error -1, sendall_multi[%d] %d bytes\n", obj_index, *len);
			break;
		}
		total += n;
		bytesleft -= n;
	}
	*len = total;
	return n == -1 ? -1 : 0;
}

static int recvall_multi(int s, char *buf, int len, int isJustOne, int obj_index)
{
	int total = 0;
	int bytesleft = len;
	int n = -1;
	int ret;
	int idletime;

	ETHSOCKET_DIAG("recvall buf = 0x%x, len = %d bytes\n", (int)buf, len);
	while (total < len && (!isStopServer[obj_index])) {
		n = recv(s, buf + total, bytesleft, 0);
		if (n == 0) {
			// client close the socket
			return 0;
		}
		if (n < 0) {
			if (errno == EAGAIN) {
				idletime = 0;
				while (!isStopServer[obj_index]) {
					/* wait idletime seconds for progress */
					fd_set rs;
					struct timeval tv;

					FD_ZERO(&rs);
					FD_SET(s, &rs);
					tv.tv_sec = 0;
					tv.tv_usec = CYGNUM_SELECT_TIMEOUT_USEC;
					ret = select(s + 1, &rs, NULL, NULL, &tv);
					if (ret == 0) {
						idletime++;
						//ETHSOCKET_DIAG("idletime=%d\r\n",idletime);
						printf("idletime=%d\r\n", idletime);
						if (idletime >= ETHSOCKET_SERVER_IDLE_TIME) {
							perror("select\r\n");
							return -1;
						} else {
						#if 0 //[141338]coverity,because USOCKET_SERVER_IDLE_TIME is 1
							continue;
						#endif
						}
					} else if (ret < 0) {
						perror("select\r\n");
						return -1;
					}
					if (FD_ISSET(s, &rs)) {
						//ETHSOCKET_DIAG("recvall FD_ISSET\r\n");
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
	ETHSOCKET_DIAG("recvall end\n");
	return n < 0 ? -1 : total;
}
/* ----------------------------------------------------------------- */
/* end of ethsocket_multi.c                                          */

