#include "FsLinuxLock.h"
#include "FsLinuxID.h"

//for Open/Close API use only
void FsLinux_Lock(FSLINUX_CTRL_OBJ *pCtrlObj)
{
	vos_sem_wait_interruptible(pCtrlObj->SemID);
}

//for Open/Close API use only
void FsLinux_Unlock(FSLINUX_CTRL_OBJ *pCtrlObj)
{
	vos_sem_sig(pCtrlObj->SemID);
}

