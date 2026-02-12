
#ifdef __KERNEL__
#include "mp_alaw_decoder.h"
#include "audio_codec_alaw.h"
#include "kwrap/semaphore.h"
#else
#include "mp_alaw_decoder.h"
#include "audio_codec_alaw.h"
#include "kwrap/semaphore.h"
#endif

#define __MODULE__          alaw_decobj
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int alaw_decobj_debug_level = NVT_DBG_WRN;

#define ALAW_DECODEID   0x616c6177 // "alaw"

extern ER MP_aLawDec1_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec1_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec1_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec1_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec1_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec2_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec2_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec2_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec2_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec2_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec3_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec3_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec3_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec3_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec3_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec4_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec4_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec4_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec4_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec4_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec5_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec5_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec5_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec5_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec5_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec6_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec6_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec6_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec6_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec6_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec7_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec7_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec7_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec7_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec7_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec8_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec8_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec8_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec8_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec8_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec9_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec9_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec9_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec9_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec9_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec10_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec10_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec10_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec10_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec10_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec11_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec11_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec11_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec11_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec11_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec12_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec12_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec12_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec12_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec12_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec13_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec13_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec13_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec13_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec13_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec14_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec14_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec14_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec14_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec14_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec15_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec15_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec15_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec15_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec15_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_aLawDec16_init(AUDIO_PLAYINFO *pobj);
extern ER MP_aLawDec16_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_aLawDec16_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_aLawDec16_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_aLawDec16_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

MP_AUDDEC_DECODER gaLawAudioDecObj[MP_AUDDEC_ID_MAX] = {
	{
		MP_aLawDec1_init,        //Initialize
		MP_aLawDec1_getInfo,     //GetInfo
		MP_aLawDec1_setInfo,     //SetInfo
		MP_aLawDec1_decodeOne,   //DecodeOne
		MP_aLawDec1_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec2_init,        //Initialize
		MP_aLawDec2_getInfo,     //GetInfo
		MP_aLawDec2_setInfo,     //SetInfo
		MP_aLawDec2_decodeOne,   //DecodeOne
		MP_aLawDec2_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec3_init,        //Initialize
		MP_aLawDec3_getInfo,     //GetInfo
		MP_aLawDec3_setInfo,     //SetInfo
		MP_aLawDec3_decodeOne,   //DecodeOne
		MP_aLawDec3_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec4_init,        //Initialize
		MP_aLawDec4_getInfo,     //GetInfo
		MP_aLawDec4_setInfo,     //SetInfo
		MP_aLawDec4_decodeOne,   //DecodeOne
		MP_aLawDec4_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec5_init,        //Initialize
		MP_aLawDec5_getInfo,     //GetInfo
		MP_aLawDec5_setInfo,     //SetInfo
		MP_aLawDec5_decodeOne,   //DecodeOne
		MP_aLawDec5_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec6_init,        //Initialize
		MP_aLawDec6_getInfo,     //GetInfo
		MP_aLawDec6_setInfo,     //SetInfo
		MP_aLawDec6_decodeOne,   //DecodeOne
		MP_aLawDec6_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec7_init,        //Initialize
		MP_aLawDec7_getInfo,     //GetInfo
		MP_aLawDec7_setInfo,     //SetInfo
		MP_aLawDec7_decodeOne,   //DecodeOne
		MP_aLawDec7_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec8_init,        //Initialize
		MP_aLawDec8_getInfo,     //GetInfo
		MP_aLawDec8_setInfo,     //SetInfo
		MP_aLawDec8_decodeOne,   //DecodeOne
		MP_aLawDec8_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec9_init,        //Initialize
		MP_aLawDec9_getInfo,     //GetInfo
		MP_aLawDec9_setInfo,     //SetInfo
		MP_aLawDec9_decodeOne,   //DecodeOne
		MP_aLawDec9_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec10_init,        //Initialize
		MP_aLawDec10_getInfo,     //GetInfo
		MP_aLawDec10_setInfo,     //SetInfo
		MP_aLawDec10_decodeOne,   //DecodeOne
		MP_aLawDec10_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec11_init,        //Initialize
		MP_aLawDec11_getInfo,     //GetInfo
		MP_aLawDec11_setInfo,     //SetInfo
		MP_aLawDec11_decodeOne,   //DecodeOne
		MP_aLawDec11_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec12_init,        //Initialize
		MP_aLawDec12_getInfo,     //GetInfo
		MP_aLawDec12_setInfo,     //SetInfo
		MP_aLawDec12_decodeOne,   //DecodeOne
		MP_aLawDec12_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec13_init,        //Initialize
		MP_aLawDec13_getInfo,     //GetInfo
		MP_aLawDec13_setInfo,     //SetInfo
		MP_aLawDec13_decodeOne,   //DecodeOne
		MP_aLawDec13_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec14_init,        //Initialize
		MP_aLawDec14_getInfo,     //GetInfo
		MP_aLawDec14_setInfo,     //SetInfo
		MP_aLawDec14_decodeOne,   //DecodeOne
		MP_aLawDec14_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec15_init,        //Initialize
		MP_aLawDec15_getInfo,     //GetInfo
		MP_aLawDec15_setInfo,     //SetInfo
		MP_aLawDec15_decodeOne,   //DecodeOne
		MP_aLawDec15_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},

	{
		MP_aLawDec16_init,        //Initialize
		MP_aLawDec16_getInfo,     //GetInfo
		MP_aLawDec16_setInfo,     //SetInfo
		MP_aLawDec16_decodeOne,   //DecodeOne
		MP_aLawDec16_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ALAW_DECODEID            //checkID
	},
};

ER MP_aLawDec1_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_1, pobj);
}

ER MP_aLawDec1_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_1, type, p1, p2, p3);
}

ER MP_aLawDec1_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_1, type, param1, param2, param3);
}

ER MP_aLawDec1_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_1, type, BsAddr, BsSize);
}

ER MP_aLawDec1_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_1, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec2_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_2, pobj);
}

ER MP_aLawDec2_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_2, type, p1, p2, p3);
}

ER MP_aLawDec2_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_2, type, param1, param2, param3);
}

ER MP_aLawDec2_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_2, type, BsAddr, BsSize);
}

ER MP_aLawDec2_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_2, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec3_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_3, pobj);
}

ER MP_aLawDec3_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_3, type, p1, p2, p3);
}

ER MP_aLawDec3_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_3, type, param1, param2, param3);
}

ER MP_aLawDec3_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_3, type, BsAddr, BsSize);
}

ER MP_aLawDec3_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_3, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec4_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_4, pobj);
}

ER MP_aLawDec4_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_4, type, p1, p2, p3);
}

ER MP_aLawDec4_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_4, type, param1, param2, param3);
}

ER MP_aLawDec4_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_4, type, BsAddr, BsSize);
}

ER MP_aLawDec4_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_4, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec5_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_5, pobj);
}

ER MP_aLawDec5_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_5, type, p1, p2, p3);
}

ER MP_aLawDec5_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_5, type, param1, param2, param3);
}

ER MP_aLawDec5_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_5, type, BsAddr, BsSize);
}

ER MP_aLawDec5_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_5, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec6_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_6, pobj);
}

ER MP_aLawDec6_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_6, type, p1, p2, p3);
}

ER MP_aLawDec6_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_6, type, param1, param2, param3);
}

ER MP_aLawDec6_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_6, type, BsAddr, BsSize);
}

ER MP_aLawDec6_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_6, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec7_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_7, pobj);
}

ER MP_aLawDec7_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_7, type, p1, p2, p3);
}

ER MP_aLawDec7_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_7, type, param1, param2, param3);
}

ER MP_aLawDec7_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_7, type, BsAddr, BsSize);
}

ER MP_aLawDec7_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_7, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec8_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_8, pobj);
}

ER MP_aLawDec8_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_8, type, p1, p2, p3);
}

ER MP_aLawDec8_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_8, type, param1, param2, param3);
}

ER MP_aLawDec8_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_8, type, BsAddr, BsSize);
}

ER MP_aLawDec8_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_8, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec9_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_9, pobj);
}

ER MP_aLawDec9_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_9, type, p1, p2, p3);
}

ER MP_aLawDec9_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_9, type, param1, param2, param3);
}

ER MP_aLawDec9_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_9, type, BsAddr, BsSize);
}

ER MP_aLawDec9_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_9, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec10_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_10, pobj);
}

ER MP_aLawDec10_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_10, type, p1, p2, p3);
}

ER MP_aLawDec10_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_10, type, param1, param2, param3);
}

ER MP_aLawDec10_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_10, type, BsAddr, BsSize);
}

ER MP_aLawDec10_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_10, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec11_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_11, pobj);
}

ER MP_aLawDec11_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_11, type, p1, p2, p3);
}

ER MP_aLawDec11_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_11, type, param1, param2, param3);
}

ER MP_aLawDec11_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_11, type, BsAddr, BsSize);
}

ER MP_aLawDec11_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_11, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec12_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_12, pobj);
}

ER MP_aLawDec12_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_12, type, p1, p2, p3);
}

ER MP_aLawDec12_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_12, type, param1, param2, param3);
}

ER MP_aLawDec12_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_12, type, BsAddr, BsSize);
}

ER MP_aLawDec12_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_12, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec13_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_13, pobj);
}

ER MP_aLawDec13_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_13, type, p1, p2, p3);
}

ER MP_aLawDec13_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_13, type, param1, param2, param3);
}

ER MP_aLawDec13_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_13, type, BsAddr, BsSize);
}

ER MP_aLawDec13_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_13, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec14_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_14, pobj);
}

ER MP_aLawDec14_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_14, type, p1, p2, p3);
}

ER MP_aLawDec14_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_14, type, param1, param2, param3);
}

ER MP_aLawDec14_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_14, type, BsAddr, BsSize);
}

ER MP_aLawDec14_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_14, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec15_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_15, pobj);
}

ER MP_aLawDec15_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_15, type, p1, p2, p3);
}

ER MP_aLawDec15_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_15, type, param1, param2, param3);
}

ER MP_aLawDec15_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_15, type, BsAddr, BsSize);
}

ER MP_aLawDec15_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_15, type, p1, p2, p3);
}

//----------------

ER MP_aLawDec16_init(AUDIO_PLAYINFO *pobj)
{
	return MP_aLawDec_init(MP_AUDDEC_ID_16, pobj);
}

ER MP_aLawDec16_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_getInfo(MP_AUDDEC_ID_16, type, p1, p2, p3);
}

ER MP_aLawDec16_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_aLawDec_setInfo(MP_AUDDEC_ID_16, type, param1, param2, param3);
}

ER MP_aLawDec16_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_aLawDec_decodeOne(MP_AUDDEC_ID_16, type, BsAddr, BsSize);
}

ER MP_aLawDec16_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_aLawDec_waitDecodeDone(MP_AUDDEC_ID_16, type, p1, p2, p3);
}

/**
    Get aLaw decoding object.

    Get aLaw decoding object.

    @return audio codec decoding object
*/
PMP_AUDDEC_DECODER MP_aLawDec_getAudioObject(MP_AUDDEC_ID AudDecId)
{
	return &gaLawAudioDecObj[AudDecId];
}

// for backward compatible
PMP_AUDDEC_DECODER MP_aLawDec_getAudioDecodeObject(void)
{
	return &gaLawAudioDecObj[0];
}

