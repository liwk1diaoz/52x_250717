
#include "video_decode.h"
#include "video_codec_mjpg.h"
#include "mp_mjpg_decoder.h"
#include "kwrap/semaphore.h"
#include "kwrap/error_no.h"
#include "jpg_dec.h"  // TODO: use kdrv later
#include "kdrv_videodec/kdrv_videodec.h"
#include "kflow_common/type_vdo.h"
#ifdef __KERNEL__
#include <linux/module.h>
#endif

#define __MODULE__          mjpgdec
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int mjpgdec_debug_level = NVT_DBG_WRN;
#ifdef __KERNEL__
module_param_named(debug_level_mjpgdec, mjpgdec_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_mjpgdec, "mp_mjpg_decoder debug level");
#else
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#endif

#define MJPG_DECODEID                				0x01020304

#define USE_VIDEODEC_KDRV     (1)           // 1: kdrv , 0: dal

#define MP_VDODEC_MJPEG_ID_MAX   (MP_VDODEC_ID_MAX+1)  // make pseudo path for internal JPEG decode snapshot
#define MP_VDODEC_MJPEG_ID_17    (MP_VDODEC_ID_MAX)

#if USE_VIDEODEC_KDRV
typedef struct _MJPEG_DECINFO_PRIV {
	UINT32    width;
	UINT32    height;
	UINT32    yuv_format;
} MJPEG_DECINFO_PRIV;

UINT32             g_mjpeg_path_dec_count = 0;
MJPEG_DECINFO_PRIV    mjpeg_decinfo[MP_VDODEC_MJPEG_ID_MAX] = {0};
#endif

UINT32 MP_MjpgDec_yuv_kdrv2kflow(UINT32 yuv_fmt)
{
	switch (yuv_fmt)
	{
		case KDRV_VDODEC_YUV420:    return VDO_PXLFMT_YUV420;
		case KDRV_VDODEC_YUV422:    return VDO_PXLFMT_YUV422;
		default:
			DBG_ERR("unknown yuv format(%d)\r\n", yuv_fmt);
			return (0xFFFFFFFF);
	}
}

ER MP_MjpgDec_init(MP_VDODEC_ID VidDecId, MP_VDODEC_INIT *pVidDecInit)
{
#if USE_VIDEODEC_KDRV
	g_mjpeg_path_dec_count++;

	if (g_mjpeg_path_dec_count == 1) {
		kdrv_videodec_open(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE_JPEG);
		DBG_IND("[JPEGDEC] open\r\n");
	}

//	mjpeg_decinfo[VidDecId].width      = pVidDecInit->uiWidth;
//	mjpeg_decinfo[VidDecId].height     = pVidDecInit->uiHeight;
//	mjpeg_decinfo[VidDecId].yuv_format = pVidDecInit->uiYuvFormat;

	return E_OK;
#else
	return dal_jpegdec_init((DAL_VDODEC_ID)VidDecId, (DAL_VDODEC_INIT *)pVidDecInit);
#endif
}

ER MP_MjpgDec_close(MP_VDODEC_ID VidDecId)
{
#if USE_VIDEODEC_KDRV
	g_mjpeg_path_dec_count--;

	if (g_mjpeg_path_dec_count == 0) {
		kdrv_videodec_close(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE_JPEG);
		DBG_IND("[JPEGDEC] close\r\n");
	}

	return E_OK;
#else
	return dal_jpegdec_close((DAL_VDODEC_ID)VidDecId);
#endif
}

ER MP_MjpgDec_getInfo(MP_VDODEC_ID VidDecId, MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
#if USE_VIDEODEC_KDRV
	INT32 ret = 0;
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE0, VidDecId);

	KDRV_VDODEC_TYPE codec = VDODEC_TYPE_JPEG;
	kdrv_videodec_set(id, VDODEC_SET_CODEC, &codec);
	ret = kdrv_videodec_get(id, (KDRV_VDODEC_GET_PARAM_ID)type, (VOID *)p1);

	return (ret == 0)? E_OK:E_SYS;
#else
	return dal_jpegdec_getinfo((DAL_VDODEC_ID)VidDecId, (DAL_VDODEC_GET_ITEM)type, p1);
#endif
}

ER MP_MjpgDec_setInfo(MP_VDODEC_ID VidDecId, MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
#if USE_VIDEODEC_KDRV
	return E_OK;
#else
	return dal_jpegdec_setinfo((DAL_VDODEC_ID)VidDecId, (DAL_VDODEC_SET_ITEM)type, param1);
#endif
}

#ifdef VDODEC_LL
ER MP_MjpgDec_decodeOne(MP_VDODEC_ID VidDecId, UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
#else
ER MP_MjpgDec_decodeOne(MP_VDODEC_ID VidDecId, UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
#endif
{
#if USE_VIDEODEC_KDRV
	INT32 ret = 0;
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_VIDEOCDC_ENGINE_JPEG, VidDecId);
	KDRV_VDODEC_PARAM *dec_obj = (KDRV_VDODEC_PARAM *)pVidDecParam;

	dec_obj->jpeg_hdr_addr= (UINT32)pVidDecParam->bs_addr;
	dec_obj->y_addr=(UINT32)pVidDecParam->y_addr;
	dec_obj->c_addr=(UINT32)pVidDecParam->c_addr;
	dec_obj->bs_size= pVidDecParam->bs_size;

#ifdef VDODEC_LL
	ret = kdrv_videodec_trigger(id, dec_obj, p_cb_func, p_user_data);

	return (ret == 0)? E_OK : E_SYS;
#else
	ret = kdrv_videodec_trigger(id, dec_obj, NULL, NULL);

	if ((ret != 0 )||(dec_obj->errorcode!= JPG_HEADER_ER_OK )) {
		DBG_ERR("jpeg dec error(0x%x)\r\n", dec_obj->errorcode);
		pVidDecParam->errorcode = E_SYS;
		return E_SYS;
	}

	pVidDecParam->uiWidth= dec_obj->uiWidth;
	pVidDecParam->uiHeight= dec_obj->uiHeight;
	pVidDecParam->yuv_fmt = MP_MjpgDec_yuv_kdrv2kflow(dec_obj->yuv_fmt);
	if (pVidDecParam->yuv_fmt == 0xFFFFFFFF) {
		DBG_ERR("invalid yuv fmt(%d)\r\n", dec_obj->errorcode);
		pVidDecParam->errorcode = E_SYS;
		return E_SYS;
	}

	pVidDecParam->errorcode = E_OK;
	return E_OK;
#endif
#else
	return dal_jpegdec_decodeone((DAL_VDODEC_ID)VidDecId, (DAL_VDODEC_PARAM *)pVidDecParam);
#endif
}

ER MP_MjpgDec1_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec1_close(void);
ER MP_MjpgDec1_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec1_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec1_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec1_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec2_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec2_close(void);
ER MP_MjpgDec2_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec2_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec2_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec2_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec3_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec3_close(void);
ER MP_MjpgDec3_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec3_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec3_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec3_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec4_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec4_close(void);
ER MP_MjpgDec4_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec4_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec4_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec4_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec5_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec5_close(void);
ER MP_MjpgDec5_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec5_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec5_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec5_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec6_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec6_close(void);
ER MP_MjpgDec6_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec6_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec6_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec6_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec7_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec7_close(void);
ER MP_MjpgDec7_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec7_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec7_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec7_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec8_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec8_close(void);
ER MP_MjpgDec8_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec8_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec8_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec8_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec9_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec9_close(void);
ER MP_MjpgDec9_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec9_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec9_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec9_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec10_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec10_close(void);
ER MP_MjpgDec10_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec10_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec10_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec10_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec11_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec11_close(void);
ER MP_MjpgDec11_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec11_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec11_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec11_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec12_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec12_close(void);
ER MP_MjpgDec12_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec12_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec12_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec12_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec13_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec13_close(void);
ER MP_MjpgDec13_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec13_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec13_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec13_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec14_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec14_close(void);
ER MP_MjpgDec14_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec14_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec14_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec14_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec15_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec15_close(void);
ER MP_MjpgDec15_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec15_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec15_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec15_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec16_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec16_close(void);
ER MP_MjpgDec16_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec16_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec16_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec16_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

ER MP_MjpgDec17_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_MjpgDec17_close(void);
ER MP_MjpgDec17_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_MjpgDec17_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_MjpgDec17_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_MjpgDec17_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif

MP_VDODEC_DECODER gMjpgVideoDecObj[MP_VDODEC_MJPEG_ID_MAX] = {
	{
		MP_MjpgDec1_init,           //Initialize
		MP_MjpgDec1_close,          //Close
		MP_MjpgDec1_getInfo,        //GetInfo
		MP_MjpgDec1_setInfo,        //SetInfo
		MP_MjpgDec1_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec2_init,           //Initialize
		MP_MjpgDec2_close,          //Close
		MP_MjpgDec2_getInfo,        //GetInfo
		MP_MjpgDec2_setInfo,        //SetInfo
		MP_MjpgDec2_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec3_init,           //Initialize
		MP_MjpgDec3_close,          //Close
		MP_MjpgDec3_getInfo,        //GetInfo
		MP_MjpgDec3_setInfo,        //SetInfo
		MP_MjpgDec3_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec4_init,           //Initialize
		MP_MjpgDec4_close,          //Close
		MP_MjpgDec4_getInfo,        //GetInfo
		MP_MjpgDec4_setInfo,        //SetInfo
		MP_MjpgDec4_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec5_init,           //Initialize
		MP_MjpgDec5_close,          //Close
		MP_MjpgDec5_getInfo,        //GetInfo
		MP_MjpgDec5_setInfo,        //SetInfo
		MP_MjpgDec5_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec6_init,           //Initialize
		MP_MjpgDec6_close,          //Close
		MP_MjpgDec6_getInfo,        //GetInfo
		MP_MjpgDec6_setInfo,        //SetInfo
		MP_MjpgDec6_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec7_init,           //Initialize
		MP_MjpgDec7_close,          //Close
		MP_MjpgDec7_getInfo,        //GetInfo
		MP_MjpgDec7_setInfo,        //SetInfo
		MP_MjpgDec7_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec8_init,           //Initialize
		MP_MjpgDec8_close,          //Close
		MP_MjpgDec8_getInfo,        //GetInfo
		MP_MjpgDec8_setInfo,        //SetInfo
		MP_MjpgDec8_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec9_init,           //Initialize
		MP_MjpgDec9_close,          //Close
		MP_MjpgDec9_getInfo,        //GetInfo
		MP_MjpgDec9_setInfo,        //SetInfo
		MP_MjpgDec9_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec10_init,           //Initialize
		MP_MjpgDec10_close,          //Close
		MP_MjpgDec10_getInfo,        //GetInfo
		MP_MjpgDec10_setInfo,        //SetInfo
		MP_MjpgDec10_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec11_init,           //Initialize
		MP_MjpgDec11_close,          //Close
		MP_MjpgDec11_getInfo,        //GetInfo
		MP_MjpgDec11_setInfo,        //SetInfo
		MP_MjpgDec11_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec12_init,           //Initialize
		MP_MjpgDec12_close,          //Close
		MP_MjpgDec12_getInfo,        //GetInfo
		MP_MjpgDec12_setInfo,        //SetInfo
		MP_MjpgDec12_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec13_init,           //Initialize
		MP_MjpgDec13_close,          //Close
		MP_MjpgDec13_getInfo,        //GetInfo
		MP_MjpgDec13_setInfo,        //SetInfo
		MP_MjpgDec13_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec14_init,           //Initialize
		MP_MjpgDec14_close,          //Close
		MP_MjpgDec14_getInfo,        //GetInfo
		MP_MjpgDec14_setInfo,        //SetInfo
		MP_MjpgDec14_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec15_init,           //Initialize
		MP_MjpgDec15_close,          //Close
		MP_MjpgDec15_getInfo,        //GetInfo
		MP_MjpgDec15_setInfo,        //SetInfo
		MP_MjpgDec15_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec16_init,           //Initialize
		MP_MjpgDec16_close,          //Close
		MP_MjpgDec16_getInfo,        //GetInfo
		MP_MjpgDec16_setInfo,        //SetInfo
		MP_MjpgDec16_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	},

	{
		MP_MjpgDec17_init,           //Initialize
		MP_MjpgDec17_close,          //Close
		MP_MjpgDec17_getInfo,        //GetInfo
		MP_MjpgDec17_setInfo,        //SetInfo
		MP_MjpgDec17_decodeOne,      //DecodeOne
		NULL,                       //TriggerDec
		NULL,                       //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		MJPG_DECODEID               //checkID
	}
};

ER MP_MjpgDec1_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_1, pVidDecInit);
}

ER MP_MjpgDec1_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_1);
}

ER MP_MjpgDec1_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_1, type, p1, p2, p3);
}

ER MP_MjpgDec1_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_1, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec1_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_1, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec1_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_1, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec2_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_2, pVidDecInit);
}

ER MP_MjpgDec2_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_2);
}

ER MP_MjpgDec2_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_2, type, p1, p2, p3);
}

ER MP_MjpgDec2_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_2, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec2_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_2, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec2_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_2, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec3_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_3, pVidDecInit);
}

ER MP_MjpgDec3_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_3);
}

ER MP_MjpgDec3_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_3, type, p1, p2, p3);
}

ER MP_MjpgDec3_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_3, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec3_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_3, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec3_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_3, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec4_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_4, pVidDecInit);
}

ER MP_MjpgDec4_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_4);
}

ER MP_MjpgDec4_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_4, type, p1, p2, p3);
}

ER MP_MjpgDec4_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_4, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec4_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_4, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec4_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_4, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec5_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_5, pVidDecInit);
}

ER MP_MjpgDec5_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_5);
}

ER MP_MjpgDec5_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_5, type, p1, p2, p3);
}

ER MP_MjpgDec5_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_5, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec5_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_5, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec5_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_5, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec6_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_6, pVidDecInit);
}

ER MP_MjpgDec6_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_6);
}

ER MP_MjpgDec6_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_6, type, p1, p2, p3);
}

ER MP_MjpgDec6_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_6, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec6_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_6, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec6_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_6, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec7_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_7, pVidDecInit);
}

ER MP_MjpgDec7_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_7);
}

ER MP_MjpgDec7_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_7, type, p1, p2, p3);
}

ER MP_MjpgDec7_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_7, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec7_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_7, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec7_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_7, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec8_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_8, pVidDecInit);
}

ER MP_MjpgDec8_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_8);
}

ER MP_MjpgDec8_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_8, type, p1, p2, p3);
}

ER MP_MjpgDec8_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_8, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec8_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_8, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec8_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_8, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec9_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_9, pVidDecInit);
}

ER MP_MjpgDec9_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_9);
}

ER MP_MjpgDec9_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_9, type, p1, p2, p3);
}

ER MP_MjpgDec9_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_9, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec9_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_9, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec9_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_9, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec10_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_10, pVidDecInit);
}

ER MP_MjpgDec10_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_10);
}

ER MP_MjpgDec10_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_10, type, p1, p2, p3);
}

ER MP_MjpgDec10_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_10, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec10_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_10, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec10_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_10, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec11_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_11, pVidDecInit);
}

ER MP_MjpgDec11_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_11);
}

ER MP_MjpgDec11_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_11, type, p1, p2, p3);
}

ER MP_MjpgDec11_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_11, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec11_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_11, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec11_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_11, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec12_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_12, pVidDecInit);
}

ER MP_MjpgDec12_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_12);
}

ER MP_MjpgDec12_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_12, type, p1, p2, p3);
}

ER MP_MjpgDec12_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_12, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec12_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_12, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec12_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_12, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec13_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_13, pVidDecInit);
}

ER MP_MjpgDec13_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_13);
}

ER MP_MjpgDec13_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_13, type, p1, p2, p3);
}

ER MP_MjpgDec13_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_13, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec13_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_13, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec13_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_13, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec14_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_14, pVidDecInit);
}

ER MP_MjpgDec14_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_14);
}

ER MP_MjpgDec14_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_14, type, p1, p2, p3);
}

ER MP_MjpgDec14_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_14, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec14_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_14, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec14_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_14, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec15_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_15, pVidDecInit);
}

ER MP_MjpgDec15_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_15);
}

ER MP_MjpgDec15_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_15, type, p1, p2, p3);
}

ER MP_MjpgDec15_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_15, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec15_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_15, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec15_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_15, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_MjpgDec16_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_ID_16, pVidDecInit);
}

ER MP_MjpgDec16_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_ID_16);
}

ER MP_MjpgDec16_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_ID_16, type, p1, p2, p3);
}

ER MP_MjpgDec16_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_ID_16, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec16_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_16, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec16_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_ID_16, type, outputAddr, pSize, pVidDecParam);
}
#endif

//---- path 17 is pseudo path, for internal jpeg decode snapshot ----
ER MP_MjpgDec17_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_MjpgDec_init(MP_VDODEC_MJPEG_ID_17, pVidDecInit);
}

ER MP_MjpgDec17_close(void)
{
	return MP_MjpgDec_close(MP_VDODEC_MJPEG_ID_17);
}

ER MP_MjpgDec17_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_MjpgDec_getInfo(MP_VDODEC_MJPEG_ID_17, type, p1, p2, p3);
}

ER MP_MjpgDec17_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_MjpgDec_setInfo(MP_VDODEC_MJPEG_ID_17, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_MjpgDec17_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_MJPEG_ID_17, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_MjpgDec17_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_MjpgDec_decodeOne(MP_VDODEC_MJPEG_ID_17, type, outputAddr, pSize, pVidDecParam);
}
#endif

/**
    Get h.264 decoding object.

    Get h.264 decoding object.

    @return video codec decoding object
*/
PMP_VDODEC_DECODER MP_MjpgDec_getVideoObject(MP_VDODEC_ID VidDecId)
{
	return &gMjpgVideoDecObj[VidDecId];
}

// for backward compatible
PMP_VDODEC_DECODER MP_MjpgDec_getVideoDecodeObject(void)
{
	return &gMjpgVideoDecObj[0];
}


#ifdef __KERNEL__
EXPORT_SYMBOL(MP_MjpgDec_getVideoObject);
EXPORT_SYMBOL(MP_MjpgDec_getVideoDecodeObject);
#endif

