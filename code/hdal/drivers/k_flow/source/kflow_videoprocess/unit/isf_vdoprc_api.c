#include "isf_vdoprc_api.h"
#include "isf_vdoprc_int.h"
#include "isf_vdoprc_dbg.h"
#if defined(_BSP_NA51000_)
#define SXCMD_NEW
#include "kwrap/sxcmd.h"
#else
#include "kwrap/sxcmd.h"
#endif
#include "kwrap/debug.h"

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
extern BOOL _isf_vdoprc_api_info(unsigned char argc, char **pargv);
extern BOOL _isf_vdoprc_api_flow(unsigned char argc, char **pargv);
extern BOOL _isf_vdoprc_api_drv(unsigned char argc, char **pargv);
extern BOOL _isf_vdoprc_api_reg(unsigned char argc, char **pargv);
extern BOOL _isf_vdoprc_api_rate2(unsigned char argc, char **pargv);
#else
extern BOOL _isf_vdoprc_api_flow(unsigned char argc, char **pargv);
extern BOOL _isf_vdoprc_api_drv(unsigned char argc, char **pargv);
extern BOOL _isf_vdoprc_api_reg(unsigned char argc, char **pargv);
extern BOOL _isf_vdoprc_api_rate2(unsigned char argc, char **pargv);
#endif

// cmd debug d0 o0 /mask=000
extern void _isf_flow_debug_port(char* unit_name, char* port_name, char* mask_name);
static BOOL _isf_vdoprc_api_debug(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 2) || (dev[0]!='d')) { _isf_flow_debug_port(0,0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdoprc%s", dev+1); 
	//verify i/o
	if((argc < 3) || ((io[0]!='i') && (io[0]!='o') && (io[0]!='c'))) { _isf_flow_debug_port(0,0,0); return 1;}
	//do debug
	_isf_flow_debug_port(unit_name, io, mask);
	return 1;
}

// cmd trace d0 o0 /mask=000
extern void _isf_flow_trace_port(char* unit_name, char* port_name, char* mask_name);
static BOOL _isf_vdoprc_api_trace(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 2) || (dev[0]!='d')) { _isf_flow_trace_port(0,0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdoprc%s", dev+1); 
	//verify i/o
	if((argc < 3) || ((io[0]!='i') && (io[0]!='o') && (io[0]!='c'))) { _isf_flow_trace_port(0,0,0); return 1;}
	//do trace
	_isf_flow_trace_port(unit_name, io, mask);
	return 1;
}

// cmd probe d0 o0 /mask=000
extern void _isf_flow_probe_port(char* unit_name, char* port_name, char* mask_name);
static BOOL _isf_vdoprc_api_probe(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 2) || (dev[0]!='d')) { _isf_flow_probe_port(0,0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdoprc%s", dev+1); 
	//verify i/o
	if((argc < 3) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_probe_port(0,0,0); return 1;}
	//do probe
	_isf_flow_probe_port(unit_name, io, mask);
	return 1;
}

// cmd perf d0 o0
extern void _isf_flow_perf_port(char* unit_name, char* port_name);
static BOOL _isf_vdoprc_api_perf(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 2) || (dev[0]!='d')) { _isf_flow_perf_port(0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdoprc%s", dev+1); 
	//verify i/o
	if((argc < 3) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_perf_port(0,0); return 1;}
	//do perf
	_isf_flow_perf_port(unit_name, io);
	return 1;
}

// cmd save d0 o0 count
extern void _isf_flow_save_port(char* unit_name, char* port_name, char* count_name);
static BOOL _isf_vdoprc_api_save(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* count = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 2) || (dev[0]!='d')) { _isf_flow_save_port(0,0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdoprc%s", dev+1); 
	//verify i/o
	if((argc < 3) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_save_port(0,0,0); return 1;}
	//do save
	_isf_flow_save_port(unit_name, io, count);
	return 1;
}

static void _vdoprc_print_output_rate(ISF_UNIT *p_thisunit)
{
	UINT32 j, i;
	UINT32 idx;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

	if (p_ctx->cur_mode == 0)
		return;

	idx = p_ctx->outq.count_push_ok[0] % VDOPRC_DBG_TS_MAXNUM;

	for (j = 0; j < ISF_VDOPRC_OUT_NUM; j++) {
		if ((p_thisunit->port_outstate[j]->state) != (ISF_PORT_STATE_RUN))
			continue;
		DBG_DUMP("\r\nvdoprc[%d] output[%d] time dump (us)\r\n", p_ctx->dev, j);
		for (i = 0; i < VDOPRC_DBG_TS_MAXNUM; i++) {
			UINT32 seq;
			UINT64 currtime, lasttime;

			seq = (idx + i)%VDOPRC_DBG_TS_MAXNUM;
			currtime = p_ctx->out_dbg.t[j][seq];
			if (seq == 0) {
				lasttime = p_ctx->out_dbg.t[j][VDOPRC_DBG_TS_MAXNUM-1];
			} else {
				lasttime = p_ctx->out_dbg.t[j][seq-1];
			}
			if (i == 0) {
				DBG_DUMP("%-10llu\r\n", currtime);
			} else {
				DBG_DUMP("%-10llu -> diff %llu ms\r\n", currtime, (currtime-lasttime));
			}
		}
	}
}

static BOOL _isf_vdoprc_api_rate(unsigned char argc, char **pargv)
{
	UINT32 dev;
	if (!_isf_vdoprc_is_init()) {
		DBG_ERR("[vdoprc] not init\r\n");
		return 1;
	}
	DBG_DUMP("[vdoprc]\r\n");
	for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
		_vdoprc_print_output_rate(DEV_UNIT(dev));
	}

	_isf_vdoprc_api_rate2(argc, pargv);
	return 1;
}

static SXCMD_BEGIN(vdoprc_cmd, "vprc")
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
SXCMD_ITEM("info",  	_isf_vdoprc_api_info, 	"show info")
#endif
SXCMD_ITEM("debug",   _isf_vdoprc_api_debug, 	"show flow-debug msg")
SXCMD_ITEM("trace",   _isf_vdoprc_api_trace, 	"show flow-trace msg")
SXCMD_ITEM("probe",   _isf_vdoprc_api_probe, 	"show data-probe msg")
SXCMD_ITEM("perf",    _isf_vdoprc_api_perf,  	"show data-perf msg")
SXCMD_ITEM("save", 	_isf_vdoprc_api_save,		"dump data to file")
SXCMD_ITEM("flow", 	_isf_vdoprc_api_flow,		"dump detail kflow info")
SXCMD_ITEM("drv", 	_isf_vdoprc_api_drv,		"dump detail kdrv info")
SXCMD_ITEM("rate", 	_isf_vdoprc_api_rate,		"dump 1s interval info")
SXCMD_ITEM("reg", 	_isf_vdoprc_api_reg,		"dump register")
SXCMD_END()

int vdoprc_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(vdoprc_cmd);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "vprc");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", vdoprc_cmd[loop].p_name, vdoprc_cmd[loop].p_desc);
	}
	return 0;
}

#if defined(__LINUX)
int vdoprc_cmd_execute(unsigned char argc, char **argv)
#else
#include "kwrap/cmdsys.h" // for MAINFUNC_ENTRY() macro
MAINFUNC_ENTRY(vprc, argc, argv)
#endif
{
	UINT32 cmd_num = SXCMD_NUM(vdoprc_cmd);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	//here, we follow standard c main() function, argv[0] should be a "program"
	//argv[0] = "vprc"
	//argv[1] = "(cmd)"
	if (strncmp(argv[1], "?", 2) == 0) {
		vdoprc_cmd_showhelp(debug_msg);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], vdoprc_cmd[loop].p_name, strlen(argv[1])) == 0) {
			ret = vdoprc_cmd[loop].p_func(argc-1, &argv[1]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		vdoprc_cmd_showhelp(debug_msg);
		return -1;
	}
	return 0;
}

