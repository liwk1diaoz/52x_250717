#include "comm/timer.h"
#include "comm/hwclock.h"

#include <kwrap/semaphore.h>

#include "kdrv_vdocdc_dbg.h"
#include "kdrv_vdocdc_thread.h"

#include "h26x.h"
#include "h26x_common.h"
#include "h26xenc_api.h"
#include "h264enc_api.h"
#include "h264dec_api.h"
#include "h265enc_api.h"
#include "h265dec_api.h"

static struct task_struct *run_task;

struct mutex mutex_add;

static ID xSemaphore_run;

BOOL do_run_thread;
BOOL b_run_idle = false;

KDRV_VDOCDC_LLC gst_llc = {0};

static int run_thread(void *usr_data)
{
	UINT32 interrupt;
	
	while(!THREAD_SHOULD_STOP) {
		if (do_run_thread == true) {
			if (gst_llc.job_num != 0) {
				if (gst_llc.codec_mode[gst_llc.run_idx] == VDOCDC_ENC_MODE) {
					gst_llc.vdoenc_osdmask_cb(KDRV_DEV_ID_CHANNEL(gst_llc.id[gst_llc.run_idx]), 1);
				}
				b_run_idle = false;
				h26x_setEncDirectRegSet(gst_llc.va[gst_llc.run_idx]);
				h26x_start();
				interrupt = h26x_waitINT();
				if (gst_llc.codec_mode[gst_llc.run_idx] == VDOCDC_ENC_MODE) {
					H26XEncResultCfg sResult = {0};
					H26XENC_VAR *p_var = gst_llc.p_var[gst_llc.run_idx];
					KDRV_VDOENC_PARAM *p_param = gst_llc.p_param[gst_llc.run_idx];

					if (p_var->eCodecType == VCODEC_H264)
						h264Enc_getResult(p_var, 1, &sResult, interrupt, 0, NULL, 0);
					else
						h265Enc_getResult(p_var, 1, &sResult, interrupt, 0, NULL, 0);

					gst_llc.vdoenc_osdmask_cb(KDRV_DEV_ID_CHANNEL(gst_llc.id[gst_llc.run_idx]), 0);
					p_param->temproal_id = sResult.uiSvcLable;
					p_param->encode_err = ((interrupt & h26x_getIntEn()) != 0x00000001);
					p_param->re_encode_en = (interrupt != H26X_FINISH_INT);
					p_param->nxt_frm_type = sResult.ucNxtPicType;
					p_param->base_qp = sResult.ucQP;
					p_param->bs_size_1 = sResult.uiBSLen;
					p_param->frm_type = sResult.ucPicType;
					p_param->encode_time = sResult.uiHwEncTime;

					gst_llc.callback[gst_llc.run_idx]->callback(p_param, gst_llc.user_data[gst_llc.run_idx]);
				}
				else {
					H26XDEC_VAR *p_var = gst_llc.p_var[gst_llc.run_idx];
					KDRV_VDODEC_PARAM *p_param = gst_llc.p_param[gst_llc.run_idx];
					KDRV_VDODEC_REFFRMCB pCb = p_param->vRefFrmCb;

					gst_llc.callback[gst_llc.run_idx]->callback("decode", gst_llc.user_data[gst_llc.run_idx]);

					if (p_var->eCodecType == VCODEC_H264) {
						if ((h264Dec_getResYAddr(p_var) != 0) && interrupt == 0x00000001)
							pCb.VdoDec_RefFrmDo(gst_llc.id[gst_llc.run_idx], h264Dec_getResYAddr(p_var), FALSE);
						#if 0
						if ((h264Dec_getResYAddr2(p_var) != 0) && interrupt == 0x00000001)
							pCb.VdoDec_RefFrmDo(gst_llc.id[gst_llc.run_idx], h264Dec_getResYAddr2(p_var), FALSE);
						#endif
					}
					else {
						if ((h265Dec_getResYAddr(p_var) != 0) && interrupt == 0x00000001)
							pCb.VdoDec_RefFrmDo(gst_llc.id[gst_llc.run_idx], h265Dec_getResYAddr(p_var), FALSE);
					}
				}
				mutex_lock(&mutex_add);
				gst_llc.job_num--;
				if (gst_llc.codec_mode[gst_llc.run_idx] == VDOCDC_ENC_MODE)
					gst_llc.enc_job_num--;
				else
					gst_llc.dec_job_num--;
				mutex_unlock(&mutex_add);
				gst_llc.run_idx = (gst_llc.run_idx+1)%VDOCDC_JOB_MAX;
			}
			else {
				#if 0
				static UINT32 t[3];
				
				t[0] = hwclock_get_counter();
				b_run_idle = true;
				vos_sem_wait(xSemaphore_run);
				t[2] = t[1];
				t[1] = hwclock_get_counter();
				DBG_DUMP("[%s][%d] %d, %d\r\n", __FUNCTION__, __LINE__, t[1] - t[0], t[0] - t[2]);
				#else
				b_run_idle = true;
				vos_sem_wait(xSemaphore_run);
				#endif
			}
		}
	}

	return 0;
}

int kdrv_vdocdc_create_thread(void)
{
	int ret;

	memset(&gst_llc, 0, sizeof(KDRV_VDOCDC_LLC));

	 if (0 != vos_sem_create(&xSemaphore_run, 1, "my_xSemaphore_run")) {
	 	return -1;
	}

	THREAD_CREATE(run_task, run_thread, NULL, "vdocdc_run");

	if (IS_ERR(run_task)) {
		DBG_ERR("run task create fail\r\n");
		ret = PTR_ERR(run_task);
		run_task = NULL;
		return ret;
	}


	#if 1
	THREAD_SET_PRIORITY(run_task, 3);
	#endif

	mutex_init(&mutex_add);

	do_run_thread = true;

	wake_up_process(run_task);

	return 0;
}

int kdrv_vdocdc_remove_thread(void)
{
	do_run_thread = false;

	//mutex_unlock(&mutex_add);

	kthread_stop(run_task);

	vos_sem_destroy(xSemaphore_run);
	
	#if 0
	run_task = NULL;
	#endif

	return 0;
}

int kdrv_vdocdc_add_job(KDRV_VDOCDC_MODE codec_mode, UINT32 id, UINT32 uiVaApbAddr, KDRV_CALLBACK_FUNC *p_cb_func, void *user_data, void *p_var, void *p_param)
{
	int ret = 0;
	//DBG_DUMP("[%s][%d] %d, %d\r\n", __FUNCTION__, __LINE__, id, hwclock_get_counter());
	if (gst_llc.job_num < VDOCDC_JOB_MAX) {
		gst_llc.id[gst_llc.add_idx] = id;
		gst_llc.va[gst_llc.add_idx] = uiVaApbAddr;
		gst_llc.codec_mode[gst_llc.add_idx] = codec_mode;
		gst_llc.callback[gst_llc.add_idx] = p_cb_func;
		gst_llc.user_data[gst_llc.add_idx] = user_data;
		gst_llc.p_var[gst_llc.add_idx] = p_var;
		gst_llc.p_param[gst_llc.add_idx] = p_param;
		gst_llc.add_idx = (gst_llc.add_idx+1)%VDOCDC_JOB_MAX;
		mutex_lock(&mutex_add);
		gst_llc.job_num++;
		if (codec_mode == VDOCDC_ENC_MODE)
			gst_llc.enc_job_num++;
		else
			gst_llc.dec_job_num++;
		mutex_unlock(&mutex_add);
		if (b_run_idle == true) vos_sem_sig(xSemaphore_run);
	}
	else {
		DBG_ERR("job queue full(%d, %d)\r\n", VDOCDC_JOB_MAX, gst_llc.job_num);
		ret = -1;
	}

	return ret;
}

int kdrv_vdocdc_set_llc_mem(UINT32 addr, UINT32 size)
{
	//memset(&gst_llc, 0, sizeof(KDRV_VDOCDC_LLC));
	
	return 0;
}

UINT32 kdrv_vdocdc_get_llc_mem(void)
{
	return 0x100;
}

BOOL kdrv_vdocdc_get_job_done(void)
{
	return (gst_llc.job_num == 0) ? 1 : 0;
}

BOOL kdrv_vdocdc_get_enc_job_done(void)
{
	return (gst_llc.enc_job_num == 0) ? TRUE : FALSE;
}

BOOL kdrv_vdocdc_get_dec_job_done(void)
{
	return (gst_llc.dec_job_num == 0) ? TRUE : FALSE;
}

void kdrv_vdocdc_set_osdmask_cb(KDRV_VDOENC_OSDMASKCB *pOsdMaskCB)
{
	gst_llc.vdoenc_osdmask_cb = pOsdMaskCB->vdoenc_osdmask_cb;
}

