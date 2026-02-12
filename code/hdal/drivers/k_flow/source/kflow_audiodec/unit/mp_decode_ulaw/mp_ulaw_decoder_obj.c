
#ifdef __KERNEL__
#include "mp_ulaw_decoder.h"
#include "audio_codec_ulaw.h"
#include "kwrap/semaphore.h"
#else
#include "mp_ulaw_decoder.h"
#include "audio_codec_ulaw.h"
#include "kwrap/semaphore.h"
#endif

#define __MODULE__          ulaw_decobj
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int ulaw_decobj_debug_level = NVT_DBG_WRN;

#define ULAW_DECODEID   0x756c6177 // "ulaw"

extern ER MP_uLawDec1_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec1_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec1_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec1_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec1_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec2_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec2_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec2_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec2_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec2_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec3_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec3_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec3_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec3_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec3_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec4_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec4_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec4_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec4_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec4_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec5_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec5_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec5_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec5_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec5_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec6_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec6_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec6_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec6_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec6_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec7_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec7_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec7_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec7_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec7_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec8_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec8_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec8_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec8_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec8_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec9_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec9_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec9_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec9_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec9_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec10_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec10_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec10_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec10_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec10_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec11_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec11_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec11_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec11_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec11_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec12_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec12_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec12_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec12_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec12_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec13_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec13_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec13_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec13_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec13_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec14_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec14_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec14_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec14_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec14_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec15_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec15_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec15_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec15_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec15_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_uLawDec16_init(AUDIO_PLAYINFO *pobj);
extern ER MP_uLawDec16_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_uLawDec16_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_uLawDec16_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_uLawDec16_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

MP_AUDDEC_DECODER guLawAudioDecObj[MP_AUDDEC_ID_MAX] = {
	{
		MP_uLawDec1_init,        //Initialize
		MP_uLawDec1_getInfo,     //GetInfo
		MP_uLawDec1_setInfo,     //SetInfo
		MP_uLawDec1_decodeOne,   //DecodeOne
		MP_uLawDec1_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec2_init,        //Initialize
		MP_uLawDec2_getInfo,     //GetInfo
		MP_uLawDec2_setInfo,     //SetInfo
		MP_uLawDec2_decodeOne,   //DecodeOne
		MP_uLawDec2_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec3_init,        //Initialize
		MP_uLawDec3_getInfo,     //GetInfo
		MP_uLawDec3_setInfo,     //SetInfo
		MP_uLawDec3_decodeOne,   //DecodeOne
		MP_uLawDec3_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec4_init,        //Initialize
		MP_uLawDec4_getInfo,     //GetInfo
		MP_uLawDec4_setInfo,     //SetInfo
		MP_uLawDec4_decodeOne,   //DecodeOne
		MP_uLawDec4_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec5_init,        //Initialize
		MP_uLawDec5_getInfo,     //GetInfo
		MP_uLawDec5_setInfo,     //SetInfo
		MP_uLawDec5_decodeOne,   //DecodeOne
		MP_uLawDec5_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec6_init,        //Initialize
		MP_uLawDec6_getInfo,     //GetInfo
		MP_uLawDec6_setInfo,     //SetInfo
		MP_uLawDec6_decodeOne,   //DecodeOne
		MP_uLawDec6_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec7_init,        //Initialize
		MP_uLawDec7_getInfo,     //GetInfo
		MP_uLawDec7_setInfo,     //SetInfo
		MP_uLawDec7_decodeOne,   //DecodeOne
		MP_uLawDec7_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec8_init,        //Initialize
		MP_uLawDec8_getInfo,     //GetInfo
		MP_uLawDec8_setInfo,     //SetInfo
		MP_uLawDec8_decodeOne,   //DecodeOne
		MP_uLawDec8_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec9_init,        //Initialize
		MP_uLawDec9_getInfo,     //GetInfo
		MP_uLawDec9_setInfo,     //SetInfo
		MP_uLawDec9_decodeOne,   //DecodeOne
		MP_uLawDec9_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec10_init,        //Initialize
		MP_uLawDec10_getInfo,     //GetInfo
		MP_uLawDec10_setInfo,     //SetInfo
		MP_uLawDec10_decodeOne,   //DecodeOne
		MP_uLawDec10_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec11_init,        //Initialize
		MP_uLawDec11_getInfo,     //GetInfo
		MP_uLawDec11_setInfo,     //SetInfo
		MP_uLawDec11_decodeOne,   //DecodeOne
		MP_uLawDec11_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec12_init,        //Initialize
		MP_uLawDec12_getInfo,     //GetInfo
		MP_uLawDec12_setInfo,     //SetInfo
		MP_uLawDec12_decodeOne,   //DecodeOne
		MP_uLawDec12_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec13_init,        //Initialize
		MP_uLawDec13_getInfo,     //GetInfo
		MP_uLawDec13_setInfo,     //SetInfo
		MP_uLawDec13_decodeOne,   //DecodeOne
		MP_uLawDec13_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec14_init,        //Initialize
		MP_uLawDec14_getInfo,     //GetInfo
		MP_uLawDec14_setInfo,     //SetInfo
		MP_uLawDec14_decodeOne,   //DecodeOne
		MP_uLawDec14_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec15_init,        //Initialize
		MP_uLawDec15_getInfo,     //GetInfo
		MP_uLawDec15_setInfo,     //SetInfo
		MP_uLawDec15_decodeOne,   //DecodeOne
		MP_uLawDec15_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},

	{
		MP_uLawDec16_init,        //Initialize
		MP_uLawDec16_getInfo,     //GetInfo
		MP_uLawDec16_setInfo,     //SetInfo
		MP_uLawDec16_decodeOne,   //DecodeOne
		MP_uLawDec16_waitDecodeDone,
		NULL,                   //CustomizeFunc
		ULAW_DECODEID            //checkID
	},
};

ER MP_uLawDec1_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_1, pobj);
}

ER MP_uLawDec1_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_1, type, p1, p2, p3);
}

ER MP_uLawDec1_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_1, type, param1, param2, param3);
}

ER MP_uLawDec1_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_1, type, BsAddr, BsSize);
}

ER MP_uLawDec1_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_1, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec2_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_2, pobj);
}

ER MP_uLawDec2_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_2, type, p1, p2, p3);
}

ER MP_uLawDec2_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_2, type, param1, param2, param3);
}

ER MP_uLawDec2_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_2, type, BsAddr, BsSize);
}

ER MP_uLawDec2_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_2, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec3_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_3, pobj);
}

ER MP_uLawDec3_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_3, type, p1, p2, p3);
}

ER MP_uLawDec3_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_3, type, param1, param2, param3);
}

ER MP_uLawDec3_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_3, type, BsAddr, BsSize);
}

ER MP_uLawDec3_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_3, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec4_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_4, pobj);
}

ER MP_uLawDec4_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_4, type, p1, p2, p3);
}

ER MP_uLawDec4_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_4, type, param1, param2, param3);
}

ER MP_uLawDec4_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_4, type, BsAddr, BsSize);
}

ER MP_uLawDec4_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_4, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec5_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_5, pobj);
}

ER MP_uLawDec5_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_5, type, p1, p2, p3);
}

ER MP_uLawDec5_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_5, type, param1, param2, param3);
}

ER MP_uLawDec5_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_5, type, BsAddr, BsSize);
}

ER MP_uLawDec5_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_5, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec6_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_6, pobj);
}

ER MP_uLawDec6_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_6, type, p1, p2, p3);
}

ER MP_uLawDec6_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_6, type, param1, param2, param3);
}

ER MP_uLawDec6_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_6, type, BsAddr, BsSize);
}

ER MP_uLawDec6_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_6, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec7_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_7, pobj);
}

ER MP_uLawDec7_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_7, type, p1, p2, p3);
}

ER MP_uLawDec7_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_7, type, param1, param2, param3);
}

ER MP_uLawDec7_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_7, type, BsAddr, BsSize);
}

ER MP_uLawDec7_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_7, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec8_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_8, pobj);
}

ER MP_uLawDec8_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_8, type, p1, p2, p3);
}

ER MP_uLawDec8_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_8, type, param1, param2, param3);
}

ER MP_uLawDec8_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_8, type, BsAddr, BsSize);
}

ER MP_uLawDec8_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_8, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec9_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_9, pobj);
}

ER MP_uLawDec9_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_9, type, p1, p2, p3);
}

ER MP_uLawDec9_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_9, type, param1, param2, param3);
}

ER MP_uLawDec9_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_9, type, BsAddr, BsSize);
}

ER MP_uLawDec9_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_9, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec10_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_10, pobj);
}

ER MP_uLawDec10_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_10, type, p1, p2, p3);
}

ER MP_uLawDec10_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_10, type, param1, param2, param3);
}

ER MP_uLawDec10_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_10, type, BsAddr, BsSize);
}

ER MP_uLawDec10_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_10, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec11_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_11, pobj);
}

ER MP_uLawDec11_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_11, type, p1, p2, p3);
}

ER MP_uLawDec11_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_11, type, param1, param2, param3);
}

ER MP_uLawDec11_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_11, type, BsAddr, BsSize);
}

ER MP_uLawDec11_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_11, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec12_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_12, pobj);
}

ER MP_uLawDec12_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_12, type, p1, p2, p3);
}

ER MP_uLawDec12_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_12, type, param1, param2, param3);
}

ER MP_uLawDec12_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_12, type, BsAddr, BsSize);
}

ER MP_uLawDec12_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_12, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec13_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_13, pobj);
}

ER MP_uLawDec13_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_13, type, p1, p2, p3);
}

ER MP_uLawDec13_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_13, type, param1, param2, param3);
}

ER MP_uLawDec13_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_13, type, BsAddr, BsSize);
}

ER MP_uLawDec13_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_13, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec14_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_14, pobj);
}

ER MP_uLawDec14_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_14, type, p1, p2, p3);
}

ER MP_uLawDec14_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_14, type, param1, param2, param3);
}

ER MP_uLawDec14_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_14, type, BsAddr, BsSize);
}

ER MP_uLawDec14_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_14, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec15_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_15, pobj);
}

ER MP_uLawDec15_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_15, type, p1, p2, p3);
}

ER MP_uLawDec15_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_15, type, param1, param2, param3);
}

ER MP_uLawDec15_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_15, type, BsAddr, BsSize);
}

ER MP_uLawDec15_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_15, type, p1, p2, p3);
}

//----------------

ER MP_uLawDec16_init(AUDIO_PLAYINFO *pobj)
{
	return MP_uLawDec_init(MP_AUDDEC_ID_16, pobj);
}

ER MP_uLawDec16_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_getInfo(MP_AUDDEC_ID_16, type, p1, p2, p3);
}

ER MP_uLawDec16_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_uLawDec_setInfo(MP_AUDDEC_ID_16, type, param1, param2, param3);
}

ER MP_uLawDec16_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_uLawDec_decodeOne(MP_AUDDEC_ID_16, type, BsAddr, BsSize);
}

ER MP_uLawDec16_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_uLawDec_waitDecodeDone(MP_AUDDEC_ID_16, type, p1, p2, p3);
}

/**
    Get uLaw decoding object.

    Get uLaw decoding object.

    @return audio codec decoding object
*/
PMP_AUDDEC_DECODER MP_uLawDec_getAudioObject(MP_AUDDEC_ID AudDecId)
{
	return &guLawAudioDecObj[AudDecId];
}

// for backward compatible
PMP_AUDDEC_DECODER MP_uLawDec_getAudioDecodeObject(void)
{
	return &guLawAudioDecObj[0];
}

