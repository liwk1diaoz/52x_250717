/**
    @brief Header file of hdal gximage module.\n
    This file contains the functions which is related to hdal gximage in the chip.

    @file hd_gximage.h

    @ingroup gximage

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _HD_GXIMAGE_H_
#define _HD_GXIMAGE_H_

#define GFX_USER_VERSION 20181022

#define GFX_MAX_PLANE_NUM                 3
#define GFX_MAX_MULTI_OUT_NUM             3

#define MAX_JOB_IN_A_LIST 8

typedef enum {
	GFX_USER_CMD_NULL,
	GFX_USER_CMD_NULL1,
	GFX_USER_CMD_NULL2,
	GFX_USER_CMD_NULL3,
	GFX_USER_CMD_INIT,
	GFX_USER_CMD_UNINIT,
	GFX_USER_CMD_COPY,
	GFX_USER_CMD_DMA_COPY,
	GFX_USER_CMD_SCALE,
	GFX_USER_CMD_ROTATE,
	GFX_USER_CMD_COLOR_TRANSFORM,
	GFX_USER_CMD_DRAW_LINE,
	GFX_USER_CMD_DRAW_RECT,
	GFX_USER_CMD_ARITHMETIC,
	GFX_USER_CMD_JOB_LIST,
	GFX_USER_CMD_ISE_SCALE_Y8,
	GFX_USER_CMD_AFFINE,
	GFX_USER_CMD_I8_COLORKEY,
	GFX_USER_CMD_SCALE_LINK_LIST_BUF,
	GFX_USER_CMD_SCALE_LINK_LIST,
	GFX_USER_CMD_ALPHA_BLEND,
	GFX_USER_CMD_RAW_GRAPHIC,
	GFX_USER_CMD_FORCE_USE_PHYSICAL_ADDRESS,
	GFX_USER_CMD_DRAW_LINE_BY_CPU,
	GFX_USER_CMD_DRAW_MASK,
	GFX_USER_CMD_MAX,
	ENUM_DUMMY4WORD(GFX_USER_CMD)
} GFX_USER_CMD;

typedef enum _GFX_GRPH_ENGINE {
	GFX_GRPH_ENGINE_1 = 0,
	GFX_GRPH_ENGINE_2,
	GFX_GRPH_ENGINE_MAX,
	ENUM_DUMMY4WORD(GFX_GRPH_ENGINE)
} GFX_GRPH_ENGINE;

typedef enum {
	GFX_ARITH_BIT_NULL,
	GFX_ARITH_BIT_8,
	GFX_ARITH_BIT_16,
	GFX_ARITH_BIT_MAX,
	ENUM_DUMMY4WORD(GFX_ARITH_BIT)
} GFX_ARITH_BIT;

typedef enum {
	GFX_ARITH_OP_NULL,
	GFX_ARITH_OP_PLUS,
	GFX_ARITH_OP_MINUS,
	GFX_ARITH_OP_MULTIPLY,
	GFX_ARITH_OP_MAX,
	ENUM_DUMMY4WORD(GFX_ARITH_OP)
} GFX_ARITH_OP;

typedef enum {
	GFX_ROTATE_ANGLE_NULL,
	GFX_ROTATE_ANGLE_90,
	GFX_ROTATE_ANGLE_180,
	GFX_ROTATE_ANGLE_270,
	GFX_ROTATE_ANGLE_MIRROR_X,
	GFX_ROTATE_ANGLE_MIRROR_Y,
	GFX_ROTATE_ANGLE_MAX,
	ENUM_DUMMY4WORD(GFX_ROTATE_ANGLE)
} GFX_ROTATE_ANGLE;

typedef enum _GFX_SCALE_METHOD {
	GFX_SCALE_METHOD_NULL,
	GFX_SCALE_METHOD_BICUBIC,
	GFX_SCALE_METHOD_BILINEAR,
	GFX_SCALE_METHOD_NEAREST,
	GFX_SCALE_METHOD_INTEGRATION,
	GFX_SCALE_METHOD_MAX,
	ENUM_DUMMY4WORD(GFX_SCALE_METHOD)
} GFX_SCALE_METHOD;

typedef enum _GFX_ISE_ENGINE {
	GFX_ISE_ENGINE_1 = 0,
	GFX_ISE_ENGINE_2,
	GFX_ISE_ENGINE_MAX,
	ENUM_DUMMY4WORD(GFX_ISE_ENGINE)
} GFX_ISE_ENGINE;

typedef enum _GFX_RECT_TYPE {
	GFX_RECT_TYPE_NULL,
	GFX_RECT_TYPE_SOLID,
	GFX_RECT_TYPE_HOLLOW,
	GFX_RECT_TYPE_MAX,
	ENUM_DUMMY4WORD(GFX_RECT_TYPE)
} GFX_RECT_TYPE;

typedef struct _GFX_IMG_BUF {
	UINT32                     w;
	UINT32                     h;
	VDO_PXLFMT                 format;
	UINT32                     p_phy_addr[GFX_MAX_PLANE_NUM];
	UINT32                     lineoffset[GFX_MAX_PLANE_NUM];
	UINT32                     palette[16];
} GFX_IMG_BUF;

typedef struct _GFX_COPY {
	GFX_IMG_BUF                src_img;
	GFX_IMG_BUF                dst_img;
	INT32                      src_x;
	INT32                      src_y;
	INT32                      src_w;
	INT32                      src_h;
	INT32                      dst_x;
	INT32                      dst_y;
	UINT32                     colorkey;
	UINT32                     alpha;
	UINT32                     flush;
	GFX_GRPH_ENGINE            engine;
} GFX_COPY;

typedef struct _GFX_DMA_COPY {
	UINT32                     p_src;
	UINT32                     p_dst;
	UINT32                     length;
} GFX_DMA_COPY;

typedef struct _GFX_SCALE {
	GFX_IMG_BUF                src_img;
	GFX_IMG_BUF                dst_img;
	INT32                      src_x;
	INT32                      src_y;
	INT32                      src_w;
	INT32                      src_h;
	INT32                      dst_x;
	INT32                      dst_y;
	INT32                      dst_w;
	INT32                      dst_h;
	GFX_SCALE_METHOD           method;
	INT32                      flush;
} GFX_SCALE;

typedef struct _GFX_ROTATE {
	GFX_IMG_BUF                src_img;
	GFX_IMG_BUF                dst_img;
	INT32                      src_x;
	INT32                      src_y;
	INT32                      src_w;
	INT32                      src_h;
	INT32                      dst_x;
	INT32                      dst_y;
	UINT32                     angle;
	UINT32                     flush;
} GFX_ROTATE;

typedef struct _GFX_COLOR_TRANSFORM {
	GFX_IMG_BUF                src_img;
	GFX_IMG_BUF                dst_img;
	UINT32                     p_tmp_buf;
	UINT32                     tmp_buf_size;
	UINT32                     p_lookup_table[GFX_MAX_PLANE_NUM];
} GFX_COLOR_TRANSFORM;

typedef struct _GFX_DRAW_LINE {
	GFX_IMG_BUF                dst_img;
	UINT32                     color;
	UINT32                     start_x;
	UINT32                     start_y;
	UINT32                     end_x;
	UINT32                     end_y;
	UINT32                     thickness;
	UINT32                     flush;
} GFX_DRAW_LINE;

typedef struct _GFX_DRAW_RECT {
	GFX_IMG_BUF                dst_img;
	GFX_RECT_TYPE              type;
	UINT32                     color;
	UINT32                     x;
	UINT32                     y;
	UINT32                     w;
	UINT32                     h;
	UINT32                     thickness;
	UINT32                     flush;
} GFX_DRAW_RECT;

typedef struct _GFX_ARITHMETIC {
	UINT32                     p_op1;
	UINT32                     p_op2;
	UINT32                     p_out;
	UINT32                     size;
	GFX_ARITH_OP               operation;
	GFX_ARITH_BIT              bits;
} GFX_ARITHMETIC;

typedef struct _GFX_ISE_SCALE_Y8 {
	INT32                      src_w;
	INT32                      src_h;
	INT32                      dst_w;
	INT32                      dst_h;
	UINT32                     p_src;
	UINT32                     src_loff;
	UINT32                     p_dst;
	UINT32                     dst_loff;
	GFX_SCALE_METHOD           method;
	GFX_ISE_ENGINE             engine;
} GFX_ISE_SCALE_Y8;

typedef struct _GFX_AFFINE {
	INT32                      w;
	INT32                      h;
	INT32                      plane_num;
	UINT32                     p_src[GFX_MAX_PLANE_NUM];
	UINT32                     src_loff[GFX_MAX_PLANE_NUM];
	UINT32                     p_dst[GFX_MAX_PLANE_NUM];
	UINT32                     dst_loff[GFX_MAX_PLANE_NUM];
	UINT32                     uvpack[GFX_MAX_PLANE_NUM];
	float                      fCoeffA[GFX_MAX_PLANE_NUM];
	float                      fCoeffB[GFX_MAX_PLANE_NUM];
	float                      fCoeffC[GFX_MAX_PLANE_NUM];
	float                      fCoeffD[GFX_MAX_PLANE_NUM];
	float                      fCoeffE[GFX_MAX_PLANE_NUM];
	float                      fCoeffF[GFX_MAX_PLANE_NUM];
} GFX_AFFINE;

typedef struct _GFX_SCALE_LINK_LIST_BUF {
	UINT32                     p_addr;
	UINT32                     length;
} GFX_SCALE_LINK_LIST_BUF;

typedef struct _GFX_SCALE_DMA_FLUSH {
	GFX_IMG_BUF                src_img;
	GFX_IMG_BUF                dst_img;
	INT32                      src_x;
	INT32                      src_y;
	INT32                      src_w;
	INT32                      src_h;
	INT32                      dst_x;
	INT32                      dst_y;
	INT32                      dst_w;
	INT32                      dst_h;
	GFX_SCALE_METHOD           method;
	int                        in_buf_flush;
	int                        out_buf_flush;
} GFX_SCALE_DMA_FLUSH;

typedef struct _GFX_ALPHA_BLEND {
	GFX_IMG_BUF                src_img;
	GFX_IMG_BUF                dst_img;
	INT32                      dst_x;
	INT32                      dst_y;
	UINT32                     p_alpha;
	UINT32                     flush;
} GFX_ALPHA_BLEND;

typedef struct _GFX_RAW_GRAPHIC {
	UINT32                     engine;
	UINT32                     p_addr;
	UINT32                     length;
	UINT32                     flush;
} GFX_RAW_GRAPHIC;

typedef struct _GFX_FORCE_USE_PHYSICAL_ADDRESS {
	UINT32                     force_physical_address;
} GFX_FORCE_USE_PHYSICAL_ADDRESS;

typedef struct _GFX_DRAW_MASK {
	GFX_IMG_BUF                dst_img;
	GFX_RECT_TYPE              type;
	UINT32                     color;
	UINT32                     x[4];
	UINT32                     y[4];
	UINT32                     thickness;
	UINT32                     flush;
} GFX_DRAW_MASK;

typedef struct _GFX_USER_DATA {
	UINT32                                     version;
	GFX_USER_CMD                               cmd;
	union {
		GFX_COPY                               copy;
		GFX_DMA_COPY                           dma_copy;
		GFX_SCALE                              scale;
		GFX_ROTATE                             rotate;
		GFX_COLOR_TRANSFORM                    color_transform;
		GFX_DRAW_LINE                          draw_line;
		GFX_DRAW_RECT                          draw_rect;
		GFX_ARITHMETIC                         arithmetic;
		GFX_ISE_SCALE_Y8                       ise_scale_y8;
		GFX_AFFINE                             affine;
		GFX_SCALE_LINK_LIST_BUF                scale_link_list_buf;
		GFX_SCALE_DMA_FLUSH                    ise_scale_dma_flush;
		GFX_ALPHA_BLEND                        alpha_blend;
		GFX_RAW_GRAPHIC                        raw_graphic;
		GFX_FORCE_USE_PHYSICAL_ADDRESS         force_use_physical_address;
		GFX_DRAW_MASK                          mask;
	} data;  
} GFX_USER_DATA;
typedef enum _GFX_ENG_ID {
	GFX_ENG_ID1 = 0,
	GFX_ENG_ID2,
	ENUM_DUMMY4WORD(GFX_ENG_ID)
} GFX_ENG_ID;

typedef enum _GFX_GRPH_CMD {
	GFX_GRPH_CMD_ROT_90,
	GFX_GRPH_CMD_ROT_270,
	GFX_GRPH_CMD_ROT_180,
	GFX_GRPH_CMD_HRZ_FLIP,
	GFX_GRPH_CMD_VTC_FLIP,
	GFX_GRPH_CMD_HRZ_FLIP_ROT_90,
	GFX_GRPH_CMD_HRZ_FLIP_ROT_270,
	GFX_GRPH_CMD_ROT_0 = 7,
	GFX_GRPH_CMD_VCOV = 8,
	GFX_GRPH_CMD_GOPMAX,

	GFX_GRPH_CMD_A_COPY = 0x10000,
	GFX_GRPH_CMD_PLUS_SHF,
	GFX_GRPH_CMD_MINUS_SHF,
	GFX_GRPH_CMD_COLOR_EQ,
	GFX_GRPH_CMD_COLOR_LE,
	GFX_GRPH_CMD_A_AND_B,
	GFX_GRPH_CMD_A_OR_B,
	GFX_GRPH_CMD_A_XOR_B,
	GFX_GRPH_CMD_TEXT_COPY,
	GFX_GRPH_CMD_TEXT_AND_A,
	GFX_GRPH_CMD_TEXT_OR_A,
	GFX_GRPH_CMD_TEXT_XOR_A,
	GFX_GRPH_CMD_TEXT_AND_AB,
	GFX_GRPH_CMD_BLENDING,
	GFX_GRPH_CMD_ACC,
	GFX_GRPH_CMD_MULTIPLY_DIV,
	GFX_GRPH_CMD_PACKING,
	GFX_GRPH_CMD_DEPACKING,
	GFX_GRPH_CMD_TEXT_MUL,
	GFX_GRPH_CMD_PLANE_BLENDING,
	GFX_GRPH_CMD_1D_LUT = GFX_GRPH_CMD_PLANE_BLENDING + 3,
	GFX_GRPH_CMD_2D_LUT,
	GFX_GRPH_CMD_RGBYUV_BLEND,
	GFX_GRPH_CMD_RGBYUV_COLORKEY,
	GFX_GRPH_CMD_RGB_INVERT,
	GFX_GRPH_CMD_AOPMAX,


	GFX_GRPH_CMD_MINUS_SHF_ABS = 0x10000 + 0x80,
	GFX_GRPH_CMD_COLOR_MR,
	GFX_GRPH_CMD_COLOR_FILTER,

	ENUM_DUMMY4WORD(GFX_GRPH_CMD)
} GFX_GRPH_CMD;

typedef enum _GRPH_GFX_FORMAT {
	GFX_GRPH_FORMAT_1BIT,
	GFX_GRPH_FORMAT_4BITS,
	GFX_GRPH_FORMAT_8BITS,
	GFX_GRPH_FORMAT_16BITS,
	GFX_GRPH_FORMAT_16BITS_UVPACK,
	GFX_GRPH_FORMAT_16BITS_UVPACK_U,
	GFX_GRPH_FORMAT_16BITS_UVPACK_V,
	GFX_GRPH_FORMAT_16BITS_RGB565,
	GFX_GRPH_FORMAT_32BITS,
	GFX_GRPH_FORMAT_32BITS_ARGB8888_RGB,
	GFX_GRPH_FORMAT_32BITS_ARGB8888_A,
	GFX_GRPH_FORMAT_16BITS_ARGB1555_RGB,
	GFX_GRPH_FORMAT_16BITS_ARGB1555_A,
	GFX_GRPH_FORMAT_16BITS_ARGB4444_RGB,
	GFX_GRPH_FORMAT_16BITS_ARGB4444_A,

	GFX_GRPH_FORMAT_PALETTE_1BIT,
	GFX_GRPH_FORMAT_PALETTE_2BITS,
	GFX_GRPH_FORMAT_PALETTE_4BITS,

	ENUM_DUMMY4WORD(GFX_GRPH_FORMAT)
} GFX_GRPH_FORMAT;

typedef enum _GFX_GRPH_IMG_ID {
	GFX_GRPH_IMG_ID_A,
	GFX_GRPH_IMG_ID_B,
	GFX_GRPH_IMG_ID_C,

	ENUM_DUMMY4WORD(GFX_GRPH_IMG_ID)
} GFX_GRPH_IMG_ID;

typedef enum _GFX_GRPH_YUV_FMT {
	GFX_GRPH_YUV_422,
	GFX_GRPH_YUV_411,

	ENUM_DUMMY4WORD(GFX_GRPH_YUV_FMT)
} GFX_GRPH_YUV_FMT;

typedef enum _GFX_GRPH_UV_SUBSAMPLE {
	GFX_GRPH_UV_COCITED,
	GFX_GRPH_UV_CENTERED,

	ENUM_DUMMY4WORD(GFX_GRPH_UV_SUBSAMPLE)
} GFX_GRPH_UV_SUBSAMPLE;

typedef enum {
	GFX_GRPH_MOSAIC_FMT_NORMAL,
	GFX_GRPH_MOSAIC_FMT_YUV444,

	ENUM_DUMMY4WORD(GFX_GRPH_MOSAIC_FMT)
} GFX_GRPH_MOSAIC_FMT;

typedef enum _GFX_GRPH_PROPERTY_ID {
	GFX_GRPH_PROPERTY_ID_NORMAL,
	GFX_GRPH_PROPERTY_ID_U,
	GFX_GRPH_PROPERTY_ID_V,
	GFX_GRPH_PROPERTY_ID_R,
	GFX_GRPH_PROPERTY_ID_G,
	GFX_GRPH_PROPERTY_ID_B,
	GFX_GRPH_PROPERTY_ID_A,

	GFX_GRPH_PROPERTY_ID_ACC_SKIPCTRL,
	GFX_GRPH_PROPERTY_ID_ACC_FULL_FLAG,
	GFX_GRPH_PROPERTY_ID_PIXEL_CNT,
	GFX_GRPH_PROPERTY_ID_VALID_PIXEL_CNT,
	GFX_GRPH_PROPERTY_ID_ACC_RESULT,
	GFX_GRPH_PROPERTY_ID_ACC_RESULT2,

	GFX_GRPH_PROPERTY_ID_LUT_BUF,

	GFX_GRPH_PROPERTY_ID_YUVFMT,
	GFX_GRPH_PROPERTY_ID_ALPHA0_INDEX,
	GFX_GRPH_PROPERTY_ID_ALPHA1_INDEX,
	GFX_GRPH_PROPERTY_ID_INVRGB,
	GFX_GRPH_PROPERTY_ID_PAL_BUF,

	GFX_GRPH_PROPERTY_ID_QUAD_PTR,
	GFX_GRPH_PROPERTY_ID_QUAD_INNER_PTR,
	GFX_GRPH_PROPERTY_ID_MOSAIC_SRC_FMT,

	GFX_GRPH_PROPERTY_ID_UV_SUBSAMPLE,

	ENUM_DUMMY4WORD(GFX_GRPH_PROPERTY_ID)
} GFX_GRPH_PROPERTY_ID;

typedef enum _GFX_GRPH_INOUT_ID {
	GFX_GRPH_INOUT_ID_IN_A,
	GFX_GRPH_INOUT_ID_IN_A_U,
	GFX_GRPH_INOUT_ID_IN_A_V,

	GFX_GRPH_INOUT_ID_IN_B,
	GFX_GRPH_INOUT_ID_IN_B_U,
	GFX_GRPH_INOUT_ID_IN_B_V,

	GFX_GRPH_INOUT_ID_OUT_C,
	GFX_GRPH_INOUT_ID_OUT_C_U,
	GFX_GRPH_INOUT_ID_OUT_C_V,

	ENUM_DUMMY4WORD(GFX_GRPH_INOUT_ID)
} GFX_GRPH_INOUT_ID;

typedef enum _GFX_GRPH_COLORKEY_FLTSEL {
	GFX_GRPH_COLORKEY_FLTSEL_0,
	GFX_GRPH_COLORKEY_FLTSEL_1,
	GFX_GRPH_COLORKEY_FLTSEL_2,

	ENUM_DUMMY4WORD(GFX_GRPH_COLORKEY_FLTSEL)
} GFX_GRPH_COLORKEY_FLTSEL;

typedef enum _GFX_GRPH_COLORKEY_FLTMODE {
	GFX_GRPH_COLORKEY_FLTMODE_BOTH,
	GFX_GRPH_COLORKEY_FLTMODE_OR,
	GFX_GRPH_COLORKEY_FLTMODE_1ST,
	GFX_GRPH_COLORKEY_FLTMODE_2ND,

	ENUM_DUMMY4WORD(GFX_GRPH_COLORKEY_FLTMODE)
} GFX_GRPH_COLORKEY_FLTMODE;

#define GFX_GRPH_PLUS_PROPTY(SHF)   ((SHF)&0x3)
#define GFX_GRPH_SUB_PROPTY(SHF)    ((SHF)&0x3)
#define GFX_GRPH_COLOR_KEY_PROPTY(COLOR) ((COLOR)&0xFF)
#define GFX_GRPH_COLOR_FLT_PROPTY(FLT_SEL, FLT_MODE, COLORKEY) ((((FLT_SEL)&0x3)<<30) | 0x20000000 | (((FLT_MODE)&0x3)<<26) | ((COLORKEY)&0xFFFFFF))
#define GFX_GRPH_BLD_WA_WB_THR(WA, WB, THR)	(((WA)&0xFF)|(((WB)&0xFF)<<8)|(((THR)&0xFF)<<16))
#define GFX_GRPH_ACC_PROPTY(MODE, THR) ((((MODE)&0x1)<<31) | ((THR)&0xFFF))
#define GFX_GRPH_MULT_PROPTY(DITHER_EN, SQ1, SQ2, SHF)   ((((DITHER_EN)&0x1)<<31) | (((SQ1)&0xF)<<24) | (((SQ2)&0x7FFF)<<8) | ((SHF)&0xF))
#define GFX_GRPH_TEXTMUL_BYTE         0x00000000
#define GFX_GRPH_TEXTMUL_HWORD        0x80000000
#define GFX_GRPH_TEXTMUL_SIGNED       0x40000000
#define GFX_GRPH_TEXTMUL_UNSIGNED     0x00000000
#define GFX_GRPH_TEXT_MULT_PROPTY(PRECISION, SIGNED, CTEX, SHF)  (((PRECISION)&GFX_GRPH_TEXTMUL_HWORD) | ((SIGNED)&GFX_GRPH_TEXTMUL_SIGNED) | (((CTEX)&0xFF)<<4) | ((SHF)&0xF))
#define GFX_GRPH_ENABLE_COLOR_CONST     0x10000
#define GFX_GRPH_ENABLE_ALPHA_CONST     0x20000
#define GFX_GRPH_ENABLE_ALPHA_SRC       0x40000
#define GFX_GRPH_ENABLE_ALPHA_FROM_C    0x40000
#define GFX_GRPH_ENABLE_ALPHA_FROM_A    0x00000
#define GFX_GRPH_COLOR_ALPHA_MASK       (GFX_GRPH_ENABLE_COLOR_CONST|GFX_GRPH_ENABLE_ALPHA_CONST)
#define GFX_GRPH_COLOR_CONST(x)         ((((x) & 0xFF) <<  0) | GFX_GRPH_ENABLE_COLOR_CONST)
#define GFX_GRPH_ALPHA_CONST(x)         ((((x) & 0xFF) <<  8) | GFX_GRPH_ENABLE_ALPHA_CONST)
#define GFX_GRPH_CONST_MASK             0x0000FFFF
#define GFX_GRPH_RGB_INV_PROPTY(threshold, inv_alpha)  (((threshold)&0xFF) | ((inv_alpha)<<8))

typedef enum _GFX_GRPH_INOPCTL {
	GFX_GRPH_INOP_NONE,
	GFX_GRPH_INOP_INVERT,
	GFX_GRPH_INOP_SHIFTR_ADD,
	GFX_GRPH_INOP_SHIFTL_ADD,
	GFX_GRPH_INOP_SHIFTL_SUB,

	ENUM_DUMMY4WORD(GFX_GRPH_INOPCTL)
} GFX_GRPH_INOPCTL;

typedef enum _GFX_GRPH_OUTOPCTL {
	GFX_GRPH_OUTOP_SHF,
	GFX_GRPH_OUTOP_INVERT,
	GFX_GRPH_OUTOP_ADD,
	GFX_GRPH_OUTOP_SUB,
	GFX_GRPH_OUTOP_ABS,

	ENUM_DUMMY4WORD(GFX_GRPH_OUTOPCTL)
} GFX_GRPH_OUTOPCTL;

typedef struct _GFX_GRPH_INOUTOP {
	UINT32                     en;
	GFX_GRPH_INOUT_ID   id;
	UINT32                     op;
	UINT32                     shifts;
	UINT32                     constant;
} GFX_GRPH_INOUTOP, *PGFX_GRPH_INOUTOP;

typedef struct _GFX_GRPH_IMG {
	GFX_GRPH_IMG_ID  img_id;
	UINT32                  dram_addr;
	UINT32                  lineoffset;
	UINT32                  width;
	UINT32                  height;
	GFX_GRPH_INOUTOP inoutop;
} GFX_GRPH_IMG, *PGFX_GRPH_IMG;

typedef struct _GFX_GRPH_CKEYFILTER {
	UINT32   FKEY1;
	UINT32   FKEY2;
	UINT32   RKEY;
} GFX_GRPH_CKEYFILTER, *PGFX_GRPH_CKEYFILTER;

typedef struct _GFX_GRPH_POINT_DESC {
	INT32   x;
	INT32   y;
} GFX_GRPH_POINT_DESC, *PGFX_GRPH_POINT_DESC;

typedef struct _GFX_GRPH_QUAD_DESC {
	BOOL   blend_en;
	UINT32 alpha;

	UINT32 mosaic_width;
	UINT32 mosaic_height;

	GFX_GRPH_POINT_DESC top_left;
	GFX_GRPH_POINT_DESC top_right;
	GFX_GRPH_POINT_DESC bottom_right;
	GFX_GRPH_POINT_DESC bottom_left;
} GFX_GRPH_QUAD_DESC, *PGFX_GRPH_QUAD_DESC;

typedef struct _GFX_GRPH_PROPERTY {
	UINT32 en;
	UINT32 id; //should be GFX_GRPH_PROPERTY_ID
	union {
		UINT32                    property;
		GFX_GRPH_QUAD_DESC quad;
	} data;
} GFX_GRPH_PROPERTY, *PGFX_GRPH_PROPERTY;

typedef struct _GFX_GRPH_TRIGGER_PARAM {
	GFX_GRPH_CMD         command;
	GFX_GRPH_FORMAT	    format;
	GFX_GRPH_IMG         images[3];
	GFX_GRPH_PROPERTY    property[5];
	GFX_GRPH_CKEYFILTER  ckeyfilter;
} GFX_GRPH_TRIGGER_PARAM;

#if defined(__FREERTOS)
extern int nvt_gfx_init(void);
extern int nvt_gfx_ioctl(int f, unsigned int cmd, void *arg);
#endif
#endif //_HD_GXIMAGE_H_
