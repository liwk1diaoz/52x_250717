#ifndef _SDE_IOCTL_
#define _SDE_IOCTL_

//=============================================================================
// SDE IOCTL command definition
//=============================================================================
#define ISP_IOC_SDE  'v'
#define SDE_IOC_GET_VERSION             _IOR(ISP_IOC_SDE,     SDET_ITEM_VERSION,            UINT32)
#define SDE_IOC_GET_SIZE_TAB            _IOR(ISP_IOC_SDE,     SDET_ITEM_SIZE_TAB,           SDET_INFO *)
#define SDE_IOC_OPEN                    _IOW(ISP_IOC_SDE,     SDET_ITEM_OPEN,               NULL)
#define SDE_IOC_CLOSE                   _IOW(ISP_IOC_SDE,     SDET_ITEM_CLOSE,              NULL)
#define SDE_IOC_TRIGGER                 _IOW(ISP_IOC_SDE,     SDET_ITEM_TRIGGER,            SDET_TRIG_INFO *)
#define SDE_IOC_GET_IO_INFO             _IOWR(ISP_IOC_SDE,    SDET_ITEM_IO_INFO,            SDET_IO_INFO *)
#define SDE_IOC_SET_IO_INFO             _IOW(ISP_IOC_SDE,     SDET_ITEM_IO_INFO,            SDET_IO_INFO *)
#define SDE_IOC_GET_CTRL                _IOWR(ISP_IOC_SDE,    SDET_ITEM_CTRL_PARAM,         SDET_CTRL_PARAM *)
#define SDE_IOC_SET_CTRL                _IOW(ISP_IOC_SDE,     SDET_ITEM_CTRL_PARAM,         SDET_CTRL_PARAM *)

#endif

