#ifndef _ETHCAMCMDPARSERINT_H_
#define _ETHCAMCMDPARSERINT_H_

//#include "SysKer.h"
#include "EthCam/EthCamSocket.h"
#include "kwrap/flag.h"

extern ID _SECTION(".kercfg_data") FLG_ID_ETHCAMCMD[ETHCAM_PATH_ID_MAX];
extern ID _SECTION(".kercfg_data") ETHCAMCMD_SEM_ID[ETHCAM_PATH_ID_MAX];
extern ID _SECTION(".kercfg_data") ETHCAMSTR_SEM_ID[ETHCAM_PATH_ID_MAX];

#endif //_ETHCAMCMDPARSERINT_H_

