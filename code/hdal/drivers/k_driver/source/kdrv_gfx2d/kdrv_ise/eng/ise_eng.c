
/*
    ISE module driver

    NT98520 ISE module driver.

    @file       ise_eng.c
    @ingroup    mIIPPISE
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "kwrap/util.h"
#include "ise_eng_int_platform.h"
#include "ise_eng_int_reg.h"
#include "ise_eng.h"

#define ISE_ENG_TIMEOUT_MS (1000)

static ISE_ENG_CTL eng_ctl;

#if 0
#endif

static UINT32 ise_eng_int_2s_complement(INT32 val, INT32 bits)
{
	UINT32 tmp = 0;
	UINT32 mask = (1 << bits) - 1;

	if (val >= 0) {
		tmp = (((UINT32)val) & mask);
	} else {
		tmp = ((UINT32)(~(-1 * val)) & mask) + 1;
	}

	return tmp;
}

static UINT32 ise_eng_int_cal_isd_coef_val_lsb(INT32 val)
{
	UINT32 get_val = 0;
	UINT32 val_lsb = 0;


	get_val = ise_eng_int_2s_complement(val, 12);
	val_lsb = (get_val & 0x000000FF);

	return val_lsb;
}

static UINT32 ise_eng_int_cal_isd_coef_val_msb(INT32 val)
{
	UINT32 get_val = 0;
	UINT32 val_msb = 0;

	get_val = ise_eng_int_2s_complement(val, 12);
	val_msb = ((get_val >> 8) & 0x0000000F);

	return val_msb;
}

static UINT32 ise_eng_int_cal_isd_base_factor(UINT32 in_size, UINT32 out_size)
{
	UINT32 isd_base_factor[6] = {0};
	UINT32 isd_base_cnt[6] = {0};
	UINT32 i = 0, j = 0;
	UINT32 s = (in_size - 1) * 65536 / (out_size - 1);
	UINT32 init_w = (s + 65536) >> 1, w = 0;
	UINT32 factor_cnt = 0;
	UINT32 rem = 0;
	UINT32 same_factor = 0, same_factor_idx_cnt = 0;
	UINT32 sum = 0, cnt = 0, base_factor = 0;

	for (i = 0; i < 160; i++) {
		factor_cnt = 0;

		rem = s;
		factor_cnt += ((init_w + 128) >> 8);
		rem -= init_w;

		factor_cnt += ((rem >> 16) << 8);
		rem = (rem & 0xffff);

		init_w = 65536 - rem;
		w = rem;
		factor_cnt += ((w + 128) >> 8);
		/* remove for cim issue */
		/* rem -= w; */

		same_factor = 0;
		for (j = 0; j < 6; j++) {
			if (isd_base_factor[j] == factor_cnt) {
				same_factor = 1;
				isd_base_cnt[j] += 1;
				break;
			}
		}

		if (same_factor == 0) {
			isd_base_factor[same_factor_idx_cnt] = factor_cnt;
			isd_base_cnt[same_factor_idx_cnt] += 1;

			same_factor_idx_cnt += 1;
		}
	}

	sum = 0, cnt = 0;
	for (j = 0; j < 6; j++) {
		if (isd_base_cnt[j] != 0) {
			sum += isd_base_factor[j];
			cnt += 1;
		}
	}

	base_factor = s;
	if (cnt != 0) {
		base_factor = (sum / cnt);
	}

	return base_factor;
}

static UINT32 ise_eng_int_cal_isd_kernel_number(UINT32 in_size, UINT32 out_size)
{
	UINT32 factor = 0;
	UINT32 ker_cnt = 0;

	factor = ((in_size - 1) * 65536) / (out_size - 1);

	if (factor > 0) {
		ker_cnt += 1;
		factor -= 65536;
	}

	while (factor > 0) {
		if (factor >= 131072) {
			ker_cnt += 2;
			factor -= 131072;
		} else {
			ker_cnt += 2;

			//coverity[result_independent_of_operands]
			factor -= factor;
		}
	}

	return ker_cnt;
}

static UINT32 ise_eng_int_cal_scale_filter_coef(UINT32 in_size, UINT32 out_size)
{
	UINT32 coef = 0x0;

	if (in_size > out_size && out_size > 0) {
		coef = (in_size * 2 / out_size);

		if (coef > 64) {
			coef = 64;
		}
		coef = 64 - coef;
	}

	return coef;
}

static void ise_eng_int_soft_reset(ISE_ENG_HANDLE *p_eng)
{
	T_ISE_ENGINE_CONTROL_REGISTER arg;

	arg.reg = ISE_ENG_GETREG(p_eng->reg_io_base + ISE_ENGINE_CONTROL_REGISTER_OFS);
	arg.bit.ise_rst = 1;
	ISE_ENG_SETREG(p_eng->reg_io_base + ISE_ENGINE_CONTROL_REGISTER_OFS, arg.reg);
}

#if 0
#endif

INT32 ise_eng_init(ISE_ENG_CTL *p_eng_ctl)
{
	CHAR name[16];
	UINT32 i = 0;

	if (eng_ctl.p_eng != NULL) {
		DBG_ERR("reinit\r\n");
		return -1;
	}

	eng_ctl.p_eng = ISE_ENG_MALLOC(sizeof(ISE_ENG_HANDLE) * p_eng_ctl->total_ch);
	if (eng_ctl.p_eng == NULL) {
		DBG_ERR("alloc buf(%d) failed\r\n", sizeof(ISE_ENG_HANDLE) * (int)p_eng_ctl->total_ch);
		return -1;
	}

	eng_ctl.chip_num = p_eng_ctl->chip_num;
	eng_ctl.eng_num = p_eng_ctl->eng_num;
	eng_ctl.total_ch= p_eng_ctl->total_ch;
	memcpy((void *)eng_ctl.p_eng, (void *)p_eng_ctl->p_eng, (sizeof(ISE_ENG_HANDLE) * p_eng_ctl->total_ch));

	for (i = 0; i < eng_ctl.total_ch; i++) {
		snprintf(name, 16, "ise_eng_%d_flg", (int)i);
		vos_flag_create(&eng_ctl.p_eng[i].flg_id, NULL, name);

		snprintf(name, 16, "ise_eng_%d_sem", (int)i);
		vos_sem_create(&eng_ctl.p_eng[i].sem, 1, name);

		ise_eng_platform_set_clk_rate(&eng_ctl.p_eng[i]);
		ise_eng_platform_prepare_clk(&eng_ctl.p_eng[i]);
	}

	return 0;
}

INT32 ise_eng_release(ISE_ENG_CTL *p_eng_ctl)
{
	UINT32 i = 0;

	for (i = 0; i < eng_ctl.total_ch; i++) {
		vos_flag_destroy(eng_ctl.p_eng[i].flg_id);
		vos_sem_destroy(eng_ctl.p_eng[i].sem);
		ise_eng_platform_unprepare_clk(&eng_ctl.p_eng[i]);
	}
	ISE_ENG_FREE(eng_ctl.p_eng);
	memset((void *)&eng_ctl, 0, sizeof(ISE_ENG_CTL));

	return 0;
}

ISE_ENG_HANDLE* ise_eng_get_handle(UINT32 chip_id, UINT32 eng_id)
{
	UINT32 idx = 0;

	/* todo: chip_id & eng_id mapping */
	if (chip_id == KDRV_CHIP0 && eng_id == KDRV_GFX2D_ISE0) {
		idx = 0;
	} else {
		DBG_ERR("unsupport chip_id %d, eng_id 0x%.8x\r\n", (int)chip_id, (unsigned int)eng_id);
		return NULL;
	}
	if (idx < eng_ctl.total_ch) {
		return &eng_ctl.p_eng[idx];
	}
	return NULL;
}

void ise_eng_reg_isr_callback(ISE_ENG_HANDLE *p_eng, ISE_ISR_CB cb)
{
	if (p_eng == NULL) {
		return ;
	}
	p_eng->isr_cb = cb;
}

INT32 ise_eng_open(ISE_ENG_HANDLE *p_eng)
{
	if (p_eng == NULL) {
		return E_PAR;
	}

	/* get semaphore */
	if (vos_sem_wait_timeout(p_eng->sem, vos_util_msec_to_tick(ISE_ENG_TIMEOUT_MS)) != E_OK) {
		return E_TMOUT;
	}

	/* enable clock & sram */
	ise_eng_platform_enable_clk(p_eng);
	ise_eng_platform_disable_sram_shutdown(p_eng);

	/* clear interrupt enable & status */
	ISE_ENG_SETREG(p_eng->reg_io_base + ISE_INTERRUPT_ENABLE_REGISTER_OFS, 0);
	ISE_ENG_SETREG(p_eng->reg_io_base + ISE_INTERRUPT_STATUS_REGISTER_OFS, 0xFFFFFFFF);

	/* software reset */
	ise_eng_int_soft_reset(p_eng);

	return E_OK;
}

INT32 ise_eng_close(ISE_ENG_HANDLE *p_eng)
{
	if (p_eng == NULL) {
		return E_PAR;
	}

	/* disable clock & sram */
	ise_eng_platform_enable_sram_shutdown(p_eng);
	ise_eng_platform_disable_clk(p_eng);

	/* release semaphore */
	vos_sem_isig(p_eng->sem);

	return E_OK;
}

void ise_eng_trig_single(ISE_ENG_HANDLE *p_eng)
{
	T_ISE_ENGINE_CONTROL_REGISTER arg;

	if (p_eng == NULL) {
		return ;
	}

	arg.reg = ISE_ENG_GETREG(p_eng->reg_io_base + ISE_ENGINE_CONTROL_REGISTER_OFS);
	arg.bit.ise_start = 1;
	ISE_ENG_SETREG(p_eng->reg_io_base + ISE_ENGINE_CONTROL_REGISTER_OFS, arg.reg);
}

void ise_eng_trig_ll(ISE_ENG_HANDLE *p_eng, UINT32 ll_addr)
{
	T_ISE_ENGINE_CONTROL_REGISTER arg;
	T_ISE_INPUT_DMA_CHANNEL_REGISTER3 ll_sai;

	if (p_eng == NULL) {
		return ;
	}

	ll_sai.reg = 0;
	ll_sai.bit.ise_dram_ll_sai = ll_addr;
	ISE_ENG_SETREG(p_eng->reg_io_base + ISE_INPUT_DMA_CHANNEL_REGISTER3_OFS, ll_sai.reg);

	arg.reg = ISE_ENG_GETREG(p_eng->reg_io_base + ISE_ENGINE_CONTROL_REGISTER_OFS);
	arg.bit.ise_ll_fire = 1;
	ISE_ENG_SETREG(p_eng->reg_io_base + ISE_ENGINE_CONTROL_REGISTER_OFS, arg.reg);
}

void ise_eng_write_reg(ISE_ENG_HANDLE *p_eng, UINT32 reg_ofs, UINT32 val)
{
	if (p_eng == NULL) {
		return ;
	}

	ISE_ENG_SETREG((p_eng->reg_io_base + reg_ofs), val);
}

void ise_eng_isr(ISE_ENG_HANDLE *p_eng)
{
	UINT32 status = 0;
	UINT32 inte = 0;

	/* assume chip 0, eng 0 */
	if (p_eng == NULL) {
		DBG_ERR("null p_eng\r\n");
		return ;
	}

	/* get interrupt status & enable bit */
	status = ISE_ENG_GETREG(p_eng->reg_io_base + ISE_INTERRUPT_STATUS_REGISTER_OFS);
	inte = ISE_ENG_GETREG(p_eng->reg_io_base + ISE_INTERRUPT_ENABLE_REGISTER_OFS);

	/* clear interrupt status */
	status &= inte;
	ISE_ENG_SETREG(p_eng->reg_io_base + ISE_INTERRUPT_STATUS_REGISTER_OFS, status);

	if (p_eng->isr_cb != NULL) {
		p_eng->isr_cb(p_eng, status, NULL);
	}
}

#if 0
#endif

INT32 ise_eng_cal_scale_factor(UINT32 in_width, UINT32 in_height, UINT32 out_width, UINT32 out_height, UINT32 scl_method, ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	UINT32 scale_factor_tmp = 0;
	UINT32 isd_scale_factor_tmp = 0;

	p_scl_factor->h_scl_ud = (in_width >= out_width) ? ISE_ENG_SCALE_DOWN : ISE_ENG_SCALE_UP;
	p_scl_factor->v_scl_ud = (in_height >= out_height) ? ISE_ENG_SCALE_DOWN : ISE_ENG_SCALE_UP;

	/* todo: p_scl_factor->isd_coef_mode */
	p_scl_factor->isd_coef_mode = 0;

	/* horizontal scale */
	scale_factor_tmp = (in_width - 1) * 65536 / (out_width - 1);
	if (p_scl_factor->h_scl_ud == ISE_ENG_SCALE_DOWN) {
		p_scl_factor->h_scl_down_rate = (scale_factor_tmp >> 16) - 1;
		p_scl_factor->h_scl_factor = scale_factor_tmp - ((p_scl_factor->h_scl_down_rate + 1) << 16);

		if (scl_method == ISE_ENG_SCALE_INTEGRATION) {
			isd_scale_factor_tmp = ise_eng_int_cal_isd_base_factor(in_width, out_width);

			p_scl_factor->isd_scl_base_h = isd_scale_factor_tmp;

			/* 1048576.0 = (256.0 * 4096.0) */
			p_scl_factor->isd_scl_factor_h[0] = (1048576 + ((isd_scale_factor_tmp - 1) >> 1)) / (isd_scale_factor_tmp - 1);
			p_scl_factor->isd_scl_factor_h[1] = (1048576 + ((isd_scale_factor_tmp - 0) >> 1)) / (isd_scale_factor_tmp - 0);
			p_scl_factor->isd_scl_factor_h[2] = (1048576 + ((isd_scale_factor_tmp + 1) >> 1)) / (isd_scale_factor_tmp + 1);

			p_scl_factor->isd_scl_coef_num_h = ise_eng_int_cal_isd_kernel_number(in_width, out_width);
		} else {
			p_scl_factor->isd_scl_base_h = 256;
			p_scl_factor->isd_scl_factor_h[0] = 4096;
			p_scl_factor->isd_scl_factor_h[1] = 4096;
			p_scl_factor->isd_scl_factor_h[2] = 4096;
			p_scl_factor->isd_scl_coef_num_h = 1;
		}
	} else {
		p_scl_factor->h_scl_down_rate = 0;
		p_scl_factor->h_scl_factor = scale_factor_tmp;

		p_scl_factor->isd_scl_base_h = 256;
		p_scl_factor->isd_scl_factor_h[0] = 4096;
		p_scl_factor->isd_scl_factor_h[1] = 4096;
		p_scl_factor->isd_scl_factor_h[2] = 4096;
	}

	/* vertical scale */
	scale_factor_tmp = (in_height - 1) * 65536 / (out_height - 1);
	if (p_scl_factor->v_scl_ud == ISE_ENG_SCALE_DOWN) {
		p_scl_factor->v_scl_down_rate = (scale_factor_tmp >> 16) - 1;
		p_scl_factor->v_scl_factor = scale_factor_tmp - ((p_scl_factor->h_scl_down_rate + 1) << 16);

		if (scl_method == ISE_ENG_SCALE_INTEGRATION) {
			isd_scale_factor_tmp = ise_eng_int_cal_isd_base_factor(in_height, out_height);

			p_scl_factor->isd_scl_base_v = isd_scale_factor_tmp;

			/* 1048576.0 = (256.0 * 4096.0) */
			p_scl_factor->isd_scl_factor_v[0] = (1048576 + ((isd_scale_factor_tmp - 1) >> 1)) / (isd_scale_factor_tmp - 1);
			p_scl_factor->isd_scl_factor_v[1] = (1048576 + ((isd_scale_factor_tmp - 0) >> 1)) / (isd_scale_factor_tmp - 0);
			p_scl_factor->isd_scl_factor_v[2] = (1048576 + ((isd_scale_factor_tmp + 1) >> 1)) / (isd_scale_factor_tmp + 1);

			p_scl_factor->isd_scl_coef_num_h = ise_eng_int_cal_isd_kernel_number(in_height, out_height);
		} else {
			p_scl_factor->isd_scl_base_v = 256;
			p_scl_factor->isd_scl_factor_v[0] = 4096;
			p_scl_factor->isd_scl_factor_v[1] = 4096;
			p_scl_factor->isd_scl_factor_v[2] = 4096;
			p_scl_factor->isd_scl_coef_num_v = 1;
		}
	} else {
		p_scl_factor->v_scl_down_rate = 0;
		p_scl_factor->v_scl_factor = scale_factor_tmp;

		p_scl_factor->isd_scl_base_v = 256;
		p_scl_factor->isd_scl_factor_v[0] = 4096;
		p_scl_factor->isd_scl_factor_v[1] = 4096;
		p_scl_factor->isd_scl_factor_v[2] = 4096;
	}

	return 0;
}

INT32 ise_eng_cal_scale_filter(UINT32 in_width, UINT32 in_height, UINT32 out_width, UINT32 out_height, ISE_ENG_SCALE_FILTER_INFO *p_scl_filter)
{
	p_scl_filter->h_filt_en = (in_width > out_width) ? 1 : 0;
	p_scl_filter->v_filt_en = (in_height > out_height) ? 1 : 0;

	p_scl_filter->h_filt_coef = ise_eng_int_cal_scale_filter_coef(in_width, out_width);
	p_scl_filter->v_filt_coef = ise_eng_int_cal_scale_filter_coef(in_height, out_height);

	return 0;
}

INT32 ise_eng_cal_stripe(UINT32 in_width, UINT32 out_width, UINT32 scl_method, ISE_ENG_STRIPE_INFO *p_stripe)
{
	/* these setting shuld be reviewed */
	UINT32 strp_size = 0;
	UINT32 ovlp_size = 0;

	/* fix hw_auto mode */
	p_stripe->overlap_mode = 0;
	if (scl_method == ISE_ENG_SCALE_INTEGRATION) {
		UINT32 scale_rate = 0;

		scale_rate = ((in_width - 1) * 512) / (out_width - 1);
		if (scale_rate < 3584) {
			/* 16pixel */
			p_stripe->overlap_size_sel = 1;
		} else {
			/* 32pixel */
			p_stripe->overlap_size_sel = 3;
		}
	} else {
		/* 8pixel */
		p_stripe->overlap_size_sel = 0;
	}

	ovlp_size = (p_stripe->overlap_size_sel + 1) << 3;
	if (in_width <= ISE_ENG_STRIPE_SIZE_MAX) {
		p_stripe->stripe_size = ISE_ENG_STRIPE_SIZE_MAX;
	} else {
		UINT32 hn, hm, hl, check_size;

		strp_size = ISE_ENG_STRIPE_SIZE_MAX;
		hn = strp_size;
		hm = in_width / (strp_size - ovlp_size);
		hl = in_width - ((strp_size - ovlp_size) * hm);

		while (hl < 64) {
			hn = strp_size;
			hm = in_width / (strp_size - ovlp_size);
			hl = in_width - ((strp_size - ovlp_size) * hm);

			strp_size -= 2;
		}

		hm += 1;
		check_size = ((hn - ovlp_size) * (hm - 1)) + hl;
		if (check_size != in_width) {
			DBG_ERR("stripe size error\r\n");
			return E_PAR;
		}

		p_stripe->stripe_size = strp_size;
	}

	return 0;
}

#if 0
#endif

ISE_ENG_REG ise_eng_gen_in_ctrl_reg(UINT8 io_fmt, UINT8 scl_method, UINT8 argb_out_mode, UINT8 in_burst, UINT8 out_burst, UINT8 ovlp_mode)
{
	T_ISE_INPUT_CONTROL_REGISTER0 arg;
	ISE_ENG_REG rt = {ISE_INPUT_CONTROL_REGISTER0_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_in_fmt = io_fmt;
	arg.bit.ise_scl_method = scl_method;
	arg.bit.ise_argb_outmode = argb_out_mode;
	arg.bit.ise_in_brt_lenth = in_burst;
	arg.bit.ise_out_brt_lenth = out_burst;
	arg.bit.ise_ovlap_mode = ovlp_mode;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_int_en_reg(UINT32 inte_en)
{
	ISE_ENG_REG rt = {ISE_INTERRUPT_ENABLE_REGISTER_OFS, 0};

	rt.val = inte_en;
	return rt;
}

ISE_ENG_REG ise_eng_gen_in_size_reg(UINT32 width, UINT32 height)
{
	T_ISE_INPUT_IMAGE_SIZE_REGISTER0 arg;
	ISE_ENG_REG rt = {ISE_INPUT_IMAGE_SIZE_REGISTER0_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_h_size = width;
	arg.bit.ise_v_size = height;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_in_strp_reg(UINT32 st_size, UINT32 ovlp_size)
{
	T_ISE_INPUT_STRIPE_SIZE_REGISTER0 arg;
	ISE_ENG_REG rt = {ISE_INPUT_STRIPE_SIZE_REGISTER0_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_st_size = st_size - 1;
	arg.bit.ise_ovlap_size = ovlp_size;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_in_lofs_reg(UINT32 lofs)
{
	T_ISE_INPUT_DMA_LINEOFFSET_REGISTER0 arg;
	ISE_ENG_REG rt = {ISE_INPUT_DMA_LINEOFFSET_REGISTER0_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_dram_ofsi = (lofs >> 2);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_in_addr_reg(UINT32 phy_addr)
{
	T_ISE_INPUT_DMA_CHANNEL_REGISTER0 arg;
	ISE_ENG_REG rt = {ISE_INPUT_DMA_CHANNEL_REGISTER0_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_dram_sai = phy_addr;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_out_ctrl_reg0(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor, ISE_ENG_SCALE_FILTER_INFO *p_scl_filt)
{
	T_ISE_OUTPUT_CONTROL_REGISTER0 arg;
	ISE_ENG_REG rt = {ISE_OUTPUT_CONTROL_REGISTER0_OFS, 0};

	arg.reg = 0;
	arg.bit.h_ud = p_scl_factor->h_scl_ud;
	arg.bit.v_ud = p_scl_factor->v_scl_ud;
	arg.bit.h_dnrate = p_scl_factor->h_scl_down_rate;
	arg.bit.v_dnrate = p_scl_factor->v_scl_down_rate;
	arg.bit.h_filtmode = p_scl_filt->h_filt_en;
	arg.bit.h_filtcoef = p_scl_filt->h_filt_coef;
	arg.bit.v_filtmode = p_scl_filt->v_filt_en;
	arg.bit.v_filtcoef = p_scl_filt->v_filt_coef;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_out_ctrl_reg1(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_OUTPUT_CONTROL_REGISTER1 arg;
	ISE_ENG_REG rt = {ISE_OUTPUT_CONTROL_REGISTER1_OFS, 0};

	arg.reg = 0;
	arg.bit.h_sfact = p_scl_factor->h_scl_factor;
	arg.bit.v_sfact = p_scl_factor->v_scl_factor;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_out_size_reg(UINT32 width, UINT32 height)
{
	T_ISE_OUTPUT_CONTROL_REGISTER2 arg;
	ISE_ENG_REG rt = {ISE_OUTPUT_CONTROL_REGISTER2_OFS, 0};

	arg.reg = 0;
	arg.bit.h_osize = width;
	arg.bit.v_osize = height;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_out_lofs_reg(UINT32 lofs)
{
	T_ISE_OUTPUT_DMA_LINEOFFSET_REGISTER0 arg;
	ISE_ENG_REG rt = {ISE_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_dram_ofso = (lofs >> 2);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_out_addr_reg(UINT32 phy_addr)
{
	T_ISE_OUTPUT_DMA_CHANNEL_REGISTER0 arg;
	ISE_ENG_REG rt = {ISE_OUTPUT_DMA_CHANNEL_REGISTER0_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_dram_sao = phy_addr;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_ofs_reg0(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_SCALING_START_OFFSET_REGISTER0 arg;
	ISE_ENG_REG rt = {ISE_SCALING_START_OFFSET_REGISTER0_OFS, 0};

	/* always 0,
		refer to ise_base.c ise_set_out_scale_ofs_h_reg/ise_set_out_scale_ofs_v_reg
	*/
	arg.reg = 0;
	arg.bit.ise_hsft_int = 0;
	arg.bit.ise_vsft_int = 0;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_ofs_reg1(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_SCALING_START_OFFSET_REGISTER1 arg;
	ISE_ENG_REG rt = {ISE_SCALING_START_OFFSET_REGISTER1_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_hsft_dec = p_scl_factor->h_scl_ofs;
	arg.bit.ise_vsft_dec = p_scl_factor->v_scl_ofs;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_base_reg(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER3 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER3_OFS, 0};

	arg.reg = 0;
	arg.bit.isd_h_base = p_scl_factor->isd_scl_base_h;
	arg.bit.isd_v_base = p_scl_factor->isd_scl_base_v;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_fact_reg0(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER0 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER0_OFS, 0};

	arg.reg = 0;
	arg.bit.isd_h_sfact0 = p_scl_factor->isd_scl_factor_h[0];
	arg.bit.isd_v_sfact0 = p_scl_factor->isd_scl_factor_v[0];
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_fact_reg1(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER1 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER1_OFS, 0};

	arg.reg = 0;
	arg.bit.isd_h_sfact1 = p_scl_factor->isd_scl_factor_h[1];
	arg.bit.isd_v_sfact1 = p_scl_factor->isd_scl_factor_v[1];
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_fact_reg2(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER2 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_SCALING_NORMALIZATION_FACTOR_REGISTER2_OFS, 0};

	arg.reg = 0;
	arg.bit.isd_h_sfact2 = p_scl_factor->isd_scl_factor_h[2];
	arg.bit.isd_v_sfact2 = p_scl_factor->isd_scl_factor_v[2];
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_ctrl_reg(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_SCALE_USER_COEFFICIENTS_REGISTER0 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_SCALE_USER_COEFFICIENTS_REGISTER0_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_mode = p_scl_factor->isd_coef_mode;
	arg.bit.ise_isd_h_coef_num = p_scl_factor->isd_scl_coef_num_h - 1;
	arg.bit.ise_isd_v_coef_num = p_scl_factor->isd_scl_coef_num_v - 1;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_reg0(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER0 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER0_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_h_coef0 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[0]);
	arg.bit.ise_isd_h_coef1 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[1]);
	arg.bit.ise_isd_h_coef2 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[2]);
	arg.bit.ise_isd_h_coef3 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[3]);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_reg1(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER1 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER1_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_h_coef4 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[4]);
	arg.bit.ise_isd_h_coef5 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[5]);
	arg.bit.ise_isd_h_coef6 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[6]);
	arg.bit.ise_isd_h_coef7 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[7]);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_reg2(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER2 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER2_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_h_coef8 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[8]);
	arg.bit.ise_isd_h_coef9 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[9]);
	arg.bit.ise_isd_h_coef10 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[10]);
	arg.bit.ise_isd_h_coef11 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[11]);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_reg3(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER3 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER3_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_h_coef12 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[12]);
	arg.bit.ise_isd_h_coef13 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[13]);
	arg.bit.ise_isd_h_coef14 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[14]);
	arg.bit.ise_isd_h_coef15 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[15]);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_reg4(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER4 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER4_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_h_coef16 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_h[16])
								+ (ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[16]) << 8);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_reg5(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER5 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER5_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_h_coef_msb0 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[0]);
	arg.bit.ise_isd_h_coef_msb1 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[1]);
	arg.bit.ise_isd_h_coef_msb2 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[2]);
	arg.bit.ise_isd_h_coef_msb3 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[3]);
	arg.bit.ise_isd_h_coef_msb4 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[4]);
	arg.bit.ise_isd_h_coef_msb5 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[5]);
	arg.bit.ise_isd_h_coef_msb6 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[6]);
	arg.bit.ise_isd_h_coef_msb7 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[7]);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_reg6(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER6 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_HORIZONTAL_SCALE_USER_COEFFICIENTS_REGISTER6_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_h_coef_msb8 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[8]);
	arg.bit.ise_isd_h_coef_msb9 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[9]);
	arg.bit.ise_isd_h_coef_msb10 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[10]);
	arg.bit.ise_isd_h_coef_msb11 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[11]);
	arg.bit.ise_isd_h_coef_msb12 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[12]);
	arg.bit.ise_isd_h_coef_msb13 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[13]);
	arg.bit.ise_isd_h_coef_msb14 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[14]);
	arg.bit.ise_isd_h_coef_msb15 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_h[15]);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_reg0(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER0 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER0_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_v_coef0 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[0]);
	arg.bit.ise_isd_v_coef1 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[1]);
	arg.bit.ise_isd_v_coef2 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[2]);
	arg.bit.ise_isd_v_coef3 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[3]);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_reg1(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER1 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER1_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_v_coef4 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[4]);
	arg.bit.ise_isd_v_coef5 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[5]);
	arg.bit.ise_isd_v_coef6 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[6]);
	arg.bit.ise_isd_v_coef7 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[7]);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_reg2(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER2 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER2_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_v_coef8 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[8]);
	arg.bit.ise_isd_v_coef9 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[9]);
	arg.bit.ise_isd_v_coef10 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[10]);
	arg.bit.ise_isd_v_coef11 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[11]);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_reg3(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER3 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER3_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_v_coef12 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[12]);
	arg.bit.ise_isd_v_coef13 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[13]);
	arg.bit.ise_isd_v_coef14 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[14]);
	arg.bit.ise_isd_v_coef15 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[15]);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_reg4(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER4 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER4_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_v_coef16 = ise_eng_int_cal_isd_coef_val_lsb(p_scl_factor->isd_user_coef_v[16])
								+ (ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[16]) << 8);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_reg5(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER5 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER5_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_v_coef_msb0 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[0]);
	arg.bit.ise_isd_v_coef_msb1 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[1]);
	arg.bit.ise_isd_v_coef_msb2 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[2]);
	arg.bit.ise_isd_v_coef_msb3 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[3]);
	arg.bit.ise_isd_v_coef_msb4 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[4]);
	arg.bit.ise_isd_v_coef_msb5 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[5]);
	arg.bit.ise_isd_v_coef_msb6 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[6]);
	arg.bit.ise_isd_v_coef_msb7 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[7]);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_reg6(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER6 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_VERTICAL_SCALE_USER_COEFFICIENTS_REGISTER6_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_v_coef_msb8 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[8]);
	arg.bit.ise_isd_v_coef_msb9 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[9]);
	arg.bit.ise_isd_v_coef_msb10 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[10]);
	arg.bit.ise_isd_v_coef_msb11 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[11]);
	arg.bit.ise_isd_v_coef_msb12 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[12]);
	arg.bit.ise_isd_v_coef_msb13 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[13]);
	arg.bit.ise_isd_v_coef_msb14 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[14]);
	arg.bit.ise_isd_v_coef_msb15 = ise_eng_int_cal_isd_coef_val_msb(p_scl_factor->isd_user_coef_v[15]);
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_all(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER7 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER7_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_h_coef_all = p_scl_factor->isd_coef_sum_all_h;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_h_half(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER8 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_HORIZONTAL_SCALING_USER_COEFFICIENTS_REGISTER8_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_h_coef_half = p_scl_factor->isd_coef_sum_half_h;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_all(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER7 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER7_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_v_coef_all = p_scl_factor->isd_coef_sum_all_v;
	rt.val = arg.reg;

	return rt;
}

ISE_ENG_REG ise_eng_gen_scl_isd_coef_v_half(ISE_ENG_SCALE_FACTOR_INFO *p_scl_factor)
{
	T_ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER8 arg;
	ISE_ENG_REG rt = {ISE_INTEGRATION_VERTICAL_SCALING_USER_COEFFICIENTS_REGISTER8_OFS, 0};

	arg.reg = 0;
	arg.bit.ise_isd_v_coef_half = p_scl_factor->isd_coef_sum_half_v;
	rt.val = arg.reg;

	return rt;
}

#if 0
#endif

void ise_eng_dump(void)
{
	UINT32 i = 0, j = 0, str_len = 0;
	CHAR out_ch;
	CHAR str_dumpmem[64];
	UINT32 va = 0;
	UINT32 length = 0;
	ISE_ENG_HANDLE *p_eng = NULL;

	DBG_DUMP("----- [ISE_ENG] DUMP START -----\r\n");

	p_eng = ise_eng_get_handle(KDRV_CHIP0, KDRV_GFX2D_ISE0);
	if (p_eng == NULL) {
		DBG_DUMP("null engine handle\r\n");
		return ;
	}

	va = p_eng->reg_io_base;
	length = ISE_ENG_REG_NUMS * 4;
	DBG_DUMP("dump reg_io_base=%08x length=%08x to console:\r\n", (unsigned int)va, (unsigned int)length);
	for (i = 0; i < length; i++) {
		memset((void *)str_dumpmem, 0, 64);
		str_len = snprintf(str_dumpmem, 64, "%08X : %08X %08X %08X %08X  ", (unsigned int)va, *((unsigned int *)va),
				*((unsigned int *)(va+4)), *((unsigned int *)(va+8)), *((unsigned int *)(va+12)));
		for (j = 0; j < 16; j++) {
			out_ch = *((char *)(va+j));
			if (((UINT32)out_ch < 0x20) || ((UINT32)out_ch >= 0x80))
				str_len += snprintf(str_dumpmem+str_len, 64-str_len, ".");
			else
				str_len += snprintf(str_dumpmem+str_len, 64-str_len, "%c", out_ch);
		}
		DBG_DUMP("%s\r\n", str_dumpmem);
		i += 16;
		va += 16;
	}

	DBG_DUMP("----- [ISE_ENG] DUMP END -----\r\n");
}
