
#if defined (__KERNEL__)
//#include <linux/version.h>
//#include <linux/module.h>
//#include <linux/seq_file.h>
//#include <linux/proc_fs.h>
//#include <linux/bitops.h>
//#include <linux/interrupt.h>
//#include <linux/mm.h>
//#include <linux/of.h>
//#include <linux/clk.h>
//#include <linux/fb.h>
//#include <linux/uaccess.h>
//#include <linux/sched.h>
//#include <linux/platform_device.h>
//#include <linux/miscdevice.h>
//#include <asm/uaccess.h>
//#include <asm/atomic.h>
//#include <asm/io.h>

#include <plat/efuse_protected.h>

#include "comm/nvtmem.h"
#include "kwrap/type.h"

#elif defined(__FREERTOS)

#include "kwrap/type.h"
#include "efuse_protected.h"

#else

#endif


#include "kdrv_videoprocess/kdrv_vpe.h"
#include "kdrv_vpe_int.h"

#include "kdrv_videoprocess/kdrv_ipp_config.h"
#include "kdrv_videoprocess/kdrv_ipp_utility.h"

#include "vpe_tasklet.h"


#define VPE_PA_START   0xFD800000
#define VPE_PA_END     0xFD80ffff
#define VPE_0_IRQ      127



VPE_DRV_INT_INFO     g_vpe_drv_data;
VPE_DRV_CFG      g_vpe_drv_cfg;
VPE_LLCMD_POOL   g_vpe_llcmd_pool;
VPE_ALL_JOB_LIST g_vpe_drv_all_job;


unsigned int vpe_platform = 0;



#define VPE_SPIN_LOCK_IRQSAVE(x,flags)               vk_spin_lock_irqsave(&(x),flags)
#define VPE_SPIN_UNLOCK_IRQRESTORE(x,flags)          vk_spin_unlock_irqrestore(&(x),flags)

struct clk *vpeclk;



struct proc_dir_entry *vpe_drv_root;
struct proc_dir_entry *vpe_drv_addr;
struct proc_dir_entry *vpe_drv_version;
struct proc_dir_entry *vpe_drv_gating_en;


VPE_SW_CONFIG     g_vpe_sw_config;

//--------------------------------------------------------------------

/**
    VPE KDRV handle structure
*/
#define KDRV_VPE_HANDLE_LOCK    0x00000001

typedef struct _KDRV_VPE_HANDLE {
	UINT32 entry_id;
	UINT32 flag_id;
	UINT32 lock_bit;
	SEM_HANDLE *sem_id;
	UINT32 sts;
	KDRV_IPP_ISRCB isrcb_fp;
} KDRV_VPE_HANDLE;


KDRV_VPE_OPENCFG g_kdrv_vpe_clk_info[KDRV_VPE_ID_MAX_NUM] = {0};
BOOL g_kdrv_vpe_set_clk_flg[KDRV_VPE_ID_MAX_NUM] = {
	FALSE
};

UINT32 kdrv_vpe_channel_num = 4;
static UINT32 g_kdrv_vpe_init_flg = 0;
static KDRV_VPE_HANDLE g_kdrv_vpe_handle_tab[4];
UINT32 kdrv_vpe_lock_chls = 0;
static KDRV_VPE_HANDLE *g_kdrv_vpe_trig_hdl = NULL;



static KDRV_VPE_HANDLE *kdrv_vpe_entry_id_conv2_handle(UINT32 entry_id)
{
	return  &g_kdrv_vpe_handle_tab[entry_id];
}

static void kdrv_vpe_sem(KDRV_VPE_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		SEM_WAIT(*p_handle->sem_id);    // wait semaphore
	} else {
		//SEM_SIGNAL(*p_handle->sem_id);  // wait semaphore
		SEM_SIGNAL_ISR(*p_handle->sem_id);  // wait semaphore
	}
}


static KDRV_VPE_HANDLE *kdrv_vpe_check_handle(KDRV_VPE_HANDLE *p_handle)
{
	UINT32 i;

	for (i = 0; i < kdrv_vpe_channel_num; i ++) {
		if (p_handle == &g_kdrv_vpe_handle_tab[i]) {
			return p_handle;
		}
	}

	return NULL;
}


static void kdrv_vpe_handle_lock(void)
{
	FLGPTN wait_flg;
	wai_flg(&wait_flg, kdrv_vpe_get_flag_id(FLG_ID_KDRV_VPE), KDRV_IPP_VPE_HDL_UNLOCK, (TWF_ORW | TWF_CLR));
}

static void kdrv_vpe_handle_unlock(void)
{
	set_flg(kdrv_vpe_get_flag_id(FLG_ID_KDRV_VPE), KDRV_IPP_VPE_HDL_UNLOCK);
}

static BOOL kdrv_vpe_channel_alloc(UINT32 chl_id)
{
	UINT32 i;
	//KDRV_VPE_HANDLE *p_handle;
	BOOL eng_init_flag = 0;


	//p_handle = NULL;
	eng_init_flag = FALSE;
	i = chl_id;
	if (!(g_kdrv_vpe_init_flg & (1 << i))) {
		//
		kdrv_vpe_handle_lock();

		if (g_kdrv_vpe_init_flg == 0) {
			eng_init_flag = TRUE;
		}

		g_kdrv_vpe_init_flg |= (1 << i);

		g_kdrv_vpe_handle_tab[i].entry_id = i;
		g_kdrv_vpe_handle_tab[i].flag_id = kdrv_vpe_get_flag_id(FLG_ID_KDRV_VPE);
		g_kdrv_vpe_handle_tab[i].lock_bit = (1 << i);
		g_kdrv_vpe_handle_tab[i].sts |= KDRV_VPE_HANDLE_LOCK;
		g_kdrv_vpe_handle_tab[i].sem_id = kdrv_vpe_get_sem_id(SEMID_KDRV_VPE);
		//p_handle = &g_kdrv_vpe_handle_tab[i];



		kdrv_vpe_handle_unlock();
	}


	//if (i >= KDRV_VPE_HANDLE_MAX_NUM) {
	//  DBG_ERR("get free handle fail(0x%.8x)\r\r\n", (unsigned int)g_kdrv_vpe_init_flg);
	//}

	return eng_init_flag;
}


//-------------------------------------------------------------------
// vpe settting function

static ER kdrv_vpe_set_isrcb(UINT32 id, void *data)
{
	KDRV_VPE_HANDLE *p_handle;

	if (data == NULL) {
		DBG_ERR("id %d input param error 0x%.8x\r\n", (int)id, (unsigned int)data);
		return E_PAR;
	}

	p_handle = g_kdrv_vpe_trig_hdl;
	if (p_handle == NULL) {
		return E_OK;
	}

	p_handle->isrcb_fp = (KDRV_IPP_ISRCB)data;

	return E_OK;
}


static ER kdrv_vpe_set_clk(UINT32 id, VOID *data)
{
	//UINT32 i;
	KDRV_VPE_OPENCFG *get_clk = NULL;

	if (data == NULL) {
		DBG_WRN("KDRV VPE: set clock parameter NULL...\r\n");
		return E_OK;
	}

	get_clk = (KDRV_VPE_OPENCFG *)data;

	g_kdrv_vpe_clk_info[id].vpe_clock_sel = get_clk->vpe_clock_sel;

	//for (i = 0; i < kdrv_vpe_channel_num; i++) {
	//  g_kdrv_vpe_clk_info[id].vpe_clock_sel = get_clk->vpe_clock_sel;
	//}

	g_kdrv_vpe_set_clk_flg[id] = TRUE;

	//g_vpe_update_param[id][KDRV_VPE_PARAM_IPL_OPENCFG] = TRUE;

	return E_OK;
}

static ER kdrv_vpe_set_reset(UINT32 id, void *data)
{
	KDRV_VPE_HANDLE *p_handle;

	if (vpe_reset() != E_OK) {
		DBG_ERR("KDRV VPE: engine reset error...\r\n");
		return E_PAR;
	}

	p_handle = kdrv_vpe_entry_id_conv2_handle(id);
	kdrv_vpe_sem(p_handle, FALSE);

	return E_OK;
}



KDRV_VPE_SET_FP kdrv_vpe_set_fp[KDRV_VPE_PARAM_MAX] = {
	kdrv_vpe_set_clk,
	kdrv_vpe_set_isrcb,
	kdrv_vpe_set_reset,
};

//-------------------------------------------------------------------

KDRV_VPE_GET_FP kdrv_vpe_get_fp[KDRV_VPE_PARAM_MAX] = {
	NULL,
	NULL,
	NULL,
};


//--------------------------------------------------------------------



int vpe_drv_get_align(KDRV_VPE_CONFIG *p_vpe_config, UINT32 type, UINT8 resNum, UINT8 dupNum)
{
	INT32 ret = 0;
	UINT32 align_src_width = 0, align_src_height = 0, align_proc_x_start = 0, align_proc_y_start = 0, align_proc_width = 0, align_proc_height = 0;
	UINT32 align_des_width = 0, align_des_height = 0;
	UINT32 align_out_width = 0, align_out_height = 0, align_out_x_start = 0, align_out_y_start = 0;
	UINT32 align_rlt_width = 0, align_rlt_height = 0, align_rlt_x_start = 0, align_rlt_y_start = 0;
	INT32 align_pip_width = 0, align_pip_height = 0, align_pip_x_start = 0, align_pip_y_start = 0;

	UINT32 align_sca_width = 0, align_sca_height = 0;
	UINT32 align_sca_crop_width = 0, align_sca_crop_height = 0, align_sca_crop_x_start = 0, align_sca_crop_y_start = 0;
	UINT32 align_tc_crop_width = 0, align_tc_crop_height = 0, align_tc_crop_x_start = 0, align_tc_crop_y_start = 0;



	align_src_width = ALIGN_16;
	align_src_height = ALIGN_2;
	align_proc_x_start = ALIGN_2;
	align_proc_y_start = ALIGN_2;
	align_proc_width = ALIGN_8;
	align_proc_height = ALIGN_2;

	if (p_vpe_config->out_img_info[resNum].out_dup_info.des_type == KDRV_VPE_DES_TYPE_YUV420) {
		align_des_width = ALIGN_16;
		align_des_height = ALIGN_2;

		align_out_x_start = ALIGN_4;
		align_out_y_start = ALIGN_2;
		align_out_width = ALIGN_4;
		align_out_height = ALIGN_2;

		align_rlt_x_start = ALIGN_4;
		align_rlt_y_start = ALIGN_2;
		align_rlt_width = ALIGN_4;
		align_rlt_height = ALIGN_2;

		align_pip_x_start = ALIGN_4;
		align_pip_y_start = ALIGN_2;
		align_pip_width = ALIGN_4;
		align_pip_height = ALIGN_2;

		align_sca_width = ALIGN_8;
		align_sca_height = ALIGN_2;

		align_sca_crop_width   = ALIGN_8;
		align_sca_crop_height  = ALIGN_2;
		align_sca_crop_x_start = ALIGN_2;
		align_sca_crop_y_start = ALIGN_2;

		align_tc_crop_width   = ALIGN_4;
		align_tc_crop_height  = ALIGN_2;
		align_tc_crop_x_start = ALIGN_4;
		align_tc_crop_y_start = ALIGN_2;
	} else if (p_vpe_config->out_img_info[resNum].out_dup_info.des_type == KDRV_VPE_DES_TYPE_YCC_YUV420) {
		align_des_width = ALIGN_32;
		align_des_height = ALIGN_2;

		align_out_x_start = ALIGN_32;
		align_out_y_start = ALIGN_2;
		align_out_width = ALIGN_32;
		align_out_height = ALIGN_2;

		align_rlt_x_start = ALIGN_32;
		align_rlt_y_start = ALIGN_2;
		align_rlt_width = ALIGN_32;
		align_rlt_height = ALIGN_2;

		align_pip_x_start = ALIGN_32;
		align_pip_y_start = ALIGN_2;
		align_pip_width = ALIGN_32;
		align_pip_height = ALIGN_2;

		align_sca_width = ALIGN_32;
		align_sca_height = ALIGN_2;

		align_sca_crop_width   = ALIGN_8;
		align_sca_crop_height  = ALIGN_2;
		align_sca_crop_x_start = ALIGN_2;
		align_sca_crop_y_start = ALIGN_2;

		align_tc_crop_width   = ALIGN_32;
		align_tc_crop_height  = ALIGN_2;
		align_tc_crop_x_start = ALIGN_32;
		align_tc_crop_y_start = ALIGN_2;
	} else {
		DBG_ERR("vpe: error destination type\r\n");
	}

	switch (type) {
	case ALIGM_TYPE_SRC_W:
		ret = align_src_width;
		break;
	case ALIGM_TYPE_SRC_H:
		ret = align_src_height;
		break;
	case ALIGM_TYPE_PROC_X:
		ret = align_proc_x_start;
		break;
	case ALIGM_TYPE_PROC_Y:
		ret = align_proc_y_start;
		break;
	case ALIGM_TYPE_PROC_W:
		ret = align_proc_width;
		break;
	case ALIGM_TYPE_PROC_H:
		ret = align_proc_height;
		break;

	case ALIGM_TYPE_DES_W:
		ret = align_des_width;
		break;
	case ALIGM_TYPE_DES_H:
		ret = align_des_height;
		break;
	case ALIGM_TYPE_OUT_W:
		ret = align_out_width;
		break;
	case ALIGM_TYPE_OUT_H:
		ret = align_out_height;
		break;
	case ALIGM_TYPE_OUT_X:
		ret = align_out_x_start;
		break;
	case ALIGM_TYPE_OUT_Y:
		ret = align_out_y_start;
		break;
	case ALIGM_TYPE_RLT_W:
		ret = align_rlt_width;
		break;
	case ALIGM_TYPE_RLT_H:
		ret = align_rlt_height;
		break;
	case ALIGM_TYPE_RLT_X:
		ret = align_rlt_x_start;
		break;
	case ALIGM_TYPE_RLT_Y:
		ret = align_rlt_y_start;
		break;
	case ALIGM_TYPE_PIP_W:
		ret = align_pip_width;
		break;
	case ALIGM_TYPE_PIP_H:
		ret = align_pip_height;
		break;
	case ALIGM_TYPE_PIP_X:
		ret = align_pip_x_start;
		break;
	case ALIGM_TYPE_PIP_Y:
		ret = align_pip_y_start;
		break;
	case ALIGM_TYPE_SCA_W:
		ret = align_sca_width;
		break;
	case ALIGM_TYPE_SCA_H:
		ret = align_sca_height;
		break;

	case ALIGM_TYPE_SCA_CROP_X:
		ret = align_sca_crop_x_start;
		break;

	case ALIGM_TYPE_SCA_CROP_Y:
		ret = align_sca_crop_y_start;
		break;

	case ALIGM_TYPE_SCA_CROP_W:
		ret = align_sca_crop_width;
		break;

	case ALIGM_TYPE_SCA_CROP_H:
		ret = align_sca_crop_height;
		break;


	case ALIGM_TYPE_TC_CROP_X:
		ret = align_tc_crop_x_start;
		break;

	case ALIGM_TYPE_TC_CROP_Y:
		ret = align_tc_crop_y_start;
		break;

	case ALIGM_TYPE_TC_CROP_W:
		ret = align_tc_crop_width;
		break;

	case ALIGM_TYPE_TC_CROP_H:
		ret = align_tc_crop_height;
		break;
	default:
		ret = 0;
	}
	return ret;
}

static UINT32 vpe_hw_get_align(VPE_DRV_CFG *p_vpe_config, UINT32 type, UINT8 resNum, UINT8 dupNum)
{
	UINT32 ret = 0;
	UINT32 align_src_width = 0, align_src_height = 0, align_proc_x_start = 0, align_proc_y_start = 0, align_proc_width = 0, align_proc_height = 0;
	UINT32 align_des_width = 0, align_des_height = 0;
	UINT32 align_out_width = 0, align_out_height = 0, align_out_x_start = 0, align_out_y_start = 0;
	UINT32 align_rlt_width = 0, align_rlt_height = 0, align_rlt_x_start = 0, align_rlt_y_start = 0;
	UINT32 align_pip_width = 0, align_pip_height = 0, align_pip_x_start = 0, align_pip_y_start = 0;
	UINT32 dest_type = KDRV_VPE_DES_TYPE_YUV420;

	UINT32 align_sca_width = 0, align_sca_height = 0;
	UINT32 align_sca_crop_width = 0, align_sca_crop_height = 0, align_sca_crop_x_start = 0, align_sca_crop_y_start = 0;
	UINT32 align_tc_crop_width = 0, align_tc_crop_height = 0, align_tc_crop_x_start = 0, align_tc_crop_y_start = 0;

	align_src_width = ALIGN_16;
	align_src_height = ALIGN_2;

	align_proc_x_start = ALIGN_2;
	align_proc_y_start = ALIGN_2;
	align_proc_width = ALIGN_8;
	align_proc_height = ALIGN_2;


	if (p_vpe_config->res[resNum].res_ctl.des_yuv420) {
		//420
		if (p_vpe_config->res[resNum].res_ctl.des_dp0_ycc_enc_en) {
			dest_type = KDRV_VPE_DES_TYPE_YCC_YUV420;
		} else {
			dest_type = KDRV_VPE_DES_TYPE_YUV420;
		}
	} else {
		//422

	}

	if (dest_type == KDRV_VPE_DES_TYPE_YUV420) {
		align_des_width = ALIGN_16;
		align_des_height = ALIGN_2;

		align_out_x_start = ALIGN_4;
		align_out_y_start = ALIGN_2;
		align_out_width = ALIGN_4;
		align_out_height = ALIGN_2;

		align_rlt_x_start = ALIGN_4;
		align_rlt_y_start = ALIGN_2;
		align_rlt_width = ALIGN_4;
		align_rlt_height = ALIGN_2;

		align_pip_x_start = ALIGN_4;
		align_pip_y_start = ALIGN_2;
		align_pip_width = ALIGN_4;
		align_pip_height = ALIGN_2;

		align_sca_width = ALIGN_8;
		align_sca_height = ALIGN_2;

		align_sca_crop_width   = ALIGN_8;
		align_sca_crop_height  = ALIGN_2;
		align_sca_crop_x_start = ALIGN_2;
		align_sca_crop_y_start = ALIGN_2;

		align_tc_crop_width   = ALIGN_4;
		align_tc_crop_height  = ALIGN_2;
		align_tc_crop_x_start = ALIGN_4;
		align_tc_crop_y_start = ALIGN_2;
	} else if (dest_type == KDRV_VPE_DES_TYPE_YCC_YUV420) {
		align_des_width = ALIGN_32;
		align_des_height = ALIGN_2;

		align_out_x_start = ALIGN_32;
		align_out_y_start = ALIGN_2;
		align_out_width = ALIGN_32;
		align_out_height = ALIGN_2;

		align_rlt_x_start = ALIGN_32;
		align_rlt_y_start = ALIGN_2;
		align_rlt_width = ALIGN_32;
		align_rlt_height = ALIGN_2;

		align_pip_x_start = ALIGN_32;
		align_pip_y_start = ALIGN_2;
		align_pip_width = ALIGN_32;
		align_pip_height = ALIGN_2;

		align_sca_width = ALIGN_32;
		align_sca_height = ALIGN_2;

		align_sca_crop_width   = ALIGN_8;
		align_sca_crop_height  = ALIGN_2;
		align_sca_crop_x_start = ALIGN_2;
		align_sca_crop_y_start = ALIGN_2;

		align_tc_crop_width   = ALIGN_32;
		align_tc_crop_height  = ALIGN_2;
		align_tc_crop_x_start = ALIGN_32;
		align_tc_crop_y_start = ALIGN_2;
	} else {
		DBG_ERR("vpe: error destanation type\r\n");
	}

	switch (type) {
	case ALIGM_TYPE_SRC_W:
		ret = align_src_width;
		break;
	case ALIGM_TYPE_SRC_H:
		ret = align_src_height;
		break;
	case ALIGM_TYPE_PROC_X:
		ret = align_proc_x_start;
		break;
	case ALIGM_TYPE_PROC_Y:
		ret = align_proc_y_start;
		break;
	case ALIGM_TYPE_PROC_W:
		ret = align_proc_width;
		break;
	case ALIGM_TYPE_PROC_H:
		ret = align_proc_height;
		break;

	case ALIGM_TYPE_DES_W:
		ret = align_des_width;
		break;
	case ALIGM_TYPE_DES_H:
		ret = align_des_height;
		break;
	case ALIGM_TYPE_OUT_W:
		ret = align_out_width;
		break;
	case ALIGM_TYPE_OUT_H:
		ret = align_out_height;
		break;
	case ALIGM_TYPE_OUT_X:
		ret = align_out_x_start;
		break;
	case ALIGM_TYPE_OUT_Y:
		ret = align_out_y_start;
		break;
	case ALIGM_TYPE_RLT_W:
		ret = align_rlt_width;
		break;
	case ALIGM_TYPE_RLT_H:
		ret = align_rlt_height;
		break;
	case ALIGM_TYPE_RLT_X:
		ret = align_rlt_x_start;
		break;
	case ALIGM_TYPE_RLT_Y:
		ret = align_rlt_y_start;
		break;
	case ALIGM_TYPE_PIP_W:
		ret = align_pip_width;
		break;
	case ALIGM_TYPE_PIP_H:
		ret = align_pip_height;
		break;
	case ALIGM_TYPE_PIP_X:
		ret = align_pip_x_start;
		break;
	case ALIGM_TYPE_PIP_Y:
		ret = align_pip_y_start;
		break;
	case ALIGM_TYPE_SCA_W:
		ret = align_sca_width;
		break;
	case ALIGM_TYPE_SCA_H:
		ret = align_sca_height;
		break;

	case ALIGM_TYPE_SCA_CROP_X:
		ret = align_sca_crop_x_start;
		break;

	case ALIGM_TYPE_SCA_CROP_Y:
		ret = align_sca_crop_y_start;
		break;

	case ALIGM_TYPE_SCA_CROP_W:
		ret = align_sca_crop_width;
		break;

	case ALIGM_TYPE_SCA_CROP_H:
		ret = align_sca_crop_height;
		break;


	case ALIGM_TYPE_TC_CROP_X:
		ret = align_tc_crop_x_start;
		break;

	case ALIGM_TYPE_TC_CROP_Y:
		ret = align_tc_crop_y_start;
		break;

	case ALIGM_TYPE_TC_CROP_W:
		ret = align_tc_crop_width;
		break;

	case ALIGM_TYPE_TC_CROP_H:
		ret = align_tc_crop_height;
		break;

	default:
		ret = 0;
	}
	return ret;
}



#if 1
static INT32 check_value(UINT32 type, INT32 value)
{
	UINT32 mask = (type - 1);
	UINT32 get_val = (UINT32)value;

	//if (get_val & mask) {
	//  ret = -1;
	//}

	return (INT32)(get_val & mask);
}

int vpe_drv_check_align(KDRV_VPE_CONFIG *p_vpe_config)
{
	INT32 ret = 0;
	INT32 i = 0, j = 0;
	UINT32 align_src_width = 0, align_src_height = 0, align_proc_x_start = 0, align_proc_y_start = 0, align_proc_width = 0, align_proc_height = 0;
	UINT32 align_des_width = 0, align_des_height = 0;
	UINT32 align_out_width = 0, align_out_height = 0, align_out_x_start = 0, align_out_y_start = 0;
	INT32 align_pip_width = 0, align_pip_height = 0, align_pip_x_start = 0, align_pip_y_start = 0;
	INT32 align_temp = 0;
	INT32 total_frm = 0;

	UINT32 align_width = 0, align_height = 0, align_x_start = 0, align_y_start = 0;
	UINT16 dewarp_out_w = 0, dewarp_out_h = 0;
	align_src_width = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_SRC_W, 0, 0);
	align_src_height = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_SRC_H, 0, 0);
	//if (!align_src_width || !align_src_height) {
	//	vpe_err("vpe drv: align_src_width/align_src_height(%d, %d) is invalid\r\n", align_src_width, align_src_height);
	//	ret = -1;
	//}

	if (!(p_vpe_config->glb_img_info.src_width) || !(p_vpe_config->glb_img_info.src_height) || check_value(align_src_width, p_vpe_config->glb_img_info.src_width) || check_value(align_src_height, p_vpe_config->glb_img_info.src_height)) {
		vpe_err("vpe drv: src_width/src_height(%d, %d) is invalid, should be %d, %d alignment\r\n", p_vpe_config->glb_img_info.src_width, p_vpe_config->glb_img_info.src_height, align_src_width, align_src_height);
		ret = -1;
	}

	if (p_vpe_config->col_info.col_num > 7) {
		vpe_err("vpe drv: col_num(%d) is invalid, should be 0~7\r\n", p_vpe_config->col_info.col_num);
		ret = -1;
	}

	align_proc_width = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_PROC_W, 0, 0);
	align_proc_height = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_PROC_H, 0, 0);
	align_proc_x_start = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_PROC_X, 0, 0);
	align_proc_y_start = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_PROC_Y, 0, 0);

	//if (!align_proc_width  || !align_proc_height || !align_proc_x_start || !align_proc_y_start) {
	//	vpe_err("vpe drv: align_proc_width(%d) align_proc_height(%d) align_proc_x_start(%d) align_proc_y_start(%d) are invalid\r\n", align_proc_width, align_proc_height, align_proc_x_start, align_proc_y_start);
	//	ret = -1;
	//}

	for (i = 0; i < p_vpe_config->col_info.col_num + 1; i++) {

		if(p_vpe_config->func_mode == KDRV_VPE_FUNC_DCE_EN){
			if ((p_vpe_config->col_info.col_size_info[i].proc_x_start + p_vpe_config->col_info.col_size_info[i].proc_width) >  p_vpe_config->dce_param.dewarp_out_width) {
				vpe_err("vpe drv: [Col%d] proc_x_start(%d) proc_width(%d) is invalid, should be < dewarp_out_w(%d)\r\n", i, p_vpe_config->col_info.col_size_info[i].proc_x_start, p_vpe_config->col_info.col_size_info[i].proc_width, p_vpe_config->dce_param.dewarp_out_width);
				ret = -1;
			}
		}else{
			if ((p_vpe_config->col_info.col_size_info[i].proc_x_start + p_vpe_config->col_info.col_size_info[i].proc_width) >  p_vpe_config->glb_img_info.src_width) {
				vpe_err("vpe drv: [Col%d] proc_x_start(%d) proc_width(%d) is invalid, should be < src_width(%d)\r\n", i, p_vpe_config->col_info.col_size_info[i].proc_x_start, p_vpe_config->col_info.col_size_info[i].proc_width, p_vpe_config->glb_img_info.src_width);
				ret = -1;
			}


		}
		if (check_value(align_proc_width, p_vpe_config->col_info.col_size_info[i].proc_width)) {
			vpe_err("vpe drv: proc_width[%d](%d) is invalid, should be %d alignment\r\n", i, p_vpe_config->col_info.col_size_info[i].proc_width, align_proc_width);
			ret = -1;
		}

		if (check_value(align_proc_x_start, p_vpe_config->col_info.col_size_info[i].proc_x_start)) {
			vpe_err("vpe drv: proc_x_start[%d](%d) is invalid, should be %d alignment\r\n", i, p_vpe_config->col_info.col_size_info[i].proc_x_start, align_proc_x_start);
			ret = -1;
		}


		if (check_value(align_proc_height, p_vpe_config->glb_img_info.src_in_h)) {
			vpe_err("vpe drv: src_in_h(%d) is invalid, should be %d alignment\r\n", p_vpe_config->glb_img_info.src_in_h, align_proc_height);
			ret = -1;
		}

		if (check_value(align_proc_y_start, p_vpe_config->glb_img_info.src_in_y)) {
			vpe_err("vpe drv: src_in_y(%d) is invalid, should be %d alignment\r\n", p_vpe_config->glb_img_info.src_in_y, align_proc_y_start);
			ret = -1;
		}

	}

	if(p_vpe_config->func_mode == KDRV_VPE_FUNC_DCE_EN){
		if (((p_vpe_config->glb_img_info.src_in_x + p_vpe_config->dce_param.dewarp_in_width) > p_vpe_config->glb_img_info.src_width) || ((p_vpe_config->glb_img_info.src_in_y + p_vpe_config->dce_param.dewarp_in_height) > p_vpe_config->glb_img_info.src_height)) {        
			vpe_err("vpe drv: src_in_x(%d)/src_in_y(%d)/dewarp_in_w(%d)/dewarp_in_h(%d) are invalid\r\n", p_vpe_config->glb_img_info.src_in_x, p_vpe_config->glb_img_info.src_in_y, p_vpe_config->dce_param.dewarp_in_width, p_vpe_config->dce_param.dewarp_in_height);

	        if ((p_vpe_config->glb_img_info.src_in_x + p_vpe_config->dce_param.dewarp_in_width) > p_vpe_config->glb_img_info.src_width) {
	    		vpe_err("vpe drv: src_in_x(%d)+dewarp_in_w(%d) > src_width(%d), invalid\r\n", p_vpe_config->glb_img_info.src_in_x, p_vpe_config->dce_param.dewarp_in_width, p_vpe_config->glb_img_info.src_width);
			}

			if ((p_vpe_config->glb_img_info.src_in_y + p_vpe_config->dce_param.dewarp_in_height) > p_vpe_config->glb_img_info.src_height) {
	            vpe_err("vpe drv: src_in_y(%d)+dewarp_in_h(%d) > src_height(%d), invalid\r\n", p_vpe_config->glb_img_info.src_in_y, p_vpe_config->dce_param.dewarp_in_height, p_vpe_config->glb_img_info.src_height);
	        }
			ret = -1;
		}
	}else{

		if (((p_vpe_config->glb_img_info.src_in_x + p_vpe_config->glb_img_info.src_in_w) > p_vpe_config->glb_img_info.src_width) || ((p_vpe_config->glb_img_info.src_in_y + p_vpe_config->glb_img_info.src_in_h) > p_vpe_config->glb_img_info.src_height)) {		
			vpe_err("vpe drv: src_in_x(%d)/src_in_y(%d)/src_in_w(%d)/src_in_h(%d) are invalid\r\n", p_vpe_config->glb_img_info.src_in_x, p_vpe_config->glb_img_info.src_in_y, p_vpe_config->glb_img_info.src_in_w, p_vpe_config->glb_img_info.src_in_h);
		
			if ((p_vpe_config->glb_img_info.src_in_x + p_vpe_config->glb_img_info.src_in_w) > p_vpe_config->glb_img_info.src_width) {
				vpe_err("vpe drv: src_in_x(%d)+src_in_w(%d) > src_width(%d), invalid\r\n", p_vpe_config->glb_img_info.src_in_x, p_vpe_config->glb_img_info.src_in_w, p_vpe_config->glb_img_info.src_width);
			}
		
			if ((p_vpe_config->glb_img_info.src_in_y + p_vpe_config->glb_img_info.src_in_h) > p_vpe_config->glb_img_info.src_height) {
				vpe_err("vpe drv: src_in_y(%d)+src_in_h(%d) > src_height(%d), invalid\r\n", p_vpe_config->glb_img_info.src_in_y, p_vpe_config->glb_img_info.src_in_h, p_vpe_config->glb_img_info.src_height);
			}
			
			ret = -1;
		}
	}
	if (!(p_vpe_config->glb_img_info.src_addr)) {
		vpe_err("vpe drv: src_addr(0x0) is invalid\r\n");
		ret = -1;
	}

	for (i = 0; i < KDRV_VPE_MAX_IMG; i++) {
		if (p_vpe_config->out_img_info[i].out_dup_num) {
			total_frm = total_frm + p_vpe_config->out_img_info[i].out_dup_num;
			if (p_vpe_config->out_img_info[i].out_dup_num > VPE_RES_OUT_DUP_NUM) {
				vpe_err("vpe drv: Res[%d] des_type(%d), out_dup_num(%d) should be == %d\r\n", i, p_vpe_config->out_img_info[i].out_dup_info.des_type, p_vpe_config->out_img_info[i].out_dup_num, VPE_RES_OUT_DUP_NUM);
				ret = -1;
			}
		}
#if VPE_RES_LIMIT
		else {
			break;
		}
#else
		else {
			continue;
		}
#endif
		if (!(p_vpe_config->out_img_info[i].res_dim_info.des_width) || !(p_vpe_config->out_img_info[i].res_dim_info.des_height) || !(p_vpe_config->out_img_info[i].res_dim_info.des_out_w) || !(p_vpe_config->out_img_info[i].res_dim_info.des_out_h) ||
			((p_vpe_config->out_img_info[i].res_dim_info.des_out_x + p_vpe_config->out_img_info[i].res_dim_info.des_out_w) > p_vpe_config->out_img_info[i].res_dim_info.des_width) ||
			((p_vpe_config->out_img_info[i].res_dim_info.des_out_y + p_vpe_config->out_img_info[i].res_dim_info.des_out_h) > p_vpe_config->out_img_info[i].res_dim_info.des_height)) {
			vpe_err("vpe drv: out_img_info[%d] des_width(%d)/des_height(%d)/des_out_x(%d)/des_out_y(%d)/des_out_w(%d)/des_out_h(%d) are invalid\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.des_width, p_vpe_config->out_img_info[i].res_dim_info.des_height,
					p_vpe_config->out_img_info[i].res_dim_info.des_out_x, p_vpe_config->out_img_info[i].res_dim_info.des_out_y, p_vpe_config->out_img_info[i].res_dim_info.des_out_w, p_vpe_config->out_img_info[i].res_dim_info.des_out_h);

            if ((p_vpe_config->out_img_info[i].res_dim_info.des_out_x + p_vpe_config->out_img_info[i].res_dim_info.des_out_w) > p_vpe_config->out_img_info[i].res_dim_info.des_width) {
                vpe_err("vpe drv: out_img_info[%d] des_out_x(%d)+des_out_w(%d) > des_width(%d), invalid\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.des_out_x, p_vpe_config->out_img_info[i].res_dim_info.des_out_w, p_vpe_config->out_img_info[i].res_dim_info.des_width);
            }

            if ((p_vpe_config->out_img_info[i].res_dim_info.des_out_y + p_vpe_config->out_img_info[i].res_dim_info.des_out_h) > p_vpe_config->out_img_info[i].res_dim_info.des_height) {
                vpe_err("vpe drv: out_img_info[%d] des_out_y(%d)+des_out_h(%d) > des_height(%d), invalid\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.des_out_y, p_vpe_config->out_img_info[i].res_dim_info.des_out_h, p_vpe_config->out_img_info[i].res_dim_info.des_height);                
            }
			ret = -1;
		}

		if (!(p_vpe_config->out_img_info[i].res_dim_info.des_rlt_w) || !(p_vpe_config->out_img_info[i].res_dim_info.des_rlt_h) ||
			((p_vpe_config->out_img_info[i].res_dim_info.des_rlt_x + p_vpe_config->out_img_info[i].res_dim_info.des_rlt_w) > p_vpe_config->out_img_info[i].res_dim_info.des_out_w) ||
			((p_vpe_config->out_img_info[i].res_dim_info.des_rlt_y + p_vpe_config->out_img_info[i].res_dim_info.des_rlt_h) > p_vpe_config->out_img_info[i].res_dim_info.des_out_h)) {
			vpe_err("vpe drv: out_img_info[%d] des_out_w(%d)/des_out_h(%d)/des_rlt_x(%d)/des_rlt_y(%d)/des_rlt_w(%d)/des_rlt_h(%d) are invalid\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.des_out_w, p_vpe_config->out_img_info[i].res_dim_info.des_out_h,
					p_vpe_config->out_img_info[i].res_dim_info.des_rlt_x, p_vpe_config->out_img_info[i].res_dim_info.des_rlt_y, p_vpe_config->out_img_info[i].res_dim_info.des_rlt_w, p_vpe_config->out_img_info[i].res_dim_info.des_rlt_h);

            if ((p_vpe_config->out_img_info[i].res_dim_info.des_rlt_x + p_vpe_config->out_img_info[i].res_dim_info.des_rlt_w) > p_vpe_config->out_img_info[i].res_dim_info.des_out_w) {
                vpe_err("vpe drv: out_img_info[%d] des_rlt_x(%d)+des_rlt_w(%d) > des_out_w(%d), invalid\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.des_rlt_x, p_vpe_config->out_img_info[i].res_dim_info.des_rlt_w, p_vpe_config->out_img_info[i].res_dim_info.des_out_w);
            }

            if ((p_vpe_config->out_img_info[i].res_dim_info.des_rlt_y + p_vpe_config->out_img_info[i].res_dim_info.des_rlt_h) > p_vpe_config->out_img_info[i].res_dim_info.des_out_w) {
                vpe_err("vpe drv: out_img_info[%d] des_rlt_y(%d)+des_rlt_h(%d) > des_out_h(%d), invalid\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.des_rlt_y, p_vpe_config->out_img_info[i].res_dim_info.des_rlt_h, p_vpe_config->out_img_info[i].res_dim_info.des_out_h);
            }
			ret = -1;
		}

		for (j = 0; j < p_vpe_config->out_img_info[i].out_dup_num; j++) {
			if (!(p_vpe_config->out_img_info[i].out_dup_info.out_addr)) {
				vpe_err("vpe drv: Res[%d]Dup[%d] out_addr(0x%x) is invalid\r\n", i, j, p_vpe_config->out_img_info[i].out_dup_info.out_addr);
				ret = -1;
			}

			align_des_width = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_DES_W, i, j);
			align_des_height = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_DES_H, i, j);
			//if (!align_des_width || !align_des_height) {
			//	vpe_err("vpe drv: Res[%d] align_des_width(%d) align_out_height(%d) are invalid\r\n", i, align_des_width, align_des_height);
			//	ret = -1;
			//}
			if (check_value(align_des_width, p_vpe_config->out_img_info[i].res_dim_info.des_width) || check_value(align_des_height, p_vpe_config->out_img_info[i].res_dim_info.des_height)) {
				vpe_err("vpe drv: Res[%d]Dup[%d] des_width/des_height(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_config->out_img_info[i].res_dim_info.des_width, p_vpe_config->out_img_info[i].res_dim_info.des_height, align_des_width, align_des_height);
				ret = -1;
			}

			// pre scale crop size and position
			align_x_start = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_SCA_CROP_X, i, j);
			align_y_start = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_SCA_CROP_Y, i, j);
			if (check_value(align_x_start, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_x) || check_value(align_y_start,  p_vpe_config->out_img_info[i].res_dim_info.sca_crop_y)) {
				vpe_err("vpe drv: Res[%d]Dup[%d] sca_crop_x/sca_crop_y(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_x, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_y, align_x_start, align_y_start);
				ret = -1;
			}

			align_width   = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_SCA_CROP_W, i, j);
			align_height  = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_SCA_CROP_H, i, j);
			if (check_value(align_width, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_w) || check_value(align_height,  p_vpe_config->out_img_info[i].res_dim_info.sca_crop_h)) {
				vpe_err("vpe drv: Res[%d]Dup[%d] des_out_w/des_out_h(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_w, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_w, align_width, align_height);
				ret = -1;
			}
			if(p_vpe_config->func_mode == KDRV_VPE_FUNC_DCE_EN){
				dewarp_out_w = p_vpe_config->dce_param.dewarp_in_width;
				dewarp_out_h = p_vpe_config->dce_param.dewarp_in_height;

				if (((p_vpe_config->dce_param.dce_2d_lut_en == 1)&& (!(p_vpe_config->func_mode & KDRV_VPE_FUNC_DCTG_EN)))||((p_vpe_config->dce_param.dce_2d_lut_en == 0)&& (p_vpe_config->func_mode & KDRV_VPE_FUNC_DCTG_EN))){
					if ((p_vpe_config->dce_param.dewarp_out_width != 0) && (p_vpe_config->dce_param.dewarp_out_height != 0)) {
					    dewarp_out_w = p_vpe_config->dce_param.dewarp_out_width;
					    dewarp_out_h = p_vpe_config->dce_param.dewarp_out_height;
					}
				}

				if (!(dewarp_out_w) || !(dewarp_out_h) ||
				((p_vpe_config->out_img_info[i].res_dim_info.sca_crop_x + p_vpe_config->out_img_info[i].res_dim_info.sca_crop_w) > dewarp_out_w) ||
				((p_vpe_config->out_img_info[i].res_dim_info.sca_crop_y + p_vpe_config->out_img_info[i].res_dim_info.sca_crop_h) > dewarp_out_h)) {
					vpe_err("vpe drv: out_img_info[%d] dewarp_out_w(%d)/dewarp_out_h(%d)/sca_crop_x(%d)/sca_crop_y(%d)/sca_crop_w(%d)/sca_crop_h(%d) are invalid\r\n", i, dewarp_out_w, dewarp_out_h,
					p_vpe_config->out_img_info[i].res_dim_info.sca_crop_x, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_y, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_w, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_h);

					if ((p_vpe_config->out_img_info[i].res_dim_info.sca_crop_x + p_vpe_config->out_img_info[i].res_dim_info.sca_crop_w) > dewarp_out_w) {
	                    vpe_err("vpe drv: out_img_info[%d] sca_crop_x(%d)+sca_crop_w(%d) > dewarp_out_w(%d), invalid\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_x, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_w, dewarp_out_w);
					}

					if ((p_vpe_config->out_img_info[i].res_dim_info.sca_crop_y + p_vpe_config->out_img_info[i].res_dim_info.sca_crop_h) > dewarp_out_h) {
	                    vpe_err("vpe drv: out_img_info[%d] sca_crop_y(%d)+sca_crop_h(%d) > dewarp_out_h(%d), invalid\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_y, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_h, dewarp_out_h);
					}
					
					ret = -1;
				}

			}else{

				if (!(p_vpe_config->glb_img_info.src_in_w) || !(p_vpe_config->glb_img_info.src_in_h) ||
				((p_vpe_config->out_img_info[i].res_dim_info.sca_crop_x + p_vpe_config->out_img_info[i].res_dim_info.sca_crop_w) > p_vpe_config->glb_img_info.src_in_w) ||
				((p_vpe_config->out_img_info[i].res_dim_info.sca_crop_y + p_vpe_config->out_img_info[i].res_dim_info.sca_crop_h) > p_vpe_config->glb_img_info.src_in_h)) {
					vpe_err("vpe drv: out_img_info[%d] src_in_w(%d)/src_in_h(%d)/sca_crop_x(%d)/sca_crop_y(%d)/sca_crop_w(%d)/sca_crop_h(%d) are invalid\r\n", i, p_vpe_config->glb_img_info.src_in_w, p_vpe_config->glb_img_info.src_in_h,
					p_vpe_config->out_img_info[i].res_dim_info.sca_crop_x, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_y, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_w, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_h);
				
					if ((p_vpe_config->out_img_info[i].res_dim_info.sca_crop_x + p_vpe_config->out_img_info[i].res_dim_info.sca_crop_w) > p_vpe_config->glb_img_info.src_in_w) {
						vpe_err("vpe drv: out_img_info[%d] sca_crop_x(%d)+sca_crop_w(%d) > src_in_w(%d), invalid\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_x, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_w, p_vpe_config->glb_img_info.src_in_w);
					}
				
					if ((p_vpe_config->out_img_info[i].res_dim_info.sca_crop_y + p_vpe_config->out_img_info[i].res_dim_info.sca_crop_h) > p_vpe_config->glb_img_info.src_in_h) {
						vpe_err("vpe drv: out_img_info[%d] sca_crop_y(%d)+sca_crop_h(%d) > src_in_w(%d), invalid\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_y, p_vpe_config->out_img_info[i].res_dim_info.sca_crop_h, p_vpe_config->glb_img_info.src_in_h);
					}
					
					ret = -1;
				}
			}


			// scale size
			align_width  = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_SCA_W, i, j);
			align_height = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_SCA_H, i, j);
			if (check_value(align_width, p_vpe_config->out_img_info[i].res_dim_info.sca_width) || check_value(align_height,  p_vpe_config->out_img_info[i].res_dim_info.sca_height)) {
				vpe_err("vpe drv: Res[%d]Dup[%d] sca_width/sca_height(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_config->out_img_info[i].res_dim_info.sca_width, p_vpe_config->out_img_info[i].res_dim_info.sca_height, align_width, align_height);
				ret = -1;
			}


			// post scale crop size and position
			align_x_start = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_TC_CROP_X, i, j);
			align_y_start = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_TC_CROP_Y, i, j);
			if (check_value(align_x_start, p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_x) || check_value(align_y_start,  p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_y)) {
				vpe_err("vpe drv: Res[%d]Dup[%d] des_crop_out_x/des_crop_out_y(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_x, p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_y, align_x_start, align_y_start);
				ret = -1;
			}

			align_width   = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_TC_CROP_W, i, j);
			align_height  = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_TC_CROP_H, i, j);
			if (check_value(align_width, p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_w) || check_value(align_height,  p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_h)) {
				vpe_err("vpe drv: Res[%d]Dup[%d] des_crop_out_w/des_crop_out_h(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_w, p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_h, align_width, align_height);
				ret = -1;
			}

			if (!(p_vpe_config->out_img_info[i].res_dim_info.sca_width) || !(p_vpe_config->out_img_info[i].res_dim_info.sca_height) ||
			((p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_x + p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_w) > p_vpe_config->out_img_info[i].res_dim_info.sca_width) ||
			((p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_y + p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_h) > p_vpe_config->out_img_info[i].res_dim_info.sca_height)) {
				vpe_err("vpe drv: out_img_info[%d] sca_width(%d)/sca_height(%d)/des_crop_out_x(%d)/des_crop_out_y(%d)/des_crop_out_w(%d)/des_crop_out_h(%d) are invalid\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.sca_width, p_vpe_config->out_img_info[i].res_dim_info.sca_height,
				p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_x, p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_y, p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_w, p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_h);


				if ((p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_x + p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_w) > p_vpe_config->out_img_info[i].res_dim_info.sca_width) {
                    vpe_err("vpe drv: out_img_info[%d] des_crop_out_x(%d)+des_crop_out_w(%d) > sca_width(%d), invalid\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_x, p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_w, p_vpe_config->out_img_info[i].res_dim_info.sca_width);
				}

				if ((p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_y + p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_h) > p_vpe_config->out_img_info[i].res_dim_info.sca_height) {
                    vpe_err("vpe drv: out_img_info[%d] des_crop_out_y(%d)+des_crop_out_h(%d) > sca_height(%d), invalid\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_y, p_vpe_config->out_img_info[i].res_dim_info.des_crop_out_h, p_vpe_config->out_img_info[i].res_dim_info.sca_height);
				}
				ret = -1;
			}


			// output image size and position
			align_out_width = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_OUT_W, i, j);
			align_out_height = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_OUT_H, i, j);
			//if (!align_out_width || !align_out_height) {
			//	vpe_err("vpe drv: Res[%d]Dup[%d] align_out_width(%d) align_out_height(%d) are invalid\r\n", i, j, align_out_width, align_out_height);
			//	ret = -1;
			//}
			if (check_value(align_out_width, p_vpe_config->out_img_info[i].res_dim_info.des_out_w) || check_value(align_out_height,  p_vpe_config->out_img_info[i].res_dim_info.des_out_h)) {
				vpe_err("vpe drv: Res[%d]Dup[%d] des_out_w/des_out_h(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_config->out_img_info[i].res_dim_info.des_out_w, p_vpe_config->out_img_info[i].res_dim_info.des_out_h, align_out_width, align_out_height);
				ret = -1;
			}

			align_out_x_start = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_OUT_X, i, j);
			align_out_y_start = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_OUT_Y, i, j);
			//if (!align_out_x_start || !align_out_y_start) {
			//	vpe_err("vpe drv: Res[%d]Dup[%d] align_out_x_start(%d) align_out_y_start(%d) are invalid\r\n", i, j, align_out_x_start, align_out_y_start);
			//	ret = -1;
			//}

			if (check_value(align_out_x_start, p_vpe_config->out_img_info[i].res_dim_info.des_out_x) || check_value(align_out_y_start,  p_vpe_config->out_img_info[i].res_dim_info.des_out_y)) {
				vpe_err("vpe drv: Res[%d]Dup[%d] des_out_x/des_out_y(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_config->out_img_info[i].res_dim_info.des_out_x, p_vpe_config->out_img_info[i].res_dim_info.des_out_y, align_out_x_start, align_out_y_start);
				ret = -1;
			}

			align_temp = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_PIP_W, i, j);
			if ((align_pip_width != ALIGN_NA && (align_pip_width < align_temp)) || align_temp == ALIGN_NA) {
				align_pip_width = align_temp;
			}

			align_temp = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_PIP_H, i, j);
			if ((align_pip_height != ALIGN_NA && (align_pip_height < align_temp)) || align_temp == ALIGN_NA) {
				align_pip_height = align_temp;
			}

			align_temp = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_PIP_X, i, j);
			if ((align_pip_x_start != ALIGN_NA && (align_pip_x_start < align_temp)) || align_temp == ALIGN_NA) {
				align_pip_x_start = align_temp;
			}

			align_temp = vpe_drv_get_align(p_vpe_config, ALIGM_TYPE_PIP_Y, i, j);
			if ((align_pip_y_start != ALIGN_NA && (align_pip_y_start < align_temp)) || align_temp == ALIGN_NA) {
				align_pip_y_start = align_temp;
			}

		}


		//if (!align_pip_width || !align_pip_height || !align_pip_x_start || !align_pip_y_start) {
		//	vpe_err("vpe drv: Res[%d] align_pip_width(%d) align_pip_height(%d) align_pip_x_start(%d) align_pip_y_start(%d) are invalid\r\n", i, align_pip_width, align_pip_height, align_pip_x_start, align_pip_y_start);
		//	ret = -1;
		//}

		if ((align_pip_x_start == ALIGN_NA || align_pip_y_start == ALIGN_NA || align_pip_width == ALIGN_NA || align_pip_height == ALIGN_NA) &&
			(p_vpe_config->out_img_info[i].res_dim_info.hole_x || p_vpe_config->out_img_info[i].res_dim_info.hole_y || p_vpe_config->out_img_info[i].res_dim_info.hole_w || p_vpe_config->out_img_info[i].res_dim_info.hole_h)) {
			vpe_err("vpe drv: Res[%d] pip_out_x/pip_out_y/pip_out_w/pip_out_h(%d, %d, %d, %d) should be 0\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.hole_x, p_vpe_config->out_img_info[i].res_dim_info.hole_y, p_vpe_config->out_img_info[i].res_dim_info.hole_w, p_vpe_config->out_img_info[i].res_dim_info.hole_h);
			ret = -1;
		}
		if (check_value(align_pip_width, p_vpe_config->out_img_info[i].res_dim_info.hole_w) || check_value(align_pip_height,  p_vpe_config->out_img_info[i].res_dim_info.hole_h)) {
			for (j = 0; j < p_vpe_config->out_img_info[i].out_dup_num; j++) {
				vpe_err("vpe drv: Res[%d]Dup[%d] des_type=%d\r\n", i, j, p_vpe_config->out_img_info[i].out_dup_info.des_type);
			}
			vpe_err("vpe drv: PIP should support the max pip width/height alignment of all duplicates\r\n");
			vpe_err("vpe drv: Res[%d] pip_out_w/pip_out_h(%d, %d) is invalid, should be %d %d alignment\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.hole_w, p_vpe_config->out_img_info[i].res_dim_info.hole_h, align_pip_width, align_pip_height);
			ret = -1;
		}
		if (check_value(align_pip_x_start, p_vpe_config->out_img_info[i].res_dim_info.hole_x) || check_value(align_pip_y_start,  p_vpe_config->out_img_info[i].res_dim_info.hole_y)) {
			for (j = 0; j < p_vpe_config->out_img_info[i].out_dup_num; j++) {
				vpe_err("vpe drv: Res[%d]Dup[%d] des_type=%d\r\n", i, j, p_vpe_config->out_img_info[i].out_dup_info.des_type);
			}
			vpe_err("vpe drv: PIP should support the max pip x/y alignment of all duplicates\r\n");
			vpe_err("vpe drv: Res[%d] pip_out_x/pip_out_y(%d, %d) is invalid, should be %d %d alignment\r\n", i, p_vpe_config->out_img_info[i].res_dim_info.hole_x, p_vpe_config->out_img_info[i].res_dim_info.hole_y, align_pip_x_start, align_pip_y_start);
			ret = -1;
		}

	}
	if (!total_frm) {
		vpe_err("vpe drv: all out_dup_num are 0\r\n");
		ret = -1;
	}

	return ret;
}

int vpe_sca_rate_check(UINT32 res_num, UINT16 in_size, UINT16 out_size)
{
    UINT32 get_in_size, get_out_size, sca_rate;
    int get_err = 0;

    get_in_size = (UINT32)in_size;
    get_out_size = (UINT32)out_size;

    // scale down
    if (get_in_size > get_out_size) {
        sca_rate = (get_in_size - 1) * 1024 / (get_out_size - 1);

        if(sca_rate > 8192) {
            vpe_err("vpe drv: res%d scale down rate larger than 1/8x\r\n", res_num);

            get_err = -1;
        }
    }

    // scale up
    if (get_in_size < get_out_size) {
        sca_rate = (get_out_size - 1) * 1024 / (get_in_size - 1);

        if(sca_rate > 4096) {
            vpe_err("vpe drv: res%d scale up rate larger than 4x\r\n", res_num);

            get_err = -1;
        }
    }

    return get_err;
}

int vpe_hw_check_align(VPE_DRV_CFG *p_vpe_hw_config)
{
	INT32 ret = 0;
	INT32 i = 0, j = 0;
	UINT32 align_src_width = 0, align_src_height = 0, align_proc_x_start = 0, align_proc_y_start = 0, align_proc_width = 0, align_proc_height = 0;
	UINT32 align_des_width = 0, align_des_height = 0;
	UINT32 align_out_width = 0, align_out_height = 0, align_out_x_start = 0, align_out_y_start = 0;

	UINT32 align_sca_crop_width = 0, align_sca_crop_height = 0, align_sca_crop_x_start = 0, align_sca_crop_y_start = 0;
	UINT32 align_sca_width = 0, align_sca_height = 0;

	UINT32 align_sca_tc_crop_width = 0, align_sca_tc_crop_height = 0, align_sca_tc_crop_x_start = 0, align_sca_tc_crop_y_start = 0;


	align_src_width = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_SRC_W, 0, 0);
	align_src_height = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_SRC_H, 0, 0);
	if (!align_src_width || !align_src_height) {
		vpe_err("vpe drv: vpe_hw_check_align align_src_width/align_src_height(%d, %d) is invalid\r\n", align_src_width, align_src_height);
		ret = -1;
	}
	if (!(p_vpe_hw_config->glo_sz.src_width) || !(p_vpe_hw_config->glo_sz.src_height) || check_value(align_src_width, p_vpe_hw_config->glo_sz.src_width) || check_value(align_src_height, p_vpe_hw_config->glo_sz.src_height)) {
		vpe_err("vpe drv: vpe_hw_check_align src_width/src_height(%d, %d) is invalid, should be %d, %d alignment\r\n", p_vpe_hw_config->glo_sz.src_width, p_vpe_hw_config->glo_sz.src_height, align_src_width, align_src_height);
		ret = -1;
	}

	if (p_vpe_hw_config->glo_ctl.col_num > 7) {
		vpe_err("vpe drv: vpe_hw_check_align col_num(%d) is invalid, should be 0~7\r\n", p_vpe_hw_config->glo_ctl.col_num);
		ret = -1;
	}


	align_proc_width = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_PROC_W, 0, 0);
	align_proc_height = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_PROC_H, 0, 0);
	align_proc_x_start = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_PROC_X, 0, 0);
	align_proc_y_start = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_PROC_Y, 0, 0);

	if (!align_proc_width || !align_proc_height || !align_proc_x_start || !align_proc_y_start) {
		vpe_err("vpe drv: vpe_hw_check_align align_proc_width(%d) align_proc_height(%d) align_proc_x_start(%d) align_proc_y_start(%d) are invalid\r\n", align_proc_width, align_proc_height, align_proc_x_start, align_proc_y_start);
		ret = -1;
	}
	for (i = 0; i < p_vpe_hw_config->glo_ctl.col_num + 1; i++) {
		if ((p_vpe_hw_config->col_sz[i].proc_x_start + p_vpe_hw_config->col_sz[i].proc_width) > p_vpe_hw_config->glo_sz.src_width) {
			vpe_err("vpe drv: vpe_hw_check_align [Col%d] proc_x_start(%d) proc_width(%d) is invalid, should be < src_width(%d)\r\n", i, p_vpe_hw_config->col_sz[i].proc_x_start, p_vpe_hw_config->col_sz[i].proc_width, p_vpe_hw_config->glo_sz.src_width);
			ret = -1;
		}

		if (check_value(align_proc_width, p_vpe_hw_config->col_sz[i].proc_width)) {
			vpe_err("vpe drv: vpe_hw_check_align proc_width[%d](%d) is invalid, should be %d alignment\r\n", i, p_vpe_hw_config->col_sz[i].proc_width, align_proc_width);
			ret = -1;
		}

		if (check_value(align_proc_x_start, p_vpe_hw_config->col_sz[i].proc_x_start)) {
			vpe_err("vpe drv: vpe_hw_check_align proc_x_start[%d](%d) is invalid, should be %d alignment\r\n", i, p_vpe_hw_config->col_sz[i].proc_x_start, align_proc_x_start);
			ret = -1;
		}
	}

	if (!(p_vpe_hw_config->dma.src_addr)) {
		vpe_err("vpe drv: vpe_hw_check_align src_addr(0x0) is invalid\r\n");
		ret = -1;
	}

	for (i = 0; i < KDRV_VPE_MAX_IMG; i++) {
		if (p_vpe_hw_config->res[i].res_ctl.sca_en) {

			if (!(p_vpe_hw_config->res[i].res_size.des_width) || !(p_vpe_hw_config->res[i].res_size.des_height)) {
				vpe_err("vpe drv: vpe_hw_check_align Res[%d] des_width(%d)/des_height(%d) are invalid\r\n", i, p_vpe_hw_config->res[i].res_size.des_width, p_vpe_hw_config->res[i].res_size.des_height);
				ret = -1;
			}

			if (!(p_vpe_hw_config->res[i].res_size.out_height) || ((p_vpe_hw_config->res[i].res_size.out_y_start + p_vpe_hw_config->res[i].res_size.out_height) > p_vpe_hw_config->res[i].res_size.des_height)) {
				vpe_err("vpe drv: vpe_hw_check_align Res[%d] des_height(%d)/out_height(%d)/out_y_start(%d) are invalid\r\n", i, p_vpe_hw_config->res[i].res_size.des_height, p_vpe_hw_config->res[i].res_size.out_height, p_vpe_hw_config->res[i].res_size.out_y_start);
				ret = -1;
			}

			if (!(p_vpe_hw_config->res[i].res_size.rlt_height) || ((p_vpe_hw_config->res[i].res_size.rlt_y_start + p_vpe_hw_config->res[i].res_size.rlt_height) > p_vpe_hw_config->res[i].res_size.out_height)) {
				vpe_err("vpe drv: vpe_hw_check_align Res[%d] out_height(%d)/rlt_height(%d)/rlt_y_start(%d) are invalid\r\n", i, p_vpe_hw_config->res[i].res_size.out_height, p_vpe_hw_config->res[i].res_size.rlt_height, p_vpe_hw_config->res[i].res_size.rlt_y_start);
				ret = -1;
			}

			align_sca_crop_x_start = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_SCA_CROP_X, i, 0);
			align_sca_crop_y_start = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_SCA_CROP_Y, i, 0);
			align_sca_crop_width   = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_SCA_CROP_W, i, 0);
			align_sca_crop_height  = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_SCA_CROP_H, i, 0);

			align_sca_width  = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_SCA_W, i, 0);
			align_sca_height = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_SCA_H, i, 0);

			align_sca_tc_crop_x_start = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_TC_CROP_X, i, 0);
			align_sca_tc_crop_y_start = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_TC_CROP_Y, i, 0);
			align_sca_tc_crop_width   = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_TC_CROP_W, i, 0);
			align_sca_tc_crop_height  = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_TC_CROP_H, i, 0);

			align_out_width   = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_OUT_W, i, 0);
			align_out_height  = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_OUT_H, i, 0);
			align_out_x_start = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_OUT_X, i, 0);
			align_out_y_start = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_OUT_Y, i, 0);

			for (j = 0; j < (p_vpe_hw_config->glo_ctl.col_num + 1); j++) {

				if (p_vpe_hw_config->res[i].res_col_size[j].tc_crop_skip == 0) {
					if (!(p_vpe_hw_config->res[i].res_col_size[j].out_width) || ((p_vpe_hw_config->res[i].res_col_size[j].out_x_start + p_vpe_hw_config->res[i].res_col_size[j].out_width) > p_vpe_hw_config->res[i].res_size.des_width)) {
						vpe_err("vpe drv: vpe_hw_check_align Res[%d] Col[%d] des_width(%d)/out_width(%d)/out_x_start(%d) are invalid\r\n", i, j, p_vpe_hw_config->res[i].res_size.des_width, p_vpe_hw_config->res[i].res_col_size[j].out_width, p_vpe_hw_config->res[i].res_col_size[j].out_x_start);
						ret = -1;
					}

					if (!j) {
						if (!(p_vpe_hw_config->res[i].res_col_size[j].rlt_width) || ((p_vpe_hw_config->res[i].res_col_size[j].rlt_x_start + p_vpe_hw_config->res[i].res_col_size[j].rlt_width) > p_vpe_hw_config->res[i].res_col_size[j].out_width)) {
							vpe_err("vpe drv: vpe_hw_check_align Res[%d] Col[%d] out_width(%d)/rlt_width(%d)/rlt_x_start(%d) are invalid\r\n", i, j, p_vpe_hw_config->res[i].res_col_size[j].out_width, p_vpe_hw_config->res[i].res_col_size[j].rlt_width, p_vpe_hw_config->res[i].res_col_size[j].rlt_x_start);
							ret = -1;
						}
					}

					if (p_vpe_hw_config->res[i].res_col_size[j].pip_width && ((p_vpe_hw_config->res[i].res_col_size[j].pip_x_start + p_vpe_hw_config->res[i].res_col_size[j].pip_width) > p_vpe_hw_config->res[i].res_col_size[j].out_width)) {
						vpe_err("vpe drv: vpe_hw_check_align Res[%d] Col[%d] out_width(%d)/pip_width(%d)/pip_x_start(%d) are invalid\r\n", i, j, p_vpe_hw_config->res[i].res_col_size[j].out_width, p_vpe_hw_config->res[i].res_col_size[j].pip_width, p_vpe_hw_config->res[i].res_col_size[j].pip_x_start);
						ret = -1;
					}

					//align_sca_crop_x_start = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_SCA_CROP_X, i, j);
					//align_sca_crop_y_start = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_SCA_CROP_Y, i, j);
					if (check_value(align_sca_crop_x_start, p_vpe_hw_config->res[i].res_col_size[j].sca_crop_x_start) || check_value(align_sca_crop_y_start,  p_vpe_hw_config->res[i].res_sca_crop.sca_crop_y_start)) {
						vpe_err("vpe drv: vpe_hw_check_align Res[%d] Col[%d] sca_crop_x_start/sca_crop_y_start(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_hw_config->res[i].res_col_size[j].sca_crop_x_start, p_vpe_hw_config->res[i].res_sca_crop.sca_crop_y_start, align_sca_crop_x_start, align_sca_crop_y_start);
						ret = -1;
					}

					//align_sca_crop_width   = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_SCA_CROP_W, i, j);
					//align_sca_crop_height  = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_SCA_CROP_H, i, j);
					if (check_value(align_sca_crop_width, p_vpe_hw_config->res[i].res_col_size[j].sca_crop_width) || check_value(align_sca_crop_height,  p_vpe_hw_config->res[i].res_sca_crop.sca_crop_height)) {
						vpe_err("vpe drv: vpe_hw_check_align Res[%d] Col[%d] sca_crop_width/sca_crop_height(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_hw_config->res[i].res_col_size[j].sca_crop_width, p_vpe_hw_config->res[i].res_sca_crop.sca_crop_height, align_sca_crop_width, align_sca_crop_height);
						ret = -1;
					}

					//align_sca_width  = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_SCA_W, i, j);
					//align_sca_height = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_SCA_H, i, j);
					if (check_value(align_sca_width, p_vpe_hw_config->res[i].res_col_size[j].sca_width) || check_value(align_sca_height,  p_vpe_hw_config->res[i].res_size.sca_height)) {
						vpe_err("vpe drv: vpe_hw_check_align Res[%d] Col[%d] sca_width/sca_height(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_hw_config->res[i].res_col_size[j].sca_width, p_vpe_hw_config->res[i].res_size.sca_height, align_sca_width, align_sca_height);
						ret = -1;
					}

					//align_sca_tc_crop_x_start = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_TC_CROP_X, i, j);
					//align_sca_tc_crop_y_start = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_TC_CROP_Y, i, j);
					if (check_value(align_sca_tc_crop_x_start, p_vpe_hw_config->res[i].res_col_size[j].tc_crop_x_start) || check_value(align_sca_tc_crop_y_start,  p_vpe_hw_config->res[i].res_tc.tc_crop_y_start)) {
						vpe_err("vpe drv: vpe_hw_check_align Res[%d] Col[%d] tc_crop_x_start/align_sca_tc_crop_y_start(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_hw_config->res[i].res_col_size[j].tc_crop_x_start, p_vpe_hw_config->res[i].res_tc.tc_crop_y_start, align_sca_tc_crop_x_start, align_sca_tc_crop_y_start);
						ret = -1;
					}

					//align_sca_tc_crop_width = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_TC_CROP_W, i, j);
					//align_sca_tc_crop_height = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_TC_CROP_W, i, j);
					if (check_value(align_sca_tc_crop_width, p_vpe_hw_config->res[i].res_col_size[j].tc_crop_width) || check_value(align_sca_tc_crop_height,  p_vpe_hw_config->res[i].res_tc.tc_crop_height)) {
						vpe_err("vpe drv: vpe_hw_check_align Res[%d] Col[%d] tc_crop_width/tc_crop_height(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_hw_config->res[i].res_col_size[j].tc_crop_width, p_vpe_hw_config->res[i].res_tc.tc_crop_height, align_sca_tc_crop_width, align_sca_tc_crop_height);
						ret = -1;
					}


					//align_out_width = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_OUT_W, i, j);
					//align_out_height = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_OUT_H, i, j);
					if (check_value(align_out_width, p_vpe_hw_config->res[i].res_col_size[j].out_width) || check_value(align_out_height,  p_vpe_hw_config->res[i].res_size.out_height)) {
						vpe_err("vpe drv: vpe_hw_check_align Res[%d] Col[%d] out_w/out_h(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_hw_config->res[i].res_col_size[j].out_width, p_vpe_hw_config->res[i].res_size.out_height, align_out_width, align_out_height);
						ret = -1;
					}

					//align_out_x_start = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_OUT_X, i, j);
					//align_out_y_start = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_OUT_Y, i, j);
					if (check_value(align_out_x_start, p_vpe_hw_config->res[i].res_col_size[j].out_x_start) || check_value(align_out_y_start,  p_vpe_hw_config->res[i].res_size.out_y_start)) {
						vpe_err("vpe drv: vpe_hw_check_align Res[%d] Col[%d] out_x/out_y(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_hw_config->res[i].res_col_size[j].out_x_start, p_vpe_hw_config->res[i].res_size.out_y_start, align_out_x_start, align_out_y_start);
						ret = -1;
					}
				}
			}

			for (j = 0; j < 1; j++) {

				align_des_width = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_DES_W, i, j);
				align_des_height = vpe_hw_get_align(p_vpe_hw_config, ALIGM_TYPE_DES_H, i, j);
				if (check_value(align_des_width, p_vpe_hw_config->res[i].res_size.des_width) || check_value(align_des_height, p_vpe_hw_config->res[i].res_size.des_height)) {
					vpe_err("vpe drv: vpe_hw_check_align Res[%d]Dup[%d] des_width/des_height(%d, %d) is invalid, should be %d %d alignment\r\n", i, j, p_vpe_hw_config->res[i].res_size.des_width, p_vpe_hw_config->res[i].res_size.des_height, align_des_width, align_des_height);
					ret = -1;
				}


			}
		}
	}



	return ret;
}




#endif

//int vpe_drv_check_func(KDRV_VPE_CONFIG *vpe_config)
//{
//	INT32 ret = 0;
//
//	return ret;
//}

int vpe_drv_job_check(KDRV_VPE_CONFIG *p_vpe_config)
{
	INT32 ret = 0;
	//ret = vpe_drv_check_func(vpe_config);
	//if (ret) {
	//  return ret;
	//}

	ret = vpe_drv_check_align(p_vpe_config);
	return ret;
}
INT32 kdrv_vpe_get(UINT32 handle, UINT32 parm_id, KDRV_VPE_COL_INFO_GET *p_col_info_get)
{
	INT32 ret = 0;
	ret = vpe_drv_column_cal(p_col_info_get);
	return ret;
}

int vpe_drv_job_config(KDRV_VPE_CONFIG *vpe_config, struct vpe_drv_lut2d_info *lut2d)
{
	INT32 i = 0, j = 0;

	if (vpe_config == NULL) {
        DBG_ERR("vpe: parameter NULL...\r\n");

        return -1;
	}

	g_vpe_drv_cfg.glo_ctl.src_format = KDRV_VPE_SRC_TYPE_YUV420;

	for (i = 0; i < VPE_MAX_RES; i++) {
		g_vpe_drv_cfg.res[i].res_ctl.des_drt = vpe_config->out_img_info[i].res_des_drt;
		for (j = 0; j < vpe_config->out_img_info[i].out_dup_num; j++) {
			switch (vpe_config->out_img_info[i].out_dup_info.des_type) {
			case KDRV_VPE_DES_TYPE_YUV420:
				g_vpe_drv_cfg.res[i].res_ctl.des_yuv420 = 1;
				g_vpe_drv_cfg.res[i].res_ctl.des_dp0_ycc_enc_en = 0;
				break;
			case KDRV_VPE_DES_TYPE_YCC_YUV420:
				g_vpe_drv_cfg.res[i].res_ctl.des_yuv420 = 1;
				g_vpe_drv_cfg.res[i].res_ctl.des_dp0_ycc_enc_en = 1;
				break;
			default:
				break;
			}
		}

	}
	vpe_drv_ctl_config(vpe_config, &g_vpe_drv_cfg);

#if 0
	if (g_vpe_drv_cfg.glo_ctl.dce_en) {
		vpe_drv_dewarp_config(vpe_config, &g_vpe_drv_cfg);
	} else {
		vpe_drv_sca_config(vpe_config, &g_vpe_drv_cfg);
	}
#else

    for (i = 0; i < VPE_MAX_RES; i++) {

        if(vpe_sca_rate_check(i, vpe_config->out_img_info[i].res_dim_info.sca_crop_w, vpe_config->out_img_info[i].res_dim_info.sca_width) != 0)
        {
            return -1;
        }

        if(vpe_sca_rate_check(i, vpe_config->out_img_info[i].res_dim_info.sca_crop_h, vpe_config->out_img_info[i].res_dim_info.sca_height) != 0)
        {
            return -1;
        }
    }

	if (vpe_cal_col_config(vpe_config, &g_vpe_drv_cfg) != 0) {
        return -1;
	}
#endif


	vpe_drv_pal_config(vpe_config, &g_vpe_drv_cfg);
	//vpe_drv_vcache_config(vpe_config, &g_vpe_drv_cfg);

	if (g_vpe_drv_cfg.glo_ctl.sharpen_en) {
		vpe_drv_shp_config(vpe_config, &g_vpe_drv_cfg);
	}

	if (g_vpe_drv_cfg.glo_ctl.dce_en) {
		vpe_drv_dce_config(vpe_config, &g_vpe_drv_cfg, lut2d);
	}
	if (g_vpe_drv_cfg.glo_ctl.dctg_en) {
		vpe_drv_dctg_config(vpe_config, &g_vpe_drv_cfg);
	}

	vpe_drv_dma_config(vpe_config, &g_vpe_drv_cfg);

	vpe_drv_vcache_config(vpe_config, &g_vpe_drv_cfg);

	return 0;
}

int vpe_drv_job_process(KDRV_VPE_JOB_LIST *p_vpe_job_list, struct vpe_drv_lut2d_info *lut2d)
{
	//unsigned long flags;
	KDRV_VPE_CONFIG *vpe_config_pos, * vpe_config_nxt;
	VPE_LLCMD_DATA   *llcmd_data, *nxt_llcmd_data = NULL, *pos_llcmd_data = NULL;
	UINT32                 llcmd_id = 0;
	UINT32 *cmduffer = NULL;
	INT32 ret = 0;
	uint8_t comps_cnt = 0;


	//INT32 i = 0;

	memset(&g_vpe_drv_cfg, 0x0, sizeof(VPE_DRV_CFG));

	//VPE_SPIN_LOCK_IRQSAVE(p_vpe_job_list->job_lock, flags);

	//coverity[dereference]
	llcmd_data = vpe_ll_get_free_cmdlist(&g_vpe_llcmd_pool);
	llcmd_id = llcmd_data->llcmd_id;
	llcmd_data->nxt_update = 0;
	g_vpe_drv_all_job.llcmd_id[g_vpe_drv_all_job.llcmd_count] = llcmd_data->llcmd_id;
	g_vpe_drv_all_job.llcmd_count++;

	list_for_each_entry_safe(vpe_config_pos, vpe_config_nxt, &p_vpe_job_list->job_list, config_list) {
		vpe_config_pos->job_status = KDRV_VPE_STATUS_NONE;

		ret = vpe_drv_job_check(vpe_config_pos);
		if (!ret) {
			if (nxt_llcmd_data) {
				pos_llcmd_data = nxt_llcmd_data;
			} else {
				pos_llcmd_data = llcmd_data;
			}
			cmduffer = (UINT32 *)pos_llcmd_data->vaddr;

            if(vpe_drv_job_config(vpe_config_pos, lut2d) != 0) {
                g_vpe_drv_all_job.ll_process = p_vpe_job_list;

				vpe_config_pos->job_status = KDRV_VPE_STATUS_PARAM_FAIL;
				return RET_ERROR;
            }

			if (vpe_hw_check_align(&g_vpe_drv_cfg) != 0) {
				g_vpe_drv_all_job.ll_process = p_vpe_job_list;

				vpe_config_pos->job_status = KDRV_VPE_STATUS_PARAM_FAIL;
				return RET_ERROR;
			}

			//---------------------------------------------
			// swap handle for compression was enabled
			comps_cnt = 0;
			if ((g_vpe_drv_cfg.res[0].res_ctl.sca_en == ENABLE) && (g_vpe_drv_cfg.res[0].res_ctl.des_dp0_ycc_enc_en == ENABLE)) {
				comps_cnt += 1;
			}

			if ((g_vpe_drv_cfg.res[1].res_ctl.sca_en == ENABLE) && (g_vpe_drv_cfg.res[1].res_ctl.des_dp0_ycc_enc_en == ENABLE)) {
				comps_cnt += 1;
			}

			if (comps_cnt != 0) {

				if (g_vpe_drv_cfg.glo_ctl.src_uv_swap == DISABLE) {
					g_vpe_drv_cfg.glo_ctl.src_uv_swap = ENABLE;
				} else {
					g_vpe_drv_cfg.glo_ctl.src_uv_swap = DISABLE;
				}

				if ((g_vpe_drv_cfg.res[0].res_ctl.sca_en == ENABLE) && (g_vpe_drv_cfg.res[0].res_ctl.des_dp0_ycc_enc_en == DISABLE)) {
					if (g_vpe_drv_cfg.res[0].res_ctl.des_uv_swap == DISABLE) {
						g_vpe_drv_cfg.res[0].res_ctl.des_uv_swap = ENABLE;
					} else {
						g_vpe_drv_cfg.res[0].res_ctl.des_uv_swap = DISABLE;
					}
				}

				if ((g_vpe_drv_cfg.res[1].res_ctl.sca_en == ENABLE) && (g_vpe_drv_cfg.res[1].res_ctl.des_dp0_ycc_enc_en == DISABLE)) {
					if (g_vpe_drv_cfg.res[1].res_ctl.des_uv_swap == DISABLE) {
						g_vpe_drv_cfg.res[1].res_ctl.des_uv_swap = ENABLE;
					} else {
						g_vpe_drv_cfg.res[1].res_ctl.des_uv_swap = DISABLE;
					}
				}

				if ((g_vpe_drv_cfg.res[2].res_ctl.sca_en == ENABLE)) {
					if (g_vpe_drv_cfg.res[2].res_ctl.des_uv_swap == DISABLE) {
						g_vpe_drv_cfg.res[2].res_ctl.des_uv_swap = ENABLE;
					} else {
						g_vpe_drv_cfg.res[2].res_ctl.des_uv_swap = DISABLE;
					}
				}

				if ((g_vpe_drv_cfg.res[3].res_ctl.sca_en == ENABLE)) {
					if (g_vpe_drv_cfg.res[3].res_ctl.des_uv_swap == DISABLE) {
						g_vpe_drv_cfg.res[3].res_ctl.des_uv_swap = ENABLE;
					} else {
						g_vpe_drv_cfg.res[3].res_ctl.des_uv_swap = DISABLE;
					}
				}
			}
			//---------------------------------------------


			vpe_set_ctl_glo_reg(&(g_vpe_drv_cfg.glo_ctl));
			vpe_set_glo_size_reg(&(g_vpe_drv_cfg.glo_sz));
			//vpe_drv_ctl_trans_regtable_to_cmdlist(&g_vpe_drv_cfg, pos_llcmd_data);

			vpe_set_res_reg(g_vpe_drv_cfg.glo_ctl.col_num, g_vpe_drv_cfg.res);
			//vpe_drv_sca_trans_regtable_to_cmdlist(&g_vpe_drv_cfg, pos_llcmd_data);


			//vpe_drv_col_trans_regtable_to_cmdlist(&g_vpe_drv_cfg, pos_llcmd_data);
			vpe_set_src_column_reg(g_vpe_drv_cfg.glo_ctl.col_num + 1, g_vpe_drv_cfg.col_sz);


			vpe_set_pal_reg(g_vpe_drv_cfg.palette);
			//vpe_drv_pal_trans_regtable_to_cmdlist(&g_vpe_drv_cfg, pos_llcmd_data);
			//vpe_drv_vcache_trans_regtable_to_cmdlist(&g_vpe_drv_cfg, pos_llcmd_data);

			if (g_vpe_drv_cfg.glo_ctl.sharpen_en) {
				vpe_drv_shp_trans_regtable_to_cmdlist(&g_vpe_drv_cfg, pos_llcmd_data);
			}


			if (g_vpe_drv_cfg.glo_ctl.dce_en) {
				//vpe_drv_dce_trans_regtable_to_cmdlist(&g_vpe_drv_cfg, pos_llcmd_data);
				vpe_set_dce_reg(&(g_vpe_drv_cfg.dce));

				vpe_set_dce_geo_lut_reg(&g_vpe_drv_cfg);
			}

			if (g_vpe_drv_cfg.glo_ctl.dctg_en) {
				//vpe_drv_dctg_trans_regtable_to_cmdlist(&g_vpe_drv_cfg, pos_llcmd_data);
				vpe_set_dctg_reg(&(g_vpe_drv_cfg.dctg));
			}

			vpe_set_vcache_reg(&(g_vpe_drv_cfg.vcache));

			vpe_drv_dma_trans_regtable_to_cmdlist(&g_vpe_drv_cfg, pos_llcmd_data);

			nxt_llcmd_data = vpe_ll_get_free_cmdlist(&g_vpe_llcmd_pool);
			cmduffer[pos_llcmd_data->cmd_num++] = VPE_LLCMD_NXTFIRE_VAL(nxt_llcmd_data->paddr, 0);
			cmduffer[pos_llcmd_data->cmd_num++] = VPE_LLCMD_NXTFIRE(nxt_llcmd_data->paddr);

			g_vpe_drv_all_job.llcmd_id[g_vpe_drv_all_job.llcmd_count] = nxt_llcmd_data->llcmd_id;
			g_vpe_drv_all_job.llcmd_count++;
			vpe_config_pos->job_status = KDRV_VPE_STATUS_READY;
		} else {
			vpe_config_pos->job_status = KDRV_VPE_STATUS_PARAM_FAIL;
		}

	}

	if (cmduffer) {
		vpe_ll_enable(1);
		pos_llcmd_data->cmd_num = pos_llcmd_data->cmd_num - 2;
		cmduffer[pos_llcmd_data->cmd_num++] = VPE_LLCMD_FIRE_VAL(1);
		cmduffer[pos_llcmd_data->cmd_num++] = VPE_LLCMD_FIRE;

		vpe_ll_fire_llcmd(&g_vpe_llcmd_pool, llcmd_data);
		ret = RET_OK;
	} else {
		ret = RET_ERROR;
	}

	g_vpe_drv_all_job.ll_process = p_vpe_job_list;

	//VPE_SPIN_UNLOCK_IRQRESTORE(p_vpe_job_list->job_lock, flags);


	return ret;
}

void vpe_drv_job_done(UINT32 status)
{
	INT32 i = 0;
	unsigned long flags;
	KDRV_VPE_CONFIG *vpe_config_pos, * vpe_config_nxt;

	//VPE_SPIN_LOCK_IRQSAVE(g_vpe_drv_all_job.ll_process->job_lock, flags);
	list_for_each_entry_safe(vpe_config_pos, vpe_config_nxt, &g_vpe_drv_all_job.ll_process->job_list, config_list) {
		if (vpe_config_pos->job_status == KDRV_VPE_STATUS_READY) {
			vpe_config_pos->job_status = KDRV_VPE_STATUS_DONE;
		}
	}
	//VPE_SPIN_UNLOCK_IRQRESTORE(g_vpe_drv_all_job.ll_process->job_lock, flags);

	VPE_SPIN_LOCK_IRQSAVE(g_vpe_drv_all_job.job_lock, flags);

	if (g_vpe_drv_all_job.ll_frm_done) {
		g_vpe_drv_all_job.ll_frm_done = 0;
	}


	for (i = 0; i < g_vpe_drv_all_job.llcmd_count; i++) {
		vpe_ll_free_cmdlist(&g_vpe_llcmd_pool, g_vpe_drv_all_job.llcmd_id[i]);
	}

	g_vpe_drv_all_job.llcmd_count = 0;

	g_vpe_drv_all_job.ll_process->callback(g_vpe_drv_all_job.ll_process);

	//exit:
	//tasklet_schedule(&g_vpe_drv_data.job_tasklet);
    vpe_tasklet_schedule(&g_vpe_drv_data);
    
	VPE_SPIN_UNLOCK_IRQRESTORE(g_vpe_drv_all_job.job_lock, flags);
}


#if 0
void vpe_clock_gating(UINT8 enable)
{

	if (enable) {
		ftpmu010_write_reg(vpe_pmufd, 0x60, 0x0 << 30, 0x1 << 30);
		ftpmu010_write_reg(vpe_pmufd, 0x50, 0x1 << 15, 0x1 << 15);
		g_vpe_sw_config.clock_en = 1;
	} else {
		ftpmu010_write_reg(vpe_pmufd, 0x50, 0x0 << 15, 0x1 << 15);
		ftpmu010_write_reg(vpe_pmufd, 0x60, 0x1 << 30, 0x1 << 30);
		g_vpe_sw_config.clock_en = 0;
	}
}
#endif


void vpe_job_tasklet(unsigned long data)
{
	unsigned long flags;
	INT32 ret = 0;

	KDRV_VPE_JOB_LIST *cur_job_list = NULL;

	PVPE_INFO pvpe_info = (PVPE_INFO)data;

	VPE_SPIN_LOCK_IRQSAVE(g_vpe_drv_all_job.job_lock, flags);

	if (list_empty(&g_vpe_drv_all_job.job_list)) {
		if (g_vpe_sw_config.proc_gating && g_vpe_sw_config.clock_en && g_vpe_drv_all_job.ll_frm_done == 0) {
			//vpe_clock_gating(0);
		}
		goto exit;
	}

	if (!g_vpe_sw_config.clock_en) {
		//vpe_clock_gating(1);
	}

	cur_job_list = list_entry(g_vpe_drv_all_job.job_list.next, KDRV_VPE_JOB_LIST, drv_job_list);
	if (g_vpe_drv_all_job.ll_frm_done) {
		goto exit;
	}

	list_del(&cur_job_list->drv_job_list);

	ret = vpe_drv_job_process(cur_job_list, &pvpe_info->lut2d);

	g_vpe_drv_all_job.ll_frm_done = 1;

exit:
	VPE_SPIN_UNLOCK_IRQRESTORE(g_vpe_drv_all_job.job_lock, flags);

	if (ret == RET_ERROR) {
		vpe_drv_job_done(0);
	}

}





#if 0
int vpe_drv_tasklet(void)
{
	//tasklet_hi_schedule(&g_vpe_drv_data.job_tasklet);
	return 0;
}
#endif

#if 0
int vpe_drv_execute(void)
{
	return 0;
}
#endif

int vpe_drv_init_procedure(VPE_INFO *pvpe_info)
{
	//INT32 i = 0;

	memset(&g_vpe_drv_data, 0x0, sizeof(VPE_DRV_INT_INFO));
	memset(&g_vpe_drv_cfg, 0x0, sizeof(VPE_DRV_CFG));
	memset(&g_vpe_llcmd_pool, 0x0, sizeof(VPE_LLCMD_POOL));

	memset(&g_vpe_drv_all_job, 0x0, sizeof(VPE_ALL_JOB_LIST));
	memset(&g_vpe_sw_config, 0x0, sizeof(VPE_SW_CONFIG));

	g_vpe_sw_config.proc_gating = 0;

	vpe_ll_init(&g_vpe_llcmd_pool);


	INIT_LIST_HEAD(&g_vpe_drv_all_job.job_list);
	vk_spin_lock_init(&g_vpe_drv_all_job.job_lock);

	//tasklet_init(&g_vpe_drv_data.job_tasklet, vpe_job_tasklet, (unsigned long)0);
    vpe_tasklet_init(&g_vpe_drv_data, vpe_job_tasklet, (unsigned long)pvpe_info);
	vk_spin_lock_init(&g_vpe_drv_data.linklist_lock);

	


	return 0;
}


int vpe_drv_exit_procedure(void)
{
	vpe_ll_exit(&g_vpe_llcmd_pool);
	//tasklet_kill(&g_vpe_drv_data.job_tasklet);
    vpe_tasklet_exit(&g_vpe_drv_data);

	return 0;
}


#if 0
int vpe_drv_probe(struct platform_device *pdev)
{
	INT32 ret = 0;
	VPE_DRV_INFO *p_drv_data = NULL;
	UINT32   version;





	vpe_drv_init_procedure();
	/* set drv data */
	p_drv_data = &g_vpe_drv_data;

	if (unlikely(p_drv_data == NULL)) {
		vpe_err("%s fails: kzalloc not OK", __FUNCTION__);
		goto err1;
	}

	p_drv_data->id = pdev->id;
	platform_set_drvdata(pdev, p_drv_data);

	init_waitqueue_head(&p_drv_data->vpe_wait_queue);

	/* setup resource */
	p_drv_data->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (unlikely((!p_drv_data->res))) {
		vpe_err("%s fails: platform_get_resource not OK", __FUNCTION__);
		goto err2;
	}

	p_drv_data->irq_no = platform_get_irq(pdev, 0);

	if (unlikely(p_drv_data->irq_no < 0)) {
		vpe_err("%s fails: platform_get_irq not OK", __FUNCTION__);
		goto err2;
	}

	p_drv_data->mem_len = p_drv_data->res->end - p_drv_data->res->start + 1;
	p_drv_data->pbase = p_drv_data->res->start;

	if (unlikely(!request_mem_region(p_drv_data->res->start, p_drv_data->mem_len, pdev->name))) {
		vpe_err("%s fails: request_mem_region not OK", __FUNCTION__);
		goto err2;
	}

	p_drv_data->vbase = (UINT32) ioremap(p_drv_data->pbase, p_drv_data->mem_len);

	if (unlikely(p_drv_data->vbase == 0)) {
		vpe_err("%s fails: ioremap_nocache not OK", __FUNCTION__);
		goto err3;
	}

	vpe_hw_init();

#if VPE_ENABLE_IRQ
	ret = request_irq(p_drv_data->irq_no, vpe_hw_isr,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
					  SA_INTERRUPT,
#else
					  IRQF_SHARED,
#endif
					  VPEDRV_DEV_NAME,
					  p_drv_data
					 );

	if (unlikely(ret != 0)) {
		vpe_err("%s fails: request_irq not OK %d\r\n", __FUNCTION__, ret);
		goto err3;
	}
#endif

	version = vpe_hw_read(VPE_REG_IP_VERSION);
	vpe_ll_enable(1);

	printk("VPE(drv): Driver[Ver: %s] init ok, Chip Ver[0x%08x]\r\n", VPEDRV_VERSION, version);

	//vpe_clock_gating(0);

	return ret;

err3:
	release_mem_region(p_drv_data->pbase, p_drv_data->mem_len);

err2:
	platform_set_drvdata(pdev, NULL);

err1:
	return ret;
}

static int vpe_drv_remove(struct platform_device *pdev)
{
	VPE_DRV_INFO *p_drv_data = NULL;



	p_drv_data = (VPE_DRV_INFO *)platform_get_drvdata(pdev);

	/* remove resource */
	iounmap((void __iomem *)p_drv_data->vbase);
	release_mem_region(p_drv_data->pbase, p_drv_data->mem_len);
	free_irq(p_drv_data->irq_no, p_drv_data);

	/* free device memory, and set drv data as NULL */
	platform_set_drvdata(pdev, NULL);

	vpe_hw_exit();
	vpe_drv_exit_procedure();

	return 0;
}

#ifndef CONFIG_OF

static void vpe_platform_release(struct device *device)
{
	/* this is called when the reference count goes to zero */
}

static struct resource vpe_dev_resource[] = {
	[0] = {
		.start = VPE_PA_START,
		.end = VPE_PA_END,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = VPE_0_IRQ,
		.end = VPE_0_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device vpe_platform_device = {
	.name = VPEDRV_DEV_NAME,
	.id = 0,
	.num_resources = ARRAY_SIZE(vpe_dev_resource),
	.resource = vpe_dev_resource,
	.dev    = {
		.release = vpe_platform_release,
	}
};
#endif

//#ifdef CONFIG_OF
static const struct of_device_id vpe_match_table[] = {
	{.compatible = "nvt,kdrv_vpe" },
	{},
};
MODULE_DEVICE_TABLE(of, vpe_match_table);
//#endif /* CONFIG_OF */

static struct platform_driver vpe_platform_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name  = VPEDRV_DEV_NAME,
//#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(vpe_match_table),
//#endif
	},
	.probe  = vpe_drv_probe,
	.remove = vpe_drv_remove,
};
#endif


/*
 * PMU registers
 */
#if 0
static pmuReg_t vpe_reg[] = {

	/*     A reset Register [offset=0x50]
	 * ----------------------------------------------------------------
	 *  [18~21]==> 0:reset (power on default) 1:reset off
	 *
	 */
	{.reg_off   = 0x16c,   .bits_mask = (BIT31), .lock_bits = (BIT31), .init_val  = (BIT31),  .init_mask = (BIT31),    },
	{.reg_off   = 0x60,   .bits_mask = (BIT30), .lock_bits = (BIT30), .init_val  = 0,  .init_mask = (BIT30),    },
	{.reg_off   = 0x70,   .bits_mask = (BIT19), .lock_bits = (BIT19), .init_val  = 0,  .init_mask = (BIT19),    },
	{.reg_off   = 0x50,   .bits_mask = (BIT15), .lock_bits = (BIT15), .init_val  = (BIT15),  .init_mask = (BIT15),    },
	{.reg_off   = 0x58,   .bits_mask = (BIT19), .lock_bits = (BIT19), .init_val  = (BIT19),  .init_mask = (BIT19),    },
};

static pmuRegInfo_t pmu_reg_info = {
	"vpe",
	ARRAY_SIZE(vpe_reg),
	ATTR_TYPE_AXI,
	&vpe_reg[0]
};
#endif

#if 0
static int vpe_proc_addr_show(struct seq_file *sfile, void *v)
{
	dma_addr_t ll_addr = 0;
	ll_addr = vpe_ll_addr_get();
	seq_printf(sfile, "The latest ll addr = 0x%x\r\n", ll_addr);
	return 0;
}

static int vpe_proc_addr_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpe_proc_addr_show, NULL);
}
static ssize_t vpe_proc_addr_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	return size;
}


static struct file_operations proc_vpe_addr_fops = {
	.owner   = THIS_MODULE,
	.open    = vpe_proc_addr_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = vpe_proc_addr_write
};
#endif

#if 0
static int vpe_proc_gating_show(struct seq_file *sfile, void *v)
{
	dma_addr_t ll_addr = 0;
	ll_addr = vpe_ll_addr_get();
	seq_printf(sfile, "gating_en = %d\r\n", g_vpe_sw_config.proc_gating);
	return 0;
}


static int vpe_proc_gating_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpe_proc_gating_show, NULL);
}
static ssize_t vpe_proc_gating_write(struct file *file, const char __user *buffer, size_t count, loff_t *off)
{
#if 0
	char value_str[16] = {'\0'};
	if (copy_from_user(value_str, buffer, count)) {
		return -EFAULT;
	}

	sscanf(value_str, "%d\r\n", &g_vpe_sw_config.proc_gating);

	if (g_vpe_sw_config.proc_gating > 1) {
		g_vpe_sw_config.proc_gating = 1;
	}

#endif
	return count;
}
#endif

#if 0
static int vpe_proc_version_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "VPE drv version: %s\r\n", VPEDRV_VERSION);
	return 0;
}

static int vpe_proc_version_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpe_proc_version_show, NULL);
}



static struct file_operations proc_vpe_version_fops = {
	.owner   = THIS_MODULE,
	.open    = vpe_proc_version_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static struct file_operations proc_vpe_gating_fops = {
	.owner   = THIS_MODULE,
	.open    = vpe_proc_gating_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = vpe_proc_gating_write
};
#endif

#if 0
int vpe_drv_proc_init(void)
{
	INT32 ret = 0;

	vpe_drv_root = proc_mkdir("kdrv_vpe", NULL);
	if (vpe_drv_root == NULL) {
		vpe_err("failed to create Module root\r\n");
		ret = -EINVAL;
		return ret;
	}

	vpe_drv_addr = proc_create("addr", S_IRUGO | S_IXUGO, vpe_drv_root, &proc_vpe_addr_fops);
	if (vpe_drv_addr == NULL) {
		vpe_err("failed to create proc cmd!\r\n");
		ret = -EINVAL;
		goto remove_root;
	}

	vpe_drv_gating_en = proc_create("gating_en", S_IRUGO | S_IXUGO, vpe_drv_root, &proc_vpe_gating_fops);
	if (vpe_drv_gating_en == NULL) {
		vpe_err("failed to create proc cmd!\r\n");
		ret = -EINVAL;
		goto remove_root;
	}

	vpe_drv_addr = proc_create("version", S_IRUGO | S_IXUGO, vpe_drv_root, &proc_vpe_version_fops);
	if (vpe_drv_addr == NULL) {
		vpe_err("failed to create proc cmd!\r\n");
		ret = -EINVAL;
		goto remove_root;
	}


	return ret;

remove_root:
	proc_remove(vpe_drv_addr);
	proc_remove(vpe_drv_gating_en);

	proc_remove(vpe_drv_root);


	return ret;
}
#endif

#if 0
void vpe_drv_proc_remove(void)
{
	if (vpe_drv_addr) {
		proc_remove(vpe_drv_addr);
	}

	if (vpe_drv_gating_en) {
		proc_remove(vpe_drv_gating_en);
	}

	if (vpe_drv_root) {
		proc_remove(vpe_drv_root);
	}

}
#endif

#if 0
#if defined(_GROUP_KO_)
#undef __init
#undef __exit
#undef module_init
#undef module_exit
#define __init
#define __exit
#define module_init(x)
#define module_exit(x)
#endif
#endif

#if 0
INT32 __init nvt_vpe_module_init(void)
{
	INT32 ret;

#if 0
	vpeclk = clk_get(NULL, "vpe_clk");
	if (IS_ERR(vpeclk)) {
		vpe_err("[vpe_drv_init] get vpe_clk failed!(err:%ld)\r\n", PTR_ERR(vpeclk));
		vpeclk = 0;
	}
	clk_prepare_enable(vpeclk);
#endif

	/* register device */
#ifndef CONFIG_OF
	if ((ret = platform_device_register(&vpe_platform_device))) {
		vpe_err("Failed to register platform device '%s'\r\n", vpe_platform_device.name);
	}
#endif

	/* register driver */
	if ((ret = platform_driver_register(&vpe_platform_driver))) {
		vpe_err("Failed to register platform driver '%s'\r\n", vpe_platform_driver.driver.name);
	}

	vpe_drv_proc_init();



	return 0;
}
#endif

#if 0
void __exit nvt_vpe_module_exit(void)
{

	//ftpmu010_deregister_reg(vpe_pmufd);
	vpe_drv_proc_remove();

	platform_driver_unregister(&vpe_platform_driver);

#ifndef CONFIG_OF
	platform_device_unregister(&vpe_platform_device);
#endif

	if (vpeclk) {
		clk_disable_unprepare(vpeclk);
		clk_put(vpeclk);
		vpeclk = 0;
	}
}
#endif

//module_init(nvt_vpe_module_init);
//module_exit(nvt_vpe_module_exit);

//MODULE_AUTHOR("Novatek Corp.");
//MODULE_DESCRIPTION("vpe driver");
//MODULE_LICENSE("GPL");
//MODULE_VERSION("1.00.000");



//-------------------------------------------------------------
static void kdrv_vpe_lock(KDRV_VPE_HANDLE *p_handle, BOOL flag)
{
	if (flag == TRUE) {
		FLGPTN flag_ptn;
		wai_flg(&flag_ptn, p_handle->flag_id, p_handle->lock_bit, TWF_CLR);
	} else {
		set_flg(p_handle->flag_id, p_handle->lock_bit);
	}
}


static void kdrv_vpe_isr_cb(UINT32 intstatus)
{
	KDRV_VPE_HANDLE *p_handle;

	p_handle = g_kdrv_vpe_trig_hdl;

	if (p_handle == NULL) {
		return;
	}

	//if (intstatus & KDRV_VPE_INTS_FRM_END) {
	//  kdrv_vpe_sem(p_handle, FALSE);
	//  kdrv_vpe_frm_end(p_handle, TRUE);
	//}

	if (p_handle->isrcb_fp != NULL) {
		p_handle->isrcb_fp((UINT32)p_handle, intstatus, NULL, NULL);
	}
}

INT32 kdrv_vpe_open(UINT32 chip, UINT32 engine)
{
	ER er_return = E_OK;

	UINT32 eng_id = 0;
	UINT32 i;

	if (efuse_check_available("vpe_lib") != TRUE) {
		DBG_FATAL("cannot support %s.\r\n", "vpe_lib");

		return -1;
	} else {
		switch (engine) {
		case KDRV_VIDEOPROCS_VPE_ENGINE0:
			eng_id = KDRV_VPE_ID_0;
			break;

		default:
			DBG_ERR("KDRV VPE: engine id error...\r\n");
			return -1;
			break;
		}


		// get vpe resource
		er_return = vpe_lock();
		if (er_return != E_OK) {
			DBG_ERR("IME: Re-opened...\r\n");

			return er_return;
		}

		vpe_platform_set_clk_rate(g_kdrv_vpe_clk_info[(UINT32)eng_id].vpe_clock_sel);

		vpe_attach();

		vpeg = (NT98528_VPE_REGISTER_STRUCT *)((UINT32)_VPE_REG_BASE_ADDR);

		vpe_platform_disable_sram_shutdown();

		vpe_platform_int_enable();

		vpe_set_ctl_pipe_reg();
		vpe_set_compression_coefs_reg();

		kdrv_vpe_lock_chls = 0;
		for (i = 0; i < kdrv_vpe_channel_num; i++) {
			kdrv_vpe_lock_chls += (1 << i);
		}

		vpe_p_int_hdl_callback = kdrv_vpe_isr_cb;

		return 0;
	}
}

INT32 kdrv_vpe_close(UINT32 handle)
{
	ER er_return = E_OK;

	//debug_dumpmem(0xf0cd0000, 0x1200);

	vpe_hw_exit();
	//vpe_drv_exit_procedure();


	vpe_platform_int_disable();
	//drv_disableInt(DRV_INT_IME);

	// disable engine clock
	//if (fw_vpe_power_saving_en == TRUE) {
	//	vpe_platform_enable_sram_shutdown();
	//	vpe_detach();
	//}
	vpe_platform_enable_sram_shutdown();
	vpe_detach();

	kdrv_vpe_lock_chls = 0;

	vpe_p_int_hdl_callback = NULL;
	g_kdrv_vpe_trig_hdl = NULL;

	// release resource
	er_return = vpe_unlock();
	if (er_return != E_OK) {
		return E_SYS;
	}

	return 0;
}

INT32 kdrv_vpe_set(UINT32 id, KDRV_VPE_PARAM_ID param_id, void *p_param)
{
	//ER rt = E_OK;
	KDRV_VPE_HANDLE *p_handle;
	UINT32 ign_chk;

	UINT32 eng_id, chl_id;

	ign_chk = (KDRV_VPE_IGN_CHK & param_id);
	param_id = param_id & (~KDRV_VPE_IGN_CHK);

#if 0
	switch (param_id) {
	case KDRV_VPE_PARAM_IPL_LOAD:
	case KDRV_VPE_PARAM_IPL_RESET:
		break;

	default:
		if (p_param == NULL) {
			DBG_ERR("KDRV VPE: set parameter NULL...\r\n");
			return -1;
		}
		break;
	}
#endif

	eng_id = KDRV_DEV_ID_ENGINE(id);
	chl_id = KDRV_DEV_ID_CHANNEL(id);

	switch (eng_id) {
	case KDRV_VIDEOPROCS_VPE_ENGINE0:
		break;

	default:
		DBG_ERR("KDRV VPE: engine id error\r\n");
		return -1;
		break;
	}

#if 1
	if (chl_id >= kdrv_vpe_channel_num) {
		DBG_ERR("KDRV VPE: channel id error\r\n");
		return -1;
	}

	//p_handle = (KDRV_VPE_HANDLE *)chl_id;
	kdrv_vpe_channel_alloc(chl_id);
	p_handle = &g_kdrv_vpe_handle_tab[chl_id];
	if (kdrv_vpe_check_handle(p_handle) == NULL) {
		DBG_WRN("KDRV VPE: illegal handle 0x%.8x\r\n", (unsigned int)chl_id);
		return -1;
	}

	if ((p_handle->sts & KDRV_VPE_HANDLE_LOCK) != KDRV_VPE_HANDLE_LOCK) {
		DBG_WRN("KDRV VPE: illegal handle sts 0x%.8x\r\n", (unsigned int)p_handle->sts);
		return -1;
	}


	if (!ign_chk) {
		kdrv_vpe_lock(p_handle, TRUE);
	}

	if (kdrv_vpe_set_fp[param_id] != NULL) {
		if (kdrv_vpe_set_fp[param_id](p_handle->entry_id, p_param) != E_OK) {
			return -1;
		}
	}

	if (!ign_chk) {
		kdrv_vpe_lock(p_handle, FALSE);
	}
#else
	if (kdrv_vpe_set_fp[param_id] != NULL) {
		if (kdrv_vpe_set_fp[param_id](p_handle->entry_id, p_param) != E_OK) {
			return -1;
		}
	}
#endif
	return 0;
}



INT32 kdrv_vpe_trigger(UINT32 id, KDRV_VPE_JOB_LIST *p_job_list)
{
	KDRV_VPE_HANDLE *p_handle;

	unsigned long flags;

	UINT32 channel = KDRV_DEV_ID_CHANNEL(id);

	//DBG_IND("id = 0x%x\r\n", id);
	//if (p_ipe_param == NULL) {
	//  DBG_ERR("p_ipe_param is NULL\r\n");
	//  return E_SYS;
	//}
	if (KDRV_DEV_ID_ENGINE(id) != KDRV_VIDEOPROCS_VPE_ENGINE0) {
		DBG_ERR("Invalid engine 0x%x\r\n", (unsigned int)KDRV_DEV_ID_ENGINE(id));
		return E_ID;
	}

	p_handle = kdrv_vpe_entry_id_conv2_handle(channel);
	g_kdrv_vpe_trig_hdl = p_handle;

    // enable engine clock and disable engine clock by isr
	if (fw_vpe_power_saving_en == TRUE) {
	    vpe_attach();
		vpe_platform_disable_sram_shutdown();		
	}

	VPE_SPIN_LOCK_IRQSAVE(g_vpe_drv_all_job.job_lock, flags);
	list_add_tail(&p_job_list->drv_job_list, &g_vpe_drv_all_job.job_list);
	VPE_SPIN_UNLOCK_IRQRESTORE(g_vpe_drv_all_job.job_lock, flags);

	//tasklet_schedule(&g_vpe_drv_data.job_tasklet);
	vpe_tasklet_schedule(&g_vpe_drv_data);

	return 0;
}

//-------------------------------------------------------------





