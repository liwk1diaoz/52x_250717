/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>

#define __MODULE__    vos_mailbox
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/mailbox.h>
#include <kwrap/spinlock.h>

#include <FreeRTOS.h>
#include <queue.h>

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define RTOS_MBX_INITED_TAG    MAKEFOURCC('R', 'M', 'B', 'X') ///< a key value
#define VOS_MBX_MIN_VALID_ID    1

#if defined(__LINUX)
#define my_alloc    vmalloc
#define my_free     vfree

#elif defined(__FREERTOS)
#define my_alloc    malloc
#define my_free     free

#else
#error Not supported OS
#endif

typedef struct {
	int st_used;
	QueueHandle_t st_hdl;
	UINT st_msgsize;
} MBX_CELL_T;

typedef struct {
	MBX_CELL_T *p_cell;
} MBX_CTRL_T;

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int vos_mailbox_debug_level = NVT_DBG_WRN;
static VK_DEFINE_SPINLOCK(g_mailbox_lock);

UINT g_max_mbxid_num = 0;
UINT g_cur_mbxid = 0;
static MBX_CTRL_T g_ctrl = {0};
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
ER vos_mbx_init(UINT max_mbxid_num)
{
	unsigned long flags = 0;
	ER ret = E_OK;

	if (!max_mbxid_num) {
		DBG_ERR("Invalid param, max_mbxid_num %d\r\n", max_mbxid_num);
		ret = E_PAR;
		goto vos_mbx_init_end;
	}

	vk_spin_lock_irqsave(&g_mailbox_lock, flags);

	if (g_ctrl.p_cell != NULL) {
		DBG_WRN("Already initialized, please call vos_mbx_exit first\r\n");
		ret = E_OK;
		goto vos_mbx_init_end; //already initialized
	}

	g_ctrl.p_cell = my_alloc(max_mbxid_num * sizeof(MBX_CELL_T));
	if (NULL == g_ctrl.p_cell) {
		DBG_ERR("get memory failed\r\n");
		ret = E_NOMEM;
		goto vos_mbx_init_end;
	}
	memset(g_ctrl.p_cell, 0, max_mbxid_num * sizeof(MBX_CELL_T));

	g_max_mbxid_num = max_mbxid_num;
	DBG_IND("g_max_mbxid_num %d\r\n", g_max_mbxid_num);

vos_mbx_init_end:
	if (E_OK != ret) {
		if (g_ctrl.p_cell) {
			my_free(g_ctrl.p_cell);
			g_ctrl.p_cell = NULL;
		}
	}
	vk_spin_unlock_irqrestore(&g_mailbox_lock, flags);

	return ret;
}

ER vos_mbx_exit(void)
{
	unsigned long flags = 0;
	ER ret = E_OK;

	vk_spin_lock_irqsave(&g_mailbox_lock, flags);

	if (NULL == g_ctrl.p_cell) {
		ret = E_OK;
		goto vos_mbx_exit_end; //already uninitialized
	}

	my_free(g_ctrl.p_cell);
	g_ctrl.p_cell = NULL;

vos_mbx_exit_end:
	vk_spin_unlock_irqrestore(&g_mailbox_lock, flags);

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

static int vos_mbx_get_free_id(ID *p_mbxid)
{
	UINT new_mbxid;
	int loop_count;
	MBX_CELL_T *p_cell = NULL;

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
			p_cell->st_used = RTOS_MBX_INITED_TAG;
			*p_mbxid = new_mbxid;
			g_cur_mbxid = new_mbxid;
			return 0;
		}
	}

	DBG_ERR("no free mbx id\r\n");
	return -1;
}

//! Common api
ER vos_mbx_create(ID *p_mbxid, VOS_MBX_PARAM *p_param)
{
	MBX_CELL_T *p_cell = NULL;
	ID new_mbxid;
	ER ret = E_OK;
	QueueHandle_t hdl;
	unsigned long flags = 0;

	// error check before lock
	if (g_max_mbxid_num < 1) {
		DBG_ERR("Please call vos_mbx_init first\r\n");
		return E_SYS;
	}

	if (NULL == p_mbxid) {
		DBG_ERR("p_mbxid is NULL\n");
		return E_PAR;
	}

	if (NULL == p_param) {
		DBG_ERR("p_param is NULL\r\n");
		return E_PAR;
	}

	if (0 == p_param->maxmsg || 0 == p_param->msgsize) {
		DBG_ERR("Invalid param, maxmsg %d, msgsize %d\r\n", p_param->maxmsg, p_param->msgsize);
		return E_PAR;
	}

	vk_spin_lock_irqsave(&g_mailbox_lock, flags);

	if (0 != vos_mbx_get_free_id(&new_mbxid)) {
		DBG_ERR("mbx out of id\r\n");
		return E_SYS;
	}
	p_cell = &g_ctrl.p_cell[new_mbxid - VOS_MBX_MIN_VALID_ID];

	hdl = xQueueCreate(p_param->maxmsg, p_param->msgsize);
	if (NULL == hdl) {
		DBG_ERR("xQueueCreate failed\r\n");
		p_cell->st_used = 0; //cancel the used tag which is set in vos_mbx_get_free_id
		ret = E_SYS;
		goto vos_mbx_create_end;
	}

	p_cell->st_hdl = hdl;
	p_cell->st_msgsize = p_param->msgsize;
	*p_mbxid = new_mbxid;

vos_mbx_create_end:
	if (E_OK != ret) {
		*p_mbxid = 0;
	}
	vk_spin_unlock_irqrestore(&g_mailbox_lock, flags);

	return ret;
}

void vos_mbx_destroy(ID mbxid)
{
	unsigned long flags = 0;
	MBX_CELL_T *p_cell;

	vk_spin_lock_irqsave(&g_mailbox_lock, flags);

	p_cell = vos_mbx_get_cell(mbxid);
	if (NULL == p_cell) {
		DBG_ERR("get cell failed, mbxid %d\r\n", mbxid);
		goto vos_mbx_destroy_end;
	}

	vQueueDelete(p_cell->st_hdl);

	memset(p_cell, 0, sizeof(MBX_CELL_T));

vos_mbx_destroy_end:
	vk_spin_unlock_irqrestore(&g_mailbox_lock, flags);
}

ER vos_mbx_snd(ID mbxid, void *p_data, UINT size)
{
	MBX_CELL_T *p_cell;

	p_cell = vos_mbx_get_cell(mbxid);
	if (NULL == p_cell) {
		DBG_ERR("get cell failed, mbxid %d\r\n", mbxid);
		return E_PAR;
	}

	if (size > p_cell->st_msgsize) {
		DBG_ERR("size > msgsize(%d)\r\n", p_cell->st_msgsize);
		return E_PAR;
	}

	if (pdTRUE != xQueueSend(p_cell->st_hdl, p_data, portMAX_DELAY)) {
		DBG_ERR("xQueueSend failed, hdl 0x%lX\r\n", (ULONG)p_cell->st_hdl);
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

	if (pdTRUE != xQueueReceive(p_cell->st_hdl, p_data, portMAX_DELAY)) {
		DBG_ERR("xQueueReceive failed, hdl 0x%lx\r\n", (ULONG)p_cell->st_hdl);
		return E_SYS;
	}

	return E_OK;
}

UINT vos_mbx_is_empty(ID mbxid)
{
	MBX_CELL_T *p_cell;

	p_cell = vos_mbx_get_cell(mbxid);
	if (NULL == p_cell) {
		DBG_ERR("get cell failed, mbxid %d\r\n", mbxid);
		return 0;
	}

	return (0 == uxQueueMessagesWaiting(p_cell->st_hdl));
}
