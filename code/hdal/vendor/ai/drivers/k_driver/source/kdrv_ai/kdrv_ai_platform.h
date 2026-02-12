/**
	@brief Header file of internal definition of kdrv_ai.

	@file kdrv_ai_platform.h

	@ingroup kdrv_ai_platform

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _KDRV_AI_PLATFORM_H_
#define _KDRV_AI_PLATFORM_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
UINT32 kdrv_ai_pa2va_remap(UINT32 pa, UINT32 sz);
UINT32 kdrv_ai_pa2va_remap_wo_sync(UINT32 pa, UINT32 sz);
VOID kdrv_ai_pa2va_unmap(UINT32 va, UINT32 pa);

#endif  /* _KDRV_AI_PLATFORM_H_ */
