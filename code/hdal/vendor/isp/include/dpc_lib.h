
#ifndef __LIGHTLIB_H__
#define __LIGHTLIB_H__

#include "hd_type.h"
#include "vendor_isp.h"

#define E_OK                   0      //success
#define E_GET_DEV_FAIL        -1      //No ISP device
#define E_GET_EV_FAIL         -2      //Get EV value fail
#define E_NOT_READY           -3      //AE not nready for get EV value
#define E_SYS                 -4      //System fail
#define E_FAIL               -99      //Other fail

typedef struct _Coordinate {
	UINT32  x;              ///< x point
	UINT32  y;              ///< y point
} Coordinate;

// defect pixel (DP) algorithm parameter
typedef enum {
	DP_PARAM_CHGFMT_SKIP,	///< not change format
	DP_PARAM_CHGFMT_AUTO,	///< only change format when dp count not overflow
	DP_PARAM_CHGFMT_FORCE,	///< force to change format

} DP_PARAM_CHGFMT;

typedef struct {
	UINT32 top;
	UINT32 bottom;
	UINT32 left;
	UINT32 right;
} DP_SKIP;


typedef struct {
	UINT32 threshold;   ///< DP detect threshold
	UINT32 block_x;     ///< DP search block X,(must be even,because of bayer fmt)
	UINT32 block_y;     ///< DP search block Y,(must be even,because of bayer fmt)
	DP_SKIP skip_search;
} DP_SETTING;

typedef struct {
	ISPT_RAW_INFO *raw_info; ///< raw inforamtion
	DP_SETTING setting;
	UINT32 max_dp_cnt;		///< Unit: pixel
	UINT32 ori_dp_cnt;
	UINT32 dp_pool_addr;    ///< defect pixel coordinate pool in DRAM
	DP_PARAM_CHGFMT b_chg_dp_format;	///< sort dp data and change format for SIE
	Coordinate sie_act_str;
	Coordinate sie_crp_str;
} CAL_ALG_DP_PARAM;


typedef struct {
	ISPT_RAW_INFO *raw_info; ///< raw inforamtion
	Coordinate raw_str_offset;
	DP_SETTING dp_set;
	UINT32 max_dp_cnt; // max_dp_cnt = height*width*max_cnt_ratio, range = 0~10000 (1/10000)
	UINT32 ori_dp_cnt;
	UINT32 dp_pool_addr;    ///< defect pixel coordinate pool in DRAM
} CAL_ALG_DP_SEARCH_INFO;


typedef struct {
	UINT32 max_cnt_ratio[15]; // max_cnt = height*width*max_cnt_ratio, range = 0~10000 (1/10000)
	DP_SETTING bright_set;
	DP_SETTING dark_set;
} CAL_ALG_DP_SETTING;


// defect pixel (DP) algorithm result
typedef struct {
	UINT32 addr;           ///< dp buffer addr
	UINT32 pixel_cnt;      ///< total defect pixel number
	UINT32 data_length;    ///< dp data length
} CAL_ALG_DP_RST;

//============================================================================
// export lenscen API
//============================================================================
extern unsigned int get_dpc_lib_version(void);
extern unsigned int dpc_get_sensor_info(int sensor_id,  ISPT_SENSOR_MODE_INFO *p_sensor_mode_info);
extern unsigned int dpc_get_raw(int sensor_id, ISPT_RAW_INFO *final_raw_info, UINT32 tmp_buf);
extern unsigned int cal_dp_process(int frame_idx, ISPT_RAW_INFO *raw_info, CAL_ALG_DP_PARAM *dp_param, CAL_ALG_DP_RST *dp_rst, ISPT_SENSOR_MODE_INFO *p_sensor_mode_info);
extern void cali_dpc_set_dbg_out(unsigned int on);
extern void cali_dpc_set_dbg_raw_path(char* file_path);
extern void set_AE(AET_MANUAL *ae_manual);

#endif // __LIGHTLIB_H__
