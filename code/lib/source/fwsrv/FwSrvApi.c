/**
    FwSrv, major APIs implementation.

    @file       FwSrvApi.c
    @ingroup    mFWSRV

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "FwSrvInt.h"
#include "FwSrvID.h"

FWSRV_ER FwSrv_Init(const FWSRV_INIT* pInit)
{
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();

    if(pInit->uiApiVer != FWSRV_API_VERSION)
    {
        return xFwSrv_Err(FWSRV_ER_API_VERSION);
    }

    if(pCtrl->Cond.uiInitKey==CFG_FWSRV_INIT_KEY)
    {
        xFwSrv_Wrn(FWSRV_WR_INIT_TWICE);
        return FWSRV_ER_OK;
    }

    FwSrv_InstallID();

    memset(pCtrl,0,sizeof(FWSRV_CTRL));
    pCtrl->Init = *pInit;

    if(pCtrl->Init.TaskID==0 || pCtrl->Init.SemID==0 || pCtrl->Init.FlagID)
    {
        //pCtrl->Init.TaskID = FWSRV_TSK_ID;
        pCtrl->Init.SemID = FWSRV_SEM_ID;
        pCtrl->Init.FlagID = FWSRV_FLG_ID;
    }
#if 0
    if(pCtrl->Init.TaskID==0)
    {
        DBG_ERR("ID not installed\r\n");
        return FWSRV_ER_SYS;
    }
#endif
    //check FwSrv and Partial Lib version is matched.
    if(pCtrl->Init.PlInit.uiApiVer!=PARTLOAD_API_VERSION)
    {
        return xFwSrv_Err(FWSRV_ER_PL_VERSION);
    }

    //Partial Load Init
    if(PartLoad_Init(&pCtrl->Init.PlInit)!=E_OK)
    {
        return xFwSrv_Err(FWSRV_ER_PL_INIT);
    }

    //xFwSrv_InstallCmd();
    pCtrl->CmdCtrl.pFuncTbl = xFwSrv_GetCallTbl(&pCtrl->CmdCtrl.uiNumFunc);
    pCtrl->Cond.uiInitKey = CFG_FWSRV_INIT_KEY;

    return FWSRV_ER_OK;
}

FWSRV_ER FwSrv_Open(VOID)
{
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();
    FWSRV_INIT* pInit = &pCtrl->Init;

    xFwSrv_Lock();
    if(xFwSrv_GetState()!=FWSRV_STATE_UNKNOWN)
    {
        xFwSrv_Unlock();
        return xFwSrv_Err(FWSRV_ER_STATE);
    }

    clr_flg(pInit->FlagID, (FLGPTN)-1);
    pCtrl->Cond.bOpened = FALSE;
    pCtrl->Cond.bSuspended = FALSE;
    pCtrl->Cond.bStopped = FALSE;
    pInit->TaskID = vos_task_create(FwSrvTsk, NULL, "FwSrvTsk", PRI_FWSRV, STKSIZE_FWSRV);
	vos_task_resume(pInit->TaskID);
    pCtrl->Cond.bOpened = TRUE;
    xFwSrv_Unlock();

    return FWSRV_ER_OK;
}

FWSRV_ER FwSrv_Close(VOID)
{
    VK_TASK_HANDLE tid;
    FLGPTN FlgPtn;
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();
    FWSRV_INIT* pInit = &pCtrl->Init;

    tid = vos_task_get_handle();
    if(tid==pInit->TaskID)
    {
        return xFwSrv_Err(FWSRV_ER_INVALID_CALL);
    }
    xFwSrv_Lock();
    if(pCtrl->Cond.bOpened==FALSE)
    {
        xFwSrv_Unlock();
        xFwSrv_Wrn(FWSRV_WR_ALREADY_CLOSED);
        return FWSRV_ER_OK;
    }
    xFwSrv_WaitIdle();
    set_flg(pInit->FlagID,FLGFWSRV_CLOSE);
    wai_flg(&FlgPtn,pInit->FlagID,FLGFWSRV_STOPPED,TWF_CLR);
    xFwSrv_SetState(FWSRV_STATE_UNKNOWN);
    pCtrl->Cond.bOpened = FALSE;
    vos_task_destroy(pInit->TaskID);
    xFwSrv_Unlock();
    return FWSRV_ER_OK;
}

FWSRV_ER FwSrv_Suspend(VOID)
{
    VK_TASK_HANDLE tid;
    FLGPTN FlgPtn;
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();
    FWSRV_INIT* pInit = &pCtrl->Init;

    tid = vos_task_get_handle();
    if(tid==pInit->TaskID)
    {
        return xFwSrv_RunSuspend();
    }
    xFwSrv_Lock();
    if(pCtrl->Cond.bOpened==FALSE)
    {
        xFwSrv_Unlock();
        return xFwSrv_Err(FWSRV_ER_STATE);
    }
    xFwSrv_WaitIdle();
    set_flg(pInit->FlagID,FLGFWSRV_SUSPEND);
    wai_flg(&FlgPtn,pInit->FlagID,FLGFWSRV_SUSPENDED,TWF_CLR);
    xFwSrv_Unlock();
    return FWSRV_ER_OK;
}

FWSRV_ER FwSrv_Resume(VOID)
{
    VK_TASK_HANDLE tid;
    FLGPTN FlgPtn;
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();
    FWSRV_INIT* pInit = &pCtrl->Init;

    tid = vos_task_get_handle();
    if(tid==pInit->TaskID)
    {
        return xFwSrv_RunResume();
    }
    xFwSrv_Lock();
    if(pCtrl->Cond.bOpened==FALSE)
    {
        xFwSrv_Unlock();
        return xFwSrv_Err(FWSRV_ER_STATE);
    }
    xFwSrv_WaitIdle();
    set_flg(pInit->FlagID,FLGFWSRV_RESUME);
    wai_flg(&FlgPtn,pInit->FlagID,FLGFWSRV_RESUMED,TWF_CLR);
    xFwSrv_Unlock();
    return FWSRV_ER_OK;
}

FWSRV_ER FwSrv_Cmd(const FWSRV_CMD* pCmd)
{
    VK_TASK_HANDLE tid;
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();
    FWSRV_INIT* pInit = &pCtrl->Init;
    const FWSRV_CMD_DESC* pDesc = &pCtrl->CmdCtrl.pFuncTbl[pCmd->Idx];
    FWSRV_ER er = FWSRV_ER_OK;

    if(pCmd->Idx!=pDesc->Idx)
    {
        return xFwSrv_Err(FWSRV_ER_CMD_NOT_MATCH);
    }

    if(pCmd->In.uiNumByte != pDesc->uiNumByteIn)
    {
        return xFwSrv_Err(FWSRV_ER_CMD_IN_DATA);
    }

    if(pCmd->Out.uiNumByte != pDesc->uiNumByteOut)
    {
        return xFwSrv_Err(FWSRV_ER_CMD_OUT_DATA);
    }

    tid = vos_task_get_handle();
    if(tid==pInit->TaskID)
    {
        pCtrl->DbgInfo.LastCallerTask = tid;
        pCtrl->DbgInfo.LastCallerAddr = (UINT32)__builtin_return_address(0)-8;
        return xFwSrv_RunCmd(pCmd);
    }
    xFwSrv_Lock();
    if(pCtrl->Cond.bOpened==FALSE)
    {
        xFwSrv_Unlock();
        return xFwSrv_Err(FWSRV_ER_STATE);
    }

    if(pCmd->Prop.bEnterOnlyIdle)
    {
        FLGPTN Flag = 0;
        set_flg(pInit->FlagID,FLGFWSRV_POLLING);
        wai_flg(&Flag,pInit->FlagID,FLGFWSRV_POLLING|FLGFWSRV_IDLE,TWF_ORW|TWF_CLR);

        if((Flag&FLGFWSRV_IDLE)==0)
        {
            xFwSrv_Unlock();
            return xFwSrv_Err(FWSRV_ER_NOT_IN_IDLE);
        }
    }
    else
    {
        xFwSrv_WaitIdle();
    }

    pCtrl->CmdCtrl.Cmd = *pCmd;

    if(pCmd->Prop.bExitCmdFinish==FALSE)
    {
        if((pCmd->Out.pData!=NULL) && (pCmd->Prop.fpFinishCb==NULL))
        {
            xFwSrv_SetIdle();
            xFwSrv_Unlock();
            return xFwSrv_Err(FWSRV_ER_OUT_DATA_VOLATILE);
        }

        if(sizeof(FWSRV_CMD_MAXDATA)>=pCmd->In.uiNumByte)
        {
            memcpy(&pCtrl->CmdCtrl.MaxInBuf,pCmd->In.pData,(size_t)pCmd->In.uiNumByte);
            pCtrl->CmdCtrl.Cmd.In.pData = &pCtrl->CmdCtrl.MaxInBuf;
        }
        else
        {
            xFwSrv_SetIdle();
            xFwSrv_Unlock();
            return xFwSrv_Err(FWSRV_ER_CMD_MAXDATA);
        }
    }

    pCtrl->DbgInfo.LastCallerTask = tid;
    pCtrl->DbgInfo.LastCallerAddr = (UINT32)__builtin_return_address(0)-8;

    set_flg(pInit->FlagID,FLGFWSRV_CMD);

    if(pCmd->Prop.bExitCmdFinish)
    {
        xFwSrv_WaitIdle();
        er = pCtrl->CmdCtrl.erCmd;
        xFwSrv_SetIdle();
    }
    xFwSrv_Unlock();
    return er;
}

FWSRV_ER FwSrv_AsyncSuspend(VOID)
{
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();

    xFwSrv_Lock();
    if(pCtrl->Cond.bOpened==FALSE)
    {
        xFwSrv_Unlock();
        return xFwSrv_Err(FWSRV_ER_STATE);
    }

    set_flg(xFwSrv_GetCtrl()->Init.FlagID,FLGFWSRV_SUSPEND);
    xFwSrv_Unlock();
    return FWSRV_ER_OK;
}

FWSRV_ER FwSrv_AsyncClose(VOID)
{
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();

    xFwSrv_Lock();
    if(pCtrl->Cond.bOpened==FALSE)
    {
        xFwSrv_Unlock();
        return xFwSrv_Err(FWSRV_ER_STATE);
    }

    set_flg(xFwSrv_GetCtrl()->Init.FlagID,FLGFWSRV_CLOSE);
    xFwSrv_Unlock();
    return FWSRV_ER_OK;
}

FWSRV_ER FwSrv_DumpInfo(VOID)
{
    UINT32 ui;
    BOOL   bOK;
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();
    FWSRV_DBGINFO*  pInfo = &pCtrl->DbgInfo;

    //dump all debug inforation to uart
    DBG_DUMP("FwSrv:\r\n");
    DBG_DUMP("Api Ver %08X\r\n",FWSRV_API_VERSION);
    DBG_DUMP("Last Error: %d\r\n",pInfo->LastEr);
    DBG_DUMP("Last Warning: %d\r\n",pInfo->LastWr);
    DBG_DUMP("Last Cmd: %d\r\n",pInfo->LastCmdIdx);
    DBG_DUMP("Last Caller Task ID = %d\r\n",(int)pInfo->LastCallerTask);
    DBG_DUMP("Last Caller Address = 0x%08X\r\n",pInfo->LastCallerAddr);
    DBG_DUMP("Current State: %d\r\n",pCtrl->State);

    bOK = TRUE;
    for(ui=0;ui<pCtrl->CmdCtrl.uiNumFunc;ui++)
    {
        if(pCtrl->CmdCtrl.pFuncTbl[ui].Idx != ui)
        {
            bOK = FALSE;
            break;
        }
    }
    DBG_DUMP("Check FuncTable...%s\r\n",(bOK)?"PASS":"FAIL");

    bOK = TRUE;
    for(ui=0;ui<pCtrl->CmdCtrl.uiNumFunc;ui++)
    {
        if(pCtrl->CmdCtrl.pFuncTbl[ui].uiNumByteIn>sizeof(FWSRV_CMD_MAXDATA))
        {
            bOK = FALSE;
            break;
        }
    }
    DBG_DUMP("Check FWSRV_CMD_MAXDATA...%s\r\n",(bOK)?"PASS":"FAIL");
    return FWSRV_ER_OK;
}
