#if defined(__FREERTOS)
#include <stdlib.h>
#else
#include <linux/slab.h>
#include <linux/vmalloc.h>
#if defined(CONFIG_PWM_NA51055)
#include <linux/pwm.h>
#endif
#endif
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "isp_dbg.h"

//=============================================================================
// define
//=============================================================================
#if defined(__KERNEL__) && defined(CONFIG_PWM_NA51055)
#define NUM_PWM_CH      12
#endif

//=============================================================================
// global
//=============================================================================
#if defined(__KERNEL__) && defined(CONFIG_PWM_NA51055)
struct pwm_device *nvt_pwm_dev[NUM_PWM_CH];
struct pwm_state nvt_pwm_state;
static BOOL nvt_pwm_enable[NUM_PWM_CH];
#endif

//=============================================================================
// function declaration
//=============================================================================

//=============================================================================
// extern functions
//=============================================================================
void *isp_uti_kmem_alloc(UINT32 mem_size)
{
	#if defined(__FREERTOS)
	return malloc(mem_size);
	#else
	return kzalloc(mem_size, GFP_KERNEL);
	#endif
}

void isp_uti_kmem_free(void *mem_addr)
{
	#if defined(__FREERTOS)
	free(mem_addr);
	#else
	kfree(mem_addr);
	#endif
}

void *isp_uti_vmem_alloc(UINT32 mem_size)
{
	#if defined(__FREERTOS)
	return malloc(mem_size);
	#else
	return vmalloc(mem_size);
	#endif
}

void isp_uti_vmem_free(void *mem_addr)
{
	#if defined(__FREERTOS)
	free(mem_addr);
	#else
	vfree(mem_addr);
	#endif
}

ER isp_uti_enable_pwm(UINT32 pwm_id, UINT32 period, UINT32 duty_cycle, UINT32 polarity)
{
	#if defined(__KERNEL__) && defined(CONFIG_PWM_NA51055)
	CHAR lable[20];

	if (pwm_id > (NUM_PWM_CH - 1)) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "pwm_id (%d) must less than max id (%d) \r\n", pwm_id, NUM_PWM_CH - 1);
		return E_SYS;
	}

	if (duty_cycle > period) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "duty_cycle (%d) must less than period (%d) \r\n", duty_cycle, period);
		return E_SYS;
	}

	if (polarity > 1) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "polarity (%d) illegal \r\n", polarity);
		return E_SYS;
	}

	if (nvt_pwm_enable[pwm_id] == FALSE) {
		sprintf(lable, "nvt_isp_pwm_%d", (int)pwm_id);
		nvt_pwm_dev[pwm_id] = pwm_request(pwm_id, lable);

		if ((nvt_pwm_dev[pwm_id] == NULL) || (nvt_pwm_dev[pwm_id] == (void *)0xFFFFFFF0)) {
			PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "pwm request fail, id (%d), lable (%s), dev (0x%X) \r\n", pwm_id, lable, (unsigned int)nvt_pwm_dev[pwm_id]);
			return E_SYS;
		}

		nvt_pwm_state.period = period;
		nvt_pwm_state.duty_cycle = duty_cycle;
		nvt_pwm_state.polarity = polarity; //PWM_POLARITY_NORMAL: 0
		nvt_pwm_state.enabled = 1;
		pwm_apply_state(nvt_pwm_dev[pwm_id], &nvt_pwm_state);

		nvt_pwm_enable[pwm_id] = TRUE;

		return E_OK;
	} else {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "enable pwm %d but has been opened \r\n", pwm_id);
		return E_SYS;
	}
	#else
	#if defined(__FREERTOS)
	PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "not support in RTOS \r\n");
	#else
	PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "not support, must enable CONFIG_PWM \r\n");
	#endif
	return E_NOSPT;
	#endif
}

ER isp_uti_set_pwm(UINT32 pwm_id, UINT32 period, UINT32 duty_cycle, UINT32 polarity)
{
	#if defined(__KERNEL__) && defined(CONFIG_PWM_NA51055)
	if (pwm_id > (NUM_PWM_CH - 1)) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "pwm_id (%d) must less than max id (%d) \r\n", pwm_id, NUM_PWM_CH - 1);
		return E_SYS;
	}

	if (duty_cycle > period) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "duty_cycle (%d) must less than period (%d) \r\n", duty_cycle, period);
		return E_SYS;
	}

	if (polarity > 1) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "polarity (%d) illegal \r\n", polarity);
		return E_SYS;
	}

	if (nvt_pwm_enable[pwm_id] == TRUE) {
		nvt_pwm_state.period = period;
		nvt_pwm_state.duty_cycle = duty_cycle;
		nvt_pwm_state.polarity = polarity;
		nvt_pwm_state.enabled = 1;
		pwm_apply_state(nvt_pwm_dev[pwm_id], &nvt_pwm_state);
		pwm_reload(nvt_pwm_dev[pwm_id]);

		return E_OK;
	} else {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "set pwm %d but has been closed \r\n", pwm_id);
		return E_SYS;
	}
	#else
	#if defined(__FREERTOS)
	PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "not support in RTOS \r\n");
	#else
	PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "not support, must enable CONFIG_PWM \r\n");
	#endif
	return E_NOSPT;
	#endif
}

ER isp_uti_disable_pwm(UINT32 pwm_id)
{
	#if defined(__KERNEL__) && defined(CONFIG_PWM_NA51055)
	if (pwm_id > (NUM_PWM_CH - 1)) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "pwm_id (%d) must less than max id (%d) \r\n", pwm_id, NUM_PWM_CH - 1);
		return E_SYS;
	}

	if (nvt_pwm_enable[pwm_id] == TRUE) {
		pwm_disable(nvt_pwm_dev[pwm_id]);
		pwm_reload(nvt_pwm_dev[pwm_id]);
		pwm_free(nvt_pwm_dev[pwm_id]);
		nvt_pwm_dev[pwm_id] = NULL;

		nvt_pwm_enable[pwm_id] = FALSE;

		return E_OK;
	} else {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "diable pwm %d but has been opened \r\n", pwm_id);
		return E_SYS;
	}
	#else
	#if defined(__FREERTOS)
	PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "not support in RTOS \r\n");
	#else
	PRINT_ISP_WRN(isp_dbg_get_dbg_mode(pwm_id) & ISP_DBG_WRN_MSG, "not support, must enable CONFIG_PWM \r\n");
	#endif
	return E_NOSPT;
	#endif
}

#if defined(__KERNEL__)
EXPORT_SYMBOL(isp_uti_kmem_alloc);
EXPORT_SYMBOL(isp_uti_kmem_free);
EXPORT_SYMBOL(isp_uti_vmem_alloc);
EXPORT_SYMBOL(isp_uti_vmem_free);
EXPORT_SYMBOL(isp_uti_enable_pwm);
EXPORT_SYMBOL(isp_uti_set_pwm);
EXPORT_SYMBOL(isp_uti_disable_pwm);
#endif


