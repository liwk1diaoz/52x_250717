/*
    H265DEC module driver for NT96660

    use INIT and INFO to setup SeqCfg and PicCfg

    @file       h265dec_cfg.c

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
	
#include "kdrv_vdocdc_dbg.h"

#include "h265dec_cfg.h"

//! Initialize decoder configurate
/*!
    Convert pH265DecInit to SeqCfg

    @return
        - @b H265DEC_SUCCESS: init success
        - @b H265DEC_FAIL   : init fail
*/
INT32 h265Dec_initCfg(H265DEC_INIT *pH265DecInit, H265DecSeqCfg *pH265DecSeqCfg, H265DecPicCfg *pH265DecPicCfg, H265DecHdrObj *pH265DecHdrObj)
{
	//H265DecSpsObj *pH265DecSpsObj = &pH265DecHdrObj->sH265DecSpsObj;
	H265DecPpsObj *pH265DecPpsObj = &pH265DecHdrObj->sH265DecPpsObj;
	UINT32 i, tile_width=0;

	// Sequence Cfg //
	//pH265DecSeqCfg->uiSeqCfg  = (pH265DecSpsObj->uiLog2MaxFrmMinus4);
	//pH265DecSeqCfg->uiSeqCfg |= (pH265DecSpsObj->uiLog2MaxPocLsbMinus4<<4);

	// Picture Size //
	//pH265DecSeqCfg->uiPicSize  = ((pH265DecSpsObj->uiMbWidth - 1) << 4);
	//pH265DecSeqCfg->uiPicSize |= ((pH265DecSpsObj->uiMbHeight - 1) << 20);

	// Source Picture height //
	pH265DecSeqCfg->uiPicWidth = pH265DecInit->uiWidth;
	pH265DecSeqCfg->uiPicHeight = pH265DecInit->uiHeight;

	// Rec LineOffset //
	pH265DecSeqCfg->uiRecLineOffset  = pH265DecInit->uiYLineOffset;
	pH265DecSeqCfg->uiRecLineOffset |= (pH265DecInit->uiUVLineOffset << 16);

	// Tile Cfg //
	if (pH265DecPpsObj->tiles_enabled_flag) {
		if (pH265DecPpsObj->num_tile_columns_minus1 + 1 > MaxTileCols) {
			DBG_ERR("wrong tile columns number:%d, max:%d\r\n", (int)pH265DecPpsObj->num_tile_columns_minus1+1, (int)MaxTileCols);
			return H265DEC_FAIL;
		} else {
			for (i = 0; i < pH265DecPpsObj->num_tile_columns_minus1; i++) {
				pH265DecSeqCfg->uiTileWidth[i] = (pH265DecPpsObj->column_width[i] << 6);
				tile_width += pH265DecSeqCfg->uiTileWidth[i];
			}
			pH265DecSeqCfg->uiTileWidth[i] = pH265DecSeqCfg->uiPicWidth - tile_width;
			return H265DEC_SUCCESS;
		}
	}
	return H265DEC_SUCCESS;
}


