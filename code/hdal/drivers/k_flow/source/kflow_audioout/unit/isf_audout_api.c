#include "include/isf_audout_api.h"
#include "isf_audout_int.h"
#include "include/isf_audout_dbg.h"

#include "kwrap/sxcmd.h"
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#include "kwrap/cmdsys.h"

extern BOOL _isf_audout_api_info(unsigned char argc, char **pargv);
#endif

// cmd debug d0 o0 /mask=000
extern void _isf_flow_debug_port(char *unit_name, char *port_name, char *mask_name);
static BOOL _isf_audout_api_debug(unsigned char argc, char **pargv)
{
	char *dev = pargv[1];
	char *io = pargv[2];
	char *mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if ((argc < 2) || (dev[0] != 'd')) {
		_isf_flow_debug_port(0, 0, 0);
		return 1;
	}
	snprintf(unit_name, sizeof(unit_name), "audout%s", dev+1);
	//verify i/o
	if ((argc < 3) || ((io[0] != 'i') && (io[0] != 'o') && (io[0] != 'c'))) {
		_isf_flow_debug_port(0, 0, 0);
		return 1;
	}
	//do debug
	_isf_flow_debug_port(unit_name, io, mask);
	return 1;
}

// cmd trace d0 o0 /mask=000
extern void _isf_flow_trace_port(char *unit_name, char *port_name, char *mask_name);
static BOOL _isf_audout_api_trace(unsigned char argc, char **pargv)
{
	char *dev = pargv[1];
	char *io = pargv[2];
	char *mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if ((argc < 2) || (dev[0] != 'd')) {
		_isf_flow_trace_port(0, 0, 0);
		return 1;
	}
	snprintf(unit_name, sizeof(unit_name), "audout%s", dev+1);
	//verify i/o
	if ((argc < 3) || ((io[0] != 'i') && (io[0] != 'o') && (io[0] != 'c'))) {
		_isf_flow_trace_port(0, 0, 0);
		return 1;
	}
	//do trace
	_isf_flow_trace_port(unit_name, io, mask);
	return 1;
}

// cmd probe d0 o0 /mask=000
extern void _isf_flow_probe_port(char *unit_name, char *port_name, char *mask_name);
static BOOL _isf_audout_api_probe(unsigned char argc, char **pargv)
{
	char *dev = pargv[1];
	char *io = pargv[2];
	char *mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if ((argc < 2) || (dev[0] != 'd')) {
		_isf_flow_probe_port(0, 0, 0);
		return 1;
	}
	snprintf(unit_name, sizeof(unit_name), "audout%s", dev+1);
	//verify i/o
	if ((argc < 3) || ((io[0] != 'i') && (io[0] != 'o'))) {
		_isf_flow_probe_port(0, 0, 0);
		return 1;
	}
	//do probe
	_isf_flow_probe_port(unit_name, io, mask);
	return 1;
}

// cmd perf d0 o0
extern void _isf_flow_perf_port(char *unit_name, char *port_name);
static BOOL _isf_audout_api_perf(unsigned char argc, char **pargv)
{
	char *dev = pargv[1];
	char *io = pargv[2];
	char unit_name[16] = {0};
	//verify dev
	if ((argc < 2) || (dev[0] != 'd')) {
		_isf_flow_perf_port(0, 0);
		return 1;
	}
	snprintf(unit_name, sizeof(unit_name), "audout%s", dev+1);
	//verify i/o
	if ((argc < 3) || ((io[0] != 'i') && (io[0] != 'o'))) {
		_isf_flow_perf_port(0, 0);
		return 1;
	}
	//do perf
	_isf_flow_perf_port(unit_name, io);
	return 1;
}

// cmd save d0 o0 count
extern void _isf_flow_save_port(char *unit_name, char *port_name, char *count_name);
static BOOL _isf_audout_api_save(unsigned char argc, char **pargv)
{
	char *dev = pargv[1];
	char *io = pargv[2];
	char *count = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if ((argc < 2) || (dev[0] != 'd')) {
		_isf_flow_save_port(0, 0, 0);
		return 1;
	}
	snprintf(unit_name, sizeof(unit_name), "audout%s", dev+1);
	//verify i/o
	if ((argc < 3) || ((io[0] != 'i') && (io[0] != 'o'))) {
		_isf_flow_save_port(0, 0, 0);
		return 1;
	}
	//do save
	_isf_flow_save_port(unit_name, io, count);
	return 1;
}

static BOOL _isf_audout_dump_info(unsigned char argc, char **pargv)
{
	UINT32 entry_num = 0;

	entry_num = wavstudio_get_config(WAVSTUD_CFG_QUEUE_ENTRY_NUM, WAVSTUD_ACT_PLAY, 0);
	DBG_DUMP("ACT Play entry num  = %d\r\n", entry_num);

	entry_num = wavstudio_get_config(WAVSTUD_CFG_QUEUE_ENTRY_NUM, WAVSTUD_ACT_PLAY2, 0);
	DBG_DUMP("ACT Play2 entry num = %d\r\n", entry_num);

	return 1;
}

static SXCMD_BEGIN(audout_cmd, "aout")
SXCMD_ITEM("debug",   _isf_audout_api_debug, 	"show flow-debug msg")
SXCMD_ITEM("trace",   _isf_audout_api_trace, 	"show flow-trace msg")
SXCMD_ITEM("probe",   _isf_audout_api_probe, 	"show data-probe msg")
SXCMD_ITEM("perf",    _isf_audout_api_perf,  	"show data-perf msg")
SXCMD_ITEM("save", 	_isf_audout_api_save,		"dump data to file")
SXCMD_ITEM("dump", 	_isf_audout_dump_info,		"dump queue info")
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
SXCMD_ITEM("info", 	_isf_audout_api_info,		"dump detail driver info")
#endif
SXCMD_END()

int audout_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(audout_cmd);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "aout");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", audout_cmd[loop].p_name, audout_cmd[loop].p_desc);
	}
	return 0;
}

#if defined(__LINUX)
int audout_cmd_execute(unsigned char argc, char **argv)
#else
MAINFUNC_ENTRY(aout, argc, argv)
#endif
{
	UINT32 cmd_num = SXCMD_NUM(audout_cmd);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		audout_cmd_showhelp(debug_msg);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], audout_cmd[loop].p_name, strlen(argv[1])) == 0) {
			ret = audout_cmd[loop].p_func(argc-1, &argv[1]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		audout_cmd_showhelp(debug_msg);
		return -1;
	}
	return 0;
}


