#ifndef _VPE_IOCTL_
#define _VPE_IOCTL_

//=============================================================================
// VPE IOCTL command definition
//=============================================================================
#define ISP_IOC_VPE  'v'
#define VPE_IOC_GET_VERSION             _IOR(ISP_IOC_VPE,     VPET_ITEM_VERSION,            UINT32)
#define VPE_IOC_GET_SIZE_TAB            _IOR(ISP_IOC_VPE,     VPET_ITEM_SIZE_TAB,           VPET_INFO *)
#define VPE_IOC_GET_SHARPEN             _IOWR(ISP_IOC_VPE,    VPET_ITEM_SHARPEN_PARAM,      VPET_SHARPEN_PARAM *)
#define VPE_IOC_SET_SHARPEN             _IOW(ISP_IOC_VPE,     VPET_ITEM_SHARPEN_PARAM,      VPET_SHARPEN_PARAM *)
#define VPE_IOC_GET_DCE_CTL             _IOWR(ISP_IOC_VPE,    VPET_ITEM_DCE_CTL_PARAM,      VPET_DCE_CTL_PARAM *)
#define VPE_IOC_SET_DCE_CTL             _IOW(ISP_IOC_VPE,     VPET_ITEM_DCE_CTL_PARAM,      VPET_DCE_CTL_PARAM *)
#define VPE_IOC_GET_GDC                 _IOWR(ISP_IOC_VPE,    VPET_ITEM_GDC_PARAM,          VPET_GDC_PARAM *)
#define VPE_IOC_SET_GDC                 _IOW(ISP_IOC_VPE,     VPET_ITEM_GDC_PARAM,          VPET_GDC_PARAM *)
#define VPE_IOC_GET_2DLUT               _IOWR(ISP_IOC_VPE,    VPET_ITEM_2DLUT_PARAM,        VPET_2DLUT_PARAM *)
#define VPE_IOC_SET_2DLUT               _IOW(ISP_IOC_VPE,     VPET_ITEM_2DLUT_PARAM,        VPET_2DLUT_PARAM *)
#define VPE_IOC_GET_DRT                 _IOWR(ISP_IOC_VPE,    VPET_ITEM_DRT_PARAM,          VPET_DRT_PARAM *)
#define VPE_IOC_SET_DRT                 _IOW(ISP_IOC_VPE,     VPET_ITEM_DRT_PARAM,          VPET_DRT_PARAM *)
#define VPE_IOC_GET_DCTG                _IOWR(ISP_IOC_VPE,    VPET_ITEM_DCTG_PARAM,         VPET_DCTG_PARAM *)
#define VPE_IOC_SET_DCTG                _IOW(ISP_IOC_VPE,     VPET_ITEM_DCTG_PARAM,         VPET_DCTG_PARAM *)
#define VPE_IOC_GET_FLIP_ROT            _IOWR(ISP_IOC_VPE,    VPET_ITEM_FLIP_ROT_PARAM,     VPET_FLIP_ROT_PARAM *)
#define VPE_IOC_SET_FLIP_ROT            _IOW(ISP_IOC_VPE,     VPET_ITEM_FLIP_ROT_PARAM,     VPET_FLIP_ROT_PARAM *)



#endif

