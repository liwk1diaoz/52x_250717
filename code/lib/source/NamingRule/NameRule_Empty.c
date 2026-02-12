/*
    Name Rule _FileDB.

    This file is the fileDB naming rule.

    @file       NameRule_Empty.c
    @ingroup    mIMPENC
    @version    V1.01.001
    @date       2013/03/12

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/

//#include "kernel.h"
#include "avfile/MediaWriteLib.h"
#include "avfile/MediaReadLib.h"
//#include "rtc.h"
//#include "Perf.h"

#include "NamingRule/NameRule_Empty.h"
#include "kwrap/error_no.h"

#define __MODULE__          NameRuleEmpty
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include <kwrap/debug.h>


UINT32 gNH_empty_msg = 0;

static void NH_Empty_SetInfo(MEDIANAMING_SETINFO_TYPE type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	if (gNH_empty_msg) {
		DBG_IND("Set Info type = %d!! p1= %d!\r\n", type, p1);
	}
	switch (type) {
	default:
		break;
	}
}


static ER NH_Empty_CreateAndGetPath(UINT32 filetype, CHAR *pPath)
{
	DBG_IND("CreateAndGetPath:should not go HERE!!\r\n");
	return E_OK;
}

static ER NH_Empty_AddPath(CHAR *pPath, UINT32 attrib)
{
	DBG_IND("AddPath:should not go HERE!!\r\n");
	return E_OK;

}

static ER NH_Empty_DelPath(CHAR *pPath)
{
	DBG_IND("DelPath:should not go HERE!!\r\n");
	return E_OK;
}


static ER NH_Empty_GetPathBySeq(UINT32 uiSequ, CHAR *pPath)
{

	DBG_IND("GetPathBySeq:should not go HERE!!\r\n");
	return E_OK;
}

static ER NH_Empty_GetLastPath(CHAR *pPath)
{
	DBG_IND("GetLastPath:should not go HERE!!\r\n");
	return E_OK;
}

static ER NH_Empty_customizeFunc(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3)
{
	switch (type) {
	default:
		break;
	}
	return E_OK;
}

MEDIANAMINGRULE  gNamingEmptyHdl = {
	NH_Empty_CreateAndGetPath, //CreateAndGetPath
	NH_Empty_AddPath,          //AddPath
	NH_Empty_DelPath,          //DelPath
	NH_Empty_GetPathBySeq,     //GetPathBySeq
	NULL, //NH_FileDB_CalcNextID,//CalcNextID
	NH_Empty_GetLastPath,//GetLastPath
	NULL,//NH_FileDB_GetNewCrashPath,//GetNewCrashPath
	NH_Empty_customizeFunc,
	NH_Empty_SetInfo,        //SetInfo
	NAMEHDL_EMPTY_CHECKID
};

PMEDIANAMINGRULE NameRule_getEmpty(void)
{
	MEDIANAMINGRULE *pHdl;


	pHdl = &gNamingEmptyHdl;
	return pHdl;
}


void NH_Empty_OpenMsg(UINT8 on)
{
	gNH_empty_msg = on;
}


