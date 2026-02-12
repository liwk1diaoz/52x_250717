/**
    @file       kdrv_dis.c
    @ingroup    Predefined_group_name

    @brief      dis device abstraction layer

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#include "dis_platform.h"
#include "kdrv_dis_int.h"
#include "kdrv_eth.h"


#define DIS_MEASURE_TIME 0

KDRV_DIS_PRAM g_kdrv_dis_parm[KDRV_DIS_HANDLE_MAX_NUM] = {0};
static KDRV_DIS_HANDLE g_kdrv_dis_handle_tab[KDRV_DIS_HANDLE_MAX_NUM] = {0};
static UINT32 g_kdrv_dis_init_flg = 0;
static KDRV_DIS_HANDLE *gp_kdrv_dis_trig_hdl = 0;
static INT32 g_kdrv_dis_open_count = 0;
static DIS_OPENOBJ g_dis_open_cfg = {0};

extern DIS_BLKSZ DIS_APLY_BLOCK_SZ;

extern DIS_SUBIN DIS_APLY_SUBIN;

static UINT32 *gp_dis_flag_id = NULL;

static ER kdrv_dis_setmode(UINT32 id)
{
	ER rt;

	DIS_PARAM dis = {0};

	dis.uiWidth = g_kdrv_dis_parm[id].in_img_info.ui_width;
    dis.uiHeight= g_kdrv_dis_parm[id].in_img_info.ui_height;
    dis.uiInOfs = g_kdrv_dis_parm[id].in_img_info.ui_inofs;

    dis.uiInAdd0 = g_kdrv_dis_parm[id].in_dma_info.ui_inadd0;
    dis.uiInAdd1 = g_kdrv_dis_parm[id].in_dma_info.ui_inadd1;
    dis.uiInAdd2 = g_kdrv_dis_parm[id].in_dma_info.ui_inadd2;

    dis.uiOutAdd0 = g_kdrv_dis_parm[id].out_dma_info.ui_outadd0;
    dis.uiOutAdd1 = g_kdrv_dis_parm[id].out_dma_info.ui_outadd1;
    dis.uiIntEn   = g_kdrv_dis_parm[id].dis_int_en; //enable when process is unlocked

	rt = dis_setMode(&dis);
	return rt;
}

static ER kdrv_dis_changeSize(UINT32 id)
{
	ER rt;

	DIS_PARAM dis = {0};

	dis.uiWidth = g_kdrv_dis_parm[id].in_img_info.ui_width;
    dis.uiHeight= g_kdrv_dis_parm[id].in_img_info.ui_height;
    dis.uiInOfs = g_kdrv_dis_parm[id].in_img_info.ui_inofs;

	rt = dis_changeSize(&dis);
	return rt;
}


static void kdrv_dis_lock(KDRV_DIS_HANDLE *p_handle, BOOL flag)
{
	if (p_handle == NULL) {
		DBG_ERR("kdrv_dis_lock : p_handle is NULL!\r\n");
		return;
	}

	if (flag == TRUE) {
		FLGPTN flag_ptn;
		wai_flg(&flag_ptn, p_handle->flag_id, p_handle->lock_bit, TWF_CLR);
	} else {
		set_flg(p_handle->flag_id, p_handle->lock_bit);
	}
}

static void kdrv_dis_handle_lock(void)
{	
	FLGPTN wait_flg;	
	wai_flg(&wait_flg, kdrv_dis_get_flag_id(FLG_ID_KDRV_DIS), KDRV_DIS_HDL_UNLOCK, (TWF_ORW | TWF_CLR));
}

static void kdrv_dis_handle_unlock(void)
{
	set_flg(kdrv_dis_get_flag_id(FLG_ID_KDRV_DIS), KDRV_DIS_HDL_UNLOCK);
}

static INT32 kdrv_dis_handle_alloc(void)
{
	UINT32 i;

	kdrv_dis_handle_lock();
	for (i = 0; i < KDRV_DIS_HANDLE_MAX_NUM; i ++) {
		if (!(g_kdrv_dis_init_flg & (1 << i))) {
			g_kdrv_dis_init_flg |= (1 << i);
			memset(&g_kdrv_dis_handle_tab[i], 0, sizeof(KDRV_DIS_HANDLE));
			g_kdrv_dis_handle_tab[i].entry_id = i;
			g_kdrv_dis_handle_tab[i].flag_id = kdrv_dis_get_flag_id(FLG_ID_KDRV_DIS);
			g_kdrv_dis_handle_tab[i].lock_bit = (1 << i);
			g_kdrv_dis_handle_tab[i].sts |= KDRV_DIS_HANDLE_LOCK;
			g_kdrv_dis_handle_tab[i].p_sem_id = kdrv_dis_get_sem_id(SEMID_KDRV_DIS);
		}
	}
	kdrv_dis_handle_unlock();

	return E_OK;
}

static UINT32 kdrv_dis_handle_free(void)
{
	UINT32 rt = E_OBJ;
	int channel = 0;
	KDRV_DIS_HANDLE *p_handle;

	kdrv_dis_handle_lock();

	for (channel = 0; channel < KDRV_DIS_HANDLE_MAX_NUM; channel++) {
		p_handle = &g_kdrv_dis_handle_tab[channel];
		p_handle->sts = 0;
		g_kdrv_dis_init_flg &= ~(1 << channel);
	}

	if (g_kdrv_dis_init_flg == 0) {
		rt = E_OK;
	}

	kdrv_dis_handle_unlock();

	return rt;
}

static KDRV_DIS_HANDLE *kdrv_dis_entry_id_conv2_handle(UINT32 entry_id)
{
	return  &g_kdrv_dis_handle_tab[entry_id];
}

static void kdrv_dis_sem(KDRV_DIS_HANDLE *p_handle, BOOL flag)
{
	if (p_handle == NULL) {
		DBG_ERR("kdrv_dis_sem : p_handle is NULL!\r\n");
	}else {
		if (flag == TRUE) {
			SEM_WAIT(*p_handle->p_sem_id);    // wait semaphore
		} else {
			SEM_SIGNAL_ISR(*p_handle->p_sem_id);  // release semaphore
		}
	}
}

static void kdrv_dis_frm_end(KDRV_DIS_HANDLE *p_handle, BOOL flag)
{
	if (p_handle == NULL) {
		DBG_ERR("kdrv_dis_frm_end : p_handle is NULL!\r\n");
	}else {
		if (flag == TRUE) {
			iset_flg(p_handle->flag_id, KDRV_DIS_FMD | KDRV_DIS_TIMEOUT);
		} else {
			clr_flg(p_handle->flag_id, KDRV_DIS_FMD | KDRV_DIS_TIMEOUT);
		}
	}
}

static void kdrv_dis_clear_timeout_flag(KDRV_DIS_HANDLE* p_handle)
{	
	clr_flg(p_handle->flag_id, KDRV_DIS_TIMEOUT);
}

static void kdrv_dis_isr_cb(UINT32 intstatus)
{
	KDRV_DIS_HANDLE *p_handle;
	p_handle = gp_kdrv_dis_trig_hdl;

	if (intstatus & DIS_INT_FRM) {
		kdrv_dis_sem(p_handle, FALSE);
		kdrv_dis_frm_end(p_handle, TRUE);
	}

	if (p_handle->isrcb_fp != NULL) {
		p_handle->isrcb_fp((UINT32)p_handle, intstatus, NULL, NULL);
	}
}


void kdrv_dis_timeout_cb(UINT32 timer_id)
{
	set_flg(*gp_dis_flag_id, KDRV_DIS_TIMEOUT);
}


INT32 kdrv_dis_open(UINT32 chip, UINT32 engine)
{
	if (g_kdrv_dis_open_count < KDRV_DIS_HANDLE_MAX_NUM && g_kdrv_dis_open_count > 0) {
	} else if (g_kdrv_dis_open_count == 0) {
		INT32 i;
		DIS_OPENOBJ dis_drv_obj;

        #if defined(__FREERTOS)
		UINT32 dis_clk_freq;	

		dis_clk_freq = kdrv_dis_drv_get_clock_freq(0);
		dis_setBaseAddr(0xF0C50000);
		dis_create_resource(dis_clk_freq);

		kdrv_dis_install_id();
		kdrv_dis_init();
		#endif

		dis_drv_obj.uiDisClockSel = g_dis_open_cfg.uiDisClockSel;
		dis_drv_obj.FP_DISISR_CB = kdrv_dis_isr_cb;
		if (dis_open(&dis_drv_obj) != E_OK) {
			DBG_WRN("KDRV DIS: dis_open failed\r\n");
			return -1;
		}
		kdrv_dis_handle_alloc();
		for (i = 0; i < KDRV_DIS_HANDLE_MAX_NUM; i++) {
			kdrv_dis_lock(&g_kdrv_dis_handle_tab[i], TRUE);
			memset((void *)&g_kdrv_dis_parm[i], 0, sizeof(KDRV_DIS_PRAM));
			kdrv_dis_lock(&g_kdrv_dis_handle_tab[i], FALSE);
		}
	} else if (g_kdrv_dis_open_count > KDRV_DIS_HANDLE_MAX_NUM) {
		DBG_ERR("open too many times!\r\n");
		g_kdrv_dis_open_count = KDRV_DIS_HANDLE_MAX_NUM;
		return -1;
	}

	g_kdrv_dis_open_count++;


	return E_OK;
}
EXPORT_SYMBOL(kdrv_dis_open);

INT32 kdrv_dis_close(UINT32 chip, UINT32 engine)
{
	ER rt = E_OK;

	g_kdrv_dis_open_count--;

	if (g_kdrv_dis_open_count > 0) {
	} else if (g_kdrv_dis_open_count == 0) {
		INT32 i;
		DBG_IND("Count 0, close dis HW\r\n");
		if (dis_isEnabled()) {
			dis_pause();
		}
		rt = dis_close();

		for (i = 0; i < KDRV_DIS_HANDLE_MAX_NUM; i++) {
			kdrv_dis_lock(&g_kdrv_dis_handle_tab[i], TRUE);
			kdrv_dis_handle_free();
			kdrv_dis_lock(&g_kdrv_dis_handle_tab[i], FALSE);
		}
	} else if (g_kdrv_dis_open_count < 0) {
		DBG_ERR("close too many times!\r\n");
		g_kdrv_dis_open_count = 0;
		rt = E_PAR;
	}

#if defined(__FREERTOS)
	dis_release_resource();

	kdrv_dis_uninstall_id();

#endif

	return rt;
}
EXPORT_SYMBOL(kdrv_dis_close);

INT32 kdrv_dis_trigger(UINT32 id, KDRV_DIS_TRIGGER_PARAM *p_dis_param, VOID *p_cb_func, VOID *p_user_data)
{
	ER rt = E_OK;
	KDRV_DIS_HANDLE *p_handle;
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);
	
	p_handle = kdrv_dis_entry_id_conv2_handle(channel);
	kdrv_dis_sem(p_handle, TRUE);

	//set dis kdrv parameters to dis driver
	if (kdrv_dis_setmode(channel) != E_OK) {
		kdrv_dis_sem(p_handle, FALSE);
		return rt;
	}
	//update trig id and trig_cfg
	gp_kdrv_dis_trig_hdl = p_handle;

	//clear flag
	kdrv_dis_frm_end(p_handle, FALSE);
	kdrv_dis_clear_timeout_flag(p_handle);	
	dis_clrFrameEnd();

	//trigger dis start
	rt = dis_start();
	if (rt != E_OK) {
		kdrv_dis_frm_end(p_handle, FALSE);
		return rt;
	}

	DBG_IND("dis start\r\n");
	
	dis_waitFrameEnd(FALSE);
	dis_pause();
	
	return rt;
}
EXPORT_SYMBOL(kdrv_dis_trigger);

static ER kdrv_dis_set_opencfg(UINT32 id, void* p_data)
{
	KDRV_DIS_OPENCFG kdrv_dis_open_obj;

	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	kdrv_dis_open_obj = *(KDRV_DIS_OPENCFG *)p_data;
	g_dis_open_cfg.uiDisClockSel= kdrv_dis_open_obj.dis_clock_sel;

	return E_OK;
}

static ER kdrv_dis_set_in_img_info(UINT32 id, void* p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	g_kdrv_dis_parm[id].in_img_info= *(KDRV_DIS_IN_IMG_INFO*)p_data;

	if (g_kdrv_dis_parm[id].in_img_info.changesize_en) {
		kdrv_dis_changeSize(id);
	}
	return E_OK;
}

static ER kdrv_dis_set_dma_in(UINT32 id, void* p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	g_kdrv_dis_parm[id].in_dma_info= *(KDRV_DIS_IN_DMA_INFO*)p_data;

	if (g_kdrv_dis_parm[id].in_dma_info.ui_inadd0 == 0) {
		DBG_ERR("KDRV DIS:uiInAdd0 should not be zero\r\n");
		return E_PAR;
	}
	else if (g_kdrv_dis_parm[id].in_dma_info.ui_inadd1 == 0) {
		DBG_ERR("KDRV DIS:uiInAdd1 should not be zero\r\n");
		return E_PAR;
	}
	else if (g_kdrv_dis_parm[id].in_dma_info.ui_inadd2 == 0) {
		DBG_ERR("KDRV DIS:uiInAdd2 should not be zero\r\n");
		return E_PAR;
	}
	else if (g_kdrv_dis_parm[id].in_dma_info.ui_inadd0 & 0x3) {
		DBG_ERR("KDRV DIS: uiInAdd0  should be word alignment\r\n");
		return E_PAR;
	}
	else if (g_kdrv_dis_parm[id].in_dma_info.ui_inadd1 & 0x3) {
		DBG_ERR("KDRV DIS: uiInAdd1  should be word alignment\r\n");
		return E_PAR;
	}
	else if (g_kdrv_dis_parm[id].in_dma_info.ui_inadd2 & 0x3) {
		DBG_ERR("KDRV DIS: uiInAdd2  should be word alignment\r\n");
		return E_PAR;
	}

	return E_OK;
}

static ER kdrv_dis_set_inte_en(UINT32 id, void *p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	g_kdrv_dis_parm[id].dis_int_en = *(UINT32 *)p_data;
	return E_OK;
}

static ER kdrv_dis_set_dma_out(UINT32 id, void* p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	g_kdrv_dis_parm[id].out_dma_info= *(KDRV_DIS_OUT_DMA_INFO*)p_data;

	if (g_kdrv_dis_parm[id].out_dma_info.ui_outadd0 == 0) {
		DBG_ERR("KDRV DIS: uiOutAdd0 should not be zero\r\n");
		return E_PAR;
	}
	if (g_kdrv_dis_parm[id].out_dma_info.ui_outadd1 == 0) {
		DBG_ERR("KDRV DIS: uiOutAdd1 should not be zero\r\n");
		return E_PAR;
	}
	else if (g_kdrv_dis_parm[id].out_dma_info.ui_outadd0 & 0x3) {
		DBG_ERR("KDRV DIS: uiOutAdd0 should be word alignment\r\n");
		return E_PAR;
	}
	else if (g_kdrv_dis_parm[id].out_dma_info.ui_outadd1 & 0x3) {
		DBG_ERR("KDRV DIS: uiOutAdd1 should be word alignment\r\n");
		return E_PAR;
	}
	return E_OK;
}
static ER kdrv_dis_set_block_dim(UINT32 id, void *p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	DIS_APLY_BLOCK_SZ = *(DIS_BLKSZ *)p_data;
	return E_OK;
}

static ER kdrv_dis_set_sub_input(UINT32 id, void *p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}
	DIS_APLY_SUBIN = *(DIS_SUBIN *)p_data;
	return E_OK;
}
static ER kdrv_dis_set_isrcb(UINT32 id, void *p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	g_kdrv_dis_handle_tab[id].isrcb_fp = (KDRV_DIS_ISRCB)p_data;

	return E_OK;
}

KDRV_DIS_SET_FP kdrv_dis_set_fp[KDRV_DIS_PARAM_MAX] = {
	kdrv_dis_set_opencfg,	
	kdrv_dis_set_in_img_info,
	kdrv_dis_set_dma_in,
	kdrv_dis_set_inte_en,
	kdrv_dis_set_dma_out,
	NULL,
	NULL,
	kdrv_dis_set_block_dim,
	kdrv_dis_set_sub_input,
	kdrv_dis_eth_set_in_param_info,
	kdrv_dis_eth_set_dma_out,
	NULL,
	kdrv_dis_set_isrcb,
};
INT32 kdrv_dis_set(UINT32 id, KDRV_DIS_PARAM_ID parm_id, VOID *p_param)
{
	ER rt = E_OK;
	//UINT32 ign_chk;
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);
	if (p_param == NULL) {
		DBG_ERR("KDRV DIS: id %d p_param NULL!\r\n", id);
		return E_PAR;
	}

	//ign_chk = (KDRV_DIS_IGN_CHK & parm_id);
	parm_id = parm_id & (~KDRV_DIS_IGN_CHK);

	if (kdrv_dis_set_fp[parm_id] != NULL) {
		rt = kdrv_dis_set_fp[parm_id](channel, p_param);
	}

	return rt;
}
EXPORT_SYMBOL(kdrv_dis_set);

static ER kdrv_dis_get_in_img_info(UINT32 id, void* p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	*(KDRV_DIS_IN_IMG_INFO*)p_data = g_kdrv_dis_parm[id].in_img_info;
	return E_OK;
}

static ER kdrv_dis_get_dma_in(UINT32 id, void* p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	*(KDRV_DIS_IN_DMA_INFO*)p_data = g_kdrv_dis_parm[id].in_dma_info;
	return E_OK;
}

static ER kdrv_dis_get_inte_en(UINT32 id, void *p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	*(UINT32 *)p_data = g_kdrv_dis_parm[id].dis_int_en;
	return E_OK;
}


static ER kdrv_dis_get_dma_out(UINT32 id, void* p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	*(KDRV_DIS_OUT_DMA_INFO*)p_data = g_kdrv_dis_parm[id].out_dma_info;
	return E_OK;
}

static ER kdrv_dis_get_mv_out(UINT32 id, void* p_data)
{
	static MOTION_INFOR *mv;
	static KDRV_MV_OUT_DMA_INFO *motresult;
#if DIS_IOREMAP_IN_KERNEL
	UINT32 motresult_addr_va = 0, motresult_addr_pa = 0;
#endif
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}
	motresult = (KDRV_MV_OUT_DMA_INFO*)p_data;
#if DIS_IOREMAP_IN_KERNEL
	motresult_addr_pa = (UINT32)motresult->p_mvaddr;
	motresult_addr_va = dis_platform_pa2va_remap(motresult_addr_pa, MVNUMMAX * sizeof(MOTION_INFOR), 0);
	mv = (MOTION_INFOR*)motresult_addr_va;
#else
	mv = (MOTION_INFOR*)motresult->p_mvaddr;
#endif

	if ((UINT32)motresult->p_mvaddr== 0) {
		DBG_ERR("KDRV DIS: MV address should not be zero\r\n");
		return E_PAR;
	}
	else if ((UINT32)motresult->p_mvaddr & 0x3) {
		DBG_ERR("KDRV DIS:MV address should be word alignment\r\n");
		return E_PAR;
	}

	dis_getMotionVectors( mv);
	
#if DIS_IOREMAP_IN_KERNEL
	dis_platform_pa2va_unmap(motresult_addr_va, motresult_addr_pa);
#endif
	return E_OK;
}
static ER kdrv_dis_get_mv_dim(UINT32 id, void* p_data)
{
	static MDS_DIM mds_dim;
	static KDRV_MDS_DIM* kdrv_mds_dim;
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}
	kdrv_mds_dim = (KDRV_MDS_DIM*)p_data;
	mds_dim = dis_getMDSDim();
	kdrv_mds_dim->ui_blknum_h = mds_dim.uiBlkNumH;
	kdrv_mds_dim->ui_mdsnum   = mds_dim.uiMdsNum;
	kdrv_mds_dim->ui_blknum_v = mds_dim.uiBlkNumV;
	
	return E_OK;
}

static ER kdrv_dis_get_block_dim(UINT32 id, void* p_data)
{
	static KDRV_BLOCKS_DIM *kdrv_blocks_dim;
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}
	kdrv_blocks_dim  = (KDRV_BLOCKS_DIM*)p_data;
	*kdrv_blocks_dim = (KDRV_BLOCKS_DIM)DIS_APLY_BLOCK_SZ;	
	return E_OK;
}
static ER kdrv_dis_get_sub_input(UINT32 id, void *p_data)
{
    static KDRV_DIS_SUBIN *kdrv_sub_in;
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}
    kdrv_sub_in  = (KDRV_DIS_SUBIN*) p_data;
	*kdrv_sub_in = (KDRV_DIS_SUBIN)DIS_APLY_SUBIN;
	return E_OK;
}

static ER kdrv_dis_get_isr_cb(UINT32 id, void* p_data)
{
	if (p_data == NULL) {
		DBG_ERR("KDRV DIS: id %d input data NULL!\r\n", id);
		return E_PAR;
	}

	*(KDRV_DIS_ISRCB*)p_data = g_kdrv_dis_handle_tab[id].isrcb_fp;
	return E_OK;
}

KDRV_DIS_GET_FP kdrv_dis_get_fp[KDRV_DIS_PARAM_MAX] = {
	NULL,
	kdrv_dis_get_in_img_info,
	kdrv_dis_get_dma_in,
	kdrv_dis_get_inte_en,
	kdrv_dis_get_dma_out,
	kdrv_dis_get_mv_out,
	kdrv_dis_get_mv_dim,
	kdrv_dis_get_block_dim,
	kdrv_dis_get_sub_input,
	kdrv_dis_eth_get_in_param_info,
	kdrv_dis_eth_get_dma_out,
	kdrv_dis_eth_get_out_info,
	kdrv_dis_get_isr_cb,
} ;


INT32 kdrv_dis_get(UINT32 id, KDRV_DIS_PARAM_ID parm_id, VOID *p_param)
{
	ER rt = E_OK;
	//UINT32 ign_chk;

	if (p_param == NULL) {
		DBG_ERR("KDRV DIS: id %d p_param NULL!\r\n", id);
		return E_PAR;
	}

	//ign_chk = (KDRV_DIS_IGN_CHK & parm_id);
	parm_id = parm_id & (~KDRV_DIS_IGN_CHK);
	if (kdrv_dis_get_fp[parm_id] != NULL) {
	    rt = kdrv_dis_get_fp[parm_id](KDRV_DEV_ID_CHANNEL(id), p_param);
	}
	return rt;
}
EXPORT_SYMBOL(kdrv_dis_get);
