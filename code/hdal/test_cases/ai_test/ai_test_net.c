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
#include "hd_debug.h"

/*-----------------------------------------------------------------------------*/
/* Network Functions                                                           */
/*-----------------------------------------------------------------------------*/

static NET_PROC g_net[16] = {0};
TEST_PROC_ID g_test_proc_id = {0};
VENDOR_AI_NET_INFO net_info = {0};
UINT64 net_proc_time[7] = {0};
DOUBLE net_cpu_loading = 0.0;
extern BOOL use_diff_model;
extern BOOL use_one_core;
extern UINT32 set_batch_num[NN_RUN_NET_NUM][NN_SET_INPUT_NUM];

static INT32 _getsize_model(char *filename)
{
	FILE *bin_fd;
	UINT32 bin_size = 0;

	bin_fd = fopen(filename, "rb");
	if (!bin_fd) {
		printf("get bin(%s) size fail\n", filename);
		return (-1);
	}

	fseek(bin_fd, 0, SEEK_END);
	bin_size = ftell(bin_fd);
	fseek(bin_fd, 0, SEEK_SET);
	fclose(bin_fd);

	return ALIGN_CEIL_64(bin_size);
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
	file_size = ALIGN_CEIL_64(ftell(fd));
	fseek ( fd, 0, SEEK_SET );

	read_size = fread ((void *)model_addr, 1, file_size, fd);
	if (read_size != file_size) {
		printf("size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)file_size);
	}
	fclose(fd);
	
	printf("load model(%s) ok\r\n", filename);
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


HD_RESULT ai_test_network_init(void)
{
	HD_RESULT ret = HD_OK;
	int i;

//	ret = hd_videoproc_init();
//	if (ret != HD_OK) {
//		printf("hd_videoproc_init fail=%d\n", ret);
//		return ret;
//	}

	ret = ai_test_network_set_config();
	if (HD_OK != ret) {
		printf("network_set_config fail=%d\n", ret);
		return 0;
	}
	ret = vendor_ai_init();
	if (ret != HD_OK) {
		printf("vendor_ai_init fail=%d\n", ret);
		return ret;
	}

	for (i = 0; i < 16; i++) {
		NET_PROC *p_net = g_net + i;
		p_net->proc_id = i;
	}
	return ret;
}

HD_RESULT ai_test_network_sel_proc_id(UINT32 proc_id)
{
	HD_RESULT ret = HD_OK;

	printf("select proc_id: (%lu)\r\n", proc_id);
	g_test_proc_id.enable = TRUE;
	g_test_proc_id.proc_id = proc_id;

	return ret;
}

HD_RESULT ai_test_network_uninit(void)
{
	HD_RESULT ret = HD_OK;

//	ret = hd_videoproc_uninit();
//	if (ret != HD_OK) {
//		printf("hd_videoproc_uninit fail=%d\n", ret);
//	}
	ret = vendor_ai_uninit();
	if (ret != HD_OK) {
		printf("vendor_ai_uninit fail=%d\n", ret);
	}
	
	return ret;
}

INT32 ai_test_network_mem_config(NET_PATH_ID net_path, HD_COMMON_MEM_INIT_CONFIG* p_mem_cfg, void* p_cfg, INT32 i)
{
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id;
	NET_MODEL_CONFIG* p_model_cfg = (NET_MODEL_CONFIG*)p_cfg;
	p_net->proc_id = net_path;
	proc_id = p_net->proc_id;
	
	if (strlen(p_model_cfg->model_filename) == 0) {
		printf("proc_id(%lu) input model is null\r\n", proc_id);
		return i;
	}

	p_model_cfg->binsize = _getsize_model(p_model_cfg->model_filename);
	if (p_model_cfg->binsize <= 0) {
		printf("proc_id(%lu) input model is not exist?\r\n", proc_id);
		return i;
	}
	
	printf("proc_id(%lu) set net_mem_cfg: model-file(%s), binsize=%ld\r\n",
		proc_id,
		p_model_cfg->model_filename,
		p_model_cfg->binsize);

	printf("proc_id(%lu) set net_mem_cfg: label-file(%s)\r\n",
		proc_id,
		p_model_cfg->label_filename);
	
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_) || defined(_BSP_NA51102_)
	// config common pool (in)
	p_mem_cfg->pool_info[i].type = HD_COMMON_MEM_USER_DEFINIED_POOL + proc_id;
	p_mem_cfg->pool_info[i].blk_size = p_model_cfg->binsize;
	p_mem_cfg->pool_info[i].blk_cnt = 1;
	p_mem_cfg->pool_info[i].ddr_id = DDR_ID0;
	i++;
#endif
	if (strlen(p_model_cfg->diff_filename) > 0) {
		p_model_cfg->diff_binsize = _getsize_model(p_model_cfg->diff_filename);
		if (p_model_cfg->diff_binsize <= 0) {
			printf("proc_id(%lu) diff model is not exist?\r\n", proc_id);
			return i;
		}

		printf("proc_id(%lu) set net_mem_cfg: diff-file(%s), binsize=%ld\r\n",
			proc_id,
			p_model_cfg->diff_filename,
			p_model_cfg->diff_binsize);
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_) || defined(_BSP_NA51102_)
		// config common pool (in)
		p_mem_cfg->pool_info[i].type = HD_COMMON_MEM_USER_DEFINIED_POOL + proc_id;
		p_mem_cfg->pool_info[i].blk_size = p_model_cfg->diff_binsize;
		p_mem_cfg->pool_info[i].blk_cnt = 1;
		p_mem_cfg->pool_info[i].ddr_id = DDR_ID0;
		i++;
#endif
	}

	return i;
}

HD_RESULT ai_test_network_set_config(void)
{
	HD_RESULT ret = HD_OK;

	// config extend engine plugin, process scheduler
	{
		UINT32 schd = VENDOR_AI_PROC_SCHD_FAIR;
		UINT32 not_check = 1;
		vendor_ai_cfg_set(VENDOR_AI_CFG_PLUGIN_ENGINE, vendor_ai_cpu1_get_engine());
		vendor_ai_cfg_set(VENDOR_AI_CFG_PROC_SCHD, &schd);
		vendor_ai_cfg_set(VENDOR_AI_CFG_NOTCHECK_INPUT_ALIGN, &not_check);
	}

	return ret;
}

HD_RESULT ai_test_network_set_config_job(NET_PATH_ID net_path, UINT32 method, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id;
	p_net->proc_id = net_path;
	proc_id = p_net->proc_id;

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	// set job opt
	{
		VENDOR_AI_NET_CFG_JOB_OPT cfg_job_opt = {0};
		cfg_job_opt.method = method;
		cfg_job_opt.wait_ms = wait_ms;
		if(use_one_core == 0)
			cfg_job_opt.schd_parm = VENDOR_AI_FAIR_CORE_ALL; //FAIR dispatch to ALL core
		else
			cfg_job_opt.schd_parm = VENDOR_AI_FAIR_CORE(0); //FAIR  select dla core
		printf("proc_id(%lu) set net_cfg: job-opt(%d), sync(%d)\r\n",
			proc_id,
			cfg_job_opt.method,
			(int)cfg_job_opt.wait_ms);
		vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_JOB_OPT, &cfg_job_opt);
	}

	return ret;
}

HD_RESULT ai_test_network_set_config_buf(NET_PATH_ID net_path, UINT32 method, UINT32 ddr_id)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id;
	p_net->proc_id = net_path;
	proc_id = p_net->proc_id;

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	// set buf opt
	{
		VENDOR_AI_NET_CFG_BUF_OPT cfg_buf_opt = {0};
		cfg_buf_opt.method = method;
		cfg_buf_opt.ddr_id = ddr_id; //DDR_ID0;
		printf("proc_id(%lu) set net_cfg: buf-opt(%d), ddr_id(%lu)\r\n",
			proc_id,
			(int)cfg_buf_opt.method,
			cfg_buf_opt.ddr_id);
		vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_BUF_OPT, &cfg_buf_opt);
	}

	return ret;
}


HD_RESULT ai_test_network_set_config_model(NET_PATH_ID net_path, NET_MODEL_CONFIG* p_model_cfg)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id;
	p_net->proc_id = net_path;
	proc_id = p_net->proc_id;

	UINT32 loadsize = 0;

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	if (strlen(p_model_cfg->model_filename) == 0) {
		printf("proc_id(%lu) input model is null\r\n", proc_id);
		return HD_ERR_NULL_PTR;
	}

	ret = ai_test_mem_get(&p_net->proc_mem, p_model_cfg->binsize, proc_id);
	if (ret != HD_OK) {
		printf("proc_id(%lu) get mem fail=%d\n", proc_id, ret);
		return HD_ERR_FAIL;
	}

	//load file
	loadsize = _load_model(p_model_cfg->model_filename, p_net->proc_mem.va);
	if (loadsize <= 0) {
		printf("proc_id(%lu) input model load fail: %s\r\n", proc_id, p_model_cfg->model_filename);
		return HD_ERR_FAIL;
	}

	ret = _load_label((UINT32)p_net->out_class_labels, VENDOR_AIS_LBL_LEN, p_model_cfg->label_filename);
	if (ret != HD_OK) {
		printf("proc_id(%lu) load label fail=%d\n", proc_id, ret);
		return HD_ERR_FAIL;
	}

	// set model
	vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_MODEL, (VENDOR_AI_NET_CFG_MODEL*)&p_net->proc_mem);

	if(use_diff_model) {
		if (strlen(p_model_cfg->diff_filename) > 0) {
			ai_test_mem_get(&p_net->diff_mem, p_model_cfg->diff_binsize, proc_id);
			if (ret != HD_OK) {
	            printf("proc_id(%lu) get mem fail=%d\n", proc_id, ret);
	            return HD_ERR_FAIL;
        	}
			//load file
			loadsize = _load_model(p_model_cfg->diff_filename, p_net->diff_mem.va);
			if (loadsize <= 0) {
				printf("proc_id(%lu) diff model load fail: %s\r\n", proc_id, p_model_cfg->diff_filename);
				return 0;
			}
			vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_MODEL_RESINFO, (VENDOR_AI_NET_CFG_MODEL*)&p_net->diff_mem);
		}
	}


	return ret;
}

HD_RESULT ai_test_network_set_in_buf(NET_PATH_ID net_path, UINT32 layer_id, UINT32 in_id, VENDOR_AI_BUF *p_inbuf)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	// set input image
	ret = vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_IN(layer_id, in_id), p_inbuf);
	if (HD_OK != ret) {
		printf("proc_id(%lu)push input fail !!\n", proc_id);
		goto exit;
	} else {
		printf("proc_id(%lu)push input ok\n", proc_id);
	}

exit:
	return ret;
}

HD_RESULT ai_test_network_set_unknown(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	UINT32 unknown_id = 0x1234;

	ret = vendor_ai_net_set(proc_id, unknown_id, 0);
	if (HD_OK != ret) {
		printf("proc_id(%lu) set unknown param_id(0x%lx) fail !!\n", proc_id, unknown_id);
		goto exit;
	}

exit:
	return ret;
}

HD_RESULT ai_test_network_get_in_buf_byid(NET_PATH_ID net_path, UINT32 layer_id, UINT32 in_id)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	VENDOR_AI_BUF inbuf = {0};

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_IN(layer_id, in_id), &inbuf);
	if (HD_OK != ret) {
		printf("proc_id(%lu) get IN_BUF fail !! layer(%lu) in(%lu)\n", proc_id, layer_id, in_id);
		goto exit;
	} else {
		printf("proc_id(%lu) get IN_BUF layer(%lu) in(%lu):\n", proc_id, layer_id, in_id);
		printf("   pa(0x%lx)\n", inbuf.pa);
		printf("   va(0x%lx)\n", inbuf.va);
		printf("   size(0x%lx)\n", inbuf.size);
		printf("\n");
	}

exit:
	return ret;
}

HD_RESULT ai_test_network_get_out_buf_byid(NET_PATH_ID net_path, UINT32 layer_id, UINT32 out_id)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	VENDOR_AI_BUF outbuf = {0};

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT(layer_id, out_id), &outbuf);
	if (HD_OK != ret) {
		printf("proc_id(%lu) get OUT_BUF fail !! layer(%lu) out(%lu)\n", proc_id, layer_id, out_id);
		goto exit;
	} else {
		printf("proc_id(%lu) get OUT_BUF layer(%lu) out(%lu):\n", proc_id, layer_id, out_id);
		printf("   pa(0x%lx)\n", outbuf.pa);
		printf("   va(0x%lx)\n", outbuf.va);
		printf("   size(0x%lx)\n", outbuf.size);
		printf("\n");
	}

exit:
	return ret;
}

HD_RESULT ai_test_network_get_first_in_buf(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	VENDOR_AI_BUF inbuf = {0};

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_IN(0, 0), &inbuf);
	if (HD_OK != ret) {
		printf("proc_id(%lu) get FIRST_LAYER_INPUT fail !!\n", proc_id);
		goto exit;
	} else {
		printf("proc_id(%lu) get FIRST_LAYER_INPUT:\n", proc_id);
		printf("   channel(%lu)\n", inbuf.channel);
		printf("   fmt(0x%x)\n", inbuf.fmt);
		printf("\n");
	}

exit:
	return ret;
}

HD_RESULT ai_test_network_get_first_out_buf(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	VENDOR_AI_BUF outbuf = {0};

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT(0, 0), &outbuf);
	if (HD_OK != ret) {
		printf("proc_id(%lu) get FIRST_LAYER_OUTPUT fail !!\n", proc_id);
		goto exit;
	} else {
		printf("proc_id(%lu) get FIRST_LAYER_OUTPUT:\n", proc_id);
		printf("   pa(0x%lx)\n", outbuf.pa);
		printf("   va(0x%lx)\n", outbuf.va);
		printf("   size(0x%lx)\n", outbuf.size);
		printf("   width(%lu)\n", outbuf.width);
		printf("   height(%lu)\n", outbuf.height);
		printf("   channel(%lu)\n", outbuf.channel);
		printf("   batch_num(%lu)\n", outbuf.batch_num);
		printf("   fmt(0x%x)\n", outbuf.fmt);
		printf("\n");
	}
	
exit:
	return ret;
}

HD_RESULT ai_test_network_get_out_buf_byname(NET_PATH_ID net_path, VENDOR_AI_BUF_NAME *p_buf_name)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	UINT32 layer_id, out_id;
	VENDOR_AI_BUF outbuf = {0};

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	// get out_path by name
	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT_PATH_BY_NAME, p_buf_name);
	if (HD_OK != ret) {
		printf("proc_id(%lu) get OUT_PATH_BY_NAME fail !!\n", proc_id);
		goto exit;
	}
	printf("proc_id(%lu) get PATH_ID(%lu) by NAME(%s)\n", proc_id, p_buf_name->path_id, p_buf_name->name);

	layer_id = p_buf_name->path_id;
	out_id = 0;
	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT(layer_id, 0), &outbuf);
	if (HD_OK != ret) {
		printf("proc_id(%lu) get OUT_BUF fail !!\n", proc_id);
		goto exit;
	} else {
		printf("proc_id(%lu) get OUT_BUF layer(%lu) out(%lu):\n", proc_id, layer_id, out_id);
		printf("   sign(0x%lx)\n", outbuf.sign);
		printf("   ddr_id(%d)\n", outbuf.ddr_id);
		printf("   pa(0x%lx)\n", outbuf.pa);
		printf("   va(0x%lx)\n", outbuf.va);
		printf("   size(%lu)\n", outbuf.size);
		printf("   width(%lu)\n", outbuf.width);
		printf("   height(%lu)\n", outbuf.height);
		printf("   channel(%lu)\n", outbuf.channel);
		printf("   batch_num(%lu)\n", outbuf.batch_num);
		printf("   line_ofs(%lu)\n", outbuf.line_ofs);
		printf("   channel_ofs(%lu)\n", outbuf.channel_ofs);
		printf("   batch_ofs(%lu)\n", outbuf.batch_ofs);
		printf("   time_ofs(%lu)\n", outbuf.time_ofs);

		// parsing pixel format
		printf("parsing pixel format:\n");
		printf("   fmt(0x%x)\n", outbuf.fmt);
		switch (AI_PXLFMT_TYPE(outbuf.fmt)) {
		case HD_VIDEO_PXLFMT_AI_UINT8:
		case HD_VIDEO_PXLFMT_AI_SINT8:
		case HD_VIDEO_PXLFMT_AI_UINT16:
		case HD_VIDEO_PXLFMT_AI_SINT16:
		case HD_VIDEO_PXLFMT_AI_UINT32:
		case HD_VIDEO_PXLFMT_AI_SINT32: {
			UINT32 int_bits = HD_VIDEO_PXLFMT_INT(outbuf.fmt);
			UINT32 frac_bits = HD_VIDEO_PXLFMT_FRAC(outbuf.fmt);
			printf("   int_bits(0x%lx)\n", int_bits);
			printf("   frac_bits(0x%lx)\n", frac_bits);
		}
		break;
		case HD_VIDEO_PXLFMT_AI_FLOAT32: {
			// [TBD]
		}
		break;
		default:
			printf("unknown pxlfmt(0x%x)\n", outbuf.fmt);
			break;
		}
		printf("\n");
	}
	
exit:
	return ret;
}

HD_RESULT ai_test_network_get_unknown(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	UINT32 unknown_id = 0x1234;

	ret = vendor_ai_net_get(proc_id, unknown_id, 0);
	if (HD_OK != ret) {
		printf("proc_id(%lu) get unknown param_id(0x%lx) fail !!\n", proc_id, unknown_id);
		goto exit;
	}

exit:
	return ret;
}

HD_RESULT ai_test_network_clr_config_model(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	//UINT32 proc_id;
	p_net->proc_id = net_path;
	//proc_id = p_net->proc_id;

	ai_test_mem_rel(&p_net->proc_mem);
	if (use_diff_model) {
		ai_test_mem_rel(&p_net->diff_mem);
	}

	return ret;
}

HD_RESULT ai_test_network_alloc_io_buf(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	VENDOR_AI_NET_CFG_WORKBUF wbuf = {0};

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("proc_id(%lu) get VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}
	p_net->io_mem.size = wbuf.size;
	ret = ai_test_mem_alloc(&p_net->io_mem, "ai_io_buf", wbuf.size);
	if (ret != HD_OK) {
		printf("proc_id(%lu) alloc ai_io_buf fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}

	wbuf.pa = p_net->io_mem.pa;
	wbuf.va = p_net->io_mem.va;
	wbuf.size = p_net->io_mem.size;
	ret = vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("proc_id(%lu) set VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}

	printf("alloc_io_buf: work buf, pa = %#lx, va = %#lx, size = %lu\r\n", wbuf.pa, wbuf.va, wbuf.size);

	return ret;
}

HD_RESULT ai_test_network_alloc_null_io_buf(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	VENDOR_AI_NET_CFG_WORKBUF wbuf = {0};

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	wbuf.pa = 0; // invalid NULL pa
	wbuf.va = 0; // invalid NULL va
	wbuf.size = 0x1234;
	ret = vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("proc_id(%lu) set VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}

	return ret;
}

HD_RESULT ai_test_network_alloc_non_align_io_buf(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	VENDOR_AI_NET_CFG_WORKBUF wbuf = {0};

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("proc_id(%lu) get VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}
	ret = ai_test_mem_alloc(&p_net->io_mem, "ai_io_buf", wbuf.size+16); // alloc extra 16 bytes
	if (ret != HD_OK) {
		printf("proc_id(%lu) alloc ai_io_buf fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}

	wbuf.pa = p_net->io_mem.pa + 16; // simulate non-32x align
	wbuf.va = p_net->io_mem.va + 16; // simulate non-32x align
	wbuf.size = p_net->io_mem.size;
	ret = vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("proc_id(%lu) set VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}

	printf("alloc_io_buf: work buf, pa = %#lx, va = %#lx, size = %lu\r\n", wbuf.pa, wbuf.va, wbuf.size);

	return ret;
}

HD_RESULT ai_test_network_free_io_buf(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;

	if (p_net->io_mem.pa && p_net->io_mem.va) {
		ai_test_mem_free(&p_net->io_mem);
	}
	return ret;
}

HD_RESULT ai_test_network_open(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	HD_RESULT ret2 = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id;
	p_net->proc_id = net_path;
	proc_id = p_net->proc_id;

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	// open
	ret = vendor_ai_net_open(proc_id);
	if (HD_OK != ret) {
		printf("proc_id(%lu) open fail !!\n", proc_id);
	}

	ret2 = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_INFO, &net_info);
	if (HD_OK != ret2) {
		printf("proc_id(%lu) get info fail !!\n", proc_id);
	} else {
		printf("proc_id(%lu) in buf cnt=%d, type=%04x; out buf cnt=%d, type=%04x\n", 
			proc_id, 
			(int)net_info.in_buf_cnt, 
			(unsigned int)net_info.in_buf_type, 
			(int)net_info.out_buf_cnt, 
			(unsigned int)net_info.out_buf_type);
	}

	return ret;
}

HD_RESULT ai_test_network_close(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	if ((ret = ai_test_network_free_io_buf(net_path)) != HD_OK) {
		printf("in free error=%d\n", ret);
		return HD_ERR_NG;
	}
	// close
	ret = vendor_ai_net_close(proc_id);
	
	return ret;
}

HD_RESULT ai_test_network_dump_postproc_buf(NET_PATH_ID net_path, void *p_out)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	VENDOR_AI_POSTPROC_RESULT_INFO *p_rslt_info = (VENDOR_AI_POSTPROC_RESULT_INFO *)(p_out);
	UINT32 i, j;
	
	printf("proc_id(%lu) classification results:\r\n", proc_id);
	if (p_rslt_info) {
		for (i = 0; i < p_rslt_info->result_num; i++) {
			VENDOR_AI_POSTPROC_RESULT *p_rslt = &p_rslt_info->p_result[i];
			for (j = 0; j < NN_POSTPROC_TOP_N; j++) {
				printf("%ld. no=%ld, label=%s, score=%f\r\n", 
					j + 1, 
					p_rslt->no[j],
					&p_net->out_class_labels[p_rslt->no[j] * VENDOR_AIS_LBL_LEN], 
					p_rslt->score[j]);
			}
		}
	}
	return ret;
}

HD_RESULT ai_test_network_dump_detout_buf(NET_PATH_ID net_path, void *p_out)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	VENDOR_AI_DETOUT_RESULT_INFO *p_rslt_info = (VENDOR_AI_DETOUT_RESULT_INFO *)(p_out);
	VENDOR_AI_DETOUT_RESULT *p_rslt;
	UINT32 i;

	printf("proc_id(%lu) detection results:\r\n", proc_id);
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

	return ret;
}


///////////////////////////////////////////////////////////////////////////////

static VOID *network_user_thread(VOID *arg);

HD_RESULT ai_test_network_user_start(TEST_NETWORK *p_stream)
{
	HD_RESULT ret = HD_OK;
	UINT32 proc_id = p_stream->net_path;

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}

	p_stream->proc_start = 0;
	p_stream->proc_exit = 0;
	p_stream->proc_oneshot = 0;
	
	ret = vendor_ai_net_start(proc_id);
	if (HD_OK != ret) {
		printf("proc_id(%lu) start fail !!\n", proc_id);
	}
	
	ret = pthread_create(&p_stream->proc_thread_id, NULL, network_user_thread, (VOID*)(p_stream));
	if (ret < 0) {
		return HD_ERR_FAIL;
	}

	p_stream->proc_start = 1;
	p_stream->proc_exit = 0;
	p_stream->proc_oneshot = 0;
	
	return ret;
}

HD_RESULT ai_test_network_user_oneshot(TEST_NETWORK *p_stream, const char* name)
{
	HD_RESULT ret = HD_OK;
	p_stream->proc_oneshot = 1;
	return ret;
}

HD_RESULT ai_test_network_user_multishot(TEST_NETWORK *p_stream, const char* name, UINT32 shot_time_ms)
{
	HD_RESULT ret = HD_OK;
	
#if USE_AI2_CPU_LOADING || defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
	UINT64 start_t= hd_gettime_us();
	BOOL b_cpu_1 = FALSE, b_cpu_2 = FALSE;
#else 
	DOUBLE cpu_usr = 0.0;
	DOUBLE cpu_sys = 0.0;
#endif

	if (shot_time_ms < 3000) {
		printf("shot_time_ms should >= 3000\n");
		return HD_ERR_PARAM;
	}

	p_stream->proc_oneshot = 2; // unlimit shot
	
#if USE_AI2_CPU_LOADING || defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
	while (1){
		// get cpu stat 1 second after  it start
		if ((b_cpu_1 == FALSE) && (hd_gettime_us() - start_t > (1000*1000))) {
			b_cpu_1 = TRUE;
			system("grep 'cpu ' /proc/stat > /tmp/cpustat_1");
		}

		// get cpu stat 1 second before it end
		if ((b_cpu_2 == FALSE) && (hd_gettime_us() - start_t > ((shot_time_ms-1000)*1000))) {
			b_cpu_2 = TRUE;
			system("grep 'cpu ' /proc/stat > /tmp/cpustat_2");
		}

		// check time exceed
		if (hd_gettime_us() - start_t > (shot_time_ms*1000)) {
			break;
		}

		// sleep 0.1 ms
		usleep(100);
	}
	p_stream->proc_oneshot = 1; // last shot
	sleep(1);

	// print CPU usage
	system("echo -e \"$(cat /tmp/cpustat_1)\n$(cat /tmp/cpustat_2)\" | awk 'NR == 1 {owork=($2+$3+$4+$6+$7+$8);oidle=$5;} NR > 1 {work=($2+$3+$4+$6+$7+$8)-owork;idle=$5-oidle; printf \"CPU usage = %.1f %\\n\", 100 * work / (work+idle)}'");
#else 
	cal_linux_cpu_loading(5, &cpu_usr, &cpu_sys);
	net_cpu_loading = cpu_usr + cpu_sys;
	p_stream->proc_oneshot = 1; // last shot
#endif

	return ret;
}

HD_RESULT ai_test_network_user_stop(TEST_NETWORK *p_stream)
{
	HD_RESULT ret = HD_OK;
	UINT32 proc_id = p_stream->net_path;

	// test proc id
	if (g_test_proc_id.enable == TRUE) {
		proc_id = g_test_proc_id.proc_id;
	}
	
	p_stream->proc_exit = 1;
	
	if (p_stream->proc_thread_id) {
		pthread_join(p_stream->proc_thread_id, NULL);
	}

	//stop: should be call after last time proc
	ret = vendor_ai_net_stop(proc_id);
	if (HD_OK != ret) {
		printf("proc_id(%lu) stop fail !!\n", proc_id);
	}

	return ret;
}

static HD_RESULT network_dump_ai_outbuf(NET_PATH_ID net_path, VENDOR_AI_BUF *p_outbuf)
{
	HD_RESULT ret = HD_OK;

	printf("  sign(0x%lx) ddr(%lu) pa(0x%lx) va(0x%lx) sz(%lu) w(%lu) h(%lu) ch(%lu) batch_num(%lu)\n",
		p_outbuf->sign, p_outbuf->ddr_id, p_outbuf->pa, p_outbuf->va, p_outbuf->size, p_outbuf->width, 
		p_outbuf->height, p_outbuf->channel, p_outbuf->batch_num);
	printf("  l_ofs(%lu) c_ofs(%lu) b_ofs(%lu) t_ofs(%lu) layout(%s) name(%s) op_name(%s) scale_ratio(%.6f)\n",
		p_outbuf->line_ofs, p_outbuf->channel_ofs, p_outbuf->batch_ofs, p_outbuf->time_ofs, p_outbuf->layout,
		p_outbuf->name, p_outbuf->op_name, p_outbuf->scale_ratio);

	// parsing pixel format
	switch (HD_VIDEO_PXLFMT_TYPE(p_outbuf->fmt))) {
	case HD_VIDEO_PXLFMT_AI_UINT8:
	case HD_VIDEO_PXLFMT_AI_SINT8:
	case HD_VIDEO_PXLFMT_AI_UINT16:
	case HD_VIDEO_PXLFMT_AI_SINT16:
	case HD_VIDEO_PXLFMT_AI_UINT32:
	case HD_VIDEO_PXLFMT_AI_SINT32:
		{
			INT8 bitdepth = HD_VIDEO_PXLFMT_BITS(p_outbuf->fmt);
			INT8 int_bits = HD_VIDEO_PXLFMT_INT(p_outbuf->fmt);
			INT8 frac_bits = HD_VIDEO_PXLFMT_FRAC(p_outbuf->fmt);
			printf("  fmt(0x%x) bits(%u) int(%u) frac(%u)\n", p_outbuf->fmt, bitdepth, int_bits, frac_bits);
		}
		break;
	default:
		switch ((UINT32)p_outbuf->fmt) {
		case HD_VIDEO_PXLFMT_BGR888_PLANAR:
			{
				printf("  fmt(0x%x), BGR888_PLANAR\n", p_outbuf->fmt);
			}
			break;
		case HD_VIDEO_PXLFMT_YUV420:
			{
				printf("  fmt(0x%x), YUV420\n", p_outbuf->fmt);
			}
			break;
		case HD_VIDEO_PXLFMT_Y8:
			{
				printf("  fmt(0x%x), Y8 only\n", p_outbuf->fmt);
			}
			break;
		case HD_VIDEO_PXLFMT_UV:
			{
				printf("  fmt(0x%x), UV only\n", p_outbuf->fmt);
			}
			break;
		case 0:
			{
				printf("  fmt(0x%x), AI BUF\n", p_outbuf->fmt);
			}
			break;
		default:
			printf("unknown pxlfmt(0x%x)\n", p_outbuf->fmt);
			break;
		}
	}
	printf("\n");

	return ret;
}

static HD_RESULT set_buf_by_in_path_list(TEST_NETWORK *p_stream)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_NET_INFO net_info = {0};
	VENDOR_AI_BUF in_buf = {0};
	VENDOR_AI_BUF tmp_buf = {0};
	UINT32 proc_id = p_stream->net_path;
	UINT32 i = 0, j = 0, idx = 0;
	UINT32 in_buf_cnt = 0, src_img_size = 0;
	UINTPTR tmp_src_va = 0, tmp_src_pa = 0;
	UINT32 *in_path_list = NULL;

	/* get input process number */
	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_INFO, &net_info);
	if (HD_OK != ret) {
		printf("proc_id(%lu) get VENDOR_AI_NET_PARAM_INFO fail(%d) !!\n", proc_id, ret);
		goto exit;
	}
	in_buf_cnt = net_info.in_buf_cnt;
	printf("==============< %s, in_buf_cnt(%lu) >==============\n", __func__, in_buf_cnt);

	/* get in path list */
	in_path_list = (UINT32 *)malloc(sizeof(UINT32) * in_buf_cnt);
	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_IN_PATH_LIST, in_path_list);
	if (HD_OK != ret) {
		printf("proc_id(%lu) get VENDOR_AI_NET_PARAM_IN_PATH_LIST fail(%d) !!\n", proc_id, ret);
		goto exit;
	}

	for (i = 0; i < in_buf_cnt; i++) {
		/* get in buf (by in path list) */
		ret = vendor_ai_net_get(proc_id, in_path_list[i], &tmp_buf);
		if (HD_OK != ret) {
			printf("proc_id(%lu) get in buf fail, i(%lu), in_path(0x%lx)\n", proc_id, i, in_path_list[i]);
			goto exit;
		}

		// dump in buf
		printf("dump_in_buf: path_id: 0x%lx\n", in_path_list[i]);
		ret = network_dump_ai_outbuf(proc_id, &tmp_buf);
		if (HD_OK != ret) {
			printf("proc_id(%lu) dump in buf fail !!\n", proc_id);
			goto exit;
		}
	}

	for (i = 0; i < p_stream->input_blob_num; i++) {
		NET_IN* p_in = g_in + i;

		/* load input bin */
		ret = ai_test_input_pull_buf((p_stream->in_path + i), &in_buf, 0);
		if (HD_OK != ret) {
			printf("in_path(%lu) pull input fail !!\n", (p_stream->in_path + i));
			goto exit;
		}

		src_img_size = in_buf.size;
		tmp_src_va = in_buf.va; // backup in_buf addr
		tmp_src_pa = in_buf.pa;

		if (in_buf.batch_num > in_buf_cnt) {
			in_buf.batch_num = in_buf_cnt;
		}
		/* fill va, pa, then set input */
		printf("set_buf: batch_num = %lu\n", set_batch_num[proc_id][i]);
		for(j = 0; j < set_batch_num[proc_id][i]; j++) {
			if (j > 0) {
				if (p_in->in_cfg.is_comb_img == 0) { // no combine image, need to use the same image as input.
					UINTPTR dst_va = in_buf.va + src_img_size;
					memcpy((VOID*)dst_va, (VOID*)in_buf.va, src_img_size);
					hd_common_mem_flush_cache((VOID *)dst_va, src_img_size);
					in_buf.pa += src_img_size;
					in_buf.va += src_img_size;
				} else {
					in_buf.pa += src_img_size;
					in_buf.va += src_img_size;
				}
			}

			printf(" path_%d(0x%lx) pa(0x%lx) va(0x%lx) size(%lu) whcb(%lu,%lu,%lu,%lu)\n",
					(int)idx, in_path_list[idx], in_buf.pa, in_buf.va, in_buf.size, in_buf.width, in_buf.height, in_buf.channel, in_buf.batch_num);
			ret = vendor_ai_net_set(proc_id, in_path_list[idx], &in_buf);
			if (HD_OK != ret) {
				printf("proc_id(%lu)push input fail !! i(%lu)\n", proc_id, i);
				goto exit;
			}
			idx++;
		}
		idx = idx + p_in->in_cfg.b - set_batch_num[proc_id][i];
		in_buf.va = tmp_src_va; // reduction in_buf addr
		in_buf.pa = tmp_src_pa;
	}

exit:
	/* free in_path_list */
	if (in_path_list)
		free(in_path_list);

	return ret;
}

static VOID *network_user_thread(VOID *arg)
{
	HD_RESULT ret = HD_OK;
	UINT32 i = 0;
	UINT32 proc_id;
	UINT64 ts, ts_new;
	UINT64 ts_proc;
	
	
	TEST_NETWORK *p_stream = (TEST_NETWORK*)arg;

	printf("\r\n");
	while (p_stream->proc_start == 0) sleep(1);

	proc_id = p_stream->net_path;

	printf("\r\n");
	
	if (p_stream->auto_test)
		printf("!!## auto_test ready ##!!\n");
	
	while (p_stream->proc_exit == 0) {

		if (p_stream->proc_oneshot) {

			p_stream->proc_oneshot = 0;
			printf("proc_id(%lu) oneshot ...\n", proc_id);

			// test proc id
			if (g_test_proc_id.enable == TRUE) {
				proc_id = g_test_proc_id.proc_id;
			}

			if (p_stream->auto_test)
				printf("!!## auto_test trigger ##!!\n");

			/*
			VENDOR_AI_BUF	in_buf = {0};
			VENDOR_AI_BUF	in_buf2 = {0};
			//VENDOR_AI_BUF	out_buf = {0};

			ret = ai_test_input_pull_buf(p_stream->in_path, &in_buf, 0);
			if (HD_OK != ret) {
				printf("proc_id(%lu) pull input fail !!\n", p_stream->in_path);
				if (g_test_proc_id.enable == TRUE) {
					// go through for proc_id test
				} else {
					goto skip;
				}
			}
				
			// set input image
			ret = vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_IN(0, 0), &in_buf);
			if (HD_OK != ret) {
				printf("proc_id(%lu)push input fail !!\n", proc_id);
				if (g_test_proc_id.enable == TRUE) {
					// go through for proc_id test
				} else {
					goto skip;
				}
			}
			*/

			// set buf by in_path_list
			ret = set_buf_by_in_path_list(p_stream);
			if (HD_OK != ret) {
				printf("proc_id(%lu) set in_buf fail(%d) !!\n", p_stream->net_path, ret);
				goto skip;
			}

			// do net proc
			ts = hd_gettime_us();
			ret = vendor_ai_net_proc(proc_id);
			ts_new = hd_gettime_us();
			ts_proc = ts_new-ts;
			net_proc_time[i] = ts_proc;
			i = (i == 6) ? 1 : (i+1);
			
			if (HD_OK != ret) {
				printf("proc_id(%lu) proc fail !!\n", proc_id);
				goto skip;
			}

			if (p_stream->proc_oneshot == 0) { //single output buffer
				printf("proc_id(%lu) oneshot done!\n", proc_id);
				
				if (net_info.out_buf_cnt == 1) {
					if (net_info.out_buf_type == MAKEFOURCC('A','C','0','1')) { //classify 01
					
						VENDOR_AI_POSTPROC_RESULT_INFO out_buf = {0};
						// get output result
						ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT(VENDOR_AI_MAXLAYER, 0), &out_buf);
						if (HD_OK != ret) {
							printf("proc_id(%lu) output get fail !!\n", p_stream->net_path);
							goto skip;
						}

						ret = ai_test_network_dump_postproc_buf(p_stream->net_path, &out_buf);
						if (HD_OK != ret) {
							printf("proc_id(%lu) output dump fail !!\n", p_stream->net_path);
							goto skip;
						}
					} else if (net_info.out_buf_type == MAKEFOURCC('A','D','0','1')) { //detection 01
					
						VENDOR_AI_DETOUT_RESULT_INFO out_buf = {0};
						// get output result
						ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT(VENDOR_AI_MAXLAYER, 0), &out_buf);
						if (HD_OK != ret) {
							printf("proc_id(%lu) output get fail !!\n", p_stream->net_path);
							goto skip;
						}

						ret = ai_test_network_dump_detout_buf(p_stream->net_path, &out_buf);
						if (HD_OK != ret) {
							printf("proc_id(%lu) output dump fail !!\n", p_stream->net_path);
							goto skip;
						}
					} else {
						printf("TODO: proc_id(%lu) output dump 1 AI_BUF...\n", p_stream->net_path);
					}
				} else { //multiple output buffer
					printf("TODO: proc_id(%lu) output dump N AI_BUF...\n", p_stream->net_path);
				}
			}
			
			if (p_stream->auto_test)
				printf("!!## auto_test done ##!!\n");
		}
		usleep(100);
	}
	
	if (p_stream->auto_test)
		printf("!!## auto_test teardown ##!!\n");
	
skip:
	
	return 0;
}
