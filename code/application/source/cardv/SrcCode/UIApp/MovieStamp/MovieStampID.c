#include "MovieStampID.h"
#include "MovieStampInt.h"
#include "MovieStampAPI.h"

//#define PRI_MOVIESTAMP          10
//#define STKSIZE_MOVIESTAMP      4096

THREAD_HANDLE MOVIESTAMPTSK_ID = 0;
ID FLG_ID_MOVIESTAMP = 0;
ID FTYPEIPC_SEM_ID = 0;

void MovieStamp_InstallID(void)
{
	vos_flag_create(&FLG_ID_MOVIESTAMP, NULL, "FLG_ID_MOVIESTAMP");
	vos_sem_create(&FTYPEIPC_SEM_ID, 1, "FTYPEIPC_SEM_ID");

}
void MovieStamp_UnInstallID(void)
{
	vos_flag_destroy(FLG_ID_MOVIESTAMP);
	vos_sem_destroy(FTYPEIPC_SEM_ID);
}
