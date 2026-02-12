#if defined(__FREERTOS)
#include "string.h"
#else
#include <linux/syscalls.h>
#include <linux/moduleparam.h>
#endif
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "vpe_dev_int.h"

#include "vpet_api.h"
#include "vpe_dbg.h"
#include "vpe_main.h"
#include "vpe_version.h"
#include "vpe_common_param_int.h"
#include "vpe_uti.h"


//=============================================================================
// extern include
//=============================================================================
extern VPE_PARAM_PTR *vpe_param[VPE_ID_MAX_NUM];
extern BOOL vpe_param_id_valid[VPE_ID_MAX_NUM];
extern USIZE vpe_2dlut_output_size[VPE_ID_MAX_NUM];
extern UINT32 vpe_2dlut_size;


//=============================================================================
// global
//=============================================================================
typedef void (*vpet_fp)(UINT32 addr);
UINT32 real_2dlut_size = VPE_2DLUT_NUM * sizeof(UINT32);

//=============================================================================
// function declaration
//=============================================================================
static void vpet_api_get_version(UINT32 addr);
static void vpet_api_get_size_tab(UINT32 addr);
static void vpet_api_get_sharpen(UINT32 addr);
static void vpet_api_get_dce_ctl(UINT32 addr);
static void vpet_api_get_gdc(UINT32 addr);
static void vpet_api_get_2dlut(UINT32 addr);
static void vpet_api_get_drt(UINT32 addr);
static void vpet_api_get_dctg(UINT32 addr);
static void vpet_api_get_flip_rot(UINT32 addr);
static void vpet_api_set_sharpen(UINT32 addr);
static void vpet_api_set_dce_ctl(UINT32 addr);
static void vpet_api_set_gdc(UINT32 addr);
static void vpet_api_set_2dlut(UINT32 addr);
static void vpet_api_set_drt(UINT32 addr);
static void vpet_api_set_dctg(UINT32 addr);
static void vpet_api_set_flip_rot(UINT32 addr);
static void vpet_api_reserve(UINT32 addr);

#define RESERVE_SIZE 0

static VPET_INFO vpet_info = { {
	//id                             size
	{VPET_ITEM_VERSION,              sizeof(UINT32)                   },
	{VPET_ITEM_SIZE_TAB,             sizeof(VPET_INFO)                },
	{VPET_ITEM_RESERVE_02,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_03,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_04,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_05,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_06,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_07,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_08,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_09,           RESERVE_SIZE                     },
	{VPET_ITEM_SHARPEN_PARAM,        sizeof(VPET_SHARPEN_PARAM)       },
	{VPET_ITEM_DCE_CTL_PARAM,        sizeof(VPET_DCE_CTL_PARAM)       },
	{VPET_ITEM_GDC_PARAM,            sizeof(VPET_GDC_PARAM)           },
	{VPET_ITEM_2DLUT_PARAM,          sizeof(VPET_2DLUT_PARAM)         },
	{VPET_ITEM_DRT_PARAM,            sizeof(VPET_DRT_PARAM)           },
	{VPET_ITEM_DCTG_PARAM,           sizeof(VPET_DCTG_PARAM)          },
	{VPET_ITEM_FLIP_ROT_PARAM,       sizeof(VPET_FLIP_ROT_PARAM)      },
	{VPET_ITEM_RESERVE_17,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_18,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_19,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_20,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_21,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_22,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_23,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_24,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_25,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_26,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_27,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_28,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_29,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_30,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_31,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_32,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_33,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_34,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_35,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_36,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_37,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_38,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_39,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_40,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_41,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_42,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_43,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_44,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_45,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_46,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_47,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_48,           RESERVE_SIZE                     },
	{VPET_ITEM_RESERVE_49,           RESERVE_SIZE                     }
} };

BOOL vpe_update[VPE_ID_MAX_NUM][VPET_ITEM_MAX];

static vpet_fp vpet_get_tab[VPET_ITEM_MAX] = {
	vpet_api_get_version,
	vpet_api_get_size_tab,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,              // 5
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_get_sharpen,          // 10
	vpet_api_get_dce_ctl,
	vpet_api_get_gdc,
	vpet_api_get_2dlut,
	vpet_api_get_drt,
	vpet_api_get_dctg,              // 15
	vpet_api_get_flip_rot,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,              // 20
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,              // 25
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,              // 30
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,              // 35
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,              // 40
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,              // 45
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve
};

static vpet_fp vpet_set_tab[VPET_ITEM_MAX] = {
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,               // 5
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_set_sharpen,           // 10
	vpet_api_set_dce_ctl,
	vpet_api_set_gdc,
	vpet_api_set_2dlut,
	vpet_api_set_drt,
	vpet_api_set_dctg,              // 15
	vpet_api_set_flip_rot,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,              // 20
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,              // 25
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,              // 30
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,              // 35
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,              // 40
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,              // 45
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve,
	vpet_api_reserve
};

UINT32 vpet_api_get_item_size(VPET_ITEM item)
{
	return vpet_info.size_tab[item][1];
}

static void vpet_api_get_version(UINT32 addr)
{
	UINT32 *data = (UINT32 *)addr;

	*data = vpe_get_version();
}

static void vpet_api_get_size_tab(UINT32 addr)
{
	memcpy((VPET_INFO *)addr, &vpet_info, sizeof(VPET_INFO));
}

static void vpet_api_get_sharpen(UINT32 addr)
{
	VPET_SHARPEN_PARAM *data = (VPET_SHARPEN_PARAM *)addr;

	if(vpe_param_id_valid[data->id]!= TRUE){
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", data->id);
		return;
	}else{
		memcpy(&(data->sharpen), vpe_param[data->id]->sharpen, sizeof(VPE_SHARPEN_PARAM));
	}
}

static void vpet_api_get_dce_ctl(UINT32 addr)
{
	VPET_DCE_CTL_PARAM *data = (VPET_DCE_CTL_PARAM *)addr;

	if(vpe_param_id_valid[data->id]!= TRUE){
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", data->id);
		return;
	}else{
		memcpy(&(data->dce_ctl), vpe_param[data->id]->dce_ctl, sizeof(VPE_DCE_CTL_PARAM));
	}
}

static void vpet_api_get_gdc(UINT32 addr)
{
	VPET_GDC_PARAM *data = (VPET_GDC_PARAM *)addr;

	if(vpe_param_id_valid[data->id]!= TRUE){
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", data->id);
		return;
	}else{
		memcpy(&(data->gdc), vpe_param[data->id]->gdc, sizeof(VPE_GDC_PARAM));
	}
}

static void vpet_api_get_2dlut(UINT32 addr)
{
	VPET_2DLUT_PARAM *data = (VPET_2DLUT_PARAM *)addr;

	if(vpe_param_id_valid[data->id]!= TRUE){
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", data->id);
		return;
	}else{
		memcpy(&(data->lut2d), vpe_param[data->id]->lut2d, sizeof(VPE_2DLUT_PARAM));
	}
}

static void vpet_api_get_drt(UINT32 addr)
{
	VPET_DRT_PARAM *data = (VPET_DRT_PARAM *)addr;

	if(vpe_param_id_valid[data->id]!= TRUE){
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", data->id);
		return;
	}else{
		memcpy(&(data->drt), vpe_param[data->id]->drt, sizeof(VPE_DRT_PARAM));
	}
}

static void vpet_api_get_dctg(UINT32 addr)
{
	VPET_DCTG_PARAM *data = (VPET_DCTG_PARAM *)addr;

	if(vpe_param_id_valid[data->id]!= TRUE){
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", data->id);
		return;
	}else{
		memcpy(&(data->dctg), vpe_param[data->id]->dctg, sizeof(VPE_DCTG_PARAM));
	}
}

static void vpet_api_get_flip_rot(UINT32 addr)
{
	VPET_FLIP_ROT_PARAM *data = (VPET_FLIP_ROT_PARAM *)addr;

	if (vpe_param_id_valid[data->id]!= TRUE) {
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", data->id);
		return;
	} else {
		memcpy(&(data->flip_rot), vpe_param[data->id]->flip_rot, sizeof(VPE_FLIP_ROT_PARAM));
	}
}

UINT32 vpet_get_param_update(UINT32 id, VPET_ITEM item)
{
	return vpe_update[id][item];
}


ER vpet_api_get_cmd(VPET_ITEM item, UINT32 addr)
{
	if (vpet_get_tab[item] == NULL) {
		DBG_ERR("vpet_get_tab(%d) NULL!!\r\n", item);
		return E_SYS;
	}
	if ((item != VPET_ITEM_VERSION) && (item != VPET_ITEM_SIZE_TAB) && (*((UINT32 *)addr) >= VPE_ID_MAX_NUM)) {
		DBG_ERR("item(%d) id(%d) out of range\r\n", item, *((UINT32 *)addr));
		return E_SYS;
	}
	if (item >= VPET_ITEM_MAX) {
		DBG_ERR("item(%d) out of range\r\n", item);
		return E_SYS;
	}

	vpet_get_tab[item](addr);
	return E_OK;
}

static void vpet_api_set_sharpen(UINT32 addr)
{
	VPET_SHARPEN_PARAM *data = (VPET_SHARPEN_PARAM *)addr;

	if(vpe_param_id_valid[data->id]!= TRUE){
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", data->id);
		return;
	}else{
		memcpy(vpe_param[data->id]->sharpen, &(data->sharpen), sizeof(VPE_SHARPEN_PARAM));
	}

	vpe_update[data->id][VPET_ITEM_SHARPEN_PARAM] = TRUE;
}

static void vpet_api_set_dce_ctl(UINT32 addr)
{
	VPET_DCE_CTL_PARAM *data = (VPET_DCE_CTL_PARAM *)addr;

	if(vpe_param_id_valid[data->id]!= TRUE){
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", data->id);
		return;
	}else{
		memcpy(vpe_param[data->id]->dce_ctl, &(data->dce_ctl), sizeof(VPE_DCE_CTL_PARAM));
	}

	vpe_update[data->id][VPET_ITEM_DCE_CTL_PARAM] = TRUE;
}

static void vpet_api_set_gdc(UINT32 addr)
{
	VPET_GDC_PARAM *data = (VPET_GDC_PARAM *)addr;
	if(vpe_param_id_valid[data->id]!= TRUE){
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", data->id);
		return;
	}else{
		memcpy(vpe_param[data->id]->gdc, &(data->gdc), sizeof(VPE_GDC_PARAM));
	}

	vpe_update[data->id][VPET_ITEM_GDC_PARAM] = TRUE;
}

static void vpet_api_set_2dlut(UINT32 addr)
{
	VPET_2DLUT_PARAM *data = (VPET_2DLUT_PARAM *)addr;

	if(vpe_param_id_valid[data->id]!= TRUE){
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", data->id);
		return;
	}else{
		//memcpy(vpe_param[data->id]->lut2d, &(data->lut2d), sizeof(VPE_2DLUT_PARAM));

		real_2dlut_size = vpe_uti_calc_2dlut_tbl_size(data->lut2d.lut_sz);
		vpe_param[data->id]->lut2d->lut_sz = data->lut2d.lut_sz;

		memcpy(vpe_param[data->id]->lut2d->lut, data->lut2d.lut, real_2dlut_size);

		vpe_uti_calc_2dlut_output_size(&vpe_2dlut_output_size[data->id], data->lut2d.lut_sz, data->lut2d.lut);

		vpe_update[data->id][VPET_ITEM_2DLUT_PARAM] = TRUE;
	}
}

static void vpet_api_set_drt(UINT32 addr)
{
	VPET_DRT_PARAM *data = (VPET_DRT_PARAM *)addr;

	if(vpe_param_id_valid[data->id]!= TRUE){
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", data->id);
		return;
	}else{
		memcpy(vpe_param[data->id]->drt, &(data->drt), sizeof(VPE_DRT_PARAM));
	}

	vpe_update[data->id][VPET_ITEM_DRT_PARAM] = TRUE;
}

static void vpet_api_set_dctg(UINT32 addr)
{
	VPET_DCTG_PARAM *data = (VPET_DCTG_PARAM *)addr;

	if(vpe_param_id_valid[data->id]!= TRUE){
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", data->id);
		return;
	}else{
		memcpy(vpe_param[data->id]->dctg, &(data->dctg), sizeof(VPE_DCTG_PARAM));
	}

	vpe_update[data->id][VPET_ITEM_DCTG_PARAM] = TRUE;
}

static void vpet_api_set_flip_rot(UINT32 addr)
{
	VPET_FLIP_ROT_PARAM *data = (VPET_FLIP_ROT_PARAM *)addr;

	if (vpe_param_id_valid[data->id]!= TRUE) {
		DBG_DUMP("invalid VPE ID (%d)!! \r\n", data->id);
		return;
	} else {
		memcpy(vpe_param[data->id]->flip_rot, &(data->flip_rot), sizeof(VPE_FLIP_ROT_PARAM));

		vpe_update[data->id][VPET_ITEM_FLIP_ROT_PARAM] = TRUE;
	}
}

static void vpet_api_reserve(UINT32 addr)
{
	return;
}

ER vpet_api_set_cmd(VPET_ITEM item, UINT32 addr)
{
	if (vpet_set_tab[item] == NULL) {
		DBG_ERR("vpet_set_tab(%d) NULL!!\r\n", item);
		return E_SYS;
	}
	if (*((UINT32 *)addr) >= VPE_ID_MAX_NUM) {
		DBG_ERR("id(%d) out of range\r\n", *((UINT32 *)addr));
		return E_SYS;
	}
	if (item >= VPET_ITEM_MAX) {
		DBG_ERR("item(%d) out of range\r\n", item);
		return E_SYS;
	}

	vpet_set_tab[item](addr);
	return E_OK;
}

void vpet_reset_param_update(UINT32 id, VPET_ITEM item)
{
	vpe_update[id][item] = FALSE;
}



