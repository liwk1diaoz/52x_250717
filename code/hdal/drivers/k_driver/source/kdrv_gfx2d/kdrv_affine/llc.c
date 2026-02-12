/*
    Link-list Controller module

    @file       llc.c
    @ingroup    mIDrvIPP_Affine
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#include "llc.h"


/**
    LLC Null

    @param[in] tableIdx  Unsigned integer

    @return LLC_Cmd stored in UINT64
*/
UINT64 llc_null(UINT32 tableIdx)
{
	LLC_NULL llcCmd;

	llcCmd.LLC_Cmd = 0x0;

	llcCmd.Bit.bTable_Index = tableIdx;
	llcCmd.Bit.bCmd = LLC_CMD_ID_NULL;

	return llcCmd.LLC_Cmd;
}

/**
    LLC Update

    @param[in] byteEn  Unsigned integer
	@param[in] regOfs  Unsigned integer
	@param[in] regVal  Unsigned integer

    @return LLC_Cmd stored in UINT64
*/
UINT64 llc_update(UINT32 byteEn, UINT32 regOfs, UINT32 regVal)
{
	LLC_UPDATE llcCmd;

	llcCmd.LLC_Cmd = 0x0;

	llcCmd.Bit.bByte_En = byteEn;
	llcCmd.Bit.bReg_Ofs = regOfs;
	llcCmd.Bit.bReg_Val = regVal;
	llcCmd.Bit.bCmd = LLC_CMD_ID_UPDATE;

	return llcCmd.LLC_Cmd;
}

/**
    LLC Next LL

    @param[in] nextJobAddr  Unsigned integer
	@param[in] tableIdx  Unsigned integer

    @return LLC_Cmd stored in UINT64
*/
UINT64 llc_next_ll(UINT32 nextJobAddr, UINT32 tableIdx)
{
	LLC_NEXT_LL llcCmd;

	llcCmd.LLC_Cmd = 0x0;

	llcCmd.Bit.bNext_Job_Addr = nextJobAddr;
	llcCmd.Bit.bTable_Index = tableIdx;
	llcCmd.Bit.bCmd = LLC_CMD_ID_NEXT_LL;

	return llcCmd.LLC_Cmd;
}

/**
    LLC Next Update

    @param[in] nextCmdAddr  Unsigned integer

    @return LLC_Cmd stored in UINT64
*/
UINT64 llc_next_update(UINT32 nextCmdAddr)
{
	LLC_NEXT_UPDATE llcCmd;

	llcCmd.LLC_Cmd = 0x0;

	llcCmd.Bit.bNext_Cmd_Addr = nextCmdAddr;
	llcCmd.Bit.bCmd = LLC_CMD_ID_NEXT_UPDATE;

	return llcCmd.LLC_Cmd;
}

