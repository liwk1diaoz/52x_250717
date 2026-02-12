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
#include <sys/time.h>
#include "ai_test_config.h"
#include "ai_test_op.h"
#include "hd_debug.h"
#include "hd_gfx.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <signal.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>

///////////////////////////////////////////////////////////////////////////////

#define VENDOR_AI_CFG  	0x000f0000  //vendor ai config

#define HD_COMMON_CFG_MULTIPROC        0x80000000    //common R4 - support multiple process
#define VENDOR_AI_MAX_PROCESS          3             //main + sub1 + sub2
#define VENDOR_AI_TEST_SHM_KEY         0xff00

///////////////////////////////////////////////////////////////////////////////

// global context
GLOBAL_DATA global_data;
int         global_fail = 0;
PROCESS_DATA process_data;

int main_process(void);
int test = 0 ;
int sem_used_cnt = 0; 
int flag_used_cnt = 0;
///////////////////////////////////////////////////////////////////////////////

static void* malloc_shm(size_t size)
{
	PROCESS_DATA* p_process_data = ai_test_get_process_data();
	char *p_shm = NULL;
	key_t key = VENDOR_AI_TEST_SHM_KEY;

	if (p_process_data->process_id == 0) {
		if( ( p_process_data->shm_id = shmget( key, size, IPC_CREAT | 0666 ) ) < 0 ) {
			perror( "shmget" );
			exit(1);
		}
		if( ( p_shm = shmat( p_process_data->shm_id, NULL, 0 ) ) == (char *)-1 ) {
			perror( "shmat" );
			exit(1);
		}
	} else {
		if( ( p_process_data->shm_id = shmget( key, size, 0666 ) ) < 0 ) {
			return NULL;
		}
		if( ( p_shm = shmat( p_process_data->shm_id, NULL, 0 ) ) == (char *)-1 ) {
			return NULL;
		}
	}

	return p_shm;
}

static void free_shm(void* p_shm)
{
	PROCESS_DATA* p_process_data = ai_test_get_process_data();

	if (p_process_data->process_id == 0) {
		if (p_shm) {
			shmdt(p_shm);
			shmctl(p_process_data->shm_id, IPC_RMID, NULL);
		}
	} else {
		if (p_shm) {
			shmdt(p_shm);
		}
	}
}

void ai_test_signal_hdanler(int sig)
{
	//assert(sig == SIGQUIT);
	if (process_data.parent_id != getpid()) {
		exit(7);
	}
}

static void ai_test_signal_quit(void)
{
	printf("pid %d: killing children\n", getpid());
	kill(0, SIGQUIT);
}

static void ai_test_signal_kill(pid_t pid)
{
	printf("pid %d: killing child pid(%d)\n", getpid(), pid);
	kill(pid, SIGKILL);
}

///////////////////////////////////////////////////////////////////////////////

static int hdal_init(void)
{
	HD_RESULT ret;
	GLOBAL_DATA* p_global_data = ai_test_get_global_data();
	PROCESS_DATA* p_process_data = ai_test_get_process_data();

	// init hdal
	if (p_global_data->multi_process) {
		if (p_process_data->process_id == 0) {
			ret = hd_common_init(HD_COMMON_CFG_MULTIPROC | 0);
		} else {
			ret = hd_common_init(HD_COMMON_CFG_MULTIPROC | 1);
		}
	} else {
		ret = hd_common_init(0);
	}
	if (ret != HD_OK) {
		printf("hd_common_init fail=%d\n", ret);
		return -1;
	}
	// set project config for AI
	hd_common_sysconfig(0, (1<<16), 0, VENDOR_AI_CFG); //enable AI engine

	// config common pool
	ret = ai_test_common_mem_config(&global_data);
	if(HD_OK != ret){
		printf("ai_test_common_mem_config() fail error=%d\r\n", ret);
		return -1;
	}

	if (p_global_data->multi_process) {
		if (p_process_data->process_id == 0) {
			ret = hd_common_mem_init(&global_data.mem_cfg);
		} else {
			ret = hd_common_mem_init(NULL);
		}
	} else {
		ret = hd_common_mem_init(&global_data.mem_cfg);
	}
	if (HD_OK != ret) {
		printf("hd_common_mem_init error=%d\r\n", ret);
		return -1;
	}

	if ((ret = hd_gfx_init()) != HD_OK)
		return ret;

	return 0;
}

static int hdal_uninit(void)
{
	HD_RESULT ret;

	if ((ret = hd_gfx_uninit()) != HD_OK)
		return ret;

	// uninit memory
	ret = hd_common_mem_uninit();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}

	// uninit hdal
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
	}

	return 0;
}

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

static INT32 split_kvalue2(CHAR *p_str, CONFIG_OPT* p_config, CHAR c)
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


static INT32 ai_test_model_image_list_parser(int *in_file_cfg_count, NET_IN_FILE_CONFIG **pp_in_file_cfg, CHAR* file_path)
{
	FILE *fp_in = NULL;
	FILE *fp_in_list = NULL;
	UINT32 i, j;
	UINT32 input_num, input_file_num;
	CHAR line[256];
	CHAR fmt[50] = { 0 };
	UINT32 in_buf_unset = 0, in_buf_zero_size = 0, in_buf_null = 0, in_buf_zero_addr = 0, in_buf_unaligned_addr = 0;
	UINT32 proc_after_restart = 0, proc_without_set_input = 0;
	NET_IN_FILE_CONFIG *in_file_cfg = NULL;

	if(!in_file_cfg_count || !pp_in_file_cfg || !file_path){
		printf("in_file_cfg_count(%p) or pp_in_file_cfg(%p) or file_path(%p) is null\n", in_file_cfg_count, pp_in_file_cfg, file_path);
		return 0;
	}

	if ((fp_in_list = fopen(file_path, "r")) == NULL)
	{
		printf("%s is not exist!\n", file_path);
		return 0;
	}

	fscanf(fp_in_list, "%u", (unsigned int*)&input_file_num);

	*in_file_cfg_count = input_file_num;

	//printf("input_file_num=%d\r\n", input_file_num);

	in_file_cfg = (NET_IN_FILE_CONFIG*)malloc(sizeof(NET_IN_FILE_CONFIG)*input_file_num);
	memset(in_file_cfg, 0, sizeof(NET_IN_FILE_CONFIG)*input_file_num);
	*pp_in_file_cfg = in_file_cfg;

	for (i = 0; i < input_file_num; i++) {
		if (feof(fp_in_list))
			break;

		in_buf_unset = 0;
		in_buf_zero_size = 0;
		in_buf_null = 0;
		in_buf_zero_addr = 0;
		in_buf_unaligned_addr = 0;
		fscanf(fp_in_list, "%s %u %u %u %u %u %u %u", line,
							(unsigned int*)&in_buf_unset,
							(unsigned int*)&in_buf_zero_size,
							(unsigned int*)&in_buf_null,
							(unsigned int*)&in_buf_zero_addr,
							(unsigned int*)&proc_after_restart,
							(unsigned int*)&proc_without_set_input,
							(unsigned int*)&in_buf_unaligned_addr);

		line[256-1] = '\0';

		//printf("[%d]=%s\r\n", i, line);

		if ((fp_in = fopen(line, "r")) == NULL)
		{
			printf("%s is not exist!\n", line);
			return 0;
		}

		fscanf(fp_in, "%u", (unsigned int*)&input_num);

		//printf("input_num=%d\r\n", input_num);

		strncpy(in_file_cfg[i].in_file_filename, line, 256-1);

		in_file_cfg[i].input_num = input_num;
		in_file_cfg[i].in_buf_unset = in_buf_unset;
		in_file_cfg[i].in_buf_zero_size = in_buf_zero_size;
		in_file_cfg[i].in_buf_null = in_buf_null;
		in_file_cfg[i].in_buf_zero_addr = in_buf_zero_addr;
		in_file_cfg[i].in_buf_unaligned_addr = in_buf_unaligned_addr;
		in_file_cfg[i].proc_after_restart = proc_after_restart;
		in_file_cfg[i].proc_without_set_input = proc_without_set_input;
		in_file_cfg[i].in_cfg = (NET_IN_CONFIG*)malloc(sizeof(NET_IN_CONFIG)*input_num);
		memset(in_file_cfg[i].in_cfg, 0, sizeof(NET_IN_CONFIG)*input_num);

		for(j=0; j < input_num; j++) {
			// parse input config
				fscanf(fp_in, "%s %u %u %u %u %u %u %s %u %u %u %u",
				in_file_cfg[i].in_cfg[j].input_filename,
				(unsigned int*)&in_file_cfg[i].in_cfg[j].w,
				(unsigned int*)&in_file_cfg[i].in_cfg[j].h,
				(unsigned int*)&in_file_cfg[i].in_cfg[j].c,
				(unsigned int*)&in_file_cfg[i].in_cfg[j].batch,
				(unsigned int*)&in_file_cfg[i].in_cfg[j].loff,
				(unsigned int*)&in_file_cfg[i].in_cfg[j].bit,
				fmt,
				(unsigned int*)&in_file_cfg[i].in_cfg[j].verify_result,
				(unsigned int*)&in_file_cfg[i].in_cfg[j].invalid,
				(unsigned int*)&in_file_cfg[i].in_cfg[j].is_comb_img,
				(unsigned int*)&in_file_cfg[i].in_cfg[j].float_to_fix
				);


				if (strcmp(fmt, "HD_VIDEO_PXLFMT_YUV420") == 0) {
					in_file_cfg[i].in_cfg[j].fmt = 0x520c0420;
				}
				else if (strcmp(fmt, "HD_VIDEO_PXLFMT_Y8") == 0) {
					in_file_cfg[i].in_cfg[j].fmt = 0x51080400;
				}
				else if (strcmp(fmt, "HD_VIDEO_PXLFMT_RGB888_PLANAR") == 0) {
					in_file_cfg[i].in_cfg[j].fmt = 0x23180888;
				}
				else if (strcmp(fmt, "HD_VIDEO_PXLFMT_BGR888_PLANAR") == 0) {
					in_file_cfg[i].in_cfg[j].fmt = 0x2B180888;
				}
				else if (strcmp(fmt, "HD_VIDEO_PXLFMT_AI_UINT8") == 0) {
					in_file_cfg[i].in_cfg[j].fmt = 0xa2080800;
				}
				else {
					sscanf(fmt, "%x", (unsigned int*)&(in_file_cfg[i].in_cfg[j].fmt));
				}

				in_file_cfg[i].in_cfg[j].input_filename[256-1] = '\0';

				if(global_data.verify_result == -1){
					in_file_cfg[i].in_cfg[j].verify_result = 0;
				}else if(global_data.verify_result == 1){
					in_file_cfg[i].in_cfg[j].verify_result = 1;
				}

				if(global_data.float_to_fix == -1){
					in_file_cfg[i].in_cfg[j].float_to_fix = 0;
				}else if(global_data.float_to_fix == 1){
					in_file_cfg[i].in_cfg[j].float_to_fix = 1;
				}

				#if 0
				printf("[%d] in=%s, w=%u, h=%u, c=%u, b=%u, loff=%u, bit=%u, fmt=%s, verify=%u, invalid=%u, is_comb_img=%u\r\n", j, in_file_cfg[i].in_cfg[j].input_filename,
				in_file_cfg[i].in_cfg[j].w,
				in_file_cfg[i].in_cfg[j].h,
				in_file_cfg[i].in_cfg[j].c,
				in_file_cfg[i].in_cfg[j].batch,
				in_file_cfg[i].in_cfg[j].loff,
				in_file_cfg[i].in_cfg[j].bit,
				fmt,
				in_file_cfg[i].in_cfg[j].verify_result,
				in_file_cfg[i].in_cfg[j].invalid,
				in_file_cfg[i].in_cfg[j].is_comb_img);
				#endif
		}
		fclose(fp_in);
	}

	fclose(fp_in_list);

	return 1;
}

static INT32 ai_test_op_image_list_parser(int *in_file_cfg_count, NET_IN_FILE_CONFIG **pp_in_file_cfg, CHAR* file_path)
{
	FILE *fp_in = NULL;
	FILE *fp_in_list = NULL;
	UINT32 i, j;
	UINT32 input_num, input_file_num;
	CHAR line[256];
	CHAR in_fmt[50] = { 0 }, out_fmt[50] = { 0 };
	UINT32 in_buf_unset, in_buf_zero_size, in_buf_null, in_buf_zero_addr, in_buf_unaligned_addr;
	NET_IN_FILE_CONFIG *in_file_cfg = NULL;

	if(!in_file_cfg_count || !pp_in_file_cfg || !file_path){
		printf("in_file_cfg_count(%p) or pp_in_file_cfg(%p) or file_path(%p) is null\n", in_file_cfg_count, pp_in_file_cfg, file_path);
		return 0;
	}

	if ((fp_in_list = fopen(file_path, "r")) == NULL)
	{
		printf("%s is not exist!\n", file_path);
		return 0;
	}

	fscanf(fp_in_list, "%u", (unsigned int*)&input_file_num);

	*in_file_cfg_count = input_file_num;

	//printf("input_file_num=%d\r\n", input_file_num);

	in_file_cfg = (NET_IN_FILE_CONFIG*)malloc(sizeof(NET_IN_FILE_CONFIG)*input_file_num);
	memset(in_file_cfg, 0, sizeof(NET_IN_FILE_CONFIG)*input_file_num);
	*pp_in_file_cfg = in_file_cfg;

	for (i = 0; i < input_file_num; i++) {
		if (feof(fp_in_list))
			break;

		in_buf_unset = 0;
		in_buf_zero_size = 0;
		in_buf_null = 0;
		in_buf_zero_addr = 0;
		in_buf_unaligned_addr = 0;
		fscanf(fp_in_list, "%s %u %u %u %u %u", line,
							(unsigned int*)&in_buf_unset,
							(unsigned int*)&in_buf_zero_size,
							(unsigned int*)&in_buf_null,
							(unsigned int*)&in_buf_zero_addr,
							(unsigned int*)&in_buf_unaligned_addr);

		line[256-1] = '\0';

		//printf("[%d]=%s\r\n", i, line);

		if ((fp_in = fopen(line, "r")) == NULL)
		{
			printf("%s is not exist!\n", line);
			return 0;
		}

		fscanf(fp_in, "%u", (unsigned int*)&input_num);

		//printf("input_num=%d\r\n", input_num);

		strncpy(in_file_cfg[i].in_file_filename, line, 256-1);

		in_file_cfg[i].input_num = input_num;
		in_file_cfg[i].in_buf_unset = in_buf_unset;
		in_file_cfg[i].in_buf_zero_size = in_buf_zero_size;
		in_file_cfg[i].in_buf_null = in_buf_null;
		in_file_cfg[i].in_buf_zero_addr = in_buf_zero_addr;
		in_file_cfg[i].in_buf_unaligned_addr = in_buf_unaligned_addr;
		in_file_cfg[i].in_out_cfg = (NET_IN_OUT_CONFIG*)malloc(sizeof(NET_IN_OUT_CONFIG)*input_num);
		memset(in_file_cfg[i].in_out_cfg, 0, sizeof(NET_IN_OUT_CONFIG)*input_num);

		for(j=0; j < input_num; j++) {
			// parse input config
				fscanf(fp_in, "%s %u %u %u %u %s %u %u %u %u %s %u %u",
				in_file_cfg[i].in_out_cfg[j].input_filename,
				(unsigned int*)&in_file_cfg[i].in_out_cfg[j].in_w,
				(unsigned int*)&in_file_cfg[i].in_out_cfg[j].in_h,
				(unsigned int*)&in_file_cfg[i].in_out_cfg[j].in_c,
				(unsigned int*)&in_file_cfg[i].in_out_cfg[j].in_loff,
				in_fmt,
				(unsigned int*)&in_file_cfg[i].in_out_cfg[j].out_w,
				(unsigned int*)&in_file_cfg[i].in_out_cfg[j].out_h,
				(unsigned int*)&in_file_cfg[i].in_out_cfg[j].out_c,
				(unsigned int*)&in_file_cfg[i].in_out_cfg[j].out_loff,
				out_fmt,
				(unsigned int*)&in_file_cfg[i].in_out_cfg[j].verify_result,
				(unsigned int*)&in_file_cfg[i].in_out_cfg[j].invalid);

				if (strcmp(in_fmt, "HD_VIDEO_PXLFMT_YUV420") == 0) {
					in_file_cfg[i].in_out_cfg[j].in_fmt = 0x520c0420;
				}
				else if (strcmp(in_fmt, "HD_VIDEO_PXLFMT_Y8") == 0) {
					in_file_cfg[i].in_out_cfg[j].in_fmt = 0x51080400;
				}
				else if (strcmp(in_fmt, "HD_VIDEO_PXLFMT_RGB888_PLANAR") == 0) {
					in_file_cfg[i].in_out_cfg[j].in_fmt = 0x23180888;
				}
				else {
					sscanf(in_fmt, "%x", (unsigned int*)&(in_file_cfg[i].in_out_cfg[j].in_fmt));
				}

				if (strcmp(out_fmt, "HD_VIDEO_PXLFMT_YUV420") == 0) {
					in_file_cfg[i].in_out_cfg[j].out_fmt = 0x520c0420;
				}
				else if (strcmp(out_fmt, "HD_VIDEO_PXLFMT_Y8") == 0) {
					in_file_cfg[i].in_out_cfg[j].out_fmt = 0x51080400;
				}
				else if (strcmp(out_fmt, "HD_VIDEO_PXLFMT_RGB888_PLANAR") == 0) {
					in_file_cfg[i].in_out_cfg[j].out_fmt = 0x23180888;
				}
				else {
					sscanf(out_fmt, "%x", (unsigned int*)&(in_file_cfg[i].in_out_cfg[j].out_fmt));
				}

				in_file_cfg[i].in_out_cfg[j].input_filename[256-1] = '\0';
		}
		fclose(fp_in);
	}

	fclose(fp_in_list);

	return 1;
}

static INT32 ai_test_global_config_parser(CHAR *file_path)
{

	FILE *fp_tmp = NULL;
	int first_loop_count = 0;


	if ((fp_tmp = fopen(file_path, "r")) == NULL)
	{
		printf("%s is not exist!\n", file_path);
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

		if (strcmp(tmp.key, "[first_loop_count]") == 0) {
			if (global_data.multi_process) {
				global_data.first_loop_count = atoi(tmp.value);
				global_data.first_loop_data = malloc(sizeof(FIRST_LOOP_DATA)*global_data.first_loop_count * (process_data.process_num-1));
				memset(global_data.first_loop_data, 0, sizeof(FIRST_LOOP_DATA)*global_data.first_loop_count * (process_data.process_num-1));
			} else {
				global_data.first_loop_count = atoi(tmp.value);
				global_data.first_loop_data = malloc(sizeof(FIRST_LOOP_DATA)*global_data.first_loop_count);
				memset(global_data.first_loop_data, 0, sizeof(FIRST_LOOP_DATA)*global_data.first_loop_count);
			}
		}

		if (strcmp(tmp.key, "[sequential]") == 0) {
			global_data.sequential = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[sequential_loop_count]") == 0) {
			global_data.sequential_loop_count = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[timeout]") == 0) {
			global_data.timeout = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[cpu_stess_level_1]") == 0) {
			global_data.cpu_stress[0].stress_level = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[cpu_stess_level_2]") == 0) {
			global_data.cpu_stress[1].stress_level = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[dma_stess_level_1]") == 0) {
			global_data.dma_stress[0].stress_level = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[dma_stess_level_2]") == 0) {
			global_data.dma_stress[1].stress_level = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[first_loop_count_path]") == 0) {
			if (global_data.first_loop_data) {
				if (first_loop_count < global_data.first_loop_count) {
					strncpy(global_data.first_loop_data[first_loop_count].first_loop_path, tmp.value, 256-1);
					global_data.first_loop_data[first_loop_count].first_loop_path[256-1] = '\0';
					first_loop_count++;
				}
			}
		}

		if (strcmp(tmp.key, "[ai_proc_schedule]") == 0) {
			global_data.ai_proc_schedule = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[perf_compare]") == 0) {
			global_data.perf_compare = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[disable_perf_compare_last]") == 0) {
			global_data.disable_perf_compare_last = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[disable_perf_compare_cpu]") == 0) {
			global_data.disable_perf_compare_cpu= atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[disable_perf_compare_dma]") == 0) {
			global_data.disable_perf_compare_dma = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[time_threshold_upper]") == 0) {
			global_data.time_threshold_upper = atof(tmp.value);
		}

		if (strcmp(tmp.key, "[time_threshold_upper_short]") == 0) {
			global_data.time_threshold_upper_short = atof(tmp.value);
		}

		if (strcmp(tmp.key, "[time_threshold_lower]") == 0) {
			global_data.time_threshold_lower = atof(tmp.value);
		}

		if (strcmp(tmp.key, "[bw_threshold_upper]") == 0) {
			global_data.bw_threshold_upper = atof(tmp.value);
		}

		if (strcmp(tmp.key, "[bw_threshold_lower]") == 0) {
			global_data.bw_threshold_lower = atof(tmp.value);
		}

		if (strcmp(tmp.key, "[size_threshold_upper]") == 0) {
			global_data.size_threshold_upper = atof(tmp.value);
		}

		if (strcmp(tmp.key, "[size_threshold_lower]") == 0) {
			global_data.size_threshold_lower = atof(tmp.value);
		}

		if (strcmp(tmp.key, "[memory_threshold_upper]") == 0) {
			global_data.memory_threshold_upper = atof(tmp.value);
		}

		if (strcmp(tmp.key, "[memory_threshold_lower]") == 0) {
			global_data.memory_threshold_lower = atof(tmp.value);
		}

		if (strcmp(tmp.key, "[loading_threshold_upper]") == 0) {
			global_data.loading_threshold_upper = atof(tmp.value);
		}

		if (strcmp(tmp.key, "[loading_threshold_lower]") == 0) {
			global_data.loading_threshold_lower = atof(tmp.value);
		}

		if (strcmp(tmp.key, "[dump_output]") == 0) {
			global_data.dump_output = atof(tmp.value);
		}

		if (strcmp(tmp.key, "[load_output]") == 0) {
			global_data.load_output = atof(tmp.value);
		}

		if (strcmp(tmp.key, "[generate_golden_output]") == 0) {
			strncpy(global_data.generate_golden_output, tmp.value, 256-1);
		}

		if (strcmp(tmp.key, "[verify_result]") == 0) {
			global_data.verify_result = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[float_to_fix]") == 0) {
			global_data.float_to_fix = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[net_supported_num_lower]") == 0) {
			global_data.net_supported_num_lower = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[net_supported_num_upper]") == 0) {
			global_data.net_supported_num_upper = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[net_supported_num_check]") == 0) {
			global_data.net_supported_num_check = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[share_model]") == 0) {
			global_data.share_model = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[init_uninit_per_thread]") == 0) {
			global_data.init_uninit_per_thread = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[perf_result_filename]") == 0) {
			strncpy(global_data.perf_result_filename, tmp.value, sizeof(global_data.perf_result_filename) - 1);
		}

		if (strcmp(tmp.key, "[multi_process]") == 0) {
			global_data.multi_process = atoi(tmp.value);
			process_data.process_num = VENDOR_AI_MAX_PROCESS;
			/* note: inheritance issue, child process is the same as parent process */
		}

		if (strcmp(tmp.key, "[multi_process_loop_count]") == 0) {
			global_data.multi_process_loop_count = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[multi_process_signal]") == 0) {
			global_data.multi_process_signal = atoi(tmp.value);
		}
		if (strcmp(tmp.key, "[ctrl_c_reset]") == 0) {
            global_data.ctrl_c_reset = atoi(tmp.value);
        }
	}

	#if 0
	printf("first_loop_count=%lu\r\n", global_data.first_loop_count);
	printf("sequential=%lu\r\n", global_data.sequential);
	printf("sequential_loop_count=%lu\r\n", global_data.sequential_loop_count);
	printf("timeout=%lu\r\n", global_data.timeout);
	for (i = 0; i < global_data.first_loop_count; i++) {
		printf("first data[%lu] = %s\r\n", i, global_data.first_loop_data[i].first_loop_path);
	}
	#endif

	fclose(fp_tmp);
	return 1;
}

static INT32 prepare_share_model_config(void)
{
	int i;

	//share model must work in parallel threads
	if(global_data.sequential){
		printf("share model can't work in sequential config\n");
		return -1;
	}

	//first_loop#0 is the master, it controls model memory
	//other first_loops set their share_model_master to first_loop#0
	for (i = 1; i < global_data.first_loop_count; i++) {
		global_data.first_loop_data[i].share_model_master = &(global_data.first_loop_data[0]);
	}

	return 0;
}

static int read_model_list(FIRST_LOOP_DATA *first_loop_data, char *model_list_file)
{
	FILE *fp = NULL;
	CHAR line[512];
	CONFIG_OPT tmp;
	int model_num = 0, i = 0;

	if(!first_loop_data || !model_list_file){
		printf("first_loop_data(%p) or model_list_file(%p) is null\n", first_loop_data, model_list_file);
		return -1;
	}

	//count model number
	if ((fp = fopen(model_list_file, "r")) == NULL)
	{
		printf("%s is not exist!\n", model_list_file);
		return -1;
	}
	while (1) {
		if (feof(fp))
			break;
		if (fgets(line, 512 - 1, fp) == NULL)
		{
			if (feof(fp))
				break;
			printf("fgets line: %s error!\n", line);
			continue;
		}

		remove_blank(line);

		if (line[0] == '#' || line[0] == '\0')
			continue;
		if (split_kvalue(line, &tmp, '=') != 1)
		{
			printf("pauser line : %s error!\r\n", line);
			continue;
		}

		if (strcmp(tmp.key, "[model_cfg_path]") == 0) {
			model_num++;
		}
	}
	fclose(fp);

	if(model_num == 0){
		printf("invalid model list file, no [model_cfg_path] entry\n");
		return -1;
	}

	//allocate model cfg array
	first_loop_data->model_cfg = malloc(sizeof(NET_MODEL_CONFIG)*model_num);
	if(first_loop_data->model_cfg == NULL){
		printf("fail to allocate %d NET_MODEL_CONFIG\n", model_num);
		return -1;
	}
	memset(first_loop_data->model_cfg, 0, sizeof(NET_MODEL_CONFIG)*model_num);
	first_loop_data->second_cfg_count = model_num;

	//read model config
	if ((fp = fopen(model_list_file, "r")) == NULL)
	{
		printf("%s is not exist!\n", model_list_file);
		return -1;
	}
	while (1) {
		if (feof(fp))
			break;
		if (fgets(line, 512 - 1, fp) == NULL)
		{
			if (feof(fp))
				break;
			printf("fgets line: %s error!\n", line);
			continue;
		}

		remove_blank(line);

		if (line[0] == '#' || line[0] == '\0')
			continue;
		if (split_kvalue(line, &tmp, '=') != 1)
		{
			printf("pauser line : %s error!\r\n", line);
			continue;
		}

		if (strcmp(tmp.key, "[model_cfg_path]") == 0) {
			if (i < first_loop_data->second_cfg_count) {
				strncpy(first_loop_data->model_cfg[i].model_cfg_filename, tmp.value, 256-1);
				first_loop_data->model_cfg[i].model_cfg_filename[256-1] = '\0';
				i++;
			} else {
				printf("model_count is larger than %d\r\n", first_loop_data->second_cfg_count);
			}
		}
	}
	fclose(fp);

	return 0;
}

static int skip_model_list(FIRST_LOOP_DATA *first_loop_data, char *skip_list_file)
{
	FILE *fp = NULL;
	CHAR line[512];
	CONFIG_OPT tmp;
	int model_num = 0, i = 0, j = 0;
	NET_MODEL_CONFIG *old_model_cfg = NULL;

	if(!first_loop_data || !skip_list_file){
		printf("first_loop_data(%p) or skip_list_file(%p) is null\n", first_loop_data, skip_list_file);
		return -1;
	}

	if(first_loop_data->second_cfg_count == 0 || first_loop_data->model_cfg == NULL){
		printf("second_cfg_count(%d) or model_cfg(%p) is null. nothing to skip\n",
					first_loop_data->second_cfg_count, first_loop_data->model_cfg);
		return 0;
	}

	//parse skip list file and remove models from first_loop_data
	if ((fp = fopen(skip_list_file, "r")) == NULL)
	{
		printf("%s is not exist!\n", skip_list_file);
		return -1;
	}
	while (1) {
		if (feof(fp))
			break;
		if (fgets(line, 512 - 1, fp) == NULL)
		{
			if (feof(fp))
				break;
			printf("fgets line: %s error!\n", line);
			continue;
		}

		remove_blank(line);

		if (line[0] == '#' || line[0] == '\0')
			continue;
		if (split_kvalue(line, &tmp, '=') != 1)
		{
			printf("pauser line : %s error!\r\n", line);
			continue;
		}

		if (strcmp(tmp.key, "[model_cfg_path]") == 0) {
			//remove models from first_loop_data
			for(i = 0 ; i < first_loop_data->second_cfg_count ; ++i){
				if(strcmp(first_loop_data->model_cfg[i].model_cfg_filename, tmp.value) == 0){
					memset(&(first_loop_data->model_cfg[i]), 0, sizeof(NET_MODEL_CONFIG));
				}
			}
		}
	}
	fclose(fp);

	//count remaining model number
	model_num = 0;
	for(i = 0 ; i < first_loop_data->second_cfg_count ; ++i){
		if(strlen(first_loop_data->model_cfg[i].model_cfg_filename)){
			model_num++;
		}
	}

	//re-allocate model cfg array
	old_model_cfg = first_loop_data->model_cfg;
	first_loop_data->model_cfg = malloc(sizeof(NET_MODEL_CONFIG)*model_num);
	if(first_loop_data->model_cfg == NULL){
		printf("fail to allocate %d NET_MODEL_CONFIG\n", model_num);
		free(old_model_cfg);
		return -1;
	}
	j = 0;
	for(i = 0 ; i < first_loop_data->second_cfg_count ; ++i){
		if(strlen(old_model_cfg[i].model_cfg_filename) && j < model_num){
			memcpy(&(first_loop_data->model_cfg[j]), &(old_model_cfg[i]), sizeof(NET_MODEL_CONFIG));
			j++;
		}
	}
	first_loop_data->second_cfg_count = model_num;
	free(old_model_cfg);

	return 0;
}

static INT32 ai_test_first_config_parser(FIRST_LOOP_DATA *first_loop_data)
{

	FILE *fp_tmp = NULL;
	int model_count = 0, op_count = 0;


	if ((fp_tmp = fopen(first_loop_data->first_loop_path, "r")) == NULL)
	{
		printf("%s is not exist!\n", first_loop_data->first_loop_path);
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

		if (strcmp(tmp.key, "[loop_count]") == 0) {
			first_loop_data->loop_count = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[net_path]") == 0) {
			first_loop_data->net_path = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[invalid_net]") == 0) {
			first_loop_data->invalid_net_path = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[model_cfg_count]") == 0) {
			first_loop_data->second_cfg_count = atoi(tmp.value);
			first_loop_data->model_cfg = malloc(sizeof(NET_MODEL_CONFIG)*first_loop_data->second_cfg_count);
			memset(first_loop_data->model_cfg, 0, sizeof(NET_MODEL_CONFIG)*first_loop_data->second_cfg_count);
		}

		if (strcmp(tmp.key, "[operator_cfg_count]") == 0) {
			first_loop_data->second_cfg_count = atoi(tmp.value);
			first_loop_data->op_cfg = malloc(sizeof(OPERATOR_CONFIG)*first_loop_data->second_cfg_count);
			memset(first_loop_data->op_cfg, 0, sizeof(OPERATOR_CONFIG)*first_loop_data->second_cfg_count);
		}

		if (strcmp(tmp.key, "[model_cfg_path]") == 0) {
			if (first_loop_data->model_cfg) {
				if (model_count < first_loop_data->second_cfg_count) {
					strncpy(first_loop_data->model_cfg[model_count].model_cfg_filename, tmp.value, 256-1);
					first_loop_data->model_cfg[model_count].model_cfg_filename[256-1] = '\0';
					model_count++;
				} else {
					printf("model_count is larger than %d\r\n", first_loop_data->second_cfg_count);
				}
			}
		}

		if (strcmp(tmp.key, "[operator_cfg_path]") == 0) {
			if (first_loop_data->op_cfg) {
				if (op_count < first_loop_data->second_cfg_count) {
					strncpy(first_loop_data->op_cfg[op_count].op_cfg_filename, tmp.value, 256-1);
					first_loop_data->op_cfg[op_count].op_cfg_filename[256-1] = '\0';
					op_count++;
				} else {
					printf("op_count is larger than %d\r\n", first_loop_data->second_cfg_count);
				}
			}
		}

		if (strcmp(tmp.key, "[net_buf_opt_method]") == 0) {
			first_loop_data->net_buf_opt.method = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[net_buf_opt_ddr_id]") == 0) {
			first_loop_data->net_buf_opt.ddr_id = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[net_buf_opt_invalid]") == 0) {
			first_loop_data->invalid_buf_opt = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[net_job_opt_invalid]") == 0) {
			first_loop_data->invalid_job_opt = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[null_work_buf]") == 0) {
			first_loop_data->null_work_buf = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[non_align_work_buf]") == 0) {
			first_loop_data->non_align_work_buf = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[net_job_opt_method]") == 0) {
			first_loop_data->net_job_opt.method = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[net_buf_opt_ddr_id]") == 0) {
			first_loop_data->net_buf_opt.ddr_id = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[net_job_opt_wait_ms]") == 0) {
			first_loop_data->net_job_opt.wait_ms = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[net_job_opt_schd_parm]") == 0) {
			first_loop_data->net_job_opt.schd_parm = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[second_loop_count]") == 0) {
			first_loop_data->second_loop_data.loop_count = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[third_loop_count]") == 0) {
			first_loop_data->second_loop_data.third_loop_data.loop_count = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[fourth_loop_count]") == 0) {
			first_loop_data->second_loop_data.third_loop_data.fourth_loop_data.loop_count = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[schd_parm_inc]") == 0) {
			first_loop_data->schd_parm_inc = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[perf_analyze]") == 0) {
			first_loop_data->perf_analyze = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[ignore_set_input_fail]") == 0) {
			first_loop_data->ignore_set_input_fail = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[net_path_max]") == 0) {
			first_loop_data->net_path_max = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[net_path_loop]") == 0) {
			first_loop_data->net_path_loop = (BOOL)atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[ignore_set_res_dim_fail]") == 0) {
			first_loop_data->ignore_set_res_dim_fail = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[cpu_perf_analyze]") == 0) {
			first_loop_data->cpu_perf_analyze = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[perf_analyze_report]") == 0) {
			first_loop_data->perf_analyze_report = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[model_list]") == 0) {
			if(read_model_list(first_loop_data, tmp.value)){
				printf("fail to parse model list file : %s\n", tmp.value);
				return 0;
			}
		}

		if (strcmp(tmp.key, "[skip_model_list]") == 0) {
			if(skip_model_list(first_loop_data, tmp.value)){
				printf("fail to parse skip list file : %s\n", tmp.value);
				return 0;
			}
		}

		if (strcmp(tmp.key, "[proc_id_test]") == 0) {
			first_loop_data->proc_id_test = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[multi_scale_set_imm]") == 0) {
			first_loop_data->multi_scale_set_imm = atoi(tmp.value);
		}
	}

	#if 0
	printf("loop_count=%lu\r\n", first_loop_data->loop_count);
	printf("net_path=%lu\r\n", first_loop_data->net_path);
	printf("invalid_net_path=%lu\r\n", first_loop_data->invalid_net_path);
	printf("second_cfg_count=%lu\r\n", first_loop_data->second_cfg_count);
	printf("net_buf_opt_method=%lu\r\n", first_loop_data->net_buf_opt.method);
	printf("net_buf_opt_ddr_id=%lu\r\n", first_loop_data->net_buf_opt.ddr_id);
	printf("invalid_buf_opt=%lu\r\n", first_loop_data->invalid_buf_opt);
	printf("net_job_opt.method=%lu\r\n", first_loop_data->net_job_opt.method);
	printf("net_buf_opt.ddr_id=%lu\r\n", first_loop_data->net_buf_opt.ddr_id);
	printf("net_job_opt.wait_ms=%lu\r\n", first_loop_data->net_job_opt.wait_ms);
	printf("net_job_opt.schd_parm=%lu\r\n", first_loop_data->net_job_opt.schd_parm);
	printf("second_loop_count=%lu\r\n", first_loop_data->second_loop_data.loop_count);
	printf("third_loop_count=%lu\r\n", first_loop_data->second_loop_data.third_loop_data.loop_count);
	printf("fourth_loop_count=%lu\r\n", first_loop_data->second_loop_data.third_loop_data.fourth_loop_data.loop_count);
	for (i = 0; i < first_loop_data->second_cfg_count; i++) {
		printf("model_cfg[%lu] = %s\r\n", i, first_loop_data->model_cfg[i].model_cfg_filename);
	}
	#endif

	fclose(fp_tmp);

	if(first_loop_data->model_cfg && first_loop_data->op_cfg){
		printf("model config and operator config can't be enabled in a single first loop\n");
		return 0;
	}else if(first_loop_data->model_cfg == NULL && first_loop_data->op_cfg == NULL){
		printf("model config and operator are empty\n");
		return 0;
	}

	return 1;
}

static INT32 ai_test_model_config_parser(NET_MODEL_CONFIG *model_cfg)
{

	FILE *fp_tmp = NULL;
	//int in_count = 0, out_count = 0, i;

	if ((fp_tmp = fopen(model_cfg->model_cfg_filename, "r")) == NULL)
	{
		printf("%s is not exist!\r\n", model_cfg->model_cfg_filename);
		return 0;
	}
	CHAR line[512] = {0};
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
		CONFIG_OPT tmp = {0};
		if (split_kvalue(line, &tmp, '=') != 1)
		{
			printf("pauser line : %s error!\r\n", line);
			continue;
		}
		//printf("k=%s, v=%s\r\n", tmp.key, tmp.value);

		if (strcmp(tmp.key, "[model_filename]") == 0) {
			strncpy(model_cfg->model_filename, tmp.value, 256-1);
			model_cfg->model_filename[256-1] = '\0';
		}

		if (strcmp(tmp.key, "[label_filename]") == 0) {
			strncpy(model_cfg->label_filename, tmp.value, 256-1);
			model_cfg->label_filename[256-1] = '\0';
		}

		if (strcmp(tmp.key, "[diff_filename]") == 0) {
			strncpy(model_cfg->diff_filename, tmp.value, 256-1);
			model_cfg->diff_filename[256-1] = '\0';
		}



		if (strcmp(tmp.key, "[invalid]") == 0) {
			model_cfg->invalid = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[in_file_path]") == 0) {
			printf("in file path = %s\r\n", tmp.value);
			tmp.value[256-1] = '\0';
			if(ai_test_model_image_list_parser(&(model_cfg->in_file_cfg_count), &(model_cfg->in_file_cfg), tmp.value) == 0){
				printf("fail to parse model image list\n");
				return 0;
			}
		}

		if (strcmp(tmp.key, "[model_unset]") == 0) {
			model_cfg->model_unset = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[model_zero_size]") == 0) {
			model_cfg->model_zero_size = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[model_null]") == 0) {
			model_cfg->model_null = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[model_zero_addr]") == 0) {
			model_cfg->model_zero_addr = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[no_set_model_between_close_open]") == 0) {
			model_cfg->no_set_model_between_close_open = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[diff_res_num]") == 0) {
			model_cfg->diff_res_num = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[diff_width]") == 0) {
			if (model_cfg->diff_res_num != 0) {
				INT32 i;
				CONFIG_OPT in_opt = {0};
				CHAR test_line[256] = {0};

				if (model_cfg->diff_width == 0) {
					model_cfg->diff_width = malloc(sizeof(INT32)*model_cfg->diff_res_num);
				}

				for (i = 0; i < model_cfg->diff_res_num; i++) {
					model_cfg->diff_width[i] = 0;
				}

				strcpy(test_line, tmp.value);

				for (i = 0; i < model_cfg->diff_res_num; i++) {
					if (split_kvalue2(test_line, &in_opt, ',') != 1) {
						break;
					}
					sscanf(in_opt.key, "%ld", &model_cfg->diff_width[i]);
					strcpy(test_line, in_opt.value);
				}

				#if 0
				printf("width: ");
				for (i = 0; i < model_cfg->diff_res_num; i++) {
					printf("%d ", model_cfg->diff_width[i]);
				}
				printf("\r\n");
				#endif
			} else {
				printf("diff_res_num is 0\r\n");
			}
		}

		if (strcmp(tmp.key, "[diff_height]") == 0) {
			if (model_cfg->diff_res_num != 0) {
				INT32 i;
				CONFIG_OPT in_opt = {0};
				CHAR test_line[256] = {0};

				if (model_cfg->diff_height == 0) {
					model_cfg->diff_height = malloc(sizeof(INT32)*model_cfg->diff_res_num);
				}

				for (i = 0; i < model_cfg->diff_res_num; i++) {
					model_cfg->diff_height[i] = 0;
				}

				strcpy(test_line, tmp.value);

				for (i = 0; i < model_cfg->diff_res_num; i++) {

					if (split_kvalue2(test_line, &in_opt, ',') != 1) {
						break;
					}

					sscanf(in_opt.key, "%ld", &model_cfg->diff_height[i]);
					strcpy(test_line, in_opt.value);
				}

				#if 0
				printf("height: ");
				for (i = 0; i < model_cfg->diff_res_num; i++) {
					printf("%d ", model_cfg->diff_height[i]);
				}
				printf("\r\n");
				#endif
			} else {
				printf("diff_res_num is 0\r\n");
			}
		}

		
		if (strcmp(tmp.key, "[dyB_num]") == 0) {
			model_cfg->dyB_res_num = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[dyB_size]") == 0) {
			if (model_cfg->dyB_res_num != 0) {
				INT32 i;
				CONFIG_OPT in_opt = {0};
				CHAR test_line[256] = {0};

				if (model_cfg->dyB_batch == 0) {
					model_cfg->dyB_batch = malloc(sizeof(INT32)*model_cfg->dyB_res_num);
				}

				for (i = 0; i < model_cfg->dyB_res_num; i++) {
					model_cfg->dyB_batch[i] = 0;
				}

				strcpy(test_line, tmp.value);

				for (i = 0; i < model_cfg->dyB_res_num; i++) {
					if (split_kvalue2(test_line, &in_opt, ',') != 1) {
						break;
					}
					sscanf(in_opt.key, "%ld", &model_cfg->dyB_batch[i]);
					strcpy(test_line, in_opt.value);
				}

		#if 1
				printf("Batch: ");
				for (i = 0; i < model_cfg->dyB_res_num; i++) {
					printf("%d ", model_cfg->dyB_batch[i]);
				}
				printf("\r\n");
		#endif
			} else {
				printf("dyB_res_num is 0\r\n");
			}
		}

		if (strcmp(tmp.key, "[dynamic_batch_random_num]") == 0) {
			model_cfg->dynamic_batch_random_num= atoi(tmp.value);
		}
				
		if (strcmp(tmp.key, "[in_path_num]") == 0) {
			model_cfg->in_path_num= atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[diff_res_loop]") == 0) {
			model_cfg->diff_res_loop = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[diff_res_random]") == 0) {
			model_cfg->diff_res_random = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[diff_res_random_times]") == 0) {
			model_cfg->diff_res_random_times = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[diff_max_w]") == 0) {
			model_cfg->diff_max_w = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[diff_max_h]") == 0) {
			model_cfg->diff_max_h = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[diff_res_padding]") == 0) {
			model_cfg->diff_res_padding = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[diff_res_num]") == 0) {
			model_cfg->diff_res_num = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[diff_res_padding_num]") == 0) {
			model_cfg->diff_res_padding_num= atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[multi_scale_res_num]") == 0) {
			model_cfg->multi_scale_res_num = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[multi_scale_random_num]") == 0) {
			model_cfg->multi_scale_random_num = atoi(tmp.value);
		}

		if (strcmp(tmp.key, "[multi_scale_valid_out]") == 0) {
			if (model_cfg->multi_scale_res_num != 0) {
				UINT32 i;
				CONFIG_OPT in_opt = {0};
				CHAR test_line[256] = {0};

				if (model_cfg->multi_scale_valid_out == 0) {
					model_cfg->multi_scale_valid_out = malloc(sizeof(MULTI_SCALE_VALID_OUT)*model_cfg->multi_scale_res_num);
				}

				memset((void*) model_cfg->multi_scale_valid_out, 0, sizeof(MULTI_SCALE_VALID_OUT)*model_cfg->multi_scale_res_num);

				strcpy(test_line, tmp.value);

				for (i = 0; i < model_cfg->multi_scale_res_num; i++) {

					//parse width
					if (split_kvalue2(test_line, &in_opt, ',') != 1) {
						break;
					}
					sscanf(in_opt.key, "%ld", &model_cfg->multi_scale_valid_out[i].w);
					strcpy(test_line, in_opt.value);

					//parse height
					if (split_kvalue2(test_line, &in_opt, ',') != 1) {
						break;
					}
					sscanf(in_opt.key, "%ld", &model_cfg->multi_scale_valid_out[i].h);
					strcpy(test_line, in_opt.value);

					//parse valid output size
					if (split_kvalue2(test_line, &in_opt, ',') != 1) {
						break;
					}
					sscanf(in_opt.key, "%ld", &model_cfg->multi_scale_valid_out[i].valid_out_size);
					strcpy(test_line, in_opt.value);
				}

				#if 0
				printf("valid_out_size: \r\n");
				for (i = 0; i < model_cfg->multi_scale_res_num; i++) {
					printf("w=%ld, h=%ld, valid size=0x%X\r\n", model_cfg->multi_scale_valid_out[i].w, model_cfg->multi_scale_valid_out[i].h, model_cfg->multi_scale_valid_out[i].valid_out_size);
				}
				printf("\r\n");
				#endif
			} else {
				printf("diff_res_num is 0\r\n");
			}
		}
	}

	#if 0
	printf("model_filename=%s\r\n", model_cfg->model_filename);
	printf("label_filename=%s\r\n", model_cfg->label_filename);
	printf("diff_filename=%s\r\n", model_cfg->diff_filename);
	printf("model_cfg_filename=%s\r\n", model_cfg->model_cfg_filename);
	#endif

	fclose(fp_tmp);
	return 1;
}

static INT32 ai_test_op_config_parser(OPERATOR_CONFIG *op_cfg)
{

	FILE *fp_tmp = NULL;
	//int in_count = 0, out_count = 0, i;

	if ((fp_tmp = fopen(op_cfg->op_cfg_filename, "r")) == NULL)
	{
		printf("%s is not exist!\r\n", op_cfg->op_cfg_filename);
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

		if (strcmp(tmp.key, "[in_file_path]") == 0) {
			printf("in file path = %s\r\n", tmp.value);
			tmp.value[256-1] = '\0';
			if(ai_test_op_image_list_parser(&(op_cfg->in_file_cfg_count), &(op_cfg->in_file_cfg), tmp.value) == 0){
				printf("fail to parse operator image list\n");
				return 0;
			}
		}

		if (strcmp(tmp.key, "[op_opt]") == 0) {
			op_cfg->op_opt = atoi(tmp.value);
		}

	}

	fclose(fp_tmp);
	return 1;
}

static int ai_test_cmd_parse_user_option(int argc, char **argv)
{
	CHAR config_path[128] = {0};
	int i, j;

	memset(&global_data, 0, sizeof(global_data));
	memset(&process_data, 0, sizeof(process_data));

	sprintf(config_path,"%s","/mnt/sd/para/global/global.txt");
    if (!(ai_test_global_config_parser(config_path)))
    {
        printf("global parser failed!!!\n");
        return -1;
    } else {

		if (global_data.perf_compare == 0) {
			for (i = 0; i < global_data.first_loop_count; i++) {
			    if (!(ai_test_first_config_parser(&(global_data.first_loop_data[i]))))
			    {
			        printf("first config parser failed!!!\n");
			        return -1;
			    } else {

					for (j = 0; j < global_data.first_loop_data[i].second_cfg_count; j++) {
						if(global_data.first_loop_data[i].model_cfg){
							if (!(ai_test_model_config_parser(&(global_data.first_loop_data[i].model_cfg[j])))) {
								printf("model config parser failed!!!\n");
								return -1;
							}

							if(global_data.first_loop_data[i].loop_count == 0){
								global_data.first_loop_data[i].loop_count = global_data.first_loop_data[i].second_cfg_count;
							}
						}else{
							if (!(ai_test_op_config_parser(&(global_data.first_loop_data[i].op_cfg[j])))) {
								printf("operator config parser failed!!!\n");
								return -1;
							}
						}
					}

					if (global_data.share_model && prepare_share_model_config()){
						printf("fail to prepare share model config!!!\n");
						return -1;
					}
			    }
			}
		}
    }

	return 0;
}


static int create_cpu_stress_thread(CPU_STRESS_DATA *cpu_stress)
{
	if(!cpu_stress){
		printf("create_cpu_stress_thread(): arg is null\n");
		return -1;
	}

	if(cpu_stress[0].stress_level){
		if(start_cpu_stress_thread(&(cpu_stress[0]))){
			printf("fail to create cpu stress1\n");
			return -1;
		}
	}

	if(cpu_stress[1].stress_level){
		if(start_cpu_stress_thread(&(cpu_stress[1]))){
			printf("fail to create cpu stress2\n");
			return -1;
		}
	}

	return 0;
}

static int destroy_cpu_stress_thread(CPU_STRESS_DATA *cpu_stress)
{
	if(!cpu_stress){
		printf("destroy_cpu_stress_thread(): arg is null\n");
		return -1;
	}

	if(cpu_stress[0].thread_id)
		stop_cpu_stress_thread(&(cpu_stress[0]));

	if(cpu_stress[1].thread_id)
		stop_cpu_stress_thread(&(cpu_stress[1]));

	return 0;
}

static int create_dma_stress_thread(DMA_STRESS_DATA *dma_stress)
{
	if(!dma_stress){
		printf("create_dma_stress_thread(): arg is null\n");
		return -1;
	}

	if(dma_stress[0].stress_level){
		if(start_dma_stress_thread(&(dma_stress[0]))){
			printf("fail to create cpu stress1\n");
			return -1;
		}
	}

	if(dma_stress[1].stress_level){
		if(start_dma_stress_thread(&(dma_stress[1]))){
			printf("fail to create dma_stress2\n");
			if(dma_stress[0].thread_id)
				stop_dma_stress_thread(&(dma_stress[0]));
			return -1;
		}
	}

	return 0;
}

static int destroy_dma_stress_thread(DMA_STRESS_DATA *dma_stress)
{
	if(!dma_stress){
		printf("destroy_dma_stress_thread(): arg is null\n");
		return -1;
	}

	if(dma_stress[0].thread_id)
		stop_dma_stress_thread(&(dma_stress[0]));

	if(dma_stress[1].thread_id)
		stop_dma_stress_thread(&(dma_stress[1]));

	return 0;
}

static int are_all_first_loop_done(void)
{
	int i;

	ai_test_lock_and_sync();

	for(i = 0 ; i < global_data.first_loop_count ; ++i){
		if(global_data.first_loop_data[i].test_success == 0)
			break;
	}

	ai_test_unlock();

	if(i == global_data.first_loop_count || global_data.first_loop_count == 0)
		return 1;
	else
		return 0;
}

static int wait_all_done_or_timeout(void)
{
	int done = 0;
	struct timeval tstart, tend;
	UINT32 time_arr;
	GLOBAL_DATA* p_global_data = ai_test_get_global_data();

	gettimeofday(&tstart, NULL);

	while(done == 0){
		done = are_all_first_loop_done();
		if(done == 1)
			break;

		gettimeofday(&tend, NULL);
		time_arr = (tend.tv_sec - tstart.tv_sec);
		if(global_data.timeout && time_arr > global_data.timeout){
			printf("timeout. %d seconds passed. ai network is stall\n", (int)time_arr);
			return -1;
		}

		sleep(1);
	};

	gettimeofday(&tend, NULL);
	time_arr = (tend.tv_sec - tstart.tv_sec);
	if (p_global_data->multi_process) {
		printf("pid %d: finish in %d seconds\n", getpid(), (int)time_arr);
	} else {
		printf("all tasks finish in %d seconds\n", (int)time_arr);
	}

	return 0;
}

static int setup_test_data(int test)
{
	char cmd[512];
	int  ret;

	ret = system("rm -rf /mnt/sd/para");
	if(ret){
		printf("fail to remove old directory /mnt/sd/para\n");
		return ret;
	}

	sprintf(cmd, "cp -r /mnt/sd/ai2_test_item/%d/para /mnt/sd/", test);
	ret = system(cmd);
	if(ret)
		printf("command fail : %s\n", cmd);

	return ret;
}

GLOBAL_DATA* ai_test_get_global_data(void)
{
	return &global_data;
}

PROCESS_DATA* ai_test_get_process_data(void)
{
	return &process_data;
}

UINT32 count_line(CHAR *txt_path)
{
	FILE *fp;
	int flag = 0, count = 0;
	if ((fp = fopen(txt_path, "r")) == NULL)
		return -1;
	while (!feof(fp))
	{
		flag = fgetc(fp);
		if (flag == '\n')
			count++;
	}
	//printf("row = %d\n", count);
	fclose(fp);
	return count;
}

static int compare_last_result(CHAR* result_path, CHAR* last_result_path, CHAR* diff_path, GLOBAL_DATA* global_data)
{
	CHAR mname_new[256] = { 0 };
	CHAR mname_old[256] = { 0 };
	CHAR img_name_new[256] = { 0 };
	CHAR img_name_old[256] = { 0 };
	CHAR msg[512];

	FLOAT time_new, time_old;
	FLOAT bw_new, bw_old, size_new, size_old, memory_new, memory_old, loading_new, loading_old;

	FLOAT diff_time = 0;
	FLOAT diff_bw = 0;
	FLOAT diff_size = 0;
	FLOAT diff_memory = 0;
	FLOAT diff_loading = 0;

	FLOAT time_upper = 0;

	UINT32 idx = 0;
	BOOL flag = TRUE;

	UINT32 new_lines = 0;
	UINT32 old_lines = 0;
	FILE *fp_new = NULL, *fp_old = NULL;

	int ret = 0;

	new_lines = count_line(result_path);
	old_lines = count_line(last_result_path);

	fp_new = fopen(result_path, "r");
	fp_old = fopen(last_result_path, "r");

	for (idx = 0; idx < new_lines; idx++) {
		fscanf(fp_new, "%s %s %f %f %f %f %f", mname_new, img_name_new, &time_new, &bw_new, &size_new, &memory_new, &loading_new);
		flag = TRUE;
		for (UINT32 j = 0; j < old_lines; j++) {
			fscanf(fp_old, "%s %s %f %f %f %f %f", mname_old, img_name_old, &time_old, &bw_old, &size_old,&memory_old, &loading_old);
			if (strcmp(mname_new, mname_old) == 0 && strcmp(img_name_new, img_name_old) == 0){
				flag = FALSE;
				diff_time = time_new - time_old;
				diff_bw = bw_new - bw_old;
				diff_size = size_new - size_old;
				diff_memory = memory_new - memory_old;
				diff_loading = loading_new -loading_old;

				if (time_new <= 5000 && global_data->time_threshold_upper_short != 0) {
					time_upper = global_data->time_threshold_upper_short;
				} else {
					time_upper = global_data->time_threshold_upper;
				}
				if ((diff_time / time_old) < global_data->time_threshold_lower || (diff_time / time_old) > time_upper ||
					time_old == 0 || time_new == 0){
                    sprintf(msg, "echo -n \"%s %s %f %f %f %f %f\n\" >> %s",
					mname_new,
					img_name_new,
					diff_time / time_old,
					diff_bw / bw_old,
					diff_size / size_old,
					diff_memory / memory_old,
					diff_loading / loading_old,
					diff_path);
				    system(msg);
					ret = -1;

					printf("time compare\r\n");
					printf("%s %s\r\n", mname_old, img_name_old);
					printf("new time=%f, bw=%f, size=%f, mem=%f, loading=%f\r\n", time_new, bw_new, size_new, memory_new, loading_new);
					printf("old time=%f, bw=%f, size=%f, mem=%f, loading=%f\r\n", time_old, bw_old, size_old, memory_old, loading_old);

					break;
				}
				if ((diff_bw / bw_old) < global_data->bw_threshold_lower || (diff_bw / bw_old) > global_data->bw_threshold_upper ||
					((bw_old == 0 || bw_new == 0) && !(bw_old == 0 && bw_new == 0))){
					sprintf(msg, "echo -n \"%s %s %f %f %f %f %f\n\" >> %s",
					mname_new,
					img_name_new,
					diff_time / time_old,
					diff_bw / bw_old,
					diff_size / size_old,
					diff_memory / memory_old,
					diff_loading / loading_old,
					diff_path);
				    system(msg);
					ret = -1;

					printf("bandwidth compare\r\n");
					printf("%s %s\r\n", mname_old, img_name_old);
					printf("new time=%f, bw=%f, size=%f, mem=%f, loading=%f\r\n", time_new, bw_new, size_new, memory_new, loading_new);
					printf("old time=%f, bw=%f, size=%f, mem=%f, loading=%f\r\n", time_old, bw_old, size_old, memory_old, loading_old);

					break;
				}
				if ((diff_size / size_old) < global_data->size_threshold_lower || (diff_size / size_old) > global_data->size_threshold_upper ||
					size_old == 0 || size_new == 0){
                    sprintf(msg, "echo -n \"%s %s %f %f %f %f %f\n\" >> %s",
					mname_new,
					img_name_new,
					diff_time / time_old,
					diff_bw / bw_old,
					diff_size / size_old,
					diff_memory / memory_old,
					diff_loading / loading_old,
					diff_path);
				    system(msg);
					ret = -1;

					printf("size compare\r\n");
					printf("%s %s\r\n", mname_old, img_name_old);
					printf("new time=%f, bw=%f, size=%f, mem=%f, loading=%f\r\n", time_new, bw_new, size_new, memory_new, loading_new);
					printf("old time=%f, bw=%f, size=%f, mem=%f, loading=%f\r\n", time_old, bw_old, size_old, memory_old, loading_old);

					break;
				}

				if ((diff_memory / memory_old) < global_data->memory_threshold_lower || (diff_memory / memory_old) > global_data->memory_threshold_upper ||
					memory_old == 0 || memory_new == 0){
                    sprintf(msg, "echo -n \"%s %s %f %f %f %f %f\n\" >> %s",
					mname_new,
					img_name_new,
					diff_time / time_old,
					diff_bw / bw_old,
					diff_size / size_old,
					diff_memory / memory_old,
					diff_loading / loading_old,
					diff_path);
				    system(msg);
					ret = -1;

					printf("memory compare\r\n");
					printf("%s %s\r\n", mname_old, img_name_old);
					printf("new time=%f, bw=%f, size=%f, mem=%f, loading=%f\r\n", time_new, bw_new, size_new, memory_new, loading_new);
					printf("old time=%f, bw=%f, size=%f, mem=%f, loading=%f\r\n", time_old, bw_old, size_old, memory_old, loading_old);

					break;
				}

				#if 0 //CPU loading
				if ((diff_loading/ loading_old) < global_data->loading_threshold_lower || (diff_loading / loading_old) > global_data->loading_threshold_upper){
                    sprintf(msg, "echo -n \"%s %s %f %f %f %f %f\n\" >> %s",
					mname_new,
					img_name_new,
					diff_time / time_old,
					diff_bw / bw_old,
					diff_size / size_old,
					diff_memory / memory_old,
					diff_loading / loading_old,
					diff_path);
				    system(msg);
					ret = -1;
					break;
				}
				#endif

				break;
			}
		}
		fseek(fp_old, 0, SEEK_SET);
		if (flag) {
			//flag =TRUE;
			sprintf(msg, "echo -n \"%s %s not exist \n\" >> %s",
				mname_new,
				img_name_new,
				diff_path);
			system(msg);
		}
	}

	//fclose(fp_old);
	if(fp_old){
		fclose(fp_old);
		fp_old = NULL ;
	}
		//fclose(fp_new);
	if(fp_new){
		fclose(fp_new);
		fp_new = NULL ;
	}

	return ret;
}


static void compare_stress_result(CHAR* result_path, CHAR* stress_result_path, CHAR* diff_path)
{
	CHAR mname_new[256] = { 0 };
	CHAR mname_old[256] = { 0 };
	CHAR img_name_new[256] = { 0 };
	CHAR img_name_old[256] = { 0 };
	FLOAT time_new, time_old;
	FLOAT bw_new, bw_old, size_new, size_old, memory_new, memory_old, loading_new, loading_old;
	CHAR msg[512];

	FLOAT diff_time = 0;
	UINT32 idx = 0;

	UINT32 non_stress_lines = 0;
	UINT32 stress_lines = 0;
	FILE *fp_none = NULL, *fp_stress = NULL;

	non_stress_lines = count_line(result_path);
	stress_lines = count_line(stress_result_path);


	fp_none = fopen(result_path, "r");
	fp_stress = fopen(stress_result_path, "r");

	for (idx = 0; idx < stress_lines; idx++) {
		fscanf(fp_stress, "%s %s %f %f %f %f %f", mname_new, img_name_new, &time_new, &bw_new, &size_new, &memory_new, &loading_new);
		//flag = TRUE;
		for (UINT32 j = 0; j < non_stress_lines; j++)
		{
			fscanf(fp_none, "%s %s %f %f %f %f %f", mname_old, img_name_old, &time_old, &bw_old, &size_old,&memory_old, &loading_old);

			if (strcmp(mname_new, mname_old) == 0 && strcmp(img_name_new, img_name_old) == 0)
			{
				//flag = FALSE;
				diff_time = time_new - time_old;
				//diff_bw = bw_new - bw_old;

				//error_flag = 1;
				sprintf(msg, "echo -n \"%s %s %f \n\" >> %s",
				mname_new,
				img_name_new,
				diff_time / time_old,
				diff_path);
				system(msg);
				break;
			}

		}
		fseek(fp_none, 0, SEEK_SET);
	}

	if(fp_none){
		fclose(fp_none);
		fp_none = NULL ;
	}

	if(fp_stress){
		fclose(fp_stress);
		fp_stress = NULL ;
	}
}

int setup_dumping_golden_output(void){

	char cmd[256];

	sprintf(cmd, "mkdir -p %s", global_data.generate_golden_output);
	system(cmd);

	return 0;
}

static int ai_test_check_net_supported_num(GLOBAL_DATA* p_global_data)
{
#define VALID_NEX_MAX(p, net) ((net <= p->net_supported_num_upper) && (net >= p->net_supported_num_lower))
	VENDOR_AI_NET_CFG_PROC_COUNT cfg_proc_count = {0};
	int result = 0;

	if (vendor_ai_cfg_get(VENDOR_AI_CFG_PROC_COUNT, (void *)&cfg_proc_count) == HD_OK) {
		if (VALID_NEX_MAX(p_global_data, p_global_data->net_supported_num_check)) {
			if (p_global_data->net_supported_num_check == (int)cfg_proc_count.max_proc_count) {
				printf("%s: count(%d) valid\n", __func__, (int)cfg_proc_count.max_proc_count);
				result = 0;
			} else {
				printf("%s: count(%d) invalid\n", __func__, (int)cfg_proc_count.max_proc_count);
				result = -1;
			}
		} else {
			if (VALID_NEX_MAX(p_global_data, (int)cfg_proc_count.max_proc_count)) {
				printf("%s: count(%d) valid\n", __func__, (int)cfg_proc_count.max_proc_count);
				result = 0;
			} else {
				printf("%s: count(%d) invalid\n", __func__, (int)cfg_proc_count.max_proc_count);
				result = -1;
			}
		}
	} else {
		printf("%s: get count failed\n", __func__);
		result = -1;
	}

	return result;
}

static int ai_test_proc_id(UINT32* test_proc_id, UINT32 test_id) {
	HD_RESULT ret = HD_OK;

	ret = vendor_ai_get_id(test_proc_id);
	if (ret != HD_OK) {
		printf("get id fail = %u\n", ret);
		return ret;
	}

	if (*test_proc_id != test_id) {
		printf("get id = %u not %u\n", *test_proc_id, test_id);
		return -1;
	}

	printf("get id = %u\r\n", *test_proc_id);

	return ret;
}
static int free_all_config_mem(FIRST_LOOP_DATA *first_loop_data_ptr)
{

       NET_MODEL_CONFIG *model_cfg;
       OPERATOR_CONFIG  *op_cfg;

       printf("free_all_config_mem start\n"); 
       model_cfg = first_loop_data_ptr->model_cfg;
       op_cfg = first_loop_data_ptr->op_cfg;        

       if(model_cfg != 0){
              printf("in_file_cfg  %x\n", model_cfg->in_file_cfg); 
              if(model_cfg->in_file_cfg != 0){         
                      printf("model_cfg->in_file_cfg != 0\n"); 
                     if(model_cfg->in_file_cfg->in_cfg != 0){    
                            printf("model_cfg->in_file_cfg->in_cfg\n");
                            free(model_cfg->in_file_cfg->in_cfg);                                 
                     } 
           
                free(model_cfg->in_file_cfg);             
              } 
              printf("multi_scale_valid_out\n");                           
              if(model_cfg->multi_scale_valid_out != 0) 
                     free(model_cfg->multi_scale_valid_out);    
              printf("model_cfg->diff_width\n");  
              if(model_cfg->diff_width != 0) 
                     free(model_cfg->diff_width);   
           
              if(model_cfg->diff_height != 0) 
                     free(model_cfg->diff_height);   
           
              if(model_cfg->diff_res_w != 0) 
                     free(model_cfg->diff_res_w);   
           
              if(model_cfg->diff_res_h != 0) 
                     free(model_cfg->diff_res_h);   
           
              if(model_cfg->diff_golden_sample != 0) 
                     free(model_cfg->diff_golden_sample);          
           
              printf("model_cfg free\n"); 
              free(model_cfg);   
              printf("model_cfg done\n"); 
       } 


       if(op_cfg != 0){
              if(op_cfg->in_file_cfg != 0){         
                     if(op_cfg->in_file_cfg->in_cfg != 0){    
                            free(op_cfg->in_file_cfg->in_cfg);                                 
                     }            
                     free(op_cfg->in_file_cfg);             
              } 
          
              free(op_cfg); 
       } 
              
                            
       printf("free_all_config_mem end\n");

       return 0;

}

int isRootProcess(pid_t pid)
{
    return (int) (pid != -1 && pid != 0);
}

pid_t createProcess(void)
{
    pid_t pid = 0;
	int ret = 0 ;

    if (isRootProcess(getppid())){
        pid = fork();
    
        if(pid == 0){
            ret = main_process();
			if (ret != 0){
				printf("=================== subprocess Fail =======================\r\n");
			}
        }
       
    }
    return pid;
}

MAIN(argc, argv)
{
    int ret = 0, i;
	pid_t pid_1 = 0 ; 

	global_fail = 0;
	memset(&global_data, 0, sizeof(GLOBAL_DATA));
	memset(&process_data, 0, sizeof(PROCESS_DATA));
	system("modprobe kflow_ai");

    if(argc > 1)
    	test = atoi(argv[1]);

    if(setup_test_data(test)){
		ret = -1 ;
        printf("fail to setup test data\n");
        printf("ai_test_suite %d finished\n", test);
        printf("[failed].\r\n");
        goto exit;
    }

    if(ai_test_cmd_parse_user_option(argc, argv)){
		ret = -1 ;
        printf("fail to parse user option\n");
        printf("ai_test_suite %d finished\n", test);
        printf("[failed].\r\n");
        goto exit;
    }

    if (!global_data.ctrl_c_reset){
        ret =  main_process();
    }else {
		// get initial sem & flag counts
		sem_used_cnt = vos_sem_used_cnt(); 
		flag_used_cnt = vos_flag_used_cnt(); 
        srand( (unsigned)time(NULL) );
		int min = 1;
        int max = 5;
        int x = rand() % (max - min + 1) + min;
        for (i = 0 ; i < 10 ; i++){
            if (isRootProcess(getppid())){
                pid_1 = createProcess();
                x = rand() % (max - min + 1) + min;
                sleep(x);
                printf("==================== After sleep :%d kill subprocess =========================\r\n", x);
                kill(pid_1, SIGKILL);
				sleep(3);
            }
        }
        if (isRootProcess(getppid())){
            pid_1 = createProcess();
            waitpid(pid_1, NULL, 0);
        }
    }

exit:
	for (i = 0 ; i < global_data.first_loop_count ; i++){
        if(global_data.first_loop_data != 0){
			free_all_config_mem(&global_data.first_loop_data[i]);
        }
    }

    if(global_data.first_loop_data != 0) {
    	free(global_data.first_loop_data);
    }
    return ret;
}

int main_process(void)
{
	int ret= 0, i;
	CHAR path_files[50] = "/mnt/sd/ai2_test_files";
	CHAR path_files_out[50] = "/mnt/sd/ai2_test_files/output";
	CHAR path_files_perf[50] = "/mnt/sd/ai2_test_files/output/perf";
	CHAR path_files_bin[50] = "/mnt/sd/ai2_test_files/output/bin";
	CHAR command_test[50];
	pid_t pid_child[VENDOR_AI_MAX_PROCESS] = {0}, pid_ret;
	int pid_loop_cnt = 0, pid_cnt = 0, pid_status;
	char *p_shm = NULL;


	if (global_data.multi_process) {

		/*
		 *  If multi_process_signal is on, we use signal to simulate ctrl-c item
		 *  and kill item, so we need to set signal handler.
		 *  Note: only set on the parent process before fork().
		 */
		if (global_data.multi_process_signal) {
			signal(SIGQUIT, ai_test_signal_hdanler);
			process_data.parent_id = getpid();
		}

		/*
		 *  If multi_process is on, we use fork() to create child as sub_process
		 *  to run net test and let original as main_process wait for sub_process
		 *  to finish.
		 *  Note: pid_cnt 0 represents main_process (process_0).
		 *        pid_cnt 1, 2 represents sub_process (process_1, process_2).
		 *        max_process_num is defined as VENDOR_AI_MAX_PROCESS (3).
		 */
		pid_cnt = 1;
main_process_fork:
		pid_child[pid_cnt] = fork();
		if (pid_child[pid_cnt] < 0) {//fork error
			printf("fork error\n");
			ret = -1;
			goto exit;
		} else if (0 == pid_child[pid_cnt]) {//child process
			process_data.process_id = pid_cnt;
			printf("pid %d: child process(%d)\n", getpid(), process_data.process_id);

			/* need to wait for the parent's hdal_init() to complete */
			do {
				p_shm = (char *)malloc_shm(5);
				usleep(1000*10);
			} while ((p_shm == NULL) || (strcmp(p_shm, "done") != 0));
			printf("pid %d: start\n", getpid());

		} else {//original process
			pid_cnt++;

			/* If the number of process does not match, fork() again. */
			if (pid_cnt < process_data.process_num) {
				goto main_process_fork;
			}

			/* need to notify children when hdal_init() completes using shared memory */
			if (pid_loop_cnt == 0) {
				process_data.process_id = 0;
				printf("pid %d: original process(%d)\n", getpid(), process_data.process_id);
				if((ret = hdal_init()) != HD_OK){
					printf("hdal init failed.\r\n");
					ret = -1;
				}
				p_shm = (char *)malloc_shm(5);
				memset(p_shm, 0, 5);
				sprintf(p_shm, "done");
				printf("pid %d: start\n", getpid());
			} else {
				/* need to do hdal_init() again for process inheritance issue */
				if((ret = hdal_init()) != HD_OK){
					printf("hdal init failed.\r\n");
					ret = -1;
				}
				sprintf(p_shm, "done");
			}

			/* If the number of process matches, wait() for child completes. */
			goto main_process_wait;
		}
	}

	if (global_data.perf_compare) {
		CHAR command_test[256];
		CHAR result_txt[256] = "/mnt/sd/ai2_test_files/output/perf/result.txt";
		CHAR result_compare_txt[256] = {0};
		CHAR diff_result_txt[256] = {0};

		if (strlen(global_data.perf_result_filename)) {
			sprintf(result_txt, "/mnt/sd/ai2_test_files/output/perf/%s", global_data.perf_result_filename);
		}

		if (access(result_txt,0) != 0){
			printf("result.txt is not exist\r\n");
			ret = -1;
			goto exit;
		}

		sprintf(command_test, "rm -rf /mnt/sd/ai2_test_files/output/perf_compare/");
		system(command_test);
		sprintf(command_test, "mkdir /mnt/sd/ai2_test_files/output/perf_compare/");
		system(command_test);

		//compare with last result
		if (!global_data.disable_perf_compare_last) {
			memset(result_compare_txt, 0, 256);
			strncpy(result_compare_txt, "/mnt/sd/ai2_test_files/output/perf_last/result.txt", 256-1);
			result_compare_txt[256-1] = '\0';

			if (strlen(global_data.perf_result_filename)) {
				sprintf(result_compare_txt, "/mnt/sd/ai2_test_files/output/perf_last/%s", global_data.perf_result_filename);
			}

			memset(diff_result_txt, 0, 256);
			strncpy(diff_result_txt, "/mnt/sd/ai2_test_files/output/perf_compare/result_compare_last.txt", 256-1);
			diff_result_txt[256-1] = '\0';

			if (strlen(global_data.perf_result_filename)) {
				sprintf(diff_result_txt, "/mnt/sd/ai2_test_files/output/perf_compare/result_compare_last_%s", global_data.perf_result_filename);
			}

			if (access(result_compare_txt,0) != 0){
				printf("last result.txt is not exist\r\n");
				ret = -1;
				goto exit;
			}

			ret = compare_last_result(result_txt, result_compare_txt, diff_result_txt, &global_data);
		}

		//compare with cpu result
		if (!global_data.disable_perf_compare_cpu) {
			memset(result_compare_txt, 0, 256);
			strncpy(result_compare_txt, "/mnt/sd/ai2_test_files/output/perf/result_cpu.txt", 256-1);
			result_compare_txt[256-1] = '\0';

			memset(diff_result_txt, 0, 256);
			strncpy(diff_result_txt, "/mnt/sd/ai2_test_files/output/perf_compare/result_compare_cpu.txt", 256-1);
			diff_result_txt[256-1] = '\0';

			if (access(result_compare_txt,0) != 0){
				printf("result_cpu.txt is not exist\r\n");
				ret = -1;
				goto exit;
			}

			compare_stress_result(result_txt, result_compare_txt, diff_result_txt);
		}

		//compare with dma result
		if (!global_data.disable_perf_compare_dma) {
			memset(result_compare_txt, 0, 256);
			strncpy(result_compare_txt, "/mnt/sd/ai2_test_files/output/perf/result_dma.txt", 256-1);
			result_compare_txt[256-1] = '\0';

			memset(diff_result_txt, 0, 256);
			strncpy(diff_result_txt, "/mnt/sd/ai2_test_files/output/perf_compare/result_compare_dma.txt", 256-1);
			diff_result_txt[256-1] = '\0';

			if (access(result_compare_txt,0) != 0){
				printf("result_dma.txt is not exist\r\n");
				ret = -1;
				goto exit;
			}

			compare_stress_result(result_txt, result_compare_txt, diff_result_txt);
		}

	} else {

		if (global_data.first_loop_data[0].perf_analyze) {
			sprintf(command_test, "mkdir %s", path_files);
			if (access(path_files,0)){
				system(command_test);
			}

			sprintf(command_test, "mkdir %s", path_files_out);
			if (access(path_files_out,0)){
				system(command_test);
			}

			sprintf(command_test, "mkdir %s", path_files_perf);
			if (access(path_files_perf,0)){
				system(command_test);
			}

			if (global_data.cpu_stress[0].stress_level == 0 && global_data.dma_stress[0].stress_level == 0) {
				if (strlen(global_data.perf_result_filename)) {
					char cmd[512];
					snprintf(cmd, sizeof(cmd), "/mnt/sd/ai2_test_files/output/perf/%s", global_data.perf_result_filename);
					if (access(cmd,0) == 0){
						snprintf(cmd, sizeof(cmd), "rm /mnt/sd/ai2_test_files/output/perf/%s", global_data.perf_result_filename);
						system(cmd);
					}
				} else if (global_data.share_model) {
					if (access("/mnt/sd/ai2_test_files/output/perf/result_share_model.txt",0) == 0){
						system("rm /mnt/sd/ai2_test_files/output/perf/result_share_model.txt");
					}
				} else if (global_data.first_loop_count == 1) {
					if (access("/mnt/sd/ai2_test_files/output/perf/result.txt",0) == 0){
						system("rm /mnt/sd/ai2_test_files/output/perf/result.txt");
					}
				} else {
					if (access("/mnt/sd/ai2_test_files/output/perf/result_0.txt",0) == 0){
						system("rm /mnt/sd/ai2_test_files/output/perf/result_0.txt");
					}

					if (access("/mnt/sd/ai2_test_files/output/perf/result_1.txt",0) == 0){
						system("rm /mnt/sd/ai2_test_files/output/perf/result_1.txt");
					}

					if (access("/mnt/sd/ai2_test_files/output/perf/result_2.txt",0) == 0){
						system("rm /mnt/sd/ai2_test_files/output/perf/result_2.txt");
					}
				}
			} else if (global_data.cpu_stress[0].stress_level != 0) {
				if (global_data.first_loop_count == 1) {
					if (access("/mnt/sd/ai2_test_files/output/perf/result_cpu.txt",0) == 0){
						system("rm /mnt/sd/ai2_test_files/output/perf/result_cpu.txt");
					}
				} else {
					if (access("/mnt/sd/ai2_test_files/output/perf/result_cpu_0.txt",0) == 0){
						system("rm /mnt/sd/ai2_test_files/output/perf/result_cpu_0.txt");
					}

					if (access("/mnt/sd/ai2_test_files/output/perf/result_cpu_1.txt",0) == 0){
						system("rm /mnt/sd/ai2_test_files/output/perf/result_cpu_1.txt");
					}

					if (access("/mnt/sd/ai2_test_files/output/perf/result_cpu_2.txt",0) == 0){
						system("rm /mnt/sd/ai2_test_files/output/perf/result_cpu_2.txt");
					}
				}
			} else if (global_data.dma_stress[0].stress_level != 0) {
				if (global_data.first_loop_count == 1) {
					if (access("/mnt/sd/ai2_test_files/output/perf/result_dma.txt",0) == 0){
						system("rm /mnt/sd/ai2_test_files/output/perf/result_dma.txt");
					}
				} else {
					if (access("/mnt/sd/ai2_test_files/output/perf/result_dma_0.txt",0) == 0){
						system("rm /mnt/sd/ai2_test_files/output/perf/result_dma_0.txt");
					}

					if (access("/mnt/sd/ai2_test_files/output/perf/result_dma_1.txt",0) == 0){
						system("rm /mnt/sd/ai2_test_files/output/perf/result_dma_1.txt");
					}

					if (access("/mnt/sd/ai2_test_files/output/perf/result_dma_2.txt",0) == 0){
						system("rm /mnt/sd/ai2_test_files/output/perf/result_dma_2.txt");
					}
				}
			}

			if (global_data.first_loop_data[0].perf_analyze_report) {
				if (global_data.first_loop_count == 1) {
					CHAR report_path[256];

					snprintf(report_path, 255, "/mnt/sd/ai2_test_files/output/perf/network_performance_J%dW%dS%dB%d.csv", (int)global_data.first_loop_data[0].net_job_opt.method, (int)global_data.first_loop_data[0].net_job_opt.wait_ms, (int)global_data.first_loop_data[0].net_job_opt.schd_parm, (int)global_data.first_loop_data[0].net_buf_opt.method);
					sprintf(command_test, "rm %s", report_path);

					if (access(report_path,0) == 0){
						system(command_test);
					}
				}
			}
		}

		if (global_data.dump_output) {

			sprintf(command_test, "mkdir %s", path_files);
			if (access(path_files,0)){
				system(command_test);
			}

			sprintf(command_test, "mkdir %s", path_files_out);
			if (access(path_files_out,0)){
				system(command_test);
			}

			sprintf(command_test, "mkdir %s", path_files_bin);
			if (access(path_files_bin,0)){
				system(command_test);
			}
		}

		if(strlen(global_data.generate_golden_output)){
			if((ret = setup_dumping_golden_output()) != HD_OK){
				printf("fail to setup dumping golden output.\r\n");
				goto exit;
			}
		}

		if((ret = hdal_init()) != HD_OK){
			printf("hdal init failed.\r\n");
			goto exit;
		}

		if(create_cpu_stress_thread(global_data.cpu_stress)){
			ret = -1;
			printf("fail to create cpu stress thread\n");
			goto hdal_uninit;
		}

		if(create_dma_stress_thread(global_data.dma_stress)){
			ret = -1;
			printf("fail to create cpu stress thread\n");
			goto hdal_uninit;
		}

		if (global_data.first_loop_data[0].proc_id_test) {
			UINT32 test_proc_id;

			if (vendor_ai_get_id(&test_proc_id) != HD_ERR_UNINIT) {
				printf("get id = %u, not HD_ERR_UNINIT\n", ret);
				goto hdal_uninit;
			}
		}

		if(global_data.init_uninit_per_thread){
			if ((ret = hd_videoproc_init()) != HD_OK) {
				printf("hd_videoproc_init fail=%d\n", ret);
				if (global_data.net_supported_num_upper && global_data.net_supported_num_lower) {
					ret = ai_test_check_net_supported_num(&global_data);
					if (ret < 0) {
						printf("check net max failed(%d)\n", ret);
					}
				}
				goto hdal_uninit;
			}
			if (global_data.net_supported_num_upper && global_data.net_supported_num_lower) {
				ret = ai_test_check_net_supported_num(&global_data);
				if (ret < 0) {
					printf("check net max failed(%d)\n", ret);
					goto ai_uninit;
				}
			}
		}else{
			if ((ret = ai_test_network_init_global(global_data.ai_proc_schedule, global_data.share_model)) != HD_OK) {
				printf("net init error=%d\n", ret);
				if (global_data.net_supported_num_upper && global_data.net_supported_num_lower) {
					ret = ai_test_check_net_supported_num(&global_data);
					if (ret < 0) {
						printf("check net max failed(%d)\n", ret);
					}
				}
				goto hdal_uninit;
			}
			if (global_data.net_supported_num_upper && global_data.net_supported_num_lower) {
				ret = ai_test_check_net_supported_num(&global_data);
				if (ret < 0) {
					printf("check net max failed(%d)\n", ret);
					goto ai_uninit;
				}
			}
		}

		if (global_data.first_loop_data[0].proc_id_test) {
			UINT32 test_proc_id;
			UINT32 test_id = 0, i;

			//id = 0
			{
				ret = ai_test_proc_id(&test_proc_id, test_id);

				if (ret != HD_OK) {
					goto ai_uninit;
				}
				test_id++;
			}

			//id = 1~3
			{
				for (i = 0; i < 3; i++) {
					ret = ai_test_proc_id(&test_proc_id, test_id);

					if (ret != HD_OK) {
						goto ai_uninit;
					}
					test_id++;
				}
			}

			//release id 1, get id 1, 4
			{
				if (vendor_ai_release_id(1) != HD_OK) {
					printf("rel id fail = %u\n", ret);
					goto ai_uninit;
				}

				printf("rel id = 1\r\n");
				test_id = 1;

				ret = ai_test_proc_id(&test_proc_id, test_id);

				if (ret != HD_OK) {
					goto ai_uninit;
				}
				test_id = 4;

				ret = ai_test_proc_id(&test_proc_id, test_id);

				if (ret != HD_OK) {
					goto ai_uninit;
				}
				test_id++;
			}

			//get id 5~max, check > max path
			{

				for(i = test_id; i < global_data.first_loop_data[0].net_path_max; i++) {
					ret = ai_test_proc_id(&test_proc_id, test_id);

					if (ret != HD_OK) {
						goto ai_uninit;
					}
					test_id++;
				}

				if (vendor_ai_get_id(&test_proc_id) != HD_ERR_RESOURCE) {
					printf("get id = %u, not HD_ERR_RESOURCE\n", ret);
					goto ai_uninit;
				}
			}

			ret = HD_OK;

			for(i = 0 ; i < (UINT32)global_data.first_loop_count ; i++) {
				global_data.first_loop_data[i].test_success = 1;
			}

			goto ai_uninit;
		}

		if(global_data.sequential){
			if(global_data.sequential_loop_count <= 0){
				ret = -1;
				printf("invalid sequential_loop_count(%d)\n", global_data.sequential_loop_count);
				goto ai_uninit;
			}

			ret = pthread_create(&global_data.thread_id, NULL, network_sequential_thread, &global_data);
			if (ret < 0) {
				printf("pthread_create() fail\n");
				goto ai_uninit;
			}

			ret = wait_all_done_or_timeout();
			if(ret)
				goto ai_uninit;

			pthread_join(global_data.thread_id, NULL);
		}else{
			for(i = 0 ; i < global_data.first_loop_count ; ++i){
				ret = pthread_create(&global_data.first_loop_data[i].thread_id, NULL, network_parallel_thread, (VOID*)&(global_data.first_loop_data[i]));
				if (ret < 0) {
					printf("pthread_create() fail\n");
					break;
				}
			}

			ret = wait_all_done_or_timeout();
			if(ret)
				goto ai_uninit;

			for(i = 0 ; i < global_data.first_loop_count ; ++i){
				if(global_data.first_loop_data[i].thread_id)
					pthread_join(global_data.first_loop_data[i].thread_id, NULL);
			}
		}

ai_uninit:
		if(global_data.init_uninit_per_thread){
			if ((ret = hd_videoproc_uninit()) != HD_OK) {
				printf("hd_videoproc_uninit fail=%d\n", ret);
			}
		}else{
			if (ai_test_network_uninit_global() != HD_OK) {
				printf("net uninit error\n");
			}
		}
hdal_uninit:
		if(destroy_cpu_stress_thread(global_data.cpu_stress)){
			printf("fail to destroy cpu stress thread\n");
		}

		if(destroy_dma_stress_thread(global_data.dma_stress)){
			printf("fail to destroy dma stress thread\n");
		}

		if (hdal_uninit() != HD_OK) {
			printf("hdal uninit error\n");
		}
	}

main_process_wait:
	if ((global_data.multi_process) && (process_data.process_id == 0)) {
		int cnt;

		/*
		 *  If multi_process_signal is on, we use signal to simulate ctrl-c item
		 *  and kill item.
		 */
		if (global_data.multi_process_signal) {
			usleep(1000*1000);
			if ((pid_loop_cnt % 4) == 0) { //loop 0, 2,...
				/* simulate ctrl-c item */
				ai_test_signal_quit();
			} else if ((pid_loop_cnt % 4) != 0 && (pid_loop_cnt % 2) == 0) { //loop 1, 3,...
				/* simulate kill item */
				ai_test_signal_kill(pid_child[1]);
			}
		}

		/*
		 *  If multi_process is on, we use wait() to wait for the child process
		 *  to finish its own run net test and use WIFEXITED/WEXITSTATUS to check
		 *  the return result
		 *  Note: pid_cnt 0 represents main_process (process_0).
		 *        pid_cnt 1, 2 represents sub_process (process_1, process_2).
		 *        max_process_num is defined as VENDOR_AI_MAX_PROCESS (3).
		 */
		pid_ret = wait(&pid_status); //waitpid
		if (pid_ret < 0) {//wait error
			printf("wait error\n");
			process_data.result = -1;
			ret = -1;
			goto main_process_exit;
		} else {//wait success
			/* Compare the returned pid with the entire child process */
			for (cnt = 1; cnt < process_data.process_num; cnt++) {
				if (pid_ret == pid_child[cnt]) {
					break;
				}
			}
			if (cnt == process_data.process_num) {
				printf("child(%d) return fail\n", pid_ret);
				process_data.result = -1;
				ret = -1;
				goto main_process_exit;
			}
			/* Use WIFEXITED/WEXITSTATUS to determine if the test passed */
			if (WIFEXITED(pid_status)) {
				printf("child(%d) exited normally with status = %d\n", pid_ret, WEXITSTATUS(pid_status));
				if ((WEXITSTATUS(pid_status) == 0) && (process_data.result == 0)) {
					ret = 0;
				} else if (global_data.multi_process_signal && (WEXITSTATUS(pid_status) == 7) && (process_data.result == 0)) {
					/* simulate ctrl-c item */
					ret = 0;
				} else {
					process_data.result = -1;
					ret = -1;
				}
			} else {
				printf("child(%d) abnormal exit, status = %d\n", pid_ret, pid_status);
				if (global_data.multi_process_signal && (process_data.result == 0)) {
					/* simulate kill item */
					ret = 0;
				} else {
					process_data.result = -1;
					ret = -1;
				}
			}
		}
		pid_loop_cnt++;
		printf("pid %d: waitpid(%d) done. loopcnt(%d).\n", getpid(), pid_ret, pid_loop_cnt);
main_process_exit:
		pid_cnt--;
		if (pid_cnt > 1) {
			goto main_process_wait;
		}
		if (hdal_uninit() != HD_OK) {
			printf("hdal uninit error\n");
		}
		/* If there are not enough loops, try fork() more process */
		if (pid_loop_cnt < global_data.multi_process_loop_count * (process_data.process_num-1)) {
			memset(p_shm, 0, 5);
			goto main_process_fork;
		}
	}

exit:
	if ((global_data.multi_process) && (process_data.process_id > 0)) {
		printf("pid %d: finished\n", getpid());
		if(ret || global_fail){
			process_data.result = -1;
		} else if (global_data.perf_compare == 1){
			process_data.result = 0;
		} else if (global_data.net_supported_num_upper && global_data.net_supported_num_lower) {
			process_data.result = 0;
		} else {
			ai_test_lock_and_sync();

			for(i = 0 ; i < global_data.first_loop_count ; ++i)
				if(global_data.first_loop_data[i].test_success != 1)
					break;

			ai_test_unlock();

			if(i == global_data.first_loop_count && global_data.first_loop_count)
				process_data.result = 0;
			else
				process_data.result = -1;
		}

		/*
		 *  If multi_process is on, we need to free shared memory and use exit() to
		 *  transfer the result of sub_process to main_process.
		 */
		free_shm((void *)p_shm);
		exit(process_data.result);
	}
	printf("ai_test_suite %d finished\n", test);
	if(ret || global_fail){
		printf("[failed].\r\n");
	} else if (global_data.perf_compare == 1){
		printf("[success].\r\n");
	} else if (global_data.net_supported_num_upper && global_data.net_supported_num_lower) {
		printf("[success].\r\n");
	} else if ((global_data.multi_process) && (process_data.process_id == 0)) {
		if (process_data.result)
			printf("[failed].\r\n");
		else
			printf("[success].\r\n");
	} else {
		ai_test_lock_and_sync();

		for(i = 0 ; i < global_data.first_loop_count ; ++i)
			if(global_data.first_loop_data[i].test_success != 1)
				break;

		ai_test_unlock();
		if(i == global_data.first_loop_count && global_data.first_loop_count){
			if (!global_data.ctrl_c_reset || (sem_used_cnt == vos_sem_used_cnt() && flag_used_cnt == vos_flag_used_cnt())){
				printf("[success].\r\n");
			}else {
				if (sem_used_cnt != vos_sem_used_cnt()) {
					printf("ERR: vos resource is leaked! Before %d After %d sem_cnt increased %d \r\n",sem_used_cnt, vos_sem_used_cnt(), vos_sem_used_cnt() - sem_used_cnt) ;
				}
				if (flag_used_cnt != vos_flag_used_cnt()) {
					printf("ERR: vos resource is leaked! Before %d After %d flag_cnt increased %d \r\n",flag_used_cnt, vos_flag_used_cnt(),vos_flag_used_cnt() - flag_used_cnt) ;
				}
				printf("[failed].\r\n");
			}
		}else
			printf("[failed].\r\n");
	}

	return ret;
}


