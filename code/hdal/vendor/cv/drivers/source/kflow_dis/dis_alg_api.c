/**
    DIS Process API

    This file contains the control interface of DIS process.

    @file       dis_alg_api.c
    @ingroup    mISYSAlg
    @note       N/A

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/


/** \addtogroup mISYSAlg */


//@{
//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#include "dis_alg_lib.h"
#include "dis_alg_task.h"
#include "dis_alg.h"

ER g_ret;

DIS_IPC_INIT g_dis_buf = {0};
BOOL g_dis_en_compen = TRUE;
BOOL g_dis_eth_out_sel = 0;
UINT16 g_dis_sub_sel = 1;
UINT16 g_dis_eth_sub_sel = 1;
UINT32 g_mv_scale_up_ration = 1000;
BOOL g_dump_global_mv = FALSE;

#define PROF                            DISABLE


ER nvt_dis_buffer_set(void* mem)
{
	DIS_IPC_INIT buf = *(DIS_IPC_INIT*)mem;

    g_dis_buf.addr = buf.addr;
    g_dis_buf.size = buf.size;

	if ((g_dis_buf.addr == 0)||(g_dis_buf.size < nvt_dis_cal_buffer_size())) {
		DBG_ERR("err:nvt_dis_buffer_set error %d\r\n",g_ret);
		return E_PAR;
	}
	return E_OK;
}


ER nvt_dis_init(void)
{
	DIS_IPC_INIT buf;
	buf.addr = g_dis_buf.addr;
	buf.size = dis_get_prvmaxBuffer();
	dis_initialize(&buf);
	buf.addr = g_dis_buf.addr + dis_get_prvmaxBuffer();
	buf.size = dis_eth_get_prvmaxBuffer();
	g_ret = dis_eth_initialize(&buf, g_dis_eth_out_sel);
	if (g_ret != E_OK) {
		DBG_ERR("err:nvt_dis_init error %d\r\n",g_ret);
		return E_PAR;
	}
	return E_OK;
}

ER nvt_dis_uninit(void)
{
	dis_end();
	g_ret = dis_eth_uninitialize();
	if (g_ret != E_OK) {
		DBG_ERR("err:nvt_dis_uninit error %d\r\n",g_ret);
		return E_PAR;
	}
	return E_OK;
}
UINT32 nvt_dis_cal_buffer_size(void)
{
	UINT32 buffer_size = 0;
	buffer_size  = dis_get_prvmaxBuffer();
	buffer_size += dis_eth_get_prvmaxBuffer();
	return buffer_size;
}

ER nvt_dis_process(void)
{
	static UINT32              prv_addr = 0;
	DIS_ETH_OUT_PARAM          eth_out_info = {0};
	DIS_ETH_BUFFER_INFO        eth_out_addr = {0};
	DIS_PARAM                  ipl_disInfo = {0};

    #if PROF
    static struct timeval tstart, tend, tstart_dis, tend_dis, tstart_dis_other, tend_dis_other;
    static UINT64 cur_time = 0, sum_time = 0;
    static UINT32 icount = 0;
    static UINT64 cur_time_dis = 0, sum_time_dis = 0;
    static UINT32 icount_dis = 0;
    static UINT64 cur_time_dis_other = 0, sum_time_dis_other = 0;
    static UINT32 icount_dis_other = 0;
    #endif

#if PROF
    do_gettimeofday(&tstart);
#endif

	g_ret = dis_eth_get_out_info(&eth_out_info);
	if (g_ret != E_OK) {
		DBG_ERR("err:dis_eth_get_out_info error %d\r\n",g_ret);
		return E_PAR;
	}

	g_ret = dis_eth_get_out_addr(&eth_out_addr);
	if ((g_ret != E_OK)&&(g_ret != E_SYS)){
		DBG_ERR("err:dis_eth_get_out_addr error %d\r\n",g_ret);
		return E_PAR;
	}
	DBG_IND("eth_out_info: w = %d, h = %d, lineoff = %d\n\r ",eth_out_info.w, eth_out_info.h, eth_out_info.out_lofs);
	DBG_IND("eth_out_addr: uiInAdd0 = 0x%x, buf_size = 0x%x, frame_cnt = %d\n\r ",eth_out_addr.ui_inadd, eth_out_addr.buf_size, eth_out_addr.frame_cnt);
	if((eth_out_info.h == 0)||(eth_out_info.w == 0)||(g_ret == E_SYS)){
		DBG_IND("eth not ready yet\n\r");
		return E_OK;
	}
#if PROF
        do_gettimeofday(&tend);
        cur_time = (tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec);
        sum_time += cur_time;
        //mean_time = sum_time/(++icount);
        DBG_DUMP("[eth] cur time(us): %lld, sum time(us): %lld, count: %d\r\n", cur_time, sum_time, ++icount);
#endif

#if PROF
        do_gettimeofday(&tstart_dis_other);
#endif

	ipl_disInfo.frame_cnt   = eth_out_addr.frame_cnt;
	ipl_disInfo.in_size_h   = eth_out_info.w;
	ipl_disInfo.in_size_v   = eth_out_info.h;
	ipl_disInfo.in_lineofs  = eth_out_info.out_lofs;

	if (prv_addr == 0) {
		ipl_disInfo.in_add0 = eth_out_addr.ui_inadd;
	} else {
		ipl_disInfo.in_add0 = prv_addr;
	}

    ipl_disInfo.in_add1     = eth_out_addr.ui_inadd;
	ipl_disInfo.in_add2     = eth_out_addr.ui_inadd;

	prv_addr = eth_out_addr.ui_inadd;

	dis_push_time_stamp((UINT64)(eth_out_addr.frame_cnt), eth_out_addr.frame_cnt);

	DBG_IND("disInfo: h = %d\n\r", ipl_disInfo.in_size_v);
	DBG_IND("disInfo: w = %d\n\r", ipl_disInfo.in_size_h);
	DBG_IND("disInfo: lineofs = %d\n\r", ipl_disInfo.in_lineofs);
	DBG_IND("disInfo: frame_cnt = %d\n\r", ipl_disInfo.frame_cnt);
	DBG_IND("disInfo: in_add0 = 0x%x\n\r", ipl_disInfo.in_add0);
	DBG_IND("disInfo: in_add1 = 0x%x\n\r", ipl_disInfo.in_add1);
	DBG_IND("disInfo: in_add2 = 0x%x\n\r", ipl_disInfo.in_add2);



    g_ret = dis_set_disinfor(&ipl_disInfo);
	if (g_ret != E_OK) {
		DBG_ERR("err:dis_set_disinfor error %d\r\n",g_ret);
		return E_PAR;
	}

#if PROF
        do_gettimeofday(&tend_dis_other);
        cur_time_dis_other = (tend_dis_other.tv_sec - tstart_dis_other.tv_sec) * 1000000 + (tend_dis_other.tv_usec - tstart_dis_other.tv_usec);
        sum_time_dis_other += cur_time_dis_other;
        //mean_time = sum_time/(++icount);
        DBG_DUMP("[dis other] cur time(us): %lld, sum time(us): %lld, count: %d\r\n", cur_time_dis_other, sum_time_dis_other, ++icount_dis_other);
#endif

#if PROF
        do_gettimeofday(&tstart_dis);
#endif

    if(g_dis_en_compen == FALSE) {
	    g_ret = dis_process_no_compen_info();
    } else {
        g_ret = dis_process();
    }
	if (g_ret != E_OK) {
		DBG_ERR("err:dis_process_no_compen_info error %d\r\n",g_ret);
		return E_PAR;
	}
#if PROF
    do_gettimeofday(&tend_dis);
    cur_time_dis = (tend_dis.tv_sec - tstart_dis.tv_sec) * 1000000 + (tend_dis.tv_usec - tstart_dis.tv_usec);
    sum_time_dis += cur_time_dis;
    //mean_time = sum_time/(++icount);
    DBG_DUMP("[dis] cur time(us): %lld, sum time(us): %lld, count: %d\r\n", cur_time_dis, sum_time_dis, ++icount_dis);
#endif

	return E_OK;
}

ER nvt_dis_set(void *p_dis_alg_set_info)
{
    DIS_ALG_PARAM *dis_alg_set = (DIS_ALG_PARAM *) p_dis_alg_set_info;

    if(dis_alg_set->subsample_sel == DIS_ALG_SUBSAMPLE_SEL_1X) {
        g_dis_eth_out_sel = 0;
        g_dis_sub_sel = 1;
        g_dis_eth_sub_sel = 1;
        dis_set_subin(DIS_SUBSAMPLE_SEL_1X);
    }else if(dis_alg_set->subsample_sel == DIS_ALG_SUBSAMPLE_SEL_2X) {
        g_dis_eth_out_sel = 1;
        g_dis_sub_sel = 1;
        g_dis_eth_sub_sel = 2;
        dis_set_subin(DIS_SUBSAMPLE_SEL_1X);
    }else if(dis_alg_set->subsample_sel == DIS_ALG_SUBSAMPLE_SEL_4X) {
        g_dis_eth_out_sel = 1;
        g_dis_sub_sel = 2;
        g_dis_eth_sub_sel = 2;
        dis_set_subin(DIS_SUBSAMPLE_SEL_2X);
    }else if(dis_alg_set->subsample_sel == DIS_ALG_SUBSAMPLE_SEL_8X) {
        g_dis_eth_out_sel = 1;
        g_dis_sub_sel = 4;
        g_dis_eth_sub_sel = 2;
        dis_set_subin(DIS_SUBSAMPLE_SEL_4X);
    }else{
        DBG_ERR("sorry, not support dis subsample = %d till now\n\r", dis_alg_set->subsample_sel);
        return E_PAR;
    }

    g_mv_scale_up_ration = dis_alg_set->scale_up_ratio;


    dis_set_dis_viewratio((UINT16)(2000 - dis_alg_set->scale_up_ratio));

    return E_OK;

}

ER nvt_dis_get(void *p_dis_alg_get_info)
{
    DIS_ALG_PARAM dis_alg_get = {0};
    DIS_PARAM info = {0};
    DISALG_VECTOR dis_alg_mv = {0};
    UINT32 frame_cnt = ((DIS_ALG_PARAM *) p_dis_alg_get_info)->frm_cnt;

    if(g_dis_eth_out_sel == 0 ) {
        dis_alg_get.subsample_sel = DIS_ALG_SUBSAMPLE_SEL_1X;
    }else if(g_dis_eth_out_sel == 1) {
        if(dis_get_subin_info() == DIS_SUBSAMPLE_SEL_1X) {
            dis_alg_get.subsample_sel = DIS_ALG_SUBSAMPLE_SEL_2X;
        }else if(dis_get_subin_info() == DIS_SUBSAMPLE_SEL_2X) {
             dis_alg_get.subsample_sel = DIS_ALG_SUBSAMPLE_SEL_4X;
        }else if(dis_get_subin_info() == DIS_SUBSAMPLE_SEL_4X) {
            dis_alg_get.subsample_sel = DIS_ALG_SUBSAMPLE_SEL_8X;
        }
    }else{
        DBG_ERR("not support\n\r");
        return E_PAR;
    }

    dis_get_input_info(&info);

    dis_get_frame_corrvec(&dis_alg_mv, frame_cnt);

	dis_alg_get.global_mv.x = dis_alg_mv.vector.ix;
	dis_alg_get.global_mv.y = dis_alg_mv.vector.iy;
    dis_alg_get.global_mv_valid = (BOOL)dis_alg_mv.score;

	DBG_IND("info.size: w = %d, h = %d\n\r", info.in_size_h, info.in_size_v);

    if(g_dump_global_mv){
	    DBG_DUMP("kflow_dis: compensation_mv: <%d %d>, score: %d, frm_cnt: %d\n\r", dis_alg_get.global_mv.x, dis_alg_get.global_mv.y, dis_alg_get.global_mv_valid, frame_cnt);
    }

    dis_alg_get.frm_cnt = frame_cnt;

    dis_alg_get.scale_up_ratio = g_mv_scale_up_ration;

    *(DIS_ALG_PARAM*)p_dis_alg_get_info = dis_alg_get;
    return E_OK;
}


ER nvt_dis_reset(void)
{
    dis_reset_compensation_mv();
    return E_OK;
}

EXPORT_SYMBOL(nvt_dis_cal_buffer_size);
EXPORT_SYMBOL(nvt_dis_buffer_set);

//@}

