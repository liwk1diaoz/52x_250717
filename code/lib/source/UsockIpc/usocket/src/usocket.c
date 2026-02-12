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
#define _TODO   0

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "usocket_int.h"

#if defined(__ECOS)
#include <network.h>
#include <arpa/inet.h>
#include <cyg/usocket/usocket.h>
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


#if defined(__USE_IPC)
#include "usocket_ipc.h"
#endif

#define USOCKET_SERVER_PORT 3333
#define USOCKET_THREAD_PRIORITY 6
#define USOCKET_THREAD_STACK_SIZE 5120
#if defined(__USE_IPC)
#define USOCKET_SERVER_BUFFER_SIZE USOCKIPC_TRANSFER_BUF_SIZE
#else
#define USOCKET_SERVER_BUFFER_SIZE 1024
#endif
/* ================================================================= */

#if 0
#if defined(__ECOS)
#define USOCKET_DIAG diag_printf
#else
#define USOCKET_DIAG printf
#endif
#else
#define USOCKET_DIAG(...)
#endif


/* ================================================================= */
/* Server socket address and file descriptor.
 */

#define CYGNUM_MAX_PORT_NUM         65535

#define USOCKET_SOCK_BUF_SIZE       8*1024

#define CYGNUM_SELECT_TIMEOUT_USEC      500000

#define USOCKET_SERVER_IDLE_TIME   1  // retry 20


// flag define

#define CYG_FLG_USOCKET_START       0x01

#define CYG_FLG_USOCKET_IDLE        0x80


static struct addrinfo server_address;

static int server_socket = -1;

static usocket_install_obj usocket_obj={0};

//static char* g_Sendbuff = NULL;

#if defined(__ECOS)
static cyg_flag_t usocket_flag;
#elif defined(__LINUX_USER__)
static unsigned int   usocket_flag;
#else
static volatile int    usocket_flag;
#endif
static COND_HANDLE     usocket_cond;
static MUTEX_HANDLE      usocket_mtx;
static bool isServerStart = false, isStopServer = false;
static int fdmax;
static fd_set master;  // master file descriptor list

/* ================================================================= */
/* Thread stacks, etc.
 */

#if defined(__ECOS)
static cyg_uint8 usocket_stacks[USOCKET_SERVER_BUFFER_SIZE+
                              USOCKET_THREAD_STACK_SIZE] = {0};
#endif

#if defined(__ECOS)
static cyg_handle_t usocket_thread;
static cyg_thread   usocket_thread_object;
#else
static pthread_t    usocket_thread;
#endif

static int recvall(int s, char* buf, int len, int isJustOne);
static int sendall(int s, char* buf, int *len);

/* ================================================================= */
/* Main processing function                                          */
/*                                                                   */
/* Reads the HTTP header, look it up in the table and calls the      */
/* handler.                                                          */

// this function will return true, if the connection is closed.
static cyg_bool usocket_process( int client_socket, struct sockaddr *client_address )
{
    #if !defined(__FREERTOS)
    int calen = sizeof(*client_address);
    char name[64];
    char port[10];
    #endif
    int  ret = 0;
    char request[USOCKET_SERVER_BUFFER_SIZE];

    #if !defined(__FREERTOS)
    getnameinfo(client_address, calen, name, sizeof(name),
                port, sizeof(port), NI_NUMERICHOST|NI_NUMERICSERV);
    USOCKET_DIAG("Connection from %s[%s]\n",name,port);
    #endif
    //ret = recv(client_socket,request, sizeof(request), 0);
    //usocket_set_sock_nonblock(client_socket);
    memset(request,0,USOCKET_SERVER_BUFFER_SIZE);
    ret = recvall(client_socket,request, sizeof(request),true);

    // the connection is closed by client, so we close the client_socket
    if (ret == 0)
    {
        #if !defined(__FREERTOS)
        USOCKET_DIAG("Connection closed by client %s[%s]\n",name,port);
        #else
        USOCKET_DIAG("Connection closed by client %d\n",client_socket);
        #endif
        SOCKET_CLOSE(client_socket);
        return true;
    }
    if (ret == -1)
    {
        perror("recv time out");
        SOCKET_CLOSE(client_socket);
        #if 0
        sprintf(request,"hello\r\n");
        len = strlen(request);
        USOCKET_DIAG("send %s %d\n",request,len);
        ret = sendall(client_socket,request, &len);
        #endif
        return true;
    }
    else
    {
        #if 0
        char *pch=0;
        int len=0;
        USOCKET_DIAG("ret %d\n",ret);

        pch=strchr(request,'\n');
        len = pch -request+1;
        if(len>ret)
            len = ret;
        USOCKET_DIAG("send back %s %d\n",request,len);
        //USOCKET_DIAG("%x %x %x %x %x %x\n",request[0],request[1],request[2],request[3],request[4],request[5],request[6]);
        ret =sendall(client_socket,request, &len);
        #else

        if(usocket_obj.recv)
        {
            ((usocket_recv *)usocket_obj.recv)(request,ret);
        }
        #endif
        return false;
    }
}

static void usocket_server( cyg_addrword_t arg )
{
    int client_socket, sock_buf_size = USOCKET_SOCK_BUF_SIZE , flag;
    struct sockaddr_in client_address;
    socklen_t calen = sizeof(client_address);
    fd_set readfds; // temp file descriptor list for select()
    int i;
    int ret;
    struct timeval tv;
    usocket_install_obj* pusocket_obj;

    pusocket_obj = &usocket_obj;

    sock_buf_size = pusocket_obj->sockbufSize;

    USOCKET_DIAG("usocket_server , sock_buf_size  =%d\r\n",sock_buf_size);

    // clear the set
    FD_ZERO(&master);
    FD_ZERO(&readfds);
    // add server socket to the set
    FD_SET(server_socket, &master);

    fdmax = server_socket;

    //cyg_flag_maskbits(&usocket_flag, 0);
    //cyg_flag_setbits(&usocket_flag, CYG_FLG_USOCKET_START);
    FLAG_SETPTN(usocket_cond, usocket_flag, CYG_FLG_USOCKET_START, usocket_mtx);

    while(!isStopServer)
    {
        /*
        cyg_flag_setbits(&usocket_flag, CYG_FLG_USOCKET_IDLE);
        USOCKET_DIAG("wait start begin flag vlaue =0x%x\n",usocket_flag.value);
        cyg_flag_wait(&usocket_flag,CYG_FLG_USOCKET_START,CYG_FLAG_WAITMODE_AND);
        USOCKET_DIAG("wait start end, flag vlaue =0x%x\n",usocket_flag.value);
        cyg_flag_maskbits(&usocket_flag, ~CYG_FLG_USOCKET_IDLE);
        USOCKET_DIAG("flag vlaue =0x%x\n",usocket_flag.value);
        */

        // set timeout to 100 ms
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        // copy it
        readfds = master;
        ret = select(fdmax+1,&readfds,NULL,NULL,&tv);
        switch(ret)
        {
            case -1:
                perror("select");
                break;
            case 0:
                //USOCKET_DIAG("select timeout\n");
                break;
            default:
                // run throuth the existing connections looking for data to read
                USOCKET_DIAG("fdmax %d  readfds 0x%x\n",fdmax,readfds);
                for (i=0; i<= fdmax; i++)
                {
                    // one data coming
                    if (FD_ISSET(i, &readfds))
                    {
                        // handle new connection
                        if (i== server_socket)
                        {
                            USOCKET_DIAG("accept begin %d\n",i);
                            client_socket = accept( server_socket, (struct sockaddr *)&client_address, &calen );
                            USOCKET_DIAG("accept end %d, client_socket=%d\n",i,client_socket);

                            if (client_socket >=0)
                            {
                                ret = setsockopt(client_socket , SOL_SOCKET, SO_SNDBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
                                if (ret == -1)
                                    printf("Couldn't setsockopt(SO_SNDBUF)\n");
                                ret = setsockopt(client_socket , SOL_SOCKET, SO_RCVBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
                                if (ret == -1)
                                    printf("Couldn't setsockopt(SO_RCVBUF)\n");

                                flag = 1;
                                ret = setsockopt( client_socket, SOL_SOCKET, SO_OOBINLINE, (char *)&flag, sizeof(int) );
                                if (ret == -1)
                                    printf("Couldn't setsockopt(SO_OOBINLINE)\n");
                                flag = 1;
                                // set socket non-block
                                if (IOCTL( client_socket, FIONBIO, &flag ) != 0)
                                {
                                    printf("%s: IOCTL errno = %d\r\n", __func__, errno);
                                }

                                USOCKET_DIAG("client_address %x \r\n",client_address.sin_addr.s_addr );

                                // client has request
                                if (pusocket_obj->notify)
                                    pusocket_obj->notify(CYG_USOCKET_STATUS_CLIENT_CONNECT,client_address.sin_addr.s_addr );


                                if(pusocket_obj->client_socket!=-1)
                                {
                                    USOCKET_DIAG("close org client_socket %d \n",pusocket_obj->client_socket);
                                    FD_CLR(pusocket_obj->client_socket, &master);
                                    USOCKET_DIAG("FD_ISSET(client_socket, &readfds) %d clr+\n",FD_ISSET(pusocket_obj->client_socket, &readfds));
                                    FD_CLR(pusocket_obj->client_socket, &readfds);
                                    USOCKET_DIAG("FD_ISSET(client_socket, &readfds) %d clr-\n",FD_ISSET(pusocket_obj->client_socket, &readfds));
                                    SOCKET_CLOSE(pusocket_obj->client_socket);
                                }
                                // add to master set
                                USOCKET_DIAG("FD_SET %d\n",client_socket);
                                FD_SET(client_socket, &master);
                                // keep track of the max
                                pusocket_obj->client_socket = client_socket;
                                if (client_socket > fdmax)
                                {
                                    fdmax = client_socket;
                                }

                                USOCKET_DIAG("client_socket %x %x master 0x%x ,fdmax 0x%x\n",pusocket_obj, pusocket_obj->client_socket,master,fdmax);

                            }
                        }
                        // handle data from a client
                        else
                        {
                            USOCKET_DIAG("handle data from a client %d\n",i);
                            // client has request
                            {
                                if (pusocket_obj->notify)
                                    pusocket_obj->notify(CYG_USOCKET_STATUS_CLIENT_REQUEST,0);
                                ret = usocket_process(i, (struct sockaddr *)&client_address);
                                // if connection close
                                if (ret)
                                {
                                    USOCKET_DIAG("FD_CLR %d\n",i);
                                    FD_CLR(i, &master);
                                    pusocket_obj->client_socket=-1;
                                    USOCKET_DIAG("disconnect %x %x\n",pusocket_obj, pusocket_obj->client_socket);

                                    if (pusocket_obj->notify)
                                        pusocket_obj->notify(CYG_USOCKET_STATUS_CLIENT_DISCONNECT,client_address.sin_addr.s_addr );

                                }
                            }
                        }
                    }
                }
        }
    }
    // close all the opened sockets
    for (i=0; i<= fdmax; i++)
    {
        if (FD_ISSET(i, &master))
        {
            SOCKET_CLOSE(i);
            USOCKET_DIAG("close socket %d\n",i);
        }
    }
//usocket_server_exit:
    // reset the server socket value
    server_socket = -1;

#if 0
    // free allocated memory
    if (g_Sendbuff)
    {
        free(g_Sendbuff);
        USOCKET_DIAG("free g_Sendbuff = 0x%x\r\n",g_Sendbuff);
        g_Sendbuff = NULL;
    }
#endif
    //cyg_flag_setbits(&usocket_flag, CYG_FLG_USOCKET_IDLE);
    FLAG_SETPTN(usocket_cond, usocket_flag, CYG_FLG_USOCKET_IDLE, usocket_mtx);

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
static void usocket_init(cyg_addrword_t arg)
#else
static void* usocket_init(void* arg)
#endif
{
    int err = 0, flag;
    struct addrinfo *res;
    char   portNumStr[10];
    int    portNum = usocket_obj.portNum;
    int    sock_buf_size = USOCKET_SOCK_BUF_SIZE;

    USOCKET_DIAG("usocket_init\n");

    memset(&server_address,0,sizeof(server_address));
    //server_address.ai_family = AF_UNSPEC; // user IPv4 or IPv6 whichever
    server_address.ai_family = AF_INET;
    server_address.ai_socktype = SOCK_STREAM;
    server_address.ai_flags = AI_PASSIVE; // fill in my IP for me

    // check if port number valid
    if (portNum > CYGNUM_MAX_PORT_NUM)
    {
        printf("error port number %d\r\n",portNum);
        return DUMMY_NULL;
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
        return DUMMY_NULL;
    }

    /* Get the network going. This is benign if the application has
     * already done this.
     */
    //USOCKET_DIAG("init_all_network_interfaces\r\n");
    //init_all_network_interfaces();

    /* Create and bind the server socket.
     */
    USOCKET_DIAG("create socket\r\n");
    server_socket = socket( res->ai_family, res->ai_socktype, res->ai_protocol );

    #if 0
    if (IOCTL(server_socket, SIOCGIFADDR, (char *)&ifreq) >= 0)
    {
        in_addr = (struct sockaddr_in *)&(ifreq.ifr_addr);
        USOCKET_DIAG("ipstr = %s\r\n",inet_ntoa(in_addr->sin_addr));
    }
    #endif

    USOCKET_DIAG("server_socket = %d\r\n",server_socket);
    CYG_ASSERT( server_socket >= 0, "Socket create failed");

    /* Disable the Nagle (TCP No Delay) Algorithm */
    #if 0
    flag = 1;
    err = setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
    if (err == -1)
        printf("Couldn't setsockopt(TCP_NODELAY)\n");
    #endif
    #if 1
    sock_buf_size = usocket_obj.sockbufSize;
    flag = 1;
    err = setsockopt( server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));
    if (err == -1)
        printf("Couldn't setsockopt(SO_REUSEADDR)\n");

    #if defined(__ECOS)
    err = setsockopt( server_socket, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
    #else
    #if defined (SO_REUSEPORT)
    err = setsockopt( server_socket, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
    #endif
    #endif
    if (err == -1)
        printf("Couldn't setsockopt(SO_REUSEPORT)\n");

    err = setsockopt(server_socket , SOL_SOCKET, SO_SNDBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
    if (err == -1)
        printf("Couldn't setsockopt(SO_SNDBUF)\n");

    err = setsockopt(server_socket , SOL_SOCKET, SO_RCVBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
    if (err == -1)
        printf("Couldn't setsockopt(SO_RCVBUF)\n");


    #endif

    USOCKET_DIAG("bind %d\r\n",portNum);
    err = bind( server_socket, res->ai_addr, res->ai_addrlen);
    CYG_ASSERT( err == 0, "bind() returned error");

    USOCKET_DIAG("listen\r\n");
    err = listen( server_socket, SOMAXCONN );
    CYG_ASSERT( err == 0, "listen() returned error" );

    freeaddrinfo(res);   // free the linked list

    /* Now go be a server ourself.
     */
    usocket_server(arg);

    return DUMMY_NULL;
}

/* ================================================================= */
/* System initializer
 *
 * This is called from the static constructor in init.cxx. It spawns
 * the main server thread and makes it ready to run. It can also be
 * called explicitly by the application if the auto start option is
 * disabled.
 */

__externC void usocket_open(void)
{
    int ret =0;
    if (!isServerStart)
    {
        isStopServer = false;
        COND_CREATE(usocket_cond);
        MUTEX_CREATE(usocket_mtx,1);
        USOCKET_DIAG("usocket_startup create thread\r\n");
        #if defined(__ECOS)
        ret = cyg_thread_create( usocket_obj.threadPriority,
                           usocket_init,
                           0,
                           "user socket",
                           &usocket_stacks[0],
                           sizeof(usocket_stacks),
                           &usocket_thread,
                           &usocket_thread_object
            );
        USOCKET_DIAG("usocket_startup resume thread\r\n");
        cyg_thread_resume( usocket_thread );
        #else
        ret = pthread_create(&usocket_thread, NULL , usocket_init , NULL);
        #endif
        if (ret < 0) {
            printf("create feed_bs_thread failed\n");
        } else {
            isServerStart = true;
        }
    }
}

__externC void usocket_close(void)
{
    if (isServerStart)
    {
        isStopServer = true;
        FLAG_CLRPTN(usocket_flag,CYG_FLG_USOCKET_START,usocket_mtx);
        USOCKET_DIAG("usocket_stop wait idle begin\r\n");
        //cyg_flag_wait(&usocket_flag,CYG_FLG_USOCKET_IDLE,CYG_FLAG_WAITMODE_AND);
        FLAG_WAITPTN(usocket_cond,usocket_flag,CYG_FLG_USOCKET_IDLE,usocket_mtx,false);
        USOCKET_DIAG("usocket_stop wait idle end\r\n");
        USOCKET_DIAG("usocket_stop ->suspend thread\r\n");
        #if defined(__ECOS)
        cyg_thread_suspend(usocket_thread);
        USOCKET_DIAG("usocket_stop ->delete thread\r\n");
        cyg_thread_delete(usocket_thread);
        #else
        pthread_join(usocket_thread, NULL);
        #endif
        //==== FLAG_DESTROY(usocket_flag,usocket_mtx);
        COND_DESTROY(usocket_cond);
        MUTEX_DESTROY(usocket_mtx);
        isServerStart = false;
        USOCKET_DIAG("usocket_stop ->end\r\n");
    }
}

__externC int usocket_send(char* buf, int* size)
{
    int ret =-1;
    int i=0;

    USOCKET_DIAG("usocket_send client_socket %d fdmax 0x%x\r\n",usocket_obj.client_socket,fdmax);
    if((buf)&&(*size))
    {
        USOCKET_DIAG("send %s %d client_socket %d master %x\n",buf,*size,usocket_obj.client_socket,master);
        for (i=0; i<= fdmax; i++)
        {
            if (FD_ISSET(i, &master))
            {
                if(i!=server_socket)
                {
                    USOCKET_DIAG("send %s %d client_socket %d\n",buf,*size,i);
                    ret = sendall(i,buf, size);
                }
            }
        }
    }

    return ret;
}

#if 0
__externC int usocket_recv(usocket_install_obj *pObj,char* addr, int* size)
{
    int ret =0;
    usocket_install_obj* pusocket_obj;
    pusocket_obj = &usocket_obj;

    USOCKET_DIAG("usocket_recv\r\n");
    #if 0
    if(pusocket_obj->client_socket)
    {
        USOCKET_DIAG("recv %s %d client_socket %d\n",addr,*size,pusocket_obj->client_socket);
        if(*size)
            ret = recvall(pusocket_obj->client_socket,addr, size,true);
    }
    #endif
    return ret;
}
#endif
__externC void usocket_install( usocket_install_obj*  pObj)
{
    usocket_obj.client_socket = -1;
    usocket_obj.notify = pObj->notify;
    usocket_obj.recv = pObj->recv;
    if (!pObj->portNum)
    {
        usocket_obj.portNum = USOCKET_SERVER_PORT;
        printf("error Port, set default port %d\r\n",USOCKET_SERVER_PORT);
    }
    else
    {
        usocket_obj.portNum = pObj->portNum;
    }
    if (!pObj->threadPriority)
    {
        usocket_obj.threadPriority = USOCKET_THREAD_PRIORITY;
        printf("error threadPriority, set default %d\r\n",USOCKET_THREAD_PRIORITY);
    }
    else
    {
        usocket_obj.threadPriority = pObj->threadPriority;
    }
    if (!pObj->sockbufSize)
    {
        usocket_obj.sockbufSize = USOCKET_SOCK_BUF_SIZE;
        printf("error sockSendbufSize %d, set default %d \r\n",pObj->sockbufSize,USOCKET_SOCK_BUF_SIZE);
    }
    else
    {
        usocket_obj.sockbufSize = pObj->sockbufSize;
    }

}

__externC void usocket_uninstall(void)
{
    memset(&usocket_obj,0,sizeof(usocket_install_obj));
}
static int sendall(int s, char* buf, int *len)
{
    int total = 0;
    int bytesleft = *len;
    int n = -1;


    USOCKET_DIAG("sendall %d bytes\n",*len);
    while (total < *len)
    {
        n = send(s, buf+total, bytesleft, 0);
        //USOCKET_DIAG("send %d bytes\n",n);
        if (n==-1)
            break;
        total +=n;
        bytesleft -=n;
    }
    *len = total;
    return n==-1?-1:0;
}

static int recvall(int s, char* buf, int len, int isJustOne)
{
    int total = 0;
    int bytesleft = len;
    int n = -1;
    int ret;
    int idletime;

    USOCKET_DIAG("recvall buf = 0x%x, len = %d bytes\n",(int)buf,len);
    while (total < len && (!isStopServer))
    {
        n = recv(s, buf+total, bytesleft, 0);
        if ( n == 0)
        {
            // client close the socket
            return 0;
        }
        if ( n < 0 )
        {
            if ( errno == EAGAIN )
            {
                idletime = 0;
                while(!isStopServer)
                {
                    /* wait idletime seconds for progress */
                    fd_set rs;
                    struct timeval tv;

                    FD_ZERO( &rs );
                    FD_SET( s, &rs );
                    tv.tv_sec = 0;
                    tv.tv_usec = CYGNUM_SELECT_TIMEOUT_USEC;
                    ret = select( s + 1, &rs, NULL, NULL, &tv );
                    if (ret == 0)
                    {
                        idletime++;
                        //USOCKET_DIAG("idletime=%d\r\n",idletime);
                        printf("idletime=%d\r\n",idletime);
                        if (idletime >= USOCKET_SERVER_IDLE_TIME)
                        {
                            perror("select\r\n");
                            return -1;
                        }
                        #if 0 //[141338]coverity,because USOCKET_SERVER_IDLE_TIME is 1
                        else
                            continue;
                        #endif
                    }
                    else if (ret <0)
                    {
                        perror("select\r\n");
                        return -1;
                    }
                    if (FD_ISSET( s, &rs ))
                    {
                        //USOCKET_DIAG("recvall FD_ISSET\r\n");
                        n = 0;
                        break;
                    }
                }
            }
        }
        if (n==-1)
            break;
        total +=n;
        bytesleft -=n;
        if (isJustOne && total)
            break;
    }
    USOCKET_DIAG("recvall end %d %s\n",n,buf);
    return n<0?-1:total;
}
/* ----------------------------------------------------------------- */
/* end of usocket.c                                                    */
