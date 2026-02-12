
#ifdef __KERNEL__
#include "mp_ppcm_encoder.h"
#include "audio_codec_ppcm.h"
#include "kwrap/semaphore.h"
#else
#include "mp_ppcm_encoder.h"
#include "audio_codec_ppcm.h"
#include "kwrap/semaphore.h"
#endif

#define __MODULE__          ppcm_encobj
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int ppcm_encobj_debug_level = NVT_DBG_WRN;

#define PPCM_ENCODEID 0x5050434D //PPCM

extern ER MP_PPCMEnc1_init(void);
extern ER MP_PPCMEnc1_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc1_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc1_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc2_init(void);
extern ER MP_PPCMEnc2_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc2_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc2_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc3_init(void);
extern ER MP_PPCMEnc3_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc3_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc3_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc4_init(void);
extern ER MP_PPCMEnc4_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc4_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc4_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc5_init(void);
extern ER MP_PPCMEnc5_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc5_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc5_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc6_init(void);
extern ER MP_PPCMEnc6_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc6_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc6_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc7_init(void);
extern ER MP_PPCMEnc7_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc7_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc7_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc8_init(void);
extern ER MP_PPCMEnc8_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc8_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc8_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc9_init(void);
extern ER MP_PPCMEnc9_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc9_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc9_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc10_init(void);
extern ER MP_PPCMEnc10_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc10_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc10_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc11_init(void);
extern ER MP_PPCMEnc11_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc11_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc11_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc12_init(void);
extern ER MP_PPCMEnc12_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc12_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc12_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc13_init(void);
extern ER MP_PPCMEnc13_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc13_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc13_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc14_init(void);
extern ER MP_PPCMEnc14_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc14_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc14_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc15_init(void);
extern ER MP_PPCMEnc15_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc15_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc15_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

extern ER MP_PPCMEnc16_init(void);
extern ER MP_PPCMEnc16_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PPCMEnc16_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PPCMEnc16_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *ptr);

MP_AUDENC_ENCODER gPPCMAudioEncObj[MP_AUDENC_ID_MAX] = {
	{
		MP_PPCMEnc1_init,        //Initialize
		MP_PPCMEnc1_getInfo,     //GetInfo
		MP_PPCMEnc1_setInfo,     //SetInfo
		MP_PPCMEnc1_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PPCM_ENCODEID            //checkID
	},

	{
		MP_PPCMEnc2_init,        //Initialize
		MP_PPCMEnc2_getInfo,     //GetInfo
		MP_PPCMEnc2_setInfo,     //SetInfo
		MP_PPCMEnc2_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PPCM_ENCODEID            //checkID
	},

	{
		MP_PPCMEnc3_init,        //Initialize
		MP_PPCMEnc3_getInfo,     //GetInfo
		MP_PPCMEnc3_setInfo,     //SetInfo
		MP_PPCMEnc3_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PPCM_ENCODEID            //checkID
	},

	{
		MP_PPCMEnc4_init,        //Initialize
		MP_PPCMEnc4_getInfo,     //GetInfo
		MP_PPCMEnc4_setInfo,     //SetInfo
		MP_PPCMEnc4_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PPCM_ENCODEID            //checkID
	},

	{
		MP_PPCMEnc5_init,        //Initialize
		MP_PPCMEnc5_getInfo,     //GetInfo
		MP_PPCMEnc5_setInfo,     //SetInfo
		MP_PPCMEnc5_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PPCM_ENCODEID            //checkID
	},

	{
		MP_PPCMEnc6_init,        //Initialize
		MP_PPCMEnc6_getInfo,     //GetInfo
		MP_PPCMEnc6_setInfo,     //SetInfo
		MP_PPCMEnc6_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PPCM_ENCODEID            //checkID
	},

	{
		MP_PPCMEnc7_init,        //Initialize
		MP_PPCMEnc7_getInfo,     //GetInfo
		MP_PPCMEnc7_setInfo,     //SetInfo
		MP_PPCMEnc7_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PPCM_ENCODEID            //checkID
	},

	{
		MP_PPCMEnc8_init,        //Initialize
		MP_PPCMEnc8_getInfo,     //GetInfo
		MP_PPCMEnc8_setInfo,     //SetInfo
		MP_PPCMEnc8_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PPCM_ENCODEID            //checkID
	},

	{
		MP_PPCMEnc9_init,        //Initialize
		MP_PPCMEnc9_getInfo,     //GetInfo
		MP_PPCMEnc9_setInfo,     //SetInfo
		MP_PPCMEnc9_encodeOne,   //EncodeOne
		NULL,                   //CustomizeFunc
		PPCM_ENCODEID            //checkID
	},

	{
		MP_PPCMEnc10_init,        //Initialize
		MP_PPCMEnc10_getInfo,     //GetInfo
		MP_PPCMEnc10_setInfo,     //SetInfo
		MP_PPCMEnc10_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		PPCM_ENCODEID             //checkID
	},

	{
		MP_PPCMEnc11_init,        //Initialize
		MP_PPCMEnc11_getInfo,     //GetInfo
		MP_PPCMEnc11_setInfo,     //SetInfo
		MP_PPCMEnc11_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		PPCM_ENCODEID             //checkID
	},

	{
		MP_PPCMEnc12_init,        //Initialize
		MP_PPCMEnc12_getInfo,     //GetInfo
		MP_PPCMEnc12_setInfo,     //SetInfo
		MP_PPCMEnc12_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		PPCM_ENCODEID             //checkID
	},

	{
		MP_PPCMEnc13_init,        //Initialize
		MP_PPCMEnc13_getInfo,     //GetInfo
		MP_PPCMEnc13_setInfo,     //SetInfo
		MP_PPCMEnc13_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		PPCM_ENCODEID             //checkID
	},

	{
		MP_PPCMEnc14_init,        //Initialize
		MP_PPCMEnc14_getInfo,     //GetInfo
		MP_PPCMEnc14_setInfo,     //SetInfo
		MP_PPCMEnc14_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		PPCM_ENCODEID             //checkID
	},

	{
		MP_PPCMEnc15_init,        //Initialize
		MP_PPCMEnc15_getInfo,     //GetInfo
		MP_PPCMEnc15_setInfo,     //SetInfo
		MP_PPCMEnc15_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		PPCM_ENCODEID             //checkID
	},

	{
		MP_PPCMEnc16_init,        //Initialize
		MP_PPCMEnc16_getInfo,     //GetInfo
		MP_PPCMEnc16_setInfo,     //SetInfo
		MP_PPCMEnc16_encodeOne,   //EncodeOne
		NULL,                    //CustomizeFunc
		PPCM_ENCODEID             //checkID
	},
};

ER MP_PPCMEnc1_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_1);
}

ER MP_PPCMEnc1_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_1, type, p1, p2, p3);
}

ER MP_PPCMEnc1_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_1, type, param1, param2, param3);
}

ER MP_PPCMEnc1_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_1, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc2_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_2);
}

ER MP_PPCMEnc2_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_2, type, p1, p2, p3);
}

ER MP_PPCMEnc2_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_2, type, param1, param2, param3);
}

ER MP_PPCMEnc2_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_2, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc3_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_3);
}

ER MP_PPCMEnc3_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_3, type, p1, p2, p3);
}

ER MP_PPCMEnc3_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_3, type, param1, param2, param3);
}

ER MP_PPCMEnc3_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_3, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc4_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_4);
}

ER MP_PPCMEnc4_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_4, type, p1, p2, p3);
}

ER MP_PPCMEnc4_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_4, type, param1, param2, param3);
}

ER MP_PPCMEnc4_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_4, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc5_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_5);
}

ER MP_PPCMEnc5_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_5, type, p1, p2, p3);
}

ER MP_PPCMEnc5_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_5, type, param1, param2, param3);
}

ER MP_PPCMEnc5_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_5, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc6_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_6);
}

ER MP_PPCMEnc6_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_6, type, p1, p2, p3);
}

ER MP_PPCMEnc6_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_6, type, param1, param2, param3);
}

ER MP_PPCMEnc6_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_6, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc7_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_7);
}

ER MP_PPCMEnc7_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_7, type, p1, p2, p3);
}

ER MP_PPCMEnc7_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_7, type, param1, param2, param3);
}

ER MP_PPCMEnc7_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_7, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc8_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_8);
}

ER MP_PPCMEnc8_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_8, type, p1, p2, p3);
}

ER MP_PPCMEnc8_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_8, type, param1, param2, param3);
}

ER MP_PPCMEnc8_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_8, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc9_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_9);
}

ER MP_PPCMEnc9_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_9, type, p1, p2, p3);
}

ER MP_PPCMEnc9_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_9, type, param1, param2, param3);
}

ER MP_PPCMEnc9_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_9, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc10_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_10);
}

ER MP_PPCMEnc10_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_10, type, p1, p2, p3);
}

ER MP_PPCMEnc10_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_10, type, param1, param2, param3);
}

ER MP_PPCMEnc10_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_10, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc11_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_11);
}

ER MP_PPCMEnc11_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_11, type, p1, p2, p3);
}

ER MP_PPCMEnc11_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_11, type, param1, param2, param3);
}

ER MP_PPCMEnc11_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_11, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc12_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_12);
}

ER MP_PPCMEnc12_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_12, type, p1, p2, p3);
}

ER MP_PPCMEnc12_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_12, type, param1, param2, param3);
}

ER MP_PPCMEnc12_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_12, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc13_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_13);
}

ER MP_PPCMEnc13_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_13, type, p1, p2, p3);
}

ER MP_PPCMEnc13_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_13, type, param1, param2, param3);
}

ER MP_PPCMEnc13_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_13, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc14_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_14);
}

ER MP_PPCMEnc14_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_14, type, p1, p2, p3);
}

ER MP_PPCMEnc14_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_14, type, param1, param2, param3);
}

ER MP_PPCMEnc14_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_14, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc15_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_15);
}

ER MP_PPCMEnc15_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_15, type, p1, p2, p3);
}

ER MP_PPCMEnc15_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_15, type, param1, param2, param3);
}

ER MP_PPCMEnc15_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_15, type, outputAddr, pSize, pAudEncParam);
}

ER MP_PPCMEnc16_init(void)
{
	return MP_PPCMEnc_init(MP_AUDENC_ID_16);
}

ER MP_PPCMEnc16_getInfo(MP_AUDENC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PPCMEnc_getInfo(MP_AUDENC_ID_16, type, p1, p2, p3);
}

ER MP_PPCMEnc16_setInfo(MP_AUDENC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PPCMEnc_setInfo(MP_AUDENC_ID_16, type, param1, param2, param3);
}

ER MP_PPCMEnc16_encodeOne(UINT32 type, UINT32 outputAddr, UINT32 *pSize, MP_AUDENC_PARAM *pAudEncParam)
{
	return MP_PPCMEnc_encodeOne(MP_AUDENC_ID_16, type, outputAddr, pSize, pAudEncParam);
}

/**
    Get PPCM encoding object.

    Get PPCM encoding object.

    @return audio codec encoding object
*/
PMP_AUDENC_ENCODER MP_PPCMEnc_getAudioObject(MP_AUDENC_ID AudEncId)
{
	return &gPPCMAudioEncObj[AudEncId];
}

// for backward compatible
PMP_AUDENC_ENCODER MP_PPCMEnc_getAudioEncodeObject(void)
{
	return &gPPCMAudioEncObj[0];
}

