#include <stdio.h>
#include <kwrap/type.h>
#include <kwrap/debug.h>
#include <kwrap/error_no.h>
#include <strg_def.h>
#include "emmc_int.h"

static BOOL bEMMCFATOpened = FALSE; //<Indicate nand driver opened already or nott
static EMMC_BOUNDARY m_boundary = {0};

static ER emmcfat_open(void)
{
	if (bEMMCFATOpened) {
		return E_OK;
	}
	emmc_open();
	// init boundary matched to phyical device size
	m_boundary.top = 0;
	m_boundary.bottom = emmc_get_phy_size();
	bEMMCFATOpened = TRUE;
	return E_OK;
}

static ER emmcfat_close(void)
{
	if (!bEMMCFATOpened) {
		return E_OK;
	}
	emmc_close();
	bEMMCFATOpened = FALSE;
	return E_OK;
}

static ER emmcfat_writeReservedBlock(INT8 *pcBuf, UINT32 uiLogicalBlockNum, UINT32 uiBlkCnt)
{
	if (m_boundary.bottom == 0) {
		DBG_ERR("STRG_SET_PARTITION_SIZE not set.\n");
	}

	EMMC_RW rw = {
		.p_buf = (unsigned char *)pcBuf,
		.blk_ofs = uiLogicalBlockNum + (m_boundary.top/emmc_get_block_size()),
		.blk_num = uiBlkCnt,
		.boundary = m_boundary,
	};

	if (emmc_write(&rw) != 0) {
		return E_SYS;
	}

	return E_OK;
}

static ER emmcfat_readReservedBlock(INT8 *pcBuf, UINT32 uiLogicalBlockNum, UINT32 uiBlkCnt)
{
	if (m_boundary.bottom == 0) {
		DBG_ERR("STRG_SET_PARTITION_SIZE not set.\n");
	}

	EMMC_RW rw = {
		.p_buf = (unsigned char *)pcBuf,
		.blk_ofs = uiLogicalBlockNum + (m_boundary.top/emmc_get_block_size()),
		.blk_num = uiBlkCnt,
		.boundary = m_boundary,
	};

	if (emmc_read(&rw) != 0) {
		return E_SYS;
	}

	return E_OK;
}

static ER emmcfat_extIOCtrl(STRG_EXT_CMD uiCmd, UINT32 param1, UINT32 param2)
{
	DBG_WRN("not support any ioctl.\n");
	return E_NOSPT;
}

static ER emmcfat_setParam(STRG_SET_PARAM_EVT uiEvt, UINT32 uiParam1, UINT32 uiParam2)
{
	ER eRet = E_OK;
	switch (uiEvt) {
	default:
		DBG_WRN("not support: %d\n", uiEvt);
		eRet = E_NOSPT;
	}
	return eRet;
}

static ER emmcfat_getParam(STRG_GET_PARAM_EVT uiEvt, UINT32 uiParam1, UINT32 uiParam2)
{
	ER eRet = E_OK;
	switch (uiEvt) {
	case STRG_GET_BEST_ACCESS_SIZE:
	case STRG_GET_SECTOR_SIZE:
		*(UINT32 *)(uiParam1) = (UINT32)emmc_get_block_size();
		eRet = E_OK;
		break;
	case STRG_GET_DEVICE_PHY_SIZE:
		*(UINT32 *)(uiParam1) = (UINT32)emmc_get_phy_size();
		break;

	case STRG_GET_DEVICE_PHY_SECTORS:
		*(UINT32 *)(uiParam1) = (UINT32)(emmc_get_phy_size() / emmc_get_block_size());
		break;
	default:
		DBG_WRN("not support: %d\n", uiEvt);
		eRet = E_NOSPT;
	}
	return eRet;
}

STORAGE_OBJ gEMMCObj_fat = {STORAGE_DETACH, STRG_TYPE_EMMC, emmc_lock, emmc_unlock, emmcfat_open, emmcfat_writeReservedBlock, emmcfat_readReservedBlock, emmcfat_close, emmcfat_open, emmcfat_close, emmcfat_setParam, emmcfat_getParam, emmcfat_extIOCtrl};