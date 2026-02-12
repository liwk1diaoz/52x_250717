/**
	@brief Source file of kflow_ai_net.

	@file kflow_ai_core.c

	@ingroup kflow_ai_net

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/

#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/file.h"
#include "kwrap/cpu.h"

//#include "ai_ioctl.h"
#include "kdrv_ai.h"
#include "kflow_ai_net/nn_net.h"
#include "kflow_ai_net/nn_parm.h"
#include "kflow_ai_net/kflow_ai_core_callback.h"
#include "kflow_ai_net/kflow_ai_net_comm.h"

//=============================================================
#define __CLASS__ 				"[ai][kflow][dsp]"
#include "kflow_ai_debug.h"
//=============================================================
#include "kflow_dsp/kflow_dsp.h"
#include "kflow_dsp/kflow_dsp_platform.h"

#define SHOW_FLOW 0
#define DUMMY 0
#define ENG_NAME "DSP"

static KFLOW_AI_CHANNEL_CTX kflow_dsp_ch1;
//static INT32 kflow_dsp_ch1_isrcb(UINT32 handle, UINT32 intstatus, UINT32 eng, void *parm);
static char output_path[STR_MAX_LENGTH] = DBG_OUT_PATH;

int kflow_dsp_set_output_path(char *path)
{
	if (path == NULL) {
		return -1;
	}
	snprintf(output_path, STR_MAX_LENGTH-1, path);
	return 0;
}
EXPORT_SYMBOL(kflow_dsp_set_output_path);

static int _kflow_dsp_write_file(char *filename, void *data, size_t count, unsigned long long *offset)
{
	int ret = 0;
	int fd;

	fd = vos_file_open(filename, O_RDWR|O_CREAT, 0666);

	if ((VOS_FILE)(-1) == fd) {
		DBG_DUMP("%s fails to open file %s\n", __func__, filename);
		ret = -1;
		goto exit;
	}

	ret = vos_file_write(fd, (void *)data, count);

	if (ret <= 0) {
		DBG_DUMP("%s: Fail to write file %s(errno = %d)!\n", __func__, filename, ret);
		ret = -1;
		goto exit;
	}

exit:
	if ((VOS_FILE)(-1) != fd)
		vos_file_close(fd);

	return ret;
}
	
static void _kflow_dsp_dump_out_buf(KFLOW_AI_CHANNEL_CTX *p_ch, KFLOW_AI_JOB* p_job)
{
	char filename[STR_MAX_LENGTH] = {0};
#if !CNN_25_MATLAB
	NN_DATA *p_sao;
	UINT32 omem_cnt;
	NN_GEN_MODE_CTRL *p_mctrl = 0;
#else
	NN_IOMEM *p_io_mem = 0;
#endif
	UINT32 out_id = 0;

	if (p_job == NULL) {
		DBG_DUMP("%s:%d p_job is NULL\n", __func__, __LINE__);
		return;
	}
	
	DBG_DUMP("kflow_ai - dump buf <%s> proc_id(%d) job_id(%d)\n", p_ch->name, p_job->proc_id, p_job->job_id);
#if CNN_25_MATLAB
	p_io_mem = (NN_IOMEM *)p_job->p_io_info;
#else
	p_mctrl = (NN_GEN_MODE_CTRL*)(p_job->p_op_info);
#endif

	if (p_job->p_io_info == NULL) {
		DBG_DUMP("%s:%d p_job->p_io_info is NULL\n", __func__, __LINE__);
		return;
	}
	if (p_job->p_op_info == NULL) {
		DBG_DUMP("%s:%d p_job->p_op_info is NULL\n", __func__, __LINE__);
		return;
	}

#if CNN_25_MATLAB
	for (out_id = 0; out_id < 3; out_id++) {
		if (p_io_mem->SAO[out_id].size > 0) {
			snprintf(filename, STR_MAX_LENGTH-1, "%s/%s_%u_OUT%u.bin", output_path, ENG_NAME, (unsigned int)p_job->job_id, (unsigned int)out_id);
			DBG_DUMP("write file: %s\n", filename);
			vos_cpu_dcache_sync(p_sao[out_id].va, p_sao[out_id].size, VOS_DMA_FROM_DEVICE);
			_kflow_dsp_write_file(filename, (void *)p_io_mem->SAO[out_id].va, p_io_mem->SAO[out_id].size, 0);
		}
	}
#else
	omem_cnt = p_mctrl->iomem.omem_cnt;
	p_sao    = (NN_DATA*)p_mctrl->iomem.omem_addr;

	for (out_id = 0; out_id < omem_cnt; out_id++) {
		if (p_sao[out_id].size > 0) {
			snprintf(filename, STR_MAX_LENGTH-1, "%s/%s_%u_OUT%u.bin", output_path, ENG_NAME, (unsigned int)p_job->job_id, (unsigned int)out_id);
			DBG_DUMP("buf info: va(0x%x), pa(0x%x) size(0x%x)\n", p_sao[out_id].va, p_sao[out_id].pa, p_sao[out_id].size);
			DBG_DUMP("write file: %s\n\n\n", filename);
			vos_cpu_dcache_sync(p_sao[out_id].va, p_sao[out_id].size, VOS_DMA_FROM_DEVICE);
			_kflow_dsp_write_file(filename, (void *)p_sao[out_id].va, p_sao[out_id].size, 0);
		}
	}
#endif
}

static void kflow_dsp_ch1_open(KFLOW_AI_CHANNEL_CTX* p_ch)
{
	//register kflow_dsp_ch1_isrcb
	INIT_LIST_HEAD(p_ch->ready_queue);
}

static void kflow_dsp_ch1_close(KFLOW_AI_CHANNEL_CTX* p_ch)
{
}

static void (*g_dsp_exec_fp)(KFLOW_AI_JOB* p_job) = 0;

void kflow_dsp_reg_exec_cb(void (*fp)(KFLOW_AI_JOB* p_job))
{
	g_dsp_exec_fp = fp;
}
EXPORT_SYMBOL(kflow_dsp_reg_exec_cb);

static void kflow_dsp_ch1_trig(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job)
{
#if (SHOW_FLOW == 1)
	NN_GEN_MODE_CTRL *p_mctrl = (NN_GEN_MODE_CTRL*)(p_job->p_op_info);
	{
		DBG_DUMP("----- dsp trig: job=%d, layer=%d, eng=%d, mode=%d, trig=%d, cnt=%d\r\n",
			p_job->job_id, p_mctrl->layer_index, p_mctrl->eng, p_mctrl->mode, p_mctrl->trig_src, p_mctrl->tot_trig_eng_times);
	}
#endif
#if (DUMMY == 1)
	p_job->exec_cnt++;
	if (p_ch->onfinish != 0) {
		p_ch->onfinish(p_ch, p_ch->run_job, 0);
		return;
	}
#endif
	if (g_dsp_exec_fp)
		g_dsp_exec_fp(p_job);

#if (SHOW_FLOW == 1)
		{
			KFLOW_AI_CHANNEL_CTX* p_ch = 0;
			p_ch = p_job->p_ch;
			DBG_DUMP("- proc[%d].job[%d] done by engine[%s] ch[%s]\r\n", 
				(int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_ch->name);
		}
#endif
	p_job->exec_cnt++;
	if (p_ch->onfinish != 0) {
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
		p_ch->onfinish(p_ch, p_ch->run_job, 0, 0);
#else
		p_ch->onfinish(p_ch, p_ch->run_job, 0);
#endif			
		return;
	}
}

static void kflow_dsp_ch1_reset(KFLOW_AI_CHANNEL_CTX* p_ch)
{
	p_ch->run_job->state = 13; //force stop
	if (g_dsp_exec_fp) {
		g_dsp_exec_fp(p_ch->run_job);
	}
}

void kflow_dsp_ch1_reset_path(KFLOW_AI_CHANNEL_CTX* p_ch, UINT32 proc_id)
{
	if(p_ch->run_job) {
		if(p_ch->run_job->proc_id == proc_id) {
			p_ch->run_job->state = 13; //force abort
			if (g_dsp_exec_fp) {
				g_dsp_exec_fp(p_ch->run_job);
			}
		}
	}
}

static void kflow_dsp_ch1_debug(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job, UINT32 info)
{
	/* dump output buffer */
	if (info & KFLOW_AI_DBG_OBUF) {
		_kflow_dsp_dump_out_buf(p_ch, p_job);
	}
}


#if 0
static INT32 kflow_dsp_ch1_isrcb(UINT32 handle, UINT32 intstatus, UINT32 eng, void *parm)
{
	//如果 p_job 某後一個 p_next_job->p_eng 不是 dsp 的 => 相關的 output buffer 要 flush cache

	KFLOW_AI_CHANNEL_CTX* p_ch = &kflow_dsp_ch1;
	if (p_ch->onfinish != 0)
		p_ch->onfinish(p_ch, p_ch->run_job);
	return 0;
}
#endif

static LIST_HEAD kflow_dsp1_rq = {0};
static KFLOW_AI_CHANNEL_CTX kflow_dsp_ch1 =
{
	.name = "dsp1",
	.ready_queue = &kflow_dsp1_rq,
	.run_job = 0,
	.open = kflow_dsp_ch1_open,
	.close = kflow_dsp_ch1_close,
	.trigger = kflow_dsp_ch1_trig,
	.reset = kflow_dsp_ch1_reset,
	.debug = kflow_dsp_ch1_debug,
};

#define MAX_CHN 1
static LIST_HEAD kflow_dsp_wq = {0};
static KFLOW_AI_CHANNEL_CTX* kflow_dsp_ch[MAX_CHN] = {&kflow_dsp_ch1};

static void kflow_dsp_init(KFLOW_AI_ENGINE_CTX* p_eng)
{
	INIT_LIST_HEAD(p_eng->wait_queue);
}

static void kflow_dsp_uninit(KFLOW_AI_ENGINE_CTX* p_eng)
{
}

static UINT32 kflow_dsp_getcfg(KFLOW_AI_ENGINE_CTX* p_eng, KFLOW_AI_JOB* p_job, UINT32 cfg)
{
	UINT32 r = 0;
	switch(cfg) {
	default:
		break;
	}
	return r;
}

static KFLOW_AI_ENGINE_CTX kflow_dsp_eng =
{
	.name = "dsp",
	.wait_queue = &kflow_dsp_wq,
	.channel_max = 0xff,
	.channel_count = MAX_CHN,
	.p_ch = kflow_dsp_ch,
	.attr = 1, //slow
	.init = kflow_dsp_init,
	.uninit = kflow_dsp_uninit,
	.getcfg = kflow_dsp_getcfg,
};

KFLOW_AI_ENGINE_CTX* kflow_dsp_get_engine(void)
{
	return &kflow_dsp_eng;
}

EXPORT_SYMBOL(kflow_dsp_get_engine);

