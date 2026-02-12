#include "hal_remote_main.h"
#include <string.h>
#include "kwrap/error_no.h"
#include <kwrap/debug.h>

static REMOTE_OBJ g_RemoteObj = { {"/dev/ttyS3"} };  //ttyS3 = UART4

INT32 HAL_Remote_Init(void)
{
    HAL_Remote_GetOp(&g_RemoteObj.op);
    return g_RemoteObj.op.init(&g_RemoteObj.data);
}

INT32 HAL_Remote_Deinit(void)
{
    return g_RemoteObj.op.deinit();
}

INT32 HAL_GPS_GetRawData(REMOTE_RAW_DATA *pRemoteData, INT32 timeoutMsec)
{
    return g_RemoteObj.op.getRawData(pRemoteData, timeoutMsec);
}

INT32 HAL_Remote_SendTestString(void)
{
    return g_RemoteObj.op.SendTestString();
}

INT32 HAL_Remote_GetTestString(void)
{
    REMOTE_RAW_DATA remoteData = {0};
    INT32 ret;
    UINT32 u32Cnt = 10;
    UINT8  recvChar = 0;

    while(u32Cnt) {
        ret = HAL_GPS_GetRawData(&remoteData, 0);
        if (ret == E_OK) {
            recvChar = remoteData.rawData[0];
            DBG_DUMP("recvChar = %c\r\n", recvChar);
            u32Cnt--;
        }
    }

    DBG_DUMP("%s: Exit\r\n", __func__);

    return E_OK;
}


