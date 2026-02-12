/**
    sensor common interface for ext_devices

    @file       ctl_sen_ext.h
    @ingroup    mIDrvSensor
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _CTL_SEN_EXT_H
#define _CTL_SEN_EXT_H

#include "ctl_sen.h"

typedef struct _SENSOR_DTSI_INFO {
	UINT8 *addr;
} SENSOR_DTSI_INFO;

typedef enum _CTL_SEN_MODE_TYPE_EXT {
	// 1 frame, get pdaf from raw plane 3.
	#if 0
	CTL_SEN_MODE_RAW_PDAF =         CTL_SEN_MODETYPE_FRM(out_type_sel_raw_pdaf, expt_gain_sel_expt, out_combine_sel_linear, 0),
	#endif

	// 1 frame, pwl & dcg strategy
	CTL_SEN_MODE_BUILTIN_DCG_HDR =  CTL_SEN_MODETYPE_FRM(out_type_sel_raw,      expt_gain_sel_dcg,  out_combine_sel_pwl, 0),


	// 2 frame, path 1: pwl, path 2: linear
	CTL_SEN_MODE_BUILTIN_DCG_SHDR = (CTL_SEN_MODETYPE_FRM(out_type_sel_raw, expt_gain_sel_expt, out_combine_sel_pwl, 0) | 
									(CTL_SEN_MODETYPE_FRM(out_type_sel_raw, expt_gain_sel_expt, out_combine_sel_linear, 0) << CTL_SEN_MODETYPE_FRMIDX1_OFS)),

	// 2 frame, get pdaf from raw plane 3
	#if 0
	CTL_SEN_MODE_STAGGER_PDAF =     (CTL_SEN_MODETYPE_FRM(out_type_sel_raw_pdaf, expt_gain_sel_expt, out_combine_sel_linear, 0) | 
									(CTL_SEN_MODETYPE_FRM(out_type_sel_raw_pdaf, expt_gain_sel_expt, out_combine_sel_linear, 0) << CTL_SEN_MODETYPE_FRMIDX1_OFS)),
	#endif
	// 2 frame, dcg strategy
	CTL_SEN_MODE_DCG_HDR =          (CTL_SEN_MODETYPE_FRM(out_type_sel_raw, expt_gain_sel_dcg, out_combine_sel_linear, 0) | 
									(CTL_SEN_MODETYPE_FRM(out_type_sel_raw, expt_gain_sel_dcg, out_combine_sel_linear, 0) << CTL_SEN_MODETYPE_FRMIDX1_OFS)),

	// 3 frame, path 1/2 dcg strategy
	#if 0
	CTL_SEN_MODE_DCG_SHDR =         (CTL_SEN_MODETYPE_FRM(out_type_sel_raw, expt_gain_sel_dcg, out_combine_sel_linear, 0) | 
									(CTL_SEN_MODETYPE_FRM(out_type_sel_raw, expt_gain_sel_dcg, out_combine_sel_linear, 0) << CTL_SEN_MODETYPE_FRMIDX1_OFS) | 
									(CTL_SEN_MODETYPE_FRM(out_type_sel_raw, expt_gain_sel_expt, out_combine_sel_linear, 0) << CTL_SEN_MODETYPE_FRMIDX2_OFS)),
	#endif

	// 3 frame
	#if 0
	CTL_SEN_MODE_STAGGER3_HDR =     (CTL_SEN_MODETYPE_FRM(out_type_sel_raw, expt_gain_sel_expt, out_combine_sel_linear, 0) | 
									(CTL_SEN_MODETYPE_FRM(out_type_sel_raw, expt_gain_sel_expt, out_combine_sel_linear, 0) << CTL_SEN_MODETYPE_FRMIDX1_OFS) | 
									(CTL_SEN_MODETYPE_FRM(out_type_sel_raw, expt_gain_sel_expt, out_combine_sel_linear, 0) << CTL_SEN_MODETYPE_FRMIDX2_OFS)),
	#endif
	ENUM_DUMMY4WORD(CTL_SEN_MODE_TYPE_EXT),
} CTL_SEN_MODE_TYPE_EXT;


typedef enum {
	CTL_SEN_EXT_MODESEL_VENDOR_TYPE_PD,

	ENUM_DUMMY4WORD(CTL_SEN_EXT_MODESEL_VENDOR_TYPE),
} CTL_SEN_EXT_MODESEL_VENDOR_TYPE;

typedef enum {
	CTL_SEN_EXT_MODESEL_VENDOR_PARAM_PD_INTERLEAVE,
	CTL_SEN_EXT_MODESEL_VENDOR_PARAM_PD_EMBEDDED,
	CTL_SEN_EXT_MODESEL_VENDOR_PARAM_PD_DATA_TYPE,
	CTL_SEN_EXT_MODESEL_VENDOR_PARAM_PD_VIRTUAL_CHANNEL,

	ENUM_DUMMY4WORD(CTL_SEN_EXT_MODESEL_VENDOR_PARAM),
} CTL_SEN_EXT_MODESEL_VENDOR_PARAM;

#endif
