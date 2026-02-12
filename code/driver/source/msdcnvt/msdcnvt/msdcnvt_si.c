#include "msdcnvt_int.h"

//Used CMD Field to be Command Type
#define CDB_IDX_CALL_ID         12   //!< Use CDB[12] to be Function ID

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_SI_CALL Message
//------------------------------------------------------------------------------
#define CDB_11_SI_CALL_UNKNOWN                  0x00 //!< Invalid Cmd
#define CDB_11_SI_CALL_PC_GET_RUN_CMD           0x01 //!< Single Direction,from PC-Get Step 0, Use CDB[12] to be Function ID
#define CDB_11_SI_CALL_PC_GET_GET_RESULT        0x02 //!< Single Direction,from PC-Get Step 1, Use CDB[12] to be Function ID
#define CDB_11_SI_CALL_PC_SET                   0x03 //!< Single Direction,from PC-Set, Use CDB[12] to be Function ID

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_SI_CALL Handler
//------------------------------------------------------------------------------
static void msdcnvt_sicall_single_get_run_cmd(void *p_info);      //!< Send Data to PC (Single Direction)
static void msdcnvt_sicall_single_get_result(void *p_info);   //!< Send Data to PC (Single Direction)
static void msdcnvt_sicall_single_set(void *p_info);             //!< Recevice PC (Single Direction)
static void msdcnvt_sicall_run_cmd_get(void);                     //!< Callback for Background Thread Run Command for Get
static void msdcnvt_sicall_run_cmd_set(void);                     //!< Callback for Background Thread Run Command for Set

//Local Variables
static MSDCNVT_FUNCTION_CMD_HANDLE m_call_map[] = {
	{CDB_11_SI_CALL_UNKNOWN, NULL},
	{CDB_11_SI_CALL_PC_GET_RUN_CMD,     msdcnvt_sicall_single_get_run_cmd},    //Get Step 0: Run Command
	{CDB_11_SI_CALL_PC_GET_GET_RESULT,  msdcnvt_sicall_single_get_result}, //Get Setp 1: Get Result Data
	{CDB_11_SI_CALL_PC_SET,             msdcnvt_sicall_single_set},
};

void msdcnvt_function_sicall(void)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;
	UINT8 cmd_id = p_host->p_cbw->cb[11];

	if (cmd_id < sizeof(m_call_map) / sizeof(MSDCNVT_FUNCTION_CMD_HANDLE)
		&& cmd_id == m_call_map[cmd_id].call_id
		&& m_call_map[cmd_id].fp_call != NULL) {
		m_call_map[cmd_id].fp_call(NULL); //p_info is not used
	} else {
		DBG_ERR("Call Map Wrong!\r\n");
	}
}

static void msdcnvt_sicall_single_get_run_cmd(void *p_info)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;
	MSDCNVT_SI_CALL_CTRL *p_si_ctrl = &p_ctrl->sicall;

	p_si_ctrl->call_id = p_host->p_cbw->cb[CDB_IDX_CALL_ID];
	msdcnvt_mem_push_host_to_bk();
	msdcnvt_bk_run_cmd(msdcnvt_sicall_run_cmd_get);
}

static void msdcnvt_sicall_single_get_result(void *p_info)
{
	msdcnvt_mem_pop_bk_to_host();
}

static void msdcnvt_sicall_single_set(void *p_info)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;
	MSDCNVT_SI_CALL_CTRL *p_si_ctrl = &p_ctrl->sicall;

	p_si_ctrl->call_id = p_host->p_cbw->cb[CDB_IDX_CALL_ID];
	msdcnvt_mem_push_host_to_bk();
	msdcnvt_bk_run_cmd(msdcnvt_sicall_run_cmd_set);
	msdcnvt_mem_pop_bk_to_host();
}

static void msdcnvt_sicall_run_cmd_get(void)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_SI_CALL_CTRL *p_si_ctrl = &p_ctrl->sicall;

	UINT32 call_id = p_si_ctrl->call_id;

	if (call_id >= p_si_ctrl->num_gets) {
		DBG_ERR("function index out of range\r\n");
		return;
	}

	p_si_ctrl->fp_tbl_get[call_id]();
}

static void msdcnvt_sicall_run_cmd_set(void)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_SI_CALL_CTRL *p_si_ctrl = &p_ctrl->sicall;

	UINT32 call_id = p_si_ctrl->call_id;

	if (call_id >= p_si_ctrl->num_sets) {
		DBG_ERR("function index out of range\r\n");
		return;
	}

	p_si_ctrl->fp_tbl_set[call_id]();
}