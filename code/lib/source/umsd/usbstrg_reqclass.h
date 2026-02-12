/*
    USB MSDC class-specific Request handler Header File

    USB Mass-Storage-Device-Class Task class-specific Request handler Header File.

    @file       UsbStrgReqClass.h
    @ingroup    mILibUsbMSDC
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _USBSTRGREQCLASS_H
#define _USBSTRGREQCLASS_H

//
//  class-specific requests
//

/* Bulk-Only Mas Storage Reset */
#define USTRG_CSR_RESET_REQTYPE     0x21    /* bm_request_type                */
#define USTRG_CSR_RESET_REQUEST     0xff    /* b_request                     */
#define USTRG_CSR_RESET_VALUE       0x00    /* w_value                       */
#define USTRG_CSR_RESET_INEDEX      0x00    /* w_index                       */
#define USTRG_CSR_RESET_LENGTH      0x00    /* w_length                      */

/* Get Max LUN */
#define USTRG_CSR_MAXLUN_REQTYPE    0xa1    /* bm_request_type                */
#define USTRG_CSR_MAXLUN_REQUEST    0xfe    /* b_request                     */
#define USTRG_CSR_MAXLUN_VALUE      0x00    /* w_value                       */
#define USTRG_CSR_MAXLUN_INEDEX     0x00    /* w_index                       */
#define USTRG_CSR_MAXLUN_LENGTH     0x01    /* w_length                      */


void U2Msdc_ClassRequestHandler(void);

#endif
