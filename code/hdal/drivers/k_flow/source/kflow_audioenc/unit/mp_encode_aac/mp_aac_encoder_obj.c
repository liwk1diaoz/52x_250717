
#ifdef __KERNEL__
#include "mp_aac_encoder.h"
#include "audio_codec_aac.h"
#include "kwrap/semaphore.h"
#else
#include "mp_aac_encoder.h"
#include "audio_codec_aac.h"
#include "kwrap/semaphore.h"
#endif

#define __MODULE__          aac_encobj
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int aac_encobj_debug_level = NVT_DBG_WRN;

#define AAC_ENCODEID 0xFFF96C40

extern ER MP_AACEnc1_init(void);
extern ER MP_AACEnc1_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc1_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc1_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc2_init(void);
extern ER MP_AACEnc2_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc2_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc2_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc3_init(void);
extern ER MP_AACEnc3_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc3_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc3_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc4_init(void);
extern ER MP_AACEnc4_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc4_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc4_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc5_init(void);
extern ER MP_AACEnc5_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc5_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc5_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc6_init(void);
extern ER MP_AACEnc6_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc6_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc6_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc7_init(void);
extern ER MP_AACEnc7_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc7_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc7_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc8_init(void);
extern ER MP_AACEnc8_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc8_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc8_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc9_init(void);
extern ER MP_AACEnc9_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc9_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc9_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc10_init(void);
extern ER MP_AACEnc10_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc10_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc10_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc11_init(void);
extern ER MP_AACEnc11_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc11_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc11_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc12_init(void);
extern ER MP_AACEnc12_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc12_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc12_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc13_init(void);
extern ER MP_AACEnc13_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc13_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc13_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc14_init(void);
extern ER MP_AACEnc14_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc14_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc14_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc15_init(void);
extern ER MP_AACEnc15_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc15_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc15_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_AACEnc16_init(void);
extern ER MP_AACEnc16_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACEnc16_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACEnc16_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

MP_AUDENC_ENCODER gAACAudioEncObj[MP_AUDENC_ID_MAX] = {
	{
		MP_AACEnc1_init,        //Initialize
		MP_AACEnc1_getInfo,     //GetInfo
		MP_AACEnc1_setInfo,     //SetInfo
		MP_AACEnc1_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		AAC_ENCODEID            //checkID
	},

	{
		MP_AACEnc2_init,        //Initialize
		MP_AACEnc2_getInfo,     //GetInfo
		MP_AACEnc2_setInfo,     //SetInfo
		MP_AACEnc2_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		AAC_ENCODEID            //checkID
	},

	{
		MP_AACEnc3_init,        //Initialize
		MP_AACEnc3_getInfo,     //GetInfo
		MP_AACEnc3_setInfo,     //SetInfo
		MP_AACEnc3_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		AAC_ENCODEID            //checkID
	},

	{
		MP_AACEnc4_init,        //Initialize
		MP_AACEnc4_getInfo,     //GetInfo
		MP_AACEnc4_setInfo,     //SetInfo
		MP_AACEnc4_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		AAC_ENCODEID            //checkID
	},

	{
		MP_AACEnc5_init,        //Initialize
		MP_AACEnc5_getInfo,     //GetInfo
		MP_AACEnc5_setInfo,     //SetInfo
		MP_AACEnc5_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		AAC_ENCODEID            //checkID
	},

	{
		MP_AACEnc6_init,        //Initialize
		MP_AACEnc6_getInfo,     //GetInfo
		MP_AACEnc6_setInfo,     //SetInfo
		MP_AACEnc6_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		AAC_ENCODEID            //checkID
	},

	{
		MP_AACEnc7_init,        //Initialize
		MP_AACEnc7_getInfo,     //GetInfo
		MP_AACEnc7_setInfo,     //SetInfo
		MP_AACEnc7_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		AAC_ENCODEID            //checkID
	},

	{
		MP_AACEnc8_init,        //Initialize
		MP_AACEnc8_getInfo,     //GetInfo
		MP_AACEnc8_setInfo,     //SetInfo
		MP_AACEnc8_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		AAC_ENCODEID            //checkID
	},

	{
		MP_AACEnc9_init,        //Initialize
		MP_AACEnc9_getInfo,     //GetInfo
		MP_AACEnc9_setInfo,     //SetInfo
		MP_AACEnc9_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		AAC_ENCODEID            //checkID
	},

	{
		MP_AACEnc10_init,        //Initialize
		MP_AACEnc10_getInfo,     //GetInfo
		MP_AACEnc10_setInfo,     //SetInfo
		MP_AACEnc10_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		AAC_ENCODEID             //checkID
	},

	{
		MP_AACEnc11_init,        //Initialize
		MP_AACEnc11_getInfo,     //GetInfo
		MP_AACEnc11_setInfo,     //SetInfo
		MP_AACEnc11_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		AAC_ENCODEID             //checkID
	},

	{
		MP_AACEnc12_init,        //Initialize
		MP_AACEnc12_getInfo,     //GetInfo
		MP_AACEnc12_setInfo,     //SetInfo
		MP_AACEnc12_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		AAC_ENCODEID             //checkID
	},

	{
		MP_AACEnc13_init,        //Initialize
		MP_AACEnc13_getInfo,     //GetInfo
		MP_AACEnc13_setInfo,     //SetInfo
		MP_AACEnc13_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		AAC_ENCODEID             //checkID
	},

	{
		MP_AACEnc14_init,        //Initialize
		MP_AACEnc14_getInfo,     //GetInfo
		MP_AACEnc14_setInfo,     //SetInfo
		MP_AACEnc14_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		AAC_ENCODEID             //checkID
	},

	{
		MP_AACEnc15_init,        //Initialize
		MP_AACEnc15_getInfo,     //GetInfo
		MP_AACEnc15_setInfo,     //SetInfo
		MP_AACEnc15_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		AAC_ENCODEID             //checkID
	},

	{
		MP_AACEnc16_init,        //Initialize
		MP_AACEnc16_getInfo,     //GetInfo
		MP_AACEnc16_setInfo,     //SetInfo
		MP_AACEnc16_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		AAC_ENCODEID             //checkID
	},
};

ER MP_AACEnc1_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_1);
}

ER MP_AACEnc1_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_1, type, p1, p2, p3);
}

ER MP_AACEnc1_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_1, type, param1, param2, param3);
}

ER MP_AACEnc1_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_1, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc2_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_2);
}

ER MP_AACEnc2_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_2, type, p1, p2, p3);
}

ER MP_AACEnc2_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_2, type, param1, param2, param3);
}

ER MP_AACEnc2_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_2, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc3_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_3);
}

ER MP_AACEnc3_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_3, type, p1, p2, p3);
}

ER MP_AACEnc3_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_3, type, param1, param2, param3);
}

ER MP_AACEnc3_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_3, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc4_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_4);
}

ER MP_AACEnc4_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_4, type, p1, p2, p3);
}

ER MP_AACEnc4_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_4, type, param1, param2, param3);
}

ER MP_AACEnc4_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_4, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc5_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_5);
}

ER MP_AACEnc5_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_5, type, p1, p2, p3);
}

ER MP_AACEnc5_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_5, type, param1, param2, param3);
}

ER MP_AACEnc5_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_5, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc6_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_6);
}

ER MP_AACEnc6_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_6, type, p1, p2, p3);
}

ER MP_AACEnc6_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_6, type, param1, param2, param3);
}

ER MP_AACEnc6_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_6, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc7_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_7);
}

ER MP_AACEnc7_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_7, type, p1, p2, p3);
}

ER MP_AACEnc7_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_7, type, param1, param2, param3);
}

ER MP_AACEnc7_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_7, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc8_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_8);
}

ER MP_AACEnc8_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_8, type, p1, p2, p3);
}

ER MP_AACEnc8_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_8, type, param1, param2, param3);
}

ER MP_AACEnc8_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_8, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc9_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_9);
}

ER MP_AACEnc9_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_9, type, p1, p2, p3);
}

ER MP_AACEnc9_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_9, type, param1, param2, param3);
}

ER MP_AACEnc9_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_9, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc10_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_10);
}

ER MP_AACEnc10_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_10, type, p1, p2, p3);
}

ER MP_AACEnc10_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_10, type, param1, param2, param3);
}

ER MP_AACEnc10_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_10, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc11_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_11);
}

ER MP_AACEnc11_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_11, type, p1, p2, p3);
}

ER MP_AACEnc11_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_11, type, param1, param2, param3);
}

ER MP_AACEnc11_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_11, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc12_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_12);
}

ER MP_AACEnc12_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_12, type, p1, p2, p3);
}

ER MP_AACEnc12_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_12, type, param1, param2, param3);
}

ER MP_AACEnc12_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_12, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc13_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_13);
}

ER MP_AACEnc13_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_13, type, p1, p2, p3);
}

ER MP_AACEnc13_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_13, type, param1, param2, param3);
}

ER MP_AACEnc13_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_13, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc14_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_14);
}

ER MP_AACEnc14_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_14, type, p1, p2, p3);
}

ER MP_AACEnc14_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_14, type, param1, param2, param3);
}

ER MP_AACEnc14_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_14, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc15_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_15);
}

ER MP_AACEnc15_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_15, type, p1, p2, p3);
}

ER MP_AACEnc15_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_15, type, param1, param2, param3);
}

ER MP_AACEnc15_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_15, type, outputAddr, pSize, pAudEncParam);
}

ER MP_AACEnc16_init(void)
{
	return MP_AACEnc_init(MP_AUDENC_ID_16);
}

ER MP_AACEnc16_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACEnc_getInfo(MP_AUDENC_ID_16, type, p1, p2, p3);
}

ER MP_AACEnc16_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACEnc_setInfo(MP_AUDENC_ID_16, type, param1, param2, param3);
}

ER MP_AACEnc16_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_AACEnc_encodeOne(MP_AUDENC_ID_16, type, outputAddr, pSize, pAudEncParam);
}

/**
    Get AAC encoding object.

    Get AAC encoding object.

    @return audio codec encoding object
*/
PMP_AUDENC_ENCODER MP_AACEnc_getAudioObject(MP_AUDENC_ID AudEncId)
{
	return &gAACAudioEncObj[AudEncId];
}

// for backward compatible
PMP_AUDENC_ENCODER MP_AACEnc_getAudioEncodeObject(void)
{
	return &gAACAudioEncObj[0];
}

