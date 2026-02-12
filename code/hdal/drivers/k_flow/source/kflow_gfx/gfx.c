#if defined(__LINUX)
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "kwrap/cmdsys.h"
#endif

#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/spinlock.h"
#include "kflow.h"
#include "kflow_common/nvtmpp.h"
#include "kdrv_builtin/nvtmpp_init.h"
#include <gximage/gximage.h>
#include <gximage/gximage.h>
#include <gximage/hd_gximage.h>
#include <gximage/gfx_internal.h>
#include <gximage/gfx_open.h>
#include "kdrv_gfx2d/kdrv_grph.h"
#include "kdrv_gfx2d/kdrv_ise.h"
#include "kdrv_gfx2d/kdrv_gfx_if.h"
#include "comm/hwclock.h"
#include "kflow_common/isf_flow_def.h"
#include "kwrap/cpu.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          gfx
#define __DBGLVL__          NVT_DBG_WRN
#include "kwrap/debug.h"
unsigned int gfx_debug_level = __DBGLVL__;
///////////////////////////////////////////////////////////////////////////////

#define USE_HWCOPY    0

#if USE_HWCOPY
#include <kdrv_gfx2d/kdrv_hwcopy.h>
#endif

#ifdef CONFIG_NVT_SMALL_HDAL
#define MAX_RECORD 16
#else
#define MAX_RECORD 128
#endif

typedef struct _RECORD {
	int           cur;
	int           pid[MAX_RECORD];
	GFX_USER_CMD  cmd[MAX_RECORD];
	int           result[MAX_RECORD];
	UINT64        time[MAX_RECORD];
	int           p1[MAX_RECORD];
	int           p2[MAX_RECORD];
	int           p3[MAX_RECORD];
	int           p4[MAX_RECORD];
} RECORD;

typedef struct _SCALE_LINK_LIST_BUF {
	UINT32    phy_addr;
	UINT32    virt_addr;
	UINT32    length;
} SCALE_LINK_LIST_BUF;

typedef struct{
	int start_x;
	int start_y;
	int end_x;
	int end_y;
	int linewidth;
	int linecolor[4];   // linecolor[0]: A, linecolor[1]: R, linecolor[2]: G, linecolor[3]: B for argb4444
	                    // linecolor[0]: R, linecolor[1]: G, linecolor[2]: B for 420
	int draw_method;
} DRAW_LINE_SETTING;

RECORD record;

//SEM_HANDLE GFX_SEM_ID = {0};
SEM_HANDLE GRAPH1_SEM_ID = {0};
SEM_HANDLE GRAPH2_SEM_ID = {0};
SEM_HANDLE SCALE_SEM_ID  = {0};
static VK_DEFINE_SPINLOCK(RECORD_LOCK_ID);

SEM_HANDLE GFX_JOB_SEM_ID = {0};

static GFX_USER_DATA  *job_buf = NULL;

static SCALE_LINK_LIST_BUF scale_ll_buf = {0};

#define MAX_ISE_MODE_SIZE (MAX_JOB_IN_A_LIST*2+1+3)
static KDRV_ISE_MODE ise_mode[MAX_ISE_MODE_SIZE] = {0};

static int gfx_init_count = 0;
static int force_use_physical_address = 0;

static INT32 (*gfx_if_affine_open)(UINT32 chip, UINT32 engine) = NULL;
static INT32 (*gfx_if_affine_trigger)(UINT32 id, KDRV_AFFINE_TRIGGER_PARAM *p_param,
						  KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data) = NULL;
static INT32 (*gfx_if_affine_close)(UINT32 chip, UINT32 engine) = NULL;

//-----------------------------------------------------------------------------
// API
//-----------------------------------------------------------------------------

extern int gximg_check_ise_limit(KDRV_ISE_MODE *ise_op);
extern int gximg_check_grph_limit(GRPH_CMD cmd, UINT32 bits, GRPH_IMG *image, int check_wh);

//static void gfx_api_lock(void)
//{
	//SEM_WAIT(GFX_SEM_ID);
//}

//static void gfx_api_unlock(void)
//{
	//SEM_SIGNAL(GFX_SEM_ID);
//}

static void gfx_graph_lock(GFX_GRPH_ENGINE engine)
{
	if(engine == GFX_GRPH_ENGINE_2)
		SEM_WAIT(GRAPH2_SEM_ID);
	else
		SEM_WAIT(GRAPH1_SEM_ID);
}
static void gfx_graph_unlock(GFX_GRPH_ENGINE engine)
{
	if(engine == GFX_GRPH_ENGINE_2)
		SEM_SIGNAL(GRAPH2_SEM_ID);
	else
		SEM_SIGNAL(GRAPH1_SEM_ID);
}
static void gfx_scale_lock(void)
{
	SEM_WAIT(SCALE_SEM_ID);
}
static void gfx_scale_unlock(void)
{
	SEM_SIGNAL(SCALE_SEM_ID);
}
static void gfx_all_lock(void)
{
	gfx_graph_lock(GFX_GRPH_ENGINE_1);
	gfx_graph_lock(GFX_GRPH_ENGINE_2);
	gfx_scale_lock();
}
static void gfx_all_unlock(void)
{
	gfx_scale_unlock();
	gfx_graph_unlock(GFX_GRPH_ENGINE_2);
	gfx_graph_unlock(GFX_GRPH_ENGINE_1);
}
static void gfx_record_lock(unsigned long *flag)
{
	unsigned long f;

	if(!flag){
		DBG_ERR("argument flag is NULL\n");
		return;
	}

	vk_spin_lock_irqsave(&RECORD_LOCK_ID, f);
	*flag = f;
}
static void gfx_record_unlock(unsigned long flag)
{
	vk_spin_unlock_irqrestore(&RECORD_LOCK_ID, flag);
}

static UINT32 argb_to_ycbcr(UINT32 argb)
{
#define cst_prom0   21
#define cst_prom1   79
#define cst_prom2   29
#define cst_prom3   43
#define RGB_GET_Y(r,g,b)    (((INT32)g) + ((cst_prom1 * (((INT32)r)-((INT32)g))) >> 8) + ((cst_prom2 * (((INT32)b)-((INT32)g))) >> 8) )
#define RGB_GET_U(r,g,b)    (128 + ((cst_prom3 * (((INT32)g)-((INT32)r))) >> 8) + ((((INT32)b)-((INT32)g)) >> 1) )
#define RGB_GET_V(r,g,b)    (128 + ((cst_prom0 * (((INT32)g)-((INT32)b))) >> 8) + ((((INT32)r)-((INT32)g)) >> 1) )

	UINT32 ycbcr = 0;

	ycbcr = RGB_GET_V((argb & 0x0FF0000) >> 16, (argb & 0x0FF00) >> 8, argb & 0x0FF);
	ycbcr <<= 8;
	ycbcr += RGB_GET_U((argb & 0x0FF0000) >> 16, (argb & 0x0FF00) >> 8, argb & 0x0FF);
	ycbcr <<= 8;
	ycbcr += RGB_GET_Y((argb & 0x0FF0000) >> 16, (argb & 0x0FF00) >> 8, argb & 0x0FF);

	return ycbcr;
}

static int check_flush(int flush_from_user)
{
	if(force_use_physical_address)
		return 0;
	else
		return flush_from_user;
}

static int gfx_init(void)
{
	//coverity[assigned_value]
	int  ret          = -1;
	int  blk_size     = (sizeof(GFX_USER_DATA) * (MAX_JOB_IN_A_LIST + 1));

	SEM_WAIT(GFX_JOB_SEM_ID);
	//gfx_api_lock();
	gfx_all_lock();
	force_use_physical_address = nvtmpp_is_dynamic_map();

	if(gfx_init_count != 0){
		gfx_init_count++;
		ret = 0;
		goto out;
	}else
		gfx_init_count = 1;

	gfx_memset(&scale_ll_buf, 0, sizeof(SCALE_LINK_LIST_BUF));

	if(job_buf){
		ret = 0;
		goto out;
	}

	job_buf = gfx_alloc(blk_size);
	if (job_buf == NULL) {
		DBG_ERR("fail to malloc job_buf of %d bytes\n", blk_size);
		ret = E_NOMEM;
		goto out;
	}

	ret = 0;

out:
	//gfx_api_unlock();
	gfx_all_unlock();
	SEM_SIGNAL(GFX_JOB_SEM_ID);

	return ret;
}

static int gfx_uninit(void)
{
	SEM_WAIT(GFX_JOB_SEM_ID);
	//gfx_api_lock();
	gfx_all_lock();

	if(gfx_init_count != 1){
		gfx_init_count--;
		goto out;
	}else
		gfx_init_count = 0;

	gfx_memset(&scale_ll_buf, 0, sizeof(SCALE_LINK_LIST_BUF));

	if(job_buf == NULL)
		goto out;

	gfx_free(job_buf);
	job_buf = NULL;

out:

	//gfx_api_unlock();
	gfx_all_unlock();
	SEM_SIGNAL(GFX_JOB_SEM_ID);

	return E_OK;
}

static void save_record(int pid, GFX_USER_CMD cmd, UINT64 time, int result,
						unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4)
{
	unsigned long flag = 0;

	gfx_record_lock(&flag);

	record.cur++;
	if(record.cur >= MAX_RECORD)
		record.cur = 0;

	record.pid[record.cur] = pid;
	record.cmd[record.cur] = cmd;
	record.result[record.cur] = result;
	record.time[record.cur] = time;
	record.p1[record.cur] = p1;
	record.p2[record.cur] = p2;
	record.p3[record.cur] = p3;
	record.p4[record.cur] = p4;

	gfx_record_unlock(flag);
}

static int gfx_copy(GFX_COPY *p_param)
{
	int           ret = E_PAR, i;
	VDO_FRAME     src_img, dst_img;
	UINT32        va, pa;
	IRECT         src_region, dst_region;
	IPOINT        src_location, dst_location;
	UINT64        start = hwclock_get_longcounter();
	GXIMG_CP_ENG  engine = GXIMG_CP_ENG1;

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	if(p_param->engine == GFX_GRPH_ENGINE_2)
		engine = GXIMG_CP_ENG2;

	gfx_graph_lock(p_param->engine);

	//DBG_ERR("gfx src pa(%x)\n", (int)p_param->src_img.p_phy_addr[0]);
	for(i = 0 ; i < GFX_MAX_PLANE_NUM ; ++i){
		pa = p_param->src_img.p_phy_addr[i];
		if(pa){
			if(force_use_physical_address){
				p_param->src_img.p_phy_addr[i] = pa;
			}else{
				va = nvtmpp_sys_pa2va(pa);
				if(!va){
					DBG_ERR("nvtmpp_sys_pa2va(%x) for src p_phy_addr[%d]\n", pa, i);
					goto out;
				}
				p_param->src_img.p_phy_addr[i] = va;
			}
		}

		pa = p_param->dst_img.p_phy_addr[i];
		if(pa){
			if(force_use_physical_address){
				p_param->dst_img.p_phy_addr[i] = pa;
			}else{
				va = nvtmpp_sys_pa2va(pa);
				if(!va){
					DBG_ERR("nvtmpp_sys_pa2va(%x) for dst p_phy_addr[%d]\n", pa, i);
					goto out;
				}
				p_param->dst_img.p_phy_addr[i] = va;
			}
		}
	}

	ret = gximg_init_buf_ex(&src_img, p_param->src_img.w, p_param->src_img.h, p_param->src_img.format, p_param->src_img.lineoffset, p_param->src_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init src gximg buf\n");
		goto out;
	}

	ret = gximg_init_buf_ex(&dst_img, p_param->dst_img.w, p_param->dst_img.h, p_param->dst_img.format, p_param->dst_img.lineoffset, p_param->dst_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init dst gximg buf\n");
		goto out;
	}

	src_region.x     = p_param->src_x;
	src_region.y     = p_param->src_y;
	src_region.w     = p_param->src_w;
	src_region.h     = p_param->src_h;
	dst_location.x   = p_param->dst_x;
	dst_location.y   = p_param->dst_y;

	if(p_param->flush){
		if(src_img.pxlfmt == dst_img.pxlfmt){//1. memory copy(yuv, rgb) 2. yuv overlay
			if(!src_region.x && !src_region.y &&
			   src_region.w == (INT32)p_param->src_img.w && src_region.h == (INT32)p_param->src_img.h &&
			   !dst_location.x && !dst_location.y &&
			   p_param->src_img.w == p_param->dst_img.w &&
			   p_param->src_img.h == p_param->dst_img.h &&
			   p_param->alpha == 255 && !p_param->colorkey) {//2d dma memory copy
				ret = gximg_copy_data(&src_img, &src_region, &dst_img, &dst_location, engine, force_use_physical_address);
			} else if(src_img.pxlfmt == VDO_PXLFMT_RGB565   || //argb overlay
					src_img.pxlfmt == VDO_PXLFMT_ARGB1555 ||
					src_img.pxlfmt == VDO_PXLFMT_ARGB4444 ||
					src_img.pxlfmt == VDO_PXLFMT_ARGB8888){
				if(p_param->colorkey){
					dst_region.x     = p_param->dst_x;
					dst_region.y     = p_param->dst_y;
					dst_region.w     = p_param->dst_img.w - dst_region.x;
					dst_region.h     = p_param->dst_img.h - dst_region.y;
					src_location.x   = 0;
					src_location.y   = 0;
					ret = gximg_copy_color_key_data(&dst_img, &dst_region, &src_img, &src_location, p_param->colorkey, 0, GXIMG_CP_ENG1, force_use_physical_address);
				}else
					ret = gximg_copy_data(&src_img, &src_region, &dst_img, &dst_location, GXIMG_CP_ENG1, force_use_physical_address);
			}else if(p_param->colorkey){//add yuv osd on yuv frame with colorkey
				dst_region.x     = p_param->dst_x;
				dst_region.y     = p_param->dst_y;
				dst_region.w     = p_param->dst_img.w - dst_region.x;
				dst_region.h     = p_param->dst_img.h - dst_region.y;
				src_location.x   = 0;
				src_location.y   = 0;
				ret = gximg_copy_color_key_data(&dst_img, &dst_region, &src_img, &src_location, p_param->colorkey, 0, GXIMG_CP_ENG1, force_use_physical_address);
			}
			else if(p_param->alpha)//add yuv osd on yuv frame with alpha
				ret = gximg_copy_blend_data(&src_img, &src_region, &dst_img, &dst_location, p_param->alpha, GXIMG_CP_ENG1, force_use_physical_address);
		}else{//src is rgb, dst is yuv
			if(p_param->colorkey)
				ret = gximg_rgb_to_yuv_color_key(&src_img, &src_region, &dst_img, &dst_location, p_param->alpha, p_param->colorkey, force_use_physical_address);
			else
				ret = gximg_argb_to_yuv_blend(&src_img, &src_region, &dst_img, &dst_location, p_param->alpha, p_param->src_img.palette, force_use_physical_address);
		}
	}else{ // no flush
		if (src_img.pxlfmt == dst_img.pxlfmt) {//1. memory copy(yuv, rgb) 2. yuv overlay
			if (!src_region.x && !src_region.y &&
			   src_region.w == (INT32)p_param->src_img.w && src_region.h == (INT32)p_param->src_img.h &&
			   !dst_location.x && !dst_location.y &&
			   p_param->src_img.w == p_param->dst_img.w &&
			   p_param->src_img.h == p_param->dst_img.h &&
			   p_param->alpha == 255 && !p_param->colorkey) {//2d dma memory copy
				ret = gximg_copy_data_no_flush(&src_img, &src_region, &dst_img, &dst_location, engine, force_use_physical_address);
			} else if (src_img.pxlfmt == VDO_PXLFMT_RGB565   || //argb overlay
					src_img.pxlfmt == VDO_PXLFMT_ARGB1555 ||
					src_img.pxlfmt == VDO_PXLFMT_ARGB4444 ||
					src_img.pxlfmt == VDO_PXLFMT_ARGB8888) {
				//colorkey + no flush is not ready
				//if(p_param->colorkey){
				//	dst_region.x     = p_param->dst_x;
				//	dst_region.y     = p_param->dst_y;
				//	dst_region.w     = p_param->dst_img.w - dst_region.x;
				//	dst_region.h     = p_param->dst_img.h - dst_region.y;
				//	src_location.x   = 0;
				//	src_location.y   = 0;
				//	ret = gximg_copy_color_key_data(&dst_img, &dst_region, &src_img, &src_location, p_param->colorkey, 0, GXIMG_CP_ENG1);
				//}else
					ret = gximg_copy_data_no_flush(&src_img, &src_region, &dst_img, &dst_location, engine, force_use_physical_address);
			}
			//colorkey + no flush is not ready
			//else if(p_param->colorkey){//add yuv osd on yuv frame with colorkey
			//	dst_region.x     = p_param->dst_x;
			//	dst_region.y     = p_param->dst_y;
			//	dst_region.w     = p_param->dst_img.w - dst_region.x;
			//	dst_region.h     = p_param->dst_img.h - dst_region.y;
			//	src_location.x   = 0;
			//	src_location.y   = 0;
			//	ret = gximg_copy_color_key_data(&dst_img, &dst_region, &src_img, &src_location, p_param->colorkey, 0, GXIMG_CP_ENG1);
			//}
			else if (p_param->alpha) {//add yuv osd on yuv frame with alpha
				ret = gximg_copy_blend_data_no_flush(&src_img, &src_region, &dst_img, &dst_location, p_param->alpha, GXIMG_CP_ENG1, force_use_physical_address);
			}
		} else {//src is rgb, dst is yuv
			//colorkey + no flush is not ready
			//if(p_param->colorkey)
			//	ret = gximg_rgb_to_yuv_color_key(&src_img, &src_region, &dst_img, &dst_location, p_param->alpha, p_param->colorkey);
			//else
				ret = gximg_argb_to_yuv_blend_no_flush(&src_img, &src_region, &dst_img, &dst_location, p_param->alpha, p_param->src_img.palette, force_use_physical_address);
		}
	}

out:

	save_record(vos_task_get_tid(), GFX_USER_CMD_COPY, hwclock_get_longcounter() - start, ret,
				p_param->src_img.w,	p_param->src_img.h, p_param->dst_img.w, p_param->dst_img.h);
	//gfx_api_unlock();
	gfx_graph_unlock(p_param->engine);

	return ret;
}

#if !USE_HWCOPY
#define HWMEM_MAX_LINE_OFFSET       10240
#define HWMEM_MAX_LINE_CNT          0x3FFF      // 2D DMA HW limitation

#define HWMEM_MAX_WIDTH             0x3FFF


#define HWMEM_PHASE1    0x01
#define HWMEM_PHASE2    0x02
#define HWMEM_PHASE3    0x04

typedef struct
{
    UINT32 uiSizeByHW;
    UINT32 uiSizeBySW;

    UINT32 uiWidth;
    UINT32 uiLineCnt;

    UINT32 uiDstAddr1;
    UINT32 uiDstAddr2;
    UINT32 uiDstAddr3;

    UINT32 uiSrcAddr1;
    UINT32 uiSrcAddr2;
    UINT32 uiSrcAddr3;
} HWMEM_INFO, *PHWMEM_INFO;

static BOOL hwmem_checkDramRange(UINT32 uiAddr, UINT32 uiSize)
{
    return TRUE;
}

static UINT32 hwmem_checkSize(UINT32 uiSrcAddr, UINT32 uiDstAddr, UINT32 uiSize, PHWMEM_INFO pHwMemInfo)
{
    UINT32 uiHWMEM_MULTIPASS_FLAG = 0x00;

    // Phase1: Destination address byte alignment use SW patch
    if(uiDstAddr & 0x3)
    {
        uiHWMEM_MULTIPASS_FLAG |= HWMEM_PHASE1;

        pHwMemInfo->uiWidth = 4 - (uiDstAddr & 0x3);
        if(pHwMemInfo->uiWidth >= uiSize)
        {
            pHwMemInfo->uiWidth = uiSize;
        }
        uiSize -= pHwMemInfo->uiWidth;
        pHwMemInfo->uiDstAddr1 = uiDstAddr;
        pHwMemInfo->uiSrcAddr1 = uiSrcAddr;
        uiDstAddr = uiDstAddr + pHwMemInfo->uiWidth;
        uiSrcAddr = uiSrcAddr + pHwMemInfo->uiWidth;
    }


    //Phase2: Word alignment use HW
    if(uiSize >= HWMEM_MAX_LINE_OFFSET)
    {
        uiHWMEM_MULTIPASS_FLAG |= HWMEM_PHASE2;

        pHwMemInfo->uiLineCnt = uiSize / HWMEM_MAX_LINE_OFFSET;
        uiSize = uiSize - (HWMEM_MAX_LINE_OFFSET * pHwMemInfo->uiLineCnt);
        pHwMemInfo->uiDstAddr2 = uiDstAddr;
        pHwMemInfo->uiSrcAddr2 = uiSrcAddr;
        uiDstAddr = uiDstAddr + (HWMEM_MAX_LINE_OFFSET * pHwMemInfo->uiLineCnt);
        uiSrcAddr = uiSrcAddr + (HWMEM_MAX_LINE_OFFSET * pHwMemInfo->uiLineCnt);
    }



    //Phase 3: Remaining bytes in word alignment use HW or SW
    if(uiSize)
    {
        uiHWMEM_MULTIPASS_FLAG |= HWMEM_PHASE3;
        pHwMemInfo->uiDstAddr3 = uiDstAddr;
        pHwMemInfo->uiSrcAddr3 = uiSrcAddr;

        if(uiSize < 4)
        {
            pHwMemInfo->uiSizeBySW = uiSize;
            pHwMemInfo->uiSizeByHW = 0;
        }
        else
        {
            pHwMemInfo->uiSizeBySW = 0;
            pHwMemInfo->uiSizeByHW = uiSize;
        }

    }

    return uiHWMEM_MULTIPASS_FLAG;
}
#endif

static int gfx_dma_copy(GFX_DMA_COPY *p_param)
{
#if USE_HWCOPY
	int                          ret = E_PAR;
	UINT32                       src_va, dst_va, len, copied;
	KDRV_HWCOPY_TRIGGER_PARAM    hwcopy_req;
	HWCOPY_CTEX                  ctex_descript;
	HWCOPY_MEM                   copy_mem_a = {HWCOPY_MEM_ID_A, 0, 0, 0, 0, NULL};
	HWCOPY_MEM                   copy_mem_b = {HWCOPY_MEM_ID_B, 0, 0, 0, 0, NULL};
	UINT64                       start = hwclock_get_longcounter();

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	//gfx_api_lock();
	gfx_hwcopy_lock();

	if(!p_param->p_src || !p_param->p_dst || !p_param->length){
		DBG_ERR("invalid dest(%x) src(%x) n(%d)\n", p_param->p_dst, p_param->p_src, (int)p_param->length);
		goto out;
	}

	src_va = nvtmpp_sys_pa2va(p_param->p_src);
	dst_va = nvtmpp_sys_pa2va(p_param->p_dst);
	if(!src_va || !dst_va){
		DBG_ERR("nvtmpp_sys_pa2va() for src(%x) or dst(%x) fail\n", p_param->p_src, p_param->p_dst);
		goto out;
	}

	copied = 0;
	while(copied < p_param->length){

		len = ((int)p_param->length - (int)copied > (63*1024*1024) ? (63*1024*1024) : p_param->length - copied);

		copy_mem_a.mem_id          = HWCOPY_MEM_ID_A;
		copy_mem_a.address         = src_va + copied;
		copy_mem_a.p_next          = &copy_mem_b;
		copy_mem_b.mem_id          = HWCOPY_MEM_ID_B;
		copy_mem_b.address         = dst_va + copied;
		ctex_descript.datalength   = len;
		hwcopy_req.command         = HWCOPY_LINEAR_COPY;
		hwcopy_req.p_memory        = &copy_mem_a;
		hwcopy_req.p_ctex_descript = &ctex_descript;

		if(kdrv_hwcopy_open(KDRV_CHIP0, KDRV_GFX2D_HWCOPY)){
			DBG_ERR("fail to open hwcopy\n");
			goto out;
		}
		if(kdrv_hwcopy_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_HWCOPY, 0), (KDRV_HWCOPY_TRIGGER_PARAM*)&hwcopy_req, NULL, NULL)){
			DBG_ERR("fail to trigger hwcopy\n");
			goto out;
		}
		if(kdrv_hwcopy_close(KDRV_CHIP0, KDRV_GFX2D_HWCOPY)){
			DBG_ERR("fail to close hwcopy\n");
			goto out;
		}

		copied += len;
	}

	ret = E_OK;

out:

	save_record(current->pid, GFX_USER_CMD_DMA_COPY, hwclock_get_longcounter() - start, ret, p_param->length, 0, 0, 0);

	//gfx_api_unlock();
	gfx_hwcopy_unlock();

	return ret;
#else
	UINT32 uiDst, uiSrc, uiSize;
	KDRV_GRPH_TRIGGER_PARAM grphRequest = {0};
	GRPH_IMG imgA = {0};
	GRPH_IMG imgC = {0};
	int ret = E_PAR;
	UINT32 grph_id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0);

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

    imgA.img_id = GRPH_IMG_ID_A;
    imgA.p_next = &imgC;
    imgC.img_id = GRPH_IMG_ID_C;
    grphRequest.command = GRPH_CMD_A_COPY;
    grphRequest.format = GRPH_FORMAT_8BITS;
    grphRequest.p_images = &imgA;

	if(force_use_physical_address){
		uiSrc = p_param->p_src;
		uiDst = p_param->p_dst;
		grphRequest.is_buf_pa = 1;
		grphRequest.is_skip_cache_flush = 1;
	}else{
		uiSrc = nvtmpp_sys_pa2va(p_param->p_src);
		uiDst = nvtmpp_sys_pa2va(p_param->p_dst);
	}
	uiSize = p_param->length;
	if(!uiSrc || !uiDst || !uiSize){
		DBG_ERR("nvtmpp_sys_pa2va() for src(%x) or dst(%x) of %d bytes fail\n", p_param->p_src, p_param->p_dst, uiSize);
		return E_PAR;
	}

	//gfx_api_lock();
	gfx_graph_lock(GFX_GRPH_ENGINE_1);

	if(gximg_open_grph(grph_id) == -1){
		DBG_ERR("fail to open graph0\n");
		//gfx_api_unlock();
		gfx_graph_unlock(GFX_GRPH_ENGINE_1);
		return E_PAR;
	}

    if ((uiDst & 0x3) != (uiSrc & 0x3))
    {
        UINT32 acc;
        UINT32 Dst,Src;

        Dst = uiDst;
        Src = uiSrc;
        imgA.lineoffset = 0x5000;
        imgA.height = 1;
        imgC.lineoffset = 0x5000;
        for(acc=0; acc < uiSize; acc += HWMEM_MAX_WIDTH)
        {

            imgA.dram_addr = Src;
            imgA.width = (uiSize >= (acc+HWMEM_MAX_WIDTH)) ?
                                HWMEM_MAX_WIDTH : (uiSize-acc);
            imgC.dram_addr = Dst;
            imgA.p_next = &imgC;
            grphRequest.p_images = &imgA;

			if(gximg_check_grph_limit(grphRequest.command, grphRequest.format, &imgA, 1) ||
				gximg_check_grph_limit(grphRequest.command, grphRequest.format, &imgC, 0)){
				DBG_ERR("limit check fail\n");
				goto out;
			}
			if(kdrv_grph_trigger(grph_id, &grphRequest, NULL, NULL) != E_OK){
				DBG_ERR("fail to trigger graphic engine(%d)\n", KDRV_GFX2D_GRPH0);
				goto out;
			}

            Src += imgA.width;
            Dst += imgA.width;
        }
    }
    else
    {
        UINT32 uiHWMEM_MULTIPASS_FLAG;
        HWMEM_INFO hwmemInfo = {0};

        // Parsing memory size
        uiHWMEM_MULTIPASS_FLAG = hwmem_checkSize(uiSrc,uiDst,uiSize, &hwmemInfo);

        // Phase 1: Destination address byte alignment use SW patch
        if(uiHWMEM_MULTIPASS_FLAG & HWMEM_PHASE1)
        {
			if(force_use_physical_address){
				DBG_ERR("when using physical address, in(0x%x) out(0x%x) buf are not supported\n", uiSrc, uiDst);
				goto out;
			}
            // phase 1 width must be 1~3 bytes only.
            gfx_memcpy((void *)hwmemInfo.uiDstAddr1, (void *)hwmemInfo.uiSrcAddr1, hwmemInfo.uiWidth);
        }

        //Phase2: Word alignment use HW
        if((uiHWMEM_MULTIPASS_FLAG & HWMEM_PHASE2) && hwmemInfo.uiLineCnt)
        {
            UINT32 uiLineChunk;
            UINT32 uiTotalLine = hwmemInfo.uiLineCnt;

            imgA.dram_addr = hwmemInfo.uiSrcAddr2;
            imgA.lineoffset = HWMEM_MAX_LINE_OFFSET;
            imgA.width = HWMEM_MAX_LINE_OFFSET;
            imgC.dram_addr = hwmemInfo.uiDstAddr2;
            imgC.lineoffset = HWMEM_MAX_LINE_OFFSET;

            while (uiTotalLine)
            {
                if (uiTotalLine < GRPH_AOP0_8BITS_HMAX)
                {
                    uiLineChunk = uiTotalLine;
                }
                else
                {
                    uiLineChunk = GRPH_AOP0_8BITS_HMAX;
                }

                imgA.height = uiLineChunk;
                grphRequest.p_images = &imgA;

				if(gximg_check_grph_limit(grphRequest.command, grphRequest.format, &imgA, 1) ||
					gximg_check_grph_limit(grphRequest.command, grphRequest.format, &imgC, 0)){
					DBG_ERR("limit check fail\n");
					goto out;
				}
				if(kdrv_grph_trigger(grph_id, &grphRequest, NULL, NULL) != E_OK){
					DBG_ERR("fail to trigger graphic engine(%d)\n", KDRV_GFX2D_GRPH0);
					goto out;
				}

                imgA.dram_addr += uiLineChunk * HWMEM_MAX_LINE_OFFSET;
                imgC.dram_addr += uiLineChunk * HWMEM_MAX_LINE_OFFSET;
                uiTotalLine -= uiLineChunk;
            }
        }

        //Phase 3: Remaining bytes in word alignment use HW or SW
        if(uiHWMEM_MULTIPASS_FLAG & HWMEM_PHASE3)
        {
            if(hwmemInfo.uiSizeByHW)
            {
                imgA.dram_addr = hwmemInfo.uiSrcAddr3;
                imgA.lineoffset = ALIGN_CEIL_4(hwmemInfo.uiSizeByHW);
                imgA.height = 1;
                imgA.width = hwmemInfo.uiSizeByHW;
                imgC.dram_addr = hwmemInfo.uiDstAddr3;
                imgC.lineoffset = ALIGN_CEIL_4(hwmemInfo.uiSizeByHW);
                imgA.p_next = &imgC;
                grphRequest.p_images = &imgA;
                if (hwmem_checkDramRange(imgA.dram_addr, imgA.lineoffset) &&
                    hwmem_checkDramRange(imgC.dram_addr, imgC.lineoffset)) {
					if(gximg_check_grph_limit(grphRequest.command, grphRequest.format, &imgA, 1) ||
						gximg_check_grph_limit(grphRequest.command, grphRequest.format, &imgC, 0)){
						DBG_ERR("limit check fail\n");
						goto out;
					}
					if(kdrv_grph_trigger(grph_id, &grphRequest, NULL, NULL) != E_OK){
						DBG_ERR("fail to trigger graphic engine(%d)\n", KDRV_GFX2D_GRPH0);
						goto out;
					}
                } else {
					if(force_use_physical_address){
						DBG_ERR("when using physical address, in(0x%x) out(0x%x) buf are not supported\n", uiSrc, uiDst);
						goto out;
					}
                    gfx_memcpy((void *)hwmemInfo.uiDstAddr3, (void *)hwmemInfo.uiSrcAddr3, hwmemInfo.uiSizeByHW);
                }
            }

            if(hwmemInfo.uiSizeBySW)
            {
				if(force_use_physical_address){
					DBG_ERR("when using physical address, in(0x%x) out(0x%x) buf are not supported\n", uiSrc, uiDst);
					goto out;
				}
                // SW width must be 1~3 bytes only.
                gfx_memcpy((void *)hwmemInfo.uiDstAddr3, (void *)hwmemInfo.uiSrcAddr3, hwmemInfo.uiSizeBySW);
            }
        }

    }

	ret = E_OK;

out:

	if(gximg_close_grph(grph_id) == -1)
		DBG_ERR("fail to close graph0\n");

	//gfx_api_unlock();
	gfx_graph_unlock(GFX_GRPH_ENGINE_1);

	return ret;
#endif
}

static int gfx_scale(GFX_SCALE *p_param)
{
	int                   ret = E_PAR, i;
	VDO_FRAME             src_img, dst_img;
	UINT32                va, pa;
	IRECT                 src_region;
	IRECT                 dst_region;
	GXIMG_SCALE_METHOD    quality;
	UINT64                start = hwclock_get_longcounter();

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	//gfx_api_lock();
	gfx_scale_lock();

	for(i = 0 ; i < GFX_MAX_PLANE_NUM ; ++i){
		pa = p_param->src_img.p_phy_addr[i];
		if(pa){
			if(force_use_physical_address){
				p_param->src_img.p_phy_addr[i] = pa;
			}else{
				va = nvtmpp_sys_pa2va(pa);
				if(!va){
					DBG_ERR("nvtmpp_sys_pa2va(%x) for src p_phy_addr[%d]\n", pa, i);
					goto out;
				}
				p_param->src_img.p_phy_addr[i] = va;
			}
		}

		pa = p_param->dst_img.p_phy_addr[i];
		if(pa){
			if(force_use_physical_address){
				p_param->dst_img.p_phy_addr[i] = pa;
			}else{
				va = nvtmpp_sys_pa2va(pa);
				if(!va){
					DBG_ERR("nvtmpp_sys_pa2va(%x) for dst p_phy_addr[%d]\n", pa, i);
					goto out;
				}
				p_param->dst_img.p_phy_addr[i] = va;
			}
		}
	}

	ret = gximg_init_buf_ex(&src_img, p_param->src_img.w, p_param->src_img.h, p_param->src_img.format, p_param->src_img.lineoffset, p_param->src_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init src gximg buf\n");
		goto out;
	}

	ret = gximg_init_buf_ex(&dst_img, p_param->dst_img.w, p_param->dst_img.h, p_param->dst_img.format, p_param->dst_img.lineoffset, p_param->dst_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init dst gximg buf\n");
		goto out;
	}

	src_region.x   = p_param->src_x;
	src_region.y   = p_param->src_y;
	src_region.w   = p_param->src_w;
	src_region.h   = p_param->src_h;
	dst_region.x   = p_param->dst_x;
	dst_region.y   = p_param->dst_y;
	dst_region.w   = p_param->dst_w;
	dst_region.h   = p_param->dst_h;

	if(p_param->method == GFX_SCALE_METHOD_BICUBIC)
		quality = GXIMG_SCALE_BICUBIC;
	else if(p_param->method == GFX_SCALE_METHOD_BILINEAR)
		quality = GXIMG_SCALE_BILINEAR;
	else if(p_param->method == GFX_SCALE_METHOD_NEAREST)
		quality = GXIMG_SCALE_NEAREST;
	else if(p_param->method == GFX_SCALE_METHOD_INTEGRATION)
		quality = GXIMG_SCALE_INTEGRATION;
	else
		quality = GXIMG_SCALE_AUTO;

	if (check_flush(p_param->flush)) {
		ret = gximg_scale_data(&src_img, &src_region, &dst_img, &dst_region, quality, force_use_physical_address);
		if(ret){
			DBG_ERR("gximg_scale_data() fail with %d\n", ret);
		}
	} else {
		ret = gximg_scale_data_no_flush(&src_img, &src_region, &dst_img, &dst_region, quality, force_use_physical_address);
		if(ret){
			DBG_ERR("gximg_scale_data_no_flush() fail with %d\n", ret);
		}
	}

out:

	save_record(vos_task_get_tid(), GFX_USER_CMD_SCALE, hwclock_get_longcounter() - start, ret,
			    p_param->src_img.w,	p_param->src_img.h, p_param->dst_img.w, p_param->dst_img.h);

	//gfx_api_unlock();
	gfx_scale_unlock();

	return ret;
}

static GFX_ROTATE_ANGLE convert_rotation_angle(UINT32 angle)
{
	switch (angle)
	{
		case GFX_ROTATE_ANGLE_90 :
			return VDO_DIR_ROTATE_90;
		case GFX_ROTATE_ANGLE_180 :
			return VDO_DIR_ROTATE_180;
		case GFX_ROTATE_ANGLE_270 :
			return VDO_DIR_ROTATE_270;
		case GFX_ROTATE_ANGLE_MIRROR_X :
			return VDO_DIR_MIRRORX;
		case GFX_ROTATE_ANGLE_MIRROR_Y :
			return VDO_DIR_MIRRORY;
		default :
			return VDO_DIR_NONE;
	};

	return VDO_DIR_NONE;
}

static int gfx_rotate(GFX_ROTATE *p_param)
{
	int           ret = E_PAR, i;
	VDO_FRAME     src_img, dst_img;
	UINT32        va, pa, angle;
	IRECT         src_region;
	IPOINT        dst_location;
	UINT64        start = hwclock_get_longcounter();

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	//gfx_api_lock();
	gfx_graph_lock(GFX_GRPH_ENGINE_2);

	for(i = 0 ; i < GFX_MAX_PLANE_NUM ; ++i){
		pa = p_param->src_img.p_phy_addr[i];
		if(pa){
			if(force_use_physical_address){
				p_param->src_img.p_phy_addr[i] = pa;
			}else{
				va = nvtmpp_sys_pa2va(pa);
				if(!va){
					DBG_ERR("nvtmpp_sys_pa2va(%x) for src p_phy_addr[%d]\n", pa, i);
					goto out;
				}
				p_param->src_img.p_phy_addr[i] = va;
			}
		}

		pa = p_param->dst_img.p_phy_addr[i];
		if(pa){
			if(force_use_physical_address){
				p_param->dst_img.p_phy_addr[i] = pa;
			}else{
				va = nvtmpp_sys_pa2va(pa);
				if(!va){
					DBG_ERR("nvtmpp_sys_pa2va(%x) for dst p_phy_addr[%d]\n", pa, i);
					goto out;
				}
				p_param->dst_img.p_phy_addr[i] = va;
			}
		}
	}

	ret = gximg_init_buf_ex(&src_img, p_param->src_img.w, p_param->src_img.h, p_param->src_img.format, p_param->src_img.lineoffset, p_param->src_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init src gximg buf\n");
		goto out;
	}

	ret = gximg_init_buf_ex(&dst_img, p_param->dst_img.w, p_param->dst_img.h, p_param->dst_img.format, p_param->dst_img.lineoffset, p_param->dst_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init dst gximg buf\n");
		goto out;
	}

	angle = convert_rotation_angle(p_param->angle);
	if(angle == VDO_DIR_NONE){
		DBG_ERR("angle(%x) is not supported\n", p_param->angle);
		goto out;
	}

	src_region.x   = p_param->src_x;
	src_region.y   = p_param->src_y;
	src_region.w   = p_param->src_w;
	src_region.h   = p_param->src_h;
	dst_location.x = p_param->dst_x;
	dst_location.y = p_param->dst_y;
	if (p_param->flush) {
		ret = gximg_rotate_paste_data(&src_img, &src_region, &dst_img, &dst_location, angle, GXIMG_ROTATE_ENG1, force_use_physical_address);
	} else { // no flush is only verified on Y8 format. other formats are not verified.
		ret = gximg_rotate_paste_data_no_flush(&src_img, &src_region, &dst_img, &dst_location, angle, GXIMG_ROTATE_ENG1, force_use_physical_address);
	}

	// coverity:158928
	if(ret != E_OK) {
		DBG_ERR("fail rotate %d\n",ret);
		goto out;
	}
out:

	save_record(vos_task_get_tid(), GFX_USER_CMD_ROTATE, hwclock_get_longcounter() - start, ret,
				p_param->src_img.w,	p_param->src_img.h, p_param->angle, 0);

	//gfx_api_unlock();
	gfx_graph_unlock(GFX_GRPH_ENGINE_2);

	return ret;
}

static int gfx_color_transform(GFX_COLOR_TRANSFORM *p_param)
{
	int           ret = E_PAR, i;
	VDO_FRAME     src_img, dst_img;
	UINT32        va, pa;
	IPOINT        dst_location;
	UINT64        start = hwclock_get_longcounter();

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	//gfx_api_lock();
	gfx_graph_lock(GFX_GRPH_ENGINE_1);

	for(i = 0 ; i < GFX_MAX_PLANE_NUM ; ++i){
		pa = p_param->src_img.p_phy_addr[i];
		if(pa){
			if(force_use_physical_address){
				p_param->src_img.p_phy_addr[i] = pa;
			}else{
				va = nvtmpp_sys_pa2va(pa);
				if(!va){
					DBG_ERR("nvtmpp_sys_pa2va(%x) for src p_phy_addr[%d]\n", pa, i);
					goto out;
				}
				p_param->src_img.p_phy_addr[i] = va;
			}
		}

		pa = p_param->dst_img.p_phy_addr[i];
		if(pa){
			if(force_use_physical_address){
				p_param->dst_img.p_phy_addr[i] = pa;
			}else{
				va = nvtmpp_sys_pa2va(pa);
				if(!va){
					DBG_ERR("nvtmpp_sys_pa2va(%x) for dst p_phy_addr[%d]\n", pa, i);
					goto out;
				}
				p_param->dst_img.p_phy_addr[i] = va;
			}
		}
	}

	ret = gximg_init_buf_ex(&src_img, p_param->src_img.w, p_param->src_img.h, p_param->src_img.format, p_param->src_img.lineoffset, p_param->src_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init src gximg buf\n");
		goto out;
	}

	ret = gximg_init_buf_ex(&dst_img, p_param->dst_img.w, p_param->dst_img.h, p_param->dst_img.format, p_param->dst_img.lineoffset, p_param->dst_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init dst gximg buf\n");
		goto out;
	}

	if(src_img.pxlfmt == VDO_PXLFMT_RGB565 || src_img.pxlfmt == VDO_PXLFMT_ARGB1555 || src_img.pxlfmt == VDO_PXLFMT_ARGB4444){
		dst_location.x = 0;
		dst_location.y = 0;
		ret = gximg_argb_to_yuv_blend(&src_img, GXIMG_REGION_MATCH_IMG, &dst_img, &dst_location, 0xFF, p_param->src_img.palette, force_use_physical_address);
	}else
		ret = gximg_color_transform(&src_img, &dst_img, 0, 0, force_use_physical_address);

out:

	save_record(vos_task_get_tid(), GFX_USER_CMD_COLOR_TRANSFORM, hwclock_get_longcounter() - start, ret,
				p_param->src_img.w,	p_param->src_img.h, p_param->src_img.format, p_param->dst_img.format);

	//gfx_api_unlock();
	gfx_graph_unlock(GFX_GRPH_ENGINE_1);

	return ret;
}

static int int_clamp(int prx, int lb, int ub)
{
	if (prx < lb)
		return(lb);
	else if (prx > ub)
		return(ub);
	else
		return(prx);
}
static int draw_line_yuv420(unsigned char *input_y, unsigned char *input_uv, int im_width, int im_height, DRAW_LINE_SETTING line_info)
{
    int x0,y0,x1,y1;//line start (x0,y0), line end (x1,y1)
    int a,b,c,slope;
    int i,j,stdx=0,stdy=0,endy=0,stx=0, stdy_even=0, endy_even=0;
    int stx1=0,stx2=0, stx1_even=0, stx2_even=0, dist,weight;
    int temp,delta;
    int transpose = 0;
    int linecolor[3];
    int draw_method = line_info.draw_method;
    int linewidth = line_info.linewidth;
	int width_up_bnd, hight_up_bnd;

    x0 = line_info.start_x;
    y0 = line_info.start_y;
    x1 = line_info.end_x;
    y1 = line_info.end_y;

	//linecolor[0] = line_info.linecolor[0];
    //linecolor[1] = line_info.linecolor[1]; 
    //linecolor[2] = line_info.linecolor[2]; 

	//-------------RGB to YUV transform-----------------//	
	linecolor[0] = ((1225 * line_info.linecolor[0]) + (2404 * line_info.linecolor[1]) + (467 * line_info.linecolor[2]) + 2048) >> 12;
	linecolor[0] = int_clamp(linecolor[0], 0, 255);

	linecolor[1] = ((-692 * line_info.linecolor[0]) + (-1360 * line_info.linecolor[1]) + (2048 * line_info.linecolor[2]));
	if(linecolor[1] >= 0)
		linecolor[1] = (linecolor[1] + 2048) >> 12;
	else
		linecolor[1] = -1 * (((-1 * linecolor[1]) + 2048) >> 12);

	linecolor[1] = int_clamp(linecolor[1], -128, 127) + 128;

	linecolor[2] = (2048 * line_info.linecolor[0]) + (-1716 * line_info.linecolor[1]) + (-333 * line_info.linecolor[2]);
	if(linecolor[2] >= 0)
		linecolor[2] = (linecolor[2] + 2048) >> 12;
	else
		linecolor[2] = -1 * (((-1 * linecolor[2]) + 2048) >> 12);

	linecolor[2] = int_clamp(linecolor[2], -128, 127) + 128;
	//------------------------------------------------------//
	
	width_up_bnd = im_width - 1;
	hight_up_bnd = im_height - 1;
    //swap x,y if slope<1
    if(abs(y0-y1)<abs(x0-x1)) {
        temp=x0 ; x0=y0 ; y0=temp ;
        temp=x1 ; x1=y1 ; y1=temp ;
		width_up_bnd = im_height - 1;
		hight_up_bnd = im_width - 1;
        transpose = 1;
    } else {
        transpose = 0;
    }

    //swap (x0,y0),(x1,y1) to make line always draw from up to down
    if(y0>y1) {
        temp=x0 ; x0=x1 ; x1=temp ;
        temp=y0 ; y0=y1 ; y1=temp ;
    }

    stdx = x0;
    stdy = y0;
    endy = y1;

    stdy_even = int_clamp(stdy + (stdy % 2), 0, hight_up_bnd);
    endy_even = int_clamp(endy - (endy % 2), 0, hight_up_bnd);

	//line formula coeff : ax+by+c=0
    a = (y0-y1);
    b = (x1-x0);
    c = (x0*y1-x1*y0);
    if(y0==y1) {
        slope = 0x7FFFFFFF;
    } else {
        slope = ((x0-x1)<<12)/(y0-y1);
    }

    delta = linewidth + 1;
    stx= (stdx << 12);
    for(j=stdy;j<=endy;j++) {
        stx1 = int_clamp((stx >> 12) - delta, 0, width_up_bnd);
        stx2 = int_clamp((stx >> 12) + delta, 0, width_up_bnd);
        for(i=stx1;i<=stx2;i++) {
            if(a == 0){
                dist = 0;
            }else{
                dist = abs((a*i+b*j+c)*256/(a+1/(2*a)));
            }
            if(dist<=linewidth*256) {
                if(draw_method == 0){
                    weight = ( 256 - dist / linewidth );//bilinear method
                } else{
                    weight = 256; //nearest method
				}
            } else {
                weight = 0;
            }
            if(dist <= linewidth*256) {
                if(transpose == 0) {
                    input_y[j * im_width + i] = (weight * linecolor[0] + (256 - weight)* input_y[j * im_width + i]) >> 8;
                } else {
                    input_y[i * im_width + j] = (weight * linecolor[0] + (256 - weight)* input_y[i * im_width + j]) >> 8;
                }
            }
        }
        stx += slope;
    }
	//--------------------------UV-------------------------------------------//
    stx= (stdx << 12);
    for(j=stdy_even; j<=endy_even; j+=2) {
        stx1_even = (((stx >> 12) - delta + 1) >> 1) << 1;
        stx2_even = (((stx >> 12) + delta    ) >> 1) << 1;
		stx1_even = int_clamp(stx1_even, 0, width_up_bnd);
		stx2_even = int_clamp(stx2_even, 0, width_up_bnd);
        for(i = stx1_even; i <= stx2_even;i+=2) {
            if(a == 0){
                dist = 0;
            }else{
                dist = abs((a*i+b*j+c)*256/(a+1/(2*a)));
            }
            if(dist <= linewidth * 256) {
                if(draw_method == 0){
                    weight = ( 256 - dist / linewidth );//bilinear method
                }else{
                    weight = 256; //nearest method
                }
            } else {
                weight = 0;
            }
            if(dist<=linewidth*256) {
                if(transpose == 0) {
                    input_uv[j/2 * im_width + i    ] = (weight * linecolor[1] + (256 - weight)* input_uv[j/2 * im_width + i]) >> 8;
                    input_uv[j/2 * im_width + i + 1] = (weight * linecolor[2] + (256 - weight)* input_uv[j/2 * im_width + i + 1]) >> 8;
                } else {
                    input_uv[i/2 * im_width + j    ] = (weight * linecolor[1] + (256 - weight)* input_uv[i/2 * im_width + j]) >> 8;
                    input_uv[i/2 * im_width + j + 1] = (weight * linecolor[2] + (256 - weight)* input_uv[i/2 * im_width + j + 1]) >> 8;
                }
            }
        }
        stx += (slope * 2);
    }
    return 0;
}

static int draw_line_argb4444(unsigned short *input, int im_width, int im_height, DRAW_LINE_SETTING line_info)
{
    int x0, y0, x1, y1; //line start (x0,y0), line end (x1,y1)
    int a, b, c, slope;
    int i, j, stdx = 0, stdy = 0, endy = 0, stx = 0;
    int stx1 = 0, stx2 = 0, dist, weight;
    int temp, delta;
    int transpose = 0;
    int linecolor[4];
    int draw_method = line_info.draw_method;
    int linewidth = line_info.linewidth;
    int width_up_bnd;
    int channel[4];

    x0 = line_info.start_x;
    y0 = line_info.start_y;
    x1 = line_info.end_x;
    y1 = line_info.end_y;

    linecolor[0] = line_info.linecolor[0];
    linecolor[1] = line_info.linecolor[1];
    linecolor[2] = line_info.linecolor[2];
    linecolor[3] = line_info.linecolor[3];

    //------------------------------------------------------//

    width_up_bnd = im_width - 1;
    //swap x,y if slope<1
    if (abs(y0 - y1) < abs(x0 - x1)) {
        temp = x0 ;
        x0 = y0 ;
        y0 = temp ;
        temp = x1 ;
        x1 = y1 ;
        y1 = temp ;
        width_up_bnd = im_height - 1;
        transpose = 1;
    } else {
        transpose = 0;
    }
    //swap (x0,y0),(x1,y1) to make line always draw from up to down
    if (y0 > y1) {
        temp = x0 ;
        x0 = x1 ;
        x1 = temp ;
        temp = y0 ;
        y0 = y1 ;
        y1 = temp ;
    }

    stdx = x0;
    stdy = y0;
    endy = y1;


    //line formula coeff : ax+by+c=0
    a = (y0 - y1);
    b = (x1 - x0);
    c = (x0 * y1 - x1 * y0);
    if (y0 == y1) {
        slope = 0x7FFFFFFF;
    } else {
        slope = ((x0 - x1) << 12) / (y0 - y1);
    }

    delta = linewidth + 1;
    stx = (stdx << 12);
    for (j = stdy; j <= endy; j++) {
        stx1 = int_clamp((stx >> 12) - delta, 0, width_up_bnd);
        stx2 = int_clamp((stx >> 12) + delta, 0, width_up_bnd);
        for (i = stx1; i <= stx2; i++) {
            if (a == 0) {
                dist = 0;
            } else {
                dist = abs((a * i + b * j + c) * 256 / (a + 1 / (2 * a)));
            }
            if (dist <= linewidth * 256) {
                if (draw_method == 0) {
                    weight = (256 - dist / linewidth);  //bilinear method
                } else {
                    weight = 256; //nearest method
                }
            } else {
                weight = 0;
            }
            if (dist <= linewidth * 256) {
                if (transpose == 0) {
                    channel[0] = (input[j * im_width + i] & 0xf000) >> 12;
                    channel[1] = (input[j * im_width + i] & 0x0f00) >> 8;
                    channel[2] = (input[j * im_width + i] & 0x00f0) >> 4;
                    channel[3] = (input[j * im_width + i] & 0x000f);

                    channel[0] = linecolor[0];
                    channel[1] = (weight * linecolor[1] + (256 - weight) * channel[1]) >> 8;
                    channel[2] = (weight * linecolor[2] + (256 - weight) * channel[2]) >> 8;
                    channel[3] = (weight * linecolor[3] + (256 - weight) * channel[3]) >> 8;					
                    input[j * im_width + i] = ((channel[0] << 12) | (channel[1] << 8) | (channel[2] << 4) | (channel[3]));
                } else {
                    channel[0] = (input[i * im_width + j] & 0xf000) >> 12;
                    channel[1] = (input[i * im_width + j] & 0x0f00) >> 8;
                    channel[2] = (input[i * im_width + j] & 0x00f0) >> 4;
                    channel[3] = (input[i * im_width + j] & 0x000f);

                    channel[0] = linecolor[0];
                    channel[1] = (weight * linecolor[1] + (256 - weight) * channel[1]) >> 8;
                    channel[2] = (weight * linecolor[2] + (256 - weight) * channel[2]) >> 8;
                    channel[3] = (weight * linecolor[3] + (256 - weight) * channel[3]) >> 8;					
                    input[i * im_width + j] = ((channel[0] << 12) | (channel[1] << 8) | (channel[2] << 4) | (channel[3]));
                }
            }
        }
        stx += slope;
    }
	return 0;
}

int gfx_draw_line_software(GFX_DRAW_LINE *p_param)
{
	DRAW_LINE_SETTING line_info = { 0 };
	int ret;
	VOS_ADDR plane1_addr, plane2_addr;

	if(p_param->dst_img.w == 0 || p_param->dst_img.h == 0){
		DBG_ERR("invalid image w(%d) or h(%d)\n", p_param->dst_img.w, p_param->dst_img.h);
		return -1;
	}

    if (p_param->start_x > p_param->dst_img.w || p_param->start_y > p_param->dst_img.h || 
	    p_param->end_x > p_param->dst_img.w || p_param->end_y > p_param->dst_img.h) {
        DBG_ERR("invalid slope line coordinate start(x:%d,y:%d) end(x:%d,y:%d) dst_dim(w:%d,h:%d)\n", 
		         p_param->start_x, p_param->start_y, p_param->end_x, p_param->end_y, p_param->dst_img.w, p_param->dst_img.h);
        return -1;
    }
	
	if(p_param->dst_img.format == VDO_PXLFMT_YUV420){
		if(p_param->dst_img.p_phy_addr[0] == 0 || p_param->dst_img.p_phy_addr[1] == 0){
			DBG_ERR("invalid y(%lx) or uv(%lx) address\n", (unsigned long)p_param->dst_img.p_phy_addr[0], (unsigned long)p_param->dst_img.p_phy_addr[1]);
			return -1;
		}
		if(p_param->dst_img.w != p_param->dst_img.lineoffset[0] || p_param->dst_img.w != p_param->dst_img.lineoffset[1]){
			DBG_ERR("image w(%d) should be equal to lineoffset(%d,%d)\n", p_param->dst_img.w, p_param->dst_img.lineoffset[0], p_param->dst_img.lineoffset[1]);
			return -1;
		}
			
		plane1_addr  = nvtmpp_sys_pa2va(p_param->dst_img.p_phy_addr[0]);
		plane2_addr  = nvtmpp_sys_pa2va(p_param->dst_img.p_phy_addr[1]);
		if(!plane1_addr || !plane2_addr){
			DBG_ERR("fail to transform physical address y(%lx) uv(%lx) to virtual address y(%lx) uv(%lx)", 
						(unsigned long)p_param->dst_img.p_phy_addr[0], (unsigned long)p_param->dst_img.p_phy_addr[1], (unsigned long)plane1_addr, (unsigned long)plane2_addr);
			return -1;
		}

		//if(p_param->flush){
		//	vos_cpu_dcache_sync(plane1_addr,  p_param->dst_img.w*p_param->dst_img.h   , VOS_DMA_FROM_DEVICE_NON_ALIGN);
		//	vos_cpu_dcache_sync(plane2_addr,  p_param->dst_img.w*p_param->dst_img.h/4 , VOS_DMA_FROM_DEVICE_NON_ALIGN);
		//}

		line_info.start_x      = p_param->start_x;
		line_info.start_y      = p_param->start_y;
		line_info.end_x        = p_param->end_x;
		line_info.end_y        = p_param->end_y;
		line_info.linewidth    = p_param->thickness;
		line_info.linecolor[0] = ((p_param->color & 0x00ff0000) >> 16);
		line_info.linecolor[1] = ((p_param->color & 0x0000ff00) >> 8);
		line_info.linecolor[2] =  (p_param->color & 0x000000ff);
		line_info.draw_method  = 0;
		ret = draw_line_yuv420((unsigned char*)plane1_addr, (unsigned char*)plane2_addr, p_param->dst_img.w, p_param->dst_img.h, line_info);
		if(ret){
			DBG_ERR("draw_line_yuv420() fail to draw slope lines\n");
			return -1;
		}

		//if(p_param->flush){
		//	vos_cpu_dcache_sync(plane1_addr,  p_param->dst_img.w*p_param->dst_img.h   , VOS_DMA_FROM_DEVICE_NON_ALIGN);
		//	vos_cpu_dcache_sync(plane2_addr,  p_param->dst_img.w*p_param->dst_img.h/4 , VOS_DMA_FROM_DEVICE_NON_ALIGN);
		//}
	}else if(p_param->dst_img.format == VDO_PXLFMT_ARGB4444){
		if(p_param->dst_img.p_phy_addr[0] == 0){
			DBG_ERR("invalid dst image address\n");
			return -1;
		}
		if(p_param->dst_img.w*2 != p_param->dst_img.lineoffset[0]){
			DBG_ERR("image w(%d) should be equal to lineoffset(%d)\n", p_param->dst_img.w, p_param->dst_img.lineoffset[0]);
			return -1;
		}
			
		plane1_addr  = nvtmpp_sys_pa2va(p_param->dst_img.p_phy_addr[0]);
		if(!plane1_addr){
			DBG_ERR("fail to transform physical address(%lx) to virtual address", (unsigned long)p_param->dst_img.p_phy_addr[0]);
			return -1;
		}

		//if(p_param->flush){
		//	vos_cpu_dcache_sync(plane1_addr, p_param->dst_img.w*p_param->dst_img.h*2 , VOS_DMA_FROM_DEVICE_NON_ALIGN);
		//}

		line_info.start_x      = p_param->start_x;
		line_info.start_y      = p_param->start_y;
		line_info.end_x        = p_param->end_x;
		line_info.end_y        = p_param->end_y;
		line_info.linewidth    = p_param->thickness;
		line_info.linecolor[0] = ((p_param->color & 0x0f000) >> 12);
		line_info.linecolor[1] = ((p_param->color & 0x00f00) >> 8);
		line_info.linecolor[2] = ((p_param->color & 0x000f0) >> 4);
		line_info.linecolor[3] =  (p_param->color & 0x0000f);
		line_info.draw_method  = 1;
		ret = draw_line_argb4444((unsigned short*)plane1_addr, p_param->dst_img.w, p_param->dst_img.h, line_info);
		if(ret){
			DBG_ERR("draw_line_argb4444() fail to draw slope lines\n");
			return -1;
		}

		//if(p_param->flush){
		//	vos_cpu_dcache_sync(plane1_addr, p_param->dst_img.w*p_param->dst_img.h*2 , VOS_DMA_FROM_DEVICE_NON_ALIGN);
		//}
	}else{
		DBG_ERR("drawing slope lines only supports yuv420 and argb4444, yours is %x\n", p_param->dst_img.format);
		return -1;
	}

	return 0;
}

static int gfx_draw_line(GFX_DRAW_LINE *p_param, UINT32 draw_by_cpu)
{
	int           ret = E_PAR, i;
	VDO_FRAME     frame;
	UINT32        va, pa;
	IRECT         dst_region;
	UINT32        small_x, small_y, large_x, large_y;
	UINT64        start = hwclock_get_longcounter();

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	//render by cpu
	if(draw_by_cpu){
		return gfx_draw_line_software(p_param);
	}
	
	//gfx_api_lock();
	gfx_graph_lock(GFX_GRPH_ENGINE_1);

	small_x = (p_param->start_x < p_param->end_x ? p_param->start_x : p_param->end_x);
	large_x = (p_param->start_x < p_param->end_x ? p_param->end_x   : p_param->start_x);
	small_y = (p_param->start_y < p_param->end_y ? p_param->start_y : p_param->end_y);
	large_y = (p_param->start_y < p_param->end_y ? p_param->end_y   : p_param->start_y);

	if(p_param->start_x == p_param->end_x){//must be a vertical line
		dst_region.x = small_x;
		dst_region.y = small_y;
		dst_region.w = p_param->thickness;
		dst_region.h = (large_y - small_y);
	}else if(p_param->start_y == p_param->end_y){//must be a horizontal line
		dst_region.x = small_x;
		dst_region.y = small_y;
		dst_region.w = (large_x - small_x);
		dst_region.h = p_param->thickness;
	}else{//a slope, this is not supported
		DBG_ERR("line(%d,%d)=>(%d,%d) is neither vertical nor horizontal\n",
			p_param->start_x, p_param->start_y, p_param->end_x, p_param->end_y);
		DBG_ERR("only vertical and horizontal are valid\n");
		gfx_memset(&dst_region, 0, sizeof(dst_region));
		goto out;
	}

	if(!p_param->thickness || (p_param->thickness & 0x01)){
		DBG_ERR("thickness(%d) can't be 0 or odd\n", p_param->thickness);
		goto out;
	}

	if(p_param->start_x == p_param->end_x && p_param->start_y == p_param->end_y){
		DBG_ERR("start point(%d,%d) can't be end point\n", p_param->start_x, p_param->start_y);
		goto out;
	}

	for(i = 0 ; i < GFX_MAX_PLANE_NUM ; ++i){
		pa = p_param->dst_img.p_phy_addr[i];
		if(pa){
			if(force_use_physical_address){
				p_param->dst_img.p_phy_addr[i] = pa;
			}else{
				va = nvtmpp_sys_pa2va(pa);
				if(!va){
					DBG_ERR("nvtmpp_sys_pa2va(%x) for p_phy_addr[%d]\n", pa, i);
					goto out;
				}
				p_param->dst_img.p_phy_addr[i] = va;
			}
		}
	}

	ret = gximg_init_buf_ex(&frame, p_param->dst_img.w, p_param->dst_img.h, p_param->dst_img.format, p_param->dst_img.lineoffset, p_param->dst_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init gximg buf\n");
		goto out;
	}

	if(p_param->flush){
		if(p_param->dst_img.format == VDO_PXLFMT_RGB565 ||
		   p_param->dst_img.format == VDO_PXLFMT_ARGB1555 ||
		   p_param->dst_img.format == VDO_PXLFMT_ARGB4444 ||
		   p_param->dst_img.format == VDO_PXLFMT_Y8 ||
		   p_param->dst_img.format == VDO_PXLFMT_I8)
			ret = gximg_fill_data(&frame, &dst_region, p_param->color, force_use_physical_address);
		else
			ret = gximg_fill_data(&frame, &dst_region, argb_to_ycbcr(p_param->color), force_use_physical_address);
	}else{
		if(p_param->dst_img.format == VDO_PXLFMT_RGB565 ||
		   p_param->dst_img.format == VDO_PXLFMT_ARGB1555 ||
		   p_param->dst_img.format == VDO_PXLFMT_ARGB4444 ||
		   p_param->dst_img.format == VDO_PXLFMT_Y8 ||
		   p_param->dst_img.format == VDO_PXLFMT_I8)
			ret = gximg_fill_data_no_flush(&frame, &dst_region, p_param->color, force_use_physical_address);
		else
			ret = gximg_fill_data_no_flush(&frame, &dst_region, argb_to_ycbcr(p_param->color), force_use_physical_address);
	}

out:

	save_record(vos_task_get_tid(), GFX_USER_CMD_DRAW_LINE, hwclock_get_longcounter() - start, ret,
				p_param->dst_img.w,	p_param->dst_img.h, dst_region.w * dst_region.h,0);

	//gfx_api_unlock();
	gfx_graph_unlock(GFX_GRPH_ENGINE_1);

	return ret;
}

static int gfx_draw_rect(GFX_DRAW_RECT *p_param)
{
	int                      ret = E_PAR, i;
	VDO_FRAME                frame;
	UINT32                   va, pa;
	IRECT                    dst_region;
	UINT64                   start = hwclock_get_longcounter();
	GXIMG_COVER_DESC         desc;

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	//gfx_api_lock();
	gfx_graph_lock(GFX_GRPH_ENGINE_1);

	dst_region.x = p_param->x;
	dst_region.y = p_param->y;
	dst_region.w = p_param->w;
	dst_region.h = p_param->h;

	for(i = 0 ; i < GFX_MAX_PLANE_NUM ; ++i){
		pa = p_param->dst_img.p_phy_addr[i];
		if(pa){
			if(force_use_physical_address){
				p_param->dst_img.p_phy_addr[i] = pa;
			}else{
				va = nvtmpp_sys_pa2va(pa);
				if(!va){
					DBG_ERR("nvtmpp_sys_pa2va(%x) for p_phy_addr[%d]\n", pa, i);
					goto out;
				}
				p_param->dst_img.p_phy_addr[i] = va;
			}
		}
	}

	ret = gximg_init_buf_ex(&frame, p_param->dst_img.w, p_param->dst_img.h, p_param->dst_img.format, p_param->dst_img.lineoffset, p_param->dst_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init gximg buf\n");
		goto out;
	}

	if(p_param->dst_img.format == VDO_PXLFMT_RGB565 ||
	   p_param->dst_img.format == VDO_PXLFMT_ARGB1555 ||
	   p_param->dst_img.format == VDO_PXLFMT_ARGB4444 ||
	   p_param->dst_img.format == VDO_PXLFMT_ARGB8888 ||
	   p_param->dst_img.format == VDO_PXLFMT_I8 ||
	   p_param->dst_img.format == VDO_PXLFMT_Y8){
			if(p_param->flush)
				ret = gximg_fill_data(&frame, &dst_region, p_param->color, force_use_physical_address);
			else
				ret = gximg_fill_data_no_flush(&frame, &dst_region, p_param->color, force_use_physical_address);
	}else if(p_param->type == GFX_RECT_TYPE_HOLLOW){
		gfx_memset(&desc, 0, sizeof(GXIMG_COVER_DESC));
		desc.top_left.x     = p_param->x;
		desc.top_left.y     = p_param->y;
		desc.top_right.x    = desc.top_left.x + p_param->w;
		desc.top_right.y    = desc.top_left.y;
		desc.bottom_right.x = desc.top_right.x;
		desc.bottom_right.y = desc.top_right.y + p_param->h;
		desc.bottom_left.x  = desc.top_left.x;
		desc.bottom_left.y  = desc.bottom_right.y;
		desc.yuva           = argb_to_ycbcr(p_param->color);
		desc.yuva          |= (p_param->color & 0xFF000000); //alpha

		dst_region.x = 0;
		dst_region.y = 0;
		dst_region.w = frame.size.w;
		dst_region.h = frame.size.h;

		if(p_param->flush)
			ret = gximg_quad_cover(&desc, &frame, &dst_region, 1, p_param->thickness, force_use_physical_address);
		else
			ret = gximg_quad_cover_with_flush(&desc, &frame, &dst_region, 1, p_param->thickness, 1, force_use_physical_address);
	}else{
		if(p_param->flush)
			ret = gximg_fill_data(&frame, &dst_region, argb_to_ycbcr(p_param->color), force_use_physical_address);
		else
			ret = gximg_fill_data_no_flush(&frame, &dst_region, argb_to_ycbcr(p_param->color), force_use_physical_address);
	}

out:

	save_record(vos_task_get_tid(), GFX_USER_CMD_DRAW_RECT, hwclock_get_longcounter() - start, ret,
				p_param->dst_img.w,	p_param->dst_img.h, dst_region.w, dst_region.h);

	//gfx_api_unlock();
	gfx_graph_unlock(GFX_GRPH_ENGINE_1);

	return ret;
}

static int gfx_arithmetic(GFX_ARITHMETIC *p_param)
{
	int                        ret = E_PAR;
	UINT32                     op1_ptr, op2_ptr, out_ptr, lineoffset;
	KDRV_GRPH_TRIGGER_PARAM    request = {0};
	GRPH_IMG                   images[3];
	GRPH_PROPERTY              property = {0};
	//GRPH_INOUTOP               ops[3];
	UINT32                     grph_id;
	UINT64                     start = hwclock_get_longcounter();

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	//gfx_api_lock();
	gfx_graph_lock(GFX_GRPH_ENGINE_1);

	if(!p_param->size){
		DBG_ERR("size is 0\n");
		goto out;
	}

	if(p_param->bits != GFX_ARITH_BIT_8 && p_param->bits != GFX_ARITH_BIT_16){
		DBG_ERR("bits(%d) is not supported\n", p_param->bits);
		goto out;
	}

	if(!p_param->p_op1 || !p_param->p_op2 || !p_param->p_out){
		DBG_ERR("p_op1(%x) or p_op2(%x) or p_out(%x) is NULL\n", p_param->p_op1, p_param->p_op2, p_param->p_out);
		goto out;
	}

	if(p_param->bits == GFX_ARITH_BIT_8){
		if(p_param->size & 0x03){
			DBG_ERR("8bits arithmetic requires size to be multiple of 4. size(%d) is invalid\n", p_param->size);
			goto out;
		}
		lineoffset = p_param->size;
	}
	else{
		if(p_param->size & 0x01){
			DBG_ERR("16bits arithmetic requires size to be multiple of 2. size(%d) is invalid\n", p_param->size);
			goto out;
		}
		if((p_param->p_op1 & 0x01) || (p_param->p_op2 & 0x01) || (p_param->p_out & 0x01)){
			DBG_ERR("p_op1(%x) or p_op2(%x) or p_out(%x) is not 2 bytes aligned\n", p_param->p_op1, p_param->p_op2, p_param->p_out);
			goto out;
		}
		lineoffset = (p_param->size << 1);
	}

	if(force_use_physical_address)
		op1_ptr = p_param->p_op1;
	else
		op1_ptr = nvtmpp_sys_pa2va(p_param->p_op1);
	if(!op1_ptr){
		DBG_ERR("fail to map p_op1(%x)\n", p_param->p_op1);
		goto out;
	}

	if(force_use_physical_address)
		op2_ptr = p_param->p_op2;
	else
		op2_ptr = nvtmpp_sys_pa2va(p_param->p_op2);
	if(!op2_ptr){
		DBG_ERR("fail to map p_op2(%x)\n", p_param->p_op2);
		goto out;
	}

	if(force_use_physical_address)
		out_ptr = p_param->p_out;
	else
		out_ptr = nvtmpp_sys_pa2va(p_param->p_out);
	if(!out_ptr){
		DBG_ERR("fail to map p_out(%x)\n", p_param->p_out);
		goto out;
	}

	gfx_memset(images, 0, sizeof(images));
	//memset(ops,    0, sizeof(ops));

	if(p_param->operation == GFX_ARITH_OP_PLUS)
		request.command = GRPH_CMD_PLUS_SHF;
	else if(p_param->operation == GFX_ARITH_OP_MINUS)
		request.command = GRPH_CMD_MINUS_SHF;
	else if(p_param->operation == GFX_ARITH_OP_MULTIPLY)
		request.command = GRPH_CMD_MULTIPLY_DIV;

	if(p_param->bits == GFX_ARITH_BIT_8)
		request.format = GRPH_FORMAT_8BITS;
	else if(p_param->bits == GFX_ARITH_BIT_16)
		request.format = GRPH_FORMAT_16BITS;

	request.is_buf_pa = force_use_physical_address;
	if(force_use_physical_address)
		request.is_skip_cache_flush = 1;

	request.p_images        = &(images[0]);
	images[0].img_id        = GRPH_IMG_ID_A;
	images[0].dram_addr     = op1_ptr;
	images[0].lineoffset    = lineoffset;
	images[0].width         = lineoffset;
	images[0].height        = 1;
	images[0].p_next        = &(images[1]);
	//images[0].p_inoutop     = &(ops[0]);
	images[1].img_id        = GRPH_IMG_ID_B;
	images[1].dram_addr     = op2_ptr;
	images[1].lineoffset    = lineoffset;
	images[1].p_next        = &(images[2]);
	images[2].img_id        = GRPH_IMG_ID_C;
	images[2].dram_addr     = out_ptr;
	images[2].lineoffset    = lineoffset;
	images[2].p_next        = NULL;
	//ops[0].id               = GRPH_INOUT_ID_IN_A;
	//ops[0].op               = GRPH_INOP_NONE;
	//ops[0].p_next           = NULL;

	if(p_param->operation == GFX_ARITH_OP_MULTIPLY){
		property.id          = GRPH_PROPERTY_ID_NORMAL;
		property.property    = GRPH_MULT_PROPTY(0, 0, 0, 0);
		property.p_next      = NULL;
		request.p_property   = &property;
		images[1].lineoffset = p_param->size;//16bits multiply requires image[1] to be 8bits
	}

	if(gximg_check_grph_limit(request.command, request.format, &(images[0]), 1) ||
		gximg_check_grph_limit(request.command, request.format, &(images[1]), 1) ||
		gximg_check_grph_limit(request.command, request.format, &(images[2]), 1)){
		DBG_ERR("limit check fail\n");
		goto out;
	}

	grph_id = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0);
	if(gximg_open_grph(grph_id) == -1){
		DBG_ERR("fail to open graphic engine(%d)\n", KDRV_GFX2D_GRPH0);
		goto out;
	}
	ret = kdrv_grph_trigger(grph_id, &request, NULL, NULL);
	if(ret != E_OK){
		DBG_ERR("fail to trigger graphic engine(%d)\n", KDRV_GFX2D_GRPH0);
		gximg_close_grph(grph_id);
		goto out;
	}
	ret = gximg_close_grph(grph_id);
	if(ret != E_OK)
		DBG_ERR("fail to close graphic engine(%d)\n", KDRV_GFX2D_GRPH0);

out:

	save_record(vos_task_get_tid(), GFX_USER_CMD_ARITHMETIC, hwclock_get_longcounter() - start, ret,
				p_param->operation,	p_param->bits, p_param->size, 0);

	//gfx_api_unlock();
	gfx_graph_unlock(GFX_GRPH_ENGINE_1);

	return ret;
}
static int gfx_scale_link_list(unsigned long arg)
{
	int                 ret = E_PAR, i, j;
	VDO_FRAME           src_img, dst_img;
	UINT32              va, pa, engine, ll_size = 0;
	int                 mode_count = 0, temp_mode_count = 0;
	GXIMG_SCALE_METHOD  quality_new, quality_old;
	IRECT               src_region, dst_region;
	GFX_SCALE_DMA_FLUSH *job = NULL;

	SEM_WAIT(GFX_JOB_SEM_ID);
	//gfx_api_lock();
	gfx_scale_lock();

	if(!job_buf){
		DBG_ERR("buffer for job list is NULL\n");
		ret = E_NOMEM;
		goto out;
	}

#ifdef __KERNEL__
	if(gfx_copy_from_user(job_buf, (void*)arg, sizeof(GFX_USER_DATA) * MAX_JOB_IN_A_LIST)){
		DBG_ERR("copy_from_user() fail\r\n");
		ret = -E_NOMEM;
		goto out;
	}
#else
	gfx_memcpy(job_buf, (void *)arg, sizeof(GFX_USER_DATA) * MAX_JOB_IN_A_LIST);
#endif

	engine = KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_ISE0, 0);

	if(force_use_physical_address){
		if(!scale_ll_buf.phy_addr || !scale_ll_buf.length){
			DBG_ERR("scale link list buffer is not registered\n");
			goto out;
		}
	}else{
		if(!scale_ll_buf.virt_addr || !scale_ll_buf.length){
			DBG_ERR("scale link list buffer is not registered\n");
			goto out;
		}
	}
	if(!force_use_physical_address)
		gfx_memset((void*)scale_ll_buf.virt_addr, 0, scale_ll_buf.length);

	gfx_memset(ise_mode, 0, sizeof(ise_mode));

	for(i = 0 ; i < MAX_JOB_IN_A_LIST ; ++i){

		if((mode_count+3) > (MAX_ISE_MODE_SIZE)){
			DBG_ERR("configured ise_mode(%d) > max(%d)\n", mode_count+3, MAX_JOB_IN_A_LIST*2+1);
			ret = E_PAR;
			goto out;
		}

		if(job_buf[i].cmd != GFX_USER_CMD_SCALE_LINK_LIST)
			break;

		job = &(job_buf[i].data.ise_scale_dma_flush);

		for(j = 0 ; j < GFX_MAX_PLANE_NUM ; ++j){
			if(!force_use_physical_address){
				pa = job->src_img.p_phy_addr[j];
				if(pa){
					va = nvtmpp_sys_pa2va(pa);
					if(!va){
						DBG_ERR("nvtmpp_sys_pa2va(%x) for src p_phy_addr[%d]\n", pa, j);
						ret = E_PAR;
						goto out;
					}
					job->src_img.p_phy_addr[j] = va;
				}

				pa = job->dst_img.p_phy_addr[j];
				if(pa){
					va = nvtmpp_sys_pa2va(pa);
					if(!va){
						DBG_ERR("nvtmpp_sys_pa2va(%x) for dst p_phy_addr[%d]\n", pa, j);
						ret = E_PAR;
						goto out;
					}
					job->dst_img.p_phy_addr[j] = va;
				}
			}
		}

		ret = gximg_init_buf_ex(&src_img,
									job->src_img.w,
									job->src_img.h,
									job->src_img.format,
									job->src_img.lineoffset,
									job->src_img.p_phy_addr);
		if(ret != E_OK){
			DBG_ERR("fail to init src gximg buf\n");
			goto out;
		}

		ret = gximg_init_buf_ex(&dst_img,
									job->dst_img.w,
									job->dst_img.h,
									job->dst_img.format,
									job->dst_img.lineoffset,
									job->dst_img.p_phy_addr);
		if(ret != E_OK){
			DBG_ERR("fail to init dst gximg buf\n");
			goto out;
		}

		src_region.x   = job->src_x;
		src_region.y   = job->src_y;
		src_region.w   = job->src_w;
		src_region.h   = job->src_h;
		dst_region.x   = job->dst_x;
		dst_region.y   = job->dst_y;
		dst_region.w   = job->dst_w;
		dst_region.h   = job->dst_h;

		if(job->method == GFX_SCALE_METHOD_BICUBIC)
			quality_new = GXIMG_SCALE_BICUBIC;
		else if(job->method == GFX_SCALE_METHOD_BILINEAR)
			quality_new = GXIMG_SCALE_BILINEAR;
		else if(job->method == GFX_SCALE_METHOD_NEAREST)
			quality_new = GXIMG_SCALE_NEAREST;
		else if(job->method == GFX_SCALE_METHOD_INTEGRATION)
			quality_new = GXIMG_SCALE_INTEGRATION;
		else
			quality_new = GXIMG_SCALE_MAX_ID;

		if(quality_new != GXIMG_SCALE_MAX_ID){
			quality_old = gximg_get_parm(GXIMG_PARM_SCALE_METHOD);
			gximg_set_parm(GXIMG_PARM_SCALE_METHOD, quality_new);
		}

		{
			extern ER gximg_scale_config_mode(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IRECT *p_dst_region, KDRV_ISE_MODE *mode, int *count, int usePA);
			ret = gximg_scale_config_mode(&src_img, &src_region, &dst_img, &dst_region, &ise_mode[mode_count], &temp_mode_count, force_use_physical_address);
			if(ret != E_OK){
				DBG_ERR("fail to config ise mode\n");
				goto out;
			}

			for(j = 0 ; j < temp_mode_count ; ++j){
				ise_mode[mode_count+j].in_buf_flush = (force_use_physical_address ? 1 : job->in_buf_flush);
				ise_mode[mode_count+j].out_buf_flush = (force_use_physical_address ? 1 : job->out_buf_flush);
			}
			mode_count += temp_mode_count;
		}

		if(quality_new != GXIMG_SCALE_MAX_ID)
			gximg_set_parm(GXIMG_PARM_SCALE_METHOD, quality_old);
	}

	for(i = 0 ; i < mode_count ; ++i){
		ise_mode[i].ll_job_nums = mode_count;
		if(force_use_physical_address)
			ise_mode[i].ll_in_addr  = scale_ll_buf.phy_addr;
		else
			ise_mode[i].ll_in_addr  = scale_ll_buf.virt_addr;
	}

	ll_size = kdrv_ise_get_linked_list_buf_size(engine, mode_count);
	if(ll_size > scale_ll_buf.length){
		DBG_ERR("required ll_size(%d) > registered size(%d)\n", ll_size, scale_ll_buf.length);
		ret = E_PAR;
		goto out;
	}

	ret = kdrv_ise_open(KDRV_CHIP0, KDRV_GFX2D_ISE0);
	if(ret != 0){
		DBG_ERR("kdrv_ise_open() fail with %d\n", ret);
		goto out;
	}

	// set jobs using linked-list
	ret = kdrv_ise_set(engine, KDRV_ISE_LL_PARAM_MODE, (void *)ise_mode);
	if (ret != 0) {
		DBG_ERR("kdrv_ise_set(KDRV_ISE_LL_PARAM_MODE) fail with %d\n", ret);
		goto out;
	}

	// linked-list fire
	ret = kdrv_ise_linked_list_trigger(engine);
	if (ret != 0) {
		DBG_ERR("kdrv_ise_linked_list_trigger() fail with %d\n", ret);
		kdrv_ise_close(KDRV_CHIP0, KDRV_GFX2D_ISE0);
		goto out;
	}

	// ise close
	ret = kdrv_ise_close(KDRV_CHIP0, KDRV_GFX2D_ISE0);
	if (ret != 0) {
		DBG_ERR("kdrv_ise_close() fail with %d\n", ret);
		goto out;
	}

out:

	//gfx_api_unlock();
	gfx_scale_unlock();
	SEM_SIGNAL(GFX_JOB_SEM_ID);

	return ret;
}

static int gfx_job_list(unsigned long arg)
{
	int              i, ret = 0;

	SEM_WAIT(GFX_JOB_SEM_ID);

	if(!job_buf){
		DBG_ERR("buffer for job list is NULL\n");
		ret = E_NOMEM;
		goto out;
	}

#ifdef __KERNEL__
	if(gfx_copy_from_user(job_buf, (void*)arg, sizeof(GFX_USER_DATA) * (MAX_JOB_IN_A_LIST + 1))){
		DBG_ERR("copy_from_user() fail\r\n");
		ret = -E_NOMEM;
		goto out;
	}
#else
	gfx_memcpy(job_buf, (void *)arg, sizeof(GFX_USER_DATA) * (MAX_JOB_IN_A_LIST + 1));
#endif

	for(i = 1 ; i <= MAX_JOB_IN_A_LIST ; ++i){

		if(job_buf[i].version != GFX_USER_VERSION)
			break;

		if(job_buf[i].cmd == GFX_USER_CMD_COPY)
			ret = gfx_copy(&job_buf[i].data.copy);
		else if(job_buf[i].cmd == GFX_USER_CMD_DMA_COPY)
			ret = gfx_dma_copy(&job_buf[i].data.dma_copy);
		else if(job_buf[i].cmd == GFX_USER_CMD_SCALE)
			ret = gfx_scale(&job_buf[i].data.scale);
		else if(job_buf[i].cmd == GFX_USER_CMD_ROTATE)
			ret = gfx_rotate(&job_buf[i].data.rotate);
		else if(job_buf[i].cmd == GFX_USER_CMD_COLOR_TRANSFORM)
			ret = gfx_color_transform(&job_buf[i].data.color_transform);
		else if(job_buf[i].cmd == GFX_USER_CMD_DRAW_LINE)
			ret = gfx_draw_line(&job_buf[i].data.draw_line, 0);
		else if(job_buf[i].cmd == GFX_USER_CMD_DRAW_RECT)
			ret = gfx_draw_rect(&job_buf[i].data.draw_rect);
		else if(job_buf[i].cmd == GFX_USER_CMD_ARITHMETIC)
			ret = gfx_arithmetic(&job_buf[i].data.arithmetic);
		else{
			DBG_ERR("job list doesn't support command %d\n", job_buf[i].cmd);
			ret = E_PAR;
		}

		if(ret){
			DBG_ERR("fail to handle %dth job/list element\n", i);
			break;
		}
	}

out:

	SEM_SIGNAL(GFX_JOB_SEM_ID);

	return ret;
}

static int gfx_ise_scale_y8(GFX_ISE_SCALE_Y8 *p_param)
{
	UINT32                    pa, id;
	KDRV_DEV_ENGINE           engine;
	int                       ret         = E_PAR;
	KDRV_ISE_MODE             ise_op      = {0};
	KDRV_ISE_TRIGGER_PARAM    ise_trigger = {1, 0};

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	if(!p_param->p_src || !p_param->p_dst){
		DBG_ERR("src buf(%x) or dst buf(%x) is NULL\n",
			p_param->p_src, p_param->p_dst);
		return E_PAR;
	}

	if(!p_param->src_w || !p_param->src_h){
		DBG_ERR("invalid src w(%d) or h(%d)\n",
			p_param->src_w, p_param->src_h);
		return E_PAR;
	}

	if(!p_param->dst_w || !p_param->dst_h){
		DBG_ERR("invalid dst w(%d) or h(%d)\n",
			p_param->dst_w, p_param->dst_h);
		return E_PAR;
	}

	if(!p_param->src_loff || !p_param->dst_loff){
		DBG_ERR("invalid src loff(%d) or dst loff(%d)\n",
			p_param->src_loff, p_param->dst_loff);
		return E_PAR;
	}

	if((int)p_param->src_loff < p_param->src_w){
		DBG_ERR("vendor_gfx_ise_scale_y8() : src loff(%d) can't be smaller than w(%d)\n",
			p_param->src_loff, p_param->src_w);
		return E_PAR;
	}

	if((int)p_param->dst_loff < p_param->dst_w){
		DBG_ERR("dst loff(%d) can't be smaller than w(%d)\n",
			p_param->dst_loff, p_param->dst_w);
		return E_PAR;
	}

	if(p_param->engine == GFX_ISE_ENGINE_1)
		engine = KDRV_GFX2D_ISE0;
	else if(p_param->engine == GFX_ISE_ENGINE_2)
		engine = KDRV_GFX2D_ISE1;
	else{
		DBG_ERR("engine(%d) is not supported\n", p_param->engine);
		return E_PAR;
	}

	if(!force_use_physical_address){
		pa = p_param->p_src;
		p_param->p_src = nvtmpp_sys_pa2va(pa);
		pa = p_param->p_dst;
		p_param->p_dst = nvtmpp_sys_pa2va(pa);
	}
	if(!p_param->p_src){
		DBG_ERR("nvtmpp_sys_pa2va(%x) for src addr\n", pa);
		return E_PAR;
	};
	if(!p_param->p_dst){
		DBG_ERR("nvtmpp_sys_pa2va(%x) for dst addr\n", pa);
		return E_PAR;
	}

	//gfx_api_lock();
	gfx_scale_lock();

	ise_op.phy_addr_en    = force_use_physical_address;
	ise_op.io_pack_fmt    = KDRV_ISE_Y8;
	ise_op.in_width       = p_param->src_w;
	ise_op.in_height      = p_param->src_h;
	ise_op.in_lofs        = p_param->src_loff;
	ise_op.in_addr        = p_param->p_src;
	ise_op.out_width      = p_param->dst_w;
	ise_op.out_height     = p_param->dst_h;
	ise_op.out_lofs       = p_param->dst_loff;
	ise_op.out_addr       = p_param->p_dst;

	if(force_use_physical_address){
		ise_op.in_buf_flush = 1;
		ise_op.out_buf_flush = 1;
	}

	if(p_param->method == GFX_SCALE_METHOD_NULL || p_param->method == GFX_SCALE_METHOD_NEAREST)
		ise_op.scl_method = KDRV_ISE_NEAREST;
	else if(p_param->method == GFX_SCALE_METHOD_BILINEAR)
		ise_op.scl_method = KDRV_ISE_BILINEAR;
	else if(p_param->method == GFX_SCALE_METHOD_INTEGRATION)
		ise_op.scl_method = KDRV_ISE_INTEGRATION;
	else{
		DBG_ERR("quality(%d) is not supported\n", p_param->method);
		goto out;
	}

	if (gximg_check_ise_limit(&ise_op)) {
		DBG_ERR("checking ise limit failed\r\n");
		goto out;
	}

	id = KDRV_DEV_ID(KDRV_CHIP0, engine, 0);
	if(kdrv_ise_open(KDRV_CHIP0, engine)){
		DBG_ERR("kdrv_ise_open fail\n");
		goto out;
	}
	ret = kdrv_ise_set(id, KDRV_ISE_PARAM_MODE, &ise_op);
	if(ret){
		DBG_ERR("kdrv_ise_set fail\n");
		kdrv_ise_close(KDRV_CHIP0, engine);
		goto out;
	}
	ret = kdrv_ise_trigger(id, &ise_trigger, NULL, NULL);
	if(ret){
		DBG_ERR("kdrv_ise_trigger fail\n");
		kdrv_ise_close(KDRV_CHIP0, engine);
		goto out;
	}

	ret = kdrv_ise_close(KDRV_CHIP0, engine);
	if(ret){
		DBG_ERR("kdrv_ise_close fail\n");
		goto out;
	}

out:

	//gfx_api_unlock();
	gfx_scale_unlock();

	return ret;
}

static int check_affine_limit(UINT32 src, UINT32 bits, AFFINE_IMAGE *image, KDRV_AFFINE_TRIGGER_PARAM *request)
{
	static unsigned int src_y8[]={
		AFFINE_SRCBUF_Y8BIT_WMIN, AFFINE_SRCBUF_Y8BIT_WMAX, AFFINE_SRCBUF_Y8BIT_WALIGN,
		AFFINE_SRCBUF_Y8BIT_HMIN, AFFINE_SRCBUF_Y8BIT_HMAX, AFFINE_SRCBUF_Y8BIT_HALIGN,
		AFFINE_SRCBUF_Y8BIT_ADDR_ALIGN};
	static unsigned int src_uvp[]={
		AFFINE_SRCBUF_UVP_WMIN, AFFINE_SRCBUF_UVP_WMAX, AFFINE_SRCBUF_UVP_WALIGN,
		AFFINE_SRCBUF_UVP_HMIN, AFFINE_SRCBUF_UVP_HMAX, AFFINE_SRCBUF_UVP_HALIGN,
		AFFINE_SRCBUF_UVP_ADDR_ALIGN};
	static unsigned int dst_y8[]={
		AFFINE_DSTBUF_Y8BIT_WMIN, AFFINE_DSTBUF_Y8BIT_WMAX, AFFINE_DSTBUF_Y8BIT_WALIGN,
		AFFINE_DSTBUF_Y8BIT_HMIN, AFFINE_DSTBUF_Y8BIT_HMAX, AFFINE_DSTBUF_Y8BIT_HALIGN,
		AFFINE_DSTBUF_Y8BIT_ADDR_ALIGN};
	static unsigned int dst_uvp[]={
		AFFINE_DSTBUF_UVP_WMIN, AFFINE_DSTBUF_UVP_WMAX, AFFINE_DSTBUF_UVP_WALIGN,
		AFFINE_DSTBUF_UVP_HMIN, AFFINE_DSTBUF_UVP_HMAX, AFFINE_DSTBUF_UVP_HALIGN,
		AFFINE_DSTBUF_UVP_ADDR_ALIGN};

	unsigned int *limit = NULL;

	if(!image){
		DBG_ERR("image is NULL\n");
		return -1;
	}

	if(!request){
		DBG_ERR("request is NULL\n");
		return -1;
	}

	if(src){
		if(bits == AFFINE_FORMAT_FORMAT_8BITS)
			limit = src_y8;
		else if(bits == AFFINE_FORMAT_FORMAT_16BITS_UVPACK)
			limit = src_uvp;
		else
			return 0;
	}else{
		if(bits == AFFINE_FORMAT_FORMAT_8BITS)
			limit = dst_y8;
		else if(bits == AFFINE_FORMAT_FORMAT_16BITS_UVPACK)
			limit = dst_uvp;
		else
			return 0;
	}

	if(image->uiLineOffset > AFFINE_LINEOFFSET_MAX){
		DBG_ERR("image lineoffset(%d) should be less than %d\n", image->uiLineOffset, AFFINE_LINEOFFSET_MAX);
		return -1;
	}
	if(request->uiWidth < limit[0]){
		DBG_ERR("image width(%d) should be larger than %d\n", (int)request->uiWidth, (int)limit[0]);
		return -1;
	}
	if(request->uiWidth > limit[1]){
		DBG_ERR("image width(%d) should be less than %d\n", (int)request->uiWidth, (int)limit[1]);
		return -1;
	}
	if(limit[2] && (request->uiWidth & (limit[2] - 1))){
		DBG_ERR("image width(%d) should be %d aligned\n", (int)request->uiWidth, (int)limit[2]);
		return -1;
	}
	if(request->uiHeight < limit[3]){
		DBG_ERR("image height(%d) should be larger than %d\n", (int)request->uiHeight, (int)limit[3]);
		return -1;
	}
	if(request->uiHeight > limit[4]){
		DBG_ERR("image height(%d) should be less than %d\n", (int)request->uiHeight, (int)limit[4]);
		return -1;
	}
	if(limit[5] && (request->uiHeight & (limit[5] - 1))){
		DBG_ERR("image height(%d) should be %d aligned\n", (int)request->uiHeight, (int)limit[5]);
		return -1;
	}
	if(limit[6] && (image->uiImageAddr & (limit[6] - 1))){
		DBG_ERR("image addr(%d) should be %d aligned\n", (int)image->uiImageAddr, (int)limit[6]);
		return -1;
	}

	return 0;
}

static int gfx_affine(GFX_AFFINE *p_param)
{
	UINT32                    s_addr, d_addr;
	int                       ret = E_PAR, i;
	KDRV_AFFINE_TRIGGER_PARAM request;
	AFFINE_IMAGE              s_img, d_img;
	AFFINE_COEFF              coeff;

	if(!gfx_if_affine_open ||
	   !gfx_if_affine_trigger ||
	   !gfx_if_affine_close){
		DBG_ERR("affine is not supported\n");
		return E_PAR;
	}

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	if(!p_param->plane_num || p_param->plane_num > GFX_MAX_PLANE_NUM){
		DBG_ERR("invalid plane number(%d). min is 1, max is %d\n",
					p_param->plane_num, GFX_MAX_PLANE_NUM);
		return E_PAR;
	}

	for(i = 0 ; i < p_param->plane_num ; ++i){
		if(!p_param->p_src[i]){
			DBG_ERR("src buf[%d] is NULL\n", i);
			return E_PAR;
		}

		if(p_param->p_src[i] & 0x03){
			DBG_ERR("src buf[%d] = %x, not 4 align\n", i, p_param->p_src[i]);
			return E_PAR;
		}

		if(!p_param->p_dst[i]){
			DBG_ERR("dst buf[%d] is NULL\n", i);
			return E_PAR;
		}

		if(p_param->p_dst[i] & 0x03){
			DBG_ERR("dst buf[%d] = %x, not 4 align\n", i, p_param->p_src[i]);
			return E_PAR;
		}

		if(!p_param->src_loff[i] || p_param->src_loff[i] != p_param->dst_loff[i]){
			DBG_ERR("invalid src lineoffset(%d) or dst lineoffset(%d)\n",
				p_param->src_loff[i], p_param->dst_loff[i]);
			return E_PAR;
		}

		if(p_param->src_loff[i] & 0x03){
			DBG_ERR("src lineoffset[%d] = %d, not 4 align\n", i, p_param->src_loff[i]);
			return E_PAR;
		}
	}

	if(!p_param->w || !p_param->h){
		DBG_ERR("invalid w(%d) or h(%d)\n", p_param->w, p_param->h);
		return E_PAR;
	}

	if((p_param->w & 0x0F) || (p_param->h & 0x07)){
		DBG_ERR("invalid w(%d) or h(%d). w shold be 16 align, h should be 8 align\n",
			p_param->w, p_param->h);
		return E_PAR;
	}

	//don't use any sleep lock, affine calls neon instructions which can't be in sleep state
	//gfx_api_lock();

	if(gfx_if_affine_open(KDRV_CHIP0, KDRV_GFX2D_AFFINE) != 0){
		DBG_ERR("open affine engine fail\n");
		goto out2;
	}

	for(i = 0 ; i < p_param->plane_num ; ++i){

		s_addr = nvtmpp_sys_pa2va(p_param->p_src[i]);
		d_addr = nvtmpp_sys_pa2va(p_param->p_dst[i]);
		if(!s_addr || !d_addr){
			DBG_ERR("nvtmpp_sys_pa2va() fail on src addr[%d]=(%x) or dst addr[%d]=(%x)\n",
				i, p_param->p_src[i], i, p_param->p_dst[i]);
			goto out1;
		}

		gfx_memset(&request, 0, sizeof(KDRV_AFFINE_TRIGGER_PARAM));
		gfx_memset(&s_img,   0, sizeof(AFFINE_IMAGE));
		gfx_memset(&d_img,   0, sizeof(AFFINE_IMAGE));
		gfx_memset(&coeff,   0, sizeof(AFFINE_COEFF));

		s_img.uiImageAddr      = s_addr;
		d_img.uiImageAddr      = d_addr;
		s_img.uiLineOffset     = p_param->src_loff[i];
		d_img.uiLineOffset     = p_param->dst_loff[i];

		coeff.fCoeffA          = p_param->fCoeffA[i];
		coeff.fCoeffB          = p_param->fCoeffB[i];
		coeff.fCoeffC          = p_param->fCoeffC[i];
		coeff.fCoeffD          = p_param->fCoeffD[i];
		coeff.fCoeffE          = p_param->fCoeffE[i];
		coeff.fCoeffF          = p_param->fCoeffF[i];

		if(p_param->uvpack[i]){
			request.format     = AFFINE_FORMAT_FORMAT_16BITS_UVPACK;
			request.uiHeight   = (p_param->h>>1);
		}else{
			request.format     = AFFINE_FORMAT_FORMAT_8BITS;
			request.uiHeight   = p_param->h;
		}
		request.uiWidth        = p_param->w;
		request.pSrcImg        = &s_img;
		request.pDstImg        = &d_img;
		request.pCoefficient   = &coeff;

		if(check_affine_limit(1, request.format, &s_img, &request)){
			DBG_ERR("limit check fail on src image\n");
			goto out1;
		}
		if(check_affine_limit(0, request.format, &d_img, &request)){
			DBG_ERR("limit check fail on dst image\n");
			goto out1;
		}
		if(gfx_if_affine_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_AFFINE, 0),
								&request,
								NULL,
								NULL) != 0){
			DBG_ERR("trigger affine engine fail\n");
			goto out1;
		}
	}

	ret = E_OK;

out1:

	if(gfx_if_affine_close(KDRV_CHIP0, KDRV_GFX2D_AFFINE) != 0)
		DBG_ERR("close affine engine fail\n");

out2:

	//don't use any sleep lock, affine calls neon instructions which can't be in sleep state
	//gfx_api_unlock();

	return ret;
}

static int gfx_i8_colorkey(GFX_COPY *p_param)
{
	int           ret = E_PAR;
	VDO_FRAME     src_img, dst_img;
	UINT32        va, pa;
	IRECT         dst_region;
	IPOINT        src_location;
	UINT64        start = hwclock_get_longcounter();

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	if((int)p_param->dst_img.w <= p_param->dst_x){
		DBG_ERR("dst image w(%d) can't <= x(%d)\n", p_param->dst_img.w, p_param->dst_x);
		return E_PAR;
	}

	if((int)p_param->dst_img.h <= p_param->dst_y){
		DBG_ERR("dst image h(%d) can't <= y(%d)\n", p_param->dst_img.h, p_param->dst_y);
		return E_PAR;
	}

	//gfx_api_lock();
	gfx_graph_lock(GFX_GRPH_ENGINE_1);

	pa = p_param->src_img.p_phy_addr[0];
	if(pa){
		if(force_use_physical_address){
			p_param->src_img.p_phy_addr[0] = pa;
		}else{
			va = nvtmpp_sys_pa2va(pa);
			if(!va){
				DBG_ERR("nvtmpp_sys_pa2va(%x) for src p_phy_addr[0]\n", pa);
				goto out;
			}
			p_param->src_img.p_phy_addr[0] = va;
		}
	}

	pa = p_param->dst_img.p_phy_addr[0];
	if(pa){
		if(force_use_physical_address){
			p_param->dst_img.p_phy_addr[0] = pa;
		}else{
			va = nvtmpp_sys_pa2va(pa);
			if(!va){
				DBG_ERR("nvtmpp_sys_pa2va(%x) for dst p_phy_addr[0]\n", pa);
				goto out;
			}
			p_param->dst_img.p_phy_addr[0] = va;
		}
	}

	ret = gximg_init_buf_ex(&src_img, p_param->src_img.w, p_param->src_img.h, p_param->src_img.format, p_param->src_img.lineoffset, p_param->src_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init src gximg buf\n");
		goto out;
	}

	ret = gximg_init_buf_ex(&dst_img, p_param->dst_img.w, p_param->dst_img.h, p_param->dst_img.format, p_param->dst_img.lineoffset, p_param->dst_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init dst gximg buf\n");
		goto out;
	}

	dst_region.x     = p_param->dst_x;
	dst_region.y     = p_param->dst_y;
	dst_region.w     = p_param->dst_img.w - dst_region.x;
	dst_region.h     = p_param->dst_img.h - dst_region.y;
	src_location.x   = 0;
	src_location.y   = 0;

	ret = gximg_copy_color_key_data(&dst_img, &dst_region, &src_img, &src_location, p_param->colorkey, 0, GXIMG_CP_ENG1, force_use_physical_address);

out:

	save_record(vos_task_get_tid(), GFX_USER_CMD_I8_COLORKEY, hwclock_get_longcounter() - start, ret,
				p_param->src_img.w,	p_param->src_img.h, p_param->dst_img.w, p_param->dst_img.h);

	//gfx_api_unlock();
	gfx_graph_unlock(GFX_GRPH_ENGINE_1);

	return ret;
}

static int gfx_scale_link_list_buf(GFX_SCALE_LINK_LIST_BUF *p_param)
{
	//coverity[assigned_value]
	int ret = E_PAR;

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	//gfx_api_lock();
	gfx_scale_lock();

	scale_ll_buf.phy_addr = p_param->p_addr;
	scale_ll_buf.length   = p_param->length;
	if(scale_ll_buf.phy_addr){
		if(force_use_physical_address)
			scale_ll_buf.virt_addr = 0;
		else{
			scale_ll_buf.virt_addr = nvtmpp_sys_pa2va(scale_ll_buf.phy_addr);
			if(!scale_ll_buf.virt_addr){
				DBG_ERR("fail to map block(%d)\n", scale_ll_buf.phy_addr);
				ret = ISF_ERR_INVALID_VALUE;
				goto out;
			}
		}
	}

	ret = E_OK;

out:

	//gfx_api_unlock();
	gfx_scale_unlock();

	return ret;
}

static int gfx_alpha_blend(GFX_ALPHA_BLEND *p_param)
{
	int           ret = E_PAR, i;
	VDO_FRAME     src_img, dst_img;
	UINT32        va, pa, alpha = 0;
	IPOINT        dst_location;

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	//gfx_api_lock();
	gfx_graph_lock(GFX_GRPH_ENGINE_1);

	//only yuv20/Y8/argb8888 are supported and yuv420/Y8 need alpha plane.
	//each pixel of argb8888 has its own alpha so alpha plane is unneccessary
	if(p_param->dst_img.format != VDO_PXLFMT_ARGB8888){
		if(!p_param->p_alpha){
			DBG_ERR("alpha plane is NULL\n");
			goto out;
		}else{
			if(force_use_physical_address)
				alpha = p_param->p_alpha;
			else
				alpha = nvtmpp_sys_pa2va(p_param->p_alpha);
			if(!alpha){
				DBG_ERR("nvtmpp_sys_pa2va(alpha:%x) fail\n", p_param->p_alpha);
				goto out;
			}
		}
	}

	for(i = 0 ; i < GFX_MAX_PLANE_NUM ; ++i){
		pa = p_param->src_img.p_phy_addr[i];
		if(pa){
			if(force_use_physical_address)
				p_param->src_img.p_phy_addr[i] = pa;
			else{
				va = nvtmpp_sys_pa2va(pa);
				if(!va){
					DBG_ERR("nvtmpp_sys_pa2va(%x) for src p_phy_addr[%d]\n", pa, i);
					goto out;
				}
				p_param->src_img.p_phy_addr[i] = va;
			}
		}

		pa = p_param->dst_img.p_phy_addr[i];
		if(pa){
			if(force_use_physical_address)
				p_param->dst_img.p_phy_addr[i] = pa;
			else{
				va = nvtmpp_sys_pa2va(pa);
				if(!va){
					DBG_ERR("nvtmpp_sys_pa2va(%x) for dst p_phy_addr[%d]\n", pa, i);
					goto out;
				}
				p_param->dst_img.p_phy_addr[i] = va;
			}
		}
	}

	ret = gximg_init_buf_ex(&src_img, p_param->src_img.w, p_param->src_img.h, p_param->src_img.format, p_param->src_img.lineoffset, p_param->src_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init src gximg buf\n");
		goto out;
	}

	ret = gximg_init_buf_ex(&dst_img, p_param->dst_img.w, p_param->dst_img.h, p_param->dst_img.format, p_param->dst_img.lineoffset, p_param->dst_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init dst gximg buf\n");
		goto out;
	}

	dst_location.x   = p_param->dst_x;
	dst_location.y   = p_param->dst_y;

	if (check_flush(p_param->flush)) {
		ret = gximg_copy_blend_data_ex(&src_img, GXIMG_REGION_MATCH_IMG, &dst_img, &dst_location, (UINT8*)alpha, force_use_physical_address);
	} else {
		ret = gximg_copy_blend_data_ex_no_flush(&src_img, GXIMG_REGION_MATCH_IMG, &dst_img, &dst_location, (UINT8*)alpha, force_use_physical_address);
	}

out:

	//gfx_api_unlock();
	gfx_graph_unlock(GFX_GRPH_ENGINE_1);

	return ret;
}

static GRPH_IMG       raw_images[2*3]    = {0};
static GRPH_PROPERTY  raw_property[2*5]  = {0};
static GRPH_INOUTOP   raw_inoutop[2*3]   = {0};
static GRPH_QUAD_DESC raw_quad[2*5]      = {0};

static int gfx_raw_graphic(GFX_RAW_GRAPHIC *p_param)
{
	//coverity[assigned_value]
	int                     ret          = E_PAR;
	int                     i;
	GFX_GRPH_TRIGGER_PARAM  param        = {0};
	KDRV_GRPH_TRIGGER_PARAM request      = {0};
	GRPH_IMG       			*images      = NULL;
	GRPH_PROPERTY           *property    = NULL;
	GRPH_INOUTOP            *inoutop     = NULL;
	GRPH_QUAD_DESC          *quad        = NULL;

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	if(p_param->length != sizeof(GFX_GRPH_TRIGGER_PARAM)){
		DBG_ERR("user space length(%d) != kernel space length(%d)\n", p_param->length, sizeof(GFX_GRPH_TRIGGER_PARAM));
		return E_PAR;
	}

	if(3 != ((sizeof(param.images)/sizeof(GFX_GRPH_IMG)))){
		DBG_ERR("images number mismatch %d vs %d\n", 3, (sizeof(param.images)/sizeof(GFX_GRPH_IMG)));
		return E_PAR;
	}

	if(5 != ((sizeof(param.property)/sizeof(GFX_GRPH_PROPERTY)))){
		DBG_ERR("property number mismatch %d vs %d\n", 5, (sizeof(param.property)/sizeof(GFX_GRPH_PROPERTY)));
		return E_PAR;
	}

	#if defined(__LINUX)
	if(gfx_copy_from_user(&param, (void*)p_param->p_addr, sizeof(param))){
		DBG_ERR("copy_from_user() fail\r\n");
		return -EFAULT;
	}
	#else
	gfx_memcpy(&param, (void*)p_param->p_addr, sizeof(param));
	#endif
	
	if(p_param->engine == 0){
		images   = &(raw_images[0]);
		property = &(raw_property[0]);
		inoutop  = &(raw_inoutop[0]);
		quad     = &(raw_quad[0]);
	}else{
		images   = &(raw_images[3]);
		property = &(raw_property[5]);
		inoutop  = &(raw_inoutop[3]);
		quad     = &(raw_quad[5]);
	}

	gfx_graph_lock(p_param->engine == 0 ? GFX_GRPH_ENGINE_1 : GFX_GRPH_ENGINE_2);

	gfx_memset(images, 0, sizeof(GRPH_IMG)*3);
	gfx_memset(property, 0, sizeof(GRPH_PROPERTY)*5);
	gfx_memset(inoutop, 0, sizeof(GRPH_INOUTOP)*3);
	gfx_memset(quad, 0, sizeof(GRPH_QUAD_DESC)*5);

	request.command    = param.command;
	request.format     = param.format;

	request.is_buf_pa  = force_use_physical_address;
	if (force_use_physical_address) {
		request.is_skip_cache_flush = 1;
	} else if (p_param->flush) {
		request.is_skip_cache_flush = 0;
	} else {
		request.is_skip_cache_flush = 1;
	}
	//setup images
	for(i = 0 ; i < 3 ; ++i){
		if(param.images[i].dram_addr == 0 || param.images[i].lineoffset == 0)
			break;

		if(force_use_physical_address)
			images[i].dram_addr = param.images[i].dram_addr;
		else
			images[i].dram_addr = nvtmpp_sys_pa2va(param.images[i].dram_addr);
		if(!images[i].dram_addr){
			DBG_ERR("nvtmpp_sys_pa2va(%x) for images[%d]\n", images[i].dram_addr, i);
			ret = -1;
			goto out;
		}
		images[i].img_id     = param.images[i].img_id;
		images[i].lineoffset = param.images[i].lineoffset;
		images[i].width      = param.images[i].width;
		images[i].height     = param.images[i].height;

		if(param.images[i].inoutop.en){
			inoutop[i].id           = param.images[i].inoutop.id;
			inoutop[i].op           = param.images[i].inoutop.op;
			inoutop[i].shifts       = param.images[i].inoutop.shifts;
			inoutop[i].constant     = param.images[i].inoutop.constant;
			images[i].p_inoutop     = &(inoutop[i]);
		}

		if(i == 0)
			request.p_images = &(images[i]);
		if(i <= 1 && param.images[i+1].dram_addr != 0 && param.images[i+1].lineoffset != 0)
			images[i].p_next = &(images[i+1]);
	}

	//setup properties
	for(i = 0 ; i < 5 ; ++i){
		if(param.property[i].en == 0)
			break;
		property[i].id = param.property[i].id;

		if(param.property[i].id  == GRPH_PROPERTY_ID_NORMAL                  ||
					param.property[i].id == GRPH_PROPERTY_ID_U               ||
					param.property[i].id == GRPH_PROPERTY_ID_V               ||
					param.property[i].id == GRPH_PROPERTY_ID_R               ||
					param.property[i].id == GRPH_PROPERTY_ID_G               ||
					param.property[i].id == GRPH_PROPERTY_ID_B               ||
					param.property[i].id == GRPH_PROPERTY_ID_A               ||
					param.property[i].id == GRPH_PROPERTY_ID_ACC_SKIPCTRL    ||
					param.property[i].id == GRPH_PROPERTY_ID_ACC_FULL_FLAG   ||
					param.property[i].id == GRPH_PROPERTY_ID_PIXEL_CNT       ||
					param.property[i].id == GRPH_PROPERTY_ID_VALID_PIXEL_CNT ||
					param.property[i].id == GRPH_PROPERTY_ID_ACC_RESULT      ||
					param.property[i].id == GRPH_PROPERTY_ID_ACC_RESULT2     ||
					param.property[i].id == GRPH_PROPERTY_ID_YUVFMT          ||
					param.property[i].id == GRPH_PROPERTY_ID_ALPHA0_INDEX    ||
					param.property[i].id == GRPH_PROPERTY_ID_ALPHA1_INDEX    ||
					param.property[i].id == GRPH_PROPERTY_ID_INVRGB          ||
					param.property[i].id == GRPH_PROPERTY_ID_MOSAIC_SRC_FMT  ||
					param.property[i].id == GRPH_PROPERTY_ID_UV_SUBSAMPLE){
			property[i].property = param.property[i].data.property;
		}else if(param.property[i].id == GRPH_PROPERTY_ID_QUAD_PTR ||
					param.property[i].id == GRPH_PROPERTY_ID_QUAD_INNER_PTR){
			quad[i].top_left.x     = param.property[i].data.quad.top_left.x;
			quad[i].top_left.y     = param.property[i].data.quad.top_left.y;
			quad[i].top_right.x    = param.property[i].data.quad.top_right.x;
			quad[i].top_right.y    = param.property[i].data.quad.top_right.y;
			quad[i].bottom_right.x = param.property[i].data.quad.bottom_right.x;
			quad[i].bottom_right.y = param.property[i].data.quad.bottom_right.y;
			quad[i].bottom_left.x  = param.property[i].data.quad.bottom_left.x;
			quad[i].bottom_left.y  = param.property[i].data.quad.bottom_left.y;
			quad[i].blend_en       = param.property[i].data.quad.blend_en;
			quad[i].alpha          = param.property[i].data.quad.alpha;
			quad[i].mosaic_width   = param.property[i].data.quad.mosaic_width;
			quad[i].mosaic_height  = param.property[i].data.quad.mosaic_height;
			property[i].property   = (UINT32)&(quad[i]);
		}else if(param.property[i].id == GRPH_PROPERTY_ID_LUT_BUF ||
					param.property[i].id == GRPH_PROPERTY_ID_PAL_BUF){
			if(param.property[i].data.property == 0){
				DBG_ERR("GRPH_PROPERTY_ID_LUT_BUF[%d] has no 1d lut buf\n", i);
				ret = -1;
				goto out;
			}
			property[i].property = nvtmpp_sys_pa2va(param.property[i].data.property);
			if(property[i].property == 0){
				DBG_ERR("nvtmpp_sys_pa2va(%x) for property[%d]\n", param.property[i].data.property, i);
				ret = -1;
				goto out;
			}
		}else{
			DBG_ERR("property id(%d) is not supported\n", param.property[i].id);
			ret = -1;
			goto out;
		}

		if(i == 0)
			request.p_property = &(property[i]);
		if(i <= 3 && param.property[i+1].en)
			property[i].p_next = &(property[i+1]);
	}

	//trigger graphic hw
	//gfx_api_lock();
	ret = gximg_raw_graphic(p_param->engine, &request);
	if(ret != E_OK)
		DBG_ERR("gximg_raw_graphic() fail with %d\n", ret);
	//gfx_api_unlock();

	//copy acc result back to user space
	for(i = 0 ; i < 5 ; ++i){
		if(param.property[i].en == 0)
			break;
		if(param.property[i].id != GRPH_PROPERTY_ID_ACC_RESULT && param.property[i].id != GRPH_PROPERTY_ID_ACC_RESULT2)
			continue;
		#if defined(__LINUX)
		if(gfx_copy_to_user(&(((GFX_GRPH_TRIGGER_PARAM*)(p_param->p_addr))->property[i].data.property), &(property[i].property), sizeof(property[i].property))){
			DBG_ERR("copy_to_user() fail\r\n");
			ret = -1;
			goto out;
		}
		#else
		gfx_memcpy(&(param.property[i].data.property), &(property[i].property), sizeof(property[i].property));
		#endif
	}
out:
	gfx_graph_unlock(p_param->engine == 0 ? GFX_GRPH_ENGINE_1 : GFX_GRPH_ENGINE_2);

	return ret;
}

static int gfx_force_use_physical_address(GFX_FORCE_USE_PHYSICAL_ADDRESS *p_param)
{
	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	gfx_all_lock();

	force_use_physical_address = p_param->force_physical_address;

	gfx_all_unlock();

	return 0;
}

static int gfx_draw_mask(GFX_DRAW_MASK *p_param)
{
	int                      ret = E_PAR, i;
	VDO_FRAME                frame;
	UINT32                   va, pa;
	IRECT                    dst_region = {0};
	UINT64                   start = hwclock_get_longcounter();
	GXIMG_COVER_DESC         desc;

	if(!p_param){
		DBG_ERR("p_param is NULL\n");
		return E_PAR;
	}

	//gfx_api_lock();
	gfx_graph_lock(GFX_GRPH_ENGINE_2);

	for(i = 0 ; i < GFX_MAX_PLANE_NUM ; ++i){
		pa = p_param->dst_img.p_phy_addr[i];
		if(pa){
			if(force_use_physical_address){
				p_param->dst_img.p_phy_addr[i] = pa;
			}else{
				va = nvtmpp_sys_pa2va(pa);
				if(!va){
					DBG_ERR("nvtmpp_sys_pa2va(%x) for p_phy_addr[%d]\n", pa, i);
					goto out;
				}
				p_param->dst_img.p_phy_addr[i] = va;
			}
		}
	}

	ret = gximg_init_buf_ex(&frame, p_param->dst_img.w, p_param->dst_img.h, p_param->dst_img.format, p_param->dst_img.lineoffset, p_param->dst_img.p_phy_addr);
	if(ret != E_OK){
		DBG_ERR("fail to init gximg buf\n");
		goto out;
	}

	gfx_memset(&desc, 0, sizeof(GXIMG_COVER_DESC));
	desc.top_left.x     = p_param->x[0];
	desc.top_left.y     = p_param->y[0];
	desc.top_right.x    = p_param->x[1];
	desc.top_right.y    = p_param->y[1];
	desc.bottom_right.x = p_param->x[2];
	desc.bottom_right.y = p_param->y[2];
	desc.bottom_left.x  = p_param->x[3];
	desc.bottom_left.y  = p_param->y[3];
	desc.yuva           = argb_to_ycbcr(p_param->color);
	desc.yuva          |= (p_param->color & 0xFF000000); //alpha

	dst_region.x = 0;
	dst_region.y = 0;
	dst_region.w = frame.size.w;
	dst_region.h = frame.size.h;

	if(p_param->flush)
		ret = gximg_quad_cover(&desc, &frame, &dst_region, p_param->type == GFX_RECT_TYPE_HOLLOW, p_param->thickness, force_use_physical_address);
	else
		ret = gximg_quad_cover_with_flush(&desc, &frame, &dst_region, p_param->type == GFX_RECT_TYPE_HOLLOW, p_param->thickness, 1, force_use_physical_address);

out:

	save_record(vos_task_get_tid(), GFX_USER_CMD_DRAW_RECT, hwclock_get_longcounter() - start, ret,
				p_param->dst_img.w,	p_param->dst_img.h, dst_region.w, dst_region.h);

	//gfx_api_unlock();
	gfx_graph_unlock(GFX_GRPH_ENGINE_2);

	return ret;
}

static char *get_rotate_str(GFX_ROTATE_ANGLE angle)
{
	switch(angle){
		case GFX_ROTATE_ANGLE_90 :
			return "90";
		case GFX_ROTATE_ANGLE_180 :
			return "180";
		case GFX_ROTATE_ANGLE_270 :
			return "270";
		case GFX_ROTATE_ANGLE_MIRROR_X :
			return "mirrorx";
		case GFX_ROTATE_ANGLE_MIRROR_Y :
			return "mirrory";
		default :
			return "";
	}
}

static char *get_fmt_str(VDO_PXLFMT fmt)
{
	switch(fmt){
		case VDO_PXLFMT_RGB888_PLANAR :
			return "rgb8883p";
		case VDO_PXLFMT_RGB888 :
			return "rgb888";
		case VDO_PXLFMT_RGB565 :
			return "rgb565";
		case VDO_PXLFMT_ARGB1555 :
			return "argb1555";
		case VDO_PXLFMT_ARGB4444 :
			return "argb4444";
		case VDO_PXLFMT_YUV420_PLANAR :
			return "yuv4203p";
		case VDO_PXLFMT_YUV420 :
			return "yuv420";
		case VDO_PXLFMT_YUV422_PLANAR :
			return "yuv4223p";
		case VDO_PXLFMT_YUV422 :
			return "yuv422";
		case VDO_PXLFMT_YUV444_PLANAR :
			return "yuv4443p";
		case VDO_PXLFMT_YUV444 :
			return "yuv444";
		default :
			return "";
	}
}

static char *get_arith_str(GFX_ARITH_OP op, GFX_ARITH_BIT bit)
{
	if(op == GFX_ARITH_OP_PLUS){
		if(bit == GFX_ARITH_BIT_8)
			return "+(8)";
		else if(bit == GFX_ARITH_BIT_16)
			return "+(16)";
	}else if(op == GFX_ARITH_OP_MINUS){
		if(bit == GFX_ARITH_BIT_8)
			return "-(8)";
		else if(bit == GFX_ARITH_BIT_16)
			return "-(16)";
	}else if(op == GFX_ARITH_OP_MULTIPLY){
		if(bit == GFX_ARITH_BIT_8)
			return "x(8)";
		else if(bit == GFX_ARITH_BIT_16)
			return "x(16)";
	}

	return "";
}

static void dump_record(int idx, void *m)
{
	char              msg[512];

	if(idx < 0 || idx > 127){
		DBG_ERR("invalid idx(%d)\n", idx);
		return;
	}

	switch(record.cmd[idx]){
		case GFX_USER_CMD_COPY:
			gfx_snprintf(msg, sizeof(msg), "%d\tcopy\t%d\t%llu\t\tsw(%d) sh(%d) dw(%d) dh(%d)\n",
				record.pid[idx], record.result[idx], record.time[idx], record.p1[idx], record.p2[idx], record.p3[idx], record.p4[idx]);
			gfx_seq_printf(m, msg);
			break;
		case GFX_USER_CMD_DMA_COPY:
			gfx_snprintf(msg, sizeof(msg), "%d\tdmacopy\t%d\t%llu\t\tlen(%d)\n",
				record.pid[idx], record.result[idx], record.time[idx], record.p1[idx]);
			gfx_seq_printf(m, msg);
			break;
		case GFX_USER_CMD_SCALE:
			gfx_snprintf(msg, sizeof(msg), "%d\tscale\t%d\t%llu\t\tsw(%d) sh(%d) dw(%d) dh(%d)\n",
				record.pid[idx], record.result[idx], record.time[idx], record.p1[idx], record.p2[idx], record.p3[idx], record.p4[idx]);
			gfx_seq_printf(m, msg);
			break;
		case GFX_USER_CMD_ROTATE:
			gfx_snprintf(msg, sizeof(msg), "%d\trotate\t%d\t%llu\t\tw(%d) h(%d) %s\n",
				record.pid[idx], record.result[idx], record.time[idx], record.p1[idx], record.p2[idx], get_rotate_str(record.p3[idx]));
			gfx_seq_printf(m, msg);
			break;
		case GFX_USER_CMD_COLOR_TRANSFORM:
			gfx_snprintf(msg, sizeof(msg), "%d\tct\t%d\t%llu\t\tw(%d) h(%d) %s=>%s\n",
				record.pid[idx], record.result[idx], record.time[idx], record.p1[idx], record.p2[idx], get_fmt_str(record.p3[idx]), get_fmt_str(record.p4[idx]));
			gfx_seq_printf(m, msg);
			break;
		case GFX_USER_CMD_DRAW_LINE:
			gfx_snprintf(msg, sizeof(msg), "%d\tline\t%d\t%llu\t\tw(%d) h(%d) area(%d)\n",
				record.pid[idx], record.result[idx], record.time[idx], record.p1[idx], record.p2[idx], record.p3[idx]);
			gfx_seq_printf(m, msg);
			break;
		case GFX_USER_CMD_DRAW_RECT:
			gfx_snprintf(msg, sizeof(msg), "%d\trect\t%d\t%llu\t\tw(%d) h(%d) rw(%d) rh(%d)\n",
				record.pid[idx], record.result[idx], record.time[idx], record.p1[idx], record.p2[idx], record.p3[idx], record.p4[idx]);
			gfx_seq_printf(m, msg);
			break;
		case GFX_USER_CMD_ARITHMETIC:
			gfx_snprintf(msg, sizeof(msg), "%d\t%s\t%d\t%llu\t\tlength(%d)\n",
				record.pid[idx], get_arith_str(record.p1[idx], record.p2[idx]), record.result[idx], record.time[idx], record.p3[idx]);
			gfx_seq_printf(m, msg);
			break;
		default :
			break;
	}
}

int gfx_info_show(void *m, void *v) {

	int i;

	//gfx_api_lock();
	gfx_all_lock();

	if(record.cur == -1){
		gfx_seq_printf(m, "no gfx operation had been submit\n");
		goto out;
	}

	gfx_seq_printf(m, "pid\taction\tret\ttime(us)\tinfo\n");

	for(i = record.cur ; i >= 0 ; --i)
		dump_record(i, m);

	for(i = 127 ; i > record.cur ; --i)
		dump_record(i, m);

out:
	//gfx_api_unlock();
	gfx_all_unlock();

	return 0;
}

int gfx_cmd_showhelp(void *m, void *v) {

	gfx_seq_printf(m,
#if defined(__LINUX)
		"cat /proc/hdal/gfx/info to learn execution time of all operations\n"
#else
		"execute \"gfx info\" under \">\" to learn execution time of all operations\n"
#endif
		);

	return 0;
}

int nvt_gfx_init(void)
{
	int                      i;

	gfx_if_affine_open = kdrv_gfx_if_affine_open;
	gfx_if_affine_trigger = kdrv_gfx_if_affine_trigger;
	gfx_if_affine_close = kdrv_gfx_if_affine_close;

	gfx_memset(&scale_ll_buf, 0, sizeof(SCALE_LINK_LIST_BUF));
	gfx_memset(&record, 0, sizeof(RECORD));
	record.cur = -1;
	force_use_physical_address = 0;
	for(i = 0 ; i < MAX_RECORD ; ++i)
		record.cmd[i] = GFX_USER_CMD_NULL;

	//SEM_CREATE(GFX_SEM_ID, 1);
	SEM_CREATE(GRAPH1_SEM_ID, 1);
	SEM_CREATE(GRAPH2_SEM_ID, 1);
	SEM_CREATE(SCALE_SEM_ID, 1);
	SEM_CREATE(GFX_JOB_SEM_ID, 1);
	return 0;
}

void nvt_gfx_exit(void)
{
	gfx_memset(&scale_ll_buf, 0, sizeof(SCALE_LINK_LIST_BUF));
	//SEM_DESTROY(GFX_SEM_ID);
	SEM_DESTROY(GRAPH1_SEM_ID);
	SEM_DESTROY(GRAPH2_SEM_ID);
	SEM_DESTROY(SCALE_SEM_ID);
	SEM_DESTROY(GFX_JOB_SEM_ID);
}

int nvt_gfx_ioctl(int f, unsigned int cmd, void *arg)
{
	GFX_USER_DATA    user_data;
	int              ret = E_PAR;

#if defined(__LINUX)
	if(gfx_copy_from_user(&user_data, arg, sizeof(user_data))){
		DBG_ERR("gfx_copy_from_user() fail\r\n");
		return -EFAULT;
	}
#else
	memcpy(&user_data, arg, sizeof(user_data));
#endif

	if(user_data.version != GFX_USER_VERSION){
		DBG_ERR("mismatch version : lib =%d ; ko = %d\r\n", user_data.version, GFX_USER_VERSION);
		return E_PAR;
	}

	if(user_data.cmd <= GFX_USER_CMD_NULL || user_data.cmd >= GFX_USER_CMD_MAX){
		DBG_ERR("invalid command = %d\r\n", user_data.cmd);
		return E_PAR;
	}

	switch(user_data.cmd){

		case GFX_USER_CMD_INIT :

			ret = gfx_init();

			break;

		case GFX_USER_CMD_UNINIT :

			ret = gfx_uninit();

			break;

		case GFX_USER_CMD_COPY :

			ret = gfx_copy(&user_data.data.copy);

			break;

		case GFX_USER_CMD_DMA_COPY :

			ret = gfx_dma_copy(&user_data.data.dma_copy);

			break;

		case GFX_USER_CMD_SCALE :

			ret = gfx_scale(&user_data.data.scale);

			break;

		case GFX_USER_CMD_ROTATE :

			ret = gfx_rotate(&user_data.data.rotate);

			break;

		case GFX_USER_CMD_COLOR_TRANSFORM :

			ret = gfx_color_transform(&user_data.data.color_transform);

			break;

		case GFX_USER_CMD_DRAW_LINE :

			ret = gfx_draw_line(&user_data.data.draw_line, 0);

			break;

		case GFX_USER_CMD_DRAW_RECT :

			ret = gfx_draw_rect(&user_data.data.draw_rect);

			break;

		case GFX_USER_CMD_ARITHMETIC :

			ret = gfx_arithmetic(&user_data.data.arithmetic);

			break;

		case GFX_USER_CMD_JOB_LIST :

			ret = gfx_job_list((unsigned long)arg);

			break;

		case GFX_USER_CMD_ISE_SCALE_Y8 :

			ret = gfx_ise_scale_y8(&user_data.data.ise_scale_y8);

			break;

		case GFX_USER_CMD_AFFINE :

			ret = gfx_affine(&user_data.data.affine);

			break;

		case GFX_USER_CMD_I8_COLORKEY :

			ret = gfx_i8_colorkey(&user_data.data.copy);

			break;

		case GFX_USER_CMD_SCALE_LINK_LIST_BUF :

			ret = gfx_scale_link_list_buf(&user_data.data.scale_link_list_buf);

			break;

		case GFX_USER_CMD_SCALE_LINK_LIST :

			ret = gfx_scale_link_list((unsigned long)arg);

			break;

		case GFX_USER_CMD_ALPHA_BLEND :

			ret = gfx_alpha_blend(&user_data.data.alpha_blend);

			break;

		case GFX_USER_CMD_RAW_GRAPHIC :

			ret = gfx_raw_graphic(&user_data.data.raw_graphic);

			break;

		case GFX_USER_CMD_FORCE_USE_PHYSICAL_ADDRESS :

			ret = gfx_force_use_physical_address(&user_data.data.force_use_physical_address);

			break;

		case GFX_USER_CMD_DRAW_LINE_BY_CPU :

			ret = gfx_draw_line(&user_data.data.draw_line, 1);

			break;

		case GFX_USER_CMD_DRAW_MASK :

			ret = gfx_draw_mask(&user_data.data.mask);

			break;

		default :
			DBG_ERR("unsupported command = %d\r\n", user_data.cmd);
			break;
	};

	return ret;
}

#if defined(__LINUX)
#else

void gfx_memset(void *buf, unsigned char val, int len)
{
	memset(buf, val, len);
}

void gfx_memcpy(void *buf, void *src, int len)
{
	memcpy(buf, src, len);
}

void* gfx_alloc(int size)
{
	return malloc(size);
}

void gfx_free(void *buf)
{
	free(buf);
}

int gfx_seq_printf(void *m, const char *fmtstr, ...)
{
	char    buf[512];
	int     len;

	va_list marker;

	va_start(marker, fmtstr);

	len = vsnprintf(buf, sizeof(buf), fmtstr, marker);
	va_end(marker);
	if (len > 0)
		DBG_DUMP(buf);
	return 0;
}

int gfx_snprintf(char *buf, int size, const char *fmtstr, ...)
{
	int     len;

	va_list marker;

	va_start(marker, fmtstr);

	len = vsnprintf(buf, size, fmtstr, marker);
	va_end(marker);

	return len;
}

MAINFUNC_ENTRY(gfx, argc, argv)
{
	if (argc < 1) {
		return -1;
	}

	if (strncmp(argv[1], "?", 2) == 0) {
		gfx_cmd_showhelp(NULL, NULL);
		return 0;
	}

	if (strncmp(argv[1], "info", strlen(argv[1])) == 0) {
		gfx_info_show(NULL, NULL);
	}else{
		DBG_ERR("Invalid CMD !!\r\n");
		gfx_cmd_showhelp(NULL, NULL);
		return -1;
	}

	return 0;
}

#endif
