/*++

Copyright (c) 2003  Novatek Microelectronics Corporation

Module Name:

    UvacReqVendor.h

Abstract:

    The header file for USB Vendor command handling.

Environment:

    For nt96400

Notes:

  Copyright (c) 2003 Novatek Microelectronics Corporation.  All Rights Reserved.


Revision History:

    10/31/03: Ceated by Alex Sun
--*/


#ifndef _U2UVACREQVENDOR_H
#define _U2UVACREQVENDOR_H

#include "UVAC.h"

#define UVAC_VENDREQ_IQ_MASK        0xE0    //0xE0 ~ 0xFF
#define UVAC_VENDREQ_CUST_MASK      0xC0    //0xC0 ~ 0xDF
#define UVAC_PATCH_CXOUT            1    //patch solution for cx out glitch issue of usd driver

extern UVAC_VENDOR_REQ_CB g_fpU2UvacVendorReqCB;
extern UVAC_VENDOR_REQ_CB g_fpU2UvacVendorReqIQCB;
extern void U2UvacVendorRequestHandler(UINT32 uiRequest);

#endif
