/**
    DIS Process API

    This file contains the HW control of DIS process.

    @file       dis_alg_hw.c
    @ingroup    mISYSAlg
    @note       N/A

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/


/** \addtogroup mISYSAlg */
//@{

#include "kdrv_dis.h"
#include "dis_alg_hw.h"

static ER                      g_ret;
UINT32                         g_cur_framecnt_hw = 0;
static UINT32                  g_ipe_out_hsize_hw = 1920, g_ipe_out_vsize_hw = 1080;
static UINT32                  g_ipe_lineofs_hw = 480;
UINT32                         g_cur_disconfig_inh_hw = 1920, g_cur_disconfig_inv_hw = 1080;
static UINT32                  g_dis_inbuf0, g_dis_inbuf1, g_dis_inbuf2;
static UINT32                  g_dis_outbuf0, g_dis_outbuf1;

UINT8                          g_dis_lock = FALSE;
volatile DIS_PROC_EVENT        g_dis_proc_event = DIS_PROC_NOOP;

DIS_BLKSZ                      g_dis_aply_block_sz;
DIS_SUBSEL                     g_dis_aply_subin_sel;
DIS_MDS_DIM                    g_dis_mv_dim;

extern DIS_IPC_INIT            g_dis_mv_addr;
extern DIS_BLKSZ               g_dis_cur_block_sz;
extern DIS_SUBSEL              g_dis_cur_subin_sel;
extern BOOL                    g_blk_size_en;

#ifdef DUMP_MV_MAP
static UINT32                  g_avail_buf_addr;
#endif

ER dishw_set_dis_infor(DISHW_PARAM *p_disinfo)
{
	g_ipe_out_hsize_hw = p_disinfo->insize_h;
	if ((g_ipe_out_hsize_hw & 0x3) != 0) {
		DBG_ERR("err: Input hsize should be 4x!!\r\n");
		return E_PAR;
	}
	g_ipe_out_vsize_hw = p_disinfo->insize_v;
	g_ipe_lineofs_hw  = p_disinfo->inlineofs;
	if (g_ipe_lineofs_hw < (((g_ipe_out_hsize_hw + 15) >> 4) << 2)) {
		DBG_ERR("err: Input lineoffset might be too small, g_ipe_out_hsize_hw = %d, g_ipe_lineofs_hw = %d!!\r\n", g_ipe_out_hsize_hw,g_ipe_lineofs_hw);
		return E_PAR;
	}
	g_dis_inbuf0      = p_disinfo->inadd0;
	g_dis_inbuf1      = p_disinfo->inadd1;
	g_dis_inbuf2      = p_disinfo->inadd2;
	g_cur_framecnt_hw = p_disinfo->frame_cnt;

	return E_OK;
}

UINT32 dishw_get_prv_maxbuffer(void)
{
	UINT32 out_bufsz;

	out_bufsz = 2 * DIS_MVNUMMAX * 8;	// jsliu@171023 (8: 4 byts/displacement * 2 displacement/block)

	return out_bufsz;
}

void disHw_BufferInit(DIS_IPC_INIT *p_buf)
{
	g_dis_outbuf0 = p_buf->addr;
	#ifndef DUMP_MV_MAP
	g_dis_outbuf1 = g_dis_outbuf0 + 4*DIS_MVNUMMAX*2;	// jsliu@171023, max_output size = 4 (bytes) * blks (max_blk#/img) * 2 (x, y displacement)
	#else
	g_dis_outbuf1 = g_dis_outbuf0;
	g_avail_buf_addr = p_buf->addr + 2 * DIS_MVNUMMAX * 4;
	#endif
	p_buf->addr += dishw_get_prv_maxbuffer();
	p_buf->size -= dishw_get_prv_maxbuffer();
}

void disHw_processLock(BOOL lock)
{
	if (lock) {
		if (g_dis_lock == FALSE) {
			g_dis_lock = TRUE;
		}
	} else {
		if (g_dis_lock == TRUE) {
			g_dis_lock = FALSE;
		}
	}
}
/**
    Lock DIS hardware engine
    @param lock UNLOCK: unlock DIS HW, LOCK:lock DIS HW
*/
ER disHw_configDisHw(void)
{
	KDRV_DIS_IN_IMG_INFO             dis_input_info;
	KDRV_DIS_IN_DMA_INFO             dis_in_addr;
	KDRV_DIS_OUT_DMA_INFO            dis_out_addr;
	UINT32                           int_en = 0;//enable when process is unlocked

	dis_in_addr.ui_inadd0        = vos_cpu_get_phy_addr(g_dis_inbuf0);
	dis_in_addr.ui_inadd1        = vos_cpu_get_phy_addr(g_dis_inbuf1);
	dis_in_addr.ui_inadd2        = vos_cpu_get_phy_addr(g_dis_inbuf2);
	dis_out_addr.ui_outadd0      = vos_cpu_get_phy_addr(g_dis_outbuf0);
	dis_out_addr.ui_outadd1      = vos_cpu_get_phy_addr(g_dis_outbuf1);
	dis_input_info.ui_width      = g_ipe_out_hsize_hw;
	dis_input_info.ui_height     = g_ipe_out_vsize_hw;
	dis_input_info.ui_inofs      = g_ipe_lineofs_hw;//(((dis_input_info.uiHeight >> 2) + 3) >> 2) << 2;
	dis_input_info.changesize_en = FALSE;

	g_cur_disconfig_inh_hw = dis_input_info.ui_width;
	g_cur_disconfig_inv_hw = dis_input_info.ui_height;

	g_ret = kdrv_dis_set(0, KDRV_DIS_PARAM_IN_IMG, &dis_input_info);
	if (g_ret != E_OK) {
		DBG_ERR("err: dis set input info error %d\n", g_ret);
		return E_PAR;
	}
	g_ret = kdrv_dis_set(0, KDRV_DIS_PARAM_DMA_IN, &dis_in_addr);
	if (g_ret != E_OK) {
		DBG_ERR("err: dis set input addr error %d\n", g_ret);
		return E_PAR;
	}
	g_ret = kdrv_dis_set(0, KDRV_DIS_PARAM_INT_EN, &int_en);
	if (g_ret != E_OK) {
		DBG_ERR("err: dis set enable error %d\n", g_ret);
		return E_PAR;
	}
	g_ret = kdrv_dis_set(0, KDRV_DIS_PARAM_DMA_OUT, &dis_out_addr);
	if (g_ret != E_OK) {
		DBG_ERR("err: dis set output addr error %d\n", g_ret);
		return E_PAR;
	}

	return g_ret;
}

ER disHw_ConfigDisSize(void)
{
	KDRV_DIS_IN_IMG_INFO dis_input_info;

	g_cur_disconfig_inh_hw = g_ipe_out_hsize_hw;
	g_cur_disconfig_inv_hw = g_ipe_out_vsize_hw;

	dis_input_info.ui_width      = g_ipe_out_hsize_hw;
	dis_input_info.ui_height     = g_ipe_out_vsize_hw;
	dis_input_info.ui_inofs      = g_ipe_lineofs_hw;
	dis_input_info.changesize_en = TRUE;

	g_ret = kdrv_dis_set(0, KDRV_DIS_PARAM_IN_IMG, &dis_input_info);
	if (g_ret != E_OK) {
		DBG_ERR("err: dis set input info error %d\n", g_ret);
		return E_PAR;
	}

	return g_ret;

}

ER disHw_setBlkSize(void)
{
	KDRV_BLOCKS_DIM				   blksz;
	blksz = (KDRV_BLOCKS_DIM)g_dis_cur_block_sz;

	g_ret = kdrv_dis_set(0, KDRV_DIS_PARAM_BLOCK_DIM, &blksz);
	if (g_ret != E_OK) {
		DBG_ERR("err: dis get BLOCKS_DIM output error %d\n", g_ret);
		return E_PAR;
	}

	return g_ret;
}

ER disHw_setSubIn(void)
{
	KDRV_DIS_SUBIN				   subin;
	subin = (KDRV_DIS_SUBIN)g_dis_cur_subin_sel;

	g_ret = kdrv_dis_set(0, KDRV_DIS_PARAM_SUBSAMPLE_SEL, &subin);
	if (g_ret != E_OK) {
		DBG_ERR("err: dis set KDRV_DIS_PARAM_SUBSAMPLE_SEL error %d\n", g_ret);
		return E_PAR;
	}

	return g_ret;
}


ER disHw_processOpen(void)
{

	disHw_processLock(0);

	g_ret = kdrv_dis_open(0, 0);
	if (g_ret != E_OK) {
		DBG_ERR("err:vendor_dis_init error %d\r\n",g_ret);
		return E_PAR;
	}

	g_dis_proc_event = DIS_PROC_NOOP;

	return g_ret;
}

ER disHw_eventStart(void)
{
	KDRV_DIS_TRIGGER_PARAM dis_trigger_param;

	g_ret = kdrv_dis_trigger(0, &dis_trigger_param, NULL, NULL);
	if (g_ret != E_OK) {
		DBG_ERR("err: dis trigger engine error %d\n", g_ret);
		return E_PAR;
	}
	g_dis_proc_event = DIS_PROC_UPDATE;

	disHw_processLock(0);

	return g_ret;
}

void disHw_eventPause(void)
{
	disHw_processLock(1);
	//To keep frame rate
	g_dis_proc_event = DIS_PROC_UPDATE; //DIS_PROC_NOOP;
}

ER disHw_processClose(void)
{
	g_ret = kdrv_dis_close(0, 0);
	if (g_ret != E_OK) {
		DBG_ERR("err:vendor_dis_init error %d\r\n",g_ret);
		return E_PAR;
	}
	g_dis_proc_event = DIS_PROC_NOOP;

	disHw_processLock(1);

	return g_ret;
}

ER disHw_getblksize_info(void)
{
	KDRV_BLOCKS_DIM				   blksz;

	g_ret = kdrv_dis_get(0, KDRV_DIS_PARAM_BLOCK_DIM, &blksz);
	if (g_ret != E_OK) {
		DBG_ERR("err: dis get BLOCKS_DIM output error %d\n", g_ret);
		return E_PAR;
	}

	g_dis_aply_block_sz = (DIS_BLKSZ)blksz;

	return g_ret;
}

ER disHw_getSubIn(void)
{
	KDRV_DIS_SUBIN				   subin;

	g_ret = kdrv_dis_get(0, KDRV_DIS_PARAM_SUBSAMPLE_SEL, &subin);
	if (g_ret != E_OK) {
		DBG_ERR("err: dis get KDRV_DIS_PARAM_SUBSAMPLE_SEL error %d\n", g_ret);
		return E_PAR;
	}

    g_dis_aply_subin_sel = (KDRV_DIS_SUBIN)subin;

	return g_ret;
}


ER disHw_getMDSDim(void)
{
	KDRV_MDS_DIM		  mdsDim = {0};
	g_ret = kdrv_dis_get(0, KDRV_DIS_PARAM_MV_DIM, &mdsDim);
	if (g_ret != E_OK) {
		DBG_ERR("err: dis get MDS_DIM output error %d\n", g_ret);
		return E_PAR;
	}

	g_dis_mv_dim.ui_blknum_h = mdsDim.ui_blknum_h;
	g_dis_mv_dim.ui_blknum_v = mdsDim.ui_blknum_v;
	g_dis_mv_dim.ui_mdsnum   = mdsDim.ui_mdsnum;

	return g_ret;

}

#ifdef DUMP_MV_MAP
UINT32 dis_disdebug_writedatatodram(UINT32 frmcnt, UINT32 bufstart, DIS_MOTION_INFOR motresult[])
{
    UINT32 blknumh, blknumv, ix, iy;
    UINT32 uiidx;
    UINT32 uicurbufptr;
    uicurbufptr = bufstart;
    blknumh = g_dis_mv_dim.ui_blknum_h * g_dis_mv_dim.ui_mdsnum;
    blknumv = g_dis_mv_dim.ui_blknum_v;

    uicurbufptr += sprintf((char *)uicurbufptr, "[ frame %d ]\t", frmcnt);
    uicurbufptr += sprintf((char *)uicurbufptr, "\r\n");
    uicurbufptr += sprintf((char *)uicurbufptr, "MV_X\r\n");
    for (iy=0; iy<blknumv; iy++) {
        uicurbufptr += sprintf((char *)uicurbufptr, "\t");
        for (ix=0; ix<blknumh; ix++) {
            uiidx = iy*blknumh + ix;
            uicurbufptr += sprintf((char *)uicurbufptr, "%d\t", motresult[uiidx].iX);
        }
        uicurbufptr += sprintf((char *)uicurbufptr, "\r\n");
    }
    uicurbufptr += sprintf((char *)uicurbufptr, "MV_Y\r\n");
    for (iy=0; iy<blknumv; iy++) {
        uicurbufptr += sprintf((char *)uicurbufptr, "\t");
        for (ix=0; ix<blknumh; ix++) {
            uiidx = iy*blknumh + ix;
            uicurbufptr += sprintf((char *)uicurbufptr, "%d\t", motresult[uiidx].iY);
        }
        uicurbufptr += sprintf((char *)uicurbufptr, "\r\n");
    }
    uicurbufptr += sprintf((char *)uicurbufptr, "bValid\r\n");
    for (iy=0; iy<blknumv; iy++) {
        uicurbufptr += sprintf((char *)uicurbufptr, "\t");
        for (ix=0; ix<blknumh; ix++) {
            uiidx = iy*blknumh + ix;
            uicurbufptr += sprintf((char *)uicurbufptr, "%d\t", motresult[uiidx].bValid);
        }
        uicurbufptr += sprintf((char *)uicurbufptr, "\r\n");
    }
    uicurbufptr += sprintf((char *)uicurbufptr, "\r\n");
    return uicurbufptr;
}
#endif

ER disHw_getMotionVectors(void)
{
	KDRV_MV_OUT_DMA_INFO dis_out_mv;
	dis_out_mv.p_mvaddr = (KDRV_MOTION_INFOR*)(vos_cpu_get_phy_addr(g_dis_mv_addr.addr));
	g_ret = kdrv_dis_get(0, KDRV_DIS_PARAM_MV_OUT, &dis_out_mv);
	if (g_ret != E_OK) {
		DBG_ERR("err: dis get mv output error %d\n", g_ret);
		return E_PAR;
	}

#ifdef DUMP_MV_MAP
	static UINT32 frmcnt = 2;
	UINT32 uiEndBuffer = 0;
	UINT32 out_length = 0;
	char filename[200] = {0};
	FILE *fsave=NULL;
	static UINT32 saveMvInit = 1;
	DIS_MOTION_INFOR* DisMv;
	DisMv = (DIS_MOTION_INFOR *)g_dis_mv_addr.addr;
	uiEndBuffer = dis_disdebug_writedatatodram(frmcnt++, g_avail_buf_addr, DisMv);
	out_length = uiEndBuffer - g_avail_buf_addr;
	sprintf(filename, "/mnt/sd/alg/motResult_bin_in.txt");

	if ( 1 == saveMvInit ) {
		fsave = fopen(filename, "w+");
		if(fsave == NULL) {
			printf("fopen fail\n");
			return;
		}
		saveMvInit = 0;
	} else {
		fsave = fopen(filename, "a+");
		if(fsave == NULL) {
			printf("fopen fail\n");
			return;
		}
		fseek(fsave,0,SEEK_END);
	}
	fwrite((UINT8*)g_avail_buf_addr, out_length, 1, fsave);
	fclose(fsave);
#endif
	return g_ret;

}

ER dishw_process(void)
{
	/*g_ret = disHw_processOpen();
	if (g_ret != HD_OK) {
		return HD_ERR_NG;
	}*/

	g_ret = disHw_configDisHw();
	if (g_ret != E_OK) {
		return E_PAR;
	}
	if (g_blk_size_en) {
		g_ret = disHw_setBlkSize();
		if (g_ret != E_OK) {
			return E_PAR;
		}
		g_blk_size_en = FALSE;
	}
	g_ret = disHw_setSubIn();
	if (g_ret != E_OK) {
		return E_PAR;
	}
	g_ret = disHw_eventStart();
	if (g_ret != E_OK) {
		return E_PAR;
	}
	disHw_eventPause();
	g_ret = disHw_getblksize_info();
	if (g_ret != E_OK) {
		return E_PAR;
	}
    g_ret = disHw_getSubIn();
	if (g_ret != E_OK) {
		return E_PAR;
	}
	g_ret = disHw_getMDSDim();
	if (g_ret != E_OK) {
		return E_PAR;
	}
	g_ret = disHw_getMotionVectors();
	if (g_ret != E_OK) {
		return E_PAR;
	}
	/*g_ret = disHw_processClose();
	if (g_ret != HD_OK) {
		return HD_ERR_NG;
	}*/

	return g_ret;
}

ER dishw_chg_dis_sizeconfig(void)
{
	g_ret = disHw_processOpen();
	if (g_ret != E_OK) {
		return E_PAR;
	}
	g_ret = disHw_ConfigDisSize();
	if (g_ret != E_OK) {
		return E_PAR;
	}
	g_ret = disHw_getMDSDim();
	if (g_ret != E_OK) {
		return E_PAR;
	}
	g_ret = disHw_processClose();
	if (g_ret != E_OK) {
		return E_PAR;
	}

	return g_ret;
}


void dishw_initialize(DIS_IPC_INIT *p_buf)
{
	disHw_BufferInit(p_buf);
    g_ret = disHw_processOpen();
	if (g_ret != E_OK) {
		DBG_ERR("disHw_processOpen fail!\n\r");
	}
	DBG_IND("NOOP\r\n");
}

void dishw_end(void)
{
	g_ret = disHw_processClose();
	if (g_ret != E_OK) {
		DBG_ERR("disHw_processClose fail!\n\r");
	}

	DBG_IND("Under construction\r\n");
}

UINT32 dis_get_cur_dis_inh(void)
{
	return g_cur_disconfig_inh_hw;
}

UINT32 dis_get_cur_dis_inv(void)
{
	return g_cur_disconfig_inv_hw;
}


//@}

