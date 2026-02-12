/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#define __MODULE__    vos_user_debug
#define __DBGLVL__    2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
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
static int vos_debug_get_fd(void)
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

void vos_debug_halt(void)
{
	VOS_DEBUG_IOARG ioarg = {0};
	int fd;

	fd = vos_debug_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return;
	}

	if (-1 == ioctl(fd, VOS_DEBUG_IOCMD_HALT, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return;
	}
}

