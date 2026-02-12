/**
    @file       UIFrameworkCommand.c

    @ingroup    mIAPPExtUIFrmwk

    @brief      NVT UI framework event transmission
                Implement functions of event sending, waiting and flushing.

    Copyright   Novatek Microelectronics Corp. 2007.  All rights reserved.
*/

#include "string.h"
#include "NvtUserInternal.h"
#include <stdarg.h>
#include "NvtMbx.h"
#include <kwrap/semaphore.h>
#include "kwrap/perf.h"


//#NT#2010/03/23#Ben Wang -begin
//#NT#Add debug function for event log
#define MSG_DEBUG_NUM   128
static NVTEVT nvtmsg_post_table[MSG_DEBUG_NUM] = {0};
static UINT32 nvtmsg_post_index = 0;

static NVTEVT nvtmsg_dispatch_table[MSG_DEBUG_NUM] = {0};
static UINT32 nvtmsg_dispatch_index = 0;
//#NT#2010/03/23#Ben Wang -end
static UINT32 PostFps =0;
static UINT32 bCalPostFps=0;
static UINT32 PostUnit=40; //per 40 event,calculate fps
static ID POSTEVT_SEM_ID = 0;

/**
@addtogroup mIAPPExtUIFrmwk
@{
*/

/**
    Initialize Message for UI framework and UI background

    @param UIFrameworkMbxID Mailbox ID for UI framework message queue

    @note The mailbox must be configured well in syscfg.c and SysCfg.h already.
    @note Message is usually initialized in system init stage or
          the beginning of key scan. Message must be initialized
          before UI framework and UI background started.
*/
void Ux_InitMessage(void)
{
	DBG_FUNC_BEGIN("\r\n");
	if (!NVTMSG_IsInit()) {
		NVTMSG_Init();
    	OS_CONFIG_SEMPHORE(POSTEVT_SEM_ID, 0, 1, 1);
	}
    DBG_FUNC_END("\r\n");
}
void Ux_UnInitMessage(void)
{
	NVTMSG_UnInit();
	SEM_DESTROY(POSTEVT_SEM_ID);
}

/**
    Check the UI framework message is initialized or not.

    @return TRUE or FALSE
*/
BOOL Ux_IsInitMessage(void)
{
	return (NVTMSG_IsInit() &&
			(NVTUSER_MBX_ID != NO_USE_MAILBOX_ID));
}
void PostEvt_Lock(void) {
    if (POSTEVT_SEM_ID) {
    	vos_sem_wait(POSTEVT_SEM_ID);
    } else {
        DBG_ERR("not ini\r\n");
    }
}
void PostEvt_UnLock(void) {
    if (POSTEVT_SEM_ID) {
        vos_sem_sig(POSTEVT_SEM_ID);
    } else {
        DBG_ERR("not ini\r\n");
    }
}

/**
    Send event to UI framework

    @param evt Command
    @param paramNum How many parameters with the event. Acceptable value: 0 ~ 3
    @param ... The parameters

    @code
    Ux_PostCommand(NVTEVT_A_COMMAND, 0);
    @endcode

    @code
    Ux_PostCommand(NVTEVT_KEY_PRESS, 1, NVTEVT_UP_KEY);
    @endcode

    @code
    Ux_PostCommand(NVTEVT_A_COMMAND, 2, param1, param2);
    @endcode

    @code
    Ux_PostCommand(NVTEVT_A_COMMAND, 3, value1, value2, value3);
    @endcode
*/
void Ux_PostEvent(NVTEVT evt, UINT32 paramNum, ...)
{
	NVTMSG *sndMsg;
	ER ret;
	UINT8 i;
	va_list ap;

	DBG_FUNC_BEGIN("\r\n");
	DBG_IND("Ux_PostEvent: %08x!\r\n", evt);
	if (NVTUSER_MBX_ID == NO_USE_MAILBOX_ID) {
		DBG_ERR("Ux_PostEvent: Mailbox ID incorrect!\n\r");
		return;
	}

	//for calculate event fps
	if(bCalPostFps)
	{
		static UINT32 lasttime=0;
		static UINT32 evtCnt =0;
		UINT32 curtime=0;
		evtCnt++;
		//curtime=Perf_GetCurrent();
        vos_perf_mark(&curtime);

		if(evtCnt%PostUnit==0){
			PostFps = PostUnit*1000000/(curtime-lasttime);
			DBG_DUMP("##PostEvent fps %d \r\n",PostFps);
		}
		if(evtCnt%PostUnit==1){
			//lasttime=Perf_GetCurrent();
            vos_perf_mark(&lasttime);
		}

	}

	ret = NVTMSG_GetElement(&sndMsg);
	if (ret == E_OK) {
		//#NT#2010/03/23#Ben Wang -begin
		//#NT#Add debug function for event log
		PostEvt_Lock();
		nvtmsg_post_table[nvtmsg_post_index] = evt;
		nvtmsg_post_index ++;
		nvtmsg_post_index = nvtmsg_post_index % MSG_DEBUG_NUM;
		PostEvt_UnLock();
		//#NT#2010/03/23#Ben Wang -end
		sndMsg->event = evt;

		if (paramNum > MAX_MESSAGE_PARAM_NUM) {
			DBG_ERR("SendCommand: Parameter number exceeds %d. Only take %d.\n\r", MAX_MESSAGE_PARAM_NUM, MAX_MESSAGE_PARAM_NUM);
			return ;
		}

		sndMsg->paramNum = paramNum;

		if (paramNum) {
			va_start(ap, paramNum);
			for (i = 0; i < paramNum; i++) {
				sndMsg->wParam[i] = va_arg(ap, UINT32);
			}
			va_end(ap);
		}
		ret = nvt_snd_msg(NVTUSER_MBX_ID, 0, (UINT8 *)sndMsg, sizeof(NVTMSG));
		if (ret != E_OK) {
			DBG_ERR("Ux_PostEvent: Error - 2\r\n");
		}
	} else {
		DBG_ERR("Ux_PostEvent: Error - 1\n\r");
		if (gpfNvtuserCB) {
			gpfNvtuserCB(UXUSER_CB_QFULL, 0, 0, 0);
		}
	}
}


/**
    Wait event in UI framework

    @param[out] evt Command
    @param[out] paramNum How many parameters with the event
    @param[out] The parameter array

    @note This function is called in UIFrameworkTsk. Programmers do not call this function.
*/
void Ux_WaitEvent(NVTEVT *evt, UINT32 *paramNum, UINT32 *paramArray)
{
	UINT32 event_id;
	NVTMSG *rcvMsg;
	UINT32 size;

	DBG_FUNC_BEGIN("\r\n");
	if (NVTUSER_MBX_ID == NO_USE_MAILBOX_ID) {
		DBG_ERR("UIFrameworkWaitCommand: Mailbox ID incorrect!\n\r");
		return;
	}

	nvt_rcv_msg(NVTUSER_MBX_ID, &event_id, (UINT8 **)(&rcvMsg), &size);
	if (event_id == 0 && size == sizeof(NVTMSG)) {
		*evt = rcvMsg->event;
		*paramNum = rcvMsg->paramNum;
		memset(paramArray, 0, sizeof(UINT32) * MAX_MESSAGE_PARAM_NUM);
		memcpy(paramArray, rcvMsg->wParam, sizeof(UINT32) * rcvMsg->paramNum);
		NVTMSG_FreeElement(rcvMsg);
		//#NT#2010/03/23#Ben Wang -begin
		//#NT#Add debug function for event log
		PostEvt_Lock();
		nvtmsg_dispatch_table[nvtmsg_dispatch_index] = *evt;
		nvtmsg_dispatch_index ++;
		nvtmsg_dispatch_index = nvtmsg_dispatch_index % MSG_DEBUG_NUM;
		PostEvt_UnLock();
		//#NT#2010/03/23#Ben Wang -end
	} else {
		DBG_ERR("UIFrameworkWaitCommand Error event_id = %d, size = %d\n\r", event_id, size);
	}
}


/**
    Flush the message queue of UI framework
*/
void Ux_FlushEvent(void)
{
	UINT32 event_id;
	NVTMSG *rcvMsg;
	UINT32 size;

	DBG_FUNC_BEGIN("\r\n");
	if (NVTUSER_MBX_ID == NO_USE_MAILBOX_ID) {
		DBG_ERR("UIFrameworkFlushCommand: Mailbox ID incorrect!\n\r");
		return;
	}
	while (nvt_chk_msg(NVTUSER_MBX_ID)) {
		nvt_rcv_msg(NVTUSER_MBX_ID, &event_id, (UINT8 **)(&rcvMsg), &size);
		if (event_id == 0 && size == sizeof(NVTMSG)) {
			NVTMSG_FreeElement(rcvMsg);
		}
		DBG_IND("Flush useless message evt = 0x%x, evt = 0x%x\n\r", rcvMsg->event, rcvMsg->wParam[0]);
	}
}

/**
    Flush the specific event message queue of UI framework
*/
void Ux_FlushEventByRange(UINT32 start, UINT32 end)
{
	DBG_FUNC_BEGIN("\r\n");
	DBG_IND("^RUx_FlushEventByRange %08x~%08x!\r\n", start, end);
	if (NVTUSER_MBX_ID == NO_USE_MAILBOX_ID) {
		DBG_ERR("UIFrameworkFlushCommand: Mailbox ID incorrect!\n\r");
		return;
	}
	NVTMSG_ClearElement(start, end);
}

//#NT#2010/03/23#Ben Wang -begin
//#NT#Add debug function for event log
void Ux_DumpEventHistory(void)
{
	UINT32 i;
	DBG_DUMP("The followings are the latest %d posted event\r\n", MSG_DEBUG_NUM);
	nvtmsg_post_index += MSG_DEBUG_NUM;
	for (i = 0; i < MSG_DEBUG_NUM; i++) {
		nvtmsg_post_index --;
		DBG_DUMP("%08x\r\n", nvtmsg_post_table[nvtmsg_post_index % MSG_DEBUG_NUM]);
	}

	DBG_DUMP("\r\nThe followings are the latest %d dispatched event\r\n", MSG_DEBUG_NUM);
	nvtmsg_dispatch_index += MSG_DEBUG_NUM;
	for (i = 0; i < MSG_DEBUG_NUM; i++) {
		nvtmsg_dispatch_index --;
		DBG_DUMP("%08x\r\n", nvtmsg_dispatch_table[nvtmsg_dispatch_index % MSG_DEBUG_NUM]);
	}
}
//#NT#2010/03/23#Ben Wang -end

void Ux_CalPostFps(UINT32 bEn,UINT32 unit)
{
	bCalPostFps = bEn;
	if(unit>1)
		PostUnit = unit;

	DBG_DUMP("bCalPostFps %d,PostUnit %d\r\n",bCalPostFps,PostUnit);

}
UINT32 Ux_Get(UINT32 param,UINT32 *value)
{
    UINT32 result = E_OK;

    if(!value) {
        DBG_ERR("no value \r\n");
        return E_PAR;
    }

    switch(param)
    {
        case UX_EVENT_QUEUE_USED:
            *value = NVTMSG_UsedElement();
        break;
        case UX_EVENT_QUEUE_MAX:
            *value = NVTMSG_ELEMENT_NUM;
        break;
        default:
            DBG_ERR("not sup %d\r\n",param);
            result =E_NOSPT;
        break;
    }
    return result;

}

/**
@}
@}
*/

