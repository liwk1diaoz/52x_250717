#include "kflow_videocapture/ctl_sie.h"
#include "ctl_sie_id_int.h"
#include "ctl_sie_utility_int.h"
#include "ctl_sie_event_id_int.h"

THREAD_HANDLE g_ctl_sie_buf_tsk_id;
THREAD_HANDLE g_ctl_sie_isp_tsk_id;
ID g_ctl_sie_buf_flg_id = 0;
ID g_ctl_sie_isp_flg_id = 0;
ID g_ctl_sie_flg_id[CTL_SIE_MAX_SUPPORT_ID] = {0};

ID ctl_sie_get_isp_flag_id(void)
{
	return g_ctl_sie_isp_flg_id;
}

ID ctl_sie_get_buf_flag_id(void)
{
	return g_ctl_sie_buf_flg_id;
}

ID ctl_sie_get_flag_id(CTL_SIE_ID id)
{
	if (!ctl_sie_module_chk_id_valid(id)) {
		return 0;
	}
	return g_ctl_sie_flg_id[id];
}

void ctl_sie_install_id(void)
{
	UINT32 i;

	// ctl sie buffer flg
	vos_flag_create(&g_ctl_sie_buf_flg_id, NULL, "ctl_sie_buf");

	// ctl sie flag
	for (i = 0; i < CTL_SIE_MAX_SUPPORT_ID; i++) {
		vos_flag_create(&g_ctl_sie_flg_id[i], NULL, "ctl_sie");
		vos_flag_set(ctl_sie_get_flag_id(i), CTL_SIE_FLG_INIT);
	}
}

void ctl_sie_isp_install_id(void)
{
	// ctl sie buffer flg
	vos_flag_create(&g_ctl_sie_isp_flg_id, NULL, "ctl_sie_isp");
}

void ctl_sie_uninstall_id(void)
{
	UINT32 i;

	vos_flag_destroy(g_ctl_sie_buf_flg_id);
	for (i = CTL_SIE_ID_1; i < CTL_SIE_MAX_SUPPORT_ID; i++) {
		vos_flag_destroy(ctl_sie_get_flag_id(i));
	}
}

void ctl_sie_isp_uninstall_id(void)
{
	vos_flag_destroy(g_ctl_sie_isp_flg_id);
}

