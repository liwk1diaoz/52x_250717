/*
    Copyright   Novatek Microelectronics Corp. 2004.  All rights reserved.

    @file       UvacVideoTsk.c
    @ingroup    mISYSUVAC

    @brief      UVAC Video Task.
                UVAC Video Task.

*/
#include "U2UvacVideoTsk_api.h"

UVAC_OPS UVAC_U2 =
{
	UVAC_TYPE_U2,
	U2UVAC_Open,
	U2UVAC_Close,
	U2UVAC_GetNeedMemSize,
	U2UVAC_SetConfig,
	U2UVAC_SetEachStrmInfo,
	U2UVC_SetTestImg,
	U2UVAC_ConfigVidReso,
	U2UVAC_ReadCdcData,
	U2UVAC_AbortCdcRead,
	U2UVAC_WriteCdcData,
	U2UVAC_PullOutStrm,
	U2UVAC_ReleaseOutStrm,
	U2UVAC_WaitStrmDone,
};

