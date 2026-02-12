#include "isf_vdoenc_internal.h"
#include "mp_h264_encoder.h"
#include "video_codec_h264.h"
#include "kwrap/semaphore.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          h264encobj
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int h264encobj_debug_level = NVT_DBG_WRN;
module_param_named(debug_level_h264encobj, h264encobj_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_h264encobj, "mp_h264_encoder_obj debug level");
///////////////////////////////////////////////////////////////////////////////

#define H264_ENCODEID 0x68323634

ER MP_H264Enc1_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc1_close(void);
ER MP_H264Enc1_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc1_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc1_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc1_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc1_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc1_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc2_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc2_close(void);
ER MP_H264Enc2_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc2_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc2_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc2_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc2_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc2_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc3_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc3_close(void);
ER MP_H264Enc3_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc3_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc3_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc3_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc3_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc3_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc4_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc4_close(void);
ER MP_H264Enc4_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc4_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc4_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc4_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc4_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc4_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc5_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc5_close(void);
ER MP_H264Enc5_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc5_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc5_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc5_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc5_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc5_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc6_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc6_close(void);
ER MP_H264Enc6_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc6_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc6_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc6_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc6_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc6_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc7_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc7_close(void);
ER MP_H264Enc7_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc7_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc7_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc7_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc7_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc7_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc8_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc8_close(void);
ER MP_H264Enc8_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc8_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc8_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc8_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc8_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc8_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc9_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc9_close(void);
ER MP_H264Enc9_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc9_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc9_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc9_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc9_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc9_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc10_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc10_close(void);
ER MP_H264Enc10_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc10_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc10_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc10_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc10_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc10_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc11_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc11_close(void);
ER MP_H264Enc11_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc11_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc11_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc11_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc11_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc11_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc12_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc12_close(void);
ER MP_H264Enc12_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc12_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc12_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc12_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc12_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc12_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc13_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc13_close(void);
ER MP_H264Enc13_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc13_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc13_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc13_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc13_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc13_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc14_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc14_close(void);
ER MP_H264Enc14_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc14_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc14_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc14_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc14_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc14_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc15_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc15_close(void);
ER MP_H264Enc15_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc15_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc15_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc15_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc15_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc15_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);

ER MP_H264Enc16_init(MP_VDOENC_INIT *pVidEncInit);
ER MP_H264Enc16_close(void);
ER MP_H264Enc16_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H264Enc16_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDOENC_LL
ER MP_H264Enc16_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H264Enc16_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam);
#endif
ER MP_H264Enc16_triggerEnc(MP_VDOENC_PARAM *pVidEncParam);
ER MP_H264Enc16_waitEncReady(MP_VDOENC_PARAM *pVidEncParam);


MP_VDOENC_ENCODER gH264VideoEncObj[MP_VDOENC_ID_MAX] = {
	{
		MP_H264Enc1_init,           //Initialize
		MP_H264Enc1_close,          //Close
		MP_H264Enc1_getInfo,        //GetInfo
		MP_H264Enc1_setInfo,        //SetInfo
		MP_H264Enc1_encodeOne,      //EncodeOne
		MP_H264Enc1_triggerEnc,     //TriggerEnc
		MP_H264Enc1_waitEncReady,   //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc2_init,           //Initialize
		MP_H264Enc2_close,          //Close
		MP_H264Enc2_getInfo,        //GetInfo
		MP_H264Enc2_setInfo,        //SetInfo
		MP_H264Enc2_encodeOne,      //EncodeOne
		MP_H264Enc2_triggerEnc,     //TriggerEnc
		MP_H264Enc2_waitEncReady,   //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc3_init,           //Initialize
		MP_H264Enc3_close,          //Close
		MP_H264Enc3_getInfo,        //GetInfo
		MP_H264Enc3_setInfo,        //SetInfo
		MP_H264Enc3_encodeOne,      //EncodeOne
		MP_H264Enc3_triggerEnc,     //TriggerEnc
		MP_H264Enc3_waitEncReady,   //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc4_init,           //Initialize
		MP_H264Enc4_close,          //Close
		MP_H264Enc4_getInfo,        //GetInfo
		MP_H264Enc4_setInfo,        //SetInfo
		MP_H264Enc4_encodeOne,      //EncodeOne
		MP_H264Enc4_triggerEnc,     //TriggerEnc
		MP_H264Enc4_waitEncReady,   //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc5_init,           //Initialize
		MP_H264Enc5_close,          //Close
		MP_H264Enc5_getInfo,        //GetInfo
		MP_H264Enc5_setInfo,        //SetInfo
		MP_H264Enc5_encodeOne,      //EncodeOne
		MP_H264Enc5_triggerEnc,     //TriggerEnc
		MP_H264Enc5_waitEncReady,   //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc6_init,           //Initialize
		MP_H264Enc6_close,          //Close
		MP_H264Enc6_getInfo,        //GetInfo
		MP_H264Enc6_setInfo,        //SetInfo
		MP_H264Enc6_encodeOne,      //EncodeOne
		MP_H264Enc6_triggerEnc,     //TriggerEnc
		MP_H264Enc6_waitEncReady,   //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc7_init,           //Initialize
		MP_H264Enc7_close,          //Close
		MP_H264Enc7_getInfo,        //GetInfo
		MP_H264Enc7_setInfo,        //SetInfo
		MP_H264Enc7_encodeOne,      //EncodeOne
		MP_H264Enc7_triggerEnc,     //TriggerEnc
		MP_H264Enc7_waitEncReady,   //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc8_init,           //Initialize
		MP_H264Enc8_close,          //Close
		MP_H264Enc8_getInfo,        //GetInfo
		MP_H264Enc8_setInfo,        //SetInfo
		MP_H264Enc8_encodeOne,      //EncodeOne
		MP_H264Enc8_triggerEnc,     //TriggerEnc
		MP_H264Enc8_waitEncReady,   //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc9_init,           //Initialize
		MP_H264Enc9_close,          //Close
		MP_H264Enc9_getInfo,        //GetInfo
		MP_H264Enc9_setInfo,        //SetInfo
		MP_H264Enc9_encodeOne,      //EncodeOne
		MP_H264Enc9_triggerEnc,     //TriggerEnc
		MP_H264Enc9_waitEncReady,   //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc10_init,          //Initialize
		MP_H264Enc10_close,         //Close
		MP_H264Enc10_getInfo,       //GetInfo
		MP_H264Enc10_setInfo,       //SetInfo
		MP_H264Enc10_encodeOne,     //EncodeOne
		MP_H264Enc10_triggerEnc,    //TriggerEnc
		MP_H264Enc10_waitEncReady,  //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc11_init,          //Initialize
		MP_H264Enc11_close,         //Close
		MP_H264Enc11_getInfo,       //GetInfo
		MP_H264Enc11_setInfo,       //SetInfo
		MP_H264Enc11_encodeOne,     //EncodeOne
		MP_H264Enc11_triggerEnc,    //TriggerEnc
		MP_H264Enc11_waitEncReady,  //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc12_init,          //Initialize
		MP_H264Enc12_close,         //Close
		MP_H264Enc12_getInfo,       //GetInfo
		MP_H264Enc12_setInfo,       //SetInfo
		MP_H264Enc12_encodeOne,     //EncodeOne
		MP_H264Enc12_triggerEnc,    //TriggerEnc
		MP_H264Enc12_waitEncReady,  //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc13_init,          //Initialize
		MP_H264Enc13_close,         //Close
		MP_H264Enc13_getInfo,       //GetInfo
		MP_H264Enc13_setInfo,       //SetInfo
		MP_H264Enc13_encodeOne,     //EncodeOne
		MP_H264Enc13_triggerEnc,    //TriggerEnc
		MP_H264Enc13_waitEncReady,  //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc14_init,          //Initialize
		MP_H264Enc14_close,         //Close
		MP_H264Enc14_getInfo,       //GetInfo
		MP_H264Enc14_setInfo,       //SetInfo
		MP_H264Enc14_encodeOne,     //EncodeOne
		MP_H264Enc14_triggerEnc,    //TriggerEnc
		MP_H264Enc14_waitEncReady,  //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc15_init,          //Initialize
		MP_H264Enc15_close,         //Close
		MP_H264Enc15_getInfo,       //GetInfo
		MP_H264Enc15_setInfo,       //SetInfo
		MP_H264Enc15_encodeOne,     //EncodeOne
		MP_H264Enc15_triggerEnc,    //TriggerEnc
		MP_H264Enc15_waitEncReady,  //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	},

	{
		MP_H264Enc16_init,          //Initialize
		MP_H264Enc16_close,         //Close
		MP_H264Enc16_getInfo,       //GetInfo
		MP_H264Enc16_setInfo,       //SetInfo
		MP_H264Enc16_encodeOne,     //EncodeOne
		MP_H264Enc16_triggerEnc,    //TriggerEnc
		MP_H264Enc16_waitEncReady,  //WaitEncReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H264_ENCODEID               //checkID
	}
};

ER MP_H264Enc1_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_1, pVidEncInit);
}

ER MP_H264Enc1_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_1);
}

ER MP_H264Enc1_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_1, type, p1, p2, p3);
}

ER MP_H264Enc1_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_1, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc1_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_1, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc1_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_1, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc1_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_1, pVidEncParam);
}

ER MP_H264Enc1_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_1, pVidEncParam);
}

ER MP_H264Enc2_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_2, pVidEncInit);
}

ER MP_H264Enc2_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_2);
}

ER MP_H264Enc2_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_2, type, p1, p2, p3);
}

ER MP_H264Enc2_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_2, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc2_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_2, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc2_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_2, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc2_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_2, pVidEncParam);
}

ER MP_H264Enc2_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_2, pVidEncParam);
}

ER MP_H264Enc3_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_3, pVidEncInit);
}

ER MP_H264Enc3_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_3);
}

ER MP_H264Enc3_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_3, type, p1, p2, p3);
}

ER MP_H264Enc3_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_3, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc3_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_3, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc3_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_3, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc3_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_3, pVidEncParam);
}

ER MP_H264Enc3_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_3, pVidEncParam);
}

ER MP_H264Enc4_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_4, pVidEncInit);
}

ER MP_H264Enc4_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_4);
}

ER MP_H264Enc4_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_4, type, p1, p2, p3);
}

ER MP_H264Enc4_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_4, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc4_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_4, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc4_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_4, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc4_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_4, pVidEncParam);
}

ER MP_H264Enc4_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_4, pVidEncParam);
}

ER MP_H264Enc5_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_5, pVidEncInit);
}

ER MP_H264Enc5_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_5);
}

ER MP_H264Enc5_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_5, type, p1, p2, p3);
}

ER MP_H264Enc5_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_5, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc5_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_5, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc5_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_5, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc5_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_5, pVidEncParam);
}

ER MP_H264Enc5_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_5, pVidEncParam);
}

ER MP_H264Enc6_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_6, pVidEncInit);
}

ER MP_H264Enc6_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_6);
}

ER MP_H264Enc6_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_6, type, p1, p2, p3);
}

ER MP_H264Enc6_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_6, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc6_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_6, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc6_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_6, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc6_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_6, pVidEncParam);
}

ER MP_H264Enc6_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_6, pVidEncParam);
}

ER MP_H264Enc7_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_7, pVidEncInit);
}

ER MP_H264Enc7_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_7);
}

ER MP_H264Enc7_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_7, type, p1, p2, p3);
}

ER MP_H264Enc7_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_7, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc7_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_7, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc7_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_7, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc7_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_7, pVidEncParam);
}

ER MP_H264Enc7_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_7, pVidEncParam);
}

ER MP_H264Enc8_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_8, pVidEncInit);
}

ER MP_H264Enc8_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_8);
}

ER MP_H264Enc8_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_8, type, p1, p2, p3);
}

ER MP_H264Enc8_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_8, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc8_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_8, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc8_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_8, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc8_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_8, pVidEncParam);
}

ER MP_H264Enc8_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_8, pVidEncParam);
}

ER MP_H264Enc9_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_9, pVidEncInit);
}

ER MP_H264Enc9_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_9);
}

ER MP_H264Enc9_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_9, type, p1, p2, p3);
}

ER MP_H264Enc9_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_9, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc9_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_9, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc9_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_9, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc9_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_9, pVidEncParam);
}

ER MP_H264Enc9_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_9, pVidEncParam);
}

ER MP_H264Enc10_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_10, pVidEncInit);
}

ER MP_H264Enc10_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_10);
}

ER MP_H264Enc10_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_10, type, p1, p2, p3);
}

ER MP_H264Enc10_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_10, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc10_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_10, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc10_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_10, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc10_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_10, pVidEncParam);
}

ER MP_H264Enc10_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_10, pVidEncParam);
}

ER MP_H264Enc11_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_11, pVidEncInit);
}

ER MP_H264Enc11_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_11);
}

ER MP_H264Enc11_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_11, type, p1, p2, p3);
}

ER MP_H264Enc11_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_11, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc11_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_11, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc11_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_11, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc11_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_11, pVidEncParam);
}

ER MP_H264Enc11_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_11, pVidEncParam);
}

ER MP_H264Enc12_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_12, pVidEncInit);
}

ER MP_H264Enc12_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_12);
}

ER MP_H264Enc12_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_12, type, p1, p2, p3);
}

ER MP_H264Enc12_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_12, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc12_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_12, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc12_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_12, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc12_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_12, pVidEncParam);
}

ER MP_H264Enc12_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_12, pVidEncParam);
}

ER MP_H264Enc13_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_13, pVidEncInit);
}

ER MP_H264Enc13_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_13);
}

ER MP_H264Enc13_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_13, type, p1, p2, p3);
}

ER MP_H264Enc13_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_13, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc13_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_13, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc13_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_13, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc13_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_13, pVidEncParam);
}

ER MP_H264Enc13_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_13, pVidEncParam);
}

ER MP_H264Enc14_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_14, pVidEncInit);
}

ER MP_H264Enc14_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_14);
}

ER MP_H264Enc14_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_14, type, p1, p2, p3);
}

ER MP_H264Enc14_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_14, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc14_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_14, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc14_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_14, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc14_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_14, pVidEncParam);
}

ER MP_H264Enc14_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_14, pVidEncParam);
}

ER MP_H264Enc15_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_15, pVidEncInit);
}

ER MP_H264Enc15_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_15);
}

ER MP_H264Enc15_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_15, type, p1, p2, p3);
}

ER MP_H264Enc15_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_15, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc15_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_15, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc15_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_15, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc15_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_15, pVidEncParam);
}

ER MP_H264Enc15_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_15, pVidEncParam);
}

ER MP_H264Enc16_init(MP_VDOENC_INIT *pVidEncInit)
{
	return MP_H264Enc_init(MP_VDOENC_ID_16, pVidEncInit);
}

ER MP_H264Enc16_close(void)
{
	return MP_H264Enc_close(MP_VDOENC_ID_16);
}

ER MP_H264Enc16_getInfo(MP_VDOENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H264Enc_getInfo(MP_VDOENC_ID_16, type, p1, p2, p3);
}

ER MP_H264Enc16_setInfo(MP_VDOENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H264Enc_setInfo(MP_VDOENC_ID_16, type, param1, param2, param3);
}

#ifdef VDOENC_LL
ER MP_H264Enc16_encodeOne(UINT32 type, MP_VDOENC_PARAM *pVidEncParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_16, type, pVidEncParam, p_cb_func, p_user_data);
}
#else
ER MP_H264Enc16_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_encodeOne(MP_VDOENC_ID_16, type, outputAddr, pSize, pVidEncParam);
}
#endif

ER MP_H264Enc16_triggerEnc(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_triggerEnc(MP_VDOENC_ID_16, pVidEncParam);
}

ER MP_H264Enc16_waitEncReady(MP_VDOENC_PARAM *pVidEncParam)
{
	return MP_H264Enc_waitEncReady(MP_VDOENC_ID_16, pVidEncParam);
}

/**
    Get h.264 encoding object.

    Get h.264 encoding object.

    @return video codec encoding object
*/
PMP_VDOENC_ENCODER MP_H264Enc_getVideoObject(MP_VDOENC_ID VidEncId)
{
	return &gH264VideoEncObj[VidEncId];
}

// for backward compatible
PMP_VDOENC_ENCODER MP_H264Enc_getVideoEncodeObject(void)
{
	return &gH264VideoEncObj[0];
}

#ifdef __KERNEL__
EXPORT_SYMBOL(MP_H264Enc_getVideoObject);
EXPORT_SYMBOL(MP_H264Enc_getVideoEncodeObject);
#endif

