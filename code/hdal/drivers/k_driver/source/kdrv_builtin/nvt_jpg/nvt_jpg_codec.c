/**
 * @file kdrv_videoenc.h
 * @brief type definition of KDRV API.
 * @author ALG2
 * @date in the year 2018
 */

//#include "comm/timer.h"
//#include "comm/hwclock.h"

#include "kwrap/flag.h"
//#include "kwrap/type.h"
//#include "kdrv_type.h"

#include "jpeg.h"
//#include "jpg_enc.h"
//#include "kdrv_builtin/vdoenc_builtin.h"


INT32 jpeg_builtin_open(void)
{
    jpeg_open();

	return 0;
}

INT32 jpeg_builtin_close(void)
{
	jpeg_close();

	return 0;
}

INT32 jpeg_builtin_trigger(UINT32 id, KDRV_VDOENC_PARAM *p_enc_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	if (p_enc_param == NULL) {
		//DBG_ERR("p_enc_param is NULL\r\n");
		return -1;
	}

	if (jpeg_add_queue(id ,JPEG_CODEC_MODE_ENC,(void *)p_enc_param,p_cb_func, p_user_data) != E_OK) {
		return -1;
	} else {
		return 0;
	}

}


#if defined(__LINUX)
//EXPORT_SYMBOL(jpeg_builtin_open);
//EXPORT_SYMBOL(jpeg_builtin_close);
//EXPORT_SYMBOL(jpeg_builtin_trigger);

//EXPORT_SYMBOL(rc_cb_init);
#endif
