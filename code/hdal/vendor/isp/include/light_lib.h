
#ifndef __LIGHTLIB_H__
#define __LIGHTLIB_H__

#include "hd_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define E_OK                   0      //success
#define E_GET_DEV_FAIL        -1      //No ISP device
#define E_GET_EV_FAIL         -2      //Get EV value fail
#define E_NOT_READY           -3      //AE not nready for get EV value
#define E_SYS                 -4      //System fail
#define E_FAIL               -99      //Other fail



/**
    CA data
*/
typedef struct _CA_DATA {
	UINT32 win_num_x;
	UINT32 win_num_y;
	UINT16 tab[3][32][32];
} CA_DATA;

/**
	AWB Alg Status
*/
typedef enum _AWB_MODE {
	AWB_ALG_MODE          = 0,
	AWB_GRAY_MODE,
	ENUM_DUMMY4WORD(AWB_MODE)
} AWB_MODE;

/**
    AWB Current Status
*/
typedef struct _LIGHT_STATUS {
	UINT32 r_gain;
	UINT32 g_gain;
	UINT32 b_gain;
	AWB_MODE mode;
} LIGHT_AWB_STATUS;

typedef struct _LIGHT_INFO {
	CA_DATA *ca;                       //< CA data
	UINT32 iso_expt_fn[3];
	LIGHT_AWB_STATUS light_awb_status;
} LIGHT_INFO;

typedef void (*LIGHT_INFO_CB_FP)(LIGHT_INFO *light_info);
//============================================================================
// global
//============================================================================


/*
Function: sw_cds_init
Description: Init sw cds library and set parameters.
Parameter:
    1. night_ev : EV threshold to switch to night mode
    2. day_ev   : EV threshold to switch to day mode. **Must greater thad night_ev to avoid vibration**
    3. sensitive: Threshold to judge the light source type.
*/
extern INT sw_cds_init(UINT32 id, INT night_ev, INT day_ev, INT sensitive, INT open_device);

/*
Function: sw_cds_exit
Description: Exit sw cds library to release device file hanlde.
Parameter: none
*/
extern INT sw_cds_exit(void);

/*
Function: sw_cds_set_max_ir_ev
Description: Set the max ev value of IR light.
Parameter: ev value
*/
extern INT sw_cds_set_max_ir_ev(INT ev);

/*
Function: sw_cds_get_curr_ev
Description: Function to get curret environment luminance.
Parameter: none
*/
extern INT sw_cds_get_curr_ev(void);

/*
Function: sw_cds_night_pre_cb
Description: Call back function before setting to night mode.
Parameter: none
*/
extern void sw_cds_night_pre_cb(void);

/*
Function: sw_cds_night_post_cb
Description: Call back function after setting to night mode.
Parameter: none
*/
extern void sw_cds_night_post_cb(void);

/*
Function: sw_cds_check_env_night
Description: Call to sw cds library to check if switch to night mode.
Parameter: none
Return value:  -1: waiting AE converge timeout
				0: Not night scene
				1: Night Scene
*/
extern INT sw_cds_check_env_night(void);

/*
Function: sw_cds_check_env_day
Description: Call to sw cds library to check if switch to day mode.
Parameter: none
Return value:  -1: waiting AE converge timeout
				0: Not night scene
				1: Night Scene
*/
extern INT sw_cds_check_env_day(void);

/*
Function: sw_cds_get_parameter
Description: sw cds get parameters.
Parameter:      1. night_ev : EV threshold to switch to night mode
				2. day_ev   : EV threshold to switch to day mode. **Must greater thad night_ev to avoid vibration**
				3. sensitive: Threshold to judge the light source type.
*/
extern INT sw_cds_get_parameter(INT *night_ev, INT *day_ev, INT *sensitive);

/*
Function: sw_cds_set_parameter
Description: sw cds set parameters.
Parameter:
    1. night_ev : EV threshold to switch to night mode
    2. day_ev   : EV threshold to switch to day mode. **Must greater thad night_ev to avoid vibration**
    3. sensitive: Threshold to judge the light source type.
*/
extern INT sw_cds_set_parameter(INT night_ev, INT day_ev, INT sensitive);

/*
Function: sw_cds_set_dbg_out
Description: Function to set debug message output on/off.
Parameter: on : 0 -> disable ; 1 -> enable
*/
extern void sw_cds_set_dbg_out(INT on);


//extern void light_lib_trig(LIGHT_PARAM *param);
extern void light_info_reg_cb(LIGHT_INFO_CB_FP cb_fp);
#ifdef __cplusplus
}
#endif

#endif // __LIGHTLIB_H__
