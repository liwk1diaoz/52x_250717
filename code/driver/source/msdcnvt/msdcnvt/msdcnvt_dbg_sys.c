#include "msdcnvt_int.h"

#if defined(__UITRON) //only uitron supports DbgSys
#define CFG_MAX_COMMAND_LINE_BYTES              64   //!< Command Task Line Max Lens

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_DBGSYS Message
//------------------------------------------------------------------------------
#define CDB_11_DBGSYS_UNKNOWN                   0x00 //!< Invalid Cmd
#define CDB_11_DBGSYS_OPEN                      0x01 //!< Stopping Command Tsk and Hooking debug_msg
#define CDB_11_DBGSYS_CLOSE                     0x02 //!< Stopping DbgSys
#define CDB_11_DBGSYS_QUERY_MSG                 0x03 //!< Query Msg is Existing in Buffer queue
#define CDB_11_DBGSYS_GET_MSG                   0x04 //!< Getting Msg from Buffer queue
#define CDB_11_DBGSYS_SEND_CMD                  0x05 //!< Sending Command from PC -> RTOS
#define CDB_11_DBGSYS_PROPERTY_SET              0x06 //!< Set Some Property from PC
#define CDB_11_DBGSYS_PROPERTY_GET              0x07 //!< Get Some Property from PC

//------------------------------------------------------------------------------
// CDB_11_DBGSYS_SET_PROPERTY / CDB_11_DBGSYS_GET_PROPERTY Message
//------------------------------------------------------------------------------
#define CDB_12_DBGSYS_PROPERTY_UNKNOWN          0x00 //!< Invalid type
#define CDB_12_DBGSYS_ALSO_OUTPUT_UART          0x01 //!< Also Output Uart Msg (ENABLE / DISABLE)

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_DBGSYS Data type
//------------------------------------------------------------------------------
//parent for Host <- Device
typedef struct _DBGSYS_RESPONSE {
	UINT32 size;        //!< Structure Size
	UINT32 b_handled;   //!< Indicate Device Handle this function (Don't Use BOOL, for 433 Compatiable)
	UINT32 b_ok;        //!< Indicate Function Action is OK or Not (Don't Use BOOL, for 433 Compatiable)
	UINT8  reversed[2]; //!< reversed value for Structure DWORD Alignment
} DBGSYS_RESPONSE;

//parent for Host -> PC
typedef struct _DBGSYS_DELIVER {
	UINT32 size; //!< Structure Size
} DBGSYS_DELIVER;

typedef struct _DBGSYS_MSG_QUERY {
	DBGSYS_RESPONSE response;  //!< parent
	UINT32          msg_bytes; //!< nBytes Msg Need to be get
} DBGSYS_MSG_QUERY;

typedef struct _DBGSYS_MSG_GET {
	DBGSYS_RESPONSE response;  //!< parent
	UINT32          msg_bytes; //!< nBytes Msg in following buffer
} DBGSYS_MSG_GET;

typedef struct _DBGSYS_CMD_SEND {
	DBGSYS_DELIVER  deliver;   //!< parent
	UINT32          cmd_bytes; //!< Following Data Size
} DBGSYS_CMD_SEND;

typedef struct _DBGSYS_PROPERTY_GET {
	DBGSYS_RESPONSE response; //!< parent
	UINT32           type;    //!< Property type
	UINT32           value;   //!< Property value
} DBGSYS_PROPERTY_GET;

typedef struct _DBGSYS_PROPERTY_SET {
	DBGSYS_DELIVER  deliver; //!< parent
	UINT32           type;   //!< Property type
	UINT32           value;  //!< Property value
} DBGSYS_PROPERTY_SET;


//------------------------------------------------------------------------------
// CDB_10_FUNCTION_DBGSYS Handler
//------------------------------------------------------------------------------
static ER   dbgsys_lock(void);
static ER   dbgsys_unlock(void);
static void dbgsys_open(void *p_info);
static void dbgsys_close(void *p_info);
static void dbgsys_msg_query(void *p_info);       //!< Query Is There Existing Msg Needs to Flush
static void dbgsys_msg_get(void *p_info);         //!< Get Buffer Address and Length
static void dbgsys_cmd_send(void *p_info);        //!< Send Command Line To CommandTsk
static void dbgsys_property_set(void *p_info);    //!< Set Some DbgSys Property
static void dbgsys_property_get(void *p_info);    //!< Get Some DbgSys Property

//Provide Callback functions
static ER   dbgsys_msg_receive(CHAR *p_msg);              //!< Callback for receive message
static void dbgsys_run_cmd(void);                //!< Callback for Background Thread Run Command
static void dbgsys_hook(void);
static void dbgsys_unhook(void);
static void dbgsys_puts(const char *s);
static int  dbgsys_gets(char *s, int len);

static MSDCNVT_FUNCTION_CMD_HANDLE m_call_map[] = {
	{CDB_11_DBGSYS_UNKNOWN, NULL},
	{CDB_11_DBGSYS_OPEN, dbgsys_open},
	{CDB_11_DBGSYS_CLOSE, dbgsys_close},
	{CDB_11_DBGSYS_QUERY_MSG, dbgsys_msg_query},
	{CDB_11_DBGSYS_GET_MSG, dbgsys_msg_get},
	{CDB_11_DBGSYS_SEND_CMD, dbgsys_cmd_send},
	{CDB_11_DBGSYS_PROPERTY_SET, dbgsys_property_set},
	{CDB_11_DBGSYS_PROPERTY_GET, dbgsys_property_get},
};

static console m_con_usb = {dbgsys_hook, dbgsys_unhook, dbgsys_puts, dbgsys_gets};

//Debug System Vendor Command
void msdcnvt_function_dbgsys(void)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;
	UINT8 cmd_id = p_host->p_cbw->cb[11];
	void *p_info = p_host->p_pool;

	if (!p_ctrl->dbgsys.b_init) {
		DBG_ERR("has not been initial\r\n");
		return;
	}

	if (cmd_id < sizeof(m_call_map) / sizeof(MSDCNVT_FUNCTION_CMD_HANDLE)
		&& cmd_id == m_call_map[cmd_id].call_id
		&& m_call_map[cmd_id].fp_call != NULL) {
		m_call_map[cmd_id].fp_call(p_info);
	} else {
		DBG_ERR("Call Map Wrong!\r\n");
	}
}
static ER dbgsys_lock(void)
{
	return wai_sem(msdcnvt_get_ctrl()->dbgsys.sem_id);
}

static ER dbgsys_unlock(void)
{
	return sig_sem(msdcnvt_get_ctrl()->dbgsys.sem_id);
}

static void dbgsys_hook(void)
{
	clr_flg(msdcnvt_get_ctrl()->flag_id, FLGDBGSYS_CMD_ABORT);
}

static void dbgsys_unhook(void)
{
	set_flg(msdcnvt_get_ctrl()->flag_id, FLGDBGSYS_CMD_ABORT);
}

static void dbgsys_puts(const char *s)
{
	dbgsys_msg_receive((CHAR *)s);
}

int dbgsys_gets(char *s, int len)
{
	FLGPTN flg_ptn;
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_BACKGROUND_CTRL *p_bk = &p_ctrl->bk;

	set_flg(p_ctrl->flag_id, FLGDBGSYS_CMD_FINISH);
	wai_flg(&flg_ptn, p_ctrl->flag_id, FLGDBGSYS_CMD_ABORT | FLGDBGSYS_CMD_ARRIVAL, TWF_ORW | TWF_CLR);

	if (flg_ptn & FLGDBGSYS_CMD_ARRIVAL) {
		strncpy(s, (char *)p_bk->p_pool, len - 1);
		return strlen(s);
	} else if (flg_ptn & FLGDBGSYS_CMD_ABORT) {
		//we have to keep ABORT Status for blocking in dbgsys_gets(). Only dbgsys_unhook can clean it
		set_flg(p_ctrl->flag_id, FLGDBGSYS_CMD_ABORT);
		s[0] = 0;
		return 0;
	}

	return 0;
}

static void dbgsys_open(void *p_info)
{
	UINT32 i;
	DBGSYS_RESPONSE *p_response = (DBGSYS_RESPONSE *)p_info;
	MSDCNVT_DBGSYS_CTRL *p_dbgsys = &msdcnvt_get_ctrl()->dbgsys;

	memset(p_response, 0, sizeof(DBGSYS_RESPONSE));
	p_response->size = sizeof(DBGSYS_RESPONSE);
	p_response->b_handled = TRUE;

	dbgsys_lock();

	p_dbgsys->msg_in_idx = p_dbgsys->msg_out_idx = 0;

	for (i = 0; i < p_dbgsys->payload_num; i++) {
		p_dbgsys->queue[i].byte_cnt = 0;
	}

	debug_reg_console(CON_2, &m_con_usb);
	debug_set_console(CON_2);

	dbgsys_unlock();

	if (msdcnvt_get_ctrl()->fp_event) {
		msdcnvt_get_ctrl()->fp_event(MSDCNVT_EVENT_HOOK_DBG_MSG_ON);
	}

	p_response->b_ok = TRUE;
}

static void dbgsys_close(void *p_info)
{
	DBGSYS_RESPONSE *p_response = (DBGSYS_RESPONSE *)p_info;

	memset(p_response, 0, sizeof(DBGSYS_RESPONSE));
	p_response->size = sizeof(DBGSYS_RESPONSE);
	p_response->b_handled = TRUE;

	dbgsys_lock();
	debug_set_console(CON_1);
	dbgsys_unlock();

	if (msdcnvt_get_ctrl()->fp_event) {
		msdcnvt_get_ctrl()->fp_event(MSDCNVT_EVENT_HOOK_DBG_MSG_OFF);
	}

	p_response->b_ok = TRUE;
}

static void dbgsys_msg_query(void *p_info)
{
	DBGSYS_MSG_QUERY *p_msg_query = (DBGSYS_MSG_QUERY *)p_info;
	DBGSYS_RESPONSE  *p_response = (DBGSYS_RESPONSE *)p_info;
	MSDCNVT_DBGSYS_CTRL *p_dbgsys = &msdcnvt_get_ctrl()->dbgsys;

	memset(p_msg_query, 0, sizeof(DBGSYS_MSG_QUERY));
	p_response->size = sizeof(DBGSYS_MSG_QUERY);
	p_response->b_handled = TRUE;

	p_msg_query->msg_bytes = p_dbgsys->queue[p_dbgsys->msg_out_idx].byte_cnt;

	p_response->b_ok = TRUE;
}

static void dbgsys_msg_get(void *p_info)
{
	DBGSYS_MSG_GET *p_msg_get = (DBGSYS_MSG_GET *)p_info;
	UINT8 *p_dst = (UINT8 *)p_msg_get + sizeof(DBGSYS_MSG_GET);
	DBGSYS_RESPONSE *p_response = &p_msg_get->response;
	MSDCNVT_DBGSYS_CTRL *p_dbgsys = &msdcnvt_get_ctrl()->dbgsys;

	memset(p_msg_get, 0, sizeof(DBGSYS_MSG_GET));
	p_response->size = sizeof(DBGSYS_MSG_GET);
	p_response->b_handled = TRUE;

	if (p_dbgsys->queue[p_dbgsys->msg_out_idx].byte_cnt == 0) {
		*p_dst = 0;
		p_msg_get->msg_bytes = 0;
		p_response->b_ok = FALSE;
		return;
	}

	p_msg_get->msg_bytes = p_dbgsys->queue[p_dbgsys->msg_out_idx].byte_cnt;
	memcpy(p_dst, p_dbgsys->queue[p_dbgsys->msg_out_idx].p_msg, p_msg_get->msg_bytes);

	dbgsys_lock();
	p_dbgsys->queue[p_dbgsys->msg_out_idx].byte_cnt = 0;
	p_dbgsys->msg_out_idx = (p_dbgsys->msg_out_idx + 1)&p_dbgsys->msg_cnt_mask;
	dbgsys_unlock();

	p_response->b_ok = TRUE;
}

static void dbgsys_cmd_send(void *p_info)
{
	UINT32 i;
	MSDCNVT_BACKGROUND_CTRL *p_bk = &msdcnvt_get_ctrl()->bk;
	DBGSYS_CMD_SEND *p_cmd_send = (DBGSYS_CMD_SEND *)p_info;
	UINT8 *pcmd_line = (UINT8 *)p_cmd_send + sizeof(DBGSYS_CMD_SEND);

	if (p_cmd_send->deliver.size != sizeof(DBGSYS_CMD_SEND)) {
		DBG_ERR("Not Handled\r\n");
		return;
	}

	p_bk->trans_size = CFG_MAX_COMMAND_LINE_BYTES;
	memcpy(p_bk->p_pool, pcmd_line, p_bk->trans_size);

	for (i = 0; i < CFG_MAX_COMMAND_LINE_BYTES; i++) {
		p_bk->p_pool[i] = pcmd_line[i];
		if (p_bk->p_pool[i] == 0x0D) {
			p_bk->p_pool[i] = 0;
			break;
		}
	}

	msdcnvt_bk_run_cmd(dbgsys_run_cmd);
}

static void dbgsys_run_cmd(void)
{
	FLGPTN flg_ptn;
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();

	clr_flg(p_ctrl->flag_id, FLGDBGSYS_CMD_FINISH);
	set_flg(p_ctrl->flag_id, FLGDBGSYS_CMD_ARRIVAL);
	//wait DoCommand finish
	wai_flg(&flg_ptn, p_ctrl->flag_id, FLGDBGSYS_CMD_FINISH, TWF_ORW | TWF_CLR);
}

static void dbgsys_property_set(void *p_info)
{
	DBGSYS_PROPERTY_SET *p_property = (DBGSYS_PROPERTY_SET *)p_info;
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;
	MSDCNVT_DBGSYS_CTRL *p_dbgsys = &p_ctrl->dbgsys;
	UINT32 type = p_host->p_cbw->cb[12];

	if (p_property->deliver.size != sizeof(DBGSYS_PROPERTY_SET)) {
		DBG_ERR("Not Handled\r\n");
		return;
	}

	switch (type) {
	case CDB_12_DBGSYS_ALSO_OUTPUT_UART:
		p_dbgsys->b_no_uart_output = (p_property->value) ? FALSE : TRUE;
		break;
	}
}

static void dbgsys_property_get(void *p_info)
{
	DBGSYS_PROPERTY_GET *p_property = (DBGSYS_PROPERTY_GET *)p_info;
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;
	MSDCNVT_DBGSYS_CTRL *p_dbgsys = &p_ctrl->dbgsys;
	UINT32 type = p_host->p_cbw->cb[12];

	if (p_property->response.size != sizeof(DBGSYS_PROPERTY_GET)) {
		DBG_ERR("Not Handled\r\n");
		return;
	}

	switch (type) {
	case CDB_12_DBGSYS_ALSO_OUTPUT_UART:
		p_property->type = CDB_12_DBGSYS_ALSO_OUTPUT_UART;
		p_property->value = (p_dbgsys->b_no_uart_output) ? FALSE : TRUE;
		break;
	}
}

static ER dbgsys_msg_receive(CHAR *p_msg)
{
	UINT32 n_cnt;
	CHAR *p_dst;
	CHAR *p_src = p_msg;
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_DBGSYS_CTRL *p_dbgsys = &p_ctrl->dbgsys;
	UINT32  max_len = p_dbgsys->payload_size - 1;

	p_dst = (CHAR *)p_dbgsys->queue[p_dbgsys->msg_in_idx].p_msg;

	n_cnt = 0;
	while ((*p_msg) != 0 && max_len--) {
		*(p_dst++) = *(p_msg++);
		n_cnt++;
	}

	*p_dst = 0;
	n_cnt++;

	dbgsys_lock();
	p_dbgsys->queue[p_dbgsys->msg_in_idx].byte_cnt = n_cnt;
	p_dbgsys->msg_in_idx = (p_dbgsys->msg_in_idx + 1)&p_dbgsys->msg_cnt_mask;
	dbgsys_unlock();

	if (!p_dbgsys->b_no_uart_output) {
		p_dbgsys->fp_uart_put_string(p_src);
	}

	//Check Next Payload is Empty
	n_cnt = p_dbgsys->queue[p_dbgsys->msg_in_idx].byte_cnt;
	if (p_dbgsys->msg_in_idx == p_dbgsys->msg_out_idx && n_cnt != 0) {
		INT32 n_retry = 500; //Retry 5 sec for Crash Detect
		//Wait to Free Payload by PC Getting
		while (p_dbgsys->msg_in_idx == p_dbgsys->msg_out_idx && n_retry--) {
			swtimer_delayms(10);
		}

		if (n_retry < 0) {
			if (kchk_flg(p_ctrl->flag_id, FLGDBGSYS_CMD_ABORT) != 0) {
				//Fix Bug for both Running telnet message and update fw then update is failed!
				debug_set_console(CON_1);
				if (msdcnvt_get_ctrl()->fp_event) {
					msdcnvt_get_ctrl()->fp_event(MSDCNVT_EVENT_HOOK_DBG_MSG_OFF);
				}
				DBG_ERR("closed, due to buffer not empty,p_dbgsys->msg_in_idx=%d\r\n", p_dbgsys->msg_in_idx);
				return E_OK;
			}
		}
	}

	return E_OK;
}
#endif
