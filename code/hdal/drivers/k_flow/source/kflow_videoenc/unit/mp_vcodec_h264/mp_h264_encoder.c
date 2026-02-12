#include "isf_vdoenc_internal.h"
#include "video_encode.h"
#include "video_codec_h264.h"
#include "mp_h264_encoder.h"
#include "kwrap/semaphore.h"
#include "kwrap/error_no.h"
#include "kdrv_videoenc/kdrv_videoenc.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          h264enc
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int h264enc_debug_level = NVT_DBG_WRN;
module_param_named(debug_level_h264enc, h264enc_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_h264enc, "mp_h264_encoder debug level");
///////////////////////////////////////////////////////////////////////////////

#define USE_VIDEOENC_KDRV     (1)           // 1: kdrv , 0: dal
UINT32             g_h26x_path_count = 0;

ER MP_H264Enc_init(MP_VDOENC_ID VidEncId, MP_VDOENC_INIT *pVidEncInit)
{
#if USE_VIDEOENC_KDRV
	INT ret = 0;
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0, VidEncId);
	KDRV_VDOENC_TYPE codec = VDOENC_TYPE_H264;

	g_h26x_path_count++;

	if (g_h26x_path_count == 1) {
		kdrv_videoenc_open(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0);
		DBG_IND("[H26XENC] open\r\n");
	}

	kdrv_videoenc_set(id, VDOENC_SET_CODEC, &codec);

	ret = kdrv_videoenc_set(id, VDOENC_SET_INIT, (KDRV_VDOENC_INIT *)pVidEncInit);
	return (ret == 0)? E_OK:E_SYS;
#else
	return dal_h264enc_init((DAL_VDOENC_ID)VidEncId, (MP_VDOENC_INIT *)pVidEncInit);
#endif
}

ER MP_H264Enc_close(MP_VDOENC_ID VidEncId)
{
#if USE_VIDEOENC_KDRV
	INT32 ret = 0;
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0, VidEncId);
	KDRV_VDOENC_TYPE codec = VDOENC_TYPE_H264;
	kdrv_videoenc_set(id, VDOENC_SET_CODEC, &codec);
	// coverity[var_deref_model]
	ret = kdrv_videoenc_set(id, VDOENC_SET_CLOSE, NULL);
	if (ret != 0) return E_SYS;

	g_h26x_path_count--;

	if (g_h26x_path_count == 0) {
		ret = kdrv_videoenc_close(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0);
		DBG_IND("[H26XENC] close\r\n");
	}
	return (ret == 0)? E_OK:E_SYS;
#else
	return dal_h264enc_close((DAL_VDOENC_ID)VidEncId);
#endif
}

ER MP_H264Enc_getInfo(MP_VDOENC_ID VidEncId, MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
#if USE_VIDEOENC_KDRV
	INT32 ret = 0;
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0, VidEncId);
	KDRV_VDOENC_TYPE codec = VDOENC_TYPE_H264;
	kdrv_videoenc_set(id, VDOENC_SET_CODEC, &codec);

	ret = kdrv_videoenc_get(id, (KDRV_VDOENC_GET_PARAM_ID)type, (VOID *)p1);
	return (ret == 0)? E_OK:E_SYS;
#else
	return dal_h264enc_getinfo((DAL_VDOENC_ID)VidEncId, (DAL_VDOENC_GET_ITEM)type, p1);
#endif
}

ER MP_H264Enc_setInfo(MP_VDOENC_ID VidEncId, MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
#if USE_VIDEOENC_KDRV
	INT32 ret=0;
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0, VidEncId);
	KDRV_VDOENC_TYPE codec = VDOENC_TYPE_H264;
	kdrv_videoenc_set(id, VDOENC_SET_CODEC, &codec);

	ret = kdrv_videoenc_set(id, (KDRV_VDOENC_SET_PARAM_ID)type, (VOID *)param1);
	return (ret == 0)? E_OK:E_SYS;
#else
	return dal_h264enc_setinfo((DAL_VDOENC_ID)VidEncId, (DAL_VDOENC_SET_ITEM)type, param1);
#endif
}

// for D2D path
#ifdef VDOENC_LL
ER MP_H264Enc_encodeOne(MP_VDOENC_ID VidEncId, UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
#else
ER MP_H264Enc_encodeOne(MP_VDOENC_ID VidEncId, UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
#endif
{
#if USE_VIDEOENC_KDRV
	INT32 ret = 0;
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0, VidEncId);
	KDRV_VDOENC_TYPE codec = VDOENC_TYPE_H264;
	kdrv_videoenc_set(id, VDOENC_SET_CODEC, &codec);

#ifdef VDOENC_LL
	ret = kdrv_videoenc_trigger(id, (KDRV_VDOENC_PARAM *) pVidEncParam, p_cb_func, p_user_data);
#else
	ret = kdrv_videoenc_trigger(id, (KDRV_VDOENC_PARAM *) pVidEncParam, NULL, NULL);
#endif
	return (ret == 0)? E_OK:E_SYS;
#else
	return dal_h264enc_encodeone((DAL_VDOENC_ID)VidEncId, (DAL_VDOENC_PARAM *)pVidEncParam);
#endif
}

// for direct path (IP only, not support 2P, 2B simultaneously enocding), trigger encode and no wait
ER MP_H264Enc_triggerEnc(MP_VDOENC_ID VidEncId, MP_VDOENC_PARAM *pVidEncParam)
{
	return E_OK;
}

// for direct path, wait for encode ready and get results
ER MP_H264Enc_waitEncReady(MP_VDOENC_ID VidEncId, MP_VDOENC_PARAM *pVidEncParam)
{
	return E_OK;
}

