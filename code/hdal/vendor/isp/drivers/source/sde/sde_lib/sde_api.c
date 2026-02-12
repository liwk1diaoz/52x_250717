#if defined(__FREERTOS)
#include "string.h"
#include <stdio.h>
#include <kwrap/nvt_type.h>
#else
#include <linux/string.h>
#include "kwrap/type.h"
#endif
#include "kwrap/cpu.h"
#include "comm/hwclock.h"

#include "kflow_common/nvtmpp.h"

#include "isp_if.h"

#include "sde_lib.h"
#include "sde_api.h"
#include "sde_alg.h"
#include "sde_dbg.h"

#include "sde_api_int.h"
#include "sde_dev_int.h"

#include "sde_common_param_int.h"

#define TEST_MODE FALSE
#define RUN_1 TRUE
#define RUN_2 TRUE

//=============================================================================
// global
//=============================================================================
BOOL sde_opened = FALSE;
SDE_PARAM sde_drv_param[SDE_ID_MAX_NUM] = {0};

extern SDE_IF_PARAM_PTR *sde_if_param[SDE_ID_MAX_NUM];

//=============================================================================
// internal functions
//=============================================================================
static void sde_flow_dbg_msg(SDE_ID id)
{
	UINT32 dbg_mode = sde_dbg_get_dbg_mode(id);

	PRINT_SDE(dbg_mode & SDE_DBG_RUN_SET, "\r\n");
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].bScanEn);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].bDiagEn);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].bInput2En);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].bCostCompMode);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].bHflip01);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].bVflip01);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].bHflip2);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].bVflip2);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].cond_disp_en);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].conf_out2_en);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].disp_val_mode);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].opMode);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].ioPara.Size.uiWidth);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].ioPara.Size.uiHeight);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].ioPara.Size.uiOfsi0);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].ioPara.Size.uiOfsi1);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].ioPara.Size.uiOfsi2);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].ioPara.Size.uiOfso);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].ioPara.Size.uiOfso2);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].ioPara.uiInAddr0);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].ioPara.uiInAddr1);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].ioPara.uiInAddr2);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].ioPara.uiOutAddr);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].ioPara.uiOutAddr2);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].ioPara.uiInAddr_link_list);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].invSel);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].outSel);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].outVolSel);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].conf_min2_sel);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].conf_out2_mode);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].burstSel);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].uiIntrEn);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].costPara.uiAdBoundUpper);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].costPara.uiAdBoundLower);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].costPara.uiCensusBoundUpper);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].costPara.uiCensusBoundLower);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].costPara.uiCensusAdRatio);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].lumaPara.uiLumaThreshSaturated);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].lumaPara.uiCostSaturatedMatch);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].lumaPara.uiDeltaLumaSmoothThresh);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].penaltyValues.uiScanPenaltyNonsmooth);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].penaltyValues.uiScanPenaltySmooth0);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].penaltyValues.uiScanPenaltySmooth1);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].penaltyValues.uiScanPenaltySmooth2);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].thSmooth.uiDeltaDispLut0);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].thSmooth.uiDeltaDispLut1);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].confidence_th);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].thDiag.uiDiagThreshLut0);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].thDiag.uiDiagThreshLut1);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].thDiag.uiDiagThreshLut2);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].thDiag.uiDiagThreshLut3);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].thDiag.uiDiagThreshLut4);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].thDiag.uiDiagThreshLut5);
	PRINT_SDE_VAR(dbg_mode & SDE_DBG_RUN_SET, sde_drv_param[id].thDiag.uiDiagThreshLut6);
	PRINT_SDE(dbg_mode & SDE_DBG_RUN_SET, "\r\n");

}

static ER sde_setmode_run1(SDE_ID id)
{
	UINT32 dbg_mode = sde_dbg_get_dbg_mode(id);
	UINT32 t0, t1;
	ER rt = E_OK;

#if (TEST_MODE == FALSE)
	sde_drv_param[id].bDiagEn = FALSE;
	sde_drv_param[id].bInput2En = FALSE;
	sde_drv_param[id].bCostCompMode = TRUE;
	sde_drv_param[id].bHflip01 = TRUE;
	sde_drv_param[id].bVflip01 = TRUE;
	sde_drv_param[id].bHflip2 = FALSE;
	sde_drv_param[id].bVflip2 = FALSE;
	sde_drv_param[id].conf_out2_en = FALSE;
	sde_drv_param[id].outSel = SDE_OUT_VOL;
#endif

	// Ctrl parameters
	sde_drv_param[id].disp_val_mode = sde_if_param[id]->ctrl->disp_val_mode;
	sde_drv_param[id].opMode = (sde_if_param[id]->io_info->width <= 640) ? sde_if_param[id]->ctrl->disp_op_mode : SDE_MAX_DISP_31;
	sde_drv_param[id].invSel = sde_if_param[id]->ctrl->disp_inv_sel;

	// IO parameters
	sde_drv_param[id].ioPara.Size.uiWidth  = SDE_MIN(sde_if_param[id]->io_info->width, 1280);
	sde_drv_param[id].ioPara.Size.uiHeight = sde_if_param[id]->io_info->height;
	sde_drv_param[id].ioPara.Size.uiOfsi0 = sde_if_param[id]->io_info->in0_lofs;
	sde_drv_param[id].ioPara.Size.uiOfsi1 = sde_if_param[id]->io_info->in1_lofs;
	sde_drv_param[id].ioPara.Size.uiOfsi2 = 0;
	sde_drv_param[id].ioPara.Size.uiOfso = sde_if_param[id]->io_info->cost_lofs;   // output cost
	sde_drv_param[id].ioPara.Size.uiOfso2 = 0;
	sde_drv_param[id].ioPara.uiInAddr0 = nvtmpp_sys_pa2va(sde_if_param[id]->io_info->in0_addr);
	sde_drv_param[id].ioPara.uiInAddr1 = nvtmpp_sys_pa2va(sde_if_param[id]->io_info->in1_addr);
	sde_drv_param[id].ioPara.uiInAddr2 = 0;;
	sde_drv_param[id].ioPara.uiOutAddr = nvtmpp_sys_pa2va(sde_if_param[id]->io_info->cost_addr);   // output cost
	sde_drv_param[id].ioPara.uiOutAddr2 = 0;;
	sde_drv_param[id].ioPara.uiInAddr_link_list = 0;

	// set to SDE engine
	t0 = hwclock_get_counter();
	#if RUN_1
	rt |= sde_setMode(&sde_drv_param[id]);
	#endif
	t1 = hwclock_get_counter();
	PRINT_SDE(dbg_mode & SDE_DBG_PERFORMANCE, "SDE time setMode (%d) us!! \r\n", t1 - t0);

	return rt;
}

#if RUN_2
static ER sde_setmode_run2(SDE_ID id)
{
	UINT32 dbg_mode = sde_dbg_get_dbg_mode(id);
	UINT32 t0, t1;
	ER rt = E_OK;

#if (TEST_MODE == FALSE)
	sde_drv_param[id].bDiagEn = TRUE;
	sde_drv_param[id].bInput2En = TRUE;
	sde_drv_param[id].bCostCompMode = FALSE;
	sde_drv_param[id].bHflip01 = FALSE;
	sde_drv_param[id].bVflip01 = FALSE;
	sde_drv_param[id].bHflip2 = TRUE;
	sde_drv_param[id].bVflip2 = TRUE;
	sde_drv_param[id].outSel = SDE_OUT_DISPARITY;
#endif

	// Ctrl parameters
	sde_drv_param[id].conf_out2_en =  sde_if_param[id]->ctrl->conf_en;

	// IO parameters
	sde_drv_param[id].ioPara.Size.uiOfsi2 = sde_if_param[id]->io_info->cost_lofs;
	sde_drv_param[id].ioPara.Size.uiOfso = sde_if_param[id]->io_info->out0_lofs;
	sde_drv_param[id].ioPara.Size.uiOfso2 = sde_if_param[id]->io_info->out1_lofs;
	sde_drv_param[id].ioPara.uiInAddr2 = nvtmpp_sys_pa2va(sde_if_param[id]->io_info->cost_addr);
	sde_drv_param[id].ioPara.uiOutAddr = nvtmpp_sys_pa2va(sde_if_param[id]->io_info->out0_addr);
	sde_drv_param[id].ioPara.uiOutAddr2 = nvtmpp_sys_pa2va(sde_if_param[id]->io_info->out1_addr);

	// set to SDE engine
	t0 = hwclock_get_counter();
	rt |= sde_setMode(&sde_drv_param[id]);
	t1 = hwclock_get_counter();
	PRINT_SDE(dbg_mode & SDE_DBG_PERFORMANCE, "SDE time setMode2 (%d) us!! \r\n", t1 - t0);

	return rt;
}
#endif

//=============================================================================
// external functions
//=============================================================================
ER sde_flow_init(SDE_ID id)
{
	ER rt = E_OK;

	sde_drv_param[id] = sde_param_init;
	return rt;
}

ER sde_trigger(SDE_ID id)
{
	UINT32 dbg_mode = sde_dbg_get_dbg_mode(id);
	UINT32 t0, t1;
	ER rt = E_OK;

	if (sde_opened == FALSE) {
		PRINT_SDE_ERR(dbg_mode & SDE_DBG_ERR_MSG, "SDE is not open!! \r\n");
		return E_SYS;
	}

	if ((sde_if_param[id]->io_info->in0_addr == 0) || (sde_if_param[id]->io_info->in1_addr == 0)) {
		PRINT_SDE_ERR(dbg_mode & SDE_DBG_ERR_MSG, "SDE(%d) input address not ready!! \r\n", id);
		return E_SYS;
	}
	if (sde_if_param[id]->io_info->cost_addr == 0) {
		PRINT_SDE_ERR(dbg_mode & SDE_DBG_ERR_MSG, "SDE(%d) cost address not ready!! \r\n", id);
		return E_SYS;
	}
	if ((sde_if_param[id]->io_info->out0_addr == 0) || ((sde_if_param[id]->ctrl->conf_en == TRUE) && (sde_if_param[id]->io_info->out1_addr == 0))) {
		PRINT_SDE_ERR(dbg_mode & SDE_DBG_ERR_MSG, "SDE(%d) output address not ready!! \r\n", id);
		return E_SYS;
	}
	if ((sde_if_param[id]->io_info->width == 0) || (sde_if_param[id]->io_info->height == 0)) {
		PRINT_SDE_ERR(dbg_mode & SDE_DBG_ERR_MSG, "SDE(%d) process error, width(%d) and height(%d)!! \r\n", id, sde_if_param[id]->io_info->width, sde_if_param[id]->io_info->height);
		return E_SYS;
	}

	if (sde_if_param[id]->io_info->width > 1280) {
		PRINT_SDE_ERR(dbg_mode & SDE_DBG_ERR_MSG, "SDE(%d) process width(%d) is out of range, fixe to 1280!! \r\n", id, sde_if_param[id]->io_info->width);
		sde_if_param[id]->io_info->width = 1280;
	}
	if ((sde_if_param[id]->io_info->width > 640) && (sde_if_param[id]->ctrl->disp_op_mode == SDE_IF_MAX_DISP_63)) {
		PRINT_SDE_ERR(dbg_mode & SDE_DBG_ERR_MSG, "SDE(%d) disp_op_mode must be SDE_IF_MAX_DISP_63 when width > 640, fixed to SDE_IF_MAX_DISP_63!! \r\n", id);
		sde_if_param[id]->ctrl->disp_op_mode = SDE_MAX_DISP_31;
	}

	// run1
	rt |= sde_setmode_run1(id);
	if (rt != E_OK) {
		return rt;
	}
#if RUN_1
	sde_flow_dbg_msg(id);
	t0 = hwclock_get_counter();
	sde_clrFrameEnd();
	rt = sde_start();
	if (rt != E_OK) {
		return rt;
	}

	sde_waitFlagFrameEnd();
	sde_pause();
	t1 = hwclock_get_counter();
	PRINT_SDE(dbg_mode & SDE_DBG_PERFORMANCE, "SDE time start (%d) us!! \r\n", t1 - t0);
#endif

#if RUN_2
	// run2
	rt |= sde_setmode_run2(id);
	if (rt != E_OK) {
		return rt;
	}
	sde_flow_dbg_msg(id);
	t0 = hwclock_get_counter();
	sde_clrFrameEnd();
	rt = sde_start();
	if (rt != E_OK) {
		return rt;
	}

	sde_waitFlagFrameEnd();
	sde_pause();
	t1 = hwclock_get_counter();
	PRINT_SDE(dbg_mode & SDE_DBG_PERFORMANCE, "SDE time start2 (%d) us!! \r\n", t1 - t0);
#endif

	// for test
	#if 0
	memset((UINT32 *)sde_drv_param[id].ioPara.uiInAddr2, 0xff, sizeof(UINT8) * (sde_drv_param[id].ioPara.Size.uiOfsi2 << 2) * sde_drv_param[id].ioPara.Size.uiHeight);
	DBG_DUMP("set 255 test, addr=0x%x(PA = 0x%x), size = %d Byte\r\n"
		, sde_drv_param[id].ioPara.uiInAddr2, nvtmpp_sys_va2pa(sde_drv_param[id].ioPara.uiInAddr2)
		, sizeof(UINT8) * (sde_drv_param[id].ioPara.Size.uiOfsi2 << 2) * sde_drv_param[id].ioPara.Size.uiHeight);

	memset((UINT32 *)sde_drv_param[id].ioPara.uiOutAddr, 0x0, sizeof(UINT8) * (sde_drv_param[id].ioPara.Size.uiOfso << 2) * sde_drv_param[id].ioPara.Size.uiHeight);
	DBG_DUMP("set zero test, addr=0x%x(PA = 0x%x), size = %d Byte\r\n"
		, sde_drv_param[id].ioPara.uiOutAddr, nvtmpp_sys_va2pa(sde_drv_param[id].ioPara.uiOutAddr)
		, sizeof(UINT8) * (sde_drv_param[id].ioPara.Size.uiOfso << 2) * sde_drv_param[id].ioPara.Size.uiHeight);
	#endif

	// kernel flush
	#if 0
	t0 = hwclock_get_counter();
	vos_cpu_dcache_sync(sde_drv_param[id].ioPara.uiInAddr2, sizeof(UINT8) * sde_drv_param[id].ioPara.Size.uiOfsi2 * sde_drv_param[id].ioPara.Size.uiHeight, VOS_DMA_FROM_DEVICE);
	vos_cpu_dcache_sync(sde_drv_param[id].ioPara.uiOutAddr, sizeof(UINT8) * sde_drv_param[id].ioPara.Size.uiOfso * sde_drv_param[id].ioPara.Size.uiHeight, VOS_DMA_FROM_DEVICE);
	vos_cpu_dcache_sync(sde_drv_param[id].ioPara.uiOutAddr2, sizeof(UINT8) * sde_drv_param[id].ioPara.Size.uiOfso2 * sde_drv_param[id].ioPara.Size.uiHeight, VOS_DMA_FROM_DEVICE);

	t1 = hwclock_get_counter();
	PRINT_SDE(dbg_mode & SDE_DBG_PERFORMANCE, "SDE time flush (%d) us!! \r\n", t1 - t0);
	#endif

	sde_dbg_clr_dbg_mode(id, SDE_DBG_RUN_SET);
	return rt;
}

