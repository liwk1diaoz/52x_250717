/* =================================================================
 *
 *      usocket_cli.h
 *
 *      A simple user socket client
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
#include "usocket_cli_int.h"

#if defined(__ECOS)
#include <network.h>
#include <arpa/inet.h>
#include <cyg/usocket_cli/usocket_cli.h>
#elif defined(__LINUX_USER__)
#include <sys/types.h>
#include <sys/socket.h>
#include <UsockCliIpc/usocket_cli.h>
#include <netdb.h>
#include <sys/ioctl.h>
#elif defined(__FREERTOS)
#include <kwrap/type.h>
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#include <FreeRTOS_POSIX/dirent.h>
#include <kwrap/semaphore.h>
#include <kwrap/task.h>
#include <UsockCliIpc/usocket_cli.h>
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

#define USOCKETCLI_SERVER_PORT 6666
#define USOCKETCLI_THREAD_PRIORITY 6
#define USOCKETCLI_THREAD_STACK_SIZE 5120
#define USOCKETCLI_BUFFER_SIZE 1024

/* ================================================================= */

#if 0
#if defined(__ECOS)
#define USOCKETCLI_DIAG diag_printf
#else
#define USOCKETCLI_DIAG printf
#endif
#else
#define USOCKETCLI_DIAG(...)
#endif

/* ================================================================= */
/* socket address and file descriptor.
 */

#define CYGNUM_MAX_PORT_NUM             (65535)

#define USOCKETCLI_SOCK_BUF_SIZE        (8*1024)

#define CYGNUM_SELECT_TIMEOUT_USEC      (500000)

#define USOCKETCLI_IDLE_TIME            (1)  // retry 20

#define USOCKETCLI_TIMEOUT              (20)

// flag define
#define CYG_FLG_USOCKETCLI_START         (0x01)
#define CYG_FLG_USOCKETCLI_IDLE          (0x80)
#define MAX_CLIENT_NUM                   (2)

usocket_cli_obj gusocket_cli_obj[MAX_CLIENT_NUM]={0};

#if defined(__ECOS)
static cyg_flag_t usocket_cli_flag;
#elif defined(__LINUX_USER__)
static unsigned int   usocket_cli_flag;
#else
static volatile int    usocket_cli_flag;
#endif
static COND_HANDLE     usocket_cli_cond;
static MUTEX_HANDLE      usocket_cli_mtx;
static bool isStartClient = false, isStopClient = false;

/* ================================================================= */
/* Thread stacks, etc.
 */

#if defined(__ECOS)
static cyg_uint8 usocket_cli_stacks[USOCKETCLI_BUFFER_SIZE+
                              USOCKETCLI_THREAD_STACK_SIZE] = {0};
#endif

#if defined(__ECOS)
static cyg_handle_t usocket_cli_thread;
static cyg_thread   usocket_cli_thread_object;
#else
static pthread_t    usocket_cli_thread;
#endif

static int recvall(int s, char* buf, int len, int isJustOne);
static int sendall(int s, char* buf, int *len);
extern void usocket_cli_ipc_notify(usocket_cli_obj* obj,int status,int parm);
extern int usocket_cli_ipc_recv(usocket_cli_obj* obj,char* addr, int size);
extern void usocket_cli_disconnect(usocket_cli_obj *pusocket_cli_obj);


usocket_cli_obj *usocket_cli_get_newobj(void)
{
    int i=0;
    for(i=0;i<MAX_CLIENT_NUM;i++)
    {
        USOCKETCLI_DIAG("gusocket_cli_obj[%d].connect_socket %x\n",i,gusocket_cli_obj[i].connect_socket);

        if(gusocket_cli_obj[i].connect_socket== 0)
        {
            USOCKETCLI_DIAG("get %d gusocket_cli_obj %x\n",i,&gusocket_cli_obj[i]);
            return &gusocket_cli_obj[i];
        }
    }
    printf("err,no empty obj\n");
    return NULL;
}

/* ================================================================= */
/* Main processing function                                          */
/*                                                                   */

// this function will return true, if the connection is closed.
static cyg_bool usocket_cli_receive( int cur_socket,usocket_cli_obj *pusocket_cli_obj)
{
    char request[USOCKETCLI_BUFFER_SIZE];
    int  ret = 0;

    memset(request,0,USOCKETCLI_BUFFER_SIZE);
    ret = recvall(cur_socket,request, sizeof(request),true);

    if (ret == 0)
    {
        USOCKETCLI_DIAG("ret %d\n",ret);
        return true;
    }
    if (ret == -1)
    {
        perror("recv time out");
        return false;
    }
    else
    {
        if(pusocket_cli_obj->recv)
        {
            USOCKETCLI_DIAG("%s\r\n",request);
            usocket_cli_recv *recvcb = (usocket_cli_recv *)(pusocket_cli_obj->recv);
            ret = recvcb(request,ret);
            if(ret!=true){  //ture is ok, false is fail
                printf("cb ret %d",ret);
            }
        }
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
static void usocket_cli_proc(cyg_addrword_t arg)
#else
static void* usocket_cli_proc(void* arg)
#endif
{
    fd_set master;  // master file descriptor list
    fd_set readfds; // temp file descriptor list for select()
    int fdmax =0,i;
    int ret;
    struct timeval tv;
    usocket_cli_obj* pusocket_cli_obj =0;

    USOCKETCLI_DIAG("usocket_cli_proc\r\n");

    // clear the set
    FD_ZERO(&master);
    FD_ZERO(&readfds);

    // add socket to the set
    for(i=0;i<MAX_CLIENT_NUM;i++)
    {
        if(gusocket_cli_obj[i].connect_socket)
        {
            USOCKETCLI_DIAG("FD_SET %d\n",gusocket_cli_obj[i].connect_socket);
            FD_SET(gusocket_cli_obj[i].connect_socket, &master);
            if(gusocket_cli_obj[i].connect_socket>fdmax)
                fdmax = gusocket_cli_obj[i].connect_socket;
        }
    }

    //cyg_flag_maskbits(&usocket_cli_flag, 0);
    //cyg_flag_setbits(&usocket_cli_flag, CYG_FLG_USOCKETCLI_START);
    FLAG_SETPTN(usocket_cli_cond, usocket_cli_flag, CYG_FLG_USOCKETCLI_START, usocket_cli_mtx);

    while(!isStopClient)
    {
        // copy it
        readfds = master;
        // set timeout to 100 ms every time
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        ret = select(fdmax+1,&readfds,NULL,NULL,&tv);
        switch(ret)
        {
            case -1:
                perror("select");
                break;
            case 0:
                if(isStopClient)
                {
                    int j=0;
                    USOCKETCLI_DIAG("select timeout %x\n",pusocket_cli_obj);
                    for(j=0;j<MAX_CLIENT_NUM;j++)
                    {
                        if(gusocket_cli_obj[j].connect_socket)
                        {
                            USOCKETCLI_DIAG("connect_socket %d\n",gusocket_cli_obj[j].connect_socket);

                            usocket_cli_disconnect(&gusocket_cli_obj[j]);
                            usocket_cli_uninstall(&gusocket_cli_obj[j]);
                            FD_CLR(gusocket_cli_obj[j].connect_socket, &master);
                            break;
                        }
                    }
                }
                break;
            default:
                // run throuth the existing connections looking for data to read
                USOCKETCLI_DIAG("fdmax %d readfds %d\n",fdmax,readfds);
                for (i=0; i<= fdmax; i++)
                {
                    // one data coming
                    if (FD_ISSET(i, &readfds))
                    {
                        int j=0;
                        for(j=0;j<MAX_CLIENT_NUM;j++)
                        {
                            if(i==gusocket_cli_obj[j].connect_socket)
                            {
                                pusocket_cli_obj = &gusocket_cli_obj[j];
                                break;
                            }
                        }
                        USOCKETCLI_DIAG("handle data from server %d  %x notify %x\n",i,pusocket_cli_obj,pusocket_cli_obj->notify);
                        // client has request
                        {
                            if (pusocket_cli_obj->notify)
                                pusocket_cli_obj->notify(CYG_USOCKETCLI_STATUS_CLIENT_REQUEST,0);
                            ret = usocket_cli_receive(i,pusocket_cli_obj);
                            // if connection close
                            if (ret)
                            {
                                usocket_cli_disconnect(pusocket_cli_obj);
                                USOCKETCLI_DIAG("FD_CLR %d\n",i);
                                FD_CLR(i, &master);
                                if (pusocket_cli_obj->notify)
                                    pusocket_cli_obj->notify(CYG_USOCKETCLI_STATUS_CLIENT_DISCONNECT,0);
                                usocket_cli_uninstall(pusocket_cli_obj);

                            }
                        }
                    }
                }
        }
    }

    // process would stop,close all the opened sockets
    for (i=0; i<= fdmax; i++)
    {
        if (FD_ISSET(i, &master))
        {
            SOCKET_CLOSE(i);
            USOCKETCLI_DIAG("close socket %d\n",i);
        }
    }
    //cyg_flag_setbits(&usocket_cli_flag, CYG_FLG_USOCKETCLI_IDLE);
    FLAG_SETPTN(usocket_cli_cond, usocket_cli_flag, CYG_FLG_USOCKETCLI_IDLE, usocket_cli_mtx);

    return DUMMY_NULL;

}


int usocket_cli_connect(usocket_cli_obj *pusocket_cli_obj)
{
    int err = 0, flag;
    struct addrinfo *res;
    char   portNumStr[10];
    int    portNum = pusocket_cli_obj->portNum;
    int    sock_buf_size = USOCKETCLI_SOCK_BUF_SIZE;
    struct addrinfo hints;
    char   *ipStr=pusocket_cli_obj->svrIP;
    int     new_socket = 0;
    fd_set  myset;

    USOCKETCLI_DIAG("usocket_cli_connect\n");

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    sprintf(portNumStr, "%d", portNum);
    USOCKETCLI_DIAG("portNum %d ip %s\r\n",portNum,ipStr);
    err = getaddrinfo(ipStr, portNumStr, &hints, &res);
    if (err!=0)
    {
        #if _TODO
        printf("getaddrinfo: %s\n", gai_strerror(err));
        #else
        printf("getaddrinfo: %d\n", err);
        #endif
        return -1;
    }

    /* Create socket.
     */
    USOCKETCLI_DIAG("create socket\r\n");
    new_socket = socket( res->ai_family, res->ai_socktype, res->ai_protocol );

    USOCKETCLI_DIAG("new_socket = %d\r\n",new_socket);
    if(new_socket<0)
        perror("socket");
    CYG_ASSERT( new_socket >= 0, "Socket create failed");

    /* Disable the Nagle (TCP No Delay) Algorithm */
    #if 0
    flag = 1;
    err = setsockopt(new_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
    if (err == -1)
        printf("Couldn't setsockopt(TCP_NODELAY)\n");
    #endif
    #if 1
    sock_buf_size = pusocket_cli_obj->sockbufSize;
    flag = 1;
    err = setsockopt( new_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));
    if (err == -1)
        printf("Couldn't setsockopt(SO_REUSEADDR)\n");

    #if defined(__ECOS)
    err = setsockopt( new_socket, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
    #else
    #if defined (SO_REUSEPORT)
    err = setsockopt( new_socket, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
    #endif
    #endif
    if (err == -1)
        printf("Couldn't setsockopt(SO_REUSEPORT)\n");

    err = setsockopt(new_socket , SOL_SOCKET, SO_SNDBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
    if (err == -1)
        printf("Couldn't setsockopt(SO_SNDBUF)\n");

    err = setsockopt(new_socket , SOL_SOCKET, SO_RCVBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
    if (err == -1)
        printf("Couldn't setsockopt(SO_RCVBUF)\n");
    #endif

    /* Set the socket for non-blocking mode. */
    int yes = 1;
    if (IOCTL(new_socket, FIONBIO, &yes) < 0)
    {
        /* Error */
        perror("errno\r\n");
    }
    USOCKETCLI_DIAG("non-block connect port %d\r\n",portNum);

    err = connect( new_socket, res->ai_addr, res->ai_addrlen);
    USOCKETCLI_DIAG("connect result %d\r\n",err);

    if (err < 0) {
     if (errno == EINPROGRESS) {
        //printf("EINPROGRESS in connect() - selecting\n");
        do {
           struct timeval tv;
           int result=0;
           int valopt;
           int lon =0 ;
           tv.tv_sec = pusocket_cli_obj->timeout;
           tv.tv_usec = 0;

           FD_ZERO(&myset);
           FD_SET(new_socket, &myset);
           result = select(new_socket+1, NULL, &myset, NULL, &tv);

           if (result < 0 && errno != EINTR) {
              perror("select\n");
              break;
           }
           else if (result > 0) {
                USOCKETCLI_DIAG("Error in delayed connection result %d \r\n",result);
                err = 0;
                // Socket selected for write
                lon = sizeof(int);
                if (getsockopt(new_socket, SOL_SOCKET, SO_ERROR, (void*)(&valopt), (socklen_t *)&lon) < 0) {
                    USOCKETCLI_DIAG("Error in getsockopt() %d\n", errno);
                    perror("getsockopt 1 \r\n");
                    err = 1;
                    break;
                }
                // Check the value returned...
                if (valopt) {
                    printf("Error in delayed connection() %d - %s\n", valopt, strerror(valopt));
                    //perror("getsockopt 2\r\n");
                    err = 1;
                    break;
                }
                else
                {
                    USOCKETCLI_DIAG("connect\n");
                    err = 0;
                    break;
                }
           }
           else {
              printf("Timeout! result %d\n",result);
              err = 1;
              break;
           }
        } while (1);

     }
     else {
        perror("errno\r\n");
     }
    }

    if(err==0)
    {
        pusocket_cli_obj->connect_socket = new_socket;
    }
    else
    {
        pusocket_cli_obj->connect_socket =0;
        SOCKET_CLOSE(new_socket);
    }

    freeaddrinfo(res);   // free the linked list
    USOCKETCLI_DIAG("usocket_cli_connect end err %d\r\n",err);
    return err;
}
void usocket_cli_disconnect(usocket_cli_obj *pusocket_cli_obj)
{
    if(pusocket_cli_obj)
    {
        USOCKETCLI_DIAG("usocket_cli_disconnect ,close socket %d\n",pusocket_cli_obj->connect_socket);
        SOCKET_CLOSE(pusocket_cli_obj->connect_socket);
        pusocket_cli_obj->connect_socket = 0 ;
        USOCKETCLI_DIAG("pusocket_cli_obj %x disconn\r\n",pusocket_cli_obj);

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

__externC void usocket_cli_start(void)
{
    int ret =0;
    if (!isStartClient)
    {
        isStopClient = false;
        //cyg_flag_init(&usocket_cli_flag);
        COND_CREATE(usocket_cli_cond);
        MUTEX_CREATE(usocket_cli_mtx,1);
        USOCKETCLI_DIAG("usocket_cli_start create thread\r\n");
        #if defined(__ECOS)
        ret = cyg_thread_create( USOCKETCLI_THREAD_PRIORITY,
                           usocket_cli_proc,
                           0,
                           "usocketCli",
                           &usocket_cli_stacks[0],
                           sizeof(usocket_cli_stacks),
                           &usocket_cli_thread,
                           &usocket_cli_thread_object
            );
        USOCKETCLI_DIAG("usocket_cli_start resume thread\r\n");
        cyg_thread_resume( usocket_cli_thread );
        #else
        ret = pthread_create(&usocket_cli_thread, NULL , usocket_cli_proc , NULL);
        #endif
        if (ret < 0) {
            printf("create feed_bs_thread failed\n");
        } else {
            isStartClient = true;
        }

    }
}

__externC void usocket_cli_stop(void)
{
    if (isStartClient)
    {
        isStopClient = true;
        //cyg_flag_maskbits(&usocket_cli_flag,~CYG_FLG_USOCKETCLI_START);
        FLAG_CLRPTN(usocket_cli_flag,CYG_FLG_USOCKETCLI_START,usocket_cli_mtx);
        USOCKETCLI_DIAG("usocket_cli_stop wait idle begin\r\n");
        //cyg_flag_wait(&usocket_cli_flag,CYG_FLG_USOCKETCLI_IDLE,CYG_FLAG_WAITMODE_AND);
        FLAG_WAITPTN(usocket_cli_cond,usocket_cli_flag,CYG_FLG_USOCKETCLI_IDLE,usocket_cli_mtx,false);
        USOCKETCLI_DIAG("usocket_cli_stop wait idle end\r\n");
        USOCKETCLI_DIAG("usocket_cli_stop ->suspend thread\r\n");
        #if defined(__ECOS)
        cyg_thread_suspend(usocket_cli_thread);
        USOCKETCLI_DIAG("usocket_cli_stop ->delete thread\r\n");
        cyg_thread_delete(usocket_cli_thread);
        #else
        pthread_join(usocket_cli_thread, NULL);
        #endif
        //cyg_flag_destroy(&usocket_cli_flag);
        COND_DESTROY(usocket_cli_cond);
        MUTEX_DESTROY(usocket_cli_mtx);
        isStartClient = false;
        USOCKETCLI_DIAG("usocket_cli_stop ->end\r\n");
    }
}

__externC int usocket_cli_send(usocket_cli_obj* pusocket_cli_obj,char* addr, int* size)
{
    int ret =-1;

    USOCKETCLI_DIAG("usocket_cli_send %d\r\n",pusocket_cli_obj->connect_socket);
    if(pusocket_cli_obj->connect_socket)
    {
        USOCKETCLI_DIAG("send %s %d client_socket %d\n",addr,*size,pusocket_cli_obj->connect_socket);
        if(*size)
            ret = sendall(pusocket_cli_obj->connect_socket,addr, size);
    }
    else
    {
        *size =0;
    }
    return ret;
}

__externC void usocket_cli_install(usocket_cli_obj*  pusocket_cli_obj,usocket_cli_obj*  pObj)
{
    pusocket_cli_obj->connect_socket = 0;
    pusocket_cli_obj->notify = pObj->notify;
    pusocket_cli_obj->recv = pObj->recv;

    strcpy(pusocket_cli_obj->svrIP,pObj->svrIP);

    if(!strlen(pObj->svrIP))
    {
        printf("no svrIP: %s\n", pObj->svrIP);
    }

    // check if port number valid
    if ((!pObj->portNum)||(pObj->portNum > CYGNUM_MAX_PORT_NUM))
    {
        pusocket_cli_obj->portNum = USOCKETCLI_SERVER_PORT;
        printf("error Port, set default port %d\r\n",USOCKETCLI_SERVER_PORT);
    }
    else
    {
        pusocket_cli_obj->portNum = pObj->portNum;
    }

    if (!pObj->sockbufSize)
    {
        pusocket_cli_obj->sockbufSize = USOCKETCLI_SOCK_BUF_SIZE;
        printf("error sockSendbufSize %d, set default %d \r\n",pObj->sockbufSize,USOCKETCLI_SOCK_BUF_SIZE);
    }
    else
    {
        pusocket_cli_obj->sockbufSize = pObj->sockbufSize;
    }

    if (!pObj->timeout)
    {
        pusocket_cli_obj->timeout = USOCKETCLI_TIMEOUT;
        printf("error timeout %d, set default %d \r\n",pObj->timeout,USOCKETCLI_TIMEOUT);
    }
    else
    {
        pusocket_cli_obj->timeout = pObj->timeout;
    }
}

__externC void usocket_cli_uninstall(usocket_cli_obj*  pusocket_cli_obj)
{
    USOCKETCLI_DIAG("pusocket_cli_obj %x uninstall\r\n",pusocket_cli_obj);

    memset(pusocket_cli_obj,0,sizeof(usocket_cli_obj));
}
static int sendall(int s, char* buf, int *len)
{
    int total = 0;
    int bytesleft = *len;
    int n = -1;


    USOCKETCLI_DIAG("sendall %d bytes\n",*len);
    while (total < *len)
    {
        n = send(s, buf+total, bytesleft, 0);
        //USOCKETCLI_DIAG("send %d bytes\n",n);
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

    USOCKETCLI_DIAG("recvall buf = 0x%x, len = %d bytes\n",(int)buf,len);
    while (total < len && (!isStopClient))
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
                while(!isStopClient)
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
                        //USOCKETCLI_DIAG("idletime=%d\r\n",idletime);
                        printf("idletime=%d\r\n",idletime);
                        if (idletime >= USOCKETCLI_IDLE_TIME)
                        {
                            perror("select\r\n");
                            return -1;
                        }
                        #if 0 //[141338]coverity,because USOCKETCLI_IDLE_TIME is 1
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
                        //USOCKETCLI_DIAG("recvall FD_ISSET\r\n");
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
    USOCKETCLI_DIAG("recvall end\n");
    return n<0?-1:total;
}
/* ----------------------------------------------------------------- */
/* end of usocket.c                                                    */
