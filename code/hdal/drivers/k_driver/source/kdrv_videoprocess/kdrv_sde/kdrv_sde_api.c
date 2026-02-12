/**
    @file       kdrv_sde.c
    @ingroup    Predefined_group_name

    @brief      sde device abstraction layer

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#include "kdrv_sde_int.h"
#include "sde_platform.h"
#include "sde_lib.h"

#include "kdrv_videoprocess/kdrv_sde.h"


static UINT32 g_kdrv_sde_init_flg = 0;
static KDRV_SDE_HANDLE *g_kdrv_sde_trig_hdl = 0;
static KDRV_SDE_HANDLE g_kdrv_sde_handle_tab[KDRV_SDE_HANDLE_MAX_NUM];
static KDRV_SDE_OPENCFG g_kdrv_sde_open_cfg = {0};
static UINT32 g_kdrv_sde_open_times = 0;

KDRV_SDE_PRAM g_kdrv_sde_info[KDRV_SDE_HANDLE_MAX_NUM];

SDE_PARAM sde_drv_param = {0};
static UINT32 g_isr_cb_cnt = 0;

static BOOL kdrv_sde_check_align(UINT32 chk_value, UINT32 align)
{
	return ((chk_value % align) == 0) ? TRUE : FALSE;
}

static BOOL kdrv_sde_check_noncache_addr(UINT32 chk_addr)
{
	//return !(dma_isCacheAddr(chk_addr));
	return TRUE;
}

static ER kdrv_sde_init_param(INT32 id)
{

	ER rt = E_OK;
	memset(&g_kdrv_sde_info[id], 0, sizeof(g_kdrv_sde_info[id]));

	return rt;
}

static ER kdrv_sde_setmode_run1(INT32 id)
{
	ER rt = E_OK;

	sde_drv_param.ioPara.Size.uiWidth  = g_kdrv_sde_info[id].io_info.width;
	sde_drv_param.ioPara.Size.uiHeight = g_kdrv_sde_info[id].io_info.height;

	//SDE lofs setting
	sde_drv_param.ioPara.Size.uiOfsi0 = g_kdrv_sde_info[id].io_info.in0_lofs;
	sde_drv_param.ioPara.Size.uiOfsi1 = g_kdrv_sde_info[id].io_info.in1_lofs;
	sde_drv_param.ioPara.Size.uiOfsi2 = g_kdrv_sde_info[id].io_info.vol_lofs;
	sde_drv_param.ioPara.Size.uiOfso = g_kdrv_sde_info[id].io_info.vol_lofs;

	//SDE address
	sde_drv_param.ioPara.uiInAddr0 = (UINT32)g_kdrv_sde_info[id].io_info.in_addr0;
	sde_drv_param.ioPara.uiInAddr1 = (UINT32)g_kdrv_sde_info[id].io_info.in_addr1;
	sde_drv_param.ioPara.uiInAddr2 = (UINT32)g_kdrv_sde_info[id].io_info.vol_addr;
	sde_drv_param.ioPara.uiOutAddr = (UINT32)g_kdrv_sde_info[id].io_info.vol_addr;

	//SDE_OUT_VOL
	sde_drv_param.bHflip01 = 1;
	sde_drv_param.bVflip01 = 1;
	sde_drv_param.bHflip2 = 0;
	sde_drv_param.bVflip2 = 0;
	sde_drv_param.bInput2En = 0;
	sde_drv_param.bCostCompMode = 1;
	sde_drv_param.outSel = 1; // output vol.

	sde_drv_param.opMode = g_kdrv_sde_info[id].mode_info.op_mode; // Disparity 31 or 63 scan mode
	sde_drv_param.outVolSel = 0; // Full volumn mode, 4 scan each
	sde_drv_param.bScanEn = 1;
	sde_drv_param.bDiagEn = 0;
	sde_drv_param.invSel = g_kdrv_sde_info[id].mode_info.inv_sel;

	// Cost parameters
	sde_drv_param.costPara.uiAdBoundLower = g_kdrv_sde_info[id].cost_param.ad_bound_lower;
    sde_drv_param.costPara.uiAdBoundUpper = g_kdrv_sde_info[id].cost_param.ad_bound_upper;
    sde_drv_param.costPara.uiCensusBoundLower = g_kdrv_sde_info[id].cost_param.census_bound_lower;
    sde_drv_param.costPara.uiCensusBoundUpper = g_kdrv_sde_info[id].cost_param.census_bound_upper;
    sde_drv_param.costPara.uiCensusAdRatio =  g_kdrv_sde_info[id].cost_param.ad_census_ratio;

    // Luma parameters
    sde_drv_param.lumaPara.uiLumaThreshSaturated = g_kdrv_sde_info[id].luma_param.luma_saturated_th;
    sde_drv_param.lumaPara.uiCostSaturatedMatch = g_kdrv_sde_info[id].luma_param.cost_saturated;
    sde_drv_param.lumaPara.uiDeltaLumaSmoothThresh = g_kdrv_sde_info[id].luma_param.delta_luma_smooth_th;

    // Scanning paramters
    sde_drv_param.penaltyValues.uiScanPenaltyNonsmooth = g_kdrv_sde_info[id].penalty_param.penalty_nonsmooth;
    sde_drv_param.penaltyValues.uiScanPenaltySmooth0 = g_kdrv_sde_info[id].penalty_param.penalty_smooth0;
    sde_drv_param.penaltyValues.uiScanPenaltySmooth1 = g_kdrv_sde_info[id].penalty_param.penalty_smooth1;
    sde_drv_param.penaltyValues.uiScanPenaltySmooth2 = g_kdrv_sde_info[id].penalty_param.penalty_smooth2;

    sde_drv_param.thSmooth.uiDeltaDispLut0 = g_kdrv_sde_info[id].smooth_param.delta_displut0;
    sde_drv_param.thSmooth.uiDeltaDispLut1 = g_kdrv_sde_info[id].smooth_param.delta_displut1;

    // LRC diagonal parameters
    sde_drv_param.thDiag.uiDiagThreshLut0 = g_kdrv_sde_info[id].lrc_th_param.thresh_lut0;
    sde_drv_param.thDiag.uiDiagThreshLut1 = g_kdrv_sde_info[id].lrc_th_param.thresh_lut1;
    sde_drv_param.thDiag.uiDiagThreshLut2 = g_kdrv_sde_info[id].lrc_th_param.thresh_lut2;
    sde_drv_param.thDiag.uiDiagThreshLut3 = g_kdrv_sde_info[id].lrc_th_param.thresh_lut3;
    sde_drv_param.thDiag.uiDiagThreshLut4 = g_kdrv_sde_info[id].lrc_th_param.thresh_lut4;
    sde_drv_param.thDiag.uiDiagThreshLut5 = g_kdrv_sde_info[id].lrc_th_param.thresh_lut5;
    sde_drv_param.thDiag.uiDiagThreshLut6 = g_kdrv_sde_info[id].lrc_th_param.thresh_lut6;


	// set to SDE engine
	rt |= sde_setMode(&sde_drv_param);

	return rt;
}

static ER kdrv_sde_setmode_run2(INT32 id)
{
	ER rt = E_OK;

	//SDE lofs setting
	sde_drv_param.ioPara.Size.uiOfsi0 = g_kdrv_sde_info[id].io_info.in0_lofs;
	sde_drv_param.ioPara.Size.uiOfsi1 = g_kdrv_sde_info[id].io_info.in1_lofs;
	sde_drv_param.ioPara.Size.uiOfsi2 = g_kdrv_sde_info[id].io_info.vol_lofs;
	sde_drv_param.ioPara.Size.uiOfso = g_kdrv_sde_info[id].io_info.out_lofs;

	//SDE address
	sde_drv_param.ioPara.uiInAddr0 = (UINT32)g_kdrv_sde_info[id].io_info.in_addr0;
	sde_drv_param.ioPara.uiInAddr1 = (UINT32)g_kdrv_sde_info[id].io_info.in_addr1;
	sde_drv_param.ioPara.uiInAddr2 = (UINT32)g_kdrv_sde_info[id].io_info.vol_addr;
	sde_drv_param.ioPara.uiOutAddr = (UINT32)g_kdrv_sde_info[id].io_info.out_addr;

	//SDE_OUT_VOL
	sde_drv_param.bHflip01 = 0;
	sde_drv_param.bVflip01 = 0;
	sde_drv_param.bHflip2 = 1;
	sde_drv_param.bVflip2 = 1;
	sde_drv_param.bInput2En = 1;
	sde_drv_param.bCostCompMode = 0;
	sde_drv_param.outSel = 0; // depth map.

	sde_drv_param.opMode = g_kdrv_sde_info[id].mode_info.op_mode; // Disparity 31 or 63 scan mode
	sde_drv_param.outVolSel = 0; // Full volumn mode, 4 scan each
	sde_drv_param.bScanEn = 1;
	sde_drv_param.bDiagEn = 1;
	sde_drv_param.invSel = g_kdrv_sde_info[id].mode_info.inv_sel;

	//TODO alex, confidence_th

	//-----------------------------
	
	rt |= sde_setMode(&sde_drv_param);


	return rt;
}

static void kdrv_sde_lock(KDRV_SDE_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		FLGPTN flag_ptn;
		wai_flg(&flag_ptn, p_handle->flag_id, p_handle->lock_bit, TWF_CLR);
	} else {
		set_flg(p_handle->flag_id, p_handle->lock_bit);
	}
}

static KDRV_SDE_HANDLE *kdrv_sde_chk_handle(KDRV_SDE_HANDLE *p_handle)
{
	UINT32 i;

	for (i = 0; i < KDRV_SDE_HANDLE_MAX_NUM; i ++) {
		if (p_handle == &g_kdrv_sde_handle_tab[i]) {
			return p_handle;
		}
	}

	return NULL;
}

static void kdrv_sde_handle_lock(void)
{
	FLGPTN wait_flg;
	wai_flg(&wait_flg, FLG_ID_KDRV_SDE, KDRV_SDE_HDL_UNLOCK, (TWF_ORW | TWF_CLR));
}

static void kdrv_sde_handle_unlock(void)
{
	set_flg(FLG_ID_KDRV_SDE, KDRV_SDE_HDL_UNLOCK);
}

static INT32 kdrv_sde_handle_alloc(void)
{
	UINT32 i;
	ER rt = E_OK;

	kdrv_sde_handle_lock();
	for (i = 0; i < KDRV_SDE_HANDLE_MAX_NUM; i++) {
		if (!(g_kdrv_sde_init_flg & (1 << i))) {

			g_kdrv_sde_init_flg |= (1 << i);

			memset(&g_kdrv_sde_handle_tab[i], 0, sizeof(KDRV_SDE_HANDLE));
			g_kdrv_sde_handle_tab[i].flag_id = FLG_ID_KDRV_SDE;
			g_kdrv_sde_handle_tab[i].lock_bit = (1 << i);
			g_kdrv_sde_handle_tab[i].sts |= KDRV_SDE_HANDLE_LOCK;
			g_kdrv_sde_handle_tab[i].sem_id = (SEM_HANDLE*)&SEMID_KDRV_SDE;//kdrv_sde_get_sem_id(SEMID_KDRV_SDE);

		}
	}
	kdrv_sde_handle_unlock();


	return rt;
}

static UINT32 kdrv_sde_handle_free(void)
{
	UINT32 rt = FALSE;
	UINT32 ch = 0;
	KDRV_SDE_HANDLE *p_handle;

	kdrv_sde_handle_lock();
	for (ch = 0; ch < KDRV_SDE_HANDLE_MAX_NUM; ch++) {
		p_handle = &(g_kdrv_sde_handle_tab[ch]);
		p_handle->sts = 0;
		g_kdrv_sde_init_flg &= ~(1 << ch);
		if (g_kdrv_sde_init_flg == 0) {
			rt = TRUE;
		}
	}
	kdrv_sde_handle_unlock();

	return rt;
}

static KDRV_SDE_HANDLE *kdrv_sde_entry_id_conv2_handle(UINT32 entry_id)
{
	return  &g_kdrv_sde_handle_tab[entry_id];
}


static void kdrv_sde_sem(KDRV_SDE_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		SEM_WAIT(*p_handle->sem_id);    // wait semaphore
	} else {
		SEM_SIGNAL(*p_handle->sem_id);  // wait semaphore
	}
}

static void kdrv_sde_frm_end(KDRV_SDE_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		set_flg(p_handle->flag_id, KDRV_SDE_FMD);
	} else {
		clr_flg(p_handle->flag_id, KDRV_SDE_FMD);
	}
}

static void kdrv_sde_isr_cb(UINT32 intstatus)
{
	KDRV_SDE_HANDLE *p_handle;
	UINT32 kdrv_intstatus;
	p_handle = g_kdrv_sde_trig_hdl;

	if (intstatus & SDE_INTE_FRMEND) {
		sde_pause();
		kdrv_sde_sem(p_handle, FALSE);
		kdrv_sde_frm_end(p_handle, TRUE);
		g_isr_cb_cnt++;
	}

	if ((g_isr_cb_cnt==2) &&(p_handle->isrcb_fp != NULL)) {
		kdrv_intstatus = KDRV_SDE_INT_2FMD;
		p_handle->isrcb_fp((UINT32)p_handle, kdrv_intstatus, NULL, NULL);
		g_isr_cb_cnt = 0;
	}
}

UINT32 kdrv_sde_init(VOID)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	sde_platform_create_resource();
	sde_platform_set_clk_rate(240);
	kdrv_sde_install_id();
	kdrv_sde_flow_init();
	return 0;
#else


	return 0;
#endif

#if 0
	UINT32 buffer_size = 0;
	UINT32 channel_used = channel_num;

	kdrv_ipe_flow_init();

	if (channel_used == 0) {
		DBG_ERR("IPE min channel number = 1\r\n");
		g_kdrv_ipe_channel_num =  1;
	} else if (channel_used > KDRV_IPP_MAX_CHANNEL_NUM) {
		DBG_ERR("IPE max channel number = %d\r\n", KDRV_IPP_MAX_CHANNEL_NUM);
		channel_used =  KDRV_IPP_MAX_CHANNEL_NUM;
	}

	g_kdrv_ipe_info = (KDRV_IPE_PRAM *)input_addr;
	buffer_size = ALIGN_CEIL_64(channel_used * sizeof(KDRV_IPE_PRAM));

	g_kdrv_ipe_handle_tab = (KDRV_IPE_HANDLE *)(input_addr + buffer_size);
	buffer_size += ALIGN_CEIL_64(channel_used * sizeof(KDRV_IPE_HANDLE));

	g_kdrv_ipe_ipl_func_en_sel = (UINT32 *)(input_addr + buffer_size);
	buffer_size += ALIGN_CEIL_64(channel_used * sizeof(UINT32));

	return buffer_size;
#endif
}

VOID kdrv_sde_uninit(VOID)
{

#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	sde_platform_release_resource();
	//kdrv_sde_uninstall_id();
#endif

}

INT32 kdrv_sde_open(UINT32 chip, UINT32 engine)
{
	SDE_OPENOBJ sde_drv_obj;
	KDRV_SDE_OPENCFG kdrv_sde_open_obj;
	int ch = 0;

	if (engine != KDRV_CV_ENGINE_SDE) {
		DBG_ERR("Invalid engine 0x%x\r\n", engine);
		return E_ID;
	}

	if (g_kdrv_sde_init_flg == 0) {
		if (kdrv_sde_handle_alloc()) {
			DBG_WRN("KDRV SDE: no free handle, max handle num = %d\r\n", KDRV_SDE_HANDLE_MAX_NUM);
			return E_SYS;
		}
	}

	if(g_kdrv_sde_open_times == 0) {

		kdrv_sde_open_obj = g_kdrv_sde_open_cfg; //*(KDRV_SDE_OPENCFG *)open_cfg;
		sde_drv_obj.uiSdeClockSel = kdrv_sde_open_obj.sde_clock;
		sde_drv_obj.FP_SDEISR_CB = kdrv_sde_isr_cb;

		if (sde_open(&sde_drv_obj) != E_OK) {
			kdrv_sde_handle_free();
			DBG_WRN("KDRV SDE: sde_open failed\r\n");
			return E_SYS;
		}

		for (ch = 0; ch < KDRV_SDE_HANDLE_MAX_NUM; ch++) {
			kdrv_sde_lock(&g_kdrv_sde_handle_tab[ch], TRUE);

			// init internal parameter
			kdrv_sde_init_param(ch);
			g_isr_cb_cnt = 0;

			kdrv_sde_lock(&g_kdrv_sde_handle_tab[ch], FALSE);
		}

	}
	g_kdrv_sde_open_times++;

	return E_OK;
}

INT32 kdrv_sde_close(UINT32 chip, UINT32 engine)
{
	ER rt = E_OK;
	KDRV_SDE_HANDLE *p_handle;
	UINT32 ch = 0;

	if (engine != KDRV_CV_ENGINE_SDE) {
		DBG_ERR("Invalid engine 0x%x\r\n", engine);
		return E_ID;
	}

	for (ch = 0; ch < KDRV_SDE_HANDLE_MAX_NUM; ch++) {
		p_handle = (KDRV_SDE_HANDLE *)&g_kdrv_sde_handle_tab[ch];
		if (kdrv_sde_chk_handle(p_handle) == NULL) {
			DBG_ERR("KDRV SDE: illegal handle[%d] 0x%.8x\r\n", ch, (UINT32)&g_kdrv_sde_handle_tab[ch]);
			return E_SYS;
		}

		if ((p_handle->sts & KDRV_SDE_HANDLE_LOCK) != KDRV_SDE_HANDLE_LOCK) {
			DBG_ERR("KDRV SDE: illegal handle[%d] sts 0x%.8x\r\n", ch, p_handle->sts);;
			return E_SYS;
		}

	}

	g_kdrv_sde_open_times--;
	if(g_kdrv_sde_open_times == 0){
		//DBG_IND("sde count 0 close\r\n");

		for (ch = 0; ch < KDRV_SDE_HANDLE_MAX_NUM; ch++) {
			p_handle = (KDRV_SDE_HANDLE *)&g_kdrv_sde_handle_tab[ch];
			kdrv_sde_lock(p_handle, TRUE);
		}

		if (kdrv_sde_handle_free()) {
			rt = sde_close();
		}

		for (ch = 0; ch < KDRV_SDE_HANDLE_MAX_NUM; ch++) {
			p_handle = (KDRV_SDE_HANDLE *)&g_kdrv_sde_handle_tab[ch];
			kdrv_sde_lock(p_handle, FALSE);
		}
	}

	return rt;
}

INT32 kdrv_sde_trigger(UINT32 id, KDRV_SDE_TRIGGER_PARAM *p_sde_drv_param,
					   KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	ER rt = E_OK;
	KDRV_SDE_HANDLE *p_handle;
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);

	//DBG_IND("id = 0x%x\r\n", id);
	//if (p_sde_drv_param == NULL) {
	//  DBG_ERR("p_sde_drv_param is NULL\r\n");
	//  return E_SYS;
	//}
	if (KDRV_DEV_ID_ENGINE(id) != KDRV_CV_ENGINE_SDE) {
		DBG_ERR("Invalid engine 0x%x\r\n", KDRV_DEV_ID_ENGINE(id));
		return E_ID;
	}

	p_handle = kdrv_sde_entry_id_conv2_handle(channel);

	//run1
	//DBG_IND("run1 \r\n");
	kdrv_sde_sem(p_handle, TRUE);
	//set sde kdrv parameters to sde driver
	rt = kdrv_sde_setmode_run1(channel);
	if (rt != E_OK) {
		kdrv_sde_sem(p_handle, FALSE);
		return rt;
	}
	//update trig id and trig_cfg
	g_kdrv_sde_trig_hdl = p_handle;
	//trigger sde start
	kdrv_sde_frm_end(p_handle, FALSE);
	sde_clrFrameEnd();

	//trigger sde start
	rt = sde_start();
	if (rt != E_OK) {
		kdrv_sde_sem(p_handle, FALSE);
		return rt;
	}

	sde_waitFlagFrameEnd();
#if 1
	//DBG_IND("run2 \r\n");
	//run2
	kdrv_sde_sem(p_handle, TRUE);
	//set sde kdrv parameters to sde driver
	rt = kdrv_sde_setmode_run2(channel);
	if (rt != E_OK) {
		kdrv_sde_sem(p_handle, FALSE);
		return rt;
	}
	//update trig id and trig_cfg
	g_kdrv_sde_trig_hdl = p_handle;
	//trigger sde start
	kdrv_sde_frm_end(p_handle, FALSE);
	sde_clrFrameEnd();

	//trigger sde start
	rt = sde_start();
	if (rt != E_OK) {
		kdrv_sde_sem(p_handle, FALSE);
		return rt;
	}
	sde_waitFlagFrameEnd();
#endif
	return rt;
}


#if 0
#endif

static ER kdrv_sde_set_opencfg(UINT32 id, void *data)
{
	KDRV_SDE_OPENCFG *open_param;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}
	open_param = (KDRV_SDE_OPENCFG *)data;
	g_kdrv_sde_open_cfg.sde_clock = open_param->sde_clock;

	return E_OK;
}

static ER kdrv_sde_set_modeinfo(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_SDE_MODE_INFO *mode_info;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	mode_info = (KDRV_SDE_MODE_INFO *)data;

	g_kdrv_sde_info[id].mode_info.op_mode = mode_info->op_mode;
	g_kdrv_sde_info[id].mode_info.inv_sel = mode_info->inv_sel;

	return rt;

}

static ER kdrv_sde_set_io_info(UINT32 id, void *data)
{
	ER rt = E_OK;
	BOOL b_align = TRUE;
	BOOL b_noncache_addr = TRUE;
	KDRV_SDE_IO_INFO *io_info;

	if (data == NULL) {
		DBG_ERR("id %u input param error 0x%08x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	io_info = (KDRV_SDE_IO_INFO *)data;

	g_kdrv_sde_info[id].io_info.width = io_info->width;
	g_kdrv_sde_info[id].io_info.height = io_info->height;

	b_align &= kdrv_sde_check_align(io_info->in_addr0, 4);
	b_align &= kdrv_sde_check_align(io_info->in_addr1, 4);
	b_align &= kdrv_sde_check_align(io_info->vol_addr, 4);
	b_align &= kdrv_sde_check_align(io_info->out_addr, 4);

	if (!b_align) {
		DBG_ERR("id %u input param error 0x%08x 0x%08x 0x%08x 0x%08x\r\n", id, io_info->in_addr0, io_info->in_addr1, io_info->vol_addr, io_info->out_addr);
		return E_PAR;
	}

	b_noncache_addr &= kdrv_sde_check_noncache_addr(io_info->in_addr0);
	b_noncache_addr &= kdrv_sde_check_noncache_addr(io_info->in_addr1);
	b_noncache_addr &= kdrv_sde_check_noncache_addr(io_info->vol_addr);
	b_noncache_addr &= kdrv_sde_check_noncache_addr(io_info->out_addr);

	if (!b_noncache_addr) {
		DBG_ERR("id %u input address error 0x%08x 0x%08x 0x%08x 0x%08x\r\n", id, io_info->in_addr0, io_info->in_addr1, io_info->vol_addr, io_info->out_addr);
		return E_PAR;
	}

	g_kdrv_sde_info[id].io_info.in_addr0 = io_info->in_addr0;
	g_kdrv_sde_info[id].io_info.in_addr1 = io_info->in_addr1;
	g_kdrv_sde_info[id].io_info.vol_addr = io_info->vol_addr;
	g_kdrv_sde_info[id].io_info.out_addr = io_info->out_addr;


	b_align &= kdrv_sde_check_align(io_info->in0_lofs, 4);
	b_align &= kdrv_sde_check_align(io_info->in1_lofs, 4);
	b_align &= kdrv_sde_check_align(io_info->vol_lofs, 4);
	b_align &= kdrv_sde_check_align(io_info->out_lofs, 4);

	if (!b_align) {
		DBG_ERR("id %u input lineoffset error 0x%08x 0x%08x 0x%08x 0x%08x\r\n", id, io_info->in0_lofs, io_info->in1_lofs, io_info->vol_lofs, io_info->out_lofs);
		return E_PAR;

	}
	g_kdrv_sde_info[id].io_info.in0_lofs = io_info->in0_lofs;
	g_kdrv_sde_info[id].io_info.in1_lofs = io_info->in1_lofs;
	g_kdrv_sde_info[id].io_info.vol_lofs = io_info->vol_lofs;
	g_kdrv_sde_info[id].io_info.out_lofs = io_info->out_lofs;

	return rt;
}

static ER kdrv_sde_set_isrcb(UINT32 id, void *data)
{
	KDRV_SDE_HANDLE *p_handle;

	if (data == NULL) {
		DBG_ERR("id %u input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	p_handle = kdrv_sde_entry_id_conv2_handle(id);
	p_handle->isrcb_fp = (KDRV_SDE_ISRCB)data;

	return E_OK;
}

static ER kdrv_sde_set_cost_param(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_SDE_COST_PARAM *cost_param;

	if (data == NULL) {
		DBG_ERR("id %u input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	cost_param = (KDRV_SDE_COST_PARAM *)data;

	g_kdrv_sde_info[id].cost_param.ad_bound_upper = cost_param->ad_bound_upper;
	g_kdrv_sde_info[id].cost_param.ad_bound_lower = cost_param->ad_bound_lower;
	g_kdrv_sde_info[id].cost_param.census_bound_upper = cost_param->census_bound_upper;
	g_kdrv_sde_info[id].cost_param.census_bound_lower = cost_param->census_bound_lower;
	g_kdrv_sde_info[id].cost_param.ad_census_ratio = cost_param->ad_census_ratio;

	return rt;
}

static ER kdrv_sde_set_luma_param(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_SDE_LUMA_PARAM *luma_param;

	if (data == NULL) {
		DBG_ERR("id %u input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	luma_param = (KDRV_SDE_LUMA_PARAM *)data;

	g_kdrv_sde_info[id].luma_param.luma_saturated_th = luma_param->luma_saturated_th;
	g_kdrv_sde_info[id].luma_param.cost_saturated = luma_param->cost_saturated;
	g_kdrv_sde_info[id].luma_param.delta_luma_smooth_th = luma_param->delta_luma_smooth_th;

	return rt;
}

static ER kdrv_sde_set_penalty_param(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_SDE_PENALTY_PARAM *penalty_param;

	if (data == NULL) {
		DBG_ERR("id %u input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	penalty_param = (KDRV_SDE_PENALTY_PARAM *)data;

	g_kdrv_sde_info[id].penalty_param.penalty_nonsmooth = penalty_param->penalty_nonsmooth;
	g_kdrv_sde_info[id].penalty_param.penalty_smooth0 = penalty_param->penalty_smooth0;
	g_kdrv_sde_info[id].penalty_param.penalty_smooth1 = penalty_param->penalty_smooth1;
	g_kdrv_sde_info[id].penalty_param.penalty_smooth2 = penalty_param->penalty_smooth2;

	return rt;
}

static ER kdrv_sde_set_smooth_param(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_SDE_SMOOTH_PARAM *smooth_param;

	if (data == NULL) {
		DBG_ERR("id %u input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	smooth_param = (KDRV_SDE_SMOOTH_PARAM *)data;

	g_kdrv_sde_info[id].smooth_param.delta_displut0 = smooth_param->delta_displut0;
	g_kdrv_sde_info[id].smooth_param.delta_displut1 = smooth_param->delta_displut1;

	return rt;
}

static ER kdrv_sde_set_lrc_param(UINT32 id, void *data)
{
	ER rt = E_OK;
	KDRV_SDE_LRC_THRESH_PARAM *lrc_param;

	if (data == NULL) {
		DBG_ERR("id %u input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	lrc_param = (KDRV_SDE_LRC_THRESH_PARAM *)data;

	g_kdrv_sde_info[id].lrc_th_param.thresh_lut0 = lrc_param->thresh_lut0;
	g_kdrv_sde_info[id].lrc_th_param.thresh_lut1 = lrc_param->thresh_lut1;
	g_kdrv_sde_info[id].lrc_th_param.thresh_lut2 = lrc_param->thresh_lut2;
	g_kdrv_sde_info[id].lrc_th_param.thresh_lut3 = lrc_param->thresh_lut3;
	g_kdrv_sde_info[id].lrc_th_param.thresh_lut4 = lrc_param->thresh_lut4;
	g_kdrv_sde_info[id].lrc_th_param.thresh_lut5 = lrc_param->thresh_lut5;
	g_kdrv_sde_info[id].lrc_th_param.thresh_lut6 = lrc_param->thresh_lut6;

	return rt;
}

KDRV_SDE_SET_FP kdrv_sde_set_fp[KDRV_SDE_PARAM_MAX] = {
	kdrv_sde_set_opencfg,

	kdrv_sde_set_modeinfo,
	kdrv_sde_set_io_info,

	kdrv_sde_set_isrcb,

	kdrv_sde_set_cost_param,
	kdrv_sde_set_luma_param,
	kdrv_sde_set_penalty_param,
	kdrv_sde_set_smooth_param,
	kdrv_sde_set_lrc_param

};


INT32 kdrv_sde_set(UINT32 id, KDRV_SDE_PARAM_ID param_id, void *p_param)
{
	ER rt = E_OK;
	KDRV_SDE_HANDLE *p_handle;
	UINT32 ign_chk;

	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);

	if (KDRV_DEV_ID_ENGINE(id) != KDRV_CV_ENGINE_SDE) {
		DBG_ERR("Invalid engine 0x%x\r\n", KDRV_DEV_ID_ENGINE(id));
		return E_ID;
	}

	ign_chk = (KDRV_SDE_IGN_CHK & param_id);
	param_id = param_id & (~KDRV_SDE_IGN_CHK);

	if ((g_kdrv_sde_open_times == 0) && (param_id != KDRV_SDE_PARAM_IPL_OPENCFG)) {
		DBG_ERR("KDRV SDE: engine is not opened. Only OPENCFG can be set. ID = %d\r\n", param_id);
		return E_SYS;
	} else if ((g_kdrv_sde_open_times > 0) && (param_id == KDRV_SDE_PARAM_IPL_OPENCFG)) {
		DBG_ERR("KDRV SDE: engine is opened. OPENCFG cannot be set.\r\n");
		return E_SYS;
	}

	if (g_kdrv_sde_init_flg == 0) {
		if (kdrv_sde_handle_alloc()) {
			DBG_WRN("KDRV SDE: no free handle, max handle num = %d\r\n", KDRV_SDE_HANDLE_MAX_NUM);
			return E_SYS;
		}
	}

	p_handle = &g_kdrv_sde_handle_tab[channel];
	if (kdrv_sde_chk_handle(p_handle) == NULL) {
		DBG_ERR("KDRV SDE: illegal handle 0x%.8x\r\n", (UINT32)&g_kdrv_sde_handle_tab[channel]);
		return E_SYS;
	}
;

	if ((p_handle->sts & KDRV_SDE_HANDLE_LOCK) != KDRV_SDE_HANDLE_LOCK) {
		DBG_ERR("KDRV SDE: illegal handle sts 0x%.8x\r\n", p_handle->sts);
		return E_SYS;
	}

	if (!ign_chk) {
		kdrv_sde_lock(p_handle, TRUE);
	}
	if (kdrv_sde_set_fp[param_id] != NULL) {
		rt = kdrv_sde_set_fp[param_id](channel, p_param);
	}
	if (!ign_chk) {
		kdrv_sde_lock(p_handle, FALSE);
	}

	return rt;
}

#if 0
#endif

static ER kdrv_sde_get_opencfg(UINT32 id, void *data)
{

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	*(KDRV_SDE_OPENCFG *)data = g_kdrv_sde_open_cfg;

	return E_OK;
}

static ER kdrv_sde_get_modeinfo(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	*(KDRV_SDE_MODE_INFO*)data = g_kdrv_sde_info[id].mode_info;

	return E_OK;
}

static ER kdrv_sde_get_io_info(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	*(KDRV_SDE_IO_INFO*)data = g_kdrv_sde_info[id].io_info;

	return E_OK;
}

static ER kdrv_sde_get_isrcb(UINT32 id, void *data)
{
	KDRV_SDE_HANDLE *p_handle;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	p_handle = kdrv_sde_entry_id_conv2_handle(id);
	*(KDRV_SDE_ISRCB *)data = p_handle->isrcb_fp;

	return E_OK;
}

static ER kdrv_sde_get_cost_param(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	*(KDRV_SDE_COST_PARAM*)data = g_kdrv_sde_info[id].cost_param;

	return E_OK;
}

static ER kdrv_sde_get_luma_param(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	*(KDRV_SDE_LUMA_PARAM*)data = g_kdrv_sde_info[id].luma_param;

	return E_OK;
}

static ER kdrv_sde_get_penalty_param(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	*(KDRV_SDE_PENALTY_PARAM*)data = g_kdrv_sde_info[id].penalty_param;

	return E_OK;
}

static ER kdrv_sde_get_smooth_param(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	*(KDRV_SDE_SMOOTH_PARAM*)data = g_kdrv_sde_info[id].smooth_param;

	return E_OK;
}

static ER kdrv_sde_get_lrc_param(UINT32 id, void *data)
{
	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", id, (UINT32)data);
		return E_PAR;
	}

	*(KDRV_SDE_LRC_THRESH_PARAM*)data = g_kdrv_sde_info[id].lrc_th_param;

	return E_OK;
}

KDRV_SDE_GET_FP kdrv_sde_get_fp[KDRV_SDE_PARAM_MAX] = {
	kdrv_sde_get_opencfg,

	kdrv_sde_get_modeinfo,
	kdrv_sde_get_io_info,

	kdrv_sde_get_isrcb,

	kdrv_sde_get_cost_param,
	kdrv_sde_get_luma_param,
	kdrv_sde_get_penalty_param,
	kdrv_sde_get_smooth_param,
	kdrv_sde_get_lrc_param

};

INT32 kdrv_sde_get(UINT32 id, KDRV_SDE_PARAM_ID parm_id, VOID *p_param)
{
	ER rt = E_OK;
	KDRV_SDE_HANDLE *p_handle;
	UINT32 ign_chk;
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);

	DBG_IND("id = 0x%x\r\n", id);
	if (KDRV_DEV_ID_ENGINE(id) != KDRV_CV_ENGINE_SDE) {
		DBG_ERR("Invalid engine 0x%x\r\n", KDRV_DEV_ID_ENGINE(id));
		return E_ID;
	}

	p_handle = &g_kdrv_sde_handle_tab[channel];
	if (kdrv_sde_chk_handle(p_handle) == NULL) {
		DBG_ERR("KDRV SDE: illegal handle 0x%.8x\r\n", (UINT32)&g_kdrv_sde_handle_tab[channel]);
		return E_SYS;
	}

	if ((p_handle->sts & KDRV_SDE_HANDLE_LOCK) != KDRV_SDE_HANDLE_LOCK) {
		DBG_ERR("KDRV SDE: illegal handle sts 0x%.8x\r\n", p_handle->sts);
		return E_SYS;
	}

	ign_chk = (KDRV_SDE_IGN_CHK & parm_id);
	parm_id = parm_id & (~KDRV_SDE_IGN_CHK);

	if (!ign_chk) {
		kdrv_sde_lock(p_handle, TRUE);
	}
	if (kdrv_sde_get_fp[parm_id] != NULL) {
		rt = kdrv_sde_get_fp[parm_id](channel, p_param);
	}
	if (!ign_chk) {
		kdrv_sde_lock(p_handle, FALSE);
	}

	return rt;
}
#if defined (__KERNEL__)
EXPORT_SYMBOL(kdrv_sde_open);
EXPORT_SYMBOL(kdrv_sde_close);
EXPORT_SYMBOL(kdrv_sde_trigger);
EXPORT_SYMBOL(kdrv_sde_set);
EXPORT_SYMBOL(kdrv_sde_get);
#endif



