
/*
    IME module driver

    NT96520 IME internal header file.

    @file       ime_compression_base.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#ifndef _IME_COMPRESSION_REG_
#define _IME_COMPRESSION_REG_

#ifdef __cplusplus
extern "C" {
#endif

#include "ime_platform.h"


#if 1
extern const UINT32 ime_yuvcomp_dct_dec_qtbl[28][3];
extern const UINT32 ime_yuvcomp_dct_enc_qtbl[28][3];
extern const UINT32 ime_yuvcomp_dct_max[28];
extern UINT32 ime_yuvcomp_q_tbl_idx[16];

extern UINT32 yuv_dct_qtab_encp[16][3];
extern UINT32 yuv_dct_qtab_decp[16][3];
extern UINT32 yuv_dct_qtab_dc[16];
#endif

extern VOID ime_comps_p1_encoder_shift_mode_enable_reg(UINT32 set_en);

extern VOID ime_comps_tmnr_out_ref_encoder_shift_mode_enable_reg(UINT32 set_en);

extern VOID ime_comps_tmnr_in_ref_decoder_shift_mode_enable_reg(UINT32 set_en);

extern VOID ime_comps_tmnr_in_ref_encoder_dithering_enable_reg(UINT32 set_en);

extern VOID ime_comps_tmnr_in_ref_encoder_dithering_initial_seed_reg(UINT32 set_seed0, UINT32 set_seed1);

extern VOID ime_comps_encoder_parameters_reg(UINT32(*p_enc_tab)[3]);

extern VOID ime_comps_decoder_parameters_reg(UINT32(*p_dec_tab)[3]);

extern VOID ime_comps_dc_max_reg(UINT32 *p_dc_max);



#ifdef __cplusplus
}
#endif


#endif // _IME_COMPRESSION_REG_
