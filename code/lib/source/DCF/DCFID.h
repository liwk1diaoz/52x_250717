#ifndef _DCF_ID_H
#define _DCF_ID_H
#include "kwrap/type.h"

#define DCF_HANDLE_NUM                 2
extern ID _SECTION(".kercfg_data") SEMID_DCF_COMM;
extern ID _SECTION(".kercfg_data") SEMID_DCF[DCF_HANDLE_NUM];
extern ID _SECTION(".kercfg_data") SEMID_DCFNEXTID[DCF_HANDLE_NUM];
extern ID _SECTION(".kercfg_data") FLG_ID_DCF[DCF_HANDLE_NUM];
#endif //_DCF_ID_H
