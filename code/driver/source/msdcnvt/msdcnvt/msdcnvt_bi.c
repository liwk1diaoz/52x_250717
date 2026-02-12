#include "msdcnvt_int.h"

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_BI_CALL Message
//------------------------------------------------------------------------------
#define CDB_11_BI_CALL_UNKNOWN                  0x00 ///< Invalid Cmd
#define CDB_11_BI_CALL_GET_PROC_STEP0           0x01 ///< Get Function Proc Address (PC Send Function String To F.W)
#define CDB_11_BI_CALL_GET_PROC_STEP1           0x02 ///< Get Function Proc Address (PC Query, Then F.W Return Function Point)
#define CDB_11_BI_CALL_CALL                     0x03 ///< Call Function,
#define CDB_11_BI_CALL_GET_DATA                 0x04 ///< Get  Result Data after Finish Function

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_BI_CALL Handler
//------------------------------------------------------------------------------
static void msdcnvt_bicall_get_proc_step_0(void *p_info); ///< PC Query Function Name
static void msdcnvt_bicall_get_proc_step_1(void *p_info); ///< Return Function Point Address to PC
static void msdcnvt_bicall_call(void *p_info);          ///< Recevice PC Data & Call Function (Bi-Direction Step 1)
static void msdcnvt_bicall_get_data(void *p_info);       ///< Send Result Data to PC (Bi-Direction Step 2)
static void msdcnvt_bicall_run_cmd(void);               ///< Callback for Background Thread Run Command

//Local Variables
static MSDCNVT_FUNCTION_CMD_HANDLE m_call_map[] = {
	{CDB_11_BI_CALL_UNKNOWN, NULL},
	{CDB_11_BI_CALL_GET_PROC_STEP0, msdcnvt_bicall_get_proc_step_0},
	{CDB_11_BI_CALL_GET_PROC_STEP1, msdcnvt_bicall_get_proc_step_1},
	{CDB_11_BI_CALL_CALL, msdcnvt_bicall_call},
	{CDB_11_BI_CALL_GET_DATA, msdcnvt_bicall_get_data},
};

void msdcnvt_function_bicall(void)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;

	UINT8 cmd_id = p_host->p_cbw->cb[11];
	void *p_info = p_host->p_pool;

	if (cmd_id < sizeof(m_call_map) / sizeof(MSDCNVT_FUNCTION_CMD_HANDLE)
		&& cmd_id == m_call_map[cmd_id].call_id
		&& m_call_map[cmd_id].fp_call != NULL) {
		m_call_map[cmd_id].fp_call(p_info);
	} else {
		DBG_ERR("Call Map Wrong!\r\n");
	}
}

static void msdcnvt_bicall_get_proc_step_0(void *p_info)
{
	msdcnvt_mem_push_host_to_bk(); //Push Name to Bk
}

static void msdcnvt_bicall_get_proc_step_1(void *p_info)
{
	BOOL     b_find = FALSE;
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;
	MSDCNVT_BI_CALL_CTRL *p_bi = &p_ctrl->bicall;
	MSDCNVT_BACKGROUND_CTRL *p_bk = &p_ctrl->bk;
	char    *name = (char *)p_bk->p_pool;
	UINT32  *p_proc_addr = (UINT32 *)p_host->p_pool;
	MSDCNVT_BI_CALL_UNIT *p_curr = p_bi->p_head;

	while (p_curr != NULL && p_curr->fp != NULL) {
		if (strcmp(name, p_curr->p_name) == 0) {
			b_find = TRUE;
			*p_proc_addr = (UINT32)p_curr->fp;
			break;
		}

		p_curr++;
		if (p_curr->p_name == NULL
			&& p_curr->fp != NULL) {
			p_curr = (MSDCNVT_BI_CALL_UNIT *)p_curr->fp;
		}
	}

	if (!b_find) {
		DBG_ERR("No Found: %s\r\n", name);
		*p_proc_addr = 0;
	}
	return;
}

static void msdcnvt_bicall_call(void *p_info)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;
	MSDCNVT_BI_CALL_CTRL *p_bi = &p_ctrl->bicall;

	p_bi->fp_call = (void (*)(void *))p_host->lb_address;

	if (p_bi->fp_call) {
		msdcnvt_mem_push_host_to_bk();
		msdcnvt_bk_run_cmd(msdcnvt_bicall_run_cmd);
	} else {
		DBG_ERR("gCtrl.fp_call==%d\r\n", (UINT32)p_bi->fp_call);
	}
}

static void msdcnvt_bicall_get_data(void *p_info)
{
	MSDCNVT_BI_CALL_CTRL *p_bi = &msdcnvt_get_ctrl()->bicall;

	if (p_bi->fp_call == NULL) {
		DBG_ERR("gCtrl.fp_call==%d\r\n", (UINT32)p_bi->fp_call);
	} else {
		msdcnvt_mem_pop_bk_to_host();
	}
	return;
}

static void msdcnvt_bicall_run_cmd(void)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_BACKGROUND_CTRL *p_bk = &p_ctrl->bk;
	MSDCNVT_BI_CALL_CTRL *p_bi = &p_ctrl->bicall;

	p_bi->fp_call(p_bk->p_pool);
}