/**
    @file       MessageElement.c

    @ingroup    mIAPPExtUIFrmwk

    @brief      NVT Message Element Functions

    Copyright   Novatek Microelectronics Corp. 2007.  All rights reserved.
*/

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UIEvent
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#include "DebugModule.h"
///////////////////////////////////////////////////////////////////////////////

#include <kwrap/debug.h>
#include <kwrap/flag.h>
#include <kwrap/mailbox.h>
#include <kwrap/semaphore.h>
#include <kwrap/task.h>
#include "MessageElement.h"
#include "NvtMbx.h"


static NVTMSG nvtmsg_element[NVTMSG_ELEMENT_NUM];
static NVTMSG *nvtmsg_free_table[NVTMSG_ELEMENT_NUM];
static UINT32 nvtmsg_free_index;
static UINT8 gbIsInit = FALSE;
static ID NVTMSG_SEM_ID = 0;


/**
@addtogroup mIAPPExtUIFrmwk
@{
@name Message Element Functions
@{
*/

/**
    Initialize message elements.

    @note Usually not called by programmers.
*/
void NVTMSG_Init(void)
{
	UINT32 i;

	for (i = 0; i < NVTMSG_ELEMENT_NUM; i++) {
		nvtmsg_free_table[i] = &(nvtmsg_element[i]);
	}
	nvtmsg_free_index = i;
	OS_CONFIG_SEMPHORE(NVTMSG_SEM_ID, 0, 1, 1);

	gbIsInit = TRUE;
}

void NVTMSG_UnInit(void)
{
	if (gbIsInit != TRUE) {
		DBG_ERR("not init\r\n");
	}

	SEM_DESTROY(NVTMSG_SEM_ID);
    gbIsInit  = FALSE;
}
/**
    Check the message elements is initialized or not.

    @return TRUE or FALSE
*/
UINT8 NVTMSG_IsInit(void)
{
	return gbIsInit;
}

void NVTMSG_Lock(void) {
    if (NVTMSG_SEM_ID) {
    	vos_sem_wait(NVTMSG_SEM_ID);
    } else {
        DBG_ERR("not ini\r\n");
    }
}
void NVTMSG_Unlock(void) {
    if (NVTMSG_SEM_ID) {
        vos_sem_sig(NVTMSG_SEM_ID);
    } else {
        DBG_ERR("not ini\r\n");
    }
}


/**
    Get an NVTMSG element

    @param[out] p_msg The pointer of the element
    @return E_OK or E_NOMEM
*/
ER NVTMSG_GetElement(NVTMSG **p_msg)
{
//#NT#2010/05/28#Janice Huang -begin
//#NT#when reference nvtmsg_free_index,need to lock it
	NVTMSG_Lock();
	if (nvtmsg_free_index) {
		nvtmsg_free_index--;
		*p_msg = nvtmsg_free_table[nvtmsg_free_index];
		NVTMSG_Unlock();
		return E_OK;
	} else {
		NVTMSG_Unlock();
		return E_NOMEM;
	}
//#NT#2010/05/28#Janice Huang -end
}

void NVTMSG_ClearElement(UINT32 start, UINT32 end)
{
	UINT32 i;

	for (i = 0; i < NVTMSG_ELEMENT_NUM; i++) {
		NVTMSG_Lock();
		if ((nvtmsg_element[i].event >= start) && (nvtmsg_element[i].event <= end)) {
			DBG_MSG("flush evt %x\r\n", nvtmsg_element[i].event);
			nvtmsg_element[i].event = 0;
		}
		NVTMSG_Unlock();
	}
}

/**
    Free an NVTMSG element

    @param p_msg The pointer of the element
*/
void NVTMSG_FreeElement(NVTMSG *p_msg)
{
	NVTMSG_Lock();
	nvtmsg_free_table[nvtmsg_free_index] = p_msg;
	nvtmsg_free_index++;
	NVTMSG_Unlock();
}

UINT32 NVTMSG_UsedElement(void)
{
    UINT32 used=0;
	NVTMSG_Lock();
    used = NVTMSG_ELEMENT_NUM-nvtmsg_free_index;
	NVTMSG_Unlock();
    return used;
}
void NVTMSG_DumpStatus()
{
	DBG_DUMP("nvtmsg_element %X\r\n", (UINT32)nvtmsg_element);
	DBG_DUMP("nvtmsg_free_table %X\r\n", (UINT32)nvtmsg_free_table);
	DBG_DUMP("free_index %d\r\n", nvtmsg_free_index);

}


//@}
//@}

