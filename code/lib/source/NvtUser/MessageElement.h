/**
    @file       MessageElement.h
    @ingroup    mIAPPExtUIFrmwk

    @brief      NVT Message Element
                Declare data type and function prototype of message element


    Copyright   Novatek Microelectronics Corp. 2007.  All rights reserved.
*/

#ifndef MESSAGE_ELEMENT_H
#define MESSAGE_ELEMENT_H
#include <kwrap/type.h>
#include "NvtUser/NVTEvent.h"

#define NVTMSG_ELEMENT_NUM           255

/**
@addtogroup mIAPPExtUIFrmwk
@{
@name Message Element Data Structures
@{
*/

/**
    @brief Message Content
*/
typedef struct _NVTMSG {
	NVTEVT event;                             ///< Command
	UINT32 wParam[MAX_MESSAGE_PARAM_NUM];     ///< Parameter
	UINT32 paramNum;                          ///< Parameter Number
} NVTMSG;

//@}
//@}

extern void  NVTMSG_Init(void);
extern void  NVTMSG_UnInit(void);
extern UINT8 NVTMSG_IsInit(void);
extern ER    NVTMSG_GetElement(NVTMSG **p_msg);
extern void  NVTMSG_FreeElement(NVTMSG *p_msg);
extern void  NVTMSG_ClearElement(UINT32 start, UINT32 end);
extern void  NVTMSG_DumpStatus(void);
extern UINT32 NVTMSG_UsedElement(void);

#endif
