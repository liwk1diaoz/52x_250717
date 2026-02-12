#ifndef _AE_NNSC_H_
#define _AE_NNSC_H_

#define AE_NNSC_INFOR_ERR   0xFFFFFFFF

typedef struct _AE_NNSC_INFO_TAB {
	UINT32 value;
	UINT32 max_value;
} AE_NNSC_INFO_TAB;

/**
	AE informatin selection index(ae_set_nnsc_info() or ae_get_nnsc_info())
*/
typedef enum _AE_NNSC_ITEM {
	AE_NNSC_ITEM_EXP_RATIO,           ///<Exposure enhance ratio, data type #AE_NNSC_EXP_ENH_RATIO
	AE_NNSC_ITEM_MAX_CNT,
	ENUM_DUMMY4WORD(AE_NNSC_ITEM)
} AE_NNSC_ITEM;

/**
	NNSC exposure ratio
*/
typedef enum _AE_NNSC_EXP_RATIO {
	AE_NNSC_EXP_RATIO_SUP_STRONG       = 0,
	AE_NNSC_EXP_RATIO_SUP_WEAK         = 25,
	AE_NNSC_EXP_RATIO_OFF              = 50,
	AE_NNSC_EXP_RATIO_ENH_WEAK         = 75,
	AE_NNSC_EXP_RATIO_ENH_STRONG       = 100,
	AE_NNSC_EXP_RATIO_MAX              = 101,
	ENUM_DUMMY4WORD(AE_NNSC_EXP_RATIO)
} AE_NNSC_EXP_RATIO;

#endif

