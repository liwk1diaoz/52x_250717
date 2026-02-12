#include "isf_vdoout_int.h"
#include "isf_vdoout_dbg.h"
#include "vdodisp_api.h"
#include "kdrv_videoout/kdrv_vdoout.h"
#include "kdrv_videoout/kdrv_vdoout_lmt.h"
#include <gximage/gximage.h>
#include <kwrap/spinlock.h>
#include "kflow_common/nvtmpp.h"
#include "kflow_common/isf_flow_core.h"
#define nvtmpp_vb_block2addr(args...)       nvtmpp_vb_blk2va(args)

#if defined(__LINUX)
#include <linux/string.h>
#else
#include "string.h"
#endif
#define OSG_ROTATE      (1)
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  mapping
//  In1 <--> Out1
///////////////////////////////////////////////////////////////////////////////
//STATIC_ASSERT(ISF_VDOOUT_INPUT_MAX == ISF_VDOOUT_OUTPUT_MAX);

#define ISF_UNIT_TRACE(opclass, p_thisunit, port, fmtstr, args...) p_thisunit->p_base->do_trace(p_thisunit, port, opclass, fmtstr, ##args)

#define FORCE_DUMP_DATA     DISABLE
#define AUTO_DROP           DISABLE
#define NO_WAIT             DISABLE //ENABLE
#define VO_FRAME_DBG        DISABLE

#if VO_FRAME_DBG
#include "comm/ddr_arb.h"
#endif

typedef enum _VDOOUT_STATE {
	VDOOUT_STATE_SYNC     = 0,   ///<
	VDOOUT_STATE_START,          ///<
	VDOOUT_STATE_RUNTIME_SYNC,   ///<
	VDOOUT_STATE_STOP,           ///<
	VDOOUT_STATE_MAX,
	ENUM_DUMMY4WORD(VDOOUT_STATE)
} VDOOUT_STATE;

#define ISF_VDOOUT_FALG_INITED        MAKEFOURCC('V', 'O', 'U', 'T')
#define ISF_VDOOUT_FALG_OPEN          0x55AA


#define FORCE_DUMP_DATA     DISABLE
#define AUTO_DROP           DISABLE
#define NO_WAIT             DISABLE //ENABLE

static ISF_DATA gISF_DispTempData[DEV_ID_MAX] = {0};
static UINT32 g_vdoout_init[ISF_FLOW_MAX] = {0};
static ISF_UNIT *isf_volist[DEV_ID_MAX] = {
	&isf_vdoout0,
#if (DEV_ID_MAX > 1)
	&isf_vdoout1,
#endif
};

static KDRV_DEV_ENGINE isf_vo_engine[DEV_ID_MAX] = {
	KDRV_VDOOUT_ENGINE0,
#if (DEV_ID_MAX > 1)
	KDRV_VDOOUT_ENGINE1,
#endif
};

static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

#define HD_VIDEO_PXLFMT_BPP_MASK		0x00ff0000
#define HD_VIDEO_PXLFMT_BPP(pxlfmt) 	(((pxlfmt) & HD_VIDEO_PXLFMT_BPP_MASK) >> 16)
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	(ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
#define DEV_UNIT(did)	isf_volist[(did)]
#define QUEUE_MAXSIZE   3
#define QUEUE_DEFSIZE   2
#define QUEUE_EMPTY     0xff


typedef struct _VDOQUEUE {
	UINT32 Total;
	UINT32 Cnt;
	UINT32 Head;
	UINT32 Tail;
	ISF_DATA Data[QUEUE_MAXSIZE];
}
VDOQUEUE;

static VDOQUEUE gQueue[DEV_ID_MAX] = {
	{QUEUE_DEFSIZE, 0, 0, QUEUE_EMPTY},
#if (DEV_ID_MAX > 1)
	{QUEUE_DEFSIZE, 0, 0, QUEUE_EMPTY},
#endif
};
SEM_HANDLE ISF_VDOOUT_PROC_SEM_ID = {0};

static ISF_RV _isf_vdoout_close(UINT32 id, ISF_UNIT  *p_thisunit, UINT32 oport);

void isf_vdoout_install_id(void)
{
	OS_CONFIG_SEMPHORE(ISF_VDOOUT_PROC_SEM_ID, 0, 1, 1);
}

void isf_vdoout_uninstall_id(void)
{
	SEM_DESTROY(ISF_VDOOUT_PROC_SEM_ID);
}

static UINT32 get_fmt_coef(UINT32 fmt)
{
	switch(fmt) {
		case VDOOUT_BUFTYPE_ARGB1555:
			return 2;
		case VDOOUT_BUFTYPE_ARGB8888:
			return 4;
		case VDOOUT_BUFTYPE_ARGB4444:
			return 2;
		case VDOOUT_BUFTYPE_ARGB8565:
			return 2; //RGB plan lineoffset
		case VDOOUT_BUFTYPE_YUV420PACK:
			return 1;
		case VDOOUT_BUFTYPE_YUV422PACK:
			return 1;
        case VDOOUT_BUFTYPE_PAL8:
			return 1;
		default:
			DBG_ERR("not supp %d\r\n",fmt);
			return 1;
	}
}
static void _isf_vout_memset_ctx(UINT32 id)
{
    /* flags should be local variable */
    unsigned long      flags;
    ISF_UNIT* p_unit = DEV_UNIT(id);
    VDOOUT_CONTEXT* p_ctx = (VDOOUT_CONTEXT*)(p_unit->refdata);

    if(id<DEV_ID_MAX) {
        memset((void *)p_ctx,0,sizeof(VDOOUT_CONTEXT));
        //reset queue
        loc_cpu(flags);
        memset(&gQueue[id],0,sizeof(VDOQUEUE));
        gQueue[id].Total = QUEUE_DEFSIZE;
        gQueue[id].Tail = QUEUE_EMPTY;
        unl_cpu(flags);
    }
}
static ISF_RV _isf_vdoout_do_init(UINT32 h_isf, UINT32 dev_max_count)
{
	ISF_RV rv = ISF_OK;
    UINT32 i=0;

	if (h_isf > 0) { //client process
		if (!g_vdoout_init[0]) { //is master process already init?
			rv = ISF_ERR_INIT; //not allow init from client process!
			goto _VOUT_INIT_ERR;
		}
		g_vdoout_init[h_isf] = 1; //set init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_vdoout_init[i]) { //client process still alive?
				rv = ISF_ERR_INIT; //not allow init from main process!
				goto _VOUT_INIT_ERR;
			}
		}
	}


    if(g_vdoout_init[0]){
        return ISF_ERR_INIT;
    } else {
        g_vdoout_init[0] = 1;
        for(i=0;i<DEV_ID_MAX;i++) {
            _isf_vout_memset_ctx(i);
#if 0//config by display dts
            p_ctx->dev_cfg.if_cfg.lcd_ctrl = VDDO_DISPDEV_LCDCTRL_SIF;
#if !defined(_BSP_NA51000_)
            p_ctx->dev_cfg.if_cfg.ui_sif_ch = VDDO_SIF_CH2;
#else
            p_ctx->dev_cfg.if_cfg.ui_sif_ch = VDDO_SIF_CH1;
#endif
#endif
        }
    }

_VOUT_INIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("init fail! %d\r\n", rv);
	}

    return rv;
}


static ISF_RV _isf_vdoout_do_uninit(UINT32 h_isf, UINT32 reset)
{
	ISF_RV rv = ISF_OK;
    UINT32 i=0;

	if (h_isf > 0) { //client process
		if (!g_vdoout_init[0]) { //is master process already init?
			rv = ISF_ERR_UNINIT; //not allow uninit from client process!
			goto _VOUT_UNINIT_ERR;
		}
		g_vdoout_init[h_isf] = 0; //clear init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_vdoout_init[i] && !reset) { //client process still alive?
				rv = ISF_ERR_UNINIT; //not allow uninit from main process!
				goto _VOUT_UNINIT_ERR;
			}
			if (reset) {
				g_vdoout_init[i] = 0; //force clear client
			}
		}
	}

	if (!g_vdoout_init[0]) {
		return ISF_ERR_UNINIT; //still not init
	}

    for(i=0;i<DEV_ID_MAX;i++){
        ISF_UNIT* p_unit = DEV_UNIT(i);
	    VDOOUT_CONTEXT* p_ctx = (VDOOUT_CONTEXT*)(p_unit->refdata);
        if(p_ctx->open_tag) {
            if(p_ctx->flow_ctrl & VDOOUT_FUNC_MERGE_WIN) {
                p_ctx->flow_ctrl = p_ctx->flow_ctrl & (~VDOOUT_FUNC_MERGE_WIN);
                DBG_IND("flow_ctrl %x \r\n",p_ctx->flow_ctrl);
                rv=_isf_vdoout_close(i,p_unit, 0);
                if(rv!=ISF_OK)
                    return ISF_ERR_UNINIT;
            } else {
            DBG_ERR("not close\r\n");
            return ISF_ERR_UNINIT;
            }
        }
    }
	g_vdoout_init[0] = 0;

_VOUT_UNINIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("uninit fail! %d\r\n", rv);
	}

	return rv;
}

#define USER_LCD_OFFSET (0x10000000)

ISF_RV isf_vdoout_open_dev(UINT32 id,const VDOOUT_DEV_CFG *drv_cfg)
{
	ISF_RV ret = ISF_OK;
	KDRV_VDDO_DISPDEV_PARAM kdrv_disp_dev = {0};
	KDRV_VDDO_DISPCTRL_PARAM kdrv_disp_ctrl = {0};
    UINT32 user_data = 0;
    BOOL user_data_enable = 0;
	UINT32 handler = KDRV_DEV_ID(0, isf_vo_engine[id], 0);
	ret=kdrv_vddo_open(0, isf_vo_engine[id]);
    if(ret!=ISF_OK) {
        return ISF_ERR_DRV;
    }
	if(drv_cfg->mode.output_type ==VDOOUT_PANEL){
        #if 0//config by display dts
        #if defined(CONFIG_NVT_FPGA_EMULATION) || defined(_NVT_FPGA_)
		kdrv_disp_dev.SEL.KDRV_VDDO_REG_IF.lcd_ctrl = VDDO_DISPDEV_LCDCTRL_GPIO;
		kdrv_disp_dev.SEL.KDRV_VDDO_REG_IF.ui_sif_ch = VDDO_SIF_CH1;
		kdrv_disp_dev.SEL.KDRV_VDDO_REG_IF.ui_gpio_sen = P_GPIO(13);//L_GPIO(22);
		kdrv_disp_dev.SEL.KDRV_VDDO_REG_IF.ui_gpio_clk = P_GPIO(14);//L_GPIO(23);
		kdrv_disp_dev.SEL.KDRV_VDDO_REG_IF.ui_gpio_data = P_GPIO(15);//L_GPIO(24);
		#else
		kdrv_disp_dev.SEL.KDRV_VDDO_REG_IF.lcd_ctrl = drv_cfg->if_cfg.lcd_ctrl;
		kdrv_disp_dev.SEL.KDRV_VDDO_REG_IF.ui_sif_ch = drv_cfg->if_cfg.ui_sif_ch;
		kdrv_disp_dev.SEL.KDRV_VDDO_REG_IF.ui_gpio_sen = drv_cfg->if_cfg.ui_gpio_sen;
		kdrv_disp_dev.SEL.KDRV_VDDO_REG_IF.ui_gpio_clk = drv_cfg->if_cfg.ui_gpio_clk;
		kdrv_disp_dev.SEL.KDRV_VDDO_REG_IF.ui_gpio_data = drv_cfg->if_cfg.ui_gpio_data;
		#endif

		ret=kdrv_vddo_set(handler, VDDO_DISPDEV_REG_IF, &kdrv_disp_dev);
		if(ret!=ISF_OK)
				return ret;
        #endif
        if((drv_cfg->mode.output_mode.lcd & USER_LCD_OFFSET) == USER_LCD_OFFSET){
            user_data = drv_cfg->mode.output_mode.lcd & ~USER_LCD_OFFSET;
            user_data_enable = 1;
        }
	}
	if(drv_cfg->mode.output_type ==VDOOUT_HDMI){
		kdrv_disp_dev.SEL.KDRV_VDDO_HDMIMODE.video_id = drv_cfg->mode.output_mode.hdmi;
		kdrv_disp_dev.SEL.KDRV_VDDO_HDMIMODE.audio_id = VDDO_HDMI_AUDIO_NO_CHANGE;
		ret=kdrv_vddo_set(handler, VDDO_DISPDEV_HDMIMODE, &kdrv_disp_dev);
		if(ret!=ISF_OK)
		    return ISF_ERR_DRV;
	}

    if((drv_cfg->mode.output_type ==VDOOUT_TV_NTSC)||(drv_cfg->mode.output_type ==VDOOUT_TV_PAL)){
		kdrv_disp_dev.SEL.KDRV_VDDO_TVFULL.en_full = TRUE;
        ret = kdrv_vddo_set(handler, VDDO_DISPDEV_TVFULL, &kdrv_disp_dev);
		if(ret!=ISF_OK)
		    return ISF_ERR_DRV;
    }
	kdrv_disp_dev.SEL.KDRV_VDDO_OPEN_DEVICE.dev_id = drv_cfg->mode.output_type;
    kdrv_disp_dev.SEL.KDRV_VDDO_OPEN_DEVICE.user_data = user_data;
    kdrv_disp_dev.SEL.KDRV_VDDO_OPEN_DEVICE.user_data_en = user_data_enable;
	ret = kdrv_vddo_set(handler, VDDO_DISPDEV_OPEN_DEVICE, &kdrv_disp_dev);
	if((ret!=ISF_OK)&&(ret!=RE_INIT))
		return ISF_ERR_DRV;
	kdrv_disp_ctrl.SEL.KDRV_VDDO_ENABLE.en = TRUE;
	ret = kdrv_vddo_set(handler, VDDO_DISPCTRL_ENABLE, &kdrv_disp_ctrl);
	if(ret!=ISF_OK)
		return ISF_ERR_DRV;
	ret = kdrv_vddo_trigger(handler, NULL);
	if(ret!=ISF_OK)
		return ISF_ERR_DRV;

	//ret = kdrv_vddo_set(handler, VDDO_DISPCTRL_WAIT_FRM_END,0);
	return ret;
}

INT32 isf_vdoout_close_dev(UINT32 id)
{
	UINT32 handler = KDRV_DEV_ID(0, isf_vo_engine[id], 0);
	KDRV_VDDO_DISPCTRL_PARAM kdrv_disp_ctrl = {0};
	INT ret = ISF_OK;

	kdrv_disp_ctrl.SEL.KDRV_VDDO_ALL_LYR_EN.disp_lyr = VDDO_DISPLAYER_VDO1 | VDDO_DISPLAYER_VDO2 | VDDO_DISPLAYER_OSD1;
	kdrv_disp_ctrl.SEL.KDRV_VDDO_ENABLE.en = FALSE;
	ret = kdrv_vddo_set(handler, VDDO_DISPCTRL_ALL_LYR_EN, &kdrv_disp_ctrl);
	if(ret!=ISF_OK)
		return ISF_ERR_DRV;
	ret = kdrv_vddo_trigger(handler, NULL);
	if(ret!=ISF_OK)
		return ISF_ERR_DRV;
	return kdrv_vddo_set(handler, VDDO_DISPDEV_CLOSE_DEVICE, &kdrv_disp_ctrl);
}

#if 0

/*
1=> ?a¡¦cinput port support (ISF_CONNECT_PUSH|ISF_CONNECT_SYNC) connnct-type
?a¡¦c sync ao input port, £go sync ¡¦N?O?u notify £g1buffer ao2¢G¢DIaI: call ImageUnit_Notify(pData->pSrc, pData); |1RE|PREAkAU¢DI¡±1ao buffer pData
?a¡¦c sync ao input port, ¡Pi?e?J buffer ¡LA?] process¢DI¡±1 buffer ?a, ?¢G¡Âaa?¡Ó£g release, ¢D2?¡PAH£gU sync ¢XT¡M1|^|? buffer
¢DB¢D2?¡P-n?uAo£go¢DX?Y¡LD bufferaosync ¢XT¡M1 (|y|paG?Y|¡M process?W1L?g¡¦ARE?!, ?h-n£g¢D¡Li?U-O?g¡¦A?~¢Di£go¢DX)
¢DNcostart?a2A?@|¡M£go syncc|?¢Ga??eoY notify port (buffer ao2¢G¢DIaI), ¢D2?¡P¢Dy¢DHcall ImageUnit_QueryUpstreamOutputPort() ?d¢DX, £gM?a¡Óa null buffer
1.1=> ?o3!¢DH-i¡LO?o?Oao ide frame end ¡LO¡Pi sync¢XT¡M1, |paG swap buffer ?U¢Dh, ¡LA?¢G¢Ds¡Le release pData, £g¢D¡Li?Uide wait frame end ?~£go sync¢XT¡M1, ¡LA|PRE|^|? pData
*/




// IME output alignment : 422 -> W:2 H:1, 422 -> W:2 H:2
static ISIZE _ISF_VdoOut_CalcImgSizebyRatio(ISIZE *devSize, USIZE *devRatio, USIZE *Imgratio, UINT32 align)
{
	ISIZE  CastSize = *devSize;
	ISIZE  ImgSize;

	if ((!devRatio->w) || (!devRatio->h)) {
		DBG_ERR("devRatio w=%d, h=%d\r\n", devRatio->w, devRatio->h);
	} else if ((!Imgratio->w) || (!Imgratio->h)) {
		DBG_ERR("Imgratio w=%d, h=%d\r\n", Imgratio->w, Imgratio->h);
	} else {
		if (((float)Imgratio->w / Imgratio->h) >= ((float)devRatio->w / devRatio->h)) {
			CastSize.w = devSize->w;
			CastSize.h = ALIGN_ROUND(devSize->h * Imgratio->h / Imgratio->w * devRatio->w / devRatio->h, align);
		} else {
			CastSize.h = devSize->h;
			CastSize.w = ALIGN_ROUND(devSize->w * Imgratio->w / Imgratio->h * devRatio->h / devRatio->w, align);
		}
	}
#if (_FPGA_EMULATION_ == ENABLE)
	//buffer size = 1/2 * visual cast size (to reduce DMA bandwidth)
	ImgSize.w = (CastSize.w >> 1);
	ImgSize.h = (CastSize.h >> 1);
	DBG_WRN("to reduce DMA b/w on FPGA, only take 1/2 size => ImgSize w=%d, h=%d\r\n", ImgSize.w, ImgSize.h);
#else
	//buffer size = 1 * visual cast size
	ImgSize = CastSize;
#endif
	DBG_IND("ImgSize w=%d, h=%d\r\n", ImgSize.w, ImgSize.h);
	return ImgSize;
}

#endif
static int _isf_vdoout_alloc_tmp_buf(NVTMPP_MODULE module,UINT32 *buf_addr, UINT32 need_buf_size)
{
	static NVTMPP_VB_POOL vb_temp_pool = NVTMPP_VB_INVALID_POOL;
	NVTMPP_VB_BLK  blk;
	UINT32         blk_size;

    if(nvtmpp_vb_check_pool_valid(vb_temp_pool) == E_OK) {
        nvtmpp_vb_destroy_pool_2(vb_temp_pool, NVTMPP_TEMP_POOL_NAME);
    }
	blk_size = need_buf_size;
	vb_temp_pool = nvtmpp_vb_create_pool(NVTMPP_TEMP_POOL_NAME, blk_size, 1, NVTMPP_DDR_1);
	if (NVTMPP_VB_INVALID_POOL == vb_temp_pool) {
		DBG_ERR("create pool err\r\n");
		return -1;
	}
	blk = nvtmpp_vb_get_block(module, vb_temp_pool, blk_size, NVTMPP_DDR_1);
	if (NVTMPP_VB_INVALID_BLK == blk) {
		DBG_ERR("get vb block err\r\n");
		if (nvtmpp_vb_destroy_pool(vb_temp_pool) != NVTMPP_ER_OK)
			DBG_ERR("free pool\r\n");
		return -1;
	}
	*buf_addr = nvtmpp_vb_block2addr(blk);
	return 0;
}
//buf(w,h): buf is assign-size,
//buf(0,0): buf is full-device-size,
//img_ratio(w,h): img is assign-ratio
//img_ratio(0,0): img is full-buf-size
static void _isf_vdoout_set_image_size(UINT32 id, UINT32 buf_w, UINT32 buf_h, UINT32 img_ratio_w, UINT32 img_ratio_h, UINT32 dir)
{
	VDOOUT_CONTEXT* p_ctx = (VDOOUT_CONTEXT*)(isf_volist[id]->refdata);

	UINT32 CurrDevObj = p_ctx->info.uiDevObj;
	if ((buf_w == 0) && (buf_h == 0)) { //need refer to device
		if (CurrDevObj == 0) { //check device object
			return;
		}
		if (!p_ctx->info.bEn) { //check device enable or not
			return;
		}
	}

	//set disp-max-imgsize = disp-dev-size
	if ((buf_w == 0) && (buf_h == 0)) {
		#if 1
		DBG_ERR("no w h\r\n");
		#else
		//set disp-dev-size
		if (DevID == 0) {
			p_ctx->info.devSize = GxVideo_GetDeviceSize(DOUT1);
		}
		if (DevID == 1) {
			p_ctx->info.devSize = GxVideo_GetDeviceSize(DOUT2);
		}
		if( ((dir & ISF_VDO_DIR_ROTATE_MASK) == ISF_VDO_DIR_ROTATE_90)
		 || ((dir & ISF_VDO_DIR_ROTATE_MASK) == ISF_VDO_DIR_ROTATE_270)) { //display is rotate 90 or 270
		    UINT32 tmp = p_ctx->info.devSize.w;
		    p_ctx->info.devSize.w = p_ctx->info.devSize.h;
		    p_ctx->info.devSize.h = tmp;
		}
		DBG_IND("Device w=%d,h=%d\r\n", p_ctx->info.devSize.w, p_ctx->info.devSize.h);
		p_ctx->info.bufSize = p_ctx->info.devSize;
		#endif
	} else {
		p_ctx->info.bufSize.w = buf_w;
		p_ctx->info.bufSize.h = buf_h;
	}
	DBG_IND("Buffer w=%d,h=%d\r\n", p_ctx->info.bufSize.w, p_ctx->info.bufSize.h);

	if (!p_ctx->info.bEn) {
		p_ctx->info.devAspect.w = 0;
		p_ctx->info.devAspect.h = 0;
	} else {
		#if 0
		if (DevID == 0) {
			p_ctx->info.devAspect = GxVideo_GetDeviceAspect(DOUT1);
		}
		if (DevID == 1) {
			p_ctx->info.devAspect = GxVideo_GetDeviceAspect(DOUT2);
		}
		if( ((dir & ISF_VDO_DIR_ROTATE_MASK) == ISF_VDO_DIR_ROTATE_90)
		 || ((dir & ISF_VDO_DIR_ROTATE_MASK) == ISF_VDO_DIR_ROTATE_270)) { //display is rotate 90 or 270
		    UINT32 tmp = p_ctx->info.devAspect.w;
		    p_ctx->info.devAspect.w = p_ctx->info.devAspect.h;
		    p_ctx->info.devAspect.h = tmp;
		}
		#endif
	}
	DBG_IND("DevAspect w=%d,h=%d\r\n", p_ctx->info.devAspect.w, p_ctx->info.devAspect.h);
	if ((img_ratio_w == 0) && (img_ratio_h == 0)) { //fix for CID 42953
		p_ctx->info.ImgRatio = p_ctx->info.devAspect;
	} else {
		p_ctx->info.ImgRatio.w = img_ratio_w;
		p_ctx->info.ImgRatio.h = img_ratio_h;
	}
	DBG_IND("ImgRatio w=%d,h=%d\r\n", p_ctx->info.ImgRatio.w, p_ctx->info.ImgRatio.h);

	//calc disp-imgsize by AUTO_MATCH_INSIDE disp-dev-size
	if(!((buf_w == 0) && (buf_h == 0))) {
		//user already assign size, do not calibrate disp-imgsize
		p_ctx->info.ImgSize = p_ctx->info.bufSize;
	} else if (!p_ctx->info.bEn) {
		//no device, cannot calibrate disp-imgsize
		p_ctx->info.ImgSize = p_ctx->info.bufSize;
	} else {
		#if 0
	    //do calibrate disp-imgsize
		if( ((dir & ISF_VDO_DIR_ROTATE_MASK) == ISF_VDO_DIR_ROTATE_90)
		 || ((dir & ISF_VDO_DIR_ROTATE_MASK) == ISF_VDO_DIR_ROTATE_270)) { //display is rotate 90 or 270
		p_ctx->info.ImgSize = _ISF_VdoOut_CalcImgSizebyRatio(&(p_ctx->info.bufSize), &(p_ctx->info.devAspect), &(p_ctx->info.ImgRatio), 4);
		} else {
		p_ctx->info.ImgSize = _ISF_VdoOut_CalcImgSizebyRatio(&(p_ctx->info.bufSize), &(p_ctx->info.devAspect), &(p_ctx->info.ImgRatio), 2);
		}
		#endif
	}
	DBG_IND("[%d] ImgSize w=%d,h=%d\r\n", id, p_ctx->info.ImgSize.w, p_ctx->info.ImgSize.h);
}

//win(x,y,w,h): win is assign-range
//win(0,0,0,0): win is full-device-range
static void _isf_vdoout_set_window(UINT32 id, UINT32 x, UINT32 y, UINT32 w, UINT32 h)
{
	VDOOUT_CONTEXT* p_ctx = (VDOOUT_CONTEXT*)(isf_volist[id]->refdata);
	UINT32 CurrDevObj = p_ctx->info.uiDevObj;
	if ((w == 0) && (h == 0)) {
		//set disp-window = disp-dev-size
		USIZE  CastSize;
		if (CurrDevObj == 0) { //check device object
			return;
		}

		//visual cast size = 1 * buffer size
		CastSize.w = p_ctx->info.ImgSize.w;
		CastSize.h = p_ctx->info.ImgSize.h;

		p_ctx->info.winRect.w = CastSize.w;
		p_ctx->info.winRect.h = CastSize.h;
		if (p_ctx->info.winRect.w == p_ctx->info.devSize.w) {
			p_ctx->info.winRect.x = 0;
			p_ctx->info.winRect.y = (p_ctx->info.devSize.h - CastSize.h) >> 1;
		}
		if (p_ctx->info.winRect.h == p_ctx->info.devSize.h) {
			p_ctx->info.winRect.x = (p_ctx->info.devSize.w - CastSize.w) >> 1;
			p_ctx->info.winRect.y = 0;
		}
#if 0
		p_ctx->info.winRect.x = 0;
		p_ctx->info.winRect.y = 0;
		p_ctx->info.winRect.w = p_ctx->info.devSize.w;
		p_ctx->info.winRect.h = p_ctx->info.devSize.h;
#endif
	} else {
		p_ctx->info.winRect.x = x;
		p_ctx->info.winRect.y = y;
		p_ctx->info.winRect.w = w;
		p_ctx->info.winRect.h = h;
	}
	DBG_IND("[%d] Window x=%d,y=%d,w=%d,h=%d\r\n", id, p_ctx->info.winRect.x, p_ctx->info.winRect.y, p_ctx->info.winRect.w, p_ctx->info.winRect.h);
}

void _isf_vdoout_release_head(UINT32 id, VDO_FRAME *img_buf)
{
    ISF_UNIT  *p_thisunit=isf_volist[id];
	VDOQUEUE *pQueue = &(gQueue[id]);
	ISF_DATA ISF_DispRelData = {0};
	ISF_DATA *p_data = NULL;
	//pop data from queue
    /* flags should be local variable */
    unsigned long      flags;
    UINT32 ret =0;
    UINT32 addr;

	loc_cpu(flags);

	if (pQueue->Cnt > 0) {

		if (pQueue->Head == 0) {
			pQueue->Head = pQueue->Total;
		}
        pQueue->Head--;
        p_data = &ISF_DispRelData;
		memcpy(p_data ,&pQueue->Data[pQueue->Head],sizeof(ISF_DATA)); //copy from queue

        #if VO_FRAME_DBG
        if(id==0)
        {
            if(pQueue->Head==0) {
                arb_disable_wp(DDR_ARB_1, WPSET_0);
            } else {
                arb_disable_wp(DDR_ARB_1, WPSET_1);
            }
        }
        #endif
    	pQueue->Cnt--;
        if(img_buf!= NULL)
        	addr = img_buf->addr[0];

        unl_cpu(flags);

        if(img_buf!= NULL) {
           	p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_REL, ISF_ENTER);
            ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM,p_thisunit,ISF_IN(0),"release_head %d addr %x",pQueue->Tail,((VDO_FRAME  *)p_data->desc)->addr[0]);
            if(addr!=((VDO_FRAME  *)p_data->desc)->addr[0]) {
                DBG_ERR("release_headpQueue->Total %x %x diff %x\r\n",addr,((VDO_FRAME  *)p_data->desc)->addr[0],p_data->h_data);
                DBG_ERR("cnt:%d tail:%d \r\n",pQueue->Cnt,pQueue->Tail);
			    DBG_DUMP("q0 %x\r\n",((VDO_FRAME *)pQueue->Data[0].desc)->addr[0]);
                DBG_DUMP("q1 %x\r\n",((VDO_FRAME *)pQueue->Data[1].desc)->addr[0]);
                nvtmpp_dump_status(vk_printk);
                vdodisp_dump_info(vk_printk);
            }

            p_thisunit->p_base->do_release(p_thisunit, ISF_IN(0), p_data, ISF_IN_PROBE_REL);
            if(ret !=0){
                DBG_ERR("cb:%x Q:%x h_data %x\r\n",addr,((VDO_FRAME  *)p_data->desc)->addr[0],p_data->h_data);
                DBG_ERR("cnt:%d tail:%d \r\n",pQueue->Cnt,pQueue->Tail);
			    DBG_DUMP("q0 %x\r\n",((VDO_FRAME *)pQueue->Data[0].desc)->addr[0]);
                DBG_DUMP("q1 %x\r\n",((VDO_FRAME *)pQueue->Data[1].desc)->addr[0]);
                nvtmpp_dump_status(vk_printk);
                vdodisp_dump_info(vk_printk);
            }
            p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_REL, ISF_OK);
        }

    } else {
		DBG_ERR("no head %d\r\n",pQueue->Cnt);
        unl_cpu(flags);
	}

}

void _isf_vdoout_finish_release(UINT32 id, VDO_FRAME *img_buf)
{
    ISF_UNIT  *p_thisunit=isf_volist[id];
	VDOOUT_CONTEXT* p_ctx = (VDOOUT_CONTEXT*)(isf_volist[id]->refdata);
    INT32 ret = ISF_OK;
    UINT32 addr;

	if(!(p_ctx->usr_func & VDOOUT_FUNC_NOWAIT)) { // WAIT case

		VDOQUEUE *pQueue = &(gQueue[id]);
		//ISF_PORT* pSrcPort = ImageUnit_In(isf_volist[id], ISF_IN1);
		ISF_DATA *p_data = NULL;
		ISF_DATA *pTempData = &(gISF_DispTempData[id]);
		VDO_FRAME * pTempImg = (VDO_FRAME  *)(pTempData->desc);
       /* flags should be local variable */
        unsigned long      flags;

		if(img_buf == pTempImg) {
			//release tmp buffer,do nothing

		} else if(img_buf != NULL) {
		    loc_cpu(flags);
			if (pQueue->Tail == QUEUE_EMPTY) {
                unl_cpu(flags);
				//queue is empty, not allow pop
				DBG_ERR("[%d] Empty! (Cnt=%d)\r\n", id, pQueue->Cnt);
			} else {
    			ISF_DATA ISF_DispRelData = {0};
				//pop data from queue

                p_data = &ISF_DispRelData;
				memcpy(p_data ,&pQueue->Data[pQueue->Tail],sizeof(ISF_DATA)); //copy from queue

                #if VO_FRAME_DBG
                if(id==0)
                {
                    if(pQueue->Tail==0) {
                        #if 0
                        VDO_FRAME *pImg = (VDO_FRAME *)p_data->desc;

                        DBG_ERR("RE0 %x %x\r\n",nvtmpp_sys_va2pa(pImg->addr[0]),pImg->size);
                        #endif
                        arb_disable_wp(DDR_ARB_1, WPSET_0);
                    } else {
                        arb_disable_wp(DDR_ARB_1, WPSET_1);
                    }
                }
                #endif

				if (pQueue->Cnt > 1) {
					pQueue->Tail++;
					if (pQueue->Tail >= pQueue->Total) {
						pQueue->Tail = 0;
					}
				} else {
					pQueue->Tail = QUEUE_EMPTY;
				}
				pQueue->Cnt--;
		addr = img_buf->addr[0];

                unl_cpu(flags);

               	p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_REL, ISF_ENTER);
                ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM,p_thisunit,ISF_IN(0),"rel Tail %d addr %x",pQueue->Tail,((VDO_FRAME  *)p_data->desc)->addr[0]);
                if(addr!=((VDO_FRAME  *)p_data->desc)->addr[0]) {
                    DBG_ERR("%x %x diff %x\r\n",addr,((VDO_FRAME  *)p_data->desc)->addr[0],p_data->h_data);
                    DBG_ERR("cnt:%d tail:%d \r\n",pQueue->Cnt,pQueue->Tail);
				    DBG_DUMP("q0 %x\r\n",((VDO_FRAME *)pQueue->Data[0].desc)->addr[0]);
                    DBG_DUMP("q1 %x\r\n",((VDO_FRAME *)pQueue->Data[1].desc)->addr[0]);
                    nvtmpp_dump_status(vk_printk);
                    vdodisp_dump_info(vk_printk);
                }

    		    ret=p_thisunit->p_base->do_release(p_thisunit, ISF_IN(0), p_data, ISF_IN_PROBE_REL);
                if(ret !=0){
                    DBG_ERR("cb:%x Q:%x h_data %x\r\n",addr,((VDO_FRAME  *)p_data->desc)->addr[0],p_data->h_data);
                    DBG_ERR("cnt:%d tail:%d \r\n",pQueue->Cnt,pQueue->Tail);
				    DBG_DUMP("q0 %x\r\n",((VDO_FRAME *)pQueue->Data[0].desc)->addr[0]);
                    DBG_DUMP("q1 %x\r\n",((VDO_FRAME *)pQueue->Data[1].desc)->addr[0]);
                    nvtmpp_dump_status(vk_printk);
                    vdodisp_dump_info(vk_printk);
                }
    		    p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_REL, ISF_OK);
			}
		}
	}
}

static ISF_RV _isf_vdoout_do_reset(ISF_UNIT *p_thisunit, UINT32 oport)
{
	//reset ctrl state and ctrl parameter
	if(oport == ISF_OUT_BASE) {
		VDOOUT_CONTEXT* p_ctx = (VDOOUT_CONTEXT*)(p_thisunit->refdata);
        UINT32 dev = p_ctx->dev;

        if(dev>=DEV_ID_MAX)
            return ISF_ERR_PARAM_EXCEED_LIMIT;

        _isf_vout_memset_ctx(dev);
        p_ctx->dev = dev ;
	}
	//reset in parameter
	if(oport == ISF_OUT_BASE) {
		ISF_INFO *p_src_info = p_thisunit->port_ininfo[oport];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
		memset(p_vdoinfo, 0, sizeof(ISF_VDO_INFO));
	}
	//reset out parameter
	{
		ISF_INFO *p_dest_info = p_thisunit->port_outinfo[0];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
		memset(p_vdoinfo, 0, sizeof(ISF_VDO_INFO));
	}
	//reset out state
	{
		ISF_STATE *p_state = p_thisunit->port_outstate[0];
		memset(p_state, 0, sizeof(ISF_STATE));
	}
	return ISF_OK;
}
static int merge_window_open_cnt = 0;
static ISF_RV _isf_vdoout_open(UINT32 id, ISF_UNIT  *p_thisunit, UINT32 oport)
{
	VDOOUT_CONTEXT* p_ctx = (VDOOUT_CONTEXT*)(p_thisunit->refdata);
	ISF_RV 			r = ISF_OK;
	KDRV_VDDO_DISPDEV_PARAM kdrv_disp_dev = {0};
	UINT32 handler = KDRV_DEV_ID(0, isf_vo_engine[id], 0);

	DBG_IND("id %x open_tag %x",id,p_ctx->open_tag);

	if (p_ctx->open_tag == 0) {

		r = nvtmpp_vb_add_module(p_thisunit->unit_module);
        if(r!=0){
			DBG_ERR("add_module er %d\r\n",r);
			r = ISF_ERR_FAILED;
            return r;
		}

        {
            //open device
    		r = isf_vdoout_open_dev(id,&p_ctx->dev_cfg);
            if(r!=ISF_OK) {
    			DBG_ERR("open_dev r %d\r\n",r);
                r = ISF_ERR_DRV;
                return r;
            }
            //update default device size and window first open
        	r = kdrv_vddo_open(0, isf_vo_engine[id]);
            if(r!=ISF_OK) {
    			DBG_ERR("kdrv r %d\r\n",r);
                r = ISF_ERR_DRV;
                return r;
            }
            kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.dev_id = p_ctx->dev_cfg.mode.output_type;
            r = kdrv_vddo_get(handler, VDDO_DISPDEV_DISPSIZE, &kdrv_disp_dev);
             if(r!=ISF_OK) {
        		DBG_ERR("get r %d\r\n",r);
                r = ISF_ERR_DRV;
                return r;
            }
        	p_ctx->info.winRect.x = 0;
        	p_ctx->info.winRect.y = 0;
        	p_ctx->info.winRect.w = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_width;
        	p_ctx->info.winRect.h = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_height;
        	p_ctx->info.devSize.w = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_width;
        	p_ctx->info.devSize.h = kdrv_disp_dev.SEL.KDRV_VDDO_DISPSIZE.buf_height;

			p_ctx->open_tag = ISF_VDOOUT_FALG_OPEN;
            p_ctx->dev = id;

		}

		if(r == ISF_OK) {
            VDODISP_ER er;
			VDODISP_INIT Init = {0};
			Init.ui_api_ver = VDODISP_API_VERSION;
			er = vdodisp_install_id(id);
            if(er!=VDODISP_ER_OK) {
    			DBG_ERR("intall er %d\r\n",er);
    			r = ISF_ERR_NOT_AVAIL;
                return r;
    		}

			er = vdodisp_init(id,&Init);
			if(er!=VDODISP_ER_OK) {
				DBG_ERR("init er %d\r\n",er);
				r = ISF_ERR_FAILED;
                return r;
			}

    		er = vdodisp_open(id);
    		if (er != VDODISP_ER_OK) {
    			DBG_ERR("open er %d\r\n",er);
    			r = ISF_ERR_FAILED;
                return r;
    		}
            //suspend vdodisp,resume when READYTIME_SYNC
            er = vdodisp_suspend(id);
            if (er != VDODISP_ER_OK) {
    			DBG_ERR("suspend er %d\r\n",er);
    			r = ISF_ERR_FAILED;
                return r;
    		}
		}
	} else {
	    if(p_ctx->flow_ctrl & VDOOUT_FUNC_MERGE_WIN) {
            DBG_IND("merge_wnd open\r\n");
	    } else {
		    DBG_WRN("already opened\r\n");
	    }
		if(merge_window_open_cnt == 0){
			extern int create_merge_window_thread(VDOOUT_CONTEXT* p_ctx);
			merge_window_open_cnt = 1;
			r = create_merge_window_thread(p_ctx);
			if(r)
				DBG_ERR("fail to create thread to merge window\n");
		}else
			merge_window_open_cnt++;
	}

	return r;
}


static ISF_RV _isf_vdoout_close(UINT32 id, ISF_UNIT  *p_thisunit, UINT32 oport)
{
	VDOOUT_CONTEXT* p_ctx = (VDOOUT_CONTEXT*)(p_thisunit->refdata);
	INT32 				r = ISF_OK;

	DBG_IND("id %x open_tag %x \r\n",id,p_ctx->open_tag);
	if(p_ctx->flow_ctrl & VDOOUT_FUNC_MERGE_WIN) {
        DBG_IND("merge_wnd close \r\n");
        return ISF_OK;;
	}
	if(p_ctx->open_tag == ISF_VDOOUT_FALG_OPEN)
	{
		r = vdodisp_close(id);
		if(r!=VDODISP_ER_OK){
			DBG_ERR("close er %d\r\n",r);
            return r;
		}
        r = vdodisp_uninstall_id(id);
		if(r!=VDODISP_ER_OK){
			DBG_ERR("uninstall er %d\r\n",r);
            return r;
		}
        if(p_ctx->vnd_func & VDOOUT_FUNC_SKIP_CLOSE) {
            p_ctx->vnd_func = p_ctx->vnd_func&~VDOOUT_FUNC_SKIP_CLOSE;
        } else {

    		r = isf_vdoout_close_dev(id);
    		if(r!=0){
    			DBG_ERR("close dev er %d\r\n",r);
    			return r;
    		}
        }

	} else {
		if(merge_window_open_cnt == 1){
			extern int destroy_merge_window_thread(void);
			merge_window_open_cnt = 0;
			r = destroy_merge_window_thread();
			if(r)
				DBG_ERR("fail to destroy thread to merge window\n");
		}else
			merge_window_open_cnt--;
	}

	p_ctx->open_tag = 0;

	return ISF_OK;
}
ISF_RV isf_vdoout_do_setportparam(ISF_UNIT  *p_thisunit, UINT32 nport, UINT32 param, UINT32 value)
{
	VDOOUT_CONTEXT* p_ctx = (VDOOUT_CONTEXT*)(p_thisunit->refdata);
    UINT32 id = p_ctx->dev;

    ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL,p_thisunit,nport,"param = 0x%x, value=%d",param,value);
    if (nport == ISF_CTRL) {
		switch (param) {
		case VDOOUT_PARAM_SLEEP:
            if(value==1) {//enter sleep
                return isf_vdoout_close_dev(id);
            } else {
    		    return isf_vdoout_open_dev(id,&p_ctx->dev_cfg);
            }
		break;
#if defined(_BSP_NA51000_)
        case VDOOUT_PARAM_HDMI_CFG:
        {
           	UINT32 handler = KDRV_DEV_ID(0, isf_vo_engine[id], 0);
        	ISF_RV r = ISF_OK;

        	r=kdrv_vddo_open(0, isf_vo_engine[id]);
            if(r!=ISF_OK) {
                return ISF_ERR_DRV;
            }
            if(((int)value>=0)&&(value<=(VDDO_DISPDEV_HDMI_FORCE_CLR-VDDO_DISPDEV_HDMI_FORCE_RGB))) {
                return kdrv_vddo_set(handler, VDDO_DISPDEV_HDMI_FORCE_RGB+value,NULL);
            } else {
                return ISF_ERR_NOT_AVAIL;
            }
        }
        break;
#endif
		case VDOOUT_PARAM_CLEAR_WIN:
            DBG_IND("clear window\r\n");
		break;
		case VDOOUT_PARAM_VND_MERGE_WND:
            DBG_IND("set merge flow value=%d \r\n",value);
            if(value==1) {
                p_ctx->flow_ctrl |= VDOOUT_FUNC_MERGE_WIN;
                p_thisunit->list_id = 2; //for isf_flow_unit_state check state
            } else {
    		    p_ctx->flow_ctrl = p_ctx->flow_ctrl & (~VDOOUT_FUNC_MERGE_WIN);
                p_thisunit->list_id = 0;
            }

		break;
#if defined(_BSP_NA51089_)
		case VDOOUT_PARAM_VND_ABORT:
        {
        	UINT32 handler = KDRV_DEV_ID(0, isf_vo_engine[id], 0);
        	ISF_RV r = ISF_OK;
            KDRV_VDDO_DISPCTRL_PARAM p_kdrv_disp_ctrl = {0};
        	r=kdrv_vddo_open(0, isf_vo_engine[id]);
            if(r!=ISF_OK) {
                return ISF_ERR_DRV;
            }
            p_kdrv_disp_ctrl.SEL.KDRV_VDDO_DMAABORT.en =value;
            r=kdrv_vddo_set(handler, VDDO_DISPCTRL_DMA_ABORT,&p_kdrv_disp_ctrl);
            if(r!=ISF_OK) {
                return ISF_ERR_INVALID_PARAM;
            }

		}
		break;
#endif
	    }
    }
	return ISF_OK;
}
UINT32 isf_vdoout_do_getportparam(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param)
{
    ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL,p_thisunit,nport,"param = 0x%x",param);

	return ISF_OK;
}

ISF_RV isf_vdoout_do_setportstruct(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size)
{
	VDOOUT_CONTEXT* p_ctx = (VDOOUT_CONTEXT*)(p_thisunit->refdata);
    UINT32 id = p_ctx->dev;
	ISF_RV ret = ISF_OK;

    ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL,p_thisunit,nport,"id = %d, param = 0x%x, size=%d",id, param, size);

	if(!p_struct)
		return ISF_ERR_INVALID_PARAM;

	if (nport == ISF_CTRL) {
		switch (param) {
		case VDOOUT_PARAM_MODE:
        {
			memcpy((void *)&p_ctx->dev_cfg.mode,(void *)p_struct,sizeof(VDOOUT_MODE));
		}
		break;
		case VDOOUT_PARAM_VND_IF_CFG:
        {
			memcpy((void *)&p_ctx->dev_cfg.if_cfg,(void *)p_struct,sizeof(VDOOUT_IF_CFG));

            ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL,p_thisunit,ISF_IN(0),"####output_type=%d \r\nlcd_ctrl=%d \r\nui_sif_ch=%d \r\nui_gpio_sen=0x%x \r\ngpio_clk=0x%x \r\nui_gpio_data=0x%x\r\n", \
            p_ctx->dev_cfg.mode.output_type, \
            p_ctx->dev_cfg.if_cfg.lcd_ctrl,p_ctx->dev_cfg.if_cfg.ui_sif_ch,  \
            p_ctx->dev_cfg.if_cfg.ui_gpio_sen,p_ctx->dev_cfg.if_cfg.ui_gpio_clk,p_ctx->dev_cfg.if_cfg.ui_gpio_data);
		}
		break;
		case VDOOUT_PARAM_LYR_ENABLE:
		{
			UINT32 handler = KDRV_DEV_ID(0, isf_vo_engine[id], 0);
			KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer = {0};
			VDOOUT_LYR_ENABLE* lyr=(VDOOUT_LYR_ENABLE *)p_struct;

            ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL,p_thisunit,nport,"layer=%d,enable=%d",lyr->layer_id,lyr->enable );

			kdrv_disp_layer.SEL.KDRV_VDDO_ENABLE.layer = lyr->layer_id;
			kdrv_disp_layer.SEL.KDRV_VDDO_ENABLE.en = lyr->enable;
			ret=kdrv_vddo_set(handler, VDDO_DISPLAYER_ENABLE, &kdrv_disp_layer);
			if(ret!=ISF_OK)
				return ISF_ERR_DRV;
			ret = kdrv_vddo_trigger(handler, NULL);
			if(ret!=ISF_OK)
				return ISF_ERR_DRV;
		}
		break;
		case VDOOUT_PARAM_LYR_FMT:
		{
			UINT32 handler = KDRV_DEV_ID(0, isf_vo_engine[id], 0);
			KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer = {0};
			VDOOUT_LYR_FMT* lyr=(VDOOUT_LYR_FMT*)p_struct;

            ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL,p_thisunit,nport,"layer=%d,fmt=%d",lyr->layer_id,lyr->fmt);

			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.format = lyr->fmt;
			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_width = p_ctx->info.devSize.w;
			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_height = p_ctx->info.devSize.h;
			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_line_ofs = p_ctx->info.devSize.w*get_fmt_coef(lyr->fmt);
			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_width = p_ctx->info.winRect.w;
			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_height = p_ctx->info.winRect.h;
			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_x = p_ctx->info.winRect.x;
			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_y = p_ctx->info.winRect.y;
			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.layer = lyr->layer_id;
			ret=kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFWINSIZE, &kdrv_disp_layer);
			if(ret!=ISF_OK)
				return ISF_ERR_DRV;
			ret = kdrv_vddo_trigger(handler, NULL);
			if(ret!=ISF_OK)
				return ISF_ERR_DRV;
		}
		break;
		case VDOOUT_PARAM_LYR_BLEND:
		{
			UINT32 handler = KDRV_DEV_ID(0, isf_vo_engine[id], 0);
			KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer = {0};
			VDOOUT_LYR_BLEND* lyr=(VDOOUT_LYR_BLEND*)p_struct;

            ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL,p_thisunit,nport,"layer=%d,blend=%d,alpha=%d",lyr->layer_id,lyr->type,lyr->global_alpha);

			kdrv_disp_layer.SEL.KDRV_VDDO_BLEND.type = lyr->type;
			kdrv_disp_layer.SEL.KDRV_VDDO_BLEND.global_alpha = lyr->global_alpha;
			kdrv_disp_layer.SEL.KDRV_VDDO_BLEND.layer = lyr->layer_id;
			ret=kdrv_vddo_set(handler, VDDO_DISPLAYER_BLEND, &kdrv_disp_layer);
			if(ret!=ISF_OK)
				return ret;
			ret = kdrv_vddo_trigger(handler, NULL);
			if(ret!=ISF_OK)
				return ISF_ERR_DRV;

		}
		break;
		case VDOOUT_PARAM_LYR_COLORKEY:
		{
			UINT32 handler = KDRV_DEV_ID(0, isf_vo_engine[id], 0);
#if defined(_BSP_NA51000_)
			KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer1 = {0};
#endif
			KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer2 = {0};
			VDOOUT_LYR_COLORKEY* lyr=(VDOOUT_LYR_COLORKEY*)p_struct;

            ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL,p_thisunit,nport,"layer=%d,colorkey_op=%d R:%x,G:%x,B:%x",lyr->layer_id,lyr->colorkey_op,lyr->key_r,lyr->key_g,lyr->key_b);

#if !defined(_BSP_NA51000_)
			kdrv_disp_layer2.SEL.KDRV_VDDO_COLORKEY.colorkey_op = lyr->colorkey_op;
			kdrv_disp_layer2.SEL.KDRV_VDDO_COLORKEY.key_y = lyr->key_r;
			kdrv_disp_layer2.SEL.KDRV_VDDO_COLORKEY.key_cb = lyr->key_g;
			kdrv_disp_layer2.SEL.KDRV_VDDO_COLORKEY.key_cr = lyr->key_b;
			kdrv_disp_layer2.SEL.KDRV_VDDO_COLORKEY.layer = lyr->layer_id;

#else
            kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.r_to_y = lyr->key_r;
            kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.g_to_u = lyr->key_g;
            kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.b_to_v = lyr->key_b;
            kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.alpha = 0;
			ret=kdrv_vddo_get(handler, VDDO_DISPLAYER_CST_FROM_RGB_TO_YUV, &kdrv_disp_layer1);
            if(ret!=ISF_OK)
				return ret;

            ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL,p_thisunit,nport,"layer=%d,colorkey_op=%d Y:%x,U:%x,V:%x",lyr->layer_id,lyr->colorkey_op,kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.r_to_y,kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.g_to_u,kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.b_to_v);

			kdrv_disp_layer2.SEL.KDRV_VDDO_COLORKEY.colorkey_op = lyr->colorkey_op;
			kdrv_disp_layer2.SEL.KDRV_VDDO_COLORKEY.key_y = kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.r_to_y;
			kdrv_disp_layer2.SEL.KDRV_VDDO_COLORKEY.key_cb = kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.g_to_u;
			kdrv_disp_layer2.SEL.KDRV_VDDO_COLORKEY.key_cr = kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.b_to_v;
			kdrv_disp_layer2.SEL.KDRV_VDDO_COLORKEY.layer = lyr->layer_id;
#endif
			ret=kdrv_vddo_set(handler, VDDO_DISPLAYER_COLORKEY, &kdrv_disp_layer2);
			if(ret!=ISF_OK)
				return ISF_ERR_DRV;
			ret = kdrv_vddo_trigger(handler, NULL);
			if(ret!=ISF_OK)
				return ISF_ERR_DRV;

		}
		break;
		case VDOOUT_PARAM_LYR_DIM:
		{
			UINT32 handler = KDRV_DEV_ID(0, isf_vo_engine[id], 0);
			KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer = {0};
			VDOOUT_LYR_DIM* lyr=(VDOOUT_LYR_DIM*)p_struct;

            ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL,p_thisunit,nport,"layer=%d,in w=%d,h=%d out(%d,%d,%d,%d)",lyr->layer_id,lyr->in_dim.w,lyr->in_dim.h,lyr->out_rect.x,lyr->out_rect.y,lyr->out_rect.w,lyr->out_rect.h);

    		if((lyr->in_dim.w>p_ctx->info.devSize.w*2)||(lyr->in_dim.h>p_ctx->info.devSize.h*2)) {
                DBG_ERR("dim(%d,%d)> device *2 (%d,%d)\r\n",(int)lyr->in_dim.w,(int)lyr->in_dim.h,(int)p_ctx->info.devSize.w,(int)p_ctx->info.devSize.h);
                return ISF_ERR_PARAM_EXCEED_LIMIT;
    		}

    		if((lyr->out_rect.x+lyr->out_rect.w>p_ctx->info.devSize.w)||(lyr->out_rect.y+lyr->out_rect.h>p_ctx->info.devSize.h)) {
                DBG_ERR("win(%d,%d,%d,%d)> device size (%d,%d)\r\n",(int)lyr->out_rect.x,(int)lyr->out_rect.y,(int)lyr->out_rect.w,(int)lyr->out_rect.h,(int)p_ctx->info.devSize.w,(int)p_ctx->info.devSize.h);
                return ISF_ERR_PARAM_EXCEED_LIMIT;
    		}

    		if((lyr->out_rect.w>KDRV_VDOOUT_MAX_WIDTH)||(lyr->out_rect.x>KDRV_VDOOUT_MAX_WIDTH) ||
               (lyr->out_rect.h>KDRV_VDOOUT_MAX_HIGH)||(lyr->out_rect.y>KDRV_VDOOUT_MAX_HIGH) ) {
                DBG_ERR("win(%d,%d,%d,%d)> max (%d,%d)\r\n",(int)lyr->out_rect.x,(int)lyr->out_rect.y,(int)lyr->out_rect.w,(int)lyr->out_rect.h,(int)p_ctx->info.devSize.w,(int)p_ctx->info.devSize.h);
                return ISF_ERR_PARAM_EXCEED_LIMIT;
    		}

			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.format = lyr->fmt;
			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_width = lyr->in_dim.w;
			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_height = lyr->in_dim.h;
    		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_line_ofs = lyr->in_dim.w*get_fmt_coef(lyr->fmt);
            if(kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_line_ofs & (KDRV_VDOOUT_O1_BUF_LOFF_ALIGN-1)){
                DBG_ERR("lineoff %d not %d align\r\n",(int)kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_line_ofs,(int)KDRV_VDOOUT_O1_BUF_LOFF_ALIGN);
                return ISF_ERR_PARAM_EXCEED_LIMIT;
            }
    		// set pixel format
    		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_width = lyr->out_rect.w;
    		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_height = lyr->out_rect.h;;
    		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_x = lyr->out_rect.x;
    		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_y = lyr->out_rect.y;
    		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.layer = lyr->layer_id;
			ret=kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFWINSIZE, &kdrv_disp_layer);
			if(ret!=ISF_OK)
				return ISF_ERR_DRV;
			ret = kdrv_vddo_trigger(handler, NULL);
			if(ret!=ISF_OK){
				return ISF_ERR_DRV;
            }
		}
		break;
		case VDOOUT_PARAM_PALETTE_TABLE:
		{
			UINT32 handler = KDRV_DEV_ID(0, isf_vo_engine[id], 0);
			KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer = {0};
			VDOOUT_PALETTE* lyr=(VDOOUT_PALETTE*)p_struct;

            ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL,p_thisunit,nport,"number=%d %x,%x,%x",lyr->number,lyr->p_pale_entry[0],lyr->p_pale_entry[1],lyr->p_pale_entry[2]);
#if defined(_BSP_NA51000_)
            {//RFB conver to YCbCr for HW
                UINT32 i=0;
		        KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer1 = {0};

                for(i=0;i<lyr->number;i++){
                    //DBG_DUMP("RGB p%i %x\r\n",i,lyr->p_pale_entry[i]);

                    kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.r_to_y = (lyr->p_pale_entry[i]>>16)&0xFF;
                    kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.g_to_u = (lyr->p_pale_entry[i]>>8)&0xFF;
                    kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.b_to_v = (lyr->p_pale_entry[i])&0xFF;
                    kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.alpha = 0;
        			ret=kdrv_vddo_get(handler, VDDO_DISPLAYER_CST_FROM_RGB_TO_YUV, &kdrv_disp_layer1);
                    if(ret!=ISF_OK)
        				return ret;

                    lyr->p_pale_entry[i] = lyr->p_pale_entry[i]&0xFF000000;//keep alpha value
                    lyr->p_pale_entry[i] |= ((kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.r_to_y)<<16)&0x00FF0000;
                    lyr->p_pale_entry[i] |= ((kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.g_to_u)<<8)&0x0000FF00;
                    lyr->p_pale_entry[i] |= (kdrv_disp_layer1.SEL.KDRV_VDDO_CST_OF_RGB_TO_YUV.b_to_v)&0x000000FF;
                    //DBG_DUMP("YUV p%i %x\r\n",i,lyr->p_pale_entry[i]);
                }
		    }
#endif
			kdrv_disp_layer.SEL.KDRV_VDDO_PALETTE.start = lyr->start;
			kdrv_disp_layer.SEL.KDRV_VDDO_PALETTE.number = lyr->number;
			kdrv_disp_layer.SEL.KDRV_VDDO_PALETTE.p_pale_entry = lyr->p_pale_entry;
			ret=kdrv_vddo_set(handler, VDDO_DISPLAYER_PALETTE, &kdrv_disp_layer);
			if(ret!=ISF_OK)
				return ISF_ERR_DRV;
			ret = kdrv_vddo_trigger(handler, NULL);
			if(ret!=ISF_OK)
				return ISF_ERR_DRV;

		}
		break;

		default:
			DBG_ERR("er.ctrl set struct[%08x] = %08x\r\n", nport-ISF_OUT_BASE, param);
			ret = ISF_ERR_NOT_SUPPORT;
			break;
		}

	} else if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
		switch (param) {
		default:
			DBG_ERR("er.out[%d] set struct[%08x] = %08x\r\n", nport-ISF_OUT_BASE, param, (UINT32)p_struct);
			ret = ISF_ERR_NOT_SUPPORT;
			break;
		}
	} else if (nport-ISF_IN_BASE < p_thisunit->port_in_count) {
		switch (param) {
		case VDOOUT_PARAM_CFG:
        {
            VDOOUT_FUNC_CFG *p_dev_cfg =  (VDOOUT_FUNC_CFG *)p_struct;
            p_ctx->usr_func = p_dev_cfg->in_func;
		}
		break;
		case VDOOUT_PARAM_VND_CFG:
        {
            VDOOUT_VND_CFG *p_vnd_cfg =  (VDOOUT_VND_CFG *)p_struct;
            p_ctx->vnd_func = p_vnd_cfg->in_func;
		}
		break;
		case VDOOUT_PARAM_VND_IN:
        {
            VDOOUT_VND_IN *p_vnd_in =  (VDOOUT_VND_IN *)p_struct;
            p_ctx->info.buffercount = p_vnd_in->queue_depth;
		}
        break;	
		case VDOOUT_PARAM_WIN_ATTR:
        {
            VDOOUT_WND_ATTR *p_wnd_cfg =  (VDOOUT_WND_ATTR *)p_struct;
            ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL,p_thisunit,nport,"p_wnd_cfg port=%d,layer=%d visible=%x",nport-ISF_IN_BASE,p_wnd_cfg->layer,p_wnd_cfg->visible);
            memcpy(&p_ctx->wnd_attr[nport-ISF_IN_BASE],p_wnd_cfg,sizeof(VDOOUT_WND_ATTR));
            ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL,p_thisunit,nport,"p_wnd_cfg %d %d %d %d",p_ctx->wnd_attr[nport-ISF_IN_BASE].rect.x,p_ctx->wnd_attr[nport-ISF_IN_BASE].rect.y,p_ctx->wnd_attr[nport-ISF_IN_BASE].rect.w,p_ctx->wnd_attr[nport-ISF_IN_BASE].rect.h);
		}
		break;
		default:
			DBG_ERR("er.in[%d] set struct[%08x] = %08x\r\n", nport-ISF_OUT_BASE, param, (UINT32)p_struct);
			ret = ISF_ERR_NOT_SUPPORT;
			break;
		}
	}
    return ret;

}
ISF_RV isf_vdoout_do_getportstruct(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size)
{
	VDOOUT_CONTEXT* p_ctx = (VDOOUT_CONTEXT*)(p_thisunit->refdata);
    UINT32 id = p_ctx->dev;
	ISF_RV ret = ISF_OK;

    ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL,p_thisunit,nport,"param = 0x%x, size=%d",param, size);


	if((p_ctx->open_tag != ISF_VDOOUT_FALG_OPEN)&&(param!=VDOOUT_PARAM_HDMI_ABILITY))
	    return ISF_ERR_NOT_OPEN_YET;

	if(!p_struct)
		return ISF_ERR_INVALID_PARAM;

	if (nport == ISF_CTRL) {
		switch(param) {
		case VDOOUT_PARAM_CAPS:
			memcpy((void *)p_struct,(void *)&p_ctx->info,sizeof(VDOOUT_INFO));
		break;
		case VDOOUT_PARAM_LYR_FMT:
		{
			UINT32 handler = KDRV_DEV_ID(0, isf_vo_engine[id], 0);
			KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer = {0};
			VDOOUT_LYR_FMT *lyr=(VDOOUT_LYR_FMT*)p_struct;

			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.layer = lyr->layer_id;
			ret=kdrv_vddo_get(handler, VDDO_DISPLAYER_BUFWINSIZE, &kdrv_disp_layer);
			if(ret!=ISF_OK)
				return ret;
			ret = kdrv_vddo_trigger(handler, NULL);
			if(ret!=ISF_OK)
				return ISF_ERR_DRV;

			lyr->fmt = kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.format;

			DBG_IND("id %d fmt %d\r\n",lyr->layer_id,lyr->fmt);

		}
		break;
		case VDOOUT_PARAM_PALETTE_TABLE:
		{
			UINT32 handler = KDRV_DEV_ID(0, isf_vo_engine[id], 0);
			KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer = {0};
			VDOOUT_PALETTE* lyr=(VDOOUT_PALETTE*)p_struct;

			ret=kdrv_vddo_get(handler, VDDO_DISPLAYER_PALETTE, &kdrv_disp_layer);
			if(ret!=ISF_OK)
				return ISF_ERR_DRV;
			ret = kdrv_vddo_trigger(handler, NULL);
			if(ret!=ISF_OK)
				return ISF_ERR_DRV;

			lyr->start = kdrv_disp_layer.SEL.KDRV_VDDO_PALETTE.start;
			lyr->number = kdrv_disp_layer.SEL.KDRV_VDDO_PALETTE.number;
			memcpy((void *)&lyr->p_pale_entry[0],(void *)&kdrv_disp_layer.SEL.KDRV_VDDO_PALETTE.p_pale_entry[0],sizeof(UINT32)*VDOOUT_PALETTE_MAX);
			DBG_IND("number %d %d,%d,%d\r\n",lyr->number,lyr->p_pale_entry[0],lyr->p_pale_entry[1],lyr->p_pale_entry[2]);

		}
		break;
        case VDOOUT_PARAM_HDMI_ABILITY:
        {
            UINT32 handler = KDRV_DEV_ID(0, isf_vo_engine[id], 0);
            KDRV_VDDO_DISPDEV_PARAM kdrv_disp_dev_temp;
            VDOOUT_HDMI_ABILITY* ability=(VDOOUT_HDMI_ABILITY*)p_struct;

            ret= kdrv_vddo_get(handler,VDDO_DISPDEV_HDMI_ABI,&kdrv_disp_dev_temp);
            if(ret!=ISF_OK)
				return ISF_ERR_DRV;

            memcpy((void *)ability,&kdrv_disp_dev_temp.SEL.KDRV_VDDO_HDMI_ABILITY,sizeof(VDOOUT_HDMI_ABILITY));

        }
        break;
		default:
			DBG_ERR("er.ctrl[%d] get struct[%08x] = %08x\r\n", nport-ISF_OUT_BASE, param, (UINT32)p_struct);
			ret = ISF_ERR_NOT_SUPPORT;
			break;
		}
	} else if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
		switch (param) {
		default:
			DBG_ERR("er.out[%d] get struct[%08x] = %08x\r\n", nport-ISF_OUT_BASE, param, (UINT32)p_struct);
			ret = ISF_ERR_NOT_SUPPORT;
			break;
		}
	} else if (nport-ISF_IN_BASE < p_thisunit->port_in_count) {
		switch (param) {
		case VDOOUT_PARAM_CFG:
        {
            VDOOUT_FUNC_CFG *p_dev_cfg =  (VDOOUT_FUNC_CFG *)p_struct;
            p_dev_cfg->in_func= p_ctx->usr_func;
		}
		break;
		case VDOOUT_PARAM_WIN_ATTR:
		break;
		default:
			DBG_ERR("er.in[%d] get struct[%08x] = %08x\r\n", nport-ISF_OUT_BASE, param, (UINT32)p_struct);
			ret = ISF_ERR_NOT_SUPPORT;
			break;
		}
	}
    return ret;

}
static ISF_RV _isf_vdoout_do_sync(UINT32 id,ISF_VDO_INFO *pImgInfo,VDOOUT_CONTEXT* p_ctx)
{
    UINT32 result=0;
    ISF_UNIT  *p_thisunit=isf_volist[id];
    VDOOUT_INFO info_keep;

    memcpy(&info_keep,&p_ctx->info,sizeof(VDOOUT_INFO));

    ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL,p_thisunit,ISF_IN(0),"dirty=0x%x,\r\ndirect=%d,\r\nimgsize.w=%d,\r\nimgsize.h=%d,\r\nfmt=0x%x,\r\nimgaspect.w=%d,\r\nimgaspect.h=%d,\r\nwindow.x=%d,\r\nwindow.y=%d,\r\nwindow.w=%d,\r\nwindow.h=%d", \
        pImgInfo->dirty,      \
        pImgInfo->direct,     \
        pImgInfo->imgsize.w,  \
        pImgInfo->imgsize.h,  \
        pImgInfo->imgfmt,     \
        pImgInfo->imgaspect.w,\
        pImgInfo->imgaspect.h,\
        pImgInfo->window.x,   \
        pImgInfo->window.y,   \
        pImgInfo->window.w,   \
        pImgInfo->window.h);

	if ((pImgInfo->dirty & ISF_INFO_DIRTY_IMGSIZE)
		|| (pImgInfo->dirty & ISF_INFO_DIRTY_IMGFMT)
		|| (pImgInfo->dirty & ISF_INFO_DIRTY_ASPECT)
		|| (pImgInfo->dirty & ISF_INFO_DIRTY_DIRECT)) { //check dirty of size|fmt|aspect|direct

        _isf_vdoout_set_image_size(id, pImgInfo->imgsize.w, pImgInfo->imgsize.h, pImgInfo->imgaspect.w, pImgInfo->imgaspect.h, pImgInfo->direct);
		p_ctx->info.bufFmt = pImgInfo->imgfmt;
		p_ctx->info.dir = pImgInfo->direct;
	}

	if ((pImgInfo->dirty & ISF_INFO_DIRTY_WINDOW)) { //check dirty of window
        //check vdo window
        if((pImgInfo->window.x>(UINT32)KDRV_VDOOUT_MAX_WIDTH)||(pImgInfo->window.w>(UINT32)KDRV_VDOOUT_MAX_WIDTH) ||
            (pImgInfo->window.y>(UINT32)KDRV_VDOOUT_MAX_HIGH)||(pImgInfo->window.h>(UINT32)KDRV_VDOOUT_MAX_HIGH)) {
            result= ISF_ERR_PARAM_EXCEED_LIMIT;
            DBG_ERR("win (%d,%d,%d,%d) > limit (0,0,%d,%d)\r\n",(int)pImgInfo->window.x,(int)pImgInfo->window.y,(int)pImgInfo->window.w,(int)pImgInfo->window.h,KDRV_VDOOUT_MAX_WIDTH,KDRV_VDOOUT_MAX_HIGH);
            //rollback ctx
            memcpy(&p_ctx->info,&info_keep,sizeof(VDOOUT_INFO));
            goto clear_dirty;
        }
        if((pImgInfo->window.x+pImgInfo->window.w>p_ctx->info.devSize.w)||(pImgInfo->window.y+pImgInfo->window.h>p_ctx->info.devSize.h)) {
            result= ISF_ERR_NOT_ALLOW;
            DBG_ERR("win (%d,%d,%d,%d) > device size (%d,%d)\r\n",(int)pImgInfo->window.x,(int)pImgInfo->window.y,(int)pImgInfo->window.w,(int)pImgInfo->window.h,(int)p_ctx->info.devSize.w,(int)p_ctx->info.devSize.h);
            //rollback ctx
            memcpy(&p_ctx->info,&info_keep,sizeof(VDOOUT_INFO));
            goto clear_dirty;
        }
		_isf_vdoout_set_window(id, pImgInfo->window.x, pImgInfo->window.y, pImgInfo->window.w, pImgInfo->window.h);
	}

    //check buf and window
    {
        VDOOUT_DIR dir = (pImgInfo->direct&VODOUT_ROT_MASK);

        if ((dir==VDOOUT_DIR_90)||(dir==VDOOUT_DIR_270)) {
            if( (p_ctx->info.ImgSize.w>p_ctx->info.winRect.h*2) || (p_ctx->info.ImgSize.h>p_ctx->info.winRect.w*2) ) {
                result= ISF_ERR_NOT_ALLOW;
                DBG_ERR("rotate buf (%d,%d)> win *2 (%d,%d),scale down ratio >2\r\n",p_ctx->info.ImgSize.w,p_ctx->info.ImgSize.h,p_ctx->info.winRect.w,p_ctx->info.winRect.h);
                //rollback ctx
                memcpy(&p_ctx->info,&info_keep,sizeof(VDOOUT_INFO));
                goto clear_dirty;
            }

        } else {
            if( (p_ctx->info.ImgSize.w>p_ctx->info.winRect.w*2) || (p_ctx->info.ImgSize.h>p_ctx->info.winRect.h*2) ) {
                result= ISF_ERR_NOT_ALLOW;
                DBG_ERR("buf (%d,%d)> win *2 (%d,%d),scale down ratio >2\r\n",p_ctx->info.ImgSize.w,p_ctx->info.ImgSize.h,p_ctx->info.winRect.w,p_ctx->info.winRect.h);
                //rollback ctx
                memcpy(&p_ctx->info,&info_keep,sizeof(VDOOUT_INFO));
                goto clear_dirty;
            }
        }
    }

    ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM,p_thisunit,ISF_IN(0),"after### direct=%d,\r\nimgsize.w=%d,\r\nimgsize.h=%d,\r\nfmt=0x%x,\r\nimgaspect.w=%d,\r\nimgaspect.h=%d,\r\nwindow.x=%d,\r\nwindow.y=%d,\r\nwindow.w=%d,\r\nwindow.h=%d \r\nbuffcount=%d \r\nusr_func 0x%x \r\nvnd_func 0x%x \r\n", \
        p_ctx->info.dir,        \
        p_ctx->info.bufSize.w,  \
        p_ctx->info.bufSize.h,  \
        p_ctx->info.bufFmt,     \
        p_ctx->info.ImgRatio.w, \
        p_ctx->info.ImgRatio.h, \
        p_ctx->info.winRect.x,  \
        p_ctx->info.winRect.y,  \
        p_ctx->info.winRect.w,  \
        p_ctx->info.winRect.h,  \
		p_ctx->info.buffercount,  \
        p_ctx->usr_func,        \
        p_ctx->vnd_func         \
        );

   ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM,p_thisunit,ISF_IN(0),"####output_type=%d \r\nlcd_ctrl=%d \r\nui_sif_ch=%d \r\nui_gpio_sen=0x%x \r\ngpio_clk=0x%x \r\nui_gpio_data=0x%x\r\n", \
        p_ctx->dev_cfg.mode.output_type, \
        p_ctx->dev_cfg.if_cfg.lcd_ctrl,p_ctx->dev_cfg.if_cfg.ui_sif_ch,  \
        p_ctx->dev_cfg.if_cfg.ui_gpio_sen,p_ctx->dev_cfg.if_cfg.ui_gpio_clk,p_ctx->dev_cfg.if_cfg.ui_gpio_data);

	{
	VDODISP_DISP_DESC desc = {0};
	VDODISP_CMD cmd;

	// set disp
	if((p_ctx->info.dir&~VODOUT_ROT_MASK) < VDOOUT_DIR_NO_HANDLE)
	    desc.degree =  (p_ctx->info.dir&~VODOUT_ROT_MASK);
    else
		desc.degree =  VDOOUT_DIR_NO_HANDLE;

	desc.wnd = *(URECT *) & (p_ctx->info.winRect);
	desc.aspect_ratio = p_ctx->info.devAspect;
	desc.device_size = p_ctx->info.devSize;
	memset(&cmd, 0, sizeof(cmd));
	cmd.device_id = id;
	cmd.idx = VDODISP_CMD_IDX_SET_DISP;
	cmd.in.p_data = &desc;
	cmd.in.ui_num_byte = sizeof(desc);
	cmd.prop.b_exit_cmd_finish = TRUE;
	result = vdodisp_cmd(&cmd);
	if(result!=0)
		return ISF_ERR_DRV;
	}
clear_dirty:
	//clear dirty
	{
		if ((pImgInfo->dirty & ISF_INFO_DIRTY_IMGSIZE)
			|| (pImgInfo->dirty & ISF_INFO_DIRTY_IMGFMT)
			|| (pImgInfo->dirty & ISF_INFO_DIRTY_ASPECT)
			|| (pImgInfo->dirty & ISF_INFO_DIRTY_DIRECT)) { //check dirty of size|fmt|aspect|direct
			pImgInfo->dirty &= ~(ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_ASPECT | ISF_INFO_DIRTY_DIRECT);
		}
		if ((pImgInfo->dirty & ISF_INFO_DIRTY_WINDOW)) { //check dirty of window
			pImgInfo->dirty &= ~(ISF_INFO_DIRTY_WINDOW);
		}
	}

    return result;

}
static ISF_RV _isf_vdoout_set_state(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 state, MEM_RANGE *pMem)
{
	VDOOUT_CONTEXT* p_ctx = (VDOOUT_CONTEXT*)(p_thisunit->refdata);
	ISF_INFO *p_src_info = p_thisunit->port_ininfo[oport];
	ISF_VDO_INFO *pImgInfo = (ISF_VDO_INFO *)(p_src_info);
    UINT32 id = p_ctx->dev;
 	VDODISP_ER result =0;
	VDODISP_CMD cmd;

    ISF_UNIT_TRACE(ISF_OP_STATE,p_thisunit,oport,"set_mode=%d tag %x",state,p_ctx->open_tag);

	if(p_ctx->open_tag != ISF_VDOOUT_FALG_OPEN)
	    return ISF_ERR_NOT_OPEN_YET;

	if (state == VDOOUT_STATE_SYNC) { //do READYTIME_SYNC

        //resume vdodisp task for cmd setting
        result = vdodisp_resume(id);
        if(result!=0)
			return ISF_ERR_INVALID_STATE;

        //sync pImgInfo to p_ctx
        result=_isf_vdoout_do_sync(id,pImgInfo,p_ctx);
		if(result!=0)
			return ISF_ERR_INVALID_VALUE;

		p_ctx->state = state;

	} else if(state ==VDOOUT_STATE_START) {
		VDODISP_EVENT_CB event_cb = {0};

        if((p_ctx->info.buffercount <= QUEUE_MAXSIZE) && (p_ctx->info.buffercount>0)) {
    		gQueue[id].Total = p_ctx->info.buffercount;
    	}else {
			gQueue[id].Total =QUEUE_DEFSIZE;
		}

        // set notify callback
		event_cb.fp_release = _isf_vdoout_finish_release; //for sync buffer to IPL
		memset(&cmd, 0, sizeof(cmd));
		cmd.device_id = id;
		cmd.idx = VDODISP_CMD_IDX_SET_EVENT_CB;
		cmd.in.p_data = &event_cb;
		cmd.in.ui_num_byte = sizeof(event_cb);
		cmd.prop.b_exit_cmd_finish = TRUE;
		result = vdodisp_cmd(&cmd);
		if(result!=0)
			return ISF_ERR_INVALID_VALUE;

		p_ctx->state = state;
		
		{
			int ret;
			extern int start_sub_wnd(ISF_UNIT *p_thisunit);

			if(p_ctx->flow_ctrl & VDOOUT_FUNC_MERGE_WIN){
				ret = start_sub_wnd(p_thisunit);
				if(ret)
					DBG_ERR("fail to start sub window\n");
			}
		}

	} else if(state==VDOOUT_STATE_RUNTIME_SYNC) {  //// do RUNTIME_SYNC
	    //only update port data to dev,not change state keep start
	    {
			int ret;
			extern int start_sub_wnd(ISF_UNIT *p_thisunit);

			if(p_ctx->flow_ctrl & VDOOUT_FUNC_MERGE_WIN){
				ret = start_sub_wnd(p_thisunit);
				if(ret)
					DBG_ERR("fail to start sub window\n");
				return ISF_OK;
			}
		}
        result=_isf_vdoout_do_sync(id,pImgInfo,p_ctx);
		if(result!=0)
			return ISF_ERR_INVALID_VALUE;
        //[IVOT_N01002_CO-537]should keep origin state,not change to run time sync
        //p_ctx->state = state ;

	} else 	{   ////do STOP
	    //chage to stop state at begine,avoid push data
		p_ctx->state = VDOOUT_STATE_STOP;
        //wait func,need to push null image to release previous one
        if(!(p_ctx->usr_func & VDOOUT_FUNC_NOWAIT)) {
            //set tmp data for release last frame
    		ISF_DATA *pData = &(gISF_DispTempData[id]);
    		VDODISP_CMD cmd;
    		VDO_FRAME *p_img = NULL;
    		memset(&cmd, 0, sizeof(cmd));
    		cmd.device_id = id;
    		cmd.idx = VDODISP_CMD_IDX_RELEASE;
    		//clear data
    		memset(pData, 0, sizeof(ISF_DATA));
            //new data from external
    		if(p_ctx->vnd_func & VDOOUT_FUNC_KEEP_LAST)
    		{
               	UINT32 bufaddr = 0;
			    USIZE imgsize = p_ctx->info.bufSize;
    		    UINT32 fmt = p_ctx->info.bufFmt;
                VDOOUT_DIR dir = (p_ctx->info.dir&VODOUT_ROT_MASK);
                UINT32 bufsize = 0;
                if ((dir==VDOOUT_DIR_90)||(dir==VDOOUT_DIR_270)) {
                    UINT32 tmp_w = imgsize.w;
                    imgsize.w = imgsize.h;
                    imgsize.h = tmp_w;
                    if(fmt==VDO_PXLFMT_YUV422) {
                        fmt = VDO_PXLFMT_YUV420;
                        DBG_WRN("422 rotate to 420\r\n");
                    }
                }
                DBG_IND("keep last : w %d,h %d dir %d fmt %x\r\n",imgsize.w,imgsize.h,dir,fmt);
                bufsize = gximg_calc_require_size(ALIGN_CEIL_16(imgsize.h), ALIGN_CEIL_16(imgsize.w), fmt,ALIGN_CEIL_16(imgsize.h));
                if(bufsize)
        			result=_isf_vdoout_alloc_tmp_buf(p_thisunit->unit_module,&bufaddr,bufsize);
                if((result ==0) && bufaddr) {
                    VDO_FRAME *p_tmp_img = (VDO_FRAME *)pData->desc;
		            result = gximg_init_buf(p_tmp_img, imgsize.w, imgsize.h, fmt, GXIMG_LINEOFFSET_ALIGN(4), bufaddr, bufsize);
                    if(result==0) {
                        DBG_IND("keep last :bufsize %d ,addr %x\r\n",bufsize,bufaddr);
                	    pData->sign = ISF_SIGN_DATA;
                	    pData->h_data = bufaddr; //fill addr if ok
                	    p_img = (VDO_FRAME *)pData->desc;
                    }
                } //no buffer would not keep last,still release last frame
    		}

    		cmd.in.p_data = &p_img;
    		cmd.in.ui_num_byte = sizeof(p_img);
    		cmd.prop.b_exit_cmd_finish = TRUE;

    		result = vdodisp_cmd(&cmd);
    		if(result!=VDODISP_ER_OK){
    			DBG_ERR("cmd rel %d\r\n",result);
    			return ISF_ERR_PROCESS_FAIL;
    		}
            p_ctx->vnd_func = p_ctx->vnd_func&~VDOOUT_FUNC_KEEP_LAST;
		
			{
				int ret;
				extern int stop_sub_wnd(ISF_UNIT *p_thisunit);
				if(p_ctx->flow_ctrl & VDOOUT_FUNC_MERGE_WIN){
					ret = stop_sub_wnd(p_thisunit);
					if(ret)
						DBG_ERR("fail to stop sub window\n");
				}
			}
		}
		result = vdodisp_suspend(id);
	}

    ISF_UNIT_TRACE(ISF_OP_STATE,p_thisunit,oport,"state=%d",p_ctx->state);


	return result;
}

ISF_RV isf_vdoout_bind_input(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport)
{
	return ISF_OK;
}
ISF_RV isf_vdoout_bind_output(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport)
{
	return ISF_OK;
}
ISF_RV isf_vdoout_inputport_do_push(UINT32 id,ISF_PORT *p_port, ISF_DATA *p_data, INT32 wait_ms)
{
	INT32 push_r =ISF_OK;
    INT32 proc_r =ISF_OK;
    INT32 ret = ISF_OK;
    ISF_UNIT *p_thisunit=isf_volist[id];
    VDOQUEUE *pQueue = &(gQueue[id]);
	VDOOUT_CONTEXT* p_ctx = (VDOOUT_CONTEXT*)(p_thisunit->refdata);
    VDO_FRAME *p_push_img = (VDO_FRAME *)p_data->desc;
    unsigned long      flags;     /* flags should be local variable */
    VDO_FRAME *pImg;
    VDOOUT_DIR dir = (p_ctx->info.dir&VODOUT_ROT_MASK);
#if (OSG_ROTATE)
    ISF_DATA *p_data_rotate=0;
#endif

    p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH, ISF_ENTER);

    if (p_push_img->sign != MAKEFOURCC('V','F','R','M')) {
        p_thisunit->p_base->do_release(p_thisunit, ISF_IN(0), p_data, ISF_IN_PROBE_PUSH);
        p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INVALID_SIGN);
        return ISF_ERR_INVALID_SIGN;
    }

	if(p_ctx->state==VDOOUT_STATE_START) {
        if(p_ctx->usr_func & VDOOUT_FUNC_NOWAIT){
            pQueue->Head =0; //always use first queue data
        }

		//push data to queue
		loc_cpu(flags);
	    if(!(p_ctx->usr_func & VDOOUT_FUNC_NOWAIT)&&(pQueue->Cnt >= pQueue->Total)) {
			//queue is full, not allow push
			p_thisunit->p_base->do_release(p_thisunit, ISF_IN(0), p_data, ISF_IN_PROBE_PUSH);
            push_r = ISF_ERR_QUEUE_FULL;
            unl_cpu(flags);

			#if 0  //dump queue
			{
                UINT i =0;
				DBG_ERR("[%d]full %d drop pBuf %x\r\n",id,pQueue->Cnt,p_push_img->addr[0]);
                for(i=0;i<pQueue->Cnt;i++) {
				    DBG_DUMP("q%d %x\r\n",i,((VDO_FRAME *)pQueue->Data[i].desc)->addr[0]);
                }
			}
			#endif

		} else {

            p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH, ISF_OK);
            p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PROC, ISF_ENTER);

            if(p_push_img->loff[0] & (KDRV_VDOOUT_V1_BUF_LOFF_ALIGN-1)){
                DBG_ERR("lineoff %d not %d align\r\n",(int)p_push_img->loff[0],(int)KDRV_VDOOUT_V1_BUF_LOFF_ALIGN);
                p_thisunit->p_base->do_release(p_thisunit, ISF_IN(0), p_data, ISF_IN_PROBE_PUSH);
                p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_PARAM_EXCEED_LIMIT);
                unl_cpu(flags);
                return ISF_ERR_PARAM_EXCEED_LIMIT;
            }


            //--- ISF_DATA from HDAL user push_in ---
			if ((p_push_img->addr[0] == 0) && (p_push_img->phyaddr[0] != 0) && (p_push_img->loff[0] != 0)) {
				// get virtual addr
				p_push_img->addr[0] = nvtmpp_sys_pa2va(p_push_img->phyaddr[0]);
				p_push_img->addr[1] = nvtmpp_sys_pa2va(p_push_img->phyaddr[1]);
                ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM,p_thisunit,ISF_IN(0),"push p_data %x pImg %x",((VDO_FRAME  *)p_data->desc)->addr[0],p_push_img->addr[0]);

			}

			memcpy((void *)&pQueue->Data[pQueue->Head],p_data,sizeof(ISF_DATA));
			pImg = (VDO_FRAME *)(pQueue->Data[pQueue->Head].desc);
            pImg->resv1 = pQueue->Head;
#if (OSG_ROTATE)
            p_data_rotate = &pQueue->Data[pQueue->Head];
#endif
            //ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM,p_thisunit,ISF_IN(0),"push p_data %x pImg %x",((VDO_FRAME  *)p_data->desc)->addr[0],pImg->addr[0]);

			//DBG_DUMP("pImg %x\r\n",pImg->addr[0]);

            #if VO_FRAME_DBG
            if(id==0) //debug IDE 1
            {
                DMA_WRITEPROT_ATTR attrib = {0};

                memset((void *)&attrib.mask, 0xff, sizeof(DMA_CH_MSK));

                attrib.level = DMA_WPLEL_UNWRITE;
                attrib.protect_rgn_attr[0].en = ENABLE;
                attrib.protect_rgn_attr[0].starting_addr = nvtmpp_sys_va2pa(pImg->addr[0]);
                attrib.protect_rgn_attr[0].size = pImg->loff[0]*pImg->ph[0]*2;

                if(pQueue->Head==0){
                    arb_enable_wp(DDR_ARB_1, WPSET_0, &attrib);
                    //DBG_ERR("WP0 %x %x %d %d\r\n",attrib.start_addr,attrib.size,pImg->loff[0],pImg->ph[0]);
                } else {
                    arb_enable_wp(DDR_ARB_1, WPSET_1, &attrib);
                }
            }
            #endif

            if(!(p_ctx->usr_func & VDOOUT_FUNC_NOWAIT)){
    			if (pQueue->Cnt == 0) {
    				pQueue->Tail = pQueue->Head;
    			}
    			pQueue->Head ++;
    			if (pQueue->Head >= pQueue->Total) {
    				pQueue->Head = 0;
    			}
    			pQueue->Cnt ++;
            }
			unl_cpu(flags);

            //do rotate
            if ((dir==VDOOUT_DIR_90)||(dir==VDOOUT_DIR_270)) {
#if (OSG_ROTATE)
                UINT grap_dir =0;
		        VDO_FRAME *pImgRotate = (VDO_FRAME *)&p_data_rotate->desc[0];
                //static ISF_DATA_CLASS g_vdoout_data = {0};


                if (dir==VDOOUT_DIR_90) {
                    grap_dir = VDO_DIR_ROTATE_90;
                }else if (dir==VDOOUT_DIR_270) {
                    grap_dir = VDO_DIR_ROTATE_270;
                }

                {
			    USIZE imgsize = p_ctx->info.ImgSize;
    		    UINT32 fmt = p_ctx->info.bufFmt;
                UINT32 buf_size = gximg_calc_require_size(ALIGN_CEIL_16(imgsize.h), ALIGN_CEIL_16(imgsize.w), fmt,ALIGN_CEIL_16(imgsize.h));
                UINT32 buf = 0;
                ER gximg_r = 0;

                //skip init p_data_rotate,becase p_data_rotate has copy from push p_data
                //p_thisunit->p_base->init_data(p_data_rotate, &g_vdoout_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
                if(buf_size)
                    buf = p_thisunit->p_base->do_new(p_thisunit, ISF_IN(0), NVTMPP_VB_INVALID_POOL, 0, buf_size, p_data_rotate, ISF_OUT_PROBE_EXT);


                //DBG_IND("buf_size %x new buf %x org %x\r\n",buf_size,buf,pImg->addr[0]);
                ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM,p_thisunit,ISF_IN(0),"buf_size %x new buf %x org %x",buf_size,buf,pImg->addr[0]);

                if(!buf){
                    DBG_ERR("new buf_size %x fail\r\n",buf_size);
                    _isf_vdoout_release_head(id,NULL);
                    proc_r= ISF_ERR_NEW_FAIL;
                }
                if(proc_r==ISF_OK){
                    gximg_r = gximg_rotate_data_no_flush(pImg, buf, buf_size, grap_dir, pImgRotate);
                    if(gximg_r!=0) {
                        DBG_ERR("rotate r %d\r\n",gximg_r);
                        pImgRotate->addr[0] = buf;
                        _isf_vdoout_release_head(id,pImgRotate);
                        proc_r = ISF_ERR_DATA_EXCEED_LIMIT;
                    } else {
                        //set rotate to pImg
                        pImg=pImgRotate;
                    }
                }
                //release cur pImg
                //ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM,p_thisunit,ISF_IN(0),"rel p_data %x",((VDO_FRAME  *)p_data->desc)->addr[0]);
                p_thisunit->p_base->do_release(p_thisunit, ISF_IN(0), p_data, ISF_IN_PROBE_PROC);
                }
#endif
            }


            if(proc_r==ISF_OK)
            {  //callback to OSG
#if (OSG_ROTATE)
    			UINT32 oport = ISF_OUT(0);
    			_isf_vdoout_do_output_mask(p_thisunit, oport, pImg, pImg);
    			_isf_vdoout_do_output_osd(p_thisunit, oport, pImg, pImg);
#endif
			}

            if(proc_r==ISF_OK) {
    			VDODISP_CMD cmd;
                VDODISP_ER cmd_r = VDODISP_ER_OK;
    			memset(&cmd, 0, sizeof(cmd));
    			cmd.device_id = id;
    			cmd.idx = VDODISP_CMD_IDX_SET_FLIP;
    			cmd.in.p_data = &pImg;
    			cmd.in.ui_num_byte = sizeof(pImg);
        		//cmd.prop.b_exit_cmd_finish = TRUE;

                ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM,p_thisunit,ISF_IN(0),"vdodisp addr %x ",pImg->addr[0]);

    			cmd_r = vdodisp_cmd(&cmd);
    			if(cmd_r!=VDODISP_ER_OK){
    				DBG_ERR("[%d]FLIP r %d\r\n",id,cmd_r);
      				_isf_vdoout_release_head(id,pImg);
                    proc_r = ISF_ERR_PROCESS_FAIL;
    			}


                if((p_ctx->usr_func & VDOOUT_FUNC_NOWAIT)){ //release frame buffer immediately
                    ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM,p_thisunit,ISF_IN(0),"rel Head %x %x",((VDO_FRAME  *)pQueue->Data[pQueue->Head].desc)->addr[0]);
      				p_thisunit->p_base->do_release(p_thisunit, ISF_IN(0), &pQueue->Data[pQueue->Head], ISF_IN_PROBE_PROC);
    			}

                if(proc_r==ISF_OK)
                    p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PROC, ISF_OK);
                else
                    p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PROC_ERR,proc_r );

            }
		}
	} else {
		//DBG_WRN("not start %d\r\n",p_ctx->state);
        p_thisunit->p_base->do_release(p_thisunit, ISF_IN(0), p_data, ISF_IN_PROBE_PUSH);
        push_r = ISF_ERR_NOT_START;
	}


    if(push_r==0) {
        ret = proc_r;
    } else {
        if(push_r==ISF_ERR_NOT_START)
            p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH_DROP, push_r);
        else if(push_r==ISF_ERR_QUEUE_FULL)
            p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH_WRN, push_r);
#if 0 //for coverity 133160
        else
            p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH_ERR, push_r);
#endif
        ret = push_r;
    }
	return ret;
}

ISF_RV isf_vdopout_do_command(UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2)
{
	UINT32 h_isf = (cmd >> 28); //extract h_isf
	cmd &= ~0xf0000000; //clear h_isf

	switch(cmd) {
	case 1: //INIT
		return _isf_vdoout_do_init(h_isf, p1); //p1: max dev count
		break;
	case 0: //UNINIT
		return _isf_vdoout_do_uninit(h_isf, p1); //p1: is under reset
		break;
	default:
		break;
	}
	return ISF_ERR_NOT_SUPPORT;
}

ISF_RV isf_vdoout_do_update(UINT32 id,ISF_UNIT  *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd)
{
    ISF_UNIT_TRACE(ISF_OP_COMMAND,p_thisunit,oport,"update_cmd=%x",cmd);

	switch (cmd) {
	case ISF_PORT_CMD_RESET:
		return _isf_vdoout_do_reset(p_thisunit, oport);
		break;
	case ISF_PORT_CMD_OFFTIME_SYNC:     ///< off -> off     (sync port info & sync off-time parameter if it is dirty)
		break;
	case ISF_PORT_CMD_OPEN:             ///< off -> ready   (alloc mempool and start task)
		return _isf_vdoout_open(id,p_thisunit, oport);
		break;
	case ISF_PORT_CMD_READYTIME_SYNC:   ///< ready -> ready (sync port info & sync ready-time parameter if it is dirty)
		return _isf_vdoout_set_state(p_thisunit, oport,VDOOUT_STATE_SYNC,0);
		break;
	case ISF_PORT_CMD_START:            ///< ready -> run   (initial context, engine enable and device power on, start data processing)
		return _isf_vdoout_set_state(p_thisunit, oport,VDOOUT_STATE_START,0);
		break;
	case ISF_PORT_CMD_RUNTIME_SYNC:     ///< run -> run     (sync port info & sync run-time parameter if it is dirty)
		return _isf_vdoout_set_state(p_thisunit, oport,VDOOUT_STATE_RUNTIME_SYNC,0);
		break;
	case ISF_PORT_CMD_RUNTIME_UPDATE:   ///< run -> run     (update change)
		break;
	case ISF_PORT_CMD_STOP:             ///< run -> stop    (stop data processing, engine disable or device power off)
		return _isf_vdoout_set_state(p_thisunit, oport,VDOOUT_STATE_STOP,0);
		break;
	case ISF_PORT_CMD_CLOSE:            ///< stop -> off    (terminate task and free mempool)
		return _isf_vdoout_close(id,p_thisunit, oport);
		break;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case ISF_PORT_CMD_PAUSE:            ///< run -> wait    (pause data processing, keep context)
		break;
	case ISF_PORT_CMD_RESUME:           ///< wait -> run    (restore context, resume data processing)
		break;
	case ISF_PORT_CMD_SLEEP:            ///< run -> sleep   (pause data processing, keep context, engine or device enter suspend mode)
		break;
	case ISF_PORT_CMD_WAKEUP:           ///< sleep -> run   (engine or device leave suspend mode, restore context, resume data processing)
		break;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	default :
		return ISF_OK;
	}
	return ISF_OK;
}

///////////////////////////////////////////////////////////////////////////////


