#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include "kwrap/type.h"

#include "sde_api.h"
//#include "sde_dev.h"

#include "sde_alg.h"
#include "sdet_api.h"

#include "sde_api_int.h"
#include "sdet_api_int.h"
#include "sde_dbg.h"
#include "sde_dev_int.h"
#include "sde_main.h"
#include "sde_proc.h"
#include "sde_version.h"

//=============================================================================
// avoid GPL api
//=============================================================================
#define MUTEX_ENABLE DISABLE

//=============================================================================
// global
//=============================================================================
#if MUTEX_ENABLE
static struct semaphore mutex;
#endif
static struct proc_dir_entry *proc_root;
static struct proc_dir_entry *proc_info;
static struct proc_dir_entry *proc_command;
static struct proc_dir_entry *proc_help;

static SDE_PROC_MSG_BUF sde_proc_cmd_msg;
static SDE_PROC_R_ITEM sde_proc_r_item = SDE_PROC_R_ITEM_NONE;
static SDE_ID sde_proc_id;

// NOTE: temporal
extern SDET_CFG_INFO cfg_data;
extern SDE_IF_PARAM_PTR *sde_if_param[SDE_ID_MAX_NUM];

//=============================================================================
// routines
//=============================================================================
static inline SDE_MODULE *sde_proc_get_mudule_from_file(struct file *file)
{
	return (SDE_MODULE *)((struct seq_file *)file->private_data)->private;
}

//=============================================================================
// proc "info" file operation functions
//=============================================================================
static INT32 sde_proc_info_show(struct seq_file *sfile, void *v)
{
	UINT32 version = sde_get_version();

	seq_printf(sfile, "---------------------------------------------------------------------------------------------- \r\n");
	seq_printf(sfile, "NVT_SDE v%d.%d.%d.%d \r\n", (version >> 24) & 0xFF, (version >> 16) & 0xFF, (version >> 8) & 0xFF, version & 0xFF);
	seq_printf(sfile, "---------------------------------------------------------------------------------------------- \r\n");

	return 0;
}

static INT32 sde_proc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, sde_proc_info_show, PDE_DATA(inode));
}

static const struct file_operations sde_proc_info_fops = {
	.owner   = THIS_MODULE,
	.open    = sde_proc_info_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "cmd" file operation functions
//=============================================================================
static inline INT32 sde_proc_msg_buf_alloc(void)
{
	sde_proc_cmd_msg.buf = kzalloc(PROC_MSG_BUFSIZE, GFP_KERNEL);

	if (sde_proc_cmd_msg.buf == NULL) {
		DBG_WRN("fail to allocate SDE message buffer!! \r\n");
		return -ENOMEM;
	}

	sde_proc_cmd_msg.size = PROC_MSG_BUFSIZE;
	sde_proc_cmd_msg.count = 0;
	return 0;
}

static inline void sde_proc_msg_buf_free(void)
{
	if (sde_proc_cmd_msg.buf) {
		kfree(sde_proc_cmd_msg.buf);
		sde_proc_cmd_msg.buf = NULL;
	}
}

static inline void sde_proc_msg_buf_clean(void)
{
	sde_proc_cmd_msg.buf[0] = '\0';
	sde_proc_cmd_msg.count = 0;
}

static INT32 sde_proc_cmd_printf(const s8 *f, ...)
{
	INT32 len;
	va_list args;

	if (sde_proc_cmd_msg.count < sde_proc_cmd_msg.size) {
		va_start(args, f);
		len = vsnprintf(sde_proc_cmd_msg.buf + sde_proc_cmd_msg.count, sde_proc_cmd_msg.size - sde_proc_cmd_msg.count, f, args);
		va_end(args);

		if ((sde_proc_cmd_msg.count + len) < sde_proc_cmd_msg.size) {
			sde_proc_cmd_msg.count += len;
			return 0;
		}
	}

	sde_proc_cmd_msg.count = sde_proc_cmd_msg.size;
	return -1;
}

static INT32 sde_proc_cmd_get_buffer_size(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	sde_proc_r_item = SDE_PROC_R_ITEM_BUFFER;
	return 0;
}

static INT32 sde_proc_cmd_get_param(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	sde_proc_id = simple_strtoul(argv[2], NULL, 0);

	if (sde_dev_get_id_valid(sde_proc_id) && (sde_proc_id < SDE_ID_MAX_NUM)) {
		sde_proc_r_item = SDE_PROC_R_ITEM_PARAM;
	} else {
		sde_proc_r_item = SDE_PROC_R_ITEM_NONE;
	}

	return 0;
}

static INT32 sde_proc_cmd_get_cfg_data(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 3) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	sde_proc_id = simple_strtoul(argv[2], NULL, 0);

	// TODO:
	#if 0
	if (sde_flow_get_id_valid(sde_proc_id) && (sde_proc_id < SDE_ID_MAX_NUM)) {
		sde_proc_r_item = SDE_PROC_R_ITEM_CFG_DATA;
	} else {
		sde_proc_r_item = SDE_PROC_R_ITEM_NONE;
	}
	#else
	sde_proc_r_item = SDE_PROC_R_ITEM_PARAM;
	#endif

	return 0;
}

static SDE_PROC_CMD sde_proc_cmd_r_list[] = {
	// keyword              function name
	{ "buffer_size",        sde_proc_cmd_get_buffer_size,    "get sde buffer size"},
	{ "param",              sde_proc_cmd_get_param,          "get sde param, param1 is sde_id(0~1)"},
	{ "cfg_data",           sde_proc_cmd_get_cfg_data,       "get sde cfg file name, param1 is sde_id(0~1)"},
};
#define NUM_OF_R_CMD (sizeof(sde_proc_cmd_r_list) / sizeof(SDE_PROC_CMD))

static INT32 sde_proc_cmd_open(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	SDE_OPENOBJ sde_drv_obj;
	ER rt = E_OK;

	if (argc < 2) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	if (sde_opened) {
		return 0;
	}

	sde_drv_obj.FP_SDEISR_CB = NULL;
	sde_drv_obj.uiSdeClockSel = 480;

	sde_proc_cmd_printf("sde_proc_cmd_open \n");
	rt = sde_open(&sde_drv_obj);
	if (rt == E_OK) {
		sde_opened = TRUE;
	}
	return 0;
}

static INT32 sde_proc_cmd_close(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	ER rt = E_OK;

	if (argc < 2) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	if (sde_opened == FALSE) {
		return 0;
	}

	sde_proc_cmd_printf("sde_proc_cmd_close \n");
	rt = sde_close();
	if (rt == E_OK) {
		sde_opened = FALSE;
	}
	return 0;
}

static INT32 sde_proc_cmd_trig(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;

	if (argc < 3) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	sde_proc_cmd_printf("sde_proc_cmd_trig (%d)!! \n", id);
	sde_trigger(id);
	return 0;
}

static INT32 sde_proc_cmd_set_dbg(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 cmd;

	if (argc < 4) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	cmd = simple_strtoul(argv[3], NULL, 0);

	sde_proc_cmd_printf("set sde(%d) dbg level(0x%x) \n", id, cmd);
	sde_dbg_set_dbg_mode(id, cmd);
	return 0;
}

static INT32 sde_proc_cmd_set_ext_en(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 scan_en;
	u32 diag_en;
	u32 input2_en;
	u32 cond_disp_en;
	u32 ui_intr_en;

	if (argc < 8) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	scan_en = simple_strtoul(argv[3], NULL, 0);
	diag_en = simple_strtoul(argv[4], NULL, 0);
	input2_en = simple_strtoul(argv[5], NULL, 0);
	cond_disp_en = simple_strtoul(argv[6], NULL, 0);
	ui_intr_en = simple_strtoul(argv[7], NULL, 0);

	sde_drv_param[id].bScanEn = scan_en;
	sde_drv_param[id].bDiagEn = diag_en;
	sde_drv_param[id].bInput2En = input2_en;
	sde_drv_param[id].cond_disp_en = cond_disp_en;
	sde_drv_param[id].uiIntrEn = ui_intr_en;
	sde_proc_cmd_printf("set sde_drv_param(%d) bScanEn = %d\r\n", id, scan_en);
	sde_proc_cmd_printf("set sde_drv_param(%d) bDiagEn = %d\r\n", id, diag_en);
	sde_proc_cmd_printf("set sde_drv_param(%d) bInput2En = %d\r\n", id, input2_en);
	sde_proc_cmd_printf("set sde_drv_param(%d) cond_disp_en = %d\r\n", id, cond_disp_en);
	sde_proc_cmd_printf("set sde_drv_param(%d) uiIntrEn = %d\r\n", id, ui_intr_en);

	return 0;
}

static INT32 sde_proc_cmd_set_ext_mode(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 cost_comp_mode;
	u32 conf_out2_mode;

	if (argc < 5) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	cost_comp_mode = simple_strtoul(argv[3], NULL, 0);
	conf_out2_mode = simple_strtoul(argv[4], NULL, 0);

	sde_drv_param[id].bCostCompMode = cost_comp_mode;
	sde_drv_param[id].conf_out2_mode = cost_comp_mode;
	sde_proc_cmd_printf("set sde_drv_param(%d) bCostCompMode = %d\r\n", id, cost_comp_mode);
	sde_proc_cmd_printf("set sde_drv_param(%d) conf_out2_mode = %d\r\n", id, conf_out2_mode);

	return 0;
}

static INT32 sde_proc_cmd_set_ext_sel(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 out_sel;
	u32 out_vol_sel;
	u32 conf_min2_sel;
	u32 burst_sel;

	if (argc < 7) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	out_sel = simple_strtoul(argv[3], NULL, 0);
	out_vol_sel = simple_strtoul(argv[4], NULL, 0);
	conf_min2_sel = simple_strtoul(argv[5], NULL, 0);
	burst_sel = simple_strtoul(argv[6], NULL, 0);

	sde_drv_param[id].outSel = out_sel;
	sde_drv_param[id].outVolSel = out_vol_sel;
	sde_drv_param[id].conf_min2_sel = conf_min2_sel;
	sde_drv_param[id].burstSel = burst_sel;
	sde_proc_cmd_printf("set sde_drv_param(%d) outSel = %d\r\n", id, out_sel);
	sde_proc_cmd_printf("set sde_drv_param(%d) outVolSel = %d\r\n", id, out_vol_sel);
	sde_proc_cmd_printf("set sde_drv_param(%d) conf_min2_sel = %d\r\n", id, conf_min2_sel);
	sde_proc_cmd_printf("set sde_drv_param(%d) burstSel = %d\r\n", id, burst_sel);

	return 0;
}

static INT32 sde_proc_cmd_set_ext_flip(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 h_flip_01;
	u32 v_flip_01;
	u32 h_flip_2;
	u32 v_flip_2;

	if (argc < 7) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	h_flip_01 = simple_strtoul(argv[3], NULL, 0);
	v_flip_01 = simple_strtoul(argv[4], NULL, 0);
	h_flip_2 = simple_strtoul(argv[5], NULL, 0);
	v_flip_2 = simple_strtoul(argv[6], NULL, 0);

	sde_drv_param[id].bHflip01 = h_flip_01;
	sde_drv_param[id].bVflip01 = v_flip_01;
	sde_drv_param[id].bHflip2 = h_flip_2;
	sde_drv_param[id].bVflip2 = v_flip_2;
	sde_proc_cmd_printf("set sde_drv_param(%d) bHflip01 = %d\r\n", id, h_flip_01);
	sde_proc_cmd_printf("set sde_drv_param(%d) bVflip01 = %d\r\n", id, v_flip_01);
	sde_proc_cmd_printf("set sde_drv_param(%d) bHflip2 = %d\r\n", id, h_flip_2);
	sde_proc_cmd_printf("set sde_drv_param(%d) bVflip2 = %d\r\n", id, v_flip_2);

	return 0;
}

static INT32 sde_proc_cmd_set_ctrl(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 disp_val_mode;
	u32 disp_op_mode;
	u32 disp_inv_sel;
	u32 conf_en;

	if (argc < 7) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	disp_val_mode = simple_strtoul(argv[3], NULL, 0);
	disp_op_mode = simple_strtoul(argv[4], NULL, 0);
	disp_inv_sel = simple_strtoul(argv[5], NULL, 0);
	conf_en = simple_strtoul(argv[6], NULL, 0);

	sde_if_param[id]->ctrl->disp_val_mode = disp_val_mode;
	sde_if_param[id]->ctrl->disp_op_mode = disp_op_mode;
	sde_if_param[id]->ctrl->disp_inv_sel = disp_inv_sel;
	sde_if_param[id]->ctrl->conf_en = conf_en;
	sde_proc_cmd_printf("set sde_if_param(%d) ctrl.disp_val_mode = %d\r\n", id, disp_val_mode);
	sde_proc_cmd_printf("set sde_if_param(%d) ctrl.disp_op_mode = %d\r\n", id, disp_op_mode);
	sde_proc_cmd_printf("set sde_if_param(%d) ctrl.disp_inv_sel = %d\r\n", id, disp_inv_sel);
	sde_proc_cmd_printf("set sde_if_param(%d) ctrl.conf_en = %d\r\n", id, conf_en);

	return 0;
}

static INT32 sde_proc_cmd_set_cost(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 ad_bound_upper;
	u32 ad_bound_lower;
	u32 census_bound_upper;
	u32 census_bound_lower;
	u32 ad_census_ratio;

	if (argc < 8) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	ad_bound_upper = simple_strtoul(argv[3], NULL, 0);
	ad_bound_lower = simple_strtoul(argv[4], NULL, 0);
	census_bound_upper = simple_strtoul(argv[5], NULL, 0);
	census_bound_lower = simple_strtoul(argv[6], NULL, 0);
	ad_census_ratio = simple_strtoul(argv[7], NULL, 0);

	sde_drv_param[id].costPara.uiAdBoundUpper = ad_bound_upper;
	sde_drv_param[id].costPara.uiAdBoundLower = ad_bound_lower;
	sde_drv_param[id].costPara.uiCensusBoundUpper = census_bound_upper;
	sde_drv_param[id].costPara.uiCensusBoundLower = census_bound_lower;
	sde_drv_param[id].costPara.uiCensusAdRatio = ad_census_ratio;
	sde_proc_cmd_printf("set sde_drv_param(%d) costPara.uiAdBoundUpper = %d\r\n", id, ad_bound_upper);
	sde_proc_cmd_printf("set sde_drv_param(%d) costPara.uiAdBoundLower = %d\r\n", id, ad_bound_lower);
	sde_proc_cmd_printf("set sde_drv_param(%d) costPara.uiCensusBoundUpper = %d\r\n", id, census_bound_upper);
	sde_proc_cmd_printf("set sde_drv_param(%d) costPara.uiCensusBoundLower = %d\r\n", id, census_bound_lower);
	sde_proc_cmd_printf("set sde_drv_param(%d) costPara.uiCensusAdRatio = %d\r\n", id, ad_census_ratio);

	return 0;
}

static INT32 sde_proc_cmd_set_luma(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 luma_saturated_th;
	u32 cost_saturated;
	u32 delta_luma_smooth_th;

	if (argc < 6) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	luma_saturated_th = simple_strtoul(argv[3], NULL, 0);
	cost_saturated = simple_strtoul(argv[4], NULL, 0);
	delta_luma_smooth_th = simple_strtoul(argv[5], NULL, 0);

	sde_drv_param[id].lumaPara.uiLumaThreshSaturated = luma_saturated_th;
	sde_drv_param[id].lumaPara.uiCostSaturatedMatch = cost_saturated;
	sde_drv_param[id].lumaPara.uiDeltaLumaSmoothThresh = delta_luma_smooth_th;
	sde_proc_cmd_printf("set sde_drv_param(%d) lumaPara.uiLumaThreshSaturated = %d\r\n", id, luma_saturated_th);
	sde_proc_cmd_printf("set sde_drv_param(%d) lumaPara.uiCostSaturatedMatch = %d\r\n", id, cost_saturated);
	sde_proc_cmd_printf("set sde_drv_param(%d) lumaPara.uiDeltaLumaSmoothThresh = %d\r\n", id, delta_luma_smooth_th);

	return 0;
}

static INT32 sde_proc_cmd_set_penalty(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 penalty_nonsmooth;
	u32 penalty_smooth0;
	u32 penalty_smooth1;
	u32 penalty_smooth2;

	if (argc < 7) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	penalty_nonsmooth = simple_strtoul(argv[3], NULL, 0);
	penalty_smooth0 = simple_strtoul(argv[4], NULL, 0);
	penalty_smooth1 = simple_strtoul(argv[5], NULL, 0);
	penalty_smooth2 = simple_strtoul(argv[6], NULL, 0);

	sde_drv_param[id].penaltyValues.uiScanPenaltyNonsmooth = penalty_nonsmooth;
	sde_drv_param[id].penaltyValues.uiScanPenaltySmooth0 = penalty_smooth0;
	sde_drv_param[id].penaltyValues.uiScanPenaltySmooth1 = penalty_smooth1;
	sde_drv_param[id].penaltyValues.uiScanPenaltySmooth2 = penalty_smooth2;
	sde_proc_cmd_printf("set sde_drv_param(%d) penaltyValues.uiScanPenaltyNonsmooth = %d\r\n", id, penalty_nonsmooth);
	sde_proc_cmd_printf("set sde_drv_param(%d) penaltyValues.uiScanPenaltySmooth0 = %d\r\n", id, penalty_smooth0);
	sde_proc_cmd_printf("set sde_drv_param(%d) penaltyValues.uiScanPenaltySmooth1 = %d\r\n", id, penalty_smooth1);
	sde_proc_cmd_printf("set sde_drv_param(%d) penaltyValues.uiScanPenaltySmooth2 = %d\r\n", id, penalty_smooth2);

	return 0;
}

static INT32 sde_proc_cmd_set_smooth(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 delta_displut0;
	u32 delta_displut1;

	if (argc < 5) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	delta_displut0 = simple_strtoul(argv[3], NULL, 0);
	delta_displut1 = simple_strtoul(argv[4], NULL, 0);

	sde_drv_param[id].thSmooth.uiDeltaDispLut0 = delta_displut0;
	sde_drv_param[id].thSmooth.uiDeltaDispLut1 = delta_displut1;
	sde_proc_cmd_printf("set sde_drv_param(%d) thSmooth.uiDeltaDispLut0 = %d\r\n", id, delta_displut0);
	sde_proc_cmd_printf("set sde_drv_param(%d) thSmooth.uiDeltaDispLut1 = %d\r\n", id, delta_displut1);

	return 0;
}

static INT32 sde_proc_cmd_set_lrc_thresh(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 thresh_lut0;
	u32 thresh_lut1;
	u32 thresh_lut2;
	u32 thresh_lut3;
	u32 thresh_lut4;
	u32 thresh_lut5;
	u32 thresh_lut6;

	if (argc < 10) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	thresh_lut0 = simple_strtoul(argv[3], NULL, 0);
	thresh_lut1 = simple_strtoul(argv[4], NULL, 0);
	thresh_lut2 = simple_strtoul(argv[5], NULL, 0);
	thresh_lut3 = simple_strtoul(argv[6], NULL, 0);
	thresh_lut4 = simple_strtoul(argv[7], NULL, 0);
	thresh_lut5 = simple_strtoul(argv[8], NULL, 0);
	thresh_lut6 = simple_strtoul(argv[9], NULL, 0);

	sde_drv_param[id].thDiag.uiDiagThreshLut0 = thresh_lut0;
	sde_drv_param[id].thDiag.uiDiagThreshLut1 = thresh_lut1;
	sde_drv_param[id].thDiag.uiDiagThreshLut2 = thresh_lut2;
	sde_drv_param[id].thDiag.uiDiagThreshLut3 = thresh_lut3;
	sde_drv_param[id].thDiag.uiDiagThreshLut4 = thresh_lut4;
	sde_drv_param[id].thDiag.uiDiagThreshLut5 = thresh_lut5;
	sde_drv_param[id].thDiag.uiDiagThreshLut6 = thresh_lut6;
	sde_proc_cmd_printf("set sde_drv_param(%d) thDiag.uiDiagThreshLut0 = %d\r\n", id, thresh_lut0);
	sde_proc_cmd_printf("set sde_drv_param(%d) thDiag.uiDiagThreshLut1 = %d\r\n", id, thresh_lut1);
	sde_proc_cmd_printf("set sde_drv_param(%d) thDiag.uiDiagThreshLut2 = %d\r\n", id, thresh_lut2);
	sde_proc_cmd_printf("set sde_drv_param(%d) thDiag.uiDiagThreshLut3 = %d\r\n", id, thresh_lut3);
	sde_proc_cmd_printf("set sde_drv_param(%d) thDiag.uiDiagThreshLut4 = %d\r\n", id, thresh_lut4);
	sde_proc_cmd_printf("set sde_drv_param(%d) thDiag.uiDiagThreshLut5 = %d\r\n", id, thresh_lut5);
	sde_proc_cmd_printf("set sde_drv_param(%d) thDiag.uiDiagThreshLut6 = %d\r\n", id, thresh_lut6);

	return 0;
}

static INT32 sde_proc_cmd_set_confidence_th(SDE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 confidence_th;

	if (argc < 4) {
		sde_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	confidence_th = simple_strtoul(argv[3], NULL, 0);

	sde_drv_param[id].confidence_th = confidence_th;
	sde_proc_cmd_printf("set sde_drv_param(%d) confidence_th = %d\r\n", id, confidence_th);

	return 0;
}

static SDE_PROC_CMD sde_proc_cmd_w_list[] = {
	// keyword              function name
	{ "open",               sde_proc_cmd_open,               "open sde"},
	{ "close",              sde_proc_cmd_close,              "close sde"},
	{ "trig",               sde_proc_cmd_trig,               "trigger sde, param1 is sde_id"},
	{ "dbg",                sde_proc_cmd_set_dbg,            "set sde dbg level, param1 is sde_id, param2 is dbg_lv"},
	{ "ext_en",             sde_proc_cmd_set_ext_en,         "set sde ext_en param (sde_id, scan_en, diag_en, input2_en, cond_disp_en, ui_intr_en) "},
	{ "ext_mode",           sde_proc_cmd_set_ext_mode,       "set sde ext_mode param (sde_id, cost_comp_mode, conf_out2_mode) "},
	{ "ext_sel",            sde_proc_cmd_set_ext_sel,        "set sde ext_sel param (sde_id, out_sel, out_vol_sel, conf_min2_sel, burst_sel) "},
	{ "ext_flip",           sde_proc_cmd_set_ext_flip,       "set sde ext_flip param (sde_id, h_flip_01, v_flip_01, h_flip_2, v_flip_2) "},
	{ "ctrl",               sde_proc_cmd_set_ctrl,           "set sde ctrl param, param1 is sde_id, param2 ~ param5 is SDE_IF_CTRL_PARAM"},
	{ "cost",               sde_proc_cmd_set_cost,           "set sde cost param, param1 is sde_id, param2 ~ param6 is SDE_IF_COST_PARAM"},
	{ "luma",               sde_proc_cmd_set_luma,           "set sde luma param, param1 is sde_id, param2 ~ param4 is SDE_IF_LUMA_PARAM"},
	{ "penalty",            sde_proc_cmd_set_penalty,        "set sde penalty param, param1 is sde_id, param2 ~ param5 is SDE_IF_PENALTY_PARAM"},
	{ "smooth",             sde_proc_cmd_set_smooth,         "set sde smooth param, param1 is sde_id, param2 ~ param3 is SDE_IF_SMOOTH_PARAM"},
	{ "lrc_thresh",         sde_proc_cmd_set_lrc_thresh,     "set sde lrc_thresh param, param1 is sde_id, param2 ~ param8 is SDE_IF_LRC_THRESH_PARAM"},
	{ "confidence_th",      sde_proc_cmd_set_confidence_th,  "set sde confidence_th param, param1 is sde_id, param2 is confidence_th"},
};
#define NUM_OF_W_CMD (sizeof(sde_proc_cmd_w_list) / sizeof(SDE_PROC_CMD))

static INT32 sde_proc_command_show(struct seq_file *sfile, void *v)
{
	int align_byte = 4;

	if (sde_proc_cmd_msg.buf == NULL && sde_proc_r_item == SDE_PROC_R_ITEM_NONE) {
		return -EINVAL;
	}

	if (sde_proc_cmd_msg.buf > 0) {
		seq_printf(sfile, "%s\n", sde_proc_cmd_msg.buf);
		sde_proc_msg_buf_clean();
	}

	if (sde_proc_r_item == SDE_PROC_R_ITEM_BUFFER) {
		seq_printf(sfile, " Buffer size :\n");
		seq_printf(sfile, "   sde_param : %9d \r\n"
			, ALIGN_CEIL(sizeof(SDE_IF_IO_INFO), align_byte) + ALIGN_CEIL(sizeof(SDE_IF_CTRL_PARAM), align_byte));
		seq_printf(sfile, "     io_info : %9d \r\n", ALIGN_CEIL(sizeof(SDE_IF_IO_INFO), align_byte));
		seq_printf(sfile, "        ctrl : %9d \r\n", ALIGN_CEIL(sizeof(SDE_IF_CTRL_PARAM), align_byte));
	}

	if (sde_proc_r_item == SDE_PROC_R_ITEM_PARAM) {
		seq_printf(sfile, " get sde param\n");
		seq_printf(sfile, " \r\n");

		// sde_if_io_info
		seq_printf(sfile, "==================== io_info ==================== \r\n");
		seq_printf(sfile, "width = %d, \r\n", sde_if_param[sde_proc_id]->io_info->width);
		seq_printf(sfile, "height = %d, \r\n", sde_if_param[sde_proc_id]->io_info->height);
		seq_printf(sfile, "in0_addr = %d, \r\n", sde_if_param[sde_proc_id]->io_info->in0_addr);
		seq_printf(sfile, "in1_addr = %d, \r\n", sde_if_param[sde_proc_id]->io_info->in1_addr);
		seq_printf(sfile, "cost_addr = %d, \r\n", sde_if_param[sde_proc_id]->io_info->cost_addr);
		seq_printf(sfile, "out0_addr = %d, \r\n", sde_if_param[sde_proc_id]->io_info->out0_addr);
		seq_printf(sfile, "out1_addr = %d, \r\n", sde_if_param[sde_proc_id]->io_info->out1_addr);
		seq_printf(sfile, "in0_lofs = %d, \r\n", sde_if_param[sde_proc_id]->io_info->in0_lofs);
		seq_printf(sfile, "in1_lofs = %d, \r\n", sde_if_param[sde_proc_id]->io_info->in1_lofs);
		seq_printf(sfile, "cost_lofs = %d, \r\n", sde_if_param[sde_proc_id]->io_info->cost_lofs);
		seq_printf(sfile, "out0_lofs = %d, \r\n", sde_if_param[sde_proc_id]->io_info->out0_lofs);
		seq_printf(sfile, "out1_lofs = %d, \r\n", sde_if_param[sde_proc_id]->io_info->out1_lofs);

		// sde_if_ctrl
		seq_printf(sfile, "==================== ctrl ==================== \r\n");
		seq_printf(sfile, "disp_val_mode = %d, \r\n", sde_if_param[sde_proc_id]->ctrl->disp_val_mode);
		seq_printf(sfile, "disp_op_mode = %d, \r\n", sde_if_param[sde_proc_id]->ctrl->disp_op_mode);
		seq_printf(sfile, "disp_inv_sel = %d, \r\n", sde_if_param[sde_proc_id]->ctrl->disp_inv_sel);
		seq_printf(sfile, "conf_en = %d, \r\n", sde_if_param[sde_proc_id]->ctrl->conf_en);
	}

	sde_proc_r_item = SDE_PROC_R_ITEM_NONE;

	return 0;
}

static INT32 sde_proc_command_open(struct inode *inode, struct file *file)
{
	//return single_open(file, sde_proc_command_show, PDE_DATA(inode));
	return single_open_size(file, sde_proc_command_show, PDE_DATA(inode), 5000*sizeof(u32));
}

static ssize_t sde_proc_command_write(struct file *file, const CHAR __user *buf, size_t size, loff_t *off)
{
	INT32 len = size;
	INT32 ret = -EINVAL;

	CHAR cmd_line[MAX_CMDLINE_LENGTH];
	CHAR *cmdstr = cmd_line;
	const CHAR delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	CHAR *argv[MAX_CMD_ARGUMENTS] = {NULL};
	INT32 argc = 0;
	UINT32 i;
	SDE_MODULE *sde_module = sde_proc_get_mudule_from_file(file);

	// check command length
	if ((len <= 1) || (len > MAX_CMDLINE_LENGTH)) {
		DBG_ERR("command is too short or long!! \r\n");
	} else {
		// copy command string from user space
		if (copy_from_user(cmd_line, buf, len)) {
			;
		} else {
			cmd_line[len-1] = '\0';

			// parse command string
			for (i = 0; i < MAX_CMD_ARGUMENTS; i++) {
				argv[i] = strsep(&cmdstr, delimiters);
				if (argv[i] != NULL) {
					argc++;
				} else {
					break;
				}
			}

			// dispatch command handler
			ret = -EINVAL;

			if (strncmp(argv[0], "r", 2) == 0) {
				for (i = 0; i < NUM_OF_R_CMD; i++) {
					if (strncmp(argv[1], sde_proc_cmd_r_list[i].cmd, MAX_CMD_LENGTH) == 0) {
						#if MUTEX_ENABLE
						down(&mutex);
						#endif
						ret = sde_proc_cmd_r_list[i].execute(sde_module, argc, argv);
						#if MUTEX_ENABLE
						up(&mutex);
						#endif
						break;
					}
				}

				if (i >= NUM_OF_R_CMD) {
					DBG_ERR("[VOE_ERR]: => ");
					for (i = 0; i < argc; i++) {
						DBG_ERR("%s ", argv[i]);
					}
					DBG_ERR("is not in r_cmd_list!! \r\n");
				}
			} else if (strncmp(argv[0], "w", 2) == 0) {
				for (i = 0; i < NUM_OF_W_CMD; i++) {
					if (strncmp(argv[1], sde_proc_cmd_w_list[i].cmd, MAX_CMD_LENGTH) == 0) {
						#if MUTEX_ENABLE
						down(&mutex);
						#endif
						ret = sde_proc_cmd_w_list[i].execute(sde_module, argc, argv);
						#if MUTEX_ENABLE
						up(&mutex);
						#endif
						break;
					}
				}

				if (i >= NUM_OF_W_CMD) {
					DBG_ERR("[SDE_ERR]: =>");
					for (i = 0; i < argc; i++) {
						DBG_ERR("%s ", argv[i]);
					}
					DBG_ERR("is not in w_cmd_list!! \r\n");
				}
			} else {
				DBG_ERR("[SDE_ERR]: =>");
				for (i = 0; i < argc; i++) {
					DBG_ERR("%s ", argv[i]);
				}
				DBG_ERR("is not legal command!! \r\n");
			}
		}
	}

	if (ret < 0) {
		DBG_ERR("[SDE_ERR]: fail to execute: ");
		for (i = 0; i < argc; i++) {
			DBG_ERR("%s ", argv[i]);
		}
		DBG_ERR("\r\n");
	}

	return size;
}

static const struct file_operations sde_proc_command_fops = {
	.owner   = THIS_MODULE,
	.open    = sde_proc_command_open,
	.read    = seq_read,
	.write   = sde_proc_command_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static INT32 sde_proc_help_show(struct seq_file *sfile, void *v)
{
	UINT32 loop;

	seq_printf(sfile, "1. 'cat /proc/hdal/vendor/sde/info' will show all the sde info \r\n");
	seq_printf(sfile, "2. 'echo r/w xxx > /proc/hdal/vendor/sde/cmd' can input command for some debug purpose \r\n");
	seq_printf(sfile, "The currently support input command are below: \r\n");

	seq_printf(sfile, "--------------------------------------------------------------------- \r\n");
	seq_printf(sfile, "  %s\n", "iq");
	seq_printf(sfile, "--------------------------------------------------------------------- \r\n");

	for (loop = 0 ; loop < NUM_OF_R_CMD ; loop++) {
		seq_printf(sfile, "r %15s : %s\r\n", sde_proc_cmd_r_list[loop].cmd, sde_proc_cmd_r_list[loop].text);
	}

	for (loop = 0 ; loop < NUM_OF_W_CMD ; loop++) {
		seq_printf(sfile, "w %15s : %s\r\n", sde_proc_cmd_w_list[loop].cmd, sde_proc_cmd_w_list[loop].text);
	}

	seq_printf(sfile, "--------------------------------------------------------------------- \r\n");
	seq_printf(sfile, " dbg_lv = \r\n");
	seq_printf(sfile, "   | 0x%8x = ERR    | 0x%8x = CFG    | 0x%8x = DTS    | \r\n", SDE_DBG_ERR_MSG, SDE_DBG_CFG, SDE_DBG_DTS);
	seq_printf(sfile, "   | 0x%8x = SET    | \r\n", SDE_DBG_RUN_SET);
	seq_printf(sfile, "--------------------------------------------------------------------- \r\n");
	seq_printf(sfile, "Ex1: 'echo w open 0 > /proc/hdal/vendor/sde/cmd;cat /proc/hdal/vendor/sde/cmd' \r\n");
	seq_printf(sfile, "Ex2: 'echo w trig 0 > /proc/hdal/vendor/sde/cmd;cat /proc/hdal/vendor/sde/cmd' \r\n");
	seq_printf(sfile, "Ex3: 'echo w close 0 > /proc/hdal/vendor/sde/cmd;cat /proc/hdal/vendor/sde/cmd' \r\n");
	seq_printf(sfile, "Ex4: 'echo w dbg 0x10 > /proc/hdal/vendor/sde/cmd;cat /proc/hdal/vendor/sde/cmd' \r\n");
	seq_printf(sfile, "Ex4: 'echo r param 0 > /proc/hdal/vendor/sde/cmd;cat /proc/hdal/vendor/sde/cmd' \r\n");
	//seq_printf(sfile, "Ex2: 'echo w reload_cfg 0 /mnt/app/isp/isp_imx290_0.cfg > /proc/hdal/vendor/iq/cmd;cat /proc/hdal/vendor/iq/cmd' \r\n");

	return 0;
}

static INT32 sde_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, sde_proc_help_show, PDE_DATA(inode));
}

static const struct file_operations sde_proc_help_fops = {
	.owner   = THIS_MODULE,
	.open    = sde_proc_help_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// extern functions
//=============================================================================
INT32 sde_proc_init(SDE_DRV_INFO *pdrv_info)
{
	INT32 ret = 0;
	struct proc_dir_entry *root = NULL, *pentry = NULL;

	// initialize synchronization mechanism
	#if MUTEX_ENABLE
	sema_init(&mutex, 1);
	#endif

	root = proc_mkdir("hdal/vendor/sde", NULL);

	if (root == NULL) {
		DBG_ERR("fail to create SDE proc root!! \r\n");
		sde_proc_remove(pdrv_info);
		return -EINVAL;
	}
	proc_root = root;

	// create "info" entry
	pentry = proc_create_data("info", S_IRUGO | S_IXUGO, root, &sde_proc_info_fops, (void *)pdrv_info);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc info!! \r\n");
		sde_proc_remove(pdrv_info);
		return -EINVAL;
	}
	proc_info = pentry;

	// create "command" entry
	pentry = proc_create_data("cmd", S_IRUGO | S_IXUGO, root, &sde_proc_command_fops, (void *)pdrv_info);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc cmd!! \r\n");
		sde_proc_remove(pdrv_info);
		return -EINVAL;
	}
	proc_command = pentry;

	// create "help" entry
	pentry = proc_create_data("help", S_IRUGO | S_IXUGO, root, &sde_proc_help_fops, (void *)pdrv_info);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc help!! \r\n");
		sde_proc_remove(pdrv_info);
		return -EINVAL;
	}
	proc_help = pentry;

	// allocate memory for massage buffer
	ret = sde_proc_msg_buf_alloc();
	if (ret < 0) {
		sde_proc_remove(pdrv_info);
	}

	return ret;
}

void sde_proc_remove(SDE_DRV_INFO *pdrv_info)
{
	if (proc_root == NULL) {
		return;
	}

	// remove "info"
	if (proc_info) {
		proc_remove(proc_info);
	}
	// remove "cmd"
	if (proc_info) {
		proc_remove(proc_command);
	}
	// remove "help"
	if (proc_help) {
		proc_remove(proc_help);
	}

	// remove root entry
	proc_remove(proc_root);

	// free message buffer
	sde_proc_msg_buf_free();
}

