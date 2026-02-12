/* =================================================================
 *
 *      ethsocket.h
 *
 *      A simple user socket
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
#define ETHSOCKET_SERVER_BUFFER_SIZE        1024
#endif

#define STKSIZE_ETHCAM_PROC           4096

/* ================================================================= */

#if 0
#if defined(__ECOS)
#define ETHSOCKET_DIAG diag_printf
#else
#define ETHSOCKET_DIAG printf
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

static struct addrinfo server_address;

static int server_socket = -1;

static ethsocket_install_obj ethsocket_obj = {0};

//static char* g_Sendbuff = NULL;

#if defined(__ECOS)
static cyg_flag_t ethsocket_flag;
#else
//static volatile int ethsocket_flag;
static ID ethsocket_flag;
#endif

static bool isServerStart = false, isStopServer = false;
static int fdmax;
static fd_set master;  // master file descriptor list

/* ================================================================= */
/* Thread stacks, etc.
 */

#if defined(__ECOS)
static cyg_uint8 ethsocket_stacks[ETHSOCKET_SERVER_BUFFER_SIZE + ETHSOCKET_THREAD_STACK_SIZE] = {0};
#endif

#if defined(__ECOS)
static cyg_handle_t ethsocket_thread;
static cyg_thread   ethsocket_thread_object;
#else
//static pthread_t    ethsocket_thread;
static THREAD_HANDLE ethsocket_thread;
#endif

static int recvall(int s, char *buf, int len, int isJustOne);
static int sendall(int s, char *buf, int *len);

/* ================================================================= */
/* Main processing function                                          */
/*                                                                   */
/* Reads the HTTP header, look it up in the table and calls the      */
/* handler.                                                          */

// this function will return true, if the connection is closed.
static cyg_bool ethsocket_process(int client_socket, struct sockaddr *client_address)
{
#if !defined(__FREERTOS)
	int calen = sizeof(*client_address);
	char name[64];
	char port[10];
#endif
	int  ret = 1;
#if 1//!defined(__USE_IPC)
	char request[ETHSOCKET_SERVER_BUFFER_SIZE];
#else //mulit socket should copy in the recv callback
	char *request = 0 ;
	extern unsigned int ethsocket_ipc_GetSndAddr(void);

	ETHSOCKET_TRANSFER_PARAM *pParam = (ETHSOCKET_TRANSFER_PARAM *)ethsocket_ipc_GetSndAddr();
	request = pParam->buf;
#endif
#if !defined(__FREERTOS)

	getnameinfo(client_address, calen, name, sizeof(name),
				port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
	ETHSOCKET_DIAG("Connection from %s[%s]\n", name, port);
#endif
	//ret = recv(client_socket,request, sizeof(request), 0);
	//ethsocket_set_sock_nonblock(client_socket);
	memset(request, 0, ETHSOCKET_SERVER_BUFFER_SIZE);
	ret = recvall(client_socket, request, ETHSOCKET_SERVER_BUFFER_SIZE, true);

	// the connection is closed by client, so we close the client_socket
	if (ret == 0) {
	#if !defined(__FREERTOS)
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
		if (ethsocket_obj.recv) {
			ethsocket_obj.recv(request, ret);
		}
#endif
		return false;
	}
}
#include <netinet/tcp.h> /* for TCP_NODELAY */

static void ethsocket_server(cyg_addrword_t arg)
{
	int client_socket, flag;
#if defined(__ECOS)
	int sock_buf_size = ETHSOCKET_SOCK_BUF_SIZE;
#endif
	struct sockaddr_in client_address;
	socklen_t calen = sizeof(client_address);
	fd_set readfds; // temp file descriptor list for select()
	int i;
	int ret;
	struct timeval tv;
	ethsocket_install_obj *pethsocket_obj;

	pethsocket_obj = &ethsocket_obj;
#if defined(__ECOS)
	sock_buf_size = pethsocket_obj->sockbufSize;
	ETHSOCKET_DIAG("ethsocket_server , sock_buf_size  =%d\r\n", sock_buf_size);
#endif

	// clear the set
	FD_ZERO(&master);
	FD_ZERO(&readfds);
	// add server socket to the set
	FD_SET(server_socket, &master);

	fdmax = server_socket;
	// set timeout to 100 ms
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	//cyg_flag_maskbits(&ethsocket_flag, 0);
	//cyg_flag_setbits(&ethsocket_flag, CYG_FLG_ETHSOCKET_START);
	vos_flag_clr(ethsocket_flag, 0xFFFFFFFF);
	vos_flag_set(ethsocket_flag, CYG_FLG_ETHSOCKET_START);
	while (!isStopServer) {
		/*
		cyg_flag_setbits(&ethsocket_flag, CYG_FLG_ETHSOCKET_IDLE);
		ETHSOCKET_DIAG("wait start begin flag vlaue =0x%x\n", ethsocket_flag.value);
		cyg_flag_wait(&ethsocket_flag, CYG_FLG_ETHSOCKET_START, CYG_FLAG_WAITMODE_AND);
		ETHSOCKET_DIAG("wait start end, flag vlaue =0x%x\n", ethsocket_flag.value);
		cyg_flag_maskbits(&ethsocket_flag, ~CYG_FLG_ETHSOCKET_IDLE);
		ETHSOCKET_DIAG("flag vlaue =0x%x\n", ethsocket_flag.value);
		*/
#if 0
		FD_ZERO(&master);
		FD_ZERO(&readfds);
		// add server socket to the set
		FD_SET(fdmax, &master);
#endif
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		// copy it
		readfds = master;
		ret = select(fdmax + 1, &readfds, NULL, NULL, &tv);
		switch (ret) {
		case -1:
			perror("select");
			break;
		case 0:
			//ETHSOCKET_DIAG("select timeout\n");
			break;
		default:
			// run throuth the existing connections looking for data to read
			ETHSOCKET_DIAG("fdmax %d  readfds 0x%x\n", fdmax, readfds);
			for (i = 0; i <= fdmax; i++) {
				// one data coming
				if (FD_ISSET(i, &readfds)) {
					// handle new connection
					if (i == server_socket) {
						ETHSOCKET_DIAG("accept begin %d\n", i);
						client_socket = accept(server_socket, (struct sockaddr *)&client_address, &calen);
						ETHSOCKET_DIAG("accept end %d, client_socket=%d\n", i, client_socket);

						#if defined(__ECOS)
						ret = setsockopt(client_socket, SOL_SOCKET, SO_SNDBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
						if (ret == -1) {
							printf("Couldn't setsockopt(SO_SNDBUF)\n");
						}
						ret = setsockopt(client_socket, SOL_SOCKET, SO_RCVBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
						if (ret == -1) {
							printf("Couldn't setsockopt(SO_RCVBUF)\n");
						}

						flag = 1;
						ret = setsockopt(client_socket, SOL_SOCKET, SO_OOBINLINE, (char *)&flag, sizeof(int));
						if (ret == -1) {
							printf("Couldn't setsockopt(SO_OOBINLINE)\n");
						}
						#endif

						//printf("EthSocketDispIsEnable=%d\n",EthSocketDispIsEnable);
						#if 1
						if(1){//EthSocketDispIsEnable==0){
							flag = 1;
							ret = setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
							if (ret == -1) {
								printf("Couldn't setsockopt(TCP_NODELAY)\n");
							}else{
								printf("setsockopt(TCP_NODELAY)\n");
							}
						}
						#endif

						flag = 1;
						// set socket non-block
						if (IOCTL(client_socket, FIONBIO, &flag) != 0) {
							printf("%s: IOCTL errno = %d\r\n", __func__, errno);
						}

						ETHSOCKET_DIAG("client_address %x \r\n", client_address.sin_addr.s_addr);

						// client has request
						if (pethsocket_obj->notify) {
							pethsocket_obj->notify(CYG_ETHSOCKET_STATUS_CLIENT_CONNECT, client_address.sin_addr.s_addr);
						}

						if (client_socket != -1) {
							if (pethsocket_obj->client_socket) {
								ETHSOCKET_DIAG("close org client_socket %d \n", pethsocket_obj->client_socket);
								FD_CLR(pethsocket_obj->client_socket, &master);
								SOCKET_CLOSE(pethsocket_obj->client_socket);
							}
							// add to master set
							ETHSOCKET_DIAG("FD_SET %d\n", client_socket);
							FD_SET(client_socket, &master);
							// keep track of the max
							pethsocket_obj->client_socket = client_socket;
							if (client_socket > fdmax) {
								fdmax = client_socket;
							}

							ETHSOCKET_DIAG("client_socket %x %x master 0x%x ,fdmax 0x%x\n", pethsocket_obj, pethsocket_obj->client_socket, master, fdmax);

						}
					}
					// handle data from a client
					else {
						ETHSOCKET_DIAG("handle data from a client %d\n", i);
						// client has request
						{
							if (pethsocket_obj->notify) {
								pethsocket_obj->notify(CYG_ETHSOCKET_STATUS_CLIENT_REQUEST, 0);
							}
							ret = ethsocket_process(i, (struct sockaddr *)&client_address);
							// if connection close
							if (ret) {
								ETHSOCKET_DIAG("FD_CLR %d\n", i);
								FD_CLR(i, &master);
								pethsocket_obj->client_socket = 0;
								ETHSOCKET_DIAG("disconnect %x %x\n", pethsocket_obj, pethsocket_obj->client_socket);

								if (pethsocket_obj->notify) {
									pethsocket_obj->notify(CYG_ETHSOCKET_STATUS_CLIENT_DISCONNECT, 0);
								}

							}
						}
					}
				}
			}
		}
	}
	if(EthCamSocket_GetEthLinkStatus(0)==ETHCAM_LINK_DOWN){
		printf("ethsocket_server linkdown clis=%d\n",pethsocket_obj->client_socket);
		if (FD_ISSET(pethsocket_obj->client_socket, &master)) {
			FD_CLR(pethsocket_obj->client_socket, &master);
		}
	}
	// close all the opened sockets
	for (i = 0; i <= fdmax; i++) {
		if (FD_ISSET(i, &master)) {
			SOCKET_CLOSE(i);
			ETHSOCKET_DIAG("close socket %d\n", i);
		}
	}
//ethsocket_server_exit:
	// reset the server socket value
	server_socket = -1;

#if 0
	// free allocated memory
	if (g_Sendbuff) {
		free(g_Sendbuff);
		ETHSOCKET_DIAG("free g_Sendbuff = 0x%x\r\n", g_Sendbuff);
		g_Sendbuff = NULL;
	}
#endif
	//cyg_flag_setbits(&ethsocket_flag, CYG_FLG_ETHSOCKET_IDLE);
	vos_flag_set(ethsocket_flag,CYG_FLG_ETHSOCKET_IDLE);

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
static void ethsocket_init(cyg_addrword_t arg)
#else
//static void *ethsocket_init(void *arg)
static THREAD_RETTYPE ethsocket_init(void *arg)
#endif
{
	int err = 0, flag;
	struct addrinfo *res;
	char   portNumStr[10];
	int    portNum = ethsocket_obj.portNum;
	//int    sock_buf_size = ETHSOCKET_SOCK_BUF_SIZE;

	ETHSOCKET_DIAG("ethsocket_init\n");
	THREAD_ENTRY();

	memset(&server_address, 0, sizeof(server_address));
	//server_address.ai_family = AF_UNSPEC; // user IPv4 or IPv6 whichever
	server_address.ai_family = AF_INET;
	server_address.ai_socktype = SOCK_STREAM;
	server_address.ai_flags = AI_PASSIVE; // fill in my IP for me

	// check if port number valid
	if (portNum > CYGNUM_MAX_PORT_NUM) {
		printf("error port number %d\r\n", portNum);
		//return DUMMY_NULL;
		goto PROC_END;
	}
	sprintf(portNumStr, "%d", portNum);
	err = getaddrinfo(NULL, portNumStr, &server_address, &res);
	if (err != 0) {
		#if _TODO
		printf("err:getaddrinfo: %s\n", gai_strerror(err));
		#else
		printf("err:getaddrinfo: %d\n", err);
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
	ETHSOCKET_DIAG("create socket\r\n");
	server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

#if 0
	if (IOCTL(server_socket, SIOCGIFADDR, (char *)&ifreq) >= 0) {
		in_addr = (struct sockaddr_in *) & (ifreq.ifr_addr);
		ETHSOCKET_DIAG("ipstr = %s\r\n", inet_ntoa(in_addr->sin_addr));
	}
#endif

	ETHSOCKET_DIAG("server_socket = %d\r\n", server_socket);
	CYG_ASSERT(server_socket >= 0, "Socket create failed");

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
	err = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_REUSEADDR)\n");
	}

#if defined(__ECOS)
	err = setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, (char *)&flag, sizeof(flag));
#else
#if 0//defined (SO_REUSEPORT)
	err = setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, (char *)&flag, sizeof(flag));
#endif
#endif
	if (err == -1) {
		printf("Couldn't setsockopt(SO_REUSEPORT)\n");
	}

#if defined(__ECOS)
	sock_buf_size = ethsocket_obj.sockbufSize;

	err = setsockopt(server_socket, SOL_SOCKET, SO_SNDBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_SNDBUF)\n");
	}

	err = setsockopt(server_socket, SOL_SOCKET, SO_RCVBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_RCVBUF)\n");
	}
#endif

#endif

	ETHSOCKET_DIAG("bind %d\r\n", portNum);
	err = bind(server_socket, res->ai_addr, res->ai_addrlen);
	CYG_ASSERT(err == 0, "bind() returned error");

	ETHSOCKET_DIAG("listen\r\n");
	err = listen(server_socket, SOMAXCONN);
	CYG_ASSERT(err == 0, "listen() returned error");

	freeaddrinfo(res);   // free the linked list

	/* Now go be a server ourself.
	 */
	ethsocket_server(arg);

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

__externC void ethsocket_open(void)
{
	int ret =0;
	T_CFLG cflg ;
	char name[40]={0};

	if (!isServerStart) {
		isStopServer = false;
		//cyg_flag_init(&ethsocket_flag);
		snprintf(name, sizeof(name) - 1, "ethsoFlag");
		vos_flag_create(&ethsocket_flag, &cflg, name);
		vos_flag_clr(ethsocket_flag, 0xFFFFFFFF);

		ETHSOCKET_DIAG("ethsocket_startup create thread\r\n");
#if defined(__ECOS)
		ret = cyg_thread_create(ethsocket_obj.threadPriority,
						  ethsocket_init,
						  0,
						  "ethsocket0",
						  &ethsocket_stacks[0],
						  sizeof(ethsocket_stacks),
						  &ethsocket_thread,
						  &ethsocket_thread_object
						 );
		ETHSOCKET_DIAG("ethsocket_startup resume thread\r\n");
		cyg_thread_resume(ethsocket_thread);
#else
		//ret = pthread_create(&ethsocket_thread, NULL, ethsocket_init, NULL);
		snprintf(name, sizeof(name) - 1, "ethsoPrc");

		ethsocket_thread=vos_task_create(ethsocket_init,  NULL, name,   ethsocket_obj.threadPriority,	STKSIZE_ETHCAM_PROC);
		vos_task_resume(ethsocket_thread);

#endif
        if (ret < 0) {
            printf("create feed_bs_thread failed\n");
        } else {
            isServerStart = true;
        }
	}
}

__externC void ethsocket_close(void)
{
	FLGPTN uiFlag;
	int delay_cnt;
	int i;

	if (isServerStart) {
		isStopServer = true;
		//cyg_flag_maskbits(&ethsocket_flag, ~CYG_FLG_ETHSOCKET_START);
		vos_flag_clr(ethsocket_flag, CYG_FLG_ETHSOCKET_START);

		ETHSOCKET_DIAG("ethsocket_stop wait idle begin\r\n");
		//cyg_flag_waitms(&ethsocket_flag, CYG_FLG_ETHSOCKET_IDLE, CYG_FLAG_WAITMODE_AND);
		vos_flag_wait(&uiFlag, ethsocket_flag, CYG_FLG_ETHSOCKET_IDLE, TWF_ORW);

		delay_cnt = 50;
		while ((server_socket != -1) && delay_cnt) {
			//CHKPNT;
			vos_util_delay_ms(10);
			delay_cnt --;
		}
		ETHSOCKET_DIAG("ethsocket_stop wait idle end\r\n");
		ETHSOCKET_DIAG("ethsocket_stop ->suspend thread\r\n");
#if defined(__ECOS)
		cyg_thread_suspend(ethsocket_thread);
		ETHSOCKET_DIAG("ethsocket_stop ->delete thread\r\n");
		cyg_thread_delete(ethsocket_thread);
#else
		//pthread_join(ethsocket_thread, NULL);

		if(server_socket!= -1){
			printf("Destroy EthCamTsk\r\n");
			for (i = 0; i <= fdmax; i++) {
				if (FD_ISSET(i, &master)) {
					SOCKET_CLOSE(i);
					ETHSOCKET_DIAG("close socket %d\n", i);
				}
			}
			server_socket=-1;
			vos_task_destroy(ethsocket_thread);
		}

#endif
		//cyg_flag_destroy(&ethsocket_flag);
		vos_flag_destroy(ethsocket_flag);
		isServerStart = false;
		ETHSOCKET_DIAG("ethsocket_stop ->end\r\n");
	}
}

__externC int ethsocket_send(char *buf, int *size)
{
	int ret = -1;
	int i = 0;
	int bSend=0;
	//ethsocket_install_obj *pethsocket_obj;
	//pethsocket_obj = &ethsocket_obj;

	ETHSOCKET_DIAG("ethsocket_send client_socket %d fdmax 0x%x\r\n", pethsocket_obj->client_socket, fdmax);
	if ((buf) && (*size)) {
		ETHSOCKET_DIAG("send %s %d client_socket %d master %x\n", buf, *size, pethsocket_obj->client_socket, master);
		for (i = 0; i <= fdmax; i++) {
			//i=fdmax;
			if (FD_ISSET(i, &master)) {
				if (i != server_socket) {
					ETHSOCKET_DIAG("send %s %d client_socket %d\n", buf, *size, i);
					ret = sendall(i, buf, size);
					bSend=1;
					if(ret!=0){
						//printf("error -1, ethsocket_send %d bytes\n", *size);
					}
				}
			}
		}
	}
	if(ret!=0 && bSend==0){
		//printf("error -1, ethsocket_send %d bytes, bSend=%d\n", *size, bSend);
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
__externC void ethsocket_install(ethsocket_install_obj  *pObj)
{
	ethsocket_obj.client_socket = 0;
	ethsocket_obj.notify = pObj->notify;
	ethsocket_obj.recv = pObj->recv;
	if (!pObj->portNum) {
		ethsocket_obj.portNum = ETHSOCKET_SERVER_PORT;
		printf("error Port, set default port %d\r\n", ETHSOCKET_SERVER_PORT);
	} else {
		ethsocket_obj.portNum = pObj->portNum;
	}
	if (!pObj->threadPriority) {
		ethsocket_obj.threadPriority = ETHSOCKET_THREAD_PRIORITY;
		printf("error threadPriority, set default %d\r\n", ETHSOCKET_THREAD_PRIORITY);
	} else {
		ethsocket_obj.threadPriority = pObj->threadPriority;
	}
	if (!pObj->sockbufSize) {
		ethsocket_obj.sockbufSize = ETHSOCKET_SOCK_BUF_SIZE;
		printf("error sockSendbufSize %d, set default %d \r\n", pObj->sockbufSize, ETHSOCKET_SOCK_BUF_SIZE);
	} else {
		ethsocket_obj.sockbufSize = pObj->sockbufSize;
	}

}

__externC void ethsocket_uninstall(void)
{
	memset(&ethsocket_obj, 0, sizeof(ethsocket_install_obj));
}

static int sendall(int s, char *buf, int *len)
{
	int total = 0;
	int bytesleft = *len;
	int n = -1;


	ETHSOCKET_DIAG("sendall %d bytes\n", *len);
	while (total < *len) {
		n = send(s, buf + total, bytesleft, 0);
		//ETHSOCKET_DIAG("send %d bytes\n",n);
		if (n == -1) {
			//printf("error -1, sendall %d bytes\n",  *len);
			break;
		}
		total += n;
		bytesleft -= n;
	}
	*len = total;
	return n == -1 ? -1 : 0;
}

static int recvall(int s, char *buf, int len, int isJustOne)
{
	int total = 0;
	int bytesleft = len;
	int n = -1;
	int ret;
	int idletime;

	ETHSOCKET_DIAG("recvall buf = 0x%x, len = %d bytes\n", (int)buf, len);
	while (total < len && (!isStopServer)) {
		n = recv(s, buf + total, bytesleft, 0);
		if (n == 0) {
			// client close the socket
			return 0;
		}
		if (n < 0) {
			if (errno == EAGAIN) {
				idletime = 0;
				while (!isStopServer) {
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
/* end of ethsocket.c                                                  */

