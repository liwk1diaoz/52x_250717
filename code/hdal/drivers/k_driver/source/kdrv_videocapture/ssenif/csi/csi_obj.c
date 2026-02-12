/*
   CSI module driver Object File

    @file       csiobj.c
    @ingroup    mIDrvIO_CSI
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#ifdef __KERNEL__
#include <mach/rcw_macro.h>
#include "kwrap/type.h"
#include "csi_dbg.h"
#include "../csi.h"
#else
#if defined(__FREERTOS)
#define __MODULE__    rtos_csi
#include <kwrap/debug.h>
extern unsigned int rtos_csi_debug_level;
#include "kwrap/type.h"
#include "../csi.h"
#else
#include "DrvCommon.h"
#include "SxCmd.h"
#include "csi.h"
#endif
#endif
#include "include/csi_int.h"


#ifdef __KERNEL__
static CSIOBJ csi_obj[2] = {
	{
		CSI_ID_CSI,  csi_open, csi_close,   csi_is_opened,    csi_set_enable,   csi_get_enable,  csi_set_config,  csi_get_config,
		csi_wait_interrupt
	},
	{
		CSI_ID_CSI2, csi2_open, csi2_close, csi2_is_opened,   csi2_set_enable,  csi2_get_enable, csi2_set_config, csi2_get_config,
		csi2_wait_interrupt
	}
};

#else
static CSIOBJ csi_obj[2] = {
	{
		CSI_ID_CSI,  csi_open, csi_close,   csi_is_opened,    csi_set_enable,   csi_get_enable,  csi_set_config,  csi_get_config,
		csi_wait_interrupt
	},
	{
		CSI_ID_CSI2, csi2_open, csi2_close, csi2_is_opened,   csi2_set_enable,  csi2_get_enable, csi2_set_config, csi2_get_config,
		csi2_wait_interrupt
	}
};

#endif
void csi_error_parser(CSI_ID csi_id, UINT32 sts0, UINT32 sts1, UINT32 line_sta1)
{
	T_CSI_INTSTS0_REG  regsts0;
	T_CSI_INTSTS1_REG  regsts1;

	regsts0.reg = sts0;
	regsts1.reg = sts1;

	DBG_DUMP("CSI%dER:", (int)(csi_id+1));
	if (regsts0.bit.ECC_ERR) {
		DBG_DUMP(" ECC");
	}
	if (regsts0.bit.CRC_ERR) {
		DBG_DUMP(" CRC");
	}
	if (regsts0.bit.DATA_LT_WC) {
		DBG_DUMP(" LWC");
	}
	if (regsts0.reg & 0xF000) {
		DBG_DUMP(" SOT%X", (unsigned int)((regsts0.reg & 0xF000)>>12));
	}
	if (regsts0.reg & 0x40F00000) {
		DBG_DUMP(" STA%X-0x%05X", (unsigned int)((regsts0.reg & 0x40F00000)>>20), (unsigned int)(line_sta1&0xFFFFF));
	}
	if (regsts1.reg & 0xF) {
		DBG_DUMP(" ESC%X", (unsigned int)(regsts1.reg & 0xF));
	}
	if (regsts1.reg & 0xF0) {
		DBG_DUMP(" HST%X", (unsigned int)((regsts1.reg & 0xF0)>>4));
	}
	if (regsts1.reg & 0x30000000) {
		DBG_DUMP(" OVR%X", (unsigned int)((regsts1.reg & 0xF0000000)>>28));
	}
	DBG_DUMP("\r\n");
}

/**
    @addtogroup mIDrvIO_CSI
*/
//@{


PCSIOBJ csi_get_drv_object(CSI_ID csiid)
{
	if (csiid >= CSI_ID_MAX) {
		DBG_ERR("No such module.%d\r\n", (int)(csiid));
		return NULL;
	}

	return &csi_obj[csiid];
}
#ifdef __KERNEL__
EXPORT_SYMBOL(csi_get_drv_object);
#endif

//@}

#if 1
#endif

/*
    Install CSI Debug command

    @return void
void csi_installCmd(void)
{
	static BOOL bInstall = FALSE;
	static SXCMD_BEGIN(csi, "csi debug info")
	SXCMD_ITEM("debug1", csi_print_info_to_uart,  "dump csi-1 control info")
	SXCMD_ITEM("debug2", csi2_print_info_to_uart, "dump csi-2 control info")
	SXCMD_ITEM("debug3", csi3_printInfoToUart, "dump csi-3 control info")
	SXCMD_ITEM("debug4", csi4_printInfoToUart, "dump csi-4 control info")
	SXCMD_ITEM("debug5", csi5_printInfoToUart, "dump csi-5 control info")
	SXCMD_ITEM("debug6", csi6_printInfoToUart, "dump csi-6 control info")
	SXCMD_ITEM("debug7", csi7_printInfoToUart, "dump csi-7 control info")
	SXCMD_ITEM("debug8", csi8_printInfoToUart, "dump csi-8 control info")
	SXCMD_END()

	if (bInstall == FALSE)
	{
		SxCmd_AddTable(csi);
		bInstall = TRUE;
	}
}*/



//@}
