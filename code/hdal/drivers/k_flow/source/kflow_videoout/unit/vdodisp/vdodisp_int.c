/**
    vdodisp, Internal function implementation

    @file       vdodisp_int.c
    @ingroup    mVDODISP

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "vdodisp_int.h"
/**
    Internal Control Object
*/
static VDODISP_CTRL m_vdodisp_ctrl[2] = {0};


/**
    Get Internal Control Object Pointer

    @return internal control object pointer
*/
VDODISP_CTRL *x_vdodisp_get_ctrl(VDODISP_DEVID device_id)
{
	return &m_vdodisp_ctrl[device_id];
}

/**
    Error code handling.

    1. show error code on uart. \n
    2. save last error code for debug. \n
    3. callback out if user set callback function

    @return error code.
*/
VDODISP_ER x_vdodisp_err(VDODISP_DEVID device_id, VDODISP_ER er)
{
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device_id);

	if (er == VDODISP_ER_OK) {
		return er;
	}

	p_ctrl->dbg_info.last_er = er;
	DBG_ERR("[%d]%d\r\n", device_id, er);

	if (p_ctrl->init.fp_error_cb != NULL) {
		p_ctrl->state = VDODISP_STATE_ER_CB_BEGIN;
		p_ctrl->init.fp_error_cb(device_id, er);
		p_ctrl->state = VDODISP_STATE_ER_CB_END;
	}
	return er;
}

/**
    Show warning code to uart and save last warning code for debug.

    @return warning code.
*/
VDODISP_WR x_vdodisp_wrn(VDODISP_DEVID device_id, VDODISP_WR wr)
{
	if (wr == VDODISP_WR_OK) {
		return wr;
	}

	x_vdodisp_get_ctrl(device_id)->dbg_info.last_wr = wr;
	DBG_WRN("[%d]%d\r\n", device_id, wr);

	return wr;
}

/**
    Set task state machine.

    @param[in] State current state
    @return error code.
*/
VDODISP_ER x_vdodisp_set_state(VDODISP_DEVID device_id, VDODISP_STATE state)
{
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device_id);

	p_ctrl->state = state;

	if (p_ctrl->init.fp_state_cb != NULL) {
		p_ctrl->state = VDODISP_STATE_STATUS_CB_BEGIN;
		p_ctrl->init.fp_state_cb(device_id, state);
		p_ctrl->state = VDODISP_STATE_STATUS_CB_END;
	}

	return VDODISP_ER_OK;
}

/**
    Get task state machine.

    @return current state
*/
VDODISP_STATE x_vdodisp_get_state(VDODISP_DEVID device_id)
{
	return x_vdodisp_get_ctrl(device_id)->state;
}

/**
    Entercritical section for API

    @return error code.
*/
VDODISP_ER x_vdodisp_lock(VDODISP_DEVID device_id)
{
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device_id);

	if (p_ctrl->cond.ui_init_key != CFG_VDODISP_INIT_KEY) {
		return x_vdodisp_err(device_id, VDODISP_ER_NOT_INIT);
	}
	#if JANICE_TODO

	if (SEM_WAIT(p_ctrl->cond.sem_id) != E_OK) {
		return x_vdodisp_err(device_id, VDODISP_ER_LOCK);
	}
	#else
    SEM_WAIT(*p_ctrl->cond.sem_id);

	#endif
	return VDODISP_ER_OK;
}

/**
    Leave critical section for API

    @return error code.
*/
VDODISP_ER x_vdodisp_unlock(VDODISP_DEVID device_id)
{

	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device_id);

	if (p_ctrl->cond.ui_init_key != CFG_VDODISP_INIT_KEY) {
		return x_vdodisp_err(device_id, VDODISP_ER_NOT_INIT);
	}
   	#if JANICE_TODO

	#ifdef __KERNEL__
	SEM_SIGNAL(p_ctrl->cond.sem_id);
	#else
	if (SEM_SIGNAL(p_ctrl->cond.sem_id) != E_OK) {
		return x_vdodisp_err(device_id, VDODISP_ER_UNLOCK);
	}
	#endif
    #else
    SEM_SIGNAL(*p_ctrl->cond.sem_id);

	#endif
	return VDODISP_ER_OK;
}

/**
    Indicate task is idle

    @return
        - @b TRUE: task is idle.
        - @b FALSE: task is not idle.
*/
BOOL x_vdodisp_chk_idle(VDODISP_DEVID device_id)
{
	if (kchk_flg(x_vdodisp_get_ctrl(device_id)->cond.flag_id, FLGVDODISP_IDLE) != 0ull) {
		return TRUE;
	}
	return FALSE;
}

/**
    Wait task idle and lock task to get access right

    @return error code.
*/
VDODISP_ER x_vdodisp_wait_idle(VDODISP_DEVID device_id)
{
	FLGPTN flg_ptn;

	if (wai_flg(&flg_ptn, x_vdodisp_get_ctrl(device_id)->cond.flag_id, FLGVDODISP_IDLE, TWF_CLR) != E_OK) {
		return x_vdodisp_err(device_id, VDODISP_ER_WAIT_IDLE);
	}
	return VDODISP_ER_OK;
}

/**
    Set task idle and unlock task to release access right

    @return error code.
*/
VDODISP_ER x_vdodisp_set_idle(VDODISP_DEVID device_id)
{
	if (set_flg(x_vdodisp_get_ctrl(device_id)->cond.flag_id, FLGVDODISP_IDLE) != E_OK) {
		return x_vdodisp_err(device_id, VDODISP_ER_SET_IDLE);
	}
	return VDODISP_ER_OK;
}

#if IDE_DUMP
#include <linux/io.h>
#define READ_REG(addr)          ioread32(ioremap(addr,4))
#include <mach/fmem.h>

UINT32 x_vdodisp_get_addr(void *io_addr)
{
    return (UINT32)READ_REG((unsigned long)io_addr);
}

UINT32 x_vdodisp_check_addr(VDO_FRAME *p_img_next)
{
    UINT32 ide_addr[2]={0};
    UINT32 i;

    ide_addr[0]= (UINT32)x_vdodisp_get_addr((void *)(IDE_ADDR+IDE_V1_BUF_Y_OFS));
    ide_addr[1]= (UINT32)x_vdodisp_get_addr((void *)(IDE_ADDR+IDE_V1_BUF_CB_OFS));

    for(i=0;i<2;i++) {
        UINT32 addr_s = fmem_lookup_pa(p_img_next->addr[i]);
        UINT32 addr_e = fmem_lookup_pa(p_img_next->addr[i]+p_img_next->size.w*p_img_next->size.h/(i+1));

        //DBG_IND("%x  range %x %x\r\n",ide_addr[i],addr_s,addr_e);

        if((ide_addr[i]<addr_s)||(ide_addr[i]>addr_e))
        {
            DBG_ERR("%x out range %x %x while1\r\n",ide_addr[i],addr_s,addr_e);
            return -1;
        }
    }

    return 0;
}
#else
UINT32 x_vdodisp_get_addr(void *io_addr)
{
    return 0;
}
UINT32 x_vdodisp_check_addr(VDO_FRAME *p_img_next)
{
    return 0;
}
#endif
