#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__FREERTOS)
#define LWIP_POSIX_SOCKETS_IO_NAMES 0
#endif
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#if defined(__LINUX_USER__)
#include <pthread.h>
#include <semaphore.h>
#include <dirent.h>
#include <sys/ioctl.h>
#elif defined(__FREERTOS)
#include <kwrap/type.h>
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#include <FreeRTOS_POSIX/dirent.h>
#include <kwrap/semaphore.h>
#include <kwrap/task.h>
#endif
#include <HfsNvt/hfs.h>
#include <time.h>
#include <pkgconf/hfs.h>
#if defined(__ECOS)
#ifdef __USE_IPC
#include <cyg/nvtipc/NvtIpcAPI.h>
#endif
#ifdef CONFIG_SUPPORT_SSL
#include <cyg/nvtssl/nvtssl.h>
#endif
#else
#ifdef __USE_IPC
#include <nvtipc.h>
#endif
//#include <sys/mman.h>
//#include <sys/ipc.h>
//#include <sys/msg.h>
//#include <sys/wait.h>
//#include <semaphore.h>
#endif

#ifdef CONFIG_SUPPORT_SSL
#include <nvtssl.h>
#endif
#if CONFIG_SUPPORT_AUTH
#include <nvtauth.h>
#endif
#if defined(__FREERTOS)
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#define SOMAXCONN               128
#endif



#if 0
#if 1
#define HFS_DIAG      printf
#define HFS_CONN_DIAG printf
#define HFS_SEND_DIAG printf
#define HFS_RW_DIAG   printf
#define HFS_FLAG_DIAG printf
#define HFS_TIME_DIAG printf
#define HFS_AUTH_DIAG printf
#define HFS_PERF_TIME      1
#else
#define HFS_DIAG      printf
#define HFS_CONN_DIAG printf
#define HFS_SEND_DIAG(...)
#define HFS_RW_DIAG(...)
#define HFS_FLAG_DIAG(...)
#define HFS_TIME_DIAG(...)
#define HFS_AUTH_DIAG(...)
#define HFS_PERF_TIME      0
#endif
#else
#define HFS_DIAG(...)
#define HFS_CONN_DIAG(...)
#define HFS_SEND_DIAG(...)
#define HFS_RW_DIAG(...)
#define HFS_FLAG_DIAG(...)
#define HFS_TIME_DIAG(...)
#define HFS_AUTH_DIAG(...)
#define HFS_PERF_TIME      0
#endif

#define PRINT_OUTFILE            stdout
#define CHKPNT                   fprintf(PRINT_OUTFILE, "\033[37mCHK: %d, %s\033[0m\r\n",__LINE__,__func__) ///< Show a color sting of line count and function name in your insert codes

#define DBG_WRN(fmtstr, args...) fprintf(PRINT_OUTFILE, "\033[33mhfs WRN:%s(): \033[0m" fmtstr,__func__, ##args)
#define DBG_ERR(fmtstr, args...) fprintf(PRINT_OUTFILE, "\033[31mhfs ERR:%s(): \033[0m" fmtstr,__func__, ##args)
#define DBG_DUMP(fmtstr, args...) fprintf(PRINT_OUTFILE,fmtstr, ##args)

#if 0
#define DBG_IND(fmtstr, args...)  fprintf(PRINT_OUTFILE, "%s(): " fmtstr, __func__, ##args)
#else
#define DBG_IND(fmtstr, args...)
#endif


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
#if CONFIG_SUPPORT_CGI
#define MUTEX_CREATE(handle,init_cnt) { pthread_mutexattr_t mattr; pthread_mutexattr_init(&mattr); \
                                      pthread_mutexattr_setpshared(&mattr,PTHREAD_PROCESS_SHARED); \
                                      pthread_mutex_init(&handle,&mattr); pthread_mutexattr_destroy(&mattr);\
                                      if(init_cnt==0)pthread_mutex_lock(&handle);}
#else
#define MUTEX_CREATE(handle,init_cnt) pthread_mutex_init(&handle,NULL);if(init_cnt==0)pthread_mutex_lock(&handle)
#endif
#define MUTEX_SIGNAL(handle) pthread_mutex_unlock(&handle)
#define MUTEX_WAIT(handle) pthread_mutex_lock(&handle);
#define MUTEX_DESTROY(handle) pthread_mutex_destroy(&handle)
#define COND_HANDLE pthread_cond_t
#define COND_CREATE(handle) pthread_cond_init(&handle,NULL)
#define COND_SIGNAL(handle) pthread_cond_broadcast(&handle)
#define COND_WAIT(handle,mtx) pthread_cond_wait(&handle, &mtx)
#define COND_DESTROY(handle) pthread_cond_destroy(&handle)
#define FLAG_SETPTN(cond,flag,ptn,mtx) {pthread_mutex_lock(&mtx);flag|=ptn;HFS_FLAG_DIAG("setflag 0x%x\r\n",flag);pthread_cond_broadcast(&cond);pthread_mutex_unlock(&mtx);}
#define FLAG_CLRPTN(flag,ptn,mtx) {pthread_mutex_lock(&mtx);flag &= ~(ptn);pthread_mutex_unlock(&mtx);}
#define FLAG_WAITPTN(cond,flag,ptn,mtx,clr) {pthread_mutex_lock(&mtx);while (1) { HFS_FLAG_DIAG("waitflag 0x%x\r\n",flag);\
	                                                                if (flag & ptn){if (clr) {flag &= ~(ptn);}pthread_mutex_unlock(&mtx);break;}\
                                                                  HFS_FLAG_DIAG("cond_wait b 0x%x\r\n",flag);pthread_cond_wait(&cond, &mtx);\
                                                                  HFS_FLAG_DIAG("cond_wait e flag 0x%x, ptn 0x%x\r\n", flag, ptn);\
                                                                  if (flag & ptn) {\
                                                                  if (clr) {flag &= ~(ptn);}\
                                                                  pthread_mutex_unlock(&mtx);\
                                                                  break;}\
                                                                  }}

#define FLAG_WAITPTN_TIMEOUT(cond,flag,ptn,mtx,clr,timeout,rtn){struct timespec to;int err;rtn=0;\
                                                         pthread_mutex_lock(&mtx);\
                                                         while (1) { HFS_FLAG_DIAG("waitflag 0x%x\r\n",flag);\
	                                                               if (flag & ptn){if (clr) {flag &= ~(ptn);}pthread_mutex_unlock(&mtx);break;}\
                                                                   HFS_FLAG_DIAG("cond_wait 0x%x\r\n",flag);\
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
#define IPC_MSGGET() NvtIPC_MsgGet(NvtIPC_Ftok(HFS_IPC_TOKEN_PATH))
#define IPC_MSGREL(msqid) NvtIPC_MsgRel(msqid)
#define IPC_MSGSND(msqid,pMsg,msgsz) NvtIPC_MsgSnd(msqid,NVTIPC_SENDTO_CORE1,pMsg,msgsz)
#define IPC_MSGRCV(msqid,pMsg,msgsz) NvtIPC_MsgRcv(msqid,pMsg,msgsz)
#else
#define IPC_MSGGET() msgget(HFS_IPCKEYID, IPC_CREAT | 0666 )
#define IPC_MSGREL(msqid) msgctl(msqid, IPC_RMID , NULL)
#define IPC_MSGSND(msqid,pMsg,msgsz) msgsnd(msqid,pMsg,msgsz-4, IPC_NOWAIT)
#define IPC_MSGRCV(msqid,pMsg,msgsz) msgrcv(msqid,pMsg,msgsz-4,HFS_IPC_MSG_TYPE_C2S, 0)
#endif

#if defined(__LINUX_660)
#define ipc_getPhyAddr(addr)        NvtMem_GetPhyAddr(addr)
#define ipc_getNonCacheAddr(addr)   NvtMem_GetNonCacheAddr(addr)
#define ipc_getCacheAddr(addr)      dma_getCacheAddr(addr)
#define ipc_getMmioAddr(addr)       dma_getMmioAddr(addr)
#define ipc_flushCache(addr,size)   cacheflush((char*)addr, size, DCACHE)
#elif defined(__LINUX_680)
#define ipc_getPhyAddr(addr)        NvtIPC_GetPhyAddr(addr)
#define ipc_getNonCacheAddr(addr)   NvtIPC_GetNonCacheAddr(addr)
#define ipc_getMmioAddr(addr)       NvtIPC_GetMmioAddr(addr)
#else
#define ipc_getPhyAddr(addr)        (addr-0xA0000000)
#define ipc_getNonCacheAddr(addr)   (addr+0xA0000000)
#define ipc_getCacheAddr(addr)      (addr+0x80000000)
#define ipc_getMmioAddr(addr)       (addr)
#define ipc_flushCache(addr,size)   HAL_DCACHE_FLUSH(addr,size)
#endif

#define my_strncpy(dst,src, num)    {strncpy(dst,src,num);dst[num-1]=0;}
//#define my_strncat(dst,src, num)    {strncat(dst,src,num);dst[num-1]=0;}

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

#if defined(__ECOS)
#define CYGNUM_HFS_SERVER_STACK_SIZE                     8*1024
#define CYGNUM_HFS_SESSION_THREAD_STACK_SIZE            24*1024
#define CYGNUM_HFS_SESSION_DATA_THREAD_STACK_SIZE        8*1024
#define CYGNUM_HFS_RCV_THREAD_STACK_SIZE                 2*1024
#define CYGNUM_HFS_CLOSE_THREAD_STACK_SIZE               2*1024
#elif defined(__FREERTOS)
#define CYGNUM_HFS_SERVER_STACK_SIZE                     8*1024
#define CYGNUM_HFS_SESSION_THREAD_STACK_SIZE             18*1024
#define CYGNUM_HFS_SESSION_DATA_THREAD_STACK_SIZE        18*1024
#define CYGNUM_HFS_RCV_THREAD_STACK_SIZE                 4*1024
#define CYGNUM_HFS_CLOSE_THREAD_STACK_SIZE               4*1024
#else
#define CYGNUM_HFS_SERVER_STACK_SIZE                     4
#define CYGNUM_HFS_SESSION_THREAD_STACK_SIZE             4
#define CYGNUM_HFS_SESSION_DATA_THREAD_STACK_SIZE        4
#define CYGNUM_HFS_RCV_THREAD_STACK_SIZE                 4
#define CYGNUM_HFS_CLOSE_THREAD_STACK_SIZE               4
#endif

#define CYGNUM_MAX_PORT_NUM                      65535

#define CYGNUM_HFS_IPC_MSG_SIZE                  5*1024  // 5k

#define CYGNUM_HFS_RESERVE_BOUNDARY_BUFF_SIZE    512

#define CYGNUM_HFS_PINPON_BUFF_SIZE     (32*1024)

#define CYGNUM_HFS_BUFF_SIZE            ((CYGNUM_HFS_PINPON_BUFF_SIZE+CYGNUM_HFS_RESERVE_BOUNDARY_BUFF_SIZE)*2)

#define CYGNUM_HFS_SOCK_BUF_SIZE        51200


#if CONFIG_SUPPORT_CGI
#define CYGNUM_HFS_REQUEST_FORM_MAXLEN  1024
#define CYGNUM_HFS_REQUEST_PATH_MAXLEN  1024
#else
#define CYGNUM_HFS_REQUEST_FORM_MAXLEN  1024
#define CYGNUM_HFS_REQUEST_PATH_MAXLEN  1024
#endif

#define CYGNUM_HFS_CGI_ARG_SIZE         10

#define CYGNUM_HFS_CONTENT_TYPE_MAXLEN        64

#define CYGNUM_HFS_REQUEST_BOUNDARY_MAXLEN    64

#define CYGNUM_HFS_UPLOAD_FILENAME_MAXLEN     128

#define CYGNUM_HFS_CONTENT_RANGE_MAXLEN       40

#define CYGNUM_HFS_CONTENT_MD5_MAXLEN         32//16

#define CYGNUM_HFS_REQUEST_FORM_LIST_MAXNUM   6

#define CYGNUM_HFS_SERVER_DATA_THREAD_ENABLE  1

#define CYGNUM_HFS_SUPPORT_UPLOAD             1

#define CYGNUM_HFS_SHOW_SEND_ERROR            0

#define CYGNUM_SELECT_TIMEOUT_USEC            500000

// performance

#define CYGNUM_HFS_PERF_SEND     0

#define CYGNUM_HFS_PERF_SELECT   0

// flag define

#define CYG_FLG_HFS_START                 0x0001
#define CYG_FLG_HFS_IDLE                  0x0002
#define CYG_FLG_HFS_CLOSED                0x0004

#define CYG_FLG_HFS_NOTIFY_CLIENT_ACK     0x0010
#define CYG_FLG_HFS_GET_CUSTOM_ACK        0x0020
#define CYG_FLG_HFS_CHK_PASSWD_ACK        0x0040
#define CYG_FLG_HFS_GEN_DIRLIST_ACK       0x0080
#define CYG_FLG_HFS_CLIENT_QUERY_ACK      0x0100
#define CYG_FLG_HFS_UPLOAD_RESULT_ACK     0x0200
#define CYG_FLG_HFS_DELETE_RESULT_ACK     0x0400
#define CYG_FLG_HFS_PUT_CUSTOM_ACK        0x0800
#define CYG_FLG_HFS_HEADER_CB_ACK         0x1000
#define CYG_FLG_HFS_DOWNLOAD_RESULT_ACK   0x2000


#define HFS_DTHREAD_FLAG_WRITE_FILE_REQ (1<<0)      // request data thread to write file
#define HFS_DTHREAD_FLAG_READ_FILE_REQ  (1<<1)      // request data thread to read file
#define HFS_DTHREAD_FLAG_DONE           (1<<2)      // request data thread done
#define HFS_DTHREAD_FLAG_EXIT           (1<<3)      // request data thread to exit
//#define HFS_DTHREAD_FLAG_IDLE           (1<<4)      // data thread idle
#define HFS_DTHREAD_FLAG_THREAD_END     (1<<5)      // data thread inform itself end


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
#define HTTP_VERSION                "HTTP/1.1"

#define HTTP_STATUS_CODE_OK                 200
#define HTTP_STATUS_CODE_CREATED            201
#define HTTP_STATUS_CODE_PARTIAL_CONTENT    206
#define HTTP_STATUS_CODE_REDIRECT           301
#define HTTP_STATUS_CODE_UNAUTHORIZED       401
#define HTTP_STATUS_CODE_NOT_FOUND          404


#define HTTP_HEADER_END_LEN         4  // \r\n\r\n



#define USER_AGENT_NONE             0
#define USER_AGENT_OTHERS           1
#define USER_AGENT_IE               2
#define USER_AGENT_CHROME           3
#define USER_AGENT_VLC              4
#define USER_AGENT_FIREFOX          5
#define USER_AGENT_AOF              6

#define HFS_LIST_DIR_LEN_ERR        -2
#define HFS_LIST_DIR_OPEN_FAIL      -1
#define HFS_LIST_DIR_OK             0
#define HFS_LIST_DIR_CONTINUE       1
#define HFS_LIST_DIR_SEGMENT_SEND   2


#define HFS_OPEN_FILE_FAIL          -11
#define HFS_SEEK_FILE_FAIL          -12

#define HFS_HTTP_SEPARATE_SEND_ALL      0
#define HFS_HTTP_SEPARATE_SEND_HEADER   1
#define HFS_HTTP_SEPARATE_SEND_BODY     2
#define HFS_HTTP_SEPARATE_SEND_END      3


static const char cyg_hfs_not_found[] =
"<html>\n<head><title>Page Not found</title></head>\n"
"<body><h2>The requested URL was not found on this server.</h2></body>\n</html>\n";

static const char cyg_hfs_access_denied[] =
"<html>\n<head><title>Access denied</title></head>\n"
"<body><h2>The user name or password is invalid.</h2></body>\n</html>\n";
static const char cyg_hfs_base64_char[]= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";



struct hfs_session;
struct hfs_server;

typedef struct
{
    int                 method;
    char                url[CYGNUM_HFS_REQUEST_PATH_MAXLEN+1];
    char                path[CYGNUM_HFS_REQUEST_PATH_MAXLEN+1];
    char                formdata[CYGNUM_HFS_REQUEST_FORM_MAXLEN+1];
    int                 httpVer;
    int                 closeConnect;
    int                 userAgent;
    int64_t             contentLength;
    char                contentType[CYGNUM_HFS_CONTENT_TYPE_MAXLEN+1];
    char                boundary[CYGNUM_HFS_REQUEST_BOUNDARY_MAXLEN+1];
    char*               pre_readdata;
    int                 pre_readdatalen;
    char                contentRange[CYGNUM_HFS_CONTENT_RANGE_MAXLEN+1];
    char                contentMD5[CYGNUM_HFS_CONTENT_MD5_MAXLEN+1];
    int                 HeaderSeq;
} hfs_req_header;


typedef struct
{
    char                *ext;
    char                *mimetype;
} cyg_hfs_file_mimetype;

typedef struct hfs_server
{
    cyg_hfs_notify          *notify;        // notify the status
    cyg_hfs_getCustomData   *getCustomData; // get xml data
    cyg_hfs_check_password  *check_pwd;     // check password callback function
    cyg_hfs_gen_dirlist_html *genDirListHtml;    // generate directory list with html format
    int                     portNum;        // server port number
    int                     isSupportHttps;      ///<  if want to support https
    int                     httpsPortNum;   // https port number
    int                     threadPriority; // server thread priority
    char                    rootdir[CYG_HFS_ROOT_DIR_MAXLEN + 1];   /* root of the visible filesystem */
    char                    key[CYG_HFS_KEY_MAXLEN+1];
    int                     sockbufSize;// socket buffer size
	int                     tos;            // type of service
    u_int32_t               sharedMemAddr;  // shared memory address
    u_int32_t               sharedMemSize;  // shared memory size
    char                    clientQueryStr[CYG_HFS_USER_QUERY_MAXLEN+1]; // client query string
    cyg_hfs_client_query    *clientQueryCb;   // client query callback function
    int                     timeoutCnt;       // timeout counter for send & receive , time base is 0.5sec
    cyg_hfs_upload_result_cb *uploadResultCb;
	cyg_hfs_download_result_cb *downloadResultCb;
    cyg_hfs_delete_result_cb *deleteResultCb;
    cyg_hfs_putCustomData   *putCustomData; // put custom data
    char                    customStr[CYG_HFS_CUSTOM_STR_MAXLEN+1];
    int                     forceCustomCallback;
    cyg_hfs_header_cb       *headerCb;                                   ///<  Callback function for header data parse by customer
    int                     isStart;
    int                     isStop;
    struct addrinfo         address;        // server address
    int                     socket;         // server socket
    u_int8_t                stacks[CYGNUM_HFS_SERVER_STACK_SIZE];
    THREAD_HANDLE           thread;
    THREAD_OBJ              thread_object;
    #if __USE_IPC
    THREAD_HANDLE           rcv_thread;
    THREAD_OBJ              rcv_thread_object;
    u_int8_t                rcv_thread_stacks[CYGNUM_HFS_RCV_THREAD_STACK_SIZE];
    THREAD_HANDLE           close_thread;
    THREAD_OBJ              close_thread_object;
    u_int8_t                close_thread_stacks[CYGNUM_HFS_CLOSE_THREAD_STACK_SIZE];
    #endif
    COND_HANDLE             cond_s;
    MUTEX_HANDLE              mutex;
    MUTEX_HANDLE              condMutex_s;
    MUTEX_HANDLE              mutex_ipc;
    MUTEX_HANDLE              mutex_sharemem[2];
    MUTEX_HANDLE              mutex_customCmd;
    u_int32_t               sharemem_pinponCnt;
    u_int32_t               eventFlag;
    unsigned int            max_nr_of_clients;
    unsigned int            nr_of_clients;
    struct hfs_session      *active_sessions;
    struct serverstruct     *servers;
} hfs_server_t;

typedef struct hfs_session
{
    volatile int   can_delete;
    volatile int   enable;
    struct hfs_session *next;
    struct hfs_server  *pServer;
    int             client_socket;
    struct sockaddr client_address;

    THREAD_OBJ     thread_obj;
    THREAD_HANDLE  thread_hdl;
    char           stack[CYGNUM_HFS_SESSION_THREAD_STACK_SIZE];
    // For data thread
    THREAD_OBJ     threadData_obj;
    THREAD_HANDLE  threadData_hdl;
    COND_HANDLE    condData;
    MUTEX_HANDLE     condMutex;
    u_int32_t      eventFlag;
    int            dataFd;
    char           *pDataBuf;
    size_t         dataLength;
    unsigned char  pStackBuf[CYGNUM_HFS_SESSION_DATA_THREAD_STACK_SIZE];
    ssize_t        hfsdata_sts;
    char           sendBuf[CYGNUM_HFS_BUFF_SIZE];
    DIR            *dirp;
    hfs_req_header reqHeader;
    int            filedesc;
    #ifdef CONFIG_SUPPORT_SSL
    NVTSSL         *ssl;
    u_int8_t       is_ssl;
    int            sslReadSize;
    u_int32_t      sslBufAddr;
    #endif
    #if CONFIG_SUPPORT_CGI
    u_int8_t       is_cgi;
    #if CONFIG_SUPPORT_AUTH
    enum tt__UserLevel userLevel;
    #endif
    int            reqtype;
    char           cgienv[CYGNUM_HFS_CGI_ARG_SIZE][CYGNUM_HFS_REQUEST_PATH_MAXLEN+1];
    #endif
    int            is_custom;
    char           filepath[CYGNUM_HFS_REQUEST_PATH_MAXLEN+1];
    char           argument[CYGNUM_HFS_REQUEST_FORM_MAXLEN+1];
    char           custom_filepath[CYG_HFS_REQUEST_PATH_MAXLEN+1];
    char           custom_mimeType[CYG_HFS_MIMETYPE_MAXLEN+1];
    int            segmentCount;
    int            state;
    int64_t        IdleTime;
    int            pid;
    int            childpid;
	#ifdef CONFIG_SUPPORT_SSL
    NVTMD5_CTX     md5ctx;
	#endif
	int            is_redirect;
	int            needclose;
} hfs_session_t;

struct serverstruct
{
    struct serverstruct *next;
    int sd;
    int is_ssl;
    #ifdef CONFIG_SUPPORT_SSL
    NVTSSL_CTX *ssl_ctx;
    #endif
};

#if CONFIG_SUPPORT_AUTH
#define HFS_CONF_FILE   "/etc/hfs.conf"

struct non_auth_node
{
    char path[CYGNUM_HFS_REQUEST_PATH_MAXLEN+1];
    struct non_auth_node *next;
};

struct non_auth_node *non_auth_head=NULL;

int is_nonauth_path(const char *rootdir, const char *path);
#endif

static hfs_server_t hfs_server = {0} ;
#if __BUILD_PROCESS
static sem_t          main_sem;
#endif
MUTEX_INIT_STATIC(openclose_mtx);

static char* gMimetypeOthers="application/octet-stream";
static int hfs_write(hfs_session_t *session, int fd, char* bufAddr, int bufSize, HFS_PUT_STATUS putStatus);
static int session_init(int sd, struct sockaddr* pClient_address, int is_ssl);
static void session_exit(hfs_session_t *session);


#if __USE_IPC
static void hfs_ipc_sendmsg(int cmd, int Notifystatus);
static int hfs_ipc_getCustomData(char* path, char* argument, HFS_U32 bufAddr, HFS_U32* bufSize, char* mimeType, HFS_U32 segmentCount);
static int hfs_ipc_check_pwd(const char *username, const char *password, char *key,char* questionstr);
static int hfs_ipc_genDirListHtml(const char *path, HFS_U32 bufAddr, HFS_U32* bufSize, const char* usr, const char* pwd, HFS_U32 segmentCount);
static int hfs_ipc_clientQueryCb(char* path, char* argument, HFS_U32 bufAddr, HFS_U32* bufSize, char* mimeType, HFS_U32 segmentCount);
static int hfs_ipc_uploadResultCb(int result, HFS_U32 bufAddr, HFS_U32* bufSize, char* mimeType);
static int hfs_ipc_deleteResultCb(int result, HFS_U32 bufAddr, HFS_U32* bufSize, char* mimeType);
#endif
static void cyg_hfs_release_resource(hfs_server_t *pServer);


static cyg_hfs_file_mimetype gfileMimetype[]=
{
    {".htm","text/html"},
    {".html","text/html"},
    {".xml","text/xml"},
    {".jpg","image/jpeg"},
    {".jpeg","image/jpeg"},
    {".jpe","image/jpeg"},
    {".gif","image/gif"},
    {".png","image/png"},
    {".bmp","image/bmp"},
    {".ico","image/x-icon"},
    {".mpeg","video/mpeg"},
    {".mpg","video/mpeg"},
    {".mpe","video/mpeg"},
    {".avi","video/x-msvideo"},
    {".mov","video/quicktime"},
    {".txt","text/plain"},
    {".css","text/css"},
    #if CONFIG_SUPPORT_CGI
    {".cgi","text/html"},
    //{".pl","text/html"},
    //{".py","text/html"},
    #endif
    {NULL,NULL}
};

#define HFS_URL_VALID_CHAR_MIN   0x20
#define HFS_URL_VALID_CHAR_MAX   0x7E
static const char hfs_url_reserved_char[] ="%#;$+@&={}|^~[] ";
static const char hfs_invalid_filename_char[] ="\\/:?*|<>";

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

static int64_t hfs_get_time(void)
{
    struct timeval tmptv;
    int64_t   ret;

    gettimeofday(&tmptv,NULL);
    ret = ((int64_t)tmptv.tv_sec * 1000000 + tmptv.tv_usec);
    //DBG_DUMP("ret time=%lld\r\n",ret);
    return ret;
}

static int cyg_caseless_compare(const char* name1, const char* name2)
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

static int cyg_caseless_compare_withlen(const char* name1, const char* name2, int len)
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



static void hfs_formdata_parse(char *data, char *list[], int size )
{
    char *p = data;
    int i = 0;

    HFS_DIAG("hfs_formdata_parse  -> %s\n",data);
    list[i] = p;

    while( p && *p != 0 && i < size-1 )
    {
        if( *p == '&' )
        {
            *p++ = 0;
            list[++i] = p;
            continue;
        }
        if( *p == '+' )
            *p = ' ';
        p++;
    }

    list[++i] = 0;
}

static void hfs_content_range_find(char *rangeStr, char **start, char **end)
{
    char *sp = NULL,*ep = NULL, *inStr;

    HFS_DIAG("hfs_content_range_find \n");
    sp = rangeStr;
    inStr = rangeStr;
    while( (*inStr) != 0 )
    {
        if (*inStr == '-')
        {
            *inStr= 0;
            ep = inStr+1;
        }
        inStr++;
    }
    *start = sp;
    *end = ep;

}

/* ----------------------------------------------------------------- */
/* Search for a form response value
 *
 * Search a form response list generated by cyg_formdata_parse() for
 * the named element. If it is found a pointer to the value part is
 * returned. If it is not found a NULL pointer is returned.
 */

static char *hfs_formlist_find( char *list[], char *name )
{
    HFS_DIAG("hfs_formlist_find \n");
    while( *list != 0 )
    {
        char *p = *list;
        char *q = name;

        while( *p == *q )
            p++, q++;

        if( *q == 0 && *p == '=' )
            return p+1;

        list++;
    }
    return 0;
}
static void hfs_path_to_url(char *path, char *url, int urlLen)
{
    int pathLen = strlen(path);
    char *pd = path;
    char *ud = url;

    while (pathLen && urlLen)
    {
        if ( *pd < HFS_URL_VALID_CHAR_MIN || *pd > HFS_URL_VALID_CHAR_MAX || strchr(hfs_url_reserved_char,*pd) )
        {
            if (urlLen < 3)
                break;
            *ud++ = '%';
            snprintf(ud,2,"%x",*pd);
            ud+=2;
            urlLen-=3;
        }
        else
        {
            *ud++ = *pd;
            urlLen--;
        }
        pd++;
        pathLen--;
    }
    if (urlLen)
        *ud = 0;
    else
        *(--ud) = 0;
}


static void hfs_url_to_path(char *url, char *path, int pathLen)
{
    int urlLen = strlen(url);
    char *pd = path;
    char *ud = url;
    char tmp[3]={0};
    int  hexNumber;

    HFS_DIAG("hfs_url_to_path, url=%s\n",url);
    while (pathLen && urlLen)
    {
        if ( *ud == '%' )
        {
            if (urlLen < 3)
                break;
            ud++;
            memcpy((void*)tmp,(void*)ud,2);
            sscanf(tmp, "%x", &hexNumber);
            *pd = hexNumber;
            ud +=2;
            urlLen -=3;
            //HFS_DIAG("0x%x,",*pd);
        }
        else
        {
            *pd = *ud++;
            urlLen--;
        }
        pd++;
        pathLen--;
    }
    if (pathLen)
        *pd = 0;
    else
        *(--pd) = 0;
    HFS_DIAG("hfs_url_to_path, path=%s\n",path);
}
// if find http header end will return the header end address else will return NULL
static char* hfs_chk_request_end(char* buff, int bufflen)
{
    char *p = buff;
    while (bufflen)
    {
        if ( *p == '\r' && (*(p+1)) == '\n' && (*(p+2)) == '\r' && (*(p+3)) == '\n' )
        {
            HFS_DIAG("hfs_chk_request_end -> true \r\n");
            return (p+4);
        }
        p++;
        bufflen--;
    }
    HFS_DIAG("hfs_chk_request_end -> false\n");
    return NULL;
}

static int hfs_check_filename(char *filename)
{
    int pathLen = strlen(filename);
    char *pd = filename;

	if (strncmp(filename, ".", 1) == 0 ) {
		return -1;
	}
	if (strncmp(filename, "..", 2) == 0) {
		return -1;
	}
    while (pathLen)
    {
        if (*pd < HFS_URL_VALID_CHAR_MIN || *pd > HFS_URL_VALID_CHAR_MAX || strchr(hfs_invalid_filename_char,*pd))
        {
			return -1;
        }
        pd++;
        pathLen--;
    }
	return 0;
}

char* hfs_base64_encode(unsigned char *org, unsigned int org_len)
{
	unsigned int  org24bit_value;
	int           has_padd1 = 0;
	int           has_padd2 = 0;
	unsigned int  i, res_bytes;
	char          *res;
	unsigned char *p_org = org;

	if (p_org == NULL) {
		return NULL;
	}
	org24bit_value = org_len / 3;
	if (org_len > org24bit_value * 3) {
		has_padd1 = 1;
	}
	if (org_len == org24bit_value * 3 + 2) {
		has_padd2 = 1;
	}
	res_bytes = 4 * (org24bit_value + has_padd1);
	res = malloc(res_bytes+1);
	if (!res) {
		return NULL;
	}
	for (i = 0; i < org24bit_value; i++) {
		res[i*4+0] = cyg_hfs_base64_char[(p_org[i*3] >> 2) & 0x3F];
		res[i*4+1] = cyg_hfs_base64_char[(((p_org[i*3] & 0x3) << 4) | (p_org[i*3 + 1] >> 4)) & 0x3F];
		res[i*4+2] = cyg_hfs_base64_char[((p_org[i*3 + 1] << 2) | (p_org[i*3 + 2] >> 6)) & 0x3F];
		res[i*4+3] = cyg_hfs_base64_char[p_org[i*3 + 2] & 0x3F];
	}
	if (has_padd1) {
		res[i*4+0] = cyg_hfs_base64_char[(p_org[i*3] >> 2) & 0x3F];
		if (has_padd2) {
			res[i*4+1] = cyg_hfs_base64_char[(((p_org[i*3] & 0x3) << 4) | (p_org[i*3+1]>>4)) & 0x3F];
			res[i*4+2] = cyg_hfs_base64_char[(p_org[i*3+1] << 2) & 0x3F];
		} else {
			res[i*4+1] = cyg_hfs_base64_char[((p_org[i*3] & 0x3) << 4) & 0x3F];
			res[i*4+2] = '=';
		}
		res[i*4+3] = '=';
	}
	res[res_bytes] = '\0';
	return res;
}

/*
GET /tools.html HTTP/1.1
Host: www.joes-hardware.com
Accept: text/html, image/gif, image/jpeg
Connection: close

*/
/* return true means the header has end */
static int hfs_parse_header( char* buff, int bufflen, hfs_req_header* req_hdr, const char* rootdir)
{
    char *p,*str, *value,*path;
    int  len, ret = false;
    char *formdata = NULL;
    char keyTempstr[30];
    int  haskey;
    int  breakCnt;
    char url[CYGNUM_HFS_REQUEST_PATH_MAXLEN+1];
    int  hasContentRange = false;


    HFS_DIAG("buffAddr= 0x%x, bufflen= %d, last 5 bytes=%d,%d,%d,%d,%d\r\n",(int)buff,bufflen,buff[bufflen-5],buff[bufflen-4],buff[bufflen-3],buff[bufflen-2],buff[bufflen-1]);
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
        if( cyg_caseless_compare( str, "GET" ) )
        {
            req_hdr->method = HTTP_REQUEST_METHOD_GET;
        }
        else if( cyg_caseless_compare( str, "PUT" ) )
        {
            req_hdr->method = HTTP_REQUEST_METHOD_PUT;
        }
        else if( cyg_caseless_compare( str, "POST" ) )
        {
            req_hdr->method = HTTP_REQUEST_METHOD_POST;
        }
        else if( cyg_caseless_compare( str, "DELETE" ) )
        {
            req_hdr->method = HTTP_REQUEST_METHOD_DELETE;
        }
        else if( cyg_caseless_compare( str, "HEAD" ) )
        {
            req_hdr->method = HTTP_REQUEST_METHOD_HEAD;
        }
        else
        {
            HFS_DIAG("Connection method %s not support\n",str);
            //return false;
        }
        HFS_DIAG("method= %s\r\n",str);
        // parsing path
        path = p+1;
        while( (*p != ' '&& *p != '?') && bufflen)
        {
            p++;
            bufflen--;
        }
        // set formdata
        if( *p == '?' )
            formdata = p+1;
        *p = 0;
        // if path last character is '/', remove it
        if (*(p-1) == '/')
            *(p-1) = 0;

        hfs_url_to_path(path,req_hdr->url,CYGNUM_HFS_REQUEST_PATH_MAXLEN);
        #if (CONFIG_SUPPORT_CGI)
		len = snprintf(url, CYGNUM_HFS_REQUEST_PATH_MAXLEN, "%s%s",rootdir,path);
        //DBG_DUMP("url %s rootdir %s\r\n",url,rootdir);
        if (strcmp(rootdir,url)==0 && (strstr(rootdir, "www")!=NULL))
        {
            DBG_IND("url is rootdir %s\r\n",url);
            snprintf(&url[len], CYGNUM_HFS_REQUEST_PATH_MAXLEN-len, "/index.html");
            DBG_IND("url %s\r\n",url);
        }
		#else
		snprintf(url, CYGNUM_HFS_REQUEST_PATH_MAXLEN, "%s%s",rootdir,path);
        #endif
        memcpy(req_hdr->path, url, sizeof(req_hdr->path));
        if( formdata != NULL )
        {
            while( *p != ' ' && bufflen)
            {
                p++;
                bufflen--;
            }
            *p = 0;
            hfs_url_to_path(formdata,req_hdr->formdata,CYGNUM_HFS_REQUEST_FORM_MAXLEN );
            len = strlen(req_hdr->url);
            snprintf(&req_hdr->url[len], CYGNUM_HFS_REQUEST_PATH_MAXLEN-len, "?%s",req_hdr->formdata);
            HFS_DIAG("Has formdata\r\n");
        }
        HFS_DIAG("path= %s, url=%s\r\n",req_hdr->path,req_hdr->url);
        // parse http version
        str = p+1;
        while( *p != '\r' && *p != '\n' && bufflen)
        {
            p++;
            bufflen--;
        }
        *p = 0;
        if (cyg_caseless_compare ("HTTP/1.1",str))
        {
            req_hdr->httpVer = HTTP_VERSION_1_1;
            // connection default set to not close
            req_hdr->closeConnect = false;
        }
        else if (cyg_caseless_compare ("HTTP/1.0",str))
        {
            req_hdr->httpVer = HTTP_VERSION_1_0;
            // connection default set to close
            req_hdr->closeConnect = true;
        }
        else
        {
            req_hdr->httpVer = HTTP_VERSION_INVALID;
            HFS_DIAG("HTTP version invalid\n");
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
            req_hdr->pre_readdata = p;
            req_hdr->pre_readdatalen = bufflen;
            HFS_DIAG("find request header end, remain bufflen=%d\r\n",bufflen);
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
            HFS_DIAG("str=%s\r\n",str);

            len = snprintf(keyTempstr,sizeof(keyTempstr),"User-Agent:");
            if (cyg_caseless_compare_withlen(keyTempstr,str,len))
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
            len = snprintf(keyTempstr,sizeof(keyTempstr),"Range:");
            if (cyg_caseless_compare_withlen(keyTempstr,str,len))
            {
                HFS_DIAG("has range 1\r\n");
                hasContentRange = true;
            }
            if (!hasContentRange)
            {
                len = snprintf(keyTempstr,sizeof(keyTempstr),"Content-Range:");
                if (cyg_caseless_compare_withlen(keyTempstr,str,len))
                {
                    HFS_DIAG("has range 2\r\n");
                    hasContentRange = true;
                }
            }
            if (hasContentRange)
            {
                char *contentRange;
                value = str+len;
                if ( (contentRange = strstr(value, "bytes=")) || (contentRange = strstr(value, "bytes ")))
                {
                    my_strncpy(req_hdr->contentRange,contentRange+strlen("bytes="),sizeof(req_hdr->contentRange));
                    HFS_DIAG("req_hdr->contentRange=%s\r\n",req_hdr->contentRange);
                }
                #if 0
                else if ( contentRange = strstr(value, "bytes "))
                {
                    my_strncpy(req_hdr->contentRange,contentRange+strlen("bytes "),CYGNUM_HFS_CONTENT_RANGE_MAXLEN);
                    HFS_DIAG("req_hdr->contentRange=%s\r\n",req_hdr->contentRange);
                }
                #endif
                *p = c;
                hasContentRange = false;
                continue;
            }
            #if CYGNUM_HFS_SUPPORT_UPLOAD
            len = snprintf(keyTempstr,sizeof(keyTempstr),"Content-Length:");
            if (cyg_caseless_compare_withlen(keyTempstr,str,len))
            {
                int64_t  filelength = 0, i=0;
                value = str+len;
                // skip space
                if (*value==' ')
                    value++;

				//while(isdigit(value[i]))
                while((value[i]) >= '0' && (value[i]) <= '9')
                {
                    if (i!=0)
                        filelength*=10;
                    filelength += (value[i]-0x30);
                    i++;
                }
                req_hdr->contentLength = filelength;
                HFS_DIAG("req_hdr->contentLength=%lld\r\n",req_hdr->contentLength);
                *p = c;
                continue;
            }
            len = snprintf(keyTempstr,sizeof(keyTempstr),"Content-Type:");
            if (cyg_caseless_compare_withlen(keyTempstr,str,len))
            {
                char *boundary;
                value = str+len;
                // skip space
                if (*value==' ')
                    value++;

                my_strncpy(req_hdr->contentType,value,sizeof(req_hdr->contentType));
                HFS_DIAG("req_hdr->contentType=%s\r\n",req_hdr->contentType);
                if ((boundary = strstr(value, "boundary=")))
                {
                    snprintf(req_hdr->boundary,sizeof(req_hdr->boundary),"--%s",boundary+9);
                    HFS_DIAG("req_hdr->boundary=%s\r\n",req_hdr->boundary);
                }
                *p = c;
                continue;
            }
            len = snprintf(keyTempstr,sizeof(keyTempstr),"Content-MD5:");
            if (cyg_caseless_compare_withlen(keyTempstr,str,len))
            {
                value = str+len;
                // skip space
                if (*value==' ')
                    value++;
                my_strncpy(req_hdr->contentMD5,value,sizeof(req_hdr->contentMD5));
                HFS_DIAG("req_hdr->contentMD5=%s\r\n",req_hdr->contentMD5);
                *p = c;
                continue;
            }
            #endif
            *p = c;
        }
    }
parse_header_end:
    if (ret == true)
        req_hdr->HeaderSeq = 0;
    else
        req_hdr->HeaderSeq++;
    HFS_DIAG("method=%d, path=%s, httpVer = %d, CloseConn=%d, userAgent=%d,HeaderSeq =%d \r\n",req_hdr->method,req_hdr->path,req_hdr->httpVer,req_hdr->closeConnect,req_hdr->userAgent,req_hdr->HeaderSeq);
    if (formdata)
        HFS_DIAG("formdata->%s\r\n",formdata);

    HFS_DIAG("parse_request end\r\n");
    return ret;
}

static int recvall(hfs_session_t* session, char* buf, int len, int isJustOne)
{
    int total = 0;
    int bytesleft = len;
    int n = -1;
    int ret;
    int idletime;
    hfs_server_t *pServer = &hfs_server;
    //int ssl_try_again = 0;

    HFS_SEND_DIAG("recvall buf = 0x%x, len = %d bytes\n",(int)buf,len);
    while (total < len && (!pServer->isStop))
    {
        #ifdef CONFIG_SUPPORT_SSL
        if (session->is_ssl)
        {
            //uint8_t *read_buf;
            //ssl_try_again = 0;

            // have already read data in buffer
            HFS_SEND_DIAG("sslReadSize %d ,total=%d, bytesleft=%d\n",session->sslReadSize,total,bytesleft);
			#if 0
            if (session->sslReadSize)
            {
               if (session->sslReadSize > bytesleft)
               {
                   n = bytesleft;
                   HFS_SEND_DIAG("1 sslBufAddr = 0x%x, n=%d\n",(int)(session->sslBufAddr),n);
                   memcpy(buf+total, (uint8_t *)session->sslBufAddr, n);
                   session->sslReadSize-=n;
                   session->sslBufAddr+=n;
                   return n;
               }
               else
               {
                   n = session->sslReadSize;
                   HFS_SEND_DIAG("2 sslBufAddr = 0x%x, n=%d\n",(int)(session->sslBufAddr),n);
                   memcpy(buf+total, (uint8_t *)session->sslBufAddr, n);
                   session->sslReadSize=0;
                   bytesleft-=n;
                   total+=n;
                   if (bytesleft ==0)
                       return total;
               }
            }
            //MUTEX_WAIT(pServer->mutex);
            if (bytesleft && (n = nvtssl_recv(session->ssl, &read_buf, bytesleft)) > SSL_OK)
            {
                HFS_SEND_DIAG("ssl_read n=%d, bytesleft=%d \n",n,bytesleft);
                if (n > bytesleft)
                {
                    session->sslReadSize = n - bytesleft;
                    n = bytesleft;
                    session->sslBufAddr = (uint32_t)read_buf + n;
                    HFS_SEND_DIAG("1 read_buf = 0x%x, sslBufAddr=0x%x, n=%d\n",(int)(read_buf),session->sslBufAddr,n);
                    memcpy(buf+total, read_buf, n );
                }
                else
                {
                    HFS_SEND_DIAG("2 read_buf = 0x%x, n=%d\n",(int)(read_buf),n);
                    memcpy(buf+total, read_buf, n );
                }
            }
			#else
            n = nvtssl_recv(session->ssl, buf+total, bytesleft, 0);
			#endif
        }
        else
        #endif
        {
            n = recv(session->client_socket, buf+total, bytesleft, 0);
        }
		if ( n == 0)
        {
            // client close the socket
            HFS_DIAG("Connection closed by client \n");
			session->needclose = 1;
            return 0;
        }
        if ( (n < 0 && (errno == EAGAIN)))
        {
            idletime = 0;
            while(!pServer->isStop)
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
                    HFS_SEND_DIAG("idletime=%d\r\n",idletime);
                    if (idletime >= pServer->timeoutCnt)
                    {
                        perror("select\r\n");
						session->needclose = 1;
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
					session->needclose = 1;
                    return -1;
                }
                if (FD_ISSET( session->client_socket, &rs ))
                {
                    //HFS_SEND_DIAG("recvall FD_ISSET\r\n");
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
    HFS_SEND_DIAG("recvall end, total=%d\n",total);
    return n<0?-1:total;
}


static int sendall(hfs_session_t* session, char* buf, int *len)
{
    int total = 0;
    int bytesleft = *len;
    int n = -1;
    int ret;
    int idletime;
    hfs_server_t *pServer = &hfs_server;
    //int ssl_try_again = 0;

    HFS_SEND_DIAG("sendall buf = 0x%x, len = %d bytes\n",(int)buf,*len);
    while (total < *len && (!pServer->isStop))
    {
        #ifdef CONFIG_SUPPORT_SSL
        if (session->is_ssl)
        {
            //ssl_try_again = 0;
            //HFS_SEND_DIAG("ssl_write bytesleft =%d\n",bytesleft);
            n = nvtssl_send(session->ssl, (buf+total), bytesleft, 0);
            //HFS_SEND_DIAG("ssl_write n =%d\n",n);
        }
        else
        #endif
        {
            n = send(session->client_socket, buf+total, bytesleft, 0);
        }
		if ( n == 0)
        {
            // client close the socket
            HFS_DIAG("Connection closed by client \n");
			session->needclose = 1;
            return 0;
        }
        if ( (n < 0 && (errno == EAGAIN)))
        {
            idletime = 0;
            while(!pServer->isStop)
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
                    HFS_SEND_DIAG("idletime=%d\r\n",idletime);
                    if (idletime >= pServer->timeoutCnt)
                    {
                        perror("select\r\n");
						session->needclose = 1;
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
					session->needclose = 1;
                    return -1;
                }
                if (FD_ISSET( session->client_socket, &ws ))
                {
                    //HFS_SEND_DIAG("sendall FD_ISSET\r\n");
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
    HFS_SEND_DIAG("sendall end\n");
    return n<0?-1:total;
}

#if CYGNUM_HFS_SERVER_DATA_THREAD_ENABLE
THREAD_DECLARE(hfs_data_thread,data)
{
    hfs_session_t *session = (hfs_session_t *) data;

    FLAG_SETPTN((session->condData),session->eventFlag,HFS_DTHREAD_FLAG_DONE,session->condMutex);
    while (1)
    {
        HFS_FLAG_DIAG("hfs_data_thread WaitSem, threadData_hdl= 0x%x\n",(unsigned int)session->threadData_hdl);
        MUTEX_WAIT(session->condMutex);
        HFS_FLAG_DIAG("hfs_data_thread ExitSem, threadData_hdl= 0x%x\n",(unsigned int)session->threadData_hdl);
        if (!(session->eventFlag & HFS_DTHREAD_FLAG_WRITE_FILE_REQ ||
            session->eventFlag & HFS_DTHREAD_FLAG_READ_FILE_REQ ||
            session->eventFlag & HFS_DTHREAD_FLAG_EXIT))
        {
        	  HFS_FLAG_DIAG("hfs_data_thread WaitCond, session->eventFlag = 0x%x, threadData_hdl= 0x%x\n",session->eventFlag,(unsigned int)session->threadData_hdl);
            COND_WAIT((session->condData),(session->condMutex));
        }
        HFS_FLAG_DIAG("hfs_data_thread 1 session->eventFlag = 0x%x, threadData_hdl= 0x%x\n",session->eventFlag,(unsigned int)session->threadData_hdl);
        if (session->eventFlag & HFS_DTHREAD_FLAG_EXIT)
        {
            session->eventFlag ^= HFS_DTHREAD_FLAG_EXIT;
            break;
        }
        if (session->eventFlag & HFS_DTHREAD_FLAG_WRITE_FILE_REQ)
        {
            HFS_RW_DIAG("DTHREAD_WRITE begin\n");
            session->eventFlag ^= HFS_DTHREAD_FLAG_WRITE_FILE_REQ;
            session->hfsdata_sts = hfs_write(session, session->dataFd, session->pDataBuf, session->dataLength, HFS_PUT_STATUS_CONTINUE);
            HFS_RW_DIAG("DTHREAD_WRITE end\n");
        }
        if (session->eventFlag & HFS_DTHREAD_FLAG_READ_FILE_REQ)
        {
            HFS_RW_DIAG("DTHREAD_READ begin\n");
            session->eventFlag ^= HFS_DTHREAD_FLAG_READ_FILE_REQ;
            #if 1
            session->hfsdata_sts = read(session->dataFd, session->pDataBuf, session->dataLength);
            #else
            {
                fd_set readfd;
                struct timeval timeout;
                int rv;
                int    idletime = 0;

            again:
                FD_ZERO(&readfd);
                FD_SET(session->dataFd, &readfd);
                timeout.tv_sec = 0;
                timeout.tv_usec = CYGNUM_SELECT_TIMEOUT_USEC;
                session->state = 11;
                rv = select(session->dataFd + 1, &readfd, NULL, NULL, &timeout);
                if(rv < 0)
                {
                    /* just an interrupted system call */
                    if (errno == EINTR || errno == EAGAIN)
                    {
                        session->state = 3;
                        //DBG_ERR("EINTR pid=%d\r\n",session->pid);
                        goto again;
                    }
                    /* handle other errors */
                    DBG_ERR("select %d\r\n",errno);
                    session->hfsdata_sts = -1;
                }
                else if(rv == 0)
                {
                    session->state = 4;
                    //DBG_ERR("timeout, pid=%d\r\n",session->pid); /* a timeout occured */
                    idletime++;
                    HFS_SEND_DIAG("idletime=%d\r\n",idletime);
                    if (idletime >= session->pServer->timeoutCnt)
                    {
                        DBG_ERR("select timeout\r\n");
                        session->hfsdata_sts = -1;
                    }
                    else
                        goto again;
                }
                else
                {
                    if (FD_ISSET( session->dataFd, &readfd ))
                    {
                        //idletime = 0;
                        session->state = 1;
                        session->hfsdata_sts = read(session->dataFd, session->pDataBuf, session->dataLength); /* there was data to read */
                        session->state = 2;
                    }
                    else
                    {
                        //DBG_ERR("no FD set\r\n");
                        goto again;
                    }
                }
            }
            #endif
            HFS_RW_DIAG("DTHREAD_READ end\n");
        }
        MUTEX_SIGNAL(session->condMutex);
        FLAG_SETPTN((session->condData),session->eventFlag,HFS_DTHREAD_FLAG_DONE,session->condMutex);
        HFS_FLAG_DIAG("hfs_data_thread 2 session->eventFlag = 0x%x\n",session->eventFlag);
    }
    HFS_FLAG_DIAG("hfs_data_thread end , threadData_hdl= 0x%x\r\n", (unsigned int)session->threadData_hdl);
    session->eventFlag|=HFS_DTHREAD_FLAG_DONE|HFS_DTHREAD_FLAG_THREAD_END;
    HFS_FLAG_DIAG("hfs_data_thread 3 session->eventFlag = 0x%x, threadData_hdl= 0x%x\n",session->eventFlag,(unsigned int)session->threadData_hdl);
    COND_SIGNAL(session->condData);
    MUTEX_SIGNAL(session->condMutex);
    THREAD_RETURN(session->threadData_hdl);
}

static void hfs_data_trigFileWrite(hfs_session_t* session, char *pBuf, size_t length, int fd)
{
    HFS_RW_DIAG("hfs_data_trigFileWrite ---- pBuf = 0x%x, length = %d\n",(int)pBuf,(int)length);
    session->pDataBuf = pBuf;
    session->dataLength = length;
    session->dataFd = fd;
    session->hfsdata_sts = 0;

    FLAG_WAITPTN((session->condData),session->eventFlag,HFS_DTHREAD_FLAG_DONE,(session->condMutex),true);
    FLAG_SETPTN((session->condData),session->eventFlag,HFS_DTHREAD_FLAG_WRITE_FILE_REQ,(session->condMutex));
    HFS_RW_DIAG("hfs_data_trigFileWrite ---- End, session->eventFlag = 0x%x\n",session->eventFlag);
}
static void hfs_data_trigFileRead(hfs_session_t* session, char *pBuf, size_t length, int fd)
{
    HFS_RW_DIAG("hfs_data_trigFileRead ---- pBuf = 0x%x, length = %d\n",(int)pBuf,(int)length);
    session->pDataBuf = pBuf;
    session->dataLength = length;
    session->dataFd = fd;
    session->hfsdata_sts = 0;
    FLAG_WAITPTN((session->condData),session->eventFlag,HFS_DTHREAD_FLAG_DONE,(session->condMutex),true);
    FLAG_SETPTN((session->condData),session->eventFlag,HFS_DTHREAD_FLAG_READ_FILE_REQ,(session->condMutex));
    HFS_RW_DIAG("hfs_data_trigFileRead ---- End, session->eventFlag = 0x%x\n",session->eventFlag);
}

static ssize_t hfs_data_waitDone(hfs_session_t* session)
{
    HFS_RW_DIAG("hfs_data_waitDone ---- ");
    FLAG_WAITPTN((session->condData),session->eventFlag,HFS_DTHREAD_FLAG_DONE,(session->condMutex),false);
    HFS_RW_DIAG("  sts = %d \n",(int)session->hfsdata_sts);
    return session->hfsdata_sts;
}


static void hfs_data_open(hfs_session_t* session)
{
	session->hfsdata_sts = 0;
    FLAG_CLRPTN(session->eventFlag,0xFFFFFFFF,session->condMutex);
    THREAD_CREATE(
            session->pServer->threadPriority-1,
            "hfs data thread",
            session->threadData_hdl,
            hfs_data_thread,
            session,
            (void *)session->pStackBuf,
            CYGNUM_HFS_SESSION_DATA_THREAD_STACK_SIZE,
            &session->threadData_obj );
    THREAD_RESUME( session->threadData_hdl );
    HFS_DIAG("hfs_data_open session =0x%x, threadData_hdl=0x%x\n",(unsigned int)session,(unsigned int)session->threadData_hdl);
}

static void hfs_data_close(hfs_session_t* session)
{
	HFS_DIAG("hfs_data_close ---- begin ,session =0x%x,  threadData_hdl=0x%x\n",(unsigned int)session,(unsigned int)session->threadData_hdl);
    FLAG_SETPTN((session->condData),session->eventFlag,HFS_DTHREAD_FLAG_EXIT,(session->condMutex));
    FLAG_WAITPTN((session->condData),session->eventFlag,HFS_DTHREAD_FLAG_THREAD_END,(session->condMutex),true);
    THREAD_DESTROY(session->threadData_hdl);
    HFS_DIAG("hfs_data_close ---- end,session =0x%x,  threadData_hdl=0x%x\n",(unsigned int)session,(unsigned int)session->threadData_hdl);
}
#endif


static int hfs_http_start( int statusCode, char *buff, int bufsize, char *content_type,
                               int64_t content_length, char* contentRange, int close_conn, char* filename)
{
    int  len, totallen = 0, remain_bufsize = bufsize;

    if (statusCode == HTTP_STATUS_CODE_OK)
        len = snprintf(buff,remain_bufsize,"%s 200 OK\r\nServer: %s\r\n",HTTP_VERSION,CYGDAT_HFS_SERVER_ID);
    else if (statusCode == HTTP_STATUS_CODE_CREATED)
        len = snprintf(buff,remain_bufsize,"%s 201 Created\r\nServer: %s\r\n",HTTP_VERSION,CYGDAT_HFS_SERVER_ID);
    else if (statusCode == HTTP_STATUS_CODE_PARTIAL_CONTENT)
        len = snprintf(buff,remain_bufsize,"%s 206 Partial Content\r\nServer: %s\r\n",HTTP_VERSION,CYGDAT_HFS_SERVER_ID);
	else if (statusCode == HTTP_STATUS_CODE_REDIRECT)
        len = snprintf(buff,remain_bufsize,"%s 301 OK\r\nServer: %s\r\n",HTTP_VERSION,CYGDAT_HFS_SERVER_ID);
	else if (statusCode == HTTP_STATUS_CODE_UNAUTHORIZED)
        len = snprintf(buff,remain_bufsize,"%s 401 Unauthorized\r\nServer: %s\r\n",HTTP_VERSION,CYGDAT_HFS_SERVER_ID);
	else if (statusCode == HTTP_STATUS_CODE_NOT_FOUND)
        len = snprintf(buff,remain_bufsize,"%s 404 Not Found\r\nServer: %s\r\n",HTTP_VERSION,CYGDAT_HFS_SERVER_ID);
    else {
        len = snprintf(buff,remain_bufsize,"%s 400 Bad Request\r\nServer: %s\r\n",HTTP_VERSION,CYGDAT_HFS_SERVER_ID);
    }
	if (len < 0 || len > remain_bufsize)
	    goto http_len_err;
    totallen+=len;
	remain_bufsize-=len;
    if (statusCode == HTTP_STATUS_CODE_OK)
    {
        len = snprintf(&buff[totallen],remain_bufsize, "Cache-Control: no-store, no-cache, must-revalidate\r\nPragma: no-cache\r\n");
		if (len < 0 || len > remain_bufsize)
	    	goto http_len_err;
        totallen+=len;
        remain_bufsize-=len;
    }
    len = snprintf( &buff[totallen],remain_bufsize, "Accept-Ranges: bytes\r\n");
    if (len < 0 || len > remain_bufsize)
	    	goto http_len_err;
    totallen+=len;
    remain_bufsize-=len;
    if( content_length != 0 )
    {
        len = snprintf( &buff[totallen],remain_bufsize, "Content-Length: %lld\r\n", content_length );
        if (len < 0 || len > remain_bufsize)
	    	goto http_len_err;
        totallen+=len;
        remain_bufsize-=len;
    }
    if( contentRange != 0 )
    {
        len = snprintf( &buff[totallen],remain_bufsize, "Content-Range: bytes %s\r\n", contentRange );
        if (len < 0 || len > remain_bufsize)
	    	goto http_len_err;
        totallen+=len;
        remain_bufsize-=len;
    }
    if( content_type != NULL )
    {
        len = snprintf( &buff[totallen],remain_bufsize,"Content-Type: %s\r\n", content_type );
        if (len < 0 || len > remain_bufsize)
	    	goto http_len_err;
        totallen+=len;
        remain_bufsize-=len;
    }
    if (close_conn)
        len = snprintf( &buff[totallen], remain_bufsize,"Connection: close\r\n");
    else
        len = snprintf( &buff[totallen], remain_bufsize,"Connection: keep-alive\r\n");
    if (len < 0 || len > remain_bufsize)
	    	goto http_len_err;
        totallen+=len;
        remain_bufsize-=len;
    len = snprintf(&buff[totallen], remain_bufsize,"\r\n");
    if (len < 0 || len > remain_bufsize)
	    	goto http_len_err;
    totallen+=len;
    remain_bufsize-=len;
    HFS_DIAG("http resonse -> \n\n%s\n",buff);
    return totallen;
http_len_err:
    DBG_ERR("totallen %d >= bufsize %d\r\n",totallen,bufsize);
    return 0;
}



static int hfs_http_combine_send(hfs_session_t* session, int statusCode,int client,char* content_type, const char* data, int dataSize, char* sendBuff)
{
    int   headerlen = 0, len = 0, writeBytes;

    headerlen = hfs_http_start( statusCode, sendBuff, CYGNUM_HFS_PINPON_BUFF_SIZE, content_type, dataSize, NULL, true, NULL);
    len += headerlen;
    memcpy((char*)((u_int32_t)sendBuff+len),data,dataSize);
    len += dataSize;
    writeBytes = len;
    if (sendall(session,sendBuff,&writeBytes) <=0)
    {
        #if CYGNUM_HFS_SHOW_SEND_ERROR
        perror("sendall");
        printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",len,writeBytes,client);
        #endif
        return false;
    }
    return true;
}



static int hfs_http_separate_send(hfs_session_t* session, int client, char* content_type, char* data, int dataSize, int httpSendPart)
{
    int    len, writeBytes;
    char   headbuf[400];

    HFS_DIAG("hfs_http_separate_send part= %d\r\n", httpSendPart);
    if (httpSendPart == HFS_HTTP_SEPARATE_SEND_ALL || httpSendPart == HFS_HTTP_SEPARATE_SEND_HEADER)
    {
        // send http start
        len = hfs_http_start(HTTP_STATUS_CODE_OK, headbuf, sizeof(headbuf), content_type, dataSize, NULL, true, NULL);
        writeBytes = len;
        if (sendall(session,headbuf,&writeBytes) <=0)
        {
            #if CYGNUM_HFS_SHOW_SEND_ERROR
            perror("sendall");
            printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",len,writeBytes,client);
            #endif
            return false;
        }
    }
    if (httpSendPart == HFS_HTTP_SEPARATE_SEND_ALL || httpSendPart == HFS_HTTP_SEPARATE_SEND_BODY)
    {
        // send data body
        len = dataSize;
        writeBytes = len;
        if (sendall(session,data,&writeBytes) <=0)
        {
            #if CYGNUM_HFS_SHOW_SEND_ERROR
            perror("sendall");
            printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",len,writeBytes,client);
            #endif
            return false;
        }
    }
    return true;
}

static int hfs_send_error( hfs_session_t* session, int client,char* sendBuff, const char* err)
{
	int status_code = HTTP_STATUS_CODE_OK;
    if (err == cyg_hfs_not_found)
		status_code = HTTP_STATUS_CODE_NOT_FOUND;
	else if (err == cyg_hfs_access_denied)
		status_code = HTTP_STATUS_CODE_UNAUTHORIZED;
	return hfs_http_combine_send(session, status_code, client, "text/html", err, strlen(err), sendBuff);

}

#if CYGNUM_HFS_SUPPORT_UPLOAD
static int hfs_send_upload_ok( hfs_session_t* session, int client,char* sendBuff, const char* str)
{
    return hfs_http_combine_send(session, HTTP_STATUS_CODE_CREATED, client, "text/plain", str, strlen(str), sendBuff);
}

static int hfs_send_upload_fail( hfs_session_t* session, int client,char* sendBuff, const char* str)
{
    return hfs_http_combine_send(session, HTTP_STATUS_CODE_OK, client, "text/plain", str, strlen(str), sendBuff);
}

#endif

static int hfs_send_delfile_ok( hfs_session_t* session, int client,char* sendBuff, const char* str)
{
    return hfs_http_combine_send(session, HTTP_STATUS_CODE_OK, client, "text/plain", str, strlen(str), sendBuff);
}


static char* hfs_get_filename(const char *filepath)
{
    int len;
    const char* p;

    len = strlen(filepath);
    p = filepath+len;
    while (len)
    {
        p--;
        if (*p == '/')
            return (char*)p;
        len--;
    }
    return (char*)p;
}



static char* hfs_get_mimetype(const char *filename)
{
    int    i;
    char   tempName[CYGNUM_HFS_REQUEST_PATH_MAXLEN+1];
    cyg_hfs_file_mimetype* pfileMimeType = &gfileMimetype[0];

    for (i=0;i<CYGNUM_HFS_REQUEST_PATH_MAXLEN;i++)
    {
        tempName[i]=filename[i];
        if (tempName[i] >= 'A' && tempName[i]<='Z')
        {
            tempName[i]+=0x20;
        }
    }
    while (pfileMimeType->mimetype)
    {
        if (strstr(tempName, pfileMimeType->ext) !=0)
        {
            return pfileMimeType->mimetype;
        }
        pfileMimeType++;
    }
    return gMimetypeOthers;
}

#if CYGNUM_HFS_SERVER_DATA_THREAD_ENABLE
static void hfs_get_buffer(hfs_session_t *session, int pinponCnt, char** sendbuf, char** rwbuf)
{
    if (pinponCnt %2 == 0)
    {
        *sendbuf = session->sendBuf;
        *rwbuf = session->sendBuf + CYGNUM_HFS_PINPON_BUFF_SIZE + CYGNUM_HFS_RESERVE_BOUNDARY_BUFF_SIZE;
    }
    else
    {
        *rwbuf = session->sendBuf;
        *sendbuf = session->sendBuf + CYGNUM_HFS_PINPON_BUFF_SIZE + CYGNUM_HFS_RESERVE_BOUNDARY_BUFF_SIZE;
    }
}
#endif

#if __USE_IPC
static u_int32_t hfs_get_ipc_sharemem(void)
{
    hfs_server_t *pServer = &hfs_server;
    int          pinponCnt = 0;
    u_int32_t    rtnAddr;

    #if 0
    MUTEX_WAIT(pServer->mutex_ipc);
    pinponCnt = pServer->sharemem_pinponCnt;
    if (pinponCnt == 0)
    {
        pServer->sharemem_pinponCnt = 1;
        rtnAddr = pServer->sharedMemAddr;
    }
    else
    {
        pServer->sharemem_pinponCnt = 0;
        rtnAddr = pServer->sharedMemAddr + (pServer->sharedMemSize >> 1);
    }
    MUTEX_SIGNAL(pServer->mutex_ipc);
    MUTEX_WAIT(pServer->mutex_sharemem[pinponCnt]);
    #else
    MUTEX_WAIT(pServer->mutex_sharemem[pinponCnt]);
    rtnAddr = pServer->sharedMemAddr;
    #endif
    return rtnAddr;
}

static void hfs_release_ipc_sharemem(u_int32_t addr)
{
    hfs_server_t *pServer = &hfs_server;

    if (addr == pServer->sharedMemAddr)
        MUTEX_SIGNAL(pServer->mutex_sharemem[0]);
    else
        MUTEX_SIGNAL(pServer->mutex_sharemem[1]);
}
#endif

#if CYGNUM_HFS_SUPPORT_UPLOAD
static char* hfs_get_boundary(char *data, int64_t datalen, const char* boundary, int64_t boundarylen)
{
    char *pdata;
    const char *pb;
    int64_t  matchlen;

    pdata = data;
    pb = boundary;
    matchlen = 0;
    //HFS_DIAG("boundarylen = %lld\r\n",boundarylen);
    while (datalen)
    {
        if (*pdata == *pb)
        {
            matchlen++;
            //if (matchlen >=10)
                //HFS_DIAG("matchlen = %lld, pdata=%d, pb= %d\r\n",matchlen,*pdata,*pb);
            if (matchlen == boundarylen)
            {
                HFS_DIAG("find boundary\r\n");
                return (pdata - boundarylen+1);
            }
            pb++;
        }
        else if(matchlen)
        {
            matchlen = 0;
            pb = boundary;
        }
        pdata++;
        datalen--;
    }
    return NULL;
}

#if 0
static int hfs_skip_breakline(char *data, int datalen)
{
    int skiplen = 0;
    while( (*data == '\r' || *data == '\n') && datalen)
    {
        data++;
        datalen--;
        skiplen++;
    }
    return skiplen;
}
#endif

static int hfs_check_password(hfs_server_t *pServer, int client, char* filepath, char* usr, char* pwd, char* formdata)
{
    if (pServer->check_pwd)
    {
        int  accepted = false;
        if (usr && pwd)
        {
            HFS_DIAG("user = %s, password = %s\n",usr, pwd);
            if (pServer->check_pwd(usr,pwd, pServer->key, formdata))
                accepted = true;
        }
        if (!accepted)
        {
            return false;
        }
    }
    return true;
}

// filepath example -> filename="IMG_2832.jpg"
// filepath example -> filename="D:\temp\IMG_2832.jpg"
static char* hfs_get_upload_name_by_path(const char *filepath)
{
    int len;
    const char* p;

    len = strlen(filepath);
    HFS_DIAG("len = %d \n",len);
    p = filepath+len;
    while (len)
    {
        p--;
        if ((*p == '/') || (*p == '\\') )
            return (char*)p+1;
        len--;
    }
    return (char*)filepath;
}

static int hfs_get_upload_filename(char *data, int datalen, char* filename)
{
    int  tempdatalen = datalen;
    int  breakCnt = 0;
    char *str,*p;
    int  skiplen = 0;
    int  haskey = false;
    int  len;
    char keyTempstr[30];

    p = data;
    while (tempdatalen)
    {
        str = p;
        while( *p != '\r' && *p != '\n' && tempdatalen)
        {
            p++;
            tempdatalen--;
            if (*p == ':')
                haskey = true;
        }
        if (haskey)
        {
            char c = *p;

            *p = 0;
            len = snprintf(keyTempstr,sizeof(keyTempstr),"Content-Disposition: ");
            if (cyg_caseless_compare_withlen(keyTempstr,str,len))
            {
                char *filepath;
                // filepath example -> filename="IMG_2832.jpg"
                // filepath example -> filename="D:\temp\IMG_2832.jpg"
                if ((filepath = strstr(str+len, "filename=")))
                {
                    char *tmpName;

                    HFS_DIAG("filepath->  %s\r\n",filepath);
                    #if 0
                    strncpy(filename,filepath+10,CYGNUM_HFS_UPLOAD_FILENAME_MAXLEN);
                    #else
                    /*
                    // just test api
                    {

                    char testname1[40]="D:\\test\\222\\111.mpg";
                    char testname2[40]="D:/test/333/444.jpg";

                    tmpName = hfs_get_upload_name_by_path(testname1);
                    HFS_DIAG("tmpName->  %s\r\n",tmpName);
                    tmpName = hfs_get_upload_name_by_path(testname2);
                    HFS_DIAG("tmpName->  %s\r\n",tmpName);

                    }
                    */
                    // get filename from filepath
                    tmpName = hfs_get_upload_name_by_path(filepath+10);
                    HFS_DIAG("tmpName->  %s\r\n",tmpName);
                    // just copy filename
                    my_strncpy(filename,tmpName,CYGNUM_HFS_UPLOAD_FILENAME_MAXLEN+1);
                    #endif
                    if (filename[strlen(filename)-1] == '\"')
                    {
                        filename[strlen(filename)-1] = 0;
                    }
                    HFS_DIAG("filename=%s\r\n",filename);
                }
                *p = c;
                continue;
            }
            *p = c;
        }
        breakCnt = 0;
        str = p;
        while( (*p == '\r' || *p == '\n') && tempdatalen && breakCnt<4)
        {
            p++;
            tempdatalen--;
            breakCnt++;
        }
        if (breakCnt && ((strncmp("\r\n\r\n",str,4) ==0) || (strncmp("\n\n",str,2) ==0) || (strncmp("\n\r\n",str,3) ==0)))
        {
            skiplen = (datalen - tempdatalen);
            break;
        }
    }
    return skiplen;
}

static int hfs_get_receive_bufsize(int64_t RemainBytes, int64_t QueueWriteBytes)
{
    int RemainBufsize = CYGNUM_HFS_PINPON_BUFF_SIZE - QueueWriteBytes;
    if (RemainBytes > RemainBufsize)
        return RemainBufsize;
    else
        return RemainBytes;
}

static int hfs_upload_result_callback(hfs_session_t *session, int client, int result)
{
    int         getDataRet, ret;
    char*       sendbuf;
    hfs_server_t* pServer = session->pServer;
    HFS_U32      bufsize = CYGNUM_HFS_PINPON_BUFF_SIZE;
    char         mimetype[CYG_HFS_MIMETYPE_MAXLEN+1]="text/xml";

    sendbuf  = session->sendBuf;
    getDataRet = pServer->uploadResultCb(result, (HFS_U32)sendbuf, (HFS_U32*)&bufsize, mimetype);
    if (getDataRet == CYG_HFS_CB_GETDATA_RETURN_ERROR)
    {
        hfs_send_error(session, client,sendbuf,cyg_hfs_not_found);
        return false;
    }
    HFS_DIAG("getDataRet -> %d, sendbuf=0x%x, bufsize=0x%x,mimetype=%s \r\n", getDataRet,(int)sendbuf,(int)bufsize,mimetype);
    ret = hfs_http_separate_send(session, client, mimetype, sendbuf, bufsize, HFS_HTTP_SEPARATE_SEND_ALL);
    return ret;
}

static int hfs_write(hfs_session_t *session, int fd, char* bufAddr, int bufSize, HFS_PUT_STATUS putStatus)
{
    hfs_server_t* pServer = session->pServer;

    session->state = 7;
    if (session->is_custom == 1)
    {
		if (NULL == pServer->putCustomData)
		{
			DBG_ERR("pServer->putCustomData is NULL");
			return -1;
		}
        return pServer->putCustomData(session->filepath, session->argument, (u_int32_t)bufAddr, bufSize, session->segmentCount++, putStatus);
    }
    else
    {
		#ifdef CONFIG_SUPPORT_SSL
        if (session->reqHeader.contentMD5[0])
        {
            nvtssl_MD5_Update(&session->md5ctx,(const uint8_t *)bufAddr,bufSize);
        }
		#endif
        return write(fd, bufAddr, bufSize);
    }
}

#if 0
static void hfs_dumpmem(u_int32_t Addr, u_int32_t size)
{
    u_int32_t i,j;
    DBG_DUMP("\r\n Addr=0x%x, size =0x%x\r\n",Addr,size);


    for(j=Addr;j<(Addr + size);j+=0x10)
    {
        DBG_DUMP("^R 0x%8x:",j);
        for(i=0;i<16;i++)
            DBG_DUMP("0x%2x,",*(char*)(j+i) );
        DBG_DUMP("\r\n");
    }
    DBG_DUMP("Data %s\r\n",(char*)Addr);

}
#endif

static int hfs_handle_put(hfs_session_t *session, char* boundary, int64_t offset)
{
    #define    LINE_BREAK_LEN      2
    #define    PUT_OK              1
    #define    PUT_RECEIVE_ERROR  -1
    #define    PUT_WRITE_ERROR    -2
    #define    PUT_FILENAME_ERROR -3
    #define    PUT_FILE_OPEN_ERROR -4
    #define    PUT_FILE_SEEK_ERROR -5

    int         ret = PUT_OK;
    char*       sendbuf = session->sendBuf;
    hfs_server_t* pServer = session->pServer;
    int        client = session->client_socket;
    char       *filepath;
    char       *formdata = session->reqHeader.formdata;
    int64_t    ContentLen = session->reqHeader.contentLength;
    char       *pre_readdata = session->reqHeader.pre_readdata;
    int        pre_readdatalen = session->reqHeader.pre_readdatalen;
	#ifdef CONFIG_SUPPORT_SSL
    char*      contentMD5 = session->reqHeader.contentMD5;
	#endif
    int64_t    bufSize, RemainBytes = ContentLen, QueueWriteBytes = 0, boundarylen = 0;
    int        receiveBytes = 1;
    int        fd = -1;
    char       filename[CYGNUM_HFS_UPLOAD_FILENAME_MAXLEN+1];
    char       *pboundary_ret = NULL;
    int        isEndofFile = false;
    char        *formlist[CYGNUM_HFS_REQUEST_FORM_LIST_MAXNUM] = {0};
    char        *custom_str = NULL, *usr = NULL, *pwd = NULL, *overwrite = NULL;
    int        openflags = O_RDWR|O_CREAT;
    int        isDataThreadOpen = false;

	filename[0] = 0;
    if (session->custom_filepath[0])
    {
        filepath = session->custom_filepath;
    }
    else
    {
        filepath = session->reqHeader.path;
    }
    if (formdata)
    {
        // because hfs_formdata_parse() will change the formdata, so we need to backup it
        //memcpy(BackupFormdata,formdata,sizeof(BackupFormdata));
        my_strncpy(session->argument,formdata,sizeof(session->argument));
        /* Parse the form data */
        hfs_formdata_parse( formdata, formlist, CYGNUM_HFS_REQUEST_FORM_LIST_MAXNUM );
        custom_str = hfs_formlist_find(formlist, pServer->customStr);
        usr = hfs_formlist_find(formlist, "usr");
        pwd = hfs_formlist_find(formlist, "pwd");
        overwrite = hfs_formlist_find(formlist, "overwrite");
        if (overwrite)
        {
            openflags |= O_TRUNC;
        }
    }
    // check user, password
    if (!hfs_check_password(pServer, client, filepath, usr, pwd, session->argument))
    {
        hfs_send_error(session, client,sendbuf,cyg_hfs_access_denied);
        return false;
    }
    // put custom data
    if (custom_str || pServer->forceCustomCallback)
    {
        session->is_custom = 1;
    }
	if (session->is_custom && (!pServer->putCustomData))
    {
        DBG_ERR("putCustomData is not installed\r\n");
        return false;
    }
	if (!ContentLen)
    {
        DBG_ERR("ContentLen is 0\r\n");
        return false;
    }
    HFS_DIAG("hfs_handle_put , offset = %lld, ContentLen = %lld, pre_readdatalen= %d\r\n",offset,ContentLen,pre_readdatalen);

    // get file name from Content-Disposition:
    if (boundary)
    {
        int  skiplen;
        boundarylen = strlen(boundary);
        skiplen = hfs_get_upload_filename(pre_readdata, pre_readdatalen, filename);
        snprintf(session->filepath,sizeof(session->filepath),"%s/%s",filepath,filename);
        pre_readdata +=skiplen;
        ContentLen -=skiplen;
        pre_readdatalen -=skiplen;
        HFS_DIAG("hfs_handle_put , skiplen = %d\r\n",skiplen);
    }
    // no boundary data, need to get file name from filepath
    else
    {
        char *tmpName;
        tmpName = hfs_get_filename(filepath);
        if (*tmpName == '/')
        {
            my_strncpy(filename,tmpName+1,sizeof(filename));
        }
        else
        {
            my_strncpy(filename,tmpName,sizeof(filename));
        }
        //*tmpName = 0;
        //snprintf(session->filepath,sizeof(session->filepath),"%s/%s",filepath,filename);
        my_strncpy(session->filepath,filepath,sizeof(session->filepath));
    }
    // custom_filepath is already set by headerCb
    if (session->custom_filepath[0])
    {
        char *tmpName;
        tmpName = hfs_get_filename(filepath);
        if (*tmpName == '/')
        {
            my_strncpy(filename,tmpName+1,sizeof(filename));
        }
        else
        {
            my_strncpy(filename,tmpName,sizeof(filename));
        }
        //*tmpName = 0;
        my_strncpy(session->filepath,filepath,sizeof(session->filepath));
    }
	HFS_DIAG("pServer->rootdir=%s , filepath = %s \r\n",pServer->rootdir,filepath);
	if (0 == session->is_custom && strcmp(pServer->rootdir, session->reqHeader.path) == 0) {
		DBG_ERR("Not support upload file to rootdir %s\r\n", session->reqHeader.path);
		ret = PUT_FILENAME_ERROR;
    	goto handle_put_end2;
	}
	if (filename[0]==0 || hfs_check_filename(filename) < 0)
    {
        ret = PUT_FILENAME_ERROR;
        goto handle_put_end2;
    }
    if (pre_readdatalen > 0)
    {
        RemainBytes = ContentLen - pre_readdatalen;
        // the RemainBytes here is the filesize want to upload
        if (RemainBytes)
        {
            QueueWriteBytes = pre_readdatalen;
            memcpy(sendbuf,pre_readdata,pre_readdatalen);
        }
        // this is data end case
        else
        {
            //hfs_dumpmem(pre_readdata, pre_readdatalen);
            if (boundary)
            {
                //boundarylen = strlen(boundary);
                pboundary_ret = hfs_get_boundary(pre_readdata, pre_readdatalen, boundary, boundarylen );
            }
            if (pboundary_ret)
            {
                pre_readdatalen = pboundary_ret-pre_readdata-LINE_BREAK_LEN;
                HFS_SEND_DIAG("find boundary\r\n");
            }
        }
    }

   HFS_DIAG("filename=%s , filepath = %s \r\n",filename,filepath);
    if (session->is_custom)
    {
        // custom callback don't need to open file
        fd = -1;
        session->segmentCount = 0;
    }
    #if CONFIG_SUPPORT_CGI
    else if (session->is_cgi)
    {
        fd = session->filedesc;
    }
    #endif
    else
    {
		//struct      stat st;
        if (offset == 0)
        {
            openflags |= O_TRUNC;
        }
        //HFS_DIAG("openflags =0x%x stat = %d\r\n",openflags,stat( session->filepath, &st ));
		#if 0
        // check if need to remove file
        if ((openflags & O_TRUNC) && (stat( session->filepath, &st ) >= 0))
        {
            if (remove(session->filepath) < 0)
                DBG_ERR("Can't remove %s\r\n",session->filepath);
        }
		#endif
        fd = open(session->filepath, openflags, 0777);
        if (fd < 0)
        {
            ret = PUT_FILE_OPEN_ERROR;
            goto handle_put_end2;
        }
        if (lseek(fd, offset, SEEK_SET) < 0)
        {
            ret = PUT_FILE_OPEN_ERROR;
            goto handle_put_end2;
        }
        session->filedesc = fd;
        //snprintf(tmpfullname,sizeof(tmpfullname),"%s/__temp__.tmp",filepath);
        //fd = open(tmpfullname, openflags, 0777);
    }

    if (session->is_custom)
        MUTEX_WAIT(pServer->mutex_customCmd);

    // MD5 init
    #ifdef CONFIG_SUPPORT_SSL
    if (contentMD5[0])
    {
        nvtssl_MD5_Init(&session->md5ctx);
    }
	#endif
#if CYGNUM_HFS_SERVER_DATA_THREAD_ENABLE
    hfs_data_open(session);
    isDataThreadOpen = true;
#endif
    // this is data end case need to write pre-read data
    if (!RemainBytes && pre_readdatalen && hfs_write(session, fd, pre_readdata, pre_readdatalen, HFS_PUT_STATUS_FINISH) <0)
    {
        ret = PUT_WRITE_ERROR;
        goto handle_put_end;
    }
    HFS_DIAG("ContentLen = %lld, RemainBytes =%lld, pre_readdatalen = %d\r\n",ContentLen,RemainBytes,pre_readdatalen);
#if CYGNUM_HFS_SERVER_DATA_THREAD_ENABLE
    {
        int        pinponCnt = 0;
        ssize_t    ByteWrite;
        char      *writebuf, * tmpSendbuf;

        hfs_get_buffer(session,pinponCnt, &tmpSendbuf, &writebuf);
        while (receiveBytes && RemainBytes && !(pServer->isStop))
        {
            session->IdleTime = hfs_get_time();
            bufSize = hfs_get_receive_bufsize(RemainBytes, QueueWriteBytes);
            HFS_SEND_DIAG("bufSize=%lld, RemainBytes = %lld, QueueWriteBytes = %lld\r\n",bufSize,RemainBytes,QueueWriteBytes);
            receiveBytes = recvall(session,tmpSendbuf+QueueWriteBytes, bufSize, false);
            if (receiveBytes <= 0)
            {
                ret = PUT_RECEIVE_ERROR;
                goto handle_put_end;
            }
            QueueWriteBytes +=receiveBytes;
            RemainBytes-=receiveBytes;
            HFS_SEND_DIAG("bufSize=%lld, RemainBytes = %lld, QueueWriteBytes = %lld, receiveBytes = %d\r\n",bufSize,RemainBytes,QueueWriteBytes,receiveBytes);
            if ((QueueWriteBytes < CYGNUM_HFS_PINPON_BUFF_SIZE) && RemainBytes)
                continue;
            // receive more data to avoid the boundary data cross the buffer
            if (RemainBytes && RemainBytes < CYGNUM_HFS_RESERVE_BOUNDARY_BUFF_SIZE)
            {
                receiveBytes = recvall(session,tmpSendbuf+QueueWriteBytes, RemainBytes, false);
                if (receiveBytes <= 0)
                {
                    ret = PUT_RECEIVE_ERROR;
                    goto handle_put_end;
                }
                QueueWriteBytes +=receiveBytes;
                RemainBytes-=receiveBytes;
                HFS_SEND_DIAG("RemainBytes = %lld, QueueWriteBytes = %lld, receiveBytes = %d\r\n",RemainBytes,QueueWriteBytes,receiveBytes);
            }
            // check the boundary tag.
            if (boundary)
                pboundary_ret = hfs_get_boundary(tmpSendbuf, QueueWriteBytes, boundary, boundarylen );
            if (pboundary_ret || RemainBytes == 0)
            {
                isEndofFile = true;
            }
            if (isEndofFile)
            {
                int64_t lastWriteBytes;
                if (pboundary_ret)
                    lastWriteBytes = pboundary_ret-tmpSendbuf-LINE_BREAK_LEN;
                else
                    lastWriteBytes = QueueWriteBytes;
                ByteWrite = hfs_data_waitDone(session);
                if (ByteWrite < 0)
                {
                    ret = PUT_WRITE_ERROR;
                    goto handle_put_end;
                }
                //if (lastWriteBytes && write(fd, tmpSendbuf, lastWriteBytes) < 0)
                if (lastWriteBytes && hfs_write(session, fd, tmpSendbuf, lastWriteBytes, HFS_PUT_STATUS_FINISH) < 0)
                {
                    ret = PUT_WRITE_ERROR;
                    goto handle_put_end;
                }
                HFS_SEND_DIAG("pboundary_ret = %d, lastWriteBytes = %lld\r\n",(int)pboundary_ret,lastWriteBytes);
            }
            else
            {
                /*
                if (pinponCnt)
                {
                    ByteWrite = hfs_data_waitDone(session);
                    if (ByteWrite < 0)
                    {
                        ret = PUT_WRITE_ERROR;
                        goto handle_put_end;
                    }
                }
                */
                if (QueueWriteBytes)
                {
                    ByteWrite = hfs_data_waitDone(session);
                    if (ByteWrite < 0)
                    {
                        ret = PUT_WRITE_ERROR;
                        goto handle_put_end;
                    }
                    hfs_data_trigFileWrite(session, tmpSendbuf, QueueWriteBytes, fd);
                }
            }
            pinponCnt++;
            hfs_get_buffer(session,pinponCnt, &tmpSendbuf, &writebuf);
            QueueWriteBytes = 0;
        }
    }

#else
    while (receiveBytes && RemainBytes && !(pServer->isStop))
    {
        session->IdleTime = hfs_get_time();
        bufSize = hfs_get_receive_bufsize(RemainBytes, QueueWriteBytes);
        receiveBytes = recvall(session,sendbuf+QueueWriteBytes, bufSize, false);
        if (receiveBytes <= 0)
        {
            ret = PUT_RECEIVE_ERROR;
            goto handle_put_end;
        }
        QueueWriteBytes +=receiveBytes;
        RemainBytes-=receiveBytes;
        HFS_SEND_DIAG("bufSize=%lld, RemainBytes = %lld, QueueWriteBytes = %lld, receiveBytes = %d\r\n",bufSize,RemainBytes,QueueWriteBytes,receiveBytes);
        if ((QueueWriteBytes < CYGNUM_HFS_PINPON_BUFF_SIZE) && RemainBytes)
            continue;
        // receive more data to avoid the boundary data cross the buffer
        if (RemainBytes && RemainBytes < CYGNUM_HFS_RESERVE_BOUNDARY_BUFF_SIZE)
        {
            receiveBytes = recvall(session,sendbuf+QueueWriteBytes, RemainBytes, false);
            if (receiveBytes <= 0)
            {
                ret = PUT_RECEIVE_ERROR;
                goto handle_put_end;
            }
            QueueWriteBytes +=receiveBytes;
            RemainBytes-=receiveBytes;
            HFS_SEND_DIAG("RemainBytes = %lld, QueueWriteBytes = %lld, receiveBytes = %d\r\n",RemainBytes,QueueWriteBytes,receiveBytes);
        }
        // check the boundary tag.
        if (boundary)
            pboundary_ret = hfs_get_boundary(sendbuf, QueueWriteBytes, boundary, boundarylen );
        if (pboundary_ret)
        {
            int64_t lastWriteBytes;

            isEndofFile = true;
            lastWriteBytes = pboundary_ret-sendbuf-LINE_BREAK_LEN;
            if (lastWriteBytes && write(fd, sendbuf, lastWriteBytes) < 0)
            {
                ret = PUT_WRITE_ERROR;
                goto handle_put_end;
            }
            HFS_SEND_DIAG("find boundary,  lastWriteBytes = %lld\r\n",lastWriteBytes);
        }
        if (!isEndofFile)
        {
            if (QueueWriteBytes && write(fd, sendbuf, QueueWriteBytes) < 0)
            {
                ret = PUT_WRITE_ERROR;
                goto handle_put_end;
            }
            HFS_SEND_DIAG("write,  QueueWriteBytes = %lld\r\n",QueueWriteBytes);
        }
        QueueWriteBytes = 0;
    }
#endif
handle_put_end:
    if (session->is_custom)
        MUTEX_SIGNAL(pServer->mutex_customCmd);

handle_put_end2:
#if CYGNUM_HFS_SERVER_DATA_THREAD_ENABLE
    if (isDataThreadOpen)
        hfs_data_close(session);
#endif
    HFS_DIAG("fd = %d\r\n",fd);
    if (fd >= 0)
    {
        close(fd);
        session->filedesc = 0;
    }
    #if CONFIG_SUPPORT_CGI
    if (session->is_cgi)
    {
        return ret;
    }
    #endif
    if(ret == PUT_FILENAME_ERROR)
    {
        DBG_ERR("filename is empty\n");
        //if (!session->is_custom)
            //remove(tmpfullname);
        if (pServer->uploadResultCb)
            hfs_upload_result_callback(session, client, CYG_HFS_UPLOAD_FAIL_FILENAME_EMPTY);
        else
            hfs_send_upload_fail(session, client,sendbuf,"Upload fail, filename error");
        return false;
    }
    if(ret == PUT_FILE_OPEN_ERROR)
    {
        DBG_ERR("open filepath %s fail, openflags = 0x%x\r\n",session->filepath,openflags);
        //if (!session->is_custom)
            //remove(tmpfullname);
        if (pServer->uploadResultCb)
            hfs_upload_result_callback(session, client, CYG_HFS_UPLOAD_FAIL_FILE_EXIST);
        else
            hfs_send_upload_fail(session, client,sendbuf,"Upload fail, file exist");
        return false;
    }
    if(ret == PUT_RECEIVE_ERROR)
    {
        perror("recv");
        //if (!session->is_custom)
            //remove(tmpfullname);
        //else
        if (session->is_custom)
            hfs_write(session, 0, 0, 0, HFS_PUT_STATUS_ERR);
        if (pServer->uploadResultCb)
            hfs_upload_result_callback(session, client, CYG_HFS_UPLOAD_FAIL_RECEIVE_ERROR);
        else
            hfs_send_upload_fail(session, client,sendbuf,"Upload fail");
        return false;
    }
    if(ret == PUT_WRITE_ERROR)
    {
        perror("write");
        //if (!session->is_custom)
            //remove(tmpfullname);
        if (session->is_custom)
            hfs_write(session, 0, 0, 0, HFS_PUT_STATUS_ERR);
        if (pServer->uploadResultCb)
            hfs_upload_result_callback(session, client, CYG_HFS_UPLOAD_FAIL_WRITE_ERROR);
        else
            hfs_send_upload_fail(session, client,sendbuf,"Upload fail");
        return false;
    }
    // server is force stopped
    if (pServer->isStop && RemainBytes)
    {
        //if (!session->is_custom)
        //    remove(tmpfullname);
        //else
        if (session->is_custom)
            hfs_write(session, 0, 0, 0, HFS_PUT_STATUS_ERR);
        return false;
    }
    // custom callback case
    else if (session->is_custom)
    {
        if (pServer->uploadResultCb)
            hfs_upload_result_callback(session, client, CYG_HFS_UPLOAD_OK);
        else
            hfs_send_upload_ok(session, client,sendbuf,"Upload ok");
        return true;
    }
    // normal case
    else
    {
        #if 0
        snprintf(session->filepath, sizeof(session->filepath),"%s/%s",filepath,filename);
        if (overwrite)
        {
            remove(session->filepath);
        }
        if ( rename(tmpfullname,session->filepath) == 0 )
        #endif
        {
			#ifdef CONFIG_SUPPORT_SSL
            if (contentMD5[0])
            {
                char   result_MD5[16];
                char   *result_MD5_base64;

                nvtssl_MD5_Final(&session->md5ctx, (uint8_t *)result_MD5);
                result_MD5_base64 = hfs_base64_encode(result_MD5,sizeof(result_MD5));
                HFS_DIAG("contentMD5 %s, result_MD5_base64 %s\r\n",contentMD5, result_MD5_base64);
                if (!result_MD5_base64)
                    return false;
                if (strcmp(contentMD5,result_MD5_base64))
                {
                    if (remove(session->filepath) < 0)
                        DBG_ERR("Can't remove %s\r\n",session->filepath);
                    if (pServer->uploadResultCb)
                        hfs_upload_result_callback(session, client, CYG_HFS_UPLOAD_FAIL_MD5_CHK_FAIL);
                    else
                        hfs_send_upload_fail(session, client,sendbuf,"MD5 check fail");
                    ret = false;
                }
                else
                {
                    if (pServer->uploadResultCb)
                        hfs_upload_result_callback(session, client, CYG_HFS_UPLOAD_OK);
                    else
                        hfs_send_upload_ok(session, client,sendbuf,"Upload ok");
                    ret = true;
                }
                free(result_MD5_base64);
                return ret;
            }
            else
			#endif
            {
            //HFS_DIAG("File successfully renamed %s -> %s\r\n",tmpfullname, session->filepath);
                if (pServer->uploadResultCb)
                    hfs_upload_result_callback(session, client, CYG_HFS_UPLOAD_OK);
                else
                    hfs_send_upload_ok(session, client,sendbuf,"Upload ok");
            }
        }
        #if 0
        else
        {
            printf("rename to %s fail\n",session->filepath);
            remove(tmpfullname);
            if (pServer->uploadResultCb)
                hfs_upload_result_callback(session, client, CYG_HFS_UPLOAD_FAIL_FILE_EXIST);
            else
                hfs_send_upload_fail(session, client,sendbuf,"Upload fail, file exist");
            return false;
        }
        #endif
        return true;
    }
}
#endif
static void hfs_getFileSizeStr(char* str, int slen, u_int64_t fileSize)
{
    float fsize;
    if (fileSize >= 1048576)
    {
        fsize = (float)fileSize/1048576;
        snprintf(str,slen," %.02f MB",fsize);
    }
    else if (fileSize >= 1024)
    {
        fsize = (float)fileSize/1024;
        snprintf(str,slen," %.02f KB",fsize);
    }
    else
    {
        //fsize = fileSize;
        snprintf(str,slen," %lld B",fileSize);
    }
}

static void hfs_getTimeStr(char* str, int slen, time_t mtime)
{
	struct tm *t;

	t = gmtime( (time_t * ) & mtime );
    if (t)
    {
	    snprintf(str,slen,"%04d/%02d/%02d %02d:%02d:%02d",
				t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
				t->tm_hour, t->tm_min, t->tm_sec );
    }
    else
    {
        snprintf(str,slen,"2000/01/01 00:00:00");
    }

}

#if defined(__LINUX_USER__)
int reverse_alphasort(const struct dirent **a, const struct dirent **b)
{
	return -alphasort(a, b);
}
#endif

static int hfs_gen_dirlist_html( hfs_session_t *session, const char *name, char* buff, int* bufflen, const char* usr, const char* pwd, const int segmentCount)
{
    //static DIR *dirp;
    int  err;
    struct dirent *entry;
    int  len = 0, totallen, remain_bufsize = *bufflen;
    int  folderCnt = 0, fileCnt = 0;
    char fullname[PATH_MAX+1];
    char url[PATH_MAX+1];
    char fileSizeStr[20];
    char timeStr[24];
    struct stat sbuf;
    int    rootlen;
    //DIR   *dirp = session->dirp;
#if defined(__LINUX_USER__)
	static struct dirent **namelist;
	static int fileNum=-1, fileIdx=0;
	int i;
#else
	struct dirent readdir_r_entry;
	struct dirent *readdir_r_result = NULL;
	int ret_readdir_r = 0;
#endif


    HFS_DIAG("<INFO>: reading directory %s\r\n",name);
	buff[0] = 0;
    totallen = 0;
    rootlen = strlen(session->pServer->rootdir);
    if (segmentCount ==0)
    {
		len = snprintf(&buff[totallen],remain_bufsize,"<html>\n<body>\n");
        totallen+=len;
        if (totallen >= *bufflen)
            goto hfs_gen_dirlist_len_err;
        remain_bufsize-=len;
        len = snprintf(&buff[totallen],remain_bufsize,"<div id=folderlabel>folder</div>\n<div id=folder>%s</div>\n<div id=body>\n",&name[rootlen]);
        totallen+=len;
        if (totallen >= *bufflen)
            goto hfs_gen_dirlist_len_err;
        remain_bufsize-=len;
        len = snprintf(&buff[totallen],remain_bufsize,"<table cellpadding=5>\n<th>Filename\n<th>Filesize\n<th>Filetime\n");
        totallen+=len;
        if (totallen >= *bufflen)
            goto hfs_gen_dirlist_len_err;
        remain_bufsize-=len;
#if defined(__LINUX_USER__)
		fileIdx = 0;
		fileNum = scandir(name, &namelist, NULL, reverse_alphasort);
		if (fileNum < 0)
		{
			HFS_DIAG("scan %s fail\n", name);
            return HFS_LIST_DIR_OPEN_FAIL;
		}
#else
        session->dirp = opendir(name);
        if( session->dirp == NULL )
        {
            HFS_DIAG("dirp is NULL\n");
            return HFS_LIST_DIR_OPEN_FAIL;
        }
#endif
    }
    while (1)
    {
        // check buffer length , reserved 512 bytes
        if (totallen > *bufflen - 512)
        {
            printf("totallen=%d >  bufflen(%d)-512\n",totallen,*bufflen);
            *bufflen = totallen;
            //session->dirp = dirp;
            return HFS_LIST_DIR_CONTINUE;
        }
#if defined(__LINUX_USER__)
		if (fileIdx < fileNum)
			entry = namelist[fileIdx++];
		else
			entry = NULL;
		if( entry == NULL )
        {
            HFS_DIAG("entry is NULL\n");
            break;
        }
#else
        //entry = readdir( session->dirp );
		ret_readdir_r = readdir_r(session->dirp, &readdir_r_entry, &readdir_r_result);
		if(0 != ret_readdir_r) {
			//readdir_r error
			HFS_DIAG("read entry error\n");
            break;
		}
		if (NULL == readdir_r_result) {
			//reach the end
			break;
		}
		entry = &readdir_r_entry;
#endif

        if(strncmp(entry->d_name,".",1) &&  strncmp(entry->d_name,"..",2) )
        {
            if( name[0] )
            {
                my_strncpy(fullname, name ,sizeof(fullname));
                if( !(name[0] == '/' && name[1] == 0 ) )
                {
                    strncat(fullname, "/" ,PATH_MAX-strlen(fullname));
                    fullname[sizeof(fullname)-1]=0;
                }
            }
            else
            {
                fullname[0] = 0;
            }
            strncat(fullname, entry->d_name, PATH_MAX-strlen(fullname));
            fullname[sizeof(fullname)-1]=0;
            err = stat( fullname, &sbuf );
            hfs_path_to_url(fullname,url,PATH_MAX);
            if(err>=0)
            {
                if (S_ISDIR(sbuf.st_mode))
                {
                    folderCnt++;
                    hfs_getTimeStr(timeStr,sizeof(timeStr),sbuf.st_mtime);/*&fullname[rootlen]*/
                    if (usr && pwd)
                        len = snprintf(&buff[totallen],remain_bufsize,"<tr><td><a href=\"%s?usr=%s&pwd=%s\"><b>%s</b></a><td align=center><i>folder</i><td align=right>%s\n",&url[rootlen],usr,pwd,entry->d_name,timeStr);
                    else
                        len = snprintf(&buff[totallen],remain_bufsize,"<tr><td><a href=\"%s\"><b>%s</b></a><td align=center><i>folder</i><td align=right>%s\n",&url[rootlen],entry->d_name,timeStr);
                    totallen+=len;
                    if (totallen >= *bufflen)
                        goto hfs_gen_dirlist_len_err;
                    remain_bufsize-=len;

                }
                else
                {
                    fileCnt++;
                    hfs_getTimeStr(timeStr,sizeof(timeStr),sbuf.st_mtime);
                    hfs_getFileSizeStr(fileSizeStr,sizeof(fileSizeStr),sbuf.st_size);
                    if (usr && pwd)
                        len = snprintf(&buff[totallen],remain_bufsize,"<tr><td><a href=\"%s?usr=%s&pwd=%s\"><b>%s</b></a><td align=right>%s<td align=right>%s\n",&url[rootlen],usr,pwd,entry->d_name,fileSizeStr,timeStr);
                    else
                        len = snprintf(&buff[totallen],remain_bufsize,"<tr><td><a href=\"%s\"><b>%s</b></a><td align=right>%s<td align=right>%s\n",&url[rootlen],entry->d_name,fileSizeStr,timeStr);
                    totallen+=len;
                    if (totallen >= *bufflen)
                        goto hfs_gen_dirlist_len_err;
                    remain_bufsize-=len;
                    if (usr && pwd)
                        len = snprintf(&buff[totallen],remain_bufsize,"<td align=right><a href=\"%s?usr=%s&pwd=%s&del=1\">Remove</a>\n",&url[rootlen],usr,pwd);
                    else
                        len = snprintf(&buff[totallen],remain_bufsize,"<td align=right><a href=\"%s?del=1\">Remove</a>\n",&url[rootlen]);
                    totallen+=len;
                    if (totallen >= *bufflen)
                        goto hfs_gen_dirlist_len_err;
                    remain_bufsize-=len;
                }
            }
            HFS_DIAG("<INFO>: entry %30s,   len=%3d, totallen=%5d\r\n",entry->d_name, len,totallen);
        }
    }
    len = snprintf(&buff[totallen],remain_bufsize,"<tr><td colspan=\"4\"><hr size=1 noshade align=top></td></tr></table>\n");
    totallen+=len;
    if (totallen >= *bufflen)
        goto hfs_gen_dirlist_len_err;
    remain_bufsize-=len;

    #if CYGNUM_HFS_SUPPORT_UPLOAD
    if (usr && pwd)
        len = snprintf(&buff[totallen],remain_bufsize,"<form name=frm action=\"%s?usr=%s&pwd=%s\" method=post enctype=\"multipart/form-data\">",&name[rootlen],usr,pwd);
    else
        len = snprintf(&buff[totallen],remain_bufsize,"<form name=frm action=\"%s\" method=post enctype=\"multipart/form-data\">",&name[rootlen]);
    totallen+=len;
    if (totallen >= *bufflen)
        goto hfs_gen_dirlist_len_err;
    remain_bufsize-=len;
    len = snprintf(&buff[totallen],remain_bufsize,"<input name=fileupload1 size=70 type=file><br/><input name=upbtn type=submit value=\"Upload files\"></form>");
    totallen+=len;
    if (totallen >= *bufflen)
        goto hfs_gen_dirlist_len_err;
    remain_bufsize-=len;

    if (usr && pwd)
        len = snprintf(&buff[totallen],remain_bufsize,"<form name=frm2 action=\"%s?%s=1&usr=%s&pwd=%s\" method=post enctype=\"multipart/form-data\">",&name[rootlen],session->pServer->customStr,usr,pwd);
    else
        len = snprintf(&buff[totallen],remain_bufsize,"<form name=frm2 action=\"%s?%s=1\" method=post enctype=\"multipart/form-data\">",&name[rootlen],session->pServer->customStr);
    totallen+=len;
    if (totallen >= *bufflen)
        goto hfs_gen_dirlist_len_err;
    remain_bufsize-=len;
    len = snprintf(&buff[totallen],remain_bufsize,"<input name=fileupload2 size=70 type=file><br/><input name=upbtn type=submit value=\"Upload Custom files\"></form>");
    totallen+=len;
    if (totallen >= *bufflen)
        goto hfs_gen_dirlist_len_err;
    remain_bufsize-=len;
    #endif
//dirlist_no_file:
    len = snprintf(&buff[totallen],remain_bufsize,"</div>\n</body>\n</html>\n");
    totallen+=len;
    if (totallen >= *bufflen)
        goto hfs_gen_dirlist_len_err;
    remain_bufsize-=len;
#if defined(__LINUX_USER__)
	if (fileNum >= 0)
	{
		for (i = 0; i < fileNum; i++)
			free(namelist[i]);

	    free(namelist);
	}
#else
    if (closedir( session->dirp) < 0)
		perror("closedir");
#endif
    *bufflen = totallen;
    return HFS_LIST_DIR_OK;

hfs_gen_dirlist_len_err:
#if defined(__LINUX_USER__)
	if (fileNum >= 0)
	{
		for (i = 0; i < fileNum; i++)
			free(namelist[i]);

	    free(namelist);
	}
#else
    if (closedir( session->dirp) < 0)
		perror("closedir");
#endif
    DBG_ERR("totallen %d >= bufsize %d\r\n",totallen,*bufflen);
    return HFS_LIST_DIR_LEN_ERR;
}


static int hfs_list_dir(hfs_session_t *session, int client, const char* filepath, const char* usr, const char* pwd)
{
    int    bufsize;
    int    ret;
    char*  sendbuf;
    hfs_server_t* pServer = session->pServer;
    int    segmentCount = 0;
    char   *mimetype = hfs_get_mimetype(".html");

    sendbuf  = session->sendBuf;
    // prepare html body
    bufsize = CYGNUM_HFS_PINPON_BUFF_SIZE;
    if (pServer->genDirListHtml)
        ret = pServer->genDirListHtml(&filepath[strlen(pServer->rootdir)], (HFS_U32)sendbuf, (HFS_U32*)&bufsize, usr ,pwd, segmentCount++);
    else
        ret = hfs_gen_dirlist_html(session,filepath, sendbuf, &bufsize, usr ,pwd, segmentCount++);
    if (ret == HFS_LIST_DIR_OPEN_FAIL)
        return ret;
    else if (ret == HFS_LIST_DIR_OK || ret == HFS_LIST_DIR_LEN_ERR)
        return hfs_http_separate_send(session, client,mimetype, sendbuf, bufsize, HFS_HTTP_SEPARATE_SEND_ALL);
    else
    {
        ret = hfs_http_separate_send(session, client, mimetype, NULL, 0, HFS_HTTP_SEPARATE_SEND_HEADER);
        do
        {
            session->IdleTime = hfs_get_time();
            ret = hfs_http_separate_send(session, client, mimetype, sendbuf, bufsize, HFS_HTTP_SEPARATE_SEND_BODY);
            bufsize = CYGNUM_HFS_PINPON_BUFF_SIZE;
            if (pServer->genDirListHtml)
                ret = pServer->genDirListHtml(&filepath[strlen(pServer->rootdir)], (HFS_U32)sendbuf, (HFS_U32*)&bufsize, usr ,pwd, segmentCount++);
            else
                ret = hfs_gen_dirlist_html(session,filepath, sendbuf, &bufsize, usr ,pwd, segmentCount++);
        }while (ret == HFS_LIST_DIR_CONTINUE);
        ret = hfs_http_separate_send(session, client, mimetype, sendbuf, bufsize, HFS_HTTP_SEPARATE_SEND_BODY);
        // segment send mode
        return HFS_LIST_DIR_SEGMENT_SEND;
    }
}

static int hfs_del_file(const char* filepath)
{
    int         ret = true;

    HFS_DIAG("hfs_del_file -> %s\r\n", filepath);
    if( remove(filepath) < 0 )
    {
        perror( "Error deleting file" );
        ret = false;
    }
    return ret;
}

static int hfs_get_custom_data(hfs_session_t *session, char* filepath, int client , char* formlist)
{
    int         getDataRet, ret, result = CYG_HFS_DOWNLOAD_OK;
    int         segmentCount = 0;
    char*       sendbuf;
    hfs_server_t* pServer = session->pServer;
    HFS_U32      bufsize = CYGNUM_HFS_PINPON_BUFF_SIZE;
    char        mimetype[CYG_HFS_MIMETYPE_MAXLEN+1]="text/xml";

    MUTEX_WAIT(pServer->mutex_customCmd);
    sendbuf  = session->sendBuf;
    getDataRet = pServer->getCustomData(filepath, formlist, (HFS_U32)sendbuf, (HFS_U32*)&bufsize, mimetype, segmentCount++);
    HFS_DIAG("getDataRet -> %d, sendbuf=0x%x, bufsize=0x%x,mimetype=%s \r\n", getDataRet,(int)sendbuf,(int)bufsize,mimetype);
    if (getDataRet == CYG_HFS_CB_GETDATA_RETURN_ERROR)
    {
        hfs_send_error(session, client,sendbuf,cyg_hfs_not_found);
        result = CYG_HFS_DOWNLOAD_FILE_NOT_EXIST;
        goto get_custom_data_exit;
    }
    else if (getDataRet == CYG_HFS_CB_GETDATA_RETURN_OK)
    {
        ret = hfs_http_separate_send(session, client, mimetype, sendbuf, bufsize, HFS_HTTP_SEPARATE_SEND_ALL);
    }
    else
    {
        ret = hfs_http_separate_send(session, client, mimetype, NULL, 0, HFS_HTTP_SEPARATE_SEND_HEADER);
        if (ret == false)
            goto get_custom_err_break;
        do
        {
            session->IdleTime = hfs_get_time();
            ret = hfs_http_separate_send(session, client, mimetype, sendbuf, bufsize, HFS_HTTP_SEPARATE_SEND_BODY);
            bufsize = CYGNUM_HFS_PINPON_BUFF_SIZE;

        }while (ret && (pServer->getCustomData(filepath, formlist, (HFS_U32)sendbuf, (HFS_U32*)&bufsize, mimetype, segmentCount++) == CYG_HFS_CB_GETDATA_RETURN_CONTINUE));
        if (ret == false)
            goto get_custom_err_break;
        ret = hfs_http_separate_send(session, client, mimetype, sendbuf, bufsize, HFS_HTTP_SEPARATE_SEND_BODY);
    }
get_custom_err_break:
    if (ret == false)
	{
    	// notify the server has error , need to break
    	pServer->getCustomData(filepath, formlist, (HFS_U32)sendbuf, (HFS_U32*)&bufsize, mimetype, CYG_HFS_CB_GETDATA_SEGMENT_ERROR_BREAK);
		result = CYG_HFS_DOWNLOAD_FAIL;
	}
get_custom_data_exit:
    MUTEX_SIGNAL(pServer->mutex_customCmd);
    return result;
}

static int hfs_get_query_data(hfs_session_t *session, char* filepath, int client , char* formlist)
{
    int         getDataRet, ret;
    int         segmentCount = 0;
    char*       sendbuf = session->sendBuf;
    hfs_server_t* pServer = session->pServer;
    u_int32_t   bufsize = CYGNUM_HFS_PINPON_BUFF_SIZE;
    char        mimetype[40]="text/xml";

    getDataRet = pServer->clientQueryCb(filepath, formlist, (HFS_U32)sendbuf, (HFS_U32*)&bufsize, mimetype, segmentCount++);
    HFS_DIAG("getDataRet -> %d\r\n", getDataRet);
    if (getDataRet == CYG_HFS_CB_GETDATA_RETURN_ERROR)
    {
        hfs_send_error(session, client,sendbuf,cyg_hfs_not_found);
        return false;
    }
    else if (getDataRet == CYG_HFS_CB_GETDATA_RETURN_OK)
    {
        ret = hfs_http_separate_send(session, client, mimetype, sendbuf, bufsize, HFS_HTTP_SEPARATE_SEND_ALL);
        return ret;
    }
    else
    {
        ret = hfs_http_separate_send(session, client, mimetype, NULL, 0, HFS_HTTP_SEPARATE_SEND_HEADER);
        if (ret == false)
            return ret;
        do
        {
            session->IdleTime = hfs_get_time();
            ret = hfs_http_separate_send(session, client, mimetype, sendbuf, bufsize, HFS_HTTP_SEPARATE_SEND_BODY);
            if (ret == false)
                return ret;
            bufsize = CYGNUM_HFS_PINPON_BUFF_SIZE;

        }while (ret && (pServer->clientQueryCb(filepath, formlist, (HFS_U32)sendbuf, (HFS_U32*)&bufsize, mimetype, segmentCount++) == CYG_HFS_CB_GETDATA_RETURN_CONTINUE));
        ret = hfs_http_separate_send(session, client, mimetype, sendbuf, bufsize, HFS_HTTP_SEPARATE_SEND_BODY);
        if (ret == false)
            return ret;
        // segment send mode, need to force close the connection, so return false
        return ret;
    }
}

static int64_t hfs_mod(int64_t offset, int64_t divider)
{
    int64_t ret = offset;
    while (ret >= divider)
    {
        ret -=divider;
    }
    return ret;
}

static int hfs_get_file(hfs_session_t *session, const char* filepath, int64_t offset, int64_t fileSize, char* contentRange, int client)
{

    int         ret = CYG_HFS_DOWNLOAD_OK;
    int         len, writeBytes;
    char*       filename;
    char*       mimetype;
    char*       sendbuf = session->sendBuf;
    hfs_server_t* pServer = session->pServer;
    int         fd;
    int64_t     RemainBytes = 0;


    HFS_DIAG("filepath= %s\r\n",filepath);
    #if CONFIG_SUPPORT_CGI
    if (session->is_cgi)
    {
        fd = session->filedesc;
    }
    else
    #endif
    {
        fd = open( filepath, O_RDONLY);
        if (fd < 0)
            return CYG_HFS_DOWNLOAD_FILE_NOT_EXIST;
        if (lseek(fd, offset, SEEK_SET) < 0)
        {
            close(fd);
            return CYG_HFS_DOWNLOAD_FAIL;
        }
        session->filedesc = fd;
    }

#if CYGNUM_HFS_SERVER_DATA_THREAD_ENABLE
    session->state = 12;
    hfs_data_open(session);
    session->state = 13;
#endif

    filename = hfs_get_filename(filepath);
    if (!session->custom_mimeType[0])
        mimetype = hfs_get_mimetype(filename);
    else
        mimetype = session->custom_mimeType;
    HFS_DIAG("filepath= %s, filename= %s, mimetype=%s\n offset= %lld fileSize = %lld , of=%lld\n",filepath,filename,mimetype, offset, fileSize, hfs_mod(offset,CYGNUM_HFS_PINPON_BUFF_SIZE));

    //printf("filename= %s, offset= %lld ReadSize = %lld \n",filename,offset, fileSize);
    // send http start
    //if (contentRange)
        //len = hfs_http_start( HTTP_STATUS_CODE_PARTIAL_CONTENT ,sendbuf, CYGNUM_HFS_PINPON_BUFF_SIZE, mimetype, fileSize, contentRange, true, filename);
    #if CONFIG_SUPPORT_CGI
    if (!session->is_cgi)
    #endif
    {
        if (contentRange)
        	len = hfs_http_start( HTTP_STATUS_CODE_PARTIAL_CONTENT ,sendbuf, CYGNUM_HFS_PINPON_BUFF_SIZE, mimetype, fileSize, contentRange, true, filename);
        else
            len = hfs_http_start( HTTP_STATUS_CODE_OK ,sendbuf, CYGNUM_HFS_PINPON_BUFF_SIZE, mimetype, fileSize, contentRange, true, filename);
        writeBytes = len;
        if (sendall(session,sendbuf,&writeBytes) <=0)
        {
            #if CYGNUM_HFS_SHOW_SEND_ERROR
            perror("sendall");
            printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",len,writeBytes,client);
            #endif
            ret = CYG_HFS_DOWNLOAD_FAIL;
            goto hfs_get_file_exit;
        }
    }
    // send file data
    {
#if CYGNUM_HFS_SERVER_DATA_THREAD_ENABLE
        //int64_t    RemainBytes;
        int64_t    bufSize;
        ssize_t    ByteRead;
        //int        err = 1;
        int        pinponCnt = 0;
        char      *readbuf, * tmpSendbuf;

        RemainBytes = fileSize;

        if (RemainBytes > CYGNUM_HFS_PINPON_BUFF_SIZE)
            bufSize = (CYGNUM_HFS_PINPON_BUFF_SIZE - hfs_mod(offset,CYGNUM_HFS_PINPON_BUFF_SIZE));
        else
            bufSize = RemainBytes;

        // read first segment
        hfs_get_buffer(session,pinponCnt, &tmpSendbuf, &readbuf);
        session->state = 14;
        hfs_data_trigFileRead(session, readbuf, bufSize, fd);
        session->state = 15;
        ByteRead = hfs_data_waitDone(session);
        if (ByteRead <0)
        {
            goto hfs_get_file_exit;
        }
        RemainBytes-=ByteRead;
        //
        if (RemainBytes == 0)
        {
            writeBytes = ByteRead;
            tmpSendbuf = readbuf;
            if (sendall(session,tmpSendbuf,&writeBytes) <=0)
            {
                #if CYGNUM_HFS_SHOW_SEND_ERROR
                perror("sendall");
                printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",(int)ByteRead,writeBytes,client);
                #endif
                ret = CYG_HFS_DOWNLOAD_FAIL;
                goto hfs_get_file_exit;
            }
        }
        else
        {
            while (ByteRead && RemainBytes && !(pServer->isStop))
            {
                session->IdleTime = hfs_get_time();
                pinponCnt++;
                hfs_get_buffer(session,pinponCnt, &tmpSendbuf, &readbuf);
                if (RemainBytes > CYGNUM_HFS_PINPON_BUFF_SIZE)
                    bufSize = CYGNUM_HFS_PINPON_BUFF_SIZE;
                else
                    bufSize = RemainBytes;
                hfs_data_trigFileRead(session, readbuf, bufSize, fd);

                writeBytes = ByteRead;
                if (sendall(session,tmpSendbuf,&writeBytes) <=0)
                {
                    #if CYGNUM_HFS_SHOW_SEND_ERROR
                    perror("sendall");
                    printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",(int)ByteRead,writeBytes,client);
                    #endif
                    ret = CYG_HFS_DOWNLOAD_FAIL;
                    goto hfs_get_file_exit;
                }
                ByteRead = hfs_data_waitDone(session);
                if (ByteRead <0)
                {
                    goto hfs_get_file_exit;
                }
                RemainBytes-= ByteRead;
                HFS_SEND_DIAG("RemainBytes -> %lld\r\n", RemainBytes);
                // send last segment
                if (RemainBytes == 0)
                {
                    writeBytes = ByteRead;
                    if (sendall(session,readbuf,&writeBytes) <=0)
                    {
                        #if CYGNUM_HFS_SHOW_SEND_ERROR
                        perror("sendall");
                        printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",(int)ByteRead,writeBytes,client);
                        #endif
                        ret = CYG_HFS_DOWNLOAD_FAIL;
                        goto hfs_get_file_exit;
                    }
                }
                // check if server want to stop
                if (pServer->isStop)
                    goto hfs_get_file_exit;
            }
        }
#else
        //int64_t    bufSize, RemainBytes;
        int        err = 1;

        RemainBytes = fileSize;
        while (err && RemainBytes && !(pServer->isStop))
        {
            session->IdleTime = hfs_get_time();
            if (RemainBytes > CYGNUM_HFS_PINPON_BUFF_SIZE)
                bufSize = CYGNUM_HFS_PINPON_BUFF_SIZE;
            else
                bufSize = RemainBytes;
            err = read(fd, sendbuf, bufSize);
            RemainBytes-=bufSize;
            writeBytes = bufSize;
            if (sendall(session,sendbuf,&writeBytes) <=0)
            {
                #if CYGNUM_HFS_SHOW_SEND_ERROR
                perror("sendall");
                printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",(int)ByteRead,writeBytes,client);
                #endif
                ret = CYG_HFS_DOWNLOAD_FAIL;
                goto hfs_get_file_exit;
            }
        }
#endif
        if (RemainBytes && !(pServer->isStop))
        {
            #if CONFIG_SUPPORT_CGI
            if (!session->is_cgi)
            #endif
            {
                DBG_ERR("fileSize = %lld, RemainBytes = %lld\r\n",fileSize,RemainBytes);
            }

        }
    }
hfs_get_file_exit:
#if CYGNUM_HFS_SERVER_DATA_THREAD_ENABLE
    hfs_data_close(session);
#endif
    close(fd);
    session->filedesc = 0;
    HFS_DIAG("RemainBytes -> %lld, Sent % lld \r\n", RemainBytes,fileSize-RemainBytes);
    return ret;
}

static int hfs_isRoot(const char* rootdir, const char* filepath)
{
    return cyg_caseless_compare(rootdir,filepath);
}

static int hfs_isDigitStr(const char* str)
{
    while( *str != 0 )
    {
        //if (!isdigit(*str))
		if (!((*str) >= '0' && (*str) <= '9'))
            return false;
        str++;
    }
    return true;

}

static int hfs_delete_result_callback(hfs_session_t *session, int client, int result)
{
    int         getDataRet, ret;
    char*       sendbuf;
    hfs_server_t* pServer = session->pServer;
    HFS_U32      bufsize = CYGNUM_HFS_PINPON_BUFF_SIZE;
    char         mimetype[CYG_HFS_MIMETYPE_MAXLEN+1]="text/xml";

    sendbuf  = session->sendBuf;
    getDataRet = pServer->deleteResultCb(result, (HFS_U32)sendbuf, (HFS_U32*)&bufsize, mimetype);
    if (getDataRet == CYG_HFS_CB_GETDATA_RETURN_ERROR)
    {
        hfs_send_error(session, client,sendbuf,cyg_hfs_not_found);
        return false;
    }
    HFS_DIAG("getDataRet -> %d, sendbuf=0x%x, bufsize=0x%x,mimetype=%s \r\n", getDataRet,(int)sendbuf,(int)bufsize,mimetype);
    ret = hfs_http_separate_send(session, client, mimetype, sendbuf, bufsize, HFS_HTTP_SEPARATE_SEND_ALL);
    return ret;
}


static int hfs_handle_delete(hfs_session_t *session, int client, char* filepath, char* formdata)
{
    int         ret;
    char*       sendbuf = session->sendBuf;
    char        *formlist[CYGNUM_HFS_REQUEST_FORM_LIST_MAXNUM] = {0};
    char        *usr = NULL, *pwd = NULL;
    char        BackupFormdata[CYGNUM_HFS_REQUEST_FORM_MAXLEN+1];
    hfs_server_t* pServer = session->pServer;

    if (formdata)
    {
        // because hfs_formdata_parse() will change the formdata, so we need to backup it
        memcpy(BackupFormdata,formdata,sizeof(BackupFormdata));

        /* Parse the form data */
        hfs_formdata_parse( formdata, formlist, CYGNUM_HFS_REQUEST_FORM_LIST_MAXNUM );
        usr = hfs_formlist_find(formlist, "usr");
        pwd = hfs_formlist_find(formlist, "pwd");
    }
    // check user, password
    if (!hfs_check_password(session->pServer, client, filepath, usr, pwd, formdata))
    {
        hfs_send_error(session, client,sendbuf,cyg_hfs_access_denied);
        return false;
    }
    ret = hfs_del_file(filepath);
    if (!ret)
    {
        if (pServer->deleteResultCb)
            hfs_delete_result_callback(session, client, CYG_HFS_DELETE_FAIL);
        else
            hfs_send_error(session, client,sendbuf,cyg_hfs_not_found);
        return false;
    }
    if (pServer->deleteResultCb)
        hfs_delete_result_callback(session, client, CYG_HFS_DELETE_OK);
    else
        hfs_send_delfile_ok(session, client,sendbuf,"Delete file ok");
    return ret;

}

static int hfs_handle_head(hfs_session_t *session)
{
    int   len,writeBytes;
    char *filepath, *filename, *mimetype;
    struct      stat st;
    int64_t     fileSize;
    char *contentRange = NULL;

    filepath = session->reqHeader.path;
    filename = hfs_get_filename(filepath);
    mimetype = hfs_get_mimetype(filename);
    if (stat( filepath, &st ) < 0)
    {
        HFS_DIAG("not found filepath=%s\n",filepath);
        hfs_send_error(session, session->client_socket,session->sendBuf,cyg_hfs_not_found);
        return false;
    }
    fileSize = st.st_size;
    //if (contentRange)
        //len = hfs_http_start( HTTP_STATUS_CODE_PARTIAL_CONTENT ,session->sendbuf, mimetype, fileSize, contentRange, true, filename);
    //else
    len = hfs_http_start( HTTP_STATUS_CODE_OK ,session->sendBuf, CYGNUM_HFS_PINPON_BUFF_SIZE, mimetype, fileSize, contentRange, true, filename);
    writeBytes = len;
    if (sendall(session,session->sendBuf,&writeBytes) <=0)
    {
        #if CYGNUM_HFS_SHOW_SEND_ERROR
        perror("sendall");
        printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",len,writeBytes,session->client_socket);
        #endif
        return false;
    }
    return true;
}

static int hfs_handle_redirect(hfs_session_t *session, char* redirect_path)
{
    int   len,writeBytes,content_len;
    char  *mimetype = "text/html";

    HFS_DIAG("hfs_handle_redirect\n");
	content_len = strlen(redirect_path);
    len = hfs_http_start( HTTP_STATUS_CODE_REDIRECT ,session->sendBuf, CYGNUM_HFS_PINPON_BUFF_SIZE, mimetype, content_len, NULL, true, NULL);
    writeBytes = len;
    if (sendall(session,session->sendBuf,&writeBytes) <=0)
    {
		#if CYGNUM_HFS_SHOW_SEND_ERROR
        perror("sendall");
        printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",len,writeBytes,session->client_socket);
        #endif
        return false;
    }
	writeBytes = content_len;
	if (sendall(session,redirect_path,&writeBytes) <=0)
    {
		#if CYGNUM_HFS_SHOW_SEND_ERROR
        perror("sendall");
        printf("error len = 0x%x, writeBytes = 0x%x, client=%d\r\n",len,writeBytes,session->client_socket);
        #endif
        return false;
    }
    return true;
}

static int hfs_handle_get(hfs_session_t *session, int client, char* filepath, char* formdata, char* contentRange, int contentRangelen)
{
    int         ret;
    int         isDir;
    struct      stat st;
    hfs_server_t* pServer = session->pServer;
    char*       sendbuf = session->sendBuf;
    char        *formlist[CYGNUM_HFS_REQUEST_FORM_LIST_MAXNUM] = {0};
    char        *usr = NULL,*pwd = NULL;
    char        *custom_str = NULL;
    char        *del_str = NULL;
    char        *clientQuery_str = NULL;
    char        BackupFormdata[CYGNUM_HFS_REQUEST_FORM_MAXLEN+1] = {0};


    HFS_DIAG("hfs_handle_get\n");
    if (formdata)
    {
        // because hfs_formdata_parse() will change the formdata, so we need to backup it
        memcpy(BackupFormdata,formdata,sizeof(BackupFormdata));
        my_strncpy(session->argument,formdata,sizeof(session->argument));
        /* Parse the form data */
        hfs_formdata_parse( formdata, formlist, CYGNUM_HFS_REQUEST_FORM_LIST_MAXNUM );
        custom_str = hfs_formlist_find(formlist, pServer->customStr);
        del_str = hfs_formlist_find(formlist, "del");
        usr = hfs_formlist_find(formlist, "usr");
        pwd = hfs_formlist_find(formlist, "pwd");
        if (pServer->clientQueryCb)
        {
            //printf("BackupFormdata = %s\r\n",BackupFormdata);
            //printf("pServer->clientQueryStr = %s\r\n",pServer->clientQueryStr);
            //clientQuery_str = hfs_formlist_find(formlist, pServer->clientQueryStr);
            if (!strncmp(BackupFormdata,pServer->clientQueryStr,strlen(pServer->clientQueryStr)))
                clientQuery_str = BackupFormdata;
        }
    }
    // check user, password
    if (!hfs_check_password(pServer, client, filepath, usr, pwd, BackupFormdata))
    {
        hfs_send_error(session, client,sendbuf,cyg_hfs_access_denied);
        return false;
    }
    // get xml data
    if ((custom_str || pServer->forceCustomCallback || session->is_custom) && pServer->getCustomData)
    {
        ret = hfs_get_custom_data(session, &filepath[strlen(pServer->rootdir)], client,  BackupFormdata);
		if (pServer->downloadResultCb)
        	pServer->downloadResultCb(ret, filepath);
		return ret;
    }
    // client query callback
    else if (clientQuery_str)
    {
        return hfs_get_query_data(session, &filepath[strlen(pServer->rootdir)], client,  BackupFormdata);
    }
    // delete a file
    else if(del_str)
    {
        ret = hfs_del_file(filepath);
        if (!ret)
        {
            if (pServer->deleteResultCb)
                hfs_delete_result_callback(session, client, CYG_HFS_DELETE_FAIL);
            else
                hfs_send_error(session, client,sendbuf,cyg_hfs_not_found);
            return false;
        }
        if (pServer->deleteResultCb)
            hfs_delete_result_callback(session, client, CYG_HFS_DELETE_OK);
        else
            hfs_send_delfile_ok(session, client,sendbuf,"Delete file ok");
    }
    else
    {
        if (hfs_isRoot(pServer->rootdir, filepath))
        {
            isDir = true;
            HFS_DIAG("is Root\n");
        }
        else
        {
			// file not exist
            if (stat( filepath, &st ) < 0)
            {
                HFS_DIAG("not found filepath=%s\n",filepath);
                hfs_send_error(session, client,sendbuf,cyg_hfs_not_found);
				if (pServer->downloadResultCb)
                	pServer->downloadResultCb(CYG_HFS_DOWNLOAD_FILE_NOT_EXIST, filepath);
                return false;
            }
            isDir = S_ISDIR(st.st_mode);
        }
		// list dir
        if (isDir)
        {
            ret = hfs_list_dir(session, client, filepath ,usr, pwd);
            if (ret == HFS_LIST_DIR_OPEN_FAIL)
            {
                hfs_send_error(session, client,sendbuf,cyg_hfs_not_found);
				if (pServer->downloadResultCb)
                	pServer->downloadResultCb(CYG_HFS_DOWNLOAD_FILE_NOT_EXIST, filepath);
			    return false;
            }
            // segment send need to close the connection
            else if (ret == HFS_LIST_DIR_SEGMENT_SEND)
            {
                return false;
            }
        }
		// download file
        else
        {
            int64_t offset = 0;
            int64_t size = st.st_size;

            if (contentRange)
            {
                char *sp, *ep;
                int64_t  start , end ;

                hfs_content_range_find(contentRange, &sp, &ep);
                if (sp && sp[0])
                {
                    sscanf( sp, "%lld", &start);
                    offset = start;
                    HFS_DIAG("range start = %lld\n",start);
                    contentRange[strlen(sp)] = '-';
                }
                if (ep && ep[0])
                {
                    sscanf( ep, "%lld", &end);
                    size = end - start +1;
                    HFS_DIAG("range end = %lld\n",end);
                }
                else
                {
                    size = st.st_size - offset;
                }
                snprintf(contentRange, contentRangelen,"%lld-%lld/%lld", offset, size+offset-1,(int64_t)st.st_size);
                HFS_DIAG("contentRange=%s\r\n",contentRange);
            }
            else if (formdata)
            {
                char *offset_string;
                char *size_string;

                /* Get file offset */
                offset_string = hfs_formlist_find(formlist, "offset");
                if (offset_string && hfs_isDigitStr(offset_string))
                    sscanf( offset_string, "%lld", &offset );
                /* Get file size */
                size_string = hfs_formlist_find( formlist, "size");
                if (size_string && hfs_isDigitStr(size_string))
                    sscanf( size_string, "%lld", &size );
            }
            ret = hfs_get_file(session, filepath, offset,  size, contentRange, client);
			if (pServer->downloadResultCb)
                pServer->downloadResultCb(ret, filepath);
			if (ret == CYG_HFS_DOWNLOAD_FILE_NOT_EXIST)
            {
                hfs_send_error(session, client,sendbuf,cyg_hfs_not_found);
                return false;
            }


        }
    }
    return ret;
}
static void hfs_notify(hfs_server_t *pServer, int event)
{
    #if __USE_IPC
    hfs_ipc_sendmsg(HFS_IPC_NOTIFY_CLIENT, event);
    #else
    if (pServer->notify)
        pServer->notify(event);
    #endif
}

static void hfs_set_sock_nonblock(int client_socket, int val)
{
    int flag;

    flag = val;
	if (IOCTL( client_socket, FIONBIO, &flag ) != 0)
    {
        printf("%s: ioctl errno = %d\r\n", __func__, errno);
    }
}

static void hfs_set_sock_linger(int client_socket, int val)
{

	#if HFS_TODO
	// to close tcp socket quickly
    struct linger so_linger = { 0 };
    so_linger.l_onoff = 1;
    so_linger.l_linger = val;
    if (setsockopt(client_socket, SOL_SOCKET, SO_LINGER,(const char*)&so_linger, sizeof so_linger) < 0) {
      printf("Couldn't setsockopt(SO_LINGER)\n");
    }
	#endif

}
static void hfs_closesocket(hfs_session_t *session)
{
    //hfs_server_t* pServer = session->pServer;
    //MUTEX_WAIT(pServer->mutex);
    // the sequence is ssl_free -> shutdown socket -> close socket
    #ifdef CONFIG_SUPPORT_SSL
    if (session->is_ssl)
    {
        HFS_DIAG("ssl_free 0x%x\r\n",(int)session->ssl);
        nvtssl_free(session->ssl);
        session->ssl = NULL;
    }
    #endif
    if (session->client_socket)
    {
        shutdown(session->client_socket, SHUT_RDWR);
        SOCKET_CLOSE(session->client_socket);
        session->client_socket = 0;
    }
    //MUTEX_SIGNAL(pServer->mutex);
}

static void hfs_closesocket_for_timeout(hfs_session_t *session)
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
        HFS_DIAG("ssl_free 0x%x\r\n",(int)session->ssl);
        nvtssl_free(session->ssl);
        session->ssl = NULL;
    }
    #endif

}

#if CONFIG_SUPPORT_CGI
static int hfs_chkifcgi(char *path)
{
    // cgi ext may be .cgi .pl .py
    //if ( ((strstr(path, ".cgi")) != NULL) || ((strstr(path, ".pl")) != NULL) || ((strstr(path, ".py")) != NULL))
    if ( (strstr(path, ".cgi")) != NULL)
        return 1;
    else
    {
        // check if this is a symbolic link file
        struct stat stbuf={0};
        char   linkpath[PATH_MAX]={0};
        int    len;

        if (lstat(path, &stbuf) < 0)
            return 0;
        DBG_IND("path= %s, stbuf.st_mode=0x%x\r\n",path,stbuf.st_mode);
        DBG_IND("stbuf.st_mode & S_IFMT =0x%x\r\n",stbuf.st_mode & S_IFMT);
        DBG_IND("S_IFMT =0x%x, S_IFLNK =0x%x\r\n",S_IFMT, S_IFLNK);

        if ((stbuf.st_mode & S_IFMT) == S_IFLNK)
        {
            len = readlink(path, linkpath, PATH_MAX-1);
            if (len < 0)
                return 0;
            linkpath[len]=0;
            DBG_IND("path=%s,linkpath=%s\r\n",path,linkpath);
            if ( (strstr(linkpath, ".cgi")) != NULL)
                return 1;
        }
    }
    return 0;
}


static int chk_cgi_parm_reach_limit(int cgi_index)
{
	if (cgi_index >= CYGNUM_HFS_CGI_ARG_SIZE){
		DBG_ERR("cgienv %d >= limit %d\r\n",cgi_index,CYGNUM_HFS_CGI_ARG_SIZE);
		return true;
	}
	return false;
}
static void hfs_runcgi(hfs_session_t *session)
{
    pid_t pid;
    int   tpipe[2], spipe[2];
    hfs_req_header   *p_req_hdr = &session->reqHeader;
    char * cgiptr[CYGNUM_HFS_CGI_ARG_SIZE];
    int    cgi_index = 0, i = 0;
    char  *type = "HEAD";

    if (p_req_hdr->method == HTTP_REQUEST_METHOD_POST)
    {
        if (pipe(spipe) == -1)
        {
            DBG_ERR("create pipe\r\n");
            return;
        }
    }
	if (pipe(tpipe) == -1)
    {
        DBG_ERR("create pipe\r\n");
        return;
    }
    if ((pid = vfork()) > 0)  /* parent */
    {
        session->pid = pid;
        /* Send POST query data to CGI script */
        if ((p_req_hdr->method == HTTP_REQUEST_METHOD_POST) && (p_req_hdr->contentLength> 0))
        {
            int ret;
            HFS_DIAG("Send POST query data to CGI script\n");
            session->filedesc = spipe[1];
            session->state = 5;
            ret = hfs_handle_put(session, NULL, 0);
            session->state = 6;
            if (ret < 0)
            {
                DBG_ERR("post data error session->pid= %d",session->pid);
                return;
            }
            close(spipe[0]);
        }
        /* Close the write descriptor */
        close(tpipe[1]);
        session->filedesc = tpipe[0];
        session->state = 8;
        return;
    }
    if (pid < 0) /* vfork failed */
        exit(1);

    /* child */

    session->childpid = getpid();

    /* Our stdout/stderr goes to the socket */
    dup2(tpipe[1], 1);
    //dup2(tpipe[1], 2);
    close(tpipe[0]);
    close(tpipe[1]);

    /* If it was a POST request, send the socket data to our stdin */
    if (p_req_hdr->method == HTTP_REQUEST_METHOD_POST)
    {
        dup2(spipe[0], 0);
        close(spipe[0]);
        close(spipe[1]);
    } else    /* Otherwise we can shutdown the read side of the sock */
        shutdown(session->client_socket, 0);

    // pass parameters to cgi
    snprintf(session->cgienv[cgi_index++], CYGNUM_HFS_REQUEST_PATH_MAXLEN,
            "URL=%s", p_req_hdr->url);
    snprintf(session->cgienv[cgi_index++], CYGNUM_HFS_REQUEST_PATH_MAXLEN,
            "QUERY_STRING=%s", p_req_hdr->formdata);
    switch (p_req_hdr->method)
    {
        case HTTP_REQUEST_METHOD_GET:
            type = "GET";
            break;

        case HTTP_REQUEST_METHOD_POST:
            type = "POST";
            snprintf(session->cgienv[cgi_index++], CYGNUM_HFS_REQUEST_PATH_MAXLEN,
                        "CONTENT_LENGTH=%lld", p_req_hdr->contentLength);
            snprintf(session->cgienv[cgi_index++], CYGNUM_HFS_REQUEST_PATH_MAXLEN,
                        "CONTENT_TYPE=%s", p_req_hdr->contentType);
            snprintf(session->cgienv[cgi_index++], CYGNUM_HFS_REQUEST_PATH_MAXLEN,
                        "BOUNDARY=%s", p_req_hdr->boundary);
            break;
		default:
			break;
    }
    snprintf(session->cgienv[cgi_index++], CYGNUM_HFS_REQUEST_PATH_MAXLEN,
        "REQUEST_METHOD=%s", type);
    snprintf(session->cgienv[cgi_index++], CYGNUM_HFS_REQUEST_PATH_MAXLEN,
        "SERVER_PROTOCOL=%s", HTTP_VERSION);
    snprintf(session->cgienv[cgi_index++], CYGNUM_HFS_REQUEST_PATH_MAXLEN,
        "SERVER_NAME=%s", CYGDAT_HFS_SERVER_ID);
    #if CONFIG_SUPPORT_AUTH
    snprintf(session->cgienv[cgi_index++], CYGNUM_HFS_REQUEST_PATH_MAXLEN,
        "USER_LEVEL=%d", session->userLevel);
    #endif

    if (session->is_ssl)
    {
        snprintf(session->cgienv[cgi_index++], CYGNUM_HFS_REQUEST_PATH_MAXLEN,
            "HTTPS=on");
    }
	if (chk_cgi_parm_reach_limit(cgi_index))
		return;
    /* copy across the pointer indexes */
    for (i = 0; i < cgi_index; i++)
    {
        cgiptr[i] = session->cgienv[i];
        DBG_IND("cgiptr[%d] = %s\r\n",i,cgiptr[i]);
    }
    cgiptr[i] = NULL;
    {
        char *argv[]={p_req_hdr->path,NULL};

        // run cgi bin
        if ( -1 == execve(argv[0],argv,cgiptr))
        {
            perror( "execve" );
            exit( EXIT_FAILURE);
        }
    }
    return;
}
#endif
/* ================================================================= */
/* Main processing function                                          */
/*                                                                   */
/* Reads the HTTP header, look it up in the table and calls the      */
/* handler.                                                          */

// this function will return false, if the connection is closed.
static int hfs_process( hfs_session_t *session)
{
    int client_socket = session->client_socket;
	#if HFS_TODO
    struct sockaddr *client_address = &session->client_address;
	int calen = sizeof(*client_address);
	#endif
    char auth_buff_ret[CYGNUM_HFS_SERVER_BUFFER_SIZE];
    char name[64] = {0};
    char port[10];
    int  ret = 1;
    int  receive_complete = 0;
    char*  headerBuf = session->sendBuf + (CYGNUM_HFS_BUFF_SIZE/2);
    int  tmpBufSize = CYGNUM_HFS_PINPON_BUFF_SIZE;
    int  recvBytes = 0;
    hfs_server_t *pServer = session->pServer;
    int err = 0;
    int sock_buf_size;
    #if CONFIG_SUPPORT_AUTH
    int auth_result = false;
    int isOnvifCgi = false;
    #endif
    char *http_header_end;


    sock_buf_size = pServer->sockbufSize;
    HFS_DIAG("sock_buf_size = %d thread_hdl=(0x%x)\n",sock_buf_size,(unsigned int)session->thread_hdl);
	#if HFS_TODO
    getnameinfo(client_address, calen, name, sizeof(name),
                port, sizeof(port), NI_NUMERICHOST|NI_NUMERICSERV);
	#else
	strncpy(name, "test", sizeof(name));
	strncpy(port, "80", sizeof(port));
	#endif
    err = setsockopt(client_socket , SOL_SOCKET, SO_SNDBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
    if (err == -1)
    {
        DBG_ERR("Couldn't setsockopt(SO_SNDBUF)\n");
        //goto processing_exit;
    }

    err = setsockopt(client_socket , SOL_SOCKET, SO_RCVBUF, (char*)&sock_buf_size,sizeof(sock_buf_size));
    if (err == -1)
    {
        DBG_ERR("Couldn't setsockopt(SO_RCVBUF)\n");
        //goto processing_exit;
    }

    // type of service
    if (pServer->tos)
    {
         int  tos = pServer->tos;
         ret = setsockopt( client_socket, IPPROTO_IP, IP_TOS, (char*) &tos, sizeof(tos) );
         if (ret == -1)
         {
             DBG_ERR("Couldn't setsockopt(IP_TOS)\n");
             //goto processing_exit;
         }

         //printf("tos = %d\n",tos);
    }
    {
		// to close tcp socket quickly
	    hfs_set_sock_linger(client_socket,30);
    }


    HFS_DIAG("Connection from %s[%s]\n",name,port);

    hfs_set_sock_nonblock(client_socket,1);
    while ((!receive_complete) && (!pServer->isStop))
    {
        session->IdleTime = hfs_get_time();
		if (recvBytes >= tmpBufSize)
		{
			DBG_ERR("recvBytes %d over tmpBufSize %d\r\n",recvBytes,tmpBufSize);
			session->needclose = 1;
            goto processing_exit;
		}
    	ret = recvall(session,headerBuf+recvBytes, tmpBufSize-recvBytes, true);
        // the connection is closed by client, so we close the client_socket
        if (ret == 0)
        {
            HFS_DIAG("Connection closed by client %s[%s]\n",name,port);
			session->needclose = 1;
            goto processing_exit;
        }
        if (ret == -1)
        {
			session->needclose = 1;
            goto processing_exit;
        }
		if (recvBytes >= HTTP_HEADER_END_LEN)
			http_header_end = hfs_chk_request_end(headerBuf+recvBytes-HTTP_HEADER_END_LEN,ret+HTTP_HEADER_END_LEN);
		else
			http_header_end = hfs_chk_request_end(headerBuf,ret+recvBytes);
        if (http_header_end!=NULL)
            receive_complete = true;
        recvBytes +=ret;
    }
	if (!receive_complete)
		goto processing_exit;
    // authentication
    #if CONFIG_SUPPORT_AUTH
    {
        ret = NVTAuth_HTTPAuth(headerBuf, recvBytes, auth_buff_ret, sizeof(auth_buff_ret), &(session->userLevel));
        HFS_AUTH_DIAG("Auth ret=%d\r\n",ret);
        if (ret == NVTAUTH_ER_BUFFER_OVERFLOW)
        {
           DBG_ERR("Return buffer overflow\n");
		   session->needclose = 1;
           goto processing_exit;
        }
        if (ret == NVTAUTH_ER_HTTP_OK)
        {
           HFS_DIAG("level is %d\n", session->userLevel);
           auth_result = true;
        }
        else if (ret == NVTAUTH_ER_HTTP_BAD_REQUEST || ret == NVTAUTH_ER_HTTP_UNAUTHORIZED)
        {
            auth_result = false;
        }
    }
    #endif
    // header callback
    if (pServer->headerCb)
    {
        int         rtn;
        HFS_U32     headerlen;

        headerlen = http_header_end-headerBuf;

        HFS_DIAG("http headerlen %lu, headerBuf=0x%x,http_header_end=0x%x\n", headerlen,(int)headerBuf,(int)http_header_end);
        rtn = pServer->headerCb((HFS_U32)headerBuf, (HFS_U32)headerlen, session->custom_filepath, session->custom_mimeType, NULL);
        if (rtn == CYG_HFS_CB_HEADER_RETURN_CUSTOM)
        {
            session->is_custom = 1;
        }
        else if (rtn == CYG_HFS_CB_HEADER_RETURN_REDIRECT)
        {
            session->is_redirect = 1;
        }
		else if (rtn == CYG_HFS_CB_HEADER_RETURN_ERROR)
        {
            printf("Not support HTTP req\r\n");
			session->needclose = 1;
            ret = true;
			goto processing_exit;
        }
    }
    // parse header
    {
        hfs_req_header   *p_req_hdr = &session->reqHeader;
        char *formdata = NULL;
        char *boundary = NULL;
        char *contentRange = NULL;

        // reset header
        memset(p_req_hdr,0, sizeof(hfs_req_header));
        hfs_parse_header(headerBuf,recvBytes, p_req_hdr, session->pServer->rootdir);
        // for test
        //strncpy(p_req_hdr->contentMD5,"l2iGk+lT9jHjWISB7mWLzQ==",sizeof(p_req_hdr->contentMD5));
        if (p_req_hdr->formdata[0])
            formdata = p_req_hdr->formdata;
        if (p_req_hdr->boundary[0])
            boundary = p_req_hdr->boundary;
        if (p_req_hdr->contentRange[0])
            contentRange = p_req_hdr->contentRange;

        // check if cgi
        #if CONFIG_SUPPORT_CGI
        if (hfs_chkifcgi(p_req_hdr->path))
        {
            struct stat stbuf;
            if (stat(p_req_hdr->path, &stbuf) < 0)
            {
                DBG_ERR("cgi file %s not exist\r\n",p_req_hdr->path);
                hfs_send_error(session, client_socket, session->sendBuf,cyg_hfs_not_found);
				session->needclose = 1;
                goto processing_exit;
            }
            session->is_cgi = 1;
            #if CONFIG_SUPPORT_AUTH
            char tmpurl[64];
            snprintf(tmpurl,sizeof(tmpurl),"%s/onvif/",session->pServer->rootdir);
            if (strncmp(tmpurl,p_req_hdr->path,strlen(tmpurl)) ==0)
            {
                isOnvifCgi = true;
            }
            #endif
        }
        #endif
        #if CONFIG_SUPPORT_AUTH
        if (is_nonauth_path(session->pServer->rootdir, p_req_hdr->path) == true)
        {
            auth_result = true;
            session->userLevel = tt__UserLevel__Administrator;
        }

        HFS_AUTH_DIAG("auth_result = %d  isOnvifCgi = %d\r\n",auth_result,isOnvifCgi);
        if (auth_result == false && isOnvifCgi == false)
        {
            int    writeBytes;
            strcat(auth_buff_ret,"\r\n");
            writeBytes = strlen(auth_buff_ret);
            sendall(session,auth_buff_ret,&writeBytes);
            goto processing_exit;
        }
        #endif
        if (p_req_hdr->method == HTTP_REQUEST_METHOD_GET)
        {
			if (session->is_redirect)
            {
                hfs_handle_redirect(session,session->custom_filepath);
			}
            #if CONFIG_SUPPORT_CGI
            else if (session->is_cgi)
            {
                hfs_notify(pServer, CYG_HFS_STATUS_SERVER_RESPONSE_START);
                // run cgi
                hfs_runcgi(session);
                // get result from cgi
                session->state = 9;
                hfs_get_file(session, p_req_hdr->path , 0,  0xFFFFFFFF, NULL, client_socket);
                session->state = 10;
                hfs_notify(pServer, CYG_HFS_STATUS_SERVER_RESPONSE_END);
            }
            #endif
			else
            {
                hfs_notify(pServer, CYG_HFS_STATUS_SERVER_RESPONSE_START);
                if (session->custom_filepath[0])
                    ret = hfs_handle_get(session, client_socket,session->custom_filepath,formdata, contentRange, CYGNUM_HFS_CONTENT_RANGE_MAXLEN);
                else
                    ret = hfs_handle_get(session, client_socket,p_req_hdr->path,formdata, contentRange, CYGNUM_HFS_CONTENT_RANGE_MAXLEN);
                hfs_notify(pServer, CYG_HFS_STATUS_SERVER_RESPONSE_END);
            }

        }
        else if (p_req_hdr->method == HTTP_REQUEST_METHOD_DELETE)
        {
            hfs_notify(pServer, CYG_HFS_STATUS_SERVER_RESPONSE_START);
            ret = hfs_handle_delete(session, client_socket,p_req_hdr->path,formdata);
            hfs_notify(pServer, CYG_HFS_STATUS_SERVER_RESPONSE_END);

        }
        #if CYGNUM_HFS_SUPPORT_UPLOAD
        else if (p_req_hdr->method == HTTP_REQUEST_METHOD_PUT || p_req_hdr->method == HTTP_REQUEST_METHOD_POST)
        {
            if (!p_req_hdr->pre_readdatalen)
            {
                ret = recvall(session,auth_buff_ret, sizeof(auth_buff_ret), true);
                if (ret <= 0)
				{
					goto processing_exit;
                }
                p_req_hdr->pre_readdatalen = ret;
                p_req_hdr->pre_readdata = auth_buff_ret;

            }
            #if CONFIG_SUPPORT_CGI
            if (session->is_cgi)
            {
                hfs_notify(pServer, CYG_HFS_STATUS_SERVER_RESPONSE_START);
                // run cgi
                hfs_runcgi(session);
                // get result from cgi
                hfs_get_file(session, p_req_hdr->path , 0,  0xFFFFFFFF, NULL, client_socket);
                hfs_notify(pServer, CYG_HFS_STATUS_SERVER_RESPONSE_END);
            }
            else
            #endif
            {
                int64_t  offset = 0;

                hfs_notify(pServer, CYG_HFS_STATUS_SERVER_RESPONSE_START);
                if (contentRange)
                {
                    char *sp, *ep;
                    int64_t  start;

                    hfs_content_range_find(contentRange, &sp, &ep);
                    if (sp && sp[0])
                    {
                        sscanf( sp, "%lld", &start);
                        offset = start;
                        HFS_DIAG("range start = %lld\n",start);
                        contentRange[strlen(sp)] = '-';
                    }
                }
                ret = hfs_handle_put(session, boundary, offset);
                hfs_notify(pServer, CYG_HFS_STATUS_SERVER_RESPONSE_END);
            }
        }
        #endif
        else if (p_req_hdr->method == HTTP_REQUEST_METHOD_HEAD)
        {
            hfs_notify(pServer, CYG_HFS_STATUS_SERVER_RESPONSE_START);
            ret = hfs_handle_head(session);
            hfs_notify(pServer, CYG_HFS_STATUS_SERVER_RESPONSE_END);

        }
        else
        {
            printf("Not support HTTP method %d\r\n",p_req_hdr->method);
			session->needclose = 1;
            ret = true;
        }
    }
processing_exit:
    return ret;

}

THREAD_DECLARE(hfs_session_thrfunc,data) {
    hfs_session_t *session = (hfs_session_t *) data;
    hfs_server_t *pServer = session->pServer;

    HFS_DIAG("hfs_session_thrfunc  begin thread_hdl=(0x%x)\r\n",(unsigned int)session->thread_hdl);
    hfs_notify(pServer, CYG_HFS_STATUS_CLIENT_REQUEST);
	#if 0 //def CONFIG_SUPPORT_SSL
	if (session->is_ssl)
	{
        if (nvtssl_accept(session->ssl) != NVTSSL_OK)
			goto hfs_session_exit;
	}
	#endif
	//while (!session->needclose)
	{
    	hfs_process(session);
	}
	hfs_closesocket(session);
//hfs_session_exit:
    hfs_notify(pServer, CYG_HFS_STATUS_CLIENT_DISCONNECT);
    HFS_DIAG("hfs_session_thrfunc  end thread_hdl=(0x%x)\r\n",(unsigned int)session->thread_hdl);
    session->can_delete = 1;
    THREAD_RETURN(session->thread_hdl);
}

#if 0
static int hfs_chk_socket_up(int socket_fd)
{
    int error = 0;
    socklen_t len = sizeof (error);
    int retval = getsockopt (socket_fd, SOL_SOCKET, SO_ERROR, &error, &len);
    if (retval != 0) {
        /* there was a problem getting the error code */
        //DBG_ERR("error getting socket error code: %s\n", strerror(retval));
        return 0;
    }

    if (error != 0) {
        /* socket has a non zero error status */
        DBG_ERR("socket error: %s\n", strerror(error));
        return 0;
    }
    return 1;
}
#endif


static int hfs_chk_timeout(hfs_session_t *session)
{
    #if defined(__ECOS)
    int64_t currentT = hfs_get_time();

    if (currentT > (session->IdleTime + ((cyg_int64)session->pServer->timeoutCnt+10) * CYGNUM_SELECT_TIMEOUT_USEC )) // idle timeout 30sec
    {
	 	DBG_WRN("timeout %lld sec, url =%s, cmd=%s, pid=%d  childpid=%d %lld %lld \r\n",(int64_t)session->pServer->timeoutCnt * CYGNUM_SELECT_TIMEOUT_USEC/1000000,session->reqHeader.url,session->argument,session->pid,session->childpid,session->IdleTime,currentT);
    	return 0;
    }
    #endif
    return 0;

}
static void hfs_cleanup_sessions(hfs_server_t *pServer)
{
    hfs_session_t *session = NULL;
    hfs_session_t *search, *last = 0;
    int           isTimeout=0;

    search = pServer->active_sessions;
    while (search)
    {
        if (search->can_delete || (isTimeout = hfs_chk_timeout(search))/*|| (!hfs_chk_socket_up(search->client_socket))*/)
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
            HFS_CONN_DIAG("nr_of_clients-- = %d,  session->thread_hdl = 0x%x\r\n",pServer->nr_of_clients, (unsigned int)session->thread_hdl);
            if (isTimeout)
            {
                #if 1
                hfs_closesocket_for_timeout(session);
                if (session->dataFd)
                {
                    close(session->dataFd );
                    session->dataFd = 0;
                }
                if (session->filedesc)
                {
                    close(session->filedesc );
                    session->filedesc = 0;
                }
                THREAD_DESTROY(session->threadData_hdl);
                THREAD_DESTROY(session->thread_hdl);
                if (session->childpid)
                {
                    char   command[128];
                    snprintf(command,sizeof(command),"kill %d",session->childpid);
                    system(command);
                }
                session_exit(session);
                #endif

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

void hfs_wait_child_close(hfs_server_t *pServer)
{
    hfs_session_t *search;
    int counter;

    HFS_DIAG("hfs_wait_AllChild_close  begin\r\n");
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
    HFS_DIAG("hfs_wait_AllChild_close  end\r\n");
}

static int session_init(int sd, struct sockaddr* pClient_address, int is_ssl)
{
    hfs_session_t *session;
    hfs_server_t *pServer = &hfs_server;
	#ifdef CONFIG_SUPPORT_SSL
    NVTSSL         *ssl;
	#endif

    session = malloc(sizeof(hfs_session_t));
    HFS_DIAG("session =0x%x , size = 0x%x\r\n",(int)session, (int)sizeof(hfs_session_t));
    if (!session)
    {
        return -1;
    }
    #ifdef CONFIG_SUPPORT_SSL
    if (is_ssl)
    {
        ssl = nvtssl_server_new(pServer->servers->ssl_ctx, sd);
        if (!ssl)
        {
            free(session);
            DBG_ERR("ssl_server_new fail\r\n");
            return -1;
        }
    }
    #endif
	MUTEX_WAIT(pServer->mutex);
    memset(session, 0, sizeof(hfs_session_t));
    session->enable = true;
    COND_CREATE((session->condData));
    MUTEX_CREATE((session->condMutex),1);
    #ifdef CONFIG_SUPPORT_SSL
    if (is_ssl)
    {
        session->ssl = ssl;
        session->is_ssl = is_ssl;
    }
    #endif
    session->pServer = pServer;
    session->client_socket = sd;
    memcpy(&session->client_address,pClient_address,sizeof(struct sockaddr));
    session->next = pServer->active_sessions;
    pServer->active_sessions = session;
    THREAD_CREATE( pServer->threadPriority,
               "Hfs Session",
               session->thread_hdl,
               hfs_session_thrfunc,
               session,
               &session->stack[0],
               CYGNUM_HFS_SESSION_THREAD_STACK_SIZE,
               &session->thread_obj
        );
    THREAD_RESUME(session->thread_hdl );
    HFS_DIAG("Resume thread = 0x%x\r\n",(unsigned int)session->thread_hdl);
    pServer->nr_of_clients++;
    session->IdleTime = hfs_get_time();
    HFS_CONN_DIAG("nr_of_clients++ = %d\r\n",pServer->nr_of_clients);
    MUTEX_SIGNAL(pServer->mutex);
   if(session->thread_hdl == 0){
//	   printf("YC:hfs:fail to create thread, close fd(%d)\n", session->client_socket);
		hfs_closesocket(session);
		hfs_notify(pServer, CYG_HFS_STATUS_CLIENT_DISCONNECT);
		session->can_delete = 1;
		return 0;
   }
    return 0;
}


static void session_exit(hfs_session_t *session)
{
    COND_DESTROY((session->condData));
    MUTEX_DESTROY((session->condMutex));
	HFS_DIAG("free session 0x%x\r\n",(int)session);
    free(session);
}


static void addconnection(int sd, struct sockaddr* pClient_address, int is_ssl)
{
    hfs_server_t *pServer = &hfs_server;

    // cleanup
    MUTEX_WAIT(pServer->mutex);
    hfs_cleanup_sessions(pServer);
    MUTEX_SIGNAL(pServer->mutex);
    if (pServer->nr_of_clients >= pServer->max_nr_of_clients) {
        //char sendbuf[512];
        DBG_ERR("Exceed max clients %d\r\n",pServer->max_nr_of_clients);
        hfs_set_sock_linger(sd,0);
        shutdown(sd, SHUT_RDWR);
        SOCKET_CLOSE(sd);
        return;
    }
    else
    {
        if (session_init(sd, pClient_address, is_ssl) < 0) {
            DBG_ERR("out of memory\r\n");
			hfs_set_sock_linger(sd,0);
            shutdown(sd, SHUT_RDWR);
            SOCKET_CLOSE(sd);
            return;
        }
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



static void hfs_server_func(void *arg)
{
    fd_set master;  // master file descriptor list
    fd_set readfds; // temp file descriptor list for select()
    int fdmax = -1,i;
    int active;
    struct timeval tv;
    hfs_server_t *pServer = (hfs_server_t *)arg;
    struct serverstruct *sp;

    HFS_DIAG("hfs_server\r\n");




    MUTEX_CREATE(pServer->mutex,1);
    pServer->nr_of_clients = 0;
    pServer->active_sessions = NULL;



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
            MUTEX_WAIT(pServer->mutex);
            hfs_cleanup_sessions(pServer);
            MUTEX_SIGNAL(pServer->mutex);
            continue;
        }
        // new connection
        sp = pServer->servers;
        while (active > 0 && sp != NULL)
        {
            if (FD_ISSET(sp->sd, &readfds))
            {
                HFS_DIAG("sp->sd %d is_ssl=%d\n",sp->sd,sp->is_ssl);
                handlenewconnection(sp->sd, sp->is_ssl);
                active--;
            }
            sp = sp->next;
        }
    }
    MUTEX_WAIT(pServer->mutex);
    hfs_wait_child_close(pServer);
	hfs_cleanup_sessions(pServer);
	MUTEX_SIGNAL(pServer->mutex);
	// close all the opened sockets
    for (i=0; i<= fdmax; i++)
    {
        if (FD_ISSET(i, &master))
        {
			HFS_DIAG("close socket %d b\n",i);
            SOCKET_CLOSE(i);
            HFS_DIAG("close socket %d e\n",i);
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
    // reset the server socket value
    pServer->socket = -1;
    MUTEX_DESTROY(pServer->mutex);
    FLAG_SETPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_IDLE,pServer->condMutex_s);
}

static int openlistener(int port)
{
    int sd;
    int err = 0, flag;
    struct addrinfo *res;//,*p;
    char   portNumStr[10];
    int    portNum;;
    int    sock_buf_size;
    struct addrinfo  address;        // server address
    hfs_server_t *pServer = &hfs_server;

    HFS_DIAG("openlistener\n");
    sock_buf_size = pServer->sockbufSize;
    HFS_DIAG("sock_buf_size = %d\n",sock_buf_size);
    memset(&address,0,sizeof(address));
    address.ai_family = AF_INET;
    address.ai_socktype = SOCK_STREAM;
    address.ai_flags = AI_PASSIVE; // fill in my IP for me

    portNum = port;
    // check if port number valid
    if (portNum > CYGNUM_MAX_PORT_NUM)
    {
        printf("error port number %d\r\n",portNum);
        return -1;
    }
    snprintf(portNumStr,sizeof(portNumStr), "%d", portNum);
    err = getaddrinfo(NULL, portNumStr, &address, &res);
    if (err!=0)
    {
		#if HFS_TODO
        printf("getaddrinfo: %s\n", gai_strerror(err));
		#endif
        return -1;
    }


    /* Get the network going. This is benign if the application has
     * already done this.
     */

    /* Create and bind the server socket.
     */
    HFS_DIAG("create socket\r\n");
    sd = socket( res->ai_family, res->ai_socktype, res->ai_protocol );

    HFS_DIAG("sd = %d\r\n",sd);
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

	{
		// to close tcp socket quickly
	    hfs_set_sock_linger(sd,30);
    }


    #endif
    HFS_DIAG("bind %d\r\n",portNum);
    err = bind( sd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);   // free the linked list
    if (err !=0)
    {
        DBG_ERR("bind() returned error\n");
        SOCKET_CLOSE(sd);
        return -1;
    }
    HFS_DIAG("listen\r\n");
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
    hfs_server_t *pServer = &hfs_server;
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
/* Initialization thread
 *
 * Optionally delay for a time before getting the network
 * running. Then create and bind the server socket and put it into
 * listen mode. Spawn any further server threads, then enter server
 * mode.
 */

THREAD_DECLARE(cyg_hfs_init,arg)
{
    int sd;
    hfs_server_t *pServer = (hfs_server_t *)arg;
    int httpPort = pServer->portNum;
	#ifdef CONFIG_SUPPORT_SSL
    int httpsPort = pServer->httpsPortNum;
	#endif

    if ((sd = openlistener(httpPort)) == -1)
    {
        THREAD_RETURN(pServer->thread);
    }
    addtoservers(sd);
    #ifdef CONFIG_SUPPORT_SSL
    if (pServer->isSupportHttps)
    {
		nvtssl_init();
        pServer->servers->ssl_ctx = NULL;
        pServer->servers->is_ssl = 0;
        if ((sd = openlistener(httpsPort)) == -1)
        {
            THREAD_RETURN(pServer->thread);
        }
        addtoservers(sd);
        pServer->servers->ssl_ctx = nvtssl_ctx_new(NVTSSL_DISPLAY_CERTS,pServer->max_nr_of_clients);
        if (!pServer->servers->ssl_ctx)
        {
            DBG_ERR("ssl_ctx_new \r\n");
        }
        pServer->servers->is_ssl = 1;
        printf("hfs: listening on ports %d (http) and %d (https)\n",httpPort, httpsPort);
    }
    else
    #endif
    {
        printf("hfs: listening on port %d (http) \n",httpPort);
    }




    /* Now go be a server ourself.
     */
    hfs_server_func((void*)arg);
    THREAD_RETURN(pServer->thread);
}

static void cyg_hfs_install(cyg_hfs_open_obj*  pObj, hfs_server_t *pServer)
{
    int len;

    if (!pObj->getCustomData)
    {
        DBG_ERR("not install getCustomData func\r\n");
    }
    pServer->getCustomData = pObj->getCustomData;
    pServer->notify = pObj->notify;
    pServer->check_pwd = pObj->check_pwd;
    pServer->genDirListHtml = pObj->genDirListHtml;
    pServer->clientQueryCb = pObj->clientQueryCb;
    pServer->uploadResultCb = pObj->uploadResultCb;
	pServer->downloadResultCb = pObj->downloadResultCb;
    pServer->deleteResultCb = pObj->deleteResultCb;
	pServer->putCustomData = pObj->putCustomData;
    // not support force callback , use headerCb instead
    //pServer->forceCustomCallback = pObj->forceCustomCallback;
    pServer->headerCb = pObj->headerCb;

    if (!pObj->portNum)
    {
        pServer->portNum = CYGNUM_HFS_SERVER_PORT;
        DBG_ERR("http Port, set default port %d\r\n",CYGNUM_HFS_SERVER_PORT);
    }
    else
    {
        pServer->portNum = pObj->portNum;
    }
    pServer->isSupportHttps = 1;
    if (!pObj->httpsPortNum)
    {
        //pServer->httpsPortNum = CYGNUM_HFS_SECURE_PORT;
        //DBG_ERR("https Port, set default port %d\r\n",CYGNUM_HFS_SECURE_PORT);
        pServer->isSupportHttps = 0;
    }
    else
    {
        pServer->httpsPortNum = pObj->httpsPortNum;
    }
    if (!pObj->threadPriority)
    {
        pServer->threadPriority = CYGNUM_HFS_THREAD_PRIORITY;
        DBG_ERR("threadPriority, set default %d\r\n",CYGNUM_HFS_THREAD_PRIORITY);
    }
    else
    {
        pServer->threadPriority = pObj->threadPriority;
    }
    my_strncpy(pServer->rootdir, pObj->rootdir,sizeof(pServer->rootdir));
    len = strlen(pServer->rootdir);
    if (pServer->rootdir[len-1] == '/')
    {
        pServer->rootdir[len-1] = 0;
    }
    HFS_DIAG("rootdir=%s\r\n", pServer->rootdir);
    my_strncpy(pServer->key,pObj->key,sizeof(pServer->key));
    if (!pObj->sockbufSize)
    {
        pServer->sockbufSize = CYGNUM_HFS_SOCK_BUF_SIZE;
        DBG_ERR("sockSendbufSize, set default %d\r\n",CYGNUM_HFS_SOCK_BUF_SIZE);
    }
    else
    {
        pServer->sockbufSize = pObj->sockbufSize;
    }
	pServer->tos = pObj->tos;
    #if __USE_IPC
    if (!pObj->sharedMemAddr)
    {
        DBG_ERR("sharedMemAddr is 0\r\n");
		return;
    }
    pServer->sharedMemAddr = ipc_getNonCacheAddr(pObj->sharedMemAddr);
    HFS_DIAG("sharedMemAddr=0x%x\r\n", pServer->sharedMemAddr);
    pServer->sharedMemSize = pObj->sharedMemSize;
    #endif
    if (!pObj->maxClientNum)
    {
        pServer->max_nr_of_clients = CYGNUM_HFS_MAX_NUM_OF_CLIENTS;
    }
    else
    {
        pServer->max_nr_of_clients = pObj->maxClientNum;
    }
    HFS_DIAG("maxClientNum=%d\r\n", pServer->max_nr_of_clients);
    if (pObj->clientQueryCb)
    {
        memcpy(pServer->clientQueryStr,pObj->clientQueryStr,sizeof(pServer->clientQueryStr));
    }
    if (!pObj->timeoutCnt)
    {
        pServer->timeoutCnt = CYGNUM_HFS_SERVER_IDLE_TIME;
    }
    else
    {
        pServer->timeoutCnt = pObj->timeoutCnt;
    }
    HFS_DIAG("timeoutCnt =%d\r\n",pServer->timeoutCnt);
    if (pObj->customStr[0])
    {
        memcpy(pServer->customStr,pObj->customStr,sizeof(pServer->customStr)-1);
        pServer->customStr[sizeof(pServer->customStr)-1] = 0;
    }
    else
    {
        my_strncpy(pServer->customStr,"custom", sizeof(pServer->customStr));
    }
    HFS_DIAG("customStr=%s\r\n", pServer->customStr);
    if (pServer->forceCustomCallback && (!pObj->getCustomData || !pObj->putCustomData))
    {
        DBG_ERR("forceCustomCallback enable but not set getCustom %d, putCustom %d\r\n",(int)pObj->getCustomData,(int)pObj->putCustomData);
    }
}

#if __USE_IPC


/*
 * Declare the message structure.
 */

static int  ipc_msgid;
#if defined(__LINUX_680)
static void *map_addr = NULL;
static unsigned int map_size;
#endif
void hfs_ipc_create(cyg_hfs_open_obj*  pObj)
{
	#if defined(__LINUX_680)
	map_size = pObj->sharedMemSize;
	map_addr = NvtIPC_mmap(NVTIPC_MEM_TYPE_NONCACHE,pObj->sharedMemAddr,pObj->sharedMemSize);
	if (map_addr == NULL)
	{
        DBG_ERR("map 0x%x, size=0x%x\r\n",pObj->sharedMemAddr,pObj->sharedMemSize);
		return;
	}
	#endif
    if ((ipc_msgid = IPC_MSGGET()) < 0) {
        DBG_ERR("IPC_MSGGET = %d\r\n",ipc_msgid);

    }
}

void hfs_ipc_release(void)
{
    // release message queue
    IPC_MSGREL(ipc_msgid);
	#if defined(__LINUX_680)
	if (map_addr)
	{
		NvtIPC_munmap(map_addr, map_size);
		map_addr = NULL;
	}
	#endif
}

static void hfs_ipc_sendmsg(int cmd, int Notifystatus)
{
    HFS_IPC_NOTIFY_MSG sbuf;
    size_t buf_length;

    sbuf.mtype = HFS_IPC_MSG_TYPE_S2C;

    sbuf.uiIPC = cmd;

    sbuf.notifyStatus = Notifystatus;

    buf_length = sizeof(sbuf);

    /*
     * Send a message.
     */
    if (IPC_MSGSND(ipc_msgid, &sbuf, buf_length) < 0) {
        perror("msgsnd");
    }
}

static int hfs_ipc_getCustomData(char* path, char* argument, HFS_U32 bufAddr, HFS_U32* bufSize, char* mimeType, HFS_U32 segmentCount)
{
    HFS_IPC_MSG sbuf;
    size_t buf_length;
    #if HFS_PERF_TIME
    struct timeval tv;
    int64_t timeBegin,timeEnd;
    #endif
    hfs_server_t *pServer = &hfs_server;
    HFS_IPC_GET_CUSTOM_S *pMsg;
    HFS_U32               shareMem;
    HFS_U32               sharebuff;
    int                   rtn;

    // check if server want to stop
    if (pServer->isStop)
    {
        HFS_TIME_DIAG("getCustom server want to stop\r\n");
        return CYG_HFS_CB_GETDATA_RETURN_ERROR;
    }
    shareMem = hfs_get_ipc_sharemem();
    sbuf.mtype = HFS_IPC_MSG_TYPE_S2C;
    sbuf.uiIPC = HFS_IPC_GET_CUSTOM_DATA;
    sbuf.shareMem = ipc_getPhyAddr(shareMem);
    pMsg = (HFS_IPC_GET_CUSTOM_S *)shareMem;
    sharebuff = shareMem + CYGNUM_HFS_IPC_MSG_SIZE;
    if (sizeof(HFS_IPC_GET_CUSTOM_S) > CYGNUM_HFS_IPC_MSG_SIZE)
    {
        DBG_ERR("ipc msg size too small\r\n");
        hfs_release_ipc_sharemem(shareMem);
        return CYG_HFS_CB_GETDATA_RETURN_ERROR;
    }
    buf_length = HFS_IPC_MSGSZ;

    my_strncpy(pMsg->path,path, sizeof(pMsg->path));
    my_strncpy(pMsg->argument,argument, sizeof(pMsg->argument));
    pMsg->bufAddr = ipc_getPhyAddr(sharebuff);
    pMsg->bufSize = *bufSize;
    pMsg->mimeType[0] = 0;
    pMsg->segmentCount = segmentCount;


    /*
     * Send a message.
     */
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeBegin = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    #endif
    if (IPC_MSGSND(ipc_msgid, &sbuf, buf_length) < 0)
    {
        perror("msgsnd");
        hfs_release_ipc_sharemem(shareMem);
        return CYG_HFS_CB_GETDATA_RETURN_ERROR;
    }
    // wait response
    #if 0
    FLAG_WAITPTN_TIMEOUT(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_GET_CUSTOM_ACK,pServer->condMutex_s,true,5 ,rtn);
    // timeout error handling
    if (rtn < 0)
    {
        hfs_release_ipc_sharemem(shareMem);
        return CYG_HFS_CB_GETDATA_RETURN_ERROR;

    }
    #else
    FLAG_WAITPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_GET_CUSTOM_ACK,pServer->condMutex_s,true);
    #endif
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeEnd = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    HFS_TIME_DIAG("getCustom t= %lld us\r\n",timeEnd-timeBegin);
    #endif
    *bufSize=pMsg->bufSize;
    my_strncpy(mimeType,pMsg->mimeType, CYG_HFS_MIMETYPE_MAXLEN+1);
    rtn = pMsg->returnStatus;
    memcpy((void*)bufAddr,(void*)sharebuff,*bufSize);
    hfs_release_ipc_sharemem(shareMem);
    return rtn;
}

static int hfs_ipc_putCustomData(char* path, char* argument, HFS_U32 bufAddr, HFS_U32 bufSize, HFS_U32 segmentCount, HFS_PUT_STATUS putStatus)
{
    HFS_IPC_MSG sbuf;
    size_t buf_length;
    #if HFS_PERF_TIME
    struct timeval tv;
    int64_t timeBegin,timeEnd;
    #endif
    hfs_server_t *pServer = &hfs_server;
    HFS_IPC_PUT_CUSTOM_S *pMsg;
    HFS_U32               shareMem;
    HFS_U32               sharebuff ;
    int                   rtn;

    shareMem = hfs_get_ipc_sharemem();
    sbuf.mtype = HFS_IPC_MSG_TYPE_S2C;
    sbuf.uiIPC = HFS_IPC_PUT_CUSTOM_DATA;
    sbuf.shareMem = ipc_getPhyAddr(shareMem);
    pMsg = (HFS_IPC_PUT_CUSTOM_S *)shareMem;
    sharebuff = shareMem + CYGNUM_HFS_IPC_MSG_SIZE;
    if (sizeof(HFS_IPC_PUT_CUSTOM_S) > CYGNUM_HFS_IPC_MSG_SIZE)
    {
        DBG_ERR("ipc msg size too small\r\n");
        hfs_release_ipc_sharemem(shareMem);
        return HFS_PUT_STATUS_ERR;
    }
    buf_length = HFS_IPC_MSGSZ;

    my_strncpy(pMsg->path,path, sizeof(pMsg->path));
    my_strncpy(pMsg->argument,argument, sizeof(pMsg->argument));
    pMsg->bufAddr = ipc_getPhyAddr(sharebuff);
    pMsg->bufSize = bufSize;
    pMsg->segmentCount = segmentCount;
    pMsg->putStatus = putStatus;
    memcpy((void*)sharebuff,(void*)bufAddr,bufSize);

    /*
     * Send a message.
     */
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeBegin = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    #endif
    if (IPC_MSGSND(ipc_msgid, &sbuf, buf_length) < 0)
    {
        perror("msgsnd");
        hfs_release_ipc_sharemem(shareMem);
        return HFS_PUT_STATUS_ERR;
    }
    // wait response
    FLAG_WAITPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_PUT_CUSTOM_ACK,pServer->condMutex_s,true);
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeEnd = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    HFS_TIME_DIAG("getCustom t= %lld us\r\n",timeEnd-timeBegin);
    #endif
    rtn = pMsg->returnStatus;
    hfs_release_ipc_sharemem(shareMem);
    return rtn;
}


static int hfs_ipc_clientQueryCb(char* path, char* argument, HFS_U32 bufAddr, HFS_U32* bufSize, char* mimeType, HFS_U32 segmentCount)
{
    HFS_IPC_MSG sbuf;
    size_t buf_length;
    #if HFS_PERF_TIME
    struct timeval tv;
    int64_t timeBegin,timeEnd;
    #endif
    hfs_server_t *pServer = &hfs_server;
    HFS_IPC_CLIENT_QUERY_S *pMsg;
    HFS_U32                 shareMem;
    HFS_U32                 sharebuff;
    int                     rtn;

    // check if server want to stop
    if (pServer->isStop)
    {
        HFS_TIME_DIAG("clientQuery server want to stop\r\n");
        return CYG_HFS_CB_GETDATA_RETURN_ERROR;
    }
    shareMem = hfs_get_ipc_sharemem();
    sbuf.mtype = HFS_IPC_MSG_TYPE_S2C;
    sbuf.uiIPC = HFS_IPC_CLIENT_QUERY_DATA;
    sbuf.shareMem = ipc_getPhyAddr(shareMem);
    pMsg = (HFS_IPC_CLIENT_QUERY_S *)shareMem;
    sharebuff = shareMem + CYGNUM_HFS_IPC_MSG_SIZE;
    if (sizeof(HFS_IPC_CLIENT_QUERY_S) > CYGNUM_HFS_IPC_MSG_SIZE)
    {
        DBG_ERR("ipc msg size too small\r\n");
        hfs_release_ipc_sharemem(shareMem);
        return CYG_HFS_CB_GETDATA_RETURN_ERROR;
    }
    buf_length = HFS_IPC_MSGSZ;

    my_strncpy(pMsg->path,path, sizeof(pMsg->path));
    my_strncpy(pMsg->argument,argument, sizeof(pMsg->argument));
    pMsg->bufAddr = ipc_getPhyAddr(sharebuff);
    pMsg->bufSize = *bufSize;
    pMsg->mimeType[0] = 0;
    pMsg->segmentCount = segmentCount;


    /*
     * Send a message.
     */
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeBegin = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    #endif
    if (IPC_MSGSND(ipc_msgid, &sbuf, buf_length) < 0)
    {
        perror("msgsnd");
        hfs_release_ipc_sharemem(shareMem);
        return CYG_HFS_CB_GETDATA_RETURN_ERROR;
    }
    // wait response
    FLAG_WAITPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_CLIENT_QUERY_ACK,pServer->condMutex_s,true);
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeEnd = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    HFS_TIME_DIAG("clientQuery t= %lld us\r\n",timeEnd-timeBegin);
    #endif
    *bufSize=pMsg->bufSize;
    my_strncpy(mimeType,pMsg->mimeType, CYG_HFS_MIMETYPE_MAXLEN+1);
    rtn = pMsg->returnStatus;
    memcpy((void*)bufAddr,(void*)sharebuff,*bufSize);
    hfs_release_ipc_sharemem(shareMem);
    return rtn;
}


static int hfs_ipc_check_pwd(const char *username, const char *password, char *key,char* questionstr)
{
    HFS_IPC_MSG sbuf;
    size_t buf_length;
    #if HFS_PERF_TIME
    struct timeval tv;
    int64_t timeBegin,timeEnd;
    #endif
    int     rtn;
    hfs_server_t *pServer = &hfs_server;
    HFS_IPC_CHK_PASSWD_S *pMsg;
    HFS_U32                 shareMem;

    // check if server want to stop
    if (pServer->isStop)
    {
        HFS_TIME_DIAG("check_pwd server want to stop\r\n");
        return 0;
    }

    shareMem = hfs_get_ipc_sharemem();
    sbuf.mtype = HFS_IPC_MSG_TYPE_S2C;
    sbuf.uiIPC = HFS_IPC_CHK_PASSWD;
    sbuf.shareMem = ipc_getPhyAddr(shareMem);
    pMsg = (HFS_IPC_CHK_PASSWD_S *)shareMem;
    buf_length = HFS_IPC_MSGSZ;

    my_strncpy(pMsg->username,username, sizeof(pMsg->username));
    my_strncpy(pMsg->password,password, sizeof(pMsg->password));
    my_strncpy(pMsg->key,key, sizeof(pMsg->key));
    my_strncpy(pMsg->questionstr,questionstr, sizeof(pMsg->questionstr));


    /*
     * Send a message.
     */
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeBegin = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    #endif
    if (IPC_MSGSND(ipc_msgid, &sbuf, buf_length) < 0)
    {
        perror("msgsnd");
        hfs_release_ipc_sharemem(shareMem);
        return -1;
    }
    // wait response
    #if 0
    FLAG_WAITPTN_TIMEOUT(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_CHK_PASSWD_ACK,pServer->condMutex_s,true, 5, rtn);
    #else
    FLAG_WAITPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_CHK_PASSWD_ACK,pServer->condMutex_s,true);
    #endif
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeEnd = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    HFS_TIME_DIAG("chkpwd t= %lld us\r\n",timeEnd-timeBegin);
    #endif
    rtn = pMsg->returnStatus;
    hfs_release_ipc_sharemem(shareMem);
    return rtn;
}

static int hfs_ipc_genDirListHtml(const char *path, HFS_U32 bufAddr, HFS_U32* bufSize, const char* usr, const char* pwd, HFS_U32 segmentCount)
{
    HFS_IPC_MSG sbuf;
    size_t buf_length;
    #if HFS_PERF_TIME
    struct timeval tv;
    int64_t timeBegin,timeEnd;
    #endif
    hfs_server_t *pServer = &hfs_server;
    HFS_IPC_GEN_DIRLIST_S *pMsg;
    int                    rtn;
    HFS_U32                shareMem;
    HFS_U32                sharebuff;

    // check if server want to stop
    if (pServer->isStop)
    {
        HFS_TIME_DIAG("genDirList server want to stop\r\n");
        return HFS_LIST_DIR_OPEN_FAIL;
    }

    shareMem = hfs_get_ipc_sharemem();
    sbuf.mtype = HFS_IPC_MSG_TYPE_S2C;
    sbuf.uiIPC = HFS_IPC_GEN_DIRLIST_DATA;
    sbuf.shareMem = ipc_getPhyAddr(shareMem);
    pMsg = (HFS_IPC_GEN_DIRLIST_S *)shareMem;
    sharebuff = shareMem + CYGNUM_HFS_IPC_MSG_SIZE;
    if (sizeof(HFS_IPC_GEN_DIRLIST_S) > CYGNUM_HFS_IPC_MSG_SIZE)
    {
        DBG_ERR("ipc msg size too small\r\n");
        hfs_release_ipc_sharemem(shareMem);
        return -1;
    }

    buf_length = HFS_IPC_MSGSZ;
    my_strncpy(pMsg->path,path, sizeof(pMsg->path));
    if (usr)
    {
        my_strncpy(pMsg->usr,usr, sizeof(pMsg->usr));
    }
    else
        pMsg->usr[0]=0;
    if (pwd)
    {
        my_strncpy(pMsg->pwd,pwd, sizeof(pMsg->pwd));
    }
    else
        pMsg->pwd[0]=0;
    //pMsg->bufAddr = ipc_getPhyAddr(bufAddr);
    pMsg->bufAddr = ipc_getPhyAddr(sharebuff);
    pMsg->bufSize = *bufSize;
    pMsg->segmentCount = segmentCount;

    /*
     * Send a message.
     */
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeBegin = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    #endif
    if (IPC_MSGSND(ipc_msgid, &sbuf, buf_length) < 0)
    {
        perror("msgsnd");
        hfs_release_ipc_sharemem(shareMem);
        return -1;
    }

    HFS_TIME_DIAG("genDirList begin\r\n");

    // wait response
    #if 0
    FLAG_WAITPTN_TIMEOUT(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_GEN_DIRLIST_ACK,pServer->condMutex_s,true,5 ,rtn);
    #else

    FLAG_WAITPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_GEN_DIRLIST_ACK,pServer->condMutex_s,true);
    #endif
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeEnd = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    HFS_TIME_DIAG("genDirList t= %lld us\r\n",timeEnd-timeBegin);
    #endif
    *bufSize=pMsg->bufSize;
    rtn = pMsg->returnStatus;
    memcpy((void*)bufAddr,(void*)sharebuff,*bufSize);
    hfs_release_ipc_sharemem(shareMem);
    return rtn;
}

static int hfs_ipc_uploadResultCb(int result, HFS_U32 bufAddr, HFS_U32* bufSize, char* mimeType)
{
    HFS_IPC_MSG sbuf;
    size_t buf_length;
    #if HFS_PERF_TIME
    struct timeval tv;
    int64_t timeBegin,timeEnd;
    #endif
    hfs_server_t *pServer = &hfs_server;
    HFS_IPC_UPLOAD_RESULT_S *pMsg;
    HFS_U32                  shareMem;
    HFS_U32                  sharebuff;
    int                      rtn;

    // check if server want to stop
    if (pServer->isStop)
    {
        HFS_TIME_DIAG("uploadResultCb server want to stop\r\n");
        return CYG_HFS_CB_GETDATA_RETURN_ERROR;
    }

    shareMem = hfs_get_ipc_sharemem();
    sbuf.mtype = HFS_IPC_MSG_TYPE_S2C;
    sbuf.uiIPC = HFS_IPC_UPLOAD_RESULT_DATA;
    sbuf.shareMem = ipc_getPhyAddr(shareMem);
    pMsg = (HFS_IPC_UPLOAD_RESULT_S *)shareMem;
    sharebuff = shareMem + CYGNUM_HFS_IPC_MSG_SIZE;
    if (sizeof(HFS_IPC_UPLOAD_RESULT_S) > CYGNUM_HFS_IPC_MSG_SIZE)
    {
        DBG_ERR("ipc msg size too small\r\n");
        hfs_release_ipc_sharemem(shareMem);
        return CYG_HFS_CB_GETDATA_RETURN_ERROR;
    }

    buf_length = HFS_IPC_MSGSZ;

    pMsg->uploadResult = result;
    pMsg->bufAddr = ipc_getPhyAddr(sharebuff);
    pMsg->bufSize = *bufSize;
    pMsg->mimeType[0] = 0;


    /*
     * Send a message.
     */
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeBegin = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    #endif
    if (IPC_MSGSND(ipc_msgid, &sbuf, buf_length) < 0)
    {
        perror("msgsnd");
        hfs_release_ipc_sharemem(shareMem);
        return CYG_HFS_CB_GETDATA_RETURN_ERROR;
    }
    // wait response
    FLAG_WAITPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_UPLOAD_RESULT_ACK,pServer->condMutex_s,true);
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeEnd = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    HFS_TIME_DIAG("uploadResultCb t= %lld us\r\n",timeEnd-timeBegin);
    #endif
    *bufSize=pMsg->bufSize;
    my_strncpy(mimeType,pMsg->mimeType, CYG_HFS_MIMETYPE_MAXLEN+1);
    rtn = pMsg->returnStatus;
    memcpy((void*)bufAddr,(void*)sharebuff,*bufSize);
    hfs_release_ipc_sharemem(shareMem);
    return rtn;
}

static void hfs_ipc_downloadResultCb(int result, char* path)
{
    HFS_IPC_MSG sbuf;
    size_t buf_length;
    #if HFS_PERF_TIME
    struct timeval tv;
    int64_t timeBegin,timeEnd;
    #endif
    hfs_server_t *pServer = &hfs_server;
    HFS_IPC_DOWNLOAD_RESULT_S *pMsg;
    HFS_U32                  shareMem;



    // check if server want to stop
    if (pServer->isStop)
    {
        HFS_TIME_DIAG("downloadResultCb server want to stop\r\n");
        return;
    }

    shareMem = hfs_get_ipc_sharemem();
    sbuf.mtype = HFS_IPC_MSG_TYPE_S2C;
    sbuf.uiIPC = HFS_IPC_DOWNLOAD_RESULT_DATA;
    sbuf.shareMem = ipc_getPhyAddr(shareMem);
    pMsg = (HFS_IPC_DOWNLOAD_RESULT_S *)shareMem;
    if (sizeof(HFS_IPC_DOWNLOAD_RESULT_S) > CYGNUM_HFS_IPC_MSG_SIZE)
    {
        DBG_ERR("ipc msg size too small\r\n");
        hfs_release_ipc_sharemem(shareMem);
        return;
    }

    buf_length = HFS_IPC_MSGSZ;

    pMsg->downloadResult = result;
    my_strncpy(pMsg->path, path, sizeof(pMsg->path));


    /*
     * Send a message.
     */
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeBegin = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    #endif
    if (IPC_MSGSND(ipc_msgid, &sbuf, buf_length) < 0)
    {
        perror("msgsnd");
        hfs_release_ipc_sharemem(shareMem);
        return;
    }
    // wait response
    FLAG_WAITPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_DOWNLOAD_RESULT_ACK,pServer->condMutex_s,true);
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeEnd = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    HFS_TIME_DIAG("downloadResultCb t= %lld us\r\n",timeEnd-timeBegin);
    #endif
    hfs_release_ipc_sharemem(shareMem);
}


static int hfs_ipc_deleteResultCb(int result, HFS_U32 bufAddr, HFS_U32* bufSize, char* mimeType)
{
    HFS_IPC_MSG sbuf;
    size_t buf_length;
    #if HFS_PERF_TIME
    struct timeval tv;
    int64_t timeBegin,timeEnd;
    #endif
    hfs_server_t *pServer = &hfs_server;
    HFS_IPC_DELETE_RESULT_S *pMsg;
    HFS_U32                 shareMem;
    HFS_U32                 sharebuff;
    int     rtn;

    // check if server want to stop
    if (pServer->isStop)
    {
        HFS_TIME_DIAG("deleteResultCb server want to stop\r\n");
        return CYG_HFS_CB_GETDATA_RETURN_ERROR;
    }
    shareMem = hfs_get_ipc_sharemem();
    sbuf.mtype = HFS_IPC_MSG_TYPE_S2C;
    sbuf.uiIPC = HFS_IPC_DELETE_RESULT_DATA;
    sbuf.shareMem = ipc_getPhyAddr(shareMem);
    pMsg = (HFS_IPC_DELETE_RESULT_S *)shareMem;
    sharebuff = shareMem + CYGNUM_HFS_IPC_MSG_SIZE;
    if (sizeof(HFS_IPC_DELETE_RESULT_S) > CYGNUM_HFS_IPC_MSG_SIZE)
    {
        DBG_ERR("ipc msg size too small\r\n");
        hfs_release_ipc_sharemem(shareMem);
        return CYG_HFS_CB_GETDATA_RETURN_ERROR;
    }

    buf_length = HFS_IPC_MSGSZ;

    pMsg->deleteResult = result;
    pMsg->bufAddr = ipc_getPhyAddr(sharebuff);
    pMsg->bufSize = *bufSize;
    pMsg->mimeType[0] = 0;


    /*
     * Send a message.
     */
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeBegin = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    #endif
    if (IPC_MSGSND(ipc_msgid, &sbuf, buf_length) < 0)
    {
        perror("msgsnd");
        hfs_release_ipc_sharemem(shareMem);
        return CYG_HFS_CB_GETDATA_RETURN_ERROR;
    }
    // wait response
    FLAG_WAITPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_DELETE_RESULT_ACK,pServer->condMutex_s,true);
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeEnd = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    HFS_TIME_DIAG("deleteResultCb t= %lld us\r\n",timeEnd-timeBegin);
    #endif
    *bufSize=pMsg->bufSize;
    my_strncpy(mimeType,pMsg->mimeType, CYG_HFS_MIMETYPE_MAXLEN+1);
    rtn = pMsg->returnStatus;
    memcpy((void*)bufAddr,(void*)sharebuff,*bufSize);
    hfs_release_ipc_sharemem(shareMem);
    return rtn;
}

static int hfs_ipc_headerCb(HFS_U32 headerAddr, HFS_U32 headerSize, char* filepath, char* mimeType, void *reserved)
{
    HFS_IPC_MSG sbuf;
    size_t buf_length;
    #if HFS_PERF_TIME
    struct timeval tv;
    int64_t timeBegin,timeEnd;
    #endif
    hfs_server_t *pServer = &hfs_server;
    HFS_IPC_HEADER_S *pMsg;
    HFS_U32                 shareMem;
    HFS_U32                 sharebuff;
    int     rtn;

    // check if server want to stop
    if (pServer->isStop)
    {
        HFS_TIME_DIAG("headerCb server want to stop\r\n");
        return CYG_HFS_CB_HEADER_RETURN_ERROR;
    }
    shareMem = hfs_get_ipc_sharemem();
    sbuf.mtype = HFS_IPC_MSG_TYPE_S2C;
    sbuf.uiIPC = HFS_IPC_HEADER_CB;
    sbuf.shareMem = ipc_getPhyAddr(shareMem);
    pMsg = (HFS_IPC_HEADER_S *)shareMem;
    sharebuff = shareMem + CYGNUM_HFS_IPC_MSG_SIZE;
    if (sizeof(HFS_IPC_HEADER_S) > CYGNUM_HFS_IPC_MSG_SIZE)
    {
        DBG_ERR("ipc msg size too small\r\n");
        hfs_release_ipc_sharemem(shareMem);
        return CYG_HFS_CB_HEADER_RETURN_ERROR;
    }

    buf_length = HFS_IPC_MSGSZ;

    pMsg->headerAddr = ipc_getPhyAddr(sharebuff);
    pMsg->headerSize = headerSize;
    pMsg->filepath[0] = 0;
    pMsg->mimeType[0] = 0;
    memcpy((void*)sharebuff,(void*)headerAddr,headerSize);
    *(char*)(sharebuff+headerSize) = 0;
    /*
     * Send a message.
     */
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeBegin = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    #endif
    if (IPC_MSGSND(ipc_msgid, &sbuf, buf_length) < 0)
    {
        perror("msgsnd");
        hfs_release_ipc_sharemem(shareMem);
        return CYG_HFS_CB_HEADER_RETURN_ERROR;
    }
    // wait response
    FLAG_WAITPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_HEADER_CB_ACK,pServer->condMutex_s,true);
    #if HFS_PERF_TIME
    gettimeofday(&tv,NULL);
    timeEnd = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    HFS_TIME_DIAG("deleteResultCb t= %lld us\r\n",timeEnd-timeBegin);
    #endif
    my_strncpy(filepath,pMsg->filepath, CYG_HFS_FILE_PATH_MAXLEN+1);
    my_strncpy(mimeType,pMsg->mimeType, CYG_HFS_MIMETYPE_MAXLEN+1);
    rtn = pMsg->returnStatus;
    hfs_release_ipc_sharemem(shareMem);
    return rtn;
}

THREAD_DECLARE(hfs_close_thrfunc,arg) {

    cyg_hfs_close();
    THREAD_RETURN(hfs_server.close_thread);
}

THREAD_DECLARE(hfs_ipc_rcv_main_func,arg)
{
    int           continue_loop = 1, ret;
    hfs_server_t  *pServer = &hfs_server;
    HFS_IPC_MSG   sbuf;

    while (continue_loop)
    {
        if ((ret = IPC_MSGRCV(ipc_msgid, &sbuf, HFS_IPC_MSGSZ)) < 0)
        {
            DBG_ERR("IPC_MSGRCV ret=%d\r\n",(int)ret);
            break;
        }
        DBG_IND("hfs uiIPC=%d\r\n",(int)sbuf.uiIPC);
        switch (sbuf.uiIPC)
        {
            case HFS_IPC_CLOSE_SERVER:
                DBG_IND("close server\r\n");
                //continue_loop = 0;
                // create ipc receive thread
                THREAD_CREATE( pServer->threadPriority,
                                   "hfs ipc close thread",
                                   pServer->close_thread,
                                   hfs_close_thrfunc,
                                   0,
                                   &pServer->close_thread_stacks[0],
                                   sizeof(pServer->close_thread_stacks),
                                   &pServer->close_thread_object
                    );
                THREAD_RESUME( pServer->close_thread );
                break;
            case HFS_IPC_CLOSE_FINISH:
                continue_loop = 0;
                break;

            case HFS_IPC_NOTIFY_CLIENT_ACK:
                FLAG_SETPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_NOTIFY_CLIENT_ACK,pServer->condMutex_s);
                break;

            case HFS_IPC_GET_CUSTOM_DATA_ACK:
                FLAG_SETPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_GET_CUSTOM_ACK,pServer->condMutex_s);
                break;

            case HFS_IPC_PUT_CUSTOM_DATA_ACK:
                FLAG_SETPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_PUT_CUSTOM_ACK,pServer->condMutex_s);
                break;

            case HFS_IPC_CHK_PASSWD_ACK:
                FLAG_SETPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_CHK_PASSWD_ACK,pServer->condMutex_s);
                break;

            case HFS_IPC_GEN_DIRLIST_DATA_ACK:
                FLAG_SETPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_GEN_DIRLIST_ACK,pServer->condMutex_s);
                break;

            case HFS_IPC_CLIENT_QUERY_DATA_ACK:
                FLAG_SETPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_CLIENT_QUERY_ACK,pServer->condMutex_s);
                break;

            case HFS_IPC_UPLOAD_RESULT_DATA_ACK:
                FLAG_SETPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_UPLOAD_RESULT_ACK,pServer->condMutex_s);
                break;

			case HFS_IPC_DOWNLOAD_RESULT_DATA_ACK:
                FLAG_SETPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_DOWNLOAD_RESULT_ACK,pServer->condMutex_s);
                break;

            case HFS_IPC_DELETE_RESULT_DATA_ACK:
                FLAG_SETPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_DELETE_RESULT_ACK,pServer->condMutex_s);
                break;

            case HFS_IPC_HEADER_CB_ACK:
                FLAG_SETPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_HEADER_CB_ACK,pServer->condMutex_s);
                break;

            default:
                DBG_ERR("Unknown uiIPC=%d\r\n",(int)sbuf.uiIPC);
                break;
        }

    }
    // release ipc
    cyg_hfs_release_resource(pServer);
    #if __BUILD_PROCESS
    sem_post(&main_sem);
    #endif
    THREAD_RETURN(pServer->rcv_thread);
}



#endif

#if CONFIG_SUPPORT_AUTH
int is_nonauth_path(const char *rootdir, const char *path)
{
    int ret = false;
    int path_offset=strlen(rootdir);

    if (non_auth_head != NULL)
    {
        struct non_auth_node *tmp_ptr=non_auth_head;

        while (tmp_ptr != NULL)
        {
            if (!strncmp(path+path_offset, tmp_ptr->path, strlen(tmp_ptr->path)))
            {
                ret = true;
                break;
            }

            tmp_ptr = tmp_ptr->next;
        }
    }

    if (ret == true)
        HFS_AUTH_DIAG("%s is non auth path\n", path);
    else
        HFS_AUTH_DIAG("%s is auth path\n", path);

    return ret;
}

void hfs_conf_file_parse(void)
{
    FILE *fp=NULL;
    char fbuf[CYGNUM_HFS_REQUEST_PATH_MAXLEN*2];

    if ((fp = fopen(HFS_CONF_FILE, "r")) != NULL)
    {
        struct non_auth_node *last_ptr=NULL, *tmp_ptr=NULL;

        while (fgets(fbuf, sizeof(fbuf), fp) != NULL)
        {
            if (fbuf[0] != '#' && strlen(fbuf) > 0)
            {
                if (!strncmp(fbuf, "NonAuthDir", strlen("NonAuthDir")))
                {
                    if ((tmp_ptr = malloc(sizeof(struct non_auth_node))) != NULL)
                    {
                        tmp_ptr->next = NULL;
                        sscanf(fbuf, "%*s %s", tmp_ptr->path);
                        if (tmp_ptr->path[strlen(tmp_ptr->path)-1] != '/')
                            strcat(tmp_ptr->path, "/");
                    }
                }
                else if (!strncmp(fbuf, "NonAuthFile", strlen("NonAuthFile")))
                {
                    if ((tmp_ptr = malloc(sizeof(struct non_auth_node))) != NULL)
                    {
                        tmp_ptr->next = NULL;
                        sscanf(fbuf, "%*s %s", tmp_ptr->path);
                    }
                }

                if (non_auth_head == NULL)
                {
                    non_auth_head = tmp_ptr;
                    last_ptr = tmp_ptr;
                }
                else
                {
                    last_ptr->next = tmp_ptr;
                    last_ptr = tmp_ptr;
                }
            }
        }
        fclose(fp);

        tmp_ptr = non_auth_head;
        DBG_IND("non auth path:\r\n");
        while (tmp_ptr != NULL)
        {
            DBG_IND("%s\r\n", tmp_ptr->path);
            tmp_ptr = tmp_ptr->next;
        }
    }
}

void hfs_conf_file_release(void)
{
    if (non_auth_head != NULL)
    {
        struct non_auth_node *tmp_ptr=non_auth_head;

        while (tmp_ptr != NULL)
        {
            non_auth_head = non_auth_head->next;
            free(tmp_ptr);
            tmp_ptr = non_auth_head;
        }
    }
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

static void cyg_hfs_create_resource(cyg_hfs_open_obj*  pObj,hfs_server_t *pServer)
{
	#if __USE_IPC
    // init ipc
    hfs_ipc_create(pObj);
    #endif
	MUTEX_CREATE(pServer->mutex_ipc,1);
    MUTEX_CREATE(pServer->mutex_sharemem[0],1);
    MUTEX_CREATE(pServer->mutex_sharemem[1],1);
    MUTEX_CREATE(pServer->mutex_customCmd,1);
    MUTEX_CREATE(pServer->condMutex_s,1);
    COND_CREATE(pServer->cond_s);
}

static void cyg_hfs_release_resource(hfs_server_t *pServer)
{
	COND_DESTROY(pServer->cond_s);
    MUTEX_DESTROY(pServer->condMutex_s);
    MUTEX_DESTROY(pServer->mutex_ipc);
    MUTEX_DESTROY(pServer->mutex_sharemem[0]);
    MUTEX_DESTROY(pServer->mutex_sharemem[1]);
    MUTEX_DESTROY(pServer->mutex_customCmd);
	#if __USE_IPC
	// release ipc
    hfs_ipc_release();
	#endif
}


extern void cyg_hfs_open(cyg_hfs_open_obj*  pObj)
{
    hfs_server_t *pServer = &hfs_server;
    MUTEX_WAIT(openclose_mtx);
    if (!pServer->isStart)
    {
        #if __USE_IPC
        // check share memory size
        if (!pObj->sharedMemAddr)
        {
            DBG_ERR("sharedMemAddr is NULL\r\n");
            MUTEX_SIGNAL(openclose_mtx);
            return;
        }
        if (pObj->sharedMemSize < CYGNUM_HFS_BUFF_SIZE + sizeof(HFS_IPC_GET_CUSTOM_S) * 2)
        {
            DBG_ERR("sharedMemSize %d < needed %d\r\n",(int)pObj->sharedMemSize,(int)(CYGNUM_HFS_BUFF_SIZE + sizeof(HFS_IPC_GET_CUSTOM_S) * 2));
            MUTEX_SIGNAL(openclose_mtx);
            return;
        }
        // init ipc
        #endif
		cyg_hfs_create_resource(pObj,pServer);
        cyg_hfs_install(pObj,pServer);
        pServer->isStop = false;


        FLAG_CLRPTN(pServer->eventFlag,0xFFFFFFFF,pServer->condMutex_s);
        HFS_DIAG("cyg_hfs_startup create thread\r\n");
        THREAD_CREATE( pServer->threadPriority,
                           "hfs server",
                           pServer->thread,
                           cyg_hfs_init,
                           pServer,
                           &pServer->stacks[0],
                           sizeof(pServer->stacks),
                           &pServer->thread_object
            );
        HFS_DIAG("cyg_hfs_startup resume thread\r\n");
        THREAD_RESUME( pServer->thread );

        #if __USE_IPC
        // create ipc receive thread
        THREAD_CREATE( pServer->threadPriority,
                           "hfs ipc rcv thread",
                           pServer->rcv_thread,
                           hfs_ipc_rcv_main_func,
                           pObj,
                           &pServer->rcv_thread_stacks[0],
                           sizeof(pServer->rcv_thread_stacks),
                           &pServer->rcv_thread_object
            );
        THREAD_RESUME( pServer->rcv_thread );
        hfs_ipc_sendmsg(HFS_IPC_SERVER_STARTED, 0);
        #endif
        pServer->isStart = true;
    }
    MUTEX_SIGNAL(openclose_mtx);
}

extern void cyg_hfs_close(void)
{
    hfs_server_t *pServer = &hfs_server;
    MUTEX_WAIT(openclose_mtx);
    if (pServer->isStart)
    {
        HFS_DIAG("cyg_hfs_close wait idle begin\r\n");
        pServer->isStop = true;
        FLAG_WAITPTN(pServer->cond_s,pServer->eventFlag,CYG_FLG_HFS_IDLE,pServer->condMutex_s,false);
        HFS_DIAG("cyg_hfs_close wait idle end\r\n");
        THREAD_DESTROY(pServer->thread);
		#if 0
        COND_DESTROY(pServer->cond_s);
        MUTEX_DESTROY(pServer->condMutex_s);
        MUTEX_DESTROY(pServer->mutex_ipc);
        MUTEX_DESTROY(pServer->mutex_sharemem[0]);
        MUTEX_DESTROY(pServer->mutex_sharemem[1]);
        MUTEX_DESTROY(pServer->mutex_customCmd);
		#endif

        #if __USE_IPC
        {
            HFS_IPC_MSG   sbuf;
            // notify client to exit
            sbuf.mtype = HFS_IPC_MSG_TYPE_S2C;
            sbuf.uiIPC = HFS_IPC_CLOSE_SERVER_ACK;
            if (IPC_MSGSND(ipc_msgid, &sbuf, HFS_IPC_MSGSZ) < 0)
                perror("msgsnd");
        }
        #else
		cyg_hfs_release_resource(pServer);
        #endif
        pServer->isStart = false;
        HFS_DIAG("cyg_hfs_close ->end\r\n");
    }
    MUTEX_SIGNAL(openclose_mtx);
}

#if __USE_IPC
extern void cyg_hfs_open2(char* cmd)
{
    cyg_hfs_open_obj  hfsObj={0};
    int         portNum = 80, httpsPort = 443, threadPriority = 6,sockBufSize = 51200, chkwpd = 0, getcustom = 0, putcustom = 0,genDirList = 0;
    int         maxClientNum=0, clientQuery = 0, uploadResultCb = 0, downloadResultCb = 0, deleteResultCb = 0, timeoutCnt = 20, forceCustomCb = 0, headerCb = 0;
    int         sharedMemAddr = 0, sharedMemSize = 0;
    char        *delim=" ";
    int         interface_ver = 0;

    DBG_IND("cyg_hfs_open2 cmd = %s\r\n",cmd);

    char *test = strtok(cmd, delim);
    while (test != NULL)
    {
        // put custom callback
        if (!strcmp(test, "-put"))
        {
            putcustom = 1;
        }
        // port number
        else if (!strcmp(test, "-p"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                portNum = atoi(test);
            }
        }
        // https port
        else if (!strcmp(test, "-l"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                httpsPort = atoi(test);
            }
        }
        // thread priority
        else if (!strcmp(test, "-to"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                timeoutCnt = atoi(test);
            }
        }
        // thread priority
        else if (!strcmp(test, "-t"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                threadPriority = atoi(test);
            }
        }
        // root path
        else if (!strcmp(test, "-r"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                my_strncpy(hfsObj.rootdir, test, sizeof(hfsObj.rootdir));
            }
        }
        // socket buffer size
        else if (!strcmp(test, "-s"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                sockBufSize = atoi(test);
            }
        }
        // shared memory address
        else if (!strcmp(test, "-ma"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                sharedMemAddr = atoi(test);
            }
        }
        // shared memory size
        else if (!strcmp(test, "-ms"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                sharedMemSize = atoi(test);
            }
        }
        // check password callback
        else if (!strcmp(test, "-w"))
        {
            chkwpd = 1;
        }
        // custom string
        else if (!strcmp(test, "-cs"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                my_strncpy(hfsObj.customStr, test, sizeof(hfsObj.customStr));
            }
        }
        // get custom callback
        else if (!strcmp(test, "-c"))
        {
            getcustom = 1;
        }
        // generate dirlist callback
        else if (!strcmp(test, "-g"))
        {
            genDirList = 1;
        }
        else if (!strcmp(test, "-n"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                maxClientNum = atoi(test);
            }
        }
        // key
        else if (!strcmp(test, "-k"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                my_strncpy(hfsObj.key, test, sizeof(hfsObj.key));
            }
        }
        // client query
        else if (!strcmp(test, "-q"))
        {
            test = strtok(NULL, delim);
            if (test != NULL)
            {
                my_strncpy(hfsObj.clientQueryStr, test, sizeof(hfsObj.clientQueryStr));
                clientQuery = 1;
            }
        }
        // upload result callback
        else if (!strcmp(test, "-u"))
        {
            uploadResultCb = 1;
        }
		// download result callback
        else if (!strcmp(test, "-dl"))
        {
            downloadResultCb = 1;
        }
        // delete result callback
        else if (!strcmp(test, "-d"))
        {
            deleteResultCb = 1;
        }
        // force custom callback
        else if (!strcmp(test, "-f"))
        {
            forceCustomCb = 1;
        }
        // header callback
        else if (!strcmp(test, "-h"))
        {
            headerCb = 1;
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
    if (HFS_INTERFACE_VER != interface_ver)
    {
        DBG_ERR("version mismatch = 0x%x, 0x%x\r\n",interface_ver,HFS_INTERFACE_VER);
        return;
    }
    if (chkwpd)
    {
        hfsObj.check_pwd = hfs_ipc_check_pwd;
    }
    if (getcustom)
    {
        hfsObj.getCustomData = hfs_ipc_getCustomData;
    }
    if (putcustom)
    {
        hfsObj.putCustomData = hfs_ipc_putCustomData;
    }
    if (genDirList)
    {
        hfsObj.genDirListHtml = hfs_ipc_genDirListHtml;
    }
    if (clientQuery)
    {
        hfsObj.clientQueryCb = hfs_ipc_clientQueryCb;
    }
    if (uploadResultCb)
    {
        hfsObj.uploadResultCb = hfs_ipc_uploadResultCb;
    }
	if (downloadResultCb)
    {
        hfsObj.downloadResultCb = hfs_ipc_downloadResultCb;
    }
    if (deleteResultCb)
    {
        hfsObj.deleteResultCb = hfs_ipc_deleteResultCb;
    }
    if (headerCb)
    {
        hfsObj.headerCb = hfs_ipc_headerCb;
    }
    // set port number
    hfsObj.portNum = portNum;
    // set https port
    hfsObj.httpsPortNum = httpsPort;
    // set thread priority
    hfsObj.threadPriority = threadPriority;
    // set socket buff size
    hfsObj.sockbufSize = sockBufSize;
    hfsObj.sharedMemAddr= sharedMemAddr;
    hfsObj.sharedMemSize= sharedMemSize;
    hfsObj.maxClientNum = maxClientNum;
    hfsObj.timeoutCnt = timeoutCnt;
    hfsObj.forceCustomCallback = forceCustomCb;

    DBG_IND("hfsObj portNum= %d, threadPriority=%d, sockBufSize=%d, sharedMemAddr=0x%x, sharedMemSize=0x%x, maxClientNum=%d\r\n",portNum,threadPriority,sockBufSize,sharedMemAddr,sharedMemSize,maxClientNum);
    cyg_hfs_open(&hfsObj);
}
#endif

#if __BUILD_PROCESS

#if 0
static void sigchld_handler(void)
{
    // wait3() waits of any child
    // wait for process to change state
    // WNOHANG : This flag specifies that waitpid should return immediately instead of waiting, if there is no child process ready to be noticed.
    while (wait3(NULL, WNOHANG, NULL) > 0)
        continue;
}
#else



static void sigchld_handler(void)
{
    int    pid, status;
    while (1)
    {
        pid = waitpid (WAIT_ANY, &status, WNOHANG);
        if (pid < 0)
        {
            //perror ("waitpid");
            break;
        }
        if (pid == 0)
            break;
        #if 0
        {
            hfs_server_t *pServer = &hfs_server;

            //MUTEX_WAIT(pServer->mutex);
            hfs_session_t *search;
            search = pServer->active_sessions;
            while (search)
            {
                if (search->childpid == pid)
                {
                    search->IdleTime = hfs_get_time();
                    //DBG_DUMP("pid=%d  IdleTime = %lld \r\n",pid,search->IdleTime);
                }
                search = search->next;
            }
            //MUTEX_SIGNAL(pServer->mutex);
            //DBG_DUMP("Terminate pid =%d, status=%d\n",pid,status);
        }
        #endif
    }
}

#endif

static void dumpclient(void)
{
    hfs_server_t *pServer = &hfs_server;
    hfs_session_t *search;

    struct sockaddr *client_address;
    int calen = sizeof(*client_address);
    char name[64];
    char port[10];

    search = pServer->active_sessions;
    DBG_DUMP("Dump HFS client list, currtime=%lld\n",hfs_get_time());
    while (search)
    {
        client_address = &search->client_address;
        getnameinfo(client_address, calen, name, sizeof(name),
                        port, sizeof(port), NI_NUMERICHOST|NI_NUMERICSERV);

        DBG_DUMP("Connection from %s[%s], fd=%d, pid=%d, childpid=%d, state=%d, candelete=%d, Idletime=%lld\n",name,port,search->client_socket,search->pid,search->childpid,search->state,search->can_delete,search->IdleTime);
        search = search->next;
    }
}
static void sigroutine(int signum)
{
    switch (signum)
    {
     case SIGPIPE:
         //DBG_ERR("Get a signal -- SIGPIPE\r\n");
         break;
     case SIGCHLD:
         sigchld_handler();
         break;
     case SIGUSR1:
         dumpclient();
        break;
     default:
        DBG_ERR("Get a signal -- %d\r\n",signum);
     return;
    }
}

int main(int argc, char *argv[])
{
    cyg_hfs_open_obj  hfsObj={0};
    int               portNum = 80, httpsPort = 443,threadPriority = 6,sockBufSize = 51200, i = 0, chkwpd = 0, getcustom = 0, putcustom = 0,genDirList = 0;
    int               maxClientNum=0, clientQuery = 0, uploadResultCb = 0, downloadResultCb = 0, deleteResultCb = 0,timeoutCnt=20, forceCustomCb = 0, headerCb = 0;
    u_int32_t         sharedMemAddr = 0, sharedMemSize = 0;
    char              hfs_root[]="/home/";
    int               interface_ver = 0;
    //hfs_server_t      *pServer = &hfs_server;

    // ignore SIGPIPE
    //signal(SIGPIPE, SIG_IGN);
    signal(SIGPIPE, sigroutine);
    signal(SIGCHLD, sigroutine);
    signal(SIGUSR1, sigroutine);
    // set HFS root dir path
    my_strncpy(hfsObj.rootdir, hfs_root, sizeof(hfsObj.rootdir));

    while (i < argc)
    {
        // put custom data
        if (!strcmp(argv[i], "-put"))
        {
            putcustom = 1;
        }
        // port number
        else if (!strcmp(argv[i], "-p"))
        {
            i++;
            if (argv[i])
            {
                portNum = atoi(argv[i]);
            }
        }
        // https port
        else if (!strcmp(argv[i], "-l"))
        {
            i++;
            if (argv[i])
            {
                httpsPort = atoi(argv[i]);
            }
        }
        // thread priority
        else if (!strcmp(argv[i], "-to"))
        {
            i++;
            if (argv[i])
            {
                timeoutCnt = atoi(argv[i]);
            }
        }
        // thread priority
        else if (!strcmp(argv[i], "-t"))
        {
            i++;
            if (argv[i])
            {
                threadPriority = atoi(argv[i]);
            }
        }
        // root path
        else if (!strcmp(argv[i], "-r"))
        {
            i++;
            if (argv[i])
            {
                my_strncpy(hfsObj.rootdir, argv[i], sizeof(hfsObj.rootdir));
            }
        }
        // socket buffer size
        else if (!strcmp(argv[i], "-s"))
        {
            i++;
            if (argv[i])
            {
                sockBufSize = atoi(argv[i]);
            }
        }
        // shared memory address
        else if (!strcmp(argv[i], "-ma"))
        {
            i++;
            if (argv[i])
            {
            	int temp = atoi(argv[i]);
            	if (temp > 0)
	                sharedMemAddr = temp;
            }
        }
        // shared memory size
        else if (!strcmp(argv[i], "-ms"))
        {
            i++;
            if (argv[i])
            {
            	int temp = atoi(argv[i]);
            	if (temp > 0)
	                sharedMemSize = atoi(argv[i]);
            }
        }
        // check password callback
        else if (!strcmp(argv[i], "-w"))
        {
            chkwpd = 1;
        }
        // custom string
        else if (!strcmp(argv[i], "-cs"))
        {
            i++;
            if (argv[i])
            {
                my_strncpy(hfsObj.customStr, argv[i], sizeof(hfsObj.customStr));
            }
        }
        // get custom callback
        else if (!strcmp(argv[i], "-c"))
        {
            getcustom = 1;
        }
        // generate dirlist callback
        else if (!strcmp(argv[i], "-g"))
        {
            genDirList = 1;
        }

        else if (!strcmp(argv[i], "-n"))
        {
            i++;
            if (argv[i])
            {
                maxClientNum = atoi(argv[i]);
            }
        }
        // key
        else if (!strcmp(argv[i], "-k"))
        {
            i++;
            if (argv[i])
            {
                my_strncpy(hfsObj.key, argv[i], sizeof(hfsObj.key));
            }
        }
        else if (!strcmp(argv[i], "-q"))
        {
            i++;
            if (argv[i])
            {
                my_strncpy(hfsObj.clientQueryStr, argv[i], sizeof(hfsObj.clientQueryStr));
                clientQuery = 1;
            }
        }
        // upload result callback
        else if (!strcmp(argv[i], "-u"))
        {
            uploadResultCb = 1;
        }
		// download result callback
        else if (!strcmp(argv[i], "-dl"))
        {
            downloadResultCb = 1;
        }
        // delete result callback
        else if (!strcmp(argv[i], "-d"))
        {
            deleteResultCb = 1;
        }
        // force custom callback
        else if (!strcmp(argv[i], "-f"))
        {
            forceCustomCb = 1;
        }
        // header callback
        else if (!strcmp(argv[i], "-h"))
        {
            headerCb = 1;
        }
        // interface version
        else if (!strcmp(argv[i], "-v"))
        {
            i++;
            if (argv[i])
            {
                sscanf( argv[i], "0x%x", &interface_ver);;
                DBG_IND(" interface_ver = 0x%x\r\n",interface_ver);
            }
        }
        i++;
    }
    if (HFS_INTERFACE_VER != interface_ver)
    {
        DBG_ERR("hfs version mismatch = 0x%x, 0x%x\r\n",interface_ver,HFS_INTERFACE_VER);
        return -1;
    }
    #if __USE_IPC
    if (chkwpd)
    {
        hfsObj.check_pwd = hfs_ipc_check_pwd;
    }
    if (getcustom)
    {
        hfsObj.getCustomData = hfs_ipc_getCustomData;
    }
    if (putcustom)
    {
        hfsObj.putCustomData = hfs_ipc_putCustomData;
    }
    if (genDirList)
    {
        hfsObj.genDirListHtml = hfs_ipc_genDirListHtml;
    }
    if (clientQuery)
    {
        hfsObj.clientQueryCb = hfs_ipc_clientQueryCb;
    }
    if (uploadResultCb)
    {
        hfsObj.uploadResultCb = hfs_ipc_uploadResultCb;
    }
	if (downloadResultCb)
    {
        hfsObj.downloadResultCb = hfs_ipc_downloadResultCb;
    }
    if (deleteResultCb)
    {
        hfsObj.deleteResultCb = hfs_ipc_deleteResultCb;
    }
    if (headerCb)
    {
        hfsObj.headerCb = hfs_ipc_headerCb;
    }
    #endif

    #if CONFIG_SUPPORT_AUTH
    hfs_conf_file_parse();
    #endif
    // set port number
    hfsObj.portNum = portNum;
    // set https port
    hfsObj.httpsPortNum = httpsPort;
    // set thread priority
    hfsObj.threadPriority = threadPriority;
    // set socket buff size
    hfsObj.sockbufSize = sockBufSize;
    hfsObj.sharedMemAddr= sharedMemAddr;
    hfsObj.sharedMemSize= sharedMemSize;
    hfsObj.maxClientNum = maxClientNum;
    hfsObj.timeoutCnt = timeoutCnt;
    hfsObj.forceCustomCallback = forceCustomCb;
    #if __USE_IPC
    // start hfs daemon
    cyg_hfs_open(&hfsObj);
    sem_init(&main_sem,0,0);
    sem_wait(&main_sem);
    #else
    sem_init(&main_sem,0,0);
    sem_wait(&main_sem);
    #endif
    #if CONFIG_SUPPORT_AUTH
    hfs_conf_file_release();
    #endif
    sem_destroy(&main_sem);
    DBG_IND("exit \r\n");
    return 0;
}
#endif

/* ----------------------------------------------------------------- */
/* end of hfs.c                                                    */
