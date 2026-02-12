#include <stdio.h>
#include <kwrap/type.h>
#include <kwrap/debug.h>
#include <kwrap/error_no.h>
#include <strg_def.h>
#include "sdio_int.h"

static BOOL bSDIOFATOpened = FALSE; //<Indicate nand driver opened already or nott
static SDIO_BOUNDARY m_boundary = {0};

static ER sdiofat_open(void)
{
	if (bSDIOFATOpened) {
		return E_OK;
	}
	sdio_open();
	// init boundary matched to phyical device size
	m_boundary.top = 0;
	m_boundary.bottom = sdio_get_phy_size();
	bSDIOFATOpened = TRUE;
	return E_OK;
}

static ER sdiofat_close(void)
{
	if (!bSDIOFATOpened) {
		return E_OK;
	}
	sdio_close();
	bSDIOFATOpened = FALSE;
	return E_OK;
}

static ER sdiofat_writeReservedBlock(INT8 *pcBuf, UINT32 uiLogicalBlockNum, UINT32 uiBlkCnt)
{
	if (m_boundary.bottom == 0) {
		DBG_ERR("STRG_SET_PARTITION_SIZE not set.\n");
	}

	SDIO_RW rw = {
		.p_buf = (unsigned char *)pcBuf,
		.blk_ofs = uiLogicalBlockNum + (m_boundary.top/sdio_get_block_size()),
		.blk_num = uiBlkCnt,
		.boundary = m_boundary,
	};

	if (sdio_write(&rw) != 0) {
		return E_SYS;
	}

	return E_OK;
}

static ER sdiofat_readReservedBlock(INT8 *pcBuf, UINT32 uiLogicalBlockNum, UINT32 uiBlkCnt)
{
	if (m_boundary.bottom == 0) {
		DBG_ERR("STRG_SET_PARTITION_SIZE not set.\n");
	}

	SDIO_RW rw = {
		.p_buf = (unsigned char *)pcBuf,
		.blk_ofs = uiLogicalBlockNum + (m_boundary.top/sdio_get_block_size()),
		.blk_num = uiBlkCnt,
		.boundary = m_boundary,
	};

	if (sdio_read(&rw) != 0) {
		return E_SYS;
	}

	return E_OK;
}

static ER sdiofat_extIOCtrl(STRG_EXT_CMD uiCmd, UINT32 param1, UINT32 param2)
{
	DBG_WRN("not support any ioctl.\n");
	return E_NOSPT;
}

static ER sdiofat_setParam(STRG_SET_PARAM_EVT uiEvt, UINT32 uiParam1, UINT32 uiParam2)
{
	ER eRet = E_OK;
	switch (uiEvt) {
	default:
		DBG_WRN("not support: %d\n", uiEvt);
		eRet = E_NOSPT;
	}
	return eRet;
}

static ER sdiofat_getParam(STRG_GET_PARAM_EVT uiEvt, UINT32 uiParam1, UINT32 uiParam2)
{
	ER eRet = E_OK;
	switch (uiEvt) {
	case STRG_GET_BEST_ACCESS_SIZE:
	case STRG_GET_SECTOR_SIZE:
		*(UINT32 *)(uiParam1) = (UINT32)sdio_get_block_size();
		eRet = E_OK;
		break;
	case STRG_GET_DEVICE_PHY_SIZE:
		*(UINT32 *)(uiParam1) = (UINT32)sdio_get_phy_size();
		break;

	case STRG_GET_DEVICE_PHY_SECTORS:
		*(UINT32 *)(uiParam1) = (UINT32)(sdio_get_phy_size() / sdio_get_block_size());
		break;
	default:
		DBG_WRN("not support: %d\n", uiEvt);
		eRet = E_NOSPT;
	}
	return eRet;
}

STORAGE_OBJ gSDIOObj_fat = {STORAGE_DETACH, STRG_TYPE_SDIO, sdio_lock, sdio_unlock, sdiofat_open, sdiofat_writeReservedBlock, sdiofat_readReservedBlock, sdiofat_close, sdiofat_open, sdiofat_close, sdiofat_setParam, sdiofat_getParam, sdiofat_extIOCtrl};