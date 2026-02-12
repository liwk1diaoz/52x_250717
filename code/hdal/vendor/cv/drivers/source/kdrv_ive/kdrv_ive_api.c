/**
	@file	   kdrv_ive.c
	@ingroup	Predefined_group_name

	@brief	  ive device abstraction layer

	Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "ive_platform.h"
#include "kdrv_ive.h"

#define CEIL(a, n) ((((a)+((1<<(n))-1))>>(n))<<(n))


static INT32 g_kdrv_is_nonblocking = 0;
static IVE_OPENOBJ g_ive_open_cfg;
static UINT32 g_kdrv_ive_init_flg = 0;
static KDRV_IVE_HANDLE g_kdrv_ive_handle_tab[KDRV_IVE_HANDLE_MAX_NUM];
static KDRV_IVE_HANDLE_NONBLOCK g_kdrv_ive_handle_tab_nonblock[KDRV_IVE_HANDLE_MAX_NUM];
KDRV_IVE_PRAM g_kdrv_ive_info[KDRV_IVE_HANDLE_MAX_NUM];

static ER kdrv_ive_set_mode(UINT8 id)
{
	ER rt;
	IVE_PARAM ive = {0};

	if(g_kdrv_ive_info[id].ll_dma_info.addr)
    {
		ive.LLDmaIn.uiLLAddr = g_kdrv_ive_info[id].ll_dma_info.addr;
        rt = ive_ll_setmode(ive.LLDmaIn.uiLLAddr);
	} else {
		ive.InSize.uiWidth = g_kdrv_ive_info[id].in_img_info.width;
		ive.InSize.uiHeight = g_kdrv_ive_info[id].in_img_info.height;

		ive.DmaIn.uiInSaddr = g_kdrv_ive_info[id].in_dma_info.addr;
		ive.DmaIn.uiInLofst = g_kdrv_ive_info[id].in_img_info.lineofst;

		ive.DmaOut.uiOutSaddr = g_kdrv_ive_info[id].out_dma_info.addr;


		//resource conflict check

		if ((g_kdrv_ive_info[id].non_max_sup.enable || g_kdrv_ive_info[id].thres_lut.enable || g_kdrv_ive_info[id].morph_filt.enable) &&
			g_kdrv_ive_info[id].integral_img.enable) {
			DBG_ERR("Any function of Non maximal supression, threshold LUT and morphological filter cannot run with integral image!\r\n");
			return E_PAR;
		}

		if (!g_kdrv_ive_info[id].thres_lut.enable && g_kdrv_ive_info[id].morph_filt.enable) {
			DBG_ERR("Threshold LUT must be enabled when morphological filter is enabled!\r\n");

		}


		if (g_kdrv_ive_info[id].morph_filt.enable && (g_kdrv_ive_info[id].gen_filt.enable ||  g_kdrv_ive_info[id].medn_filt.enable ||
			g_kdrv_ive_info[id].edge_filt.enable || g_kdrv_ive_info[id].non_max_sup.enable || g_kdrv_ive_info[id].thres_lut.enable)) {
			if ((MORPH_IN_SEL)g_kdrv_ive_info[id].morph_filt.in_sel != TH_LUT_IN) {
				DBG_ERR("Morphological filter input path is wrong!\r\n");
				return E_PAR;
			}
		}

		if (g_kdrv_ive_info[id].gen_filt.enable) {
			ive.FuncEn.bGenFiltEn = TRUE;
			ive.GenF.puiGenCoeff = g_kdrv_ive_info[id].gen_filt.filt_coeff;
			DBG_IND("Set general filter done\r\n");
		}
		if (g_kdrv_ive_info[id].medn_filt.enable) {
			ive.FuncEn.bMednFiltEn = TRUE;
			ive.uiMednMode = g_kdrv_ive_info[id].medn_filt.mode;
			DBG_IND("Set median filter done\r\n");
		}
		//DBG_ERR("ive.uiMednMod = %d \r\n", (UINT32)ive.uiMednMode);


		if (g_kdrv_ive_info[id].edge_filt.enable) {
			ive.FuncEn.bEdgeFiltEn = TRUE;
			ive.EdgeF.uiEdgeMode = (EDGE_MODE)g_kdrv_ive_info[id].edge_filt.mode;
			if (g_kdrv_ive_info[id].edge_filt.mode == KDRV_IVE_NO_DIR) {
				ive.EdgeF.pEdgeCoeff1 = (INT32 *)g_kdrv_ive_info[id].edge_filt.edge_coeff1;
			} else if (g_kdrv_ive_info[id].edge_filt.mode == KDRV_IVE_BI_DIR) {
				ive.EdgeF.pEdgeCoeff1 = (INT32 *)g_kdrv_ive_info[id].edge_filt.edge_coeff1;
				ive.EdgeF.pEdgeCoeff2 = (INT32 *)g_kdrv_ive_info[id].edge_filt.edge_coeff2;
			} else {
				DBG_ERR("unknown edge mode!\r\n");
				return E_PAR;
			}
			ive.EdgeF.uiAngSlpFact = g_kdrv_ive_info[id].edge_filt.AngSlpFact; //0x6a;  reserved function, fixed to default value 0x6a
			ive.EdgeF.uiEdgeShiftBit = g_kdrv_ive_info[id].edge_filt.edge_shift_bit;
			DBG_IND("Set edge filter done\r\n");
		}

		if (g_kdrv_ive_info[id].non_max_sup.enable || g_kdrv_ive_info[id].thres_lut.enable || g_kdrv_ive_info[id].morph_filt.enable) {
			if (g_kdrv_ive_info[id].non_max_sup.enable) {
				ive.FuncEn.bNonMaxEn = TRUE;
				ive.uiEdgeMagTh = g_kdrv_ive_info[id].non_max_sup.mag_thres;
				DBG_IND("Set non-maximal suppression done\r\n");
			}
			if (g_kdrv_ive_info[id].thres_lut.enable) {
				ive.FuncEn.bThresLutEn = TRUE;
				ive.Thres.puiThresLut = g_kdrv_ive_info[id].thres_lut.thres_lut;
				DBG_IND("Set threshold LUT done\r\n");
			}
			if (g_kdrv_ive_info[id].morph_filt.enable) {
				ive.FuncEn.bMorphFiltEn = TRUE;
				ive.uiMorphInSel = (MORPH_IN_SEL)g_kdrv_ive_info[id].morph_filt.in_sel;
				ive.MorphF.uiMorphOp = (MORPH_OP)g_kdrv_ive_info[id].morph_filt.operation;
				ive.MorphF.pbMorphNeighEn = g_kdrv_ive_info[id].morph_filt.neighbor;
				DBG_IND("Set morphological filter done\r\n");
			}
		} else if (g_kdrv_ive_info[id].integral_img.enable) {
			ive.FuncEn.bIntegralEn = TRUE;
			DBG_IND("Set integral image done\r\n");
		}

		if(nvt_get_chip_id() == CHIP_NA51055) {
		} else {
			ive.Irv.uiMednInvalTh = g_kdrv_ive_info[id].medn_filt.medn_inval_th;
			ive.FuncEn.bIrvEn = g_kdrv_ive_info[id].irv.en;
			ive.Irv.uiHistMode = g_kdrv_ive_info[id].irv.hist_mode_sel;
			ive.Irv.uiInvalidVal = g_kdrv_ive_info[id].irv.invalid_val;
			ive.Irv.uiThrS = g_kdrv_ive_info[id].irv.thr_s;
			ive.Irv.uiThrH = g_kdrv_ive_info[id].irv.thr_h;
		}

		ive.flowCT.dma_do_not_sync = g_kdrv_ive_info[id].flow_ct.dma_do_not_sync;

		ive.uiOutDataSel = g_kdrv_ive_info[id].outsel_info.OutDataSel;
		switch(g_kdrv_ive_info[id].outsel_info.OutDataSel)
		{
			case 0:
				ive.DmaOut.uiOutLofst = (g_kdrv_ive_info[id].in_img_info.width*1+7)>>3;
				break;
			case 1:
				ive.DmaOut.uiOutLofst = (g_kdrv_ive_info[id].in_img_info.width*4+7)>>3;
				break;
			case 2:
			case 3:
				ive.DmaOut.uiOutLofst = (g_kdrv_ive_info[id].in_img_info.width*8+7)>>3;
				break;
			case 4:
				ive.DmaOut.uiOutLofst = (g_kdrv_ive_info[id].in_img_info.width*12+7)>>3;
				break;
			case 5:
				ive.DmaOut.uiOutLofst = (g_kdrv_ive_info[id].in_img_info.width*16+7)>>3;
				break;
			case 6:
				ive.DmaOut.uiOutLofst = (g_kdrv_ive_info[id].in_img_info.width*32+7)>>3;
				break;
			default:
				break;
		}
		//ive.DmaOut.uiOutLofst = ((ive.DmaOut.uiOutLofst+3)>>2)<<2;
		ive.DmaOut.uiOutLofst =  g_kdrv_ive_info[id].outsel_info.Outlofs;

		rt = ive_setMode(&ive);
	}
	return rt;
}

static void kdrv_ive_lock(KDRV_IVE_HANDLE* p_handle, BOOL flag)
{
	if (flag == TRUE) {
		FLGPTN flag_ptn;
		wai_flg(&flag_ptn, p_handle->flag_id, p_handle->lock_bit, TWF_CLR);
	} else {
		set_flg( p_handle->flag_id, p_handle->lock_bit);
	}
}

static void kdrv_ive_handle_lock(void)
{
	FLGPTN wait_flg;
	wai_flg(&wait_flg, kdrv_ive_get_flag_id(FLG_ID_KDRV_IVE), KDRV_IVE_HDL_UNLOCK, (TWF_ORW | TWF_CLR));
}

static void kdrv_ive_handle_unlock(void)
{
	set_flg(kdrv_ive_get_flag_id(FLG_ID_KDRV_IVE), KDRV_IVE_HDL_UNLOCK);
}

static void kdrv_ive_handle_alloc_all(UINT32 *eng_init_flag)
{
	UINT32 i;

	kdrv_ive_handle_lock();
	*eng_init_flag = FALSE;
	for (i = 0; i < KDRV_IVE_HANDLE_MAX_NUM; i++) {
		if (!(g_kdrv_ive_init_flg & (1 << i))) {

			if (g_kdrv_ive_init_flg == 0)
			{
				*eng_init_flag = TRUE;
			}

			g_kdrv_ive_init_flg |= (1 << i);

			memset(&g_kdrv_ive_handle_tab[i], 0, sizeof(KDRV_IVE_HANDLE));
			memset(&g_kdrv_ive_handle_tab_nonblock[i], 0, sizeof(KDRV_IVE_HANDLE_NONBLOCK));
			g_kdrv_ive_handle_tab[i].entry_id = i;
			g_kdrv_ive_handle_tab[i].flag_id = kdrv_ive_get_flag_id(FLG_ID_KDRV_IVE);
			g_kdrv_ive_handle_tab[i].lock_bit = (1 << i);
			g_kdrv_ive_handle_tab[i].sts |= KDRV_IVE_HANDLE_LOCK;
			g_kdrv_ive_handle_tab[i].sem_id = kdrv_ive_get_sem_id(SEMID_KDRV_IVE);
		}
	}
	kdrv_ive_handle_unlock();

	return;
}

static UINT32 kdrv_ive_handle_free(KDRV_IVE_HANDLE* p_handle)
{
	UINT32 rt = FALSE;
	kdrv_ive_handle_lock();
	p_handle->sts = 0;
	g_kdrv_ive_init_flg &= ~(1 << p_handle->entry_id);
	if (g_kdrv_ive_init_flg == 0) {
		rt = TRUE;
	}
	kdrv_ive_handle_unlock();

	return rt;
}

static KDRV_IVE_HANDLE* kdrv_ive_entry_id_conv2_handle(UINT32 entry_id)
{
	return  &g_kdrv_ive_handle_tab[entry_id];
}

static KDRV_IVE_HANDLE_NONBLOCK* kdrv_ive_entry_id_conv2_handle_nonblock(UINT32 entry_id)
{
	return  &g_kdrv_ive_handle_tab_nonblock[entry_id];
}

static void kdrv_ive_clear_timeout_flag(KDRV_IVE_HANDLE* p_handle)
{
	clr_flg(p_handle->flag_id, KDRV_IVE_TIMEOUT);
}

INT32 kdrv_ive_open(UINT32 chip, UINT32 engine)
{
	UINT32 clk_freq_mhz;
	INT32 i;
	IVE_OPENOBJ ive_drv_obj;
	UINT32 eng_init_flag;

	DBG_IND("Count 0, open ive HW\r\n");

	ive_create_resource(NULL, 0);
	clk_freq_mhz = ive_get_clock_rate();
	if (clk_freq_mhz == 0) {
		ive_drv_obj.IVE_CLOCKSEL = g_ive_open_cfg.IVE_CLOCKSEL;
	} else {
		ive_drv_obj.IVE_CLOCKSEL = clk_freq_mhz;	
	}
	ive_drv_obj.FP_IVEISR_CB = ive_platform_isr_cb;
	if (ive_open(&ive_drv_obj) != E_OK) {
		DBG_WRN("KDRV IVE: ive_open failed\r\n");
		return -1;
	}
	kdrv_ive_handle_alloc_all(&eng_init_flag);
	for (i = 0; i < KDRV_IVE_HANDLE_MAX_NUM; i++) {
		kdrv_ive_lock(&g_kdrv_ive_handle_tab[i], TRUE);
		memset((void *)&g_kdrv_ive_info[i], 0, sizeof(KDRV_IVE_PRAM));
		kdrv_ive_lock(&g_kdrv_ive_handle_tab[i], FALSE);
	}
	

	return 0;
}
EXPORT_SYMBOL(kdrv_ive_open);

INT32 kdrv_ive_close(UINT32 chip, UINT32 engine)
{
	ER rt = E_OK;

	INT32 i;
	DBG_IND("Count 0, close ive HW\r\n");
	rt = ive_close();
	for (i = 0; i < KDRV_IVE_HANDLE_MAX_NUM; i++) {
		kdrv_ive_lock(&g_kdrv_ive_handle_tab[i], TRUE);
		kdrv_ive_handle_free(&g_kdrv_ive_handle_tab[i]);
		kdrv_ive_lock(&g_kdrv_ive_handle_tab[i], FALSE);
	}
	ive_release_resource(NULL);
	

	return (INT32) rt;
}
EXPORT_SYMBOL(kdrv_ive_close);

static void kdrv_ive_sem(KDRV_IVE_HANDLE* p_handle, BOOL flag)
{
	if (flag == TRUE) {
		SEM_WAIT(*p_handle->sem_id);	// wait semaphore
	} else {
		SEM_SIGNAL(*p_handle->sem_id);	// wait semaphore
	}
}

static INT32 kdrv_ive_trigger_internal(UINT32 id, KDRV_IVE_TRIGGER_PARAM *p_ive_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data, 
						ive_hook_t ive_hook, KDRV_IVE_TRIGGER_HOOK_PARAM tri_hook)
{
	ER rt = E_ID;
	KDRV_IVE_HANDLE *p_handle,*p_handle_lock;
	INT32 channel = KDRV_DEV_ID_CHANNEL(id);
	INT32 channel_lock = KDRV_DEV_ID_CHANNEL(0);
	FLGPTN  flag_ptn = 0;
#if IVE_MEASURE_TIME
	UINT64 start_cnt, end_cnt;
#endif
	
	if (channel >= KDRV_IVE_HANDLE_MAX_NUM || channel < 0){
		DBG_ERR("ive kdrv: invalid id: %d\n", channel);
		return rt;
	}

	p_handle = kdrv_ive_entry_id_conv2_handle(channel);
	p_handle_lock = kdrv_ive_entry_id_conv2_handle(channel_lock);

	kdrv_ive_sem(p_handle_lock, TRUE);
	
	DBG_IND("id 0x%x\r\n", id);

#if IVE_MEASURE_TIME
	start_cnt = hwclock_get_longcounter();
#endif
	//set ive kdrv parameters to ive driver
	if (kdrv_ive_set_mode((UINT8)(channel)) != E_OK) {
		goto IVE_END;
	}
#if IVE_MEASURE_TIME
	end_cnt = hwclock_get_longcounter();
	DBG_DUMP("set_mode time: %llu us\r\n", end_cnt - start_cnt);
#endif

	ive_setTriHdl((VOID *)p_handle);

	DBG_IND("1\r\n");

    //clear flag
	ive_platform_kdrv_ive_frm_end(p_handle, FALSE);
	kdrv_ive_clear_timeout_flag(p_handle);
	if(p_ive_param->is_nonblock) {
		ive_clearFrameEndFlag_LL();
        g_kdrv_is_nonblocking = 1;
		rt = ive_ll_start();
		if (rt != E_OK) {
			goto IVE_END;
		}
		if (ive_hook) {
			ive_hook(tri_hook.hook_mode, tri_hook.hook_value);
			if (tri_hook.hook_is_direct_return == 1) {
				ive_ll_pause();
				goto IVE_END;
			}
		}
		DBG_IND("ive start\r\n");
		ive_waitFrameEnd_LL(FALSE);
		ive_ll_pause();
	} else {
		ive_clearFrameEndFlag();
        g_kdrv_is_nonblocking = 0;
		//trigger ive start
#if IVE_MEASURE_TIME
		start_cnt = hwclock_get_longcounter();
#endif
		rt = ive_start();
		if (rt != E_OK) {
			goto IVE_END;
		}
		DBG_IND("ive start\r\n");

		if (ive_hook) {
			ive_hook(tri_hook.hook_mode, tri_hook.hook_value);
			if (tri_hook.hook_is_direct_return == 1) {
				ive_pause();
				goto IVE_END;
			}
		}

		if (p_ive_param->wait_end) {
			if (p_ive_param->time_out_ms != 0) {
				DBG_IND("wait FMD with timeout\r\n");

				ive_platform_set_timer(channel, p_ive_param);
				wai_flg(&flag_ptn, p_handle->flag_id, KDRV_IVE_FMD | KDRV_IVE_TIMEOUT, TWF_ORW | TWF_CLR);
				DBG_IND("flag_ptn 0x%x", flag_ptn);

				ive_platform_close_timer(channel);
				if (flag_ptn & KDRV_IVE_FMD) {
					DBG_IND("IVE frame end\r\n");
					ive_waitFrameEnd();
				} else if (flag_ptn & KDRV_IVE_TIMEOUT) {
					DBG_ERR("IVE timeout!\r\n");
				}
			} else {
				DBG_IND("wait FMD without timeout\r\n");
				ive_waitFrameEnd();
			}
			ive_pause();
		}
#if IVE_MEASURE_TIME
		end_cnt = hwclock_get_longcounter();
		DBG_DUMP("processing time: %llu us\r\n", end_cnt - start_cnt);
#endif
	}
	DBG_IND("2\r\n");

IVE_END:

	kdrv_ive_sem(p_handle_lock, FALSE);

	return (INT32) rt;

}

INT32 kdrv_ive_trigger_with_hook(UINT32 id, KDRV_IVE_TRIGGER_PARAM *p_ive_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data, 
						ive_hook_t ive_hook, KDRV_IVE_TRIGGER_HOOK_PARAM tri_hook)
{
	INT32 ret;

	ret = kdrv_ive_trigger_internal(id, p_ive_param, p_cb_func, p_user_data, ive_hook, tri_hook);

	return ret;
}

INT32 kdrv_ive_trigger(UINT32 id, KDRV_IVE_TRIGGER_PARAM *p_ive_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	INT32 ret;
	KDRV_IVE_TRIGGER_HOOK_PARAM  kdrv_ive_hook_parm = {0};

	kdrv_ive_hook_parm.hook_mode = (UINT32) 0;
	ret = kdrv_ive_trigger_internal(id, p_ive_param, p_cb_func, p_user_data, 0, kdrv_ive_hook_parm);

	return ret;
}
EXPORT_SYMBOL(kdrv_ive_trigger);


INT32 kdrv_ive_trigger_nonblock(UINT32 id)
{
	ER rt = E_ID;
	INT32 ret;
	KDRV_IVE_TRIGGER_HOOK_PARAM  kdrv_ive_hook_parm = {0};
	KDRV_IVE_TRIGGER_PARAM ive_param;
	KDRV_IVE_HANDLE *p_handle = 0;
	KDRV_IVE_HANDLE_NONBLOCK *p_handle_nonblock = 0;
	INT32 channel = KDRV_DEV_ID_CHANNEL(id);
	
	if (channel >= KDRV_IVE_HANDLE_MAX_NUM || channel < 0){
		DBG_ERR("ive kdrv: invalid id: %d\n", channel);
		return rt;
	}
	p_handle_nonblock = kdrv_ive_entry_id_conv2_handle_nonblock(channel);
	if (p_handle_nonblock == 0) {
		DBG_ERR("ive kdrv: invalid p_handle_nonblock: %d\n", channel);
        return rt;
	}
	p_handle = kdrv_ive_entry_id_conv2_handle(channel);
	if (p_handle == 0) {
        DBG_ERR("ive kdrv: invalid p_handle: %d\n", channel);
        return rt;
    }

	//prepare for triggering
	kdrv_ive_sem(p_handle, TRUE);
	rt = E_OK;
	if (p_handle_nonblock->stat == KDRV_IVE_IDLE || p_handle_nonblock->stat == KDRV_IVE_WAIT) {
		if (p_handle_nonblock->is_change == 0) {
			p_handle_nonblock->is_change = 1;
		} else {
			rt = E_PAR;
			DBG_IND("ive kdrv: Error, handle_nonblock[%d] sts=%d at change state\n", channel, p_handle_nonblock->stat);
			kdrv_ive_sem(p_handle, FALSE);
			return rt;
		}
	} else if (p_handle_nonblock->stat == KDRV_IVE_TRIG){
		rt = E_PAR;
		DBG_IND("ive kdrv: Error, handle_nonblock[%d] sts=%d at change state\n", channel, p_handle_nonblock->stat);
		kdrv_ive_sem(p_handle, FALSE);
		return rt;
	} else { //else if ((p_handle_nonblock->waitdone_ok == 1) && (p_handle_nonblock->trigger_ok == 1)){
		rt = E_PAR;
		DBG_IND("ive kdrv: Error (sts), handle_nonblock[%d] sts=%d\n", channel, p_handle_nonblock->stat);
		kdrv_ive_sem(p_handle, FALSE);
        return rt;
	}
    kdrv_ive_sem(p_handle, FALSE);

    
	kdrv_ive_hook_parm.hook_mode = (UINT32) 0;
	ive_param.is_nonblock = 0;
	ive_param.wait_end = 0;
	ive_param.time_out_ms = 0;
	ret = kdrv_ive_trigger_internal(id, &ive_param, NULL, NULL, 0, kdrv_ive_hook_parm);
	if (ret != E_OK) {
		DBG_ERR("ive kdrv(trigger_nonblock): Error to do trigger_internal(), channel=%d\n", channel);
		kdrv_ive_sem(p_handle, TRUE);
		p_handle_nonblock->stat = KDRV_IVE_WAIT;
		p_handle_nonblock->is_change = 0;
		kdrv_ive_sem(p_handle, FALSE);
		goto IVE_END;
	}
	ive_pause();

	//after triggering
	kdrv_ive_sem(p_handle, TRUE);
	rt = E_OK;
	if (p_handle_nonblock->stat == KDRV_IVE_IDLE || p_handle_nonblock->stat == KDRV_IVE_WAIT) {
		if (p_handle_nonblock->is_change == 1) {
			p_handle_nonblock->is_change = 0;
			p_handle_nonblock->stat = KDRV_IVE_TRIG;
		} else {
			DBG_IND("ive kdrv: Error, handle_nonblock[%d] sts=%d at change done state\n", channel, p_handle_nonblock->stat);
			kdrv_ive_sem(p_handle, FALSE);
			return rt;
		}
	} else if (p_handle_nonblock->stat == KDRV_IVE_TRIG){
		rt = E_PAR;
		DBG_IND("ive kdrv: Error, handle_nonblock[%d] sts=%d at change done state\n", channel, p_handle_nonblock->stat);
		kdrv_ive_sem(p_handle, FALSE);
		return rt;
	} else { //else if ((p_handle_nonblock->waitdone_ok == 1) && (p_handle_nonblock->trigger_ok == 1)){
		rt = E_PAR;
		DBG_IND("ive kdrv: Error (sts), handle_nonblock[%d] sts=%d\n", channel, p_handle_nonblock->stat);
		kdrv_ive_sem(p_handle, FALSE);
        return rt;
	}
    kdrv_ive_sem(p_handle, FALSE);

IVE_END:

	return ret;
}
EXPORT_SYMBOL(kdrv_ive_trigger_nonblock);

static ER kdrv_ive_set_opencfg(UINT32 id, void* data)
{
	KDRV_IVE_OPENCFG kdrv_ive_open_obj;

	if (data == NULL) {
		DBG_ERR("id %d input data NULL!\r\n", id);
		return E_PAR;
	}
	kdrv_ive_open_obj = *(KDRV_IVE_OPENCFG *)data;
	g_ive_open_cfg.IVE_CLOCKSEL = kdrv_ive_open_obj.ive_clock_sel;

	return E_OK;
}

static ER kdrv_ive_set_in_img_info(UINT32 id, void* data)
{
	g_kdrv_ive_info[id].in_img_info = *(KDRV_IVE_IN_IMG_INFO*)data;
	return E_OK;
}

static ER kdrv_ive_set_dma_in(UINT32 id, void* data)
{
	g_kdrv_ive_info[id].in_dma_info = *(KDRV_IVE_IMG_IN_DMA_INFO*)data;

	if(g_kdrv_ive_info[id].in_dma_info.addr == 0){
		DBG_ERR("KDRV IVE:Input address should not be zero\r\n");
		return E_PAR;
	}
	else if(g_kdrv_ive_info[id].in_dma_info.addr & 0x3){
		DBG_ERR("KDRV IVE: Input address should be word alignment\r\n");
		return E_PAR;
	}

	return E_OK;
}

static ER kdrv_ive_set_dma_out(UINT32 id, void* data)
{
	g_kdrv_ive_info[id].out_dma_info = *(KDRV_IVE_IMG_OUT_DMA_INFO*)data;

	if(g_kdrv_ive_info[id].out_dma_info.addr == 0){
		DBG_ERR("KDRV IVE: Output address should not be zero\r\n");
		return E_PAR;
	}
	else if(g_kdrv_ive_info[id].out_dma_info.addr & 0x3){
		DBG_ERR("KDRV IVE:Output address should be word alignment\r\n");
		return E_PAR;
	}

	return E_OK;
}

static ER kdrv_ive_set_isr_cb(UINT32 id, void* data)
{
	g_kdrv_ive_handle_tab[id].isrcb_fp = (KDRV_IPP_ISRCB)data;
	return E_OK;
}

static ER kdrv_ive_set_general_filter_param(UINT32 id, void *data)
{
	g_kdrv_ive_info[id].gen_filt = *(KDRV_IVE_GENERAL_FILTER_PARAM *)data;
	return E_OK;
}

static ER kdrv_ive_set_median_filter_param(UINT32 id, void *data)
{
	g_kdrv_ive_info[id].medn_filt = *(KDRV_IVE_MEDIAN_FILTER_PARAM *)data;
	if(nvt_get_chip_id() == CHIP_NA51055) {
		if (g_kdrv_ive_info[id].medn_filt.mode == KDEV_IVE_MEDIAN_IVD_TH) {
			DBG_ERR("KDRV IVE: Error, Median can't be used the mode(%d) on 520/525. \
					force to KDRV_IVE_MEDIAN (id=%d)\r\n",
					g_kdrv_ive_info[id].medn_filt.mode, id);
			g_kdrv_ive_info[id].medn_filt.mode = KDRV_IVE_MEDIAN;
		}
		g_kdrv_ive_info[id].medn_filt.medn_inval_th = 0;
	}
		
	return E_OK;
}

static ER kdrv_ive_set_edge_filter_param(UINT32 id, void *data)
{
	g_kdrv_ive_info[id].edge_filt = *(KDRV_IVE_EDGE_FILTER_PARAM *)data;
	return E_OK;
}

static ER kdrv_ive_set_irv_param(UINT32 id, void *data)
{
	g_kdrv_ive_info[id].irv = *(KDRV_IVE_IRV_PARAM *)data;
	return E_OK;
}

static ER kdrv_ive_set_flowct_param(UINT32 id, void *data)
{
	g_kdrv_ive_info[id].flow_ct = *(KDRV_IVE_FLOW_CT_PARAM *)data;
	return E_OK;
}

static ER kdrv_ive_set_non_max_sup_param(UINT32 id, void *data)
{
	g_kdrv_ive_info[id].non_max_sup = *(KDRV_IVE_NON_MAX_SUP_PARAM *)data;
	return E_OK;
}

static ER kdrv_ive_set_threshold_lut_param(UINT32 id, void *data)
{
	g_kdrv_ive_info[id].thres_lut = *(KDRV_IVE_THRES_LUT_PARAM *)data;
	return E_OK;
}

static ER kdrv_ive_set_morph_filter_param(UINT32 id, void *data)
{
	g_kdrv_ive_info[id].morph_filt = *(KDRV_IVE_MORPH_FILTER_PARAM *)data;
	return E_OK;
}

static ER kdrv_ive_set_integral_img_param(UINT32 id, void *data)
{
	g_kdrv_ive_info[id].integral_img = *(KDRV_IVE_INTEGRAL_IMG_PARAM *)data;
	return E_OK;
}


static ER kdrv_ive_set_outdata_sel_param(UINT32 id, void *data)
{
	g_kdrv_ive_info[id].outsel_info = *(KDRV_IVE_OUTSEL_PARAM *)data;

	return E_OK;
}

static ER kdrv_ive_set_dma_in_ll(UINT32 id, void* data)
{
	g_kdrv_ive_info[id].ll_dma_info = *(KDRV_IVE_LL_DMA_INFO*)data;

	return E_OK;
}

KDRV_IVE_SET_FP kdrv_ive_set_fp[KDRV_IVE_PARAM_MAX] = {
	kdrv_ive_set_opencfg,
	kdrv_ive_set_in_img_info,
	kdrv_ive_set_dma_in,
	kdrv_ive_set_dma_out,
	kdrv_ive_set_isr_cb,
	kdrv_ive_set_general_filter_param,
	kdrv_ive_set_median_filter_param,
	kdrv_ive_set_edge_filter_param,
	kdrv_ive_set_non_max_sup_param,
	kdrv_ive_set_threshold_lut_param,
	kdrv_ive_set_morph_filter_param,
	kdrv_ive_set_integral_img_param,
	kdrv_ive_set_outdata_sel_param,
	kdrv_ive_set_dma_in_ll,
	kdrv_ive_set_irv_param,
	kdrv_ive_set_flowct_param,
};

INT32 kdrv_ive_set(UINT32 id, KDRV_IVE_PARAM_ID parm_id, VOID *p_param)
{
	ER rt = E_OK;
	//UINT32 ign_chk; //set but not used
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);

	//ign_chk = (KDRV_IVE_IGN_CHK & parm_id);
	parm_id = parm_id & (~KDRV_IVE_IGN_CHK);

	if (kdrv_ive_set_fp[parm_id] != NULL && KDRV_DEV_ID_CHANNEL(id) < KDRV_IVE_HANDLE_MAX_NUM) {
		rt = kdrv_ive_set_fp[parm_id](channel, p_param);
	}else{
		DBG_ERR("KDRV IVE: kdrv_ive_set fail on id : %d,  parm_id : %d \r\n",id , (UINT32)parm_id);
		rt = E_PAR ;
	}

	return (INT32) rt;
}
EXPORT_SYMBOL(kdrv_ive_set);

static ER kdrv_ive_get_in_img_info(UINT32 id, void* data)
{
	*(KDRV_IVE_IN_IMG_INFO*)data = g_kdrv_ive_info[id].in_img_info;
	return E_OK;
}

static ER kdrv_ive_get_dma_in(UINT32 id, void* data)
{
	*(KDRV_IVE_IMG_IN_DMA_INFO*)data = g_kdrv_ive_info[id].in_dma_info;
	return E_OK;
}

static ER kdrv_ive_get_dma_out(UINT32 id, void* data)
{
	*(KDRV_IVE_IMG_OUT_DMA_INFO*)data = g_kdrv_ive_info[id].out_dma_info;
	return E_OK;
}

static ER kdrv_ive_get_isr_cb(UINT32 id, void* data)
{
	*(KDRV_IPP_ISRCB*)data = g_kdrv_ive_handle_tab[id].isrcb_fp;
	return E_OK;
}

static ER kdrv_ive_get_general_filter_param(UINT32 id, void *data)
{
	*(KDRV_IVE_GENERAL_FILTER_PARAM *)data = g_kdrv_ive_info[id].gen_filt;
	return E_OK;
}

static ER kdrv_ive_get_median_filter_param(UINT32 id, void *data)
{
	*(KDRV_IVE_MEDIAN_FILTER_PARAM *)data = g_kdrv_ive_info[id].medn_filt;
	return E_OK;
}

static ER kdrv_ive_get_edge_filter_param(UINT32 id, void *data)
{
	*(KDRV_IVE_EDGE_FILTER_PARAM *)data = g_kdrv_ive_info[id].edge_filt;
	return E_OK;
}

static ER kdrv_ive_get_irv_param(UINT32 id, void *data)
{
	*(KDRV_IVE_IRV_PARAM *)data = g_kdrv_ive_info[id].irv;
	return E_OK;
}

static ER kdrv_ive_get_flowct_param(UINT32 id, void *data)
{
	*(KDRV_IVE_FLOW_CT_PARAM *)data = g_kdrv_ive_info[id].flow_ct;
	return E_OK;
}

static ER kdrv_ive_get_non_max_sup_param(UINT32 id, void *data)
{
	*(KDRV_IVE_NON_MAX_SUP_PARAM *)data = g_kdrv_ive_info[id].non_max_sup;
	return E_OK;
}

static ER kdrv_ive_get_threshold_lut_param(UINT32 id, void *data)
{
	*(KDRV_IVE_THRES_LUT_PARAM *)data = g_kdrv_ive_info[id].thres_lut;
	return E_OK;
}

static ER kdrv_ive_get_morph_filter_param(UINT32 id, void *data)
{
	*(KDRV_IVE_MORPH_FILTER_PARAM *)data = g_kdrv_ive_info[id].morph_filt;
	return E_OK;
}

static ER kdrv_ive_get_integral_img_param(UINT32 id, void *data)
{
	*(KDRV_IVE_INTEGRAL_IMG_PARAM *)data = g_kdrv_ive_info[id].integral_img;
	return E_OK;
}

static ER kdrv_ive_get_outdata_sel_param(UINT32 id, void *data)
{
	*(KDRV_IVE_OUTSEL_PARAM *)data = g_kdrv_ive_info[id].outsel_info;
	return E_OK;
}

static ER kdrv_ive_get_dma_in_ll(UINT32 id, void* data)
{
	*(KDRV_IVE_LL_DMA_INFO*)data = g_kdrv_ive_info[id].ll_dma_info;

	return E_OK;
}

KDRV_IVE_GET_FP kdrv_ive_get_fp[KDRV_IVE_PARAM_MAX] = {
	NULL,
	kdrv_ive_get_in_img_info,
	kdrv_ive_get_dma_in,
	kdrv_ive_get_dma_out,
	kdrv_ive_get_isr_cb,
	kdrv_ive_get_general_filter_param,
	kdrv_ive_get_median_filter_param,
	kdrv_ive_get_edge_filter_param,
	kdrv_ive_get_non_max_sup_param,
	kdrv_ive_get_threshold_lut_param,
	kdrv_ive_get_morph_filter_param,
	kdrv_ive_get_integral_img_param,
	kdrv_ive_get_outdata_sel_param,
	kdrv_ive_get_dma_in_ll,
	kdrv_ive_get_irv_param,
	kdrv_ive_get_flowct_param,
};

INT32 kdrv_ive_get(UINT32 id, KDRV_IVE_PARAM_ID parm_id, VOID *p_param)
{
	ER rt = E_OK;

	//UINT32 ign_chk; //set but not used

	//ign_chk = (KDRV_IVE_IGN_CHK & parm_id);
	parm_id = parm_id & (~KDRV_IVE_IGN_CHK);
	if (kdrv_ive_get_fp[parm_id] != NULL && KDRV_DEV_ID_CHANNEL(id) < KDRV_IVE_HANDLE_MAX_NUM) {
		rt = kdrv_ive_get_fp[parm_id](KDRV_DEV_ID_CHANNEL(id), p_param);
	}

	return (INT32) rt;
}
EXPORT_SYMBOL(kdrv_ive_get);

#if !defined(CONFIG_NVT_SMALL_HDAL)
void kdrv_ive_dump_info(int (*dump)(const char *fmt, ...))
{
	BOOL is_print_header;
	BOOL is_open[KDRV_IVE_HANDLE_MAX_NUM];
	INT32 id;

	memset((void *)is_open, FALSE, (sizeof(BOOL) * KDRV_IVE_HANDLE_MAX_NUM));
	

	for (id = 0; id < KDRV_IVE_HANDLE_MAX_NUM; id++) {
		if ((g_kdrv_ive_init_flg & (1 << id)) == (UINT32)(1 << id)) {
			is_open[id] = TRUE;
			kdrv_ive_lock(&g_kdrv_ive_handle_tab[id], TRUE);
		}
	}

	dump("\r\n[IVE]\r\n");
	dump("\r\n-----ctrl info-----\r\n");
	dump("%12s%12s\r\n", "init_flg", "trig_id");
	dump("%#12x 0x%.8x\r\n", g_kdrv_ive_init_flg, (KDRV_IVE_HANDLE *) ive_getTriHdl());

	/**
		input info
	*/
	dump("\r\n-----input info-----\r\n");

	/**
		ouput info
	*/
	dump("\r\n-----output info-----\r\n");

	/**
		Function enable info
	*/
	dump("\r\n-----Function enable info-----\r\n");

	/**
		IQ parameter info
	*/
	dump("\r\n-----IQ parameter info-----\r\n");
	// CFA
	is_print_header = FALSE;
	for (id = 0; id < KDRV_IVE_HANDLE_MAX_NUM; id++) {
		if (!is_print_header) {
			dump("%12s%3s%12s%12s%12s%12s\r\n", "CFA-Dir", "id", "PAT", "DifNBit", "NsMarDiff", "NsMarEdge");
			is_print_header = TRUE;
		}
	}

	for (id = 0; id < KDRV_IVE_HANDLE_MAX_NUM; id++) {
		if (is_open[id]) {
			kdrv_ive_lock(&g_kdrv_ive_handle_tab[id], FALSE);
		}
	}

}
EXPORT_SYMBOL(kdrv_ive_dump_info);

INT32 kdrv_ive_waitdone_nonblock(UINT32 id, UINT32 *timeout)
{
	ER rt = E_OK;
    KDRV_IVE_HANDLE *p_handle = 0;
    INT32 channel = KDRV_DEV_ID_CHANNEL(id);
#if IVE_MEASURE_TIME
    UINT64 start_cnt, end_cnt;
#endif
	KDRV_IVE_HANDLE_NONBLOCK *p_handle_nonblock = 0;
	
	if (channel >= KDRV_IVE_HANDLE_MAX_NUM || channel < 0){
		DBG_ERR("ive kdrv: invalid id: %d\n", channel);
		goto IVE_END_NO_SEM;
	}
	if (timeout == 0) {
		DBG_ERR("ive kdrv: invalid timeout_ptr: %d\n", channel);
		goto IVE_END_NO_SEM;
	}
	p_handle_nonblock = kdrv_ive_entry_id_conv2_handle_nonblock(channel);
	if (p_handle_nonblock == 0) {
		 DBG_ERR("ive kdrv: invalid p_handle_nonblock: %d\n", channel);
        goto IVE_END_NO_SEM;
	}
	p_handle = kdrv_ive_entry_id_conv2_handle(channel);
	if (p_handle == 0) {
         DBG_ERR("ive kdrv: invalid p_handle: %d\n", channel);
        goto IVE_END_NO_SEM;
    }

	//prepare for waitdone
	kdrv_ive_sem(p_handle, TRUE);
	rt = E_OK;
	if (p_handle_nonblock->stat == KDRV_IVE_TRIG) {
		if (p_handle_nonblock->is_change == 0) {
			p_handle_nonblock->is_change = 1;
		} else {
			rt = E_PAR;
			DBG_IND("ive kdrv: Error, handle_nonblock[%d] sts=%d at change state\n", channel, p_handle_nonblock->stat);
			goto IVE_END;
		}
	} else if (p_handle_nonblock->stat == KDRV_IVE_IDLE || p_handle_nonblock->stat == KDRV_IVE_WAIT) {
		rt = E_OK;
		goto IVE_END;
	} else { //else if ((p_handle_nonblock->waitdone_ok == 1) && (p_handle_nonblock->trigger_ok == 1)){
		rt = E_PAR;
		DBG_IND("ive kdrv: Error (sts), handle_nonblock[%d] sts=%d\n", channel, p_handle_nonblock->stat);
		goto IVE_END;
	}

	DBG_IND("id 0x%x\r\n", id);

#if IVE_MEASURE_TIME
    start_cnt = hwclock_get_longcounter();
#endif

	rt = ive_waitFrameEnd_timeout(*timeout);
	if (rt != E_OK) {
		rt = IVE_MSG_TIMEOUT;
		DBG_IND("ive kdrv: timeout: %d\n", channel);
		p_handle_nonblock->stat = KDRV_IVE_TRIG;
		p_handle_nonblock->is_change = 0;
		goto IVE_END;
	}
	ive_pause();

#if IVE_MEASURE_TIME
    end_cnt = hwclock_get_longcounter();
    DBG_DUMP("set_mode time: %llu us\r\n", end_cnt - start_cnt);
#endif

	//prepare for waitdone
	rt = E_OK;
	if (p_handle_nonblock->stat == KDRV_IVE_TRIG) {
		if (p_handle_nonblock->is_change == 1) {
			p_handle_nonblock->is_change = 0;
			p_handle_nonblock->stat = KDRV_IVE_WAIT;
		} else {
			rt = E_PAR;
			DBG_IND("ive kdrv: Error, handle_nonblock[%d] sts=%d at change done state\n", channel, p_handle_nonblock->stat);
			goto IVE_END;
		}
	} else { //else if ((p_handle_nonblock->waitdone_ok == 1) && (p_handle_nonblock->trigger_ok == 1)){
		rt = E_PAR;
		DBG_IND("ive kdrv: Error (sts), handle_nonblock[%d] sts=%d\n", channel, p_handle_nonblock->stat);
		goto IVE_END;	
	}
    
IVE_END:

    kdrv_ive_sem(p_handle, FALSE);

IVE_END_NO_SEM:

    return (INT32) rt;

}
EXPORT_SYMBOL(kdrv_ive_waitdone_nonblock);
#endif
