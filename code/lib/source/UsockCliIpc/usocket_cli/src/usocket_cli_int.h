#ifndef _USOCKETCLI_INT_H_
#define _USOCKETCLI_INT_H_

#include <sys/types.h>
#include <sys/stat.h>
#if defined(__FREERTOS)
#undef LWIP_POSIX_SOCKETS_IO_NAMES
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


#if 0 //debug
#if defined(__ECOS)
#define USOCKETCLI_PRINT(fmtstr, args...) diag_printf("eCoooooos: %s() " fmtstr, __func__, ##args)
#else
#define USOCKETCLI_PRINT(fmtstr, args...) printf("usocket: %s() " fmtstr, __func__, ##args)
#endif
#else //release
#define USOCKETCLI_PRINT(...)
#endif

#if defined(__ECOS)
#define USOCKETCLI_ERR(fmtstr, args...) diag_printf("eCoooooos: ERR %s() " fmtstr, __func__, ##args)
#else
#define USOCKETCLI_ERR(fmtstr, args...) printf("usocket: ERR %s() " fmtstr, __func__, ##args)
#endif



#if defined(__USE_IPC)
#define ipc_isCacheAddr(addr)       ((((addr) & 0x20000000) == 0x20000000) ? FALSE : TRUE)
#define ipc_getPhyAddr(addr)        ((addr) & 0x1FFFFFFF)
#define ipc_getNonCacheAddr(addr)   (((addr) & 0x1FFFFFFF)|0xA0000000)
#else
#define ipc_getPhyAddr(addr)        NvtMem_GetPhyAddr(addr)
#define ipc_getNonCacheAddr(addr)   NvtMem_GetNonCacheAddr(addr)
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
#define MUTEX_CREATE(handle,init_cnt) pthread_mutex_init(&handle,NULL);if(init_cnt==0)pthread_mutex_lock(&handle)
#define MUTEX_SIGNAL(handle) pthread_mutex_unlock(&handle)
#define MUTEX_WAIT(handle) pthread_mutex_lock(&handle);
#define MUTEX_DESTROY(handle) pthread_mutex_destroy(&handle)
#define COND_HANDLE pthread_cond_t
#define COND_CREATE(handle) pthread_cond_init(&handle,NULL)
#define COND_SIGNAL(handle) pthread_cond_broadcast(&handle)
#define COND_WAIT(handle,mtx) pthread_cond_wait(&handle, &mtx)
#define COND_DESTROY(handle) pthread_cond_destroy(&handle)
#define FLAG_SETPTN(cond,flag,ptn,mtx) {pthread_mutex_lock(&mtx);flag|=ptn;USOCKETCLI_PRINT("setflag 0x%x, ptn 0x%x\r\n",flag,ptn);pthread_cond_broadcast(&cond);pthread_mutex_unlock(&mtx);}
#define FLAG_CLRPTN(flag,ptn,mtx) {pthread_mutex_lock(&mtx);flag &= ~(ptn);USOCKETCLI_PRINT("clrflag 0x%x, ptn 0x%x\r\n",flag,ptn);pthread_mutex_unlock(&mtx);}
#define FLAG_WAITPTN(cond,flag,ptn,mtx,clr) {pthread_mutex_lock(&mtx);while (1) { USOCKETCLI_PRINT("waitflag b 0x%x, ptn 0x%x\r\n",flag,ptn);\
                                                                    if (flag & ptn){if (clr) {flag &= ~(ptn);}pthread_mutex_unlock(&mtx);USOCKETCLI_PRINT("waitflag e 0x%x, ptn 0x%x\r\n",flag,ptn);break;}\
                                                                  pthread_cond_wait(&cond, &mtx);\
                                                                  if (flag & ptn) {\
                                                                  if (clr) {flag &= ~(ptn);}\
																  pthread_mutex_unlock(&mtx);\
																  USOCKETCLI_PRINT("waitflag e 0x%x, ptn 0x%x\r\n",flag,ptn);\
                                                                  break;}\
                                                                  }}

#define FLAG_WAITPTN_TIMEOUT(cond,flag,ptn,mtx,clr,timeout,rtn){struct timespec to;int err;rtn=0;\
                                                         pthread_mutex_lock(&mtx);\
                                                         while (1) { USOCKETCLI_PRINT("waitflag 0x%x\r\n",flag);\
	                                                               if (flag & ptn){if (clr) {flag &= ~(ptn);}pthread_mutex_unlock(&mtx);break;}\
                                                                   USOCKETCLI_PRINT("cond_wait 0x%x\r\n",flag);\
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

#endif //_USOCKETCLI_DEBUG_H_
