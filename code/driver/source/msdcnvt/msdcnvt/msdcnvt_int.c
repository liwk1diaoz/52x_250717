#include "msdcnvt_int.h"
#include "msdcnvt_ipc.h"

#define MSDCNVT_IPC_CAST_IN(type, p_icmd) \
	((p_icmd->in_size == sizeof(type))?(type *)MSDCNVT_NONCACHE_ADDR(p_icmd->in_addr) : NULL)
#define MSDCNVT_IPC_CAST_OUT(type, p_icmd) \
	((p_icmd->out_size == sizeof(type))?(type *)MSDCNVT_NONCACHE_ADDR(p_icmd->out_addr) : NULL)


//Local Variable
static MSDCNVT_CTRL m_ctrl; //do not init to {0} for speed up zi area

//Internal APIs
MSDCNVT_CTRL *msdcnvt_get_ctrl(void)
{
	return &m_ctrl;
}

#if defined(__UITRON)
static void msdcnvt_send_ipc(UINT32 api_addr)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_IPC_MSG msg = {0};

	msg.mtype = 2;
	msg.api_addr = api_addr;

	if (nvtipc_msgsnd(p_ctrl->ipc.ipc_msgid, NVTIPC_SENDTO_CORE2, &msg, sizeof(msg)) < 0) {
		DBG_ERR("nvtipc_msgsnd\r\n");
	}
}
#endif

//void xmsdcnvt_tsk_ipc(void)
THREAD_DECLARE(xmsdcnvt_tsk_ipc, arglist)
{
#if defined(__UITRON)
	FLGPTN flgptn;
	NVTIPC_I32 ipc_err = 0;
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();

	while (1) {
		MSDCNVT_IPC_MSG msg = {0};

		PROFILE_TASK_IDLE();

		ipc_err = nvtipc_msgrcv(p_ctrl->ipc.ipc_msgid, &msg, sizeof(msg));
		if (ipc_err == NVTIPC_ER_MSGQUE_RELEASED) {
			break;
		} else if (ipc_err <= 0) {
			DBG_ERR("nvtipc_msgrcv\r\n");
		}

		PROFILE_TASK_BUSY();

		MSDCNVT_ICMD *p_cmd = &p_ctrl->ipc.p_cfg->cmd;
		((MSDCNVT_IAPI)p_cmd->api_addr)(p_cmd);
		msdcnvt_send_ipc(p_cmd->api_addr);
	}

	set_flg(p_ctrl->flag_id, FLGMSDCNVT_IPC_STOPPED);
	wai_flg(&flgptn, p_ctrl->flag_id, FLGMSDCNVT_UNKNOWN, TWF_ORW | TWF_CLR);
#else
	DBG_ERR("not support\r\n");
	THREAD_RETURN(0);
#endif
}

static void msdcnvt_icmd_cb_open(MSDCNVT_ICMD *icmd)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();

	p_ctrl->ipc.b_net_running = TRUE;
	icmd->err = 0;
}

static void msdcnvt_icmd_cb_close(MSDCNVT_ICMD *icmd)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();

	p_ctrl->ipc.b_net_running = FALSE;
	icmd->err = 0;
}

static void msdcnvt_icmd_cb_verify(MSDCNVT_ICMD *icmd)
{
	MSDCNVT_CBW *p_cbw = MSDCNVT_IPC_CAST_IN(MSDCNVT_CBW, icmd);
	UINT32 *p_addr = MSDCNVT_IPC_CAST_OUT(UINT32, icmd);

	if (p_cbw == NULL) {
		DBG_ERR("invalid p_cbw size\r\n");
		icmd->err = -2;
		return;
	} else if (p_addr == NULL) {
		DBG_ERR("invalid MSDCNVT_IPC_CAST_OUT address\r\n");
		icmd->err = -3;
		return;
	}
	icmd->err = (msdcnvt_verify_cb((UINT32)p_cbw, p_addr)) ? 0 : -1;

	//writeback cache for receiving data from PC
	if (p_cbw->flags == SBC_CMD_DIR_IN) {
		//data from device to PC, so writeback to DRAM
		dma_flushwritecache(*p_addr, msdcnvt_get_ctrl()->host_info.lb_length);
	} else if (p_cbw->flags == SBC_CMD_DIR_OUT) {
		//data from PC to device, so drop cache
		dma_flushreadcache(*p_addr, msdcnvt_get_ctrl()->host_info.lb_length);
	}

	*p_addr = MSDCNVT_PHY_ADDR(*p_addr);
}

static void msdcnvt_icmd_cb_vendor(MSDCNVT_ICMD *icmd)
{
	MSDCVENDOR_PARAM param = {0};
	MSDCNVT_CBW *p_cbw = MSDCNVT_IPC_CAST_IN(MSDCNVT_CBW, icmd);
	MSDCNVT_VENDOR_OUTPUT *p_output = MSDCNVT_IPC_CAST_OUT(MSDCNVT_VENDOR_OUTPUT, icmd);

	if (p_cbw == NULL) {
		DBG_ERR("invalid p_cbw size\r\n");
		icmd->err = -2;
		return;
	}
	if (p_output == NULL) {
		DBG_ERR("invalid p_output size\r\n");
		icmd->err = -3;
		return;
	}

	param.vendor_cmd_buf = (UINT32)p_cbw;
	param.vendor_csw_buf = (UINT32)&p_output->csw;
	msdcnvt_vendor_cb(&param);
	p_output->in_data_buf = MSDCNVT_PHY_ADDR(param.in_data_buf);
	p_output->in_data_buf_len = param.in_data_buf_len;
	p_output->out_data_buf_len = param.out_data_buf_len;

	//writeback cache for sending data to PC
	if (param.in_data_buf == 0) {
		//accessing register will get param.in_data_buf is NULL
		dma_flushwritecache((UINT32)msdcnvt_get_ctrl()->host_info.p_pool, p_output->in_data_buf_len);
	} else {
		dma_flushwritecache(param.in_data_buf, p_output->in_data_buf_len);
	}

	icmd->err = 0;
}

static MSDCNVT_ICALL_TBL m_calls = {
	msdcnvt_icmd_cb_open,
	msdcnvt_icmd_cb_close,
	msdcnvt_icmd_cb_verify,
	msdcnvt_icmd_cb_vendor,
};

MSDCNVT_ICALL_TBL *msdcnvt_get_ipc_call_tbl(UINT32 *p_cnt)
{
	if (p_cnt) {
		*p_cnt = sizeof(m_calls) / sizeof(MSDCNVT_IAPI);
	}
	return &m_calls;
}

UINT32 msdcnvt_get_phy(UINT32 addr)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	UINT32 shm_begin = (UINT32)p_ctrl->ipc.p_cfg;
	UINT32 shm_end = shm_begin + CFG_NVTMSDC_SIZE_IPC_DATA + CFG_MIN_HOST_MEM_SIZE;

	if (addr == 0) {
		return 0;
	} else if (addr < shm_begin || addr > shm_end) {
		DBG_ERR("invalid addr: %08X, valid(%08X ~ %08X)\r\n",
			(unsigned int)addr,
			(unsigned int)shm_begin,
			(unsigned int)shm_end);
		return 0;
	} else {
		return (unsigned int)(addr - shm_begin);
	}
}

UINT32 msdcnvt_get_noncache(UINT32 addr)
{
	// for pure linux, use cache only
	return msdcnvt_get_cache(addr);
}

UINT32 msdcnvt_get_cache(UINT32 addr)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	UINT32 shm_begin = (UINT32)p_ctrl->ipc.p_cfg;
	UINT32 shm_end = shm_begin + CFG_NVTMSDC_SIZE_IPC_DATA + CFG_MIN_HOST_MEM_SIZE;
	UINT32 cache_mem = shm_begin + addr;

	if (addr == 0) {
		return 0;
	} else if (cache_mem < shm_begin || cache_mem > shm_end) {
		DBG_ERR("invalid addr: %08X, valid(%08X ~ %08X)\r\n",
			(unsigned int)cache_mem,
			(unsigned int)shm_begin,
			(unsigned int)shm_end);
		return 0;
	} else {
		return (unsigned int)(addr + shm_begin);
	}
}