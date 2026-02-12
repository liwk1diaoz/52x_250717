/**
    ctrl SIE utility

    @file       ctl_sie_utility_int.c
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "ctl_sie_utility_int.h"

#define CTL_SIE_TAG(ch0, ch1, ch2, ch3) ((UINT32)(UINT8)(ch0) |((UINT32)(UINT8)(ch1) << 8) |((UINT32)(UINT8)(ch2) << 16) |((UINT32)(UINT8)(ch3) << 24))
#define CTL_SIE_HDL_TAG CTL_SIE_TAG('C', 'S', 'I', 'E')

BOOL ctl_sie_module_chk_id_valid(CTL_SIE_ID id)
{
	if (id >= CTL_SIE_MAX_SUPPORT_ID) {
		ctl_sie_dbg_err("illegal id %d > maximum id %d\r\n", (int)(id), (int)(CTL_SIE_MAX_SUPPORT_ID - 1));
		return FALSE;
	}
	return TRUE;
}

static INT32 ctl_sie_chk_hdl(UINT32 hdl)
{
	CTL_SIE_HDL *ptr = (CTL_SIE_HDL *)hdl;

	if (hdl == 0) {
		ctl_sie_dbg_err("input handle zero\r\n");
		return CTL_SIE_E_HDL;
	}

	if (ptr->tag != CTL_SIE_HDL_TAG) {
		ctl_sie_dbg_err("check handle fail\r\n");
		return CTL_SIE_E_HDL;
	}
	return CTL_SIE_E_OK;
}

UINT32 ctl_sie_get_hdl_tag(void)
{
	return CTL_SIE_HDL_TAG;
}

CTL_SIE_ID ctl_sie_hdl_conv2_id(UINT32 hdl)
{
	CTL_SIE_HDL *ctl_sie_hdl;

	if (ctl_sie_chk_hdl(hdl) != CTL_SIE_E_OK) {
		return CTL_SIE_MAX_SUPPORT_ID;
	}

	ctl_sie_hdl = (CTL_SIE_HDL *)hdl;

	return ctl_sie_hdl->id;
}

BOOL ctl_sie_module_chk_is_raw(CTL_SEN_MODE_TYPE mode_type)
{
	switch (mode_type) {
	case CTL_SEN_MODE_CCIR:
	case CTL_SEN_MODE_CCIR_INTERLACE:
		return FALSE;

	case CTL_SEN_MODE_LINEAR:
	case CTL_SEN_MODE_BUILTIN_HDR:
	case CTL_SEN_MODE_STAGGER_HDR:
	case CTL_SEN_MODE_PDAF:
	default:
//		ctl_sie_dbg_wrn("N.S. mode type %d\r\n", (int)(mode_type));
		return TRUE;
	}
}

UINT32 ctl_sie_uint64_dividend(UINT64 dividend, UINT32 divisor)
{
	if (divisor == 0) {
		return 0;
	}

#if defined (__FREERTOS)
	return (UINT32)(dividend / divisor);
#else
	do_div(dividend, divisor);
	return (UINT32)(dividend);
#endif
}

UINT32 ctl_sie_max(UINT32 x, UINT32 y)
{
	if (x >= y) {
		return x;
	} else {
		return y;
	}
}

UINT32 ctl_sie_min(UINT32 x, UINT32 y)
{
	if (x <= y) {
		return x;
	} else {
		return y;
	}
}

UINT32 ctl_sie_lcm(UINT32 a, UINT32 b)
{
	UINT64 input_a, input_b;
	UINT64 tmp;

	if (a == 0) {
		a++;
	}
	if (b == 0) {
		b++;
	}

	input_a = a;
	input_b = b;

	while (b != 0) {
		tmp = a % b;
		a = b;
		b = tmp;
	}

	return CTL_SIE_DIV_U64(input_a * input_b, a);
}

BOOL ctl_sie_chk_align(UINT32 src, UINT32 align)
{
	if ((src == 0) || (align == 0)) {
		return TRUE;
	}
	if ((src % align) == 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

UINT64 ctl_sie_util_get_syst_timestamp(void)
{
	return hwclock_get_longcounter();
}

UINT32 ctl_sie_util_get_timestamp(void)
{
	return hwclock_get_counter();
}

INT32 ctl_sie_util_os_malloc(CTL_SIE_VOS_MEM_INFO *vod_mem_info, UINT32 req_size)
{
	if (vos_mem_init_cma_info(&vod_mem_info->cma_info, VOS_MEM_CMA_TYPE_CACHE, req_size) != 0) {
		DBG_ERR("init cma failed\r\n");
		return CTL_SIE_E_NOMEM;
	}

	vod_mem_info->cma_hdl = vos_mem_alloc_from_cma(&vod_mem_info->cma_info);
	if (vod_mem_info->cma_hdl == NULL) {
		DBG_ERR("alloc cma failed\r\n");
		return CTL_SIE_E_NOMEM;
	}

	return CTL_SIE_E_OK;
}

INT32 ctl_sie_util_os_mfree(CTL_SIE_VOS_MEM_INFO *vod_mem_info)
{
	if (vos_mem_release_from_cma(vod_mem_info->cma_hdl) != 0) {
		DBG_ERR("release cma failed\r\n");
		return CTL_SIE_E_NOMEM;
	}
	return CTL_SIE_E_OK;
}

