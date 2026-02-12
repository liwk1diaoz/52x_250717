
// INCLUDE PROTECTED
#include "bsmux_init.h"

BOOL g_bsmux_debug_msg = BSMUX_DEBUG_MSG;

static BOOL Cmd_bsmu_openlog(unsigned char argc, char **argv)
{
	INT32 type = 1;

	if (argc < 1) {
		DBG_DUMP("wrong argument:%d\r\n", argc);
		return FALSE;
	}
	sscanf(argv[0], "%d", (int *)&type);

	switch (type) {
	case 1:
		bsmux_cb_dbg(1);
		break;
	case 2:
		bsmux_ctrl_mem_dbg(1);
		break;
	case 3:
		bsmux_ctrl_bs_dbg(1);
		break;
	case 4:
		bsmux_tsk_status_dbg(1);
		break;
	case 5:
		bsmux_tsk_action_dbg(1);
		break;
	case 6:
		bsmux_util_plugin_dbg(1);
		break;
	case 9:
		g_bsmux_debug_msg = 1;
		break;
	default:
		break;
	}

	return TRUE;
}

static BOOL Cmd_bsmu_closelog(unsigned char argc, char **argv)
{
	INT32 type = 1;

	if (argc < 1) {
		DBG_DUMP("wrong argument:%d\r\n", argc);
		return FALSE;
	}
	sscanf(argv[0], "%d", (int *)&type);

	switch (type) {
	case 1:
		bsmux_cb_dbg(0);
		break;
	case 2:
		bsmux_ctrl_mem_dbg(0);
		break;
	case 3:
		bsmux_ctrl_bs_dbg(0);
		break;
	case 4:
		bsmux_tsk_status_dbg(0);
		break;
	case 5:
		bsmux_tsk_action_dbg(0);
		break;
	case 6:
		bsmux_util_plugin_dbg(0);
		break;
	case 9:
		g_bsmux_debug_msg = 0;
		break;
	default:
		break;
	}

	return TRUE;
}

static BOOL Cmd_bsmu_set_time(unsigned char argc, char **argv)
{
	struct tm Curr_DateTime;
	struct tm Curr_DateTime2;
	int year, month, day;
	int hour, minute, second;

	if (argc < 6) {
		DBG_DUMP("wrong argument:%d\r\n", argc);
		return FALSE;
	}
	sscanf(argv[0], "%d", (int *)&year);
	sscanf(argv[1], "%d", (int *)&month);
	sscanf(argv[2], "%d", (int *)&day);
	sscanf(argv[3], "%d", (int *)&hour);
	sscanf(argv[4], "%d", (int *)&minute);
	sscanf(argv[5], "%d", (int *)&second);

	Curr_DateTime.tm_year = year;
	Curr_DateTime.tm_mon  = month;
	Curr_DateTime.tm_mday = day;
	Curr_DateTime.tm_hour = hour;
	Curr_DateTime.tm_min  = minute;
	Curr_DateTime.tm_sec  = second;
	Curr_DateTime.tm_wday = 0;//fix coverity
	Curr_DateTime.tm_yday = 0;//fix coverity
	Curr_DateTime.tm_isdst = 0; //fix coverity
	DBG_DUMP("set time = %d/%d/%d %d:%d:%d\r\n",
		Curr_DateTime.tm_year, Curr_DateTime.tm_mon, Curr_DateTime.tm_mday,
		Curr_DateTime.tm_hour, Curr_DateTime.tm_min, Curr_DateTime.tm_sec);
	hwclock_set_time(TIME_ID_CURRENT, Curr_DateTime, 0);

	Curr_DateTime2 = hwclock_get_time(TIME_ID_CURRENT);
	DBG_DUMP("get time = %d/%d/%d %d:%d:%d\r\n",
		Curr_DateTime2.tm_year, Curr_DateTime2.tm_mon, Curr_DateTime2.tm_mday,
		Curr_DateTime2.tm_hour, Curr_DateTime2.tm_min, Curr_DateTime2.tm_sec);
	return TRUE;
}

static BOOL Cmd_bsmu_get_time(unsigned char argc, char **argv)
{
	struct tm Curr_DateTime;
	Curr_DateTime = hwclock_get_time(TIME_ID_CURRENT);
	DBG_DUMP("get time = %d/%d/%d %d:%d:%d\r\n",
		Curr_DateTime.tm_year, Curr_DateTime.tm_mon, Curr_DateTime.tm_mday,
		Curr_DateTime.tm_hour, Curr_DateTime.tm_min, Curr_DateTime.tm_sec);
	return TRUE;
}

static BOOL Cmd_bsmu_set_cut_now(unsigned char argc, char **argv)
{
	UINT32 idx = 0;

	if (argc < 1) {
		DBG_DUMP("wrong argument:%d\r\n", argc);
		return FALSE;
	}
	sscanf(argv[0], "%d", (int *)&idx);

	if (0 != bsmux_ctrl_trig_cutnow(idx)) {
		DBG_DUMP("[%d] set cutnow failed\r\n", (int)idx);
	}

	return TRUE;
}

static BOOL Cmd_bsmu_MWP(unsigned char argc, char **argv)
{

	return TRUE;
}

static BOOL Cmd_bsmu_encrypto(unsigned char argc, char **argv)
{

	return TRUE;
}

static BOOL Cmd_bsmu_set_flush_freq(unsigned char argc, char **argv)
{
	UINT32 idx = 0, flush_freq = 0;

	if (argc < 2) {
		DBG_DUMP("wrong argument:%d\r\n", argc);
		return FALSE;
	}
	sscanf(argv[0], "%d", (int *)&idx);
	sscanf(argv[1], "%d", (int *)&flush_freq);

	bsmux_util_set_param(idx, BSMUXER_PARAM_WRINFO_FLUSH_FREQ, flush_freq);
	DBG_DUMP("set %d flush frequency 0x%X\r\n", idx, flush_freq);

	return TRUE;
}

static BOOL Cmd_bsmu_set_wrblk_size(unsigned char argc, char **argv)
{
	UINT32 idx = 0, wrblk_size = 0;

	if (argc < 2) {
		DBG_DUMP("wrong argument:%d\r\n", argc);
		return FALSE;
	}
	sscanf(argv[0], "%d", (int *)&idx);
	sscanf(argv[1], "%d", (int *)&wrblk_size);

	bsmux_util_set_param(idx, BSMUXER_PARAM_WRINFO_WRBLK_SIZE, wrblk_size);
	DBG_DUMP("set %d write block size 0x%X\r\n", idx, wrblk_size);

	return TRUE;
}

static BOOL Cmd_bsmu_get_calc_sec(unsigned char argc, char **argv)
{
	UINT32 sec;
	UINT32 rec_id;
	UINT32 frame_rate, target_bitrate, aud_rate, aud_ch;
	UINT64 remain_size;
	static HD_BSMUX_CALC_SEC movie_setting = {0};

	memset(&movie_setting, 0, sizeof(HD_BSMUX_CALC_SEC));
	remain_size = FileSys_GetDiskInfoEx('A', FST_INFO_FREE_SPACE);

	rec_id = 0;
	bsmux_util_get_param(rec_id, BSMUXER_PARAM_VID_VFR, &frame_rate);
	bsmux_util_get_param(rec_id, BSMUXER_PARAM_VID_TBR, &target_bitrate);
	bsmux_util_get_param(rec_id, BSMUXER_PARAM_AUD_SR, &aud_rate);
	bsmux_util_get_param(rec_id, BSMUXER_PARAM_AUD_CHS, &aud_ch);
	movie_setting.info[rec_id].vidfps = frame_rate;
	movie_setting.info[rec_id].vidTBR = target_bitrate;
	movie_setting.info[rec_id].audSampleRate = aud_rate;
	movie_setting.info[rec_id].audChs = aud_ch;
	movie_setting.info[rec_id].gpson = TRUE;
	movie_setting.info[rec_id].nidxon = TRUE;
	movie_setting.givenSpace = remain_size;

	sec = bsmux_util_calc_sec(&movie_setting);
	DBG_DUMP("GetCalcSec: %d\r\n", sec);

	return TRUE;
}

static BOOL Cmd_bsmu_set_gps_event(unsigned char argc, char **argv)
{
#if BSMUX_TEST_GPS_ON
	UINT32 string_addr = 0;
	UINT32 idx = 0;
	UINT i;

	sscanf(argv[0], "%d", (int *)&idx);

	// create mempool
	BSMUX_REC_MEMINFO Mem = {0};
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 size = (1 * 1024 - 16);
	UINT32 phy_addr;
	void *vrt_addr;
	HD_RESULT rv;
	rv = hd_common_mem_alloc("BsMux", &phy_addr, (void **)&vrt_addr, size, ddr_id);
	if (rv != HD_OK) {
		DBG_ERR("alloc fail size 0x%x, ddr %d\r\n", (unsigned int)(size), (int)ddr_id);
		return E_SYS;
	}
	Mem.addr = (UINT32)vrt_addr;
	Mem.size = size;

	// add gps
	bsmux_util_get_version(&string_addr);
	{
		char *pSrc = (char *)string_addr;
		char *pb = (char *)Mem.addr;
		for (i = 0; i < 0x10; i++) {
			*(pb + i) = *(pSrc + i);
		}
	}
	bsmux_util_add_gps(idx, Mem.addr, Mem.size);

	// release mempool
	rv = hd_common_mem_free(phy_addr, vrt_addr);
	if (rv != HD_OK) {
		DBG_ERR("release blk failed! (%d)\r\n", rv);
		return E_SYS;
	}

#endif
	return TRUE;
}

#if BSMUX_TEST_USER_ON
#define MOVAPP_COMP_MANU_STRING         "NOVATEK"
#define MOVAPP_COMP_NAME_STRING         "CARDV"
static UINT8 g_bsmu_user_data_buf[128] = {0};
static unsigned char g_bsmu_user_str[] = MOVAPP_COMP_MANU_STRING;
static unsigned char g_bsmu_comp_str[] = MOVAPP_COMP_NAME_STRING;
#endif
static BOOL Cmd_bsmu_set_user_data(unsigned char argc, char **argv)
{
	UINT32 userdataadr = 0, userdatasize = 0;
	UINT32 idx = 0;

	sscanf(argv[0], "%d", (int *)&idx);

#if BSMUX_TEST_USER_ON
	UINT32             buf;
	MOV_USER_MAKERINFO makeinfo;
	buf = (UINT32) g_bsmu_user_data_buf;
	makeinfo.ouputAddr = buf;
	makeinfo.makerLen = sizeof(g_bsmu_user_str);
	makeinfo.pMaker = g_bsmu_user_str;
	makeinfo.modelLen =  sizeof(g_bsmu_comp_str);
	makeinfo.pModel = g_bsmu_comp_str;
	userdataadr = (UINT32) g_bsmu_user_data_buf;
	userdatasize = MOVWrite_UserMakerModelData(&makeinfo);
#endif

	bsmux_util_set_param(idx, BSMUXER_PARAM_USERDATA_ON, TRUE);
	bsmux_util_set_param(idx, BSMUXER_PARAM_USERDATA_ADDR, userdataadr);
	bsmux_util_set_param(idx, BSMUXER_PARAM_USERDATA_SIZE, userdatasize);

	return TRUE;
}

static BOOL Cmd_bsmu_set_cust_data(unsigned char argc, char **argv)
{
	UINT32 custaddr = 0, custsize = 0;
	UINT32 idx = 0;

	sscanf(argv[0], "%d", (int *)&idx);

#if BSMUX_TEST_USER_ON
	UINT32             buf;
	MOV_USER_MAKERINFO makeinfo;
	buf = (UINT32) g_bsmu_user_data_buf;
	makeinfo.ouputAddr = buf;
	makeinfo.makerLen = sizeof(g_bsmu_user_str);
	makeinfo.pMaker = g_bsmu_user_str;
	makeinfo.modelLen =  sizeof(g_bsmu_comp_str);
	makeinfo.pModel = g_bsmu_comp_str;
	custaddr = (UINT32) g_bsmu_user_data_buf;
	custsize = MOVWrite_UserMakerModelData(&makeinfo);
#endif

	bsmux_util_set_param(idx, BSMUXER_PARAM_CUSTDATA_ADDR, custaddr);
	bsmux_util_set_param(idx, BSMUXER_PARAM_CUSTDATA_SIZE, custsize);
	bsmux_util_set_param(idx, BSMUXER_PARAM_CUSTDATA_TAG, 0x61626364);

	return TRUE;
}

static BOOL Cmd_bsmu_set_extend(unsigned char argc, char **argv)
{
	UINT32 idx = 0, is_on = 0, enable = 0;

	if (argc < 2) {
		DBG_DUMP("wrong argument:%d\r\n", argc);
		return FALSE;
	}
	sscanf(argv[0], "%d", (int *)&idx);
	sscanf(argv[1], "%d", (int *)&is_on);

	bsmux_util_get_param(idx, BSMUXER_PARAM_EXTINFO_ENABLE, &enable);
	if (is_on) {
		if (enable) {
			if (0 != bsmux_ctrl_extend(idx)) {
				DBG_DUMP("[%d] set extend failed\r\n", (int)idx);
			}
		} else {
			bsmux_util_set_param(idx, BSMUXER_PARAM_EXTINFO_ENABLE, 1);
		}
	} else {
		if (enable) {
			bsmux_util_set_param(idx, BSMUXER_PARAM_EXTINFO_ENABLE, 0);
		}
	}

	return TRUE;
}

static BOOL Cmd_bsmu_set_pause(unsigned char argc, char **argv)
{
	UINT32 idx = 0;

	sscanf(argv[0], "%d", (int *)&idx);

	if (0 != bsmux_ctrl_pause(idx)) {
		DBG_DUMP("[%d] set pause failed\r\n", (int)idx);
	}

	return TRUE;
}

static BOOL Cmd_bsmu_set_resume(unsigned char argc, char **argv)
{
	UINT32 idx = 0;

	sscanf(argv[0], "%d", (int *)&idx);

	if (0 != bsmux_ctrl_resume(idx)) {
		DBG_DUMP("[%d] set resume failed\r\n", (int)idx);
	}

	return TRUE;
}

static BOOL Cmd_bsmu_set_frontmoov(unsigned char argc, char **argv)
{
	UINT32 enable = 0, freq = 0, tune = 0;
	HD_PATH_ID bsmux_ctrl = 0;
	HD_BSMUX_EN_UTIL util = {0};
	HD_RESULT hd_ret;

	sscanf(argv[0], "%d", (int *)&enable);
	sscanf(argv[1], "%d", (int *)&freq);
	sscanf(argv[2], "%d", (int *)&tune);

	if ((hd_ret = hd_bsmux_open(0, HD_BSMUX_CTRL(0), &bsmux_ctrl)) != HD_OK) {
		DBG_ERR("hd_bsmux_open(HD_BSMUX_CTRL) fail(%d)\n", hd_ret);
	}

	util.type = HD_BSMUX_EN_UTIL_FRONTMOOV;
	util.enable = enable;
	if (freq) {
		util.resv[0] = freq;
	}
	if (tune) {
		util.resv[1] = 1; //auto-tune
	}
	if ((hd_ret = hd_bsmux_set(bsmux_ctrl, HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
		DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
	}

	if ((hd_ret = hd_bsmux_close(bsmux_ctrl)) != HD_OK) {
		DBG_ERR("hd_bsmux_close(HD_BSMUX_CTRL) fail(%d)\n", hd_ret);
	}

	return TRUE;
}

static BOOL Cmd_bsmu_trig_event(unsigned char argc, char **argv)
{
	UINT32 type = 0;
	HD_PATH_ID bsmux_ctrl = 0;
	HD_BSMUX_TRIG_EVENT event = {0};
	HD_RESULT hd_ret;

	sscanf(argv[0], "%d", (int *)&type);

	if ((hd_ret = hd_bsmux_open(0, HD_BSMUX_CTRL(0), &bsmux_ctrl)) != HD_OK) {
		DBG_ERR("hd_bsmux_open(HD_BSMUX_CTRL) fail(%d)\n", hd_ret);
	}

	event.type = type;
	if ((hd_ret = hd_bsmux_set(bsmux_ctrl, HD_BSMUX_PARAM_TRIG_EVENT, &event)) != HD_OK) {
		DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_TRIG_EVENT) fail(%d)\n", hd_ret);
	}

	if ((hd_ret = hd_bsmux_close(bsmux_ctrl)) != HD_OK) {
		DBG_ERR("hd_bsmux_close(HD_BSMUX_CTRL) fail(%d)\n", hd_ret);
	}

	return TRUE;
}

static BOOL Cmd_bsmu_enable_utility(unsigned char argc, char **argv)
{
	UINT32 type = 0, enable = 0, resv = 0;
	HD_PATH_ID bsmux_ctrl = 0;
	HD_BSMUX_EN_UTIL util = {0};
	HD_RESULT hd_ret;

	sscanf(argv[0], "%d", (int *)&type);
	sscanf(argv[1], "%d", (int *)&enable);
	sscanf(argv[2], "%d", (int *)&resv);

	if ((hd_ret = hd_bsmux_open(0, HD_BSMUX_CTRL(0), &bsmux_ctrl)) != HD_OK) {
		DBG_ERR("hd_bsmux_open(HD_BSMUX_CTRL) fail(%d)\n", hd_ret);
	}

	util.type = type;
	util.enable = enable;
	if (resv) {
		util.resv[0] = resv;
	}
	if ((hd_ret = hd_bsmux_set(bsmux_ctrl, HD_BSMUX_PARAM_EN_UTIL, &util)) != HD_OK) {
		DBG_ERR("hd_bsmux_set(HD_BSMUX_PARAM_EN_UTIL) fail(%d)\n", hd_ret);
	}

	if ((hd_ret = hd_bsmux_close(bsmux_ctrl)) != HD_OK) {
		DBG_ERR("hd_bsmux_close(HD_BSMUX_CTRL) fail(%d)\n", hd_ret);
	}

	return TRUE;
}

static BOOL Cmd_bsmu_get_utility(unsigned char argc, char **argv)
{
	HD_BSMUX_EN_UTIL utilinfo = {0};
	HD_BSMUX_EN_UTIL *p_utilinfo = &utilinfo;

	bsmux_util_get_param(0, BSMUXER_PARAM_EN_DROP, &p_utilinfo->enable);
	bsmux_util_get_param(0, BSMUXER_PARAM_VID_DROP, &p_utilinfo->resv[0]);
	bsmux_util_get_param(0, BSMUXER_PARAM_SUB_DROP, &p_utilinfo->resv[1]);

	DBG_DUMP("BSMUX_EN_UTIL: enable_drop %d main_drop %d sub_drop %d\r\n",
				p_utilinfo->enable, p_utilinfo->resv[0], p_utilinfo->resv[1]);

	return TRUE;
}

static BOOL Cmd_bsmu_dump_info(unsigned char argc, char **argv)
{
	UINT32 id;
	for (id = 0; id < BSMUX_MAX_CTRL_NUM; id++) {
		bsmux_util_dump_info(id);
	}
	return TRUE;
}

static SXCMD_BEGIN(bsmux_cmd_tbl, "bsmux_cmd_tbl")
SXCMD_ITEM("on",	Cmd_bsmu_openlog, 			"open log [type]")
SXCMD_ITEM("off",	Cmd_bsmu_closelog, 			"close log [type]")
SXCMD_ITEM("st",	Cmd_bsmu_set_time,			"set time")
SXCMD_ITEM("gt",	Cmd_bsmu_get_time,			"get time")
SXCMD_ITEM("cut",	Cmd_bsmu_set_cut_now,		"set cut now")
SXCMD_ITEM("mwp",	Cmd_bsmu_MWP,				"write protect")
SXCMD_ITEM("cryp",	Cmd_bsmu_encrypto,			"set encrypto")
SXCMD_ITEM("flush",	Cmd_bsmu_set_flush_freq,	"set flush frequency")
SXCMD_ITEM("blk",	Cmd_bsmu_set_wrblk_size,	"set write block size")
SXCMD_ITEM("gcs",	Cmd_bsmu_get_calc_sec,		"get calc sec")
SXCMD_ITEM("gps",	Cmd_bsmu_set_gps_event,		"set gps event [id]")
SXCMD_ITEM("user",	Cmd_bsmu_set_user_data,		"set user data [id]")
SXCMD_ITEM("cust",	Cmd_bsmu_set_cust_data,		"set cust data [id]")
SXCMD_ITEM("ext",	Cmd_bsmu_set_extend,		"set extend [id][on/off]")
SXCMD_ITEM("pe",	Cmd_bsmu_set_pause,			"set pause [id]")
SXCMD_ITEM("re",	Cmd_bsmu_set_resume,		"set resume [id]")
SXCMD_ITEM("fmoov",	Cmd_bsmu_set_frontmoov,		"set front moov [on/off]")
SXCMD_ITEM("event",	Cmd_bsmu_trig_event,		"trig event by ctrl path [event]")
SXCMD_ITEM("util",	Cmd_bsmu_enable_utility,	"enable util by ctrl path [util][on/off][resv]")
SXCMD_ITEM("getu",	Cmd_bsmu_get_utility,		"trig event by ctrl path [event]")
SXCMD_ITEM("dump",	Cmd_bsmu_dump_info,			"dump bsmu path info")
SXCMD_END()

static int bsmux_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(bsmux_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "bsmu (hd_bsmux_lib)");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", bsmux_cmd_tbl[loop].p_name, bsmux_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(bsmu, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(bsmux_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		bsmux_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], bsmux_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = bsmux_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	return 0;
}
