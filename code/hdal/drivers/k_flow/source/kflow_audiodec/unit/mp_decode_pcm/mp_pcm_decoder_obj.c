
#ifdef __KERNEL__
#include "mp_pcm_decoder.h"
#include "audio_codec_pcm.h"
#include "kwrap/semaphore.h"
#else
#include "mp_pcm_decoder.h"
#include "audio_codec_pcm.h"
#include "kwrap/semaphore.h"
#endif

#define __MODULE__          pcm_decobj
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
unsigned int pcm_decobj_debug_level = NVT_DBG_WRN;

#define PCM_DECODEID  0x74776f73//twos

extern ER MP_PCMDec1_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec1_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec1_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec1_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec1_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec2_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec2_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec2_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec2_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec2_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec3_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec3_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec3_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec3_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec3_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec4_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec4_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec4_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec4_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec4_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec5_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec5_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec5_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec5_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec5_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec6_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec6_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec6_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec6_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec6_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec7_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec7_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec7_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec7_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec7_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec8_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec8_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec8_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec8_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec8_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec9_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec9_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec9_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec9_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec9_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec10_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec10_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec10_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec10_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec10_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec11_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec11_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec11_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec11_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec11_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec12_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec12_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec12_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec12_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec12_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec13_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec13_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec13_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec13_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec13_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec14_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec14_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec14_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec14_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec14_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec15_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec15_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec15_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec15_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec15_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

extern ER MP_PCMDec16_init(AUDIO_PLAYINFO *pobj);
extern ER MP_PCMDec16_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER MP_PCMDec16_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3);
extern ER MP_PCMDec16_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize);
extern ER MP_PCMDec16_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);

MP_AUDDEC_DECODER gPCMAudioDecObj[MP_AUDDEC_ID_MAX] = {
	{
		MP_PCMDec1_init,        //Initialize
		MP_PCMDec1_getInfo,     //GetInfo
		MP_PCMDec1_setInfo,     //SetInfo
		MP_PCMDec1_decodeOne,   //DecodeOne
		MP_PCMDec1_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec2_init,        //Initialize
		MP_PCMDec2_getInfo,     //GetInfo
		MP_PCMDec2_setInfo,     //SetInfo
		MP_PCMDec2_decodeOne,   //DecodeOne
		MP_PCMDec2_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec3_init,        //Initialize
		MP_PCMDec3_getInfo,     //GetInfo
		MP_PCMDec3_setInfo,     //SetInfo
		MP_PCMDec3_decodeOne,   //DecodeOne
		MP_PCMDec3_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec4_init,        //Initialize
		MP_PCMDec4_getInfo,     //GetInfo
		MP_PCMDec4_setInfo,     //SetInfo
		MP_PCMDec4_decodeOne,   //DecodeOne
		MP_PCMDec4_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec5_init,        //Initialize
		MP_PCMDec5_getInfo,     //GetInfo
		MP_PCMDec5_setInfo,     //SetInfo
		MP_PCMDec5_decodeOne,   //DecodeOne
		MP_PCMDec5_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec6_init,        //Initialize
		MP_PCMDec6_getInfo,     //GetInfo
		MP_PCMDec6_setInfo,     //SetInfo
		MP_PCMDec6_decodeOne,   //DecodeOne
		MP_PCMDec6_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec7_init,        //Initialize
		MP_PCMDec7_getInfo,     //GetInfo
		MP_PCMDec7_setInfo,     //SetInfo
		MP_PCMDec7_decodeOne,   //DecodeOne
		MP_PCMDec7_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec8_init,        //Initialize
		MP_PCMDec8_getInfo,     //GetInfo
		MP_PCMDec8_setInfo,     //SetInfo
		MP_PCMDec8_decodeOne,   //DecodeOne
		MP_PCMDec8_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec9_init,        //Initialize
		MP_PCMDec9_getInfo,     //GetInfo
		MP_PCMDec9_setInfo,     //SetInfo
		MP_PCMDec9_decodeOne,   //DecodeOne
		MP_PCMDec9_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec10_init,        //Initialize
		MP_PCMDec10_getInfo,     //GetInfo
		MP_PCMDec10_setInfo,     //SetInfo
		MP_PCMDec10_decodeOne,   //DecodeOne
		MP_PCMDec10_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec11_init,        //Initialize
		MP_PCMDec11_getInfo,     //GetInfo
		MP_PCMDec11_setInfo,     //SetInfo
		MP_PCMDec11_decodeOne,   //DecodeOne
		MP_PCMDec11_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec12_init,        //Initialize
		MP_PCMDec12_getInfo,     //GetInfo
		MP_PCMDec12_setInfo,     //SetInfo
		MP_PCMDec12_decodeOne,   //DecodeOne
		MP_PCMDec12_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec13_init,        //Initialize
		MP_PCMDec13_getInfo,     //GetInfo
		MP_PCMDec13_setInfo,     //SetInfo
		MP_PCMDec13_decodeOne,   //DecodeOne
		MP_PCMDec13_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec14_init,        //Initialize
		MP_PCMDec14_getInfo,     //GetInfo
		MP_PCMDec14_setInfo,     //SetInfo
		MP_PCMDec14_decodeOne,   //DecodeOne
		MP_PCMDec14_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec15_init,        //Initialize
		MP_PCMDec15_getInfo,     //GetInfo
		MP_PCMDec15_setInfo,     //SetInfo
		MP_PCMDec15_decodeOne,   //DecodeOne
		MP_PCMDec15_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},

	{
		MP_PCMDec16_init,        //Initialize
		MP_PCMDec16_getInfo,     //GetInfo
		MP_PCMDec16_setInfo,     //SetInfo
		MP_PCMDec16_decodeOne,   //DecodeOne
		MP_PCMDec16_waitDecodeDone,
		NULL,                   //CustomizeFunc
		PCM_DECODEID            //checkID
	},
};

ER MP_PCMDec1_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_1, pobj);
}

ER MP_PCMDec1_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_1, type, p1, p2, p3);
}

ER MP_PCMDec1_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_1, type, param1, param2, param3);
}

ER MP_PCMDec1_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_1, type, BsAddr, BsSize);
}

ER MP_PCMDec1_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_1, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec2_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_2, pobj);
}

ER MP_PCMDec2_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_2, type, p1, p2, p3);
}

ER MP_PCMDec2_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_2, type, param1, param2, param3);
}

ER MP_PCMDec2_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_2, type, BsAddr, BsSize);
}

ER MP_PCMDec2_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_2, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec3_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_3, pobj);
}

ER MP_PCMDec3_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_3, type, p1, p2, p3);
}

ER MP_PCMDec3_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_3, type, param1, param2, param3);
}

ER MP_PCMDec3_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_3, type, BsAddr, BsSize);
}

ER MP_PCMDec3_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_3, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec4_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_4, pobj);
}

ER MP_PCMDec4_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_4, type, p1, p2, p3);
}

ER MP_PCMDec4_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_4, type, param1, param2, param3);
}

ER MP_PCMDec4_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_4, type, BsAddr, BsSize);
}

ER MP_PCMDec4_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_4, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec5_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_5, pobj);
}

ER MP_PCMDec5_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_5, type, p1, p2, p3);
}

ER MP_PCMDec5_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_5, type, param1, param2, param3);
}

ER MP_PCMDec5_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_5, type, BsAddr, BsSize);
}

ER MP_PCMDec5_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_5, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec6_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_6, pobj);
}

ER MP_PCMDec6_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_6, type, p1, p2, p3);
}

ER MP_PCMDec6_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_6, type, param1, param2, param3);
}

ER MP_PCMDec6_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_6, type, BsAddr, BsSize);
}

ER MP_PCMDec6_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_6, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec7_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_7, pobj);
}

ER MP_PCMDec7_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_7, type, p1, p2, p3);
}

ER MP_PCMDec7_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_7, type, param1, param2, param3);
}

ER MP_PCMDec7_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_7, type, BsAddr, BsSize);
}

ER MP_PCMDec7_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_7, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec8_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_8, pobj);
}

ER MP_PCMDec8_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_8, type, p1, p2, p3);
}

ER MP_PCMDec8_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_8, type, param1, param2, param3);
}

ER MP_PCMDec8_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_8, type, BsAddr, BsSize);
}

ER MP_PCMDec8_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_8, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec9_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_9, pobj);
}

ER MP_PCMDec9_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_9, type, p1, p2, p3);
}

ER MP_PCMDec9_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_9, type, param1, param2, param3);
}

ER MP_PCMDec9_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_9, type, BsAddr, BsSize);
}

ER MP_PCMDec9_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_9, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec10_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_10, pobj);
}

ER MP_PCMDec10_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_10, type, p1, p2, p3);
}

ER MP_PCMDec10_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_10, type, param1, param2, param3);
}

ER MP_PCMDec10_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_10, type, BsAddr, BsSize);
}

ER MP_PCMDec10_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_10, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec11_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_11, pobj);
}

ER MP_PCMDec11_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_11, type, p1, p2, p3);
}

ER MP_PCMDec11_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_11, type, param1, param2, param3);
}

ER MP_PCMDec11_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_11, type, BsAddr, BsSize);
}

ER MP_PCMDec11_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_11, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec12_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_12, pobj);
}

ER MP_PCMDec12_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_12, type, p1, p2, p3);
}

ER MP_PCMDec12_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_12, type, param1, param2, param3);
}

ER MP_PCMDec12_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_12, type, BsAddr, BsSize);
}

ER MP_PCMDec12_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_12, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec13_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_13, pobj);
}

ER MP_PCMDec13_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_13, type, p1, p2, p3);
}

ER MP_PCMDec13_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_13, type, param1, param2, param3);
}

ER MP_PCMDec13_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_13, type, BsAddr, BsSize);
}

ER MP_PCMDec13_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_13, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec14_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_14, pobj);
}

ER MP_PCMDec14_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_14, type, p1, p2, p3);
}

ER MP_PCMDec14_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_14, type, param1, param2, param3);
}

ER MP_PCMDec14_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_14, type, BsAddr, BsSize);
}

ER MP_PCMDec14_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_14, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec15_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_15, pobj);
}

ER MP_PCMDec15_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_15, type, p1, p2, p3);
}

ER MP_PCMDec15_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_15, type, param1, param2, param3);
}

ER MP_PCMDec15_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_15, type, BsAddr, BsSize);
}

ER MP_PCMDec15_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_15, type, p1, p2, p3);
}

//----------------

ER MP_PCMDec16_init(AUDIO_PLAYINFO *pobj)
{
	return MP_PCMDec_init(MP_AUDDEC_ID_16, pobj);
}

ER MP_PCMDec16_getInfo(MP_AUDDEC_GETINFO_TYPE type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_getInfo(MP_AUDDEC_ID_16, type, p1, p2, p3);
}

ER MP_PCMDec16_setInfo(MP_AUDDEC_SETINFO_TYPE type, UINT32 param1, UINT32 param2, UINT32 param3)
{
	return MP_PCMDec_setInfo(MP_AUDDEC_ID_16, type, param1, param2, param3);
}

ER MP_PCMDec16_decodeOne(UINT32 type, UINT32 BsAddr, UINT32 BsSize)
{
	return MP_PCMDec_decodeOne(MP_AUDDEC_ID_16, type, BsAddr, BsSize);
}

ER MP_PCMDec16_waitDecodeDone(UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3)
{
	return MP_PCMDec_waitDecodeDone(MP_AUDDEC_ID_16, type, p1, p2, p3);
}

/**
    Get PCM decoding object.

    Get PCM decoding object.

    @return audio codec decoding object
*/
PMP_AUDDEC_DECODER MP_PCMDec_getAudioObject(MP_AUDDEC_ID AudDecId)
{
	return &gPCMAudioDecObj[AudDecId];
}

// for backward compatible
PMP_AUDDEC_DECODER MP_PCMDec_getAudioDecodeObject(void)
{
	return &gPCMAudioDecObj[0];
}

