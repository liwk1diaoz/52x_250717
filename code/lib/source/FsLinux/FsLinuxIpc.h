#ifndef _FSLINUXMSG_H_
#define _FSLINUXMSG_H_
#if FSLINUX_USE_IPC
ER FsLinux_Ipc_Init(void);
ER FsLinux_Ipc_Uninit(void);

ER FsLinux_Ipc_WaitMsg(FSLINUX_IPCMSG *pRcvMsg);
ER FsLinux_Ipc_PostMsg(FSLINUX_IPCMSG *pPostMsg);//this API will NOT wait the ack
#endif
ER FsLinux_Ipc_SendMsgWaitAck(const FSLINUX_IPCMSG *pSendMsg);//this API will wait the ack
ER FsLinux_Ipc_ReqInfoWaitAck(const FSLINUX_IPCMSG *pSendMsg);//only used by FsLinux_GetDiskInfo
#if FSLINUX_USE_IPC
ER FsLinux_Ipc_SysCallReq(CHAR *szCmd);
#endif
#endif //_FSLINUXMSG_H_

