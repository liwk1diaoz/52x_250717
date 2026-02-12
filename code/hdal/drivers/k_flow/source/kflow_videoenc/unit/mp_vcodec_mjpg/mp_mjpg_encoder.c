#include "isf_vdoenc_internal.h"
#include "video_encode.h"
#include "video_codec_mjpg.h"
#include "mp_mjpg_encoder.h"
#include "kwrap/semaphore.h"
#include "kwrap/error_no.h"
#include "jpg_enc.h"  // TODO: use kdrv later
#include "kdrv_videoenc/kdrv_videoenc.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          mjpgenc
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int mjpgenc_debug_level = NVT_DBG_WRN;
module_param_named(debug_level_mjpgenc, mjpgenc_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_mjpgenc, "mp_mjpg_encoder debug level");
///////////////////////////////////////////////////////////////////////////////

#define MJPG_ENCODEID                				0x04030201

#define USE_VIDEOENC_KDRV     (1)           // 1: kdrv , 0: dal

#define MP_VDOENC_MJPEG_ID_MAX   (MP_VDOENC_ID_MAX+1)  // make pseudo path for internal JPEG encode snapshot
#define MP_VDOENC_MJPEG_ID_17    (MP_VDOENC_ID_MAX)

#if USE_VIDEOENC_KDRV
typedef struct _MJPEG_INFO_PRIV {
	UINT32    width;
	UINT32    height;
	UINT32    yuv_format;
} MJPEG_INFO_PRIV;

UINT32             g_mjpeg_path_count = 0;
MJPEG_INFO_PRIV    mjpeg_info[MP_VDOENC_MJPEG_ID_MAX] = {0};
#endif

ER MP_MjpgEnc_init(MP_VDOENC_ID VidEncId, MP_VDOENC_INIT *pVidEncInit)
{
#if USE_VIDEOENC_KDRV
	g_mjpeg_path_count++;

	if (g_mjpeg_path_count == 1) {
		kdrv_videoenc_open(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE_JPEG);
		DBG_IND("[JPEGENC] open\r\n");
	}

	mjpeg_info[VidEncId].width      = pVidEncInit->width;
	mjpeg_info[VidEncId].height     = pVidEncInit->height;
	mjpeg_info[VidEncId].yuv_format = pVidEncInit->jpeg_yuv_format;

	return E_OK;
#else
	return dal_jpegenc_init((DAL_VDOENC_ID)VidEncId, (DAL_VDOENC_INIT *)pVidEncInit);
#endif
}

ER MP_MjpgEnc_close(MP_VDOENC_ID VidEncId)
{
#if USE_VIDEOENC_KDRV
	g_mjpeg_path_count--;

	if (g_mjpeg_path_count == 0) {
		kdrv_videoenc_close(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE_JPEG);
		DBG_IND("[JPEGENC] close\r\n");
	}

	return E_OK;
#else
	return dal_jpegenc_close((DAL_VDOENC_ID)VidEncId);
#endif
}

ER MP_MjpgEnc_getInfo(MP_VDOENC_ID VidEncId, MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
#if USE_VIDEOENC_KDRV
	return E_OK;
#else
	return dal_jpegenc_getinfo((DAL_VDOENC_ID)VidEncId, (DAL_VDOENC_GET_ITEM)type, p1);
#endif
}

ER MP_MjpgEnc_setInfo(MP_VDOENC_ID VidEncId, MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
#if USE_VIDEOENC_KDRV
	INT32 ret=0;
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE_JPEG, VidEncId);

	if (type == VDOENC_SET_INIT) {  // only support kdrv_set(VDOENC_SET_INIT) for jpg
		ret = kdrv_videoenc_set(id, (KDRV_VDOENC_SET_PARAM_ID)type, (VOID *)param1);
		return (ret == 0)? E_OK:E_SYS;
	} else {
		return E_OK;
	}
#else
	return dal_jpegenc_setinfo((DAL_VDOENC_ID)VidEncId, (DAL_VDOENC_SET_ITEM)type, param1);
#endif
}

#ifdef VDOENC_LL
ER MP_MjpgEnc_encodeOne(MP_VDOENC_ID VidEncId, UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
#else
ER MP_MjpgEnc_encodeOne(MP_VDOENC_ID VidEncId, UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
#endif
{
#if USE_VIDEOENC_KDRV
	INT32 ret = 0;
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE_JPEG, VidEncId);
#ifdef VDOENC_LL
		pVidEncParam->encode_width	 = mjpeg_info[VidEncId].width;
		pVidEncParam->encode_height   = mjpeg_info[VidEncId].height;
		pVidEncParam->in_fmt		 = mjpeg_info[VidEncId].yuv_format;  // 0: YUV422,	1: YUV420	 (dal & kdrv  same define, don't have to rewrite)

		ret = kdrv_videoenc_trigger(id, (KDRV_VDOENC_PARAM *) pVidEncParam, p_cb_func, p_user_data);

		return (ret == 0)? E_OK:E_SYS;
#else
	KDRV_VDOENC_PARAM enc_obj;

	memcpy(&enc_obj, (void *)pVidEncParam, sizeof(MP_VDOENC_PARAM));
	enc_obj.encode_width      = mjpeg_info[VidEncId].width;
	enc_obj.encode_height     = mjpeg_info[VidEncId].height;
	enc_obj.in_fmt            = mjpeg_info[VidEncId].yuv_format;  // 0: YUV422,  1: YUV420   (dal & kdrv  same define, don't have to rewrite)

	ret = kdrv_videoenc_trigger(id, &enc_obj, NULL, NULL);

	if (ret == 0) {
		pVidEncParam->bs_size_1 = enc_obj.bs_size_1;
		pVidEncParam->base_qp   = enc_obj.base_qp;
		pVidEncParam->frm_type  = enc_obj.frm_type;
	}
#endif
	return (ret == 0)? E_OK:E_SYS;
#else
	return dal_jpegenc_encodeone((DAL_VDOENC_ID)VidEncId, (DAL_VDOENC_PARAM *)pVidEncParam);
#endif
}

ER MP_MjpgEnc1_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc1_close(void);
ER MP_MjpgEnc1_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc1_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc1_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc1_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc2_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc2_close(void);
ER MP_MjpgEnc2_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc2_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc2_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc2_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc3_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc3_close(void);
ER MP_MjpgEnc3_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc3_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc3_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc3_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc4_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc4_close(void);
ER MP_MjpgEnc4_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc4_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc4_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc4_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc5_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc5_close(void);
ER MP_MjpgEnc5_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc5_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc5_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc5_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc6_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc6_close(void);
ER MP_MjpgEnc6_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc6_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc6_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc6_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc7_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc7_close(void);
ER MP_MjpgEnc7_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc7_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc7_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc7_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc8_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc8_close(void);
ER MP_MjpgEnc8_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc8_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc8_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc8_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc9_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc9_close(void);
ER MP_MjpgEnc9_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc9_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc9_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc9_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc10_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc10_close(void);
ER MP_MjpgEnc10_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc10_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc10_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc10_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc11_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc11_close(void);
ER MP_MjpgEnc11_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc11_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc11_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc11_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc12_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc12_close(void);
ER MP_MjpgEnc12_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc12_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc12_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc12_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc13_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc13_close(void);
ER MP_MjpgEnc13_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc13_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc13_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc13_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc14_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc14_close(void);
ER MP_MjpgEnc14_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc14_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc14_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc14_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc15_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc15_close(void);
ER MP_MjpgEnc15_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc15_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc15_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc15_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc16_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc16_close(void);
ER MP_MjpgEnc16_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc16_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc16_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc16_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

ER MP_MjpgEnc17_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_MjpgEnc17_close(void);
ER MP_MjpgEnc17_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgEnc17_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_MjpgEnc17_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgEnc17_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif

MP_VDOENC_ENCODER gMjpgVideoEncObj[MP_VDOENC_MJPEG_ID_MAX] = {
	{
		MP_MjpgEnc1_init,           //Initialize
		MP_MjpgEnc1_close,          //Close
		MP_MjpgEnc1_getInfo,        //GetInfo
		MP_MjpgEnc1_setInfo,        //SetInfo
		MP_MjpgEnc1_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc2_init,           //Initialize
		MP_MjpgEnc2_close,          //Close
		MP_MjpgEnc2_getInfo,        //GetInfo
		MP_MjpgEnc2_setInfo,        //SetInfo
		MP_MjpgEnc2_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc3_init,           //Initialize
		MP_MjpgEnc3_close,          //Close
		MP_MjpgEnc3_getInfo,        //GetInfo
		MP_MjpgEnc3_setInfo,        //SetInfo
		MP_MjpgEnc3_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc4_init,           //Initialize
		MP_MjpgEnc4_close,          //Close
		MP_MjpgEnc4_getInfo,        //GetInfo
		MP_MjpgEnc4_setInfo,        //SetInfo
		MP_MjpgEnc4_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc5_init,           //Initialize
		MP_MjpgEnc5_close,          //Close
		MP_MjpgEnc5_getInfo,        //GetInfo
		MP_MjpgEnc5_setInfo,        //SetInfo
		MP_MjpgEnc5_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc6_init,           //Initialize
		MP_MjpgEnc6_close,          //Close
		MP_MjpgEnc6_getInfo,        //GetInfo
		MP_MjpgEnc6_setInfo,        //SetInfo
		MP_MjpgEnc6_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc7_init,           //Initialize
		MP_MjpgEnc7_close,          //Close
		MP_MjpgEnc7_getInfo,        //GetInfo
		MP_MjpgEnc7_setInfo,        //SetInfo
		MP_MjpgEnc7_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc8_init,           //Initialize
		MP_MjpgEnc8_close,          //Close
		MP_MjpgEnc8_getInfo,        //GetInfo
		MP_MjpgEnc8_setInfo,        //SetInfo
		MP_MjpgEnc8_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc9_init,           //Initialize
		MP_MjpgEnc9_close,          //Close
		MP_MjpgEnc9_getInfo,        //GetInfo
		MP_MjpgEnc9_setInfo,        //SetInfo
		MP_MjpgEnc9_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc10_init,           //Initialize
		MP_MjpgEnc10_close,          //Close
		MP_MjpgEnc10_getInfo,        //GetInfo
		MP_MjpgEnc10_setInfo,        //SetInfo
		MP_MjpgEnc10_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc11_init,           //Initialize
		MP_MjpgEnc11_close,          //Close
		MP_MjpgEnc11_getInfo,        //GetInfo
		MP_MjpgEnc11_setInfo,        //SetInfo
		MP_MjpgEnc11_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc12_init,           //Initialize
		MP_MjpgEnc12_close,          //Close
		MP_MjpgEnc12_getInfo,        //GetInfo
		MP_MjpgEnc12_setInfo,        //SetInfo
		MP_MjpgEnc12_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc13_init,           //Initialize
		MP_MjpgEnc13_close,          //Close
		MP_MjpgEnc13_getInfo,        //GetInfo
		MP_MjpgEnc13_setInfo,        //SetInfo
		MP_MjpgEnc13_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc14_init,           //Initialize
		MP_MjpgEnc14_close,          //Close
		MP_MjpgEnc14_getInfo,        //GetInfo
		MP_MjpgEnc14_setInfo,        //SetInfo
		MP_MjpgEnc14_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc15_init,           //Initialize
		MP_MjpgEnc15_close,          //Close
		MP_MjpgEnc15_getInfo,        //GetInfo
		MP_MjpgEnc15_setInfo,        //SetInfo
		MP_MjpgEnc15_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc16_init,           //Initialize
		MP_MjpgEnc16_close,          //Close
		MP_MjpgEnc16_getInfo,        //GetInfo
		MP_MjpgEnc16_setInfo,        //SetInfo
		MP_MjpgEnc16_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	},

	{
		MP_MjpgEnc17_init,           //Initialize
		MP_MjpgEnc17_close,          //Close
		MP_MjpgEnc17_getInfo,        //GetInfo
		MP_MjpgEnc17_setInfo,        //SetInfo
		MP_MjpgEnc17_encodeOne,      //EncodeOne
		NULL,                       //TriggerEnc
		NULL,                       //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_ENCODEID               //checkID
	}
};

ER MP_MjpgEnc1_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_1, pVidEncInit);
}

ER MP_MjpgEnc1_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_1);
}

ER MP_MjpgEnc1_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_1, type, p1, p2, p3);
}

ER MP_MjpgEnc1_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_1, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc1_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_1, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc1_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_1, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc2_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_2, pVidEncInit);
}

ER MP_MjpgEnc2_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_2);
}

ER MP_MjpgEnc2_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_2, type, p1, p2, p3);
}

ER MP_MjpgEnc2_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_2, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc2_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_2, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc2_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_2, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc3_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_3, pVidEncInit);
}

ER MP_MjpgEnc3_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_3);
}

ER MP_MjpgEnc3_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_3, type, p1, p2, p3);
}

ER MP_MjpgEnc3_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_3, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc3_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_3, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc3_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_3, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc4_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_4, pVidEncInit);
}

ER MP_MjpgEnc4_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_4);
}

ER MP_MjpgEnc4_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_4, type, p1, p2, p3);
}

ER MP_MjpgEnc4_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_4, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc4_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_4, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc4_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_4, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc5_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_5, pVidEncInit);
}

ER MP_MjpgEnc5_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_5);
}

ER MP_MjpgEnc5_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_5, type, p1, p2, p3);
}

ER MP_MjpgEnc5_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_5, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc5_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_5, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc5_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_5, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc6_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_6, pVidEncInit);
}

ER MP_MjpgEnc6_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_6);
}

ER MP_MjpgEnc6_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_6, type, p1, p2, p3);
}

ER MP_MjpgEnc6_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_6, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc6_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_6, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc6_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_6, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc7_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_7, pVidEncInit);
}

ER MP_MjpgEnc7_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_7);
}

ER MP_MjpgEnc7_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_7, type, p1, p2, p3);
}

ER MP_MjpgEnc7_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_7, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc7_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_7, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc7_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_7, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc8_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_8, pVidEncInit);
}

ER MP_MjpgEnc8_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_8);
}

ER MP_MjpgEnc8_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_8, type, p1, p2, p3);
}

ER MP_MjpgEnc8_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_8, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc8_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_8, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc8_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_8, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc9_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_9, pVidEncInit);
}

ER MP_MjpgEnc9_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_9);
}

ER MP_MjpgEnc9_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_9, type, p1, p2, p3);
}

ER MP_MjpgEnc9_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_9, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc9_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_9, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc9_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_9, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc10_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_10, pVidEncInit);
}

ER MP_MjpgEnc10_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_10);
}

ER MP_MjpgEnc10_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_10, type, p1, p2, p3);
}

ER MP_MjpgEnc10_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_10, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc10_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_10, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc10_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_10, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc11_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_11, pVidEncInit);
}

ER MP_MjpgEnc11_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_11);
}

ER MP_MjpgEnc11_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_11, type, p1, p2, p3);
}

ER MP_MjpgEnc11_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_11, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc11_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_11, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc11_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_11, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc12_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_12, pVidEncInit);
}

ER MP_MjpgEnc12_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_12);
}

ER MP_MjpgEnc12_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_12, type, p1, p2, p3);
}

ER MP_MjpgEnc12_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_12, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc12_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_12, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc12_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_12, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc13_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_13, pVidEncInit);
}

ER MP_MjpgEnc13_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_13);
}

ER MP_MjpgEnc13_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_13, type, p1, p2, p3);
}

ER MP_MjpgEnc13_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_13, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc13_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_13, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc13_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_13, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc14_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_14, pVidEncInit);
}

ER MP_MjpgEnc14_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_14);
}

ER MP_MjpgEnc14_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_14, type, p1, p2, p3);
}

ER MP_MjpgEnc14_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_14, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc14_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_14, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc14_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_14, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc15_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_15, pVidEncInit);
}

ER MP_MjpgEnc15_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_15);
}

ER MP_MjpgEnc15_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_15, type, p1, p2, p3);
}

ER MP_MjpgEnc15_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_15, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc15_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_15, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc15_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_15, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_MjpgEnc16_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_ID_16, pVidEncInit);
}

ER MP_MjpgEnc16_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_ID_16);
}

ER MP_MjpgEnc16_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_ID_16, type, p1, p2, p3);
}

ER MP_MjpgEnc16_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_ID_16, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc16_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_16, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc16_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_ID_16, type, outputAddr, pSize, pVidEncParam);
}
#endif

//---- path 17 is pseudo path, for internal jpeg encode snapshot ----
ER MP_MjpgEnc17_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_MjpgEnc_init(MP_VDOENC_MJPEG_ID_17, pVidEncInit);
}

ER MP_MjpgEnc17_close(void)
{
	return MP_MjpgEnc_close(MP_VDOENC_MJPEG_ID_17);
}

ER MP_MjpgEnc17_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgEnc_getInfo(MP_VDOENC_MJPEG_ID_17, type, p1, p2, p3);
}

ER MP_MjpgEnc17_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgEnc_setInfo(MP_VDOENC_MJPEG_ID_17, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_MjpgEnc17_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_MJPEG_ID_17, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgEnc17_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_MjpgEnc_encodeOne(MP_VDOENC_MJPEG_ID_17, type, outputAddr, pSize, pVidEncParam);
}
#endif

/**
    Get h.264 encoding object.

    Get h.264 encoding object.

    @return video codec encoding object
*/
PMP_VDOENC_ENCODER MP_MjpgEnc_getVideoObject(MP_VDOENC_ID VidEncId)
{
	return &gMjpgVideoEncObj[VidEncId];
}

// for backward compatible
PMP_VDOENC_ENCODER MP_MjpgEnc_getVideoEncodeObject(void)
{
	return &gMjpgVideoEncObj[0];
}


#ifdef __KERNEL__
EXPORT_SYMBOL(MP_MjpgEnc_getVideoObject);
EXPORT_SYMBOL(MP_MjpgEnc_getVideoEncodeObject);
#endif

