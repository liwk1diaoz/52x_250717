#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <umsd.h>
#include <msdcnvt/MsdcNvtApi.h>
#include <kwrap/debug.h>
#include "MsdcNvtCbCom.h"

BOOL m_IsUsbConnected = FALSE;
BOOL m_IsComConnected = FALSE;

UINT8 *MsdcNvtCb_GetTransAddr(void)
{
	if (m_IsComConnected) {
		return MsdcNvtCbCom_GetTransAddr();
	} else {
		return MsdcNvt_GetDataBufferAddress();
	}
}

UINT32 MsdcNvtCb_GetTransSize(void)
{
	if (m_IsComConnected) {
		return MsdcNvtCbCom_GetTransSize();
	} else {
		return MsdcNvt_GetTransSize();
	}
}