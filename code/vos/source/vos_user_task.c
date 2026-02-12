/**
	@brief vos task (virtual-os user-space)

	@file vos_user_task.c

	@ingroup vos_user

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#define _GNU_SOURCE /* define this for pthread_setname_np */

#define __MODULE__    vos_user_task
#define __DBGLVL__    2
#include <kwrap/debug.h>
#include <kwrap/task.h>
#include "vos_ioctl.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h> /* to get PTHREAD_STACK_MIN */
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define VOS_STRCPY(dst, src, dst_size) do { \
	strncpy(dst, src, (dst_size)-1); \
	dst[(dst_size)-1] = '\0'; \
} while(0)

#ifndef MIN_NICE
#define MIN_NICE -20
#endif

#define USER_NICE_TO_VOS_PRIO(nice) ((nice) - MIN_NICE + VK_TASK_HIGHEST_PRIORITY)

typedef void *(*USER_FP) (void *);
/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_ioctl_fd = -1;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
static int vos_task_get_fd(void)
{
	pthread_mutex_lock(&g_mutex);

	if (g_ioctl_fd < 0) {
		g_ioctl_fd = open("/dev/"VOS_IOCTL_DEV_NAME, O_RDWR);
		if (g_ioctl_fd < 0) {
			DBG_ERR("open %s failed", "/dev/"VOS_IOCTL_DEV_NAME);
			pthread_mutex_unlock(&g_mutex);
			return -1;
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return g_ioctl_fd;
}

static int vos_task_check_priority(int priority)
{
	STATIC_ASSERT(VK_TASK_LOWEST_PRIORITY >= VK_TASK_HIGHEST_PRIORITY);

	if (priority > VK_TASK_LOWEST_PRIORITY || priority < VK_TASK_HIGHEST_PRIORITY) {
		DBG_ERR("Invalid %d, Lowest(%d) ~ Highest(%d)\r\n", priority, VK_TASK_LOWEST_PRIORITY, VK_TASK_HIGHEST_PRIORITY);
		return -1;
	}

	return 0;
}

static void* vos_task_run_func(void *param)
{
	VOS_TASK_IOARG_REG_N_RUN ioarg = {0};
	int fd;
	int oldtype = 0;
	int pthread_ret;

	fd = vos_task_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return 0;
	}

	//VOS_TASK_IOCMD_REG_N_RUN will do:
	//1. register tid to vos driver
	//2. set priority (nice value)
	//3. get user_fp and user_parm to run
	ioarg.pthread_id = (unsigned long)pthread_self();
	ioarg.drv_task_id = (int)((long)param);
	ioarg.tid = (unsigned long)syscall(__NR_gettid);
	if (-1 == ioctl(fd, VOS_TASK_IOCMD_REG_N_RUN, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return NULL;
	}

	//set pthread other attributes
	pthread_ret = pthread_setname_np(ioarg.pthread_id, ioarg.name);
	if (0 != pthread_ret) {
		DBG_WRN("%s set comm name failed, ret %d\r\n", ioarg.name, pthread_ret);
	}

	//set thread type can be canceled immediately,
	//combine this type with xxx_interruptible to let threads can be terminated cleanly
	if (0 != pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype)) {
		DBG_WRN("setcanceltype failed\r\n");
	}

	//run user function
	return ((USER_FP)ioarg.user_fp)(ioarg.user_parm);
}

void vos_task_set_priority(VK_TASK_HANDLE task_hdl, int priority)
{
	VOS_TASK_IOARG_PRIORITY ioarg = {0};
	int fd;

	if (0 != vos_task_check_priority(priority)) {
		DBG_ERR("skip, task_hdl 0x%X\r\n", (unsigned int)task_hdl);
		return;
	}

	fd = vos_task_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return;
	}

	ioarg.drv_task_id = (int)task_hdl;
	ioarg.vos_prio = priority;
	if (-1 == ioctl(fd, VOS_TASK_IOCMD_SET_PRIORITY, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return;
	}
}

VK_TASK_HANDLE vos_task_create(void *fp, void *parm, const char name[], int priority, int stksize)
{
	VOS_TASK_IOARG_UINFO uinfo = {0};
	int fd;

	// if priority is not set, mapping it to nice(0)
	if (0 == priority) {
		priority = USER_NICE_TO_VOS_PRIO(0);
	}

	if (0 != vos_task_check_priority(priority)) {
		DBG_ERR("check_priority %d failed\n", priority);
		return 0;
	}

	if (NULL == fp) {
		DBG_ERR("fp is NULL\r\n");
		return 0;
	}

	fd = vos_task_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return 0;
	}

	if (stksize < PTHREAD_STACK_MIN) {
		stksize = PTHREAD_STACK_MIN;
	}

	VOS_STRCPY(uinfo.name, name, sizeof(uinfo.name));
	uinfo.drv_task_id = -1; // drv_task_id -1 to get a new task id and set user info
	uinfo.vos_prio = priority;
	uinfo.user_fp = fp;
	uinfo.user_parm = parm;
	uinfo.user_stksize = stksize;
	if (-1 == ioctl(fd, VOS_TASK_IOCMD_SET_UINFO, &uinfo)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return 0;
	}

	if (uinfo.drv_task_id < 0) {
		DBG_ERR("Invalid drv_task_id %d\r\n", uinfo.drv_task_id);
		return 0;
	}

	return (VK_TASK_HANDLE)uinfo.drv_task_id;
}

int vos_task_resume(VK_TASK_HANDLE task_hdl)
{
	VOS_TASK_IOARG_UINFO uinfo = {0};
	VOS_TASK_IOARG ioarg = {0};
	pthread_attr_t attr = {0};
	pthread_t pthread_id;
	int pthread_ret;
	int fd;

	fd = vos_task_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return -1;
	}

	uinfo.drv_task_id = (int)task_hdl; // drv_task_id > 0 to get user info
	if (-1 == ioctl(fd, VOS_TASK_IOCMD_GET_UINFO, &uinfo)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return -1;
	}

	pthread_ret = pthread_attr_init(&attr);
	if (0 != pthread_ret) {
		DBG_ERR("%s attr_init failed, ret %d\r\n", uinfo.name, pthread_ret);
		return -1;
	}

	pthread_ret = pthread_attr_setstacksize (&attr, uinfo.user_stksize);
	if (0 != pthread_ret) {
		DBG_ERR("%s setstacksize failed, ret %d\r\n", uinfo.name, pthread_ret);
		return -1;
	}

	pthread_ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (0 != pthread_ret) {
		DBG_ERR("%s setdetachstate failed, ret %d\r\n", uinfo.name, pthread_ret);
		return -1;
	}

	pthread_ret = pthread_create(&pthread_id, &attr, vos_task_run_func, (void *)((long)uinfo.drv_task_id));
	if (0 != pthread_ret) {
		DBG_ERR("%s create failed, ret %d\r\n", uinfo.name, pthread_ret);
		return -1;
	}

	pthread_ret = pthread_attr_destroy(&attr);
	if (0 != pthread_ret) {
		DBG_ERR("%s attr_destroy failed, ret %d\r\n", uinfo.name, pthread_ret);
		//do not return NULL, since it is created
	}

	//Note: We have to register pthread_id right away,
	// because users may call vos_task_set_priority which needs pthread_id to convert handles
	ioarg.pthread_id = (unsigned long)pthread_id;
	ioarg.drv_task_id = (int)task_hdl;
	if (-1 == ioctl(fd, VOS_TASK_IOCMD_RESUME, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		//do not return NULL, since it is created
	}

	return 0;
}

void vos_task_destroy(VK_TASK_HANDLE task_hdl)
{
	VOS_TASK_IOARG ioarg = {0};
	int fd;

	if (0 == task_hdl) {
		DBG_ERR("Invalid task_hdl 0x%lX\r\n", (ULONG)task_hdl);
		return;
	}

	fd = vos_task_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return;
	}

	ioarg.drv_task_id = (int)task_hdl;
	if (-1 == ioctl(fd, VOS_TASK_IOCMD_DESTROY, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
	}

	if (ioarg.pthread_id) {
		pthread_cancel((pthread_t)ioarg.pthread_id);
	}
}

VK_TASK_HANDLE vos_task_get_handle(void)
{
	VOS_TASK_IOARG ioarg = {0};
	int fd;

	fd = vos_task_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return 0;
	}

	ioarg.pthread_id = (unsigned long)pthread_self();
	if (-1 == ioctl(fd, VOS_TASK_IOCMD_CONVERT_HDL, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return 0;
	}

	return (VK_TASK_HANDLE)ioarg.drv_task_id;
}

int vos_task_get_name(VK_TASK_HANDLE task_hdl, char *name, unsigned int len)
{
	VOS_TASK_IOARG ioarg = {0};
	int fd;
	int pthread_ret;

	if (0 == task_hdl) {
		DBG_ERR("Invalid task_hdl 0x%lX\r\n", (ULONG)task_hdl);
		return -1;
	}

	if (NULL == name) {
		return -1;
	}

	fd = vos_task_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return -1;
	}

	ioarg.drv_task_id = (int)task_hdl;
	if (-1 == ioctl(fd, VOS_TASK_IOCMD_CONVERT_HDL, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return -1;
	}

	pthread_ret = pthread_getname_np(ioarg.pthread_id, name, len);
	if (0 != pthread_ret) {
		DBG_ERR("0x%lX get_name failed, ret %d\r\n", (ULONG)task_hdl, pthread_ret);
		return -1;
	}

	return 0;
}

void vos_task_enter(void)
{
}

int vos_task_return(int rtn_val)
{
	VOS_TASK_IOARG ioarg = {0};
	int fd;

	fd = vos_task_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return 0;
	}

	ioarg.pthread_id = (unsigned long)pthread_self();
	if (-1 == ioctl(fd, VOS_TASK_IOCMD_RETURN, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
	}

	pthread_exit(NULL);
	return 0;
}

int vos_task_max_cnt(void)
{
	VOS_TASK_IOARG_INFO ioarg = {0};
	int fd;
	int ret_ioctl = 0;

	fd = vos_task_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return -1;
	}

	ret_ioctl = ioctl(fd, VOS_TASK_IOCMD_INFO, &ioarg);
	if (-1 == ret_ioctl) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return -1;
	}

	return ioarg.max_cnt;
}

int vos_task_used_cnt(void)
{
	VOS_TASK_IOARG_INFO ioarg = {0};
	int fd;
	int ret_ioctl = 0;

	fd = vos_task_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return -1;
	}

	ret_ioctl = ioctl(fd, VOS_TASK_IOCMD_INFO, &ioarg);
	if (-1 == ret_ioctl) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return -1;
	}

	return ioarg.used_cnt;
}