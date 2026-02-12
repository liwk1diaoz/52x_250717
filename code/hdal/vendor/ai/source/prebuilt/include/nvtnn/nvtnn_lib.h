/**
    @brief Header file of NvtNN library definitions.

    @file nvtnn_lib.h

    @ingroup nvtnn

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2022.  All rights reserved.
*/
#ifndef _NVTNN_LIB_H_
#define _NVTNN_LIB_H_

/********************************************************************
    INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "net_flow_sample/nn_parm.h"

/********************************************************************
    TYPE DEFINITIONS
********************************************************************/
typedef NN_SOFTMAX_PARM     NVTNN_SOFTMAX_PRMS;
typedef NN_PRELU_PARM       NVTNN_PRELU_PRMS;
typedef NN_PRIORBOX_PARM    NVTNN_PRIORBOX_PRMS;
typedef NN_DETOUT_PARM      NVTNN_DETOUT_PRMS;
typedef NN_LRN_PARM         NVTNN_LRN_PRMS;
typedef NN_FC_POST_PARM     NVTNN_FC_POST_PRMS;
typedef NN_POOL_PARM        NVTNN_POOL_PRMS;
typedef NN_LSTM_PARM        NVTNN_LSTM_PRMS;
typedef NN_PERMUTE_PARM     NVTNN_PERMUTE_PRMS;
typedef NN_REVERSE_PARM     NVTNN_REVERSE_PRMS;
typedef NN_NORM_PARM        NVTNN_NORM_PRMS;
typedef NN_LAYER_NORMALIZATION_PARM   NVTNN_LAYER_LAYERNORM_PRMS;

typedef enum {
	RNDA = 0,             ///< Round away from zero								
	ENUM_DUMMY4WORD(RND_MODE)
} RND_MODE;

/********************************************************************
    EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif


extern const CHAR *prebuilt_ai_get_lib_version(VOID);

/* Vision Layers */
extern HD_RESULT nvtnn_pool_process(NVTNN_POOL_PRMS *p_prms);

/* Recurrent Layers */
extern HD_RESULT nvtnn_lstm_process(NVTNN_LSTM_PRMS *p_prms);

/* Normalization Layers */
extern HD_RESULT nvtnn_lrn_process(NVTNN_LRN_PRMS *p_prms);
extern HD_RESULT nvtnn_norm_process(NVTNN_NORM_PRMS *p_prms);

/* Activation Layers */
extern HD_RESULT nvtnn_prelu_process(NVTNN_PRELU_PRMS *p_prms);
extern HD_RESULT nvtnn_layernorm_process(NVTNN_LAYER_LAYERNORM_PRMS *p_prms);

/* Utility Layers */
extern HD_RESULT nvtnn_softmax_process(NVTNN_SOFTMAX_PRMS *p_prms);
extern HD_RESULT nvtnn_permute_process(NVTNN_PERMUTE_PRMS *p_prms);
extern HD_RESULT nvtnn_reverse_process(NVTNN_REVERSE_PRMS *p_prms);

/* Post-processing */
extern HD_RESULT nvtnn_priorbox_process(NVTNN_PRIORBOX_PRMS *p_prms);
extern HD_RESULT nvtnn_detout_process(NVTNN_DETOUT_PRMS *p_prms);
extern HD_RESULT nvtnn_fc_post_process(NVTNN_FC_POST_PRMS *p_prms);


/* Float to Fix */
extern VOID nvtnn_floattofix_c(FLOAT *p_in, VOID *p_out, UINT32 size, NN_FMT *p_fmt, NN_SF *p_sf);
extern VOID nvtnn_floattofix_neon(FLOAT *p_in, VOID *p_out, UINT32 size, NN_FMT *p_fmt, NN_SF *p_sf);
extern VOID floattoint8_c(FLOAT *input_data, INT8 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID floattouint8_c(FLOAT *input_data, UINT8 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID floattoint16_c(FLOAT *input_data, INT16 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID floattouint16_c(FLOAT *input_data, UINT16 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID floattoint8_neon(FLOAT *input_data, INT8 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID floattouint8_neon(FLOAT *input_data, UINT8 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID floattoint16_neon(FLOAT *input_data, INT16 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID floattouint16_neon(FLOAT *input_data, UINT16 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);

extern VOID nvtnn_floattofix_rnd_c(FLOAT *p_in, VOID *p_out, UINT32 size, NN_FMT *p_fmt, NN_SF *p_sf, RND_MODE rnd_mode);
extern VOID floattoint8_rnd_c(FLOAT *input_data, INT8 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio, RND_MODE rnd_mode);
extern VOID floattouint8_rnd_c(FLOAT *input_data, UINT8 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio, RND_MODE rnd_mode);
extern VOID floattoint16_rnd_c(FLOAT *input_data, INT16 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio, RND_MODE rnd_mode);
extern VOID floattouint16_rnd_c(FLOAT *input_data, UINT16 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio, RND_MODE rnd_mode);
extern VOID floattoint32_rnd_c(FLOAT *input_data, INT32 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio, RND_MODE rnd_mode);
extern VOID floattouint32_rnd_c(FLOAT *input_data, UINT32 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio, RND_MODE rnd_mode);

extern VOID nvtnn_floattofix_rnd_neon(FLOAT *p_in, VOID *p_out, UINT32 size, NN_FMT *p_fmt, NN_SF *p_sf, RND_MODE rnd_mode);
extern VOID floattoint8_rnd_neon(FLOAT *input_data, INT8 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio, RND_MODE rnd_mode);
extern VOID floattouint8_rnd_neon(FLOAT *input_data, UINT8 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio, RND_MODE rnd_mode);
extern VOID floattoint16_rnd_neon(FLOAT *input_data, INT16 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio, RND_MODE rnd_mode);
extern VOID floattouint16_rnd_neon(FLOAT *input_data, UINT16 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio, RND_MODE rnd_mode);
extern VOID floattoint32_rnd_neon(FLOAT *input_data, INT32 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio, RND_MODE rnd_mode);
extern VOID floattouint32_rnd_neon(FLOAT *input_data, UINT32 *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio, RND_MODE rnd_mode);

/* Fix to Float */
extern VOID nvtnn_fixtofloat_c(VOID *p_in, FLOAT *p_out, UINT32 size, NN_FMT *p_fmt, NN_SF *p_sf);
extern VOID nvtnn_fixtofloat_neon(VOID *p_in, FLOAT *p_out, UINT32 size, NN_FMT *p_fmt, NN_SF *p_sf);
extern VOID int8tofloat_c(INT8 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID uint8tofloat_c(UINT8 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID int16tofloat_c(INT16 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID uint16tofloat_c(UINT16 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID int8tofloat_neon(INT8 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID uint8tofloat_neon(UINT8 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID int16tofloat_neon(INT16 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);
extern VOID uint16tofloat_neon(UINT16 *input_data, FLOAT *output_data, INT16 frac_bits, INT32 size_data, FLOAT scale_ratio);

/* Permute Data */
extern VOID permute_data_out(INT8 *p_in, INT8 *p_out, UINT32 stride, UINT32 num);
extern VOID permute_data_in(INT8 *p_in, INT8 *p_out, UINT32 stride, UINT32 num);


#ifdef __cplusplus
}
#endif

#endif  /* _NVTNN_LIB_H_ */
