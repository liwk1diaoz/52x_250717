/**
	@brief Header file of vendor basic type definition.\n

	@file vendor_type.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _ETHCAM_TYPE_H_
#define _ETHCAM_TYPE_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define ETHCAM_META_SIGN MAKEFOURCC('E','C','M','T')

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/
/**
	dummy data descriptor.
*/
typedef struct _ETHCAM_META_HEADER {
	UINT32              sign;             		///< signature for meta alloc
	UINT32              data_size;        	///< data size for total frame
	UINT32              count_hi;             	///< frame count
	UINT32              count_lo;             	///< frame count
	UINT32              timestamp_hi;          ///< decode timestamp (unit: microsecond)
	UINT32              timestamp_lo;          ///< decode timestamp (unit: microsecond)
	UINT32              reserved[10];
} ETHCAM_META_HEADER;


/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
#endif

