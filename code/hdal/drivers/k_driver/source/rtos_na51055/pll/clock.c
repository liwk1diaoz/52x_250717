/*
    Change clock APIs

    Change clock APIs.

    @file       clock.c
    @ingroup    mIDrvSys_CG
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#include <kwrap/error_no.h>
#include <kwrap/spinlock.h>
#include <kwrap/nvt_type.h>
#include <kwrap/type.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <stdio.h>
#include "pll.h"
#include "pll_protected.h"
#include "pll_reg.h"
#include "pll_int.h"
#include "clock.h"
#include "plat/interrupt.h"
#include "plat/top.h"
#include <plat/ide.h>
#include <uart.h>
#include "cache_protected.h"
#include "raw_change_dma_clk.h"
#include "string.h"


#define __MODULE__    rtos_ckg
#ifndef __DBGFLT__
#if defined(_NVT_EMULATION_)
#define __DBGLVL__  NVT_DBG_IND
#else
#define __DBGLVL__  1       // Output all message by default. __DBGLVL__ will be set to 1 via make parameter when release code.
#endif
#define __DBGFLT__  	"*"     // Display everything when debug level is 2
#endif

#include <kwrap/debug.h>
#include <kwrap/task.h>

//unsigned int rtos_ckg_debug_level = NVT_DBG_IND;
static BOOL clock_opened = FALSE;

static ID		FLG_ID_CG;
static SEM_HANDLE       SEMID_CG;

#define FLGPTN_CG		(1<<0)

#define CLOCK_IND_MSG(...)          DBG_DUMP(__VA_ARGS__)
#define CLOCK_ERR_MSG(...)          DBG_DUMP(__VA_ARGS__)

static  VK_DEFINE_SPINLOCK(clk_spinlock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&clk_spinlock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&clk_spinlock, flags)

#define loc_multi_cores(flags)   loc_cpu(flags)
#define unl_multi_cores(flags)   unl_cpu(flags)

extern void clock_do_power_down(void);
extern void clock_do_power_down_with_power_off(void);
extern void clock_do_power_down_reload_fw_addr(UINT32, UINT32);
extern void clock_do_power_down_with_dram_off(void);
extern char _section_01_addr[];
extern char _load_change_dma_clock_start[];
extern char _ram_change_dma_clock_start[];
extern char _load_change_dma_clock_start[];
extern char _load_change_dma_clock_base[];
extern char _sram_vector_table[];


static CLK_CALLBACK_HDL clk_pwr_down_handler[CLK_CALLBACK_CNT] = {
	NULL,   NULL,   NULL,   NULL,   NULL
};

static UINT32 clk_wake_src = 0x1F;

/*
 * SCTLR_EL1/SCTLR_EL2/SCTLR_EL3 bits definitions
 */
#define CR_M            (1 << 0)        /* MMU enable                   */
#define CR_A            (1 << 1)        /* Alignment abort enable       */
#define CR_C            (1 << 2)        /* Dcache enable                */
#define CR_SA           (1 << 3)        /* Stack Alignment Check Enable */
#define CR_Z           	(1 << 11)       /* Branch prediction	       	*/
#define CR_I            (1 << 12)       /* Icache enable                */
#define CR_WXN          (1 << 19)       /* Write Permision Imply XN     */
#define CR_EE           (1 << 25)       /* Exception (Big) Endian       */

static inline unsigned int get_cr(void)
{
        unsigned int val;

        asm volatile("mrc p15, 0, %0, c1, c0, 0 @ get CR" : "=r" (val)
        												  :
                                                          : "cc" );
        return val;
}

static inline void set_cr(unsigned int val)
{
        asm volatile("mcr p15, 0, %0, c1, c0, 0 @ set CR" :
                                                          : "r" (val)
                                                          : "cc" );
        asm volatile("isb");
}




/*
    Lock module

    @return
        - @b E_OK: success
        - @b Else: fail
*/
static ER clock_lock(void)
{
	ER erReturn;

	erReturn        = SEM_WAIT(SEMID_CG);
	if (erReturn != E_OK) {
		DBG_ERR("%s: wait semaphore fail\r\n", __func__);
		return erReturn;
	}
	return E_OK;
}

/*
    Unlock module

    @return
        - @b E_OK: success
        - @b Else: fail
*/
static ER clock_unlock(void)
{
	ER erReturn = E_OK;

	SEM_SIGNAL(SEMID_CG);

	return erReturn;
}

static irqreturn_t clock_isr(int irq, void *devid)
{
	T_PLL_SLP_REG pdn_reg;

	// Clear CG interrupt status
	pdn_reg.reg = PLL_GETREG(PLL_SLP_REG_OFS);
	if (pdn_reg.bit.SLP_INT == 0) {
		return IRQ_NONE;
	}
	pdn_reg.bit.SLP_EN = 0;                  // PDN_EN may be cleared after 1T(PCLK) when PDN_INT is set.
	// Force PDN_EN field to 0 to prevent re-enable power down
	PLL_SETREG(PLL_SLP_REG_OFS, pdn_reg.reg);

	iset_flg(FLG_ID_CG, FLGPTN_CG);

	return IRQ_HANDLED;
}

//#if defined(_NVT_EMULATION_) ||  defined(_NVT_FPGA_)
/**
    Wait for Clock event flag

    Wait for Clock event flag

    @return void
*/
static void clk_wait_flag(void)
{
	FLGPTN ui_flag;

	wai_flg(&ui_flag, FLG_ID_CG, FLGPTN_CG, TWF_CLR | TWF_ORW);
}
//#endif

/**
    Clear for Clock event flag

    Clear for Clock event flag

    @return void
*/
static void clk_clear_flag(void)
{
	clr_flg(FLG_ID_CG, FLGPTN_CG);
}

/*
    clock enter power down

    Invoke all registered power down call back to enter power down

    @param[in] mode   power down mode. Available values are below:
		- @b CLK_PDN_MODE_SLEEP1
		- @b CLK_PDN_MODE_SLEEP2
		- @b CLK_PDN_MODE_SLEEP3

    @return void
*/
static void clk_enter_pwr_down(UINT32 mode)
{
	UINT32 i;

	// Find registered call back and invoke it
	for (i = 0; i < CLK_CALLBACK_CNT; i++) {
		if (clk_wake_src & (1 << i)) {
			if (clk_pwr_down_handler[i] != NULL) {
				clk_pwr_down_handler[i](mode, TRUE);
			}
		}
	}
}

/*
    clock exit power down

    Invoke all registered power down call back to leave power down

    @param[in] mode   power down mode. Available values are below:
		- @b CLK_PDN_MODE_SLEEP1
		- @b CLK_PDN_MODE_SLEEP2
		- @b CLK_PDN_MODE_SLEEP3

    @return void
*/
static void clk_exit_pwr_down(UINT32 mode)
{
	UINT32 i;

	// Find registered call back and invoke it
	for (i = 0; i < CLK_CALLBACK_CNT; i++) {
		if (clk_wake_src & (1 << i)) {
			if (clk_pwr_down_handler[i] != NULL) {
				clk_pwr_down_handler[i](mode, FALSE);
			}
		}
	}
}

/*
    Set CLKGEN call back routine

    This function provides a facility for upper layer to install callback routine.

    (SUGGESTED to be invoked in system init phase)

    @note Designed to be invoked by system driver, NOT project layer.

    @param[in] callback_id   callback routine identifier
    @param[in] pf_callback    callback routine to be installed

    @return
        - @b E_OK: install callback success
        - @b E_NOSPT: callback_id is not valid
*/
ER clk_set_callback(CLK_CALLBACK callback_id, CLK_CALLBACK_HDL pf_callback)
{
	ER ret = E_OK;

	if (clock_lock() != E_OK) return E_SYS;

	if (callback_id < CLK_CALLBACK_CNT) {
		clk_pwr_down_handler[callback_id] = pf_callback;
	} else {
		ret = E_NOSPT;
	}

	if (clock_unlock() != E_OK) return E_SYS;

	return ret;
}

/*
    Switch CPU/OCP clock
    (Internal API)

    @param[in] cpu_clk     New CPU clock select
                            - @b PLL_CLKSEL_CPU_80
                            - @b PLL_CLKSEL_CPU_PLL8
    @param[in] skip_int    Skip interrupt or not

    @return void
*/

static void clk_change_cpu_clock(UINT32 cpu_sel, UINT32 skip_int)
{
	UINT32 core_reg;
    unsigned long flags = 0;

	INT_INTC_ENABLE gic_int_en;
	T_PLL_SLP_REG reg_pdn;
	if (cpu_sel != PLL_CLKSEL_CPU_80 && cpu_sel != PLL_CLKSEL_CPU_PLL8 && cpu_sel != PLL_CLKSEL_CPU_480) {
		return ;
	}
	while (!(uart_checkIntStatus() & UART_INT_STATUS_TX_EMPTY)) {
	}
	int_get_gic_enable(&gic_int_en);
	int_disable_multi(gic_int_en);

	if(!skip_int) {
		request_irq(INT_ID_CG, clock_isr ,IRQF_TRIGGER_HIGH, "clock", 0);
		int_gic_set_target(INT_ID_CG, 1);
	} else {
		DBG_IND("skip int spin lock\r\n");
		loc_multi_cores(flags);
	}

#if defined(_NVT_FPGA_)
	if((*(UINT32 *)0xFFD00004 & 0x3) == 0) {

	} else {
		core_reg = *(volatile UINT32 *)(IOADDR_CPU_REG_BASE + 0xFC);
		core_reg |= (1<<1);
		*(volatile UINT32 *)(IOADDR_CPU_REG_BASE + 0xFC) = core_reg;
	}
#else
	if( nvt_get_chip_id() == CHIP_NA51084) {
		core_reg = *(volatile UINT32 *)(IOADDR_CPU_REG_BASE + 0xFC);
		core_reg |= (1<<1);
		*(volatile UINT32 *)(IOADDR_CPU_REG_BASE + 0xFC) = core_reg;
	}

#endif
	// Set new CPU clock
	pll_set_clock_rate(PLL_CLKSEL_CPU, cpu_sel);

	// Clear CG interrupt status
	reg_pdn.reg = PLL_GETREG(PLL_SLP_REG_OFS);
	reg_pdn.bit.SLP_INTE = ~skip_int;
	reg_pdn.bit.SLP_SKIP_CPU_SLEEP = skip_int;
	PLL_SETREG(PLL_SLP_REG_OFS, reg_pdn.reg);
	// Set power down mode to clock scaling mode
	reg_pdn.bit.SLP_MODE = CLK_PDN_MODE_CLKSCALING;
	reg_pdn.bit.SLP_EN = 1;

	if(!skip_int)
		clk_clear_flag();

	PLL_SETREG(PLL_SLP_REG_OFS, reg_pdn.reg);

	// Wait clock change
	if(!skip_int) {
		asm volatile("wfi");
		clk_wait_flag();
		CLOCK_IND_MSG("wfi interrupt\r\n");
	} else {
		CLOCK_IND_MSG("Skip interrupt\r\n");
		while (! (PLL_GETREG(PLL_SLP_REG_OFS) & (1<<3))) {
		};
		CLOCK_IND_MSG("Skip interrupt done\r\n");
		unl_multi_cores(flags);
	}

	int_enable_multi(gic_int_en);
	DBG_IND("Change clock done\r\n");
}

/**
    Change CPU and OCP clock

    This function changes the CPU and OCP clock rate.
\n  The hardware limitation of CPU clock rate is 80Mhz and PLL8 (max. 420MHz)
\n  The hardware limitation of OCP clock rate is the same as CPU clock.
\n  user should not set the CPU/OCP clock rate upper than this value.

    @param[in] cpu_clk     New CPU clock rate. (Unit: MHz) (Valid range: 80 and others(PLL8))

    @param[in] skip_int    Skip interrupt or not

    @return
        - @b TRUE:  Success
        - @b FALSE: Fail
*/
#define CPU_CLOCK_RATE_RATIO0_OFS 0x1188
#define CPU_CLOCK_RATE_RATIO1_OFS 0x118C
#define CPU_CLOCK_RATE_RATIO2_OFS 0x1190

#define CPU_CLOCK_RATE_RATIO0_528_OFS (0x47C0 + 0x20)
#define CPU_CLOCK_RATE_RATIO1_528_OFS (0x47C0 + 0x24)
#define CPU_CLOCK_RATE_RATIO2_528_OFS (0x47C0 + 0x28)

BOOL clk_change_cpu_ahb(UINT32 cpu_clk, UINT32 skip_int)
{
	UINT32  ui_cpu_sel;
#if !defined(_NVT_FPGA_)
	UINT32	ori_cpu_clock;
#endif

#if defined (_NVT_RUN_CORE2_)
	return TRUE;
#endif

	if (clock_opened == FALSE) {
		return FALSE;
	}

#if !defined(_NVT_FPGA_)
	if(nvt_get_chip_id() == CHIP_NA51055) {
		if (cpu_clk > 960) {
			cpu_clk = 960;
		}
	} else {
		if (cpu_clk > 1100) {
			cpu_clk = 1100;
		}
	}
#endif
	switch (cpu_clk) {
	case 80:
		ui_cpu_sel = PLL_CLKSEL_CPU_80;
		break;
	case 480:
		ui_cpu_sel = PLL_CLKSEL_CPU_480;
		break;
	default:
		ui_cpu_sel = PLL_CLKSEL_CPU_PLL8;
#if !defined(_NVT_FPGA_)
		// If already under PLL8, temp change to 80MHz
		if (pll_get_clock_rate(PLL_CLKSEL_CPU) == PLL_CLKSEL_CPU_PLL8) {
			if(nvt_get_chip_id() == CHIP_NA51055) {
				CLOCK_IND_MSG("0x%08x 0x%08x 0x%08x\r\n", *(UINT32 *)0xF0021188, *(UINT32 *)0xF002118C, *(UINT32 *)0xF0021190);
			} else {
				CLOCK_IND_MSG("0x%08x 0x%08x 0x%08x\r\n", *(UINT32 *)0xF00247E0, *(UINT32 *)0xF00247E4, *(UINT32 *)0xF00247E8);
			}
			ori_cpu_clock = pll_getPLLFreq(PLL_ID_8) / 1000000;
			CLOCK_IND_MSG("cpu_clk original = %3d MHz\r\n", ori_cpu_clock);
			if ((cpu_clk == ori_cpu_clock) || (cpu_clk == (ori_cpu_clock -1))) {
				CLOCK_IND_MSG("target clock = %d already = cpu_clk = %3d MHz\r\n", cpu_clk, ori_cpu_clock);
				return TRUE;
			}
			//clk_change_cpu_clock(PLL_CLKSEL_CPU_80, skip_int);
			//return TRUE;
		}
		//pll_set_pll_enable(PLL_ID_8, FALSE);
		//pll_set_pll_enable(PLL_ID_8, TRUE);
		pll_set_driver_pll(PLL_ID_8, cpu_clk * 131072 / 12);
		// After PLL_ID_8 ratio is written, MPLL needs 1023T (aboute 85 us) to latch it.
		// We take 2*85us = 170 us to ensure new setting take effect
		//Delay_DelayUs(170);
		//vos_task_delay_us(170);
#endif
		break;
	}

	clk_change_cpu_clock(ui_cpu_sel, skip_int);

	return TRUE;
}

typedef void (*CHG_CLK_GENERIC_CB)(void);
#define JUMP_ADDR 0xf07C0000

/**
    Change DMA clock to 507MHz

    This function changes the DMA clock to 507MHz.

    @return
        - @b TRUE:  Success
        - @b FALSE: Fail
*/
BOOL clk_change_dma_clk_507MHz(void)
{
	UINT32 reg;
	INT_INTC_ENABLE gic_int_en;
    unsigned long flags = 0;
	void (*image_entry)(void) = NULL;

	while (!(uart_checkIntStatus() & UART_INT_STATUS_TX_EMPTY)) {
	}
	int_get_gic_enable(&gic_int_en);
	int_disable_multi(gic_int_en);

	CLOCK_IND_MSG("memcpy->");
    memcpy((void *)JUMP_ADDR, ddr_slow, sizeof(ddr_slow));
    CLOCK_IND_MSG("done\n");
	cpu_cleanDCacheBlock(JUMP_ADDR, JUMP_ADDR + ALIGN_ROUND_32(sizeof(ddr_slow)));
    CLOCK_IND_MSG("flush->");
//	cpu_invalidateDCacheBlock(JUMP_ADDR, JUMP_ADDR + ALIGN_ROUND_32(sizeof(ddr_slow)));
    CLOCK_IND_MSG("done\r\n");

	loc_multi_cores(flags);
	reg = get_cr();

	cpu_cleanInvalidateDCacheAll();

	cpu_invalidateICacheAll();

	//icache & dcache & MMU disable
	reg &=~(CR_C|CR_Z|CR_I|CR_A|CR_M);
	set_cr(reg);

	//outer cache disable
    cpu_v7_outer_cache_disable();
	asm volatile("dsb");

	CLOCK_IND_MSG("Jump into sram\n");
	image_entry = (CHG_CLK_GENERIC_CB)(*((unsigned long*)JUMP_ADDR));
	image_entry();

	CLOCK_IND_MSG("Back\n");

	reg = get_cr();
	reg |=(CR_C|CR_Z|CR_I|CR_A);

	set_cr(reg);

	set_cr(reg|CR_M);

	cpu_v7_outer_cache_enable();

	asm volatile("dsb");
	int_enable_multi(gic_int_en);
	unl_multi_cores(flags);

	return TRUE;
}




/**
    Change APB clock

    This function change the APB clock rate.

    @param[in] apb_clk     New APB clock rate. Available values are below:
		- @b PLL_CLKSEL_APB_48:     APB clock = 48 MHz
		- @b PLL_CLKSEL_APB_60:     APB clock = 60 MHz
		- @b PLL_CLKSEL_APB_80:     APB clock = 80 MHz (Default setting)
		- @b PLL_CLKSEL_APB_120:    APB clock = 120 MHz

    @return TRUE if success, FALSE if fail
*/
BOOL clk_change_apb(UINT32 apb_clk)
{
	// Check APB clock
	if (apb_clk > PLL_CLKSEL_APB_120) {
		return FALSE;
	}

	pll_set_clock_rate(PLL_CLKSEL_APB, apb_clk);
	return TRUE;
}

/**
    Power down system

    This function powers down system.
\n  This api would also power down the PLL if CPU/APB are not useing PLL respectively.
    If user select to enter power down mode 3, but still have some modules are using PHY oscillator,
    the system PHY oscillator would not truely turn-off.(although you can enter power down mode still.)
    So, user should take care the management of the clock usage of each modules.

    @note Currently only support CLK_PDN_MODE_SLEEP1

    @param[in] mode   power down mode. Available values are below:
		- @b CLK_PDN_MODE_SLEEP1    CPU off,  MPLL on.   OSC on.   PCLK = OSC1/1024
		- @b CLK_PDN_MODE_SLEEP2    CPU off,  MPLL off.  OSC on.   PCLK = OSC1/1024
		- @b CLK_PDN_MODE_SLEEP3    CPU off,  MPLL off.  OSC off.  PCLK = RTC

    @return
		- @b TRUE:  Success.
		- @b FALSE: Fail.
*/
BOOL clk_powerdown(CLK_PDN_MODE mode)
{
#if !defined(_CPU2_RTOS_)
	UINT32  load_code_addr_start = (UINT32)&_load_change_dma_clock_start;
	UINT32  load_code_addr_end = (UINT32)&_load_change_dma_clock_base;
	UINT32  ram_exe_addr = (UINT32)&_ram_change_dma_clock_start;
	UINT32  len;
	BOOL    is_ide1_enabled;//, is_ide2_enabled;

	INT_INTC_ENABLE gic_int_en;
#if defined(_NVT_EMULATION_) ||  defined(_NVT_FPGA_)
	UINT32          old_vbar;
	UINT32          temp_vbar = (UINT32)&_sram_vector_table;
#endif
	if (clock_opened == FALSE) {
		DBG_ERR("driver already closed\r\n");
		return FALSE;
	}

    if(mode > CLK_PDN_MODE_CLKSCALING) {
    	is_ide1_enabled = pll_isClockEnabled(IDE1_CLK);
    	if (is_ide1_enabled == FALSE) {
    		pll_enableClock(IDE1_CLK);
    		DBG_WRN("IDE1_CLK is not enabled, force it enabled\r\n");
    	}

    	pll_clear_clk_auto_gating(CNN_M_GCLK);
    	pll_clear_clk_auto_gating(CNN2_M_GCLK);

    	idec_set_lb_read_en(IDE_ID_1, TRUE);
    	//idec_setLBReadEn(IDE_ID_2, TRUE);

    	len = load_code_addr_end - load_code_addr_start;

    	DBG_IND("default_vector_addr[0x%08x]/shadow_vector_addr[0x%08x]\r\n", (UINT32)&_section_01_addr, (UINT32)&_load_change_dma_clock_start);
    	DBG_IND("Bin load addr[0x%08x] - [0x%08x] len[0x%08x]/ram exe addr [0x%08x]\r\n", load_code_addr_start, load_code_addr_end, len, ram_exe_addr);


    	//  Copy instruction from DRAM to SRAM, esure word access on SRAM (APB domain)
    	{
    		UINT32 *pSrc = (UINT32 *)ram_exe_addr;
    		UINT32 *pDst = (UINT32 *)load_code_addr_start;
    		UINT32 i;

    		for (i = 0; i <= len; i += 4) {
    			*pDst++ = *pSrc++;
    		}
    	}
    	cpu_cleanInvalidateDCacheAll();
    	asm volatile("dmb");
    	asm volatile("dsb");

    	// Disable all interrupts except CG
    	int_get_gic_enable(&gic_int_en);
    	int_disable_multi(gic_int_en);

    	request_irq(INT_ID_CG, 0, IRQF_TRIGGER_HIGH, 0, 0);
    }

#endif
	if (mode >= CLK_PDN_MODE_SYSOFF_DRAMON) {
		DBG_ERR("Not support mode 0x%x\r\n", (int)mode);
		return FALSE;
	}

	if (clock_lock() != E_OK) return FALSE;

	// Setup registered module to power down state
	clk_enter_pwr_down(mode);

	/* FPGA Emulation Usage*/
	if(mode <= CLK_PDN_MODE_CLKSCALING) {
		clk_clear_flag();
	}

	{
		UINT32 core_reg;
#if defined(_NVT_FPGA_)
		if((*(UINT32 *)0xFFD00004 & 0x3) == 0) {

		} else {
			core_reg = *(volatile UINT32 *)(IOADDR_CPU_REG_BASE + 0xFC);
			core_reg |= (1<<1);
			*(volatile UINT32 *)(IOADDR_CPU_REG_BASE + 0xFC) = core_reg;
		}
#else
		if( nvt_get_chip_id() == CHIP_NA51084) {
			core_reg = *(volatile UINT32 *)(IOADDR_CPU_REG_BASE + 0xFC);
			core_reg |= (1<<1);
			*(volatile UINT32 *)(IOADDR_CPU_REG_BASE + 0xFC) = core_reg;
		}
#endif
	}

#if defined(_NVT_EMULATION_) ||  defined(_NVT_FPGA_)
	{
		T_PLL_SLP_REG   pdn_reg;
		// Enter power down mode and wait wakeup
		// Clear CG interrupt and enable CG interrupt
		pdn_reg.reg = 0;
		pdn_reg.bit.SLP_MODE = mode;
		pdn_reg.bit.SLP_INT = 1;
		pdn_reg.bit.SLP_INTE = 1;
		PLL_SETREG(PLL_SLP_REG_OFS, pdn_reg.reg);

		pdn_reg.bit.SLP_INT = 0;
		pdn_reg.bit.SLP_EN = 1;
		PLL_SETREG(PLL_SLP_REG_OFS, pdn_reg.reg);

		//Remap VBAR
        if(mode > CLK_PDN_MODE_CLKSCALING) {
    		asm volatile("mrc p15, 0, %0, c12, c0, 0" :"=r"(old_vbar));

    		DBG_IND("Original VBAR = [0x%08x] -> temp VBAR = [0x%08x]\r\n", old_vbar, temp_vbar);


    		asm volatile("mcr p15, 0, %0, c12, c0, 0" ::"r"(temp_vbar));
    		asm volatile("mrc p15, 0, %0, c12, c0, 0" :"=r"(temp_vbar));
    		DBG_IND("NEW VBAR = [0x%08x]\r\n", temp_vbar);
    		if (mode == CLK_PDN_MODE_SYSOFF_DRAMON) {
    			//unl_cpu();
    			CLOCK_IND_MSG("start clock_do_power_down_with_power_off\r\n");
    			clock_do_power_down_with_power_off();
    		} else if (mode == CLK_PDN_MODE_POWEROFF) {
    		    CLOCK_IND_MSG("start clock_do_power_down_with_dram_off\r\n");
    		    clock_do_power_down_with_dram_off();
    		} else {
    			DBG_IND("start clock_do_power_down\r\n");
    //			loc_cpu();
    			clock_do_power_down();

    			asm volatile("mcr p15, 0, %0, c12, c0, 0" ::"r"(old_vbar));
    			asm volatile("mrc p15, 0, %0, c12, c0, 0" :"=r"(temp_vbar));
    			asm volatile("isb");
    			asm volatile("dsb");
    			DBG_IND("Rollback to old VBAR = [0x%08x]\r\n", temp_vbar);

    //			unl_cpu();
    		}
        }
		// FPGA Emualtion
		if(mode <= CLK_PDN_MODE_CLKSCALING) {
			clk_wait_flag();
		}
	}
#else
	DBG_WRN("not support this API\r\n");
#endif

//	DBG_IND("int_enable_multi = [0x%08x]\r\n", gic_int_en);
    if(mode > CLK_PDN_MODE_CLKSCALING) {
	    int_enable_multi(gic_int_en);
    }

	// Setup registered module to normal state
	clk_exit_pwr_down(mode);

	if (clock_unlock() != E_OK) return FALSE;

	return TRUE;
}


void clock_platform_init(void)
{
	clock_opened = TRUE;
	OS_CONFIG_FLAG(FLG_ID_CG);
	SEM_CREATE(SEMID_CG, 1);

	clk_clear_flag();

	request_irq(INT_ID_CG, clock_isr, IRQF_TRIGGER_HIGH, "CG", 0);
}

void clock_platform_uninit(void)
{
	free_irq(INT_ID_CG, NULL);
	SEM_DESTROY(SEMID_CG);

	rel_flg(FLG_ID_CG);
}

