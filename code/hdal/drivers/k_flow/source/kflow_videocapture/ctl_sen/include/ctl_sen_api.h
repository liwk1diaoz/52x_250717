#ifndef __CTL_SEN_API_h_
#define __CTL_SEN_API_h_
#include "ctl_sen_drv.h"
#include "kwrap/sxcmd.h"
#include "kflow_videocapture/ctl_sen.h"

#define CTL_SEN_PROC_NAME "ctl_sen"
#define CTL_SEN_CEIL_64(n) ((((n) + 63) >> 6) << 6) // for memory 64 byte alignment

typedef struct {
	UINT32 frame[CTL_SEN_MFRAME_MAX_NUM];    ///< exposure frame
	UINT32 line[CTL_SEN_MFRAME_MAX_NUM];     ///< exposure line
	UINT32 pixel[CTL_SEN_MFRAME_MAX_NUM];    ///< exposure pixel
	UINT32 line_ratio;                       ///< exposure line ratio (long expt line * 100 / short expt line)
} CTL_SENDRV_SET_EXPT_PARAM;

typedef struct {
	UINT32 data1[CTL_SEN_MFRAME_MAX_NUM];         ///< depend on sensor register
	UINT32 data2[CTL_SEN_MFRAME_MAX_NUM];         ///< depend on sensor register
	UINT32 data3[CTL_SEN_MFRAME_MAX_NUM];         ///< depend on sensor register
} CTL_SENDRV_SET_GAIN_PARAM;

typedef struct {
	UINT32 frame[CTL_SEN_MFRAME_MAX_NUM];        ///< [in] exposure frame
	UINT32 line[CTL_SEN_MFRAME_MAX_NUM];         ///< [in] exposure line
	UINT32 pixel[CTL_SEN_MFRAME_MAX_NUM];        ///< [in] exposure pixel
	UINT32 line_ratio;                           ///< [in] exposure line ratio (long expt line * 100 / short expt line)
} CTL_SEN_SET_EXPT_PARAM;

typedef struct {
	UINT32 data1[CTL_SEN_MFRAME_MAX_NUM];         ///< [in] depend on sensor register
	UINT32 data2[CTL_SEN_MFRAME_MAX_NUM];         ///< [in] depend on sensor register
	UINT32 data3[CTL_SEN_MFRAME_MAX_NUM];         ///< [in] depend on sensor register
} CTL_SEN_SET_GAIN_PARAM;

typedef struct {
	UINT32 exp_time[CTL_SEN_MFRAME_MAX_NUM];        ///< [in] exposure time
	CTL_SENDRV_SET_EXPT_PARAM expt_param;           ///< [out] exposure parameters
	UINT32 compensate_ratio[CTL_SEN_MFRAME_MAX_NUM];///< [out] compensate ratio
} CTL_SEN_GET_EXPT_PARAM;

typedef struct {
	UINT32 gain_ratio[CTL_SEN_MFRAME_MAX_NUM];  ///< [in] gain ratio (1.5x = 1.5 x 1000 = 1500)
	CTL_SENDRV_SET_GAIN_PARAM gain_param;       ///< [out] gain parameters
} CTL_SEN_GET_GAIN_PARAM;

#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
#else
int ctl_sen_cmd_execute(unsigned char argc, char **argv);
#endif

#endif
