#include "kflow_videoprocess/ctl_ipp.h"
#include "ctl_ipp_id_int.h"
#include "ipp_event_id_int.h"

THREAD_HANDLE g_ctl_ipp_tsk_id;
ID g_ctl_ipp_flg_id = 0;

THREAD_HANDLE g_ctl_ipp_buf_tsk_id;
ID g_ctl_ipp_buf_flg_id = 0;

THREAD_HANDLE g_ctl_ipp_ise_tsk_id;
ID g_ctl_ipp_ise_flg_id = 0;

THREAD_HANDLE g_ctl_ipp_isp_tsk_id;
ID g_ctl_ipp_isp_flg_id = 0;

void ctl_ipp_install_id(void)
{
	OS_CONFIG_FLAG(g_ctl_ipp_flg_id);
	OS_CONFIG_FLAG(g_ctl_ipp_buf_flg_id);
	OS_CONFIG_FLAG(g_ctl_ipp_ise_flg_id);
	OS_CONFIG_FLAG(g_ctl_ipp_isp_flg_id);
}

void ctl_ipp_uninstall_id(void)
{
	rel_flg(g_ctl_ipp_flg_id);
	rel_flg(g_ctl_ipp_buf_flg_id);
	rel_flg(g_ctl_ipp_ise_flg_id);
	rel_flg(g_ctl_ipp_isp_flg_id);
}


