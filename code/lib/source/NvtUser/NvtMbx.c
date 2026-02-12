///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          NvtMbx
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "[T]" //*=All, [mark]=CustomClass
//#include "DebugModule.h"
///////////////////////////////////////////////////////////////////////////////

#include <kwrap/debug.h>
#include <kwrap/flag.h>
#include <kwrap/mailbox.h>
#include <kwrap/semaphore.h>
#include <kwrap/task.h>
#include "NvtMbx.h"

#define T_MSG_ELEMENT_NUM           255
ID NVTUSER_MBX_ID = 0;
ID NVTBACK_MBX_ID = 0;

void nvt_user_mbx_init(void)
{
	VOS_MBX_PARAM user_mbx_param = {T_MSG_ELEMENT_NUM, sizeof(NVT_USER_MSG)};
	DBG_FUNC_BEGIN("\n");

	if (E_OK != vos_mbx_create(&NVTUSER_MBX_ID, &user_mbx_param)) {
	    DBG_ERR("NVTUSER_MBX_ID failed\r\n");
    }
    DBG_IND("NVTUSER_MBX_ID %x\r\n",NVTUSER_MBX_ID);
}
ER nvt_user_mbx_uninit(void)
{
	DBG_FUNC_BEGIN("\n");
    vos_mbx_destroy(NVTUSER_MBX_ID);
    return  E_OK;
}

void nvt_back_mbx_init(void)
{
	VOS_MBX_PARAM back_mbx_param = {T_MSG_ELEMENT_NUM, sizeof(NVT_USER_MSG)};
	DBG_FUNC_BEGIN("\n");

	if (E_OK != vos_mbx_create(&NVTBACK_MBX_ID, &back_mbx_param)) {
	    DBG_ERR("NVTBACK_MBX_ID failed\r\n");
    }
    DBG_IND("NVTBACK_MBX_ID %x\r\n",NVTBACK_MBX_ID);

}
ER nvt_back_mbx_uninit(void)
{
	DBG_FUNC_BEGIN("\n");

    vos_mbx_destroy(NVTBACK_MBX_ID);
    return  E_OK;

}
ER nvt_get_msg_element(NVT_USER_MSG **p_msg)
{
    return 0;
}

void nvt_free_msg_element(NVT_USER_MSG *p_msg)
{
}

ER nvt_snd_msg(ID mbxid, UINT32 eventid, UINT8 *pbuf, UINT32 size)
{
   	NVT_USER_MSG msg = {0};
	DBG_FUNC_BEGIN("\n");

	msg.param1 = eventid;
	if (pbuf && size) {
		msg.param2 = (UINT32)pbuf;
		msg.param3 = size;
	}
    return vos_mbx_snd(mbxid, (void *)&msg, sizeof(NVT_USER_MSG));
}

void nvt_rcv_msg(ID mbxid, UINT32 *eventid, UINT8 **pbuf, UINT32 *size)
{
   	NVT_USER_MSG msg = {0};
    ER ret = 0;
	DBG_FUNC_BEGIN("\n");

    ret = vos_mbx_rcv(mbxid,(void *)&msg,sizeof(NVT_USER_MSG));
	if (ret < 0) {
		DBG_ERR("nvt_rcv_msg (%d)", ret);
	}
	//#NT#2009/04/24#Lincy -end

	*eventid = msg.param1;
	*pbuf   = (UINT8 *)msg.param2;
	*size    = msg.param3;
}

BOOL nvt_chk_msg(ID mbxid)
{
	return (BOOL)vos_mbx_is_empty(mbxid);
}
