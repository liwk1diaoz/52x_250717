/**
    FwSrv, Internal function implementation

    @file       FwSrvInt.c
    @ingroup    mFWSRV

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "FwSrvInt.h"

/**
    Internal Control Object
*/
static FWSRV_CTRL m_FwSrvCtrl = {0};


/**
    Get Internal Control Object Pointer

    @return internal control object pointer
*/
FWSRV_CTRL* xFwSrv_GetCtrl(void)
{
    return &m_FwSrvCtrl;
}

/**
    Error code handling.

    1. show error code on uart. \n
    2. save last error code for debug. \n
    3. callback out if user set callback function

    @return error code.
*/
FWSRV_ER xFwSrv_Err(FWSRV_ER er)
{
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();

    if(er==FWSRV_ER_OK)
    {
        return er;
    }

    pCtrl->DbgInfo.LastEr = er;
    DBG_ERR("%d\r\n",er);

    if(pCtrl->Init.fpErrorCb!=NULL)
    {
        pCtrl->State = FWSRV_STATE_ER_CB_BEGIN;
        pCtrl->Init.fpErrorCb(er);
        pCtrl->State = FWSRV_STATE_ER_CB_END;
    }
    return er;
}

/**
    Show warning code to uart and save last warning code for debug.

    @return warning code.
*/
FWSRV_WR xFwSrv_Wrn(FWSRV_WR wr)
{
    if(wr==FWSRV_WR_OK)
    {
        return wr;
    }

    xFwSrv_GetCtrl()->DbgInfo.LastWr = wr;
    DBG_WRN("%d\r\n",wr);

    return wr;
}

/**
    Set task state machine.

    @param[in] State current state
    @return error code.
*/
FWSRV_ER xFwSrv_SetState(FWSRV_STATE State)
{
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();

    pCtrl->State = State;

    if(pCtrl->Init.fpStateCb!=NULL)
    {
        pCtrl->State = FWSRV_STATE_STATUS_CB_BEGIN;
        pCtrl->Init.fpStateCb(State);
        pCtrl->State = FWSRV_STATE_STATUS_CB_END;
    }

    return FWSRV_ER_OK;
}

/**
    Get task state machine.

    @return current state
*/
FWSRV_STATE xFwSrv_GetState(void)
{
    return xFwSrv_GetCtrl()->State;
}

/**
    Entercritical section for API

    @return error code.
*/
FWSRV_ER xFwSrv_Lock(void)
{
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();
    FWSRV_INIT* pInit = &pCtrl->Init;

    if(pCtrl->Cond.uiInitKey!=CFG_FWSRV_INIT_KEY)
    {
        return xFwSrv_Err(FWSRV_ER_NOT_INIT);
    }
    if(vos_sem_wait(pInit->SemID)!=E_OK)
    {
        return xFwSrv_Err(FWSRV_ER_LOCK);
    }

    return FWSRV_ER_OK;
}

/**
    Leave critical section for API

    @return error code.
*/
FWSRV_ER xFwSrv_Unlock(void)
{
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();
    FWSRV_INIT* pInit = &pCtrl->Init;

    if(pCtrl->Cond.uiInitKey!=CFG_FWSRV_INIT_KEY)
    {
        return xFwSrv_Err(FWSRV_ER_NOT_INIT);
    }
    vos_sem_sig(pInit->SemID);

    return FWSRV_ER_OK;
}

/**
    Indicate task is idle

    @return
        - @b TRUE: task is idle.
        - @b FALSE: task is not idle.
*/
BOOL xFwSrv_ChkIdle(void)
{
    if(kchk_flg(xFwSrv_GetCtrl()->Init.FlagID,FLGFWSRV_IDLE)!=0ull)
    {
        return TRUE;
    }
    return FALSE;
}

/**
    Wait task idle and lock task to get access right

    @return error code.
*/
FWSRV_ER xFwSrv_WaitIdle(void)
{
    FLGPTN FlgPtn;

    if(wai_flg(&FlgPtn,xFwSrv_GetCtrl()->Init.FlagID,FLGFWSRV_IDLE,TWF_CLR)!=E_OK)
    {
        return xFwSrv_Err(FWSRV_ER_WAIT_IDLE);
    }
    return FWSRV_ER_OK;
}

/**
    Set task idle and unlock task to release access right

    @return error code.
*/
FWSRV_ER xFwSrv_SetIdle(void)
{
    if(set_flg(xFwSrv_GetCtrl()->Init.FlagID,FLGFWSRV_IDLE)!=E_OK)
    {
        return xFwSrv_Err(FWSRV_ER_SET_IDLE);
    }
    return FWSRV_ER_OK;
}

/**
    Task open extension action

    if there's something to do when task start (such as initial some variable).
    Please implemnt here.

    @return error code.
*/
FWSRV_ER xFwSrv_RunOpen(void)
{
    xFwSrv_SetState(FWSRV_STATE_OPEN_BEGIN);

	//move to single command
    //pStrgFw->Lock();
    //pStrgFw->Open();
    //pStrgFw->Unlock();

    xFwSrv_SetState(FWSRV_STATE_OPEN_END);
    return FWSRV_ER_OK;
}

/**
    Task close extension action

    if there's something to do when task close. Please implemnt here.

    @return error code.
*/
FWSRV_ER xFwSrv_RunClose(void)
{
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();

    xFwSrv_SetState(FWSRV_STATE_CLOSE_BEGIN);

    pCtrl->Cond.bStopped = TRUE;

    xFwSrv_SetState(FWSRV_STATE_CLOSE_END);
    return FWSRV_ER_OK;
}

/**
    Task suspend extension action

    if there's something to do when task suspend. Please implemnt here.

    @return error code.
*/
FWSRV_ER xFwSrv_RunSuspend(void)
{
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();

    xFwSrv_SetState(FWSRV_STATE_SUSPEND_BEGIN);

    //Do something extension here
    //.................

    pCtrl->Cond.bSuspended = TRUE;
    xFwSrv_SetState(FWSRV_STATE_SUSPEND_END);
    return FWSRV_ER_OK;
}

/**
    Task resume extension action

    if there's something to do when task resume. Please implemnt here.

    @return error code.
*/
FWSRV_ER xFwSrv_RunResume(void)
{
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();

    xFwSrv_SetState(FWSRV_STATE_RESUME_BEGIN);

    //Do something here
    //.................


    pCtrl->Cond.bSuspended = FALSE;
    xFwSrv_SetState(FWSRV_STATE_RESUME_END);
    return FWSRV_ER_OK;
}

/**
    Task command manager

    any command will be assigned an action function via this manager. don't
    modify here if you want to do some extension job.

    @return error code.
*/
FWSRV_ER xFwSrv_RunCmd(const FWSRV_CMD* pCmd)
{
    FWSRV_CTRL*  pCtrl = xFwSrv_GetCtrl();
    FWSRV_FP_CMD fpCmd = pCtrl->CmdCtrl.pFuncTbl[pCmd->Idx].fpCmd;

    pCtrl->CmdCtrl.erCmd = FWSRV_ER_OK;
    pCtrl->DbgInfo.LastCmdIdx= pCmd->Idx;

    if(pCmd->Idx >= FWSRV_CMD_IDX_MAX_NUM)
    {
        pCtrl->CmdCtrl.erCmd = FWSRV_ER_INVALID_CMD_IDX;
        goto Quit;
    }

    if(fpCmd==NULL)
    {
        pCtrl->CmdCtrl.erCmd = FWSRV_ER_CMD_MAP_NULL;
        goto Quit;
    }

    xFwSrv_SetState(FWSRV_STATE_CMD_BEGIN);
    pCtrl->CmdCtrl.erCmd = fpCmd(pCmd);
    xFwSrv_SetState(FWSRV_STATE_CMD_END);

Quit:
    if(pCmd->Prop.fpFinishCb!=NULL)
    {
        FWSRV_FINISH FinishInfo = {0};
        FinishInfo.Idx = pCmd->Idx;
        FinishInfo.er = pCtrl->CmdCtrl.erCmd;
        FinishInfo.pUserData = pCmd->Prop.pUserData;
        xFwSrv_SetState(FWSRV_STATE_CMD_CB_BEGIN);
        pCmd->Prop.fpFinishCb(&FinishInfo);
        xFwSrv_SetState(FWSRV_STATE_CMD_CB_END);
    }

    if(pCtrl->CmdCtrl.erCmd==FWSRV_ER_OK)
    {
        return pCtrl->CmdCtrl.erCmd;
    }

    return pCtrl->CmdCtrl.erCmd;
}
