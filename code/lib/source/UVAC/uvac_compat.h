/*
    USB UVAC U2/U3 Compatible Header File


    @file       uvac_compat.h
    @ingroup    mILibUsbUVAC
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/

#ifndef _UVAC_COMPAT_H
#define _UVAC_COMPAT_H

#include "UVAC.h"
#include "usb2dev.h"



#define uvac_dummyapi(x)

#define usb_open                    usb2dev_open
#define usb_close                   usb2dev_close
#define usb_setCallBack             usb2dev_set_callback
#define usb_setManagement           usb2dev_set_management
#define usb_initManagement          usb2dev_init_management
#define usb_setEpConfig             usb2dev_set_ep_config
#define usb_maskEPINT               usb2dev_mask_ep_interrupt
#define usb_unmaskEPINT             usb2dev_unmask_ep_interrupt
#define usb_getControllerState      usb2dev_get_controller_state
#define usb_abortEndpoint           usb2dev_abort_endpoint
#define usb_readEndpoint            usb2dev_read_endpoint
#define usb_writeEndpoint           usb2dev_write_endpoint
#define usb_readEndpointTimeout     usb2dev_read_endpoint_timeout
#define usb_writeEndpointTimeout    usb2dev_write_endpoint_timeout
#define usb_setEPWrite              usb2dev_set_ep_write
#define usb_getEPByteCount          usb2dev_get_ep_bytecount
#define usb_chkEPBusy               usb2dev_check_ep_busy

#define usb_setTX0Byte              usb2dev_set_tx0byte

#define usb_RetSetupData            usb2dev_reture_setup_data
#define usb_setEP0Done              usb2dev_set_ep0_done
#define usb_setEP0Stall()           usb2dev_set_ep_stall(USB_EP0)
#define usb_clrEPFIFO               usb2dev_clear_ep_fifo
#define usb_getSOF                  usb2dev_get_sof_number

#define CONTROL_DATA                usb2dev_control_data

typedef void UVAC_OP_INSTALLID(void);
typedef void UVAC_OP_UNINSTALLID(void);
typedef UINT32 UVAC_OP_OPEN(UVAC_INFO *pClassInfo);
typedef void UVAC_OP_CLOSE(void);
typedef UINT32 UVAC_OP_GETNEEDMEMSIZE(void);
typedef void UVAC_OP_SETCONFIG(UVAC_CONFIG_ID ConfigID, UINT32 Value);
typedef void UVAC_OP_SETEACHSTRMINFO(PUVAC_STRM_FRM pStrmFrm);
typedef void UVAC_OP_SETTESTIMG(UINT32 imgAddr, UINT32 imgSize);
typedef ER UVAC_OP_CONFIGVIDRESO(PUVAC_VID_RESO pVidReso, UINT32 cnt);
typedef INT32 UVAC_OP_READCDCDATA(CDC_COM_ID ComID, void *p_buf, UINT32 buffer_size, INT32 timeout);
typedef void UVAC_OP_ABORTCDCREAD(CDC_COM_ID ComID);
typedef INT32 UVAC_OP_WRITECDCDATA(CDC_COM_ID ComID, void *p_buf, UINT32 buffer_size, INT32 timeout);

typedef ER UVAC_OP_PULLOUTSTRM(PUVAC_STRM_FRM pStrmFrm, INT32 wait_ms);
typedef ER UVAC_OP_RELEASEOUTSTRM(PUVAC_STRM_FRM pStrmFrm);

typedef ER UVAC_OP_WAITSTRMDONE(UVAC_STRM_PATH path);

typedef struct _UVAC_OPS {
	UVAC_TYPE                 Type;
	UVAC_OP_OPEN             *Open;
	UVAC_OP_CLOSE            *Close;
	UVAC_OP_GETNEEDMEMSIZE   *GetNeedMemSize;
	UVAC_OP_SETCONFIG        *SetConfig;
	UVAC_OP_SETEACHSTRMINFO  *SetEachStrmInfo;
	UVAC_OP_SETTESTIMG       *SetTestImg;
	UVAC_OP_CONFIGVIDRESO    *ConfigVidReso;
	UVAC_OP_READCDCDATA      *ReadCdcData;
	UVAC_OP_ABORTCDCREAD     *AbortCdcRead;
	UVAC_OP_WRITECDCDATA     *WriteCdcData;
	UVAC_OP_PULLOUTSTRM       *PullOutStrm;
	UVAC_OP_RELEASEOUTSTRM    *ReleaseOutStrm;
	UVAC_OP_WAITSTRMDONE     *WaitStrmDone;
} UVAC_OPS;

#endif
