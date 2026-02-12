
#ifdef __KERNEL__
#include "mp_alaw_encoder.h"
#include "audio_codec_alaw.h"
#include "kwrap/semaphore.h"
#else
#include "mp_alaw_encoder.h"
#include "audio_codec_alaw.h"
#include "kwrap/semaphore.h"
#endif

#define __MODULE__          alaw_encobj
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int alaw_encobj_debug_level = NVT_DBG_WRN;

#define ALAW_ENCODEID   0x616c6177 // "alaw"

extern ER MP_aLawEnc1_init(void);
extern ER MP_aLawEnc1_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc1_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc1_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc2_init(void);
extern ER MP_aLawEnc2_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc2_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc2_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc3_init(void);
extern ER MP_aLawEnc3_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc3_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc3_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc4_init(void);
extern ER MP_aLawEnc4_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc4_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc4_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc5_init(void);
extern ER MP_aLawEnc5_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc5_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc5_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc6_init(void);
extern ER MP_aLawEnc6_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc6_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc6_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc7_init(void);
extern ER MP_aLawEnc7_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc7_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc7_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc8_init(void);
extern ER MP_aLawEnc8_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc8_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc8_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc9_init(void);
extern ER MP_aLawEnc9_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc9_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc9_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc10_init(void);
extern ER MP_aLawEnc10_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc10_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc10_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc11_init(void);
extern ER MP_aLawEnc11_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc11_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc11_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc12_init(void);
extern ER MP_aLawEnc12_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc12_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc12_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc13_init(void);
extern ER MP_aLawEnc13_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc13_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc13_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc14_init(void);
extern ER MP_aLawEnc14_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc14_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc14_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc15_init(void);
extern ER MP_aLawEnc15_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc15_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc15_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_aLawEnc16_init(void);
extern ER MP_aLawEnc16_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawEnc16_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawEnc16_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

MP_AUDENC_ENCODER gaLawAudioEncObj[MP_AUDENC_ID_MAX] = {
	{
		MP_aLawEnc1_init,        //Initialize
		MP_aLawEnc1_getInfo,     //GetInfo
		MP_aLawEnc1_setInfo,     //SetInfo
		MP_aLawEnc1_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ALAW_ENCODEID            //checkID
	},

	{
		MP_aLawEnc2_init,        //Initialize
		MP_aLawEnc2_getInfo,     //GetInfo
		MP_aLawEnc2_setInfo,     //SetInfo
		MP_aLawEnc2_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ALAW_ENCODEID            //checkID
	},

	{
		MP_aLawEnc3_init,        //Initialize
		MP_aLawEnc3_getInfo,     //GetInfo
		MP_aLawEnc3_setInfo,     //SetInfo
		MP_aLawEnc3_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ALAW_ENCODEID            //checkID
	},

	{
		MP_aLawEnc4_init,        //Initialize
		MP_aLawEnc4_getInfo,     //GetInfo
		MP_aLawEnc4_setInfo,     //SetInfo
		MP_aLawEnc4_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ALAW_ENCODEID            //checkID
	},

	{
		MP_aLawEnc5_init,        //Initialize
		MP_aLawEnc5_getInfo,     //GetInfo
		MP_aLawEnc5_setInfo,     //SetInfo
		MP_aLawEnc5_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ALAW_ENCODEID            //checkID
	},

	{
		MP_aLawEnc6_init,        //Initialize
		MP_aLawEnc6_getInfo,     //GetInfo
		MP_aLawEnc6_setInfo,     //SetInfo
		MP_aLawEnc6_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ALAW_ENCODEID            //checkID
	},

	{
		MP_aLawEnc7_init,        //Initialize
		MP_aLawEnc7_getInfo,     //GetInfo
		MP_aLawEnc7_setInfo,     //SetInfo
		MP_aLawEnc7_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ALAW_ENCODEID            //checkID
	},

	{
		MP_aLawEnc8_init,        //Initialize
		MP_aLawEnc8_getInfo,     //GetInfo
		MP_aLawEnc8_setInfo,     //SetInfo
		MP_aLawEnc8_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ALAW_ENCODEID            //checkID
	},

	{
		MP_aLawEnc9_init,        //Initialize
		MP_aLawEnc9_getInfo,     //GetInfo
		MP_aLawEnc9_setInfo,     //SetInfo
		MP_aLawEnc9_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		ALAW_ENCODEID            //checkID
	},

	{
		MP_aLawEnc10_init,        //Initialize
		MP_aLawEnc10_getInfo,     //GetInfo
		MP_aLawEnc10_setInfo,     //SetInfo
		MP_aLawEnc10_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		ALAW_ENCODEID             //checkID
	},

	{
		MP_aLawEnc11_init,        //Initialize
		MP_aLawEnc11_getInfo,     //GetInfo
		MP_aLawEnc11_setInfo,     //SetInfo
		MP_aLawEnc11_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		ALAW_ENCODEID             //checkID
	},

	{
		MP_aLawEnc12_init,        //Initialize
		MP_aLawEnc12_getInfo,     //GetInfo
		MP_aLawEnc12_setInfo,     //SetInfo
		MP_aLawEnc12_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		ALAW_ENCODEID             //checkID
	},

	{
		MP_aLawEnc13_init,        //Initialize
		MP_aLawEnc13_getInfo,     //GetInfo
		MP_aLawEnc13_setInfo,     //SetInfo
		MP_aLawEnc13_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		ALAW_ENCODEID             //checkID
	},

	{
		MP_aLawEnc14_init,        //Initialize
		MP_aLawEnc14_getInfo,     //GetInfo
		MP_aLawEnc14_setInfo,     //SetInfo
		MP_aLawEnc14_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		ALAW_ENCODEID             //checkID
	},

	{
		MP_aLawEnc15_init,        //Initialize
		MP_aLawEnc15_getInfo,     //GetInfo
		MP_aLawEnc15_setInfo,     //SetInfo
		MP_aLawEnc15_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		ALAW_ENCODEID             //checkID
	},

	{
		MP_aLawEnc16_init,        //Initialize
		MP_aLawEnc16_getInfo,     //GetInfo
		MP_aLawEnc16_setInfo,     //SetInfo
		MP_aLawEnc16_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		ALAW_ENCODEID             //checkID
	},
};

ER MP_aLawEnc1_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_1);
}

ER MP_aLawEnc1_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_1, type, p1, p2, p3);
}

ER MP_aLawEnc1_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_1, type, param1, param2, param3);
}

ER MP_aLawEnc1_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_1, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc2_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_2);
}

ER MP_aLawEnc2_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_2, type, p1, p2, p3);
}

ER MP_aLawEnc2_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_2, type, param1, param2, param3);
}

ER MP_aLawEnc2_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_2, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc3_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_3);
}

ER MP_aLawEnc3_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_3, type, p1, p2, p3);
}

ER MP_aLawEnc3_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_3, type, param1, param2, param3);
}

ER MP_aLawEnc3_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_3, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc4_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_4);
}

ER MP_aLawEnc4_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_4, type, p1, p2, p3);
}

ER MP_aLawEnc4_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_4, type, param1, param2, param3);
}

ER MP_aLawEnc4_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_4, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc5_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_5);
}

ER MP_aLawEnc5_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_5, type, p1, p2, p3);
}

ER MP_aLawEnc5_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_5, type, param1, param2, param3);
}

ER MP_aLawEnc5_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_5, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc6_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_6);
}

ER MP_aLawEnc6_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_6, type, p1, p2, p3);
}

ER MP_aLawEnc6_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_6, type, param1, param2, param3);
}

ER MP_aLawEnc6_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_6, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc7_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_7);
}

ER MP_aLawEnc7_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_7, type, p1, p2, p3);
}

ER MP_aLawEnc7_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_7, type, param1, param2, param3);
}

ER MP_aLawEnc7_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_7, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc8_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_8);
}

ER MP_aLawEnc8_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_8, type, p1, p2, p3);
}

ER MP_aLawEnc8_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_8, type, param1, param2, param3);
}

ER MP_aLawEnc8_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_8, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc9_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_9);
}

ER MP_aLawEnc9_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_9, type, p1, p2, p3);
}

ER MP_aLawEnc9_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_9, type, param1, param2, param3);
}

ER MP_aLawEnc9_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_9, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc10_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_10);
}

ER MP_aLawEnc10_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_10, type, p1, p2, p3);
}

ER MP_aLawEnc10_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_10, type, param1, param2, param3);
}

ER MP_aLawEnc10_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_10, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc11_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_11);
}

ER MP_aLawEnc11_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_11, type, p1, p2, p3);
}

ER MP_aLawEnc11_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_11, type, param1, param2, param3);
}

ER MP_aLawEnc11_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_11, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc12_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_12);
}

ER MP_aLawEnc12_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_12, type, p1, p2, p3);
}

ER MP_aLawEnc12_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_12, type, param1, param2, param3);
}

ER MP_aLawEnc12_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_12, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc13_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_13);
}

ER MP_aLawEnc13_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_13, type, p1, p2, p3);
}

ER MP_aLawEnc13_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_13, type, param1, param2, param3);
}

ER MP_aLawEnc13_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_13, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc14_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_14);
}

ER MP_aLawEnc14_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_14, type, p1, p2, p3);
}

ER MP_aLawEnc14_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_14, type, param1, param2, param3);
}

ER MP_aLawEnc14_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_14, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc15_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_15);
}

ER MP_aLawEnc15_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_15, type, p1, p2, p3);
}

ER MP_aLawEnc15_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_15, type, param1, param2, param3);
}

ER MP_aLawEnc15_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_15, type, outputAddr, pSize, pAudEncParam);
}

ER MP_aLawEnc16_init(void)
{
	return MP_aLawEnc_init(MP_AUDENC_ID_16);
}

ER MP_aLawEnc16_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawEnc_getInfo(MP_AUDENC_ID_16, type, p1, p2, p3);
}

ER MP_aLawEnc16_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawEnc_setInfo(MP_AUDENC_ID_16, type, param1, param2, param3);
}

ER MP_aLawEnc16_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_aLawEnc_encodeOne(MP_AUDENC_ID_16, type, outputAddr, pSize, pAudEncParam);
}

/**
    Get aLaw encoding object.

    Get aLaw encoding object.

    @return audio codec encoding object
*/
PMP_AUDENC_ENCODER MP_aLawEnc_getAudioObject(MP_AUDENC_ID AudEncId)
{
	return &gaLawAudioEncObj[AudEncId];
}

// for backward compatible
PMP_AUDENC_ENCODER MP_aLawEnc_getAudioEncodeObject(void)
{
	return &gaLawAudioEncObj[0];
}

