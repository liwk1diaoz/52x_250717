/* =================================================================
 *
 *      usocket_multi.h
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
//#include <dirent.h>

#if defined(__ECOS)
#include <network.h>
#include <arpa/inet.h>
#include <cyg/usocket/usocket.h>
#elif defined(__LINUX)
#include <sys/types.h>
#include <usocket.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <pthread.h>
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



#if defined(__USE_IPC)
#include "usocket_ipc.h"
#endif

#define USOCKET_SERVER_PORT 3333
#define USOCKET_THREAD_PRIORITY 6
#define USOCKET_THREAD_STACK_SIZE 5120
#if defined(__USE_IPC)
#define USOCKET_SERVER_BUFFER_SIZE USOCKIPC_TRANSFER_BUF_SIZE
#else
#define USOCKET_SERVER_BUFFER_SIZE (2*1024)
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


// HTTP animation mode
#define ANIMATION_MODE_SERVER_PUSH                      0
#define ANIMATION_MODE_CLIENT_PULL_AUTO_REFRESH         1
#define ANIMATION_MODE_CLIENT_PULL_MANUAL_REFRESH       2


#define MAX_USOCKET_NUMBER 5

static struct addrinfo server_address[MAX_USOCKET_NUMBER];

static int server_socket[MAX_USOCKET_NUMBER] = {-1};

static usocket_install_obj_multi usocket_obj_list[MAX_USOCKET_NUMBER]={0};

//static char* g_Sendbuff = NULL;

#if defined(__ECOS)
static cyg_flag_t usocket_flag[MAX_USOCKET_NUMBER];
#else
static volatile int usocket_flag[MAX_USOCKET_NUMBER];
#endif

static bool isServerStart[MAX_USOCKET_NUMBER] = {false}, isStopServer[MAX_USOCKET_NUMBER] = {false};
static int fdmax[MAX_USOCKET_NUMBER];
static fd_set master[MAX_USOCKET_NUMBER];  // master file descriptor list

/* ================================================================= */
/* Thread stacks, etc.
 */

#if defined(__ECOS)
static cyg_uint8 usocket_stacks[MAX_USOCKET_NUMBER][USOCKET_SERVER_BUFFER_SIZE+
                              USOCKET_THREAD_STACK_SIZE] = {0};
#endif

#if defined(__ECOS)
static cyg_handle_t usocket_thread[MAX_USOCKET_NUMBER];
static cyg_thread   usocket_thread_object[MAX_USOCKET_NUMBER];
#else
static pthread_t    usocket_thread[MAX_USOCKET_NUMBER];
#endif

static int recvall_multi(int s, char* buf, int len, int isJustOne, int obj_index);
static int sendall_multi(int s, char* buf, int *len, int obj_index);

/* ================================================================= */
/* Main processing function                                          */
/*                                                                   */
/* Reads the HTTP header, look it up in the table and calls the      */
/* handler.                                                          */

// this function will return true, if the connection is closed.
static cyg_bool usocket_process_multi( int client_socket, struct sockaddr *client_address, int obj_index )
{
    #if _TODO
    int calen = sizeof(*client_address);
    char name[64];
    char port[10];
    #endif
    int  ret = 0;
    char request[USOCKET_SERVER_BUFFER_SIZE];

    #if _TODO
    getnameinfo(client_address, calen, name, sizeof(name),
                port, sizeof(port), NI_NUMERICHOST|NI_NUMERICSERV);
    USOCKET_DIAG("Connection from %s[%s]\n",name,port);
    #endif
    //ret = recv(client_socket,request, sizeof(request), 0);
    //usocket_set_sock_nonblock(client_socket);
    memset(request,0,USOCKET_SERVER_BUFFER_SIZE);
    ret = recvall_multi(client_socket,request, sizeof(request),true,obj_index);

    // the connection is closed by client, so we close the client_socket
    if (ret == 0)
    {
        #if _TODO
        USOCKET_DIAG("Connection closed by client %s[%s]\n",name,port);
        #endif
        close(client_socket);
        return true;
    }
    if (ret == -1)
    {
        perror("recv time out");
        //close(client_socket);
        #if 0
        sprintf(request,"hello\r\n");
        len = strlen(request);
        USOCKET_DIAG("send %s %d\n",request,len);
        ret = sendall(client_socket,request, &len);
        #endif
        return false;
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
        if(usocket_obj_list[obj_index].recv)
        {
            ((usocket_recv_multi *)usocket_obj_list[obj_index].recv)(request,ret,obj_index);
        }
        #endif
        return false;
    }
}

static void usocket_server_multi( int obj_index )
{
    int client_socket, sock_buf_size = USOCKET_SOCK_BUF_SIZE , flag;
    struct sockaddr_in client_address;
    socklen_t calen = sizeof(client_address);
    fd_set readfds; // temp file descriptor list for select()
    int i;
    int ret;
    struct timeval tv;
    usocket_install_obj_multi* pusocket_obj;

    pusocket_obj = &usocket_obj_list[obj_index];

    sock_buf_size = pusocket_obj->sockbufSize;

    USOCKET_DIAG("usocket_server[%d] , sock_buf_size  =%d\r\n",obj_index,sock_buf_size);

    // clear the set
    FD_ZERO(&master[obj_index]);
    FD_ZERO(&readfds);
    // add server socket to the set
    FD_SET(server_socket[obj_index], &master[obj_index]);

    fdmax[obj_index] = server_socket[obj_index];
    // set timeout to 100 ms
    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    cyg_flag_maskbits(&usocket_flag[obj_index], 0);
    cyg_flag_setbits(&usocket_flag[obj_index], CYG_FLG_USOCKET_START);
    while(!isStopServer[obj_index])
    {


        // copy it
        readfds = master[obj_index];
        ret = select(fdmax[obj_index]+1,&readfds,NULL,NULL,&tv);
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
                USOCKET_DIAG("fdmax[%d] %d  readfds 0x%x\n",obj_index,fdmax[obj_index],readfds);
                for (i=0; i<= fdmax[obj_index]; i++)
                {
                    // one data coming
                    if (FD_ISSET(i, &readfds))
                    {
                        // handle new connection
                        if (i== server_socket[obj_index])
                        {
                            USOCKET_DIAG("accept[%d] begin %d\n",obj_index,i);
                            client_socket = accept( server_socket[obj_index], (struct sockaddr *)&client_address, &calen );
                            USOCKET_DIAG("accept[%d] end %d, client_socket=%d\n",obj_index,i,client_socket);
                            if (client_socket >=0)
                            {
                                ret = setsockopt(client_socket , SOL_SOCKET, SO_SNDBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
                                if (ret == -1)
                                    printf("Couldn't setsockopt(SO_SNDBUF)[%d]\n",obj_index);
                                ret = setsockopt(client_socket , SOL_SOCKET, SO_RCVBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
                                if (ret == -1)
                                    printf("Couldn't setsockopt(SO_RCVBUF)[%d]\n",obj_index);

                                flag = 1;
                                ret = setsockopt( client_socket, SOL_SOCKET, SO_OOBINLINE, (char *)&flag, sizeof(int) );
                                if (ret == -1)
                                    printf("Couldn't setsockopt(SO_OOBINLINE)[%d]\n",obj_index);
                                flag = 1;
                                // set socket non-block
                                if (ioctl( client_socket, FIONBIO, &flag ) != 0)
                                {
                                    printf("%s: ioctl errno = %d\r\n", __func__, errno);
                                }

                                USOCKET_DIAG("client_address[%d] %x \r\n",obj_index,client_address.sin_addr.s_addr );

                                // client has request
                                if (pusocket_obj->notify)
                                    pusocket_obj->notify(CYG_USOCKET_STATUS_CLIENT_CONNECT,client_address.sin_addr.s_addr,obj_index );

                                if(pusocket_obj->client_socket!=-1)
                                {
                                    USOCKET_DIAG("close org client_socket %d \n",pusocket_obj->client_socket);
                                    FD_CLR(pusocket_obj->client_socket, &master[obj_index]);
                                    USOCKET_DIAG("FD_ISSET(client_socket, &readfds) %d clr+\n",FD_ISSET(pusocket_obj->client_socket, &readfds));
                                    FD_CLR(pusocket_obj->client_socket, &readfds);
                                    USOCKET_DIAG("FD_ISSET(client_socket, &readfds) %d clr-\n",FD_ISSET(pusocket_obj->client_socket, &readfds));
                                    close(pusocket_obj->client_socket);
                                }

                                // add to master set
                                USOCKET_DIAG("FD_SET[%d] %d\n",obj_index,client_socket);
                                FD_SET(client_socket, &master[obj_index]);
                                // keep track of the max
                                pusocket_obj->client_socket = client_socket;
                                if (client_socket > fdmax[obj_index])
                                {
                                    fdmax[obj_index] = client_socket;
                                }

                                USOCKET_DIAG("client_socket[%d] %x %x master 0x%x ,fdmax 0x%x\n",obj_index,pusocket_obj, pusocket_obj->client_socket,master[obj_index],fdmax[obj_index]);

                            }
                        }
                        // handle data from a client
                        else
                        {
                            USOCKET_DIAG("handle data from a client[%d] %d\n",obj_index,i);
                            // client has request
                            {
                                if (pusocket_obj->notify)
                                    pusocket_obj->notify(CYG_USOCKET_STATUS_CLIENT_REQUEST,0,obj_index);
                                ret = usocket_process_multi(i, (struct sockaddr *)&client_address,obj_index);
                                // if connection close
                                if (ret)
                                {
                                    USOCKET_DIAG("FD_CLR %d\n",i);
                                    FD_CLR(i, &master[obj_index]);
                                    pusocket_obj->client_socket=-1;
                                    USOCKET_DIAG("disconnect %x %x\n",pusocket_obj, pusocket_obj->client_socket);

                                    if (pusocket_obj->notify)
                                        pusocket_obj->notify(CYG_USOCKET_STATUS_CLIENT_DISCONNECT,0,obj_index);

                                }
                            }
                        }
                    }
                }
        }
    }
    // close all the opened sockets
    for (i=0; i<= fdmax[obj_index]; i++)
    {
        if (FD_ISSET(i, &master[obj_index]))
        {
            close(i);
            USOCKET_DIAG("close socket %d\n",i);
        }
    }
//usocket_server_exit:
    // reset the server socket value
    server_socket[obj_index] = -1;

#if 0
    // free allocated memory
    if (g_Sendbuff)
    {
        free(g_Sendbuff);
        USOCKET_DIAG("free g_Sendbuff = 0x%x\r\n",g_Sendbuff);
        g_Sendbuff = NULL;
    }
#endif
    cyg_flag_setbits(&usocket_flag[obj_index], CYG_FLG_USOCKET_IDLE);
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
static void usocket_init_multi(cyg_addrword_t arg)
#else
static void* usocket_init_multi(void* arg)
#endif
{
    int err = 0, flag;
	#if (defined(__ECOS)|| defined(__FREERTOS))
	int obj_index = (int) arg;
	#else
	int obj_index = (int) *arg;
	#endif
    struct addrinfo *res;
    char   portNumStr[10];
    int    portNum = usocket_obj_list[obj_index].portNum;
    int    sock_buf_size = USOCKET_SOCK_BUF_SIZE;

    USOCKET_DIAG("usocket_init[%d]\n",obj_index);

    memset(&server_address[obj_index],0,sizeof(server_address[obj_index]));
    //server_address.ai_family = AF_UNSPEC; // user IPv4 or IPv6 whichever
    server_address[obj_index].ai_family = AF_INET;
    server_address[obj_index].ai_socktype = SOCK_STREAM;
    server_address[obj_index].ai_flags = AI_PASSIVE; // fill in my IP for me

    // check if port number valid
    if (portNum > CYGNUM_MAX_PORT_NUM)
    {
        printf("error port number %d\r\n",portNum);
        return DUMMY_NULL;
    }
    sprintf(portNumStr, "%d", portNum);
    err = getaddrinfo(NULL, portNumStr, &server_address[obj_index], &res);
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
    USOCKET_DIAG("create socket[%d]\r\n",obj_index);
    server_socket[obj_index] = socket( res->ai_family, res->ai_socktype, res->ai_protocol );


    USOCKET_DIAG("server_socket[%d] = %d\r\n",obj_index,server_socket[obj_index]);
    CYG_ASSERT( server_socket[obj_index] >= 0, "Socket create failed");

    /* Disable the Nagle (TCP No Delay) Algorithm */
    #if 0
    flag = 1;
    err = setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
    if (err == -1)
        printf("Couldn't setsockopt(TCP_NODELAY)\n");
    #endif
    #if 1
    sock_buf_size = usocket_obj_list[obj_index].sockbufSize;
    flag = 1;
    err = setsockopt( server_socket[obj_index], SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));
    if (err == -1)
        printf("Couldn't setsockopt(SO_REUSEADDR)[%d]\n",obj_index);

    #if defined(__ECOS)
    err = setsockopt( server_socket[obj_index], SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
    #else
    #if defined (SO_REUSEPORT)
    err = setsockopt( server_socket[obj_index], SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
    #endif
    #endif
    if (err == -1)
        printf("Couldn't setsockopt(SO_REUSEPORT)[%d]\n",obj_index);

    err = setsockopt(server_socket[obj_index] , SOL_SOCKET, SO_SNDBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
    if (err == -1)
        printf("Couldn't setsockopt(SO_SNDBUF)[%d]\n",obj_index);

    err = setsockopt(server_socket[obj_index] , SOL_SOCKET, SO_RCVBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
    if (err == -1)
        printf("Couldn't setsockopt(SO_RCVBUF)[%d]\n",obj_index);


    #endif

    USOCKET_DIAG("bind %d\r\n",portNum);
    err = bind( server_socket[obj_index], res->ai_addr, res->ai_addrlen);
    CYG_ASSERT( err == 0, "bind() returned error");

    USOCKET_DIAG("listen\r\n");
    err = listen( server_socket[obj_index], SOMAXCONN );
    CYG_ASSERT( err == 0, "listen() returned error" );

    freeaddrinfo(res);   // free the linked list

    /* Now go be a server ourself.
     */
    usocket_server_multi(obj_index);

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

__externC void usocket_open_multi(int obj_index)
{
    int ret =0;
    if (!isServerStart[obj_index])
    {
        isStopServer[obj_index] = false;
        cyg_flag_init(&usocket_flag[obj_index]);
        USOCKET_DIAG("usocket_startup create thread %d\r\n",obj_index);
        #if defined(__ECOS)
        ret = cyg_thread_create( usocket_obj_list[obj_index].threadPriority,
                           usocket_init_multi,
                           obj_index,
                           "user socket",
                           &usocket_stacks[obj_index][0],
                           sizeof(usocket_stacks[obj_index]),
                           &usocket_thread[obj_index],
                           &usocket_thread_object[obj_index]
            );
        USOCKET_DIAG("usocket_startup resume thread\r\n");
        cyg_thread_resume( usocket_thread[obj_index] );
        #else
        ret = pthread_create(&usocket_thread[obj_index], NULL , usocket_init_multi , (void *)obj_index);
        #endif
        if (ret < 0) {
            printf("create feed_bs_thread failed\n");
        } else {
            isServerStart[obj_index] = true;
        }
    }
}

__externC void usocket_uninstall_multi(int obj_index)
{
    memset(&usocket_obj_list[obj_index],0,sizeof(usocket_install_obj_multi));
}

__externC void usocket_close_multi(int obj_index)
{
    if (isServerStart[obj_index])
    {
		usocket_uninstall_multi(obj_index);
        isStopServer[obj_index] = true;
        cyg_flag_maskbits(&usocket_flag[obj_index],~CYG_FLG_USOCKET_START);
        USOCKET_DIAG("usocket_stop[%d] wait idle begin\r\n",obj_index);
        cyg_flag_wait(&usocket_flag[obj_index],CYG_FLG_USOCKET_IDLE,CYG_FLAG_WAITMODE_AND);
        USOCKET_DIAG("usocket_stop[%d] wait idle end\r\n",obj_index);
        USOCKET_DIAG("usocket_stop[%d] ->suspend thread\r\n",obj_index);
        #if defined(__ECOS)
        cyg_thread_suspend(usocket_thread[obj_index]);
        USOCKET_DIAG("usocket_stop[%d] ->delete thread\r\n",obj_index);
        cyg_thread_delete(usocket_thread[obj_index]);
        #else
        pthread_join(usocket_thread[obj_index], NULL);
        #endif
        cyg_flag_destroy(&usocket_flag[obj_index]);
        isServerStart[obj_index] = false;
        USOCKET_DIAG("usocket_stop[%d] ->end\r\n",obj_index);
    }
}


__externC int usocket_send_multi(char* buf, int* size, int obj_index)
{
    int ret =-1;
    int i=0;
    usocket_install_obj_multi* pusocket_obj;
    pusocket_obj = &usocket_obj_list[obj_index];
    USOCKET_DIAG("usocket_send[%d] client_socket %d fdmax 0x%x\r\n",obj_index,pusocket_obj->client_socket,fdmax[obj_index]);
    if((buf)&&(*size))
    {
        USOCKET_DIAG("send %s %d client_socket %d master %x\n",buf,*size,pusocket_obj->client_socket,master[obj_index]);
        for (i=0; i<= fdmax[obj_index]; i++)
        {
            if (FD_ISSET(i, &master[obj_index]))
            {
                if(i!=server_socket[obj_index])
                {
                    USOCKET_DIAG("send %s %d client_socket %d\n",buf,*size,i);
                    ret = sendall_multi(i,buf, size,obj_index);
                    pusocket_obj->client_socket =i;
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
__externC int usocket_install_multi( usocket_install_obj_multi*  pObj,int obj_index)
{
    #if 0
	int i;
	int obj_index=-1;
    //i =1 in 96660,reserved 0 for origin usocket
	for(i = 1 ; i < MAX_USOCKET_NUMBER;i++){
		if(usocket_obj_list[i].occupy == 0){
			obj_index = i;
			break;

		}
	}
    #endif
	if((obj_index <(0)||(obj_index >MAX_USOCKET_NUMBER-1))){
		printf("obj_index %d out range\r\n",obj_index);
		return -1;
	}
	if(usocket_obj_list[obj_index].occupy == 1){
		printf("obj_index %d is used\r\n",obj_index);
		return -1;
	}
	usocket_obj_list[obj_index].occupy = 1;
    usocket_obj_list[obj_index].client_socket = -1;
    usocket_obj_list[obj_index].notify = pObj->notify;
    usocket_obj_list[obj_index].recv = pObj->recv;
    if (!pObj->portNum)
    {
        usocket_obj_list[obj_index].portNum = USOCKET_SERVER_PORT;
        printf("error Port, set default port %d\r\n",USOCKET_SERVER_PORT);
    }
    else
    {
        usocket_obj_list[obj_index].portNum = pObj->portNum;
    }
    if (!pObj->threadPriority)
    {
        usocket_obj_list[obj_index].threadPriority = USOCKET_THREAD_PRIORITY;
        printf("error threadPriority, set default %d\r\n",USOCKET_THREAD_PRIORITY);
    }
    else
    {
        usocket_obj_list[obj_index].threadPriority = pObj->threadPriority;
    }
    if (!pObj->sockbufSize)
    {
        usocket_obj_list[obj_index].sockbufSize = USOCKET_SOCK_BUF_SIZE;
        printf("error sockSendbufSize %d, set default %d \r\n",pObj->sockbufSize,USOCKET_SOCK_BUF_SIZE);
    }
    else
    {
        usocket_obj_list[obj_index].sockbufSize = pObj->sockbufSize;
    }
	return obj_index;

}

__externC int usocket_get_obj_index(int port){

	if(port > 65535 || port <=0 ){
		printf("port error port=%d\r\n",port);
		return -1;
	}
	int i;
	for(i=0; i<MAX_USOCKET_NUMBER ; i++){
		if(usocket_obj_list[i].occupy == 1 && usocket_obj_list[i].portNum == port){
			return i;

		}
	}
	printf("can not find usocket object port=%d\r\n",port);
	return -1;
}


static int sendall_multi(int s, char* buf, int *len,int obj_index)
{
    int total = 0;
    int bytesleft = *len;
    int n = -1;


    USOCKET_DIAG("sendall[%d] %d bytes\n",obj_index,*len);
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

static int recvall_multi(int s, char* buf, int len, int isJustOne, int obj_index)
{
    int total = 0;
    int bytesleft = len;
    int n = -1;
    int ret;
    int idletime;

    USOCKET_DIAG("recvall buf = 0x%x, len = %d bytes\n",(int)buf,len);
    while (total < len && (!isStopServer[obj_index]))
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
                while(!isStopServer[obj_index])
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
                        #if 0 //[141338]coverity,decause USOCKET_SERVER_IDLE_TIME is 1
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
    USOCKET_DIAG("recvall end\n");
    return n<0?-1:total;
}
/* ----------------------------------------------------------------- */
/* end of usocket.c                                                    */
