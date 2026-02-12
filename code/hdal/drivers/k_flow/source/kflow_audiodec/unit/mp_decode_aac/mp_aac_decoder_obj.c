
#ifdef __KERNEL__
#include "mp_aac_decoder.h"
#include "audio_codec_aac.h"
#include "kwrap/semaphore.h"
#else
#include "mp_aac_decoder.h"
#include "audio_codec_aac.h"
#include "kwrap/semaphore.h"
#endif

#define __MODULE__          aac_decobj
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int aac_decobj_debug_level = NVT_DBG_WRN;

#define AAC_DECODEID 0x74776f73

extern ER MP_AACDec1_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec1_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec1_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec1_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec1_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec2_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec2_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec2_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec2_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec2_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec3_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec3_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec3_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec3_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec3_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec4_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec4_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec4_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec4_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec4_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec5_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec5_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec5_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec5_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec5_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec6_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec6_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec6_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec6_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec6_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec7_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec7_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec7_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec7_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec7_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec8_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec8_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec8_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec8_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec8_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec9_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec9_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec9_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec9_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec9_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec10_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec10_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec10_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec10_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec10_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec11_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec11_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec11_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec11_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec11_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec12_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec12_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec12_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec12_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec12_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec13_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec13_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec13_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec13_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec13_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec14_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec14_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec14_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec14_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec14_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec15_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec15_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec15_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec15_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec15_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_AACDec16_init(AUDIO_PLAYINFO *pobj);
extern ER MP_AACDec16_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_AACDec16_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_AACDec16_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_AACDec16_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

MP_AUDDEC_DECODER gAACAudioDecObj[MP_AUDDEC_ID_MAX] = {
	{
		MP_AACDec1_init,        //Initialize
		MP_AACDec1_getInfo,     //GetInfo
		MP_AACDec1_setInfo,     //SetInfo
		MP_AACDec1_decodeOne,   //DecodeOne
		MP_AACDec1_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec2_init,        //Initialize
		MP_AACDec2_getInfo,     //GetInfo
		MP_AACDec2_setInfo,     //SetInfo
		MP_AACDec2_decodeOne,   //DecodeOne
		MP_AACDec2_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec3_init,        //Initialize
		MP_AACDec3_getInfo,     //GetInfo
		MP_AACDec3_setInfo,     //SetInfo
		MP_AACDec3_decodeOne,   //DecodeOne
		MP_AACDec3_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec4_init,        //Initialize
		MP_AACDec4_getInfo,     //GetInfo
		MP_AACDec4_setInfo,     //SetInfo
		MP_AACDec4_decodeOne,   //DecodeOne
		MP_AACDec4_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec5_init,        //Initialize
		MP_AACDec5_getInfo,     //GetInfo
		MP_AACDec5_setInfo,     //SetInfo
		MP_AACDec5_decodeOne,   //DecodeOne
		MP_AACDec5_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec6_init,        //Initialize
		MP_AACDec6_getInfo,     //GetInfo
		MP_AACDec6_setInfo,     //SetInfo
		MP_AACDec6_decodeOne,   //DecodeOne
		MP_AACDec6_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec7_init,        //Initialize
		MP_AACDec7_getInfo,     //GetInfo
		MP_AACDec7_setInfo,     //SetInfo
		MP_AACDec7_decodeOne,   //DecodeOne
		MP_AACDec7_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec8_init,        //Initialize
		MP_AACDec8_getInfo,     //GetInfo
		MP_AACDec8_setInfo,     //SetInfo
		MP_AACDec8_decodeOne,   //DecodeOne
		MP_AACDec8_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec9_init,        //Initialize
		MP_AACDec9_getInfo,     //GetInfo
		MP_AACDec9_setInfo,     //SetInfo
		MP_AACDec9_decodeOne,   //DecodeOne
		MP_AACDec9_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec10_init,        //Initialize
		MP_AACDec10_getInfo,     //GetInfo
		MP_AACDec10_setInfo,     //SetInfo
		MP_AACDec10_decodeOne,   //DecodeOne
		MP_AACDec10_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec11_init,        //Initialize
		MP_AACDec11_getInfo,     //GetInfo
		MP_AACDec11_setInfo,     //SetInfo
		MP_AACDec11_decodeOne,   //DecodeOne
		MP_AACDec11_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec12_init,        //Initialize
		MP_AACDec12_getInfo,     //GetInfo
		MP_AACDec12_setInfo,     //SetInfo
		MP_AACDec12_decodeOne,   //DecodeOne
		MP_AACDec12_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec13_init,        //Initialize
		MP_AACDec13_getInfo,     //GetInfo
		MP_AACDec13_setInfo,     //SetInfo
		MP_AACDec13_decodeOne,   //DecodeOne
		MP_AACDec13_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec14_init,        //Initialize
		MP_AACDec14_getInfo,     //GetInfo
		MP_AACDec14_setInfo,     //SetInfo
		MP_AACDec14_decodeOne,   //DecodeOne
		MP_AACDec14_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec15_init,        //Initialize
		MP_AACDec15_getInfo,     //GetInfo
		MP_AACDec15_setInfo,     //SetInfo
		MP_AACDec15_decodeOne,   //DecodeOne
		MP_AACDec15_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},

	{
		MP_AACDec16_init,        //Initialize
		MP_AACDec16_getInfo,     //GetInfo
		MP_AACDec16_setInfo,     //SetInfo
		MP_AACDec16_decodeOne,   //DecodeOne
		MP_AACDec16_waitDecodeDone,
		NULL,                   //CustomizeFunc
		AAC_DECODEID            //checkID
	},
};

ER MP_AACDec1_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_1, pobj);
}

ER MP_AACDec1_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_1, type, p1, p2, p3);
}

ER MP_AACDec1_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_1, type, param1, param2, param3);
}

ER MP_AACDec1_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_1, type, BsAddr, BsSize);
}

ER MP_AACDec1_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_1, type, p1, p2, p3);
}

//----------------

ER MP_AACDec2_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_2, pobj);
}

ER MP_AACDec2_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_2, type, p1, p2, p3);
}

ER MP_AACDec2_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_2, type, param1, param2, param3);
}

ER MP_AACDec2_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_2, type, BsAddr, BsSize);
}

ER MP_AACDec2_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_2, type, p1, p2, p3);
}

//----------------

ER MP_AACDec3_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_3, pobj);
}

ER MP_AACDec3_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_3, type, p1, p2, p3);
}

ER MP_AACDec3_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_3, type, param1, param2, param3);
}

ER MP_AACDec3_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_3, type, BsAddr, BsSize);
}

ER MP_AACDec3_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_3, type, p1, p2, p3);
}

//----------------

ER MP_AACDec4_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_4, pobj);
}

ER MP_AACDec4_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_4, type, p1, p2, p3);
}

ER MP_AACDec4_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_4, type, param1, param2, param3);
}

ER MP_AACDec4_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_4, type, BsAddr, BsSize);
}

ER MP_AACDec4_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_4, type, p1, p2, p3);
}

//----------------

ER MP_AACDec5_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_5, pobj);
}

ER MP_AACDec5_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_5, type, p1, p2, p3);
}

ER MP_AACDec5_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_5, type, param1, param2, param3);
}

ER MP_AACDec5_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_5, type, BsAddr, BsSize);
}

ER MP_AACDec5_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_5, type, p1, p2, p3);
}

//----------------

ER MP_AACDec6_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_6, pobj);
}

ER MP_AACDec6_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_6, type, p1, p2, p3);
}

ER MP_AACDec6_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_6, type, param1, param2, param3);
}

ER MP_AACDec6_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_6, type, BsAddr, BsSize);
}

ER MP_AACDec6_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_6, type, p1, p2, p3);
}

//----------------

ER MP_AACDec7_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_7, pobj);
}

ER MP_AACDec7_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_7, type, p1, p2, p3);
}

ER MP_AACDec7_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_7, type, param1, param2, param3);
}

ER MP_AACDec7_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_7, type, BsAddr, BsSize);
}

ER MP_AACDec7_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_7, type, p1, p2, p3);
}

//----------------

ER MP_AACDec8_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_8, pobj);
}

ER MP_AACDec8_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_8, type, p1, p2, p3);
}

ER MP_AACDec8_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_8, type, param1, param2, param3);
}

ER MP_AACDec8_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_8, type, BsAddr, BsSize);
}

ER MP_AACDec8_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_8, type, p1, p2, p3);
}

//----------------

ER MP_AACDec9_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_9, pobj);
}

ER MP_AACDec9_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_9, type, p1, p2, p3);
}

ER MP_AACDec9_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_9, type, param1, param2, param3);
}

ER MP_AACDec9_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_9, type, BsAddr, BsSize);
}

ER MP_AACDec9_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_9, type, p1, p2, p3);
}

//----------------

ER MP_AACDec10_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_10, pobj);
}

ER MP_AACDec10_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_10, type, p1, p2, p3);
}

ER MP_AACDec10_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_10, type, param1, param2, param3);
}

ER MP_AACDec10_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_10, type, BsAddr, BsSize);
}

ER MP_AACDec10_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_10, type, p1, p2, p3);
}

//----------------

ER MP_AACDec11_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_11, pobj);
}

ER MP_AACDec11_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_11, type, p1, p2, p3);
}

ER MP_AACDec11_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_11, type, param1, param2, param3);
}

ER MP_AACDec11_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_11, type, BsAddr, BsSize);
}

ER MP_AACDec11_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_11, type, p1, p2, p3);
}

//----------------

ER MP_AACDec12_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_12, pobj);
}

ER MP_AACDec12_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_12, type, p1, p2, p3);
}

ER MP_AACDec12_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_12, type, param1, param2, param3);
}

ER MP_AACDec12_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_12, type, BsAddr, BsSize);
}

ER MP_AACDec12_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_12, type, p1, p2, p3);
}

//----------------

ER MP_AACDec13_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_13, pobj);
}

ER MP_AACDec13_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_13, type, p1, p2, p3);
}

ER MP_AACDec13_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_13, type, param1, param2, param3);
}

ER MP_AACDec13_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_13, type, BsAddr, BsSize);
}

ER MP_AACDec13_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_13, type, p1, p2, p3);
}

//----------------

ER MP_AACDec14_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_14, pobj);
}

ER MP_AACDec14_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_14, type, p1, p2, p3);
}

ER MP_AACDec14_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_14, type, param1, param2, param3);
}

ER MP_AACDec14_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_14, type, BsAddr, BsSize);
}

ER MP_AACDec14_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_14, type, p1, p2, p3);
}

//----------------

ER MP_AACDec15_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_15, pobj);
}

ER MP_AACDec15_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_15, type, p1, p2, p3);
}

ER MP_AACDec15_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_15, type, param1, param2, param3);
}

ER MP_AACDec15_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_15, type, BsAddr, BsSize);
}

ER MP_AACDec15_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_15, type, p1, p2, p3);
}

//----------------

ER MP_AACDec16_init(AUDIO_PLAYINFO *pobj)
{
	return MP_AACDec_init(MP_AUDDEC_ID_16, pobj);
}

ER MP_AACDec16_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_getInfo(MP_AUDDEC_ID_16, type, p1, p2, p3);
}

ER MP_AACDec16_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_AACDec_setInfo(MP_AUDDEC_ID_16, type, param1, param2, param3);
}

ER MP_AACDec16_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_AACDec_decodeOne(MP_AUDDEC_ID_16, type, BsAddr, BsSize);
}

ER MP_AACDec16_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_AACDec_waitDecodeDone(MP_AUDDEC_ID_16, type, p1, p2, p3);
}

/**
    Get AAC decoding object.

    Get AAC decoding object.

    @return audio codec decoding object
*/
PMP_AUDDEC_DECODER MP_AACDec_getAudioObject(MP_AUDDEC_ID AudDecId)
{
	return &gAACAudioDecObj[AudDecId];
}

// for backward compatible
PMP_AUDDEC_DECODER MP_AACDec_getAudioDecodeObject(void)
{
	return &gAACAudioDecObj[0];
}

