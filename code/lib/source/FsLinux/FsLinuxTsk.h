#ifndef _FSLINUXTSK_H_
#define _FSLINUXTSK_H_

#include "FsLinuxAPI.h"

void FsLinux_Tsk_WaitForAsyncCmd(FSLINUX_CMDID CmdId, FileSys_CB CB, CHAR Drive);

void FsLinux_Tsk_WaitTskReady(void);

void FsLinux_Tsk(void);

#endif //_FSLINUXTSK_H_

