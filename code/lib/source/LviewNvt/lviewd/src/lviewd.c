/* =================================================================
 *
 *      lviewd.h
 *
 *      A simple live view streaming server
 *
 * =================================================================
 */

#include <sys/types.h>
#include <sys/stat.h>
#if defined(__FREERTOS)
#define LWIP_POSIX_SOCKETS_IO_NAMES 0
#endif
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#if defined(__LINUX_USER__)
#include <pthread.h>
#include <semaphore.h>
#include <sys/ioctl.h>
#elif defined(__FREERTOS)
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#include <kwrap/semaphore.h>
#include <kwrap/task.h>
#endif
#include <LviewNvt/lviewd.h>
#include <pkgconf/lviewd.h>
#if defined(__ECOS)
#ifdef __USE_IPC
#include <cyg/nvtipc/NvtIpcAPI.h>
#endif
#include <cyg/hal/hal_cache.h>
#ifdef CONFIG_SUPPORT_SSL
#include <cyg/nvtssl/nvtssl.h>
#endif
#else
#ifdef __USE_IPC
#include <nvtipc.h>
#endif
//#include <sys/mman.h>
#if defined(__LINUX_660)
#include <sys/cachectl.h>
#include <nvtmem.h>
#include <protected/nvtmem_protected.h>
#endif
//#include <sys/ipc.h>
//#include <sys/msg.h>
//#include <semaphore.h>
#ifdef CONFIG_SUPPORT_SSL
#include <nvtssl.h>
#endif
#endif
#if defined(__FREERTOS)
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#define SOMAXCONN               128
#endif
#if 0
#if 0
#define LVIEW_DIAG printf
#define LVIEW_TIME_DIAG printf
#define LVIEW_SEND_DIAG printf
#define LVIEW_FLAG_DIAG printf
#define LVIEW_CONN_DIAG printf
#else
#define LVIEW_DIAG printf
#define LVIEW_TIME_DIAG(...)
#define LVIEW_SEND_DIAG(...)
#define LVIEW_FLAG_DIAG(...)
#define LVIEW_CONN_DIAG printf
#endif
#else
#define LVIEW_DIAG(...)
#define LVIEW_TIME_DIAG(...)
#define LVIEW_SEND_DIAG(...)
#define LVIEW_FLAG_DIAG(...)
#define LVIEW_CONN_DIAG(...)
#endif

#define LVIEW_DEBUG_LATENCY           0

#define CHKPNT    printf("\033[37mCHK: %d, %s\033[0m\r\n",__LINE__,__func__) ///< Show a color sting of line count and function name in your insert codes
#define DBG_WRN(fmtstr, args...) printf("\033[33mlviewd %s(): \033[0m" fmtstr,__func__, ##args)
#define DBG_ERR(fmtstr, args...) printf("\033[31mlviewd %s(): \033[0m" fmtstr,__func__, ##args)

#if 1
#define DBG_IND(fmtstr, args...)
#else
#define DBG_IND(fmtstr, args...) printf("lviewd %s(): " fmtstr, \
    __func__, ##args)
#endif

#define DBG_DUMP(fmtstr, args...) printf("lviewd: " fmtstr, ##args)

#if defined(__ECOS)
#include <cyg/kernel/kapi.h>
#define THREAD_DECLARE(name,arglist) static void  name (cyg_addrword_t arglist)
#define THREAD_HANDLE cyg_handle_t
#define THREAD_OBJ cyg_thread
#define THREAD_CREATE(pri, name,thread_handle,fp,p_data,p_stack,size_stack,p_thread_obj)  cyg_thread_create(pri ,fp,(cyg_addrword_t)p_data,name,(void*)p_stack,size_stack,&thread_handle,p_thread_obj)
#define THREAD_RESUME(thread_handle) cyg_thread_resume(thread_handle)
#define THREAD_DESTROY(thread_handle) if (thread_handle){cyg_thread_suspend(thread_handle);while(!cyg_thread_delete(thread_handle)){cyg_thread_delay(1);}}
#define THREAD_KILL(thread_handle) if (thread_handle){cyg_thread_suspend(thread_handle);while(!cyg_thread_delete(thread_handle)){cyg_thread_delay(1);}}
#define THREAD_RETURN(thread_handle) cyg_thread_exit()

#elif defined(__FREERTOS)
#undef THREAD_RETTYPE
#undef THREAD_DECLARE
#undef THREAD_HANDLE
#undef THREAD_CREATE
#undef THREAD_RESUME
#undef THREAD_DESTROY
#undef THREAD_RETURN

#define THREAD_RETTYPE                          void
#define THREAD_DECLARE(name, arglist)           static void* name (void* arglist)
#define THREAD_HANDLE                           VK_TASK_HANDLE
#define THREAD_OBJ                              int
#define THREAD_CREATE(pri, name,thread_handle,fp,p_data,p_stack,size_stack,p_thread_obj)   thread_handle = vos_task_create(fp, (void *)p_data, name, pri, size_stack)
#define THREAD_RESUME(handle)                   vos_task_resume(handle)
#define THREAD_DESTROY(handle)                  //vos_task_destroy(handle)
#define THREAD_RETURN(value)                    {vos_task_return((int)value); vTaskDelete(NULL); return 0;}
#define THREAD_SET_PRIORITY(handle, pri)        vos_task_set_priority(handle, pri)
#else
#define THREAD_DECLARE(name,arglist) static void* name (void* arglist)
#define THREAD_HANDLE pthread_t
#define THREAD_OBJ int
#define THREAD_CREATE(pri,name,thread_handle,fp,p_data,p_stack,size_stack,p_thread_obj)  {pthread_attr_t attr;int ret ; pthread_attr_init(&attr);pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);ret = pthread_create(&thread_handle,&attr,fp,p_data); pthread_attr_destroy(&attr);if (ret !=0) printf("Create pthread error! %s\n",strerror(ret));}
#define THREAD_RESUME(thread_handle)
#define THREAD_DESTROY(thread_handle)
#define THREAD_KILL(thread_handle) {int timeoutCnt = 10,ret=0;if (thread_handle){ret=pthread_kill(thread_handle,SIGKILL); while(timeoutCnt--) {sleep(1);DBG_DUMP("timeoutCnt=%d , ret=%d,thread_handle=0x%x\r\n",timeoutCnt,ret,thread_handle);if ((ret=pthread_kill(thread_handle,0))<0) break; }}}
#define THREAD_RETURN(thread_handle) {thread_handle = 0; /*pthread_detach(pthread_self());*/pthread_exit(NULL); return 0;}
#endif

#define MUTEX_INIT_STATIC(handle) pthread_mutex_t handle = PTHREAD_MUTEX_INITIALIZER;
#define MUTEX_HANDLE pthread_mutex_t
#define MUTEX_CREATE(handle,init_cnt) pthread_mutex_init(&handle,NULL);if(init_cnt==0)pthread_mutex_lock(&handle)
#define MUTEX_SIGNAL(handle) pthread_mutex_unlock(&handle)
#define MUTEX_WAIT(handle) pthread_mutex_lock(&handle);
#define MUTEX_DESTROY(handle) pthread_mutex_destroy(&handle)
#define COND_HANDLE pthread_cond_t
#define COND_CREATE(handle) pthread_cond_init(&handle,NULL)
#define COND_SIGNAL(handle) pthread_cond_broadcast(&handle)
#define COND_WAIT(handle,mtx) pthread_cond_wait(&handle, &mtx)
#define COND_DESTROY(handle) pthread_cond_destroy(&handle)
#define FLAG_SETPTN(cond,flag,ptn,mtx) {pthread_mutex_lock(&mtx);flag|=ptn;LVIEW_FLAG_DIAG("setflag 0x%x, ptn 0x%x\r\n",flag,ptn);pthread_cond_broadcast(&cond);pthread_mutex_unlock(&mtx);}
#define FLAG_CLRPTN(flag,ptn,mtx) {pthread_mutex_lock(&mtx);flag &= ~(ptn);LVIEW_FLAG_DIAG("clrflag 0x%x, ptn 0x%x\r\n",flag,ptn);pthread_mutex_unlock(&mtx);}
#define FLAG_WAITPTN(cond,flag,ptn,mtx,clr) {pthread_mutex_lock(&mtx);while (1) { LVIEW_FLAG_DIAG("waitflag b 0x%x, ptn 0x%x\r\n",flag,ptn);\
                                                                    if (flag & ptn){if (clr) {flag &= ~(ptn);}pthread_mutex_unlock(&mtx);LVIEW_FLAG_DIAG("waitflag e 0x%x, ptn 0x%x\r\n",flag,ptn);break;}\
                                                                  pthread_cond_wait(&cond, &mtx);\
                                                                  if (flag & ptn) {\
                                                                  if (clr) {flag &= ~(ptn);}\
																  pthread_mutex_unlock(&mtx);\
																  LVIEW_FLAG_DIAG("waitflag e 0x%x, ptn 0x%x\r\n",flag,ptn);\
                                                                  break;}\
                                                                  }}

#define FLAG_WAITPTN_TIMEOUT(cond,flag,ptn,mtx,clr,timeout,rtn){struct timespec to;int err;rtn=0;\
                                                         pthread_mutex_lock(&mtx);\
                                                         while (1) { LVIEW_FLAG_DIAG("waitflag 0x%x\r\n",flag);\
	                                                               if (flag & ptn){if (clr) {flag &= ~(ptn);}pthread_mutex_unlock(&mtx);break;}\
                                                                   LVIEW_FLAG_DIAG("cond_wait 0x%x\r\n",flag);\
                                                                   to.tv_sec = time(NULL) + timeout;\
                                                                   to.tv_nsec = 0;\
                                                                   err = pthread_cond_timedwait(&cond, &mtx, &to);\
                                                                   if (err == ETIMEDOUT) {\
                                                                       printf("timeout\r\n");\
                                                                       rtn = -1;\
                                                                       pthread_mutex_unlock(&mtx);\
                                                                       break;\
                                                                   }\
                                                                   if (flag & ptn) {\
                                                                     if (clr) {flag &= ~(ptn);}\
                                                                     pthread_mutex_unlock(&mtx);\
                                                                     break;}\
                                                                  }\
                                                         }\

#if defined(__LINUX_650)
#define __COREIPC__   0
#else
#define __COREIPC__   1
#endif

#if (__COREIPC__)
#define IPC_MSGGET() NvtIPC_MsgGet(NvtIPC_Ftok(LVIEWD_IPC_TOKEN_PATH))
#define IPC_MSGREL(msqid) NvtIPC_MsgRel(msqid)
#define IPC_MSGSND(msqid,pMsg,msgsz) NvtIPC_MsgSnd(msqid,NVTIPC_SENDTO_CORE1,pMsg,msgsz)
#define IPC_MSGRCV(msqid,pMsg,msgsz) NvtIPC_MsgRcv(msqid,pMsg,msgsz)
#else
#define IPC_MSGGET() msgget(LVIEWD_IPCKEYID, IPC_CREAT | 0666 )
#define IPC_MSGREL(msqid) msgctl(msqid, IPC_RMID , NULL)
#define IPC_MSGSND(msqid,pMsg,msgsz) msgsnd(msqid,pMsg,msgsz-4, IPC_NOWAIT)
#define IPC_MSGRCV(msqid,pMsg,msgsz) msgrcv(msqid,pMsg,msgsz-4,LVIEWD_IPC_MSG_TYPE_C2S, 0)
#endif


#if defined(__LINUX_660)
#define ipc_getPhyAddr(addr)        NvtMem_GetPhyAddr(addr)
#define ipc_getNonCacheAddr(addr)   NvtMem_GetNonCacheAddr(addr)
#define ipc_getCacheAddr(addr)      dma_getCacheAddr(addr)
#define ipc_getMmioAddr(addr)       dma_getMmioAddr(addr)
#define ipc_flushCache(addr,size)   cacheflush((char*)addr, size, DCACHE)
#define TIMER0_COUNTER_REG          0xC0040108
#define NVTIPC_MEM_INVALID_ADDR     0xFFFFFFFF
#elif defined(__LINUX_680)
#define ipc_getPhyAddr(addr)        NvtIPC_GetPhyAddr(addr)
#define ipc_getCacheAddr(addr)      NvtIPC_GetCacheAddr(addr)
#define ipc_getNonCacheAddr(addr)   NvtIPC_GetNonCacheAddr(addr)
#define ipc_getMmioAddr(addr)       NvtIPC_GetMmioAddr(addr)
#define ipc_flushCache(addr,size)   NvtIPC_FlushCache(addr, size)
#define TIMER0_COUNTER_REG          0xF0040108
#else
#define ipc_getPhyAddr(addr)        (addr-0xA0000000)
#define ipc_getNonCacheAddr(addr)   (addr+0xA0000000)
#define ipc_getCacheAddr(addr)      (addr+0x80000000)
#define ipc_getMmioAddr(addr)       (addr)
#define ipc_flushCache(addr,size)   HAL_DCACHE_FLUSH(addr,size)
#define TIMER0_COUNTER_REG          0xC0040108
#define NVTIPC_MEM_INVALID_ADDR     0xFFFFFFFF
#endif

#ifndef true
#define true  1
#define false 0
#endif

#ifndef __USE_IPC
#define __USE_IPC              0
#endif
#ifndef __BUILD_PROCESS
#define __BUILD_PROCESS        0
#endif


#if defined(__FREERTOS)
#define IOCTL                  lwip_ioctl
#define SOCKET_CLOSE           lwip_close
#else
#define IOCTL                  ioctl
#define SOCKET_CLOSE           close
#endif
/* ================================================================= */




/* ================================================================= */
/* Server socket address and file descriptor.
 */

#define CYGNUM_MAX_PORT_NUM        65535

#define CYGNUM_MAX_JPG_SIZE        200*1024

#define CYGNUM_HTTP_BUFF_SIZE      2048

#define CYGNUM_LVIEW_SESSION_THREAD_STACK_SIZE  24*1024

#define CYGNUM_LVIEW_COMBINE_SEND  0

#define CYGNUM_LVIEW_USE_HWCOPY    1


#define CYGNUM_LVIEW_SEND_REFRESH_HTML     0

#define CYGNUM_LVIEW_FRAME_RATE            30

#define CYGNUM_LVIEW_SOCK_BUF_SIZE         50*1024

#define CYGNUM_SELECT_TIMEOUT_USEC         500000

#define CYGNUM_LVIEW_SERVER_IDLE_TIME      10  // retry 20

// performance

#define CYGNUM_LVIEW_PERF_SEND     0

#define CYGNUM_LVIEW_PERF_SELECT   0

// flag define

#define CYG_FLG_LVIEWD_START               0x01

#define CYG_FLG_LVIEWD_NEW_FRAME           0x02

#define CYG_FLG_LVIEWD_NOTIFY_CLIENT_ACK   0x04

#define CYG_FLG_LVIEWD_GET_JPG_ACK         0x08

#define CYG_FLG_LVIEWD_JPG_TSK_IDLE        0x20
#define CYG_FLG_LVIEWD_IDLE                0x40
#define CYG_FLG_LVIEWD_CLOSED              0x80


#define CYG_FLG_LVIEWD_SESSION_IDLE        0x100


// HTTP header define value

#define HTTP_REQUEST_METHOD_GET     1
#define HTTP_REQUEST_METHOD_HEAD    2
#define HTTP_REQUEST_METHOD_POST    3
#define HTTP_REQUEST_METHOD_PUT     4
#define HTTP_REQUEST_METHOD_TRACE   5
#define HTTP_REQUEST_METHOD_OPTIONS 6
#define HTTP_REQUEST_METHOD_DELETE  7

#define HTTP_VERSION_INVALID        -1
#define HTTP_VERSION_1_0            0
#define HTTP_VERSION_1_1            1


#define USER_AGENT_NONE             0
#define USER_AGENT_OTHERS           1
#define USER_AGENT_IE               2
#define USER_AGENT_CHROME           3
#define USER_AGENT_VLC              4
#define USER_AGENT_FIREFOX          5
#define USER_AGENT_AOF              6

#define HTTP_HEADER_END_LEN         4  // \r\n\r\n





// HTTP animation mode
#define ANIMATION_MODE_SERVER_PUSH                      0
#define ANIMATION_MODE_CLIENT_PULL_AUTO_REFRESH         1
#define ANIMATION_MODE_CLIENT_PULL_MANUAL_REFRESH       2


#define JPG_QUEUE_ELEMENT_NUM    3

typedef struct _MEM_RANGE
{
    int Addr;    ///< Memory buffer starting address
    int Size;    ///< Memory buffer size
} MEM_RANGE, *PMEM_RANGE;


typedef struct _JPG_QUEUE
{
    u_int32_t   inIdx;
    u_int32_t   outIdx;
    MEM_RANGE   frame[JPG_QUEUE_ELEMENT_NUM];
} JPG_QUEUE, *PJPG_QUEUE;

typedef struct lviewd_session
{
	int            enable;
    int            can_delete;
	struct lviewd_session *next;
    struct lviewd_server  *pServer;
	int             client_socket;
    struct sockaddr client_address;
	THREAD_OBJ     thread_obj;
    THREAD_HANDLE  thread_hdl;
    char           stack[CYGNUM_LVIEW_SESSION_THREAD_STACK_SIZE];
    COND_HANDLE    condData;
    MUTEX_HANDLE     condMutex;
    u_int32_t      eventFlag;
    #ifdef CONFIG_SUPPORT_SSL
	NVTSSL            *ssl;
    u_int32_t      is_ssl;
	int            sslReadSize;
    u_int32_t      sslBufAddr;
    #endif
	char           sendbuff[CYGNUM_HTTP_BUFF_SIZE];
	uint64_t       IdleTime;
	int            pid;
    int            childpid;
	int            exit;
} lviewd_session_t;

struct serverstruct
{
    struct   serverstruct *next;
    int      sd;
    int      is_ssl;
    #ifdef CONFIG_SUPPORT_SSL
    NVTSSL_CTX *ssl_ctx;
    #endif
};

typedef struct lviewd_server
{
	int                         isStart;
	int                         isStop;
	int                         timeoutCnt;       // timeout counter for send & receive , time base is 0.5sec
    struct lviewd_session       *active_sessions;
    struct serverstruct         *servers;
	#if 0
    int                         is_ssl;
    #ifdef CONFIG_SUPPORT_SSL
    SSL_CTX                     *ssl_ctx;
    #endif
	#endif
	MUTEX_HANDLE                  s_mutex;
	COND_HANDLE 				cond;
    MUTEX_HANDLE  				mtx;
    u_int32_t   				eventFlag;
	int                         nr_of_clients;
	int                         max_nr_of_clients;
	u_int64_t                   time_offset;
} lviewd_server_t;




static lviewd_server_t lviewd_server = {0} ;

static JPG_QUEUE   jpgQueue;

static cyg_lviewd_install_obj liveview_obj={0};

static MUTEX_INIT_STATIC(openclose_mtx);

#if __BUILD_PROCESS
static sem_t main_sem;
#endif


#if __USE_IPC
static MEM_RANGE   jpgData={0};
#if defined(__LINUX_680)
static void       *jpg_map_addr = NULL;
u_int32_t          jpg_map_size = NULL;
#endif
#endif

#if 0
static char cyg_lviewd_not_found[] =
"<html>\n<head><title>Page Not found</title></head>\n"
"<body><h2>The requested URL was not found on this server.</h2></body>\n</html>\n";
#endif


/* ================================================================= */
/* Thread stacks, etc.
 */
#if defined(__ECOS)
static u_int8_t lview_stacks[ CYGNUM_LVIEWD_SERVER_BUFFER_SIZE+
                              CYGNUM_LVIEWD_THREAD_STACK_SIZE
                              #ifdef CONFIG_SUPPORT_SSL
                              +4*1024
                              #endif
                              ];
static u_int8_t lview_getjpg_stacks[4096];


static THREAD_OBJ lview_thread_object;
static THREAD_OBJ lview_getjpg_thread_object;

#if __USE_IPC
static u_int8_t lview_rcv_thread_stacks[4096];
static THREAD_OBJ lview_rcv_thread_object;
static u_int8_t lview_close_thread_stacks[4096];
static THREAD_OBJ lview_close_thread_object;
#endif

#elif defined(__FREERTOS)
static u_int8_t lview_stacks[ CYGNUM_LVIEWD_SERVER_BUFFER_SIZE+
                              CYGNUM_LVIEWD_THREAD_STACK_SIZE
                              #ifdef CONFIG_SUPPORT_SSL
                              +4*1024
                              #endif
                              ];
static u_int8_t lview_getjpg_stacks[4096];
#endif



static THREAD_HANDLE lview_thread;
static THREAD_HANDLE lview_getjpg_thread;
#if __USE_IPC
static THREAD_HANDLE lview_rcv_thread;
static THREAD_HANDLE lview_close_thread;
#endif

#if CYGNUM_LVIEW_SEND_REFRESH_HTML
static void cyg_lviewd_send_html( int client);
#endif
#if __USE_IPC
static void lviewd_ipc_create(void);
static void lviewd_ipc_release(void);
static void lviewd_ipc_sendmsg(int cmd,int status);
static void lviewd_ipc_notify(int status);
static int  lviewd_ipc_getjpg(int* jpgAddr, int* jpgSize);
#endif

static int cyg_lviewd_send_data(lviewd_session_t* session,int close_conn, int serverPush, int isFirstFrame);
static int recvall(lviewd_session_t* session, char* buf, int len, int isJustOne);
static void lviewd_process(lviewd_session_t  *session, struct sockaddr *client_address, int is_push_mode);
static void lviewd_setsockopt_common(int client_socket,int sock_buf_size, int tos);
static void cyg_lviewd_release_resource(lviewd_server_t *pServer);

THREAD_DECLARE(lviewd_session_thrfunc,data);
THREAD_DECLARE(lview_getjpg_thread_main,arg);

typedef struct
{
    int                 method;
    char                *path;
    char                *formdata;
    int                 httpVer;
    int                 closeConnect;
    int                 userAgent;
    int                 HeaderSeq;
} cyg_lviewd_request_header;


static void delay_ms( int64_t ms )
{
	#if defined(__FREERTOS)
	msleep(ms);
	#else
	struct timespec timeout;

	timeout.tv_sec = ms/1000;
	timeout.tv_nsec = (ms%1000)*1000000;
	nanosleep( &timeout, NULL );
	#endif
}

#if CYGNUM_LVIEW_PERF_SELECT
static u_int32_t getCurrTime(void)
{
    struct timeval tv1;
    struct timezone tz;
    u_int64_t t1;
    gettimeofday(&tv1,&tz);
    t1 = (tv1.tv_sec*1000000+tv1.tv_usec);
    return (u_int32_t)t1;
}
#endif

#if LVIEW_DEBUG_LATENCY
static unsigned int drv_getSysTick(void)
{
    volatile unsigned int* p1 = (volatile unsigned int*)ipc_getMmioAddr(TIMER0_COUNTER_REG);
    return *p1;

}
#endif



static int caseless_compare(char* name1, char* name2)
{
    int    i=0;
    char   c1,c2;

    while(1)
    {
        c1 = name1[i];
        c2 = name2[i];
        if (c1 >= 'a' && c1<='z')
            c1-=0x20;
        if (c2 >= 'a' && c2<='z')
            c2-=0x20;
        if (c1!=c2)
            return false;
        if (c1==0 && c2==0)
            return true;
        i++;
    }
    return true;
}

static int caseless_compare_withlen(char* name1, char* name2, int len)
{
    int    i=0;
    char   c1,c2;

    while(i<len)
    {
        c1 = name1[i];
        c2 = name2[i];
        if (c1 >= 'a' && c1<='z')
            c1-=0x20;
        if (c2 >= 'a' && c2<='z')
            c2-=0x20;
        if (c1!=c2)
            return false;
        i++;
    }
    return true;
}



static uint64_t lviewd_get_long_counter(void)
{
	#if __USE_IPC
	NVTIPC_SYS_MSG              ipc_sys_msg;
	NVTIPC_SYS_LONG_COUNTER_MSG long_counter_msg;

	ipc_sys_msg.sysCmdID = NVTIPC_SYSCMD_GET_LONG_COUNTER_REQ;
	ipc_sys_msg.SenderCoreID = NVTIPC_SENDER_CORE2;
	ipc_sys_msg.DataAddr = (uint32_t)&long_counter_msg;
	ipc_sys_msg.DataSize = sizeof(long_counter_msg);
	/*
	 * Send a message.
	 */
	if (IPC_MSGSND(NVTIPC_SYS_QUEUE_ID, &ipc_sys_msg, sizeof(ipc_sys_msg)) < 0) {
		DBG_ERR("msgsnd");
		return 0;
	}
	DBG_IND("long counter = %d.%d, \r\n", long_counter_msg.sec, long_counter_msg.usec);
	return ((((uint64_t)long_counter_msg.sec) << 32) | long_counter_msg.usec);
	#else
	return 0;
	#endif
}

uint64_t lviewd_diff_long_counter(uint64_t time_start, uint64_t time_end)
{
	uint32_t time_start_sec = 0;
	uint32_t time_start_usec = 0;
	uint32_t time_end_sec = 0;
	uint32_t time_end_usec = 0;
	int32_t  diff_time_sec = 0 ;
	int32_t  diff_time_usec = 0;
	uint64_t diff_time;

	time_start_sec = (time_start >> 32) & 0xFFFFFFFF;
	time_start_usec = time_start & 0xFFFFFFFF;
	time_end_sec = (time_end >> 32) & 0xFFFFFFFF;
	time_end_usec = time_end & 0xFFFFFFFF;

	diff_time_sec = (int32_t)time_end_sec - (int32_t)time_start_sec;
	diff_time_usec = (int32_t)time_end_usec - (int32_t)time_start_usec;
	diff_time = ((int64_t)diff_time_sec * 1000000) + diff_time_usec;

	DBG_IND("%d %d %d %d\r\n", time_start_sec, time_start_usec, time_end_sec, time_end_usec);
	return diff_time;
}

static uint64_t lviewd_get_time_offset(void)
{
	struct     timeval tv;
	uint64_t   long_counter, timeofday;


	long_counter = lviewd_get_long_counter();
	gettimeofday(&tv, NULL);
	timeofday    = ((((uint64_t)tv.tv_sec) << 32) | tv.tv_usec);

	return lviewd_diff_long_counter(long_counter, timeofday);
}

static uint64_t lviewd_get_time(void)
{
	struct timeval tmptv;
	uint64_t   ret;

	gettimeofday(&tmptv, NULL);
	ret = ((uint64_t)tmptv.tv_sec * 1000000 + tmptv.tv_usec);
	DBG_IND("ret time=%d sec, %d usec\r\n", tmptv.tv_sec, tmptv.tv_usec);

	return ret;
}

static void lviewd_notify(cyg_lviewd_install_obj *pLiveview_obj, int notify_status)
{
    if (pLiveview_obj->notify)
        pLiveview_obj->notify(notify_status);
}



/*
GET /tools.html HTTP/1.1
Host: www.joes-hardware.com
Accept: text/html, image/gif, image/jpeg
Connection: close

*/
/* return true means the header has end */
static int lviewd_parse_request( char* buff, int bufflen, cyg_lviewd_request_header* req_hdr)
{
    char *p,*str, *value;
    int  len, ret = false;
    int  haskey;
    int  breakCnt;
    char keyTempstr[30];


    LVIEW_DIAG("buffAddr= 0x%x, bufflen= %d, last 5 bytes=%d,%d,%d,%d,%d\r\n",(int)buff,bufflen,buff[bufflen-5],buff[bufflen-4],buff[bufflen-3],buff[bufflen-2],buff[bufflen-1]);
    str = p = buff;

    #if 1
    if (req_hdr->HeaderSeq == 0)
    {
        // parsing method
        while( *p != ' ' && bufflen)
        {
            p++;
            bufflen--;
        }
        *p = 0;
        if( caseless_compare( str, "GET" ) )
        {
            req_hdr->method = HTTP_REQUEST_METHOD_GET;
        }
        else
        {
            LVIEW_DIAG("Connection method %s not support\n",str);
            //return false;
        }
        // parsing path
        req_hdr->path = p+1;
        while( (*p != ' '&& *p != '?') && bufflen)
        {
            p++;
            bufflen--;
        }
        // set formdata
        if( *p == '?' )
            req_hdr->formdata = p+1;
        else
            req_hdr->formdata = NULL;
        *p = 0;
        if( req_hdr->formdata != NULL )
        {
            while( *p != ' ' && bufflen)
            {
                p++;
                bufflen--;
            }
            *p = 0;
        }
        // parse http version
        str = p+1;
        while( *p != '\r' && *p != '\n' && bufflen)
        {
            p++;
            bufflen--;
        }
        *p = 0;
        if (caseless_compare ("HTTP/1.1",str))
        {
            req_hdr->httpVer = HTTP_VERSION_1_1;
            // connection default set to not close
            req_hdr->closeConnect = false;
        }
        else if (caseless_compare ("HTTP/1.0",str))
        {
            req_hdr->httpVer = HTTP_VERSION_1_0;
            // connection default set to close
            req_hdr->closeConnect = true;
        }
        else
        {
            req_hdr->httpVer = HTTP_VERSION_INVALID;
            LVIEW_DIAG("HTTP version invalid\n");
            //return false;
        }
    }
    #endif
    while (bufflen)
    {
        str = p;
        breakCnt = 0;
        while( (*p == '\r' || *p == '\n') && bufflen && breakCnt<4)
        {
            p++;
            bufflen--;
            breakCnt++;
        }
        if (breakCnt && ((strncmp("\r\n\r\n",str,4) ==0) || (strncmp("\n\n",str,2) ==0) || (strncmp("\n\r\n",str,3) ==0)))
        {
            req_hdr->HeaderSeq = 0;
            ret = true;
            LVIEW_DIAG("find request header end, remain bufflen=%d\r\n",bufflen);
            goto parse_header_end;
        }
        str = p;
        haskey = false;
        while( *p != '\r' && *p != '\n' && bufflen)
        {
            if (*p == ':')
                haskey = true;
            p++;
            bufflen--;
        }
        if (haskey)
        {
            char c;

            c = *p;
            *p = 0;
            LVIEW_DIAG("str=%s\r\n",str);

            len = sprintf(keyTempstr,"User-Agent:");
            if (caseless_compare_withlen(keyTempstr,str,len))
            {
                value = str+len;
                if (strstr (value, "MSIE")!=NULL)
                    req_hdr->userAgent = USER_AGENT_IE;
                else if (strstr (value, "Chrome/")!=NULL)
                    req_hdr->userAgent = USER_AGENT_CHROME;
                else if (strstr (value, "VLC/")!=NULL)
                    req_hdr->userAgent = USER_AGENT_VLC;
                else if (strstr (value, "Firefox/")!=NULL)
                    req_hdr->userAgent = USER_AGENT_FIREFOX;
                else if (strstr (value, "AOF LiveView")!=NULL)
                    req_hdr->userAgent = USER_AGENT_AOF;
                else
                    req_hdr->userAgent = USER_AGENT_OTHERS;
                *p = c;
                continue;
            }
        }
    }
parse_header_end:
    if (ret == true)
        req_hdr->HeaderSeq = 0;
    else
        req_hdr->HeaderSeq++;
    LVIEW_DIAG("method=%d, path=%s, httpVer = %d, CloseConn=%d, userAgent=%d,HeaderSeq =%d \r\n",req_hdr->method,req_hdr->path,req_hdr->httpVer,req_hdr->closeConnect,req_hdr->userAgent,req_hdr->HeaderSeq);
    if (req_hdr->formdata)
        LVIEW_DIAG("formdata->%s\r\n",req_hdr->formdata);
    return ret;
}

static void lviewd_JpgQueueInit(void)
{
    PJPG_QUEUE   pJpgQ = &jpgQueue;
    pJpgQ->inIdx = 0;
    pJpgQ->outIdx = 0;
}

static int lviewd_JpgQueueIsEmpty(void)
{
    PJPG_QUEUE   pJpgQ = &jpgQueue;

    if (pJpgQ->inIdx == pJpgQ->outIdx)
        return true;
    return false;
}

static int lviewd_JpgQueueIsfull(void)
{
    PJPG_QUEUE   pJpgQ = &jpgQueue;

    if (pJpgQ->outIdx+JPG_QUEUE_ELEMENT_NUM-1 < pJpgQ->inIdx)
        return true;
    return false;
}

// if find http header end will return the header end address else will return NULL
static char* lviewd_chk_request_end(char* buff, int bufflen)
{
    char *p = buff;
	//LVIEW_DIAG("bufflen=%d\r\n",bufflen);
    while (bufflen)
    {
        if ( *p == '\r' && (*(p+1)) == '\n' && (*(p+2)) == '\r' && (*(p+3)) == '\n' )
        {
            LVIEW_DIAG("lviewd_chk_request_end -> true \r\n");
            return (p+4);
        }
        p++;
        bufflen--;
    }
    //LVIEW_DIAG("lviewd_chk_request_end -> false\n");
    return NULL;
}



static int session_init(int client_socket, struct sockaddr* pClient_address, int is_ssl)
{
	lviewd_session_t *session;
    lviewd_server_t *pServer = &lviewd_server;
	#ifdef CONFIG_SUPPORT_SSL
    NVTSSL          *ssl;
	#endif

    session = malloc(sizeof(lviewd_session_t));
    LVIEW_DIAG("session =0x%x , size = 0x%x\r\n",(int)session, (int)sizeof(lviewd_session_t));
    if (!session)
    {
        return -1;
    }
    #ifdef CONFIG_SUPPORT_SSL
    if (is_ssl)
    {
        ssl = nvtssl_server_new(pServer->servers->ssl_ctx, client_socket);
        if (!ssl)
        {
            free(session);
            DBG_ERR("ssl_server_new fail\r\n");
            return -1;
        }
    }
    #endif
	MUTEX_WAIT(pServer->s_mutex);
    memset(session, 0, sizeof(lviewd_session_t));
    session->enable = true;
    COND_CREATE((session->condData));
    MUTEX_CREATE((session->condMutex),1);
	FLAG_CLRPTN(pServer->eventFlag,CYG_FLG_LVIEWD_SESSION_IDLE,pServer->mtx);
    #ifdef CONFIG_SUPPORT_SSL
    if (is_ssl)
    {
        session->ssl = ssl;
        session->is_ssl = is_ssl;
    }
    #endif
    session->pServer = pServer;
    session->client_socket = client_socket;
    memcpy(&session->client_address,pClient_address,sizeof(struct sockaddr));
    session->next = pServer->active_sessions;
    pServer->active_sessions = session;
	lviewd_setsockopt_common(client_socket,liveview_obj.sockbufSize,liveview_obj.tos);
    THREAD_CREATE( liveview_obj.threadPriority-1,
               "lview Session",
               session->thread_hdl,
               lviewd_session_thrfunc,
               session,
               &session->stack[0],
               CYGNUM_LVIEW_SESSION_THREAD_STACK_SIZE,
               &session->thread_obj
        );
    THREAD_RESUME(session->thread_hdl );
    LVIEW_DIAG("Resume thread = 0x%x\r\n",(unsigned int)session->thread_hdl);
    pServer->nr_of_clients++;
	if (pServer->nr_of_clients > pServer->max_nr_of_clients) {
		DBG_ERR("nr_of_clients = %d\r\n",pServer->nr_of_clients);
	}
    session->IdleTime = lviewd_get_time();
    LVIEW_CONN_DIAG("nr_of_clients++ = %d\r\n",pServer->nr_of_clients);
    MUTEX_SIGNAL(pServer->s_mutex);
    return 0;
}


static void session_exit(lviewd_session_t *session)
{
	COND_DESTROY((session->condData));
    MUTEX_DESTROY((session->condMutex));
	LVIEW_DIAG("free session 0x%x\r\n",(int)session);
	free(session);
}

static void lviewd_set_sock_linger(int client_socket, int val)
{
	#if _TODO_
	// to close tcp socket quickly
    struct linger so_linger = { 0 };
    so_linger.l_onoff = 1;
    so_linger.l_linger = val;
    if (setsockopt(client_socket, SOL_SOCKET, SO_LINGER,(const char*)&so_linger, sizeof so_linger) < 0) {
      printf("Couldn't setsockopt(SO_LINGER)\n");
    }
	#endif
}

static void lviewd_closesocket(lviewd_session_t *session)
{
	#ifdef CONFIG_SUPPORT_SSL
    if (session->is_ssl)
    {
        nvtssl_free(session->ssl);
		session->ssl = NULL;
    }
    #endif
    // the sequence is ssl_free -> shutdown socket -> close socket
    if (session->client_socket)
    {
        shutdown(session->client_socket, SHUT_RDWR);
        SOCKET_CLOSE(session->client_socket);
        session->client_socket = 0;
    }
}

static void lviewd_closesocket_for_timeout(lviewd_session_t *session)
{
    // the sequence is shutdown socket -> close socket -> ssl_free
    // because when the timeout case, the socket may be blocked, if we call ssl_free firstly,
    // it will block on this API, because of the line send_alert(ssl, SSL_ALERT_CLOSE_NOTIFY); in this API

    if (session->client_socket)
    {
        shutdown(session->client_socket, SHUT_RDWR);
        SOCKET_CLOSE(session->client_socket);
        session->client_socket = 0;
    }
    #ifdef CONFIG_SUPPORT_SSL
    if (session->is_ssl)
    {
        nvtssl_free(session->ssl);
		session->ssl = NULL;
    }
    #endif

}


static int lviewd_chk_timeout(lviewd_session_t *session)
{
    #if defined(__ECOS)
    int64_t currentT = lviewd_get_time();

    if (currentT > (session->IdleTime + (session->pServer->timeoutCnt+10) * CYGNUM_SELECT_TIMEOUT_USEC )) // idle timeout 30sec
    {
        DBG_WRN("timeout %lld sec, id=%d  childpid=%d %lld %lld \r\n",(int64_t)session->pServer->timeoutCnt * CYGNUM_SELECT_TIMEOUT_USEC/1000000,session->pid,session->childpid,session->IdleTime,currentT);
        return 0;
    }
    #endif
    return 0;
}


static void lviewd_cleanup_sessions(lviewd_server_t *pServer)
{
    lviewd_session_t *session = NULL;
    lviewd_session_t *search, *last = 0;
    int           isTimeout=0;

    search = pServer->active_sessions;
    while (search)
    {
        if (search->can_delete || (isTimeout = lviewd_chk_timeout(search)))
        {
            session = search;
            search = search->next;
            if (last == 0)
                pServer->active_sessions = session->next;
            else
            {
                last->next = session->next;
            }
            pServer->nr_of_clients--;
            LVIEW_CONN_DIAG("nr_of_clients-- = %d,  session->thread_hdl = 0x%x\r\n",pServer->nr_of_clients, (unsigned int)session->thread_hdl);
            if (isTimeout)
            {
                lviewd_closesocket_for_timeout(session);
                THREAD_DESTROY(session->thread_hdl);
                if (session->childpid)
                {
                    char   command[128];
                    snprintf(command,sizeof(command),"kill %d",session->childpid);
                    system(command);
                }
				session_exit(session);

            }
            else
            {
                THREAD_DESTROY(session->thread_hdl);
				session_exit(session);
            }
        }
        else
        {
            last = search;
            search = search->next;
        }
    }
}

void lviewd_wait_child_close(lviewd_server_t *pServer)
{
    lviewd_session_t *search;
    int counter;

    LVIEW_DIAG("lviewd_wait_child_close  begin\r\n");
    // check if all child are idle
    for (counter=1000; counter; counter--)
    {
        int allThreadIdle = true;

        search = pServer->active_sessions;
        while (search) {
            if (/*(search->waitCommand == false) && */(search->can_delete == false))
            {
                allThreadIdle = false;
                break;
            }
            search = search->next;
        }
        if (allThreadIdle == true)
        {
            break;
        }
        delay_ms(10);
    }
    // close listen socket of each child (make them leave select())
    search = pServer->active_sessions;
    while (search) {
        SOCKET_CLOSE( search->client_socket);
        search = search->next;
    }
    LVIEW_DIAG("lviewd_wait_child_close  end\r\n");
}

static void lviewd_setsockopt_common(int sd,int sock_buf_size, int tos)
{
	int ret, flag;

	flag = sock_buf_size;
    ret = setsockopt(sd , SOL_SOCKET, SO_SNDBUF, (char*)&flag,sizeof(flag));
    if (ret == -1)
        printf("Couldn't setsockopt(SO_SNDBUF)\n");
    ret = setsockopt(sd , SOL_SOCKET, SO_RCVBUF, (char*)&flag,sizeof(flag));
    if (ret == -1)
        printf("Couldn't setsockopt(SO_RCVBUF)\n");

    flag = 1;
    ret = setsockopt( sd, SOL_SOCKET, SO_OOBINLINE, (char *)&flag, sizeof(flag) );
    if (ret == -1)
        printf("Couldn't setsockopt(SO_OOBINLINE)\n");
    flag = 1;
    // set socket non-block
    if (IOCTL( sd, FIONBIO, &flag ) != 0)
    {
        printf("%s: ioctl errno = %d\r\n", __func__, errno);
    }
    // type of service
    if (tos)
    {
         flag = tos;
         ret = setsockopt( sd, IPPROTO_IP, IP_TOS, (char*) &flag, sizeof(flag) );
         if (ret == -1)
             printf("Couldn't setsockopt(IP_TOS)\n");

         printf("tos = %d\n",tos);
    }
	lviewd_set_sock_linger(sd,2);
}

/* ================================================================= */
/*  HTTP protocol handling
 *
 * cyg_http_start() generates an HTTP header with the given content
 * type and, if non-zero, length.
 * cyg_http_finish() just adds a couple of newlines for luck and
 * flushes the stream.
 */


static int recvall(lviewd_session_t* session, char* buf, int len, int isJustOne)
{
    int total = 0;
    int bytesleft = len;
    int n = -1;
    int ret;
    int idletime;
    lviewd_server_t *pServer = &lviewd_server;
    //int ssl_try_again = 0;

    LVIEW_SEND_DIAG("recvall buf = 0x%x, len = %d bytes\n",(int)buf,len);
    while (total < len && (!pServer->isStop) && (!session->exit))
    {
		session->IdleTime = lviewd_get_time();
        #ifdef CONFIG_SUPPORT_SSL
        if (session->is_ssl)
        {
            n = nvtssl_recv(session->ssl, buf+total, bytesleft, 0);
        }
        else
        #endif
        {
			n = recv(session->client_socket, buf+total, bytesleft, 0);
        }
		if ( n == 0)
        {
            // client close the socket
            LVIEW_DIAG("Connection closed by client \n");
            return 0;
        }
        if ( (n < 0 && (errno == EAGAIN)) )
        {
            idletime = 0;
            while(!pServer->isStop && (!session->exit))
            {
                /* wait idletime seconds for progress */
                fd_set rs;
                struct timeval tv;

                FD_ZERO( &rs );
                FD_SET( session->client_socket, &rs );
                tv.tv_sec = 0;
                tv.tv_usec = CYGNUM_SELECT_TIMEOUT_USEC;
				ret = select( session->client_socket + 1, &rs, NULL, NULL, &tv );
                if (ret == 0)
                {
                    idletime++;
                    LVIEW_SEND_DIAG("idletime=%d\r\n",idletime);
                    if (idletime >= pServer->timeoutCnt)
                    {
                        perror("select\r\n");
                        return -1;
                    }
                    else
                        continue;
                }
                else if (ret <0)
                {
                    if (errno == EINTR || errno == EAGAIN)
                        continue;
                    perror("select\r\n");
                    return -1;
                }
                if (FD_ISSET( session->client_socket, &rs ))
                {
                    n = 0;
                    break;
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
    LVIEW_SEND_DIAG("recvall end, total=%d\n",total);
    return n<0?-1:total;
}


static int sendall(lviewd_session_t* session, char* buf, int *len)
{
    int total = 0;
    int bytesleft = *len;
    int n = -1;
    int ret;
    int idletime;
    lviewd_server_t *pServer = &lviewd_server;

    LVIEW_SEND_DIAG("sendall buf = 0x%x, len = %d bytes\n",(int)buf,*len);
    while (total < *len && (!pServer->isStop) && (!session->exit))
    {
		session->IdleTime = lviewd_get_time();
        #ifdef CONFIG_SUPPORT_SSL
        if (session->is_ssl)
        {
            n = nvtssl_send(session->ssl, (buf+total), bytesleft, 0);
        }
        else
        #endif
        {
			n = send(session->client_socket, buf+total, bytesleft, 0);

        }
		if ( n == 0)
        {
            // client close the socket
            LVIEW_DIAG("Connection closed by client \n");
            return 0;
        }
        if ( n < 0 && (errno == EAGAIN) )
        {
            idletime = 0;
            while(!pServer->isStop && (!session->exit))
            {
                /* wait idletime seconds for progress */
                fd_set ws;
                struct timeval tv;

                FD_ZERO( &ws );
                FD_SET( session->client_socket, &ws );
                tv.tv_sec = 0;
                tv.tv_usec = CYGNUM_SELECT_TIMEOUT_USEC;
				ret = select( session->client_socket + 1, NULL, &ws, NULL, &tv );
                if (ret == 0)
                {
                    idletime++;
                    LVIEW_SEND_DIAG("idletime=%d\r\n",idletime);
                    if (idletime >= pServer->timeoutCnt)
                    {
                        perror("select\r\n");
						printf("timeout %d\r\n",idletime);
                        return -1;
                    }
                    else
                        continue;
                }
                else if (ret <0)
                {
                    if (errno == EINTR || errno == EAGAIN)
                        continue;
                    perror("select\r\n");
					printf("errno = %d\r\n",errno);
                    return -1;
                }
                if (FD_ISSET( session->client_socket, &ws ))
                {
                    n = 0;
                    break;
                }
            }
        }
        if (n==-1)
            break;
        total +=n;
        bytesleft -=n;
    }
    *len = total;
    LVIEW_SEND_DIAG("sendall end\n");
    return n<0?-1:total;
}

static void close_old_connection(lviewd_server_t *pServer)
{
	lviewd_session_t *session;

	session = pServer->active_sessions;
	if (session){
		session->exit = 1;
		DBG_DUMP("close_old_connection b, session=0x%x\r\n",(int)session);
		FLAG_WAITPTN(pServer->cond,pServer->eventFlag,CYG_FLG_LVIEWD_SESSION_IDLE,pServer->mtx,true);
		DBG_DUMP("close_old_connection e\r\n");
	}
	MUTEX_WAIT(pServer->s_mutex);
    lviewd_cleanup_sessions(pServer);
	MUTEX_SIGNAL(pServer->s_mutex);
}


static void addconnection(int sd, struct sockaddr* pClient_address, int is_ssl)
{
    lviewd_server_t *pServer = &lviewd_server;

    // cleanup
    MUTEX_WAIT(pServer->s_mutex);
    lviewd_cleanup_sessions(pServer);
    MUTEX_SIGNAL(pServer->s_mutex);
    if (pServer->nr_of_clients >= pServer->max_nr_of_clients) {
		#if 0
        DBG_ERR("Exceed max clients %d\r\n",pServer->max_nr_of_clients);
		lviewd_set_sock_linger(sd,0);
        shutdown(sd, SHUT_RDWR);
        SOCKET_CLOSE(sd);
        return;
        #else
		DBG_WRN("Exceed max clients %d\r\n",pServer->max_nr_of_clients);
		close_old_connection(pServer);
		#endif
    }

    if (session_init(sd, pClient_address, is_ssl) < 0) {
        DBG_ERR("out of memory\r\n");
		lviewd_set_sock_linger(sd,0);
        shutdown(sd, SHUT_RDWR);
        SOCKET_CLOSE(sd);
        return;
    }
}

static void handlenewconnection(int listenfd, int is_ssl)
{
    struct sockaddr client_address;
    socklen_t calen = sizeof(socklen_t);
    int connfd = accept(listenfd, (struct sockaddr *)&client_address, &calen);
    if(connfd < 0)
    {
        printf("%s: accept fail %d, errno %d\r\n", __func__, connfd, errno);
        delay_ms(10);
        return;
    }
    addconnection(connfd, &client_address, is_ssl);
}

static int openlistener(int port)
{
    int sd;
    int err = 0, flag;
    struct addrinfo *res = NULL;//,*p;
    char   portNumStr[10];
    int    portNum;;
    int    sock_buf_size;
    struct addrinfo  address;        // server address

    LVIEW_DIAG("openlistener\n");
    sock_buf_size = liveview_obj.sockbufSize;
    LVIEW_DIAG("sock_buf_size = %d\n",sock_buf_size);
    memset(&address,0,sizeof(address));
    address.ai_family = AF_INET;
    address.ai_socktype = SOCK_STREAM;
    address.ai_flags = AI_PASSIVE; // fill in my IP for me

    portNum = port;
    // check if port number valid
    if (portNum > CYGNUM_MAX_PORT_NUM)
    {
        printf("error port number %d\r\n",portNum);
    }
    snprintf(portNumStr,sizeof(portNumStr), "%d", portNum);
    err = getaddrinfo(NULL, portNumStr, &address, &res);
    if (err!=0 || res == NULL)
    {
        #if _TODO_
        printf("getaddrinfo: %s\n", gai_strerror(err));
		#endif
		return -1;
    }


    /* Get the network going. This is benign if the application has
     * already done this.
     */

    /* Create and bind the server socket.
     */
    LVIEW_DIAG("create socket\r\n");
    sd = socket( res->ai_family, res->ai_socktype, res->ai_protocol );

    LVIEW_DIAG("sd = %d\r\n",sd);
    if (sd < 0)
    {
        printf("Socket create failed\n");
        freeaddrinfo(res);   // free the linked list
        return -1;
    }

    /* Disable the Nagle (TCP No Delay) Algorithm */
    #if 1
    flag = 1;
    err = setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));
    if (err == -1)
        DBG_ERR("Couldn't setsockopt(SO_REUSEADDR)\n");

    #ifdef SO_REUSEPORT
    err = setsockopt( sd, SOL_SOCKET, SO_REUSEPORT, (char*)&flag, sizeof(flag));
    if (err == -1)
        DBG_ERR("Couldn't setsockopt(SO_REUSEPORT)\n");
    #endif
    err = setsockopt( sd , SOL_SOCKET, SO_SNDBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
    if (err == -1)
        DBG_ERR("Couldn't setsockopt(SO_SNDBUF)\n");

    err = setsockopt( sd , SOL_SOCKET, SO_RCVBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
    if (err == -1)
        DBG_ERR("Couldn't setsockopt(SO_RCVBUF)\n");


    #endif
    LVIEW_DIAG("bind %d\r\n",portNum);
    err = bind( sd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);   // free the linked list
    if (err !=0)
    {
        DBG_ERR("bind() returned error\n");
        SOCKET_CLOSE(sd);
        return -1;
    }

    LVIEW_DIAG("listen\r\n");
    err = listen( sd, SOMAXCONN );
    if (err !=0)
    {
        DBG_ERR("listen() returned error\n");
        SOCKET_CLOSE(sd);
        return -1;
    }
    return sd;
}

static void addtoservers(int sd)
{
    lviewd_server_t *pServer = &lviewd_server;
    struct serverstruct *sp = (struct serverstruct *)
                            malloc(sizeof(struct serverstruct));

    if (sp == NULL)
    {
        DBG_ERR("malloc");
        return;
    }
    memset(sp, 0, sizeof(struct serverstruct));
    sp->next = pServer->servers;
    sp->sd = sd;
    pServer->servers = sp;
}






/* ================================================================= */
/* Main processing function                                          */
/*                                                                   */
/* Reads the HTTP header, look it up in the table and calls the      */
/* handler.                                                          */

// this function will return true, if the connection is closed.
static void lviewd_process(lviewd_session_t  *session, struct sockaddr *client_address , int is_push_mode)
{
	#if _TODO_
    int calen = sizeof(*client_address);
	#endif
    //char request[CYGNUM_LVIEWD_SERVER_BUFFER_SIZE];
    char name[64];
    char port[10];
    int  ret;
    lviewd_server_t *pServer = &lviewd_server;
	int  receive_complete = 0;
    char  *headerBuf = session->sendbuff;
    int    recvBytes = 0;
	int    tmpBufSize = CYGNUM_HTTP_BUFF_SIZE-1;
	char  *http_header_end;
	//int client_socket = session->client_socket;

	#if _TODO_
    getnameinfo(client_address, calen, name, sizeof(name),
                port, sizeof(port), NI_NUMERICHOST|NI_NUMERICSERV);
	#else
	strncpy(name, "test", sizeof(name));
	strncpy(port, "80", sizeof(port));
	#endif


	//lviewd_set_sock_nonblock(client_socket);

	while ((!receive_complete) && (!pServer->isStop) && (!session->exit))
    {
		if (recvBytes >= tmpBufSize)
		{
			DBG_ERR("recvBytes %d over tmpBufSize %d\r\n",recvBytes,tmpBufSize);
            goto processing_exit;
		}
        ret = recvall(session,headerBuf+recvBytes, tmpBufSize-recvBytes, true);
        // the connection is closed by client, so we close the client_socket
        if (ret == 0)
        {
            LVIEW_DIAG("Connection closed by client %s[%s]\n",name,port);
            goto processing_exit;
        }
        if (ret < 0)
        {
            perror("recv");
            goto processing_exit;
        }
		if (recvBytes >= HTTP_HEADER_END_LEN)
			http_header_end = lviewd_chk_request_end(headerBuf+recvBytes-HTTP_HEADER_END_LEN,ret+HTTP_HEADER_END_LEN);
		else
			http_header_end = lviewd_chk_request_end(headerBuf,ret+recvBytes);
		if (http_header_end!=NULL)
            receive_complete = true;
        recvBytes +=ret;
    }
	if (!receive_complete)
		goto processing_exit;
    {
        static cyg_lviewd_request_header   req_hdr = {0};

		headerBuf[recvBytes] = 0;
        LVIEW_DIAG("request URL= %s\n",headerBuf);
        // parse http heaer
        if (!lviewd_parse_request(headerBuf,recvBytes, &req_hdr))
        {
            // header not have end tag, need to receive another header data
            goto processing_exit;
        }
        memset(&req_hdr, 0, sizeof(req_hdr));

        int            result=true;
        int            isFirstFrame = true;
        int            hasError = false;

        MUTEX_WAIT(pServer->s_mutex);
        lviewd_JpgQueueInit();
		MUTEX_SIGNAL(pServer->s_mutex);

		FLAG_CLRPTN(pServer->eventFlag,CYG_FLG_LVIEWD_NEW_FRAME,pServer->mtx);
		if (!is_push_mode){
            THREAD_CREATE( liveview_obj.threadPriority-1,
                           "getjpg thread",
                           lview_getjpg_thread,
                           lview_getjpg_thread_main,
                           &hasError,
                           &lview_getjpg_stacks[0],
                           sizeof(lview_getjpg_stacks),
                           &lview_getjpg_thread_object);
            THREAD_RESUME( lview_getjpg_thread );
			//delay_ms(5);
		}
        while (result && (!pServer->isStop) && (!session->exit))
        {
            if (lviewd_JpgQueueIsEmpty())
            {
                FLAG_WAITPTN(pServer->cond,pServer->eventFlag,CYG_FLG_LVIEWD_NEW_FRAME,pServer->mtx,true);
            }
            if (!lviewd_JpgQueueIsEmpty())
            {
				LVIEW_DIAG("cyg_lviewd_send_data\r\n");
				result = cyg_lviewd_send_data(session, true, 1, isFirstFrame);
            }
            isFirstFrame = false;
        }
        if ((!result) || (session->exit))
        {
            hasError =  true;
        }
		if (!is_push_mode){
            LVIEW_DIAG("CYG_FLG_LVIEWD_JPG_TSK_IDLE b\r\n");
            FLAG_WAITPTN(pServer->cond,pServer->eventFlag,CYG_FLG_LVIEWD_JPG_TSK_IDLE,pServer->mtx,false);
            LVIEW_DIAG("CYG_FLG_LVIEWD_JPG_TSK_IDLE e\r\n");
            THREAD_DESTROY(lview_getjpg_thread);
		}
        goto processing_exit;

    }
processing_exit:
    lviewd_closesocket(session);
    LVIEW_DIAG("lviewd_process end\r\n");
}

/* ================================================================= */
/* Initialization thread
 *
 * Optionally delay for a time before getting the network
 * running. Then create and bind the server socket and put it into
 * listen mode. Spawn any further server threads, then enter server
 * mode.
 */

static void lviewd_init(lviewd_server_t *pServer)
{
	int sd;
    int httpPort = liveview_obj.portNum;
	#ifdef CONFIG_SUPPORT_SSL
    int httpsPort = httpPort+1;
	#endif

    if ((sd = openlistener(httpPort)) == -1)
    {
        return;
    }
    addtoservers(sd);
    #ifdef CONFIG_SUPPORT_SSL
    //if (pServer->isSupportHttps)
    if (liveview_obj.is_ssl)
    {
		nvtssl_init();
        pServer->servers->ssl_ctx = NULL;
        pServer->servers->is_ssl = 0;
        if ((sd = openlistener(httpsPort)) == -1)
        {
            return;
        }
        addtoservers(sd);
        pServer->servers->ssl_ctx = nvtssl_ctx_new(NVTSSL_DISPLAY_CERTS,pServer->max_nr_of_clients);
        if (!pServer->servers->ssl_ctx)
        {
            DBG_ERR("ssl_ctx_new \r\n");
        }
        pServer->servers->is_ssl = 1;
        DBG_DUMP("listening on ports %d (http) and %d (https)\n",httpPort, httpsPort);
    }
    else
    #endif
    {
        DBG_DUMP("listening on port %d (http) \n",httpPort);
    }
	lviewd_server.timeoutCnt = liveview_obj.timeoutCnt;
    lviewd_server.time_offset  = lviewd_get_time_offset();
}


THREAD_DECLARE(cyg_lviewd_server,arg)
{
    fd_set master;  // master file descriptor list
    fd_set readfds; // temp file descriptor list for select()
    int fdmax = -1,i;
    int /*ret, */active;
    struct timeval tv;
    #if CYGNUM_LVIEW_PERF_SELECT
    int timeBegin,timeEnd;
    #endif
	lviewd_server_t *pServer = (lviewd_server_t *)arg;
	struct serverstruct *sp;

    lviewd_init(pServer);
	pServer->nr_of_clients = 0;
	pServer->max_nr_of_clients = 1;
    pServer->active_sessions = NULL;
    LVIEW_DIAG("cyg_lview_server , sock_buf_size  =%d\r\n",liveview_obj.sockbufSize);

	// clear the set
    FD_ZERO(&master);
    FD_ZERO(&readfds);
    // add server socket to the set
    sp = pServer->servers;
    while (sp != NULL)  /* read each server port */
    {
        FD_SET(sp->sd, &master);
        if (sp->sd > fdmax)
            fdmax = sp->sd;
        sp = sp->next;
    }
	while(!pServer->isStop)
    {
        // copy it
        readfds = master;
        // set timeout to 500 ms
        tv.tv_sec = 0;
        tv.tv_usec = CYGNUM_SELECT_TIMEOUT_USEC;
        active = select(fdmax+1,&readfds,NULL,NULL,&tv);
        // timeout
        if (active <= 0)
        {
			MUTEX_WAIT(pServer->s_mutex);
            lviewd_cleanup_sessions(pServer);
			MUTEX_SIGNAL(pServer->s_mutex);
			continue;
        }
		// new connection
        sp = pServer->servers;
        while (active > 0 && sp != NULL)
        {
            if (FD_ISSET(sp->sd, &readfds))
            {
                LVIEW_DIAG("sp->sd %d is_ssl=%d\n",sp->sd,sp->is_ssl);
                handlenewconnection(sp->sd, sp->is_ssl);
                active--;
            }
            sp = sp->next;
        }
    }
	MUTEX_WAIT(pServer->s_mutex);
	lviewd_wait_child_close(pServer);
    lviewd_cleanup_sessions(pServer);
    MUTEX_SIGNAL(pServer->s_mutex);
    // close all the opened sockets
    for (i=0; i<= fdmax; i++)
    {
        if (FD_ISSET(i, &master))
        {
            SOCKET_CLOSE(i);
            LVIEW_DIAG("close socket %d\n",i);
        }
    }
	// clean up http & https servers struct
    {
        struct serverstruct *sp;
        while (pServer->servers != NULL)
        {
            #ifdef CONFIG_SUPPORT_SSL
            if (pServer->servers->is_ssl)
                nvtssl_ctx_free(pServer->servers->ssl_ctx);
            #endif
            sp = pServer->servers->next;
            free(pServer->servers);
            pServer->servers = sp;
        }
    }
	FLAG_SETPTN(pServer->cond, pServer->eventFlag, CYG_FLG_LVIEWD_IDLE, pServer->mtx);
    THREAD_RETURN(lview_thread);
}

THREAD_DECLARE(lview_getjpg_thread_main,arg)
{
    struct timeval tv;
    struct timeval tv1,tv2;
    int64_t frameInterval,time1=0,time2=0,targetTime;
    PJPG_QUEUE   pJpgQ = &jpgQueue;
    PMEM_RANGE   pJpgFrame;
    int    ret, frameCount;
    cyg_lviewd_install_obj* pLiveview_obj;
    int    *hasError = (int*)arg;
	lviewd_server_t *pServer = &lviewd_server;


    pLiveview_obj = &liveview_obj;
    frameInterval = 1000000/liveview_obj.frameRate;

    gettimeofday(&tv1,NULL);
    time1 = (int64_t)tv1.tv_sec * 1000000 + tv1.tv_usec;
    frameCount = 0;
    while(!pServer->isStop && !(*hasError))
    {
        targetTime = frameCount*frameInterval;
        gettimeofday(&tv2,NULL);
        time2 = (int64_t)tv2.tv_sec * 1000000 + tv2.tv_usec;
        if (frameCount && (targetTime+time1 > time2))
        {
            tv.tv_sec = 0;
            tv.tv_usec = targetTime+time1-time2;
            //printf("inIdx=%d, time=%lld\n",pJpgQ->inIdx,targetTime-time2+time1);
            ret = select(1,NULL,NULL,NULL,&tv);
        }
        else
        {
            ret = 0;
            //printf("inIdx=%d\n",pJpgQ->inIdx);
        }
        switch(ret)
        {
            case -1:
                //perror("select");
                break;
            case 0:
                // get jpg
                {

                    int index = pJpgQ->inIdx%JPG_QUEUE_ELEMENT_NUM;
                    pJpgFrame = &pJpgQ->frame[index];

                    ret = pLiveview_obj->getJpg(&pJpgFrame->Addr,&pJpgFrame->Size);
                    if (!ret)
                    {
                        DBG_WRN("getJpg fail\r\n");
                        pJpgFrame->Addr = 0;
                        pJpgFrame->Size = 0;
						continue;
                    }
                    else if (pJpgFrame->Addr == 0 || pJpgFrame->Size == 0 )
                    {
                        DBG_WRN("jpgAddr = 0x%x, jpgSize = 0x%x\r\n",pJpgFrame->Addr,pJpgFrame->Size);
                        pJpgFrame->Addr = 0;
                        pJpgFrame->Size = 0;
						continue;
                    }
                    else
                    {
                        //LVIEW_DIAG("jpgAddr = 0x%x, jpgSize=0x%x\n",pJpgFrame->Addr,pJpgFrame->Size);
                    }
                    pJpgQ->inIdx++;

                }
                FLAG_SETPTN(pServer->cond, pServer->eventFlag, CYG_FLG_LVIEWD_NEW_FRAME, pServer->mtx);
                break;
        }
        frameCount++;
    }
    FLAG_SETPTN(pServer->cond,pServer->eventFlag,CYG_FLG_LVIEWD_JPG_TSK_IDLE|CYG_FLG_LVIEWD_NEW_FRAME,pServer->mtx);
    LVIEW_DIAG("CYG_FLG_LVIEWD_JPG_TSK_IDLE\n");
    THREAD_RETURN(lview_getjpg_thread);
}

THREAD_DECLARE(lviewd_session_thrfunc,data) {
    lviewd_session_t *session = (lviewd_session_t *) data;
	cyg_lviewd_install_obj* pLiveview_obj = &liveview_obj;
	lviewd_server_t *pServer = &lviewd_server;

    LVIEW_DIAG("lviewd_session_thrfunc  begin thread_hdl=(0x%x)\r\n",(unsigned int)session->thread_hdl);
    lviewd_notify(pLiveview_obj, CYG_LVIEW_STATUS_CLIENT_REQUEST);
    #if 0 //def CONFIG_SUPPORT_SSL
	if (session->is_ssl)
	{
        if (nvtssl_accept(session->ssl) != NVTSSL_OK)
			goto lviewd_session_exit;
	}
	#endif
    lviewd_process(session, &session->client_address, pLiveview_obj->is_push_mode);
//lviewd_session_exit:
    lviewd_notify(pLiveview_obj, CYG_LVIEW_STATUS_CLIENT_DISCONNECT);
    LVIEW_DIAG("lviewd_session_thrfunc  end thread_hdl=(0x%x)\r\n",(unsigned int)session->thread_hdl);
	session->can_delete = 1;
	FLAG_SETPTN(pServer->cond,pServer->eventFlag,CYG_FLG_LVIEWD_SESSION_IDLE,pServer->mtx);
	THREAD_RETURN(session->thread_hdl);
}







static int cyg_lviewd_http_start( char *buff, char *content_type, int content_length, int close_conn,int serverPush, int isFirstFrame)
{
    int  len;


    if (serverPush)
    {
        if (isFirstFrame)
        {
            sprintf(buff,"HTTP/1.1 200 OK\r\nServer: %s\r\n",CYGDAT_LVIEWD_SERVER_ID);

            strcat( buff, "Connection: close\r\n");

            strcat(buff, "Cache-Control: no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\nPragma: no-cache\r\n");

            strcat(buff,"Content-Type: multipart/x-mixed-replace;boundary=arflebarfle\r\n\r\n--arflebarfle\r\n");
        }
        else
        {
            sprintf(buff,"\r\n--arflebarfle\r\n");
        }
    }
    else
    {
        sprintf(buff,"HTTP/1.1 200 OK\r\nServer: %s\r\n",CYGDAT_LVIEWD_SERVER_ID);
        if (close_conn)
            strcat( buff, "Connection: close\r\n");
        else
            strcat( buff, "Connection: Keep-Alive\r\n");
    }
    if( content_type != NULL )
        sprintf( &buff[strlen(buff)],"Content-Type: %s\r\n", content_type );

    sprintf( &buff[strlen(buff)], "Content-Length: %d\r\n\r\n", content_length );
    len = strlen(buff);
    return len;
}

/* return false means client has closed the connection */
static int cyg_lviewd_send_data(lviewd_session_t  *session,int close_conn, int serverPush, int isFirstFrame)
{
    int jpgAddr = 0, jpgSize = 0, writeBytes;
    int headerlen = 0, /*endlen = 0 ,*/ len = 0;
    cyg_lviewd_install_obj* pLiveview_obj;
    char* sendBuff = session->sendbuff;
    int ret = true;
    #if CYGNUM_LVIEW_PERF_SEND
    struct timeval tv1,tv2;
    int64_t      timeBegin,timeEnd;
    #endif
	int client = session->client_socket;

    pLiveview_obj = &liveview_obj;

    if (!pLiveview_obj->getJpg)
    {
        DBG_ERR("not install\r\n");
        return false;
    }

    // get jpg addr & size
    if (serverPush)
    {
        PMEM_RANGE   pJpgFrame;
        PJPG_QUEUE   pJpgQ = &jpgQueue;

        if (lviewd_JpgQueueIsfull())
        {
            //pJpgQ->outIdx = pJpgQ->inIdx-(JPG_QUEUE_ELEMENT_NUM/2)+1;
            pJpgQ->outIdx = pJpgQ->inIdx-1;
            LVIEW_DIAG("coI=%d\n",pJpgQ->outIdx);
        }
        //printf("outIdx = %d\n",pJpgQ->outIdx);
        pJpgFrame = &pJpgQ->frame[pJpgQ->outIdx%JPG_QUEUE_ELEMENT_NUM];
        jpgAddr = pJpgFrame->Addr;
        jpgSize = pJpgFrame->Size;
        pJpgQ->outIdx++;
        #if LVIEW_DEBUG_LATENCY
        DBG_DUMP("inIdx=%d, outIdx=%d, time = %d ms\n",pJpgQ->inIdx,pJpgQ->outIdx,drv_getSysTick()/1000);
        #endif
		//DBG_DUMP("jpgAddr=0x%x, jpgSize=%d\r\n",jpgAddr,jpgSize);
        //sleep(1);
        //delay_us(30000);
    }
    else
    {
        ret = pLiveview_obj->getJpg(&jpgAddr,&jpgSize);
    }
    if (!ret || jpgAddr == 0 || jpgSize == 0)
    {
        DBG_ERR("Error getJpg fail\r\n");
        jpgAddr = 0;
        jpgSize = 0;
        return true;
    }

    // start send data
    lviewd_notify(pLiveview_obj,CYG_LVIEW_STATUS_SERVER_RESPONSE_START);

    #if CYGNUM_LVIEW_COMBINE_SEND
    if (jpgSize > pLiveview_obj->maxJpgSize)
    {
        DBG_ERR("jpgSize %d > maxJpgSize %d\r\n",jpgSize,pLiveview_obj->maxJpgSize);
        if (!serverPush)
        {
            cyg_lviewd_send_error(session,close_conn,cyg_lviewd_not_found,serverPush);
        }
        return true;
    }
    #endif

    #if CYGNUM_LVIEWD_USE_HTTP_PROTOCOL
    headerlen = cyg_lviewd_http_start( sendBuff, "image/jpeg", jpgSize, close_conn, serverPush, isFirstFrame);
    len += headerlen;
    #endif

    #if CYGNUM_LVIEW_COMBINE_SEND

    if (jpgSize)
    {
        if (pLiveview_obj->hwmem_memcpy)
            pLiveview_obj->hwmem_memcpy((u_int32_t)sendBuff+len,(u_int32_t)jpgAddr,jpgSize);
        else
            memcpy((char*)((u_int32_t)sendBuff+len),(void*)jpgAddr,jpgSize);
    }

    // send jpg data to client
    len += jpgSize;


    #if CYGNUM_LVIEWD_USE_HTTP_PROTOCOL
	/*
    endlen = cyg_lviewd_http_finish((char*)((u_int32_t)sendBuff+len), serverPush);
    len +=endlen;
    */
    #endif

    writeBytes = len;

    #if CYGNUM_LVIEW_PERF_SEND
    gettimeofday(&tv1,NULL);
    timeBegin = (int64_t)tv1.tv_sec * 1000000 + tv1.tv_usec;
    #endif
    if (sendall(session,sendBuff,&writeBytes) <=0)
    {
        perror("sendall");
        printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",len,writeBytes,client);
        ret  = false;
    }
    #if CYGNUM_LVIEW_PERF_SEND
    gettimeofday(&tv2,NULL);
    timeEnd = (int64_t)tv2.tv_sec * 1000000 + tv2.tv_usec;
    printf("Send Bytes = %d, perf=%lld us\r\n",writeBytes,timeEnd-timeBegin);
    #endif

    #else
    writeBytes = headerlen;
    if (sendall(session,(char*)sendBuff,&writeBytes) <=0)
    {
        perror("sendall");
        printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",len,writeBytes,client);
        ret  = false;
    }
    #if CYGNUM_LVIEW_PERF_SEND
    gettimeofday(&tv1,NULL);
    timeBegin = (int64_t)tv1.tv_sec * 1000000 + tv1.tv_usec;
    #endif
    if (jpgSize)
    {
        writeBytes = jpgSize;
        if (sendall(session,(char*)jpgAddr,&writeBytes) <=0)
        {
            perror("sendall");
            printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",len,writeBytes,client);
            ret  = false;
        }
    }
    #if CYGNUM_LVIEW_PERF_SEND
    gettimeofday(&tv2,NULL);
    timeEnd = (int64_t)tv2.tv_sec * 1000000 + tv2.tv_usec;
    printf("perf=%lld us\r\n",timeEnd-timeBegin);
    #endif
	/*
    endlen = cyg_lviewd_http_finish((char*)sendBuff, serverPush);
    if (endlen)
    {
      writeBytes = endlen;
      if (sendall(session,(char*)sendBuff,&writeBytes) <=0)
      {
          perror("sendall");
          printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",len,writeBytes,client);
          ret  = false;
      }
    }
    */
    #endif
    // send data end
    lviewd_notify(pLiveview_obj,CYG_LVIEW_STATUS_SERVER_RESPONSE_END);
    return ret;
}


#if __USE_IPC
/*
 * Declare the message structure.
 */

static int  ipc_msgid;
static void lviewd_ipc_create(void)
{
    if ((ipc_msgid = IPC_MSGGET()) < 0) {
        DBG_ERR("IPC_MSGGET = %d\r\n",ipc_msgid);
    }
	#if defined(__LINUX_680)
	jpg_map_addr = NvtIPC_mmap(NVTIPC_MEM_TYPE_CACHE,liveview_obj.shareMemAddr,liveview_obj.shareMemSize);
	if (jpg_map_addr == NULL)
	{
        DBG_ERR("map error addr=0x%x, size=0x%x\r\n",liveview_obj.shareMemAddr,liveview_obj.shareMemSize);
	}
	jpg_map_size = liveview_obj.shareMemSize;
	#endif
}

static void lviewd_ipc_release(void)
{
    // release message queue
    IPC_MSGREL(ipc_msgid);
	#if defined(__LINUX_680)
	if (jpg_map_addr)
	{
		NvtIPC_munmap(jpg_map_addr, jpg_map_size);
		jpg_map_addr = NULL;
	}
	#endif
}

static void lviewd_ipc_sendmsg(int cmd,int status)
{
    LVIEWD_IPC_NOTIFY_MSG sbuf;
    size_t buf_length;

    sbuf.mtype = LVIEWD_IPC_MSG_TYPE_S2C;

    sbuf.uiIPC = cmd;

    sbuf.notifyStatus = status;

    buf_length = sizeof(sbuf);

    /*
     * Send a message.
     */
    if (IPC_MSGSND(ipc_msgid, &sbuf, buf_length) < 0) {
        perror("msgsnd");
    }

}

static void lviewd_ipc_notify(int Notifystatus)
{
    LVIEWD_IPC_NOTIFY_MSG sbuf;
    size_t buf_length;

    sbuf.mtype = LVIEWD_IPC_MSG_TYPE_S2C;

    sbuf.uiIPC = LVIEWD_IPC_NOTIFY_CLIENT;

    sbuf.notifyStatus = Notifystatus;

    buf_length = sizeof(sbuf);

    /*
     * Send a message.
     */
    if (IPC_MSGSND(ipc_msgid, &sbuf, buf_length) < 0) {
        perror("msgsnd");
    }
}

static int lviewd_ipc_getjpg(int* jpgAddr, int* jpgSize)
{
	lviewd_server_t *pServer = &lviewd_server;
    LVIEWD_IPC_GETJPG_MSG sbuf;
    size_t buf_length;

    sbuf.mtype = LVIEWD_IPC_MSG_TYPE_S2C;

    sbuf.uiIPC = LVIEWD_IPC_GET_JPG;

    buf_length = sizeof(sbuf);

    /*
     * Send a message.
     */
    //gettimeofday(&tv,NULL);
    //timeBegin = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    if (IPC_MSGSND(ipc_msgid, &sbuf, buf_length) < 0)
    {
        perror("msgsnd");
        return false;
    }
    DBG_IND("Get jpg Req\r\n");
    #if 0
    FLAG_WAITPTN_TIMEOUT(pServer->cond,pServer->eventFlag,CYG_FLG_LVIEWD_GET_JPG_ACK,pServer->mtx,true, 5, rtn);
    #else
    FLAG_WAITPTN(pServer->cond,pServer->eventFlag,CYG_FLG_LVIEWD_GET_JPG_ACK,pServer->mtx,true);

    #endif
    if (jpgData.Size)
    {
		#if 1
        *jpgAddr=ipc_getCacheAddr(jpgData.Addr);
		if ((unsigned int)*jpgAddr == NVTIPC_MEM_INVALID_ADDR)
			return false;
        *jpgSize=jpgData.Size;
		if (ipc_flushCache(*jpgAddr,*jpgSize) < 0)
			return false;
		#else
		*jpgAddr=ipc_getNonCacheAddr(jpgData.Addr);
        *jpgSize=jpgData.Size;
		#endif
        return true;
    }
    else
        return false;

}

THREAD_DECLARE(lview_close_thrfunc,arg) {

    cyg_lviewd_stop();
    THREAD_RETURN(lview_close_thread);
}

THREAD_DECLARE(lview_ipc_rcv_main_func,arg)
{
    int              continue_loop = 1, ret;
    LVIEWD_IPC_MSG   sbuf;
	lviewd_server_t *pServer = &lviewd_server;

    LVIEW_DIAG("rcv_main b\r\n");
    while (continue_loop)
    {
        if ((ret = IPC_MSGRCV(ipc_msgid, &sbuf, LVIEWD_IPC_MSGSZ)) < 0)
        {
            DBG_ERR("IPC_MSGRCV ret=%d\r\n",(int)ret);
            continue_loop = 0;
            continue;
        }
		//LVIEW_DIAG("rcv uiIPC=0x%x\r\n",sbuf.uiIPC);
        switch (sbuf.uiIPC)
        {
            case LVIEWD_IPC_CLOSE_SERVER:
                //continue_loop = 0;
                LVIEW_DIAG("LVIEWD_IPC_CLOSE_SERVER\r\n");
                THREAD_CREATE( liveview_obj.threadPriority-1,
                           "lviewd ipc rcv thread",
                           lview_close_thread,
                           lview_close_thrfunc,
                           NULL,
                           &lview_close_thread_stacks[0],
                           sizeof(lview_close_thread_stacks),
                           &lview_close_thread_object
                );
                THREAD_RESUME( lview_close_thread );
                break;
            case LVIEWD_IPC_CLOSE_FINISH:
                continue_loop = 0;
                break;
            case LVIEWD_IPC_NOTIFY_CLIENT_ACK:
                FLAG_SETPTN(pServer->cond,pServer->eventFlag,CYG_FLG_LVIEWD_NOTIFY_CLIENT_ACK,pServer->mtx);
                break;
            case LVIEWD_IPC_GET_JPG_ACK:
                {
                    DBG_IND("Get jpg Ack\r\n");
                    LVIEWD_IPC_GETJPG_MSG *pMsg = (LVIEWD_IPC_GETJPG_MSG *)&sbuf;
                    jpgData.Addr = pMsg->jpgAddr;
                    jpgData.Size = pMsg->jpgSize;
                    FLAG_SETPTN(pServer->cond,pServer->eventFlag,CYG_FLG_LVIEWD_GET_JPG_ACK,pServer->mtx);
                }
                break;
			case LVIEWD_IPC_PUSH_FRAME:
                {
                    DBG_IND("push frame\r\n");
                    LVIEWD_IPC_PUSH_FRAME_MSG *pMsg = (LVIEWD_IPC_PUSH_FRAME_MSG *)&sbuf;
					cyg_lviewd_frame_info     frame_info;

					frame_info.addr = ipc_getCacheAddr(pMsg->addr);
					if (frame_info.addr == NVTIPC_MEM_INVALID_ADDR)
						break;
					if (ipc_flushCache(frame_info.addr,pMsg->size) < 0)
						break;
					frame_info.size = pMsg->size;
					cyg_lviewd_push_frame(&frame_info);


                }
                break;

            default:
                DBG_ERR("Unknown uiIPC=%d\r\n",(int)sbuf.uiIPC);
                break;
        }

    }
    cyg_lviewd_release_resource(pServer);
    #if __BUILD_PROCESS
    sem_post(&main_sem);
    #endif
	LVIEW_DIAG("rcv_main e\r\n");
    THREAD_RETURN(lview_rcv_thread);
}

#endif

extern void cyg_lviewd_install( cyg_lviewd_install_obj*  pObj)
{
    liveview_obj.getJpg = pObj->getJpg;
    liveview_obj.notify = pObj->notify;
    liveview_obj.hwmem_memcpy = pObj->hwmem_memcpy;
    #if __USE_IPC
    liveview_obj.notify = lviewd_ipc_notify;
    liveview_obj.getJpg = lviewd_ipc_getjpg;
    #endif

    #if !__USE_IPC
    if (!pObj->getJpg)
    {
        DBG_ERR("not install getJpg function\r\n");
    }
    #endif
    if (!pObj->portNum)
    {
        liveview_obj.portNum = CYGNUM_LVIEWD_SERVER_PORT;
        DBG_WRN("set default port %d\r\n",CYGNUM_LVIEWD_SERVER_PORT);
    }
    else
    {
        liveview_obj.portNum = pObj->portNum;
    }
    if (!pObj->threadPriority)
    {
        liveview_obj.threadPriority = CYGNUM_LVIEWD_THREAD_PRIORITY;
        DBG_WRN("set default threadPriority %d\r\n",CYGNUM_LVIEWD_THREAD_PRIORITY);
    }
    else
    {
        liveview_obj.threadPriority = pObj->threadPriority;
    }
    if (!pObj->maxJpgSize)
    {
        liveview_obj.maxJpgSize = CYGNUM_MAX_JPG_SIZE;
        //DBG_WRN("set default maxJpgSize %d \r\n",CYGNUM_MAX_JPG_SIZE);
    }
    else
    {
        liveview_obj.maxJpgSize = pObj->maxJpgSize;
    }
    if (!pObj->frameRate)
    {
        liveview_obj.frameRate = CYGNUM_LVIEW_FRAME_RATE;
        DBG_WRN("set default frameRate %d\r\n",CYGNUM_LVIEW_FRAME_RATE);
    }
    else
    {
        liveview_obj.frameRate = pObj->frameRate;
    }
    if (!pObj->sockbufSize)
    {
        liveview_obj.sockbufSize = CYGNUM_LVIEW_SOCK_BUF_SIZE;
        DBG_WRN("set default sockSendbufSize %d\r\n",CYGNUM_LVIEW_SOCK_BUF_SIZE);
    }
    else
    {
        liveview_obj.sockbufSize = pObj->sockbufSize;
    }
	liveview_obj.shareMemAddr = pObj->shareMemAddr;
    liveview_obj.shareMemSize = pObj->shareMemSize;
    liveview_obj.tos = pObj->tos;
	liveview_obj.timeoutCnt = pObj->timeoutCnt;
	if (liveview_obj.timeoutCnt == 0)
    {
        liveview_obj.timeoutCnt = CYGNUM_LVIEW_SERVER_IDLE_TIME;
        DBG_WRN("set default timeouCnt %d\r\n",CYGNUM_LVIEW_SERVER_IDLE_TIME);
    }
	#ifdef CONFIG_SUPPORT_SSL
	liveview_obj.is_ssl = pObj->is_ssl;
	#endif
	liveview_obj.is_push_mode = pObj->is_push_mode;
	DBG_DUMP("fps=%d,sockbufSize=%d,tos=0x%x,timeoutCnt=%d,ssl=%d,push_mode=%d\r\n",liveview_obj.frameRate,liveview_obj.sockbufSize,liveview_obj.tos,liveview_obj.timeoutCnt,liveview_obj.is_ssl,liveview_obj.is_push_mode);
}





/* ================================================================= */
/* System initializer
 *
 * This is called from the static constructor in init.cxx. It spawns
 * the main server thread and makes it ready to run. It can also be
 * called explicitly by the application if the auto start option is
 * disabled.
 */

static void cyg_lviewd_create_resource(lviewd_server_t *pServer)
{
	#if __USE_IPC
    // init ipc
    lviewd_ipc_create();
    #endif
	MUTEX_CREATE(pServer->mtx,1);
	MUTEX_CREATE(pServer->s_mutex,1);
    COND_CREATE(pServer->cond);
}

static void cyg_lviewd_release_resource(lviewd_server_t *pServer)
{
	COND_DESTROY(pServer->cond);
    MUTEX_DESTROY(pServer->mtx);
	MUTEX_DESTROY(pServer->s_mutex);
	#if __USE_IPC
	// release ipc
    lviewd_ipc_release();
	#endif
}



void cyg_lviewd_startup(void)
{
	lviewd_server_t *pServer = &lviewd_server;
	MUTEX_WAIT(openclose_mtx);
	LVIEW_DIAG("cyg_lviewd_startup pServer->isStart = %d, sizeof mutex_t = %d\r\n",pServer->isStart,sizeof(pthread_mutex_t));
    if (!pServer->isStart)
    {
        #if __USE_IPC
        // init ipc
        //lviewd_ipc_create();
        #endif
		memset(pServer,0x00,sizeof(lviewd_server_t));
        pServer->isStop = false;

		cyg_lviewd_create_resource(pServer);

        //MUTEX_CREATE(pServer->mtx,1);
		//MUTEX_CREATE(pServer->s_mutex,1);
        //COND_CREATE(pServer->cond);
        FLAG_CLRPTN(pServer->eventFlag,0xFFFFFFFF,pServer->mtx);
        LVIEW_DIAG("cyg_lviewd_startup create thread\r\n");
        THREAD_CREATE( liveview_obj.threadPriority,
                           "Liveview daemon",
                           lview_thread,
                           cyg_lviewd_server,
                           pServer,
                           &lview_stacks[0],
                           sizeof(lview_stacks),
                           &lview_thread_object
            );
        LVIEW_DIAG("cyg_lviewd_startup resume thread\r\n");
        THREAD_RESUME( lview_thread );


        #if __USE_IPC
        // create ipc receive thread
        THREAD_CREATE( liveview_obj.threadPriority,
                           "lviewd ipc rcv thread",
                           lview_rcv_thread,
                           lview_ipc_rcv_main_func,
                           NULL,
                           &lview_rcv_thread_stacks[0],
                           sizeof(lview_rcv_thread_stacks),
                           &lview_rcv_thread_object
            );
        THREAD_RESUME( lview_rcv_thread );
        lviewd_ipc_sendmsg(LVIEWD_IPC_SERVER_STARTED,0);
        #endif
        pServer->isStart = true;
    }
	MUTEX_SIGNAL(openclose_mtx);

}

void cyg_lviewd_stop(void)
{
	lviewd_server_t *pServer = &lviewd_server;

	LVIEW_DIAG("cyg_lviewd_stop\r\n");
	MUTEX_WAIT(openclose_mtx);
    if (pServer->isStart)
    {
        pServer->isStop = true;
        //FLAG_MASK(pServer->cond,~CYG_FLG_LVIEWD_START);
        LVIEW_DIAG("cyg_lviewd_stop wait idle begin\r\n");
		FLAG_SETPTN(pServer->cond, pServer->eventFlag, CYG_FLG_LVIEWD_NEW_FRAME, pServer->mtx);
        FLAG_WAITPTN(pServer->cond,pServer->eventFlag,CYG_FLG_LVIEWD_IDLE,pServer->mtx,false);
        LVIEW_DIAG("cyg_lviewd_stop wait idle end\r\n");
        THREAD_DESTROY(lview_thread);
        //COND_DESTROY(pServer->cond);
        //MUTEX_DESTROY(pServer->mtx);
		//MUTEX_DESTROY(pServer->s_mutex);
        #if __USE_IPC
        {
            LVIEWD_IPC_MSG   sbuf;
            // notify client to exit
            sbuf.mtype = LVIEWD_IPC_MSG_TYPE_S2C;
            sbuf.uiIPC = LVIEWD_IPC_CLOSE_SERVER_ACK;
            if (IPC_MSGSND(ipc_msgid, &sbuf, LVIEWD_IPC_MSGSZ) < 0)
            {
                perror("msgsnd");
            }
        }
        #else
		cyg_lviewd_release_resource(pServer);
        #endif
        pServer->isStart = false;
		//pServer->isStop = false;
        LVIEW_DIAG("cyg_lviewd_stop ->end\r\n");
    }
	MUTEX_SIGNAL(openclose_mtx);
}

void  cyg_lviewd_startup2(char* cmd)
{
    int   portNum = 80, threadPriority = 6, maxJpgSize = 200*1024 , frameRate=30, sockBufSize=50*1024, is_ssl =0, timeoutCnt = CYGNUM_LVIEW_SERVER_IDLE_TIME, tos = 0, shareMemAddr = 0, shareMemSize = 0, is_push_mode = 0;
    char  *delim=" ";
    int   interface_ver = 0;
    cyg_lviewd_install_obj  lviewObj={0};


    LVIEW_DIAG("cyg_lviewd_startup2 cmd = %s\r\n",cmd);
    char *test = strtok(cmd, delim);
    while (test != NULL)
    {
		if (!strcmp(test, "-pm"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                is_push_mode = atoi(test);
            }
        }
        else if (!strcmp(test, "-p"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                portNum = atoi(test);
            }
        }
		// tos
        else if (!strcmp(test, "-tos"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                tos = atoi(test);
            }
        }
		// timeout count
        else if (!strcmp(test, "-to"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                timeoutCnt = atoi(test);
            }
        }
        else if (!strcmp(test, "-t"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                threadPriority = atoi(test);
            }
        }
        else if (!strcmp(test, "-f"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                frameRate = atoi(test);
            }
        }
		else if (!strcmp(test, "-ssl"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                is_ssl = atoi(test);
            }
        }
        else if (!strcmp(test, "-s"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                sockBufSize = atoi(test);
            }
        }
        else if (!strcmp(test, "-j"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                maxJpgSize = atoi(test);
            }
        }
		else if (!strcmp(test, "-m"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                sscanf( test, "%x", &shareMemAddr);
            }
			test = strtok(NULL, delim);
            if (test != NULL)
            {
                sscanf( test, "%x", &shareMemSize);
            }
        }
        // interface version
        else if (!strcmp(test, "-v"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                sscanf( test, "0x%x", &interface_ver);;
                DBG_IND(" interface_ver = 0x%x\r\n",interface_ver);
            }
        }
        test = strtok(NULL, delim);
    }
    if (LVIEWD_INTERFACE_VER != interface_ver)
    {
        DBG_ERR("version mismatch = 0x%x, 0x%x\r\n",interface_ver,LVIEWD_INTERFACE_VER);
        return;
    }

    LVIEW_DIAG("portNum=%d,threadPriority=%d,maxJpgSize=%d,frameRate=%d,sockBufSize=%d\r\n",portNum,threadPriority,maxJpgSize,frameRate,sockBufSize);

    // set port number
    lviewObj.portNum = portNum;
    // set http live view server thread priority
    lviewObj.threadPriority = threadPriority;
    // set the maximum JPG size that http live view server to support
    lviewObj.maxJpgSize = maxJpgSize;
    // live view streaming frame rate
    lviewObj.frameRate = frameRate;
    // socket buffer size
    lviewObj.sockbufSize = sockBufSize;
	lviewObj.is_ssl = is_ssl;
	lviewObj.timeoutCnt = timeoutCnt;
	lviewObj.tos = tos;
    lviewObj.shareMemAddr = shareMemAddr;
	lviewObj.shareMemSize = shareMemSize;
	lviewObj.is_push_mode = is_push_mode;
    // reserved parameter , no use now, just fill NULL.
    lviewObj.arg = NULL;
    // install the CB functions and set some parameters
    cyg_lviewd_install(&lviewObj);
    // start the http live view server
    cyg_lviewd_startup();
}

void cyg_lviewd_push_frame(cyg_lviewd_frame_info  *frame_info)
{
	lviewd_server_t *pServer = &lviewd_server;
	PJPG_QUEUE                pJpgQ = &jpgQueue;
	PMEM_RANGE                pJpgFrame;

    int index = pJpgQ->inIdx%JPG_QUEUE_ELEMENT_NUM;
    pJpgFrame = &pJpgQ->frame[index];
	DBG_IND("Addr = 0x%x, size =0x%x\r\n",frame_info->addr,frame_info->size);
	pJpgFrame->Addr = frame_info->addr;
    pJpgFrame->Size = frame_info->size;
	DBG_IND("Addr = 0x%x, size =0x%x\r\n",pJpgFrame->Addr,pJpgFrame->Size);
	MUTEX_WAIT(pServer->s_mutex);
    pJpgQ->inIdx++;
	MUTEX_SIGNAL(pServer->s_mutex);
	FLAG_SETPTN(pServer->cond, pServer->eventFlag, CYG_FLG_LVIEWD_NEW_FRAME, pServer->mtx);
#if 0
		{
			uint64_t                  counter, curtime;
			curtime = lviewd_get_time();
			counter = curtime - pServer->time_offset;
			DBG_DUMP("curtime = %lld, time_offset = %lld, [%4d.%06d]\r\n", curtime, pServer->time_offset, (uint32_t)(counter / 1000000), (uint32_t)(counter % 1000000));
		}
#endif
}



#if __BUILD_PROCESS


int main(int argc, char *argv[])
{
    u_int32_t  portNum = 80, threadPriority = 6, maxJpgSize = 200*1024 , frameRate=30, sockBufSize=50*1024, shareMemAddr = 0, shareMemSize = 0, is_ssl =0,  is_push_mode = 0, timeoutCnt = CYGNUM_LVIEW_SERVER_IDLE_TIME, tos = 0;
    int i = 0;
    int interface_ver = 0;

    // ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    while (i < argc)
    {
        if (!strcmp(argv[i], "-pm"))
        {
            i++;
            if (argv[i])
            {
                is_push_mode = atoi(argv[i]);
            }
        }
        else if (!strcmp(argv[i], "-p"))
        {
            i++;
            if (argv[i])
            {
                portNum = atoi(argv[i]);
            }
        }
		// tos
        else if (!strcmp(argv[i], "-tos"))
        {
            i++;
            if (argv[i])
            {
                tos = atoi(argv[i]);
            }
        }
		// timeout count
        else if (!strcmp(argv[i], "-to"))
        {
            i++;
            if (argv[i])
            {
                timeoutCnt = atoi(argv[i]);
            }
        }
        else if (!strcmp(argv[i], "-t"))
        {
            i++;
            if (argv[i])
            {
                threadPriority = atoi(argv[i]);
            }
        }
        else if (!strcmp(argv[i], "-f"))
        {
            i++;
            if (argv[i])
            {
                frameRate = atoi(argv[i]);
            }
        }
		else if (!strcmp(argv[i], "-ssl"))
        {
            i++;
            if (argv[i])
            {
                is_ssl = atoi(argv[i]);
            }
        }
        else if (!strcmp(argv[i], "-s"))
        {
            i++;
            if (argv[i])
            {
                sockBufSize = atoi(argv[i]);
            }
        }
        else if (!strcmp(argv[i], "-j"))
        {
            i++;
            if (argv[i])
            {
                maxJpgSize = atoi(argv[i]);
            }
        }
		else if (!strcmp(argv[i], "-m"))
        {
            i++;
            if (argv[i])
            {
                sscanf( argv[i], "%x", &shareMemAddr);
            }
			i++;
            if (argv[i])
            {
                sscanf( argv[i], "%x", &shareMemSize);
            }
        }

        // interface version
        else if (!strcmp(argv[i], "-v"))
        {
            i++;
            if (argv[i])
            {
                sscanf( argv[i], "0x%x", &interface_ver);
                DBG_IND(" interface_ver = 0x%x\r\n",interface_ver);
            }
        }
        i++;
    }
    if (LVIEWD_INTERFACE_VER != interface_ver)
    {
        DBG_ERR("lviewd version mismatch = 0x%x, 0x%x\r\n",interface_ver,LVIEWD_INTERFACE_VER);
        return -1;
    }
    LVIEW_DIAG("portNum=%d,threadPriority=%d,maxJpgSize=%d,frameRate=%d,sockBufSize=%d\r\n",portNum,threadPriority,maxJpgSize,frameRate,sockBufSize);
    #if __USE_IPC
    // start Http liveview daemon
    sem_init(&main_sem,0,0);
	cyg_lviewd_install_obj  lviewObj={0};

    // set port number
    lviewObj.portNum = portNum;
    // set http live view server thread priority
    lviewObj.threadPriority = threadPriority;
    // set the maximum JPG size that http live view server to support
    lviewObj.maxJpgSize = maxJpgSize;
    // live view streaming frame rate
    lviewObj.frameRate = frameRate;
    // socket buffer size
    lviewObj.sockbufSize = sockBufSize;
    // is use ssl
	lviewObj.is_ssl = is_ssl;
	lviewObj.shareMemAddr = shareMemAddr;
	lviewObj.shareMemSize = shareMemSize;
	lviewObj.is_push_mode = is_push_mode;
    lviewObj.timeoutCnt = timeoutCnt;
	lviewObj.tos = tos;
    // reserved parameter , no use now, just fill NULL.
    lviewObj.arg = NULL;
    // install the CB functions and set some parameters
    cyg_lviewd_install(&lviewObj);
    // start the http live view server
    cyg_lviewd_startup();
    // wait close
    //FLAG_WAITPTN(pServer->cond,pServer->eventFlag,CYG_FLG_LVIEWD_CLOSED,pServer->mtx,false);
    sem_wait(&main_sem);
    #else
    {
        sem_init(&main_sem,0,0);
        sem_wait(&main_sem);
    }
    #endif
    sem_destroy(&main_sem);
    return 0;
}

#endif

/* ----------------------------------------------------------------- */
/* end of lview.c                                                    */
