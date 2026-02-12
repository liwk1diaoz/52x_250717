
#ifdef __KERNEL__
#include "mp_pcm_encoder.h"
#include "audio_codec_pcm.h"
#include "kwrap/semaphore.h"
#else
#include "mp_pcm_encoder.h"
#include "audio_codec_pcm.h"
#include "kwrap/semaphore.h"
#endif

#define __MODULE__          pcm_encobj
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int pcm_encobj_debug_level = NVT_DBG_WRN;

#define PCM_ENCODEID 0x736f7774

extern ER MP_PCMEnc1_init(void);
extern ER MP_PCMEnc1_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc1_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc1_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc2_init(void);
extern ER MP_PCMEnc2_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc2_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc2_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc3_init(void);
extern ER MP_PCMEnc3_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc3_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc3_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc4_init(void);
extern ER MP_PCMEnc4_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc4_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc4_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc5_init(void);
extern ER MP_PCMEnc5_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc5_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc5_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc6_init(void);
extern ER MP_PCMEnc6_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc6_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc6_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc7_init(void);
extern ER MP_PCMEnc7_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc7_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc7_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc8_init(void);
extern ER MP_PCMEnc8_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc8_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc8_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc9_init(void);
extern ER MP_PCMEnc9_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc9_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc9_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc10_init(void);
extern ER MP_PCMEnc10_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc10_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc10_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc11_init(void);
extern ER MP_PCMEnc11_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc11_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc11_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc12_init(void);
extern ER MP_PCMEnc12_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc12_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc12_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc13_init(void);
extern ER MP_PCMEnc13_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc13_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc13_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc14_init(void);
extern ER MP_PCMEnc14_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc14_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc14_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc15_init(void);
extern ER MP_PCMEnc15_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc15_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc15_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PCMEnc16_init(void);
extern ER MP_PCMEnc16_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMEnc16_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMEnc16_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

MP_AUDENC_ENCODER gPCMAudioEncObj[MP_AUDENC_ID_MAX] = {
	{
		MP_PCMEnc1_init,        //Initialize
		MP_PCMEnc1_getInfo,     //GetInfo
		MP_PCMEnc1_setInfo,     //SetInfo
		MP_PCMEnc1_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PCM_ENCODEID            //checkID
	},

	{
		MP_PCMEnc2_init,        //Initialize
		MP_PCMEnc2_getInfo,     //GetInfo
		MP_PCMEnc2_setInfo,     //SetInfo
		MP_PCMEnc2_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PCM_ENCODEID            //checkID
	},

	{
		MP_PCMEnc3_init,        //Initialize
		MP_PCMEnc3_getInfo,     //GetInfo
		MP_PCMEnc3_setInfo,     //SetInfo
		MP_PCMEnc3_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PCM_ENCODEID            //checkID
	},

	{
		MP_PCMEnc4_init,        //Initialize
		MP_PCMEnc4_getInfo,     //GetInfo
		MP_PCMEnc4_setInfo,     //SetInfo
		MP_PCMEnc4_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PCM_ENCODEID            //checkID
	},

	{
		MP_PCMEnc5_init,        //Initialize
		MP_PCMEnc5_getInfo,     //GetInfo
		MP_PCMEnc5_setInfo,     //SetInfo
		MP_PCMEnc5_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PCM_ENCODEID            //checkID
	},

	{
		MP_PCMEnc6_init,        //Initialize
		MP_PCMEnc6_getInfo,     //GetInfo
		MP_PCMEnc6_setInfo,     //SetInfo
		MP_PCMEnc6_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PCM_ENCODEID            //checkID
	},

	{
		MP_PCMEnc7_init,        //Initialize
		MP_PCMEnc7_getInfo,     //GetInfo
		MP_PCMEnc7_setInfo,     //SetInfo
		MP_PCMEnc7_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PCM_ENCODEID            //checkID
	},

	{
		MP_PCMEnc8_init,        //Initialize
		MP_PCMEnc8_getInfo,     //GetInfo
		MP_PCMEnc8_setInfo,     //SetInfo
		MP_PCMEnc8_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PCM_ENCODEID            //checkID
	},

	{
		MP_PCMEnc9_init,        //Initialize
		MP_PCMEnc9_getInfo,     //GetInfo
		MP_PCMEnc9_setInfo,     //SetInfo
		MP_PCMEnc9_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PCM_ENCODEID            //checkID
	},

	{
		MP_PCMEnc10_init,        //Initialize
		MP_PCMEnc10_getInfo,     //GetInfo
		MP_PCMEnc10_setInfo,     //SetInfo
		MP_PCMEnc10_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		PCM_ENCODEID             //checkID
	},

	{
		MP_PCMEnc11_init,        //Initialize
		MP_PCMEnc11_getInfo,     //GetInfo
		MP_PCMEnc11_setInfo,     //SetInfo
		MP_PCMEnc11_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		PCM_ENCODEID             //checkID
	},

	{
		MP_PCMEnc12_init,        //Initialize
		MP_PCMEnc12_getInfo,     //GetInfo
		MP_PCMEnc12_setInfo,     //SetInfo
		MP_PCMEnc12_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		PCM_ENCODEID             //checkID
	},

	{
		MP_PCMEnc13_init,        //Initialize
		MP_PCMEnc13_getInfo,     //GetInfo
		MP_PCMEnc13_setInfo,     //SetInfo
		MP_PCMEnc13_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		PCM_ENCODEID             //checkID
	},

	{
		MP_PCMEnc14_init,        //Initialize
		MP_PCMEnc14_getInfo,     //GetInfo
		MP_PCMEnc14_setInfo,     //SetInfo
		MP_PCMEnc14_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		PCM_ENCODEID             //checkID
	},

	{
		MP_PCMEnc15_init,        //Initialize
		MP_PCMEnc15_getInfo,     //GetInfo
		MP_PCMEnc15_setInfo,     //SetInfo
		MP_PCMEnc15_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		PCM_ENCODEID             //checkID
	},

	{
		MP_PCMEnc16_init,        //Initialize
		MP_PCMEnc16_getInfo,     //GetInfo
		MP_PCMEnc16_setInfo,     //SetInfo
		MP_PCMEnc16_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		PCM_ENCODEID             //checkID
	},
};

ER MP_PCMEnc1_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_1);
}

ER MP_PCMEnc1_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_1, type, p1, p2, p3);
}

ER MP_PCMEnc1_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_1, type, param1, param2, param3);
}

ER MP_PCMEnc1_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_1, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc2_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_2);
}

ER MP_PCMEnc2_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_2, type, p1, p2, p3);
}

ER MP_PCMEnc2_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_2, type, param1, param2, param3);
}

ER MP_PCMEnc2_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_2, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc3_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_3);
}

ER MP_PCMEnc3_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_3, type, p1, p2, p3);
}

ER MP_PCMEnc3_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_3, type, param1, param2, param3);
}

ER MP_PCMEnc3_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_3, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc4_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_4);
}

ER MP_PCMEnc4_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_4, type, p1, p2, p3);
}

ER MP_PCMEnc4_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_4, type, param1, param2, param3);
}

ER MP_PCMEnc4_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_4, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc5_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_5);
}

ER MP_PCMEnc5_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_5, type, p1, p2, p3);
}

ER MP_PCMEnc5_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_5, type, param1, param2, param3);
}

ER MP_PCMEnc5_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_5, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc6_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_6);
}

ER MP_PCMEnc6_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_6, type, p1, p2, p3);
}

ER MP_PCMEnc6_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_6, type, param1, param2, param3);
}

ER MP_PCMEnc6_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_6, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc7_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_7);
}

ER MP_PCMEnc7_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_7, type, p1, p2, p3);
}

ER MP_PCMEnc7_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_7, type, param1, param2, param3);
}

ER MP_PCMEnc7_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_7, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc8_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_8);
}

ER MP_PCMEnc8_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_8, type, p1, p2, p3);
}

ER MP_PCMEnc8_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_8, type, param1, param2, param3);
}

ER MP_PCMEnc8_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_8, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc9_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_9);
}

ER MP_PCMEnc9_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_9, type, p1, p2, p3);
}

ER MP_PCMEnc9_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_9, type, param1, param2, param3);
}

ER MP_PCMEnc9_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_9, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc10_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_10);
}

ER MP_PCMEnc10_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_10, type, p1, p2, p3);
}

ER MP_PCMEnc10_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_10, type, param1, param2, param3);
}

ER MP_PCMEnc10_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_10, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc11_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_11);
}

ER MP_PCMEnc11_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_11, type, p1, p2, p3);
}

ER MP_PCMEnc11_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_11, type, param1, param2, param3);
}

ER MP_PCMEnc11_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_11, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc12_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_12);
}

ER MP_PCMEnc12_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_12, type, p1, p2, p3);
}

ER MP_PCMEnc12_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_12, type, param1, param2, param3);
}

ER MP_PCMEnc12_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_12, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc13_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_13);
}

ER MP_PCMEnc13_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_13, type, p1, p2, p3);
}

ER MP_PCMEnc13_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_13, type, param1, param2, param3);
}

ER MP_PCMEnc13_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_13, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc14_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_14);
}

ER MP_PCMEnc14_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_14, type, p1, p2, p3);
}

ER MP_PCMEnc14_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_14, type, param1, param2, param3);
}

ER MP_PCMEnc14_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_14, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc15_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_15);
}

ER MP_PCMEnc15_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_15, type, p1, p2, p3);
}

ER MP_PCMEnc15_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_15, type, param1, param2, param3);
}

ER MP_PCMEnc15_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_15, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PCMEnc16_init(void)
{
	return MP_PCMEnc_init(MP_AUDENC_ID_16);
}

ER MP_PCMEnc16_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMEnc_getInfo(MP_AUDENC_ID_16, type, p1, p2, p3);
}

ER MP_PCMEnc16_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMEnc_setInfo(MP_AUDENC_ID_16, type, param1, param2, param3);
}

ER MP_PCMEnc16_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PCMEnc_encodeOne(MP_AUDENC_ID_16, type, outputAddr, pSize, pAudEncParam);
}

/**
    Get PCM encoding object.

    Get PCM encoding object.

    @return audio codec encoding object
*/
PMP_AUDENC_ENCODER MP_PCMEnc_getAudioObject(MP_AUDENC_ID AudEncId)
{
	return &gPCMAudioEncObj[AudEncId];
}

// for backward compatible
PMP_AUDENC_ENCODER MP_PCMEnc_getAudioEncodeObject(void)
{
	return &gPCMAudioEncObj[0];
}

