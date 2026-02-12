#include "kdf_ipp_int.h"
#include "kdf_ipp_id_int.h"

THREAD_HANDLE g_kdf_ipp_tsk_id;
ID g_kdf_ipp_flg_id = 0;

void kdf_ipp_install_id(void)
{
	OS_CONFIG_FLAG(g_kdf_ipp_flg_id);
}

void kdf_ipp_uninstall_id(void)
{
	rel_flg(g_kdf_ipp_flg_id);
}


