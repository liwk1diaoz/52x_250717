/*
    IME module driver

    NT96520 IME module driver.

    @file       ime_osd_base.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#include "ime_reg.h"
#include "ime_int_public.h"

#include "ime_osd_base.h"


static UINT32 ime_osd0_in_addr = 0x0;
static UINT32 ime_osd1_in_addr = 0x0;
static UINT32 ime_osd2_in_addr = 0x0;
static UINT32 ime_osd3_in_addr = 0x0;


#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)

VOID ime_set_osd_cst_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_ds_cst_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_color_key_enable_reg(UINT32 set_en)
{
	T_IME_DATA_STAMP_SET0_POSITION_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_POSITION_REGISTER_OFS);

	arg.bit.ime_ds0_ck_en = set_en;

	IME_SETREG(IME_DATA_STAMP_SET0_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_color_palette_enable_reg(UINT32 set_en)
{
	T_IME_DATA_STAMP_SET0_WEIGHT_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_WEIGHT_REGISTER_OFS);

	arg.bit.ime_ds0_plt_en = set_en;

	IME_SETREG(IME_DATA_STAMP_SET0_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_color_key_mode_reg(UINT32 set_mode)
{
	T_IME_DATA_STAMP_SET0_WEIGHT_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_WEIGHT_REGISTER_OFS);

	arg.bit.ime_ds0_ck_mode = set_mode;

	IME_SETREG(IME_DATA_STAMP_SET0_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_ds_en0 = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_osd_set0_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_ds_en0;
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_image_size_reg(UINT32 size_h, UINT32 size_v)
{
	T_IME_DATA_STAMP_SET0_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_CONTROL_REGISTER0_OFS);

	arg.bit.ime_ds0_hsize = size_h - 1;
	arg.bit.ime_ds0_vsize = size_v - 1;

	IME_SETREG(IME_DATA_STAMP_SET0_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_format_reg(UINT32 set_fmt)
{
	T_IME_DATA_STAMP_SET0_POSITION_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_POSITION_REGISTER_OFS);

	arg.bit.ime_ds0_fmt = set_fmt;

	IME_SETREG(IME_DATA_STAMP_SET0_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_osd_set0_blend_weight_reg(UINT32 wet0, UINT32 wet1)
{
	T_IME_DATA_STAMP_SET0_WEIGHT_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_WEIGHT_REGISTER_OFS);

	arg.bit.ime_ds0_gawet0 = wet0;
	arg.bit.ime_ds0_gawet1 = wet1;

	IME_SETREG(IME_DATA_STAMP_SET0_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_position_reg(UINT32 pos_h, UINT32 pos_v)
{
	T_IME_DATA_STAMP_SET0_POSITION_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_POSITION_REGISTER_OFS);

	arg.bit.ime_ds0_posx = pos_h;
	arg.bit.ime_ds0_posy = pos_v;

	IME_SETREG(IME_DATA_STAMP_SET0_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_color_key_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b)
{
	T_IME_DATA_STAMP_SET0_COLOR_KEY_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_COLOR_KEY_REGISTER_OFS);

	arg.bit.ime_ds0_ck_a = ck_a;
	arg.bit.ime_ds0_ck_r = ck_r;
	arg.bit.ime_ds0_ck_g = ck_g;
	arg.bit.ime_ds0_ck_b = ck_b;

	IME_SETREG(IME_DATA_STAMP_SET0_COLOR_KEY_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_lineoffset_reg(UINT32 lofs)
{
	T_IME_DATA_STAMP_SET0_DMA_LINEOFFSET_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_DMA_LINEOFFSET_REGISTER_OFS);

	arg.bit.ime_ds0_ofsi = (lofs >> 2);

	IME_SETREG(IME_DATA_STAMP_SET0_DMA_LINEOFFSET_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_dma_addr_reg(UINT32 addr)
{
	T_IME_DATA_STAMP_SET0_DMA_ADDRESS_REGISTER arg;

	ime_osd0_in_addr = addr;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_DMA_ADDRESS_REGISTER_OFS);

	arg.bit.ime_ds0_sai = ime_platform_va2pa(ime_osd0_in_addr) >> 2;//(uiaddr >> 2);

	IME_SETREG(IME_DATA_STAMP_SET0_DMA_ADDRESS_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set0_image_size_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	T_IME_DATA_STAMP_SET0_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_CONTROL_REGISTER0_OFS);

	*p_get_size_h = arg.bit.ime_ds0_hsize + 1;
	*p_get_size_v = arg.bit.ime_ds0_vsize + 1;
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set0_lineoffset_reg(UINT32 *p_get_lofs)
{
	T_IME_DATA_STAMP_SET0_DMA_LINEOFFSET_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_DMA_LINEOFFSET_REGISTER_OFS);

	*p_get_lofs = (arg.bit.ime_ds0_ofsi << 2);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set0_dma_addr_reg(UINT32 *p_get_addr)
{
	//T_IME_DATA_STAMP_SET0_DMA_ADDRESS_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_DMA_ADDRESS_REGISTER_OFS);

	*p_get_addr = ime_osd0_in_addr;//(arg.bit.ime_ds0_sai << 2);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set1_color_key_enable_reg(UINT32 set_en)
{
	T_IME_DATA_STAMP_SET1_POSITION_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_POSITION_REGISTER_OFS);

	arg.bit.ime_ds1_ck_en = set_en;

	IME_SETREG(IME_DATA_STAMP_SET1_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set1_color_palette_enable_reg(UINT32 set_en)
{
	T_IME_DATA_STAMP_SET1_WEIGHT_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_WEIGHT_REGISTER_OFS);

	arg.bit.ime_ds1_plt_en = set_en;

	IME_SETREG(IME_DATA_STAMP_SET1_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set1_color_key_mode_reg(UINT32 set_mode)
{
	T_IME_DATA_STAMP_SET1_WEIGHT_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_WEIGHT_REGISTER_OFS);

	arg.bit.ime_ds1_ck_mode = set_mode;

	IME_SETREG(IME_DATA_STAMP_SET1_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------



VOID ime_set_osd_set1_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_ds_en1 = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_osd_set1_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_ds_en1;
}
//------------------------------------------------------------------------------


VOID ime_set_osd_set1_image_size_reg(UINT32 size_h, UINT32 size_v)
{
	T_IME_DATA_STAMP_SET1_CONTROL_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_CONTROL_REGISTER_OFS);

	arg.bit.ime_ds1_hsize = size_h - 1;
	arg.bit.ime_ds1_vsize = size_v - 1;

	IME_SETREG(IME_DATA_STAMP_SET1_CONTROL_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set1_blend_weight_reg(UINT32 wet0, UINT32 wet1)
{
	T_IME_DATA_STAMP_SET1_WEIGHT_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_WEIGHT_REGISTER_OFS);

	arg.bit.ime_ds1_gawet0 = wet0;
	arg.bit.ime_ds1_gawet1 = wet1;

	IME_SETREG(IME_DATA_STAMP_SET1_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set1_format_reg(UINT32 fmt)
{
	T_IME_DATA_STAMP_SET1_POSITION_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_POSITION_REGISTER_OFS);

	arg.bit.ime_ds1_fmt = fmt;

	IME_SETREG(IME_DATA_STAMP_SET1_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set1_position_reg(UINT32 pos_h, UINT32 pos_v)
{
	T_IME_DATA_STAMP_SET1_POSITION_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_POSITION_REGISTER_OFS);

	arg.bit.ime_ds1_posx = pos_h;
	arg.bit.ime_ds1_posy = pos_v;

	IME_SETREG(IME_DATA_STAMP_SET1_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set1_color_key_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b)
{
	T_IME_DATA_STAMP_SET1_COLOR_KEY_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_COLOR_KEY_REGISTER_OFS);

	arg.bit.ime_ds1_ck_a = ck_a;
	arg.bit.ime_ds1_ck_r = ck_r;
	arg.bit.ime_ds1_ck_g = ck_g;
	arg.bit.ime_ds1_ck_b = ck_b;

	IME_SETREG(IME_DATA_STAMP_SET1_COLOR_KEY_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------
VOID ime_set_osd_set1_lineoffset_reg(UINT32 lofs)
{
	T_IME_DATA_STAMP_SET1_DMA_LINEOFFSET_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_DMA_LINEOFFSET_REGISTER_OFS);

	arg.bit.ime_ds1_ofsi = (lofs >> 2);

	IME_SETREG(IME_DATA_STAMP_SET1_DMA_LINEOFFSET_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------
VOID ime_set_osd_set1_dma_addr_reg(UINT32 addr)
{
	T_IME_DATA_STAMP_SET1_DMA_ADDRESS_REGISTER arg;

	ime_osd1_in_addr = addr;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_DMA_ADDRESS_REGISTER_OFS);

	arg.bit.ime_ds1_sai = ime_platform_va2pa(ime_osd1_in_addr) >> 2;//(uiaddr >> 2);

	IME_SETREG(IME_DATA_STAMP_SET1_DMA_ADDRESS_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set1_image_size_reg(UINT32 *p_size_h, UINT32 *p_size_v)
{
	T_IME_DATA_STAMP_SET1_CONTROL_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_CONTROL_REGISTER_OFS);

	*p_size_h = arg.bit.ime_ds1_hsize + 1;
	*p_size_v = arg.bit.ime_ds1_vsize + 1;
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set1_lineoffset_reg(UINT32 *p_lofs)
{
	T_IME_DATA_STAMP_SET1_DMA_LINEOFFSET_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_DMA_LINEOFFSET_REGISTER_OFS);

	*p_lofs = (arg.bit.ime_ds1_ofsi << 2);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set1_dma_addr_reg(UINT32 *p_addr)
{
	//T_IME_DATA_STAMP_SET1_DMA_ADDRESS_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_DMA_ADDRESS_REGISTER_OFS);

	*p_addr = ime_osd1_in_addr;//(arg.bit.ime_ds1_sai << 2);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set2_color_key_enable_reg(UINT32 set_en)
{
	T_IME_DATA_STAMP_SET2_POSITION_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_POSITION_REGISTER_OFS);

	arg.bit.ime_ds2_ck_en = set_en;

	IME_SETREG(IME_DATA_STAMP_SET2_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set2_color_palette_enable_reg(UINT32 set_en)
{
	T_IME_DATA_STAMP_SET2_WEIGHT_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_WEIGHT_REGISTER_OFS);

	arg.bit.ime_ds2_plt_en = set_en;

	IME_SETREG(IME_DATA_STAMP_SET2_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set2_color_key_mode_reg(UINT32 set_mode)
{
	T_IME_DATA_STAMP_SET2_WEIGHT_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_WEIGHT_REGISTER_OFS);

	arg.bit.ime_ds2_ck_mode = set_mode;

	IME_SETREG(IME_DATA_STAMP_SET2_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_osd_set2_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_ds_en2 = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_osd_set2_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_ds_en2;
}
//------------------------------------------------------------------------------


VOID ime_set_osd_set2_image_size_reg(UINT32 size_h, UINT32 size_v)
{
	T_IME_DATA_STAMP_SET2_CONTROL_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_CONTROL_REGISTER_OFS);

	arg.bit.ime_ds2_hsize = size_h - 1;
	arg.bit.ime_ds2_vsize = size_v - 1;

	IME_SETREG(IME_DATA_STAMP_SET2_CONTROL_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set2_blend_weight_reg(UINT32 wet0, UINT32 wet1)
{
	T_IME_DATA_STAMP_SET2_WEIGHT_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_WEIGHT_REGISTER_OFS);

	arg.bit.ime_ds2_gawet0 = wet0;
	arg.bit.ime_ds2_gawet1 = wet1;

	IME_SETREG(IME_DATA_STAMP_SET2_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set2_format_reg(UINT32 fmt)
{
	T_IME_DATA_STAMP_SET2_POSITION_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_POSITION_REGISTER_OFS);

	arg.bit.ime_ds2_fmt = fmt;

	IME_SETREG(IME_DATA_STAMP_SET2_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set2_position_reg(UINT32 pos_h, UINT32 pos_v)
{
	T_IME_DATA_STAMP_SET2_POSITION_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_POSITION_REGISTER_OFS);

	arg.bit.ime_ds2_posx = pos_h;
	arg.bit.ime_ds2_posy = pos_v;

	IME_SETREG(IME_DATA_STAMP_SET2_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------
VOID ime_set_osd_set2_color_key_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b)
{
	T_IME_DATA_STAMP_SET2_COLOR_KEY_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_COLOR_KEY_REGISTER_OFS);

	arg.bit.ime_ds2_ck_a = ck_a;
	arg.bit.ime_ds2_ck_r = ck_r;
	arg.bit.ime_ds2_ck_g = ck_g;
	arg.bit.ime_ds2_ck_b = ck_b;

	IME_SETREG(IME_DATA_STAMP_SET2_COLOR_KEY_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------
VOID ime_set_osd_set2_lineoffset_reg(UINT32 lofs)
{
	T_IME_DATA_STAMP_SET2_DMA_LINEOFFSET_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_DMA_LINEOFFSET_REGISTER_OFS);

	arg.bit.ime_ds2_ofsi = (lofs >> 2);

	IME_SETREG(IME_DATA_STAMP_SET2_DMA_LINEOFFSET_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------
VOID ime_set_osd_set2_dma_addr_reg(UINT32 addr)
{
	T_IME_DATA_STAMP_SET2_DMA_ADDRESS_REGISTER arg;

	ime_osd2_in_addr = addr;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_DMA_ADDRESS_REGISTER_OFS);

	arg.bit.ime_ds2_sai = ime_platform_va2pa(ime_osd2_in_addr) >> 2;//(uiaddr >> 2);

	IME_SETREG(IME_DATA_STAMP_SET2_DMA_ADDRESS_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set2_image_size_reg(UINT32 *p_size_h, UINT32 *p_size_v)
{
	T_IME_DATA_STAMP_SET2_CONTROL_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_CONTROL_REGISTER_OFS);

	*p_size_h = arg.bit.ime_ds2_hsize + 1;
	*p_size_v = arg.bit.ime_ds2_vsize + 1;
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set2_lineoffset_reg(UINT32 *p_lofs)
{
	T_IME_DATA_STAMP_SET2_DMA_LINEOFFSET_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_DMA_LINEOFFSET_REGISTER_OFS);

	*p_lofs = (arg.bit.ime_ds2_ofsi << 2);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set2_dma_addr_reg(UINT32 *p_addr)
{
	//T_IME_DATA_STAMP_SET2_DMA_ADDRESS_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_DMA_ADDRESS_REGISTER_OFS);

	*p_addr = ime_osd2_in_addr;//(arg.bit.ime_ds2_sai << 2);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_color_key_enable_reg(UINT32 set_en)
{
	T_IME_DATA_STAMP_SET3_POSITION_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_POSITION_REGISTER_OFS);

	arg.bit.ime_ds3_ck_en = set_en;

	IME_SETREG(IME_DATA_STAMP_SET3_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_color_palette_enable_reg(UINT32 set_en)
{
	T_IME_DATA_STAMP_SET3_WEIGHT_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_WEIGHT_REGISTER_OFS);

	arg.bit.ime_ds3_plt_en = set_en;

	IME_SETREG(IME_DATA_STAMP_SET3_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_color_key_mode_reg(UINT32 set_mode)
{
	T_IME_DATA_STAMP_SET3_WEIGHT_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_WEIGHT_REGISTER_OFS);

	arg.bit.ime_ds3_ck_mode = set_mode;

	IME_SETREG(IME_DATA_STAMP_SET3_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------



VOID ime_set_osd_set3_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_ds_en3 = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_osd_set3_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_ds_en3;
}
//------------------------------------------------------------------------------


VOID ime_set_osd_set3_image_size_reg(UINT32 size_h, UINT32 size_v)
{
	T_IME_DATA_STAMP_SET3_CONTROL_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_CONTROL_REGISTER_OFS);

	arg.bit.ime_ds3_hsize = size_h - 1;
	arg.bit.ime_ds3_vsize = size_v - 1;

	IME_SETREG(IME_DATA_STAMP_SET3_CONTROL_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_blend_weight_reg(UINT32 wet0, UINT32 wet1)
{
	T_IME_DATA_STAMP_SET3_WEIGHT_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_WEIGHT_REGISTER_OFS);

	arg.bit.ime_ds3_gawet0 = wet0;
	arg.bit.ime_ds3_gawet1 = wet1;

	IME_SETREG(IME_DATA_STAMP_SET3_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_format_reg(UINT32 fmt)
{
	T_IME_DATA_STAMP_SET3_POSITION_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_POSITION_REGISTER_OFS);

	arg.bit.ime_ds3_fmt = fmt;

	IME_SETREG(IME_DATA_STAMP_SET3_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_position_reg(UINT32 pos_h, UINT32 pos_v)
{
	T_IME_DATA_STAMP_SET3_POSITION_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_POSITION_REGISTER_OFS);

	arg.bit.ime_ds3_posx = pos_h;
	arg.bit.ime_ds3_posy = pos_v;

	IME_SETREG(IME_DATA_STAMP_SET3_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_color_key_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b)
{
	T_IME_DATA_STAMP_SET3_COLOR_KEY_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_COLOR_KEY_REGISTER_OFS);

	arg.bit.ime_ds3_ck_a = ck_a;
	arg.bit.ime_ds3_ck_r = ck_r;
	arg.bit.ime_ds3_ck_g = ck_g;
	arg.bit.ime_ds3_ck_b = ck_b;

	IME_SETREG(IME_DATA_STAMP_SET3_COLOR_KEY_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_lineoffset_reg(UINT32 lofs)
{
	T_IME_DATA_STAMP_SET3_DMA_LINEOFFSET_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_DMA_LINEOFFSET_REGISTER_OFS);

	arg.bit.ime_ds3_ofsi = (lofs >> 2);

	IME_SETREG(IME_DATA_STAMP_SET3_DMA_LINEOFFSET_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_dma_addr_reg(UINT32 addr)
{
	T_IME_DATA_STAMP_SET3_DMA_ADDRESS_REGISTER arg;

	ime_osd3_in_addr = addr;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_DMA_ADDRESS_REGISTER_OFS);

	arg.bit.ime_ds3_sai = ime_platform_va2pa(ime_osd3_in_addr) >> 2;//(uiaddr >> 2);

	IME_SETREG(IME_DATA_STAMP_SET3_DMA_ADDRESS_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set3_image_size_reg(UINT32 *p_size_h, UINT32 *p_size_v)
{
	T_IME_DATA_STAMP_SET3_CONTROL_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_CONTROL_REGISTER_OFS);

	*p_size_h = arg.bit.ime_ds3_hsize + 1;
	*p_size_v = arg.bit.ime_ds3_vsize + 1;
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set3_lineoffset_reg(UINT32 *p_lofs)
{
	T_IME_DATA_STAMP_SET3_DMA_LINEOFFSET_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_DMA_LINEOFFSET_REGISTER_OFS);

	*p_lofs = (arg.bit.ime_ds3_ofsi << 2);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set3_dma_addr_reg(UINT32 *p_addr)
{
	//T_IME_DATA_STAMP_SET3_DMA_ADDRESS_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_DMA_ADDRESS_REGISTER_OFS);

	*p_addr = ime_osd3_in_addr; //(arg.bit.ime_ds3_sai << 2);
}
//------------------------------------------------------------------------------


VOID ime_set_osd_cst_coef_reg(UINT32 c0, UINT32 c1, UINT32 c2, UINT32 c3)
{
	T_IME_DATA_STAMP_COLOR_SPACE_TRANSFORM_REGISTER arg;

	arg.reg = IME_GETREG(IME_DATA_STAMP_COLOR_SPACE_TRANSFORM_REGISTER_OFS);

	arg.bit.ime_ds_cst_coef0 = c0;
	arg.bit.ime_ds_cst_coef1 = c1;
	arg.bit.ime_ds_cst_coef2 = c2;
	arg.bit.ime_ds_cst_coef3 = c3;

	IME_SETREG(IME_DATA_STAMP_COLOR_SPACE_TRANSFORM_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_color_palette_mode_reg(UINT32 set_mode)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_ds_plt_sel = set_mode;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_osd_color_palette_lut_reg(UINT32 *p_lut_a, UINT32 *p_lut_r, UINT32 *p_lut_g, UINT32 *p_lut_b)
{
	UINT32 arg_val, i;

	for (i = 0; i < 16; i++) {
		arg_val = 0;
		arg_val = (p_lut_b[i] << 24) + (p_lut_g[i] << 16) + (p_lut_r[i] << 8) + (p_lut_a[i]);

		IME_SETREG(IME_DATA_STAMP_COLOR_PALETTE_REGISTER0_OFS + (i << 2), arg_val);
	}
}
//------------------------------------------------------------------------------

#else


VOID ime_set_osd_cst_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_ds_cst_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_color_key_enable_reg(UINT32 set_en)
{
	//T_IME_DATA_STAMP_SET0_POSITION_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_POSITION_REGISTER_OFS);

	imeg->reg_177.bit.ime_ds0_ck_en = set_en;

	//IME_SETREG(IME_DATA_STAMP_SET0_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_color_palette_enable_reg(UINT32 set_en)
{
	//T_IME_DATA_STAMP_SET0_WEIGHT_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_WEIGHT_REGISTER_OFS);

	imeg->reg_179.bit.ime_ds0_plt_en = set_en;

	//IME_SETREG(IME_DATA_STAMP_SET0_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_color_key_mode_reg(UINT32 set_mode)
{
	//T_IME_DATA_STAMP_SET0_WEIGHT_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_WEIGHT_REGISTER_OFS);

	imeg->reg_179.bit.ime_ds0_ck_mode = set_mode;

	//IME_SETREG(IME_DATA_STAMP_SET0_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_ds_en0 = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_osd_set0_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.bit.ime_ds_en0;
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_image_size_reg(UINT32 size_h, UINT32 size_v)
{
	//T_IME_DATA_STAMP_SET0_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_CONTROL_REGISTER0_OFS);

	imeg->reg_176.bit.ime_ds0_hsize = size_h - 1;
	imeg->reg_176.bit.ime_ds0_vsize = size_v - 1;

	//IME_SETREG(IME_DATA_STAMP_SET0_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_format_reg(UINT32 set_fmt)
{
	//T_IME_DATA_STAMP_SET0_POSITION_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_POSITION_REGISTER_OFS);

	imeg->reg_177.bit.ime_ds0_fmt = set_fmt;

	//IME_SETREG(IME_DATA_STAMP_SET0_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_osd_set0_blend_weight_reg(UINT32 wet0, UINT32 wet1)
{
	//T_IME_DATA_STAMP_SET0_WEIGHT_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_WEIGHT_REGISTER_OFS);

	imeg->reg_179.bit.ime_ds0_gawet0 = wet0;
	imeg->reg_179.bit.ime_ds0_gawet1 = wet1;

	//IME_SETREG(IME_DATA_STAMP_SET0_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_position_reg(UINT32 pos_h, UINT32 pos_v)
{
	//T_IME_DATA_STAMP_SET0_POSITION_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_POSITION_REGISTER_OFS);

	imeg->reg_177.bit.ime_ds0_posx = pos_h;
	imeg->reg_177.bit.ime_ds0_posy = pos_v;

	//IME_SETREG(IME_DATA_STAMP_SET0_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_color_key_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b)
{
	//T_IME_DATA_STAMP_SET0_COLOR_KEY_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_COLOR_KEY_REGISTER_OFS);

	imeg->reg_178.bit.ime_ds0_ck_a = ck_a;
	imeg->reg_178.bit.ime_ds0_ck_r = ck_r;
	imeg->reg_178.bit.ime_ds0_ck_g = ck_g;
	imeg->reg_178.bit.ime_ds0_ck_b = ck_b;

	//IME_SETREG(IME_DATA_STAMP_SET0_COLOR_KEY_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_lineoffset_reg(UINT32 lofs)
{
	//T_IME_DATA_STAMP_SET0_DMA_LINEOFFSET_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_DMA_LINEOFFSET_REGISTER_OFS);

	imeg->reg_180.bit.ime_ds0_ofsi = (lofs >> 2);

	//IME_SETREG(IME_DATA_STAMP_SET0_DMA_LINEOFFSET_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set0_dma_addr_reg(UINT32 addr)
{
	//T_IME_DATA_STAMP_SET0_DMA_ADDRESS_REGISTER arg;

	ime_osd0_in_addr = addr;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_DMA_ADDRESS_REGISTER_OFS);

	imeg->reg_181.bit.ime_ds0_sai = ime_platform_va2pa(ime_osd0_in_addr) >> 2;//(uiaddr >> 2);

	//IME_SETREG(IME_DATA_STAMP_SET0_DMA_ADDRESS_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set0_image_size_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	//T_IME_DATA_STAMP_SET0_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_CONTROL_REGISTER0_OFS);

	*p_get_size_h = imeg->reg_176.bit.ime_ds0_hsize + 1;
	*p_get_size_v = imeg->reg_176.bit.ime_ds0_vsize + 1;
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set0_lineoffset_reg(UINT32 *p_get_lofs)
{
	//T_IME_DATA_STAMP_SET0_DMA_LINEOFFSET_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_DMA_LINEOFFSET_REGISTER_OFS);

	*p_get_lofs = (imeg->reg_180.bit.ime_ds0_ofsi << 2);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set0_dma_addr_reg(UINT32 *p_get_addr)
{
	//T_IME_DATA_STAMP_SET0_DMA_ADDRESS_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET0_DMA_ADDRESS_REGISTER_OFS);

	*p_get_addr = ime_osd0_in_addr;//(arg.bit.ime_ds0_sai << 2);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set1_color_key_enable_reg(UINT32 set_en)
{
	//T_IME_DATA_STAMP_SET1_POSITION_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_POSITION_REGISTER_OFS);

	imeg->reg_183.bit.ime_ds1_ck_en = set_en;

	//IME_SETREG(IME_DATA_STAMP_SET1_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set1_color_palette_enable_reg(UINT32 set_en)
{
	//T_IME_DATA_STAMP_SET1_WEIGHT_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_WEIGHT_REGISTER_OFS);

	imeg->reg_185.bit.ime_ds1_plt_en = set_en;

	//IME_SETREG(IME_DATA_STAMP_SET1_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set1_color_key_mode_reg(UINT32 set_mode)
{
	//T_IME_DATA_STAMP_SET1_WEIGHT_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_WEIGHT_REGISTER_OFS);

	imeg->reg_185.bit.ime_ds1_ck_mode = set_mode;

	//IME_SETREG(IME_DATA_STAMP_SET1_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------



VOID ime_set_osd_set1_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_ds_en1 = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_osd_set1_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.bit.ime_ds_en1;
}
//------------------------------------------------------------------------------


VOID ime_set_osd_set1_image_size_reg(UINT32 size_h, UINT32 size_v)
{
	//T_IME_DATA_STAMP_SET1_CONTROL_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_CONTROL_REGISTER_OFS);

	imeg->reg_182.bit.ime_ds1_hsize = size_h - 1;
	imeg->reg_182.bit.ime_ds1_vsize = size_v - 1;

	//IME_SETREG(IME_DATA_STAMP_SET1_CONTROL_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set1_blend_weight_reg(UINT32 wet0, UINT32 wet1)
{
	//T_IME_DATA_STAMP_SET1_WEIGHT_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_WEIGHT_REGISTER_OFS);

	imeg->reg_185.bit.ime_ds1_gawet0 = wet0;
	imeg->reg_185.bit.ime_ds1_gawet1 = wet1;

	//IME_SETREG(IME_DATA_STAMP_SET1_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set1_format_reg(UINT32 fmt)
{
	//T_IME_DATA_STAMP_SET1_POSITION_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_POSITION_REGISTER_OFS);

	imeg->reg_183.bit.ime_ds1_fmt = fmt;

	//IME_SETREG(IME_DATA_STAMP_SET1_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set1_position_reg(UINT32 pos_h, UINT32 pos_v)
{
	//T_IME_DATA_STAMP_SET1_POSITION_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_POSITION_REGISTER_OFS);

	imeg->reg_183.bit.ime_ds1_posx = pos_h;
	imeg->reg_183.bit.ime_ds1_posy = pos_v;

	//IME_SETREG(IME_DATA_STAMP_SET1_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set1_color_key_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b)
{
	//T_IME_DATA_STAMP_SET1_COLOR_KEY_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_COLOR_KEY_REGISTER_OFS);

	imeg->reg_184.bit.ime_ds1_ck_a = ck_a;
	imeg->reg_184.bit.ime_ds1_ck_r = ck_r;
	imeg->reg_184.bit.ime_ds1_ck_g = ck_g;
	imeg->reg_184.bit.ime_ds1_ck_b = ck_b;

	//IME_SETREG(IME_DATA_STAMP_SET1_COLOR_KEY_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------
VOID ime_set_osd_set1_lineoffset_reg(UINT32 lofs)
{
	//T_IME_DATA_STAMP_SET1_DMA_LINEOFFSET_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_DMA_LINEOFFSET_REGISTER_OFS);

	imeg->reg_186.bit.ime_ds1_ofsi = (lofs >> 2);

	//IME_SETREG(IME_DATA_STAMP_SET1_DMA_LINEOFFSET_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------
VOID ime_set_osd_set1_dma_addr_reg(UINT32 addr)
{
	//T_IME_DATA_STAMP_SET1_DMA_ADDRESS_REGISTER arg;

	ime_osd1_in_addr = addr;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_DMA_ADDRESS_REGISTER_OFS);

	imeg->reg_187.bit.ime_ds1_sai = ime_platform_va2pa(ime_osd1_in_addr) >> 2;//(uiaddr >> 2);

	//IME_SETREG(IME_DATA_STAMP_SET1_DMA_ADDRESS_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set1_image_size_reg(UINT32 *p_size_h, UINT32 *p_size_v)
{
	//T_IME_DATA_STAMP_SET1_CONTROL_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_CONTROL_REGISTER_OFS);

	*p_size_h = imeg->reg_182.bit.ime_ds1_hsize + 1;
	*p_size_v = imeg->reg_182.bit.ime_ds1_vsize + 1;
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set1_lineoffset_reg(UINT32 *p_lofs)
{
	//T_IME_DATA_STAMP_SET1_DMA_LINEOFFSET_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_DMA_LINEOFFSET_REGISTER_OFS);

	*p_lofs = (imeg->reg_186.bit.ime_ds1_ofsi << 2);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set1_dma_addr_reg(UINT32 *p_addr)
{
	//T_IME_DATA_STAMP_SET1_DMA_ADDRESS_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET1_DMA_ADDRESS_REGISTER_OFS);

	*p_addr = ime_osd1_in_addr;//(arg.bit.ime_ds1_sai << 2);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set2_color_key_enable_reg(UINT32 set_en)
{
	//T_IME_DATA_STAMP_SET2_POSITION_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_POSITION_REGISTER_OFS);

	imeg->reg_189.bit.ime_ds2_ck_en = set_en;

	//IME_SETREG(IME_DATA_STAMP_SET2_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set2_color_palette_enable_reg(UINT32 set_en)
{
	//T_IME_DATA_STAMP_SET2_WEIGHT_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_WEIGHT_REGISTER_OFS);

	imeg->reg_191.bit.ime_ds2_plt_en = set_en;

	//IME_SETREG(IME_DATA_STAMP_SET2_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set2_color_key_mode_reg(UINT32 set_mode)
{
	//T_IME_DATA_STAMP_SET2_WEIGHT_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_WEIGHT_REGISTER_OFS);

	imeg->reg_191.bit.ime_ds2_ck_mode = set_mode;

	//IME_SETREG(IME_DATA_STAMP_SET2_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_osd_set2_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_ds_en2 = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_osd_set2_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.bit.ime_ds_en2;
}
//------------------------------------------------------------------------------


VOID ime_set_osd_set2_image_size_reg(UINT32 size_h, UINT32 size_v)
{
	//T_IME_DATA_STAMP_SET2_CONTROL_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_CONTROL_REGISTER_OFS);

	imeg->reg_188.bit.ime_ds2_hsize = size_h - 1;
	imeg->reg_188.bit.ime_ds2_vsize = size_v - 1;

	//IME_SETREG(IME_DATA_STAMP_SET2_CONTROL_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set2_blend_weight_reg(UINT32 wet0, UINT32 wet1)
{
	//T_IME_DATA_STAMP_SET2_WEIGHT_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_WEIGHT_REGISTER_OFS);

	imeg->reg_191.bit.ime_ds2_gawet0 = wet0;
	imeg->reg_191.bit.ime_ds2_gawet1 = wet1;

	//IME_SETREG(IME_DATA_STAMP_SET2_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set2_format_reg(UINT32 fmt)
{
	//T_IME_DATA_STAMP_SET2_POSITION_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_POSITION_REGISTER_OFS);

	imeg->reg_189.bit.ime_ds2_fmt = fmt;

	//IME_SETREG(IME_DATA_STAMP_SET2_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set2_position_reg(UINT32 pos_h, UINT32 pos_v)
{
	//T_IME_DATA_STAMP_SET2_POSITION_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_POSITION_REGISTER_OFS);

	imeg->reg_189.bit.ime_ds2_posx = pos_h;
	imeg->reg_189.bit.ime_ds2_posy = pos_v;

	//IME_SETREG(IME_DATA_STAMP_SET2_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------
VOID ime_set_osd_set2_color_key_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b)
{
	//T_IME_DATA_STAMP_SET2_COLOR_KEY_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_COLOR_KEY_REGISTER_OFS);

	imeg->reg_190.bit.ime_ds2_ck_a = ck_a;
	imeg->reg_190.bit.ime_ds2_ck_r = ck_r;
	imeg->reg_190.bit.ime_ds2_ck_g = ck_g;
	imeg->reg_190.bit.ime_ds2_ck_b = ck_b;

	//IME_SETREG(IME_DATA_STAMP_SET2_COLOR_KEY_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------
VOID ime_set_osd_set2_lineoffset_reg(UINT32 lofs)
{
	//T_IME_DATA_STAMP_SET2_DMA_LINEOFFSET_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_DMA_LINEOFFSET_REGISTER_OFS);

	imeg->reg_192.bit.ime_ds2_ofsi = (lofs >> 2);

	//IME_SETREG(IME_DATA_STAMP_SET2_DMA_LINEOFFSET_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------
VOID ime_set_osd_set2_dma_addr_reg(UINT32 addr)
{
	//T_IME_DATA_STAMP_SET2_DMA_ADDRESS_REGISTER arg;

	ime_osd2_in_addr = addr;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_DMA_ADDRESS_REGISTER_OFS);

	imeg->reg_193.bit.ime_ds2_sai = ime_platform_va2pa(ime_osd2_in_addr) >> 2;//(uiaddr >> 2);

	//IME_SETREG(IME_DATA_STAMP_SET2_DMA_ADDRESS_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set2_image_size_reg(UINT32 *p_size_h, UINT32 *p_size_v)
{
	//T_IME_DATA_STAMP_SET2_CONTROL_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_CONTROL_REGISTER_OFS);

	*p_size_h = imeg->reg_188.bit.ime_ds2_hsize + 1;
	*p_size_v = imeg->reg_188.bit.ime_ds2_vsize + 1;
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set2_lineoffset_reg(UINT32 *p_lofs)
{
	//T_IME_DATA_STAMP_SET2_DMA_LINEOFFSET_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_DMA_LINEOFFSET_REGISTER_OFS);

	*p_lofs = (imeg->reg_192.bit.ime_ds2_ofsi << 2);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set2_dma_addr_reg(UINT32 *p_addr)
{
	//T_IME_DATA_STAMP_SET2_DMA_ADDRESS_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET2_DMA_ADDRESS_REGISTER_OFS);

	*p_addr = ime_osd2_in_addr;//(arg.bit.ime_ds2_sai << 2);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_color_key_enable_reg(UINT32 set_en)
{
	//T_IME_DATA_STAMP_SET3_POSITION_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_POSITION_REGISTER_OFS);

	imeg->reg_195.bit.ime_ds3_ck_en = set_en;

	//IME_SETREG(IME_DATA_STAMP_SET3_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_color_palette_enable_reg(UINT32 set_en)
{
	//T_IME_DATA_STAMP_SET3_WEIGHT_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_WEIGHT_REGISTER_OFS);

	imeg->reg_197.bit.ime_ds3_plt_en = set_en;

	//IME_SETREG(IME_DATA_STAMP_SET3_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_color_key_mode_reg(UINT32 set_mode)
{
	//T_IME_DATA_STAMP_SET3_WEIGHT_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_WEIGHT_REGISTER_OFS);

	imeg->reg_197.bit.ime_ds3_ck_mode = set_mode;

	//IME_SETREG(IME_DATA_STAMP_SET3_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------



VOID ime_set_osd_set3_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_ds_en3 = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_osd_set3_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.bit.ime_ds_en3;
}
//------------------------------------------------------------------------------


VOID ime_set_osd_set3_image_size_reg(UINT32 size_h, UINT32 size_v)
{
	//T_IME_DATA_STAMP_SET3_CONTROL_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_CONTROL_REGISTER_OFS);

	imeg->reg_194.bit.ime_ds3_hsize = size_h - 1;
	imeg->reg_194.bit.ime_ds3_vsize = size_v - 1;

	//IME_SETREG(IME_DATA_STAMP_SET3_CONTROL_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_blend_weight_reg(UINT32 wet0, UINT32 wet1)
{
	//T_IME_DATA_STAMP_SET3_WEIGHT_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_WEIGHT_REGISTER_OFS);

	imeg->reg_197.bit.ime_ds3_gawet0 = wet0;
	imeg->reg_197.bit.ime_ds3_gawet1 = wet1;

	//IME_SETREG(IME_DATA_STAMP_SET3_WEIGHT_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_format_reg(UINT32 fmt)
{
	//T_IME_DATA_STAMP_SET3_POSITION_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_POSITION_REGISTER_OFS);

	imeg->reg_195.bit.ime_ds3_fmt = fmt;

	//IME_SETREG(IME_DATA_STAMP_SET3_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_position_reg(UINT32 pos_h, UINT32 pos_v)
{
	//T_IME_DATA_STAMP_SET3_POSITION_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_POSITION_REGISTER_OFS);

	imeg->reg_195.bit.ime_ds3_posx = pos_h;
	imeg->reg_195.bit.ime_ds3_posy = pos_v;

	//IME_SETREG(IME_DATA_STAMP_SET3_POSITION_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_color_key_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b)
{
	//T_IME_DATA_STAMP_SET3_COLOR_KEY_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_COLOR_KEY_REGISTER_OFS);

	imeg->reg_196.bit.ime_ds3_ck_a = ck_a;
	imeg->reg_196.bit.ime_ds3_ck_r = ck_r;
	imeg->reg_196.bit.ime_ds3_ck_g = ck_g;
	imeg->reg_196.bit.ime_ds3_ck_b = ck_b;

	//IME_SETREG(IME_DATA_STAMP_SET3_COLOR_KEY_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_lineoffset_reg(UINT32 lofs)
{
	//T_IME_DATA_STAMP_SET3_DMA_LINEOFFSET_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_DMA_LINEOFFSET_REGISTER_OFS);

	imeg->reg_198.bit.ime_ds3_ofsi = (lofs >> 2);

	//IME_SETREG(IME_DATA_STAMP_SET3_DMA_LINEOFFSET_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_set3_dma_addr_reg(UINT32 addr)
{
	//T_IME_DATA_STAMP_SET3_DMA_ADDRESS_REGISTER arg;

	ime_osd3_in_addr = addr;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_DMA_ADDRESS_REGISTER_OFS);

	imeg->reg_199.bit.ime_ds3_sai = ime_platform_va2pa(ime_osd3_in_addr) >> 2;//(uiaddr >> 2);

	//IME_SETREG(IME_DATA_STAMP_SET3_DMA_ADDRESS_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set3_image_size_reg(UINT32 *p_size_h, UINT32 *p_size_v)
{
	//T_IME_DATA_STAMP_SET3_CONTROL_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_CONTROL_REGISTER_OFS);

	*p_size_h = imeg->reg_194.bit.ime_ds3_hsize + 1;
	*p_size_v = imeg->reg_194.bit.ime_ds3_vsize + 1;
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set3_lineoffset_reg(UINT32 *p_lofs)
{
	//T_IME_DATA_STAMP_SET3_DMA_LINEOFFSET_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_DMA_LINEOFFSET_REGISTER_OFS);

	*p_lofs = (imeg->reg_198.bit.ime_ds3_ofsi << 2);
}
//------------------------------------------------------------------------------

VOID ime_get_osd_set3_dma_addr_reg(UINT32 *p_addr)
{
	//T_IME_DATA_STAMP_SET3_DMA_ADDRESS_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_SET3_DMA_ADDRESS_REGISTER_OFS);

	*p_addr = ime_osd3_in_addr; //(arg.bit.ime_ds3_sai << 2);
}
//------------------------------------------------------------------------------


VOID ime_set_osd_cst_coef_reg(UINT32 c0, UINT32 c1, UINT32 c2, UINT32 c3)
{
	//T_IME_DATA_STAMP_COLOR_SPACE_TRANSFORM_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DATA_STAMP_COLOR_SPACE_TRANSFORM_REGISTER_OFS);

	imeg->reg_200.bit.ime_ds_cst_coef0 = c0;
	imeg->reg_200.bit.ime_ds_cst_coef1 = c1;
	imeg->reg_200.bit.ime_ds_cst_coef2 = c2;
	imeg->reg_200.bit.ime_ds_cst_coef3 = c3;

	//IME_SETREG(IME_DATA_STAMP_COLOR_SPACE_TRANSFORM_REGISTER_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_osd_color_palette_mode_reg(UINT32 set_mode)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_ds_plt_sel = set_mode;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_osd_color_palette_lut_reg(UINT32 *p_lut_a, UINT32 *p_lut_r, UINT32 *p_lut_g, UINT32 *p_lut_b)
{
	UINT32 arg_val, i;

	for (i = 0; i < 16; i++) {
		arg_val = 0;
		arg_val = (p_lut_b[i] << 24) + (p_lut_g[i] << 16) + (p_lut_r[i] << 8) + (p_lut_a[i]);

		IME_SETREG(IME_DATA_STAMP_COLOR_PALETTE_REGISTER0_OFS + (i << 2), arg_val);
	}
}
//------------------------------------------------------------------------------


#endif

