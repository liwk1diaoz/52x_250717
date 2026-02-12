#ifndef _USBCMDINT_H
#define _USBCMDINT_H

#include "UsbCmdAPI.h"
#include "hd_common.h"
#include <string.h>

#define __MODULE__          UsbCmdTask
#define __DBGLVL__          6 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass

#include <kwrap/debug.h>
#include <kwrap/flag.h>
#include <kwrap/semaphore.h>
#include <kwrap/stdio.h>
#include <kwrap/task.h>
#include <kwrap/util.h>
#include <string.h>
#include "UVAC.h"


// Flag
#define FLGUSBCMD_IDLE          FLGPTN_BIT(0)
#define FLGUSBCMD_IDLE2         FLGPTN_BIT(1)
#define FLGUSBCMD_IDLE3         FLGPTN_BIT(2)
#define FLGUSBCMD_DOWNLOAD      FLGPTN_BIT(3)
#define FLGUSBCMD_ALL           0xFFFFFFFF


#define USB_CMD_PORT    CDC_COM_1ST
#define USB_DATA_PORT   CDC_COM_2ND

#define UNKNOWN_PAYLOAD_LENGTH_BEGIN  0xFFFFFFFF
#define UNKNOWN_PAYLOAD_LENGTH_END    0

#define USBCMD_PAYLOAD_TYPE_KEY    0x5AA569C0  // just a magic number for verification
typedef enum {
	USBCMD_PAYLOAD_RETURN = USBCMD_PAYLOAD_TYPE_KEY,     // the result of command, usually XML
	USBCMD_PAYLOAD_NOTIFICATION,                         // Notification
	USBCMD_PAYLOAD_RAW_DATA                              // file
} USBCMD_PAYLOAD_TYPE;

typedef struct _USBCMD_PAYLOAD_HEADER {
	UINT32 TotalLength;       // payload header + payload data
	USBCMD_PAYLOAD_TYPE Type;
} USBCMD_PAYLOAD_HEADER, *PUSBCMD_PAYLOAD_HEADER;

extern USBCMD_MEMRANGE g_RecvBuf[2];

//extern UINT32 USBCMDTSK_ID;
//extern UINT32 USBCMDTSK2_ID;
//extern UINT32 USBCMDDOWNLOADTSK_ID;
extern ID FLG_ID_USBCMD;
extern ID SEMID_USBCMD_1ST_COM;

extern void UsbCmdHandler(UINT8 *pUsbCmdStr, UINT32 CmdStrLen);
extern void UsbRcvDataHandler(UINT32 Addr, UINT32 Size);
extern void UsbCmdDownloadHandler(void);
extern ER UsbCmdTsk_Open(void);
extern ER UsbCmdTsk_Close(void);
//extern void UsbCmdTsk(void);
//extern void UsbCmdTsk2(void);
//extern void UsbCmdDownloadTsk(void);


#endif //_USBCMDINT_H
