/**
	@file	   kdrv_md.c
	@ingroup	Predefined_group_name

	@brief	  md device abstraction layer

	Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "kdrv_md_int.h"

#if defined __UITRON || defined __ECOS
#define __MODULE__	kdrv_ai
#define __DBGLVL__	2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__	"*" // *=All, [mark1]=CustomClass
#include "DebugModule.h"
#else
//#include "md_dbg.h"
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/flag.h"
#endif

#if defined(__FREERTOS)
#include <rtosfdt.h>
#include <stdio.h>
#include <libfdt.h>
#include <compiler.h>
#endif
#define CEIL(a, n) ((((a)+((1<<(n))-1))>>(n))<<(n))

static INT32 g_kdrv_md_id = 0;
static INT32 g_kdrv_is_nonblocking = 0;
static INT32 g_kdrv_md_open_count = 0;
static MDBC_OPENOBJ g_mdbc_open_cfg;
static UINT32 g_kdrv_md_init_flg = 0;
static KDRV_MD_HANDLE g_kdrv_md_handle_tab[KDRV_MD_HANDLE_MAX_NUM];
static KDRV_MD_HANDLE *g_kdrv_md_trig_hdl = 0;
KDRV_MD_PARAM g_kdrv_md_parm[KDRV_MD_HANDLE_MAX_NUM] = {0};
#if defined(__FREERTOS)
unsigned int md_clock_freq[1];
#endif
/*
#ifdef __KERNEL__
#else
static UINT32 *gp_md_flag_id = NULL;
#endif
*/
static UINT32 kdrv_md_fmd_flag = 0;
INT32 KDRV_MD_ISR_CB(UINT32 handle, UINT32 sts, void* in_data, void* out_data)
{
	if(g_kdrv_md_id == KDRV_MD_ID_0) {
		if ((sts & MDBC_INT_FRM)||(sts & MDBC_INT_LLEND)) {
			kdrv_md_fmd_flag = 1;
		}
	}
	return 0;
}
static ER kdrv_md_set_mode(UINT32 id)
{
	ER rt = E_OK;
	if(g_kdrv_md_id == KDRV_MD_ID_0) {
		MDBC_PARAM mdbc = {0};

        if(g_kdrv_md_parm[id].uiLLAddr)
        {
            mdbc.uiLLAddr = g_kdrv_md_parm[id].uiLLAddr;
            rt = mdbc_ll_setmode(mdbc.uiLLAddr);
        } else {
    		if (g_kdrv_md_parm[id].mode == KDRV_INIT) {
    			mdbc.mode = INIT;
    		}else{
                mdbc.mode = NORM;
            }

            mdbc.controlEn.update_nei_en = g_kdrv_md_parm[id].controlEn.update_nei_en;
            mdbc.controlEn.deghost_en    = g_kdrv_md_parm[id].controlEn.deghost_en;
            mdbc.controlEn.roi_en0       = g_kdrv_md_parm[id].controlEn.roi_en0;
            mdbc.controlEn.roi_en1       = g_kdrv_md_parm[id].controlEn.roi_en1;
            mdbc.controlEn.roi_en2       = g_kdrv_md_parm[id].controlEn.roi_en2;
            mdbc.controlEn.roi_en3       = g_kdrv_md_parm[id].controlEn.roi_en3;
            mdbc.controlEn.roi_en4       = g_kdrv_md_parm[id].controlEn.roi_en4;
            mdbc.controlEn.roi_en5       = g_kdrv_md_parm[id].controlEn.roi_en5;
            mdbc.controlEn.roi_en6       = g_kdrv_md_parm[id].controlEn.roi_en6;
            mdbc.controlEn.roi_en7       = g_kdrv_md_parm[id].controlEn.roi_en7;
            mdbc.controlEn.chksum_en     = g_kdrv_md_parm[id].controlEn.chksum_en;
            mdbc.controlEn.bgmw_save_bw_en = g_kdrv_md_parm[id].controlEn.bgmw_save_bw_en;

    		mdbc.uiIntEn  				= MDBC_INTE_FRM;
    		mdbc.InInfo.uiInAddr0  		= g_kdrv_md_parm[id].InInfo.uiInAddr0;
    		mdbc.InInfo.uiInAddr1  		= g_kdrv_md_parm[id].InInfo.uiInAddr1;
    		mdbc.InInfo.uiInAddr2  		= g_kdrv_md_parm[id].InInfo.uiInAddr2;
    		mdbc.InInfo.uiInAddr3  		= g_kdrv_md_parm[id].InInfo.uiInAddr3;
    		mdbc.InInfo.uiInAddr4  		= g_kdrv_md_parm[id].InInfo.uiInAddr4;
    		mdbc.InInfo.uiInAddr5  		= g_kdrv_md_parm[id].InInfo.uiInAddr5;
    		mdbc.InInfo.uiLofs0    		= g_kdrv_md_parm[id].InInfo.uiLofs0;
    		mdbc.InInfo.uiLofs1    		= g_kdrv_md_parm[id].InInfo.uiLofs1;

    		mdbc.OutInfo.uiOutAddr0  	= g_kdrv_md_parm[id].OutInfo.uiOutAddr0;
    		mdbc.OutInfo.uiOutAddr1  	= g_kdrv_md_parm[id].OutInfo.uiOutAddr1;
    		mdbc.OutInfo.uiOutAddr2  	= g_kdrv_md_parm[id].OutInfo.uiOutAddr2;
    		mdbc.OutInfo.uiOutAddr3  	= g_kdrv_md_parm[id].OutInfo.uiOutAddr3;

    		mdbc.Size.uiMdbcWidth  		= g_kdrv_md_parm[id].Size.uiMdbcWidth;
    		mdbc.Size.uiMdbcHeight 		= g_kdrv_md_parm[id].Size.uiMdbcHeight;

    		mdbc.MdmatchPara.lbsp_th    = g_kdrv_md_parm[id].MdmatchPara.lbsp_th;
    		mdbc.MdmatchPara.d_colour	= g_kdrv_md_parm[id].MdmatchPara.d_colour;
    		mdbc.MdmatchPara.r_colour	= g_kdrv_md_parm[id].MdmatchPara.r_colour;
    		mdbc.MdmatchPara.d_lbsp 	= g_kdrv_md_parm[id].MdmatchPara.d_lbsp;
    		mdbc.MdmatchPara.r_lbsp 	= g_kdrv_md_parm[id].MdmatchPara.r_lbsp;
    		mdbc.MdmatchPara.model_num 	= g_kdrv_md_parm[id].MdmatchPara.model_num;
    		mdbc.MdmatchPara.t_alpha 	= g_kdrv_md_parm[id].MdmatchPara.t_alpha;
    		mdbc.MdmatchPara.dw_shift 	= g_kdrv_md_parm[id].MdmatchPara.dw_shift;
    		mdbc.MdmatchPara.dlast_alpha= g_kdrv_md_parm[id].MdmatchPara.dlast_alpha;
    		mdbc.MdmatchPara.min_match 	= g_kdrv_md_parm[id].MdmatchPara.min_match;
    		mdbc.MdmatchPara.dlt_alpha 	= g_kdrv_md_parm[id].MdmatchPara.dlt_alpha;
    		mdbc.MdmatchPara.dst_alpha 	= g_kdrv_md_parm[id].MdmatchPara.dst_alpha;
    		mdbc.MdmatchPara.uv_thres 	= g_kdrv_md_parm[id].MdmatchPara.uv_thres;
    		mdbc.MdmatchPara.s_alpha 	= g_kdrv_md_parm[id].MdmatchPara.s_alpha;
    		mdbc.MdmatchPara.dbg_lumDiff= g_kdrv_md_parm[id].MdmatchPara.dbg_lumDiff;
    		mdbc.MdmatchPara.dbg_lumDiff_en	= g_kdrv_md_parm[id].MdmatchPara.dbg_lumDiff_en;

    		mdbc.MorPara.th_ero			= g_kdrv_md_parm[id].MorPara.th_ero;
    		mdbc.MorPara.th_dil			= g_kdrv_md_parm[id].MorPara.th_dil;
    		mdbc.MorPara.mor_sel0		= g_kdrv_md_parm[id].MorPara.mor_sel0;
    		mdbc.MorPara.mor_sel1		= g_kdrv_md_parm[id].MorPara.mor_sel1;
    		mdbc.MorPara.mor_sel2		= g_kdrv_md_parm[id].MorPara.mor_sel2;
    		mdbc.MorPara.mor_sel3		= g_kdrv_md_parm[id].MorPara.mor_sel3;

    		mdbc.UpdPara.minT			= g_kdrv_md_parm[id].UpdPara.minT;
    		mdbc.UpdPara.maxT			= g_kdrv_md_parm[id].UpdPara.maxT;
    		mdbc.UpdPara.maxFgFrm		= g_kdrv_md_parm[id].UpdPara.maxFgFrm;
    		mdbc.UpdPara.deghost_dth	= g_kdrv_md_parm[id].UpdPara.deghost_dth;
    		mdbc.UpdPara.deghost_sth	= g_kdrv_md_parm[id].UpdPara.deghost_sth;
    		mdbc.UpdPara.stable_frm		= g_kdrv_md_parm[id].UpdPara.stable_frm;
    		mdbc.UpdPara.update_dyn		= g_kdrv_md_parm[id].UpdPara.update_dyn;
    		mdbc.UpdPara.va_distth		= g_kdrv_md_parm[id].UpdPara.va_distth;
    		mdbc.UpdPara.t_distth		= g_kdrv_md_parm[id].UpdPara.t_distth;
    		mdbc.UpdPara.dbg_frmID		= g_kdrv_md_parm[id].UpdPara.dbg_frmID;
    		mdbc.UpdPara.dbg_frmID_en	= g_kdrv_md_parm[id].UpdPara.dbg_frmID_en;
    		mdbc.UpdPara.dbg_rnd		= g_kdrv_md_parm[id].UpdPara.dbg_rnd;
    		mdbc.UpdPara.dbg_rnd_en		= g_kdrv_md_parm[id].UpdPara.dbg_rnd_en;

    		if(g_kdrv_md_parm[id].controlEn.roi_en0){
    			mdbc.ROIPara0.roi_x  		= g_kdrv_md_parm[id].ROIPara0.roi_x;
    			mdbc.ROIPara0.roi_y  		= g_kdrv_md_parm[id].ROIPara0.roi_y;
    			mdbc.ROIPara0.roi_w  		= g_kdrv_md_parm[id].ROIPara0.roi_w;
    			mdbc.ROIPara0.roi_h  		= g_kdrv_md_parm[id].ROIPara0.roi_h;
    			mdbc.ROIPara0.roi_uv_thres 	= g_kdrv_md_parm[id].ROIPara0.roi_uv_thres;
    			mdbc.ROIPara0.roi_lbsp_th  	= g_kdrv_md_parm[id].ROIPara0.roi_lbsp_th;
    			mdbc.ROIPara0.roi_d_colour 	= g_kdrv_md_parm[id].ROIPara0.roi_d_colour;
    			mdbc.ROIPara0.roi_r_colour 	= g_kdrv_md_parm[id].ROIPara0.roi_r_colour;
    			mdbc.ROIPara0.roi_d_lbsp 	= g_kdrv_md_parm[id].ROIPara0.roi_d_lbsp;
    			mdbc.ROIPara0.roi_r_lbsp 	= g_kdrv_md_parm[id].ROIPara0.roi_r_lbsp;
    			mdbc.ROIPara0.roi_morph_en 	= g_kdrv_md_parm[id].ROIPara0.roi_morph_en;
    			mdbc.ROIPara0.roi_minT 		= g_kdrv_md_parm[id].ROIPara0.roi_minT;
    			mdbc.ROIPara0.roi_maxT 		= g_kdrv_md_parm[id].ROIPara0.roi_maxT;
    		}
    		if(g_kdrv_md_parm[id].controlEn.roi_en1){
    			mdbc.ROIPara1.roi_x  		= g_kdrv_md_parm[id].ROIPara1.roi_x;
    			mdbc.ROIPara1.roi_y  		= g_kdrv_md_parm[id].ROIPara1.roi_y;
    			mdbc.ROIPara1.roi_w  		= g_kdrv_md_parm[id].ROIPara1.roi_w;
    			mdbc.ROIPara1.roi_h  		= g_kdrv_md_parm[id].ROIPara1.roi_h;
    			mdbc.ROIPara1.roi_uv_thres 	= g_kdrv_md_parm[id].ROIPara1.roi_uv_thres;
    			mdbc.ROIPara1.roi_lbsp_th  	= g_kdrv_md_parm[id].ROIPara1.roi_lbsp_th;
    			mdbc.ROIPara1.roi_d_colour 	= g_kdrv_md_parm[id].ROIPara1.roi_d_colour;
    			mdbc.ROIPara1.roi_r_colour 	= g_kdrv_md_parm[id].ROIPara1.roi_r_colour;
    			mdbc.ROIPara1.roi_d_lbsp 	= g_kdrv_md_parm[id].ROIPara1.roi_d_lbsp;
    			mdbc.ROIPara1.roi_r_lbsp 	= g_kdrv_md_parm[id].ROIPara1.roi_r_lbsp;
    			mdbc.ROIPara1.roi_morph_en 	= g_kdrv_md_parm[id].ROIPara1.roi_morph_en;
    			mdbc.ROIPara1.roi_minT 		= g_kdrv_md_parm[id].ROIPara1.roi_minT;
    			mdbc.ROIPara1.roi_maxT 		= g_kdrv_md_parm[id].ROIPara1.roi_maxT;
    		}
    		if(g_kdrv_md_parm[id].controlEn.roi_en2){
    			mdbc.ROIPara2.roi_x  		= g_kdrv_md_parm[id].ROIPara2.roi_x;
    			mdbc.ROIPara2.roi_y  		= g_kdrv_md_parm[id].ROIPara2.roi_y;
    			mdbc.ROIPara2.roi_w  		= g_kdrv_md_parm[id].ROIPara2.roi_w;
    			mdbc.ROIPara2.roi_h  		= g_kdrv_md_parm[id].ROIPara2.roi_h;
    			mdbc.ROIPara2.roi_uv_thres 	= g_kdrv_md_parm[id].ROIPara2.roi_uv_thres;
    			mdbc.ROIPara2.roi_lbsp_th  	= g_kdrv_md_parm[id].ROIPara2.roi_lbsp_th;
    			mdbc.ROIPara2.roi_d_colour 	= g_kdrv_md_parm[id].ROIPara2.roi_d_colour;
    			mdbc.ROIPara2.roi_r_colour 	= g_kdrv_md_parm[id].ROIPara2.roi_r_colour;
    			mdbc.ROIPara2.roi_d_lbsp 	= g_kdrv_md_parm[id].ROIPara2.roi_d_lbsp;
    			mdbc.ROIPara2.roi_r_lbsp 	= g_kdrv_md_parm[id].ROIPara2.roi_r_lbsp;
    			mdbc.ROIPara2.roi_morph_en 	= g_kdrv_md_parm[id].ROIPara2.roi_morph_en;
    			mdbc.ROIPara2.roi_minT 		= g_kdrv_md_parm[id].ROIPara2.roi_minT;
    			mdbc.ROIPara2.roi_maxT 		= g_kdrv_md_parm[id].ROIPara2.roi_maxT;
    		}
    		if(g_kdrv_md_parm[id].controlEn.roi_en3){
    			mdbc.ROIPara3.roi_x  		= g_kdrv_md_parm[id].ROIPara3.roi_x;
    			mdbc.ROIPara3.roi_y  		= g_kdrv_md_parm[id].ROIPara3.roi_y;
    			mdbc.ROIPara3.roi_w  		= g_kdrv_md_parm[id].ROIPara3.roi_w;
    			mdbc.ROIPara3.roi_h  		= g_kdrv_md_parm[id].ROIPara3.roi_h;
    			mdbc.ROIPara3.roi_uv_thres 	= g_kdrv_md_parm[id].ROIPara3.roi_uv_thres;
    			mdbc.ROIPara3.roi_lbsp_th  	= g_kdrv_md_parm[id].ROIPara3.roi_lbsp_th;
    			mdbc.ROIPara3.roi_d_colour 	= g_kdrv_md_parm[id].ROIPara3.roi_d_colour;
    			mdbc.ROIPara3.roi_r_colour 	= g_kdrv_md_parm[id].ROIPara3.roi_r_colour;
    			mdbc.ROIPara3.roi_d_lbsp 	= g_kdrv_md_parm[id].ROIPara3.roi_d_lbsp;
    			mdbc.ROIPara3.roi_r_lbsp 	= g_kdrv_md_parm[id].ROIPara3.roi_r_lbsp;
    			mdbc.ROIPara3.roi_morph_en 	= g_kdrv_md_parm[id].ROIPara3.roi_morph_en;
    			mdbc.ROIPara3.roi_minT 		= g_kdrv_md_parm[id].ROIPara3.roi_minT;
    			mdbc.ROIPara3.roi_maxT 		= g_kdrv_md_parm[id].ROIPara3.roi_maxT;
    		}
    		if(g_kdrv_md_parm[id].controlEn.roi_en4){
    			mdbc.ROIPara4.roi_x  		= g_kdrv_md_parm[id].ROIPara4.roi_x;
    			mdbc.ROIPara4.roi_y  		= g_kdrv_md_parm[id].ROIPara4.roi_y;
    			mdbc.ROIPara4.roi_w  		= g_kdrv_md_parm[id].ROIPara4.roi_w;
    			mdbc.ROIPara4.roi_h  		= g_kdrv_md_parm[id].ROIPara4.roi_h;
    			mdbc.ROIPara4.roi_uv_thres 	= g_kdrv_md_parm[id].ROIPara4.roi_uv_thres;
    			mdbc.ROIPara4.roi_lbsp_th  	= g_kdrv_md_parm[id].ROIPara4.roi_lbsp_th;
    			mdbc.ROIPara4.roi_d_colour 	= g_kdrv_md_parm[id].ROIPara4.roi_d_colour;
    			mdbc.ROIPara4.roi_r_colour 	= g_kdrv_md_parm[id].ROIPara4.roi_r_colour;
    			mdbc.ROIPara4.roi_d_lbsp 	= g_kdrv_md_parm[id].ROIPara4.roi_d_lbsp;
    			mdbc.ROIPara4.roi_r_lbsp 	= g_kdrv_md_parm[id].ROIPara4.roi_r_lbsp;
    			mdbc.ROIPara4.roi_morph_en 	= g_kdrv_md_parm[id].ROIPara4.roi_morph_en;
    			mdbc.ROIPara4.roi_minT 		= g_kdrv_md_parm[id].ROIPara4.roi_minT;
    			mdbc.ROIPara4.roi_maxT 		= g_kdrv_md_parm[id].ROIPara4.roi_maxT;
    		}
    		if(g_kdrv_md_parm[id].controlEn.roi_en5){
    			mdbc.ROIPara5.roi_x  		= g_kdrv_md_parm[id].ROIPara5.roi_x;
    			mdbc.ROIPara5.roi_y  		= g_kdrv_md_parm[id].ROIPara5.roi_y;
    			mdbc.ROIPara5.roi_w  		= g_kdrv_md_parm[id].ROIPara5.roi_w;
    			mdbc.ROIPara5.roi_h  		= g_kdrv_md_parm[id].ROIPara5.roi_h;
    			mdbc.ROIPara5.roi_uv_thres 	= g_kdrv_md_parm[id].ROIPara5.roi_uv_thres;
    			mdbc.ROIPara5.roi_lbsp_th  	= g_kdrv_md_parm[id].ROIPara5.roi_lbsp_th;
    			mdbc.ROIPara5.roi_d_colour 	= g_kdrv_md_parm[id].ROIPara5.roi_d_colour;
    			mdbc.ROIPara5.roi_r_colour 	= g_kdrv_md_parm[id].ROIPara5.roi_r_colour;
    			mdbc.ROIPara5.roi_d_lbsp 	= g_kdrv_md_parm[id].ROIPara5.roi_d_lbsp;
    			mdbc.ROIPara5.roi_r_lbsp 	= g_kdrv_md_parm[id].ROIPara5.roi_r_lbsp;
    			mdbc.ROIPara5.roi_morph_en 	= g_kdrv_md_parm[id].ROIPara5.roi_morph_en;
    			mdbc.ROIPara5.roi_minT 		= g_kdrv_md_parm[id].ROIPara5.roi_minT;
    			mdbc.ROIPara5.roi_maxT 		= g_kdrv_md_parm[id].ROIPara5.roi_maxT;
    		}
    		if(g_kdrv_md_parm[id].controlEn.roi_en6){
    			mdbc.ROIPara6.roi_x  		= g_kdrv_md_parm[id].ROIPara6.roi_x;
    			mdbc.ROIPara6.roi_y  		= g_kdrv_md_parm[id].ROIPara6.roi_y;
    			mdbc.ROIPara6.roi_w  		= g_kdrv_md_parm[id].ROIPara6.roi_w;
    			mdbc.ROIPara6.roi_h  		= g_kdrv_md_parm[id].ROIPara6.roi_h;
    			mdbc.ROIPara6.roi_uv_thres 	= g_kdrv_md_parm[id].ROIPara6.roi_uv_thres;
    			mdbc.ROIPara6.roi_lbsp_th  	= g_kdrv_md_parm[id].ROIPara6.roi_lbsp_th;
    			mdbc.ROIPara6.roi_d_colour 	= g_kdrv_md_parm[id].ROIPara6.roi_d_colour;
    			mdbc.ROIPara6.roi_r_colour 	= g_kdrv_md_parm[id].ROIPara6.roi_r_colour;
    			mdbc.ROIPara6.roi_d_lbsp 	= g_kdrv_md_parm[id].ROIPara6.roi_d_lbsp;
    			mdbc.ROIPara6.roi_r_lbsp 	= g_kdrv_md_parm[id].ROIPara6.roi_r_lbsp;
    			mdbc.ROIPara6.roi_morph_en 	= g_kdrv_md_parm[id].ROIPara6.roi_morph_en;
    			mdbc.ROIPara6.roi_minT 		= g_kdrv_md_parm[id].ROIPara6.roi_minT;
    			mdbc.ROIPara6.roi_maxT 		= g_kdrv_md_parm[id].ROIPara6.roi_maxT;
    		}
    		if(g_kdrv_md_parm[id].controlEn.roi_en7){
    			mdbc.ROIPara7.roi_x  		= g_kdrv_md_parm[id].ROIPara7.roi_x;
    			mdbc.ROIPara7.roi_y  		= g_kdrv_md_parm[id].ROIPara7.roi_y;
    			mdbc.ROIPara7.roi_w  		= g_kdrv_md_parm[id].ROIPara7.roi_w;
    			mdbc.ROIPara7.roi_h  		= g_kdrv_md_parm[id].ROIPara7.roi_h;
    			mdbc.ROIPara7.roi_uv_thres 	= g_kdrv_md_parm[id].ROIPara7.roi_uv_thres;
    			mdbc.ROIPara7.roi_lbsp_th  	= g_kdrv_md_parm[id].ROIPara7.roi_lbsp_th;
    			mdbc.ROIPara7.roi_d_colour 	= g_kdrv_md_parm[id].ROIPara7.roi_d_colour;
    			mdbc.ROIPara7.roi_r_colour 	= g_kdrv_md_parm[id].ROIPara7.roi_r_colour;
    			mdbc.ROIPara7.roi_d_lbsp 	= g_kdrv_md_parm[id].ROIPara7.roi_d_lbsp;
    			mdbc.ROIPara7.roi_r_lbsp 	= g_kdrv_md_parm[id].ROIPara7.roi_r_lbsp;
    			mdbc.ROIPara7.roi_morph_en 	= g_kdrv_md_parm[id].ROIPara7.roi_morph_en;
    			mdbc.ROIPara7.roi_minT 		= g_kdrv_md_parm[id].ROIPara7.roi_minT;
    			mdbc.ROIPara7.roi_maxT 		= g_kdrv_md_parm[id].ROIPara7.roi_maxT;
    		}

    		rt = mdbc_setMode(&mdbc);
        }
	}
	return rt;
}

static void kdrv_md_lock(KDRV_MD_HANDLE* p_handle, BOOL flag)
{
	if (p_handle == NULL) {
		DBG_ERR("kdrv_md_lock : p_handle is NULL!\r\n");
	} else {
    	if (flag == TRUE) {
    		FLGPTN flag_ptn;
    		wai_flg(&flag_ptn, p_handle->flag_id, p_handle->lock_bit, TWF_CLR);
    	} else {
    		set_flg( p_handle->flag_id, p_handle->lock_bit);
    	}
	}
}

static void kdrv_md_handle_lock(void)
{
	FLGPTN wait_flg;
	wai_flg(&wait_flg, kdrv_md_get_flag_id(FLG_ID_KDRV_MD), KDRV_MD_HDL_UNLOCK, (TWF_ORW | TWF_CLR));
}

static void kdrv_md_handle_unlock(void)
{
	set_flg(kdrv_md_get_flag_id(FLG_ID_KDRV_MD), KDRV_MD_HDL_UNLOCK);
}

static void kdrv_md_handle_alloc_all(void)
{
	UINT32 i;

	kdrv_md_handle_lock();
	for (i = 0; i < KDRV_MD_HANDLE_MAX_NUM; i++) {
		if (!(g_kdrv_md_init_flg & (1 << i))) {
			g_kdrv_md_init_flg |= (1 << i);

			memset(&g_kdrv_md_handle_tab[i], 0, sizeof(KDRV_MD_HANDLE));
			g_kdrv_md_handle_tab[i].entry_id = i;
			g_kdrv_md_handle_tab[i].flag_id = kdrv_md_get_flag_id(FLG_ID_KDRV_MD);
			g_kdrv_md_handle_tab[i].lock_bit = (1 << i);
			g_kdrv_md_handle_tab[i].sts |= KDRV_MD_HANDLE_LOCK;
			g_kdrv_md_handle_tab[i].sem_id = kdrv_md_get_sem_id(SEMID_KDRV_MD);
		}
	}
	kdrv_md_handle_unlock();

	return;
}

static UINT32 kdrv_md_handle_free(KDRV_MD_HANDLE* p_handle)
{
	UINT32 rt = FALSE;
	if (p_handle == NULL) {
		DBG_ERR("kdrv_md_handle_free : p_handle is NULL!\r\n");
		return rt;
	}
	kdrv_md_handle_lock();
	p_handle->sts = 0;
	g_kdrv_md_init_flg &= ~(1 << p_handle->entry_id);
	if (g_kdrv_md_init_flg == 0) {
		rt = TRUE;
	}
	kdrv_md_handle_unlock();

	return rt;
}

static KDRV_MD_HANDLE* kdrv_md_entry_id_conv2_handle(UINT32 entry_id)
{
	return  &g_kdrv_md_handle_tab[entry_id];
}

static void kdrv_md_sem(KDRV_MD_HANDLE* p_handle, BOOL flag)
{
	if (p_handle == NULL) {
		DBG_ERR("kdrv_md_sem : p_handle is NULL!\r\n");
	} else {
    	if (flag == TRUE) {
    		SEM_WAIT(*p_handle->sem_id);	// wait semaphore
    	} else {
    		SEM_SIGNAL_ISR(*p_handle->sem_id);	// wait semaphore
    	}
	}
}

static void kdrv_md_frm_end(KDRV_MD_HANDLE* p_handle, BOOL flag)
{
	if (p_handle == NULL) {
		DBG_ERR("kdrv_md_frm_end : p_handle is NULL!\r\n");
	} else {
    	if (flag == TRUE) {
    		iset_flg(p_handle->flag_id, KDRV_MD_FMD | KDRV_MD_TIMEOUT);
    	} else {
    		clr_flg(p_handle->flag_id, KDRV_MD_FMD | KDRV_MD_TIMEOUT);
    	}
	}
}

static void kdrv_md_isr_cb(UINT32 intstatus)
{
	KDRV_MD_HANDLE *p_handle;
	p_handle = g_kdrv_md_trig_hdl;

	//DBG_IND("intstatus %d\r\n", intstatus);
	if(g_kdrv_md_id == KDRV_MD_ID_0) {
		if ((intstatus & MDBC_INT_FRM) || (intstatus & MDBC_INT_LLEND)){
			if (g_kdrv_is_nonblocking) {
				mdbc_ll_pause();
				kdrv_md_sem(p_handle, FALSE);
				kdrv_md_frm_end(p_handle, TRUE);
			} else {
			    mdbc_pause();
				kdrv_md_sem(p_handle, FALSE);
				kdrv_md_frm_end(p_handle, TRUE);
			}
		}
        if(intstatus & MDBC_INT_LLERROR) {
            DBG_ERR("ERROR : Linking List ERROR\r\n");
            mdbc_ll_pause();
			kdrv_md_sem(p_handle, FALSE);
			kdrv_md_frm_end(p_handle, TRUE);
        }
	}

	if (p_handle->isrcb_fp != NULL) {
		p_handle->isrcb_fp((UINT32)p_handle, intstatus, NULL, NULL);
	}
}

#if defined(__FREERTOS)
UINT32 kdrv_md_drv_get_clock_freq(UINT8 clk_idx)
{
	unsigned char *fdt_addr = (unsigned char *)fdt_get_base();
	char path[64] = {0};
	INT nodeoffset;
	UINT32 *cell = NULL;
	UINT32 idx;

	for(idx = 0; idx < 1; idx++) {
		md_clock_freq[idx] = 0;
	}
	sprintf(path,"/md@f0c10000");
	nodeoffset = fdt_path_offset((const void*)fdt_addr, path);

	cell = (UINT32 *)fdt_getprop((const void*)fdt_addr, nodeoffset, "clock-frequency", NULL);
	if (cell) {
		md_clock_freq[0] = be32_to_cpu(cell[0]);
	}

	//DBG_ERR("AI_RTOS: ai_clock(%d %d %d %d)\r\n", ai_clock_freq[0], ai_clock_freq[1], ai_clock_freq[2], ai_clock_freq[3]);	

	return md_clock_freq[clk_idx];
}
#endif
/*
#ifndef __KERNEL__
static void kdrv_md_timeout_cb(UINT32 timer_id)
{
	set_flg(*gp_md_flag_id, KDRV_MD_TIMEOUT);
}
#endif
*/
INT32 kdrv_md_open(UINT32 chip, UINT32 engine)
{
	if (g_kdrv_md_open_count < KDRV_MD_HANDLE_MAX_NUM && g_kdrv_md_open_count > 0) {
	} else if (g_kdrv_md_open_count == 0) {
		INT32 i;
		MDBC_OPENOBJ mdbc_drv_obj;

		switch (engine) {
		case KDRV_CV_ENGINE_MD:
			g_kdrv_md_id = KDRV_MD_ID_0;
			break;

		default:
			DBG_ERR("engine id error...\r\n");
			return -1;
			break;
		}

#if defined(__FREERTOS)
		UINT32 md_clk_freq;	
		md_clk_freq = kdrv_md_drv_get_clock_freq(0);
		md_platform_create_resource(md_clk_freq);

		kdrv_md_install_id();
	    kdrv_md_init();
#endif

		DBG_IND("Engine #%d, open mdbc HW\r\n",g_kdrv_md_id);
		mdbc_drv_obj.uiMdbcClockSel = g_mdbc_open_cfg.uiMdbcClockSel;
		mdbc_drv_obj.FP_MDBCISR_CB = kdrv_md_isr_cb;
		if (mdbc_open(&mdbc_drv_obj) != E_OK) {
			DBG_WRN("KDRV MD: mdbc_open failed\r\n");
			return -1;
		}

		kdrv_md_handle_alloc_all();
		for (i = 0; i < KDRV_MD_HANDLE_MAX_NUM; i++) {
			kdrv_md_lock(&g_kdrv_md_handle_tab[i], TRUE);
			memset((void *)&g_kdrv_md_parm[i], 0, sizeof(KDRV_MD_PARAM));
			kdrv_md_lock(&g_kdrv_md_handle_tab[i], FALSE);
		}

	} else if (g_kdrv_md_open_count > KDRV_MD_HANDLE_MAX_NUM) {
		DBG_ERR("open too many times!\r\n");
		g_kdrv_md_open_count = KDRV_MD_HANDLE_MAX_NUM;
		return -1;
	}

	g_kdrv_md_open_count++;

	return 0;
}
EXPORT_SYMBOL(kdrv_md_open);

INT32 kdrv_md_close(UINT32 chip, UINT32 engine)
{
	ER rt = E_OK;

	g_kdrv_md_open_count--;

	if (g_kdrv_md_open_count > 0) {
	} else if (g_kdrv_md_open_count == 0) {
		INT32 i;
		DBG_IND("Engine #%d, close mdbc HW\r\n",g_kdrv_md_id);
		rt = mdbc_close();
		for (i = 0; i < KDRV_MD_HANDLE_MAX_NUM; i++) {
    		kdrv_md_lock(&g_kdrv_md_handle_tab[i], TRUE);
    		kdrv_md_handle_free(&g_kdrv_md_handle_tab[i]);
    		kdrv_md_lock(&g_kdrv_md_handle_tab[i], FALSE);
		}
	} else if (g_kdrv_md_open_count < 0) {
		DBG_ERR("close too many times!\r\n");
		g_kdrv_md_open_count = 0;
		rt = E_PAR;
	}

#if defined(__FREERTOS)
	kdrv_md_install_id();
	kdrv_md_init();
#endif

	return rt;
}
EXPORT_SYMBOL(kdrv_md_close);

INT32 kdrv_md_trigger(UINT32 id, KDRV_MD_TRIGGER_PARAM *p_md_param, VOID *p_cb_func, VOID *p_user_data)
{
	ER rt = E_ID;
	KDRV_MD_HANDLE *p_handle;
	INT32 channel = KDRV_DEV_ID_CHANNEL(id);

    kdrv_md_fmd_flag = 0;
	p_handle = kdrv_md_entry_id_conv2_handle(channel);
    g_kdrv_md_handle_tab[channel].isrcb_fp = (KDRV_MD_ISRCB)&KDRV_MD_ISR_CB;
	kdrv_md_sem(p_handle, TRUE);

	DBG_IND("id 0x%x\r\n", id);
	//set md kdrv parameters to mdbc driver
	if (kdrv_md_set_mode(channel) != E_OK) {
		kdrv_md_sem(p_handle, FALSE);
		return rt;
	}

	g_kdrv_md_trig_hdl = p_handle;
	//DBG_IND("1\r\n");

	kdrv_md_frm_end(p_handle, FALSE);
	if(g_kdrv_md_id == KDRV_MD_ID_0) {
        if(p_md_param->is_nonblock) {
            mdbc_clr_ll_frameend();
    		//DBG_IND("2\r\n");
            g_kdrv_is_nonblocking = 1;
    		//trigger mdbc start
    		rt = mdbc_ll_start();
    		if (rt != E_OK) {
                DBG_ERR("mdbc_ll_start fail");
    			kdrv_md_sem(p_handle, FALSE);
    			return rt;
    		}
    		DBG_IND("mdbc ll start\r\n");
    		mdbc_wait_ll_frameend(FALSE);
    		DBG_IND("mdbc ll frameend\r\n");
        } else {
            mdbc_clrFrameEnd();
    		//DBG_IND("2\r\n");
            g_kdrv_is_nonblocking = 0;
    		//trigger mdbc start
    		rt = mdbc_start();
    		if (rt != E_OK) {
                DBG_ERR("mdbc_start fail");
    			kdrv_md_sem(p_handle, FALSE);
    			return rt;
    		}
    		DBG_IND("mdbc start\r\n");
    		mdbc_waitFrameEnd(FALSE);
        }
	}
    //DBG_IND("Before while\r\n");
    while(!kdrv_md_fmd_flag) {
		vos_task_delay_ms(10);
	}
	return rt;
}
EXPORT_SYMBOL(kdrv_md_trigger);

static ER kdrv_md_set_opencfg(UINT32 id, void* data)
{
	KDRV_MD_OPENCFG kdrv_md_open_obj;

	if (data == NULL) {
		DBG_ERR("id %d input data NULL!\r\n", id);
		return E_PAR;
	}
	kdrv_md_open_obj = *(KDRV_MD_OPENCFG *)data;
	g_mdbc_open_cfg.uiMdbcClockSel = kdrv_md_open_obj.clock_sel;

	return E_OK;
}

static ER kdrv_md_set_param(UINT32 id, void* data)
{
	if (data == NULL) {
		DBG_ERR("id %d input data NULL!\r\n", id);
		return E_PAR;
	}
	g_kdrv_md_parm[id] = *((KDRV_MD_PARAM*)data);

	return E_OK;
}

static ER kdrv_md_set_isr_cb(UINT32 id, void* data)
{
	if (data == NULL) {
		DBG_ERR("id %d input data NULL!\r\n", id);
		return E_PAR;
	}
	g_kdrv_md_handle_tab[id].isrcb_fp = (KDRV_MD_ISRCB)data;
	return E_OK;
}

KDRV_MD_SET_FP kdrv_md_set_fp[KDRV_MD_PARAM_MAX] = {
	kdrv_md_set_opencfg,
	kdrv_md_set_param,
	kdrv_md_set_isr_cb,
};

INT32 kdrv_md_set(UINT32 id, KDRV_MD_PARAM_ID parm_id, VOID *p_param)
{
	ER rt = E_OK;
	//UINT32 ign_chk;
	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);

	if (p_param == NULL) {
		DBG_ERR("id %d p_param NULL!\r\n", id);
		return E_PAR;
	}
	//ign_chk = (KDRV_MD_IGN_CHK & parm_id);
	parm_id = parm_id & (~KDRV_MD_IGN_CHK);

	if (kdrv_md_set_fp[parm_id] != NULL) {
		rt = kdrv_md_set_fp[parm_id](channel, p_param);
	}
	return rt;
}
EXPORT_SYMBOL(kdrv_md_set);

static ER kdrv_md_get_param(UINT32 id, void* data)
{
	if (data == NULL) {
		DBG_ERR("id %d input data NULL!\r\n", id);
		return E_PAR;
	}
	*(KDRV_MD_PARAM*)data = g_kdrv_md_parm[id];
	return E_OK;
}

static ER kdrv_md_get_isr_cb(UINT32 id, void* data)
{
	if (data == NULL) {
		DBG_ERR("id %d input data NULL!\r\n", id);
		return E_PAR;
	}
	*(KDRV_MD_ISRCB*)data = g_kdrv_md_handle_tab[id].isrcb_fp;
	return E_OK;
}

static ER kdrv_md_get_reg_data(UINT32 id, void* data)
{
	KDRV_MD_REG_DATA reg_data;
	if (data == NULL) {
		DBG_ERR("id %d input data NULL!\r\n", id);
		return E_PAR;
	}
	reg_data.uiLumDiff = mdbc_getLumDiff();
	reg_data.uiFrmID = mdbc_getFrmID();
	reg_data.uiRnd = mdbc_getRND();
	*(KDRV_MD_REG_DATA*)data = reg_data;
	return E_OK;
}

KDRV_MD_GET_FP kdrv_md_get_fp[KDRV_MD_PARAM_MAX] = {
	NULL,
	kdrv_md_get_param,
	kdrv_md_get_isr_cb,
	kdrv_md_get_reg_data,
};

INT32 kdrv_md_get(UINT32 id, KDRV_MD_PARAM_ID parm_id, VOID *p_param)
{
	ER rt = E_OK;

	//UINT32 ign_chk;
	if (p_param == NULL) {
		DBG_ERR("id %d p_param NULL!\r\n", id);
		return E_PAR;
	}
	//ign_chk = (KDRV_MD_IGN_CHK & parm_id);
	parm_id = parm_id & (~KDRV_MD_IGN_CHK);
	if (kdrv_md_get_fp[parm_id] != NULL) {
		rt = kdrv_md_get_fp[parm_id](KDRV_DEV_ID_CHANNEL(id), p_param);
	}

	return rt;
}
EXPORT_SYMBOL(kdrv_md_get);

#if 0
void kdrv_ai_dump_info(int (*dump)(const char *fmt, ...))
{
	BOOL is_print_header;
	BOOL is_open[KDRV_MD_HANDLE_MAX_NUM] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
	INT32 id;


	if (g_kdrv_md_open_count == 0) {
		dump("\r\n[KDRV AI] not open\r\n");
		return;
	}


	for (id = 0; id < KDRV_MD_HANDLE_MAX_NUM; id++) {
		if ((g_kdrv_md_init_flg & (1 << id)) == (UINT32)(1 << id)) {
			is_open[id] = TRUE;
			kdrv_md_lock(kdrv_md_entry_id_conv2_handle(id), TRUE);
		}
	}

	dump("\r\n[AI]\r\n");
	dump("\r\n-----ctrl info-----\r\n");
	dump("%12s%12s\r\n", "init_flg", "trig_id");
	dump("%#12x 0x%.8x\r\n", g_kdrv_md_init_flg, g_kdrv_md_trig_hdl);

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
	for (id = 0; id < KDRV_MD_HANDLE_MAX_NUM; id++) {
		if (!is_print_header) {
			dump("%12s%3s%12s%12s%12s%12s\r\n", "CFA-Dir", "id", "PAT", "DifNBit", "NsMarDiff", "NsMarEdge");
			is_print_header = TRUE;
		}
	}

	for (id = 0; id < KDRV_MD_HANDLE_MAX_NUM; id++) {
		if (is_open[id]) {
			kdrv_md_lock(kdrv_md_entry_id_conv2_handle(id), FALSE);
		}
	}

}
EXPORT_SYMBOL(kdrv_ai_dump_info);
#endif