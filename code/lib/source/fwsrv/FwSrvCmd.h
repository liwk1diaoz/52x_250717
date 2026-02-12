/**
    FwSrv, Service command function declare

    @file       FwSrvCmd.c
    @ingroup    mFWSRV

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _FWSRVCMD_H
#define _FWSRVCMD_H

typedef FWSRV_ER (*FWSRV_FP_CMD)(const FWSRV_CMD* pCmd); ///< for Command Function Table

/**
    Command Function Table Element
*/
typedef struct _FWSRV_CMD_DESC{
    FWSRV_CMD_IDX  Idx;            ///< Command Index
    FWSRV_FP_CMD   fpCmd;          ///< mapped function pointer
    UINT32          uiNumByteIn;    ///< require data in size
    UINT32          uiNumByteOut;   ///< require data out size
}FWSRV_CMD_DESC;

const FWSRV_CMD_DESC* xFwSrv_GetCallTbl(UINT32* pNum);

FWSRV_ER xFwSrv_CmdPlLoadBurst(const FWSRV_CMD* pCmd);
FWSRV_ER xFwSrv_CmdBinDePack(const FWSRV_CMD* pCmd);
FWSRV_ER xFwSrv_CmdBinUpdateAllInOne(const FWSRV_CMD* pCmd);
FWSRV_ER xFwSrv_CmdFastLoad(const FWSRV_CMD* pCmd);
#endif
