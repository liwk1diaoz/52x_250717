#include "isf_vdocap_api.h"
#include "isf_vdocap_int.h"
#include "isf_vdocap_dbg.h"
#if defined(_BSP_NA51000_)
#define SXCMD_NEW
#include "kwrap/sxcmd.h"
#else
#include "kwrap/sxcmd.h"
#endif
#include "kwrap/debug.h"

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#include "kwrap/cmdsys.h"
#define simple_strtoul(param1, param2, param3) strtoul(param1, param2, param3)
#include "stdlib.h"
extern BOOL _isf_vdocap_api_info(unsigned char argc, char **pargv);
#endif

extern BOOL _isf_vdocap_api_drv(unsigned char argc, char **pargv);
extern BOOL _isf_vdocap_api_reg(unsigned char argc, char **pargv);


//temp solution
#if defined(_BSP_NA51000_)
#undef _BSP_NA51000_
#endif

// cmd debug d0 o0 /mask=000
extern void _isf_flow_debug_port(char* unit_name, char* port_name, char* mask_name);
static BOOL _isf_vdocap_api_debug(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 1) || (dev[0]!='d')) { _isf_flow_debug_port(0,0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdocap%s", dev+1);
	//verify i/o
	if((argc < 2) || ((io[0]!='i') && (io[0]!='o') && (io[0]!='c'))) { _isf_flow_debug_port(0,0,0); return 1;}
	//do debug
	_isf_flow_debug_port(unit_name, io, mask);
	return 1;
}

// cmd trace d0 o0 /mask=000
extern void _isf_flow_trace_port(char* unit_name, char* port_name, char* mask_name);
static BOOL _isf_vdocap_api_trace(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 1) || (dev[0]!='d')) { _isf_flow_trace_port(0,0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdocap%s", dev+1);
	//verify i/o
	if((argc < 2) || ((io[0]!='i') && (io[0]!='o') && (io[0]!='c'))) { _isf_flow_trace_port(0,0,0); return 1;}
	//do trace
	_isf_flow_trace_port(unit_name, io, mask);
	return 1;
}

// cmd probe d0 o0 /mask=000
extern void _isf_flow_probe_port(char* unit_name, char* port_name, char* mask_name);
static BOOL _isf_vdocap_api_probe(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 1) || (dev[0]!='d')) { _isf_flow_probe_port(0,0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdocap%s", dev+1);
	//verify i/o
	if((argc < 2) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_probe_port(0,0,0); return 1;}
	//do probe
	_isf_flow_probe_port(unit_name, io, mask);
	return 1;
}

// cmd perf d0 o0
extern void _isf_flow_perf_port(char* unit_name, char* port_name);
static BOOL _isf_vdocap_api_perf(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 1) || (dev[0]!='d')) { _isf_flow_perf_port(0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdocap%s", dev+1);
	//verify i/o
	if((argc < 2) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_perf_port(0,0); return 1;}
	//do perf
	_isf_flow_perf_port(unit_name, io);
	return 1;
}

// cmd save d0 o0 count
extern void _isf_flow_save_port(char* unit_name, char* port_name, char* count_name);
static BOOL _isf_vdocap_api_save(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* count = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 1) || (dev[0]!='d')) { _isf_flow_save_port(0,0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdocap%s", dev+1);
	//verify i/o
	if((argc < 2) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_save_port(0,0,0); return 1;}
	//do save
	_isf_flow_save_port(unit_name, io, count);
	return 1;
}

#if 0
static BOOL _isf_vdocap_api_dbg_lv(unsigned char argc, char **pargv)
{
	if (argc != 2) {
		DBG_ERR("wrong argument:%d", argc);
		return 0;
	}
	isf_vdocap_debug_level = simple_strtoul(pargv[0], NULL, 0);
	return 1;
}
#endif
static void _vdocap_print_output_rate(VDOCAP_CONTEXT *p_ctx)
{
	UINT32 j, i;
	UINT32 idx;
	UINT32 shdr_seq;

	if (p_ctx == NULL)
		return;

	shdr_seq = _vdocap_shdr_map_to_seq(p_ctx->shdr_map);

	if (p_ctx->started == 0 || shdr_seq > 0)
		return;

	idx = p_ctx->out_dbg.idx;

	DBG_DUMP("\r\nvdocap%d output time dump (us)\r\n", p_ctx->id);
	for (j = 0; j < ISF_VDOCAP_OUT_NUM; j++) {
		for (i = 0; i < VDOCAP_DBG_TS_MAXNUM; i++) {
			UINT32 seq;
			UINT64 currtime, lasttime;

			seq = (idx + i)%VDOCAP_DBG_TS_MAXNUM;
			currtime = p_ctx->out_dbg.t[j][seq];
			if (seq == 0) {
				lasttime = p_ctx->out_dbg.t[j][VDOCAP_DBG_TS_MAXNUM-1];
			} else {
				lasttime = p_ctx->out_dbg.t[j][seq-1];
			}
			if (i == 0) {
				DBG_DUMP("%-10llu\r\n", currtime);
			} else {
				DBG_DUMP("%-10llu -> diff %llu us\r\n", currtime, (currtime-lasttime));
			}
		}
	}
}

static BOOL _isf_vdocap_api_rate(unsigned char argc, char **pargv)
{
	ER rt = E_OK;
	#if SHDR_QUEUE_DEBUG
	UINT32 i;
	VDOCAP_SHDR_OUT_QUEUE *p_outq;
	UINT32 new_total = 0;
	UINT32 release_total = 0;
	p_outq = &_vdocap_shdr_queue[0];
	for (i=0; i < SHDR_MAX_FRAME_NUM; i++) {
	DBG_DUMP("[%d]NEW ok/do/release=%5d,%5d,%5d, lock=%d,unlock=%3d,unlock_release=%3d  PUSH cb/do/colect/release/drop=%5d, %5d, %5d, %5d, %d\r\n", i
																					, p_outq->new_ok[i], p_outq->do_new[i], p_outq->new_release[i]
																					, p_outq->lock[i], p_outq->unlock[i], p_outq->unlock_release[i]
																					, p_outq->push[i]
																					, p_outq->do_push[i]
																					, p_outq->push_collect[i]
																					, p_outq->push_release[i]
																					, p_outq->push_drop[i]);
		new_total += p_outq->do_new[i];
		release_total += (p_outq->new_release[i]+p_outq->unlock_release[i]+p_outq->push_release[i]+p_outq->do_push[i]);
	}
	DBG_DUMP("do new total = %d, release total = %d\r\n", new_total, release_total);

	return (int)rt;
	#endif
	output_rate_update_pause = TRUE;
	_vdocap_print_output_rate((VDOCAP_CONTEXT *)isf_vdocap0.refdata);
	_vdocap_print_output_rate((VDOCAP_CONTEXT *)isf_vdocap1.refdata);
	_vdocap_print_output_rate((VDOCAP_CONTEXT *)isf_vdocap2.refdata);
	_vdocap_print_output_rate((VDOCAP_CONTEXT *)isf_vdocap3.refdata);
	_vdocap_print_output_rate((VDOCAP_CONTEXT *)isf_vdocap4.refdata);
#if defined(_BSP_NA51000_)
	_vdocap_print_output_rate((VDOCAP_CONTEXT *)isf_vdocap5.refdata);
	_vdocap_print_output_rate((VDOCAP_CONTEXT *)isf_vdocap6.refdata);
	_vdocap_print_output_rate((VDOCAP_CONTEXT *)isf_vdocap7.refdata);
#endif
	output_rate_update_pause = FALSE;
	return (int)rt;
}

static SXCMD_BEGIN(vdocap_cmd, "vcap")
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
SXCMD_ITEM("info",  	_isf_vdocap_api_info, 	"show info")
#endif
SXCMD_ITEM("debug",   _isf_vdocap_api_debug, 	"show flow-debug msg")
SXCMD_ITEM("trace",   _isf_vdocap_api_trace, 	"show flow-trace msg")
SXCMD_ITEM("probe",   _isf_vdocap_api_probe, 	"show data-probe msg")
SXCMD_ITEM("perf",    _isf_vdocap_api_perf,  	"show data-perf msg")
SXCMD_ITEM("save", 	_isf_vdocap_api_save,		"dump data to file")
SXCMD_ITEM("drv", 	_isf_vdocap_api_drv,		"dump detail driver info")
SXCMD_ITEM("rate", 	_isf_vdocap_api_rate,		"dump 1s interval info")
SXCMD_ITEM("reg", 	_isf_vdocap_api_reg,		"dump register")
//SXCMD_ITEM("dbg_lv", _isf_vdocap_api_dbg_lv,    "set debug level")
SXCMD_END()

int vdocap_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(vdocap_cmd);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "vcap");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", vdocap_cmd[loop].p_name, vdocap_cmd[loop].p_desc);
	}
	return 0;
}

#if defined(__LINUX)
int vdocap_cmd_execute(unsigned char argc, char **argv)
#else
MAINFUNC_ENTRY(vcap, argc, argv)
#endif
{
	UINT32 cmd_num = SXCMD_NUM(vdocap_cmd);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	//here, we follow standard c main() function, argv[0] should be a "program"
	//argv[0] = "vprc"
	//argv[1] = "(cmd)"
	if (strncmp(argv[1], "?", 2) == 0) {
		vdocap_cmd_showhelp(debug_msg);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], vdocap_cmd[loop].p_name, strlen(argv[1])) == 0) {
			ret = vdocap_cmd[loop].p_func(argc-1, &argv[1]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		vdocap_cmd_showhelp(debug_msg);
		return -1;
	}
	return 0;
}
