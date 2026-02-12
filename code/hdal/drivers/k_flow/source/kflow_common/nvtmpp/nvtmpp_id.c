#include "nvtmpp_int.h"
#include "nvtmpp_id.h"

SEM_HANDLE NVTMPP_SEM_ID;
SEM_HANDLE NVTMPP_HEAP_SEM_ID;
SEM_HANDLE NVTMPP_PROC_SEM_ID;
ID         NVTMPP_FLAG_ID;

void nvtmpp_install_id(void)
{
	OS_CONFIG_SEMPHORE(NVTMPP_SEM_ID, 0, 1, 1);
	OS_CONFIG_SEMPHORE(NVTMPP_HEAP_SEM_ID, 0, 1, 1);
	#ifdef __KERNEL__
	SEM_CREATE(NVTMPP_PROC_SEM_ID, 1);
	#endif
	vos_flag_create(&NVTMPP_FLAG_ID, NULL, "nvtmpp_flg");
}

void nvtmpp_uninstall_id(void)
{
	SEM_DESTROY(NVTMPP_SEM_ID);
	SEM_DESTROY(NVTMPP_HEAP_SEM_ID);
	#ifdef __KERNEL__
	SEM_DESTROY(NVTMPP_PROC_SEM_ID);
	#endif
	vos_flag_destroy(NVTMPP_FLAG_ID);
}

