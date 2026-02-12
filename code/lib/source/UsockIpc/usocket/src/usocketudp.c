/* =================================================================
 *
 *      usocket.h
 *
 *      A simple user socket
 *
 * =================================================================
 */

#if defined(__ECOS)
#include <cyg/infra/cyg_trac.h>        /* tracing macros */
#include <cyg/infra/cyg_ass.h>         /* assertion macros */
#endif

#include "usocket_int.h"

#if defined(__ECOS)
#include <network.h>
#include <cyg/usocket/usocket.h>
#include <arpa/inet.h>
#elif defined(__LINUX_USER__)
#include <sys/types.h>
#include <sys/socket.h>
#include <UsockIpc/usocket.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#elif defined(__FREERTOS)
#include <kwrap/type.h>
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#include <FreeRTOS_POSIX/dirent.h>
#include <kwrap/semaphore.h>
#include <kwrap/task.h>
#include <UsockIpc/usocket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#define SOMAXCONN   4
#endif

#if defined(__FREERTOS)
#define IOCTL                  lwip_ioctl
#define SOCKET_CLOSE           lwip_close
#else
#define IOCTL                  ioctl
#define SOCKET_CLOSE           close
#endif
#define USOCKET_UDP_SERVER_PORT 2222
#define USOCKET_UDP_THREAD_PRIORITY 6
#define USOCKET_UDP_THREAD_STACK_SIZE 5120
#define USOCKET_UDP_SERVER_BUFFER_SIZE 512

/* ================================================================= */

#if 0
#if defined(__ECOS)
#define USOCKET_UDP_DIAG diag_printf
#else
#define USOCKET_UDP_DIAG printf
#endif
#else
#define USOCKET_UDP_DIAG(...)
#endif


/* ================================================================= */
/* UdpServer socket address and file descriptor.
 */

#define CYGNUM_MAX_PORT_NUM         65535

#define USOCKET_UDP_SOCK_BUF_SIZE       8*1024

#define CYGNUM_SELECT_TIMEOUT_USEC      500000

#define USOCKET_UDP_SERVER_IDLE_TIME   1  // retry 20


// flag define

#define CYG_FLG_USOCKET_UDP_START       0x01
#define CYG_FLG_USOCKET_UDP_IDLE        0x02

static usocket_install_obj usocket_udp_obj={0};

#if defined(__ECOS)
static cyg_flag_t usocket_udp_flag;
#elif defined(__LINUX_USER__)
static unsigned int   usocket_udp_flag;
#elif defined(__FREERTOS)
static volatile int usocket_udp_flag;
#endif
static COND_HANDLE    usocket_udp_cond;
static MUTEX_HANDLE     usocket_udp_mtx;

static bool isUdpServerStart = false, isStopUdpServer = false;
//static bool isUdpReceiveDid = false;
static struct sockaddr_in g_UdpSocketSendAddr;
static int g_UdpSocketSendAddrLen = sizeof(struct sockaddr_in);
static int g_Udpsockfd;

/* ================================================================= */
/* Thread stacks, etc.
 */
#if defined(__ECOS)
static cyg_uint8 usocket_udp_stacks[USOCKET_UDP_SERVER_BUFFER_SIZE+
                              USOCKET_UDP_THREAD_STACK_SIZE] = {0};
static cyg_handle_t usocket_udp_thread;
static cyg_thread  usocket_udp_thread_object;
#else
static pthread_t usocket_udp_thread;
#endif

//static int sendall(int s, char* buf, int *len);

#if defined(__ECOS)
static void usocket_udp_server(cyg_addrword_t arg)
#else
static void *usocket_udp_server(void *arg)
#endif
{
    int err = 0, flag;
    int sock_buf_size = USOCKET_UDP_SOCK_BUF_SIZE;
    int portNum = usocket_udp_obj.portNum;
    int sockfd,len;
    struct sockaddr_in addr;
    int addr_len = sizeof(struct sockaddr_in);
    char buffer[USOCKET_UDP_SERVER_BUFFER_SIZE];
    fd_set master;  // master file descriptor list
    fd_set readfds; // temp file descriptor list for select()
    int fdmax,i;
    int ret;
    int n = 1;

    struct timeval tv;
    usocket_install_obj* pusocket_udp_obj;

    pusocket_udp_obj = &usocket_udp_obj;

    USOCKET_UDP_DIAG("usocket_udp_udp_server portNum %d \n",portNum);

    /*config sockaddr_in */
    bzero ( &addr, sizeof(addr) );
    addr.sin_family=AF_INET;
    addr.sin_port=htons(portNum);
    addr.sin_addr.s_addr=htonl(INADDR_ANY) ;

    // check if port number valid
    if (portNum > CYGNUM_MAX_PORT_NUM)
    {
        printf("error port number %d\r\n",portNum);
        return DUMMY_NULL;
    }

    /* create socket*/
    if((sockfd=socket(AF_INET,SOCK_DGRAM,0))<0)
    {
        perror ("socket");
        return DUMMY_NULL;
    }

    #if 1
    sock_buf_size = pusocket_udp_obj->sockbufSize;
    flag = 1;
    err = setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));
    if (err == -1)
        printf("Couldn't setsockopt(SO_REUSEADDR)\n");

    #if defined(__ECOS)
    err = setsockopt( sockfd, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
    #else
    #if defined (SO_REUSEPORT)
    err = setsockopt( sockfd, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
    #endif
    #endif
    if (err == -1)
        printf("Couldn't setsockopt(SO_REUSEPORT)\n");

    err = setsockopt(sockfd , SOL_SOCKET, SO_SNDBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
    if (err == -1)
        printf("Couldn't setsockopt(SO_SNDBUF)\n");

    err = setsockopt(sockfd , SOL_SOCKET, SO_RCVBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
    if (err == -1)
        printf("Couldn't setsockopt(SO_RCVBUF)\n");

    err = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *) &n, sizeof(n));
    if (err == -1)
        printf("Couldn't setsockopt(SO_BROADCAST)\n");

    #endif

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr))<0)
    {
        perror("connect");
        return DUMMY_NULL;
    }

    USOCKET_UDP_DIAG("sockfd %d\n",sockfd);

    // clear the set
    FD_ZERO(&master);
    FD_ZERO(&readfds);
    // add server socket to the set
    FD_SET(sockfd, &master);

    fdmax = sockfd;

    //cyg_flag_maskbits(&usocket_udp_flag, 0);
    //cyg_flag_setbits(&usocket_udp_flag, CYG_FLG_USOCKET_UDP_START);
    FLAG_SETPTN(usocket_udp_cond,usocket_udp_flag, CYG_FLG_USOCKET_UDP_START,usocket_udp_mtx);
    while(!isStopUdpServer)
    {

        // set timeout to 100 ms
        // select would modify timeout in different platform

        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        // copy it
        readfds = master;
        ret = select(fdmax+1,&readfds,NULL,NULL,&tv);
        //ret = select(fdmax+1,&readfds,NULL,NULL,NULL);
        switch(ret)
        {
            case -1:
                perror("select");
                break;
            case 0:
                //USOCKET_UDP_DIAG("select timeout\n");
                break;
            default:
                USOCKET_UDP_DIAG("fdmax %d  readfds 0x%x\n",fdmax,readfds);
                for (i=0; i<= fdmax; i++)
                {
                    // one data coming
                    if (FD_ISSET(i, &readfds))
                    {
                        // handle new connection
                        if (i== sockfd)
                        {
                            USOCKET_UDP_DIAG("sockfd %d client_address %x \r\n",sockfd,addr.sin_addr.s_addr );
                            // client has request
                            if (pusocket_udp_obj->notify)
                                pusocket_udp_obj->notify(CYG_USOCKET_UDP_STATUS_CLIENT_REQUEST,(int)&addr);

                            bzero(buffer,sizeof(buffer));
                            //memset(buffer,97,sizeof(buffer)); for test
                            len = recvfrom(sockfd,buffer,sizeof(buffer), 0 , (struct sockaddr *)&addr ,(socklen_t *)&addr_len);

                            //printf("receive from %s\n" , inet_ntoa( addr.sin_addr));
                            if ( len > 0 ) {
                                if(pusocket_udp_obj->recv)
                                    ((usocket_recv *)pusocket_udp_obj->recv)(buffer,len);

                                #if 0
                                /*send back to client*/
                                sendto(sockfd,buffer,len,0,(struct sockaddr *)&addr,addr_len);
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
                }
                break;
        }

    }

    // close all the opened sockets
    for (i=0; i<= fdmax; i++)
    {
        if (FD_ISSET(i, &master))
        {
            SOCKET_CLOSE(i);
            USOCKET_UDP_DIAG("close socket %d\n",i);
        }
    }

    //cyg_flag_setbits(&usocket_udp_flag, CYG_FLG_USOCKET_UDP_IDLE);
    FLAG_SETPTN(usocket_udp_cond, usocket_udp_flag, CYG_FLG_USOCKET_UDP_IDLE, usocket_udp_mtx);

	return DUMMY_NULL;
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
static void usocket_udp_init(cyg_addrword_t arg)
{
    int err = 0, flag;
    struct addrinfo *res;
    char   portNumStr[10];
    int    portNum = usocket_udp_obj.portNum;
    int    sock_buf_size = USOCKET_UDP_SOCK_BUF_SIZE;

    USOCKET_UDP_DIAG("usocket_udp_init\n");

    memset(&server_address,0,sizeof(server_address));
    //server_address.ai_family = AF_UNSPEC; // user IPv4 or IPv6 whichever
    server_address.ai_family = AF_INET;
    server_address.ai_socktype = SOCK_DGRAM;
    server_address.ai_flags = AI_PASSIVE; // fill in my IP for me

    // check if port number valid
    if (portNum > CYGNUM_MAX_PORT_NUM)
    {
        printf("error port number %d\r\n",portNum);
        return;
    }
    sprintf(portNumStr, "%d", portNum);
    err = getaddrinfo(NULL, portNumStr, &server_address, &res);
    if (err!=0)
    {
        #if _TODO
        printf("getaddrinfo: %s\n", gai_strerror(err));
        #else
        printf("getaddrinfo: %d\n", err);
        #endif
        return;
    }

    /* Create and bind the server socket.
     */
    USOCKET_UDP_DIAG("create socket\r\n");
    udp_server_socket = socket( res->ai_family, res->ai_socktype, res->ai_protocol );

    USOCKET_UDP_DIAG("udp_server_socket = %d\r\n",udp_server_socket);
    CYG_ASSERT( udp_server_socket >= 0, "Socket create failed");

    #if 0
    flag = 1;
    err = setsockopt(udp_server_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
    if (err == -1)
        printf("Couldn't setsockopt(TCP_NODELAY)\n");
    #endif
    #if 1
    sock_buf_size = usocket_udp_obj.sockbufSize;
    flag = 1;
    err = setsockopt( udp_server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));
    if (err == -1)
        printf("Couldn't setsockopt(SO_REUSEADDR)\n");

    #if defined(__ECOS)
    err = setsockopt( udp_server_socket, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
    #else
    #if defined (SO_REUSEPORT)
    err = setsockopt( udp_server_socket, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
    #endif
    #endif
    if (err == -1)
        printf("Couldn't setsockopt(SO_REUSEPORT)\n");

    err = setsockopt(udp_server_socket , SOL_SOCKET, SO_SNDBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
    if (err == -1)
        printf("Couldn't setsockopt(SO_SNDBUF)\n");

    err = setsockopt(udp_server_socket , SOL_SOCKET, SO_RCVBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
    if (err == -1)
        printf("Couldn't setsockopt(SO_RCVBUF)\n");


    #endif

    USOCKET_UDP_DIAG("bind %d\r\n",portNum);
    err = bind( udp_server_socket, res->ai_addr, res->ai_addrlen);
    CYG_ASSERT( err == 0, "bind() returned error");

    USOCKET_UDP_DIAG("listen\r\n");
    err = listen( udp_server_socket, 1 );

    freeaddrinfo(res);   // free the linked list

    /* Now go be a server ourself.
     */
    usocket_udp_server(arg);

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

__externC void usocket_udp_open(void)
{
    int ret =0;
    if (!isUdpServerStart)
    {
        isStopUdpServer = false;
        //cyg_flag_init(&usocket_udp_flag);
        COND_CREATE(usocket_udp_cond);
        MUTEX_CREATE(usocket_udp_mtx,1);
        //==== FLAG_INIT(usocket_udp_flag,usocket_udp_mtx);

        USOCKET_UDP_DIAG("usocket_udp_startup create thread\r\n");
        #if defined(__ECOS)
        ret = cyg_thread_create( usocket_udp_obj.threadPriority,
                           usocket_udp_server,
                           0,
                           "user udp socket",
                           &usocket_udp_stacks[0],
                           sizeof(usocket_udp_stacks),
                           &usocket_udp_thread,
                           &usocket_udp_thread_object
            );
        USOCKET_UDP_DIAG("usocket_udp_startup resume thread\r\n");
        cyg_thread_resume( usocket_udp_thread );
        #else
        ret = pthread_create(&usocket_udp_thread, NULL , usocket_udp_server , NULL);
        #endif
        if (ret < 0) {
            printf("create feed_bs_thread failed\n");
        } else {
            isUdpServerStart = true;
        }

    }
}

__externC void usocket_udp_close(void)
{
    if (isUdpServerStart)
    {
        isStopUdpServer = true;
        //cyg_flag_maskbits(&usocket_udp_flag,~CYG_FLG_USOCKET_UDP_START);
        FLAG_CLRPTN(usocket_udp_flag,CYG_FLG_USOCKET_UDP_START,usocket_udp_mtx);
        USOCKET_UDP_DIAG("usocket_udp_stop wait idle begin\r\n");
        FLAG_WAITPTN(usocket_udp_cond,usocket_udp_flag,CYG_FLG_USOCKET_UDP_IDLE,usocket_udp_mtx,false);
        USOCKET_UDP_DIAG("usocket_udp_stop wait idle end\r\n");
        USOCKET_UDP_DIAG("usocket_udp_stop ->suspend thread\r\n");
        #if defined(__ECOS)
        cyg_thread_suspend(usocket_udp_thread);
        USOCKET_UDP_DIAG("usocket_udp_stop ->delete thread\r\n");
        cyg_thread_delete(usocket_udp_thread);
        #else
        pthread_join(usocket_udp_thread, NULL);
        #endif
        //cyg_flag_destroy(&usocket_udp_flag);
        //==== FLAG_DESTROY(usocket_udp_flag,usocket_udp_mtx);
        COND_DESTROY(usocket_udp_cond);
        MUTEX_DESTROY(usocket_udp_mtx);

        isUdpServerStart = false;
        USOCKET_UDP_DIAG("usocket_udp_stop ->end\r\n");
    }
}

__externC int usocket_udp_send(char* buf, int* size)
{
    int ret =0;

    USOCKET_UDP_DIAG("usocket_udp_send g_Udpsockfd:%d 0x%x\r\n",g_Udpsockfd,g_UdpSocketSendAddr);


	if((buf)&&(*size))
	{
	    ret = sendto(g_Udpsockfd, buf, *size, 0, (struct sockaddr *)&g_UdpSocketSendAddr, g_UdpSocketSendAddrLen);
	    if (ret <= 0)
	    {
	        perror("usocket_udp_send");
	        *size = 0;
            return ret;
	    }
	    else
	    {
	        *size = ret;
	    }
	}
    return 0;
}

__externC int usocket_udp_sendto(char* dest_ip, int dest_port,char* buf, int* size)
{
    int sockfd;
    struct sockaddr_in client;
    int numbytes;
    int broadcast = 1;

    USOCKET_UDP_DIAG("dest_port %d buf  %s sent %d bytes to %s\n",dest_port,buf, *size,dest_ip);

    if((buf)&&(*size))
    {
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            perror("socket");
            return (-1);
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,sizeof broadcast) == -1) {
            perror("setsockopt (SO_BROADCAST)");
            SOCKET_CLOSE(sockfd);
            return (-1);
        }

        memset(&client, 0, sizeof(client));
        client.sin_family = AF_INET;
        client.sin_addr.s_addr = inet_addr(dest_ip);
#if !defined(__LINUX_USER__)
        client.sin_len=sizeof(client);
#endif
        client.sin_port = htons(dest_port);
        USOCKET_UDP_DIAG("0x%x \n",client.sin_addr.s_addr);

#if 0  //add to routing table for bradcast packet
        __externC int DHCPAddMAC(const char* dest,const char* gateway,const int index);

        #define MAC_BCAST_ADDR      (char *) "\xff\xff\xff\xff\xff\xff"
        if(client.sin_addr.s_addr==INADDR_BROADCAST)
        {
            DHCPAddMAC(dest_ip,MAC_BCAST_ADDR,2);
            show_arp_tables(diag_printf);
        }
#endif

        if ((numbytes=sendto(sockfd, buf, *size, 0,(struct sockaddr *)&client, sizeof client)) == -1) {
            perror("sendto");
            SOCKET_CLOSE(sockfd);
            *size = 0;
            return (-1);
        }

        USOCKET_UDP_DIAG("sent %d bytes to %s\n", numbytes,inet_ntoa(client.sin_addr));
        *size= numbytes;
        SOCKET_CLOSE(sockfd);
    }
    return 0;
}


__externC void usocket_udp_install( usocket_install_obj*  pObj)
{
    usocket_udp_obj.client_socket = 0;
    usocket_udp_obj.notify = pObj->notify;
    usocket_udp_obj.recv = pObj->recv;
    if (!pObj->portNum)
    {
        usocket_udp_obj.portNum = USOCKET_UDP_SERVER_PORT;
        printf("error Port, set default port %d\r\n",USOCKET_UDP_SERVER_PORT);
    }
    else
    {
        usocket_udp_obj.portNum = pObj->portNum;
    }
    if (!pObj->threadPriority)
    {
        usocket_udp_obj.threadPriority = USOCKET_UDP_THREAD_PRIORITY;
        printf("error threadPriority, set default %d\r\n",USOCKET_UDP_THREAD_PRIORITY);
    }
    else
    {
        usocket_udp_obj.threadPriority = pObj->threadPriority;
    }
    if (!pObj->sockbufSize)
    {
        usocket_udp_obj.sockbufSize = USOCKET_UDP_SOCK_BUF_SIZE;
        printf("error sockSendbufSize %d, set default %d \r\n",pObj->sockbufSize,USOCKET_UDP_SOCK_BUF_SIZE);
    }
    else
    {
        usocket_udp_obj.sockbufSize = pObj->sockbufSize;
    }

}
__externC void usocket_udp_uninstall(void)
{
    memset(&usocket_udp_obj,0,sizeof(usocket_install_obj));
}
#if 0
static int sendall(int s, char* buf, int *len)
{
    int total = 0;
    int bytesleft = *len;
    int n = -1;


    USOCKET_UDP_DIAG("sendall %d bytes\n",*len);
    while (total < *len)
    {
        n = send(s, buf+total, bytesleft, 0);
        //USOCKET_UDP_DIAG("send %d bytes\n",n);
        if (n==-1)
            break;
        total +=n;
        bytesleft -=n;
    }
    *len = total;
    return n==-1?-1:0;
}
#endif
/* ----------------------------------------------------------------- */
/* end of usocket.c                                                    */
