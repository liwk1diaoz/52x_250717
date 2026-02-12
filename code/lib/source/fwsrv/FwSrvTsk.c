/**
   FwSrv, Task control implementation

    @file       FwSrvTsk.c
    @ingroup    mFWSRV

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "FwSrvInt.h"

THREAD_DECLARE(FwSrvTsk, args)
{
    FLGPTN FlgPtn;
    FWSRV_CTRL* pCtrl = xFwSrv_GetCtrl();
    ID FlagID = pCtrl->Init.FlagID;
    FLGPTN Mask = FLGFWSRV_CLOSE|FLGFWSRV_SUSPEND|FLGFWSRV_RESUME|FLGFWSRV_CMD;

    THREAD_ENTRY();

    xFwSrv_RunOpen();
    set_flg(FlagID,FLGFWSRV_OPENED);

    while(1)
    {
        xFwSrv_SetIdle();
        PROFILE_TASK_IDLE();
        vos_flag_wait_interruptible(&FlgPtn, FlagID, Mask, TWF_ORW|TWF_CLR);
        PROFILE_TASK_BUSY();

        if((FlgPtn&FLGFWSRV_CLOSE)!=0U)
        {
            xFwSrv_RunClose();
            set_flg(FlagID,FLGFWSRV_STOPPED);
            continue;
        }

        if((FlgPtn&FLGFWSRV_SUSPEND)!=0U)
        {
            if(pCtrl->Cond.bSuspended)
            {
                xFwSrv_Wrn(FWSRV_WR_SUSPEND_TWICE);
            }
            else
            {
                xFwSrv_RunSuspend();
            }
            set_flg(FlagID,FLGFWSRV_SUSPENDED);
            continue;
        }

        if((FlgPtn&FLGFWSRV_RESUME)!=0U)
        {
            if(pCtrl->Cond.bSuspended==FALSE)
            {
                xFwSrv_Wrn(FWSRV_WR_NOT_IN_SUSPEND);
            }
            else
            {
                xFwSrv_RunResume();
            }
            set_flg(FlagID,FLGFWSRV_RESUMED);
        }

        if((pCtrl->Cond.bStopped==TRUE) || (pCtrl->Cond.bSuspended==TRUE))
        {
            xFwSrv_Wrn(FWSRV_WR_CMD_SKIP);
            continue;
        }

        if((FlgPtn&FLGFWSRV_CMD)!=0U)
        {
            xFwSrv_RunCmd(&pCtrl->CmdCtrl.Cmd);
        }
    }
}
