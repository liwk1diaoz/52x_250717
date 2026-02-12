#include <stdio.h>
#include <kwrap/type.h>
#include <kwrap/debug.h>
#include <kwrap/error_no.h>
#include <strg_def.h>
#include "nand_int.h"

static BOOL bFWNANDOpened = FALSE; //<Indicate nand driver opened already or nott
static NAND_BOUNDARY m_boundary = {0};

static ER nandfw_open(void)
{
	if (bFWNANDOpened) {
		return E_OK;
	}
	nand_open();
	bFWNANDOpened = TRUE;
	return E_OK;
}

static ER nandfw_close(void)
{
	if (!bFWNANDOpened) {
		return E_OK;
	}
	nand_close();
	bFWNANDOpened = FALSE;
	return E_OK;
}

static ER nandfw_writeReservedBlock(INT8 *pcBuf, UINT32 uiLogicalBlockNum, UINT32 uiBlkCnt)
{
	if (m_boundary.bottom == 0) {
		DBG_ERR("STRG_SET_PARTITION_SIZE not set.\n");
	}

	NAND_RW rw = {
		.p_buf = (unsigned char *)pcBuf,
		.blk_ofs = uiLogicalBlockNum + (m_boundary.top/nand_get_block_size()),
		.blk_num = uiBlkCnt,
		.boundary = m_boundary,
	};

	if (nand_write(&rw) != 0) {
		return E_SYS;
	}

	return E_OK;
}

static ER nandfw_readReservedBlock(INT8 *pcBuf, UINT32 uiLogicalBlockNum, UINT32 uiBlkCnt)
{
	if (m_boundary.bottom == 0) {
		DBG_ERR("STRG_SET_PARTITION_SIZE not set.\n");
	}

	NAND_RW rw = {
		.p_buf = (unsigned char *)pcBuf,
		.blk_ofs = uiLogicalBlockNum + (m_boundary.top/nand_get_block_size()),
		.blk_num = uiBlkCnt,
		.boundary = m_boundary,
	};

	if (nand_read(&rw) != 0) {
		return E_SYS;
	}

	return E_OK;
}

static ER nandfw_extIOCtrl(STRG_EXT_CMD uiCmd, UINT32 param1, UINT32 param2)
{
	DBG_WRN("not support any ioctl.\n");
	return E_NOSPT;
}

static ER nandfw_setParam(STRG_SET_PARAM_EVT uiEvt, UINT32 uiParam1, UINT32 uiParam2)
{
	ER eRet = E_OK;
	switch (uiEvt) {
	case STRG_SET_PARTITION_SECTORS: {
			loff_t top = (loff_t)uiParam1 * nand_get_block_size();
			loff_t bottom = top + (loff_t)uiParam2 * nand_get_block_size();
			if (bottom > nand_get_phy_size()) {
				DBG_ERR("partition bottom larger than physical size. %llX > %llx\n",
						bottom,
						nand_get_phy_size());

				eRet = E_PAR;
			} else {
				m_boundary.top = top;
				m_boundary.bottom = bottom;
			}
		}
		break;
	default:
		DBG_WRN("not support: %d\n", uiEvt);
		eRet = E_NOSPT;
	}
	return eRet;
}

static ER nandfw_getParam(STRG_GET_PARAM_EVT uiEvt, UINT32 uiParam1, UINT32 uiParam2)
{
	ER eRet = E_OK;
	switch (uiEvt) {
	case STRG_GET_BEST_ACCESS_SIZE:
	case STRG_GET_SECTOR_SIZE:
		*(UINT32 *)(uiParam1) = (UINT32)nand_get_block_size();
		eRet = E_OK;
		break;
	case STRG_GET_DEVICE_PHY_SIZE:
		*(UINT32 *)(uiParam1) = (UINT32)nand_get_phy_size();
		break;

	case STRG_GET_DEVICE_PHY_SECTORS:
		*(UINT32 *)(uiParam1) = (UINT32)(nand_get_phy_size() / nand_get_block_size());
		break;
	default:
		DBG_WRN("not support: %d\n", uiEvt);
		eRet = E_NOSPT;
	}
	return eRet;
}

STORAGE_OBJ gNandFWObj5 = {STORAGE_DETACH, STRG_TYPE_NAND, nand_lock, nand_unlock, nandfw_open, nandfw_writeReservedBlock, nandfw_readReservedBlock, nandfw_close, nandfw_open, nandfw_close, nandfw_setParam, nandfw_getParam, nandfw_extIOCtrl};