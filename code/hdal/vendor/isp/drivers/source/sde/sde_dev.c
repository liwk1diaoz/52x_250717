#if defined(__FREERTOS)
#include <string.h>
#include <stdlib.h>
#include "sde_dbg.h"
#include "sde_dev_int.h"
#else
#include <linux/slab.h>

#include "sde_ioctl.h"
#include "sde_dbg.h"
#include "sde_main.h"
#endif

#include "kwrap/error_no.h"

#include "sde_alg.h"
#include "sde_api_int.h"
#include "sde_param_default.h"

//=============================================================================
// define
//=============================================================================
#define SDE_IF_REG_NAME             "NVT_SDE_IF"

//=============================================================================
// global variable
//=============================================================================
extern UINT32 sde_id_list;
#if defined(__FREERTOS)
static SDE_DEV_INFO pdev_info;
#endif

SDE_MODULE sde_module;

SDE_IF_PARAM_PTR *sde_if_param[SDE_ID_MAX_NUM] = {NULL};
SDE_IF_PARAM_PTR sde_param_memalloc_addr[SDE_ID_MAX_NUM] = {0};
static BOOL sde_param_memalloc_valid[SDE_ID_MAX_NUM] = {0};
static BOOL sde_param_id_valid[SDE_ID_MAX_NUM] = {0};
static UINT32 sde_info_id_map[SDE_ID_MAX_NUM] = {0};
static UINT32 sde_info_id_map_index;

//=============================================================================
// function declaration
//=============================================================================


//=============================================================================
// external functions
//=============================================================================
#if defined(__FREERTOS)
SDE_DEV_INFO *sde_get_dev_info(void)
{
	return &pdev_info;
}
#endif

//=============================================================================
// internal functions
//=============================================================================
static INT32 sde_dev_get_param_addr(SDE_ID id)
{
	UINT32 total_param_size;
	SDE_IF_PARAM_PTR *sde_if_param_temp = NULL;
	void *param_mem_addr = NULL;
	static BOOL use_param_phy_addr = TRUE;
	int align_byte = 4;

	if (use_param_phy_addr == TRUE) {
		use_param_phy_addr = FALSE;
		sde_if_param[id] = (SDE_IF_PARAM_PTR *)sde_get_param_default();
	} else {
		total_param_size = ALIGN_CEIL(sizeof(SDE_IF_IO_INFO), align_byte)+ ALIGN_CEIL(sizeof(SDE_IF_CTRL_PARAM), align_byte);
		#if defined(__FREERTOS)
		param_mem_addr = malloc(total_param_size);
		if (param_mem_addr == NULL) {
			DBG_ERR("fail to allocate sde parameter fail!\n");
			return -E_SYS;
		}
		#else
		param_mem_addr = kzalloc(total_param_size, GFP_KERNEL);
		if (param_mem_addr == NULL) {
			DBG_ERR("fail to allocate sde parameter fail!\n");
			return -ENOMEM;
		}
		#endif
		sde_param_memalloc_addr[id].io_info = (SDE_IF_IO_INFO *)param_mem_addr;
		sde_param_memalloc_addr[id].ctrl = (SDE_IF_CTRL_PARAM *)((UINT8 *)sde_param_memalloc_addr[id].io_info + ALIGN_CEIL(sizeof(SDE_IF_IO_INFO), align_byte));
		sde_if_param_temp = (SDE_IF_PARAM_PTR *)sde_get_param_default();
		memcpy(sde_param_memalloc_addr[id].io_info, sde_if_param_temp->io_info, sizeof(SDE_IF_IO_INFO));
		memcpy(sde_param_memalloc_addr[id].ctrl, sde_if_param_temp->ctrl, sizeof(SDE_IF_CTRL_PARAM));
		sde_if_param[id] = &sde_param_memalloc_addr[id];
		sde_param_memalloc_valid[id] = TRUE;
	}
	return 0;
}

INT32 sde_dev_construct(SDE_DEV_INFO *pdev_info)
{
	INT32 ret = 0;
	UINT32 buffer_num = 0, i;

	#if defined(__KERNEL__)
	// initialize synchronization mechanism
	sema_init(&pdev_info->api_mutex, 1);
	sema_init(&pdev_info->ioc_mutex, 1);
	sema_init(&pdev_info->proc_mutex, 1);
	#endif

	if (sde_id_list == 0) {
		DBG_WRN("sde_id_list = 0, modify to 1 \r\n");
		sde_id_list = 1;
	}

	// clean SDE module
	memset(&sde_module, 0x0, sizeof(SDE_MODULE));

	// calculate buffer needed
	for (i = 0; i < SDE_ID_MAX_NUM; i++) {
		if ((sde_id_list >> i) & 0x1) {
			buffer_num++;
			sde_param_id_valid[i] = TRUE;
			sde_info_id_map[i] = sde_info_id_map_index;
			sde_info_id_map_index++;
		}
	}

	for (i = 0; i < SDE_ID_MAX_NUM; i++) {
		if ((sde_id_list >> i) & 0x1) {
			sde_dev_get_param_addr(i);
		}
	}

	// registed callback function
	// TODO: wait EXPORT_SYMBOL
	//ctl_sde_isp_evt_fp_reg(SDE_IF_REG_NAME, &sde_api_cb_flow, ISP_EVENT_SDE_CFG_IMM, CTL_SDE_ISP_CB_MSG_NONE);

	return ret;
}

void sde_dev_deconstruct(SDE_DEV_INFO *pdev_info)
{
	UINT32 i;

	for (i = 0; i < SDE_ID_MAX_NUM; i++) {
		if ((sde_id_list >> i) & 0x1) {
			if (sde_param_memalloc_valid[i] == TRUE) {
			#if defined(__FREERTOS)
			free(sde_param_memalloc_addr[i].io_info);
			#else
			kfree(sde_param_memalloc_addr[i].io_info);
			#endif
			sde_param_memalloc_addr[i].io_info = NULL;
				sde_param_memalloc_valid[i] = FALSE;
			}
		}
	}
	sde_info_id_map_index = 0;

	// un-registed callback function
	// TODO: wait EXPORT_SYMBOL
	//ctl_sde_isp_evt_fp_unreg(SDE_IF_REG_NAME);
}

BOOL sde_dev_get_id_valid(UINT32 id)
{
	return sde_param_id_valid[id];
}

