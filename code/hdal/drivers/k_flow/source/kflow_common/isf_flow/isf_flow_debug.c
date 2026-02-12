#include "isf_flow_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_flow_dg
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_flow_dg_debug_level = NVT_DBG_WRN;
//module_param_named(isf_flow_dg_debug_level, isf_flow_dg_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_flow_dg_debug_level, "flow debug level");
///////////////////////////////////////////////////////////////////////////////

#include <kwrap/file.h>

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)

#else
#include <linux/wait.h>
#endif

#include "comm/hwclock.h"

#define DEBUG_ALL		DISABLE
#define TRACE_ALL		DISABLE
#define PROBE_ALL		DISABLE

//vos_off_t vos_file_lseek(VOS_FILE vos_file, vos_off_t offset, int whence);
//int vos_file_fstat(VOS_FILE vos_file, struct vos_stat *p_stat);
//int vos_file_stat(const char *pathname, struct vos_stat *p_stat);

static int g_wo_fp = 0;
static int g_ro_fp = 0;

void debug_write_open(char *filename)
{
	g_wo_fp = vos_file_open(filename, O_CREAT|O_WRONLY|O_SYNC, 0);
	if (-1 == g_wo_fp) {
		DBG_ERR("open failed:%s\r\n", filename);
		return;
	}
}

void debug_write_data(void* p_data, UINT32 datasize)
{
	if (-1 != g_wo_fp) {
		int len = 0;
		len = vos_file_write(g_wo_fp, p_data, datasize);
		if (len != (int)datasize) {
			DBG_ERR("write failed\r\n");
			return;
		}
		if (vos_file_fsync(g_wo_fp) < 0) {  //force sync to storage
			DBG_ERR("fsync failed\r\n");
			return;
		}
	}
}

void debug_write_close(void)
{
	if (-1 != g_wo_fp) {
		vos_file_close(g_wo_fp);
	}
}

void debug_read_open(char *filename)
{
	g_ro_fp = vos_file_open(filename, O_RDONLY, 0);
	if (-1 == g_ro_fp) {
		DBG_ERR("open failed:%s\r\n", filename);
		return;
	}
}

void debug_read_data(void* p_data, UINT32 datasize)
{
	if (-1 != g_ro_fp) {
		int len = 0;
		len = vos_file_read(g_ro_fp, p_data, datasize);
		if (len != (int)datasize) {
			DBG_ERR("read failed\r\n");
			return;
		}
	}
}

void debug_read_close(void)
{
	if (-1 != g_ro_fp) {
		vos_file_close(g_ro_fp);
	}
}

#if 0 //not used yet
static CHAR _isf_dump_unit[33] = {0};
static INT32 _isf_dump_dir = -1; //0 is out, 1 is in, 0xff is any
static INT32 _isf_dump_port = -1; //0xff is any
#endif

#if (DEBUG_ALL == ENABLE)
static CHAR _isf_debug_unit[33] = {"*"};
static INT32 _isf_debug_dir = 0xff; //0 is out, 1 is in, 0xff is any
static INT32 _isf_debug_port = 0xff; //0xff is any
static UINT32 _isf_debug_mask = 0xffff; //0xfff is any
#else
static CHAR _isf_debug_unit[33] = {0};
static INT32 _isf_debug_dir = -1; //0 is out, 1 is in, 0xff is any
static INT32 _isf_debug_port = -1; //0xff is any
static UINT32 _isf_debug_mask = 0x0000; //0xfff is any
#endif

#if (TRACE_ALL == ENABLE)
static CHAR _isf_trace_unit[33] = {"*"};
static INT32 _isf_trace_dir = 0xff; //0 is out, 1 is in, 0xff is any
static INT32 _isf_trace_port = 0xff; //0xff is any
static UINT32 _isf_trace_mask = 0xffff; //0xfff is any
#else
static CHAR _isf_trace_unit[33] = {0};
static INT32 _isf_trace_dir = -1; //0 is out, 1 is in, 0xff is any
static INT32 _isf_trace_port = -1; //0xff is any
static UINT32 _isf_trace_mask = 0x0000; //0xfff is any
#endif

static CHAR _isf_perf_unit[33] = {0};
static INT32 _isf_perf_dir = -1; //0 is out, 1 is in, 0xff is any
static INT32 _isf_perf_port = -1; //0xff is any

#if (PROBE_ALL == ENABLE)
static CHAR _isf_probe_unit[33] = {"*"}; //c[0] == '*'
static INT32 _isf_probe_dir = 0; //0 is out, 1 is in
static INT32 _isf_probe_port = 0xff; //0xff is any
static UINT32 _isf_probe_mask = 0xffff; //0xffff is any
#else
static CHAR _isf_probe_unit[33] = {0}; //c[0] == '*'
static INT32 _isf_probe_dir = -1; //0 is out, 1 is in
static INT32 _isf_probe_port = -1; //0xff is any
static UINT32 _isf_probe_mask = 0x0000; //0xffff is any
#endif

//static INT32 _isf_save_strm = -1;
static INT32 _isf_save_unit_len = 0;
static CHAR _isf_save_unit[33] = {0};
static INT32 _isf_save_dir = -1; //0 is out, 1 is in
static INT32 _isf_save_port = -1;
static INT32 _isf_save_count = 0;

static CHAR portstr[2][4] = {"out","in"};

//debug
static CHAR msg_d1[] = _ANSI_C_"hd: \"%s\".%s[%d]: %s"_ANSI_0_"\r\n";
static CHAR msg_d2[] = _ANSI_G_"hd: \"%s\".%s[%d]: %s"_ANSI_0_"\r\n";
static CHAR msg_d3[] = _ANSI_C_"hd: \"%s\".ctrl: %s"_ANSI_0_"\r\n";
static CHAR msg_d4[] = _ANSI_G_"hd: \"%s\".ctrl: %s"_ANSI_0_"\r\n";
//trace
static CHAR msg_t1[] = _ANSI_W_"hd: \"%s\".%s[%d]: %s"_ANSI_0_"\r\n";
static CHAR msg_t2[] = _ANSI_W_"hd: \"%s\".%s[%d]: %s"_ANSI_0_"\r\n";
static CHAR msg_t3[] = _ANSI_W_"hd: \"%s\".ctrl: %s"_ANSI_0_"\r\n";
static CHAR msg_t4[] = _ANSI_W_"hd: \"%s\".ctrl: %s"_ANSI_0_"\r\n";
//each probe
#define msg_p0	_ANSI_W_"hd: \"%s\".%s[%d] - %s (result=%d) %s "_ANSI_0_"\r\n"
#define msg_p1	_ANSI_Y_"hd: \"%s\".%s[%d] - %s (result=%d) %s "_ANSI_0_"\r\n"
#define msg_p2	_ANSI_R_"hd: \"%s\".%s[%d] - %s (result=%d) %s "_ANSI_0_"\r\n"
//each new
#define msg_n0	_ANSI_W_"hd: \"%s\".%s[%d] - %s - new -- h=%08x size=%08x addr=%08x %s "_ANSI_0_"\r\n"
#define msg_n1	_ANSI_Y_"hd: \"%s\".%s[%d] - %s - new -- h=%08x size=%08x addr=%08x %s "_ANSI_0_"\r\n"
#define msg_n2	_ANSI_R_"hd: \"%s\".%s[%d] - %s - new -- h=%08x size=%08x addr=%08x %s "_ANSI_0_"\r\n"
//each add
#define msg_a0	_ANSI_W_"hd: \"%s\".%s[%d] - %s - add -- h=%08x (result=%d) %s "_ANSI_0_"\r\n"
#define msg_a1	_ANSI_Y_"hd: \"%s\".%s[%d] - %s - add -- h=%08x (result=%d) %s "_ANSI_0_"\r\n"
#define msg_a2	_ANSI_R_"hd: \"%s\".%s[%d] - %s - add -- h=%08x (result=%d) %s "_ANSI_0_"\r\n"
//each release
#define msg_r0	_ANSI_W_"hd: \"%s\".%s[%d] - %s - rel -- h=%08x (result=%d) %s "_ANSI_0_"\r\n"
#define msg_r1	_ANSI_Y_"hd: \"%s\".%s[%d] - %s - rel -- h=%08x (result=%d) %s "_ANSI_0_"\r\n"
#define msg_r2	_ANSI_R_"hd: \"%s\".%s[%d] - %s - rel -- h=%08x (result=%d) %s "_ANSI_0_"\r\n"

int _isf_msg_color(UINT32 probe)
{
	if(probe &
		(ISF_IN_PROBE_PUSH_ERR|ISF_IN_PROBE_EXT_ERR|ISF_IN_PROBE_PROC_ERR
		|ISF_OUT_PROBE_NEW_ERR|ISF_OUT_PROBE_PROC_ERR|ISF_OUT_PROBE_EXT_ERR|ISF_OUT_PROBE_PUSH_ERR
		|ISF_USER_PROBE_PULL_ERR))
	{
		return 2;
	}

	if(probe &
		(ISF_IN_PROBE_PUSH_WRN|ISF_IN_PROBE_EXT_WRN|ISF_IN_PROBE_PROC_WRN
		|ISF_OUT_PROBE_NEW_WRN|ISF_OUT_PROBE_PROC_WRN|ISF_OUT_PROBE_EXT_WRN|ISF_OUT_PROBE_PUSH_WRN
		|ISF_USER_PROBE_PULL_WRN))
	{
		return 1;
	}

	return 0;
}


const CHAR* _isf_in_probe(UINT32 probe)
{
	const CHAR* str = "(?)";
	switch(probe) {
	case ISF_IN_PROBE_PUSH:
	case ISF_IN_PROBE_PUSH_DROP:
	case ISF_IN_PROBE_PUSH_WRN:
	case ISF_IN_PROBE_PUSH_ERR:
		return "PUSH";
		break;
	case ISF_IN_PROBE_EXT:
	case ISF_IN_PROBE_EXT_DROP:
	case ISF_IN_PROBE_EXT_WRN:
	case ISF_IN_PROBE_EXT_ERR:
		return "EXT";
		break;
	case ISF_IN_PROBE_PROC:
	case ISF_IN_PROBE_PROC_DROP:
	case ISF_IN_PROBE_PROC_WRN:
	case ISF_IN_PROBE_PROC_ERR:
		return "PROC";
		break;
	case ISF_IN_PROBE_REL:
		return "REL";
		break;
	}

	return str;
}

const CHAR* _isf_out_probe(UINT32 probe)
{
	const CHAR* str = "(?)";
	switch(probe) {
	case ISF_OUT_PROBE_PUSH:
	case ISF_OUT_PROBE_PUSH_DROP:
	case ISF_OUT_PROBE_PUSH_WRN:
	case ISF_OUT_PROBE_PUSH_ERR:
		return "PUSH";
		break;
	case ISF_OUT_PROBE_EXT:
	case ISF_OUT_PROBE_EXT_DROP:
	case ISF_OUT_PROBE_EXT_WRN:
	case ISF_OUT_PROBE_EXT_ERR:
		return "EXT";
		break;
	case ISF_OUT_PROBE_PROC:
	case ISF_OUT_PROBE_PROC_DROP:
	case ISF_OUT_PROBE_PROC_WRN:
	case ISF_OUT_PROBE_PROC_ERR:
		return "PROC";
		break;
	case ISF_OUT_PROBE_NEW:
	case ISF_OUT_PROBE_NEW_DROP:
	case ISF_OUT_PROBE_NEW_WRN:
	case ISF_OUT_PROBE_NEW_ERR:
		return "NEW";
		break;
	case ISF_USER_PROBE_PULL:
	case ISF_USER_PROBE_PULL_SKIP:
	case ISF_USER_PROBE_PULL_WRN:
	case ISF_USER_PROBE_PULL_ERR:
		return "PULL";
	}

	return str;
}

const CHAR* _isf_user_probe(UINT32 probe)
{
	const CHAR* str = "(?)";
	switch(probe) {
	case ISF_USER_PROBE_PULL:
	case ISF_USER_PROBE_PULL_SKIP:
	case ISF_USER_PROBE_PULL_WRN:
	case ISF_USER_PROBE_PULL_ERR:
	case ISF_USER_PROBE_REL:
		return "USER";
		break;
	}

	return str;
}

typedef struct _RET_STR {
	ISF_RV r;
	const CHAR* str;
} RET_STR;

static const RET_STR _isf_ret_str[] =
{
	{ISF_OK, "OK"},
	{ISF_ENTER, "ENTER"},
	{ISF_ERR_FATAL, "FATAL"},
	//flow state fail
	{ISF_ERR_INACTIVE_STATE, "INACTIVE_STATE"},
	{ISF_ERR_NOT_OPEN_YET, "NOT_OPEN"},
	{ISF_ERR_NOT_START, "NOT_START"},
	{ISF_ERR_START_FAIL, "START_FAIL"},
	{ISF_ERR_STOP_FAIL, "STOP_FAIL"},
	{ISF_ERR_PARAM_EXCEED_LIMIT, "PARAM_EXCEED_LIMIT"},
	{ISF_ERR_PARAM_NOT_SUPPORT, "PARAM_NOT_SUPPORT"},
	//for flow control / data rate control drop
	{ISF_ERR_QUEUE_DROP, "QUEUE_DROP"},
	{ISF_ERR_FRC_DROP, "FRC_DROP"},
	//flow data check fail
	{ISF_ERR_INVALID_DATA, "INVALID_DATA"},
	{ISF_ERR_INCOMPLETE_DATA, "INCOMPLETE_DATA"},
	{ISF_ERR_NEW_FAIL, "NEW_FAIL"},
	{ISF_ERR_ADD_FAIL, "ADD_FAIL"},
	{ISF_ERR_RELEASE_FAIL, "RELEASE_FAIL"},
	//flow queue fail
	{ISF_ERR_WAIT_TIMEOUT, "WAIT_TIMEOUT"},
	{ISF_ERR_QUEUE_EMPTY, "QUEUE_EMPTY"},
	{ISF_ERR_QUEUE_FULL, "QUEUE_FULL"},
	{ISF_ERR_QUEUE_ERROR, "QUEUE_ERROR"},
	//flow proc fail
	{ISF_ERR_PROCESS_FAIL, "PROCESS_FAIL"},
	{ISF_ERR_DATA_EXCEED_LIMIT, "DATA_EXCEED_LIMIT"},
	{ISF_ERR_DATA_NOT_SUPPORT, "DATA_NOT_SUPPORT"},
	//flow push fail
	{ISF_ERR_NOT_CONNECT_YET, "NOT_CONNECT_YET"},
	{0xffff, 0}
};

const CHAR* _isf_err(INT32 r)
{
	const CHAR* str = 0;
	const RET_STR* ret_str = _isf_ret_str;

	while(ret_str->r != 0xffff) {
		if(r == ret_str->r) {
			str = ret_str->str;
			break;
		}
		ret_str ++;
	}
	if(!str)return "(?)";

	return str;
}

#define ISF_PERF	1

#define ONE_SECOND 1000000
#define TWO_SECOND 2000000

#if 1

ISF_RV _isf_probe_is_ready(ISF_UNIT *p_thisunit, UINT32 nport)
{
	ISF_RV r = ISF_OK;
	register UINT32 k;
	if(!p_thisunit) {
		return ISF_OK;
	}

	if (ISF_IS_IPORT(p_thisunit, nport)) {
		//IN: PUSH-EXT-PROC-REL
		ISF_IN_STATUS *p_status = &(p_thisunit->port_ininfo[nport - ISF_IN_BASE]->status.in);
			p_status->ts_push[ISF_TS_THIS] = hwclock_get_longcounter();
			p_status->ts_push[ISF_TS_DIFF] = p_status->ts_push[ISF_TS_THIS] - p_status->ts_push[ISF_TS_LAST];
			if (p_status->ts_push[ISF_TS_DIFF] >= ONE_SECOND) {
				//record ts diff
				p_status->ts_push[ISF_TS_SECOND] = p_status->ts_push[ISF_TS_DIFF];
				//record cnt
				for(k=0; k<16; k++) {
					p_status->total_cnt[k] = p_status->this_cnt[k];
				}
				//reset all count
				for(k=0; k<16; k++) {
					p_status->this_cnt[k] = 0;
				}
				//reset this ts
				p_status->ts_push[ISF_TS_LAST] = p_status->ts_push[ISF_TS_THIS];
				//DBG_DUMP(_ANSI_Y_"hd: \"%s\".in[%d] - work status expired then 1 sec, please try again"_ANSI_0_"\r\n",
				//    p_thisunit->unit_name, nport - ISF_IN_BASE);
				r = ISF_ERR_FAILED;
			}
    	} else if (ISF_IS_OPORT(p_thisunit, nport)) {
    		BOOL b_reset = FALSE;
    		{
		//OUT: NEW-PROC-OUT-PUSH
		//OUT: NEW-PROC-OUT-QUEUE-PULL
		ISF_OUT_STATUS *p_status = &(p_thisunit->port_outinfo[nport - ISF_OUT_BASE]->status.out);
			p_status->ts_new[ISF_TS_THIS] = hwclock_get_longcounter();
			p_status->ts_new[ISF_TS_DIFF] = p_status->ts_new[ISF_TS_THIS] - p_status->ts_new[ISF_TS_LAST];
			if (p_status->ts_new[ISF_TS_DIFF] >= ONE_SECOND) {

				b_reset = TRUE;

				//record ts diff
				p_status->ts_new[ISF_TS_SECOND] = p_status->ts_new[ISF_TS_DIFF];
				//record cnt
				for(k=0; k<16; k++) {
					p_status->total_cnt[k] = p_status->this_cnt[k];
				}
				//reset all count
				for(k=0; k<16; k++) {
					p_status->this_cnt[k] = 0;
				}
				//reset this ts
				p_status->ts_new[ISF_TS_LAST] = p_status->ts_new[ISF_TS_THIS];
				//DBG_DUMP(_ANSI_Y_"hd: \"%s\".out[%d] - work status expired then 1 sec, please try again"_ANSI_0_"\r\n",
				//    p_thisunit->unit_name, nport - ISF_OUT_BASE);
				r = ISF_ERR_FAILED;
			}
		}
		{
		//USER: PULL--------REL
		ISF_USER_STATUS *p_status = &(p_thisunit->port_outinfo[nport - ISF_OUT_BASE]->status.out);
			p_status->ts_pull[ISF_TS_THIS] = hwclock_get_longcounter();
			p_status->ts_pull[ISF_TS_DIFF] = p_status->ts_pull[ISF_TS_THIS] - p_status->ts_pull[ISF_TS_LAST];
			//if (p_status->ts_pull[ISF_TS_DIFF] >= ONE_SECOND) {
			if (b_reset) { // don't care about user pull, just reset with out status
				//record ts diff
				p_status->ts_pull[ISF_TS_SECOND] = p_status->ts_pull[ISF_TS_DIFF];
				//record cnt
				for(k=0; k<8; k++) {
					p_status->total_cnt[16+k] = p_status->this_cnt[16+k];
				}
				//reset all count
				for(k=0; k<8; k++) {
					p_status->this_cnt[16+k] = 0;
				}
				//reset this ts
				p_status->ts_pull[ISF_TS_LAST] = p_status->ts_pull[ISF_TS_THIS];
				//DBG_DUMP(_ANSI_Y_"hd: \"%s\".out[%d] - work status expired then 1 sec, please try again"_ANSI_0_"\r\n",
				//    p_thisunit->unit_name, nport - ISF_OUT_BASE);
				r = ISF_ERR_FAILED;
			}
		}
	}
	return r;
}
EXPORT_SYMBOL(_isf_probe_is_ready);
#endif

void _isf_probe_debug(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 probe, ISF_RV result)
{
	if (ISF_IS_IPORT(p_thisunit, nport)) {
		switch(probe) {
		case ISF_IN_PROBE_PUSH_DROP:
		case ISF_IN_PROBE_EXT_DROP:
		case ISF_IN_PROBE_PROC_DROP:
			nvtmpp_vb_add_err_cnt(p_thisunit->unit_module, NVTMPP_MODULE_ER_DROP);
			#if 0
			isf_debug_log_print(msg_p0, p_thisunit->unit_name, portstr[1], nport-ISF_IN_BASE,
					_isf_in_probe(probe),
					 result,
					 _isf_err(result));
			#endif
			break;
		case ISF_IN_PROBE_PUSH_WRN:
		case ISF_IN_PROBE_EXT_WRN:
		case ISF_IN_PROBE_PROC_WRN:
			nvtmpp_vb_add_err_cnt(p_thisunit->unit_module, NVTMPP_MODULE_ER_WRN);
			isf_debug_log_print(msg_p1, p_thisunit->unit_name, portstr[1], nport-ISF_IN_BASE,
					_isf_in_probe(probe),
					 result,
					 _isf_err(result));
			break;

		case ISF_IN_PROBE_PUSH_ERR:
		case ISF_IN_PROBE_EXT_ERR:
		case ISF_IN_PROBE_PROC_ERR:
			nvtmpp_vb_add_err_cnt(p_thisunit->unit_module, NVTMPP_MODULE_ER_MISC);
			isf_debug_log_print(msg_p2, p_thisunit->unit_name, portstr[1], nport-ISF_IN_BASE,
					_isf_in_probe(probe),
					 result,
					 _isf_err(result));
			break;
		default:
			break;
		}
	} else {
		switch(probe) {
		case ISF_OUT_PROBE_NEW_DROP:
		case ISF_OUT_PROBE_PUSH_DROP:
		case ISF_OUT_PROBE_EXT_DROP:
		case ISF_OUT_PROBE_PROC_DROP:
			nvtmpp_vb_add_err_cnt(p_thisunit->unit_module, NVTMPP_MODULE_ER_DROP);
			#if 0
			isf_debug_log_print(msg_p0, p_thisunit->unit_name, portstr[0], nport-ISF_OUT_BASE,
					_isf_out_probe(probe),
					 result,
					 _isf_err(result));
			#endif
			break;
		case ISF_OUT_PROBE_NEW_WRN:
		case ISF_OUT_PROBE_PUSH_WRN:
		case ISF_OUT_PROBE_EXT_WRN:
		case ISF_OUT_PROBE_PROC_WRN:
			nvtmpp_vb_add_err_cnt(p_thisunit->unit_module, NVTMPP_MODULE_ER_WRN);
			isf_debug_log_print(msg_p1, p_thisunit->unit_name, portstr[0], nport-ISF_OUT_BASE,
					_isf_out_probe(probe),
					 result,
					 _isf_err(result));
			break;
		case ISF_OUT_PROBE_NEW_ERR:
		case ISF_OUT_PROBE_PUSH_ERR:
		case ISF_OUT_PROBE_EXT_ERR:
		case ISF_OUT_PROBE_PROC_ERR:
			nvtmpp_vb_add_err_cnt(p_thisunit->unit_module, NVTMPP_MODULE_ER_MISC);
			isf_debug_log_print(msg_p2, p_thisunit->unit_name, portstr[0], nport-ISF_OUT_BASE,
					_isf_out_probe(probe),
					 result,
					 _isf_err(result));
			break;
		default:
			break;
		}
	}
}

void _isf_probe(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 probe, ISF_RV result)
{
	if(!p_thisunit) {
		return;
	}

	_isf_probe_debug(p_thisunit, nport, probe, result);

	if (ISF_IS_IPORT(p_thisunit, nport)) {
		//IN: PUSH-EXT-PROC-REL
		ISF_IN_STATUS *p_status = &(p_thisunit->port_ininfo[nport - ISF_IN_BASE]->status.in);
		register UINT32 k, g = 0xff;
		for(k=0; k<16; k++) {
			if(probe == (UINT32)(1<<k)) {
				g = k;
			}
		}
		if(g == 0xff)
			return;
		//reset
		//p_status->ts_push[ISF_TS_LAST] = p_status->ts_push[ISF_TS_THIS] = 0;
		//p_status->cnt[g] = 0;
		if(probe == ISF_IN_PROBE_PUSH) {
			if(result != ISF_ENTER)
				return;
			p_status->this_cnt[g] ++;
			p_status->ts_push[ISF_TS_THIS] = hwclock_get_longcounter();
			p_status->ts_push[ISF_TS_DIFF] = p_status->ts_push[ISF_TS_THIS] - p_status->ts_push[ISF_TS_LAST];
			if (p_status->ts_push[ISF_TS_DIFF] >= TWO_SECOND) {
				//reset all count
				for(k=0; k<16; k++) {
					p_status->this_cnt[k] = 0;
				}
				//reset this ts
				p_status->ts_push[ISF_TS_LAST] = p_status->ts_push[ISF_TS_THIS];
			} else if (p_status->ts_push[ISF_TS_DIFF] >= ONE_SECOND) {
				//record ts diff
				p_status->ts_push[ISF_TS_SECOND] = p_status->ts_push[ISF_TS_DIFF];
				//record cnt
				for(k=0; k<16; k++) {
					p_status->total_cnt[k] = p_status->this_cnt[k];
				}
				//reset all count
				for(k=0; k<16; k++) {
					p_status->this_cnt[k] = 0;
				}
				//reset this ts
				p_status->ts_push[ISF_TS_LAST] = p_status->ts_push[ISF_TS_THIS];
			}
		} else if(probe == ISF_IN_PROBE_EXT) {
			if(result != ISF_ENTER)
				return;
			p_status->this_cnt[g] ++;
		} else if(probe == ISF_IN_PROBE_PROC) {
			if(result != ISF_ENTER)
				return;
			p_status->this_cnt[g] ++;
		} else if(probe == ISF_IN_PROBE_REL) {
			if(result != ISF_ENTER)
				return;
			p_status->this_cnt[g] ++;
		} else {
			p_status->this_cnt[g] ++;
		}
    	} else if (ISF_IS_OPORT(p_thisunit, nport) && (probe & 0x0000ffff)) {
		//OUT: NEW-PROC-OUT-PUSH
		//OUT: NEW-PROC-OUT-QUEUE-PULL
		ISF_OUT_STATUS *p_status = &(p_thisunit->port_outinfo[nport - ISF_OUT_BASE]->status.out);
		register UINT32 k, g = 0xff;
		for(k=0; k<16; k++) {
			if(probe == (UINT32)(1<<k)) {
				g = k;
			}
		}
		if(g == 0xff)
			return;
		//reset
		//p_status->ts_push[ISF_TS_LAST] = p_status->ts_push[ISF_TS_THIS] = 0;
		//p_status->cnt[g] = 0;
		if(probe == ISF_OUT_PROBE_NEW) {
			if(result != ISF_ENTER)
				return;
			p_status->this_cnt[g] ++;
			p_status->ts_new[ISF_TS_THIS] = hwclock_get_longcounter();
			p_status->ts_new[ISF_TS_DIFF] = p_status->ts_new[ISF_TS_THIS] - p_status->ts_new[ISF_TS_LAST];
			if (p_status->ts_new[ISF_TS_DIFF] >= TWO_SECOND) {
				//reset all count
				for(k=0; k<16; k++) {
					p_status->this_cnt[k] = 0;
				}
				//reset this ts
				p_status->ts_new[ISF_TS_LAST] = p_status->ts_new[ISF_TS_THIS];
			} else if (p_status->ts_new[ISF_TS_DIFF] >= ONE_SECOND) {
				//record ts diff
				p_status->ts_new[ISF_TS_SECOND] = p_status->ts_new[ISF_TS_DIFF];
				//record cnt
				for(k=0; k<16; k++) {
					p_status->total_cnt[k] = p_status->this_cnt[k];
				}
				//reset all count
				for(k=0; k<16; k++) {
					p_status->this_cnt[k] = 0;
				}
				//reset this ts
				p_status->ts_new[ISF_TS_LAST] = p_status->ts_new[ISF_TS_THIS];
			}
		} else if(probe == ISF_OUT_PROBE_PROC) {
			if(result != ISF_ENTER)
				return;
			p_status->this_cnt[g] ++;
		} else if(probe == ISF_OUT_PROBE_EXT) {
			if(result != ISF_ENTER)
				return;
			p_status->this_cnt[g] ++;
		} else if(probe == ISF_OUT_PROBE_PUSH) {
			if(result != ISF_ENTER)
				return;
			p_status->this_cnt[g] ++;
		} else {
			p_status->this_cnt[g] ++;
		}
    	} else if (ISF_IS_OPORT(p_thisunit, nport) && (probe & 0xffff0000)) {
		//USER: PULL--------REL
		ISF_USER_STATUS *p_status = &(p_thisunit->port_outinfo[nport - ISF_OUT_BASE]->status.out);
		register UINT32 k, g = 0xff;
		for(k=0; k<8; k++) {
			if((UINT32)(probe >> 16) == (UINT32)(1<<k)) {
				g = 16+k;
			}
		}
		if(g == 0xff)
			return;
		//reset
		//p_status->ts_push[ISF_TS_LAST] = p_status->ts_push[ISF_TS_THIS] = 0;
		//p_status->cnt[g] = 0;
		if(probe == ISF_USER_PROBE_PULL) {
			if(result != ISF_ENTER)
				return;
			p_status->this_cnt[g] ++;
			p_status->ts_pull[ISF_TS_THIS] = hwclock_get_longcounter();
			p_status->ts_pull[ISF_TS_DIFF] = p_status->ts_pull[ISF_TS_THIS] - p_status->ts_pull[ISF_TS_LAST];
			if (p_status->ts_pull[ISF_TS_DIFF] >= TWO_SECOND) {
				//reset all count
				for(k=0; k<8; k++) {
					p_status->this_cnt[16+k] = 0;
				}
				//reset this ts
				p_status->ts_pull[ISF_TS_LAST] = p_status->ts_pull[ISF_TS_THIS];
			} else if (p_status->ts_pull[ISF_TS_DIFF] >= ONE_SECOND) {
				//record ts diff
				p_status->ts_pull[ISF_TS_SECOND] = p_status->ts_pull[ISF_TS_DIFF];
				//record cnt
				for(k=0; k<8; k++) {
					p_status->total_cnt[16+k] = p_status->this_cnt[16+k];
				}
				//reset all count
				for(k=0; k<8; k++) {
					p_status->this_cnt[16+k] = 0;
				}
				//reset this ts
				p_status->ts_pull[ISF_TS_LAST] = p_status->ts_pull[ISF_TS_THIS];
			}
		} else if(probe == ISF_USER_PROBE_REL) {
			if(result != ISF_ENTER)
				return;
			p_status->this_cnt[g] ++;
		} else {
			p_status->this_cnt[g] ++;
		}
    	}

	{
		INT32 pd = 0, pi = 0;

		if (((_isf_probe_dir == 0)||(_isf_probe_dir == 0xff)) && ISF_IS_OPORT(p_thisunit, nport)) { //output port
			pd = 0;
			pi = nport - ISF_OUT_BASE;
		} else if (((_isf_probe_dir == 1)||(_isf_probe_dir == 0xff)) && ISF_IS_IPORT(p_thisunit, nport)) { //input port
			pd = 1;
			pi = nport - ISF_IN_BASE;
		} else {
			return;
		}

    /// probe
        if ( ((_isf_probe_port == 0xff) && (_isf_probe_unit[0] == '*'))
         || ((_isf_probe_port == pi) && (strncmp(p_thisunit->unit_name, _isf_probe_unit, 32) == 0)) ) {
			if (_isf_probe_mask & probe) {
				int c = _isf_msg_color(probe);
				if(c == 0) {
					DBG_DUMP(msg_p0,
						p_thisunit->unit_name, portstr[pd], pi,
						pd?_isf_in_probe(probe):_isf_out_probe(probe),
						result,
						_isf_err(result)
						);
				} else if(c == 1) {
					DBG_DUMP(msg_p1,
						p_thisunit->unit_name, portstr[pd], pi,
						pd?_isf_in_probe(probe):_isf_out_probe(probe),
						result,
						_isf_err(result)
						);
				} else if(c == 2) {
					DBG_DUMP(msg_p2,
						p_thisunit->unit_name, portstr[pd], pi,
						pd?_isf_in_probe(probe):_isf_out_probe(probe),
						result,
						_isf_err(result)
						);
				}
			}
        }
	}
}

void _isf_unit_debug_new(ISF_UNIT *p_thisunit, UINT32 nport, ISF_DATA *p_data, UINT32 addr, UINT32 size, UINT32 probe)
{
	INT32 pd, pi = 0;

    if (!p_thisunit)
    		return;

	if (((_isf_probe_dir == 0)||(_isf_probe_dir == 0xff)) && ISF_IS_OPORT(p_thisunit, nport)) { //output port
		pd = 0;
		pi = nport - ISF_OUT_BASE;
	} else if (((_isf_probe_dir == 1)||(_isf_probe_dir == 0xff)) && ISF_IS_IPORT(p_thisunit, nport)) { //input port
		pd = 1;
		pi = nport - ISF_IN_BASE;
	} else {
		return;
	}

    /// probe
        if ( ((_isf_probe_port == 0xff) && (_isf_probe_unit[0] == '*'))
		|| ((_isf_probe_port == pi) && (strncmp(p_thisunit->unit_name, _isf_probe_unit, 32) == 0)) ) {
			if (_isf_probe_mask & probe) {
				int c = _isf_msg_color(probe);
				if(c == 0) {
					DBG_DUMP(msg_n0,
						p_thisunit->unit_name, portstr[pd], pi,
						pd?_isf_in_probe(probe):_isf_out_probe(probe),
						p_data?p_data->h_data:0, size, addr,
						_isf_err(addr?ISF_OK:ISF_ERR_NEW_FAIL)
						);
				} else if(c == 1) {
					DBG_DUMP(msg_n1,
						p_thisunit->unit_name, portstr[pd], pi,
						pd?_isf_in_probe(probe):_isf_out_probe(probe),
						p_data?p_data->h_data:0, size, addr,
						_isf_err(addr?ISF_OK:ISF_ERR_NEW_FAIL)
						);
				} else if(c == 2) {
					DBG_DUMP(msg_n2,
						p_thisunit->unit_name, portstr[pd], pi,
						pd?_isf_in_probe(probe):_isf_out_probe(probe),
						p_data?p_data->h_data:0, size, addr,
						_isf_err(addr?ISF_OK:ISF_ERR_NEW_FAIL)
						);
				}
			}
		}
}

void _isf_unit_debug_add(ISF_UNIT *p_thisunit, UINT32 nport, ISF_DATA *p_data, UINT32 probe, ISF_RV result)
{
	INT32 pd, pi = 0;

    if (!p_thisunit)
    		return;

	if (((_isf_probe_dir == 0)||(_isf_probe_dir == 0xff)) && ISF_IS_OPORT(p_thisunit, nport)) { //output port
		pd = 0;
		pi = nport - ISF_OUT_BASE;
	} else if (((_isf_probe_dir == 1)||(_isf_probe_dir == 0xff)) && ISF_IS_IPORT(p_thisunit, nport)) { //input port
		pd = 1;
		pi = nport - ISF_IN_BASE;
	} else {
		return;
	}

    /// probe
        if ( ((_isf_probe_port == 0xff) && (_isf_probe_unit[0] == '*'))
		|| ((_isf_probe_port == pi) && (strncmp(p_thisunit->unit_name, _isf_probe_unit, 32) == 0)) ) {
			if (_isf_probe_mask & probe) {
				int c = _isf_msg_color(probe);
				if(c == 0) {
					DBG_DUMP(msg_a0,
						p_thisunit->unit_name, portstr[pd], pi,
						pd?_isf_in_probe(probe):_isf_out_probe(probe),
						p_data?p_data->h_data:0, result,
						_isf_err(result)
						);
				} else if(c == 1) {
					DBG_DUMP(msg_a1,
						p_thisunit->unit_name, portstr[pd], pi,
						pd?_isf_in_probe(probe):_isf_out_probe(probe),
						p_data?p_data->h_data:0, result,
						_isf_err(result)
						);
				} else if(c == 2) {
					DBG_DUMP(msg_a2,
						p_thisunit->unit_name, portstr[pd], pi,
						pd?_isf_in_probe(probe):_isf_out_probe(probe),
						p_data?p_data->h_data:0, result,
						_isf_err(result)
						);
				}
			}
		}
}

void _isf_unit_debug_release(ISF_UNIT *p_thisunit, UINT32 nport, ISF_DATA *p_data, UINT32 probe, ISF_RV result)
{
	INT32 pd, pi = 0;

    if (!p_thisunit)
    		return;

	if (((_isf_probe_dir == 0)||(_isf_probe_dir == 0xff)) && ISF_IS_OPORT(p_thisunit, nport)) { //output port
		pd = 0;
		pi = nport - ISF_OUT_BASE;
	} else if (((_isf_probe_dir == 1)||(_isf_probe_dir == 0xff)) && ISF_IS_IPORT(p_thisunit, nport)) { //input port
		pd = 1;
		pi = nport - ISF_IN_BASE;
	} else {
		return;
	}

    /// probe
        if ( ((_isf_probe_port == 0xff) && (_isf_probe_unit[0] == '*'))
         || ((_isf_probe_port == pi) && (strncmp(p_thisunit->unit_name, _isf_probe_unit, 32) == 0)) ) {
			if (_isf_probe_mask & probe) {
				int c = _isf_msg_color(probe);
				if(c == 0) {
					DBG_DUMP(msg_r0,
						p_thisunit->unit_name, portstr[pd], pi,
						pd?_isf_in_probe(probe):_isf_out_probe(probe),
						p_data?p_data->h_data:0, result,
						_isf_err(result)
						);
				} else if(c == 1) {
					DBG_DUMP(msg_r1,
						p_thisunit->unit_name, portstr[pd], pi,
						pd?_isf_in_probe(probe):_isf_out_probe(probe),
						p_data?p_data->h_data:0, result,
						_isf_err(result)
						);
				} else if(c == 2) {
					DBG_DUMP(msg_r2,
						p_thisunit->unit_name, portstr[pd], pi,
						pd?_isf_in_probe(probe):_isf_out_probe(probe),
						p_data?p_data->h_data:0, result,
						_isf_err(result)
						);
				}
			}
        }
}

void _isf_unit_debug_postpush(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data)
{
	INT32 pd = 0, pi = 0;
	ISF_DATA_CLASS* p_thisclass;


    if (!p_thisunit || !p_data)
    		return;

	p_thisclass = (ISF_DATA_CLASS*)(p_data->p_class);
	if (((_isf_probe_dir == 0)||(_isf_probe_dir == 0xff)) && ISF_IS_OPORT(p_thisunit, oport)) { //output port
		pd = 0;
		pi = oport - ISF_OUT_BASE;
	} else {
		goto it_perf;
	}

    /// probe
        if ( ((_isf_probe_port == 0xff) && (_isf_probe_unit[0] == '*'))
         || ((_isf_probe_port == pi) && (strncmp(p_thisunit->unit_name, _isf_probe_unit, 32) == 0)) ) {
			if (_isf_probe_mask & ISF_IN_PROBE_PUSH) {
				BOOL show = 0;
				if ((p_data->desc[0] != 0) && (p_thisclass != 0)) {
					if(p_thisclass->do_probe) {
						show = 1;
						p_thisclass->do_probe(p_data, p_thisunit, pd, pi, "PUSH");
					}
				}
				if(!show) {
		        		DBG_DUMP(_ANSI_W_"\"%s\".%s[%d] %s - data -- h=%08x (unknown? %08x)"_ANSI_0_"\r\n",
		        		    p_thisunit->unit_name, portstr[pd], pi,
		        		    _isf_in_probe(ISF_IN_PROBE_PUSH),
		        		    p_data->h_data,
		        		    p_data->desc[0]);
				}
			}
		}

it_perf:
	if ((_isf_perf_dir == 0) && ISF_IS_OPORT(p_thisunit, oport)) { //output port
		pd = 0;
		pi = oport - ISF_OUT_BASE;
	} else {
		goto it_save;
	}

#if ISF_PERF
    /// perf
        if ((_isf_perf_port == pi) && (strncmp(p_thisunit->unit_name, _isf_perf_unit, 32) == 0)) {
            if ((p_data->desc[0] != 0) && (p_thisclass != 0)) {
            		if(p_thisclass->do_perf) {
	            		p_thisclass->do_perf(p_data, p_thisunit, pd, pi);
            		} else {
	        			DBG_DUMP(_ANSI_M_"\"%s\".%s[%d] perf -- ignore!"_ANSI_0_"\r\n",
	        			    p_thisunit->unit_name, portstr[pd], pi);
            		}
            } else {
        			DBG_DUMP(_ANSI_M_"\"%s\".%s[%d] perf -- ignore!"_ANSI_0_"\r\n",
        			    p_thisunit->unit_name, portstr[pd], pi);
            }
        }
#endif

it_save:
	if ((_isf_save_dir == 0) && ISF_IS_OPORT(p_thisunit, oport)) { //output port
		pd = 0;
		pi = oport - ISF_OUT_BASE;
	} else {
		return;
	}

#define MAX_FILENAME_LEN	80

#if defined (__UITRON)
	#define DUMP_DISK "A:\\"
#else
	#define DUMP_DISK "/mnt/sd/"
#endif
    /// save
        if (_isf_save_count > 0)
        if (((_isf_save_port == 999) && (strncmp(p_thisunit->unit_name, _isf_save_unit, _isf_save_unit_len) == 0))
        || ((_isf_save_port == pi) && (strncmp(p_thisunit->unit_name, _isf_save_unit, _isf_save_unit_len) == 0)) ) {
			if ((p_data->desc[0] != 0) && (p_thisclass != 0)) {
				if(p_thisclass->do_save) {
					p_thisclass->do_save(p_data, p_thisunit, pd, pi);
				} else {
	        		DBG_DUMP(_ANSI_C_"\"%s\".%s[%d] save -- h=%08x (unknown? %08x) -- ignore"_ANSI_0_"\r\n",
	        		    p_thisunit->unit_name, portstr[pd], pi,
	        		    p_data->h_data,
	        		    p_data->desc[0]);
	        		DBG_DUMP(_ANSI_C_"\"%s\".%s[%d] save -- ignore!"_ANSI_0_"\r\n",
	        		    p_thisunit->unit_name, portstr[pd], pi);
        		    }
			} else {
	        		DBG_DUMP(_ANSI_C_"\"%s\".%s[%d] save -- h=%08x (unknown? %08x) -- ignore"_ANSI_0_"\r\n",
	        		    p_thisunit->unit_name, portstr[pd], pi,
	        		    p_data->h_data,
	        		    p_data->desc[0]);
	        		DBG_DUMP(_ANSI_C_"\"%s\".%s[%d] save -- ignore!"_ANSI_0_"\r\n",
	        		    p_thisunit->unit_name, portstr[pd], pi);
			}

			_isf_save_count--;
			if (_isf_save_count == 0) {
				DBG_DUMP("save port end\r\n");
				_isf_save_unit[0] = 0;
				_isf_save_port = -1; //auto off
			}
        }
}

void _isf_unit_debug_prepush(ISF_UNIT *p_thisunit, UINT32 iport, ISF_DATA *p_data)
{
	INT32 pd = 0, pi = 0;
	ISF_DATA_CLASS* p_thisclass;


    if (!p_thisunit || !p_data)
    		return;

	p_thisclass = (ISF_DATA_CLASS*)(p_data->p_class);
	if (((_isf_probe_dir == 1)||(_isf_probe_dir == 0xff)) && ISF_IS_IPORT(p_thisunit, iport)) { //input port
		pd = 1;
		pi = iport - ISF_IN_BASE;
	} else {
		goto it_perf;
	}

    /// probe
        if ( ((_isf_probe_port == 0xff) && (_isf_probe_unit[0] == '*'))
         || ((_isf_probe_port == pi) && (strncmp(p_thisunit->unit_name, _isf_probe_unit, 32) == 0)) ) {
			if (_isf_probe_mask & ISF_OUT_PROBE_PUSH) {
				BOOL show = 0;
				if ((p_data->desc[0] != 0) && (p_thisclass != 0)) {
					if(p_thisclass->do_probe) {
						show = 1;
						p_thisclass->do_probe(p_data, p_thisunit, pd, pi, "PUSH");
					}
				}
				if(!show) {
		        		DBG_DUMP(_ANSI_W_"hd: \"%s\".%s[%d] %s - data -- h=%08x (unknown? %08x)"_ANSI_0_"\r\n",
		        		    p_thisunit->unit_name, portstr[pd], pi,
		        		    _isf_out_probe(ISF_OUT_PROBE_PUSH),
		        		    p_data->h_data,
		        		    p_data->desc[0]);
				}
			}
		}

it_perf:
	if ((_isf_perf_dir == 1) && ISF_IS_IPORT(p_thisunit, iport)) { //input port
		pd = 1;
		pi = iport - ISF_IN_BASE;
	} else {
		goto it_save;
	}

#if ISF_PERF
    /// perf
        if ((_isf_perf_port == pi) && (strncmp(p_thisunit->unit_name, _isf_perf_unit, 32) == 0)) {
            if ((p_data->desc[0] != 0) && (p_thisclass != 0)) {
            		if(p_thisclass->do_perf) {
	            		p_thisclass->do_perf(p_data, p_thisunit, pd, pi);
            		} else {
	        			DBG_DUMP(_ANSI_M_"hd: \"%s\".%s[%d] perf -- ignore!"_ANSI_0_"\r\n",
	        			    p_thisunit->unit_name, portstr[pd], pi);
            		}
            } else {
        			DBG_DUMP(_ANSI_M_"hd: \"%s\".%s[%d] perf -- ignore!"_ANSI_0_"\r\n",
        			    p_thisunit->unit_name, portstr[pd], pi);
            }
        }
#endif

it_save:
	if ((_isf_save_dir == 1) && ISF_IS_IPORT(p_thisunit, iport)) { //input port
		pd = 1;
		pi = iport - ISF_IN_BASE;
	} else {
		return;
	}

#define MAX_FILENAME_LEN	80

#if defined (__UITRON)
	#define DUMP_DISK "A:\\"
#else
	#define DUMP_DISK "/mnt/sd/"
#endif
    /// save
        if (_isf_save_count > 0)
        if (((_isf_save_port == 999) && (strncmp(p_thisunit->unit_name, _isf_save_unit, _isf_save_unit_len) == 0))
        || ((_isf_save_port == pi) && (strncmp(p_thisunit->unit_name, _isf_save_unit, _isf_save_unit_len) == 0)) ) {
			if ((p_data->desc[0] != 0) && (p_thisclass != 0)) {
				if(p_thisclass->do_save) {
					p_thisclass->do_save(p_data, p_thisunit, pd, pi);
				} else {
	        		DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] save -- h=%08x (unknown? %08x) -- ignore"_ANSI_0_"\r\n",
	        		    p_thisunit->unit_name, portstr[pd], pi,
	        		    p_data->h_data,
	        		    p_data->desc[0]);
	        		DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] save -- ignore!"_ANSI_0_"\r\n",
	        		    p_thisunit->unit_name, portstr[pd], pi);
        		    }
			} else {
	        		DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] save -- h=%08x (unknown? %08x) -- ignore"_ANSI_0_"\r\n",
	        		    p_thisunit->unit_name, portstr[pd], pi,
	        		    p_data->h_data,
	        		    p_data->desc[0]);
	        		DBG_DUMP(_ANSI_C_"hd: \"%s\".%s[%d] save -- ignore!"_ANSI_0_"\r\n",
	        		    p_thisunit->unit_name, portstr[pd], pi);
			}

			_isf_save_count--;
			if (_isf_save_count == 0) {
				DBG_DUMP("save port end\r\n");
				_isf_save_unit[0] = 0;
				_isf_save_port = -1; //auto off
			}
        }
}

void _isf_flow_probe_port(char* unit_name, char* port_name, char* mask_name)
{
	SEM_WAIT(ISF_SEM_ID);
	if ((unit_name == 0) || (port_name == 0) || (port_name[0] != '*' && port_name[0]!='o' && port_name[0] != 'i')) {
		if (_isf_probe_unit[0] != 0) {
	        	DBG_DUMP("probe i/o end\r\n");
		    _isf_probe_unit[0] = 0;
		    _isf_probe_dir = -1;
		    _isf_probe_port = -1;
			_isf_probe_mask = 0x0000;
	    } else {
	        	DBG_DUMP("syntax: cmd probe [dev] [i/o] (mask)\r\n");
	        	DBG_DUMP("\r\n");
	        	DBG_DUMP("dev: d0, d1, d2, ...\r\n");
	        	DBG_DUMP("i/o: i0, i1, i2, ..., or o0, o1, o2, ...\r\n");
	        	DBG_DUMP("\r\n");
	        	DBG_DUMP("for i:");
	        	DBG_DUMP("mask: m1000 PUSH (default)\r\n");
	        	DBG_DUMP("mask: m2000 PUSH_DROP\r\n");
	        	DBG_DUMP("mask: m4000 PUSH_WRN\r\n");
	        	DBG_DUMP("mask: m8000 PUSH_ERR\r\n");
	        	DBG_DUMP("mask: m0100 EXT\r\n");
	        	DBG_DUMP("mask: m0200 EXT_DROP\r\n");
	        	DBG_DUMP("mask: m0400 EXT_WRN\r\n");
	        	DBG_DUMP("mask: m0800 EXT_ERR\r\n");
	        	DBG_DUMP("mask: m0010 PROC\r\n");
	        	DBG_DUMP("mask: m0020 PROC_DROP\r\n");
	        	DBG_DUMP("mask: m0040 PROC_WRN\r\n");
	        	DBG_DUMP("mask: m0080 PROC_ERR\r\n");
	        	DBG_DUMP("mask: m0001 REL\r\n");
	        	DBG_DUMP("\r\n");
	        	DBG_DUMP("for o:");
	        	DBG_DUMP("mask: m1000 NEW\r\n");
	        	DBG_DUMP("mask: m2000 NEW_DROP\r\n");
	        	DBG_DUMP("mask: m4000 NEW_WRN\r\n");
	        	DBG_DUMP("mask: m8000 NEW_ERR\r\n");
	        	DBG_DUMP("mask: m0100 PROC\r\n");
	        	DBG_DUMP("mask: m0200 PROC_DROP\r\n");
	        	DBG_DUMP("mask: m0400 PROC_WRN\r\n");
	        	DBG_DUMP("mask: m0800 PROC_ERR\r\n");
	        	DBG_DUMP("mask: m0010 EXT\r\n");
	        	DBG_DUMP("mask: m0020 EXT_DROP\r\n");
	        	DBG_DUMP("mask: m0040 EXT_WRN\r\n");
	        	DBG_DUMP("mask: m0080 EXT_ERR\r\n");
	        	DBG_DUMP("mask: m0001 PUSH or REL (default)\r\n");
	        	DBG_DUMP("mask: m0002 PUSH_DROP\r\n");
	        	DBG_DUMP("mask: m0004 PUSH_WRN\r\n");
	        	DBG_DUMP("mask: m0008 PUSH_ERR\r\n");
	    }
    } else if ((unit_name != 0) && ((port_name[0] == 'o') || (port_name[0] == 'i')))  {
	    strncpy(_isf_probe_unit, unit_name, 32);
	    if (port_name[0] == 'o')
		    _isf_probe_dir = 0;
		else
		    _isf_probe_dir = 1;
		_ATOI(port_name+1, &_isf_probe_port);
		if(port_name[0] == 'o')
			_isf_probe_mask = ISF_OUT_PROBE_PUSH;
		else
			_isf_probe_mask = ISF_IN_PROBE_PUSH;
		if ((mask_name != 0) && (mask_name[0] == 'm') && (strlen(mask_name) == 5)) {
			INT32 m1=0,m2=0,m3=0,m4=0;
			if (mask_name[1] >= 'a') m1 = mask_name[1] - 'a' + 10;else m1 = mask_name[1] - '0';
			if (mask_name[2] >= 'a') m2 = mask_name[2] - 'a' + 10;else m2 = mask_name[2] - '0';
			if (mask_name[3] >= 'a') m3 = mask_name[3] - 'a' + 10;else m3 = mask_name[3] - '0';
			if (mask_name[4] >= 'a') m4 = mask_name[4] - 'a' + 10;else m4 = mask_name[4] - '0';
			if (m1 < 16 && m2 < 16 && m3 < 16 && m4 < 16) {
				_isf_probe_mask = (m1 << 12) | (m2 << 8) | (m3 << 4) | (m4);
			}
		} else if ((mask_name != 0) && (mask_name[0] == 'm') && (strlen(mask_name) == 7)) {
			INT32 m1=0,m2=0,m3=0,m4=0,m5=0,m6=0;
			if (mask_name[1] >= 'a') m1 = mask_name[1] - 'a' + 10;else m1 = mask_name[1] - '0';
			if (mask_name[2] >= 'a') m2 = mask_name[2] - 'a' + 10;else m2 = mask_name[2] - '0';
			if (mask_name[3] >= 'a') m3 = mask_name[3] - 'a' + 10;else m3 = mask_name[3] - '0';
			if (mask_name[4] >= 'a') m4 = mask_name[4] - 'a' + 10;else m4 = mask_name[4] - '0';
			if (mask_name[5] >= 'a') m5 = mask_name[5] - 'a' + 10;else m5 = mask_name[5] - '0';
			if (mask_name[6] >= 'a') m6 = mask_name[6] - 'a' + 10;else m6 = mask_name[6] - '0';
			if (m1 < 16 && m2 < 16 && m3 < 16 && m4 < 16 && m4 < 16 && m5 < 16) {
				_isf_probe_mask = (m1 << 20) | (m2 << 16) | (m3 << 12) | (m4 << 8) | (m5 << 4) | (m6);
			}
		}
        	DBG_DUMP("probe i/o begin: \"%s\".%s[%d], action mask=0x%04x\r\n", _isf_probe_unit, portstr[_isf_probe_dir], _isf_probe_port, _isf_probe_mask);
    } else if ((unit_name[0] == '*') && (port_name[0] == '*'))  {
	    _isf_probe_unit[0] = '*';
	    _isf_probe_unit[1] = 0;
	    _isf_probe_dir = 0xff;
	    _isf_probe_port = 0xff;
		if(port_name[0] == 'o')
			_isf_probe_mask = ISF_OUT_PROBE_PUSH;
		else
			_isf_probe_mask = ISF_IN_PROBE_PUSH;
		if ((mask_name != 0) && (mask_name[0] == 'm') && (strlen(mask_name) == 5)) {
			INT32 m1=0,m2=0,m3=0,m4=0;
			if (mask_name[1] >= 'a') m1 = mask_name[1] - 'a' + 10;else m1 = mask_name[1] - '0';
			if (mask_name[2] >= 'a') m2 = mask_name[2] - 'a' + 10;else m2 = mask_name[2] - '0';
			if (mask_name[3] >= 'a') m3 = mask_name[3] - 'a' + 10;else m3 = mask_name[3] - '0';
			if (mask_name[4] >= 'a') m4 = mask_name[4] - 'a' + 10;else m4 = mask_name[4] - '0';
			if (m1 < 16 && m2 < 16 && m3 < 16 && m4 < 16) {
				_isf_probe_mask = (m1 << 12) | (m2 << 8) | (m3 << 4) | (m4);
				//_isf_probe_mask &= ~(_ISF_UNIT_PROBE_NEW_OK|_ISF_UNIT_PROBE_ADD_OK|_ISF_UNIT_PROBE_PUSH_OK|_ISF_UNIT_PROBE_RELEASE_OK);
			}
		}
        	DBG_DUMP("probe i/o begin: all unit, all i/o, action mask=0x%04x\r\n", _isf_probe_mask);
    }
	SEM_SIGNAL(ISF_SEM_ID);
}
EXPORT_SYMBOL(_isf_flow_probe_port);

void _isf_flow_perf_port(char* unit_name, char* port_name)
{
	SEM_WAIT(ISF_SEM_ID);
	if ((unit_name == 0) || (port_name == 0) || (port_name[0] != 'o' && port_name[0] != 'i')) {
		if (_isf_perf_unit[0] != 0) {
        	DBG_DUMP("perf i/o end\r\n");
		    _isf_perf_unit[0] = 0;
		    _isf_perf_dir = -1;
		    _isf_perf_port = -1;
	    } else {
	        	DBG_DUMP("syntax: cmd perf [dev] [i/o]\r\n");
	        	DBG_DUMP("\r\n");
	        	DBG_DUMP("dev: d0, d1, d2, ...\r\n");
	        	DBG_DUMP("i/o: i0, i1, i2, ..., or o0, o1, o2, ...\r\n");
	        	DBG_DUMP("\r\n");
	    }
    } else if ((unit_name != 0) && ((port_name[0] == 'o') || (port_name[0] == 'i')))  {

#if ISF_PERF
/*
	    //reset
		fps_t1 = fps_t2 = 0;
		fps_count=0;
	    //reset
		sps_t1 = sps_t2 = 0;
		sps_count=0;
	    //reset
		vbps_t1 = vbps_t2 = 0;
		vbps_count=0;
	    //reset
		abps_t1 = abps_t2 = 0;
		abps_diff = abps_count = 0;
*/
#endif

	    strncpy(_isf_perf_unit, unit_name, 32);
	    if (port_name[0]=='o')
		    _isf_perf_dir = 0;
		else
		    _isf_perf_dir = 1;
		_ATOI(port_name+1, &_isf_perf_port);
        	DBG_DUMP("perf i/o begin: \"%s\".%s[%d]\r\n", _isf_perf_unit, portstr[_isf_perf_dir], _isf_perf_port);
    }
	SEM_SIGNAL(ISF_SEM_ID);
}
EXPORT_SYMBOL(_isf_flow_perf_port);

void _isf_flow_save_port(char* unit_name, char* port_name, char* count_name)
{
	SEM_WAIT(ISF_SEM_ID);
	if ((unit_name == 0) || (port_name == 0) || (port_name[0] != 'o' && port_name[0] != 'i')) {
		if (_isf_save_unit[0] != 0) {
        	DBG_DUMP("save i/o end\r\n");
		    _isf_save_unit[0] = 0;
		    _isf_save_dir = -1;
		    _isf_save_port = -1;
		    _isf_save_count = 0;
	    } else {
	        	DBG_DUMP("syntax: cmd save [dev] [i/o] (count)\r\n");
	        	DBG_DUMP("\r\n");
	        	DBG_DUMP("dev: d0, d1, d2, ...\r\n");
	        	DBG_DUMP("i/o: i0, i1, i2, ..., or o0, o1, o2, ...\r\n");
	        	DBG_DUMP("count: range = 1~32, default 1\r\n");
	    }
    } else if ((unit_name != 0) && ((port_name[0] == 'o') || (port_name[0] == 'i'))) {
	    INT len = strlen(unit_name);
		INT32 count = 1;
	    if ((len == 0) || (len >= 33)) {
        	DBG_DUMP("invalid name?\r\n");
			SEM_SIGNAL(ISF_SEM_ID);
	    		return;
	    }
	    strncpy(_isf_save_unit, unit_name, 32);
	    if (_isf_save_unit[len-1] == '*') {
		    _isf_save_unit_len = len-1;
	    } else {
		    _isf_save_unit_len = len;
	    }
	    if (port_name[0]=='o')
		    _isf_save_dir = 0;
		else
		    _isf_save_dir = 1;
	    if (port_name[1] == '*') {
		    _isf_save_port =  999; //all ports
		} else {
			_ATOI(port_name+1, &_isf_save_port);
		}
		if ((count_name != 0) && (count_name[0] != 0)) {
			_ATOI(count_name, &count);
		}
	    if (count < 32) {
	 	    _isf_save_count = count;
 	    }
 	    if (_isf_save_port == 999) {
        		DBG_DUMP("save i/o begin: \"%s\".%s[*] count=%d\r\n", _isf_save_unit, portstr[_isf_save_dir], _isf_save_count);
        	} else {
        		DBG_DUMP("save i/o begin: \"%s\".%s[%d] count=%d\r\n", _isf_save_unit, portstr[_isf_save_dir], _isf_save_port, _isf_save_count);
    		}
    }
	SEM_SIGNAL(ISF_SEM_ID);
}
EXPORT_SYMBOL(_isf_flow_save_port);


#if 0
void _isf_stream_dump_stream(ISF_STREAM *p_stream)
{
	UINT32 i;
	UINT32 list_id = 0;
	if (!p_stream) {
		//DBG_ERR("(null)\r\n");
		return;
	}
    DBG_DUMP("[%s] ---------------------------------------------------------------------\r\n", p_stream->stream_name);
    if (!p_stream->connect_count) {
        DBG_DUMP("(empty)\r\n");
            DBG_DUMP("\r\n");
		return;
    }
	//for "all connected ports", do dump (in normal order)
	for (i = 0; i <= p_stream->queue_tail; i++) {
		//get unit
		ISF_UNIT *p_unit = p_stream->connect_list[i].p_unit;
		if (p_unit && (p_unit->list_id == 0)) {
			_isf_unit_dump_connectinfo(p_stream, p_unit);
			//_isf_unit_dump_pathinfo(p_stream, p_unit);
			list_id ++;
			p_unit->list_id = list_id;
		}
	}
	for (i = 0; i <= p_stream->queue_tail; i++) {
		//get unit
		ISF_UNIT *p_unit = p_stream->connect_list[i].p_unit;
		if (p_unit && (p_unit->list_id != 0)) {
			p_unit->list_id = 0;
		}
	}
    DBG_DUMP("\r\n");
}

void _isf_stream_dump_streambyname(char* stream_name)
{
	int i;
	SEM_WAIT(ISF_SEM_ID);
	for (i = 0; i < ISF_MAX_STREAM; i++) {
	    if ((stream_name == NULL) || (strncmp(isf_stream[i].stream_name, stream_name, 8) == 0)) {
            _isf_stream_dump_stream(&isf_stream[i]);
		}
	}
	if (stream_name == NULL) {
		nvtmpp_dump_status(debug_msg);
	}
	SEM_SIGNAL(ISF_SEM_ID);
}

void _isf_stream_dump_stream2(ISF_STREAM *p_stream)
{
	UINT32 i;
	UINT32 list_id = 0;
	if (!p_stream) {
		//DBG_ERR("(null)\r\n");
		return;
	}
    if (!p_stream->connect_count) {
		return;
    }
	//for "all connected ports", do dump (in normal order)
	for (i = 0; i <= p_stream->queue_tail; i++) {
		//get unit
		ISF_UNIT *p_unit = p_stream->connect_list[i].p_unit;
		if (p_unit && (p_unit->list_id == 0)) {
			INT32 pi;
			//INT32 pd = -1;
			ISF_PORT* p_destport = _isf_unit_out(p_unit, p_stream->connect_list[i].oport);
			if ((_isf_dump_port == 0xff) && (strncmp(p_unit->unit_name, _isf_dump_unit, 32) == 0)) { //whole unit
    DBG_DUMP("[%s] ---------------------------------------------------------------------\r\n", p_unit->unit_name);
	            _isf_unit_dump_pathinfo(p_unit);
	            _isf_unit_dump_connectinfo(NULL, p_unit);
				_isf_dump_unit[0] = 0;
				_isf_dump_port = -1;
				_isf_dump_dir = -1;
			} else if ((_isf_dump_dir == 0)) { //output port
				//pd = 0;
				pi = p_destport->oport - ISF_OUT_BASE;
				if ((_isf_dump_port == pi) && (strncmp(p_unit->unit_name, _isf_dump_unit, 32) == 0)) {
					_isf_unit_dump_port(p_unit, p_destport->oport);
					_isf_dump_unit[0] = 0;
					_isf_dump_port = -1;
					_isf_dump_dir = -1;
				}
			} else if ((_isf_dump_dir == 1) && (p_destport->p_destunit != 0)) { //input port
				//pd = 1;
				pi = p_destport->iport - ISF_IN_BASE;
				if ((_isf_dump_port == pi) && (strncmp(p_destport->p_destunit->unit_name, _isf_dump_unit, 32) == 0)) {
					_isf_unit_dump_port(p_destport->p_destunit, p_destport->iport);
					_isf_dump_unit[0] = 0;
					_isf_dump_port = -1;
					_isf_dump_dir = -1;
				}
			}
			list_id ++;
			p_unit->list_id = list_id;
		}
	}
	for (i = 0; i <= p_stream->queue_tail; i++) {
		//get unit
		ISF_UNIT *p_unit = p_stream->connect_list[i].p_unit;
		if (p_unit && (p_unit->list_id != 0)) {
			p_unit->list_id = 0;
		}
	}
	if (_isf_dump_unit[0] != 0) {
		if (_isf_dump_port == 0xff) { DBG_ERR("this unit is not found!\r\n");}
		else { DBG_ERR("this port is not found or not connected!\r\n");}
		_isf_dump_unit[0] = 0;
		_isf_dump_port = -1;
		_isf_dump_dir = -1;
	}
    DBG_DUMP("\r\n");
}

void _isf_stream_dump_streambyname2(char* stream_name)
{
	int i;
	SEM_WAIT(ISF_SEM_ID);
	for (i = 0; i < ISF_MAX_STREAM; i++) {
	    if ((stream_name == NULL) || (strncmp(isf_stream[i].stream_name, stream_name, 8) == 0)) {
            _isf_stream_dump_stream2(&isf_stream[i]);
		}
	}
	SEM_SIGNAL(ISF_SEM_ID);
}
#endif

#if 0 //not used yet
static void _isf_unit_dump_port_backtrace(ISF_UNIT *p_unit, UINT32 nport)
{
	if (!p_unit) {
		return;
	}
	if ((nport < (int)ISF_IN_BASE) && (nport < p_unit->port_out_count)) {
		ISF_PORT_PATH *p_path;
		UINT32 j;

		//oport
		p_path = p_unit->port_path;
		for (j = 0; j < p_unit->port_path_count; j++) { //use unit's port path to find related dest port
			if ((nport == p_path->oport) && (ISF_CTRL != p_path->iport)) { //use oport to find iport
				//iport is found
				ISF_PORT* p_srcport = _isf_unit_in(p_unit, p_path->iport);
				if (p_srcport != NULL) {
					_isf_unit_dump_port_backtrace(p_srcport->p_srcunit, p_srcport->oport);
				}
			}
			p_path ++;
		}

		//DBG_DUMP("backtrace port: \"%s\".%s[%d]\r\n", p_unit->unit_name, portstr[0], nport - ISF_OUT_BASE);
		_isf_unit_dump_port(p_unit, nport); //dump this
	} else if (nport-ISF_IN_BASE < p_unit->port_in_count) {
		//iport
		ISF_PORT* p_srcport;
		//DBG_DUMP("backtrace port: \"%s\".%s[%d]\r\n", p_unit->unit_name, portstr[1], nport - ISF_IN_BASE);
		p_srcport = _isf_unit_in(p_unit, nport);

		if (p_srcport != NULL) {
			_isf_unit_dump_port_backtrace(p_srcport->p_srcunit, p_srcport->oport);
		}
	}
	return;
}
#endif

#if 0
void _isf_stream_dump_stream3(ISF_STREAM *p_stream)
{
	UINT32 i;
	UINT32 list_id = 0;
	if (!p_stream) {
		//DBG_ERR("(null)\r\n");
		return;
	}
    if (!p_stream->connect_count) {
		return;
    }
	//for "all connected ports", do dump (in normal order)
	for (i = 0; i <= p_stream->queue_tail; i++) {
		//get unit
		ISF_UNIT *p_unit = p_stream->connect_list[i].p_unit;
		if (p_unit && (p_unit->list_id == 0)) {
			if ((_isf_dump_port == 0xff) && (strncmp(p_unit->unit_name, _isf_dump_unit, 32) == 0)) { //whole unit
				//do nothing
			} else if ((_isf_dump_dir == 0)) { //output port
        			//DBG_DUMP("x backtrace port: \"%s\".%s[%d]\r\n", _isf_dump_unit, portstr[_isf_dump_dir], _isf_dump_port);
				if((strncmp(p_unit->unit_name, _isf_dump_unit, 32)==0)) {
					_isf_unit_dump_port_backtrace(p_unit, _isf_dump_port + ISF_OUT_BASE);
					_isf_dump_unit[0] = 0;
					_isf_dump_port = -1;
					_isf_dump_dir = -1;
				}
			} else if((_isf_dump_dir == 1)) { //input port
        			//DBG_DUMP("x backtrace port: \"%s\".%s[%d]\r\n", _isf_dump_unit, portstr[_isf_dump_dir], _isf_dump_port);
				if((strncmp(p_unit->unit_name, _isf_dump_unit, 32)==0)) {
					_isf_unit_dump_port_backtrace(p_unit, _isf_dump_port + ISF_IN_BASE);
					_isf_dump_unit[0] = 0;
					_isf_dump_port = -1;
					_isf_dump_dir = -1;
				}
			}
			list_id ++;
			p_unit->list_id = list_id;
		}
	}
	for (i = 0; i <= p_stream->queue_tail; i++) {
		//get unit
		ISF_UNIT *p_unit = p_stream->connect_list[i].p_unit;
		if (p_unit && (p_unit->list_id != 0)) {
			p_unit->list_id = 0;
		}
	}
    DBG_DUMP("\r\n");
}

void _isf_stream_dump_streambyname3(char* stream_name)
{
	int i;
	SEM_WAIT(ISF_SEM_ID);
	for (i = 0; i < ISF_MAX_STREAM; i++) {
	    if ((stream_name == NULL) || (strncmp(isf_stream[i].stream_name, stream_name, 9) == 0)) {
            _isf_stream_dump_stream3(&isf_stream[i]);
            if(stream_name != NULL) {
				if (_isf_dump_unit[0] != 0) {
					if (_isf_dump_port == 0xff) { DBG_ERR("cannot backtrace dump!\r\n");}
					else { DBG_ERR("this port is not found or not connected!\r\n");}
					_isf_dump_unit[0] = 0;
					_isf_dump_port = -1;
					_isf_dump_dir = -1;
				}
				SEM_SIGNAL(ISF_SEM_ID);
				return;
            }
		}
	}
	if (_isf_dump_unit[0] != 0) {
		if (_isf_dump_port == 0xff) { DBG_ERR("cannot backtrace dump!\r\n");}
		else { DBG_ERR("this port is not found or not connected!\r\n");}
		_isf_dump_unit[0] = 0;
		_isf_dump_port = -1;
		_isf_dump_dir = -1;
	}
	SEM_SIGNAL(ISF_SEM_ID);
}
#endif

#if 0
void _isf_stream_dump_port(char* unit_name, char* port_name)
{
	SEM_WAIT(ISF_SEM_ID);
	if ((unit_name == 0) || (port_name[0] != 0 && port_name[0] != 'o' && port_name[0] != 'i')) {
		if (_isf_dump_unit[0] != 0) {
        	//DBG_DUMP("trace port end\r\n");
	    _isf_dump_unit[0] = 0;
	    _isf_dump_dir = -1;
	    _isf_dump_port = -1;
	    }
    } else if ((unit_name != 0) && (port_name[0] == 0))  {
	    strncpy(_isf_dump_unit, unit_name, 32);
	    _isf_dump_dir = 0xff;
	    _isf_dump_port = 0xff;
        	//DBG_DUMP("trace port begin: \"%s\".%s[%d]\r\n", _isf_dump_unit, portstr[_isf_dump_dir], _isf_dump_port);
    } else if ((unit_name != 0) && ((port_name[0] == 'o') || (port_name[0] == 'i')))  {
	    strncpy(_isf_dump_unit, unit_name, 32);
	    if (port_name[0] == 'o')
		    _isf_dump_dir = 0;
		else
		    _isf_dump_dir = 1;
		_ATOI(port_name+1, &_isf_dump_port);
        	//DBG_DUMP("trace port begin: \"%s\".%s[%d]\r\n", _isf_dump_unit, portstr[_isf_dump_dir], _isf_dump_port);
    }
	SEM_SIGNAL(ISF_SEM_ID);
	_isf_stream_dump_streambyname2(NULL);
}

void _isf_stream_dump_port3(char* unit_name, char* port_name)
{
	SEM_WAIT(ISF_SEM_ID);
	if ((unit_name == 0) || (port_name[0] != 0 && port_name[0] != 'o' && port_name[0] != 'i')) {
		if (_isf_dump_unit[0] != 0) {
        	//DBG_DUMP("trace port end\r\n");
	    _isf_dump_unit[0] = 0;
	    _isf_dump_dir = -1;
	    _isf_dump_port = -1;
	    }
    } else if ((unit_name != 0) && (port_name[0] == 0))  {
	    strncpy(_isf_dump_unit, unit_name, 32);
	    _isf_dump_dir = 0xff;
	    _isf_dump_port = 0xff;
        	//DBG_DUMP("trace port begin: \"%s\".%s[%d]\r\n", _isf_dump_unit, portstr[_isf_dump_dir], _isf_dump_port);
    } else if ((unit_name != 0) && ((port_name[0] == 'o') || (port_name[0] == 'i')))  {
	    strncpy(_isf_dump_unit, unit_name, 32);
	    if (port_name[0] == 'o')
		    _isf_dump_dir = 0;
		else
		    _isf_dump_dir = 1;
		_ATOI(port_name+1, &_isf_dump_port);
        	//DBG_DUMP("trace port begin: \"%s\".%s[%d]\r\n", _isf_dump_unit, portstr[_isf_dump_dir], _isf_dump_port);
    }
	SEM_SIGNAL(ISF_SEM_ID);
	_isf_stream_dump_streambyname3(NULL);
}
#endif

void _isf_debug(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 opclass, const char *fmtstr, ...)
{
	INT32 pd = -1, pi = 0;

    if (!(_isf_debug_mask & opclass) || !p_thisunit)
    		return;

	if (((_isf_debug_dir == 0)||(_isf_debug_dir == 0xff))) { //output port
		if (nport < ISF_IN_BASE) {
			pd = 0;
			pi = nport - ISF_OUT_BASE;
		} else if (nport == ISF_CTRL) {
			pd = 2;
			pi = 0;
		}
	} else if (((_isf_debug_dir == 1)||(_isf_debug_dir == 0xff))) { //input port
		if (nport == ISF_CTRL) {
			pd = 2;
			pi = 0;
		} else if (nport >= ISF_IN_BASE) {
			pd = 1;
			pi = nport - ISF_IN_BASE;
		}
	} else if (((_isf_debug_dir == 2)||(_isf_debug_dir == 0xff))  && (nport == ISF_CTRL)) { //ctrl port
		pd = 2;
		pi = 0;
	}

	if (pd < 0) {
		return;
	}

    /// debug
	if ( ((_isf_debug_port == 0xff) && (_isf_debug_unit[0] == '*'))
	|| ((_isf_debug_port == 0xff) && (strncmp(p_thisunit->unit_name, _isf_debug_unit, 32) == 0))
	|| ((_isf_debug_port == pi) && (strncmp(p_thisunit->unit_name, _isf_debug_unit, 32) == 0)) ) {

		CHAR new_fmtstr[256] = {0};
		CHAR* k_new_fmtstr = new_fmtstr;
		int len = 256;
		va_list marker;

		if (((opclass & ISF_OP_STATE) || (opclass & ISF_OP_BIND)) && (fmtstr[0]=='\r') && (fmtstr[1]=='\n')) {
			snprintf(k_new_fmtstr, len, "\r\n");
			fmtstr += 2;
			k_new_fmtstr += 2;
			len -= 2;
		}

		if (nport == ISF_CTRL) {
		if (opclass & 0xf000) { snprintf(k_new_fmtstr, len, msg_d3, p_thisunit->unit_name, fmtstr); }
		if (opclass & 0x0fff) { snprintf(k_new_fmtstr, len, msg_d4, p_thisunit->unit_name, fmtstr); }
		} else {
		if (opclass & 0xf000) { snprintf(k_new_fmtstr, len, msg_d1, p_thisunit->unit_name, portstr[pd], pi, fmtstr); }
		if (opclass & 0x0fff) { snprintf(k_new_fmtstr, len, msg_d2, p_thisunit->unit_name, portstr[pd], pi, fmtstr); }
		}

		va_start(marker, fmtstr);

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
		//coverity[dereference]
		debug_vprintf(new_fmtstr, marker);
#else
#ifdef __KERNEL__
		debug_vprintf(new_fmtstr, marker);
#else
		if (marker) {
			debug_vprintf(new_fmtstr, marker);
		}
#endif
#endif

		va_end(marker);
	}
}
//EXPORT_SYMBOL(p_unit->p_base->do_debug);

void _isf_flow_debug_port(char* unit_name, char* port_name, char* mask_name)
{
	SEM_WAIT(ISF_SEM_ID);
	if ((unit_name == 0) || (port_name == 0) || (port_name[0] != '*' && port_name[0] != 'c' && port_name[0] != 'o' && port_name[0] != 'i')) {
		if (_isf_debug_unit[0] != 0) {
	        	DBG_DUMP("debug i/o end\r\n");
		    _isf_debug_unit[0] = 0;
		    _isf_debug_dir = -1;
		    _isf_debug_port = -1;
			_isf_debug_mask = 0;
	    } else {
	        	DBG_DUMP("syntax: cmd debug [dev] [i/o] (mask)\r\n");
	        	DBG_DUMP("\r\n");
	        	DBG_DUMP("dev: d0, d1, d2, ...\r\n");
	        	DBG_DUMP("i/o: i0, i1, i2, ..., or o0, o1, o2, ..., or c\r\n");
	        	DBG_DUMP("\r\n");
	        	DBG_DUMP("mask: m1000 command dispatch (default)\r\n");
	        	DBG_DUMP("mask: m2000 state operation (default)\r\n");
	        	DBG_DUMP("mask: m4000 bind operation (default)\r\n");
	        	DBG_DUMP("mask: m0100 ctrl parameter\r\n");
	        	DBG_DUMP("mask: m0200 ctrl IMM parameter\r\n");
	        	DBG_DUMP("mask: m0400 general parameter\r\n");
	        	DBG_DUMP("mask: m0800 general IMM parameter\r\n");
	        	DBG_DUMP("mask: m0010 video frame parameter\r\n");
	        	DBG_DUMP("mask: m0020 video frame IMM parameter\r\n");
	        	DBG_DUMP("mask: m0040 video bitstream\r\n");
	        	DBG_DUMP("mask: m0080 video bitstream IMM parameter\r\n");
	        	DBG_DUMP("mask: m0001 audio frame parameter\r\n");
	        	DBG_DUMP("mask: m0002 audio frame IMM parameter\r\n");
	        	DBG_DUMP("mask: m0004 audio bitstream parameter\r\n");
	        	DBG_DUMP("mask: m0008 audio bitstream IMM parameter\r\n");
	    }
    } else if ((unit_name[0] == '*') && (port_name[0] == '*'))  {
	    _isf_debug_unit[0] = '*';
	    _isf_debug_unit[1] = 0;
	    _isf_debug_dir = 0xff;
	    _isf_debug_port = 0xff;
		_isf_debug_mask = 0xf000;
		if ((mask_name != 0) && (mask_name[0] == 'm') && (strlen(mask_name) == 5)) {
			INT32 m1=0,m2=0,m3=0,m4=0;
			if (mask_name[1] >= 'a') m1 = mask_name[1] - 'a' + 10;else m1 = mask_name[1] - '0';
			if (mask_name[2] >= 'a') m2 = mask_name[2] - 'a' + 10;else m2 = mask_name[2] - '0';
			if (mask_name[3] >= 'a') m3 = mask_name[3] - 'a' + 10;else m3 = mask_name[3] - '0';
			if (mask_name[4] >= 'a') m4 = mask_name[4] - 'a' + 10;else m4 = mask_name[4] - '0';
			if (m1 < 16 && m2 < 16 && m3 < 16 && m4 < 16) {
				_isf_debug_mask = (m1 << 12) | (m2 << 8) | (m3 << 4) | (m4);
			}
		}
        	DBG_DUMP("debug i/o begin: all unit, all i/o, action mask=0x%03x\r\n", _isf_debug_mask);
    } else if ((unit_name != 0) && (port_name[0] == '*'))  {
	    strncpy(_isf_debug_unit, unit_name, 32);
	    _isf_debug_dir = 0xff;
	    _isf_debug_port = 0xff;
		_isf_debug_mask = 0xf000;
		if ((mask_name != 0) && (mask_name[0] == 'm') && (strlen(mask_name) == 5)) {
			INT32 m1=0,m2=0,m3=0,m4=0;
			if (mask_name[1] >= 'a') m1 = mask_name[1] - 'a' + 10;else m1 = mask_name[1] - '0';
			if (mask_name[2] >= 'a') m2 = mask_name[2] - 'a' + 10;else m2 = mask_name[2] - '0';
			if (mask_name[3] >= 'a') m3 = mask_name[3] - 'a' + 10;else m3 = mask_name[3] - '0';
			if (mask_name[4] >= 'a') m4 = mask_name[4] - 'a' + 10;else m4 = mask_name[4] - '0';
			if (m1 < 16 && m2 < 16 && m3 < 16 && m4 < 16) {
				_isf_debug_mask = (m1 << 12) | (m2 << 8) | (m3 << 4) | (m4);
			}
		}
        	DBG_DUMP("debug i/o begin: \"%s\", all i/o, action mask=0x%03x\r\n", _isf_debug_unit, _isf_debug_mask);
    } else if ((unit_name != 0) && ((port_name[0] == 'o') || (port_name[0] == 'i')))  {
	    strncpy(_isf_debug_unit, unit_name, 32);
 	    if (port_name[0] == 'o')
		    _isf_debug_dir = 0;
		else
		    _isf_debug_dir = 1;
		_ATOI(port_name+1, &_isf_debug_port);
		_isf_debug_mask = 0xf000;
		if ((mask_name != 0) && (mask_name[0] == 'm') && (strlen(mask_name) == 5)) {
			INT32 m1=0,m2=0,m3=0,m4=0;
			if (mask_name[1] >= 'a') m1 = mask_name[1] - 'a' + 10;else m1 = mask_name[1] - '0';
			if (mask_name[2] >= 'a') m2 = mask_name[2] - 'a' + 10;else m2 = mask_name[2] - '0';
			if (mask_name[3] >= 'a') m3 = mask_name[3] - 'a' + 10;else m3 = mask_name[3] - '0';
			if (mask_name[4] >= 'a') m4 = mask_name[4] - 'a' + 10;else m4 = mask_name[4] - '0';
			if (m1 < 16 && m2 < 16 && m3 < 16 && m4 < 16) {
				_isf_debug_mask = (m1 << 12) | (m2 << 8) | (m3 << 4) | (m4);
			}
		}
       	DBG_DUMP("debug i/o begin: \"%s\".%s[%d], action mask=0x%03x\r\n", _isf_debug_unit, portstr[_isf_debug_dir], _isf_debug_port, _isf_debug_mask);
    } else if ((unit_name != 0) && (port_name[0] == 'c'))  {
	    strncpy(_isf_debug_unit, unit_name, 32);
	    _isf_debug_dir = 2;
    		_isf_debug_port = 0;
		_isf_debug_mask = 0xf000;
		if ((mask_name != 0) && (mask_name[0] == 'm') && (strlen(mask_name) == 5)) {
			INT32 m1=0,m2=0,m3=0,m4=0;
			if (mask_name[1] >= 'a') m1 = mask_name[1] - 'a' + 10;else m1 = mask_name[1] - '0';
			if (mask_name[2] >= 'a') m2 = mask_name[2] - 'a' + 10;else m2 = mask_name[2] - '0';
			if (mask_name[3] >= 'a') m3 = mask_name[3] - 'a' + 10;else m3 = mask_name[3] - '0';
			if (mask_name[4] >= 'a') m4 = mask_name[4] - 'a' + 10;else m4 = mask_name[4] - '0';
			if (m1 < 16 && m2 < 16 && m3 < 16 && m4 < 16) {
				_isf_debug_mask = (m1 << 12) | (m2 << 8) | (m3 << 4) | (m4);
			}
		}
        	DBG_DUMP("debug i/o begin: \"%s\".ctrl, action mask=0x%03x\r\n", _isf_debug_unit, _isf_debug_mask);
    }
	SEM_SIGNAL(ISF_SEM_ID);
}
EXPORT_SYMBOL(_isf_flow_debug_port);


void _isf_trace(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 opclass, const char *fmtstr, ...)
{
	INT32 pd = -1, pi = 0;

    if (!(_isf_trace_mask & opclass) || !p_thisunit)
    		return;

	if (((_isf_trace_dir == 0)||(_isf_trace_dir == 0xff))) { //output port
		if (nport < ISF_IN_BASE) {
			pd = 0;
			pi = nport - ISF_OUT_BASE;
		} else if (nport == ISF_CTRL) {
			pd = 2;
			pi = 0;
		}
	} else if (((_isf_trace_dir == 1)||(_isf_trace_dir == 0xff))) { //input port
		if (nport == ISF_CTRL) {
			pd = 2;
			pi = 0;
		} else if (nport >= ISF_IN_BASE) {
			pd = 1;
			pi = nport - ISF_IN_BASE;
		}
	} else if (((_isf_trace_dir == 2)||(_isf_trace_dir == 0xff)) && (nport == ISF_CTRL)) { //ctrl port
		pd = 2;
		pi = 0;
	}

	if (pd < 0) {
		return;
	}

    /// trace
	if ( ((_isf_trace_port == 0xff) && (_isf_trace_unit[0] == '*'))
	|| ((_isf_trace_port == 0xff) && (strncmp(p_thisunit->unit_name, _isf_trace_unit, 32) == 0))
	|| ((_isf_trace_port == pi) && (strncmp(p_thisunit->unit_name, _isf_trace_unit, 32) == 0)) ) {

		CHAR new_fmtstr[256] = {0};
		va_list marker;

		if (nport == ISF_CTRL) {
		if (opclass & 0xf000) { snprintf(new_fmtstr, 256, msg_t3, p_thisunit->unit_name, fmtstr); }
		if (opclass & 0x0fff) { snprintf(new_fmtstr, 256, msg_t4, p_thisunit->unit_name, fmtstr); }
		} else {
		if (opclass & 0xf000) { snprintf(new_fmtstr, 256, msg_t1, p_thisunit->unit_name, portstr[pd], pi, fmtstr); }
		if (opclass & 0x0fff) { snprintf(new_fmtstr, 256, msg_t2, p_thisunit->unit_name, portstr[pd], pi, fmtstr); }
		}

		va_start(marker, fmtstr);

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
		//coverity[dereference]
		debug_vprintf(new_fmtstr, marker);
#else
#ifdef __KERNEL__
		debug_vprintf(new_fmtstr, marker);
#else
		if (marker) {
			debug_vprintf(new_fmtstr, marker);
		}
#endif
#endif

		va_end(marker);
	}
}
//EXPORT_SYMBOL(ISF_UNIT_TRACE);

void _isf_flow_trace_port(char* unit_name, char* port_name, char* mask_name)
{
	SEM_WAIT(ISF_SEM_ID);
	if ((unit_name == 0) || (port_name == 0) || (port_name[0] != '*' && port_name[0] != 'c' && port_name[0] != 'o' && port_name[0] != 'i')) {
		if (_isf_trace_unit[0] != 0) {
	        	DBG_DUMP("trace i/o end\r\n");
		    _isf_trace_unit[0] = 0;
		    _isf_trace_dir = -1;
		    _isf_trace_port = -1;
			_isf_trace_mask = 0;
	    } else {
	        	DBG_DUMP("syntax: cmd trace [dev] [i/o] (mask)\r\n");
	        	DBG_DUMP("\r\n");
	        	DBG_DUMP("dev: d0, d1, d2, ...\r\n");
	        	DBG_DUMP("i/o: i0, i1, i2, ..., or o0, o1, o2, ..., or c\r\n");
	        	DBG_DUMP("\r\n");
	        	DBG_DUMP("mask: m1000 command dispatch (default)\r\n");
	        	DBG_DUMP("mask: m2000 state operation (default)\r\n");
	        	DBG_DUMP("mask: m4000 bind operation (default)\r\n");
	        	DBG_DUMP("mask: m0100 ctrl parameter\r\n");
	        	DBG_DUMP("mask: m0200 ctrl IMM parameter\r\n");
	        	DBG_DUMP("mask: m0400 general parameter\r\n");
	        	DBG_DUMP("mask: m0800 general IMM parameter\r\n");
	        	DBG_DUMP("mask: m0010 video frame parameter\r\n");
	        	DBG_DUMP("mask: m0020 video frame IMM parameter\r\n");
	        	DBG_DUMP("mask: m0040 video bitstream\r\n");
	        	DBG_DUMP("mask: m0080 video bitstream IMM parameter\r\n");
	        	DBG_DUMP("mask: m0001 audio frame parameter\r\n");
	        	DBG_DUMP("mask: m0002 audio frame IMM parameter\r\n");
	        	DBG_DUMP("mask: m0004 audio bitstream parameter\r\n");
	        	DBG_DUMP("mask: m0008 audio bitstream IMM parameter\r\n");
	    }
    } else if ((unit_name[0] == '*') && (port_name[0] == '*'))  {
	    _isf_trace_unit[0] = '*';
	    _isf_trace_unit[1] = 0;
	    _isf_trace_dir = 0xff;
	    _isf_trace_port = 0xff;
		_isf_trace_mask = 0xf000;
		if ((mask_name != 0) && (mask_name[0] == 'm') && (strlen(mask_name) == 5)) {
			INT32 m1=0,m2=0,m3=0,m4=0;
			if (mask_name[1] >= 'a') m1 = mask_name[1] - 'a' + 10;else m1 = mask_name[1] - '0';
			if (mask_name[2] >= 'a') m2 = mask_name[2] - 'a' + 10;else m2 = mask_name[2] - '0';
			if (mask_name[3] >= 'a') m3 = mask_name[3] - 'a' + 10;else m3 = mask_name[3] - '0';
			if (mask_name[4] >= 'a') m4 = mask_name[4] - 'a' + 10;else m4 = mask_name[4] - '0';
			if (m1 < 16 && m2 < 16 && m3 < 16 && m4 < 16) {
				_isf_trace_mask = (m1 << 12) | (m2 << 8) | (m3 << 4) | (m4);
			}
		}
        	DBG_DUMP("trace i/o begin: all unit, all i/o, action mask=0x%03x\r\n", _isf_trace_mask);
    } else if ((unit_name != 0) && (port_name[0] == '*'))  {
	    strncpy(_isf_trace_unit, unit_name, 32);
	    _isf_trace_dir = 0xff;
	    _isf_trace_port = 0xff;
		_isf_trace_mask = 0xf000;
		if ((mask_name != 0) && (mask_name[0] == 'm') && (strlen(mask_name) == 5)) {
			INT32 m1=0,m2=0,m3=0,m4=0;
			if (mask_name[1] >= 'a') m1 = mask_name[1] - 'a' + 10;else m1 = mask_name[1] - '0';
			if (mask_name[2] >= 'a') m2 = mask_name[2] - 'a' + 10;else m2 = mask_name[2] - '0';
			if (mask_name[3] >= 'a') m3 = mask_name[3] - 'a' + 10;else m3 = mask_name[3] - '0';
			if (mask_name[4] >= 'a') m4 = mask_name[4] - 'a' + 10;else m4 = mask_name[4] - '0';
			if (m1 < 16 && m2 < 16 && m3 < 16 && m4 < 16) {
				_isf_trace_mask = (m1 << 12) | (m2 << 8) | (m3 << 4) | (m4);
			}
		}
        	DBG_DUMP("trace i/o begin: \"%s\", all i/o, action mask=0x%03x\r\n", _isf_trace_unit, _isf_trace_mask);
    } else if ((unit_name != 0) && ((port_name[0] == 'o') || (port_name[0] == 'i')))  {
	    strncpy(_isf_trace_unit, unit_name, 32);
 	    if (port_name[0]=='o')
		    _isf_trace_dir = 0;
		else
		    _isf_trace_dir = 1;
		_ATOI(port_name+1, &_isf_trace_port);
		_isf_trace_mask = 0xf000;
		if ((mask_name != 0) && (mask_name[0] == 'm') && (strlen(mask_name) == 5)) {
			INT32 m1=0,m2=0,m3=0,m4=0;
			if (mask_name[1] >= 'a') m1 = mask_name[1] - 'a' + 10;else m1 = mask_name[1] - '0';
			if (mask_name[2] >= 'a') m2 = mask_name[2] - 'a' + 10;else m2 = mask_name[2] - '0';
			if (mask_name[3] >= 'a') m3 = mask_name[3] - 'a' + 10;else m3 = mask_name[3] - '0';
			if (mask_name[4] >= 'a') m4 = mask_name[4] - 'a' + 10;else m4 = mask_name[4] - '0';
			if (m1 < 16 && m2 < 16 && m3 < 16 && m4 < 16) {
				_isf_trace_mask = (m1 << 12) | (m2 << 8) | (m3 << 4) | (m4);
			}
		}
        	DBG_DUMP("trace i/o begin: \"%s\".%s[%d], action mask=0x%03x\r\n", _isf_trace_unit, portstr[_isf_trace_dir], _isf_trace_port, _isf_trace_mask);
    } else if ((unit_name != 0) && (port_name[0] == 'c'))  {
	    strncpy(_isf_trace_unit, unit_name, 32);
	    _isf_trace_dir = 2;
    		_isf_trace_port = 0;
		_isf_trace_mask = 0xf000;
		if ((mask_name != 0) && (mask_name[0] == 'm') && (strlen(mask_name) == 5)) {
			INT32 m1=0,m2=0,m3=0,m4=0;
			if (mask_name[1] >= 'a') m1 = mask_name[1] - 'a' + 10;else m1 = mask_name[1] - '0';
			if (mask_name[2] >= 'a') m2 = mask_name[2] - 'a' + 10;else m2 = mask_name[2] - '0';
			if (mask_name[3] >= 'a') m3 = mask_name[3] - 'a' + 10;else m3 = mask_name[3] - '0';
			if (mask_name[4] >= 'a') m4 = mask_name[4] - 'a' + 10;else m4 = mask_name[4] - '0';
			if (m1 < 16 && m2 < 16 && m3 < 16 && m4 < 16) {
				_isf_trace_mask = (m1 << 12) | (m2 << 8) | (m3 << 4) | (m4);
			}
		}
        	DBG_DUMP("trace i/o begin: \"%s\".ctrl, action mask=0x%03x\r\n", _isf_trace_unit, _isf_trace_mask);
    }
	SEM_SIGNAL(ISF_SEM_ID);
}
EXPORT_SYMBOL(_isf_flow_trace_port);


