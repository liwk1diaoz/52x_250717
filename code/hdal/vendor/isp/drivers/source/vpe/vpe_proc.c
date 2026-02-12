#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include "vpe_api.h"
//#include "vpe_dev.h"

#include "vpe_alg.h"
#include "vpet_api.h"
#include "vpe_api.h"
#include "vpet_api_int.h"
#include "vpe_dbg.h"
#include "vpe_main.h"
#include "vpe_proc.h"
#include "vpe_version.h"
#include "vpe_dev_int.h"

//=============================================================================
// avoid GPL api
//=============================================================================
#define MUTEX_ENABLE DISABLE
extern BOOL vpe_param_id_valid[VPE_ID_MAX_NUM];

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

static VPE_PROC_MSG_BUF vpe_proc_cmd_msg;
static VPE_PROC_R_ITEM vpe_proc_r_item = VPE_PROC_R_ITEM_NONE;
static VPE_ID vpe_proc_id;
//static UINT32 vpe_proc_iso;

// NOTE: temporal
extern BOOL vpe_update[VPE_ID_MAX_NUM][VPET_ITEM_MAX];

//=============================================================================
// routines
//=============================================================================
static inline VPE_MODULE *vpe_proc_get_mudule_from_file(struct file *file)
{
	return (VPE_MODULE *)((struct seq_file *)file->private_data)->private;
}

//=============================================================================
// proc "info" file operation functions
//=============================================================================
static INT32 vpe_proc_info_show(struct seq_file *sfile, void *v)
{
	UINT32 version = vpe_get_version();

	seq_printf(sfile, "---------------------------------------------------------------------------------------------- \r\n");
	seq_printf(sfile, "NVT_VPE v%d.%d.%d.%d \r\n", (version >> 24) & 0xFF, (version >> 16) & 0xFF, (version >> 8) & 0xFF, version & 0xFF);
	seq_printf(sfile, "---------------------------------------------------------------------------------------------- \r\n");

	return 0;
}

static INT32 vpe_proc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpe_proc_info_show, PDE_DATA(inode));
}

static const struct file_operations vpe_proc_info_fops = {
	.owner   = THIS_MODULE,
	.open    = vpe_proc_info_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "cmd" file operation functions
//=============================================================================
static inline INT32 vpe_proc_msg_buf_alloc(void)
{
	vpe_proc_cmd_msg.buf = kzalloc(PROC_MSG_BUFSIZE, GFP_KERNEL);

	if (vpe_proc_cmd_msg.buf == NULL) {
		DBG_WRN("fail to allocate VPE message buffer!! \r\n");
		return -ENOMEM;
	}

	vpe_proc_cmd_msg.size = PROC_MSG_BUFSIZE;
	vpe_proc_cmd_msg.count = 0;
	return 0;
}

static inline void vpe_proc_msg_buf_free(void)
{
	if (vpe_proc_cmd_msg.buf) {
		kfree(vpe_proc_cmd_msg.buf);
		vpe_proc_cmd_msg.buf = NULL;
	}
}

static inline void vpe_proc_msg_buf_clean(void)
{
	vpe_proc_cmd_msg.buf[0] = '\0';
	vpe_proc_cmd_msg.count = 0;
}

static INT32 vpe_proc_cmd_printf(const s8 *f, ...)
{
	INT32 len;
	va_list args;

	if (vpe_proc_cmd_msg.count < vpe_proc_cmd_msg.size) {
		va_start(args, f);
		len = vsnprintf(vpe_proc_cmd_msg.buf + vpe_proc_cmd_msg.count, vpe_proc_cmd_msg.size - vpe_proc_cmd_msg.count, f, args);
		va_end(args);

		if ((vpe_proc_cmd_msg.count + len) < vpe_proc_cmd_msg.size) {
			vpe_proc_cmd_msg.count += len;
			return 0;
		}
	}

	vpe_proc_cmd_msg.count = vpe_proc_cmd_msg.size;
	return -1;
}

static INT32 vpe_proc_cmd_get_buffer_size(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	vpe_proc_r_item = VPE_PROC_R_ITEM_BUFFER;
	return 0;
}

static INT32 vpe_proc_cmd_get_param(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 4) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	vpe_proc_id = simple_strtoul(argv[2], NULL, 0);

	if((vpe_param_id_valid[vpe_proc_id]== TRUE) && (vpe_proc_id < VPE_ID_MAX_NUM)){
		vpe_proc_r_item = VPE_PROC_R_ITEM_PARAM;
	}else{
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", vpe_proc_id);
		vpe_proc_r_item = VPE_PROC_R_ITEM_NONE;
	}

	return 0;
}

static INT32 vpe_proc_cmd_get_cfg_data(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 3) {
		vpe_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	vpe_proc_id = simple_strtoul(argv[2], NULL, 0);

	if((vpe_param_id_valid[vpe_proc_id]== TRUE) && (vpe_proc_id < VPE_ID_MAX_NUM)){
		vpe_proc_r_item = VPE_PROC_R_ITEM_PARAM;
	}else{
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", vpe_proc_id);
		vpe_proc_r_item = VPE_PROC_R_ITEM_NONE;
	}

	return 0;
}

static INT32 vpe_proc_cmd_get_sharpen_param(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	vpe_proc_id = simple_strtoul(argv[2], NULL, 0);

	if((vpe_param_id_valid[vpe_proc_id]== TRUE) && (vpe_proc_id < VPE_ID_MAX_NUM)){
		vpe_proc_r_item = VPE_PROC_R_ITEM_SHARPEN;
	}else{
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", vpe_proc_id);
		vpe_proc_r_item = VPE_PROC_R_ITEM_NONE;
	}

	return 0;
}

static INT32 vpe_proc_cmd_get_dce_ctl_param(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	vpe_proc_id = simple_strtoul(argv[2], NULL, 0);

	if((vpe_param_id_valid[vpe_proc_id]== TRUE) && (vpe_proc_id < VPE_ID_MAX_NUM)){
		vpe_proc_r_item = VPE_PROC_R_ITEM_DCE_CTL;
	}else{
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", vpe_proc_id);
		vpe_proc_r_item = VPE_PROC_R_ITEM_NONE;
	}

	return 0;
}

static INT32 vpe_proc_cmd_get_gdc_param(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	vpe_proc_id = simple_strtoul(argv[2], NULL, 0);

	if((vpe_param_id_valid[vpe_proc_id]== TRUE) && (vpe_proc_id < VPE_ID_MAX_NUM)){
		vpe_proc_r_item = VPE_PROC_R_ITEM_GDC;
	}else{
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", vpe_proc_id);
		vpe_proc_r_item = VPE_PROC_R_ITEM_NONE;
	}

	return 0;
}

static INT32 vpe_proc_cmd_get_2dlut_param(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	vpe_proc_id = simple_strtoul(argv[2], NULL, 0);

	if((vpe_param_id_valid[vpe_proc_id]== TRUE) && (vpe_proc_id < VPE_ID_MAX_NUM)){
		vpe_proc_r_item = VPE_PROC_R_ITEM_2DLUT;
	}else{
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", vpe_proc_id);
		vpe_proc_r_item = VPE_PROC_R_ITEM_NONE;
	}

	return 0;
}

static INT32 vpe_proc_cmd_get_drt_param(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	vpe_proc_id = simple_strtoul(argv[2], NULL, 0);

	if((vpe_param_id_valid[vpe_proc_id]== TRUE) && (vpe_proc_id < VPE_ID_MAX_NUM)){
		vpe_proc_r_item = VPE_PROC_R_ITEM_DRT;
	}else{
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", vpe_proc_id);
		vpe_proc_r_item = VPE_PROC_R_ITEM_NONE;
	}

	return 0;
}

static INT32 vpe_proc_cmd_get_dctg_param(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	vpe_proc_id = simple_strtoul(argv[2], NULL, 0);

	if((vpe_param_id_valid[vpe_proc_id]== TRUE) && (vpe_proc_id < VPE_ID_MAX_NUM)){
		vpe_proc_r_item = VPE_PROC_R_ITEM_DCTG;
	}else{
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", vpe_proc_id);
		vpe_proc_r_item = VPE_PROC_R_ITEM_NONE;
	}

	return 0;
}

static INT32 vpe_proc_cmd_get_flip_rot_param(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 3) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	vpe_proc_id = simple_strtoul(argv[2], NULL, 0);

	if((vpe_param_id_valid[vpe_proc_id]== TRUE) && (vpe_proc_id < VPE_ID_MAX_NUM)){
		vpe_proc_r_item = VPE_PROC_R_ITEM_FLIP_ROT;
	}else{
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", vpe_proc_id);
		vpe_proc_r_item = VPE_PROC_R_ITEM_NONE;
	}

	return 0;
}


static VPE_PROC_CMD vpe_proc_cmd_r_list[] = {
	// keyword              function name
	{ "buffer_size",        vpe_proc_cmd_get_buffer_size,    "get vpe buffer size"},
	{ "param",              vpe_proc_cmd_get_param,          "get vpe param, param1 is vpe_id(0~2), param2 is param_id(0~11)"},
	{ "cfg_data",           vpe_proc_cmd_get_cfg_data,       "get vpe cfg file name, param1 is vpe_id(0~2)"},
	{ "get_sharpen_param",  vpe_proc_cmd_get_sharpen_param,  "get vpe sharpen param, param1 is vpe_id(0~2)"},
	{ "get_dce_ctl_param",  vpe_proc_cmd_get_dce_ctl_param,  "get vpe dce ctl param, param1 is vpe_id(0~2)"},
	{ "get_gdc_param",      vpe_proc_cmd_get_gdc_param,      "get vpe gdc param, param1 is vpe_id(0~2)"},
	{ "get_2dlut_param",    vpe_proc_cmd_get_2dlut_param,    "get vpe 2dlut param, param1 is vpe_id(0~2)"},
	{ "get_drt_param",      vpe_proc_cmd_get_drt_param,      "get vpe drt param, param1 is vpe_id(0~2)"},
	{ "get_dctg_param",     vpe_proc_cmd_get_dctg_param,     "get vpe dctg param, param1 is vpe_id(0~2)"},
	{ "get_flip_rot_param",  vpe_proc_cmd_get_flip_rot_param,  "get vpe flip_rot param, param1 is vpe_id(0~2)"},
};
#define NUM_OF_R_CMD (sizeof(vpe_proc_cmd_r_list) / sizeof(VPE_PROC_CMD))

static INT32 vpe_proc_cmd_set_dbg(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	VPE_ID id;
	u32 cmd;

	if (argc < 4) {
		vpe_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	cmd = simple_strtoul(argv[3], NULL, 0);

	vpe_proc_cmd_printf("set vpe(%d) dbg level(0x%x) \n", id, cmd);
	vpe_dbg_set_dbg_mode(id, cmd);
	return 0;
}

static INT32 vpe_proc_cmd_sharpen_en(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	VPE_ID id;
	UINT8  sharpen_en;

	if (argc < 4) {
		vpe_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	sharpen_en = simple_strtoul(argv[3], NULL, 0);

	if((vpe_param_id_valid[id]== TRUE) && (id < VPE_ID_MAX_NUM)){
		vpe_param[id]->sharpen->enable = sharpen_en;
		vpe_update[id][VPET_ITEM_SHARPEN_PARAM] = TRUE;
	}else{
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", id);
		return 0;
	}

	return 0;
}

static INT32 vpe_proc_cmd_edge_sharp_str(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	VPE_ID id;
	UINT8  edge_sharp_str1;
	UINT8  edge_sharp_str2;

	if (argc < 5) {
		vpe_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	edge_sharp_str1 = simple_strtoul(argv[3], NULL, 0);
	edge_sharp_str2 = simple_strtoul(argv[4], NULL, 0);

	if((vpe_param_id_valid[id]== TRUE) && (id < VPE_ID_MAX_NUM)){
		vpe_param[id]->sharpen->edge_sharp_str1 = edge_sharp_str1;
		vpe_param[id]->sharpen->edge_sharp_str2 = edge_sharp_str2;
		vpe_update[id][VPET_ITEM_SHARPEN_PARAM] = TRUE;
		
	}else{
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", id);
		return 0;
	}

	return 0;
}

static INT32 vpe_proc_cmd_flat_sharp_str(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	VPE_ID id;
	UINT8  flat_sharp_str;

	if (argc < 4) {
		vpe_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	flat_sharp_str = simple_strtoul(argv[3], NULL, 0);

	if((vpe_param_id_valid[id]== TRUE) && (id < VPE_ID_MAX_NUM)){
		vpe_param[id]->sharpen->flat_sharp_str = flat_sharp_str;
		vpe_update[id][VPET_ITEM_SHARPEN_PARAM] = TRUE;
	}else{
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", id);
		return 0;
	}

	return 0;
}

static INT32 vpe_proc_cmd_edge_weight_gain(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	VPE_ID id;
	UINT8  edge_weight_gain;

	if (argc < 4) {
		vpe_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	edge_weight_gain = simple_strtoul(argv[3], NULL, 0);

	if((vpe_param_id_valid[id]== TRUE) && (id < VPE_ID_MAX_NUM)){
		vpe_param[id]->sharpen->edge_weight_gain = edge_weight_gain;
		vpe_update[id][VPET_ITEM_SHARPEN_PARAM] = TRUE;
	}else{
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", id);
		return 0;
	}

	return 0;
}

/*
static INT32 vpe_proc_cmd_dctg(VPE_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	VPE_ID id;
	UINT8  mount_type;
	UINT8  lut2d_sz;
	UINT16 lens_r;
	UINT16 lens_x_st, lens_y_st;
	INT32  theta_st, theta_ed;
	INT32  phi_st, phi_ed;
	INT32  rot_z, rot_y;

	if (argc < 14) {
		vpe_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	mount_type = simple_strtoul(argv[3], NULL, 0);
	lut2d_sz = simple_strtoul(argv[4], NULL, 0);
	lens_r = simple_strtoul(argv[5], NULL, 0);
	lens_x_st = simple_strtoul(argv[6], NULL, 0);
	lens_y_st = simple_strtoul(argv[7], NULL, 0);
	theta_st = simple_strtoul(argv[8], NULL, 0);
	theta_ed = simple_strtoul(argv[9], NULL, 0);
	phi_st = simple_strtoul(argv[10], NULL, 0);
	phi_ed = simple_strtoul(argv[11], NULL, 0);
	rot_y = simple_strtoul(argv[12], NULL, 0);
	rot_z = simple_strtoul(argv[13], NULL, 0);

	if((vpe_param_id_valid[id]== TRUE) && (id < VPE_ID_MAX_NUM)){
		vpe_param[id]->dctg->lens_r = lens_r;
		vpe_param[id]->dctg->lens_x_st = lens_x_st;
		vpe_param[id]->dctg->lens_y_st = lens_y_st;
		vpe_param[id]->dctg->lut2d_sz = lut2d_sz;
		vpe_param[id]->dctg->mount_type = mount_type;
		vpe_param[id]->dctg->phi_ed = phi_ed;
		vpe_param[id]->dctg->phi_st = phi_st;
		vpe_param[id]->dctg->rot_y= rot_y;
		vpe_param[id]->dctg->rot_z = rot_z;
		vpe_param[id]->dctg->theta_ed = theta_ed;
		vpe_param[id]->dctg->theta_st = theta_st;
		vpe_update[id][VPET_ITEM_DCTG_PARAM] = TRUE;
	}else{
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", id);
		return 0;
	}

	return 0;
}
*/

static VPE_PROC_CMD vpe_proc_cmd_w_list[] = {
	// keyword              function name
	{ "dbg",                vpe_proc_cmd_set_dbg,            "set vpe dbg level, param1 is vpe_id(0~2), param2 is dbg_lv"},
	{ "sharpen_en",         vpe_proc_cmd_sharpen_en,         "set sharpen_en, param1 is vpe_id(0~2), param2 is sharpen_en"},
	{ "edge_sharp_str",     vpe_proc_cmd_edge_sharp_str,     "set edge sharp str, param1 is vpe_id(0~2), param2 is edge sharp str1 value, param3 is edge sharp str2 value"},
	{ "flat_sharp_str",     vpe_proc_cmd_flat_sharp_str,     "set flat_sharp_str, param1 is vpe_id(0~2), param2 is flat_sharp_str value"},
	{ "edge_weight_gain",   vpe_proc_cmd_edge_weight_gain,   "set edge_weight_gain, param1 is vpe_id(0~2), param2 is edge_weight_gain"},
	//{ "dctg",               vpe_proc_cmd_dctg,               "set dctg, param1 is vpe_id(0~2), param2 is enable, param 3 is mount_type, param 4 is lut2d_sz, param 5 is lens_r, param 6 is lens_x_st, param 7 is lens_y_st, parma 8 is theta_st, parma 9 is theta_ed, param 10 is rot_y, param 11 is rot_z"},
};
#define NUM_OF_W_CMD (sizeof(vpe_proc_cmd_w_list) / sizeof(VPE_PROC_CMD))

static INT32 vpe_proc_command_show(struct seq_file *sfile, void *v)
{
	int i=0;

	if (vpe_proc_r_item == VPE_PROC_R_ITEM_SHARPEN) {
		seq_printf(sfile, " get vpe param (%d) \n", vpe_proc_id);

		// vpe_sharpen_param
		seq_printf(sfile, "====================id(%d) vpe sharpen_param==================== \r\n", vpe_proc_id);
		seq_printf(sfile, "sharpen enable = %d, \r\n", vpe_param[vpe_proc_id]->sharpen->enable);
		seq_printf(sfile, "sharpen edge sharp str1 = %d, \r\n", vpe_param[vpe_proc_id]->sharpen->edge_sharp_str1);
		seq_printf(sfile, "sharpen edge sharp str2 = %d, \r\n", vpe_param[vpe_proc_id]->sharpen->edge_sharp_str2);
		seq_printf(sfile, "sharpen flat sharp str = %d, \r\n", vpe_param[vpe_proc_id]->sharpen->flat_sharp_str);
		seq_printf(sfile, "sharpen edge weight gain = %d, \r\n", vpe_param[vpe_proc_id]->sharpen->edge_weight_gain);
	}

	if (vpe_proc_r_item == VPE_PROC_R_ITEM_DCE_CTL) {
		seq_printf(sfile, " get vpe param (%d) \n", vpe_proc_id);

		// vpe_dce_ctl_param
		seq_printf(sfile, "====================id(%d) vpe ctl_param==================== \r\n", vpe_proc_id);
		seq_printf(sfile, "dce enable = %d, \r\n", vpe_param[vpe_proc_id]->dce_ctl->enable);
		seq_printf(sfile, "dce mode = %d, \r\n", vpe_param[vpe_proc_id]->dce_ctl->dce_mode);
		seq_printf(sfile, "dce lens_radius = %d, \r\n", vpe_param[vpe_proc_id]->dce_ctl->lens_radius);
	}

	if (vpe_proc_r_item == VPE_PROC_R_ITEM_GDC) {
		seq_printf(sfile, " get vpe param (%d) \n", vpe_proc_id);

		// vpe_gdc_param
		seq_printf(sfile, "====================id(%d) vpe gdc_param==================== \r\n", vpe_proc_id);
		seq_printf(sfile, "gdc fovgain = %d, \r\n", vpe_param[vpe_proc_id]->gdc->fovgain);
		seq_printf(sfile, "cent_x_ratio = %d, \r\n", vpe_param[vpe_proc_id]->gdc->cent_x_ratio);
		seq_printf(sfile, "cent_y_ratio = %d, \r\n", vpe_param[vpe_proc_id]->gdc->cent_y_ratio);
		for(i=0; i<7; i++){
			seq_printf(sfile, "gdc_lut[%d] = %d, \r\n", i, vpe_param[vpe_proc_id]->gdc->gdc_lut[i]);
		}
	}

	if (vpe_proc_r_item == VPE_PROC_R_ITEM_2DLUT) {
		seq_printf(sfile, " get vpe param (%d) \n", vpe_proc_id);

		// vpe_2dlut_param
		seq_printf(sfile, "====================id(%d) vpe 2dlut_param==================== \r\n", vpe_proc_id);
		seq_printf(sfile, "2dlut size = %d, \r\n", vpe_param[vpe_proc_id]->lut2d->lut_sz);
		for(i=0; i<7; i++){
			seq_printf(sfile, "2dlut[%d] = 0x%X, \r\n", i, vpe_param[vpe_proc_id]->lut2d->lut[i]);
		}
		seq_printf(sfile, "outsize_w=%d \r\n", vpe_2dlut_output_size[vpe_proc_id].w);
		seq_printf(sfile, "outsize_w= %d \r\n", vpe_2dlut_output_size[vpe_proc_id].h);
	}

	if (vpe_proc_r_item == VPE_PROC_R_ITEM_DRT) {
		seq_printf(sfile, " get vpe param (%d) \n", vpe_proc_id);

		// vpe_drt_param
		seq_printf(sfile, "====================id(%d) vpe drt_param==================== \r\n", vpe_proc_id);
		for(i=0; i<4; i++){
			seq_printf(sfile, "drt[%d] = %d, \r\n", i, vpe_param[vpe_proc_id]->drt->cvt_sel[i]);
		}
	
	}

	if (vpe_proc_r_item == VPE_PROC_R_ITEM_DCTG) {
		seq_printf(sfile, " get vpe param (%d) \n", vpe_proc_id);

		// vpe_dctg_param
		seq_printf(sfile, "====================id(%d) vpe dctg_param==================== \r\n", vpe_proc_id);
		seq_printf(sfile, "dctg mount_type=%d, \r\n", vpe_param[vpe_proc_id]->dctg->mount_type);
		seq_printf(sfile, "dctg lut2d_sz=%d, \r\n", vpe_param[vpe_proc_id]->dctg->lut2d_sz);
		seq_printf(sfile, "dctg lens_r=%d, \r\n", vpe_param[vpe_proc_id]->dctg->lens_r);
		seq_printf(sfile, "dctg lens_x_st=%d, \r\n", vpe_param[vpe_proc_id]->dctg->lens_x_st);
		seq_printf(sfile, "dctg lens_y_st=%d, \r\n", vpe_param[vpe_proc_id]->dctg->lens_y_st);
		seq_printf(sfile, "dctg theta_st=%d, \r\n", vpe_param[vpe_proc_id]->dctg->theta_st);
		seq_printf(sfile, "dctg theta_ed=%d, \r\n", vpe_param[vpe_proc_id]->dctg->theta_ed);
		seq_printf(sfile, "dctg phi_st=%d, \r\n", vpe_param[vpe_proc_id]->dctg->phi_st);
		seq_printf(sfile, "dctg phi_ed=%d, \r\n", vpe_param[vpe_proc_id]->dctg->phi_ed);
		seq_printf(sfile, "dctg rot_y=%d, \r\n", vpe_param[vpe_proc_id]->dctg->rot_y);
		seq_printf(sfile, "dctg rot_z=%d, \r\n", vpe_param[vpe_proc_id]->dctg->rot_z);
	}

	if (vpe_proc_r_item == VPE_PROC_R_ITEM_FLIP_ROT) {
		seq_printf(sfile, " get vpe param (%d) \n", vpe_proc_id);

		// vpe_dctg_param
		seq_printf(sfile, "====================id(%d) vpe flip_rot_param==================== \r\n", vpe_proc_id);
		seq_printf(sfile, "flip_rot_mode=%d, \r\n", vpe_param[vpe_proc_id]->flip_rot->flip_rot_mode);
		seq_printf(sfile, "manual rot_degree=%d, \r\n", vpe_param[vpe_proc_id]->flip_rot->rot_manual_param.rot_degree);
		seq_printf(sfile, "manual flip=%d, \r\n", vpe_param[vpe_proc_id]->flip_rot->rot_manual_param.flip);
		seq_printf(sfile, "manual ratio_mode=%d, \r\n", vpe_param[vpe_proc_id]->flip_rot->rot_manual_param.ratio_mode);
		seq_printf(sfile, "manual fov_bound=%d, \r\n", vpe_param[vpe_proc_id]->flip_rot->rot_manual_param.fovbound);
		seq_printf(sfile, "manual boundy=%d, \r\n", vpe_param[vpe_proc_id]->flip_rot->rot_manual_param.boundy);
		seq_printf(sfile, "manual boundu=%d, \r\n", vpe_param[vpe_proc_id]->flip_rot->rot_manual_param.boundu);
		seq_printf(sfile, "manual boundv=%d, \r\n", vpe_param[vpe_proc_id]->flip_rot->rot_manual_param.boundv);
	}

	return 0;
}

static INT32 vpe_proc_command_open(struct inode *inode, struct file *file)
{
	//return single_open(file, vpe_proc_command_show, PDE_DATA(inode));
	return single_open_size(file, vpe_proc_command_show, PDE_DATA(inode), 5000*sizeof(u32));
}

static ssize_t vpe_proc_command_write(struct file *file, const CHAR __user *buf, size_t size, loff_t *off)
{
	INT32 len = size;
	INT32 ret = -EINVAL;

	CHAR cmd_line[MAX_CMDLINE_LENGTH];
	CHAR *cmdstr = cmd_line;
	const CHAR delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	CHAR *argv[MAX_CMD_ARGUMENTS] = {NULL};
	INT32 argc = 0;
	UINT32 i;
	VPE_MODULE *vpe_module = vpe_proc_get_mudule_from_file(file);

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
					if (strncmp(argv[1], vpe_proc_cmd_r_list[i].cmd, MAX_CMD_LENGTH) == 0) {
						#if MUTEX_ENABLE
						down(&mutex);
						#endif
						ret = vpe_proc_cmd_r_list[i].execute(vpe_module, argc, argv);
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
					if (strncmp(argv[1], vpe_proc_cmd_w_list[i].cmd, MAX_CMD_LENGTH) == 0) {
						#if MUTEX_ENABLE
						down(&mutex);
						#endif
						ret = vpe_proc_cmd_w_list[i].execute(vpe_module, argc, argv);
						#if MUTEX_ENABLE
						up(&mutex);
						#endif
						break;
					}
				}

				if (i >= NUM_OF_W_CMD) {
					DBG_ERR("[VPE_ERR]: =>");
					for (i = 0; i < argc; i++) {
						DBG_ERR("%s ", argv[i]);
					}
					DBG_ERR("is not in w_cmd_list!! \r\n");
				}
			} else {
				DBG_ERR("[VPE_ERR]: =>");
				for (i = 0; i < argc; i++) {
					DBG_ERR("%s ", argv[i]);
				}
				DBG_ERR("is not legal command!! \r\n");
			}
		}
	}

	if (ret < 0) {
		DBG_ERR("[VPE_ERR]: fail to execute: ");
		for (i = 0; i < argc; i++) {
			DBG_ERR("%s ", argv[i]);
		}
		DBG_ERR("\r\n");
	}

	return size;
}

static const struct file_operations vpe_proc_command_fops = {
	.owner   = THIS_MODULE,
	.open    = vpe_proc_command_open,
	.read    = seq_read,
	.write   = vpe_proc_command_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static INT32 vpe_proc_help_show(struct seq_file *sfile, void *v)
{
	UINT32 loop;

	seq_printf(sfile, "1. 'cat /proc/hdal/vendor/isp/info' will show all the isp info\r\n");
	seq_printf(sfile, "2. 'echo r/w xxx > /proc/hdal/vendor/isp/cmd' can input command for some debug purpose\r\n");
	seq_printf(sfile, "The currently support input command are below:\r\n");

	seq_printf(sfile, "---------------------------------------------------------------------\r\n");
	seq_printf(sfile, "  %s\n", "isp");
	seq_printf(sfile, "---------------------------------------------------------------------\r\n");

	for (loop = 0 ; loop < NUM_OF_R_CMD ; loop++) {
		seq_printf(sfile, "r %15s : %s\r\n", vpe_proc_cmd_r_list[loop].cmd, vpe_proc_cmd_r_list[loop].text);
	}

	for (loop = 0 ; loop < NUM_OF_W_CMD ; loop++) {
		seq_printf(sfile, "w %15s : %s\r\n", vpe_proc_cmd_w_list[loop].cmd, vpe_proc_cmd_w_list[loop].text);
	}

	seq_printf(sfile, "---------------------------------------------------------------------\r\n");
	seq_printf(sfile, "Ex: 'echo r get_dce_ctl_param 0 > /proc/hdal/vendor/vpe/cmd; cat /proc/hdal/vendor/vpe/cmd' \r\n");
	seq_printf(sfile, "Ex: 'echo r get_gdc_param 0 > /proc/hdal/vendor/vpe/cmd; cat /proc/hdal/vendor/vpe/cmd' \r\n");
	seq_printf(sfile, "Ex: 'echo r get_2dlut_param 0 > /proc/hdal/vendor/vpe/cmd; cat /proc/hdal/vendor/vpe/cmd' \r\n");
	seq_printf(sfile, "Ex: 'echo r get_drt_param 0 > /proc/hdal/vendor/vpe/cmd; cat /proc/hdal/vendor/vpe/cmd' \r\n");
	seq_printf(sfile, "Ex: 'echo r get_sharpen_param 0 > /proc/hdal/vendor/vpe/cmd; cat /proc/hdal/vendor/vpe/cmd' \r\n");
	seq_printf(sfile, "Ex: 'echo w sharpen_en 0 1 > /proc/hdal/vendor/vpe/cmd' \r\n");
	seq_printf(sfile, "Ex: 'echo w edge_sharp_str 0 50 60 > /proc/hdal/vendor/vpe/cmd' \r\n");
	seq_printf(sfile, "Ex: 'echo w flat_sharp_str 0 30 > /proc/hdal/vendor/vpe/cmd' \r\n");
	seq_printf(sfile, "Ex: 'echo w edge_weight_gain 0 190 > /proc/hdal/vendor/vpe/cmd' \r\n");

	return 0;
}

static INT32 vpe_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpe_proc_help_show, PDE_DATA(inode));
}

static const struct file_operations vpe_proc_help_fops = {
	.owner   = THIS_MODULE,
	.open    = vpe_proc_help_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// extern functions
//=============================================================================
INT32 vpe_proc_init(VPE_DRV_INFO *pdrv_info)
{
	INT32 ret = 0;
	struct proc_dir_entry *root = NULL, *pentry = NULL;

	// initialize synchronization mechanism
	#if MUTEX_ENABLE
	sema_init(&mutex, 1);
	#endif

	root = proc_mkdir("hdal/vendor/vpe", NULL);

	if (root == NULL) {
		DBG_ERR("fail to create VPE proc root!! \r\n");
		vpe_proc_remove(pdrv_info);
		return -EINVAL;
	}
	proc_root = root;

	// create "info" entry
	pentry = proc_create_data("info", S_IRUGO | S_IXUGO, root, &vpe_proc_info_fops, (void *)pdrv_info);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc info!! \r\n");
		vpe_proc_remove(pdrv_info);
		return -EINVAL;
	}
	proc_info = pentry;

	// create "command" entry
	pentry = proc_create_data("cmd", S_IRUGO | S_IXUGO, root, &vpe_proc_command_fops, (void *)pdrv_info);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc cmd!! \r\n");
		vpe_proc_remove(pdrv_info);
		return -EINVAL;
	}
	proc_command = pentry;

	// create "help" entry
	pentry = proc_create_data("help", S_IRUGO | S_IXUGO, root, &vpe_proc_help_fops, (void *)pdrv_info);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc help!! \r\n");
		vpe_proc_remove(pdrv_info);
		return -EINVAL;
	}
	proc_help = pentry;

	// allocate memory for massage buffer
	ret = vpe_proc_msg_buf_alloc();
	if (ret < 0) {
		vpe_proc_remove(pdrv_info);
	}

	return ret;
}

void vpe_proc_remove(VPE_DRV_INFO *pdrv_info)
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
	vpe_proc_msg_buf_free();
}

