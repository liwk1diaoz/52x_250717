/**
	@brief vos flag (virtual-os user-space)

	@file vos_user_flag.c

	@ingroup vos_user

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#define __MODULE__    vos_user_flag
#define __DBGLVL__    2
#include <kwrap/debug.h>
#include <kwrap/flag.h>
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
typedef struct {
	int ioctl_fd;
} _flag_ctrl_t;

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_ioctl_fd = -1;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
static int vos_flag_get_fd(void)
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

ER vos_flag_create(ID *p_flgid, T_CFLG *p_cflg, char *p_name)
{
	VOS_FLAG_IOARG ioarg = {0};
	int fd;

	if (NULL == p_flgid) {
		DBG_ERR("p_flgid is NULL\r\n");
		return E_PAR;
	}

	if (NULL == p_name) {
		DBG_ERR("p_name is NULL\r\n");
		return E_PAR;
	}

	fd = vos_flag_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return E_SYS;
	}

	if (p_cflg) {
		ioarg.bits = (unsigned long)p_cflg->iflgptn;
	} else {
		ioarg.bits = 0;
	}

	VOS_STRCPY(ioarg.name, p_name, sizeof(ioarg.name));

	if (-1 == ioctl(fd, VOS_FLAG_IOCMD_CREATE, &ioarg)) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return E_SYS;
	}

	*p_flgid = (ID)ioarg.id;

	return E_OK;
}

ER vos_flag_destroy(ID flgid)
{
	ER stReslut = E_OK;
	int ret_ioctl = 0;
	VOS_FLAG_IOARG ioarg = {0};
	int fd;

	fd = vos_flag_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return E_SYS;
	}

	ioarg.id = (unsigned long)flgid;

	ret_ioctl = ioctl(fd, VOS_FLAG_IOCMD_DESTROY, &ioarg);
	if (ret_ioctl == -1) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		stReslut = E_SYS;
	}

	return stReslut;
}

ER vos_flag_set(ID flgid, FLGPTN setptn)
{
	ER stReslut = E_OK;
	int ret_ioctl = 0;
	VOS_FLAG_IOARG ioarg = {0};
	int fd;

	fd = vos_flag_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return E_SYS;
	}

	ioarg.id = (unsigned long)flgid;
	ioarg.bits = (unsigned long)setptn;

	ret_ioctl = ioctl(fd, VOS_FLAG_IOCMD_SET, &ioarg);
	if (ret_ioctl == -1) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		stReslut = E_SYS;
	}

	return stReslut;
}

ER vos_flag_clr(ID flgid, FLGPTN clrptn)
{
	ER stReslut = E_OK;
	int ret_ioctl = 0;
	VOS_FLAG_IOARG ioarg = {0};
	int fd;

	fd = vos_flag_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return E_SYS;
	}

	ioarg.id = (unsigned long)flgid;
	ioarg.bits = (unsigned long)clrptn;

	ret_ioctl = ioctl(fd, VOS_FLAG_IOCMD_CLR, &ioarg);
	if (ret_ioctl == -1) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		stReslut = E_SYS;
	}

	return stReslut;
}

//vos_flag_wait is identical to vos_flag_wait_interruptible,
//because we don't want Ctrl+C fails to terminate a process
ER vos_flag_wait(PFLGPTN p_flgptn, ID flgid, FLGPTN waiptn, UINT wfmode)
{
	int ret_ioctl = 0;
	VOS_FLAG_IOARG ioarg = {0};
	int fd;

	fd = vos_flag_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return E_SYS;
	}

	ioarg.id = (unsigned long)flgid;
	ioarg.bits = (unsigned long)waiptn;
	ioarg.mode = (unsigned long)wfmode;
	ioarg.timeout_tick = -1;
	ioarg.interruptible = 1; //NOTE: for user-space, set interruptible by default

	ret_ioctl = ioctl(fd, VOS_FLAG_IOCMD_WAIT, &ioarg);
	if (-1 == ret_ioctl) {
		if (errno == -E_RLWAI) {
			return E_RLWAI; //interrupted
		} else {
			DBG_ERR("ioctl fails, error = %d\r\n", errno);
			return E_SYS;
		}
	}

	*p_flgptn = (FLGPTN)ioarg.bits;

	return E_OK;
}

ER vos_flag_wait_timeout(PFLGPTN p_flgptn, ID flgid, FLGPTN waiptn, UINT wfmode, int timeout)
{
	int ret_ioctl = 0;
	VOS_FLAG_IOARG ioarg = {0};
	int fd;

	fd = vos_flag_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return E_SYS;
	}

	ioarg.id = (unsigned long)flgid;
	ioarg.bits = (unsigned long)waiptn;
	ioarg.mode = (unsigned long)wfmode;
	ioarg.timeout_tick = timeout;
	ioarg.interruptible = 0;

	ret_ioctl = ioctl(fd, VOS_FLAG_IOCMD_WAIT, &ioarg);

	if (-1 == ret_ioctl) {
		if (errno == -E_TMOUT) {
			return E_TMOUT; //timeout
		} else {
			DBG_ERR("ioctl fails, error = %d\r\n", errno);
			return E_SYS;
		}
	}

	*p_flgptn = (FLGPTN)ioarg.bits;

	return E_OK;
}

ER vos_flag_wait_interruptible(PFLGPTN p_flgptn, ID flgid, FLGPTN waiptn, UINT wfmode)
{
	int ret_ioctl = 0;
	VOS_FLAG_IOARG ioarg = {0};
	int fd;

	fd = vos_flag_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return E_SYS;
	}

	ioarg.id = (unsigned long)flgid;
	ioarg.bits = (unsigned long)waiptn;
	ioarg.mode = (unsigned long)wfmode;
	ioarg.timeout_tick = -1;
	ioarg.interruptible = 1;

	ret_ioctl = ioctl(fd, VOS_FLAG_IOCMD_WAIT, &ioarg);
	if (-1 == ret_ioctl) {
		if (errno == -E_RLWAI) {
			return E_RLWAI; //interrupted
		} else {
			DBG_ERR("ioctl fails, error = %d\r\n", errno);
			return E_SYS;
		}
	}

	*p_flgptn = (FLGPTN)ioarg.bits;

	return E_OK;
}

FLGPTN vos_flag_chk(ID flgid, FLGPTN chkptn)
{
	int ret_ioctl = 0;
	VOS_FLAG_IOARG ioarg = {0};
	int fd;

	fd = vos_flag_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return E_SYS;
	}

	ioarg.id = (unsigned long)flgid;
	ioarg.bits = (unsigned long)chkptn;

	ret_ioctl = ioctl(fd, VOS_FLAG_IOCMD_CHK, &ioarg);
	if (ret_ioctl == -1) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return 0;
	}

	return (FLGPTN)ioarg.bits;
}

int vos_flag_max_cnt(void)
{
	VOS_FLAG_IOARG_INFO ioarg = {0};
	int fd;
	int ret_ioctl = 0;

	fd = vos_flag_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return -1;
	}

	ret_ioctl = ioctl(fd, VOS_FLAG_IOCMD_INFO, &ioarg);
	if (-1 == ret_ioctl) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return -1;
	}

	return ioarg.max_cnt;
}

int vos_flag_used_cnt(void)
{
	VOS_FLAG_IOARG_INFO ioarg = {0};
	int fd;
	int ret_ioctl = 0;

	fd = vos_flag_get_fd();
	if (fd < 0) {
		DBG_ERR("get fd failed\r\n");
		return -1;
	}

	ret_ioctl = ioctl(fd, VOS_FLAG_IOCMD_INFO, &ioarg);
	if (-1 == ret_ioctl) {
		DBG_ERR("ioctl fails, error = %d\r\n", errno);
		return -1;
	}

	return ioarg.used_cnt;
}