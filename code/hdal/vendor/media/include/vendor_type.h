/**
	@brief Header file of vendor basic type definition.\n

	@file vendor_type.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _VENDOR_TYPE_H_
#define _VENDOR_TYPE_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define HD_VENDOR_META_HEADER_SIZE  64  ///< size of signature header in BYTES

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/
/**
	dummy data descriptor.
*/
typedef struct _VENDOR_COMM_DUMMY {
	UINT32 sign;                        ///< signature = MAKEFOURCC('D','U','M','Y')
	HD_METADATA* p_next;                ///< pointer to next meta
	UINT32 header_size;                 ///< signature header size of meta data
	UINT32 buffer_size;                 ///< buffer size of meta data
	UINT32 reserved[12];                ///< reserved for 64-align
} VENDOR_COMM_DUMMY;
STATIC_ASSERT(sizeof(VENDOR_COMM_DUMMY) == HD_VENDOR_META_HEADER_SIZE);

/**
	lut data descriptor.
*/
typedef struct _VENDOR_COMM_LUT {
	UINT32 sign;                        ///< signature = MAKEFOURCC('2','L','U','T')
	HD_METADATA* p_next;                ///< pointer to next meta
	UINT32 header_size;                 ///< signature header size of meta data
	UINT32 buffer_size;                 ///< buffer size of meta data
	UINT32 lut_sel;                     ///< for vpe lut select
	UINT32 frame_id;                    ///< frame for this lut
	UINT32 reserved[10];                ///< reserved for 64-align
} VENDOR_COMM_LUT;
STATIC_ASSERT(sizeof(VENDOR_COMM_LUT) == HD_VENDOR_META_HEADER_SIZE);

/**
	gyro data descriptor.
*/
typedef struct _VENDOR_COMM_GYRO {
	UINT32 sign;                        ///< signature = MAKEFOURCC('G','Y','R','O')
	HD_METADATA* p_next;                ///< pointer to next meta
	UINT32 header_size;                 ///< signature header size of meta data
	UINT32 buffer_size;                 ///< buffer size of meta data
	UINT32 data_count;                  ///< gyro count per frame (ex:16/frame)
	UINT32 frame_id;                    ///< frame for this gyro
	UINT32 axis;                        ///< 3 is xyz, 6 is include accl
	UINT32 reserved[9];                 ///< reserved for 64-align
} VENDOR_COMM_GYRO;
STATIC_ASSERT(sizeof(VENDOR_COMM_GYRO) == HD_VENDOR_META_HEADER_SIZE);

/**
	meta allocation descriptor.

	@note:
	the sign must be equal to the sign of the allocated data descriptor.
	ex. if for lut data descriptor, sign = MAKEFOURCC('2','L','U','T').
*/
typedef struct _VENDOR_META_ALLOC {
	UINT32              sign;             ///< signature for meta alloc
	struct _VENDOR_META_ALLOC *p_next;    ///< pointer to next meta alloc
	UINT32              data_size;        ///< data size for meta alloc
	UINT32              header_size;      ///< signature header size for meta alloc
} VENDOR_META_ALLOC;

/**
	data descriptor for cross-platform.
*/
typedef struct _VENDOR_META_CROSSPLATFORM {
	UINT64 sign_area;                   ///< area of signature
	UINT64 pointer_area;                ///< area of pointer to next meta
	UINT64 meta_area;                   ///< area of common meta information
	UINT32 reserved[10];                ///< reserved for 64-align
} VENDOR_META_CROSSPLATFORM;
STATIC_ASSERT(sizeof(VENDOR_META_CROSSPLATFORM) == HD_VENDOR_META_HEADER_SIZE);

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
#endif

