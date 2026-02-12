/**
	@brief Source file of vendor user-space net flow sample.

	@file net_flow_user_sample.c

	@ingroup net_flow_user_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/platform.h"
#include <stdio.h>
#include <string.h>
#include "hd_type.h"
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#include "hd_common.h"
#endif
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#include "vendor_common.h"
#endif
#include "../include/vendor_ai_net_util.h" //for debug

#include "ai_ioctl.h"
#include "nvt_ai.h"
#include "kdrv_ai.h"
#include "vendor_ai_dla/vendor_ai_dla.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_dsp/vendor_ai_dsp.h"
#include "vendor_ai.h"
#include "kflow_ai_net/kflow_ai_net.h"
#include "kflow_ai_net/kflow_ai_net_comm.h"
#include "kflow_ai_net/nn_net.h"
#include "kflow_ai_net/nn_parm.h"
#include "vendor_ai_net_flow.h"
//#include "vendor_ai_net_group.h"
#include "vendor_ai_net_debug.h"
//#include "vendor_ai_net_cmd.h"
#include "vendor_ai_comm.h"
#include "vendor_ai_comm_flow.h"

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#include "kflow_common/nvtmpp.h"
#include "kflow_common/nvtmpp_ioctl.h"
#endif

//=============================================================
#define __CLASS__ 				"[ai][lib][op]"
#include "vendor_ai_debug.h"
//=============================================================


#include <pthread.h>			//for pthread API
#if defined(_BSP_NA51090_)
#include "vendor_pcie.h"
#endif


/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

typedef struct _VENDOR_AI_PROC_WORK {	
	NN_GEN_MODE_CTRL mctrl;
	VENDOR_AI_APP_HEAD head;
	VENDOR_AI_LL_HEAD ll_head;
	UINT32 data[256]; // 1K for VENDOR_AI_FC_PARM	
} VENDOR_AI_OP_WORK;


extern VENDOR_AIS_FLOW_MAP_MEM_PARM *g_map_mem;
extern UINT32 g_ai_support_net_max;
extern BOOL is_multi_process;


#if defined(_BSP_NA51068_)
static VENDOR_AI_NET_MEM_INFO g_mem_info = {0};
#endif


HD_RESULT _vendor_ai_fill_fc_parm(VENDOR_AI_FC_PARM *p_parm, VENDOR_AI_NET_INFO_PROC *p_proc) 
{
	HD_RESULT rv = HD_OK;

	p_parm->in_type = AI_IO_INT8;
	p_parm->in_addr = p_proc->input_img.pa; 			   //< input address	 (size = SV_FEA_LENGTH bytes)
	p_parm->out_interm_addr   = p_proc->output_img.pa;	   //< output address	 (size = SV_LENGTH*4 bytes)
	//p_parm->out_addr =0?
	//p_parm->in_interm_addr =0?
	//p_parm->fc_ker.bias_addr =0?
	p_parm->fc_ker.weight_addr = p_proc->input2_img.pa;    //< sv weight address (size = SV_LENGTH*SV_FEA_LENGTH bytes)
	p_parm->fc_ker.weight_w = p_proc->input2_img.width;    //< sv weight width
	p_parm->fc_ker.weight_h = p_proc->input2_img.height;   //< sv weight height
	//p_parm->fcd.quanti_kmeans_addr =0?
	//p_parm->fcd.p_vlc_code = 0
	//p_parm->fcd.p_vlc_valid = 0
	//p_parm->fcd.p_vlc_ofs = 0
	{
		INT32 svm_shift, def_shf;
		INT32 in_bit_num = HD_VIDEO_PXLFMT_BITS(p_proc->input_img.fmt);
		INT32 in_sign_bit_num = HD_VIDEO_PXLFMT_SIGN(p_proc->input_img.fmt);
		//INT32 in_int_bit_num = HD_VIDEO_PXLFMT_INT(p_proc->input_img.fmt);
		INT32 in_frac_bit_num = HD_VIDEO_PXLFMT_FRAC(p_proc->input_img.fmt);
		INT32 kernel_bit_num = HD_VIDEO_PXLFMT_BITS(p_proc->input2_img.fmt);
		INT32 kernel_sign_bit_num = HD_VIDEO_PXLFMT_SIGN(p_proc->input2_img.fmt);
		//INT32 kernel_int_bit_num = HD_VIDEO_PXLFMT_INT(p_proc->input2_img.fmt);
		INT32 kernel_frac_bit_num = HD_VIDEO_PXLFMT_FRAC(p_proc->input2_img.fmt);
		INT32 out_bit_num = HD_VIDEO_PXLFMT_BITS(p_proc->output_img.fmt);
		INT32 out_sign_bit_num = HD_VIDEO_PXLFMT_SIGN(p_proc->output_img.fmt);
		//INT32 out_int_bit_num = HD_VIDEO_PXLFMT_INT(p_proc->output_img.fmt);
		INT32 out_frac_bit_num = HD_VIDEO_PXLFMT_FRAC(p_proc->output_img.fmt);

		//DBGD(in_bit_num);
		//DBGD(in_sign_bit_num);
		//DBGD(in_frac_bit_num);
		//DBGD(kernel_bit_num);
		//DBGD(kernel_sign_bit_num);
		//DBGD(kernel_frac_bit_num);
		//DBGD(out_bit_num);
		//DBGD(out_sign_bit_num);
		//DBGD(out_frac_bit_num);
		if ((in_bit_num != 8) || (kernel_bit_num != 8) || (out_bit_num != 32)) {
			DBG_ERR("not support fmt bits (%d, %d, %d)!\n", (int)in_bit_num, (int)kernel_bit_num, (int)out_bit_num);
			rv = HD_ERR_NG;
			goto exit;
		}
		if ((in_sign_bit_num != 1) || (kernel_sign_bit_num != 1) || (out_sign_bit_num != 1)) {
			DBG_ERR("not support sign bits (%d, %d, %d)!\n", (int)in_bit_num, (int)kernel_bit_num, (int)out_bit_num);
			rv = HD_ERR_NG;
			goto exit;
		}
		/*
		if ((in_int_bit_num==0) && (in_frac_bit_num==0)) {
			in_int_bit_num = in_bit_num-in_sign_bit_num-in_frac_bit_num;
		}
		if ((kernel_int_bit_num==0) && (kernel_frac_bit_num==0)) {
			kernel_int_bit_num = kernel_bit_num-kernel_sign_bit_num-kernel_frac_bit_num;
		}
		if ((out_int_bit_num==0) && (out_frac_bit_num==0)) {
			out_int_bit_num = out_bit_num-out_sign_bit_num-out_frac_bit_num;
		}
		*/

		// auto calc shfit param - begin
		svm_shift = (in_frac_bit_num - (!in_sign_bit_num) + 4) + (kernel_frac_bit_num - (!kernel_sign_bit_num) + 4) - out_frac_bit_num;

		// weight_shf
		def_shf = 5; 
		p_parm->fc_ker.weight_shf = def_shf;
		svm_shift -= p_parm->fc_ker.weight_shf;
		 
		// norm_scl+acc_shf
		if (svm_shift < 0) {
			p_parm->fc_ker.norm_scl = MIN(1 << (-svm_shift), (1 << 12)); // max norm_scl = (1 << 12)
			p_parm->fc_ker.acc_shf = 0;
			svm_shift -= MAX(svm_shift, -12); // max norm_scl = (1 << 12)
		}
		else {
			p_parm->fc_ker.norm_scl = 1;
			p_parm->fc_ker.acc_shf = MIN(svm_shift, 15); //max acc_shf = 15
			svm_shift -= p_parm->fc_ker.acc_shf;
		}
		 
		if ((svm_shift < 0) || (svm_shift > (15 - def_shf))) {
			DBG_WRN("FC: svm shift(%d) exceed range...\n", (int)svm_shift);
		}
		 
		p_parm->fc_ker.weight_shf = MIN(p_parm->fc_ker.weight_shf + svm_shift, 15); // max weight_shf = 15
		p_parm->fc_ker.norm_shf = 3; // not used

		//DBGD(p_parm->fc_ker.weight_shf);
		//DBGD(p_parm->fc_ker.acc_shf);
		//DBGD(p_parm->fc_ker.norm_scl);
		//DBGD(p_parm->fc_ker.norm_shf);
		
		// auto calc shfit param - end
	}
exit:
	return rv;
}

static void _vendor_ai_set_kdrv_preproc_addr_ofs(VENDOR_AI_BUF *img, VENDOR_PREPROC_PARM *p_parm, UINT32 in_or_out, UINT32 io_idx)
{   
    if(in_or_out == 0){
	    p_parm->in_addr[io_idx] = img->pa;
        p_parm->in_ofs[io_idx].line_ofs = img->line_ofs;
        //p_parm->in_ofs[io_idx].channel_ofs = img->channel_ofs;
        //p_parm->in_ofs[io_idx].batch_ofs = img->batch_ofs;
    } else {
        p_parm->out_addr[io_idx] = img->pa;
        p_parm->out_ofs[io_idx].line_ofs = img->line_ofs;
        //p_parm->out_ofs[io_idx].channel_ofs = img->channel_ofs;
        //p_parm->out_ofs[io_idx].batch_ofs = img->batch_ofs;
    }        
}
HD_RESULT _vendor_ai_set_preproc_addr_ofs(VENDOR_AI_BUF *img, VENDOR_PREPROC_PARM *p_parm, UINT32 in_or_out)
{
	if (img == NULL) {
		if (in_or_out == 0) {
			DBG_ERR("invalid input addr (%08x)!\n", (unsigned int)img);
		} else {
			DBG_ERR("invalid output addr (%08x)!\n", (unsigned int)img);
		}
		return HD_ERR_NULL_PTR;
	}
    switch ((INT32)img->fmt) {
		case HD_VIDEO_PXLFMT_R8:
            _vendor_ai_set_kdrv_preproc_addr_ofs(img, p_parm, in_or_out, 0);
            break;
        case HD_VIDEO_PXLFMT_G8:
            _vendor_ai_set_kdrv_preproc_addr_ofs(img, p_parm, in_or_out, 1);
            break;
        case HD_VIDEO_PXLFMT_B8:
            _vendor_ai_set_kdrv_preproc_addr_ofs(img, p_parm, in_or_out, 2);
            break;
        case HD_VIDEO_PXLFMT_Y8:
            _vendor_ai_set_kdrv_preproc_addr_ofs(img, p_parm, in_or_out, 0);
            break;
        case HD_VIDEO_PXLFMT_UV:
            _vendor_ai_set_kdrv_preproc_addr_ofs(img, p_parm, in_or_out, 1);
            break;
        default:
        {
            DBG_ERR("Unknown PXLFMT = %li!\r\n",(INT32)img->fmt);
			return HD_ERR_LIMIT;
		}
        break;
    }
    return HD_OK;
}

static void _vendor_ai_job_select_eng2(NN_GEN_MODE_CTRL *p_this_mctrl, UINT32* p_eng_id, UINT32* p_eng_op)
{
	UINT32 engine_id;
	UINT32 engine_op;

	if ((p_this_mctrl->eng == NN_GEN_ENG_CNN) || (p_this_mctrl->eng == NN_GEN_ENG_CNN2)) {
		engine_id = VENDOR_AI_ENGINE_CNN; engine_op = ((p_this_mctrl->trig_src << 16) | (p_this_mctrl->mode));
	} else if (p_this_mctrl->eng == NN_GEN_ENG_NUE) {
#if (CNN_NUE_EXCLUSIVE == 1)
		//running this job by CNN engine, and mark it with 0x00008000 for NUE mode
		engine_id = VENDOR_AI_ENGINE_CNN; engine_op = ((p_this_mctrl->trig_src << 16) | (p_this_mctrl->mode) | 0x00008000);
#else
		engine_id = VENDOR_AI_ENGINE_NUE; engine_op = ((p_this_mctrl->trig_src << 16) | (p_this_mctrl->mode));
#endif
	} else if (p_this_mctrl->eng == NN_GEN_ENG_NUE2) {
		engine_id = VENDOR_AI_ENGINE_NUE2; engine_op = ((p_this_mctrl->trig_src << 16) | (p_this_mctrl->mode));
	} else if (p_this_mctrl->eng == NN_GEN_ENG_CPU) {
		engine_id = VENDOR_AI_ENGINE_CPU; engine_op = ((p_this_mctrl->trig_src << 16) | (p_this_mctrl->mode));
	} else if (p_this_mctrl->eng == NN_GEN_ENG_DSP) {
		engine_id = VENDOR_AI_ENGINE_DSP; engine_op = ((p_this_mctrl->trig_src << 16) | (p_this_mctrl->mode));
	} else {
		engine_id = VENDOR_AI_ENGINE_UNKNOWN; engine_op = 0;
	}
	if (p_eng_id) p_eng_id[0] = engine_id;
	if (p_eng_op) p_eng_op[0] = engine_op;
}

#if defined(_BSP_NA51068_) || defined(_BSP_NA51090_)
HD_RESULT _vendor_ai_op_mem_alloc(UINT32 *pa, void **va, UINT32 size, HD_COMMON_MEM_VB_BLK *blk, HD_COMMON_MEM_DDR_ID ddr)
{
	HD_RESULT ret = HD_OK;

	// get blk
	*blk = hd_common_mem_get_block(HD_COMMON_MEM_CNN_POOL, size, ddr);
	if (HD_COMMON_MEM_VB_INVALID_BLK == *blk) {
		printf("%s: get block fail\r\n", __func__);
		ret = HD_ERR_NG;
		goto exit;
	}

	// get pa
	*pa = hd_common_mem_blk2pa(*blk);
	if (*pa == 0) {
		printf("%s: blk2pa fail, blk = %#lx\r\n", __func__, *blk);
		hd_common_mem_release_block(*blk);
		ret = HD_ERR_NG;
		goto exit;
	}

	// get va
	/* Must use "HD_COMMON_MEM_MEM_TYPE_CACHE", or it will cause the cpu layer to perform inefficiently */
	*va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, *pa, size);
	if (va == 0) {
		printf("%s: va = 0, mem_mmap fail\r\n", __func__);
		ret = hd_common_mem_munmap((void *)*va, size);
		if (ret != HD_OK) {
			printf("%s: mem_munmap fail\r\n", __func__);
			goto exit;
		}
		ret = HD_ERR_NG;
		goto exit;
	}

	//printf("%s:%d va(0x%lx), blk(0x%lx), size(0x%lx)\r\n", __func__, __LINE__, (UINT32)*va, *blk, size);

exit:
	return ret;
}

HD_RESULT _vendor_ai_op_mem_free(UINT32 phy_addr, void *virt_addr, UINT32 size, HD_COMMON_MEM_VB_BLK blk)
{
	HD_RESULT ret = HD_OK;

	//printf("%s:%d va(0x%lx), blk(0x%lx), size(0x%lx)\r\n", __func__, __LINE__, (UINT32)virt_addr, blk, size);

	// unmap va
	if (virt_addr) {
		ret = hd_common_mem_munmap(virt_addr, size);
		if (ret != HD_OK) {
			printf("%s: mem_munmap fail\n", __func__);
		}
	}

	// release blk
	ret = hd_common_mem_release_block(blk);
	if (ret != HD_OK) {
		printf("%s: release_block fail, blk = 0x%lx\n", __func__, blk);
	}

	return ret;
}
#endif

HD_RESULT _vendor_ai_op_work_buf_alloc(UINT32 proc_id, UINT32 n)
{
	UINT32 pa = 0;
	void  *va = NULL;
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(proc_id);
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager = &p_proc->mem_manager;
	// size = 1 NN_GEN_MODE_CTRL + 1 VENDOR_AI_APP_HEAD + 1K for param of op
	UINT32 parm_sz = ALIGN_CEIL_32(sizeof(VENDOR_AI_OP_WORK) * n);
#if defined(_BSP_NA51068_) || defined(_BSP_NA51090_)
	HD_COMMON_MEM_VB_BLK blk = 0;
#endif
	if (parm_sz == 0) {
		DBG_ERR("proc_id(%lu) req_size = 0?\n", proc_id);
		return HD_ERR_FAIL;
	}

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	{
		CHAR   pool_name[24] = {0};
		snprintf(pool_name, 24, "ai.proc[%lu].work", proc_id);
		rv = hd_common_mem_alloc((CHAR *)pool_name, &pa, &va, parm_sz * 2, p_cfg->buf_opt.ddr_id);
		if (pa % 32 != 0) {
			DBG_WRN("io_buf start address pa(0x%08x) is NOT 32x align !!\n", (UINT)pa);
		}
		//DBGH(pa);
		//DBGH(va);
		if (rv != HD_OK) {
			DBG_ERR("proc_id(%lu) alloc memory fail, need size = %lu\n", proc_id, parm_sz);
			return HD_ERR_NOBUF;
		}
	}
#else
	rv = _vendor_ai_op_mem_alloc(&pa, &va, parm_sz * 2, &blk, p_proc->buf_opt.ddr_id);
	if (rv != HD_OK) {
		DBG_ERR("proc_id(%lu) alloc memory fail, need size = %lu\n", proc_id, parm_sz * 2);
		return HD_ERR_NOBUF;
	}
#endif	
	// backup alloc memory info ,so we can do free at close()
	p_proc->priv.init_buf.pa   = (UINT32)pa;
	p_proc->priv.init_buf.va   = (UINT32)va;
	p_proc->priv.init_buf.size = parm_sz * 2;
#if defined(_BSP_NA51068_)
	p_proc->priv.init_buf.blk  = (INT32)blk;
#endif

	p_mem_manager->user_parm.pa     = (UINT32)pa;
	p_mem_manager->user_parm.va     = (UINT32)va;
	p_mem_manager->user_parm.size   = parm_sz;
	p_mem_manager->user_model.pa     = (UINT32)pa;
	p_mem_manager->user_model.va     = (UINT32)va;
	p_mem_manager->user_model.size   = parm_sz;
	p_mem_manager->user_buff.pa     = (UINT32)pa;
	p_mem_manager->user_buff.va     = (UINT32)va;
	p_mem_manager->user_buff.size   = parm_sz;
	
	p_mem_manager->kerl_parm.pa		= ALIGN_CEIL_32(p_mem_manager->user_parm.pa + parm_sz);
	p_mem_manager->kerl_parm.va		= ALIGN_CEIL_32(p_mem_manager->user_parm.va + parm_sz);
	p_mem_manager->kerl_parm.size	= parm_sz;

	return HD_OK;
}

HD_RESULT _vendor_ai_op_work_buf_free(UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
#if defined(_BSP_NA51055_) ||  defined(_BSP_NA51089_)
	rv = hd_common_mem_free((UINT32)p_proc->priv.init_buf.pa, (void *)p_proc->priv.init_buf.va);
#else
	rv = _vendor_ai_op_mem_free(p_proc->priv.init_buf.pa, (void *)p_proc->priv.init_buf.va,
								 p_proc->priv.init_buf.size, (HD_COMMON_MEM_VB_BLK)p_proc->priv.init_buf.blk);
	if (rv != HD_OK) {
		DBG_ERR("proc_id(%lu) free init memory fail, ret = %d\n", proc_id, rv);
	}
	p_proc->priv.init_buf.blk = 0;
#endif
	p_proc->priv.init_buf.pa = 0;
	p_proc->priv.init_buf.va = 0;
	p_proc->priv.init_buf.size = 0;
	p_proc->mem_manager.user_parm.pa = 0;
    p_proc->mem_manager.user_parm.va = 0;
 
	return rv;
}

HD_RESULT _vendor_ai_op_gen_init(VENDOR_AIS_FLOW_MAP_MEM_PARM map_mem, UINT32 proc_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc;
#else
#endif
	VENDOR_AIS_FLOW_MAP_MEM_INFO map_mem_info;
	HD_RESULT ret;

	/*
		init memory size
	*/
	memcpy(&map_mem_info.parm, &map_mem, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM));
	map_mem_info.net_id = proc_id;
	/*
		open vendor ai
	*/
	ret = vendor_ais_open_net(proc_id);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_open_net error %d\r\n", ret);
		return HD_ERR_ABORT;
	}
	if (is_multi_process) {
        vendor_ai_dla_open_path(proc_id);
    }
	/*
	    map memory with kernel flow and get kernel va
	*/
	ret = vendor_ais_remap_kerl_mem(&map_mem_info);
	if (ret != HD_OK) {
		return ret;
	}
#if (NET_GEN_HEAP == 1)
	p_proc = _vendor_ai_info(proc_id);
	p_proc->map_mem = map_mem_info.parm;
#else
	g_map_mem[proc_id] = map_mem_info.parm;
#endif
	
	return HD_OK;
}

HD_RESULT _vendor_ai_op_gen_uninit(UINT32 proc_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc;
#else
#endif
	VENDOR_AIS_FLOW_MAP_MEM_INFO map_mem_info;
#if (NET_GEN_HEAP == 1)
	p_proc = _vendor_ai_info(proc_id);
	map_mem_info.parm = p_proc->map_mem;
#else
	map_mem_info.parm = g_map_mem[proc_id];
#endif
	map_mem_info.net_id = proc_id;

	vendor_ais_unmap_kerl_mem(&map_mem_info);

    if (is_multi_process) {
        vendor_ai_dla_close_path(proc_id);
    }
	
	vendor_ais_close_net(proc_id);

	return HD_OK;
}

HD_RESULT vendor_ai_op_open(UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	//VENDOR_AI_NET_INFO_PROC *p_proc = NULL;

	vendor_ais_lock_net(proc_id);

#if (NET_OPEN_HEAP == 1)
	rv = _vendor_ai_alloc_proc(proc_id);
	if (rv != HD_OK) {
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_NOMEM;
	}
#endif
	
	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
#if (NET_OPEN_HEAP == 1)
		_vendor_ai_free_proc(proc_id);
#endif
		vendor_ais_unlock_net(proc_id);
		return rv;
	}

	//p_proc = _vendor_ai_info(proc_id);

	// check status
	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) {
		DBG_ERR("FATAL ERROR : module is NOT init yet, please call vendor_ai_net_init() first !!!\r\n");
#if (NET_OPEN_HEAP == 1)
		_vendor_ai_free_proc(proc_id);
#endif
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_UNINIT;
	} else if ((_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_OPEN) ||
				(_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_START)){
		DBG_ERR("module is already opened...ignore request !!\r\n");
#if (NET_OPEN_HEAP == 1)
		_vendor_ai_free_proc(proc_id);
#endif
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_ALREADY_OPEN;
	}

	// set open
	_vendor_ai_state(proc_id)[0] = VENDOR_AI_PROC_STATE_OPEN;   // [INIT] -> [OPEN]
	vendor_ais_unlock_net(proc_id);

	return rv;
}

HD_RESULT vendor_ai_op_start(UINT32 proc_id)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	HD_RESULT rv = HD_OK;
	UINT32 max_job_cnt = 1;
	UINT32 ddr_id = 0;
	UINT32 job_id;
	vendor_ais_lock_net(proc_id);
	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}
	p_proc = _vendor_ai_info(proc_id);
	//--- check status ---
	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) {
		DBG_ERR("FATAL ERROR : module is NOT init & open yet, please call vendor_ai_net_init() and vendor_ai_op_open() first !!!\r\n");
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_UNINIT;
	} else if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_INIT){
		DBG_ERR("FATAL ERROR : module is NOT open yet, please call vendor_ai_op_open() first !!!\r\n");
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_NOT_OPEN;
	} else if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_START){
		DBG_ERR("module is already start, ignore start request....\r\n");
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_ALREADY_START;
	}
	
	//check if work_buf is set by user
	if(p_proc->mem_manager.user_parm.pa == 0 && p_proc->mem_manager.user_parm.va == 0) {
		_vendor_ai_op_work_buf_alloc(proc_id, max_job_cnt);
	}
	rv = _vendor_ai_op_gen_init(p_proc->mem_manager, proc_id);
	if (rv != HD_OK) {
		DBG_ERR("proc_id(%lu) op gen init fail=%d\r\n", proc_id, rv);
		goto gen_init_fail;
	}
	//open
	rv = vendor_ai_dla_push_job(proc_id, 0xf0000001, 0); // push begin
	if (rv != HD_OK) {
		goto gen_init_fail;
	}
	job_id = 0xf0000001;
	vendor_ai_dla_pull_job(proc_id, &job_id); // pull begin
	//start
	vendor_ai_net_trace(proc_id, AI_FLOW, "start() - create_joblist (%u jobs, %u binds)\r\n", max_job_cnt, max_job_cnt);
	vendor_ai_dla_create_joblist(proc_id, max_job_cnt, max_job_cnt, max_job_cnt, ddr_id);
	//vendor_ai_net_trace(net_id, AI_FLOW, "start() - job_callback_thread_init\r\n");
	//_vendor_ai_job_callback_thread_init(net_id); //job done callback
	//vendor_ai_net_trace(net_id, AI_FLOW, "start() - cpu_callback_thread_init\r\n");
	//vendor_ai_cpu_thread_init(net_id); //cpu exec callback
	//vendor_ai_net_trace(net_id, AI_FLOW, "start() - dsp_callback_thread_init\r\n");
	//vendor_ai_dsp_thread_init(net_id); //dsp exec callback
	// set open
	_vendor_ai_state(proc_id)[0] = VENDOR_AI_PROC_STATE_START;   // [OPEN] -> [START]
	vendor_ais_unlock_net(proc_id);
	return rv;

gen_init_fail:
	_vendor_ai_op_gen_uninit(proc_id);
	
	vendor_ais_unlock_net(proc_id);
	
	return HD_ERR_FAIL;
}

HD_RESULT vendor_ai_op_stop(UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	UINT32 job_id;

	vendor_ais_lock_net(proc_id);

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		vendor_ais_unlock_net(proc_id);
		return rv;
	}

	p_proc = _vendor_ai_info(proc_id);

	//--- check status ---
	if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_START) {
		DBG_ERR("module is NOT start yet, ignore stop request.... \r\n");
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_NOT_START;
	}

	//stop
	vendor_ai_dla_destory_joblist(proc_id);
	//vendor_ai_net_trace(net_id, AI_FLOW, "stop() - job_callback_thread_exit\r\n");
	//_vendor_ai_job_callback_thread_exit(net_id); //job done callback
	//vendor_ai_net_trace(net_id, AI_FLOW, "stop() - cpu_callback_thread_exit\r\n");
	//vendor_ai_cpu_thread_exit(net_id); //cpu exec callback
	//vendor_ai_net_trace(net_id, AI_FLOW, "stop() - dsp_callback_thread_exit\r\n");
	//vendor_ai_dsp_thread_exit(net_id); //dsp exec callback

	//close
	rv = vendor_ai_dla_push_job(proc_id, 0xf0000002, 0); // push end
	if (rv != HD_OK) {
		vendor_ais_unlock_net(proc_id);
		return rv;
	}
	job_id = 0xf0000002;
	vendor_ai_dla_pull_job(proc_id, &job_id); // pull end

	_vendor_ai_op_gen_uninit(proc_id);

	if(p_proc->priv.init_buf.pa != 0 && p_proc->priv.init_buf.va != 0) {
		_vendor_ai_op_work_buf_free(proc_id);
	}

	// set close
	_vendor_ai_state(proc_id)[0] = VENDOR_AI_PROC_STATE_OPEN;  // [START] -> [OPEN]
	vendor_ais_unlock_net(proc_id);
	
	return rv;
}

HD_RESULT vendor_ai_op_close (UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	//VENDOR_AI_NET_INFO_PROC *p_proc = NULL;

	vendor_ais_lock_net(proc_id);
	
	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		vendor_ais_unlock_net(proc_id);
		return rv;
	}

	//p_proc = _vendor_ai_info(proc_id);

	//--- check status ---
	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_START) {
	    DBG_ERR("module is NOT stop yet... ignore close request !! \r\n");
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_ALREADY_START;
	} else if ((_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) ||
	           (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_INIT)) {
	    DBG_ERR("module is NOT open yet... ignore close request !! \r\n");
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_NOT_OPEN;
	}
	// set close
	_vendor_ai_state(proc_id)[0] = VENDOR_AI_PROC_STATE_INIT;  // [OPEN] -> [INIT]
	
#if (NET_OPEN_HEAP == 1)
	_vendor_ai_free_proc(proc_id);
#endif
	vendor_ais_unlock_net(proc_id);
	
	return rv;
}

HD_RESULT vendor_ai_op_get (UINT32 proc_id, VENDOR_AI_OP_PARAM_ID param_id, void* p_param)
{
	HD_RESULT rv = HD_OK;

	vendor_ais_lock_net(proc_id);
/*
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;

	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}

	p_proc = _vendor_ai_info(proc_id);

	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) {
		DBG_ERR("could only be called after INIT, please call vendor_ai_net_init() first\r\n");
		return HD_ERR_UNINIT;
	}
*/
	switch (param_id) {
		case VENDOR_AI_OP_PARAM_CFG_MAX:
			{
				VENDOR_AI_OP_CFG_MAX *p_max = (VENDOR_AI_OP_CFG_MAX *) p_param;
				/*
				if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) {
					DBG_ERR("could only be called at OPEN\r\n");
					return HD_ERR_ALREADY_OPEN;
				}
				*/
				if (p_max) {
					p_max->size = 0;
					if (p_max->op == VENDOR_AI_OP_LIST) {
						UINT32 max_job_cnt = 1;
						//VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
						rv = _vendor_ai_validate(proc_id);
						if (rv != HD_OK) {
							vendor_ais_unlock_net(proc_id);
							return rv;
						}

						//p_proc = _vendor_ai_info(proc_id);

						if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) {
							DBG_ERR("could only be called after INIT, please call vendor_ai_net_init() first\r\n");
							vendor_ais_unlock_net(proc_id);
							return HD_ERR_UNINIT;
						}
						p_max->size = ALIGN_CEIL_32(sizeof(VENDOR_AI_OP_WORK) * max_job_cnt) * 2 + ALIGN_CEIL_32(vendor_ais_net_get_fc_cmd_size(p_max->max_param[0],proc_id));
					}
					else if (p_max->op == VENDOR_AI_OP_PREPROC) {
						UINT32 max_job_cnt = 1;
						p_max->size = ALIGN_CEIL_32(sizeof(VENDOR_AI_OP_WORK) * max_job_cnt) * 2;
					}
					else if (p_max->op == VENDOR_AI_OP_FC) {
						DBG_ERR("VENDOR_AI_OP_FC is deprecated, please use VENDOR_AI_OP_LIST instead!!\r\n");
						vendor_ais_unlock_net(proc_id);
						return HD_ERR_NOT_SUPPORT;
					}
					else {
						DBG_ERR("UnSupported p_max->op !!\r\n");
						vendor_ais_unlock_net(proc_id);
						return HD_ERR_PARAM;
					}
				}
				else {
					DBG_ERR("p_param is null !!\r\n");
					vendor_ais_unlock_net(proc_id);
					return HD_ERR_PARAM;
				}
			}
			break;
		default:
			{
				DBG_ERR("param_id(%u) not support !!\r\n", param_id);
				vendor_ais_unlock_net(proc_id);
				return HD_ERR_PARAM;
			}
			break;
	}

	vendor_ais_unlock_net(proc_id);
	
	return rv;
}

HD_RESULT vendor_ai_op_set (UINT32 proc_id, VENDOR_AI_OP_PARAM_ID param_id, void* p_param)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = NULL;
	vendor_ais_lock_net(proc_id);
	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		return rv;
	}
	p_proc = _vendor_ai_info(proc_id);

	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) {
		DBG_ERR("could only be called after INIT, please call vendor_ai_net_init() first\r\n");
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_UNINIT;
	}

	switch (param_id) {
		case VENDOR_AI_OP_PARAM_CFG_WORKBUF:
			{
				UINT32 max_job_cnt = 1;
				VENDOR_AI_OP_CFG_WORKBUF *work_buf = (VENDOR_AI_OP_CFG_WORKBUF *) p_param;
				VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager = &p_proc->mem_manager;
				UINT32 pa = work_buf->pa;
				UINT32 va = work_buf->va;
				UINT32 parm_sz = ALIGN_CEIL_32(sizeof(VENDOR_AI_OP_WORK) * max_job_cnt);
				if (work_buf) {
					switch (work_buf->op) {
						case VENDOR_AI_OP_PREPROC:
						{
							if (_vendor_ai_state(proc_id)[0] != VENDOR_AI_PROC_STATE_OPEN) {
								DBG_ERR("could only be called at OPEN\r\n");
								vendor_ais_unlock_net(proc_id);
								return HD_ERR_ALREADY_OPEN;
							}
							p_mem_manager->user_parm.pa     = (UINT32)pa;
							p_mem_manager->user_parm.va     = (UINT32)va;
							p_mem_manager->user_parm.size   = parm_sz;
							p_mem_manager->user_model.pa     = (UINT32)pa;
							p_mem_manager->user_model.va     = (UINT32)va;
							p_mem_manager->user_model.size   = parm_sz;
							p_mem_manager->user_buff.pa     = (UINT32)pa;
							p_mem_manager->user_buff.va     = (UINT32)va;
							p_mem_manager->user_buff.size   = parm_sz;
							
							p_mem_manager->kerl_parm.pa		= ALIGN_CEIL_32(p_mem_manager->user_parm.pa + parm_sz);
							p_mem_manager->kerl_parm.va		= ALIGN_CEIL_32(p_mem_manager->user_parm.va + parm_sz);
							p_mem_manager->kerl_parm.size	= parm_sz;
						}
						break;
						case VENDOR_AI_OP_LIST:
						{
							if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_OPEN) {
								p_mem_manager->user_parm.pa     = (UINT32)pa;
								p_mem_manager->user_parm.va     = (UINT32)va;
								p_mem_manager->user_parm.size   = parm_sz;
								//using user_model as ll_cmd buf
								p_mem_manager->user_model.pa     = ALIGN_CEIL_32(p_mem_manager->user_parm.pa + parm_sz*2);
								p_mem_manager->user_model.va     = ALIGN_CEIL_32(p_mem_manager->user_parm.va + parm_sz*2);
								p_mem_manager->user_model.size   = work_buf->size - parm_sz*2;
								p_mem_manager->user_buff.pa     = (UINT32)pa;
								p_mem_manager->user_buff.va     = (UINT32)va;
								p_mem_manager->user_buff.size   = parm_sz;
								p_mem_manager->kerl_parm.pa		= ALIGN_CEIL_32(p_mem_manager->user_parm.pa + parm_sz);
								p_mem_manager->kerl_parm.va		= ALIGN_CEIL_32(p_mem_manager->user_parm.va + parm_sz);
								p_mem_manager->kerl_parm.size	= parm_sz;
							}
							else {
								//only update ll_cmd
								p_mem_manager->user_model.pa     = ALIGN_CEIL_32((UINT32)pa + parm_sz*2);
								p_mem_manager->user_model.va     = ALIGN_CEIL_32((UINT32)va + parm_sz*2);
								p_mem_manager->user_model.size   = work_buf->size - parm_sz*2;
							}
							/*
							DBGH(p_mem_manager->user_parm.pa);
							DBGH(p_mem_manager->user_parm.va);
							DBGH(p_mem_manager->user_parm.size);
							DBGH(p_mem_manager->user_model.pa);
							DBGH(p_mem_manager->user_model.va);
							DBGH(p_mem_manager->user_model.size);
							DBGH(p_mem_manager->user_buff.va);
							DBGH(p_mem_manager->user_buff.pa);
							DBGH(p_mem_manager->user_buff.size);
							DBGH(p_mem_manager->kerl_parm.pa);
							DBGH(p_mem_manager->kerl_parm.va);
							DBGH(p_mem_manager->kerl_parm.size);
							*/
						}
						break;
						case VENDOR_AI_OP_FC:
						{
							DBG_ERR("VENDOR_AI_OP_FC is deprecated, please use VENDOR_AI_OP_LIST instead!!\r\n");
							vendor_ais_unlock_net(proc_id);
							return HD_ERR_NOT_SUPPORT;
						}
						break;
						default:
						{
							DBG_ERR("op(%u) not support !!\r\n", work_buf->op);
							vendor_ais_unlock_net(proc_id);
							return HD_ERR_PARAM;
						}
						break;
					}
				}
				else {
					DBG_ERR("p_param is null !!\r\n");
					vendor_ais_unlock_net(proc_id);
					return HD_ERR_PARAM;
				}
			}
			break;
		default:
			{
				DBG_ERR("param_id(%u) not support !!\r\n", param_id);
				vendor_ais_unlock_net(proc_id);
				return HD_ERR_PARAM;
			}
			break;
	}
	vendor_ais_unlock_net(proc_id);
	return rv;
}

HD_RESULT vendor_ai_op_add(UINT32 proc_id, VENDOR_AI_OP aop, void* p_param, UINT32 in_cnt, VENDOR_AI_BUF* p_in_buf, UINT32 out_cnt, VENDOR_AI_BUF* p_out_buf)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_BUF* p_in0 = (in_cnt > 0) ? (p_in_buf + 0) : 0;
	VENDOR_AI_BUF* p_in1 = (in_cnt > 1) ? (p_in_buf + 1) : 0;
    //VENDOR_AI_BUF* p_in2 = (in_cnt > 2) ? (p_in_buf + 2) : 0;
	VENDOR_AI_BUF* p_out0 = (out_cnt > 0) ? (p_out_buf + 0) : 0;
    //VENDOR_AI_BUF* p_out1 = (out_cnt > 1) ? (p_out_buf + 1) : 0;
    //VENDOR_AI_BUF* p_out2 = (out_cnt > 2) ? (p_out_buf + 2) : 0;
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager = &p_proc->mem_manager;
	VENDOR_AI_OP_WORK* p_op = (VENDOR_AI_OP_WORK*)p_mem_manager->user_parm.va;
	NN_GEN_MODE_CTRL *p_mctrl = 0;

	vendor_ais_lock_net(proc_id);
	
	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		vendor_ais_unlock_net(proc_id);
		return rv;
	}
	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) {
		DBG_ERR("FATAL ERROR : module is NOT init & open & start yet, please call vendor_ai_net_init() & vendor_ai_op_open() & vendor_ai_op_start() first !!!\r\n");
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_UNINIT;
	} else if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_INIT){
		DBG_ERR("FATAL ERROR : module is NOT open & start yet, please call vendor_ai_op_open() & vendor_ai_op_start() first !!!\r\n");
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_NOT_OPEN;
	} else if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_OPEN){
		DBG_ERR("FATAL ERROR : module is NOT start yet, please call vendor_ai_op_start() first !!!\r\n");
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_NOT_START;
	}
	
	switch (aop) {
		case VENDOR_AI_OP_LIST:
		{
			VENDOR_AI_FC_PARM *p_parm = 0;
			VENDOR_AI_LL_HEAD *p_ll_head = 0;
			VENDOR_AI_OP_FC_CMDBUF p_cmd_buf = {0};

			if (p_in0==0) {
				DBG_ERR("invalid in1 addr (%08x)!\n", 
					(unsigned int)p_in0);
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
			if (p_in1==0) {
				DBG_ERR("invalid in2 addr (%08x)!\n", 
					(unsigned int)p_in1);
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
			if (p_out0==0) {
				DBG_ERR("invalid out1 addr (%08x)!\n", 
					(unsigned int)p_out0);
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}

			if (p_in0 != 0) {
				memcpy(&p_proc->input_img, p_in0, sizeof(VENDOR_AI_BUF));
			}
			if (p_in1 != 0) {
				memcpy(&p_proc->input2_img, p_in1, sizeof(VENDOR_AI_BUF));
			}
			if (p_out0 != 0) {
				memcpy(&p_proc->output_img, p_out0, sizeof(VENDOR_AI_BUF));
			}

			if ((p_proc->input_img.width == 0)
			|| (p_proc->input_img.height != 1)
			|| (p_proc->input_img.channel != 1)
			|| (p_proc->input_img.batch_num != 1)){
				DBG_ERR("invalid in1.dim width,height,channel,batch_num (%d, %d, %d, %d)!\n", 
					(int)p_proc->input_img.width, 
					(int)p_proc->input_img.height, 
					(int)p_proc->input_img.channel,
					(int)p_proc->input_img.batch_num);
				rv = HD_ERR_LIMIT;
				goto exit;
			}
			if ((p_proc->input2_img.width == 0)
			|| (p_proc->input2_img.height == 0)
			|| (p_proc->input2_img.channel != 1)
			|| (p_proc->input2_img.batch_num != 1)){
				DBG_ERR("invalid in2.dim width,height,channel,batch_num (%d, %d, %d, %d)!\n", 
					(int)p_proc->input2_img.width, 
					(int)p_proc->input2_img.height, 
					(int)p_proc->input2_img.channel,
					(int)p_proc->input2_img.batch_num);
				rv = HD_ERR_LIMIT;
				goto exit;
			}
			if ((p_proc->output_img.width == 0)
			|| (p_proc->output_img.height != 1)
			|| (p_proc->output_img.channel != 1)
			|| (p_proc->output_img.batch_num != 1)){
				DBG_ERR("invalid out1.dim width,height,channel,batch_num (%d, %d, %d, %d)!\n", 
					(int)p_proc->output_img.width, 
					(int)p_proc->output_img.height, 
					(int)p_proc->output_img.channel,
					(int)p_proc->output_img.batch_num);
				rv = HD_ERR_LIMIT;
				goto exit;
			}
			if (p_proc->input_img.width != p_proc->input2_img.width) {
				DBG_ERR("limit in1.width != in2.width (%d, %d)!\n", 
					(int)p_proc->input_img.width,
					(int)p_proc->input2_img.width);
				rv = HD_ERR_LIMIT;
				goto exit;
			}
			if (p_proc->input2_img.height != p_proc->output_img.width) {
				DBG_ERR("limit in1.height != out1.width (%d, %d)!\n", 
					(int)p_proc->input2_img.height,
					(int)p_proc->output_img.width);
				rv = HD_ERR_LIMIT;
				goto exit;
			}
			// need a mctrl
			p_mctrl = (NN_GEN_MODE_CTRL*)&(p_op->mctrl);
			p_mctrl->mode = AI_MODE_FC;
			p_mctrl->idea_cycle = 1;
			p_mctrl->eng = NN_GEN_ENG_NUE;
			p_mctrl->trig_src = NN_GEN_TRIG_LL_AI_DRV; //ll mode
			p_mctrl->tot_trig_eng_times = 1;
			p_mctrl->addr = (UINT32)&(p_op->ll_head);

			// need a ll_head
			p_ll_head = (VENDOR_AI_LL_HEAD *)p_mctrl->addr;
			p_ll_head->mode = AI_MODE_FC;
			p_ll_head->parm_addr = (UINT32)(p_op->data);   //Temporarily put the app parm, need to be modify
			p_ll_head->parm_size = sizeof(VENDOR_AI_FC_PARM); //Temporarily put the app parm, need to be modify
			p_ll_head->eng = AI_ENG_NUE;
			//p_ll_head->stripe_head_addr = 0;

			// need a ll_param
			p_parm = (VENDOR_AI_FC_PARM *)p_ll_head->parm_addr; //Temporarily put the app parm, need to be modify
			rv = _vendor_ai_fill_fc_parm(p_parm, p_proc); //Temporarily put the app parm, need to be modify
			if (rv != HD_OK) {
				goto exit;
			}
			
			//fill cmd_buf from user
			//memcpy(&p_cmd_buf,p_param,sizeof(VENDOR_AI_OP_FC_CMDBUF));
			
			p_cmd_buf.pa = p_mem_manager->user_model.pa;
			p_cmd_buf.va = p_mem_manager->user_model.va;
			p_cmd_buf.size = p_mem_manager->user_model.size;

			//DBGH(p_cmd_buf.pa);
			//DBGH(p_cmd_buf.va);
			//DBGH(p_cmd_buf.size);

			vendor_common_mem_cache_sync((VOID *)p_mem_manager->user_parm.va, p_mem_manager->user_parm.size, VENDOR_COMMON_MEM_DMA_TO_DEVICE);
			rv = vendor_ais_set_fc_ll_cmd(&p_cmd_buf, proc_id); 
			if (rv != HD_OK){
				DBG_ERR("proc_id(%lu) vendor_ais_set_fc_ll_cmd fail.r\n", proc_id);
				vendor_ais_unlock_net(proc_id);
				return rv;
			}

		    ///< cache clean - output to engine's input
		    vendor_common_mem_cache_sync((VOID *)p_cmd_buf.va, p_cmd_buf.size, VENDOR_COMMON_MEM_DMA_TO_DEVICE);
		}
		break;
		default:
        {
            DBG_ERR("Unknown AI OP!\r\n");
			vendor_ais_unlock_net(proc_id);
			return HD_ERR_LIMIT;
		}
        break;  
	}
exit:

	vendor_ais_unlock_net(proc_id);
	
	return rv;
}

HD_RESULT vendor_ai_op_proc(UINT32 proc_id, VENDOR_AI_OP aop, void* p_param, UINT32 in_cnt, VENDOR_AI_BUF* p_in_buf, UINT32 out_cnt, VENDOR_AI_BUF* p_out_buf)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_BUF* p_in0 = (in_cnt > 0) ? (p_in_buf + 0) : 0;
	VENDOR_AI_BUF* p_in1 = (in_cnt > 1) ? (p_in_buf + 1) : 0;
    VENDOR_AI_BUF* p_in2 = (in_cnt > 2) ? (p_in_buf + 2) : 0;
	VENDOR_AI_BUF* p_out0 = (out_cnt > 0) ? (p_out_buf + 0) : 0;
    VENDOR_AI_BUF* p_out1 = (out_cnt > 1) ? (p_out_buf + 1) : 0;
    VENDOR_AI_BUF* p_out2 = (out_cnt > 2) ? (p_out_buf + 2) : 0;
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager = &p_proc->mem_manager;
	VENDOR_AI_OP_WORK* p_op = (VENDOR_AI_OP_WORK*)p_mem_manager->user_parm.va;
	NN_GEN_MODE_CTRL *p_mctrl = 0;
	VENDOR_AI_APP_HEAD *p_head = 0;
	UINT32 process_id = 0;
	UINT32 engine_id = 0;
	UINT32 engine_op = 0;
	UINT32 job_id;

	vendor_ais_lock_net(proc_id);
	
	rv = _vendor_ai_validate(proc_id);
	if (rv != HD_OK) {
		vendor_ais_unlock_net(proc_id);
		return rv;
	}
	if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_UNINIT) {
		DBG_ERR("FATAL ERROR : module is NOT init & open & start yet, please call vendor_ai_net_init() & vendor_ai_op_open() & vendor_ai_op_start() first !!!\r\n");
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_UNINIT;
	} else if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_INIT){
		DBG_ERR("FATAL ERROR : module is NOT open & start yet, please call vendor_ai_op_open() & vendor_ai_op_start() first !!!\r\n");
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_NOT_OPEN;
	} else if (_vendor_ai_state(proc_id)[0] == VENDOR_AI_PROC_STATE_OPEN){
		DBG_ERR("FATAL ERROR : module is NOT start yet, please call vendor_ai_op_start() first !!!\r\n");
		vendor_ais_unlock_net(proc_id);
		return HD_ERR_NOT_START;
	}

	switch (aop) {
		case VENDOR_AI_OP_LIST:
		{
			VENDOR_AI_LL_HEAD *p_ll_head = 0;

			if(p_mem_manager->user_parm.pa == p_mem_manager->user_model.pa) {
				DBG_ERR("proc_id(%lu): Must set work_buf when using FC ll mode!!\r\n", proc_id);
				vendor_ais_unlock_net(proc_id);
				return HD_ERR_NOT_SUPPORT;
			}
			//modify ll_head
			p_mctrl = (NN_GEN_MODE_CTRL*)&(p_op->mctrl);
			if(p_mctrl->trig_src != NN_GEN_TRIG_LL_AI_DRV) {
				DBG_ERR("proc_id(%lu): Must call vendor_ai_op_add when using FC ll mode!!\r\n", proc_id);
				vendor_ais_unlock_net(proc_id);
				return HD_ERR_NOT_SUPPORT;
			}
			p_mctrl->addr = (UINT32)&(p_op->ll_head);
			p_ll_head = (VENDOR_AI_LL_HEAD *)p_mctrl->addr;	
			p_ll_head->parm_addr = p_mem_manager->user_model.pa; //convert to kernel_va in kernel space laterly  
			p_ll_head->parm_size = p_mem_manager->user_model.size;

			vendor_common_mem_cache_sync((VOID *)p_mem_manager->user_parm.va, p_mem_manager->user_parm.size, VENDOR_COMMON_MEM_DMA_TO_DEVICE);

			/*
			DBGD(p_mctrl->mode);
			DBGD(p_mctrl->idea_cycle);
			DBGD(p_mctrl->eng);
			DBGD(p_mctrl->trig_src);
			DBGD(p_mctrl->tot_trig_eng_times);
			DBGH(p_mctrl->addr);
			DBGH(p_ll_head->mode);
			DBGH(p_ll_head->parm_addr);
			DBGD(p_ll_head->parm_size);
			DBGD(p_ll_head->eng);
			*/
			//DBGH(p_mctrl->addr);
			//DBGH(p_ll_head->parm_addr);
			//DBGH(p_ll_head->parm_size);

			//SET JOB
			_vendor_ai_job_select_eng2(p_mctrl, &engine_id, &engine_op);
			vendor_ai_dla_set_job3(proc_id, process_id, engine_id, engine_op, 0, (void*)p_mctrl, (void*)0, 0);
            vendor_ai_dla_sum_job(proc_id, 0, 0);
            /*
				//need a job
				p_job->exec_cnt++;
				p_job->proc_id, 
				p_job->job_id, 
				p_job->p_eng->name
			*/
			//proc
			
			rv = vendor_ai_dla_lock_job(proc_id, 0); //pause
			if (rv != HD_OK){
				DBG_ERR("proc_id(%lu) op proc fail=%d in lock_job\r\n", proc_id, (int)rv);
				vendor_ais_unlock_net(proc_id);
				return rv;
			}
			rv = vendor_ai_dla_push_job(proc_id, process_id, 0); //push one
			if (rv != HD_OK){
				DBG_ERR("proc_id(%lu) op proc fail=%d in push_job\r\n", proc_id, (int)rv);
				vendor_ais_unlock_net(proc_id);
				return rv;
			}
			rv = vendor_ai_dla_unlock_job(proc_id, 0); //resume
			if (rv != HD_OK){
				DBG_ERR("proc_id(%lu) op proc fail=%d in unlock_job\r\n", proc_id, (int)rv);
				vendor_ais_unlock_net(proc_id);
				return rv;
			}
			job_id = 0xf0000004;
			rv = vendor_ai_dla_pull_job(proc_id, &job_id); // pull wait
			if (rv != HD_ERR_EOL){
				DBG_ERR("proc_id(%lu) op proc fail=%d in pull_job\r\n", proc_id, (int)rv);
				vendor_ais_unlock_net(proc_id);
				return rv;
			}
			vendor_ai_dla_clear_job3(proc_id, process_id);
            rv = HD_OK;
		}
		break;
		case VENDOR_AI_OP_FC:
		{
			DBG_ERR("VENDOR_AI_OP_FC is deprecated, please use VENDOR_AI_OP_LIST instead!!\r\n");
			vendor_ais_unlock_net(proc_id);
			return HD_ERR_NOT_SUPPORT;
#if 0
			VENDOR_AI_FC_PARM *p_parm = 0;

			if (p_in0==0) {
				DBG_ERR("invalid in1 addr (%08x)!\n", 
					(unsigned int)p_in0);
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
			if (p_in1==0) {
				DBG_ERR("invalid in2 addr (%08x)!\n", 
					(unsigned int)p_in1);
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
			if (p_out0==0) {
				DBG_ERR("invalid out1 addr (%08x)!\n", 
					(unsigned int)p_out0);
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}

			if (p_in0 != 0) {
				memcpy(&p_proc->input_img, p_in0, sizeof(VENDOR_AI_BUF));
			}
			if (p_in1 != 0) {
				memcpy(&p_proc->input2_img, p_in1, sizeof(VENDOR_AI_BUF));
			}
			if (p_out0 != 0) {
				memcpy(&p_proc->output_img, p_out0, sizeof(VENDOR_AI_BUF));
			}

			if ((p_proc->input_img.width == 0)
			|| (p_proc->input_img.height != 1)
			|| (p_proc->input_img.channel != 1)
			|| (p_proc->input_img.batch_num != 1)){
				DBG_ERR("invalid in1.dim width,height,channel,batch_num (%d, %d, %d, %d)!\n", 
					(int)p_proc->input_img.width, 
					(int)p_proc->input_img.height, 
					(int)p_proc->input_img.channel,
					(int)p_proc->input_img.batch_num);
				rv = HD_ERR_LIMIT;
				goto exit;
			}
			if ((p_proc->input2_img.width == 0)
			|| (p_proc->input2_img.height == 0)
			|| (p_proc->input2_img.channel != 1)
			|| (p_proc->input2_img.batch_num != 1)){
				DBG_ERR("invalid in2.dim width,height,channel,batch_num (%d, %d, %d, %d)!\n", 
					(int)p_proc->input2_img.width, 
					(int)p_proc->input2_img.height, 
					(int)p_proc->input2_img.channel,
					(int)p_proc->input2_img.batch_num);
				rv = HD_ERR_LIMIT;
				goto exit;
			}
			if ((p_proc->output_img.width == 0)
			|| (p_proc->output_img.height != 1)
			|| (p_proc->output_img.channel != 1)
			|| (p_proc->output_img.batch_num != 1)){
				DBG_ERR("invalid out1.dim width,height,channel,batch_num (%d, %d, %d, %d)!\n", 
					(int)p_proc->output_img.width, 
					(int)p_proc->output_img.height, 
					(int)p_proc->output_img.channel,
					(int)p_proc->output_img.batch_num);
				rv = HD_ERR_LIMIT;
				goto exit;
			}
			if (p_proc->input_img.width != p_proc->input2_img.width) {
				DBG_ERR("limit in1.width != in2.width (%d, %d)!\n", 
					(int)p_proc->input_img.width,
					(int)p_proc->input2_img.width);
				rv = HD_ERR_LIMIT;
				goto exit;
			}
			if (p_proc->input2_img.height != p_proc->output_img.width) {
				DBG_ERR("limit in1.height != out1.width (%d, %d)!\n", 
					(int)p_proc->input2_img.height,
					(int)p_proc->output_img.width);
				rv = HD_ERR_LIMIT;
				goto exit;
			}

			// need a mctrl
			p_mctrl = (NN_GEN_MODE_CTRL*)&(p_op->mctrl);
			p_mctrl->mode = AI_MODE_FC;
			p_mctrl->idea_cycle = 1;
			p_mctrl->eng = NN_GEN_ENG_NUE;
			p_mctrl->trig_src = NN_GEN_TRIG_USER_AI_DRV; //user op
			p_mctrl->tot_trig_eng_times = 1;
			p_mctrl->addr = (UINT32)&(p_op->head);

			// need a app_head
			p_head = (VENDOR_AI_APP_HEAD *)p_mctrl->addr;
			p_head->mode = AI_MODE_FC;
			p_head->parm_addr = (UINT32)(p_op->data);
			p_head->parm_size = sizeof(VENDOR_AI_FC_PARM);
			p_head->stripe_head_addr = 0;

				// need a app_param
				p_parm = (VENDOR_AI_FC_PARM *)p_head->parm_addr;
				rv = _vendor_ai_fill_fc_parm(p_parm, p_proc);
				if (rv != HD_OK) {
					goto exit;
				}

	            ///< cache clean - output to engine's input
	            vendor_common_mem_cache_sync((VOID *)p_mem_manager->user_parm.va, p_mem_manager->user_parm.size, VENDOR_COMMON_MEM_DMA_TO_DEVICE);

			//SET JOB
			_vendor_ai_job_select_eng2(p_mctrl, &engine_id, &engine_op);
			vendor_ai_dla_set_job2(proc_id, process_id, engine_id, engine_op, 0, (void*)p_mctrl, (void*)0, -1);
            vendor_ai_dla_sum_job(proc_id, 0, 0);
            /*
				//need a job
				p_job->exec_cnt++;
				p_job->proc_id, 
				p_job->job_id, 
				p_job->p_eng->name
			*/
			//proc
			rv = vendor_ai_dla_lock_job(proc_id, 0); //pause
			if (rv != HD_OK){
				DBG_ERR("proc_id(%lu) op proc fail=%d in lock_job\r\n", proc_id, (int)rv);
				vendor_ais_unlock_net(proc_id);
				return rv;
			}
			rv = vendor_ai_dla_push_job(proc_id, process_id, 0); //push one
			if (rv != HD_OK){
				DBG_ERR("proc_id(%lu) op proc fail=%d in push_job\r\n", proc_id, (int)rv);
				return rv;
			}
			rv = vendor_ai_dla_unlock_job(proc_id, 0); //resume
			if (rv != HD_OK){
				DBG_ERR("proc_id(%lu) op proc fail=%d in unlock_job\r\n", proc_id, (int)rv);
				vendor_ais_unlock_net(proc_id);
				return rv;
			}
			job_id = 0xf0000004;
			rv = vendor_ai_dla_pull_job(proc_id, &job_id); // pull wait
			if (rv != HD_ERR_EOL){
				DBG_ERR("proc_id(%lu) op proc fail=%d in pull_job\r\n", proc_id, (int)rv);
				vendor_ais_unlock_net(proc_id);
				return rv;
			}
            rv = HD_OK;
#endif
		}
		break;

        case VENDOR_AI_OP_PREPROC:
        {
            VENDOR_AI_OP_PREPROC_PARAM *p_user = (VENDOR_AI_OP_PREPROC_PARAM *) p_param;
            VENDOR_PREPROC_PARM *p_parm = 0;

            //check input, output, p_param valid(TODO)

            if (p_in0 != 0) {
				memcpy(&p_proc->input_img, p_in0, sizeof(VENDOR_AI_BUF));
			}
            if (p_in1 != 0) {
				memcpy(&p_proc->input2_img, p_in1, sizeof(VENDOR_AI_BUF));
			}
            if (p_in2 != 0) {
				memcpy(&p_proc->input3_img, p_in2, sizeof(VENDOR_AI_BUF));
			}
			if (p_out0 != 0) {
				memcpy(&p_proc->output_img, p_out0, sizeof(VENDOR_AI_BUF));
			}
            if (p_out1 != 0) {
				memcpy(&p_proc->output2_img, p_out1, sizeof(VENDOR_AI_BUF));
			}
            if (p_out2 != 0) {
				memcpy(&p_proc->output3_img, p_out2, sizeof(VENDOR_AI_BUF));
			}

            // need a mctrl
            p_mctrl = (NN_GEN_MODE_CTRL*)&(p_op->mctrl);
            memset(p_mctrl, 0, sizeof(NN_GEN_MODE_CTRL));
            p_mctrl->mode = AI_MODE_PREPROC;
            
            if(p_proc->input_img.fmt == HD_VIDEO_PXLFMT_UV && in_cnt == 2) {
                //YUV420, use Y size
                p_mctrl->idea_cycle = p_proc->input2_img.width * p_proc->input2_img.height;
            } else {
                p_mctrl->idea_cycle = p_proc->input_img.width * p_proc->input_img.height;   
            }
            
            p_mctrl->eng = NN_GEN_ENG_NUE2;
            p_mctrl->trig_src = NN_GEN_TRIG_USER_AI_DRV; //user op
            p_mctrl->tot_trig_eng_times = 1;
            p_mctrl->addr = (UINT32)&(p_op->head);
            //p_mctrl->size   
            //p_mctrl->iomem 
            
            // need a app_head
            p_head = (VENDOR_AI_APP_HEAD *)p_mctrl->addr;
            memset(p_head, 0, sizeof(VENDOR_AI_APP_HEAD));
            p_head->mode = AI_MODE_PREPROC;
            p_head->parm_addr = (UINT32)(p_op->data);
            p_head->parm_size = sizeof(VENDOR_PREPROC_PARM);
            p_head->stripe_head_addr = 0;

            // need a app_param
            UINT32 func_idx = 0;
            p_parm = (VENDOR_PREPROC_PARM *)p_head->parm_addr;
            memset(p_parm, 0, sizeof(VENDOR_PREPROC_PARM));
            p_parm->in_type = AI_IO_UINT8;
            p_parm->out_type = AI_IO_UINT8;

            
            if(p_proc->input_img.fmt == HD_VIDEO_PXLFMT_UV && in_cnt == 2) {
                //YUV420, use Y size
                p_parm->in_size.width = p_proc->input2_img.width;
			    p_parm->in_size.height = p_proc->input2_img.height;
            } else {
                p_parm->in_size.width = p_proc->input_img.width;
			    p_parm->in_size.height = p_proc->input_img.height;
            }
            p_parm->in_size.channel = in_cnt;
                

            //set io format
            if(in_cnt == 3) { 
                p_parm->src_fmt = AI_PREPROC_SRC_RGB;
            } else if(in_cnt == 2) { 
#if defined(_BSP_NA51068_) || defined(_BSP_NA51090_)
                p_parm->src_fmt = AI_PREPROC_SRC_YUV420_NV21;
#else
				p_parm->src_fmt = AI_PREPROC_SRC_YUV420;
#endif
            } else if(p_proc->input_img.fmt == HD_VIDEO_PXLFMT_Y8) {
                p_parm->src_fmt = AI_PREPROC_SRC_YONLY;
            } else if(p_proc->input_img.fmt == HD_VIDEO_PXLFMT_UV) {
                p_parm->src_fmt = AI_PREPROC_SRC_UVPACK;
            } else {
                DBG_ERR("not support input format: 0x%08x\r\n", p_proc->input_img.fmt);
				vendor_ais_unlock_net(proc_id);
                return HD_ERR_NG;
            }
            if(out_cnt == 3) {
                p_parm->rst_fmt = AI_PREPROC_SRC_RGB;
            } else if(out_cnt == 2) { 
#if defined(_BSP_NA51068_) || defined(_BSP_NA51090_)
                p_parm->src_fmt = AI_PREPROC_SRC_YUV420_NV21;
#else
				p_parm->src_fmt = AI_PREPROC_SRC_YUV420;
#endif
            } else if(p_proc->output_img.fmt == HD_VIDEO_PXLFMT_Y8) {
                p_parm->rst_fmt = AI_PREPROC_SRC_YONLY;
            } else if(p_proc->output_img.fmt == HD_VIDEO_PXLFMT_UV) {
                p_parm->rst_fmt = AI_PREPROC_SRC_UVPACK;
            } else {
                DBG_ERR("not support output format: 0x%08x\r\n", p_proc->output_img.fmt);
				vendor_ais_unlock_net(proc_id);
                return HD_ERR_NG;
            }
			
#if defined(_BSP_NA51068_) || defined(_BSP_NA51090_)
			if(p_parm->src_fmt == AI_PREPROC_SRC_YUV420_NV21 && p_parm->rst_fmt == AI_PREPROC_SRC_YUV420_NV21) {
				DBG_ERR("YUV2YUV is not support!\n");
				vendor_ais_unlock_net(proc_id);
				return HD_ERR_NG;
			}
#else
			if(p_parm->src_fmt == AI_PREPROC_SRC_YUV420 && p_parm->rst_fmt == AI_PREPROC_SRC_YUV420) {
				DBG_ERR("YUV2YUV is not support!\n");
				vendor_ais_unlock_net(proc_id);
				return HD_ERR_NG;
			}
#endif


#if defined(_BSP_NA51068_) || defined(_BSP_NA51090_)       
            // check format & YUV2RGB
	        if(p_parm->src_fmt != p_parm->rst_fmt) {
                if(p_parm->src_fmt == AI_PREPROC_SRC_YUV420_NV21 && p_parm->rst_fmt == AI_PREPROC_SRC_RGB) {
                    p_parm->func_list[func_idx] = KDRV_AI_PREPROC_YUV2RGB_EN;
			        func_idx++;
                } else {
                    DBG_ERR("Only support YUV2RGB!\n");
					vendor_ais_unlock_net(proc_id);
			        return HD_ERR_NG;
                }
	        }
#else
            // check format & YUV2RGB
	        if(p_parm->src_fmt != p_parm->rst_fmt) {
                if(p_parm->src_fmt == AI_PREPROC_SRC_YUV420 && p_parm->rst_fmt == AI_PREPROC_SRC_RGB) {
                    p_parm->func_list[func_idx] = KDRV_AI_PREPROC_YUV2RGB_EN;
			        func_idx++;
                } else {
                    DBG_ERR("Only support YUV2RGB!\n");
					vendor_ais_unlock_net(proc_id);
			        return HD_ERR_NG;
                }
	        }
#endif

			// check meansub plane mode limit
			if(p_user->p_out_sub.pa && p_parm->src_fmt == AI_PREPROC_SRC_RGB) {
				DBG_ERR("meansub plane mode not support when src format = RGB!\n");
			}

			// check hardware address alignment limit
			if(p_parm->src_fmt == AI_PREPROC_SRC_YUV420 || p_parm->src_fmt == AI_PREPROC_SRC_YUV420_NV21) {
				if (p_proc->input_img.pa % 2 != 0) {
					DBG_ERR("Input format is YUV420,input_img pa = 0x%lx is not 2-byte aligned!\n",p_proc->input_img.pa);
					vendor_ais_unlock_net(proc_id);
			        return HD_ERR_NG;
				}
				if (p_proc->input2_img.pa % 2 != 0) {
					DBG_ERR("Input format is YUV420,input2_img pa = 0x%lx is not 2-byte aligned!\n",p_proc->input2_img.pa);
					vendor_ais_unlock_net(proc_id);
			        return HD_ERR_NG;
				}
			}

			if(p_parm->src_fmt == AI_PREPROC_SRC_UVPACK) {
				if (p_proc->input_img.pa % 2 != 0) {
					DBG_ERR("Input format is UV PACK,input_img pa = 0x%lx is not 2-byte aligned!\n",p_proc->input_img.pa);
					vendor_ais_unlock_net(proc_id);
			        return HD_ERR_NG;
				}
				if (p_proc->output_img.pa % 2 != 0) {
					DBG_ERR("Output format is UV PACK,output_img pa = 0x%lx is not 2-byte aligned!\n",p_proc->output_img.pa);
					vendor_ais_unlock_net(proc_id);
			        return HD_ERR_NG;
				}
			}
			
            // set io info (input addr/output addr/in ofs/out ofs)
            if ((rv =_vendor_ai_set_preproc_addr_ofs(&p_proc->input_img, p_parm, 0)) != HD_OK) {
				vendor_ais_unlock_net(proc_id);
                return rv;
            }
            if(in_cnt > 1) 
                if ((rv =_vendor_ai_set_preproc_addr_ofs(&p_proc->input2_img, p_parm, 0)) != HD_OK) {
					vendor_ais_unlock_net(proc_id);
					return rv;
                }
            if(in_cnt > 2) 
                if ((rv =_vendor_ai_set_preproc_addr_ofs(&p_proc->input3_img, p_parm, 0)) != HD_OK) {
					vendor_ais_unlock_net(proc_id);
                    return rv;
                }
            if ((rv =_vendor_ai_set_preproc_addr_ofs(&p_proc->output_img, p_parm, 1)) != HD_OK) {
				vendor_ais_unlock_net(proc_id);
                return rv;
            }
            if(out_cnt > 1) 
                if ((rv =_vendor_ai_set_preproc_addr_ofs(&p_proc->output2_img, p_parm, 1)) != HD_OK) {
					vendor_ais_unlock_net(proc_id);
                    return rv;
                }
            if(out_cnt > 2) 
                if ((rv =_vendor_ai_set_preproc_addr_ofs(&p_proc->output3_img, p_parm, 1)) != HD_OK) {
					vendor_ais_unlock_net(proc_id);
                    return rv;
                }

			if(p_parm->src_fmt == AI_PREPROC_SRC_UVPACK) {
	    		p_parm->in_addr[0] = p_parm->in_addr[1];
				p_parm->in_addr[1] = 0;
        		p_parm->in_ofs[0].line_ofs = p_parm->in_ofs[1].line_ofs;
				p_parm->in_ofs[1].line_ofs = 0;
    		}
			if(p_parm->rst_fmt == AI_PREPROC_SRC_UVPACK) {
	    		p_parm->out_addr[0] = p_parm->out_addr[1];
				p_parm->out_addr[1] = 0;
        		p_parm->out_ofs[0].line_ofs = p_parm->out_ofs[1].line_ofs;
				p_parm->out_ofs[1].line_ofs = 0;
    		}			
			
            if (p_user->p_out_sub.pa) {
			    // for submean planar mode, in addr 2 must be sub planar
			    p_parm->in_addr[2] = p_user->p_out_sub.pa;
			    p_parm->in_ofs[2].line_ofs = p_user->p_out_sub.line_ofs;
		    }
       
            //scale
            if (p_user->scale_dim.w == 0 && p_user->scale_dim.h == 0) {
                p_parm->scale_ker.scl_out_size.width  = p_proc->input_img.width;
	        	p_parm->scale_ker.scl_out_size.height = p_proc->input_img.height;
	    	    p_parm->scale_ker.fact_update_en = 0;
	        } else {
#if defined(_BSP_NA51055_)
			UINT32 chip_old = 0 ; 
			rv = _vendor_ai_query_chip_old( &chip_old) ; 
			if ( chip_old == 1) {
				if(p_user->scale_dim.w > p_proc->input_img.width || p_user->scale_dim.h > p_proc->input_img.height){ 
					DBG_ERR("Only support scale down.\n");
					vendor_ais_unlock_net(proc_id);
					return HD_ERR_LIMIT;
				}
			}
	        	
#endif				
		        p_parm->scale_ker.scl_out_size.width = p_user->scale_dim.w;
                p_parm->scale_ker.scl_out_size.height = p_user->scale_dim.h;
		        p_parm->scale_ker.fact_update_en = 0;
	        }

            //mean substration
            if(p_user->p_out_sub.pa || p_user->out_sub_color[0] || p_user->out_sub_color[1] || p_user->out_sub_color[2]){
                p_parm->func_list[func_idx] = KDRV_AI_PREPROC_SUB_EN;
		        func_idx++;
                if(p_user->p_out_sub.pa) {
                    //plane mode
                    p_parm->sub_ker.sub_mode = AI_SUB_PLANAR;
			        p_parm->sub_ker.sub_in_w = p_user->p_out_sub.width;
			        p_parm->sub_ker.sub_in_h = p_user->p_out_sub.height;
                } else {
                    // dc mode
                    p_parm->sub_ker.sub_mode = AI_SUB_DC;
			        p_parm->sub_ker.sub_dc_coef[0] = p_user->out_sub_color[0];
			        p_parm->sub_ker.sub_dc_coef[1] = p_user->out_sub_color[1];
			        p_parm->sub_ker.sub_dc_coef[2] = p_user->out_sub_color[2];
                }
                p_parm->sub_ker.dup_rate = 0;
		        p_parm->sub_ker.sub_shf = 0;
            }

            //crop & pad
            //temporally not support in AI1
#if 0
                p_parm->pad_ker.crop_x     = p_user->out_crop_win.x;
		        p_parm->pad_ker.crop_y     = p_user->out_crop_win.y;
		        p_parm->pad_ker.crop_w     = p_user->out_crop_win.w;
		        p_parm->pad_ker.crop_h     = p_user->out_crop_win.h;
		        p_parm->pad_ker.pad_out_x  = p_user->out_crop_pt.x;
		        p_parm->pad_ker.pad_out_y  = p_user->out_crop_pt.y;
		        p_parm->pad_ker.pad_out_w  = p_user->pad_dim.w;
		        p_parm->pad_ker.pad_out_h  = p_user->pad_dim.h;
		        p_parm->pad_ker.pad_val[0] = p_user->pad_color[0];
		        p_parm->pad_ker.pad_val[1] = p_user->pad_color[1];
		        p_parm->pad_ker.pad_val[2] = p_user->pad_color[2];
#endif            
            /*
            printf("func_list[0] : %d\n", p_parm->func_list[0]);
            printf("func_list[1] : %d\n", p_parm->func_list[1]);
            printf("func_list[2] : %d\n", p_parm->func_list[2]);
            printf("src_fmt : %d\n", p_parm->src_fmt);
            printf("rst_fmt : %d\n", p_parm->rst_fmt);
            printf("in_type : %d\n", p_parm->in_type);
            printf("out_type : %d\n", p_parm->out_type);
            printf("in_size.width : %d\n", p_parm->in_size.width);
            printf("in_size.height : %d\n", p_parm->in_size.height);
            printf("in_ofs[0].line_ofs : %lu\n", p_parm->in_ofs[0].line_ofs);
            printf("in_ofs[1].line_ofs : %lu\n", p_parm->in_ofs[1].line_ofs);
            printf("in_ofs[2].line_ofs : %lu\n", p_parm->in_ofs[2].line_ofs);
            printf("out_ofs[0].line_ofs : %lu\n", p_parm->out_ofs[0].line_ofs);
            printf("out_ofs[1].line_ofs : %lu\n", p_parm->out_ofs[1].line_ofs);
            printf("out_ofs[2].line_ofs : %lu\n", p_parm->out_ofs[2].line_ofs);
            
            printf("intput : pa=0x%lx\n", p_parm->in_addr[0]);
	        printf("intput2: pa=0x%lx\n", p_parm->in_addr[1]);
	        printf("intput3: pa=0x%lx\n", p_parm->in_addr[2]);
	        printf("output : pa=0x%lx\n", p_parm->out_addr[0]);
            printf("output2: pa=0x%lx\n", p_parm->out_addr[1]);
            printf("output3: pa=0x%lx\n", p_parm->out_addr[2]);
            */

            ///< cache clean - output to engine's input
			vendor_common_mem_cache_sync((VOID *)p_mem_manager->user_parm.va, p_mem_manager->user_parm.size, VENDOR_COMMON_MEM_DMA_TO_DEVICE);
                      
            //SET JOB
			_vendor_ai_job_select_eng2(p_mctrl, &engine_id, &engine_op);
			vendor_ai_dla_set_job2(proc_id, process_id, engine_id, engine_op, 0, (void*)p_mctrl, (void*)0, -1);
			
			vendor_ai_dla_sum_job(proc_id, 0, 0);
            /*
	        //need a job
            p_job->exec_cnt++;
	        p_job->proc_id, 
	        p_job->job_id, 
	        p_job->p_eng->name
	        */
	        //proc
	        rv = vendor_ai_dla_lock_job(proc_id, 0); //pause
	        if (rv != HD_OK){
	        	DBG_ERR("proc_id(%lu) op proc fail=%d in lock_job\r\n", proc_id, (int)rv);
				vendor_ais_unlock_net(proc_id);
	        	return rv;
	        }
	        rv = vendor_ai_dla_push_job(proc_id, process_id, 0); //push one
	        if (rv != HD_OK){
	        	DBG_ERR("proc_id(%lu) op proc fail=%d in push_job\r\n", proc_id, (int)rv);
				vendor_ais_unlock_net(proc_id);
	        	return rv;
	        }
	        rv = vendor_ai_dla_unlock_job(proc_id, 0); //resume
	        if (rv != HD_OK){
	        	DBG_ERR("proc_id(%lu) op proc fail=%d in unlock_job\r\n", proc_id, (int)rv);
				vendor_ais_unlock_net(proc_id);
	    	    return rv;
	        }
	        job_id = 0xf0000004;
	        rv = vendor_ai_dla_pull_job(proc_id, &job_id); // pull wait
	        if (rv != HD_ERR_EOL){
	        	DBG_ERR("proc_id(%lu) op proc fail=%d in pull_job\r\n", proc_id, (int)rv);
				vendor_ais_unlock_net(proc_id);
		    	return rv;
	        }
            rv = HD_OK;
        }
        break;
		default:
        {
            DBG_ERR("Unknown AI OP!\r\n");
			vendor_ais_unlock_net(proc_id);
			return HD_ERR_LIMIT;
		}
        break;  
	}
	
	vendor_ais_unlock_net(proc_id);
	
	return rv;
}

