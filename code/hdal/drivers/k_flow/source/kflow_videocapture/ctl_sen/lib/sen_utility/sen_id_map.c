/*
    Sensor Interface DAL library - id mapping

    Sensor Interface DAL library.

    @file       sen_id_map.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "sen_id_map_int.h"

UINT32 sen_get_drvdev_csi(CTL_SEN_ID id)
{
	KDRV_DEV_ENGINE drvdev_tbl[CTL_SEN_DRVDEV_IDX_CSI(CTL_SEN_DRVDEV_CSI_MAX) + 1] = {
		KDRV_SSENIF_ENGINE_CSI0, KDRV_SSENIF_ENGINE_CSI1, KDRV_SSENIF_ENGINE_CSI2, KDRV_SSENIF_ENGINE_CSI3,
		KDRV_SSENIF_ENGINE_CSI4, KDRV_SSENIF_ENGINE_CSI5, KDRV_SSENIF_ENGINE_CSI6, KDRV_SSENIF_ENGINE_CSI7
	};
	CTL_SEN_DRVDEV drvdev = g_ctl_sen_init_obj[id]->init_cfg_obj.drvdev;

	if ((CTL_SEN_DRVDEV_MASK_CSI(drvdev) >= CTL_SEN_DRVDEV_CSI_BASE) && (CTL_SEN_DRVDEV_MASK_CSI(drvdev) <= CTL_SEN_DRVDEV_CSI_MAX)) {
		return drvdev_tbl[CTL_SEN_DRVDEV_IDX_CSI(drvdev)];
	}

	CTL_SEN_DBG_ERR("drvdev 0x%.8x error\r\n", (unsigned int)drvdev);
	return CTL_SEN_IGNORE;
}

UINT32 sen_get_drvdev_lvds(CTL_SEN_ID id)
{
	KDRV_DEV_ENGINE drvdev_tbl[CTL_SEN_DRVDEV_IDX_LVDS(CTL_SEN_DRVDEV_LVDS_MAX) + 1] = {
		KDRV_SSENIF_ENGINE_LVDS0, KDRV_SSENIF_ENGINE_LVDS1, KDRV_SSENIF_ENGINE_LVDS2, KDRV_SSENIF_ENGINE_LVDS3,
		KDRV_SSENIF_ENGINE_LVDS4, KDRV_SSENIF_ENGINE_LVDS5, KDRV_SSENIF_ENGINE_LVDS6, KDRV_SSENIF_ENGINE_LVDS7
	};
	CTL_SEN_DRVDEV drvdev = g_ctl_sen_init_obj[id]->init_cfg_obj.drvdev;

	if ((CTL_SEN_DRVDEV_MASK_LVDS(drvdev) >= CTL_SEN_DRVDEV_LVDS_BASE) && (CTL_SEN_DRVDEV_MASK_LVDS(drvdev) <= CTL_SEN_DRVDEV_LVDS_MAX)) {
		return drvdev_tbl[CTL_SEN_DRVDEV_IDX_LVDS(drvdev)];
	}

	CTL_SEN_DBG_ERR("drvdev 0x%.8x error\r\n", (unsigned int)drvdev);
	return CTL_SEN_IGNORE;
}

UINT32 sen_get_drvdev_tge(CTL_SEN_ID id)
{
	KDRV_DEV_ENGINE drvdev_tbl[CTL_SEN_DRVDEV_IDX_TGE(CTL_SEN_DRVDEV_TGE_MAX) + 1] = {
		KDRV_VDOCAP_TGE_ENGINE0
	};
	CTL_SEN_DRVDEV drvdev = g_ctl_sen_init_obj[id]->init_cfg_obj.drvdev;

	if ((CTL_SEN_DRVDEV_MASK_TGE(drvdev) >= CTL_SEN_DRVDEV_TGE_BASE) && (CTL_SEN_DRVDEV_MASK_TGE(drvdev) <= CTL_SEN_DRVDEV_TGE_MAX)) {
//		return drvdev_tbl[CTL_SEN_DRVDEV_IDX_TGE(drvdev)]; // ch
		return drvdev_tbl[0];
	}

	CTL_SEN_DBG_ERR("drvdev 0x%.8x error\r\n", (unsigned int)drvdev);
	return CTL_SEN_IGNORE;
}

UINT32 sen_get_drvdev_tge_chsft(CTL_SEN_ID id)
{
	KDRV_DEV_ENGINE drvdev_tbl[CTL_SEN_DRVDEV_IDX_TGE(CTL_SEN_DRVDEV_TGE_MAX) + 1] = {
		TGE_CH_SFT0, TGE_CH_SFT1, TGE_CH_SFT2, TGE_CH_SFT3,
		TGE_CH_SFT4, TGE_CH_SFT5, TGE_CH_SFT6, TGE_CH_SFT7
	};
	CTL_SEN_DRVDEV drvdev = g_ctl_sen_init_obj[id]->init_cfg_obj.drvdev;

	if ((CTL_SEN_DRVDEV_MASK_TGE(drvdev) >= CTL_SEN_DRVDEV_TGE_BASE) && (CTL_SEN_DRVDEV_MASK_TGE(drvdev) <= CTL_SEN_DRVDEV_TGE_MAX)) {
		return drvdev_tbl[CTL_SEN_DRVDEV_IDX_TGE(drvdev)];
	}

	CTL_SEN_DBG_ERR("drvdev 0x%.8x error\r\n", (unsigned int)drvdev);
	return CTL_SEN_IGNORE;
}

UINT32 sen_get_drvdev_vx1(CTL_SEN_ID id)
{
	KDRV_DEV_ENGINE drvdev_tbl[CTL_SEN_DRVDEV_IDX_VX1(CTL_SEN_DRVDEV_VX1_MAX) + 1] = {
		KDRV_SSENIF_ENGINE_VX1_0, KDRV_SSENIF_ENGINE_VX1_1
	};
	CTL_SEN_DRVDEV drvdev = g_ctl_sen_init_obj[id]->init_cfg_obj.drvdev;

	if ((CTL_SEN_DRVDEV_MASK_VX1(drvdev) >= CTL_SEN_DRVDEV_VX1_BASE) && (CTL_SEN_DRVDEV_MASK_VX1(drvdev) <= CTL_SEN_DRVDEV_VX1_MAX)) {
		return drvdev_tbl[CTL_SEN_DRVDEV_IDX_VX1(drvdev)];
	}

	CTL_SEN_DBG_ERR("drvdev 0x%.8x error\r\n", (unsigned int)drvdev);
	return CTL_SEN_IGNORE;
}

UINT32 sen_get_drvdev_slvsec(CTL_SEN_ID id)
{
	KDRV_DEV_ENGINE drvdev_tbl[CTL_SEN_DRVDEV_IDX_SLVSEC(CTL_SEN_DRVDEV_SLVSEC_MAX) + 1] = {
		KDRV_SSENIF_ENGINE_SLVSEC0
	};
	CTL_SEN_DRVDEV drvdev = g_ctl_sen_init_obj[id]->init_cfg_obj.drvdev;

	if ((CTL_SEN_DRVDEV_MASK_SLVSEC(drvdev) >= CTL_SEN_DRVDEV_SLVSEC_BASE) && (CTL_SEN_DRVDEV_MASK_SLVSEC(drvdev) <= CTL_SEN_DRVDEV_SLVSEC_MAX)) {
		return drvdev_tbl[CTL_SEN_DRVDEV_IDX_SLVSEC(drvdev)];
	}

	CTL_SEN_DBG_ERR("drvdev 0x%.8x error\r\n", (unsigned int)drvdev);
	return CTL_SEN_IGNORE;
}

