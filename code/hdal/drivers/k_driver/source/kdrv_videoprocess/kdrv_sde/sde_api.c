#include <comm/nvtmem.h>
#include <plat-na51055/nvt-sramctl.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "sde_api.h"
#include "sde_drv.h"
#include "sde_dbg.h"

#include <linux/delay.h>
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "sde_lib.h"
//#include "frammap/frammap_if.h"
#include <mach/fmem.h>
#include "kdrv_videoprocess/kdrv_sde.h"

extern int nvt_kdrv_sde_api_test_drv(PSDE_INFO pmodule_info, unsigned char argc, char **pargv);
extern int nvt_kdrv_sde_api_test_kdrv(PSDE_INFO pmodule_info, unsigned char argc, char **pargv);


static UINT32 kdrv_sde_fmd_flag = 0;
INT32 KDRV_SDE_ISR_CB(UINT32 handle, UINT32 sts, void *in_data, void *out_data)
{
	if (sts & KDRV_SDE_INT_2FMD) {
		kdrv_sde_fmd_flag += 1;
	}
	return 0;
}

int nvt_sde_api_write_reg(PSDE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0, reg_value = 0;
	nvt_dbg(ERR, "nvt_sde_api_write_reg    argc:%d", argc);
	
	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}
	nvt_dbg(ERR, "nvt_sde_api_write_reg    reg_addr:0x%lx", reg_addr);
	

	if (kstrtoul(pargv[1], 0, &reg_value)) {
		nvt_dbg(ERR, "invalid rag value:%s\n", pargv[1]);
		return -EINVAL;

	}
	nvt_dbg(ERR, "nvt_sde_api_write_reg    reg_value:0x%lx", reg_value);

	nvt_dbg(ERR, "W REG 0x%lx to 0x%lx\n", reg_value, reg_addr);

	nvt_sde_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}

int nvt_sde_api_write_pattern(PSDE_INFO pmodule_info, unsigned char argc, char **pargv)
{
#if 0
	mm_segment_t old_fs;
	struct file *fp;
	int len = 0;
	//unsigned char *pbuffer;
	frammap_buf_t      buf_info = {0};

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	fp = filp_open(pargv[0], O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		nvt_dbg(ERR, "failed in file open:%s\n", pargv[0]);
		return -EFAULT;
	}

	//Allocate memory
	buf_info.size = 0x600000;
	buf_info.align = 64;      ///< address alignment
	buf_info.name = "nvtmpp";
	buf_info.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &buf_info);
	nvt_dbg(IND, "buf_info.va_addr = 0x%08x, buf_info.phy_addr = 0x%08x\r\n", (UINT32)buf_info.va_addr, (UINT32)buf_info.phy_addr);


	old_fs = get_fs();
	set_fs(get_ds());

	len = vfs_read(fp, buf_info.va_addr, 1152 * 64, &fp->f_pos);

	/* Do something after get data from file */
	//sde_bypass_test((UINT32)buf_info.va_addr);

	//kfree(pbuffer);
	frm_free_buf_ddr(buf_info.va_addr);
	filp_close(fp, NULL);
	set_fs(old_fs);

	return len;
#endif
	return 0;
}

int nvt_sde_api_read_reg(PSDE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0;
	unsigned long value;

	//nvt_dbg(ERR, "wrong argument:%s", pargv[0]);
	
	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(ERR, "R REG 0x%lx\n", reg_addr);
	value = nvt_sde_drv_read_reg(pmodule_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%lx = 0x%lx\n", reg_addr, value);
	return 0;
}

int nvt_kdrv_sde_api_test_drv(PSDE_INFO pmodule_info, unsigned char argc, char **pargv)
{
#if 0
	ER er_return = E_OK;

	frammap_buf_t in0_buf_info = {0};
	frammap_buf_t in1_buf_info = {0};
	frammap_buf_t out_buf_info = {0};
	mm_segment_t old_fs;
	struct file *fp;
	int len = 0;

	SDE_OPENOBJ sde_openobj = {0};
	SDE_PARAM sde_param = {0};

	/*get in buffer*/
	in0_buf_info.size = 512 * 384;
	in0_buf_info.align = 64;      ///< address alignment
	in0_buf_info.name = "sde_t_in0";
	in0_buf_info.alloc_type = ALLOC_CACHEABLE;
	if (frm_get_buf_ddr(DDR_ID0, &in0_buf_info)) {
		DBG_ERR("get input buffer fail\n");
		return -EFAULT;
	}
	nvt_dbg(IND, "input0 : va_addr = 0x%08x, phy_addr = 0x%08x\r\n", (UINT32)in0_buf_info.va_addr, (UINT32)in0_buf_info.phy_addr);

	in1_buf_info.size = 512 * 384;
	in1_buf_info.align = 64;      ///< address alignment
	in1_buf_info.name = "sde_t_in1";
	in1_buf_info.alloc_type = ALLOC_CACHEABLE;
	if (frm_get_buf_ddr(DDR_ID0, &in1_buf_info)) {
		DBG_ERR("get input buffer fail\n");
		return -EFAULT;
	}
	nvt_dbg(IND, "input1 : va_addr = 0x%08x, phy_addr = 0x%08x\r\n", (UINT32)in1_buf_info.va_addr, (UINT32)in1_buf_info.phy_addr);

	/*get out buffer*/
	out_buf_info.size = ((512 * 384 * 32 * 6) >> 3);
	out_buf_info.align = 64;      ///< address alignment
	out_buf_info.name = "sde_t_out";
	out_buf_info.alloc_type = ALLOC_CACHEABLE;
	if (frm_get_buf_ddr(DDR_ID0, &out_buf_info)) {
		frm_free_buf_ddr(out_buf_info.va_addr);
		DBG_ERR("get output buffer fail\n");
		return -EFAULT;
	}
	nvt_dbg(IND, "output : va_addr = 0x%08x, phy_addr = 0x%08x\r\n", (UINT32)out_buf_info.va_addr, (UINT32)out_buf_info.phy_addr);


	/* load image */
	fp = filp_open(pargv[0], O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		DBG_ERR("failed in file open:%s\n", pargv[0]);
		frm_free_buf_ddr(in0_buf_info.va_addr);
		frm_free_buf_ddr(in1_buf_info.va_addr);
		frm_free_buf_ddr(out_buf_info.va_addr);
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	len += vfs_read(fp, in0_buf_info.va_addr, in0_buf_info.size, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);

	/* load image */
	fp = filp_open(pargv[1], O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		DBG_ERR("failed in file open:%s\n", pargv[1]);
		frm_free_buf_ddr(in0_buf_info.va_addr);
		frm_free_buf_ddr(in1_buf_info.va_addr);
		frm_free_buf_ddr(out_buf_info.va_addr);
		len = -EFAULT;
		goto EOP;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	len += vfs_read(fp, in1_buf_info.va_addr, in1_buf_info.size, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);

	sde_openobj.FP_SDEISR_CB = NULL;
	sde_openobj.uiSdeClockSel = 480;

	sde_param.ioPara.Size.uiWidth  = 512;
	sde_param.ioPara.Size.uiHeight = 384;

	//SDE lofs setting
	sde_param.ioPara.Size.uiOfsi0 = 512;
	sde_param.ioPara.Size.uiOfsi1 = 512;
	sde_param.ioPara.Size.uiOfsi2 = 12288;
	sde_param.ioPara.Size.uiOfso = 12288;

	//SDE address
	sde_param.ioPara.uiInAddr0 = (UINT32)in0_buf_info.va_addr;
	sde_param.ioPara.uiInAddr1 = (UINT32)in1_buf_info.va_addr;
	sde_param.ioPara.uiInAddr2 = (UINT32)in1_buf_info.va_addr;
	sde_param.ioPara.uiOutAddr = (UINT32)out_buf_info.va_addr;

	//SDE_OUT_VOL
	sde_param.bHflip01 = 1;
	sde_param.bVflip01 = 1;
	sde_param.bHflip2 = 0;
	sde_param.bVflip2 = 0;
	sde_param.bInput2En = 0;
	sde_param.bCostCompMode = 1;
	sde_param.outSel = 1; // output vol.

	sde_param.opMode = SDE_MAX_DISP_31; // Disparity 31 or 63 scan mode
	sde_param.outVolSel = 0; // Full volumn mode, 4 scan each
	sde_param.bScanEn = 1;
	sde_param.bDiagEn = 0;
	sde_param.invSel = 0;

	// Cost parameters
	sde_param.costPara.uiAdBoundLower = 5;
	sde_param.costPara.uiAdBoundUpper = 63;
	sde_param.costPara.uiCensusBoundLower = 4;
	sde_param.costPara.uiCensusBoundUpper = 21;
	sde_param.costPara.uiCensusAdRatio =  208;

	// Luma parameters
	sde_param.lumaPara.uiLumaThreshSaturated = 212;
	sde_param.lumaPara.uiCostSaturatedMatch = 15;
	sde_param.lumaPara.uiDeltaLumaSmoothThresh = 35;

	// Scanning paramters
	sde_param.penaltyValues.uiScanPenaltyNonsmooth = 31; // ps of original simulation
	sde_param.penaltyValues.uiScanPenaltySmooth0 = 0;
	sde_param.penaltyValues.uiScanPenaltySmooth1 = 1;
	sde_param.penaltyValues.uiScanPenaltySmooth2 = 2;

	sde_param.thSmooth.uiDeltaDispLut0 = 2;
	sde_param.thSmooth.uiDeltaDispLut1 = 3;

	// LRC diagonal parameters
	sde_param.thDiag.uiDiagThreshLut0 = 0;
	sde_param.thDiag.uiDiagThreshLut1 = 0;
	sde_param.thDiag.uiDiagThreshLut2 = 2;
	sde_param.thDiag.uiDiagThreshLut3 = 2;
	sde_param.thDiag.uiDiagThreshLut4 = 2;
	sde_param.thDiag.uiDiagThreshLut5 = 2;
	sde_param.thDiag.uiDiagThreshLut6 = 6;


	//open
	er_return = sde_open(&sde_openobj);
	if (er_return != E_OK) {
		goto EOP;
	}
	nvt_dbg(IND, "sde open...\r\n");

	//enable interrupt

	er_return = sde_setMode(&sde_param);
	if (er_return != E_OK) {
		goto EOP;
	}
	nvt_dbg(IND, "sde setMode...done\r\n");

	sde_start();
	nvt_dbg(IND, "sde start...done\r\n");

	sde_waitFlagFrameEnd();
	nvt_dbg(IND, "sde wait frame-end...done\r\n");

	sde_pause();
	nvt_dbg(IND, "sde pause...done\r\n");

	sde_close();
	nvt_dbg(IND, "sde close...done\r\n");

	/* save image */
	fp = filp_open("/mnt/sd/sde_out.raw", O_CREAT | O_WRONLY | O_SYNC, 0);
	if (IS_ERR_OR_NULL(fp)) {
		DBG_ERR("failed in file open:%s\n", "/mnt/sd/yout.raw");
		len = -EFAULT;
		goto EOP;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	len += vfs_write(fp, out_buf_info.va_addr, out_buf_info.size, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
	nvt_dbg(IND, "save image out...done\r\n");

#if 0
	/* save dat */
	fp = filp_open("/mnt/sd/sde_reg.raw", O_CREAT | O_WRONLY | O_SYNC, 0);
	if (IS_ERR_OR_NULL(fp)) {
		DBG_ERR("failed in file open:%s\n", "/mnt/sd/reg.raw");
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	len = vfs_write(fp, (0xfddb0000), 0x48, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
	nvt_dbg(IND, "save reg out...done\r\n");
#endif


EOP:
	/* free buffer */
	frm_free_buf_ddr(in0_buf_info.va_addr);
	frm_free_buf_ddr(in1_buf_info.va_addr);
	frm_free_buf_ddr(out_buf_info.va_addr);
	return len;

#endif
	return 0;
}


int nvt_kdrv_sde_api_test_kdrv(PSDE_INFO pmodule_info, unsigned char argc, char **pargv)
{
#if 0
	frammap_buf_t in0_buf_info = {0};
	frammap_buf_t in1_buf_info = {0};
	frammap_buf_t vol_buf_info = {0};
	frammap_buf_t out_buf_info = {0};
	mm_segment_t old_fs;
	struct file *fp;
	int len = 0;
	int width = 256;
	int height = 192;

	UINT32 id = 0, chip = 0, engine = 0xc001, channel = 0;
	KDRV_SDE_OPENCFG kdrv_sde_open_obj;
	KDRV_SDE_MODE_INFO mode_info;
	KDRV_SDE_IO_INFO io_info;
	KDRV_SDE_COST_PARAM cost_param;
	KDRV_SDE_LUMA_PARAM luma_param;
	KDRV_SDE_PENALTY_PARAM penalty_param;
	KDRV_SDE_SMOOTH_PARAM smooth_param;
	KDRV_SDE_LRC_THRESH_PARAM lrc_param;

	/*get in buffer*/
	in0_buf_info.size = width * height;
	in0_buf_info.align = 64;      ///< address alignment
	in0_buf_info.name = "sde_t_in0";
	in0_buf_info.alloc_type = ALLOC_CACHEABLE;
	if (frm_get_buf_ddr(DDR_ID0, &in0_buf_info)) {
		DBG_ERR("get input buffer fail\n");
		return -EFAULT;
	}
	nvt_dbg(IND, "input0 : va_addr = 0x%08x, phy_addr = 0x%08x\r\n", (UINT32)in0_buf_info.va_addr, (UINT32)in0_buf_info.phy_addr);

	in1_buf_info.size = width * height;
	in1_buf_info.align = 64;      ///< address alignment
	in1_buf_info.name = "sde_t_in1";
	in1_buf_info.alloc_type = ALLOC_CACHEABLE;
	if (frm_get_buf_ddr(DDR_ID0, &in1_buf_info)) {
		DBG_ERR("get input buffer fail\n");
		return -EFAULT;
	}
	nvt_dbg(IND, "input1 : va_addr = 0x%08x, phy_addr = 0x%08x\r\n", (UINT32)in1_buf_info.va_addr, (UINT32)in1_buf_info.phy_addr);

	vol_buf_info.size = ((width * height * 32 * 6) >> 3);
	vol_buf_info.align = 64;      ///< address alignment
	vol_buf_info.name = "sde_t_in2";
	vol_buf_info.alloc_type = ALLOC_CACHEABLE;
	if (frm_get_buf_ddr(DDR_ID0, &vol_buf_info)) {
		DBG_ERR("get input buffer fail\n");
		return -EFAULT;
	}
	nvt_dbg(IND, "input2 : va_addr = 0x%08x, phy_addr = 0x%08x\r\n", (UINT32)vol_buf_info.va_addr, (UINT32)vol_buf_info.phy_addr);

	/*get out buffer*/
	out_buf_info.size = width * height;
	out_buf_info.align = 64;      ///< address alignment
	out_buf_info.name = "sde_t_out";
	out_buf_info.alloc_type = ALLOC_CACHEABLE;
	if (frm_get_buf_ddr(DDR_ID0, &out_buf_info)) {
		frm_free_buf_ddr(out_buf_info.va_addr);
		DBG_ERR("get output buffer fail\n");
		return -EFAULT;
	}
	nvt_dbg(IND, "output : va_addr = 0x%08x, phy_addr = 0x%08x\r\n", (UINT32)out_buf_info.va_addr, (UINT32)out_buf_info.phy_addr);


	/* load image */
	fp = filp_open(pargv[0], O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		DBG_ERR("failed in file open:%s\n", pargv[0]);
		frm_free_buf_ddr(in0_buf_info.va_addr);
		frm_free_buf_ddr(in1_buf_info.va_addr);
		frm_free_buf_ddr(out_buf_info.va_addr);
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	len += vfs_read(fp, in0_buf_info.va_addr, in0_buf_info.size, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);

	/* load image */
	fp = filp_open(pargv[1], O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		DBG_ERR("failed in file open:%s\n", pargv[1]);
		frm_free_buf_ddr(in0_buf_info.va_addr);
		frm_free_buf_ddr(in1_buf_info.va_addr);
		frm_free_buf_ddr(out_buf_info.va_addr);
		len = -EFAULT;
		goto EOP;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	len += vfs_read(fp, in1_buf_info.va_addr, in1_buf_info.size, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);


	/******** sde d2d flow *********/
	DBG_IND("sde d2d flow start test\n");
	id = KDRV_DEV_ID(chip, engine, channel);
	/*open*/
	kdrv_sde_open_obj.sde_clock = 480;
	kdrv_sde_set(id, KDRV_SDE_PARAM_IPL_OPENCFG, (void *)&kdrv_sde_open_obj);
	DBG_IND("sde d2d flow open\n");
	kdrv_sde_open(chip, engine);

	/*set input img parameters*/
	io_info.width  = width;
	io_info.height = height;

	//SDE lofs setting
	io_info.in0_lofs = width;
	io_info.in1_lofs  = width;
	io_info.vol_lofs  = ((width * 32 * 6) >> 3);
	io_info.out_lofs  = width;
	//SDE address
	io_info.in_addr0 = (UINT32)in0_buf_info.va_addr;
	io_info.in_addr1 = (UINT32)in1_buf_info.va_addr;
	io_info.vol_addr = (UINT32)vol_buf_info.va_addr;
	io_info.out_addr = (UINT32)out_buf_info.va_addr;
	kdrv_sde_set(id, KDRV_SDE_PARAM_IPL_IO_INFO, (void *)&io_info);

	mode_info.op_mode = KDRV_SDE_MAX_DISP_31;
	mode_info.inv_sel = KDRV_SDE_INV_0;
	kdrv_sde_set(id, KDRV_SDE_PARAM_IPL_MODE_INFO, (void *)&mode_info);

	// Cost parameters
	cost_param.ad_bound_lower = 5;
	cost_param.ad_bound_upper = 63;
	cost_param.census_bound_lower = 4;
	cost_param.census_bound_upper = 21;
	cost_param.ad_census_ratio = 208;
	kdrv_sde_set(id, KDRV_SDE_PARAM_IQ_COST_PARAM, (void *)&cost_param);


	// Luma parameters
	luma_param.luma_saturated_th = 212;
	luma_param.cost_saturated = 15;
	luma_param.delta_luma_smooth_th = 35;
	kdrv_sde_set(id, KDRV_SDE_PARAM_IQ_LUMA_PARAM, (void *)&luma_param);

	// Scanning paramters
	penalty_param.penalty_nonsmooth = 31;
	penalty_param.penalty_smooth0 = 0;
	penalty_param.penalty_smooth1 = 1;
	penalty_param.penalty_smooth2 = 2;
	kdrv_sde_set(id, KDRV_SDE_PARAM_IQ_PENALTY_PARAM, (void *)&penalty_param);

	// Scanning paramters
	smooth_param.delta_displut0 = 2;
	smooth_param.delta_displut1 = 3;
	kdrv_sde_set(id, KDRV_SDE_PARAM_IQ_SMOOTH_PARAM, (void *)&smooth_param);

	// LRC diagonal parameters
	lrc_param.thresh_lut0 = 0;
	lrc_param.thresh_lut1 = 0;
	lrc_param.thresh_lut2 = 2;
	lrc_param.thresh_lut3 = 2;
	lrc_param.thresh_lut4 = 2;
	lrc_param.thresh_lut5 = 2;
	lrc_param.thresh_lut6 = 6;

	kdrv_sde_set(id, KDRV_SDE_PARAM_IQ_LRC_THRESH_PARAM, (void *)&lrc_param);
	DBG_IND("set sde LRC threshold\n");

	kdrv_sde_fmd_flag = 0;
	kdrv_sde_set(id, KDRV_SDE_PARAM_IPL_ISR_CB, (void *)&KDRV_SDE_ISR_CB);
	DBG_IND("set sde isr cb\n");

	/*trig*/
	DBG_IND("sde trigger\n");
	kdrv_sde_trigger(id, NULL, NULL, NULL);

	/*wait end*/
	while (!kdrv_sde_fmd_flag) {
		msleep(10);
	}

	/*close*/
	kdrv_sde_close(chip, engine);
	DBG_IND("sde d2d flow end\n");
	/******** ipe d2d flow *********/


	/* save image */
	fp = filp_open("/mnt/sd/sde_out.raw", O_CREAT | O_WRONLY | O_SYNC, 0);
	if (IS_ERR_OR_NULL(fp)) {
		DBG_ERR("failed in file open:%s\n", "/mnt/sd/yout.raw");
		len = -EFAULT;
		goto EOP;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	len += vfs_write(fp, out_buf_info.va_addr, out_buf_info.size, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
	nvt_dbg(IND, "save image out...done\r\n");


	/* save image */
	fp = filp_open("/mnt/sd/sde_volout.raw", O_CREAT | O_WRONLY | O_SYNC, 0);
	if (IS_ERR_OR_NULL(fp)) {
		DBG_ERR("failed in file open:%s\n", "/mnt/sd/yout.raw");
		len = -EFAULT;
		goto EOP;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	len += vfs_write(fp, vol_buf_info.va_addr, vol_buf_info.size, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
	nvt_dbg(IND, "save image out...done\r\n");


#if 0
	/* save dat */
	fp = filp_open("/mnt/sd/sde_reg.raw", O_CREAT | O_WRONLY | O_SYNC, 0);
	if (IS_ERR_OR_NULL(fp)) {
		DBG_ERR("failed in file open:%s\n", "/mnt/sd/reg.raw");
		return -EFAULT;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	len = vfs_write(fp, (0xfddb0000), 0x48, &fp->f_pos);
	filp_close(fp, NULL);
	set_fs(old_fs);
	nvt_dbg(IND, "save reg out...done\r\n");
#endif


EOP:
	/* free buffer */
	frm_free_buf_ddr(in0_buf_info.va_addr);
	frm_free_buf_ddr(in1_buf_info.va_addr);
	frm_free_buf_ddr(out_buf_info.va_addr);

	
	return len;
	#endif
	return 0;
}

