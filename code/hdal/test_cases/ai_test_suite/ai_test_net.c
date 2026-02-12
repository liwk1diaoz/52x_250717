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
#include "ai_test_config.h"
#include <sys/time.h>
#include "vendor_ai.h"
/*-----------------------------------------------------------------------------*/
/* Network Functions                                                           */
/*-----------------------------------------------------------------------------*/

extern INT32 vendor_mau_ch_mon_start(int ch, int rw, int dram);
extern UINT64 vendor_mau_ch_mon_stop(int ch, int dram);
extern HD_RESULT _vendor_ai_net_debug_get_memsize(UINT32 proc_id, UINT32* p_model_size, UINT32* p_initbuf_size, UINT32* p_workbuf_size);

#define AI_TEST_ROUND(x) (x + 0.5)
static INT32 _getsize_model(char* filename)
{
	FILE *bin_fd;
	UINT32 bin_size = 0;

	bin_fd = fopen(filename, "rb");
	if (!bin_fd) {
		printf("get bin(%s) size fail\n", filename);
		return 0;
	}

	fseek(bin_fd, 0, SEEK_END);
	bin_size = ftell(bin_fd);
	fseek(bin_fd, 0, SEEK_SET);
	fclose(bin_fd);

	return bin_size;
}

static UINT32 _load_model(CHAR *filename, UINT32 va)
{
	FILE  *fd;
	UINT32 file_size = 0, read_size = 0;
	const UINT32 model_addr = va;
	//DBG_DUMP("model addr = %08x\r\n", (int)model_addr);

	fd = fopen(filename, "rb");
	if (!fd) {
		printf("load model(%s) fail\r\n", filename);
		return 0;
	}

	fseek ( fd, 0, SEEK_END );
	file_size = ALIGN_CEIL_4( ftell(fd) );
	fseek ( fd, 0, SEEK_SET );

	read_size = fread ((void *)model_addr, 1, file_size, fd);
	if (read_size != file_size) {
		printf("size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)file_size);
	}
	fclose(fd);

	//printf("load model(%s) ok\r\n", filename);
	return read_size;
}

static HD_RESULT _load_label(UINT32 addr, UINT32 line_len, const CHAR *filename)
{
	FILE *fd;
	CHAR *p_line = (CHAR *)addr;

	fd = fopen(filename, "r");
	if (!fd) {
		printf("load label(%s) fail\r\n", filename);
		return HD_ERR_NG;
	}

	while (fgets(p_line, line_len, fd) != NULL) {
		p_line[strlen(p_line) - 1] = '\0'; // remove newline character
		p_line += line_len;
	}

	if (fd) {
		fclose(fd);
	}

	printf("load label(%s) ok\r\n", filename);
	return HD_OK;
}

static void show_ai_sdk_version(void)
{
	printf("dumping ai sdk version\n");
	system("cat /proc/kflow_ai/version");
	sleep(3);
}

HD_RESULT ai_test_network_init(UINT32 ai_proc_schedule, int share_model, int init_uninit_per_thread)
{
	HD_RESULT ret = HD_OK;
	GLOBAL_DATA* p_global_data = ai_test_get_global_data();
	PROCESS_DATA* p_process_data = ai_test_get_process_data();

	ret = ai_test_network_set_config(ai_proc_schedule);
	if (HD_OK != ret) {
		printf("network_set_config fail=%d\n", ret);
		return 0;
	}

	// config share model mode 1
	if(share_model){
		printf("VENDOR_AI_CFG_SHAREMODEL_MODE %d is set \n", share_model);
		ret = vendor_ai_cfg_set(VENDOR_AI_CFG_SHAREMODEL_MODE, &share_model);
		if (ret != HD_OK) {
			printf("VENDOR_AI_CFG_SHAREMODEL_MODE fail=%d\n", ret);
			return ret;
		}
	}

	if(init_uninit_per_thread){
		UINT32 is_multi = 1;
		ret = vendor_ai_cfg_set(VENDOR_AI_CFG_MULTI_THREAD, &is_multi);
		if (ret != HD_OK) {
			printf("VENDOR_AI_CFG_MULTI_THREAD fail=%d\n", ret);
			return ret;
		}
	}

	if ((p_global_data->multi_process) && (p_process_data->process_id > 0)) {
		UINT32 is_multi_process = 1;
		ret = vendor_ai_cfg_set(VENDOR_AI_CFG_MULTI_PROCESS, &is_multi_process);
		if (ret != HD_OK) {
			printf("VENDOR_AI_CFG_MULTI_PROCESS fail=%d\n", ret);
			return ret;
		}
	}

	ret = vendor_ai_init();
	if (ret != HD_OK) {
		printf("vendor_ai_init fail=%d\n", ret);
		return ret;
	}

	show_ai_sdk_version();

	return ret;
}

HD_RESULT ai_test_network_init_global(UINT32 ai_proc_schedule, int share_model)
{
	HD_RESULT ret = HD_OK;
	GLOBAL_DATA* p_global_data = ai_test_get_global_data();
	PROCESS_DATA* p_process_data = ai_test_get_process_data();

	/* If it is sub_process, do ai_test_network_init() only */
	if ((p_global_data->multi_process) && (p_process_data->process_id > 0)) {
		goto label_ai_test_network_init;
	}

	ret = hd_videoproc_init();
	if (ret != HD_OK) {
		printf("hd_videoproc_init fail=%d\n", ret);
		return ret;
	}

label_ai_test_network_init:
	return ai_test_network_init(ai_proc_schedule, share_model, 0);
}

HD_RESULT ai_test_network_uninit(void)
{
	HD_RESULT ret = HD_OK;

	ret = vendor_ai_uninit();
	if (ret != HD_OK) {
		printf("vendor_ai_uninit fail=%d\n", ret);
	}

	return ret;
}

HD_RESULT ai_test_network_uninit_global(void)
{
	HD_RESULT ret = HD_OK;
	GLOBAL_DATA* p_global_data = ai_test_get_global_data();
	PROCESS_DATA* p_process_data = ai_test_get_process_data();

	/* If it is sub_process, do ai_test_network_uninit() only */
	if ((p_global_data->multi_process) && (p_process_data->process_id > 0)) {
		goto label_ai_test_network_uninit;
	}

	ret = hd_videoproc_uninit();
	if (ret != HD_OK) {
		printf("hd_videoproc_uninit fail=%d\n", ret);
	}

label_ai_test_network_uninit:
	return ai_test_network_uninit();
}

INT32 ai_test_common_mem_config(GLOBAL_DATA *global_data)
{
	NET_MODEL_CONFIG *p_model_cfg = NULL;
	HD_COMMON_MEM_INIT_CONFIG* p_mem_cfg;
	INT32            max_size = 0, max_diff_size = 0, i, j, index = 0;
	FIRST_LOOP_DATA *first_loop;
	int first_loop_count = global_data->first_loop_count;
	PROCESS_DATA* p_process_data = ai_test_get_process_data();

	if(!global_data){
		printf("global_data is null\n");
		return -1;
	}

	first_loop = global_data->first_loop_data;
	p_mem_cfg = &(global_data->mem_cfg);
	if(!first_loop || !p_mem_cfg){
		printf("first_loop(%p) or p_mem_cfg(%p) is null\n", first_loop, p_mem_cfg);
		return -1;
	}

	if (global_data->multi_process) {
		if (p_process_data->process_id == 0) { //main_process
			for(i = 0 ; i < global_data->first_loop_count; ++i){
				memcpy(&first_loop[i + global_data->first_loop_count], &first_loop[i], sizeof(FIRST_LOOP_DATA));
			}
			first_loop_count = global_data->first_loop_count * (p_process_data->process_num-1);
			//set mem seq
			for(i = 0 ; i < first_loop_count; ++i){
				first_loop[i].mem_seq = i;
			}
		} else { //sub_process
			first_loop_count = global_data->first_loop_count;
			//set mem seq
			for(i = 0 ; i < first_loop_count; ++i){
				first_loop[i].mem_seq = i + global_data->first_loop_count * (p_process_data->process_id - 1);
			}
		}
	}

	for(i = 0 ; i < first_loop_count; ++i){

		p_model_cfg = first_loop[i].model_cfg;
		if(!p_model_cfg || !first_loop[i].second_cfg_count){
			continue;
		}

		if(!global_data->sequential)
			max_size = 0;

		for(j = 0 ; j < first_loop[i].second_cfg_count ; ++j){
			if (strlen(p_model_cfg[j].model_filename) == 0) {
				printf("input model[%d] has null file name\r\n", (int)j);
				return -1;
			}

			//if this first_loop shares model memory from master, no need to register its own memory
			if(first_loop->share_model_master != NULL){
				printf("net_path(%d) is sharing model from net_path(%d)\n", (int)first_loop->net_path, (int)first_loop->share_model_master->net_path);
				continue;
			}

			p_model_cfg[j].binsize = _getsize_model(p_model_cfg[j].model_filename);
			if (p_model_cfg[j].binsize <= 0) {
				printf("input model(%s) is not exist?\r\n", p_model_cfg[j].model_filename);
				printf("do not stop. keep testing other models\r\n");
				global_fail = 1;
				continue;
			}
			if(p_model_cfg[j].binsize > max_size)
				max_size = p_model_cfg[j].binsize;

			printf("model[%d] set net_mem_cfg: model-file(%s), binsize=%d\r\n",
				(int)first_loop[i].net_path,
				p_model_cfg[j].model_filename,
				(int)p_model_cfg[j].binsize);

			printf("model[%d] set net_mem_cfg: label-file(%s)\r\n",
				(int)first_loop[i].net_path,
				p_model_cfg[j].label_filename);
		}

		if(!global_data->sequential)
			max_diff_size = 0;

		for(j = 0 ; j < first_loop[i].second_cfg_count ; ++j){
			if (strlen(p_model_cfg[j].diff_filename) == 0)
				continue;

			p_model_cfg[j].diff_binsize = _getsize_model(p_model_cfg[j].diff_filename);
			if (p_model_cfg[j].diff_binsize <= 0) {
				printf("diff model(%s) is not exist?\r\n", p_model_cfg[j].diff_filename);
				printf("do not stop. keep testing other models\r\n");
				global_fail = 1;
				continue;
			}
			if(p_model_cfg[j].diff_binsize > max_diff_size)
				max_diff_size = p_model_cfg[j].diff_binsize;

			printf("model[%d] set net_mem_cfg: diff-file(%s), binsize=%d\r\n",
				(int)first_loop[i].net_path,
				p_model_cfg[j].diff_filename,
				(int)p_model_cfg[j].diff_binsize);
		}

		if(!global_data->sequential){
			//if this first_loop doesn't share model memory, register its own memory
			if(first_loop->share_model_master == NULL){
				if (global_data->multi_process) {
					p_mem_cfg->pool_info[index].type = HD_COMMON_MEM_USER_DEFINIED_POOL + first_loop[i].mem_seq*2;
				} else {
					p_mem_cfg->pool_info[index].type = HD_COMMON_MEM_USER_DEFINIED_POOL + first_loop[i].net_path*2;
				}
				p_mem_cfg->pool_info[index].blk_size = max_size;
				p_mem_cfg->pool_info[index].blk_cnt = 1;
				p_mem_cfg->pool_info[index].ddr_id = DDR_ID1;
				index++;
			}

			if(max_diff_size){
				if (global_data->multi_process) {
					p_mem_cfg->pool_info[index].type = HD_COMMON_MEM_USER_DEFINIED_POOL + first_loop[i].mem_seq*2 + 1;
				} else {
					p_mem_cfg->pool_info[index].type = HD_COMMON_MEM_USER_DEFINIED_POOL + first_loop[i].net_path*2 + 1;
				}
				p_mem_cfg->pool_info[index].blk_size = max_diff_size;
				p_mem_cfg->pool_info[index].blk_cnt = 1;
				p_mem_cfg->pool_info[index].ddr_id = DDR_ID1;
				index++;
			}
		}
	}

	if(global_data->sequential){
		p_mem_cfg->pool_info[index].type = HD_COMMON_MEM_USER_DEFINIED_POOL;
		p_mem_cfg->pool_info[index].blk_size = max_size;
		p_mem_cfg->pool_info[index].blk_cnt = 1;
		p_mem_cfg->pool_info[index].ddr_id = DDR_ID1;
		index++;

		if(max_diff_size){
			p_mem_cfg->pool_info[index].type = HD_COMMON_MEM_USER_DEFINIED_POOL + 1;
			p_mem_cfg->pool_info[index].blk_size = max_diff_size;
			p_mem_cfg->pool_info[index].blk_cnt = 1;
			p_mem_cfg->pool_info[index].ddr_id = DDR_ID1;
			index++;
		}
	}

	return 0;
}

HD_RESULT ai_test_network_set_config(UINT32 ai_proc_schedule)
{
	HD_RESULT ret = HD_OK;

	// config extend engine plugin, process scheduler
	{
		//UINT32 schd = VENDOR_AI_PROC_SCHD_FAIR;
		printf("VENDOR_AI_CFG_PROC_SCHD=%d\n", (int)ai_proc_schedule);
		vendor_ai_cfg_set(VENDOR_AI_CFG_PLUGIN_ENGINE, vendor_ai_cpu1_get_engine());
		vendor_ai_cfg_set(VENDOR_AI_CFG_PROC_SCHD, &ai_proc_schedule);
	}

	return ret;
}

HD_RESULT ai_test_network_set_config_job(NET_PATH_ID net_path, VENDOR_AI_NET_CFG_JOB_OPT *opt, int schd_parm_inc)
{
	HD_RESULT ret;

	if(!opt){
		printf("opt is null\n");
		return -1;
	}

	if(schd_parm_inc != 0 && schd_parm_inc != 1 && schd_parm_inc != -1){
		printf("invalid [schd_parm_inc]=%d, only 0/1/-1 are valid\n", schd_parm_inc);
		return -1;
	}

	// set job opt
	{
		printf("net_path(%lu) set net_cfg: job-opt(%d), sync(%d), core(%d)\r\n",
					net_path, opt->method, (int)opt->wait_ms, (int)opt->schd_parm);
		ret = vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_CFG_JOB_OPT, opt);

		if(schd_parm_inc == 1){
			if(opt->schd_parm == 0)
				opt->schd_parm = 1;
			else if(opt->schd_parm == 1)
				opt->schd_parm = 255;
			else
				opt->schd_parm = 0;
		}else if(schd_parm_inc == -1){
			if(opt->schd_parm == 0)
				opt->schd_parm = 255;
			else if(opt->schd_parm == 1)
				opt->schd_parm = 0;
			else
				opt->schd_parm = 1;
		}
	}

	return ret;
}

HD_RESULT ai_test_network_set_config_buf(NET_PATH_ID net_path, UINT32 method, UINT32 ddr_id)
{
	HD_RESULT ret;

	// set buf opt
	{
		VENDOR_AI_NET_CFG_BUF_OPT cfg_buf_opt = {0};
		cfg_buf_opt.method = method;
		cfg_buf_opt.ddr_id = ddr_id; //DDR_ID0;
		printf("net_path(%lu) set net_cfg: buf-opt(%d), ddr_id(%u)\r\n",
					net_path, (int)cfg_buf_opt.method, cfg_buf_opt.ddr_id);
		ret = vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_CFG_BUF_OPT, &cfg_buf_opt);
	}

	return ret;
}

static HD_RESULT ai_test_network_set_config_model(FIRST_LOOP_DATA *first_loop, NET_MODEL_CONFIG* p_model_cfg, int sequential)
{
	HD_RESULT ret = HD_OK;
	UINT32 loadsize = 0;
	VENDOR_AI_NET_CFG_MODEL cfg = {0}, *p_cfg;
	GLOBAL_DATA* p_global_data = ai_test_get_global_data();
	PROCESS_DATA* p_process_data = ai_test_get_process_data();

	if(!first_loop || !p_model_cfg) {
		printf("first_loop(%p) or p_model_cfg(%p) is null\r\n", first_loop, p_model_cfg);
		return HD_ERR_NULL_PTR;
	}

	if (strlen(p_model_cfg->model_filename) == 0) {
		printf("input model name is null\r\n");
		return HD_ERR_NULL_PTR;
	}

	//only 1. none share model 2. share model master needs to setup model
	if(first_loop->share_model_master == NULL){
		if ((p_global_data->multi_process) && (p_process_data->process_id > 0)) {
			printf("process_id(%d) get pool(%d)\n", p_process_data->process_id, first_loop->mem_seq*2);
			if(sequential)
				ret = ai_test_mem_get(&first_loop->model_mem, p_model_cfg->binsize, 0);
			else
				ret = ai_test_mem_get(&first_loop->model_mem, p_model_cfg->binsize, first_loop->mem_seq*2);
		} else {
			if(sequential)
				ret = ai_test_mem_get(&first_loop->model_mem, p_model_cfg->binsize, 0);
			else
				ret = ai_test_mem_get(&first_loop->model_mem, p_model_cfg->binsize, first_loop->net_path*2);
		}
		if (ret != HD_OK) {
			printf("get mem fail=%d\n", ret);
			return HD_ERR_FAIL;
		}

		//load file
		loadsize = _load_model(p_model_cfg->model_filename, first_loop->model_mem.va);
		if (loadsize <= 0) {
			printf("input model load fail: %s\r\n", p_model_cfg->model_filename);
			return HD_ERR_FAIL;
		}
	}

	if(strlen(p_model_cfg->label_filename)){
		ret = _load_label((UINT32)first_loop->out_class_labels, VENDOR_AIS_LBL_LEN, p_model_cfg->label_filename);
		if (ret != HD_OK) {
			printf("load label fail=%s\n", p_model_cfg->label_filename);
			return HD_ERR_FAIL;
		}
	}

	if(wait_for_loading_share_model(first_loop)){
		printf("fail to wait for loading share model\n");
		return -1;
	}

	// set model
	//1. none share model 2. share model master uses its own model memroy
	if(first_loop->share_model_master == NULL){
		cfg.pa = first_loop->model_mem.pa;
		cfg.va = first_loop->model_mem.va;
		cfg.size = first_loop->model_mem.size;
	}else{//share model slaves use master's model memory
		cfg.pa = first_loop->share_model_master->model_mem.pa;
		cfg.va = first_loop->share_model_master->model_mem.va;
		cfg.size = first_loop->share_model_master->model_mem.size;
	}
	p_cfg = &cfg;

	if(p_model_cfg->model_unset){
		memset(&cfg, 0, sizeof(cfg));
	}else if(p_model_cfg->model_zero_size){
		cfg.size = 0;
	}else if(p_model_cfg->model_null){
		p_cfg = NULL;
	}else if(p_model_cfg->model_zero_addr){
		cfg.pa = 0;
		cfg.va = 0;
	}

	ret = vendor_ai_net_set(first_loop->net_path, VENDOR_AI_NET_PARAM_CFG_MODEL, p_cfg);
	if(ret){
		printf("set model fail\n");
		return -1;
	}

	if(p_model_cfg->invalid)
		return 0;

	if (strlen(p_model_cfg->diff_filename) > 0) {
		if ((p_global_data->multi_process) && (p_process_data->process_id > 0)) {
			printf("process_id(%d) get pool(%d)\n", p_process_data->process_id, first_loop->mem_seq*2+1);
			if(sequential)
				ret = ai_test_mem_get(&first_loop->diff_model_mem, p_model_cfg->diff_binsize, 1);
			else
				ret = ai_test_mem_get(&first_loop->diff_model_mem, p_model_cfg->diff_binsize, first_loop->mem_seq*2+1);
		} else {
			if(sequential)
				ret = ai_test_mem_get(&first_loop->diff_model_mem, p_model_cfg->diff_binsize, 1);
			else
				ret = ai_test_mem_get(&first_loop->diff_model_mem, p_model_cfg->diff_binsize, first_loop->net_path*2+1);
		}
		if (ret != HD_OK) {
			printf("get mem fail=%d\n", ret);
			return HD_ERR_FAIL;
		}

		//load file
		loadsize = _load_model(p_model_cfg->diff_filename, first_loop->diff_model_mem.va);
		if (loadsize <= 0) {
			printf("diff model load fail: %s\r\n", p_model_cfg->diff_filename);
			return -1;
		}

		cfg.pa = first_loop->diff_model_mem.pa;
		cfg.va = first_loop->diff_model_mem.va;
		cfg.size = first_loop->diff_model_mem.size;
		ret = vendor_ai_net_set(first_loop->net_path, VENDOR_AI_NET_PARAM_CFG_MODEL_RESINFO, &cfg);
		if(ret){
			printf("set diff model fail\n");
			return -1;
		}
	}

	return 0;
}

static HD_RESULT ai_test_network_alloc_work_buf(FIRST_LOOP_DATA *first_loop, MEM_PARM *work_mem)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_NET_CFG_WORKBUF wbuf = {0};
	NET_PATH_ID net_path;

	if(!first_loop || !work_mem){
		printf("first_loop(%p) or work_mem(%p) is null\n", first_loop, work_mem);
		return -1;
	}

	net_path = first_loop->net_path;

	ret = vendor_ai_net_get(net_path, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("net_path(%lu) get VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", net_path);
		return HD_ERR_FAIL;
	}
	//printf("alloc_io_buf: work buf, pa = %#lx, va = %#lx, size = %lu\r\n", wbuf.pa, wbuf.va, wbuf.size);
	if (wbuf.size == 0)
		return 0;

	ret = ai_test_mem_alloc(work_mem, "ai_io_buf", wbuf.size);
	if (ret != HD_OK) {
		printf("net_path(%lu) alloc ai_io_buf of %d bytes fail\r\n", net_path, wbuf.size);
		return HD_ERR_FAIL;
	}

	if(first_loop->null_work_buf){
		wbuf.pa = 0;
		wbuf.va = 0;
	}else if(first_loop->non_align_work_buf){
		wbuf.pa = work_mem->pa + 16; // simulate non-32x align
		wbuf.va = work_mem->va + 16; // simulate non-32x align
	}else{
		wbuf.pa = work_mem->pa;
		wbuf.va = work_mem->va;
	}
	wbuf.size = work_mem->size;
	ret = vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("net_path(%lu) set VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", net_path);
		return HD_ERR_FAIL;
	}
	printf("alloc_io_buf: work mem, pa = %#lx, va = %#lx, size = %lu\r\n", work_mem->pa, work_mem->va, work_mem->size);

	return ret;
}

static HD_RESULT ai_test_network_alloc_rbuf(FIRST_LOOP_DATA *first_loop, MEM_PARM *rbuf_mem)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_NET_CFG_RONLYBUF rbuf = {0};
	NET_PATH_ID net_path;

	if(!first_loop || !rbuf_mem){
		printf("first_loop(%p) or rbuf_mem(%p) is null\n", first_loop, rbuf_mem);
		return -1;
	}

	net_path = first_loop->net_path;

	ret = vendor_ai_net_get(net_path, VENDOR_AI_NET_PARAM_CFG_RONLYBUF, &rbuf);
	if (ret != HD_OK) {
		printf("net_path(%lu) get VENDOR_AI_NET_PARAM_CFG_RONLYBUF fail\r\n", net_path);
		return HD_ERR_FAIL;
	}
	// printf("ai_rbuf_buf: rbuf mem, pa = %#lx, va = %#lx, size = %lu\r\n", rbuf.pa, rbuf.va, rbuf.size);
	if (rbuf.size == 0)
		return 0;

	ret = ai_test_mem_alloc(rbuf_mem, "ai_rbuf_buf", rbuf.size);
	if (ret != HD_OK) {
		printf("net_path(%lu) alloc ai_rbuf of %d bytes fail\r\n", net_path, rbuf.size);
		return HD_ERR_FAIL;
	}

	
	rbuf.pa = (UINT32)rbuf_mem->pa;
	rbuf.va = (UINT32)rbuf_mem->va;

	rbuf.size = rbuf_mem->size;
	ret = vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_CFG_RONLYBUF, &rbuf);
	if (ret != HD_OK) {
		printf("net_path(%lu) set VENDOR_AI_NET_PARAM_CFG_RONLYBUF fail\r\n", net_path);
		return HD_ERR_FAIL;
	}
	printf("alloc_ronly_buf: rbuf mem, pa = %#lx, va = %#lx, size = %lu\r\n", rbuf_mem->pa, rbuf_mem->va, rbuf_mem->size);

	return ret;
}

HD_RESULT ai_test_network_open(NET_PATH_ID net_path, VENDOR_AI_NET_INFO *net_info)
{
	HD_RESULT ret = HD_OK;
	HD_RESULT ret2 = HD_OK;

	if(!net_info){
		printf("net_info is null\n");
		return -1;
	}

	// open
	ret = vendor_ai_net_open(net_path);
	if (HD_OK != ret) {
		printf("net_path(%lu) open fail !!\n", net_path);
		return ret;
	}

	ret2 = vendor_ai_net_get(net_path, VENDOR_AI_NET_PARAM_INFO, net_info);
	if (HD_OK != ret2) {
		printf("net_path(%lu) get info fail !!\n", net_path);
	} else {
		//printf("net_path(%lu) in buf cnt=%d, type=%04x; out buf cnt=%d, type=%04x\n",
		//	net_path,
		//	(int)net_info->in_buf_cnt,
		//	(unsigned int)net_info->in_buf_type,
		//	(int)net_info->out_buf_cnt,
		//	(unsigned int)net_info->out_buf_type);
	}

	return ret;
}

HD_RESULT ai_test_network_dump_postproc_buf(NET_PATH_ID net_path, CHAR *out_class_labels, void *p_out)
{
	HD_RESULT ret = HD_OK;
#if 0
	VENDOR_AI_POSTPROC_RESULT_INFO *p_rslt_info = (VENDOR_AI_POSTPROC_RESULT_INFO *)(p_out);
	UINT32 i, j;

	printf("net_path(%lu) classification results:\r\n", net_path);
	if (p_rslt_info) {
		for (i = 0; i < p_rslt_info->result_num; i++) {
			VENDOR_AI_POSTPROC_RESULT *p_rslt = &p_rslt_info->p_result[i];
			for (j = 0; j < NN_POSTPROC_TOP_N; j++) {
				printf("%ld. no=%ld, label=%s, score=%f\r\n",
					j + 1,
					p_rslt->no[j],
					&(out_class_labels[p_rslt->no[j] * VENDOR_AIS_LBL_LEN]),
					p_rslt->score[j]);
			}
		}
	}
#endif
	return ret;
}

HD_RESULT ai_test_network_dump_detout_buf(NET_PATH_ID net_path, void *p_out)
{
	HD_RESULT ret = HD_OK;
#if 0
	VENDOR_AI_DETOUT_RESULT_INFO *p_rslt_info = (VENDOR_AI_DETOUT_RESULT_INFO *)(p_out);
	VENDOR_AI_DETOUT_RESULT *p_rslt;
	UINT32 i;

	printf("net_path(%lu) detection results:\r\n", net_path);
	if (p_rslt_info) {
		for (i = 0; i < p_rslt_info->result_num; i++) {
			p_rslt = &p_rslt_info->p_result[i];

			// coordinate transform by src_img
			p_rslt->x = p_rslt->x * 1.0f;
			p_rslt->y = p_rslt->y * 1.0f;
			p_rslt->w = p_rslt->w * 1.0f;
			p_rslt->h = p_rslt->h * 1.0f;

			printf("%ld: no=%ld score=%f x,y,w,h=(%f, %f, %f, %f)\r\n",
				i + 1,
				p_rslt->no[0],
				p_rslt->score[0],
				p_rslt->x,
				p_rslt->y,
				p_rslt->w,
				p_rslt->h);
		}
	}
#endif

	return ret;
}


///////////////////////////////////////////////////////////////////////////////

static int compare_with_golden_file(void *out_buf, UINT32 size, GOLDEN_SAMPLE_RESULT *golden_sample, char *filename, int reload)
{
	FILE *fd;
	UINT32 file_size = 0;

	fd = fopen(filename, "rb");
	if (!fd) {
		printf("get bin(%s) size fail\n", filename);
		return (-1);
	}

	fseek(fd, 0, SEEK_END);
	file_size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	fclose(fd);

	if(file_size != size){
		printf("file(%s) size(%u) != output size(%u)\n", filename, (int)file_size, (int)size);
		return -1;
	}

	if(reload && golden_sample->data && size != golden_sample->size){
		free(golden_sample->data);
		golden_sample->size = 0;
		golden_sample->data = malloc(size);
		if(!golden_sample->data){
			printf("fail to allocate bytes %d for golden sample\n", (int)size);
			return -1;
		}
		golden_sample->size = size;
	}

	fd = fopen(filename, "rb");
	if (!fd) {
		printf("get bin(%s) size fail\n", filename);
		return (-1);
	}
	file_size = fread (golden_sample->data, 1, size, fd);
	fclose(fd);
	if (size != file_size) {
		printf("size mismatch, real = %u, idea = %u\r\n", (int)file_size, (int)size);
		return -1;
	}

	if(memcmp(out_buf, golden_sample->data, size)){
		printf("output != golden sample\n");
		return -1;
	}

	return 0;
}

static int compare_with_golden_file_dynamic_batch(void *out_buf, UINT32 size, GOLDEN_SAMPLE_RESULT *golden_sample, char *filename, int reload)
{
	FILE *fd;
	UINT32 file_size = 0;
	VENDOR_AI_BUF* buf = out_buf;
	fd = fopen(filename, "rb");
	if (!fd) {
		printf("get bin(%s) size fail\n", filename);
		return (-1);
	}

	fseek(fd, 0, SEEK_END);
	file_size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	fclose(fd);
	
	if(reload && golden_sample->data && size != golden_sample->size){
		free(golden_sample->data);
		golden_sample->size = 0;
		golden_sample->data = malloc(file_size);
		if(!golden_sample->data){
			printf("fail to allocate bytes %d for golden sample\n", (int)size);
			return -1;
		}
		golden_sample->size = file_size;
	}else{
		golden_sample->data = malloc(file_size);
		golden_sample->size = file_size;
	}

	fd = fopen(filename, "rb");
	if (!fd) {
		printf("get bin(%s) size fail\n", filename);
		return (-1);
	}
	file_size = fread (golden_sample->data, 1, file_size, fd);
	fclose(fd);


	if(memcmp((unsigned char*)buf->va, golden_sample->data, size)){
		printf("output != golden sample\n");
		return -1;
	}

	if (golden_sample->data) {
		free(golden_sample->data);
		golden_sample->data = 0;
	}
	return 0;
}


#if 0

static int compare_with_golden_file(void *out_buf, UINT32 size, GOLDEN_SAMPLE_RESULT *golden_sample, char *filename, int reload)
{
	FILE *fd;
	UINT32 file_size = 0;
	VENDOR_AI_BUF* buf = out_buf;
	unsigned char* buf_va = (unsigned char*)buf->va;
	fd = fopen(filename, "rb");
	if (!fd) {
		printf("get bin(%s) size fail\n", filename);
		return (-1);
	}

	fseek ( fd, 0, SEEK_END );
	file_size = ALIGN_CEIL_4( ftell(fd) );
	fseek ( fd, 0, SEEK_SET );

	golden_sample->data = malloc(size);

	file_size = fread ((void*)golden_sample->data, 1, file_size, fd);
	printf("size mismatch, real = %u, idea = %u\r\n", (int)file_size, (int)size);
	/*if (size != file_size) {
		return -1;
		printf("size mismatch, real = %u, idea = %u\r\n", (int)file_size, (int)size);
	}

	for(UINT32 j = 0; j<1000; ++j){
		printf("buf_va[%d]=%d \n",j,buf_va[j]);
	}

	for(UINT32 j = 0; j<1000; ++j){
		printf("golden_sample->data[%d]=%d \n",j,golden_sample->data[j]);
	}*/

	if(memcmp(buf_va, golden_sample->data, size)){
		printf("output != golden sample\n");
		return -1;
	}
	free(golden_sample->data);

	fclose(fd);
	return 0;
}
#endif


#if 0
static int compare_with_previous_output(void *out_buf, UINT32 size, GOLDEN_SAMPLE_RESULT *golden_sample)
{
	if(golden_sample->size == 0){
		golden_sample->data = malloc(size);
		if(!golden_sample->data){
			printf("fail to allocate %d bytes for golden sample\n", size);
			return -1;
		}
		golden_sample->size = size;
		memcpy(golden_sample->data, out_buf, size);
	}else{
		if(size != golden_sample->size || memcmp(out_buf, golden_sample->data, size)){
			printf("output != golden sample\n");
			return -1;
		}
	}

	return 0;
}
#endif

static INT32 get_random_scale(VENDOR_AI_BUF *src_img, NET_MODEL_CONFIG *model_cfg)
{
	UINT32 random_tmp_w = 0;
	UINT32 random_tmp_h = 0;
	int min_value_w = 0;
	int min_value_h = 0;

	INT32 total_res = 0;

	INT32* width_arr = 0;
	INT32* height_arr = 0;
	INT32 count = 0;

	INT32 i;

	srand(time(0));

	if (model_cfg->diff_res_random_times == 0) {
		model_cfg->diff_res_random_times = 1;
	}

	if (model_cfg->diff_res_loop) {
		total_res = 2 * model_cfg->diff_res_num * 2;
	} else {
		total_res = model_cfg->diff_res_num * (1 + model_cfg->diff_res_random_times);
	}

	model_cfg->diff_total_num = total_res;

	if (model_cfg->diff_res_w == 0) {
		model_cfg->diff_res_w = malloc(sizeof(INT32)*total_res);
		memset(model_cfg->diff_res_w, 0, sizeof(INT32)*total_res);
	}

	if (model_cfg->diff_res_h == 0) {
		model_cfg->diff_res_h = malloc(sizeof(INT32)*total_res);
		memset(model_cfg->diff_res_h, 0, sizeof(INT32)*total_res);
	}

	if (model_cfg->diff_golden_sample == 0) {
		model_cfg->diff_golden_sample = malloc(sizeof(GOLDEN_SAMPLE_RESULT)*model_cfg->diff_res_num*2);
		memset(model_cfg->diff_golden_sample, 0, sizeof(GOLDEN_SAMPLE_RESULT)*model_cfg->diff_res_num*2);

		//printf("allocate %d gold sample\r\n", model_cfg->diff_res_num*2);
	}


	width_arr = model_cfg->diff_res_w;
	height_arr = model_cfg->diff_res_h;

	for (i = 0; i < model_cfg->diff_res_num*2; i++) {

		if (i < model_cfg->diff_res_num) {
			random_tmp_w = model_cfg->diff_width[i];
			random_tmp_h = model_cfg->diff_height[i];

			/*if (random_tmp_w <= (src_img->width)/16) {
				random_tmp_w = (src_img->width)/16+1;
			}
			if (random_tmp_h <= (src_img->height)/16) {
				random_tmp_h = (src_img->height)/16+1;
			}

			random_tmp_w = random_tmp_w % 2 ? (random_tmp_w + 1) : random_tmp_w;
			random_tmp_h = random_tmp_h % 2 ? (random_tmp_h + 1) : random_tmp_h;*/

			width_arr[i]  = random_tmp_w;
			height_arr[i] = random_tmp_h;

			if (model_cfg->diff_res_loop) {
				width_arr[total_res - i - 1]  = random_tmp_w;
				height_arr[total_res - i - 1] = random_tmp_h;
			}

			count++;
		} else if (model_cfg->diff_res_loop) {
			int value_w_next = 0;
			int value_h_next = 0;

			if (i == model_cfg->diff_res_num) {
				min_value_w = 0;
				min_value_h = 0;

				value_w_next = model_cfg->diff_width[i-model_cfg->diff_res_num];
				value_h_next = model_cfg->diff_height[i-model_cfg->diff_res_num];
			} else {
				min_value_w = model_cfg->diff_width[i-model_cfg->diff_res_num - 1];
				min_value_h = model_cfg->diff_height[i-model_cfg->diff_res_num - 1];

				value_w_next = model_cfg->diff_width[i-model_cfg->diff_res_num];
				value_h_next = model_cfg->diff_height[i-model_cfg->diff_res_num];
			}

			if (value_w_next == min_value_w) {
				random_tmp_w = value_w_next;
			} else {
				random_tmp_w = rand() % (value_w_next - min_value_w) + min_value_w;
			}
			if (value_h_next == min_value_h) {
				random_tmp_h = value_h_next;
			} else {
				random_tmp_h = rand() % (value_h_next - min_value_h) + min_value_h;
			}

			if (random_tmp_w <= (src_img->width)/16) {
				random_tmp_w = (src_img->width)/16+1;
			}
			if (random_tmp_h <= (src_img->height)/16) {
				random_tmp_h = (src_img->height)/16+1;
			}

			random_tmp_w = random_tmp_w % 2 ? (random_tmp_w + 1) : random_tmp_w;
			random_tmp_h = random_tmp_h % 2 ? (random_tmp_h + 1) : random_tmp_h;

			width_arr[i]  = random_tmp_w;
			height_arr[i] = random_tmp_h;

			width_arr[total_res - i - 1]  = random_tmp_w;
			height_arr[total_res - i - 1] = random_tmp_h;

			//printf("min w = %d, min h = %d, r w = %d, r h = %d\r\n", min_value_w, min_value_h, random_tmp_w, random_tmp_h);
		} else {
			int value_w_next = 0;
			int value_h_next = 0;
			INT32 j;

			if (i == model_cfg->diff_res_num) {
				min_value_w = 0;
				min_value_h = 0;

				value_w_next = model_cfg->diff_width[i-model_cfg->diff_res_num];
				value_h_next = model_cfg->diff_height[i-model_cfg->diff_res_num];
			} else {
				min_value_w = model_cfg->diff_width[i-model_cfg->diff_res_num - 1];
				min_value_h = model_cfg->diff_height[i-model_cfg->diff_res_num - 1];

				value_w_next = model_cfg->diff_width[i-model_cfg->diff_res_num];
				value_h_next = model_cfg->diff_height[i-model_cfg->diff_res_num];
			}

			for (j = 0; j < model_cfg->diff_res_random_times; j++) {
				if (value_w_next == min_value_w) {
					random_tmp_w = value_w_next;
				} else {
					random_tmp_w = rand() % (value_w_next - min_value_w) + min_value_w;
				}
				if (value_h_next == min_value_h) {
					random_tmp_h = value_h_next;
				} else {
					random_tmp_h = rand() % (value_h_next - min_value_h) + min_value_h;
				}

				if (random_tmp_w <= (src_img->width)/16) {
					random_tmp_w = (src_img->width)/16+1;
				}
				if (random_tmp_h <= (src_img->height)/16) {
					random_tmp_h = (src_img->height)/16+1;
				}

				random_tmp_w = random_tmp_w % 2 ? (random_tmp_w + 1) : random_tmp_w;
				random_tmp_h = random_tmp_h % 2 ? (random_tmp_h + 1) : random_tmp_h;

				//printf("i=%d, j=%d, count=%d, min w = %d, min h = %d, r w = %d, r h = %d\r\n", i, j, count, min_value_w, min_value_h, random_tmp_w, random_tmp_h);

				width_arr[count]  = random_tmp_w;
				height_arr[count] = random_tmp_h;

				count++;
			}
			//printf("min w = %d, min h = %d, r w = %d, r h = %d\r\n", min_value_w, min_value_h, random_tmp_w, random_tmp_h);
		}
	}

	return 0;
}

static INT32 get_random_dim(VENDOR_AI_BUF *src_img, NET_MODEL_CONFIG *model_cfg)
{
	UINT32 random_tmp_w = 0;
	UINT32 random_tmp_h = 0;
	int min_value_w = 0;
	int min_value_h = 0;
	int max_value_w = 0;
	int max_value_h = 0;

	INT32 total_res = 0;

	INT32* width_arr = 0;
	INT32* height_arr = 0;

	INT32 i;

	UINT32 random_idx = 0;

	srand(time(0));

	total_res = model_cfg->multi_scale_random_num;

	model_cfg->diff_total_num = total_res;

	if (model_cfg->diff_res_w == 0) {
		model_cfg->diff_res_w = malloc(sizeof(INT32)*total_res);
		memset(model_cfg->diff_res_w, 0, sizeof(INT32)*total_res);
	}

	if (model_cfg->diff_res_h == 0) {
		model_cfg->diff_res_h = malloc(sizeof(INT32)*total_res);
		memset(model_cfg->diff_res_h, 0, sizeof(INT32)*total_res);
	}

	width_arr = model_cfg->diff_res_w;
	height_arr = model_cfg->diff_res_h;

	for (i = 0; i < total_res; i++) {
		random_idx = rand() % (model_cfg->diff_res_num + 1);

		if (random_idx == (UINT32)(model_cfg->diff_res_num)) {
			random_idx -= 1;

			min_value_w = model_cfg->diff_width[random_idx];
			min_value_h = model_cfg->diff_height[random_idx];
			max_value_w = model_cfg->diff_max_w;
			max_value_h = model_cfg->diff_max_h;

		} else if (random_idx == 0) {

			min_value_w = 1;
			min_value_h = 1;
			max_value_w = model_cfg->diff_width[random_idx];
			max_value_h = model_cfg->diff_height[random_idx];
		} else {
			random_idx -= 1;

			min_value_w = model_cfg->diff_width[random_idx];
			min_value_h = model_cfg->diff_height[random_idx];
			max_value_w = model_cfg->diff_width[random_idx+1];
			max_value_h = model_cfg->diff_height[random_idx+1];
		}

		//printf("idx = %d, min_value_w = %d, min_value_h = %d, max_value_w = %d, max_value_h = %d\r\n", random_idx, min_value_w, min_value_h, max_value_w, max_value_h);

		if (i == 0 || i == (total_res - 1)) {
			random_tmp_w = model_cfg->diff_max_w;
			random_tmp_h = model_cfg->diff_max_h;
		} else if (i%3 == 0) {
			random_tmp_w = model_cfg->diff_width[random_idx];
			random_tmp_h = model_cfg->diff_height[random_idx];
		} else {

			if (max_value_w == min_value_w) {
				random_tmp_w = max_value_w;
			} else {
				random_tmp_w = rand() % (max_value_w - min_value_w) + min_value_w;
			}

			if (max_value_h == min_value_h) {
				random_tmp_h = max_value_h;
			} else {
				random_tmp_h = rand() % (max_value_h - min_value_h) + min_value_h;
			}

			random_tmp_w = random_tmp_w % 2 ? (random_tmp_w + 1) : random_tmp_w;
			random_tmp_h = random_tmp_h % 2 ? (random_tmp_h + 1) : random_tmp_h;
		}

		width_arr[i]  = random_tmp_w;
		height_arr[i] = random_tmp_h;
		//printf("r w = %d, r h = %d\r\n", random_tmp_w, random_tmp_h);
	}

	return 0;
}

static INT32 get_random_batch(VENDOR_AI_BUF *src_img, NET_MODEL_CONFIG *model_cfg)
{

	INT32 total_res = 0;
	INT32* batch_arr = 0;
	INT32 i;
	UINT32 random_idx = 0;

	srand(time(0));

	total_res = model_cfg->dynamic_batch_random_num;

	
	//printf("model_cfg->dyB_batch=%d \n",model_cfg->dyB_batch);

	if (model_cfg->dyB_batch_random== 0) {
		model_cfg->dyB_batch_random = malloc(sizeof(INT32)*total_res);
		memset(model_cfg->dyB_batch_random, 0, sizeof(INT32)*total_res);
	}

	batch_arr = model_cfg->dyB_batch_random;

	for (i = 0; i < total_res; i++) {		
		random_idx = rand() % (model_cfg->dyB_res_num+ 1);
		//printf("random_idx = %d\n",random_idx);
		
		if (random_idx == (UINT32)(model_cfg->dyB_res_num)) {
			random_idx -= 1;
			batch_arr[i] = model_cfg->dyB_batch[random_idx];
		} else if (random_idx == 0) {
			batch_arr[i] = model_cfg->dyB_batch[random_idx];
		} else {
			random_idx -= 1;
			batch_arr[i] = model_cfg->dyB_batch[random_idx];
		}
		printf("idx[%d] = %d \r\n", i, batch_arr[i]);
	}	
	model_cfg->dyB_res_num= total_res;

	model_cfg->dyB_batch = model_cfg->dyB_batch_random;
	

	return 0;
}


#if 0
HD_RESULT yuv_input_scale(HD_GFX_IMG_BUF *imgin, HD_GFX_IMG_BUF *imgout, HD_GFX_SCALE_QUALITY method)
{
	HD_RESULT ret;
	HD_GFX_SCALE param = {0};
	UINT32 i;

	param.src_img.dim.w            = imgin->dim.w;
	param.src_img.dim.h            = imgin->dim.h;
	param.src_img.format           = imgin->format;
	param.src_img.p_phy_addr[0]    = imgin->p_phy_addr[0];
	param.src_img.p_phy_addr[1]    = imgin->p_phy_addr[1];
	param.src_img.p_phy_addr[2]    = imgin->p_phy_addr[2];
	param.src_img.lineoffset[0]    = imgin->lineoffset[0];
	param.src_img.lineoffset[1]    = imgin->lineoffset[1];
	param.src_img.lineoffset[2]    = imgin->lineoffset[2];
	param.src_img.ddr_id           = imgin->ddr_id;

	param.dst_img.dim.w            = imgout->dim.w;
	param.dst_img.dim.h            = imgout->dim.h;
	param.dst_img.format           = imgout->format;
	param.dst_img.p_phy_addr[0]    = imgout->p_phy_addr[0];
	param.dst_img.p_phy_addr[1]    = imgout->p_phy_addr[1];
	param.dst_img.p_phy_addr[2]    = imgout->p_phy_addr[2];
	param.dst_img.lineoffset[0]    = imgout->lineoffset[0];
	param.dst_img.lineoffset[1]    = imgout->lineoffset[1];
	param.dst_img.lineoffset[2]    = imgout->lineoffset[2];
	param.dst_img.ddr_id           = imgout->ddr_id;

	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = imgin->dim.w;
	param.src_region.h             = imgin->dim.h;

	param.dst_region.x             = 0;
	param.dst_region.y             = 0;
	param.dst_region.w             = imgout->dim.w;
	param.dst_region.h             = imgout->dim.h;

	param.quality				   = method;

	if (imgin->format == HD_VIDEO_PXLFMT_RGB888_PLANAR) {
		for (i = 0; i < 3; i++) {
			param.src_img.format        = HD_VIDEO_PXLFMT_Y8;
			param.src_img.p_phy_addr[0] = imgin->p_phy_addr[i];
			param.dst_img.format        = HD_VIDEO_PXLFMT_Y8;
			param.dst_img.p_phy_addr[0] = imgout->p_phy_addr[i];

			ret = hd_gfx_scale(&param);
			if (HD_OK != ret) {
			    printf("hd_gfx_scale fail\r\n");
			}
		}
	} else {
		ret = hd_gfx_scale(&param);
		if (HD_OK != ret) {
			printf("hd_gfx_scale fail\r\n");
		}
	}
	return ret;
}

static VOID random_scale(VENDOR_AI_BUF *p_img, UINT32 ii, VENDOR_AI_BUF *test_scale_rgb, NET_MODEL_CONFIG *model_cfg)
{

	HD_GFX_IMG_BUF input_image;
	HD_GFX_IMG_BUF input_scale_image;
	UINT32 random_tmp_w = 0;
	UINT32 random_tmp_h = 0;

	#if 0
	int min_value_w = 0;
	int min_value_h = 0;
	srand(time(0));
	#endif

	input_image.dim.w = p_img->width;
	input_image.dim.h = p_img->height;
	input_image.format = HD_VIDEO_PXLFMT_RGB888_PLANAR;
	input_image.p_phy_addr[0] = p_img->pa;
	input_image.p_phy_addr[1] = input_image.p_phy_addr[0] + p_img->width*p_img->height;
	input_image.p_phy_addr[2] = input_image.p_phy_addr[1] + p_img->width*p_img->height;
	input_image.lineoffset[0] = ALIGN_CEIL_4(input_image.dim.w);
	input_image.lineoffset[1] = ALIGN_CEIL_4(input_image.dim.w);
	input_image.lineoffset[2] = ALIGN_CEIL_4(input_image.dim.w);
	input_image.ddr_id = p_img->ddr_id;

#if 0
	if (ii < model_cfg->diff_res_num) {
		random_tmp_w = model_cfg->diff_width[ii];
		random_tmp_h = model_cfg->diff_height[ii];
	} else {
		int value_w_next = 0;
		int value_h_next = 0;

		if (ii == model_cfg->diff_res_num) {
			min_value_w = 0;
			min_value_h = 0;

			value_w_next = model_cfg->diff_width[ii-model_cfg->diff_res_num];
			value_h_next = model_cfg->diff_height[ii-model_cfg->diff_res_num];
		} else {
			min_value_w = model_cfg->diff_width[ii-model_cfg->diff_res_num - 1];
			min_value_h = model_cfg->diff_height[ii-model_cfg->diff_res_num - 1];

			value_w_next = model_cfg->diff_width[ii-model_cfg->diff_res_num];
			value_h_next = model_cfg->diff_height[ii-model_cfg->diff_res_num];
		}

		if (value_w_next == min_value_w) {
			random_tmp_w = value_w_next;
		} else {
			random_tmp_w = rand() % (value_w_next - min_value_w) + min_value_w;
		}
		if (value_h_next == min_value_h) {
			random_tmp_h = value_h_next;
		} else {
			random_tmp_h = rand() % (value_h_next - min_value_h) + min_value_h;
		}


		//printf("min w = %d, min h = %d, r w = %d, r h = %d\r\n", min_value_w, min_value_h, random_tmp_w, random_tmp_h);
	}


	if (random_tmp_w <= (input_image.dim.w)/16) {
		random_tmp_w = (input_image.dim.w)/16+1;
	}
	if (random_tmp_h <= (input_image.dim.h)/16) {
		random_tmp_h = (input_image.dim.h)/16+1;
	}


	random_tmp_w = random_tmp_w % 2 ? (random_tmp_w + 1) : random_tmp_w;
	random_tmp_h = random_tmp_h % 2 ? (random_tmp_h + 1) : random_tmp_h;

	if (1) {
		printf("width random:%d\r\n", random_tmp_w);
		printf("height random:%d\r\n", random_tmp_h);
	}
#else

	random_tmp_w = model_cfg->diff_res_w[ii];
	random_tmp_h = model_cfg->diff_res_h[ii];

	if (0) {
		printf("width random:%d\r\n", random_tmp_w);
		printf("height random:%d\r\n", random_tmp_h);
	}


#endif
	input_scale_image.dim.w = random_tmp_w;
	input_scale_image.dim.h = random_tmp_h;
	input_scale_image.format = HD_VIDEO_PXLFMT_Y8;
	input_scale_image.p_phy_addr[0] = test_scale_rgb->pa;
	//input_scale_image.p_phy_addr[1] = input_scale_image.p_phy_addr[0] + ALIGN_CEIL_4(random_tmp_w)*random_tmp_h;
	//input_scale_image.p_phy_addr[2] = input_scale_image.p_phy_addr[1] + ALIGN_CEIL_4(random_tmp_w)*random_tmp_h;
	input_scale_image.lineoffset[0] = ALIGN_CEIL_4(input_scale_image.dim.w);
	//input_scale_image.lineoffset[1] = ALIGN_CEIL_4(input_scale_image.dim.w);
	//input_scale_image.lineoffset[2] = ALIGN_CEIL_4(input_scale_image.dim.w);
	input_scale_image.ddr_id = test_scale_rgb->ddr_id;

	yuv_input_scale(&input_image,&input_scale_image,HD_GFX_SCALE_QUALITY_BILINEAR);

	test_scale_rgb->width = input_scale_image.dim.w;
	test_scale_rgb->height = input_scale_image.dim.h;
	test_scale_rgb->channel = p_img->channel;
	test_scale_rgb->line_ofs = input_scale_image.dim.w;
	test_scale_rgb->fmt = HD_VIDEO_PXLFMT_Y8;
	test_scale_rgb->sign = MAKEFOURCC('A','B','U','F');
	test_scale_rgb->size = test_scale_rgb->line_ofs * test_scale_rgb->height * 3 / 2;
}

static HD_RESULT ai_test_input_scale_open(MEM_PARM *input_mem, MEM_PARM *scale_mem, VENDOR_AI_BUF *src_img, VENDOR_AI_BUF *scale_img, INT32 ii, NET_MODEL_CONFIG *model_cfg)
{
	HD_RESULT ret = HD_OK;

	if(!input_mem || !src_img || !scale_mem || !scale_img){
		printf("input_mem(%p) or src_img(%p) or scale_img(%p) is null\n", input_mem, src_img, scale_img);
		return -1;
	}

	if (model_cfg->diff_res_loop) {
		if (ii >= model_cfg->diff_res_num*2*2 + 1) {
			return HD_ERR_NOT_ALLOW;
		}
	} else {
		if (model_cfg->diff_res_random_times == 0) {
			model_cfg->diff_res_random_times = 1;
		}
		if (ii >= model_cfg->diff_res_num* (1 + model_cfg->diff_res_random_times) + 1) {
			return HD_ERR_NOT_ALLOW;
		}
	}

	scale_mem->size = input_mem->size;

	ret = ai_test_mem_alloc(scale_mem, "ai_scale_buf", scale_mem->size);
	if (ret != HD_OK) {
		printf("alloc ai_scale_buf fail\r\n");
		return HD_ERR_FAIL;
	}

	if (model_cfg->diff_res_w == 0) {
		get_random_scale(src_img, model_cfg);
	}

	scale_img->pa   = (uintptr_t)scale_mem->pa;
	scale_img->va   = (uintptr_t)scale_mem->va;
	scale_img->ddr_id = scale_mem->ddr_id;

	random_scale(src_img, ii, scale_img, model_cfg);

	hd_common_mem_flush_cache((void *)scale_img->va, scale_img->size);

	return ret;
}
#endif

static HD_RESULT ai_test_input_scale_open(MEM_PARM *input_mem, MEM_PARM *scale_mem, VENDOR_AI_BUF *src_img, VENDOR_AI_BUF *scale_img, INT32 ii, HD_DIM *dim)
{
	HD_RESULT ret = HD_OK;

	if(!input_mem || !src_img || !scale_mem || !scale_img){
		printf("input_mem(%p) or src_img(%p) or scale_img(%p) is null\n", input_mem, src_img, scale_img);
		return -1;
	}


	//scale_mem->size = input_mem->size;

	ret = ai_test_mem_alloc(scale_mem, "ai_scale_buf", scale_mem->size);
	if (ret != HD_OK) {
		printf("alloc ai_scale_buf fail\r\n");
		return HD_ERR_FAIL;
	}

	scale_img->pa   = scale_mem->pa;
	scale_img->va   = scale_mem->va;
	scale_img->ddr_id = scale_mem->ddr_id;
	scale_img->size = scale_mem->size;
	scale_img->width = dim->w;
	scale_img->height = dim->h;
	scale_img->channel = src_img->channel;
	scale_img->line_ofs = dim->w;
	scale_img->fmt = src_img->fmt ;
	scale_img->sign = MAKEFOURCC('A','B','U','F');

	ai_test_crop((char *)src_img->va, src_img->width, src_img->height, src_img->line_ofs, (char *)scale_img->va, scale_img->width, scale_img->height);

	return ret;
}

#if 0
static void debug_dumpmem(ULONG addr, ULONG length)
{
	ULONG   offs;
	UINT32  str_len;
	UINT32  cnt;
	CHAR    str_dumpmem[64];
	UINT32  u32array[4];
	UINT32  *p_u32;
	CHAR    *p_char;

	addr = ALIGN_FLOOR_4(addr); //align to 4 bytes (UINT32)

	p_u32 = (UINT32 *)addr;

	printf("dump va=%08lx, addr=%08lx length=%08lx to console:\r\n", (ULONG)p_u32, addr, length);

	for (offs = 0; offs < length; offs += sizeof(u32array)) {
		u32array[0] = *p_u32++;
		u32array[1] = *p_u32++;
		u32array[2] = *p_u32++;
		u32array[3] = *p_u32++;

		str_len = snprintf(str_dumpmem, sizeof(str_dumpmem), "%08lX : %08X %08X %08X %08X  ",
			(addr + offs), (UINT)u32array[0], (UINT)u32array[1], (UINT)u32array[2], (UINT)u32array[3]);

		p_char = (char *)&u32array[0];
		for (cnt = 0; cnt < sizeof(u32array); cnt++, p_char++) {
			if (*p_char < 0x20 || *p_char >= 0x80)
				str_len += snprintf(str_dumpmem+str_len, 64-str_len, ".");
			else
				str_len += snprintf(str_dumpmem+str_len, 64-str_len, "%c", *p_char);
		}

		printf("%s\r\n", str_dumpmem);
	}
	printf("\r\n\r\n");
}
#endif

static UINT32 result_compare(UINT32 va, UINT32 gtsize, UINT8* data, UINT32 size)
{
	UINT32 idx;
	UINT8 *gt = (UINT8 *)va;
	if (gtsize != size) {
		printf("result_compare():gtsize(%d) != size(%d)\n", (int)gtsize, (int)size);
		return -1;
	}
	for (idx = 0; idx < size; idx++) {
		if (((*(gt++)) - (*(data++))) != 0) {
			return -1;
		}
	}

	return 0;
}

static HD_RESULT ai_test_network_veritfy_postproc_buf(void *p_out, VENDOR_AI_POSTPROC_RESULT_INFO* rslt_postproc_info)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_POSTPROC_RESULT_INFO *p_rslt_info = (VENDOR_AI_POSTPROC_RESULT_INFO *)(p_out);

	UINT32 result_size;

	if (p_rslt_info) {
		result_size = sizeof(VENDOR_AI_POSTPROC_RESULT)*p_rslt_info->result_num;
	} else {
		return HD_ERR_NG;
	}

	if (rslt_postproc_info->p_result == 0) {
		rslt_postproc_info->p_result = malloc(result_size);

		if (rslt_postproc_info->p_result != 0) {
			rslt_postproc_info->sign       = p_rslt_info->sign;
			rslt_postproc_info->chunk_size = p_rslt_info->chunk_size;
			rslt_postproc_info->result_num = p_rslt_info->result_num;

			memcpy(rslt_postproc_info->p_result, p_rslt_info->p_result, result_size);

			//printf("save 1st\r\n");
		} else {
			printf("1st out is null\r\n");
			return HD_ERR_NG;
		}

		return ret;
	}

	if (rslt_postproc_info->sign != p_rslt_info->sign) {
		printf("sign compare failed\r\n");
		return HD_ERR_NG;
	}

	if (rslt_postproc_info->chunk_size != p_rslt_info->chunk_size) {
		printf("chunk_size compare failed\r\n");
		return HD_ERR_NG;
	}

	if (rslt_postproc_info->result_num != p_rslt_info->result_num) {
		printf("result_num compare failed\r\n");
		return HD_ERR_NG;
	}

	if (result_compare((UINT32)rslt_postproc_info->p_result, result_size, (UINT8*)p_rslt_info->p_result, result_size) != 0) {
		printf("result compare failed\r\n");
		return HD_ERR_NG;
	}

	//printf("verify OK\r\n");

	return ret;
}

static HD_RESULT ai_test_network_veritfy_detout_buf(void *p_out, VENDOR_AI_DETOUT_RESULT_INFO* rslt_detout_info)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_DETOUT_RESULT_INFO *p_rslt_info = (VENDOR_AI_DETOUT_RESULT_INFO *)(p_out);

	UINT32 result_size;

	if (p_rslt_info) {
		result_size = sizeof(VENDOR_AI_DETOUT_RESULT)*p_rslt_info->result_num;
	} else {
		printf("compare failed\r\n");
		return HD_ERR_NG;
	}

	if (rslt_detout_info->p_result == 0) {
		rslt_detout_info->p_result = malloc(result_size);

		if (rslt_detout_info->p_result != 0) {
			rslt_detout_info->sign       = p_rslt_info->sign;
			rslt_detout_info->chunk_size = p_rslt_info->chunk_size;
			rslt_detout_info->result_num = p_rslt_info->result_num;

			memcpy(rslt_detout_info->p_result, p_rslt_info->p_result, result_size);

			//printf("save 1st\r\n");
		} else {
			printf("1st out is null\r\n");
			return HD_ERR_NG;
		}

		return ret;
	}

	if (rslt_detout_info->sign != p_rslt_info->sign) {
		printf("sign compare failed\r\n");
		return HD_ERR_NG;
	}

	if (rslt_detout_info->chunk_size != p_rslt_info->chunk_size) {
		printf("chunk_size compare failed\r\n");
		return HD_ERR_NG;
	}

	if (rslt_detout_info->result_num != p_rslt_info->result_num) {
		printf("result_num compare failed\r\n");
		return HD_ERR_NG;
	}

	if (result_compare((UINT32)rslt_detout_info->p_result, result_size, (UINT8*)p_rslt_info->p_result, result_size) != 0) {
		printf("result compare failed\r\n");
		return HD_ERR_NG;
	}

	//printf("verify OK\r\n");

	return ret;
}

static HD_RESULT ai_test_network_veritfy_ai_outbuf(VENDOR_AI_BUF *p_out, VENDOR_AI_BUF *p_rslt_buf, CHAR *dump_filename, UINT32 valid_size)
{
	HD_RESULT ret = HD_OK;
	UINT32 result_size;
	GLOBAL_DATA* global_data = ai_test_get_global_data();
	UINT32 result_buf_size = 0;
	UINT32 out_size = 0;

	//printf("ai outbuf:\r\n");

	if (p_out && p_rslt_buf) {
		result_size = VENDOR_AI_NET_OUTBUF_SIZE(p_out->width, p_out->height, p_out->channel, p_out->batch_num, p_out->fmt);
		if(result_size > p_out->size){
			printf("calculated output size(%d) > AI_BUF.size(%d)\n", result_size, p_out->size);
			return -1;
		}
	} else {
		printf("ai_test_network_veritfy_ai_outbuf() input is null\r\n");
		return HD_ERR_NG;
	}

	if(!p_out->va || !result_size){
		printf("ai outbuf pa(%x) or size(%d) is null\n", (int)p_out->va, (int)result_size);
		return -1;
	}

	hd_common_mem_flush_cache((void *)p_out->va, p_out->size);

	if (p_rslt_buf->va == 0) {
		p_rslt_buf->va = (UINT32)malloc(result_size);

		if (p_rslt_buf->va != 0) {
			p_rslt_buf->sign       = p_out->sign;
			p_rslt_buf->chunk_size = p_out->chunk_size;
			p_rslt_buf->ddr_id     = p_out->ddr_id;
			p_rslt_buf->size       = p_out->size;
			p_rslt_buf->fmt        = p_out->fmt;
			p_rslt_buf->width      = p_out->width;
			p_rslt_buf->height     = p_out->height;
			p_rslt_buf->channel    = p_out->channel;
			p_rslt_buf->batch_num  = p_out->batch_num;
			p_rslt_buf->time       = p_out->time;
			p_rslt_buf->reserve    = p_out->reserve;
			p_rslt_buf->line_ofs   = p_out->line_ofs;
			p_rslt_buf->channel_ofs = p_out->channel_ofs;
			p_rslt_buf->batch_ofs   = p_out->batch_ofs;
			p_rslt_buf->time_ofs    = p_out->time_ofs;
			p_rslt_buf->layout[0]    = p_out->layout[0];
			p_rslt_buf->layout[1]    = p_out->layout[1];
			p_rslt_buf->layout[2]    = p_out->layout[2];
			p_rslt_buf->layout[3]    = p_out->layout[3];
			p_rslt_buf->layout[4]    = p_out->layout[4];
			p_rslt_buf->layout[5]    = p_out->layout[5];
			p_rslt_buf->layout[6]    = p_out->layout[6];
			p_rslt_buf->layout[7]    = p_out->layout[7];

			if (global_data->load_output && dump_filename) {
				FILE *pFile = 0;
				INT32 size = 0;
				INT32 read_size = 0;

				//printf("==== load name = %s\r\n", dump_filename);

				pFile = fopen(dump_filename,"rb");

			    if(!pFile) {
			        printf("Error opening file.");
			        return HD_ERR_NG;
			    }

				fseek(pFile, 0, SEEK_END);
				size = ftell(pFile);
				fseek(pFile, 0, SEEK_SET);

				if (size < 0) {
					printf("getting %s size failed\r\n", dump_filename);
					read_size = -1;
				} else if ((INT32)result_size < size) {
					printf("buffer size(%ld) < file size(%ld)\r\n", p_out->size, size);
					read_size = -1;
				} else {
					read_size = (INT32)fread((VOID *)p_rslt_buf->va, 1, size, pFile);
				}

				if(hd_common_mem_flush_cache((VOID *)p_rslt_buf->va, read_size) != HD_OK) {
			        printf("flush cache failed.\r\n");
			    }

				fclose(pFile);
			} else {
				memcpy((void*)p_rslt_buf->va, (const void*)p_out->va, result_size);

				if (global_data->dump_output && dump_filename) {
					FILE *pFile = 0;

					//printf("dump name = %s\r\n", dump_filename);

					pFile = fopen(dump_filename,"wb");

				    if(!pFile) {
				        printf("Error opening file.");
				        return HD_ERR_NG;
				    }
				    fwrite((char*)p_out->va, sizeof(char), p_out->size, pFile);
					fclose(pFile);
				}
				return ret;
			}

			//printf("save 1st\r\n");
		} else {
			printf("1st out is null\r\n");
			return HD_ERR_NG;
		}
	}

	if (p_rslt_buf->sign != p_out->sign) {
		printf("compare sign failed\r\n");
		return HD_ERR_NG;
	}

	if (p_rslt_buf->chunk_size != p_out->chunk_size) {
		printf("compare chunk_size failed\r\n");
		return HD_ERR_NG;
	}

	if (p_rslt_buf->ddr_id != p_out->ddr_id) {
		printf("compare ddr_id failed\r\n");
		return HD_ERR_NG;
	}

	if (p_rslt_buf->size != p_out->size) {
		printf("compare size failed\r\n");
		return HD_ERR_NG;
	}

	if (p_rslt_buf->fmt != p_out->fmt) {
		printf("compare fmt failed\r\n");
		return HD_ERR_NG;
	}

	if (p_rslt_buf->width != p_out->width) {
		printf("compare width failed\r\n");
		return HD_ERR_NG;
	}

	if (p_rslt_buf->height != p_out->height) {
		printf("compare height failed\r\n");
		return HD_ERR_NG;
	}

	if (p_rslt_buf->channel != p_out->channel) {
		printf("compare channel failed\r\n");
		return HD_ERR_NG;
	}

	if (p_rslt_buf->batch_num != p_out->batch_num) {
		printf("compare batch_num failed\r\n");
		return HD_ERR_NG;
	}

	if (p_rslt_buf->time != p_out->time) {
		printf("compare time failed\r\n");
		return HD_ERR_NG;
	}

	if (p_rslt_buf->reserve != p_out->reserve) {
		printf("compare reserve failed\r\n");
		return HD_ERR_NG;
	}

	if (p_rslt_buf->line_ofs != p_out->line_ofs) {
		printf("compare line_ofs failed\r\n");
		return HD_ERR_NG;
	}

	if (p_rslt_buf->channel_ofs != p_out->channel_ofs) {
		printf("compare channel_ofs failed\r\n");
		return HD_ERR_NG;
	}

	if (p_rslt_buf->batch_ofs != p_out->batch_ofs) {
		printf("compare batch_ofs failed\r\n");
		return HD_ERR_NG;
	}

	if (p_rslt_buf->time_ofs != p_out->time_ofs) {
		printf("compare time_ofs failed\r\n");
		return HD_ERR_NG;
	}

	if (result_compare((UINT32)p_rslt_buf->layout, 8, (UINT8*)p_out->layout, 8) != 0) {
		printf("compare layout failed\r\n");
		return HD_ERR_NG;
	}

	if (valid_size != 0) {
		result_buf_size = valid_size;
		out_size = valid_size;
	} else {
		result_buf_size = VENDOR_AI_NET_OUTBUF_SIZE(p_out->width, p_out->height, p_out->channel, p_out->batch_num, p_out->fmt);
		out_size = VENDOR_AI_NET_OUTBUF_SIZE(p_out->width, p_out->height, p_out->channel, p_out->batch_num, p_out->fmt);
		if(result_buf_size > p_rslt_buf->size){
			printf("calculated output size(%d) > backup.size(%d)\n", result_buf_size, p_rslt_buf->size);
			return -1;
		}
		if(out_size > p_out->size){
			printf("calculated output size(%d) > AI_BUF.size(%d)\n", out_size, p_out->size);
			return -1;
		}
	}

	if (result_compare((UINT32)p_rslt_buf->va, result_buf_size, (UINT8*)p_out->va, out_size) != 0) {
		printf("compare result buffer failed\r\n");
		return HD_ERR_NG;
	}

	//printf("verify OK\r\n");

	return ret;
}

static int verify_result(UINT32 type, void *out_buf, UINT32 size, NET_IN_CONFIG *in_cfg, FOURTH_LOOP_DATA *fourth_loop_data, int reload, int idx, int out_cfg_count, CHAR *dump_filename, UINT32 valid_size)
{
	GOLDEN_SAMPLE_RESULT *golden_sample;

	if(!in_cfg || !fourth_loop_data || !out_buf || !size){
		printf("in_cfg(%p) or fourth_loop_data(%p) or out_buf(%p) or size(%u)is null\n",
					in_cfg, fourth_loop_data, out_buf, (int)size);
		return -1;
	}

	golden_sample = &(fourth_loop_data->golden_sample);
	if(!golden_sample){
		printf("golden_sample is null\n");
		return -1;
	}

	if(in_cfg->verify_result == 0)
		return 0;
	else if(strlen(in_cfg->golden_sample_filename)){
		if(compare_with_golden_file(out_buf, size, golden_sample, in_cfg->golden_sample_filename, reload)){
			printf("output != previous output\n");
			return -1;
		}
	}else{
		#if 0
		if(compare_with_previous_output(out_buf, size, golden_sample)){
			printf("output != previous output\n");
			return -1;
		}
		#else
		if (type == MAKEFOURCC('A','C','0','1')) { //classify 01
			if(ai_test_network_veritfy_postproc_buf(out_buf, &fourth_loop_data->golden_sample.rslt_postproc_info)){
				printf("fail to verify postproc buf\n");
				return -1;
			}
		} else if (type == MAKEFOURCC('A','D','0','1')) { //detection 01
			if(ai_test_network_veritfy_detout_buf(out_buf, &fourth_loop_data->golden_sample.rslt_detout_info)){
				printf("fail to verify detout buf\n");
				return -1;
			}
		} else {
			if(fourth_loop_data->golden_sample.rslt_outbuf == NULL){
				fourth_loop_data->golden_sample.rslt_outbuf = malloc(out_cfg_count * sizeof(VENDOR_AI_BUF));
				if(fourth_loop_data->golden_sample.rslt_outbuf == NULL){
					printf("fail to allocate %d GOLDEN_SAMPLE_RESULT\n", out_cfg_count);
					fourth_loop_data->golden_sample.rslt_outbuf_num = 0;
					return -1;
				}else{
					fourth_loop_data->golden_sample.rslt_outbuf_num = out_cfg_count;
					memset(fourth_loop_data->golden_sample.rslt_outbuf, 0, out_cfg_count * sizeof(VENDOR_AI_BUF));
				}
			}
			if(idx >=  (int)fourth_loop_data->golden_sample.rslt_outbuf_num){
				printf("idx(%d) >= rslt_outbuf_num(%d)\n", idx, fourth_loop_data->golden_sample.rslt_outbuf_num);
				return -1;
			}
			if(ai_test_network_veritfy_ai_outbuf(out_buf, &fourth_loop_data->golden_sample.rslt_outbuf[idx], dump_filename, valid_size)){
				printf("fail to verify ai outbuf\n");
				return -1;
			}
		}
		#endif
//printf("compare ok\n");
	}

	return 0;
}

static int verify_result_diff(UINT32 type, void *out_buf, NET_MODEL_CONFIG *model_cfg, int out_idx, int out_cfg_count, int res_idx, CHAR *dump_filename, UINT32 valid_size)
{
	if(!model_cfg || !out_buf){
		printf("model_cfg(%p) or out_buf(%p) is null\n", model_cfg, out_buf);
		return -1;
	}

	#if 0
	if (type == MAKEFOURCC('A','C','0','1')) { //classify 01
		if(ai_test_network_veritfy_postproc_buf(out_buf, &fourth_loop_data->golden_sample.rslt_postproc_info)){
			printf("fail to verify postproc buf\n");
			return -1;
		}
	} else if (type == MAKEFOURCC('A','D','0','1')) { //detection 01
		if(ai_test_network_veritfy_detout_buf(out_buf, &fourth_loop_data->golden_sample.rslt_detout_info)){
			printf("fail to verify detout buf\n");
			return -1;
		}
	} else
	#endif
	{
		int idx;

		if (res_idx >= model_cfg->diff_res_num*2) {
			idx = model_cfg->diff_res_num*2*2 - res_idx - 1;
		} else {
			idx = res_idx;
		}

		//printf("res idx = %d, out idx = %d, compare idx = %d\r\n", res_idx, out_idx, idx);

		if(model_cfg->diff_golden_sample[idx].rslt_outbuf == NULL){
			model_cfg->diff_golden_sample[idx].rslt_outbuf= malloc(out_cfg_count * sizeof(VENDOR_AI_BUF));
			if(model_cfg->diff_golden_sample[idx].rslt_outbuf == NULL){
				printf("fail to allocate %d GOLDEN_SAMPLE_RESULT\n", out_cfg_count);
				model_cfg->diff_golden_sample[idx].rslt_outbuf_num = 0;
				return -1;
			}else{
				model_cfg->diff_golden_sample[idx].rslt_outbuf_num = out_cfg_count;
				memset(model_cfg->diff_golden_sample[idx].rslt_outbuf, 0, out_cfg_count * sizeof(VENDOR_AI_BUF));
			}
		}
		if(out_idx >= (int)model_cfg->diff_golden_sample[idx].rslt_outbuf_num){
			printf("idx(%d) >= rslt_outbuf_num(%d)\n", out_idx, model_cfg->diff_golden_sample[idx].rslt_outbuf_num);
			return -1;
		}


		if(ai_test_network_veritfy_ai_outbuf(out_buf, &model_cfg->diff_golden_sample[idx].rslt_outbuf[out_idx], dump_filename, valid_size)){
			printf("fail to verify ai outbuf\n");
			return -1;
		}
	}

	return 0;
}

static int verify_result_dynamic_batch(UINT32 type, void *out_buf, UINT32 size, NET_IN_CONFIG *in_cfg, FOURTH_LOOP_DATA *fourth_loop_data, int reload, int idx, int out_cfg_count, CHAR *dump_filename, UINT32 valid_size)
{
	GOLDEN_SAMPLE_RESULT *golden_sample;

	if(!in_cfg || !fourth_loop_data || !out_buf || !size){
		printf("in_cfg(%p) or fourth_loop_data(%p) or out_buf(%p) or size(%u)is null\n",
					in_cfg, fourth_loop_data, out_buf, (int)size);
		return -1;
	}
	

	golden_sample = &(fourth_loop_data->golden_sample);
	if(!golden_sample){
		printf("golden_sample is null\n");
		return -1;
	}
		
	if(in_cfg->verify_result == 0)
		return 0;
	else if(dump_filename){
		if(compare_with_golden_file_dynamic_batch(out_buf, size, golden_sample, dump_filename, reload)){
			printf("output != previous output\n");
			return -1;
		}
	}

	return 0;
}


static int ai_test_free_out_result(GOLDEN_SAMPLE_RESULT *golden_sample){
	UINT32 i;

	if (golden_sample == 0) {
		return 0;
	}

	if (golden_sample->rslt_postproc_info.p_result) {
		free(golden_sample->rslt_postproc_info.p_result);
		golden_sample->rslt_postproc_info.p_result = 0;
	}

	if (golden_sample->rslt_detout_info.p_result) {
		free(golden_sample->rslt_detout_info.p_result);
		golden_sample->rslt_detout_info.p_result = 0;
	}

	if(golden_sample->rslt_outbuf_num && golden_sample->rslt_outbuf){
		for (i = 0; i < golden_sample->rslt_outbuf_num; i++) {
			if (golden_sample->rslt_outbuf[i].va) {
				free((void*)golden_sample->rslt_outbuf[i].va);
				golden_sample->rslt_outbuf[i].va = 0;
			}
		}
		free(golden_sample->rslt_outbuf);
		golden_sample->rslt_outbuf_num = 0;
		golden_sample->rslt_outbuf = NULL;
	}

	return 0;
}

void bubble_sort(UINT32 arr[], int len) {
	int i, j;
	UINT32 temp;
	for (i = 0; i < len - 1; i++)
		for (j = 0; j < len - 1 - i; j++)
			if (arr[j] > arr[j + 1]) {
				temp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = temp;
			}
}

#if 1  //CPU usage
void* ai_test_cpu_loading(VOID *arg)
{
	CPU_LOADING_ANALYZE* cpu_analyze = (CPU_LOADING_ANALYZE*)arg;

	#if 0
	UINT64 start_t= hd_gettime_us();
	BOOL b_cpu_1 = FALSE, b_cpu_2 = FALSE;
	FILE *cpu_ptr;
	char cpu_str[20];
	UINT32 temp, cpu_time1, idle_time1, cpu_time2, idle_time2;

	cpu_time1 = 0;
	idle_time1 = 0;
	cpu_time2 = 0;
	idle_time2 = 0;

	while (1){
		// get cpu stat 1 second after  it start
		if ((b_cpu_1 == FALSE) && (hd_gettime_us() - start_t > (1000*1000))) {
			b_cpu_1 = TRUE;
			cpu_time1 = 0;
			idle_time1 = 0;

			cpu_ptr = fopen("/proc/stat", "r");

			fscanf(cpu_ptr, "%s ", cpu_str);

			for(int i = 0; i < 6; i++) {
				fscanf(cpu_ptr, "%lu ", &temp);

				if (i == 3) {
					idle_time1 += temp;
				} else {
					cpu_time1 += temp;
				}
			}
			fclose(cpu_ptr);
		}

		// get cpu stat 1 second before it end
		if ((b_cpu_2 == FALSE) && (hd_gettime_us() - start_t > (9000*1000))) {
			b_cpu_2 = TRUE;

			cpu_analyze->test_done = TRUE;

			cpu_ptr = fopen("/proc/stat", "r");

			fscanf(cpu_ptr, "%s ", cpu_str);

			for(int i = 0; i < 6; i++) {
			  fscanf(cpu_ptr, "%lu ", &temp);

				if (i == 3) {
					idle_time2 += temp;
				} else {
					cpu_time2 += temp;
				}
			}
			fclose(cpu_ptr);

			break;
		}

		// sleep 0.1 ms
		usleep(100);
	}

	cpu_analyze->usage = (float)(100 * (cpu_time2 - cpu_time1)) / (float)((cpu_time2-cpu_time1) + (idle_time2 - idle_time1));
	#else
	DOUBLE cpu_usr = 0.0;
	DOUBLE cpu_sys = 0.0;

	cal_linux_cpu_loading(5, &cpu_usr, &cpu_sys);
	cpu_analyze->usage = cpu_usr + cpu_sys;
	cpu_analyze->test_done = TRUE;
	//printf("net_cpu_loading(%f) usr(%f) sys(%f)\n", cpu_analyze->usage, cpu_usr, cpu_sys);
	#endif

	return 0;
}
#endif

static int check_proc_after_restart(NET_PATH_ID net_path){

	int ret = 0;

	ret = vendor_ai_net_stop(net_path);
	if (HD_OK != ret) {
		printf("net_path(%lu) stop fail !!\n", net_path);
		return -1;
	}

	ret = vendor_ai_net_start(net_path);
	if (HD_OK != ret) {
		printf("net_path(%lu) start fail !!\n", net_path);
		return -1;
	}

	ret = vendor_ai_net_proc(net_path);
	if (HD_OK == ret) {
		printf("net_path(%lu) proc failed to report error after restart without set in !!\n", net_path);
		return -1;
	}

	return 0;
}

static int check_proc_without_set_input(NET_PATH_ID net_path){

	int ret = 0;

	ret = vendor_ai_net_proc(net_path);
	if (HD_OK == ret) {
		printf("net_path(%lu) proc failed to report error without set in !!\n", net_path);
		return -1;
	}

	return 0;
}

static int ai_test_net_set_unaligned_start_address(VENDOR_AI_BUF *ai_buf, UINT32 buf_count)
{
	UINT32 fmt;
	int align = 1;

	fmt = ai_buf->fmt;
	switch (fmt) {
	case HD_VIDEO_PXLFMT_YUV420:
		{
			if (buf_count == 1) {
				align = 2;
				if ((ai_buf[0].pa) && !(ai_buf[0].pa % align)) {
					ai_buf[0].pa += 1;
				}
				if ((ai_buf[0].va) && !(ai_buf[0].va % align)) {
					ai_buf[0].va += 1;
				}
			}
		}
		break;
	case HD_VIDEO_PXLFMT_Y8:
		{
			if (buf_count == 2) {
				if (ai_buf[1].fmt == HD_VIDEO_PXLFMT_UV) {
					align = 2;
					if ((ai_buf[1].pa) && !(ai_buf[1].pa % align)) {
						ai_buf[1].pa += 1;
					}
					if ((ai_buf[1].va) && !(ai_buf[1].va % align)) {
						ai_buf[1].va += 1;
					}
				}
			} else {
				ai_buf[0].pa += 1;
				ai_buf[0].va += 1;
			}
		}
		break;
	#if 0
	case HD_VIDEO_PXLFMT_YUV422:
		{
			// set YUV422
			// 33x differ from 52x/56x
			align = 4;
			if ((ai_buf[0].pa) && !(ai_buf[0].pa % align)) {
				ai_buf[0].pa += 1;
			}
			if ((ai_buf[0].va) && !(ai_buf[0].va % align)) {
				ai_buf[0].va += 1;
			}
		}
		break;
	#endif
	case HD_VIDEO_PXLFMT_RGB888_PLANAR:
	case HD_VIDEO_PXLFMT_BGR888_PLANAR:
	case HD_VIDEO_PXLFMT_R8:
	case HD_VIDEO_PXLFMT_B8:
		{
			ai_buf[0].pa += 1;
			ai_buf[0].va += 1;
		}
		break;
	default: // feature-in
		break;
	}

	return 0;
}

static int ai_test_net_check_unaligned_start_address(VENDOR_AI_BUF *ai_buf, UINT32 buf_count, HD_RESULT ret)
{
	UINT32 fmt;
	int result = ret;

	fmt = ai_buf->fmt;
	switch (fmt) {
	case HD_VIDEO_PXLFMT_YUV420:
		{
			if (buf_count == 1) {
				result = ((ret) ? 0 : -1);
			}
		}
		break;
	case HD_VIDEO_PXLFMT_Y8:
		{
			//if (buf_count == 2) {
				//if (ai_buf[1].fmt == HD_VIDEO_PXLFMT_UV) {
					//result = ((ret) ? 0 : -1);
				//}
			//}
			result = ((ret) ? 0 : -1);    
		}
		break;
	#if 0
	case HD_VIDEO_PXLFMT_YUV422:
		{
			// check YUV422
			result = ((ret) ? 0 : -1);
		}
		break;
	#endif
	case HD_VIDEO_PXLFMT_RGB888_PLANAR:
	case HD_VIDEO_PXLFMT_BGR888_PLANAR:
	case HD_VIDEO_PXLFMT_R8:
	case HD_VIDEO_PXLFMT_B8:
		{
			result = ((ret) ? 0 : -1);
		}                    
	default: // feature-in
		break;
	}

	return result;
}

static HD_RESULT dump_outbuf(NET_PATH_ID net_path, NET_MODEL_CONFIG *model_cfg)
{
	GLOBAL_DATA* global_data = ai_test_get_global_data();
	extern HD_RESULT _vendor_ai_net_debug_layer(UINT32 proc_id, UINT32 layer_opt, CHAR *filedir);

	if(global_data == NULL || model_cfg == NULL){
		printf("global_data(%p) or model_cfg(%p) is NULL\n", global_data, model_cfg);
		return -1;
	}

	if(strlen(global_data->generate_golden_output) == 0){
		return HD_OK;
	}

	_vendor_ai_net_debug_layer(net_path, 1, global_data->generate_golden_output);

	return HD_OK;
}

static int fourth_loop(FIRST_LOOP_DATA *first_loop_data, NET_MODEL_CONFIG *model_cfg)
{
	HD_RESULT ret = HD_OK;
	HD_RESULT ret_scale = HD_OK;
	int i = 0, j, in_file_cfg_idx = 0, batch_idx = 0;
	UINT32 k, l;
	UINT32 buf_count = 1;
	VENDOR_AI_BUF	in_buf = {0}, *p_in_buf = NULL;
	int loop_count, in_cfg_count, out_cfg_count, in_file_count;
	NET_PATH_ID net_path;
	MEM_PARM *input_mem;
	MEM_PARM *scale_mem;
	VENDOR_AI_BUF *scale_img;
	VENDOR_AI_BUF *src_img;
	NET_IN_CONFIG *in_cfg = NULL;
	NET_IN_FILE_CONFIG *in_file_cfg;
	VENDOR_AI_NET_INFO *net_info;
	UINT32 total_in_batch = 0;
	HD_DIM dump_dim = {0};
	INT32* dyB_batch;
	INT32 dyB_batch_nu;
	UINT32 set_batch_num[16];

	UINT32 sum_time = 0;
	struct timeval tstart, tend;
	UINT32 time_arr[20] = { 0 };
	UINT32 t_i = 0;
	INT32 bw_start = 0;
	UINT64 bw_end = 0;
	UINT64 bw_result = 0;
	CHAR msg[512] = { 0 };
	pthread_t cal_cpu_loading_thread_id;
	CPU_LOADING_ANALYZE cpu_analyze = {0};
	//NET_OUT_CONFIG *out_cfg;
	CHAR model_filename[80] = {0};
	CHAR dump_filename[256] = {0};
	GLOBAL_DATA* global_data = ai_test_get_global_data();
	VOID *tmp_buf[16];

	if(!first_loop_data || !model_cfg){
		printf("first_loop_data(%p) or model_cfg(%p) is null\n", first_loop_data, model_cfg);
		return -1;
	}

	loop_count = first_loop_data->second_loop_data.third_loop_data.fourth_loop_data.loop_count;
	input_mem = &(first_loop_data->second_loop_data.third_loop_data.input_mem);
	scale_mem = &(first_loop_data->second_loop_data.third_loop_data.scale_mem);
	in_file_cfg = model_cfg->in_file_cfg;
	in_file_count = model_cfg->in_file_cfg_count;
	//out_cfg = model_cfg->out_cfg;
	src_img = &(first_loop_data->second_loop_data.third_loop_data.src_img);
	scale_img = &(first_loop_data->second_loop_data.third_loop_data.scale_img);
	net_path = first_loop_data->net_path;
	net_info = &(first_loop_data->net_info);
	in_cfg_count = net_info->in_buf_cnt;
	out_cfg_count = net_info->out_buf_cnt;

	// dynamic batch set
	if (model_cfg->dynamic_batch_random_num != 0){
		get_random_batch(src_img, model_cfg);
	}
	dyB_batch_nu = (model_cfg->dyB_res_num > 1 ? model_cfg->dyB_res_num : 1);
	dyB_batch = model_cfg->dyB_batch;
		for(batch_idx = 0; batch_idx < dyB_batch_nu; ++batch_idx){
		for(in_file_cfg_idx = 0 ; in_file_cfg_idx < in_file_count ; in_file_cfg_idx++){

		{
			total_in_batch = 0;

			for (UINT32 t = 0; t < in_file_cfg[in_file_cfg_idx].input_num; t++) {
				total_in_batch += in_file_cfg[in_file_cfg_idx].in_cfg[t].batch;
			}

			if ((int)total_in_batch != in_cfg_count) {
				printf("total_in_batch=%d, in_cfg_count=%d\r\n", (unsigned int)total_in_batch, (unsigned int)in_cfg_count);
				//continue;
			}

			ai_test_free_out_result(&first_loop_data->second_loop_data.third_loop_data.fourth_loop_data.golden_sample);
		}

		for(i = 0 ; i < loop_count ; ++i){

			//error handling check : proc without setting input
			if(in_file_cfg[in_file_cfg_idx].proc_without_set_input){
				if(check_proc_without_set_input(net_path)){
					ret = -1;
					printf("fail to detect proc without setting input\n");
					goto exit;
				}else{
					goto next_in_cfg;
				}
			}

			if (first_loop_data->perf_analyze && loop_count >= 27 && i < 27) {
				#if 1  //CPU usage
				if (i == 26 && cpu_analyze.test_start == FALSE) {
					if (first_loop_data->cpu_perf_analyze) {
						cpu_analyze.test_start = TRUE;
						cpu_analyze.test_done = FALSE;
						cpu_analyze.usage = 0;
						ret = pthread_create(&cal_cpu_loading_thread_id, NULL, ai_test_cpu_loading, (VOID*)(&cpu_analyze));
					} else {
						cpu_analyze.test_start = TRUE;
						cpu_analyze.test_done = TRUE;
						cpu_analyze.usage = 0;
					}
				}
				#endif
			}
//printf("in count %d\n", in_cfg_count);
			if (in_cfg_count > 1) {

				UINT32 *in_path_list = NULL;
				UINT32 idx = 0;

				for(j = 0; j < (int)in_file_cfg[in_file_cfg_idx].input_num; j++) {
					if(in_file_cfg[in_file_cfg_idx].in_cfg[j].input_mem.size){
						if (ai_test_input_close(&(in_file_cfg[in_file_cfg_idx].in_cfg[j].input_mem)) != HD_OK) {
							printf("input close error\n");
						}
						memset(&(in_file_cfg[in_file_cfg_idx].in_cfg[j].input_mem), 0, sizeof(MEM_PARM));
							}
						}

					if ((dyB_batch_nu > 0) && (dyB_batch != 0)) {
						FLOAT batch_ratio = 1;
							printf("dyB_batch[%d] = %d\n",batch_idx,dyB_batch[batch_idx]);
							ret = vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_BATCH_N_IMM, &dyB_batch[batch_idx]);
							if(ret) {
								ret = 0;
								printf("ignore net_path(%lu)set batch number OK\n", net_path);		
								goto next_in_cfg;
						}
						src_img->batch_num = dyB_batch[batch_idx];
						in_cfg = in_file_cfg[in_file_cfg_idx].in_cfg;
						for(j = 0; j < (int)in_file_cfg[in_file_cfg_idx].input_num; j++) {
							if(j == 0){
								set_batch_num[j] = dyB_batch[batch_idx];
								batch_ratio = (FLOAT)in_cfg[j].batch / dyB_batch[batch_idx];
							}else {
								set_batch_num[j] = (UINT32)(AI_TEST_ROUND((FLOAT)in_cfg[j].batch / batch_ratio));
							}
						}
				}

				in_path_list = (UINT32 *)malloc(sizeof(UINT32) * in_cfg_count);
				if(!in_path_list){
					printf("fail to allocate in_path_list\n");
					goto exit;
				}

				ret = vendor_ai_net_get(net_path, VENDOR_AI_NET_PARAM_IN_PATH_LIST, in_path_list);
				if(ret){
					printf("fail to get path list\n");
					free(in_path_list);
					return -1;
				}

				l = 0;
				in_cfg = in_file_cfg[in_file_cfg_idx].in_cfg;

				for(j = 0; j < (int)in_file_cfg[in_file_cfg_idx].input_num; j++) {
					UINT32 tmp_src_va = 0;
					UINT32 tmp_src_pa = 0;
					UINT32 src_img_size = 0;

					memset(&(in_cfg[j].input_mem), 0, sizeof(MEM_PARM));
					ret = ai_test_input_open(net_path, in_path_list[l], &(in_cfg[j]), &(in_cfg[j].input_mem), src_img);
					if (ret != HD_OK) {
						printf("in open error=%d\n", ret);
						free(in_path_list);
						return ret;
					}

					memcpy(&in_buf, src_img, sizeof(VENDOR_AI_BUF));
					p_in_buf = &in_buf;
					tmp_src_va = in_buf.va;
					tmp_src_pa = in_buf.pa;
					src_img_size = in_buf.size;
					src_img_size = ALIGN_CEIL_64(src_img_size);
                    
					if(in_file_cfg[in_file_cfg_idx].in_buf_unset){
						memset(p_in_buf, 0, sizeof(VENDOR_AI_BUF));
					}else if(in_file_cfg[in_file_cfg_idx].in_buf_zero_size){
						p_in_buf->size = 0;
					}else if(in_file_cfg[in_file_cfg_idx].in_buf_null){
						p_in_buf = NULL;
					}else if(in_file_cfg[in_file_cfg_idx].in_buf_zero_addr){
						p_in_buf->pa = 0;
						p_in_buf->va = 0;
					}else if(in_file_cfg[in_file_cfg_idx].in_buf_unaligned_addr){
						ai_test_net_set_unaligned_start_address(p_in_buf, buf_count);
					}

  					if (in_cfg[j].is_comb_img != 0) {						
						for(k =0 ; k< in_cfg[j].batch; k++) {
							tmp_buf[k] = (UINT32 *)malloc(in_buf.size);
							memcpy((VOID*)tmp_buf[k], (VOID*)in_buf.va+(k*in_buf.size), in_buf.size);
						}
                    }            
                    if ((dyB_batch_nu > 0) && (dyB_batch != 0)){
						for(k = 0; k < set_batch_num[j]; k++) {
							if (k > 0) {
								if (in_cfg[j].is_comb_img == 0) { // no combine image, need to use the same image as input.
									UINT32 dst_va = in_buf.va + src_img_size;
									memcpy((VOID*)dst_va, (VOID*)in_buf.va, src_img_size);
									hd_common_mem_flush_cache((VOID *)dst_va, src_img_size);
									in_buf.pa += src_img_size;
									in_buf.va += src_img_size;
								} else {
									in_buf.pa += src_img_size;
									in_buf.va += src_img_size;
								}
							}

							usleep(10000);
							// set input image

							//ret = vendor_ai_net_set(net_path, in_path_list[l], p_in_buf);
							//printf(" path_%d(0x%x) pa(0x%lx) va(0x%lx) size(%u) whcb(%d,%d,%d,%d)\n",
							//	idx, in_path_list[idx], p_in_buf->pa, p_in_buf->va, p_in_buf->size, p_in_buf->width, p_in_buf->height, p_in_buf->channel, p_in_buf->batch_num);		
							if ( model_cfg->in_path_num != 0){
									if (idx !=  model_cfg->in_path_num)
										ret = vendor_ai_net_set(net_path, in_path_list[idx], p_in_buf);
				            }else {
									ret = vendor_ai_net_set(net_path, in_path_list[idx], p_in_buf);
			      			}
							usleep(10000);
							if (in_file_cfg[in_file_cfg_idx].in_buf_unset ||
								in_file_cfg[in_file_cfg_idx].in_buf_zero_size ||
								in_file_cfg[in_file_cfg_idx].in_buf_null ||
								in_file_cfg[in_file_cfg_idx].in_buf_zero_addr ||
								in_cfg[j].invalid){
								if(ret) {
									ret = 0;
									free(in_path_list);
									goto exit;
								}
							}else if(in_file_cfg[in_file_cfg_idx].in_buf_unaligned_addr){
								if (ret) {
									if (0 != ai_test_net_check_unaligned_start_address(p_in_buf, buf_count, ret)) {
										free(in_path_list);
										printf("fail to detect unaligned addr buf\n");
										goto exit;
									} else {
										ret = 0;
										free(in_path_list);
										goto exit;
									}
								} else {
									if (0 != ai_test_net_check_unaligned_start_address(p_in_buf, buf_count, ret)) {
										ret = -1;
										free(in_path_list);
										printf("fail to detect unaligned addr buf\n");
										goto exit;
									}
								}
							}else {
								if(HD_OK != ret){
									free(in_path_list);
									if(first_loop_data->ignore_set_input_fail) {
										printf("ignore net_path(%lu)push input fail !!\n", net_path);
										ret = 0;
										goto next_in_cfg;
									}else{
										printf("net_path(%lu)push input fail !!\n", net_path);
										goto exit;
									}
								}
							}
							idx++;
							l++;
						}					
						idx = idx + in_cfg[j].batch - set_batch_num[j];
						in_buf.va = tmp_src_va; // reduction in_buf addr
						in_buf.pa = tmp_src_pa;
                    }else{
						for(k = 0; k < in_cfg[j].batch; k++) {
							if (k > 0) {
								if (in_cfg[j].is_comb_img == 0) { // no combine image, need to use the same image as input.
									UINT32 dst_va = in_buf.va + src_img_size;
									memcpy((VOID*)dst_va, (VOID*)in_buf.va, src_img_size);
									hd_common_mem_flush_cache((VOID *)dst_va, src_img_size);
									in_buf.pa += src_img_size;
									in_buf.va += src_img_size;
								} else {
									UINT32 dst_va = in_buf.va + src_img_size;
									memcpy((VOID*)dst_va, (VOID*)tmp_buf[k], in_buf.size);
									hd_common_mem_flush_cache((VOID *)dst_va, src_img_size);                             
									in_buf.pa += src_img_size;
									in_buf.va += src_img_size;
								}
							}

							usleep(10000);
							// set input image

							ret = vendor_ai_net_set(net_path, in_path_list[l], p_in_buf);
							usleep(10000);
							if (in_file_cfg[in_file_cfg_idx].in_buf_unset ||
								in_file_cfg[in_file_cfg_idx].in_buf_zero_size ||
								in_file_cfg[in_file_cfg_idx].in_buf_null ||
								in_file_cfg[in_file_cfg_idx].in_buf_zero_addr ||
								in_cfg[j].invalid){
								if(ret) {
									ret = 0;
									free(in_path_list);
									goto exit;
								}
							}else if(in_file_cfg[in_file_cfg_idx].in_buf_unaligned_addr){
								if (ret) {
									if (0 != ai_test_net_check_unaligned_start_address(p_in_buf, buf_count, ret)) {
										free(in_path_list);
										printf("fail to detect unaligned addr buf\n");
										goto exit;
									} else {
										ret = 0;
										free(in_path_list);
										goto exit;
									}
								} else {
									if (0 != ai_test_net_check_unaligned_start_address(p_in_buf, buf_count, ret)) {
										ret = -1;
										free(in_path_list);
										printf("fail to detect unaligned addr buf\n");
										goto exit;
									}
								}
							}else {
								if(HD_OK != ret){
									free(in_path_list);
									if(first_loop_data->ignore_set_input_fail) {
										printf("ignore net_path(%lu)push input fail !!\n", net_path);
										ret = 0;
										goto next_in_cfg;
									}else{
										printf("net_path(%lu)push input fail !!\n", net_path);
										goto exit;
									}
								}
							}
							l++;
						}
                    }
					if (in_cfg[j].is_comb_img != 0) {
						for(k =0 ; k< in_cfg[j].batch; k++) {
							if(tmp_buf[k] != NULL)
								free(tmp_buf[k]);
						}    
                    }                    
					in_buf.va = tmp_src_va;
					in_buf.pa = tmp_src_pa;
				}



				free(in_path_list);
			} else {
				if(i == 0){
					memset(input_mem, 0, sizeof(MEM_PARM));

					in_cfg = in_file_cfg[in_file_cfg_idx].in_cfg;

					ret = ai_test_input_open(net_path, VENDOR_AI_NET_PARAM_IN(0, 0), &in_cfg[0], input_mem, src_img);
					if (ret != HD_OK) {
						printf("in open error=%d\n", ret);
						return ret;
					}

					dump_dim.w = src_img->width;
					dump_dim.h = src_img->height;

					if (first_loop_data->diff_model_mem.pa && first_loop_data->ignore_set_res_dim_fail) {
						HD_DIM dim = {0};

						if (first_loop_data->multi_scale_set_imm == FALSE) {
							vendor_ai_net_stop(net_path);
						}

						if (in_file_cfg_idx < model_cfg->diff_res_num) {
							dim.w = model_cfg->diff_width[in_file_cfg_idx];
							dim.h = model_cfg->diff_height[in_file_cfg_idx];
						} else {
							dim.w = model_cfg->diff_width[0];
							dim.h = model_cfg->diff_height[0];
						}

						if (first_loop_data->multi_scale_set_imm == FALSE) {
							ret = vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_RES_DIM, &dim);
						} else {
							ret = vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_RES_DIM_IMM, &dim);
						}

						if(ret) {
							ret = 0;
							printf("ignore net_path(%lu)set RES_DIM OK\n", net_path);
							if (first_loop_data->multi_scale_set_imm == FALSE) {
								vendor_ai_net_start(net_path);
							}
							goto next_in_cfg;
						} else {
							printf("ignore net_path(%lu)set RES_DIM fail !!\n", net_path);
							ret = -1;
							if (first_loop_data->multi_scale_set_imm == FALSE) {
								vendor_ai_net_start(net_path);
							}
							goto exit;
						}

					} else if (first_loop_data->diff_model_mem.pa && model_cfg->diff_res_random) {
						HD_DIM dim = {0};

						if (model_cfg->diff_res_w == 0) {
							get_random_scale(src_img, model_cfg);
						}

						{//if (ret_scale == HD_OK) {

							if (first_loop_data->multi_scale_set_imm == FALSE) {
								vendor_ai_net_stop(net_path);
							}

							if (in_file_cfg_idx < model_cfg->diff_total_num) {
								dim.w = model_cfg->diff_res_w[in_file_cfg_idx];
								dim.h = model_cfg->diff_res_h[in_file_cfg_idx];
							} else {
								dim.w = model_cfg->diff_res_w[0];
								dim.h = model_cfg->diff_res_h[0];
							}

							src_img->width  = dim.w;
							src_img->height = dim.h;

							//printf("1 w = %d, h = %d\r\n", dim.w, dim.h);

							if (first_loop_data->multi_scale_set_imm == FALSE) {
								vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_RES_DIM, &dim);
								vendor_ai_net_start(net_path);
							} else {
								vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_RES_DIM_IMM, &dim);
							}

							ret_scale = HD_ERR_NOT_SUPPORT;

							dump_dim.w = dim.w;
							dump_dim.h = dim.h;
						}
					} else if (first_loop_data->diff_model_mem.pa && model_cfg->multi_scale_random_num) {
						HD_DIM dim = {0};

						if (model_cfg->diff_res_w == 0) {
							get_random_dim(src_img, model_cfg);
						}

						{//if (ret_scale == HD_OK) {

							if (first_loop_data->multi_scale_set_imm == FALSE) {
								vendor_ai_net_stop(net_path);
							}

							if (in_file_cfg_idx < model_cfg->diff_total_num) {
								dim.w = model_cfg->diff_res_w[in_file_cfg_idx];
								dim.h = model_cfg->diff_res_h[in_file_cfg_idx];
							} else {
								dim.w = model_cfg->diff_res_w[0];
								dim.h = model_cfg->diff_res_h[0];
							}

							scale_mem->size = _calc_ai_buf_size(dim.w, dim.h, in_cfg->c, in_cfg->bit/8, in_cfg->fmt);

							ret_scale = ai_test_input_scale_open(input_mem, scale_mem, src_img, scale_img, in_file_cfg_idx, &dim);

							//printf("1 w = %d, h = %d\r\n", dim.w, dim.h);

							if (first_loop_data->multi_scale_set_imm == FALSE) {
								vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_RES_DIM, &dim);
								vendor_ai_net_start(net_path);
							} else {
								vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_RES_DIM_IMM, &dim);
							}

							dump_dim.w = dim.w;
							dump_dim.h = dim.h;
						}
					} else if (first_loop_data->diff_model_mem.pa && model_cfg->diff_res_padding) {
						HD_DIM dim = {0};

						if (first_loop_data->multi_scale_set_imm == FALSE) {
							vendor_ai_net_stop(net_path);
						}

						if (in_file_cfg_idx < model_cfg->diff_res_num) {
							dim.w = model_cfg->diff_width[(in_file_cfg_idx/model_cfg->diff_res_padding_num)*model_cfg->diff_res_padding_num];
							dim.h = model_cfg->diff_height[(in_file_cfg_idx/model_cfg->diff_res_padding_num)*model_cfg->diff_res_padding_num];
						} else {
							dim.w = model_cfg->diff_width[0];
							dim.h = model_cfg->diff_height[0];
						}

						scale_mem->size = _calc_ai_buf_size(dim.w, dim.h, in_cfg->c, in_cfg->bit/8, in_cfg->fmt);

						ret_scale = ai_test_input_scale_open(input_mem, scale_mem, src_img, scale_img, in_file_cfg_idx, &dim);

						if (ret_scale == HD_OK) {
							ai_test_padding((char *)scale_img->va, dim.w, dim.h, dim.w, model_cfg->diff_width[in_file_cfg_idx], model_cfg->diff_height[in_file_cfg_idx]);
						}

						//printf("2 src_w(%d), src_h(%d)\n", src_img->width, src_img->height);
						//printf("2 w = %d, h = %d\r\n", dim.w, dim.h);

						if (first_loop_data->multi_scale_set_imm == FALSE) {
							vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_RES_DIM, &dim);
							vendor_ai_net_start(net_path);
						} else {
							vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_RES_DIM_IMM, &dim);
						}

						dump_dim.w = model_cfg->diff_width[in_file_cfg_idx];
						dump_dim.h = model_cfg->diff_height[in_file_cfg_idx];
					} else if (first_loop_data->diff_model_mem.pa) {
						HD_DIM dim = {0};

						if (first_loop_data->multi_scale_set_imm == FALSE) {
							vendor_ai_net_stop(net_path);
						}
						if (in_file_cfg_idx < model_cfg->diff_res_num) {
							dim.w = model_cfg->diff_width[in_file_cfg_idx];
							dim.h = model_cfg->diff_height[in_file_cfg_idx];
						} else {
							dim.w = model_cfg->diff_width[0];
							dim.h = model_cfg->diff_height[0];
						}

						scale_mem->size = _calc_ai_buf_size(dim.w, dim.h, in_cfg->c, in_cfg->bit/8, in_cfg->fmt);

						ret_scale = ai_test_input_scale_open(input_mem, scale_mem, src_img, scale_img, in_file_cfg_idx, &dim);

						//printf("3 src_w(%d), src_h(%d)\n", src_img->width, src_img->height);
						//printf("3 w = %d, h = %d\r\n", dim.w, dim.h);

						if (first_loop_data->multi_scale_set_imm == FALSE) {
							vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_RES_DIM, &dim);
							vendor_ai_net_start(net_path);
						} else {
							vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_RES_DIM_IMM, &dim);
						}

						dump_dim.w = dim.w;
						dump_dim.h = dim.h;

						//ret_scale = HD_ERR_NOT_SUPPORT;
					}
				}

				if (first_loop_data->diff_model_mem.pa && ret_scale == HD_OK) {
					memcpy(&in_buf, scale_img, sizeof(VENDOR_AI_BUF));
					//printf("copy scale\r\n");
				} else {
					memcpy(&in_buf, src_img, sizeof(VENDOR_AI_BUF));
					//printf("copy src\r\n");
				}

				p_in_buf = &in_buf;
				if(in_file_cfg[in_file_cfg_idx].in_buf_unset){
					memset(p_in_buf, 0, sizeof(VENDOR_AI_BUF));
				}else if(in_file_cfg[in_file_cfg_idx].in_buf_zero_size){
					p_in_buf->size = 0;
				}else if(in_file_cfg[in_file_cfg_idx].in_buf_null){
					p_in_buf = NULL;
				}else if(in_file_cfg[in_file_cfg_idx].in_buf_zero_addr){
					p_in_buf->pa = 0;
					p_in_buf->va = 0;
				}else if(in_file_cfg[in_file_cfg_idx].in_buf_unaligned_addr){
					ai_test_net_set_unaligned_start_address(p_in_buf, buf_count);
				}

				ret = vendor_ai_net_set(net_path, VENDOR_AI_NET_PARAM_IN(0, 0), p_in_buf);
				if (in_file_cfg[in_file_cfg_idx].in_buf_unset ||
					in_file_cfg[in_file_cfg_idx].in_buf_zero_size ||
					in_file_cfg[in_file_cfg_idx].in_buf_null ||
					in_file_cfg[in_file_cfg_idx].in_buf_zero_addr ||
				    in_cfg[0].invalid){
					if(ret) {
						ret = 0;
						goto exit;
					}
				}else if(in_file_cfg[in_file_cfg_idx].in_buf_unaligned_addr){
					if (ret) {
						if (0 != ai_test_net_check_unaligned_start_address(p_in_buf, buf_count, ret)) {
							printf("fail to detect unaligned addr buf\n");
							goto exit;
						} else {
							ret = 0;
							goto exit;
						}
					} else {
						if (0 != ai_test_net_check_unaligned_start_address(p_in_buf, buf_count, ret)) {
							ret = -1;
							printf("fail to detect unaligned addr buf\n");
							goto exit;
						}
					}
				}else if(first_loop_data->diff_model_mem.pa && model_cfg->multi_scale_random_num) {
					if(ret) {
						printf("invalid net_path(%lu)push input\n", net_path);
						ret = 0;
						goto next_in_cfg;
					}
				}else {
					if(HD_OK != ret){
						if(first_loop_data->ignore_set_input_fail) {
							printf("ignore net_path(%lu)push input fail !!\n", net_path);
							ret = 0;
							goto next_in_cfg;
						}else{
							printf("net_path(%lu)push input fail !!\n", net_path);
							goto exit;
						}
					}
				}
			}

			if(in_file_cfg[in_file_cfg_idx].in_buf_unset){
				ret = -1;
				printf("fail to detect unset buf\n");
				goto exit;
			}else if(in_file_cfg[in_file_cfg_idx].in_buf_zero_size){
				ret = -1;
				printf("fail to detect zero size buf\n");
				goto exit;
			}else if(in_file_cfg[in_file_cfg_idx].in_buf_null){
				ret = -1;
				printf("fail to detect null buf\n");
				goto exit;
			}else if(in_file_cfg[in_file_cfg_idx].in_buf_zero_addr){
				ret = -1;
				printf("fail to detect zero addr buf\n");
				goto exit;
			}

			//error handling check : proc after restart should report fail
			if(in_file_cfg[in_file_cfg_idx].proc_after_restart){
				if(check_proc_after_restart(net_path)){
					ret = -1;
					printf("fail to detect proc after restart\n");
					goto exit;
				}else{
					goto next_in_cfg;
				}
			}

			if (first_loop_data->perf_analyze && loop_count >= 27 && i < 27) {
				if (i < 20) {
					int len;

					gettimeofday(&tstart, NULL);
					ret = vendor_ai_net_proc(net_path);
					gettimeofday(&tend, NULL);

					time_arr[i] = (tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec);

					//printf("[%d] proc = %lu\r\n", i, time_arr[i]);

					if ((i+1) == 20) {
						len = (int) sizeof(time_arr) / sizeof(*time_arr);
						bubble_sort(time_arr, len);

						for (t_i = 5; t_i < 15; t_i++) {
							sum_time = sum_time + time_arr[t_i];
						}

						//printf("proc time = %lu us\r\n", sum_time);
					}
				} else if (i == 20){

					if (first_loop_data->net_job_opt.schd_parm == 0 && (first_loop_data->net_job_opt.method == VENDOR_AI_NET_JOB_OPT_LINEAR || first_loop_data->net_job_opt.method == VENDOR_AI_NET_JOB_OPT_LINEAR_O1)) {
						bw_start = vendor_mau_ch_mon_start(1, 2, 0); //CNN1
						ret = vendor_ai_net_proc(net_path);
						bw_end = vendor_mau_ch_mon_stop(1, 0);

						//printf("bw_start_cnn1=%d, bw_end_cnn1=%llu, %llu\r\n",  bw_start, bw_end, bw_end - bw_start);

						bw_result += bw_end - bw_start;
					} else {
						ret = vendor_ai_net_proc(net_path);
					}
				} else if (i == 21){

					if (first_loop_data->net_job_opt.schd_parm == 1 && (first_loop_data->net_job_opt.method == VENDOR_AI_NET_JOB_OPT_LINEAR || first_loop_data->net_job_opt.method == VENDOR_AI_NET_JOB_OPT_LINEAR_O1)) {
						bw_start = vendor_mau_ch_mon_start(2, 2, 0); //CNN2
						ret = vendor_ai_net_proc(net_path);
						bw_end = vendor_mau_ch_mon_stop(2, 0);

						//printf("bw_start_cnn2=%d, bw_end_cnn2=%llu, %llu\r\n",  bw_start, bw_end, bw_end - bw_start);

						bw_result += bw_end - bw_start;
					} else {
						ret = vendor_ai_net_proc(net_path);
					}
				} else if (i == 22) {

					if (first_loop_data->net_job_opt.schd_parm != VENDOR_AI_FAIR_CORE_ALL && (first_loop_data->net_job_opt.method == VENDOR_AI_NET_JOB_OPT_LINEAR || first_loop_data->net_job_opt.method == VENDOR_AI_NET_JOB_OPT_LINEAR_O1)) {
						bw_start = vendor_mau_ch_mon_start(3, 2, 0);  //NUE
						ret = vendor_ai_net_proc(net_path);
						bw_end = vendor_mau_ch_mon_stop(3, 0);

						//printf("bw_start_nue=%d, bw_end_nue=%llu, %llu\r\n",  bw_start, bw_end, bw_end - bw_start);

						bw_result += bw_end - bw_start;
					} else {
						ret = vendor_ai_net_proc(net_path);
					}
				} else if (i == 23){

					if (first_loop_data->net_job_opt.schd_parm == 0 && (first_loop_data->net_job_opt.method == VENDOR_AI_NET_JOB_OPT_LINEAR || first_loop_data->net_job_opt.method == VENDOR_AI_NET_JOB_OPT_LINEAR_O1)) {
						bw_start = vendor_mau_ch_mon_start(1, 2, 1); //CNN1
						ret = vendor_ai_net_proc(net_path);
						bw_end = vendor_mau_ch_mon_stop(1, 1);

						//printf("bw_start_cnn1=%d, bw_end_cnn1=%llu, %llu\r\n",  bw_start, bw_end, bw_end - bw_start);

						bw_result += bw_end - bw_start;
					} else {
						ret = vendor_ai_net_proc(net_path);
					}
				} else if (i == 24){

					if (first_loop_data->net_job_opt.schd_parm == 1 && (first_loop_data->net_job_opt.method == VENDOR_AI_NET_JOB_OPT_LINEAR || first_loop_data->net_job_opt.method == VENDOR_AI_NET_JOB_OPT_LINEAR_O1)) {
						bw_start = vendor_mau_ch_mon_start(2, 2, 1); //CNN2
						ret = vendor_ai_net_proc(net_path);
						bw_end = vendor_mau_ch_mon_stop(2, 1);

						//printf("bw_start_cnn2=%d, bw_end_cnn2=%llu, %llu\r\n",  bw_start, bw_end, bw_end - bw_start);

						bw_result += bw_end - bw_start;
					} else {
						ret = vendor_ai_net_proc(net_path);
					}
				} else if (i == 25) {

					if (first_loop_data->net_job_opt.schd_parm != VENDOR_AI_FAIR_CORE_ALL && (first_loop_data->net_job_opt.method == VENDOR_AI_NET_JOB_OPT_LINEAR || first_loop_data->net_job_opt.method == VENDOR_AI_NET_JOB_OPT_LINEAR_O1)) {
						bw_start = vendor_mau_ch_mon_start(3, 2, 1);  //NUE
						ret = vendor_ai_net_proc(net_path);
						bw_end = vendor_mau_ch_mon_stop(3, 1);

						//printf("bw_start_nue=%d, bw_end_nue=%llu, %llu\r\n",  bw_start, bw_end, bw_end - bw_start);

						bw_result += bw_end - bw_start;
					} else {
						ret = vendor_ai_net_proc(net_path);
					}
				} else {

					ret = vendor_ai_net_proc(net_path);

					if (cpu_analyze.test_done) {
					//if (1) {
						CHAR file_name[128] = {0};
						UINT32 model_size= 0;
						UINT32 initbuf_size = 0;
						UINT32 workbuf_size = 0;
						UINT32 total_buf_size = 0;

						if (first_loop_data->cpu_perf_analyze) {
							pthread_join(cal_cpu_loading_thread_id, NULL);
						}

						_vendor_ai_net_debug_get_memsize(net_path, &model_size, &initbuf_size, &workbuf_size);
						total_buf_size = model_size + initbuf_size + workbuf_size;

						//printf("model=%d, init=%d, work=%d\r\n", model_size, initbuf_size, workbuf_size);

						if (global_data->cpu_stress[0].thread_id) {
							if (global_data->first_loop_count > 1) {
								snprintf(file_name, sizeof(file_name), "result_cpu_%ld.txt", first_loop_data->net_path);
							} else {
								snprintf(file_name, sizeof(file_name), "result_cpu.txt");
							}
							snprintf(msg, sizeof(msg), "echo -n \"%s \" >> /mnt/sd/ai2_test_files/output/perf/%s", model_cfg->model_filename, file_name);
							system(msg);
							snprintf(msg, sizeof(msg), "echo \"%s %lu %lld %ld %lu %f\" >> /mnt/sd/ai2_test_files/output/perf/%s",
								in_file_cfg[in_file_cfg_idx].in_file_filename,
								sum_time / 10, bw_result, model_cfg->binsize, total_buf_size, cpu_analyze.usage, file_name);
							system(msg);
						} else if (global_data->dma_stress[0].thread_id) {
							if (global_data->first_loop_count > 1) {
								snprintf(file_name, sizeof(file_name), "result_dma_%ld.txt", first_loop_data->net_path);
							} else {
								snprintf(file_name, sizeof(file_name), "result_dma.txt");
							}
							snprintf(msg, sizeof(msg), "echo -n \"%s \" >> /mnt/sd/ai2_test_files/output/perf/%s", model_cfg->model_filename, file_name);
							system(msg);
							snprintf(msg, sizeof(msg), "echo \"%s %lu %lld %ld %lu %f\" >> /mnt/sd/ai2_test_files/output/perf/%s",
								in_file_cfg[in_file_cfg_idx].in_file_filename,
								sum_time / 10, bw_result, model_cfg->binsize, total_buf_size, cpu_analyze.usage, file_name);
							system(msg);
						} else {
							if (strlen(global_data->perf_result_filename)) {
								strncpy(file_name, global_data->perf_result_filename, sizeof(file_name) - 1);
							} else if (global_data->share_model) {
								snprintf(file_name, sizeof(file_name), "result_share_model.txt");
							} else if (global_data->first_loop_count > 1) {
								snprintf(file_name, sizeof(file_name), "result_%ld.txt", first_loop_data->net_path);
							} else {
								snprintf(file_name, sizeof(file_name), "result.txt");
							}
							snprintf(msg, sizeof(msg), "echo -n \"%s \" >> /mnt/sd/ai2_test_files/output/perf/%s", model_cfg->model_filename, file_name);
							system(msg);
							snprintf(msg, sizeof(msg), "echo \"%s %lu %lld %ld %lu %f\" >> /mnt/sd/ai2_test_files/output/perf/%s",
								in_file_cfg[in_file_cfg_idx].in_file_filename,
								sum_time / 10, bw_result, model_cfg->binsize, total_buf_size, cpu_analyze.usage, file_name);
							system(msg);
						}

						//report
						{
							FILE *report_fd;

							if (global_data->first_loop_data[0].perf_analyze_report) {
								if (global_data->first_loop_count == 1) {
									snprintf(msg, sizeof(msg), "/mnt/sd/ai2_test_files/output/perf/network_performance_J%dW%dS%dB%d.csv", (int)first_loop_data->net_job_opt.method, (int)first_loop_data->net_job_opt.wait_ms, (int)first_loop_data->net_job_opt.schd_parm, (int)first_loop_data->net_buf_opt.method);

									if ((report_fd = fopen(msg, "a")) == NULL) {
										printf("open file(%s) fail\n", msg);
										return -1;
									}

									fseek(report_fd, 0, SEEK_END);
									if(ftell(report_fd) < 10) {
										fprintf(report_fd, "model_name,proc_time,dram_bandwidth,model_bin_size,total_buf_size,cpu_loading\n");
									}

									sscanf(model_cfg->model_filename, "/mnt/sd/%*[^/]/%[^/]nvt_model.bin", model_filename);

									fprintf(report_fd, "%s,%lu,%lld,%ld,%lu,%f\n", model_filename, sum_time / 10, bw_result, model_cfg->binsize, total_buf_size, cpu_analyze.usage);
									fclose(report_fd);
								}
							}
						}

						sum_time = 0;
						bw_result = 0;
						cpu_analyze.test_start = FALSE;
						cpu_analyze.test_done = FALSE;
					} else {
						i--;
						continue;
					}
				}
			} else {
				ret = vendor_ai_net_proc(net_path);
			}

			/*if (in_cfg[0].invalid){
				if(ret) {
				//goto next_in_cfg;
					break;
				} else {
					ret = -1;
					printf("net_path(%lu) has wrong in_cfg but vendor_ai fail to detect !!\n", net_path);
					goto exit;
				}
			}else {
				if(HD_OK != ret){
					printf("net_path(%lu) proc fail !!\n", net_path);
					goto exit;
				}
			}*/
			
			if (in_cfg[0].invalid){
				if(ret) {
					if (model_cfg->in_path_num > 0){
						ret = 0;
						goto next_in_cfg;
					}else{
						//goto next_in_cfg;
						break;
					}
				} else {
					ret = -1;
					printf("net_path(%lu) has wrong in_cfg but vendor_ai fail to detect !!\n", net_path);
					goto exit;
				}
			}else {
				if(HD_OK != ret){
					printf("net_path(%lu) proc fail !!\n", net_path);
					goto exit;
				}
			}

			dump_outbuf(net_path, model_cfg);

			if (net_info->out_buf_cnt != 0) {
				if (net_info->out_buf_type == MAKEFOURCC('A','C','0','1')) { //classify 01

					VENDOR_AI_POSTPROC_RESULT_INFO out_buf = {0};
//printf("classify count %d\n", net_info->out_buf_cnt);
					// get output result
					ret = vendor_ai_net_get(net_path, VENDOR_AI_NET_PARAM_OUT(VENDOR_AI_MAXLAYER, 0), &out_buf);
					if (HD_OK != ret) {
						printf("net_path(%lu) output get fail !!\n", net_path);
						goto exit;
					}

					ret = ai_test_network_dump_postproc_buf(net_path, first_loop_data->out_class_labels, &out_buf);
					if (HD_OK != ret) {
						printf("net_path(%lu) output dump fail !!\n", net_path);
						goto exit;
					}

					ret = verify_result(net_info->out_buf_type,
										&out_buf,
										sizeof(out_buf),
										(in_cfg),
										&(first_loop_data->second_loop_data.third_loop_data.fourth_loop_data),
										in_cfg_count != 1,
										0,
										1,
										NULL,
										0);
					if (HD_OK != ret) {
						printf("net_path(%lu) output data is incorrect !!\n", net_path);
						goto exit;
					}
				} else if (net_info->out_buf_type == MAKEFOURCC('A','D','0','1')) { //detection 01

					VENDOR_AI_DETOUT_RESULT_INFO out_buf = {0};
//printf("detection count %d\n", net_info->out_buf_cnt);
					// get output result
					ret = vendor_ai_net_get(net_path, VENDOR_AI_NET_PARAM_OUT(VENDOR_AI_MAXLAYER, 0), &out_buf);
					if (HD_OK != ret) {
						printf("net_path(%lu) output get fail !!\n", net_path);
						goto exit;
					}

					ret = ai_test_network_dump_detout_buf(net_path, &out_buf);
					if (HD_OK != ret) {
						printf("net_path(%lu) output dump fail !!\n", net_path);
						goto exit;
					}

					ret = verify_result(net_info->out_buf_type,
										&out_buf,
										sizeof(out_buf),
										(in_cfg),
										&(first_loop_data->second_loop_data.third_loop_data.fourth_loop_data),
										in_cfg_count != 1,
										0,
										1,
										NULL,
										0);
					if (HD_OK != ret) {
						printf("net_path(%lu) output data is incorrect !!\n", net_path);
						goto exit;
					}
				} else {
					//printf("TODO: net_path(%lu) output dump 1 AI_BUF...\n", net_path);
					//ai_test_network_get_buf_by_out_path_list(net_path);
					//HD_RESULT ret = HD_OK;
					UINT32 *path_list = NULL;
					VENDOR_AI_BUF out_buf = {0};
					UINT32 valid_out_size = 0;
//printf("AI_BUF count(%d,%d)\n", net_info->out_buf_cnt, out_cfg_count);

					// get out_buf_cnt
					ret = vendor_ai_net_get(net_path, VENDOR_AI_NET_PARAM_INFO, net_info);
					if (HD_OK != ret) {
						printf("proc_id(%lu) get info fail !!\n", net_path);
						goto exit;
					}

					//printf("out_buf_cnt(%lu)\n", out_cfg_count);

					// alloc for path_list
					path_list = (UINT32 *)malloc(sizeof(UINT32) * out_cfg_count);
					if(!path_list){
						printf("fail to allocate path_list\n");
						goto exit;
					}

					// get path_list
					ret = vendor_ai_net_get(net_path, VENDOR_AI_NET_PARAM_OUT_PATH_LIST, path_list);
					if (HD_OK != ret) {
						printf("proc_id(%u) get out_path_list fail !!\n", (int)net_path);
						free(path_list);
						goto exit;
					}

					// get out_buf by path_list
					for (j = 0; j < out_cfg_count; j++) {
						ret = vendor_ai_net_get(net_path, path_list[j], &out_buf);

						if (HD_OK != ret) {
							printf("proc_id(%u) output get fail, i(%d), path_list(0x%lx)\n", (int)net_path, j, path_list[j]);
							free(path_list);
							goto exit;
						}
						//printf("[%lu] path_id: 0x%lx\n", j, path_list[j]);

						if ((global_data->load_output || global_data->dump_output) && first_loop_data->diff_model_mem.pa) {
							sscanf(model_cfg->model_cfg_filename, "/mnt/sd/para/model_cfg/%[a-zA-Z0-9_].txt", model_filename);
							//sscanf(model_cfg->model_filename, "/mnt/sd/model/%[a-zA-Z0-9_]/nvt_model.bin", model_filename);
							sprintf(dump_filename, "/mnt/sd/ai2_test_files/output/bin/%s_%02d_%02d_W%ldH%ld.bin", model_filename, in_file_cfg_idx, j, dump_dim.w, dump_dim.h);

							for (UINT32 res_num = 0; res_num < model_cfg->multi_scale_res_num; res_num++) {
								if (dump_dim.w <= model_cfg->multi_scale_valid_out[res_num].w &&
									dump_dim.h <= model_cfg->multi_scale_valid_out[res_num].h) {
									valid_out_size = model_cfg->multi_scale_valid_out[res_num].valid_out_size;
									break;
								}
							}
						}

						if (first_loop_data->diff_model_mem.pa && ret_scale == HD_OK && model_cfg->diff_res_loop) {
							ret = verify_result_diff(net_info->out_buf_type,
											&out_buf,
											model_cfg,
											j,
											out_cfg_count,
											in_file_cfg_idx,
											dump_filename,
											valid_out_size);
							} else if ((dyB_batch_nu > 0) && (dyB_batch != 0)) {
									INT8 bitdepth = HD_VIDEO_PXLFMT_BITS(out_buf.fmt);
									UINT32 length = out_buf.width * out_buf.height * out_buf.channel * out_buf.batch_num;
									if (global_data->load_output){
										sscanf(model_cfg->model_cfg_filename, "/mnt/sd/para/model_cfg/%[a-zA-Z0-9_].txt", model_filename);
										sprintf(dump_filename, "/mnt/sd/ai2_test_files/output/bin/%s_%02d_%02d_W%ldH%ldB%ld.bin", model_filename, in_file_cfg_idx, j, src_img->width, src_img->height,dyB_batch[batch_idx]);
										printf("%s \n",dump_filename);
										length = out_buf.width * out_buf.height * out_buf.channel * out_buf.batch_num * bitdepth>>3;
										ret = verify_result_dynamic_batch(net_info->out_buf_type,
											&out_buf,
											length,
											(in_cfg),
											&(first_loop_data->second_loop_data.third_loop_data.fourth_loop_data),
											in_cfg_count != 1,
											j,
											out_cfg_count,
											dump_filename,
											valid_out_size);
									}
						} else {
							ret = verify_result(net_info->out_buf_type,
											&out_buf,
											sizeof(out_buf),
											(in_cfg),
											&(first_loop_data->second_loop_data.third_loop_data.fourth_loop_data),
											in_cfg_count != 1,
											j,
											out_cfg_count,
											dump_filename,
											valid_out_size);
						}
						if (HD_OK != ret) {
							printf("proc_id(%u) dump ai_buf fail !!\n", (int)net_path);
							free(path_list);
							goto exit;
						}
					}

					free(path_list);
				}
			}
		}

next_in_cfg:
		if(input_mem->size){
			if (ai_test_input_close(input_mem) != HD_OK) {
				printf("input close error\n");
			}
			memset(input_mem, 0, sizeof(MEM_PARM));
		}

		#if 1
		if(scale_mem->size){
			if (ai_test_input_close(scale_mem) != HD_OK) {
				printf("input scale_mem close error\n");
			}
			memset(scale_mem, 0, sizeof(MEM_PARM));
		}
		#endif

		if (in_cfg_count > 1) {
			for(j = 0; j < (int)in_file_cfg[in_file_cfg_idx].input_num; j++) {
				if(in_file_cfg[in_file_cfg_idx].in_cfg[j].input_mem.size){
					if (ai_test_input_close(&(in_file_cfg[in_file_cfg_idx].in_cfg[j].input_mem)) != HD_OK) {
						printf("input close error\n");
					}
					memset(&(in_file_cfg[in_file_cfg_idx].in_cfg[j].input_mem), 0, sizeof(MEM_PARM));
				}
			}
		}

			ai_test_free_out_result(&first_loop_data->second_loop_data.third_loop_data.fourth_loop_data.golden_sample);
		}
	}

exit:
	if(input_mem->size){
		if (ai_test_input_close(input_mem) != HD_OK) {
			printf("input close error\n");
		}
		memset(input_mem, 0, sizeof(MEM_PARM));
	}

	#if 1
	if(scale_mem->size){
		if (ai_test_input_close(scale_mem) != HD_OK) {
			printf("input scale_mem close error\n");
		}
		memset(scale_mem, 0, sizeof(MEM_PARM));
	}
	#endif

	if (in_cfg_count > 1) {
		for(in_file_cfg_idx = 0 ; in_file_cfg_idx < in_file_count ; in_file_cfg_idx++){
			for(j = 0; j < (int)in_file_cfg[in_file_cfg_idx].input_num; j++) {
				if(in_file_cfg[in_file_cfg_idx].in_cfg[j].input_mem.size){
					if (ai_test_input_close(&(in_file_cfg[in_file_cfg_idx].in_cfg[j].input_mem)) != HD_OK) {
						printf("input close error\n");
					}
					memset(&(in_file_cfg[in_file_cfg_idx].in_cfg[j].input_mem), 0, sizeof(MEM_PARM));
				}
			}
		}
	}

	ai_test_free_out_result(&first_loop_data->second_loop_data.third_loop_data.fourth_loop_data.golden_sample);

	if (first_loop_data->diff_model_mem.pa && model_cfg->diff_res_loop && model_cfg->diff_golden_sample) {
		UINT32 i;

		for (i = 0; i < (UINT32)model_cfg->diff_res_num*2; i++) {
			ai_test_free_out_result(&model_cfg->diff_golden_sample[i]);
		}

		model_cfg->diff_golden_sample = 0;

		//printf("free diff result\r\n");
	}

	return ret;
}

static int third_loop(FIRST_LOOP_DATA *first_loop_data, NET_MODEL_CONFIG *model_cfg)
{
	int ret = 0, i;

	if(!first_loop_data || !model_cfg){
		printf("first_loop_data(%p) or model_cfg(%p) is null\n", first_loop_data, model_cfg);
		return -1;
	}

	for(i = 0 ; i < first_loop_data->second_loop_data.third_loop_data.loop_count ; ++i){

		ret = vendor_ai_net_start(first_loop_data->net_path);
		if (HD_OK != ret) {
			printf("first_loop_data->net_path(%lu) start fail !!\n", first_loop_data->net_path);
			return ret;
		}

		ret = fourth_loop(first_loop_data, model_cfg);
		if (ret != HD_OK) {
			printf("fourth_loop() fail=%d, net_path=%d\n", ret, (int)first_loop_data->net_path);
			vendor_ai_net_stop(first_loop_data->net_path);
			return -1;
		}

		ret = vendor_ai_net_stop(first_loop_data->net_path);
		if (HD_OK != ret) {
			printf("net_path(%lu) stop fail !!\n", first_loop_data->net_path);
			return -1;
		}
	}

	return 0;
}

static void reset_second_loop_data(FIRST_LOOP_DATA *first_loop_data)
{
	if(!first_loop_data){
		printf("first_loop_data is null\n");
		return ;
	}

	memset(&first_loop_data->net_info, 0, sizeof(VENDOR_AI_NET_INFO));
}

static int ai_test_net_set_opt(FIRST_LOOP_DATA *first_loop)
{
	HD_RESULT ret;

	if(!first_loop){
		printf("first_loop is null\n");
		return -1;
	}

	ret = ai_test_network_set_config_job(first_loop->net_path, &(first_loop->net_job_opt), first_loop->schd_parm_inc);
	if (ret) {
		printf("proc_id(%lu) network_set_config_job fail=%d\n", first_loop->net_path, ret);
		return -1;
	}

	ret = ai_test_network_set_config_buf(first_loop->net_path, first_loop->net_buf_opt.method, first_loop->net_buf_opt.ddr_id);
	if (ret) {
		printf("proc_id(%lu) network_set_config_buf fail=%d\n", first_loop->net_path, ret);
		return -1;
	}

	return 0;
}

static int second_loop(FIRST_LOOP_DATA *first_loop_data, NET_MODEL_CONFIG *model_cfg)
{
	int ret = 0, final_ret = 0, i;
	GLOBAL_DATA *global = ai_test_get_global_data();
	if(!first_loop_data || !model_cfg){
		printf("first_loop_data(%p) or model_cfg(%p) is null\n", first_loop_data, model_cfg);
		return -1;
	}

	memset(&(first_loop_data->work_mem), 0, sizeof(MEM_PARM));

	if (global->share_model == 2 )
		memset(&(first_loop_data->rbuf_mem), 0, sizeof(MEM_PARM));

	for(i = 0 ; i < first_loop_data->second_loop_data.loop_count ; ++i){

		reset_second_loop_data(first_loop_data);

		if ((ret = ai_test_net_set_opt(first_loop_data)) != HD_OK) {
			if(first_loop_data->invalid_net_path){
				ret = 0;
				goto exit;
			}
			if(first_loop_data->invalid_buf_opt){
				ret = 0;
				goto exit;
			}
			if(first_loop_data->invalid_job_opt){
				ret = 0;
				goto exit;
			}
			printf("net set opt error=%d\n", ret);
			global_fail = 1;
			final_ret = -1;
			goto next;
		}

		ret = ai_test_network_open(first_loop_data->net_path, &(first_loop_data->net_info));
		if (ret != HD_OK) {
			if(model_cfg->invalid){
				ret = 0;
				goto exit;
			}
			printf("net open error=%d\n", ret);
			global_fail = 1;
			final_ret = -1;
			goto next;
		}

		if(first_loop_data->invalid_net_path){
			printf("fail to detect invalid path\n");
			ret = -1;
			vendor_ai_net_close(first_loop_data->net_path);
			global_fail = 1;
			final_ret = -1;
			goto next;
		}
		if(first_loop_data->invalid_buf_opt){
			printf("fail to detect invalid buf opt\n");
			ret = -1;
			vendor_ai_net_close(first_loop_data->net_path);
			global_fail = 1;
			final_ret = -1;
			goto next;
		}
		if(first_loop_data->invalid_job_opt){
			printf("fail to detect invalid job opt\n");
			ret = -1;
			vendor_ai_net_close(first_loop_data->net_path);
			global_fail = 1;
			final_ret = -1;
			goto next;
		}

		ret = ai_test_network_alloc_work_buf(first_loop_data, &(first_loop_data->work_mem));
		if (ret != HD_OK) {
			if(first_loop_data->null_work_buf || first_loop_data->non_align_work_buf){
				ret = 0;
				vendor_ai_net_close(first_loop_data->net_path);
				goto exit;
			}else{
				printf("net_path(%d) alloc work buf error=%d\n", (int)first_loop_data->net_path, ret);
				global_fail = 1;
				final_ret = -1;
				goto next;
			}
		}
		if (global->share_model == 2 ){
			ret = ai_test_network_alloc_rbuf(first_loop_data, &(first_loop_data->rbuf_mem));
			if (ret != HD_OK) {
				printf("net_path(%d) alloc ronly buf error=%d\n", (int)first_loop_data->net_path, ret);
				global_fail = 1;
				final_ret = -1;
				goto next;
			}
		}
		

		if(first_loop_data->null_work_buf){
			printf("fail to detect null work buf\n");
			ret = -1;
			vendor_ai_net_close(first_loop_data->net_path);
			global_fail = 1;
			final_ret = -1;
			goto next;
		}
		if(first_loop_data->non_align_work_buf){
			printf("fail to detect non-aligned work buf\n");
			ret = -1;
			vendor_ai_net_close(first_loop_data->net_path);
			global_fail = 1;
			final_ret = -1;
			goto next;
		}

		ret = third_loop(first_loop_data, model_cfg);
		if (ret != HD_OK) {
			printf("third_loop() fail=%d, net_path=%d\n", ret, (int)first_loop_data->net_path);
			vendor_ai_net_close(first_loop_data->net_path);
			global_fail = 1;
			final_ret = -1;
			goto next;
		}

next:
		if(first_loop_data->work_mem.pa)
			ai_test_mem_free(&(first_loop_data->work_mem));
		if(first_loop_data->rbuf_mem.pa)
			ai_test_mem_free(&(first_loop_data->rbuf_mem));

		memset(&(first_loop_data->work_mem), 0, sizeof(MEM_PARM));
		memset(&(first_loop_data->rbuf_mem), 0, sizeof(MEM_PARM));
		ret = vendor_ai_net_close(first_loop_data->net_path);
		if (ret != HD_OK) {
			printf("net close error=%d\n", ret);
			global_fail = 1;
			final_ret = -1;
		}
	}

exit:
	if(first_loop_data->work_mem.pa)
		ai_test_mem_free(&(first_loop_data->work_mem));
	if(first_loop_data->rbuf_mem.pa)
			ai_test_mem_free(&(first_loop_data->rbuf_mem));

	memset(&(first_loop_data->work_mem), 0, sizeof(MEM_PARM));
	memset(&(first_loop_data->rbuf_mem), 0, sizeof(MEM_PARM));

	return final_ret;
}

static void reset_first_loop_data(FIRST_LOOP_DATA *first_loop_data)
{
	GOLDEN_SAMPLE_RESULT *golden_sample;
	if(!first_loop_data){
		printf("first_loop_data is null\n");
		return ;
	}

	golden_sample = &first_loop_data->second_loop_data.third_loop_data.fourth_loop_data.golden_sample;
	if(golden_sample->data)
		free(golden_sample->data);
	memset(golden_sample, 0, sizeof(GOLDEN_SAMPLE_RESULT));
}

static int check_no_set_model_between_close_open(FIRST_LOOP_DATA *first_loop, NET_MODEL_CONFIG* p_model_cfg){

	HD_RESULT ret = HD_OK;
	VENDOR_AI_NET_CFG_MODEL cfg = {0}, *p_cfg;

	if(!first_loop || !p_model_cfg) {
		printf("first_loop(%p) or p_model_cfg(%p) is null\r\n", first_loop, p_model_cfg);
		return HD_ERR_NULL_PTR;
	}

	if (strlen(p_model_cfg->model_filename) == 0) {
		printf("input model name is null\r\n");
		return HD_ERR_NULL_PTR;
	}

	// set model
	cfg.pa = first_loop->model_mem.pa;
	cfg.va = first_loop->model_mem.va;
	cfg.size = first_loop->model_mem.size;
	p_cfg = &cfg;

	ret = vendor_ai_net_set(first_loop->net_path, VENDOR_AI_NET_PARAM_CFG_MODEL, p_cfg);
	if(ret){
		return 0;
	}

	if (strlen(p_model_cfg->diff_filename) > 0) {

		cfg.pa = first_loop->diff_model_mem.pa;
		cfg.va = first_loop->diff_model_mem.va;
		cfg.size = first_loop->diff_model_mem.size;
		ret = vendor_ai_net_set(first_loop->net_path, VENDOR_AI_NET_PARAM_CFG_MODEL_RESINFO, &cfg);
		if(ret){
			printf("set diff model fail\n");
			return -1;
		}
	}

	ret = vendor_ai_net_open(first_loop->net_path);
	if (HD_OK == ret) {
		printf("net_path(%lu) open failed to report error without set model !!\n", first_loop->net_path);
		return -1;
	}

	return 0;
}

static void dump_failed_model(FIRST_LOOP_DATA *first_loop_data)
{
	int model_idx = 0, i;

	if(!first_loop_data){
		printf("first_loop_data is null\n");
		return;
	}

	for(i = 0 ; i < first_loop_data->loop_count ; ++i){

		if(first_loop_data->model_cfg[model_idx].failed){
			printf("%s\n", first_loop_data->model_cfg[model_idx].model_filename);
		}

		model_idx++;
		if(model_idx >= first_loop_data->second_cfg_count)
			model_idx = 0;
	}
}

static int first_loop(FIRST_LOOP_DATA *first_loop_data, int sequential)
{
	int ret = 0, final_ret = 0, model_idx = 0, i, fail_cnt = 0;
	GLOBAL_DATA *global = ai_test_get_global_data();
	PROCESS_DATA* p_process_data = ai_test_get_process_data();

	if(!first_loop_data || !global){
		printf("first_loop_data(%x) or global(%x) is null\n", first_loop_data, global);
		return -1;
	}

	if(global->init_uninit_per_thread){
		printf("net_path(%d) init its own thread\n", (int)first_loop_data->net_path);
		if ((ret = ai_test_network_init(global->ai_proc_schedule, global->share_model, 1)) != HD_OK) {
			printf("net init error=%d\n", ret);
			global_fail = 1;
			final_ret = -1;
			goto out;
		}
	}

	memset(&(first_loop_data->model_mem), 0, sizeof(MEM_PARM));
	memset(&(first_loop_data->diff_model_mem), 0, sizeof(MEM_PARM));
//	memset(&(first_loop_data->work_mem), 0, sizeof(MEM_PARM));

	for(i = 0 ; i < first_loop_data->loop_count ; ++i){

		/* If multi_process is on, use vendor_ai_get_id() to get new proc_id */
		if ((global->multi_process) && (p_process_data->process_id > 0)) {
			UINT32 multi_proc_id;
			if (HD_OK != vendor_ai_get_id(&multi_proc_id)) {
				global_fail = 1;
				final_ret = -1;
				fail_cnt++;
				first_loop_data->model_cfg[model_idx].failed = 1;
				goto next_model;
			} else {
				first_loop_data->net_path = multi_proc_id;
				printf("process(%d) thread(%d) get_id(%d)\n", p_process_data->process_id,first_loop_data->thread_id, multi_proc_id);
			}
		}

		printf("net_path(%d) model(%s) start\n", (int)first_loop_data->net_path, first_loop_data->model_cfg[model_idx].model_filename);
		reset_first_loop_data(first_loop_data);

		ret = ai_test_network_set_config_model(first_loop_data, &(first_loop_data->model_cfg[model_idx]), sequential);
		if (ret != HD_OK) {
			if(first_loop_data->model_cfg[model_idx].invalid ||
			   first_loop_data->model_cfg[model_idx].model_unset ||
			   first_loop_data->model_cfg[model_idx].model_zero_size ||
			   first_loop_data->model_cfg[model_idx].model_null ||
			   first_loop_data->model_cfg[model_idx].model_zero_addr){
				ret = 0;
				goto next_model;
			}
			if(first_loop_data->invalid_net_path){
				ret = 0;
				goto next_model;
			}
			if(first_loop_data->invalid_buf_opt){
				ret = 0;
				goto next_model;
			}
			if(first_loop_data->invalid_job_opt){
				ret = 0;
				goto next_model;
			}
			printf("net_path(%d) set model error=%d\n", (int)first_loop_data->net_path, ret);
			global_fail = 1;
			final_ret = -1;
			fail_cnt++;
			first_loop_data->model_cfg[model_idx].failed = 1;
			goto next_model;
		}

		ret = second_loop(first_loop_data, &(first_loop_data->model_cfg[model_idx]));
		if(ret){
			printf("net_path(%d) second_loop fail=%d\n", (int)first_loop_data->net_path, ret);
			global_fail = 1;
			final_ret = -1;
			fail_cnt++;
			first_loop_data->model_cfg[model_idx].failed = 1;
			goto next_model;
		}

		if(first_loop_data->model_cfg[model_idx].no_set_model_between_close_open){
			if(check_no_set_model_between_close_open(first_loop_data, &(first_loop_data->model_cfg[model_idx]))){
				ret = -1;
				printf("net_path(%d) fail to detect open without setting model\n", first_loop_data->net_path);
				global_fail = 1;
				final_ret = -1;
				fail_cnt++;
				first_loop_data->model_cfg[model_idx].failed = 1;
				goto next_model;
			}else{
				ret = 0;
				goto next_model;
			}
		}

next_model:

		if(syc_for_freeing_share_model(first_loop_data)){
			printf("net_path(%d) fail to sync for freeing share model\n", (int)first_loop_data->net_path);
			global_fail = 1;
			final_ret = -1;
			fail_cnt++;
			first_loop_data->model_cfg[model_idx].failed = 1;
			break;
		}

		//only 1. none share model 2. share model master has valid model memory
		if(first_loop_data->share_model_master == NULL){
			if(first_loop_data->model_mem.pa){
				ai_test_mem_rel(&(first_loop_data->model_mem));
			}
		}
		if(first_loop_data->diff_model_mem.pa)
			ai_test_mem_rel(&(first_loop_data->diff_model_mem));
//		if(first_loop_data->work_mem.pa)
//			ai_test_mem_rel(&(first_loop_data->work_mem));

		printf("net_path(%d) model(%s) end\n", (int)first_loop_data->net_path, first_loop_data->model_cfg[model_idx].model_filename);
		memset(&(first_loop_data->model_mem), 0, sizeof(MEM_PARM));
		memset(&(first_loop_data->diff_model_mem), 0, sizeof(MEM_PARM));
//		memset(&(first_loop_data->work_mem), 0, sizeof(MEM_PARM));

		/* If multi_process is on, use vendor_ai_release_id() to release proc_id */
		if ((global->multi_process) && (p_process_data->process_id > 0)) {
			UINT32 multi_proc_id;
			multi_proc_id = first_loop_data->net_path;
			if (HD_OK != vendor_ai_release_id(multi_proc_id)) {
				global_fail = 1;
				final_ret = -1;
				fail_cnt++;
				first_loop_data->model_cfg[model_idx].failed = 1;
			} else {
				printf("process(%d) thread(%d) release_id(%d)\n", p_process_data->process_id,first_loop_data->thread_id, multi_proc_id);
			}
		}

		model_idx++;
		if(model_idx >= first_loop_data->second_cfg_count)
			model_idx = 0;

		if (first_loop_data->net_path_max && first_loop_data->net_path_loop) {
			first_loop_data->net_path = (first_loop_data->net_path + 1) % first_loop_data->net_path_max;
		}
	}

	//only 1. none share model 2. share model master has valid model memory
	if(first_loop_data->share_model_master == NULL){
		if(first_loop_data->model_mem.pa){
			ai_test_mem_rel(&(first_loop_data->model_mem));
		}
	}
	if(first_loop_data->diff_model_mem.pa)
		ai_test_mem_rel(&(first_loop_data->diff_model_mem));
//	if(first_loop_data->work_mem.pa)
//		ai_test_mem_rel(&(first_loop_data->work_mem));

	memset(&(first_loop_data->model_mem), 0, sizeof(MEM_PARM));
	memset(&(first_loop_data->diff_model_mem), 0, sizeof(MEM_PARM));
//	memset(&(first_loop_data->work_mem), 0, sizeof(MEM_PARM));

	ai_test_lock_and_sync();
	if(final_ret){
		printf("net_path(%d) has %d model fail\n", (int)first_loop_data->net_path, fail_cnt);
		dump_failed_model(first_loop_data);
	}
	ai_test_unlock();

	if(global->init_uninit_per_thread){
		printf("net_path(%d) uninit its own thread\n", (int)first_loop_data->net_path);
		if ((ret = ai_test_network_uninit()) != HD_OK) {
			printf("net uninit error=%d\n", ret);
			global_fail = 1;
			final_ret = -1;
			goto out;
		}
	}

out:
	ai_test_lock_and_sync();
	if(final_ret == 0)
		first_loop_data->test_success = 1;
	else{
		first_loop_data->test_success = -1;
	}
	ai_test_unlock();

	return final_ret;
}

VOID *network_sequential_thread(VOID *arg)
{
	int ret, i, first_loop_idx = 0;
	GLOBAL_DATA *global = (GLOBAL_DATA*)arg;
	FIRST_LOOP_DATA *first_loop_data;

	if(!global){
		printf("network_sequential_thread() global is null\n");
		return NULL;
	}

	first_loop_data = global->first_loop_data;
	if(!first_loop_data){
		printf("network_sequential_thread() first_loop is null\n");
		return NULL;
	}

	for(i = 0 ; i < global->sequential_loop_count ; ++i){
		if(first_loop_data[first_loop_idx].model_cfg){
			ret = first_loop(&(first_loop_data[first_loop_idx]), 1);
		}else{
			ret = op_first_loop(&(first_loop_data[first_loop_idx]));
		}
		if(ret){
			printf("%dth first_loop() fail, net_path=%d\n", i, (int)first_loop_data[i].net_path);
			global_fail = 1;
		}

		first_loop_idx++;
		if(first_loop_idx >= global->first_loop_count)
			first_loop_idx = 0;
	}

	return NULL;
}

VOID *network_parallel_thread(VOID *arg)
{
	int ret;

	FIRST_LOOP_DATA *first_loop_data = (FIRST_LOOP_DATA*)arg;
	if(!first_loop_data){
		printf("network_parallel_thread() first_loop is null\n");
		return NULL;
	}

	if(first_loop_data->model_cfg){
		ret = first_loop(first_loop_data, 0);
	}else{
		ret = op_first_loop(first_loop_data);
	}
	if(ret)
		printf("first_loop() fail, net_path=%d\n", (int)first_loop_data->net_path);

	return NULL;
}
