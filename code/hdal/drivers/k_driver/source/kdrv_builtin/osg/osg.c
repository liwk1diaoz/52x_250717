#include <osg_internal.h>
#include "kwrap/error_no.h"
#include "kwrap/spinlock.h"
#include "kwrap/debug.h"
#include "h26xenc_api.h"
#include "kdrv_videoenc/kdrv_videoenc_lmt.h"
#include "kdrv_gfx2d/kdrv_grph.h"
#include <plat/top.h>

#ifdef __KERNEL__

VK_DEFINE_SPINLOCK(BUILTIN_OSG_LOCK);

static BUILTIN_OSG osg_info[BUILTIN_VDOENC_PATH_ID_MAX] = { 0 };

int osg_module_init(void)
{
	memset(osg_info, 0, sizeof(osg_info));
	
	if(builtin_osg_demo_init()){
		DBG_ERR("fail to initialize customer's OSG setting\n");
		return -1;
	}

	return 0;
}

int vds_set_early_osg(UINT32 path, OSG *osg)
{
	unsigned long flag;
	int           ret = -1, i, size;

	if(path >= BUILTIN_VDOENC_PATH_ID_MAX){
		DBG_ERR("path(%u) > max(%u)\n", path, BUILTIN_VDOENC_PATH_ID_MAX);
		return -1;
	}

	if(osg == NULL){
		DBG_ERR("argument osg is NULL\n");
		return -1;
	}

	if(osg->stamp == NULL){
		DBG_ERR("osg->stamp is NULL\n");
		return -1;
	}

	if(osg->num >= BUILTIN_OSG_MAX_NUM){
		DBG_ERR("osg number(%d) > max(%d)\n", osg->num, BUILTIN_OSG_MAX_NUM);
		return -1;
	}

	vk_spin_lock_irqsave(&BUILTIN_OSG_LOCK, flag);

	osg_info[path].num = osg->num;

	for(i = 0 ; i < osg->num ; ++i){

		if(osg->stamp[i].buf.type != OSG_BUF_TYPE_SINGLE && osg->stamp[i].buf.type != OSG_BUF_TYPE_PING_PONG){
			DBG_ERR("invalid osg->stamp[%d].buf.type(%u)\n", i, osg->stamp[i].buf.type);
			goto out;
		}

		if(osg->stamp[i].buf.size == 0){
			DBG_ERR("osg->stamp[%d].buf.size is 0\n", i);
			goto out;
		}

		if(osg->stamp[i].buf.size & 0x07f){
			DBG_ERR("osg->stamp[%d].buf.size(%u) is not 128 aligned\n", i, osg->stamp[i].buf.size);
			goto out;
		}

		if(osg->stamp[i].buf.p_addr == 0){
			DBG_ERR("osg->stamp[%d].buf.p_addr is 0\n", i);
			goto out;
		}

		if(osg->stamp[i].buf.p_addr & 0x07f){
			DBG_ERR("osg->stamp[%d].buf.p_addr(%lx) is not 128 aligned\n", i, (unsigned long)osg->stamp[i].buf.p_addr);
			goto out;
		}

		if(osg->stamp[i].img.dim.w == 0){
			DBG_ERR("osg->stamp[%d].img.dim.w is 0\n", i);
			goto out;
		}

		if(osg->stamp[i].img.dim.h == 0){
			DBG_ERR("osg->stamp[%d].img.dim.h is 0\n", i);
			goto out;
		}

		size = osg->stamp[i].img.dim.w;
		if(osg->stamp[i].img.fmt == OSG_PXLFMT_ARGB1555){
			size *= 2;
		}else if(osg->stamp[i].img.fmt == OSG_PXLFMT_ARGB4444){
			size *= 2;
		}else if(osg->stamp[i].img.fmt == OSG_PXLFMT_ARGB8888){
			size *= 4;
		}else{
			DBG_ERR("invalid osg->stamp[%d].img.fmt(%x)\n", i, osg->stamp[i].img.fmt);
			goto out;
		}
		size *= osg->stamp[i].img.dim.h;

		if(osg->stamp[i].buf.type == OSG_BUF_TYPE_PING_PONG){
			size *= 2;
		}
		if(size > osg->stamp[i].buf.size){
			DBG_ERR("osg->stamp[%d] needs %d bytes but only %d bytes are allocated\n", i, size, osg->stamp[i].buf.size);
			goto out;
		}

		if(osg->stamp[i].attr.layer >= 2){
			DBG_ERR("invalid osg->stamp[%d].attr.layer(%u). Only 0 and 1 are valid\n", i, osg->stamp[i].attr.layer);
			goto out;
		}

		if(osg->stamp[i].attr.region >= 16){
			DBG_ERR("invalid osg->stamp[%d].attr.region(%u). Only 0~15 are valid\n", i, osg->stamp[i].attr.region);
			goto out;
		}

		osg_info[path].stamp[i].buf.type        = osg->stamp[i].buf.type;
		osg_info[path].stamp[i].buf.ddr_id      = osg->stamp[i].buf.ddr_id;

		if(osg->stamp[i].buf.type == OSG_BUF_TYPE_PING_PONG){
			osg_info[path].stamp[i].buf.size      = (osg->stamp[i].buf.size >> 1);
			osg_info[path].stamp[i].buf.p_addr[0] = osg->stamp[i].buf.p_addr;
			osg_info[path].stamp[i].buf.p_addr[1] = (osg_info[path].stamp[i].buf.p_addr[0] + osg_info[path].stamp[i].buf.size);
		}else{
			osg_info[path].stamp[i].buf.size      = osg->stamp[i].buf.size;
			osg_info[path].stamp[i].buf.p_addr[0] = osg->stamp[i].buf.p_addr;
			osg_info[path].stamp[i].buf.p_addr[1] = osg->stamp[i].buf.p_addr;
		}

		osg_info[path].stamp[i].img.fmt         = osg->stamp[i].img.fmt;
		osg_info[path].stamp[i].img.dim.w       = osg->stamp[i].img.dim.w;
		osg_info[path].stamp[i].img.dim.h       = osg->stamp[i].img.dim.h;

		osg_info[path].stamp[i].attr.alpha      = osg->stamp[i].attr.alpha;
		osg_info[path].stamp[i].attr.position.x = osg->stamp[i].attr.position.x;
		osg_info[path].stamp[i].attr.position.y = osg->stamp[i].attr.position.y;
		osg_info[path].stamp[i].attr.layer      = osg->stamp[i].attr.layer;
		osg_info[path].stamp[i].attr.region     = osg->stamp[i].attr.region;
	}

	ret = 0;

out:

	vk_spin_unlock_irqrestore(&BUILTIN_OSG_LOCK, flag);

	return ret;
}

int vds_update_early_osg(UINT32 path, int idx, uintptr_t p_addr)
{
	unsigned long flag;
	int           ret = -1;
	uintptr_t     dst;
	UINT32        size;

	if(path >= BUILTIN_VDOENC_PATH_ID_MAX){
		DBG_ERR("invalid encoding path(%u). max is %u\n", path, BUILTIN_VDOENC_PATH_ID_MAX);
		return -1;
	}

	if(idx >= BUILTIN_OSG_MAX_NUM){
		DBG_ERR("osg idx(%d) of encoding path(%u) > max(%d)\n", idx, path, BUILTIN_OSG_MAX_NUM);
		return -1;
	}

	if(p_addr == 0){
		DBG_ERR("set NULL p_addr to osg idx(%d) of encoding path(%u)\n", idx, path);
		return -1;
	}

	vk_spin_lock_irqsave(&BUILTIN_OSG_LOCK, flag);

	if(osg_info[path].stamp[idx].buf.size == 0 || osg_info[path].stamp[idx].buf.p_addr[0] == 0){
		DBG_ERR("try to update osg image but osg->stamp[%d] of encoding path(%u) is not init\n", idx, path);
		goto out;
	}
	
	if(osg_info[path].stamp[idx].buf.swap > 1){
		DBG_ERR("invalid osg->stamp[%d].buif.swap(%u) of encoding path(%u)\n", idx, osg_info[path].stamp[idx].buf.swap, path);
		goto out;
	}

	dst  = osg_info[path].stamp[idx].buf.p_addr[osg_info[path].stamp[idx].buf.swap];
	size = (osg_info[path].stamp[idx].img.dim.w * osg_info[path].stamp[idx].img.dim.h);
	if(osg_info[path].stamp[idx].img.fmt == OSG_PXLFMT_ARGB1555){
		size *= 2;
	}else if(osg_info[path].stamp[idx].img.fmt == OSG_PXLFMT_ARGB4444){
		size *= 2;
	}else if(osg_info[path].stamp[idx].img.fmt == OSG_PXLFMT_ARGB8888){
		size *= 4;
	}else{
		DBG_ERR("invalid osg->stamp[%d].img.fmt(%x)\n", idx, osg_info[path].stamp[idx].img.fmt);
		goto out;
	}
	
	if(size > osg_info[path].stamp[idx].buf.size){
		DBG_ERR("osg->stamp[%d] requires %d bytes but only %d bytes are allocated\n", idx, size, osg_info[path].stamp[idx].buf.size);
		goto out;
	}

	//avoid memcpy to block encoding thread
	vk_spin_unlock_irqrestore(&BUILTIN_OSG_LOCK, flag);
	memcpy((void*)dst, (void*)p_addr, size);
	vk_spin_lock_irqsave(&BUILTIN_OSG_LOCK, flag);

	if(osg_info[path].stamp[idx].buf.swap == 0){
		osg_info[path].stamp[idx].buf.swap = 1;
	}else{
		osg_info[path].stamp[idx].buf.swap = 0;
	}
	osg_info[path].stamp[idx].valid = 1;

	ret = 0;

out:

	vk_spin_unlock_irqrestore(&BUILTIN_OSG_LOCK, flag);

	return ret;
}

int builtin_osg_setup_h26x_stamp(H26XENC_VAR *pVar, UINT32 path)
{
	H26XEncOsgRgbCfg stRgbCfg = {0};
	H26XEncOsgWinCfg stWinCfg = {0};
	int              i, j, ret = -1;
	unsigned long    flag;
	unsigned int     layer1_osd = 0, layer2_osd = 0, loff = 0;
	
	if(pVar == NULL){
		DBG_ERR("pVar of encoding path(%u) is NULL\n", path);
		return -1;
	}
	
	if(path >= BUILTIN_VDOENC_PATH_ID_MAX){
		DBG_ERR("invalid encoding path(%u). max is %u\n", path, BUILTIN_VDOENC_PATH_ID_MAX);
		return -1;
	}

	stRgbCfg.ucRgb2Yuv[0][0] = 0x26;
	stRgbCfg.ucRgb2Yuv[0][1] = 0x4b;
	stRgbCfg.ucRgb2Yuv[0][2] = 0x0f;
	stRgbCfg.ucRgb2Yuv[1][0] = 0xea;
	stRgbCfg.ucRgb2Yuv[1][1] = 0xd5;
	stRgbCfg.ucRgb2Yuv[1][2] = 0x41;
	stRgbCfg.ucRgb2Yuv[2][0] = 0x41;
	stRgbCfg.ucRgb2Yuv[2][1] = 0xc9;
	stRgbCfg.ucRgb2Yuv[2][2] = 0xf5;
	if(!h26XEnc_setOsgRgbCfg(pVar, &stRgbCfg)){
		DBG_ERR("h26XEnc_setOsgRgbCfg() for encoding path(%u) fail\n", path);
		return -1;
	}

	stWinCfg.bEnable = 0;
	for(i = 0 ; i < 2 ; ++i){
		for(j = 0 ; j < 16 ; ++j){
			if(!h26XEnc_setOsgWinCfg(pVar, ((i<<4) + j), &stWinCfg)){
				DBG_ERR("h26XEnc_setOsgRgbCfg(layer=%d;region=%d;en=0) for encoding path(%u) fail\n", i, j, path);
				return -1;
			}
		}
	}

	vk_spin_lock_irqsave(&BUILTIN_OSG_LOCK, flag);
	
	if(osg_info[path].num > BUILTIN_OSG_MAX_NUM){
		DBG_ERR("osg number(%d) of encoding path(%u) > max(%d)\n", osg_info[path].num, path, BUILTIN_OSG_MAX_NUM);
		goto out;
	}
	
	for(i = 0 ; i < osg_info[path].num ; ++i){
		
		if(osg_info[path].stamp[i].valid == 0){
			continue;
		}
		
		if(osg_info[path].stamp[i].attr.layer < 0 || osg_info[path].stamp[i].attr.layer > 1){
			DBG_ERR("invalid stamp[%d] layer(%d)\r\n", i, osg_info[path].stamp[i].attr.layer);
			goto out;
		}

		if(osg_info[path].stamp[i].attr.region < 0 || osg_info[path].stamp[i].attr.region > 15){
			DBG_ERR("invalid stamp[%d] region(%d)\r\n", i, osg_info[path].stamp[i].attr.region);
			goto out;
		}

		if(osg_info[path].stamp[i].attr.layer == 0){
			if(layer1_osd & (1 << osg_info[path].stamp[i].attr.region)){
				DBG_ERR("stamp[%d] has duplicate layer(1) region(%d)\r\n", i, osg_info[path].stamp[i].attr.region);
				goto out;
			}else
				layer1_osd |= (1 << osg_info[path].stamp[i].attr.region);
		}

		if(osg_info[path].stamp[i].attr.layer == 1){
			if(layer2_osd & (1 << osg_info[path].stamp[i].attr.region)){
				DBG_ERR("stamp[%d] has duplicate layer(2) region(%d)\r\n", i, osg_info[path].stamp[i].attr.region);
				goto out;
			}else
				layer2_osd |= (1 << osg_info[path].stamp[i].attr.region);
		}

		stWinCfg.bEnable          = 1;
		stWinCfg.stGrap.usWidth   = osg_info[path].stamp[i].img.dim.w;
		stWinCfg.stGrap.usHeight  = osg_info[path].stamp[i].img.dim.h;
		stWinCfg.stDisp.ucMode    = 0;
		stWinCfg.stDisp.usXStr    = osg_info[path].stamp[i].attr.position.x;
		stWinCfg.stDisp.usYStr    = osg_info[path].stamp[i].attr.position.y;
		stWinCfg.stDisp.ucBgAlpha = 0;
		stWinCfg.stDisp.ucFgAlpha = ((((osg_info[path].stamp[i].attr.alpha) & 0xF0) >> 4) * 17);
		
		if(osg_info[path].stamp[i].buf.swap){
			stWinCfg.stGrap.uiAddr = osg_info[path].stamp[i].buf.p_addr[0];
		}else{
			stWinCfg.stGrap.uiAddr = osg_info[path].stamp[i].buf.p_addr[1];
		}
		
		if(osg_info[path].stamp[i].img.fmt == OSG_PXLFMT_ARGB1555){
			loff                   = (osg_info[path].stamp[i].img.dim.w*2);
			stWinCfg.stGrap.usLofs = (osg_info[path].stamp[i].img.dim.w>>1);
            stWinCfg.stGrap.ucType = 0;
		}else if(osg_info[path].stamp[i].img.fmt == OSG_PXLFMT_ARGB4444){
			loff                   = (osg_info[path].stamp[i].img.dim.w*2);
			stWinCfg.stGrap.usLofs = (osg_info[path].stamp[i].img.dim.w>>1);
            stWinCfg.stGrap.ucType = 2;
		}else if(osg_info[path].stamp[i].img.fmt == OSG_PXLFMT_ARGB8888){
			loff                   = (osg_info[path].stamp[i].img.dim.w*4);
			stWinCfg.stGrap.usLofs = osg_info[path].stamp[i].img.dim.w;
            stWinCfg.stGrap.ucType = 1;
		}else{
			DBG_ERR("invalid osg->stamp[%d].img.fmt(%x)\n", i, osg_info[path].stamp[i].img.fmt);
			goto out;
		}
		
		if(!stWinCfg.stGrap.uiAddr || !stWinCfg.stGrap.usWidth || !stWinCfg.stGrap.usHeight){
			DBG_ERR("invalid stamp[%d] buffer : addr(%lx) w(%d) h(%d)\r\n", i, (unsigned long)stWinCfg.stGrap.uiAddr, stWinCfg.stGrap.usWidth, stWinCfg.stGrap.usHeight);
			goto out;
		}

		if(H26XE_OSG_ADR_ALIGN && (stWinCfg.stGrap.uiAddr & (H26XE_OSG_ADR_ALIGN - 1))){
			DBG_ERR("encoder osg buf addr should be %d aligned. current buf addr is 0x%lx\n", H26XE_OSG_ADR_ALIGN, (unsigned long)stWinCfg.stGrap.uiAddr);
			goto out;
		}
		if(H26XE_OSG_LOFS_ALIGN && (loff & (H26XE_OSG_LOFS_ALIGN - 1))){
			DBG_ERR("encoder osg lineoffset should be %d aligned. current lineoffset is %d\n", H26XE_OSG_LOFS_ALIGN, loff);
			goto out;
		}
		
		if(H26XE_OSG_WIDTH_ALIGN && (stWinCfg.stGrap.usWidth & (H26XE_OSG_WIDTH_ALIGN - 1))){
			DBG_ERR("encoder osg width should be %d aligned. current width is %d\n", H26XE_OSG_WIDTH_ALIGN, stWinCfg.stGrap.usWidth);
			goto out;
		}
		if(H26XE_OSG_HEIGHT_ALIGN && (stWinCfg.stGrap.usHeight & (H26XE_OSG_HEIGHT_ALIGN - 1))){
			DBG_ERR("encoder osg height should be %d aligned. current height is %d\n", H26XE_OSG_HEIGHT_ALIGN, stWinCfg.stGrap.usHeight);
			goto out;
		}
#if 0
		if(H26XE_OSG_POSITION_X_ALIGN && (stWinCfg.stDisp.usXStr & (H26XE_OSG_POSITION_X_ALIGN - 1))){
			DBG_ERR("encoder osg x coordinate should be %d aligned. current x is %d\n", H26XE_OSG_POSITION_X_ALIGN, stWinCfg.stDisp.usXStr);
			goto out;
		}
		if(H26XE_OSG_POSITION_Y_ALIGN && (stWinCfg.stDisp.usYStr & (H26XE_OSG_POSITION_Y_ALIGN - 1))){
			DBG_ERR("encoder osg y coordinate should be %d aligned. current y is %d\n", H26XE_OSG_POSITION_Y_ALIGN, stWinCfg.stDisp.usYStr);
			goto out;
		}
#endif
		
		if(!h26XEnc_setOsgWinCfg(pVar, ((osg_info[path].stamp[i].attr.layer<<4) + osg_info[path].stamp[i].attr.region), &stWinCfg)){
			DBG_ERR("h26XEnc_setOsgWinCfg(layer=%d;region=%d;en=1) for encoding path(%u) fail\n", i, j, path);
			goto out;
		}
	}

	ret = 0;

out:

	vk_spin_unlock_irqrestore(&BUILTIN_OSG_LOCK, flag);

	return ret;
}

int builtin_osg_setup_jpeg_stamp(KDRV_VDOENC_PARAM *jpeg, UINT32 path)
{
#if 0
	int                     i, ret = -1, x, y;
	UINT32                  loff;
	unsigned long           flag;
	KDRV_GRPH_TRIGGER_PARAM request = {0};
	GRPH_IMG                image_a = {0};
	GRPH_IMG                image_b = {0};
	GRPH_IMG                image_c = {0};
	GRPH_PROPERTY           property[4] = {0};
	BOOL                    is_yuv420; //1: 420, 0: 422
	uintptr_t               yoffset, coffset;

	if(jpeg == NULL){
		DBG_ERR("jpeg of jpeg path(%u) is NULL\n", path);
		return -1;
	}

	if(path >= BUILTIN_VDOENC_PATH_ID_MAX){
		DBG_ERR("invalid jpeg path(%u). max is %u\n", path, BUILTIN_VDOENC_PATH_ID_MAX);
		return -1;
	}

	if(jpeg->in_fmt == KDRV_JPEGYUV_FORMAT_422){ //0: YUV422,1:YUV420
		is_yuv420 = 0;
	}else if(jpeg->in_fmt == KDRV_JPEGYUV_FORMAT_420){
		is_yuv420 = 1;
	}else{
		DBG_ERR("jpeg input format(%x) is not supported\n", jpeg->in_fmt);
		return -1;
	}

	vk_spin_lock_irqsave(&BUILTIN_OSG_LOCK, flag);

	if(osg_info[path].num > BUILTIN_OSG_MAX_NUM){
		DBG_ERR("osg number(%d) of encoding path(%u) > max(%d)\n", osg_info[path].num, path, BUILTIN_OSG_MAX_NUM);
		goto out;
	}

	for(i = 0 ; i < osg_info[path].num ; ++i){

		if(osg_info[path].stamp[i].valid == 0){
			continue;
		}

		if(osg_info[path].stamp[i].attr.position.x < 0 || osg_info[path].stamp[i].img.dim.w < 0){
			DBG_ERR("osg number(%d) of encoding path(%u) has invalid x(%d) and width(%d). x and width must >= 0\n", i, path, osg_info[path].stamp[i].attr.position.x, osg_info[path].stamp[i].img.dim.w);
			goto out;
		}

		if((osg_info[path].stamp[i].attr.position.x + osg_info[path].stamp[i].img.dim.w) > jpeg->encode_width){
			DBG_ERR("osg number(%d) of encoding path(%u) has invalid x(%d) and width(%d). jpeg is only %d pixel wide\n", i, path, osg_info[path].stamp[i].attr.position.x, osg_info[path].stamp[i].img.dim.w, jpeg->encode_width);
			goto out;
		}

		if(osg_info[path].stamp[i].attr.position.y < 0 || osg_info[path].stamp[i].img.dim.h < 0){
			DBG_ERR("osg number(%d) of encoding path(%u) has invalid y(%d) and height(%d). y and height must >= 0\n", i, path, osg_info[path].stamp[i].attr.position.y, osg_info[path].stamp[i].img.dim.h);
			goto out;
		}

		if((osg_info[path].stamp[i].attr.position.y + osg_info[path].stamp[i].img.dim.h) > jpeg->encode_height){
			DBG_ERR("osg number(%d) of encoding path(%u) has invalid y(%d) and height(%d). jpeg is only %d pixel high\n", i, path, osg_info[path].stamp[i].attr.position.y, osg_info[path].stamp[i].img.dim.h, jpeg->encode_height);
			goto out;
		}

		request.command = GRPH_CMD_RGBYUV_BLEND;
		request.p_images = &image_a;
		request.p_property = &property[0];
		request.is_skip_cache_flush = 0;
		request.is_buf_pa = 0;

		if(osg_info[path].stamp[i].img.fmt == OSG_PXLFMT_ARGB1555){
			request.format = GRPH_FORMAT_16BITS_ARGB1555_RGB;
			property[0].id = GRPH_PROPERTY_ID_YUVFMT;
			property[0].property = is_yuv420;
			property[0].p_next = &property[1];
			property[1].id = GRPH_PROPERTY_ID_ALPHA0_INDEX;
			property[1].property = (osg_info[path].stamp[i].attr.alpha) & 0x0F; //Alpha[3..0]
			property[1].p_next = &property[2];
			property[2].id = GRPH_PROPERTY_ID_ALPHA1_INDEX;
			property[2].property = (osg_info[path].stamp[i].attr.alpha >> 4) & 0x0F; //Alpha[7..4]
			if (nvt_get_chip_id() == CHIP_NA51055){
				property[2].p_next = NULL;
			}else{
				property[2].p_next = &property[3];
				property[3].id = GRPH_PROPERTY_ID_UV_SUBSAMPLE;
				property[3].property = GRPH_UV_CENTERED;
				property[3].p_next = NULL;
			}
			loff = osg_info[path].stamp[i].img.dim.w * 2;
		}else if(osg_info[path].stamp[i].img.fmt == OSG_PXLFMT_ARGB4444){
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
			loff = osg_info[path].stamp[i].img.dim.w * 2;
		}else if(osg_info[path].stamp[i].img.fmt == OSG_PXLFMT_ARGB8888){
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
			loff = osg_info[path].stamp[i].img.dim.w * 4;
		}else{
			DBG_ERR("invalid osg->stamp[%d].img.fmt(%x)\n", i, osg_info[path].stamp[i].img.fmt);
			goto out;
		}

		image_a.img_id = GRPH_IMG_ID_A; //src argb
		if(osg_info[path].stamp[i].buf.swap){
			image_a.dram_addr = osg_info[path].stamp[i].buf.p_addr[0];
		}else{
			image_a.dram_addr = osg_info[path].stamp[i].buf.p_addr[1];
		}
		image_a.lineoffset = loff;
		image_a.width = loff;
		image_a.height = osg_info[path].stamp[i].img.dim.h;
		image_a.p_next = &image_b;

		x = (osg_info[path].stamp[i].attr.position.x & ~0x01);
		if(is_yuv420){
			y = (osg_info[path].stamp[i].attr.position.y & ~0x01);
		}else{
			y = osg_info[path].stamp[i].attr.position.y;
		}
		yoffset = jpeg->y_line_offset * y + x;
		coffset = jpeg->c_line_offset * (y >> 1) + x;

		image_b.img_id = GRPH_IMG_ID_B; //dst Y
		image_b.dram_addr = jpeg->y_addr + yoffset;
		image_b.lineoffset = jpeg->y_line_offset;
		image_b.p_next = &image_c;

		image_c.img_id = GRPH_IMG_ID_C; //dst UV
		image_c.dram_addr = jpeg->c_addr + coffset;
		image_c.lineoffset = jpeg->c_line_offset;
		image_c.p_next = NULL;

		vk_spin_unlock_irqrestore(&BUILTIN_OSG_LOCK, flag);

		if (E_OK != kdrv_grph_open(KDRV_CHIP0, KDRV_GFX2D_GRPH0)) {
			DBG_ERR("kdrv_grph_open\r\n");
			vk_spin_lock_irqsave(&BUILTIN_OSG_LOCK, flag);
			goto out;
		}

		if (E_OK != kdrv_grph_trigger(KDRV_DEV_ID(KDRV_CHIP0, KDRV_GFX2D_GRPH0, 0), &request, NULL, NULL)) {
			DBG_ERR("kdrv_grph_trigger\r\n");
			vk_spin_lock_irqsave(&BUILTIN_OSG_LOCK, flag);
			goto out;
		}

		if (E_OK != kdrv_grph_close(KDRV_CHIP0, KDRV_GFX2D_GRPH0)) {
			DBG_ERR("kdrv_grph_close\r\n");
			vk_spin_lock_irqsave(&BUILTIN_OSG_LOCK, flag);
			goto out;
		}

		vk_spin_lock_irqsave(&BUILTIN_OSG_LOCK, flag);
	}

	ret = 0;

out:

	vk_spin_unlock_irqrestore(&BUILTIN_OSG_LOCK, flag);

	return ret;
#endif

	return 0;
}
#else
int osg_module_init(void)
{
	return 0;
}
#endif
