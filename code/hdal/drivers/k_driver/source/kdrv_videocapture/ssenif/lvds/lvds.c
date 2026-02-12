/*
   LVDS/HiSPi module driver

    @file       lvds.c
    @ingroup    mIDrvIO_LVDS
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef __KERNEL__
#include <kwrap/util.h>
#include <kwrap/type.h>
#include <kwrap/spinlock.h>
#include "../lvds.h"
#include "lvds_int.h"

#define __MODULE__    rtos_lvds
#include <kwrap/debug.h>
unsigned int rtos_lvds_debug_level = NVT_DBG_WRN;

static ID SEMID_LVDS;
static ID FLG_ID_LVDS;
#else
#include <kwrap/util.h>
#include <kwrap/spinlock.h>
#include <mach/rcw_macro.h>
#include <kwrap/type.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include "../lvds.h"
#include "lvds_int.h"
#endif

static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

//#define LVDS_INSTALLCOMMAND

// Module related definition -begin
#define LVDS_GETREG(ofs)            LVDS_R_REGISTER((ofs))
#define LVDS_SETREG(ofs, value)     LVDS_W_REGISTER((ofs), (value))

#ifndef __KERNEL__
#define MODULE_FLAG_ID              FLG_ID_LVDS
#define MODULE_FLAG_PTN             FLGPTN_LVDS
#define MODULE_CLK_ID				MIPI_LVDS_CLK
#define MODULE_DRV_INT				INT_ID_LVDS
#else
#define MODULE_CLK_ID				0
#endif
#define MODULE_SEM_ID               SEMID_LVDS
//#define MODULE_RST_ID				MIPI_LVDS_RSTN
#define MODULE_SENPHY               SENPHY_SEL_MIPILVDS

#define LVDS_PHASE_INV
#define LVDS_ENABLE_INT             lvds_set_int_enable
#define LVDS_DISABLE_INT            lvds_set_int_disable
#define LVDS_SET_ENABLE             lvds_set_enable
#define LVDS_GET_ENABLE             lvds_get_enable
#define LVDS_WAIT_INTERR            lvds_wait_interrupt
#define LVDS_FIFO_RESET             lvds_set_fifo_reset
#define LVDS_OUT_REMAPORDER         lvds_out_remap_order
#define LVDS_CHK_REG                lvds_chk_register
#define LVDS_LOCK                   lvds_lock
#define LVDS_UNLOCK                 lvds_unlock
#define LVDS_DUMPINFO               lvds_dump_debug_info
#ifndef __KERNEL__
#define LVDS_TIMEOUTCALLBACK        lvds_timeout_timer_callback
#endif

#define LVDS_CK1_EN					DISABLE
#define LVDS_TWOCLK_SUPPORT         DISABLE
#define LVDS_CLK0_SEL               SENPHY_CLKMAP_CK0
#define LVDS_CLK0_ALT_SEL           SENPHY_CLKMAP_CK2
#define LVDS_CLK1_SEL               SENPHY_CLKMAP_OFF
#define LVDS_CLK1_ALT_SEL           SENPHY_CLKMAP_OFF

#define LVDS_MAX_CLAN_NUMBER        LVDS_CLKLANE_1
#define LVDS_MAX_DLAN_NUMBER        LVDS_DATLANE_4
#define LVDS_VALID_MASK             0x0F
#define LVDS_VALID_OFFSET           0
#define LVDS_DLANE_PATCH            0

#define LVDS_API_DEFINE(name)        (lvds##_##name)

//REGDEF_SETIOBASE(IOADDR_LVDS_REG_BASE);
// Module related definition -end


/**
    @addtogroup mIDrvIO_LVDS
*/
//@{

static BOOL 			LVDS_DESKEW_EN = DISABLE;

static BOOL             lvds_opened			= FALSE;
static BOOL             dropixel_modified	= FALSE;
static UINT32           valid_width;
static UINT32           lvds_out_order[2]  = {0x76543210, 0x00000098};
static UINT32           lvds_vd_cnt = 0;
static UINT32           lvds_error_count;
static UINT32           lvds_interrupt_status;
static BOOL             lvds_image_duplicate  = FALSE;
static BOOL             lvds_clk_switch     = FALSE;
#ifndef __KERNEL__
//static SWTIMER_ID		lvds_timeout_timer_id;
#endif
static UINT32           lvds_timeout_period;
static UINT32           lvds_force_enable;


#ifdef __KERNEL__
void LVDS_API_DEFINE(isr)(void);
static struct completion lvds_completion;
static SEM_HANDLE SEMID_LVDS;
void lvds_create_resource(void)
//void LVDS_API_DEFINE(create_resource)(void)
{
	SEM_CREATE(SEMID_LVDS, 1);
	init_completion(&lvds_completion);
}
void lvds_release_resource(void)
//void LVDS_API_DEFINE(release_resource)(void)
{
	SEM_DESTROY(SEMID_LVDS);
}
#endif

#if 1

//
//  Analog Related internal APIs
//
static void LVDS_API_DEFINE(set_int_enable)(LVDS_INTERRUPT interrupt);
static void LVDS_API_DEFINE(set_int_disable)(LVDS_INTERRUPT interrupt);
/*
    LVDS Digital FIFO Reset
*/
static void lvds_set_fifo_reset(void)
//static void LVDS_API_DEFINE(set_fifo_reset)(void)
{
	T_LVDS_CTRL0_REG reg_ctrl0;
	unsigned long      flags;

	LVDS_DISABLE_INT(LVDS_DEFAULT_INT);

	loc_cpu(flags);
	// Reset LVDS Digital Control FIFO
	reg_ctrl0.reg = LVDS_GETREG(LVDS_CTRL0_REG_OFS);
	reg_ctrl0.bit.FIFO_RST = 1;
	LVDS_SETREG(LVDS_CTRL0_REG_OFS, reg_ctrl0.reg);
	unl_cpu(flags);

	//dummy read
	reg_ctrl0.reg = LVDS_GETREG(LVDS_CTRL0_REG_OFS);
	reg_ctrl0.reg = LVDS_GETREG(LVDS_CTRL0_REG_OFS);
	reg_ctrl0.reg = LVDS_GETREG(LVDS_CTRL0_REG_OFS);

	// Clear Reset
	reg_ctrl0.bit.FIFO_RST = 0;
	LVDS_SETREG(LVDS_CTRL0_REG_OFS, reg_ctrl0.reg);

	LVDS_ENABLE_INT(LVDS_DEFAULT_INT);
}

void lvds_trigger_relock(void)
//static void LVDS_API_DEFINE(trigger_relock)(void)
{
	T_LVDS_CTRL0_REG reg_ctrl0;

	reg_ctrl0.reg = LVDS_GETREG(LVDS_CTRL0_REG_OFS);
	reg_ctrl0.bit.LOCK_CLEAR = 1;
	LVDS_SETREG(LVDS_CTRL0_REG_OFS, reg_ctrl0.reg);
	LVDS_DELAY_US_POLL(20);
	reg_ctrl0.bit.LOCK_CLEAR = 0;
	LVDS_SETREG(LVDS_CTRL0_REG_OFS, reg_ctrl0.reg);
}

static void lvds_out_remap_order(void)
//static void LVDS_API_DEFINE(out_remap_order)(void)
{
	UINT32              i, j, lanevalid = 0, new_outorder;

	new_outorder = lvds_out_order[0];
	for (i = 0; i < 8; i++) {
		if (lanevalid & (0x1 << ((new_outorder >> (i << 2)) & 0xF))) {
			for (j = 0; j < 8; j++) {
				if (!(lanevalid & (0x1 << j))) {
					break;
				}
			}
			new_outorder &= ~(0xF << (i << 2));
			new_outorder |= (j << (i << 2));

			lanevalid |= (0x1 << ((new_outorder >> (i << 2)) & 0xF));
		} else {
			lanevalid |= (0x1 << ((new_outorder >> (i << 2)) & 0xF));
		}
	}
	lvds_out_order[0] = new_outorder;
}

#endif

#ifdef __KERNEL__
UINT32 lvds_isr_check(void)
//UINT32 LVDS_API_DEFINE(isr_check)(void)
{
	T_LVDS_INTSTS_REG   reg_sts;

	reg_sts.reg =  LVDS_GETREG(LVDS_INTSTS_REG_OFS);
	reg_sts.reg &= LVDS_GETREG(LVDS_INTEN_REG_OFS);
	return reg_sts.reg;
}
#endif

/*
    LVDS/HiSPi ISR

    This function is called by the IST.
*/
#ifndef __KERNEL__
irqreturn_t lvds_isr(int irq, void *devid)
//irqreturn_t LVDS_API_DEFINE(isr)(int irq, void *devid)
#else
void lvds_isr(void)
//void LVDS_API_DEFINE(isr)(void)
#endif
{
	T_LVDS_INTSTS_REG   reg_sts;

	// Get the interrupt status and AND with the enable bits
	// Because we only handle the enabled interrupt status bits only.
	reg_sts.reg =  LVDS_GETREG(LVDS_INTSTS_REG_OFS);
	reg_sts.reg &= LVDS_GETREG(LVDS_INTEN_REG_OFS);

	// LVDS and CSI use share interrupt source.
	// If not the lvds issued interrupt, we should just skip it.
	if (reg_sts.reg) {
		// Clear interrupt status
		LVDS_SETREG(LVDS_INTSTS_REG_OFS, reg_sts.reg);

		// Log the interrupt status events.
		// This is used in lvds_wait_interrupt() to differentiate the interrupt events.
		lvds_interrupt_status |= reg_sts.reg;

		if (reg_sts.bit.VD_STS) {
			lvds_vd_cnt++;
		}

		if ((reg_sts.bit.PIXCNT_ERR_STS) || (reg_sts.bit.FIFO_ERR_STS) || (reg_sts.bit.FIFO_OV_STS) || (reg_sts.bit.FIFO_OV2_STS) || (reg_sts.bit.PIXCNT_ERR2_STS)) {
			// After LVDS error interrupt events occur,
			// we should clear FIFO first.
			if (reg_sts.bit.FIFO_ERR_STS) {
				LVDS_FIFO_RESET();
			}

			// In current driver design, we are not always report the error event on console,
			// we increment a counter and report the error event if the error repeated 1000 times.
			// this is because during the sensor mode change, we may normally get the error event
			// during the transition period, this temporily error message may let the user feel uncomfortable,
			// but this is not due to the real error situations.
			lvds_error_count++;

			if (lvds_error_count >= 1000) {
				lvds_error_count = 0;
				if (reg_sts.bit.PIXCNT_ERR_STS) {
					DBG_ERR("Rx Width Not Enough\r\n");
				}
				if (reg_sts.bit.PIXCNT_ERR2_STS) {
					DBG_ERR("Rx2 Width Not Enough\r\n");
				}
				if (reg_sts.bit.FIFO_ERR_STS) {
					DBG_ERR("FIFO ERR\r\n");
				}



				if (reg_sts.bit.FIFO_OV_STS) {
					DBG_ERR("FIFO1 OV\r\n");
				}
				if (reg_sts.bit.FIFO_OV2_STS) {
					DBG_ERR("FIFO2 OV\r\n");
				}

				LVDS_FIFO_RESET();
			}
		}


		// Output debug messages
#if LVDS_DEBUG
		if (reg_sts.bit.VD_STS) {
			// Debug Msg: Receiving VD SYNC code from Sensor
			DBG_DUMP("VD\r\n");
		}
		if (reg_sts.bit.VD2_STS) {
			// Debug Msg: Receiving VD SYNC code from Sensor
			DBG_DUMP("VD2\r\n");
		}
		if (reg_sts.bit.VD3_STS) {
			// Debug Msg: Receiving VD SYNC code from Sensor
			DBG_DUMP("VD3\r\n");
		}
		if (reg_sts.bit.VD4_STS) {
			// Debug Msg: Receiving VD SYNC code from Sensor
			DBG_DUMP("VD4\r\n");
		}
		if (reg_sts.bit.HD_STS) {
			// Debug Msg: Receiving HD SYNC code from Sensor
			DBG_DUMP("HD\r\n");
		}
		if (reg_sts.bit.FRMEND_STS) {
			// Debug Msg: Receiving specified valid frame lines from Sensor
			DBG_DUMP("FRMEND\r\n");
		}
		if (reg_sts.bit.FRMEND2_STS) {
			// Debug Msg: Receiving specified valid frame lines from Sensor
			DBG_DUMP("FRMEND2\r\n");
		}
		if (reg_sts.bit.FRMEND3_STS) {
			// Debug Msg: Receiving specified valid frame lines from Sensor
			DBG_DUMP("FRMEND3\r\n");
		}
		if (reg_sts.bit.FRMEND4_STS) {
			// Debug Msg: Receiving specified valid frame lines from Sensor
			DBG_DUMP("FRMEND4\r\n");
		}
		if (reg_sts.bit.FIFO_OV_STS) {
			// Debug Msg: Receiving specified valid frame lines from Sensor
			DBG_DUMP("FIFO1 OV\r\n");
		}
		if (reg_sts.bit.FIFO_OV2_STS) {
			// Debug Msg: Receiving specified valid frame lines from Sensor
			DBG_DUMP("FIFO2 OV\r\n");
		}
		if (reg_sts.bit.FIFO_ERR_STS) {
			// Debug Msg: FIFO Pointer mis-match at VD
			DBG_DUMP("FIFO_ERR\r\n");
		}
#endif

#ifndef __KERNEL__
		iset_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);
#else
		complete(&lvds_completion);
#endif
	}

#ifndef __KERNEL__
		return IRQ_HANDLED;
#endif
}

/*
    LVDS Interrupt Enable
*/
static void lvds_set_int_enable(LVDS_INTERRUPT interrupt)
//static void LVDS_API_DEFINE(set_int_enable)(LVDS_INTERRUPT interrupt)
{
	T_LVDS_INTEN_REG    reg_intr_en;
	unsigned long      flags;

	// lock cpu before access the interrupt enable register
	loc_cpu(flags);

	// OR the specified interrupt with the interrupt enable register and then write it back.
	reg_intr_en.reg  = LVDS_GETREG(LVDS_INTEN_REG_OFS);
	reg_intr_en.reg |= interrupt;
	LVDS_SETREG(LVDS_INTEN_REG_OFS, reg_intr_en.reg);

	// unlock cpu
	unl_cpu(flags);
}

/*
    LVDS Interrupt Disable
*/
static void lvds_set_int_disable(LVDS_INTERRUPT interrupt)
//static void LVDS_API_DEFINE(set_int_disable)(LVDS_INTERRUPT interrupt)
{
	T_LVDS_INTEN_REG    reg_intr_en;
	unsigned long      flags;

	// lock cpu before access the interrupt enable register.
	// This is to prevent the dummy interrupt situation.
	loc_cpu(flags);

	// CLEAR the specified interrupt from the interrupt enable register and then write it back.
	reg_intr_en.reg  = LVDS_GETREG(LVDS_INTEN_REG_OFS);
	reg_intr_en.reg &= ~interrupt;
	LVDS_SETREG(LVDS_INTEN_REG_OFS, reg_intr_en.reg);

	// unlock cpu
	unl_cpu(flags);
}

/*
    LVDS ReSource Lock
*/
static ER lvds_lock(void)
//static ER LVDS_API_DEFINE(lock)(void)
{
	// Wait the semaphore to get the access right of the lvds_wait_interrupt() API.
	#ifdef __KERNEL__
	SEM_WAIT(MODULE_SEM_ID);
	return E_OK;
	#else
	wai_sem(MODULE_SEM_ID);
	return E_OK;
	#endif
}

/*
    LVDS ReSource Un-Lock
*/
static ER lvds_unlock(void)
//static ER LVDS_API_DEFINE(unlock)(void)
{
	// Release the semaphore after interrupt status is waited.
	#ifdef __KERNEL__
	SEM_SIGNAL(MODULE_SEM_ID);
	return E_OK;
	#else
	sig_sem(MODULE_SEM_ID);
	return E_OK;
	#endif
}

#if 0//ndef __KERNEL__
static void lvds_timeout_timer_callback(UINT32 event)
//static ER LVDS_API_DEFINE(timeout_timer_callback)(UINT32 event)
{
	unsigned long      flags;

	loc_cpu(flags);
	lvds_interrupt_status |= LVDS_INTERRUPT_ABORT;
	unl_cpu(flags);

	set_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);
}
#endif

#ifndef __KERNEL__
static BOOL lvds_init_done;
void lvds_init(void)
//ER LVDS_API_DEFINE(init)(void)
{
	vos_flag_create(&MODULE_FLAG_ID, NULL, "FLG_ID_LVDS");
	vos_sem_create(&MODULE_SEM_ID, 1, "SEMID_LVDS");
}

void lvds_uninit(void)
//ER LVDS_API_DEFINE(uninit)(void)
{
	vos_sem_destroy(MODULE_SEM_ID);
	vos_flag_destroy(MODULE_FLAG_ID);
}
#endif

#if 1

/**
    Open LVDS driver

    Open the LVDS driver and initialize the related resource, such as source clock, analog logic power,
    and the OS resource. The user must use this API first for LVDS engine.

    @return
     - @b E_OK: LVDS driver open done and success.
*/
ER lvds_open(void)
//ER LVDS_API_DEFINE(open)(void)
{
	UINT32 i;

	if (lvds_opened) {
		return E_OK;
	}

#ifndef __KERNEL__
	if(!lvds_init_done) {
		lvds_init();
		lvds_init_done = 1;
	}
#endif

	//LVDS_INSTALLCOMMAND

	senphy_init();

	LVDS_ENABLE_CLK(MODULE_CLK_ID);

	// Enable Analog Block
	senphy_set_power(MODULE_SENPHY,				ENABLE);
	#if LVDS_CK1_EN
	senphy_set_config(SENPHY_CONFIG_CK1_EN,		ENABLE);
	#endif
	senphy_set_config(SENPHY_CONFIG_LVDS_MODE,	ENABLE);

	// Enable the error events detection interrupt status.
	LVDS_ENABLE_INT(LVDS_DEFAULT_INT);

	lvds_interrupt_status = 0x0;

#ifndef __KERNEL__
	// clear the interrupt flag
	clr_flg(MODULE_FLAG_ID, MODULE_FLAG_PTN);

	// Enable interrupt
	request_irq(MODULE_DRV_INT, lvds_isr ,IRQF_TRIGGER_HIGH, "lvds1", 0);
#endif

	// Enable LVDS FIFO Auto Reset at VD
	{
		T_LVDS_DEBUG0_REG reg_dbg0;

		reg_dbg0.reg = LVDS_GETREG(LVDS_DEBUG0_REG_OFS);
		reg_dbg0.bit.AUTO_RST = 0;
		LVDS_SETREG(LVDS_DEBUG0_REG_OFS, reg_dbg0.reg);
	}

	// Default set CTRL VD2/3/4 as 0xADAD
	for (i = 0; i < LVDS_DATLANE_ID_MAX; i++) {
		LVDS_SETREG((LVDS_DL0CTRL2_REG_OFS + (i << 4)), 0xADAD0000);
		LVDS_SETREG((LVDS_DL0CTRL3_REG_OFS + (i << 4)), 0xADADADAD);
	}
	LVDS_SETREG(LVDS_CTRL2_REG_OFS, 0xFFF0FFFF);
	LVDS_SETREG(LVDS_LE0_REG_OFS,   0xADADADAD);
	LVDS_SETREG(LVDS_LE1_REG_OFS,   0xADADADAD);
	LVDS_SETREG(LVDS_D0FE_REG_OFS,  0xADADADAD);
	LVDS_SETREG(LVDS_D1FE_REG_OFS,  0xADADADAD);
	LVDS_SETREG(LVDS_D2FE_REG_OFS,  0xADADADAD);
	LVDS_SETREG(LVDS_D3FE_REG_OFS,  0xADADADAD);

	lvds_error_count = 0;
	lvds_opened = TRUE;
	return E_OK;
}

/**
    Close LVDS Driver

    Close the LVDS driver and also close the related resource, such as source clock, analog logic power,
    and the OS resource. The user must also remember to disable the LVDS controller before closing
    the LVDS driver by this API to insure the sensor interface is terminated in normal operation.

    @return
     - @b E_OK: LVDS close done and success.
*/
ER lvds_close(void)
//ER LVDS_API_DEFINE(close)(void)
{
	if (!lvds_opened) {
		return E_OK;
	}

	// Check if the LVDS controller is disabled or not
	if (LVDS_GET_ENABLE()) {
		LVDS_SET_ENABLE(DISABLE);
		DBG_WRN("Not disabled before lvds_close!\r\n");
		LVDS_WAIT_INTERR(LVDS_INTERRUPT_FRMEND);
	}

#ifndef __KERNEL__
	// Disable interrupt
	free_irq(MODULE_DRV_INT, 0);
#endif

	// Disable analog block power
	senphy_set_power(MODULE_SENPHY, DISABLE);

	// Disanle all the LVDS related interrupt enable bits
	LVDS_DISABLE_INT(LVDS_INTERRUPT_VD | LVDS_INTERRUPT_HD | LVDS_INTERRUPT_FRMEND | LVDS_INTERRUPT_PIXCNT_ER | LVDS_INTERRUPT_FIFO_ER | LVDS_INTERRUPT_FIFO1_OV | LVDS_INTERRUPT_PIXCNT_ER2);


	// Disable Clock Source
	LVDS_DISABLE_CLK(MODULE_CLK_ID);

	#if LVDS_CK1_EN
	senphy_set_config(SENPHY_CONFIG_CK1_EN,			DISABLE);
	#endif
	senphy_set_config(SENPHY_CONFIG_LVDS_MODE, DISABLE);

	lvds_opened = FALSE;
	return E_OK;
}

/**
    Check if the LVDS driver is opened or not.

    Check if the LVDS driver is opened or not.

    @return
     - @b TRUE:  LVDS driver is already opened.
     - @b FALSE: LVDS driver has not opened.
*/
BOOL lvds_is_opened(void)
//BOOL LVDS_API_DEFINE(is_opened)(void)
{
	return lvds_opened;
}

#endif

/*
    Before LVDS Engine Enable,
    This api is used to check some register setting
    and remap it to correct settings to match the design limitations.
*/
static BOOL lvds_chk_register(void)
//static BOOL LVDS_API_DEFINE(chk_register)(void)
{
	T_LVDS_CTRL1_REG            reg_ctrl;
	T_LVDS_CTLCOUNT_REG         reg_ctlcount;
	T_LVDS_PIXDROP_CTRL0_REG    reg_dropctl0;
	UINT32                      i, laneno, lanevalid = 0, multiple = 0x0;


	//
	// CHK-1:  Check if the Pixel Out Order Select is Valid or Not.
	//
	LVDS_OUT_REMAPORDER();

	for (i = 0; i < 32; i += 4) {
		multiple |= (0x1 << ((lvds_out_order[0] >> i)&LVDS_PXIELOUT_MSK));
	}
	for (i = 0; i < 8; i += 4) {
		multiple |= (0x1 << ((lvds_out_order[1] >> i)&LVDS_PXIELOUT_MSK));
	}

	// We must make sure that the output swap registers should be set without the same value
	// according to the controller design.
	if (multiple != 0x3FF) {
		DBG_ERR("Data Lanes SWAP Config overlaped!\r\n");
		return TRUE;
	}

	LVDS_SETREG(LVDS_REORDER0_REG_OFS, lvds_out_order[0]);
	//LVDS_SETREG(LVDS_REORDER1_REG_OFS, lvds_out_order[1]);


	//
	//  CHK-2: If Data Lane Number is 1 or 3, ReMap the Valid Width
	//
	reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
	if (reg_ctrl.bit.DAT_LANE_NUM == LVDS_DATLANE_1) {
		// If Data Lane Number is 1, the fifo would fill one more dummy pixels during receiving one pixel.
		// So the setting for pixel count per line should be doubled accoding to the controller design.
		// We must also notice that the 1 lane required SIE clock speed is the same as 2 lanes for the same
		// reason. The dummy pixels would be dropped by the controller's pixel drop function.
		// So the system DMA bandwidth is also the same as 1 lanes, but we need the faster SIE clock frequency.
		reg_ctlcount.reg = LVDS_GETREG(LVDS_CTLCOUNT_REG_OFS);
		reg_ctlcount.bit.PIXCNT_PER_LINE = LVDS_REMAP_WIDTH_1CH(valid_width);
		LVDS_SETREG(LVDS_CTLCOUNT_REG_OFS, reg_ctlcount.reg);
	} else if (reg_ctrl.bit.DAT_LANE_NUM == LVDS_DATLANE_3) {
		// If Data Lane Number is 3, the fifo would fill one more dummy pixels during receiving 3 pixels.
		// So the setting for pixel count per line should add 33% accoding to the controller design.
		// We must also notice that the 3 lane required SIE clock speed is the same as 4 lanes for the same
		// reason. The dummy pixels would be dropped by the controller's pixel drop function.
		// So the system DMA bandwidth is also the same as 3 lanes, but we need the faster SIE clock frequency.
		reg_ctlcount.reg = LVDS_GETREG(LVDS_CTLCOUNT_REG_OFS);
		reg_ctlcount.bit.PIXCNT_PER_LINE = LVDS_REMAP_WIDTH_3CH(valid_width);
		LVDS_SETREG(LVDS_CTLCOUNT_REG_OFS, reg_ctlcount.reg);
	}


	//
	// CHK-3:  Pixel count per line must be multiples of data lanes.
	//         This is the controller design limitations.
	//
	if (reg_ctrl.bit.DAT_LANE_NUM == LVDS_DATLANE_10) {
		multiple = 10;
		laneno   = 10;
	} else if (reg_ctrl.bit.DAT_LANE_NUM == LVDS_DATLANE_8) {
		multiple = 8;
		laneno   = 8;
	} else if (reg_ctrl.bit.DAT_LANE_NUM == LVDS_DATLANE_6) {
		multiple = 6;
		laneno   = 6;
	} else if (reg_ctrl.bit.DAT_LANE_NUM == LVDS_DATLANE_1) {
		multiple = 2;
		laneno   = 1;
	} else if (reg_ctrl.bit.DAT_LANE_NUM == LVDS_DATLANE_3) {
		multiple = 4;
		laneno   = 3;
	} else {
		multiple = (reg_ctrl.bit.DAT_LANE_NUM + 1);
		laneno   = multiple;
	}

	reg_ctlcount.reg = LVDS_GETREG(LVDS_CTLCOUNT_REG_OFS);
	reg_ctlcount.bit.PIXCNT_PER_LINE = (UINT32)((reg_ctlcount.bit.PIXCNT_PER_LINE + multiple - 1) / multiple) * multiple;
	LVDS_SETREG(LVDS_CTLCOUNT_REG_OFS, reg_ctlcount.reg);

	// Set lane valid
	for (i = 0; i < laneno; i++) {
		lanevalid |= (0x1 << ((LVDS_GETREG(LVDS_REORDER0_REG_OFS) >> (i << 2)) & 0xF));
	}
	LVDS_SETREG(LVDS_INPUTSEL_REG_OFS, lanevalid);

#if LVDS_DLANE_PATCH
	if (lanevalid & 0xC) {
		//lanevalid |= LVDS_IN_VALID_D2;
		#if LVDS_DESKEW_EN
		senphy_set_config(SENPHY_CONFIG_CK1_EN,		ENABLE);
		#endif
	}
#endif
	senphy_set_valid_lanes(MODULE_SENPHY, lanevalid << LVDS_VALID_OFFSET);

	//
	//  CHK-4: If VD Gen Method is not MATCH, Force the VD_GEN_WITH_HD to Disable to avoid malfunction.
	//         If VD Gen is MATCH, the configuration is project determined.
	//
	if (reg_ctrl.bit.VSYNC_GEN_METHOD != LVDS_VSYNC_GEN_MATCH_VD) {
		reg_ctrl.bit.VD_GEN_WITH_HD   = 0;
		LVDS_SETREG(LVDS_CTRL1_REG_OFS, reg_ctrl.reg);
	}


	//
	//  CHK-5: If Data Lane Number is 1 or 3, Must Enable DropPixel and also Re-Map Valid Width.
	//         If DP is already opened, leave the original setup.(project determined).
	//         The reason is listed in the CHK-2 above.
	//
	reg_dropctl0.reg = LVDS_GETREG(LVDS_PIXDROP_CTRL0_REG_OFS);
	if (dropixel_modified == TRUE) {
		// ReStore the DP Disable.
		dropixel_modified = FALSE;

		reg_dropctl0.bit.DROP_PIXEL_EN = 0;
		LVDS_SETREG(LVDS_PIXDROP_CTRL0_REG_OFS, reg_dropctl0.reg);
		LVDS_SETREG(LVDS_PIXDROP_CTRL1_REG_OFS, 0x00);
	}

	if ((reg_dropctl0.bit.DROP_PIXEL_EN == DISABLE) && ((reg_ctrl.bit.DAT_LANE_NUM == LVDS_DATLANE_1) || (reg_ctrl.bit.DAT_LANE_NUM == LVDS_DATLANE_3))) {
		// Store the DP is modified by driver.
		dropixel_modified = TRUE;

		reg_dropctl0.bit.DROP_PIXEL_EN     = 1;

		if (reg_ctrl.bit.DAT_LANE_NUM == LVDS_DATLANE_1) {
			reg_dropctl0.bit.PIXDROP_LEN   = 1;// 2
			reg_dropctl0.bit.LINE_DROP_LEN = 1;// 2

			LVDS_SETREG(LVDS_PIXDROP_CTRL1_REG_OFS, 0x02);
			LVDS_SETREG(LVDS_PIXDROP_CTRL2_REG_OFS, 0x00);
			LVDS_SETREG(LVDS_PIXDROP_CTRL3_REG_OFS, 0x00);
			LVDS_SETREG(LVDS_PIXDROP_CTRL4_REG_OFS, 0x00);
		} else {
			reg_dropctl0.bit.PIXDROP_LEN   = 3;// 4
			reg_dropctl0.bit.LINE_DROP_LEN = 1;// 2

			LVDS_SETREG(LVDS_PIXDROP_CTRL1_REG_OFS, 0x08);
			LVDS_SETREG(LVDS_PIXDROP_CTRL2_REG_OFS, 0x00);
			LVDS_SETREG(LVDS_PIXDROP_CTRL3_REG_OFS, 0x00);
			LVDS_SETREG(LVDS_PIXDROP_CTRL4_REG_OFS, 0x00);
		}

		LVDS_SETREG(LVDS_PIXDROP_CTRL0_REG_OFS, reg_dropctl0.reg);
	}



	if (lvds_image_duplicate) {
		T_LVDS_CTRL2_REG        reg_ctrl2;
		T_LVDS_CTLCOUNT_REG     reg_count0;
		T_LVDS_CTLCOUNT1_REG    reg_count1;
		T_LVDS_CTLCOUNT2_REG    reg_count2;
		T_LVDS_CTLCOUNT3_REG    reg_count3;

		// Set CHID2/3/4 = CHID
		reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
		reg_ctrl2.bit.VALID_CHID2 = reg_ctrl2.bit.VALID_CHID;
		reg_ctrl2.bit.VALID_CHID3 = reg_ctrl2.bit.VALID_CHID;
		reg_ctrl2.bit.VALID_CHID4 = reg_ctrl2.bit.VALID_CHID;
		LVDS_SETREG(LVDS_CTRL2_REG_OFS, reg_ctrl2.reg);

		reg_count0.reg = LVDS_GETREG(LVDS_CTLCOUNT_REG_OFS);
		reg_count1.reg = LVDS_GETREG(LVDS_CTLCOUNT1_REG_OFS);
		reg_count2.reg = LVDS_GETREG(LVDS_CTLCOUNT2_REG_OFS);
		reg_count3.reg = LVDS_GETREG(LVDS_CTLCOUNT3_REG_OFS);


		reg_count1.bit.FRMEND2_LINE_CNT = reg_count0.bit.FRMEND_LINE_CNT;
		reg_count2.bit.FRMEND3_LINE_CNT = reg_count0.bit.FRMEND_LINE_CNT;
		reg_count3.bit.FRMEND4_LINE_CNT = reg_count0.bit.FRMEND_LINE_CNT;
		LVDS_SETREG(LVDS_CTLCOUNT1_REG_OFS, reg_count1.reg);
		LVDS_SETREG(LVDS_CTLCOUNT2_REG_OFS, reg_count2.reg);
		LVDS_SETREG(LVDS_CTLCOUNT3_REG_OFS, reg_count3.reg);
	}

	//coverity[end_of_path]
	return FALSE;
}

#if 1

/**
    Set LVDS Engine Enable/Disable

    This api is used to start or stop the LVDS engine.
    After enabling the LVDS engine, the LVDS would start immediately to receive the sensor pixel streaming.
    After disabling the LVDS engine, the LVDS would be stopped during this frame receiving end
    and the enable bit would be cleared at this frame end.
    After disabling the LVDS engine, the user can use lvds_wait_interrupt(LVDS_INTERRUPT_FRMEND)
    to make sure the engine is disabled.

    @param[in] benable
     - @b TRUE:   Enable the LVDS engine immediately.
     - @b FALSE:  Disable the LVDS engine at the current frame end.

    @return
     - @b E_OK:  Done and success.
     - @b E_SYS: LVDS driver has not opened.
     - @b E_PAR: LVDS output lane swap config error.
*/
ER lvds_set_enable(BOOL benable)
//ER LVDS_API_DEFINE(set_enable)(BOOL benable)
{
#if LVDS_TWOCLK_SUPPORT
	T_LVDS_CTRL1_REG reg_ctrl;
#endif

	if (!lvds_opened) {
		DBG_ERR("Set EN without open\r\n");
		return E_SYS;
	}

	if (benable) {
		if (LVDS_GETREG(LVDS_CTRL0_REG_OFS) & 0x1) {
			return E_OK;
		}

		// Add security check here for checking valid settings:
		// such as valid lane and out swap
		if (LVDS_CHK_REG()) {
			return E_PAR;
		}
	}


	// One Clock Lane
	if (lvds_clk_switch) {
		senphy_set_clock_map(MODULE_SENPHY, LVDS_CLK0_ALT_SEL);
	} else {
		senphy_set_clock_map(MODULE_SENPHY, LVDS_CLK0_SEL);
	}

#if LVDS_TWOCLK_SUPPORT
	reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
	if (reg_ctrl.bit.CLK_LANE_NUM) {
		// Two Clock Lanes
		if (lvds_clk_switch) {
			senphy_set_clock2_map(MODULE_SENPHY, LVDS_CLK1_ALT_SEL);
		} else {
			senphy_set_clock2_map(MODULE_SENPHY, LVDS_CLK1_SEL);
		}
	} else {
		senphy_set_clock2_map(MODULE_SENPHY, SENPHY_CLKMAP_OFF);
	}
#endif

	if (benable) {
		// Set PHY Enable/Disable
		senphy_enable(MODULE_SENPHY, DISABLE);
		LVDS_DELAY_US_POLL(1);
		senphy_enable(MODULE_SENPHY, ENABLE);

		LVDS_DELAY_US_POLL(1);

		// Reset LVDS FIFO before each time controller enabled
		LVDS_FIFO_RESET();
	}

	if ((lvds_force_enable&0x2) && (benable == 0)) {
		UINT32 timeout_cnt = 0;
		LVDS_SETREG(LVDS_CTRL0_REG_OFS, LVDS_GETREG(LVDS_CTRL0_REG_OFS)| 0x20);

		while(LVDS_GETREG(LVDS_DEBUG1_REG_OFS)&LVDS_DES_W_FLAG_MSK) {
			if (timeout_cnt++ == LVDS_WAIT_FLAG_CLR_TIMEOUT) {
				DBG_ERR("lock clear: timeout! \r\n");
				break;
			}
		}

		// Set Controller ENABLE / DISABLE
		LVDS_SETREG(LVDS_CTRL0_REG_OFS, (benable > 0) | lvds_force_enable | 0x20);
	} else {
		// Set Controller ENABLE / DISABLE
		LVDS_SETREG(LVDS_CTRL0_REG_OFS, (benable > 0) | lvds_force_enable);
	}
	lvds_error_count   = 0;

	#ifdef __KERNEL__
	lvds_enable_errchk_timer(LVDS_ID_LVDS, benable);
	#endif

	return E_OK;
}

/**
    Check the LVDS engine is enabled or not

    Check the LVDS engine is enabled or not.
    After disabling the LVDS engine, the LVDS engine would not be disabled immediately.
    The user can use this API to make sure the engine is really disabled or using lvds_wait_interrupt(LVDS_INTERRUPT_FRMEND).

    @return
     - @b TRUE:  The LVDS engine is already enabled.
     - @b FALSE: The LVDS engine is disabled.
*/
BOOL lvds_get_enable(void)
//BOOL LVDS_API_DEFINE(get_enable)(void)
{
	return LVDS_GETREG(LVDS_CTRL0_REG_OFS) & 0x1;
}

/**
    Enable the LVDS Pixel Streaming

    After the LVDS engine is enabled and the sensor output streaming is also enabled by the serial interface,
    the user can use use api to make sure the pixel streaming is in normal working.
    This api would check the pixel stream is enabled and then return, otherwise, it would hang inside the api.
    So the user must insure the LVDS controller and the sensor is enabled before calling this api.

    @return
     - @b E_OK:  The LVDS pixel streaming is in normal working.
     - @b E_SYS: The LVDS driver has not enabled.
*/
ER lvds_enable_streaming(void)
//ER LVDS_API_DEFINE(enable_streaming)(void)
{
#if 0//LVDS_DESKEW_EN//_FPGA_EMULATION_
	UINT32 cnt = 0, hd_cnt = 0, retry_cnt = 0;
	static BOOL rst = 0;

	DBG_DUMP("?%d",rst);

	// In FPGA Emulation, We use this api to make sure that the
	// digital implemented PHY is run normally.

	if (LVDS_GET_ENABLE() == FALSE) {
		DBG_ERR("No Open\r\n");
		return E_SYS;
	}

	LVDS_SETREG(LVDS_INTSTS_REG_OFS, (0x2 & (~(LVDS_GETREG(LVDS_INTEN_REG_OFS)))));

	while (1) {
		if (LVDS_GETREG(LVDS_INTSTS_REG_OFS) & 0x2) {
			LVDS_SETREG(LVDS_INTSTS_REG_OFS, (0x2 & (~(LVDS_GETREG(LVDS_INTEN_REG_OFS)))));
			hd_cnt++;
			if (hd_cnt >= 3) {
				return E_OK;
			}
		}

		#if _FPGA_EMULATION_
		LVDS_DELAY_US_POLL(4001);
		#else
		LVDS_DELAY_US_POLL(41);
		#endif
		cnt++;

		if (cnt >= 50) {
			// If FPGA PHY is not run normally, Set digital PHY Reset.
			lvds_trigger_relock();
			#if _FPGA_EMULATION_
			DBG_DUMP("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!x!!!!!!");
			#endif
			rst++;

			cnt = 0;
			hd_cnt = 0;
			retry_cnt++;
			if (retry_cnt > 10) {
				DBG_ERR("Retry Cnt OVER 10!\r\n");
				return E_OK;
			}
		}
	}
#else
	return E_OK;
#endif
}

/**
    Set LVDS engine general configurations

    This api is used to configure the general function of the LVDS engine,
    such as DataLane number, ClockLane number, pixel format, synchronization method,... etc.
    The configurations of Pixel output order (LVDS_CONFIG_ID_OUTORDER_*) would not be active immediately.
    The configurations of Pixel output order is activated after LVDS engine enabled.
    This API can only be used under LVDS engine disabled, and should not be changed dynamically under engine working state.

    @param[in] config_id     The configuration selection ID. Please refer to LVDS_CONFIG_ID for details.
    @param[in] config_value   The configuration parameter according to the config_id.

    @return void
*/
void lvds_set_config(LVDS_CONFIG_ID config_id, UINT32 config_value)
//void LVDS_API_DEFINE(set_config)(LVDS_CONFIG_ID config_id, UINT32 config_value)
{
	unsigned long      flags;

	loc_cpu(flags);
	switch (config_id) {

		case LVDS_CONFIG_ID_DLANE_NUMBER: {
			T_LVDS_CTRL1_REG reg_ctrl;

			if ((config_value > LVDS_MAX_DLAN_NUMBER) || (config_value == LVDS_DATLANE_3)) {
				DBG_ERR("No supported lane number. %d\r\n", (UINT)config_value);
				break;
			}

			// Set Config: Data Lane Numbers
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			reg_ctrl.bit.DAT_LANE_NUM = config_value;// LVDS_DATLANE_*
			LVDS_SETREG(LVDS_CTRL1_REG_OFS, reg_ctrl.reg);
		}
		break;
/*
		case LVDS_CONFIG_ID_CLANE_NUMBER: {
			T_LVDS_CTRL1_REG reg_ctrl;

			if (config_value > LVDS_MAX_CLAN_NUMBER) {
				DBG_ERR("Exceed max clock lane number. %d\r\n", config_value);
				break;
			}

			// Set Config: Clock Lane Numbers
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			reg_ctrl.bit.CLK_LANE_NUM = config_value;// LVDS_CLKLANE_*
			LVDS_SETREG(LVDS_CTRL1_REG_OFS, reg_ctrl.reg);
		}
		break;
*/
		case LVDS_CONFIG_ID_PIXEL_DEPTH: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// Set Config: Pixel Depth as 8/10/12/14/16 Bits per pixel
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			reg_ctrl.bit.PIXEL_DEPTH = config_value;// LVDS_PIXDEPTH_*BIT
			LVDS_SETREG(LVDS_CTRL1_REG_OFS, reg_ctrl.reg);
		}
		break;

		case LVDS_CONFIG_ID_PIXEL_INORDER: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// Set Config: Pixel input order is MSB or LSB first
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			reg_ctrl.bit.DATA_IN_ORDER = config_value;// LVDS_DATAIN_BIT_ORDER_*
			LVDS_SETREG(LVDS_CTRL1_REG_OFS, reg_ctrl.reg);
		}
		break;

		case LVDS_CONFIG_ID_CROPALIGN: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// Set Config: If pixel depth is LVDS_PIXDEPTH_CROP2LANE, Use CropAlign to select crop bits area.
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			reg_ctrl.bit.CROP_ALIGN = config_value;// LVDS_CROP2LANE_BIT*_*
			LVDS_SETREG(LVDS_CTRL1_REG_OFS, reg_ctrl.reg);
		}
		break;

	case LVDS_CONFIG_ID_CROP_BITORDER: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// Set Config: If pixel depth is LVDS_PIXDEPTH_CROP2LANE, Use this to select Crop
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			reg_ctrl.bit.CROP_PIXEL_BIT_ORDER = config_value;// LVDS_CROP2LANE_BIT_ORDER_*
			LVDS_SETREG(LVDS_CTRL1_REG_OFS, reg_ctrl.reg);
		}
		break;

	case LVDS_CONFIG_ID_ROUND_ENABLE: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// Set Config: Rounding 14/16bits to 12bits function Enable or Disable.
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			reg_ctrl.bit.ROUND_EN = config_value;// ENABLE/DISABLE
			LVDS_SETREG(LVDS_CTRL1_REG_OFS, reg_ctrl.reg);
		}
		break;

	case LVDS_CONFIG_ID_VD_GEN_METHOD: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// Set Config: Vsync Pulse generation method.
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			reg_ctrl.bit.VSYNC_GEN_METHOD = config_value;// LVDS_VSYNC_GEN_*
			LVDS_SETREG(LVDS_CTRL1_REG_OFS, reg_ctrl.reg);
		}
		break;

	case LVDS_CONFIG_ID_VD_GEN_WITH_HD: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// Set Config: Enable or disable the Vsync Pulse Gen together with one Hsync Pulse.
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			reg_ctrl.bit.VD_GEN_WITH_HD   = config_value; // ENABLE/DISABLE
			LVDS_SETREG(LVDS_CTRL1_REG_OFS, reg_ctrl.reg);
		}
		break;

	case LVDS_CONFIG_ID_VALID_WIDTH: {
			T_LVDS_CTLCOUNT_REG reg_count;

			// Valid Line Width in pixel count
			reg_count.reg = LVDS_GETREG(LVDS_CTLCOUNT_REG_OFS);
			reg_count.bit.PIXCNT_PER_LINE = config_value;
			LVDS_SETREG(LVDS_CTLCOUNT_REG_OFS, reg_count.reg);

			valid_width = config_value;
		}
		break;

	case LVDS_CONFIG_ID_VALID_HEIGHT: {
			T_LVDS_CTLCOUNT_REG reg_count;

			// Valid Frame Height in line count
			reg_count.reg = LVDS_GETREG(LVDS_CTLCOUNT_REG_OFS);
			reg_count.bit.FRMEND_LINE_CNT = config_value;
			LVDS_SETREG(LVDS_CTLCOUNT_REG_OFS, reg_count.reg);
		}
		break;

	case LVDS_CONFIG_ID_VALID_HEIGHT2: {
			T_LVDS_CTLCOUNT1_REG reg_count;

			// Valid Frame Height 2 in line count
			reg_count.reg = LVDS_GETREG(LVDS_CTLCOUNT1_REG_OFS);
			reg_count.bit.FRMEND2_LINE_CNT = config_value;
			LVDS_SETREG(LVDS_CTLCOUNT1_REG_OFS, reg_count.reg);
		}
		break;

	case LVDS_CONFIG_ID_VALID_HEIGHT3: {
			T_LVDS_CTLCOUNT2_REG reg_count;

			// Valid Frame Height 3 in line count
			reg_count.reg = LVDS_GETREG(LVDS_CTLCOUNT2_REG_OFS);
			reg_count.bit.FRMEND3_LINE_CNT = config_value;
			LVDS_SETREG(LVDS_CTLCOUNT2_REG_OFS, reg_count.reg);
		}
		break;

	case LVDS_CONFIG_ID_VALID_HEIGHT4: {
			T_LVDS_CTLCOUNT3_REG reg_count;

			// Valid Frame Height 4 in line count
			reg_count.reg = LVDS_GETREG(LVDS_CTLCOUNT3_REG_OFS);
			reg_count.bit.FRMEND4_LINE_CNT = config_value;
			LVDS_SETREG(LVDS_CTLCOUNT3_REG_OFS, reg_count.reg);
		}
		break;

	case LVDS_CONFIG_ID_FSET_BIT: {
			T_LVDS_CTRL1_REG reg_ctrl;

			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			reg_ctrl.bit.FRAMESET_OFS   = config_value;
			LVDS_SETREG(LVDS_CTRL1_REG_OFS, reg_ctrl.reg);
		}
		break;

	case LVDS_CONFIG_ID_CHID_BIT0: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			reg_ctrl2.bit.CHID_BIT0_OFS = config_value;
			LVDS_SETREG(LVDS_CTRL2_REG_OFS, reg_ctrl2.reg);
		}
		break;

	case LVDS_CONFIG_ID_CHID_BIT1: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			reg_ctrl2.bit.CHID_BIT1_OFS = config_value;
			LVDS_SETREG(LVDS_CTRL2_REG_OFS, reg_ctrl2.reg);
		}
		break;

	case LVDS_CONFIG_ID_CHID_BIT2: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			reg_ctrl2.bit.CHID_BIT2_OFS = config_value;
			LVDS_SETREG(LVDS_CTRL2_REG_OFS, reg_ctrl2.reg);
		}
		break;

	case LVDS_CONFIG_ID_CHID_BIT3: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			reg_ctrl2.bit.CHID_BIT3_OFS = config_value;
			LVDS_SETREG(LVDS_CTRL2_REG_OFS, reg_ctrl2.reg);
		}
		break;

	case LVDS_CONFIG_ID_CHID: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			reg_ctrl2.bit.VALID_CHID = config_value;
			LVDS_SETREG(LVDS_CTRL2_REG_OFS, reg_ctrl2.reg);
		}
		break;

	case LVDS_CONFIG_ID_CHID2: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			reg_ctrl2.bit.VALID_CHID2 = config_value;
			LVDS_SETREG(LVDS_CTRL2_REG_OFS, reg_ctrl2.reg);
		}
		break;

	case LVDS_CONFIG_ID_CHID3: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			reg_ctrl2.bit.VALID_CHID3 = config_value;
			LVDS_SETREG(LVDS_CTRL2_REG_OFS, reg_ctrl2.reg);
		}
		break;

	case LVDS_CONFIG_ID_CHID4: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			reg_ctrl2.bit.VALID_CHID4 = config_value;
			LVDS_SETREG(LVDS_CTRL2_REG_OFS, reg_ctrl2.reg);
		}
		break;

	case LVDS_CONFIG_ID_IMGDUPLICATE_EN: {
			lvds_image_duplicate = config_value;
		}
		break;

	case LVDS_CONFIG_ID_DISABLE_SOURCE: {
			T_LVDS_CTRL3_REG reg_ctrl3;

			reg_ctrl3.reg = LVDS_GETREG(LVDS_CTRL3_REG_OFS);
			reg_ctrl3.bit.DISABLE_SRC = config_value;
			LVDS_SETREG(LVDS_CTRL3_REG_OFS, reg_ctrl3.reg);
		}
		break;

	case LVDS_CONFIG_ID_DP_ENABLE: {
			T_LVDS_PIXDROP_CTRL0_REG    reg_dropctl0;

			// Set Config: Drop Pixel Function Enable/Disable.
			reg_dropctl0.reg = LVDS_GETREG(LVDS_PIXDROP_CTRL0_REG_OFS);
			reg_dropctl0.bit.DROP_PIXEL_EN = config_value;// ENABLE/DISABLE
			LVDS_SETREG(LVDS_PIXDROP_CTRL0_REG_OFS, reg_dropctl0.reg);

			if (config_value > 0) {
				// The DP is opened by the user. Not by the driver internal.
				dropixel_modified = FALSE;
			}
		}
		break;

	case LVDS_CONFIG_ID_DP_PIXEL_LEN: {
			T_LVDS_PIXDROP_CTRL0_REG    reg_dropctl0;

			// Drop Pixel Horizontal Period
			reg_dropctl0.reg = LVDS_GETREG(LVDS_PIXDROP_CTRL0_REG_OFS);
			reg_dropctl0.bit.PIXDROP_LEN   = config_value - 1;
			LVDS_SETREG(LVDS_PIXDROP_CTRL0_REG_OFS, reg_dropctl0.reg);
		}
		break;

	case LVDS_CONFIG_ID_DP_PIXEL_SEL0: {
			// Set Config: Drop Pixel Pixel Select bits[31:0]
			LVDS_SETREG(LVDS_PIXDROP_CTRL1_REG_OFS, config_value);
		}
		break;

	case LVDS_CONFIG_ID_DP_PIXEL_SEL1: {
			// Set Config: Drop Pixel Pixel Select bits[32:63]
			LVDS_SETREG(LVDS_PIXDROP_CTRL2_REG_OFS, config_value);
		}
		break;

	case LVDS_CONFIG_ID_DP_LINE_LEN: {
			T_LVDS_PIXDROP_CTRL0_REG    reg_dropctl0;

			// Drop Pixel Vertical Period
			reg_dropctl0.reg = LVDS_GETREG(LVDS_PIXDROP_CTRL0_REG_OFS);
			reg_dropctl0.bit.LINE_DROP_LEN = config_value - 1;
			LVDS_SETREG(LVDS_PIXDROP_CTRL0_REG_OFS, reg_dropctl0.reg);
		}
		break;

	case LVDS_CONFIG_ID_DP_LINE_SEL0: {
			// Set Config: Drop Pixel Line Select bits[31:0]
			LVDS_SETREG(LVDS_PIXDROP_CTRL3_REG_OFS, config_value);
		}
		break;

	case LVDS_CONFIG_ID_DP_LINE_SEL1: {
			// Set Config: Drop Pixel Line Select bits[32:63]
			LVDS_SETREG(LVDS_PIXDROP_CTRL4_REG_OFS, config_value);
		}
		break;

	case LVDS_CONFIG_ID_OUTORDER_0:
	case LVDS_CONFIG_ID_OUTORDER_1:
	case LVDS_CONFIG_ID_OUTORDER_2:
	case LVDS_CONFIG_ID_OUTORDER_3:
	case LVDS_CONFIG_ID_OUTORDER_4:
	case LVDS_CONFIG_ID_OUTORDER_5:
	case LVDS_CONFIG_ID_OUTORDER_6:
	case LVDS_CONFIG_ID_OUTORDER_7:
		//case LVDS_CONFIG_ID_OUTORDER_8:
		//case LVDS_CONFIG_ID_OUTORDER_9:
		{
			if ((config_value == LVDS_PIXEL_ORDER_DL8) || (config_value == LVDS_PIXEL_ORDER_DL9)) {
				break;
			}

			if (config_value & LVDS_PIXEL_ORDER_D0) {
				config_value = (config_value & 0xF);
				if ((0x1 << config_value) & LVDS_VALID_MASK) {
					config_value = config_value - LVDS_VALID_OFFSET;
				} else {
					DBG_ERR("Not valid output order lane setting (%d)\n", (UINT)config_value);
					break;
				}
			}

			config_id = config_id - LVDS_CONFIG_ID_OUTORDER_0;

			if (config_id < 8) {
				lvds_out_order[0] &= ~(LVDS_PXIELOUT_MSK << (config_id << 2));
				lvds_out_order[0] |= (config_value << (config_id << 2));
			} else {
				//coverity[dead_error_begin]
				lvds_out_order[1] &= ~(LVDS_PXIELOUT_MSK << ((config_id - 8) << 2));
				lvds_out_order[1] |= (config_value << ((config_id - 8) << 2));
			}
		}
		break;

	case LVDS_CONFIG_ID_FIFO_AUTO_RST: {
			T_LVDS_DEBUG0_REG reg_dbg0;

			if (!lvds_opened) {
				DBG_ERR("Drv no open.\r\n");
				break;
			}

			if (config_value) {
				LVDS_DISABLE_INT(LVDS_INTERRUPT_FIFO_ER);
			} else {
				LVDS_ENABLE_INT(LVDS_INTERRUPT_FIFO_ER);
			}

			reg_dbg0.reg = LVDS_GETREG(LVDS_DEBUG0_REG_OFS);
			reg_dbg0.bit.AUTO_RST = config_value > 0;
			LVDS_SETREG(LVDS_DEBUG0_REG_OFS, reg_dbg0.reg);
		}
		break;

	case LVDS_CONFIG_ID_PATGEN_EN: {
			T_LVDS_PATGEN_REG reg_patgen;

			reg_patgen.reg = LVDS_GETREG(LVDS_PATGEN_REG_OFS);
			reg_patgen.bit.PATGEN_EN = config_value > 0;
			LVDS_SETREG(LVDS_PATGEN_REG_OFS, reg_patgen.reg);
		}
		break;

	case LVDS_CONFIG_ID_PATGEN_SEL: {
			T_LVDS_PATGEN_REG reg_patgen;

			reg_patgen.reg = LVDS_GETREG(LVDS_PATGEN_REG_OFS);
			reg_patgen.bit.PATGEN_VAL  = (config_value >> 16) & 0xFFFF;
			reg_patgen.bit.PATGEN_MODE = (config_value) & 0x7;
			LVDS_SETREG(LVDS_PATGEN_REG_OFS, reg_patgen.reg);
		}
		break;

	case LVDS_CONFIG_ID_CLANE_SWITCH: {
			lvds_clk_switch = config_value > 0;
		}
		break;

	case LVDS_CONFIG_ID_TIMEOUT_PERIOD: {
			lvds_timeout_period = config_value;
		}
		break;
	case LVDS_CONFIG_ID_FORCE_EN: {
			if (config_value) {
				lvds_force_enable = 2;
			} else {
				lvds_force_enable = 0;
			}

			if(LVDS_DESKEW_EN) {
				lvds_force_enable |= 0x10;
			}
		}
		break;
	case LVDS_CONFIG_ID_DESKEW_EN:
		LVDS_DESKEW_EN = config_value > 0;
		if(LVDS_DESKEW_EN) {
			lvds_force_enable |= 0x10;
		} else {
			lvds_force_enable &= ~0x10;
		}

		break;

	default:
		break;
	}
	unl_cpu(flags);
}

/**
    Get LVDS engine general configurations

    This api is used to retrieve the general function configuration value of the LVDS engine,
    such as DataLane number, ClockLane number, pixel format, synchronization method,... etc.
    This api returns the current register configuration value.

    @param[in] config_id     The configuration selection ID. Please refer to LVDS_CONFIG_ID for details.

    @return The configuration parameter according to the config_id.
*/
UINT32 lvds_get_config(LVDS_CONFIG_ID config_id)
//UINT32 LVDS_API_DEFINE(get_config)(LVDS_CONFIG_ID config_id)
{
	UINT32 ret = 0;

	switch (config_id) {
	case LVDS_CONFIG_ID_DLANE_NUMBER: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// LVDS_DATLANE_*
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			ret = reg_ctrl.bit.DAT_LANE_NUM;
		}
		break;

	case LVDS_CONFIG_ID_CLANE_NUMBER: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// LVDS_CLKLANE_*
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			ret = reg_ctrl.bit.CLK_LANE_NUM;
		}
		break;

	case LVDS_CONFIG_ID_PIXEL_DEPTH: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// LVDS_PIXDEPTH_*BIT
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			ret = reg_ctrl.bit.PIXEL_DEPTH;
		}
		break;

	case LVDS_CONFIG_ID_PIXEL_INORDER: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// LVDS_DATAIN_BIT_ORDER_LSB / LVDS_DATAIN_BIT_ORDER_MSB
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			ret = reg_ctrl.bit.DATA_IN_ORDER;
		}
		break;

	case LVDS_CONFIG_ID_CROPALIGN: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// LVDS_CROP2LANE_BIT*_*
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			ret = reg_ctrl.bit.CROP_ALIGN;
		}
		break;

	case LVDS_CONFIG_ID_CROP_BITORDER: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// LVDS_CROP2LANE_BIT_ORDER_MSB / LVDS_CROP2LANE_BIT_ORDER_LSB
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			ret = reg_ctrl.bit.CROP_PIXEL_BIT_ORDER;
		}
		break;

	case LVDS_CONFIG_ID_ROUND_ENABLE: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// ENABLE / DISABLE
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			ret = reg_ctrl.bit.ROUND_EN;
		}
		break;

	case LVDS_CONFIG_ID_VD_GEN_METHOD: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// LVDS_VSYNC_GEN_MATCH_VD / LVDS_VSYNC_GEN_VD2HD / LVDS_VSYNC_GEN_HD2VD
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			ret = reg_ctrl.bit.VSYNC_GEN_METHOD;
		}
		break;

	case LVDS_CONFIG_ID_VD_GEN_WITH_HD: {
			T_LVDS_CTRL1_REG reg_ctrl;

			// ENABLE / DISABLE
			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			ret = reg_ctrl.bit.VD_GEN_WITH_HD;
		}
		break;

	case LVDS_CONFIG_ID_VALID_WIDTH: {
			T_LVDS_CTLCOUNT_REG reg_count;

			// Valid Line Width in pixel count
			reg_count.reg = LVDS_GETREG(LVDS_CTLCOUNT_REG_OFS);
			ret = reg_count.bit.PIXCNT_PER_LINE;
		}
		break;

	case LVDS_CONFIG_ID_VALID_HEIGHT: {
			T_LVDS_CTLCOUNT_REG reg_count;

			// Valid Frame Height in line count
			reg_count.reg = LVDS_GETREG(LVDS_CTLCOUNT_REG_OFS);
			ret = reg_count.bit.FRMEND_LINE_CNT;
		}
		break;

	case LVDS_CONFIG_ID_VALID_HEIGHT2: {
			T_LVDS_CTLCOUNT1_REG reg_count;

			// Valid Frame Height 2 in line count
			reg_count.reg = LVDS_GETREG(LVDS_CTLCOUNT1_REG_OFS);
			ret = reg_count.bit.FRMEND2_LINE_CNT;
		}
		break;

	case LVDS_CONFIG_ID_VALID_HEIGHT3: {
			T_LVDS_CTLCOUNT2_REG reg_count;

			// Valid Frame Height 3 in line count
			reg_count.reg = LVDS_GETREG(LVDS_CTLCOUNT2_REG_OFS);
			ret = reg_count.bit.FRMEND3_LINE_CNT;
		}
		break;

	case LVDS_CONFIG_ID_VALID_HEIGHT4: {
			T_LVDS_CTLCOUNT3_REG reg_count;

			// Valid Frame Height 4 in line count
			reg_count.reg = LVDS_GETREG(LVDS_CTLCOUNT3_REG_OFS);
			ret = reg_count.bit.FRMEND4_LINE_CNT;
		}
		break;

	case LVDS_CONFIG_ID_FSET_BIT: {
			T_LVDS_CTRL1_REG reg_ctrl;

			reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
			ret = reg_ctrl.bit.FRAMESET_OFS;
		}
		break;

	case LVDS_CONFIG_ID_CHID_BIT0: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			ret = reg_ctrl2.bit.CHID_BIT0_OFS;
		}
		break;

	case LVDS_CONFIG_ID_CHID_BIT1: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			ret = reg_ctrl2.bit.CHID_BIT1_OFS;
		}
		break;

	case LVDS_CONFIG_ID_CHID_BIT2: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			ret = reg_ctrl2.bit.CHID_BIT2_OFS;
		}
		break;

	case LVDS_CONFIG_ID_CHID_BIT3: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			ret = reg_ctrl2.bit.CHID_BIT3_OFS;
		}
		break;

	case LVDS_CONFIG_ID_CHID: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			ret = reg_ctrl2.bit.VALID_CHID;
		}
		break;

	case LVDS_CONFIG_ID_CHID2: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			ret = reg_ctrl2.bit.VALID_CHID2;
		}
		break;

	case LVDS_CONFIG_ID_CHID3: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			ret = reg_ctrl2.bit.VALID_CHID3;
		}
		break;

	case LVDS_CONFIG_ID_CHID4: {
			T_LVDS_CTRL2_REG reg_ctrl2;

			reg_ctrl2.reg = LVDS_GETREG(LVDS_CTRL2_REG_OFS);
			ret = reg_ctrl2.bit.VALID_CHID4;
		}
		break;

	case LVDS_CONFIG_ID_IMGDUPLICATE_EN: {
			ret = lvds_image_duplicate;
		}
		break;

	case LVDS_CONFIG_ID_DISABLE_SOURCE: {
			T_LVDS_CTRL3_REG reg_ctrl3;

			reg_ctrl3.reg = LVDS_GETREG(LVDS_CTRL3_REG_OFS);
			ret = reg_ctrl3.bit.DISABLE_SRC;
		}
		break;

	case LVDS_CONFIG_ID_DP_ENABLE: {
			T_LVDS_PIXDROP_CTRL0_REG    reg_dropctl0;

			// ENABLE / DISABLE
			reg_dropctl0.reg = LVDS_GETREG(LVDS_PIXDROP_CTRL0_REG_OFS);
			ret = reg_dropctl0.bit.DROP_PIXEL_EN;
		}
		break;

	case LVDS_CONFIG_ID_DP_PIXEL_LEN: {
			T_LVDS_PIXDROP_CTRL0_REG    reg_dropctl0;

			// Drop Pixel Horizontal Period
			reg_dropctl0.reg = LVDS_GETREG(LVDS_PIXDROP_CTRL0_REG_OFS);
			ret = reg_dropctl0.bit.PIXDROP_LEN + 1;
		}
		break;

	case LVDS_CONFIG_ID_DP_PIXEL_SEL0: {
			ret = LVDS_GETREG(LVDS_PIXDROP_CTRL1_REG_OFS);
		}
		break;

	case LVDS_CONFIG_ID_DP_PIXEL_SEL1: {
			ret = LVDS_GETREG(LVDS_PIXDROP_CTRL2_REG_OFS);
		}
		break;

	case LVDS_CONFIG_ID_DP_LINE_LEN: {
			T_LVDS_PIXDROP_CTRL0_REG    reg_dropctl0;

			// Drop Pixel Vertical Period
			reg_dropctl0.reg = LVDS_GETREG(LVDS_PIXDROP_CTRL0_REG_OFS);
			ret = reg_dropctl0.bit.LINE_DROP_LEN + 1;
		}
		break;

	case LVDS_CONFIG_ID_DP_LINE_SEL0: {
			ret = LVDS_GETREG(LVDS_PIXDROP_CTRL3_REG_OFS);
		}
		break;

	case LVDS_CONFIG_ID_DP_LINE_SEL1: {
			ret = LVDS_GETREG(LVDS_PIXDROP_CTRL4_REG_OFS);
		}
		break;

	case LVDS_CONFIG_ID_VALID_LANE: {
			ret = LVDS_GETREG(LVDS_INPUTSEL_REG_OFS) << LVDS_VALID_OFFSET;
			ret &= LVDS_VALID_MASK;
		}
		break;

	case LVDS_CONFIG_ID_OUTORDER_0:
	case LVDS_CONFIG_ID_OUTORDER_1:
	case LVDS_CONFIG_ID_OUTORDER_2:
	case LVDS_CONFIG_ID_OUTORDER_3:
	case LVDS_CONFIG_ID_OUTORDER_4:
	case LVDS_CONFIG_ID_OUTORDER_5:
	case LVDS_CONFIG_ID_OUTORDER_6:
	case LVDS_CONFIG_ID_OUTORDER_7:
		//case LVDS_CONFIG_ID_OUTORDER_8:
		//case LVDS_CONFIG_ID_OUTORDER_9:
		{
			T_LVDS_REORDER0_REG     reg_reorder0;
			//T_LVDS_REORDER1_REG     reg_reorder1;

			config_id = config_id - LVDS_CONFIG_ID_OUTORDER_0;
			if (config_id < 8) {
				reg_reorder0.reg  = lvds_out_order[0];
				ret         = (reg_reorder0.reg >> (config_id << 2)) & 0xF;
			}

			ret = LVDS_PIXEL_ORDER_D0 + ret + LVDS_VALID_OFFSET;
		}
		break;
	case LVDS_CONFIG_ID_LANEMSK: {
			ret = LVDS_VALID_MASK;
		}
		break;
	case LVDS_CONFIG_ID_LANEOFS: {
			ret = LVDS_VALID_OFFSET;
		}
		break;
	case LVDS_CONFIG_ID_FORCE_EN: {
			ret = ((lvds_force_enable&0x2) > 0);
		}
		break;
	case LVDS_CONFIG_ID_CLANE_SWITCH: {
			ret = lvds_clk_switch;
		}
		break;

	case LVDS_CONFIG_ID_TIMEOUT_PERIOD: {
			ret = lvds_timeout_period;
		}
		break;

	case LVDS_CONFIG_ID_VD_CNT: {
			ret = lvds_vd_cnt;
		}
		break;

	default:
		DBG_WRN("Get Cfg ID err\r\n");
		break;
	}

	return ret;
}

/**
    Set LVDS specified Data Lane configurations

    Set new configurations to the specified Data Lane, such as synchronization codes.
    The LVDS_DATLANE_ID is mapping to the external pin out HSI_D0 ~ HSI_D9.
    The user must fill the synchronization codes which the sensor outputed to the correct data lane.
    If the data lane orders are swap by project external circuit layout, the user must also remember
    change the correct synchronization codes to correct data lane location.

    @param[in] channel_id    The specified data lane channel. Use LVDS_DATLANE_ID_* as input.
    @param[in] config_id     The configuration selection ID.
    @param[in] config_value  The configuration parameter according to the config_id.

    @return void
*/
void lvds_set_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id, UINT32 config_value)
//void LVDS_API_DEFINE(set_channel_config)(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id, UINT32 config_value)
{
	UINT32               ofs;
	T_LVDS_DL0CTRL0_REG  reg_ctrl0;
	T_LVDS_DL0CTRL1_REG  reg_ctrl1;
	T_LVDS_DL0CTRL2_REG  reg_ctrl2;
	T_LVDS_DL0CTRL3_REG  reg_ctrl3;
	unsigned long      flags;
	T_LVDS_LE0_REG       reg_le;
	T_LVDS_FE_REG        reg_fe;
	T_LVDS_FE2_REG       reg_fe2;

	ofs = channel_id << 4;

	loc_cpu(flags);
	switch (config_id) {
	case LVDS_CONFIG_CTRLWORD_HD: {
			// Set Data Channel Control Word HD
			reg_ctrl0.reg = LVDS_GETREG(LVDS_DL0CTRL0_REG_OFS + ofs);
			reg_ctrl0.bit.CTRL_W_HD = config_value;
			LVDS_SETREG(LVDS_DL0CTRL0_REG_OFS + ofs, reg_ctrl0.reg);
		}
		break;

	case LVDS_CONFIG_CTRLWORD_VD: {
			// Set Data Channel Control Word VD
			reg_ctrl0.reg = LVDS_GETREG(LVDS_DL0CTRL0_REG_OFS + ofs);
			reg_ctrl0.bit.CTRL_W_VD = config_value;
			LVDS_SETREG(LVDS_DL0CTRL0_REG_OFS + ofs, reg_ctrl0.reg);
		}
		break;

	case LVDS_CONFIG_CTRLWORD_VD2: {
			// Set Data Channel Control Word VD2
			reg_ctrl2.reg = LVDS_GETREG(LVDS_DL0CTRL2_REG_OFS + ofs);
			reg_ctrl2.bit.CTRL_W_VD2 = config_value;
			LVDS_SETREG(LVDS_DL0CTRL2_REG_OFS + ofs, reg_ctrl2.reg);
		}
		break;

	case LVDS_CONFIG_CTRLWORD_VD3: {
			// Set Data Channel Control Word VD3
			reg_ctrl3.reg = LVDS_GETREG(LVDS_DL0CTRL3_REG_OFS + ofs);
			reg_ctrl3.bit.CTRL_W_VD3 = config_value;
			LVDS_SETREG(LVDS_DL0CTRL3_REG_OFS + ofs, reg_ctrl3.reg);
		}
		break;

	case LVDS_CONFIG_CTRLWORD_VD4: {
			// Set Data Channel Control Word VD4
			reg_ctrl3.reg = LVDS_GETREG(LVDS_DL0CTRL3_REG_OFS + ofs);
			reg_ctrl3.bit.CTRL_W_VD4 = config_value;
			LVDS_SETREG(LVDS_DL0CTRL3_REG_OFS + ofs, reg_ctrl3.reg);
		}
		break;

	case LVDS_CONFIG_CTRLWORD_HD_MASK: {
			// Set Data Channel Control Word HD Mask
			reg_ctrl1.reg = LVDS_GETREG(LVDS_DL0CTRL1_REG_OFS + ofs);
			reg_ctrl1.bit.CW_HD_VALID = config_value;
			LVDS_SETREG(LVDS_DL0CTRL1_REG_OFS + ofs, reg_ctrl1.reg);
		}
		break;

	case LVDS_CONFIG_CTRLWORD_VD_MASK: {
			// Set Data Channel Control Word VD Mask
			reg_ctrl1.reg = LVDS_GETREG(LVDS_DL0CTRL1_REG_OFS + ofs);
			reg_ctrl1.bit.CW_VD_VALID = config_value;
			LVDS_SETREG(LVDS_DL0CTRL1_REG_OFS + ofs, reg_ctrl1.reg);
		}
		break;

	case LVDS_CONFIG_CTRLWORD_LE: {
			ofs = (channel_id >> 1) << 2;

			reg_le.reg = LVDS_GETREG(LVDS_LE0_REG_OFS + ofs);
			reg_le.reg &= ~(0xFFFF << ((channel_id & 0x1) ? 16: 0));
			reg_le.reg |= ((config_value & 0xFFFF) << ((channel_id & 0x1) ? 16: 0));
			LVDS_SETREG(LVDS_LE0_REG_OFS + ofs, reg_le.reg);
		}
		break;

	case LVDS_CONFIG_CTRLWORD_FE: {
			ofs = channel_id << 3;

			reg_fe.reg = LVDS_GETREG(LVDS_D0FE_REG_OFS + ofs);
			reg_fe.bit.CTRL_W_FE = config_value;
			LVDS_SETREG(LVDS_D0FE_REG_OFS + ofs, reg_fe.reg);
		}
		break;
	case LVDS_CONFIG_CTRLWORD_FE2: {
			ofs = channel_id << 3;

			reg_fe.reg = LVDS_GETREG(LVDS_D0FE_REG_OFS + ofs);
			reg_fe.bit.CTRL_W_FE2 = config_value;
			LVDS_SETREG(LVDS_D0FE_REG_OFS + ofs, reg_fe.reg);
		}
		break;
	case LVDS_CONFIG_CTRLWORD_FE3: {
			ofs = channel_id << 3;

			reg_fe2.reg = LVDS_GETREG(LVDS_D0FE2_REG_OFS + ofs);
			reg_fe2.bit.CTRL_W_FE3 = config_value;
			LVDS_SETREG(LVDS_D0FE2_REG_OFS + ofs, reg_fe2.reg);
		}
		break;
	case LVDS_CONFIG_CTRLWORD_FE4: {
			ofs = channel_id << 3;

			reg_fe2.reg = LVDS_GETREG(LVDS_D0FE2_REG_OFS + ofs);
			reg_fe2.bit.CTRL_W_FE4 = config_value;
			LVDS_SETREG(LVDS_D0FE2_REG_OFS + ofs, reg_fe2.reg);
		}
		break;

	default:
		break;
	}
	unl_cpu(flags);

}

/**
    Set LVDS specified Data Lane configurations by Pad Pin Naming

    Set new configurations to the specified Data Lane, such as synchronization codes.
    The LVDS_DATLANE_ID is mapping to the external pin out HSI_D0 ~ HSI_D9.
    The user must fill the synchronization codes which the sensor outputed to the correct data lane.
    If the data lane orders are swap by project external circuit layout, the user must also remember
    change the correct synchronization codes to correct data lane location.

    @param[in] lane_select   The specified data lane channel. Use LVDS_DATLANE_ID_* as input.
    @param[in] config_id     The configuration selection ID.
    @param[in] config_value  The configuration parameter according to the config_id.

    @return void
*/
void lvds_set_padlane_config(LVDS_IN_VALID lane_select, LVDS_CH_CONFIG_ID config_id, UINT32 config_value)
//void LVDS_API_DEFINE(set_padlane_config)(LVDS_IN_VALID lane_select, LVDS_CH_CONFIG_ID config_id, UINT32 config_value)
{
	UINT32               ofs, i;
	T_LVDS_DL0CTRL0_REG  reg_ctrl0;
	T_LVDS_DL0CTRL1_REG  reg_ctrl1;
	T_LVDS_DL0CTRL2_REG  reg_ctrl2;
	T_LVDS_DL0CTRL3_REG  reg_ctrl3;
	unsigned long      flags;
	T_LVDS_LE0_REG       reg_le;
	T_LVDS_FE_REG        reg_fe;
	T_LVDS_FE2_REG       reg_fe2;

	lane_select &= LVDS_VALID_MASK;
	lane_select = lane_select >> LVDS_VALID_OFFSET;

	for (i = 0; i < 8; i++) {

		if (lane_select & (1 << i)) {
			ofs = i << 4;

			loc_cpu(flags);
			switch (config_id) {
			case LVDS_CONFIG_CTRLWORD_HD: {
					// Set Data Channel Control Word HD
					reg_ctrl0.reg = LVDS_GETREG(LVDS_DL0CTRL0_REG_OFS + ofs);
					reg_ctrl0.bit.CTRL_W_HD = config_value;
					LVDS_SETREG(LVDS_DL0CTRL0_REG_OFS + ofs, reg_ctrl0.reg);
				}
				break;

			case LVDS_CONFIG_CTRLWORD_VD: {
					// Set Data Channel Control Word VD
					reg_ctrl0.reg = LVDS_GETREG(LVDS_DL0CTRL0_REG_OFS + ofs);
					reg_ctrl0.bit.CTRL_W_VD = config_value;
					LVDS_SETREG(LVDS_DL0CTRL0_REG_OFS + ofs, reg_ctrl0.reg);
				}
				break;

			case LVDS_CONFIG_CTRLWORD_VD2: {
					// Set Data Channel Control Word VD2
					reg_ctrl2.reg = LVDS_GETREG(LVDS_DL0CTRL2_REG_OFS + ofs);
					reg_ctrl2.bit.CTRL_W_VD2 = config_value;
					LVDS_SETREG(LVDS_DL0CTRL2_REG_OFS + ofs, reg_ctrl2.reg);
				}
				break;

			case LVDS_CONFIG_CTRLWORD_VD3: {
					// Set Data Channel Control Word VD3
					reg_ctrl3.reg = LVDS_GETREG(LVDS_DL0CTRL3_REG_OFS + ofs);
					reg_ctrl3.bit.CTRL_W_VD3 = config_value;
					LVDS_SETREG(LVDS_DL0CTRL3_REG_OFS + ofs, reg_ctrl3.reg);
				}
				break;

			case LVDS_CONFIG_CTRLWORD_VD4: {
					// Set Data Channel Control Word VD4
					reg_ctrl3.reg = LVDS_GETREG(LVDS_DL0CTRL3_REG_OFS + ofs);
					reg_ctrl3.bit.CTRL_W_VD4 = config_value;
					LVDS_SETREG(LVDS_DL0CTRL3_REG_OFS + ofs, reg_ctrl3.reg);
				}
				break;

			case LVDS_CONFIG_CTRLWORD_HD_MASK: {
					// Set Data Channel Control Word HD Mask
					reg_ctrl1.reg = LVDS_GETREG(LVDS_DL0CTRL1_REG_OFS + ofs);
					reg_ctrl1.bit.CW_HD_VALID = config_value;
					LVDS_SETREG(LVDS_DL0CTRL1_REG_OFS + ofs, reg_ctrl1.reg);
				}
				break;

			case LVDS_CONFIG_CTRLWORD_VD_MASK: {
					// Set Data Channel Control Word VD Mask
					reg_ctrl1.reg = LVDS_GETREG(LVDS_DL0CTRL1_REG_OFS + ofs);
					reg_ctrl1.bit.CW_VD_VALID = config_value;
					LVDS_SETREG(LVDS_DL0CTRL1_REG_OFS + ofs, reg_ctrl1.reg);
				}
				break;

			case LVDS_CONFIG_CTRLWORD_LE: {
					ofs = (i >> 1) << 2;

					reg_le.reg = LVDS_GETREG(LVDS_LE0_REG_OFS + ofs);
					reg_le.reg &= ~(0xFFFF << ((i & 0x1) ? 16: 0));
					reg_le.reg |= ((config_value & 0xFFFF) << ((i & 0x1) ? 16: 0));
					LVDS_SETREG(LVDS_LE0_REG_OFS + ofs, reg_le.reg);
				}
				break;

			case LVDS_CONFIG_CTRLWORD_FE: {
					ofs = i << 3;

					reg_fe.reg = LVDS_GETREG(LVDS_D0FE_REG_OFS + ofs);
					reg_fe.bit.CTRL_W_FE = config_value;
					LVDS_SETREG(LVDS_D0FE_REG_OFS + ofs, reg_fe.reg);
				}
				break;
			case LVDS_CONFIG_CTRLWORD_FE2: {
					ofs = i << 3;

					reg_fe.reg = LVDS_GETREG(LVDS_D0FE_REG_OFS + ofs);
					reg_fe.bit.CTRL_W_FE2 = config_value;
					LVDS_SETREG(LVDS_D0FE_REG_OFS + ofs, reg_fe.reg);
				}
				break;
			case LVDS_CONFIG_CTRLWORD_FE3: {
					ofs = i << 3;

					reg_fe2.reg = LVDS_GETREG(LVDS_D0FE2_REG_OFS + ofs);
					reg_fe2.bit.CTRL_W_FE3 = config_value;
					LVDS_SETREG(LVDS_D0FE2_REG_OFS + ofs, reg_fe2.reg);
				}
				break;
			case LVDS_CONFIG_CTRLWORD_FE4: {
					ofs = i << 3;

					reg_fe2.reg = LVDS_GETREG(LVDS_D0FE2_REG_OFS + ofs);
					reg_fe2.bit.CTRL_W_FE4 = config_value;
					LVDS_SETREG(LVDS_D0FE2_REG_OFS + ofs, reg_fe2.reg);
				}
				break;


			default:
				break;
			}
			unl_cpu(flags);
		}
	}

}

/**
    Get LVDS specified Data Lane configurations

    Get the configurations of the specified Data Lane, such as synchronization codes.

    @param[in] channel_id    The specified data lane channel. Use LVDS_DATLANE_ID_* as input.
    @param[in] config_id     The configuration selection ID.

    @return The configuration parameter according to the config_id.
*/
UINT32 lvds_get_channel_config(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id)
//UINT32 LVDS_API_DEFINE(get_channel_config)(LVDS_DATLANE_ID channel_id, LVDS_CH_CONFIG_ID config_id)
{
	UINT32               ofs, ret = 0;
	T_LVDS_DL0CTRL0_REG  reg_ctrl0;
	T_LVDS_DL0CTRL1_REG  reg_ctrl1;

	ofs = channel_id << 4;

	switch (config_id) {
	case LVDS_CONFIG_CTRLWORD_HD: {
			reg_ctrl0.reg = LVDS_GETREG(LVDS_DL0CTRL0_REG_OFS + ofs);
			ret = reg_ctrl0.bit.CTRL_W_HD;
		}
		break;

	case LVDS_CONFIG_CTRLWORD_VD: {
			reg_ctrl0.reg = LVDS_GETREG(LVDS_DL0CTRL0_REG_OFS + ofs);
			ret = reg_ctrl0.bit.CTRL_W_VD;
		}
		break;

	case LVDS_CONFIG_CTRLWORD_HD_MASK: {
			reg_ctrl1.reg = LVDS_GETREG(LVDS_DL0CTRL1_REG_OFS + ofs);
			ret = reg_ctrl1.bit.CW_HD_VALID;
		}
		break;

	case LVDS_CONFIG_CTRLWORD_VD_MASK: {
			reg_ctrl1.reg = LVDS_GETREG(LVDS_DL0CTRL1_REG_OFS + ofs);
			ret = reg_ctrl1.bit.CW_VD_VALID;
		}
		break;

	default:
		break;
	}

	return ret;
}

/**
    Set the synchronization word

    Set the LVDS synchronization words.
    The synchronozation word is used to locate the position of the control word.
    The control word VD/HD is configured by the another api lvds_set_channel_config().

    @param[in] sync_word_length The synchronization word length. Valid value range from 0x3 to 0x7.
    @param[in] psync_word       The synchronization word values.

    @return void
*/
void lvds_set_sync_word(UINT32 sync_word_length, UINT32 *psync_word)
//void LVDS_API_DEFINE(set_sync_word)(UINT32 sync_word_length, UINT32 *psync_word)
{
	T_LVDS_CTRL1_REG    reg_ctrl;
	T_LVDS_HDSYNC0_REG  reg_sync_words;
	UINT32              i;

	if ((sync_word_length < 1) || (sync_word_length > 7)) {
		return;
	}

	// Set SYNC Word Length
	reg_ctrl.reg = LVDS_GETREG(LVDS_CTRL1_REG_OFS);
	reg_ctrl.bit.SYNC_LEN = sync_word_length;
	LVDS_SETREG(LVDS_CTRL1_REG_OFS, reg_ctrl.reg);

	// Set SYNC Word values according to the Length
	for (i = 0; i < 7; i++) {
		if (i < sync_word_length) {
			reg_sync_words.reg  = LVDS_GETREG(LVDS_HDSYNC0_REG_OFS + ((i >> 1) << 2));
			reg_sync_words.reg &= ~(0xFFFF << ((i & 0x1) << 4));
			reg_sync_words.reg |= ((psync_word[i] & 0xFFFF) << ((i & 0x1) << 4));
			LVDS_SETREG(LVDS_HDSYNC0_REG_OFS + ((i >> 1) << 2), reg_sync_words.reg);
		} else {
			reg_sync_words.reg  = LVDS_GETREG(LVDS_HDSYNC0_REG_OFS + ((i >> 1) << 2));
			reg_sync_words.reg &= ~(0xFFFF << ((i & 0x1) << 4));
			LVDS_SETREG(LVDS_HDSYNC0_REG_OFS + ((i >> 1) << 2), reg_sync_words.reg);
		}
	}
}

/**
    Wait the LVDS interrupt event

    Wait the LVDS specified interrupt event. When the interrupt event is triggered and match the wanted events,
    the waited event flag would be returned.

    @param[in] waited_flag  The bit-wise OR of the waited interrupt events.

    @return The waited interrupt events.
*/
LVDS_INTERRUPT lvds_wait_interrupt(LVDS_INTERRUPT waited_flag)
//LVDS_INTERRUPT LVDS_API_DEFINE(wait_interrupt)(LVDS_INTERRUPT waited_flag)
{
	UINT32              temp;
	unsigned long      flags;
#ifndef __KERNEL__
	FLGPTN              uiflag = 0;
	//ER                  timer_ret = E_SYS;
#endif

	// This api would be used for both the LVDS and LVDS-TG driver.
	// So we must check if one of the both drivers is enabled.
	if (LVDS_GET_ENABLE()) {
		// Use semaphore lock to protect the wait interrupt access for multi-thread environment.
		if (LVDS_LOCK() != E_OK) {
			return 0x0;
		}

		// Clear Interrupt Status if the waited interrupt NOT enabled
		LVDS_SETREG(LVDS_INTSTS_REG_OFS, (waited_flag & (~(LVDS_GETREG(LVDS_INTEN_REG_OFS)))));

		// Clear waited flag
		loc_cpu(flags);
		lvds_interrupt_status &= (~(waited_flag | LVDS_INTERRUPT_ABORT));
		unl_cpu(flags);

		// This is race condition protection to prevent LVDS disable activate just after status clear
		// and this may cause the user hang at frame end waiting.
		if ((waited_flag & (LVDS_INTERRUPT_FRMEND | LVDS_INTERRUPT_FRMEND2 | LVDS_INTERRUPT_FRMEND3 | LVDS_INTERRUPT_FRMEND4)) && (LVDS_GET_ENABLE() == FALSE)) {
			LVDS_UNLOCK();
			return 0x0;
		}

		// Set Interrupt Enable
		LVDS_ENABLE_INT(waited_flag);

		waited_flag |= LVDS_INTERRUPT_ABORT;

		while (1) {
#ifndef __KERNEL__
			//if (lvds_timeout_period) {
			//	timer_ret = SW_TIMER_OPEN(&lvds_timeout_timer_id, LVDS_TIMEOUTCALLBACK);
			//	if (timer_ret == E_OK) {
			//		SW_TIMER_CFG(lvds_timeout_timer_id, lvds_timeout_period, SWTIMER_MODE_ONE_SHOT);
			//		SW_TIMER_START(lvds_timeout_timer_id);
			//	}
			//}

			// Wait the LVDS Interrupt Event Issued.
			//wai_flg(&uiflag, MODULE_FLAG_ID, MODULE_FLAG_PTN, TWF_ORW | TWF_CLR);
			vos_flag_wait_timeout(&uiflag, MODULE_FLAG_ID, MODULE_FLAG_PTN, TWF_ORW | TWF_CLR, 1000);

			if (uiflag == 0) {
				lvds_interrupt_status |= LVDS_INTERRUPT_ABORT;
			}


			//if (lvds_timeout_period) {
			//	if (timer_ret == E_OK) {
			//		SW_TIMER_STOP(lvds_timeout_timer_id);
			//		SW_TIMER_CLOSE(lvds_timeout_timer_id);
			//	}
			//}
#else
			init_completion(&lvds_completion);
			if (!wait_for_completion_timeout(&lvds_completion, msecs_to_jiffies(lvds_timeout_period))) {
				loc_cpu(flags);
				lvds_interrupt_status |= LVDS_INTERRUPT_ABORT;
				unl_cpu(flags);
			}
#endif

			// Store the Interrupt Events and check to see if the waited event hit?
			// If not the waited event hit, we would go to wait the interrupt event again till hit.
			temp = lvds_interrupt_status;
			if (temp & waited_flag) {
				loc_cpu(flags);
				lvds_interrupt_status &= (~waited_flag);
				unl_cpu(flags);
				LVDS_DISABLE_INT((waited_flag & (~LVDS_DEFAULT_INT)));

				// Unlock
				LVDS_UNLOCK();

				if (temp & LVDS_INTERRUPT_ABORT) {
					DBG_ERR("TIMEOUT ABORT\r\n");
				}

				return (temp & waited_flag);
			}
		}

	}

	//DBG_ERR("Not EN\r\n");
	return 0x0;
}

#endif

/**
    Dump LVDS engine Debug information

    This api should be used only when the sensor engineering developing period.
    This api would dump some of the important debugging messages for engineer debug.
    This api can be used only after the sensor and the LVDS controller are already enabling.
    The user must input the current clock lane operating frequency for driver reference.
    The debug information would be displayed on the console.

    @param[in] clock_frequency  The frequency of the current clock lane speed. Unit in Hertz. Not the bit rate bps.

    @return void
*/
void lvds_dump_debug_info(UINT32 clock_frequency)
//void LVDS_API_DEFINE(dump_debug_info)(UINT32 clock_frequency)
{}

/*
    print lvds info to uart
*/
BOOL lvds_print_info_to_uart(CHAR *str_cmd)
//BOOL LVDS_API_DEFINE(print_info_to_uart)(CHAR *str_cmd)
{
	LVDS_DUMPINFO(0);
	return TRUE;
}






















//@}
