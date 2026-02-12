/*#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/uaccess.h>*/
//#include "mach/fmem.h"
#include "kwrap/semaphore.h"
/*#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
*/

#include "ai_lib.h"
#include "kdrv_ai.h"
#include "cnn_lib.h"
#include "nue_lib.h"
#include "kdrv_ai_int.h"
#if defined(__FREERTOS)
#include "string.h" //for memcpy
#endif
//=============================================================
#define __CLASS__ 				"[ai][kdrv][lib]"
#include "kdrv_ai_debug.h"
//=============================================================
#include "nue2_lib.h"
#include "kwrap/flag.h"
#include "kdrv_ai_platform.h"
#include "nue/nue_ll_cmd.h"
#include "kwrap/cpu.h"

#define NEURAL_PRINT_PARM               DISABLE
#define FC_PRINT_PARM                   DISABLE
#define PERMUTE_PRINT_PARM              DISABLE
#define REORG_PRINT_PARM                DISABLE
#define PREPROC_PRINT_PARM              DISABLE

UINT32 g_kdrv_ai_ll_base_addr[AI_ENG_TOTAL] = {0};

INT32 kdrv_ai_int_clamp(INT32 prx, INT32 lb, INT32 ub);


INT32 kdrv_ai_int_clamp(INT32 prx, INT32 lb, INT32 ub)
{
	/* avoid integer overflow or underflow */
	if (prx < lb) {
		return (lb);
	} else if (prx > ub) {
		return (ub);
	} else {
		return (prx);
	}
}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER kdrv_ai_cnn_trigger(BOOL cnn_id, CNN_OPMODE mode, CNN_PARM *p_parm)
{
	ER er_return = E_OK;
	if (p_parm == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}
	er_return = cnn_setmode(cnn_id, mode, p_parm);
	if (er_return != E_OK) {
		DBG_ERR("cnn setmode fail!\r\n");
		return er_return;
	}
	if (g_ai_isr_trig == 0) {
		er_return = cnn_start(cnn_id);
		if (er_return != E_OK) {
			DBG_ERR("cnn %d start fail!\r\n", cnn_id);
		}
	}
	else {
		er_return = cnn_isr_start(cnn_id);
		if (er_return != E_OK) {
			DBG_ERR("cnn %d cnn_isr_start fail!\r\n", cnn_id);
		}

	}

	return er_return;
}

ER kdrv_ai_cnn_done(BOOL cnn_id, KDRV_AI_HANDLE *p_handle)
{
	ER er_return = E_OK;
#if (KDRV_AI_FLG_IMPROVE == 0)	
	FLGPTN flag_ptn = 0;
	UINT32 cnn_fmd = 0;

	if (p_handle == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	if (cnn_id == 0) {
		cnn_fmd = KDRV_AI_CNN_FMD;
	} else {
		cnn_fmd = KDRV_AI_CNN2_FMD;
	}

	wai_flg(&flag_ptn, p_handle->flag_id, cnn_fmd | KDRV_AI_TIMEOUT | KDRV_AI_RESET, TWF_ORW | TWF_CLR);

	if (flag_ptn & cnn_fmd) {
		//DBG_IND("CNN app frame end\r\n");
		cnn_wait_frameend(cnn_id, FALSE);
	} else if (flag_ptn & KDRV_AI_TIMEOUT) {
		DBG_ERR("CNN app timeout!\r\n");
		cnn_reset(cnn_id);
	} else if (flag_ptn & KDRV_AI_RESET) {
		DBG_ERR("CNN app reset!\r\n");
	}
#else
	cnn_wait_frameend(cnn_id, FALSE);
#endif

	er_return = cnn_pause(cnn_id);
	if (er_return != E_OK) {
		DBG_ERR("cnn pause fail!\r\n");
		return er_return;
	}
#if (KDRV_AI_FLG_IMPROVE == 0)
	if (flag_ptn & KDRV_AI_TIMEOUT) {
		er_return = E_TMOUT;
	}
#endif
	return er_return;
}

ER kdrv_ai_nue_trigger(NUE_OPMODE mode, NUE_PARM *p_parm)
{
	ER er_return = E_OK;
	if (p_parm == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	er_return = nue_setmode(mode, p_parm);
	if (er_return != E_OK) {
		DBG_ERR("nue setmode fail!\r\n");
		return er_return;
	}
	if (g_ai_isr_trig == 0) {
		er_return = nue_start();
		if (er_return != E_OK) {
			DBG_ERR("nue start fail!\r\n");
		}
	} else {
		er_return = nue_isr_start();
		if (er_return != E_OK) {
			DBG_ERR("nue_isr_start fail!\r\n");
		}

	}

	return er_return;
}

ER kdrv_ai_nue_done(KDRV_AI_HANDLE *p_handle)
{
	ER er_return = E_OK;
#if (KDRV_AI_FLG_IMPROVE == 0)
	FLGPTN flag_ptn = 0;
	if (p_handle == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	wai_flg(&flag_ptn, p_handle->flag_id, KDRV_AI_NUE_FMD | KDRV_AI_TIMEOUT | KDRV_AI_RESET, TWF_ORW | TWF_CLR);

	if (flag_ptn & KDRV_AI_NUE_FMD) {
		//DBG_IND("NUE app frame end\r\n");
		nue_wait_frameend(FALSE);
	} else if (flag_ptn & KDRV_AI_TIMEOUT) {
		DBG_ERR("NUE app timeout!\r\n");
		nue_reset();
	} else if (flag_ptn & KDRV_AI_RESET) {
		DBG_ERR("NUE app reset!\r\n");
	}
#else
	nue_wait_frameend(FALSE);
#endif

	er_return = nue_pause();
	if (er_return != E_OK) {
		DBG_ERR("nue pause fail!\r\n");
		return er_return;
	}
#if (KDRV_AI_FLG_IMPROVE == 0)
	if (flag_ptn & KDRV_AI_TIMEOUT) {
		er_return = E_TMOUT;
	}
#endif

	return er_return;
}

ER kdrv_ai_nue2_trigger(NUE2_OPMODE mode, NUE2_PARM *p_parm)
{
	ER er_return = E_OK;
	if (p_parm == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	er_return = nue2_setmode(mode, p_parm);
	if (er_return != E_OK) {
		DBG_ERR("nue2 setmode fail!\r\n");
		return er_return;
	}

	if (g_ai_isr_trig == 0) {
		er_return = nue2_start();
		if (er_return != E_OK) {
			DBG_ERR("nue2 start fail!\r\n");
		}
	} else {
		er_return = nue2_isr_start();
		if (er_return != E_OK) {
			DBG_ERR("nue2_isr_start fail!\r\n");
		}
	}

	return er_return;
}

ER kdrv_ai_nue2_done(KDRV_AI_HANDLE *p_handle)
{
	ER er_return = E_OK;
#if (KDRV_AI_FLG_IMPROVE == 0)
	FLGPTN flag_ptn = 0;
	if (p_handle == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	wai_flg(&flag_ptn, p_handle->flag_id, KDRV_AI_NUE2_FMD | KDRV_AI_TIMEOUT | KDRV_AI_RESET, TWF_ORW | TWF_CLR);

	if (flag_ptn & KDRV_AI_NUE2_FMD) {
		nue2_wait_frameend(FALSE);
	} else if (flag_ptn & KDRV_AI_TIMEOUT) {
		DBG_ERR("NUE2 app timeout!\r\n");
		nue2_reset();
	} else if (flag_ptn & KDRV_AI_RESET) {
		DBG_ERR("NUE2 app reset!\r\n");
	}
#else
	nue2_wait_frameend(FALSE);
#endif

	er_return = nue2_pause();
	if (er_return != E_OK) {
		DBG_ERR("nue2 pause fail!\r\n");
		return er_return;
	}
#if (KDRV_AI_FLG_IMPROVE == 0)
	if (flag_ptn & KDRV_AI_TIMEOUT) {
		er_return = E_TMOUT;
	}
#endif

	return er_return;
}

ER kdrv_ai_neural_trig(BOOL cnn_id, KDRV_AI_NEURAL_PARM *p_parm)
{
	ER er_return = E_OK;
	UINT32 idx = 0, func_list = 0;
	CNN_OPMODE cnn_mode = CNN_OPMODE_USERDEFINE;
	CNN_PARM cnn_parm;

#if NEURAL_PRINT_PARM
	DBGD(p_parm->src_fmt);
	DBGD(p_parm->in_type);
	DBGD(p_parm->out0_type);
	DBGD(p_parm->out1_type);
	DBGD(p_parm->size.width);
	DBGD(p_parm->size.height);
	DBGD(p_parm->size.channel);
	DBGD(p_parm->batch_num);
	DBGD(p_parm->in_addr);
	DBGH(p_parm->out0_addr);
	DBGH(p_parm->out1_addr);
	DBGH(p_parm->in_interm_addr);
	DBGH(p_parm->out_interm_addr);
	DBGH(p_parm->tmp_buf_addr);
	DBGD(p_parm->out0_dma_en);
	DBGD(p_parm->out1_dma_en);
	DBGD(p_parm->in_interm_dma_en);
	DBGD(p_parm->out_interm_dma_en);
	DBGD(p_parm->in_ofs.line_ofs);
	DBGD(p_parm->in_ofs.channel_ofs);
	DBGD(p_parm->in_ofs.batch_ofs);
	DBGD(p_parm->out0_ofs.line_ofs);
	DBGD(p_parm->out0_ofs.channel_ofs);
	DBGD(p_parm->out0_ofs.batch_ofs);
	DBGD(p_parm->out1_ofs.line_ofs);
	DBGD(p_parm->out1_ofs.channel_ofs);
	DBGD(p_parm->out1_ofs.batch_ofs);
	DBGD(p_parm->out0_cropy);
#endif

	if (p_parm == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	func_list = 0;





	for (idx = 0; idx < KDRV_AI_NEURAL_FUNC_CNT; idx++) {
		if (p_parm->func_list[idx] == 0)
			break;
		if ((func_list | p_parm->func_list[idx]) ==  func_list)
			goto exit;
		func_list = func_list | p_parm->func_list[idx];
	}

	if ((func_list & 0x1) == 1 && (func_list >> 1) > 0) {
		goto exit;
	}
	er_return = kdrv_ai_tran_cnn_parm(p_parm, &cnn_parm, func_list);
	if (er_return != E_OK) {
		goto exit;
	}
	er_return = kdrv_ai_cnn_trigger(cnn_id, cnn_mode, &cnn_parm);
	if (er_return != E_OK) {
		goto exit;
	}
exit:
	return er_return;
}

ER kdrv_ai_neural_done(BOOL cnn_id, KDRV_AI_NEURAL_PARM *p_parm, KDRV_AI_HANDLE *p_handle)
{
	ER er_return = E_OK;
	UINT32 idx = 0, func_list = 0;

	if ((p_parm == NULL) || (p_handle == NULL)) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	func_list = 0;
	for (idx = 0; idx < KDRV_AI_NEURAL_FUNC_CNT; idx++) {
		if (p_parm->func_list[idx] == 0)
			break;
		if ((func_list | p_parm->func_list[idx]) == func_list)
			return E_NOEXS;
		func_list = func_list | p_parm->func_list[idx];
	}

	if ((func_list & 0x1) == 1 && (func_list >> 1) > 0) {
		return E_NOEXS;
	}
	er_return = kdrv_ai_cnn_done(cnn_id, p_handle);

	return er_return;
}

ER kdrv_ai_roipool_trig(KDRV_AI_ROIPOOL_PARM *p_parm)
{
	ER er_return = E_OK;
	NUE_OPMODE mode = NUE_OPMODE_ROIPOOLING;
	NUE_PARM eng_parm;
	if (p_parm == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	er_return = kdrv_ai_tran_roipool_parm(p_parm, &eng_parm);
	if (er_return != E_OK) {
		goto exit;
	}
	er_return = kdrv_ai_nue_trigger(mode, &eng_parm);
	if (er_return != E_OK) {
		goto exit;
	}

exit:
	return er_return;
}

ER kdrv_ai_roipool_done(KDRV_AI_HANDLE *p_handle)
{
	if (p_handle == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}
	return kdrv_ai_nue_done(p_handle);
}

ER kdrv_ai_svm_trig(KDRV_AI_SVM_PARM *p_parm)
{
	ER er_return = E_OK;
	NUE_OPMODE mode = NUE_OPMODE_USERDEFINE;
	NUE_PARM eng_parm;
	if (p_parm == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	er_return = kdrv_ai_tran_svm_parm(p_parm, &eng_parm);
	if (er_return != E_OK) {
		goto exit;
	}
	er_return = kdrv_ai_nue_trigger(mode, &eng_parm);
	if (er_return != E_OK) {
		goto exit;
	}

exit:
	return er_return;
}

ER kdrv_ai_svm_done(KDRV_AI_SVM_PARM *p_parm, KDRV_AI_HANDLE *p_handle)
{
	ER er_return = E_OK;
	NUE_SVMRSTS svmrslts;
	if ((p_parm == NULL) || (p_handle == NULL)) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	er_return = kdrv_ai_nue_done(p_handle);
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = nue_get_rsts(&svmrslts);
	if (er_return != E_OK) {
		return er_return;
	}
	memcpy((VOID *)p_parm->out_addr, svmrslts.rslts, sizeof(INT32)*p_parm->obj_num);
	return er_return;
}

ER kdrv_ai_fc_trig(KDRV_AI_FC_PARM *p_parm)
{
	ER er_return = E_OK;
	UINT32 func_list = 0, idx = 0;
	NUE_OPMODE nue_mode = NUE_OPMODE_USERDEFINE;
	NUE_PARM nue_parm;
	if (p_parm == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

#if FC_PRINT_PARM
	DBGD(p_parm->src_fmt);
	DBGD(p_parm->in_type);
	DBGD(p_parm->out_type);
	DBGD(p_parm->size.width);
	DBGD(p_parm->size.height);
	DBGD(p_parm->size.channel);
	DBGD(p_parm->batch_num);
	DBGH(p_parm->in_addr);
	DBGH(p_parm->out_addr);
	DBGH(p_parm->in_interm_addr);
	DBGH(p_parm->out_interm_addr);
	DBGD(p_parm->in_interm_dma_en);
	DBGD(p_parm->out_interm_dma_en);
	DBGH(p_parm->fc_ker.weight_addr);
	DBGH(p_parm->fc_ker.bias_addr);
	DBGD(p_parm->fc_ker.weight_w);
	DBGD(p_parm->fc_ker.weight_h);
	DBGD(p_parm->fc_ker.weight_shf);
	DBGD(p_parm->fc_ker.acc_shf);
	DBGD(p_parm->fc_ker.norm_scl);
	DBGD(p_parm->fc_ker.norm_shf);
	DBGD(p_parm->act.mode);
	DBGD(p_parm->act.relu.leaky_val);
	DBGD(p_parm->act.relu.leaky_shf);
	DBGD(p_parm->act.act_shf0);
	DBGD(p_parm->act.act_shf1);
#endif

	func_list = 0;
	for (idx = 0; idx < KDRV_AI_FC_FUNC_CNT; idx++) {
		if (p_parm->func_list[idx] == 0)
			break;
		if ((func_list | p_parm->func_list[idx]) == func_list)
			return E_NOEXS;
		func_list = func_list | p_parm->func_list[idx];
	}

	if (idx == 0) {
		return E_NOEXS;
	}

	er_return = kdrv_ai_tran_nue_fc_parm(p_parm, &nue_parm, func_list);
	if (er_return != E_OK) {
		return E_NOEXS;
	}

	er_return = kdrv_ai_nue_trigger(nue_mode, &nue_parm);
	if (er_return != E_OK) {
		return E_NOEXS;
	}

	return er_return;
}

ER kdrv_ai_fc_done(KDRV_AI_FC_PARM *p_parm, KDRV_AI_HANDLE *p_handle)
{
	ER er_return = E_OK;
	UINT32 out_size = 0, idx = 0;
	if ((p_parm == NULL) || (p_handle == NULL)) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	er_return = kdrv_ai_nue_done(p_handle);
	if (er_return != E_OK) {
		return er_return;
	}

	out_size = p_parm->batch_num * p_parm->fc_ker.weight_h;
	if (p_parm->in_interm_dma_en) {
		INT32 *p_int_val = (INT32 *)(p_parm->in_interm_addr);
		INT8 norm_shf = p_parm->fc_ker.norm_shf;
		INT32 out_val = 0;
		INT8 out_shf = 0;
		for (idx = 0; idx < out_size; idx++) {
			if (p_parm->out_interm_dma_en) {
				out_shf = 0;
				out_val = *(INT32 *)(p_parm->out_interm_addr);
			} else if (p_parm->out_type == AI_IO_INT16) {
				out_shf = norm_shf;
				out_val = *(INT16 *)(p_parm->out_addr);
			} else if (p_parm->out_type == AI_IO_UINT16) {
				out_shf = norm_shf;
				out_val = *(UINT16 *)(p_parm->out_addr);
			} else if (p_parm->out_type == AI_IO_INT8) {
				out_shf = norm_shf + 8;
				out_val = *(INT8 *)(p_parm->out_addr);
			} else {
				out_shf = norm_shf + 8;
				out_val = *(UINT8 *)(p_parm->out_addr);
			}

			if (out_shf > 0) {
				out_val += ((p_int_val[idx] + (1 << (out_shf - 1))) >> out_shf);
			} else {
				out_val += (p_int_val[idx] << out_shf);
			}

			if (p_parm->out_interm_dma_en) {
				*(INT32 *)(p_parm->out_interm_addr) = out_val;
			} else if (p_parm->out_type == AI_IO_INT16) {
				*(INT16 *)(p_parm->out_addr) = (INT16)kdrv_ai_int_clamp(out_val, -(1 << 15), (1 << 15) - 1);
			} else if (p_parm->out_type == AI_IO_UINT16) {
				*(UINT16 *)(p_parm->out_addr) = (UINT16)kdrv_ai_int_clamp(out_val, 0, (1 << 16) - 1);
			} else if (p_parm->out_type == AI_IO_INT8) {
				*(INT8 *)(p_parm->out_addr) = (INT8)kdrv_ai_int_clamp(out_val, -(1 << 7), (1 << 7) - 1);
			} else {
				*(UINT8 *)(p_parm->out_addr) = (UINT8)kdrv_ai_int_clamp(out_val, 0, (1 << 8) - 1);
			}
		}
	}

	return er_return;
}

ER kdrv_ai_permute_trig(KDRV_AI_PERMUTE_PARM *p_parm)
{
	ER er_return = E_OK;
	NUE_OPMODE mode = NUE_OPMODE_PERMUTE;
	NUE_PARM eng_parm = {0};

#if PERMUTE_PRINT_PARM
	DBGD(p_parm->in_type);
	DBGD(p_parm->out_type);
	DBGD(p_parm->size.width);
	DBGD(p_parm->size.height);
	DBGD(p_parm->size.channel);
	DBGH(p_parm->in_addr);
	DBGH(p_parm->out_addr);
	DBGD(p_parm->perm_ker.mode);
#endif

	if (p_parm == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	er_return = kdrv_ai_tran_permute_parm(p_parm, &eng_parm);
	if (er_return != E_OK) {
		return er_return;
	}
	er_return = kdrv_ai_nue_trigger(mode, &eng_parm);
	return er_return;
}

ER kdrv_ai_permute_done(KDRV_AI_HANDLE *p_handle)
{
	if (p_handle == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	return kdrv_ai_nue_done(p_handle);
}

ER kdrv_ai_reorg_trig(KDRV_AI_REORG_PARM *p_parm)
{
	ER er_return = E_OK;
	NUE_OPMODE mode = NUE_OPMODE_REORG;
	NUE_PARM eng_parm;

#if REORG_PRINT_PARM
	DBGD(p_parm->in_type);
	DBGD(p_parm->out_type);
	DBGD(p_parm->size.width);
	DBGD(p_parm->size.height);
	DBGD(p_parm->size.channel);
	DBGH(p_parm->in_addr);
	DBGH(p_parm->out_addr);
	DBGD(p_parm->reorg_ker.reorg_shf);
#endif

	if (p_parm == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	er_return = kdrv_ai_tran_reorg_parm(p_parm, &eng_parm);
	if (er_return != E_OK) {
		return er_return;
	}
	er_return = kdrv_ai_nue_trigger(mode, &eng_parm);
	return er_return;
}

ER kdrv_ai_reorg_done(KDRV_AI_HANDLE *p_handle)
{
	if (p_handle == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}
	return kdrv_ai_nue_done(p_handle);
}

ER kdrv_ai_softmax_trig(KDRV_AI_SOFTMAX_PARM *p_parm)
{
	ER er_return = E_OK;
	NUE_OPMODE mode = NUE_OPMODE_SOFTMAX;
	NUE_PARM eng_parm = {0};

	if (p_parm == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	er_return = kdrv_ai_tran_softmax_parm(p_parm, &eng_parm);
	if (er_return != E_OK) {
		return er_return;
	}
	er_return = kdrv_ai_nue_trigger(mode, &eng_parm);
	return er_return;
}

ER kdrv_ai_softmax_done(KDRV_AI_HANDLE *p_handle)
{
	if (p_handle == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}
	return kdrv_ai_nue_done(p_handle);
}

ER kdrv_ai_anchor_trig(KDRV_AI_ANCHOR_PARM *p_parm)
{
	ER er_return = E_OK;
	NUE_OPMODE mode = NUE_OPMODE_ANCHOR;
	NUE_PARM eng_parm = {0};

	if (p_parm == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	er_return = kdrv_ai_tran_anchor_parm(p_parm, &eng_parm);
	if (er_return != E_OK) {
		return er_return;
	}
	er_return = kdrv_ai_nue_trigger(mode, &eng_parm);
	return er_return;
}

ER kdrv_ai_anchor_done(KDRV_AI_HANDLE *p_handle)
{
	if (p_handle == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}
	return kdrv_ai_nue_done(p_handle);
}

ER kdrv_ai_preproc_trig(KDRV_AI_PREPROC_PARM *p_parm)
{
	ER er_return = E_OK;
	UINT32 func_list = 0, idx = 0;
	NUE2_OPMODE nue2_mode = NUE2_OPMODE_USERDEFINE;
	NUE2_PARM nue2_parm;
	if (p_parm == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}
#if PREPROC_PRINT_PARM
	DBGD(p_parm->in_type);
	DBGD(p_parm->out_type);
	DBGD(p_parm->in_size.width);
	DBGD(p_parm->in_size.height);
	DBGD(p_parm->in_size.channel);
	DBGH(p_parm->in_addr[0]);
	DBGH(p_parm->in_addr[1]);
	DBGH(p_parm->in_addr[2]);
	DBGH(p_parm->out_addr[0]);
	DBGH(p_parm->out_addr[1]);
	DBGH(p_parm->out_addr[2]);
#endif
	func_list = 0;
	for (idx = 0; idx < KDRV_AI_PREPROC_FUNC_CNT; idx++) {
		if (p_parm->func_list[idx] == 0)
			break;
		if ((func_list | p_parm->func_list[idx]) ==  func_list)
			return E_NOEXS;
		func_list = func_list | p_parm->func_list[idx];
	}

	if (((func_list & KDRV_AI_PREPROC_ROT_EN) > 0) && ((func_list & 0x7) > 0)) {
		return E_NOEXS;
	}

	er_return = kdrv_ai_tran_nue2_preproc_parm(p_parm, &nue2_parm, func_list);
	if (er_return != E_OK) {
		return E_NOEXS;
	}


	er_return = kdrv_ai_nue2_trigger(nue2_mode, &nue2_parm);
	if (er_return != E_OK) {
		return E_NOEXS;
	}


	return er_return;
}

ER kdrv_ai_preproc_done(KDRV_AI_HANDLE *p_handle)
{
	ER er_return = E_OK;
	if (p_handle == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	er_return = kdrv_ai_nue2_done(p_handle);
	if (er_return != E_OK) {
		return er_return;
	}


	return er_return;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

ER kdrv_ai_cnn_ll_trig(BOOL cnn_id, UINT32 pa)
{
	ER er_return = E_OK;
	CNN_LL_PRM cnn_ll_prm = {0};
	
	if (pa == 0) {
		DBG_ERR("input address is null\r\n");
		return E_NOMEM;
	}

	cnn_ll_prm.addrin_ll_base = (cnn_id == 0)?g_kdrv_ai_ll_base_addr[AI_ENG_CNN]:g_kdrv_ai_ll_base_addr[AI_ENG_CNN2];
	cnn_ll_prm.addrin_ll = pa;
	er_return = cnn_ll_setmode(cnn_id, &cnn_ll_prm);
	
	if (er_return != E_OK) {
		DBG_ERR("set mode error : %08x\r\n", (int)er_return);
		return er_return;
	}
	if (g_ai_isr_trig == 0) {
		er_return = cnn_ll_start(cnn_id);
		if (er_return != E_OK) {
			DBG_ERR("start error : %08x\r\n", (int)er_return);
			return er_return;
		}
	} else {
		er_return = cnn_ll_isr_start(cnn_id);
		if (er_return != E_OK) {
			DBG_ERR("cnn_ll_isr_start error : %08x\r\n", (int)er_return);
			return er_return;
		}

	}

	return er_return;
}

ER kdrv_ai_cnn_ll_done(BOOL cnn_id, KDRV_AI_HANDLE *p_handle)
{
	ER er_return = E_OK;
#if (KDRV_AI_FLG_IMPROVE == 0)
	FLGPTN flag_ptn = 0;
	UINT32 cnn_fmd = 0;

	if (p_handle == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	if (cnn_id == 0) {
		cnn_fmd = KDRV_AI_CNN_FMD;
	} else {
		cnn_fmd = KDRV_AI_CNN2_FMD;
	}

	wai_flg(&flag_ptn, p_handle->flag_id, cnn_fmd | KDRV_AI_TIMEOUT | KDRV_AI_RESET, TWF_ORW | TWF_CLR);

	if (flag_ptn & cnn_fmd) {
		//DBG_IND("CNN ll frame end\r\n");
		cnn_wait_ll_frameend(cnn_id, FALSE);
		er_return = cnn_ll_pause(cnn_id);
	} else if (flag_ptn & KDRV_AI_TIMEOUT) {
		DBG_ERR("CNN ll timeout!\r\n");
		cnn_reset(cnn_id);
		cnn_ll_pause(cnn_id);
		er_return = E_TMOUT;
	} else if (flag_ptn & KDRV_AI_RESET) {
		DBG_ERR("CNN ll reset!\r\n");
		er_return = E_SYS;
	}
#else
	cnn_wait_ll_frameend(cnn_id, FALSE);
	er_return = cnn_ll_pause(cnn_id);
#endif

	return er_return;
}

ER kdrv_ai_nue_ll_trig(UINT32 pa)
{
	ER er_return = E_OK;
	NUE_LL_PRM nue_ll_prm = {0};
	
	if (pa == 0) {
		DBG_ERR("input address is null\r\n");
		return E_NOMEM;
	}

	nue_ll_prm.addrin_ll_base = g_kdrv_ai_ll_base_addr[AI_ENG_NUE];
	nue_ll_prm.addrin_ll = pa;
	er_return = nue_ll_setmode(&nue_ll_prm);
	if (er_return != E_OK) {
		DBG_ERR("set mode error : %08x\r\n", (int)er_return);
		return er_return;
	}

	if (g_ai_isr_trig == 0) {
		er_return = nue_ll_start();
		if (er_return != E_OK) {
			DBG_ERR("start error : %08x\r\n", (int)er_return);
			return er_return;
		}
	} else {
		er_return = nue_ll_isr_start();
		if (er_return != E_OK) {
			DBG_ERR("nue_ll_isr_start error : %08x\r\n", (int)er_return);
			return er_return;
		}
	}
	return er_return;
}

ER kdrv_ai_nue_ll_done(KDRV_AI_HANDLE *p_handle)
{
	ER er_return = E_OK;
#if (KDRV_AI_FLG_IMPROVE == 0)
	FLGPTN flag_ptn = 0;
	if (p_handle == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	wai_flg(&flag_ptn, p_handle->flag_id, KDRV_AI_NUE_FMD | KDRV_AI_TIMEOUT | KDRV_AI_RESET, TWF_ORW | TWF_CLR);

	if (flag_ptn & KDRV_AI_NUE_FMD) {
		//DBG_IND("NUE ll frame end\r\n");
		nue_wait_ll_frameend(FALSE);
		er_return = nue_ll_pause();
	} else if (flag_ptn & KDRV_AI_TIMEOUT) {
		DBG_ERR("NUE ll timeout!\r\n");
		nue_reset();
		nue_ll_pause();
		er_return = E_TMOUT;
	} else if (flag_ptn & KDRV_AI_RESET) {
		DBG_ERR("NUE ll reset!\r\n");
		er_return = E_SYS;
	}
#else
	nue_wait_ll_frameend(FALSE);
	er_return = nue_ll_pause();
#endif

	return er_return;
}

ER kdrv_ai_nue2_ll_trig(UINT32 pa)
{
	ER er_return = E_OK;
	NUE2_LL_PRM kdrv_ll_parm;

	if (pa == 0) {
		DBG_ERR("input address is null\r\n");
		return E_NOMEM;
	}

	kdrv_ll_parm.ll_addr = pa;
	kdrv_ll_parm.ll_base_addr = g_kdrv_ai_ll_base_addr[AI_ENG_NUE2];
	er_return = nue2_ll_setmode(&kdrv_ll_parm);
	if (er_return != E_OK) {
		DBG_ERR("set mode error : %08x\r\n", (int)er_return);
		return er_return;
	}

	if (g_ai_isr_trig == 0) {
		er_return = nue2_ll_start();
		if (er_return != E_OK) {
			DBG_ERR("start error : %08x\r\n", (int)er_return);
			return er_return;
		}
	} else {
		er_return = nue2_ll_isr_start();
		if (er_return != E_OK) {
			DBG_ERR("nue2_ll_isr_start error : %08x\r\n", (int)er_return);
			return er_return;
		}

	}

	return er_return;
}

ER kdrv_ai_nue2_ll_done(KDRV_AI_HANDLE *p_handle)
{
	ER er_return = E_OK;
#if (KDRV_AI_FLG_IMPROVE == 0)
	FLGPTN flag_ptn = 0;
	if (p_handle == NULL) {
		DBG_ERR("invalid input\r\n");
		return E_NOEXS;
	}

	wai_flg(&flag_ptn, p_handle->flag_id, KDRV_AI_NUE2_FMD | KDRV_AI_TIMEOUT | KDRV_AI_RESET, TWF_ORW | TWF_CLR);

	if (flag_ptn & KDRV_AI_NUE2_FMD) {
		//DBG_IND("NUE2 ll frame end\r\n");
		nue2_wait_ll_frameend(FALSE);
		er_return = nue2_ll_pause();
	} else if (flag_ptn & KDRV_AI_TIMEOUT) {
		DBG_ERR("NUE2 ll timeout!\r\n");
		nue2_reset();
		//nue2_ll_pause();
		er_return = E_TMOUT;
	} else if (flag_ptn & KDRV_AI_RESET) {
		DBG_ERR("NUE2 ll reset!\r\n");
		er_return = E_SYS;
	}
#else
	nue2_wait_ll_frameend(FALSE);
	er_return = nue2_ll_pause();
#endif

	return er_return;
}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER kdrv_ai_preproc_run(KDRV_AI_PREPROC_INFO *p_preproc_info)
{
	ER er_return = E_OK;
	UINT32 func_idx = 0;
	UINT32 func_en = 0;
	KDRV_AI_PREPROC_PARM* p_preproc_input_parm = NULL;
	NUE2_PARM* p_preproc_eng_parm = NULL;
	
	if (p_preproc_info == NULL) {
		DBG_ERR("kdrv_ai_preproc_run: NULL input\r\n");
		return E_NOEXS;
	}
	
	if (p_preproc_info->input_parm_pa == 0 || p_preproc_info->eng_parm_pa == 0) {
		DBG_ERR("kdrv_ai_preproc_run: NULL input addr in parameter\r\n");
		return E_NOEXS;
	}
	
	// mapping kernel va for input_parm
	p_preproc_info->input_parm_va = kdrv_ai_pa2va_remap(p_preproc_info->input_parm_pa, p_preproc_info->input_parm_size);
	if (p_preproc_info->input_parm_va == 0) {
		DBG_ERR("kdrv_ai_preproc_run: remap fail in input_parm_pa\r\n");
		return E_NOEXS;
	}
	p_preproc_input_parm = (KDRV_AI_PREPROC_PARM*)p_preproc_info->input_parm_va;
	
	// mapping kernel va for eng_parm
	p_preproc_info->eng_parm_va = kdrv_ai_pa2va_remap(p_preproc_info->eng_parm_pa, p_preproc_info->eng_parm_size);
	if (p_preproc_info->eng_parm_va == 0) {
		DBG_ERR("kdrv_ai_preproc_run: remap fail in eng_parm_pa\r\n");
		return E_NOEXS;
	}
	p_preproc_eng_parm = (NUE2_PARM*)p_preproc_info->eng_parm_va;
	
	
	for (func_idx = 0; func_idx < KDRV_AI_PREPROC_FUNC_CNT; func_idx++) {
		if (p_preproc_input_parm->func_list[func_idx] == 0) {
			break;
		}
		func_en |= p_preproc_input_parm->func_list[func_idx];
	}
	
	er_return = kdrv_ai_tran_nue2_preproc_parm(p_preproc_input_parm, p_preproc_eng_parm, func_en);
	if (er_return != E_OK) {
		DBG_ERR("kdrv_ai_preproc_run: parse nue2 parameter fail\r\n");
		goto PREPROC_RUN_EXIT;
	}
	
	er_return = nue2_open(NULL);
	if (er_return != E_OK) {
		DBG_ERR("kdrv_ai_preproc_run: nue2 open fail\r\n");
		goto PREPROC_RUN_EXIT;
	}
	
	p_preproc_eng_parm->dmaio_addr.dma_do_not_sync = 1;
	p_preproc_eng_parm->dmaio_addr.is_pa = 1;
	er_return = nue2_setmode(NUE2_OPMODE_USERDEFINE, p_preproc_eng_parm);
	if (er_return != E_OK) {
		DBG_ERR("kdrv_ai_preproc_run: nue2 setmode fail\r\n");
		goto PREPROC_RUN_EXIT;
	}
	
	er_return = nue2_start();
	if (er_return != E_OK) {
		DBG_ERR("kdrv_ai_preproc_run: nue2 start fail\r\n");
		goto PREPROC_RUN_EXIT;
	}
	
PREPROC_RUN_EXIT:
	kdrv_ai_pa2va_unmap(p_preproc_info->input_parm_va, p_preproc_info->input_parm_pa);
	kdrv_ai_pa2va_unmap(p_preproc_info->eng_parm_va, p_preproc_info->eng_parm_pa);
	return er_return;
}
#if KDRV_AI_SUPPORT_NUELL
ER kdrv_ai_construct_nue_fc_cmd(KDRV_AI_FC_INFO *p_fc_info)
{
	KDRV_AI_FC_PARM* p_fc_input_parm = NULL;
	NUE_PARM* p_nue_parm = NULL;
	UINT32 run_idx = 0;
	UINT32 run_time = 0;
	UINT32 tmp_h = 0;
	UINT64* p_llcmd = NULL;
	UINT32 llcmd_idx = 0;
	UINT32 input_parm_va = 0;
	UINT32 eng_parm_va = 0;
	UINT32 cmd_va = 0;
	
	if (p_fc_info == NULL) {
		DBG_ERR("kdrv_ai_construct_nue_fc_cmd: NULL input\r\n");
		return E_NOEXS;
	}
	
	if (p_fc_info->input_parm_pa == 0 || p_fc_info->eng_parm_pa == 0 || p_fc_info->cmd_pa == 0) {
		DBG_ERR("kdrv_ai_construct_nue_fc_cmd: NULL input addr in parameter\r\n");
		return E_NOEXS;
	}
	
	//DBG_ERR("in kernel:input_parm_pa = 0x%08X, eng pa = 0x%08X, cmd pa = 0x%08X\n", (unsigned int)p_fc_info->input_parm_pa,
	//(unsigned int)p_fc_info->eng_parm_pa, (unsigned int)p_fc_info->cmd_pa);
	
	// mapping kernel va for input_parm
	if (p_fc_info->input_parm_va == 0) {
		input_parm_va = kdrv_ai_pa2va_remap(p_fc_info->input_parm_pa, p_fc_info->input_parm_size);
	} else {
		input_parm_va = p_fc_info->input_parm_va;
	}
	if (input_parm_va == 0) {
		DBG_ERR("kdrv_ai_construct_nue_fc_cmd: remap fail in input_parm_pa\r\n");
		return E_NOEXS;
	}
	p_fc_input_parm = (KDRV_AI_FC_PARM*)input_parm_va;
	
	// mapping kernel va for eng_parm
	if (p_fc_info->eng_parm_va == 0) {
		eng_parm_va = kdrv_ai_pa2va_remap(p_fc_info->eng_parm_pa, p_fc_info->eng_parm_size);
	} else {
		eng_parm_va = p_fc_info->eng_parm_va;
	}
	if (eng_parm_va == 0) {
		DBG_ERR("kdrv_ai_construct_nue_fc_cmd: remap fail in eng_parm_pa\r\n");
		return E_NOEXS;
	}
	p_nue_parm = (NUE_PARM*)eng_parm_va;
	
	// parsing parameter to driver form
	p_fc_input_parm->func_list[0] = KDRV_AI_FC_FULLY_EN;
	p_fc_input_parm->func_list[1] = 0;
	p_fc_input_parm->src_fmt  = AI_FC_SRC_FEAT;
	p_fc_input_parm->out_type = AI_IO_INT8;
	p_fc_input_parm->out_addr = 0;
	p_fc_input_parm->size.width   = 1;
	p_fc_input_parm->size.height  = 1;
	p_fc_input_parm->size.channel = 1;
	p_fc_input_parm->batch_num = 1;
	p_fc_input_parm->in_interm_addr    = 0;
	p_fc_input_parm->in_interm_dma_en  = 0;
	p_fc_input_parm->out_interm_dma_en = 3;
	p_fc_input_parm->in_interm_ofs.line_ofs = 1;              
	p_fc_input_parm->in_interm_ofs.channel_ofs = 1;      
	p_fc_input_parm->in_interm_ofs.batch_ofs = 256;
	p_fc_input_parm->fc_ker.sclshf.in_shift  = 0;
	p_fc_input_parm->fc_ker.sclshf.out_shift = 0;
	p_fc_input_parm->fc_ker.sclshf.in_scale  = 1;
	p_fc_input_parm->fc_ker.sclshf.out_scale = 1;
	p_fc_input_parm->fc_ker.bias_addr = 0;
	p_fc_input_parm->act.mode = AI_ACT_RELU;
	p_fc_input_parm->act.relu.leaky_val = 0;
	p_fc_input_parm->act.relu.leaky_shf = 0;
	p_fc_input_parm->act.neg_en = 0;
	p_fc_input_parm->act.act_shf0 = 0;
	p_fc_input_parm->act.act_shf1 = 0;
	p_fc_input_parm->act.sclshf.in_shift  = 0;
	p_fc_input_parm->act.sclshf.out_shift = 0;
	p_fc_input_parm->act.sclshf.in_scale  = 1;
	p_fc_input_parm->act.sclshf.out_scale = 1;
	p_fc_input_parm->fcd.func_en = KDRV_AI_FCD_QUANTI_EN;
	p_fc_input_parm->fcd.quanti_kmeans_addr = 0;
	p_fc_input_parm->fcd.enc_bit_length = 0;
	p_fc_input_parm->fcd.vlc_code_size = 0;
	p_fc_input_parm->fcd.vlc_valid_size = 0;
	p_fc_input_parm->fcd.vlc_ofs_size = 0;
	kdrv_ai_tran_nue_fc_parm(p_fc_input_parm, p_nue_parm, KDRV_AI_FC_FULLY_EN);
	
	// calculate process number
	run_time = (p_nue_parm->insvm_size.sv_h / 256) + (((p_nue_parm->insvm_size.sv_h % 256) > 0)?1:0);
	
	// mapping kernel va for ll cmd buffer
	p_fc_info->cmd_size = (10 + 5*run_time)*sizeof(UINT64);
	cmd_va = kdrv_ai_pa2va_remap(p_fc_info->cmd_pa, p_fc_info->cmd_size);
	if (cmd_va == 0) {
		DBG_ERR("kdrv_ai_construct_nue_fc_cmd: remap fail in cmd_pa\r\n");
		return E_NOEXS;
	}
	p_llcmd = (UINT64*)cmd_va;
	
	// construct ll cmd
	tmp_h = p_fc_input_parm->fc_ker.weight_h;
	for (run_idx = 0; run_idx < run_time; run_idx++) {
		p_nue_parm->insvm_size.sv_h = (tmp_h < 256)?tmp_h:256;
		p_nue_parm->dmaio_addr.addr_out  = p_fc_input_parm->out_interm_addr + run_idx*4*256;
		p_nue_parm->dmaio_addr.addrin_sv = p_fc_input_parm->fc_ker.weight_addr + run_idx*p_nue_parm->insvm_size.sv_w*256;
		p_nue_parm->svm_parm.fcd_parm.fcd_enc_bit_length = p_fc_input_parm->fc_ker.weight_w*p_nue_parm->insvm_size.sv_h*8;

		if (run_idx == 0) {
			// SAI0
			p_llcmd[llcmd_idx++] = nue_ll_upd_cmd(0xF, 0x0c, p_nue_parm->dmaio_addr.addrin0);
			
			// input size reg1
			p_llcmd[llcmd_idx++] = nue_ll_upd_cmd(0xF, 0x44, p_nue_parm->insvm_size.insize | (p_nue_parm->insvm_size.channel << 16));
			
			// input size reg2
			p_llcmd[llcmd_idx++] = nue_ll_upd_cmd(0xF, 0x48, p_nue_parm->insvm_size.objnum | (0x1 << 16));
			
			// mode reg0
			p_llcmd[llcmd_idx++] = nue_ll_upd_cmd(0xF, 0x04, 0x110);
			
			// mode reg1
			p_llcmd[llcmd_idx++] = nue_ll_upd_cmd(0xF, 0x08, 0x200350);
			
			// scale shift reg0
			p_llcmd[llcmd_idx++] = nue_ll_upd_cmd(0xF, 0xF0, 0x1000);
			
			// kernel reg0
			p_llcmd[llcmd_idx++] = nue_ll_upd_cmd(0xF, 0x4C, p_nue_parm->svm_parm.svmker_parms.gv | (p_nue_parm->svm_parm.svmker_parms.gv_shf << 16)
			| (p_nue_parm->svm_parm.svmker_parms.degree << 20));
			
			// kernel reg1
			p_llcmd[llcmd_idx++] = nue_ll_upd_cmd(0xF, 0x50, p_nue_parm->svm_parm.svmker_parms.ft_shf << 24);
		
			// relu reg0
			p_llcmd[llcmd_idx++] = nue_ll_upd_cmd(0xF, 0x54, 0);
		
			// result reg0
			p_llcmd[llcmd_idx++] = nue_ll_upd_cmd(0xF, 0x58, 0);
		
		}
		
		// SAOR
		p_llcmd[llcmd_idx++] = nue_ll_upd_cmd(0xF, 0x2c, p_nue_parm->dmaio_addr.addr_out);
		
		// SAISV
		p_llcmd[llcmd_idx++] = nue_ll_upd_cmd(0xF, 0x14, p_nue_parm->dmaio_addr.addrin_sv);
		
		// input size reg0
		p_llcmd[llcmd_idx++] = nue_ll_upd_cmd(0xF, 0x40, p_nue_parm->insvm_size.sv_w | (p_nue_parm->insvm_size.sv_h << 16));
		
		// fcd bit length
		p_llcmd[llcmd_idx++] = nue_ll_upd_cmd(0xF, 0xF4, p_nue_parm->svm_parm.fcd_parm.fcd_enc_bit_length);
		
		// set nextll or null
		if (run_idx == run_time - 1) {
			p_llcmd[llcmd_idx++] = nue_ll_null_cmd(run_idx);
		} else {
			p_llcmd[llcmd_idx] = nue_ll_nextll_cmd(p_fc_info->cmd_pa + (llcmd_idx+1)*sizeof(UINT64), run_idx);
			llcmd_idx++;
		}
		tmp_h -= p_nue_parm->insvm_size.sv_h;
	}
	// flush cmd buffer for hw usage
	vos_cpu_dcache_sync(cmd_va, llcmd_idx*sizeof(UINT64), VOS_DMA_TO_DEVICE);

	// unmap va
	// input parm
	if (p_fc_info->input_parm_va == 0) {
		kdrv_ai_pa2va_unmap(input_parm_va, p_fc_info->input_parm_pa);
	}
	// eng parm
	if (p_fc_info->eng_parm_va == 0) {
		kdrv_ai_pa2va_unmap(eng_parm_va, p_fc_info->eng_parm_pa);
	}
	// ll cmd
	kdrv_ai_pa2va_unmap(cmd_va, p_fc_info->cmd_pa);
	
	return E_OK;
}
#endif
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

#ifdef __KERNEL__

EXPORT_SYMBOL(kdrv_ai_cnn_ll_trig);
EXPORT_SYMBOL(kdrv_ai_cnn_ll_done);
EXPORT_SYMBOL(kdrv_ai_nue_ll_trig);
EXPORT_SYMBOL(kdrv_ai_nue_ll_done);
EXPORT_SYMBOL(kdrv_ai_nue2_ll_trig);
EXPORT_SYMBOL(kdrv_ai_nue2_ll_done);
//MODULE_AUTHOR("Novatek Microelectronics Corp.");
//MODULE_LICENSE("NVT");
//MODULE_LICENSE("GPL");

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
EXPORT_SYMBOL(kdrv_ai_roipool_trig);
EXPORT_SYMBOL(kdrv_ai_svm_trig);
EXPORT_SYMBOL(kdrv_ai_reorg_done);
EXPORT_SYMBOL(kdrv_ai_permute_trig);
EXPORT_SYMBOL(kdrv_ai_neural_trig);
EXPORT_SYMBOL(kdrv_ai_fc_trig);
EXPORT_SYMBOL(kdrv_ai_roipool_done);
EXPORT_SYMBOL(kdrv_ai_neural_done);
EXPORT_SYMBOL(kdrv_ai_fc_done);
EXPORT_SYMBOL(kdrv_ai_reorg_trig);
EXPORT_SYMBOL(kdrv_ai_svm_done);
EXPORT_SYMBOL(kdrv_ai_permute_done);
EXPORT_SYMBOL(kdrv_ai_softmax_trig);
EXPORT_SYMBOL(kdrv_ai_softmax_done);
EXPORT_SYMBOL(kdrv_ai_anchor_trig);
EXPORT_SYMBOL(kdrv_ai_anchor_done);
EXPORT_SYMBOL(kdrv_ai_preproc_trig);
EXPORT_SYMBOL(kdrv_ai_preproc_done);
#endif

#endif
