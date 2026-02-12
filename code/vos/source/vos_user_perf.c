/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#define __MODULE__    vos_user_perf
#define __DBGLVL__    2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/perf.h>
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
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_ioctl_fd = -1;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void vos_perf_init(void *param)
{
}

void vos_perf_exit(void)
{
}

static int vos_perf_get_fd(void)
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

void vos_perf_mark(VOS_TICK *p_tick)
{
	VOS_PERF_IOARG ioarg = {0};
	int fd;

	if (NULL == p_tick) {
		DBG_ERR("p_tick is NULL\r\n");
		return;
	}

	fd = vos_perf_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return;
	}

	if (-1 == ioctl(fd, VOS_PERF_IOCMD_MARK, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return;
	}

	*p_tick = ioarg.tick;
}

VOS_TICK vos_perf_duration(VOS_TICK t_begin, VOS_TICK t_end)
{
	return (t_end - t_begin);
}

void vos_perf_list_reset(void)
{
	VOS_PERF_IOARG ioarg = {0};
	int fd;

	fd = vos_perf_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return;
	}

	if (-1 == ioctl(fd, VOS_PERF_IOCMD_LIST_RESET, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return;
	}
}

void vos_perf_list_mark(const CHAR *p_name, UINT32 line_no, UINT32 cus_val)
{
	VOS_PERF_IOARG ioarg = {0};
	int fd;

	fd = vos_perf_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return;
	}

	VOS_STRCPY(ioarg.name, p_name, sizeof(ioarg.name));
	ioarg.line_no = line_no;
	ioarg.cus_val = cus_val;

	if (-1 == ioctl(fd, VOS_PERF_IOCMD_LIST_MARK, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return;
	}
}

void vos_perf_list_dump(void)
{
	VOS_PERF_IOARG ioarg = {0};
	int fd;

	fd = vos_perf_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return;
	}

	if (-1 == ioctl(fd, VOS_PERF_IOCMD_LIST_DUMP, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return;
	}
}

