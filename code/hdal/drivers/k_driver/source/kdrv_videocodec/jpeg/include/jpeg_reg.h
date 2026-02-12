/*
    Header file for JPEG module register.

    This file is the header file that define register for JPEG module.

    @file       jpeg_reg.h
    @ingroup    mIDrvCodec_JPEG
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/

#ifndef _JEPG_REG_H
#define _JEPG_REG_H

#ifdef __KERNEL__

#include "mach/rcw_macro.h"
#include "kwrap/type.h"
#else
#if defined(__FREERTOS)
#include "rcw_macro.h"
#include "kwrap/type.h"
#else
#include "DrvCommon.h"
#endif
#endif


/*
    @addtogroup mIDrvCodec_JPEG
*/
//@{

// Control Register
#define JPG_CTRL_OFS                    0x00
REGDEF_BEGIN(JPG_CTRL)
REGDEF_BIT(jpg_act, 1)
REGDEF_BIT(, 2)
REGDEF_BIT(jpg_srst, 1)
REGDEF_BIT(, 28)
REGDEF_END(JPG_CTRL)

// Configure Register
#define JPG_CFG_OFS                     0x04
REGDEF_BEGIN(JPG_CFG)
REGDEF_BIT(dec_mod, 1)
REGDEF_BIT(rstr_en, 1)
REGDEF_BIT(rstr_sync_en, 1)
REGDEF_BIT(, 5)
REGDEF_BIT(slice_en, 1)
REGDEF_BIT(slice_adr_renew, 1)
REGDEF_BIT(, 4)
REGDEF_BIT(img_fmt_comp, 1)
REGDEF_BIT(img_fmt_uv, 1)
REGDEF_BIT(img_fmt_y, 2)
REGDEF_BIT(img_fmt_mcu, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(flush_eof_en, 1)
REGDEF_BIT(, 3)
REGDEF_BIT(fmt_tran_en, 1)
REGDEF_BIT(new_dma_sel, 1)
REGDEF_BIT(rot_sel, 2)
REGDEF_BIT(err_detect_en, 1)
REGDEF_BIT(, 2)
REGDEF_BIT(dc_out_uvpack_en, 1)
REGDEF_END(JPG_CFG)

// 0x08, Reserved Register

// Restart Interval Register
#define JPG_RSTR_OFS                    0x0C
REGDEF_BEGIN(JPG_RSTR)
REGDEF_BIT(rstr_num, 16)
REGDEF_BIT(, 16)
REGDEF_END(JPG_RSTR)

// LUT Control Register
#define JPG_LUT_OFS                     0x10
REGDEF_BEGIN(JPG_LUT)
REGDEF_BIT(tbl_adr, 10)
REGDEF_BIT(, 2)
REGDEF_BIT(tbl_sel, 4)
REGDEF_BIT(tbl_autoi, 1)
REGDEF_BIT(, 3)
REGDEF_BIT(tbl_mod, 1)
REGDEF_BIT(, 11)
REGDEF_END(JPG_LUT)

// LUT Data Access Port Register
#define JPG_LUT_DATA_OFS                0x14
REGDEF_BEGIN(JPG_LUT_DATA)
REGDEF_BIT(tbl_data, 32)
REGDEF_END(JPG_LUT_DATA)

// Interrup Control Register
#define JPG_INT_EN_OFS                  0x18
REGDEF_BEGIN(JPG_INT_EN)
REGDEF_BIT(int_frame_end_en, 1)
REGDEF_BIT(int_slice_done_en, 1)
REGDEF_BIT(int_dec_err_en, 1)
REGDEF_BIT(int_bsbuf_end_en, 1)
REGDEF_BIT(, 28)
REGDEF_END(JPG_INT_EN)

// Interrupt Status Register
#define JPG_INT_STA_OFS                 0x1C
REGDEF_BEGIN(JPG_INT_STA)
REGDEF_BIT(int_frame_end, 1)
REGDEF_BIT(int_slice_done, 1)
REGDEF_BIT(int_dec_err, 1)
REGDEF_BIT(int_bsbuf_end, 1)
REGDEF_BIT(, 28)
REGDEF_END(JPG_INT_STA)

// Image & Scaling Control Register
#define JPG_SCALE_OFS                   0x20
REGDEF_BEGIN(JPG_SCALE)
REGDEF_BIT(, 1)
REGDEF_BIT(scale_en, 1)
REGDEF_BIT(scale_mod, 2)
REGDEF_BIT(dc_out_en, 1)
REGDEF_BIT(, 3)
REGDEF_BIT(dc_out_hor, 2)
REGDEF_BIT(, 2)
REGDEF_BIT(dc_out_ver, 2)
REGDEF_BIT(, 18)
REGDEF_END(JPG_SCALE)

// Slice Control Register
#define JPG_SLICE_OFS                   0x24
REGDEF_BEGIN(JPG_SLICE)
REGDEF_BIT(slice_num, 13)
REGDEF_BIT(, 18)
REGDEF_BIT(slice_go, 1)
REGDEF_END(JPG_SLICE)

// Image Data Size Register
#define JPG_DATA_SIZE_OFS                0x28
REGDEF_BEGIN(JPG_DATA_SIZE)
REGDEF_BIT(resol_xmcu, 13)
REGDEF_BIT(, 3)
REGDEF_BIT(resol_ymcu, 13)
REGDEF_BIT(, 3)
REGDEF_END(JPG_DATA_SIZE)

// MCU Count Register
#define JPG_MCU_OFS                     0x2C
REGDEF_BEGIN(JPG_MCU)
REGDEF_BIT(mcu_num, 26)
REGDEF_BIT(, 6)
REGDEF_END(JPG_MCU)

// 0x30 ~ 0x38, Reserved Registers

// Bitstream Control Register
#define JPG_BS_OFS                      0x3C
REGDEF_BEGIN(JPG_BS)
REGDEF_BIT(, 5)
REGDEF_BIT(bs_out_en, 1)
REGDEF_BIT(, 2)
REGDEF_BIT(bs_dma_burst, 1)
REGDEF_BIT(, 23)
REGDEF_END(JPG_BS)

// Bitstream DMA Starting Address Register
#define JPG_BS_STADR_OFS                0x40
REGDEF_BEGIN(JPG_BS_STADR)
REGDEF_BIT(bs_stadr, 31)
REGDEF_BIT(bs_valid, 1)
REGDEF_END(JPG_BS_STADR)

// Bitstream DMA Buffer Size Register
#define JPG_BS_BUFLEN_OFS               0x44
REGDEF_BEGIN(JPG_BS_BUFLEN)
REGDEF_BIT(bs_buflen, 25)
REGDEF_BIT(, 7)
REGDEF_END(JPG_BS_BUFLEN)

// Bitstream DMA Current Address Register
#define JPG_BS_CURADR_R_OFS             0x48
REGDEF_BEGIN(JPG_BS_CURADR_R)
REGDEF_BIT(bs_curadr_r, 31)
REGDEF_BIT(, 1)
REGDEF_END(JPG_BS_CURADR_R)

// Bitstream Total Length Register
#define JPG_BS_CODECLEN_OFS             0x4C
REGDEF_BEGIN(JPG_BS_CODECLEN)
REGDEF_BIT(codec_bs_len_out, 32)
REGDEF_END(JPG_BS_CODECLEN)

// Version Register
#define JPG_VERSION_OFS                 0x50
REGDEF_BEGIN(JPG_VERSION)
REGDEF_BIT(DE_VERSION, 32)
REGDEF_END(JPG_VERSION)

// Image Data Y Starting Address Register
#define JPG_Y_STADR_OFS                 0x54
REGDEF_BEGIN(JPG_Y_STADR)
REGDEF_BIT(y_stadr, 31)
REGDEF_BIT(, 1)
REGDEF_END(JPG_Y_STADR)

// Image Data UV Starting Address Register
#define JPG_UV_STADR_OFS                0x58
REGDEF_BEGIN(JPG_UV_STADR)
REGDEF_BIT(uv_stadr, 31)
REGDEF_BIT(, 1)
REGDEF_END(JPG_UV_STADR)

// 0x5C, Reserved Register

// Image Data Y Line Offset Register
#define JPG_Y_LOFS_OFS                  0x60
REGDEF_BEGIN(JPG_Y_LOFS)
REGDEF_BIT(y_lofs, 17)
REGDEF_BIT(, 15)
REGDEF_END(JPG_Y_LOFS)

// Image Data UV Line Offset Register
#define JPG_UV_LOFS_OFS                 0x64
REGDEF_BEGIN(JPG_UV_LOFS)
REGDEF_BIT(uv_lofs, 17)
REGDEF_BIT(, 15)
REGDEF_END(JPG_UV_LOFS)

// 0x68, Reserved Register

// DC Output Data Y Starting Address Register
#define JPG_DC_Y_STADR_OFS              0x6C
REGDEF_BEGIN(JPG_DC_Y_STADR)
REGDEF_BIT(dc_y_stadr, 31)
REGDEF_BIT(, 1)
REGDEF_END(JPG_DC_Y_STADR)

// DC Output Data UV Starting Address Register
#define JPG_DC_UV_STADR_OFS             0x70
REGDEF_BEGIN(JPG_DC_UV_STADR)
REGDEF_BIT(dc_uv_stadr, 31)
REGDEF_BIT(, 1)
REGDEF_END(JPG_DC_UV_STADR)

// 0x74, Reserved Register

// DC Output Data Y Line Offset Register
#define JPG_DC_Y_LOFS_OFS               0x78
REGDEF_BEGIN(JPG_DC_Y_LOFS)
REGDEF_BIT(dc_y_lofs, 17)
REGDEF_BIT(, 15)
REGDEF_END(JPG_DC_Y_LOFS)

// DC Output Data UV Line Offset Register
#define JPG_DC_UV_LOFS_OFS              0x7C
REGDEF_BEGIN(JPG_DC_UV_LOFS)
REGDEF_BIT(dc_uv_lofs, 17)
REGDEF_BIT(, 15)
REGDEF_END(JPG_DC_UV_LOFS)

// 0x80, Reserved Register

// Decode Crop Starting Point Register
#define JPG_CROP_ST_OFS                 0x84
REGDEF_BEGIN(JPG_CROP_ST)
REGDEF_BIT(crop_stxmcu, 13)
REGDEF_BIT(, 3)
REGDEF_BIT(crop_stymcu, 13)
REGDEF_BIT(, 2)
REGDEF_BIT(crop_en, 1)
REGDEF_END(JPG_CROP_ST)

// Decode Crop Size Register
#define JPG_CROP_SIZE_OFS               0x88
REGDEF_BEGIN(JPG_CROP_SIZE)
REGDEF_BIT(crop_xmcu, 13)
REGDEF_BIT(, 3)
REGDEF_BIT(crop_ymcu, 13)
REGDEF_BIT(, 3)
REGDEF_END(JPG_CROP_SIZE)

// Rho Value of 1/8Q Register
#define JPG_RHO_1_8_OFS                 0x8C
REGDEF_BEGIN(JPG_RHO_1_8)
REGDEF_BIT(rho_1_8, 32)
REGDEF_END(JPG_RHO_1_8)

// Rho Value of 1/4Q Register
#define JPG_RHO_1_4_OFS                 0x90
REGDEF_BEGIN(JPG_RHO_1_4)
REGDEF_BIT(rho_1_4, 32)
REGDEF_END(JPG_RHO_1_4)

// Rho Value of 1/4Q Register
#define JPG_RHO_1_2_OFS                 0x94
REGDEF_BEGIN(JPG_RHO_1_2)
REGDEF_BIT(rho_1_2, 32)
REGDEF_END(JPG_RHO_1_2)

// Rho Value of Q Register
#define JPG_RHO_1_OFS                   0x98
REGDEF_BEGIN(JPG_RHO_1)
REGDEF_BIT(rho_1, 32)
REGDEF_END(JPG_RHO_1)

// Rho Value of 2Q Register
#define JPG_RHO_2_OFS                   0x9C
REGDEF_BEGIN(JPG_RHO_2)
REGDEF_BIT(rho_2, 32)
REGDEF_END(JPG_RHO_2)

// Rho Value of 4Q Register
#define JPG_RHO_4_OFS                   0xA0
REGDEF_BEGIN(JPG_RHO_4)
REGDEF_BIT(rho_4, 32)
REGDEF_END(JPG_RHO_4)

// Rho Value of 8Q Register
#define JPG_RHO_8_OFS                   0xA4
REGDEF_BEGIN(JPG_RHO_8)
REGDEF_BIT(rho_8, 32)
REGDEF_END(JPG_RHO_8)

// Cycle Count Register
#define JPG_CYCLE_CNT_OFS               0xA8
REGDEF_BEGIN(JPG_CYCLE_CNT)
REGDEF_BIT(cycle_cnt, 32)
REGDEF_END(JPG_CYCLE_CNT)

// Error Status Register
#define JPG_ERR_STS_OFS                 0xAC
REGDEF_BEGIN(JPG_ERR_STS)
REGDEF_BIT(err_eoi_marker, 1)
REGDEF_BIT(, 3)
REGDEF_BIT(err_filled_byte, 1)
REGDEF_BIT(, 3)
REGDEF_BIT(err_stuffed_byte, 1)
REGDEF_BIT(, 23)
REGDEF_END(JPG_ERR_STS)

// Image DMA Block Axis Status Register
#define JPG_IMGDMA_AXIS_OFS             0xB0
REGDEF_BEGIN(JPG_IMGDMA_AXIS)
REGDEF_BIT(imgdma_nxb, 14)
REGDEF_BIT(, 2)
REGDEF_BIT(imgdma_nyb, 13)
REGDEF_BIT(, 3)
REGDEF_END(JPG_IMGDMA_AXIS)

// Bitstream Checksum Register
#define JPG_CHECKSUM_BS_OFS             0xB4
REGDEF_BEGIN(JPG_CHECKSUM_BS)
REGDEF_BIT(checksum_bs, 32)
REGDEF_END(JPG_CHECKSUM_BS)

// YUV Checksum Register
#define JPG_CHECKSUM_YUV_OFS            0xB8
REGDEF_BEGIN(JPG_CHECKSUM_YUV)
REGDEF_BIT(checksum_yuv, 32)
REGDEF_END(JPG_CHECKSUM_YUV)

// Debug Register
#define JPG_DEBUG_OFS                   0xBC
REGDEF_BEGIN(JPG_DEBUG)
REGDEF_BIT(checksum_dc_en, 1)
REGDEF_BIT(debug_select_1, 4)
REGDEF_BIT(debug_select_2, 3)
REGDEF_BIT(, 24)
REGDEF_END(JPG_DEBUG)

//@}

#endif
