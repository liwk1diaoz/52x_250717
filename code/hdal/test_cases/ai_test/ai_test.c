/**
	@brief Source file of vendor ai net test code.

	@file ai_test.c

	@ingroup ai_net_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "ai_test_int.h"
#include "ai_test_util.h"
#include "ai_test_net.h"
#include "ai_test_op.h"
#include "hd_debug.h"
#if defined(_BSP_NA51090_)
#include "vendor_common.h"
#endif

///////////////////////////////////////////////////////////////////////////////

#define VENDOR_AI_CFG  	0x000f0000  //vendor ai config

#define NET_VDO_SIZE_W	1920 //max for net
#define NET_VDO_SIZE_H	1080 //max for net

#define AI_TEST_ROUND(x) (x + 0.5)
///////////////////////////////////////////////////////////////////////////////

// global context

TEST_NETWORK stream[NN_RUN_NET_NUM] = {0};
HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
NET_IN_CONFIG in_cfg = {.input_filename = "jpg/YUV420_SP_W512H376.bin", .w = 512, .h = 376, .c = 2, .b = 1, .bitdepth = 8, .loff = 512, .fmt = HD_VIDEO_PXLFMT_YUV420, .is_comb_img = 0};
NET_MODEL_CONFIG net_model_cfg = {	.model_filename = "para/nvt_model.bin",	.label_filename = "accuracy/labels.txt", .diff_filename = "para/nvt_stripe_model.bin"};
UINT32 set_batch_num[NN_RUN_NET_NUM][NN_SET_INPUT_NUM] = {0};
VENDOR_AI_NET_CFG_BUF_OPT net_buf_opt = {0, DDR_ID0};
VENDOR_AI_NET_CFG_JOB_OPT net_job_opt = {0, -1};
CHAR *dump_dir = NULL; //for debug compare
extern TEST_PROC_ID g_test_proc_id;
extern UINT64 net_proc_time[7];
extern DOUBLE net_cpu_loading;
BOOL use_diff_model = 0;
BOOL use_one_core = 0;
CHAR in_cfg_filename[256] = {'\0'};
UINT32 op_config_mode = 0; //0:preproc 1:mau
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
AI_OP_MAU_CONFIG_DATA op_mau_config_data = {0};
#endif
AI_OP_PREPROC_CONFIG_DATA op_preproc_config_data = {0};

static INT32 remove_blank(CHAR *p_str)
{
	CHAR *p_str0 = p_str;
	CHAR *p_str1 = p_str;

	while (*p_str0 != '\0')
	{
		if ((*p_str0 != ' ') && (*p_str0 != '\t') && (*p_str0 != '\n'))
			*p_str1++ = *p_str0++;
		else
			p_str0++;
	}

	*p_str1 = '\0';
	return TRUE;
}
static INT32 split_kvalue(CHAR *p_str, CONFIG_OPT* p_config, CHAR c)
{
	CHAR *p_key = p_str;
	CHAR *p_value = p_str;
	CHAR *p_local = p_str;
	INT32 cnt = 0;

	// check only one '='
	while (*p_local != '\0')
	{
		p_local++;
		if (*p_local == c)
			cnt++;
	}
	if (cnt != 1)
		return FALSE;

	p_local = p_str;
	while (*p_local != c)
	{
		p_local++;
	}
	p_value = p_local + 1;
	*p_local = '\0';
	strcpy(p_config->key, p_key);
	strcpy(p_config->value, p_value);
	return TRUE;
}

static INT32 ai_test_op_config_parser(CHAR *op_config_file_path)
{

	FILE *fp_tmp = NULL;

	if ((fp_tmp = fopen(op_config_file_path, "r")) == NULL)
	{
		printf("%s is not exist!\n", op_config_file_path);
		return 0;
	}
	CHAR line[512];
	while (1) {

		if (feof(fp_tmp))
			break;
		if (fgets(line, 512 - 1, fp_tmp) == NULL)
		{
			if (feof(fp_tmp))
				break;
			printf("fgets line: %s error!\n", line);
			continue;
		}

		remove_blank(line);

		if (line[0] == '#' || line[0] == '\0')
			continue;
		CONFIG_OPT tmp;
		if (split_kvalue(line, &tmp, '=') != 1)
		{
			printf("pauser line : %s error!\r\n", line);
			continue;
		}
		//printf("k=%s, v=%s\r\n", tmp.key, tmp.value);

		if (strcmp(tmp.key, "[op_config_mode]") == 0) {
			op_config_mode = atoi(tmp.value);
		}
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
		if (strcmp(tmp.key, "[mau_cal_mode]") == 0) {
			op_mau_config_data.mau_cal_mode = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[mau_sort_op]") == 0) {
			op_mau_config_data.mau_sort_op = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[mau_in_b_mode]") == 0) {
			op_mau_config_data.mau_in_b_mode = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[mau_mat_dc]") == 0) {
			op_mau_config_data.mau_mat_dc = atof(tmp.value);
		}

		if (strcmp(tmp.key, "[mau_sort_top_n]") == 0) {
			op_mau_config_data.mau_sort_top_n = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[mau_l2_norm_iter]") == 0) {
			op_mau_config_data.mau_l2_norm_iter = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[in_a_w]") == 0) {
			op_mau_config_data.in_a_w = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[in_a_h]") == 0) {
			op_mau_config_data.in_a_h = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[in_a_frac_bit]") == 0) {
			op_mau_config_data.in_a_frac_bit = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[in_a_bit_depth]") == 0) {
			op_mau_config_data.in_a_bit_depth = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[in_a_fmt]") == 0) {
			op_mau_config_data.in_a_fmt = ai_test_operator_get_fmt(atoi(tmp.value), op_mau_config_data.in_a_frac_bit);
		}
		
		if (strcmp(tmp.key, "[in_a_file_path]") == 0) {
			strncpy(op_mau_config_data.in_a_file_path, tmp.value, 256-1);
			op_mau_config_data.in_a_file_path[256-1] = '\0';
		}
		if (strcmp(tmp.key, "[in_b_w]") == 0) {
			op_mau_config_data.in_b_w = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[in_b_h]") == 0) {
			op_mau_config_data.in_b_h = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[in_b_frac_bit]") == 0) {
			op_mau_config_data.in_b_frac_bit = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[in_b_bit_depth]") == 0) {
			op_mau_config_data.in_b_bit_depth = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[in_b_fmt]") == 0) {
			op_mau_config_data.in_b_fmt = ai_test_operator_get_fmt(atoi(tmp.value), op_mau_config_data.in_b_frac_bit);
		}
		
		if (strcmp(tmp.key, "[in_b_file_path]") == 0) {
			strncpy(op_mau_config_data.in_b_file_path, tmp.value, 256-1);
			op_mau_config_data.in_b_file_path[256-1] = '\0';
		}
		if (strcmp(tmp.key, "[out_c_frac_bit]") == 0) {
			op_mau_config_data.out_c_frac_bit = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[out_c_bit_depth]") == 0) {
			op_mau_config_data.out_c_bit_depth = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[out_c_fmt]") == 0) {
			op_mau_config_data.out_c_fmt = ai_test_operator_get_fmt(atoi(tmp.value), op_mau_config_data.out_c_frac_bit);
		}
		
		if (strcmp(tmp.key, "[out_c_file_path]") == 0) {
			strncpy(op_mau_config_data.out_c_file_path, tmp.value, 256-1);
			op_mau_config_data.out_c_file_path[256-1] = '\0';
		}

		if (strcmp(tmp.key, "[sort_idx_bit_depth]") == 0) {
			op_mau_config_data.sort_idx_bit_depth = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[sort_idx_fmt]") == 0) {
			op_mau_config_data.sort_idx_fmt = atoi(tmp.value);
		}
#endif

		if (strcmp(tmp.key, "[in_cnt]") == 0) {
			op_preproc_config_data.in_cnt = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[out_cnt]") == 0) {
			op_preproc_config_data.out_cnt = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[src_0_width]") == 0) {
			op_preproc_config_data.src_0_width = atoi(tmp.value);
			in_cfg.w = op_preproc_config_data.src_0_width;
		}
		if (strcmp(tmp.key, "[src_0_height]") == 0) {
			op_preproc_config_data.src_0_height = atoi(tmp.value);
			in_cfg.h = op_preproc_config_data.src_0_height;
		}
		if (strcmp(tmp.key, "[src_0_fmt]") == 0) {
			op_preproc_config_data.src_0_fmt = (UINT32)strtol(tmp.value, NULL, 0);
		}
		if (strcmp(tmp.key, "[src_0_line_ofs]") == 0) {
			op_preproc_config_data.src_0_line_ofs = atoi(tmp.value);
			in_cfg.loff = op_preproc_config_data.src_0_line_ofs;
		}
		if (strcmp(tmp.key, "[src_1_width]") == 0) {
			op_preproc_config_data.src_1_width = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[src_1_height]") == 0) {
			op_preproc_config_data.src_1_height = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[src_1_fmt]") == 0) {
			op_preproc_config_data.src_1_fmt = (UINT32)strtol(tmp.value, NULL, 0);
		}
		if (strcmp(tmp.key, "[src_1_line_ofs]") == 0) {
			op_preproc_config_data.src_1_line_ofs = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[src_2_width]") == 0) {
			op_preproc_config_data.src_2_width = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[src_2_height]") == 0) {
			op_preproc_config_data.src_2_height = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[src_2_fmt]") == 0) {
			op_preproc_config_data.src_2_fmt = (UINT32)strtol(tmp.value, NULL, 0);
		}
		if (strcmp(tmp.key, "[src_2_line_ofs]") == 0) {
			op_preproc_config_data.src_2_line_ofs = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[src_file_path]") == 0) {
			strncpy(op_preproc_config_data.src_file_path, tmp.value, 256-1);
			op_preproc_config_data.src_file_path[256-1] = '\0';
			memcpy(in_cfg.input_filename, op_preproc_config_data.src_file_path, 256);
		}
		if (strcmp(tmp.key, "[dst_0_width]") == 0) {
			op_preproc_config_data.dst_0_width = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[dst_0_height]") == 0) {
			op_preproc_config_data.dst_0_height = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[dst_0_fmt]") == 0) {
			op_preproc_config_data.dst_0_fmt = (UINT32)strtol(tmp.value, NULL, 0);
		}
		if (strcmp(tmp.key, "[dst_0_line_ofs]") == 0) {
			op_preproc_config_data.dst_0_line_ofs = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[dst_1_width]") == 0) {
			op_preproc_config_data.dst_1_width = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[dst_1_height]") == 0) {
			op_preproc_config_data.dst_1_height = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[dst_1_fmt]") == 0) {
			op_preproc_config_data.dst_1_fmt = (UINT32)strtol(tmp.value, NULL, 0);
		}
		if (strcmp(tmp.key, "[dst_1_line_ofs]") == 0) {
			op_preproc_config_data.dst_1_line_ofs = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[dst_2_width]") == 0) {
			op_preproc_config_data.dst_2_width = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[dst_2_height]") == 0) {
			op_preproc_config_data.dst_2_height = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[dst_2_fmt]") == 0) {
			op_preproc_config_data.dst_2_fmt = (UINT32)strtol(tmp.value, NULL, 0);
		}
		if (strcmp(tmp.key, "[dst_2_line_ofs]") == 0) {
			op_preproc_config_data.dst_2_line_ofs = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[scale_w]") == 0) {
			op_preproc_config_data.scale_w = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[scale_h]") == 0) {
			op_preproc_config_data.scale_h = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[out_sub_color_0]") == 0) {
			op_preproc_config_data.out_sub_color_0 = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[out_sub_color_1]") == 0) {
			op_preproc_config_data.out_sub_color_1 = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[out_sub_color_2]") == 0) {
			op_preproc_config_data.out_sub_color_2 = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[dst_file_path]") == 0) {
			strncpy(op_preproc_config_data.dst_file_path, tmp.value, 256-1);
			op_preproc_config_data.dst_file_path[256-1] = '\0';
		}
	}

	fclose(fp_tmp);

	return 1;
}

static int ai_test_cmd_load_user_option(int argc, char **argv)
{
	INT32 idx;
	UINT32 job_method, buf_method;
	INT32 job_wait_ms;
	
	if (argc < 3){
		printf("usage : ai_test <input_bin> <in_w> <in_h> <in_c> <in_loff> <in_pxlfmt> <model_bin> <out_label> (job_opt) (job_sync) (buf_opt)\n");
		return -1;
	}
	
	idx = 1;
	// parse input config
	if (argc > idx) {
		strncpy(in_cfg.input_filename, argv[idx++], 256-1);
		in_cfg.input_filename[256-1] = '\0';
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%lu", &in_cfg.w);
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%lu", &in_cfg.h);
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%lu", &in_cfg.c);
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%lu", &in_cfg.loff);
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%x", (unsigned int *)&in_cfg.fmt);
	}
	
	// parse network config
	if (argc > idx) {
		strncpy(net_model_cfg.model_filename, argv[idx++], 256-1);
		net_model_cfg.model_filename[256-1] = '\0';
	}
	if (argc > idx) {
		strncpy(net_model_cfg.label_filename, argv[idx++], 256-1);
		net_model_cfg.label_filename[256-1] = '\0';
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%lu", &job_method);
		net_job_opt.method = job_method;
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%ld", &job_wait_ms);
		net_job_opt.wait_ms = job_wait_ms;
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%lu", &buf_method);
		net_buf_opt.method = buf_method;
	}
	if (argc > idx) {
		dump_dir = argv[idx++];
		if (dump_dir) {
			UINT32 i = 0;
			for (i = 0; i < NN_RUN_NET_NUM; i++) {
				stream[i].auto_test = 1; //enable auto-test mode
			}
		}
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%i", &use_diff_model);
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%i", &use_one_core);
	}
	if (argc > idx) {
		strncpy(in_cfg_filename, argv[idx++], 256 - 1);
		in_cfg_filename[256 - 1] = '\0';
	}
	return 0;
}

static int ai_test_cmd_load_user_option_multi(int argc, char **argv)
{
	INT32 idx;
	UINT32 job_method, buf_method;
	INT32 job_wait_ms;

	if (argc < 3) {
		printf("usage : ai_test <in_cfg_filename> <model_bin> <out_label> (job_opt) (job_sync) (buf_opt) <dump_dir> (use_diff_model)(use_one_core)\n");
		return -1;
	}

	idx = 1;
	
	if (argc > idx) {
		strncpy(in_cfg_filename, argv[idx++], 256 - 1);
		in_cfg_filename[256 - 1] = '\0';
	}

	// parse network config
	if (argc > idx) {
		strncpy(net_model_cfg.model_filename, argv[idx++], 256 - 1);
		net_model_cfg.model_filename[256 - 1] = '\0';
	}
	if (argc > idx) {
		strncpy(net_model_cfg.label_filename, argv[idx++], 256 - 1);
		net_model_cfg.label_filename[256 - 1] = '\0';
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%lu", &job_method);
		net_job_opt.method = job_method;
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%ld", &job_wait_ms);
		net_job_opt.wait_ms = job_wait_ms;
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%lu", &buf_method);
		net_buf_opt.method = buf_method;
	}
	if (argc > idx) {
		dump_dir = argv[idx++];
		if (dump_dir) {
			UINT32 i = 0;
			for (i = 0; i < NN_RUN_NET_NUM; i++) {
				stream[i].auto_test = 1; //enable auto-test mode
			}
		}
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%i", &use_diff_model);
	}
	if (argc > idx) {
		sscanf(argv[idx++], "%i", &use_one_core);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//SELECT PROC ID
static int ai_test_select_proc_id_min(void)
{
	return ai_test_network_sel_proc_id(0);
}
static int ai_test_select_proc_id_max(void)
{
	return ai_test_network_sel_proc_id(15);
}
static int ai_test_select_proc_id_invalid(void)
{
	return ai_test_network_sel_proc_id(0xff);
}

HD_DBG_MENU ai_test_sel_proc_id_menu[] = {
	{1, "min proc id", 			ai_test_select_proc_id_min,		TRUE},
	{2, "max proc id", 			ai_test_select_proc_id_max,		TRUE},
	{3, "invalid proc id",		ai_test_select_proc_id_invalid,	TRUE},

	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

////////////////////////////////////////////////////////////////////////////////
//SET
static int ai_test_net_set_cfg_buf_opt_0(void)
{
	return ai_test_network_set_config_buf(stream[0].net_path, 0, DDR_ID0);
}
static int ai_test_net_set_cfg_buf_opt_1(void)
{
	return ai_test_network_set_config_buf(stream[0].net_path, 1, DDR_ID0);
}
static int ai_test_net_set_cfg_buf_opt_2(void)
{
	return ai_test_network_set_config_buf(stream[0].net_path, 2, DDR_ID0);
}
static int ai_test_net_set_cfg_buf_opt_3(void)
{
	return ai_test_network_set_config_buf(stream[0].net_path, 3, DDR_ID0);
}
static int ai_test_net_set_cfg_buf_opt_ERR(void)
{
	return ai_test_network_set_config_buf(stream[0].net_path, 5, DDR_ID0);
}

static HD_DBG_MENU ai_test_net_cfg_buf_opt_menu[] = {
	{1, "net_cfg_buf_opt_0", 		ai_test_net_set_cfg_buf_opt_0,	TRUE},
	{2, "net_cfg_buf_opt_1", 		ai_test_net_set_cfg_buf_opt_1,	TRUE},
	{3, "net_cfg_buf_opt_2", 		ai_test_net_set_cfg_buf_opt_2,	TRUE},
	{4, "net_cfg_buf_opt_3", 		ai_test_net_set_cfg_buf_opt_3,	TRUE},
	{5, "net_cfg_buf_opt_ERR", 		ai_test_net_set_cfg_buf_opt_ERR,	TRUE},

	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static int ai_test_net_set_cfg_buf_opt(void)
{
	return my_debug_menu_entry_p(ai_test_net_cfg_buf_opt_menu, "AI NET CFG BUFOPT");
}

//////////////////////////////////////////////////////////////////

static int ai_test_net_set_cfg_job_opt_sync(void)
{
	return ai_test_network_set_config_job(stream[0].net_path, 0, -1);
}
static int ai_test_net_set_cfg_job_opt_0(void)
{
	return ai_test_network_set_config_job(stream[0].net_path, 0, 0);
}
static int ai_test_net_set_cfg_job_opt_1(void)
{
	return ai_test_network_set_config_job(stream[0].net_path, 1, 0);
}
static int ai_test_net_set_cfg_job_opt_10(void)
{
	return ai_test_network_set_config_job(stream[0].net_path, 10, 0);
}
static int ai_test_net_set_cfg_job_opt_11(void)
{
	return ai_test_network_set_config_job(stream[0].net_path, 11, 0);
}
static int ai_test_net_set_cfg_job_opt_12(void)
{
	return ai_test_network_set_config_job(stream[0].net_path, 12, 0);
}
static int ai_test_net_set_cfg_job_opt_ERR(void)
{
	return ai_test_network_set_config_job(stream[0].net_path, 5, 5);
}

static HD_DBG_MENU ai_test_net_cfg_job_opt_menu[] = {
	{1, "net_cfg_job_opt_sync", 	ai_test_net_set_cfg_job_opt_sync,	TRUE},
	{2, "net_cfg_job_opt_0", 		ai_test_net_set_cfg_job_opt_0,	TRUE},
	{3, "net_cfg_job_opt_1", 		ai_test_net_set_cfg_job_opt_1,	TRUE},
	{4, "net_cfg_job_opt_10", 		ai_test_net_set_cfg_job_opt_10,	TRUE},
	{5, "net_cfg_job_opt_11", 		ai_test_net_set_cfg_job_opt_11,	TRUE},
	{6, "net_cfg_job_opt_12",		ai_test_net_set_cfg_job_opt_12, TRUE},
	{7, "net_cfg_job_opt_ERR", 		ai_test_net_set_cfg_job_opt_ERR,	TRUE},

	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static int ai_test_net_set_cfg_job_opt(void)
{
	return my_debug_menu_entry_p(ai_test_net_cfg_job_opt_menu, "AI NET CFG JOBOPT");
}

//////////////////////////////////////////////////////////////////

static int ai_test_net_set_cfg_model_bin_default(void)
{
	NET_MODEL_CONFIG s_net_model_cfg = {	.model_filename = "para/nvt_model.bin",	.label_filename = "accuracy/labels.txt"};
	s_net_model_cfg.binsize = net_model_cfg.binsize;
	s_net_model_cfg.diff_binsize = net_model_cfg.diff_binsize;
	return ai_test_network_set_config_model(stream[0].net_path, &s_net_model_cfg);
}
static int ai_test_net_set_cfg_model_bin_ERR_nofile(void)
{
	NET_MODEL_CONFIG s_net_model_cfg = {	.model_filename = "para/nofile.bin",	.label_filename = "accuracy/labels.txt"};
	s_net_model_cfg.binsize = net_model_cfg.binsize;
	return ai_test_network_set_config_model(stream[0].net_path, &s_net_model_cfg);}
static int ai_test_net_set_cfg_model_bin_ERR_wrongbin(void)
{
	NET_MODEL_CONFIG s_net_model_cfg = {	.model_filename = "para/nvt_model_wrong.bin",	.label_filename = "accuracy/labels.txt"};
	s_net_model_cfg.binsize = net_model_cfg.binsize;
	return ai_test_network_set_config_model(stream[0].net_path, &s_net_model_cfg);
}
static int ai_test_net_set_cfg_model_bin_ERR_large_iomem(void)
{
	NET_MODEL_CONFIG s_net_model_cfg = {	.model_filename = "para/nvt_model_large_iomem.bin",	.label_filename = "accuracy/labels.txt"};
	s_net_model_cfg.binsize = net_model_cfg.binsize;
	return ai_test_network_set_config_model(stream[0].net_path, &s_net_model_cfg);
}

static HD_DBG_MENU ai_test_net_cfg_model_bin_menu[] = {
	{1, "net_cfg_model_bin_default",      ai_test_net_set_cfg_model_bin_default,      TRUE},
	{2, "net_cfg_model_bin_ERR_nofile",   ai_test_net_set_cfg_model_bin_ERR_nofile,   TRUE},
	{3, "net_cfg_model_bin_ERR_wrongbin", ai_test_net_set_cfg_model_bin_ERR_wrongbin, TRUE},
	{4, "net_cfg_model_bin_ERR_large_iomem", ai_test_net_set_cfg_model_bin_ERR_large_iomem, TRUE},
	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static int ai_test_net_set_cfg_model_bin(void)
{
	return my_debug_menu_entry_p(ai_test_net_cfg_model_bin_menu, "AI NET CFG MODEL_BIN");
}

//////////////////////////////////////////////////////////////////
static int ai_test_net_set_cfg_res_bin_default(void)
{
	NET_MODEL_CONFIG s_net_model_cfg = {	.model_filename = "para/nvt_model_1280x720.bin",
											.label_filename = "accuracy/labels.txt",
											.diff_filename = "para/nvt_stripe_model.bin"};
	s_net_model_cfg.binsize = net_model_cfg.binsize;
	s_net_model_cfg.diff_binsize = net_model_cfg.diff_binsize;
	return ai_test_network_set_config_model(stream[0].net_path, &s_net_model_cfg);
}
static int ai_test_net_set_cfg_res_bin_ERR_nofile(void)
{
	NET_MODEL_CONFIG s_net_model_cfg = {	.model_filename = "para/nvt_model_1280x720.bin",
											.label_filename = "accuracy/labels.txt",
											.diff_filename = "para/nofile.bin"};
	s_net_model_cfg.binsize = net_model_cfg.binsize;
	s_net_model_cfg.diff_binsize = net_model_cfg.diff_binsize;
	return ai_test_network_set_config_model(stream[0].net_path, &s_net_model_cfg);}
static int ai_test_net_set_cfg_res_bin_ERR_wrongbin(void)
{
	NET_MODEL_CONFIG s_net_model_cfg = {	.model_filename = "para/nvt_model_1280x720.bin",
											.label_filename = "accuracy/labels.txt",
											.diff_filename = "para/nofile.bin"};
	s_net_model_cfg.binsize = net_model_cfg.binsize;
	s_net_model_cfg.diff_binsize = net_model_cfg.diff_binsize;
	return ai_test_network_set_config_model(stream[0].net_path, &s_net_model_cfg);
}

static HD_DBG_MENU ai_test_net_cfg_res_bin_menu[] = {
	{1, "net_cfg_res_bin_default",      ai_test_net_set_cfg_res_bin_default,      TRUE},
	{2, "net_cfg_res_bin_ERR_nofile",   ai_test_net_set_cfg_res_bin_ERR_nofile,   TRUE},
	{3, "net_cfg_res_bin_ERR_wrongbin", ai_test_net_set_cfg_res_bin_ERR_wrongbin, TRUE},
	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static int ai_test_net_set_cfg_res_bin(void)
{
	return my_debug_menu_entry_p(ai_test_net_cfg_res_bin_menu, "AI NET CFG RES_BIN");
}

//////////////////////////////////////////////////////////////////

static int ai_test_net_set_cfg_work_buf_default(void)
{
	return ai_test_network_alloc_io_buf(stream[0].net_path);
}
static int ai_test_net_set_cfg_work_buf_ERR_null(void)
{
	return ai_test_network_alloc_null_io_buf(stream[0].net_path);
}
static int ai_test_net_set_cfg_work_buf_ERR_non_align(void)
{
	return ai_test_network_alloc_non_align_io_buf(stream[0].net_path);
}

static HD_DBG_MENU ai_test_net_cfg_work_buf_menu[] = {
	{1, "net_cfg_work_buf_default",       ai_test_net_set_cfg_work_buf_default,       TRUE},
	{2, "net_cfg_work_buf_ERR_null",      ai_test_net_set_cfg_work_buf_ERR_null,      TRUE},
	{3, "net_cfg_work_buf_ERR_non_align", ai_test_net_set_cfg_work_buf_ERR_non_align, TRUE},
	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};


static int ai_test_net_set_cfg_work_buf(void)
{
	return my_debug_menu_entry_p(ai_test_net_cfg_work_buf_menu, "AI NET CFG WORK_BUF");
}

//////////////////////////////////////////////////////////////////

static int ai_test_net_set_cfg_inner_buf(void)
{
	return 1;
}

//////////////////////////////////////////////////////////////////

static int ai_test_net_set_cfg_res_id_default(void)
{
	UINT32 idx = 4;
	UINT32 proc_id = 0;

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_RES_ID, &idx);
	printf("set VENDOR_AI_NET_PARAM_RES_ID\n");
	return 1;
}

static int ai_test_net_set_cfg_res_id_ERR_min(void)
{
	UINT32 idx = 0;
	UINT32 proc_id = 0;

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}
	
	vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_RES_ID, &idx);
	printf("set VENDOR_AI_NET_PARAM_RES_ID\n");
	return 1;
}

static int ai_test_net_set_cfg_res_id_ERR_max(void)
{
	UINT32 idx = 10;
	UINT32 proc_id = 0;

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_RES_ID, &idx);
	printf("set VENDOR_AI_NET_PARAM_RES_ID\n");
	return 1;
}
static HD_DBG_MENU ai_test_net_cfg_res_id_menu[] = {
	{1, "net_cfg_res_id_default",   ai_test_net_set_cfg_res_id_default, TRUE},
	{2, "net_cfg_res_id_ERR_min",   ai_test_net_set_cfg_res_id_ERR_min, TRUE},
	{3, "net_cfg_res_id_ERR_max", 	ai_test_net_set_cfg_res_id_ERR_max, TRUE},
	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static int ai_test_net_set_res_id(void)
{
	return my_debug_menu_entry_p(ai_test_net_cfg_res_id_menu, "AI NET CFG RES_ID");
}
//////////////////////////////////////////////////////////////////

static int ai_test_net_set_cfg_res_dim_default(void)
{
	HD_DIM dim = {0};
	UINT32 proc_id = 0;

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	dim.w = 480;
	dim.h = 320;
	vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_RES_DIM, &dim);
	printf("set VENDOR_AI_NET_PARAM_RES_DIM\n");
	return 1;
}

static int ai_test_net_set_cfg_res_dim_ERR_min(void)
{
	HD_DIM dim = {0};
	UINT32 proc_id = 0;

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	dim.w = 0;
	dim.h = 0;
	vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_RES_DIM, &dim);
	printf("set VENDOR_AI_NET_PARAM_RES_DIM\n");
	return 1;
}

static int ai_test_net_set_cfg_res_dim_ERR_max(void)
{
	HD_DIM dim = {0};
	UINT32 proc_id = 0;

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	dim.w = 4096;
	dim.h = 4096;
	vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_RES_DIM, &dim);
	printf("set VENDOR_AI_NET_PARAM_RES_DIM\n");
	return 1;
}

static HD_DBG_MENU ai_test_net_cfg_res_dim_menu[] = {
	{1, "net_cfg_res_dim_default",   ai_test_net_set_cfg_res_dim_default, TRUE},
	{2, "net_cfg_res_dim_ERR_min",   ai_test_net_set_cfg_res_dim_ERR_min, TRUE},
	{3, "net_cfg_res_dim_ERR_max", 	ai_test_net_set_cfg_res_dim_ERR_max, TRUE},
	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static int ai_test_net_set_res_dim(void)
{
	return my_debug_menu_entry_p(ai_test_net_cfg_res_dim_menu, "AI NET CFG RES_DIM");
}

//////////////////////////////////////////////////////////////////
// SET IN_BUF
static int ai_test_net_set_in_buf_normal(void)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_BUF inbuf = {0};

	// get inbuf from src_img
	ret = ai_test_input_pull_buf(stream[0].in_path, &inbuf, 0);
	if (HD_OK != ret) {
		printf("proc_id(%lu) pull input fail !!\n", stream[0].in_path);
		return ret;
	}

	return ai_test_network_set_in_buf(stream[0].net_path, 0, 0, &inbuf);
}

static int ai_test_net_set_in_buf_unset(void)
{
	VENDOR_AI_BUF inbuf = {0};

	return ai_test_network_set_in_buf(stream[0].net_path, 0, 0, &inbuf);
}

static int ai_test_net_set_in_buf_exceed_outsize(void)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_BUF inbuf = {0};

	// get inbuf from src_img
	ret = ai_test_input_pull_buf(stream[0].in_path, &inbuf, 0);
	if (HD_OK != ret) {
		printf("proc_id(%lu) pull input fail !!\n", stream[0].in_path);
		return ret;
	}

	inbuf.size = 0xffffffff;

	return ai_test_network_set_in_buf(stream[0].net_path, 0, 0, &inbuf);
}

static int ai_test_net_set_in_buf_nullbuf(void)
{
	return ai_test_network_set_in_buf(stream[0].net_path, 0, 0, NULL);
}

static int ai_test_net_set_in_buf_err_addr(void)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_BUF inbuf = {0};

	// get inbuf from src_img
	ret = ai_test_input_pull_buf(stream[0].in_path, &inbuf, 0);
	if (HD_OK != ret) {
		printf("proc_id(%lu) pull input fail !!\n", stream[0].in_path);
		return ret;
	}

	inbuf.pa = 0xffffffff;
	inbuf.va = 0xffffffff;

	return ai_test_network_set_in_buf(stream[0].net_path, 0, 0, &inbuf);
}

static int ai_test_net_set_in_buf_zero_addr(void)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_BUF inbuf = {0};

	// get inbuf from src_img
	ret = ai_test_input_pull_buf(stream[0].in_path, &inbuf, 0);
	if (HD_OK != ret) {
		printf("proc_id(%lu) pull input fail !!\n", stream[0].in_path);
		return ret;
	}

	inbuf.pa = 0;
	inbuf.va = 0;

	return ai_test_network_set_in_buf(stream[0].net_path, 0, 0, &inbuf);
}

static int ai_test_net_set_in_buf_zero_size(void)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_BUF inbuf = {0};

	// get inbuf from src_img
	ret = ai_test_input_pull_buf(stream[0].in_path, &inbuf, 0);
	if (HD_OK != ret) {
		printf("proc_id(%lu) pull input fail !!\n", stream[0].in_path);
		return ret;
	}

	inbuf.size = 0;

	return ai_test_network_set_in_buf(stream[0].net_path, 0, 0, &inbuf);
}

static HD_DBG_MENU ai_test_net_set_in_buf_menu[] = {
	{1,  "net_set_in_buf_normal",			ai_test_net_set_in_buf_normal, 			TRUE},

	// invalid cases
	{11, "net_set_in_buf_unset",			ai_test_net_set_in_buf_unset, 			TRUE},
	{12, "net_set_in_buf_exceed_outsize",	ai_test_net_set_in_buf_exceed_outsize, 	TRUE},
	{13, "net_set_in_buf_nullbuf",			ai_test_net_set_in_buf_nullbuf, 		TRUE},
	{14, "net_set_in_buf_err_addr",			ai_test_net_set_in_buf_err_addr, 		TRUE},
	{15, "net_set_in_buf_zero_addr",		ai_test_net_set_in_buf_zero_addr, 		TRUE},
	{16, "net_set_in_buf_zero_size",		ai_test_net_set_in_buf_zero_size, 		TRUE},

	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static int ai_test_net_set_in_buf(void)
{
	return my_debug_menu_entry_p(ai_test_net_set_in_buf_menu, "AI NET SET IN_BUF");
}

//////////////////////////////////////////////////////////////////
// SET UNKNOWN_PARAM
static int ai_test_net_set_unknown_param(void)
{
	return ai_test_network_set_unknown(stream[0].net_path);
}

//////////////////////////////////////////////////////////////////

static HD_DBG_MENU ai_test_net_set_menu[] = {
	{1,  "net_cfg_buf_opt", 	ai_test_net_set_cfg_buf_opt,	TRUE},
	{2,  "net_cfg_job_opt", 	ai_test_net_set_cfg_job_opt,	TRUE},
	{3,  "net_cfg_model_bin",	ai_test_net_set_cfg_model_bin,	TRUE},
	{4,  "net_cfg_res_bin", 	ai_test_net_set_cfg_res_bin,	TRUE},
	{5,  "net_cfg_work_buf",	ai_test_net_set_cfg_work_buf,	TRUE},
	{6,  "net_cfg_inner_buf",	ai_test_net_set_cfg_inner_buf,	TRUE},
	{7,  "net_set_res_id",		ai_test_net_set_res_id, 		TRUE},
	{8,  "net_set_res_dim", 	ai_test_net_set_res_dim,		TRUE},
	{9,  "net_set_in_buf",		ai_test_net_set_in_buf, 		TRUE},

	// invalid cases
	{11, "net_set_unknown_param",	ai_test_net_set_unknown_param, 	TRUE},

	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static int ai_test_net_set(void)
{
	return my_debug_menu_entry_p(ai_test_net_set_menu, "AI NET SET");
}

////////////////////////////////////////////////////////////////////////////////
//GET IN_BUF
static int ai_test_net_get_in_buf_ly1_in0(void)
{
	return ai_test_network_get_in_buf_byid(stream[0].net_path, 1, 0);
}

static int ai_test_net_get_in_buf_minus1_in0(void)
{
	return ai_test_network_get_in_buf_byid(stream[0].net_path, -1, 0);
}

static int ai_test_net_get_in_buf_ly1_minus1(void)
{
	return ai_test_network_get_in_buf_byid(stream[0].net_path, 1, -1);
}

static HD_DBG_MENU ai_test_net_get_in_buf_menu[] = {
	{1,  "net_get_in_buf_ly1_in0",			ai_test_net_get_in_buf_ly1_in0, 	TRUE},

	// invalid cases
	{11, "net_get_in_buf_minus1_in0",		ai_test_net_get_in_buf_minus1_in0, 	TRUE},
	{12, "net_get_in_buf_ly1_minus1",		ai_test_net_get_in_buf_ly1_minus1, 	TRUE},

	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static int ai_test_net_get_in_buf(void)
{
	return my_debug_menu_entry_p(ai_test_net_get_in_buf_menu, "AI NET GET IN_BUF");
}

////////////////////////////////////////////////////////////////////////////////
//GET OUT_BUF
static int ai_test_net_get_out_buf_ly10_out0(void)
{
	return ai_test_network_get_out_buf_byid(stream[0].net_path, 10, 0);
}

static int ai_test_net_get_out_buf_minus1_out0(void)
{
	return ai_test_network_get_out_buf_byid(stream[0].net_path, -1, 0);
}

static int ai_test_net_get_out_buf_ly10_minus1(void)
{
	return ai_test_network_get_out_buf_byid(stream[0].net_path, 10, -1);
}

static HD_DBG_MENU ai_test_net_get_out_buf_menu[] = {
	{1,  "net_get_out_buf_ly10_out0",		ai_test_net_get_out_buf_ly10_out0, 		TRUE},

	// invalid cases
	{11, "net_get_out_buf_minus1_out0",		ai_test_net_get_out_buf_minus1_out0,	TRUE},
	{12, "net_get_out_buf_ly10_minus1",		ai_test_net_get_out_buf_ly10_minus1, 	TRUE},

	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static int ai_test_net_get_out_buf(void)
{
	return my_debug_menu_entry_p(ai_test_net_get_out_buf_menu, "AI NET GET OUT_BUF");
}

////////////////////////////////////////////////////////////////////////////////
//GET FIRST IN_BUF
static int ai_test_net_get_first_in_buf(void)
{
	return ai_test_network_get_first_in_buf(stream[0].net_path);
}

////////////////////////////////////////////////////////////////////////////////
//GET FIRST OUT_BUF
static int ai_test_net_get_first_out_buf(void)
{
	return ai_test_network_get_first_out_buf(stream[0].net_path);
}

////////////////////////////////////////////////////////////////////////////////
//GET OUT_BUF BY NAME
static int ai_test_net_get_out_byname_probout0(void)
{
	VENDOR_AI_BUF_NAME buf_name = {0};
	sprintf(buf_name.name, "prob.out0");
	return ai_test_network_get_out_buf_byname(stream[0].net_path, &buf_name);
}

static int ai_test_net_get_out_byname_null(void)
{
	VENDOR_AI_BUF_NAME null_name = {0};
	return ai_test_network_get_out_buf_byname(stream[0].net_path, &null_name);
}

static int ai_test_net_get_out_byname_toolong(void)
{
	VENDOR_AI_BUF_NAME buf_name = {0};
	memset(buf_name.name, 0x46, sizeof(buf_name.name));
	return ai_test_network_get_out_buf_byname(stream[0].net_path, &buf_name);
}

static HD_DBG_MENU ai_test_net_get_out_buf_byname_menu[] = {
	{1,  "net_get_out_byname_prob_out0",	ai_test_net_get_out_byname_probout0, 	TRUE},

	// invalid cases
	{11, "net_get_out_byname_null",			ai_test_net_get_out_byname_null,		TRUE},
	{12, "net_get_out_byname_toolong",		ai_test_net_get_out_byname_toolong, 	TRUE},

	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static int ai_test_net_get_out_buf_byname(void)
{
	return my_debug_menu_entry_p(ai_test_net_get_out_buf_byname_menu, "AI NET GET OUT_BYNAME");
}

//////////////////////////////////////////////////////////////////
// GET UNKNOWN_PARAM
static int ai_test_net_get_unknown_param(void)
{
	return ai_test_network_get_unknown(stream[0].net_path);
}

//////////////////////////////////////////////////////////////////

static HD_DBG_MENU ai_test_net_get_menu[] = {
	{1,  "net_get_in_buf",			ai_test_net_get_in_buf, 		TRUE},
	{2,  "net_get_out_buf", 		ai_test_net_get_out_buf,		TRUE},
	{3,  "net_get_first_in_buf",	ai_test_net_get_first_in_buf,	TRUE},
	{4,  "net_get_first_out_buf",	ai_test_net_get_first_out_buf,	TRUE},
	{5,  "net_get_out_buf_byname",	ai_test_net_get_out_buf_byname,	TRUE},

	// invalid cases
	{11, "net_get_unknown_param",	ai_test_net_get_unknown_param,	TRUE},


	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static int ai_test_net_get(void)
{
	return my_debug_menu_entry_p(ai_test_net_get_menu, "AI NET GET");
}

////////////////////////////////////////////////////////////////////////////////
//FLOW
static int ai_test_flow_1(void)
{
	return 1;
}
static int ai_test_flow_2(void)
{
	return 1;
}

static HD_DBG_MENU ai_test_flow_menu[] = {
	//80~89
	{81, "flow_1",				ai_test_flow_1, TRUE},
	{82, "flow_2",				ai_test_flow_2, TRUE},
	
	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static int ai_test_flow(void)
{
	return my_debug_menu_entry_p(ai_test_flow_menu, "AI FLOW");
}


////////////////////////////////////////////////////////////////////////////////
//OPERATION
static int ai_test_op_0(void)
{
	ai_test_operator_set_config(stream[0].op_path, 0);
	return 1;
}
static int ai_test_op_1(void)
{
	ai_test_operator_set_config(stream[0].op_path, 1);
	return 1;
}
static int ai_test_op_2(void)
{
	ai_test_operator_set_config(stream[0].op_path, 2);
	return 1;
}
static int ai_test_op_3(void)
{
	ai_test_operator_set_config(stream[0].op_path, 3);
	return 1;
}
static int ai_test_op_4(void)
{
	ai_test_operator_set_config(stream[0].op_path, 4);
	return 1;
}
static int ai_test_op_5(void)
{
	ai_test_operator_set_config(stream[0].op_path, 5);
	return 1;
}
static int ai_test_op_6(void)
{
	ai_test_operator_set_config(stream[0].op_path, 6);
	return 1;
}
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
static int ai_test_op_7(void)
{
	ai_test_operator_set_config(stream[0].op_path, 7);
	return 1;
}
static int ai_test_op_8(void)
{
	ai_test_operator_set_config(stream[0].op_path, 8);
	return 1;
}
static int ai_test_op_9(void)
{
	ai_test_operator_set_config(stream[0].op_path, 9);
	return 1;
}
static int ai_test_op_10(void)
{
	ai_test_operator_set_config(stream[0].op_path, 10);
	return 1;
}
static int ai_test_op_11(void)
{
	ai_test_operator_set_config(stream[0].op_path, 11);
	return 1;
}
static int ai_test_op_12(void)
{
	ai_test_operator_set_config(stream[0].op_path, 12);
	return 1;
}
static int ai_test_op_13(void)
{
	ai_test_operator_set_config(stream[0].op_path, 13);
	return 1;
}
#endif

static int ai_test_op_get_workbuf_size(void)
{
	return ai_test_operator_get_workbuf_size(stream[0].op_path);
}

static int ai_test_op_set_workbuf(void)
{
	return ai_test_operator_set_workbuf(stream[0].op_path);
}

static int ai_test_op_add_llcmd(void)
{
	return ai_test_operator_add_llcmd(stream[0].op_path);
}



static HD_DBG_MENU ai_test_sel_op_menu[] = {
	//80~89
	{ 0, "FC",									 ai_test_op_0, TRUE},
	{ 1, "PREPROC (YUV2RGB)",					 ai_test_op_1, TRUE},
	{ 2, "PREPROC (YUV2RGB & scale)",			 ai_test_op_2, TRUE},
	{ 3, "PREPROC (YUV2RGB & meansub_plane)",	 ai_test_op_3, TRUE},
	{ 4, "PREPROC (YUV2RGB & meansub_dc)",		 ai_test_op_4, TRUE},
	{ 5, "PREPROC (UVPACK2UVPACK)",				 ai_test_op_5, TRUE},
	{ 6, "FC 	 (LL MODE)",				     ai_test_op_6, TRUE},
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
	{ 7, "MAU 	 (Matrix multiplication)",		 ai_test_op_7, TRUE},
	{ 8, "MAU 	 (Matrix addition)",		     ai_test_op_8, TRUE},
	{ 9, "MAU 	 (Matrix subtraction )",		 ai_test_op_9, TRUE},
	{10, "MAU 	 (L2 norm)",		            ai_test_op_10, TRUE},
	{11, "MAU 	 (L2 norm inverse)",	        ai_test_op_11, TRUE},
	{12, "MAU 	 (Vector Normalization)",       ai_test_op_12, TRUE},
	{13, "MAU 	 (Topn Sorting)",               ai_test_op_13, TRUE},
#endif
	
	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static HD_DBG_MENU ai_test_op_get_menu[] = {
	{1,  "op_get_work_buf_size",			ai_test_op_get_workbuf_size, 		TRUE},

	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static HD_DBG_MENU ai_test_op_set_menu[] = {
	{1,  "op_set_work_buf",			    ai_test_op_set_workbuf, 		TRUE},

	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static HD_DBG_MENU ai_test_op_add_menu[] = {
	{1,  "op_add_llcmd",			    ai_test_op_add_llcmd, 		TRUE},

	// escape muse be last
	{-1, "",					NULL,	 FALSE},
};

static int ai_test_op_select_op(void)
{
	return my_debug_menu_entry_p(ai_test_sel_op_menu, "SELECT OPERATION");
}

static int ai_test_op_get(void)
{
	return my_debug_menu_entry_p(ai_test_op_get_menu, "AI OP GET");
}

static int ai_test_op_set(void)
{
	return my_debug_menu_entry_p(ai_test_op_set_menu, "AI OP SET");
}

static int ai_test_op_add(void)
{
	return my_debug_menu_entry_p(ai_test_op_add_menu, "AI OP ADD");
}



////////////////////////////////////////////////////////////////////////////////
//MAIN

static int ai_test_init(void)
{
	HD_RESULT ret;
	UINT32 i = 0;
	for (i = 0; i < NN_RUN_NET_NUM; i++) {
		stream[i].in_path = 0;
		stream[i].net_path = i;
		stream[i].op_path = i;
	}

	//stream[0].in_path = 0;
	//stream[0].net_path = 0;
	//stream[0].op_path = 1;

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("hd_common_init fail=%d\n", ret);
		return 0;
	}
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_) || defined(_BSP_NA51102_)
	// set project config for AI
	hd_common_sysconfig(0, (1 << 16), 0, VENDOR_AI_CFG); //enable AI engine
#else
	ret = vendor_common_clear_pool_blk(HD_COMMON_MEM_CNN_POOL, 0);
	if (ret != HD_OK) {
		printf("vendor_common_clear_pool_blk fail=%d\n", ret);
		return 0;
	}
#endif
	// init mem
	{
		INT32 idx = 0; // mempool index

		// config common pool
		ai_test_input_mem_config(stream[0].in_path, &mem_cfg, 0, idx);
		for (i = 0; i < NN_RUN_NET_NUM; i++) {
			ai_test_network_mem_config(stream[i].net_path, &mem_cfg, &net_model_cfg, i);
		}
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_) || defined(_BSP_NA51102_)
		ret = hd_common_mem_init(&mem_cfg);
		if (HD_OK != ret) {
			printf("hd_common_mem_init error=%d\r\n", ret);
			return 0;
		}
#endif
	}

	if ((ret = ai_test_input_init()) != HD_OK) {
		printf("in init error=%d\n", ret);
		return 0;
	}
	if ((ret = ai_test_network_init()) != HD_OK) {
		printf("net init error=%d\n", ret);
		return 0;
	}
	if ((ret = ai_test_operator_init()) != HD_OK) {
		return 0;
	}
	return 1;
}

static int ai_test_proc_id(void)
{
	return my_debug_menu_entry_p(ai_test_sel_proc_id_menu, "SELECT PROC_ID");
}

static int ai_test_uninit(void)
{
	HD_RESULT ret;
	if ((ret = ai_test_input_uninit()) != HD_OK) {
		printf("in uninit error=%d\n", ret);
	}
	if ((ret = ai_test_network_uninit()) != HD_OK) {
		printf("net uninit error=%d\n", ret);
	}
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_) || defined(_BSP_NA51102_)
	// uninit memory
	ret = hd_common_mem_uninit();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}
#endif
	// uninit hdal
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
	}

	return 1;
}

static int ai_test_net_set_config(void)
{
	HD_RESULT ret;
	UINT32 i = 0;

	for (i = 0; i < NN_RUN_NET_NUM; i++) {
		ret = ai_test_network_set_config_job(stream[i].net_path, net_job_opt.method, net_job_opt.wait_ms);
		if (HD_OK != ret) {
			printf("proc_id(%lu) network_set_config_job fail=%d\n", stream[i].net_path, ret);
			return 0;
		}
		ret = ai_test_network_set_config_buf(stream[i].net_path, net_buf_opt.method, net_buf_opt.ddr_id);
		if (HD_OK != ret) {
			printf("proc_id(%lu) network_set_config_buf fail=%d\n", stream[i].net_path, ret);
			return 0;
		}
		ret = ai_test_network_set_config_model(stream[i].net_path, &net_model_cfg);
		if (HD_OK != ret) {
			printf("proc_id(%lu) network_set_config_model fail=%d\n", stream[i].net_path, ret);
			return 0;
		}
	}

	return 1;
}

static int ai_test_net_clr_config(void)
{
	HD_RESULT ret;
	UINT32 i = 0;

	for (i = 0; i < NN_RUN_NET_NUM; i++) {
		ret = ai_test_network_clr_config_model(stream[i].net_path);
		if (HD_OK != ret) {
			printf("proc_id(%lu) network_clr_config_model fail=%d\n", stream[i].net_path, ret);
			return 0;
		}
	}

	return 1;
}

static int ai_test_net_open(void)
{
	HD_RESULT ret;
	UINT32 i = 0;

	// set open config
    if (strlen(in_cfg_filename) != 0) {
		FILE *fp;
		UINT32 input_num = 0;
		UINT32 j;
		fp = fopen(in_cfg_filename, "r+");
		fscanf(fp, "%lu", &input_num);
		for (i = 0; i < NN_RUN_NET_NUM; i++) {
    		stream[i].input_blob_num = input_num;
    	}
		for(j=0; j < input_num; j++) {
			// parse input config
				fscanf(fp, "%s %lu %lu %lu %lu %lu %lu %x %lu", 
				in_cfg.input_filename,
				&in_cfg.w,
				&in_cfg.h,
				&in_cfg.c,
				&in_cfg.b,
	            &in_cfg.bitdepth,
				&in_cfg.loff,
				(unsigned int *)&in_cfg.fmt,
				&in_cfg.is_comb_img);
				in_cfg.input_filename[256-1] = '\0';
			for (i = 0; i < NN_RUN_NET_NUM; i++) {
				set_batch_num[i][j] = in_cfg.b;
			}
			ret = ai_test_input_set_config(j, &in_cfg);
			if (HD_OK != ret) {
				printf("in_id(%lu) input_set_config fail=%d\n", j, ret);
				return 0;
			}
			if ((ret = ai_test_input_open(j)) != HD_OK) {
				printf("in open error=%d\n", ret);
				return 0;
			}
		}
		fclose(fp);

    } else {
    	for (i = 0; i < NN_RUN_NET_NUM; i++) {
    		stream[i].input_blob_num = 1;
			set_batch_num[i][0] = 1;
    	}
		ret = ai_test_input_set_config(stream[0].in_path, &in_cfg);
		if (HD_OK != ret) {
			printf("in_id(%lu) input_set_config fail=%d\n", stream[0].in_path, ret);
			return 0;
		}
		if ((ret = ai_test_input_open(stream[0].in_path)) != HD_OK) {
			printf("in open error=%d\n", ret);
			return 0;
		}
    }
	for (i = 0; i < NN_RUN_NET_NUM; i++) {
		if ((ret = ai_test_network_open(stream[i].net_path)) != HD_OK) {
			printf("net open error=%d\n", ret);
			return 0;
		}
	}
	
	return 1;
}

static int ai_test_net_close(void)
{
	HD_RESULT ret;
	UINT32 i = 0;
	UINT32 j;
	for(j=0; j < stream[0].input_blob_num; j++) {
		if ((ret = ai_test_input_close(j)) != HD_OK) {
			printf("in close error=%d\n", ret);
			return 0;
		}
	}
	for (i = 0; i < NN_RUN_NET_NUM; i++) {
		if ((ret = ai_test_network_close(stream[i].net_path)) != HD_OK) {
			printf("net close error=%d\n", ret);
			return 0;
		}
	}

	return 1;
}

static int ai_test_net_start(void)
{
	UINT32 i = 0;
	ai_test_input_start(stream[0].in_path);
	for (i = 0; i < NN_RUN_NET_NUM; i++) {
		ai_test_network_user_start(&stream[i]);
	}
	sleep(1);

	return 1;
}

static int ai_test_net_stop(void)
{
	ai_test_input_stop(stream[0].in_path);
	UINT32 i = 0;
	for (i = 0; i < NN_RUN_NET_NUM; i++) {
		ai_test_network_user_stop(&stream[i]);
	}
	return 1;
}

static int ai_test_net_proc(void)
{
	UINT32 i = 0;
	for (i = 0; i < NN_RUN_NET_NUM; i++) {
		ai_test_network_user_oneshot(&stream[i], my_debug_get_menu_name());
	}
	sleep(1);

	return 1;
}

static int ai_test_net_proc_multitime(void)
{
	ai_test_network_user_multishot(&stream[0], my_debug_get_menu_name(), 3000);  // shot for 10 second
	return 1;
}

static int ai_test_op_open(void)
{
	HD_RESULT ret;
	
	// set open config
	ret = ai_test_input_set_config(stream[0].in_path, &in_cfg);
	if (HD_OK != ret) {
		printf("proc_id(%lu) input_set_config fail=%d\n", stream[0].in_path, ret);
		return 0;
	}

	if ((ret = ai_test_operator_open(stream[0].op_path, stream[0].in_path)) != HD_OK) {
		printf("operator open error=%d\n", ret);
		return 0;
	}
	
	return 1;
}

static int ai_test_op_close(void)
{
	HD_RESULT ret;
	if ((ret = ai_test_operator_close(stream[0].op_path)) != HD_OK) {
		printf("net close error=%d\n", ret);
		return 0;
	}
	return 1;
}

static int ai_test_op_start(void)
{
	ai_test_operator_user_start(&stream[0]);
	sleep(1);

	return 1;
}

static int ai_test_op_stop(void)
{
	ai_test_operator_user_stop(&stream[0]);

	return 1;
}

static int ai_test_op_proc(void)
{
	ai_test_operator_user_oneshot(&stream[0]);
	sleep(1);
	
	return 1;
}

extern HD_RESULT _vendor_ai_net_debug_layer(UINT32 proc_id, UINT32 layer_opt, CHAR *filedir);
static int ai_test_debug_dumpinfo(void)
{
	return 1;
}
static int ai_test_debug_dumpall(void)
{
	if (dump_dir) {
		_vendor_ai_net_debug_layer(0, 0, dump_dir);
		printf("!!## dump_layer done ##!!\n");
	}
	return 1;
}

static int ai_test_debug_dumpout(void)
{
	if (dump_dir) {
		_vendor_ai_net_debug_layer(0, 1, dump_dir);
		printf("!!## dump_layer done ##!!\n");
	}
	return 1;
}

extern HD_RESULT _vendor_ai_net_debug_performance(UINT32 proc_id, CHAR *p_model_name, UINT64 *p_proc_time, DOUBLE p_cpu_loading);
static int ai_test_debug_performance(void)
{
	_vendor_ai_net_debug_performance(0, net_model_cfg.model_filename, net_proc_time, net_cpu_loading);
	printf("!!## dump performance done ##!!\n");
	return 1;
}

static HD_DBG_MENU ai_test_main_menu[] = {
	//-------------------------------------------------------------------------
	//0~
	{ 1, "init",  				ai_test_init,		TRUE}, //vendor_ai_init
	{ 2, "(proc_id)", 			ai_test_proc_id,	TRUE}, //...(select proc_id)
	{ 9, "uninit", 				ai_test_uninit, 	TRUE}, //vendor_ai_uninit

	//-------------------------------------------------------------------------
	{11, "(net_set_cfg)",  		ai_test_net_set_config,	TRUE}, //...(set default proc_id, load model, set config)
	{12, "net_open",  			ai_test_net_open,	TRUE}, //vendor_ai_net_open
	{13, "net_start",  			ai_test_net_start,	TRUE}, //vendor_ai_net_start
	{14, "net_proc", 			ai_test_net_proc,	TRUE}, //vendor_ai_net_proc
	{15, "net_stop",  			ai_test_net_stop,	TRUE}, //vendor_ai_net_stop
	{16, "net_close",  			ai_test_net_close,	TRUE}, //vendor_ai_net_close
	{17, "net_set", 			ai_test_net_set,	TRUE}, //vendor_ai_net_set
	{18, "net_get", 			ai_test_net_get,	TRUE}, //vendor_ai_net_get
	{19, "(net_clr_cfg)",		ai_test_net_clr_config,	TRUE}, // //...(unload model)

	//-------------------------------------------------------------------------
	{21, "(op_select_op)", 		ai_test_op_select_op,	TRUE}, //...(select operation)
	{22, "op_open", 			ai_test_op_open,	TRUE}, //vendor_ai_op_open
	{23, "op_start", 			ai_test_op_start,	TRUE}, //vendor_ai_op_start
	{24, "op_proc", 			ai_test_op_proc,	TRUE}, //vendor_ai_op_proc
	{25, "op_stop", 			ai_test_op_stop,	TRUE}, //vendor_ai_op_stop
	{26, "op_close",			ai_test_op_close,	TRUE}, //vendor_ai_op_close
	{27, "op_set", 				ai_test_op_set,	    TRUE}, //vendor_ai_op_set
	{28, "op_get", 				ai_test_op_get,     TRUE}, //vendor_ai_op_get
	{29, "op_add", 				ai_test_op_add,     TRUE}, //vendor_ai_op_add

	//-------------------------------------------------------------------------
	//80
	{80, "flow", 				ai_test_flow,		TRUE}, //(fixed flow)

	//-------------------------------------------------------------------------
	//90~99
	{90, "dump_info",  			ai_test_debug_dumpinfo,	TRUE},
	{91, "dump_outbuf",			ai_test_debug_dumpout,	TRUE},
	{92, "dump_allbuf", 		ai_test_debug_dumpall,	TRUE},

	//-------------------------------------------------------------------------

	// escape muse be last
	{-1, "",               		NULL,    FALSE},
};


MAIN(argc, argv)
{
	HD_RESULT ret= 0;

	if (argc == 1) {
		
		printf("Enter MENU mode...\n");
		my_debug_menu_entry_p(ai_test_main_menu, "AI MAIN");
		printf("Exit MENU mode.\n");

	} else if (argc == 2) {

		printf("Enter op file mode...\n");
		ai_test_init();
		ai_test_op_config_parser(argv[1]);
		ai_test_operator_set_config_by_file(stream[0].op_path);
		ai_test_op_open();
		ai_test_op_get_workbuf_size();
		ai_test_op_set_workbuf();
		ai_test_op_start();
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_)
		if(op_config_mode == 1) {
			ai_test_op_add_llcmd();
		}
#endif
		ai_test_op_proc();
		ai_test_op_stop();
		ai_test_op_close();
		ai_test_uninit();		
		
	} else {

		INT key;

		printf("Enter CMD mode...\n");
		if (argc < 12) ai_test_cmd_load_user_option_multi(argc, argv);
		else ai_test_cmd_load_user_option(argc, argv);
		ai_test_init();
		ai_test_net_set_config();
		printf("Enter o to open\n");
		printf("Enter r to start\n");
		printf("Enter p to proc\n");
		printf("Enter s to stop\n");
		printf("Enter c to close\n");
		printf("Enter q to quit\n");
		printf("--------------------------------\n");
		printf("Enter d to dump result_layer bin\n");
		printf("Enter u to dump network_performance\n");
		printf("Enter m to proc multi-time (run 10 second)\n");
		printf("Enter w to dynamic set batch ID\n");
		printf("Enter x to dynamic set batch number\n");
		printf("Enter y to set resolution width, height\n");
		printf("Enter z to set resolution id\n");
		printf("Enter k to dynamic set batch ID (immediately)\n");
		printf("Enter l to dynamic set batch number (immediately)\n");

		do {
			key = GETCHAR();
			if (key == 'o') {
				ai_test_net_open();
				continue;
			}
			if (key == 'r') {
				ai_test_net_start();
				continue;
			}
			if (key == 'p') {
				ai_test_net_proc();
				continue;
			}
			if (key == 'm') {
				ai_test_net_proc_multitime();
				continue;
			}
			if (key == 's') {
				ai_test_net_stop();
				continue;
			}
			if (key == 'c') {
				ai_test_net_close();
				continue;
			}
			if (key == 'd') {
				ai_test_debug_dumpout();
				continue;
			}
			if (key == 'u') {
				ai_test_debug_performance();
				continue;
			}
			if (key == 'f') {
				ai_test_debug_dumpall();
				continue;
			}
			/* dynamic set batch ID (change batch ID), set before start (after stop) */
			if (key == 'w') {
				UINT32 idx = 2;
				printf("Input batch ID:\n");
				scanf("%lu", &idx);
				vendor_ai_net_set(0, VENDOR_AI_NET_PARAM_BATCH_ID, &idx);
				printf("set VENDOR_AI_NET_PARAM_BATCH_ID dim(%lu)\n", idx);
				printf("update start (by ID)\n");
				continue;
			}

			/* dynamic set batch number (change batch number), set before start (after stop) */
			if (key == 'x') {
				UINT32 idx = 2;
				UINT32 i,j;
				FLOAT batch_ratio = 1;
				NET_IN* p_in = NULL;
				printf("Input batch number:\n");
				scanf("%lu", &idx);
				printf("set VENDOR_AI_NET_PARAM_BATCH_N idx(%lu)\n", idx);
				printf("update start (by number)\n");
				for (i = 0; i < NN_RUN_NET_NUM; i++) {
					vendor_ai_net_set(i, VENDOR_AI_NET_PARAM_BATCH_N, &idx);
					for(j = 0;j < stream[i].input_blob_num; j++) {
						p_in = g_in + j;
						if (j == 0) {
							set_batch_num[i][j] = idx;
							batch_ratio = (FLOAT)p_in->in_cfg.b / idx;
						} else {
							set_batch_num[i][j] = (UINT32)(AI_TEST_ROUND((FLOAT)p_in->in_cfg.b / batch_ratio));
						}
					}
				}
				continue;
			}

			/* dynamic set batch ID (change batch ID), set only after start */
			if (key == 'k') {
				UINT32 idx = 2;
				printf("Input batch ID:\n");
				scanf("%lu", &idx);
				vendor_ai_net_set(0, VENDOR_AI_NET_PARAM_BATCH_ID_IMM, &idx);
				printf("set VENDOR_AI_NET_PARAM_BATCH_ID dim(%lu)\n", idx);
				printf("update start (by ID)\n");
				continue;
			}

			/* dynamic set batch number (change batch number), set only after start */
			if (key == 'l') {
				UINT32 idx = 2;
				UINT32 i,j;
				printf("Input batch number:\n");
				scanf("%lu", &idx);
				printf("set VENDOR_AI_NET_PARAM_BATCH_N idx(%lu)\n", idx);
				printf("update start (by number)\n");
				for (i = 0; i < NN_RUN_NET_NUM; i++) {
					vendor_ai_net_set(i, VENDOR_AI_NET_PARAM_BATCH_N_IMM, &idx);
					for(j = 0;j < stream[i].input_blob_num; j++) {
						set_batch_num[i][j] = idx;
					}
				}
				continue;
			}

			if (key == 'y') {
				HD_DIM dim = {0};
				printf("Enter resolution width, height: ");
				scanf("%lu %lu",&(dim.w),&(dim.h));
				vendor_ai_net_set(0, VENDOR_AI_NET_PARAM_RES_DIM, &dim);
				printf("set VENDOR_AI_NET_PARAM_RES_DIM\n");
				continue;
			}
			if (key == 'z') {
				UINT32 idx = 0;
				printf("Enter resolution id: ");
				scanf("%lu",&idx);
				vendor_ai_net_set(0, VENDOR_AI_NET_PARAM_RES_ID, &idx);
				printf("set VENDOR_AI_NET_PARAM_RES_DIM, res_id = %lu\n",idx);
				continue;
			}
			if (key == 'q' || key == 0x3) {
				break;
			}
		} while(1);
		
		ai_test_net_clr_config();
		ai_test_uninit();
		printf("Exit CMD mode.\n");
	}
	printf("\r\n\r\n");

	return ret;
}


