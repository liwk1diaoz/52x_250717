/*
    @file       jpeg_int.c
    @ingroup    mIDrvCodec_JPEG

    @brief      Internal driver file for JPEG module
				This file is the internal driver file that define the internal driver API for JPEG module.

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/


#if defined __UITRON || defined __ECOS
#include    "Type.h"//a header for basic variable type
#include    "dma.h"//header for Dma(cache handle)

#elif defined __KERNEL__
#include    <linux/dma-mapping.h> // header file Dma(cache handle)
#include    "kwrap/type.h"
#else

#include    "kwrap/type.h"
#endif
//#include "math.h"

#include "../include/jpeg.h"
#include "jpeg_int.h"
#include "jpeg_reg.h"
#include "jpeg_platform.h"

#if defined __KERNEL__

UINT32 IOADDR_JPEG_REG_BASE;


void jpeg_set_baseaddr(UINT32 addr)
{
	IOADDR_JPEG_REG_BASE = addr;

	//printk("[%s] = 0x%x! \n", __func__, (unsigned int)(IOADDR_JPEG_REG_BASE));

}
#endif

/**
    @addtogroup mIDrvCodec_JPEG
*/
//@{

/*
    Init JPEG Configuration

    Init JPEG Configuration.

    @return void
*/
void jpeg_reg_init(void)
{
	T_JPG_CFG       jpg_cfg;
	T_JPG_SCALE     jpg_scale;
	T_JPG_BS        jpg_bs;

	// ========================================================
	// Offset 0x04, Configure Register
	// ========================================================
	jpg_cfg.reg                 = 0;

	// Encode mode
	jpg_cfg.bit.dec_mod         = JPEG_MODE_ENCODE;
	// Disable restart marker
	jpg_cfg.bit.rstr_en         = 0;
	// Disable slice mode
	jpg_cfg.bit.slice_en        = 0;
	jpg_cfg.bit.slice_adr_renew = 0;
	// 2h11 format
	jpg_cfg.bit.img_fmt_comp    = JPEG_COLOR_COMP_3CH;
	jpg_cfg.bit.img_fmt_uv      = JPEG_MCU_UV_FMT_1COMP;
	jpg_cfg.bit.img_fmt_y       = JPEG_MCU_Y_FMT_2COMP;
	jpg_cfg.bit.img_fmt_mcu     = JPEG_MCU_SLOPE_HORIZONTAL;
	// Ouput 0xD9
	jpg_cfg.bit.flush_eof_en    = 1;
	// Disable 411 to 2h11 tranform
	jpg_cfg.bit.fmt_tran_en     = 0;
	// DC out must be UV packed
	jpg_cfg.bit.dc_out_uvpack_en = 1;

	JPEG_SETREG(JPG_CFG_OFS, jpg_cfg.reg);

	// ========================================================
	// Offset 0x18, Interrupt Control Register
	// ========================================================
	// Disable all interrupts
	JPEG_SETREG(JPG_INT_EN_OFS, 0);

	// ========================================================
	// Offset 0x1C, Interrupt Status Register
	// ========================================================
	// Clear all interrupt status
	JPEG_SETREG(JPG_INT_STA_OFS, JPEG_INT_ALL);

	// ========================================================
	// Offset 0x20, Image & Scaling Control Register
	// ========================================================
	jpg_scale.reg               = 1;
	// Disable decode scaling
	jpg_scale.bit.scale_en      = 0;
	// Disable encode DC out
	jpg_scale.bit.dc_out_en     = 0;
	JPEG_SETREG(JPG_SCALE_OFS, jpg_scale.reg);

	// ========================================================
	// Offset 0x3C, Bit-stream Control Register
	// ========================================================
	// Enable bit-stream output
	jpg_bs.reg                  = 0;
	jpg_bs.bit.bs_out_en        = 1;
	JPEG_SETREG(JPG_BS_OFS, jpg_bs.reg);

	// ========================================================
	// Offset 0x84, Decode Crop Register
	// ========================================================
	// Disable crop function
	JPEG_SETREG(JPG_CROP_ST_OFS, 0);
}

/*
    Set decode base index or mincode data

    Set decode base index or mincode data to the controller.

    @param[in] puiHuffTabLAC    Huffman table for luminance AC
    @param[in] puiHuffTabLDC    Huffman table for luminance DC
    @param[in] puiHuffTabCAC    Huffman table for chrominance AC
    @param[in] puiHuffTabCDC    Huffman table for chrominance DC

    @return void
*/
void jpeg_setbaseidx_mincode(UINT8 *phufftab_lumac, UINT8 *phufftab_lumdc, UINT8 *phufftab_chrac, UINT8 *phufftab_chrdc)
{
	UINT32      i;
	UINT32      reg;
	UINT8       *huffman_tbl;

	huffman_tbl = phufftab_lumac;
	for (i = 0; i < (0x10 / 4); i++) {
		reg = *huffman_tbl++;
		reg |= (*huffman_tbl++) << 8;
		reg |= (*huffman_tbl++) << 16;
		reg |= (*huffman_tbl++) << 24;
		JPEG_SETREG(JPG_LUT_DATA_OFS, reg);
	}

	huffman_tbl = phufftab_lumdc;
	for (i = 0; i < (0x10 / 4); i++) {
		reg = *huffman_tbl++;
		reg |= (*huffman_tbl++) << 8;
		reg |= (*huffman_tbl++) << 16;
		reg |= (*huffman_tbl++) << 24;
		JPEG_SETREG(JPG_LUT_DATA_OFS, reg);
	}

	huffman_tbl = phufftab_chrac;
	for (i = 0; i < (0x10 / 4); i++) {
		reg = *huffman_tbl++;
		reg |= (*huffman_tbl++) << 8;
		reg |= (*huffman_tbl++) << 16;
		reg |= (*huffman_tbl++) << 24;
		JPEG_SETREG(JPG_LUT_DATA_OFS, reg);
	}

	huffman_tbl = phufftab_chrdc;
	for (i = 0; i < (0x10 / 4); i++) {
		reg = *huffman_tbl++;
		reg |= (*huffman_tbl++) << 8;
		reg |= (*huffman_tbl++) << 16;
		reg |= (*huffman_tbl++) << 24;
		JPEG_SETREG(JPG_LUT_DATA_OFS, reg);
	}
}

/*
    Build decode base index or mincode data from "Huffman table definition length".

    Build decode base index or mincode data from "Huffman table definition length".

    @param[in] puiHuffTab       Huffman table definition length
    @param[out] puiMinTab       Mincode table
    @param[out] puiBaseTab      Base index table

    @return void
*/
void jpeg_buildbaseidx_mincode(UINT8 *phuff_tab, UINT8 *pmin_tab, UINT8 *pbase_tab)
{
	UINT32      i, huff_base, code_number, bit_offset;

	huff_base = 0;
	bit_offset  = 0;
	for (i = 0; i < 16; i++) {
		*pmin_tab++    = (huff_base & 0xFF);
		//DBG_WRN("MinTab = 0x%x\r\n", (huff_base & 0xFF));
		*pbase_tab++   = bit_offset;
		//DBG_WRN("BaseTab = 0x%x\r\n", (unsigned int)((bit_offset)));

		code_number    = *phuff_tab++;
		bit_offset     += code_number;
		huff_base     = (huff_base + code_number) << 1 ;
	}
}

//@}
