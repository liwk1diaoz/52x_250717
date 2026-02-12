#ifndef _IQ_LIB_NVT_H_
#define _IQ_LIB_NVT_H_

#include "kwrap/type.h"

//=============================================================================
// struct & enum definition
//=============================================================================
#define IQ_MAX(a, b)         (((INT32)(a) > (INT32)(b)) ? (INT32)(a) : (INT32)(b))
#define IQ_MIN(a, b)         (((INT32)(a) < (INT32)(b)) ? (INT32)(a) : (INT32)(b))
#define IQ_CLAMP(in, lb, ub) (((INT32)(in) <= (INT32)(lb)) ? (INT32)(lb) : (((INT32)(in) >= (INT32)(ub)) ? (INT32)(ub) : (INT32)(in)))

#define IQ_DFG_AIRLIGHT_NUM 3

typedef struct _IQLIB_DEFOG {
	UINT16 fog_level;
	UINT8 outbld_lum_wt;
	UINT8 gain_th;
} IQLIB_DEFOG;

typedef struct _IQLIB_SHDR_FCURVE_INFO {
	BOOL dbg_en;
	UINT32 tm_ratio;
	UINT8 *idx_lut;     // length = IQ_SHDR_FCURVE_IDX_NUM
	UINT8 *split_lut;   // length = IQ_SHDR_FCURVE_SPLIT_NUM
	UINT16 *val_lut;    // length = IQ_SHDR_FCURVE_VAL_NUM
} IQLIB_SHDR_FCURVE_INFO;

typedef struct _IQLIB_SHDR_FCURVE {
	UINT16 *val_lut;    // length = IQ_SHDR_FCURVE_VAL_NUM
} IQLIB_SHDR_FCURVE;

//=============================================================================
// extern variable
//=============================================================================
extern UINT32 iq_fcurve_y_bound[257];

//=============================================================================
// extern functions
//=============================================================================
extern INT32 iq_intpl(INT32 index, INT32 l_value, INT32 h_value, INT32 l_index, INT32 h_index);
extern void iq_intpl_tbl_uint8(UINT8 *l_value, UINT8 *h_value, INT32 size, UINT8 *target, INT32 index, INT32 l_index, INT32 h_index);
extern void iq_intpl_tbl_uint16(UINT16 *l_value, UINT16 *h_value, INT32 size, UINT16 *target, INT32 index, INT32 l_index, INT32 h_index);
extern void iq_intpl_tbl_uint32(UINT32 *l_value, UINT32 *h_value, INT32 size, UINT32 *target, INT32 index, INT32 l_index, INT32 h_index);
extern UINT32 iq_lib_wdr_nvt(UINT32 id);
extern void iq_lib_defog_airlight_nvt(UINT32 id, UINT16 *out_airlight_r, UINT16 *out_airlight_g, UINT16 *out_airlight_b);
extern IQLIB_DEFOG iq_lib_defog_strength_nvt(UINT32 id, UINT16 dr_th, UINT16 fog_mod_level_lb);
extern void iq_lib_defog_fog_mod_level_to_lut_nvt(UINT32 id, UINT16 level, UINT16 *lut);
extern void iq_lib_shdr_fcurve_nvt(IQLIB_SHDR_FCURVE_INFO input, IQLIB_SHDR_FCURVE output);

#endif

