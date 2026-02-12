/**
	@brief vos semaphore (virtual-os user-space)

	@file vos_user_semaphore.c

	@ingroup vos_user

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#define __MODULE__    vos_user_semaphore
#define __DBGLVL__    2
#include <kwrap/debug.h>
#include <kwrap/error_no.h>
#include <kwrap/semaphore.h>
#include "vos_ioctl.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define VOS_STRCPY(dst, src, dst_size) do { \
	strncpy(dst, src, (dst_size)-1); \
	dst[(dst_size)-1] = '\0'; \
} while(0)
/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Function Protype                                                      */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_ioctl_fd = -1;
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
static int vos_sem_get_fd(void)
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

int vos_sem_create(ID *p_semid, int init_cnt, char *name)
{
	VOS_SEM_IOARG ioarg = {0};
	int fd;

	if (NULL == p_semid) {
		DBG_ERR("p_semid is NULL\r\n");
		return -1;
	}

	if (NULL == name) {
		DBG_ERR("p_name is NULL\r\n");
		return -1;
	}

	fd = vos_sem_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return -1;
	}

	ioarg.init_cnt = init_cnt;
	VOS_STRCPY(ioarg.name, name, sizeof(ioarg.name));

	if (-1 == ioctl(fd, VOS_SEM_IOCMD_CREATE, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return -1;
	}

	*p_semid = (ID)ioarg.semid;

	return 0;
}

void vos_sem_destroy(ID semid)
{
	VOS_SEM_IOARG ioarg = {0};
	int fd;

	if (0 == semid) {
		DBG_ERR("Invalid semid %d\r\n", semid);
		return;
	}

	fd = vos_sem_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return;
	}

	ioarg.semid = semid;

	if (-1 == ioctl(fd, VOS_SEM_IOCMD_DESTROY, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return;
	}
}

void vos_sem_sig(ID semid)
{
	VOS_SEM_IOARG ioarg = {0};
	int fd;

	if (0 == semid) {
		DBG_ERR("Invalid semid %d\r\n", semid);
		return;
	}

	fd = vos_sem_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return;
	}

	ioarg.semid = semid;

	if (-1 == ioctl(fd, VOS_SEM_IOCMD_SIG, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return;
	}
}

//vos_sem_wait is identical to vos_sem_wait_interruptible,
//because we don't want Ctrl+C fails to terminate a process
int vos_sem_wait(ID semid)
{
	VOS_SEM_IOARG ioarg = {0};
	int fd;
	int ret_ioctl;

	if (0 == semid) {
		DBG_ERR("Invalid semid %d\r\n", semid);
		return -1;
	}

	fd = vos_sem_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return -1;
	}

	ioarg.semid = semid;
	ioarg.timeout_tick = -1;
	ioarg.interruptible = 1; //NOTE: for user-space, set interruptible by default

	ret_ioctl = ioctl(fd, VOS_SEM_IOCMD_WAIT, &ioarg);
	if (-1 == ret_ioctl) {
		if (errno == -E_RLWAI) {
			return E_RLWAI; //interrupted
		} else {
			DBG_ERR("ioctl fails, error = %d\r\n", errno);
			return -1;
		}
	}

	return 0;
}

int vos_sem_wait_timeout(ID semid, int timeout_tick)
{
	VOS_SEM_IOARG ioarg = {0};
	int fd;
	int ret_ioctl = 0;

	if (0 == semid) {
		DBG_ERR("Invalid semid %d\r\n", semid);
		return -1;
	}

	fd = vos_sem_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return -1;
	}

	ioarg.semid = semid;
	ioarg.timeout_tick = timeout_tick; //For Linux user-space, use timeout_tick as msec.
	ioarg.interruptible = 0;

	ret_ioctl = ioctl(fd, VOS_SEM_IOCMD_WAIT, &ioarg);

	if (-1 == ret_ioctl) {
		if (errno == -E_TMOUT) {
			return E_TMOUT; //timeout
		} else {
			DBG_ERR("ioctl fails, error = %d\r\n", errno);
			return -1;
		}
	}

	return 0;
}

int vos_sem_wait_interruptible(ID semid)
{
	VOS_SEM_IOARG ioarg = {0};
	int fd;
	int ret_ioctl;

	if (0 == semid) {
		DBG_ERR("Invalid semid %d\r\n", semid);
		return -1;
	}

	fd = vos_sem_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return -1;
	}

	ioarg.semid = semid;
	ioarg.timeout_tick = -1;
	ioarg.interruptible = 1;

	ret_ioctl = ioctl(fd, VOS_SEM_IOCMD_WAIT, &ioarg);
	if (-1 == ret_ioctl) {
		if (errno == -E_RLWAI) {
			return E_RLWAI; //interrupted
		} else {
			DBG_ERR("ioctl fails, error = %d\r\n", errno);
			return -1;
		}
	}

	return 0;
}

int vos_sem_max_cnt(void)
{
	VOS_SEM_IOARG_INFO ioarg = {0};
	int fd;
	int ret_ioctl = 0;

	fd = vos_sem_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return -1;
	}

	ret_ioctl = ioctl(fd, VOS_SEM_IOCMD_INFO, &ioarg);
	if (-1 == ret_ioctl) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return -1;
	}

	return ioarg.max_cnt;
}

int vos_sem_used_cnt(void)
{
	VOS_SEM_IOARG_INFO ioarg = {0};
	int fd;
	int ret_ioctl = 0;

	fd = vos_sem_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return -1;
	}

	ret_ioctl = ioctl(fd, VOS_SEM_IOCMD_INFO, &ioarg);
	if (-1 == ret_ioctl) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return -1;
	}

	return ioarg.used_cnt;
}