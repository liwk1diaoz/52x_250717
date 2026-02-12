#ifndef HAL_REMOTE_MAIN_H
#define HAL_REMOTE_MAIN_H
#include "hal_remote_op.h"
#include <kwrap/type.h>

INT32 HAL_Remote_Init(void);

INT32 HAL_Remote_Deinit(void);

INT32 HAL_Remote_GetRawData(REMOTE_RAW_DATA *RemoteData, INT32 timeoutMsec);

INT32 HAL_Remote_SetWorkMode(void);

INT32 HAL_Remote_GetTestString(void);

#endif /* End of HAL_REMOTE_MAIN_H */
