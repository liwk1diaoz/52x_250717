#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
//#include <comm/nvtmem.h>

#include "mach/rcw_macro.h"
#include "mach/nvt-io.h"
#include "kwrap/type.h"
#include <plat-na51055/nvt-sramctl.h>
//#include <frammap/frammap_if.h>
#include <mach/fmem.h>


#include "vpe_api.h"
#include "vpe_drv.h"
#include "vpe_dbg.h"

#if 0
typedef struct _vpe_img_info_ {
	UINT32 in_size_h;
	UINT32 in_size_v;
	UINT32 in_size_ofs_y;
	UINT32 in_size_ofs_uv;
	UINT32 in_addr_y;
	UINT32 in_addr_uv;

	UINT32 out_size_h;
	UINT32 out_size_v;
	UINT32 out_size_ofs_y;
	UINT32 out_size_ofs_uv;
	UINT32 out_addr_y;
	UINT32 out_addr_uv;
} vpe_img_info;
#endif

//VPE_MODE_PRAM vpe_param = {0};

#if 0
void vpe_bypass_test(vpe_img_info *p_vpe_img_info)
{
	VPE_OPENOBJ vpe_open_obj;



	vpe_open_obj.uiImeClockSel = 240;
	vpe_open_obj.FP_VPEISR_CB = NULL;
	vpe_open(&vpe_open_obj);
	nvt_dbg(IND, "vpe open...\r\n");

	memset((&vpe_param), 0, sizeof(VPE_MODE_PRAM));

	// input info
	vpe_param.operation_mode = VPE_OPMODE_D2D;
	vpe_param.in_path_info.InSize.uiSizeH = p_vpe_img_info->in_size_h;
	vpe_param.in_path_info.InSize.uiSizeV = p_vpe_img_info->in_size_v;
	vpe_param.in_path_info.in_lineoffset.uiLineOffsetY = p_vpe_img_info->in_size_ofs_y;
	vpe_param.in_path_info.in_lineoffset.uiLineOffsetCb = p_vpe_img_info->in_size_ofs_uv;
	vpe_param.in_path_info.in_format = VPE_INPUT_Y_ONLY;
	vpe_param.in_path_info.in_addr.uiAddrY = p_vpe_img_info->in_addr_y;
	vpe_param.in_path_info.in_addr.uiAddrCb = p_vpe_img_info->in_addr_uv;
	vpe_param.in_path_info.in_addr.uiAddrCr = p_vpe_img_info->in_addr_uv;


	//output info
	vpe_param.out_path1.out_path_enable = VPE_FUNC_ENABLE;
	vpe_param.out_path1.out_path_dram_enable = VPE_FUNC_ENABLE;
	vpe_param.out_path1.out_path_sprtout_enable = VPE_FUNC_DISABLE;
	vpe_param.out_path1.out_path_out_dest = VPE_OUTDST_DRAM;
	vpe_param.out_path1.out_path_image_format.OutFormatTypeSel = VPE_OUTPUT_Y_ONLY;
	vpe_param.out_path1.out_path_image_format.OutFormatSel = VPE_OUTPUT_YCC_PLANAR;
	vpe_param.out_path1.out_path_scale_size.uiSizeH = p_vpe_img_info->out_size_h;
	vpe_param.out_path1.out_path_scale_size.uiSizeV = p_vpe_img_info->out_size_v;
	vpe_param.out_path1.out_path_scale_factors.CalScaleFactorMode = VPE_SCALE_FACTOR_COEF_AUTO_MODE;
	vpe_param.out_path1.out_path_scale_filter.CoefCalMode = VPE_SCALE_FILTER_COEF_USER_MODE;
	vpe_param.out_path1.out_path_scale_filter.HScaleFilterEnable = VPE_FUNC_DISABLE;
	vpe_param.out_path1.out_path_scale_filter.VScaleFilterEnable = VPE_FUNC_DISABLE;
	vpe_param.out_path1.out_path_scale_method = VPEALG_BILINEAR;
	vpe_param.out_path1.out_path_scale_enh.EnhMode = VPE_SCALE_ENH_MODE_USER;
	vpe_param.out_path1.out_path_scale_enh.uiEnhFactor = 0;
	vpe_param.out_path1.out_path_scale_enh.uiEnhBit = 0;
	vpe_param.out_path1.out_path_addr.uiAddrY = p_vpe_img_info->out_addr_y;
	vpe_param.out_path1.out_path_addr.uiAddrCb = p_vpe_img_info->out_addr_uv;
	vpe_param.out_path1.out_path_addr.uiAddrCr = p_vpe_img_info->out_addr_uv;
	vpe_param.out_path1.out_path_crop_pos.uiPosX = 0;
	vpe_param.out_path1.out_path_crop_pos.uiPosY = 0;
	vpe_param.out_path1.out_path_out_size.uiSizeH = p_vpe_img_info->out_size_h;
	vpe_param.out_path1.out_path_out_size.uiSizeV = p_vpe_img_info->out_size_v;
	vpe_param.out_path1.out_path_out_lineoffset.uiLineOffsetY = p_vpe_img_info->out_size_ofs_y;
	vpe_param.out_path1.out_path_out_lineoffset.uiLineOffsetCb = p_vpe_img_info->out_size_ofs_uv;

	vpe_param.out_path1.OutPathEncode.EncodeEn = VPE_FUNC_DISABLE;        ///< path encode enable, only for output path1

	vpe_param.out_path2.out_path_enable = VPE_FUNC_DISABLE;
	vpe_param.out_path3.out_path_enable = VPE_FUNC_DISABLE;
	vpe_param.out_path4.out_path_enable = VPE_FUNC_DISABLE;
	vpe_param.out_path5.out_path_enable = VPE_FUNC_DISABLE;

	vpe_param.interrupt_enable = VPE_INTE_FRM_END;
	vpe_param.set_stripe.StripeCalMode = VPE_STRIPE_AUTO_MODE;

	vpe_param.p_vpe_iq_info = NULL;

	vpe_set_mode(&vpe_param);
	nvt_dbg(IND, "vpe setMode...done\r\n");

	vpe_clear_flag_frame_end();

	vpe_start();
	nvt_dbg(IND, "vpe start...done\r\n");

	vpe_waitFlagFrameEnd(VPE_FLAG_NO_CLEAR);
	nvt_dbg(IND, "vpe wait frame-end...done\r\n");

	vpe_pause();
	nvt_dbg(IND, "vpe pause...done\r\n");

	vpe_close();
	nvt_dbg(IND, "vpe close...done\r\n");
}
#endif

int nvt_vpe_api_write_reg(PVPE_INFO pvpe_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0, reg_value = 0;

	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", (int)argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtoul(pargv[1], 0, &reg_value)) {
		nvt_dbg(ERR, "invalid rag value:%s\n", pargv[1]);
		return -EINVAL;

	}

	nvt_dbg(IND, "W REG 0x%x to 0x%x\n", (unsigned int)reg_value, (unsigned int)reg_addr);

	nvt_vpe_drv_write_reg(pvpe_info, reg_addr, reg_value);
	return 0;
}

int nvt_vpe_api_write_pattern(PVPE_INFO pvpe_info, unsigned char argc, char **pargv)
{
	return 0;
}

int nvt_vpe_api_read_reg(PVPE_INFO pvpe_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0;
	unsigned long value;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", (int)argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "R REG 0x%lx\n", reg_addr);
	value = nvt_vpe_drv_read_reg(pvpe_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%x = 0x%x\n", (unsigned int)reg_addr, (unsigned int)value);
	return 0;
}

int nvt_vpe_emu(PVPE_INFO pvpe_info, unsigned char argc, char **pargv)
{
	return 0;
}


