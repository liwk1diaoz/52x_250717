#if defined(__FREERTOS)
#include <string.h>
#include <stdlib.h>
#include "vpe_dbg.h"
#include "vpe_dev_int.h"
#else
#include <linux/slab.h>

#include "vpe_ioctl.h"
#include "vpe_dbg.h"
#include "vpe_main.h"
#endif

#include "kwrap/error_no.h"
#include "ctl_vpe_isp.h"

#include "vpe_alg.h"
#include "vpe_api_int.h"
#include "vpe_param_default.h"

//=============================================================================
// define
//=============================================================================
#define VPE_IF_REG_NAME             "NVT_VPE_IF"

//=============================================================================
// global variable
//=============================================================================
extern UINT32 vpe_id_list;
#if defined(__FREERTOS)
static VPE_DEV_INFO pdev_info;
#endif

VPE_MODULE vpe_module;

VPE_PARAM_PTR *vpe_param[VPE_ID_MAX_NUM] = {NULL};
VPE_PARAM_PTR vpe_param_memalloc_addr[VPE_ID_MAX_NUM] = {0};
static BOOL vpe_param_memalloc_valid[VPE_ID_MAX_NUM] = {0};
BOOL vpe_param_id_valid[VPE_ID_MAX_NUM] = {0};
static UINT32 vpe_info_id_map[VPE_ID_MAX_NUM] = {0};
static UINT32 vpe_info_id_map_index;

//=============================================================================
// function declaration
//=============================================================================


//=============================================================================
// external functions
//=============================================================================
#if defined(__FREERTOS)
VPE_DEV_INFO *vpe_get_dev_info(void)
{
	return &pdev_info;
}
#endif

//=============================================================================
// internal functions
//=============================================================================
static INT32 vpe_dev_get_param_addr(VPE_ID id)
{
	UINT32 total_param_size;
	VPE_PARAM_PTR *vpe_param_temp = NULL;
	void *param_mem_addr = NULL;
	static BOOL use_param_phy_addr = TRUE;
	int align_byte = 4;

	if (use_param_phy_addr == TRUE) {
		use_param_phy_addr = FALSE;
		vpe_param[id] = (VPE_PARAM_PTR*)vpe_get_param_default();
	} else {
		total_param_size = ALIGN_CEIL(sizeof(VPE_SHARPEN_PARAM), align_byte)+ ALIGN_CEIL(sizeof(VPE_DCE_CTL_PARAM), align_byte)
			+ALIGN_CEIL(sizeof(VPE_GDC_PARAM), align_byte)+ALIGN_CEIL(sizeof(VPE_2DLUT_PARAM), align_byte)+ ALIGN_CEIL(sizeof(VPE_DRT_PARAM), align_byte)+ALIGN_CEIL(sizeof(VPE_DCTG_PARAM), align_byte);
		#if defined(__FREERTOS)
		param_mem_addr = malloc(total_param_size);
		if (param_mem_addr == NULL) {
			DBG_ERR("fail to allocate vpe parameter fail!\n");
			return -E_SYS;
		}
		#else
		param_mem_addr = kzalloc(total_param_size, GFP_KERNEL);
		if (param_mem_addr == NULL) {
			DBG_ERR("fail to allocate vpe parameter fail!\n");
			return -ENOMEM;
		}
		#endif
		vpe_param_memalloc_addr[id].sharpen = (VPE_SHARPEN_PARAM *)param_mem_addr;
		vpe_param_memalloc_addr[id].dce_ctl = (VPE_DCE_CTL_PARAM *)((UINT8 *)vpe_param_memalloc_addr[id].sharpen + ALIGN_CEIL(sizeof(VPE_SHARPEN_PARAM), align_byte));
		vpe_param_memalloc_addr[id].gdc = (VPE_GDC_PARAM *)((UINT8 *)vpe_param_memalloc_addr[id].dce_ctl + ALIGN_CEIL(sizeof(VPE_DCE_CTL_PARAM), align_byte));
		vpe_param_memalloc_addr[id].lut2d = (VPE_2DLUT_PARAM *)((UINT8 *)vpe_param_memalloc_addr[id].gdc + ALIGN_CEIL(sizeof(VPE_GDC_PARAM), align_byte));
		vpe_param_memalloc_addr[id].drt = (VPE_DRT_PARAM *)((UINT8 *)vpe_param_memalloc_addr[id].lut2d + ALIGN_CEIL(sizeof(VPE_2DLUT_PARAM), align_byte));
		vpe_param_memalloc_addr[id].dctg = (VPE_DCTG_PARAM *)((UINT8 *)vpe_param_memalloc_addr[id].drt + ALIGN_CEIL(sizeof(VPE_DRT_PARAM), align_byte));
		vpe_param_temp = (VPE_PARAM_PTR*)vpe_get_param_default();
		memcpy(vpe_param_memalloc_addr[id].sharpen, vpe_param_temp->sharpen, sizeof(VPE_SHARPEN_PARAM));
		memcpy(vpe_param_memalloc_addr[id].dce_ctl, vpe_param_temp->dce_ctl, sizeof(VPE_DCE_CTL_PARAM));
		memcpy(vpe_param_memalloc_addr[id].gdc, vpe_param_temp->gdc, sizeof(VPE_GDC_PARAM));
		memcpy(vpe_param_memalloc_addr[id].lut2d, vpe_param_temp->lut2d, sizeof(VPE_2DLUT_PARAM));
		memcpy(vpe_param_memalloc_addr[id].drt, vpe_param_temp->drt, sizeof(VPE_DRT_PARAM));
		memcpy(vpe_param_memalloc_addr[id].dctg, vpe_param_temp->dctg, sizeof(VPE_DCTG_PARAM));
		vpe_param[id] = &vpe_param_memalloc_addr[id];
		vpe_param_memalloc_valid[id] = TRUE;
	}
	return 0;
}

INT32 vpe_dev_construct(VPE_DEV_INFO *pdev_info)
{
	INT32 ret = 0;
	UINT32 buffer_num = 0, i;

	#if defined(__KERNEL__)
	// initialize synchronization mechanism
	sema_init(&pdev_info->api_mutex, 1);
	sema_init(&pdev_info->ioc_mutex, 1);
	sema_init(&pdev_info->proc_mutex, 1);
	#endif

	if (vpe_id_list == 0) {
		DBG_WRN("vpe_id_list = 0, modify to 1 \r\n");
		vpe_id_list = 1;
	}

	// clean VPE module
	memset(&vpe_module, 0x0, sizeof(VPE_MODULE));

	// calculate buffer needed
	for (i = 0; i < VPE_ID_MAX_NUM; i++) {
		if ((vpe_id_list >> i) & 0x1) {
			buffer_num++;
			vpe_param_id_valid[i] = TRUE;
			vpe_info_id_map[i] = vpe_info_id_map_index;
			vpe_info_id_map_index++;
		}
	}

	for (i = 0; i < VPE_ID_MAX_NUM; i++) {
		if ((vpe_id_list >> i) & 0x1) {
			vpe_dev_get_param_addr(i);
		}
	}

	// registed callback function
	// TODO: wait EXPORT_SYMBOL
	ctl_vpe_isp_evt_fp_reg(VPE_IF_REG_NAME, &vpe_api_cb_flow, (ISP_EVENT_VPE_CFG_IMM | ISP_EVENT_PARAM_RST), CTL_VPE_ISP_CB_MSG_NONE);

	return ret;
}

void vpe_dev_deconstruct(VPE_DEV_INFO *pdev_info)
{
	UINT32 i;

	for (i = 0; i < VPE_ID_MAX_NUM; i++) {
		if ((vpe_id_list >> i) & 0x1) {
			if (vpe_param_memalloc_valid[i] == TRUE) {
			#if defined(__FREERTOS)
			free(vpe_param_memalloc_addr[i].sharpen);
			#else
			kfree(vpe_param_memalloc_addr[i].sharpen);
			#endif
			vpe_param_memalloc_addr[i].sharpen = NULL;
			vpe_param_memalloc_addr[i].dce_ctl = NULL;
			vpe_param_memalloc_addr[i].gdc = NULL;
			vpe_param_memalloc_addr[i].lut2d = NULL;
			vpe_param_memalloc_addr[i].drt = NULL;
			vpe_param_memalloc_addr[i].dctg = NULL;
			vpe_param_memalloc_valid[i] = FALSE;
			}
		}
	}
	vpe_info_id_map_index = 0;

	// un-registed callback function
	// TODO: wait EXPORT_SYMBOL

	ctl_vpe_isp_evt_fp_unreg(VPE_IF_REG_NAME);
}

