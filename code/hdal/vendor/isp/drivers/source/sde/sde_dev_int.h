#ifndef _SDE_DEV_INT_H_
#define _SDE_DEV_INT_H_

#if defined(__FREERTOS)
#else
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/wait.h>
#endif

//#include "sde_dev.h"

//=============================================================================
// struct & definition
//=============================================================================
#define SDE_MAX_NAME_SIZE       16
#define FPS_TIMER_INTERVAL      3       // sec
#define KTHREAD_EVENT_TIME_OUT  600     // ms
#define IOCTL_BUF_SIZE          8192+2  // To match SDET_MEMORY_INFO

typedef struct _SDE_DEV_INFO {
	#if defined(__FREERTOS)
	#else
	// local variable
	UINT32 ioctl_buf[IOCTL_BUF_SIZE];
	// synchronization mechanism
	struct semaphore api_mutex;
	struct semaphore ioc_mutex;
	struct semaphore proc_mutex;
	#endif
} SDE_DEV_INFO;

typedef struct _SDE_MODULE {
	CHAR     name[SDE_MAX_NAME_SIZE];        // module name
	void     *private;                       // private date
} SDE_MODULE;

//=============================================================================
// extern functions
//=============================================================================
#if defined(__FREERTOS)
extern SDE_DEV_INFO *sde_get_dev_info(void);
#endif
extern INT32 sde_dev_construct(SDE_DEV_INFO *pdev_info);
extern void sde_dev_deconstruct(SDE_DEV_INFO *pdev_info);
#endif

extern BOOL sde_dev_get_id_valid(UINT32 id);

