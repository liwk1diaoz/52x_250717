/*
	@file       pwm.c
	@ingroup    mIDrvIO_PWM

	@brief      Driver file for PWM module
				This file is the driver file that define the driver API for PWM module.

	@note       Nothing.

	Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#include "pwm_reg.h"


static UINT32 pwm_debug = DISABLE;
#define PWM_ISR_MSG(fmt, args...) do { \
	if (pwm_debug) DBG_DUMP(fmt, ##args); \
} while (0)

void pwm_isr(void);
void pwm_isr2(void);
void pwm_dump_info(void);

static void pwm_set_destination(UINT32 ui_pwm_id, PWM_DEST ui_pwm_dest);
UINT32 pwm_open_dest = PWM_DEST_TO_CPU1;

static UINT32 PWM_CTRL_INTEN_OFS = PWM_CTRL_INTEN_OFS0;
static UINT32 PWM_MS_CTRL_INTEN_OFS = PWM_MS_CTRL_INTEN_OFS0;
static UINT32 PWM_CCNT_CTRL_INTEN_OFS = PWM_CCNT_CTRL_INTEN_OFS0;
static UINT32 PWM_CLKDIV_LOAD_INTEN_OFS = PWM_CLKDIV_LOAD_INTEN_OFS0;
static UINT32 PWM_CTRL_INTSTS_OFS = PWM_CTRL_INTSTS_OFS0;
static UINT32 PWM_MS_CTRL_INTSTS_OFS = PWM_MS_CTRL_INTSTS_OFS0;
static UINT32 PWM_CCNT_CTRL_INTSTS_OFS = PWM_CCNT_CTRL_INTSTS_OFS0;
static UINT32 PWM_CLKDIV_LOAD_STS_OFS = PWM_CLKDIV_LOAD_STS_OFS0;

// PWM Lock Status
static ID pwm_lock_status = NO_TASK_LOCKED;

#if (_EMULATION_ == ENABLE)
#define PWM_CLOSE_TRIG_DGPIO    ENABLE
#define PWM_TOUT_TRIG_DGPIO     ENABLE
#else
#define PWM_CLOSE_TRIG_DGPIO    DISABLE
#define PWM_TOUT_TRIG_DGPIO     DISABLE

#endif

#if (PWM_CLOSE_TRIG_DGPIO == ENABLE)
#define PWM_CLOSE_GPIO  C_GPIO_0
#endif

#define PWM_CCNT0_TOUT_TRIG_GPIO    1
#define PWM_CCNT1_TOUT_TRIG_GPIO    2
#define PWM_CCNT2_TOUT_TRIG_GPIO    3
#define PWM_CCNT3_TOUT_TRIG_GPIO    4


#if (PWM_TOUT_TRIG_DGPIO == ENABLE)
#define PWM_TOUT_GPIO  P_GPIO_13
#endif

static BOOL b_pwm_auto_disable_pinmux = FALSE;
static BOOL b_disable_ccnt_after_target = FALSE;
static ER pwm_ccnt_disable(UINT32 ui_pwm_id);

/*
static FLGPTN v_pwm_flg_id[PWM_PWMCH_BITS] = {
	FLGPTN_PWM0,    FLGPTN_PWM1,    FLGPTN_PWM2,    FLGPTN_PWM3,
	FLGPTN_PWM4,    FLGPTN_PWM5,    FLGPTN_PWM6,    FLGPTN_PWM7,
	FLGPTN_PWM8,    FLGPTN_PWM9,    FLGPTN_PWM10,   FLGPTN_PWM11,
};

static FLGPTN v_pwm_flg_id2[PWM_MSCH_BITS] = {
	FLGPTN_PWM_MS0,    FLGPTN_PWM_MS1,    FLGPTN_PWM_MS2,    FLGPTN_PWM_MS3,
	FLGPTN_PWM_MS4,    FLGPTN_PWM_MS5,    FLGPTN_PWM_MS6,    FLGPTN_PWM_MS7,
};
*/

/*
    PWM ISR

    Interrupt service routine for PWM driver for CPU1.

    @return void
*/
void pwm_isr(void)
{
	UINT32 i;
	FLGPTN ptn = 0;
	FLGPTN ptn2 = 0;
	FLGPTN ptn3 = 0;
	UINT32 tmp_status;
	T_PWM_CTRL_INTSTS       pwm_int_sts;
	T_PWM_MS_CTRL_INTSTS    ms_int_sts;
	T_PWM_CCNT_CTRL_INTSTS  ccnt_int_sts;
	T_PWM_CLKDIV_LOAD_STS   pwm_clkdiv_load_sts;

	// Clear PWM interrupt status
	pwm_int_sts.reg = PWM_GETREG(PWM_CTRL_INTSTS_OFS);
	pwm_int_sts.reg &= PWM_GETREG(PWM_CTRL_INTEN_OFS);

	// Clear MS interrupt status
	ms_int_sts.reg = PWM_GETREG(PWM_MS_CTRL_INTSTS_OFS);
	ms_int_sts.reg &= PWM_GETREG(PWM_MS_CTRL_INTEN_OFS);

	// Clear CCNT interrupt status
	ccnt_int_sts.reg = PWM_GETREG(PWM_CCNT_CTRL_INTSTS_OFS);
	ccnt_int_sts.reg &= PWM_GETREG(PWM_CCNT_CTRL_INTEN_OFS);

	// Clear PWM clock load & target cnt
	pwm_clkdiv_load_sts.reg = PWM_GETREG(PWM_CLKDIV_LOAD_STS_OFS);
	pwm_clkdiv_load_sts.reg &= PWM_GETREG(PWM_CLKDIV_LOAD_INTEN_OFS);

	PWM_ISR_MSG("[CCNT] = [0x%08x]\r\n", (unsigned int)ccnt_int_sts.reg);
	PWM_ISR_MSG("[ PWM] = [0x%08x]\r\n", (unsigned int)pwm_int_sts.reg);
	PWM_ISR_MSG("[  MS] = [0x%08x]\r\n", (unsigned int)ms_int_sts.reg);
	PWM_ISR_MSG("[CKLD] = [0x%08x]\r\n", (unsigned int)pwm_clkdiv_load_sts.reg);

	i = 0;
	tmp_status = ms_int_sts.reg;
	PWM_SETREG(PWM_MS_CTRL_INTSTS_OFS, ms_int_sts.reg);
	while (tmp_status) {
		if (tmp_status & 0x01) {
			ptn2 |= (FLGPTN_PWM_MS0 << i);
			PWM_ISR_MSG("[MS] ms done => ch[%02d] ptn2[%x]\r\n", (unsigned int)i, (unsigned int)ptn2);
		}
		i++;
		tmp_status >>= 1;
	}

	i = 0;
	tmp_status = pwm_clkdiv_load_sts.reg & 0x3;
	PWM_SETREG(PWM_CLKDIV_LOAD_STS_OFS, (pwm_clkdiv_load_sts.reg & 0x3));
	while (tmp_status) {
		if (tmp_status & 0x01) {
			ptn3 |= (FLGPTN_PWM_00_03_CLKDIV_LOAD_DONE << i);
			PWM_ISR_MSG("[CKLD] clkdiv load done => ch[%02d]\r\n", (unsigned int)i);
		}
		i++;
		tmp_status >>= 1;
	}

	i = 0;
	tmp_status = ((pwm_clkdiv_load_sts.reg & 0x30000) >> 16);
	PWM_SETREG(PWM_CLKDIV_LOAD_STS_OFS, (pwm_clkdiv_load_sts.reg & 0x30000));
	while (tmp_status) {
		if (tmp_status & 0x01) {
			ptn3 |= (FLGPTN_PWM_00_03_TGT_CNT_DONE << i);
			PWM_ISR_MSG("[CKLD] target arrived => ch[%02d]\r\n", (unsigned int)i);
		}
		i++;
		tmp_status >>= 1;
	}

	if (ccnt_int_sts.reg) {
		PWM_SETREG(PWM_CCNT_CTRL_INTSTS_OFS, ccnt_int_sts.reg);
		for (i = PWM_CCNT0_BITS_START; i < (PWM_CCNT_BITS_CNT + PWM_CCNT0_BITS_START); i++) {
			if (!(ccnt_int_sts.reg & (PWM_CCNT0_EDGE | PWM_CCNT0_TRIG | PWM_CCNT0_TOUT))) {
				break;
			}
			if (ccnt_int_sts.reg & (PWM_CCNT0_EDGE << i)) {
				ptn2 |= (FLGPTN_PWM_CCNT0_EDGE << (i - PWM_CCNT0_BITS_START));

				if (i == PWM_CCNT0_BITS_START) {
					PWM_ISR_MSG("[CCNT] CCNT0 edge trigger\r\n");
				} else if (i == PWM_CCNT0_BITS_START + 1) {
					PWM_ISR_MSG("[CCNT] CCNT0 target value trigger\r\n");
					if (b_disable_ccnt_after_target) {
						//PWM_SETREG(PWM_CCNT_CTRL_INTEN_OFS, 0);
						pwm_ccnt_disable(0);
					}
				} else {
					PWM_ISR_MSG("[CCNT] CCNT0 timeout\r\n");
#if (PWM_TOUT_TRIG_DGPIO == ENABLE)
					gpio_clearPin(PWM_TOUT_GPIO);
#endif
				}
			}
		}

		for (i = PWM_CCNT1_BITS_START; i < (PWM_CCNT_BITS_CNT + PWM_CCNT1_BITS_START); i++) {
			if (!(ccnt_int_sts.reg & (PWM_CCNT1_EDGE | PWM_CCNT1_TRIG | PWM_CCNT1_TOUT))) {
				break;
			}
			if (ccnt_int_sts.reg & (PWM_CCNT0_EDGE << i)) {
				ptn2 |= (FLGPTN_PWM_CCNT1_EDGE << (i - PWM_CCNT1_BITS_START));

				if (i == PWM_CCNT1_BITS_START) {
					PWM_ISR_MSG("[CCNT] CCNT1 edge trigger\r\n");
				} else if (i == PWM_CCNT1_BITS_START + 1) {
					PWM_ISR_MSG("[CCNT] CCNT1 target value trigger\r\n");
					if (b_disable_ccnt_after_target) {
						pwm_ccnt_disable(1);
					}
				} else {
					PWM_ISR_MSG("[CCNT] CCNT1 timeout\r\n");
#if (PWM_TOUT_TRIG_DGPIO == ENABLE)
					gpio_clearPin(PWM_TOUT_GPIO);
#endif

				}
			}
		}

		for (i = PWM_CCNT2_BITS_START; i < (PWM_CCNT_BITS_CNT + PWM_CCNT2_BITS_START); i++) {
			if (!(ccnt_int_sts.reg & (PWM_CCNT2_EDGE | PWM_CCNT2_TRIG | PWM_CCNT2_TOUT))) {
				break;
			}
			if (ccnt_int_sts.reg & (PWM_CCNT0_EDGE << i)) {
				ptn2 |= (FLGPTN_PWM_CCNT2_EDGE << (i - PWM_CCNT2_BITS_START));

				if (i == PWM_CCNT2_BITS_START) {
					PWM_ISR_MSG("[CCNT] CCNT2 edge trigger\r\n");
				} else if (i == PWM_CCNT2_BITS_START + 1) {
					PWM_ISR_MSG("[CCNT] CCNT2 target value trigger\r\n");
					if (b_disable_ccnt_after_target) {
						pwm_ccnt_disable(2);
					}
				} else {
					PWM_ISR_MSG("[CCNT] CCNT2 timeout\r\n");
#if (PWM_TOUT_TRIG_DGPIO == ENABLE)
					gpio_clearPin(PWM_TOUT_GPIO);
#endif

				}
			}
		}

	}

	i = 0;
	tmp_status = pwm_int_sts.reg;
	PWM_SETREG(PWM_CTRL_INTSTS_OFS, pwm_int_sts.reg);
	while (tmp_status) {
		if (tmp_status & 0x01) {
			ptn |= (FLGPTN_PWM0 << i);
			PWM_ISR_MSG("[PWM] pwm done => ch[%02d]\r\n", (unsigned int)i);
		}
		i++;
		tmp_status >>= 1;
	}
	// Signal event flag
	if (ptn) {
		pwm_platform_flg_set(0, ptn);
	}

	if (ptn2) {
		//PWM_ISR_MSG("ptn2[%x]\r\n", (unsigned int)ptn2);
		pwm_platform_flg_set(1, ptn2);
	}

	if (ptn3) {
		pwm_platform_flg_set(2, ptn3);
	}
}

/*
    PWM ISR2

    Interrupt service routine for PWM driver for CPU2.

    @return void
*/
void pwm_isr2(void)
{
	UINT32 i;
	FLGPTN ptn = 0;
	FLGPTN ptn2 = 0;
	FLGPTN ptn3 = 0;
	UINT32 tmp_status;
	T_PWM_CTRL_INTSTS       pwm_int_sts;
	T_PWM_MS_CTRL_INTSTS    ms_int_sts;
	T_PWM_CCNT_CTRL_INTSTS  ccnt_int_sts;
	T_PWM_CLKDIV_LOAD_STS   pwm_clkdiv_load_sts;

	// Clear PWM interrupt status
	pwm_int_sts.reg = PWM_GETREG(PWM_CTRL_INTSTS_OFS);
	pwm_int_sts.reg &= PWM_GETREG(PWM_CTRL_INTEN_OFS);

	// Clear MS interrupt status
	ms_int_sts.reg = PWM_GETREG(PWM_MS_CTRL_INTSTS_OFS);
	ms_int_sts.reg &= PWM_GETREG(PWM_MS_CTRL_INTEN_OFS);

	// Clear CCNT interrupt status
	ccnt_int_sts.reg = PWM_GETREG(PWM_CCNT_CTRL_INTSTS_OFS);
	ccnt_int_sts.reg &= PWM_GETREG(PWM_CCNT_CTRL_INTEN_OFS);

	// Clear PWM clock load & target cnt
	pwm_clkdiv_load_sts.reg = PWM_GETREG(PWM_CLKDIV_LOAD_STS_OFS);
	pwm_clkdiv_load_sts.reg &= PWM_GETREG(PWM_CLKDIV_LOAD_INTEN_OFS);

	PWM_ISR_MSG("[CCNT] = [0x%08x]\r\n", (unsigned int)ccnt_int_sts.reg);
	PWM_ISR_MSG("[ PWM] = [0x%08x]\r\n", (unsigned int)pwm_int_sts.reg);
	PWM_ISR_MSG("[  MS] = [0x%08x]\r\n", (unsigned int)ms_int_sts.reg);
	PWM_ISR_MSG("[CKLD] = [0x%08x]\r\n", (unsigned int)pwm_clkdiv_load_sts.reg);

	i = 0;
	tmp_status = ms_int_sts.reg;
	PWM_SETREG(PWM_MS_CTRL_INTSTS_OFS, ms_int_sts.reg);
	while (tmp_status) {
		if (tmp_status & 0x01) {
			ptn2 |= (FLGPTN_PWM_MS0 << i);
			PWM_ISR_MSG("[MS] ms done => ch[%02d] ptn2[%x]\r\n", (unsigned int)i, (unsigned int)ptn2);
		}
		i++;
		tmp_status >>= 1;
	}

	i = 0;
	tmp_status = pwm_clkdiv_load_sts.reg & 0x3;
	PWM_SETREG(PWM_CLKDIV_LOAD_STS_OFS, (pwm_clkdiv_load_sts.reg & 0x3));
	while (tmp_status) {
		if (tmp_status & 0x01) {
			ptn3 |= (FLGPTN_PWM_00_03_CLKDIV_LOAD_DONE << i);
			PWM_ISR_MSG("[CKLD] clkdiv load done => ch[%02d]\r\n", (unsigned int)i);
		}
		i++;
		tmp_status >>= 1;
	}

	i = 0;
	tmp_status = ((pwm_clkdiv_load_sts.reg & 0x30000) >> 16);
	PWM_SETREG(PWM_CLKDIV_LOAD_STS_OFS, (pwm_clkdiv_load_sts.reg & 0x30000));
	while (tmp_status) {
		if (tmp_status & 0x01) {
			ptn3 |= (FLGPTN_PWM_00_03_TGT_CNT_DONE << i);
			PWM_ISR_MSG("[CKLD] target arrived => ch[%02d]\r\n", (unsigned int)i);
		}
		i++;
		tmp_status >>= 1;
	}

	if (ccnt_int_sts.reg) {
		PWM_SETREG(PWM_CCNT_CTRL_INTSTS_OFS, ccnt_int_sts.reg);
		for (i = PWM_CCNT0_BITS_START; i < (PWM_CCNT_BITS_CNT + PWM_CCNT0_BITS_START); i++) {
			if (!(ccnt_int_sts.reg & (PWM_CCNT0_EDGE | PWM_CCNT0_TRIG | PWM_CCNT0_TOUT))) {
				break;
			}
			if (ccnt_int_sts.reg & (PWM_CCNT0_EDGE << i)) {
				ptn2 |= (FLGPTN_PWM_CCNT0_EDGE << (i - PWM_CCNT0_BITS_START));

				if (i == PWM_CCNT0_BITS_START) {
					PWM_ISR_MSG("[CCNT] CCNT0 edge trigger\r\n");
				} else if (i == PWM_CCNT0_BITS_START + 1) {
					PWM_ISR_MSG("[CCNT] CCNT0 target value trigger\r\n");
				} else {
					PWM_ISR_MSG("[CCNT] CCNT0 timeout\r\n");
#if (PWM_TOUT_TRIG_DGPIO == ENABLE)
					gpio_clearPin(PWM_TOUT_GPIO);
#endif
				}
			}
		}

		for (i = PWM_CCNT1_BITS_START; i < (PWM_CCNT_BITS_CNT + PWM_CCNT1_BITS_START); i++) {
			if (!(ccnt_int_sts.reg & (PWM_CCNT1_EDGE | PWM_CCNT1_TRIG | PWM_CCNT1_TOUT))) {
				break;
			}
			if (ccnt_int_sts.reg & (PWM_CCNT0_EDGE << i)) {
				ptn2 |= (FLGPTN_PWM_CCNT1_EDGE << (i - PWM_CCNT1_BITS_START));

				if (i == PWM_CCNT1_BITS_START) {
					PWM_ISR_MSG("[CCNT] CCNT1 edge trigger\r\n");
				} else if (i == PWM_CCNT1_BITS_START + 1) {
					PWM_ISR_MSG("[CCNT] CCNT1 target value trigger\r\n");
				} else {
					PWM_ISR_MSG("[CCNT] CCNT1 timeout\r\n");
#if (PWM_TOUT_TRIG_DGPIO == ENABLE)
					gpio_clearPin(PWM_TOUT_GPIO);
#endif

				}
			}
		}

		for (i = PWM_CCNT2_BITS_START; i < (PWM_CCNT_BITS_CNT + PWM_CCNT2_BITS_START); i++) {
			if (!(ccnt_int_sts.reg & (PWM_CCNT2_EDGE | PWM_CCNT2_TRIG | PWM_CCNT2_TOUT))) {
				break;
			}
			if (ccnt_int_sts.reg & (PWM_CCNT0_EDGE << i)) {
				ptn2 |= (FLGPTN_PWM_CCNT2_EDGE << (i - PWM_CCNT2_BITS_START));

				if (i == PWM_CCNT2_BITS_START) {
					PWM_ISR_MSG("[CCNT] CCNT2 edge trigger\r\n");
				} else if (i == PWM_CCNT2_BITS_START + 1) {
					PWM_ISR_MSG("[CCNT] CCNT2 target value trigger\r\n");
				} else {
					PWM_ISR_MSG("[CCNT] CCNT2 timeout\r\n");
#if (PWM_TOUT_TRIG_DGPIO == ENABLE)
					gpio_clearPin(PWM_TOUT_GPIO);
#endif

				}
			}
		}

	}

	i = 0;
	tmp_status = pwm_int_sts.reg;
	PWM_SETREG(PWM_CTRL_INTSTS_OFS, pwm_int_sts.reg);
	while (tmp_status) {
		if (tmp_status & 0x01) {
			ptn |= (FLGPTN_PWM0 << i);
			PWM_ISR_MSG("[PWM] pwm done => ch[%02d]\r\n", (unsigned int)i);
		}
		i++;
		tmp_status >>= 1;
	}
	// Signal event flag
	if (ptn) {
		pwm_platform_flg_set(0, ptn);
	}

	if (ptn2) {
		//PWM_ISR_MSG("ptn2[%x]\r\n", (unsigned int)ptn2);
		pwm_platform_flg_set(1, ptn2);
	}

	if (ptn3) {
		pwm_platform_flg_set(2, ptn3);
	}
}

/*
    Check PWM ID

    Check whether the PWM ID is valid or not

    @param[in] ui_start_bit   Start search bit
    @param[in] ui_pwm_id      PWM ID (bitwise), one ID at a time

    @return Check ID status
		- @b PWM_INVALID_ID: Not a valid PWM ID
		- @b Other: ID offset (0 ~ 2 for PWM, 4 for CCNT)
*/
static UINT32 pwm_is_valid_id(UINT32 ui_start_bit, UINT32 ui_pwm_id)
{
	UINT32 i;

	for (i = ui_start_bit; i < (PWM_ALLCH_BITS + PWM_CCNTCH_BITS); i++) {
		if (ui_pwm_id & (1 << i)) {
			return i;
		}
	}
	return PWM_INVALID_ID;
}



/*
    Get if PWM enable

    Get specific PWM channel is enabled or not

    @param[in] ui_pwm_id PWM ID(one PWM id per time)

	@return Start status
		- @b TRUE   : Enabled
		- @b FALSE  : Disabled
*/
static BOOL pwm_pwm_is_en(UINT32 ui_pwm_id)
{
	if ((PWM_GETREG(PWM_ENABLE_OFS) & ui_pwm_id) == ui_pwm_id) {
		return TRUE;
	} else {
		return FALSE;
	}
}


/*
    Get if CCNT enable

    Get specific CCNT channel is enabled or not

    @param[in] ui_pwm_id PWM ID

    @return Start status
		- @b TRUE   : Enabled
		- @b FALSE  : Disabled
*/
static BOOL pwm_ccnt_is_en(UINT32 ui_pwm_id)
{
	UINT32  ui_ccnt_start, ui_cur_ccnt_start;
	UINT32  ui_pattern;

	if (ui_pwm_id != PWMID_CCNT0 && ui_pwm_id != PWMID_CCNT1 && ui_pwm_id != PWMID_CCNT2) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	ui_ccnt_start     = pwm_is_valid_id(0, PWMID_CCNT0);
	ui_cur_ccnt_start  = pwm_is_valid_id(0, ui_pwm_id);
	if ((ui_ccnt_start == PWM_INVALID_ID) || (ui_cur_ccnt_start == PWM_INVALID_ID)) {
		DBG_ERR("invalid ID!\r\n");
		return E_PAR;		
	}
	
	ui_pattern       = (1 << (ui_cur_ccnt_start - ui_ccnt_start));

	if ((PWM_GETREG(PWM_CCNT_ENABLE_OFS) & ui_pattern) == ui_pattern) {
		return TRUE;
	} else {
		return FALSE;
	}
}


/*
    Stop CCNT

    Stop MSTP based on MSTP parameter descriptor set by pwm_ms_set().

    @param ui_pwm_id PWM ID, one ID at a time
    @return Start status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
static ER pwm_ccnt_disable(UINT32 ui_pwm_id)
{
	T_PWM_CCNT_ENABLE pwm_ccnt_en;
	// Disable CCNT
	pwm_ccnt_en.reg = PWM_GETREG(PWM_CCNT_ENABLE_OFS);
	pwm_ccnt_en.reg &= ~(1 << ui_pwm_id);
	PWM_SETREG(PWM_CCNT_ENABLE_OFS, pwm_ccnt_en.reg);

	return E_OK;
}


/*
    Attach PWM driver

    Pre-initialize PWM driver before PWM driver is opened.
    This function should be the very first function to be invoked.

    @param[in] ui_enum_pwm_id      PWM ID (enum), one ID at a time.

    @return void
*/

static void pwm_attach(UINT32 ui_enum_pwm_id)
{
	//#NT#2015/10/05#Steven Wang -begin
	//#NT#43842/43841
	if (ui_enum_pwm_id >= PWMID_NO_TOTAL_CNT) {
		return;
	}
	//#NT#2015/10/05#Steven Wang -end
	pwm_platform_enable_clk(ui_enum_pwm_id);

	if (b_pwm_auto_disable_pinmux) {
		//#NT#2015/10/07#Steven Wang -begin
		//#NT#Coverity 43842
		if (ui_enum_pwm_id <= PWMID_NO_11)
			//#NT#2015/10/07#Steven Wang -end
		{
			pwm_platform_set_pinmux(ui_enum_pwm_id, TRUE);
		}
	}
}


/*
    Detach PWM driver

    De-initialize PWM driver after SPI driver is closed.

    @param[in] ui_enum_pwm_id  PWM ID (enum), one ID at a time.

    @return void
*/
static void pwm_detach(UINT32 ui_enum_pwm_id)
{
	//#NT#2015/10/05#Steven Wang -begin
	//#NT#43844/43843
	if (ui_enum_pwm_id >= PWMID_NO_TOTAL_CNT) {
		return;
	}
	//#NT#2015/10/05#Steven Wang -end
	pwm_platform_disable_clk(ui_enum_pwm_id);

	if (b_pwm_auto_disable_pinmux) {
		//#NT#2015/10/07#Steven Wang -begin
		//#NT#Coverity 43844
		if (ui_enum_pwm_id <= PWMID_NO_11)
			//#NT#2015/10/07#Steven Wang -end
		{
			pwm_platform_set_pinmux(ui_enum_pwm_id, FALSE);
		}
	}
}


/*
    Enter PWM access

    Enter PWM access

    @param[in] ui_enum_pwm_id  PWM ID (enum), one ID at a time.

    @return Enter access status
		- @b E_OK: Enter PWM access successfully
		- @b E_PAR: Invalid PWM ID
*/
static ER pwm_lock(UINT ui_enum_pwm_id)
{
	ER er_return = E_OK;
	UINT32 flags = 0;

	//#NT#2015/10/05#Steven Wang -begin
	//#NT#43845
	if (ui_enum_pwm_id >= PWMID_NO_TOTAL_CNT) {
		return E_PAR;
	}
	//#NT#2015/10/05#Steven Wang -end

	if (ui_enum_pwm_id >= PWMID_CCNT_NO_0 && ui_enum_pwm_id <= PWMID_CCNT_NO_2) {
		er_return = pwm_platform_sem_wait(PWM_TYPE_CCNT, ui_enum_pwm_id - PWMID_CCNT_NO_0);
	} else {
		er_return = pwm_platform_sem_wait(PWM_TYPE_PWM, ui_enum_pwm_id);
	}

	if (er_return != E_OK) {
		return er_return;
	}
	flags = pwm_platform_spin_lock();
	pwm_lock_status |= (1 << ui_enum_pwm_id);
	pwm_platform_spin_unlock(flags);

	return E_OK;
}


/*
    Leave PWM access

    Leave PWM access

    @param[in] ui_enum_pwm_id      PWM ID (enum), one ID at a time.

    @return Leave access status
		- @b E_OK: Leave PWM access successfully
		- @b E_PAR: Invalid PWM ID
*/
static ER pwm_unlock(UINT ui_enum_pwm_id)
{
	ER er_return = 0;
	UINT32 flags = 0;

	//#NT#2015/10/05#Steven Wang -begin
	//#NT#43846
	if (ui_enum_pwm_id >= PWMID_NO_TOTAL_CNT) {
		return E_PAR;
	}
	//#NT#2015/10/05#Steven Wang -end
	if (ui_enum_pwm_id >= PWMID_CCNT_NO_0 && ui_enum_pwm_id <= PWMID_CCNT_NO_2) {
		er_return = pwm_platform_sem_signal(PWM_TYPE_CCNT, ui_enum_pwm_id - PWMID_CCNT_NO_0);
	} else {
		er_return = pwm_platform_sem_signal(PWM_TYPE_PWM, ui_enum_pwm_id);

	}

	// there is no owner no longer
	flags = pwm_platform_spin_lock();
	pwm_lock_status &= ~(1 << ui_enum_pwm_id);;
	pwm_platform_spin_unlock(flags);

	return er_return;
}

/*
    Internal open flow.

    Internal open function shared by pwm_open() and pwm_open_NoSwitchPinmux().

    @param[in] ui_pwm_id          PWM ID (bitwise), allow multiple ID at a time.

    @return Open PWM status
		- @b E_OK: Success
		- @b Others: Open PWM failed
*/
static ER pwm_open_internal(UINT32 ui_pwm_id)
{
	ER                      er_return;
	UINT32                  ui_enum_pwm_id;

	T_PWM_CTRL_INTEN        pwm_ctrl_int_en;
	T_PWM_CTRL_INTSTS       pwm_ctrl_int_sts;

	T_PWM_MS_CTRL_INTEN     ms_ctrl_int_en;
	T_PWM_MS_CTRL_INTSTS    ms_ctrl_int_sts;

	T_PWM_CCNT_CTRL_INTEN   ccnt_ctrl_int_en;
	T_PWM_CCNT_CTRL_INTSTS  ccnt_ctrl_int_sts;

#if defined __FREERTOS
	pwm_platform_create_resource();
#endif

	ui_enum_pwm_id = pwm_is_valid_id(0, ui_pwm_id);
	if (ui_enum_pwm_id == PWM_INVALID_ID) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	// Lock PWM resource
	er_return = pwm_lock(ui_enum_pwm_id);

	if (er_return != E_OK) {
		return er_return;
	}

	// First time to open PWM
	if ((UINT32)pwm_lock_status == ui_pwm_id) {
		// Clear all PWM interrupt status
		pwm_ctrl_int_sts.reg = PWM_GETREG(PWM_CTRL_INTSTS_OFS);
		pwm_ctrl_int_sts.reg |= PWM_INT_STS_ALL;
		PWM_SETREG(PWM_CTRL_INTSTS_OFS, pwm_ctrl_int_sts.reg);

		// Clear all MS interrupt status
		ms_ctrl_int_sts.reg = PWM_GETREG(PWM_MS_CTRL_INTSTS_OFS);
		ms_ctrl_int_sts.reg |= PWM_INT_MS_STS_ALL;
		PWM_SETREG(PWM_MS_CTRL_INTSTS_OFS, ms_ctrl_int_sts.reg);

		// Clear all CCNT interrupt status
		ccnt_ctrl_int_sts.reg = PWM_GETREG(PWM_CCNT_CTRL_INTSTS_OFS);
		ccnt_ctrl_int_sts.reg |= PWM_INT_CCNT_STS_ALL;
		PWM_SETREG(PWM_CCNT_CTRL_INTSTS_OFS, ccnt_ctrl_int_sts.reg);
#if 0
		// Clear all CLKDIV load interrupt status
		pwm_clkdiv_load_sts.reg = PWM_GETREG(PWM_CLKDIV_LOAD_OFS);
		pwm_clkdiv_load_sts.reg |= PWM_INT_CLKDIV_STS_ALL;
		PWM_SETREG(PWM_CLKDIV_LOAD_OFS, pwm_clkdiv_load_sts.reg);
#endif
		pwm_platform_int_enable();
	}
	if (ui_enum_pwm_id >= PWMID_CCNT_NO_0 && ui_enum_pwm_id <= PWMID_CCNT_NO_2) {
		// Enable all PWM interrupts
		ccnt_ctrl_int_en.reg = PWM_GETREG(PWM_CCNT_CTRL_INTEN_OFS);
		ccnt_ctrl_int_en.reg |= ((/*PWM_INT_CCNT0_EDGE_STS|*/PWM_INT_CCNT0_TRIG_STS) << ((ui_enum_pwm_id - PWMID_CCNT_NO_0) * 4));
		PWM_SETREG(PWM_CCNT_CTRL_INTEN_OFS, ccnt_ctrl_int_en.reg);

	} else {
		// Only enable specific id (opened)
		// Enable specific PWM interrupts
		pwm_ctrl_int_en.reg = PWM_GETREG(PWM_CTRL_INTEN_OFS);
		pwm_ctrl_int_en.reg |= (1 << ui_enum_pwm_id);
		PWM_SETREG(PWM_CTRL_INTEN_OFS, pwm_ctrl_int_en.reg);

		// Enable specific MS interrupts
		ms_ctrl_int_en.reg = PWM_GETREG(PWM_MS_CTRL_INTEN_OFS);
		ms_ctrl_int_en.reg |= (1 << ui_enum_pwm_id);
		PWM_SETREG(PWM_MS_CTRL_INTEN_OFS, ms_ctrl_int_en.reg);
	}

	// Enable clock and switch pinmux
	pwm_attach(ui_enum_pwm_id);

	return er_return;
}

/*
    Internal close flow.

    Internal close function shared by pwm_close() and pwm_closeNoPatch().

    @param[in] ui_pwm_id      PWM ID (bitwise), one ID at a time
    @param[in] b_wait        driver is closed after waiting PWM signal level back to correct level

    @return Open PWM status
		- @b E_OK: Success
		- @b Others: Close PWM failed
*/
static ER pwm_close_internal(UINT32 ui_pwm_id, BOOL b_wait)
{
	UINT32 ui_enum_pwm_id;
	ER er_return;
	T_PWM_ENABLE pwm_enable;
	T_PWM_CTRL_INTEN        pwm_ctrl_int_en;
	T_PWM_MS_CTRL_INTEN     ms_ctrl_int_en;
	T_PWM_CCNT_CTRL_INTEN   ccnt_ctrl_int_en;
	T_PWM_CCNT_CTRL_INTSTS  ccnt_ctrl_int_sts;

	ui_enum_pwm_id = pwm_is_valid_id(0, ui_pwm_id);
	if (ui_enum_pwm_id == PWM_INVALID_ID) {
		return E_PAR;
	}

	if (ui_enum_pwm_id >= PWMID_CCNT_NO_0 && ui_enum_pwm_id <= PWMID_CCNT_NO_2) {
		pwm_ccnt_disable((ui_enum_pwm_id - PWMID_CCNT_NO_0));
		ccnt_ctrl_int_en.reg = PWM_GETREG(PWM_CCNT_CTRL_INTEN_OFS);
		ccnt_ctrl_int_sts.reg = PWM_GETREG(PWM_CCNT_CTRL_INTEN_OFS);
		ccnt_ctrl_int_en.reg &= ~((PWM_INT_CCNT0_EDGE_STS | PWM_INT_CCNT0_TRIG_STS) << ((ui_enum_pwm_id - PWMID_CCNT_NO_0) * 4));
		ccnt_ctrl_int_sts.reg |= ((PWM_INT_CCNT0_EDGE_STS | PWM_INT_CCNT0_TRIG_STS) << ((ui_enum_pwm_id - PWMID_CCNT_NO_0) * 4));
		PWM_SETREG(PWM_CCNT_CTRL_INTEN_OFS, ccnt_ctrl_int_en.reg);
		PWM_SETREG(PWM_CCNT_CTRL_INTSTS_OFS, ccnt_ctrl_int_sts.reg);
	} else {

		// Stop at close
		if (b_wait == TRUE) {
			pwm_enable.reg = PWM_GETREG(PWM_ENABLE_OFS);
			if (pwm_enable.reg & (1 << ui_enum_pwm_id)) {
#if (PWM_CLOSE_TRIG_DGPIO == ENABLE)
				gpio_setPin(PWM_CLOSE_GPIO);
#endif
				er_return = pwm_pwm_disable(ui_pwm_id);
				if (er_return != E_OK) {
					return er_return;
				}
			}
			pwm_wait(ui_pwm_id, PWM_TYPE_PWM);
#if (PWM_CLOSE_TRIG_DGPIO == ENABLE)
			gpio_clearPin(PWM_CLOSE_GPIO);
#endif
		} else {
			pwm_enable.reg = PWM_GETREG(PWM_ENABLE_OFS);
			if (pwm_enable.reg & (1 << ui_enum_pwm_id)) {
				er_return = pwm_pwm_disable(ui_pwm_id);
				if (er_return != E_OK) {
					return er_return;
				}
			}
		}

		// Only enable specific id (opened)
		// Enable specific PWM interrupts
		pwm_ctrl_int_en.reg = PWM_GETREG(PWM_CTRL_INTEN_OFS);
		pwm_ctrl_int_en.reg &= ~(1 << ui_enum_pwm_id);
		PWM_SETREG(PWM_CTRL_INTEN_OFS, pwm_ctrl_int_en.reg);

		// Enable specific MS interrupts
		ms_ctrl_int_en.reg = PWM_GETREG(PWM_MS_CTRL_INTEN_OFS);
		ms_ctrl_int_en.reg &= ~(1 << ui_enum_pwm_id);
		PWM_SETREG(PWM_MS_CTRL_INTEN_OFS, ms_ctrl_int_en.reg);
	}

	// Check if all PWMs/CCNT are disabled
	if ((UINT32)pwm_lock_status == ui_pwm_id) {
		// Disable interrupt
		pwm_platform_int_disable();
		// Disable PWM module clock
		//debug_msg("last time close PWM\r\n");
	}
	// Disable clock and switch pinmux back to GPIO
	pwm_detach(ui_enum_pwm_id);

	// Release PWM resource
	return pwm_unlock(ui_enum_pwm_id);
}

/**
    @addtogroup mIDrvIO_PWM
*/
//@{

/**
    @name PWM public driver API
*/
//@{
/**
    Open specific pwm channel.

    If want to access pwm channel, need call pwm open API, will\n
    lock related semphare, enable clock and interrupt.

    @note   pwm_open do not switch pinmux(config at top API), only obtain semphare,\n
			and enable clock

    @param[in] ui_pwm_id  pwm id, one id at a time
    @return Open PWM status
		- @b E_OK: Success
		- @b Others: Open PWM failed
*/
ER pwm_open(UINT32 ui_pwm_id)
{
    T_PWM_CTRL_INTEN        pwm_ctrl_int_en;

    pwm_ctrl_int_en.reg = PWM_GETREG(PWM_CTRL_INTEN_OFS);
    if (pwm_ctrl_int_en.reg & ui_pwm_id) {
        //DBG_ERR("ch[%2ld] is already open\r\n", ui_pwm_id);
        return E_PAR;
    }

	pwm_set_destination(ui_pwm_id, pwm_open_dest);

	return pwm_open_internal(ui_pwm_id);
}


/**
    Open specific pwm micro step by set unit

    where PWM0-3 : set 0, PWM4-7 : set 1, PWM8-11 : set 2, PWM12-15 : set 3

    @note pwm_open_set do not switch pinmux(config at top API)

    @param[in] ui_ms_set  pwm set id, set0:PWM0-3, set1:PWM4-7, set2:PWM8-11, set3:PWM12-15
    @return Open PWM set status
		- @b E_OK: Success
		- @b Others: Open PWM set failed
*/
ER pwm_open_set(PWM_MS_CHANNEL_SET ui_ms_set)
{
	UINT32 ui_set_cnt;

	for (ui_set_cnt = (ui_ms_set * 4); ui_set_cnt < ((ui_ms_set + 1) * 4); ui_set_cnt++) {
		if (pwm_open_internal(1 << ui_set_cnt) != E_OK) {
			return E_SYS;
		}

		// set isr destination
		pwm_set_destination(1 << ui_set_cnt, pwm_open_dest);
	}
	return E_OK;
}


/**
    Close specific pwm channel.

    Let other tasks to use specific pwm channel, including set specific channel\n
    disable (finish at an end of cycle). Disable clock, and unlock semphare

    @note Because of pwm_close() include pwm_pwm_disable(), so MUST called behind\n
		pwm_pwm_enable(). In other word, it's illegal that called pwm_close(pwm_ch, TRUE)\n
		without calling pwm_pwnEnable(pwm_ch).

    @param[in] ui_pwm_id                  pwm id, one id at a time(PWMID_0, PWMID_1 ... PWMID_19)
    @param[in] b_wait_auto_disable_done     wait a complete cycle done or not after pwm disable completed.\n
											(system will halt once if close a disabled PWM channel.)

    Example:
    @code
    {
		pwm_open(pwm_ch_x);
		pwm_pwm_config(pwm_ch_x, xxx);
		pwm_pwm_enable(pwm_ch_x);

		//case1:
		pwm_pwm_disable(pwm_ch_x);
		//Need wait disable done
		pwm_wait(pwm_ch_x);
		//Fill FALSE due to already disable done
		pwm_close(pwm_ch_x, FALSE);

		//case2:
		//Fill TRUE if pwm_close called directly
		pwm_close(pwm_ch_x, TRUE);

		//Flow of case1 are equal to case2
    }
    @endcode
    @return Open PWM status
		- @b E_OK: Success
		- @b Others: Close PWM failed
*/
ER pwm_close(UINT32 ui_pwm_id, BOOL b_wait_auto_disable_done)
{
	return pwm_close_internal(ui_pwm_id, b_wait_auto_disable_done);
}


/**
    Close micro step by set.

    Let other tasks to use specific pwm channel, include set specific \n
    channel disable (finish at an end of cycle). Disable clock, and \n
    unlock semphare

    @param[in] ui_ms_set                  pwm set id, set0:pwm0-3, set1:pwm4-7, set2:pwm8-11, set3:pwm12-15
    @param[in] b_wait_auto_disable_done     wait a complete cycle done or not after pwm disable completed.\n
											(system will halt once if close a disabled PWM channel within ui_ms_set)

    @return Open PWM status
		- @b E_OK: Success
		- @b Others: Close PWM failed
*/
ER pwm_close_set(PWM_MS_CHANNEL_SET ui_ms_set, BOOL b_wait_auto_disable_done)
{
	UINT32      ui_set_cnt;

	for (ui_set_cnt = (ui_ms_set * 4); ui_set_cnt < (((ui_ms_set + 1) * 4) - 1); ui_set_cnt++) {
		pwm_close_internal((1 << ui_set_cnt), TRUE);
	}
	return pwm_close_internal((1 << (((ui_ms_set + 1) * 4) - 1)), b_wait_auto_disable_done);
}


/**
    Get cycle count number.

    Get PWM(or micro step) cycle count operation number

    @param ui_pwm_id      PWM ID (from PWMID_CCNT_NO_0 ~ PWMID_CCNT_NO_3)

    @return Set cycle count parameter status
		- @b Cycle count
*/
UINT32 pwm_pwm_get_cycle_number(UINT32 ui_pwm_id)
{
	UINT32 ui_enum_pwm_id;
	T_PWM_CTRL_REG_BUF  ui_ctrl_reg;

	ui_enum_pwm_id = pwm_is_valid_id(0, ui_pwm_id);
	if (ui_enum_pwm_id == PWM_INVALID_ID) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	if (pwm_is_valid_id(ui_enum_pwm_id + 1, ui_pwm_id) != PWM_INVALID_ID) {
		DBG_ERR("more than 1 channel to wait done at one time\r\n");
		return E_SYS;
	}

	if (ui_enum_pwm_id < 16) {
		ui_ctrl_reg.reg = PWM_GETREG(PWM_CTRL_REG_BUF_OFS + ui_enum_pwm_id * PWM_PER_CH_OFFSET);
	} else {
		ui_ctrl_reg.reg = PWM_GETREG(PWM_CTRL_REG_BUF_OFS + ui_enum_pwm_id * PWM_PER_CH_OFFSET + 0x20);
	}

	return (UINT32)ui_ctrl_reg.bit.pwm_on;
}


/**
    Set PWM clock divided by micro step set unit(for micro step usage)

    Set clock divid by micro step set unit where\n
    set0:pwm0-3, set1:pwm4-7, set2:pwm8-11, set3:pwm12-15

    @param[in] ui_ms_set  pwm set id, set0:pwm0-3, set1:pwm4-7, set2:pwm8-11, set3:pwm12-15
    @param[in] ui_div    PWM divid
    @return Set cycle count parameter status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_mstep_config_clock_div(PWM_MS_CHANNEL_SET ui_ms_set, UINT32 ui_div)
{
	UINT32  ui_offset;
	ER      er_return = E_OK;

	//#NT#2015/10/19#Steven Wang -begin
	//#NT#43554 => ui_ms_set >= PWM_MS_SET_0 always true
	//if(ui_ms_set >= PWM_MS_SET_0 && ui_ms_set <= PWM_MS_SET_2)

	if (ui_ms_set <= PWM_MS_SET_1)
		//#NT#2015/10/19#Steven Wang -end
	{
		ui_offset = (ui_ms_set * 4);
		pwm_platform_set_clk_rate(ui_offset, ui_div);
	} else {
		er_return = E_PAR;
	}

	return er_return;
}


/**
    Set PWM clock divided by micro step set unit(for micro step usage)

    Set clock divid by micro step set unit where\n
    set0:pwm0-3, set1:pwm4-7, set2:pwm8-11, set3:pwm12-15

    @param[in] ui_clk_src     pwm clock divided source
    @param[in] ui_div        PWM divid

    @note for PWM_CLOCK_DIV
    @return Set cycle count parameter status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_pwm_config_clock_div(PWM_CLOCK_DIV ui_clk_src, UINT32 ui_div)
{
	UINT32 id = 0;

	switch (ui_clk_src) {
	case PWM0_3_CLKDIV:
		if ((PWM_GETREG(PWM_ENABLE_OFS) & (PWMID_0 | PWMID_1 | PWMID_2 | PWMID_3)) != 0) {
			DBG_ERR("PWM0~3 are using same clock div, make sure PWM0~3 are all disabled\r\n");
			return E_SYS;
		}

		id = pwm_is_valid_id(0, PWMID_0);
		if (id == PWM_INVALID_ID) {
			DBG_ERR("invalid ID!\r\n");
			return E_PAR;		
		}		
		break;

	case PWM4_7_CLKDIV:
		if ((PWM_GETREG(PWM_ENABLE_OFS) & (PWMID_4 | PWMID_5 | PWMID_6 | PWMID_7)) != 0) {
			DBG_ERR("PWM4~7 are using same clock div, make sure PWM4~7 are all disabled\r\n");
			return E_SYS;
		}

		id = pwm_is_valid_id(4, PWMID_4);
		if (id == PWM_INVALID_ID) {
			DBG_ERR("invalid ID!\r\n");
			return E_PAR;		
		}		
		break;

	case PWM8_CLKDIV:
	case PWM9_CLKDIV:
	case PWM10_CLKDIV:
	case PWM11_CLKDIV:
		if (pwm_pwm_is_en(1 << ui_clk_src) == TRUE) {
			DBG_ERR("make sure PWM%d are disabled\r\n", ui_clk_src);
			return E_SYS;
		}

		id = ui_clk_src;
		break;
	default:
		DBG_ERR("Un know clock source \r\n");
		return E_SYS;
	}

	if (id > PWM_ALLCH_BITS) {
		return E_ID;
	}

	pwm_platform_set_clk_rate(id, ui_div);

	return E_OK;
}



/**
    Wait operation done

    Calling this API to wait until the operation of target channel is done

    @param[in] ui_pwm_id      PWM ID, one ID at a time
    @param[in] ui_pwm_type    determine is PWM, CCNT or micro step, wait different flag
    @return Operation status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
		- @b PWM_CCNT_EDGE_TRIG : (if ui_pwm_type = PWM_TYPE_CCNT),represent CCNT edge trigger event occurred.
		- @b PWM_CCNT_TAGT_TRIG : (if ui_pwm_type = PWM_TYPE_CCNT),represent CCNT target value arrived trigger
		- @b PWM_CCNT_TOUT_TRIG : (if ui_pwm_type = PWM_TYPE_CCNT),represent CCNT time out trigger
*/
ER pwm_wait(UINT32 ui_pwm_id, PWM_TYPE ui_pwm_type)
{
	FLGPTN  flag = 0, ptn = 0;
	UINT32  ui_flag_group;
	UINT32  ui_enum_pwm_id;
	INT32   ui_result = E_OK;

	if (ui_pwm_type == PWM_TYPE_PWM) {
		ui_flag_group = 0;
	} else {
		ui_flag_group = 1;
	}

	ui_enum_pwm_id = pwm_is_valid_id(0, ui_pwm_id);
	if (ui_enum_pwm_id == PWM_INVALID_ID) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	if (pwm_is_valid_id(ui_enum_pwm_id + 1, ui_pwm_id) != PWM_INVALID_ID) {
		DBG_ERR("more than 1 channel to wait done at one time\r\n");
		return E_SYS;
	}

	// PWM need to be opened before
	if (!(pwm_lock_status & (1 << ui_enum_pwm_id))) {
		DBG_ERR("not opened yet!\r\n");
		return E_PAR;
	}
	// PWM0 ~ PWM11
	if (ui_enum_pwm_id <= PWMID_NO_11) {
		if (ui_flag_group == 0) {
			ptn |= (FLGPTN_PWM0 << ui_enum_pwm_id);
		} else if (ui_flag_group == 1) {
			ptn |= (FLGPTN_PWM_MS0 << ui_enum_pwm_id);
		} else {
			DBG_ERR("unknow flag group 0x%08x", (unsigned int)ui_flag_group);
			return E_SYS;
		}

		if (pwm_pwm_is_en(ui_pwm_id))
			pwm_platform_flg_wait(&flag, ui_flag_group, ptn);
		else
			DBG_WRN("pwm %d is not enable!\r\n", (int)ui_enum_pwm_id);
	} else {
		//ptn |= (FLGPTN_PWM_CCNT0_EDGE | FLGPTN_PWM_CCNT0_TRIG | FLGPTN_PWM_CCNT0_TOUT) << ((ui_enum_pwm_id - PWMID_CCNT_NO_0) * 3);
        ptn |= (FLGPTN_PWM_CCNT0_EDGE | FLGPTN_PWM_CCNT0_TRIG | FLGPTN_PWM_CCNT0_TOUT);

		if (pwm_ccnt_is_en(ui_pwm_id))
			pwm_platform_flg_wait(&flag, ui_flag_group, ptn);
		else
			DBG_WRN("pwm %d is not enable!\r\n", (int)ui_enum_pwm_id);
	}
	//DBG_ERR("!! %d %d\r\n", (unsigned int)ui_flag_group, (unsigned int)ptn);
	//pwm_platform_flg_wait(&flag, ui_flag_group, ptn);
	//CCNT
	if (ui_enum_pwm_id > PWMID_NO_11) {
		if (flag & (FLGPTN_PWM_CCNT0_EDGE)) {
			ui_result |= PWM_CCNT_EDGE_TRIG;
		}

		if (flag & (FLGPTN_PWM_CCNT0_TRIG)) {
			ui_result |= PWM_CCNT_TAGT_TRIG;
		}

		if (flag & (FLGPTN_PWM_CCNT0_TOUT)) {
			ui_result |= PWM_CCNT_TOUT_TRIG;
		}

	}
	pwm_platform_flg_clear(ui_flag_group, flag);

	return ui_result;
}



/**
    Wait operation done by set

    Calling this API to wait until the operation of target channel is done

    @param[in] ui_ms_set  pwm set id, set0:pwm0-3, set1:pwm4-7, set2:pwm8-11, set3:pwm12-15

    @return Open PWM status
		- @b E_OK: Success
		- @b Others: Close PWM failed
*/
ER pwm_wait_set(PWM_MS_CHANNEL_SET ui_ms_set)
{
	UINT32      ui_set_cnt;

	ui_set_cnt = (ui_ms_set * 4);
	pwm_wait((1 << ui_set_cnt), PWM_TYPE_MICROSTEP);
	return pwm_platform_flg_clear(1, (1 << (ui_set_cnt + 1)) | (1 << (ui_set_cnt + 2)) | (1 << (ui_set_cnt + 3)));
}


/**
    Wait micro step clock load done

    After micro step clock load done,  this API to wait until the operation of target channel is done

    @param[in] ui_pwm_clk_div_src   clock div source(only valid @ div_0_3 / div_4_7 / div_8_11
    @return Operation status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_mstep_clk_div_wait_load_done(PWM_CLOCK_DIV ui_pwm_clk_div_src)
{
	FLGPTN  flag = 0, ptn = 0;

	if (ui_pwm_clk_div_src > PWM4_7_CLKDIV) {
		DBG_ERR("Invalid PWM clock div src only 0-3/4-7/ can be load during operation\r\n");
		return E_PAR;
	}

	ptn |= (FLGPTN_PWM_00_03_CLKDIV_LOAD_DONE << ui_pwm_clk_div_src);


	pwm_platform_flg_wait(&flag, 2, ptn);


	pwm_platform_flg_clear(2, flag);

	return E_OK;
}


/**
    Wait micro step target count arrived

    Micro step target count engine arrived target count value

    @param[in] ui_target_cnt      clock div source(only valid @ div_0_3 / div_4_7
    @return Operation status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_mstep_target_count_wait_done(PWM_TGT_COUNT ui_target_cnt)
{
	FLGPTN  flag = 0, ptn = 0;

	if (ui_target_cnt >= PWM_TGT_CNT_NUM) {
		DBG_ERR("Invalid PWM clock div src only 0-3/4-7 can be load during operation\r\n");
		return E_PAR;
	}

	ptn |= (FLGPTN_PWM_00_03_TGT_CNT_DONE << ui_target_cnt);

	pwm_platform_flg_wait(&flag, 2, ptn);

	pwm_platform_flg_clear(2, flag);

	return E_OK;
}

//@}


/**
    @name PWM pwm function related API
*/
//@{
/**
    Enable(Start) PWM

    Start PWM based on PWM parameter descriptor set by pwm_set().

    @param[in] ui_pwm_id PWM ID, could be OR's of any PWM ID

    @return Start status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_pwm_enable(UINT32 ui_pwm_id)
{
	T_PWM_ENABLE    pwm_ctrl_en;
	// PWM need to be opened before
	if ((pwm_lock_status & ui_pwm_id) != ui_pwm_id) {
		DBG_ERR("pwm_pwm_enable(), not opened yet!\r\n");
		return E_PAR;
	}

	if (PWM_GETREG(PWM_ENABLE_OFS) & ui_pwm_id) {
		DBG_ERR("pwm_pwm_enable(), do not re enable same channel!! (%x)\r\n", (unsigned int)ui_pwm_id);
		return E_PAR;
	}
	pwm_ctrl_en.reg = ui_pwm_id;
	PWM_SETREG(PWM_ENABLE_OFS, pwm_ctrl_en.reg);

	// Because enable bit is in APB clock domain. Need double sync to PWM clock
	// domain. So after PWM enable bit set => read until 1(represent PWM clock domain is active)
	while ((PWM_GETREG(PWM_ENABLE_OFS) & ui_pwm_id) != ui_pwm_id) {
		;
	}
	return E_OK;
}


/**
    Disable(stop) PWM

    Disable PWM if it is running in free run mode.

    @param[in] ui_pwm_id PWM ID, one ID at a time
    @return Stop status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_pwm_disable(UINT32 ui_pwm_id)
{
	T_PWM_DISABLE pwm_stop;

	// PWM should be opened
	if ((pwm_lock_status & ui_pwm_id) != ui_pwm_id) {
		DBG_ERR("not opened yet!\r\n");
		return E_PAR;
	}

	// Disable PWM
	if (PWM_GETREG(PWM_ENABLE_OFS) & ui_pwm_id) {
		pwm_stop.reg = ui_pwm_id;
		PWM_SETREG(PWM_DISABLE_OFS, pwm_stop.reg);
		return E_OK;
	}
	DBG_ERR("pwm_pwm_disable(), do not disable the channel not enable yet!! (%x)\r\n", (unsigned int)ui_pwm_id);
	return E_PAR;


}



/**
    Set PWM parameters.

    This function sets PWM duty cycles, repeat count and signal level.\n
    After set, the specified pwm channel can be started and stopped by API\n
    functions pwm_pwm_enable() and pwm_pwm_disable(). If the on cycle is not PWM_FREE_RUN,\n
    PWM will issue an interrupt after all cycles are done.

    @param[in] ui_pwm_id  pwm id, one id at a time
    @param[in] p_pwm_cfg  PWM parameter descriptor

    @return Set parameter status.
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID
*/
ER pwm_pwm_config(UINT32 ui_pwm_id, PPWM_CFG p_pwm_cfg)
{
	UINT32 ui_offset;
	T_PWM_CTRL_REG_BUF          reg_pwm_ctrl_buf;
	T_PWM_PERIOD_REG_BUF        reg_pwm_period_buf;
	T_PWM_PERIOD_REG_BUF        org_pwm_period_buf;
	T_PWM_EXPEBD_PERIOD_REG_BUF exp_period_buf = {0};


	reg_pwm_ctrl_buf.reg = 0;
	reg_pwm_period_buf.reg = 0;

	ui_offset = pwm_is_valid_id(0, ui_pwm_id);
	if (ui_pwm_id == PWMID_CCNT0 || ui_pwm_id == PWMID_CCNT1 || ui_pwm_id == PWMID_CCNT2 ||
		ui_offset == PWM_INVALID_ID) {
		DBG_ERR("invalid PWM ID 0x%08x\r\n", (unsigned int)ui_pwm_id);
		return E_PAR;
	}



	org_pwm_period_buf.reg = PWM_GETREG(PWM_PWM0_PRD_OFS + ui_offset * PWM_PER_CH_OFFSET);




	if (p_pwm_cfg->ui_prd < 2) {
		DBG_ERR("invalid PWM base period %d MUST 2~255\r\n", (int)p_pwm_cfg->ui_prd);
		return E_PAR;
	}


	//pll_setPWMClockRate(ui_offset, p_pwm_cfg->ui_div);

	reg_pwm_ctrl_buf.bit.pwm_on     = p_pwm_cfg->ui_on_cycle;
	reg_pwm_ctrl_buf.bit.pwm_type   = PWM_TYPE_PWM;
	reg_pwm_period_buf.bit.pwm_r    = p_pwm_cfg->ui_rise;
	reg_pwm_period_buf.bit.pwm_f    = p_pwm_cfg->ui_fall;
	reg_pwm_period_buf.bit.pwm_prd  = p_pwm_cfg->ui_prd;
	reg_pwm_period_buf.bit.pwm_inv  = p_pwm_cfg->ui_inv;

	if (ui_offset < 8) {
		exp_period_buf.reg = PWM_GETREG(PWM0_EXPEND_PERIOD_REG_BUF_OFS + (ui_offset << 2));
		exp_period_buf.bit.pwm_r    = (p_pwm_cfg->ui_rise >> 8) & 0xFF;
		exp_period_buf.bit.pwm_f    = (p_pwm_cfg->ui_fall >> 8) & 0xFF;
		exp_period_buf.bit.pwm_prd  = (p_pwm_cfg->ui_prd >> 8) & 0xFF;
		PWM_SETREG(PWM0_EXPEND_PERIOD_REG_BUF_OFS + (ui_offset << 2), exp_period_buf.reg);

	}

	if (org_pwm_period_buf.bit.pwm_inv != p_pwm_cfg->ui_inv && pwm_pwm_is_en(ui_pwm_id)) {
		DBG_ERR("Try to change Invert signal but PWM [%d] is enabled already\r\n", (int)pwm_is_valid_id(0, ui_pwm_id));
		return E_PAR;
	}

	PWM_SETREG(PWM_PWM0_PRD_OFS + ui_offset * PWM_PER_CH_OFFSET, reg_pwm_period_buf.reg);
	PWM_SETREG(PWM_PWM0_CTRL_OFS + ui_offset * PWM_PER_CH_OFFSET, reg_pwm_ctrl_buf.reg);

	return E_OK;
}


/**
    Enable PWM by micro step set group

    Enable PWM by micro step set group

    @param[in] ui_ms_set  pwm set id, set0:pwm0-3, set1:pwm4-7, set2:pwm8-11, set3:pwm12-15

    @note for pwm_pwm_enable()

    @return Start status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_pwm_enable_set(PWM_MS_CHANNEL_SET ui_ms_set)
{
	UINT32  ui_pwm_en_bit;
	UINT32  ui_set_cnt;

	ui_pwm_en_bit = 0;
	if (ui_ms_set >= PWM_MS_SET_TOTAL) {
		DBG_ERR("set error\r\n");
		return E_PAR;
	}

	for (ui_set_cnt = (ui_ms_set * 4); ui_set_cnt < ((ui_ms_set + 1) * 4); ui_set_cnt++) {
		ui_pwm_en_bit |= (1 << ui_set_cnt);
	}

	return pwm_pwm_enable(ui_pwm_en_bit);
}


/**
    Disable PWM by micro stepping set group

    Disable PWM by micro step set group

    @param[in] ui_ms_set  pwm set id, set0:pwm0-3, set1:pwm4-7, set2:pwm8-11, set3:pwm12-15

    @note for pwm_pwm_enable()

    @return Start status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_pwm_disable_set(PWM_MS_CHANNEL_SET ui_ms_set)
{
	UINT32  ui_pwm_en_bit;
	UINT32  ui_set_cnt;
	ER      u_ret = E_OK;

	ui_pwm_en_bit = 0;
	if (ui_ms_set >= PWM_MS_SET_TOTAL) {
		DBG_ERR("set error\r\n");
		return E_PAR;
	}

	for (ui_set_cnt = (ui_ms_set * 4); ui_set_cnt < ((ui_ms_set + 1) * 4); ui_set_cnt++) {
		ui_pwm_en_bit |= (1 << ui_set_cnt);
	}

	u_ret = pwm_pwm_disable(ui_pwm_en_bit);

	if (u_ret != E_OK) {
		return u_ret;
	}

	return pwm_wait(1 << (ui_ms_set * 4), PWM_TYPE_PWM);
}


/**
    Set PWM reload parameters.

    During pwm is active, rising and falling time and base period can be changed \n
    without disable pwm\n

    @param[in] ui_pwm_id      pwm id, one id at a time
    @param[in] i_rise        rising time (-1 if not modified)
    @param[in] i_fall        falling time(-1 if not modified)
    @param[in] i_base_period  base period (-1 if not modified)


    @return Set parameter status.
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID
*/
ER pwm_pwm_reload_config(UINT32 ui_pwm_id, INT32 i_rise, INT32 i_fall, INT32 i_base_period)
{
	UINT32 ui_offset;
	T_PWM_PERIOD_REG_BUF        reg_pwm_period_buf;
	T_PWM_EXPEBD_PERIOD_REG_BUF exp_period_buf = {0};


	reg_pwm_period_buf.reg = 0;

	ui_offset = pwm_is_valid_id(0, ui_pwm_id);
	if (ui_pwm_id == PWMID_CCNT0 || ui_pwm_id == PWMID_CCNT1 || ui_pwm_id == PWMID_CCNT2 ||
		ui_offset == PWM_INVALID_ID) {
		DBG_ERR("invalid PWM ID 0x%08x\r\n", (unsigned int)ui_pwm_id);
		return E_PAR;
	}


	reg_pwm_period_buf.reg = PWM_GETREG(PWM_PWM0_PRD_OFS + ui_offset * PWM_PER_CH_OFFSET);


	if (i_base_period < 2 && i_base_period >= 0) {
		DBG_ERR("invalid PWM base period %d MUST 2~255\r\n", (int)i_base_period);
		return E_PAR;
	}

	if (i_rise >= 0) {
		reg_pwm_period_buf.bit.pwm_r = i_rise;
	}

	if (i_fall >= 0) {
		reg_pwm_period_buf.bit.pwm_f = i_fall;
	}

	if (i_base_period >= 2) {
		reg_pwm_period_buf.bit.pwm_prd  = i_base_period;
	}

	PWM_SETREG(PWM_PWM0_PRD_OFS + ui_offset * PWM_PER_CH_OFFSET, reg_pwm_period_buf.reg);

	if (ui_offset < 8) {
		exp_period_buf.reg = PWM_GETREG((PWM0_EXPEND_PERIOD_REG_BUF_OFS + (ui_offset << 2)));
		exp_period_buf.bit.pwm_r    = (i_rise >= 0) ? ((i_rise >> 8) & 0xFF) : exp_period_buf.bit.pwm_r;
		exp_period_buf.bit.pwm_f    = (i_fall >= 0) ? ((i_fall >> 8) & 0xFF) : exp_period_buf.bit.pwm_f;
		exp_period_buf.bit.pwm_prd  = (i_base_period >= 2) ? ((i_base_period >> 8) & 0xFF) : exp_period_buf.bit.pwm_prd;
		PWM_SETREG((PWM0_EXPEND_PERIOD_REG_BUF_OFS + (ui_offset << 2)), exp_period_buf.reg);
	}
	return E_OK;
}


/**
    Reload PWM

    Reload PWM based on PWM parameter descriptor set by pwm_set().

    @param[in] ui_pwm_id  PWM ID, could be OR's of any PWM ID

    @return Start status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_pwm_reload(UINT32 ui_pwm_id)
{
	T_PWM_LOAD pwm_load_reg;

	// PWM need to be opened before
	if ((pwm_lock_status & ui_pwm_id) != ui_pwm_id) {
		DBG_ERR("not opened yet!\r\n");
		return E_PAR;
	}

	if (pwm_pwm_is_en(ui_pwm_id) == FALSE) {
		DBG_ERR("pwm[0x%08x]not enable yet!\r\n", (unsigned int)ui_pwm_id);
		return E_SYS;
	}

	// Reload PWM
	pwm_load_reg.reg = PWM_GETREG(PWM_LOAD_OFS);
	pwm_load_reg.reg |= ui_pwm_id;

	// Avoid PWM channel(s) are under reloading(wait until done if under reloading)
	while ((PWM_GETREG(PWM_LOAD_OFS) & ui_pwm_id)) {
		;
	}
	PWM_SETREG(PWM_LOAD_OFS, pwm_load_reg.reg);



	return E_OK;
}


/**
    Reload CCNT

    Reload CCNTx based on CCNTx parameter descriptor set by pwm_ccnt_config().

    @note Only Trigger value(ui_trigger_value) and ui_count_mode(increase or decrease) at PPWM_CCNT_CFG\n
			can be modify while PWMx_ccntEnabled

    @param[in] ui_pwm_id  pwm id, must be PWMID_CCNTx, one id at a time(PWMID_CCNT0~PWMID_CCNT3)

    @return Start status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_ccnt_reload(UINT32 ui_pwm_id)
{
	T_PWM_CCNT_LOAD ccnt_load_reg = {0};
	UINT32 ui_offset;

	if (ui_pwm_id != PWMID_CCNT0 && ui_pwm_id != PWMID_CCNT1 && ui_pwm_id != PWMID_CCNT2) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	// Check PWM/CCNT ID
	ui_offset = pwm_is_valid_id(0, ui_pwm_id);

	if (ui_offset == PWM_INVALID_ID) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	// PWM need to be opened before
	if ((pwm_lock_status & ui_pwm_id) != ui_pwm_id) {
		DBG_ERR("not opened yet!\r\n");
		return E_PAR;
	}

	if (pwm_ccnt_is_en(ui_pwm_id) == FALSE) {
		DBG_ERR("ccnt[%d]not enable yet!\r\n", (int)(ui_offset - PWMID_CCNT_NO_0));
		return E_SYS;
	}

	// Fix coverity of bad shift
	if ((ui_offset - PWMID_CCNT_NO_0) > 31) {
		DBG_ERR("invalid PWM ID shift!\r\n");
		return E_PAR;
	}

	// Reload CCNT
	ccnt_load_reg.reg = PWM_GETREG(PWM_CCNT_LOAD_OFS);
	ccnt_load_reg.reg |= (1 << (ui_offset - PWMID_CCNT_NO_0));
	PWM_SETREG(PWM_CCNT_LOAD_OFS, ccnt_load_reg.reg);

	return E_OK;
}


/**
    Reload Micro step clock

    Reload clock divided during micro step operation

    @note Only valid under micro step mode

    @param[in] ui_pwm_clk_div  PWM clock divided source
    @param[in] b_wait_done    After load new clock divided, wait load done or not

    @return Start status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_mstep_clock_div_reload(PWM_CLOCK_DIV ui_pwm_clk_div, BOOL  b_wait_done)
{
	UINT32  ui_clk_div_start_ofs;
	T_PWM_CLKDIV_LOAD       pwm_clock_div_load_reg;
	T_PWM_CLKDIV_LOAD_INTEN pwm_clock_load_int_en_reg;
	T_PWM_CLKDIV_LOAD_STS   pwm_clock_load_int_en_sts;

	if (ui_pwm_clk_div > PWM4_7_CLKDIV) {
		return E_PAR;
	}

	ui_clk_div_start_ofs = (ui_pwm_clk_div << 2);

	if ((pwm_lock_status & (0xF << ui_clk_div_start_ofs)) != (0xF << ui_clk_div_start_ofs)) {
		DBG_ERR("PWM%d - %d not all opened yet!\r\n", ui_pwm_clk_div * 4, ui_pwm_clk_div * 4 + 3);
		return E_PAR;
	}


	if ((PWM_GETREG(PWM_ENABLE_OFS) & (0xF << ui_clk_div_start_ofs)) != (UINT32)(0xF << ui_clk_div_start_ofs)) {
		DBG_ERR("PWM%d - %d not all enable yet!\r\n", (int)ui_clk_div_start_ofs, (int)ui_clk_div_start_ofs + 3);
		return E_PAR;
	}

	// Clear load done status
	pwm_clock_load_int_en_sts.reg = PWM_GETREG(PWM_CLKDIV_LOAD_STS_OFS);
	pwm_clock_load_int_en_sts.reg |= (1 << ui_pwm_clk_div);
	PWM_SETREG(PWM_CLKDIV_LOAD_STS_OFS, pwm_clock_load_int_en_sts.reg);

	// Enable load done interrupt
	pwm_clock_load_int_en_reg.reg = PWM_GETREG(PWM_CLKDIV_LOAD_INTEN_OFS);
	pwm_clock_load_int_en_reg.reg |= (1 << ui_pwm_clk_div);
	PWM_SETREG(PWM_CLKDIV_LOAD_INTEN_OFS, pwm_clock_load_int_en_reg.reg);

	// Reload PWM
	pwm_clock_div_load_reg.reg = PWM_GETREG(PWM_CLKDIV_LOAD_OFS);
	pwm_clock_div_load_reg.reg |= (1 << ui_pwm_clk_div);

	// Avoid PWM channel(s) are under reloading(wait until done if under reloading)
	while ((PWM_GETREG(PWM_CLKDIV_LOAD_OFS) & (1 << ui_pwm_clk_div))) {
		;
	}
	PWM_SETREG(PWM_CLKDIV_LOAD_OFS, pwm_clock_div_load_reg.reg);

	if (b_wait_done) {
		pwm_mstep_clk_div_wait_load_done(ui_pwm_clk_div);
	}

	return E_OK;
}

//@}


/**
    @name PWM micro step function related API
*/
//@{
/**
    Start MSTP

    Start MSTP based on MSTP parameter descriptor set by pwm_ms_set().

    @param ui_pwm_id PWM ID, one or more ID at a time(bitwise)
    @return Start status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_mstep_enable(UINT32 ui_pwm_id)
{
	T_PWM_MS_START    pwm_ms_ctrl_en;
	// Enable PWM
	if (PWM_GETREG(PWM_MS_START_OFS) & ui_pwm_id) {
		DBG_ERR("pwm_mstep_enable(), do not re enable same channel!! (%x)\r\n", (unsigned int)ui_pwm_id);
		return E_PAR;
	}
	pwm_ms_ctrl_en.reg = ui_pwm_id;
	PWM_SETREG(PWM_MS_START_OFS, pwm_ms_ctrl_en.reg);
	//DBG_ERR("pwm_mstep_enable(), do not re enable same channel!! (%x)\r\n", en);

	while ((PWM_GETREG(PWM_MS_START_OFS) & ui_pwm_id) !=  ui_pwm_id) {
		;
	}
	return E_OK;
}


/**
    Stop micro step by specific channel

    Stop micro step if it is running in free run mode.

    @param[in] ui_pwm_id  PWM ID, one ID at a time
    @param[in] b_wait    Wait a complete cycle done or not after micro step stopped
    @return Stop status
		- @b E_OK: Success
*/
ER pwm_mstep_disable(UINT32 ui_pwm_id, BOOL b_wait)
{
	T_PWM_MS_STOP pwm_ms_stop;

	if (PWM_GETREG(PWM_MS_START_OFS) & ui_pwm_id) {
		pwm_ms_stop.reg = (ui_pwm_id);
		PWM_SETREG(PWM_MS_STOP_OFS, pwm_ms_stop.reg);
	} else {
		DBG_ERR("pwm_mstep_disable(), do not disable the channel not enable yet!! (%x)\r\n", (unsigned int)ui_pwm_id);
		return E_PAR;
	}


	if (b_wait) {
		pwm_wait(ui_pwm_id, FLG_ID_PWM2);
	}
	return E_OK;
}


/**
    Enable micro step by micro step set group

    Enable micro step by micro step set group

    @param[in] ui_ms_set  pwm set id, set0:pwm0-3, set1:pwm4-7, set2:pwm8-11, set3:pwm12-15

    @note for pwm_pwm_enable()

    @return Start status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_mstep_enable_set(PWM_MS_CHANNEL_SET ui_ms_set)
{
	UINT32  ui_pwm_en_bit;
	UINT32  ui_set_cnt;

	ui_pwm_en_bit = 0;
	if (ui_ms_set >= PWM_MS_SET_TOTAL) {
		DBG_ERR("set error\r\n");
		return E_PAR;
	}

	for (ui_set_cnt = (ui_ms_set * 4); ui_set_cnt < ((ui_ms_set + 1) * 4); ui_set_cnt++) {
		ui_pwm_en_bit |= (1 << ui_set_cnt);
	}

	return pwm_mstep_enable(ui_pwm_en_bit);
}


/**
    Set micro step parameters.

    This function sets Mirco step parameters.

    @param[in] ui_pwm_id          PWM ID, one ID at a time
    @param[in] p_mstp_cfg         Micro step control configuration

    @return Set micro step result
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or error configuration
*/
ER pwm_mstep_config(UINT32 ui_pwm_id, PMSTP_CFG p_mstp_cfg)
{
	UINT32 ui_offset;
	T_PWM_CTRL_REG_BUF      reg_pwm_ctrl;
	T_PWM_MS_EXT_REG_BUF    reg_pwm_ms_ctrl;
	UINT32                  ui_grad_per_step;
	UINT32                  ui_step_per_unit;

	reg_pwm_ms_ctrl.reg = 0;
	reg_pwm_ctrl.reg = 0;

	ui_offset = pwm_is_valid_id(0, ui_pwm_id);
	if (ui_pwm_id == PWMID_CCNT0 || ui_pwm_id == PWMID_CCNT1 || ui_pwm_id == PWMID_CCNT2 ||
		ui_offset == PWM_INVALID_ID) {
		DBG_ERR("invalid PWM ID 0x%08x\r\n", (unsigned int)ui_pwm_id);
		return E_PAR;
	}

	reg_pwm_ctrl.reg = PWM_GETREG(PWM_CTRL_REG_BUF_OFS + ui_offset * PWM_PER_CH_OFFSET);
	reg_pwm_ctrl.bit.pwm_on = 0;
	PWM_SETREG(PWM_CTRL_REG_BUF_OFS + ui_offset * PWM_PER_CH_OFFSET, reg_pwm_ctrl.reg);

	while (pwm_pwm_is_en(ui_pwm_id)) {
		reg_pwm_ctrl.reg = PWM_GETREG(PWM_CTRL_REG_BUF_OFS + ui_offset * PWM_PER_CH_OFFSET);
		if (reg_pwm_ctrl.bit.pwm_on == 1) {
			break;
		}
	}

	if (p_mstp_cfg->ui_step_per_phase % 8 != 0) {
		DBG_ERR("invalid PWM step per phase param %2d multiple of 8\r\n", (int)p_mstp_cfg->ui_step_per_phase);
		return E_PAR;
	}

	if (p_mstp_cfg->ui_ph > 7) {
		DBG_ERR("invalid PWM start phase param %d, 0~7\r\n", (int)p_mstp_cfg->ui_ph);
		return E_PAR;
	}

	if (p_mstp_cfg->is_square_wave) {
		if (p_mstp_cfg->ui_phase_type == PWM_MS_1_2_PHASE_TYPE) {
			reg_pwm_ctrl.bit.pwm_ms_threshold_en = TRUE;
			reg_pwm_ctrl.bit.pwm_ms_threshold = 38;
			reg_pwm_ms_ctrl.bit.pwm_ms_start_grads      = 33 + p_mstp_cfg->ui_ph * 64;
		} else {
			reg_pwm_ctrl.bit.pwm_ms_threshold_en = TRUE;
			reg_pwm_ctrl.bit.pwm_ms_threshold = 0;
			reg_pwm_ms_ctrl.bit.pwm_ms_start_grads      = 1 + p_mstp_cfg->ui_ph * 64;
		}
	} else {
		if(p_mstp_cfg->ui_threshold > 0x63) {
			DBG_ERR("invalid MS threshold %02d MUST less than 100\r\n", (int)p_mstp_cfg->ui_threshold);
			return E_PAR;
		}
		reg_pwm_ctrl.bit.pwm_ms_threshold_en = p_mstp_cfg->ui_threshold_en;
		reg_pwm_ctrl.bit.pwm_ms_threshold = p_mstp_cfg->ui_threshold;
		reg_pwm_ms_ctrl.bit.pwm_ms_start_grads      = p_mstp_cfg->ui_ph * 64;
	}

	reg_pwm_ctrl.bit.pwm_on                     = p_mstp_cfg->ui_on_cycle;
	reg_pwm_ctrl.bit.pwm_type                   = PWM_TYPE_MICROSTEP;
	reg_pwm_ctrl.bit.pwm_ms_dir                 = p_mstp_cfg->ui_dir;

	ui_grad_per_step = 64 / p_mstp_cfg->ui_step_per_phase;

	if (ui_grad_per_step == 1) {
		reg_pwm_ms_ctrl.bit.pwm_ms_grads_per_step = 0x0;
	} else if (ui_grad_per_step == 2) {
		reg_pwm_ms_ctrl.bit.pwm_ms_grads_per_step = 0x1;
	} else if (ui_grad_per_step == 4) {
		reg_pwm_ms_ctrl.bit.pwm_ms_grads_per_step = 0x2;
	} else if (ui_grad_per_step == 8) {
		reg_pwm_ms_ctrl.bit.pwm_ms_grads_per_step = 0x3;
	} else {
		DBG_ERR("invalid MS threshold %02d MUST less than 100\r\n", (int)p_mstp_cfg->ui_threshold);
		return E_PAR;
	}

	if (PWM_MS_1_2_PHASE_TYPE == p_mstp_cfg->ui_phase_type) {
		ui_step_per_unit = 64 / ui_grad_per_step;
	} else {
		ui_step_per_unit = (64 / ui_grad_per_step) * 2;
	}

	if (ui_step_per_unit == 8) {
		reg_pwm_ms_ctrl.bit.pwm_ms_step_per_unit = 0x3;
	} else if (ui_step_per_unit == 16) {
		reg_pwm_ms_ctrl.bit.pwm_ms_step_per_unit = 0x4;
	} else if (ui_step_per_unit == 32) {
		reg_pwm_ms_ctrl.bit.pwm_ms_step_per_unit = 0x5;
	} else if (ui_step_per_unit == 64) {
		reg_pwm_ms_ctrl.bit.pwm_ms_step_per_unit = 0x6;
	} else if (ui_step_per_unit == 128) {
		reg_pwm_ms_ctrl.bit.pwm_ms_step_per_unit = 0x7;
	} else {
		DBG_ERR("invalid ui_step_per_unit %d\r\n", (int)ui_step_per_unit);
	}

	PWM_SETREG(PWM_CTRL_REG_BUF_OFS + ui_offset * PWM_PER_CH_OFFSET, reg_pwm_ctrl.reg);
	PWM_SETREG(PWM_MS_EXT_REG_BUF_OFS + ui_offset * PWM_PER_CH_OFFSET, reg_pwm_ms_ctrl.reg);

	return E_OK;
}



/**
    Set micro step parameters config by set

    This function sets Mirco step parameters.

    @param[in] ui_ms_set              Micro set(4 sets total), PWM0-3(set0), PWM4-7(set1), PWM8-11(set2), PWM12-15(set3)
    @param[in] p_ms_set_ch_ph_cfg        Start phase of each channel in specific MS set(4 channel / per set)
    @param[in] p_ms_common_cfg         Common configuration of micro step

    @note for pwm_ms_set()

    @return Set micro step result
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or error configuration
*/
ER pwm_mstep_config_set(PWM_MS_CHANNEL_SET ui_ms_set, PMS_CH_PHASE_CFG p_ms_set_ch_ph_cfg, PMSCOMMON_CFG p_ms_common_cfg)
{
	UINT32      ui_set_cnt, ui_cnt;
	MSTP_CFG    ms_channel_cfg;


	if (ui_ms_set >= PWM_MS_SET_TOTAL) {
		DBG_ERR("invalid ui_ms_set %d\r\n", ui_ms_set);
	}

	ui_cnt = 0;
	ms_channel_cfg.ui_dir = p_ms_common_cfg->ui_dir;
	ms_channel_cfg.ui_on_cycle = p_ms_common_cfg->ui_on_cycle;
	ms_channel_cfg.ui_phase_type = p_ms_common_cfg->ui_phase_type;
	ms_channel_cfg.ui_step_per_phase = p_ms_common_cfg->ui_step_per_phase;
	ms_channel_cfg.is_square_wave = p_ms_common_cfg->is_square_wave;
	ms_channel_cfg.ui_threshold = p_ms_common_cfg->ui_threshold;
	ms_channel_cfg.ui_threshold_en = p_ms_common_cfg->ui_threshold_en;
	for (ui_set_cnt = (ui_ms_set * 4); ui_set_cnt < ((ui_ms_set + 1) * 4); ui_set_cnt++) {
		if (ui_cnt == 0) {
			ms_channel_cfg.ui_ph = p_ms_set_ch_ph_cfg->ui_ch0_phase;
		} else if (ui_cnt == 1) {
			ms_channel_cfg.ui_ph = p_ms_set_ch_ph_cfg->ui_ch1_phase;
		} else if (ui_cnt == 2) {
			ms_channel_cfg.ui_ph = p_ms_set_ch_ph_cfg->ui_ch2_phase;
		} else {
			ms_channel_cfg.ui_ph = p_ms_set_ch_ph_cfg->ui_ch3_phase;
		}

		if (pwm_mstep_config((1 << ui_set_cnt), &ms_channel_cfg) != E_OK) {
			DBG_ERR("Config Micro step channel %d fail\r\n", (int)ui_set_cnt);
			return E_SYS;
		}
		ui_cnt++;
	}
	return E_OK;
}


/**
    Disable micro step by set group

    Disable(stop) micro step by set if it is running in free run mode.

    @param[in] ui_ms_set  pwm set id, set0:pwm0-3, set1:pwm4-7, set2:pwm8-11, set3:pwm12-15
    @return Stop status
		- @b E_OK: Success
*/
ER pwm_mstep_disable_set(PWM_MS_CHANNEL_SET ui_ms_set)
{
	UINT32 ui_set_cnt;
	UINT32 ui_ms_bit = 0;
	T_PWM_MS_STOP pwm_ms_stop = {0};

	for (ui_set_cnt = (ui_ms_set * 4); ui_set_cnt < ((ui_ms_set + 1) * 4); ui_set_cnt++) {
		ui_ms_bit |= (1 << ui_set_cnt);
	}
	pwm_ms_stop.reg = ui_ms_bit;
	if (PWM_GETREG(PWM_MS_START_OFS) & ui_ms_bit) {
		PWM_SETREG(PWM_MS_STOP_OFS, pwm_ms_stop.reg);
	} else {
		DBG_ERR("pwm_mstep_disable_set(), do not disable the channel not enable yet!! (%x)\r\n", ui_ms_set);
		return E_PAR;
	}

	return pwm_wait((1 << (ui_ms_set * 4)), PWM_TYPE_MICROSTEP);
}


/**
    Set cycle target count number parameter.

    PWM controller can count down target step number

    @param[in] ui_target_cnt_set       Micro step target count set
    @param[in] ui_target_cnt          Micro step target count number
    @param[in] b_enable_target_cnt     Enable target count down action or not

    @note for PWM_TGT_COUNT
    @return Set cycle count parameter status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_mstep_config_target_count_enable(PWM_TGT_COUNT ui_target_cnt_set, UINT32 ui_target_cnt, BOOL b_enable_target_cnt)
{
	T_PWM_TGT_CNT_REG0      u_tgt_cnt_reg0 = {0};
	T_PWM_TGT_CNT_REG1      u_tgt_cnt_reg1 = {0};
	T_PWM_TGT_CNT_ENABLE    u_tgt_cnt_en_reg = {0};
	T_PWM_TGT_CNT_DISABLE   u_tgt_cnt_dis_reg = {0};
	T_PWM_CLKDIV_LOAD_INTEN u_tgt_cnt_int_en_reg = {0};

	if (!ui_target_cnt) {
		DBG_ERR("Target count should > 0\r\n");
		return E_PAR;
	}

	u_tgt_cnt_en_reg.reg = PWM_GETREG(PWM_TGT_CNT_ENABLE_OFS);

	if (b_enable_target_cnt) {
		if ((u_tgt_cnt_en_reg.reg & (1 << ui_target_cnt_set)) == (UINT32)(1 << ui_target_cnt_set)) {
			DBG_ERR("Target count set [%d] already enabled\r\n", ui_target_cnt_set);
			return E_PAR;
		}
	} else {
		if ((u_tgt_cnt_en_reg.reg & (1 << ui_target_cnt_set)) != (UINT32)(1 << ui_target_cnt_set)) {
			DBG_ERR("Target count set [%d] already disabled\r\n", ui_target_cnt_set);
			return E_PAR;
		}
		u_tgt_cnt_dis_reg.reg |= (1 << ui_target_cnt_set);
		PWM_SETREG(PWM_TGT_CNT_DISABLE_OFS, u_tgt_cnt_dis_reg.reg);
		return E_OK;
	}

	u_tgt_cnt_reg0.reg = PWM_GETREG(PWM_TGT_CNT_REG0_OFS);
	u_tgt_cnt_reg1.reg = PWM_GETREG(PWM_TGT_CNT_REG1_OFS);
	u_tgt_cnt_int_en_reg.reg = PWM_GETREG(PWM_CLKDIV_LOAD_INTEN_OFS);

	switch (ui_target_cnt_set) {
	case PWM_00_03_TGT_CNT:
		u_tgt_cnt_reg0.bit.pwm00_03_target_cnt = ui_target_cnt;
		PWM_SETREG(PWM_TGT_CNT_REG0_OFS, u_tgt_cnt_reg0.reg);
		u_tgt_cnt_en_reg.bit.pwm00_03_target_cnt_en = 1;
		u_tgt_cnt_int_en_reg.bit.clkdiv_00_03_tgt_cnt_inten = 1;

		break;

	case PWM_04_07_TGT_CNT:
		u_tgt_cnt_reg1.bit.pwm04_07_target_cnt = ui_target_cnt;
		PWM_SETREG(PWM_TGT_CNT_REG1_OFS, u_tgt_cnt_reg1.reg);
		u_tgt_cnt_en_reg.bit.pwm04_07_target_cnt_en = 1;
		u_tgt_cnt_int_en_reg.bit.clkdiv_04_07_tgt_cnt_inten = 1;
		break;

	default:
		return E_SYS;
	}
	PWM_SETREG(PWM_CLKDIV_LOAD_INTEN_OFS, u_tgt_cnt_int_en_reg.reg);
	PWM_SETREG(PWM_TGT_CNT_ENABLE_OFS, u_tgt_cnt_en_reg.reg);

	while (!(PWM_GETREG(PWM_TGT_CNT_ENABLE_OFS) & (1 << ui_target_cnt_set))) {
		;
	}

	return E_OK;
}



//@}

/**
    @name PWM cycle count (ccnt) function related API
*/
//@{

/**
    Start CCNT

    Start cycle count operation.

    @param[in] ui_pwm_id cycle count PWM ID, one ID at a time(PWMID_CCNT0~PWMID_CCNT3)
    @return Start status
		- @b E_OK: Success
*/
ER pwm_ccnt_enable(UINT32 ui_pwm_id)
{
	T_PWM_CCNT_ENABLE pwm_ccnt_en;

	if (ui_pwm_id != PWMID_CCNT0 && ui_pwm_id != PWMID_CCNT1 && ui_pwm_id != PWMID_CCNT2) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	pwm_ccnt_en.reg = PWM_GETREG(PWM_CCNT_ENABLE_OFS);
	pwm_ccnt_en.reg |= (ui_pwm_id >> 12);
	PWM_SETREG(PWM_CCNT_ENABLE_OFS, pwm_ccnt_en.reg);

	return E_OK;
}


/**
    Set cycle count parameter.

    Set cycle count parameter. CCNT will monitor specific IO PIN as input signal\n
    and count down input signal, once match the CCNT configured value then issue\n
    an interrupt.

    @param ui_pwm_id      pwm id, must be PWMID_CCNTx, one id at a time(PWMID_CCNT0~PWMID_CCNT3)
    @param p_ccnt_cfg     CCNT parameter descriptor
    @return Set cycle count parameter status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_ccnt_config(UINT32 ui_pwm_id, PPWM_CCNT_CFG p_ccnt_cfg)
{
	T_PWM_CCNT_CONFIG           pwm_ccnt = {0};
	T_PWM_CCNT_START_COUNT_VAL  pwm_sta_val = {0};
	T_PWM_CCNT_TRIGGER_VAL      pwm_trig_val = {0};
	T_PWM_CCNT_CTRL_INTEN       pwm_ccnt_ctrl_inten = {0};
	T_PWM_CCNT_CTRL_INTSTS      pwm_ccnt_ctrl_int_sts;
	UINT32              ui_offset;
	UINT32              ui_ccnt_start_offset;
	UINT32              ui_ccnt_offset;

	pwm_ccnt.reg = 0;
	// invalid CCNT num
	ui_ccnt_start_offset = pwm_is_valid_id(12, PWMID_CCNT0);
	if (ui_ccnt_start_offset == PWM_INVALID_ID) {
		DBG_ERR("invalid ID!\r\n");
		return E_PAR;		
	}
	if (ui_pwm_id != PWMID_CCNT0 && ui_pwm_id != PWMID_CCNT1 && ui_pwm_id != PWMID_CCNT2) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	ui_offset = pwm_is_valid_id(12, ui_pwm_id);

	if (ui_offset == PWM_INVALID_ID) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	// CCNT need to be opened before
	if ((pwm_lock_status & ui_pwm_id) == NO_TASK_LOCKED) {
		DBG_ERR("not opened yet!\r\n");
		return E_PAR;
	}

	ui_ccnt_offset = (ui_offset - ui_ccnt_start_offset) * 0x20;

	//Trigger value & Increase or decrease can configured whether ccnt enable or not
	if (pwm_ccnt_is_en(ui_pwm_id)) {
		pwm_ccnt.reg = PWM_GETREG(PWM_CCNT_CONFIG_OFS + ui_ccnt_offset);
	} else {
		pwm_sta_val.bit.ccnt_sta_val    = p_ccnt_cfg->ui_start_value;
		pwm_ccnt.bit.ccnt_filter        = p_ccnt_cfg->ui_filter;
		pwm_ccnt.bit.ccnt_inv           = p_ccnt_cfg->ui_inv;
		pwm_ccnt.bit.ccnt_mode          = p_ccnt_cfg->ui_mode;
		pwm_ccnt.bit.ccnt_signal_source = p_ccnt_cfg->ui_sig_src;


		//if TRUE : represent issue interrupt @edge change
		pwm_ccnt_ctrl_inten.reg = PWM_GETREG(PWM_CCNT_CTRL_INTEN_OFS);
		pwm_ccnt_ctrl_int_sts.reg = PWM_GETREG(PWM_CCNT_CTRL_INTSTS_OFS);
		p_ccnt_cfg->ui_trig_int_en &= (PWM_CCNT_TAGT_TRIG_INTEN | PWM_CCNT_EDGE_TRIG_INTEN);
		if (p_ccnt_cfg->ui_trig_int_en) {
			pwm_ccnt_ctrl_inten.reg &= ~((PWM_INT_CCNT0_EDGE_STS | PWM_INT_CCNT0_TRIG_STS) << ((ui_offset - PWMID_CCNT_NO_0) * 4));
			pwm_ccnt_ctrl_inten.reg |= ((p_ccnt_cfg->ui_trig_int_en) << ((ui_offset - PWMID_CCNT_NO_0) * 4));

			pwm_ccnt_ctrl_int_sts.reg |= ((p_ccnt_cfg->ui_trig_int_en) << ((ui_offset - PWMID_CCNT_NO_0) * 4));
		} else {
			DBG_WRN("No interrupt triggered, force trigger target value arrived interrupt\r\n");
			//Disable edge trigger & enable target value trigger
			pwm_ccnt_ctrl_inten.reg |= ((PWM_INT_CCNT0_TRIG_STS) << ((ui_offset - PWMID_CCNT_NO_0) * 4));
			pwm_ccnt_ctrl_inten.reg &= ~((PWM_INT_CCNT0_EDGE_STS) << ((ui_offset - PWMID_CCNT_NO_0) * 4));

			pwm_ccnt_ctrl_int_sts.reg |= ((PWM_INT_CCNT0_TRIG_STS) << ((ui_offset - PWMID_CCNT_NO_0) * 4));
			pwm_ccnt_ctrl_int_sts.reg &= ~((PWM_INT_CCNT0_EDGE_STS) << ((ui_offset - PWMID_CCNT_NO_0) * 4));
		}
		PWM_SETREG(PWM_CCNT_CTRL_INTEN_OFS, pwm_ccnt_ctrl_inten.reg);
		PWM_SETREG(PWM_CCNT_CTRL_INTSTS_OFS, pwm_ccnt_ctrl_int_sts.reg);
		PWM_SETREG((PWM_CCNT_START_COUNT_VAL_OFS + ui_ccnt_offset), pwm_sta_val.reg);
	}

	pwm_trig_val.bit.ccnt_trig_val  = p_ccnt_cfg->ui_trigger_value;
	pwm_ccnt.bit.ccnt_decrease      = p_ccnt_cfg->ui_count_mode;

	PWM_SETREG((PWM_CCNT_CONFIG_OFS + ui_ccnt_offset), pwm_ccnt.reg);
	PWM_SETREG((PWM_CCNT_TRIGGER_VAL_OFS + ui_ccnt_offset), pwm_trig_val.reg);

	return E_OK;
}


/**
    Set cycle count time out parameter.

    @param[in] ui_pwm_id          pwm id, must be PWMID_CCNTx, one id at a time(PWMID_CCNT0~PWMID_CCNT3)
    @param[in] p_ccnt_tout_cfg     CCNT timeout parameter descriptor
    @return Set cycle count parameter status
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID or not opened yet
*/
ER pwm_ccnt_config_timeout_enable(UINT32 ui_pwm_id, PPWM_CCNT_TOUT_CFG p_ccnt_tout_cfg)
{
	T_PWM_CCNT_CONTROL          pwm_ccnt_ctrl = {0};
	T_PWM_CCNT_CTRL_INTEN       pwm_inten_reg = {0};
	T_PWM_CCNT_CTRL_INTSTS      pwm_inten_sts = {0};
	UINT32                      ui_offset;
	UINT32                      ui_ccnt_start_offset;
	UINT32                      ui_ccnt_offset;
	UINT32                      ui_interrupt_offset;

	pwm_ccnt_ctrl.reg = 0;

	pwm_inten_reg.reg = PWM_GETREG(PWM_CCNT_CTRL_INTEN_OFS);
	pwm_inten_sts.reg = PWM_GETREG(PWM_CCNT_CTRL_INTSTS_OFS);

	ui_ccnt_start_offset = pwm_is_valid_id(0, PWMID_CCNT0);
	if (ui_ccnt_start_offset == PWM_INVALID_ID) {
		DBG_ERR("invalid ID!\r\n");
		return E_PAR;		
	}	
	if (ui_pwm_id != PWMID_CCNT0 && ui_pwm_id != PWMID_CCNT1 && ui_pwm_id != PWMID_CCNT2) {
		DBG_ERR("pwm_setCCNT(), invalid PWM ID!\r\n");
		return E_PAR;
	}

	ui_offset = pwm_is_valid_id(0, ui_pwm_id);

	if (ui_offset == PWM_INVALID_ID) {
		DBG_ERR("pwm_setCCNT(), invalid PWM ID!\r\n");
		return E_PAR;
	}


	ui_ccnt_offset = (ui_offset - ui_ccnt_start_offset) * 0x20;
	ui_interrupt_offset = 2 + ((ui_offset - ui_ccnt_start_offset) * (4));

	pwm_inten_reg.reg |= (1 << (ui_interrupt_offset));
	pwm_inten_sts.reg |= (1 << (ui_interrupt_offset));


	PWM_SETREG((PWM_CCNT_CTRL_INTEN_OFS), pwm_inten_reg.reg);
	PWM_SETREG((PWM_CCNT_CTRL_INTSTS_OFS), pwm_inten_sts.reg);
	if (p_ccnt_tout_cfg->ub_tout_en == TRUE) {
		pwm_ccnt_ctrl.bit.ccnt_time_en = FALSE;
		PWM_SETREG((PWM_CCNT_CONTROL_OFS + ui_ccnt_offset), pwm_ccnt_ctrl.reg);
		// Wait for enable bit truly become FALSE(in real chip)
		while (1) {
			pwm_ccnt_ctrl.reg = PWM_GETREG((PWM_CCNT_CONTROL_OFS + ui_ccnt_offset));
			if (pwm_ccnt_ctrl.bit.ccnt_time_en == FALSE) {
				break;
			}
		}
	}
	pwm_ccnt_ctrl.bit.ccnt_time_cnt = p_ccnt_tout_cfg->ui_tout_value;
	pwm_ccnt_ctrl.bit.ccnt_time_en = p_ccnt_tout_cfg->ub_tout_en;
	PWM_SETREG((PWM_CCNT_CONTROL_OFS + ui_ccnt_offset), pwm_ccnt_ctrl.reg);
	return E_OK;
}


/**
    Get cycle count current counting value.

    It will show current Cycle Count value\n
    Once Overflow:\n
		a. If CCNT2_DECREASE = 0 and CCNT2_CURRENT_VAL = 0xFFFF, the next CCNT2_CURRENT_VALUE value is 0 and keep counting.\n
		b. If CCNT2_DECREASE = 1 and CCNT2_CURRENT_VAL = 0, the next CCNT2_CURRENT_VALUE value is 0xFFFF and keep counting.

    @param ui_pwm_id      PWM ID(PWMID_CCNT0~PWMID_CCNT3)

    @return CCNT current count value
*/
UINT32 pwm_ccnt_get_current_val(UINT32 ui_pwm_id)
{
	UINT32              ui_offset;
	UINT32              ui_ccnt_start_offset;
	UINT32              ui_ccnt_offset;

	ui_ccnt_start_offset = pwm_is_valid_id(0, PWMID_CCNT0);
	if (ui_ccnt_start_offset == PWM_INVALID_ID) {
		DBG_ERR("invalid ID!\r\n");
		return E_PAR;		
	}	
	if (ui_pwm_id != PWMID_CCNT0 && ui_pwm_id != PWMID_CCNT1 && ui_pwm_id != PWMID_CCNT2) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	ui_offset = pwm_is_valid_id(0, ui_pwm_id);

	if (ui_offset == PWM_INVALID_ID) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	ui_ccnt_offset = (ui_offset - ui_ccnt_start_offset) * 0x20;

	return (PWM_GETREG(PWM_CCNT_CURRENT_VAL_OFS + ui_ccnt_offset) & 0xFFFF);
}


/**
    Get cycle count edge and edge detect interval consume clock

    Record clock counts (unit :4*ccnt clock) between 2 CCNTx_EDGE_STS.\n
    This field value is updated when CCNTx_EDGE_STS is issued

    @note: This value will keep on 0xFFFF once if overflow occurred.

    @param[in] ui_pwm_id    PWM ID(PWMID_CCNT0~PWMID_CCNT3)

    @return clock counts (unit :4 *ccnt clock) between 2 CCNTx_EDGE_STS.
*/
UINT32 pwm_ccnt_get_edge_interval(UINT32 ui_pwm_id)
{
	UINT32              ui_offset;
	UINT32              ui_ccnt_start_offset;
	UINT32              ui_ccnt_offset;

	ui_ccnt_start_offset = pwm_is_valid_id(0, PWMID_CCNT0);
	if (ui_ccnt_start_offset == PWM_INVALID_ID) {
		DBG_ERR("invalid ID!\r\n");
		return E_PAR;		
	}	
	if (ui_pwm_id != PWMID_CCNT0 && ui_pwm_id != PWMID_CCNT1 && ui_pwm_id != PWMID_CCNT2) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	ui_offset = pwm_is_valid_id(0, ui_pwm_id);

	if (ui_offset == PWM_INVALID_ID) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	ui_ccnt_offset = (ui_offset - ui_ccnt_start_offset) * 0x20;

	return (PWM_GETREG(PWM_CCNT_EDGE_INTERVAL_OFS + ui_ccnt_offset) & 0xFFFF);
}


/**
    Get cycle count start value

    Return start value

    @note: This value will keep on 0xFFFF once if overflow occurred.

    @param[in] ui_pwm_id    PWM ID(PWMID_CCNT0~PWMID_CCNT3)

    @return start value
*/
UINT32 pwm_ccnt_get_start_value(UINT32 ui_pwm_id)
{
	UINT32              ui_offset;
	UINT32              ui_ccnt_start_offset;
	UINT32              ui_ccnt_offset;

	ui_ccnt_start_offset = pwm_is_valid_id(0, PWMID_CCNT0);
	if (ui_ccnt_start_offset == PWM_INVALID_ID) {
		DBG_ERR("invalid ID!\r\n");
		return E_PAR;		
	}	
	if (ui_pwm_id != PWMID_CCNT0 && ui_pwm_id != PWMID_CCNT1 && ui_pwm_id != PWMID_CCNT2) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	ui_offset = pwm_is_valid_id(0, ui_pwm_id);

	if (ui_offset == PWM_INVALID_ID) {
		DBG_ERR("invalid PWM ID!\r\n");
		return E_PAR;
	}

	ui_ccnt_offset = (ui_offset - ui_ccnt_start_offset) * 0x20;

	return (PWM_GETREG(PWM_CCNT_START_COUNT_VAL_OFS + ui_ccnt_offset) & 0xFFFF);
}

/**
    Set PWM driver configuration

    Set PWM driver configuration

    @param[in] config_id     configuration identifier
    @param[in] ui_config     configuration context for config_id

    @return
		- @b E_OK       : Set configuration success
		- @b E_NOSPT    : config_id is not supported
*/
ER pwm_set_config(PWM_CONFIG_ID config_id, UINT32 ui_config)
{
	switch (config_id) {
	case PWM_CONFIG_ID_AUTOPINMUX:
		b_pwm_auto_disable_pinmux = (BOOL)ui_config;
		break;

	case PWM_CONFIG_ID_PWM_DEBUG: {
			if (pwm_debug) {
				pwm_debug = 0;
				DBG_DUMP("pwm_debug is disabled\r\n");
			} else {
				pwm_debug = 1;
				DBG_DUMP("pwm_debug is enabled\r\n");
			}
		}
		break;

	case PWM_CONFIG_ID_PWM_OPEN_DESTINATION: {
			if (ui_config == PWM_DEST_TO_CPU1) {
				pwm_open_dest = PWM_DEST_TO_CPU1;
				PWM_CTRL_INTEN_OFS = PWM_CTRL_INTEN_OFS0;
				PWM_MS_CTRL_INTEN_OFS = PWM_MS_CTRL_INTEN_OFS0;
				PWM_CCNT_CTRL_INTEN_OFS = PWM_CCNT_CTRL_INTEN_OFS0;
				PWM_CLKDIV_LOAD_INTEN_OFS = PWM_CLKDIV_LOAD_INTEN_OFS0;
				PWM_CTRL_INTSTS_OFS = PWM_CTRL_INTSTS_OFS0;
				PWM_MS_CTRL_INTSTS_OFS = PWM_MS_CTRL_INTSTS_OFS0;
				PWM_CCNT_CTRL_INTSTS_OFS = PWM_CCNT_CTRL_INTSTS_OFS0;
				PWM_CLKDIV_LOAD_STS_OFS = PWM_CLKDIV_LOAD_STS_OFS0;
				DBG_DUMP("pwm_open_dest to CPU1\r\n");
			} else if (ui_config == PWM_DEST_TO_CPU2) {
				pwm_open_dest = PWM_DEST_TO_CPU2;
				PWM_CTRL_INTEN_OFS = PWM_CTRL_INTEN_OFS1;
				PWM_MS_CTRL_INTEN_OFS = PWM_MS_CTRL_INTEN_OFS1;
				PWM_CCNT_CTRL_INTEN_OFS = PWM_CCNT_CTRL_INTEN_OFS1;
				PWM_CLKDIV_LOAD_INTEN_OFS = PWM_CLKDIV_LOAD_INTEN_OFS1;
				PWM_CTRL_INTSTS_OFS = PWM_CTRL_INTSTS_OFS1;
				PWM_MS_CTRL_INTSTS_OFS = PWM_MS_CTRL_INTSTS_OFS1;
				PWM_CCNT_CTRL_INTSTS_OFS = PWM_CCNT_CTRL_INTSTS_OFS1;
				PWM_CLKDIV_LOAD_STS_OFS = PWM_CLKDIV_LOAD_STS_OFS1;
				DBG_DUMP("pwm_open_dest to CPU2\r\n");
			} else {
				DBG_DUMP("PWM_OPEN_DESTINATION config error\r\n");
			}
		}
		break;

	case PWM_CONFIG_ID_DISABLE_CCNT_AFTER_TARGET: {
			b_disable_ccnt_after_target = (BOOL)ui_config;
			if (b_disable_ccnt_after_target) {
				DBG_DUMP("b_disable_ccnt_after_target is enabled\r\n");
			} else {
				DBG_DUMP("b_disable_ccnt_after_target is disabled\r\n");
			}
		}
		break;

#if defined(__FREERTOS)
	case PWM_CONFIG_ID_PWM_REQUEST_IRQ: {
			if (ui_config == PWM_DEST_TO_CPU1) {
				pwm_platform_request_irq(PWM_DEST_TO_CPU1);
				DBG_DUMP("request pwm_isr\r\n");
			} else if (ui_config == PWM_DEST_TO_CPU2) {
				pwm_platform_request_irq(PWM_DEST_TO_CPU2);
				DBG_DUMP("request pwm_isr2\r\n");
			} else {
				DBG_DUMP("PWM_REQUEST_IRQ config error\r\n");
			}
		}
		break;

	case PWM_CONFIG_ID_PWM_FREE_IRQ: {
			if (ui_config == PWM_DEST_TO_CPU1) {
				pwm_platform_free_irq(PWM_DEST_TO_CPU1);
				DBG_DUMP("free pwm_isr\r\n");
			} else if (ui_config == PWM_DEST_TO_CPU2) {
				pwm_platform_free_irq(PWM_DEST_TO_CPU2);
				DBG_DUMP("free pwm_isr2\r\n");
			} else {
				DBG_DUMP("PWM_FREE_IRQ config error\r\n");
			}
		}
		break;
#endif

	default:
		return E_NOSPT;
	}

	return E_OK;
}

//@}
//@}

/*
    PWM debug port select

    @note: Internal usage

    @param ui_debug_sel      debug select

    @return E_OK
*/
ER pwm_debug_sel(UINT32 ui_debug_sel)
{
	T_PWM_DEBUG_PORT    ui_debug_port;

	ui_debug_port.reg = PWM_GETREG(PWM_DEBUG_PORT_OFS);

	ui_debug_port.bit.debug_sel = (ui_debug_sel & 0xF);

	PWM_SETREG(PWM_DEBUG_PORT_OFS, ui_debug_port.reg);

	return E_OK;
}

/*
    PWM debug port select

    @note: Internal usage

    @param ui_debug_sel      debug select

    @return E_OK
*/

void pwm_dump_info(void)
{
#if 0
    T_PWM_CTRL_REG_BUF      ui_pwm_ctrl_reg;
    T_PWM_PERIOD_REG_BUF    ui_pwm_period_reg;
    T_PWM_EXPEBD_PERIOD_REG_BUF exp_period_buf;
    T_PWM_MS_EXT_REG_BUF    reg_pwm_ms_ctrl;
    T_PWM_CCNT_CONFIG           pwm_ccnt = {0};
    PAD_DRIVINGSINK         ui_pad_driving;
    UINT32  ui_pwm_id, ccnt_id;
    UINT32  ui_div;
    UINT32  ui_clock;

	for (ui_pwm_id = 0; ui_pwm_id < 20; ui_pwm_id++) {
		if (pad_get_driving_sink((PAD_DS_PGPIO0 + ui_pwm_id), &ui_pad_driving) == E_OK) {
			switch (ui_pad_driving) {
			case PAD_DRIVINGSINK_4MA:
				DBG_DUMP("PWM%02d_Driving = 4 mA\r\n", ui_pwm_id);
				break;
			case PAD_DRIVINGSINK_10MA:
				DBG_DUMP("PWM%02d_Driving = 10 mA\r\n", ui_pwm_id);
				break;
			default:
				DBG_DUMP("PWM%02d_Driving = 0 mA(Get driving error)\r\n", ui_pwm_id);
				break;
			}
		} else {
			DBG_DUMP("PWM%02d_Driving = 0 mA(Get driving error)\r\n", ui_pwm_id);
		}
	}

	for (ui_pwm_id = 0; ui_pwm_id < 20; ui_pwm_id++) {
		if(PWM_GETREG(PWM_ENABLE_OFS) & (1 << ui_pwm_id)) {
			ui_pwm_ctrl_reg.reg = PWM_GETREG(0x8 * ui_pwm_id + PWM_CTRL_REG_BUF_OFS);
			if (ui_pwm_ctrl_reg.bit.pwm_type == PWM_TYPE_PWM) {
				if(ui_pwm_id > 7 && ui_pwm_id < 16) {
					ui_pwm_period_reg.reg = PWM_GETREG(0x8 * ui_pwm_id + PWM_PERIOD_REG_BUF_OFS);
					pll_get_pwm_clock_rate(ui_pwm_id, &ui_div);
					ui_clock = (120000000 / ui_div) / ui_pwm_period_reg.bit.pwm_prd;

					DBG_DUMP("=================================\r\n");
					DBG_DUMP("==            PWM %d           ==\r\n", ui_pwm_id);
					DBG_DUMP("=================================\r\n");
					DBG_DUMP("period          %d               \r\n", ui_pwm_period_reg.bit.pwm_prd);
					DBG_DUMP("rising          %d               \r\n", ui_pwm_period_reg.bit.pwm_r);
					DBG_DUMP("falling         %d               \r\n", ui_pwm_period_reg.bit.pwm_f);
					DBG_DUMP("inverse         %d               \r\n", ui_pwm_period_reg.bit.pwm_inv);
					DBG_DUMP("cycle           %d               \r\n", ui_pwm_ctrl_reg.bit.pwm_on);
					DBG_DUMP("freq            %d (Hz)          \r\n", ui_clock);
					DBG_DUMP("=================================\r\n");
					DBG_DUMP("\r\n");
				} else if (ui_pwm_id >= 16) {

					ui_pwm_period_reg.reg = PWM_GETREG(0x8 * ui_pwm_id + PWM_PERIOD_REG_BUF_OFS + 0x20);
					pll_get_pwm_clock_rate(ui_pwm_id, &ui_div);
					ui_clock = (120000000 / ui_div) / ui_pwm_period_reg.bit.pwm_prd;

					DBG_DUMP("=================================\r\n");
					DBG_DUMP("==            PWM %d           ==\r\n", ui_pwm_id);
					DBG_DUMP("=================================\r\n");
					DBG_DUMP("period          %d               \r\n", ui_pwm_period_reg.bit.pwm_prd);
					DBG_DUMP("rising          %d               \r\n", ui_pwm_period_reg.bit.pwm_r);
					DBG_DUMP("falling         %d               \r\n", ui_pwm_period_reg.bit.pwm_f);
					DBG_DUMP("inverse         %d               \r\n", ui_pwm_period_reg.bit.pwm_inv);
					DBG_DUMP("cycle           %d               \r\n", ui_pwm_ctrl_reg.bit.pwm_on);
					DBG_DUMP("freq            %d (Hz)          \r\n", ui_clock);
					DBG_DUMP("=================================\r\n");
					DBG_DUMP("\r\n");
				} else {
					exp_period_buf.reg = PWM_GETREG(0x4 * ui_pwm_id + PWM0_EXPEND_PERIOD_REG_BUF_OFS);
					ui_pwm_period_reg.reg = PWM_GETREG(0x8 * ui_pwm_id + PWM_PERIOD_REG_BUF_OFS);
					pll_get_pwm_clock_rate(ui_pwm_id, &ui_div);
					ui_clock = (120000000 / ui_div) / ((ui_pwm_period_reg.bit.pwm_prd & 0xff) | ((exp_period_buf.bit.pwm_prd << 8) & 0xff00));

					DBG_DUMP("=================================\r\n");
					DBG_DUMP("==            PWM %d           ==\r\n", ui_pwm_id);
					DBG_DUMP("=================================\r\n");
					DBG_DUMP("period          %d               \r\n", ((ui_pwm_period_reg.bit.pwm_prd & 0xff) | ((exp_period_buf.bit.pwm_prd << 8) & 0xff00)));
					DBG_DUMP("rising          %d               \r\n", ((ui_pwm_period_reg.bit.pwm_r & 0xff) | ((exp_period_buf.bit.pwm_r << 8) & 0xff00)));
					DBG_DUMP("falling         %d               \r\n", ((ui_pwm_period_reg.bit.pwm_f & 0xff) | ((exp_period_buf.bit.pwm_f << 8) & 0xff00)));
					DBG_DUMP("inverse         %d               \r\n", ui_pwm_period_reg.bit.pwm_inv);
					DBG_DUMP("cycle           %d               \r\n", ui_pwm_ctrl_reg.bit.pwm_on);
					DBG_DUMP("freq            %d (Hz)          \r\n", ui_clock);
					DBG_DUMP("=================================\r\n");
					DBG_DUMP("\r\n");
				}

			} else {
				if (PWM_GETREG(PWM_MS_START_OFS) & (1 << ui_pwm_id)) {
					reg_pwm_ms_ctrl.reg = PWM_GETREG(0x8 * ui_pwm_id + PWM_MS_EXT_REG_BUF_OFS);
					pll_get_pwm_clock_rate(ui_pwm_id, &ui_div);
					ui_clock = (120000000 / ui_div) / 100;

					DBG_DUMP("=================================\r\n");
					DBG_DUMP("==            MS %d            ==\r\n", ui_pwm_id);
					DBG_DUMP("=================================\r\n");
					DBG_DUMP("dir             %d               \r\n", ui_pwm_ctrl_reg.bit.pwm_ms_dir);
					DBG_DUMP("threshold       %d               \r\n", ui_pwm_ctrl_reg.bit.pwm_ms_threshold_en);
					DBG_DUMP("th value        %d               \r\n", ui_pwm_ctrl_reg.bit.pwm_ms_threshold);
					DBG_DUMP("grad            %d               \r\n", reg_pwm_ms_ctrl.bit.pwm_ms_start_grads);
					DBG_DUMP("step/unit       %d               \r\n", reg_pwm_ms_ctrl.bit.pwm_ms_step_per_unit);
					DBG_DUMP("grad/step       %d               \r\n", reg_pwm_ms_ctrl.bit.pwm_ms_grads_per_step);
					DBG_DUMP("cycle           %d (Hz)          \r\n", ui_pwm_ctrl_reg.bit.pwm_on);
					DBG_DUMP("freq            %d (Hz)          \r\n", ui_clock);
					DBG_DUMP("=================================\r\n");
					DBG_DUMP("\r\n");
				}
			}
		}
	}

	for (ccnt_id = 0; ccnt_id < 4; ccnt_id++) {
		if (PWM_GETREG(PWM_CCNT_ENABLE_OFS) & (1 << ccnt_id)) {
			pwm_ccnt.reg = PWM_GETREG(PWM_CCNT_CONFIG_OFS + 0x20 * ccnt_id);

			DBG_DUMP("==================================\r\n");
			DBG_DUMP("==            CCNT %d           ==\r\n", ccnt_id);
			DBG_DUMP("==================================\r\n");
			DBG_DUMP("filter          %d               \r\n", pwm_ccnt.bit.ccnt_filter );
			DBG_DUMP("dir             %d               \r\n", pwm_ccnt.bit.ccnt_decrease);
			DBG_DUMP("inv             %d               \r\n", pwm_ccnt.bit.ccnt_inv);
			DBG_DUMP("mode            %d               \r\n", pwm_ccnt.bit.ccnt_mode);
			DBG_DUMP("source          %d               \r\n", pwm_ccnt.bit.ccnt_signal_source);
			DBG_DUMP("start           %d               \r\n", PWM_GETREG(PWM_CCNT_START_COUNT_VAL_OFS + 0x20 * ccnt_id) & 0xFFFF);
			DBG_DUMP("trig            %d               \r\n", PWM_GETREG(PWM_CCNT_TRIGGER_VAL_OFS + 0x20 * ccnt_id) & 0xffff);
			DBG_DUMP("timeout         %d               \r\n", PWM_GETREG(PWM_CCNT_CONTROL_OFS + 0x20 * ccnt_id) & 0x1);
			DBG_DUMP("timeout_cnt     %d               \r\n", (PWM_GETREG(PWM_CCNT_CONTROL_OFS + 0x20 * ccnt_id) & 0xffff0000) >> 16);
			DBG_DUMP("=================================\r\n");
			DBG_DUMP("\r\n");
		}
	}
#endif
}

/**
    Set PWM destination

    Set PWM destination, the new settings will take effect after calling pwm_open.

    @param[in] ui_pwm_id  pwm id, one id at a time
    @param[in] ui_pwm_dest  PWM destination

    @return void
*/
void pwm_set_destination(UINT32 ui_pwm_id, PWM_DEST ui_pwm_dest)
{
	T_PWM_TO_CPU1      ui_pwm_to_cpu1;
	T_PWM_TO_CPU2      ui_pwm_to_cpu2;
	UINT32 flags = 0;

	flags = pwm_platform_spin_lock();

	switch (ui_pwm_dest) {
	case PWM_DEST_TO_CPU1:
		ui_pwm_to_cpu1.reg = PWM_GETREG(PWM_TO_CPU1_OFS);
		ui_pwm_to_cpu1.reg |= ui_pwm_id;
		PWM_SETREG(PWM_TO_CPU1_OFS, ui_pwm_to_cpu1.reg);

		ui_pwm_to_cpu2.reg = PWM_GETREG(PWM_TO_CPU2_OFS);
		ui_pwm_to_cpu2.reg &= ~ui_pwm_id;
		PWM_SETREG(PWM_TO_CPU2_OFS, ui_pwm_to_cpu2.reg);

		break;

	case PWM_DEST_TO_CPU2:
		ui_pwm_to_cpu2.reg = PWM_GETREG(PWM_TO_CPU2_OFS);
		ui_pwm_to_cpu2.reg |= ui_pwm_id;
		PWM_SETREG(PWM_TO_CPU2_OFS, ui_pwm_to_cpu2.reg);

		ui_pwm_to_cpu1.reg = PWM_GETREG(PWM_TO_CPU1_OFS);
		ui_pwm_to_cpu1.reg &= ~ui_pwm_id;
		PWM_SETREG(PWM_TO_CPU1_OFS, ui_pwm_to_cpu1.reg);

		break;

	default:
		ui_pwm_to_cpu1.reg = PWM_GETREG(PWM_TO_CPU1_OFS);
		ui_pwm_to_cpu1.reg |= ui_pwm_id;
		PWM_SETREG(PWM_TO_CPU1_OFS, ui_pwm_to_cpu1.reg);

		ui_pwm_to_cpu2.reg = PWM_GETREG(PWM_TO_CPU2_OFS);
		ui_pwm_to_cpu2.reg &= ~ui_pwm_id;
		PWM_SETREG(PWM_TO_CPU2_OFS, ui_pwm_to_cpu2.reg);
		break;
	}

	pwm_platform_spin_unlock(flags);
}

