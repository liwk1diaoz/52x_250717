#include <stdio.h>
#include "ImageApp/ImageApp_UsbMovie.h"
#include "hdal.h"
#include "PrjInc.h"

//#define ENABLE      1
//#define DISABLE     0

#define VDO_SIZE_W                  1920
#define VDO_SIZE_H                  1080

#define VDO_MAIN_SIZE_W             1920
#define VDO_MAIN_SIZE_H             1080

#define SEN_OUT_FMT         HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT         HD_VIDEO_PXLFMT_RAW12
#define DBGINFO_BUFSIZE()	(0x200)
#define CA_WIN_NUM_W        32
#define CA_WIN_NUM_H        32
#define LA_WIN_NUM_W        32
#define LA_WIN_NUM_H        32
#define VA_WIN_NUM_W        16
#define VA_WIN_NUM_H        16
#define YOUT_WIN_NUM_W      128
#define YOUT_WIN_NUM_H      128
#define ETH_8BIT_SEL        0 //0: 2bit out, 1:8 bit out
#define ETH_OUT_SEL         1 //0: full, 1: subsample 1/2

///////////////////////////////////////////////////////////////////////////////

//header
#define DBGINFO_BUFSIZE()	(0x200)

//RAW
#define VDO_RAW_BUFSIZE(w, h, pxlfmt)   (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NRX: RAW compress: Only support 12bit mode
#define RAW_COMPRESS_RATIO 59
#define VDO_NRX_BUFSIZE(w, h)           (ALIGN_CEIL_4(ALIGN_CEIL_64(w) * 12 / 8 * RAW_COMPRESS_RATIO / 100 * (h)))
//CA for AWB
#define VDO_CA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 3) << 1)
//LA for AE
#define VDO_LA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 1) << 1)

//YUV
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	(ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NVX: YUV compress
#define YUV_COMPRESS_RATIO 75
#define VDO_NVX_BUFSIZE(w, h, pxlfmt)	(VDO_YUV_BUFSIZE(w, h, pxlfmt) * YUV_COMPRESS_RATIO / 100)
 
///////////////////////////////////////////////////////////////////////////////


static HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

void UsbMovie_CommPoolInit(void)
{
	UINT32 id;

	// config common pool (cap)
	id = 0;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() +
									VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, CAP_OUT_FMT) +
									VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H) +
									VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	mem_cfg.pool_info[id].blk_cnt = 3;
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;

	// config common pool (main)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE() + VDO_YUV_BUFSIZE(VDO_MAIN_SIZE_W, VDO_MAIN_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_cnt = 6; //3 for 3dnr-off, 4 for 3dnr-on
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;

	ImageApp_UsbMovie_Config(USBMOVIE_CFG_MEM_POOL_INFO, (UINT32)&mem_cfg);
}

