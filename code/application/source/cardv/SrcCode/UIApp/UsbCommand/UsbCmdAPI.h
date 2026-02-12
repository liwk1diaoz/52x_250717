/**
    Header file of USB Command API

    Exported API of USB Command functions.

    @file       UsbCmdAPI.h
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#ifndef _USBCMDAPI_H
#define _USBCMDAPI_H

#include <kwrap/type.h>

/**
    @addtogroup
*/
//@{

typedef struct _USBCMD_MEMRANGE {
	UINT32  addr_va;
	UINT32  addr_pa;
	UINT32  size;
} USBCMD_MEMRANGE, *PUSBCMD_MEMRANGE;

#define MIN_USB_CMD_BUF_SIZE    (64*1024)


//upload/download command
#define USBCMD_UPLOAD_FILE_PATH       5001
#define USBCMD_DOWNLOAD_FILE_PATH     5002


/**
     Install USB Command task, flag and semaphore id.

     Install USB Command task, flag and semaphore id.

     @return void
*/
extern void UsbCmd_InstallID(void) _SECTION(".kercfg_text");



extern ER UsbCmd_Open(PUSBCMD_MEMRANGE pWorkBuf);
extern ER UsbCmd_Close(void);
extern int UsbCmd_Send(char *addr, int *size);



//@}


#endif //_USBCMDAPI_H
