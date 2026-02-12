#ifndef _FSLINUX_CMD_H_
#define _FSLINUX_CMD_H_

#include "fslinux.h"

int fslinux_cmd_Init(void);

int fslinux_cmd_DoCmd(const FSLINUX_IPCMSG *pMsgRcv);

#endif //_FSLINUX_CMD_H_