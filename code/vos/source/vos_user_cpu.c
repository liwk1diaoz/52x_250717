/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#define __MODULE__    vos_user_cpu
#define __DBGLVL__    2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/cpu.h>
#include "vos_ioctl.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define RTOS_CPU_INITED_TAG       MAKEFOURCC('R', 'C', 'P', 'U') ///< a key value

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_ioctl_fd = -1;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
static int vos_cpu_get_fd(void)
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

void rtos_cpu_init(void *param)
{
}

void rtos_cpu_exit(void)
{
}

void vos_cpu_dcache_sync(VOS_ADDR vaddr, UINT32 len, VOS_DMA_DIRECTION dir)
{
	VOS_CPU_IOARG ioarg = {0};
	int ret_ioctl = 0;
	int fd;

	fd = vos_cpu_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return;
	}

	ioarg.vaddr = (unsigned long)vaddr;
	ioarg.len = (unsigned long)len;
	ioarg.dir = (unsigned int)dir;
	ioarg.is_vb = 0;

	ret_ioctl = ioctl(fd, VOS_CPU_IOCMD_DCACHE_SYNC, &ioarg);
	if (ret_ioctl == -1) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return;
	}
}

void vos_cpu_dcache_sync_vb(VOS_ADDR vaddr, UINT32 len, VOS_DMA_DIRECTION dir)
{
	VOS_CPU_IOARG ioarg = {0};
	int ret_ioctl = 0;
	int fd;

	fd = vos_cpu_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return;
	}

	ioarg.vaddr = (unsigned long)vaddr;
	ioarg.len = (unsigned long)len;
	ioarg.dir = (unsigned int)dir;
	ioarg.is_vb = 1;

	ret_ioctl = ioctl(fd, VOS_CPU_IOCMD_DCACHE_SYNC, &ioarg);
	if (ret_ioctl == -1) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return;
	}
}

int vos_cpu_dcache_sync_by_cpu(VOS_ADDR vaddr, UINT32 len, VOS_DMA_DIRECTION dir, UINT cache_op_cpu_id)
{
	VOS_CPU_IOARG_SYNC_CPU ioarg = {0};
	int ret_ioctl = 0;
	int fd;

	fd = vos_cpu_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return -1;
	}

	ioarg.vaddr = (unsigned long)vaddr;
	ioarg.len = (unsigned long)len;
	ioarg.dir = (unsigned int)dir;
	ioarg.cpu_id = (unsigned int)cache_op_cpu_id;

	ret_ioctl = ioctl(fd, VOS_CPU_IOCMD_SYNC_CPU, &ioarg);
	if (ret_ioctl == -1) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return -1;
	}

	return 0;
}

#if 0 //default is 0, set 1 for debug. Set disable to prevent user-space issues.
VOS_ADDR vos_cpu_get_phy_addr(VOS_ADDR vaddr)
{
	VOS_CPU_IOARG_VA_PA ioarg = {0};
	int ret_ioctl = 0;
	int fd;

	fd = vos_cpu_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return VOS_ADDR_INVALID;
	}

	ioarg.vaddr = (unsigned long)vaddr;

	ret_ioctl = ioctl(fd, VOS_CPU_IOCMD_GET_PHY_ADDR, &ioarg);
	if (ret_ioctl == -1) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return VOS_ADDR_INVALID;
	}

	return (VOS_ADDR)ioarg.paddr;
}
#endif

