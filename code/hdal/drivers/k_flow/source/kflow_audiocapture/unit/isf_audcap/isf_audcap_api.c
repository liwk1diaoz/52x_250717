#include "../include/isf_audcap_api.h"
#include "isf_audcap_int.h"
#include "../include/isf_audcap_dbg.h"

#include "kwrap/sxcmd.h"
#ifdef __KERNEL__
#include "kwrap/file.h"
#endif
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#include "kwrap/cmdsys.h"

extern BOOL _isf_audcap_api_info(unsigned char argc, char **pargv);
#endif

// cmd debug d0 o0 /mask=000
extern void _isf_flow_debug_port(char *unit_name, char *port_name, char *mask_name);
static BOOL _isf_audcap_api_debug(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if ((argc < 2) || (dev[0] != 'd')) {
		_isf_flow_debug_port(0, 0, 0);
		return 1;
	}
	snprintf(unit_name, sizeof(unit_name), "audcap");
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
static BOOL _isf_audcap_api_trace(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if ((argc < 2) || (dev[0] != 'd')) {
		_isf_flow_trace_port(0, 0, 0);
		return 1;
	}
	snprintf(unit_name, sizeof(unit_name), "audcap");
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
static BOOL _isf_audcap_api_probe(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* mask = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if ((argc < 2) || (dev[0] != 'd')) {
		_isf_flow_probe_port(0, 0, 0);
		return 1;
	}
	snprintf(unit_name, sizeof(unit_name), "audcap");
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
static BOOL _isf_audcap_api_perf(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char unit_name[16] = {0};
	//verify dev
	if ((argc < 2) || (dev[0] != 'd')) {
		_isf_flow_perf_port(0, 0);
		return 1;
	}
	snprintf(unit_name, sizeof(unit_name), "audcap");
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
static BOOL _isf_audcap_api_save(unsigned char argc, char **pargv)
{
	char* dev = pargv[1];
	char* io = pargv[2];
	char* count = pargv[3];
	char unit_name[16] = {0};
	//verify dev
	if ((argc < 2) || (dev[0] != 'd')) {
		_isf_flow_save_port(0, 0, 0);
		return 1;
	}
	snprintf(unit_name, sizeof(unit_name), "audcap");
	//verify i/o
	if ((argc < 3) || ((io[0] != 'i') && (io[0] != 'o'))) {
		_isf_flow_save_port(0, 0, 0);
		return 1;
	}
	//do save
	_isf_flow_save_port(unit_name, io, count);
	return 1;
}

#ifdef __KERNEL__
static BOOL _isf_audcap_api_dump_fastboot(unsigned char argc, char **pargv)
{
	int              fd, len;
	char             tmp_buf[128]="/mnt/sd/nvt-na51055-fastboot-acap.dtsi";
	WAVSTUD_INFO_SET info = {0};

	isf_audcap_get_audinfo(&info);

	sscanf(pargv[1], "%s", tmp_buf);

	fd = vos_file_open(tmp_buf, O_CREAT|O_WRONLY|O_SYNC, 0);
	if ((VOS_FILE)(-1) == fd) {
		printk("open %s failure\r\n", tmp_buf);
		return FALSE;
	}
	len = snprintf(tmp_buf, sizeof(tmp_buf), "&fastboot {\r\n\tacap {\r\n");
	vos_file_write(fd, (void *)tmp_buf, len);


	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tsamplerate = <%d>;\r\n", info.aud_info.aud_sr);
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tchannel = <%d>;\r\n", info.aud_info.aud_ch);
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tbufnum = <%d>;\r\n", info.buf_count);
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tbufsamplecnt = <%d>;\r\n", info.aud_info.buf_sample_cnt);
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tvol = <%d>;\r\n", isf_audcap_get_vol(0));
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\trecsrc = <%d>;\r\n", isf_audcap_get_recsrc());
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\taec = <%d>;\r\n", wavstudio_get_config(WAVSTUD_CFG_AEC_EN, 0, 0));
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\ttxchannel = <%d>;\r\n", wavstudio_get_config(WAVSTUD_CFG_AUD_CH, 0, 0)); //WAVSTUD_ACT_PLAY
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t\tdefaultsetting = <%d>;\r\n", wavstudio_get_config(WAVSTUD_CFG_DEFAULT_SETTING, 0, 0));
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "\t};\r\n};");
	vos_file_write(fd, (void *)tmp_buf, len);

	vos_file_close(fd);

	return TRUE;
}
#endif

static BOOL _isf_audcap_api_otrace(unsigned char argc, char **pargv)
{
	_isf_audcap_sxcmd_otrace("");

	return TRUE;
}

static BOOL _isf_audcap_api_ctrace(unsigned char argc, char **pargv)
{
	_isf_audcap_sxcmd_ctrace("");

	return TRUE;
}

static BOOL _isf_audcap_api_dumpque(unsigned char argc, char **pargv)
{
	_isf_audcap_sxcmd_dumpque("");

	return TRUE;
}

static BOOL _isf_audcap_api_dumpquebuf(unsigned char argc, char **pargv)
{
	_isf_audcap_sxcmd_dumpbuf("");

	return TRUE;
}

static BOOL _isf_audcap_api_dumpque_num(unsigned char argc, char **pargv)
{
	_isf_audcap_sxcmd_dump_que_num("");

	return TRUE;
}

static BOOL _isf_audcap_api_dump_info(unsigned char argc, char **pargv)
{
	int act = WAVSTUD_ACT_REC;

	if (argc == 2) {
		sscanf(pargv[1], "%d", &act);
	}

	wavstudio_dump_module_info(act);

	return TRUE;
}

static SXCMD_BEGIN(audcap_cmd, "acap")
SXCMD_ITEM("debug",   _isf_audcap_api_debug, 	"show flow-debug msg")
SXCMD_ITEM("trace",   _isf_audcap_api_trace, 	"show flow-trace msg")
SXCMD_ITEM("probe",   _isf_audcap_api_probe, 	"show data-probe msg")
SXCMD_ITEM("perf",    _isf_audcap_api_perf,  	"show data-perf msg")
SXCMD_ITEM("save", 	_isf_audcap_api_save,		"dump data to file")
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
SXCMD_ITEM("info", 	_isf_audcap_api_info,		"dump detail driver info")
#endif
#ifdef __KERNEL__
SXCMD_ITEM("dump_fastboot", _isf_audcap_api_dump_fastboot, "set fast boot info dump")
#endif
SXCMD_ITEM("otrace", _isf_audcap_api_otrace, "open trace message")
SXCMD_ITEM("ctrace", _isf_audcap_api_ctrace, "close trace message")
SXCMD_ITEM("dump_q", _isf_audcap_api_dumpque, "dump queue")
SXCMD_ITEM("dump_qb", _isf_audcap_api_dumpquebuf, "dump queue buffer")
SXCMD_ITEM("dump_qn", _isf_audcap_api_dumpque_num, "dump queue num")
SXCMD_ITEM("dump_info", _isf_audcap_api_dump_info, "dump queue num")
SXCMD_END()

int audcap_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(audcap_cmd);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "acap");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", audcap_cmd[loop].p_name, audcap_cmd[loop].p_desc);
	}
	return 0;
}

#if defined(__LINUX)
int audcap_cmd_execute(unsigned char argc, char **argv)
#else
MAINFUNC_ENTRY(acap, argc, argv)
#endif
{
	UINT32 cmd_num = SXCMD_NUM(audcap_cmd);
	UINT32 loop;
	int    ret;

	if (argc < 1) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		audcap_cmd_showhelp(debug_msg);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], audcap_cmd[loop].p_name, strlen(argv[1])) == 0) {
			ret = audcap_cmd[loop].p_func(argc-1, &argv[1]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		audcap_cmd_showhelp(debug_msg);
		return -1;
	}
	return 0;
}

