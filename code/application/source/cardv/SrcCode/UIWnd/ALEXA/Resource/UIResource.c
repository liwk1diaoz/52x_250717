#include "PrjInc.h"
#include "PrjCfg.h"
#include "UIResource.h"
//#NT#2010/10/15#Niven Cho -begin
//#NT#CMP_LANG
static STRING_TABLE g_LangCurrTbl = 0;
//#NT#2010/10/15#Niven Cho -end
//MOVIE_SIZE_TAG
char *gUserStr[STRID_USER_END - STRID_USER_START] = {
	"MCTF",
	"Edge",
	"NR",
	"WiFi/ETH",
	"6400",
	"12800",
	//single
	"UHD P50",
	"UHD P30",
	"UHD P24",
	"2.7K P60",
	"QHD P80",
	"QHD P60",
	"QHD P30",
	"3MHD P30",
	"FHD P120",
	"FHD P96",
	"FHD P60",
	"FHD P30",
	"HD P240",
	"HD P120",
	"HD P60",
	"HD P30",
	"WVGA P30",
	"VGA P240",
	"VGA P30",
	"QVGA P30",
	//dual
	"QHD P30+HD P30",
	"3MHD P30+HD P30",
	"FHD P30+FHD P30",
	"FHD P30+HD P30",
	"FHD P30+WVGA P30",
	//clone
	"FHD P30+FHD P30",
	"FHD P30+HD P30",
	"QHD P30+WVGA P30",
	"3MHD P30+WVGA P30",
	"FHD P60+WVGA P30",
	"FHD P60+VGA P30", //640x360
	"FHD P30+WVGA P30",
	"2048x2048 P30 + 480x480 P30",
	"Both2",
	"SideBySide",
	"Burst 30",
	"5M 2992x1696",
	"Codec",
	"MJPG",
	"H264",
	"H265"
};
char *UIRes_GetUserString(UINT32 TxtId)
{
	return gUserStr[TxtId - STRID_USER_START];
}

///////////////////////////////////////////////////////////////////////////////
//
//  String (LANG)

//!!!POOL_ID_LANG need to sync current_lang_id for load language correct
#if(_LANG_STORE_ == _INSIDECODE_)
//#NT#2010/10/15#Niven Cho -begin
//#NT#CMP_LANG
UINT32 Get_LanguageValue(UINT32 uhIndex)
{

	switch (uhIndex) {
	case LANG_EN:
		g_LangCurrTbl = (STRING_TABLE)&gDemoKit_String_EN;
		break;
#if (_BOARD_DRAM_SIZE_ > 0x04000000)

	case LANG_DE:
		g_LangCurrTbl = (STRING_TABLE)&gDemoKit_String_DE;
		break;
	case LANG_FR:
		g_LangCurrTbl = (STRING_TABLE)&gDemoKit_String_FR;
		break;
	case LANG_ES:
		g_LangCurrTbl = (STRING_TABLE)&gDemoKit_String_ES;
		break;
	case LANG_IT:
		g_LangCurrTbl = (STRING_TABLE)&gDemoKit_String_IT;
		break;
	case LANG_PO:
		g_LangCurrTbl = (STRING_TABLE)&gDemoKit_String_PO;
		break;
	case LANG_TC:
		g_LangCurrTbl = (STRING_TABLE)&gDemoKit_String_TC;
		break;
	case LANG_SC:
		g_LangCurrTbl = (STRING_TABLE)&gDemoKit_String_SC;
		break;
	case LANG_RU:
		g_LangCurrTbl = (STRING_TABLE)&gDemoKit_String_RU;
		break;
	case LANG_JP:
		g_LangCurrTbl = (STRING_TABLE)&gDemoKit_String_JP;
		break;
#endif
	}
	return g_LangCurrTbl;
}
//#NT#2010/10/15#Niven Cho -end
#endif
UINT32 Get_LanguageTable(void)
{
//#NT#2010/10/15#Niven Cho -begin
//#NT#CMP_LANG
	if (g_LangCurrTbl == 0) {
		g_LangCurrTbl = (STRING_TABLE)&gDemoKit_String_EN;
	}
	return g_LangCurrTbl;
//#NT#2010/10/15#Niven Cho -end
}
