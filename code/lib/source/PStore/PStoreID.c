
#include "PStore.h"
#include "ps_int.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

ID FLG_ID_PSTORE = 0;
ID SEMID_PS_SEC1 = 0;
ID SEMID_PS_SEC2 = 0;

void PStore_InstallID(void)
{
        OS_CONFIG_FLAG(FLG_ID_PSTORE);
        OS_CONFIG_SEMPHORE(SEMID_PS_SEC1, 0, 1, 1);
        OS_CONFIG_SEMPHORE(SEMID_PS_SEC2, 0, 1, 1);
}

void PStore_UninstallID(void)
{
	vos_flag_destroy(FLG_ID_PSTORE);
	SEM_DESTROY(SEMID_PS_SEC1);
	SEM_DESTROY(SEMID_PS_SEC2);
}
