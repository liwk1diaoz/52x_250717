#include "kflow_videocapture/ctl_sen.h"
#include "ctl_sen_api.h"
#include "ctl_sen_dbg.h"
#include "sen_middle_int.h"
#include "kdrv_videocapture/kdrv_sie.h"
#include "sen_id_map_int.h"
#include "sen_int.h"
#include "kflow_common/nvtmpp.h"
#include <kwrap/cmdsys.h>
#include "kwrap/file.h"
#include "kwrap/mem.h"
#include "kwrap/util.h"
#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
#include "stdlib.h"
#else
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h> // header file Dma(cache handle)
#include <mach/fmem.h>
#include "ctl_sen_drv.h"
#endif


#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
#define CLT_SEN_REGVALUE                uint32_t
#define CLT_SEN_OUTW(addr,value)    (*(CLT_SEN_REGVALUE volatile *)(addr) = (CLT_SEN_REGVALUE)(value))
#define CLT_SEN_INW(addr)           (*(CLT_SEN_REGVALUE volatile *)(addr))
#define WRITE_ENG_REG(addr, value) CLT_SEN_OUTW(addr, value)//iowrite32(value, (void __iomem *)(0xFD000000 + (addr & 0xFFFFF)))
#define READ_ENG_REG(addr) CLT_SEN_INW(addr) //ioread32((void __iomem *)(addr | 0xFD000000))
#define WRITE_ENG_REG_BIT(addr, bit, lv) WRITE_ENG_REG(addr, (READ_ENG_REG(addr) & (~(0x1 << bit))) | ((BOOL)lv << bit))

#define simple_strtoul(param1, param2, param3) strtoul(param1, param2, param3)

#define EINVAL (1)

#else
#define WRITE_ENG_REG(addr, value) iowrite32(value, (void __iomem *)(0xFD000000 + (addr & 0xFFFFF)))
#define READ_ENG_REG(addr) ioread32((void __iomem *)(addr | 0xFD000000))
#define WRITE_ENG_REG_BIT(addr, bit, lv) WRITE_ENG_REG(addr, (READ_ENG_REG(addr) & (~(0x1 << bit))) | ((BOOL)lv << bit))
#endif

PCTL_SEN_OBJ p_sen_ctl_obj[CTL_SEN_ID_MAX] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

BOOL nvt_ctl_sen_api_w_reg(unsigned char argc, char **pargv)
{
	ER rt = E_OK;

	CTL_SEN_ID id = CTL_SEN_ID_1;
	CTL_SEN_CMD cmd = {0};

	/*[id][ui_addr][ui_data_len][ui_data[0]][ui_data[1]]*/
	if (argc != 5) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(pargv[0], NULL, 0);
	cmd.addr = simple_strtoul(pargv[1], NULL, 0);
	cmd.data_len = simple_strtoul(pargv[2], NULL, 0);
	cmd.data[0] = simple_strtoul(pargv[3], NULL, 0);
	cmd.data[1] = simple_strtoul(pargv[4], NULL, 0);

	CTL_SEN_DBG_WRN("start: id %d, addr 0x%.8x, len %d, data %d(0x%x) %d(0x%x)\r\n", id, cmd.addr, cmd.data_len, cmd.data[0], cmd.data[0], cmd.data[1], cmd.data[1]);

	p_sen_ctl_obj[id] = ctl_sen_get_object(id);

	if (p_sen_ctl_obj[id] == NULL) {
		nvt_dbg(ERR, "error id:%d", id);
		return E_NOEXS;
	}

	if (p_sen_ctl_obj[id]->write_reg == NULL) {
		nvt_dbg(ERR, "error id:%d", id);
		return E_NOEXS;
	}

	rt = p_sen_ctl_obj[id]->write_reg(&cmd);

	CTL_SEN_DBG_WRN("end\r\n");

	return (int)rt;
}

BOOL nvt_ctl_sen_api_dbg(unsigned char argc, char **pargv)
{
	ER rt = E_OK;
	CTL_SEN_ID id = CTL_SEN_ID_1;
	CTL_SEN_DBG_SEL dbg_sel;
	UINT32 param = 0;

	/*[id][CTL_SEN_DBG_SEL][param]*/
	if (argc < 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(pargv[0], NULL, 0);
	dbg_sel = simple_strtoul(pargv[1], NULL, 0);
	if (argc > 2) {
		param = simple_strtoul(pargv[2], NULL, 0);
	}

	CTL_SEN_DBG_IND("start: id %d dbg_sel 0x%.8x\r\n", id, dbg_sel);

	p_sen_ctl_obj[id] = ctl_sen_get_object(id);

	if (p_sen_ctl_obj[id] == NULL) {
		nvt_dbg(ERR, "error id:%d", id);
		return E_NOEXS;
	}

	if (p_sen_ctl_obj[id]->dbg_info == NULL) {
		nvt_dbg(ERR, "error id:%d", id);
		return E_NOEXS;
	}

	p_sen_ctl_obj[id]->dbg_info(dbg_sel, param);

	return (int)rt;
}

BOOL nvt_ctl_sen_api_r_reg(unsigned char argc, char **pargv)
{
	ER rt = E_OK;

	CTL_SEN_ID id = CTL_SEN_ID_1;
	CTL_SEN_CMD cmd = {0};

	/*[id][ui_addr][ui_data_len]*/
	if (argc != 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(pargv[0], NULL, 0);
	cmd.addr = simple_strtoul(pargv[1], NULL, 0);
	cmd.data_len = simple_strtoul(pargv[2], NULL, 0);

	CTL_SEN_DBG_WRN("start: id %d, addr 0x%.8x\r\n", id, cmd.addr);

	p_sen_ctl_obj[id] = ctl_sen_get_object(id);

	if (p_sen_ctl_obj[id] == NULL) {
		nvt_dbg(ERR, "error id:%d", id);
		return E_NOEXS;
	}

	if (p_sen_ctl_obj[id]->read_reg == NULL) {
		nvt_dbg(ERR, "error id:%d", id);
		return E_NOEXS;
	}

	rt = p_sen_ctl_obj[id]->read_reg(&cmd);

	CTL_SEN_DBG_WRN("end: data[0] 0x%x, data[1] 0x%x\r\n", cmd.data[0], cmd.data[1]);

	return (int)rt;
}

#if CTL_SEN_DBG_DET
BOOL nvt_ctl_sen_api_detect_plug_in(unsigned char argc, char **pargv)
{
	ER rt = E_OK;
	CTL_SEN_ID id = CTL_SEN_ID_1;
	BOOL det_rst = FALSE;
	UINT32 i, det_cnt = 1000;

	/*[id]*/
	if ((argc > 2) || (argc < 1)) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(pargv[0], NULL, 0);
	if (argc >= 2) {
		det_cnt = simple_strtoul(pargv[1], NULL, 0);
	}

	CTL_SEN_DBG_IND("start: id %d\r\n", id);

	p_sen_ctl_obj[id] = ctl_sen_get_object(id);

	if (p_sen_ctl_obj[id] == NULL) {
		nvt_dbg(ERR, "error id:%d", id);
		return E_NOEXS;
	}

	if (p_sen_ctl_obj[id]->get_cfg == NULL) {
		nvt_dbg(ERR, "error id:%d", id);
		return E_NOEXS;
	}

	for (i = 0; i < det_cnt; i++) {
		rt = p_sen_ctl_obj[id]->get_cfg(CTL_SEN_CFGID_GET_PLUG, (void *)(&det_rst));
		ctl_sen_delayms(500);
		if (det_rst == TRUE) {
			break;
		}
	}

	CTL_SEN_DBG_WRN("det_rst: %d\r\n", det_rst);

	return (int)rt;
}
#endif

BOOL nvt_ctl_sen_api_dump_op_msg(unsigned char argc, char **pargv)
{
	ER rt = E_OK;
	CTL_SEN_ID id = CTL_SEN_ID_1;
	BOOL simple = FALSE, en = FALSE;

	/*[id][en][skip_ae]*/
	if (argc < 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(pargv[0], NULL, 0);
	en = simple_strtoul(pargv[1], NULL, 0);
	if (argc > 2) {
		simple = simple_strtoul(pargv[2], NULL, 0);
	}

	CTL_SEN_DBG_WRN("id %d, en %d, simple %d\r\n", id, en, simple);

	p_sen_ctl_obj[id] = ctl_sen_get_object(id);

	if (p_sen_ctl_obj[id] == NULL) {
		nvt_dbg(ERR, "error id:%d", id);
		return E_NOEXS;
	}

	if (p_sen_ctl_obj[id]->dbg_info == NULL) {
		nvt_dbg(ERR, "error id:%d", id);
		return E_NOEXS;
	}

	if (simple) {
		p_sen_ctl_obj[id]->dbg_info(CTL_SEN_DBG_SEL_DBG_MSG, (UINT32)CTL_SEN_DBG_MSG_PARAM(en, CTL_SEN_MSG_TYPE_OP_SIMPLE));
	} else {
		p_sen_ctl_obj[id]->dbg_info(CTL_SEN_DBG_SEL_DBG_MSG, (UINT32)CTL_SEN_DBG_MSG_PARAM(en, CTL_SEN_MSG_TYPE_OP));
	}

	return (int)rt;
}

BOOL nvt_ctl_sen_api_set_dbg_lv(unsigned char argc, char **pargv)
{
	ER rt = E_OK;
	CTL_SEN_ID id = CTL_SEN_ID_1;
	CTL_SEN_DBG_LV dbg_lv;

	/*[CTL_SEN_DBG_LV]*/
	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	dbg_lv = simple_strtoul(pargv[0], NULL, 0);

	p_sen_ctl_obj[id] = ctl_sen_get_object(id);

	if (p_sen_ctl_obj[id] == NULL) {
		nvt_dbg(ERR, "error id:%d", id);
		return E_NOEXS;
	}

	if (p_sen_ctl_obj[id]->dbg_info == NULL) {
		nvt_dbg(ERR, "error id:%d", id);
		return E_NOEXS;
	}

	p_sen_ctl_obj[id]->dbg_info(CTL_SEN_DBG_SEL_LV, (UINT32)dbg_lv);
	CTL_SEN_DBG_WRN("dbg_lv %d\r\n", dbg_lv);

	return (int)rt;
}

#if 0
#endif

/*
    PROC CMD ENTRY
*/
static SXCMD_BEGIN(ctl_sen, CTL_SEN_PROC_NAME)
SXCMD_ITEM("dbg",           nvt_ctl_sen_api_dbg,            "input : (CTL_SEN_ID)(CTL_SEN_DBG_SEL)(param)")
SXCMD_ITEM("w_reg",         nvt_ctl_sen_api_w_reg,          "input : (CTL_SEN_ID)(addr)(data_len)(data0)(data1)")
SXCMD_ITEM("r_reg",         nvt_ctl_sen_api_r_reg,          "input : (CTL_SEN_ID)(addr)(data_len)")
SXCMD_ITEM("dump_op_msg",   nvt_ctl_sen_api_dump_op_msg,    "input : (CTL_SEN_ID)(en)(simple_en)")
SXCMD_ITEM("dbg_lv",        nvt_ctl_sen_api_set_dbg_lv,     "input : (CTL_SEN_DBG_LV)")
#if CTL_SEN_DBG_DET
SXCMD_ITEM("det",           nvt_ctl_sen_api_detect_plug_in, "input : (CTL_SEN_ID)")
#endif
SXCMD_END()

int ctl_sen_cmd_showhelp(void)
{
	UINT32 cmd_num = SXCMD_NUM(ctl_sen);
	UINT32 loop = 1;

	DBG_DUMP("---------------------------------------------------------------------\r\n");
	DBG_DUMP("  %s\n", CTL_SEN_PROC_NAME);
	DBG_DUMP("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		DBG_DUMP("%15s : %s\r\n", ctl_sen[loop].p_name, ctl_sen[loop].p_desc);
	}
	return 0;
}

#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
MAINFUNC_ENTRY(ctl_sen, argc, argv)
#else
int ctl_sen_cmd_execute(unsigned char argc, char **argv)
#endif
{
	UINT32 cmd_num = SXCMD_NUM(ctl_sen);
	UINT32 loop;
	int    ret;
	unsigned char ucargc = 0;

	//DBG_DUMP("%d, %s, %s, %s, %s\r\n", (int)argc, argv[0], argv[1], argv[2], argv[3]);

	if (strncmp(argv[1], "?", 2) == 0) {
		ctl_sen_cmd_showhelp();
		return -1;
	}

	if (argc < 1) {
		DBG_ERR("input param error\r\n");
		return -1;
	}
	ucargc = argc - 2;
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], ctl_sen[loop].p_name, strlen(argv[1])) == 0) {
			ret = ctl_sen[loop].p_func(ucargc, &argv[2]);
			return ret;
		}
	}

	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		ctl_sen_cmd_showhelp();
		return -1;
	}
	return 0;
}

