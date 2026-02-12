#include "isf_vdoprc_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoprc_ext
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoprc_ext_debug_level = NVT_DBG_WRN;
//module_param_named(isf_vdoprc_ext_debug_level, isf_vdoprc_ext_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_vdoprc_ext_debug_level, "vdoprc_ext debug level");

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)	vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)	vk_spin_unlock_irqrestore(&my_lock, flags)
///////////////////////////////////////////////////////////////////////////////

#define FLG_ID_VDOPRC_EXT_ALL           0xFFFFFFFF
#define FLG_ID_VDOPRC_EXT_DATA_IN       FLGPTN_BIT(0)
#define FLG_ID_VDOPRC_EXT_STOP          FLGPTN_BIT(30)
#define FLG_ID_VDOPRC_EXT_IDLE          FLGPTN_BIT(31)

static UINT32 g_vdoprc_ext_open = 0;

ISF_RV _isf_vdoprc_ext_tsk_open(void)
{
	if (g_vdoprc_ext_open) {
		g_vdoprc_ext_open ++;
		return ISF_OK;
	}
    clr_flg(FLG_ID_VDOPRC_EXT, FLG_ID_VDOPRC_EXT_ALL); //clear all flag
    THREAD_CREATE(ISF_VDOPRC_EXT_TSK_ID, isf_vdoprc_ext_tsk, NULL, "isf_vext_tsk");
	if (ISF_VDOPRC_EXT_TSK_ID == 0) {
		DBG_ERR("task create fail!\r\n");
		return ISF_ERR_FAILED;
	}
    THREAD_SET_PRIORITY(ISF_VDOPRC_EXT_TSK_ID, ISF_VDOPRC_EXT_TSK_PRI);
    THREAD_RESUME(ISF_VDOPRC_EXT_TSK_ID);
	g_vdoprc_ext_open ++;
	return ISF_OK;
}

static void _isf_vdoprc_ext_tsk_cancel_data(ISF_UNIT *p_thisunit, UINT32 ext_pid);

ISF_RV _isf_vdoprc_ext_tsk_close(void)
{
	FLGPTN          flag = 0;

	if (g_vdoprc_ext_open == 0) {
		DBG_ERR("already close or not start yet!\r\n");
		return ISF_ERR_NOT_OPEN_YET;
	}
	if (g_vdoprc_ext_open > 1) {
		g_vdoprc_ext_open--;
		return ISF_OK;
	}
	clr_flg(FLG_ID_VDOPRC_EXT, FLG_ID_VDOPRC_EXT_IDLE); //clear IDLE before send cmd
	set_flg(FLG_ID_VDOPRC_EXT, FLG_ID_VDOPRC_EXT_STOP); //send STOP cmd
	if (wai_flg(&flag, FLG_ID_VDOPRC_EXT, FLG_ID_VDOPRC_EXT_IDLE, TWF_ORW) != E_OK) { //wait IDLE (stopped)
		DBG_WRN("ctrl-c break, wai_flg() is forced quit!\r\n");
	}
	//THREAD_DESTROY(ISF_VDOPRC_EXT_TSK_ID);  //already call THREAD_RETURN() in task, do not call THREAD_DESTROY() here.
	ISF_VDOPRC_EXT_TSK_ID = 0; //clear task id
	g_vdoprc_ext_open--;
	return ISF_OK;
}

ISF_RV _isf_vdoprc_ext_tsk_trigger_proc(ISF_UNIT *p_thisunit, UINT32 ext_pid, UINT32 src_path, ISF_DATA* p_data)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = ext_pid - ISF_VDOPRC_PHY_OUT_NUM;
	VDOPRC_EXT_CONTEXT* p_ext_ctx = &(p_ctx->out_ext[i]);
	unsigned long flags = 0;
	if (p_data == NULL) {
		DBG_ERR("invalid p_data 1\r\n");
		return ISF_ERR_INVALID_DATA;
	}
	if (g_vdoprc_ext_open == 0) {
		return ISF_ERR_QUEUE_FULL;
	}
	if (p_ext_ctx->is_proc == TRUE) {
		return ISF_ERR_QUEUE_FULL;
	}
	p_thisunit->p_base->do_add(p_thisunit, ISF_OUT(ext_pid), p_data, ISF_OUT_PROBE_PUSH);
	loc_cpu(flags);
	p_ext_ctx->is_proc = TRUE;
	p_ext_ctx->src_path = src_path;
	p_ext_ctx->in_data = p_data[0];
	p_ext_ctx->p_data = &(p_ext_ctx->in_data);
	unl_cpu(flags);
	// trigger task to process data 
	set_flg(FLG_ID_VDOPRC_EXT, FLG_ID_VDOPRC_EXT_DATA_IN);
	return ISF_OK;
}

static void _isf_vdoprc_ext_tsk_proc_data(ISF_UNIT *p_thisunit, UINT32 ext_pid)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = ext_pid - ISF_VDOPRC_PHY_OUT_NUM;
	VDOPRC_EXT_CONTEXT* p_ext_ctx = &(p_ctx->out_ext[i]);
	BOOL is_proc;
	UINT32 src_path;
	ISF_DATA* p_data;
	ISF_DATA* p_tmp_data;
	unsigned long flags = 0;
	loc_cpu(flags);
	is_proc = p_ext_ctx->is_proc;
	src_path = p_ext_ctx->src_path;
	p_data = p_ext_ctx->p_data;
	unl_cpu(flags);
	// process data 
	if (is_proc != TRUE) {
		return;
	}
	if (p_data == NULL) {
		DBG_ERR("invalid p_data 2\r\n");
		return;
	}
	p_tmp_data = p_ctx->out_rot[src_path].p_data;
	_isf_vdoprc_oport_do_dispatch_out_ext(p_thisunit, ISF_OUT(ext_pid), p_data, 0, p_tmp_data);
	p_thisunit->p_base->do_release(p_thisunit, ISF_OUT(ext_pid), p_data, ISF_OUT_PROBE_PUSH);
	loc_cpu(flags);
	p_ext_ctx->is_proc = FALSE;
	p_ext_ctx->src_path = 0;
	p_ext_ctx->p_data = 0;
	unl_cpu(flags);
}

static void _isf_vdoprc_ext_tsk_cancel_data(ISF_UNIT *p_thisunit, UINT32 ext_pid)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = ext_pid - ISF_VDOPRC_PHY_OUT_NUM;
	VDOPRC_EXT_CONTEXT* p_ext_ctx = &(p_ctx->out_ext[i]);
	BOOL is_proc;
	UINT32 src_path;
	ISF_DATA* p_data;
	ISF_DATA* p_tmp_data;
	unsigned long flags = 0;
	loc_cpu(flags);
	is_proc = p_ext_ctx->is_proc;
	src_path = p_ext_ctx->src_path;
	p_data = p_ext_ctx->p_data;
	unl_cpu(flags);
	// process data 
	if (is_proc != TRUE) {
		return;
	}
	if (p_data == NULL) {
		DBG_ERR("invalid p_data 3\r\n");
		return;
	}
	p_tmp_data = p_ctx->out_rot[src_path].p_data;
	if (p_tmp_data) {
		p_thisunit->p_base->do_release(p_thisunit, ISF_OUT(ext_pid), p_tmp_data, ISF_OUT_PROBE_PUSH);
	}
	p_thisunit->p_base->do_release(p_thisunit, ISF_OUT(ext_pid), p_data, ISF_OUT_PROBE_PUSH);
	loc_cpu(flags);
	p_ext_ctx->is_proc = FALSE;
	p_ext_ctx->src_path = 0;
	p_ext_ctx->p_data = 0;
	unl_cpu(flags);
}

THREAD_DECLARE(isf_vdoprc_ext_tsk, arglist)
{
	FLGPTN flag = 0;
	BOOL is_continue = 1;

	THREAD_ENTRY();
	
	//clr_flg(FLG_ID_VDOPRC_EXT, FLG_ID_VDOPRC_EXT_IDLE);

	while (is_continue) {
		//set_flg(FLG_ID_VDOPRC_EXT, FLG_ID_VDOPRC_EXT_IDLE);
		PROFILE_TASK_IDLE();
		wai_flg(&flag, FLG_ID_VDOPRC_EXT, FLG_ID_VDOPRC_EXT_DATA_IN | FLG_ID_VDOPRC_EXT_STOP, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		//clr_flg(FLG_ID_VDOPRC_EXT, FLG_ID_VDOPRC_EXT_IDLE);
		DBG_IND("flag=0x%x\r\n", flag);
		if (flag & FLG_ID_VDOPRC_EXT_STOP) {
			UINT32 dev;
			for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
				ISF_UNIT *p_thisunit = (ISF_UNIT*)(DEV_UNIT(dev));
				VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
				UINT32 pid;
				for (pid = ISF_VDOPRC_PHY_OUT_NUM; pid < ISF_VDOPRC_OUT_NUM; pid++) {
					//XDATA
					if (!(p_ctx->phy_mask & (1<<pid))) {
						_isf_vdoprc_ext_tsk_cancel_data(p_thisunit, pid);
					}
				}
			}
			is_continue = 0;
			continue;
		}
		if (flag & FLG_ID_VDOPRC_EXT_DATA_IN) {
			UINT32 dev;
			for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
				ISF_UNIT *p_thisunit = (ISF_UNIT*)(DEV_UNIT(dev));
				VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
				UINT32 pid;
				ISF_DATA* p_tmp_data = NULL;
				for (pid = 0; pid < ISF_VDOPRC_PHY_OUT_NUM; pid++) {
					p_tmp_data = 0;
					if (_vdoprc_is_out_rotate(p_thisunit, ISF_OUT(pid))) {
						p_tmp_data = &(p_ctx->out_rot[pid].rotate_data); 
						p_tmp_data->sign = 0;
					}
					p_ctx->out_rot[pid].p_data = p_tmp_data;
				}
				for (pid = ISF_VDOPRC_PHY_OUT_NUM; pid < ISF_VDOPRC_OUT_NUM; pid++) {
					//XDATA
					if (!(p_ctx->phy_mask & (1<<pid))) {
						_isf_vdoprc_ext_tsk_proc_data(p_thisunit, pid);
					}
				}
				for (pid = 0; pid < ISF_VDOPRC_PHY_OUT_NUM; pid++) {
					p_tmp_data = p_ctx->out_rot[pid].p_data;
					if ((p_tmp_data != NULL) && (p_tmp_data->sign != 0)) {
						p_thisunit->p_base->do_release(p_thisunit, ISF_OUT(pid), p_tmp_data, ISF_OUT_PROBE_EXT);
						p_tmp_data->sign = 0;
					}
					p_ctx->out_rot[pid].p_data = 0;
				}
			}
		}
	}
	set_flg(FLG_ID_VDOPRC_EXT, FLG_ID_VDOPRC_EXT_IDLE);

	THREAD_RETURN(0);
}

