#include "isp_api.h"
#include "isp_dev.h"
#include "isp_uti.h"

#include "iq_alg.h"
#include "iqt_api.h"

#include "iq_dbg.h"
#include "iq_main.h"
#include "iq_proc.h"

#if defined (CONFIG_NVT_SMALL_HDAL)
#define IQ_PROC_REDUCE 1
#else
#define IQ_PROC_REDUCE 0
#endif

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

static IQ_PROC_MSG_BUF iq_proc_cmd_msg;
static IQ_PROC_R_ITEM iq_proc_r_item = IQ_PROC_R_ITEM_NONE;
static IQ_ID iq_proc_id;
static UINT32 iq_proc_iso;

// NOTE: temporal
extern IQT_CFG_INFO cfg_data;

//=============================================================================
// routines
//=============================================================================
static inline ISP_MODULE *iq_proc_get_mudule_from_file(struct file *file)
{
	return (ISP_MODULE *)((struct seq_file *)file->private_data)->private;
}

//=============================================================================
// proc "info" file operation functions
//=============================================================================
static INT32 iq_proc_info_show(struct seq_file *sfile, void *v)
{
	iq_flow_show_info(sfile);

	return 0;
}

static INT32 iq_proc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, iq_proc_info_show, PDE_DATA(inode));
}

static const struct file_operations iq_proc_info_fops = {
	.owner	 = THIS_MODULE,
	.open	 = iq_proc_info_open,
	.read	 = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "cmd" file operation functions
//=============================================================================
static inline INT32 iq_proc_msg_buf_alloc(void)
{
	iq_proc_cmd_msg.buf = isp_uti_vmem_alloc(PROC_MSG_BUFSIZE);
	if (iq_proc_cmd_msg.buf == NULL) {
		DBG_ERR("fail to allocate IQ message buffer!! \r\n");
		return -E_SYS;
	}

	iq_proc_cmd_msg.size = PROC_MSG_BUFSIZE;
	iq_proc_cmd_msg.count = 0;
	return 0;
}

static inline void iq_proc_msg_buf_free(void)
{
	if (iq_proc_cmd_msg.buf) {
		isp_uti_vmem_free(iq_proc_cmd_msg.buf);
		iq_proc_cmd_msg.buf = NULL;
	}
}

static inline void iq_proc_msg_buf_clean(void)
{
	iq_proc_cmd_msg.buf[0] = '\0';
	iq_proc_cmd_msg.count = 0;
}

static INT32 iq_proc_cmd_printf(const s8 *f, ...)
{
	INT32 len;
	va_list args;

	if (iq_proc_cmd_msg.count < iq_proc_cmd_msg.size) {
		va_start(args, f);
		len = vsnprintf(iq_proc_cmd_msg.buf + iq_proc_cmd_msg.count, iq_proc_cmd_msg.size - iq_proc_cmd_msg.count, f, args);
		va_end(args);

		if ((iq_proc_cmd_msg.count + len) < iq_proc_cmd_msg.size) {
			iq_proc_cmd_msg.count += len;
			return 0;
		}
	}

	iq_proc_cmd_msg.count = iq_proc_cmd_msg.size;
	return -1;
}

#if (!IQ_PROC_REDUCE)
static INT32 iq_proc_cmd_get_buffer_size(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	iq_proc_r_item = IQ_PROC_R_ITEM_BUFFER;
	return 0;
}
#endif

static INT32 iq_proc_cmd_get_param(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 4) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	iq_proc_id = simple_strtoul(argv[2], NULL, 0);
	iq_proc_iso = simple_strtoul(argv[3], NULL, 0);

	if (iq_flow_get_id_valid(iq_proc_id) && (iq_proc_id < IQ_ID_MAX_NUM)) {
		iq_proc_r_item = IQ_PROC_R_ITEM_PARAM;
	} else {
		iq_proc_r_item = IQ_PROC_R_ITEM_NONE;
	}

	return 0;
}

#if (!IQ_PROC_REDUCE)
static INT32 iq_proc_cmd_get_ui_param(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 3) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	iq_proc_id = simple_strtoul(argv[2], NULL, 0);

	if (iq_flow_get_id_valid(iq_proc_id) && (iq_proc_id < IQ_ID_MAX_NUM)) {
		iq_proc_r_item = IQ_PROC_R_ITEM_UI_PARAM;
	} else {
		iq_proc_r_item = IQ_PROC_R_ITEM_NONE;
	}

	return 0;
}
#endif

#if (!IQ_PROC_REDUCE)
static INT32 iq_proc_cmd_get_nnsc_param(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 3) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	iq_proc_id = simple_strtoul(argv[2], NULL, 0);

	if (iq_flow_get_id_valid(iq_proc_id) && (iq_proc_id < IQ_ID_MAX_NUM)) {
		iq_proc_r_item = IQ_PROC_R_ITEM_NNSC_PARAM;
	} else {
		iq_proc_r_item = IQ_PROC_R_ITEM_NONE;
	}

	return 0;
}
#endif

static INT32 iq_proc_cmd_get_cfg_data(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 3) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	iq_proc_id = simple_strtoul(argv[2], NULL, 0);

	if (iq_flow_get_id_valid(iq_proc_id) && (iq_proc_id < IQ_ID_MAX_NUM)) {
		iq_proc_r_item = IQ_PROC_R_ITEM_CFG_DATA;
	} else {
		iq_proc_r_item = IQ_PROC_R_ITEM_NONE;
	}

	return 0;
}


static IQ_PROC_CMD iq_proc_cmd_r_list[] = {
	// keyword              function name
	#if (!IQ_PROC_REDUCE)
	{ "buffer_size",        iq_proc_cmd_get_buffer_size,    "get iq buffer size"},
	#endif
	{ "param",              iq_proc_cmd_get_param,          "get iq param, param1 is iq_id(0~2), param2 is param_id(0~11)"},
	#if (!IQ_PROC_REDUCE)
	{ "ui_param",           iq_proc_cmd_get_ui_param,       "get iq ui param, param1 is iq_id(0~2)"},
	{ "nnsc_param",         iq_proc_cmd_get_nnsc_param,     "get iq nnsc param, param1 is iq_id(0~2)"},
	#endif
	{ "cfg_data",           iq_proc_cmd_get_cfg_data,       "get iq cfg file name, param1 is iq_id(0~2)"},
};
#define NUM_OF_R_CMD (sizeof(iq_proc_cmd_r_list) / sizeof(IQ_PROC_CMD))

// NOTE: Photon mart
#if 0
static INT32 iq_proc_cmd_open(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQ_EVENT_OBJ iq_event_tab = {0};
	IQ_ID id;

	if (argc < 3) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	iq_proc_cmd_printf("iq_proc_cmd_open \n");
	iq_task_id_install();
	iq_event_tab.init =    iq_flow_init;
	iq_event_tab.process = iq_flow_process;
	iq_event_tab.pause =   NULL;
	iq_event_tab.resume =  NULL;

	iq_task_open(id, &iq_event_tab);
	return 0;
}

static INT32 iq_proc_cmd_close(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQ_ID id;

	if (argc < 3) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	iq_proc_cmd_printf("iq_proc_cmd_close \n");
	iq_task_close(id);
	return 0;
}

static INT32 iq_proc_cmd_pause(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQ_ID id;

	if (argc < 3) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	iq_proc_cmd_printf("iq_proc_cmd_pause \n");
	iq_task_pause(id);
	return 0;
}

static INT32 iq_proc_cmd_resume(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQ_ID id;

	if (argc < 3) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	iq_proc_cmd_printf("iq_proc_cmd_resume \n");
	iq_task_resume(id);
	return 0;
}

static INT32 iq_proc_cmd_init(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQ_ID id;

	if (argc < 3) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	iq_proc_cmd_printf("iq_proc_cmd_init \n");
	iq_task_init(id);
	return 0;
}

static INT32 iq_proc_cmd_trig_sie(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	ISP_IQ_TRIG_OBJ iq_trig_obj = {0};
	IQ_ID id;

	if (argc < 3) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	iq_proc_cmd_printf("iq_proc_cmd_trig_sie \n");
	iq_trig_obj.msg = ISP_TRIG_IQ_SIE;
	iq_task_trig(id, &iq_trig_obj);
	return 0;
}

static INT32 iq_proc_cmd_trig_ipp(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	ISP_IQ_TRIG_OBJ iq_trig_obj;
	IQ_ID id;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	iq_trig_obj.ipp.func_en = simple_strtoul(argv[3], NULL, 0); //ISP_FUNC_EN

	iq_proc_cmd_printf("iq_proc_cmd_trig_ipp \n");
	iq_trig_obj.msg = ISP_TRIG_IQ_IPP;
	iq_task_trig(id, &iq_trig_obj);

	return 0;
}
#endif

static INT32 iq_proc_cmd_set_dbg(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQ_ID id;
	u32 cmd;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	cmd = simple_strtoul(argv[3], NULL, 0);

	iq_proc_cmd_printf("set iq(%d) dbg level(0x%x) \n", id, cmd);
	iq_dbg_set_dbg_mode(id, cmd);
	return 0;
}

#if (!IQ_PROC_REDUCE)
static INT32 iq_proc_cmd_set_ui_param(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQ_ID id;
	IQ_UI_ITEM ui_type;
	UINT32 ui_value;

	if (argc < 5) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		iq_proc_cmd_printf("please set (id, ui_type, ui_value) \r\n");
		return -EINVAL;
	}
	id = simple_strtoul(argv[2], NULL, 0);
	ui_type = simple_strtoul(argv[3], NULL, 0);
	ui_value = simple_strtoul(argv[4], NULL, 0);

	iq_proc_cmd_printf("iq_sxcmd_set_ui_param \r\n");
	iq_proc_cmd_printf("id : %d \r\n", id);
	iq_proc_cmd_printf("set ui type : %d, value : %d \r\n", ui_type, ui_value);
	iq_ui_set_info(id, ui_type, ui_value);

	return 0;
}

static INT32 iq_proc_cmd_set_nnsc_param(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQ_ID id;
	IQ_NNSC_ITEM nnsc_type;
	UINT32 nnsc_value;

	if (argc < 5) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		iq_proc_cmd_printf("please set (id, nnsc_type, nnsc_value) \r\n");
		return -EINVAL;
	}
	id = simple_strtoul(argv[2], NULL, 0);
	nnsc_type = simple_strtoul(argv[3], NULL, 0);
	nnsc_value = simple_strtoul(argv[4], NULL, 0);

	iq_proc_cmd_printf("iq_sxcmd_set_nnsc_param \r\n");
	iq_proc_cmd_printf("id : %d \r\n", id);
	iq_proc_cmd_printf("set nnsc type : %d, value : %d \r\n", nnsc_type, nnsc_value);
	iq_nnsc_set_info(id, nnsc_type, nnsc_value);

	return 0;
}
#endif

static INT32 iq_proc_cmd_reload_cfg(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQT_CFG_INFO cfg_info = {0};

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	cfg_info.id = simple_strtoul(argv[2], NULL, 0);
	strncpy(cfg_info.path, argv[3], IQ_CFG_NAME_LENGTH - 1);
	cfg_info.path[IQ_CFG_NAME_LENGTH - 1] = '\0';

	iqt_api_set_cmd(IQT_ITEM_RLD_CONFIG, (UINT32)&cfg_info);

	return 0;
}

static INT32 iq_proc_cmd_reload_dtsi(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQT_DTSI_INFO dtsi_info = {0};

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	dtsi_info.id = simple_strtoul(argv[2], NULL, 0);
	strncpy(dtsi_info.node_path, argv[3], IQ_DTSI_NAME_LENGTH - 1);
	dtsi_info.node_path[IQ_DTSI_NAME_LENGTH - 1] = '\0';
	strncpy(dtsi_info.file_path, argv[4], IQ_DTSI_NAME_LENGTH - 1);
	dtsi_info.file_path[IQ_DTSI_NAME_LENGTH - 1] = '\0';

	iqt_api_set_cmd(IQT_ITEM_RLD_DTSI, (UINT32)&dtsi_info);

	return 0;
}

#if (!IQ_PROC_REDUCE)
static INT32 iq_proc_cmd_set_test_mode(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 en;
	u32 ob_mode;
	u32 dg_mode;
	u32 cg_mode;

	if (argc < 7) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		iq_proc_cmd_printf("param1 = id, range 0~4 \r\n");
		iq_proc_cmd_printf("param2 = en, range 0~1 \r\n");
		iq_proc_cmd_printf("param3 = ob_mode, range 0~2(        %d:SIE, %d:IFE_F, %d:IFE) \r\n", IQ_OB_SIE, IQ_OB_IFE_F, IQ_OB_IFE);
		iq_proc_cmd_printf("param4 = dg_mode, range 0~4(%d:OFF, %d:SIE, %d:IFE_F, %d:IFE, %d:DCE) \r\n", IQ_DG_OFF, IQ_DG_SIE, IQ_DG_IFE_F, IQ_DG_IFE, IQ_DG_DCE);
		iq_proc_cmd_printf("param5 = cg_mode, range 0~3(        %d:SIE, %d:IFE_F, %d:IFE, %d:DCE) \r\n", IQ_CG_SIE, IQ_CG_IFE_F, IQ_CG_IFE, IQ_CG_DCE);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	en = simple_strtoul(argv[3], NULL, 0);
	ob_mode = simple_strtoul(argv[4], NULL, 0);
	dg_mode = simple_strtoul(argv[5], NULL, 0);
	cg_mode = simple_strtoul(argv[6], NULL, 0);

	test_mode_en[id] = en;
	test_mode_ob[id] = ob_mode;
	test_mode_dg[id] = dg_mode;
	test_mode_cg[id] = cg_mode;
	iq_proc_cmd_printf("set iq_info(%d) test_mode(%d) ob_mode(%d) dg_mode(%d) cg_mode(%d) \r\n", id, en, ob_mode, dg_mode, cg_mode);
	return 0;
}

static INT32 iq_proc_cmd_set_companding_en(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 en;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	en = simple_strtoul(argv[3], NULL, 0);

	companding_en[id] = en;
	iq_proc_cmd_printf("set iq_info(%d) companding_en = %d\r\n", id, en);
	return 0;
}

static INT32 iq_proc_cmd_set_fcurve_tmratio(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQ_ID id;
	u32 fcurve_tmratio;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	fcurve_tmratio = simple_strtoul(argv[3], NULL, 0);

	// range : 0~16384, sync from AE. 0 = linear fcurve, 64 = 0EV, 128 = 1EV, 256 = 2EV, 512 = 3EV, 1024 = 4EV
	fcurve_tm_ratio[id] = fcurve_tmratio;
	iq_proc_cmd_printf("set iq_info(%d) fcurve_tmratio = %d\r\n", id, fcurve_tmratio);
	return 0;
}

static INT32 iq_proc_cmd_set_nr_ratio(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQ_ID id;
	u32 r_ratio;
	u32 b_ratio;

	if (argc < 5) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	r_ratio = simple_strtoul(argv[3], NULL, 0);
	b_ratio = simple_strtoul(argv[4], NULL, 0);

	nr_r_ratio[id] = r_ratio;
	nr_b_ratio[id] = b_ratio;
	iq_proc_cmd_printf("set iq_info(%d) nr_r_ratio(%d) nr_b_ratio(%d)\n", id, r_ratio, b_ratio);
	return 0;
}

static INT32 iq_proc_cmd_set_nr_ir_ratio(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQ_ID id;
	u32 ir_ratio;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	ir_ratio = simple_strtoul(argv[3], NULL, 0);

	nr_ir_ratio[id] = ir_ratio;
	iq_proc_cmd_printf("set iq_info(%d) nr_ir_ratio(%d) \n", id, ir_ratio);
	return 0;
}

static INT32 iq_proc_cmd_set_cfa_en(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQ_ID id;
	BOOL cfa_en;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	cfa_en = simple_strtoul(argv[3], NULL, 0);

	iq_flow_ext_set_cfa_en(id, cfa_en);
	return 0;
}

static INT32 iq_proc_cmd_set_wdr_subimg_lpf(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 coef_0;
	u32 coef_1;
	u32 coef_2;

	if (argc < 6) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	coef_0 = simple_strtoul(argv[3], NULL, 0);
	coef_1 = simple_strtoul(argv[4], NULL, 0);
	coef_2 = simple_strtoul(argv[5], NULL, 0);

	iq_flow_ext_set_wdr_subimg_lpf(id, coef_0, coef_1, coef_2);

	return 0;
}

static INT32 iq_proc_cmd_set_wdr_blend_w(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 blend_w;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	blend_w = simple_strtoul(argv[3], NULL, 0);

	iq_flow_ext_set_wdr_blend_w(id, blend_w);

	return 0;
}

static INT32 iq_proc_cmd_set_rgblpf(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQ_ID id;
	BOOL en;
	u32 th0;
	u32 th1;

	if (argc < 6) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	en = simple_strtoul(argv[3], NULL, 0);
	th0 = simple_strtoul(argv[4], NULL, 0);
	th1 = simple_strtoul(argv[5], NULL, 0);

	iq_flow_ext_set_rgblpf(id, en, th0, th1);

	return 0;
}

static INT32 iq_proc_cmd_set_ipe_subimg_size(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 width;
	u32 height;

	if (argc < 5) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	width = simple_strtoul(argv[3], NULL, 0);
	height = simple_strtoul(argv[4], NULL, 0);

	iq_flow_ext_set_ipe_subimg_size(id, width, height);

	return 0;
}

static INT32 iq_proc_cmd_set_edge_gamma_sel(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 sel;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	sel = simple_strtoul(argv[3], NULL, 0);

	iq_flow_ext_set_edge_gamma_sel(id, sel);

	return 0;
}

static INT32 iq_proc_cmd_set_defog_min_diff(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 min_diff;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	min_diff = simple_strtoul(argv[3], NULL, 0);

	iq_flow_ext_set_defog_min_diff(id, min_diff);

	return 0;
}

static INT32 iq_proc_cmd_set_dbcs_mode(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 mode;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		iq_proc_cmd_printf("param1 = id, range 0~13");
		iq_proc_cmd_printf("param2 = mode, range 0~2(0:Dark, 1:Bright, 2:Both)");
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	mode = simple_strtoul(argv[3], NULL, 0);

	iq_flow_ext_set_dbcs_mode(id, mode);

	return 0;
}

static INT32 iq_proc_cmd_set_dbcs_y_wt(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 wt0;
	u32 wt1;
	u32 wt2;
	u32 wt3;
	u32 wt4;
	u32 wt5;
	u32 wt6;
	u32 wt7;
	u32 wt8;
	u32 wt9;
	u32 wt10;
	u32 wt11;
	u32 wt12;
	u32 wt13;
	u32 wt14;
	u32 wt15;

	if (argc < 19) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	wt0 = simple_strtoul(argv[3], NULL, 0);
	wt1 = simple_strtoul(argv[4], NULL, 0);
	wt2 = simple_strtoul(argv[5], NULL, 0);
	wt3 = simple_strtoul(argv[6], NULL, 0);
	wt4 = simple_strtoul(argv[7], NULL, 0);
	wt5 = simple_strtoul(argv[8], NULL, 0);
	wt6 = simple_strtoul(argv[9], NULL, 0);
	wt7 = simple_strtoul(argv[10], NULL, 0);
	wt8 = simple_strtoul(argv[11], NULL, 0);
	wt9 = simple_strtoul(argv[12], NULL, 0);
	wt10 = simple_strtoul(argv[13], NULL, 0);
	wt11 = simple_strtoul(argv[14], NULL, 0);
	wt12 = simple_strtoul(argv[15], NULL, 0);
	wt13 = simple_strtoul(argv[16], NULL, 0);
	wt14 = simple_strtoul(argv[17], NULL, 0);
	wt15 = simple_strtoul(argv[18], NULL, 0);

	iq_flow_ext_set_dbcs_y_wt(id, wt0, wt1, wt2, wt3, wt4, wt5, wt6, wt7, wt8, wt9, wt10, wt11, wt12, wt13, wt14, wt15);

	return 0;
}

static INT32 iq_proc_cmd_set_dbcs_c_wt(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 wt0;
	u32 wt1;
	u32 wt2;
	u32 wt3;
	u32 wt4;
	u32 wt5;
	u32 wt6;
	u32 wt7;
	u32 wt8;
	u32 wt9;
	u32 wt10;
	u32 wt11;
	u32 wt12;
	u32 wt13;
	u32 wt14;
	u32 wt15;

	if (argc < 19) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	wt0 = simple_strtoul(argv[3], NULL, 0);
	wt1 = simple_strtoul(argv[4], NULL, 0);
	wt2 = simple_strtoul(argv[5], NULL, 0);
	wt3 = simple_strtoul(argv[6], NULL, 0);
	wt4 = simple_strtoul(argv[7], NULL, 0);
	wt5 = simple_strtoul(argv[8], NULL, 0);
	wt6 = simple_strtoul(argv[9], NULL, 0);
	wt7 = simple_strtoul(argv[10], NULL, 0);
	wt8 = simple_strtoul(argv[11], NULL, 0);
	wt9 = simple_strtoul(argv[12], NULL, 0);
	wt10 = simple_strtoul(argv[13], NULL, 0);
	wt11 = simple_strtoul(argv[14], NULL, 0);
	wt12 = simple_strtoul(argv[15], NULL, 0);
	wt13 = simple_strtoul(argv[16], NULL, 0);
	wt14 = simple_strtoul(argv[17], NULL, 0);
	wt15 = simple_strtoul(argv[18], NULL, 0);

	iq_flow_ext_set_dbcs_c_wt(id, wt0, wt1, wt2, wt3, wt4, wt5, wt6, wt7, wt8, wt9, wt10, wt11, wt12, wt13, wt14, wt15);

	return 0;
}

static INT32 iq_proc_cmd_set_residue_reset(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	IQ_ID id;
	u32 reset_freq;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	reset_freq = simple_strtoul(argv[3], NULL, 0);

	residue_reset_num[id] = reset_freq;
	iq_proc_cmd_printf("set iq(%d) reset(%d)\n", id, reset_freq);

	return 0;
}

static INT32 iq_proc_cmd_set_tmnr_en(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	u32 en;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	tmnr_en_flag[id] = simple_strtoul(argv[3], NULL, 0);
	en = simple_strtoul(argv[4], NULL, 0);

	tmnr_en[id] = en;
	iq_proc_cmd_printf("set iq_info(%d) tmnr_en = %d\r\n", id, en);

	return 0;
}

static INT32 iq_proc_cmd_set_tmnr_ae_still_en(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	BOOL en;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	en = simple_strtoul(argv[3], NULL, 0);

	tmnr_ae_still_en[id] = en;

	iq_proc_cmd_printf("set iq_info(%d) tmnr_ae_still_en = %d\r\n", id, en);

	return 0;
}

static INT32 iq_proc_cmd_set_tmnr_nr_type(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 id;
	BOOL nr_type;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	nr_type = simple_strtoul(argv[3], NULL, 0);

	iq_flow_ext_set_tmnr_nr_type(id, nr_type);
	return 0;
}

static INT32 iq_proc_cmd_set_wdr(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 dark_th;
	u32 dr_th;
	u32 dark_compensate;

	if (argc < 5) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	dark_th = simple_strtoul(argv[2], NULL, 0);
	dr_th = simple_strtoul(argv[3], NULL, 0);
	dark_compensate = simple_strtoul(argv[4], NULL, 0);

	iq_auto_wdr_dark_th = dark_th;
	iq_auto_wdr_dr_th = dr_th;
	iq_auto_wdr_dark_compensate = dark_compensate;
	iq_proc_cmd_printf("set WDR iq_auto_wdr_dark_th(%d) iq_auto_wdr_dr_th(%d) iq_auto_wdr_dark_compensate(%d)\r\n", dark_th, dr_th, dark_compensate);
	return 0;
}

static INT32 iq_proc_cmd_set_airlight(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 airlight_th;
	u32 airlight_diff_max_ratio;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	airlight_th = simple_strtoul(argv[2], NULL, 0);
	airlight_diff_max_ratio = simple_strtoul(argv[3], NULL, 0);

	iq_auto_defog_airlight_th = airlight_th;
	iq_auto_defog_airlight_diff_max_ratio = airlight_diff_max_ratio;
	iq_proc_cmd_printf("set AIRLIGHT iq_auto_defog_airlight_th(%d) iq_auto_defog_airlight_diff_max(%d)\r\n", airlight_th, airlight_diff_max_ratio);
	return 0;
}

static INT32 iq_proc_cmd_set_defog(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	u32 cnt_ratio_th1;
	u32 cnt_ratio_th2;

	if (argc < 4) {
		iq_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	cnt_ratio_th1 = simple_strtoul(argv[2], NULL, 0);
	cnt_ratio_th2 = simple_strtoul(argv[3], NULL, 0);

	iq_auto_defog_cnt_ratio_th1 = cnt_ratio_th1;
	iq_auto_defog_cnt_ratio_th2 = cnt_ratio_th2;
	iq_proc_cmd_printf("set AIRLIGHT iq_auto_defog_cnt_ratio_th1(%d) iq_auto_defog_cnt_ratio_th2(%d)\r\n", cnt_ratio_th1, cnt_ratio_th2);
	return 0;
}
#endif

static IQ_PROC_CMD iq_proc_cmd_w_list[] = {
	// keyword              function name
	{ "dbg",                iq_proc_cmd_set_dbg,                "set iq dbg level, param1 is iq_id(0~2), param2 is dbg_lv"},
	#if (!IQ_PROC_REDUCE)
	{ "ui_param",           iq_proc_cmd_set_ui_param,           "set iq ui param, param1 is iq_id(0~2), param2 is ui_type, param3 is ui_value"},
	{ "nnsc_param",         iq_proc_cmd_set_nnsc_param,         "set iq nnsc param, param1 is iq_id(0~2), param2 is nnsc_type, param3 is nnsc_value"},
	#endif
	{ "reload_cfg",         iq_proc_cmd_reload_cfg,             "reload cfg file, param1 is iq_id(0~2), param2 is path of cfg file"},
	{ "reload_dtsi",        iq_proc_cmd_reload_dtsi,            "reload dtsi ile, param1 is iq_id(0~2), param2 is node path, param3 is file path"},
	#if (!IQ_PROC_REDUCE)
	// test proc item with id
	{ "mode",               iq_proc_cmd_set_test_mode,          "set iq test mode, param1 is iq_id(0~2), param2 is en, param3 is ob_mode, param4 is dg_mode, param5 is cg_mode"},
	{ "companding",         iq_proc_cmd_set_companding_en,      "set iq companding en, param1 is iq_id(0~2), param2 is companding_en"},
	{ "fcurve_tm",          iq_proc_cmd_set_fcurve_tmratio,     "set iq fcurve tm-ratio, param1 is iq_id(0~2), param2 is tm-ratio(0~1024)"},
	{ "nr",                 iq_proc_cmd_set_nr_ratio,           "set iq nr ratio, param1 is iq_id(0~2), param2 is r_ratio, param3 is b_ratio"},
	{ "nr_ir",              iq_proc_cmd_set_nr_ir_ratio,        "set iq nr_ir ratio, param1 is iq_id(0~2), param2 is ir_ratio"},
	{ "cfa_en",             iq_proc_cmd_set_cfa_en,             "set iq cfa en, param1 is iq_id(0~2), param2 is cfa_en"},
	{ "wdr_lpf",            iq_proc_cmd_set_wdr_subimg_lpf,     "set iq wdr_subimg_lpf, param1 is iq_id(0~2), param2 is coef_0, param3 is coef_1, param4 is coef_2"},
	{ "wdr_blend_w",        iq_proc_cmd_set_wdr_blend_w,        "set iq wdr_blend_w, param1 is iq_id(0~2), param2 is blend_w"},
	{ "rgblpf",             iq_proc_cmd_set_rgblpf,             "set iq rgb lpf, param1 is iq_id(0~2), param2 is en, param3 is th0, param4 is th1"},
	{ "ipe_subimg",         iq_proc_cmd_set_ipe_subimg_size,    "set iq ipe_subimg_size, param1 is iq_id(0~2), param2 is width, param3 is height"},
	{ "edge_gamma_sel",     iq_proc_cmd_set_edge_gamma_sel,     "set iq edge_gamma_sel, param1 is iq_id(0~2), param2 is sel"},
	{ "defog_min_diff",     iq_proc_cmd_set_defog_min_diff,     "set iq defog_min_diff, param1 is iq_id(0~2), param2 is min_diff"},
	{ "dbcs_mode",          iq_proc_cmd_set_dbcs_mode,          "set iq dbcs mode, param1 is iq_id(0~2), param2 is mode"},
	{ "dbcs_y",             iq_proc_cmd_set_dbcs_y_wt,          "set iq dbcs y wt, param1 is iq_id(0~2), param2~17 is y_wt"},
	{ "dbcs_c",             iq_proc_cmd_set_dbcs_c_wt,          "set iq dbcs c wt, param1 is iq_id(0~2), param2~17 is c_wt"},
	{ "tmnr_en",            iq_proc_cmd_set_tmnr_en,            "set iq tmnr en, param1 is iq_id(0~2), param2 is tmnr_en_flag, param3 is tmnr_en"},
	{ "tmnr_ae_en",         iq_proc_cmd_set_tmnr_ae_still_en,   "set iq tmnr ae_en, param1 is iq_id(0~2), param2 is tmnr_ae_still_en"},
	{ "tmnr_reset",         iq_proc_cmd_set_residue_reset,      "set iq tmnr residue reset freq, param1 is iq_id(0~2), param2 is reset_freq(0~60)"},
	{ "tmnr_nr_type",       iq_proc_cmd_set_tmnr_nr_type,       "set iq tmnr nr_type, param1 is iq_id(0~2), param2 is nr_type"},
	// test proc item with non-id
	{ "wdr",                iq_proc_cmd_set_wdr,                "set iq wdr, param1 is dark_th, param2 is dr_th, param3 is dark_compensate"},
	{ "airlight",           iq_proc_cmd_set_airlight,           "set iq airlight, param1 is airlight_th, param2 is airlight_diff_max_ratio"},
	{ "defog",              iq_proc_cmd_set_defog,              "set iq defog, param1 is cnt_ratio_th1, param2 is cnt_ratio_th2"},
	#endif
};
#define NUM_OF_W_CMD (sizeof(iq_proc_cmd_w_list) / sizeof(IQ_PROC_CMD))

static INT32 iq_proc_command_show(struct seq_file *sfile, void *v)
{
	IQ_OB_TUNE_PARAM                 *ob_m, *ob_a;
	IQ_NR_TUNE_PARAM                 *nr_m, *nr_a;
	IQ_CFA_TUNE_PARAM                *cfa_m, *cfa_a;
	IQ_VA_TUNE_PARAM                 *va_m, *va_a;
	IQ_GAMMA_MANUAL_PARAM            *gamma_m;
	IQ_GAMMA_AUTO_PARAM              *gamma_a;
	IQ_CCM_MANUAL_PARAM              *ccm_m;
	IQ_CCM_AUTO_PARAM                *ccm_a;
	IQ_CCM_EXT_MANUAL_PARAM          *ccm_ext_m;
	IQ_CCM_EXT_AUTO_PARAM            *ccm_ext_a;
	IQ_CCM_EXT2_AUTO_PARAM           *ccm_ext2_a;
	IQ_COLOR_TUNE_PARAM              *color_m, *color_a;
	IQ_CONTRAST_TUNE_PARAM           *contrast_m, *contrast_a;
	IQ_EDGE_TUNE_PARAM               *edge_m, *edge_a;
	IQ_3DNR_TUNE_PARAM               *_3dnr_m, *_3dnr_a;
	IQ_PFR_TUNE_PARAM                *pfr_m, *pfr_a;
	IQ_WDR_MANUAL_PARAM              *wdr_m;
	IQ_WDR_AUTO_PARAM                *wdr_a;
	IQ_DEFOG_MANUAL_PARAM            *defog_m;
	IQ_DEFOG_AUTO_PARAM              *defog_a;
	IQ_SHDR_TUNE_PARAM               *shdr_m, *shdr_a;
	IQ_RGBIR_MANUAL_PARAM            *rgbir_m;
	IQ_RGBIR_AUTO_PARAM              *rgbir_a;
	IQ_POST_3DNR_TUNE_PARAM          *post_3dnr_m, *post_3dnr_a;
	IQ_RGBIR_ENH_MANUAL_PARAM        *rgbir_enh_m;
	IQ_RGBIR_ENH_AUTO_PARAM          *rgbir_enh_a;
	IQ_EDGE_REGION_TUNE_PARAM        *edge_region_m, *edge_region_a;
	int align_byte = 4;

	if (iq_proc_cmd_msg.buf == NULL && iq_proc_r_item == IQ_PROC_R_ITEM_NONE) {
		return -EINVAL;
	}

	if (iq_proc_cmd_msg.buf > 0) {
		seq_printf(sfile, "%s\n", iq_proc_cmd_msg.buf);
		iq_proc_msg_buf_clean();
	}

	if (iq_proc_r_item == IQ_PROC_R_ITEM_BUFFER) {
		// TODO:
		#if 0
		seq_printf(sfile, " Buffer size :\n");
		seq_printf(sfile, "     iq_info : %9d \r\n", sizeof(IQALG_INFO));
		seq_printf(sfile, "   final_sie : %9d \r\n", sizeof(IQ_FINAL_SIE));
		seq_printf(sfile, "   final_ipp : %9d \r\n", sizeof(IQ_FINAL_IPP));
		#endif
		seq_printf(sfile, "    iq_param : %9d \r\n"
			, ALIGN_CEIL(sizeof(IQ_OB_PARAM), align_byte)+ ALIGN_CEIL(sizeof(IQ_NR_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_CFA_PARAM), align_byte)+ ALIGN_CEIL(sizeof(IQ_VA_PARAM), align_byte)+ ALIGN_CEIL(sizeof(IQ_GAMMA_PARAM), align_byte) +
			ALIGN_CEIL(sizeof(IQ_CCM_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_COLOR_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_CONTRAST_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_EDGE_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_3DNR_PARAM), align_byte)+
			ALIGN_CEIL(sizeof(IQ_PFR_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_WDR_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_DEFOG_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_SHDR_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_RGBIR_PARAM), align_byte) +
			ALIGN_CEIL(sizeof(IQ_COMPANDING_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_POST_3DNR_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_RGBIR_ENH_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_CCM_EXT_PARAM), align_byte));
		seq_printf(sfile, "          ob : %9d \r\n", ALIGN_CEIL(sizeof(IQ_OB_PARAM), align_byte));
		seq_printf(sfile, "          nr : %9d \r\n", ALIGN_CEIL(sizeof(IQ_NR_PARAM), align_byte));
		seq_printf(sfile, "         cfa : %9d \r\n", ALIGN_CEIL(sizeof(IQ_CFA_PARAM), align_byte));
		seq_printf(sfile, "          va : %9d \r\n", ALIGN_CEIL(sizeof(IQ_VA_PARAM), align_byte));
		seq_printf(sfile, "       gamma : %9d \r\n", ALIGN_CEIL(sizeof(IQ_GAMMA_PARAM), align_byte));
		seq_printf(sfile, "         ccm : %9d \r\n", ALIGN_CEIL(sizeof(IQ_CCM_PARAM), align_byte));
		seq_printf(sfile, "       color : %9d \r\n", ALIGN_CEIL(sizeof(IQ_COLOR_PARAM), align_byte));
		seq_printf(sfile, "    contrast : %9d \r\n", ALIGN_CEIL(sizeof(IQ_CONTRAST_PARAM), align_byte));
		seq_printf(sfile, "        edge : %9d \r\n", ALIGN_CEIL(sizeof(IQ_EDGE_PARAM), align_byte));
		seq_printf(sfile, "        3dnr : %9d \r\n", ALIGN_CEIL(sizeof(IQ_3DNR_PARAM), align_byte));
		seq_printf(sfile, "         pfr : %9d \r\n", ALIGN_CEIL(sizeof(IQ_PFR_PARAM), align_byte));
		seq_printf(sfile, "         wdr : %9d \r\n", ALIGN_CEIL(sizeof(IQ_WDR_PARAM), align_byte));
		seq_printf(sfile, "       defog : %9d \r\n", ALIGN_CEIL(sizeof(IQ_DEFOG_PARAM), align_byte));
		seq_printf(sfile, "        shdr : %9d \r\n", ALIGN_CEIL(sizeof(IQ_SHDR_PARAM), align_byte));
		seq_printf(sfile, "       rgbir : %9d \r\n", ALIGN_CEIL(sizeof(IQ_RGBIR_PARAM), align_byte));
		seq_printf(sfile, "  companding : %9d \r\n", ALIGN_CEIL(sizeof(IQ_COMPANDING_PARAM), align_byte));
		seq_printf(sfile, "   post_3dnr : %9d \r\n", ALIGN_CEIL(sizeof(IQ_POST_3DNR_PARAM), align_byte));
		seq_printf(sfile, "   rgbir_eng : %9d \r\n", ALIGN_CEIL(sizeof(IQ_RGBIR_ENH_PARAM), align_byte));
		seq_printf(sfile, "     ccm_ext : %9d \r\n", ALIGN_CEIL(sizeof(IQ_CCM_EXT_PARAM), align_byte));
	}

	if (iq_proc_r_item == IQ_PROC_R_ITEM_PARAM) {
		seq_printf(sfile, " get iq(%d) param_id(%d) param\n", iq_proc_id, iq_proc_iso);
		seq_printf(sfile, " \r\n");

		// iq_ob_param
		seq_printf(sfile, "====================id(%d) iq_ob_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->ob->mode);
		ob_m = &iq_param[iq_proc_id]->ob->manual_param;
		seq_printf(sfile, "manual_param.cofs = %d, %d, %d, %d, %d, \r\n"
			, ob_m->cofs[0], ob_m->cofs[1], ob_m->cofs[2], ob_m->cofs[3], ob_m->cofs[4]);
		ob_a = &iq_param[iq_proc_id]->ob->auto_param[iq_proc_iso];
		seq_printf(sfile, "auto_param[%d].cofs = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, ob_a->cofs[0], ob_a->cofs[1], ob_a->cofs[2], ob_a->cofs[3], ob_a->cofs[4]);

		// iq_nr_param
		seq_printf(sfile, "====================id(%d) iq_nr_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "outl_enable = %d, \r\n", iq_param[iq_proc_id]->nr->outl_enable);
		seq_printf(sfile, "gbal_enable = %d, \r\n", iq_param[iq_proc_id]->nr->gbal_enable);
		seq_printf(sfile, "filter_enable = %d, \r\n", iq_param[iq_proc_id]->nr->filter_enable);
		seq_printf(sfile, "lca_enable = %d, \r\n", iq_param[iq_proc_id]->nr->lca_enable);
		seq_printf(sfile, "dbcs_enable = %d, \r\n", iq_param[iq_proc_id]->nr->dbcs_enable);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->nr->mode);
		seq_printf(sfile, "outl_avg_mode = %d, \r\n", iq_param[iq_proc_id]->nr->outl_avg_mode);
		seq_printf(sfile, "outl_ord_protect_th = %d, \r\n", iq_param[iq_proc_id]->nr->outl_ord_protect_th);
		seq_printf(sfile, "outl_ord_blend_w = %d, \r\n", iq_param[iq_proc_id]->nr->outl_ord_blend_w);
		nr_m = &iq_param[iq_proc_id]->nr->manual_param;
		seq_printf(sfile, "manual_param.outl_sel = %d, \r\n", nr_m->outl_sel);
		seq_printf(sfile, "manual_param.outl_bright_th = %d, %d, %d, %d, %d, \r\n"
			, nr_m->outl_bright_th[0], nr_m->outl_bright_th[1], nr_m->outl_bright_th[2], nr_m->outl_bright_th[3], nr_m->outl_bright_th[4]);
		seq_printf(sfile, "manual_param.outl_bright_th = %d, %d, %d, %d, %d, \r\n"
			, nr_m->outl_dark_th[0], nr_m->outl_dark_th[1], nr_m->outl_dark_th[2], nr_m->outl_dark_th[3], nr_m->outl_dark_th[4]);
		seq_printf(sfile, "manual_param.gbal_diff_th_str = %d, \r\n", nr_m->gbal_diff_th_str);
		seq_printf(sfile, "manual_param.gbal_edge_protect_th = %d, \r\n", nr_m->gbal_edge_protect_th);
		seq_printf(sfile, "manual_param.filter_th = %d, %d, %d, %d, %d, \r\n"
			, nr_m->filter_th[0], nr_m->filter_th[1], nr_m->filter_th[2], nr_m->filter_th[3], nr_m->filter_th[4]);
		seq_printf(sfile, "manual_param.filter_lut = %d, %d, %d, %d, %d, \r\n"
			, nr_m->filter_lut[0], nr_m->filter_lut[1], nr_m->filter_lut[2], nr_m->filter_lut[3], nr_m->filter_lut[4]);
		seq_printf(sfile, "manual_param.filter_blend_w = %d, \r\n", nr_m->filter_blend_w);
		seq_printf(sfile, "manual_param.filter_clamp_th = %d, \r\n", nr_m->filter_clamp_th);
		seq_printf(sfile, "manual_param.filter_clamp_mul = %d, \r\n", nr_m->filter_clamp_mul);
		seq_printf(sfile, "manual_param.lca_sub_center_filter_y = %d, \r\n", nr_m->lca_sub_center_filter_y);
		seq_printf(sfile, "manual_param.lca_sub_center_filter_c = %d, \r\n", nr_m->lca_sub_center_filter_c);
		seq_printf(sfile, "manual_param.lca_sub_filter_y = %d, \r\n", nr_m->lca_sub_filter_y);
		seq_printf(sfile, "manual_param.lca_sub_filter_c = %d, \r\n", nr_m->lca_sub_filter_c);
		seq_printf(sfile, "manual_param.lca_out_y_wt = %d, \r\n", nr_m->lca_out_y_wt);
		seq_printf(sfile, "manual_param.lca_out_c_wt = %d, \r\n", nr_m->lca_out_c_wt);
		seq_printf(sfile, "manual_param.dbcs_step_y = %d, \r\n", nr_m->dbcs_step_y);
		seq_printf(sfile, "manual_param.dbcs_step_c = %d, \r\n", nr_m->dbcs_step_c);
		nr_a = &iq_param[iq_proc_id]->nr->auto_param[iq_proc_iso];
		seq_printf(sfile, "auto_param[%d].outl_sel = %d, \r\n", iq_proc_iso, nr_m->outl_sel);
		seq_printf(sfile, "auto_param[%d].outl_bright_th = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, nr_a->outl_bright_th[0], nr_a->outl_bright_th[1], nr_a->outl_bright_th[2], nr_a->outl_bright_th[3], nr_a->outl_bright_th[4]);
		seq_printf(sfile, "auto_param[%d].outl_bright_th = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, nr_a->outl_dark_th[0], nr_a->outl_dark_th[1], nr_a->outl_dark_th[2], nr_a->outl_dark_th[3], nr_a->outl_dark_th[4]);
		seq_printf(sfile, "auto_param[%d].gbal_diff_th_str = %d, \r\n", iq_proc_iso, nr_a->gbal_diff_th_str);
		seq_printf(sfile, "auto_param[%d].gbal_edge_protect_th = %d, \r\n", iq_proc_iso, nr_a->gbal_edge_protect_th);
		seq_printf(sfile, "auto_param[%d].filter_th = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, nr_a->filter_th[0], nr_a->filter_th[1], nr_a->filter_th[2], nr_a->filter_th[3], nr_a->filter_th[4]);
		seq_printf(sfile, "auto_param[%d].filter_lut = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, nr_a->filter_lut[0], nr_a->filter_lut[1], nr_a->filter_lut[2], nr_a->filter_lut[3], nr_a->filter_lut[4]);
		seq_printf(sfile, "auto_param[%d].filter_blend_w = %d, \r\n", iq_proc_iso, nr_a->filter_blend_w);
		seq_printf(sfile, "auto_param[%d].filter_clamp_th = %d, \r\n", iq_proc_iso, nr_a->filter_clamp_th);
		seq_printf(sfile, "auto_param[%d].filter_clamp_mul = %d, \r\n", iq_proc_iso, nr_a->filter_clamp_mul);
		seq_printf(sfile, "auto_param[%d].lca_sub_center_filter_y = %d, \r\n", iq_proc_iso, nr_a->lca_sub_center_filter_y);
		seq_printf(sfile, "auto_param[%d].lca_sub_center_filter_c = %d, \r\n", iq_proc_iso, nr_a->lca_sub_center_filter_c);
		seq_printf(sfile, "auto_param[%d].lca_sub_filter_y = %d, \r\n", iq_proc_iso, nr_a->lca_sub_filter_y);
		seq_printf(sfile, "auto_param[%d].lca_sub_filter_c = %d, \r\n", iq_proc_iso, nr_a->lca_sub_filter_c);
		seq_printf(sfile, "auto_param[%d].lca_out_y_wt = %d, \r\n", iq_proc_iso, nr_a->lca_out_y_wt);
		seq_printf(sfile, "auto_param[%d].lca_out_c_wt = %d, \r\n", iq_proc_iso, nr_a->lca_out_c_wt);
		seq_printf(sfile, "auto_param[%d].dbcs_step_y = %d, \r\n", iq_proc_iso, nr_a->dbcs_step_y);
		seq_printf(sfile, "auto_param[%d].dbcs_step_c = %d, \r\n", iq_proc_iso, nr_a->dbcs_step_c);

		// iq_cfa_param
		seq_printf(sfile, "====================id(%d) iq_cfa_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->cfa->mode);
		cfa_m = &iq_param[iq_proc_id]->cfa->manual_param;
		seq_printf(sfile, "manual_param.edge_dth = %d, \r\n", cfa_m->edge_dth);
		seq_printf(sfile, "manual_param.edge_dth2 = %d, \r\n", cfa_m->edge_dth2);
		seq_printf(sfile, "manual_param.freq_th = %d, \r\n", cfa_m->freq_th);
		seq_printf(sfile, "manual_param.fcs_weight = %d, \r\n", cfa_m->fcs_weight);
		cfa_a = &iq_param[iq_proc_id]->cfa->auto_param[iq_proc_iso];
		seq_printf(sfile, "auto_param[%d].edge_dth = %d, \r\n", iq_proc_iso, cfa_a->edge_dth);
		seq_printf(sfile, "auto_param[%d].edge_dth2 = %d, \r\n", iq_proc_iso, cfa_a->edge_dth2);
		seq_printf(sfile, "auto_param[%d].freq_th = %d, \r\n", iq_proc_iso, cfa_a->freq_th);
		seq_printf(sfile, "auto_param[%d].fcs_weight = %d, \r\n", iq_proc_iso, cfa_a->fcs_weight);

		// iq_va_param
		seq_printf(sfile, "====================id(%d) iq_va_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->va->mode);
		seq_printf(sfile, "indep_roi[0] = %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->va->indep_roi[0].x, iq_param[iq_proc_id]->va->indep_roi[0].y
			, iq_param[iq_proc_id]->va->indep_roi[0].w, iq_param[iq_proc_id]->va->indep_roi[0].h);
		seq_printf(sfile, "indep_roi[1] = %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->va->indep_roi[1].x, iq_param[iq_proc_id]->va->indep_roi[1].y
			, iq_param[iq_proc_id]->va->indep_roi[1].w, iq_param[iq_proc_id]->va->indep_roi[1].h);
		seq_printf(sfile, "indep_roi[2] = %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->va->indep_roi[2].x, iq_param[iq_proc_id]->va->indep_roi[2].y
			, iq_param[iq_proc_id]->va->indep_roi[2].w, iq_param[iq_proc_id]->va->indep_roi[2].h);
		seq_printf(sfile, "indep_roi[3] = %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->va->indep_roi[3].x, iq_param[iq_proc_id]->va->indep_roi[3].y
			, iq_param[iq_proc_id]->va->indep_roi[3].w, iq_param[iq_proc_id]->va->indep_roi[3].h);
		seq_printf(sfile, "indep_roi[4] = %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->va->indep_roi[4].x, iq_param[iq_proc_id]->va->indep_roi[4].y
			, iq_param[iq_proc_id]->va->indep_roi[4].w, iq_param[iq_proc_id]->va->indep_roi[4].h);
		seq_printf(sfile, "g1_tap_a = %d, \r\n", iq_param[iq_proc_id]->va->g1_tap_a);
		seq_printf(sfile, "g1_tap_b = %d, \r\n", iq_param[iq_proc_id]->va->g1_tap_b);
		seq_printf(sfile, "g1_tap_c = %d, \r\n", iq_param[iq_proc_id]->va->g1_tap_c);
		seq_printf(sfile, "g1_tap_d = %d, \r\n", iq_param[iq_proc_id]->va->g1_tap_d);
		seq_printf(sfile, "g1_div = %d, \r\n", iq_param[iq_proc_id]->va->g1_div);
		seq_printf(sfile, "g2_tap_a = %d, \r\n", iq_param[iq_proc_id]->va->g2_tap_a);
		seq_printf(sfile, "g2_tap_b = %d, \r\n", iq_param[iq_proc_id]->va->g2_tap_b);
		seq_printf(sfile, "g2_tap_c = %d, \r\n", iq_param[iq_proc_id]->va->g2_tap_c);
		seq_printf(sfile, "g2_tap_d = %d, \r\n", iq_param[iq_proc_id]->va->g2_tap_d);
		seq_printf(sfile, "g2_div = %d, \r\n", iq_param[iq_proc_id]->va->g2_div);
		va_m = &iq_param[iq_proc_id]->va->manual_param;
		seq_printf(sfile, "manual_param.g1_th_l = %d, \r\n", va_m->g1_th_l);
		seq_printf(sfile, "manual_param.g1_th_u = %d, \r\n", va_m->g1_th_u);
		seq_printf(sfile, "manual_param.g2_th_l = %d, \r\n", va_m->g2_th_l);
		seq_printf(sfile, "manual_param.g2_th_u = %d, \r\n", va_m->g2_th_u);
		va_a = &iq_param[iq_proc_id]->va->auto_param[iq_proc_iso];
		seq_printf(sfile, "auto_param[%d].g1_th_l = %d, \r\n", iq_proc_iso, va_a->g1_th_l);
		seq_printf(sfile, "auto_param[%d].g1_th_u = %d, \r\n", iq_proc_iso, va_a->g1_th_u);
		seq_printf(sfile, "auto_param[%d].g2_th_l = %d, \r\n", iq_proc_iso, va_a->g2_th_l);
		seq_printf(sfile, "auto_param[%d].g2_th_u = %d, \r\n", iq_proc_iso, va_a->g2_th_u);

		// iq_gamma_param
		seq_printf(sfile, "====================id(%d) iq_gamma_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "tone_enable = %d, \r\n", iq_param[iq_proc_id]->gamma->tone_enable);
		seq_printf(sfile, "gamma_enable = %d, \r\n", iq_param[iq_proc_id]->gamma->gamma_enable);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->gamma->mode);
		seq_printf(sfile, "tone_input_blend_lut = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma->tone_input_blend_lut[0], iq_param[iq_proc_id]->gamma->tone_input_blend_lut[1]
			, iq_param[iq_proc_id]->gamma->tone_input_blend_lut[2], iq_param[iq_proc_id]->gamma->tone_input_blend_lut[3]
			, iq_param[iq_proc_id]->gamma->tone_input_blend_lut[4]);
		seq_printf(sfile, "tone_lut_linear_idx = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma->tone_lut_linear_idx[0], iq_param[iq_proc_id]->gamma->tone_lut_linear_idx[1]
			, iq_param[iq_proc_id]->gamma->tone_lut_linear_idx[2], iq_param[iq_proc_id]->gamma->tone_lut_linear_idx[3]
			, iq_param[iq_proc_id]->gamma->tone_lut_linear_idx[4]);
		seq_printf(sfile, "tone_lut_linear_split = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma->tone_lut_linear_split[0], iq_param[iq_proc_id]->gamma->tone_lut_linear_split[1]
			, iq_param[iq_proc_id]->gamma->tone_lut_linear_split[2], iq_param[iq_proc_id]->gamma->tone_lut_linear_split[3]
			, iq_param[iq_proc_id]->gamma->tone_lut_linear_split[4]);
		seq_printf(sfile, "tone_lut_linear_val = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma->tone_lut_linear_val[0], iq_param[iq_proc_id]->gamma->tone_lut_linear_val[1]
			, iq_param[iq_proc_id]->gamma->tone_lut_linear_val[2], iq_param[iq_proc_id]->gamma->tone_lut_linear_val[3]
			, iq_param[iq_proc_id]->gamma->tone_lut_linear_val[4]);
		seq_printf(sfile, "tone_lut_shdr_idx = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_idx[0], iq_param[iq_proc_id]->gamma->tone_lut_shdr_idx[1]
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_idx[2], iq_param[iq_proc_id]->gamma->tone_lut_shdr_idx[3]
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_idx[4]);
		seq_printf(sfile, "tone_lut_shdr_split = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_split[0], iq_param[iq_proc_id]->gamma->tone_lut_shdr_split[1]
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_split[2], iq_param[iq_proc_id]->gamma->tone_lut_shdr_split[3]
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_split[4]);
		seq_printf(sfile, "tone_lut_shdr_set_num = %d, \r\n", iq_param[iq_proc_id]->gamma->tone_lut_shdr_set_num);
		seq_printf(sfile, "tone_lut_shdr_set0_lv = %d, \r\n", iq_param[iq_proc_id]->gamma->tone_lut_shdr_set0_lv);
		seq_printf(sfile, "tone_lut_shdr_set0_val = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set0_val[0], iq_param[iq_proc_id]->gamma->tone_lut_shdr_set0_val[1]
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set0_val[2], iq_param[iq_proc_id]->gamma->tone_lut_shdr_set0_val[3]
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set0_val[4]);
		seq_printf(sfile, "tone_lut_shdr_set1_lv = %d, \r\n", iq_param[iq_proc_id]->gamma->tone_lut_shdr_set1_lv);
		seq_printf(sfile, "tone_lut_shdr_set1_val = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set1_val[0], iq_param[iq_proc_id]->gamma->tone_lut_shdr_set1_val[1]
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set1_val[2], iq_param[iq_proc_id]->gamma->tone_lut_shdr_set1_val[3]
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set1_val[4]);
		seq_printf(sfile, "tone_lut_shdr_set2_lv = %d, \r\n", iq_param[iq_proc_id]->gamma->tone_lut_shdr_set2_lv);
		seq_printf(sfile, "tone_lut_shdr_set2_val = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set3_val[0], iq_param[iq_proc_id]->gamma->tone_lut_shdr_set2_val[1]
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set3_val[2], iq_param[iq_proc_id]->gamma->tone_lut_shdr_set2_val[3]
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set3_val[4]);
		seq_printf(sfile, "tone_lut_shdr_set3_lv = %d, \r\n", iq_param[iq_proc_id]->gamma->tone_lut_shdr_set3_lv);
		seq_printf(sfile, "tone_lut_shdr_set3_val = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set3_val[0], iq_param[iq_proc_id]->gamma->tone_lut_shdr_set3_val[1]
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set3_val[2], iq_param[iq_proc_id]->gamma->tone_lut_shdr_set3_val[3]
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set3_val[4]);
		seq_printf(sfile, "tone_lut_shdr_set4_lv = %d, \r\n", iq_param[iq_proc_id]->gamma->tone_lut_shdr_set4_lv);
		seq_printf(sfile, "tone_lut_shdr_set4_val = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set4_val[0], iq_param[iq_proc_id]->gamma->tone_lut_shdr_set4_val[1]
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set4_val[2], iq_param[iq_proc_id]->gamma->tone_lut_shdr_set4_val[3]
			, iq_param[iq_proc_id]->gamma->tone_lut_shdr_set4_val[4]);
		gamma_m = &iq_param[iq_proc_id]->gamma->manual_param;
		seq_printf(sfile, "manual_param.gamma_lut = %d, %d, %d, %d, %d, \r\n"
			, gamma_m->gamma_lut[0], gamma_m->gamma_lut[1], gamma_m->gamma_lut[2], gamma_m->gamma_lut[3], gamma_m->gamma_lut[4]);
		if (iq_proc_iso < IQ_GAMMA_ID_MAX_NUM) {
			gamma_a = &iq_param[iq_proc_id]->gamma->auto_param[iq_proc_iso];
			seq_printf(sfile, "auto_param[%d].lv = %d, \r\n", iq_proc_iso, gamma_a->lv);
			seq_printf(sfile, "auto_param[%d].gamma_lut = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
				, gamma_a->gamma_lut[0], gamma_a->gamma_lut[1], gamma_a->gamma_lut[2], gamma_a->gamma_lut[3], gamma_a->gamma_lut[4]);
		}

		// iq_gamma_ext_param
		seq_printf(sfile, "====================id(%d) iq_gamma_ext_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "ext_sel = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->ext_sel);
		seq_printf(sfile, "gamma_set0_lv = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->gamma_set0_lv);
		seq_printf(sfile, "gamma_set0_lut = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma_ext->gamma_set0_lut[0], iq_param[iq_proc_id]->gamma_ext->gamma_set0_lut[1]
			, iq_param[iq_proc_id]->gamma_ext->gamma_set0_lut[2], iq_param[iq_proc_id]->gamma_ext->gamma_set0_lut[3]
			, iq_param[iq_proc_id]->gamma_ext->gamma_set0_lut[4]);
		seq_printf(sfile, "gamma_set1_lv = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->gamma_set1_lv);
		seq_printf(sfile, "gamma_set1_lut = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma_ext->gamma_set1_lut[0], iq_param[iq_proc_id]->gamma_ext->gamma_set1_lut[1]
			, iq_param[iq_proc_id]->gamma_ext->gamma_set1_lut[2], iq_param[iq_proc_id]->gamma_ext->gamma_set1_lut[3]
			, iq_param[iq_proc_id]->gamma_ext->gamma_set1_lut[4]);
		seq_printf(sfile, "gamma_set2_lv = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->gamma_set2_lv);
		seq_printf(sfile, "gamma_set2_lut = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma_ext->gamma_set2_lut[0], iq_param[iq_proc_id]->gamma_ext->gamma_set2_lut[1]
			, iq_param[iq_proc_id]->gamma_ext->gamma_set2_lut[2], iq_param[iq_proc_id]->gamma_ext->gamma_set2_lut[3]
			, iq_param[iq_proc_id]->gamma_ext->gamma_set2_lut[4]);
		seq_printf(sfile, "gamma_set3_lv = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->gamma_set3_lv);
		seq_printf(sfile, "gamma_set3_lut = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma_ext->gamma_set3_lut[0], iq_param[iq_proc_id]->gamma_ext->gamma_set3_lut[1]
			, iq_param[iq_proc_id]->gamma_ext->gamma_set3_lut[2], iq_param[iq_proc_id]->gamma_ext->gamma_set3_lut[3]
			, iq_param[iq_proc_id]->gamma_ext->gamma_set3_lut[4]);
		seq_printf(sfile, "gamma_set4_lv = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->gamma_set4_lv);
		seq_printf(sfile, "gamma_set4_lut = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->gamma_ext->gamma_set4_lut[0], iq_param[iq_proc_id]->gamma_ext->gamma_set4_lut[1]
			, iq_param[iq_proc_id]->gamma_ext->gamma_set4_lut[2], iq_param[iq_proc_id]->gamma_ext->gamma_set4_lut[3]
			, iq_param[iq_proc_id]->gamma_ext->gamma_set4_lut[4]);
		seq_printf(sfile, "auto_param[0].lv = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->auto_param[0].lv);
		seq_printf(sfile, "auto_param[0].gamma_level = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->auto_param[0].gamma_level);
		seq_printf(sfile, "auto_param[1].lv = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->auto_param[1].lv);
		seq_printf(sfile, "auto_param[1].gamma_level = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->auto_param[1].gamma_level);
		seq_printf(sfile, "auto_param[2].lv = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->auto_param[2].lv);
		seq_printf(sfile, "auto_param[2].gamma_level = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->auto_param[2].gamma_level);
		seq_printf(sfile, "auto_param[3].lv = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->auto_param[3].lv);
		seq_printf(sfile, "auto_param[3].gamma_level = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->auto_param[3].gamma_level);
		seq_printf(sfile, "auto_param[4].lv = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->auto_param[4].lv);
		seq_printf(sfile, "auto_param[4].gamma_level = %d, \r\n", iq_param[iq_proc_id]->gamma_ext->auto_param[4].gamma_level);

		// iq_ccm_param
		seq_printf(sfile, "====================id(%d) iq_ccm_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->ccm->mode);
		ccm_m = &iq_param[iq_proc_id]->ccm->manual_param;
		seq_printf(sfile, "manual_param.coef = %d, %d, %d, %d, %d, %d, %d, %d, %d, \r\n"
			, ccm_m->coef[0], ccm_m->coef[1], ccm_m->coef[2], ccm_m->coef[3], ccm_m->coef[4]
			, ccm_m->coef[5], ccm_m->coef[6], ccm_m->coef[7], ccm_m->coef[8]);
		seq_printf(sfile, "manual_param.hue_tab = %d, %d, %d, %d, %d, \r\n"
			, ccm_m->hue_tab[0], ccm_m->hue_tab[1], ccm_m->hue_tab[2], ccm_m->hue_tab[3], ccm_m->hue_tab[4]);
		seq_printf(sfile, "manual_param.sat_tab = %d, %d, %d, %d, %d, \r\n"
			, ccm_m->sat_tab[0], ccm_m->sat_tab[1], ccm_m->sat_tab[2], ccm_m->sat_tab[3], ccm_m->sat_tab[4]);
		if (iq_proc_iso < IQ_COLOR_ID_MAX_NUM) {
			ccm_a = &iq_param[iq_proc_id]->ccm->auto_param[iq_proc_iso];
			seq_printf(sfile, "auto_param[%d].ct = %d, \r\n", iq_proc_iso, ccm_a->ct);
			seq_printf(sfile, "auto_param[%d].coef = %d, %d, %d, %d, %d, %d, %d, %d, %d, \r\n", iq_proc_iso
				, ccm_a->coef[0], ccm_a->coef[1], ccm_a->coef[2], ccm_a->coef[3], ccm_a->coef[4]
				, ccm_a->coef[5], ccm_a->coef[6], ccm_a->coef[7], ccm_a->coef[8]);
			seq_printf(sfile, "auto_param[%d].hue_tab = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
				, ccm_a->hue_tab[0], ccm_a->hue_tab[1], ccm_a->hue_tab[2], ccm_a->hue_tab[3], ccm_a->hue_tab[4]);
			seq_printf(sfile, "auto_param[%d].sat_tab = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
				, ccm_a->sat_tab[0], ccm_a->sat_tab[1], ccm_a->sat_tab[2], ccm_a->sat_tab[3], ccm_a->sat_tab[4]);
		}

		// iq_ccm_ext_param
		seq_printf(sfile, "====================id(%d) iq_ccm_ext_param==================== \r\n", iq_proc_id);
		ccm_ext_m = &iq_param[iq_proc_id]->ccm_ext->manual_param;
		seq_printf(sfile, "manual_param.int_tab = %d, %d, %d, %d, %d, \r\n"
			, ccm_ext_m->int_tab[0], ccm_ext_m->int_tab[1], ccm_ext_m->int_tab[2], ccm_ext_m->int_tab[3], ccm_ext_m->int_tab[4]);
		if (iq_proc_iso < IQ_COLOR_ID_MAX_NUM) {
			ccm_ext_a = &iq_param[iq_proc_id]->ccm_ext->auto_param[iq_proc_iso];
			seq_printf(sfile, "auto_param[%d].int_tab = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
				, ccm_ext_a->int_tab[0], ccm_ext_a->int_tab[1], ccm_ext_a->int_tab[2], ccm_ext_a->int_tab[3], ccm_ext_a->int_tab[4]);
		}

		// iq_ccm_ext2_param
		seq_printf(sfile, "====================id(%d) iq_ccm_ext2_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "enable = %d, \r\n", iq_param[iq_proc_id]->ccm_ext2->enable);
		if (iq_proc_iso < IQ_COLOR_EXT2_ID_MAX_NUM) {
			ccm_ext2_a = &iq_param[iq_proc_id]->ccm_ext2->auto_param[iq_proc_iso];
			seq_printf(sfile, "auto_param[%d].ct = %d, \r\n", iq_proc_iso, ccm_ext2_a->ct);
			seq_printf(sfile, "auto_param[%d].coef = %d, %d, %d, %d, %d, %d, %d, %d, %d, \r\n", iq_proc_iso
				, ccm_ext2_a->coef[0], ccm_ext2_a->coef[1], ccm_ext2_a->coef[2], ccm_ext2_a->coef[3], ccm_ext2_a->coef[4]
				, ccm_ext2_a->coef[5], ccm_ext2_a->coef[6], ccm_ext2_a->coef[7], ccm_ext2_a->coef[8]);
			seq_printf(sfile, "auto_param[%d].hue_tab = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
				, ccm_ext2_a->hue_tab[0], ccm_ext2_a->hue_tab[1], ccm_ext2_a->hue_tab[2], ccm_ext2_a->hue_tab[3], ccm_ext2_a->hue_tab[4]);
			seq_printf(sfile, "auto_param[%d].sat_tab = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
				, ccm_ext2_a->sat_tab[0], ccm_ext2_a->sat_tab[1], ccm_ext2_a->sat_tab[2], ccm_ext2_a->sat_tab[3], ccm_ext2_a->sat_tab[4]);
			seq_printf(sfile, "auto_param[%d].int_tab = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
				, ccm_ext2_a->int_tab[0], ccm_ext2_a->int_tab[1], ccm_ext2_a->int_tab[2], ccm_ext2_a->int_tab[3], ccm_ext2_a->int_tab[4]);
		}

		// iq_color_param
		seq_printf(sfile, "====================id(%d) iq_color_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->color->mode);
		color_m = &iq_param[iq_proc_id]->color->manual_param;
		seq_printf(sfile, "manual_param.c_con = %d, \r\n", color_m->c_con);
		seq_printf(sfile, "manual_param.fstab = %d, %d, %d, %d, %d, \r\n"
			, color_m->fstab[0], color_m->fstab[1], color_m->fstab[2], color_m->fstab[3], color_m->fstab[4]);
		seq_printf(sfile, "manual_param.fdtab = %d, %d, %d, %d, %d, \r\n"
			, color_m->fdtab[0], color_m->fdtab[1], color_m->fdtab[2], color_m->fdtab[3], color_m->fdtab[4]);
		seq_printf(sfile, "manual_param.cconlut = %d, %d, %d, %d, %d, \r\n"
			, color_m->cconlut[0], color_m->cconlut[1], color_m->cconlut[2], color_m->cconlut[3], color_m->cconlut[4]);
		color_a = &iq_param[iq_proc_id]->color->auto_param[iq_proc_iso];
		seq_printf(sfile, "auto_param[%d].c_con = %d, \r\n", iq_proc_iso, color_a->c_con);
		seq_printf(sfile, "auto_param[%d].fstab = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, color_a->fstab[0], color_a->fstab[1], color_a->fstab[2], color_a->fstab[3], color_a->fstab[4]);
		seq_printf(sfile, "auto_param[%d].fstab = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, color_a->fdtab[0], color_a->fdtab[1], color_a->fdtab[2], color_a->fdtab[3], color_a->fdtab[4]);
		seq_printf(sfile, "auto_param[%d].fstab = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, color_a->cconlut[0], color_a->cconlut[1], color_a->cconlut[2], color_a->cconlut[3], color_a->cconlut[4]);

		// iq_contrast_param
		seq_printf(sfile, "====================id(%d) iq_contrast_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->contrast->mode);
		contrast_m = &iq_param[iq_proc_id]->contrast->manual_param;
		seq_printf(sfile, "manual_param.y_con = %d, \r\n", contrast_m->y_con);
		seq_printf(sfile, "manual_param.lce_lum_wt_lut = %d, %d, %d, %d, %d, \r\n"
			, contrast_m->lce_lum_wt_lut[0], contrast_m->lce_lum_wt_lut[1], contrast_m->lce_lum_wt_lut[2], contrast_m->lce_lum_wt_lut[3], contrast_m->lce_lum_wt_lut[4]);
		contrast_a = &iq_param[iq_proc_id]->contrast->auto_param[iq_proc_iso];
		seq_printf(sfile, "auto_param[%d].y_con = %d, \r\n", iq_proc_iso, contrast_a->y_con);
		seq_printf(sfile, "auto_param[%d].lce_lum_wt_lut = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, contrast_a->lce_lum_wt_lut[0], contrast_a->lce_lum_wt_lut[1], contrast_a->lce_lum_wt_lut[2], contrast_a->lce_lum_wt_lut[3], contrast_a->lce_lum_wt_lut[4]);

		// iq_edge_param
		seq_printf(sfile, "====================id(%d) iq_edge_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "enable = %d, \r\n", iq_param[iq_proc_id]->edge->enable);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->edge->mode);
		seq_printf(sfile, "th_overshoot = %d, \r\n", iq_param[iq_proc_id]->edge->th_overshoot);
		seq_printf(sfile, "th_undershoot = %d, \r\n", iq_param[iq_proc_id]->edge->th_undershoot);
		seq_printf(sfile, "edge_map_lut = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->edge->edge_map_lut[0], iq_param[iq_proc_id]->edge->edge_map_lut[1]
			, iq_param[iq_proc_id]->edge->edge_map_lut[2], iq_param[iq_proc_id]->edge->edge_map_lut[3]
			, iq_param[iq_proc_id]->edge->edge_map_lut[4]);
		seq_printf(sfile, "es_map_lut = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->edge->es_map_lut[0], iq_param[iq_proc_id]->edge->es_map_lut[1]
			, iq_param[iq_proc_id]->edge->es_map_lut[2], iq_param[iq_proc_id]->edge->es_map_lut[3]
			, iq_param[iq_proc_id]->edge->es_map_lut[4]);
		seq_printf(sfile, "edge_tab = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->edge->edge_tab[0], iq_param[iq_proc_id]->edge->edge_tab[1]
			, iq_param[iq_proc_id]->edge->edge_tab[2], iq_param[iq_proc_id]->edge->edge_tab[3]
			, iq_param[iq_proc_id]->edge->edge_tab[4]);
		seq_printf(sfile, "reduce_ratio = %d, \r\n", iq_param[iq_proc_id]->edge->reduce_ratio);
		edge_m = &iq_param[iq_proc_id]->edge->manual_param;
		seq_printf(sfile, "manual_param.edge_enh_p = %d, \r\n", edge_m->edge_enh_p);
		seq_printf(sfile, "manual_param.edge_enh_n = %d, \r\n", edge_m->edge_enh_n);
		seq_printf(sfile, "manual_param.thin_freq = %d, \r\n", edge_m->thin_freq);
		seq_printf(sfile, "manual_param.robust_freq = %d, \r\n", edge_m->robust_freq);
		seq_printf(sfile, "manual_param.wt_low = %d, \r\n", edge_m->wt_low);
		seq_printf(sfile, "manual_param.wt_high = %d, \r\n", edge_m->wt_high);
		seq_printf(sfile, "manual_param.th_flat = %d, \r\n", edge_m->th_flat);
		seq_printf(sfile, "manual_param.th_edge = %d, \r\n", edge_m->th_edge);
		seq_printf(sfile, "manual_param.overshoot_str = %d, \r\n", edge_m->overshoot_str);
		seq_printf(sfile, "manual_param.undershoot_str = %d, \r\n", edge_m->undershoot_str);
		seq_printf(sfile, "manual_param.edge_ethr_low = %d, \r\n", edge_m->edge_ethr_low);
		seq_printf(sfile, "manual_param.edge_ethr_high = %d, \r\n", edge_m->edge_ethr_high);
		seq_printf(sfile, "manual_param.edge_etab_low = %d, \r\n", edge_m->edge_etab_low);
		seq_printf(sfile, "manual_param.edge_etab_high = %d, \r\n", edge_m->edge_etab_high);
		seq_printf(sfile, "manual_param.es_ethr_low = %d, \r\n", edge_m->es_ethr_low);
		seq_printf(sfile, "manual_param.es_ethr_high = %d, \r\n", edge_m->es_ethr_high);
		seq_printf(sfile, "manual_param.es_etab_low = %d, \r\n", edge_m->es_etab_low);
		seq_printf(sfile, "manual_param.es_etab_high = %d, \r\n", edge_m->es_etab_high);
		edge_a = &iq_param[iq_proc_id]->edge->auto_param[iq_proc_iso];
		seq_printf(sfile, "auto_param[%d].edge_enh_p = %d, \r\n", iq_proc_iso, edge_a->edge_enh_p);
		seq_printf(sfile, "auto_param[%d].edge_enh_n = %d, \r\n", iq_proc_iso, edge_a->edge_enh_n);
		seq_printf(sfile, "auto_param[%d].thin_freq = %d, \r\n", iq_proc_iso, edge_a->thin_freq);
		seq_printf(sfile, "auto_param[%d].robust_freq = %d, \r\n", iq_proc_iso, edge_a->robust_freq);
		seq_printf(sfile, "auto_param[%d].wt_low = %d, \r\n", iq_proc_iso, edge_a->wt_low);
		seq_printf(sfile, "auto_param[%d].wt_high = %d, \r\n", iq_proc_iso, edge_a->wt_high);
		seq_printf(sfile, "auto_param[%d].th_flat = %d, \r\n", iq_proc_iso, edge_a->th_flat);
		seq_printf(sfile, "auto_param[%d].th_edge = %d, \r\n", iq_proc_iso, edge_a->th_edge);
		seq_printf(sfile, "auto_param[%d].overshoot_str = %d, \r\n", iq_proc_iso, edge_a->overshoot_str);
		seq_printf(sfile, "auto_param[%d].undershoot_str = %d, \r\n", iq_proc_iso, edge_a->undershoot_str);
		seq_printf(sfile, "auto_param[%d].edge_ethr_low = %d, \r\n", iq_proc_iso, edge_a->edge_ethr_low);
		seq_printf(sfile, "auto_param[%d].edge_ethr_high = %d, \r\n", iq_proc_iso, edge_a->edge_ethr_high);
		seq_printf(sfile, "auto_param[%d].edge_etab_low = %d, \r\n", iq_proc_iso, edge_a->edge_etab_low);
		seq_printf(sfile, "auto_param[%d].edge_etab_high = %d, \r\n", iq_proc_iso, edge_a->edge_etab_high);
		seq_printf(sfile, "auto_param[%d].es_ethr_low = %d, \r\n", iq_proc_iso, edge_a->es_ethr_low);
		seq_printf(sfile, "auto_param[%d].es_ethr_high = %d, \r\n", iq_proc_iso, edge_a->es_ethr_high);
		seq_printf(sfile, "auto_param[%d].es_etab_low = %d, \r\n", iq_proc_iso, edge_a->es_etab_low);
		seq_printf(sfile, "auto_param[%d].es_etab_high = %d, \r\n", iq_proc_iso, edge_a->es_etab_high);

		// iq_3dnr_param
		seq_printf(sfile, "====================id(%d) iq_3dnr_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "enable = %d, \r\n", iq_param[iq_proc_id]->_3dnr->enable);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->_3dnr->mode);
		_3dnr_m = &iq_param[iq_proc_id]->_3dnr->manual_param;
		seq_printf(sfile, "manual_param.sad_penalty = %d, %d, %d, %d, %d, \r\n"
			, _3dnr_m->sad_penalty[0], _3dnr_m->sad_penalty[1], _3dnr_m->sad_penalty[2], _3dnr_m->sad_penalty[3], _3dnr_m->sad_penalty[4]);
		seq_printf(sfile, "manual_param.detail_penalty = %d, %d, %d, %d, %d, \r\n"
			, _3dnr_m->detail_penalty[0], _3dnr_m->detail_penalty[1], _3dnr_m->detail_penalty[2], _3dnr_m->detail_penalty[3], _3dnr_m->detail_penalty[4]);
		seq_printf(sfile, "manual_param.switch_th = %d, %d, %d, %d, %d, \r\n"
			, _3dnr_m->switch_th[0], _3dnr_m->switch_th[1], _3dnr_m->switch_th[2], _3dnr_m->switch_th[3], _3dnr_m->switch_th[4]);
		seq_printf(sfile, "manual_param.switch_rto = %d, \r\n", _3dnr_m->switch_rto);
		seq_printf(sfile, "manual_param.probability = %d, \r\n", _3dnr_m->probability);
		seq_printf(sfile, "manual_param.sad_base = %d, %d, %d, %d, %d, \r\n"
			, _3dnr_m->sad_base[0], _3dnr_m->sad_base[1], _3dnr_m->sad_base[2], _3dnr_m->sad_base[3], _3dnr_m->sad_base[4]);
		seq_printf(sfile, "manual_param.sad_coefa = %d, %d, %d, %d, %d, \r\n"
			, _3dnr_m->sad_coefa[0], _3dnr_m->sad_coefa[1], _3dnr_m->sad_coefa[2], _3dnr_m->sad_coefa[3], _3dnr_m->sad_coefa[4]);
		seq_printf(sfile, "manual_param.sad_coefb = %d, %d, %d, %d, %d, \r\n"
			, _3dnr_m->sad_coefb[0], _3dnr_m->sad_coefb[1], _3dnr_m->sad_coefb[2], _3dnr_m->sad_coefb[3], _3dnr_m->sad_coefb[4]);
		seq_printf(sfile, "manual_param.sad_std = %d, %d, %d, %d, %d, \r\n"
			, _3dnr_m->sad_std[0], _3dnr_m->sad_std[1], _3dnr_m->sad_std[2], _3dnr_m->sad_std[3], _3dnr_m->sad_std[4]);
		seq_printf(sfile, "manual_param.fth = %d, %d, \r\n", _3dnr_m->fth[0], _3dnr_m->fth[1]);
		seq_printf(sfile, "manual_param.mv_th = %d, \r\n", _3dnr_m->mv_th);
		seq_printf(sfile, "manual_param.mix_ratio = %d, %d, \r\n", _3dnr_m->mix_ratio[0], _3dnr_m->mix_ratio[1]);
		seq_printf(sfile, "manual_param.ds_th = %d, \r\n", _3dnr_m->ds_th);
		seq_printf(sfile, "manual_param.luma_residue_th = %d, \r\n", _3dnr_m->luma_residue_th);
		seq_printf(sfile, "manual_param.chroma_residue_th = %d, \r\n", _3dnr_m->chroma_residue_th);
		seq_printf(sfile, "manual_param.pre_filter_str = %d, %d, %d, %d\r\n"
			, _3dnr_m->pre_filter_str[0], _3dnr_m->pre_filter_str[1], _3dnr_m->pre_filter_str[2], _3dnr_m->pre_filter_str[3]);
		seq_printf(sfile, "manual_param.pre_filter_rto = %d, %d, \r\n", _3dnr_m->pre_filter_rto[0], _3dnr_m->pre_filter_rto[1]);
		seq_printf(sfile, "manual_param.snr_base_th = %d, \r\n", _3dnr_m->snr_base_th);
		seq_printf(sfile, "manual_param.tnr_base_th = %d, \r\n", _3dnr_m->tnr_base_th);
		seq_printf(sfile, "manual_param.freq_wet = %d, %d, %d, %d\r\n"
			, _3dnr_m->freq_wet[0], _3dnr_m->freq_wet[1], _3dnr_m->freq_wet[2], _3dnr_m->freq_wet[3]);
		seq_printf(sfile, "manual_param.luma_wet = %d, %d, %d, %d, %d, \r\n"
			, _3dnr_m->luma_wet[0], _3dnr_m->luma_wet[1], _3dnr_m->luma_wet[2], _3dnr_m->luma_wet[3], _3dnr_m->luma_wet[4]);
		seq_printf(sfile, "manual_param.snr_str = %d, %d, %d\r\n"
			, _3dnr_m->snr_str[0], _3dnr_m->snr_str[1], _3dnr_m->snr_str[2]);
		seq_printf(sfile, "manual_param.tnr_str = %d, %d, %d\r\n"
			, _3dnr_m->tnr_str[0], _3dnr_m->tnr_str[1], _3dnr_m->tnr_str[2]);
		seq_printf(sfile, "manual_param.luma_3d_th = %d, %d, %d, %d\r\n"
			, _3dnr_m->luma_3d_th[0], _3dnr_m->luma_3d_th[1], _3dnr_m->luma_3d_th[2], _3dnr_m->luma_3d_th[3]);
		seq_printf(sfile, "manual_param.luma_3d_lut = %d, %d, %d, %d, %d, \r\n"
			, _3dnr_m->luma_3d_lut[0], _3dnr_m->luma_3d_lut[1], _3dnr_m->luma_3d_lut[2], _3dnr_m->luma_3d_lut[3], _3dnr_m->luma_3d_lut[4]);
		seq_printf(sfile, "manual_param.chroma_3d_lut = %d, %d, %d, %d, %d, \r\n"
			, _3dnr_m->chroma_3d_lut[0], _3dnr_m->chroma_3d_lut[1], _3dnr_m->chroma_3d_lut[2], _3dnr_m->chroma_3d_lut[3], _3dnr_m->chroma_3d_lut[4]);
		seq_printf(sfile, "manual_param.chroma_3d_rto = %d, %d, \r\n", _3dnr_m->chroma_3d_rto[0], _3dnr_m->chroma_3d_rto[1]);
		_3dnr_a = &iq_param[iq_proc_id]->_3dnr->auto_param[iq_proc_iso];
		seq_printf(sfile, "auto_param[%d].sad_penalty = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, _3dnr_a->sad_penalty[0], _3dnr_a->sad_penalty[1], _3dnr_a->sad_penalty[2], _3dnr_a->sad_penalty[3], _3dnr_a->sad_penalty[4]);
		seq_printf(sfile, "auto_param[%d].detail_penalty = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, _3dnr_a->detail_penalty[0], _3dnr_a->detail_penalty[1], _3dnr_a->detail_penalty[2], _3dnr_a->detail_penalty[3], _3dnr_a->detail_penalty[4]);
		seq_printf(sfile, "auto_param[%d].switch_th = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, _3dnr_a->switch_th[0], _3dnr_a->switch_th[1], _3dnr_a->switch_th[2], _3dnr_a->switch_th[3], _3dnr_a->switch_th[4]);
		seq_printf(sfile, "auto_param[%d].switch_rto = %d, \r\n", iq_proc_iso, _3dnr_a->switch_rto);
		seq_printf(sfile, "auto_param[%d].probability = %d, \r\n", iq_proc_iso, _3dnr_a->probability);
		seq_printf(sfile, "auto_param[%d].sad_base = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, _3dnr_a->sad_base[0], _3dnr_a->sad_base[1], _3dnr_a->sad_base[2], _3dnr_a->sad_base[3], _3dnr_a->sad_base[4]);
		seq_printf(sfile, "auto_param[%d].sad_coefa = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, _3dnr_a->sad_coefa[0], _3dnr_a->sad_coefa[1], _3dnr_a->sad_coefa[2], _3dnr_a->sad_coefa[3], _3dnr_a->sad_coefa[4]);
		seq_printf(sfile, "auto_param[%d].sad_coefb = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, _3dnr_a->sad_coefb[0], _3dnr_a->sad_coefb[1], _3dnr_a->sad_coefb[2], _3dnr_a->sad_coefb[3], _3dnr_a->sad_coefb[4]);
		seq_printf(sfile, "auto_param[%d].sad_std = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, _3dnr_a->sad_std[0], _3dnr_a->sad_std[1], _3dnr_a->sad_std[2], _3dnr_a->sad_std[3], _3dnr_a->sad_std[4]);
		seq_printf(sfile, "auto_param[%d].fth = %d, %d, \r\n", iq_proc_iso, _3dnr_a->fth[0], _3dnr_a->fth[1]);
		seq_printf(sfile, "auto_param[%d].mv_th = %d, \r\n", iq_proc_iso, _3dnr_a->mv_th);
		seq_printf(sfile, "auto_param[%d].mix_ratio = %d, %d, \r\n", iq_proc_iso, _3dnr_a->mix_ratio[0], _3dnr_a->mix_ratio[1]);
		seq_printf(sfile, "auto_param[%d].ds_th = %d, \r\n", iq_proc_iso, _3dnr_a->ds_th);
		seq_printf(sfile, "auto_param[%d].luma_residue_th = %d, \r\n", iq_proc_iso, _3dnr_a->luma_residue_th);
		seq_printf(sfile, "auto_param[%d].chroma_residue_th = %d, \r\n", iq_proc_iso, _3dnr_a->chroma_residue_th);
		seq_printf(sfile, "auto_param[%d].pre_filter_str = %d, %d, %d, %d\r\n", iq_proc_iso
			, _3dnr_a->pre_filter_str[0], _3dnr_a->pre_filter_str[1], _3dnr_a->pre_filter_str[2], _3dnr_a->pre_filter_str[3]);
		seq_printf(sfile, "auto_param[%d].pre_filter_rto = %d, %d, \r\n", iq_proc_iso, _3dnr_a->pre_filter_rto[0], _3dnr_a->pre_filter_rto[1]);
		seq_printf(sfile, "auto_param[%d].snr_base_th = %d, \r\n", iq_proc_iso, _3dnr_a->snr_base_th);
		seq_printf(sfile, "auto_param[%d].tnr_base_th = %d, \r\n", iq_proc_iso, _3dnr_a->tnr_base_th);
		seq_printf(sfile, "auto_param[%d].freq_wet = %d, %d, %d, %d\r\n", iq_proc_iso
			, _3dnr_a->freq_wet[0], _3dnr_a->freq_wet[1], _3dnr_a->freq_wet[2], _3dnr_a->freq_wet[3]);
		seq_printf(sfile, "auto_param[%d].luma_wet = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, _3dnr_a->luma_wet[0], _3dnr_a->luma_wet[1], _3dnr_a->luma_wet[2], _3dnr_a->luma_wet[3], _3dnr_a->luma_wet[4]);
		seq_printf(sfile, "auto_param[%d].snr_str = %d, %d, %d\r\n", iq_proc_iso
			, _3dnr_a->snr_str[0], _3dnr_a->snr_str[1], _3dnr_a->snr_str[2]);
		seq_printf(sfile, "auto_param[%d].tnr_str = %d, %d, %d\r\n", iq_proc_iso
			, _3dnr_a->tnr_str[0], _3dnr_a->tnr_str[1], _3dnr_a->tnr_str[2]);
		seq_printf(sfile, "auto_param[%d].luma_3d_th = %d, %d, %d, %d\r\n", iq_proc_iso
			, _3dnr_a->luma_3d_th[0], _3dnr_a->luma_3d_th[1], _3dnr_a->luma_3d_th[2], _3dnr_a->luma_3d_th[3]);
		seq_printf(sfile, "auto_param[%d].luma_3d_lut = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, _3dnr_a->luma_3d_lut[0], _3dnr_a->luma_3d_lut[1], _3dnr_a->luma_3d_lut[2], _3dnr_a->luma_3d_lut[3], _3dnr_a->luma_3d_lut[4]);
		seq_printf(sfile, "auto_param[%d].chroma_3d_lut = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, _3dnr_a->chroma_3d_lut[0], _3dnr_a->chroma_3d_lut[1], _3dnr_a->chroma_3d_lut[2], _3dnr_a->chroma_3d_lut[3], _3dnr_a->chroma_3d_lut[4]);
		seq_printf(sfile, "auto_param[%d].chroma_3d_rto = %d, %d, \r\n", iq_proc_iso, _3dnr_a->chroma_3d_rto[0], _3dnr_a->chroma_3d_rto[1]);

		// iq_pfr_param
		seq_printf(sfile, "====================id(%d) iq_pfr_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "enable = %d, \r\n", iq_param[iq_proc_id]->pfr->enable);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->pfr->mode);
		seq_printf(sfile, "luma_lut = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->pfr->luma_lut[0], iq_param[iq_proc_id]->pfr->luma_lut[1]
			, iq_param[iq_proc_id]->pfr->luma_lut[2], iq_param[iq_proc_id]->pfr->luma_lut[3]
			, iq_param[iq_proc_id]->pfr->luma_lut[4]);
		seq_printf(sfile, "set0_color_u = %d, \r\n", iq_param[iq_proc_id]->pfr->set0_color_u);
		seq_printf(sfile, "set0_color_v = %d, \r\n", iq_param[iq_proc_id]->pfr->set0_color_v);
		seq_printf(sfile, "set1_en = %d, \r\n", iq_param[iq_proc_id]->pfr->set1_en);
		seq_printf(sfile, "set1_color_u = %d, \r\n", iq_param[iq_proc_id]->pfr->set1_color_u);
		seq_printf(sfile, "set1_color_v = %d, \r\n", iq_param[iq_proc_id]->pfr->set1_color_v);
		pfr_m = &iq_param[iq_proc_id]->pfr->manual_param;
		seq_printf(sfile, "manual_param.pfr_strength = %d, \r\n", pfr_m->pfr_strength);
		seq_printf(sfile, "manual_param.luma_th = %d, \r\n", pfr_m->luma_th);
		pfr_a = &iq_param[iq_proc_id]->pfr->auto_param[iq_proc_iso];
		seq_printf(sfile, "auto_param[%d].pfr_strength = %d, \r\n", iq_proc_iso, pfr_a->pfr_strength);
		seq_printf(sfile, "auto_param[%d].luma_th = %d, \r\n", iq_proc_iso, pfr_a->luma_th);

		// iq_wdr_param
		seq_printf(sfile, "====================id(%d) iq_wdr_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "enable = %d, \r\n", iq_param[iq_proc_id]->wdr->enable);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->wdr->mode);
		seq_printf(sfile, "subimg_size_h = %d, \r\n", iq_param[iq_proc_id]->wdr->subimg_size_h);
		seq_printf(sfile, "subimg_size_v = %d, \r\n", iq_param[iq_proc_id]->wdr->subimg_size_v);
		seq_printf(sfile, "contrast = %d, \r\n", iq_param[iq_proc_id]->wdr->contrast);
		seq_printf(sfile, "max_gain = %d, \r\n", iq_param[iq_proc_id]->wdr->max_gain);
		seq_printf(sfile, "min_gain = %d, \r\n", iq_param[iq_proc_id]->wdr->min_gain);
		seq_printf(sfile, "lut_idx = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->wdr->lut_idx[0], iq_param[iq_proc_id]->wdr->lut_idx[1]
			, iq_param[iq_proc_id]->wdr->lut_idx[2], iq_param[iq_proc_id]->wdr->lut_idx[3]
			, iq_param[iq_proc_id]->wdr->lut_idx[4]);
		seq_printf(sfile, "lut_split = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->wdr->lut_split[0], iq_param[iq_proc_id]->wdr->lut_split[1]
			, iq_param[iq_proc_id]->wdr->lut_split[2], iq_param[iq_proc_id]->wdr->lut_split[3]
			, iq_param[iq_proc_id]->wdr->lut_split[4]);
		seq_printf(sfile, "lut_val = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->wdr->lut_val[0], iq_param[iq_proc_id]->wdr->lut_val[1]
			, iq_param[iq_proc_id]->wdr->lut_val[2], iq_param[iq_proc_id]->wdr->lut_val[3]
			, iq_param[iq_proc_id]->wdr->lut_val[4]);
		wdr_m = &iq_param[iq_proc_id]->wdr->manual_param;
		seq_printf(sfile, "manual_param.strength = %d, \r\n", wdr_m->strength);
		wdr_a = &iq_param[iq_proc_id]->wdr->auto_param[iq_proc_iso];
		seq_printf(sfile, "auto_param[%d].strength = %d, \r\n", iq_proc_iso, wdr_a->level);
		seq_printf(sfile, "auto_param[%d].strength_min = %d, \r\n", iq_proc_iso, wdr_a->strength_min);
		seq_printf(sfile, "auto_param[%d].strength_max = %d, \r\n", iq_proc_iso, wdr_a->strength_max);

		// iq_defog_param
		seq_printf(sfile, "====================id(%d) iq_defog_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "enable = %d, \r\n", iq_param[iq_proc_id]->defog->enable);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->defog->mode);
		seq_printf(sfile, "outbld_local_en = %d, \r\n", iq_param[iq_proc_id]->defog->outbld_local_en);
		seq_printf(sfile, "outbld_local_en = %d, \r\n", iq_param[iq_proc_id]->defog->outbld_local_en);
		defog_m = &iq_param[iq_proc_id]->defog->manual_param;
		seq_printf(sfile, "manual_param.fog_mod_level = %d, \r\n", defog_m->fog_level);
		seq_printf(sfile, "manual_param.fog_ratio = %d, \r\n", defog_m->fog_ratio);
		seq_printf(sfile, "manual_param.gain_th = %d, \r\n", defog_m->gain_th);
		seq_printf(sfile, "manual_param.outbld_lum_wt = %d, %d, %d, %d, %d, \r\n"
			, defog_m->outbld_lum_wt[0], defog_m->outbld_lum_wt[1], defog_m->outbld_lum_wt[2], defog_m->outbld_lum_wt[3], defog_m->outbld_lum_wt[4]);
		defog_a = &iq_param[iq_proc_id]->defog->auto_param[iq_proc_iso];
		seq_printf(sfile, "auto_param[%d].dr_th = %d, \r\n", iq_proc_iso, defog_a->dr_th);
		seq_printf(sfile, "auto_param[%d].fog_mod_level_lb = %d, \r\n", iq_proc_iso, defog_a->fog_level_max);
		seq_printf(sfile, "auto_param[%d].fog_ratio = %d, \r\n", iq_proc_iso, defog_a->fog_ratio);
		seq_printf(sfile, "auto_param[%d].outbld_lum_wt_str = %d, \r\n", iq_proc_iso, defog_a->outbld_wt);

		// iq_shdr_param
		seq_printf(sfile, "====================id(%d) iq_shdr_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "nrs_enable = %d, \r\n", iq_param[iq_proc_id]->shdr->nrs_enable);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->shdr->mode);
		seq_printf(sfile, "fusion_ev_ratio = %d, \r\n", iq_param[iq_proc_id]->shdr->fusion_ev_ratio);
		seq_printf(sfile, "fusion_nor_sel = %d, \r\n", iq_param[iq_proc_id]->shdr->fusion_nor_sel);
		seq_printf(sfile, "fusion_l_nor_knee = %d, \r\n", iq_param[iq_proc_id]->shdr->fusion_l_nor_knee);
		seq_printf(sfile, "fusion_l_nor_range = %d, \r\n", iq_param[iq_proc_id]->shdr->fusion_l_nor_range);
		seq_printf(sfile, "fusion_s_nor_knee = %d, \r\n", iq_param[iq_proc_id]->shdr->fusion_s_nor_knee);
		seq_printf(sfile, "fusion_s_nor_range = %d, \r\n", iq_param[iq_proc_id]->shdr->fusion_s_nor_range);
		seq_printf(sfile, "fusion_dif_sel = %d, \r\n", iq_param[iq_proc_id]->shdr->fusion_dif_sel);
		seq_printf(sfile, "fusion_l_dif_knee = %d, \r\n", iq_param[iq_proc_id]->shdr->fusion_l_dif_knee);
		seq_printf(sfile, "fusion_l_dif_range = %d, \r\n", iq_param[iq_proc_id]->shdr->fusion_l_dif_range);
		seq_printf(sfile, "fusion_s_dif_knee = %d, \r\n", iq_param[iq_proc_id]->shdr->fusion_s_dif_knee);
		seq_printf(sfile, "fusion_s_dif_range = %d, \r\n", iq_param[iq_proc_id]->shdr->fusion_s_dif_range);
		seq_printf(sfile, "fusion_lum_th = %d, \r\n", iq_param[iq_proc_id]->shdr->fusion_lum_th);
		seq_printf(sfile, "fusion_diff_w = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->shdr->fusion_diff_w[0], iq_param[iq_proc_id]->shdr->fusion_diff_w[1]
			, iq_param[iq_proc_id]->shdr->fusion_diff_w[2], iq_param[iq_proc_id]->shdr->fusion_diff_w[3]
			, iq_param[iq_proc_id]->shdr->fusion_diff_w[4]);
		seq_printf(sfile, "fcurve_y_mean_sel = %d, \r\n", iq_param[iq_proc_id]->shdr->fcurve_y_mean_sel);
		seq_printf(sfile, "fcurve_yv_w = %d, \r\n", iq_param[iq_proc_id]->shdr->fcurve_yv_w);
		seq_printf(sfile, "fcurve_idx_lut = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->shdr->fcurve_idx_lut[0], iq_param[iq_proc_id]->shdr->fcurve_idx_lut[1]
			, iq_param[iq_proc_id]->shdr->fcurve_idx_lut[2], iq_param[iq_proc_id]->shdr->fcurve_idx_lut[3]
			, iq_param[iq_proc_id]->shdr->fcurve_idx_lut[4]);
		seq_printf(sfile, "fcurve_split_lut = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->shdr->fcurve_split_lut[0], iq_param[iq_proc_id]->shdr->fcurve_split_lut[1]
			, iq_param[iq_proc_id]->shdr->fcurve_split_lut[2], iq_param[iq_proc_id]->shdr->fcurve_split_lut[3]
			, iq_param[iq_proc_id]->shdr->fcurve_split_lut[4]);
		seq_printf(sfile, "fcurve_val_lut = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->shdr->fcurve_val_lut[0], iq_param[iq_proc_id]->shdr->fcurve_val_lut[1]
			, iq_param[iq_proc_id]->shdr->fcurve_val_lut[2], iq_param[iq_proc_id]->shdr->fcurve_val_lut[3]
			, iq_param[iq_proc_id]->shdr->fcurve_val_lut[4]);
		shdr_m = &iq_param[iq_proc_id]->shdr->manual_param;
		seq_printf(sfile, "manual_param.nrs_diff_th = %d, \r\n", shdr_m->nrs_diff_th);
		seq_printf(sfile, "manual_param.nrs_ofs = %d, %d, %d, %d, %d, %d, \r\n"
			, shdr_m->nrs_ofs[0], shdr_m->nrs_ofs[1], shdr_m->nrs_ofs[2]
			, shdr_m->nrs_ofs[3], shdr_m->nrs_ofs[4], shdr_m->nrs_ofs[5]);
		seq_printf(sfile, "manual_param.nrs_ofs = %d, %d, %d, %d, %d, \r\n"
			, shdr_m->nrs_th[0], shdr_m->nrs_th[1], shdr_m->nrs_th[2], shdr_m->nrs_th[3], shdr_m->nrs_th[4]);
		shdr_a = &iq_param[iq_proc_id]->shdr->auto_param[iq_proc_iso];
		seq_printf(sfile, "auto_param[%d].nrs_diff_th = %d, \r\n", iq_proc_iso, shdr_a->nrs_diff_th);
		seq_printf(sfile, "auto_param[%d].nrs_ofs = %d, %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, shdr_a->nrs_ofs[0], shdr_a->nrs_ofs[1], shdr_a->nrs_ofs[2]
			, shdr_a->nrs_ofs[3], shdr_a->nrs_ofs[4], shdr_a->nrs_ofs[5]);
		seq_printf(sfile, "auto_param[%d].nrs_ofs = %d, %d, %d, %d, %d, \r\n", iq_proc_iso
			, shdr_a->nrs_th[0], shdr_a->nrs_th[1], shdr_a->nrs_th[2], shdr_a->nrs_th[3], shdr_a->nrs_th[4]);

		// iq_rgbir_param
		seq_printf(sfile, "====================id(%d) iq_rgbir_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->rgbir->mode);
		rgbir_m = &iq_param[iq_proc_id]->rgbir->manual_param;
		seq_printf(sfile, "manual_param.irsub_r_weight = %d, \r\n", rgbir_m->irsub_r_weight);
		seq_printf(sfile, "manual_param.irsub_g_weight = %d, \r\n", rgbir_m->irsub_g_weight);
		seq_printf(sfile, "manual_param.irsub_b_weight = %d, \r\n", rgbir_m->irsub_b_weight);
		seq_printf(sfile, "manual_param.ir_sat_gain = %d, \r\n", rgbir_m->ir_sat_gain);
		rgbir_a = &iq_param[iq_proc_id]->rgbir->auto_param;
		seq_printf(sfile, "auto_param.irsub_r_weight = %d, \r\n", rgbir_a->irsub_r_weight);
		seq_printf(sfile, "auto_param.irsub_g_weight = %d, \r\n", rgbir_a->irsub_g_weight);
		seq_printf(sfile, "auto_param.irsub_b_weight = %d, \r\n", rgbir_a->irsub_b_weight);
		seq_printf(sfile, "auto_param.irsub_reduce_th = %d, \r\n", rgbir_a->irsub_reduce_th);
		seq_printf(sfile, "auto_param.night_mode_th = %d, \r\n", rgbir_a->night_mode_th);

		// iq_companding_param
		seq_printf(sfile, "====================id(%d) iq_companding_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "decomp_knee_pts = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->companding->decomp_knee_pts[0], iq_param[iq_proc_id]->companding->decomp_knee_pts[1]
			, iq_param[iq_proc_id]->companding->decomp_knee_pts[2], iq_param[iq_proc_id]->companding->decomp_knee_pts[3]
			, iq_param[iq_proc_id]->companding->decomp_knee_pts[4]);
		seq_printf(sfile, "decomp_str_pts = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->companding->decomp_str_pts[0], iq_param[iq_proc_id]->companding->decomp_str_pts[1]
			, iq_param[iq_proc_id]->companding->decomp_str_pts[2], iq_param[iq_proc_id]->companding->decomp_str_pts[3]
			, iq_param[iq_proc_id]->companding->decomp_str_pts[4]);
		seq_printf(sfile, "decomp_shift_pts = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->companding->decomp_shift_pts[0], iq_param[iq_proc_id]->companding->decomp_shift_pts[1]
			, iq_param[iq_proc_id]->companding->decomp_shift_pts[2], iq_param[iq_proc_id]->companding->decomp_shift_pts[3]
			, iq_param[iq_proc_id]->companding->decomp_shift_pts[4]);
		seq_printf(sfile, "comp_knee_pts = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->companding->comp_knee_pts[0], iq_param[iq_proc_id]->companding->comp_knee_pts[1]
			, iq_param[iq_proc_id]->companding->comp_knee_pts[2], iq_param[iq_proc_id]->companding->comp_knee_pts[3]
			, iq_param[iq_proc_id]->companding->comp_knee_pts[4]);
		seq_printf(sfile, "comp_str_pts = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->companding->comp_str_pts[0], iq_param[iq_proc_id]->companding->comp_str_pts[1]
			, iq_param[iq_proc_id]->companding->comp_str_pts[2], iq_param[iq_proc_id]->companding->comp_str_pts[3]
			, iq_param[iq_proc_id]->companding->comp_str_pts[4]);
		seq_printf(sfile, "comp_shift_pts = %d, %d, %d, %d, %d, \r\n"
			, iq_param[iq_proc_id]->companding->comp_shift_pts[0], iq_param[iq_proc_id]->companding->comp_shift_pts[1]
			, iq_param[iq_proc_id]->companding->comp_shift_pts[2], iq_param[iq_proc_id]->companding->comp_shift_pts[3]
			, iq_param[iq_proc_id]->companding->comp_shift_pts[4]);

		// iq_post_3dnr_param
		seq_printf(sfile, "====================id(%d) iq_post_3dnr_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "enable = %d, \r\n", iq_param[iq_proc_id]->post_3dnr->enable);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->post_3dnr->mode);
		post_3dnr_m = &iq_param[iq_proc_id]->post_3dnr->manual_param;
		seq_printf(sfile, "manual_param.nr_3d_adp_th_p2p = %d, %d, %d, \r\n"
			, post_3dnr_m->nr_3d_adp_th_p2p[0], post_3dnr_m->nr_3d_adp_th_p2p[1], post_3dnr_m->nr_3d_adp_th_p2p[2]);
		seq_printf(sfile, "manual_param.nr_3d_adp_weight_p2p = %d, %d, %d, \r\n"
			, post_3dnr_m->nr_3d_adp_weight_p2p[0], post_3dnr_m->nr_3d_adp_weight_p2p[1], post_3dnr_m->nr_3d_adp_weight_p2p[2]);
		seq_printf(sfile, "manual_param.tnr_p2p_border_check_th = %d, \r\n", post_3dnr_m->tnr_p2p_border_check_th);
		seq_printf(sfile, "manual_param.cur_p2p_mctf_motion_th = %d, \r\n", post_3dnr_m->cur_p2p_mctf_motion_th);
		seq_printf(sfile, "manual_param.ref_p2p_mctf_motion_th = %d, \r\n", post_3dnr_m->ref_p2p_mctf_motion_th);
		seq_printf(sfile, "manual_param.nr3d_temporal_spatial_y = %d, %d, %d, \r\n"
			, post_3dnr_m->nr3d_temporal_spatial_y[0], post_3dnr_m->nr3d_temporal_spatial_y[1], post_3dnr_m->nr3d_temporal_spatial_y[2]);
		seq_printf(sfile, "manual_param.nr3d_temporal_spatial_c = %d, %d, %d, \r\n"
			, post_3dnr_m->nr3d_temporal_spatial_c[0], post_3dnr_m->nr3d_temporal_spatial_c[1], post_3dnr_m->nr3d_temporal_spatial_c[2]);
		seq_printf(sfile, "manual_param.nr3d_temporal_range_y = %d, %d, %d, \r\n"
			, post_3dnr_m->nr3d_temporal_range_y[0], post_3dnr_m->nr3d_temporal_range_y[1], post_3dnr_m->nr3d_temporal_range_y[2]);
		seq_printf(sfile, "manual_param.nr3d_temporal_range_c = %d, %d, %d, \r\n"
			, post_3dnr_m->nr3d_temporal_range_c[0], post_3dnr_m->nr3d_temporal_range_c[1], post_3dnr_m->nr3d_temporal_range_c[2]);
		seq_printf(sfile, "manual_param.nr3d_temporal_spatial_y_mctf = %d, %d, %d, \r\n"
			, post_3dnr_m->nr3d_temporal_spatial_y_mctf[0], post_3dnr_m->nr3d_temporal_spatial_y_mctf[1], post_3dnr_m->nr3d_temporal_spatial_y_mctf[2]);
		seq_printf(sfile, "manual_param.nr3d_temporal_spatial_c_mctf = %d, %d, %d, \r\n"
			, post_3dnr_m->nr3d_temporal_spatial_c_mctf[0], post_3dnr_m->nr3d_temporal_spatial_c_mctf[1], post_3dnr_m->nr3d_temporal_spatial_c_mctf[2]);
		seq_printf(sfile, "manual_param.nr3d_temporal_range_y_mctf = %d, %d, %d, \r\n"
			, post_3dnr_m->nr3d_temporal_range_y_mctf[0], post_3dnr_m->nr3d_temporal_range_y_mctf[1], post_3dnr_m->nr3d_temporal_range_y_mctf[2]);
		seq_printf(sfile, "manual_param.nr3d_temporal_range_c_mctf = %d, %d, %d, \r\n"
			, post_3dnr_m->nr3d_temporal_range_c_mctf[0], post_3dnr_m->nr3d_temporal_range_c_mctf[1], post_3dnr_m->nr3d_temporal_range_c_mctf[2]);
		post_3dnr_a = &iq_param[iq_proc_id]->post_3dnr->auto_param[iq_proc_iso];
		seq_printf(sfile, "auto_param.nr_3d_adp_th_p2p = %d, %d, %d, \r\n"
			, post_3dnr_a->nr_3d_adp_th_p2p[0], post_3dnr_a->nr_3d_adp_th_p2p[1], post_3dnr_a->nr_3d_adp_th_p2p[2]);
		seq_printf(sfile, "auto_param.nr_3d_adp_weight_p2p = %d, %d, %d, \r\n"
			, post_3dnr_a->nr_3d_adp_weight_p2p[0], post_3dnr_a->nr_3d_adp_weight_p2p[1], post_3dnr_a->nr_3d_adp_weight_p2p[2]);
		seq_printf(sfile, "auto_param.tnr_p2p_border_check_th = %d, \r\n", post_3dnr_a->tnr_p2p_border_check_th);
		seq_printf(sfile, "auto_param.cur_p2p_mctf_motion_th = %d, \r\n", post_3dnr_a->cur_p2p_mctf_motion_th);
		seq_printf(sfile, "auto_param.ref_p2p_mctf_motion_th = %d, \r\n", post_3dnr_a->ref_p2p_mctf_motion_th);
		seq_printf(sfile, "auto_param.nr3d_temporal_spatial_y = %d, %d, %d, \r\n"
			, post_3dnr_a->nr3d_temporal_spatial_y[0], post_3dnr_a->nr3d_temporal_spatial_y[1], post_3dnr_a->nr3d_temporal_spatial_y[2]);
		seq_printf(sfile, "auto_param.nr3d_temporal_spatial_c = %d, %d, %d, \r\n"
			, post_3dnr_a->nr3d_temporal_spatial_c[0], post_3dnr_a->nr3d_temporal_spatial_c[1], post_3dnr_a->nr3d_temporal_spatial_c[2]);
		seq_printf(sfile, "auto_param.nr3d_temporal_range_y = %d, %d, %d, \r\n"
			, post_3dnr_a->nr3d_temporal_range_y[0], post_3dnr_a->nr3d_temporal_range_y[1], post_3dnr_a->nr3d_temporal_range_y[2]);
		seq_printf(sfile, "auto_param.nr3d_temporal_range_c = %d, %d, %d, \r\n"
			, post_3dnr_a->nr3d_temporal_range_c[0], post_3dnr_a->nr3d_temporal_range_c[1], post_3dnr_a->nr3d_temporal_range_c[2]);
		seq_printf(sfile, "auto_param.nr3d_temporal_spatial_y_mctf = %d, %d, %d, \r\n"
			, post_3dnr_a->nr3d_temporal_spatial_y_mctf[0], post_3dnr_a->nr3d_temporal_spatial_y_mctf[1], post_3dnr_a->nr3d_temporal_spatial_y_mctf[2]);
		seq_printf(sfile, "auto_param.nr3d_temporal_spatial_c_mctf = %d, %d, %d, \r\n"
			, post_3dnr_a->nr3d_temporal_spatial_c_mctf[0], post_3dnr_a->nr3d_temporal_spatial_c_mctf[1], post_3dnr_a->nr3d_temporal_spatial_c_mctf[2]);
		seq_printf(sfile, "auto_param.nr3d_temporal_range_y_mctf = %d, %d, %d, \r\n"
			, post_3dnr_a->nr3d_temporal_range_y_mctf[0], post_3dnr_a->nr3d_temporal_range_y_mctf[1], post_3dnr_a->nr3d_temporal_range_y_mctf[2]);
		seq_printf(sfile, "auto_param.nr3d_temporal_range_c_mctf = %d, %d, %d, \r\n"
			, post_3dnr_a->nr3d_temporal_range_c_mctf[0], post_3dnr_a->nr3d_temporal_range_c_mctf[1], post_3dnr_a->nr3d_temporal_range_c_mctf[2]);

		// iq_rgbir_enh_param
		seq_printf(sfile, "====================id(%d) iq_rgbir_enh_param==================== \r\n", iq_proc_id);
		seq_printf(sfile, "enable = %d, \r\n", iq_param[iq_proc_id]->rgbir_enh->enable);
		seq_printf(sfile, "mode = %d, \r\n", iq_param[iq_proc_id]->rgbir_enh->mode);
		seq_printf(sfile, "min_ir_th = %d, \r\n", iq_param[iq_proc_id]->rgbir_enh->min_ir_th);
		rgbir_enh_m = &iq_param[iq_proc_id]->rgbir_enh->manual_param;
		seq_printf(sfile, "manual_param.enh_ratio = %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, \r\n"
			, rgbir_enh_m->enh_ratio[0], rgbir_enh_m->enh_ratio[1], rgbir_enh_m->enh_ratio[2], rgbir_enh_m->enh_ratio[3]
			, rgbir_enh_m->enh_ratio[4], rgbir_enh_m->enh_ratio[5], rgbir_enh_m->enh_ratio[6], rgbir_enh_m->enh_ratio[7]
			, rgbir_enh_m->enh_ratio[8], rgbir_enh_m->enh_ratio[9], rgbir_enh_m->enh_ratio[10], rgbir_enh_m->enh_ratio[11]
			, rgbir_enh_m->enh_ratio[12], rgbir_enh_m->enh_ratio[13], rgbir_enh_m->enh_ratio[14], rgbir_enh_m->enh_ratio[15]);
		if (iq_proc_iso < IQ_LIGHT_ID_MAX_NUM) {
			rgbir_enh_a = &iq_param[iq_proc_id]->rgbir_enh->auto_param[iq_proc_iso];
			seq_printf(sfile, "manual_param.ir_th = %d, \r\n", rgbir_enh_a->ir_th);
			seq_printf(sfile, "manual_param.enh_ratio = %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, \r\n"
				, rgbir_enh_a->enh_ratio[0], rgbir_enh_a->enh_ratio[1], rgbir_enh_a->enh_ratio[2], rgbir_enh_a->enh_ratio[3]
				, rgbir_enh_a->enh_ratio[4], rgbir_enh_a->enh_ratio[5], rgbir_enh_a->enh_ratio[6], rgbir_enh_a->enh_ratio[7]
				, rgbir_enh_a->enh_ratio[8], rgbir_enh_a->enh_ratio[9], rgbir_enh_a->enh_ratio[10], rgbir_enh_a->enh_ratio[11]
				, rgbir_enh_a->enh_ratio[12], rgbir_enh_a->enh_ratio[13], rgbir_enh_a->enh_ratio[14], rgbir_enh_a->enh_ratio[15]);
		}

		// iq_edge_region_param
		seq_printf(sfile, "====================id(%d) iq_edge_region_param==================== \r\n", iq_proc_id);
		edge_region_m = &iq_param[iq_proc_id]->edge_region->manual_param;
		seq_printf(sfile, "manual_param.str_flat = %d, \r\n", edge_region_m->str_flat);
		seq_printf(sfile, "manual_param.str_edge = %d, \r\n", edge_region_m->str_edge);
		seq_printf(sfile, "manual_param.th_flat_low = %d, \r\n", edge_region_m->th_flat_low);
		seq_printf(sfile, "manual_param.th_edge_high = %d, \r\n", edge_region_m->th_edge_high);
		edge_region_a = &iq_param[iq_proc_id]->edge_region->auto_param[iq_proc_iso];
		seq_printf(sfile, "auto_param[%d].str_flat = %d, \r\n", iq_proc_iso, edge_region_a->str_flat);
		seq_printf(sfile, "auto_param[%d].str_edge = %d, \r\n", iq_proc_iso, edge_region_a->str_edge);
		seq_printf(sfile, "auto_param[%d].th_flat_low = %d, \r\n", iq_proc_iso, edge_region_a->th_flat_low);
		seq_printf(sfile, "auto_param[%d].th_edge_high = %d, \r\n", iq_proc_iso, edge_region_a->th_edge_high);

		iq_proc_id = 8;
	}

	if (iq_proc_r_item == IQ_PROC_R_ITEM_UI_PARAM) {
		seq_printf(sfile, "iq_proc_get_ui_param \r\n");
		seq_printf(sfile, "id : %d \r\n", iq_proc_id);
		seq_printf(sfile, "get ui nr_lv : %d \r\n", iq_ui_get_info(iq_proc_id, IQ_UI_ITEM_NR_LV));
		seq_printf(sfile, "get ui 3dnr_lv : %d \r\n", iq_ui_get_info(iq_proc_id, IQ_UI_ITEM_3DNR_LV));
		seq_printf(sfile, "get ui sharpness_lv : %d \r\n", iq_ui_get_info(iq_proc_id, IQ_UI_ITEM_SHARPNESS_LV));
		seq_printf(sfile, "get ui saturation_lv : %d \r\n", iq_ui_get_info(iq_proc_id, IQ_UI_ITEM_SATURATION_LV));
		seq_printf(sfile, "get ui contrast_lv : %d \r\n", iq_ui_get_info(iq_proc_id, IQ_UI_ITEM_CONTRAST_LV));
		seq_printf(sfile, "get ui brightness_lv : %d \r\n", iq_ui_get_info(iq_proc_id, IQ_UI_ITEM_BRIGHTNESS_LV));
		seq_printf(sfile, "get ui night_mode : %d \r\n", iq_ui_get_info(iq_proc_id, IQ_UI_ITEM_NIGHT_MODE));
		seq_printf(sfile, "get ui ycc_format : %d \r\n", iq_ui_get_info(iq_proc_id, IQ_UI_ITEM_YCC_FORMAT));
		seq_printf(sfile, "get ui operation : %d \r\n", iq_ui_get_info(iq_proc_id, IQ_UI_ITEM_OPERATION));
		seq_printf(sfile, "get ui imageeffect : %d \r\n", iq_ui_get_info(iq_proc_id, IQ_UI_ITEM_IMAGEEFFECT));
		seq_printf(sfile, "get ui ccid : %d \r\n", iq_ui_get_info(iq_proc_id, IQ_UI_ITEM_CCID));
		seq_printf(sfile, "get ui hue_shift : %d \r\n", iq_ui_get_info(iq_proc_id, IQ_UI_ITEM_HUE_SHIFT));
	}

	if (iq_proc_r_item == IQ_PROC_R_ITEM_NNSC_PARAM) {
		seq_printf(sfile, "iq_proc_get_nnsc_param \r\n");
		seq_printf(sfile, "id : %d \r\n", iq_proc_id);
		seq_printf(sfile, "get nnsc dark_enh_ratio : %d \r\n", iq_nnsc_get_info(iq_proc_id, IQ_NNSC_ITEM_DARK_ENH_RATIO));
		seq_printf(sfile, "get nnsc contrast_enh_ratio : %d \r\n", iq_nnsc_get_info(iq_proc_id, IQ_NNSC_ITEM_CONTRAST_ENH_RATIO));
		seq_printf(sfile, "get nnsc green_enh_ratio : %d \r\n", iq_nnsc_get_info(iq_proc_id, IQ_NNSC_ITEM_GREEN_ENH_RATIO));
		seq_printf(sfile, "get nnsc skin_enh_ratio : %d \r\n", iq_nnsc_get_info(iq_proc_id, IQ_NNSC_ITEM_SKIN_ENH_RATIO));
	}

	if (iq_proc_r_item == IQ_PROC_R_ITEM_CFG_DATA) {
		seq_printf(sfile, "iq_proc_get_cfg_data \r\n");
		seq_printf(sfile, "id = %d, cfg_data =%s \r\n", cfg_data.id, cfg_data.path);
	}

	iq_proc_r_item = IQ_PROC_R_ITEM_NONE;

	return 0;
}

static INT32 iq_proc_command_open(struct inode *inode, struct file *file)
{
	//return single_open(file, iq_proc_command_show, PDE_DATA(inode));
	return single_open_size(file, iq_proc_command_show, PDE_DATA(inode), 5000 * sizeof(u32));
}

static ssize_t iq_proc_command_write(struct file *file, const CHAR __user *buf, size_t size, loff_t *off)
{
	INT32 len = size;
	INT32 ret = -EINVAL;

	CHAR cmd_line[MAX_CMDLINE_LENGTH];
	CHAR *cmdstr = cmd_line;
	const CHAR delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	CHAR *argv[MAX_CMD_ARGUMENTS] = {NULL};
	INT32 argc = 0;
	UINT32 i;
	ISP_MODULE *iq_module = iq_proc_get_mudule_from_file(file);

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
					if (strncmp(argv[1], iq_proc_cmd_r_list[i].cmd, MAX_CMD_LENGTH) == 0) {
						#if MUTEX_ENABLE
						down(&mutex);
						#endif
						ret = iq_proc_cmd_r_list[i].execute(iq_module, argc, argv);
						#if MUTEX_ENABLE
						up(&mutex);
						#endif
						break;
					}
				}

				if (i >= NUM_OF_R_CMD) {
					DBG_ERR("[IQ_ERR]: => ");
					for (i = 0; i < argc; i++) {
						DBG_ERR("%s ", argv[i]);
					}
					DBG_ERR("is not in r_cmd_list!! \r\n");
				}
			} else if (strncmp(argv[0], "w", 2) == 0) {
				for (i = 0; i < NUM_OF_W_CMD; i++) {
					if (strncmp(argv[1], iq_proc_cmd_w_list[i].cmd, MAX_CMD_LENGTH) == 0) {
						#if MUTEX_ENABLE
						down(&mutex);
						#endif
						ret = iq_proc_cmd_w_list[i].execute(iq_module, argc, argv);
						#if MUTEX_ENABLE
						up(&mutex);
						#endif
						break;
					}
				}

				if (i >= NUM_OF_W_CMD) {
					DBG_ERR("[IQ_ERR]: =>");
					for (i = 0; i < argc; i++) {
						DBG_ERR("%s ", argv[i]);
					}
					DBG_ERR("is not in w_cmd_list!! \r\n");
				}
			} else {
				DBG_ERR("[IQ_ERR]: =>");
				for (i = 0; i < argc; i++) {
					DBG_ERR("%s ", argv[i]);
				}
				DBG_ERR("is not legal command!! \r\n");
			}
		}
	}

	if (ret < 0) {
		DBG_ERR("[IQ_ERR]: fail to execute: ");
		for (i = 0; i < argc; i++) {
			DBG_ERR("%s ", argv[i]);
		}
		DBG_ERR("\r\n");
	}

	return size;
}

static const struct file_operations iq_proc_command_fops = {
	.owner   = THIS_MODULE,
	.open    = iq_proc_command_open,
	.read    = seq_read,
	.write   = iq_proc_command_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static INT32 iq_proc_help_show(struct seq_file *sfile, void *v)
{
	UINT32 loop;

	seq_printf(sfile, "1. 'cat /proc/hdal/vendor/iq/info' will show all the iq info \r\n");
	seq_printf(sfile, "2. 'echo r/w xxx > /proc/hdal/vendor/iq/cmd' can input command for some debug purpose \r\n");
	seq_printf(sfile, "The currently support input command are below: \r\n");

	seq_printf(sfile, "--------------------------------------------------------------------- \r\n");
	seq_printf(sfile, "  %s\n", "iq");
	seq_printf(sfile, "--------------------------------------------------------------------- \r\n");

	for (loop = 0 ; loop < NUM_OF_R_CMD ; loop++) {
		seq_printf(sfile, "r %15s : %s\r\n", iq_proc_cmd_r_list[loop].cmd, iq_proc_cmd_r_list[loop].text);
	}

	for (loop = 0 ; loop < NUM_OF_W_CMD ; loop++) {
		seq_printf(sfile, "w %15s : %s\r\n", iq_proc_cmd_w_list[loop].cmd, iq_proc_cmd_w_list[loop].text);
	}

	seq_printf(sfile, "------------------------------------------------------------------------ \r\n");
	seq_printf(sfile, " func_en = \r\n");
	seq_printf(sfile, "   | 0x%8x = WDR    | 0x%8x = SHDR   | 0x%8x = DEFOG  |                  \r\n", ISP_FUNC_EN_WDR, ISP_FUNC_EN_SHDR, ISP_FUNC_EN_DEFOG);
	seq_printf(sfile, " dbg_lv = \r\n");
	seq_printf(sfile, "   | 0x%8x = WRN    | 0x%8x = SYNC   | 0x%8x = CFG    | 0x%8x = DTS    | \r\n", IQ_DBG_WRN_MSG, IQ_DBG_SYNC, IQ_DBG_CFG, IQ_DBG_DTS);
	seq_printf(sfile, "   | 0x%8x = VIG    | 0x%8x = GAMMA  | 0x%8x = YCURVE | 0x%8x = TABLE  | \r\n", IQ_DBG_VIG, IQ_DBG_GAMMA, IQ_DBG_YCURVE, IQ_DBG_TABLE);
	seq_printf(sfile, "   | 0x%8x = SIE    | 0x%8x = IFE    | 0x%8x = DCE    | 0x%8x = IPE    | \r\n", IQ_DBG_P_SIE, IQ_DBG_P_IFE, IQ_DBG_P_DCE, IQ_DBG_P_IPE);
	seq_printf(sfile, "   | 0x%8x = IFE2   | 0x%8x = IME    | 0x%8x = ENC    |                  \r\n", IQ_DBG_P_IFE2, IQ_DBG_P_IME, IQ_DBG_P_ENC);
	seq_printf(sfile, "   | 0x%8x = WDR    | 0x%8x = DEFOG  | 0x%8x = SHDR   | 0x%8x = RGBIR  | \r\n", IQ_DBG_A_WDR, IQ_DBG_A_DEFOG, IQ_DBG_A_SHDR, IQ_DBG_A_RGBIR);
	seq_printf(sfile, "   | 0x%8x = ISO    | 0x%8x = EDGE   | 0x%8x = TMNR   | 0x%8x = ECS    | \r\n", IQ_DBG_O_ISO, IQ_DBG_O_EDGE, IQ_DBG_O_TMNR, IQ_DBG_O_ECS);
	seq_printf(sfile, "   | 0x%8x = CAP    | 0x%8x = NNSC   |                                   \r\n", IQ_DBG_CAPTURE, IQ_DBG_NNSC);
	seq_printf(sfile, "   | 0x%8x = PERFOR | 0x%8x = BUFFER | \r\n", IQ_DBG_PERFORMANCE, IQ_DBG_BUFFER);
	seq_printf(sfile, "--------------------------------------------------------------------- \r\n");
	seq_printf(sfile, " ui_type = \r\n");
	seq_printf(sfile, "   | 0x%8x = NR     | 0x%8x = 3DNR   | 0x%8x = SHARP  |                  \r\n", IQ_UI_ITEM_NR_LV ,IQ_UI_ITEM_3DNR_LV, IQ_UI_ITEM_SHARPNESS_LV);
	seq_printf(sfile, "   | 0x%8x = SAT    | 0x%8x = CONT   | 0x%8x = BRIGHT |                  \r\n", IQ_UI_ITEM_SATURATION_LV, IQ_UI_ITEM_CONTRAST_LV, IQ_UI_ITEM_BRIGHTNESS_LV);
	seq_printf(sfile, "   | 0x%8x = NIGHT  | 0x%8x = YCC    | 0x%8x = OPERA  |                  \r\n", IQ_UI_ITEM_NIGHT_MODE, IQ_UI_ITEM_YCC_FORMAT, IQ_UI_ITEM_OPERATION);
	seq_printf(sfile, "   | 0x%8x = EFFECT | 0x%8x = CCID   | 0x%8x = HUE    |                  \r\n", IQ_UI_ITEM_IMAGEEFFECT, IQ_UI_ITEM_CCID, IQ_UI_ITEM_HUE_SHIFT);
	seq_printf(sfile, "   | 0x%8x = TONELV |                                                    \r\n", IQ_UI_ITEM_SHDR_TONE_LV);
	seq_printf(sfile, "------------------------------------------------------------------------ \r\n");
	seq_printf(sfile, " nnsc_type = \r\n");
	seq_printf(sfile, "   | 0x%8x = DARK   | 0x%8x = CONT   | 0x%8x = GREEN  | 0x%8x = SKIN   | \r\n", IQ_NNSC_ITEM_DARK_ENH_RATIO, IQ_NNSC_ITEM_CONTRAST_ENH_RATIO, IQ_NNSC_ITEM_GREEN_ENH_RATIO, IQ_NNSC_ITEM_SKIN_ENH_RATIO);
	seq_printf(sfile, "------------------------------------------------------------------------ \r\n");
	seq_printf(sfile, "Ex1: 'echo r param 0 0 > /proc/hdal/vendor/iq/cmd;cat /proc/hdal/vendor/iq/cmd' \r\n");
	seq_printf(sfile, "Ex2: 'echo r ui_param 0 > /proc/hdal/vendor/iq/cmd;cat /proc/hdal/vendor/iq/cmd' \r\n");
	seq_printf(sfile, "Ex3: 'echo w dbg 0 0x100 > /proc/hdal/vendor/iq/cmd' \r\n");
	seq_printf(sfile, "Ex4: 'echo w reload_cfg 0 /mnt/app/isp/isp_imx290_0.cfg > /proc/hdal/vendor/iq/cmd' \r\n");
	return 0;
}

static INT32 iq_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, iq_proc_help_show, PDE_DATA(inode));
}

static const struct file_operations iq_proc_help_fops = {
	.owner	 = THIS_MODULE,
	.open	 = iq_proc_help_open,
	.read	 = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// extern functions
//=============================================================================

INT32 iq_proc_create(void)
{
	INT32 ret = 0;
	struct proc_dir_entry *root = NULL, *pentry = NULL;

	// initialize synchronization mechanism
	#if MUTEX_ENABLE
	sema_init(&mutex, 1);
	#endif

	root = proc_mkdir("hdal/vendor/iq", NULL);

	if (root == NULL) {
		DBG_ERR("fail to create IQ proc root!! \r\n");
		iq_proc_remove();
		return -EINVAL;
	}
	proc_root = root;

	// create "info" entry
	pentry = proc_create_data("info", S_IRUGO | S_IXUGO, root, &iq_proc_info_fops, NULL);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc info!! \r\n");
		iq_proc_remove();
		return -EINVAL;
	}
	proc_info = pentry;

	// create "command" entry
	pentry = proc_create_data("cmd", S_IRUGO | S_IXUGO, root, &iq_proc_command_fops, NULL);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc cmd!! \r\n");
		iq_proc_remove();
		return -EINVAL;
	}
	proc_command = pentry;

	// create "help" entry
	pentry = proc_create_data("help", S_IRUGO | S_IXUGO, root, &iq_proc_help_fops, NULL);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc help!! \r\n");
		iq_proc_remove();
		return -EINVAL;
	}
	proc_help = pentry;

	// allocate memory for massage buffer
	ret = iq_proc_msg_buf_alloc();
	if (ret < 0) {
		iq_proc_remove();
	}

	return ret;
}

void iq_proc_remove(void)
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
	iq_proc_msg_buf_free();
}

