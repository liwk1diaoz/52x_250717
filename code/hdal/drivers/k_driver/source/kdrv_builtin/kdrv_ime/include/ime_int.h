/*
    IME module driver

    NT96510 IME internal header file.

    @file       ime_int.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2017  All rights reserved.
*/

#ifndef _IME_INT_H_
#define _IME_INT_H_

#ifdef __cplusplus
extern "C" {
#endif


//#include "ime_platform.h"

/*
#ifdef __KERNEL__




#include <plat-na51055/nvt-sramctl.h>
//#include <frammap/frammap_if.h>
#include <mach/fmem.h>

#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#define dma_getNonCacheAddr(parm) 0
#define dma_getPhyAddr(parm) 0
#define dma_flushWriteCache(parm,parm2)
#define dma_flushReadCache(parm,parm2)
#define dma_flushReadCacheWidthNEQLineOffset(parm,parm2)


#define pll_enableClock(x)
#define pll_disableClock(x)
#define pll_setClockRate(x,y)
#define pll_getPLLEn(x,y)
#define pll_setPLLEn(x)


#else

#include "Type.h"
#include "ErrorNo.h"
#include "Debug.h"
#include "dma.h"


#endif  // end #ifdef
*/

#include "ime_reg.h"
#include "ime_int_public.h"

#include "ime_control_base.h"
#include "ime_in_path_base.h"
#include "ime_out_path1_base.h"
#include "ime_out_path2_base.h"
#include "ime_out_path3_base.h"
#include "ime_out_path4_base.h"

#include "ime_lca_base.h"
#include "ime_adas_base.h"
#include "ime_dbcs_base.h"
#include "ime_pm_base.h"
#include "ime_osd_base.h"
#include "ime_3dnr_base.h"
#include "ime_compression_base.h"
#include "ime_function_base.h"

#include "ime_lib.h"



#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Type.h"
#include "ErrorNo.h"
#include "Debug.h"
#include "dma.h"

#include "ime_int_public.h"

#include "ime_control_base.h"
#include "ime_in_path_base.h"
#include "ime_out_path1_base.h"
#include "ime_out_path2_base.h"
#include "ime_out_path3_base.h"
#include "ime_out_path4_base.h"

#include "ime_pm_base.h"
#include "ime_osd_base.h"
#include "ime_3dnr_base.h"
#include "ime_compression_base.h"
#include "ime_function_base.h"

#include "ime_lib.h"

#endif

#define IME_GET_MAX(a, b)   ((a >= b) ? a : b)
#define IME_GET_MIN(a, b)   ((a <= b) ? a : b)



#define IME_DBG_MSG  0

#define IME_DMA_CACHE_HANDLE    1


#define IME_STRIPE_UNIT      4
#define IME_H_OVERLAP        32
#define IME_V_OVERLAP        4


#define IME_SEL_INBUF0      0
#define IME_SEL_INBUF1      1



extern UINT32 ime_max_stp_isd_out_buf_size;
extern UINT32 ime_stp_size_max, ime_stp_num_max;

extern volatile NT98520_IME_REGISTER_STRUCT *imeg;


extern IME_OPMODE               ime_engine_operation_mode;
extern IME_HV_STRIPE_INFO       ime_stripe_info;
//extern IME_SCALE_FILTER_INFO    ime_scale_filter_params;

//extern IME_SCALE_FACTOR_INFO    ime_scale_factor_path1_params;
//extern IME_SCALE_FACTOR_INFO    ime_scale_factor_path2_params;
//extern IME_SCALE_FACTOR_INFO    ime_scale_factor_path3_params;
//extern IME_SCALE_FACTOR_INFO    ime_scale_factor_path4_params;
//extern IME_SCALE_FACTOR_INFO    ime_scale_factor_path5_params;

//extern IME_DMA_ADDR_INFO        ime_input_buffer_addr, ime_output_buffer_addr;
//extern IME_DMA_ADDR_INFO        ime_tmp_addr0, ime_tmp_addr1;
//extern IME_LINEOFFSET_INFO      ime_lofs_params;

//extern IME_SIZE_INFO ime_input_size, ime_output_size;
extern IME_SIZE_INFO ime_output_size_path1, ime_output_size_path2, ime_output_size_path3, ime_output_size_path4, ime_output_size_path5;
extern UINT32 ime_tmp_input_size, ime_tmp_output_size_path1, ime_tmp_output_size_path2, ime_tmp_output_size_path3, ime_tmp_output_size_path4, ime_tmp_output_size_path5, ime_tmp_output_size_max;

//extern IME_SSR_MODE_SEL g_SsrMode;

typedef enum _IME_INSRC_SEL {
	IME_INSRC_IPE   = 0,         ///< input from IPE
	IME_INSRC_DRAM  = 1,         ///< input from DRAM
	ENUM_DUMMY4WORD(IME_INSRC_SEL)
} IME_INSRC_SEL;



typedef enum _IME_SRC_CTRL_SEL {
	SRC_CTRL_IME = 0,         ///< input port control by IME input source
	SRC_CTRL_DCE = 1,         ///< input port control by DCE
	ENUM_DUMMY4WORD(IME_SRC_CTRL_SEL)
} IME_SRC_CTRL_SEL;


/*
    @name IME enum - Engine DMA direction selecion
*/
typedef enum _IME_DMA_DIRECTIOIN {
	IME_DMA_INPUT = 0,   ///< input DMA direction
	IME_DMA_OUTPUT = 1,  ///< output DMA direction
	ENUM_DUMMY4WORD(IME_DMA_DIRECTIOIN)
} IME_DMA_DIRECTIOIN;


/*
    @name IME enum - Engine lock status selection
*/
typedef enum _IME_USED_STATUS {
	IME_ENGINE_UNLOCKED = 0,  ///< input DMA direction
	IME_ENGINE_LOCKED = 1,    ///< output DMA direction
	ENUM_DUMMY4WORD(IME_USED_STATUS)
} IME_USED_STATUS;


/*
    @name IME enum - Status of State Machine
*/
typedef enum _IME_STATE_MACHINE {
	IME_ENGINE_IDLE  = 0,        ///< Idel status
	IME_ENGINE_READY = 1,        ///< Ready status
	IME_ENGINE_PAUSE = 2,        ///< Pause status
	IME_ENGINE_RUN   = 3,        ///< Run status
	//IME_ENGINE_READY2RUN = 4,    ///< ready to Run status
	ENUM_DUMMY4WORD(IME_STATE_MACHINE)
} IME_STATE_MACHINE;


/**
    @name IME enum - Operations of engine action
*/
typedef enum _IME_ACTION_OP {
	IME_ACTOP_OPEN          = 0,    ///< Open engine
	IME_ACTOP_CLOSE         = 1,    ///< Close engine
	IME_ACTOP_START         = 2,    ///< Start engine
	IME_ACTOP_PARAM         = 3,    ///< Set parameter
	IME_ACTOP_PAUSE         = 4,    ///< Pause engine
	//IME_ACTOP_SWRESET       = 5,    ///< Software reset
	IME_ACTOP_CHGCLOCK      = 6,    ///< Change clock rate
	IME_ACTOP_DYNAMICPARAM  = 7,    ///< Change dynamic parameter
	//IME_ACTOP_TRIGGERSLICE  = 8,    ///< Slice trigger
	ENUM_DUMMY4WORD(IME_ACTION_OP)
} IME_ACTION_OP;


/**
    @name IME enum - Engine Stripe mode selection
*/
typedef enum _IME_STRIPE_MODE_SEL {
	IME_SINGLE_STRIPE = 0,  ///< Single stripe
	IME_MULTI_STRIPE = 1,   ///< Multi stripe
	ENUM_DUMMY4WORD(IME_STRIPE_MODE_SEL)
} IME_STRIPE_MODE_SEL;



typedef enum _IME_ENGEIN_SET_CTRL {
	IME_ENGINE_SET_RESET   = 0,  // set reset
	IME_ENGINE_SET_START   = 2,  // set start
	IME_ENGINE_SET_LOAD    = 3,  // set load
	IME_ENGINE_SET_INSRC   = 4,  // set input source
	IME_ENGINE_SET_DRCL    = 5,  // set input port control when direct mode
	IME_ENGINE_SET_INTPS   = 6,  // set interrupt status
	IME_ENGINE_SET_INTPE   = 7,  // set interrupt enable
	ENUM_DUMMY4WORD(IME_ENGEIN_SET_CTRL)
} IME_ENGEIN_SET_CTRL;


typedef enum _IME_ENGEIN_GET_CTRL {
	IME_ENGINE_GET_INTP         = 0,  // get interrupt status
	IME_ENGINE_GET_INTPE        = 1,  // get interrupt enable
	//IME_ENGINE_GET_LOCKSTATUS   = 2,  // get locked status
	ENUM_DUMMY4WORD(IME_ENGEIN_GET_CTRL)
} IME_ENGEIN_GET_CTRL;


/**
    IME enum - privacy mask type selection
*/
typedef enum _IME_PM_OPR {
	IME_PM_OPR_LARGER_EQUAL = 0,  ///< larger than and equal to
	IME_PM_OPR_LESS_EQUAL   = 1,  ///< less than and equal to
	IME_PM_OPR_LARGER       = 2,  ///< larger than and equal to
	IME_PM_OPR_LESS         = 3,  ///< less than and equal to
	ENUM_DUMMY4WORD(IME_PM_OPR)
} IME_PM_OPR;



typedef struct _IME_ISD_FACTOR_ {
	UINT32 uiFactor;
	UINT32 uiCnt;
} IME_ISD_FACTOR;

/**
    IME structure - privacy mask coordinate
*/
typedef struct _IME_POINT {
#if defined (__KERNEL__) || defined (__FREERTOS)

	UINT32 pnt_x;
	UINT32 pnt_y;
	UINT32 pnt_dg;

#else

	FLOAT pnt_x;
	FLOAT pnt_y;
	FLOAT pnt_dg;

#endif
} IME_POINT;

typedef enum _IME_OUTPUT_BUF_SEL_ {
	IME_OUT_P1_Y  = 0,
	IME_OUT_P1_U,
	IME_OUT_P1_V,
	IME_OUT_P1S_Y,
	IME_OUT_P1S_U,
	IME_OUT_P2_Y,
	IME_OUT_P2_U,
	IME_OUT_P2S_Y,
	IME_OUT_P2S_U,
	IME_OUT_P3_Y,
	IME_OUT_P3_U,
	IME_OUT_P3S_Y,
	IME_OUT_P3S_U,
	IME_OUT_P4_Y,
	IME_OUT_P4_E,
	IME_OUT_P4_H,

	IME_OUT_LCASUBOUT,
	IME_OUT_TMNR_REF_Y,
	IME_OUT_TMNR_REF_U,
	IME_OUT_TMNR_MV,
	IME_OUT_TMNR_MS,
	IME_OUT_TMNR_MSROI,
	IME_OUT_TMNR_STA,

	IME_OUT_BUF_MAX,
	ENUM_DUMMY4WORD(IME_OUTPUT_BUF_SEL)
} IME_OUTPUT_BUF_SEL;

typedef struct _IME_OUTPUT_BUF_INFO_ {
	BOOL flush_en;

	UINT32 buf_addr;
	UINT32 buf_size;
} IME_OUTPUT_BUF_INFO;


//================================================================================

extern IME_OUTPUT_BUF_INFO ime_out_buf_flush[IME_OUT_BUF_MAX];

extern ER ime_check_state_machine(IME_STATE_MACHINE current_status, IME_ACTION_OP action_op);

extern IME_H_STRIPE_OVLP_SEL ime_cal_overlap_size(IME_SIZE_INFO *p_in_size, IME_SIZE_INFO *p_out_size);
extern VOID ime_cal_hv_strip_param(UINT32 in_width, UINT32 in_height, UINT32 stripe_size, IME_HV_STRIPE_INFO *p_strip_param);
extern IME_OPMODE ime_get_operate_mode(VOID);
extern VOID ime_cal_convex_hull_coefs(IME_PM_POINT *p_pm_cvx_point, INT32 pnt_num, INT32 *p_coefs_line1, INT32 *p_coefs_line2, INT32 *p_coefs_line3, INT32 *p_coefs_line4);


// engine control APIs

//extern VOID ime_set_engine_control(IME_ENGEIN_SET_CTRL set_sel, UINT32 set_val);
//extern UINT32 ime_get_engine_control(IME_ENGEIN_GET_CTRL get_sel);
extern VOID ime_set_init(VOID);


//------------------------------------------------------------------------
// check and translate API
extern UINT32 ime_input_dma_buf_flush(UINT32 in_addr, UINT32 in_size, IME_OPMODE op_mode);
extern UINT32 ime_output_dma_buf_flush(UINT32 out_addr, UINT32 out_size, BOOL flash_type, IME_BUF_FLUSH_SEL is_dma_flush, IME_OPMODE op_mode);
//extern ER ime_check_dma_addr(UINT32 addr_y, UINT32 addr_u, UINT32 addr_v, IME_DMA_DIRECTIOIN in_out_sel);
extern ER ime_check_lineoffset(UINT32 lofs_y, UINT32 lofs_c, BOOL zero_check_en);
extern ER ime_check_input_path_stripe(VOID);
//extern ER ime_check_stripe_mode_data_stamp(IME_DS_SETNUM set_num);
//extern ER ime_check_stripe_mode_statistical(VOID);
//extern ER ime_check_stripe_mode_stitching(IME_PATH_SEL set_path);


//------------------------------------------------------------------------
// input APIs
//extern VOID ime_set_in_path_source(IME_INSRC_SEL in_src);
//extern VOID ime_set_in_path_direct_ctrl(IME_SRC_CTRL_SEL drct_ctrl);
//extern ER ime_set_in_path_stripe_info(IME_HV_STRIPE_INFO *p_set_stripe);
//extern VOID ime_set_in_path_image_format(IME_INPUT_FORMAT_SEL set_in_fmt);
//extern ER ime_set_in_path_image_size(IME_SIZE_INFO *p_set_img_size);
//extern ER ime_set_in_path_lineoffset(IME_LINEOFFSET_INFO *p_set_lofs);
//extern ER ime_set_in_path_dma_addr(IME_DMA_ADDR_INFO *p_set_addr);

extern VOID ime_set_in_path_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL bst_u, IME_BURST_SEL bst_v);

//extern VOID ime_get_in_path_source(IME_INSRC_SEL *p_get_src);
//extern VOID ime_get_in_path_direct_ctrl(IME_SRC_CTRL_SEL *p_get_drct_ctrl);
//extern VOID ime_get_in_path_image_format(IME_INPUT_FORMAT_SEL *p_get_fmt);
//extern VOID ime_get_in_path_image_size(IME_SIZE_INFO *p_set_img_size);
//extern VOID ime_get_in_path_lineoffset(IME_LINEOFFSET_INFO *p_get_lofs);
//extern VOID ime_get_in_path_dma_addr(IME_DMA_ADDR_INFO *p_get_addr);
//extern VOID ime_get_in_path_stripe_info(IME_HV_STRIPE_INFO *p_get_stripe_size);
//extern IME_STRIPE_MODE_SEL ime_get_in_path_stripe_mode(VOID);

//extern UINT32 ime_get_function0_enable(VOID);
//extern UINT32 ime_get_function1_enable(VOID);


//------------------------------------------------------------------------
// output Path1 APIs
//extern VOID ime_set_out_path_p1_enable(IME_FUNC_EN set_path_en);
//extern VOID ime_set_out_path_image_format_p1(IME_OUTPUT_FORMAT_TYPE set_type, IME_OUTPUT_IMG_FORMAT_SEL set_fmt);
//extern VOID ime_set_out_path_scale_method_p1(IME_SCALE_METHOD_SEL set_scl_method);
//extern ER ime_set_out_path_scale_factor_p1(IME_SCALE_FACTOR_INFO *p_set_scl_factor);
//extern ER ime_set_out_path_scale_filter_p1(IME_SCALE_FILTER_INFO *p_set_scl_filter);
//extern ER ime_set_out_path_scale_size_p1(IME_SIZE_INFO *p_set_size);
//extern ER ime_set_out_path_output_crop_size_p1(IME_SIZE_INFO *p_set_scale_size, IME_SIZE_INFO *p_set_out_size, IME_POS_INFO *p_set_crop_pos);
//extern ER ime_set_out_path_lineoffset_p1(IME_LINEOFFSET_INFO *p_set_lofs);
//extern ER ime_set_out_path_dma_addr_p1(IME_DMA_ADDR_INFO *p_set_addr);
extern VOID ime_set_out_path_p1_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL bst_u, IME_BURST_SEL bst_v);
//extern VOID ime_set_encode_enable_p1(IME_FUNC_EN set_en);
//extern ER ime_set_encode_side_info_addr_p1(UINT32 addr);
//extern UINT32 ime_get_encode_side_info_addr_p1(VOID);
//extern VOID ime_set_encode_ktable_p1(IME_CODEC_KT_INFO *pSetKTab);
//extern VOID ime_get_encode_ktable_p1(IME_CODEC_KT_INFO *pGetKTab);
//extern VOID ime_set_encode_side_info_fmt_p1(IME_SINFO_FMT *pSInfoFmt);
//extern VOID ime_set_encode_ktable_ctrl_p1(IME_FUNC_EN ClearEn, IME_FUNC_EN UpdateEn);

//extern VOID ime_get_out_path_enable_p1(IME_FUNC_EN *p_get_path_en);
//extern VOID ime_get_out_path_format_type_p1(IME_OUTPUT_FORMAT_TYPE *p_get_type);
//extern VOID ime_get_out_path_image_format_p1(IME_OUTPUT_IMG_FORMAT_SEL *p_get_fmt);
//extern VOID ime_get_out_path_scale_size_p1(IME_SIZE_INFO *p_get_scale_size);
//extern VOID ime_get_out_path_output_size_p1(IME_SIZE_INFO *p_get_out_size);
//extern VOID ime_get_out_path_lineoffset_p1(IME_LINEOFFSET_INFO *p_get_lofs);
//extern VOID ime_get_out_path_lineoffset2_p1(IME_LINEOFFSET_INFO *p_get_lofs);
//extern VOID ime_get_out_path_dma_addr_p1(IME_DMA_ADDR_INFO *p_get_addr);
//extern VOID ime_get_out_path_dma_addr2_p1(IME_DMA_ADDR_INFO *p_get_addr);
//extern VOID ime_get_out_path_scale_method_p1(IME_SCALE_METHOD_SEL *p_get_method);
//extern VOID ime_get_out_path_sprt_out_enable_p1(IME_FUNC_EN *p_get_spr_en);
//extern VOID ime_get_out_path_out_dest_p1(IME_OUTDST_CTRL_SEL *p_get_out_dest);
//extern VOID ime_get_out_path_encoder_enable_p1(IME_FUNC_EN *p_get_en);
//extern VOID ime_get_out_path_crop_start_pos_p1(IME_POS_INFO *p_get_pos);

//------------------------------------------------------------------------
// output Path2 APIs
//extern VOID ime_set_out_path_p2_enable(IME_FUNC_EN set_path_en);
//extern VOID ime_set_out_path_image_format_p2(IME_OUTPUT_FORMAT_TYPE set_type, IME_OUTPUT_IMG_FORMAT_SEL set_fmt);
//extern VOID ime_set_out_path_scale_method_p2(IME_SCALE_METHOD_SEL set_scl_method);
//extern ER ime_set_out_path_scale_factor_p2(IME_SCALE_FACTOR_INFO *p_set_scl_factor);
//extern ER ime_set_out_path_isd_scale_factor_p2(IME_SCALE_FACTOR_INFO *p_set_scl_factor);
//extern ER ime_set_out_path_scale_filter_p2(IME_SCALE_FILTER_INFO *p_set_scl_filter);
//extern ER ime_set_out_path_scale_size_p2(IME_SIZE_INFO *p_set_size);
//extern ER ime_set_out_path_output_crop_size_p2(IME_SIZE_INFO *p_set_scale_size, IME_SIZE_INFO *p_set_out_size, IME_POS_INFO *p_set_crop_pos);
//extern ER ime_set_out_path_lineoffset_p2(IME_LINEOFFSET_INFO *p_set_lofs);
//extern ER ime_set_out_path_dma_addr_p2(IME_DMA_ADDR_INFO *p_set_addr);
extern VOID ime_set_out_path_p2_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL bst_u);

//extern VOID ime_get_out_path_enable_p2(IME_FUNC_EN *p_get_path_en);
//extern VOID ime_get_out_path_format_type_p2(IME_OUTPUT_FORMAT_TYPE *p_get_type);
//extern VOID ime_get_out_path_image_format_p2(IME_OUTPUT_IMG_FORMAT_SEL *p_get_fmt);
//extern VOID ime_get_out_path_scale_size_p2(IME_SIZE_INFO *p_get_scale_size);
//extern VOID ime_get_out_path_output_size_p2(IME_SIZE_INFO *p_get_out_size);
//extern VOID ime_get_out_path_lineoffset_p2(IME_LINEOFFSET_INFO *p_get_lofs);
//extern VOID ime_get_out_path_lineoffset2_p2(IME_LINEOFFSET_INFO *p_get_lofs);
//extern VOID ime_get_out_path_dma_addr_p2(IME_DMA_ADDR_INFO *p_get_addr);
//extern VOID ime_get_out_path_dma_addr2_p2(IME_DMA_ADDR_INFO *p_get_addr);
//extern VOID ime_get_out_path_scale_method_p2(IME_SCALE_METHOD_SEL *p_get_method);
//extern VOID ime_get_out_path_sprt_out_enable_p2(IME_FUNC_EN *p_get_spr_en);
//extern VOID ime_get_out_path_crop_start_pos_p2(IME_POS_INFO *p_get_pos);



//------------------------------------------------------------------------
// output Path3 APIs
//extern VOID ime_set_out_path_p3_enable(IME_FUNC_EN set_path_en);
//extern VOID ime_set_out_path_image_format_p3(IME_OUTPUT_FORMAT_TYPE set_type, IME_OUTPUT_IMG_FORMAT_SEL set_fmt);
//extern VOID ime_set_out_path_scale_method_p3(IME_SCALE_METHOD_SEL set_scl_method);
//extern ER ime_set_out_path_scale_factor_p3(IME_SCALE_FACTOR_INFO *p_set_scl_factor);
//extern ER ime_set_out_path_isd_scale_factor_p3(IME_SCALE_FACTOR_INFO *p_set_scl_factor);
//extern ER ime_set_out_path_scale_filter_p3(IME_SCALE_FILTER_INFO *p_set_scl_filter);
//extern ER ime_set_out_path_scale_size_p3(IME_SIZE_INFO *p_set_size);
//extern ER ime_set_out_path_output_crop_size_p3(IME_SIZE_INFO *p_set_scale_size, IME_SIZE_INFO *p_set_out_size, IME_POS_INFO *p_set_crop_pos);
//extern ER ime_set_out_path_lineoffset_p3(IME_LINEOFFSET_INFO *p_set_lofs);
//extern ER ime_set_out_path_dma_addr_p3(IME_DMA_ADDR_INFO *p_set_addr);
extern VOID ime_set_out_path_p3_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL bst_u);


//extern VOID ime_get_out_path_enable_p3(IME_FUNC_EN *p_get_path_en);
//extern VOID ime_get_out_path_format_type_p3(IME_OUTPUT_FORMAT_TYPE *p_get_type);
//extern VOID ime_get_out_path_image_format_p3(IME_OUTPUT_IMG_FORMAT_SEL *p_get_fmt);
//extern VOID ime_get_out_path_scale_size_p3(IME_SIZE_INFO *p_get_scale_size);
//extern VOID ime_get_out_path_output_size_p3(IME_SIZE_INFO *p_get_out_size);
//extern VOID ime_get_out_path_lineoffset_p3(IME_LINEOFFSET_INFO *p_get_lofs);
//extern VOID ime_get_out_path_lineoffset2_p3(IME_LINEOFFSET_INFO *p_get_lofs);
//extern VOID ime_get_out_path_dma_addr_p3(IME_DMA_ADDR_INFO *p_get_addr);
//extern VOID ime_get_out_path_dma_addr2_p3(IME_DMA_ADDR_INFO *p_get_addr);
//extern VOID ime_get_out_path_scale_method_p3(IME_SCALE_METHOD_SEL *p_get_method);
//extern VOID ime_get_out_path_sprt_out_enable_p3(IME_FUNC_EN *p_get_spr_en);
//extern VOID ime_get_out_path_crop_start_pos_p3(IME_POS_INFO *p_get_pos);


//------------------------------------------------------------------------
// output Path4 APIs
//extern VOID ime_set_out_path_p4_enable(IME_FUNC_EN set_path_en);
//extern VOID ime_set_out_path_image_format_p4(IME_OUTPUT_FORMAT_TYPE set_type, IME_OUTPUT_IMG_FORMAT_SEL set_fmt);
//extern VOID ime_set_out_path_scale_method_p4(IME_SCALE_METHOD_SEL set_scl_method);
//extern ER ime_set_out_path_scale_factor_p4(IME_SCALE_FACTOR_INFO *p_set_scl_factor);
//extern ER ime_set_out_path_scale_filter_p4(IME_SCALE_FILTER_INFO *p_set_scl_filter);
//extern ER ime_set_out_path_scale_size_p4(IME_SIZE_INFO *p_set_size);
//extern ER ime_set_out_path_output_crop_size_p4(IME_SIZE_INFO *p_set_scale_size, IME_SIZE_INFO *p_set_out_size, IME_POS_INFO *p_set_crop_pos);
//extern ER ime_set_out_path_lineoffset_p4(IME_LINEOFFSET_INFO *p_set_lofs);
//extern ER ime_set_out_path_dma_addr_p4(IME_DMA_ADDR_INFO *p_set_addr);
extern VOID ime_set_out_path_p4_burst_size(IME_BURST_SEL bst_y);

//extern VOID ime_get_out_path_enable_p4(IME_FUNC_EN *p_get_path_en);
//extern VOID ime_get_out_path_format_type_p4(IME_OUTPUT_FORMAT_TYPE *p_get_type);
//extern VOID ime_get_out_path_image_format_p4(IME_OUTPUT_IMG_FORMAT_SEL *p_get_fmt);
//extern VOID ime_get_out_path_scale_size_p4(IME_SIZE_INFO *p_get_scale_size);
//extern VOID ime_get_out_path_output_size_p4(IME_SIZE_INFO *p_get_out_size);
//extern VOID ime_get_out_path_lineoffset_p4(IME_LINEOFFSET_INFO *p_get_lofs);
//extern VOID ime_get_out_path_dma_addr_p4(IME_DMA_ADDR_INFO *p_get_addr);
//extern VOID ime_get_out_path_scale_method_p4(IME_SCALE_METHOD_SEL *p_get_method);
//extern VOID ime_get_out_path_crop_start_pos_p4(IME_POS_INFO *p_get_pos);

//------------------------------------------------------------------------
// output Path5 APIs
#if 0
extern VOID ime_set_out_path_p5_enable(IME_FUNC_EN SetPathEn);
extern VOID ime_set_out_path_dma_output_enable_p5(IME_FUNC_EN SetPathEn);
extern VOID ime_set_out_path_image_format_p5(IME_OUTPUT_FORMAT_TYPE set_type, IME_OUTPUT_IMG_FORMAT_SEL set_fmt);
extern VOID ime_set_out_path_scale_method_p5(IME_SCALE_METHOD_SEL set_scl_method);
extern ER ime_set_out_path_enhance_p5(IME_SCALE_ENH_INFO *pSetFactor);
extern ER ime_set_out_path_scale_factor_p5(IME_SCALE_FACTOR_INFO *p_set_scl_factor);
extern ER ime_set_out_path_scale_filter_p5(IME_SCALE_FILTER_INFO *p_set_scl_filter);
extern ER ime_set_out_path_scale_size_p5(IME_SIZE_INFO *p_set_size);
extern ER ime_set_out_path_output_crop_size_p5(IME_SIZE_INFO *p_set_scale_size, IME_SIZE_INFO *p_set_out_size, IME_POS_INFO *p_set_crop_pos);
extern ER ime_set_out_path_lineoffset_p5(IME_LINEOFFSET_INFO *p_set_lofs);
extern ER ime_set_out_path_dma_addr_p5(IME_DMA_ADDR_INFO *p_set_addr);
extern VOID ime_set_out_path_p5_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL bst_u);


extern VOID ime_get_out_path_enable_p5(IME_FUNC_EN *p_get_path_en);
extern VOID ime_get_out_path_format_type_p5(IME_OUTPUT_FORMAT_TYPE *p_get_type);
extern VOID ime_get_out_path_image_format_p5(IME_OUTPUT_IMG_FORMAT_SEL *p_get_fmt);
extern VOID ime_get_out_path_scale_size_p5(IME_SIZE_INFO *p_get_out_size);
extern VOID ime_get_out_path_output_size_p5(IME_SIZE_INFO *p_get_out_size);
extern VOID ime_get_out_path_lineoffset_p5(IME_LINEOFFSET_INFO *p_get_lofs);
extern VOID ime_get_out_path_lineoffset2_p5(IME_LINEOFFSET_INFO *p_get_lofs);
extern VOID ime_get_out_path_dma_addr_p5(IME_DMA_ADDR_INFO *p_get_addr);
extern VOID ime_get_out_path_dma_addr2_p5(IME_DMA_ADDR_INFO *p_get_addr);
extern VOID ime_get_out_path_scale_method_p5(IME_SCALE_METHOD_SEL *p_get_method);
extern VOID ime_get_out_path_sprt_out_enable_p5(IME_FUNC_EN *p_get_spr_en);
extern VOID ime_get_out_path_crop_start_pos_p5(IME_POS_INFO *p_get_pos);
#endif


//==========================================================================
// IQ function APIs

//------------------------------------------------------------------------
// chroma median filter
//extern VOID ime_set_cmf_enable(IME_FUNC_EN set_en);

//------------------------------------------------------------------------
// Image Local Adaptation
//extern ER ime_set_lca_enable(IME_FUNC_EN set_en);
//extern VOID ime_set_lca_chroma_adj_enable(IME_FUNC_EN set_en);
//extern VOID ime_set_lca_luma_enable(IME_FUNC_EN set_en);

extern ER ime_set_lca_input_image_info(IME_CHROMA_ADAPTION_IMAGE_INFO *p_set_info);
//extern ER ime_set_lca_chroma_adjust_info(IME_CHROMA_ADAPTION_CA_INFO *p_set_info);
//extern ER ime_set_lca_chroma_adapt_info(IME_CHROMA_ADAPTION_IQC_INFO *p_set_info);
//extern ER ime_set_lca_luma_suppress_info(IME_CHROMA_ADAPTION_IQL_INFO *p_set_info);
extern VOID ime_set_in_path_lca_burst_size(IME_BURST_SEL bst);
extern VOID ime_set_out_path_lca_burst_size(IME_BURST_SEL bst);
extern ER ime_set_lca_scale_factor(VOID);

//extern IME_FUNC_EN ime_get_ila_enable(VOID);
//extern VOID ime_get_lca_image_size_info(IME_SIZE_INFO *p_get_info);
//extern VOID ime_get_lca_lineoffset_info(IME_LINEOFFSET_INFO *p_get_info);
//extern VOID ime_get_lca_dma_addr_info(IME_DMA_ADDR_INFO *p_get_info_set0, IME_DMA_ADDR_INFO *p_get_info_set1);


//------------------------------------------------------------------------
// LCA subout
//extern ER ime_set_lca_subout_enable(IME_FUNC_EN set_en);
//extern UINT32 ime_get_lca_subout_enable(VOID);
//extern VOID ime_set_lca_subout_src(IME_LCA_SUBOUT_SRC set_src);

//extern ER ime_set_lca_subout_scale_factor(IME_SCALE_FACTOR_INFO *p_set_scl_factor);
//extern ER ime_set_lca_subout_lineoffset(UINT32 lofs);
//extern UINT32 ime_get_lca_subout_lineoffset(VOID);
//extern ER ime_set_lca_subout_dma_addr(UINT32 addr);
//extern UINT32 ime_get_lca_subout_dma_addr(VOID);
extern ER ime_set_lca_subout(IME_CHROMA_APAPTION_SUBOUT_PARAM *p_set_info);




//------------------------------------------------------------------------
// dark and bright region chroma suppression
//extern VOID ime_set_dbcs_enable(IME_FUNC_EN set_en);
//extern IME_FUNC_EN ime_get_dbcs_enable(VOID);
//extern ER ime_set_dbcs_iq_info(IME_DBCS_IQ_INFO *p_set_info);

//-----------------------------------------------------------------------
// CST
//extern VOID ime_set_cst_enable(IME_CST_INFO *SetCstEn);

//------------------------------------------------------------------------
// SSR
//extern VOID ime_set_ssr_enable(IME_FUNC_EN set_en);
//extern IME_FUNC_EN ime_get_ssr_enable(VOID);
//extern ER ime_set_ssr_iq_info(IME_SSR_IQ_INFO *p_set_info);
//extern ER ime_set_scale_factor_with_ssr(VOID);

//------------------------------------------------------------------------
// grain noise
#if 0  // 520 do not support
extern VOID ime_set_gn_enable(IME_FUNC_EN set_en);
extern ER ime_set_gn_iq_info(IME_FILM_GRAIN_IQ_INFO *p_set_info);
#endif

//------------------------------------------------------------------------
// P2I filter
//extern VOID ime_set_p2i_enable(IME_FUNC_EN set_en);


//------------------------------------------------------------------------
// data stamp

//extern VOID ime_set_ds_cst_enable(IME_FUNC_EN set_en);

// data stamp set0
//extern ER ime_set_osd_set0_image_info(IME_STAMP_IMAGE_INFO *p_set_info);
//extern ER ime_set_osd_set0_iq_info(IME_STAMP_IQ_INFO *p_set_info);
//extern VOID ime_get_osd_set0_image_size(IME_SIZE_INFO *p_get_info);
//extern VOID ime_get_osd_set0_lineoffset(IME_LINEOFFSET_INFO *p_get_info);
//extern VOID ime_get_osd_set0_dma_addr(IME_DMA_ADDR_INFO *p_get_info);
//extern IME_FUNC_EN ime_get_osd_set0_enable(VOID);
extern VOID ime_set_osd_set0_burst_size(IME_BURST_SEL bst);

//------------------------------------------------------------------------
// data stamp set1
//extern ER ime_set_osd_set1_image_info(IME_STAMP_IMAGE_INFO *p_set_info);
//extern ER ime_set_osd_set1_iq_info(IME_STAMP_IQ_INFO *p_set_info);
//extern VOID ime_get_osd_set1_image_size(IME_SIZE_INFO *p_get_info);
//extern VOID ime_get_osd_set1_lineoffset(IME_LINEOFFSET_INFO *p_get_info);
//extern VOID ime_get_osd_set1_dma_addr(IME_DMA_ADDR_INFO *p_get_info);
//extern IME_FUNC_EN ime_get_osd_set1_enable(VOID);


//------------------------------------------------------------------------
// data stamp set2
//extern ER ime_set_osd_set2_image_info(IME_STAMP_IMAGE_INFO *p_set_info);
//extern ER ime_set_osd_set2_iq_info(IME_STAMP_IQ_INFO *p_set_info);
//extern VOID ime_get_osd_set2_image_size(IME_SIZE_INFO *p_get_info);
//extern VOID ime_get_osd_set2_lineoffset(IME_LINEOFFSET_INFO *p_get_info);
//extern VOID ime_get_osd_set2_dma_addr(IME_DMA_ADDR_INFO *p_get_info);
//extern IME_FUNC_EN ime_get_osd_set2_enable(VOID);



//------------------------------------------------------------------------
// data stamp set3
//extern ER ime_set_osd_set3_image_info(IME_STAMP_IMAGE_INFO *p_set_info);
//extern ER ime_set_osd_set3_iq_info(IME_STAMP_IQ_INFO *p_set_info);
//extern VOID ime_get_osd_set3_image_size(IME_SIZE_INFO *p_get_info);
//extern VOID ime_get_osd_set3_lineoffset(IME_LINEOFFSET_INFO *p_get_info);
//extern VOID ime_get_osd_set3_dma_addr(IME_DMA_ADDR_INFO *p_get_info);
//extern IME_FUNC_EN ime_get_osd_set3_enable(VOID);


//------------------------------------------------------------------------
// data stamp color lut
//extern ER ime_set_osd_cst_coef(UINT32 coef0, UINT32 coef1, UINT32 coef2, UINT32 coef3);
//extern ER ime_set_osd_color_lut_info(UINT32 *p_color_lut_y, UINT32 *p_color_lut_u, UINT32 *p_color_lut_v);

//------------------------------------------------------------------------
// privacy mask
extern VOID ime_set_privacy_mask_enable(IME_PM_SET_SEL set_sel, IME_FUNC_EN set_en);
extern VOID ime_set_privacy_mask_type(IME_PM_SET_SEL set_sel, IME_PM_MASK_TYPE set_type);
extern VOID ime_set_privacy_mask_color(IME_PM_SET_SEL set_sel, IME_PM_MASK_COLOR set_color);
extern VOID ime_set_privacy_mask_weight(IME_PM_SET_SEL set_sel, UINT32 set_wet);
extern VOID ime_set_privacy_mask_line0(IME_PM_SET_SEL set_sel, INT32 *p_coefs);
extern VOID ime_set_privacy_mask_line1(IME_PM_SET_SEL set_sel, INT32 *p_coefs);
extern VOID ime_set_privacy_mask_line2(IME_PM_SET_SEL set_sel, INT32 *p_coefs);
extern VOID ime_set_privacy_mask_line3(IME_PM_SET_SEL set_sel, INT32 *p_coefs);

//extern VOID ime_set_privacy_mask_pxl_image_size(UINT32 size_h, UINT32 size_v);
//extern VOID ime_set_privacy_mask_pxl_image_format(UINT32 fmt);
//extern VOID ime_set_privacy_mask_pxl_block_size(UINT32 blk_size);
extern VOID ime_set_privacy_mask_burst_size(IME_BURST_SEL bst);
//extern VOID ime_set_privacy_mask_pxl_image_lineoffset(UINT32 lofs);
//extern VOID ime_set_privacy_mask_pxl_image_dma_addr(UINT32 addr);

//extern VOID ime_get_privacy_mask_pxl_image_size(UINT32 *p_get_size_h, UINT32 *p_get_size_v);
//extern VOID ime_get_privacy_mask_pxl_image_lineoffset(UINT32 *p_get_lofs);
//extern VOID ime_get_privacy_mask_pxl_image_dma_addr(UINT32 *p_get_addr);

//extern IME_FUNC_EN ime_get_privacy_mask_enable(IME_PM_SET_SEL set_sel);


//------------------------------------------------------------------------
// ADAS
//extern VOID ime_set_adas_enable(IME_FUNC_EN set_en);
//extern VOID ime_set_adas_median_filter_enable(IME_FUNC_EN set_en);
//extern VOID ime_set_adas_image_out_sel(IME_STL_IMGOUT_SEL img_out_sel);
//extern ER ime_set_adas_edge_kernel(IME_STL_EDGE_INFO *p_set_edge_ker_info);
//extern ER ime_set_adas_histogram(IME_STL_HIST_INFO *p_set_hist_info);
//extern VOID ime_set_adas_edge_map_lofs(UINT32 lofs);
//extern UINT32 ime_get_adas_edge_map_lofs(VOID);
//extern ER ime_set_adas_edge_map_addr(UINT32 addr);
//extern ER ime_set_adas_histogram_addr(UINT32 addr);
//extern ER ime_set_adas_roi(IME_STL_ROI_INFO *p_set_roi_info);
//extern VOID ime_set_adas_burst_length(VOID);
//extern VOID ime_get_adas_hitogram_info(IME_GET_STL_INFO *p_get_hist_info);

//extern VOID ime_get_adas_edge_map_addr(IME_DMA_ADDR_INFO *p_get_addr);
//extern VOID ime_get_adas_histogram_addr(IME_DMA_ADDR_INFO *p_get_addr);
//extern IME_FUNC_EN ime_get_adas_enable(VOID);

//------------------------------------------------------------------------
// YUV Converter
//extern VOID ime_set_yuv_converter_enable(IME_FUNC_EN set_en);
//extern VOID ime_set_yuv_converter_sel(IME_YUV_CVT set_cvt);
//extern UINT32 ime_get_yuv_converter_enable(VOID);
//extern UINT32 ime_get_yuv_converter_sel(VOID);

//------------------------------------------------------------------------
// stitching
//extern VOID ime_set_stitching_enable(IME_PATH_SEL path_sel, IME_FUNC_EN set_en);
//extern VOID ime_set_stitching_base_position(IME_PATH_SEL path_sel, UINT32 set_pos);
// extern VOID ime_set_stitching_lineoffset(IME_PATH_SEL path_sel, IME_LINEOFFSET_INFO *p_set_lofs);
//extern VOID ime_set_stitching_dma_addr(IME_PATH_SEL path_sel, IME_DMA_ADDR_INFO *p_set_addr);
//extern ER ime_check_stitching(IME_PATH_SEL path_sel);

//------------------------------------------------------------------------
// TMNR

extern VOID ime_set_in_path_3dnr_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL bst_c, IME_BURST_SEL bst_mv, IME_BURST_SEL bst_ms);
extern VOID ime_set_out_path_3dnr_burst_size(IME_BURST_SEL bst_y, IME_BURST_SEL BstC, IME_BURST_SEL BstMV, IME_BURST_SEL BstMO, IME_BURST_SEL BstMOROI, IME_BURST_SEL BstSta);



//------------------------------------------------------------------------
// integration APIs
extern ER ime_set_in_path(IME_INPATH_INFO *p_in_path_info);
extern ER ime_set_out_path_p1(IME_OUTPATH_INFO *p_out_path_info);
extern ER ime_set_out_path_p2(IME_OUTPATH_INFO *p_out_path_info);
extern ER ime_set_out_path_p3(IME_OUTPATH_INFO *p_out_path_info);
extern ER ime_set_out_path_p4(IME_OUTPATH_INFO *p_out_path_info);
//extern ER ime_set_out_path_p5(IME_OUTPATH_INFO *p_out_path_info);
extern INT32 ime_get_burst_size_info(UINT32 chl_num);

extern void ime_output_buffer_flush_dma_end(void);

//------------------------------------------------------------------------
// debug message API
//extern VOID ime_get_debug_message(VOID);
//------------------------------------------------------------------------


//------------------------------------------------------------------------------
extern ER ime_waitFlagFrameStart(IME_FLAG_CLEAR_SEL IsClearFlag);
extern VOID ime_clear_flag_frame_start(VOID);
//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif


#endif


