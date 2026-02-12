
#include "mp_h265_decoder.h"
#include "video_codec_h265.h"


#define __MODULE__          h265decobj
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int h265decobj_debug_level = NVT_DBG_WRN;
#ifdef __KERNEL__
module_param_named(debug_level_h265decobj, h265decobj_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_h265decobj, "mp_h265_decoder_obj debug level");
#else
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#endif


#define H265_DECODEID 0x68323635

ER MP_H265Dec1_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec1_close(void);
ER MP_H265Dec1_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec1_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec1_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec1_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec1_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec1_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec2_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec2_close(void);
ER MP_H265Dec2_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec2_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec2_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec2_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec2_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec2_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec3_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec3_close(void);
ER MP_H265Dec3_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec3_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec3_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec3_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec3_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec3_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec4_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec4_close(void);
ER MP_H265Dec4_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec4_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec4_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec4_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec4_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec4_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec5_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec5_close(void);
ER MP_H265Dec5_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec5_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec5_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec5_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec5_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec5_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec6_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec6_close(void);
ER MP_H265Dec6_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec6_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec6_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec6_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec6_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec6_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec7_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec7_close(void);
ER MP_H265Dec7_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec7_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec7_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec7_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec7_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec7_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec8_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec8_close(void);
ER MP_H265Dec8_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec8_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec8_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec8_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec8_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec8_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec9_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec9_close(void);
ER MP_H265Dec9_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec9_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec9_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec9_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec9_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec9_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec10_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec10_close(void);
ER MP_H265Dec10_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec10_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec10_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec10_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec10_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec10_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec11_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec11_close(void);
ER MP_H265Dec11_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec11_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec11_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec11_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec11_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec11_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec12_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec12_close(void);
ER MP_H265Dec12_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec12_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec12_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec12_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec12_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec12_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec13_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec13_close(void);
ER MP_H265Dec13_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec13_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec13_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec13_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec13_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec13_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec14_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec14_close(void);
ER MP_H265Dec14_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec14_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec14_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec14_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec14_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec14_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec15_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec15_close(void);
ER MP_H265Dec15_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec15_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec15_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec15_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec15_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec15_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);

ER MP_H265Dec16_init(MP_VDODEC_INIT *pVidDecInit);
ER MP_H265Dec16_close(void);
ER MP_H265Dec16_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER MP_H265Dec16_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
#ifdef VDODEC_LL
ER MP_H265Dec16_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
#else
ER MP_H265Dec16_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam);
#endif
ER MP_H265Dec16_triggerDec(MP_VDODEC_PARAM *pVidDecParam);
ER MP_H265Dec16_waitDecReady(MP_VDODEC_PARAM *pVidDecParam);


MP_VDODEC_DECODER gH265VideoDecObj[MP_VDODEC_ID_MAX] = {
	{
		MP_H265Dec1_init,           //Initialize
		MP_H265Dec1_close,          //Close
		MP_H265Dec1_getInfo,        //GetInfo
		MP_H265Dec1_setInfo,        //SetInfo
		MP_H265Dec1_decodeOne,      //DecodeOne
		MP_H265Dec1_triggerDec,     //TriggerDec
		MP_H265Dec1_waitDecReady,   //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec2_init,           //Initialize
		MP_H265Dec2_close,          //Close
		MP_H265Dec2_getInfo,        //GetInfo
		MP_H265Dec2_setInfo,        //SetInfo
		MP_H265Dec2_decodeOne,      //DecodeOne
		MP_H265Dec2_triggerDec,     //TriggerDec
		MP_H265Dec2_waitDecReady,   //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec3_init,           //Initialize
		MP_H265Dec3_close,          //Close
		MP_H265Dec3_getInfo,        //GetInfo
		MP_H265Dec3_setInfo,        //SetInfo
		MP_H265Dec3_decodeOne,      //DecodeOne
		MP_H265Dec3_triggerDec,     //TriggerDec
		MP_H265Dec3_waitDecReady,   //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec4_init,           //Initialize
		MP_H265Dec4_close,          //Close
		MP_H265Dec4_getInfo,        //GetInfo
		MP_H265Dec4_setInfo,        //SetInfo
		MP_H265Dec4_decodeOne,      //DecodeOne
		MP_H265Dec4_triggerDec,     //TriggerDec
		MP_H265Dec4_waitDecReady,   //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec5_init,           //Initialize
		MP_H265Dec5_close,          //Close
		MP_H265Dec5_getInfo,        //GetInfo
		MP_H265Dec5_setInfo,        //SetInfo
		MP_H265Dec5_decodeOne,      //DecodeOne
		MP_H265Dec5_triggerDec,     //TriggerDec
		MP_H265Dec5_waitDecReady,   //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec6_init,           //Initialize
		MP_H265Dec6_close,          //Close
		MP_H265Dec6_getInfo,        //GetInfo
		MP_H265Dec6_setInfo,        //SetInfo
		MP_H265Dec6_decodeOne,      //DecodeOne
		MP_H265Dec6_triggerDec,     //TriggerDec
		MP_H265Dec6_waitDecReady,   //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec7_init,           //Initialize
		MP_H265Dec7_close,          //Close
		MP_H265Dec7_getInfo,        //GetInfo
		MP_H265Dec7_setInfo,        //SetInfo
		MP_H265Dec7_decodeOne,      //DecodeOne
		MP_H265Dec7_triggerDec,     //TriggerDec
		MP_H265Dec7_waitDecReady,   //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec8_init,           //Initialize
		MP_H265Dec8_close,          //Close
		MP_H265Dec8_getInfo,        //GetInfo
		MP_H265Dec8_setInfo,        //SetInfo
		MP_H265Dec8_decodeOne,      //DecodeOne
		MP_H265Dec8_triggerDec,     //TriggerDec
		MP_H265Dec8_waitDecReady,   //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec9_init,           //Initialize
		MP_H265Dec9_close,          //Close
		MP_H265Dec9_getInfo,        //GetInfo
		MP_H265Dec9_setInfo,        //SetInfo
		MP_H265Dec9_decodeOne,      //DecodeOne
		MP_H265Dec9_triggerDec,     //TriggerDec
		MP_H265Dec9_waitDecReady,   //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec10_init,          //Initialize
		MP_H265Dec10_close,         //Close
		MP_H265Dec10_getInfo,       //GetInfo
		MP_H265Dec10_setInfo,       //SetInfo
		MP_H265Dec10_decodeOne,     //DecodeOne
		MP_H265Dec10_triggerDec,    //TriggerDec
		MP_H265Dec10_waitDecReady,  //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec11_init,          //Initialize
		MP_H265Dec11_close,         //Close
		MP_H265Dec11_getInfo,       //GetInfo
		MP_H265Dec11_setInfo,       //SetInfo
		MP_H265Dec11_decodeOne,     //DecodeOne
		MP_H265Dec11_triggerDec,    //TriggerDec
		MP_H265Dec11_waitDecReady,  //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec12_init,          //Initialize
		MP_H265Dec12_close,         //Close
		MP_H265Dec12_getInfo,       //GetInfo
		MP_H265Dec12_setInfo,       //SetInfo
		MP_H265Dec12_decodeOne,     //DecodeOne
		MP_H265Dec12_triggerDec,    //TriggerDec
		MP_H265Dec12_waitDecReady,  //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec13_init,          //Initialize
		MP_H265Dec13_close,         //Close
		MP_H265Dec13_getInfo,       //GetInfo
		MP_H265Dec13_setInfo,       //SetInfo
		MP_H265Dec13_decodeOne,     //DecodeOne
		MP_H265Dec13_triggerDec,    //TriggerDec
		MP_H265Dec13_waitDecReady,  //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec14_init,          //Initialize
		MP_H265Dec14_close,         //Close
		MP_H265Dec14_getInfo,       //GetInfo
		MP_H265Dec14_setInfo,       //SetInfo
		MP_H265Dec14_decodeOne,     //DecodeOne
		MP_H265Dec14_triggerDec,    //TriggerDec
		MP_H265Dec14_waitDecReady,  //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec15_init,          //Initialize
		MP_H265Dec15_close,         //Close
		MP_H265Dec15_getInfo,       //GetInfo
		MP_H265Dec15_setInfo,       //SetInfo
		MP_H265Dec15_decodeOne,     //DecodeOne
		MP_H265Dec15_triggerDec,    //TriggerDec
		MP_H265Dec15_waitDecReady,  //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	},

	{
		MP_H265Dec16_init,          //Initialize
		MP_H265Dec16_close,         //Close
		MP_H265Dec16_getInfo,       //GetInfo
		MP_H265Dec16_setInfo,       //SetInfo
		MP_H265Dec16_decodeOne,     //DecodeOne
		MP_H265Dec16_triggerDec,    //TriggerDec
		MP_H265Dec16_waitDecReady,  //WaitDecReady
		NULL,                       //AdjustBPS
		NULL,                       //CustomizeFunc
		H265_DECODEID               //checkID
	}
};

ER MP_H265Dec1_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_1, pVidDecInit);
}

ER MP_H265Dec1_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_1);
}

ER MP_H265Dec1_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_1, type, p1, p2, p3);
}

ER MP_H265Dec1_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_1, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec1_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_1, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec1_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_1, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec1_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_1, pVidDecParam);
}

ER MP_H265Dec1_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_1, pVidDecParam);
}

ER MP_H265Dec2_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_2, pVidDecInit);
}

ER MP_H265Dec2_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_2);
}

ER MP_H265Dec2_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_2, type, p1, p2, p3);
}

ER MP_H265Dec2_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_2, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec2_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_2, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec2_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_2, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec2_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_2, pVidDecParam);
}

ER MP_H265Dec2_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_2, pVidDecParam);
}

ER MP_H265Dec3_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_3, pVidDecInit);
}

ER MP_H265Dec3_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_3);
}

ER MP_H265Dec3_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_3, type, p1, p2, p3);
}

ER MP_H265Dec3_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_3, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec3_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_3, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec3_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_3, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec3_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_3, pVidDecParam);
}

ER MP_H265Dec3_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_3, pVidDecParam);
}

ER MP_H265Dec4_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_4, pVidDecInit);
}

ER MP_H265Dec4_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_4);
}

ER MP_H265Dec4_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_4, type, p1, p2, p3);
}

ER MP_H265Dec4_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_4, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec4_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_4, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec4_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_4, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec4_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_4, pVidDecParam);
}

ER MP_H265Dec4_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_4, pVidDecParam);
}

ER MP_H265Dec5_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_5, pVidDecInit);
}

ER MP_H265Dec5_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_5);
}

ER MP_H265Dec5_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_5, type, p1, p2, p3);
}

ER MP_H265Dec5_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_5, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec5_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_5, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec5_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_5, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec5_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_5, pVidDecParam);
}

ER MP_H265Dec5_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_5, pVidDecParam);
}

ER MP_H265Dec6_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_6, pVidDecInit);
}

ER MP_H265Dec6_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_6);
}

ER MP_H265Dec6_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_6, type, p1, p2, p3);
}

ER MP_H265Dec6_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_6, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec6_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_6, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec6_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_6, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec6_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_6, pVidDecParam);
}

ER MP_H265Dec6_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_6, pVidDecParam);
}

ER MP_H265Dec7_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_7, pVidDecInit);
}

ER MP_H265Dec7_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_7);
}

ER MP_H265Dec7_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_7, type, p1, p2, p3);
}

ER MP_H265Dec7_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_7, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec7_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_7, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec7_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_7, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec7_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_7, pVidDecParam);
}

ER MP_H265Dec7_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_7, pVidDecParam);
}

ER MP_H265Dec8_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_8, pVidDecInit);
}

ER MP_H265Dec8_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_8);
}

ER MP_H265Dec8_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_8, type, p1, p2, p3);
}

ER MP_H265Dec8_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_8, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec8_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_8, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec8_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_8, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec8_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_8, pVidDecParam);
}

ER MP_H265Dec8_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_8, pVidDecParam);
}

ER MP_H265Dec9_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_9, pVidDecInit);
}

ER MP_H265Dec9_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_9);
}

ER MP_H265Dec9_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_9, type, p1, p2, p3);
}

ER MP_H265Dec9_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_9, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec9_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_9, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec9_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_9, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec9_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_9, pVidDecParam);
}

ER MP_H265Dec9_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_9, pVidDecParam);
}

ER MP_H265Dec10_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_10, pVidDecInit);
}

ER MP_H265Dec10_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_10);
}

ER MP_H265Dec10_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_10, type, p1, p2, p3);
}

ER MP_H265Dec10_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_10, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec10_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_10, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec10_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_10, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec10_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_10, pVidDecParam);
}

ER MP_H265Dec10_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_10, pVidDecParam);
}

ER MP_H265Dec11_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_11, pVidDecInit);
}

ER MP_H265Dec11_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_11);
}

ER MP_H265Dec11_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_11, type, p1, p2, p3);
}

ER MP_H265Dec11_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_11, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec11_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_11, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec11_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_11, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec11_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_11, pVidDecParam);
}

ER MP_H265Dec11_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_11, pVidDecParam);
}

ER MP_H265Dec12_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_12, pVidDecInit);
}

ER MP_H265Dec12_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_12);
}

ER MP_H265Dec12_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_12, type, p1, p2, p3);
}

ER MP_H265Dec12_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_12, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec12_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_12, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec12_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_12, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec12_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_12, pVidDecParam);
}

ER MP_H265Dec12_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_12, pVidDecParam);
}

ER MP_H265Dec13_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_13, pVidDecInit);
}

ER MP_H265Dec13_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_13);
}

ER MP_H265Dec13_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_13, type, p1, p2, p3);
}

ER MP_H265Dec13_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_13, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec13_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_13, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec13_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_13, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec13_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_13, pVidDecParam);
}

ER MP_H265Dec13_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_13, pVidDecParam);
}

ER MP_H265Dec14_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_14, pVidDecInit);
}

ER MP_H265Dec14_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_14);
}

ER MP_H265Dec14_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_14, type, p1, p2, p3);
}

ER MP_H265Dec14_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_14, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec14_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_14, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec14_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_14, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec14_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_14, pVidDecParam);
}

ER MP_H265Dec14_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_14, pVidDecParam);
}

ER MP_H265Dec15_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_15, pVidDecInit);
}

ER MP_H265Dec15_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_15);
}

ER MP_H265Dec15_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_15, type, p1, p2, p3);
}

ER MP_H265Dec15_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_15, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec15_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_15, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec15_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_15, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec15_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_15, pVidDecParam);
}

ER MP_H265Dec15_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_15, pVidDecParam);
}

ER MP_H265Dec16_init(MP_VDODEC_INIT *pVidDecInit)
{
	return MP_H265Dec_init(MP_VDODEC_ID_16, pVidDecInit);
}

ER MP_H265Dec16_close(void)
{
	return MP_H265Dec_close(MP_VDODEC_ID_16);
}

ER MP_H265Dec16_getInfo(MP_VDODEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_H265Dec_getInfo(MP_VDODEC_ID_16, type, p1, p2, p3);
}

ER MP_H265Dec16_setInfo(MP_VDODEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_H265Dec_setInfo(MP_VDODEC_ID_16, type, param1, param2, param3);
}

#ifdef VDODEC_LL
ER MP_H265Dec16_decodeOne(UINT32 type, MP_VDODEC_PARAM *pVidDecParam, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_16, type, pVidDecParam, p_cb_func, p_user_data);
}
#else
ER MP_H265Dec16_decodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_decodeOne(MP_VDODEC_ID_16, type, outputAddr, pSize, pVidDecParam);
}
#endif

ER MP_H265Dec16_triggerDec(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_triggerDec(MP_VDODEC_ID_16, pVidDecParam);
}

ER MP_H265Dec16_waitDecReady(MP_VDODEC_PARAM *pVidDecParam)
{
	return MP_H265Dec_waitDecReady(MP_VDODEC_ID_16, pVidDecParam);
}

/**
    Get h.265 decoding object.

    Get h.265 decoding object.

    @return video codec decoding object
*/
PMP_VDODEC_DECODER MP_H265Dec_getVideoObject(MP_VDODEC_ID VidDecId)
{
	return &gH265VideoDecObj[VidDecId];
}

// for backward compatible
PMP_VDODEC_DECODER MP_H265Dec_getVideoDecodeObject(void)
{
	return &gH265VideoDecObj[0];
}
