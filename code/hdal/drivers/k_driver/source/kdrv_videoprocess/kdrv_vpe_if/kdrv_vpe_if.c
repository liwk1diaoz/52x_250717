#include "kwrap/error_no.h"
#include "kdrv_videoprocess/kdrv_vpe_if.h"

static KDRV_VPE_IF_OBJ g_vpe_if_ctl_obj;

INT32 kdrv_vpe_if_trigger(UINT32 handle, KDRV_VPE_JOB_LIST *p_job_list)
{
	if (g_vpe_if_ctl_obj.trigger) {
		return g_vpe_if_ctl_obj.trigger(handle, p_job_list);
	}
	return E_OBJ;
}

INT32 kdrv_vpe_if_get(UINT32 handle, UINT32 param_id, void *p_param)
{
	if (g_vpe_if_ctl_obj.get) {
		return g_vpe_if_ctl_obj.get(handle, param_id, p_param);
	}
	return E_OBJ;
}

INT32 kdrv_vpe_if_set(UINT32 handle, UINT32 param_id, void *p_param)
{
	if (g_vpe_if_ctl_obj.set) {
		return g_vpe_if_ctl_obj.set(handle, param_id, p_param);
	}
	return E_OBJ;
}

INT32 kdrv_vpe_if_open(UINT32 chip, UINT32 engine)
{
	if (g_vpe_if_ctl_obj.open) {
		return g_vpe_if_ctl_obj.open(chip, engine);
	}
	return E_OBJ;
}

INT32 kdrv_vpe_if_close(UINT32 handle)
{
	if (g_vpe_if_ctl_obj.close) {
		return g_vpe_if_ctl_obj.close(handle);
	}
	return E_OBJ;
}

BOOL kdrv_vpe_if_is_init(void)
{
	if (g_vpe_if_ctl_obj.open == NULL ||
		g_vpe_if_ctl_obj.open == NULL ||
		g_vpe_if_ctl_obj.open == NULL ||
		g_vpe_if_ctl_obj.open == NULL ||
		g_vpe_if_ctl_obj.open == NULL) {
		return FALSE;
	}
	return TRUE;
}

#if 0
#endif

INT32 kdrv_vpe_if_register(KDRV_VPE_IF_OBJ *p_obj)
{
	if (p_obj) {
		g_vpe_if_ctl_obj = *p_obj;
	}

	return E_OK;
}

INT32 kdrv_vpe_if_init(void)
{
	g_vpe_if_ctl_obj.open = NULL;
	g_vpe_if_ctl_obj.close = NULL;
	g_vpe_if_ctl_obj.get = NULL;
	g_vpe_if_ctl_obj.set = NULL;
	g_vpe_if_ctl_obj.trigger = NULL;

	return E_OK;
}

INT32 kdrv_vpe_if_exit(void)
{
	g_vpe_if_ctl_obj.open = NULL;
	g_vpe_if_ctl_obj.close = NULL;
	g_vpe_if_ctl_obj.get = NULL;
	g_vpe_if_ctl_obj.set = NULL;
	g_vpe_if_ctl_obj.trigger = NULL;

	return E_OK;
}