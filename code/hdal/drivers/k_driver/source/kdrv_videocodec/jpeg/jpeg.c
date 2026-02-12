/*
    @file       jpeg.c
    @ingroup    mIDrvCodec_JPEG

    @brief      Driver file for JPEG module
			This file is the driver file that define the driver API for JPEG module.

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#if defined __UITRON || defined __ECOS
#include "kernel.h"
#include "Type.h"
#include "dma.h"
#include "nvtDrvProtected.h"
#include "interrupt.h"

#if (_EMULATION_ == ENABLE)
#include "Delay.h"
#endif

#elif defined __KERNEL__
#include    <linux/dma-mapping.h> // header file Dma(cache handle)
#include    <mach/rcw_macro.h>
#include    "kwrap/type.h"
#include    "kwrap/flag.h"
#include    "kwrap/cpu.h"
#include    "kwrap/semaphore.h"
#include    <mach/fmem.h>
#include    "jpeg_dbg.h"
#include    <plat-na51055/nvt-sramctl.h>
#else
#include    "kwrap/type.h"
#include    "kwrap/flag.h"
#include    "kwrap/semaphore.h"
#include    "jpeg_dbg.h"
#include    "kwrap/spinlock.h"
#include    "dma_protected.h"
#include    "pll_protected.h"
#include    "interrupt.h"

#if defined(_BSP_NA51055_)
#include "plat/nvt-sramctl.h"
#endif


#endif
#include "../include/jpeg.h"
#include "../include/jpg_enc.h"
#include "jpeg_int.h"
#include "jpeg_reg.h"
#include "jpeg_platform.h"

#include "hal/jpg_int.h"
#include "kdrv_videoenc/kdrv_videoenc.h"
#include "brc/jpeg_brc.h"

#if defined(__FREERTOS)
irqreturn_t jpeg_isr(int irq, void *devid);
#else
void jpeg_isr(void);
#endif

//BRC control
#define JPEG_MAX_BRC_WEIGHTING       		100
#define JPEG_BRC_WEIGHTING            		60

//Record frame size
#define FRAME_WINDOW   30
#define CHECK_TOTALFRAMESIZE_NUM     100

#define JPEG_DEBUG   0

//static UINT32  uiNewQValue;
//static UINT32  uiTargetQF;

static UINT32               jpeg_intsts;
static JPEG_YUV_FORMAT      jpeg_yuvfmt       = JPEG_YUV_FORMAT_211;
static ID                   jpeg_lockstatus   = NO_TASK_LOCKED;
static BOOL                 jpeg_check_decerr = FALSE;
static BOOL                 jpeg_decerr_int_enable    = FALSE;

// Y, UV address
static JPEG_IMG_ADDR        jpeg_imgaddr         = { 0, 0 };
// DC output Y, UV address
static JPEG_IMG_ADDR        jpeg_dcoutaddr       = { 0, 0 };
// Bit-stream address
static JPEG_BS_ADDR         jpeg_bsaddr;

// Huffman base index table for decoding
static UINT8                jpeg_hufbase_table[4][16];
// Huffman mincode table for decoding
static UINT8                jpeg_hufmin_table[4][16];


#if defined(__FREERTOS)
#if defined(_BSP_NA51055_)
static UINT32               jpeg_clock = PLL_CLKSEL_JPEG_320;
#else
static UINT32               jpeg_clock = PLL_CLKSEL_JPEG_240;
#endif
#else
extern  UINT32             jpeg_clock;
#endif

//BRC control
extern UINT32 g_uiBrcCurrTarget_Rate[KDRV_VDOENC_ID_MAX+1];
extern UINT32 g_uiBrcTargetSize[KDRV_VDOENC_ID_MAX+1];
extern UINT32 g_uiBrcQF[KDRV_VDOENC_ID_MAX+1];
extern JPGBRC_PARAM g_brcParam[KDRV_VDOENC_ID_MAX+1];

//for VBR mode control
extern UINT32 g_uiVBR[KDRV_VDOENC_ID_MAX+1];
extern UINT32 g_uiVBRmodechange[KDRV_VDOENC_ID_MAX+1];
extern UINT32 g_uiBrcStdQTableQuality[KDRV_VDOENC_ID_MAX+1];
extern UINT32 g_uiVBR_QUALITY[KDRV_VDOENC_ID_MAX+1];

UINT32 g_uiFrameSize[KDRV_VDOENC_ID_MAX+1][FRAME_WINDOW];
UINT32 g_uiFRAME_WINDOW_size[KDRV_VDOENC_ID_MAX+1] = {0};

UINT32 g_CurrentFrameID[KDRV_VDOENC_ID_MAX+1] = {0};
UINT32 g_TotalFrameID[KDRV_VDOENC_ID_MAX+1] = {0};  // Total frame ID

UINT32 g_VBR_AVGsize[KDRV_VDOENC_ID_MAX+1] = {0};  // Keep total FRAME_WINDOW frame size

UINT32 g_reEnc[KDRV_VDOENC_ID_MAX+1] = {0};

/**
    @addtogroup mIDrvCodec_JPEG
*/
//@{

/*
    JPEG Interrupt Handler.

    JPEG Interrupt Handler.

    @return void
*/
#if defined(__FREERTOS)
irqreturn_t jpeg_isr(int irq, void *devid)
#else
void jpeg_isr(void)
#endif

{
	T_JPG_INT_EN    jpg_int_en;
	UINT32          status;

	// Get interrupt status
	// Only handle interrupt enabled status
	status = JPEG_GETREG(JPG_INT_STA_OFS) & JPEG_GETREG(JPG_INT_EN_OFS);

	if (status != 0) {
		jpeg_intsts = status;
		JPGDBG_IND("jpeg_intsts = %d\n", (int)(jpeg_intsts));

		// Disable all interrupt
		jpg_int_en.reg = 0;
		JPEG_SETREG(JPG_INT_EN_OFS, jpg_int_en.reg);

		// Clear interrupt status
		JPEG_SETREG(JPG_INT_STA_OFS, jpeg_intsts);

		// Set flag
		//iset_flg(FLG_ID_JPEG, FLGPTN_JPEG);
		jpeg_platform_set_flg();
	}
#if defined(__FREERTOS)
	return IRQ_HANDLED;
#endif
}




/*
    Lock JPEG Module.

    Lock JPEG Module.

    @return Lock result
	- @b E_OK       : Success
	- @b E_ID    : Outside semaphore ID number range
	- @b E_NOEXS    : Semaphore does not yet exist
*/
static ER jpeg_lock(void)
{
	ER ret = E_OK;

//#ifdef __KERNEL__
//	SEM_WAIT(SEMID_JPEG);
//#else
//	ret = wai_sem(SEMID_JPEG);
//#endif
	ret = jpeg_platform_sem_wait();

	if (ret != E_OK) {
		return ret;
	}

	jpeg_lockstatus = TASK_LOCKED;

	return E_OK;
}

/*
    Unlock JPEG Module.

    Unlock JPEG Module.

    @return Unlock result
	- @b E_OK   : Success
	- @b E_ID: Outside semaphore ID number range
	- @b E_NOEXS: Semaphore does not yet exist
	- @b E_QOVR : Semaphore's counter error, maximum counter < counter
*/
static ER jpeg_unlock(void)
{
	jpeg_lockstatus = NO_TASK_LOCKED;

	return jpeg_platform_sem_signal();

//#ifdef __KERNEL__
//	SEM_SIGNAL(SEMID_JPEG);
//	return E_OK;
//#else
//	return sig_sem(SEMID_JPEG);
//#endif
}

/**
    Get JPEG Lock Status.

    Get JPEG Lock Status.

    @return
	- @b NO_TASK_LOCKED: JPEG is NOT locked by any task
	- @b TASK_LOCKED: JPEG is locked some task
*/
ID jpeg_getlock_status(void)
{
	return jpeg_lockstatus;
}

/**
    Set JPEG driver configuration

    Set JPEG driver configuration.

    @param[in] ConfigID     configuration identifier
    @param[in] uiConfig     configuration context for configID

    @return
	- @b E_OK       : Set configuration success
	- @b E_NOSPT    : ConfigID is not supported
*/
ER jpeg_set_config(JPEG_CONFIG_ID cfg_id, UINT32 cfg)
{
return E_OK;
#if 0
	switch (cfg_id) {
	case JPEG_CONFIG_ID_CHECK_DEC_ERR:
		if (cfg == TRUE) {
			jpeg_check_decerr = TRUE;
		} else {
			jpeg_check_decerr = FALSE;
		}
		break;

	case JPEG_CONFIG_ID_FREQ:
		if (cfg < 240) {
			//DBG_WRN("input frequency %d round to 240 MHz\r\n", (int)(cfg));
			jpeg_clock = 240;//PLL_CLKSEL_JPEG_240;
		} else if (cfg == 240) {
			jpeg_clock = 240;//PLL_CLKSEL_JPEG_240;
		} else if (cfg < 320) {
			//DBG_WRN("input frequency %d round to 240 MHz\r\n", (int)(cfg));
			jpeg_clock = 240;//PLL_CLKSEL_JPEG_240;
		} else if (cfg == 320) {
			jpeg_clock = 320;//PLL_CLKSEL_JPEG_PLL6;
		}

#ifdef __KERNEL__

#else
//#if (_EMULATION_ == ENABLE)
 #if defined(_NVT_EMULATION_)
		// emu jpeg will use PLL6 as 380 clk src
		else if (cfg < 420) {
			//DBG_WRN("input frequency %d round to 380 MHz\r\n", (int)(cfg));
			JPGDBG_WRN("input frequency %d round to 380 MHz!\r\n", (int)(cfg));
			jpeg_clock = 360;
		}
#endif
#endif
		else {
			//DBG_WRN("input frequency %d round to 480 MHz\r\n", (int)(cfg));
			JPGDBG_WRN("input frequency %d round to 240 MHz!\r\n", (int)(cfg));
			jpeg_clock = 240;
		}
		//printk("jpeg_set_config ***** jpeg_clock = %d \r\n", jpeg_clock);
		break;

	default:
		JPGDBG_ERR("Not supported ID (%d)!\r\n", (int)(cfg_id));
		return E_NOSPT;
	}

	return E_OK;
#endif	
}

/**
    Get JPEG driver configuration

    Get JPEG driver configuration.

    @note   For configure ID "JPEG_CONFIG_ID_FREQ", the return value will be the real
			clock setting of controller. If the clock doesn't match your setting,
			please make sure you have called jpeg_open() after jpeg_set_config().

    @param[in] cfg_id     configuration identifier

    @return Value of the specific configuration identifier
*/
UINT32 jpeg_get_config(JPEG_CONFIG_ID cfg_id)
{
	UINT32 freq = 0;

	switch (cfg_id) {
	case JPEG_CONFIG_ID_CHECK_DEC_ERR:
		return (UINT32)jpeg_check_decerr;

	case JPEG_CONFIG_ID_FREQ:
		jpeg_platform_clk_get_freq(&freq);

		JPGDBG_IND("JPEG_CONFIG_ID_FREQ get (%d)!\r\n", (int)(freq));
		return freq;
	default:
		JPGDBG_ERR("Not supported ID (%d)!\r\n", (int)(cfg_id));
		return 0;
	}
}

/**
    Open JPEG Module

    Open JPEG Module. The following function will be disabled.
    - Restart marker (Encode and decode)
    - Format transform (Encode only)
    - DC raw output (Encode only)
    - Scaling (Decode only)
    - Cropping (Decode only)

    @return Open result
	- @b E_OK       : Open JPEG success
	- @b E_ID    : Outside semaphore ID number range
	- @b E_NOEXS    : Semaphore does not yet exist
*/
ER jpeg_open(void)
{
	ER ret;

#if defined(__FREERTOS)
    jpeg_create_resource();
    request_irq(INT_ID_JPEG, jpeg_isr, IRQF_TRIGGER_HIGH, "jpeg", 0);
#endif

	if (jpeg_getlock_status() == TASK_LOCKED){
		JPGDBG_WRN("JPEG have opened!\r\n");
		return E_OK;
	}

	// Lock JPEG
	ret = jpeg_lock();
	if (ret != E_OK) {
		JPGDBG_ERR("Lock error!\r\n");
		return ret;
	}

	jpeg_platform_clk_set_freq(jpeg_clock);

#if 0
#ifdef __KERNEL__

#else
	// Set JPEG clock
	switch (jpeg_clock) {
	case PLL_CLKSEL_JPEG_PLL6:
		if (pll_getPLLEn(PLL_ID_6) == FALSE) {
			pll_setPLLEn(PLL_ID_6, TRUE);
		}
		break;
	case PLL_CLKSEL_JPEG_PLL13:
		if (pll_getPLLEn(PLL_ID_13) == FALSE) {
			pll_setPLLEn(PLL_ID_13, TRUE);
		}
		break;
	default:
		break;
	}


	pll_setClockRate(PLL_CLKSEL_JPEG, jpeg_clock);
#endif
#endif
	// Clear flag
	//clr_flg(FLG_ID_JPEG, FLGPTN_JPEG);
	jpeg_platform_clear_flg();

#ifdef __KERNEL__
	nvt_disable_sram_shutdown(JPG_SD);
#else

#if defined(__FREERTOS)
#if defined(_BSP_NA51055_)
    nvt_disable_sram_shutdown(JPG_SD);
#endif

	// Enable JPEG Clock
	//KH mark  to do  pll_enableClock(JPG_CLK);

	// Enable JPEG interrupt
	//KH mark  to do   drv_enableInt(DRV_INT_JPEG);
#else

	// Turn on power
	pmc_turnonPower(PMC_MODULE_JPEG);
//#if (_FPGA_EMULATION_ == ENABLE)
	pll_disableSramShutDown(JPG_RSTN);
//#endif


	// Enable JPEG interrupt
	drv_enableInt(DRV_INT_JPEG);

#endif
	// Enable JPEG Clock
	pll_enableClock(JPG_CLK);

#endif

	// Init JPEG controller
	jpeg_reg_init();

	// Init variables
	jpeg_yuvfmt       = JPEG_YUV_FORMAT_211;
	jpeg_intsts        = 0;
	jpeg_decerr_int_enable    = FALSE;

	return E_OK;
}


/**
    Close JPEG Module.

    Close JPEG Module.

    @return Close result
	- @b E_OK   : Close JPEG success
	- @b E_SYS  : JPEG is not opened
	- @b E_ID: Outside semaphore ID number range
	- @b E_NOEXS: Semaphore does not yet exist
	- @b E_QOVR : Semaphore's counter error, maximum counter < counter
*/
ER jpeg_close(void)
{
	ER      ret;

	// Check lock status
	if (jpeg_lockstatus != TASK_LOCKED) {
		JPGDBG_ERR("Not locked!\r\n");
		return E_SYS;
	}

	// Todo: Wait for operation done??
#ifdef __KERNEL__

#else

#if defined(__FREERTOS)
#if defined(_BSP_NA51055_)
    nvt_enable_sram_shutdown(JPG_SD);
#endif
#else
	// Disable interrupt
	drv_disableInt(DRV_INT_JPEG);

	// Disable JPEG clock
	pll_disableClock(JPG_CLK);

#if ((_EMULATION_ == ENABLE) && (_FPGA_EMULATION_ == DISABLE))
	// Real chip verification, try to turn off power for PMC verification
	// Turn off power
	pmc_turnoffPower(PMC_MODULE_JPEG);

#if (_FPGA_EMULATION_ == ENABLE)
	pll_enableSramShutDown(JPG_RSTN);
#endif

#endif
#endif

#endif //__KERNEL__
	// Unlock JPEG module
	ret = jpeg_unlock();
	if (ret != E_OK) {
		JPGDBG_ERR("Unlock error!\r\n");
		return ret;
	}

	return E_OK;
}


/**
    Check if JPEG Driver is opened for accessing.

    Check if JPEG Driver is opened for accessing.

    @return Driver is opened or closed
	- @b TRUE   : JPEG driver is opened
	- @b FALSE  : JPEG driver is closed
 */
BOOL jpeg_is_opened(void)
{
	return (BOOL)(jpeg_lockstatus == TASK_LOCKED);
}

/**
    Get JPEG Interrupt Status.

    Get JPEG Interrupt Status.

    @return Interrupt status. Can be OR'd of
	- @b JPEG_INT_FRAMEEND
	- @b JPEG_INT_DECERR
	- @b JPEG_INT_BUFEND
*/
UINT32 jpeg_get_status(void)
{
	UINT8           *pffd9_addr;
	UINT32          status;
	T_JPG_CROP_ST   jpg_crop_st;
	T_JPG_CFG       jpg_cfg;
	T_JPG_BS_STADR  jpg_bs_stadr;

	// Get cropping status
	jpg_crop_st.reg     = JPEG_GETREG(JPG_CROP_ST_OFS);
	// Get mode
	jpg_cfg.reg         = JPEG_GETREG(JPG_CFG_OFS);
	// Get bit-stream starting address
	jpg_bs_stadr.reg    = JPEG_GETREG(JPG_BS_STADR_OFS);

	status = jpeg_intsts;

	// JPEG decode error status
	// Use FW to check 0xFFD9
	if ((jpeg_check_decerr == TRUE) &&
		(jpg_cfg.bit.dec_mod == JPEG_MODE_DECODE) &&    // Must be decode mode
		(jpeg_decerr_int_enable == TRUE) &&                 // Decode error interrupt must be enabled
		(status & JPEG_INT_FRAMEEND) &&                     // Frame end interrupt must be issued
		(jpg_crop_st.bit.crop_en == 0)                             // CODEC_BS_LEN_OUT is not valid in cropping mode
	) {
		// CODEC_BS_LEN_OUT is bit unit in decode mode
		// Check 0xFFD9 in the end of bit-stream
#ifdef __KERNEL__
	    // TODO check
		pffd9_addr = (UINT8 *)(jpeg_bsaddr + (ALIGN_CEIL_8(JPEG_GETREG(JPG_BS_CODECLEN_OFS)) >> 3));
#else
		pffd9_addr = (UINT8 *)(dma_getNonCacheAddr(jpg_bs_stadr.bit.bs_stadr) + (ALIGN_CEIL_8(JPEG_GETREG(JPG_BS_CODECLEN_OFS)) >> 3));
#endif
		if ((*pffd9_addr != 0xFF) || (*(pffd9_addr + 1) != 0xD9)) {
			status |= JPEG_INT_DECERR;
		}
	}

	return status;
}

/**
    Get JPEG Active Status.

    Get JPEG Active Status.

    @return
	- @b FALSE  : JPEG is inactive
	- @b TRUE   : JPEG is active
*/
UINT32 jpeg_get_activestatus(void)
{
	T_JPG_CTRL ctrl_reg;

	ctrl_reg.reg = JPEG_GETREG(JPG_CTRL_OFS);
	return ctrl_reg.bit.jpg_act;
}

/**
    Get JPEG compressed bit-stream size

    Get JPEG compressed bit-stream size.
    For encode mode, the size include 0xFFD9 (when encode done) and unit is byte.
    For decode mode, the size DOESN'T include 0xFFD9 and unit is bit.

    @return Compressed bit-stream size
*/
UINT32 jpeg_get_bssize(void)
{
	// CODEC LEN means encoded byte length in jpeg FIFO before frame done.
	// It dose NOT mean actual length outputting to DRAM.
	// Only use CODEC LEN after jpeg frame done.
	if (jpeg_get_activestatus()) {
		T_JPG_BS_STADR  jpg_bs_stadr;

		// Get bit-stream starting address
		jpg_bs_stadr.reg = JPEG_GETREG(JPG_BS_STADR_OFS);

		// JPEG is still active ==> no frame done
		// Current bit-stream address - starting address
		return (JPEG_GETREG(JPG_BS_CURADR_R_OFS) - jpg_bs_stadr.bit.bs_stadr);
	} else {
		// JPEG is inactive ==> frame done
		// CODEC_BS_LEN_OUT is byte unit in encode mode
		return JPEG_GETREG(JPG_BS_CODECLEN_OFS);
	}
}


/**
    JPEG Software Reset.

    JPEG Software Reset. For irregular termination.

    @return void
*/
void jpeg_set_swreset(void)
{
	T_JPG_CTRL  jpg_ctrl;
#ifdef __KERNEL__
	jpg_ctrl.reg            = 0;
#else
#if defined(_EMULATION_)
	jpg_ctrl.reg            = JPEG_GETREG(JPG_CTRL_OFS);
#else
	jpg_ctrl.reg            = 0;
#endif
#endif
	jpg_ctrl.bit.jpg_srst   = 1;
	JPEG_SETREG(JPG_CTRL_OFS, jpg_ctrl.reg);

//#if (_EMULATION_ == ENABLE)
	// DMA channels are disabled
	// if ((INW(0xC0008010) & 0x380000) == 0)
	// {
	// if (jpg_ctrl.bit.jpg_act == 0x1)
	// {
	// DBG_DUMP("JPEG SRST @ ACTIVE and DMA disabled!\r\n");
	// }
	// else
	// {
	// DBG_DUMP("JPEG SRST @ INACTIVE and DMA disabled!\r\n");
	// }
	// }
//#endif

	// Wait for software reset done
	do {
#ifdef __KERNEL__

#else
#if defined(_EMULATION_)
		// JPEG software reset will fail if JPEG is active and DMA channels are disabled.
		// DRAM consume task will not enable channel again if we don't let the caller enter
		// waiting state (The priority of DRAM consume task is very low).
		Delay_DelayUs(100);
#endif
#endif
		jpg_ctrl.reg = JPEG_GETREG(JPG_CTRL_OFS);
	} while (jpg_ctrl.bit.jpg_srst == 1);
}

/**
    Start JPEG Encode.

    Start JPEG Encode.

    @return void
*/
void jpeg_set_startencode(UINT32 addr , UINT32 size)
{
	T_JPG_CFG           jpg_cfg;
	T_JPG_CTRL          jpg_ctrl;
	T_JPG_SCALE         jpg_scale;
	T_JPG_DATA_SIZE     jpg_datasize;
	UINT32              y_size, uv_size;
	UINT32              line_offset, width, height;
	BOOL                clean_invalidateall;

	// Do we clean and invalidate all data cache in this API yet?
	clean_invalidateall = FALSE;

	// Clear interrupt status
	jpeg_intsts = 0;

	// Configure to encode mode
	jpg_cfg.reg = JPEG_GETREG(JPG_CFG_OFS);
	jpg_cfg.bit.dec_mod = JPEG_MODE_ENCODE;
	JPEG_SETREG(JPG_CFG_OFS, jpg_cfg.reg);

	// Software reset
	jpeg_set_swreset();

	// Flush image buffer cache
	jpg_datasize.reg = JPEG_GETREG(JPG_DATA_SIZE_OFS);

	JPGDBG_IND("jpeg_imgaddr.yaddr =0x%x    \r\n", (unsigned int)(jpeg_imgaddr.yaddr));
	// Y
	if (jpeg_platform_dma_is_cacheable(jpeg_imgaddr.yaddr) == TRUE) {
		line_offset    = (JPEG_GETREG(JPG_Y_LOFS_OFS) + 1);
		height        = ((jpg_datasize.bit.resol_ymcu + 1) << (((jpeg_yuvfmt & JPEG_BLK_Y_2Y) != 0) ? 4 : 3));
		y_size         = line_offset * height;

		// Only flush cache if we didn't clean and invalidate all data cache in this API before
		if (clean_invalidateall == FALSE) {
			clean_invalidateall = jpeg_platform_dma_flush_mem2dev(JPEG_MODE_ENCODE, jpeg_imgaddr.yaddr, y_size);
		}
	}
	JPGDBG_IND("jpeg_imgaddr.uvaddr =0x%x    \r\n", (unsigned int)(jpeg_imgaddr.uvaddr));

	// UV
	if ((jpeg_platform_dma_is_cacheable(jpeg_imgaddr.uvaddr) == TRUE) && (jpeg_yuvfmt != JPEG_YUV_FORMAT_100)) {
		line_offset    = (JPEG_GETREG(JPG_UV_LOFS_OFS) + 1);
		height        = ((jpg_datasize.bit.resol_ymcu + 1) << (((jpeg_yuvfmt & JPEG_BLK_U_2Y) != 0) ? 4 : 3));
		uv_size        = line_offset * height;

		// 411 source and output 211 bit-stream
		if ((jpeg_yuvfmt == JPEG_YUV_FORMAT_211) && (jpg_cfg.bit.fmt_tran_en == 1)) {
			uv_size >>= 1;
		}

		if (jpeg_yuvfmt == JPEG_YUV_FORMAT_100)  {
			uv_size = 0;
		}

		// Only flush cache if we didn't clean and invalidate all data cache in this API before
		if (clean_invalidateall == FALSE) {
			clean_invalidateall = jpeg_platform_dma_flush_mem2dev(JPEG_MODE_ENCODE, jpeg_imgaddr.uvaddr, uv_size);
		}
	}

	// Flush DC output buffer cache
	jpg_scale.reg = JPEG_GETREG(JPG_SCALE_OFS);
	if (jpg_scale.bit.dc_out_en == 1) {
		// Y
		if (jpeg_platform_dma_is_cacheable(jpeg_dcoutaddr.yaddr) == TRUE) {
			line_offset    = JPEG_GETREG(JPG_DC_Y_LOFS_OFS) + 1;
			width         = ((jpg_datasize.bit.resol_xmcu + 1) << (((jpeg_yuvfmt & JPEG_BLK_Y_2X) != 0) ? 4 : 3)) >> jpg_scale.bit.dc_out_hor;
			height        = ((jpg_datasize.bit.resol_ymcu + 1) << (((jpeg_yuvfmt & JPEG_BLK_Y_2Y) != 0) ? 4 : 3)) >> jpg_scale.bit.dc_out_ver;
			y_size         = line_offset * height;

			// Flush read cache need to take care the condition when line offset is not equal to width
			if (line_offset != width) {
				// Only flush cache if we didn't clean and invalidate all data cache in this API before
				if (clean_invalidateall == FALSE) {
					clean_invalidateall = jpeg_platform_dma_flush_dev2mem_width_neq_loff(JPEG_MODE_ENCODE, jpeg_dcoutaddr.yaddr, y_size);
				}
			} else {
				// Only flush cache if we didn't clean and invalidate all data cache in this API before
				if (clean_invalidateall == FALSE) {
					clean_invalidateall = jpeg_platform_dma_flush_dev2mem(JPEG_MODE_ENCODE, jpeg_dcoutaddr.yaddr, y_size);
				}
			}
		}

		// UV
		if ((jpeg_platform_dma_is_cacheable(jpeg_dcoutaddr.uvaddr) == TRUE) && (jpeg_yuvfmt != JPEG_YUV_FORMAT_100)) {
			line_offset    = JPEG_GETREG(JPG_DC_UV_LOFS_OFS) + 1;
			// UV width = U width + V width
			width         = ((jpg_datasize.bit.resol_xmcu + 1) << (((jpeg_yuvfmt & JPEG_BLK_U_2X) != 0) ? 5 : 4)) >> jpg_scale.bit.dc_out_hor;
			height        = ((jpg_datasize.bit.resol_ymcu + 1) << (((jpeg_yuvfmt & JPEG_BLK_U_2Y) != 0) ? 4 : 3)) >> jpg_scale.bit.dc_out_ver;
			uv_size        = line_offset * height;

			// Flush read cache need to take care the condition when line offset is not equal to width
			if (line_offset != width) {
				// Only flush cache if we didn't clean and invalidate all data cache in this API before
				if (clean_invalidateall == FALSE) {
					clean_invalidateall = jpeg_platform_dma_flush_dev2mem_width_neq_loff(JPEG_MODE_ENCODE, jpeg_dcoutaddr.uvaddr, uv_size);
				}
			} else {
				// Only flush cache if we didn't clean and invalidate all data cache in this API before
				if (clean_invalidateall == FALSE) {
					clean_invalidateall = jpeg_platform_dma_flush_dev2mem(JPEG_MODE_ENCODE, jpeg_dcoutaddr.uvaddr, uv_size);
				}
			}
		}
	}

	JPGDBG_IND("jpeg_bsaddr = 0x%x   \r\n", (unsigned int)(jpeg_bsaddr));
	// Flush bit-stream buffer cache
	// Only flush cache if we didn't clean and invalidate all data cache in this API before
	if ((jpeg_platform_dma_is_cacheable(jpeg_bsaddr) == TRUE) && (clean_invalidateall == FALSE)) {
		// use JPEG_MODE_DECODE parameter to enable flush API
		//clean_invalidateall = jpeg_platform_dma_flush_dev2mem(JPEG_MODE_DECODE, jpeg_bsaddr, JPEG_GETREG(JPG_BS_BUFLEN_OFS));
		clean_invalidateall = jpeg_platform_dma_flush_dev2mem_width_neq_loff(JPEG_MODE_DECODE, addr, size);
		JPGDBG_IND("clean_invalidateall = %d\r\n", (int)clean_invalidateall);
	}

	// Active JPEG
	jpg_ctrl.reg            = 0;
	jpg_ctrl.bit.jpg_act    = 1;
	JPEG_SETREG(JPG_CTRL_OFS, jpg_ctrl.reg);
}


void jpeg_flush_bsbuf(UINT32 addr, UINT32 size)
{
	// Flush bit-stream buffer cache
	// Only flush cache if we didn't clean and invalidate all data cache in this API before
	if (jpeg_platform_dma_is_cacheable(addr) == TRUE) {
		// use JPEG_MODE_DECODE parameter to enable flush API
		JPGDBG_IND("jpeg_flush_bsbuf (0x%x)!  %x\r\n", (unsigned int)(addr),(unsigned int)(size));
		//jpeg_platform_dma_flush_dev2mem(JPEG_MODE_DECODE, jpeg_bsaddr, size);
		jpeg_platform_dma_post_flush_dev2mem( addr, size);
	}
}


/**
    Start JPEG Decode.

    Start JPEG Decode.

    @return void
*/
void jpeg_set_startdecode(void)
{
	T_JPG_SCALE         jpg_scale;
	T_JPG_CFG           jpg_cfg;
	T_JPG_CTRL          jpg_ctrl;
	T_JPG_DATA_SIZE     jpg_datasize;
	T_JPG_CROP_ST       jpg_crop_st;
	UINT32              y_size, uv_size;
	UINT32              line_offset, width, height;
	UINT32              vertical_mcunum, horizontal_mcunum;
	BOOL                clean_invalidateall;

	// Do we clean and invalidate all data cache in this API yet?
	clean_invalidateall = FALSE;

	jpg_scale.reg = JPEG_GETREG(JPG_SCALE_OFS);

	// Clear interrupt status
	jpeg_intsts = 0;

	// Configure to decode mode
	jpg_cfg.reg = JPEG_GETREG(JPG_CFG_OFS);
	jpg_cfg.bit.dec_mod = JPEG_MODE_DECODE;
	JPEG_SETREG(JPG_CFG_OFS, jpg_cfg.reg);

	// Software reset
	jpeg_set_swreset();

	// Check decode scaling line offset here
	if (jpg_scale.bit.scale_en == 1) {
		// Decode scaling mode, line offset must be 8 words (32 bytes) alignment
		if (((JPEG_GETREG(JPG_Y_LOFS_OFS) + 1) & 0x1F) ||
			((jpeg_yuvfmt != JPEG_YUV_FORMAT_100) && ((JPEG_GETREG(JPG_UV_LOFS_OFS) + 1) & 0x1F))) {
			JPGDBG_WRN("Decode scaling, line offset isn't 32 bytes alignment!\r\n");
		}
	}

	// Flush bit-stream buffer cache
	// Only flush cache if we didn't clean and invalidate all data cache in this API before
	if ((jpeg_platform_dma_is_cacheable(jpeg_bsaddr) == TRUE) && (clean_invalidateall == FALSE)) {
	    clean_invalidateall = jpeg_platform_dma_flush_mem2dev(JPEG_MODE_DECODE, jpeg_bsaddr, JPEG_GETREG(JPG_BS_BUFLEN_OFS));
	}

	// Flush image buffer cache
	if (((jpeg_platform_dma_is_cacheable(jpeg_imgaddr.yaddr) == TRUE) ||
		 ((jpeg_platform_dma_is_cacheable(jpeg_imgaddr.uvaddr) == TRUE) && (jpeg_yuvfmt != JPEG_YUV_FORMAT_100))) &&
		(clean_invalidateall == FALSE)) {
		jpg_datasize.reg    = JPEG_GETREG(JPG_DATA_SIZE_OFS);
		jpg_crop_st.reg     = JPEG_GETREG(JPG_CROP_ST_OFS);

		// Cropping is enabled
		if (jpg_crop_st.bit.crop_en == 1) {
			T_JPG_CROP_SIZE jpg_crop_size;

			jpg_crop_size.reg = JPEG_GETREG(JPG_CROP_SIZE_OFS);

			horizontal_mcunum  = jpg_crop_size.bit.crop_xmcu;
			vertical_mcunum    = jpg_crop_size.bit.crop_ymcu;
		}
		// Cropping is disabled
		else {
			horizontal_mcunum  = jpg_datasize.bit.resol_xmcu + 1;
			vertical_mcunum    = jpg_datasize.bit.resol_ymcu + 1;
		}

		// Y
		if (jpeg_platform_dma_is_cacheable(jpeg_imgaddr.yaddr) == TRUE) {
			line_offset    = JPEG_GETREG(JPG_Y_LOFS_OFS) + 1;
			width         = horizontal_mcunum << (((jpeg_yuvfmt & JPEG_BLK_Y_2X) != 0) ? 4 : 3);
			height        = vertical_mcunum << (((jpeg_yuvfmt & JPEG_BLK_Y_2Y) != 0) ? 4 : 3);

			// Decode scaling might output data in non-MCU alignemnt,
			// must be handled after converting to byte number.
			// Scaling is enabled
			if (jpg_scale.bit.scale_en == 1) {
				width   >>= jpg_scale.bit.scale_mod;
				height  >>= jpg_scale.bit.scale_mod;
			}

			y_size         = line_offset * height;

			// Flush read cache need to take care the condition when line offset is not equal to width
			if (line_offset != width) {
				// Only flush cache if we didn't clean and invalidate all data cache in this API before
				if (clean_invalidateall == FALSE) {
					clean_invalidateall = jpeg_platform_dma_flush_dev2mem_width_neq_loff(JPEG_MODE_DECODE, jpeg_imgaddr.yaddr, y_size);
				}
			} else {
				// Only flush cache if we didn't clean and invalidate all data cache in this API before
				if (clean_invalidateall == FALSE) {
					clean_invalidateall = jpeg_platform_dma_flush_dev2mem(JPEG_MODE_DECODE, jpeg_imgaddr.yaddr, y_size);
				}
			}
		}

		// UV
		if ((jpeg_platform_dma_is_cacheable(jpeg_imgaddr.uvaddr) == TRUE) && (jpeg_yuvfmt != JPEG_YUV_FORMAT_100)) {
			line_offset    = JPEG_GETREG(JPG_UV_LOFS_OFS) + 1;
			width         = horizontal_mcunum << (((jpeg_yuvfmt & JPEG_BLK_U_2X) != 0) ? 5 : 4);
			height        = vertical_mcunum << (((jpeg_yuvfmt & JPEG_BLK_U_2Y) != 0) ? 4 : 3);

			// Decode scaling might output data in non-MCU alignemnt,
			// must be handled after converting to byte number.
			// Scaling is enabled
			if (jpg_scale.bit.scale_en == 1) {
				width   >>= jpg_scale.bit.scale_mod;
				height  >>= jpg_scale.bit.scale_mod;
			}

			uv_size        = line_offset * height;

			if (jpeg_yuvfmt == JPEG_YUV_FORMAT_100)  {
				uv_size = 0;
			}

			// Flush read cache need to take care carefully when line offset is not equal to width
			if (line_offset != width) {
				// Only flush cache if we didn't clean and invalidate all data cache in this API before
				if (clean_invalidateall == FALSE) {
					clean_invalidateall = jpeg_platform_dma_flush_dev2mem_width_neq_loff(JPEG_MODE_DECODE, jpeg_imgaddr.uvaddr, uv_size);
				}
			} else {
				// Only flush cache if we didn't clean and invalidate all data cache in this API before
				if (clean_invalidateall == FALSE) {
					clean_invalidateall = jpeg_platform_dma_flush_dev2mem(JPEG_MODE_DECODE, jpeg_imgaddr.uvaddr, uv_size);
				}
			}

			if(clean_invalidateall) {
				JPGDBG_IND("clean_invalidateall = TRUE \r\n");
			} else {
				JPGDBG_IND("clean_invalidateall = FALSE \r\n");
			}

		}
	}

	// Active JPEG
	jpg_ctrl.reg            = 0;
	jpg_ctrl.bit.jpg_act    = 1;
	JPEG_SETREG(JPG_CTRL_OFS, jpg_ctrl.reg);
}

/**
    Finish JPEG encode

    Finish JPEG encode operation.

    @return void
*/
void jpeg_set_endencode(void)
{
	// If encoding is not finished, do software reset to force JPEG abort the operation.
	if ((jpeg_intsts & JPEG_INT_FRAMEEND) == 0) {
		jpeg_set_swreset();
//#if (_EMULATION_ == ENABLE)
		JPGDBG_IND("Abort encoding!\r\n");
//#endif
	}
}

/**
    Finish JPEG decode

    Finish JPEG decode operation.

    @return void
*/
void jpeg_set_enddecode(void)
{
	// If decoding is not finished, do software reset to force JPEG abort the operation.
	if ((jpeg_intsts & JPEG_INT_FRAMEEND) == 0) {
		jpeg_set_swreset();
//#if (_EMULATION_ == ENABLE)
		JPGDBG_IND("Abort decoding!\r\n");
//#endif
	}
}

/**
    Enable JPEG Interrupt.

    Enable JPEG Interrupt.

    @param[in] interrupt  Interrupt enable mask. Can be bit OR'd of:
	- @b JPEG_INT_FRAMEEND
	- @b JPEG_INT_DECERR
	- @b JPEG_INT_BUFEND

    @return
	- @b E_OK   : Success
	- @b E_SYS  : Parameter is not valid
*/
ER jpeg_set_enableint(UINT32 interrupt)
{
	T_JPG_INT_EN    jpg_int_en;

	interrupt &= JPEG_INT_ALL;

	if (interrupt == 0) {
		JPGDBG_ERR("Parameter error!\r\n");
		return E_SYS;
	}

	jpg_int_en.reg = JPEG_GETREG(JPG_INT_EN_OFS);

	if (interrupt & JPEG_INT_FRAMEEND) {
		jpg_int_en.bit.int_frame_end_en = 1;
	}

#if 0
	if (interrupt & JPEG_INT_SLICEDONE) {
		jpg_int_en.bit.int_slice_done_en = 1;
	}
#endif

	if (interrupt & JPEG_INT_DECERR) {
		// FW solution
//        jpg_int_en.bit.int_dec_err_en = 1;
		jpeg_decerr_int_enable = TRUE;
	}

	if (interrupt & JPEG_INT_BUFEND) {
		jpg_int_en.bit.int_bsbuf_end_en = 1;
	}

	JPEG_SETREG(JPG_INT_EN_OFS, jpg_int_en.reg);

	return E_OK;
}

/**
    Disable JPEG interrupt

    Disable JPEG interrupt.

    @param[in] interrupt  Interrupt disable mask. Can be bit OR'd of:
	- @b JPEG_INT_FRAMEEND
	- @b JPEG_INT_DECERR
	- @b JPEG_INT_BUFEND

    @return void
*/
void jpeg_set_disableint(UINT32 interrupt)
{
	T_JPG_INT_EN    jpg_int_en;

	interrupt &= JPEG_INT_ALL;

	if (interrupt == 0) {
		JPGDBG_ERR("Parameter error!\r\n");
		return;
	}

	jpg_int_en.reg = JPEG_GETREG(JPG_INT_EN_OFS);

	if (interrupt & JPEG_INT_FRAMEEND) {
		jpg_int_en.bit.int_frame_end_en     = 0;
	}

#if 0
	if (interrupt & JPEG_INT_SLICEDONE) {
		jpg_int_en.bit.int_slice_done_en    = 0;
	}
#endif

	if (interrupt & JPEG_INT_DECERR) {
		// FW solution
//        jpg_int_en.bit.int_dec_err_en       = 0;
		jpeg_decerr_int_enable = FALSE;
	}

	if (interrupt & JPEG_INT_BUFEND) {
		jpg_int_en.bit.int_bsbuf_end_en     = 0;
	}

	JPEG_SETREG(JPG_INT_EN_OFS, jpg_int_en.reg);
}

/**
    Set JPEG image starting address

    Set JPEG image (Y, U, V) starting address.

    @param[in] yaddr     Image Y address
	@note               Y address should be word alignment for decode mode, byte alignment for encode mode.
    @param[in] uaddr     Image UV address
	@note               UV address should be word alignment for decode mode, half-word alignment for encode mode.
    @param[in] vaddr     Obsoleted parameter

    @return void
*/
void jpeg_set_imgstartaddr(UINT32 yaddr, UINT32 uaddr, UINT32 vaddr)
{
	// Store logical address
	jpeg_imgaddr.yaddr     = yaddr;
	jpeg_imgaddr.uvaddr    = uaddr;

	// Set address (physical) to register
#ifdef __KERNEL__
	// TODO
	JPEG_SETREG(JPG_Y_STADR_OFS, fmem_lookup_pa(yaddr));
	JPEG_SETREG(JPG_UV_STADR_OFS, fmem_lookup_pa(uaddr));
#else
	JPEG_SETREG(JPG_Y_STADR_OFS, dma_getPhyAddr(yaddr));
	JPEG_SETREG(JPG_UV_STADR_OFS, dma_getPhyAddr(uaddr));
#endif
}

/**
    Get JPEG image starting address

    Get JPEG image (Y, U, V) starting address. The returned address will be non-cacheable address.

    @param[out] yaddr    Image Y address
    @param[out] uaddr    Image UV address
    @param[out] vaddr    Obsoleted parameter

    @return void
*/
void jpeg_get_imgstartaddr(UINT32 *yaddr, UINT32 *uaddr, UINT32 *vaddr)
{
#ifdef __KERNEL__
	 // TODO check
	*yaddr = JPEG_GETREG(JPG_Y_STADR_OFS);
	*uaddr = JPEG_GETREG(JPG_UV_STADR_OFS);
#else
	*yaddr = dma_getNonCacheAddr(JPEG_GETREG(JPG_Y_STADR_OFS));
	*uaddr = dma_getNonCacheAddr(JPEG_GETREG(JPG_UV_STADR_OFS));
#endif
}

/**
    Set JPEG image line offset

    Set JPEG image line offset. Uint: byte

    @param[in] YLOFS        Image Y line offset
	@note               It must be 8 words alignment in decode scaling and encode DC out mode, words alignment in other modes.
    @param[in] ULOFS        Image UV line offset
	@note               It must be 8 words alignment in decode scaling and encode DC out mode, words alignment in other modes.
    @param[in] VLOFS        Obsoleted parameter

    @return
	- @b E_OK   : Success
	- @b E_SYS  : Encounter constraints with YLOFS, ULOFS
*/
ER jpeg_set_imglineoffset(UINT32 YLOFS, UINT32 ULOFS, UINT32 VLOFS)
{
	// Check parameter
	if (((YLOFS == 0) || (YLOFS > JPEG_MAX_LINEOFFSET) || (YLOFS & 0x3)) ||
		((jpeg_yuvfmt != JPEG_YUV_FORMAT_100) && ((ULOFS == 0) || (ULOFS > JPEG_MAX_LINEOFFSET) || (ULOFS & 0x3)))) {
		JPGDBG_ERR("line offset error! Y: %d, UV: %d\r\n", (int)(YLOFS), (int)(ULOFS));
		return E_SYS;
	}

	JPEG_SETREG(JPG_Y_LOFS_OFS, YLOFS - 1);
	JPEG_SETREG(JPG_UV_LOFS_OFS, ULOFS - 1);

	return E_OK;
}

//EXPORT_SYMBOL(jpeg_set_imglineoffset);

/**
    Get JPEG image Y's line offset

    Get JPEG image Y's line offset. Unit: byte

    @return Image Y's line offset
*/
UINT32 jpeg_get_imglineoffsety(void)
{
	return (JPEG_GETREG(JPG_Y_LOFS_OFS) + 1);
}

/**
    Get JPEG image UV's line offset

    Get JPEG image UV's line offset. Unit: byte

    @return Image UV's line offset
*/
UINT32 jpeg_get_imglineoffsetu(void)
{
	return (JPEG_GETREG(JPG_UV_LOFS_OFS) + 1);
}


/**
    Get JPEG image V's line offset

    (OBSOLETE API)

    Get JPEG image V's line offset. Since NT96660 only support UV packed format.
    This API is obsoleted and always return 0.

    @return 0
*/
UINT32 jpeg_get_imglineoffsetv(void)
{
	return 0;
}


/**
    Set JPEG File Format.

    Set JPEG File Format.

    @param[in] imgwidth         Image width. Unit: pixel
    @param[in] imgheight        Image height. Unit: pixel
    @param[in] fmt              JPEG YUV format. Only support the following format now
							- @b JPEG_YUV_FORMAT_100
							- @b JPEG_YUV_FORMAT_211
							- @b JPEG_YUV_FORMAT_411

    @return
	- @b E_OK   : Success
	- @b E_SYS  : Image width/height out of range or FileFormat is incorrect
*/
ER jpeg_set_format(UINT32 imgwidth, UINT32 imgheight, JPEG_YUV_FORMAT fmt)
{
	T_JPG_CFG       jpg_cfg;
	T_JPG_MCU       jpg_mcu;
	T_JPG_DATA_SIZE jpg_datasize;

	UINT32 mcu_num, img_xb, img_yb;
	UINT32 nY_x, nY_y;
#if (JPEG_SUPPORT_ALL_FORMAT == ENABLE)
	UINT32 nU_x, nU_y;
	UINT32 nV_x;
	, nV_y;
#endif
	if ((imgwidth == 0) ||
		(imgheight == 0) ||
		(imgwidth > JPEG_MAX_W) ||
		(imgheight > JPEG_MAX_H)) {
		JPGDBG_ERR("Width or height error!\r\n");
		return E_SYS;
	}

	switch (fmt) {
	case JPEG_YUV_FORMAT_100:
	case JPEG_YUV_FORMAT_211:
	case JPEG_YUV_FORMAT_411:
#if (JPEG_SUPPORT_ALL_FORMAT == ENABLE)
	case JPEG_YUV_FORMAT_111:
	case JPEG_YUV_FORMAT_211V:
	case JPEG_YUV_FORMAT_222:
	case JPEG_YUV_FORMAT_222V:
	case JPEG_YUV_FORMAT_422:
	case JPEG_YUV_FORMAT_422V:
#endif
		break;

	default:
		JPGDBG_ERR("Unknown file format!\r\n");
		return E_SYS;
	}

	nY_x = JPEG_GET_YHOR_BLKNUM(fmt);
	nY_y = JPEG_GET_YVER_BLKNUM(fmt);
#if (JPEG_SUPPORT_ALL_FORMAT == ENABLE)
	nU_x = JPEG_GET_UHOR_BLKNUM(fmt);
	nU_y = JPEG_GET_UVER_BLKNUM(fmt);
	nV_x = JPEG_GET_VHOR_BLKNUM(fmt);
	nV_y = JPEG_GET_VVER_BLKNUM(fmt);
#endif
	// Save format
	jpeg_yuvfmt = fmt;

	// Default YUV format value
	jpg_cfg.reg = JPEG_GETREG(JPG_CFG_OFS);
	jpg_cfg.bit.img_fmt_uv      = JPEG_MCU_UV_FMT_1COMP;
	jpg_cfg.bit.img_fmt_y       = JPEG_MCU_Y_FMT_1COMP;
	jpg_cfg.bit.img_fmt_mcu     = JPEG_MCU_SLOPE_HORIZONTAL;

	if (nY_x == 2) {
		if (nY_y == 2) {
			jpg_cfg.bit.img_fmt_y = JPEG_MCU_Y_FMT_4COMP;
		} else {
			jpg_cfg.bit.img_fmt_y = JPEG_MCU_Y_FMT_2COMP;
		}
	}
#if (JPEG_SUPPORT_ALL_FORMAT == ENABLE)
	else if ((nY_x == 1) && (nY_y == 2)) {
		jpg_cfg.bit.img_fmt_y   = JPEG_MCU_Y_FMT_2COMP;
		jpg_cfg.bit.img_fmt_mcu = JPEG_MCU_SLOPE_VERTICAL;
	}
#endif

#if (JPEG_SUPPORT_ALL_FORMAT == ENABLE)
	if ((nU_x == 2) || (nU_y == 2)) {
		jpg_cfg.bit.img_fmt_uv = JPEG_MCU_UV_FMT_2COMP;

		if (nU_y == 2) {
			jpg_cfg.bit.img_fmt_mcu = JPEG_MCU_SLOPE_VERTICAL;
		}
	}
#endif

	// 1 channel or 3 channels
	if (jpeg_yuvfmt == JPEG_YUV_FORMAT_100) {
		jpg_cfg.bit.img_fmt_comp = JPEG_COLOR_COMP_1CH;
	} else {
		jpg_cfg.bit.img_fmt_comp = JPEG_COLOR_COMP_3CH;
	}

	// Set configure register
	JPEG_SETREG(JPG_CFG_OFS, jpg_cfg.reg);

	// nY_x, nY_y is 1 or 2
	img_xb = imgwidth >> (2 + nY_x);
	img_yb = imgheight >> (2 + nY_y);

	// Not MCU alignment
	if (imgwidth & ((1 << (2 + nY_x)) - 1)) {
		img_xb++;
	}
	// Not MCU alignment
	if (imgheight & ((1 << (2 + nY_y)) - 1)) {
		img_yb++;
	}

	mcu_num = img_xb * img_yb;

	// Set resolution
	jpg_datasize.reg = 0;
	jpg_datasize.bit.resol_xmcu = img_xb - 1;
	jpg_datasize.bit.resol_ymcu = img_yb - 1;
	JPEG_SETREG(JPG_DATA_SIZE_OFS, jpg_datasize.reg);

	// Set MCU count
	jpg_mcu.reg = 0;
	jpg_mcu.bit.mcu_num = mcu_num - 1;
	JPEG_SETREG(JPG_MCU_OFS, jpg_mcu.reg);

	return E_OK;
}

/**
    Wait for JPEG's operation finished

    Wait for JPEG's operation finished.

    @note   This function will not return unless JPEG interrupt is issued.

    @return Interrupt status, can be OR'd of:
	- @b JPEG_INT_FRAMEEND
	- @b JPEG_INT_DECERR
	- @b JPEG_INT_BUFEND
*/
UINT32 jpeg_waitdone(void)
{
	//FLGPTN flg;

	//wai_flg(&flg, FLG_ID_JPEG, FLGPTN_JPEG, TWF_ORW | TWF_CLR);
	jpeg_platform_wait_flg();

	return jpeg_get_status();
}

/**
    Check if JPEG's operation finished

    Check if JPEG's operation finished.

    @return
	- @b TRUE   : JPEG operation is done.
	- @b FALSE  : JPEG operation is NOT done.
*/
BOOL jpeg_waitdone_polling(void)
{
	//if (kchk_flg(FLG_ID_JPEG, FLGPTN_JPEG) & FLGPTN_JPEG) {
	if (jpeg_platform_check_flg()) {
		//clr_flg(FLG_ID_JPEG, FLGPTN_JPEG);
		jpeg_platform_clear_flg();
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
    Set JPEG bit-stream buffer starting address and size

    Set JPEG bit-stream buffer starting address and size.
    If buffer size < 256 byte, the buffer size will be 256 byte.
    If buffer size > 32MB - 1, the buffer size will be 32MB - 1 bytes

    @param[in] BSAddr       Bit-stream buffer address
    @param[in] BufSize      Bit-stream buffer size (Unit: byte) 256B ~ 32MB - 1

    @return
	- @b E_OK: success
*/
ER jpeg_set_bsstartaddr(UINT32 bs_addr, UINT32 buf_size)
{
	T_JPG_BS_STADR  jpg_bs_stadr;
	//T_JPG_BS        jpg_bs;
	T_JPG_CFG       jpg_cfg;
	T_JPG_CTRL      ctrl_reg;

	// Check buffer size
	if (buf_size < JPEG_MIN_BSLENGTH) {
		JPGDBG_WRN("Buffer size must >= %d\r\n", (int)(JPEG_MIN_BSLENGTH));
		buf_size = JPEG_MIN_BSLENGTH;
	} else if (buf_size > JPEG_MAX_BSLENGTH) {
		//JPGDBG_WRN("Buffer size must <= %d\r\n", (int)(JPEG_MAX_BSLENGTH));
		buf_size = JPEG_MAX_BSLENGTH;
	}

	// Set buffer size
	JPEG_SETREG(JPG_BS_BUFLEN_OFS, buf_size);

	// Flush cache when JPEG is active (buffer end interrupt is issued and re-assign new BS buffer).
	// If JPEG is not active, flush the cache when start encode or decode
	ctrl_reg.reg = JPEG_GETREG(JPG_CTRL_OFS);
	if (ctrl_reg.bit.jpg_act == 1) {
		jpg_cfg.reg = JPEG_GETREG(JPG_CFG_OFS);
#ifdef __KERNEL__
		// TODO
		if (jpg_cfg.bit.dec_mod == JPEG_MODE_ENCODE) {
			//fmem_dcache_sync((void *)bs_addr, buf_size, DMA_BIDIRECTIONAL);
		}
		// Decode mode
		else {
			vos_cpu_dcache_sync((VOS_ADDR)bs_addr, buf_size, VOS_DMA_TO_DEVICE);
		}
#else
		// Encode mode
		if (jpg_cfg.bit.dec_mod == JPEG_MODE_ENCODE) {
			dma_flushReadCache(bs_addr, buf_size);
		}
		// Decode mode
		else {
			dma_flushWriteCache(bs_addr, buf_size);
		}
#endif
	}

	// Store BS buffer address (logical)
	jpeg_bsaddr = bs_addr;
	//jpg_bs.reg = JPEG_GETREG(JPG_BS_OFS);
	// Set buffer starting address and valid bit
	jpg_bs_stadr.reg            = 0;
#ifdef __KERNEL__
	// TODO
	jpg_bs_stadr.bit.bs_stadr   = fmem_lookup_pa(bs_addr);
#else
	jpg_bs_stadr.bit.bs_stadr   = dma_getPhyAddr(bs_addr);
#endif
	jpg_bs_stadr.bit.bs_valid         = 1;
	JPEG_SETREG(JPG_BS_STADR_OFS, jpg_bs_stadr.reg);
	//JPEG_SETREG(JPG_BS_OFS, jpg_bs.reg);

	return E_OK;
}

/**
    Get JPEG bit-stream buffer starting address

    Get JPEG bit-stream buffer starting address. This will be non-cacheable address.

    @return JPEG bit-stream buffer starting address
*/
UINT32 jpeg_get_bsstartaddr(void)
{
	T_JPG_BS_STADR  jpg_bs_stadr;

	jpg_bs_stadr.reg = JPEG_GETREG(JPG_BS_STADR_OFS);
#ifdef __KERNEL__
	// TODO check
	return (UINT32)(jpg_bs_stadr.bit.bs_stadr);
#else
	return dma_getNonCacheAddr(jpg_bs_stadr.bit.bs_stadr);
#endif
}

/**
    Get JPEG current bit-stream buffer address.

    Get JPEG current bit-stream buffer address. This will be non-cacheable address.

    @return Current bit-stream buffer address
*/
UINT32 jpeg_get_bscurraddr(void)
{
#ifdef __KERNEL__
	// TODO check
	return JPEG_GETREG(JPG_BS_CURADR_R_OFS);
#else
	return dma_getNonCacheAddr(JPEG_GETREG(JPG_BS_CURADR_R_OFS));
#endif
}


/**
    Enable or disable JPEG bit-stream output

    Enable or disable JPEG bit-stream output.

    @param[in] bEn  Enable or disable bit-stream output
	- @b FALSE  : Disable bit-stream output
	- @b TRUE   : Enable bit-stream output
    @return void
*/
void jpeg_set_bsoutput(BOOL en)
{
	T_JPG_BS    jpg_bs;

	jpg_bs.reg = JPEG_GETREG(JPG_BS_OFS);

	if (en) {
		jpg_bs.bit.bs_out_en = 1;
	} else {
		jpg_bs.bit.bs_out_en = 0;
	}
	JPEG_SETREG(JPG_BS_OFS, jpg_bs.reg);
}

/**
    Set JPEG restart interval MCU number

    Set JPEG restart interval MCU number.

    @param[in] MCUNum       MCU number within the restart mark

    @return
	- @b E_OK   : Success.
	- @b E_SYS  : MCU number is 0
*/
ER jpeg_set_restartinterval(UINT32 mcu_num)
{
	ER ret = E_OK;

	//value could not be 0
	if (mcu_num != 0) {
		JPEG_SETREG(JPG_RSTR_OFS, (mcu_num - 1));
		ret = E_OK;
	} else {
		JPGDBG_ERR("Parameter error!\r\n");
		ret = E_SYS;
	}

	return ret;
}

/**
    Enable or disable JPEG "Restart Mark Mode"

    Enable or disable JPEG "Restart Mark Mode".

    @param[in] Enable   Enable or disable "Restart Mark Mode"
	- @b FALSE  : Disable restart mark mode
	- @b TRUE   : Enable restart mark mode

    @return Always return E_OK
*/
ER jpeg_set_restartenable(BOOL en)
{
	T_JPG_CFG    jpg_cfg;

	jpg_cfg.reg = JPEG_GETREG(JPG_CFG_OFS);

	if (en == TRUE) {
		jpg_cfg.bit.rstr_en = 1;
	} else {
		jpg_cfg.bit.rstr_en = 0;
	}
	JPEG_SETREG(JPG_CFG_OFS, jpg_cfg.reg);

	return E_OK;
}

/**
    Enable or disable JPEG "Restart Mark patch" (for dummy bytes before DRI tag)

    Enable or disable JPEG "Restart Mark patch".

    @param[in] Enable   Enable or disable "Restart Mark patch"
	- @b FALSE  : Disable restart mark patch
	- @b TRUE   : Enable restart mark patch

    @return void
*/
void jpeg_set_restartpatch(BOOL en)
{
	T_JPG_CFG           jpg_cfg;

	jpg_cfg.reg = JPEG_GETREG(JPG_CFG_OFS);

	if (en == TRUE) {
		jpg_cfg.bit.rstr_sync_en = 1;
	} else {
		jpg_cfg.bit.rstr_sync_en = 0;
	}

	JPEG_SETREG(JPG_CFG_OFS, jpg_cfg.reg);

}

/**
    Replace last restart mark with EOI (End Of Image)

    This API will find restart mark from the end of bit-stream buffer and replace
    it with EOI (0xFFD9).

    @return Bit-stream size include the EOI
*/
UINT32 jpeg_set_restarteof(void)
{
	UINT32 i;
	UINT32 bs_addr = jpeg_get_bsstartaddr();
	UINT32 bs_size = jpeg_get_bssize();

	for (i = (bs_size - 1); i != 0; i--) {
		if (*((UINT8 *)(bs_addr + i))   >= 0xD0 &&
			*((UINT8 *)(bs_addr + i))   <= 0xD7 &&
			*((UINT8 *)(bs_addr + i - 1)) == 0xFF) {
			*((UINT8 *)(bs_addr + i)) = 0xD9;
			return (i + 1);
		}
	}
	JPGDBG_ERR("Can't find 0xFFDX!\r\n");
	return 0;
}

/**
    Configure JPEG DC raw output function

    Configure JPEG DC raw output function.

    @param[in] pdcout_cfg        DC raw output configuration

    @return Configuration status
	- @b E_OK   : Success
	- @b E_SYS  : Parameters in DC raw output configuration are not valid.
*/
ER jpeg_set_dcout(PJPEG_DC_OUT_CFG pdcout_cfg)
{
	T_JPG_SCALE jpg_scale;
	ER ret = E_OK;

	if (pdcout_cfg->dc_enable == FALSE) {
		jpg_scale.reg = JPEG_GETREG(JPG_SCALE_OFS);
		jpg_scale.bit.dc_out_en     = 0;
		JPEG_SETREG(JPG_SCALE_OFS, jpg_scale.reg);

		ret = E_OK;
	} else if (pdcout_cfg->dc_enable == TRUE) {
		// Store DC output address (logical)
		jpeg_dcoutaddr.yaddr   = pdcout_cfg->dc_yaddr;
		jpeg_dcoutaddr.uvaddr  = pdcout_cfg->dc_uaddr;

		// Line offset must > 0, <= JPEG_MAX_LINEOFFSET, and 8 words (32 bytes) alignment
		if ((pdcout_cfg->dc_ylineoffset == 0) ||
			(pdcout_cfg->dc_ylineoffset > JPEG_MAX_LINEOFFSET) ||
			((pdcout_cfg->dc_ylineoffset & 0x1F) != 0) ||
			(pdcout_cfg->dc_ulineoffset == 0) ||
			(pdcout_cfg->dc_ulineoffset > JPEG_MAX_LINEOFFSET) ||
			((pdcout_cfg->dc_ulineoffset & 0x1F) != 0)) {
			JPGDBG_ERR("Line offset error!\r\n");
			return E_SYS;
		}

		// Enable DC out and set ratio
		jpg_scale.reg               = JPEG_GETREG(JPG_SCALE_OFS);
		jpg_scale.bit.dc_out_en     = 1;
		jpg_scale.bit.dc_out_hor    = pdcout_cfg->dc_xratio;
		jpg_scale.bit.dc_out_ver    = pdcout_cfg->dc_yratio;
		JPEG_SETREG(JPG_SCALE_OFS, jpg_scale.reg);

		// DC out starting address
#ifdef __KERNEL__
		// TODO
		JPEG_SETREG(JPG_DC_Y_STADR_OFS,  fmem_lookup_pa(pdcout_cfg->dc_yaddr));
		JPEG_SETREG(JPG_DC_UV_STADR_OFS, fmem_lookup_pa(pdcout_cfg->dc_uaddr));
#else
		JPEG_SETREG(JPG_DC_Y_STADR_OFS,  dma_getPhyAddr(pdcout_cfg->dc_yaddr));
		JPEG_SETREG(JPG_DC_UV_STADR_OFS, dma_getPhyAddr(pdcout_cfg->dc_uaddr));
#endif

		// DC out line offset
		JPEG_SETREG(JPG_DC_Y_LOFS_OFS,  pdcout_cfg->dc_ylineoffset - 1);
		JPEG_SETREG(JPG_DC_UV_LOFS_OFS, pdcout_cfg->dc_ulineoffset - 1);

		ret =  E_OK;
	} else {
		JPGDBG_ERR("DCEnable error!\r\n");
		ret = E_SYS;
	}

	return ret;
}

/**
    Enable JPEG decode cropping function

    Enable JPEG decode cropping function.

    @return void
*/
void jpeg_set_cropenable(void)
{
	T_JPG_CROP_ST    jpg_crop_st;

	jpg_crop_st.reg = JPEG_GETREG(JPG_CROP_ST_OFS);
	jpg_crop_st.bit.crop_en = 1;
	JPEG_SETREG(JPG_CROP_ST_OFS, jpg_crop_st.reg);
}


/**
    Disable JPEG decode cropping function

    Disable JPEG decode cropping function.

    @return void
*/
void jpeg_set_cropdisable(void)
{
	T_JPG_CROP_ST    jpg_crop_st;

	jpg_crop_st.reg = JPEG_GETREG(JPG_CROP_ST_OFS);
	jpg_crop_st.bit.crop_en = 0;
	JPEG_SETREG(JPG_CROP_ST_OFS, jpg_crop_st.reg);
}

/**
    Set JPEG decode cropping information.

    Set JPEG decode cropping information, includes starting point (X,Y)
    and croping width and height. The starting point, width and height
    will be aligned ceiling (round up) to MCU unit.

    @note You have to call jpeg_set_format() before calling this API.

    @param[in] StartX       Crop horizontal start point (Unit: pixel)
    @param[in] StartY       Crop vertical start point (Unit: pixel)
    @param[in] Width        Cropping width (Unit: pixel)
    @param[in] Height       Cropping height (Unit: pixel)

    @return
	- @b E_OK: success
	- @b E_SYS: cropped region is not valid
*/
ER jpeg_set_crop(UINT32 sta_x, UINT32 sta_y, UINT32 width, UINT32 height)
{
	UINT32          crop_x, crop_y, crop_width, crop_height;
	T_JPG_DATA_SIZE jpg_datasize;
	T_JPG_CROP_ST   jpg_crop_st;
	T_JPG_CROP_SIZE jpg_crop_size;

	// To MUC unit
	switch (jpeg_yuvfmt) {
	case JPEG_YUV_FORMAT_100:
		crop_x = (sta_x + 0x7) >> 3;
		crop_y = (sta_y + 0x7) >> 3;
		crop_width = (width  + 0x7) >> 3;
		crop_height = (height + 0x7) >> 3;

		// For MCU 100, crop width must >= 2
		if (crop_width < JPEG_MIN_CROP_W_100) {
			JPGDBG_ERR("MCU100 crop width must >= %d\r\n", (int)(JPEG_MIN_CROP_W_100 << 3));
			return E_SYS;
		}
		break;

	case JPEG_YUV_FORMAT_211:
		crop_x = (sta_x + 0xF) >> 4;
		crop_y = (sta_y + 0x7) >> 3;
		crop_width = (width  + 0xF) >> 4;
		crop_height = (height + 0x7) >> 3;
		break;

	case JPEG_YUV_FORMAT_411:
		crop_x = (sta_x + 0xF) >> 4;
		crop_y = (sta_y + 0xF) >> 4;
		crop_width = (width  + 0xF) >> 4;
		crop_height = (height + 0xF) >> 4;
		break;

#if (JPEG_SUPPORT_ALL_FORMAT == ENABLE)
	// Todo: Add more format....
	case JPEG_YUV_FORMAT_111:
		break;
#endif

	default:
		JPGDBG_ERR("Not supported format (0x%.4X)!\r\n", jpeg_yuvfmt);
		return E_SYS;
	}

	// Get resolution from register
	jpg_datasize.reg = JPEG_GETREG(JPG_DATA_SIZE_OFS);

	// Check X, Y and Width
	if ((crop_x >= (JPEG_MAX_CROP_X + 1)) ||                                            // Maximum X is JPEG_MAX_CROP_X
		(crop_y >= (JPEG_MAX_CROP_Y + 1)) ||                                          // Maximum Y is JPEG_MAX_CROP_Y
		(crop_width >= (JPEG_MAX_CROP_W + 1)) || (crop_width == 0) ||               // Maximum Width  is JPEG_MAX_CROP_W and can't be 0
		(crop_height >= (JPEG_MAX_CROP_H + 1)) || (crop_height == 0) ||                 // Maximum Height is JPEG_MAX_CROP_H and can't be 0
		((crop_x + crop_width) > (jpg_datasize.bit.resol_xmcu + (UINT32)1)) ||    // Cropping X + width  must <= total width
		((crop_y + crop_height) > (jpg_datasize.bit.resol_ymcu + (UINT32)1))) {    // Cropping Y + height must <= total height
		//DBG_ERR("Parameter error! X: %ld, Y: %ld, Width: %ld, Height: %ld\r\n",
		  JPGDBG_ERR("Parameter error! X: %d, Y: %d, Width: %d, Height: %d\r\n",
		      (int)sta_x, (int)sta_y, (int)width, (int)height);
		return E_SYS;
	}

	// Set cropping X, Y
	jpg_crop_st.reg = 0;
	jpg_crop_st.bit.crop_stxmcu = crop_x;
	jpg_crop_st.bit.crop_stymcu = crop_y;
	JPEG_SETREG(JPG_CROP_ST_OFS, jpg_crop_st.reg);

	// Set cropping width and height
	jpg_crop_size.reg = 0;
	jpg_crop_size.bit.crop_xmcu = crop_width;
	jpg_crop_size.bit.crop_ymcu = crop_height;
	JPEG_SETREG(JPG_CROP_SIZE_OFS, jpg_crop_size.reg);

	return E_OK;
}

/**
    Enable JPEG decode scaling function

    Enable JPEG decode scaling function.

    @return void
*/
void jpeg_set_scaleenable(void)
{
	T_JPG_SCALE    jpg_scale;

	jpg_scale.reg = JPEG_GETREG(JPG_SCALE_OFS);
	jpg_scale.bit.scale_en = 1;
	JPEG_SETREG(JPG_SCALE_OFS, jpg_scale.reg);
}

/**
    Disable JPEG decode scaling function

    Disable JPEG decode scaling function.

    @return void
*/
void jpeg_set_scaledisable(void)
{
	T_JPG_SCALE    jpg_scale;

	jpg_scale.reg = JPEG_GETREG(JPG_SCALE_OFS);
	jpg_scale.bit.scale_en = 0;
	JPEG_SETREG(JPG_SCALE_OFS, jpg_scale.reg);
}

/**
    Set JPEG decode scaling ratio

    Set JPEG decode scaling ratio.

    @param[in] ScaleRatio       Scaling ratio
	- @b JPEG_DECODE_RATIO_WIDTH_1_2
	- @b JPEG_DECODE_RATIO_BOTH_1_2
	- @b JPEG_DECODE_RATIO_BOTH_1_4
	- @b JPEG_DECODE_RATIO_BOTH_1_8

    @return
	- @b E_OK   : Success
	- @b E_SYS  : Invalid parameter
*/
ER jpeg_set_scaleratio(JPEG_DECODE_RATIO scale_ratio)
{
	T_JPG_SCALE    jpg_scale;
	ER ret = E_OK;

	// NT96650 has bug in decode scaling mode when ration is width 1/2 only.
	// Disable this function in design-in environment.
//#if (_EMULATION_ == ENABLE)
	if (scale_ratio <= JPEG_DECODE_RATIO_BOTH_1_8)
//#else
//    if ((ScaleRatio > JPEG_DECODE_RATIO_WIDTH_1_2) && (ScaleRatio <= JPEG_DECODE_RATIO_BOTH_1_8))
//#endif
	{
		jpg_scale.reg = JPEG_GETREG(JPG_SCALE_OFS);
		jpg_scale.bit.scale_mod = scale_ratio;
		JPEG_SETREG(JPG_SCALE_OFS, jpg_scale.reg);
		ret = E_OK;
	} else {
		JPGDBG_ERR("Parameter error!\r\n");
		ret = E_SYS;
	}
	return ret;
}

/**
    Enable JPEG encode format transform function

    Enable JPEG encode format transform function.
    This function can encode JPEG_YUV_FORMAT_411 image to JPEG_YUV_FORMAT_211 bit stream.
    The source format is 411 but the file format must set to 211.

    @return void
*/
void jpeg_set_fmttransenable(void)
{
	T_JPG_CFG    jpg_cfg;

	jpg_cfg.reg = JPEG_GETREG(JPG_CFG_OFS);
	jpg_cfg.bit.fmt_tran_en = 1;
	JPEG_SETREG(JPG_CFG_OFS, jpg_cfg.reg);
}

/**
    Disable JPEG encode format transform function

    Disable JPEG encode format transform function.

    @return void
*/
void jpeg_set_fmttransdisable(void)
{
	T_JPG_CFG    jpg_cfg;

	jpg_cfg.reg = JPEG_GETREG(JPG_CFG_OFS);
	jpg_cfg.bit.fmt_tran_en = 0;
	JPEG_SETREG(JPG_CFG_OFS, jpg_cfg.reg);
}

/**
    Set JPEG Rotation Mode.

    Set JPEG Rotation Mode.

    @param[in] RotateMode       Rotation mode
                                - @b JPG_ROTATE_DISABLE: disable
                                - @b JPG_ROTATE_CCW: counter-clockwise 90 angle
                                - @b JPG_ROTATE_CW: clockwise 90 angle
                                - @b JPG_ROTATE_180: 180 angle

    @return
        - @b E_OK: success
*/
ER jpeg_set_rotate(JPG_HW_ROTATE_MODE RotateMode)
{
    T_JPG_CFG jpg_cfg;

    jpg_cfg.reg = JPEG_GETREG(JPG_CFG_OFS);

    jpg_cfg.bit.rot_sel = RotateMode;
    jpg_cfg.bit.new_dma_sel = 1;

    JPEG_SETREG(JPG_CFG_OFS, jpg_cfg.reg);

    return E_OK;
}

/**
    Set JPEG Quantization table (For encode and decode)

    Set JPEG Quantization table to the controller (For encode and decode).

    @param[in] pucQTabY     Y Q-tab
    @param[in] pucQTabUV    Cb/Cr Q-tab

    @return
	- @b E_OK   : Success
	- @b E_SYS  : Content in Q-tab has error
*/
ER jpeg_set_hwqtable(UINT8 *pqtab_y, UINT8 *pqtab_uv)
{
	T_JPG_LUT   jpg_lut;
	UINT32      i, j;
	UINT32      reg;
	UINT8       *ptab;

	// Enable table access
	jpg_lut.reg = JPEG_GETREG(JPG_LUT_OFS);
	jpg_lut.bit.tbl_mod     = JPEG_TBL_ACCESS_EN;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Select Q table, table index = 0, table index auto incremental
	jpg_lut.bit.tbl_sel     = JPEG_TABSEL_QUANT;
	jpg_lut.bit.tbl_adr     = 0;
	jpg_lut.bit.tbl_autoi   = JPEG_TBL_AUTOI_EN;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	if (pqtab_y == NULL) {
		JPGDBG_ERR("Parameter error!\r\n");
	} else {
		// Update Q table (64 entries) for Y
		ptab = pqtab_y;
		// Write 4 entries (total 32 bits) at a time
		for (i = 16; i > 0; i--) {
			reg = 0;
			for (j = 0; j < 4; j++) {
				if (*ptab == 0) {
					JPGDBG_ERR("Content is 0\r\n");

					// De-select table
					jpg_lut.bit.tbl_sel     = JPEG_TABSEL_NONE;
					JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

					// Disable table access
					jpg_lut.bit.tbl_mod     = JPEG_TBL_ACCESS_DIS;
					JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

					return E_SYS;
				}

				reg |= (*ptab++) << (j << 3);
			}

			// Write 32bits data to Q table
			JPEG_SETREG(JPG_LUT_DATA_OFS, reg);
		}
	}

	// Y only format, UV's Q table might be NULL
	if (jpeg_yuvfmt != JPEG_YUV_FORMAT_100) {
		if (pqtab_uv == NULL) {
			JPGDBG_ERR("pucQTabUV is NULL\r\n");
		} else {
			// Update Q table (64 entries) for UV
			ptab = pqtab_uv;

			// Write 4 entries (total 32 bits) at a time
			for (i = 16; i > 0; i--) {
				reg = 0;
				for (j = 0; j < 4; j++) {
					if (*ptab == 0) {
						JPGDBG_ERR("Content is 0\r\n");

						// De-select table
						jpg_lut.bit.tbl_sel     = JPEG_TABSEL_NONE;
						JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

						// Disable table access
						jpg_lut.bit.tbl_mod     = JPEG_TBL_ACCESS_DIS;
						JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

						return E_SYS;
					}

					reg |= (*ptab++) << (j << 3);
				}

				JPEG_SETREG(JPG_LUT_DATA_OFS, reg);
			}
		}
	}

	// De-select table
	jpg_lut.bit.tbl_sel     = JPEG_TABSEL_NONE;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Disable table access
	jpg_lut.bit.tbl_mod     = JPEG_TBL_ACCESS_DIS;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	return E_OK;
}

/**
    Set JPEG Huffman table for encode mode

    Set JPEG Huffman table to the controller for encode mode.

    @param[in] phufftablumac    Huffman table for luminance AC
    @param[in] phufftablumdc    Huffman table for luminance DC
    @param[in] phufftabchrac    Huffman table for chrominance AC
    @param[in] phufftabchrdc    Huffman table for chrominance DC

    @return void
*/
void jpeg_enc_set_hufftable(UINT16 *phufftablumac, UINT16 *phufftablumdc, UINT16 *phufftabchrac, UINT16 *phufftabchrdc)
{
	T_JPG_LUT   jpg_lut;
	UINT32      i;
	UINT32      reg;
	UINT16      *phuff_table;

	// Enable table access
	jpg_lut.reg = JPEG_GETREG(JPG_LUT_OFS);
	jpg_lut.bit.tbl_mod     = JPEG_TBL_ACCESS_EN;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Select Huffman table, table index = 0, table index auto incremental
	jpg_lut.bit.tbl_sel     = JPEG_TABSEL_HUFFMAN;
	jpg_lut.bit.tbl_adr     = 0;
	jpg_lut.bit.tbl_autoi   = JPEG_TBL_AUTOI_EN;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Write Huffman code to table
	// Luminance AC
	// Write 2 entries (total 32 bits) at a time
	phuff_table = phufftablumac;
	for (i = 0; i < (0xA2 / 2); i++) {
		reg = *phuff_table++;
		reg |= (*phuff_table++) << 16;
		JPEG_SETREG(JPG_LUT_DATA_OFS, reg);
	}

	// Luminance DC
	// Write 2 entries (total 32 bits) at a time
	phuff_table = phufftablumdc;
	for (i = 0; i < (0xE / 2); i++) {
		reg = *phuff_table++;
		reg |= (*phuff_table++) << 16;
		JPEG_SETREG(JPG_LUT_DATA_OFS, reg);
	}

	// Chrominance AC
	// Write 2 entries (total 32 bits) at a time
	phuff_table = phufftabchrac;
	for (i = 0; i < (0xA2 / 2); i++) {
		reg = *phuff_table++;
		reg |= (*phuff_table++) << 16;
		JPEG_SETREG(JPG_LUT_DATA_OFS, reg);
	}

	// Chrominance DC
	// Write 2 entries (total 32 bits) at a time
	phuff_table = phufftabchrdc;
	for (i = 0; i < (0xC / 2); i++) {
		reg = *phuff_table++;
		reg |= (*phuff_table++) << 16;
		JPEG_SETREG(JPG_LUT_DATA_OFS, reg);
	}

	// De-select table
	jpg_lut.bit.tbl_sel     = JPEG_TABSEL_NONE;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Disable table access
	jpg_lut.bit.tbl_mod     = JPEG_TBL_ACCESS_DIS;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);
}

/**
    Set JPEG Huffman table for decode mode

    Set JPEG Huffman table (From JPEG file header) to the controller for decode mode.

    @param[in] phuffdc0th       point to Y-HuffTbl of jpg-file-header
    @param[in] phuffdc1th       point to UV-HuffTbl of jpg-file-header
    @param[in] phuffac0th       point to Y-HuffTbl of jpg-file-header
    @param[in] phuffac1th       point to UV-HuffTbl of jpg-file-header

    @return void
*/
void jpeg_set_decode_hufftabhw(UINT8 *phuffdc0th, UINT8 *phuffdc1th, UINT8 *phuffac0th, UINT8 *phuffac1th)
{
	T_JPG_LUT   jpg_lut;
	UINT32      i;
	UINT32      reg;
	UINT8       *phuffman_tab;

	// Set huffman symbol
	// Enable table access
	jpg_lut.reg = JPEG_GETREG(JPG_LUT_OFS);
	jpg_lut.bit.tbl_mod     = JPEG_TBL_ACCESS_EN;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Select huffman table, table index = 0, table index auto incremental
	jpg_lut.bit.tbl_sel     = JPEG_TABSEL_HUFFMAN;
	jpg_lut.bit.tbl_adr     = 0;
	jpg_lut.bit.tbl_autoi   = JPEG_TBL_AUTOI_EN;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Luminance AC
	phuffman_tab = (phuffac0th + 16);
	for (i = 0; i < (0xA2 / 2); i++) {
		reg = *phuffman_tab++;
		reg |= (*phuffman_tab++) << 16;
		JPEG_SETREG(JPG_LUT_DATA_OFS, reg);
	}

	// Chrominance AC
	phuffman_tab = (phuffac1th + 16);
	for (i = 0; i < (0xA2 / 2); i++) {
		reg = *phuffman_tab++;
		reg |= (*phuffman_tab++) << 16;
		JPEG_SETREG(JPG_LUT_DATA_OFS, reg);
	}

	// Luminance DC
	phuffman_tab = (phuffdc0th + 16);
	for (i = 0; i < (0xC / 2); i++) {
		reg = *phuffman_tab++;
		reg |= (*phuffman_tab++) << 16;
		JPEG_SETREG(JPG_LUT_DATA_OFS, reg);
	}

	// Chrominance DC
	phuffman_tab = (phuffdc1th + 16);
	for (i = 0; i < (0xC / 2); i++) {
		reg = *phuffman_tab++;
		reg |= (*phuffman_tab++) << 16;
		JPEG_SETREG(JPG_LUT_DATA_OFS, reg);
	}

	// De-select table
	jpg_lut.bit.tbl_sel     = JPEG_TABSEL_NONE;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Disable table access
	// We need to set base index and mincode table later, don't disable table access
//    jpg_lut.bit.tbl_mod     = JPEG_TBL_ACCESS_DIS;
//    JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	//----------------------------------------------------------------
	// Y-AC Min & Diff table
	//----------------------------------------------------------------
	jpeg_buildbaseidx_mincode(phuffac0th, jpeg_hufmin_table[0], jpeg_hufbase_table[0]);

	//----------------------------------------------------------------
	// UV-AC Min & Diff table
	//----------------------------------------------------------------
	jpeg_buildbaseidx_mincode(phuffac1th, jpeg_hufmin_table[2], jpeg_hufbase_table[2]);

	//----------------------------------------------------------------
	// Y-DC Min & Diff table
	//----------------------------------------------------------------
	jpeg_buildbaseidx_mincode(phuffdc0th, jpeg_hufmin_table[1], jpeg_hufbase_table[1]);

	//----------------------------------------------------------------
	// UV-DC Min & Diff table
	//----------------------------------------------------------------
	jpeg_buildbaseidx_mincode(phuffdc1th, jpeg_hufmin_table[3], jpeg_hufbase_table[3]);

	// Set base index table
	// Enable table access
	// We didn't disable table access in previous operation
//    jpg_lut.reg = JPEG_GETREG(JPG_LUT_OFS);
//    jpg_lut.bit.tbl_mod     = JPEG_TBL_ACCESS_EN;
//    JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Select base index table, table index = 0, table index auto incremental
	jpg_lut.bit.tbl_sel     = JPEG_TABSEL_BASEIDX;
	jpg_lut.bit.tbl_adr     = 0;
//    jpg_lut.bit.tbl_autoi   = JPEG_TBL_AUTOI_EN;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Set table
	jpeg_setbaseidx_mincode(jpeg_hufbase_table[0], jpeg_hufbase_table[1], jpeg_hufbase_table[2], jpeg_hufbase_table[3]);

	// De-select table
	jpg_lut.bit.tbl_sel     = JPEG_TABSEL_NONE;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Disable table access
	// We need to set base index and mincode table later, don't disable table access
//    jpg_lut.bit.tbl_mod     = JPEG_TBL_ACCESS_DIS;
//    JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Set mincode table
	// Enable table access
	// We didn't disable table access in previous operation
//    jpg_lut.reg = JPEG_GETREG(JPG_LUT_OFS);
//    jpg_lut.bit.tbl_mod     = JPEG_TBL_ACCESS_EN;
//    JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Select mincode table, table index = 0, table index auto incremental
	jpg_lut.bit.tbl_sel     = JPEG_TABSEL_MINCODE;
	jpg_lut.bit.tbl_adr     = 0;
//    jpg_lut.bit.tbl_autoi   = JPEG_TBL_AUTOI_EN;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Set table
	jpeg_setbaseidx_mincode(jpeg_hufmin_table[0], jpeg_hufmin_table[1], jpeg_hufmin_table[2], jpeg_hufmin_table[3]);

	// De-select table
	jpg_lut.bit.tbl_sel     = JPEG_TABSEL_NONE;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Disable table access
	jpg_lut.bit.tbl_mod     = JPEG_TBL_ACCESS_DIS;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);
}

/*
    Get JPEG cycle count

    Get JPEG cycle count.

    @return JPEG cycle count for current operation
*/
UINT32 jpeg_get_cyclecnt(void)
{
	return JPEG_GETREG(JPG_CYCLE_CNT_OFS);
}

/**
    Enable imgdma with new path (Only support YUV411 format)

    Enable imgdma with new path

    @param[in] Enable   Enable or disable "imgdma new path"
	- @b FALSE  : Disable imgdma new path
	- @b TRUE   : Enable imgdma new path

    @return
	    - @b E_OK   : Success
	    - @b E_SYS  : Enable imgdma new path failed
*/
ER jpeg_set_newimgdma(BOOL en)
{
	T_JPG_CFG    jpg_cfg;

    if (jpeg_yuvfmt != JPEG_YUV_FORMAT_411 ) {
        JPGDBG_ERR("New imgdma only support Fmt411 ! Current format is (0x%.4X)!\r\n", jpeg_yuvfmt);
		return E_SYS;
    }
	jpg_cfg.reg = JPEG_GETREG(JPG_CFG_OFS);

    if (en == TRUE) {
		jpg_cfg.bit.new_dma_sel = 1;
		//JPGDBG_ERR("[New IMGDMA path]  ============================    \r\n");
	} else {
		jpg_cfg.bit.new_dma_sel = 0;
	}

	JPEG_SETREG(JPG_CFG_OFS, jpg_cfg.reg);

	return E_OK;
}


/**    Set bitstream bma burst length

       Set bitstream bma burst length

       @param[in] bslen  BS DMA burst configuration
       - @b JPEG_BS_DMA_BURST_32W
       - @b JPEG_BS_DMA_BURST_64W

       @return Always return E_OK*/
ER jpeg_set_burstlen(JPEG_BS_DMA_BURST bslen)
{
    T_JPG_BS    jpg_bs;

	jpg_bs.reg = JPEG_GETREG(JPG_BS_OFS);

	if (bslen == JPEG_BS_DMA_BURST_64W) {
		jpg_bs.bit.bs_dma_burst = 1;
		JPGDBG_IND("BSDMA 64W  ============================    \r\n");
	} else {
	    jpg_bs.bit.bs_dma_burst = 0;
	}

	JPEG_SETREG(JPG_BS_OFS, jpg_bs.reg);

	return E_OK;
}

/**
    Get JPEG "Bit Rate Control" information

    Get JPEG "Bit Rate Control" information.

    @param[out] pBRCInfo    Bit Rate Control information

    @return void
*/
void jpeg_get_brcinfo(PJPEG_BRC_INFO p_brcinfo)
{
	p_brcinfo->brcinfo1 = JPEG_GETREG(JPG_RHO_1_8_OFS);
	p_brcinfo->brcinfo2 = JPEG_GETREG(JPG_RHO_1_4_OFS);
	p_brcinfo->brcinfo3 = JPEG_GETREG(JPG_RHO_1_2_OFS);
	p_brcinfo->brcinfo4 = JPEG_GETREG(JPG_RHO_1_OFS);
	p_brcinfo->brcinfo5 = JPEG_GETREG(JPG_RHO_2_OFS);
	p_brcinfo->brcinfo6 = JPEG_GETREG(JPG_RHO_4_OFS);
	p_brcinfo->brcinfo7 = JPEG_GETREG(JPG_RHO_8_OFS);
}

UINT32 jpeg_get_DCQvalue(void)
{
	T_JPG_LUT   jpg_lut;
	UINT32      reg;

    // Enable table access
	jpg_lut.reg = JPEG_GETREG(JPG_LUT_OFS);
	jpg_lut.bit.tbl_mod     = JPEG_TBL_ACCESS_EN;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Select Q table, table index = 0, table index auto incremental
	jpg_lut.bit.tbl_sel     = JPEG_TABSEL_QUANT;
	jpg_lut.bit.tbl_adr     = 0;
	jpg_lut.bit.tbl_autoi   = JPEG_TBL_AUTOI_EN;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	reg = JPEG_GETREG(JPG_LUT_DATA_OFS);
	//printk("[JPG_LUT_DATA_OFS] >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 0x%x\r\n", (unsigned int)(JPEG_GETREG(JPG_LUT_DATA_OFS)));
	// De-select table
	jpg_lut.bit.tbl_sel     = JPEG_TABSEL_NONE;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	// Disable table access
	jpg_lut.bit.tbl_mod     = JPEG_TBL_ACCESS_DIS;
	JPEG_SETREG(JPG_LUT_OFS, jpg_lut.reg);

	return reg;
}

UINT32 g_uiVBR_Q[KDRV_VDOENC_ID_MAX+1];
UINT32 g_uiDC_Q[KDRV_VDOENC_ID_MAX+1][FRAME_WINDOW];
UINT32 g_uiDC_Qavg[KDRV_VDOENC_ID_MAX+1];

void jpeg_frame_window_count(UINT32 id, UINT32 size)
{
    UINT32 i;

	g_uiFRAME_WINDOW_size[id] = 0;

	g_uiFrameSize[id][g_CurrentFrameID[id]] = size;
	g_uiDC_Q[id][g_CurrentFrameID[id]] = jpeg_get_DCQvalue()&0xFF;

	for(i=0;i<FRAME_WINDOW;i++)
	{
	    g_uiFRAME_WINDOW_size[id]+= g_uiFrameSize[id][i];
		g_uiDC_Qavg[id]+= g_uiDC_Q[id][i];
	}
	if(g_uiDC_Qavg[id]) // if g_uiDC_Qavg[id] not zero
	    g_uiDC_Qavg[id] =g_uiDC_Qavg[id]/FRAME_WINDOW;

#if JPEG_DEBUG
	printk("[g_uiFRAME_WINDOW_size] ==================> 0x%x\r\n", (unsigned int)(g_uiFRAME_WINDOW_size[id]));
	printk("[g_uiDC_Qavg] ==================> %d\r\n", (int)(g_uiDC_Qavg[id]));
#endif

	if(g_CurrentFrameID[id] == (FRAME_WINDOW-1))
		g_CurrentFrameID[id] = 0;
	else
		g_CurrentFrameID[id]++;

	g_TotalFrameID[id]++;

	if(g_TotalFrameID[id] == CHECK_TOTALFRAMESIZE_NUM)
	{
		g_VBR_AVGsize[id] = g_uiFRAME_WINDOW_size[id];
#if JPEG_DEBUG
		printk("[g_VBR_AVGsize] ==================> 0x%x\r\n", (unsigned int)(g_VBR_AVGsize[id]));
#endif
	}

}

/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
#ifdef __KERNEL__
//static DEFINE_SPINLOCK(my_lock);

//#define loc_cpu(myflags)   spin_lock_irqsave(&my_lock, myflags)
//#define unl_cpu(myflags)   spin_unlock_irqrestore(&my_lock, myflags)
#else
#if defined(__FREERTOS)
//static  VK_DEFINE_SPINLOCK(my_lock);
//#define loc_cpu(myflags) vk_spin_lock_irqsave(&my_lock, myflags)
//#define unl_cpu(myflags) vk_spin_unlock_irqrestore(&my_lock, myflags)

#else

#endif

#endif

//extern KDRV_JPEG_TRIG_INFO g_jpeg_trig_info;
extern JPGHEAD_DEC_CFG      jpegdec_Cfg;

ER jpeg_add_queue(UINT32 id, JPEG_CODEC_MODE codec_mode, void *p_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data)
{
	KDRV_VDOENC_PARAM *p_enc_param;
	KDRV_VDODEC_PARAM *p_dec_param;
	KDRV_JPEG_TRIG_INFO    *p_trig;
	BOOL                is_block_mode = FALSE;
	UINT32              uiBrcReTryCnt = 0;
	//unsigned long flag;
	JPG_HEADER_ER jpghdr_errcode = JPG_HEADER_ER_OK;
	ER  ret = 0;

	UINT32 spin_flags;
	if (1) {
		//loc_cpu(flag);
		// Enter critical section
		spin_flags = jpeg_platform_spin_lock();

        if (codec_mode == JPEG_CODEC_MODE_DEC) {
	    //KDRV_JPEG_QUEUE_ELEMENT   element;
	    p_dec_param = (KDRV_VDODEC_PARAM *) p_param;

		memset((void *)&jpegdec_Cfg, 0, sizeof(JPGHEAD_DEC_CFG));
		p_dec_param->errorcode = 0;

		jpegdec_Cfg.inbuf = (UINT8 *)p_dec_param->jpeg_hdr_addr;
		DBG_IND("p_dec_param->jpeg_hdr_addr = 0x%x\r\n", (unsigned int)(p_dec_param->jpeg_hdr_addr));

		jpghdr_errcode = jpg_parse_header(&jpegdec_Cfg, DEC_PRIMARY, NULL);

		if (jpghdr_errcode != JPG_HEADER_ER_OK) {
            //printk("Kdrv Dec error \r\n");
			jpeg_platform_spin_unlock(spin_flags);
			return -1;
		}
		//else
			//printk("Kdrv Dec OK \r\n");
        }else {
			p_enc_param = (KDRV_VDOENC_PARAM *)p_param;

			//Keep channel id
			p_enc_param->temproal_id = id;
		}

		if (p_cb_func == NULL || p_cb_func->callback == NULL) {
			//wait_flg_ptn = kdrv_rpc_get_free_bit_ptn_p();
			//if (wait_flg_ptn == 0) {
			//	DBG_ERR("No free bit_ptn\r\n");
			//	unl_cpu();
			//	return KDRV_RPC_ER_SYS;
			//}
			is_block_mode = TRUE;
		}
		//p_trig = &g_jpeg_trig_info;
		p_trig = kdrv_jpeg_get_triginfo_by_coreid();

		// check if need add cmd to queue
		if (p_trig->is_busy == TRUE) {
			KDRV_JPEG_QUEUE_ELEMENT   element;

			element.p_cb_func   =  p_cb_func;
			//printk("jpeg_add_queue - Busy now\r\n");
			if (codec_mode == JPEG_CODEC_MODE_ENC) {
				element.jpeg_enc_param = (KDRV_VDOENC_PARAM *) p_param;
				element.jpeg_dec_param = NULL;
				element.jpeg_mode = JPEG_CODEC_MODE_ENC;
			} else {
				element.jpeg_enc_param = NULL;
				element.jpeg_dec_param = (KDRV_VDODEC_PARAM *) p_param;
				element.jpeg_mode = JPEG_CODEC_MODE_DEC;
			}

			//element.p_user_data =  p_user_data;
			//element.flg_ptn     =  wait_flg_ptn;
			if (kdrv_jpeg_queue_add_p(p_trig->p_queue, &element) == -3/*KDRV_RPC_ER_QUEUE_FULL*/) {
				//unl_cpu(flag);
				jpeg_platform_spin_unlock(spin_flags);
				return -3/*KDRV_RPC_ER_QUEUE_FULL*/;
			}
			//unl_cpu(flag);
			jpeg_platform_spin_unlock(spin_flags);
		} else {
#ifdef __KERNEL__
			p_trig->is_busy     = TRUE;
#else
			p_trig->is_busy     = FALSE;
#endif
			p_trig->cb          = p_cb_func;
			//p_trig->user_data   = p_user_data;
			//p_trig->flg_ptn     = wait_flg_ptn;
			//unl_cpu(flag);
			jpeg_platform_spin_unlock(spin_flags);
			//printk("jpeg_add_queue - Not Busy, trigger \r\n");
BRC_REENC:
			p_trig->tri_func(codec_mode, p_param, p_cb_func);

			//p_trig->cc_send_cmd((PCC_CMD)p_rpc_param->cmd_buf);
		}

		if (is_block_mode) {
			//printk("wait cmd finish\r\n");

#if 0
			if (codec_mode == JPEG_CODEC_MODE_DEC) {

				p_dec_param = (KDRV_VDODEC_PARAM *) p_param;

				if(p_dec_param->errorcode != 0) {
					p_trig->is_busy = FALSE;
					return -1;
				}
			}
#endif

			//wai_flg(&flg_ptn, g_rpc_flgid, wait_flg_ptn, TWF_ORW|TWF_CLR);
			//jpeg_platform_wait_flg();
			//kdrv_rpc_rel_bit_ptn_p(wait_flg_ptn);
			//printk("cmd finish\r\n");

			while (1) {
				UINT32  uiIntSts;

                jpeg_platform_wait_flg();
				uiIntSts = jpeg_get_status();

				g_reEnc[id] = 0;

				if (uiIntSts & JPEG_INT_FRAMEEND) {
				    //printk("JPEG_INT_FRAMEEND\r\n");
					if (codec_mode == JPEG_CODEC_MODE_ENC) {
						p_enc_param = (KDRV_VDOENC_PARAM *)p_param;

						p_enc_param->bs_size_1 = jpeg_get_bssize() + p_enc_param->svc_hdr_size;
						jpeg_flush_bsbuf(p_enc_param->bs_addr_1, p_enc_param->bs_size_1);

						if (g_uiBrcTargetSize[id])
						{
#if JPEG_BRC2
							p_enc_param->base_qp = g_uiBrcStdQTableQuality[id];//g_uiBrcQF[id];
#else
							p_enc_param->base_qp = g_uiBrcQF[id];
#endif
						}

						//Count frame sequence size
						if(p_enc_param->vbr_mode)
							jpeg_frame_window_count(id, p_enc_param->bs_size_1);
#if JPEG_DEBUG
						printk("p_enc_param->bs_size_1 = 0x%x\r\n", (unsigned int)(p_enc_param->bs_size_1));
#endif
					}
					break;
				} else if (uiIntSts & JPEG_INT_BUFEND) {
					//printk("JPEG_INT_BUFEND\r\n");
					ret = -1;
					break;
				} else {
					//printk(" [jpeg_get_status] uiIntSts = 0x%x\r\n", (unsigned int)(uiIntSts));
					ret = -1;
					break;
				}
			}

			if (codec_mode == JPEG_CODEC_MODE_ENC) {
				if (g_uiBrcTargetSize[id])
				{
					//UINT32  uiNewQValue, uiTargetQF;
					UINT32 bs_size;

					p_enc_param = (KDRV_VDOENC_PARAM *)p_param;
                    //p_enc_param->vbr_mode = 1;  //for test
					bs_size = jpeg_get_bssize()+p_enc_param->svc_hdr_size;

					if(p_enc_param->vbr_mode)
					{
#if JPEG_DEBUG
						printk(" )))))))))))))))))))))))))))))))))))))))))))))  g_TotalFrameID = %d \r\n", ( int)g_TotalFrameID[id]);
#endif
					    if(g_uiVBR[id] == VBR_NORAMAL)
				    	{
							//if(bs_size >= g_uiBrcTargetSize[id])
							if((g_TotalFrameID[id]>30)&&(g_uiFRAME_WINDOW_size[id] >= g_uiBrcCurrTarget_Rate[id]))
							{
#if JPEG_DEBUG
								//printk("VBR_TO_CBR [%d] TARGET SIZE = 0x%x , current size = 0x%x\r\n",(int)id, (unsigned int)g_uiBrcTargetSize[id], (unsigned int)bs_size);
								printk("VBR_TO_CBR [%d] TARGET SIZE = 0x%x , g_uiFRAME_WINDOW_size = 0x%x\r\n",(int)id, (unsigned int)g_uiBrcCurrTarget_Rate[id], (unsigned int)g_uiFRAME_WINDOW_size[id]);
#endif
								g_uiVBR[id] = VBR_TO_CBR;
	                			g_uiVBRmodechange[id] = 1;
								g_TotalFrameID[id] = 0;
							}
					    } else {  // g_uiVBR[id] == VBR_TO_CBR

							if (p_enc_param->in_fmt == KDRV_JPEGYUV_FORMAT_420)
								//jpg_get_brc_qf(JPG_FMT_YUV420, &uiTargetQF, bs_size);
								jpg_get_brc_qf(id,JPG_FMT_YUV420, &g_uiBrcQF[id], bs_size);
							else
								//jpg_get_brc_qf(JPG_FMT_YUV211, &uiTargetQF, bs_size);
								jpg_get_brc_qf(id,JPG_FMT_YUV211, &g_uiBrcQF[id], bs_size);


							//if(((g_uiBrcStdQTableQuality[id] >=(g_uiVBR_QUALITY[id])) &&(g_uiBrcQF[id] <= DEFAULT_BRC_VBR_QUALITY)) ||
							//	((g_uiBrcStdQTableQuality[id] <=(g_uiVBR_QUALITY[id])) &&(g_uiFRAME_WINDOW_size[id] >= (g_VBR_AVGsize[id]+0x10000))&&(g_TotalFrameID[id]>100)))
#if JPEG_BRC2
							if((g_TotalFrameID[id]>30)&&(g_uiBrcStdQTableQuality[id] > g_uiVBR_QUALITY[id]))
#else
							if((g_TotalFrameID[id]>30)&&(g_uiDC_Qavg[id] < g_uiVBR_Q[id]))
#endif
							{
#if JPEG_DEBUG
								printk(" CBR TO VBR  g_uiBrcQF = 0x%x \r\n",(unsigned int) g_uiBrcQF[id]);
								printk(" CBR TO VBR   g_uiFRAME_WINDOW_size = 0x%x , g_VBR_AVGsize = 0x%x\r\n", (unsigned int)g_uiFRAME_WINDOW_size[id], (unsigned int)g_VBR_AVGsize[id]);
								printk(" CBR TO VBR   g_TotalFrameID = 0x%x , g_uiDC_Qavg[id] = %d\r\n", (unsigned int)g_TotalFrameID[id], ( int)g_uiDC_Qavg[id]);
#endif
								g_uiVBR[id] = VBR_NORAMAL;
	                			g_uiVBRmodechange[id] = 1;
								g_TotalFrameID[id] = 0;
							}

					    }
					}
					else
					{
						if (!p_enc_param->quality){
							//printk(" g_uiBrcQF[id] = (%d)!\r\n", (int)g_uiBrcQF[id]);
							//printk(" g_uiBrcStdQTableQuality[id] = (%d)!\r\n", (int)g_uiBrcStdQTableQuality[id]);
							if ((g_uiBrcQF[id] == JPGBRC_QUALITY_MIN &&(g_uiBrcStdQTableQuality[id] == 100) && bs_size < g_brcParam[id].lbound_byte) ||
								(g_uiBrcQF[id] == JPGBRC_QUALITY_MAX &&(g_uiBrcStdQTableQuality[id] == 5)&& bs_size > g_brcParam[id].ubound_byte)) {
								//printk("Reach BRC limitation!\r\n");

							} else if (bs_size > g_brcParam[id].ubound_byte|| bs_size < g_brcParam[id].lbound_byte) {
								if (uiBrcReTryCnt < IMAGE_RETRY_COUNTS) {
									uiBrcReTryCnt++;
									g_reEnc[id] = 1; //enable re-encode
									//printk("Retry again!\r\n");
									//printk(" size = (0x%x)!\r\n", (unsigned int)bs_size);

									if (p_enc_param->in_fmt == KDRV_JPEGYUV_FORMAT_420){
										jpg_get_brc_qf(id,JPG_FMT_YUV420, &g_uiBrcQF[id], bs_size);
									}
	    							else if (p_enc_param->in_fmt == KDRV_JPEGYUV_FORMAT_422){
										jpg_get_brc_qf(id,JPG_FMT_YUV211, &g_uiBrcQF[id], bs_size);
									} else {
										//printk(" Reach BRC re-try count limitation!\r\n");

									}
									//recover origin bs size to do re encode
									p_enc_param->bs_size_1 = p_enc_param->bs_size_2;

									goto BRC_REENC;
								}
								else
								{
									//printk("Over retry count!\r\n");
								}
							}
							//printk(" size = (0x%x)!\r\n", (unsigned int)bs_size);
						}
						else
						{
							//p_enc_param->base_qp = g_uiBrcQF;
							if (p_enc_param->in_fmt == KDRV_JPEGYUV_FORMAT_420)
								//jpg_get_brc_qf(JPG_FMT_YUV420, &uiTargetQF, bs_size);
								jpg_get_brc_qf(id,JPG_FMT_YUV420, &g_uiBrcQF[id], bs_size);
							else
								//jpg_get_brc_qf(JPG_FMT_YUV211, &uiTargetQF, bs_size);
								jpg_get_brc_qf(id,JPG_FMT_YUV211, &g_uiBrcQF[id], bs_size);
						}
					}
#if 0
					if(uiTargetQF < g_uiBrcQF)
					{
						uiNewQValue = g_uiBrcQF - (g_uiBrcQF - uiTargetQF)*JPEG_BRC_WEIGHTING/JPEG_MAX_BRC_WEIGHTING;
					}
					else
					{
						//#NT#Make weighting mechanism valid only when QF is decaying.
						#if 1//(BRC_WEIGHTING_VALID_IN_DECAY_ONLY)
						uiNewQValue = uiTargetQF;
						#else
						uiNewQValue = m_uiBrcQF + (uiTargetQF - m_uiBrcQF )*JPEG_BRC_WEIGHTING/JPEG_MAX_BRC_WEIGHTING;
						#endif
					}
					g_uiBrcQF = uiNewQValue;
#endif
				    //printk("ID[%d]  BaseQP = %d , [g_uiBrcQF] = %d\r\n",(int)id, (int)p_enc_param->base_qp, (int)g_uiBrcQF[id]);

				}

				jpeg_set_endencode();
				nvt_dbg(INFO, "jpeg_add_queue - encode done\n");
			} else {
				jpeg_set_enddecode();
				nvt_dbg(INFO, "jpeg_add_queue - decode done\n");

				p_dec_param = (KDRV_VDODEC_PARAM *) p_param;

				if(p_dec_param->errorcode != 0) {
					ret = -1;
				}
			}
		}
	}

	return ret;
}

#ifdef __KERNEL__
EXPORT_SYMBOL(jpeg_get_config);
EXPORT_SYMBOL(jpeg_open);
EXPORT_SYMBOL(jpeg_close);
EXPORT_SYMBOL(jpeg_get_bssize);
EXPORT_SYMBOL(jpeg_set_config);
EXPORT_SYMBOL(jpeg_get_brcinfo);
EXPORT_SYMBOL(jpeg_add_queue);
#endif

//@}

// Verification code
#ifdef __KERNEL__
// TODO

/*
    Get BS checksum

    Get BS checksum

    @return BS checksum (24 bits)
*/
UINT32 jpegtest_getbs_checksum(void)
{
	return JPEG_GETREG(JPG_CHECKSUM_BS_OFS);
}

/*
    Get YUV checksum

    Get YUV checksum

    @return YUV checksum (32 bits)
*/
UINT32 jpegtest_getyuv_checksum(void)
{
	return JPEG_GETREG(JPG_CHECKSUM_YUV_OFS);
}

EXPORT_SYMBOL(jpegtest_getbs_checksum);
EXPORT_SYMBOL(jpegtest_getyuv_checksum);

#else

#if defined(_EMULATION_)

static JPEG_REG_DEFAULT jpeg_reg_default[] = {
	{ 0x00,     JPEG_CTRL_REG_DEFAULT,           "Control"      },
	{ 0x04,     JPEG_CONF_REG_DEFAULT,           "Config"       },
	{ 0x0C,     JPEG_RSTR_REG_DEFAULT,           "Restart"      },
	{ 0x10,     JPEG_LUTCTRL_REG_DEFAULT,        "LUTControl"   },
	{ 0x14,     JPEG_LUTDATA_REG_DEFAULT,        "LUTData"      },
	{ 0x18,     JPEG_INTCTRL_REG_DEFAULT,        "IntCtrl"      },
	{ 0x1C,     JPEG_INTSTS_REG_DEFAULT,         "IntSts"       },
	{ 0x20,     JPEG_IMGCTRL_REG_DEFAULT,        "ImgCtrl"      },
	{ 0x24,     JPEG_SLICECTRL_REG_DEFAULT,      "SliceCtrl"    },
	{ 0x28,     JPEG_IMGSIZE_REG_DEFAULT,        "ImgSize"      },
	{ 0x2C,     JPEG_MCUCNT_REG_DEFAULT,         "MCU_CNT"      },
	{ 0x3C,     JPEG_BSCTRL_REG_DEFAULT,         "BSCtrl"       },
	{ 0x40,     JPEG_BSDMA_ADDR_REG_DEFAULT,     "BSDMAAddr"    },
	{ 0x44,     JPEG_BSDMA_SIZE_REG_DEFAULT,     "BSDMASize"    },
	{ 0x48,     JPEG_BSDMA_CURADDR_REG_DEFAULT,  "BSDMACurAddr" },
	{ 0x4C,     JPEG_BSDMA_LEN_REG_DEFAULT,      "BSDMALen"     },
	{ 0x54,     JPEG_IMGY_ADDR_REG_DEFAULT,      "ImgYAddr"     },
	{ 0x58,     JPEG_IMGUV_ADDR_REG_DEFAULT,     "ImgUVAddr"    },
	{ 0x60,     JPEG_IMGY_LOFS_REG_DEFAULT,      "ImgYLofs"     },
	{ 0x64,     JPEG_IMGUV_LOFS_REG_DEFAULT,     "ImgUVLofs"    },
	{ 0x6C,     JPEG_DCY_ADDR_REG_DEFAULT,       "DCYAddr"      },
	{ 0x70,     JPEG_DCUV_ADDR_REG_DEFAULT,      "DCUVAddr"     },
	{ 0x78,     JPEG_DCY_LOFS_REG_DEFAULT,       "DCYLofs"      },
	{ 0x7C,     JPEG_DCUV_LOFS_REG_DEFAULT,      "DCUVLofs"     },
	{ 0x84,     JPEG_CROPXY_REG_DEFAULT,         "CropXY"       },
	{ 0x88,     JPEG_CROPSIZE_REG_DEFAULT,       "CropSize"     },
	{ 0x8C,     JPEG_RHO18_REG_DEFAULT,          "Rho1/8"       },
	{ 0x90,     JPEG_RHO14_REG_DEFAULT,          "Rho1/4"       },
	{ 0x94,     JPEG_RHO12_REG_DEFAULT,          "Rho1/2"       },
	{ 0x98,     JPEG_RHO_REG_DEFAULT,            "Rho"          },
	{ 0x9C,     JPEG_RHO2_REG_DEFAULT,           "Rho2"         },
	{ 0xA0,     JPEG_RHO4_REG_DEFAULT,           "Rho4"         },
	{ 0xA4,     JPEG_RHO8_REG_DEFAULT,           "Rho8"         },
	{ 0xA8,     JPEG_CYCCNT_REG_DEFAULT,         "CycleCnt"     },
	{ 0xAC,     JPEG_ERRSTS_REG_DEFAULT,         "ErrorSts"     },
	{ 0xB0,     JPEG_IMGDMA_BLK_REG_DEFAULT,     "ImgDMABlock"  },
	{ 0xB4,     JPEG_BSSUM_REG_DEFAULT,          "BSChecksum"   },
	{ 0xB8,     JPEG_YUVSUM_REG_DEFAULT,         "YUVChecksum"  },
};

UINT32  jpegtest_getbs_checksum(void);
UINT32  jpegtest_getyuv_checksum(void);
BOOL    jpegtest_comp_regdef(void);

/*
    Get BS checksum

    Get BS checksum

    @return BS checksum (24 bits)
*/
UINT32 jpegtest_getbs_checksum(void)
{
	return JPEG_GETREG(JPG_CHECKSUM_BS_OFS);
}

/*
    Get YUV checksum

    Get YUV checksum

    @return YUV checksum (32 bits)
*/
UINT32 jpegtest_getyuv_checksum(void)
{
	return JPEG_GETREG(JPG_CHECKSUM_YUV_OFS);
}

/*
    (Verification code) Compare JPEG register default value

    (Verification code) Compare JPEG register default value.

    @return Compare status
	- @b FALSE  : Register default value is incorrect.
	- @b TRUE   : Register default value is correct.
*/
BOOL jpegtest_comp_regdef(void)
{
	UINT32  index;
	BOOL    ret = TRUE;

	for (index = 0; index < (sizeof(jpeg_reg_default) / sizeof(JPEG_REG_DEFAULT)); index++) {
		if (JPEG_GETREG(jpeg_reg_default[index].offset) != jpeg_reg_default[index].value) {
			JPGDBG_ERR("%s Register (0x%.2X) default value 0x%.8X isn't 0x%.8X\r\n",
					jpeg_reg_default[index].pname,
					jpeg_reg_default[index].offset,
					JPEG_GETREG(jpeg_reg_default[index].offset),
					jpeg_reg_default[index].value);
			ret = FALSE;
		}
	}

	return ret;
}

#endif


#endif
