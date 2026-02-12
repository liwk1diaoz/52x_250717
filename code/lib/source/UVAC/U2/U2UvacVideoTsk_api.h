/////////////////////////////////////////////////////////////////
/*
    Copyright (c) 2004~  Novatek Microelectronics Corporation

    @file UvacVideoTsk.h the pc-cam device firmware

    @hardware: Novatek 96610

    @version

    @date

*//////////////////////////////////////////////////////////////////
#ifndef _U2UVACVIDEOTSKAPI_H
#define _U2UVACVIDEOTSKAPI_H

#include "UVAC.h"
#include "../uvac_compat.h"

extern void U2UVAC_InstallID(void);
extern void U2UVAC_UnInstallID(void);
extern void U2UVAC_Close(void);
extern UINT32 U2UVAC_GetNeedMemSize(void);
extern void U2UVAC_SetConfig(UVAC_CONFIG_ID ConfigID, UINT32 Value);
extern void U2UVAC_SetEachStrmInfo(PUVAC_STRM_FRM pStrmFrm);
extern void U2UVC_SetTestImg(UINT32 imgAddr, UINT32 imgSize);
extern ER U2UVAC_ConfigVidReso(PUVAC_VID_RESO pVidReso, UINT32 cnt);
extern UINT32 U2UVAC_Open(UVAC_INFO *pClassInfo);
extern INT32 U2UVAC_ReadCdcData(CDC_COM_ID ComID, void *p_buf, UINT32 buffer_size, INT32 timeout);
extern void U2UVAC_AbortCdcRead(CDC_COM_ID ComID);
extern INT32 U2UVAC_WriteCdcData(CDC_COM_ID ComID, void *p_buf, UINT32 buffer_size, INT32 timeout);
extern UINT32 U2UVAC_GetCdcDataCount(CDC_COM_ID ComID);

extern ER U2UVAC_PullOutStrm(PUVAC_STRM_FRM pStrmFrm, INT32 wait_ms);
extern ER U2UVAC_ReleaseOutStrm(PUVAC_STRM_FRM pStrmFrm);

extern ER U2UVAC_WaitStrmDone(UVAC_STRM_PATH path);

extern UVAC_OPS UVAC_U2;

#endif  // _UVACVIDEOTSK_H

