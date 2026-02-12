/**
    DIS Process API

    This file contains the FW control of DIS process.

    @file       dis_alg_fw.c
    @ingroup    mISYSAlg
    @note       N/A

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/


/** \addtogroup mISYSAlg */
//@{

#include "dis_alg_fw.h"
#include "dis_alg_hw.h"

UINT32                 g_cur_framecnt_fw = 0;
static UINT32          g_ipe_out_hsize_fw = 1920, g_ipe_out_vsize_fw = 1080;
DIS_MOTION_VECTOR      g_imv;
DIS_IPC_INIT           g_dis_mv_addr;

UINT16                 g_dis_new_view_ratio = 900; //IPL input ratio=90% //1100; //New size = original*g_dis_new_view_ratio/1000
DISALG_MOTION_VECTOR   g_ide_corr;
DISALG_VECTOR          g_frame_corr[10];
UINT32                 g_frame_corr_cnt = 0;
DISALG_MOTION_VECTOR   /*g_ide_center,*/ g_ide_vec_bef_lock;
volatile BOOL          g_bLock_match = FALSE;

//OIS
UINT32                 g_uidis_Isois_on = FALSE;
INT32                  g_idis_ois_x = 0, g_idis_ois_y = 0;
//DIS
DIS_MOTION_INFOR       g_mv, g_omv, g_cmv1, g_tmv1, g_pmv1[DIS_TBUFNUM];
UINT32                 g_ui_cur_cr_x, g_ui_cur_cr_y;
DIS_SGM                g_s_info;
DIS_IPC_INIT           g_blkmv_addr;
DIS_MOTION_INFOR       *g_blkmv = 0;
MOTION_VECTOR_QUEUE    g_last_motvect1;
UINT32                 g_uicand[DIS_MVNUMMAX] = {0};
UINT32                 g_uicand_roi[DIS_MVNUMMAX] = {0};

extern UINT16 g_dis_sub_sel;
extern UINT16 g_dis_eth_sub_sel;

extern UINT32 g_dis_sensor_w;
extern UINT32 g_dis_sensor_h;


static void disFw_setLockMatch(void);
static void disFw_resetCorrVec(void);

static VOID dis_doIntegMotiVectComp(DIS_GM gm_info);
static VOID dis_getIntMV(DIS_GM gm_info, DIS_SGM mv);
static VOID dis_getDisDetMotiVect(DIS_GM gm_info, DIS_SGM mv);
static VOID dis_compensation(DIS_GM gm_info, DIS_SGM mv);
static VOID dis_initAlgCal(VOID);


/**
    DIS: Reset correction vectors
*/
void disFw_resetCorrVec(void)
{
	int ip;
	
	g_imv.ix = 0;
	g_imv.iy = 0;
	
	for (ip = 0; ip < 10; ip++) {
		g_frame_corr[ip].vector.ix = 0;
		g_frame_corr[ip].vector.iy = 0;
		g_frame_corr[ip].score = 0;
	}
}

void disfw_set_dis_infor(DIS_PARAM *p_disinfo)
{
	g_ipe_out_hsize_fw = p_disinfo->in_size_h;
	if ((g_ipe_out_hsize_fw & 0x3) != 0) {
		DBG_ERR("err: Input hsize should be 4x!!\r\n");
	}
	g_ipe_out_vsize_fw = p_disinfo->in_size_v;
	g_cur_framecnt_fw = p_disinfo->frame_cnt;

}

void disfw_accum_update_process(DIS_MOTION_INFOR *p_mv)
{
	COMPENSATION_INFO comp_info;
	DISALG_MOTION_VECTOR bound;
	DIS_GM dgm_info;

	bound.ix = (g_dis_sensor_w* (1000 - g_dis_new_view_ratio) / 2) / 1000;
	bound.iy = (g_dis_sensor_h* (1000 - g_dis_new_view_ratio) / 2) / 1000;
	//bound.ix = g_ipe_out_hsize_fw * (g_dis_new_view_ratio - 100) / (2 * g_dis_new_view_ratio);
	//bound.iy = g_ipe_out_vsize_fw * (g_dis_new_view_ratio - 100) / (2 * g_dis_new_view_ratio);
	DBG_IND("g_dis_new_view_ratio = %d \n\r", g_dis_new_view_ratio);
    DBG_IND("g_ipe_out_fw = <%d %d>\n\r", g_ipe_out_hsize_fw, g_ipe_out_vsize_fw);
	DBG_IND("bound = <%d %d>\n\r", bound.ix, bound.iy);

	comp_info.ui_bound_h    = bound.ix;
	comp_info.ui_bound_v    = bound.iy;
	comp_info.ui_dzoom_rate = 1;

	dgm_info.p_comp   = &comp_info;
	dgm_info.p_imv    = &g_imv;
	dgm_info.score_lv = DIS_MVSCORE_NORMAL;
	dgm_info.sticky_lv= DIS_STICKY_TOP;
	dgm_info.p_mv     = p_mv;
	dis_doIntegMotiVectComp(dgm_info);

	if ((dis_get_cur_dis_inh() != g_ipe_out_hsize_fw) || (dis_get_cur_dis_inv() != g_ipe_out_vsize_fw)) {
		DBG_ERR("^G%d,%d,%d,%d\r\n", dis_get_cur_dis_inh(), dis_get_cur_dis_inv(), g_ipe_out_hsize_fw, g_ipe_out_vsize_fw);

		//#NT#Avoid transition and offset during Dzoom
		disFw_setLockMatch();
		//Reconfig DIS engine & do not update g_ide_corr
		dishw_chg_dis_sizeconfig();
	} else {
		//Check if g_imv exceeds compensation range
		g_imv.ix =  g_imv.ix * g_dis_eth_sub_sel * g_dis_sub_sel;
        g_imv.iy =  g_imv.iy * g_dis_eth_sub_sel * g_dis_sub_sel;
		if (g_imv.ix < (INT32)(1 - bound.ix)) {
			g_imv.ix = (INT32)(1 - bound.ix);
		}
		if (g_imv.ix > (INT32)(bound.ix - 1)) {
			g_imv.ix = (INT32)(bound.ix - 1);
		}
		if (g_imv.iy < (INT32)(1 - bound.iy)) {
			g_imv.iy = (INT32)(1 - bound.iy);
		}
		if (g_imv.iy > (INT32)(bound.iy - 1)) {
			g_imv.iy = (INT32)(bound.iy - 1);
		}
		g_ide_corr.ix = (-g_imv.ix);
		g_ide_corr.iy = (-g_imv.iy);

	}

	g_frame_corr[g_frame_corr_cnt].vector.ix  = g_ide_corr.ix;
	g_frame_corr[g_frame_corr_cnt].vector.iy  = g_ide_corr.iy;
	g_frame_corr[g_frame_corr_cnt].frame_cnt = g_cur_framecnt_fw;
	g_frame_corr_cnt++;
	if (g_frame_corr_cnt >= 10) {
		g_frame_corr_cnt = 0;
	}
	DBG_IND("g_frame_corr_cnt = %d, g_cur_framecnt_fw = %d, g_ide_corr = <%d %d>  \r\n",g_frame_corr_cnt,g_cur_framecnt_fw,g_ide_corr.ix,g_ide_corr.iy);
	DBG_IND("g_imv <%d %d> ,bound <%d %d>\r\n",g_imv.ix, g_imv.iy, bound.ix, bound.iy);
}


UINT32 disfw_get_prv_maxbuffer(void)
{
	UINT32 out_bufsz;

	out_bufsz = DIS_MVNUMMAX * sizeof(DIS_MOTION_INFOR)*2;

	return out_bufsz;
}

void disFw_BufferInit(DIS_IPC_INIT *p_buf)
{
	g_dis_mv_addr.addr= p_buf->addr;
    g_blkmv_addr.addr = g_dis_mv_addr.addr + DIS_MVNUMMAX * sizeof(DIS_MOTION_INFOR);
    g_blkmv = (DIS_MOTION_INFOR *)g_blkmv_addr.addr;
	p_buf->addr += disfw_get_prv_maxbuffer();
	p_buf->size -= disfw_get_prv_maxbuffer();

}

UINT16 dis_get_dis_viewratio(void)
{
	//DBG_ERR("dis_get_dis_viewratio %d\r\n", g_dis_new_view_ratio);
	return g_dis_new_view_ratio;
}

void dis_set_dis_viewratio(UINT16 ratio)
{
	g_dis_new_view_ratio = ratio;
}

#if 0
/**
    Get current frame motion vector
    @return    motion vector
*/
DISALG_MOTION_VECTOR dis_getIdeCorrVec(void)
{
	return g_ide_corr;
}
#endif

/**
    Get specific frame motion vector
    @param[in] frame_cnt frame count number
    @return    motion vector
*/
ER dis_get_frame_corrvec(DISALG_VECTOR *p_corr, UINT32 frame_cnt)
{
	UINT32 cnt = 0;
	UINT32 min_diff = 0;

	p_corr->vector.ix  = ((g_frame_corr[0].vector.ix) << DISLIB_VECTOR_NORM) / (INT32)g_dis_sensor_w;
	p_corr->vector.iy  = ((g_frame_corr[0].vector.iy) << DISLIB_VECTOR_NORM) / (INT32)g_dis_sensor_h;
	p_corr->frame_cnt  = g_frame_corr[0].frame_cnt;
	if (g_frame_corr[0].frame_cnt == frame_cnt) {
		p_corr->score = 1;
		return E_OK;
	} else {
		p_corr->score = 0;
		min_diff = frame_cnt - g_frame_corr[0].frame_cnt;
	}

	for (cnt = 1; cnt < 10; cnt++) {
		if (g_frame_corr[cnt].frame_cnt == frame_cnt) {
			p_corr->vector.ix  = ((g_frame_corr[cnt].vector.ix) << DISLIB_VECTOR_NORM) / (INT32)g_dis_sensor_w;
			p_corr->vector.iy  = ((g_frame_corr[cnt].vector.iy) << DISLIB_VECTOR_NORM) / (INT32)g_dis_sensor_h;
			p_corr->frame_cnt  = g_frame_corr[cnt].frame_cnt;
			p_corr->score = 1;
			break;
		} else {
			if ((frame_cnt - g_frame_corr[cnt].frame_cnt) < min_diff) {
				min_diff = (frame_cnt - g_frame_corr[cnt].frame_cnt);
				p_corr->vector.ix  = ((g_frame_corr[cnt].vector.ix) << DISLIB_VECTOR_NORM) / (INT32)g_dis_sensor_w;
				p_corr->vector.iy  = ((g_frame_corr[cnt].vector.iy) << DISLIB_VECTOR_NORM) / (INT32)g_dis_sensor_h;
				p_corr->frame_cnt  = g_frame_corr[cnt].frame_cnt;
				p_corr->score = 0;
			}
		}
	}
	return E_OK;
}

/**
    Get specific frame motion vector in pixels
    @param[in] frame_cnt frame count number
    @return    motion vector
*/
ER dis_get_frame_corrvec_pxl(DISALG_VECTOR *p_corr, UINT32 frame_cnt)
{
	UINT32 cnt = 0;
	UINT32 min_diff = 0;

	//Corr->vector.ix  = (g_frame_corr[0].vector.ix << DISLIB_VECTOR_NORM) / (INT32)gEthOutHsize;
	//Corr->vector.iy  = (g_frame_corr[0].vector.iy << DISLIB_VECTOR_NORM) / (INT32)gEthOutVsize;
	p_corr->vector.ix  = g_frame_corr[0].vector.ix;
	p_corr->vector.iy  = g_frame_corr[0].vector.iy;
	p_corr->frame_cnt  = g_frame_corr[0].frame_cnt;
	if (g_frame_corr[0].frame_cnt == frame_cnt) {
		p_corr->score = 1;
		return E_OK;
	} else {
		p_corr->score = 0;
		min_diff = frame_cnt - g_frame_corr[0].frame_cnt;
	}

	for (cnt = 1; cnt < 10; cnt++) {
		if (g_frame_corr[cnt].frame_cnt == frame_cnt) {
			p_corr->vector.ix  = g_frame_corr[cnt].vector.ix;
			p_corr->vector.iy  = g_frame_corr[cnt].vector.iy;
			p_corr->frame_cnt  = g_frame_corr[cnt].frame_cnt;
			p_corr->score = 1;
			break;
		} else {
			if ((frame_cnt - g_frame_corr[cnt].frame_cnt) < min_diff) {
				min_diff = (frame_cnt - g_frame_corr[cnt].frame_cnt);
				p_corr->vector.ix  = g_frame_corr[cnt].vector.ix;
				p_corr->vector.iy  = g_frame_corr[cnt].vector.iy;
				p_corr->frame_cnt  = g_frame_corr[cnt].frame_cnt;
				p_corr->score = 0;
			}
		}
	}
	return E_OK;
}
//DIS_PROC_EVENT dis_getProcEvent(void)
//{
//    return gDisProcEvent;
//}

static void disFw_setLockMatch(void)
{
	g_ide_vec_bef_lock.ix = g_ide_corr.ix;
	g_ide_vec_bef_lock.iy = g_ide_corr.iy;
	g_bLock_match = TRUE;
}
////////////////////////////////////////////////////////////////////////////////

UINT32 dis_access_ois_on_off(DIS_ACCESS_TYPE acc_type, UINT32 ui_isois_on)
{
	UINT32 retV = ui_isois_on;
	DBG_MSG("AccOis:%d,onOff=%ld\r\n", acc_type, ui_isois_on);

	if (DIS_ACCESS_SET == acc_type) {
		g_uidis_Isois_on = ui_isois_on;
	} else if (DIS_ACCESS_GET) {
		retV = g_uidis_Isois_on;
	} else {
		DBG_ERR("Unkown accType=%d\r\n", acc_type);
		retV = DIS_ACCESS_MAX;
	}
	return retV;
}

UINT32 dis_set_ois_det_motvec(INT32 ix, INT32 iy)
{
	DBG_MSG("SetOis x,y=%ld,%ld\r\n", ix, iy);
	if (FALSE == g_uidis_Isois_on) {
		DBG_ERR("OIS is OFF\r\n");
		return FALSE;
	}
	g_idis_ois_x = ix;
	g_idis_ois_y = iy;
	return TRUE;
}


VOID dis_get_ois_det_motvec(DIS_SGM *p_iv)
{
	DBG_IND("OisOn:X=%ld,Y=%ld\r\n", g_idis_ois_x, g_idis_ois_y);
	p_iv->p_cur_mv->ix = g_idis_ois_x;
	p_iv->p_cur_mv->iy = g_idis_ois_y;
}

VOID dis_doIntegMotiVectComp(DIS_GM gm_info)
{
	UINT32 idx;

	for (idx = 0; idx < DIS_MVNUMMAX; idx++) {
		g_blkmv[idx].bvalid = gm_info.p_mv[idx].bvalid;
		g_blkmv[idx].ix     = gm_info.p_mv[idx].ix;
		g_blkmv[idx].iy     = gm_info.p_mv[idx].iy;
		g_blkmv[idx].ui_cnt  = gm_info.p_mv[idx].ui_cnt;
 		g_blkmv[idx].ui_idx  = gm_info.p_mv[idx].ui_idx;
 		g_blkmv[idx].ui_sad  = gm_info.p_mv[idx].ui_sad;
	}

	g_s_info.p_cur_mv = &g_cmv1;
	g_s_info.p_tmp_mv = &g_tmv1;
	g_s_info.p_pre_mv = g_pmv1;
	g_s_info.p_last_motvect_que = &g_last_motvect1;
	g_s_info.p_blk_mv = g_blkmv;

	dis_getIntMV(gm_info, g_s_info);
	dis_compensation(gm_info, g_s_info);
#if 0
	//DbgMsg_Dis(("CurMV-XY=%d,%d;TmpMV-XY=%d,%d;BlkMV-XY=%d,%d;PrvMV-XY=%d,%d;IntgMV-XY=%d,%d\r\n",
	//    g_s_info.pCurmv->x,g_s_info.pCurmv->y,g_s_info.pTmpmv->x, g_s_info.pTmpmv->y,g_s_info.pBlkmv->x, g_s_info.pBlkmv->y,
	//    g_s_info.pPremv->x,g_s_info.pPremv->y,gmInfo.g_imv->x, gmInfo.g_imv->y));
#endif
}

static VOID dis_getIntMV(DIS_GM gm_info, DIS_SGM mv)
{
	//DBG_MSG("+-%s:disOn=%d\r\n",__func__,g_uidis_Isois_on);

	if (TRUE == g_uidis_Isois_on) {
		//OIS
		dis_get_ois_det_motvec(&mv);
	} else {
		//DIS
		//dis_getMotionResults(dis_mv);
		//dis_getMotionVectors(dis_mv);
		dis_getDisDetMotiVect(gm_info, mv);

		// tracking test
		//dis_getTrkMotionVec(dis_mv, trk_mv);
	}

}
static VOID dis_getDisDetMotiVect(DIS_GM gm_info, DIS_SGM mv)
{
	UINT32 ip, index, num_cand;
	INT32 cattenu, mvmax;
	UINT32 ui_score_min;
	DIS_MDS_DIM mds_dim;
	UINT32 ttl_mvnum, /*nth,*/ nth_min;
	INT32 mean_x[5], mean_y[5];
	UINT32 ttl_candnum, vect_rngth;

	cattenu = DIS_VECTOR_PERCENT;
	mds_dim = dis_get_mds_dim();
	ttl_mvnum =  mds_dim.ui_mdsnum * mds_dim.ui_blknum_h * mds_dim.ui_blknum_v;

	nth_min = DIS_NUM_CAND_FILTER;
	if (nth_min > ttl_mvnum) {
		nth_min = ttl_mvnum;
	}

	//nth = ttl_mvnum * DIS_QUALITY_CAND_PERCENT / 100;
	//if(nth < nth_min)
	//{
	//    nth = nth_min;
	//}

	//DBG_MSG("+-%s\r\n",__func__);

	switch (gm_info.score_lv) {
	case DIS_MVSCORE_HIGH:
		ui_score_min = 1600;
		break;
	case DIS_MVSCORE_NORMAL:
		ui_score_min = 1400;
		break;
	case DIS_MVSCORE_LOW:
		ui_score_min = 1200;
		break;
	case DIS_MVSCORE_LOW2:
		ui_score_min = 1000;
		break;
	default:
		ui_score_min = 1400;
		break;

	}
	switch (gm_info.sticky_lv) {
	case DIS_STICKY_TOP:
		mvmax = 31;
		break;
	case DIS_STICKY_HIGH:
		mvmax = 22;
		break;
	case DIS_STICKY_NORMAL:
		mvmax = 16;
		break;
	case DIS_STICKY_LOW:
		mvmax = 8;
		break;
	case DIS_STICKY_LOW2:
		mvmax = 4;
		break;
	default:
		mvmax = 16;
		break;
	}

	//pick out candidates

	for (ip = 0; ip < ttl_mvnum; ip++) {
		g_uicand[ip] = ip;
	}

	ttl_candnum = ttl_mvnum;
	vect_rngth = 16;
	for (index = 0; index < 5; index++) {
		mean_x[index] = 0;
		mean_y[index] = 0;
		num_cand = 0;
		for (ip = 0; ip < ttl_candnum; ip++) {
			if ((mv.p_blk_mv[g_uicand[ip]].ui_sad > ui_score_min) && (mv.p_blk_mv[g_uicand[ip]].bvalid)) {
				g_uicand[num_cand] = g_uicand[ip];
				mean_x[index] += mv.p_blk_mv[g_uicand[ip]].ix;
				mean_y[index] += mv.p_blk_mv[g_uicand[ip]].iy;
				num_cand++;
			}
		}
		ttl_candnum = num_cand;
		if (num_cand) {
			mean_x[index] = mean_x[index] / (INT32)num_cand;
			mean_y[index] = mean_y[index] / (INT32)num_cand;
		}
		if (vect_rngth) {
			for (ip = 0; ip < ttl_candnum; ip++) {
				if (((UINT32)DIS_ABS(mv.p_blk_mv[g_uicand[ip]].ix - mean_x[index]) > vect_rngth) ||
					((UINT32)DIS_ABS(mv.p_blk_mv[g_uicand[ip]].iy - mean_y[index]) > vect_rngth)) {
					mv.p_blk_mv[g_uicand[ip]].bvalid = 0;
				}
			}
		}
		vect_rngth = vect_rngth / 2;
	}

	if (num_cand > nth_min) {
		if ((DIS_ABS(mean_x[4]) > mvmax) || (DIS_ABS(mean_y[4]) > mvmax)) {
			mv.p_cur_mv->ix = mv.p_cur_mv->ix * cattenu / 100;
			mv.p_cur_mv->iy = mv.p_cur_mv->iy * cattenu / 100;
		} else {
			mv.p_cur_mv->ix = mean_x[4];
			mv.p_cur_mv->iy = mean_y[4];
		}
	} else {
		mv.p_cur_mv->ix = mv.p_cur_mv->ix * cattenu / 100;
		mv.p_cur_mv->iy = mv.p_cur_mv->iy * cattenu / 100;
	}

	DBG_IND("CurMV x=%d y=%d,hori=%d,ver=%d\r\n", mv.p_cur_mv->ix, mv.p_cur_mv->iy, gm_info.p_comp->ui_bound_h,gm_info.p_comp->ui_bound_v);
}

static VOID dis_compensation(DIS_GM gm_info, DIS_SGM mv)
{
	UINT32 ip;
	INT32 tweight, tweight2, tweight3;
	INT32 bound_x, bound_y;

	bound_x = 216;
	bound_y = bound_x* (INT32)g_dis_sensor_h / (INT32)g_dis_sensor_w;

	//DBG_MSG("+-%s:curMV-XY=%d,%d\r\n",__func__,mv.pCurmv->ix,mv.pCurmv->iy);


	tweight  = 98;//DIS_TEMP_FILTER_WEIGHT;
	tweight2 = 95;
	tweight3 = 90;

	g_mv.ix = mv.p_cur_mv->ix;
	g_mv.iy = mv.p_cur_mv->iy;
	//x direction
	for (ip = (DIS_TBUFNUM - 1); ip > 0; ip--) {
		mv.p_pre_mv[ip].ix = mv.p_pre_mv[ip - 1].ix;
	}
	if (DIS_ABS(mv.p_pre_mv[0].ix) < 50) {
		mv.p_pre_mv[0].ix = (mv.p_pre_mv[1].ix * tweight  / 100) + g_mv.ix;
	} else if (DIS_ABS(mv.p_pre_mv[0].ix) < 100) {
		mv.p_pre_mv[0].ix = (mv.p_pre_mv[1].ix * tweight2 / 100) + g_mv.ix;
	} else {
		mv.p_pre_mv[0].ix = (mv.p_pre_mv[1].ix * tweight3 / 100) + g_mv.ix;
	}

	//y direction
	for (ip = (DIS_TBUFNUM - 1); ip > 0; ip--) {
		mv.p_pre_mv[ip].iy = mv.p_pre_mv[ip - 1].iy;
	}
	if (DIS_ABS(mv.p_pre_mv[0].iy) < 50) {
		mv.p_pre_mv[0].iy = (mv.p_pre_mv[1].iy * tweight  / 100) + g_mv.iy;
	} else if (DIS_ABS(mv.p_pre_mv[0].iy) < 100) {
		mv.p_pre_mv[0].iy = (mv.p_pre_mv[1].iy * tweight2 / 100) + g_mv.iy;
	} else {
		mv.p_pre_mv[0].iy = (mv.p_pre_mv[1].iy * tweight3 / 100) + g_mv.iy;
	}

	if (mv.p_pre_mv[0].ix > bound_x) {
		mv.p_pre_mv[0].ix = bound_x;
	} else if (mv.p_pre_mv[0].ix < -bound_x) {
		mv.p_pre_mv[0].ix = -bound_x;
	}

	if (mv.p_pre_mv[0].iy > bound_y) {
		mv.p_pre_mv[0].iy = bound_y;
	} else if (mv.p_pre_mv[0].iy < -bound_y) {
		mv.p_pre_mv[0].iy = -bound_y;
	}

#if 0//(DIS_TEMP_FILTER_EN == ENABLE)
	sum = 0;
	for (ip = 0; ip < (DIS_TBUFNUM); ip++) {
		sum += iv.pPremv[ip].x;
	}
	//When and where to set iv.pTmpmv->x and iv.pTmpmv->y??
	DbgMsg_DisIO(("g_s_info.pTmpmv->x=%d,y=%d\r\n", iv.pTmpmv->x, iv.pTmpmv->y));
	iv.pTmpmv->x = (tweight * iv.pTmpmv->x + (100 - tweight) * (sum / DIS_TBUFNUM)) / 100;
	sum = 0;
	for (ip = 0; ip < (DIS_TBUFNUM); ip++) {
		sum += iv.pPremv[ip].y;
	}
	iv.pTmpmv->y = (tweight * iv.pTmpmv->y + (100 - tweight) * (sum / DIS_TBUFNUM)) / 100;

	//calculate integral
	iv.pPremv[0].x = iv.pPremv[0].x - iv.pTmpmv->x;
	iv.pPremv[0].y = iv.pPremv[0].y - iv.pTmpmv->y;

#endif
	gm_info.p_imv->ix = mv.p_pre_mv[0].ix;
	gm_info.p_imv->iy = mv.p_pre_mv[0].iy;


	DBG_IND("IntegralX,Y=%d%d\r\n", gm_info.p_imv->ix,gm_info.p_imv->iy);

}

/**
    DIS Reset DIS Algorithm Calculation Vectors

    Reset DIS algorithm calculation vectors

    @return None.
*/
VOID dis_initAlgCal(VOID)
{
	int ip;

	//g_uiDisFcnt=0;
	g_mv.ix = 0;
	g_mv.iy = 0;
	g_mv.ui_sad = 0;
	g_omv.ix = 0;
	g_omv.iy = 0;
	g_omv.ui_sad = 0;
	g_tmv1.ix = 0;
	g_tmv1.iy = 0;
	g_tmv1.ui_sad = 0;
	for (ip = 0; ip < DIS_TBUFNUM; ip++) {
		g_pmv1[ip].ix = 0;
		g_pmv1[ip].iy = 0;
		g_pmv1[ip].ui_sad = 0;
	}
	g_cmv1.ix = 0;
	g_cmv1.iy = 0;
	g_cmv1.ui_sad = 0;

	g_ui_cur_cr_x = 0;
	g_ui_cur_cr_y = 0;
	g_last_motvect1.ui_total_x_cnt= 0;
	g_last_motvect1.ui_zero_x_cnt= 0;
}
#if 0
#define TRK_OBJSIZE 3
VOID dis_getTrkMotionVec(DIS_MOTION_INFOR *pMotVec, TRACK_DIS_MOTION_INFOR *pTrkVec)
{
	DIS_MDS_DIM mds_dim;
	UINT32 ix, iy, im, in, vblk, hblk, idx;
	INT8 upper, lower;
	UINT32 idxn, leftTopOffs;
	//INT32 movX, movY;
	UINT8 movCnt;

	INT8 mvTol = 3; //=10*32/100;
	UINT32 cntThr = 614; // = 64*48*20/100;
	//UINT8 objSize = 3;
	UINT8 movObj_CntThr = 4;
	UINT8 movObj_mvThr = mvTol;
	INT8 histX[TRK_OBJSIZE * TRK_OBJSIZE], histY[TRK_OBJSIZE * TRK_OBJSIZE], histN[TRK_OBJSIZE * TRK_OBJSIZE];
	INT8 candX[TRK_OBJSIZE * TRK_OBJSIZE], candY[TRK_OBJSIZE * TRK_OBJSIZE];
	INT8 candTol = 2;
	UINT8 ih, candCnt, b_match, max, candIdx;
	UINT8 movObj_hnThr = 0;


	debug_msg("dis_getTrkMotionVec\r\n");

	mds_dim = dis_get_mds_dim();
	vblk = mds_dim.ui_blknum_v;
	hblk = mds_dim.ui_blknum_h * mds_dim.ui_mdsnum;


	// remove vectors of less confidence
	upper = 31 - mvTol;
	lower = -32 + mvTol;
	for (iy = 0; iy < vblk; iy++) {
		for (ix = 0; ix < hblk; ix++) {
			idx = iy * hblk + ix;
			if (pMotVec[idx].bvalid) {
				if ((pMotVec[idx].ix > upper) || (pMotVec[idx].iy > upper) ||
					(pMotVec[idx].ix < lower) || (pMotVec[idx].iy < lower)) {
					pMotVec[idx].bvalid = 0;
				}
				if (pMotVec[idx].ui_cnt < cntThr) {
					pMotVec[idx].bvalid = 0;
				}
			}
		}
	}

	// detect moving obj
	leftTopOffs = (TRK_OBJSIZE / 2) * hblk + (TRK_OBJSIZE / 2);
	for (iy = (TRK_OBJSIZE / 2); iy < (vblk - (TRK_OBJSIZE / 2)); iy++) {
		for (ix = (TRK_OBJSIZE / 2); ix < (hblk - (TRK_OBJSIZE / 2)); ix++) {
			idx = iy * hblk + ix;
			pTrkVec[idx].uiPosX = ix << 6;
			pTrkVec[idx].uiPosY = iy * 48;
			movCnt = 0;
			candCnt = 0;
			for (ih = 0; ih < (TRK_OBJSIZE * TRK_OBJSIZE); ih++) {
				histX[ih] = 0;
				histY[ih] = 0;
				histN[ih] = 100;
				candX[ih] = 0;
				candY[ih] = 0;
			}
			if ((pMotVec[idx].bvalid) && ((abs(pMotVec[idx].ix) > mvTol) || (abs(pMotVec[idx].iy) > mvTol))) {
				for (in = 0; in < TRK_OBJSIZE; in++) {
					for (im = 0; im < TRK_OBJSIZE; im++) {
						idxn = idx - leftTopOffs + in * hblk + im;
						if ((pMotVec[idxn].bvalid) && ((abs(pMotVec[idxn].ix) > mvTol) || (abs(pMotVec[idxn].iy) > mvTol))) {
							movCnt++;
							b_match = 0;
							for (ih = 0; ih < candCnt; ih++) {
								//debug_msg("   in %d %d, cand %d %d hist %d %d %d\r\n",pMotVec[idxn].ix,pMotVec[idxn].iy,candX[ih],candY[ih],histX[ih],histY[ih],histN[ih]);
								if ((abs(pMotVec[idxn].ix - candX[ih]) < candTol) && (abs(pMotVec[idxn].iy - candY[ih]) < candTol)) {
									histX[ih] += pMotVec[idxn].ix;
									histY[ih] += pMotVec[idxn].iy;
									histN[ih]++;
									b_match = 1;
								}
							}
							if (b_match == 0) {
								histX[candCnt] = pMotVec[idxn].ix;
								histY[candCnt] = pMotVec[idxn].iy;
								histN[candCnt] = 1;
								candX[candCnt] = pMotVec[idxn].ix;
								candY[candCnt] = pMotVec[idxn].iy;
								candCnt++;
							}
						}
					}
				}
				//debug_msg("   movCnt %d candCnt %d\r\n",movCnt, candCnt);
				if (movCnt > movObj_CntThr) {
					max = 0;
					candIdx = 0;
					for (ih = 0; ih < candCnt; ih++) {
						if (histN[ih] > max) {
							max = histN[ih];
							candIdx = ih;
						}
						//debug_msg("   hist (%d, %d) n = %d\r\n",histX[ih],histY[ih],histN[ih]);
					}
					pTrkVec[idx].iTrkX = histX[candIdx] / histN[candIdx];
					pTrkVec[idx].iTrkY = histY[candIdx] / histN[candIdx];

					if (((abs(pTrkVec[idx].iTrkX) > movObj_mvThr) || (abs(pTrkVec[idx].iTrkY) > movObj_mvThr)) && (histN[candIdx] > movObj_hnThr)) {
						pTrkVec[idx].bMovObj = 1;
						//debug_msg("      mov obj (%d, %d)--------------------\r\n",pTrkVec[idx].iTrkX,pTrkVec[idx].iTrkY);
						debug_msg("      mov obj (%d, %d) at (%d, %d) --------------------\r\n", pTrkVec[idx].iTrkX, pTrkVec[idx].iTrkY, pTrkVec[idx].uiPosX, pTrkVec[idx].uiPosY);
					} else {
						pTrkVec[idx].bMovObj = 0;
					}
				} else {
					pTrkVec[idx].iTrkX = 0;
					pTrkVec[idx].iTrkY = 0;
					pTrkVec[idx].bMovObj = 0;
				}
			} else {
				pTrkVec[idx].iTrkX = 0;
				pTrkVec[idx].iTrkY = 0;
				pTrkVec[idx].bMovObj = 0;
			}
#if 0
			if ((pMotVec[idx].bvalid) && ((abs(pMotVec[idx].ix) > mvTol) || (abs(pMotVec[idx].iy) > mvTol))) {
				for (in = 0; in < OBJSIZE; in++) {
					for (im = 0; im < OBJSIZE; im++) {
						idxn = idx - leftTopOffs + in * hblk + im;
						if ((pMotVec[idxn].bvalid) && ((abs(pMotVec[idxn].ix) > tol) || (abs(pMotVec[idxn].iy) > tol))) {
							movCnt++;
							movX += pMotVec[idxn].ix;
							movY += pMotVec[idxn].iy;
							debug_msg("   cnt %d mv (%d, %d)\r\n", movCnt, pMotVec[idxn].ix, pMotVec[idxn].iy);
						}
					}
				}
				if (movCnt > movObj_CntThr) {
					pTrkVec[idx].iTrkX = movX / movCnt;
					pTrkVec[idx].iTrkY = movY / movCnt;
					if ((abs(pTrkVec[idx].iTrkX) > movObj_mvThr) || (abs(pTrkVec[idx].iTrkY) > movObj_mvThr)) {
						pTrkVec[idx].bMovObj = 1;
						debug_msg("      mov obj (%d, %d)--------------------\r\n", pTrkVec[idx].iTrkX, pTrkVec[idx].iTrkY);
						//debug_msg("      mov obj (%d, %d) at (%d, %d) --------------------\r\n",pTrkVec[idx].iTrkX,pTrkVec[idx].iTrkY,pTrkVec[idx].uiPosX,pTrkVec[idx].uiPosY);
					} else {
						pTrkVec[idx].bMovObj = 0;
					}
				} else {
					pTrkVec[idx].iTrkX = 0;
					pTrkVec[idx].iTrkY = 0;
					pTrkVec[idx].bMovObj = 0;
				}
			} else {
				pTrkVec[idx].iTrkX = 0;
				pTrkVec[idx].iTrkY = 0;
				pTrkVec[idx].bMovObj = 0;
			}
#endif
		} //for(ix)
	} //for(iy)

}
#endif

void disfw_initialize(DIS_IPC_INIT *p_buf)
{
	disFw_BufferInit(p_buf);
	disFw_resetCorrVec(); //Reset DIS flow
	dis_initAlgCal();
}

void disfw_reset(void)
{
	disFw_resetCorrVec(); //Reset DIS flow
	dis_initAlgCal();
}

void disfw_process(DIS_MOTION_INFOR *p_mv)
{
	//if (gDisTrigger == FALSE)
	//{
	//    return;
	//}
	//gDisOutbuf0 = buf->Addr;
	//gDisOutbuf1 = gDisOutbuf0 + 12 * 1024;

	//dis_processOpen();
	//dis_eventStart();
	//dis_eventPause();
	disfw_accum_update_process(p_mv);
	//dis_processClose();

	//loc_cpu();
	//gDisTrigger = FALSE;
	//unl_cpu();
}
void disfw_end(void)
{
	DBG_IND("DISLib: under construction\r\n");
}

DIS_ROIGMV_OUT disfw_get_roi_motvec(DIS_ROIGMV_IN *p_roi_in, DIS_MOTION_INFOR *p_mv)
{
	UINT32 ip, index, num_cand;
	UINT32 ui_score_min;
	DIS_MDS_DIM mds_dim;
	UINT32 ui_mv_width, ui_mv_height;
	UINT32 ttl_mvnum, nth_min;
	INT32 i_mean_x[5], i_mean_y[5];
	UINT32 ttl_candnum, vect_rngth;
	DIS_ROIGMV_OUT roiOut;

	// Check ROI setting
	if (((p_roi_in->ui_start_x + p_roi_in->ui_size_x) > 100) || ((p_roi_in->ui_start_y + p_roi_in->ui_size_y) > 100)) {
		DBG_ERR("Illegal ROI setting!\r\n");
		roiOut.bvalid_vec = FALSE;
		roiOut.vector.ix  = 0;
		roiOut.vector.iy  = 0;
	} else {
		// Calculate total MV num in ROI
		mds_dim = dis_get_mds_dim();
		//uimvStX = (mds_dim.ui_mdsnum * mds_dim.ui_blknum_h)*pRoiIn->uiStartX/100;
		//uimvStY = mds_dim.ui_blknum_v*pRoiIn->uiStartY/100;
		ui_mv_width = (mds_dim.ui_mdsnum * mds_dim.ui_blknum_h) * p_roi_in->ui_size_x / 100;
		ui_mv_height = mds_dim.ui_blknum_v * p_roi_in->ui_size_y / 100;
		ttl_mvnum = ui_mv_width * ui_mv_height;

		DBG_IND("sz %d x %d, totoal %d x %d!\r\n", ui_mv_width, ui_mv_height, mds_dim.ui_mdsnum * mds_dim.ui_blknum_h, mds_dim.ui_blknum_v);

		// Level = Normal
		ui_score_min = 1400;
		//uimvMax = 16;

		// Copy to global variables for further modification
		for (ip = 0; ip < ttl_mvnum; ip++) {
			g_blkmv[ip].bvalid = p_mv[ip].bvalid;
			g_blkmv[ip].ix = p_mv[ip].ix;
			g_blkmv[ip].iy = p_mv[ip].iy;
			g_blkmv[ip].ui_cnt = p_mv[ip].ui_cnt;
			g_blkmv[ip].ui_idx = p_mv[ip].ui_idx;
			g_blkmv[ip].ui_sad = p_mv[ip].ui_sad;
		}

		// Pick out candidates
		nth_min = DIS_NUM_CAND_FILTER;
		if (nth_min >= ttl_mvnum) {
			nth_min = ttl_mvnum >> 1;
		}

		for (ip = 0; ip < ttl_mvnum; ip++) {
			g_uicand[ip] = ip;
		}

		ttl_candnum = ttl_mvnum;
		vect_rngth = 16;
		for (index = 0; index < 5; index++) {
			i_mean_x[index] = 0;
			i_mean_y[index] = 0;
			num_cand = 0;
			for (ip = 0; ip < ttl_candnum; ip++) {
				if ((g_blkmv[g_uicand[ip]].ui_sad > ui_score_min) && (g_blkmv[g_uicand[ip]].bvalid)) {
					g_uicand[num_cand] = g_uicand[ip];
					i_mean_x[index] += g_blkmv[g_uicand[ip]].ix;
					i_mean_y[index] += g_blkmv[g_uicand[ip]].iy;
					num_cand++;
				}
			}
			ttl_candnum = num_cand;

            DBG_IND("ttl_candnum = %d\n\r", ttl_candnum);
			if (num_cand) {
				i_mean_x[index] = i_mean_x[index] / (INT32)num_cand;
				i_mean_y[index] = i_mean_y[index] / (INT32)num_cand;
			}
            DBG_IND("i_mean_x index %d, (%d, %d)\r\n", index, i_mean_x[index], i_mean_y[index]);
			if (vect_rngth) {
				for (ip = 0; ip < ttl_candnum; ip++) {
					if (((UINT32)DIS_ABS(g_blkmv[g_uicand[ip]].ix - i_mean_x[index]) > vect_rngth) ||
						((UINT32)DIS_ABS(g_blkmv[g_uicand[ip]].iy - i_mean_y[index]) > vect_rngth)) {
						g_blkmv[g_uicand[ip]].bvalid = 0;
					}
				}
			}
			vect_rngth = vect_rngth / 2;
		}

        DBG_IND("num_cand %d, nth_min %d\r\n", num_cand, nth_min);

		if (num_cand > nth_min) {
			//if((DIS_ABS(i_mean_x[4])>uimvMax) ||(DIS_ABS(i_mean_y[4])>uimvMax))
			//{
			//    DBG_ERR("Invalid motion information!\r\n");
			//    pRoiInfo->bvalidVec = FALSE;
			//    pRoiInfo->vector.ix = 0;
			//    pRoiInfo->vector.iy = 0;
			//}
			//else
			//{
			roiOut.bvalid_vec = TRUE;
			roiOut.vector.ix  = i_mean_x[4];
			roiOut.vector.iy  = i_mean_y[4];
			//}
		} else {
			DBG_IND("Insufficient motion information!\r\n");
			roiOut.bvalid_vec = FALSE;
			roiOut.vector.ix  = 0;
			roiOut.vector.iy  = 0;
		}
		DBG_IND("roi vec %d, (%d, %d)\r\n", roiOut.bvalid_vec, roiOut.vector.ix, roiOut.vector.iy);
	}

	return roiOut;

}

//@}

