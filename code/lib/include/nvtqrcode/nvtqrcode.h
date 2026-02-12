
#include "hdal.h"
#include <kwrap/debug.h>
#include <kwrap/error_no.h>

typedef struct {
    HD_VIDEO_FRAME *pVdoFrm;
    void   *pQrcodeBuf;
    UINT32 u32QrcodeBufSize;
    BOOL   bEnDbgMsg;
} NVTQRCODE_INFO;

extern ER nvtqrcode_scan(NVTQRCODE_INFO *pQrCodeInfo);