#ifndef _MSF_LIB_
#define _MSF_LIB_

#include "hd_type.h"
/************************* DEFINE *************************/
typedef enum _MSF_DBG_LVL {
	MSF_DBG_LVL_ERR = 1,
	MSF_DBG_LVL_IND,
	MSF_DBG_LVL_MAX,
} MSF_DBG_LVL;

typedef enum _MSF_DBG_MODE {
	MSF_DBG_MODE_DISABLE = 0x0,
	MSF_DBG_MODE_BYPASS_PROC = 0x1, //bypass fusion process, result image = yuv image
} MSF_DBG_MODE;

/************************* TYPE *************************/
typedef struct _MSF_BUF {
	UINT8 ddr_id;	//ddr id
	void *va_addr;	//virtual addr
	UINT32 pa_addr;	//physical addr
	UINT32 size;	//buffer size
} MSF_BUF;

typedef struct _MSF_IQ_PARAM {
	UINT8 curve_th0;	//0~255
	UINT8 curve_th1;	//0~255
	UINT8 curve_val0;	//0~255
	UINT8 curve_val1;	//0~255
} MSF_IQ_PARAM;

/************************* API *************************/
/**
     init msf lib

     @return Description of data returned.
         - @HD_RESULT HD_OK: success
         - @HD_RESULT HD_ERR_SYS: fail
*/
extern HD_RESULT msf_init(void);

/**
     msf uninit

     @return Description of data returned.
         - @HD_RESULT HD_OK: success
*/
extern HD_RESULT msf_uninit(void);

/**
     msf open
     @param[in] buf working buffer parameters(call msf_get_work_buf_szie to get buf size)

     @return Description of data returned.
         - @UINT32 >0: success, msf handle
         - @UINT32 Others: fail
*/
extern UINT32 msf_open(MSF_BUF buf);

/**
     msf close
     @param[in] handle msf handle

     @return Description of data returned.
         - @UINT32 HD_OK: success
         - @UINT32 Others: fail
*/
extern HD_RESULT msf_close(UINT32 handle);

/**
     msf process, note:output image overwrite yuv_img
     msf_init -> msf_open -> msf_proc -> msf_proc ... -> msf_close -> msf_uninit

     @param[in] handle ctl handle(msf_open return value)
     @param[in] yuv_img yuv image(only support y planer format)
     @param[in] ir_img ir image(only support y planer format)
     @param[in] iq_param weight curve

     @return Description of data returned.
         - @HD_RESULT HD_OK: success
         - @HD_RESULT others: fail
*/
extern HD_RESULT msf_proc(UINT32 handle, HD_VIDEO_FRAME *yuv_img, HD_VIDEO_FRAME *ir_img, MSF_IQ_PARAM *iq_param);

/**
     get msf work buffer size

     @param[in] img_w image width pixel(1 pixel = 1 byte)
     @param[in] img_h image height(1 pixel = 1 byte)

     @return Description of data returned.
         - @char != null: success, msf lib version str
         - @char = null: fail
*/
extern UINT32 msf_get_work_buf_szie(UINT32 img_w, UINT32 img_h);

/**
     get msf lib version

     @return Description of data returned.
         - @char != null: success, msf lib version str
         - @char = null: fail
*/
extern CHAR* msf_get_version(void);

/**
     set debug level

     @return Description of data returned.
         - @HD_RESULT E_OK: success
*/
extern HD_RESULT msf_set_dbg_lvl(MSF_DBG_LVL lvl);

/**
     msf dump infor

     @return Description of data returned.
         - @HD_RESULT HD_OK: success
*/
extern HD_RESULT msf_dump_info(void);

/**
     msf set debug mode
     @param[in] handle ctl handle(msf_open return value)
     @param[in] mode, debug mode, ref. MSF_DBG_MODE

     @return Description of data returned.
         - @HD_RESULT HD_OK: success
*/
extern HD_RESULT msf_set_dbg_mode(UINT32 handle, MSF_DBG_MODE mode);
#endif //_MSF_LIB_

