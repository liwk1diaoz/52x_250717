#ifndef _MD_LIB_
#define _MD_LIB_

#include "hd_type.h"

//=============================================================================
// define
//=============================================================================
#define MD_MAX_ID                  2
#define MD_LA_W_WINNUM             32
#define MD_LA_H_WINNUM             32
#define MD_LA_MAX_WINNUM           MD_LA_W_WINNUM*MD_LA_H_WINNUM

//=============================================================================
// struct & enum definition
//=============================================================================
typedef enum _MD_RSLT {
	NO_MOTION,
	MOTION,
	NOT_VALID,
	ENUM_DUMMY4WORD(MD_RSLT)
} MD_RSLT;

typedef struct _MD_LIB_RSLT {
	UINT8 th[MD_LA_MAX_WINNUM];       ///< Output, 
	UINT16 blk_dif_cnt;               ///< Output,
	UINT32 total_blk_diff;            ///< Output,
	MD_RSLT rslt;                     ///< Output,
} MD_LIB_RSLT;

typedef struct _MD_LIB_PARAM {
	UINT8 sum_frms;                   ///< Input,
	UINT32 mask0;                     ///< Input,
	UINT32 mask1;                     ///< Input,
	UINT32 blkdiff_thr;               ///< Input,
	UINT32 total_blkdiff_thr;         ///< Input,
	UINT16 blkdiff_cnt_thr;           ///< Input,
} MD_LIB_PARAM;

typedef struct _MD_LA_DATA {
	UINT16 lum[MD_LA_MAX_WINNUM];   ///< Input,
} MD_LA_DATA;

//=============================================================================
// extern functions
//=============================================================================
extern HD_RESULT md_get_rslt(UINT32 id, MD_LIB_RSLT *rslt);
extern HD_RESULT md_get_param(UINT32 id, MD_LIB_PARAM *param);
extern HD_RESULT md_set_param(UINT32 id, MD_LIB_PARAM *param);
extern HD_RESULT md_trig_flow(UINT32 id, MD_LA_DATA *la_data);

#endif

