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


#include "ime_api.h"
#include "ime_drv.h"
#include "ime_dbg.h"
//#include "ime_lib.h"

//#include "kdrv_videoprocess/kdrv_ipp_utility.h"

//#include "kdrv_videoprocess/kdrv_ime.h"

//#include "kdrv_videoprocess/kdrv_ipp.h"
//#include "kdrv_videoprocess/kdrv_ipp_config.h"


//#include "kdrv_type.h"

#if 0
typedef struct _ime_img_info_ {
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
} ime_img_info;
#endif

//IME_MODE_PRAM ime_param = {0};

#if 0
void ime_bypass_test(ime_img_info *p_ime_img_info)
{
	IME_OPENOBJ ime_open_obj;



	ime_open_obj.uiImeClockSel = 240;
	ime_open_obj.FP_IMEISR_CB = NULL;
	ime_open(&ime_open_obj);
	nvt_dbg(IND, "ime open...\r\n");

	memset((&ime_param), 0, sizeof(IME_MODE_PRAM));

	// input info
	ime_param.operation_mode = IME_OPMODE_D2D;
	ime_param.in_path_info.InSize.uiSizeH = p_ime_img_info->in_size_h;
	ime_param.in_path_info.InSize.uiSizeV = p_ime_img_info->in_size_v;
	ime_param.in_path_info.in_lineoffset.uiLineOffsetY = p_ime_img_info->in_size_ofs_y;
	ime_param.in_path_info.in_lineoffset.uiLineOffsetCb = p_ime_img_info->in_size_ofs_uv;
	ime_param.in_path_info.in_format = IME_INPUT_Y_ONLY;
	ime_param.in_path_info.in_addr.uiAddrY = p_ime_img_info->in_addr_y;
	ime_param.in_path_info.in_addr.uiAddrCb = p_ime_img_info->in_addr_uv;
	ime_param.in_path_info.in_addr.uiAddrCr = p_ime_img_info->in_addr_uv;


	//output info
	ime_param.out_path1.out_path_enable = IME_FUNC_ENABLE;
	ime_param.out_path1.out_path_dram_enable = IME_FUNC_ENABLE;
	ime_param.out_path1.out_path_sprtout_enable = IME_FUNC_DISABLE;
	ime_param.out_path1.out_path_out_dest = IME_OUTDST_DRAM;
	ime_param.out_path1.out_path_image_format.OutFormatTypeSel = IME_OUTPUT_Y_ONLY;
	ime_param.out_path1.out_path_image_format.OutFormatSel = IME_OUTPUT_YCC_PLANAR;
	ime_param.out_path1.out_path_scale_size.uiSizeH = p_ime_img_info->out_size_h;
	ime_param.out_path1.out_path_scale_size.uiSizeV = p_ime_img_info->out_size_v;
	ime_param.out_path1.out_path_scale_factors.CalScaleFactorMode = IME_SCALE_FACTOR_COEF_AUTO_MODE;
	ime_param.out_path1.out_path_scale_filter.CoefCalMode = IME_SCALE_FILTER_COEF_USER_MODE;
	ime_param.out_path1.out_path_scale_filter.HScaleFilterEnable = IME_FUNC_DISABLE;
	ime_param.out_path1.out_path_scale_filter.VScaleFilterEnable = IME_FUNC_DISABLE;
	ime_param.out_path1.out_path_scale_method = IMEALG_BILINEAR;
	ime_param.out_path1.out_path_scale_enh.EnhMode = IME_SCALE_ENH_MODE_USER;
	ime_param.out_path1.out_path_scale_enh.uiEnhFactor = 0;
	ime_param.out_path1.out_path_scale_enh.uiEnhBit = 0;
	ime_param.out_path1.out_path_addr.uiAddrY = p_ime_img_info->out_addr_y;
	ime_param.out_path1.out_path_addr.uiAddrCb = p_ime_img_info->out_addr_uv;
	ime_param.out_path1.out_path_addr.uiAddrCr = p_ime_img_info->out_addr_uv;
	ime_param.out_path1.out_path_crop_pos.uiPosX = 0;
	ime_param.out_path1.out_path_crop_pos.uiPosY = 0;
	ime_param.out_path1.out_path_out_size.uiSizeH = p_ime_img_info->out_size_h;
	ime_param.out_path1.out_path_out_size.uiSizeV = p_ime_img_info->out_size_v;
	ime_param.out_path1.out_path_out_lineoffset.uiLineOffsetY = p_ime_img_info->out_size_ofs_y;
	ime_param.out_path1.out_path_out_lineoffset.uiLineOffsetCb = p_ime_img_info->out_size_ofs_uv;

	ime_param.out_path1.OutPathEncode.EncodeEn = IME_FUNC_DISABLE;        ///< path encode enable, only for output path1

	ime_param.out_path2.out_path_enable = IME_FUNC_DISABLE;
	ime_param.out_path3.out_path_enable = IME_FUNC_DISABLE;
	ime_param.out_path4.out_path_enable = IME_FUNC_DISABLE;
	ime_param.out_path5.out_path_enable = IME_FUNC_DISABLE;

	ime_param.interrupt_enable = IME_INTE_FRM_END;
	ime_param.set_stripe.StripeCalMode = IME_STRIPE_AUTO_MODE;

	ime_param.p_ime_iq_info = NULL;

	ime_set_mode(&ime_param);
	nvt_dbg(IND, "ime setMode...done\r\n");

	ime_clear_flag_frame_end();

	ime_start();
	nvt_dbg(IND, "ime start...done\r\n");

	ime_waitFlagFrameEnd(IME_FLAG_NO_CLEAR);
	nvt_dbg(IND, "ime wait frame-end...done\r\n");

	ime_pause();
	nvt_dbg(IND, "ime pause...done\r\n");

	ime_close();
	nvt_dbg(IND, "ime close...done\r\n");
}
#endif

#if (__IME_DBG_CMD__ == 1)

int nvt_ime_api_write_reg(PIME_INFO pime_info, unsigned char argc, char **pargv)
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

	nvt_ime_drv_write_reg(pime_info, reg_addr, reg_value);
	return 0;
}

int nvt_ime_api_write_pattern(PIME_INFO pime_info, unsigned char argc, char **pargv)
{
	return 0;
}

int nvt_ime_api_read_reg(PIME_INFO pime_info, unsigned char argc, char **pargv)
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
	value = nvt_ime_drv_read_reg(pime_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%x = 0x%x\n", (unsigned int)reg_addr, (unsigned int)value);
	return 0;
}

int nvt_ime_emu(PIME_INFO pime_info, unsigned char argc, char **pargv)
{
	return 0;
}
#endif

