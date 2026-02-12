#if defined(__LINUX)
#else
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#endif
#include "kwrap/error_no.h"
#include <kwrap/task.h>
#include "gximage/gximage.h"
#include "gximage/gximage_internal.h"
#include "kdrv_gfx2d/kdrv_ise.h"
#include "kdrv_gfx2d/kdrv_ise_lmt.h"
#include "kdrv_gfx2d/kdrv_grph.h"
#include "kdrv_gfx2d/kdrv_grph_lmt.h"
#include "comm/hwclock.h"
#include <plat/top.h>

#define COLOR_RGBA5551_GET_B(c)       (CVALUE)(((c) & 0x0000001f) >> 0) ///< get B channel value of RGBA5551 color
#define COLOR_RGBA5551_GET_G(c)       (CVALUE)(((c) & 0x000003e0) >> 5)  ///< get G channel value of RGBA5551 color
#define COLOR_RGBA5551_GET_R(c)       (CVALUE)(((c) & 0x00007c00) >> 10)  ///< get R channel value of RGBA5551 color
#define COLOR_RGB444_GET_B(c)         (CVALUE)(((c) & 0x0000000f) >> 0) ///< get B channel value of RGB444 color
#define COLOR_RGB444_GET_G(c)         (CVALUE)(((c) & 0x000000f0) >> 4)  ///< get G channel value of RGB444 color
#define COLOR_RGB444_GET_R(c)         (CVALUE)(((c) & 0x00000f00) >> 8)  ///< get R channel value of RGB444 color
#define COLOR_RGB565_GET_B(c)         (CVALUE)(((c) & 0x0000001f) >> 0) ///< get B channel value of RGB5658 color
#define COLOR_RGB565_GET_G(c)         (CVALUE)(((c) & 0x000007e0) >> 5)  ///< get G channel value of RGB5658 color
#define COLOR_RGB565_GET_R(c)         (CVALUE)(((c) & 0x0000f800) >> 11)  ///< get R channel value of RGB5658 color

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__        gximage
#define __DBGLVL__                NVT_DBG_WRN
#include "kwrap/debug.h"
unsigned int gximage_debug_level = __DBGLVL__;
///////////////////////////////////////////////////////////////////////////////

/*
#define USE_GRPH    1   // graphic engine is included in FPGA
#if (_FPGA_EMULATION_ == ENABLE)
#define USE_ISE     0   // ISE     engine is included in FPGA
#define USE_IME     0   // IME     engine is included in FPGA
#else
#define USE_ISE     1   // ISE     engine is included in FPGA
#define USE_IME     1   // IME     engine is included in FPGA
#endif
#define USE_FD      1   // FD      engine is included in FPGA
*/
#define USE_GRPH    1   // graphic
#define USE_ISE     1   // ISE
#define USE_IME     1   // IME
#define USE_FD      0   // FD
#define USE_ROTATE  0   // ROTATE
#define USE_VIRT_COORD  0   // ROTATE

#if USE_ROTATE
#include "kdrv_gfx2d/kdrv_rotation.h"
#endif

#define DUMP_SCALE_RAW  0


//#define MAKE_ALIGN(s, bytes)    ((((UINT32)(s))+((bytes)-1)) & ~((UINT32)((bytes)-1)))
//#define MAKE_ALIGN_S(s, bytes)  (((UINT32)(s)) & ~((UINT32)((bytes)-1)))
#define Not_16_OP   0
#define Use_16_OP   1
#define NO_SWAP     0
#define NEW_GRPH_API     1
//#define GRPH_ROT_W_ALIGN 16
//#define GRPH_ROT_H_ALIGN 8
//#define GRPH_ROT_LINEOFF 16


#define GXIMAGE_FALG_INITED           MAKEFOURCC('V','R','A','W') ///< a key value 'V','R','A','W' for indicating image initial.
// Virutal Coordinate
#define SCALE_UNIT_1X                 0x00010000
#define SCALE_UNIT_SHIFT              16


// border frame
#define FILTER_MODE_1_CHANNEL   ((UINT32)0x0<<29)
#define FILTER_MODE_3_CHANNEL   ((UINT32)0x1<<29)

#define MATCH_RULE_Y1_AND_Y2    ((UINT32)0x0<<26)
#define MATCH_RULE_Y1_OR_Y2     ((UINT32)0x1<<26)
#define MATCH_RULE_Y1_ONLY      ((UINT32)0x2<<26)
#define MATCH_RULE_Y2_ONLY      ((UINT32)0x3<<26)

#define COMPARE_RULE_L_L     ((UINT32)0x00<<30) // FKEY1 < KEY < FKEY2
#define COMPARE_RULE_LE_L    ((UINT32)0x01<<30) // FKEY1 <= KEY < FKEY2
#define COMPARE_RULE_L_LE    ((UINT32)0x02<<30) // FKEY1 < KEY <= FKEY2

#ifndef MIN
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#endif

typedef UINT8   CVALUE; ///< single color value
#define COLOR_YUVA_GET_A(c)         (CVALUE)(((c) & 0xff000000) >> 24) ///< get A channel value of YUVA color
#define COLOR_YUVA_GET_V(c)         (CVALUE)(((c) & 0x00ff0000) >> 16) ///< get V channel value of YUVA color
#define COLOR_YUVA_GET_U(c)         (CVALUE)(((c) & 0x0000ff00) >> 8) ///< get U channel value of YUVA color
#define COLOR_YUVA_GET_Y(c)         (CVALUE)(((c) & 0x000000ff) >> 0) ///< get Y channel value of YUVA color
#define COLOR_RGBD_GET_B(c)         (CVALUE)(((c) & 0x000000ff) >> 0) ///< get B channel value of RGBD color
#define COLOR_RGBD_GET_G(c)         (CVALUE)(((c) & 0x0000ff00) >> 8) ///< get G channel value of RGBD color
#define COLOR_RGBD_GET_R(c)         (CVALUE)(((c) & 0x00ff0000) >> 16) ///< get R channel value of RGBD color

typedef struct {
	UINT8   *p_src;
	UINT8   *p_dst;
	UINT32  input_width;
	UINT32  input_height;
	UINT32  input_offset;
	UINT32  output_width;
	UINT32  output_height;
	UINT32  output_offset;
} GXIMG_JPEG_SCALE_UP_DOWN;

//extern UINT32 g_ime_clk ;
//extern UINT32 g_ise_clk ;
//extern UINT32 g_fde_clk;

static UINT32 g_scale_method = GXIMG_SCALE_AUTO;
static int dynamic_clock = 1;

#if 0 //USE_IME
static UINT32 gximg_map_to_ime_scale_method(UINT32 scale_method);
#endif
#if USE_ISE
static UINT32 gximg_map_to_ise_scale_method(UINT32 scale_method);
#endif
static void   GXIMG_SCALE_POINT(PVDO_FRAME p_img_buf, IPOINT    *p_point);
static void   GXIMG_SCALE_RECT(PVDO_FRAME p_img_buf, IRECT    *p_rect);
static UINT32 gximg_get_rotate_width_align(VDO_PXLFMT fmt);
static UINT32 gximg_get_rotate_height_align(VDO_PXLFMT fmt);

//-----------------------------------------------------------------------------
// GxImage Basic
//-----------------------------------------------------------------------------

static inline int check_no_flush(int force_use_physical_address, int no_flush_from_user)
{
	if(force_use_physical_address)
		return 1;
	else
		return no_flush_from_user;
}

#if 0
static void DBG_REG_DUMP(void)
{
#define RegLength   0x300
#define REG_ISE1    0xF0C90000
#define REG_ISE2    0xF0DC0000
#define REG_IME     0xF0C40000


	UINT32 i, j;

	DBG_DUMP("\r\nISE1\r\n");
	for (j = REG_ISE1; j < (REG_ISE1 + RegLength); j += 0x10) {
		DBG_DUMP("^R 0x%08X:", j);
		for (i = 0; i < 4; i++) {
			DBG_DUMP("0x%08X,", *(UINT32 *)(j + 4 * i));
		}
		DBG_DUMP("\r\n");
	}

	DBG_DUMP("\r\nISE2\r\n");
	for (j = REG_ISE2; j < (REG_ISE2 + RegLength); j += 0x10) {
		DBG_DUMP("^R 0x%08X:", j);
		for (i = 0; i < 4; i++) {
			DBG_DUMP("0x%08X,", *(UINT32 *)(j + 4 * i));
		}
		DBG_DUMP("\r\n");
	}

	DBG_DUMP("\r\nIME\r\n");
	for (j = REG_IME; j < (REG_IME + RegLength); j += 0x10) {
		DBG_DUMP("^R 0x%08X:", j);
		for (i = 0; i < 4; i++) {
			DBG_DUMP("0x%08X,", *(UINT32 *)(j + 4 * i));
		}
		DBG_DUMP("\r\n");
	}
}
#endif

#if USE_IME
#if 0 //USE_IME
static UINT32 gximg_map_to_ime_out_fmt_type(UINT32 fmt)
{
	UINT32 rtn_fmt = 0;

	switch (fmt) {
	case VDO_PXLFMT_YUV422_PLANAR:
		rtn_fmt = IME_OUTPUT_YCC_PLANAR;
		break;
	case VDO_PXLFMT_YUV420_PLANAR:
		rtn_fmt = IME_OUTPUT_YCC_PLANAR;
		break;
	case VDO_PXLFMT_YUV422:
		rtn_fmt = IME_OUTPUT_YCC_UVPACKIN;
		break;
	case VDO_PXLFMT_YUV420:
		rtn_fmt = IME_OUTPUT_YCC_UVPACKIN;
		break;
	case VDO_PXLFMT_Y8:
		rtn_fmt = IME_OUTPUT_YCC_PLANAR;
		break;
	case VDO_PXLFMT_RGB888_PLANAR:
		rtn_fmt = IME_OUTPUT_YCC_PLANAR;
		break;

	default:
		DBG_ERR("pixel format %x\r\n", fmt);
		break;
	}
	return rtn_fmt;
}
#endif
#endif

#if 0
static UINT32 gximg_map_to_warp_in_fmt(UINT32 fmt)
{
	UINT32 rtn_fmt = 0;

	switch (fmt) {
	case VDO_PXLFMT_YUV422_PLANAR:
		rtn_fmt = WARP_FORMAT_422;
		break;
	case VDO_PXLFMT_YUV420_PLANAR:
		rtn_fmt = WARP_FORMAT_420;
		break;
	case VDO_PXLFMT_YUV422:
		rtn_fmt = WARP_FORMAT_422;
		break;
	case VDO_PXLFMT_YUV420:
		rtn_fmt = WARP_FORMAT_420;
		break;
	default:
		GM_ERR(("GxImg: Invalid pixel format %x\r\n", fmt));
		break;
	}
	return rtn_fmt;
}
#endif

#if 0 //USE_IME
static UINT32 gximg_map_to_ime_scale_method(UINT32 scale_method)
{
	UINT32 rtn = 0;

	switch (scale_method) {
	case GXIMG_SCALE_BICUBIC:
		rtn = IMEALG_BICUBIC;
		break;
	case GXIMG_SCALE_BILINEAR:
		rtn = IMEALG_BILINEAR;
		break;
	case GXIMG_SCALE_NEAREST:
		rtn = IMEALG_NEAREST;
		break;
	case GXIMG_SCALE_INTEGRATION:
		rtn = IMEALG_INTEGRATION;
		break;
	default:
		DBG_ERR("%d\r\n", scale_method);
		rtn = IMEALG_BILINEAR;
		break;
	}
	return rtn;
}
#endif

#if USE_ISE
static UINT32 gximg_map_to_ise_scale_method(UINT32 scale_method)
{
	UINT32 rtn = 0;

	switch (scale_method) {
	case GXIMG_SCALE_BICUBIC:
		rtn = KDRV_ISE_BILINEAR;
		DBG_WRN("Not Support %d\r\n", scale_method);
		break;
	case GXIMG_SCALE_BILINEAR:
		rtn = KDRV_ISE_BILINEAR;
		break;
	case GXIMG_SCALE_NEAREST:
		rtn = KDRV_ISE_NEAREST;
		break;
	case GXIMG_SCALE_INTEGRATION:
		rtn = KDRV_ISE_INTEGRATION;
		break;
	default:
		DBG_ERR("%d\r\n", scale_method);
		rtn = KDRV_ISE_BILINEAR;
		break;
	}
	return rtn;
}
#endif

static UINT32 gximg_get_rotate_width_align(VDO_PXLFMT fmt)
{
#if USE_ROTATE
	return 4;
#else
	switch (fmt)
	{
	    case VDO_PXLFMT_RGB888_PLANAR:
	    case VDO_PXLFMT_Y8:
	        return 8;
	    default:
	        return 16;
	}
#endif
}

static UINT32 gximg_get_rotate_height_align(VDO_PXLFMT fmt)
{
#if USE_ROTATE
	return 4;
#else
	return 8;
#endif
}

#if USE_ROTATE
static ROTATION_CMD gximg_map_to_graph_rotate_dir(UINT32 rotate_dir)
{
	int rtn_dir = 0;
	switch (rotate_dir) {
	case VDO_DIR_ROTATE_90:
		rtn_dir = CMD_ROT_90;
		break;
	case VDO_DIR_ROTATE_270:
		rtn_dir = CMD_ROT_270;
		break;
	case VDO_DIR_ROTATE_180:
		rtn_dir = CMD_ROT_180;
		break;
	case VDO_DIR_MIRRORX:
		rtn_dir = CMD_HRZ_FLIP;
		break;
	case VDO_DIR_MIRRORY:
		rtn_dir = CMD_VTC_FLIP;
		break;
	case (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_90):
		rtn_dir = CMD_HRZ_FLIP_ROT_90;
		break;
	case (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_270):
		rtn_dir = CMD_HRZ_FLIP_ROT_270;
		break;
	default:
		DBG_ERR("RotateDir %d not supported\r\n", rotate_dir);
		break;
	}
	return rtn_dir;
}

static INT32 gximg_rotate_map_to_grph_id(GXIMG_RT_ENG rotate_engine)
{
	INT32 id;

	switch (rotate_engine) {
	case GXIMG_ROTATE_ENG1:
		id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_ROTATE, 0);
		break;
	default:
		DBG_ERR("Invalid RotateEng %d\r\n", rotate_engine);
		id = -1;
		break;
	}
	return id;
}
#else
static GRPH_CMD gximg_map_to_graph_rotate_dir(UINT32 rotate_dir)
{
	int rtn_dir = 0;
	switch (rotate_dir) {
	case VDO_DIR_ROTATE_90:
		rtn_dir = GRPH_CMD_ROT_90;
		break;
	case VDO_DIR_ROTATE_270:
		rtn_dir = GRPH_CMD_ROT_270;
		break;
	case VDO_DIR_ROTATE_180:
		rtn_dir = GRPH_CMD_ROT_180;
		break;
	case VDO_DIR_MIRRORX:
		rtn_dir = GRPH_CMD_HRZ_FLIP;
		break;
	case VDO_DIR_MIRRORY:
		rtn_dir = GRPH_CMD_VTC_FLIP;
		break;
	case (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_90):
		rtn_dir = GRPH_CMD_HRZ_FLIP_ROT_90;
		break;
	case (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_270):
		rtn_dir = GRPH_CMD_HRZ_FLIP_ROT_270;
		break;
	case VDO_DIR_ROTATE_0:
		rtn_dir = GRPH_CMD_ROT_0;
		break;
	default:
		DBG_ERR("RotateDir %d not supported\r\n", rotate_dir);
		break;
	}
	return rtn_dir;
}

static INT32 gximg_rotate_map_to_grph_id(GXIMG_RT_ENG rotate_engine)
{
	INT32 rtnID;

	switch (rotate_engine) {
	case GXIMG_ROTATE_ENG1:
		rtnID = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH1, 0);
		break;
	case GXIMG_ROTATE_ENG2:
		rtnID = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH1, 0);
		break;
	default:
		DBG_ERR("Invalid rotate_engine %d\r\n", rotate_engine);
		rtnID = -1;
		break;
	}
	return rtnID;
}
#endif

static UINT32 gximg_copy_map_to_grph_id(UINT32 copy_engine)
{
	UINT32 id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0);

	switch (copy_engine) {
	case GXIMG_CP_ENG1:
		id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0);
		break;
	case GXIMG_CP_ENG2:
		id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH1, 0);
		break;
	default:
		DBG_ERR("CpEng %d\r\n", copy_engine);
		break;
	}
	return id;
}

int gximg_check_grph_limit(GRPH_CMD cmd, UINT32 bits, GRPH_IMG *image, int check_wh, GRPH_IMG_ID id)
{
	static unsigned int gop0_8b[]={
		GRPH_GOP0_8BITS_WMIN, GRPH_GOP0_8BITS_WMAX, GRPH_GOP0_8BITS_WALIGN,
		GRPH_GOP0_8BITS_HMIN, GRPH_GOP0_8BITS_HMAX, GRPH_GOP0_8BITS_HALIGN,
		GRPH_GOP0_8BITS_ADDR_ALIGN, GRPH_GOP0_8BITS_ADDR_ALIGN, GRPH_GOP0_8BITS_ADDR_ALIGN};
	static unsigned int gop0_16b[]={
		GRPH_GOP0_16BITS_WMIN, GRPH_GOP0_16BITS_WMAX, GRPH_GOP0_16BITS_WALIGN,
		GRPH_GOP0_16BITS_HMIN, GRPH_GOP0_16BITS_HMAX, GRPH_GOP0_16BITS_HALIGN,
		GRPH_GOP0_16BITS_ADDR_ALIGN, GRPH_GOP0_16BITS_ADDR_ALIGN, GRPH_GOP0_16BITS_ADDR_ALIGN};
	static unsigned int gop0_32b[]={
		GRPH_GOP0_32BITS_WMIN, GRPH_GOP0_32BITS_WMAX, GRPH_GOP0_32BITS_WALIGN,
		GRPH_GOP0_32BITS_HMIN, GRPH_GOP0_32BITS_HMAX, GRPH_GOP0_32BITS_HALIGN,
		GRPH_GOP0_32BITS_ADDR_ALIGN, GRPH_GOP0_32BITS_ADDR_ALIGN, GRPH_GOP0_32BITS_ADDR_ALIGN};
	static unsigned int gop1_8b[]={
		GRPH_GOP1_8BITS_WMIN, GRPH_GOP1_8BITS_WMAX, GRPH_GOP1_8BITS_WALIGN,
		GRPH_GOP1_8BITS_HMIN, GRPH_GOP1_8BITS_HMAX, GRPH_GOP1_8BITS_HALIGN,
		GRPH_GOP1_8BITS_ADDR_ALIGN, GRPH_GOP1_8BITS_ADDR_ALIGN, GRPH_GOP1_8BITS_ADDR_ALIGN};
	static unsigned int gop1_16b[]={
		GRPH_GOP1_16BITS_WMIN, GRPH_GOP1_16BITS_WMAX, GRPH_GOP1_16BITS_WALIGN,
		GRPH_GOP1_16BITS_HMIN, GRPH_GOP1_16BITS_HMAX, GRPH_GOP1_16BITS_HALIGN,
		GRPH_GOP1_16BITS_ADDR_ALIGN, GRPH_GOP1_16BITS_ADDR_ALIGN, GRPH_GOP1_16BITS_ADDR_ALIGN};
	static unsigned int gop1_32b[]={
		GRPH_GOP1_32BITS_WMIN, GRPH_GOP1_32BITS_WMAX, GRPH_GOP1_32BITS_WALIGN,
		GRPH_GOP1_32BITS_HMIN, GRPH_GOP1_32BITS_HMAX, GRPH_GOP1_32BITS_HALIGN,
		GRPH_GOP1_32BITS_ADDR_ALIGN, GRPH_GOP1_32BITS_ADDR_ALIGN, GRPH_GOP1_32BITS_ADDR_ALIGN};
	static unsigned int gop2_8b[]={
		GRPH_GOP2_8BITS_WMIN, GRPH_GOP2_8BITS_WMAX, GRPH_GOP2_8BITS_WALIGN,
		GRPH_GOP2_8BITS_HMIN, GRPH_GOP2_8BITS_HMAX, GRPH_GOP2_8BITS_HALIGN,
		GRPH_GOP2_8BITS_ADDR_ALIGN, GRPH_GOP2_8BITS_ADDR_ALIGN, GRPH_GOP2_8BITS_ADDR_ALIGN};
	static unsigned int gop2_16b[]={
		GRPH_GOP2_16BITS_WMIN, GRPH_GOP2_16BITS_WMAX, GRPH_GOP2_16BITS_WALIGN,
		GRPH_GOP2_16BITS_HMIN, GRPH_GOP2_16BITS_HMAX, GRPH_GOP2_16BITS_HALIGN,
		GRPH_GOP2_16BITS_ADDR_ALIGN, GRPH_GOP2_16BITS_ADDR_ALIGN, GRPH_GOP2_16BITS_ADDR_ALIGN};
	static unsigned int gop2_32b[]={
		GRPH_GOP2_32BITS_WMIN, GRPH_GOP2_32BITS_WMAX, GRPH_GOP2_32BITS_WALIGN,
		GRPH_GOP2_32BITS_HMIN, GRPH_GOP2_32BITS_HMAX, GRPH_GOP2_32BITS_HALIGN,
		GRPH_GOP2_32BITS_ADDR_ALIGN, GRPH_GOP2_32BITS_ADDR_ALIGN, GRPH_GOP2_32BITS_ADDR_ALIGN};
	static unsigned int gop3_8b[]={
		GRPH_GOP3_8BITS_WMIN, GRPH_GOP3_8BITS_WMAX, GRPH_GOP3_8BITS_WALIGN,
		GRPH_GOP3_8BITS_HMIN, GRPH_GOP3_8BITS_HMAX, GRPH_GOP3_8BITS_HALIGN,
		GRPH_GOP3_8BITS_ADDR_ALIGN, GRPH_GOP3_8BITS_ADDR_ALIGN, GRPH_GOP3_8BITS_ADDR_ALIGN};
	static unsigned int gop3_16b[]={
		GRPH_GOP3_16BITS_WMIN, GRPH_GOP3_16BITS_WMAX, GRPH_GOP3_16BITS_WALIGN,
		GRPH_GOP3_16BITS_HMIN, GRPH_GOP3_16BITS_HMAX, GRPH_GOP3_16BITS_HALIGN,
		GRPH_GOP3_16BITS_ADDR_ALIGN, GRPH_GOP3_16BITS_ADDR_ALIGN, GRPH_GOP3_16BITS_ADDR_ALIGN};
	static unsigned int gop3_32b[]={
		GRPH_GOP3_32BITS_WMIN, GRPH_GOP3_32BITS_WMAX, GRPH_GOP3_32BITS_WALIGN,
		GRPH_GOP3_32BITS_HMIN, GRPH_GOP3_32BITS_HMAX, GRPH_GOP3_32BITS_HALIGN,
		GRPH_GOP3_32BITS_ADDR_ALIGN, GRPH_GOP3_32BITS_ADDR_ALIGN, GRPH_GOP3_32BITS_ADDR_ALIGN};
	static unsigned int gop4_8b[]={
		GRPH_GOP4_8BITS_WMIN, GRPH_GOP4_8BITS_WMAX, GRPH_GOP4_8BITS_WALIGN,
		GRPH_GOP4_8BITS_HMIN, GRPH_GOP4_8BITS_HMAX, GRPH_GOP4_8BITS_HALIGN,
		GRPH_GOP4_8BITS_ADDR_ALIGN, GRPH_GOP4_8BITS_ADDR_ALIGN, GRPH_GOP4_8BITS_ADDR_ALIGN};
	static unsigned int gop4_16b[]={
		GRPH_GOP4_16BITS_WMIN, GRPH_GOP4_16BITS_WMAX, GRPH_GOP4_16BITS_WALIGN,
		GRPH_GOP4_16BITS_HMIN, GRPH_GOP4_16BITS_HMAX, GRPH_GOP4_16BITS_HALIGN,
		GRPH_GOP4_16BITS_ADDR_ALIGN, GRPH_GOP4_16BITS_ADDR_ALIGN, GRPH_GOP4_16BITS_ADDR_ALIGN};
	static unsigned int gop4_32b[]={
		GRPH_GOP4_32BITS_WMIN, GRPH_GOP4_32BITS_WMAX, GRPH_GOP4_32BITS_WALIGN,
		GRPH_GOP4_32BITS_HMIN, GRPH_GOP4_32BITS_HMAX, GRPH_GOP4_32BITS_HALIGN,
		GRPH_GOP4_32BITS_ADDR_ALIGN, GRPH_GOP4_32BITS_ADDR_ALIGN, GRPH_GOP4_32BITS_ADDR_ALIGN};
	static unsigned int gop8_8b[]={
		GRPH_GOP8_8BITS_WMIN, GRPH_GOP8_8BITS_WMAX, GRPH_GOP8_8BITS_WALIGN,
		GRPH_GOP8_8BITS_HMIN, GRPH_GOP8_8BITS_HMAX, GRPH_GOP8_8BITS_HALIGN,
		GRPH_GOP8_8BITS_ADDR_ALIGN, GRPH_GOP8_8BITS_ADDR_ALIGN, GRPH_GOP8_8BITS_ADDR_ALIGN};
	static unsigned int gop8_16b[]={
		GRPH_GOP8_16BITS_WMIN, GRPH_GOP8_16BITS_WMAX, GRPH_GOP8_16BITS_WALIGN,
		GRPH_GOP8_16BITS_HMIN, GRPH_GOP8_16BITS_HMAX, GRPH_GOP8_16BITS_HALIGN,
		GRPH_GOP8_16BITS_ADDR_ALIGN, GRPH_GOP8_16BITS_ADDR_ALIGN, GRPH_GOP8_16BITS_ADDR_ALIGN};
	static unsigned int agop0_8b[]={
		GRPH_AOP0_8BITS_WMIN, GRPH_AOP0_8BITS_WMAX, GRPH_AOP0_8BITS_WALIGN,
		GRPH_AOP0_8BITS_HMIN, GRPH_AOP0_8BITS_HMAX, GRPH_AOP0_8BITS_HALIGN,
		GRPH_AOP0_8BITS_ADDR_ALIGN, GRPH_AOP0_8BITS_ADDR_ALIGN, GRPH_AOP0_8BITS_ADDR_ALIGN};
	//static unsigned int agop0_16b[]={
	//	GRPH_AOP0_16BITS_WMIN, GRPH_AOP0_16BITS_WMAX, GRPH_AOP0_16BITS_WALIGN,
	//	GRPH_AOP0_16BITS_HMIN, GRPH_AOP0_16BITS_HMAX, GRPH_AOP0_16BITS_HALIGN,
	//	GRPH_AOP0_16BITS_ADDR_ALIGN, GRPH_AOP0_16BITS_ADDR_ALIGN, GRPH_AOP0_16BITS_ADDR_ALIGN};
	//static unsigned int agop0_32b[]={
	//	GRPH_AOP0_32BITS_WMIN, GRPH_AOP0_32BITS_WMAX, GRPH_AOP0_32BITS_WALIGN,
	//	GRPH_AOP0_32BITS_HMIN, GRPH_AOP0_32BITS_HMAX, GRPH_AOP0_32BITS_HALIGN,
	//	GRPH_AOP0_32BITS_ADDR_ALIGN, GRPH_AOP0_32BITS_ADDR_ALIGN, GRPH_AOP0_32BITS_ADDR_ALIGN};
	static unsigned int agop3_8b[]={
		GRPH_AOP3_8BITS_WMIN, GRPH_AOP3_8BITS_WMAX, GRPH_AOP3_8BITS_WALIGN,
		GRPH_AOP3_8BITS_HMIN, GRPH_AOP3_8BITS_HMAX, GRPH_AOP3_8BITS_HALIGN,
		GRPH_AOP3_8BITS_ADDR_ALIGN, GRPH_AOP3_8BITS_ADDR_ALIGN, GRPH_AOP3_8BITS_ADDR_ALIGN};
	//static unsigned int agop3_16b[]={
	//	GRPH_AOP3_16BITS_WMIN, GRPH_AOP3_16BITS_WMAX, GRPH_AOP3_16BITS_WALIGN,
	//	GRPH_AOP3_16BITS_HMIN, GRPH_AOP3_16BITS_HMAX, GRPH_AOP3_16BITS_HALIGN,
	//	GRPH_AOP3_16BITS_ADDR_ALIGN, GRPH_AOP3_16BITS_ADDR_ALIGN, GRPH_AOP3_16BITS_ADDR_ALIGN};
	static unsigned int agop3_16uv[]={
		GRPH_AOP3_16BITSUV_WMIN, GRPH_AOP3_16BITSUV_WMAX, GRPH_AOP3_16BITSUV_WALIGN,
		GRPH_AOP3_16BITSUV_HMIN, GRPH_AOP3_16BITSUV_HMAX, GRPH_AOP3_16BITSUV_HALIGN,
		GRPH_AOP3_16BITSUV_ADDR_ALIGN, GRPH_AOP3_16BITSUV_ADDR_ALIGN, GRPH_AOP3_16BITSUV_ADDR_ALIGN};
	//static unsigned int agop3_16rgb[]={
	//	GRPH_AOP3_16BITARGB_WMIN, GRPH_AOP3_16BITARGB_WMAX, GRPH_AOP3_16BITARGB_WALIGN,
	//	GRPH_AOP3_16BITARGB_HMIN, GRPH_AOP3_16BITARGB_HMAX, GRPH_AOP3_16BITARGB_HALIGN,
	//	GRPH_AOP3_16BITARGB_ADDR_ALIGN, GRPH_AOP3_16BITARGB_ADDR_ALIGN, GRPH_AOP3_16BITARGB_ADDR_ALIGN};
	//static unsigned int agop3_32b[]={
	//	GRPH_AOP3_32BITS_WMIN, GRPH_AOP3_32BITS_WMAX, GRPH_AOP3_32BITS_WALIGN,
	//	GRPH_AOP3_32BITS_HMIN, GRPH_AOP3_32BITS_HMAX, GRPH_AOP3_32BITS_HALIGN,
	//	GRPH_AOP3_32BITS_ADDR_ALIGN, GRPH_AOP3_32BITS_ADDR_ALIGN, GRPH_AOP3_32BITS_ADDR_ALIGN};
	static unsigned int agop8_8b[]={
		GRPH_AOP8_8BITS_WMIN, GRPH_AOP8_8BITS_WMAX, GRPH_AOP8_8BITS_WALIGN,
		GRPH_AOP8_8BITS_HMIN, GRPH_AOP8_8BITS_HMAX, GRPH_AOP8_8BITS_HALIGN,
		GRPH_AOP8_8BITS_ADDR_ALIGN, GRPH_AOP8_8BITS_ADDR_ALIGN, GRPH_AOP8_8BITS_ADDR_ALIGN};
	static unsigned int agop8_16uv[]={
		GRPH_AOP8_16BITSUV_WMIN, GRPH_AOP8_16BITSUV_WMAX, GRPH_AOP8_16BITSUV_WALIGN,
		GRPH_AOP8_16BITSUV_HMIN, GRPH_AOP8_16BITSUV_HMAX, GRPH_AOP8_16BITSUV_HALIGN,
		GRPH_AOP8_16BITSUV_ADDR_ALIGN, GRPH_AOP8_16BITSUV_ADDR_ALIGN, GRPH_AOP8_16BITSUV_ADDR_ALIGN};
	//static unsigned int agop8_32b[]={
	//	GRPH_AOP8_32BITS_WMIN, GRPH_AOP8_32BITS_WMAX, GRPH_AOP8_32BITS_WALIGN,
	//	GRPH_AOP8_32BITS_HMIN, GRPH_AOP8_32BITS_HMAX, GRPH_AOP8_32BITS_HALIGN,
	//	GRPH_AOP8_32BITS_ADDR_ALIGN, GRPH_AOP8_32BITS_ADDR_ALIGN, GRPH_AOP8_32BITS_ADDR_ALIGN};
	static unsigned int agop13_8b[]={
		GRPH_AOP13_8BITS_WMIN, GRPH_AOP13_8BITS_WMAX, GRPH_AOP13_8BITS_WALIGN,
		GRPH_AOP13_8BITS_HMIN, GRPH_AOP13_8BITS_HMAX, GRPH_AOP13_8BITS_HALIGN,
		GRPH_AOP13_8BITS_ADDR_ALIGN, GRPH_AOP13_8BITS_ADDR_ALIGN, GRPH_AOP13_8BITS_ADDR_ALIGN};
	static unsigned int agop13_16uv[]={
		GRPH_AOP13_16BITSUV_WMIN, GRPH_AOP13_16BITSUV_WMAX, GRPH_AOP13_16BITSUV_WALIGN,
		GRPH_AOP13_16BITSUV_HMIN, GRPH_AOP13_16BITSUV_HMAX, GRPH_AOP13_16BITSUV_HALIGN,
		GRPH_AOP13_16BITSUV_ADDR_ALIGN, GRPH_AOP13_16BITSUV_ADDR_ALIGN, GRPH_AOP13_16BITSUV_ADDR_ALIGN};
	//static unsigned int agop13_16rgb[]={
	//	GRPH_AOP13_16BITSRGB_WMIN, GRPH_AOP13_16BITSRGB_WMAX, GRPH_AOP13_16BITSRGB_WALIGN,
	//	GRPH_AOP13_16BITSRGB_HMIN, GRPH_AOP13_16BITSRGB_HMAX, GRPH_AOP13_16BITSRGB_HALIGN,
	//	GRPH_AOP13_16BITSRGB_ADDR_ALIGN, GRPH_AOP13_16BITSRGB_ADDR_ALIGN, GRPH_AOP13_16BITSRGB_ADDR_ALIGN};
	//static unsigned int agop13_32b[]={
	//	GRPH_AOP13_32BITS_WMIN, GRPH_AOP13_32BITS_WMAX, GRPH_AOP13_32BITS_WALIGN,
	//	GRPH_AOP13_32BITS_HMIN, GRPH_AOP13_32BITS_HMAX, GRPH_AOP13_32BITS_HALIGN,
	//	GRPH_AOP13_32BITS_ADDR_ALIGN, GRPH_AOP13_32BITS_ADDR_ALIGN, GRPH_AOP13_32BITS_ADDR_ALIGN};
	static unsigned int agop16_8b[]={
		GRPH_AOP16_8BITS_WMIN, GRPH_AOP16_8BITS_WMAX, GRPH_AOP16_8BITS_WALIGN,
		GRPH_AOP16_8BITS_HMIN, GRPH_AOP16_8BITS_HMAX, GRPH_AOP16_8BITS_HALIGN,
		GRPH_AOP16_8BITS_ADDR_ALIGN, GRPH_AOP16_8BITS_ADDR_ALIGN, GRPH_AOP16_8BITS_ADDR_ALIGN};
	static unsigned int agop19_8b[]={
		GRPH_AOP19_8BITS_WMIN, GRPH_AOP19_8BITS_WMAX, GRPH_AOP19_8BITS_WALIGN,
		GRPH_AOP19_8BITS_HMIN, GRPH_AOP19_8BITS_HMAX, GRPH_AOP19_8BITS_HALIGN,
		GRPH_AOP19_8BITS_ADDR_ALIGN, GRPH_AOP19_8BITS_ADDR_ALIGN, GRPH_AOP19_8BITS_ADDR_ALIGN};
	static unsigned int agop19_16uv[]={
		GRPH_AOP19_16BITSUV_WMIN, GRPH_AOP19_16BITSUV_WMAX, GRPH_AOP19_16BITSUV_WALIGN,
		GRPH_AOP19_16BITSUV_HMIN, GRPH_AOP19_16BITSUV_HMAX, GRPH_AOP19_16BITSUV_HALIGN,
		GRPH_AOP19_16BITSUV_ADDR_ALIGN, GRPH_AOP19_16BITSUV_ADDR_ALIGN, GRPH_AOP19_16BITSUV_ADDR_ALIGN};
	//static unsigned int agop19_16rgb[]={
	//	GRPH_AOP19_16BITSRGB_WMIN, GRPH_AOP19_16BITSRGB_WMAX, GRPH_AOP19_16BITSRGB_WALIGN,
	//	GRPH_AOP19_16BITSRGB_HMIN, GRPH_AOP19_16BITSRGB_HMAX, GRPH_AOP19_16BITSRGB_HALIGN,
	//	GRPH_AOP19_16BITSRGB_ADDR_ALIGN, GRPH_AOP19_16BITSRGB_ADDR_ALIGN, GRPH_AOP19_16BITSRGB_ADDR_ALIGN};
	//static unsigned int agop19_32b[]={
	//	GRPH_AOP19_32BITS_WMIN, GRPH_AOP19_32BITS_WMAX, GRPH_AOP19_32BITS_WALIGN,
	//	GRPH_AOP19_32BITS_HMIN, GRPH_AOP19_32BITS_HMAX, GRPH_AOP19_32BITS_HALIGN,
	//	GRPH_AOP19_32BITS_ADDR_ALIGN, GRPH_AOP19_32BITS_ADDR_ALIGN, GRPH_AOP19_32BITS_ADDR_ALIGN};
	static unsigned int agop24_16b[]={
		GRPH_AOP24_16BITS_WMIN, GRPH_AOP24_16BITS_WMAX, GRPH_AOP24_16BITS_WALIGN,
		GRPH_AOP24_16BITS_HMIN, GRPH_AOP24_16BITS_HMAX, GRPH_AOP24_16BITS_HALIGN,
		GRPH_AOP24_16BITS_ADDR_ALIGN, GRPH_AOP24_16BITS_ADDR_B_ALIGN, GRPH_AOP24_16BITS_ADDR_C_ALIGN};
	static unsigned int agop25_16b[]={
		GRPH_AOP25_16BITS_WMIN, GRPH_AOP25_16BITS_WMAX, GRPH_AOP25_16BITS_WALIGN,
		GRPH_AOP25_16BITS_HMIN, GRPH_AOP25_16BITS_HMAX, GRPH_AOP25_16BITS_HALIGN,
		GRPH_AOP25_16BITS_ADDR_ALIGN, GRPH_AOP25_16BITS_ADDR_B_ALIGN, GRPH_AOP25_16BITS_ADDR_C_ALIGN};

	unsigned int *limit = NULL;

	if(!image){
		DBG_ERR("image is NULL\n");
		return -1;
	}

	if(cmd == GRPH_CMD_ROT_90){
		if(bits == GRPH_FORMAT_8BITS)
			limit = gop0_8b;
		else if(bits == GRPH_FORMAT_16BITS)
			limit = gop0_16b;
		else if(bits == GRPH_FORMAT_32BITS)
			limit = gop0_32b;
		else
			return 0;
	}else if(cmd == GRPH_CMD_ROT_270){
		if(bits == GRPH_FORMAT_8BITS)
			limit = gop1_8b;
		else if(bits == GRPH_FORMAT_16BITS)
			limit = gop1_16b;
		else if(bits == GRPH_FORMAT_32BITS)
			limit = gop1_32b;
		else
			return 0;
	}else if(cmd == GRPH_CMD_ROT_180){
		if(bits == GRPH_FORMAT_8BITS)
			limit = gop2_8b;
		else if(bits == GRPH_FORMAT_16BITS)
			limit = gop2_16b;
		else if(bits == GRPH_FORMAT_32BITS)
			limit = gop2_32b;
		else
			return 0;
	}else if(cmd == GRPH_CMD_HRZ_FLIP){
		if(bits == GRPH_FORMAT_8BITS)
			limit = gop3_8b;
		else if(bits == GRPH_FORMAT_16BITS)
			limit = gop3_16b;
		else if(bits == GRPH_FORMAT_32BITS)
			limit = gop3_32b;
		else
			return 0;
	}else if(cmd == GRPH_CMD_VTC_FLIP){
		if(bits == GRPH_FORMAT_8BITS)
			limit = gop4_8b;
		else if(bits == GRPH_FORMAT_16BITS)
			limit = gop4_16b;
		else if(bits == GRPH_FORMAT_32BITS)
			limit = gop4_32b;
		else
			return 0;
	}else if(cmd == GRPH_CMD_HRZ_FLIP_ROT_90){
		//if(bits == 8)
		//	limit = gop5_8b;
		//else if(bits == 16)
		//	limit = gop5_16b;
		//else
		//	limit = gop5_32b;
		return 0;
	}else if(cmd == GRPH_CMD_HRZ_FLIP_ROT_270){
		//if(bits == 8)
		//	limit = gop6_8b;
		//else if(bits == 16)
		//	limit = gop6_16b;
		//else
		//	limit = gop6_32b;
		return 0;
	}else if(cmd == GRPH_CMD_ROT_0){
		//if(bits == 8)
		//	limit = gop7_8b;
		//else if(bits == 16)
		//	limit = gop7_16b;
		//else
		//	limit = gop7_32b;
		return 0;
	}else if(cmd == GRPH_CMD_VCOV){
		if(bits == GRPH_FORMAT_8BITS)
			limit = gop8_8b;
		else if(bits == GRPH_FORMAT_16BITS)
			limit = gop8_16b;
		else
			return 0;
	}else if(cmd == GRPH_CMD_A_COPY){
		if(bits == GRPH_FORMAT_8BITS)
			limit = agop0_8b;
		else
			return 0;
	}else if(cmd == GRPH_CMD_COLOR_EQ){
		if(bits == GRPH_FORMAT_8BITS)
			limit = agop3_8b;
		else if(bits == GRPH_FORMAT_16BITS_UVPACK)
			limit = agop3_16uv;
		else
			return 0;
	}else if(cmd == GRPH_CMD_TEXT_COPY){
		if(bits == GRPH_FORMAT_8BITS)
			limit = agop8_8b;
		else if(bits == GRPH_FORMAT_16BITS_UVPACK)
			limit = agop8_16uv;
		else
			return 0;
	}else if(cmd == GRPH_CMD_BLENDING){
		if(bits == GRPH_FORMAT_8BITS)
			limit = agop13_8b;
		else if(bits == GRPH_FORMAT_16BITS_UVPACK)
			limit = agop13_16uv;
		else
			return 0;
	}else if(cmd == GRPH_CMD_PACKING){
		if(bits == GRPH_FORMAT_8BITS)
			limit = agop16_8b;
		else
			return 0;
	}else if(cmd == GRPH_CMD_PLANE_BLENDING){
		if(bits == GRPH_FORMAT_8BITS)
			limit = agop19_8b;
		else if(bits == GRPH_FORMAT_16BITS_UVPACK)
			limit = agop19_16uv;
		else
			return 0;
	}else if(cmd == GRPH_CMD_RGBYUV_BLEND){
		limit = agop24_16b;
	}else if(cmd == GRPH_CMD_RGBYUV_COLORKEY){
		limit = agop25_16b;
	}else
		return 0;

	if(image->lineoffset > GRPH_ALL_LOFF_MAX){
		DBG_ERR("image lineoffset(%d) should be less than %d\n", image->lineoffset, GRPH_ALL_LOFF_MAX);
		return -1;
	}
	if(GRPH_ALL_LOFF_ALIGN && (image->lineoffset & (GRPH_ALL_LOFF_ALIGN - 1))){
		DBG_ERR("image lineoffset(%d) should be %d aligned\n", image->lineoffset, GRPH_ALL_LOFF_ALIGN);
		return -1;
	}

	if (id == GRPH_IMG_ID_A) {
		if (limit[6] && (image->dram_addr & (limit[6] - 1))) {
			DBG_ERR("imageA addr(%d) should be %d aligned\n", image->dram_addr, limit[6]);
			return -1;
		}
	} else if (id == GRPH_IMG_ID_B) {
		if (limit[7] && (image->dram_addr & (limit[7] - 1))) {
			DBG_ERR("imageB addr(%d) should be %d aligned\n", image->dram_addr, limit[7]);
			return -1;
		}
	} else if (id == GRPH_IMG_ID_C) {
		if (limit[8] && (image->dram_addr & (limit[8] - 1))) {
			DBG_ERR("imageB addr(%d) should be %d aligned\n", image->dram_addr, limit[8]);
			return -1;
		}
	}

	if(check_wh){
		if(image->width < limit[0]){
			DBG_ERR("image width(%d) should be larger than %d\n", image->width, limit[0]);
			return -1;
		}
		if(image->width > limit[1]){
			DBG_ERR("image width(%d) should be less than %d\n", image->width, limit[1]);
			return -1;
		}
		if(limit[2] && (image->width & (limit[2] - 1))){
			DBG_ERR("image width(%d) should be %d aligned\n", image->width, limit[2]);
			return -1;
		}
		if(image->height < limit[3]){
			DBG_ERR("image height(%d) should be larger than %d\n", image->height, limit[3]);
			return -1;
		}
		if(image->height > limit[4]){
			DBG_ERR("image height(%d) should be less than %d\n", image->height, limit[4]);
			return -1;
		}
		if(limit[5] && (image->height & (limit[5] - 1))){
			DBG_ERR("image height(%d) should be %d aligned\n", image->height, limit[5]);
			return -1;
		}
	}

	return 0;
}

int gximg_open_grph(UINT32 grph_id)
{
	if(dynamic_clock == 0){
		return 0;
	}

	switch (grph_id) {
	case KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0):
		return kdrv_grph_open(KDRV_CHIP0, KDRV_GFX2D_GRPH0);
	case KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH1, 0):
		return kdrv_grph_open(KDRV_CHIP0, KDRV_GFX2D_GRPH1);
	default:
		DBG_ERR("unknown graph id %d\r\n", grph_id);
		break;
	}
	return -1;
}

int gximg_close_grph(UINT32 grph_id)
{
	if(dynamic_clock == 0){
		return 0;
	}

	switch (grph_id) {
	case KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0):
		return kdrv_grph_close(KDRV_CHIP0, KDRV_GFX2D_GRPH0);
	case KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH1, 0):
		return kdrv_grph_close(KDRV_CHIP0, KDRV_GFX2D_GRPH1);
	default:
		DBG_ERR("unknown graph id %d\r\n", grph_id);
		break;
	}
	return -1;
}


static BOOL gximg_calc_plane_rect(PVDO_FRAME p_img_buf,  UINT32 plane, IRECT    *p_rect, IRECT    *p_plane_rect)
{
	if (p_rect == 0) {
		DBG_ERR("p_rect is NULL\r\n");
		return FALSE;
	}

	if (plane >= GXIMG_MAX_PLANE_NUM) {
		DBG_ERR("plane(%d) is invalid\r\n", plane);
		return FALSE;
	}

	p_plane_rect->x = p_rect->x;
	p_plane_rect->y = p_rect->y;
	p_plane_rect->w = p_rect->w;
	p_plane_rect->h = p_rect->h;

	switch (p_img_buf->pxlfmt) {
	case VDO_PXLFMT_YUV444_PLANAR:
		break;
	case VDO_PXLFMT_YUV422_PLANAR:
		if (plane > 0) {
			p_plane_rect->x >>= 1;
			p_plane_rect->w >>= 1;
		}
		break;
	case VDO_PXLFMT_YUV420_PLANAR:
		if (plane > 0) {
			p_plane_rect->x >>= 1;
			p_plane_rect->y >>= 1;
			p_plane_rect->w >>= 1;
			p_plane_rect->h >>= 1;
		}
		break;
	case VDO_PXLFMT_YUV422:
		break;
	case VDO_PXLFMT_YUV420:
		if (plane > 0) {
			p_plane_rect->y >>= 1;
			p_plane_rect->h >>= 1;
		}
		break;
	case VDO_PXLFMT_YUV422_ONE:
		p_plane_rect->x <<= 1;
		p_plane_rect->w <<= 1;
		break;
	case VDO_PXLFMT_Y8:
	case VDO_PXLFMT_I8:
	case VDO_PXLFMT_RGB888_PLANAR:
		break;
	case VDO_PXLFMT_ARGB8888:
		p_plane_rect->x <<= 2;
		p_plane_rect->w <<= 2;
		break;
	case VDO_PXLFMT_ARGB8565:
		if (plane > 0) {
			p_plane_rect->x <<= 1;
			p_plane_rect->y <<= 1;
			p_plane_rect->w <<= 1;
			p_plane_rect->h <<= 1;
		}
		break;
	case VDO_PXLFMT_ARGB1555:
	case VDO_PXLFMT_ARGB4444:
	case VDO_PXLFMT_RGB565:
	case VDO_PXLFMT_RAW16:
		p_plane_rect->x <<= 1;
		p_plane_rect->w <<= 1;
		break;
	case VDO_PXLFMT_I1:
			p_plane_rect->x >>= 3;
			p_plane_rect->w >>= 3;
		break;
	case VDO_PXLFMT_I2:
			p_plane_rect->x >>= 2;
			p_plane_rect->w >>= 2;
		break;
	case VDO_PXLFMT_I4:
			p_plane_rect->x >>= 1;
			p_plane_rect->w >>= 1;
		break;
	default:
		DBG_ERR("pixel format %x\r\n", p_img_buf->pxlfmt);
		return E_PAR;
	}
	return TRUE;
}

static UINT32 gximg_calc_plane_offset(PVDO_FRAME p_img_buf, UINT32 plane, IRECT *p_plane_rect)
{
	UINT32 lineoffset;

	if (NULL == p_plane_rect) {
		return 0;
	}

	if (plane >= GXIMG_MAX_PLANE_NUM) {
		return 0;
	}

	lineoffset = p_plane_rect->x + (p_img_buf->loff[plane] * p_plane_rect->y);
	return lineoffset;
}

static ER _gximg_init_buf(PVDO_FRAME p_img_buf, UINT32 width, UINT32 height, UINT32 halign, VDO_PXLFMT pxlfmt, UINT32 lineoff, UINT32 addr, UINT32 available_size)
{
	UINT32    need_size, plane_num, i;
	IRECT     tmp_rect = {0}, tmp_plane_rect = {0};
	UINT32    buf_end;

	if (!p_img_buf) {
		DBG_ERR("p_img_buf is Null\r\n");
		return E_PAR;
	}
	if (!addr) {
		DBG_ERR("addr is Null\r\n");
		return E_PAR;
	}
	gximg_memset(p_img_buf, 0x00, sizeof(VDO_FRAME));
	if (lineoff & GXIMG_LINEOFFSET_PATTERN) {
		p_img_buf->loff[0] = ALIGN_CEIL(width, lineoff - GXIMG_LINEOFFSET_PATTERN);
	} else if (lineoff) {
		p_img_buf->loff[0] = lineoff;
	}
	// line offset is 0
	else {
		p_img_buf->loff[0] = ALIGN_CEIL(width, 4);
	}
	switch (pxlfmt) {
	case VDO_PXLFMT_YUV422_PLANAR:
		p_img_buf->loff[1] = p_img_buf->loff[0] >> 1;
		p_img_buf->loff[2] = p_img_buf->loff[1];
		plane_num = 3;
		break;

	case VDO_PXLFMT_YUV420_PLANAR:
		p_img_buf->loff[1] = p_img_buf->loff[0] >> 1;
		p_img_buf->loff[2] = p_img_buf->loff[1];
		plane_num = 3;
		break;

	case VDO_PXLFMT_YUV422:
		p_img_buf->loff[1] = p_img_buf->loff[0];
		p_img_buf->loff[2] = p_img_buf->loff[1];
		plane_num = 2;
		break;

	case VDO_PXLFMT_YUV420:
		p_img_buf->loff[1] = p_img_buf->loff[0];
		p_img_buf->loff[2] = p_img_buf->loff[1];
		plane_num = 2;
		break;

	case VDO_PXLFMT_Y8:
	case VDO_PXLFMT_I8:
		p_img_buf->loff[1] = 0;
		p_img_buf->loff[2] = 0;
		plane_num = 1;
		break;

	case VDO_PXLFMT_ARGB8565:
		p_img_buf->loff[1] = p_img_buf->loff[0] << 1;
		plane_num = 2;
		break;

	case VDO_PXLFMT_RGB888_PLANAR:
		p_img_buf->loff[1] = p_img_buf->loff[0];
		p_img_buf->loff[2] = p_img_buf->loff[1];
		plane_num = 3;
		break;

	case VDO_PXLFMT_ARGB8888:
		p_img_buf->loff[0] = p_img_buf->loff[0] << 2;
		plane_num = 1;
		break;

	case VDO_PXLFMT_ARGB1555:
	case VDO_PXLFMT_ARGB4444:
	case VDO_PXLFMT_RGB565:
	case VDO_PXLFMT_RAW16:
		p_img_buf->loff[0] = p_img_buf->loff[0] << 1;
		plane_num = 1;
		break;

	case VDO_PXLFMT_I1:
		p_img_buf->loff[0] = (p_img_buf->loff[0] >> 3);
		plane_num = 1;
		break;
	case VDO_PXLFMT_I2:
		p_img_buf->loff[0] = (p_img_buf->loff[0] >> 2);
		plane_num = 1;
		break;
	case VDO_PXLFMT_I4:
		p_img_buf->loff[0] = (p_img_buf->loff[0] >> 1);
		plane_num = 1;
		break;

	default:
		DBG_ERR("pixel format %x\r\n", p_img_buf->pxlfmt);
		return E_PAR;
	}
	p_img_buf->size.w = width;
	p_img_buf->size.h = height;
	p_img_buf->pxlfmt = pxlfmt;
	p_img_buf->addr[0] = addr;
	buf_end = addr;
	tmp_rect.w = width;
	tmp_rect.h = height;

	for (i = 1; i < plane_num; i++) {
		//addr is height aligned
		tmp_rect.h = halign;
		gximg_calc_plane_rect(p_img_buf, i - 1, &tmp_rect, &tmp_plane_rect);
		p_img_buf->addr[i] = buf_end + (tmp_plane_rect.h) * p_img_buf->loff[i - 1];
		buf_end += (tmp_plane_rect.h) * p_img_buf->loff[i - 1];
		//ph is image's height
		tmp_rect.h = height;
		gximg_calc_plane_rect(p_img_buf, i - 1, &tmp_rect, &tmp_plane_rect);
		p_img_buf->pw[i - 1] = tmp_plane_rect.w;
		p_img_buf->ph[i - 1] = tmp_plane_rect.h;
	}
	//ph is image's height
	tmp_rect.h = height;
	gximg_calc_plane_rect(p_img_buf, plane_num - 1, &tmp_rect, &tmp_plane_rect);
	p_img_buf->pw[plane_num - 1] = tmp_plane_rect.w;
	p_img_buf->ph[plane_num - 1] = tmp_plane_rect.h;
	//addr is height aligned
	tmp_rect.h = halign;
	gximg_calc_plane_rect(p_img_buf, plane_num - 1, &tmp_rect, &tmp_plane_rect);
	buf_end += (tmp_plane_rect.h) * p_img_buf->loff[plane_num - 1];

	need_size = buf_end - addr;
	if (available_size < need_size) {
		DBG_ERR("need_size 0x%x > Available size 0x%x\r\n", need_size, available_size);
		return E_SYS;
	}
	p_img_buf->sign = MAKEFOURCC('V','F','R','M');
    #if USE_VIRT_COORD
	GXIMG_GET_SCALE_RATIO(p_img_buf)->x = SCALE_UNIT_1X;
	GXIMG_GET_SCALE_RATIO(p_img_buf)->y = SCALE_UNIT_1X;
    #endif
	p_img_buf->count = 1;
	p_img_buf->timestamp = hwclock_get_longcounter();

	//DBG_FUNC_CHK("[init]\r\n");
	DBG_MSG("[init]W=%04d, H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_img_buf->size.w, p_img_buf->size.h, p_img_buf->loff[0], p_img_buf->loff[1], p_img_buf->loff[2], p_img_buf->pxlfmt);
	DBG_MSG("[init]A[0]=0x%x, A[1]=0x%x, A[2]=0x%x, need_size=0x%x , AvailSize=0x%x\r\n", p_img_buf->addr[0], p_img_buf->addr[1], p_img_buf->addr[2], need_size, available_size);
	return E_OK;
}

ER gximg_init_buf(PVDO_FRAME p_img_buf, UINT32 width, UINT32 height, VDO_PXLFMT pxlfmt, UINT32 lineoff, UINT32 addr, UINT32 available_size)
{
	return _gximg_init_buf(p_img_buf, width, height, height, pxlfmt, lineoff, addr, available_size);
}

ER gximg_init_buf_h_align(PVDO_FRAME p_img_buf, UINT32 width, UINT32 height, UINT32 halign, VDO_PXLFMT pxlfmt, UINT32 lineoff, UINT32 addr, UINT32 available_size)
{
	return _gximg_init_buf(p_img_buf, width, height, halign, pxlfmt, lineoff, addr, available_size);
}

ER gximg_init_buf_ex(PVDO_FRAME p_img_buf, UINT32 width, UINT32 height, VDO_PXLFMT pxlfmt, UINT32 lineoff[], UINT32 addr[])
{
	UINT32    plane_num, i;
	IRECT     tmp_rect = {0}, tmp_plane_rect = {0};

	if (!p_img_buf) {
		DBG_ERR("p_img_buf is Null\r\n");
		return E_PAR;
	}
	if (!addr) {
		DBG_ERR("addr is Null\r\n");
		return E_PAR;
	}
	if (!lineoff) {
		DBG_ERR("lineoff is Null\r\n");
		return E_PAR;
	}
	switch (pxlfmt) {
	case VDO_PXLFMT_YUV444_PLANAR:
		plane_num = 3;
		break;
	case VDO_PXLFMT_YUV422_PLANAR:
		plane_num = 3;
		break;
	case VDO_PXLFMT_YUV420_PLANAR:
		plane_num = 3;
		break;
	case VDO_PXLFMT_YUV422:
		plane_num = 2;
		break;

	case VDO_PXLFMT_YUV420:
		plane_num = 2;
		break;

	case VDO_PXLFMT_YUV422_ONE:
		plane_num = 1;
		break;

	case VDO_PXLFMT_Y8:
	case VDO_PXLFMT_I8:
		plane_num = 1;
		break;

	case VDO_PXLFMT_ARGB8565:
		plane_num = 2;
		break;

	case VDO_PXLFMT_RGB888_PLANAR:
		plane_num = 3;
		break;

	case VDO_PXLFMT_ARGB8888:
		plane_num = 1;
		break;

	case VDO_PXLFMT_ARGB1555:
	case VDO_PXLFMT_ARGB4444:
	case VDO_PXLFMT_RGB565:
		plane_num = 1;
		break;

	case VDO_PXLFMT_I1:
	case VDO_PXLFMT_I2:
	case VDO_PXLFMT_I4:
	case VDO_PXLFMT_RAW8:
	case VDO_PXLFMT_RAW10:
	case VDO_PXLFMT_RAW12:
	case VDO_PXLFMT_RAW16:
		plane_num = 1;
		break;

	default:
		DBG_ERR("pixel format %x\r\n", pxlfmt);
		return E_PAR;
	}
	gximg_memset(p_img_buf, 0x00, sizeof(VDO_FRAME));
	p_img_buf->size.w = width;
	p_img_buf->size.h = height;
	p_img_buf->pxlfmt = pxlfmt;
	tmp_rect.w = width;
	tmp_rect.h = height;
	for (i = 0; i < plane_num; i++) {
		p_img_buf->loff[i] = lineoff[i];
		p_img_buf->addr[i] = addr[i];
		gximg_calc_plane_rect(p_img_buf, i, &tmp_rect, &tmp_plane_rect);
		p_img_buf->pw[i] = tmp_plane_rect.w;
		p_img_buf->ph[i] = tmp_plane_rect.h;
	}
	p_img_buf->sign = MAKEFOURCC('V','F','R','M');
    #if USE_VIRT_COORD
	GXIMG_GET_SCALE_RATIO(p_img_buf)->x = SCALE_UNIT_1X;
	GXIMG_GET_SCALE_RATIO(p_img_buf)->y = SCALE_UNIT_1X;
    #endif
	p_img_buf->count = 1;
	p_img_buf->timestamp = hwclock_get_longcounter();

	//DBG_FUNC_CHK("[init]\r\n");
	DBG_MSG("[init]W=%04d, H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_img_buf->size.w, p_img_buf->size.h, p_img_buf->loff[0], p_img_buf->loff[1], p_img_buf->loff[2], p_img_buf->pxlfmt);
	DBG_MSG("[init]A[0]=0x%x, A[1]=0x%x, A[2]=0x%x\r\n", p_img_buf->addr[0], p_img_buf->addr[1], p_img_buf->addr[2]);
	return E_OK;
}

static UINT32 _gximg_calc_require_size(UINT32 width, UINT32 height, VDO_PXLFMT pxlfmt, UINT32 lineoff)
{
	UINT32 loff_adj;
	UINT32 need_size;

	if (lineoff & GXIMG_LINEOFFSET_PATTERN) {
		loff_adj = ALIGN_CEIL(width, lineoff - GXIMG_LINEOFFSET_PATTERN);
	} else if (lineoff) {
		loff_adj = lineoff;
	} else {
		// line offset is 0
		loff_adj = ALIGN_CEIL(width, 4);
	}

	switch (pxlfmt) {
	case VDO_PXLFMT_YUV422_PLANAR:
		need_size = height * loff_adj + height * (loff_adj >> 1) * 2;
		break;

	case VDO_PXLFMT_YUV420_PLANAR:
		need_size = height * loff_adj + (height >> 1) * (loff_adj >> 1) * 2;
		break;

	case VDO_PXLFMT_YUV422:
		need_size = height * loff_adj + height * loff_adj;
		break;

	case VDO_PXLFMT_YUV420:
		need_size = height * loff_adj + (height >> 1) * loff_adj;
		break;

	case VDO_PXLFMT_Y8:
		need_size = height * loff_adj;
		break;

	case VDO_PXLFMT_ARGB8565:
		need_size = height * loff_adj + height * loff_adj * 2;
		break;

	case VDO_PXLFMT_RGB888_PLANAR:
		need_size = height * loff_adj * 3;
		break;

	case VDO_PXLFMT_ARGB8888:
		need_size = height * loff_adj * 4;
		break;

	case VDO_PXLFMT_ARGB1555:
	case VDO_PXLFMT_ARGB4444:
	case VDO_PXLFMT_RGB565:
		need_size = height * loff_adj * 2;
		break;

	default:
		DBG_ERR("pixel format %x\r\n", pxlfmt);
		need_size = 0;
	}

	return need_size;
}

UINT32 gximg_calc_require_size(UINT32 width, UINT32 height, VDO_PXLFMT pxlfmt, UINT32 lineoff)
{
	return _gximg_calc_require_size(width, height, pxlfmt, lineoff);
}

UINT32 gximg_calc_require_size_h_align(UINT32 width, UINT32 height, UINT32 halign, VDO_PXLFMT pxlfmt, UINT32 lineoff)
{
	return _gximg_calc_require_size(width, halign, pxlfmt, lineoff);
}

ER gximg_get_buf_addr(PVDO_FRAME p_img_buf, UINT32 *p_start_addr, UINT32 *p_end_addr)
{
	UINT32    plane_num, i;
	IRECT     tmp_rect = {0}, tmp_plane_rect = {0};
	UINT32    buf_end;

	if (!p_img_buf) {
		DBG_ERR("p_img_buf is Null\r\n");
		return E_PAR;
	}
	if (p_img_buf->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_img_buf is not initialized\r\n");
		return E_PAR;
	}

	switch (p_img_buf->pxlfmt) {
	case VDO_PXLFMT_YUV422_PLANAR:
		plane_num = 3;
		break;

	case VDO_PXLFMT_YUV420_PLANAR:
		plane_num = 3;
		break;

	case VDO_PXLFMT_YUV422:
		plane_num = 2;
		break;

	case VDO_PXLFMT_YUV420:
		plane_num = 2;
		break;

	case VDO_PXLFMT_Y8:
		plane_num = 1;
		break;

	case VDO_PXLFMT_ARGB8565:
		plane_num = 2;
		break;

	case VDO_PXLFMT_RGB888_PLANAR:
		plane_num = 3;
		break;

	case VDO_PXLFMT_ARGB8888:
	case VDO_PXLFMT_ARGB1555:
	case VDO_PXLFMT_ARGB4444:
	case VDO_PXLFMT_RGB565:
		plane_num = 1;
		break;


	default:
		DBG_ERR("pixel format %x\r\n", p_img_buf->pxlfmt);
		return E_PAR;
	}
	tmp_rect.w = p_img_buf->size.w;
	tmp_rect.h = p_img_buf->size.h;


	buf_end = p_img_buf->addr[0];
	for (i = 1; i < plane_num; i++) {
		gximg_calc_plane_rect(p_img_buf, i - 1, &tmp_rect, &tmp_plane_rect);
		buf_end += (tmp_plane_rect.h) * p_img_buf->loff[i - 1];
	}
	gximg_calc_plane_rect(p_img_buf, plane_num - 1, &tmp_rect, &tmp_plane_rect);
	buf_end += (tmp_plane_rect.h) * p_img_buf->loff[plane_num - 1];

	*p_start_addr = p_img_buf->addr[0];
	*p_end_addr = buf_end;
	return E_OK;
}

//-----------------------------------------------------------------------------
// GxImage Process
//-----------------------------------------------------------------------------
static BOOL gximg_calc_clipped_rect(IRECT *p_src_rect, IPOINT *p_dest_point, IRECT *p_dest_size, IRECT *p_dest_rect)
{
	int w, h;

	w =  p_src_rect->w;

	if (p_dest_point->x >= p_dest_size->w) {
		return FALSE;
	}

	if (p_dest_point->x + w <= 0) {
		return FALSE;
	}

	if (p_dest_point->x + w > p_dest_size->w) {
		p_src_rect->w = (p_dest_size->w - p_dest_point->x);
	}
	if (p_dest_point->x < 0) {
		p_src_rect->x += -p_dest_point->x;
		p_src_rect->w += p_dest_point->x;
		p_dest_point->x = 0;
	}

	h =  p_src_rect->h;

	if (p_dest_point->y >= p_dest_size->h) {
		return FALSE;
	}

	if (p_dest_point->y + h <= 0) {
		return FALSE;
	}

	if (p_dest_point->y + h > p_dest_size->h) {
		p_src_rect->h = (p_dest_size->h - p_dest_point->y);
	}
	if (p_dest_point->y < 0) {
		p_src_rect->y += -p_dest_point->y;
		p_src_rect->h += p_dest_point->y;
		p_dest_point->y = 0;
	}

	p_dest_rect->x = p_dest_point->x;
	p_dest_rect->y = p_dest_point->y;
	p_dest_rect->w = p_src_rect->w;
	p_dest_rect->h = p_src_rect->h;

	return TRUE;
}

static void gximg_adjust_uv_packed_align(UINT32 pxlfmt, IRECT *p_rect)
{
	p_rect->x = ALIGN_FLOOR(p_rect->x, 2);
	p_rect->w = ALIGN_FLOOR(p_rect->w, 2);
	if (pxlfmt == VDO_PXLFMT_YUV420) {
		p_rect->y = ALIGN_FLOOR(p_rect->y, 2);
		p_rect->h = ALIGN_FLOOR(p_rect->h, 2);
	}
}

static ER _gximg_fill_data_ex_with_flush(PVDO_FRAME p_dst_img, IRECT    *p_dst_region, UINT32 color, GXIMG_FILL_ENG engine, int no_flush, int usePA)
{
	UINT32 valid_w, valid_h, dst_x, dst_y, plane_num, i;
	UINT32 y, cb, cr, cbcr;
	IRECT  tmp_rect = {0}, tmp_plane_rect = {0};
	UINT32 fill_data[4], grph_id, bits = GRPH_FORMAT_8BITS;

	DBG_FUNC_BEGIN("[fill]\r\n");
	if (!p_dst_img) {
		DBG_ERR("p_dst_img is Null\r\n");
		return E_PAR;
	} else if (p_dst_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_dst_img is not initialized\r\n");
		return E_PAR;
	} else if (!p_dst_img->addr[0]) {
		DBG_ERR("p_dst_img->addr[0] is Null\r\n");
		return E_PAR;
	}
	DBG_MSG("[fill]p_dst_img.W=%04d, H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,pxlfmt=%x\r\n", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], p_dst_img->loff[1], p_dst_img->loff[2], p_dst_img->pxlfmt);
	if (p_dst_region != GXIMG_REGION_MATCH_IMG) {
		GXIMG_SCALE_RECT(p_dst_img, p_dst_region);
		DBG_MSG("[fill]DstRgn x=%04d, y=%04d, w=%04d, h=%04d\r\n", p_dst_region->x, p_dst_region->y, p_dst_region->w, p_dst_region->h);
		if (p_dst_region->x < 0) {
			if (p_dst_region->x + (INT32)p_dst_img->size.w <= 0) {
				DBG_ERR("Pos x=%d\r\n", p_dst_region->x);
				return E_PAR;
			} else if (p_dst_region->x + (INT32)p_dst_region->w > (INT32)p_dst_img->size.w) {
				valid_w = p_dst_img->size.w;
			} else {
				valid_w = p_dst_region->x + p_dst_region->w;
			}
			dst_x = 0;
		}
		// x > 0
		else {
			if (p_dst_region->x >= (INT32)p_dst_img->size.w) {
				DBG_ERR("Pos x= %d\r\n", p_dst_region->x);
				return E_PAR;
			} else if (p_dst_region->x + p_dst_region->w > (INT32)p_dst_img->size.w) {
				valid_w = p_dst_img->size.w - p_dst_region->x;
			} else {
				valid_w = p_dst_region->w;
			}
			dst_x = p_dst_region->x;
		}
		if (p_dst_region->y < 0) {
			if (p_dst_region->y + (INT32)p_dst_img->size.h <= 0) {
				DBG_ERR("Pos y= %d\r\n", p_dst_region->y);
				return E_PAR;
			} else if (p_dst_region->y + (INT32)p_dst_region->h > (INT32)p_dst_img->size.h) {
				valid_h = p_dst_img->size.h;
			} else {
				valid_h = p_dst_region->h + p_dst_region->y;
			}
			dst_y = 0;
		}
		// y > 0
		else {
			if (p_dst_region->y >= (INT32)p_dst_img->size.h) {
				DBG_ERR("Pos y= %d\r\n", p_dst_region->y);
				return E_PAR;
			} else if (p_dst_region->y + p_dst_region->h > (INT32)p_dst_img->size.h) {
				valid_h = p_dst_img->size.h - p_dst_region->y;
			} else {
				valid_h = p_dst_region->h;
			}
			dst_y = p_dst_region->y;
		}
	}
	// no pDstRegion so use default x, y, w, h
	else {
		dst_x = 0;
		dst_y = 0;
		valid_w = p_dst_img->size.w;
		valid_h = p_dst_img->size.h;
	}

	tmp_rect.x = dst_x;
	tmp_rect.y = dst_y;
	tmp_rect.w = valid_w;
	tmp_rect.h = valid_h;

	if(p_dst_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR ||
	   p_dst_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR ||
	   p_dst_img->pxlfmt == VDO_PXLFMT_YUV422 ||
	   p_dst_img->pxlfmt == VDO_PXLFMT_YUV420)
		gximg_adjust_uv_packed_align(p_dst_img->pxlfmt, &tmp_rect);

	DBG_MSG("[fill]tmp_rect x=%04d, y=%04d, w=%04d, h=%04d\r\n", tmp_rect.x, tmp_rect.y, tmp_rect.w, tmp_rect.h);

	y = (color & 0x000000FF);
	y = (y + (y << 8) + (y << 16) + (y << 24));
	cb = ((color & 0x0000FF00) >> 8);
	cb = cb + (cb << 8) + (cb << 16) + (cb << 24);
	cr = ((color & 0x00FF0000) >> 16);
	cr = cr + (cr << 8) + (cr << 16) + (cr << 24);
	cbcr = ((color & 0x0000FF00) >> 8) + ((color & 0x00FF0000) >> 8) ;
	cbcr = cbcr + (cbcr << 16);

	switch (p_dst_img->pxlfmt) {
	case VDO_PXLFMT_YUV422_PLANAR:
	case VDO_PXLFMT_YUV420_PLANAR:
		fill_data[0] = y;
		fill_data[1] = cb;
		fill_data[2] = cr;
		plane_num = 3;
		bits = GRPH_FORMAT_8BITS;
		break;
	case VDO_PXLFMT_YUV422:
	case VDO_PXLFMT_YUV420:
		fill_data[0] = y;
		fill_data[1] = cbcr;
		plane_num = 2;
		bits = GRPH_FORMAT_8BITS;
		break;

	case VDO_PXLFMT_Y8:
	case VDO_PXLFMT_I8:
		fill_data[0] = y;
		plane_num = 1;
		bits = GRPH_FORMAT_8BITS;
		break;
	case VDO_PXLFMT_RGB565:
	case VDO_PXLFMT_ARGB1555:
	case VDO_PXLFMT_ARGB4444:
		fill_data[0] = (color & 0x00FF);
		fill_data[1] = ((color & 0xFF00) >> 8);
		fill_data[0] = (fill_data[0] | (fill_data[0] << 8) | (fill_data[0] << 16) | (fill_data[0] << 24));
		fill_data[1] = (fill_data[1] | (fill_data[1] << 8) | (fill_data[1] << 16) | (fill_data[1] << 24));
		plane_num = 1;
		bits = GRPH_FORMAT_16BITS_UVPACK;
		break;
	case VDO_PXLFMT_ARGB8888:
		fill_data[0] = ((color & 0xFF000000) >> 24);
		fill_data[1] = ((color & 0x00FF0000) >> 16);
		fill_data[2] = ((color & 0x0000FF00) >> 8);
		fill_data[3] =  (color & 0x000000FF);
		plane_num = 2;
		break;
	default:
		DBG_ERR("pixel format %x\r\n", p_dst_img->pxlfmt);
		return E_PAR;
	}

	grph_id = gximg_copy_map_to_grph_id(engine);
	DBG_MSG("[fill]grph_id = %d\r\n", grph_id);
	if (tmp_rect.w >= 2 && tmp_rect.h >= 2) {
		// use grph to copy
		if(gximg_open_grph(grph_id) == -1){
			DBG_ERR("gximg_open_grph(%d) fail\n", grph_id);
			return E_PAR;
		}

		for (i = 0; i < plane_num; i++) {
#if USE_GRPH
			{
				KDRV_GRPH_TRIGGER_PARAM request = {0};
				GRPH_IMG images[1];
				GRPH_PROPERTY property1[1], property2[1], property3[1];

				gximg_memset(images, 0, sizeof(images));
				gximg_memset(property1, 0, sizeof(property1));
				gximg_memset(property2, 0, sizeof(property2));
				gximg_memset(property3, 0, sizeof(property3));

				request.command = GRPH_CMD_TEXT_COPY;
				request.is_skip_cache_flush = check_no_flush(usePA, no_flush);
				request.is_buf_pa = usePA;
				request.p_images = &(images[0]);
				request.p_property = &(property1[0]);

				images[0].img_id =            GRPH_IMG_ID_C;       // destination image
				images[0].p_next = NULL;
				if(p_dst_img->pxlfmt == VDO_PXLFMT_ARGB8888){
					gximg_calc_plane_rect(p_dst_img, 0, &tmp_rect, &tmp_plane_rect);
					images[0].dram_addr = (UINT32)p_dst_img->addr[0] + gximg_calc_plane_offset(p_dst_img, 0, &tmp_plane_rect);
					images[0].lineoffset =     p_dst_img->loff[0];
					images[0].width =          tmp_plane_rect.w;
					images[0].height =         tmp_plane_rect.h;

					if(i == 0){
						request.format = GRPH_FORMAT_32BITS_ARGB8888_A;
						property1[0].id       = GRPH_PROPERTY_ID_A;
						property1[0].property = fill_data[0];
						property1[0].p_next   = NULL;
					}else{
						request.format = GRPH_FORMAT_32BITS_ARGB8888_RGB;
						property1[0].id       = GRPH_PROPERTY_ID_R;
						property1[0].property = fill_data[1];
						property1[0].p_next   = property2;
						property2[0].id       = GRPH_PROPERTY_ID_G;
						property2[0].property = fill_data[2];
						property2[0].p_next   = property3;
						property3[0].id       = GRPH_PROPERTY_ID_B;
						property3[0].property = fill_data[3];
						property3[0].p_next   = NULL;
					}
				}else{
					gximg_calc_plane_rect(p_dst_img, i, &tmp_rect, &tmp_plane_rect);
					images[0].dram_addr = (UINT32)p_dst_img->addr[i] + gximg_calc_plane_offset(p_dst_img, i, &tmp_plane_rect);
					images[0].lineoffset =     p_dst_img->loff[i];
					images[0].width =          tmp_plane_rect.w;
					images[0].height =         tmp_plane_rect.h;
					 if(bits == GRPH_FORMAT_16BITS_UVPACK){
						request.format = bits;

						property1[0].id       = GRPH_PROPERTY_ID_U;
						property1[0].property = fill_data[i];
						property1[0].p_next   = property2;
						property2[0].id       = GRPH_PROPERTY_ID_V;
						property2[0].property = fill_data[i + plane_num];
						property2[0].p_next   = NULL;
					}else{
						request.format = bits;

						property1[0].id       = GRPH_PROPERTY_ID_NORMAL;
						property1[0].property = fill_data[i];
						property1[0].p_next   = NULL;
					}
				}

				if(gximg_check_grph_limit(request.command, bits, &(images[0]), 1, GRPH_IMG_ID_C)){
					DBG_ERR("limit check fail\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
				if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
					DBG_ERR("kdrv_grph_trigger() fail\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
			}
#else
			{
				INT32 j, k;

				gximg_calc_plane_rect(p_dst_img, i, &tmprect, &tmp_plane_rect);
				for (j = 0; j < tmp_plane_rect.h; j++)
				{
#if 0
					gximg_memset((UINT8 *)p_dst_img->addr[i] + gximg_calc_plane_offset(p_dst_img, i, &tmp_plane_rect) + j * p_dst_img->loff[i], fill_data[i], tmp_plane_rect.w);
#else
					/*
					UINT32 *p_data;

					p_ata = (UINT32 *)(p_dst_img->addr[i]+gximg_calc_plane_offset(p_dst_img, i ,&tmpP_pane_rect)+j*p_dst_img->loff[i]);

					for (k=0;k<tmp_plane_rect.w;k+=4)
					{
					    *p_data = fill_data[i];
					    p_data++;
					}
					*/
					UINT16 *p_data;

					p_data = (UINT16 *)(p_dst_img->addr[i] + gximg_calc_plane_offset(p_dst_img, i, &tmp_plane_rect) + j * p_dst_img->loff[i]);

					for (k = 0; k < tmp_plane_rect.w; k += 2) {
						*p_data = fill_data[i] & 0xFFFF;
						p_data++;
					}
#endif
				}
			}
#endif
		}
		if(gximg_close_grph(grph_id) == -1){
			DBG_ERR("gximg_close_grph(%d) fail\n", grph_id);
			return E_PAR;
		}
	} else {
		DBG_ERR("valid_w=%d valid_h=%d\r\n", tmp_rect.w, tmp_rect.h);
		return E_PAR;
	}
	DBG_FUNC_END("[fill]\r\n");
	return E_OK;
}

ER gximg_fill_data(PVDO_FRAME p_dst_img, IRECT    *p_dst_region, UINT32 color, int usePA)
{
	return _gximg_fill_data_ex_with_flush(p_dst_img, p_dst_region, color, GXIMG_FILL_ENG1, 0, usePA);
}

ER gximg_fill_data_no_flush(PVDO_FRAME p_dst_img, IRECT    *p_dst_region, UINT32 color, int usePA)
{
	return _gximg_fill_data_ex_with_flush(p_dst_img, p_dst_region, color, GXIMG_FILL_ENG1, 1, usePA);
}

ER gximg_fill_data_ex(PVDO_FRAME p_dst_img, IRECT    *p_dst_region, UINT32 color, GXIMG_FILL_ENG engine, int usePA)
{
	return _gximg_fill_data_ex_with_flush(p_dst_img, p_dst_region, color, engine, 0, usePA);
}

static ER gximg_get_copy_rect(PVDO_FRAME p_src_img, IRECT *p_src_region, IPOINT *p_dst_location, PVDO_FRAME p_dst_img, IRECT *p_out_src_rect, IRECT *p_out_dst_rect)
{
	IRECT     tmp_dst_rect = {0};//, TmpOutRect = {0}, TmpSrcRegion={0};
	IPOINT    tmp_pos = {0};


	if (!p_src_img) {
		DBG_ERR("p_src_img is Null\r\n");
		return E_PAR;
	} else if (!p_dst_img) {
		DBG_ERR("p_dst_img is Null\r\n");
		return E_PAR;
	} else if (p_src_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_src_img is not initialized\r\n");
		return E_PAR;
	} else if (p_dst_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_dst_img is not initialized\r\n");
		return E_PAR;
	} else if (!p_src_img->addr[0]) {
		DBG_ERR("p_src_img->addr[0] is Null\r\n");
		return E_PAR;
	} else if (!p_dst_img->addr[0]) {
		DBG_ERR("p_dst_img->addr[0] is Null\r\n");
		return E_PAR;
	}
	/*
	    else if(p_src_img->pxlfmt != p_dst_mg->pxlfmt)
	    {
	        DBG_ERR("p_src_img->pxlfmt %d !=  p_dst_img->pxlfmt %d\r\n",p_src_img->pxlfmt,p_dst_img->pxlfmt);
	        return E_PAR;
	    }
	*/
	if (p_dst_location) {
		tmp_pos.x = p_dst_location->x;
		tmp_pos.y = p_dst_location->y;
		GXIMG_SCALE_POINT(p_dst_img, &tmp_pos);
	} else {
		tmp_pos.x = 0;
		tmp_pos.y = 0;
	}
	if (p_src_region == GXIMG_REGION_MATCH_IMG) {
		p_out_src_rect->x = 0;
		p_out_src_rect->y = 0;
		p_out_src_rect->w = p_src_img->size.w;
		p_out_src_rect->h = p_src_img->size.h;
	} else {
		if ((p_src_region->x + p_src_region->w > (INT32)p_src_img->size.w) ||
			(p_src_region->y + p_src_region->h > (INT32)p_src_img->size.h)) {
			DBG_ERR("Invalid src region (Src %dx%d, SrcRegion %d %d %d %d)\r\n",
					p_src_img->size.w, p_src_img->size.h, p_src_region->x, p_src_region->y, p_src_region->w, p_src_region->h);
			return E_PAR;
		}

		p_out_src_rect->x = p_src_region->x;
		p_out_src_rect->y = p_src_region->y;
		p_out_src_rect->w = p_src_region->w;
		p_out_src_rect->h = p_src_region->h;
		GXIMG_SCALE_RECT(p_src_img, p_out_src_rect);
	}
	if (p_out_src_rect->w <= 0 || p_out_src_rect->h <= 0) {
		DBG_ERR("SrcRgn x=%d, y=%d, w=%d, h=%d\r\n", p_out_src_rect->x, p_out_src_rect->y, p_out_src_rect->w, p_out_src_rect->h);
		return E_PAR;
	} else if (p_out_src_rect->x > (INT32)p_src_img->size.w) {
		DBG_ERR("SrcRgn x=%d > img w=%d\r\n", p_out_src_rect->x, p_src_img->size.w);
		return E_PAR;
	} else if (p_out_src_rect->y > (INT32)p_src_img->size.h) {
		DBG_ERR("SrcRgn y=%d > img h=%d\r\n", p_out_src_rect->y, p_src_img->size.h);
		return E_PAR;
	}
	tmp_dst_rect.w = p_dst_img->size.w;
	tmp_dst_rect.h = p_dst_img->size.h;
	if (!gximg_calc_clipped_rect(p_out_src_rect, &tmp_pos, &tmp_dst_rect, p_out_dst_rect)) {
		DBG_ERR("Clip Rect NULL\r\n");
		return E_PAR;
	}
	gximg_adjust_uv_packed_align(p_src_img->pxlfmt, p_out_src_rect);
	gximg_adjust_uv_packed_align(p_dst_img->pxlfmt, p_out_dst_rect);
	DBG_MSG("[copy]SrcRgn x=%04d, y=%04d, w=%04d, h=%04d \r\n", p_out_src_rect->x, p_out_src_rect->y, p_out_src_rect->w, p_out_src_rect->h);
	DBG_MSG("[copy]DstRgn x=%04d, y=%04d, w=%04d, h=%04d \r\n", p_out_dst_rect->x, p_out_dst_rect->y, p_out_dst_rect->w, p_out_dst_rect->h);
	return E_OK;
}

static ER _gximg_copy_data_ex_with_flush(PVDO_FRAME p_src_img, IRECT    *p_src_region, PVDO_FRAME p_dst_img, IPOINT    *p_dst_location, GXIMG_CP_ENG engine, int no_flush, int usePA)
{
	UINT32 i, plane_num;
	IRECT     tmp_src_rect = {0}, tmp_src_plane_rect = {0}, tmp_dst_rect = {0}, tmp_dst_plane_rect = {0};
	UINT32    grph_id;

	DBG_FUNC_BEGIN("[copy]\r\n");

	if (p_src_img->pxlfmt != p_dst_img->pxlfmt) {
		DBG_ERR("p_src_img->pxlfmt %d !=  p_dst_img->pxlfmt %d\r\n", p_src_img->pxlfmt, p_dst_img->pxlfmt);
		return E_PAR;
	}

	if (gximg_get_copy_rect(p_src_img, p_src_region, p_dst_location, p_dst_img, &tmp_src_rect, &tmp_dst_rect) != E_OK) {
		return E_PAR;
	}
	DBG_MSG("[copy] p_src_img.W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], p_src_img->loff[1], p_src_img->loff[2], p_src_img->pxlfmt);
	DBG_MSG("[copy] p_dst_img.W=%04d,H=%04d,OutOff[0]=%04d,OutOff[1]=%04d,OutOff[2]=%04d,PxlFmt=%x\r\n", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], p_dst_img->loff[1], p_dst_img->loff[2], p_dst_img->pxlfmt);

	switch (p_dst_img->pxlfmt) {
	case VDO_PXLFMT_YUV422_PLANAR:
	case VDO_PXLFMT_YUV420_PLANAR:
		plane_num = 3;
		break;

	case VDO_PXLFMT_YUV422:
	case VDO_PXLFMT_YUV420:
	case VDO_PXLFMT_ARGB8565:
		plane_num = 2;
		break;

	case VDO_PXLFMT_Y8:
	case VDO_PXLFMT_I8:
		plane_num = 1;
		break;

	case VDO_PXLFMT_RGB888_PLANAR:
		plane_num = 3;
		break;

	case VDO_PXLFMT_ARGB8888:
	case VDO_PXLFMT_ARGB1555:
	case VDO_PXLFMT_ARGB4444:
	case VDO_PXLFMT_RGB565:
		plane_num = 1;
		break;

	default:
		DBG_ERR("pixel format %x\r\n", p_dst_img->pxlfmt);
		return E_PAR;
	}
	grph_id = gximg_copy_map_to_grph_id(engine);
	DBG_MSG("[copy]grph_id = %d\r\n", grph_id);

	// use grph to copy
	if (tmp_dst_rect.w >= 2 && tmp_dst_rect.h >= 2) {
		if(gximg_open_grph(grph_id) == -1){
			DBG_ERR("fail to open graphic engine(%d)\n", grph_id);
			return E_PAR;
		}

		for (i = 0; i < plane_num; i++) {
			gximg_calc_plane_rect(p_dst_img, i, &tmp_src_rect, &tmp_src_plane_rect);
			gximg_calc_plane_rect(p_dst_img, i, &tmp_dst_rect, &tmp_dst_plane_rect);
#if USE_GRPH
			{
				KDRV_GRPH_TRIGGER_PARAM request = {0};
				GRPH_IMG images[3];

				gximg_memset(images, 0, sizeof(images));

				request.command = GRPH_CMD_A_COPY;
				request.format = GRPH_FORMAT_8BITS;
				request.is_skip_cache_flush = check_no_flush(usePA, no_flush);
				request.is_buf_pa = usePA;
				request.p_images = &(images[0]);
				images[0].img_id =             GRPH_IMG_ID_A;       // source image
				images[0].dram_addr = (UINT32)p_src_img->addr[i] + gximg_calc_plane_offset(p_src_img, i, &tmp_src_plane_rect);
				images[0].lineoffset =     p_src_img->loff[i];
				images[0].width =         tmp_src_plane_rect.w;
				images[0].height =         tmp_src_plane_rect.h;
				images[0].p_next =             &(images[1]);
				images[1].img_id =         GRPH_IMG_ID_C;       // destination image
				images[1].dram_addr = (UINT32)p_dst_img->addr[i] + gximg_calc_plane_offset(p_dst_img, i, &tmp_dst_plane_rect);
				images[1].lineoffset =     p_dst_img->loff[i];
				images[1].width =         tmp_dst_plane_rect.w;
				images[1].height =         tmp_dst_plane_rect.h;

				if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
					gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_C)){
					DBG_ERR("limit check fail\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
				if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
					DBG_ERR("fail to trigger graphic engine\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
			}
#else
			{
				INT32 j;
				for (j = 0; j < tmp_src_plane_rect.h; j++) {
					gximg_memcpy((UINT8 *)p_dst_img->addr[i] + gximg_calc_plane_offset(p_dst_img, i, &tmp_dst_plane_rect) + j * p_dst_img->loff[i], (UINT8 *)p_dst_img->addr[i] + gximg_calc_plane_offset(p_src_img, i, &tmp_src_plane_rect) + j * p_src_img->loff[i], tmp_src_plane_rect.w);
				}

			}
#endif
		}
		if(gximg_close_grph(grph_id) == -1){
			DBG_ERR("fail to close graphic engine\n");
			return E_PAR;
		}
	} else {
		DBG_ERR("valid_w=%d valid_h=%d\r\n", tmp_dst_rect.w, tmp_dst_rect.h);
		return E_PAR;
	}
	DBG_FUNC_END("[copy]\r\n");
	return E_OK;
}

ER gximg_copy_data(PVDO_FRAME p_src_img, IRECT    *p_src_region, PVDO_FRAME p_dst_img, IPOINT    *p_dst_location, GXIMG_CP_ENG engine, int usePA)
{
	return _gximg_copy_data_ex_with_flush(p_src_img, p_src_region, p_dst_img, p_dst_location, engine, 0, usePA);
}

ER gximg_copy_data_no_flush(PVDO_FRAME p_src_img, IRECT    *p_src_region, PVDO_FRAME p_dst_img, IPOINT    *p_dst_location, GXIMG_CP_ENG engine, int usePA)
{
	return _gximg_copy_data_ex_with_flush(p_src_img, p_src_region, p_dst_img, p_dst_location, engine, 1, usePA);
}

ER gximg_copy_data_ex(PVDO_FRAME p_src_img, IRECT    *p_src_region, PVDO_FRAME p_dst_img, IPOINT    *p_dst_location, GXIMG_CP_ENG engine)
{
	return  _gximg_copy_data_ex_with_flush(p_src_img, p_src_region, p_dst_img, p_dst_location, engine, 0, 0);
}

ER gximg_copy_color_key_data(PVDO_FRAME p_src_img, IRECT *p_copy_region, PVDO_FRAME p_key_img, IPOINT *p_key_location, UINT32 color_key, BOOL is_copy_to_key_img, GXIMG_CP_ENG engine, int usePA)
{
	UINT32 y, cb, cr;//, CbCr;
	IRECT  tmp_src_rect = {0}, tmp_dst_rect = {0}, tmp_key_rect = {0};
	UINT32 grph_id;

	DBG_FUNC_BEGIN("[copy]\r\n");
	DBG_MSG("[copy]colorkey = 0x%x\r\n", color_key);

	if (p_src_img->pxlfmt != p_key_img->pxlfmt) {
		DBG_ERR("p_src_img->PxlFmt(%d) != p_key_img->PxlFmt(%d)\r\n", p_src_img->pxlfmt, p_key_img->pxlfmt);
		return E_PAR;
	}

	if (gximg_get_copy_rect(p_src_img, p_copy_region, p_key_location, p_key_img, &tmp_src_rect, &tmp_key_rect) != E_OK) {
		return E_PAR;
	}

	grph_id = gximg_copy_map_to_grph_id(engine);
	DBG_MSG("[copy]grph_id = %d\r\n", grph_id);
	DBG_MSG("[copy]p_src_img.W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], p_src_img->loff[1], p_src_img->loff[2], p_src_img->pxlfmt);
	DBG_MSG("[copy]p_key_img.W=%04d,H=%04d,OutOff[0]=%04d,OutOff[1]=%04d,OutOff[2]=%04d,PxlFmt=%x\r\n", p_key_img->size.w, p_key_img->size.h, p_key_img->loff[0], p_key_img->loff[1], p_key_img->loff[2], p_key_img->pxlfmt);

	if (is_copy_to_key_img) {
		tmp_dst_rect = tmp_key_rect;
	} else {
		tmp_dst_rect = tmp_src_rect;
	}

	y = (color_key & 0x000000FF);
	y = (y + (y << 8) + (y << 16) + (y << 24));
	cb = ((color_key & 0x0000FF00) >> 8);
	cb = cb + (cb << 8) + (cb << 16) + (cb << 24);
	cr = ((color_key & 0x00FF0000) >> 16);
	cr = cr + (cr << 8) + (cr << 16) + (cr << 24);
	//cbcr = ((colorkey& 0x0000FF00) >>8) + ((colorkey& 0x00FF0000) >> 8) ;
	//cbcr = cbcr + (cbcr << 16);

	// use grph to copy
	if (tmp_dst_rect.w >= 2 && tmp_dst_rect.h >= 2) {
#if USE_GRPH
		IRECT tmp_src_plane_rect = {0}, tmp_dst_plane_rect = {0}, tmp_key_plane_rect = {0};
		UINT32 p_key_value[GXIMG_MAX_PLANE_NUM];
		UINT32 i=0, plane_num=0;
		PVDO_FRAME p_dst_img;
		UINT32 grph_fmt=0;
		UINT32 color_r=0,color_g=0,color_b=0;

		if (is_copy_to_key_img) {
			p_dst_img = p_key_img;
		} else {
			p_dst_img = p_src_img;
		}

		switch (p_src_img->pxlfmt) {
		case VDO_PXLFMT_YUV422_PLANAR:
		case VDO_PXLFMT_YUV420_PLANAR:
			p_key_value[0] = y;
			p_key_value[1] = cb;
			p_key_value[2] = cr;
			plane_num = 3;
			grph_fmt = GRPH_FORMAT_8BITS;
			break;
		case VDO_PXLFMT_YUV422:
		case VDO_PXLFMT_YUV420:
			plane_num = 2;
			break;

		case VDO_PXLFMT_Y8:
		case VDO_PXLFMT_I8:
			p_key_value[0] = y;
			plane_num = 1;
			grph_fmt = GRPH_FORMAT_8BITS;
			break;
		case VDO_PXLFMT_RGB565:
			p_key_value[0] = color_key;
			plane_num = 1;
			grph_fmt = GRPH_FORMAT_16BITS;
			break;
		case VDO_PXLFMT_ARGB1555:
			p_key_value[0] = y;
			plane_num = 1;
			grph_fmt = GRPH_FORMAT_16BITS_ARGB1555_RGB;
			color_r = COLOR_RGBA5551_GET_R(color_key);
			color_g = COLOR_RGBA5551_GET_G(color_key);
			color_b = COLOR_RGBA5551_GET_B(color_key);
			break;
		case VDO_PXLFMT_ARGB4444:
			p_key_value[0] = y;
			plane_num = 1;
			grph_fmt = GRPH_FORMAT_16BITS_ARGB4444_RGB;
			color_r = COLOR_RGB444_GET_R(color_key);
			color_g = COLOR_RGB444_GET_G(color_key);
			color_b = COLOR_RGB444_GET_B(color_key);
			break;
		case VDO_PXLFMT_ARGB8888:
			p_key_value[0] = y;
			plane_num = 1;
			grph_fmt = GRPH_FORMAT_32BITS_ARGB8888_RGB;
			color_r = ((color_key & 0x00FF0000) >> 16);
			color_g = ((color_key & 0x0000FF00) >> 8);
			color_b = (color_key & 0x000000FF);
			break;
		default:
			DBG_ERR("pixel format %x\r\n", p_dst_img->pxlfmt);
			return E_PAR;
		}

		if(gximg_open_grph(grph_id) == -1){
			DBG_ERR("fail to open graphic engine(%d)\n", grph_id);
			return E_PAR;
		}

		if (p_src_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR ||
		    p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR ||
		    p_src_img->pxlfmt == VDO_PXLFMT_RGB565 ||
			p_src_img->pxlfmt == VDO_PXLFMT_Y8 ||
			p_src_img->pxlfmt == VDO_PXLFMT_I8) {
			for (i = 0; i < plane_num; i++) {
				gximg_calc_plane_rect(p_src_img, i, &tmp_src_rect, &tmp_src_plane_rect);
				gximg_calc_plane_rect(p_key_img, i, &tmp_key_rect, &tmp_key_plane_rect);
				gximg_calc_plane_rect(p_dst_img, i, &tmp_dst_rect, &tmp_dst_plane_rect);
				{
					KDRV_GRPH_TRIGGER_PARAM request = {0};
					GRPH_IMG images[3];
					GRPH_PROPERTY property[1];

					gximg_memset(images, 0, sizeof(images));
					gximg_memset(property, 0, sizeof(property));

					//request.command = GRPH_CMD_COLOR_LE;
					request.command = GRPH_CMD_COLOR_EQ;
					request.format = grph_fmt;
					request.is_skip_cache_flush = check_no_flush(usePA, 0);
					request.is_buf_pa = usePA;
					request.p_images = &(images[0]);
					request.p_property = &(property[0]);
					images[0].img_id =             GRPH_IMG_ID_A;       // source image
					images[0].dram_addr = (UINT32)p_src_img->addr[i] + gximg_calc_plane_offset(p_src_img, i, &tmp_src_plane_rect);
					images[0].lineoffset =     p_src_img->loff[i];
					images[0].width =         tmp_src_plane_rect.w;
					images[0].height =         tmp_src_plane_rect.h;
					images[0].p_next =             &(images[1]);

					images[1].img_id =         GRPH_IMG_ID_B;       // destination image
					images[1].dram_addr = (UINT32)p_key_img->addr[i] + gximg_calc_plane_offset(p_key_img, i, &tmp_key_plane_rect);
					images[1].lineoffset =     p_key_img->loff[i];
					images[1].width =         tmp_key_plane_rect.w;
					images[1].height =         tmp_key_plane_rect.h;
					images[1].p_next =             &(images[2]);

					images[2].img_id =         GRPH_IMG_ID_C;       // destination image
					images[2].dram_addr = (UINT32)p_dst_img->addr[i] + gximg_calc_plane_offset(p_dst_img, i, &tmp_dst_plane_rect);
					images[2].lineoffset =     p_dst_img->loff[i];
					images[2].width =         tmp_dst_plane_rect.w;
					images[2].height =         tmp_dst_plane_rect.h;

					property[0].id =             GRPH_PROPERTY_ID_NORMAL;
					property[0].property =     p_key_value[i];
					property[0].p_next = NULL;

					if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
						gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_B) ||
						gximg_check_grph_limit(request.command, request.format, &(images[2]), 1, GRPH_IMG_ID_C)){
						DBG_ERR("limit check fail\n");
						gximg_close_grph(grph_id);
						return E_PAR;
					}
					if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
						DBG_ERR("fail to start graphic engine(%d)\n", grph_id);
						gximg_close_grph(grph_id);
						return E_PAR;
					}
				}
			}
		} else if (p_src_img->pxlfmt == VDO_PXLFMT_YUV422 || p_src_img->pxlfmt == VDO_PXLFMT_YUV420) {
			{
				KDRV_GRPH_TRIGGER_PARAM request = {0};
				GRPH_IMG images[3];
				GRPH_PROPERTY property[2];

				gximg_memset(images, 0, sizeof(images));
				gximg_memset(property, 0, sizeof(property));

				gximg_memset(images, 0, sizeof(images));
				gximg_memset(property, 0, sizeof(property));

				//request.command = GRPH_CMD_COLOR_LE; //(B < KEY) ? A:B -> destination with color key (<)
				request.command = GRPH_CMD_COLOR_EQ;  //(B == KEY) ? A:B -> destination with color key (=)
				request.format = GRPH_FORMAT_8BITS;
				request.is_skip_cache_flush = check_no_flush(usePA, 0);
				request.is_buf_pa = usePA;
				request.p_images = &(images[0]);
				request.p_property = &(property[0]);

				gximg_calc_plane_rect(p_src_img, 0, &tmp_src_rect, &tmp_src_plane_rect);
				gximg_calc_plane_rect(p_key_img, 0, &tmp_key_rect, &tmp_key_plane_rect);
				gximg_calc_plane_rect(p_dst_img, 0, &tmp_dst_rect, &tmp_dst_plane_rect);

				images[0].img_id =             GRPH_IMG_ID_A;       // source image
				images[0].dram_addr = (UINT32)p_src_img->addr[0] + gximg_calc_plane_offset(p_src_img, 0, &tmp_src_plane_rect);
				images[0].lineoffset =     p_src_img->loff[0];
				images[0].width =         tmp_src_plane_rect.w;
				images[0].height =         tmp_src_plane_rect.h;
				images[0].p_next =             &(images[1]);

				images[1].img_id =         GRPH_IMG_ID_B;       // destination image
				images[1].dram_addr = (UINT32)p_key_img->addr[0] + gximg_calc_plane_offset(p_key_img, 0, &tmp_key_plane_rect);
				images[1].lineoffset =     p_key_img->loff[0];
				images[1].width =         tmp_key_plane_rect.w;
				images[1].height =         tmp_key_plane_rect.h;
				images[1].p_next =             &(images[2]);

				images[2].img_id =         GRPH_IMG_ID_C;       // destination image
				images[2].dram_addr = (UINT32)p_dst_img->addr[0] + gximg_calc_plane_offset(p_dst_img, 0, &tmp_dst_plane_rect);
				images[2].lineoffset =     p_dst_img->loff[0];
				images[2].width =         tmp_dst_plane_rect.w;
				images[2].height =         tmp_dst_plane_rect.h;
				property[0].id =             GRPH_PROPERTY_ID_NORMAL;
				property[0].property =     y;
				property[0].p_next = NULL;

				if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
					gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_B) ||
					gximg_check_grph_limit(request.command, request.format, &(images[2]), 1, GRPH_IMG_ID_C)){
					DBG_ERR("limit check fail\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
				if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
					DBG_ERR("fail to start graphic engine(%d)\n", grph_id);
					gximg_close_grph(grph_id);
					return E_PAR;
				}

				gximg_calc_plane_rect(p_dst_img, 1, &tmp_src_rect, &tmp_src_plane_rect);
				gximg_calc_plane_rect(p_key_img, 1, &tmp_key_rect, &tmp_key_plane_rect);
				gximg_calc_plane_rect(p_dst_img, 1, &tmp_dst_rect, &tmp_dst_plane_rect);
				request.format = GRPH_FORMAT_16BITS_UVPACK;
				images[0].img_id =             GRPH_IMG_ID_A;       // source image
				images[0].dram_addr = (UINT32)p_src_img->addr[1] + gximg_calc_plane_offset(p_src_img, 1, &tmp_src_plane_rect);
				images[0].lineoffset =     p_src_img->loff[1];
				images[0].width =         tmp_src_plane_rect.w;
				images[0].height =         tmp_src_plane_rect.h;
				images[0].p_next =             &(images[1]);

				images[1].img_id =         GRPH_IMG_ID_B;       // destination image
				images[1].dram_addr = (UINT32)p_key_img->addr[1] + gximg_calc_plane_offset(p_key_img, 1, &tmp_key_plane_rect);
				images[1].lineoffset =     p_key_img->loff[1];
				images[1].width =         tmp_key_plane_rect.w;
				images[1].height =         tmp_key_plane_rect.h;
				images[1].p_next =             &(images[2]);

				images[2].img_id =         GRPH_IMG_ID_C;       // destination image
				images[2].dram_addr = (UINT32)p_dst_img->addr[1] + gximg_calc_plane_offset(p_dst_img, 1, &tmp_dst_plane_rect);
				images[2].lineoffset =     p_dst_img->loff[1];
				images[2].width =         tmp_dst_plane_rect.w;
				images[2].height =         tmp_dst_plane_rect.h;
				images[2].p_next =             NULL;

				property[0].id =             GRPH_PROPERTY_ID_U;
				property[0].property =     cb;
				property[0].p_next = &(property[1]);

				property[1].id =             GRPH_PROPERTY_ID_V;
				property[1].property =     cr;
				property[1].p_next = NULL;

				if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
					gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_B) ||
					gximg_check_grph_limit(request.command, request.format, &(images[2]), 1, GRPH_IMG_ID_C)){
					DBG_ERR("limit check fail\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
				if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
					DBG_ERR("fail to start graphic engine(%d)\n", grph_id);
					gximg_close_grph(grph_id);
					return E_PAR;
				}

			}
		}else if ((p_src_img->pxlfmt == VDO_PXLFMT_ARGB1555)||
		(p_src_img->pxlfmt == VDO_PXLFMT_ARGB8888) ||(p_src_img->pxlfmt == VDO_PXLFMT_ARGB4444)){

			gximg_calc_plane_rect(p_src_img, 0, &tmp_src_rect, &tmp_src_plane_rect);
			gximg_calc_plane_rect(p_key_img, 0, &tmp_key_rect, &tmp_key_plane_rect);
			gximg_calc_plane_rect(p_dst_img, 0, &tmp_dst_rect, &tmp_dst_plane_rect);
			{
				KDRV_GRPH_TRIGGER_PARAM request = {0};
				GRPH_IMG images[3];
				GRPH_PROPERTY property[3];

				gximg_memset(images, 0, sizeof(images));
				gximg_memset(property, 0, sizeof(property));

				request.command = GRPH_CMD_COLOR_EQ;
				request.format = grph_fmt;
				request.is_skip_cache_flush = check_no_flush(usePA, 0);
				request.is_buf_pa = usePA;
				request.p_images = &(images[0]);
				request.p_property = &(property[0]);
				images[0].img_id =             GRPH_IMG_ID_A;       // source image
				images[0].dram_addr = (UINT32)p_src_img->addr[0] + gximg_calc_plane_offset(p_src_img, i, &tmp_src_plane_rect);
				images[0].lineoffset =     p_src_img->loff[0];
				images[0].width =         tmp_src_plane_rect.w;
				images[0].height =         tmp_src_plane_rect.h;
				images[0].p_next =             &(images[1]);

				images[1].img_id =         GRPH_IMG_ID_B;       // destination image
				images[1].dram_addr = (UINT32)p_key_img->addr[0] + gximg_calc_plane_offset(p_key_img, i, &tmp_key_plane_rect);
				images[1].lineoffset =     p_key_img->loff[0];
				images[1].width =         tmp_key_plane_rect.w;
				images[1].height =         tmp_key_plane_rect.h;
				images[1].p_next =             &(images[2]);

				images[2].img_id =         GRPH_IMG_ID_C;       // destination image
				images[2].dram_addr = (UINT32)p_dst_img->addr[0] + gximg_calc_plane_offset(p_dst_img, i, &tmp_dst_plane_rect);
				images[2].lineoffset =     p_dst_img->loff[0];
				images[2].width =         tmp_dst_plane_rect.w;
				images[2].height =         tmp_dst_plane_rect.h;

				property[0].id = GRPH_PROPERTY_ID_B;
				property[0].property = color_b;
				property[0].p_next = &(property[1]);
				property[1].id = GRPH_PROPERTY_ID_G;
				property[1].property = color_g;
				property[1].p_next = &(property[2]);
				property[2].id = GRPH_PROPERTY_ID_R;
				property[2].property = color_r;
				property[2].p_next =  0;

				if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
					gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_B) ||
					gximg_check_grph_limit(request.command, request.format, &(images[2]), 1, GRPH_IMG_ID_C)){
					DBG_ERR("limit check fail\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
				if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
					DBG_ERR("fail to start graphic engine(%d)\n", grph_id);
					gximg_close_grph(grph_id);
					return E_PAR;
				}
			}
		}
		if(gximg_close_grph(grph_id) == -1){
			DBG_ERR("fail to close graphic engine(%d)\n", grph_id);
			return E_PAR;
		}
#endif
	} else {
		DBG_ERR("valid_w=%d valid_h=%d\r\n", tmp_dst_rect.w, tmp_dst_rect.h);
		return E_PAR;
	}
	DBG_FUNC_END("[copy]\r\n");
	return E_OK;
}

static ER _gximg_copy_blend_data_with_flush(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 alpha, GXIMG_CP_ENG engine, int no_flush, int usePA)
{
	IRECT     tmp_src_rect = {0}, tmp_dst_rect = {0};
	UINT32 grph_id;

	DBG_FUNC_BEGIN("[copy]\r\n");
	DBG_MSG("[copy]alpha = %d\r\n", alpha);
	if (alpha > 255) {
		//SrcAlpha>8, not support
		DBG_ERR("alpha %d > 255 Not support\r\n", alpha);
		return E_PAR;
	} else if (alpha == 0) {
		//SrcAlpha=0, COPY B
		return E_OK;
	} else if (alpha == 255) {
		//SrcAlpha=0, COPY A
		return gximg_copy_data(p_src_img, p_src_region, p_dst_img, p_dst_location, engine, usePA);
	} else {
		//SrcAlpha=N, BLEND AB
		alpha = GRPH_BLD_WA_WB_THR(alpha, (255 - alpha), 0xFF);
	}

	if (p_src_img->pxlfmt != p_dst_img->pxlfmt) {
		DBG_ERR("p_src_img->pxlfmt %d !=  p_dst_img->pxlfmt %d\r\n", p_src_img->pxlfmt, p_dst_img->pxlfmt);
		return E_PAR;
	}

	if (gximg_get_copy_rect(p_src_img, p_src_region, p_dst_location, p_dst_img, &tmp_src_rect, &tmp_dst_rect) != E_OK) {
		return E_PAR;
	}

	grph_id = gximg_copy_map_to_grph_id(engine);
	DBG_MSG("[copy]grph_id = %d\r\n", grph_id);

	DBG_MSG("[copy]p_src_img.W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,pxlfmt=%x\r\n", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], p_src_img->loff[1], p_src_img->loff[2], p_src_img->pxlfmt);
	DBG_MSG("[copy]p_dst_img.W=%04d,H=%04d,OutOff[0]=%04d,OutOff[1]=%04d,OutOff[2]=%04d,pxlfmt=%x\r\n", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], p_dst_img->loff[1], p_dst_img->loff[2], p_dst_img->pxlfmt);

	// use grph to copy
	if (tmp_dst_rect.w >= 2 && tmp_dst_rect.h >= 2) {
#if USE_GRPH
		IRECT tmp_src_plane_rect = {0}, tmp_dst_plane_rect = {0};
		UINT32 i, plane_num;

		switch (p_dst_img->pxlfmt) {
		case VDO_PXLFMT_YUV422_PLANAR:
		case VDO_PXLFMT_YUV420_PLANAR:
			plane_num = 3;
			break;
			break;
		case VDO_PXLFMT_YUV422:
		case VDO_PXLFMT_YUV420:
			plane_num = 2;
			break;

		case VDO_PXLFMT_Y8:
		case VDO_PXLFMT_I8:
			plane_num = 1;
			break;
		default:
			DBG_ERR("pixel format %x\r\n", p_dst_img->pxlfmt);
			return E_PAR;
		}

		if(gximg_open_grph(grph_id) == -1){
			DBG_ERR("fail to open graphic engine(%d)\n", grph_id);
			return E_PAR;
		}

		if (p_dst_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR ||
		    p_dst_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR ||
			p_dst_img->pxlfmt == VDO_PXLFMT_Y8 ||
			p_dst_img->pxlfmt == VDO_PXLFMT_I8) {
			for (i = 0; i < plane_num; i++) {
				gximg_calc_plane_rect(p_src_img, i, &tmp_src_rect, &tmp_src_plane_rect);
				gximg_calc_plane_rect(p_dst_img, i, &tmp_dst_rect, &tmp_dst_plane_rect);
				{
					KDRV_GRPH_TRIGGER_PARAM request = {0};
					GRPH_IMG images[3];
					GRPH_PROPERTY property[1];

					gximg_memset(images, 0, sizeof(images));
					gximg_memset(property, 0, sizeof(property));

					request.command = GRPH_CMD_BLENDING;
					request.format = GRPH_FORMAT_8BITS;
					request.is_skip_cache_flush = check_no_flush(usePA, no_flush);
					request.is_buf_pa = usePA;
					request.p_images = &(images[0]);
					request.p_property = &(property[0]);
					images[0].img_id =             GRPH_IMG_ID_A;       // source image
					images[0].dram_addr = (UINT32)p_src_img->addr[i] + gximg_calc_plane_offset(p_src_img, i, &tmp_src_plane_rect);
					images[0].lineoffset =     p_src_img->loff[i];
					images[0].width =         tmp_src_plane_rect.w;
					images[0].height =         tmp_src_plane_rect.h;
					images[0].p_next =             &(images[1]);

					images[1].img_id =         GRPH_IMG_ID_B;       // destination image
					images[1].dram_addr = (UINT32)p_dst_img->addr[i] + gximg_calc_plane_offset(p_dst_img, i, &tmp_dst_plane_rect);
					images[1].lineoffset =     p_dst_img->loff[i];
					images[1].width =         tmp_dst_plane_rect.w;
					images[1].height =         tmp_dst_plane_rect.h;
					images[1].p_next =             &(images[2]);

					images[2].img_id =         GRPH_IMG_ID_C;       // destination image
					images[2].dram_addr = (UINT32)p_dst_img->addr[i] + gximg_calc_plane_offset(p_dst_img, i, &tmp_dst_plane_rect);
					images[2].lineoffset =     p_dst_img->loff[i];
					images[2].width =         tmp_src_plane_rect.w;
					images[2].height =         tmp_src_plane_rect.h;
					property[0].id =             GRPH_PROPERTY_ID_NORMAL;
					property[0].property =     alpha;
					property[0].p_next = NULL;

					if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
						gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_B) ||
						gximg_check_grph_limit(request.command, request.format, &(images[2]), 1, GRPH_IMG_ID_C)){
						DBG_ERR("limit check fail\n");
						gximg_close_grph(grph_id);
						return E_PAR;
					}
					if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
						DBG_ERR("fail to start graphic engine(%d)\n", grph_id);
						gximg_close_grph(grph_id);
						return E_PAR;
					}
				}
			}
		} else if (p_dst_img->pxlfmt == VDO_PXLFMT_YUV422 || p_dst_img->pxlfmt == VDO_PXLFMT_YUV420) {

			{
				KDRV_GRPH_TRIGGER_PARAM request = {0};
				GRPH_IMG images[3];
				GRPH_PROPERTY property[2];

				gximg_memset(images, 0, sizeof(images));
				gximg_memset(property, 0, sizeof(property));

				gximg_calc_plane_rect(p_src_img, 0, &tmp_src_rect, &tmp_src_plane_rect);
				gximg_calc_plane_rect(p_dst_img, 0, &tmp_dst_rect, &tmp_dst_plane_rect);

				request.command = GRPH_CMD_BLENDING;
				request.format = GRPH_FORMAT_8BITS;
				request.is_skip_cache_flush = check_no_flush(usePA, no_flush);
				request.is_buf_pa = usePA;
				request.p_images = &(images[0]);
				request.p_property = &(property[0]);
				images[0].img_id =             GRPH_IMG_ID_A;       // source image
				images[0].dram_addr = (UINT32)p_src_img->addr[0] + gximg_calc_plane_offset(p_src_img, 0, &tmp_src_plane_rect);
				images[0].lineoffset =     p_src_img->loff[0];
				images[0].width =         tmp_src_plane_rect.w;
				images[0].height =         tmp_src_plane_rect.h;
				images[0].p_next =             &(images[1]);

				images[1].img_id =         GRPH_IMG_ID_B;       // destination image
				images[1].dram_addr = (UINT32)p_dst_img->addr[0] + gximg_calc_plane_offset(p_dst_img, 0, &tmp_dst_plane_rect);
				images[1].lineoffset =     p_dst_img->loff[0];
				images[1].width =         tmp_dst_plane_rect.w;
				images[1].height =         tmp_dst_plane_rect.h;
				images[1].p_next =             &(images[2]);

				images[2].img_id =         GRPH_IMG_ID_C;       // destination image
				images[2].dram_addr = (UINT32)p_dst_img->addr[0] + gximg_calc_plane_offset(p_dst_img, 0, &tmp_dst_plane_rect);
				images[2].lineoffset =     p_dst_img->loff[0];
				images[2].width =         tmp_dst_plane_rect.w;
				images[2].height =         tmp_dst_plane_rect.h;
				property[0].id =             GRPH_PROPERTY_ID_NORMAL;
				property[0].property =     alpha;
				property[0].p_next = NULL;

				if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
					gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_B) ||
					gximg_check_grph_limit(request.command, request.format, &(images[2]), 1, GRPH_IMG_ID_C)){
					DBG_ERR("limit check fail\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
				if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
					DBG_ERR("fail to start graphic engine(%d)\n", grph_id);
					gximg_close_grph(grph_id);
					return E_PAR;
				}


				gximg_calc_plane_rect(p_src_img, 1, &tmp_src_rect, &tmp_src_plane_rect);
				gximg_calc_plane_rect(p_dst_img, 1, &tmp_dst_rect, &tmp_dst_plane_rect);

				request.format = GRPH_FORMAT_16BITS_UVPACK;
				images[0].img_id =             GRPH_IMG_ID_A;       // source image
				images[0].dram_addr = (UINT32)p_src_img->addr[1] + gximg_calc_plane_offset(p_src_img, 1, &tmp_src_plane_rect);
				images[0].lineoffset =     p_src_img->loff[1];
				images[0].width =         tmp_src_plane_rect.w;
				images[0].height =         tmp_src_plane_rect.h;
				images[0].p_next =             &(images[1]);

				images[1].img_id =         GRPH_IMG_ID_B;       // destination image
				images[1].dram_addr = (UINT32)p_dst_img->addr[1] + gximg_calc_plane_offset(p_dst_img, 1, &tmp_dst_plane_rect);
				images[1].lineoffset =     p_dst_img->loff[1];
				images[1].width =         tmp_dst_plane_rect.w;
				images[1].height =         tmp_dst_plane_rect.h;
				images[1].p_next =             &(images[2]);

				images[2].img_id =         GRPH_IMG_ID_C;       // destination image
				images[2].dram_addr = (UINT32)p_dst_img->addr[1] + gximg_calc_plane_offset(p_dst_img, 1, &tmp_dst_plane_rect);
				images[2].lineoffset =     p_dst_img->loff[1];
				images[2].width =         tmp_dst_plane_rect.w;
				images[2].height =         tmp_dst_plane_rect.h;
				property[0].id =             GRPH_PROPERTY_ID_U;
				property[0].property =     alpha;
				property[0].p_next = &(property[1]);

				property[1].id =             GRPH_PROPERTY_ID_V;
				property[1].property =     alpha;
				property[1].p_next = NULL;

				if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
					gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_B) ||
					gximg_check_grph_limit(request.command, request.format, &(images[2]), 1, GRPH_IMG_ID_C)){
					DBG_ERR("limit check fail\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
				if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
					DBG_ERR("fail to start graphic engine(%d)\n", grph_id);
					gximg_close_grph(grph_id);
					return E_PAR;
				}

			}
		}
		if(gximg_close_grph(grph_id) == -1){
			DBG_ERR("fail to close graphic engine(%d)\n", grph_id);
			return E_PAR;
		}
#endif
	} else {
		DBG_ERR("valid_w=%d valid_h=%d\r\n", tmp_dst_rect.w, tmp_dst_rect.h);
		return E_PAR;
	}
	DBG_FUNC_END("[copy]\r\n");
	return E_OK;
}

ER gximg_copy_blend_data(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 alpha, GXIMG_CP_ENG engine, int usePA)
{
	return _gximg_copy_blend_data_with_flush(p_src_img, p_src_region, p_dst_img, p_dst_location, alpha, engine, 0, usePA);
}

ER gximg_copy_blend_data_no_flush(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 alpha, GXIMG_CP_ENG engine, int usePA)
{
	return _gximg_copy_blend_data_with_flush(p_src_img, p_src_region, p_dst_img, p_dst_location, alpha, engine, 1, usePA);
}

static ER _gximg_copy_blend_data_ex_with_flush(PVDO_FRAME p_src_img, IRECT    *p_src_region, PVDO_FRAME p_dst_img, IPOINT    *p_dst_location, UINT8 *p_alpha_plane, int no_flush, int usePA)
{
	IRECT     tmp_src_rect = {0}, tmp_dst_rect = {0};
	UINT32    grph_id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0);

	DBG_FUNC_BEGIN("[copy]\r\n");
	if (p_dst_img->pxlfmt != VDO_PXLFMT_ARGB8888) {//yuv, Y8 need alpha plane
		if (!p_alpha_plane) {
			DBG_ERR("p_alpha_plane is NULL\r\n");
			return E_PAR;
		}
	}
	if (p_src_img->pxlfmt != p_dst_img->pxlfmt) {
		DBG_ERR("p_src_img->pxlfmt %d !=  p_dst_img->pxlfmt %d\r\n", p_src_img->pxlfmt, p_dst_img->pxlfmt);
		return E_PAR;
	}
	if (gximg_get_copy_rect(p_src_img, p_src_region, p_dst_location, p_dst_img, &tmp_src_rect, &tmp_dst_rect) != E_OK) {
		return E_PAR;
	}

	DBG_MSG("[copy]p_src_img.W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], p_src_img->loff[1], p_src_img->loff[2], p_src_img->pxlfmt);
	DBG_MSG("[copy]p_dst_img.W=%04d,H=%04d,OutOff[0]=%04d,OutOff[1]=%04d,OutOff[2]=%04d,PxlFmt=%x\r\n", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], p_dst_img->loff[1], p_dst_img->loff[2], p_dst_img->pxlfmt);

	// use grph to copy
	if (tmp_dst_rect.w >= 2 && tmp_dst_rect.h >= 2) {
#if USE_GRPH
		IRECT tmp_src_plane_rect = {0}, tmp_dst_plane_rect = {0};
		UINT32 i, plane_num;

		switch (p_dst_img->pxlfmt) {
		case VDO_PXLFMT_YUV422_PLANAR:
		case VDO_PXLFMT_YUV420_PLANAR:
			plane_num = 3;
			break;
			break;
		case VDO_PXLFMT_YUV422:
		case VDO_PXLFMT_YUV420:
			plane_num = 2;
			break;

		case VDO_PXLFMT_Y8:
		case VDO_PXLFMT_ARGB8888:
			plane_num = 1;
			break;
		default:
			DBG_ERR("pixel format %x\r\n", p_dst_img->pxlfmt);
			return E_PAR;
		}

		if( gximg_open_grph(grph_id) == -1){
			DBG_ERR("fail to open graphic engine(%d)\n", KDRV_GFX2D_GRPH0);
			return E_PAR;
		}

		if (p_dst_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR || p_dst_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR || p_dst_img->pxlfmt == VDO_PXLFMT_Y8) {
			for (i = 0; i < plane_num; i++) {
				gximg_calc_plane_rect(p_src_img, i, &tmp_src_rect, &tmp_src_plane_rect);
				gximg_calc_plane_rect(p_dst_img, i, &tmp_dst_rect, &tmp_dst_plane_rect);
				{
					KDRV_GRPH_TRIGGER_PARAM request = {0};
					GRPH_IMG images[3];
					GRPH_PROPERTY property[1];

					gximg_memset(images, 0, sizeof(images));
					gximg_memset(property, 0, sizeof(property));

					gximg_calc_plane_rect(p_src_img, 0, &tmp_src_rect, &tmp_src_plane_rect);
					gximg_calc_plane_rect(p_dst_img, 0, &tmp_dst_rect, &tmp_dst_plane_rect);

					request.command =  GRPH_CMD_PLANE_BLENDING;
					request.format = GRPH_FORMAT_8BITS;
					request.is_skip_cache_flush = check_no_flush(usePA, no_flush);
					request.is_buf_pa = usePA;
					request.p_images = &(images[0]);
					request.p_property = &(property[0]);
					images[0].img_id =             GRPH_IMG_ID_A;       // source image
					images[0].dram_addr = (UINT32)p_src_img->addr[i] + gximg_calc_plane_offset(p_src_img, i, &tmp_src_plane_rect);
					images[0].lineoffset =     p_src_img->loff[i];
					images[0].width =         tmp_src_plane_rect.w;
					images[0].height =         tmp_src_plane_rect.h;
					images[0].p_next =             &(images[1]);

					images[1].img_id =         GRPH_IMG_ID_B;       // destination image
					images[1].dram_addr = (UINT32)p_alpha_plane + gximg_calc_plane_offset(p_src_img, i, &tmp_src_plane_rect);
					images[1].lineoffset =     p_src_img->loff[i];
					images[1].width =         tmp_src_plane_rect.w;
					images[1].height =         tmp_src_plane_rect.h;
					images[1].p_next =             &(images[2]);

					images[2].img_id =         GRPH_IMG_ID_C;       // destination image
					images[2].dram_addr = (UINT32)p_dst_img->addr[i] + gximg_calc_plane_offset(p_dst_img, i, &tmp_dst_plane_rect);
					images[2].lineoffset =     p_dst_img->loff[i];
					images[2].width =         tmp_src_plane_rect.w;
					images[2].height =         tmp_src_plane_rect.h;
					property[0].id =             GRPH_PROPERTY_ID_NORMAL;
					property[0].property =     0;
					property[0].p_next = NULL;

					if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
						gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_B) ||
						gximg_check_grph_limit(request.command, request.format, &(images[2]), 1, GRPH_IMG_ID_C)){
						DBG_ERR("limit check fail\n");
						gximg_close_grph(grph_id);
						return E_PAR;
					}
					if(kdrv_grph_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0), &request, NULL, NULL) != E_OK){
						DBG_ERR("fail to start graphic engine(%d)\n", KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0));
						gximg_close_grph(grph_id);
						return E_PAR;
					}
				}
			}
		} else if (p_dst_img->pxlfmt == VDO_PXLFMT_YUV422 || p_dst_img->pxlfmt == VDO_PXLFMT_YUV420) {

			{
				KDRV_GRPH_TRIGGER_PARAM request = {0};
				GRPH_IMG images[3];
				GRPH_PROPERTY property[1];

				gximg_memset(images, 0, sizeof(images));
				gximg_memset(property, 0, sizeof(property));

				gximg_calc_plane_rect(p_src_img, 0, &tmp_src_rect, &tmp_src_plane_rect);
				gximg_calc_plane_rect(p_dst_img, 0, &tmp_dst_rect, &tmp_dst_plane_rect);

				request.command = GRPH_CMD_PLANE_BLENDING;
				request.format = GRPH_FORMAT_8BITS;
				request.is_skip_cache_flush = check_no_flush(usePA, no_flush);
				request.is_buf_pa = usePA;
				request.p_images = &(images[0]);
				request.p_property = &(property[0]);
				images[0].img_id =             GRPH_IMG_ID_A;       // source image
				images[0].dram_addr = (UINT32)p_src_img->addr[0] + gximg_calc_plane_offset(p_src_img, 0, &tmp_src_plane_rect);
				images[0].lineoffset =     p_src_img->loff[0];
				images[0].width =         tmp_src_plane_rect.w;
				images[0].height =         tmp_src_plane_rect.h;
				images[0].p_next =             &(images[1]);

				images[1].img_id =         GRPH_IMG_ID_B;         // alpha plane
				images[1].dram_addr = (UINT32)p_alpha_plane + gximg_calc_plane_offset(p_src_img, 0, &tmp_src_plane_rect);
				images[1].lineoffset =     p_src_img->loff[0];
				images[1].width =         tmp_src_plane_rect.w;
				images[1].height =         tmp_src_plane_rect.h;
				images[1].p_next =             &(images[2]);

				images[2].img_id =         GRPH_IMG_ID_C;       // destination image
				images[2].dram_addr = (UINT32)p_dst_img->addr[0] + gximg_calc_plane_offset(p_dst_img, 0, &tmp_dst_plane_rect);
				images[2].lineoffset =     p_dst_img->loff[0];
				images[2].width =         tmp_dst_plane_rect.w;
				images[2].height =         tmp_dst_plane_rect.h;
				property[0].id =             GRPH_PROPERTY_ID_NORMAL;
				property[0].property =     0;
				property[0].p_next = NULL;

				if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
					gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_B) ||
					gximg_check_grph_limit(request.command, request.format, &(images[2]), 1, GRPH_IMG_ID_C)){
					DBG_ERR("limit check fail\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
				if(kdrv_grph_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0), &request, NULL, NULL) != E_OK){
					DBG_ERR("fail to start graphic engine(%d)\n", KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0));
					gximg_close_grph(grph_id);
					return E_PAR;
				}

				gximg_calc_plane_rect(p_src_img, 1, &tmp_src_rect, &tmp_src_plane_rect);
				gximg_calc_plane_rect(p_dst_img, 1, &tmp_dst_rect, &tmp_dst_plane_rect);

				request.format = GRPH_FORMAT_16BITS_UVPACK;
				images[0].img_id =             GRPH_IMG_ID_A;       // source image
				images[0].dram_addr = (UINT32)p_src_img->addr[1] + gximg_calc_plane_offset(p_src_img, 1, &tmp_src_plane_rect);
				images[0].lineoffset =     p_src_img->loff[1];
				images[0].width =         tmp_src_plane_rect.w;
				images[0].height =         tmp_src_plane_rect.h;
				images[0].p_next =             &(images[1]);

				// alpha plane begin
				images[1].img_id =         GRPH_IMG_ID_B;
				//address the same with Y
				//images[1].uiAddress =
				images[1].width =         tmp_src_plane_rect.w;
				images[1].height =        tmp_src_plane_rect.h;

				if (p_dst_img->pxlfmt == VDO_PXLFMT_YUV420) {
					images[1].lineoffset =     p_src_img->loff[1] << 1;
				} else {
					images[1].lineoffset =     p_src_img->loff[1];
				}
				images[1].p_next =             &(images[2]);
				// alpha plane end
				images[2].img_id =         GRPH_IMG_ID_C;       // destination image
				images[2].dram_addr = (UINT32)p_dst_img->addr[1] + gximg_calc_plane_offset(p_dst_img, 1, &tmp_dst_plane_rect);
				images[2].lineoffset =     p_dst_img->loff[1];
				images[2].width =         tmp_dst_plane_rect.w;
				images[2].height =         tmp_dst_plane_rect.h;
				property[0].id =             GRPH_PROPERTY_ID_NORMAL;
				property[0].property =     0;
				property[0].p_next = NULL;

				if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
					gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_B) ||
					gximg_check_grph_limit(request.command, request.format, &(images[2]), 1, GRPH_IMG_ID_C)){
					DBG_ERR("limit check fail\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
				if(kdrv_grph_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0), &request, NULL, NULL) != E_OK){
					DBG_ERR("fail to start graphic engine(%d)\n", KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0));
					gximg_close_grph(grph_id);
					return E_PAR;
				}

			}
		} else if (p_dst_img->pxlfmt == VDO_PXLFMT_ARGB8888) {
				KDRV_GRPH_TRIGGER_PARAM request = {0};
				GRPH_IMG images[3];
				GRPH_PROPERTY property[1];

				gximg_memset(images, 0, sizeof(images));
				gximg_memset(property, 0, sizeof(property));

				gximg_calc_plane_rect(p_src_img, 0, &tmp_src_rect, &tmp_src_plane_rect);
				gximg_calc_plane_rect(p_dst_img, 0, &tmp_dst_rect, &tmp_dst_plane_rect);

				request.command = GRPH_CMD_PLANE_BLENDING;
				request.format = GRPH_FORMAT_32BITS_ARGB8888_RGB;
				request.is_skip_cache_flush = check_no_flush(usePA, no_flush);
				request.is_buf_pa = usePA;
				request.p_images = &(images[0]);
				request.p_property = &(property[0]);
				images[0].img_id =             GRPH_IMG_ID_A;       // source image
				images[0].dram_addr = (UINT32)p_src_img->addr[0] + gximg_calc_plane_offset(p_src_img, 0, &tmp_src_plane_rect);
				images[0].lineoffset =     p_src_img->loff[0];
				images[0].width =         tmp_src_plane_rect.w;
				images[0].height =         tmp_src_plane_rect.h;
				images[0].p_next =             &(images[1]);

				images[1].img_id =         GRPH_IMG_ID_B;       // destination image
				images[1].dram_addr = (UINT32)p_dst_img->addr[0] + gximg_calc_plane_offset(p_dst_img, 0, &tmp_dst_plane_rect);
				images[1].lineoffset =     p_dst_img->loff[0];
				images[1].width =         tmp_dst_plane_rect.w;
				images[1].height =         tmp_dst_plane_rect.h;
				images[1].p_next =             &(images[2]);

				images[2].img_id =         GRPH_IMG_ID_C;       // destination image
				images[2].dram_addr = (UINT32)p_dst_img->addr[0] + gximg_calc_plane_offset(p_dst_img, 0, &tmp_dst_plane_rect);
				images[2].lineoffset =     p_dst_img->loff[0];
				images[2].width =         tmp_dst_plane_rect.w;
				images[2].height =         tmp_dst_plane_rect.h;
				property[0].id =             GRPH_PROPERTY_ID_R;
				property[0].property =     GRPH_ENABLE_ALPHA_FROM_A;
				property[0].p_next = NULL;

				if (gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
					gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_B) ||
					gximg_check_grph_limit(request.command, request.format, &(images[2]), 1, GRPH_IMG_ID_C)) {
					DBG_ERR("limit check fail\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
				if (kdrv_grph_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0), &request, NULL, NULL) != E_OK) {
					DBG_ERR("fail to start graphic engine(%d)\n", KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0));
					gximg_close_grph(grph_id);
					return E_PAR;
				}
			}
		if(gximg_close_grph(grph_id)){
			DBG_ERR("fail to close graphic engine(%d)\n", KDRV_GFX2D_GRPH0);
			return E_PAR;
		}
#endif
	} else {
		DBG_ERR("valid_w=%d valid_h=%d\r\n", tmp_dst_rect.w, tmp_dst_rect.h);
		return E_PAR;
	}
	DBG_FUNC_END("[copy]\r\n");
	return E_OK;
}

ER gximg_copy_blend_data_ex(PVDO_FRAME p_src_img, IRECT    *p_src_region, PVDO_FRAME p_dst_img, IPOINT    *p_dst_location, UINT8 *p_alpha_plane, int usePA)
{
	return _gximg_copy_blend_data_ex_with_flush(p_src_img, p_src_region, p_dst_img, p_dst_location, p_alpha_plane, 0, usePA);
}

ER gximg_copy_blend_data_ex_no_flush(PVDO_FRAME p_src_img, IRECT    *p_src_region, PVDO_FRAME p_dst_img, IPOINT    *p_dst_location, UINT8 *p_alpha_plane, int usePA)
{
	return _gximg_copy_blend_data_ex_with_flush(p_src_img, p_src_region, p_dst_img, p_dst_location, p_alpha_plane, 1, usePA);
}

/*
static void GxImg_ResetParm(void)
{
    gScaleMethod = GXIMG_SCALE_AUTO;
}
*/

ER gximg_set_parm(GXIMG_PARM_ID parm_id, UINT32 value)
{
	switch (parm_id) {
	case GXIMG_PARM_SCALE_METHOD:
		if (value >= GXIMG_SCALE_MAX_ID) {
			DBG_ERR("ParmID=%d,value=%d \r\n", parm_id, value);
			return E_PAR;
		}
		g_scale_method = value;
		break;
	default:
		DBG_ERR("parm_id=%d \r\n", parm_id);
		return E_PAR;
	}
	return E_OK;
}

UINT32 gximg_get_parm(GXIMG_PARM_ID parm_id)
{
	switch (parm_id) {
	case GXIMG_PARM_SCALE_METHOD:
		return g_scale_method;
	default:
		DBG_ERR("parm_id=%d \r\n", parm_id);
		return 0;
	}
	return 0;
}

#if 0 //USE_IME
static ER gximg_scale_by_ime(PVDO_FRAME p_src_img, IRECT *p_src_rect, PVDO_FRAME p_dst_img, IRECT *p_dst_rect)
{
	IME_OPENOBJ ime_obj = {0};
	IME_IQ_FLOW_INFO ime_info = {0};
	IRECT         tmp_src_plane_rect = {0}, tmp_dst_plane_rect = {0};
	UINT32   p_in_offset[GXIMG_MAX_PLANE_NUM] = {0}, p_out_offset[GXIMG_MAX_PLANE_NUM] = {0}, plane_num, i;
	UINT32   max_scale_up_factor, max_scale_down_factor, in_width_align, addr_align, loff_align, out_width_align, in_height_align;
	UINT32   scale_method;

	gximg_memset(&ime_arg, 0, sizeof(IME_MODE_PRAM));
	ime_arg.OperationMode = IME_OPMODE_D2D;

	DBG_FUNC_BEGIN("[scale]\r\n");
	//For better playback quality, should choose linear or cubic
	DBG_IND("In w =%d, h=%d, out w=%d, h=%d\r\n", p_src_rect->w, p_src_rect->h, p_dst_rect->w, p_dst_rect->h);

	// choose scale method
	if (g_scale_method == GXIMG_SCALE_AUTO) {
		if ((p_src_rect->w < p_dst_rect->w * 11) && (p_src_rect->h < p_dst_rect->h * 11) && (p_src_rect->w > p_dst_rect->w * 2) && (p_src_rect->h > p_dst_rect->h * 2)) {
			scale_method = IMEALG_INTEGRATION;
			DBG_MSG("[scale]INTEGRATION\r\n");
		} else {
			scale_method = IMEALG_BICUBIC;
			DBG_MSG("[scale]BICUBIC\r\n");
		}
	} else {
		scale_method = gximg_map_to_ime_scale_method(g_scale_method);
	}
	DBG_MSG("[scale] scale_method = %d\r\n", scale_method);

	if (p_src_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR || p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR) {
		plane_num = 3;
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_YUV422 || p_src_img->pxlfmt == VDO_PXLFMT_YUV420) {
		plane_num = 2;
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_Y8) {
		plane_num = 1;
	} else {
		DBG_ERR("src pixel format %x\r\n", p_src_img->pxlfmt);
		return E_PAR;
	}

	for (i = 0; i < plane_num; i++) {
		gximg_calc_plane_rect(p_src_img, i, p_src_rect, &tmp_src_plane_rect);
		p_in_offset[i] = gximg_calc_plane_offset(p_src_img, i, &tmp_src_plane_rect);
	}

	if (p_dst_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR || p_dst_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR) {
		plane_num = 3;
	} else if (p_dst_img->pxlfmt == VDO_PXLFMT_YUV422 || p_dst_img->pxlfmt == VDO_PXLFMT_YUV420) {
		plane_num = 2;
	} else if (p_dst_img->pxlfmt == VDO_PXLFMT_Y8) {
		plane_num = 1;
	} else {
		DBG_ERR("dst pixel format %x\r\n", p_dst_img->pxlfmt);
		return E_PAR;
	}

	for (i = 0; i < plane_num; i++) {
		gximg_calc_plane_rect(p_dst_img, i, p_dst_rect, &tmp_dst_plane_rect);
		p_out_offset[i] = gximg_calc_plane_offset(p_dst_img, i, &tmp_dst_plane_rect);
	}

	// check alignment begin
	max_scale_up_factor = 32;
	max_scale_down_factor = 16;
	in_width_align = 4;
	out_width_align = 4;
	in_height_align = 2;
	addr_align = 4;
	loff_align = 4;

	if (p_src_rect->w >= (max_scale_down_factor * p_dst_rect->w) || p_src_rect->h >= (max_scale_down_factor * p_dst_rect->h)) {
		DBG_ERR("scale down factor over %d, SrcW=%d,SrcH=%d,DstW=%d,DstH=%d\r\n", max_scale_down_factor, p_src_rect->w, p_src_rect->h, p_dst_rect->w, p_dst_rect->h);
		return E_PAR;
	} else if (p_dst_rect->w >= (max_scale_up_factor * p_src_rect->w) || p_dst_rect->h >= (max_scale_up_factor * p_src_rect->h)) {
		DBG_ERR("scale up factor over %d, SrcW=%d,SrcH=%d,DstW=%d,DstH=%d\r\n", max_scale_up_factor, p_src_rect->w, p_src_rect->h, p_dst_rect->w, p_dst_rect->h);
		return E_PAR;
	} else if ((p_src_rect->w) % in_width_align) {
		DBG_ERR("In W(%d) not %d byte aligned\r\n", p_src_rect->w, in_width_align);
		return E_PAR;
	} else if ((p_dst_rect->w) % out_width_align) {
		DBG_ERR("Out W(%d) not %d byte aligned\r\n", p_dst_rect->w, out_width_align);
		return E_PAR;
	} else if ((p_src_rect->h) % in_height_align) {
		DBG_ERR("In H(%d) not %d byte aligned\r\n", p_src_rect->h, in_height_align);
		return E_PAR;
	} else if (((p_src_img->addr[0] + p_in_offset[0]) % addr_align)) {
		DBG_ERR("In Addr[0](0x%x) not %d byte aligned\r\n", p_src_img->addr[0] + p_in_offset[0], addr_align);
		return E_PAR;
	} else if (((p_src_img->addr[1] + p_in_offset[1]) % addr_align)) {
		DBG_ERR("In Addr[1](0x%x) not %d byte aligned\r\n", p_src_img->addr[1] + p_in_offset[1], addr_align);
		return E_PAR;
	} else if (((p_dst_img->addr[0] + p_out_offset[0]) % addr_align)) {
		DBG_ERR("Out Addr[0](0x%x) not %d byte aligned\r\n", p_dst_img->addr[0] + p_out_offset[0], addr_align);
		return E_PAR;
	} else if (((p_dst_img->addr[1] + p_out_offset[1]) % addr_align)) {
		DBG_ERR("Out Addr[1](0x%x) not %d byte aligned\r\n", p_dst_img->addr[1] + p_out_offset[1], addr_align);
		return E_PAR;
	} else if ((p_src_img->loff[0] % loff_align)) {
		DBG_ERR("In lineOff[0](0x%x) not %d byte aligned\r\n", p_src_img->loff[0], loff_align);
		return E_PAR;
	} else if ((p_src_img->loff[1] % loff_align)) {
		DBG_ERR("In lineOff[1](0x%x) not %d byte aligned\r\n", p_src_img->loff[1], loff_align);
		return E_PAR;
	} else if ((p_dst_img->loff[0] % loff_align)) {
		DBG_ERR("Out lineOff[0](0x%x) not %d byte aligned\r\n", p_dst_img->loff[0], loff_align);
		return E_PAR;
	} else if ((p_dst_img->loff[1] % loff_align)) {
		DBG_ERR("Out lineOff[1](0x%x) not %d byte aligned\r\n", p_dst_img->loff[1], loff_align);
		return E_PAR;
	}
	// check alignment end

	//ime_arg.InPathInfo.InAddr.InBufEnable          = IME_IPPB_DISABLE;
	ime_arg.InPathInfo.InAddr.uiAddrY        = p_src_img->addr[0] + p_in_offset[0];
	ime_arg.InPathInfo.InAddr.uiAddrCb       = p_src_img->addr[1] + p_in_offset[1];
	ime_arg.InPathInfo.InAddr.uiAddrCr       = p_src_img->addr[2] + p_in_offset[2];
	ime_arg.InPathInfo.InSize.uiSizeH        = p_src_rect->w;
	ime_arg.InPathInfo.InSize.uiSizeV        = p_src_rect->h;
	ime_arg.InPathInfo.InLineoffset.uiLineOffsetY = p_src_img->loff[0];
	ime_arg.InPathInfo.InLineoffset.uiLineOffsetCb = p_src_img->loff[1];

	// If destination image is Y only , then force the source image to Y only
	if (p_dst_img->pxlfmt == VDO_PXLFMT_Y8) {
		ime_arg.InPathInfo.InFormat      = gximg_map_to_ime_in_fmt(VDO_PXLFMT_Y8);
	} else {
		ime_arg.InPathInfo.InFormat      = gximg_map_to_ime_in_fmt(p_src_img->pxlfmt);
	}
	DBG_MSG("[scale]In Addr[0]=0x%07x, Addr[1]=0x%07x, Addr[2]=0x%07x\r\n", ime_arg.InPathInfo.InAddr.uiAddrY, ime_arg.InPathInfo.InAddr.uiAddrCb, ime_arg.InPathInfo.InAddr.uiAddrCr);

	ime_arg.OutPath3.OutPathAddr.uiAddrY   = p_dst_img->addr[0] + p_out_offset[0];
	ime_arg.OutPath3.OutPathAddr.uiAddrCb  = p_dst_img->addr[1] + p_out_offset[1];
	ime_arg.OutPath3.OutPathAddr.uiAddrCr  = p_dst_img->addr[2] + p_out_offset[2];
	ime_arg.OutPath3.OutPathOutSize.uiSizeH    = p_dst_rect->w;
	ime_arg.OutPath3.OutPathOutSize.uiSizeV    = p_dst_rect->h;
	ime_arg.OutPath3.OutPathScaleSize.uiSizeH = p_dst_rect->w;
	ime_arg.OutPath3.OutPathScaleSize.uiSizeV = p_dst_rect->h;

	ime_arg.OutPath3.OutPathOutLineoffset.uiLineOffsetY = p_dst_img->loff[0];
	ime_arg.OutPath3.OutPathOutLineoffset.uiLineOffsetCb = p_dst_img->loff[1];
	ime_arg.OutPath3.OutPathImageFormat.OutFormatSel = gximg_map_to_ime_out_fmt(p_dst_img->pxlfmt);
	ime_arg.OutPath3.OutPathImageFormat.OutFormatTypeSel = gximg_map_to_ime_out_fmt_type(p_dst_img->pxlfmt);
	DBG_MSG("[scale]Out Addr[0]=0x%07x, Addr[1]=0x%07x, Addr[2]=0x%07x\r\n", ime_arg.OutPath3.OutPathAddr.uiAddrY, ime_arg.OutPath3.OutPathAddr.uiAddrCb, ime_arg.OutPath3.OutPathAddr.uiAddrCr);
	ime_arg.OutPath3.OutPathCropPos.uiPosX = 0;
	ime_arg.OutPath3.OutPathCropPos.uiPosY = 0;
	ime_arg.OutPath3.OutPathScaleFilter.CoefCalMode = IME_SCALE_FILTER_COEF_AUTO_MODE;

	ime_arg.OutPath3.OutPathScaleMethod = scale_method;
	//ime_arg.OutPath3. P3OutAddr.OutBufNums = IME_OUTPUT_PPB_1;
	ime_arg.OutPath3.OutPathEnable = IME_FUNC_ENABLE;
	ime_arg.OutPath3.OutPathDramEnable = IME_FUNC_ENABLE ;
	ime_arg.uiInterruptEnable = IME_INTE_ALL;

	DBG_MSG("[scale] gIMEP2Ien = %d\r\n", g_ime_p2i_en);
	if (g_ime_p2i_en) {
		IME_FUNC_EN  im2_p2i_en = IME_FUNC_ENABLE;
		ime_arg.pImeIQInfo = &ime_info;
		ime_info.pP2IInfo = &im2_p2i_en;
	} else {
		ime_arg.pImeIQInfo = NULL;
	}
	ime_obj.uiImeClockSel = 480;
	ime_obj.FP_IMEISR_CB = NULL;

	ime_open(&ime_obj);
	if (ime_setMode(&ime_arg) != E_OK) {
		DBG_ERR("ime_setMode error ..\r\n");
		ime_close();
		return E_PAR;
	}

	//ime_clrIntFlag();
	ime_setStart();
	ime_waitFlagFrameEnd(IME_FLAG_NO_CLEAR);
	ime_pause();
	ime_close();
	DBG_FUNC_END("[scale]\r\n");
	return E_OK;
}
#endif

#if USE_ISE
static ER gximg_ise_open(GXIMG_SC_ENG engine)
{
	switch (engine) {
	case GXIMG_SC_ENG1:
		return kdrv_ise_open(KDRV_CHIP0, KDRV_GFX2D_ISE0);
	case GXIMG_SC_ENG2:
		return kdrv_ise_open(KDRV_CHIP0, KDRV_GFX2D_ISE1);
	// coverity[dead_error_begin]
	default:
		DBG_ERR("Invalid scale eng %d\r\n", engine);
		return E_PAR;
	}
}

static ER gximg_ise_close(GXIMG_SC_ENG engine)
{
	switch (engine) {
	case GXIMG_SC_ENG1:
		return kdrv_ise_close(KDRV_CHIP0, KDRV_GFX2D_ISE0);
	case GXIMG_SC_ENG2:
		return kdrv_ise_close(KDRV_CHIP0, KDRV_GFX2D_ISE1);
	default:
		DBG_ERR("Invalid scale eng %d\r\n", engine);
		return E_PAR;
	}
}

static ER gximg_ise_start(GXIMG_SC_ENG engine, KDRV_ISE_TRIGGER_PARAM *p_param)
{
	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	switch (engine) {
	case GXIMG_SC_ENG1:
		return kdrv_ise_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_ISE0, 0), p_param, NULL, NULL);
	case GXIMG_SC_ENG2:
		return kdrv_ise_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_ISE1, 0), p_param, NULL, NULL);
	default:
		DBG_ERR("Invalid scale eng %d\r\n", engine);
		return E_PAR;
	}
}

static ER gximg_ise_set_mode(GXIMG_SC_ENG engine, KDRV_ISE_MODE *p_info)
{
	if(!p_info){
		DBG_ERR("p_info is NULL\n");
		return E_PAR;
	}

	switch (engine) {
	case GXIMG_SC_ENG1:
		return kdrv_ise_set(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_ISE0, 0), KDRV_ISE_PARAM_MODE, p_info);
	case GXIMG_SC_ENG2:
		return kdrv_ise_set(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_ISE1, 0), KDRV_ISE_PARAM_MODE, p_info);
	default:
		DBG_ERR("Invalid scale eng %d\r\n", engine);
		return E_PAR;
	}
}

int gximg_check_ise_limit(KDRV_ISE_MODE *ise_op){

	static unsigned int limit_1555_src[]={
		ISE_SRCBUF_RGB1555_WMIN, ISE_SRCBUF_RGB1555_WMAX, ISE_SRCBUF_RGB1555_WALIGN,
		ISE_SRCBUF_RGB1555_HMIN, ISE_SRCBUF_RGB1555_HMAX, ISE_SRCBUF_RGB1555_HALIGN,
		ISE_SRCBUF_RGB1555_LOFF_ALIGN, ISE_SRCBUF_RGB1555_ADDR_ALIGN};
	static unsigned int limit_1555_dst[]={
		ISE_DSTBUF_RGB1555_WMIN, ISE_DSTBUF_RGB1555_WMAX, ISE_DSTBUF_RGB1555_WALIGN,
		ISE_DSTBUF_RGB1555_HMIN, ISE_DSTBUF_RGB1555_HMAX, ISE_DSTBUF_RGB1555_HALIGN,
		ISE_DSTBUF_RGB1555_LOFF_ALIGN, ISE_DSTBUF_RGB1555_ADDR_ALIGN};
	static unsigned int limit_4444_src[]={
		ISE_SRCBUF_RGB4444_WMIN, ISE_SRCBUF_RGB4444_WMAX, ISE_SRCBUF_RGB4444_WALIGN,
		ISE_SRCBUF_RGB4444_HMIN, ISE_SRCBUF_RGB4444_HMAX, ISE_SRCBUF_RGB4444_HALIGN,
		ISE_SRCBUF_RGB4444_LOFF_ALIGN, ISE_SRCBUF_RGB4444_ADDR_ALIGN};
	static unsigned int limit_4444_dst[]={
		ISE_DSTBUF_RGB4444_WMIN, ISE_DSTBUF_RGB4444_WMAX, ISE_DSTBUF_RGB4444_WALIGN,
		ISE_DSTBUF_RGB4444_HMIN, ISE_DSTBUF_RGB4444_HMAX, ISE_DSTBUF_RGB4444_HALIGN,
		ISE_DSTBUF_RGB4444_LOFF_ALIGN, ISE_DSTBUF_RGB4444_ADDR_ALIGN};
	static unsigned int limit_8888_src[]={
		ISE_SRCBUF_RGB8888_WMIN, ISE_SRCBUF_RGB8888_WMAX, ISE_SRCBUF_RGB8888_WALIGN,
		ISE_SRCBUF_RGB8888_HMIN, ISE_SRCBUF_RGB8888_HMAX, ISE_SRCBUF_RGB8888_HALIGN,
		ISE_SRCBUF_RGB8888_LOFF_ALIGN, ISE_SRCBUF_RGB8888_ADDR_ALIGN};
	static unsigned int limit_8888_dst[]={
		ISE_DSTBUF_RGB8888_WMIN, ISE_DSTBUF_RGB8888_WMAX, ISE_DSTBUF_RGB8888_WALIGN,
		ISE_DSTBUF_RGB8888_HMIN, ISE_DSTBUF_RGB8888_HMAX, ISE_DSTBUF_RGB8888_HALIGN,
		ISE_DSTBUF_RGB8888_LOFF_ALIGN, ISE_DSTBUF_RGB8888_ADDR_ALIGN};
	static unsigned int limit_565_src[]={
		ISE_SRCBUF_RGB565_WMIN, ISE_SRCBUF_RGB565_WMAX, ISE_SRCBUF_RGB565_WALIGN,
		ISE_SRCBUF_RGB565_HMIN, ISE_SRCBUF_RGB565_HMAX, ISE_SRCBUF_RGB565_HALIGN,
		ISE_SRCBUF_RGB565_LOFF_ALIGN, ISE_SRCBUF_RGB565_ADDR_ALIGN};
	static unsigned int limit_565_dst[]={
		ISE_DSTBUF_RGB565_WMIN, ISE_DSTBUF_RGB565_WMAX, ISE_DSTBUF_RGB565_WALIGN,
		ISE_DSTBUF_RGB565_HMIN, ISE_DSTBUF_RGB565_HMAX, ISE_DSTBUF_RGB565_HALIGN,
		ISE_DSTBUF_RGB565_LOFF_ALIGN, ISE_DSTBUF_RGB565_ADDR_ALIGN};
	static unsigned int limit_Y8_src[]={
		ISE_SRCBUF_Y8BIT_WMIN, ISE_SRCBUF_Y8BIT_WMAX, ISE_SRCBUF_Y8BIT_WALIGN,
		ISE_SRCBUF_Y8BIT_HMIN, ISE_SRCBUF_Y8BIT_HMAX, ISE_SRCBUF_Y8BIT_HALIGN,
		ISE_SRCBUF_Y8BIT_LOFF_ALIGN, ISE_SRCBUF_Y8BIT_ADDR_ALIGN};
	static unsigned int limit_Y8_dst[]={
		ISE_DSTBUF_Y8BIT_WMIN, ISE_DSTBUF_Y8BIT_WMAX, ISE_DSTBUF_Y8BIT_WALIGN,
		ISE_DSTBUF_Y8BIT_HMIN, ISE_DSTBUF_Y8BIT_HMAX, ISE_DSTBUF_Y8BIT_HALIGN,
		ISE_DSTBUF_Y8BIT_LOFF_ALIGN, ISE_DSTBUF_Y8BIT_ADDR_ALIGN};
	static unsigned int limit_UVP_src[]={
		ISE_SRCBUF_UVP_WMIN, ISE_SRCBUF_UVP_WMAX, ISE_SRCBUF_UVP_WALIGN,
		ISE_SRCBUF_UVP_HMIN, ISE_SRCBUF_UVP_HMAX, ISE_SRCBUF_UVP_HALIGN,
		ISE_SRCBUF_UVP_LOFF_ALIGN, ISE_SRCBUF_UVP_ADDR_ALIGN};
	static unsigned int limit_UVP_dst[]={
		ISE_DSTBUF_UVP_WMIN, ISE_DSTBUF_UVP_WMAX, ISE_DSTBUF_UVP_WALIGN,
		ISE_DSTBUF_UVP_HMIN, ISE_DSTBUF_UVP_HMAX, ISE_DSTBUF_UVP_HALIGN,
		ISE_DSTBUF_UVP_LOFF_ALIGN, ISE_DSTBUF_UVP_ADDR_ALIGN};

	unsigned int *limit_src = NULL;
	unsigned int *limit_dst = NULL;

	if(!ise_op){
		return -1;
	}

	if(ise_op->io_pack_fmt == KDRV_ISE_ARGB1555){
		limit_src = limit_1555_src;
		limit_dst = limit_1555_dst;
	}else if(ise_op->io_pack_fmt == KDRV_ISE_ARGB4444){
		limit_src = limit_4444_src;
		limit_dst = limit_4444_dst;
	}else if(ise_op->io_pack_fmt == KDRV_ISE_ARGB8888){
		limit_src = limit_8888_src;
		limit_dst = limit_8888_dst;
	}else if(ise_op->io_pack_fmt == KDRV_ISE_RGB565){
		limit_src = limit_565_src;
		limit_dst = limit_565_dst;
	}else if(ise_op->io_pack_fmt == KDRV_ISE_Y8){
		limit_src = limit_Y8_src;
		limit_dst = limit_Y8_dst;
	}else if(ise_op->io_pack_fmt == KDRV_ISE_UVP){
		limit_src = limit_UVP_src;
		limit_dst = limit_UVP_dst;
	}else{
		DBG_ERR("unsupported ise format %x\n", ise_op->io_pack_fmt);
		return -1;
	}

	if(ise_op->in_width < limit_src[0] || ise_op->in_width > limit_src[1]){
		DBG_ERR("invalid with(%d), min is %d, max is %d\n", ise_op->in_width, limit_src[0], limit_src[1]);
		return -1;
	}
	if(limit_src[2] && (ise_op->in_width & (limit_src[2] - 1))){
		DBG_ERR("src width(%d) must be %d aligned\n", ise_op->in_width, limit_src[2]);
		return -1;
	}

	if(ise_op->in_height < limit_src[3] || ise_op->in_height > limit_src[4]){
		DBG_ERR("invalid height(%d), min is %d, max is %d\n", ise_op->in_height, limit_src[3], limit_src[4]);
		return -1;
	}
	if(limit_src[5] && (ise_op->in_height & (limit_src[5] - 1))){
		DBG_ERR("src height(%d) must be %d aligned\n", ise_op->in_height, limit_src[5]);
		return -1;
	}

	if(ise_op->in_lofs < limit_src[0] || ise_op->in_lofs > limit_src[1]){
		DBG_ERR("invalid in_lofs(%d), min is %d, max is %d\n", ise_op->in_lofs, limit_src[0], limit_src[1]);
		return -1;
	}
	if(limit_src[6] && (ise_op->in_lofs & (limit_src[6] - 1))){
		DBG_ERR("src in_lofs(%d) must be %d aligned\n", ise_op->in_lofs, limit_src[6]);
		return -1;
	}

	if(limit_src[7] && (ise_op->in_addr & (limit_src[7] - 1))){
		DBG_ERR("src in_addr(%p) must be %d aligned\n", (unsigned char*)ise_op->in_addr, limit_src[7]);
		return -1;
	}

	if(ise_op->out_width < limit_dst[0] || ise_op->out_width > limit_dst[1]){
		DBG_ERR("invalid out_width(%d), min is %d, max is %d\n", ise_op->out_width, limit_dst[0], limit_dst[1]);
		return -1;
	}
	if(limit_dst[2] && (ise_op->out_width & (limit_dst[2] - 1))){
		DBG_ERR("src out_width(%d) must be %d aligned\n", ise_op->out_width, limit_dst[2]);
		return -1;
	}

	if(ise_op->out_height < limit_dst[3] || ise_op->out_height > limit_dst[4]){
		DBG_ERR("invalid out_height(%d), min is %d, max is %d\n", ise_op->out_height, limit_dst[3], limit_dst[4]);
		return -1;
	}
	if(limit_dst[5] && (ise_op->out_height & (limit_dst[5] - 1))){
		DBG_ERR("src out_height(%d) must be %d aligned\n", ise_op->out_height, limit_dst[5]);
		return -1;
	}

	if(ise_op->out_lofs < limit_dst[0] || ise_op->out_lofs > limit_dst[1]){
		DBG_ERR("invalid out_lofs(%d), min is %d, max is %d\n", ise_op->out_lofs, limit_dst[0], limit_dst[1]);
		return -1;
	}
	if(limit_dst[6] && (ise_op->out_lofs & (limit_dst[6] - 1))){
		DBG_ERR("src out_lofs(%d) must be %d aligned\n", ise_op->out_lofs, limit_dst[6]);
		return -1;
	}

	if(limit_dst[7] && (ise_op->out_addr & (limit_dst[7] - 1))){
		DBG_ERR("src out_addr(%p) must be %d aligned\n", (unsigned char*)ise_op->out_addr, limit_dst[7]);
		return -1;
	}

	return 0;
}

static ER gximg_scale_by_ise(PVDO_FRAME p_src_img, IRECT *p_src_rect, PVDO_FRAME p_dst_img, IRECT *p_dst_rect, GXIMG_SC_ENG engine, int no_flush, GXIMG_SCALE_METHOD method, int usePA)
{
	IRECT         tmp_src_plane_rect = {0}, tmp_dst_plane_rect = {0};
	UINT32        in_loff[GXIMG_MAX_PLANE_NUM] = {0}, out_loff[GXIMG_MAX_PLANE_NUM] = {0}, plane_num, dst_plane_num, i, scale_method, max_scale_factor, loff, uv_addr_align;

	DBG_FUNC_BEGIN("[scale]\r\n");
	DBG_MSG("[scale]In w =%d, h=%d, out w=%d, h=%d\r\n", p_src_rect->w, p_src_rect->h, p_dst_rect->w, p_dst_rect->h);

	if (VDO_PXLFMT_ARGB8565 == p_src_img->pxlfmt ||
		VDO_PXLFMT_ARGB1555 == p_src_img->pxlfmt ||
		VDO_PXLFMT_ARGB4444 == p_src_img->pxlfmt ||
		VDO_PXLFMT_ARGB8888 == p_src_img->pxlfmt ||
		VDO_PXLFMT_RGB565 == p_src_img->pxlfmt) {
		if (p_src_img->pxlfmt != p_dst_img->pxlfmt) {
			DBG_ERR("Src fmt %d != Dst fmt %d\r\n", p_src_img->pxlfmt, p_dst_img->pxlfmt);
			return E_PAR;
		}
	}

	switch (p_src_img->pxlfmt) {
	case VDO_PXLFMT_YUV420_PLANAR:
		plane_num = 3;
		break;
	case VDO_PXLFMT_YUV422:
	case VDO_PXLFMT_YUV420:
	case VDO_PXLFMT_ARGB8565:
		plane_num = 2;
		break;
	case VDO_PXLFMT_Y8:
	case VDO_PXLFMT_I8:
	case VDO_PXLFMT_ARGB1555:
	case VDO_PXLFMT_ARGB4444:
	case VDO_PXLFMT_ARGB8888:
	case VDO_PXLFMT_RGB565:
		plane_num = 1;
		break;
	default:
		DBG_ERR("src pixel format %x\r\n", p_src_img->pxlfmt);
		return E_PAR;
	}

	for (i = 0; i < plane_num; i++) {
		gximg_calc_plane_rect(p_src_img, i, p_src_rect, &tmp_src_plane_rect);
		in_loff[i] = gximg_calc_plane_offset(p_src_img, i, &tmp_src_plane_rect);
	}

	switch (p_dst_img->pxlfmt) {
	case VDO_PXLFMT_YUV420_PLANAR:
		dst_plane_num = 3;
		break;
	case VDO_PXLFMT_YUV422:
	case VDO_PXLFMT_YUV420:
	case VDO_PXLFMT_ARGB8565:
		dst_plane_num = 2;
		break;
	case VDO_PXLFMT_Y8:
	case VDO_PXLFMT_I8:
	case VDO_PXLFMT_ARGB1555:
	case VDO_PXLFMT_ARGB4444:
	case VDO_PXLFMT_ARGB8888:
	case VDO_PXLFMT_RGB565:
		dst_plane_num = 1;
		break;
	default:
		DBG_ERR("dst pixel format %x\r\n", p_dst_img->pxlfmt);
		return E_PAR;
	}
	plane_num = MIN(plane_num, dst_plane_num);

	for (i = 0; i < plane_num; i++) {
		gximg_calc_plane_rect(p_dst_img, i, p_dst_rect, &tmp_dst_plane_rect);
		out_loff[i] = gximg_calc_plane_offset(p_dst_img, i, &tmp_dst_plane_rect);
	}
	// check ISE alignment begin
	max_scale_factor = 16;
	uv_addr_align = 2;
	loff = 4;

	if (p_src_rect->w >= ((INT32)max_scale_factor * p_dst_rect->w) || p_src_rect->h >= ((INT32)max_scale_factor * p_dst_rect->h) || p_dst_rect->w >= ((INT32)max_scale_factor *p_src_rect->w) || p_dst_rect->h >= ((INT32)max_scale_factor * p_src_rect->h)) {
		DBG_ERR("scale factor over %d, SrcW=%d,SrcH=%d,DstW=%d,DstH=%d\r\n", max_scale_factor, p_src_rect->w, p_src_rect->h, p_dst_rect->w, p_dst_rect->h);
		return E_PAR;
	} else if ((p_src_img->addr[1] + in_loff[1]) % uv_addr_align) {
		DBG_WRN("In UV Addr not %d byte aligned, auto truncate\r\n", uv_addr_align);
		in_loff[1] &= (~(uv_addr_align - 1));
	} else if ((p_dst_img->addr[1] + out_loff[1]) % uv_addr_align) {
		DBG_WRN("Out UV Addr not %d byte aligned, auto truncate\r\n", uv_addr_align);
		out_loff[1] &= (~(uv_addr_align - 1));
	} else if ((p_src_img->loff[0] % loff) || (p_src_img->loff[1] % loff)) {
		DBG_ERR("In lineoff not %d byte aligned\r\n", loff);
		return E_PAR;
	} else if ((p_dst_img->loff[0] % loff) || (p_dst_img->loff[1] % loff)) {
		DBG_ERR("Out lineoff not %d byte aligned\r\n", loff);
		return E_PAR;
	}
	// check ISE alignment end


	// choose scale method
	if (method == GXIMG_SCALE_AUTO){
		if (p_src_img->pxlfmt == VDO_PXLFMT_RGB565 ||
		    p_src_img->pxlfmt == VDO_PXLFMT_ARGB1555 ||
		    p_src_img->pxlfmt == VDO_PXLFMT_ARGB4444 ||
		    p_src_img->pxlfmt == VDO_PXLFMT_ARGB8888)
			scale_method = KDRV_ISE_BILINEAR;
		else if (p_src_rect->w >= p_dst_rect->w && p_src_rect->h >= p_dst_rect->h)
			scale_method = KDRV_ISE_INTEGRATION;
		else
			scale_method = KDRV_ISE_BILINEAR;
	}else
		scale_method = gximg_map_to_ise_scale_method(method);

	DBG_MSG("[scale] scale_method = %d\r\n", scale_method);

	{
		KDRV_ISE_MODE             ise_op = {0};
		KDRV_ISE_TRIGGER_PARAM    ise_trigger = {1, 0};

		if(gximg_ise_open(engine) != E_OK){
			DBG_ERR("gximg_ise_open() failed\r\n");
			return E_PAR;
		}
		{
			switch (p_dst_img->pxlfmt) {
			case VDO_PXLFMT_ARGB1555:
				ise_op.io_pack_fmt = KDRV_ISE_ARGB1555;
				break;
			case VDO_PXLFMT_ARGB4444:
				ise_op.io_pack_fmt = KDRV_ISE_ARGB4444;
				break;
			case VDO_PXLFMT_ARGB8888:
				ise_op.io_pack_fmt = KDRV_ISE_ARGB8888;
				break;
			case VDO_PXLFMT_RGB565:
				ise_op.io_pack_fmt = KDRV_ISE_RGB565;
				break;
			default:
				ise_op.io_pack_fmt = KDRV_ISE_Y8;
			}
			ise_op.scl_method = scale_method;
			ise_op.in_buf_flush = check_no_flush(usePA, no_flush);
			ise_op.out_buf_flush = check_no_flush(usePA, no_flush);
			ise_op.phy_addr_en = usePA;
			ise_op.in_width = p_src_rect->w;
			ise_op.in_height = p_src_rect->h;
			ise_op.in_lofs = p_src_img->loff[0];
			ise_op.in_addr = p_src_img->addr[0] + in_loff[0];
			ise_op.out_width = p_dst_rect->w;
			ise_op.out_height = p_dst_rect->h;
			ise_op.out_lofs = p_dst_img->loff[0];
			ise_op.out_addr = p_dst_img->addr[0] + out_loff[0];
			if (gximg_check_ise_limit(&ise_op)) {
				DBG_ERR("checking ise limit failed\r\n");
				gximg_ise_close(engine);
				return E_PAR;
			}
			if (gximg_ise_set_mode(engine, &ise_op) != E_OK) {
				DBG_ERR("ise_setMode error ..\r\n");
				gximg_ise_close(engine);
				return E_PAR;
			}
			if(gximg_ise_start(engine, &ise_trigger) != E_OK){
				DBG_ERR("gximg_ise_start() fail\r\n");
				gximg_ise_close(engine);
				return E_PAR;
			}
		}

		for(i = 1 ; i < plane_num ; ++i){
			if (plane_num > 1) {
				gximg_calc_plane_rect(p_src_img, i, p_src_rect, &tmp_src_plane_rect);
				gximg_calc_plane_rect(p_dst_img, i, p_dst_rect, &tmp_dst_plane_rect);

				if (tmp_src_plane_rect.w >= (INT32)max_scale_factor * tmp_dst_plane_rect.w ||
					tmp_src_plane_rect.h >= (INT32)max_scale_factor * tmp_dst_plane_rect.h ||
					tmp_dst_plane_rect.w >= (INT32)max_scale_factor * tmp_src_plane_rect.w ||
					tmp_dst_plane_rect.h >= (INT32)max_scale_factor * tmp_src_plane_rect.h) {
					DBG_ERR("Plane%d scale over %d, SrcW=%d,SrcH=%d,DstW=%d,DstH=%d\r\n",
							plane_num, max_scale_factor, tmp_src_plane_rect.w, tmp_src_plane_rect.h, tmp_dst_plane_rect.w, tmp_dst_plane_rect.h);
					return E_PAR;
				}

				if (p_src_img->pxlfmt == VDO_PXLFMT_ARGB8565) {
					ise_op.io_pack_fmt = KDRV_ISE_RGB565;
				} else if(p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR){
					//ise_op.io_pack_fmt = KDRV_ISE_Y4;
					ise_op.io_pack_fmt = KDRV_ISE_Y8;
				} else {
					ise_op.io_pack_fmt = KDRV_ISE_UVP;
				}
				ise_op.scl_method = scale_method;
				ise_op.in_buf_flush = check_no_flush(usePA, no_flush);
				ise_op.out_buf_flush = check_no_flush(usePA, no_flush);
				ise_op.phy_addr_en = usePA;
				if(p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR)
					ise_op.in_width = tmp_src_plane_rect.w;
				else
					// UV packed the UV width should be the half of Y width
					ise_op.in_width = tmp_src_plane_rect.w >> 1;
				ise_op.in_height = tmp_src_plane_rect.h;
				ise_op.in_lofs = p_src_img->loff[i];
				ise_op.in_addr = p_src_img->addr[i] + in_loff[i];
				if(p_dst_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR)
					ise_op.out_width = tmp_dst_plane_rect.w;
				else
					// UV packed the UV width should be the half of Y width
					ise_op.out_width = tmp_dst_plane_rect.w >> 1;
				ise_op.out_height = tmp_dst_plane_rect.h;
				ise_op.out_lofs = p_dst_img->loff[i];
				ise_op.out_addr = p_dst_img->addr[i] + out_loff[i];

				if (0 == ise_op.out_width || 0 == ise_op.out_height) {
					//add this check to prevent ise_setMode divide_by_zero (coverity)
					DBG_ERR("ise_op w(%d) or h(%d) invalid\r\n", ise_op.out_width, ise_op.out_height);
					gximg_ise_close(engine);
					return E_PAR;
				}

				if (gximg_check_ise_limit(&ise_op)) {
					DBG_ERR("checking ise limit failed\r\n");
					gximg_ise_close(engine);
					return E_PAR;
				}
				if (gximg_ise_set_mode(engine, &ise_op) != E_OK) {
					DBG_ERR("ise_setMode error ..\r\n");
					gximg_ise_close(engine);
					return E_PAR;
				}
#if 0
				DBG_REG_DUMP();
#endif
				if(gximg_ise_start(engine, &ise_trigger) != E_OK){
					DBG_ERR("gximg_ise_start() fail\r\n");
					gximg_ise_close(engine);
					return E_PAR;
				}
			}
		}
		if(gximg_ise_close(engine) != E_OK){
			DBG_ERR("gximg_ise_close() failed\r\n");
			return E_PAR;
		}
	}
	DBG_FUNC_END("[scale]\r\n");

	return E_OK;
}
#endif

#if USE_FD
static ER gximg_scale_by_fd(PVDO_FRAME p_src_img, IRECT *p_src_rect, PVDO_FRAME p_dst_img, IRECT *p_dst_rect)
{
#define SCALE_FD_MAX_W              1920
#define SCALE_FD_MAX_H              1920
#define SCALE_FD_MAX_SCALE_FACTOR   4

	FDE_OPENOBJ   fde_obj = {0};
	FDE_FDE_PRAM  fde_info = {0};
	UINT32        in_offset, out_offset, loff;
	FDE_SCLFAC    scale_factor;
	float         w_factor, h_factor;
	ER            er_return = E_OK;

	DBG_FUNC_BEGIN("[scale]\r\n");
	DBG_MSG("[scale]In w =%d, h=%d, out w=%d, h=%d\r\n", p_src_rect->w, p_src_rect->h, p_dst_rect->w, p_dst_rect->h);
	if (!(p_dst_img->pxlfmt == VDO_PXLFMT_Y8)) {
		DBG_ERR("dst pixel format %x\r\n", p_dst_img->pxlfmt);
		return E_PAR;
	}
	// FD scale just support Y plane
	in_offset = gximg_calc_plane_offset(p_src_img, 0, p_src_rect);
	out_offset = gximg_calc_plane_offset(p_dst_img, 0, p_dst_rect);

	// check FD alignment begin
	loff = 4;

	w_factor = (float)p_src_rect->w / p_dst_rect->w;
	h_factor = (float)p_src_rect->h / p_dst_rect->h;

	if (p_src_rect->w > SCALE_FD_MAX_W) {
		DBG_ERR("input Size SrcW=%d > limit=%d\r\n", p_src_rect->w, SCALE_FD_MAX_W);
		return E_PAR;
	}
	if (p_src_rect->h > SCALE_FD_MAX_H) {
		DBG_ERR("input Size SrcH=%d > limit=%d\r\n", p_src_rect->h, SCALE_FD_MAX_H);
		return E_PAR;
	}
	if (p_src_rect->w < p_dst_rect->w || p_src_rect->h < p_dst_rect->h) {
		DBG_ERR("No scale up support, SrcW=%d,SrcH=%d,DstW=%d,DstH=%d\r\n", p_src_rect->w, p_src_rect->h, p_dst_rect->w, p_dst_rect->h);
		return E_PAR;
	}
	if (w_factor >= SCALE_FD_MAX_SCALE_FACTOR || h_factor >= SCALE_FD_MAX_SCALE_FACTOR) {
		DBG_ERR("scale factor over %d, SrcW=%d,SrcH=%d,DstW=%d,DstH=%d\r\n", SCALE_FD_MAX_SCALE_FACTOR, p_src_rect->w, p_src_rect->h, p_dst_rect->w, p_dst_rect->h);
		return E_PAR;
	}
	if (p_src_img->loff[0] % loff) {
		DBG_ERR("In lineoff not %d byte aligned\r\n", loff);
		return E_PAR;
	}
	if (p_dst_img->loff[0] % loff) {
		DBG_ERR("Out lineoff not %d byte aligned\r\n", loff);
		return E_PAR;
	}
	// check FD alignment end
	fde_obj.FP_FDEISR_CB = NULL;
	fde_obj.uiFdeClockSel = DxSys_GetCfgClk(CFG_FDE_CLK);

	if (E_OK != fde_open(&fde_obj)) {
		DBG_ERR("fde_open failed\r\n");
		return E_SYS;
	}

	fde_info.uiInAddr = p_src_img->addr[0] + in_offset;
	fde_info.uiOutAddr0 = p_dst_img->addr[0] + out_offset;
	fde_info.uiWidth = p_src_rect->w;
	fde_info.uiHeight = p_src_rect->h;
	fde_info.uiInOfs = p_src_img->loff[0];
	fde_info.uiOutOfs = p_dst_img->loff[0];

	scale_factor.uiHIntFac = (UINT16)w_factor;
	scale_factor.uiHFrFac = (UINT16)((w_factor - scale_factor.uiHIntFac) * 0XFFFF);
	scale_factor.uiVIntFac = (UINT16)h_factor;
	scael_factor.uiVFrFac = (UINT16)((h_factor - scale_factor.uiVIntFac) * 0XFFFF);
	fde_info.pSclFac = &scale_factor;
	fde_info.uiSclNum = 1;

	DBG_MSG("[scale]HIntFac=%d, HFrFac=%d, VIntFac=%d, VFrFac=%d\r\n", scale_factor.uiHIntFac, scale_factor.uiHFrFac, scale_factor.uiVIntFac, scale_factor.uiVFrFac);

	if (E_OK != fde_setFDMode(FDE_FD_SCALING, &fde_info)) {
		DBG_ERR("fde_open failed\r\n");
		er_return = E_SYS;
		goto L_GxImg_ScaleByFD_EndClose;
	}

	if (E_OK != fde_start()) {
		DBG_ERR("fde_start failed\r\n");
		er_return = E_SYS;
		goto L_GxImg_ScaleByFD_EndClose;
	}

L_GxImg_ScaleByFD_EndClose:
	if (E_OK != fde_close()) {
		DBG_ERR("fde_close failed\r\n");
	}

	DBG_FUNC_END("[scale]\r\n");

	return erReturn;

}
#endif
static void gximg_jpg_scale_up_down_planar(GXIMG_JPEG_SCALE_UP_DOWN *p_scale)
{
	UINT32 i, j, deltax, deltay, out_offset, in_offset;
	UINT8 *dst, *src, *src_start;
	UINT32 posx, posy;

	//DBG_FUNC_CHK("[scale]\r\n");
	out_offset = p_scale->output_offset;
	in_offset  = p_scale->input_offset;

	deltax = (p_scale->input_width << 8) / p_scale->output_width;
	deltay = (p_scale->input_height << 8) / p_scale->output_height;
	posy = 0;

	for (i = 0; i < p_scale->output_height; i++) {
		dst = (p_scale->p_dst + out_offset * i);
		src_start = (p_scale->p_src + in_offset * (posy >> 8));
		posx = 0;
		for (j = 0; j < (p_scale->output_width); j++) {
			src = src_start + (posx >> 8);
			*dst++ = *src;
			posx += deltax;
		}
		posy += deltay;
	}
}
static void gximg_jpg_scale_up_down_packed(GXIMG_JPEG_SCALE_UP_DOWN *p_scale)
{
	UINT32 i, j, deltax, deltay, out_offset, in_offset;
	UINT8  *dst, *src, *src_start;
	UINT32 posx, posy;

	//DBG_FUNC_CHK("[scale]\r\n");
	out_offset = p_scale->output_offset;
	in_offset  = p_scale->input_offset;

	deltax = (p_scale->input_width << 8) / p_scale->output_width;
	deltay = (p_scale->input_height << 8) / p_scale->output_height;
	posy = 0;

	for (i = 0; i < p_scale->output_height; i++) {
		dst = (p_scale->p_dst + out_offset * i);
		src_start = (p_scale->p_src + in_offset * (posy >> 8));
		posx = 0;
		for (j = 0; j < (p_scale->output_width >> 1); j++) {
			src = (UINT8 *)ALIGN_FLOOR(((UINT32)src_start + (posx >> 7)), 2);
			*dst++ = *src++;
			*dst++ = *src++;
			posx += (deltax);
		}
		posy += deltay;
	}
}

static ER gximg_scale_by_fw(PVDO_FRAME p_src_img, IRECT    *p_src_rect, PVDO_FRAME p_dst_img, IRECT    *p_dst_rect)
{
	GXIMG_JPEG_SCALE_UP_DOWN jpeg_scale;
	UINT32   i, src_plane_num, dst_plane_num, plane_num;
	UINT32   p_in_offset[GXIMG_MAX_PLANE_NUM] = {0}, p_out_offset[GXIMG_MAX_PLANE_NUM] = {0};
	UINT32   p_in_width[GXIMG_MAX_PLANE_NUM] = {0}, p_in_height[GXIMG_MAX_PLANE_NUM] = {0}, p_out_width[GXIMG_MAX_PLANE_NUM] = {0}, p_out_height[GXIMG_MAX_PLANE_NUM] = {0};
	IRECT    tmp_src_plane_rect = {0}, tmp_dst_plane_rect = {0};

	//DBG_FUNC_CHK("[scale]\r\n");
	if (p_src_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR || p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR) {
		src_plane_num = 3;
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_YUV422 || p_src_img->pxlfmt == VDO_PXLFMT_YUV420) {
		src_plane_num = 2;
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_Y8) {
		src_plane_num = 1;
	} else {
		DBG_ERR("Src pixel format %x\r\n", p_src_img->pxlfmt);
		return E_PAR;
	}

	for (i = 0; i < src_plane_num; i++) {
		gximg_calc_plane_rect(p_src_img, i, p_src_rect, &tmp_src_plane_rect);
		p_in_offset[i] = gximg_calc_plane_offset(p_src_img, i, &tmp_src_plane_rect);
		p_in_width[i] = tmp_src_plane_rect.w;
		p_in_height[i] = tmp_src_plane_rect.h;
	}

	if (p_dst_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR || p_dst_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR) {
		dst_plane_num = 3;
	} else if (p_dst_img->pxlfmt == VDO_PXLFMT_YUV422 || p_dst_img->pxlfmt == VDO_PXLFMT_YUV420) {
		dst_plane_num = 2;
	} else if (p_dst_img->pxlfmt == VDO_PXLFMT_Y8) {
		dst_plane_num = 1;
	} else {
		DBG_ERR("Dst pixel format %x\r\n", p_dst_img->pxlfmt);
		return E_PAR;
	}

	for (i = 0; i < dst_plane_num; i++) {
		gximg_calc_plane_rect(p_dst_img, i, p_dst_rect, &tmp_dst_plane_rect);
		p_out_offset[i] = gximg_calc_plane_offset(p_dst_img, i, &tmp_dst_plane_rect);
		p_out_width[i] = tmp_dst_plane_rect.w;
		p_out_height[i] = tmp_dst_plane_rect.h;
	}
	plane_num = (src_plane_num <= dst_plane_num) ? src_plane_num : dst_plane_num;


	for (i = 0; i < plane_num; i++) {
		jpeg_scale.p_src = (UINT8 *)p_src_img->addr[i] + p_in_offset[i];
		jpeg_scale.p_dst = (UINT8 *)p_dst_img->addr[i] + p_out_offset[i];
		jpeg_scale.input_width = p_in_width[i];
		jpeg_scale.input_height = p_in_height[i];
		jpeg_scale.input_offset = p_src_img->loff[i];
		jpeg_scale.output_width = p_out_width[i];
		jpeg_scale.output_height = p_out_height[i];
		jpeg_scale.output_offset = p_dst_img->loff[i];
		// UV packed
		if (plane_num == 2 && i == 1) {
			gximg_jpg_scale_up_down_packed(&jpeg_scale);
		}
		// others
		else {
			gximg_jpg_scale_up_down_planar(&jpeg_scale);
		}
	}
	return E_OK;
}

static ER gximg_scale_adjust_rect(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region, IRECT *p_src_rect, IRECT *p_dst_rect)
{
	INT32 srcx, srcy, srcw, srch;
	INT32 dstx, dsty, dstw, dsth;

	DBG_FUNC_BEGIN("[scale]\r\n");
	if (!p_src_img) {
		DBG_ERR("p_src_img is Null\r\n");
		return E_PAR;
	} else if (!p_dst_img) {
		DBG_ERR("p_dst_img is Null\r\n");
		return E_PAR;
	} else if (p_src_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_src_img is not initialized\r\n");
		return E_PAR;
	} else if (p_dst_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_dst_img is not initialized\r\n");
		return E_PAR;
	}
	DBG_MSG("[scale]p_src_img.W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], p_src_img->loff[1], p_src_img->loff[2], p_src_img->pxlfmt);
	DBG_MSG("[scale]p_dst_img.W=%04d,H=%04d,OutOff[0]=%04d,OutOff[1]=%04d,OutOff[2]=%04d,PxlFmt=%x\r\n", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], p_dst_img->loff[1], p_dst_img->loff[2], p_dst_img->pxlfmt);

	if (p_src_region != GXIMG_REGION_MATCH_IMG) {
		//GXIMG_SCALE_RECT(p_src_img, p_src_region);
		DBG_IND("[scale]pSrc.x=%d,y=%d,w=%d,h=%d\r\n", p_src_region->x, p_src_region->y, p_src_region->w, p_src_region->h);
		// check source x, y, should not < 0
		if (p_src_region->x < 0 || p_src_region->y < 0) {
			DBG_ERR("Pos x=%d, y=%d\r\n", p_src_region->x, p_src_region->y);
			return E_PAR;
		} else if (!p_src_region->w) {
			DBG_ERR("Region w=%d\r\n", p_src_region->w);
			return E_PAR;
		} else if (!p_src_region->h) {
			DBG_ERR("Region h=%d\r\n", p_src_region->h);
			return E_PAR;
		} else {
			if (p_src_region->x + p_src_region->w > (INT32)p_src_img->size.w) {
				p_src_region->w = p_src_img->size.w - p_src_region->x;
				srcw = p_src_region->w;
			} else {
				srcw = p_src_region->w;
			}
			if (p_src_region->y + p_src_region->h > (INT32)p_src_img->size.h) {
				p_src_region->h = p_src_img->size.h - p_src_region->y;
				srch = p_src_region->h;
			} else {
				srch = p_src_region->h;
			}
			srcx = p_src_region->x;
			srcy = p_src_region->y;
		}
	}
	// pSrcRegion is NULL
	else {
		srcx = 0;
		srcy = 0;
		srcw = p_src_img->size.w;
		srch = p_src_img->size.h;
	}

	if (p_dst_region != GXIMG_REGION_MATCH_IMG) {
		//GXIMG_SCALE_RECT(p_dst_img, p_dst_region);
		DBG_MSG("[scale]pDst.x=%d,y=%d,w=%d,h=%d\r\n", p_dst_region->x, p_dst_region->y, p_dst_region->w, p_dst_region->h);
		if (p_dst_region->x < 0) {
			if (p_dst_region->x + (INT32)p_dst_region->w <= 0) {
				DBG_ERR("Dst x= %d\r\n", p_dst_region->x);
				return E_PAR;
			} else if (p_dst_region->x + p_dst_region->w > (INT32)p_dst_img->size.w) {
				dstw = p_dst_img->size.w;

			} else {
				dstw = p_dst_region->x + p_dst_region->w;
			}
			dstx = 0;
			// ---------------------------------------------
			// need to re-calculate srcX, srcW
			srcx = srcx - (p_dst_region->x * srcw / p_dst_region->w);
			srcw = srcw * dstw / p_dst_region->w;
			//----------------------------------------------
		}
		// x >= 0
		else {
			if (p_dst_region->x >= (INT32)p_dst_img->size.w) {
				DBG_ERR("Dst x= %d\r\n", p_dst_region->x);
				return E_PAR;
			} else if (p_dst_region->x + p_dst_region->w > (INT32)p_dst_img->size.w) {
				dstw = p_dst_img->size.w - p_dst_region->x;
				// ---------------------------------------------
				// need to re-calculate srcX, srcW
				//srcX = srcX+((p_dst_region->x+ p_dst_region->w-p_dst_img->Width)*srcW/p_dst_region->w);
				srcw = srcw * dstw / p_dst_region->w;
				//----------------------------------------------
			} else {
				dstw = p_dst_region->w;
			}
			dstx = p_dst_region->x;
		}
		if (p_dst_region->y < 0) {
			if (p_dst_region->y + (INT32)p_dst_region->h <= 0) {
				DBG_ERR("Dst y= %d\r\n", p_dst_region->y);
				return E_PAR;
			} else if (p_dst_region->y + p_dst_region->h > (INT32)p_dst_img->size.h) {
				dsth = p_dst_img->size.h;
			} else {
				dsth = p_dst_region->y + p_dst_region->h;
			}
			dsty = 0;
			// ---------------------------------------------
			// need to re-calculate srcH, srcY
			srcy = srcy - (p_dst_region->y * srch / p_dst_region->h);
			srch = srch * dsth / p_dst_region->h;
			//----------------------------------------------
		}
		// y >= 0
		else {
			if (p_dst_region->y >= (INT32)p_dst_img->size.h) {
				DBG_ERR("Dst y= %d\r\n", p_dst_region->y);
				return E_PAR;
			} else if (p_dst_region->y + p_dst_region->h > (INT32)p_dst_img->size.h) {
				dsth = p_dst_img->size.h - p_dst_region->y;
				// ---------------------------------------------
				// need to re-calculate srcH, srcY
				//srcy = srcy+((p_dst_region->y+ p_dst_region->h-p_dst_img->size.h)*srch/p_dst_region->h);
				srch = srch * dsth / p_dst_region->h;
				//----------------------------------------------
			} else {
				dsth = p_dst_region->h;
			}
			dsty = p_dst_region->y;
		}
	}
	// pDstRegion is NULL
	else {
		dstx = 0;
		dsty = 0;
		dstw = p_dst_img->size.w;
		dsth = p_dst_img->size.h;
	}

	p_src_rect->x = srcx;
	p_src_rect->y = srcy;
	p_src_rect->w = srcw;
	p_src_rect->h = srch;
	p_dst_rect->x = dstx;
	p_dst_rect->y = dsty;
	p_dst_rect->w = dstw;
	p_dst_rect->h = dsth;
	gximg_adjust_uv_packed_align(p_src_img->pxlfmt, p_src_rect);
	gximg_adjust_uv_packed_align(p_dst_img->pxlfmt, p_dst_rect);
	if (p_src_rect->w == 0 || p_src_rect->h == 0 || p_dst_rect->w == 0 || p_dst_rect->h == 0) {
		DBG_ERR("scale srcW=%04d,srcH=%04d,dstW=%04d,dstH=%04d\r\n", srcw, srch, dstw, dsth);
		return E_PAR;
	}
	DBG_MSG("[scale]srcX=%04d,srcY=%04d,srcW=%04d,srcH=%04d\r\n", p_src_rect->x, p_src_rect->y, p_src_rect->w, p_src_rect->h);
	DBG_MSG("[scale]dstX=%04d,dstY=%04d,dstW=%04d,dstH=%04d\r\n", p_dst_rect->x, p_dst_rect->y, p_dst_rect->w, p_dst_rect->h);
	DBG_FUNC_END("[scale]\r\n");
	return E_OK;
}

static ER _gximg_scale_data_ex_with_flush(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region, GXIMG_SC_ENG engine, int no_flush, GXIMG_SCALE_METHOD method, int usePA)
{
	IRECT     src_rect = {0};
	IRECT     dst_rect = {0};
	ER        result = E_OK;

	result = gximg_scale_adjust_rect(p_src_img, p_src_region, p_dst_img, p_dst_region, &src_rect, &dst_rect);
	if (result == E_OK) {
#if USE_ISE
		result = gximg_scale_by_ise(p_src_img, &src_rect, p_dst_img, &dst_rect, engine, no_flush, method, usePA);
#if DUMP_SCALE_RAW
		if (p_src_region && result == E_OK) {
			char path[128];
			char src_fmt[4] = "422", dst_fmt[4] = "422";

			if (p_src_img->pxlfmt == 3) {
				sprintf(src_fmt, "420");
			}
			if (p_dst_img->pxlfmt == 3) {
				sprintf(dst_fmt, "420");
			}
			sprintf(path, "A:\\Gximage\\ISE%04dx%04d_lf%04d(%dx%d_%s)To(%dx%d_%s).raw", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], src_rect.w, src_rect.h, src_fmt, dst_rect.w, dst_rect.h, dst_fmt);
			DBG_DUMP("^MGxImg_DumpBuf %s\r\n", path);
			GxImg_DumpBuf(path, p_dst_img, GXIMG_DUMP_ALL);
		}
#endif
#elif 0 //USE_IME
		result = gximg_scale_by_ime(p_src_img, &src_rect, p_dst_img, &dst_rect);
#else
		result = gximg_scale_by_fw(p_src_img, &src_rect, p_dst_img, &dst_rect);
#endif
		if (result != E_OK) {
			//DBG_ERR("Use FW Scale instead\r\n");
			//result = gximg_scale_by_fw(p_src_img, &src_rect, p_dst_img, &dst_rect);
			DBG_ERR("gximg_scale_data_ex() fail\n");
		}
	}
	return result;
}

ER gximg_scale_data(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region, GXIMG_SCALE_METHOD method, int usePA)
{
	return _gximg_scale_data_ex_with_flush(p_src_img, p_src_region, p_dst_img, p_dst_region, GXIMG_SC_ENG1, 0, method, usePA);
}

ER gximg_scale_data_no_flush(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region, GXIMG_SCALE_METHOD method, int usePA)
{
	return _gximg_scale_data_ex_with_flush(p_src_img, p_src_region, p_dst_img, p_dst_region, GXIMG_SC_ENG1, 1, method, usePA);
}

ER gximg_scale_data_ex(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region, GXIMG_SC_ENG engine, GXIMG_SCALE_METHOD method)
{
	return _gximg_scale_data_ex_with_flush(p_src_img, p_src_region, p_dst_img, p_dst_region, engine, 0, method, 0);
}

ER gximg_scale_data_fine(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region)
{
	IRECT     src_rect = {0};
	IRECT     dst_rect = {0};
	ER    result = E_OK;

	result = gximg_scale_adjust_rect(p_src_img, p_src_region, p_dst_img, p_dst_region, &src_rect, &dst_rect);
	if (result == E_OK) {
#if 0 //USE_IME
		result = gximg_scale_by_ime(p_src_img, &src_rect, p_dst_img, &dst_rect);
#if DUMP_SCALE_RAW
		if (p_src_region && result == E_OK) {
			char path[128];
			char src_fmt[4] = "422", dst_fmt[4] = "422";

			if (p_src_img->pxlfmt == 3) {
				sprintf(src_fmt, "420");
			}
			if (p_dst_img->pxlfmt == 3) {
				sprintf(dst_fmt, "420");
			}
			sprintf(path, "A:\\Gximage\\IME%04dx%04d_lf%04d(%dx%d_%s)To(%dx%d_%s).raw", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], src_rect.w, src_rect.h, src_fmt, dst_rect.w, dst_rect.h, dst_fmt);
			DBG_DUMP("^MGxImg_DumpBuf %s\r\n", path);
			GxImg_DumpBuf(path, p_dst_img, GXIMG_DUMP_ALL);
		}
#endif
#elif USE_ISE
		result = gximg_scale_by_ise(p_src_img, &src_rect, p_dst_img, &dst_rect, GXIMG_SC_ENG1, 0, GXIMG_SCALE_AUTO, 0);
#else
		result = gximg_scale_by_fw(p_src_img, &src_rect, p_dst_img, &dst_rect);
#endif
		if (result != E_OK) {
			DBG_ERR("Use FW scale instead\r\n");
			result = gximg_scale_by_fw(p_src_img, &src_rect, p_dst_img, &dst_rect);

		}
	}
	return result;
}


ER gximg_scale_data_down_y_only(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region)
{
	IRECT     src_rect = {0};
	IRECT     dst_rect = {0};
	ER        result = E_OK;

	result = gximg_scale_adjust_rect(p_src_img, p_src_region, p_dst_img, p_dst_region, &src_rect, &dst_rect);
	if (result == E_OK) {
#if DUMP_SCALE_RAW
		if (result == E_OK) {
			char path[128];
			char src_fmt[4] = "y", dst_fmt[4] = "y";

			sprintf(path, "A:\\Gximage\\FD%04dx%04d_lf%04d(%dx%d_%s)To(%dx%d_%s)_IN.raw", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], src_rect.w, src_rect.h, src_fmt, dst_rect.w, dst_rect.h, dst_fmt);
			DBG_DUMP("^MGxImg_DumpBuf %s\r\n", path);
			gximg_dump_buf(path, p_src_img, GXIMG_DUMP_Y);
		}
#endif


#if USE_FD
		result = gximg_scale_by_fd(p_src_img, &src_rect, p_dst_img, &dst_rect);
#else
		result = gximg_scale_by_fw(p_src_img, &src_rect, p_dst_img, &dst_rect);
#endif

#if DUMP_SCALE_RAW
		if (result == E_OK) {
			char path[128];
			char src_fmt[4] = "y", dst_fmt[4] = "y";

			sprintf(path, "A:\\Gximage\\FD%04dx%04d_lf%04d(%dx%d_%s)To(%dx%d_%s)_OUT.raw", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], src_rect.w, src_rect.h, src_fmt, dst_rect.w, dst_rect.h, dst_fmt);
			DBG_DUMP("^MGxImg_DumpBuf %s\r\n", path);
			gximg_dump_buf(path, p_dst_img, GXIMG_DUMP_Y);
		}
#endif

		if (result != E_OK) {
			DBG_ERR("Use FW scale instead\r\n");
			result = gximg_scale_by_fw(p_src_img, &src_rect, p_dst_img, &dst_rect);
		}
	}
	return result;
}

static void gximg_clr_rotate_redundant_data(PVDO_FRAME p_src_img, PVDO_FRAME p_dst_img, UINT32 rotate_dir, UINT32 align_width, UINT32 align_height)
{
	IRECT      fill_region = {0};
	IRECT      tmp_rect = {0}, tmp_plane_rect = {0};
	BOOL       is_clear = FALSE;

	//original redundant data graph
	//                      XX
	//                      XX
	//                      XX
	//                      XX
	//                      XX
	//                      XX
	//                      XX
	//                      XX
	//                      XX
	//                      XX
	//XXXXXXXXXXXXXXXXXXXXXXXX
	//XXXXXXXXXXXXXXXXXXXXXXXX

	// clear redundant area
	DBG_MSG("[rotate]RotateDir = %d, Src w=%d, h=%d, align w=%d,h=%d\r\n", rotate_dir, p_src_img->size.w, p_src_img->size.h, align_width, align_height);
	if (rotate_dir == VDO_DIR_ROTATE_90) {
		// clear redundant data graph
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XXXXXXXXXXXXXXXXXXXXXXXX
		//XXXXXXXXXXXXXXXXXXXXXXXX

		// clear Left H
		//fill_region.x = 0;
		//fill_region.y = 0;
		fill_region.w = ALIGN_CEIL(align_height - p_src_img->size.h, 2); // 420  width should be 2 bytes align
		//fill_region.h = align_width;
		p_dst_img->size.w = align_height;
		p_dst_img->size.h = align_width;
		if (fill_region.w) {
			is_clear = TRUE;
			tmp_rect.x = fill_region.w;
			//gximg_fill_data(p_dst_img,&fill_region,COLOR_YUV_BLACK);
			//DBG_MSG("[rotate]Clear Region x=%d, y=%d, w=%d,h=%d\r\n",fill_region.x,fill_region.y,fill_region.w,fill_region.h);
		}

		// clear Lower W
		/*
		fill_region.x = 0;
		fill_region.y = p_src_img->size.w;
		fill_region.w = align_height;
		fill_region.h = ALIGN_CEIL(align_width - p_src_img->size.w,2);
		if (fill_region.h)
		{
		    is_clear = TRUE;
		    gximg_fill_data(p_dst_img,&fill_region,COLOR_YUV_BLACK);
		    DBG_MSG("[rotate]Clear Region x=%d, y=%d, w=%d,h=%d\r\n",fill_region.x,fill_region.y,fill_region.w,fill_region.h);
		}
		*/
		p_dst_img->size.w = p_src_img->size.h;
		p_dst_img->size.h = p_src_img->size.w;
	} else if (rotate_dir == VDO_DIR_ROTATE_270) {
		// clear redundant data graph
		//XXXXXXXXXXXXXXXXXXXXXXXX
		//XXXXXXXXXXXXXXXXXXXXXXXX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX

		// clear Right H
		/*
		fill_region.x = p_src_img->size.h;;
		fill_region.y = 0;
		fill_region.w = ALIGN_CEIL(align_height - p_src_img->size.h,2); // 420  width should be 2 bytes align
		fill_region.h = align_width;
		p_dst_img->size.w = align_height;
		p_dst_img->size.h = align_width;
		if (fill_region.w)
		{
		    is_clear = TRUE;
		    gximg_fill_data(p_dst_img,&fill_region,COLOR_YUV_BLACK);
		    DBG_MSG("[rotate]Clear Region x=%d, y=%d, w=%d,h=%d\r\n",fill_region.x,fill_region.y,fill_region.w,fill_region.h);
		}
		*/
		// clear Upper W
		//fill_region.x = 0;
		//fill_region.y = 0;
		//fill_region.w = align_height;
		fill_region.h = ALIGN_CEIL(align_width - p_src_img->size.w, 2);
		if (fill_region.h) {
			is_clear = TRUE;
			tmp_rect.y = fill_region.h;
			//gximg_fill_data(p_dst_img,&fill_region,COLOR_YUV_BLACK);
			//DBG_MSG("[rotate]Clear Region x=%d, y=%d, w=%d,h=%d\r\n",fill_region.x,fill_region.y,fill_region.w,fill_region.h);
		}
		p_dst_img->size.w = p_src_img->size.h;
		p_dst_img->size.h = p_src_img->size.w;
	} else if (rotate_dir == VDO_DIR_ROTATE_180) {
		// clear redundant data graph
		//XXXXXXXXXXXXXXXXXXXXXXXX
		//XXXXXXXXXXXXXXXXXXXXXXXX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX

		// clear Left H
		//fill_region.x = 0;
		//fill_region.y = 0;
		fill_region.w = ALIGN_CEIL(align_width - p_src_img->size.w, 2); // 420  width should be 2 bytes align
		//fill_region.h = align_height;
		p_dst_img->size.w = align_width;
		p_dst_img->size.h = align_height;
		if (fill_region.w) {
			is_clear = TRUE;
			tmp_rect.x = fill_region.w;
			//gximg_fill_data(p_dst_img,&fill_region,COLOR_YUV_BLACK);
			//DBG_MSG("[rotate]Clear Region x=%d, y=%d, w=%d,h=%d\r\n",fill_region.x,fill_region.y,fill_region.w,fill_region.h);
		}
		// clear Upper W
		//fill_region.x = 0;
		//fill_region.y = 0;
		//fill_region.w = p_src_img->size.w;
		fill_region.h = ALIGN_CEIL(align_height - p_src_img->size.h, 2);
		p_dst_img->size.w = align_width;
		p_dst_img->size.h = align_height;
		if (fill_region.h) {
			is_clear = TRUE;
			tmp_rect.y = fill_region.h;
			//gximg_fill_data(p_dst_img,&fill_region,COLOR_YUV_BLACK);
			//DBG_MSG("[rotate]Clear Region x=%d, y=%d, w=%d,h=%d\r\n",fill_region.x,fill_region.y,fill_region.w,fill_region.h);
		}
		p_dst_img->size.w = p_src_img->size.w;
		p_dst_img->size.h = p_src_img->size.h;
	} else if (rotate_dir == VDO_DIR_MIRRORX) {
		// clear redundant data graph
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XXXXXXXXXXXXXXXXXXXXXXXX
		//XXXXXXXXXXXXXXXXXXXXXXXX

		// clear Left H
		//fill_region.x = 0;
		//fill_region.y = 0;
		fill_region.w = ALIGN_CEIL(align_width - p_src_img->size.w, 2); // 420  width should be 2 bytes align
		//fill_region.h = align_height;
		p_dst_img->size.w = align_width;
		p_dst_img->size.h = align_height;
		if (fill_region.w) {
			is_clear = TRUE;
			tmp_rect.x = fill_region.w;
			//gximg_fill_data(p_dst_img,&fill_region,COLOR_YUV_BLACK);
			//DBG_MSG("[rotate]Clear Region x=%d, y=%d, w=%d,h=%d\r\n",fill_region.x,fill_region.y,fill_region.w,fill_region.h);
		}
		p_dst_img->size.w = p_src_img->size.w;
		p_dst_img->size.h = p_src_img->size.h;
	} else if (rotate_dir == VDO_DIR_MIRRORY) {
		// clear redundant data graph
		//XXXXXXXXXXXXXXXXXXXXXXXX
		//XXXXXXXXXXXXXXXXXXXXXXXX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		// clear Upper W
		//fill_region.x = 0;
		//fill_region.y = 0;
		//fill_region.w = p_src_img->size.w;
		fill_region.h = ALIGN_CEIL(align_height - p_src_img->size.h, 2);
		p_dst_img->size.w = align_width;
		p_dst_img->size.h = align_height;
		if (fill_region.h) {
			is_clear = TRUE;
			tmp_rect.y = fill_region.h;
			//gximg_fill_data(p_dst_img,&fill_region,COLOR_YUV_BLACK);
			//DBG_MSG("[rotate]Clear Region x=%d, y=%d, w=%d,h=%d\r\n",fill_region.x,fill_region.y,fill_region.w,fill_region.h);
		}
		p_dst_img->size.w = p_src_img->size.w;
		p_dst_img->size.h = p_src_img->size.h;
	} else if (rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_90)) {
		// clear redundant data graph
		//XXXXXXXXXXXXXXXXXXXXXXXX
		//XXXXXXXXXXXXXXXXXXXXXXXX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX
		//XX

		tmp_rect.x = ALIGN_CEIL(align_height - p_src_img->size.h, 2); // 420  width should be 2 bytes align
		tmp_rect.y = ALIGN_CEIL(align_width - p_src_img->size.w, 2);

		if (tmp_rect.x) {
			is_clear = TRUE;
		}

		if (tmp_rect.y) {
			is_clear = TRUE;
		}

		p_dst_img->size.w = p_src_img->size.h;
		p_dst_img->size.h = p_src_img->size.w;
	} else if (rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_270)) {
		//the same as the original redundant data graph
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//XXXXXXXXXXXXXXXXXXXXXXXX
		//XXXXXXXXXXXXXXXXXXXXXXXX

		is_clear = TRUE;

		p_dst_img->size.w = p_src_img->size.h;
		p_dst_img->size.h = p_src_img->size.w;
	} else {
		p_dst_img->size.w = p_src_img->size.w;
		p_dst_img->size.h = p_src_img->size.h;
	}

	if (is_clear) {
		tmp_rect.w = p_dst_img->size.w;
		tmp_rect.h = p_dst_img->size.h;
		gximg_calc_plane_rect(p_dst_img, 0, &tmp_rect, &tmp_plane_rect);
		p_dst_img->addr[0] += gximg_calc_plane_offset(p_dst_img, 0, &tmp_plane_rect);
		gximg_calc_plane_rect(p_dst_img, 1, &tmp_rect, &tmp_plane_rect);
		p_dst_img->addr[1] += gximg_calc_plane_offset(p_dst_img, 1, &tmp_plane_rect);
		gximg_calc_plane_rect(p_dst_img, 2, &tmp_rect, &tmp_plane_rect);
		p_dst_img->addr[2] += gximg_calc_plane_offset(p_dst_img, 2, &tmp_plane_rect);
	}
}

static ER gximg_rotate_by_engine(PVDO_FRAME p_src_img, PVDO_FRAME p_dst_img, UINT32 rotate_dir, GXIMG_RT_ENG engine, int no_flush, int usePA)
{
	UINT32         align_width, align_height;
	UINT32         plane_num, i;
	IRECT          tmp_rect = {0}, tmp_plane_rect = {0};
#if USE_ROTATE
	INT32          rot_id;
#else
	UINT32         grph_id;
#endif

	DBG_FUNC_BEGIN("[rotate]\r\n");

	if (rotate_dir == VDO_DIR_ROTATE_90 ||
		rotate_dir == VDO_DIR_ROTATE_270 ||
		rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_90) ||
		rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_270)) {
		align_width = p_dst_img->size.h;
		align_height = p_dst_img->size.w;
	} else {
		align_width = p_dst_img->size.w;
		align_height = p_dst_img->size.h;
	}

	if (p_src_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR ||
	    p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR ||
	    p_src_img->pxlfmt == VDO_PXLFMT_YUV444_PLANAR) {
		plane_num = 3;
		// output is 420 planar
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_YUV422 || p_src_img->pxlfmt == VDO_PXLFMT_YUV420) {
		plane_num = 2;
		// output is 420 packed
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_Y8 ||
			p_src_img->pxlfmt == VDO_PXLFMT_I8) {
		plane_num = 1;
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_RGB888_PLANAR) {
		plane_num = 3;
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_RGB565 ||
	    p_src_img->pxlfmt == VDO_PXLFMT_ARGB1555 ||
	    p_src_img->pxlfmt == VDO_PXLFMT_ARGB4444 ||
		p_src_img->pxlfmt == VDO_PXLFMT_ARGB8888 ||
		p_src_img->pxlfmt == VDO_PXLFMT_RAW16) {
		plane_num = 1;
	} else {
		DBG_ERR("pixel format %x\r\n", p_src_img->pxlfmt);
		return E_PAR;
	}

	if (engine == GXIMG_ROTATE_RESERVE) {
		goto L_GxImg_RotateByEngine_END;    //Only need output structure, skip
	}


#if USE_ROTATE
	rot_id = gximg_rotate_map_to_grph_id(engine);
	if (rot_id < 0) {
		DBG_ERR("get rot id failed\r\n");
		return E_PAR;
	}
	DBG_MSG("[rotate]rotID = %d\r\n", rot_id);
	// Open 2D graphic module
	if (E_OK != kdrv_rotation_open(KDRV_CHIP0, KDRV_GFX2D_ROTATE)) {
		DBG_ERR("kdrv_rotation_open(%d,%d) failed\r\n", KDRV_CHIP0, KDRV_GFX2D_ROTATE);
		return E_SYS;
	}
#else
	grph_id = gximg_rotate_map_to_grph_id(engine);
	DBG_MSG("[rotate]grph_id = %d\r\n", grph_id);
	if(gximg_open_grph(grph_id) == -1){
		DBG_ERR("fail to open graphic engine(%d)\n", grph_id);
		return E_PAR;
	}
#endif

	for (i = 0; i < plane_num; i++) {
		tmp_rect.w = align_width;
		tmp_rect.h = align_height;
		gximg_calc_plane_rect(p_src_img, i, &tmp_rect, &tmp_plane_rect);

#if USE_ROTATE
		{
			KDRV_ROTATION_TRIGGER_PARAM rot_request = {0};
			ROTATION_IMG rot_img_src = {0}, rot_img_dst = {0};

			rot_img_src.img_id = ROTATION_IMG_ID_SRC;
			rot_img_src.address = (UINT32)p_src_img->addr[i];
			// Always rotate to 420. If source is 422, set CbCr's source line offset to dobule
			if ((i != 0) && (p_src_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR || p_src_img->pxlfmt == VDO_PXLFMT_YUV422)) {
				rot_img_src.lineoffset = p_src_img->loff[i] << 1;
				rot_img_src.height = tmp_plane_rect.h >> 1;
			} else {
				rot_img_src.lineoffset = p_src_img->loff[i];
				rot_img_src.height = tmp_plane_rect.h;
			}
			rot_img_src.width = tmp_plane_rect.w;
			rot_img_src.p_next = &rot_img_dst;

			rot_img_dst.img_id = ROTATION_IMG_ID_DST;       // destination image
			rot_img_dst.address = (UINT32)p_dst_img->addr[i];
			rot_img_dst.lineoffset = p_dst_img->loff[i];

			rot_request.command = gximg_map_to_graph_rotate_dir(rotate_dir);
			// UV packed need to use 16 bit operation
			if (i == 1 && (p_src_img->pxlfmt == VDO_PXLFMT_YUV422 || p_src_img->pxlfmt == VDO_PXLFMT_YUV420)) {
				rot_request.format = FORMAT_16BITS;
			} else if (p_src_img->pxlfmt == VDO_PXLFMT_RGB565 ||
				p_src_img->pxlfmt == VDO_PXLFMT_ARGB1555 ||
				p_src_img->pxlfmt == VDO_PXLFMT_ARGB4444 ||
				p_src_img->pxlfmt == VDO_PXLFMT_RAW16) {
				rot_request.format = FORMAT_16BITS;
			} else {
				rot_request.format = FORMAT_8BITS;
			}

			rot_request.p_images = &rot_img_src;

			if (E_OK != kdrv_rotation_trigger(rot_id, &rot_request, NULL, NULL)) {
				DBG_ERR("kdrv_rotation_trigger(%d) plane %d failed\r\n", rot_id, i);
			}
		}
#else
		{

			KDRV_GRPH_TRIGGER_PARAM request = {0};
			GRPH_IMG images[3];

			gximg_memset(&request, 0, sizeof(request));
			gximg_memset(images, 0, sizeof(images));
			request.command = gximg_map_to_graph_rotate_dir(rotate_dir);
			// UV packed need to use 16 bit operation
			if (i == 1 && (p_src_img->pxlfmt == VDO_PXLFMT_YUV422 || p_src_img->pxlfmt == VDO_PXLFMT_YUV420)) {
				request.format = GRPH_FORMAT_16BITS;
			} else if(p_src_img->pxlfmt == VDO_PXLFMT_ARGB1555 ||
				  p_src_img->pxlfmt == VDO_PXLFMT_ARGB4444 ||
				  p_src_img->pxlfmt == VDO_PXLFMT_RGB565 ||
					p_src_img->pxlfmt == VDO_PXLFMT_RAW16) {
				request.format = GRPH_FORMAT_16BITS;
			} else if(p_src_img->pxlfmt == VDO_PXLFMT_ARGB8888) {
				request.format = GRPH_FORMAT_32BITS;
			} else {
				request.format = GRPH_FORMAT_8BITS;
			}
			request.p_images = &(images[0]);
			request.is_skip_cache_flush = check_no_flush(usePA, no_flush);
			request.is_buf_pa = usePA;
			images[0].img_id =             GRPH_IMG_ID_A;       // source image
			images[0].dram_addr = (UINT32)p_src_img->addr[i];
			// Always rotate to 420. If source is 422, set CbCr's source line offset to dobule
			if ((i != 0) && (p_src_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR || p_src_img->pxlfmt == VDO_PXLFMT_YUV422)) {
				images[0].lineoffset =     p_src_img->loff[i] << 1;
				images[0].height =         tmp_plane_rect.h >> 1;
			} else {
				images[0].lineoffset =     p_src_img->loff[i];
				images[0].height =         tmp_plane_rect.h;
			}
			images[0].width =         tmp_plane_rect.w;
			images[0].p_next =             &(images[1]);
			images[1].img_id =             GRPH_IMG_ID_C;       // destination image
			images[1].dram_addr = (UINT32)p_dst_img->addr[i];
			images[1].lineoffset =     p_dst_img->loff[i];

			if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
				gximg_check_grph_limit(request.command, request.format, &(images[1]), 0, GRPH_IMG_ID_C)){
				DBG_ERR("limit check fail\n");
				gximg_close_grph(grph_id);
				return E_PAR;
			}
			if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
				DBG_ERR("fail to start graphic engine(%d)\n", grph_id);
				gximg_close_grph(grph_id);
				return E_PAR;
			}
		}
#endif
	}
#if USE_ROTATE
	// Close 2D graphic module
	if (E_OK != kdrv_rotation_close(KDRV_CHIP0, KDRV_GFX2D_ROTATE)) {
		DBG_ERR("kdrv_rotation_close(%d,%d) failed\r\n", KDRV_CHIP0, KDRV_GFX2D_ROTATE);
		return E_SYS;
	}
#else
	// Close 2D graphic module
	if(gximg_close_grph(grph_id) == -1){
		DBG_ERR("fail to close graphic engine(%d)\n", grph_id);
		return E_PAR;
	}
#endif

L_GxImg_RotateByEngine_END:
	// clear redundant area
	gximg_clr_rotate_redundant_data(p_src_img, p_dst_img, rotate_dir, align_width, align_height);
	DBG_MSG("[rotate]uiAlignWidth=%04d, uiAlignHeight=%04d \r\n", align_width, align_height);
	DBG_MSG("[rotate]DstImg.W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], p_dst_img->loff[1], p_dst_img->loff[2], p_dst_img->pxlfmt);
	DBG_MSG("[rotate]DstImg.Addr[0]=0x%x,Addr[1]=0x%x,Addr[2]=0x%x\r\n", p_dst_img->addr[0], p_dst_img->addr[1], p_dst_img->addr[2]);
	DBG_FUNC_END("[rotate]\r\n");
	return E_OK;
}

static ER _gximg_rotate_data_ex(PVDO_FRAME p_src_img, UINT32 dst_buff, UINT32 dst_buf_size, UINT32 rotate_dir, PVDO_FRAME p_dst_img, GXIMG_RT_ENG engine, int no_flush)
{
	BOOL       dst_width, dst_height, dst_loff;
	UINT32     out_fmt, rotate_need_buf_size;
	VDO_FRAME dst_img_buf = {0};

	DBG_FUNC_BEGIN("[rotate]\r\n");
	// RT_IMG_INFO should not be NULL
	if (p_src_img == NULL) {
		DBG_ERR("Rotate pImgBuf is NULL\r\n");
		return E_PAR;
	} else if (p_src_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("pImgBuf is not initialized\r\n");
		return E_PAR;
	} else if ((p_src_img->loff[0] % gximg_get_rotate_width_align(p_src_img->pxlfmt))) {
		DBG_ERR("In lineoff %d not %d byte aligned\r\n", p_src_img->loff[0], gximg_get_rotate_width_align(p_src_img->pxlfmt));
		return E_PAR;
	} else if ((p_src_img->size.h % gximg_get_rotate_height_align(p_src_img->pxlfmt))) {
		DBG_ERR("Height %d not %d byte aligned\r\n", p_src_img->size.h, gximg_get_rotate_height_align(p_src_img->pxlfmt));
		return E_PAR;
	}

	DBG_MSG("[rotate]p_src_img.W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], p_src_img->loff[1], p_src_img->loff[2], p_src_img->pxlfmt);
	DBG_MSG("[rotate]p_src_img.Addr[0]=0x%x,Addr[1]=0x%x,Addr[2]=0x%x\r\n", p_src_img->addr[0], p_src_img->addr[1], p_src_img->addr[2]);
	DBG_MSG("[rotate]Dir=%d,engine %d, dst_buff=0x%x, dst_buf_size=0x%x \r\n", rotate_dir, engine, dst_buff, dst_buf_size);

	// choose rotate engine
	if (rotate_dir == VDO_DIR_ROTATE_90 ||
		rotate_dir == VDO_DIR_ROTATE_270 ||
		rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_90) ||
		rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_270)) {
		dst_width = p_src_img->size.h;
		dst_height = p_src_img->size.w;
		dst_width = ALIGN_CEIL(dst_width, gximg_get_rotate_height_align(p_src_img->pxlfmt));
		dst_height = ALIGN_CEIL(dst_height, gximg_get_rotate_width_align(p_src_img->pxlfmt));
	} else {
		dst_width = p_src_img->size.w;
		dst_height = p_src_img->size.h;
		dst_width = ALIGN_CEIL(dst_width, gximg_get_rotate_width_align(p_src_img->pxlfmt));
		dst_height = ALIGN_CEIL(dst_height, gximg_get_rotate_height_align(p_src_img->pxlfmt));
	}
	out_fmt = p_src_img->pxlfmt;
	if (p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR || p_src_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR) {
		out_fmt = VDO_PXLFMT_YUV420_PLANAR;
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_YUV420 || p_src_img->pxlfmt == VDO_PXLFMT_YUV422) {
		out_fmt = VDO_PXLFMT_YUV420;
	}
	dst_loff = ALIGN_CEIL(dst_width, gximg_get_rotate_width_align(p_src_img->pxlfmt));

	rotate_need_buf_size = gximg_calc_require_size(dst_width, dst_height, out_fmt, dst_loff);
	// available size is larger than img size
	if (dst_buf_size >= rotate_need_buf_size) {
		if(gximg_init_buf(&dst_img_buf, dst_width, dst_height, out_fmt, dst_loff, dst_buff, dst_buf_size) != E_OK){
			DBG_ERR("gximg_init_buf() fail\n");
			return E_PAR;
		}
	} else {
		DBG_ERR("dstBufSize size %d <  Need Size %d\r\n", dst_buf_size, rotate_need_buf_size);
		//vos_dump_stack();
		return E_PAR;
	}
	gximg_rotate_by_engine(p_src_img, &dst_img_buf, rotate_dir, engine, no_flush, 0);
	gximg_memcpy(p_dst_img, &dst_img_buf, sizeof(dst_img_buf));
	DBG_FUNC_END("[rotate]\r\n");
	return E_OK;
}

ER gximg_rotate_data_ex(PVDO_FRAME p_src_img, UINT32 dst_buff, UINT32 dst_buf_size, UINT32 rotate_dir, PVDO_FRAME p_dst_img, GXIMG_RT_ENG engine)
{
	return  _gximg_rotate_data_ex(p_src_img, dst_buff, dst_buf_size, rotate_dir, p_dst_img, engine, 0);
}

ER gximg_rotate_data(PVDO_FRAME p_src_img, UINT32 dst_buff, UINT32 dst_buf_size, UINT32 rotate_dir, PVDO_FRAME p_dst_img)
{
	return  _gximg_rotate_data_ex(p_src_img, dst_buff, dst_buf_size, rotate_dir, p_dst_img, GXIMG_ROTATE_ENG1, 0);
}

ER gximg_rotate_data_no_flush(PVDO_FRAME p_src_img, UINT32 dst_buff, UINT32 dst_buf_size, UINT32 rotate_dir, PVDO_FRAME p_dst_img)
{
	return  _gximg_rotate_data_ex(p_src_img, dst_buff, dst_buf_size, rotate_dir, p_dst_img, GXIMG_ROTATE_ENG1, 1);
}

#if !USE_ROTATE
static ER gximg_rotate_paste_aligned(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 rotate_dir, GXIMG_RT_ENG engine, int no_flush, int usePA)
{
	VDO_FRAME src_img_buf = {0};
	VDO_FRAME dst_img_buf = {0};
	IRECT dst_region = {0};
	UINT32  align_w, align_h;
	UINT32 plane_idx;

	//a private function, no arguments check here, only check alignment
	align_w = gximg_get_rotate_width_align(p_src_img->pxlfmt);
	align_h = gximg_get_rotate_height_align(p_src_img->pxlfmt);
	if (align_w == 0 || align_h == 0) {
		DBG_ERR("Get align limitation failed, %d x %d\r\n", align_w, align_h);
		return E_PAR;
	}

	if (p_src_region->w % align_w || p_src_region->h % align_h)   {
		DBG_ERR("Invalid arg, Src W(%d) x H(%d)\r\n", p_src_region->w, p_src_region->h);
		return E_PAR;
	}

	//prepare the src image
	src_img_buf = *p_src_img;

	{
		IRECT tmp_plane_rect = {0};

		if (p_src_region->x < 0 || p_src_region->y < 0 || (p_src_region->x + p_src_region->w > (INT32)p_src_img->size.w) || p_src_region->y + p_src_region->h > (INT32)p_src_img->size.h) {
			DBG_ERR("Invalid arg, Src W(%d) x H(%d), Rgn (%d,%d,%d,%d)\r\n", p_src_img->size.w, p_src_img->size.h, p_src_region->x, p_src_region->y, p_src_region->w, p_src_region->h);
			return E_PAR;
		}

		src_img_buf.size.w = p_src_region->w;
		src_img_buf.size.h = p_src_region->h;
		for (plane_idx = 0; plane_idx < 2; plane_idx++) {
			gximg_calc_plane_rect(p_src_img, plane_idx, p_src_region, &tmp_plane_rect);
			src_img_buf.addr[plane_idx] = (UINT32)p_src_img->addr[plane_idx] + gximg_calc_plane_offset(p_src_img, plane_idx, &tmp_plane_rect);
		}
	}

	//prepare the dst image
	dst_img_buf = *p_dst_img;

	if (rotate_dir == VDO_DIR_ROTATE_90 || rotate_dir == VDO_DIR_ROTATE_270) {
		//switch W and H
		dst_region.w = src_img_buf.size.h;
		dst_region.h = src_img_buf.size.w;
	} else {
		dst_region.w = src_img_buf.size.w;
		dst_region.h = src_img_buf.size.h;
	}

	if (p_dst_location) {
		dst_region.x = p_dst_location->x;
		dst_region.y = p_dst_location->y;
	}

	{
		IRECT tmp_plane_rect = {0};

		if (dst_region.x < 0 || dst_region.y < 0 || (dst_region.x + dst_region.w) > (INT32)p_dst_img->size.w || (dst_region.y + dst_region.h) > (INT32)p_dst_img->size.h) {
			DBG_ERR("Invalid arg, Dst W(%d) x H(%d), Rgn (%d,%d,%d,%d)\r\n", p_dst_img->size.w, p_dst_img->size.h, dst_region.x, dst_region.y, dst_region.w, dst_region.h);
			return E_PAR;
		}

		dst_img_buf.size.w = dst_region.w;
		dst_img_buf.size.h = dst_region.h;
		for (plane_idx = 0; plane_idx < 2; plane_idx++) {
			gximg_calc_plane_rect(p_dst_img, plane_idx, &dst_region, &tmp_plane_rect);
			dst_img_buf.addr[plane_idx] = (UINT32)p_dst_img->addr[plane_idx] + gximg_calc_plane_offset(p_dst_img, plane_idx, &tmp_plane_rect);
		}
	}

	//do eng rotate
	DBG_IND("[rotate]src_img_buf W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", src_img_buf.size.w, src_img_buf.size.h, src_img_buf.loff[0], src_img_buf.loff[1], src_img_buf.loff[2], src_img_buf.pxlfmt);
	DBG_IND("[rotate]dst_img_buf W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", dst_img_buf.size.w, dst_img_buf.size.h, dst_img_buf.loff[0], dst_img_buf.loff[1], dst_img_buf.loff[2], dst_img_buf.pxlfmt);
	if (E_OK != gximg_rotate_by_engine(&src_img_buf, &dst_img_buf, rotate_dir, engine, no_flush, usePA)) {
		DBG_ERR("gximg_rotate_by_engine failed\r\n");
		return E_SYS;
	}

	return E_OK;
}

static ER _gximg_rotate_paste_data_with_flush(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 rotate_dir, GXIMG_RT_ENG engine, int no_flush, int usePA)
{
	UINT32 no_align_w = 0, no_align_h = 0;
	UINT32 out_fmt;
	UINT32 align_w, align_h;
	IRECT  cur_src_rgn = {0}, org_src_rgn = {0};
	IPOINT cur_dst_loc = {0};

	DBG_FUNC_BEGIN("[rotate]\r\n");
	// RT_IMG_INFO should not be NULL
	if (p_src_img == NULL) {
		DBG_ERR("p_src_img is NULL\r\n");
		return E_PAR;
	}
	if (p_dst_img == NULL) {
		DBG_ERR("p_dst_img is NULL\r\n");
		return E_PAR;
	}
	if (p_src_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_src_img is not init\r\n");
		return E_PAR;
	}
	if (p_dst_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_dst_img is not init\r\n");
		return E_PAR;
	}
	if (p_src_img->loff[0] % GRPH_ALL_LOFF_ALIGN) {
		DBG_ERR("Src lineoff %d not %d byte aligned\r\n", p_src_img->loff[0], GRPH_ALL_LOFF_ALIGN);
		return E_PAR;
	}
	if (p_dst_img->loff[0] % GRPH_ALL_LOFF_ALIGN) {
		DBG_ERR("Dst lineoff %d not %d byte aligned\r\n", p_dst_img->loff[0], GRPH_ALL_LOFF_ALIGN);
		return E_PAR;
	}

	out_fmt = p_src_img->pxlfmt;
	if (p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR || p_src_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR) {
		out_fmt = VDO_PXLFMT_YUV420_PLANAR;
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_YUV420 || p_src_img->pxlfmt == VDO_PXLFMT_YUV422) {
		out_fmt = VDO_PXLFMT_YUV420;
	}

	if (out_fmt !=  p_dst_img->pxlfmt) {
		DBG_ERR("Src PxlFmt %d Dst PxlFmt %d\r\n", p_src_img->pxlfmt, p_dst_img->pxlfmt);
		return E_PAR;
	}

	DBG_MSG("[rotate]pImg.W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], p_src_img->loff[1], p_src_img->loff[2], p_src_img->pxlfmt);
	DBG_MSG("[rotate]pImg.Addr[0]=0x%x,Addr[1]=0x%x,Addr[2]=0x%x\r\n", p_src_img->addr[0], p_src_img->addr[1], p_src_img->addr[2]);
	DBG_MSG("[rotate]Dir=%d,RotEng %d\r\n", rotate_dir, engine);

	align_w = gximg_get_rotate_width_align(p_src_img->pxlfmt);
	align_h = gximg_get_rotate_height_align(p_src_img->pxlfmt);
	if (align_w == 0 || align_h == 0) {
		DBG_ERR("Get align limitation failed, %d x %d\r\n", align_w, align_h);
		return E_PAR;
	}

	if (p_src_region != GXIMG_REGION_MATCH_IMG) {
		org_src_rgn = *p_src_region;
	} else {
		org_src_rgn.x = 0;
		org_src_rgn.y = 0;
		org_src_rgn.w = p_src_img->size.w;
		org_src_rgn.h = p_src_img->size.h;
	}

	// GOP starting address should be word alignment
	if (org_src_rgn.w % 4 || org_src_rgn.h % 4) {
		DBG_ERR("SrcRgn W(%d) or H(%d) not 4 byte aligned\r\n", org_src_rgn.w, org_src_rgn.h);
		return E_PAR;
	}

	no_align_w = org_src_rgn.w % align_w;
	no_align_h = org_src_rgn.h % align_h;

	//---------------------------
	//                   |
	//                   |
	//                   |
	//                   |
	//       Main        | Right
	//                   |
	//                   |
	//                   |
	//---------------------------
	//                   |
	//       Bottom      | Right-Bottom
	//---------------------------

	//Main region
	DBG_IND("main region\r\n");
	cur_src_rgn.x = org_src_rgn.x;
	cur_src_rgn.y = org_src_rgn.y;
	cur_src_rgn.w = org_src_rgn.w - no_align_w;
	cur_src_rgn.h = org_src_rgn.h - no_align_h;

	if (p_dst_location) {
		cur_dst_loc.x = p_dst_location->x;
		cur_dst_loc.y = p_dst_location->y;
	} else {
		cur_dst_loc.x = 0;
		cur_dst_loc.y = 0;
	}

	switch (rotate_dir) {
	case VDO_DIR_ROTATE_90:
		cur_dst_loc.x += no_align_h;
		break;
	case VDO_DIR_ROTATE_270:
		cur_dst_loc.y += no_align_w;
		break;
	case VDO_DIR_ROTATE_180:
		cur_dst_loc.x += no_align_w;
		cur_dst_loc.y += no_align_h;
		break;
	case VDO_DIR_MIRRORX:
		cur_dst_loc.x += no_align_w;
		break;
	case VDO_DIR_MIRRORY:
		cur_dst_loc.y += no_align_h;
		break;
	default:
		DBG_ERR("Invalid Rotate %d\r\n", rotate_dir);
		return E_PAR;
	}

	if (E_OK != gximg_rotate_paste_aligned(p_src_img, &cur_src_rgn, p_dst_img, &cur_dst_loc, rotate_dir, engine, no_flush, usePA)) {
		DBG_ERR("Main rgn rotate failed\r\n");
		return E_SYS;
	}

	//Right region
	if (no_align_w) {
		DBG_IND("right region\r\n");
		cur_src_rgn.x = org_src_rgn.x + org_src_rgn.w - align_w;
		cur_src_rgn.y = org_src_rgn.y;
		cur_src_rgn.w = align_w;
		cur_src_rgn.h = org_src_rgn.h - no_align_h;

		if (p_dst_location) {
			cur_dst_loc.x = p_dst_location->x;
			cur_dst_loc.y = p_dst_location->y;
		} else {
			cur_dst_loc.x = 0;
			cur_dst_loc.y = 0;
		}

		switch (rotate_dir) {
		case VDO_DIR_ROTATE_90:
			cur_dst_loc.x += no_align_h;
			cur_dst_loc.y += (org_src_rgn.w - align_w);
			break;
		case VDO_DIR_ROTATE_270:
			//no change
			break;
		case VDO_DIR_ROTATE_180:
			cur_dst_loc.y += no_align_h;
			break;
		case VDO_DIR_MIRRORX:
			//no change
			break;
		case VDO_DIR_MIRRORY:
			cur_dst_loc.x += (org_src_rgn.w - align_w);
			cur_dst_loc.y += no_align_h;
			break;
		// coverity[dead_error_begin]: Reserve default to prevent any wrong modifications in the future
		default:
			DBG_ERR("Invalid Rotate %d\r\n", rotate_dir);
			return E_PAR;
		}

		if (E_OK != gximg_rotate_paste_aligned(p_src_img, &cur_src_rgn, p_dst_img, &cur_dst_loc, rotate_dir, engine, no_flush, usePA)) {
			DBG_ERR("Right rgn rotate failed\r\n");
			return E_SYS;
		}
	}

	//Bottom region
	if (no_align_h) {
		DBG_IND("bottom region\r\n");
		cur_src_rgn.x = org_src_rgn.x;
		cur_src_rgn.y = org_src_rgn.y + org_src_rgn.h - align_h;
		cur_src_rgn.w = org_src_rgn.w - no_align_w;
		cur_src_rgn.h = align_h;

		if (p_dst_location) {
			cur_dst_loc.x = p_dst_location->x;
			cur_dst_loc.y = p_dst_location->y;
		} else {
			cur_dst_loc.x = 0;
			cur_dst_loc.y = 0;
		}

		switch (rotate_dir) {
		case VDO_DIR_ROTATE_90:
			//no change
			break;
		case VDO_DIR_ROTATE_270:
			cur_dst_loc.x += org_src_rgn.h - align_h;
			cur_dst_loc.y += no_align_w;
			break;
		case VDO_DIR_ROTATE_180:
			cur_dst_loc.x += no_align_w;
			break;
		case VDO_DIR_MIRRORX:
			cur_dst_loc.x += no_align_w;
			cur_dst_loc.y += org_src_rgn.h - align_h;
			break;
		case VDO_DIR_MIRRORY:
			//no change
			break;
		// coverity[dead_error_begin]: Reserve default to prevent any wrong modifications in the future
		default:
			DBG_ERR("Invalid Rotate %d\r\n", rotate_dir);
			return E_PAR;
		}

		if (E_OK != gximg_rotate_paste_aligned(p_src_img, &cur_src_rgn, p_dst_img, &cur_dst_loc, rotate_dir, engine, no_flush, usePA)) {
			DBG_ERR("Bottom rgn rotate failed\r\n");
			return E_SYS;
		}
	}

	//Right-Bottom region
	if (no_align_w && no_align_h) {
		DBG_IND("right-bottom region\r\n");
		cur_src_rgn.x = org_src_rgn.x + org_src_rgn.w - align_w;
		cur_src_rgn.y = org_src_rgn.y + org_src_rgn.h - align_h;
		cur_src_rgn.w = align_w;
		cur_src_rgn.h = align_h;

		if (p_dst_location) {
			cur_dst_loc.x = p_dst_location->x;
			cur_dst_loc.y = p_dst_location->y;
		} else {
			cur_dst_loc.x = 0;
			cur_dst_loc.y = 0;
		}

		switch (rotate_dir) {
		case VDO_DIR_ROTATE_90:
			cur_dst_loc.y += org_src_rgn.w - align_w;
			break;
		case VDO_DIR_ROTATE_270:
			cur_dst_loc.x += org_src_rgn.h - align_h;
			break;
		case VDO_DIR_ROTATE_180:
			//no change
			break;
		case VDO_DIR_MIRRORX:
			cur_dst_loc.y += org_src_rgn.h - align_h;
			break;
		case VDO_DIR_MIRRORY:
			cur_dst_loc.x += org_src_rgn.w - align_w;
			break;
		// coverity[dead_error_begin]: Reserve default to prevent any wrong modifications in the future
		default:
			DBG_ERR("Invalid Rotate %d\r\n", rotate_dir);
			return E_PAR;
		}

		if (E_OK != gximg_rotate_paste_aligned(p_src_img, &cur_src_rgn, p_dst_img, &cur_dst_loc, rotate_dir, engine, no_flush, usePA)) {
			DBG_ERR("Bottom rgn rotate failed\r\n");
			return E_SYS;
		}
	}

	DBG_FUNC_END("[rotate]\r\n");
	return E_OK;
}

ER gximg_rotate_paste_data(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 rotate_dir, GXIMG_RT_ENG engine, int usePA){
	return _gximg_rotate_paste_data_with_flush(p_src_img, p_src_region, p_dst_img, p_dst_location, rotate_dir, engine, 0, usePA);
}

ER gximg_rotate_paste_data_no_flush(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 rotate_dir, GXIMG_RT_ENG engine, int usePA){
	return _gximg_rotate_paste_data_with_flush(p_src_img, p_src_region, p_dst_img, p_dst_location, rotate_dir, engine, 1, usePA);
}
#else
ER gximg_rotate_paste_data(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 rotate_dir, GXIMG_RT_ENG engine)
{
	BOOL      dst_width, dst_height, is_need_HdlW_align = FALSE;//, isNeedHdlHAlign = FALSE;
	UINT32    out_fmt,  i, reamin_w = 0, plane_num = 1;
	VDO_FRAME src_img_buf = {0};
	VDO_FRAME dst_img_buf = {0};
	IPOINT    tmp_pos = {0};
	UINT32    width_align, height_align;



	DBG_FUNC_BEGIN("[rotate]\r\n");
	// RT_IMG_INFO should not be NULL
	if (p_src_img == NULL) {
		DBG_ERR("p_src_img is NULL\r\n");
		return E_PAR;
	}
	if (p_dst_img == NULL) {
		DBG_ERR("p_dst_img is NULL\r\n");
		return E_PAR;
	}
	if (p_src_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_src_img is not init\r\n");
		return E_PAR;
	}
	if (p_dst_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_dst_img is not init\r\n");
		return E_PAR;
	}
	if (p_src_img->loff[0] % GRPH_ALL_LOFF_ALIGN) {
		DBG_ERR("Src lineoff %d not %d byte aligned\r\n", p_src_img->loff[0], GRPH_ALL_LOFF_ALIGN);
		return E_PAR;
	}
	if (p_dst_img->loff[0] % GRPH_ALL_LOFF_ALIGN) {
		DBG_ERR("Dst lineoff %d not %d byte aligned\r\n", p_dst_img->loff[0], GRPH_ALL_LOFF_ALIGN);
		return E_PAR;
	}
	out_fmt = p_src_img->pxlfmt;
	if (p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR || p_src_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR) {
		out_fmt = VDO_PXLFMT_YUV420_PLANAR;
		plane_num = 2;
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_YUV420 || p_src_img->pxlfmt == VDO_PXLFMT_YUV422) {
		out_fmt = VDO_PXLFMT_YUV420;
		plane_num = 2;
	}
	if (out_fmt !=  p_dst_img->pxlfmt) {
		DBG_ERR("Src PxlFmt %d Dst PxlFmt %d\r\n", p_src_img->pxlfmt, p_dst_img->pxlfmt);
		return E_PAR;
	}

	DBG_MSG("[rotate]pImg.W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], p_src_img->loff[1], p_src_img->loff[2], p_src_img->pxlfmt);
	DBG_MSG("[rotate]pImg.Addr[0]=0x%x,Addr[1]=0x%x,Addr[2]=0x%x\r\n", p_src_img->addr[0], p_src_img->addr[1], p_src_img->addr[2]);
	DBG_MSG("[rotate]Dir=%d,RotEng %d\r\n", rotate_dir, engine);
	src_img_buf = *p_src_img;
	if (p_src_region != GXIMG_REGION_MATCH_IMG) {
		IRECT            tmp_plane_rect;

		if (p_src_region->x < 0 || p_src_region->y < 0 || (p_src_region->x + p_src_region->w > (INT32)p_src_img->size.w) || p_src_region->y + p_src_region->h > (INT32)p_src_img->size.h) {
			DBG_ERR("Src Img W %d H %d, Rgn (%d,%d,%d,%d)\r\n", p_src_img->size.w, p_src_img->size.h, p_src_region->x, p_src_region->y, p_src_region->w, p_src_region->h);
			return E_PAR;
		}
		src_img_buf.size.w = p_src_region->w;
		src_img_buf.size.h = p_src_region->h;
		for (i = 0; i < plane_num; i++) {
			gximg_calc_plane_rect(p_src_img, i, p_src_region, &tmp_plane_rect);
			src_img_buf.addr[i] = (UINT32)p_src_img->addr[i] + gximg_calc_plane_offset(p_src_img, i, &tmp_plane_rect);
		}
	}
	// GOP starting address should be word alignment
	if (src_img_buf.size.w % 4) {
		DBG_ERR("Src W %d not 4 byte aligned\r\n", src_img_buf.size.w);
		return E_PAR;
	}
	width_align = gximg_get_rotate_width_align(p_src_img->pxlfmt);
	height_align = gximg_get_rotate_height_align(p_src_img->pxlfmt);
	if (width_align == 0 ||  height_align == 0) {
		return E_PAR;
	}

	if (src_img_buf.size.w % width_align) {
		//DBG_ERR("Src W %d not %d byte aligned\r\n",src_img_buf.size.w,gximg_get_rotate_width_align(p_src_img->pxlfmt));
		//return E_PAR;
		if (src_img_buf.size.w < width_align) {
			DBG_ERR("Src W %d < %d\r\n", src_img_buf.size.w, width_align);
			return E_PAR;
		}
		is_need_HdlW_align = TRUE;
		reamin_w = src_img_buf.size.w % width_align;
		src_img_buf.size.w -= reamin_w;
		DBG_MSG("[rotate] reamin_w = %d, src_img_buf.size.w=%d\r\n", reamin_w, src_img_buf.size.w);

	}
	if (src_img_buf.size.h % height_align) {
		DBG_ERR("Src H %d not %d byte aligned\r\n", src_img_buf.size.h, height_align);
		return E_PAR;
		//is_need_HdlH_align = TRUE;

	}
	dst_img_buf = *p_dst_img;
	// choose rotate engine
	if (rotate_dir == VDO_DIR_ROTATE_90 ||
		rotate_dir == VDO_DIR_ROTATE_270 ||
		rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_90) ||
		rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_270)) {
		dst_width  = src_img_buf.size.h;
		dst_height = src_img_buf.size.w;
		dst_width  = ALIGN_FLOOR(dst_width, height_align);
		dst_height = ALIGN_FLOOR(dst_height, width_align);
	} else {
		dst_width  = src_img_buf.size.w;
		dst_height = src_img_buf.size.h;
		dst_width  = ALIGN_FLOOR(dst_width, width_align);
		dst_height = ALIGN_FLOOR(dst_height, height_align);
	}
	if (p_dst_location) {
		tmp_pos.x = p_dst_location->x;
		tmp_pos.y = p_dst_location->y;
		if (tmp_pos.x < 0 || tmp_pos.y < 0 || (tmp_pos.x + dst_width > (INT32)p_dst_img->size.w) || tmp_pos.y + dst_height > (INT32)p_dst_img->size.h) {
			DBG_ERR("Rotate W %d, H %d, Dst Img W %d H %d, Pos (%d,%d)\r\n", dst_width, dst_height, p_dst_img->size.w, p_dst_img->size.h, tmp_pos.x, tmp_pos.y);
			return E_PAR;
		}
	} else {
		tmp_pos.x = 0;
		tmp_pos.y = 0;
	}
	// cacluate dst_img_buf
	{
		IRECT            tmp_plane_rect;
		IRECT            dst_rect;

		if (rotate_dir == VDO_DIR_ROTATE_180 || rotate_dir == VDO_DIR_MIRRORX) {
			dst_rect.x  = tmp_pos.x + reamin_w;
		} else {
			dst_rect.x  = tmp_pos.x;
		}

		if (rotate_dir == VDO_DIR_ROTATE_270 || rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_90)) {
			dst_rect.y  = tmp_pos.y + reamin_w;
		} else {
			dst_rect.y  = tmp_pos.y;
		}
		dst_rect.w  = dst_width;
		dst_rect.h =  dst_height;
		dst_img_buf.size.w = dst_width;
		dst_img_buf.size.h = dst_height;
		for (i = 0; i < plane_num; i++) {
			gximg_calc_plane_rect(p_dst_img, i, &dst_rect, &tmp_plane_rect);
			dst_img_buf.addr[i] = (UINT32)p_dst_img->addr[i] + gximg_calc_plane_offset(p_dst_img, i, &tmp_plane_rect);
		}
	}
	DBG_MSG("[rotate]src_img_buf W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", src_img_buf.size.w, src_img_buf.size.h, src_img_buf.loff[0], src_img_buf.loff[1], src_img_buf.loff[2], src_img_buf.pxlfmt);
	DBG_MSG("[rotate]dst_img_buf W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", dst_img_buf.size.w, dst_img_buf.size.h, dst_img_buf.loff[0], dst_img_buf.loff[1], dst_img_buf.loff[2], dst_img_buf.pxlfmt);
	gximg_rotate_by_engine(&src_img_buf, &dst_img_buf, rotate_dir, engine);

	if (is_need_HdlW_align) {
		VDO_FRAME src_img_buf2 = {0};
		VDO_FRAME dst_img_buf2 = {0};
		IRECT            tmp_plane_rect;
		IRECT            tmp_rect;

		// src
		src_img_buf2 = src_img_buf;
		tmp_rect.x  = src_img_buf.size.w - (width_align - reamin_w);
		tmp_rect.y  = 0;
		tmp_rect.w  = width_align;
		tmp_rect.h =  src_img_buf.size.h;
		DBG_MSG("[rotate]src tmp_rect x=%d,y=%d,w=%d,h=%d\r\n", tmp_rect.x, tmp_rect.y, tmp_rect.w, tmp_rect.h);
		for (i = 0; i < plane_num; i++) {
			gximg_calc_plane_rect(&src_img_buf, i, &tmp_rect, &tmp_plane_rect);
			src_img_buf2.addr[i] = (UINT32)src_img_buf.addr[i] + gximg_calc_plane_offset(&src_img_buf, i, &tmp_plane_rect);
		}
		src_img_buf2.size.w = width_align;

		// dst
		dst_img_buf2 = dst_img_buf;

		if (rotate_dir == VDO_DIR_ROTATE_90) {
			tmp_rect.x  = tmp_pos.x;
			tmp_rect.y  = tmp_pos.y + dst_img_buf.size.h - (width_align - reamin_w);
			tmp_rect.w  = dst_img_buf.size.w;
			tmp_rect.h =  width_align;
		} else if (rotate_dir == VDO_DIR_ROTATE_180 || rotate_dir == VDO_DIR_MIRRORX) {
			tmp_rect.x  = tmp_pos.x;
			tmp_rect.y  = tmp_pos.y;
			tmp_rect.w  = width_align;
			tmp_rect.h =  dst_img_buf.size.h;
		} else if (rotate_dir == VDO_DIR_ROTATE_270) {
			tmp_rect.x  = tmp_pos.x;
			tmp_rect.y  = tmp_pos.y;
			tmp_rect.w  = dst_img_buf.size.w;
			tmp_rect.h =  width_align;
		} else if (rotate_dir == VDO_DIR_MIRRORY) {
			tmp_rect.x  = tmp_pos.x + dst_img_buf.size.w - (width_align - reamin_w);
			tmp_rect.y  = tmp_pos.y;
			tmp_rect.w  = width_align;
			tmp_rect.h =  dst_img_buf.size.h;
		}
		DBG_MSG("[rotate]dst tmp_rect x=%d,y=%d,w=%d,h=%d\r\n", tmp_rect.x, tmp_rect.y, tmp_rect.w, tmp_rect.h);
		for (i = 0; i < plane_num; i++) {
			gximg_calc_plane_rect(p_dst_img, i, &tmp_rect, &tmp_plane_rect);
			dst_img_buf2.addr[i] = (UINT32)p_dst_img->addr[i] + gximg_calc_plane_offset(p_dst_img, i, &tmp_plane_rect);
		}
		dst_img_buf2.size.w = tmp_rect.w;
		dst_img_buf2.size.h = tmp_rect.h;
		DBG_MSG("[rotate]src_img_buf2 W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", src_img_buf2.size.w, src_img_buf2.size.h, src_img_buf2.loff[0], src_img_buf2.loff[1], src_img_buf2.loff[2], src_img_buf2.pxlfmt);
		DBG_MSG("[rotate]dst_img_buf2 W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", dst_img_buf2.size.w, dst_img_buf2.size.h, dst_img_buf2.loff[0], dst_img_buf2.loff[1], dst_img_buf2.loff[2], dst_img_buf2.pxlfmt);
		gximg_rotate_by_engine(&src_img_buf2, &dst_img_buf2, rotate_dir, engine);
	}
	DBG_FUNC_END("[rotate]\r\n");
	return E_OK;
}
#endif

static ER gximg_self_rotate_by_engine(PVDO_FRAME p_src_img, PVDO_FRAME p_dst_img, UINT32 tmp_buff, UINT32 rotate_dir)
{
	UINT32         align_width, align_height;
	UINT32         plane_num, copy_offset = 0;
	INT32          i;
	IRECT          tmp_rect = {0}, tmp_plane_rect = {0};
	IRECT          tmp_align_rect = {0}, tmp_align_plane_rect = {0};
	UINT32         p_buf_offset[GXIMG_MAX_PLANE_NUM];
#if USE_ROTATE
	UINT32         rotate_id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_ROTATE, 0);
#endif
	UINT32         grph_id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0);

	if (rotate_dir == VDO_DIR_ROTATE_90 ||
		rotate_dir == VDO_DIR_ROTATE_270 ||
		rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_90) ||
		rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_270)) {
		align_width = p_dst_img->size.h;
		align_height = p_dst_img->size.w;
	} else {
		align_width = p_dst_img->size.w;
		align_height = p_dst_img->size.h;
	}
	if (p_src_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR || p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR) {
		plane_num = 3;

		p_buf_offset[2] = 0;
		p_buf_offset[1] = p_dst_img->loff[1] * (ALIGN_CEIL(p_dst_img->size.h, gximg_get_rotate_height_align(p_src_img->pxlfmt)) >> 1);
		p_buf_offset[0] = 0;
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_YUV422 || p_src_img->pxlfmt == VDO_PXLFMT_YUV420) {
		plane_num = 2;
		p_buf_offset[2] = 0;
		p_buf_offset[1] = 0;
		p_buf_offset[0] = 0;
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_Y8) {
		plane_num = 1;
		p_buf_offset[2] = 0;
		p_buf_offset[1] = 0;
		p_buf_offset[0] = 0;
	} else {
		DBG_ERR("pixel format %x\r\n", p_src_img->pxlfmt);
		return E_PAR;
	}

#if USE_ROTATE
	// Open 2D graphic module
	if (E_OK != kdrv_rotation_open(KDRV_CHIP0, KDRV_GFX2D_ROTATE)) {
		DBG_ERR("kdrv_rotation_open(%d,%d) failed\r\n", KDRV_CHIP0, KDRV_GFX2D_ROTATE);
		return E_SYS;
	}
#endif

	if(gximg_open_grph(grph_id) != E_OK){
		DBG_ERR("fail to open graphic engine(%d)\n", KDRV_GFX2D_GRPH0);
		return E_PAR;
	}

	for (i = plane_num - 1; i >= 0; i--) {
		tmp_align_rect.w = align_width;
		tmp_align_rect.h = align_height;
		gximg_calc_plane_rect(p_src_img, i, &tmp_align_rect, &tmp_align_plane_rect);

#if USE_ROTATE
		{
			KDRV_ROTATION_TRIGGER_PARAM rot_request = {0};
			ROTATION_IMG rot_img_src = {0}, rot_img_dst = {0};

			gximg_memset(&rot_request, 0, sizeof(rot_request));
			gximg_memset(&rot_img_src, 0, sizeof(rot_img_src));
			gximg_memset(&rot_img_dst, 0, sizeof(rot_img_dst));

			//set source image
			rot_img_src.img_id = ROTATION_IMG_ID_SRC;
			rot_img_src.address = (UINT32)p_src_img->addr[i];
			// Always rotate to 420. If source is 422, set CbCr's source line offset to dobule
			if ((i != 0) && (p_src_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR || p_src_img->pxlfmt == VDO_PXLFMT_YUV422)) {
				rot_img_src.lineoffset =     p_src_img->loff[i] << 1;
				rot_img_src.height =         tmp_align_plane_rect.h >> 1;
			} else {
				rot_img_src.lineoffset =     p_src_img->loff[i];
				rot_img_src.height =         tmp_align_plane_rect.h;
			}
			rot_img_src.width = tmp_align_plane_rect.w;
			rot_img_src.p_next = &rot_img_dst;

			//set destination image
			rot_img_dst.img_id = ROTATION_IMG_ID_DST;
			rot_img_dst.address = (UINT32)tmp_buff + p_buf_offset[i];
			rot_img_dst.lineoffset = p_dst_img->loff[i];

			//set rotation request
			rot_request.command = gximg_map_to_graph_rotate_dir(rotate_dir);
			// UV packed need to use 16 bit operation
			if (i == 1 && (p_src_img->pxlfmt == VDO_PXLFMT_YUV422 || p_src_img->pxlfmt == VDO_PXLFMT_YUV420)) {
				rot_request.format = FORMAT_16BITS;
			} else {
				rot_request.format = FORMAT_8BITS;
			}

			rot_request.p_images = &rot_img_src;

			//trigger rotation engine
			if (E_OK != kdrv_rotation_trigger(rotate_id, &rot_request, NULL, NULL)) {
				DBG_ERR("kdrv_rotation_trigger %d plane %d failed\r\n", rotate_id, i);
			}
		}
#else
		{
			KDRV_GRPH_TRIGGER_PARAM request = {0};
			GRPH_IMG images[3];

			gximg_memset(&request, 0, sizeof(request));
			gximg_memset(images, 0, sizeof(images));
			request.command = gximg_map_to_graph_rotate_dir(rotate_dir);
			// UV packed need to use 16 bit operation
			if (i == 1 && (p_src_img->pxlfmt == VDO_PXLFMT_YUV422 || p_src_img->pxlfmt == VDO_PXLFMT_YUV420)) {
				request.format = GRPH_FORMAT_16BITS;
			} else {
				request.format = GRPH_FORMAT_8BITS;
			}
			request.p_images = &(images[0]);
			images[0].img_id =             GRPH_IMG_ID_A;       // source image
			images[0].dram_addr = (UINT32)p_src_img->addr[i];
			// Always rotate to 420. If source is 422, set CbCr's source line offset to dobule
			if ((i != 0) && (p_src_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR || p_src_img->pxlfmt == VDO_PXLFMT_YUV422)) {
				images[0].lineoffset =     p_src_img->loff[i] << 1;
				images[0].height =         tmp_align_plane_rect.h >> 1;
			} else {
				images[0].lineoffset =      p_src_img->loff[i];
				images[0].height =         tmp_align_plane_rect.h;
			}
			images[0].width =         tmp_align_plane_rect.w;
			images[0].p_next =             &(images[1]);
			images[1].img_id =             GRPH_IMG_ID_C;       // destination image
			images[1].dram_addr = (UINT32)tmp_buff + p_buf_offset[i];
			images[1].lineoffset =     p_dst_img->loff[i];

			if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
				gximg_check_grph_limit(request.command, request.format, &(images[1]), 0, GRPH_IMG_ID_C)){
				DBG_ERR("limit check fail\n");
				gximg_close_grph(grph_id);
				return E_PAR;
			}
			if(kdrv_grph_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0), &request, NULL, NULL) != E_OK){
				DBG_ERR("fail to start graphic engine(%d)\n", KDRV_GFX2D_GRPH0);
				gximg_close_grph(grph_id);
				return E_PAR;
			}
		}
#endif

		//original redundant data graph
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//                      XX
		//XXXXXXXXXXXXXXXXXXXXXXXX
		//XXXXXXXXXXXXXXXXXXXXXXXX

		if (rotate_dir == VDO_DIR_ROTATE_90) {
			// clear redundant data graph
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XXXXXXXXXXXXXXXXXXXXXXXX
			//XXXXXXXXXXXXXXXXXXXXXXXX

			p_dst_img->size.w = p_src_img->size.h;
			p_dst_img->size.h = p_src_img->size.w;

			tmp_rect.x = ALIGN_CEIL(align_height - p_src_img->size.h, 2);
			tmp_rect.y = 0;
			tmp_rect.w = p_dst_img->size.w;
			tmp_rect.h = p_dst_img->size.h;

			gximg_calc_plane_rect(p_dst_img, i, &tmp_rect, &tmp_plane_rect);
			copy_offset = gximg_calc_plane_offset(p_dst_img, i, &tmp_plane_rect);

		} else if (rotate_dir == VDO_DIR_ROTATE_270) {

			// clear redundant data graph
			//XXXXXXXXXXXXXXXXXXXXXXXX
			//XXXXXXXXXXXXXXXXXXXXXXXX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX

			p_dst_img->size.w = p_src_img->size.h;
			p_dst_img->size.h = p_src_img->size.w;

			tmp_rect.x = 0;
			tmp_rect.y = ALIGN_CEIL(align_width - p_src_img->size.w, 2);
			tmp_rect.w = p_dst_img->size.w;
			tmp_rect.h = p_dst_img->size.h;

			gximg_calc_plane_rect(p_dst_img, i, &tmp_rect, &tmp_plane_rect);
			copy_offset = gximg_calc_plane_offset(p_dst_img, i, &tmp_plane_rect);


		} else if (rotate_dir == VDO_DIR_ROTATE_180) {
			// clear redundant data graph
			//XXXXXXXXXXXXXXXXXXXXXXXX
			//XXXXXXXXXXXXXXXXXXXXXXXX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX

			p_dst_img->size.w = p_src_img->size.w;
			p_dst_img->size.h = p_src_img->size.h;
			tmp_rect.x = ALIGN_CEIL(align_width - p_src_img->size.w, 2);
			tmp_rect.y = ALIGN_CEIL(align_height - p_src_img->size.h, 2);
			tmp_rect.w = p_dst_img->size.w;
			tmp_rect.h = p_dst_img->size.h;

			gximg_calc_plane_rect(p_dst_img, i, &tmp_rect, &tmp_plane_rect);
			copy_offset = gximg_calc_plane_offset(p_dst_img, i, &tmp_plane_rect);
		} else if (rotate_dir == VDO_DIR_MIRRORX) {
			// clear redundant data graph
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XXXXXXXXXXXXXXXXXXXXXXXX
			//XXXXXXXXXXXXXXXXXXXXXXXX
			p_dst_img->size.w = p_src_img->size.w;
			p_dst_img->size.h = p_src_img->size.h;
			tmp_rect.x = ALIGN_CEIL(align_width - p_src_img->size.w, 2);
			tmp_rect.y = 0;
			tmp_rect.w = p_dst_img->size.w;
			tmp_rect.h = p_dst_img->size.h;

			gximg_calc_plane_rect(p_dst_img, i, &tmp_rect, &tmp_plane_rect);
			copy_offset = gximg_calc_plane_offset(p_dst_img, i, &tmp_plane_rect);
		} else if (rotate_dir == VDO_DIR_MIRRORY) {
			// clear redundant data graph
			//XXXXXXXXXXXXXXXXXXXXXXXX
			//XXXXXXXXXXXXXXXXXXXXXXXX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX

			p_dst_img->size.w = p_src_img->size.w;
			p_dst_img->size.h = p_src_img->size.h;
			tmp_rect.x = 0;
			tmp_rect.y = ALIGN_CEIL(align_height - p_src_img->size.h, 2);
			tmp_rect.w = p_dst_img->size.w;
			tmp_rect.h = p_dst_img->size.h;

			gximg_calc_plane_rect(p_dst_img, i, &tmp_rect, &tmp_plane_rect);
			copy_offset = gximg_calc_plane_offset(p_dst_img, i, &tmp_plane_rect);

		} else if (rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_90)) {
			// clear redundant data graph
			//XXXXXXXXXXXXXXXXXXXXXXXX
			//XXXXXXXXXXXXXXXXXXXXXXXX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX
			//XX

			p_dst_img->size.w = p_src_img->size.h;
			p_dst_img->size.h = p_src_img->size.w;
			tmp_rect.x = ALIGN_CEIL(align_height - p_src_img->size.h, 2);
			tmp_rect.y = ALIGN_CEIL(align_width - p_src_img->size.w, 2);
			tmp_rect.w = p_dst_img->size.w;
			tmp_rect.h = p_dst_img->size.h;

			gximg_calc_plane_rect(p_dst_img, i, &tmp_rect, &tmp_plane_rect);
			copy_offset = gximg_calc_plane_offset(p_dst_img, i, &tmp_plane_rect);
		} else if (rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_270)) {
			//the same as the original redundant data graph
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//                      XX
			//XXXXXXXXXXXXXXXXXXXXXXXX
			//XXXXXXXXXXXXXXXXXXXXXXXX

			p_dst_img->size.w = p_src_img->size.h;
			p_dst_img->size.h = p_src_img->size.w;
			tmp_rect.x = 0;
			tmp_rect.y = 0;
			tmp_rect.w = p_dst_img->size.w;
			tmp_rect.h = p_dst_img->size.h;

			gximg_calc_plane_rect(p_dst_img, i, &tmp_rect, &tmp_plane_rect);
			copy_offset = gximg_calc_plane_offset(p_dst_img, i, &tmp_plane_rect);
		} else {
			DBG_WRN("RotateDir = %d\r\n", rotate_dir);
			p_dst_img->size.w = p_src_img->size.w;
			p_dst_img->size.h = p_src_img->size.h;
		}

		if ((p_dst_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR) && i == 2) { // this is palnner case
			// do nothing
			// Planar format should wait Cb, Cr rotate to tmp buffer , then we can copy from tmp buffer to dst buff or it will overwrite original image

		} else if ((p_dst_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR) && i == 1) { // this is palnner case
			// copy U from tmp buffer to dst buff
			tmp_rect.x = 0;
			tmp_rect.y = 0;
			tmp_rect.w = p_dst_img->size.w;
			tmp_rect.h = p_dst_img->size.h;
			gximg_calc_plane_rect(p_dst_img, 1, &tmp_rect, &tmp_plane_rect);
			{
				KDRV_GRPH_TRIGGER_PARAM request = {0};
				GRPH_IMG images[3];
				gximg_memset(images, 0, sizeof(images));

				request.command = GRPH_CMD_A_COPY;
				request.format = GRPH_FORMAT_8BITS;
				request.p_images = &(images[0]);
				images[0].img_id =          GRPH_IMG_ID_A;       // source image
				images[0].dram_addr = (UINT32)tmp_buff + p_buf_offset[1] + copy_offset;
				images[0].lineoffset =   p_dst_img->loff[1];
				images[0].width =        tmp_plane_rect.w;
				images[0].height =       tmp_plane_rect.h;
				images[0].p_next =          &(images[1]);
				images[1].img_id =          GRPH_IMG_ID_C;       // destination image
				images[1].dram_addr = (UINT32)p_dst_img->addr[1];
				images[1].lineoffset =   p_dst_img->loff[1];
				images[1].width =        tmp_plane_rect.w;
				images[1].height =       tmp_plane_rect.h;

				if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
					gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_C)){
					DBG_ERR("limit check fail\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
				if(kdrv_grph_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0), &request, NULL, NULL) != E_OK){
					DBG_ERR("fail to start graphic engine(%d)\n", KDRV_GFX2D_GRPH0);
					gximg_close_grph(grph_id);
					return E_PAR;
				}
			}

			// copy V from tmp buffer to dst buff
			tmp_rect.x = 0;
			tmp_rect.y = 0;
			tmp_rect.w = p_dst_img->size.w;
			tmp_rect.h = p_dst_img->size.h;
			gximg_calc_plane_rect(p_dst_img, 2, &tmp_rect, &tmp_plane_rect);
			{
				KDRV_GRPH_TRIGGER_PARAM request = {0};
				GRPH_IMG images[3];
				gximg_memset(images, 0, sizeof(images));

				request.command = GRPH_CMD_A_COPY;
				request.format = GRPH_FORMAT_8BITS;
				request.p_images = &(images[0]);
				images[0].img_id =          GRPH_IMG_ID_A;       // source image
				images[0].dram_addr = (UINT32)tmp_buff + p_buf_offset[2] + copy_offset;
				images[0].lineoffset =   p_dst_img->loff[2];
				images[0].width =        tmp_plane_rect.w;
				images[0].height =       tmp_plane_rect.h;
				images[0].p_next =          &(images[1]);
				images[1].img_id =          GRPH_IMG_ID_C;       // destination image
				images[1].dram_addr = (UINT32)p_dst_img->addr[2];
				images[1].lineoffset =   p_dst_img->loff[2];
				images[1].width =        tmp_plane_rect.w;
				images[1].height =       tmp_plane_rect.h;

				if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
					gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_C)){
					DBG_ERR("limit check fail\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
				if(kdrv_grph_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0), &request, NULL, NULL) != E_OK){
					DBG_ERR("fail to start graphic engine(%d)\n", KDRV_GFX2D_GRPH0);
					gximg_close_grph(grph_id);
					return E_PAR;
				}
			}
		} else {
			// copy from tmp buffer to dst buff
			tmp_rect.x = 0;
			tmp_rect.y = 0;
			tmp_rect.w = p_dst_img->size.w;
			tmp_rect.h = p_dst_img->size.h;
			gximg_calc_plane_rect(p_dst_img, i, &tmp_rect, &tmp_plane_rect);
			{
				KDRV_GRPH_TRIGGER_PARAM request = {0};
				GRPH_IMG images[3];
				gximg_memset(images, 0, sizeof(images));

				request.command = GRPH_CMD_A_COPY;
				request.format = GRPH_FORMAT_8BITS;
				request.p_images = &(images[0]);
				images[0].img_id =          GRPH_IMG_ID_A;       // source image
				images[0].dram_addr = (UINT32)tmp_buff + p_buf_offset[i] + copy_offset;
				images[0].lineoffset =   p_dst_img->loff[i];
				images[0].width =        tmp_plane_rect.w;
				images[0].height =       tmp_plane_rect.h;
				images[0].p_next =          &(images[1]);
				images[1].img_id =          GRPH_IMG_ID_C;       // destination image
				images[1].dram_addr = (UINT32)p_dst_img->addr[i];
				images[1].lineoffset =   p_dst_img->loff[i];
				images[1].width =        tmp_plane_rect.w;
				images[1].height =       tmp_plane_rect.h;

				if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
					gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_C)){
					DBG_ERR("limit check fail\n");
					gximg_close_grph(grph_id);
					return E_PAR;
				}
				if(kdrv_grph_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0), &request, NULL, NULL) != E_OK){
					DBG_ERR("fail to start graphic engine(%d)\n", KDRV_GFX2D_GRPH0);
					gximg_close_grph(grph_id);
					return E_PAR;
				}
			}
		}

	}
	DBG_IND("[rotate]By Grph DstImg.W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], p_dst_img->loff[1], p_dst_img->loff[2], p_dst_img->pxlfmt);
	DBG_IND("[rotate]DstImg.Addr[0]=0x%x,Addr[1]=0x%x,Addr[2]=0x%x\r\n", p_dst_img->addr[0], p_dst_img->addr[1], p_dst_img->addr[2]);
	// Close 2D graphic module
	if(gximg_close_grph(grph_id) != E_OK){
		DBG_ERR("fail to close graphic engine(%d)\n", KDRV_GFX2D_GRPH0);
		return E_PAR;
	}

#if USE_ROTATE
	if (E_OK != kdrv_rotation_close(KDRV_CHIP0, KDRV_GFX2D_ROTATE)) {
		DBG_ERR("kdrv_rotation_close(%d,%d) failed\r\n", KDRV_CHIP0, KDRV_GFX2D_ROTATE);
		return E_SYS;
	}
#endif
	return E_OK;
}


ER gximg_self_rotate(PVDO_FRAME p_src_img, UINT32 tmp_buff, UINT32 tmp_buf_size, UINT32 rotate_dir)
{
	BOOL    /*is_use_grph = FALSE,*/ is_continue_buff = FALSE;
	BOOL    dst_width, dst_height, dst_loff;
	UINT32  out_fmt, rotate_need_buf_size, in_img_size, out_img_size, in_y_img_size, out_y_img_size;
	UINT32  src_start_addr = 0, src_end_addr = 0, new_tmp_buff;
	VDO_FRAME        dst_img_buf = {0};
	PVDO_FRAME       p_dst_img = &dst_img_buf;
	ER               ret;

	DBG_FUNC_BEGIN("[rotate]\r\n");
	// RT_IMG_INFO should not be NULL
	if (p_src_img == NULL) {
		DBG_ERR("p_src_img is NULL\r\n");
		return E_PAR;
	} else if (p_src_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_src_img is not initialized\r\n");
		return E_PAR;
	} else if ((p_src_img->loff[0] % gximg_get_rotate_width_align(p_src_img->pxlfmt))) {
		DBG_ERR("In lineoff %d not %d byte aligned\r\n", p_src_img->loff[0], gximg_get_rotate_width_align(p_src_img->pxlfmt));
		return E_PAR;
	} else if ((p_src_img->size.h % gximg_get_rotate_height_align(p_src_img->pxlfmt))) {
		DBG_ERR("Height %d not %d byte aligned\r\n", p_src_img->size.h, gximg_get_rotate_height_align(p_src_img->pxlfmt));
		return E_PAR;
	}
	DBG_MSG("[rotate]pImg.W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], p_src_img->loff[1], p_src_img->loff[2], p_src_img->pxlfmt);
	DBG_MSG("[rotate]pImg.Addr[0]=0x%x,Addr[1]=0x%x,Addr[2]=0x%x\r\n", p_src_img->addr[0], p_src_img->addr[1], p_src_img->addr[2]);
	DBG_MSG("[rotate]Dir=%d,tmpBuff=0x%x, tmpBufSize=0x%x \r\n", rotate_dir, tmp_buff, tmp_buf_size);

	if (rotate_dir == VDO_DIR_ROTATE_90 ||
		rotate_dir == VDO_DIR_ROTATE_270 ||
		rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_90) ||
		rotate_dir == (VDO_DIR_MIRRORX | VDO_DIR_ROTATE_270)) {
		dst_width = p_src_img->size.h;
		dst_height = p_src_img->size.w;
		dst_width = ALIGN_CEIL(dst_width, gximg_get_rotate_height_align(p_src_img->pxlfmt));
		dst_height = ALIGN_CEIL(dst_height, gximg_get_rotate_width_align(p_src_img->pxlfmt));
	} else {
		dst_width = p_src_img->size.w;
		dst_height = p_src_img->size.h;
		dst_width = ALIGN_CEIL(dst_width, gximg_get_rotate_width_align(p_src_img->pxlfmt));
		dst_height = ALIGN_CEIL(dst_height, gximg_get_rotate_height_align(p_src_img->pxlfmt));
	}

	out_fmt = p_src_img->pxlfmt;
	if (p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR || p_src_img->pxlfmt == VDO_PXLFMT_YUV422_PLANAR) {
		out_fmt = VDO_PXLFMT_YUV420_PLANAR;
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_YUV420 || p_src_img->pxlfmt == VDO_PXLFMT_YUV422) {
		out_fmt = VDO_PXLFMT_YUV420;
	}
	dst_loff = ALIGN_CEIL(dst_width, gximg_get_rotate_width_align(p_src_img->pxlfmt));


	gximg_get_buf_addr(p_src_img, &src_start_addr, &src_end_addr);
	in_y_img_size =  p_src_img->loff[0] * p_src_img->size.h;
	out_y_img_size = dst_loff * dst_height;
	in_img_size = src_end_addr - src_start_addr;
	out_img_size = gximg_calc_require_size(dst_width, dst_height, out_fmt, dst_loff);

	if (src_end_addr == tmp_buff) {
		is_continue_buff = TRUE;
	}
	new_tmp_buff = tmp_buff;

	rotate_need_buf_size = dst_loff * dst_height;
	// available size is larger than img size
	if (tmp_buf_size < rotate_need_buf_size) {
		DBG_ERR("tmp_buf_size size %d <  Need Size %d\r\n", tmp_buf_size, rotate_need_buf_size);
		return E_PAR;
	} else if (out_img_size > in_img_size) {
		// because out image size > in image size, so we need to use continuout buff to put rotated image & recaculated tmpbuffer size
		if (is_continue_buff) {
			if (tmp_buf_size - out_img_size + in_img_size < rotate_need_buf_size) {
				DBG_ERR("tmp_buf_size size %d <  Need Size %d\r\n", out_img_size - in_img_size + rotate_need_buf_size, rotate_need_buf_size);
				return E_PAR;
			}
			new_tmp_buff = tmp_buff + out_img_size - in_img_size;
		} else {
			DBG_ERR("out_img_size size %d >  in_img_size %d, but buffer is not continue\r\n", out_img_size, in_img_size);
			return E_PAR;
		}
	}
	if (in_y_img_size < out_y_img_size) {
		if(gximg_init_buf(p_dst_img, dst_width, dst_height, out_fmt, dst_loff, p_src_img->addr[0], out_img_size) != E_OK){
			DBG_ERR("gximg_init_buf() fail\n");
			return E_PAR;
		}
	} else { // inYImgSize > outYImgSize
		// output is 420
		UINT32  loff[3], addr[3];
		loff[0] = dst_loff;
		loff[1] = dst_loff;
		loff[2] = dst_loff;
		addr[0] = p_src_img->addr[0];
		addr[1] = p_src_img->addr[1];
		addr[2] = p_src_img->addr[2];
		ret = gximg_init_buf_ex(p_dst_img, dst_width, dst_height, out_fmt, loff, addr);
		if(ret != E_OK){
			DBG_ERR("fail to init dst buffer\n");
			return ret;
		}
	}
	DBG_MSG("[rotate]is_continue_buff=%d,new_tmp_buff=0x%x, in_img_size=0x%x, out_img_size=0x%x \r\n", is_continue_buff, new_tmp_buff, in_img_size, out_img_size);
	gximg_self_rotate_by_engine(p_src_img, p_dst_img, new_tmp_buff, rotate_dir);
	*p_src_img = *p_dst_img;
	DBG_FUNC_END("[rotate]\r\n");
	return E_OK;
}
static void GXIMG_SCALE_POINT(PVDO_FRAME p_src_img, IPOINT    *p_point)
{
    #if USE_VIRT_COORD
	if (p_src_img == NULL) {
		DBG_ERR("p_src_img is NULL\r\n");
		return;
	}
	if (p_src_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_src_img is not initialized\r\n");
		return;
	}
	p_point->x += GXIMG_GET_VIRT_COORD(p_src_img)->x;
	p_point->y += GXIMG_GET_VIRT_COORD(p_src_img)->y;
	if ((GXIMG_GET_SCALE_RATIO(p_src_img)->x == SCALE_UNIT_1X) && (GXIMG_GET_SCALE_RATIO(p_src_img)->y == SCALE_UNIT_1X)) {
		return;
	}
	p_point->x = (p_point->x * GXIMG_GET_SCALE_RATIO(p_src_img)->x) >> SCALE_UNIT_SHIFT;
	p_point->y = (p_point->y * GXIMG_GET_SCALE_RATIO(p_src_img)->y) >> SCALE_UNIT_SHIFT;
    #endif
}

static void GXIMG_SCALE_RECT(PVDO_FRAME p_src_img, IRECT    *p_rect)
{
    #if USE_VIRT_COORD
	if (p_src_img == NULL) {
		DBG_ERR("p_src_img is NULL\r\n");
		return;
	}
	if (p_src_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_src_img is not initialized\r\n");
		return;
	}
	p_rect->x += GXIMG_GET_VIRT_COORD(p_src_img)->x;
	p_rect->y += GXIMG_GET_VIRT_COORD(p_src_img)->y;
	if ((GXIMG_GET_SCALE_RATIO(p_src_img)->x == SCALE_UNIT_1X) && (GXIMG_GET_SCALE_RATIO(p_src_img)->y == SCALE_UNIT_1X)) {
		return;
	}
	p_rect->x = (p_rect->x * GXIMG_GET_SCALE_RATIO(p_src_img)->x) >> SCALE_UNIT_SHIFT;
	p_rect->y = (p_rect->y * GXIMG_GET_SCALE_RATIO(p_src_img)->y) >> SCALE_UNIT_SHIFT;
	p_rect->w = (p_rect->w * GXIMG_GET_SCALE_RATIO(p_src_img)->x) >> SCALE_UNIT_SHIFT;
	p_rect->h = (p_rect->h * GXIMG_GET_SCALE_RATIO(p_src_img)->y) >> SCALE_UNIT_SHIFT;
    #endif
}

static ER gximg_argb_to_yuv_blend_with_flush(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 alpha, void *palette, int no_flush, int usePA)
{
	IRECT src_adj_rect = {0}, dst_adj_rect = {0};
	UINT32 grph_id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0);

	if (alpha == 0) {
		//Alpha=0, Only DstImg
		return E_OK;
	}

	if (alpha > 255) {
		DBG_ERR("alpha %d > 255 Not support\r\n", alpha);
		return E_PAR;
	}

	if (p_src_img->pxlfmt != VDO_PXLFMT_ARGB1555 &&
		p_src_img->pxlfmt != VDO_PXLFMT_ARGB4444 &&
		p_src_img->pxlfmt != VDO_PXLFMT_ARGB8888 &&
		p_src_img->pxlfmt != VDO_PXLFMT_RGB565   &&
		p_src_img->pxlfmt != VDO_PXLFMT_I1       &&
		p_src_img->pxlfmt != VDO_PXLFMT_I2       &&
		p_src_img->pxlfmt != VDO_PXLFMT_I4   ) {
		DBG_ERR("Invalid src fmt 0x%X\r\n", p_src_img->pxlfmt);
		return E_PAR;
	}

	if (p_src_img->pxlfmt == VDO_PXLFMT_I1 ||
		p_src_img->pxlfmt == VDO_PXLFMT_I2 ||
		p_src_img->pxlfmt == VDO_PXLFMT_I4) {
		if(!palette){
			DBG_ERR("palette image must have palette table\n");
			return E_PAR;
		}
	}

	if (p_dst_img->pxlfmt != VDO_PXLFMT_YUV422 &&
		p_dst_img->pxlfmt != VDO_PXLFMT_YUV420) {
		DBG_ERR("Invalid dst fmt 0x%X\r\n", p_dst_img->pxlfmt);
		return E_PAR;
	}

	if (E_OK != gximg_get_copy_rect(p_src_img, p_src_region, p_dst_location, p_dst_img, &src_adj_rect, &dst_adj_rect)) {
		DBG_ERR("src image is out of boundary of dst image\n");
		if(p_src_region)
			DBG_ERR("src image w(%d) h(%d), src region x(%d) y(%d) w(%d) h(%d)\n", p_src_img->size.w, p_src_img->size.h, p_src_region->x, p_src_region->y, p_src_region->w, p_src_region->h);
		else
			DBG_ERR("src image w(%d) h(%d)\n", p_src_img->size.w, p_src_img->size.h);
		DBG_ERR("dst image w(%d) h(%d), dst pos x(%d) y(%d)\n", p_dst_img->size.w, p_dst_img->size.h, p_dst_location->x, p_dst_location->y);
		return E_PAR;
	}
	DBG_MSG("[copy]p_src_img.W=%04d,H=%04d, lnOff[0]=%04d,PxlFmt=%x\r\n", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], p_src_img->pxlfmt);
	DBG_MSG("[copy]p_dst_img.W=%04d,H=%04d,OutOff[0]=%04d,PxlFmt=%x\r\n", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], p_dst_img->pxlfmt);

	if (dst_adj_rect.w < 2 || dst_adj_rect.h < 2) {
		DBG_ERR("AdjW %d AdjH %d\r\n", dst_adj_rect.w, dst_adj_rect.h);
		return E_PAR;
	}

#if USE_GRPH
	{
		KDRV_GRPH_TRIGGER_PARAM request = {0};
		GRPH_IMG image_a = {0};
		GRPH_IMG image_b = {0};
		GRPH_IMG image_c = {0};
		GRPH_PROPERTY property[4];
		IRECT src_grph_rect = {0};
		IRECT dst_grph_rect = {0};
		BOOL is_yuv420; //1: 420, 0: 422
		ER ret = E_OK;

		gximg_memset(property, 0, sizeof(property));

		is_yuv420 = (p_dst_img->pxlfmt == VDO_PXLFMT_YUV420) ? 1 : 0;

		request.command = GRPH_CMD_RGBYUV_BLEND;
		request.p_images = &image_a;
		request.p_property = &property[0];
		request.is_skip_cache_flush = check_no_flush(usePA, no_flush);
		request.is_buf_pa = usePA;

		//format and properties
		switch (p_src_img->pxlfmt) {
		case VDO_PXLFMT_ARGB1555:
			request.format = GRPH_FORMAT_16BITS_ARGB1555_RGB;
			property[0].id = GRPH_PROPERTY_ID_YUVFMT;
			property[0].property = is_yuv420;
			property[0].p_next = &property[1];
			property[1].id = GRPH_PROPERTY_ID_ALPHA0_INDEX;
			property[1].property = (alpha) & 0x0F; //Alpha[3..0]
			property[1].p_next = &property[2];
			property[2].id = GRPH_PROPERTY_ID_ALPHA1_INDEX;
			property[2].property = (alpha >> 4) & 0x0F; //Alpha[7..4]
			if (nvt_get_chip_id() == CHIP_NA51055){
				property[2].p_next = NULL;
			}else{
				property[2].p_next = &property[3];
				property[3].id = GRPH_PROPERTY_ID_UV_SUBSAMPLE;
				property[3].property = GRPH_UV_CENTERED;
				property[3].p_next = NULL;
			}
			break;
		case VDO_PXLFMT_ARGB4444:
			request.format = GRPH_FORMAT_16BITS_ARGB4444_RGB;
			property[0].id = GRPH_PROPERTY_ID_YUVFMT;
			property[0].property = is_yuv420;
			if (nvt_get_chip_id() == CHIP_NA51055){
				property[0].p_next = NULL;
			}else{
				property[0].p_next = &property[1];
				property[1].id = GRPH_PROPERTY_ID_UV_SUBSAMPLE;
				property[1].property = GRPH_UV_CENTERED;
				property[1].p_next = NULL;
			}
			break;
		case VDO_PXLFMT_ARGB8888:
			request.format = GRPH_FORMAT_32BITS_ARGB8888_RGB;
			property[0].id = GRPH_PROPERTY_ID_YUVFMT;
			property[0].property = is_yuv420;
			if (nvt_get_chip_id() == CHIP_NA51055) {
				property[0].p_next = NULL;
			} else {
				property[0].p_next = &property[1];
				property[1].id = GRPH_PROPERTY_ID_UV_SUBSAMPLE;
				property[1].property = GRPH_UV_CENTERED;
				property[1].p_next = NULL;
			}
			break;
		case VDO_PXLFMT_RGB565:
			request.format = GRPH_FORMAT_16BITS_RGB565;
			property[0].id = GRPH_PROPERTY_ID_YUVFMT;
			property[0].property = is_yuv420;
			property[0].p_next = &property[1];
			property[1].id = GRPH_PROPERTY_ID_NORMAL;
			property[1].property = alpha;
			if (nvt_get_chip_id() == CHIP_NA51055){
				property[1].p_next = NULL;
			}else{
				property[1].p_next = &property[2];
				property[2].id = GRPH_PROPERTY_ID_UV_SUBSAMPLE;
				property[2].property = GRPH_UV_CENTERED;
				property[2].p_next = NULL;
			}
			break;
		case VDO_PXLFMT_I1:
		case VDO_PXLFMT_I2:
		case VDO_PXLFMT_I4:
			if(p_src_img->pxlfmt == VDO_PXLFMT_I1)
				request.format = GRPH_FORMAT_PALETTE_1BIT;
			else if(p_src_img->pxlfmt == VDO_PXLFMT_I2)
				request.format = GRPH_FORMAT_PALETTE_2BITS;
			else
				request.format = GRPH_FORMAT_PALETTE_4BITS;
			property[0].id = GRPH_PROPERTY_ID_YUVFMT;
			property[0].property = is_yuv420;
			property[0].p_next = &property[1];
			property[1].id = GRPH_PROPERTY_ID_PAL_BUF;
			property[1].property = (UINT32)palette;
			if (nvt_get_chip_id() == CHIP_NA51055){
				property[1].p_next = NULL;
			}else{
				property[1].p_next = &property[2];
				property[2].id = GRPH_PROPERTY_ID_UV_SUBSAMPLE;
				property[2].property = GRPH_UV_CENTERED;
				property[2].p_next = NULL;
			}
			break;
		default:
			DBG_ERR("Not supported fmt 0x%X\r\n", p_src_img->pxlfmt);
			return E_PAR;
		}

		//image setting
		gximg_calc_plane_rect(p_src_img, 0, &src_adj_rect, &src_grph_rect);

		image_a.img_id = GRPH_IMG_ID_A; //src argb
		image_a.dram_addr = (UINT32)p_src_img->addr[0] + gximg_calc_plane_offset(p_src_img, 0, &src_grph_rect);
		image_a.lineoffset = p_src_img->loff[0];
		if(p_src_img->pxlfmt == VDO_PXLFMT_I1 ||
		   p_src_img->pxlfmt == VDO_PXLFMT_I2 ||
		   p_src_img->pxlfmt == VDO_PXLFMT_I4){
			image_a.width = p_src_img->size.w;
			image_a.height = p_src_img->size.h;
		}else{
			image_a.width = src_grph_rect.w;
			image_a.height = src_grph_rect.h;
		}
		image_a.p_next = &image_b;

		gximg_calc_plane_rect(p_dst_img, 0, &dst_adj_rect, &dst_grph_rect);
		image_b.img_id = GRPH_IMG_ID_B; //dst Y
		image_b.dram_addr = (UINT32)p_dst_img->addr[0] + gximg_calc_plane_offset(p_dst_img, 0, &dst_grph_rect);
		image_b.lineoffset = p_dst_img->loff[0];
		image_b.p_next = &image_c;

		gximg_calc_plane_rect(p_dst_img, 1, &dst_adj_rect, &dst_grph_rect);
		image_c.img_id = GRPH_IMG_ID_C; //dst UV
		image_c.dram_addr = (UINT32)p_dst_img->addr[1] + gximg_calc_plane_offset(p_dst_img, 1, &dst_grph_rect);;
		image_c.lineoffset = p_dst_img->loff[1];
		image_c.p_next = NULL;

		if(gximg_check_grph_limit(request.command, request.format, &image_a, 1, GRPH_IMG_ID_A) ||
			gximg_check_grph_limit(request.command, request.format, &image_b, 0, GRPH_IMG_ID_B) ||
			gximg_check_grph_limit(request.command, request.format, &image_c, 0, GRPH_IMG_ID_C)){
			DBG_ERR("limit check fail\n");
			return E_PAR;
		}

		if (E_OK != gximg_open_grph(grph_id)) {
			DBG_ERR("gximg_open_grph\r\n");
			return E_SYS;
		}

		if (E_OK != kdrv_grph_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0), &request, NULL, NULL)) {
			DBG_ERR("kdrv_grph_trigger\r\n");
			ret = E_SYS;
		}

		if (E_OK != gximg_close_grph(grph_id)) {
			DBG_ERR("gximg_close_grph\r\n");
			return E_SYS;
		}

		DBG_FUNC_END("[copy]\r\n");
		return ret;
	}
#else
	DBG_FUNC_END("[copy]\r\n");
	return E_NOSPT;
#endif
}

ER gximg_argb_to_yuv_blend(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 alpha, void *palette, int usePA)
{
	return gximg_argb_to_yuv_blend_with_flush(p_src_img, p_src_region, p_dst_img, p_dst_location, alpha, palette, 0, usePA);
}

ER gximg_argb_to_yuv_blend_no_flush(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 alpha, void *palette, int usePA)
{
	return gximg_argb_to_yuv_blend_with_flush(p_src_img, p_src_region, p_dst_img, p_dst_location, alpha, palette, 1, usePA);
}

ER gximg_rgb_to_yuv_color_key(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 alpha, UINT32 color_key, int usePA)
{
	IRECT src_adj_rect = {0}, dst_adj_rect = {0};
	UINT32 grph_id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0);

	if (alpha == 0) {
		//Alpha=0, Only DstImg
		return E_OK;
	}

	if (alpha > 255) {
		DBG_ERR("alpha %d > 255 Not support\r\n", alpha);
		return E_PAR;
	}

	if (p_src_img->pxlfmt != VDO_PXLFMT_RGB565   &&
	    p_src_img->pxlfmt != VDO_PXLFMT_ARGB1555 &&
		p_src_img->pxlfmt != VDO_PXLFMT_ARGB4444) {
		DBG_ERR("Invalid src fmt 0x%X\r\n", p_src_img->pxlfmt);
		return E_PAR;
	}

	if (p_dst_img->pxlfmt != VDO_PXLFMT_YUV422 &&
		p_dst_img->pxlfmt != VDO_PXLFMT_YUV420) {
		DBG_ERR("Invalid dst fmt 0x%X\r\n", p_dst_img->pxlfmt);
		return E_PAR;
	}

	if (E_OK != gximg_get_copy_rect(p_src_img, p_src_region, p_dst_location, p_dst_img, &src_adj_rect, &dst_adj_rect)) {
		DBG_ERR("gximg_get_copy_rect\r\n");
		return E_PAR;
	}
	DBG_MSG("[copy]p_src_img.W=%04d,H=%04d, lnOff[0]=%04d,PxlFmt=%x\r\n", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], p_src_img->pxlfmt);
	DBG_MSG("[copy]p_dst_img.W=%04d,H=%04d,OutOff[0]=%04d,PxlFmt=%x\r\n", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], p_dst_img->pxlfmt);

	if (dst_adj_rect.w < 2 || dst_adj_rect.h < 2) {
		DBG_ERR("AdjW %d AdjH %d\r\n", dst_adj_rect.w, dst_adj_rect.h);
		return E_PAR;
	}

#if USE_GRPH
	{
		KDRV_GRPH_TRIGGER_PARAM request = {0};
		GRPH_IMG image_a = {0};
		GRPH_IMG image_b = {0};
		GRPH_IMG image_c = {0};
		GRPH_PROPERTY property[6];
		IRECT src_grph_rect = {0};
		IRECT dst_grph_rect = {0};
		BOOL is_yuv420; //1: 420, 0: 422
		ER ret = E_OK;

		gximg_memset(property, 0, sizeof(property));

		is_yuv420 = (p_dst_img->pxlfmt == VDO_PXLFMT_YUV420) ? 1 : 0;

		request.command = GRPH_CMD_RGBYUV_COLORKEY;
		request.is_skip_cache_flush = check_no_flush(usePA, 0);
		request.is_buf_pa = usePA;
		request.p_images = &image_a;
		request.p_property = &property[0];

		//format and properties
		switch (p_src_img->pxlfmt) {
		case VDO_PXLFMT_RGB565:
			request.format = GRPH_FORMAT_16BITS_RGB565;

			property[0].id = GRPH_PROPERTY_ID_YUVFMT;
			property[0].property = is_yuv420;
			property[0].p_next = &property[1];

			property[1].id = GRPH_PROPERTY_ID_NORMAL;
			property[1].property = alpha;                       // alpha value
			property[1].p_next = &property[2];

			property[2].id = GRPH_PROPERTY_ID_R;
			property[2].property = COLOR_RGBD_GET_R(color_key);  // R color key
			property[2].p_next = &property[3];

			property[3].id = GRPH_PROPERTY_ID_G;
			property[3].property = COLOR_RGBD_GET_G(color_key);  // G color key
			property[3].p_next = &property[4];

			property[4].id = GRPH_PROPERTY_ID_B;
			property[4].property = COLOR_RGBD_GET_B(color_key);  // B color key
			property[4].p_next = NULL;
			break;
		case VDO_PXLFMT_ARGB1555:
			request.format = GRPH_FORMAT_16BITS_ARGB1555_RGB;

			property[0].id = GRPH_PROPERTY_ID_YUVFMT;
			property[0].property = is_yuv420;
			property[0].p_next = &property[1];

			property[1].id = GRPH_PROPERTY_ID_ALPHA0_INDEX;
			property[1].property = 0;
			property[1].p_next = &property[2];

			property[2].id = GRPH_PROPERTY_ID_ALPHA1_INDEX;
			property[2].property = 0xF;
			property[2].p_next = &property[3];

			property[3].id = GRPH_PROPERTY_ID_R;
			property[3].property = ((color_key >> 10) & 0x1F);  // R color key
			property[3].p_next = &property[4];

			property[4].id = GRPH_PROPERTY_ID_G;
			property[4].property = ((color_key >> 5) & 0x1F);  // G color key
			property[4].p_next = &property[5];;

			property[5].id = GRPH_PROPERTY_ID_B;
			property[5].property = (color_key & 0x1F);  // B color key
			property[5].p_next = NULL;
			break;
		case VDO_PXLFMT_ARGB4444:
			request.format = GRPH_FORMAT_16BITS_ARGB4444_RGB;

			property[0].id = GRPH_PROPERTY_ID_YUVFMT;
			property[0].property = is_yuv420;
			property[0].p_next = &property[1];

			property[1].id = GRPH_PROPERTY_ID_R;
			property[1].property = ((color_key >> 8) & 0xF);  // R color key
			property[1].p_next = &property[2];

			property[2].id = GRPH_PROPERTY_ID_G;
			property[2].property = ((color_key >> 4) & 0xF);  // G color key
			property[2].p_next = &property[3];

			property[3].id = GRPH_PROPERTY_ID_B;
			property[3].property = (color_key & 0xF);  // B color key
			property[3].p_next = NULL;
			break;
		default:
			DBG_ERR("Not supported fmt 0x%X\r\n", p_src_img->pxlfmt);
			return E_PAR;
		}

		//image setting
		gximg_calc_plane_rect(p_src_img, 0, &src_adj_rect, &src_grph_rect);

		image_a.img_id = GRPH_IMG_ID_A; //src RGB
		image_a.dram_addr = (UINT32)p_src_img->addr[0] + gximg_calc_plane_offset(p_src_img, 0, &src_grph_rect);;
		image_a.lineoffset = p_src_img->loff[0];
		image_a.width = src_grph_rect.w;
		image_a.height = src_grph_rect.h;
		image_a.p_next = &image_b;

		gximg_calc_plane_rect(p_dst_img, 0, &dst_adj_rect, &dst_grph_rect);
		image_b.img_id = GRPH_IMG_ID_B; //dst Y
		image_b.dram_addr = (UINT32)p_dst_img->addr[0] + gximg_calc_plane_offset(p_dst_img, 0, &dst_grph_rect);
		image_b.lineoffset = p_dst_img->loff[0];
		image_b.p_next = &image_c;

		gximg_calc_plane_rect(p_dst_img, 1, &dst_adj_rect, &dst_grph_rect);
		image_c.img_id = GRPH_IMG_ID_C; //dst UV
		image_c.dram_addr = (UINT32)p_dst_img->addr[1] + gximg_calc_plane_offset(p_dst_img, 1, &dst_grph_rect);;
		image_c.lineoffset = p_dst_img->loff[1];
		image_c.p_next = NULL;

		if(gximg_check_grph_limit(request.command, request.format, &image_a, 1, GRPH_IMG_ID_A) ||
			gximg_check_grph_limit(request.command, request.format, &image_b, 0, GRPH_IMG_ID_B) ||
			gximg_check_grph_limit(request.command, request.format, &image_c, 0, GRPH_IMG_ID_C)){
			DBG_ERR("limit check fail\n");
			return E_PAR;
		}

		if (E_OK != gximg_open_grph(grph_id)) {
			DBG_ERR("gximg_open_grph\r\n");
			return E_SYS;
		}

		if (E_OK != kdrv_grph_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0), &request, NULL, NULL)) {
			DBG_ERR("graph_request\r\n");
			ret = E_SYS;
		}

		if (E_OK != gximg_close_grph(grph_id)) {
			DBG_ERR("gximg_close_grph\r\n");
			return E_SYS;
		}

		DBG_FUNC_END("[copy]\r\n");
		return ret;
	}
#else
	DBG_FUNC_END("[copy]\r\n");
	return E_NOSPT;
#endif
}

static ER _420_planar_to_packed(PVDO_FRAME p_src_img, PVDO_FRAME p_dst_img, int usePA)
{
	UINT32                     grph_id;
	KDRV_GRPH_TRIGGER_PARAM    request;
	GRPH_IMG                   images[3];

	DBG_FUNC_BEGIN("[copy]\r\n");

	if (p_src_img->pxlfmt != VDO_PXLFMT_YUV420_PLANAR || p_dst_img->pxlfmt != VDO_PXLFMT_YUV420) {
		DBG_ERR("p_src_img->pxlfmt %d !=  p_dst_img->pxlfmt %d\r\n", p_src_img->pxlfmt, p_dst_img->pxlfmt);
		return E_PAR;
	}

	if(!p_src_img->addr[0] || !p_src_img->loff[0]){
		DBG_ERR("Invalid src y addr(%x) or loff(%d)\n", p_src_img->addr[0], p_src_img->loff[0]);
		return E_PAR;
	}
	if(!p_src_img->addr[1] || !p_src_img->loff[1]){
		DBG_ERR("Invalid src u addr(%x) or loff(%d)\n", p_src_img->addr[1], p_src_img->loff[1]);
		return E_PAR;
	}
	if(!p_src_img->addr[2] || !p_src_img->loff[2]){
		DBG_ERR("Invalid src v addr(%x) or loff(%d)\n", p_src_img->addr[2], p_src_img->loff[2]);
		return E_PAR;
	}

	if(!p_dst_img->addr[0] || !p_dst_img->loff[0]){
		DBG_ERR("Invalid dst y addr(%x) or loff(%d)\n", p_dst_img->addr[0], p_dst_img->loff[0]);
		return E_PAR;
	}
	if(!p_dst_img->addr[1] || !p_dst_img->loff[1]){
		DBG_ERR("Invalid dst uv addr(%x) or loff(%d)\n", p_dst_img->addr[1], p_dst_img->loff[1]);
		return E_PAR;
	}

	DBG_MSG("[copy] p_src_img.W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], p_src_img->loff[1], p_src_img->loff[2], p_src_img->pxlfmt);
	DBG_MSG("[copy] p_dst_img.W=%04d,H=%04d,OutOff[0]=%04d,OutOff[1]=%04d,OutOff[2]=%04d,PxlFmt=%x\r\n", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], p_dst_img->loff[1], p_dst_img->loff[2], p_dst_img->pxlfmt);

	grph_id = gximg_copy_map_to_grph_id(GXIMG_CP_ENG1);
	DBG_MSG("[copy]grph_id = %d\r\n", grph_id);

	//generate y plane by copy
	gximg_memset(images, 0, sizeof(images));
	images[0].img_id     =    GRPH_IMG_ID_A;       // source image
	images[0].dram_addr  =    (UINT32)p_src_img->addr[0];
	images[0].lineoffset =    p_src_img->loff[0];
	images[0].width      =    p_src_img->size.w;
	images[0].height     =    p_src_img->size.h;
	images[0].p_next     =    &(images[1]);
	images[1].img_id     =    GRPH_IMG_ID_C;       // destination image
	images[1].dram_addr  =    (UINT32)p_dst_img->addr[0];
	images[1].lineoffset =    p_dst_img->loff[0];
	images[1].width      =    p_src_img->size.w;
	images[1].height     =    p_src_img->size.h;

	gximg_memset(&request, 0, sizeof(KDRV_GRPH_TRIGGER_PARAM));
	request.command = GRPH_CMD_A_COPY;
	request.format = GRPH_FORMAT_8BITS;
	request.is_skip_cache_flush = check_no_flush(usePA, 0);
	request.is_buf_pa = usePA;
	request.p_images = &(images[0]);

	if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
		gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_C)){
		DBG_ERR("limit check fail\n");
		return E_PAR;
	}

	if(gximg_open_grph(grph_id) != E_OK){
		DBG_ERR("fail to open graphic engine(%d)\n", grph_id);
		return E_PAR;
	}
	if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
		DBG_ERR("fail to trigger graphic engine(%d)\n", grph_id);
		return E_PAR;
	}
	if(gximg_close_grph(grph_id) != E_OK){
		DBG_ERR("fail to close graphic engine(%d)\n", grph_id);
		return E_PAR;
	}

	//generate uv plane by packing
	gximg_memset(images, 0, sizeof(images));
	images[0].img_id     =    GRPH_IMG_ID_A;
	images[0].dram_addr  =    (UINT32)p_src_img->addr[1];
	images[0].lineoffset =    p_src_img->loff[1];
	images[0].width      =    p_src_img->size.w;
	images[0].height     =    p_src_img->size.h;
	images[0].p_next     =    &(images[1]);
	images[1].img_id     =    GRPH_IMG_ID_B;
	images[1].dram_addr  =    (UINT32)p_src_img->addr[2];
	images[1].lineoffset =    p_src_img->loff[2];
	images[1].p_next     =    &(images[2]);
	images[2].img_id     =    GRPH_IMG_ID_C;
	images[2].dram_addr  =    (UINT32)p_dst_img->addr[1];
	images[2].lineoffset =    p_dst_img->loff[1];
	images[2].p_next     =    NULL;

	gximg_memset(&request, 0, sizeof(KDRV_GRPH_TRIGGER_PARAM));
	request.command  = GRPH_CMD_PACKING;
	request.format   = GRPH_FORMAT_8BITS;
	request.is_skip_cache_flush = check_no_flush(usePA, 0);
	request.is_buf_pa = usePA;
	request.p_images = &(images[0]);

	if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
		gximg_check_grph_limit(request.command, request.format, &(images[1]), 0, GRPH_IMG_ID_B) ||
		gximg_check_grph_limit(request.command, request.format, &(images[2]), 0, GRPH_IMG_ID_C)){
		DBG_ERR("limit check fail\n");
		return E_PAR;
	}

	if(gximg_open_grph(grph_id) != E_OK){
		DBG_ERR("fail to open graphic engine(%d)\n", grph_id);
		return E_PAR;
	}
	if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
		DBG_ERR("fail to trigger graphic engine(%d)\n", grph_id);
		return E_PAR;
	}
	if(gximg_close_grph(grph_id) != E_OK){
		DBG_ERR("fail to close graphic engine(%d)\n", grph_id);
		return E_PAR;
	}

	return E_OK;
}

static ER _420_packed_to_planar(PVDO_FRAME p_src_img, PVDO_FRAME p_dst_img, int usePA)
{
	UINT32                     grph_id;
	KDRV_GRPH_TRIGGER_PARAM    request;
	GRPH_IMG                   images[3];

	DBG_FUNC_BEGIN("[copy]\r\n");

	if (p_src_img->pxlfmt != VDO_PXLFMT_YUV420 || p_dst_img->pxlfmt != VDO_PXLFMT_YUV420_PLANAR) {
		DBG_ERR("p_src_img->pxlfmt %d !=  p_dst_img->pxlfmt %d\r\n", p_src_img->pxlfmt, p_dst_img->pxlfmt);
		return E_PAR;
	}

	if(!p_src_img->addr[0] || !p_src_img->loff[0]){
		DBG_ERR("Invalid src y addr(%x) or loff(%d)\n", p_src_img->addr[0], p_src_img->loff[0]);
		return E_PAR;
	}
	if(!p_src_img->addr[1] || !p_src_img->loff[1]){
		DBG_ERR("Invalid src uv addr(%x) or loff(%d)\n", p_src_img->addr[1], p_src_img->loff[1]);
		return E_PAR;
	}

	if(!p_dst_img->addr[0] || !p_dst_img->loff[0]){
		DBG_ERR("Invalid dst y addr(%x) or loff(%d)\n", p_dst_img->addr[0], p_dst_img->loff[0]);
		return E_PAR;
	}
	if(!p_dst_img->addr[1] || !p_dst_img->loff[1]){
		DBG_ERR("Invalid dst u addr(%x) or loff(%d)\n", p_dst_img->addr[1], p_dst_img->loff[1]);
		return E_PAR;
	}
	if(!p_dst_img->addr[2] || !p_dst_img->loff[2]){
		DBG_ERR("Invalid dst v addr(%x) or loff(%d)\n", p_dst_img->addr[2], p_dst_img->loff[2]);
		return E_PAR;
	}

	DBG_MSG("[copy] p_src_img.W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], p_src_img->loff[1], p_src_img->loff[2], p_src_img->pxlfmt);
	DBG_MSG("[copy] p_dst_img.W=%04d,H=%04d,OutOff[0]=%04d,OutOff[1]=%04d,OutOff[2]=%04d,PxlFmt=%x\r\n", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], p_dst_img->loff[1], p_dst_img->loff[2], p_dst_img->pxlfmt);

	grph_id = gximg_copy_map_to_grph_id(GXIMG_CP_ENG1);
	DBG_MSG("[copy]grph_id = %d\r\n", grph_id);

	//generate y plane by copy
	gximg_memset(images, 0, sizeof(images));
	images[0].img_id     =    GRPH_IMG_ID_A;       // source image
	images[0].dram_addr  =    (UINT32)p_src_img->addr[0];
	images[0].lineoffset =    p_src_img->loff[0];
	images[0].width      =    p_src_img->size.w;
	images[0].height     =    p_src_img->size.h;
	images[0].p_next     =    &(images[1]);
	images[1].img_id     =    GRPH_IMG_ID_C;       // destination image
	images[1].dram_addr  =    (UINT32)p_dst_img->addr[0];
	images[1].lineoffset =    p_dst_img->loff[0];
	images[1].width      =    p_src_img->size.w;
	images[1].height     =    p_src_img->size.h;

	gximg_memset(&request, 0, sizeof(KDRV_GRPH_TRIGGER_PARAM));
	request.command = GRPH_CMD_A_COPY;
	request.format = GRPH_FORMAT_8BITS;
	request.is_skip_cache_flush = check_no_flush(usePA, 0);
	request.is_buf_pa = usePA;
	request.p_images = &(images[0]);

	if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
		gximg_check_grph_limit(request.command, request.format, &(images[1]), 1, GRPH_IMG_ID_C)){
		DBG_ERR("limit check fail\n");
		return E_PAR;
	}

	if(gximg_open_grph(grph_id) != E_OK){
		DBG_ERR("fail to open graphic engine(%d)\n", grph_id);
		return E_PAR;
	}
	if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
		DBG_ERR("fail to trigger graphic engine(%d)\n", grph_id);
		return E_PAR;
	}
	if(gximg_close_grph(grph_id) != E_OK){
		DBG_ERR("fail to close graphic engine(%d)\n", grph_id);
		return E_PAR;
	}

	//generate u/v plane by depacking
	gximg_memset(images, 0, sizeof(images));
	images[0].img_id     =    GRPH_IMG_ID_A;
	images[0].dram_addr  =    (UINT32)p_src_img->addr[1];
	images[0].lineoffset =    p_src_img->loff[1];
	images[0].width      =    p_src_img->size.w;
	images[0].height     =    p_src_img->size.h/2;
	images[0].p_next     =    &(images[1]);
	images[1].img_id     =    GRPH_IMG_ID_B;
	images[1].dram_addr  =    (UINT32)p_dst_img->addr[1];
	images[1].lineoffset =    p_dst_img->loff[1];
	images[1].p_next     =    &(images[2]);
	images[2].img_id     =    GRPH_IMG_ID_C;
	images[2].dram_addr  =    (UINT32)p_dst_img->addr[2];
	images[2].lineoffset =    p_dst_img->loff[2];
	images[2].p_next     =    NULL;

	gximg_memset(&request, 0, sizeof(KDRV_GRPH_TRIGGER_PARAM));
	request.command  = GRPH_CMD_DEPACKING;
	request.format   = GRPH_FORMAT_16BITS;
	request.is_skip_cache_flush = check_no_flush(usePA, 0);
	request.is_buf_pa = usePA;
	request.p_images = &(images[0]);

	if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1, GRPH_IMG_ID_A) ||
		gximg_check_grph_limit(request.command, request.format, &(images[1]), 0, GRPH_IMG_ID_B) ||
		gximg_check_grph_limit(request.command, request.format, &(images[2]), 0, GRPH_IMG_ID_C)){
		DBG_ERR("limit check fail\n");
		return E_PAR;
	}

	if(gximg_open_grph(grph_id) != E_OK){
		DBG_ERR("fail to open graphic engine(%d)\n", grph_id);
		return E_PAR;
	}
	if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
		DBG_ERR("fail to trigger graphic engine(%d)\n", grph_id);
		return E_PAR;
	}
	if(gximg_close_grph(grph_id) != E_OK){
		DBG_ERR("fail to close graphic engine(%d)\n", grph_id);
		return E_PAR;
	}

	return E_OK;
}

static ER _422_packed_to_yuyv(PVDO_FRAME p_src_img, PVDO_FRAME p_dst_img, int usePA)
{
	UINT32                     grph_id;
	KDRV_GRPH_TRIGGER_PARAM    request;
	GRPH_IMG                   images[3];

	DBG_FUNC_BEGIN("[copy]\r\n");

	if (p_src_img->pxlfmt != VDO_PXLFMT_YUV422 || p_dst_img->pxlfmt != VDO_PXLFMT_YUV422_ONE) {
		DBG_ERR("p_src_img->pxlfmt %d !=  VDO_PXLFMT_YUV422 or p_dst_img->pxlfmt %d !=  VDO_PXLFMT_YUV422_ONE\r\n",
					p_src_img->pxlfmt, p_dst_img->pxlfmt);
		return E_PAR;
	}

	if(!p_src_img->addr[0] || !p_src_img->loff[0]){
		DBG_ERR("Invalid src y addr(%x) or loff(%d)\n", p_src_img->addr[0], p_src_img->loff[0]);
		return E_PAR;
	}
	if(!p_src_img->addr[1] || !p_src_img->loff[1]){
		DBG_ERR("Invalid src u addr(%x) or loff(%d)\n", p_src_img->addr[1], p_src_img->loff[1]);
		return E_PAR;
	}

	if(!p_dst_img->addr[0] || !p_dst_img->loff[0]){
		DBG_ERR("Invalid dst y addr(%x) or loff(%d)\n", p_dst_img->addr[0], p_dst_img->loff[0]);
		return E_PAR;
	}

	DBG_MSG("[copy] p_src_img.W=%04d,H=%04d, lnOff[0]=%04d, lnOff[1]=%04d, lnOff[2]=%04d,PxlFmt=%x\r\n", p_src_img->size.w, p_src_img->size.h, p_src_img->loff[0], p_src_img->loff[1], p_src_img->loff[2], p_src_img->pxlfmt);
	DBG_MSG("[copy] p_dst_img.W=%04d,H=%04d,OutOff[0]=%04d,OutOff[1]=%04d,OutOff[2]=%04d,PxlFmt=%x\r\n", p_dst_img->size.w, p_dst_img->size.h, p_dst_img->loff[0], p_dst_img->loff[1], p_dst_img->loff[2], p_dst_img->pxlfmt);

	grph_id = gximg_copy_map_to_grph_id(GXIMG_CP_ENG1);
	DBG_MSG("[copy]grph_id = %d\r\n", grph_id);

	gximg_memset(images, 0, sizeof(images));
	images[0].img_id     =    GRPH_IMG_ID_A;
	images[0].dram_addr  =    (UINT32)p_src_img->addr[0];
	images[0].lineoffset =    p_src_img->loff[0];
	images[0].width      =    p_src_img->size.w;
	images[0].height     =    p_src_img->size.h;
	images[0].p_next     =    &(images[1]);
	images[1].img_id     =    GRPH_IMG_ID_B;
	images[1].dram_addr  =    (UINT32)p_src_img->addr[1];
	images[1].lineoffset =    p_src_img->loff[1];
	images[1].p_next     =    &(images[2]);
	images[2].img_id     =    GRPH_IMG_ID_C;
	images[2].dram_addr  =    (UINT32)p_dst_img->addr[0];
	images[2].lineoffset =    p_dst_img->loff[0];
	images[2].p_next     =    NULL;

	gximg_memset(&request, 0, sizeof(KDRV_GRPH_TRIGGER_PARAM));
	request.command  = GRPH_CMD_PACKING;
	request.format   = GRPH_FORMAT_8BITS;
	request.is_skip_cache_flush = check_no_flush(usePA, 0);
	request.is_buf_pa = usePA;
	request.p_images = &(images[0]);

	if (gximg_open_grph(grph_id) == -1) {
		DBG_ERR("fail to open graphic engine(%d)\n", grph_id);
		return E_PAR;
	}
	if (kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK) {
		DBG_ERR("kdrv_grph_trigger() fail\n");
		gximg_close_grph(grph_id);
		return E_PAR;
	}
	if (gximg_close_grph(grph_id) != E_OK) {
		DBG_ERR("fail to close graphic engine(%d)\n", grph_id);
		return E_PAR;
	}

	return E_OK;
}

static ER _420_packed_to_yuyv(PVDO_FRAME p_src_img, PVDO_FRAME p_dst_img, UINT32 tmp_buff, UINT32 tmp_buf_size, int usePA)
{
	UINT32                     grph_id;
	KDRV_GRPH_TRIGGER_PARAM    request;
	GRPH_IMG                   images[3];

	//sanity check
	if (p_src_img->pxlfmt != VDO_PXLFMT_YUV420 || p_dst_img->pxlfmt != VDO_PXLFMT_YUV422_ONE) {
		DBG_ERR("p_src_img->pxlfmt %d !=  VDO_PXLFMT_YUV420 or p_dst_img->pxlfmt %d !=  VDO_PXLFMT_YUV422_ONE\r\n",
					p_src_img->pxlfmt, p_dst_img->pxlfmt);
		return E_PAR;
	}
	if(!p_src_img->addr[0] || !p_src_img->loff[0]){
		DBG_ERR("Invalid src y addr(%x) or loff(%d)\n", p_src_img->addr[0], p_src_img->loff[0]);
		return E_PAR;
	}
	if(!p_src_img->addr[1] || !p_src_img->loff[1]){
		DBG_ERR("Invalid src u addr(%x) or loff(%d)\n", p_src_img->addr[1], p_src_img->loff[1]);
		return E_PAR;
	}
	if(!p_dst_img->addr[0] || !p_dst_img->loff[0]){
		DBG_ERR("Invalid dst y addr(%x) or loff(%d)\n", p_dst_img->addr[0], p_dst_img->loff[0]);
		return E_PAR;
	}
	if(p_src_img->size.w != p_dst_img->size.w || p_src_img->size.h != p_dst_img->size.h){
		DBG_ERR("src dim(%dx%d) must = dst dim(%dx%d)\n", p_src_img->size.w, p_dst_img->size.w, p_src_img->size.h, p_dst_img->size.h);
		return E_PAR;
	}

	gximg_memset(images, 0, sizeof(images));
	images[0].img_id     =    GRPH_IMG_ID_A;
	images[0].dram_addr  =    (UINT32)p_src_img->addr[0];
	images[0].lineoffset =    p_src_img->loff[0]*2;
	images[0].width      =    p_src_img->size.w;
	images[0].height     =    p_src_img->size.h/2;
	images[0].p_next     =    &(images[1]);
	images[1].img_id     =    GRPH_IMG_ID_B;
	images[1].dram_addr  =    (UINT32)p_src_img->addr[1];
	images[1].lineoffset =    p_src_img->loff[1];
	images[1].p_next     =    &(images[2]);
	images[2].img_id     =    GRPH_IMG_ID_C;
	images[2].dram_addr  =    (UINT32)p_dst_img->addr[0];
	images[2].lineoffset =    p_dst_img->loff[0]*2;
	images[2].p_next     =    NULL;

	grph_id = gximg_copy_map_to_grph_id(GXIMG_CP_ENG1);

	gximg_memset(&request, 0, sizeof(KDRV_GRPH_TRIGGER_PARAM));
	request.command  = GRPH_CMD_PACKING;
	request.format   = GRPH_FORMAT_8BITS;
	request.p_images = &(images[0]);
	request.is_skip_cache_flush = check_no_flush(usePA, 0);

	if(gximg_open_grph(grph_id) == -1){
		DBG_ERR("gximg_open_grph(%d) fail\n", grph_id);
		return E_PAR;
	}
	if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
		DBG_ERR("fail to start graphic engine(%d)\n", grph_id);
		gximg_close_grph(grph_id);
		return E_PAR;
	}
	if(gximg_close_grph(grph_id) == -1){
		DBG_ERR("gximg_close_grph(%d) fail\n", grph_id);
		return E_PAR;
	}

	images[0].dram_addr  = (UINT32)p_src_img->addr[0] + images[0].width;
	images[2].dram_addr  = (UINT32)p_dst_img->addr[0] + images[0].width * 2;

	if(gximg_open_grph(grph_id) == -1){
		DBG_ERR("gximg_open_grph(%d) fail\n", grph_id);
		return E_PAR;
	}
	if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
		DBG_ERR("fail to start graphic engine(%d)\n", grph_id);
		gximg_close_grph(grph_id);
		return E_PAR;
	}
	if(gximg_close_grph(grph_id) == -1){
		DBG_ERR("gximg_close_grph(%d) fail\n", grph_id);
		return E_PAR;
	}

	return E_OK;
}

#if USE_ISE
static ER _444_planar_to_420_planar(PVDO_FRAME p_src_img, PVDO_FRAME p_dst_img, int usePA)
{
	int i;

	if (VDO_PXLFMT_YUV444_PLANAR != p_src_img->pxlfmt ||
		VDO_PXLFMT_YUV420_PLANAR != p_dst_img->pxlfmt) {
		DBG_ERR("Src fmt %d != 444 Planar or Dst fmt %d != 420 Planar\r\n", p_src_img->pxlfmt, p_dst_img->pxlfmt);
		return E_PAR;
	}

	{
		KDRV_ISE_MODE             ise_op = {0};
		KDRV_ISE_TRIGGER_PARAM    ise_trigger = {1, 0};

		if(gximg_ise_open(GXIMG_SC_ENG1) != E_OK){
			DBG_ERR("gximg_ise_open() failed\r\n");
			return E_PAR;
		}
		for(i = 0 ; i < 3 ; ++i){
			ise_op.io_pack_fmt = KDRV_ISE_Y8;
			ise_op.scl_method = KDRV_ISE_NEAREST;
			ise_op.in_buf_flush = check_no_flush(usePA, 0);
			ise_op.out_buf_flush = check_no_flush(usePA, 0);
			ise_op.phy_addr_en = usePA;
			ise_op.in_width = p_src_img->size.w;
			ise_op.in_height = p_src_img->size.h;
			ise_op.in_lofs = p_src_img->loff[i];
			ise_op.in_addr = p_src_img->addr[i];
			if(i == 0){
				ise_op.out_width = p_dst_img->size.w;
				ise_op.out_height = p_dst_img->size.h;
			}else{
				ise_op.out_width = ((p_dst_img->size.w)>>1);
				ise_op.out_height = ((p_dst_img->size.h)>>1);
			}
			ise_op.out_lofs = p_dst_img->loff[i];
			ise_op.out_addr = p_dst_img->addr[i];
			if (gximg_check_ise_limit(&ise_op)) {
				DBG_ERR("checking ise limit failed\r\n");
				gximg_ise_close(GXIMG_SC_ENG1);
				return E_PAR;
			}
			if (gximg_ise_set_mode(GXIMG_SC_ENG1, &ise_op) != E_OK) {
				DBG_ERR("ise_setMode error ..\r\n");
				gximg_ise_close(GXIMG_SC_ENG1);
				return E_PAR;
			}
			if(gximg_ise_start(GXIMG_SC_ENG1, &ise_trigger) != E_OK){
				DBG_ERR("gximg_ise_start() fail\r\n");
				gximg_ise_close(GXIMG_SC_ENG1);
				return E_PAR;
			}
		}

		#if 0
		for(i = 1 ; i < plane_num ; ++i){
			if (plane_num > 1) {
				if(p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR)
					ise_op.in_width = tmp_src_plane_rect.w;
				else
					// UV packed the UV width should be the half of Y width
					ise_op.in_width = tmp_src_plane_rect.w >> 1;
				ise_op.in_height = tmp_src_plane_rect.h;
				ise_op.in_lofs = p_src_img->loff[i];
				ise_op.in_addr = p_src_img->addr[i] + in_loff[i];
				if(p_dst_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR)
					ise_op.out_width = tmp_dst_plane_rect.w;
				else
					// UV packed the UV width should be the half of Y width
					ise_op.out_width = tmp_dst_plane_rect.w >> 1;
				ise_op.out_height = tmp_dst_plane_rect.h;
				ise_op.out_lofs = p_dst_img->loff[i];
				ise_op.out_addr = p_dst_img->addr[i] + out_loff[i];
			}
		}
		#endif
		if(gximg_ise_close(GXIMG_SC_ENG1) != E_OK){
			DBG_ERR("gximg_ise_close() fail\r\n");
			return E_PAR;
		}
	}
	DBG_FUNC_END("[scale]\r\n");

	return E_OK;
}
#endif

static ER _422_to_420(PVDO_FRAME p_src_img, PVDO_FRAME p_dst_img)
{
	UINT32                     grph_id;
	KDRV_GRPH_TRIGGER_PARAM    request;
	GRPH_IMG                   images[2];

	//sanity check
	if (p_src_img->pxlfmt != VDO_PXLFMT_YUV422 || p_dst_img->pxlfmt != VDO_PXLFMT_YUV420) {
		DBG_ERR("p_src_img->pxlfmt %d !=  VDO_PXLFMT_YUV422 or p_dst_img->pxlfmt %d !=  VDO_PXLFMT_YUV420\r\n",
					p_src_img->pxlfmt, p_dst_img->pxlfmt);
		return E_PAR;
	}
	if(!p_src_img->phyaddr[0] || !p_src_img->loff[0]){
		DBG_ERR("Invalid src y phyaddr(%lx) or loff(%d)\n", (unsigned long)p_src_img->phyaddr[0], p_src_img->loff[0]);
		return E_PAR;
	}
	if(!p_src_img->phyaddr[1] || !p_src_img->loff[1]){
		DBG_ERR("Invalid src uv phyaddr(%lx) or loff(%d)\n", (unsigned long)p_src_img->phyaddr[1], p_src_img->loff[1]);
		return E_PAR;
	}
	if(!p_dst_img->phyaddr[0] || !p_dst_img->loff[0]){
		DBG_ERR("Invalid dst y phyaddr(%lx) or loff(%d)\n", (unsigned long)p_dst_img->phyaddr[0], p_dst_img->loff[0]);
		return E_PAR;
	}
	if(!p_dst_img->phyaddr[1] || !p_dst_img->loff[1]){
		DBG_ERR("Invalid dst uv phyaddr(%lx) or loff(%d)\n", (unsigned long)p_dst_img->phyaddr[1], p_dst_img->loff[1]);
		return E_PAR;
	}
	if(p_src_img->size.w != p_dst_img->size.w || p_src_img->size.h != p_dst_img->size.h){
		DBG_ERR("src dim(%dx%d) must = dst dim(%dx%d)\n", p_src_img->size.w, p_dst_img->size.w, p_src_img->size.h, p_dst_img->size.h);
		return E_PAR;
	}

	//copy y plane
	gximg_memset(images, 0, sizeof(images));
	images[0].img_id     =    GRPH_IMG_ID_A;
	images[0].dram_addr  =    p_src_img->addr[0];
	images[0].lineoffset =    p_src_img->loff[0];
	images[0].width      =    p_src_img->size.w;
	images[0].height     =    p_src_img->size.h;
	images[0].p_next     =    &(images[1]);
	images[1].img_id     =    GRPH_IMG_ID_C;
	images[1].dram_addr  =    p_dst_img->addr[0];
	images[1].lineoffset =    p_dst_img->loff[0];
	images[1].width      =    p_dst_img->size.w;
	images[1].height     =    p_dst_img->size.h;
	images[1].p_next     =    NULL;

	grph_id = gximg_copy_map_to_grph_id(GXIMG_CP_ENG1);

	gximg_memset(&request, 0, sizeof(KDRV_GRPH_TRIGGER_PARAM));
	request.command  = GRPH_CMD_A_COPY;
	request.format   = GRPH_FORMAT_8BITS;
	request.p_images = &(images[0]);

	if(gximg_open_grph(grph_id) == -1){
		DBG_ERR("gximg_open_grph(%d) fail\n", grph_id);
		return E_PAR;
	}
	if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
		DBG_ERR("fail to start graphic engine(%d)\n", grph_id);
		gximg_close_grph(grph_id);
		return E_PAR;
	}
	if(gximg_close_grph(grph_id) == -1){
		DBG_ERR("gximg_close_grph(%d) fail\n", grph_id);
		return E_PAR;
	}

	//copy 1/2 uv plane by dropping odd-number lines
	images[0].dram_addr  =    p_src_img->addr[1];
	images[0].lineoffset =    p_src_img->loff[1]*2;
	images[0].width      =    p_src_img->size.w;
	images[0].height     =    p_src_img->size.h/2;
	images[1].dram_addr  =    p_dst_img->addr[1];
	images[1].lineoffset =    p_dst_img->loff[1];
	images[1].width      =    p_dst_img->size.w;
	images[1].height     =    p_dst_img->size.h/2;

	if(gximg_open_grph(grph_id) == -1){
		DBG_ERR("gximg_open_grph(%d) fail\n", grph_id);
		return E_PAR;
	}
	if(kdrv_grph_trigger(grph_id, &request, NULL, NULL) != E_OK){
		DBG_ERR("fail to start graphic engine(%d)\n", grph_id);
		gximg_close_grph(grph_id);
		return E_PAR;
	}
	if(gximg_close_grph(grph_id) == -1){
		DBG_ERR("gximg_close_grph(%d) fail\n", grph_id);
		return E_PAR;
	}

	return E_OK;
}

ER gximg_color_transform(PVDO_FRAME p_src_img, PVDO_FRAME p_dst_img, UINT32 tmp_buff, UINT32 tmp_buf_size, int usePA)
{
	if (!p_src_img) {
		DBG_ERR("p_src_img is Null\r\n");
		return E_PAR;
	}
	if (!p_dst_img) {
		DBG_ERR("p_dst_img is Null\r\n");
		return E_PAR;
	}
	if (p_src_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_src_img is not initialized\r\n");
		return E_PAR;
	}
	if (p_dst_img->sign != MAKEFOURCC('V','F','R','M')) {
		DBG_ERR("p_dst_img is not initialized\r\n");
		return E_PAR;
	}

	if (p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR && p_dst_img->pxlfmt == VDO_PXLFMT_YUV420) {
		return _420_planar_to_packed(p_src_img, p_dst_img, usePA);
	}else if (p_src_img->pxlfmt == VDO_PXLFMT_YUV420 && p_dst_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR) {
		return _420_packed_to_planar(p_src_img, p_dst_img, usePA);
#if USE_ISE
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_YUV444_PLANAR && p_dst_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR) {
		return _444_planar_to_420_planar(p_src_img, p_dst_img, usePA);
#endif
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_YUV422 && p_dst_img->pxlfmt == VDO_PXLFMT_YUV422_ONE) {
		//YUV422 Semi Plane to YUV422 YUYV
		return _422_packed_to_yuyv(p_src_img, p_dst_img, usePA);
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_YUV420 && p_dst_img->pxlfmt == VDO_PXLFMT_YUV422_ONE) {
		//YUV420 Semi Plane to YUV422 YUYV
		return _420_packed_to_yuyv(p_src_img, p_dst_img, tmp_buff, tmp_buf_size, usePA);
	} else if (p_src_img->pxlfmt == VDO_PXLFMT_YUV422 && p_dst_img->pxlfmt == VDO_PXLFMT_YUV420) {
		return _422_to_420(p_src_img, p_dst_img);
	}

	DBG_ERR("p_src_img->pxlfmt = %d, p_dst_img->pxlfmt=%x\r\n", p_src_img->pxlfmt, p_dst_img->pxlfmt);
	return E_PAR;
}

ER gximg_quad_cover_with_flush(GXIMG_COVER_DESC *p_cover, PVDO_FRAME p_dst_img, IRECT *p_dst_region, UINT32 hollow, UINT32 thickness, int no_flush, int usePA)
{
	IRECT  dst_region = {0}, aligned_dst_region = {0}, tmp_plane_rect = {0};
	UINT32 aligned_width = 4;
	int    min_x, max_x, min_y, max_y;
	IPOINT top_left, top_right, bottom_right, bottom_left;

	if (NULL == p_cover) {
		DBG_ERR("p_cover is NULL\r\n");
		return E_PAR;
	}

	if (NULL == p_dst_img) {
		DBG_ERR("p_dst_img is Null\r\n");
		return E_PAR;
	}

	if (p_dst_img->pxlfmt != VDO_PXLFMT_YUV422 &&
		p_dst_img->pxlfmt != VDO_PXLFMT_YUV420) {
		DBG_ERR("Invalid dst fmt 0x%X\r\n", p_dst_img->pxlfmt);
		return E_PAR;
	}


	gximg_memcpy(&top_left    , &p_cover->top_left    , sizeof(IPOINT));
	gximg_memcpy(&top_right   , &p_cover->top_right   , sizeof(IPOINT));
	gximg_memcpy(&bottom_left , &p_cover->bottom_left , sizeof(IPOINT));
	gximg_memcpy(&bottom_right, &p_cover->bottom_right, sizeof(IPOINT));

	min_x = p_cover->top_left.x;
	if(p_cover->top_right.x < min_x){
		min_x = p_cover->top_right.x;
	}
	if(p_cover->bottom_left.x < min_x){
		min_x = p_cover->bottom_left.x;
	}
	if(p_cover->bottom_right.x < min_x){
		min_x = p_cover->bottom_right.x;
	}
	max_x = p_cover->top_left.x;
	if(p_cover->top_right.x > max_x){
		max_x = p_cover->top_right.x;
	}
	if(p_cover->bottom_left.x > max_x){
		max_x = p_cover->bottom_left.x;
	}
	if(p_cover->bottom_right.x > max_x){
		max_x = p_cover->bottom_right.x;
	}
	min_y = p_cover->top_left.y;
	if(p_cover->top_right.y < min_y){
		min_y = p_cover->top_right.y;
	}
	if(p_cover->bottom_left.y < min_y){
		min_y = p_cover->bottom_left.y;
	}
	if(p_cover->bottom_right.y < min_y){
		min_y = p_cover->bottom_right.y;
	}
	max_y = p_cover->top_left.y;
	if(p_cover->top_right.y > max_y){
		max_y = p_cover->top_right.y;
	}
	if(p_cover->bottom_left.y > max_y){
		max_y = p_cover->bottom_left.y;
	}
	if(p_cover->bottom_right.y > max_y){
		max_y = p_cover->bottom_right.y;
	}

	dst_region.x = min_x;
	dst_region.y = min_y;
	dst_region.w = (max_x - min_x);
	dst_region.h = (max_y - min_y);

	if ((dst_region.x + (INT32)dst_region.w) <= 0) {
		return E_OK;
	} else if (dst_region.x >= (INT32)p_dst_img->size.w) {
		return E_OK;
	} else if (dst_region.x < 0) {
		aligned_dst_region.x = 0;
		aligned_dst_region.w = (dst_region.x + dst_region.w);
		if(aligned_dst_region.w > (INT32)p_dst_img->size.w){
			aligned_dst_region.w = (INT32)p_dst_img->size.w;
		}
	} else {
		aligned_dst_region.x = (dst_region.x - (dst_region.x & (aligned_width - 1)));
		aligned_dst_region.w = (dst_region.w + (dst_region.x & (aligned_width - 1)));
		if((aligned_dst_region.x + aligned_dst_region.w) > (INT32)p_dst_img->size.w){
			aligned_dst_region.w = (INT32)p_dst_img->size.w - aligned_dst_region.x;
		}
		dst_region.x = (dst_region.x & (aligned_width - 1));
		top_left.x     -= (min_x - dst_region.x);
		top_right.x    -= (min_x - dst_region.x);
		bottom_left.x  -= (min_x - dst_region.x);
		bottom_right.x -= (min_x - dst_region.x);
	}

	if(aligned_dst_region.w & (aligned_width - 1)){
		aligned_dst_region.w += (aligned_width - (aligned_dst_region.w & (aligned_width - 1)));
	}

	if ((dst_region.y + (INT32)dst_region.h) <= 0) {
		return E_OK;
	} else if (dst_region.y >= (INT32)p_dst_img->size.h) {
		return E_OK;
	} else if (dst_region.y < 0) {
		aligned_dst_region.y = 0;
		aligned_dst_region.h = (dst_region.y + dst_region.h);
		if(aligned_dst_region.h > (INT32)p_dst_img->size.h){
			aligned_dst_region.h = (INT32)p_dst_img->size.h;
		}
	} else {
		aligned_dst_region.y = (dst_region.y - (dst_region.y & 0x01));
		aligned_dst_region.h = (dst_region.h + (dst_region.y & 0x01));
		if((aligned_dst_region.y + aligned_dst_region.h) > (INT32)p_dst_img->size.h){
			aligned_dst_region.h = (INT32)p_dst_img->size.h - aligned_dst_region.y;
		}
		dst_region.y = (dst_region.y & 0x01);
		top_left.y     -= (min_y - dst_region.y);
		top_right.y    -= (min_y - dst_region.y);
		bottom_left.y  -= (min_y - dst_region.y);
		bottom_right.y -= (min_y - dst_region.y);
	}

	if(aligned_dst_region.h & 0x01){
		aligned_dst_region.h += (2 - (aligned_dst_region.h & 0x01));
	}

	if (aligned_dst_region.x < 0) {
		DBG_ERR("Invalid aligned rect x(%d), must >= 0\r\n", aligned_dst_region.x);
		return E_PAR;
	}
	if ((aligned_dst_region.x + (INT32)aligned_dst_region.w) > (INT32)p_dst_img->size.w) {
		DBG_ERR("Invalid aligned rect x(%d)+w(%d), must <= background w(%d)\r\n", aligned_dst_region.x, aligned_dst_region.w, p_dst_img->size.w);
		return E_PAR;
	}
	if (aligned_dst_region.y < 0) {
		DBG_ERR("Invalid aligned rect y(%d), must >= 0\r\n", aligned_dst_region.y);
		return E_PAR;
	}
	if ((aligned_dst_region.y + (INT32)aligned_dst_region.h) > (INT32)p_dst_img->size.h) {
		DBG_ERR("Invalid aligned rect y(%d)+h(%d), must <= background h(%d)\r\n", aligned_dst_region.y, aligned_dst_region.h, p_dst_img->size.h);
		return E_PAR;
	}

#if USE_GRPH
	{
		GRPH_QUAD_DESC quad_desc = {0};
		GRPH_PROPERTY property[5];
		KDRV_GRPH_TRIGGER_PARAM request = {0};
		GRPH_IMG image_a = {0};
		GRPH_IMG image_b = {0};
		GRPH_IMG image_c = {0};
		UINT32 grph_id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH1, 0);
		BOOL use_mosaic; //mosaic or constant value
		BOOL is_yuv420; //1: 420, 0: 422
		GRPH_QUAD_DESC inner_quad;

		//hollow only supports rectangle, not quad
		if(hollow){
			if(p_cover->top_right.x <= p_cover->top_left.x ||
			   p_cover->top_left.x != p_cover->bottom_left.x ||
			   p_cover->bottom_right.y <= p_cover->top_right.y ||
			   p_cover->bottom_right.y != p_cover->bottom_left.y){
					DBG_ERR("hollow quad only supports rectangle. point[0] should be top left corner\n");
					return E_PAR;
			}
			if(p_cover->top_left.x + 2*(INT32)thickness >= p_cover->top_right.x ||
			   p_cover->top_right.y + 2*(INT32)thickness >= p_cover->bottom_right.y){
					DBG_ERR("hollow quad is over thick(%d)\n", thickness);
					return E_PAR;
			}
			inner_quad.top_left.x = top_left.x + thickness;
			inner_quad.top_left.y = top_left.y + thickness;
			inner_quad.top_right.x = top_right.x - thickness;
			inner_quad.top_right.y = top_right.y + thickness;
			inner_quad.bottom_right.x = bottom_right.x - thickness;
			inner_quad.bottom_right.y = bottom_right.y - thickness;
			inner_quad.bottom_left.x = bottom_left.x + thickness;
			inner_quad.bottom_left.y = bottom_left.y - thickness;
			inner_quad.blend_en = TRUE;
			inner_quad.alpha = COLOR_YUVA_GET_A(p_cover->yuva);
			inner_quad.mosaic_width = 0;
			inner_quad.mosaic_height = 0;
		}

		gximg_memset(property, 0, sizeof(property));

		is_yuv420 = (p_dst_img->pxlfmt == VDO_PXLFMT_YUV420) ? 1 : 0;
		use_mosaic = (0 == p_cover->yuva);

		property[0].id = GRPH_PROPERTY_ID_YUVFMT;
		property[0].property = is_yuv420;
		property[0].p_next = &(property[1]);

		property[1].id = GRPH_PROPERTY_ID_QUAD_PTR;
		property[1].property = (UINT32)&quad_desc;

		quad_desc.top_left.x = top_left.x;
		quad_desc.top_left.y = top_left.y;
		quad_desc.top_right.x = top_right.x;
		quad_desc.top_right.y = top_right.y;
		quad_desc.bottom_right.x = bottom_right.x;
		quad_desc.bottom_right.y = bottom_right.y;
		quad_desc.bottom_left.x = bottom_left.x;
		quad_desc.bottom_left.y = bottom_left.y;

		request.command = GRPH_CMD_VCOV;
		request.p_images = &image_a;
		request.p_property = &property[0];
		request.is_skip_cache_flush = check_no_flush(usePA, no_flush);
		request.is_buf_pa = usePA;

		//Y plane ---------------------------------
		if (use_mosaic) {
			property[1].p_next = NULL;
		} else {
			property[1].p_next = &(property[2]);
			property[2].id = GRPH_PROPERTY_ID_NORMAL;
			property[2].property = COLOR_YUVA_GET_Y(p_cover->yuva);

			if(hollow){
				property[2].p_next = &(property[3]);
				property[3].id = GRPH_PROPERTY_ID_QUAD_INNER_PTR;
				property[3].property = (UINT32)&inner_quad;
				property[3].p_next = NULL;
			}else
				property[2].p_next = NULL;
		}

		if (use_mosaic) {
			quad_desc.blend_en = FALSE;
			quad_desc.alpha = 0;
			quad_desc.mosaic_width = p_cover->mosaic_blk_size.w;
			quad_desc.mosaic_height = p_cover->mosaic_blk_size.h;
		} else {
			quad_desc.blend_en = TRUE;
			quad_desc.alpha = COLOR_YUVA_GET_A(p_cover->yuva);
			quad_desc.mosaic_width = 0;
			quad_desc.mosaic_height = 0;
		}

		gximg_calc_plane_rect(p_dst_img, 0, &aligned_dst_region, &tmp_plane_rect);

		image_a.img_id = GRPH_IMG_ID_A; //src
		image_a.dram_addr = (UINT32)p_dst_img->addr[0] + gximg_calc_plane_offset(p_dst_img, 0, &tmp_plane_rect);
		image_a.lineoffset = p_dst_img->loff[0];
		image_a.width = tmp_plane_rect.w;
		image_a.height = tmp_plane_rect.h;
		image_a.p_next = &image_c;

		image_c.img_id = GRPH_IMG_ID_C; //dst
		image_c.dram_addr = (UINT32)p_dst_img->addr[0] + gximg_calc_plane_offset(p_dst_img, 0, &tmp_plane_rect);
		image_c.lineoffset = p_dst_img->loff[0];
		image_c.p_next = NULL;

		if (use_mosaic && p_cover->p_mosaic_img) {
			image_c.p_next = &image_b;

			image_b.img_id = GRPH_IMG_ID_B; //mosaic image
			image_b.dram_addr = p_cover->p_mosaic_img->addr[0];
			image_b.lineoffset = p_cover->p_mosaic_img->loff[0];
			image_b.p_next = NULL;
		} else {
			image_c.p_next = NULL;
		}

		request.format = GRPH_FORMAT_8BITS; //Y plane

		if(use_mosaic && p_cover->p_mosaic_img){
			if(gximg_check_grph_limit(request.command, request.format, &image_a, 1, GRPH_IMG_ID_A) ||
				gximg_check_grph_limit(request.command, request.format, &image_b, 0, GRPH_IMG_ID_B) ||
				gximg_check_grph_limit(request.command, request.format, &image_c, 0, GRPH_IMG_ID_C)){
				DBG_ERR("limit check fail\n");
				return E_PAR;
			}
		}else{
			if(gximg_check_grph_limit(request.command, request.format, &image_a, 1, GRPH_IMG_ID_A) ||
				gximg_check_grph_limit(request.command, request.format, &image_c, 0, GRPH_IMG_ID_C)){
				DBG_ERR("limit check fail\n");
				return E_PAR;
			}
		}

		if (E_OK != gximg_open_grph(grph_id)) {
			DBG_ERR("gximg_open_grph\r\n");
			return E_SYS;
		}

		if (E_OK != kdrv_grph_trigger(grph_id, &request, NULL, NULL)) {
			DBG_ERR("kdrv_grph_trigger for Y\r\n");

			if (E_OK != gximg_close_grph(grph_id)) {
				DBG_ERR("gximg_close_grph\r\n");
			}

			return E_SYS;
		}

		//UV plane ---------------------------------
		if(hollow){
			inner_quad.blend_en = TRUE;
			inner_quad.alpha = COLOR_YUVA_GET_A(p_cover->yuva);
			inner_quad.mosaic_width = 0;
			inner_quad.mosaic_height = 0;
		}

		if (use_mosaic) {
			property[1].p_next = NULL;
		} else {
			property[1].p_next = &(property[2]);

			property[2].id = GRPH_PROPERTY_ID_U;
			property[2].property = COLOR_YUVA_GET_U(p_cover->yuva);
			property[2].p_next = &(property[3]);

			property[3].id = GRPH_PROPERTY_ID_V;
			property[3].property = COLOR_YUVA_GET_V(p_cover->yuva);

			if(hollow){
				property[3].p_next = &(property[4]);
				property[4].id = GRPH_PROPERTY_ID_QUAD_INNER_PTR;
				property[4].property = (UINT32)&inner_quad;
				property[4].p_next = NULL;
			}else
				property[3].p_next = NULL;
		}

		if (use_mosaic) {
			//NOTE:
			//The smallest 8x8 mosaic operation, Y is (8x8)*2 paired with UV_packed (16x8)*1
			//Y (8x8)*2 will become (1x1)*2, and UV_packed (16x8)*1 will become (2x1)*1
			//HW will map two Y pixels to one UV_packed pixel in horizontal,
			//  map one Y pixel to one UV_packed pixel in vertical for YUV422
			//  map two Y pixels to one UV_packed pixel in vertical for YUV420
			//So YUV420 and YUV422 should set the same setting and let HW to handle the mapping.
			quad_desc.blend_en = FALSE;
			quad_desc.alpha = 0;
			quad_desc.mosaic_width = p_cover->mosaic_blk_size.w << 1;
			quad_desc.mosaic_height = p_cover->mosaic_blk_size.h;
		} else {
			quad_desc.blend_en = TRUE;
			quad_desc.alpha = COLOR_YUVA_GET_A(p_cover->yuva);
			quad_desc.mosaic_width = 0;
			quad_desc.mosaic_height = 0;
		}

		gximg_calc_plane_rect(p_dst_img, 1, &aligned_dst_region, &tmp_plane_rect);

		image_a.img_id = GRPH_IMG_ID_A; //src
		image_a.dram_addr = (UINT32)p_dst_img->addr[1] + gximg_calc_plane_offset(p_dst_img, 1, &tmp_plane_rect);
		image_a.lineoffset = p_dst_img->loff[1];
		image_a.width = tmp_plane_rect.w;
		image_a.height = tmp_plane_rect.h;
		image_a.p_next = &image_c;

		image_c.img_id = GRPH_IMG_ID_C; //dst
		image_c.dram_addr = (UINT32)p_dst_img->addr[1] + gximg_calc_plane_offset(p_dst_img, 1, &tmp_plane_rect);
		image_c.lineoffset = p_dst_img->loff[1];
		image_c.p_next = NULL;

		if (use_mosaic && p_cover->p_mosaic_img) {
			image_c.p_next = &image_b;

			image_b.img_id = GRPH_IMG_ID_B; //mosaic image
			image_b.dram_addr = p_cover->p_mosaic_img->addr[1];
			image_b.lineoffset = p_cover->p_mosaic_img->loff[1];
			image_b.p_next = NULL;
		} else {
			image_c.p_next = NULL;
		}

		request.format = GRPH_FORMAT_16BITS; //UV plane

		if(use_mosaic && p_cover->p_mosaic_img){
			if(gximg_check_grph_limit(request.command, request.format, &image_a, 1, GRPH_IMG_ID_A) ||
				gximg_check_grph_limit(request.command, request.format, &image_b, 0, GRPH_IMG_ID_B) ||
				gximg_check_grph_limit(request.command, request.format, &image_c, 0, GRPH_IMG_ID_C)){
				DBG_ERR("limit check fail\n");
				return E_PAR;
			}
		}else{
			if(gximg_check_grph_limit(request.command, request.format, &image_a, 1, GRPH_IMG_ID_A) ||
				gximg_check_grph_limit(request.command, request.format, &image_c, 0, GRPH_IMG_ID_C)){
				DBG_ERR("limit check fail\n");
				return E_PAR;
			}
		}

		if (E_OK != kdrv_grph_trigger(grph_id, &request, NULL, NULL)) {
			DBG_ERR("kdrv_grph_trigger for UV\r\n");

			if (E_OK != gximg_close_grph(grph_id)) {
				DBG_ERR("gximg_close_grph\r\n");
			}

			return E_SYS;
		}

		if (E_OK != gximg_close_grph(grph_id)) {
			DBG_ERR("gximg_close_grph\r\n");
			return E_SYS;
		}
	}
	return E_OK;
#else
	DBG_FUNC_END("[copy]\r\n");
	return E_NOSPT;
#endif

}

ER gximg_quad_cover(GXIMG_COVER_DESC *p_cover, PVDO_FRAME p_dst_img, IRECT *p_dst_region, UINT32 hollow, UINT32 thickness, int usePA){
	return gximg_quad_cover_with_flush(p_cover, p_dst_img, p_dst_region, hollow, thickness, 0, usePA);
}

ER gximg_quad_cover_no_flush(GXIMG_COVER_DESC *p_cover, PVDO_FRAME p_dst_img, IRECT *p_dst_region, UINT32 hollow, UINT32 thickness){
	return gximg_quad_cover_with_flush(p_cover, p_dst_img, p_dst_region, hollow, thickness, 1, 0);
}

ER gximg_scale_config_mode(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region, KDRV_ISE_MODE *mode, int *count, int usePA)
{
	IRECT     src_rect = {0};
	IRECT     dst_rect = {0};
	ER        result = E_OK;

	if(!p_src_img || !p_src_region || !p_dst_img || !p_dst_region || !mode || !count){
		DBG_ERR("invalid src_img(%p) or src_rgn(%p) or dst_img(%p) or dst_rgn(%p) or mode(%p) or count(%p)\r\n",
				p_src_img, p_src_region, p_dst_img, p_dst_region, mode, count);
		return E_SYS;
	}

	result = gximg_scale_adjust_rect(p_src_img, p_src_region, p_dst_img, p_dst_region, &src_rect, &dst_rect);
	if (result == E_OK)
#if USE_ISE
	{
//		return gximg_scale_by_ise(p_src_img, &src_rect, p_dst_img, &dst_rect, engine);
		IRECT         tmp_src_plane_rect = {0}, tmp_dst_plane_rect = {0};
		UINT32        in_loff[GXIMG_MAX_PLANE_NUM] = {0}, out_loff[GXIMG_MAX_PLANE_NUM] = {0}, i, scale_method, max_scale_factor, loff, uv_addr_align;

		if (p_src_img->pxlfmt != p_dst_img->pxlfmt) {
			DBG_ERR("Src fmt %d != Dst fmt %d\r\n", p_src_img->pxlfmt, p_dst_img->pxlfmt);
			return E_PAR;
		}

		switch (p_src_img->pxlfmt) {
		case VDO_PXLFMT_YUV420_PLANAR:
			*count = 3;
			break;
		case VDO_PXLFMT_YUV422:
		case VDO_PXLFMT_YUV420:
		case VDO_PXLFMT_ARGB8565:
			*count = 2;
			break;
		case VDO_PXLFMT_Y8:
		case VDO_PXLFMT_I8:
		case VDO_PXLFMT_ARGB1555:
		case VDO_PXLFMT_ARGB4444:
		case VDO_PXLFMT_RGB565:
			*count = 1;
			break;
		default:
			DBG_ERR("src pixel format %x\r\n", p_src_img->pxlfmt);
			return E_PAR;
		}

		for (i = 0; (int)i < *count; i++) {
			gximg_calc_plane_rect(p_src_img, i, &src_rect, &tmp_src_plane_rect);
			in_loff[i] = gximg_calc_plane_offset(p_src_img, i, &tmp_src_plane_rect);
		}

		for (i = 0; (int)i < *count; i++) {
			gximg_calc_plane_rect(p_dst_img, i, &dst_rect, &tmp_dst_plane_rect);
			out_loff[i] = gximg_calc_plane_offset(p_dst_img, i, &tmp_dst_plane_rect);
		}
		// check ISE alignment begin
		max_scale_factor = 16;
		uv_addr_align = 2;
		loff = 4;

		if (src_rect.w >= ((INT32)max_scale_factor * dst_rect.w) || src_rect.h >= ((INT32)max_scale_factor * dst_rect.h) || dst_rect.w >= ((INT32)max_scale_factor *src_rect.w) || dst_rect.h >= ((INT32)max_scale_factor * src_rect.h)) {
			DBG_ERR("scale factor over %d, SrcW=%d,SrcH=%d,DstW=%d,DstH=%d\r\n", max_scale_factor, src_rect.w, src_rect.h, dst_rect.w, dst_rect.h);
			return E_PAR;
		} else if ((p_src_img->addr[1] + in_loff[1]) % uv_addr_align) {
			DBG_WRN("In UV Addr not %d byte aligned, auto truncate\r\n", uv_addr_align);
			in_loff[1] &= (~(uv_addr_align - 1));
		} else if ((p_dst_img->addr[1] + out_loff[1]) % uv_addr_align) {
			DBG_WRN("Out UV Addr not %d byte aligned, auto truncate\r\n", uv_addr_align);
			out_loff[1] &= (~(uv_addr_align - 1));
		} else if ((p_src_img->loff[0] % loff) || (p_src_img->loff[1] % loff)) {
			DBG_ERR("In lineoff not %d byte aligned\r\n", loff);
			return E_PAR;
		} else if ((p_dst_img->loff[0] % loff) || (p_dst_img->loff[1] % loff)) {
			DBG_ERR("Out lineoff not %d byte aligned\r\n", loff);
			return E_PAR;
		}
		// check ISE alignment end

		 // choose scale method
		 if (g_scale_method == GXIMG_SCALE_AUTO) {
			 if ((src_rect.w < dst_rect.w * 11) && (src_rect.h < dst_rect.h * 11) && (src_rect.w > dst_rect.w * 2) && (src_rect.h > dst_rect.h * 2)) {
				 scale_method = KDRV_ISE_INTEGRATION;
				// DBG_MSG("[scale]INTEGRATION\r\n");
			 } else {
				 scale_method = KDRV_ISE_BILINEAR;
				// DBG_MSG("[scale]BILINEAR\r\n");
			 }
		 } else {
			 scale_method = gximg_map_to_ise_scale_method(g_scale_method);
			 //GxImg_ResetParm();
		 }
		//scale_method = KDRV_ISE_BILINEAR;

		switch (p_dst_img->pxlfmt) {
		case VDO_PXLFMT_ARGB1555:
			mode[0].io_pack_fmt = KDRV_ISE_ARGB1555;
			break;
		case VDO_PXLFMT_ARGB4444:
			mode[0].io_pack_fmt = KDRV_ISE_ARGB4444;
			break;
		case VDO_PXLFMT_RGB565:
			mode[0].io_pack_fmt = KDRV_ISE_RGB565;
			break;
		default:
			mode[0].io_pack_fmt = KDRV_ISE_Y8;
		}
		mode[0].scl_method = scale_method;
		mode[0].in_buf_flush = check_no_flush(usePA, 0);
		mode[0].out_buf_flush = check_no_flush(usePA, 0);
		mode[0].phy_addr_en = usePA;
		mode[0].in_width   = src_rect.w;
		mode[0].in_height  = src_rect.h;
		mode[0].in_lofs    = p_src_img->loff[0];
		mode[0].in_addr    = p_src_img->addr[0] + in_loff[0];
		mode[0].out_width  = dst_rect.w;
		mode[0].out_height = dst_rect.h;
		mode[0].out_lofs   = p_dst_img->loff[0];
		mode[0].out_addr   = p_dst_img->addr[0] + out_loff[0];

		if (gximg_check_ise_limit(&(mode[0]))) {
			DBG_ERR("checking ise limit failed\r\n");
			return E_PAR;
		}

		for(i = 1 ; (int)i < *count ; ++i){
			if (*count > 1) {
				gximg_calc_plane_rect(p_src_img, i, &src_rect, &tmp_src_plane_rect);
				gximg_calc_plane_rect(p_dst_img, i, &dst_rect, &tmp_dst_plane_rect);

				if (tmp_src_plane_rect.w >= (INT32)max_scale_factor * tmp_dst_plane_rect.w ||
					tmp_src_plane_rect.h >= (INT32)max_scale_factor * tmp_dst_plane_rect.h ||
					tmp_dst_plane_rect.w >= (INT32)max_scale_factor * tmp_src_plane_rect.w ||
					tmp_dst_plane_rect.h >= (INT32)max_scale_factor * tmp_src_plane_rect.h) {
					DBG_ERR("Plane%d scale over %d, SrcW=%d,SrcH=%d,DstW=%d,DstH=%d\r\n",
							i, max_scale_factor, tmp_src_plane_rect.w, tmp_src_plane_rect.h, tmp_dst_plane_rect.w, tmp_dst_plane_rect.h);
					return E_PAR;
				}

				if (p_src_img->pxlfmt == VDO_PXLFMT_ARGB8565) {
					mode[i].io_pack_fmt = KDRV_ISE_RGB565;
				} else if(p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR){
					//ise_op.io_pack_fmt = KDRV_ISE_Y4;
					mode[i].io_pack_fmt = KDRV_ISE_Y8;
				} else {
					mode[i].io_pack_fmt = KDRV_ISE_UVP;
				}
				mode[i].scl_method = scale_method;
				if(p_src_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR)
					mode[i].in_width = tmp_src_plane_rect.w;
				else
					// UV packed the UV width should be the half of Y width
					mode[i].in_width = tmp_src_plane_rect.w >> 1;
				mode[i].in_height = tmp_src_plane_rect.h;
				mode[i].in_lofs = p_src_img->loff[i];
				mode[i].in_addr = p_src_img->addr[i] + in_loff[i];
				if(p_dst_img->pxlfmt == VDO_PXLFMT_YUV420_PLANAR)
					mode[i].out_width = tmp_dst_plane_rect.w;
				else
					// UV packed the UV width should be the half of Y width
					mode[i].out_width = tmp_dst_plane_rect.w >> 1;
				mode[i].out_height = tmp_dst_plane_rect.h;
				mode[i].out_lofs = p_dst_img->loff[i];
				mode[i].out_addr = p_dst_img->addr[i] + out_loff[i];

				if (gximg_check_ise_limit(&(mode[i]))) {
					DBG_ERR("checking ise limit failed\r\n");
					return E_PAR;
				}
			}
		}
		return E_OK;
	}else
#endif
		DBG_ERR("fail\n");
	return result;
}

ER gximg_raw_graphic(UINT32 engine, void *p_request)
{
	ER        result = E_OK;
	UINT32    id     = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0);

	if(engine == 0)
		id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0);
	else if(engine == 1)
		id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH1, 0);
	else{
		DBG_ERR("engine(%d) is not supported\n", engine);
		return E_PAR;
	}

	if(!p_request){
		DBG_ERR("p_request is NULL\n");
		return E_PAR;
	}

	if (E_OK != gximg_open_grph(id)) {
		DBG_ERR("gximg_open_grph\r\n");
		return E_SYS;
	}

	if (E_OK != kdrv_grph_trigger(id, (KDRV_GRPH_TRIGGER_PARAM*)p_request, NULL, NULL)) {
		DBG_ERR("kdrv_grph_trigger fail\r\n");
		result = E_SYS;
	}

	if (E_OK != gximg_close_grph(id))
		DBG_ERR("gximg_close_grph\r\n");

	return result;
}

int gximage_init(void)
{
	dynamic_clock = 1;
	return 0;
}

void gximage_exit(void)
{
}

int gximage_set_dynamic_clock(int dynamic)
{
	UINT32 grph_id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0);

	if(gximg_open_grph(grph_id) == -1){
		DBG_ERR("gximg_open_grph(%d) fail\n", grph_id);
		return E_PAR;
	}

	grph_id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH1, 0);

	if(gximg_open_grph(grph_id) == -1){
		DBG_ERR("gximg_open_grph(%d) fail\n", grph_id);
		return E_PAR;
	}

	dynamic_clock = 0;
	DBG_DUMP("gximage disables dynamic clock\n");

	return 0;
}

#if defined(__LINUX)
#else
void gximg_memset(void *buf, unsigned char val, int len)
{
	memset(buf, val, len);
}

void gximg_memcpy(void *buf, void *src, int len)
{
	memcpy(buf, src, len);
}
#endif
