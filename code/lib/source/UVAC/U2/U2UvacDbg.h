#ifndef _USB_U2UVC_DBG_H_
#define _USB_U2UVC_DBG_H_
//#include <kwrap/debug.h>

//------------------------------------------------------------
#define _UVC_DBG_NONE_           0
#define _UVC_DBG_CHK_            1
#define _UVC_DBG_IO_             2
#define _UVC_DBG_ALL_            3

#define _UVC_DBG_LVL_            _UVC_DBG_NONE_
#if (_UVC_DBG_LVL_ == _UVC_DBG_NONE_)
#define DbgMsg_UVCIO(msg)
#define DbgMsg_UVC(msg)
#define DbgCode_UVC(x)
#elif (_UVC_DBG_LVL_ == _UVC_DBG_ALL_)
#define DbgMsg_UVCIO(msg)      DBG_DUMP msg
#define DbgMsg_UVC(msg)        DBG_DUMP msg
#define DbgCode_UVC(x)         x
#elif (_UVC_DBG_LVL_ == _UVC_DBG_CHK_)
#define DbgMsg_UVCIO(msg)
#define DbgMsg_UVC(msg)        DBG_DUMP msg
#define DbgMsg_UVCFUNC(msg)    DBG_DUMP("%s",__func__)
#define DbgCode_UVC(x)         x
#elif (_UVC_DBG_LVL_ == _UVC_DBG_IO_)
#define DbgMsg_UVCIO(msg)      DBG_DUMP msg
#define DbgMsg_UVC(msg)
#define DbgCode_UVC(x)
#endif
//------------------------------------------------------------

//------------------------------------------------------------
#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UVAC
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

#if (THIS_DBGLVL >= 5)
#define DbgCode_PTP(x)         x
#define DbgCode_MTP(x)         x
#else
#define DbgCode_PTP(x)
#define DbgCode_MTP(x)
#endif

#define UVAC_DBG_CMD_NONE                    0

#define UVAC_DBG_ISO_TX             0x00000001
#define UVAC_DBG_ISO_QUE_CNT        0x00000002


#define UVAC_DBG_VID_START          0x00000001
#define UVAC_DBG_VID_QUEUE          0x00000002
#define UVAC_DBG_VID_PAYLOAD        0x00000004
#define UVAC_DBG_VID_RCV_ALL        0x00000008
#define UVAC_DBG_VID_RCV_V1         0x00000010
#define UVAC_DBG_VID_RCV_V2         0x00000020
#define UVAC_DBG_VID_RCV_A1         0x00000040
#define UVAC_DBG_VID_RCV_A2         0x00000080
#define UVAC_DBG_VID_QUE_CNT        0x00000100


extern UINT32 m_uiU2UvacDbgIso;
extern UINT32 m_uiU2UvacDbgVid;

extern void xU2UVAC_InstallCmd(void);


#endif

