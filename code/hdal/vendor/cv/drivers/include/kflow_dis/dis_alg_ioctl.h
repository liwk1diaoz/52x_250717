#ifndef __DIS_ALG_IOCTL_CMD_H_
#define __DIS_ALG_IOCTL_CMD_H_

#include "kwrap/ioctl.h"


//============================================================================
// IOCTL command
//============================================================================
#define DIS_FLOW_IOC_COMMON_TYPE 'M'

#define DIS_FLOW_IOC_INIT                          _VOS_IO(DIS_FLOW_IOC_COMMON_TYPE,    1)
#define DIS_FLOW_IOC_UNINIT                        _VOS_IO(DIS_FLOW_IOC_COMMON_TYPE,    2)
#define DIS_FLOW_IOC_SET_PARAM_IN                  _VOS_IOWR(DIS_FLOW_IOC_COMMON_TYPE,  3, void*)
#define DIS_FLOW_IOC_GET_PARAM_IN                  _VOS_IOWR(DIS_FLOW_IOC_COMMON_TYPE,  4, void*)
#define DIS_FLOW_IOC_SET_IMG_DMA_IN                _VOS_IOWR(DIS_FLOW_IOC_COMMON_TYPE,  5, void*)
#define DIS_FLOW_IOC_SET_ONLY_MV_EN                _VOS_IOWR(DIS_FLOW_IOC_COMMON_TYPE,  6, void*)
#define DIS_FLOW_IOC_GET_ONLY_MV_EN                _VOS_IOWR(DIS_FLOW_IOC_COMMON_TYPE,  7, void*)
#define DIS_FLOW_IOC_GET_MV_MAP_OUT                _VOS_IOWR(DIS_FLOW_IOC_COMMON_TYPE,  8, void*)
#define DIS_FLOW_IOC_GET_MV_MDS_DIM                _VOS_IOWR(DIS_FLOW_IOC_COMMON_TYPE,  9, void*)
#define DIS_FLOW_IOC_SET_MV_BLOCKS_DIM             _VOS_IOWR(DIS_FLOW_IOC_COMMON_TYPE,  10, void*)
#define DIS_FLOW_IOC_GET_MV_BLOCKS_DIM             _VOS_IOWR(DIS_FLOW_IOC_COMMON_TYPE,  11, void*)
#define DIS_FLOW_IOC_TRIGGER                       _VOS_IO(DIS_FLOW_IOC_COMMON_TYPE,    12)
#define DIS_FLOW_IOC_GET_VER                       _VOS_IOWR(DIS_FLOW_IOC_COMMON_TYPE,  13, void*)




/* Add other command ID here*/
#if defined(__FREERTOS)
int nvt_dis_alg_ioctl(int fd, unsigned int uiCmd, void *p_arg);
#endif



#endif
