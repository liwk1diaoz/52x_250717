/**
    Public header file for KDRV_IPP

    This file is the header file that define the API and data type for KDRV_IPP.

    @file       kdrv_ipp.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _KDRV_IPP_H_
#define _KDRV_IPP_H_

#if defined __UITRON || defined __ECOS
#include "Type.h"
#include "mIType.h"
#include "ErrorNo.h"
#include "dal_ipp_utility.h"
#include "dal_ipp_config.h"
#include "dal_dce.h"
#include "dal_ife.h"

#include "dal_ime.h"
#include "dal_ife2.h"

#elif defined(__FREERTOS)
// NOTE: XXXXX
#include "kdrv_ipp_utility.h"
#include "kdrv_type.h"
#include "kdrv_ipe.h"
#include "kdrv_dce.h"
#include "kdrv_ife.h"
#include "kdrv_ime.h"
#include "kdrv_ife2.h"

#else
#include "kwrap/type.h"
//#include "kwrap/mitype.h"
#include "kwrap/error_no.h"
#include "kdrv_ipp_utility.h"
//#include "kdrv_ife_config.h"
//#include "kdrv_dce_config.h"
#include "kdrv_type.h"
#include "kdrv_ipe.h"
#include "kdrv_dce.h"
#include "kdrv_ife.h"
//#include "kdrv_rhe.h"
#include "kdrv_ime.h"
#include "kdrv_ife2.h"
#endif


/* engine parameter channel buffer numbers */
#define KDRV_IPP_CBUF_MAX_NUM    17


/**
    Open device
*/

INT32  kdrv_ipe_open(UINT32 chip, UINT32 engine);
INT32  kdrv_dce_open(UINT32 chip, UINT32 engine);
INT32  kdrv_ife_open(UINT32 chip, UINT32 engine);
#if 0

//UINT32 kdrv_ife_open(void* open_cfg);
INT32 kdrv_rhe_open(UINT32 chip, UINT32 engine);
#endif

/*!
 * @fn INT32 kdrv_ime_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 *                  - @b KDRV_VIDEOPROCS_IME_ENGINE0: IME engine 0
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ime_open(UINT32 chip, UINT32 engine);


/*!
 * @fn INT32 kdrv_ife2_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 *                  - @b KDRV_VIDEOPROCS_IFE2_ENGINE0: IFE2 engine 0
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ife2_open(UINT32 chip, UINT32 engine);


/**
    Close device
*/

INT32 kdrv_ipe_close(UINT32 chip, UINT32 engine);
INT32 kdrv_dce_close(UINT32 chip, UINT32 engine);
INT32 kdrv_ife_close(UINT32 chip, UINT32 engine);
#if 0
//ER kdrv_ife_close(UINT32 hdl);
INT32 kdrv_rhe_close(UINT32 chip, UINT32 engine);
#endif


/*!
 * @fn INT32 kdrv_ime_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 *                  - @b KDRV_VIDEOPROCS_IME_ENGINE0: IME engine 0
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ime_close(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_ife2_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 *                  - @b KDRV_VIDEOPROCS_IFE2_ENGINE0: IFE2 engine 0
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ife2_close(UINT32 chip, UINT32 engine);


/**
    Trigger device
*/

/*!
 * @fn INT32 kdrv_xxx_trigger(INT32 id, KDRV_RPC_TRIGGER_PARAM *p_rpc_param,
                                 KDRV_CALLBACK_FUNC *p_cb_func,
                                 VOID *p_user_data);
 * @brief trigger hardware engine
 * @param id                    the id of hardware
 * @param p_rpc_param           the parameter for trigger
 * @param p_cb_func             the callback function
 * @param p_user_data           the private user data
 * @return return 0 on success, < 0 on error
 */

INT32 kdrv_ife_trigger(UINT32 id, KDRV_IFE_TRIGGER_PARAM *p_ife_param,
					   KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
INT32 kdrv_ipe_trigger(UINT32 id, KDRV_IPE_TRIGGER_PARAM *p_ipe_param,
					   KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);


INT32 kdrv_dce_trigger(UINT32 id, KDRV_DCE_TRIGGER_PARAM *p_dce_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#if 0
INT32 kdrv_rhe_trigger(UINT32 id, KDRV_RHE_TRIGGER_PARAM *p_rhe_param,
					   KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#endif

/*!
 * @fn INT32 kdrv_ime_trigger(UINT32 id, KDRV_IME_TRIGGER_PARAM *p_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
 * @brief trigger hardware engine
 * @param id                the id of hardware
 * @param p_param           the parameter for trigger
 * @param p_cb_func         the callback function
 * @param p_user_data       the private user data
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ime_trigger(UINT32 id, KDRV_IME_TRIGGER_PARAM *p_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);


/*!
 * @fn INT32 kdrv_ife2_trigger(UINT32 id, KDRV_IFE2_TRIGGER_PARAM *p_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
 * @brief trigger hardware engine
 * @param id                the id of hardware
 * @param p_param           the parameter for trigger
 * @param p_cb_func         the callback function
 * @param p_user_data       the private user data
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ife2_trigger(UINT32 id, KDRV_IFE2_TRIGGER_PARAM *p_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);


/**
    Config device parameter
*/
INT32 kdrv_ipe_set(UINT32 id, KDRV_IPE_PARAM_ID param_id, void *p_param);
INT32 kdrv_dce_set(UINT32 id, KDRV_DCE_PARAM_ID parm_id, VOID *p_param);
INT32 kdrv_ife_set(UINT32 id, KDRV_IFE_PARAM_ID parm_id, VOID *p_param);
#if 0
INT32 kdrv_rhe_set(UINT32 id, KDRV_RHE_PARAM_ID param_id, void *p_param);
//ER kdrv_ife_set(UINT32 hdl, KDRV_IFE_PARAM_ID item, void* data);
#endif

/*!
 * @fn INT32 kdrv_ime_set(UINT32 id, KDRV_IME_PARAM_ID param_id, VOID *p_param)
 * @brief set parameters to hardware engine
 * @param id        the id of hardware
 * @param param_id  the id of parameters
 * @param p_param   the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ime_set(UINT32 id, KDRV_IME_PARAM_ID param_id, VOID *p_param);


/*!
 * @fn INT32 kdrv_ife2_set(UINT32 id, KDRV_IFE2_PARAM_ID param_id, VOID *p_param)
 * @brief set parameters to hardware engine
 * @param id        the id of hardware
 * @param param_id  the id of parameters
 * @param p_param   the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ife2_set(UINT32 id, KDRV_IFE2_PARAM_ID param_id, VOID *p_param);



/**
    Get device parameter
*/

INT32 kdrv_ipe_get(UINT32 id, KDRV_IPE_PARAM_ID param_id, void *p_param);
INT32 kdrv_dce_get(UINT32 id, KDRV_DCE_PARAM_ID parm_id, VOID *p_param);
INT32 kdrv_ife_get(UINT32 id, KDRV_IFE_PARAM_ID parm_id, VOID *p_param);
#if 0
//ER kdrv_ife_get(UINT32 hdl, KDRV_IFE_PARAM_ID item, void* data);
INT32 kdrv_rhe_get(UINT32 id, KDRV_RHE_PARAM_ID param_id, void *p_param);
#endif


/*!
 * @fn INT32 kdrv_ime_get(UINT32 id, KDRV_IME_PARAM_ID param_id, VOID *p_param)
 * @brief get parameters from hardware engine
 * @param id        the id of hardware
 * @param param_id  the id of parameters
 * @param p_param   the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ime_get(UINT32 id, KDRV_IME_PARAM_ID param_id, VOID *p_param);


/*!
 * @fn INT32 kdrv_ife2_get(UINT32 id, KDRV_IFE2_PARAM_ID param_id, VOID *p_param)
 * @brief get parameters from hardware engine
 * @param id        the id of hardware
 * @param param_id  the id of parameters
 * @param p_param   the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ife2_get(UINT32 id, KDRV_IFE2_PARAM_ID param_id, VOID *p_param);

/*!
 * @fn INT32 kdrv_ife_pause(UINT32 chip, UINT32 engine)
 * @brief pause
 * @param chip      the id of hardware
 * @param engine    the engine id of hardware
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ife_pause(UINT32 chip, UINT32 engine);

/**
    Pause device
*/

INT32 kdrv_dce_pause(UINT32 chip, UINT32 engine);
INT32 kdrv_ipe_pause(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_ime_pause(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 *                  - @b KDRV_VIDEOPROCS_IME_ENGINE0: IME engine 0
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ime_pause(UINT32 chip, UINT32 engine);


/*!
 * @fn INT32 kdrv_ife2_pause(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 *                  - @b KDRV_VIDEOPROCS_IFE2_ENGINE0: IFE2 engine 0
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ife2_pause(UINT32 chip, UINT32 engine);

VOID kdrv_ife_init(VOID);
VOID kdrv_ife_uninit(VOID);
UINT32 kdrv_ife_buf_query(UINT32 channel_num);
UINT32 kdrv_ife_buf_init(UINT32 input_addr, UINT32 channel_num);
VOID kdrv_ife_buf_uninit(VOID);

VOID kdrv_dce_init(VOID);
VOID kdrv_dce_uninit(VOID);
UINT32 kdrv_dce_buf_query(UINT32 channel_num);
UINT32 kdrv_dce_buf_init(UINT32 input_addr, UINT32 channel_num);
VOID kdrv_dce_buf_uninit(VOID);

VOID kdrv_ipe_init(VOID);
VOID kdrv_ipe_uninit(VOID);
UINT32 kdrv_ipe_buf_query(UINT32 channel_num);
UINT32 kdrv_ipe_buf_init(UINT32 input_addr, UINT32 channel_num);
VOID kdrv_ipe_buf_uninit(VOID);



VOID kdrv_ime_init(VOID);
VOID kdrv_ime_uninit(VOID);
UINT32 kdrv_ime_buf_query(UINT32 channel_num);
UINT32 kdrv_ime_buf_init(UINT32 input_addr, UINT32 channel_num);
VOID kdrv_ime_buf_uninit(VOID);


VOID kdrv_ife2_init(VOID);
VOID kdrv_ife2_uninit(VOID);
UINT32 kdrv_ife2_buf_query(UINT32 channel_num);
UINT32 kdrv_ife2_buf_init(UINT32 input_addr, UINT32 channel_num);
VOID kdrv_ife2_buf_uninit(VOID);



/**
    dump info
*/
#if defined __UITRON || defined __ECOS
void dal_ipe_dump_info(void (*dump)(char *fmt, ...));
void dal_dce_dump_info(void (*dump)(char *fmt, ...));
void dal_ife_dump_info(void (*dump)(char *fmt, ...));
void dal_rhe_dump_info(void (*dump)(char *fmt, ...));
void dal_ime_dump_info(void (*dump)(char *fmt, ...));
void dal_ife2_dump_info(void (*dump)(char *fmt, ...));
#else

void kdrv_ipe_dump_info(int (*dump)(const char *fmt, ...));
void kdrv_dce_dump_info(int (*dump)(const char *fmt, ...));
void kdrv_ife_dump_info(int (*dump)(const char *fmt, ...));
#if 0
void kdrv_rhe_dump_info(int (*dump)(const char *fmt, ...));
#endif

void kdrv_ime_dump_info(int (*dump)(const char *fmt, ...));

void kdrv_ife2_dump_info(int (*dump)(const char *fmt, ...));

#endif


#endif //_KDRV_IPP_H_
