
#include "video_decode.h"
#include "video_codec_h264.h"
#include "mp_h264_decoder.h"
#include "kwrap/semaphore.h"
#include "kwrap/error_no.h"
#include "kdrv_videodec/kdrv_videodec.h"
#ifdef __KERNEL__
#include <linux/module.h>
#endif



#define __MODULE__          h264dec
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int h264dec_debug_level = NVT_DBG_WRN;
#ifdef __KERNEL__
module_param_named(debug_level_h264dec, h264dec_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_h264dec, "mp_h264_decoder debug level");
#else
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#endif

#define USE_VIDEODEC_KDRV     (1)           // 1: kdrv , 0: dal
UINT32             g_h26x_path_dec_count = 0;

ER MP_H264Dec_init(MP_VDODEC_ID VidDecId, MP_VDODEC_INIT *pVidDecInit)
{
#if USE_VIDEODEC_KDRV
	INT ret = 0;
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0, VidDecId);
	KDRV_VDODEC_TYPE codec = VDODEC_TYPE_H264;

	g_h26x_path_dec_count++;

	if (g_h26x_path_dec_count == 1) {
		kdrv_videodec_open(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0);
		DBG_IND("[H26XDEC] open\r\n");
	}

	kdrv_videodec_set(id, VDODEC_SET_CODEC, &codec);

	ret = kdrv_videodec_set(id, VDODEC_SET_INIT, (MP_VDODEC_INIT *)pVidDecInit);
	return (ret == 0)? E_OK:E_SYS;
#else
	return dal_h264dec_init((DAL_VDODEC_ID)VidDecId, (MP_VDODEC_INIT *)pVidDecInit);
#endif
}

ER MP_H264Dec_close(MP_VDODEC_ID VidDecId)
{
#if USE_VIDEODEC_KDRV
	INT32 ret = 0;
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0, VidDecId);
	KDRV_VDODEC_TYPE codec = VDODEC_TYPE_H264;
	kdrv_videodec_set(id, VDODEC_SET_CODEC, &codec);

	ret = kdrv_videodec_set(id, VDODEC_SET_CLOSE, &codec);

	if (ret != 0) return E_SYS;

	g_h26x_path_dec_count--;

	if (g_h26x_path_dec_count == 0) {
		ret = kdrv_videodec_close(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0);
		DBG_IND("[H26XDEC] close\r\n");
	}

	return (ret == 0)? E_OK:E_SYS;
#else
	return dal_h264dec_close((DAL_VDODEC_ID)VidDecId);
#endif
}

ER MP_H264Dec_getInfo(MP_VDODEC_ID VidDecId, MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
#if USE_VIDEODEC_KDRV
	INT32 ret = 0;
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0, VidDecId);
	KDRV_VDODEC_TYPE codec = VDODEC_TYPE_H264;
	kdrv_videodec_set(id, VDODEC_SET_CODEC, &codec);
	ret = kdrv_videodec_get(id, (KDRV_VDODEC_GET_PARAM_ID)type, (VOID *)p1);

	return (ret == 0)? E_OK:E_SYS;
#else
	return dal_h264dec_getinfo((DAL_VDODEC_ID)VidDecId, (DAL_VDODEC_GET_ITEM)type, p1);
#endif
}

ER MP_H264Dec_setInfo(MP_VDODEC_ID VidDecId, MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
#if USE_VIDEODEC_KDRV
	INT32 ret=0;
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0, VidDecId);
	KDRV_VDODEC_TYPE codec = VDODEC_TYPE_H264;
	kdrv_videodec_set(id, VDODEC_SET_CODEC, &codec);
	ret = kdrv_videodec_set(id, (KDRV_VDODEC_SET_PARAM_ID)type, (VOID *)param1);

	return (ret == 0)? E_OK:E_SYS;
#else
	return dal_h264dec_setinfo((DAL_VDODEC_ID)VidDecId, (DAL_VDODEC_SET_ITEM)type, param1);
#endif
}

// for D2D path
#ifdef VDODEC_LL
ER MP_H264Dec_decodeOne(MP_VDODEC_ID VidDecId, UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
#else
ER MP_H264Dec_decodeOne(MP_VDODEC_ID VidDecId, UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
#endif
{
#if USE_VIDEODEC_KDRV
	INT32 ret = 0;
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0, VidDecId);
	KDRV_VDODEC_TYPE codec = VDODEC_TYPE_H264;
	kdrv_videodec_set(id, VDODEC_SET_CODEC, &codec);

#ifdef VDODEC_LL
	ret = kdrv_videodec_trigger(id, (KDRV_VDODEC_PARAM *) pVidDecParam, p_cb_func, p_user_data);
#else
	ret = kdrv_videodec_trigger(id, (KDRV_VDODEC_PARAM *) pVidDecParam, NULL, NULL);
#endif
	if (ret != E_OK) {
		pVidDecParam->errorcode = E_SYS;
	} else {
		pVidDecParam->errorcode = E_OK;
	}

	return (ret == 0)? E_OK:E_SYS;
#else
	//return dal_h264dec_decodeone((DAL_VDODEC_ID)VidDecId, (DAL_VDODEC_PARAM *)pVidDecParam);
	ER ret;
	ret = dal_h264dec_decodeone((DAL_VDODEC_ID)VidDecId, (DAL_VDODEC_PARAM *)pVidDecParam);
	if (ret != E_OK) {
		pVidDecParam->errorcode = E_SYS;
	} else {
		pVidDecParam->errorcode = E_OK;
	}
	return ret;
#endif
}

// for direct path (IP only, not support 2P, 2B simultaneously enocding), trigger decode and no wait
ER MP_H264Dec_triggerDec(MP_VDODEC_ID VidDecId, MP_VDODEC_PARAM *pVidDecParam)
{
	return E_OK;
}

// for direct path, wait for decode ready and get results
ER MP_H264Dec_waitDecReady(MP_VDODEC_ID VidDecId, MP_VDODEC_PARAM *pVidDecParam)
{
	return E_OK;
}


