/**
	@brief vos mailbox (virtual-os user-space)

	@file vos_user_mailbox.c

	@ingroup vos_user

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#define __MODULE__    vos_user_mailbox
#define __DBGLVL__    7
#include <kwrap/debug.h>
#include <kwrap/mailbox.h>

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <unistd.h>

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define VOS_MBX_MIN_VALID_ID    1
#define VOS_MBX_DEV_PATH        "/dev/RTOS_MBox"
#define VOS_MBX_MQUEUE_NAME     "/vos_mqueue_name"
#define VOS_MBX_MSG_PRIO        0
#define RTOS_MBX_INITED_TAG     MAKEFOURCC('R', 'M', 'B', 'X') ///< a key value
#define _BUFFER_SIZE_           32
#define VOS_MBX_USE_IOCTL       0
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
typedef struct {
	int st_used;
	mqd_t st_mqd;
	UINT st_msgsize;
} MBX_CELL_T;

typedef struct {
	int ioctl_fd;
	MBX_CELL_T *p_cell;
} MBX_CTRL_T;

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
UINT g_max_mbxid_num = 0;
UINT g_cur_mbxid = 0;

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static MBX_CTRL_T g_ctrl = {.ioctl_fd = -1, .p_cell = NULL};
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
ER vos_mbx_init(UINT max_mbxid_num)
{
	ER ret = E_OK;

	pthread_mutex_lock(&g_mutex);

	if (g_ctrl.ioctl_fd >= 0) {
		DBG_WRN("Already initialized, please call vos_mbx_exit first\r\n");
		ret = E_OK;
		goto vos_mbx_init_end; //already initialized
	}

	if (!max_mbxid_num) {
		DBG_ERR("Invalid param, max_mbxid_num %d\r\n", max_mbxid_num);
		ret = E_PAR;
		goto vos_mbx_init_end;
	}

	g_ctrl.p_cell = calloc(1, max_mbxid_num * sizeof(MBX_CELL_T));
	if (NULL == g_ctrl.p_cell) {
		DBG_ERR("get memory failed\r\n");
		ret = E_NOMEM;
		goto vos_mbx_init_end;
	}

#if VOS_MBX_USE_IOCTL
	g_ctrl.ioctl_fd = open(VOS_MBX_DEV_PATH, O_RDWR);
	if (-1 == g_ctrl.ioctl_fd) {
		DBG_ERR("open %s failed\r\n", VOS_MBX_DEV_PATH);
		ret = E_SYS;
		goto vos_mbx_init_end;
	}
#else
	g_ctrl.ioctl_fd = 0; //set 0 to pretend initialized
#endif

	g_max_mbxid_num = max_mbxid_num;

	DBG_IND("g_max_mbxid_num %d\r\n", g_max_mbxid_num);

vos_mbx_init_end:
	if (E_OK != ret) {
		if (g_ctrl.p_cell) {
			free(g_ctrl.p_cell);
			g_ctrl.p_cell = NULL;
		}
	}
	pthread_mutex_unlock(&g_mutex);

	return ret;
}

ER vos_mbx_exit(void)
{
	ER ret = E_OK;

	pthread_mutex_lock(&g_mutex);

	if (g_ctrl.ioctl_fd < 0) {
		ret = E_OK;
		goto vos_mbx_exit_end; //already uninitialized
	}

#if VOS_MBX_USE_IOCTL
	if (0 != close(g_ctrl.ioctl_fd)) {
		DBG_ERR("close fails, ioctl_fd 0x%X\r\n", g_ctrl.ioctl_fd);
		ret = E_SYS;
		goto vos_mbx_exit_end;
	}
#endif

	g_ctrl.ioctl_fd = -1;

vos_mbx_exit_end:
	pthread_mutex_unlock(&g_mutex);

	return ret;
}

static MBX_CELL_T* vos_mbx_get_cell(ID mbxid)
{
	MBX_CELL_T *p_cell;

	if (NULL == g_ctrl.p_cell || 0 == g_max_mbxid_num) {
		DBG_ERR("Please call vos_mbx_init first\r\n");
		return NULL;
	}

	if ((UINT)mbxid < VOS_MBX_MIN_VALID_ID || (UINT)mbxid > g_max_mbxid_num) {
		DBG_ERR("Invalid mbxid %d, should be %d ~ %d\r\n", mbxid, VOS_MBX_MIN_VALID_ID, g_max_mbxid_num);
		return NULL;
	}

	p_cell = g_ctrl.p_cell + (mbxid - VOS_MBX_MIN_VALID_ID);

	if (RTOS_MBX_INITED_TAG != p_cell->st_used) {
		DBG_ERR("mbxid %d not created\r\n", mbxid);
		return NULL;
	}

	return p_cell;
}

static void vos_mbx_dump_mqattr(mqd_t mqd)
{
	struct mq_attr attr;

	if (-1 == mq_getattr(mqd, &attr)) {
		DBG_ERR("mq_getattr failed\r\n");
		return;
	}

	DBG_DUMP("mq_flags 0x%lX, mq_maxmsg %ld, mq_msgsize %ld, mq_curmsgs %ld\r\n",
		attr.mq_flags, attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);
}

//! Common api
ER vos_mbx_create(ID *p_mbxid, VOS_MBX_PARAM *p_param)
{
	char mqueue_name[_BUFFER_SIZE_];
	struct mq_attr attr;
	UINT new_mbxid;
	int loop_count;
	MBX_CELL_T *p_cell = NULL;
	ER ret = E_OK;
	int is_found = 0;
	mqd_t mqd;

	// error check before lock
	if (g_max_mbxid_num < 1) {
		DBG_ERR("Please call vos_mbx_init first\r\n");
		return E_SYS;
	}

	if (NULL == p_param) {
		DBG_ERR("p_param is NULL\r\n");
		return E_PAR;
	}

	if (0 == p_param->maxmsg || 0 == p_param->msgsize) {
		DBG_ERR("Invalid param, maxmsg %d, msgsize %d\r\n", p_param->maxmsg, p_param->msgsize);
		return E_PAR;
	}

	pthread_mutex_lock(&g_mutex);

	// loop initial state
	new_mbxid = g_cur_mbxid;
	loop_count = g_max_mbxid_num;

	while(loop_count--) {
		new_mbxid++;
		if (new_mbxid > g_max_mbxid_num) {
			new_mbxid = VOS_MBX_MIN_VALID_ID;
		}

		p_cell = &g_ctrl.p_cell[new_mbxid - VOS_MBX_MIN_VALID_ID]; //mbxid to mbx array index
		if (0 == p_cell->st_used) {
			// found a free cell
			is_found = 1;
			break;
		}
	}

	if (!is_found) {
		DBG_ERR("no free mbx id\r\n");
		ret = E_SYS;
		goto vos_mbx_create_end;
	}

	// already found here
	attr.mq_flags = 0; //Flags: 0 or O_NONBLOCK
	attr.mq_maxmsg = (long)p_param->maxmsg;
	attr.mq_msgsize = (long)p_param->msgsize;
	attr.mq_curmsgs = 0; //dummy

	snprintf((char*)mqueue_name, _BUFFER_SIZE_, "%s_%d", VOS_MBX_MQUEUE_NAME, new_mbxid);
	mqd = mq_open(mqueue_name, O_CREAT | O_RDWR, 0644, &attr);
	if (((mqd_t)-1) == mqd) {
		DBG_ERR("mq_open %s failed, errno %d\r\n", mqueue_name, errno);
		ret = E_SYS;
		goto vos_mbx_create_end;
	}

	p_cell->st_used = RTOS_MBX_INITED_TAG;
	p_cell->st_mqd = mqd;
	p_cell->st_msgsize = p_param->msgsize;

	g_cur_mbxid = new_mbxid;
	*p_mbxid = new_mbxid;

vos_mbx_create_end:
	if (E_OK != ret) {
		*p_mbxid = 0;
	}
	pthread_mutex_unlock(&g_mutex);

	return ret;
}

void vos_mbx_destroy(ID mbxid)
{
	MBX_CELL_T *p_cell;

	pthread_mutex_lock(&g_mutex);

	p_cell = vos_mbx_get_cell(mbxid);
	if (NULL == p_cell) {
		DBG_ERR("get cell failed, mbxid %d\r\n", mbxid);
		goto vos_mbx_destroy_end;
	}

	if (-1 == mq_close(p_cell->st_mqd)) {
		DBG_ERR("mq_close failed, mbxid %d, errno %d\r\n", mbxid, errno);
		goto vos_mbx_destroy_end;
	}

	memset(p_cell, 0, sizeof(MBX_CELL_T));

vos_mbx_destroy_end:
	pthread_mutex_unlock(&g_mutex);
}

ER vos_mbx_snd(ID mbxid, void *p_data, UINT size)
{
	MBX_CELL_T *p_cell;

	p_cell = vos_mbx_get_cell(mbxid);
	if (NULL == p_cell) {
		DBG_ERR("get cell failed, mbxid %d\r\n", mbxid);
		return E_PAR;
	}

	if (-1 == mq_send(p_cell->st_mqd, p_data, size, VOS_MBX_MSG_PRIO)) {
		if (EBADF == errno) {
			DBG_ERR("Invalid mqd, mbxid %d\r\n", mbxid);
		} else if (EMSGSIZE == errno) {
			DBG_ERR("snd size (%d) exceeds msgsize\r\n", size);
			vos_mbx_dump_mqattr(p_cell->st_mqd);
		} else {
			DBG_ERR("mq_send failed, mbxid %d, errno %d\r\n", mbxid, errno);
			vos_mbx_dump_mqattr(p_cell->st_mqd);
		}
		return E_SYS;
	}

	return E_OK;
}

ER vos_mbx_rcv(ID mbxid, void *p_data, UINT size)
{
	MBX_CELL_T *p_cell;

	p_cell = vos_mbx_get_cell(mbxid);
	if (NULL == p_cell) {
		DBG_ERR("get cell failed, mbxid %d\r\n", mbxid);
		return E_PAR;
	}

	if (size < p_cell->st_msgsize) {
		DBG_ERR("size(%d) < msgsize(%d)\r\n", size, p_cell->st_msgsize);
		return E_PAR; //if size < mq_msgsize, mq_receive will return fail
	}

	if (-1 == mq_receive(p_cell->st_mqd, p_data, size, NULL)) {
		if (EBADF == errno) {
			DBG_ERR("Invalid mqd, mbxid %d\r\n", mbxid);
		} else if (EMSGSIZE == errno) {
			DBG_ERR("rcv size (%d) not enouth\r\n", size);
			vos_mbx_dump_mqattr(p_cell->st_mqd);
		} else {
			DBG_ERR("mq_receive failed, mbxid %d, errno %d\r\n", mbxid, errno);
			vos_mbx_dump_mqattr(p_cell->st_mqd);
		}
		return E_SYS;
	}

	return E_OK;
}

UINT vos_mbx_is_empty(ID mbxid)
{
	struct mq_attr attr;
	MBX_CELL_T *p_cell;

	p_cell = vos_mbx_get_cell(mbxid);
	if (NULL == p_cell) {
		DBG_ERR("get cell failed, mbxid %d\r\n", mbxid);
		return 0;
	}

	if (-1 == mq_getattr(p_cell->st_mqd, &attr)) {
		DBG_ERR("mq_getattr failed\r\n");
		return 0;
	}

	DBG_DUMP("mq_flags 0x%lX, mq_maxmsg %ld, mq_msgsize %ld, mq_curmsgs %ld\r\n",
		attr.mq_flags, attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);

	return (attr.mq_curmsgs == 0);
}
