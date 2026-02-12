/* =================================================================
 *
 *      ethsocketudp.h
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
#include <dirent.h>
#include "EthCam/ethsocket.h"
#include "ethsocket_int.h"

#include "kwrap/task.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/type.h"
#include <kwrap/util.h>

#if defined(__ECOS)
#include <network.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <cyg/ethsocket/ethsocket.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
//#include <UsockIpc/usocket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#endif

#define ETHSOCKET_UDP_SERVER_PORT           2222
#define ETHSOCKET_UDP_THREAD_PRIORITY       6
#define ETHSOCKET_UDP_THREAD_STACK_SIZE     5120
#define ETHSOCKET_UDP_SERVER_BUFFER_SIZE    512

/* ================================================================= */

#if 0
#if defined(__ECOS)
#define ETHSOCKET_UDP_DIAG diag_printf
#else
#define ETHSOCKET_UDP_DIAG printf
#endif
#else
#define ETHSOCKET_UDP_DIAG(...)
#endif


/* ================================================================= */
/* UdpServer socket address and file descriptor.
 */

#define CYGNUM_MAX_PORT_NUM                 65535

#define ETHSOCKET_UDP_SOCK_BUF_SIZE         (8 * 1024)

#define CYGNUM_SELECT_TIMEOUT_USEC          500000

#define ETHSOCKET_UDP_SERVER_IDLE_TIME      1  // retry 20

#define STKSIZE_ETHCAM_UDP_PROC           4096

// flag define

#define CYG_FLG_ETHSOCKET_UDP_START         (0x01)
#define CYG_FLG_ETHSOCKET_UDP_IDLE          (0x02)

static ethsocket_install_obj ethsocket_udp_obj = {0};

#if defined(__ECOS)
static cyg_flag_t ethsocket_udp_flag;
#else
//static volatile int ethsocket_udp_flag;
ID ethsocket_udp_flag;
#endif

static bool isUdpServerStart = false, isStopUdpServer = false;
//static bool isUdpReceiveDid = false;
static struct sockaddr_in g_UdpSocketSendAddr;
static int g_UdpSocketSendAddrLen = sizeof(struct sockaddr_in);
static int g_Udpsockfd;

/* ================================================================= */
/* Thread stacks, etc.
 */
#if defined(__ECOS)
static cyg_uint8 ethsocket_udp_stacks[ETHSOCKET_UDP_SERVER_BUFFER_SIZE + ETHSOCKET_UDP_THREAD_STACK_SIZE] = {0};
static cyg_handle_t ethsocket_udp_thread;
static cyg_thread  ethsocket_udp_thread_object;
#else
//static pthread_t ethsocket_udp_thread;
static THREAD_HANDLE ethsocket_udp_thread;
#endif
static int fdmax;
static fd_set master;  // master file descriptor list

//static int sendall(int s, char* buf, int *len);

#if defined(__ECOS)
static void ethsocket_udp_server(cyg_addrword_t arg)
#else
static void *ethsocket_udp_server(void *arg)
#endif
{
	int err = 0, flag;
	int sock_buf_size = ETHSOCKET_UDP_SOCK_BUF_SIZE;
	int portNum = ethsocket_udp_obj.portNum;
	int sockfd, len;
	struct sockaddr_in addr;
	int addr_len = sizeof(struct sockaddr_in);
	char buffer[ETHSOCKET_UDP_SERVER_BUFFER_SIZE];
	//fd_set master;  // master file descriptor list
	fd_set readfds; // temp file descriptor list for select()
	//int fdmax, i;
	int i;
	int ret;
	int n = 1;

	struct timeval tv;
	ethsocket_install_obj *pethsocket_udp_obj;

	pethsocket_udp_obj = &ethsocket_udp_obj;

	ETHSOCKET_UDP_DIAG("ethsocket_udp_udp_server\n");
	THREAD_ENTRY();

	/*config sockaddr_in */
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(portNum);
	addr.sin_addr.s_addr = htonl(INADDR_ANY) ;

	// check if port number valid
	if (portNum > CYGNUM_MAX_PORT_NUM) {
		printf("error port number %d\r\n", portNum);
		//return DUMMY_NULL;
		goto func_end;
	}

	/* create socket*/
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		//return DUMMY_NULL;
		goto func_end;
	}

#if 1
	sock_buf_size = pethsocket_udp_obj->sockbufSize;
	flag = 1;
	err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_REUSEADDR)\n");
	}

#if defined(__ECOS)
	err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&flag, sizeof(flag));
#else
#if defined (SO_REUSEPORT)
	err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&flag, sizeof(flag));
#endif
#endif
	if (err == -1) {
		printf("Couldn't setsockopt(SO_REUSEPORT)\n");
	}

	err = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_SNDBUF)\n");
	}

	err = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_RCVBUF)\n");
	}

	err = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *) &n, sizeof(n));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_BROADCAST)\n");
	}



#endif

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		//return DUMMY_NULL;
		goto func_end;
	}

	ETHSOCKET_UDP_DIAG("sockfd %d\n", sockfd);

	// clear the set
	FD_ZERO(&master);
	FD_ZERO(&readfds);
	// add server socket to the set
	FD_SET(sockfd, &master);

	fdmax = sockfd;
	// set timeout to 100 ms
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	pethsocket_udp_obj->client_socket = sockfd;

	//cyg_flag_maskbits(&ethsocket_udp_flag, 0);
	//cyg_flag_setbits(&ethsocket_udp_flag, CYG_FLG_ETHSOCKET_UDP_START);
	vos_flag_clr(ethsocket_udp_flag, 0xFFFFFFFF);
	vos_flag_set(ethsocket_udp_flag, CYG_FLG_ETHSOCKET_UDP_START);

	while (!isStopUdpServer) {
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		// copy it
		readfds = master;
		ret = select(fdmax + 1, &readfds, NULL, NULL, &tv);
		//ret = select(fdmax+1,&readfds,NULL,NULL,NULL);
		switch (ret) {
		case -1:
			perror("select");
			break;
		case 0:
			//ETHSOCKET_UDP_DIAG("select timeout\n");
			break;
		default:
			ETHSOCKET_UDP_DIAG("fdmax %d\n", fdmax);
			for (i = 0; i <= fdmax; i++) {
				// one data coming
				if (FD_ISSET(i, &readfds)) {
					// handle new connection
					if (i == sockfd) {
						// client has request
						if (pethsocket_udp_obj->notify) {
							pethsocket_udp_obj->notify(CYG_ETHSOCKET_UDP_STATUS_CLIENT_REQUEST, (int)&addr);
						}

						bzero(buffer, sizeof(buffer));
						//memset(buffer,97,sizeof(buffer)); for test
						len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, (socklen_t *)&addr_len);

						//printf("receive from %s\n" , inet_ntoa( addr.sin_addr));

						if (pethsocket_udp_obj->recv) {
							pethsocket_udp_obj->recv(buffer, len);
						}

#if 0
						/*send back to client*/
						sendto(sockfd, buffer, len, 0, (struct sockaddr *)&addr, addr_len);
#endif
						//if(!isUdpReceiveDid)
						{
							//isUdpReceiveDid = true;
							memcpy((void *)&g_UdpSocketSendAddr, (void *)&addr, sizeof(addr));
							g_UdpSocketSendAddrLen = addr_len;
							g_Udpsockfd = sockfd;
						}
					}
				}
			}
			break;
		}

	}

	// close all the opened sockets
	for (i = 0; i <= fdmax; i++) {
		if (FD_ISSET(i, &master)) {
			close(i);
			ETHSOCKET_UDP_DIAG("close socket %d\n", i);
		}
	}

	pethsocket_udp_obj->client_socket =0;
	//cyg_flag_setbits(&ethsocket_udp_flag, CYG_FLG_ETHSOCKET_UDP_IDLE);
	vos_flag_set(ethsocket_udp_flag,CYG_FLG_ETHSOCKET_UDP_IDLE);
	printf("ethsocket_udp_server close\n");

	func_end:

	THREAD_RETURN(0);

	//return DUMMY_NULL;
}

/* ================================================================= */
/* Initialization thread
 *
 * Optionally delay for a time before getting the network
 * running. Then create and bind the server socket and put it into
 * listen mode. Spawn any further server threads, then enter server
 * mode.
 */
#if 0
static void ethsocket_udp_init(cyg_addrword_t arg)
{
	int err = 0, flag;
	struct addrinfo *res;
	char   portNumStr[10];
	int    portNum = ethsocket_udp_obj.portNum;
	int    sock_buf_size = ETHSOCKET_UDP_SOCK_BUF_SIZE;

	ETHSOCKET_UDP_DIAG("ethsocket_udp_init\n");

	memset(&server_address, 0, sizeof(server_address));
	//server_address.ai_family = AF_UNSPEC; // user IPv4 or IPv6 whichever
	server_address.ai_family = AF_INET;
	server_address.ai_socktype = SOCK_STREAM;
	server_address.ai_flags = AI_PASSIVE; // fill in my IP for me

	// check if port number valid
	if (portNum > CYGNUM_MAX_PORT_NUM) {
		printf("error port number %d\r\n", portNum);
		return;
	}
	sprintf(portNumStr, "%d", portNum);
	err = getaddrinfo(NULL, portNumStr, &server_address, &res);
	if (err != 0) {
		printf("getaddrinfo: %s\n", gai_strerror(err));
		return;
	}

	/* Create and bind the server socket.
	 */
	ETHSOCKET_UDP_DIAG("create socket\r\n");
	udp_server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	ETHSOCKET_UDP_DIAG("udp_server_socket = %d\r\n", udp_server_socket);
	CYG_ASSERT(udp_server_socket > 0, "Socket create failed");

#if 1
	flag = 1;
	err = setsockopt(udp_server_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
	if (err == -1) {
		printf("Couldn't setsockopt(TCP_NODELAY)\n");
	}
#endif
#if 1
	sock_buf_size = ethsocket_udp_obj.sockbufSize;
	flag = 1;
	err = setsockopt(udp_server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_REUSEADDR)\n");
	}

	err = setsockopt(udp_server_socket, SOL_SOCKET, SO_REUSEPORT, (char *)&flag, sizeof(flag));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_REUSEPORT)\n");
	}

	err = setsockopt(udp_server_socket, SOL_SOCKET, SO_SNDBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_SNDBUF)\n");
	}

	err = setsockopt(udp_server_socket, SOL_SOCKET, SO_RCVBUF, (char *)&sock_buf_size, sizeof(sock_buf_size));
	if (err == -1) {
		printf("Couldn't setsockopt(SO_RCVBUF)\n");
	}


#endif

	ETHSOCKET_UDP_DIAG("bind %d\r\n", portNum);
	err = bind(udp_server_socket, res->ai_addr, res->ai_addrlen);
	CYG_ASSERT(err == 0, "bind() returned error");

	ETHSOCKET_UDP_DIAG("listen\r\n");
	err = listen(udp_server_socket, 1);
	printf("** listen   %d\n", 1);
	CYG_ASSERT(err == 0, "listen() returned error");

	freeaddrinfo(res);   // free the linked list

	/* Now go be a server ourself.
	 */
	ethsocket_udp_server(arg);
}
#endif
/* ================================================================= */
/* System initializer
 *
 * This is called from the static constructor in init.cxx. It spawns
 * the main server thread and makes it ready to run. It can also be
 * called explicitly by the application if the auto start option is
 * disabled.
 */

__externC void ethsocket_udp_open(void)
{
	T_CFLG cflg ;

	if (!isUdpServerStart) {
		isStopUdpServer = false;
		//cyg_flag_init(&ethsocket_udp_flag);
		vos_flag_create(&ethsocket_udp_flag, &cflg, "ethsoUdpFlag");
		vos_flag_clr(ethsocket_udp_flag, 0xFFFFFFFF);

		ETHSOCKET_UDP_DIAG("ethsocket_udp_startup create thread\r\n");
#if defined(__ECOS)
		cyg_thread_create(ethsocket_udp_obj.threadPriority,
						  ethsocket_udp_server,
						  0,
						  "ethcam udp socket",
						  &ethsocket_udp_stacks[0],
						  sizeof(ethsocket_udp_stacks),
						  &ethsocket_udp_thread,
						  &ethsocket_udp_thread_object
						 );
		ETHSOCKET_UDP_DIAG("ethsocket_udp_startup resume thread\r\n");
		cyg_thread_resume(ethsocket_udp_thread);
#else

		//pthread_create(&ethsocket_udp_thread, NULL, ethsocket_udp_server, NULL);
		ethsocket_udp_thread=vos_task_create(ethsocket_udp_server,  0, "ethsoUdp",   ethsocket_udp_obj.threadPriority,	STKSIZE_ETHCAM_UDP_PROC);
		vos_task_resume(ethsocket_udp_thread);

#endif
		isUdpServerStart = true;
	}
}

__externC void ethsocket_udp_close(void)
{
	FLGPTN uiFlag;
	int delay_cnt;
	int i;

	if (isUdpServerStart) {
		isStopUdpServer = true;
		//cyg_flag_maskbits(&ethsocket_udp_flag, ~CYG_FLG_ETHSOCKET_UDP_START);
		vos_flag_clr(ethsocket_udp_flag, CYG_FLG_ETHSOCKET_UDP_START);
		printf("ethsocket_udp_stop wait idle begin\r\n");
		//cyg_flag_wait(&ethsocket_udp_flag, CYG_FLG_ETHSOCKET_UDP_IDLE, CYG_FLAG_WAITMODE_AND);
		vos_flag_wait(&uiFlag, ethsocket_udp_flag, CYG_FLG_ETHSOCKET_UDP_IDLE, TWF_ORW);
		delay_cnt = 50;
		while ((ethsocket_udp_obj.client_socket != 0) && delay_cnt) {
			vos_util_delay_ms(10);
			delay_cnt --;
		}

		printf("ethsocket_udp_stop wait idle end\r\n");
		printf("ethsocket_udp_stop ->suspend thread\r\n");
#if defined(__ECOS)
		cyg_thread_suspend(ethsocket_udp_thread);
		ETHSOCKET_UDP_DIAG("ethsocket_udp_stop ->delete thread\r\n");
		cyg_thread_delete(ethsocket_udp_thread);
#else
		//pthread_join(ethsocket_udp_thread, NULL);
		if(ethsocket_udp_obj.client_socket != 0){
			printf("Destroy ethsocket_udp_close\r\n");
			for (i = 0; i <= fdmax; i++) {
				if (FD_ISSET(i, &master)) {
					close(i);
				}
			}

			ethsocket_udp_obj.client_socket=0;
			vos_task_destroy(ethsocket_udp_thread);
		}
#endif
		//cyg_flag_destroy(&ethsocket_udp_flag);
		vos_flag_destroy(ethsocket_udp_flag);

		isUdpServerStart = false;
		printf("ethsocket_udp_stop ->end\r\n");
	}
}

__externC int ethsocket_udp_send(char *buf, int *size)
{
	int ret = 0;
	//ethsocket_install_obj *pethsocket_udp_obj;
	//pethsocket_udp_obj = &ethsocket_udp_obj;
	//ETHSOCKET_UDP_DIAG("ethsocket_udp_send %d\r\n", pethsocket_udp_obj->client_socket);
#if 0
	if (pethsocket_udp_obj->client_socket) {
		ETHSOCKET_UDP_DIAG("send %s %d client_socket %d\n", buf, *size, pethsocket_udp_obj->client_socket);
		if (*size) {
			ret = sendall(pethsocket_udp_obj->client_socket, buf, size);
		}
	}
#else
	ret = sendto(g_Udpsockfd, buf, *size, 0, (struct sockaddr *)&g_UdpSocketSendAddr, g_UdpSocketSendAddrLen);
	if (ret <= 0) {
		perror("ethsocket_udp_send");
		*size = 0;
	} else {
		*size = ret;
	}
#endif
	return ret;
}

__externC int ethsocket_udp_sendto(char *dest_ip, int dest_port, char *buf, int *size)
{
	int sockfd;
	struct sockaddr_in client;
	int numbytes;
	int broadcast = 1;

	ETHSOCKET_UDP_DIAG("dest_port %d buf  %s sent %d bytes to %s\n", dest_port, buf, *size, dest_ip);

	if ((buf) && (*size)) {
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
			perror("socket");
			return (-1);
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast) == -1) {
			perror("setsockopt (SO_BROADCAST)");
			close(sockfd);
			return (-1);
		}

		memset(&client, 0, sizeof(client));
		client.sin_family = AF_INET;
		client.sin_addr.s_addr = inet_addr(dest_ip);
#if !defined(__LINUX_USER__)
		client.sin_len = sizeof(client);
#endif
		client.sin_port = htons(dest_port);
		ETHSOCKET_UDP_DIAG("0x%x \n", client.sin_addr.s_addr);

#if 0  //add to routing table for bradcast packet
		__externC int DHCPAddMAC(const char *dest, const char *gateway, const int index);

#define MAC_BCAST_ADDR      (char *) "\xff\xff\xff\xff\xff\xff"
		if (client.sin_addr.s_addr == INADDR_BROADCAST) {
			DHCPAddMAC(dest_ip, MAC_BCAST_ADDR, 2);
			show_arp_tables(diag_printf);
		}
#endif

		if ((numbytes = sendto(sockfd, buf, *size, 0, (struct sockaddr *)&client, sizeof client)) == -1) {
			perror("sendto");
			close(sockfd);
			*size = 0;
			return (-1);
		}

		ETHSOCKET_UDP_DIAG("sent %d bytes to %s\n", numbytes, inet_ntoa(client.sin_addr));
		*size = numbytes;
		close(sockfd);
	}
	return 0;
}


__externC void ethsocket_udp_install(ethsocket_install_obj  *pObj)
{
	ethsocket_udp_obj.client_socket = 0;
	ethsocket_udp_obj.notify = pObj->notify;
	ethsocket_udp_obj.recv = pObj->recv;
	if (!pObj->portNum) {
		ethsocket_udp_obj.portNum = ETHSOCKET_UDP_SERVER_PORT;
		printf("error Port, set default port %d\r\n", ETHSOCKET_UDP_SERVER_PORT);
	} else {
		ethsocket_udp_obj.portNum = pObj->portNum;
	}
	if (!pObj->threadPriority) {
		ethsocket_udp_obj.threadPriority = ETHSOCKET_UDP_THREAD_PRIORITY;
		printf("error threadPriority, set default %d\r\n", ETHSOCKET_UDP_THREAD_PRIORITY);
	} else {
		ethsocket_udp_obj.threadPriority = pObj->threadPriority;
	}
	if (!pObj->sockbufSize) {
		ethsocket_udp_obj.sockbufSize = ETHSOCKET_UDP_SOCK_BUF_SIZE;
		printf("error sockSendbufSize %d, set default %d \r\n", pObj->sockbufSize, ETHSOCKET_UDP_SOCK_BUF_SIZE);
	} else {
		ethsocket_udp_obj.sockbufSize = pObj->sockbufSize;
	}

}
__externC void ethsocket_udp_uninstall(void)
{
	memset(&ethsocket_udp_obj, 0, sizeof(ethsocket_install_obj));
}
#if 0
static int sendall(int s, char *buf, int *len)
{
	int total = 0;
	int bytesleft = *len;
	int n = -1;


	ETHSOCKET_UDP_DIAG("sendall %d bytes\n", *len);
	while (total < *len) {
		n = send(s, buf + total, bytesleft, 0);
		//ETHSOCKET_UDP_DIAG("send %d bytes\n",n);
		if (n == -1) {
			break;
		}
		total += n;
		bytesleft -= n;
	}
	*len = total;
	return n == -1 ? -1 : 0;
}
#endif
/* ----------------------------------------------------------------- */
/* end of ethsocketudp.c                                             */

