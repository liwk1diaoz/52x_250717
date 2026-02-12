#include "isf_vdoout_api.h"
#include "isf_vdoout_int.h"
#include "isf_vdoout_dbg.h"
#include "kwrap/sxcmd.h"
#include "kwrap/debug.h"
#include "kwrap/cmdsys.h"
#include "vdodisp/vdodisp_sx_cmd.h"
#if !defined(__LINUX)
extern BOOL _isf_vdoout_api_info(unsigned char argc, char **pargv);
#endif
// cmd debug d0 o0 /mask=000
extern void _isf_flow_debug_port(char* unit_name, char* port_name, char* mask_name);
static BOOL _isf_vdoout_api_debug(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 1) || (dev[0]!='d')) { _isf_flow_debug_port(0,0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdoout%s", dev+1);
	//verify i/o
	if((argc < 2) || ((io[0]!='i') && (io[0]!='o') && (io[0]!='c'))) { _isf_flow_debug_port(0,0,0); return 1;}
	//do debug
	_isf_flow_debug_port(unit_name, io, mask);
	return 1;
}

// cmd trace d0 o0 /mask=000
extern void _isf_flow_trace_port(char* unit_name, char* port_name, char* mask_name);
static BOOL _isf_vdoout_api_trace(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 1) || (dev[0]!='d')) { _isf_flow_trace_port(0,0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdoout%s", dev+1);
	//verify i/o
	if((argc < 2) || ((io[0]!='i') && (io[0]!='o') && (io[0]!='c'))) { _isf_flow_trace_port(0,0,0); return 1;}
	//do trace
	_isf_flow_trace_port(unit_name, io, mask);
	return 1;
}

// cmd probe d0 o0 /mask=000
extern void _isf_flow_probe_port(char* unit_name, char* port_name, char* mask_name);
static BOOL _isf_vdoout_api_probe(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 1) || (dev[0]!='d')) { _isf_flow_probe_port(0,0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdoout%s", dev+1);
	//verify i/o
	if((argc < 2) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_probe_port(0,0,0); return 1;}
	//do probe
	_isf_flow_probe_port(unit_name, io, mask);
	return 1;
}

// cmd perf d0 o0
extern void _isf_flow_perf_port(char* unit_name, char* port_name);
static BOOL _isf_vdoout_api_perf(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 1) || (dev[0]!='d')) { _isf_flow_perf_port(0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdoout%s", dev+1);
	//verify i/o
	if((argc < 2) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_perf_port(0,0); return 1;}
	//do perf
	_isf_flow_perf_port(unit_name, io);
	return 1;
}

// cmd save d0 o0 count
extern void _isf_flow_save_port(char* unit_name, char* port_name, char* count_name);
static BOOL _isf_vdoout_api_save(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* count = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if((argc < 1) || (dev[0]!='d')) { _isf_flow_save_port(0,0,0); return 1;}
	snprintf(unit_name, sizeof(unit_name), "vdoout%s", dev+1);
	//verify i/o
	if((argc < 2) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_save_port(0,0,0); return 1;}
	//do save
	_isf_flow_save_port(unit_name, io, count);
	return 1;
}
#if 0

static void _vdoout_print_out_size(VDOOUT_CONTEXT *p_ctx)
{
}

static void _vdoout_print_output_rate(VDOOUT_CONTEXT *p_ctx)
{
}

int _isf_vdoout_api_dump_out_buf_size(PISF_VDOOUT_INFO pmodule_info, unsigned char argc, char **pargv)
{
	ER rt = E_OK;
	VDOOUT_CONTEXT *p_ctx;

	p_ctx = (VDOOUT_CONTEXT *)(isf_vdoout0.refdata);
	_vdoout_print_out_size(p_ctx);
	p_ctx = (VDOOUT_CONTEXT *)(isf_vdoout1.refdata);
	_vdoout_print_out_size(p_ctx);
	return (int)rt;
}

int _isf_vdoout_api_dump_output_rate(PISF_VDOOUT_INFO pmodule_info, unsigned char argc, char **pargv)
{
	ER rt = E_OK;
	VDOOUT_CONTEXT *p_ctx;

	p_ctx = (VDOOUT_CONTEXT *)(isf_vdoout0.refdata);
	_vdoout_print_output_rate(p_ctx);
	p_ctx = (VDOOUT_CONTEXT *)(isf_vdoout1.refdata);
	_vdoout_print_output_rate(p_ctx);

	return (int)rt;
}
#endif



static SXCMD_BEGIN(vdoout_cmd, "vout")
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
SXCMD_ITEM("info",  	_isf_vdoout_api_info, 	"show info")
#endif
SXCMD_ITEM("debug",   _isf_vdoout_api_debug, 	"show flow-debug msg")
SXCMD_ITEM("trace",   _isf_vdoout_api_trace, 	"show flow-trace msg")
SXCMD_ITEM("probe",   _isf_vdoout_api_probe, 	"show data-probe msg")
SXCMD_ITEM("perf",    _isf_vdoout_api_perf,  	"show data-perf msg")
SXCMD_ITEM("save", 	  _isf_vdoout_api_save,		"dump data to file")
SXCMD_ITEM("dump",    x_vdodisp_sx_cmd_dump,    "dump info")
SXCMD_ITEM("dumpbuf", x_vdodisp_sx_cmd_dump_buf, "dump current frame")
SXCMD_ITEM("fps",     x_vdodisp_sx_cmd_fps,     "calc fps")
SXCMD_ITEM("vd",      x_vdodisp_sx_cmd_vd,      "dump vd")

#if 0
SXCMD_ITEM("drv", 	_isf_vdoout_api_drv,		"dump detail driver info")
SXCMD_ITEM("rate", 	_isf_vdoout_api_rate,		"dump 1s interval info")
SXCMD_ITEM("reg", 	_isf_vdoout_api_reg,		"dump register")
#endif
SXCMD_END()

int vdoout_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(vdoout_cmd);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "vout");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", vdoout_cmd[loop].p_name, vdoout_cmd[loop].p_desc);
	}
	return 0;
}

#if defined(__LINUX)
int vdoout_cmd_execute(unsigned char argc, char **argv)
#else
MAINFUNC_ENTRY(vout, argc, argv)
#endif
{
	UINT32 cmd_num = SXCMD_NUM(vdoout_cmd);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	//here, we follow standard c main() function, argv[0] should be a "program"
	//argv[0] = "vprc"
	//argv[1] = "(cmd)"
	if (strncmp(argv[1], "?", 2) == 0) {
		vdoout_cmd_showhelp(debug_msg);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], vdoout_cmd[loop].p_name, strlen(argv[1])) == 0) {
			ret = vdoout_cmd[loop].p_func(argc-1, &argv[1]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		vdoout_cmd_showhelp(debug_msg);
		return -1;
	}
	return 0;
}

