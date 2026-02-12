//#include "SysKer.h"
#include "EthCamCmdParser.h"
#include "EthCamCmdParserInt.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include <stdio.h>
#include <string.h>

ID FLG_ID_ETHCAMCMD[ETHCAM_PATH_ID_MAX] = {0};
ID ETHCAMCMD_SEM_ID[ETHCAM_PATH_ID_MAX] = {0};
ID ETHCAMSTR_SEM_ID[ETHCAM_PATH_ID_MAX] = {0};

void EthCamCmd_InstallID(UINT32 path_cnt)
{
	UINT32 i;
	T_CFLG cflg ;
	char name[40];

	for(i=0;i<path_cnt;i++){
		snprintf(name, sizeof(name) - 1, "FLG_ID_ETHCAMCMD_%d",i);
		vos_flag_create(&FLG_ID_ETHCAMCMD[i], &cflg, name);
		snprintf(name, sizeof(name) - 1, "ETHCAMCMD_SEM_ID_%d",i);
		vos_sem_create(&ETHCAMCMD_SEM_ID[i], 1, name);
		snprintf(name, sizeof(name) - 1, "ETHCAMSTR_SEM_ID_%d",i);
		vos_sem_create(&ETHCAMSTR_SEM_ID[i], 1, name);
		//OS_CONFIG_FLAG(FLG_ID_ETHCAMCMD[i]);
		//OS_CONFIG_SEMPHORE(ETHCAMCMD_SEM_ID[i], 0, 1, 1);
		//OS_CONFIG_SEMPHORE(ETHCAMSTR_SEM_ID[i], 0, 1, 1);
	}
}

