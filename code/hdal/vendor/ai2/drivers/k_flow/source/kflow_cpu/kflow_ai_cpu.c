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
#define __CLASS__ 				"[ai][kflow][cpu]"
#include "kflow_ai_debug.h"
//=============================================================
#include "kflow_cpu/kflow_cpu.h"
#include "kflow_cpu/kflow_cpu_platform.h"

#define SHOW_FLOW 0
#define DUMMY 0
#define ENG_NAME "CPU"
#define CLEAR_OUTBUF_BEFORE_TRIG 0
static KFLOW_AI_CHANNEL_CTX kflow_cpu_ch1;
//static INT32 kflow_cpu_ch1_isrcb(UINT32 handle, UINT32 intstatus, UINT32 eng, void *parm);
static char output_path[STR_MAX_LENGTH] = DBG_OUT_PATH;
#if (CLEAR_OUTBUF_BEFORE_TRIG == 1)
#define MAX_BUF_NUM 1000
extern BOOL outbuf_is_clear[MAX_BUF_NUM];
#endif

int kflow_cpu_set_output_path(char *path)
{
	if (path == NULL) {
		return -1;
	}
	snprintf(output_path, STR_MAX_LENGTH-1, path);
	return 0;
}
EXPORT_SYMBOL(kflow_cpu_set_output_path);

static int _kflow_cpu_write_file(char *filename, void *data, size_t count, unsigned long long *offset)
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

static void _kflow_cpu_dump_out_buf(KFLOW_AI_CHANNEL_CTX *p_ch, KFLOW_AI_JOB* p_job)
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
			_kflow_cpu_write_file(filename, (void *)p_io_mem->SAO[out_id].va, p_io_mem->SAO[out_id].size, 0);
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
			_kflow_cpu_write_file(filename, (void *)p_sao[out_id].va, p_sao[out_id].size, 0);
		}
	}
#endif
}

static void kflow_cpu_ch1_open(KFLOW_AI_CHANNEL_CTX* p_ch)
{
	//register kflow_cpu_ch1_isrcb
	INIT_LIST_HEAD(p_ch->ready_queue);
}

static void kflow_cpu_ch1_close(KFLOW_AI_CHANNEL_CTX* p_ch)
{
}

static void (*g_cpu_exec_fp)(KFLOW_AI_JOB* p_job) = 0;

void kflow_cpu_reg_exec_cb(void (*fp)(KFLOW_AI_JOB* p_job))
{
	g_cpu_exec_fp = fp;
}
EXPORT_SYMBOL(kflow_cpu_reg_exec_cb);

static void kflow_cpu_ch1_trig(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job)
{
#if (SHOW_FLOW == 1)
	NN_GEN_MODE_CTRL *p_mctrl = (NN_GEN_MODE_CTRL*)(p_job->p_op_info);
	{
		DBG_DUMP("----- cpu trig: job=%d, layer=%d, eng=%d, mode=%d, trig=%d, cnt=%d\r\n",
			p_job->job_id, p_mctrl->layer_index, p_mctrl->eng, p_mctrl->mode, p_mctrl->trig_src, p_mctrl->tot_trig_eng_times);
	}
#endif

#if (CLEAR_OUTBUF_BEFORE_TRIG == 1)
	NN_GEN_MODE_CTRL *p_mctrl = (NN_GEN_MODE_CTRL*)(p_job->p_op_info);
	NN_DATA *p_sao;
	UINT32 omem_cnt;
	UINT32 out_id = 0;
	omem_cnt = p_mctrl->iomem.omem_cnt;
	p_sao    = (NN_DATA*)p_mctrl->iomem.omem_addr;
	for (out_id = 0; out_id < omem_cnt; out_id++) {
		if (p_sao[out_id].size > 0) {
			if (outbuf_is_clear[OUT_BUF_INDEX(p_mctrl, out_id)] == 1) {
				continue;
			}
			memset((void *)p_sao[out_id].va,0,p_sao[out_id].size);
			vos_cpu_dcache_sync(p_sao[out_id].va, p_sao[out_id].size, VOS_DMA_TO_DEVICE);
			outbuf_is_clear[OUT_BUF_INDEX(p_mctrl, out_id)] = 1;
			DBG_IND("set outbuf_is_clear[%u] to 1\n",OUT_BUF_INDEX(p_mctrl, out_id));
		}
	}
#endif
#if (DUMMY == 1)
	p_job->exec_cnt++;
	if (p_ch->onfinish != 0) {
		p_ch->onfinish(p_ch, p_ch->run_job, 0);
		return;
	}
#endif
	if (g_cpu_exec_fp)
		g_cpu_exec_fp(p_job);

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

static void kflow_cpu_ch1_reset(KFLOW_AI_CHANNEL_CTX* p_ch)
{
	p_ch->run_job->state = 13; //force abort
	if (g_cpu_exec_fp) {
		g_cpu_exec_fp(p_ch->run_job);
	}
}

void kflow_cpu_ch1_reset_path(KFLOW_AI_CHANNEL_CTX* p_ch, UINT32 proc_id)
{
	if(p_ch->run_job) {
		if(p_ch->run_job->proc_id == proc_id) {
			p_ch->run_job->state = 13; //force abort
			if (g_cpu_exec_fp) {
				g_cpu_exec_fp(p_ch->run_job);
			}
		}
	}
}

static void kflow_cpu_ch1_debug(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job, UINT32 info)
{
	/* dump output buffer */
	if (info & KFLOW_AI_DBG_OBUF) {
		_kflow_cpu_dump_out_buf(p_ch, p_job);
	}
}

#if NN_DLI
static void kflow_cpu_ch1_start(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job)
{
	p_job->state = 0xff; //start

    //DBG_DUMP("- proc[%d].job[%d] start by engine[%s] ch[%s] *** engine_op[%08x]\r\n", 
    //    (int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_ch->name, p_job->engine_op);
    if ((p_job->engine_op & 0x3FF) >= 0x100) {
        // this is a DLI op
    	if (g_cpu_exec_fp) {
    		g_cpu_exec_fp(p_job);
    	}
    }
}

static void kflow_cpu_ch1_stop(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job)
{
	p_job->state = 0xfe; //stop
    //DBG_DUMP("- proc[%d].job[%d] start by engine[%s] ch[%s] *** engine_op[%08x]\r\n", 
    //    (int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_ch->name, p_job->engine_op);
    if ((p_job->engine_op & 0x3FF) >= 0x100) {
        // this is a DLI op
    	if (g_cpu_exec_fp) {
    		g_cpu_exec_fp(p_job);
    	}
    }
}
#endif

#if 0
static INT32 kflow_cpu_ch1_isrcb(UINT32 handle, UINT32 intstatus, UINT32 eng, void *parm)
{
	//如果 p_job 某後一個 p_next_job->p_eng 不是 cpu 的 => 相關的 output buffer 要 flush cache

	KFLOW_AI_CHANNEL_CTX* p_ch = &kflow_cpu_ch1;
	if (p_ch->onfinish != 0)
		p_ch->onfinish(p_ch, p_ch->run_job);
	return 0;
}
#endif

static LIST_HEAD kflow_cpu1_rq = {0};
static KFLOW_AI_CHANNEL_CTX kflow_cpu_ch1 =
{
	.name = "cpu1",
	.ready_queue = &kflow_cpu1_rq,
	.run_job = 0,
	.open = kflow_cpu_ch1_open,
	.close = kflow_cpu_ch1_close,
#if NN_DLI
	.start = kflow_cpu_ch1_start,
	.stop = kflow_cpu_ch1_stop,
#endif
	.trigger = kflow_cpu_ch1_trig,
	.reset = kflow_cpu_ch1_reset,
	.debug = kflow_cpu_ch1_debug,
};

#define MAX_CHN 1
static LIST_HEAD kflow_cpu_wq = {0};
static KFLOW_AI_CHANNEL_CTX* kflow_cpu_ch[MAX_CHN] = {&kflow_cpu_ch1};

static void kflow_cpu_init(KFLOW_AI_ENGINE_CTX* p_eng)
{
	INIT_LIST_HEAD(p_eng->wait_queue);
}

static void kflow_cpu_uninit(KFLOW_AI_ENGINE_CTX* p_eng)
{
}

static UINT32 kflow_cpu_getcfg(KFLOW_AI_ENGINE_CTX* p_eng, KFLOW_AI_JOB* p_job, UINT32 cfg)
{
	UINT32 r = 0;
	switch(cfg) {
	default:
		break;
	}
	return r;
}

static KFLOW_AI_ENGINE_CTX kflow_cpu_eng =
{
	.name = "cpu",
	.wait_queue = &kflow_cpu_wq,
	.channel_max = 0xff,
	.channel_count = MAX_CHN,
	.p_ch = kflow_cpu_ch,
	.attr = 1, //slow
	.init = kflow_cpu_init,
	.uninit = kflow_cpu_uninit,
	.getcfg = kflow_cpu_getcfg,
};

KFLOW_AI_ENGINE_CTX* kflow_cpu_get_engine(void)
{
	return &kflow_cpu_eng;
}

EXPORT_SYMBOL(kflow_cpu_get_engine);

