#include "msdcnvt_ipc.h"
#include "msdcnvt_int.h"
#include "msdcnvt_id.h"

//Local Functions
static BOOL msdcnvt_function(void);
static BOOL msdcnvt_calc_access_address_with_size(void);
static void msdcnvt_reg_access(UINT32 *p_dst, UINT32 *p_src, UINT32 num_words);

//Local Variable
#if defined (__UITRON)
static MSDCNVT_LUN  m_null_lun = {0};
#endif

//APIs
BOOL msdcnvt_verify_cb(UINT32 p_cmd_buf, UINT32 *p_data_buf) //!< Callback for USB_MSDC_INFO.msdc_check_cb
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;

	SEM_WAIT(m_msdcnvt_sem_cb_id);

	//Check Internal Status
	if (p_ctrl->init_key != CFG_MSDCNVT_INIT_KEY) {
		DBG_ERR("Not Initial\r\n");
		SEM_WAIT(m_msdcnvt_sem_cb_id);
		return FALSE;
	}

	p_host->p_cbw = (MSDCNVT_CBW *)p_cmd_buf;
	p_host->cmd_id = p_host->p_cbw->cb[0];

	//Check Novatek Tag
	if (p_host->p_cbw->cb[1] != CDB_01_NVT_TAG) {
		SEM_WAIT(m_msdcnvt_sem_cb_id);
		return FALSE;
	} else if (!msdcnvt_calc_access_address_with_size()) {
		SEM_WAIT(m_msdcnvt_sem_cb_id);
		return FALSE;
	} else if (p_ctrl->host_info.lb_length  > p_ctrl->host_info.pool_size) {
		DBG_ERR("data exceed %d Bytes\r\n", p_ctrl->host_info.pool_size);
		SEM_WAIT(m_msdcnvt_sem_cb_id);
		return FALSE;
	}

	switch (p_host->cmd_id) {
	case SBC_CMD_MSDCNVT_READ:
	case SBC_CMD_MSDCNVT_WRITE:
		if ((p_host->lb_address & CFG_VIRTUAL_MEM_MAP_MASK) == CFG_VIRTUAL_MEM_MAP_ADDR) {
			if (p_ctrl->handler.fp_virt_mem_map) {
				p_host->lb_address = p_ctrl->handler.fp_virt_mem_map((p_host->lb_address & (~CFG_VIRTUAL_MEM_MAP_MASK)), p_host->lb_length); //Customer
				*p_data_buf = p_host->lb_address;
			} else {
				DBG_ERR("MSDCNVT_HANDLER_TYPE_INVALID_MEM_READ called, but no handler registered\r\n");
				*p_data_buf = 0;
			}
		} else if (p_host->lb_address >= CFG_REGISTER_BEGIN_ADDR) {
			*p_data_buf = (UINT32)p_host->p_pool; //Register
		} else {
#if defined(__UITRON)
			//DRAM
			*p_data_buf = p_host->lb_address;
#else
			//for pure linux, cannot access DRAM directly
			*p_data_buf = (UINT32)p_host->p_pool;
#endif
		}
		break;
	case SBC_CMD_MSDCNVT_FUNCTION:
		*p_data_buf = (UINT32)p_host->p_pool;
		break;
	default:
		SEM_WAIT(m_msdcnvt_sem_cb_id);
		return FALSE;
	}

	if (p_ctrl->p_msdc) {
		p_ctrl->p_msdc->set_config(USBMSDC_CONFIG_ID_CHGVENINBUGADDR, *p_data_buf);
	}

	//SEM_WAIT(m_msdcnvt_sem_cb_id); at msdcnvt_vendor_cb when msdcnvt_verify_cb success
	//coverity[missing_unlock] ,the unlock at msdcnvt_vendor_cb
	return TRUE;
}

void msdcnvt_vendor_cb(PMSDCVENDOR_PARAM p_buf) //!< Callback for USB_MSDC_INFO.msdc_vendor_cb
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;

	p_host->p_cbw = (MSDCNVT_CBW *)p_buf->vendor_cmd_buf;
	p_host->p_csw = (MSDCNVT_CSW *)p_buf->vendor_csw_buf;

	//At device side, the CSWTag has to be written as the same with tag
	p_host->p_csw->tag = p_host->p_cbw->tag;

	switch (p_host->cmd_id) {
	case SBC_CMD_MSDCNVT_READ:
		if ((p_host->lb_address & CFG_VIRTUAL_MEM_MAP_MASK) == CFG_VIRTUAL_MEM_MAP_ADDR) {
			p_buf->in_data_buf = p_host->lb_address;
		} else if (p_host->lb_address >= CFG_REGISTER_BEGIN_ADDR) {
			//Register
#if defined (__UITRON)
			msdcnvt_reg_access((UINT32 *)p_host->p_pool, (UINT32 *)p_host->lb_address, p_host->lb_length >> 2);
#else
			//for linux, always use pool mem as temp buffer because user space memory map only msdcnvt working buffer
			void *p_addr = (void *)ioremap_nocache(p_host->lb_address, p_host->lb_length);

			msdcnvt_reg_access((UINT32 *)p_host->p_pool, p_addr, p_host->lb_length >> 2);
			iounmap(p_addr);
#endif
		} else {//DRAM
#if defined (__UITRON)
			p_buf->in_data_buf = p_host->lb_address;
#else
			//for linux, always use pool mem as temp buffer because user space memory map only msdcnvt working buffer
			void *p_addr = (void *)ioremap_nocache(p_host->lb_address, p_host->lb_length);

			memcpy(p_host->p_pool, p_addr, p_host->lb_length);
			iounmap(p_addr);
#endif
		}
		break;
	case SBC_CMD_MSDCNVT_WRITE:
		if ((p_host->lb_address & CFG_VIRTUAL_MEM_MAP_MASK) == CFG_VIRTUAL_MEM_MAP_ADDR) {
			p_buf->in_data_buf = p_host->lb_address;
		} else if (p_host->lb_address >= CFG_REGISTER_BEGIN_ADDR) {
			//Register
#if defined (__UITRON)
			msdcnvt_reg_access((UINT32 *)p_host->lb_address, (UINT32 *)p_host->p_pool, p_host->lb_length >> 2);
#else
			//for linux, always use pool mem as temp buffer because user space memory map only msdcnvt working buffer
			void *p_addr = (void *)ioremap_nocache(p_host->lb_address, p_host->lb_length);

			msdcnvt_reg_access((UINT32 *)p_addr, (UINT32 *)p_host->p_pool, p_host->lb_length >> 2);
#endif
		} else {
			//DRAM
#if defined (__UITRON)
			p_buf->in_data_buf = p_host->lb_address;
#else
			//for linux, always use pool mem as temp buffer because user space memory map only msdcnvt working buffer
			void *p_addr = (void *)ioremap_nocache(p_host->lb_address, p_host->lb_length);

			memcpy(p_addr, p_host->p_pool, p_host->lb_length);
			iounmap(p_addr);
#endif
		}
		break;
	case SBC_CMD_MSDCNVT_FUNCTION:
		msdcnvt_function();
		p_buf->in_data_buf = (UINT32)p_host->p_pool;
		break;
	default:
		DBG_ERR("unknown command id: %d\r\n", p_host->cmd_id);
		SEM_SIGNAL(m_msdcnvt_sem_cb_id);
		return;
	}

	p_buf->in_data_buf_len = p_buf->out_data_buf_len = p_host->lb_length;
	SEM_SIGNAL(m_msdcnvt_sem_cb_id);
}

//!< for Register Callback Function
BOOL msdcnvt_add_callback_bi(MSDCNVT_BI_CALL_UNIT *p_unit)
{
	MSDCNVT_BI_CALL_CTRL *p_bicall = &msdcnvt_get_ctrl()->bicall;

	if (p_bicall->p_head == NULL) {
		p_bicall->p_head = p_unit;
	}

	if (p_bicall->p_last != NULL) {
		p_bicall->p_last->fp = (void *)p_unit;
	}

	while (p_unit->p_name != NULL) {
		p_unit++;
	}
	p_bicall->p_last = p_unit;

	return TRUE;
}

BOOL msdcnvt_add_callback_si(FP *fp_tbl_get, UINT8 num_gets, FP *fp_tbl_set, UINT8 num_sets)
{
	MSDCNVT_SI_CALL_CTRL *p_si_ctrl = &msdcnvt_get_ctrl()->sicall;

	p_si_ctrl->fp_tbl_get = fp_tbl_get;
	p_si_ctrl->num_gets = num_gets;
	p_si_ctrl->fp_tbl_set = fp_tbl_set;
	p_si_ctrl->num_sets = num_sets;

	return TRUE;
}

UINT8 *msdcnvt_get_data_addr(void)
{
	return msdcnvt_get_ctrl()->bk.p_pool;
}

UINT32 msdcnvt_get_data_size(void)
{
	return msdcnvt_get_ctrl()->bk.pool_size;
}

UINT32 msdcnvt_get_trans_size(void)
{
	return msdcnvt_get_ctrl()->bk.trans_size;
}

BOOL msdcnvt_init(MSDCNVT_INIT *p_init)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;
	MSDCNVT_BACKGROUND_CTRL *p_bk = &p_ctrl->bk;
	UINT8 *p_mem_pool = (UINT8 *)MSDCNVT_CACHE_ADDR((UINT32)p_init->p_cache);
	UINT32 mem_size = p_init->cache_size;

	memset(p_ctrl, 0, sizeof(MSDCNVT_CTRL));

	if (p_init->api_ver != MSDCNVT_API_VERSION) {
		DBG_ERR("wrong version\r\n");
		return FALSE;
	}

	//setup working buffer
	if (p_mem_pool) {
		//Check Valid
		if (p_init->cache_size < CFG_MIN_WORKING_BUF_SIZE) {
			DBG_ERR("sizeCache minimum need %d bytes\r\n", CFG_MIN_WORKING_BUF_SIZE);
			return FALSE;
		}
		//IPC Data
		p_ctrl->ipc.p_cfg = (MSDCNVT_IPC_CFG *)MSDCNVT_NONCACHE_ADDR((UINT32)p_mem_pool);
		p_mem_pool += CFG_NVTMSDC_SIZE_IPC_DATA;
		mem_size -= CFG_NVTMSDC_SIZE_IPC_DATA;
		//Set Host Ctrl
		p_host->p_pool = p_mem_pool;
		p_host->pool_size = CFG_MIN_HOST_MEM_SIZE;
		p_mem_pool += p_host->pool_size;
		mem_size -= p_host->pool_size;
		p_bk->p_pool = p_mem_pool;
		p_bk->pool_size = CFG_MIN_BACKGROUND_MEM_SIZE;
		p_mem_pool += p_bk->pool_size;
		mem_size -= p_bk->pool_size;
	} else {
#if defined(__UITRON)
		DBG_ERR("p_mem_pool = NULL.\r\n");
		return FALSE;
#else
		u8 *p_share_mem = (u8 *)kzalloc(CFG_NVTMSDC_SIZE_IPC_DATA + CFG_MIN_HOST_MEM_SIZE, GFP_KERNEL);

		p_ctrl->ipc.p_cfg = (MSDCNVT_IPC_CFG *)p_share_mem;
		p_host->p_pool = p_share_mem + CFG_NVTMSDC_SIZE_IPC_DATA;
		p_host->pool_size = CFG_MIN_HOST_MEM_SIZE;
		p_bk->p_pool = (UINT8 *)kzalloc(CFG_MIN_BACKGROUND_MEM_SIZE, GFP_KERNEL);
		p_bk->pool_size = CFG_MIN_BACKGROUND_MEM_SIZE;
#endif
	}

	//init setup
	p_ctrl->flag_id = m_msdcnvt_flg_id;
	p_ctrl->fp_event = p_init->event_cb;
	p_ctrl->p_msdc = p_init->p_msdc;
	p_bk->task_id = m_msdcnvt_tsk_id;

#if defined(__UITRON)
	if (p_bk->task_id == 0) {
		DBG_ERR("ID not installed\r\n");
		return FALSE;
	}
#endif

	//Set IPC (for pure linux, also use ipc structure to commuicate kernel/user)
	if (p_ctrl->ipc.p_cfg) {
		p_ctrl->ipc.p_cfg->ipc_version = MSDCNVT_IPC_VERSION;
		p_ctrl->ipc.p_cfg->port = (p_init->port == 0) ? 1968 : p_init->port;
		p_ctrl->ipc.p_cfg->tos = p_init->tos;
		p_ctrl->ipc.p_cfg->snd_buf_size = p_init->snd_buf_size;
		p_ctrl->ipc.p_cfg->rcv_buf_size = p_init->rcv_buf_size;
		p_ctrl->ipc.p_cfg->call_tbl = *msdcnvt_get_ipc_call_tbl(NULL);
#if defined(__UITRON)
		p_ctrl->ipc.ipc_msgid = nvipc_msgget(nvipc_ftok("MSDCNVT"));
		if (p_ctrl->ipc.ipc_msgid < 0) {
			DBG_ERR("failed to msgget\r\n");
			return FALSE;
		}
#endif
	}

	if (p_init->b_hook_dbg_msg == TRUE) {
		INT32 i;
		MSDCNVT_DBGSYS_CTRL *p_dbgsys = &p_ctrl->dbgsys;
		//Set DbgSys
		p_dbgsys->sem_id = m_msdcnvt_sem_id;
		p_dbgsys->msg_cnt_mask   = CFG_DBGSYS_MSG_PAYLOAD_NUM - 1;
		p_dbgsys->payload_num   = CFG_DBGSYS_MSG_PAYLOAD_NUM;
		p_dbgsys->payload_size  = mem_size / CFG_DBGSYS_MSG_PAYLOAD_NUM;
		p_dbgsys->b_no_uart_output  = TRUE;
#if defined(__UITRON)
		switch (p_init->uart_index) {
		case 0:
			p_dbgsys->fp_uart_put_string = uart_putstring;
			break;
		case 1:
			p_dbgsys->fp_uart_put_string = uart2_putstring;
			break;
		default:
			p_dbgsys->fp_uart_put_string = NULL;
			break;

		}
#else
		p_dbgsys->fp_uart_put_string = NULL;
#endif

		for (i = 0; i < CFG_DBGSYS_MSG_PAYLOAD_NUM; i++) {
			MSDCNVT_DBGSYS_UNIT *p_unit = &p_dbgsys->queue[i];

			p_unit->p_msg = p_mem_pool;
			p_mem_pool += p_dbgsys->payload_size;
			mem_size -= p_dbgsys->payload_size;
		}

		p_dbgsys->b_init = TRUE;
	}


#if defined(__UITRON)
	// only uitron need ipc task
	THREAD_RESUME(m_msdcnvt_tsk_ipc_id);
#endif

	p_ctrl->init_key = CFG_MSDCNVT_INIT_KEY;
	return TRUE;
}

BOOL msdcnvt_exit(void)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
#if defined(__UITRON)
	MSDCNVT_DBGSYS_CTRL *p_dbgsys = &p_ctrl->dbgsys;
#endif

#if defined(__UITRON)
	if (p_dbgsys->b_init == TRUE) {
		FLGPTN flg_ptn;

		debug_set_console(CON_1);
		if (msdcnvt_get_ctrl()->fp_event) {
			msdcnvt_get_ctrl()->fp_event(MSDCNVT_EVENT_HOOK_DBG_MSG_OFF);
		}
		if (p_ctrl->ipc.b_net_running) {
			p_ctrl->ipc.p_cfg->stop_server = 1;
		} else {
			if (nvtipc_msgrel(p_ctrl->ipc.ipc_msgid) < 0) {
				DBG_ERR("nvtipc_msgrel.\r\n");
			}
			p_ctrl->ipc.ipc_msgid = 0;
		}
		wai_flg(&flg_ptn, p_ctrl->flag_id, FLGMSDCNVT_IPC_STOPPED, TWF_ORW | TWF_CLR);
		ter_tsk(m_msdcnvt_tsk_ipc_id);
		p_dbgsys->b_init = FALSE;
	}
#else
	if (p_ctrl->ipc.p_cfg) {
		kfree(p_ctrl->ipc.p_cfg);
		p_ctrl->ipc.p_cfg = NULL;
	}
	if (p_ctrl->bk.p_pool) {
		kfree(p_ctrl->bk.p_pool);
		p_ctrl->bk.p_pool = NULL;
	}
#endif

	p_ctrl->init_key = 0;
	return TRUE;
}

BOOL msdcnvt_net(BOOL b_turn_on)
{
#if defined(__UITRON)
	INT32 n_retry = 100;
	MSDCNVT_IPC_CTRL *p_ipc = &msdcnvt_get_ctrl()->ipc;

	if (b_turn_on) {
		if (p_ipc->b_net_running) {
			DBG_WRN("cannot turn on twice.\r\n");
			return FALSE;
		}

		p_ipc->p_cfg->stop_server = 0;
		snprintf(p_ipc->cmd_line, sizeof(p_ipc->cmd_line) - 1, "msdcnvt 0x%08lX 0x%08X &", MSDCNVT_PHY_ADDR((UINT32)p_ipc->p_cfg), sizeof(MSDCNVT_IPC_CFG));
#if defined(_NETWORK_ON_CPU1_)
		msdcnvt_ecos_main(p_ipc->cmd_line);
#else
		NVTIPC_SYS_MSG  sys_msg;

		sys_msg.syscmdid = NVTIPC_SYSCMD_SYSCALL_REQ;
		sys_msg.dataaddr = (UINT32)&p_ipc->cmd_line[0];
		sys_msg.datasize = strlen(p_ipc->cmd_line) + 1;
		if (nvtipc_msgsnd(NVTIPC_SYS_QUEUE_ID, NVTIPC_SENDTO_CORE2, &sys_msg, sizeof(sys_msg)) < 0) {
			DBG_ERR("Failed to NVTIPC_SYS_QUEUE_ID\r\n");
		}
#endif
		while (!p_ipc->b_net_running && n_retry-- > 0) {
			delay_delayms(10);
		}
		if (!p_ipc->b_net_running) {
			DBG_ERR("Failed to start MSDC-NET\r\n");
		}

	} else {
		if (p_ipc->b_net_running) {
			p_ipc->p_cfg->stop_server = 1;
			while (p_ipc->b_net_running && n_retry-- > 0) {
				delay_delayms(10);
			}
			if (p_ipc->b_net_running) {
				DBG_ERR("Failed to close MSDC-NET\r\n");
			}
		}
	}
#endif
	return TRUE;
}

//Check Parameters, return NULL indicate error parameters
void *msdcnvt_check_params(void *p_data, UINT32 size)
{
	MSDCEXT_PARENT *p_desc = (MSDCEXT_PARENT *)p_data;

	if (p_desc->size != size) {
		DBG_ERR("MsdcNvtCallback: Data Size Wrong, %d(Host) != %d(Device)\r\n", p_desc->size, size);
		p_desc->b_handled = TRUE;
		p_desc->b_ok = FALSE;
		return NULL;
	}
	return p_data;
}


BOOL msdcnvt_reg_handler(MSDCNVT_HANDLER_TYPE type, void *p_cb)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();

	switch (type) {
	case MSDCNVT_HANDLER_TYPE_VIRTUAL_MEM_MAP:
		p_ctrl->handler.fp_virt_mem_map = p_cb;
		break;
	default:
		DBG_ERR("unknown typ:%d\r\n", type);
		break;
	}
	return TRUE;
}

BOOL msdcnvt_set_msdcobj(MSDC_OBJ *p_msdc)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();

	p_ctrl->p_msdc = p_msdc;
	return TRUE;
}

MSDC_OBJ *msdcnvt_get_msdcobj(void)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();

	return p_ctrl->p_msdc;
}

#if defined(__UITRON)
static BOOL null_lun_det_strg_card(void)
{
	return FALSE;
}
static BOOL null_lun_det_strg_cardwp(void)
{
	return TRUE;
}

static ER null_lun_lock(void)
{
	return E_OK;
}
static ER null_lun_unlock(void)
{
	return E_OK;
}
static ER null_lun_open(void)
{
	return E_OK;
}
static ER null_lun_write_sectors(INT8 *p_ubf, UINT32 lb_addr, UINT32 sect_cnt)
{
	return E_OK;
}
static ER null_lun_read_sectors(INT8 *p_ubf, UINT32 lb_addr, UINT32 sect_cnt)
{
	return E_OK;
}
static ER null_lun_close(void)
{
	return E_OK;
}
static ER null_lun_open_mem_bus(void)
{
	return E_OK;
}
static ER null_lun_close_mem_buf(void)
{
	return E_OK;
}
static ER null_lun_fw_set_param(STRG_SET_PARAM_EVT evt, UINT32 param1, UINT32 param2)
{
	return E_OK;
}
static ER null_lun_fw_get_param(STRG_GET_PARAM_EVT evt, UINT32 param1, UINT32 param2)
{
	return E_OK;
}
static ER null_lun_ext_ioctl(STRG_EXT_CMD cmd_id, UINT32 param1, UINT32 param2)
{
	return E_OK;
}

static STORAGE_OBJ m_null_lun_obj = {0, 0, null_lun_lock, null_lun_unlock, null_lun_open, null_lun_write_sectors, null_lun_read_sectors, null_lun_close, null_lun_open_mem_bus, null_lun_close_mem_buf, null_lun_fw_set_param, null_lun_fw_get_param, null_lun_ext_ioctl};

static UINT32 dev_null_get_caps(UINT32 cap_id, UINT32 param1) // Get Capability Flag (Base on interface version)
{
	UINT32 v = 0;

	if (cap_id == STORAGE_CAPS_HANDLE) {
		v = (UINT32)(&m_null_lun_obj);
	}
	return v;
}
static UINT32 dev_null_open(void)
{
	return 0;
}
static UINT32 dev_null_close(void)
{
	return 0;
}
static DX_OBJECT m_null_dev = {
	DXFLAG_SIGN,
	DX_CLASS_STORAGE_EXT | DX_TYPE_CARD1,
	STORAGE_VER,
	"Storage Null",
	0, 0, 0, 0,
	dev_null_get_caps,
	0,
	NULL,
	dev_null_open,
	dev_null_close,
	NULL,
	NULL,
	NULL,
	0,
};
#endif

MSDCNVT_LUN *msdcnvt_get_null_lun(void)
{
#if defined(__UITRON)
	m_null_lun.strg = (DX_HANDLE)(&m_null_dev);
	m_null_lun.type = MSDC_STRG;
	m_null_lun.detect_cb = null_lun_det_strg_card;
	m_null_lun.lock_cb = null_lun_det_strg_cardwp;
	m_null_lun.luns = 1;
	return &m_null_lun;
#else
	return NULL;
#endif
}

static BOOL msdcnvt_calc_access_address_with_size(void)
{
	UINT32   cdb_cmdid;
	UINT32   cdb_data_xfer_len, cdw_data_xfer_len;
	MSDCNVT_HOST_INFO *p_host = &msdcnvt_get_ctrl()->host_info;

	cdb_cmdid = p_host->p_cbw->cb[0];
	cdw_data_xfer_len = p_host->p_cbw->data_trans_len;

	p_host->lb_address =  p_host->p_cbw->cb[2] << 24;
	p_host->lb_address |= p_host->p_cbw->cb[3] << 16;
	p_host->lb_address |= p_host->p_cbw->cb[4] << 8;
	p_host->lb_address |= p_host->p_cbw->cb[5];

	p_host->lb_length =  p_host->p_cbw->cb[6] << 24;
	p_host->lb_length |= p_host->p_cbw->cb[7] << 16;
	p_host->lb_length |= p_host->p_cbw->cb[8] << 8;
	p_host->lb_length |= p_host->p_cbw->cb[9];

	cdb_data_xfer_len = p_host->lb_length;

	//Check DWORD Alignment with Starting Address
	if (p_host->lb_address & 0x3) {
		return FALSE;
	}
	//Case(1) : Hn = Dn
	else if ((cdw_data_xfer_len == 0) && (cdb_data_xfer_len == 0)) {
		return TRUE;
	}
	// case(2): Hn < Di , case(3): Hn < Do
	else if ((cdw_data_xfer_len == 0) && (cdb_data_xfer_len > 0)) {
		return FALSE;
	}
	// case(8): Hi <> Do
	else if ((cdb_cmdid == SBC_CMD_MSDCNVT_WRITE) && ((p_host->p_cbw->flags & SBC_CMD_DIR_MASK) != SBC_CMD_DIR_OUT)) {
		return FALSE;
	}
	// case(10): Ho <> Di
	else if ((cdb_cmdid == SBC_CMD_MSDCNVT_READ)  && ((p_host->p_cbw->flags & SBC_CMD_DIR_MASK) != SBC_CMD_DIR_IN)) {
		return FALSE;
	}
	// case(4): Hi>Dn, case(5): hi>Di, case(11): Ho>Do
	else if (cdw_data_xfer_len > cdb_data_xfer_len) {
		return FALSE;
	}
	// case(7): Hi<Di, case(13): Ho < Do
	else if (cdw_data_xfer_len < cdb_data_xfer_len) {
		return FALSE;
	}

	return TRUE;
}

static void msdcnvt_reg_access(UINT32 *p_dst, UINT32 *p_src, UINT32 num_words)
{
	while (num_words--) {
		*(p_dst++) = *(p_src++);
	}
}

static BOOL msdcnvt_function(void)
{
	MSDCNVT_HOST_INFO *p_host = &msdcnvt_get_ctrl()->host_info;

	switch (p_host->p_cbw->cb[10]) {
#if defined(__UITRON) //only uitron supports DbgSys
	case CDB_10_FUNCTION_DBGSYS: //!< DBGSYS Needs to High Priority
		msdcnvt_function_dbgsys();
		break;
#endif
	case CDB_10_FUNCTION_BI_CALL:
		msdcnvt_function_bicall();
		break;
	case CDB_10_FUNCTION_SI_CALL:
		msdcnvt_function_sicall();
		break;
	case CDB_10_FUNCTION_MISC:
		msdcnvt_function_misc();
		break;
	default:
		DBG_ERR("Not Handled ID: %d\r\n", p_host->p_cbw->cb[10]);
	}
	return TRUE;
}
