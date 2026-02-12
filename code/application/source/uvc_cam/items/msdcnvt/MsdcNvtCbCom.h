#ifndef _MSDCNVTCBCOM_H
#define _MSDCNVTCBCOM_H

extern BOOL MsdcNvtCbCom_Open(void);
extern BOOL MsdcNvtCbCom_Close(void);
extern UINT8 *MsdcNvtCbCom_GetTransAddr(void);
extern UINT32 MsdcNvtCbCom_GetTransSize(void);
extern BOOL MsdcNvtCbCom_SetupBaudRate(int BaudRate); //only 115200 and 3000000 allowed
#endif