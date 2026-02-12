/**
    UCDC, APIs declare.

    @file       USBCDC.h
    @ingroup    mUCDC
    @note       --

    Copyright   Novatek Microelectronics Corp. 2021.  All rights reserved.
*/

#ifndef _UCDCAPI_H
#define _UCDCAPI_H

/**
    Supported CDC PSTN request codes.

    This definition is used for CDC_PSTN_REQUEST_CB.
*/
typedef enum _CDC_PSTN_REQUEST {
	REQ_SET_LINE_CODING         =    0x20,
	REQ_GET_LINE_CODING         =    0x21,
	REQ_SET_CONTROL_LINE_STATE  =    0x22,
	REQ_SEND_BREAK              =    0x23,
	ENUM_DUMMY4WORD(CDC_PSTN_REQUEST)
} CDC_PSTN_REQUEST;

/**
     Line coding structure.
*/
typedef _PACKED_BEGIN struct {
	UINT32   uiBaudRateBPS; ///< Data terminal rate, in bits per second.
	UINT8    uiCharFormat;  ///< Stop bits.
	UINT8    uiParityType;  ///< Parity.
	UINT8    uiDataBits;    ///< Data bits (5, 6, 7, 8 or 16).
} _PACKED_END CDCLineCoding;

#endif

