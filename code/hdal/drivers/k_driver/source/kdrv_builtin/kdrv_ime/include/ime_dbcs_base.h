/*
    IME module driver

    NT96520 IME internal header file.

    @file       ime_dbcs_base.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#ifndef _IME_DBCS_REG_
#define _IME_DBCS_REG_

#ifdef __cplusplus
extern "C" {
#endif

#include "ime_platform.h"

//-------------------------------------------------------------------------------
// DBCS
extern VOID ime_set_dbcs_enable_reg(UINT32 set_en);
extern UINT32 ime_get_dbcs_enable_reg(VOID);
extern VOID ime_set_dbcs_center_reg(UINT32 cent_u, UINT32 cent_v);
extern VOID ime_set_dbcs_mode_reg(UINT32 set_mode);
extern VOID ime_set_dbcs_step_reg(UINT32 step_y, UINT32 step_uv);
extern VOID ime_set_dbcs_weight_y_reg(UINT32 *p_wts);
extern VOID ime_set_dbcs_weight_uv_reg(UINT32 *p_wts);



#ifdef __cplusplus
}
#endif


#endif // _IME_DBCS_REG_
