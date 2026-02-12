/**
	@brief Source file of vendor ai net sample code.

	@file ai_net_with_buf.c

	@ingroup ai_net_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_type.h"
#include "hd_debug.h"
#include "vendor_ai.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_cpu_postproc.h"
#include <arm_neon.h>
#include <pdcnn_lib_ai2.h>
#include <sys/time.h>

// platform dependent
#if defined(__LINUX)
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()
#else
#include <FreeRTOS_POSIX.h>	
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(ai_net_with_buf, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif
#define AI_POST_PROC	1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////

#define NET_PATH_ID		UINT32

#define VENDOR_AI_CFG  				0x000f0000  //vendor ai config

#define NET_VDO_SIZE_W	1920 //max for net
#define NET_VDO_SIZE_H	1080 //max for net

#define SAVE_SCALE		DISABLE
#define PROF			ENABLE
#if PROF
	static struct timeval tstart, tend;
	#define PROF_START()    gettimeofday(&tstart, NULL);
	#define PROF_END(msg)   gettimeofday(&tend, NULL);  \
			printf("%s time (us): %lu\r\n", msg,         \
					(tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec));
#else
	#define PROF_START()
	#define PROF_END(msg)
#endif


///////////////////////////////////////////////////////////////////////////////

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/

typedef struct _NET_PROC {
	CHAR model_filename[256];
	INT32 binsize;
	int job_method;
	int job_wait_ms;
	int buf_method;
	UINT32 proc_id;

} NET_PROC;

typedef struct _NET_IN {
	CHAR input_filename[256];
	UINT32 w;
	UINT32 h;
	UINT32 c;
	UINT32 loff;
	UINT32 fmt;
	VENDOR_AI_BUF src_img;
} NET_IN;

/*-----------------------------------------------------------------------------*/
/* Global Functions                                                             */
/*-----------------------------------------------------------------------------*/

static HD_RESULT mem_get(MEM_PARM *mem_parm, UINT32 size, UINT32 id)
{
	HD_RESULT ret = HD_OK;
	UINT32 pa   = 0;
	void  *va   = NULL;
	HD_COMMON_MEM_VB_BLK      blk;
	
	blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_DEFINIED_POOL + id, size, DDR_ID0);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		printf("hd_common_mem_get_block fail\r\n");
		return HD_ERR_NG;
	}
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		printf("not get buffer, pa=%08x\r\n", (int)pa);
		return HD_ERR_NOMEM;
	}
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, size);

	/* Release buffer */
	if (va == 0) {
		ret = hd_common_mem_munmap(va, size);
		if (ret != HD_OK) {
			printf("mem unmap fail\r\n");
			return ret;
		}
	}

	mem_parm->pa = pa;
	mem_parm->va = (UINT32)va;
	mem_parm->size = size;
	mem_parm->blk = blk;

	return HD_OK;
}

static HD_RESULT mem_rel(MEM_PARM *mem_parm)
{
	HD_RESULT ret = HD_OK;

	/* Release in buffer */
	if (mem_parm->va) {
		ret = hd_common_mem_munmap((void *)mem_parm->va, mem_parm->size);
		if (ret != HD_OK) {
			printf("mem_uninit : (g_mem.va)hd_common_mem_munmap fail.\r\n");
			return ret;
		}
	}
	
	ret = hd_common_mem_release_block(mem_parm->blk);
	if (ret != HD_OK) {
		printf("mem_uninit : (g_mem.pa)hd_common_mem_release_block fail.\r\n");
		return ret;
	}

	mem_parm->pa = 0;
	mem_parm->va = 0;
	mem_parm->size = 0;
	mem_parm->blk = (UINT32)-1;
	return HD_OK;
}

BOOL need_ise_resize(NET_IN nn_in, INT32* size)
{
	if (nn_in.w < (UINT32)size[0] || nn_in.h < (UINT32)size[1])
		return TRUE;
	else if (nn_in.w > MAX_FRAME_WIDTH || nn_in.h > MAX_FRAME_HEIGHT)
		return TRUE;
	else if (MAX_DISTANCE_MODE)
		return TRUE;
	else
		return FALSE;
}

static INT32 mem_load(MEM_PARM *mem_parm, const CHAR *filename)
{
	FILE *fd;
	INT32 size = 0;

	fd = fopen(filename, "rb");

	if (!fd) {
		printf("cannot read %s\r\n", filename);
		return -1;
	}

	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	// check "ai_in_buf" enough or not
	if (mem_parm->size < (UINT32)size) {
		printf("ERROR: ai_in_buf(%u) is not enough, input file(%u)\r\n", mem_parm->size, (UINT32)size);
		size = -1;
		goto exit;
	}
	
	if (size < 0) {
		printf("getting %s size failed\r\n", filename);
	} else if ((INT32)fread((VOID *)mem_parm->va, 1, size, fd) != size) {
		printf("read size < %ld\r\n", size);
		size = -1;
	}
	//mem_parm->size = size;
exit:
	if (fd) {
		fclose(fd);
	}

	return size;
}

/*-----------------------------------------------------------------------------*/
/* Input Functions                                                             */
/*-----------------------------------------------------------------------------*/

///////////////////////////////////////////////////////////////////////////////


static HD_RESULT input_open(NET_IN *p_nn_in, PDCNN_MEM *pdcnn_mem)
{
	HD_RESULT ret = HD_OK;



	UINT32 file_len = mem_load(&(pdcnn_mem->input_mem), p_nn_in->input_filename);
	if (file_len < 0) {
		printf("load buf(%s) fail\r\n", p_nn_in->input_filename);
		return HD_ERR_NG;
	}
	printf("load buf(%s) ok\r\n", p_nn_in->input_filename);
	hd_common_mem_flush_cache((VOID *)pdcnn_mem->input_mem.va, file_len);

	p_nn_in->src_img.width 		= p_nn_in->w;
	p_nn_in->src_img.height 	= p_nn_in->h;
	p_nn_in->src_img.channel 	= p_nn_in->c;
	p_nn_in->src_img.line_ofs 	= p_nn_in->loff;
	p_nn_in->src_img.fmt      	= p_nn_in->fmt;
	p_nn_in->src_img.pa      	= pdcnn_mem->input_mem.pa;
	p_nn_in->src_img.va   		= pdcnn_mem->input_mem.va;
	p_nn_in->src_img.sign 		= MAKEFOURCC('A','B','U','F');
	p_nn_in->src_img.size 		= p_nn_in->loff * p_nn_in->h * 3 / 2;

	return ret;
}


/*-----------------------------------------------------------------------------*/
/* Network Functions                                                             */
/*-----------------------------------------------------------------------------*/


//static NET_PROC g_net[16] = {0};

static INT32 _getsize_model(char* filename)
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

	return bin_size;
}

UINT32 _load_model(CHAR *filename, MEM_PARM *mem_parm)
{
	FILE *fd;
	INT32 size = 0;
	fd = fopen(filename, "rb");
	if (!fd) {
		printf("cannot read %s\r\n", filename);
		return -1;
	}

	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	
	if (size < 0) {
		printf("getting %s size failed\r\n", filename);
	} else if ((INT32)fread((VOID *)mem_parm->va, 1, size, fd) != size) {
		printf("read size < %ld\r\n", size);
		size = -1;
	};
	//mem_parm->size = size;
	
	if (fd) {
		fclose(fd);
	};
	printf("model buf size: %d\r\n", size);
	return size;
}

static HD_RESULT network_open(NET_PROC *p_net, PDCNN_MEM *pdcnn_mem)
{
	HD_RESULT ret = HD_OK;

	if (strlen(p_net->model_filename) == 0) {
		printf("proc_id(%u) input model is null\r\n", p_net->proc_id);
		return 0;
	}
	// set model
	vendor_ai_net_set(p_net->proc_id, VENDOR_AI_NET_PARAM_CFG_MODEL, (VENDOR_AI_NET_CFG_MODEL*)&pdcnn_mem->model_mem);
	// open
	vendor_ai_net_open(p_net->proc_id);
	
	return ret;
}

static HD_RESULT network_close(NET_PROC *p_net, MEM_PARM *all_mem)
{
	HD_RESULT ret = HD_OK;

	// close
	ret = vendor_ai_net_close(p_net->proc_id);
	
	mem_rel(all_mem);

	return ret;
}

static HD_RESULT network_get_layer0_info(UINT32 proc_id)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_BUF p_inbuf = {0};
	VENDOR_AI_BUF p_outbuf = {0};
		
	// get layer0 in buf
	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_IN(0, 0), &p_inbuf);
	if (HD_OK != ret) {
		printf("proc_id(%u) get layer0 inbuf fail !!\n", proc_id);
		return ret;
	}
	
	// get layer0 in buf
	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT(0, 0), &p_outbuf);
	if (HD_OK != ret) {
		printf("proc_id(%u) get layer0 outbuf fail !!\n", proc_id);
		return ret;
	}
	
	printf("dump layer0 info:\n");
	printf("   channel(%lu)\n", p_inbuf.channel);
	printf("   fmt(0x%lx)\n", p_inbuf.fmt);
	printf("   width(%lu)\n", p_outbuf.width);
	printf("   height(%lu)\n", p_outbuf.height);
	printf("   channel(%lu)\n", p_outbuf.channel);
	printf("   batch_num(%lu)\n", p_outbuf.batch_num);
	printf("   fmt(0x%lx)\n", p_outbuf.fmt);
	printf("\n");

	return ret;
}


static VOID *nn_thread_api(VOID *arg)
{
	HD_RESULT ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
	UINT32 proc_id = *(UINT32*)arg;
	UINT32 req_mem_size = 0;
	HD_GFX_IMG_BUF gfx_img = {0};
	VENDOR_AI_BUF	p_src_img = {0};
	//network params
	NET_PROC net_info;
	net_info.proc_id = proc_id;
	sprintf(net_info.model_filename, "/mnt/sd/CNNLib/para/pdcnn_v4_ll/nvt_model.bin");
	net_info.binsize		= _getsize_model(net_info.model_filename);
	net_info.job_method     = VENDOR_AI_NET_JOB_OPT_LINEAR_O1;
	net_info.job_wait_ms    = 0;
	net_info.buf_method     = VENDOR_AI_NET_BUF_OPT_NONE;
	if (net_info.binsize <= 0) {
		printf("proc_id(%u) input model is not exist?\r\n", proc_id);
		goto exit_thread;
	}
	printf("proc_id(%u) set net_info: model-file(%s), binsize=%d\r\n", proc_id, net_info.model_filename, net_info.binsize);

	//mem_cfg set
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_USER_DEFINIED_POOL;
	mem_cfg.pool_info[0].blk_size = PD_MAX_MEM_SIZE;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_USER_DEFINIED_POOL;
	mem_cfg.pool_info[1].blk_size = SCALE_BUF_SIZE;
	mem_cfg.pool_info[1].blk_cnt = 1;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
		goto exit_thread;
	}
	
	//ai cfg set
	UINT32 schd = VENDOR_AI_PROC_SCHD_FAIR;
	vendor_ai_cfg_set(VENDOR_AI_CFG_PLUGIN_ENGINE, vendor_ai_cpu1_get_engine());
	vendor_ai_cfg_set(VENDOR_AI_CFG_PROC_SCHD, &schd);
	ret = vendor_ai_init();
	if (ret != HD_OK) {
		printf("vendor_ai_init fail=%d\n", ret);
		goto exit_thread;
	}
	//set buf option

	VENDOR_AI_NET_CFG_BUF_OPT cfg_buf_opt = {0};
	cfg_buf_opt.method = net_info.buf_method;
	cfg_buf_opt.ddr_id = DDR_ID0;
	vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_BUF_OPT, &cfg_buf_opt);
	
	// set job option
	VENDOR_AI_NET_CFG_JOB_OPT cfg_job_opt = {0};
	cfg_job_opt.method = net_info.job_method;
	cfg_job_opt.wait_ms = net_info.job_wait_ms;
	cfg_job_opt.schd_parm = VENDOR_AI_FAIR_CORE_ALL; //FAIR dispatch to ALL core
	vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_JOB_OPT, &cfg_job_opt);
	
	MEM_PARM allbuf = {0}, rel_buf = {0};
	PDCNN_MEM pdcnn_mem;
	ret = mem_get(&allbuf, (UINT32)PD_MAX_MEM_SIZE, proc_id);
	if(ret != HD_OK){
		printf("pdcnn all mem get fail (%d)!!\r\n", ret);
		goto exit_thread;
	}
	rel_buf = allbuf;
	MEM_PARM scale_buf = {0};
	ret = mem_get(&scale_buf, (UINT32)SCALE_BUF_SIZE, proc_id);
	if(ret != HD_OK){
		printf("scale mem get fail (%d)!!\r\n", ret);
		goto exit_thread;
	}

	ret = get_pd_mem(&allbuf, &(pdcnn_mem.model_mem), net_info.binsize, 32);
	if(ret != HD_OK){
		printf("get model mem fail (%d)!!\r\n", ret);
		goto exit_thread;
	}
	
	UINT32 modelsize = _load_model(net_info.model_filename, &(pdcnn_mem.model_mem));
	if (modelsize <= 0) {
		printf("proc_id(%u) input model load fail: %s\r\n", proc_id, net_info.model_filename);
		return 0;
	}

	pdcnn_get_version();
#if 1
	ret = pdcnn_version_check(&(pdcnn_mem.model_mem));
	if(ret != HD_OK){
		printf("pdcnn version check fail (%d)!!\r\n", ret);
		goto exit_thread;
	}
#endif
	// open network(model, get network mem)
	if ((ret = network_open(&net_info, &pdcnn_mem)) != HD_OK) {
		printf("proc_id(%u  nn open fail !!\n", proc_id);
		goto exit_thread;
	}

	//set work buf and assign pdcnn mem
	VENDOR_AI_NET_CFG_WORKBUF wbuf = {0};
	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("proc_id(%lu) get VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", proc_id);
		goto exit_thread;
	}
	printf("work buf size: %ld\r\n", wbuf.size);

	ret = get_pd_mem(&allbuf, &(pdcnn_mem.io_mem), wbuf.size, 32);
	if(ret != HD_OK){
		printf("get io_mem fail (%d)!!\r\n", ret);
		goto exit_thread;
	}

#if MAX_DISTANCE_MODE
	ret = get_pd_mem(&allbuf, &(pdcnn_mem.scale_buf), (YUV_WIDTH * YUV_HEIGHT * 3 / 2), 32);
	if(ret != HD_OK){
		printf("get scale_buf fail (%d)!!\r\n", ret);
		goto exit_thread;
	}
#endif

	ret = vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &(pdcnn_mem.io_mem));
	if (ret != HD_OK) {
		printf("proc_id(%lu) set VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", proc_id);
		goto exit_thread;
	}

	ret = get_pd_mem(&allbuf, &(pdcnn_mem.input_mem), MAX_FRAME_WIDTH * MAX_FRAME_HEIGHT * 3 / 2, 32);
	if(ret != HD_OK){
		printf("get input YUV mem fail (%d)!!\r\n", ret);
		goto exit_thread;
	}


	ret = get_post_mem(&allbuf, &pdcnn_mem);
	if(ret != HD_OK){
		printf("get pdcnn postprocess mem fail (%d)!!\r\n", ret);
		goto exit_thread;
	}

	req_mem_size = PD_MAX_MEM_SIZE - allbuf.size;
	printf("AI turnkey allocate total of buf size: (%ld), use total of buf size: (%ld)!\r\n", PD_MAX_MEM_SIZE, req_mem_size);

	// start network
	ret = vendor_ai_net_start(proc_id);
	if (HD_OK != ret) {
		printf("proc_id(%u) nn start fail !!\n", proc_id);
		goto exit_thread;
	}

	//get nn layer 0 info
	network_get_layer0_info(proc_id);

	//image parameter
	NET_IN nn_in;
	nn_in.c = 2,
	nn_in.fmt = HD_VIDEO_PXLFMT_YUV420;

	CHAR para_file[] = "/mnt/sd/CNNLib/para/pdcnn_v4_ll/para.txt";
	PROPOSAL_PARAM proposal_params;
	FLOAT score_thr = 0.45, nms_thr = 0.2;
    LIMIT_PARAM limit_param;
	//PDCNN_MEM* pdcnn_mem;
	proposal_params.score_thres = score_thr;
	proposal_params.nms_thres = nms_thr;
	proposal_params.run_id = proc_id;
	proposal_params.ratiow = (FLOAT)nn_in.w / (FLOAT)YUV_WIDTH;
	proposal_params.ratioh = (FLOAT)nn_in.h / (FLOAT)YUV_HEIGHT;
	//pdcnn_mem_init(&pdcnn_mem);
#if MAX_DISTANCE_MODE
	limit_param.max_distance = 1;
	limit_param.sm_thr_num = 2;
#else
	limit_param.max_distance = 0;
	limit_param.sm_thr_num = 6;
#endif

	
	BACKBONE_OUT* backbone_outputs = (BACKBONE_OUT*)pdcnn_mem.backbone_output.va;
	pdcnn_init(&proposal_params, backbone_outputs, &limit_param, para_file);

	UINT32 all_time = 0;
	INT32 img_num = 0;
#if SAVE_SCALE
	CHAR BMP_FILE[256];
#endif
	CHAR IMG_PATH[256];
	CHAR SAVE_TXT[256];
	CHAR IMG_LIST[256];
	CHAR list_infor[256];
	CHAR *line_infor;
	BOOL INPUT_STATE = TRUE;

	sprintf(IMG_LIST, "/mnt/sd/jpg/pd_image_list.txt");
	sprintf(IMG_PATH, "/mnt/sd/jpg/PD");

	FILE *fs, *fr;

    sprintf(SAVE_TXT, "/mnt/sd/det_results/pd_test_results.txt");
	
	fr = fopen(IMG_LIST, "r");
	fs = fopen(SAVE_TXT, "w+");

	INT32 len = 0;
	CHAR img_name[256]={0};
	CHAR *token;
	INT32 sl = 0;
	BOOL iseresize = FALSE;

	if(NULL == fr)
	{
		printf("Failed to open img_list!\r\n");	
	} 
	while(fgets(list_infor, 256, fr) != NULL){
		len = strlen(list_infor);
		list_infor[len - 1] = '\0';
		sl = 0;
		line_infor = list_infor;

		while((token = strtok(line_infor, " ")) != NULL)
		{
			if(sl > 2){
				break;
			}
			if (sl == 0){
				strcpy(img_name, token);
				sprintf(nn_in.input_filename, "%s/%s", IMG_PATH, token);
				printf("%s ", token);
			}
			if (sl == 1){
				nn_in.w = atoi(token);
				nn_in.loff = ALIGN_CEIL_4(nn_in.w);
				printf("%s ", token);
			}
			if (sl == 2){
				nn_in.h = atoi(token);
				printf("%s\r\n", token);
			}
			line_infor = NULL;
			sl++;
		}

		if ((ret = input_open(&nn_in, &pdcnn_mem)) != HD_OK) {
			printf("proc_id(%u) input image open fail !!\n", proc_id);
			goto exit_thread;
		}

		proposal_params.ratiow = (FLOAT)nn_in.w / (FLOAT)proposal_params.input_size[0];
		proposal_params.ratioh = (FLOAT)nn_in.h / (FLOAT)proposal_params.input_size[1];

		iseresize = need_ise_resize(nn_in, proposal_params.input_size);
		if(iseresize)
		{		
#if MAX_DISTANCE_MODE
			gfx_img.dim.w = 1024;
			gfx_img.dim.h = 576;
			gfx_img.format = nn_in.src_img.fmt;
			gfx_img.lineoffset[0] = ALIGN_CEIL_4(1024);
			gfx_img.lineoffset[1] = ALIGN_CEIL_4(576);
			gfx_img.p_phy_addr[0] = scale_buf.pa;
			gfx_img.p_phy_addr[1] = scale_buf.pa + 1024 * 576;
#else
			gfx_img.dim.w = proposal_params.input_size[0];
			gfx_img.dim.h = proposal_params.input_size[1];
			gfx_img.format = nn_in.src_img.fmt;
			gfx_img.lineoffset[0] = ALIGN_CEIL_4(proposal_params.input_size[0]);
			gfx_img.lineoffset[1] = ALIGN_CEIL_4(proposal_params.input_size[0]);
			gfx_img.p_phy_addr[0] = scale_buf.pa;
			gfx_img.p_phy_addr[1] = scale_buf.pa + proposal_params.input_size[0] * proposal_params.input_size[1];
			//printf("scale image width: %d, height: %d\r\n", gfx_img.dim.w, gfx_img.dim.h);
#endif
			PD_IRECT roi = {0, 0, nn_in.src_img.width, nn_in.src_img.height};
			ret = ai_crop_img(&gfx_img, &(nn_in.src_img), HD_GFX_SCALE_QUALITY_NULL, &roi);
			if (ret != HD_OK) {
				printf("ai_crop_img fail=%d\n", ret);
			}
			gfx_img_to_vendor(&p_src_img, &gfx_img, (UINT32)scale_buf.va);
#if SAVE_SCALE
			FILE *fb;
			sprintf(BMP_FILE, "/mnt/sd/save_bmp/%s_scale.bin", img_name);
			fb = fopen(BMP_FILE, "wb+");
			fwrite((UINT32 *)scale_buf.va, sizeof(UINT32), (gfx_img.dim.h * gfx_img.dim.w + gfx_img.dim.h * gfx_img.dim.w / 2), fb);
			fclose(fb);
#endif
		}
		else {
			p_src_img = nn_in.src_img;
		}

		PROF_START();

		ret = pdcnn_process(&proposal_params, &pdcnn_mem, &limit_param, backbone_outputs, &p_src_img, (UINT32)MAX_DISTANCE_MODE);

		if(ret != HD_OK){
			printf("pdcnn_process fail!\r\n");
			goto exit_thread;
		}
		PROF_END("PDCNN");
		all_time += (tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec); 

		PDCNN_RESULT *final_out_results = (PDCNN_RESULT*)pdcnn_mem.final_result.va;
		for(INT32 num = 0; num < pdcnn_mem.out_num; num++){
			FLOAT xmin = final_out_results[num].x1 * proposal_params.ratiow;
			FLOAT ymin = final_out_results[num].y1 * proposal_params.ratioh;
			FLOAT width = final_out_results[num].x2 * proposal_params.ratiow - xmin;
			FLOAT height = final_out_results[num].y2 * proposal_params.ratioh - ymin;
			FLOAT score = final_out_results[num].score;
			
			printf("obj information is: (socre: %f [%f %f %f %f])\r\n", score, xmin, ymin, width, height);
			fprintf(fs, "%s %f %f %f %f %f\r\n", img_name, score, xmin, ymin, width, height);
		}

		
		img_num++;

	}
	if (INPUT_STATE == TRUE){
    	printf("all test img number is: %d\r\n", img_num);
	}
	fclose(fs);
	fclose(fr);
	//printf("mean_time: %d\r\n", all_time / img_num);
	printf("mean_time: %d\r\n", all_time / img_num);
	
exit_thread:

	ret = vendor_ai_net_stop(proc_id);
	if (HD_OK != ret) {
		printf("proc_id(%u) nn stop fail !!\n", proc_id);
	}

	ret = mem_rel(&scale_buf);
	if(ret != HD_OK){
		printf("release scale buf fail!!\r\n");
	}
		
	// close network modules
	if ((ret = network_close(&net_info, &rel_buf)) != HD_OK) {
		printf("proc_id(%u) nn close fail !!\n", proc_id);
	}
	
	// uninit network modules
	ret = vendor_ai_uninit();
	if (ret != HD_OK) {
		printf("vendor_ai_uninit fail=%d\n", ret);
	}

	// uninit memory
	ret = hd_common_mem_uninit();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}
	
	return 0;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	UINT32 proc_id = 0;
	HD_RESULT ret;
	pthread_t nn_thread_id;

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("hd_common_init fail=%d\n", ret);
		goto exit;
	}
	// set project config for AI
	hd_common_sysconfig(0, (1<<16), 0, VENDOR_AI_CFG); //enable AI engine

	ret = hd_gfx_init();
	if (ret != HD_OK) {
		printf("hd_gfx_init fail\r\n");
		goto exit;
	}

	ret = pthread_create(&nn_thread_id, NULL, nn_thread_api, (VOID*)(&proc_id));
	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}
	
	pthread_join(nn_thread_id, NULL);

exit:
	ret = hd_gfx_uninit();
	if (ret != HD_OK) {
		printf("hd_gfx_uninit fail\r\n");
	}

	// uninit hdal
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
	}
	printf("test finish!!!\r\n");
	return ret;
}
