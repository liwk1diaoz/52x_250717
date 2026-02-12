#ifndef _NVTMBX_H_
#define _NVTMBX_H_


#include <kwrap/type.h>

/**
    NVT user message

    NVT user message.
*/
typedef struct _NVT_USER_MSG {
	UINT        kRsvd;
	UINT        param1;
	UINT        param2;
	UINT        param3;
}  NVT_USER_MSG;

extern void nvt_user_mbx_init(void);
extern ER nvt_user_mbx_uninit(void);
extern void nvt_back_mbx_init(void);
extern ER nvt_back_mbx_uninit(void);
extern ER nvt_snd_msg(ID mbxid, UINT32 eventid, UINT8 *pbuf, UINT32 size);
extern void nvt_rcv_msg(ID mbxid, UINT32 *eventid, UINT8 **pbuf, UINT32 *size);
extern BOOL nvt_chk_msg(ID mbxid);

extern ID NVTBACK_MBX_ID;
extern ID NVTUSER_MBX_ID;


#define NO_USE_MAILBOX_ID   0

#endif //_NVTMBX_H_
