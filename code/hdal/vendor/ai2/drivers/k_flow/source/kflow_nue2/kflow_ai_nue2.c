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
#include "kwrap/semaphore.h"
#include "kwrap/cpu.h"

#include "ai_ioctl.h" //for AI_DRV_OPENCFG
#include "kdrv_ai.h"
#include "kflow_ai_net/nn_net.h"
#include "kflow_ai_net/nn_parm.h"
#include "kflow_ai_net/kflow_ai_net_comm.h"

//=============================================================
#define __CLASS__ 				"[ai][kflow][nue2]"
#include "kflow_ai_debug.h"
//=============================================================
#include "kflow_nue2/kflow_nue2.h"
#include "kflow_nue2/kflow_nue2_platform.h"

#define SHOW_FLOW 0
#define DUMMY 0
#define CLEAR_OUTBUF_BEFORE_TRIG 0
#define ENG_NAME "NUE2"

extern void kdrv_ai_config_flow(UINT32 flow);

static SEM_HANDLE SEMID_KFLOW_NUE2;
static KFLOW_AI_CHANNEL_CTX kflow_nue2_ch1;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
static INT32 kflow_nue2_ch1_isrcb(UINT32 cycle_eng, UINT32 cycle_ll, UINT32 cycle_dma, UINT32 intstatus, UINT32 eng, void *parm);
#else
static INT32 kflow_nue2_ch1_isrcb(UINT32 handle, UINT32 intstatus, UINT32 eng, void *parm);
#endif
static char output_path[STR_MAX_LENGTH] = DBG_OUT_PATH;
#if (CLEAR_OUTBUF_BEFORE_TRIG == 1)
#define MAX_BUF_NUM 1000
extern BOOL outbuf_is_clear[MAX_BUF_NUM];
#endif

int kflow_nue2_set_output_path(char *path)
{
	if (path == NULL) {
		return -1;
	}
	snprintf(output_path, STR_MAX_LENGTH-1, path);
	return 0;
}
EXPORT_SYMBOL(kflow_nue2_set_output_path);

static int _kflow_nue2_write_file(char *filename, void *data, size_t count, unsigned long long *offset)
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

static void _kflow_nue2_dump_engine_register(KFLOW_AI_CHANNEL_CTX *p_ch, KFLOW_AI_JOB* p_job)
{
	if (!p_ch) {
		DBG_DUMP("%s:%d p_ch is NULL", __func__, __LINE__);
		return;
	}
	
	if (p_job == 0 ){
		DBG_DUMP("kflow_ai - dump reg <%s>\n", p_ch->name);
	}else {
		DBG_DUMP("kflow_ai - dump reg <%s> proc_id(%d) job_id(%d)\n", p_ch->name, p_job->proc_id, p_job->job_id);
	}

	if (strcmp(p_ch->name, "nue2") == 0) {
		debug_dumpmem(0xf0d50000, 0x200);
	}
}
	
static void _kflow_nue2_dump_out_buf(KFLOW_AI_CHANNEL_CTX *p_ch, KFLOW_AI_JOB* p_job)
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
	if (!p_mctrl->tot_trig_eng_times) {
		return;
	}

#if CNN_25_MATLAB
	for (out_id = 0; out_id < 3; out_id++) {
		if (p_io_mem->SAO[out_id].size > 0) {
			snprintf(filename, STR_MAX_LENGTH-1, "%s/%s_%u_OUT%u.bin", output_path, ENG_NAME, (unsigned int)p_job->job_id, (unsigned int)out_id);
			DBG_DUMP("write file: %s\n", filename);
			vos_cpu_dcache_sync(p_sao[out_id].va, p_sao[out_id].size, VOS_DMA_FROM_DEVICE);
			_kflow_nue2_write_file(filename, (void *)p_io_mem->SAO[out_id].va, p_io_mem->SAO[out_id].size, 0);
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
			_kflow_nue2_write_file(filename, (void *)p_sao[out_id].va, p_sao[out_id].size, 0);
		}
	}
#endif
}

static void kflow_nue2_ch1_open(KFLOW_AI_CHANNEL_CTX* p_ch)
{
	const UINT32 chip = 0;
	const UINT32 channel = 0;
	UINT32 id;
	AI_DRV_OPENCFG cfg;

	SEM_CREATE(SEMID_KFLOW_NUE2, 1);
	SEM_WAIT(SEMID_KFLOW_NUE2);
	
	cfg.opencfg.clock_sel = 0;
	cfg.engine = AI_ENG_NUE2;
	cfg.net_id = 0;

    kdrv_ai_config_flow(1); //config to new AI flow

	id = KDRV_DEV_ID(chip, KDRV_AI_ENGINE_NUE2, channel);
	kdrv_ai_set(id, KDRV_AI_PARAM_OPENCFG, &cfg.opencfg);
	kdrv_ai_set(id, KDRV_AI_PARAM_ISR_CB, kflow_nue2_ch1_isrcb);
	kdrv_ai_open(chip, KDRV_AI_ENGINE_NUE2);  //clk set to 480MHz
	INIT_LIST_HEAD(p_ch->ready_queue);

	p_ch->wait_ms = -1;
	
	SEM_SIGNAL(SEMID_KFLOW_NUE2);
}

static void kflow_nue2_ch1_close(KFLOW_AI_CHANNEL_CTX* p_ch)
{
	const UINT32 chip = 0, channel = 0;
	KDRV_DEV_ENGINE engine = 0;
	UINT32 id = 0;
	KDRV_AI_TRIGGER_PARAM trig_parm = {0};

	SEM_WAIT(SEMID_KFLOW_NUE2);
	
	engine = KDRV_AI_ENGINE_NUE2;

	id = KDRV_DEV_ID(chip, engine, channel);

    if (p_ch->wait_ms >= 0) {  // p_job->wait_ms; ///<-1: blocking, 0: non-blocking, >0: non-blocking + timeout
		trig_parm.is_nonblock = 1; //non-blocking
		trig_parm.time_out_ms = (UINT32)(p_ch->wait_ms);
	    kdrv_ai_waitdone(id, &trig_parm, NULL, NULL);
	}
	kdrv_ai_close(chip, KDRV_AI_ENGINE_NUE2);
	
	SEM_SIGNAL(SEMID_KFLOW_NUE2);
	SEM_DESTROY(SEMID_KFLOW_NUE2);
}

static void kflow_nue2_ch1_trig(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job)
{
	const UINT32 chip = 0, channel = 0;
	NN_GEN_ENG_TYPE ctrl_type;
	KDRV_DEV_ENGINE engine = 0;
	NN_GEN_MODE_CTRL *p_mctrl = (NN_GEN_MODE_CTRL*)(p_job->p_op_info);
	KDRV_AI_TRIGGER_PARAM trig_parm = {0};
	UINT32 id = 0;
#if (CLEAR_OUTBUF_BEFORE_TRIG == 1)
	NN_DATA *p_sao;
	UINT32 omem_cnt;
	UINT32 out_id = 0;
#endif

#if (SHOW_FLOW == 1)
	{
		DBG_DUMP("----- proc[%d].job[%d] nue2 trig: layer=%d, eng=%d, mode=%d, trig=%d, cnt=%d\r\n",
			(int)p_job->proc_id, (int)p_job->job_id, p_mctrl->layer_index, p_mctrl->eng, p_mctrl->mode, p_mctrl->trig_src, p_mctrl->tot_trig_eng_times);
	}
#endif
#if (DUMMY == 1)
	p_job->exec_cnt++;
	if (p_ch->onfinish != 0) {
		p_ch->onfinish(p_ch, p_ch->run_job, 0);
		return;
	}
#endif
	ctrl_type = p_mctrl->eng;
	if (ctrl_type == NN_GEN_ENG_NUE2)
		engine = KDRV_AI_ENGINE_NUE2;

	id = KDRV_DEV_ID(chip, engine, channel);

	switch (p_mctrl->trig_src) {
    case NN_GEN_TRIG_USER_AI_DRV:
		if (p_mctrl->tot_trig_eng_times) {
			KDRV_AI_TRIG_MODE mode = AI_TRIG_MODE_PREC; 
			KDRV_AI_APP_INFO info = { (KDRV_AI_APP_HEAD *)(p_mctrl->addr), p_mctrl->tot_trig_eng_times };
	        p_ch->wait_ms = -1; // always blocking
			SEM_WAIT(SEMID_KFLOW_NUE2);
			kdrv_ai_set(id, KDRV_AI_PARAM_MODE_INFO, &mode);
			kdrv_ai_set(id, KDRV_AI_PARAM_APP_INFO, &info);
			kdrv_ai_trigger(id, &trig_parm, NULL, NULL);
			SEM_SIGNAL(SEMID_KFLOW_NUE2);
        	p_job->exec_cnt++;
			if (p_ch->onfinish != 0) {
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
				p_ch->onfinish(p_ch, p_ch->run_job, 0, 0);
#else
				p_ch->onfinish(p_ch, p_ch->run_job, 0);
#endif
			}
		} else {
        	p_job->exec_cnt++;
			if (p_ch->onfinish != 0) {
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
				p_ch->onfinish(p_ch, p_ch->run_job, 0, 0);
#else
				p_ch->onfinish(p_ch, p_ch->run_job, 0);
#endif
			}
		}
		break;
        
	case NN_GEN_TRIG_APP_AI_DRV:
		if (p_mctrl->tot_trig_eng_times) {
			KDRV_AI_TRIG_MODE mode = AI_TRIG_MODE_APP;
			KDRV_AI_APP_INFO info = { (KDRV_AI_APP_HEAD *)(p_mctrl->addr), p_mctrl->tot_trig_eng_times };

	        p_ch->wait_ms = -1; // always blocking
			SEM_WAIT(SEMID_KFLOW_NUE2);
			kdrv_ai_set(id, KDRV_AI_PARAM_MODE_INFO, &mode);
			kdrv_ai_set(id, KDRV_AI_PARAM_APP_INFO, &info);
			kdrv_ai_trigger(id, &trig_parm, NULL, NULL);
			SEM_SIGNAL(SEMID_KFLOW_NUE2);
		} else {
        	p_job->exec_cnt++;
			if (p_ch->onfinish != 0) {
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
				p_ch->onfinish(p_ch, p_ch->run_job, 0, 0);
#else
				p_ch->onfinish(p_ch, p_ch->run_job, 0);
#endif
			}
		}
		break;

	case NN_GEN_TRIG_LL_AI_DRV:
		if (p_mctrl->tot_trig_eng_times) {
			KDRV_AI_TRIG_MODE mode = AI_TRIG_MODE_LL;
			KDRV_AI_LL_INFO info = { (KDRV_AI_LL_HEAD *)(p_mctrl->addr), p_mctrl->tot_trig_eng_times };
            p_ch->wait_ms = -1;
#if (CLEAR_OUTBUF_BEFORE_TRIG == 1)
			if ((p_mctrl->layer_index ==0)||(p_mctrl->layer_index > (p_mctrl-1)->layer_index)) {
				omem_cnt = p_mctrl->iomem.omem_cnt;
				p_sao    = (NN_DATA*)p_mctrl->iomem.omem_addr;
				for (out_id = 0; out_id < omem_cnt; out_id++) {
					if (p_sao[out_id].size > 0) {
						memset((void *)p_sao[out_id].va,0,p_sao[out_id].size);
						vos_cpu_dcache_sync(p_sao[out_id].va, p_sao[out_id].size, VOS_DMA_TO_DEVICE);
					}
				}
			}
#endif
            if (p_job->wait_ms >= 0) {  // p_job->wait_ms; ///<-1: blocking, 0: non-blocking, >0: non-blocking + timeout
    			trig_parm.is_nonblock = 1; //non-blocking
    			trig_parm.time_out_ms = (UINT32)(p_job->wait_ms);
    			p_ch->wait_ms = p_job->wait_ms;
				kdrv_ai_set(id, KDRV_AI_PARAM_REV | KDRV_AI_PARAM_MODE_INFO, &mode);
				kdrv_ai_set(id, KDRV_AI_PARAM_REV | KDRV_AI_PARAM_LL_INFO, &info);
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
    			kdrv_ai_trigger_isr(id, &trig_parm, NULL, NULL, p_job->parm_addr);
#else
				kdrv_ai_trigger(id, &trig_parm, NULL, NULL);
#endif
    		} else if (p_job->wait_ms == -1) {
				SEM_WAIT(SEMID_KFLOW_NUE2);
    			kdrv_ai_set(id, KDRV_AI_PARAM_MODE_INFO, &mode);
    			kdrv_ai_set(id, KDRV_AI_PARAM_LL_INFO, &info);
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
    			kdrv_ai_trigger_isr(id, &trig_parm, NULL, NULL, p_job->parm_addr);
#else
				kdrv_ai_trigger(id, &trig_parm, NULL, NULL);
#endif
				SEM_SIGNAL(SEMID_KFLOW_NUE2);
			}
		} else {
        	p_job->exec_cnt++;
			if (p_ch->onfinish != 0) {
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
				p_ch->onfinish(p_ch, p_ch->run_job, 0, 0);
#else
				p_ch->onfinish(p_ch, p_ch->run_job, 0);
#endif
			}
		}
		break;

	case NN_GEN_TRIG_COMMON:
	default:
		break;
	}
}

static void kflow_nue2_ch1_debug(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job, UINT32 info)
{
	/* dump engine register */
	if (info & KFLOW_AI_DBG_CTX) {
		_kflow_nue2_dump_engine_register(p_ch, p_job);
	}

	/* dump output buffer */
	if (info & KFLOW_AI_DBG_OBUF) {
		_kflow_nue2_dump_out_buf(p_ch, p_job);
	}
}

#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
static INT32 kflow_nue2_ch1_isrcb(UINT32 cycle_eng, UINT32 cycle_ll, UINT32 cycle_dma, UINT32 intstatus, UINT32 eng, void *parm)
#else
static INT32 kflow_nue2_ch1_isrcb(UINT32 handle, UINT32 intstatus, UINT32 eng, void *parm)
#endif
{
	KFLOW_AI_CHANNEL_CTX* p_ch = &kflow_nue2_ch1;
	KFLOW_AI_JOB* p_job = p_ch->run_job;
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
	UINT32 macc = cycle_eng - cycle_dma;
#endif

#if (SHOW_FLOW == 1)
	{
		KFLOW_AI_CHANNEL_CTX* p_ch = 0;
		p_ch = p_job->p_ch;
		DBG_DUMP("- proc[%d].job[%d] done by engine[%s] ch[%s]\r\n", 
			(int)p_job->proc_id, (int)p_job->job_id, p_job->p_eng->name, p_ch->name);
	}
#endif
	if (p_job == NULL) {
		DBG_DUMP("kflow_nue2_ch1_isrcb ERR - NULL run_job????????????\r\n");
		return 0;
	}

	p_job->exec_cnt++;
	if (p_ch->onfinish != 0) {
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
		p_ch->onfinish(p_ch, p_ch->run_job, cycle_eng, macc);
#else
		p_ch->onfinish(p_ch, p_ch->run_job, handle);
#endif
	}

	return 0;
}

static void kflow_nue2_ch1_reset2(struct _KFLOW_AI_CHANNEL_CTX* p_ch)
{
	KFLOW_AI_JOB* p_job = p_ch->run_job;
	//NN_GEN_MODE_CTRL *p_mctrl = (NN_GEN_MODE_CTRL*)(p_job->p_op_info);
	
	//DBG_IND("- do reset NUE2!\r\n");
	kdrv_ai_engine_reset(AI_ENG_NUE2);

	//DBG_IND("- do reset isr!\r\n");
	p_job->exec_cnt++;
	if (p_ch->onfinish != 0)
#if defined(_BSP_NA51090_) || defined(_BSP_NA51102_) || defined(_BSP_NA51103_)
		p_ch->onfinish(p_ch, p_ch->run_job, 0xfffe0000, 0xfffe0000);
#else
		p_ch->onfinish(p_ch, p_ch->run_job, 0xfffe0000);
#endif	
}


static LIST_HEAD kflow_nue2_rq = {0};
static KFLOW_AI_CHANNEL_CTX kflow_nue2_ch1 =
{
	.name = "nue2",
	.ready_queue = &kflow_nue2_rq,
	.run_job = 0,
	.open = kflow_nue2_ch1_open,
	.close = kflow_nue2_ch1_close,
	.trigger = kflow_nue2_ch1_trig,
	.debug = kflow_nue2_ch1_debug,
	.reset2 = kflow_nue2_ch1_reset2,
};

#define MAX_CHN 1
static LIST_HEAD kflow_nue2_wq = {0};
static KFLOW_AI_CHANNEL_CTX* kflow_nue2_ch[MAX_CHN] = {&kflow_nue2_ch1};

static void kflow_nue2_init(KFLOW_AI_ENGINE_CTX* p_eng)
{
	INT idx = 0;
	INIT_LIST_HEAD(p_eng->wait_queue);

	if (kdrv_ai_get_eng_caps(AI_ENG_NUE2)) {
		kflow_nue2_ch[idx] = &kflow_nue2_ch1;
		idx++;
	}

	p_eng->channel_count = idx;
	//DBG_DUMP("engine[%s] hw-limit ch=%d\r\n", p_eng->name, p_eng->channel_count);
	if (p_eng->channel_count > p_eng->channel_max) {
		//DBG_DUMP("engine[%s] sw-limit ch=%d\r\n", p_eng->name, p_eng->channel_max);
		p_eng->channel_count = p_eng->channel_max;
	}
}

static void kflow_nue2_uninit(KFLOW_AI_ENGINE_CTX* p_eng)
{
}

static UINT32 kflow_nue2_getcfg(KFLOW_AI_ENGINE_CTX* p_eng, KFLOW_AI_JOB* p_job, UINT32 cfg)
{
	UINT32 r = 0;
	NN_GEN_MODE_CTRL *p_mctrl = NULL;
	switch(cfg) {
	case 0:
		if (p_job) {
			p_mctrl = (NN_GEN_MODE_CTRL*)(p_job->p_op_info);
			if (p_mctrl->idea_cycle != 0) {
				r = p_mctrl->idea_cycle / 480;
			}
		}
		break;
	case 1:
		if (p_job) {
			NN_GEN_MODE_CTRL *p_mctrl = (NN_GEN_MODE_CTRL*)(p_job->p_op_info);
			if (p_mctrl->idea_cycle != 0) {
				r = p_mctrl->idea_cycle;
			}
		}
		break;
	default:
		break;
	}
	return r;
}

static KFLOW_AI_ENGINE_CTX kflow_nue2_eng =
{
	.name = "nue2",
	.wait_queue = &kflow_nue2_wq,
	.channel_max = 0xff,
	.channel_count = MAX_CHN,
	.p_ch = kflow_nue2_ch,
	.attr = 0, //fast
	.init = kflow_nue2_init,
	.uninit = kflow_nue2_uninit,
	.getcfg = kflow_nue2_getcfg,
};

KFLOW_AI_ENGINE_CTX* kflow_nue2_get_engine(void)
{
	return &kflow_nue2_eng;
}
EXPORT_SYMBOL(kflow_nue2_get_engine);

