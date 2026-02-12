/**
    DIS Process API

    This file contains the control interface of DIS process.

    @file       dis_alg.c
    @ingroup    mISYSAlg
    @note       N/A

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/


/** \addtogroup mISYSAlg */


//@{
//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#include "dis_alg.h"


static volatile UINT32             g_dis_trigger_flag = FALSE;
static DIS_PARAM                   g_dis_size_info = {0};
static DISALG_CFGPARA              g_dis_prcfg_para;

static UINT8                       g_update_ready_flag = 0;
static DIS_MOTION_INFOR            *gp_dis_mv;
static UINT32                      g_dis_blocknum;

extern DIS_BLKSZ                   g_dis_aply_block_sz;
DIS_BLKSZ                          g_dis_cur_block_sz = DIS_BLKSZ_32x32;
extern DIS_BLKSZ                   g_dis_aply_subin_sel;
DIS_SUBSEL                         g_dis_cur_subin_sel;
BOOL                               g_blk_size_en = TRUE;

extern DIS_MDS_DIM                 g_dis_mv_dim;
extern DIS_IPC_INIT                g_dis_mv_addr;

static volatile UINT32             g_roi_update = FALSE;
DIS_ROIGMV_IN                      g_roi_gmv_in;
DIS_ROIGMV_OUT                     g_roi_gmv_out;

static UINT32                      g_dis_framecount_set_in = 0;
static UINT32                      g_dis_only_proc_mv = 0;

static DIS_ROI_MV_INFO             g_save_ready_mv = {0};


#define PROF                            DISABLE

#ifdef DIS_DEBUG
unsigned int g_dis_alg_debug_level = NVT_DBG_ERR;
#endif


/*============================================================================
                                  TimeStamp
==============================================================================*/

//#NT#2018/07/23#joshua liu -begin
//#NT#Support time stamp
static INT8                        g_dis_tstamp_index = 0;
static INT8                        g_dis_mv_index = 0;
static DIS_TIME_STAMP              g_dis_tstamp[DIS_TSTAMP_BUF_SIZE];
static DIS_MOV_VEC                 g_dis_mv[DIS_MV_BUF_SIZE];
static DIS_ROI_MOV_VEC             g_roi_gmv[DIS_MV_BUF_SIZE];


void dis_dump_tstamp_buf(void)
{
	INT32 idx;

	DBG_DUMP("\n\r - b ------------------------ \n\r");
	DBG_DUMP("\n\rDIS tstamp rbuf idx %d\n\r", g_dis_tstamp_index);
	for (idx=0; idx < DIS_TSTAMP_BUF_SIZE; idx++) {
		DBG_DUMP("  %d: fc %u, ts %llu\n\r", idx, g_dis_tstamp[idx].frame_count, g_dis_tstamp[idx].time_stamp);
	}
	DBG_DUMP("\n\rDIS mv rbuf idx %d\n\r", g_dis_mv_index);
	for (idx=0; idx < DIS_MV_BUF_SIZE; idx++) {
		DBG_DUMP("  %d: fc %u, ready %d\n\r", idx, g_dis_mv[idx].frame_count, (INT32)g_dis_mv[idx].mv_ready);
	}
	for (idx=0; idx < DIS_MV_BUF_SIZE; idx++) {
		DBG_DUMP("  %d: fc %u, ready %d\n\r", idx, g_roi_gmv[idx].frame_count, (INT32)g_roi_gmv[idx].mv_ready);
	}
	DBG_DUMP("\n\r - e ------------------------ \n\r");
}

void dis_tstamp_initialize(void)
{
	INT32 i,j;

	g_dis_tstamp_index = -1;
	for (i=0; i<DIS_TSTAMP_BUF_SIZE; i++) {
		g_dis_tstamp[i].frame_count = 0;
		g_dis_tstamp[i].time_stamp  = 0;
	}
	for (i=0; i<DIS_MV_BUF_SIZE; i++) {
		g_dis_mv[i].mv_ready    = FALSE;
		g_dis_mv[i].frame_count = 0;
		for(j=0; j<DIS_MVNUMMAX; j++) {
			memset(&g_dis_mv[i].mv[j], 0, sizeof(DIS_MOTION_INFOR));
		}
		g_roi_gmv[i].mv_ready      = FALSE;
		g_roi_gmv[i].frame_count = 0;
		g_roi_gmv[i].mv.bvalid_vec = FALSE;
        g_roi_gmv[i].mv.vector.ix  = 0;
        g_roi_gmv[i].mv.vector.iy  = 0;
	}
	DBG_DUMP("DIS time stamp mechanism initialized, %d, %d\n\r", g_dis_mv_index, g_dis_tstamp_index);

}

void dis_push_time_stamp(UINT64 time_stamp, UINT32 frame_count)
{
	INT32 new_indx;

	if (g_dis_tstamp_index >= DIS_TSTAMP_BUF_SIZE) {
		DBG_ERR("err:Index of DIS time stamp buffer out of range!?\n\r");
		return;
	}

	new_indx = (g_dis_tstamp_index+1) % DIS_TSTAMP_BUF_SIZE;
	g_dis_tstamp[new_indx].frame_count = frame_count;
	g_dis_tstamp[new_indx].time_stamp  = time_stamp;
	g_dis_tstamp_index = new_indx;

	#if 0
	dis_dump_tstamp_buf();
	#endif
}

BOOL dis_time_stamp_ready(void)
{
	if (g_dis_mv_index < 0 || g_dis_tstamp_index < 0)
		return FALSE;
	else
		return TRUE;
}

UINT64 dis_get_ready_motionvec(DIS_MV_INFO_SIZE *p_mvinfo_size)
{
	INT32 i, j,idx;
	DIS_MDS_DIM mds_dim;

	mds_dim = g_dis_mv_dim;
	p_mvinfo_size->ui_num_h = mds_dim.ui_mdsnum * mds_dim.ui_blknum_h;
	p_mvinfo_size->ui_num_v = mds_dim.ui_blknum_v;

	if (g_dis_mv_index < 0 || g_dis_mv_index >= DIS_MV_BUF_SIZE) {
		DBG_ERR("err:Index of DIS mv buffer is out of range!?\n\r");
		return 0;
	}
	if (g_dis_tstamp_index < 0 || g_dis_tstamp_index >= DIS_TSTAMP_BUF_SIZE) {
		DBG_ERR("err:Index of DIS tstamp buffer is out of range!?\n\r");
		return 0;
	}

    if ((UINT32)p_mvinfo_size->motvec == 0) {
		DBG_ERR("err: get mv address should not be zero\r\n");
		return -1;
	}

	#if 0

	DBG_DUMP("\n\r g_dis_mv_index = %d ------------------------ \n\r", g_dis_mv_index);
	DBG_DUMP("\n\r g_dis_tstamp_index = %d ------------------------ \n\r", g_dis_tstamp_index);

	dis_dump_tstamp_buf();
	#endif

	for (i=g_dis_mv_index; i>=0; i--) {
		if (g_dis_mv[i].mv_ready) {
			for (j=g_dis_tstamp_index; j>=0; j--) {
				if (g_dis_tstamp[j].frame_count==g_dis_mv[i].frame_count) {
					for (idx=0; idx < (INT32)g_dis_blocknum; idx++) {
						if (g_update_ready_flag){
							memcpy(&p_mvinfo_size->motvec[idx], &g_dis_mv[i].mv[idx], sizeof(DIS_MOTION_INFOR));
						}else{
						    memcpy(&p_mvinfo_size->motvec[idx], &gp_dis_mv[idx], sizeof(DIS_MOTION_INFOR));
						}
					}
//					DBG_MSG("-> mv idx %d, tstamp idx %d: tstamp %llu\n\r", i, j, g_dis_tstamp[j].time_stamp);
					return g_dis_tstamp[j].time_stamp;
				} else if (g_dis_tstamp[j].frame_count < g_dis_mv[i].frame_count) {
					break;
				}
			}
			for (j=DIS_TSTAMP_BUF_SIZE-1; j>g_dis_tstamp_index; j--) {
				if (g_dis_tstamp[j].frame_count==g_dis_mv[i].frame_count) {
					for (idx=0; idx < (INT32)g_dis_blocknum; idx++) {
						if (g_update_ready_flag){
							memcpy(&p_mvinfo_size->motvec[idx], &g_dis_mv[i].mv[idx], sizeof(DIS_MOTION_INFOR));
						}else{
						    memcpy(&p_mvinfo_size->motvec[idx], &gp_dis_mv[idx], sizeof(DIS_MOTION_INFOR));
						}
					}
//					DBG_MSG("-> mv idx %d, tstamp idx %d: tstamp %llu\n\r", i, j, g_dis_tstamp[j].time_stamp);
					return g_dis_tstamp[j].time_stamp;
				} else if (g_dis_tstamp[j].frame_count < g_dis_mv[i].frame_count) {
					break;
				}
			}
		}
	}
	for (i=DIS_MV_BUF_SIZE-1; i>g_dis_mv_index; i--) {
		if (g_dis_mv[i].mv_ready) {
			for (j=g_dis_tstamp_index; j>=0; j--) {
				if (g_dis_tstamp[j].frame_count==g_dis_mv[i].frame_count) {
					for (idx=0; idx < (INT32)g_dis_blocknum; idx++) {
						if (g_update_ready_flag){
							memcpy(&p_mvinfo_size->motvec[idx], &g_dis_mv[i].mv[idx], sizeof(DIS_MOTION_INFOR));
						}else{
						    memcpy(&p_mvinfo_size->motvec[idx], &gp_dis_mv[idx], sizeof(DIS_MOTION_INFOR));
						}
					}
//					DBG_MSG("-> mv idx %d, tstamp idx %d: tstamp %llu\n\r", i, j, g_dis_tstamp[j].time_stamp);
					return g_dis_tstamp[j].time_stamp;
				} else if (g_dis_tstamp[j].frame_count < g_dis_mv[i].frame_count) {
					break;
				}
			}
			for (j=DIS_TSTAMP_BUF_SIZE-1; j>g_dis_tstamp_index; j--) {
				if (g_dis_tstamp[j].frame_count==g_dis_mv[i].frame_count) {
					for (idx=0; idx < (INT32)g_dis_blocknum; idx++) {
						if (g_update_ready_flag){
							memcpy(&p_mvinfo_size->motvec[idx], &g_dis_mv[i].mv[idx], sizeof(DIS_MOTION_INFOR));
						}else{
						    memcpy(&p_mvinfo_size->motvec[idx], &gp_dis_mv[idx], sizeof(DIS_MOTION_INFOR));
						}
					}
//					DBG_MSG("-> mv idx %d, tstamp idx %d: tstamp %llu\n\r", i, j, g_dis_tstamp[j].time_stamp);
					return g_dis_tstamp[j].time_stamp;
				} else if (g_dis_tstamp[j].frame_count < g_dis_mv[i].frame_count) {
					break;
				}
			}
		}
	}

	DBG_ERR("err:Access valid motion vector with tstamp failed!\n\r");
	return -1;
}

INT32 dis_get_ready_roi_motionvec(DIS_ROI_MV_INFO *p_gmvinfo)
{
	INT32 i, j;

	if (g_dis_mv_index < 0 || g_dis_mv_index >= DIS_MV_BUF_SIZE) {
		DBG_ERR("err:Index of DIS roi mv buffer is out of range!?\n\r");
		return 0;
	}
	if (g_dis_tstamp_index < 0 || g_dis_tstamp_index >= DIS_TSTAMP_BUF_SIZE) {
		DBG_ERR("err:Index of DIS tstamp buffer is out of range!?\n\r");
		return 0;
	}

	#if 0

	DBG_DUMP("\n\r g_dis_mv_index = %d ------------------------ \n\r", g_dis_mv_index);
	DBG_DUMP("\n\r g_dis_tstamp_index = %d ------------------------ \n\r", g_dis_tstamp_index);

	dis_dump_tstamp_buf();
	#endif

	for (i=g_dis_mv_index; i>=0; i--) {
		if (g_roi_gmv[i].mv_ready) {
			for (j=g_dis_tstamp_index; j>=0; j--) {
				if (g_dis_tstamp[j].frame_count==g_roi_gmv[i].frame_count) {
                    if (g_roi_update == FALSE) {
                        p_gmvinfo->ix = g_roi_gmv[i].mv.vector.ix;
                        p_gmvinfo->iy = g_roi_gmv[i].mv.vector.iy;
                        p_gmvinfo->b_valid = g_roi_gmv[i].mv.bvalid_vec;
                        p_gmvinfo->frm_cnt = g_roi_gmv[i].frame_count;
                    } else {
                        p_gmvinfo->ix = g_roi_gmv_out.vector.ix;
                        p_gmvinfo->iy = g_roi_gmv_out.vector.iy;
                        p_gmvinfo->b_valid = g_roi_gmv_out.bvalid_vec;
                        p_gmvinfo->frm_cnt = g_dis_tstamp[j].frame_count;
                    }

					DBG_IND("-> mv idx %d, tstamp idx %d: tstamp %llu\n\r", i, j, g_dis_tstamp[j].time_stamp);
                    DBG_IND("-> mv : (%d, %d) valid: %d tstamp %llu\n\r", p_gmvinfo->ix,  p_gmvinfo->iy, p_gmvinfo->b_valid, g_dis_tstamp[j].time_stamp);
					return 0;
				} else if (g_dis_tstamp[j].frame_count < g_roi_gmv[i].frame_count) {
					break;
				}
			}
			for (j=DIS_TSTAMP_BUF_SIZE-1; j>g_dis_tstamp_index; j--) {
				if (g_dis_tstamp[j].frame_count==g_roi_gmv[i].frame_count) {
                    if (g_roi_update == FALSE) {
                        p_gmvinfo->ix = g_roi_gmv[i].mv.vector.ix;
                        p_gmvinfo->iy = g_roi_gmv[i].mv.vector.iy;
                        p_gmvinfo->b_valid = g_roi_gmv[i].mv.bvalid_vec;
                        p_gmvinfo->frm_cnt = g_dis_tstamp[j].frame_count;
                    } else {
                        p_gmvinfo->ix = g_roi_gmv_out.vector.ix;
                        p_gmvinfo->iy = g_roi_gmv_out.vector.iy;
                        p_gmvinfo->b_valid = g_roi_gmv_out.bvalid_vec;
                        p_gmvinfo->frm_cnt = g_dis_tstamp[j].frame_count;
                    }

					DBG_IND("-> mv idx %d, tstamp idx %d: tstamp %llu\n\r", i, j, g_dis_tstamp[j].time_stamp);
                    DBG_IND("-> mv : (%d, %d) valid: %d tstamp %llu\n\r", p_gmvinfo->ix,  p_gmvinfo->iy, p_gmvinfo->b_valid, g_dis_tstamp[j].time_stamp);
					return 0;
				} else if (g_dis_tstamp[j].frame_count < g_roi_gmv[i].frame_count) {
					break;
				}
			}
		}
	}
	for (i=DIS_MV_BUF_SIZE-1; i>g_dis_mv_index; i--) {
		if (g_roi_gmv[i].mv_ready) {
			for (j=g_dis_tstamp_index; j>=0; j--) {
				if (g_dis_tstamp[j].frame_count==g_roi_gmv[i].frame_count) {
                    if (g_roi_update == FALSE) {
                        p_gmvinfo->ix = g_roi_gmv[i].mv.vector.ix;
                        p_gmvinfo->iy = g_roi_gmv[i].mv.vector.iy;
                        p_gmvinfo->b_valid = g_roi_gmv[i].mv.bvalid_vec;
                        p_gmvinfo->frm_cnt = g_dis_tstamp[j].frame_count;
                    } else {
                        p_gmvinfo->ix = g_roi_gmv_out.vector.ix;
                        p_gmvinfo->iy = g_roi_gmv_out.vector.iy;
                        p_gmvinfo->b_valid = g_roi_gmv_out.bvalid_vec;
                        p_gmvinfo->frm_cnt = g_dis_tstamp[j].frame_count;
                    }

					DBG_IND("-> mv idx %d, tstamp idx %d: tstamp %llu\n\r", i, j, g_dis_tstamp[j].time_stamp);
                    DBG_IND("-> mv : (%d, %d) valid: %d tstamp %llu\n\r", p_gmvinfo->ix,  p_gmvinfo->iy, p_gmvinfo->b_valid, g_dis_tstamp[j].time_stamp);
					return 0;
				} else if (g_dis_tstamp[j].frame_count < g_roi_gmv[i].frame_count) {
					break;
				}
			}
			for (j=DIS_TSTAMP_BUF_SIZE-1; j>g_dis_tstamp_index; j--) {
				if (g_dis_tstamp[j].frame_count==g_roi_gmv[i].frame_count) {
                    if (g_roi_update == FALSE) {
                        p_gmvinfo->ix = g_roi_gmv[i].mv.vector.ix;
                        p_gmvinfo->iy = g_roi_gmv[i].mv.vector.iy;
                        p_gmvinfo->b_valid = g_roi_gmv[i].mv.bvalid_vec;
                        p_gmvinfo->frm_cnt = g_dis_tstamp[j].frame_count;
                    } else {
                        p_gmvinfo->ix = g_roi_gmv_out.vector.ix;
                        p_gmvinfo->iy = g_roi_gmv_out.vector.iy;
                        p_gmvinfo->b_valid = g_roi_gmv_out.bvalid_vec;
                        p_gmvinfo->frm_cnt = g_dis_tstamp[j].frame_count;
                    }

					DBG_IND("-> mv idx %d, tstamp idx %d: tstamp %llu\n\r", i, j, g_dis_tstamp[j].time_stamp);
                    DBG_IND("-> mv : (%d, %d) valid: %d tstamp %llu\n\r", p_gmvinfo->ix,  p_gmvinfo->iy, p_gmvinfo->b_valid, g_dis_tstamp[j].time_stamp);
					return 0;
				} else if (g_dis_tstamp[j].frame_count < g_roi_gmv[i].frame_count) {
					break;
				}
			}
		}
	}

	DBG_ERR("err:Access valid motion vector with tstamp failed!\n\r");
	return -1;
}

INT32 dis_get_match_roi_motionvec(DIS_ROI_MV_INFO *p_gmvinfo)
{
	INT32 i;

	if (g_dis_mv_index < 0 || g_dis_mv_index >= DIS_MV_BUF_SIZE) {
		DBG_ERR("err:Index of DIS roi mv buffer is out of range!?\n\r");
		return 0;
	}
	if (g_dis_tstamp_index < 0 || g_dis_tstamp_index >= DIS_TSTAMP_BUF_SIZE) {
		DBG_ERR("err:Index of DIS tstamp buffer is out of range!?\n\r");
		return 0;
	}

	#if 0

	DBG_DUMP("\n\r g_dis_mv_index = %d ------------------------ \n\r", g_dis_mv_index);
	DBG_DUMP("\n\r g_dis_tstamp_index = %d ------------------------ \n\r", g_dis_tstamp_index);

	dis_dump_tstamp_buf();
	#endif

	for (i=0; i< DIS_MV_BUF_SIZE; i++) {
		if (g_roi_gmv[i].mv_ready) {
			if (g_dis_framecount_set_in == g_roi_gmv[i].frame_count) {
                if (g_roi_update == FALSE) {
                    p_gmvinfo->ix = g_roi_gmv[i].mv.vector.ix;
                    p_gmvinfo->iy = g_roi_gmv[i].mv.vector.iy;
                    p_gmvinfo->b_valid = g_roi_gmv[i].mv.bvalid_vec;
                    p_gmvinfo->frm_cnt = g_roi_gmv[i].frame_count;
                } else {
                    p_gmvinfo->ix = g_roi_gmv_out.vector.ix;
                    p_gmvinfo->iy = g_roi_gmv_out.vector.iy;
                    p_gmvinfo->b_valid = g_roi_gmv_out.bvalid_vec;
                    p_gmvinfo->frm_cnt = g_roi_gmv[i].frame_count;
                }
                g_save_ready_mv.ix = p_gmvinfo->ix;
                g_save_ready_mv.iy = p_gmvinfo->iy;
                g_save_ready_mv.b_valid = p_gmvinfo->b_valid;
                g_save_ready_mv.frm_cnt = p_gmvinfo->frm_cnt;
                DBG_IND("-> mv idx %d, tstamp %u\n\r", i, g_roi_gmv[i].frame_count);
                DBG_IND("-> mv : (%d, %d) valid: %d tstamp %d\n\r", p_gmvinfo->ix,  p_gmvinfo->iy,  p_gmvinfo->b_valid, p_gmvinfo->frm_cnt);
                return 0;
            } else {
                DBG_IND("-> g_dis_framecount_set_in %d, g_roi_gmv[i].frame_count %u\n\r", g_dis_framecount_set_in, g_roi_gmv[i].frame_count);
            }
		}

	}

    p_gmvinfo->ix =  g_save_ready_mv.ix;
    p_gmvinfo->iy = g_save_ready_mv.iy;
    p_gmvinfo->b_valid = g_save_ready_mv.b_valid;
    p_gmvinfo->frm_cnt = g_save_ready_mv.frm_cnt;

    //dis_get_ready_roi_motionvec(p_gmvinfo);

	return -1;
}


//#NT#2018/07/23#joshua liu -end


UINT32 dis_get_prvmaxBuffer(void)
{
	UINT32 out_bufsz;
	UINT32 out_bufsz_hw;
	UINT32 out_bufsz_fw;

	out_bufsz_fw = ALIGN_CEIL_4(disfw_get_prv_maxbuffer());
	out_bufsz_hw = ALIGN_CEIL_4(dishw_get_prv_maxbuffer());
 	out_bufsz    = out_bufsz_fw+out_bufsz_hw;

	return out_bufsz;
}
void dis_roi_initialize(void)
{
	g_roi_gmv_in.ui_start_x = 0;
	g_roi_gmv_in.ui_start_y = 0;
	g_roi_gmv_in.ui_size_x  = 100;
	g_roi_gmv_in.ui_size_y  = 100;
}

void dis_initialize(DIS_IPC_INIT *p_buf)
{
	DIS_IPC_INIT buf_temp;
	buf_temp.addr = p_buf->addr;
	buf_temp.size = p_buf->size;

	dishw_initialize(&buf_temp);
	disfw_initialize(&buf_temp);

	dis_roi_initialize();

	// Add func here
	//.....

	//#NT#2018/07/23#joshua liu -begin
	//#NT#Support time stamp
	dis_tstamp_initialize();
	//#NT#2018/07/23#joshua liu -end

	g_dis_prcfg_para.bglobal_motioncal_en= TRUE;
	g_dis_prcfg_para.broi_motioncal_en   = TRUE;
	g_dis_prcfg_para.btwo_mdslayers_en   = FALSE;

	g_blk_size_en = FALSE;
}
void dis_reset_compensation_mv(void)
{
	disfw_reset();
}
ER dis_process(void)
{
	UINT32 idx = 0;
	ER ret = E_OK;

    #if PROF
    static struct timeval tstart, tend, tstart_mvcopy, tend_mvcopy;
    static UINT64 cur_time = 0, sum_time = 0;
    static UINT32 icount = 0;
    static UINT64 cur_time_mvcopy = 0, sum_time_mvcopy = 0;
    static UINT32 icount_mvcopy = 0;
    if(g_dis_only_proc_mv == 0) {
        static struct timeval tstart_compen, tend_compen, tstart_gmv, tend_gmv;
        static UINT64 cur_time_compen = 0, sum_time_compen = 0;
        static UINT32 icount_compen = 0;
        static UINT64 cur_time_gmv = 0, sum_time_gmv = 0;
        static UINT32 icount_gmv = 0;
    }
    #endif

	if (g_dis_trigger_flag == FALSE) {
		return E_OK;
	}

    #if PROF
    do_gettimeofday(&tstart);
    #endif

	// DIS HW process
	ret = dishw_process();
	if (ret != E_OK) {
		DBG_ERR("err: dishw_process error %d\n", ret);
		g_dis_trigger_flag = FALSE;
		return E_PAR;
	}

    #if PROF
        do_gettimeofday(&tend);
        cur_time = (tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec);
        sum_time += cur_time;
        //mean_time = sum_time/(++icount);
        DBG_DUMP("[dishw_process] cur time(us): %lld, sum time(us): %lld, count: %d\r\n", cur_time, sum_time, ++icount);
    #endif
	g_update_ready_flag = 0;

	g_dis_blocknum = g_dis_mv_dim.ui_blknum_h *g_dis_mv_dim.ui_blknum_v *g_dis_mv_dim.ui_mdsnum;
	gp_dis_mv = (DIS_MOTION_INFOR *)g_dis_mv_addr.addr;

#if PROF
    do_gettimeofday(&tstart_mvcopy);
#endif

	//#NT#2018/07/23#joshua liu -begin
	//#NT#Support time stamp
	if (g_dis_mv_index < 0 || g_dis_mv_index >= DIS_MV_BUF_SIZE) {
		DBG_ERR("err:Index of DIS time stamp buffer out of range, %d!?\n\r", g_dis_mv_index);
		g_dis_trigger_flag = FALSE;
		return E_PAR;
	}
	for (idx = 0; idx < g_dis_blocknum; idx++) {
		memcpy(&g_dis_mv[g_dis_mv_index].mv[idx], &gp_dis_mv[idx], sizeof(DIS_MOTION_INFOR));
	}
	g_dis_mv[g_dis_mv_index].mv_ready = TRUE;
	//#NT#2018/07/23#joshua liu -end
	g_update_ready_flag = 1;

#if PROF
    do_gettimeofday(&tend_mvcopy);
    cur_time_mvcopy = (tend_mvcopy.tv_sec - tstart_mvcopy.tv_sec) * 1000000 + (tend_mvcopy.tv_usec - tstart_mvcopy.tv_usec);
    sum_time_mvcopy += cur_time_mvcopy;
    //mean_time = sum_time/(++icount);
    DBG_DUMP("[dis_process_memcpy] cur time(us): %lld, sum time(us): %lld, count: %d\r\n", cur_time_mvcopy, sum_time_mvcopy, ++icount_mvcopy);
#endif

    if(g_dis_only_proc_mv == 0) {
#if PROF
        do_gettimeofday(&tstart_compen);
#endif


    	// DIS FW process
    	disfw_process(gp_dis_mv);

#if PROF
        do_gettimeofday(&tend_compen);
        cur_time_compen = (tend_compen.tv_sec - tstart_compen.tv_sec) * 1000000 + (tend_compen.tv_usec - tstart_compen.tv_usec);
        sum_time_compen += cur_time_compen;
        //mean_time_compen = sum_time_compen/(++icount_compen);
        DBG_DUMP("[disfw_process] cur time(us): %lld, sum time(us): %lld, count: %d\r\n", cur_time_compen, sum_time_compen, ++icount_compen);
#endif


#if PROF
        do_gettimeofday(&tstart_gmv);
#endif


    	// DIS ROI: global motion vector
    	g_roi_update = TRUE;
    	g_roi_gmv_out = disfw_get_roi_motvec(&g_roi_gmv_in, gp_dis_mv);
        memcpy(&g_roi_gmv[g_dis_mv_index].mv, &g_roi_gmv_out, sizeof(DIS_ROIGMV_OUT));

    	DBG_IND("roi_mv:vec = %d, x = %d, y = %d\n\r", g_roi_gmv_out.bvalid_vec, g_roi_gmv_out.vector.ix, g_roi_gmv_out.vector.iy);

        g_roi_gmv[g_dis_mv_index].mv_ready = TRUE;
    	g_roi_update = FALSE;

#if PROF
            do_gettimeofday(&tend_gmv);
            cur_time_gmv = (tend_gmv.tv_sec - tstart_gmv.tv_sec) * 1000000 + (tend_gmv.tv_usec - tstart_gmv.tv_usec);
            sum_time_gmv += cur_time_gmv;
            //mean_time_gmv = sum_time_gmv/(++icount_gmv);
            DBG_DUMP("[disfw_get_roi_motvec] cur time(us): %lld, sum time(us): %lld, count: %d\r\n", cur_time_gmv, sum_time_gmv, ++icount_gmv);
#endif
    }

	// Add new func here
	//.....
	g_dis_trigger_flag = FALSE;

	return E_OK;
}

ER dis_process_no_compen_info(void)
{
	UINT32 idx = 0;
	ER ret = E_OK;

	if (g_dis_trigger_flag == FALSE) {
		return E_OK;
	}

	// DIS HW process
	ret = dishw_process();
	if (ret != E_OK) {
		DBG_ERR("err: dishw_process error %d\n", ret);
		g_dis_trigger_flag = FALSE;
		return E_PAR;
	}
	g_update_ready_flag = 0;

	g_dis_blocknum = g_dis_mv_dim.ui_blknum_h *g_dis_mv_dim.ui_blknum_v *g_dis_mv_dim.ui_mdsnum;
	gp_dis_mv = (DIS_MOTION_INFOR *)g_dis_mv_addr.addr;

    DBG_IND("g_dis_mv_dim:ui_blknum_h = %d, ui_blknum_v = %d, ui_mdsnum = %d\n\r", g_dis_mv_dim.ui_blknum_h, g_dis_mv_dim.ui_blknum_v, g_dis_mv_dim.ui_mdsnum);

	//#NT#2018/07/23#joshua liu -begin
	//#NT#Support time stamp
	if (g_dis_mv_index < 0 || g_dis_mv_index >= DIS_MV_BUF_SIZE) {
		DBG_ERR("err: Index of DIS time stamp buffer out of range, %d!?\n\r", g_dis_mv_index);
		g_dis_trigger_flag = FALSE;
		return E_PAR;
	}
	for (idx = 0; idx < g_dis_blocknum; idx++) {
        DBG_IND("idx = %d, ix = %d, iy = %d, ivalid = %d \n\r", idx, gp_dis_mv[idx].ix, gp_dis_mv[idx].iy, gp_dis_mv[idx].bvalid);
		memcpy(&g_dis_mv[g_dis_mv_index].mv[idx], &gp_dis_mv[idx], sizeof(DIS_MOTION_INFOR));
	}
	g_dis_mv[g_dis_mv_index].mv_ready = TRUE;
	//#NT#2018/07/23#joshua liu -end

	#if 0
	dis_dump_tstamp_buf();
    #endif

    if(g_dis_only_proc_mv == 0) {
    	// DIS ROI: global motion vector
    	g_roi_update = TRUE;
    	g_roi_gmv_out = disfw_get_roi_motvec(&g_roi_gmv_in, gp_dis_mv);
    	DBG_IND("roi_mv:vec = %d, x = %d, y = %d\n\r", g_roi_gmv_out.bvalid_vec, g_roi_gmv_out.vector.ix, g_roi_gmv_out.vector.iy);
        memcpy(&g_roi_gmv[g_dis_mv_index].mv, &g_roi_gmv_out, sizeof(DIS_ROIGMV_OUT));
        g_roi_gmv[g_dis_mv_index].mv_ready = TRUE;
    	g_roi_update = FALSE;
    }



	g_update_ready_flag = 1;

	g_dis_trigger_flag = FALSE;

	return E_OK;

}

void dis_end(void)
{
	disfw_end();
	dishw_end();

	// Add func here
	//.....

}

ER dis_set_disinfor(DIS_PARAM *p_disinfo)
{
	DISHW_PARAM dis_hw_info;
	UINT8       new_indx;
	ER   ret;

	if (g_dis_trigger_flag == FALSE) {
		dis_hw_info.frame_cnt = p_disinfo->frame_cnt;
		dis_hw_info.inadd0    = p_disinfo->in_add0;
		dis_hw_info.inadd1    = p_disinfo->in_add1;
		dis_hw_info.inadd2    = p_disinfo->in_add2;
		dis_hw_info.inlineofs = p_disinfo->in_lineofs;
		dis_hw_info.insize_h  = p_disinfo->in_size_h;
		dis_hw_info.insize_v  = p_disinfo->in_size_v;
		ret = dishw_set_dis_infor(&dis_hw_info);
		if (ret != E_OK) {
			DBG_ERR("err: dishw_set_dis_infor error %d\n", ret);
			return E_PAR;
		}

		disfw_set_dis_infor(p_disinfo);

		g_dis_size_info.frame_cnt  = p_disinfo->frame_cnt;
		g_dis_size_info.in_add0    = p_disinfo->in_add0;
		g_dis_size_info.in_add1    = p_disinfo->in_add1;
		g_dis_size_info.in_add2    = p_disinfo->in_add2;
		g_dis_size_info.in_lineofs = p_disinfo->in_lineofs;
		g_dis_size_info.in_size_h  = p_disinfo->in_size_h;
		g_dis_size_info.in_size_v  = p_disinfo->in_size_v;

		//#NT#2018/07/23#joshua liu -begin
		//#NT#Support time stamp
		new_indx = (g_dis_mv_index+1) % DIS_MV_BUF_SIZE;
		g_dis_mv[new_indx].mv_ready = FALSE;	// invalid the new entry
		g_dis_mv[new_indx].frame_count = p_disinfo->frame_cnt;
		g_dis_mv_index = new_indx;
		g_roi_gmv[new_indx].mv_ready = FALSE;	// invalid the new entry
		g_roi_gmv[new_indx].frame_count = p_disinfo->frame_cnt;


		#if 0
		dis_dump_tstamp_buf();
		#endif
		//#NT#2018/07/23#joshua liu -end

		g_dis_trigger_flag = TRUE;
	}

	return E_OK;
}


BOOL dis_set_blksize(DIS_BLKSZ blksz)
{
	if (blksz==DIS_BLKSZ_64x48 || blksz==DIS_BLKSZ_32x32) {
		g_dis_cur_block_sz = blksz;
		g_blk_size_en = TRUE;
		return TRUE;
	}
	else {
		DBG_ERR("err:Wrong block size, %d\n\r", blksz);
		return FALSE;
	}
}

BOOL dis_set_only_mv(UINT32 dis_only_proc_mv)
{
	g_dis_only_proc_mv = dis_only_proc_mv;
    return TRUE;
}


BOOL dis_set_subin(DIS_SUBSEL subsel)
{
    if(nvt_get_chip_id() == CHIP_NA51055) {
    	if (subsel==DIS_SUBSAMPLE_SEL_2X || subsel==DIS_SUBSAMPLE_SEL_4X) {
            DBG_ERR("525 dis not support subin 4x and 8x, it will be set to 1x or 2x\n\r");
        }
        subsel=DIS_SUBSAMPLE_SEL_1X;
 	} else{
 	}
    g_dis_cur_subin_sel = subsel;
    return TRUE;
}

BOOL dis_framecnt_set_in(UINT32 frame_count)
{
    g_dis_framecount_set_in = frame_count;
    return TRUE;
}


void dis_SetConfig(DISALG_CFGID cfg_id, DISALG_CFGINFO *p_info)
{
	switch (cfg_id) {
	case DISALG_CFG_GLOBALMOTEN:
		g_dis_prcfg_para.bglobal_motioncal_en = (BOOL) p_info->icfgval;
		break;
	case DISALG_CFG_ROIMOTEN:
		g_dis_prcfg_para.broi_motioncal_en    = (BOOL) p_info->icfgval;
		break;
	case DISALG_CFG_2MDSLAYEREN:
		g_dis_prcfg_para.btwo_mdslayers_en    = (BOOL) p_info->icfgval;
		break;
	default:
		DBG_ERR("err:Invalid config ID = %d\r\n", cfg_id);
		break;
	}
}

void dis_roi_setInputInfo(DIS_ROIGMV_IN *roiIn)
{
	g_roi_gmv_in.ui_start_x = roiIn->ui_start_x;
	g_roi_gmv_in.ui_start_y = roiIn->ui_start_y;
	g_roi_gmv_in.ui_size_x  = roiIn->ui_size_x;
	g_roi_gmv_in.ui_size_y  = roiIn->ui_size_y;
}


UINT32 dis_get_trigger_status(void)
{
	return g_dis_trigger_flag;
}

void dis_get_input_info(DIS_PARAM *p_info)
{
	p_info->frame_cnt  = g_dis_size_info.frame_cnt;
	p_info->in_add0    = g_dis_size_info.in_add0;
	p_info->in_add1    = g_dis_size_info.in_add1;
	p_info->in_add2    = g_dis_size_info.in_add2;
	p_info->in_lineofs = g_dis_size_info.in_lineofs;
	p_info->in_size_h  = g_dis_size_info.in_size_h;
	p_info->in_size_v  = g_dis_size_info.in_size_v;
}

DIS_MDS_DIM dis_get_mds_dim(void)
{
	return g_dis_mv_dim;
}

DIS_BLKSZ dis_get_blksize_info(void)
{
	return g_dis_aply_block_sz;
}

DIS_SUBSEL dis_get_subin_info(void)
{
	return g_dis_aply_subin_sel;
}

UINT32 dis_get_only_mv(void)
{
    return g_dis_only_proc_mv;
}


void dis_GetConfig(DISALG_CFGID cfg_id, DISALG_CFGINFO *p_info)
{
	switch (cfg_id) {
	case DISALG_CFG_GLOBALMOTEN:
		p_info->icfgval = (INT32) g_dis_prcfg_para.bglobal_motioncal_en;
		break;
	case DISALG_CFG_ROIMOTEN:
		p_info->icfgval = (INT32) g_dis_prcfg_para.broi_motioncal_en;
		break;
	case DISALG_CFG_2MDSLAYEREN:
		p_info->icfgval = (INT32) g_dis_prcfg_para.btwo_mdslayers_en;
		break;
	default:
		DBG_ERR("err:Invalid config ID = %d\r\n", cfg_id);
		break;
	}
}
void dis_roi_getInputInfo(DIS_ROIGMV_IN *roiIn)
{
	roiIn->ui_start_x = g_roi_gmv_in.ui_start_x;
	roiIn->ui_start_y = g_roi_gmv_in.ui_start_y;
	roiIn->ui_size_x  = g_roi_gmv_in.ui_size_x;
	roiIn->ui_size_y  = g_roi_gmv_in.ui_size_y;
}

DIS_ROIGMV_OUT dis_roi_getResult(void)
{
	DIS_ROIGMV_OUT roi_out;

	if (g_roi_update == TRUE) {
		roi_out.bvalid_vec = 0;
		roi_out.vector.ix  = 0;
		roi_out.vector.iy  = 0;
		DBG_ERR("err:Updaing. Check again later.\r\n");
	} else {
		roi_out.bvalid_vec = g_roi_gmv_out.bvalid_vec;
		roi_out.vector.ix  = g_roi_gmv_out.vector.ix;
		roi_out.vector.iy  = g_roi_gmv_out.vector.iy;
	}

	return roi_out;
}

//@}

