#ifndef _MSDCNVT_INT_H
#define _MSDCNVT_INT_H

#define CFG_DBG_IND    0 // ON / OFF

#if defined(WIN32)
#define DBG_DUMP printf
#define DBG_WRN(fmtstr, ...) DBG_DUMP("%s(): " fmtstr, __func__, __VA_ARGS__)
#define DBG_ERR(fmtstr, ...) DBG_DUMP("%s(): " fmtstr, __func__, __VA_ARGS__)
#define DBGD(x) DBG_DUMP("%s=%d\r\n", #x, x)
#define DBGH(x) DBG_DUMP("%s=0x%08X\r\n", #x, x)
#define CHKPNT DBG_DUMP("CHK: %d, %s\r\n", __LINE__, __func__)
#if CFG_DBG_IND
#define DBG_IND(fmtstr, ...) DBG_DUMP("%s(): " fmtstr, __func__, __VA_ARGS__)
#else
#define DBG_IND(fmtstr, ...)
#endif
#elif defined(__LINUX)
#define DBG_DUMP printf
#define DBG_WRN(fmtstr, args...) DBG_DUMP("\033[33m%s(): \033[0m" fmtstr, __func__, ##args)
#define DBG_ERR(fmtstr, args...) DBG_DUMP("\033[31m%s(): \033[0m" fmtstr, __func__, ##args)
#define DBGD(x) DBG_DUMP("\033[0;35m%s=%d\033[0m\r\n", #x, x)
#define DBGH(x) DBG_DUMP("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define CHKPNT DBG_DUMP("\033[37mCHK: %d, %s\033[0m\r\n", __LINE__, __func__)
#if CFG_DBG_IND
#define DBG_IND(fmtstr, args...) DBG_DUMP("%s(): " fmtstr, __func__, ##args)
#else
#define DBG_IND(fmtstr, ...)
#endif
#define closesocket close
typedef int BOOL;
#elif defined(__ECOS)
#define DBG_DUMP diag_printf
#define DBG_WRN(fmtstr, args...) DBG_DUMP("%s(): " fmtstr, __func__, ##args)
#define DBG_ERR(fmtstr, args...) DBG_DUMP("%s(): " fmtstr, __func__, ##args)
#define DBGD(x) DBG_DUMP("%s=%d\r\n", #x, x)
#define DBGH(x) DBG_DUMP("%s=0x%08X\r\n", #x, x)
#define CHKPNT DBG_DUMP("CHK: %d, %s\r\n", __LINE__, __func__)
#if CFG_DBG_IND
#define DBG_IND(fmtstr, args...) DBG_DUMP("%s(): " fmtstr, __func__, ##args)
#else
#define DBG_IND(fmtstr, ...)
#endif
#define closesocket close
#endif

#if defined(__ECOS)
#include <cyg/kernel/kapi.h>
#define THREAD_DECLARE(name, arglist) static void  name (cyg_addrword_t arglist)
#define THREAD_HANDLE cyg_handle_t
#define THREAD_OBJ cyg_thread
#define THREAD_CREATE(name, thread_handle, fp, p_data, p_stack, size_stack, p_thread_obj)  cyg_thread_create(22, fp, (cyg_addrword_t)p_data, name, (void *)p_stack, size_stack, &thread_handle, p_thread_obj)
#define THREAD_RESUME(thread_handle) cyg_thread_resume(thread_handle)
#define THREAD_DESTROY(thread_handle) cyg_thread_suspend(thread_handle); while (!cyg_thread_delete(thread_handle)) { cyg_thread_delay(1); }
#define THREAD_RETURN return
#define THREAD_EXIT() cyg_thread_exit()
#define SEM_HANDLE cyg_sem_t
#define SEM_CREATE(handle, init_cnt) cyg_semaphore_init(&handle, init_cnt)
#define SEM_SIGNAL(handle) cyg_semaphore_post(&handle)
#define SEM_WAIT(handle) cyg_semaphore_wait(&handle);
#define SEM_DESTROY(handle) cyg_semaphore_destroy(&handle)
#elif defined(WIN32)
#define THREAD_DECLARE(name, arglist) static DWORD WINAPI name (LPVOID arglist)
#define THREAD_HANDLE HANDLE
#define THREAD_OBJ DWORD
#define THREAD_CREATE(name, thread_handle, fp, p_data, p_stack, size_stack, p_thread_obj)  thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)fp, (LPVOID)p_data, 0, p_thread_obj)
#define THREAD_RESUME(thread_handle)
#define THREAD_DESTROY(thread_handle) TerminateThread(thread_handle, 0);
#define THREAD_RETURN return 0
#define THREAD_EXIT()  ExitThread(0)
#define SEM_HANDLE HANDLE
#define SEM_CREATE(handle, init_cnt) handle = CreateSemaphore(NULL, init_cnt, 1, NULL)
#define SEM_SIGNAL(handle) ReleaseSemaphore(handle, 1, NULL)
#define SEM_WAIT(handle) WaitForSingleObject(handle, INFINITE);
#define SEM_DESTROY(handle) CloseHandle(handle)
#elif defined(__LINUX)
#include <pthread.h>
#define THREAD_DECLARE(name, arglist) static void *name (void *arglist)
#define THREAD_HANDLE pthread_t
#define THREAD_OBJ int
#define THREAD_CREATE(name, thread_handle, fp, p_data, p_stack, size_stack, p_thread_obj)  pthread_create(&thread_handle, NULL, fp, p_data)
#define THREAD_RESUME(thread_handle)
#define THREAD_DESTROY(thread_handle) pthread_cancel(thread_handle);
#define THREAD_RETURN return NULL
#define THREAD_EXIT()  pthread_exit(0)
#define SEM_HANDLE pthread_mutex_t
#define SEM_CREATE(handle, init_cnt) pthread_mutex_init(&handle, NULL); if (init_cnt == 0)pthread_mutex_lock(&handle)
#define SEM_SIGNAL(handle) pthread_mutex_unlock(&handle)
#define SEM_WAIT(handle) pthread_mutex_lock(&handle);
#define SEM_DESTROY(handle) pthread_mutex_destroy(&handle)
#endif

#if defined(__PURE_LINUX)
#define CFG_VIA_IOCTL 1
#define CFG_VIA_IPC 0
#elif defined(__ECOS660) || defined(__LINUX)
#define CFG_VIA_IOCTL 0
#define CFG_VIA_IPC 1
#else
#define CFG_VIA_IOCTL 0
#define CFG_VIA_IPC 0
#endif

#define ALIGN_FLOOR(value, base)  ((value) & ~((base)-1))                   ///< Align Floor
#define ALIGN_ROUND(value, base)  ALIGN_FLOOR((value) + ((base)/2), base)   ///< Align Round
#define ALIGN_CEIL(value, base)   ALIGN_FLOOR((value) + ((base)-1), base)   ///< Align Ceil

#define MSDCNVT_ASSERT2(e, func) \
	((void)DBG_DUMP("ASSERT FAIL:%s() %s\r\n", func, e), 0)
#define MSDCNVT_ASSERT(e) \
	((void) ((e) ? 0 : MSDCNVT_ASSERT2 (#e, __func__)))

#endif
