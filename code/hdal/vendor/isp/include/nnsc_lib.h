#ifndef _NNSC_LIB_
#define _NNSC_LIB_

#include "hd_type.h"

//=============================================================================
// define
//=============================================================================
#define NNSC_PARAM_SENSITIVE_BASE 5
#define NNSC_PARAM_SENSITIVE_MAX  10
#define NNSC_PARAM_STRENGTH_BASE  100
#define NNSC_PARAM_STRENGTH_MAX   200

#define NNSC_CA_MAX_WINNUM        (32 * 32)
#define NNSC_CA_MAX_NUM           ((1 << 12) -1)

#define NNSC_DBG_TYPE_NULL        0x00000000
#define NNSC_DBG_TYPE_MAIN        0x00000001
#define NNSC_DBG_TYPE_SCD         0x00000002
#define NNSC_DBG_TYPE_AE          0x00000010
#define NNSC_DBG_TYPE_AWB         0x00000020
#define NNSC_DBG_TYPE_IQ          0x00000040
#define NNSC_DBG_TYPE_PARAM       0x00000100
#define NNSC_DBG_TYPE_ALL         0xFFFFFFFF

//=============================================================================
// struct & enum definition
//=============================================================================
typedef enum _NNSC_SCORE_ITEM {
	NNSC_SCORE_BACKLIGHT_HUMAN,
	NNSC_SCORE_BACKLIGHT,
	NNSC_SCORE_FOGGY,
	NNSC_SCORE_GRASS_A,
	NNSC_SCORE_GRASS_B,
	NNSC_SCORE_HAND_COLOR,
	NNSC_SCORE_NORMAL,
	NNSC_SCORE_SNOWFIELD,
	NNSC_SCORE_MAX_NUM,
	ENUM_DUMMY4WORD(NNSC_SCORE_ITEM)
} NNSC_SCORE_ITEM;

typedef enum _NNSC_TYPE {
	NNSC_TYPE_BACKLIGHT,
	NNSC_TYPE_FOGGY,
	NNSC_TYPE_GRASS,
	NNSC_TYPE_SKIN_COLOR,
	NNSC_TYPE_SNOWFIELD,
	NNSC_TYPE_MAX_NUM,
	ENUM_DUMMY4WORD(NNSC_TYPE)
} NNSC_TYPE;

typedef struct _NNSC_TUNE_PARAM {
	UINT32 sensitive;                           ///< range : 0~10, 0 donot detect, 5 normal, 10 sensitive
	UINT32 adj_strength;                        ///< range : 0~200, 0 = max sun tanning, 100 = no adjust, 200 = max whitening for SKIN_COLOR
												///<                0 = no adjust, 100 = 1x adjust, 200 = max adjust for others
} NNSC_TUNE_PARAM;

typedef struct _NNSC_SCORE {
	UINT32 score[NNSC_SCORE_MAX_NUM];
} NNSC_SCORE;

typedef struct _NNSC_PARAM {
	NNSC_TUNE_PARAM param[NNSC_TYPE_MAX_NUM];
} NNSC_PARAM;

typedef struct _NNSC_ADJ {
	UINT32 exp_ratio;                           // range : 0~100, AE implement
	BOOL green_remove;                          // range : 0~1, AWB implement
	BOOL skin_remove;                           // range : 0~1, AWB implement
	UINT32 dark_enh_ratio;                      // range : 0~100, IQ implement
	UINT32 contrast_enh_ratio;                  // range : 0~100, IQ implement
	UINT32 green_enh_ratio;                     // range : 0~100, IQ implement
	UINT32 skin_enh_ratio;                      // range : 0~100, IQ implement
} NNSC_ADJ;

typedef struct _NNSC_TEST {
	BOOL enable;
	NNSC_TYPE mode_type;
} NNSC_TEST;

typedef struct _SCD_CONTROL {
	BOOL enable;
	BOOL auto_enable;
	BOOL manual_stable;
} SCD_CONTROL;

typedef struct _SCD_PARAM {
	UINT32 stable_sensitive;                    // range : 0~10, tolerance count number threshold for stable status
	UINT32 change_sensitive;                    // range : 0~10, tolerance count number threshold for changing status
	UINT32 g_diff_num_th;                       // range : 0~1024, block number threshold of cnhaging block
} SCD_PARAM;

typedef struct _SCD_STATISTICS {
	UINT16 ca_r[NNSC_CA_MAX_WINNUM];
	UINT16 ca_g[NNSC_CA_MAX_WINNUM];
	UINT16 ca_b[NNSC_CA_MAX_WINNUM];
} SCD_STATISTICS;

//=============================================================================
// extern functions
//=============================================================================
extern HD_RESULT nnsc_adj_cal(NNSC_ADJ *adj_ratio);
extern UINT32 nnsc_get_version(void);
extern HD_RESULT nnsc_set_score(NNSC_SCORE *score);
extern HD_RESULT nnsc_set_param(NNSC_PARAM *param);
extern HD_RESULT nnsc_set_test_mode(NNSC_TEST *test);
extern HD_RESULT nnsc_set_dbg_out(UINT32 type);
extern HD_RESULT nnsc_set_scd_control(SCD_CONTROL *control);
extern HD_RESULT nnsc_set_scd_param(SCD_PARAM *param);
extern HD_RESULT nnsc_set_scd_statistic(SCD_STATISTICS *statis);

#endif

