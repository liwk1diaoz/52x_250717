
#ifdef __KERNEL__
#include "mp_ulaw_encoder.h"
#include "audio_codec_ulaw.h"
#include "kwrap/semaphore.h"
#else
#include "mp_ulaw_encoder.h"
#include "audio_codec_ulaw.h"
#include "kwrap/semaphore.h"
#endif

#define __MODULE__          ulaw_encobj
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int ulaw_encobj_debug_level = NVT_DBG_WRN;

#define ULAW_ENCODEID   0x756c6177 // "ulaw"

extern ER MP_uLawEnc1_init(void);
extern ER MP_uLawEnc1_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc1_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc1_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc2_init(void);
extern ER MP_uLawEnc2_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc2_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc2_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc3_init(void);
extern ER MP_uLawEnc3_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc3_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc3_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc4_init(void);
extern ER MP_uLawEnc4_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc4_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc4_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc5_init(void);
extern ER MP_uLawEnc5_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc5_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc5_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc6_init(void);
extern ER MP_uLawEnc6_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc6_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc6_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc7_init(void);
extern ER MP_uLawEnc7_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc7_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc7_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc8_init(void);
extern ER MP_uLawEnc8_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc8_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc8_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc9_init(void);
extern ER MP_uLawEnc9_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc9_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc9_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc10_init(void);
extern ER MP_uLawEnc10_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc10_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc10_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc11_init(void);
extern ER MP_uLawEnc11_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc11_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc11_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc12_init(void);
extern ER MP_uLawEnc12_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc12_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc12_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc13_init(void);
extern ER MP_uLawEnc13_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc13_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc13_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc14_init(void);
extern ER MP_uLawEnc14_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc14_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc14_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc15_init(void);
extern ER MP_uLawEnc15_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc15_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc15_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_uLawEnc16_init(void);
extern ER MP_uLawEnc16_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawEnc16_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawEnc16_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

MP_AUDENC_ENCODER guLawAudioEncObj[MP_AUDENC_ID_MAX] = {
	{
		MP_uLawEnc1_init,        //Initialize
		MP_uLawEnc1_getInfo,     //GetInfo
		MP_uLawEnc1_setInfo,     //SetInfo
		MP_uLawEnc1_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ULAW_ENCODEID            //checkID
	},

	{
		MP_uLawEnc2_init,        //Initialize
		MP_uLawEnc2_getInfo,     //GetInfo
		MP_uLawEnc2_setInfo,     //SetInfo
		MP_uLawEnc2_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ULAW_ENCODEID            //checkID
	},

	{
		MP_uLawEnc3_init,        //Initialize
		MP_uLawEnc3_getInfo,     //GetInfo
		MP_uLawEnc3_setInfo,     //SetInfo
		MP_uLawEnc3_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ULAW_ENCODEID            //checkID
	},

	{
		MP_uLawEnc4_init,        //Initialize
		MP_uLawEnc4_getInfo,     //GetInfo
		MP_uLawEnc4_setInfo,     //SetInfo
		MP_uLawEnc4_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ULAW_ENCODEID            //checkID
	},

	{
		MP_uLawEnc5_init,        //Initialize
		MP_uLawEnc5_getInfo,     //GetInfo
		MP_uLawEnc5_setInfo,     //SetInfo
		MP_uLawEnc5_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ULAW_ENCODEID            //checkID
	},

	{
		MP_uLawEnc6_init,        //Initialize
		MP_uLawEnc6_getInfo,     //GetInfo
		MP_uLawEnc6_setInfo,     //SetInfo
		MP_uLawEnc6_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ULAW_ENCODEID            //checkID
	},

	{
		MP_uLawEnc7_init,        //Initialize
		MP_uLawEnc7_getInfo,     //GetInfo
		MP_uLawEnc7_setInfo,     //SetInfo
		MP_uLawEnc7_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ULAW_ENCODEID            //checkID
	},

	{
		MP_uLawEnc8_init,        //Initialize
		MP_uLawEnc8_getInfo,     //GetInfo
		MP_uLawEnc8_setInfo,     //SetInfo
		MP_uLawEnc8_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ULAW_ENCODEID            //checkID
	},

	{
		MP_uLawEnc9_init,        //Initialize
		MP_uLawEnc9_getInfo,     //GetInfo
		MP_uLawEnc9_setInfo,     //SetInfo
		MP_uLawEnc9_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ULAW_ENCODEID            //checkID
	},

	{
		MP_uLawEnc10_init,        //Initialize
		MP_uLawEnc10_getInfo,     //GetInfo
		MP_uLawEnc10_setInfo,     //SetInfo
		MP_uLawEnc10_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		ULAW_ENCODEID             //checkID
	},

	{
		MP_uLawEnc11_init,        //Initialize
		MP_uLawEnc11_getInfo,     //GetInfo
		MP_uLawEnc11_setInfo,     //SetInfo
		MP_uLawEnc11_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		ULAW_ENCODEID             //checkID
	},

	{
		MP_uLawEnc12_init,        //Initialize
		MP_uLawEnc12_getInfo,     //GetInfo
		MP_uLawEnc12_setInfo,     //SetInfo
		MP_uLawEnc12_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		ULAW_ENCODEID             //checkID
	},

	{
		MP_uLawEnc13_init,        //Initialize
		MP_uLawEnc13_getInfo,     //GetInfo
		MP_uLawEnc13_setInfo,     //SetInfo
		MP_uLawEnc13_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		ULAW_ENCODEID             //checkID
	},

	{
		MP_uLawEnc14_init,        //Initialize
		MP_uLawEnc14_getInfo,     //GetInfo
		MP_uLawEnc14_setInfo,     //SetInfo
		MP_uLawEnc14_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		ULAW_ENCODEID             //checkID
	},

	{
		MP_uLawEnc15_init,        //Initialize
		MP_uLawEnc15_getInfo,     //GetInfo
		MP_uLawEnc15_setInfo,     //SetInfo
		MP_uLawEnc15_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		ULAW_ENCODEID             //checkID
	},

	{
		MP_uLawEnc16_init,        //Initialize
		MP_uLawEnc16_getInfo,     //GetInfo
		MP_uLawEnc16_setInfo,     //SetInfo
		MP_uLawEnc16_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		ULAW_ENCODEID             //checkID
	},
};

ER MP_uLawEnc1_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_1);
}

ER MP_uLawEnc1_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_1, type, p1, p2, p3);
}

ER MP_uLawEnc1_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_1, type, param1, param2, param3);
}

ER MP_uLawEnc1_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_1, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc2_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_2);
}

ER MP_uLawEnc2_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_2, type, p1, p2, p3);
}

ER MP_uLawEnc2_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_2, type, param1, param2, param3);
}

ER MP_uLawEnc2_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_2, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc3_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_3);
}

ER MP_uLawEnc3_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_3, type, p1, p2, p3);
}

ER MP_uLawEnc3_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_3, type, param1, param2, param3);
}

ER MP_uLawEnc3_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_3, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc4_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_4);
}

ER MP_uLawEnc4_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_4, type, p1, p2, p3);
}

ER MP_uLawEnc4_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_4, type, param1, param2, param3);
}

ER MP_uLawEnc4_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_4, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc5_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_5);
}

ER MP_uLawEnc5_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_5, type, p1, p2, p3);
}

ER MP_uLawEnc5_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_5, type, param1, param2, param3);
}

ER MP_uLawEnc5_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_5, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc6_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_6);
}

ER MP_uLawEnc6_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_6, type, p1, p2, p3);
}

ER MP_uLawEnc6_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_6, type, param1, param2, param3);
}

ER MP_uLawEnc6_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_6, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc7_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_7);
}

ER MP_uLawEnc7_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_7, type, p1, p2, p3);
}

ER MP_uLawEnc7_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_7, type, param1, param2, param3);
}

ER MP_uLawEnc7_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_7, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc8_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_8);
}

ER MP_uLawEnc8_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_8, type, p1, p2, p3);
}

ER MP_uLawEnc8_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_8, type, param1, param2, param3);
}

ER MP_uLawEnc8_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_8, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc9_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_9);
}

ER MP_uLawEnc9_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_9, type, p1, p2, p3);
}

ER MP_uLawEnc9_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_9, type, param1, param2, param3);
}

ER MP_uLawEnc9_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_9, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc10_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_10);
}

ER MP_uLawEnc10_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_10, type, p1, p2, p3);
}

ER MP_uLawEnc10_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_10, type, param1, param2, param3);
}

ER MP_uLawEnc10_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_10, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc11_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_11);
}

ER MP_uLawEnc11_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_11, type, p1, p2, p3);
}

ER MP_uLawEnc11_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_11, type, param1, param2, param3);
}

ER MP_uLawEnc11_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_11, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc12_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_12);
}

ER MP_uLawEnc12_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_12, type, p1, p2, p3);
}

ER MP_uLawEnc12_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_12, type, param1, param2, param3);
}

ER MP_uLawEnc12_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_12, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc13_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_13);
}

ER MP_uLawEnc13_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_13, type, p1, p2, p3);
}

ER MP_uLawEnc13_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_13, type, param1, param2, param3);
}

ER MP_uLawEnc13_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_13, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc14_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_14);
}

ER MP_uLawEnc14_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_14, type, p1, p2, p3);
}

ER MP_uLawEnc14_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_14, type, param1, param2, param3);
}

ER MP_uLawEnc14_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_14, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc15_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_15);
}

ER MP_uLawEnc15_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_15, type, p1, p2, p3);
}

ER MP_uLawEnc15_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_15, type, param1, param2, param3);
}

ER MP_uLawEnc15_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_15, type, outputAddr, pSize, pAudEncParam);
}

ER MP_uLawEnc16_init(void)
{
	return MP_uLawEnc_init(MP_AUDENC_ID_16);
}

ER MP_uLawEnc16_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawEnc_getInfo(MP_AUDENC_ID_16, type, p1, p2, p3);
}

ER MP_uLawEnc16_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawEnc_setInfo(MP_AUDENC_ID_16, type, param1, param2, param3);
}

ER MP_uLawEnc16_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_uLawEnc_encodeOne(MP_AUDENC_ID_16, type, outputAddr, pSize, pAudEncParam);
}

/**
    Get uLaw encoding object.

    Get uLaw encoding object.

    @return audio codec encoding object
*/
PMP_AUDENC_ENCODER MP_uLawEnc_getAudioObject(MP_AUDENC_ID AudEncId)
{
	return &guLawAudioEncObj[AudEncId];
}

// for backward compatible
PMP_AUDENC_ENCODER MP_uLawEnc_getAudioEncodeObject(void)
{
	return &guLawAudioEncObj[0];
}

