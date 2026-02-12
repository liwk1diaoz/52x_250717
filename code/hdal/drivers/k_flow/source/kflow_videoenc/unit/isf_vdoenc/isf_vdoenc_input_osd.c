#include "isf_vdoenc_internal.h"
#include "video_encode.h"
#include "nmediarec_api.h"
#include "kflow_videoenc/isf_vdoenc.h"
#include "videosprite/videosprite_open.h"
#include "videosprite/videosprite_enc.h"
#include "kdrv_videoenc/kdrv_videoenc.h"
#include "kdrv_videoenc/kdrv_videoenc_lmt.h"
#include "comm/hwclock.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__    	  isf_vdoenc_io
#define __DBGLVL__		  8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoenc_io_debug_level = NVT_DBG_WRN;
///////////////////////////////////////////////////////////////////////////////
static int vds_render_enc_coe(int vid, VDS_INTERNAL_COE_STAMP_MASK *stamp, UINT32 w, UINT32 h, UINT32 *palette)
{
	int                 i, j, ret;
	unsigned int        layer1_osd, layer2_osd, loff = 0;
	KDRV_VDOENC_OSG_WIN config;
	UINT32              kdrv_id     = ((KDRV_VIDEOCDC_ENGINE0<<KDRV_ENGINE_OFFSET) | (vid));
	//init CST
	KDRV_VDOENC_OSG_RGB CST_Setting = {{{0x26, 0x4b, 0x0f},{0xea, 0xd5, 0x41},{0x41, 0xc9, 0xf5}}};
	KDRV_VDOENC_OSG_PAL pal         = { 0 };
	
	ret = kdrv_videoenc_set(kdrv_id, VDOENC_SET_OSG_RGB, &CST_Setting); 
	if(ret){		
		DBG_ERR("fail to set CST_Setting ret: %d\n", ret);		
		return ret; 
	}
	
	if(palette){
		for(i = 0 ; i < 16 ; ++i){
			pal.idx   = i;
			pal.alpha = ((palette[i] & 0xFF000000) >> 24);
			pal.red   = ((palette[i] & 0x00FF0000) >> 16);
			pal.green = ((palette[i] & 0x0000FF00) >> 8);
			pal.blue  = ((palette[i] & 0x000000FF));
			ret = kdrv_videoenc_set(kdrv_id, VDOENC_SET_OSG_PAL, &pal); 
			if(ret){		
				DBG_ERR("fail to set %dth palette : %d\n", i, ret);		
				return ret; 
			}
		}
	}
	
	vds_memset(&config, 0, sizeof(KDRV_VDOENC_OSG_WIN));
	for(i = 0 ; i < 2 ; ++i){
		for(j = 0 ; j < 16 ; ++j){
			config.layer_idx = i;
			config.win_idx   = j;
			config.enable    = 0;
			ret = kdrv_videoenc_set(kdrv_id, VDOENC_SET_OSG_WIN, &config);
			if(ret){
				DBG_ERR("fail to disable osd of layer(%d) win(%d)\n", i, j);
				return ret;
			}
		}
	}
	
	if(!stamp || !w || !h)
		return 0;

	layer1_osd = 0;
	layer2_osd = 0;
	for(i = 0 ; i < vds_max_coe_stamp ; ++i){
	
		if(!stamp[i].en)
			continue;
			
		if(stamp[i].is_mask){
			//coverity[unsigned_compare]
			if(stamp[i].data.mask.layer < 0 || stamp[i].data.mask.layer > 1){
				DBG_ERR("invalid mask[%d] layer(%d)\r\n", i, stamp[i].data.mask.layer);
				return E_OBJ;
			}
				
			//coverity[unsigned_compare]
			if(stamp[i].data.mask.region < 0 || stamp[i].data.mask.region > 15){
				DBG_ERR("invalid mask[%d] region(%d)\r\n", i, stamp[i].data.mask.region);
				return E_OBJ;
			}
				
			if(!stamp[i].data.mask.w || !stamp[i].data.mask.h){
				DBG_ERR("invalid mask[%d] w(%d) h(%d)\r\n", i, stamp[i].data.mask.w, stamp[i].data.mask.h);
				return E_OBJ;
			}
			
			if(stamp[i].data.mask.layer == 0){
				if(layer1_osd & (1 << stamp[i].data.mask.region)){
					DBG_ERR("mask[%d] has duplicate layer(1) region(%d)\r\n", i, stamp[i].data.mask.region);
					return E_PAR;
				}else
					layer1_osd |= (1 << stamp[i].data.mask.region);
			}
			
			if(stamp[i].data.mask.layer == 1){
				if(layer2_osd & (1 << stamp[i].data.mask.region)){
					DBG_ERR("mask[%d] has duplicate layer(2) region(%d)\r\n", i, stamp[i].data.mask.region);
					return E_PAR;
				}else
					layer2_osd |= (1 << stamp[i].data.mask.region);
			}
			
			vds_memset(&config, 0, sizeof(KDRV_VDOENC_OSG_WIN));
			config.layer_idx         = stamp[i].data.mask.layer;
			config.win_idx           = stamp[i].data.mask.region;
			config.enable            = 1;
			config.st_grap.width     = stamp[i].data.mask.w;
			config.st_grap.height    = stamp[i].data.mask.h;
			config.st_disp.mode      = 1;
			config.st_disp.str_x     = stamp[i].data.mask.x;
			config.st_disp.str_y     = stamp[i].data.mask.y;
			config.st_disp.bg_alpha  = 0;
			config.st_disp.fg_alpha  = stamp[i].data.mask.alpha;
			config.st_disp.mask_y[0] = (stamp[i].data.mask.color & 0x0FF);
			config.st_disp.mask_y[1] = (stamp[i].data.mask.color & 0x0FF);
			config.st_disp.mask_cb   = ((stamp[i].data.mask.color & 0x0FF00)>>8);
			config.st_disp.mask_cr   = ((stamp[i].data.mask.color & 0x0FF0000)>>16);
			if(stamp[i].data.mask.type == VDS_INTERNAL_MASK_TYPE_HOLLOW)
				config.st_disp.mask_type = 1;
			else
				config.st_disp.mask_type = 0;
			if(stamp[i].data.mask.thickness <= 2)
				config.st_disp.mask_bd_size = 0;
			else if(stamp[i].data.mask.thickness <= 4)
				config.st_disp.mask_bd_size = 1;
			else if(stamp[i].data.mask.thickness <= 6)
				config.st_disp.mask_bd_size = 2;
			else
				config.st_disp.mask_bd_size = 3;
		}else{
			//coverity[unsigned_compare]
			if(stamp[i].data.stamp.layer < 0 || stamp[i].data.stamp.layer > 1){
				DBG_ERR("invalid stamp[%d] layer(%d)\r\n", i, stamp[i].data.stamp.layer);
				return E_OBJ;
			}
				
			//coverity[unsigned_compare]
			if(stamp[i].data.stamp.region < 0 || stamp[i].data.stamp.region > 15){
				DBG_ERR("invalid stamp[%d] region(%d)\r\n", i, stamp[i].data.stamp.region);
				return E_OBJ;
			}
				
			if(stamp[i].data.stamp.fmt != VDO_PXLFMT_RGB565   && 
			   stamp[i].data.stamp.fmt != VDO_PXLFMT_ARGB1555 && 
			   stamp[i].data.stamp.fmt != VDO_PXLFMT_ARGB4444 &&
			   stamp[i].data.stamp.fmt != VDO_PXLFMT_ARGB8888 &&
			   stamp[i].data.stamp.fmt != VDO_PXLFMT_I1       &&
			   stamp[i].data.stamp.fmt != VDO_PXLFMT_I2       &&
			   stamp[i].data.stamp.fmt != VDO_PXLFMT_I4){
				DBG_ERR("invalid stamp[%d] format(%d)\r\n", i, stamp[i].data.stamp.fmt);
				return E_OBJ;
			}
				
			if(!stamp[i].data.stamp.addr || !stamp[i].data.stamp.size.w || !stamp[i].data.stamp.size.h){
				DBG_ERR("invalid stamp[%d] buffer : addr(%p) w(%d) h(%d)\r\n", i, (void*)stamp[i].data.stamp.addr, stamp[i].data.stamp.size.w, stamp[i].data.stamp.size.h);
				return E_OBJ;
			}
			
			if(stamp[i].data.stamp.layer == 0){
				if(layer1_osd & (1 << stamp[i].data.stamp.region)){
					DBG_ERR("stamp[%d] has duplicate layer(1) region(%d)\r\n", i, stamp[i].data.stamp.region);
					return E_PAR;
				}else
					layer1_osd |= (1 << stamp[i].data.stamp.region);
			}
			
			if(stamp[i].data.stamp.layer == 1){
				if(layer2_osd & (1 << stamp[i].data.stamp.region)){
					DBG_ERR("stamp[%d] has duplicate layer(2) region(%d)\r\n", i, stamp[i].data.stamp.region);
					return E_PAR;
				}else
					layer2_osd |= (1 << stamp[i].data.stamp.region);
			}
			
			vds_memset(&config, 0, sizeof(KDRV_VDOENC_OSG_WIN));
			config.layer_idx        = stamp[i].data.stamp.layer;
			config.win_idx          = stamp[i].data.stamp.region;
			config.enable           = 1;
			config.st_grap.width    = stamp[i].data.stamp.size.w;
			config.st_grap.height   = stamp[i].data.stamp.size.h;
			config.st_grap.addr     = stamp[i].data.stamp.addr;
			config.st_disp.mode     = 0;
			config.st_disp.str_x    = stamp[i].data.stamp.pos.x;
			config.st_disp.str_y    = stamp[i].data.stamp.pos.y;
			config.st_disp.bg_alpha = ((((stamp[i].data.stamp.bg_alpha) & 0xF0) >> 4) * 17);
			config.st_disp.fg_alpha = ((((stamp[i].data.stamp.fg_alpha) & 0xF0) >> 4) * 17);
			config.st_key.enable    = stamp[i].data.stamp.ckey_en;
			
			if(stamp[i].data.stamp.fmt == VDO_PXLFMT_RGB565){
				loff                       = (stamp[i].data.stamp.size.w*2);
				config.st_grap.line_offset = (stamp[i].data.stamp.size.w>>1);
				config.st_grap.type        = 3;
			}else if(stamp[i].data.stamp.fmt == VDO_PXLFMT_ARGB1555){
				loff                       = (stamp[i].data.stamp.size.w*2);
				config.st_grap.line_offset = (stamp[i].data.stamp.size.w>>1);
				config.st_grap.type        = 0;
				if(config.st_key.enable){
					config.st_key.alpha_en = 1;
					config.st_key.alpha    = ((stamp[i].data.stamp.ckey_val & 0x8000)>>8);
					config.st_key.red      = ((stamp[i].data.stamp.ckey_val & 0x7C00)>>7);
					config.st_key.green    = ((stamp[i].data.stamp.ckey_val & 0x03E0)>>2);
					config.st_key.blue     = ((stamp[i].data.stamp.ckey_val & 0x001F)<<3);
				}
			}else if(stamp[i].data.stamp.fmt == VDO_PXLFMT_ARGB4444){
				loff                       = (stamp[i].data.stamp.size.w*2);
				config.st_grap.line_offset = (stamp[i].data.stamp.size.w>>1);
				config.st_grap.type        = 2;
				if(config.st_key.enable){
					config.st_key.alpha_en = 1;
					config.st_key.alpha    = ((stamp[i].data.stamp.ckey_val & 0xF000)>>8);
					config.st_key.red      = ((stamp[i].data.stamp.ckey_val & 0x0F00)>>4);
					config.st_key.green    = ((stamp[i].data.stamp.ckey_val & 0x00F0));
					config.st_key.blue     = ((stamp[i].data.stamp.ckey_val & 0x000F)<<4);
				}
			}else if(stamp[i].data.stamp.fmt == VDO_PXLFMT_ARGB8888){
				loff                       = (stamp[i].data.stamp.size.w*4);
				config.st_grap.line_offset = (stamp[i].data.stamp.size.w);
				config.st_grap.type        = 1;
				if(config.st_key.enable){
					config.st_key.alpha_en = 1;
					config.st_key.alpha    = ((stamp[i].data.stamp.ckey_val & 0xFF000000)>>24);
					config.st_key.red      = ((stamp[i].data.stamp.ckey_val & 0x00FF0000)>>16);
					config.st_key.green    = ((stamp[i].data.stamp.ckey_val & 0x0000FF00)>>8);
					config.st_key.blue     = ((stamp[i].data.stamp.ckey_val & 0x000000FF));
				}
			}else if(stamp[i].data.stamp.fmt == VDO_PXLFMT_I1){
				loff                       = (stamp[i].data.stamp.size.w>>3);
				config.st_grap.line_offset = (stamp[i].data.stamp.size.w>>5);
				config.st_grap.type        = 4;
			}else if(stamp[i].data.stamp.fmt == VDO_PXLFMT_I2){
				loff                       = (stamp[i].data.stamp.size.w>>2);
				config.st_grap.line_offset = (stamp[i].data.stamp.size.w>>4);
				config.st_grap.type        = 5;
			}else if(stamp[i].data.stamp.fmt == VDO_PXLFMT_I4){
				loff                       = (stamp[i].data.stamp.size.w>>1);
				config.st_grap.line_offset = (stamp[i].data.stamp.size.w>>3);
				config.st_grap.type        = 6;
			}else{
				DBG_ERR("unknown coe format(%d)\r\n", stamp[i].data.stamp.fmt);
			}

			//limit check : buffer
			if(H26XE_OSG_ADR_ALIGN && (config.st_grap.addr & (H26XE_OSG_ADR_ALIGN - 1))){
				DBG_ERR("encoder osg buf addr should be %d aligned. current buf addr is 0x%x\n",
							H26XE_OSG_ADR_ALIGN, config.st_grap.addr);
				return E_PAR;
			}
			if(H26XE_OSG_LOFS_ALIGN && (loff & (H26XE_OSG_LOFS_ALIGN - 1))){
				DBG_ERR("encoder osg lineoffset should be %d aligned. current lineoffset is %d\n",
							H26XE_OSG_LOFS_ALIGN, loff);
				return E_PAR;
			}

			//set qp
			if(stamp[i].data.stamp.qp_en) {
				if (stamp[i].data.stamp.qp_fix){
					config.st_qp_map.qp_mode = 3;
				}else{
					config.st_qp_map.qp_mode = 0;
				}
				config.st_qp_map.qp = stamp[i].data.stamp.qp_val;
			} else {
				config.st_qp_map.qp_mode = 0;
				config.st_qp_map.qp = 0;
			}
			
			config.st_qp_map.lpm_mode = stamp[i].data.stamp.qp_lpm_mode;
			config.st_qp_map.tnr_mode = stamp[i].data.stamp.qp_tnr_mode;
			config.st_qp_map.fro_mode = stamp[i].data.stamp.qp_fro_mode;

			//set gcac
			config.st_gcac.enable          = stamp[i].data.stamp.gcac_enable;
			config.st_gcac.blk_width       = stamp[i].data.stamp.gcac_blk_width;       //32;
			config.st_gcac.blk_height      = stamp[i].data.stamp.gcac_blk_height;      //32;
			config.st_gcac.blk_num         = stamp[i].data.stamp.gcac_blk_num;         //stamp[i].size.w/32;
			config.st_gcac.org_color_level = stamp[i].data.stamp.gcac_org_color_level; //5;
			config.st_gcac.inv_color_level = stamp[i].data.stamp.gcac_inv_color_level; //5;
			config.st_gcac.nor_diff_th     = stamp[i].data.stamp.gcac_nor_diff_th;     //150;//0~255
			config.st_gcac.inv_diff_th     = stamp[i].data.stamp.gcac_inv_diff_th;     //150;//0~255
			config.st_gcac.sta_only_mode   = stamp[i].data.stamp.gcac_sta_only_mode;   //0; //Normal processing
			config.st_gcac.full_eval_mode  = stamp[i].data.stamp.gcac_full_eval_mode;  //0;
			config.st_gcac.eval_lum_targ   = stamp[i].data.stamp.gcac_eval_lum_targ;   //0;
		}

		//limit check : w h
		if(H26XE_OSG_WIDTH_ALIGN && (config.st_grap.width & (H26XE_OSG_WIDTH_ALIGN - 1))){
			DBG_ERR("encoder osg width should be %d aligned. current width is %d\n",
						H26XE_OSG_WIDTH_ALIGN, config.st_grap.width);
			return E_PAR;
		}
		if(H26XE_OSG_HEIGHT_ALIGN && (config.st_grap.height & (H26XE_OSG_HEIGHT_ALIGN - 1))){
			DBG_ERR("encoder osg height should be %d aligned. current height is %d\n",
						H26XE_OSG_HEIGHT_ALIGN, config.st_grap.height);
			return E_PAR;
		}
		//limit check : x y
		if(H26XE_OSG_POSITION_X_ALIGN && (config.st_disp.str_x & (H26XE_OSG_POSITION_X_ALIGN - 1))){
			DBG_ERR("encoder osg x coordinate should be %d aligned. current x is %d\n",
						H26XE_OSG_POSITION_X_ALIGN, config.st_disp.str_x);
			return E_PAR;
		}
		if(H26XE_OSG_POSITION_Y_ALIGN && (config.st_disp.str_y & (H26XE_OSG_POSITION_Y_ALIGN - 1))){
			DBG_ERR("encoder osg y coordinate should be %d aligned. current y is %d\n",
						H26XE_OSG_POSITION_Y_ALIGN, config.st_disp.str_y);
			return E_PAR;
		}

		ret = kdrv_videoenc_set(kdrv_id, VDOENC_SET_OSG_WIN, &config);
		if(ret){
			DBG_ERR("fail to enable enc osd(%d)\n", i);
			return ret;
		}
	}
	
	return E_OK;
}

void _isf_vdoenc_do_input_osd(ISF_UNIT *p_thisunit, UINT32 iport, void *in, void *out, BOOL b_before_rotate)
{
	MP_VDOENC_YUV_SRC *p_in = (MP_VDOENC_YUV_SRC *)in;
	UINT32 oport = iport - ISF_IN_BASE + ISF_OUT_BASE;
	UINT32 codec_type;
	VDS_TO_ENC_EXT_STAMP *p_vs_enc;
	VDS_TO_ENC_COE_STAMP *p_vs_coe;
	UINT32 loff[2];
	UINT64 start = hwclock_get_longcounter();
	
	p_in->uiVidPathID = oport;
	codec_type = p_thisunit->do_getportparam(p_thisunit, oport, VDOENC_PARAM_CODEC);

	// do lock
	DBG_IND("[Lock][%8s][Port=%d][Codec=%d]\r\n", p_thisunit->unit_name, iport, codec_type);

	// stamp (Graph or COE)
	DBG_IND("VidPathID = %d, uiYAddr = %08x, uiCbAddr = %08x, uiCrAddr = %08x, uiYLineOffset = %d, uiWidth = %d, uiHeight = %d\r\n",
		p_in->uiVidPathID, p_in->uiYAddr, p_in->uiCbAddr, p_in->uiCrAddr, p_in->uiYLineOffset, p_in->uiWidth, p_in->uiHeight);

	//encoder and "jpeg + before rotation" should render ext stamp
	if (codec_type != NMEDIAREC_ENC_MJPG || b_before_rotate) {
		p_vs_enc = vds_lock_context(VDS_QS_ENC_EXT_STAMP, p_in->uiVidPathID, NULL);
		if(p_vs_enc && p_vs_enc->dirty) {
			loff[0] = p_in->uiYLineOffset;
			loff[1] = p_in->uiUVLineOffset;
			vds_render_enc_ext(VDS_QS_ENC_EXT_STAMP, p_in->uiYAddr, p_in->uiCbAddr, 
								p_vs_enc, p_in->uiWidth, p_in->uiHeight, vds_get_coe_palette(p_in->uiVidPathID), loff);
			p_vs_enc->dirty = 0;
		}
		vds_save_latency(VDS_QS_ENC_EXT_STAMP, p_in->uiVidPathID, 0, hwclock_get_longcounter() - start);
		vds_unlock_context(VDS_QS_ENC_EXT_STAMP, p_in->uiVidPathID, 0);
	}

	//encoder and "jpeg + after rotation" should render built-in stamp
	if (codec_type == NMEDIAREC_ENC_MJPG) {
		// JPG
		if(!b_before_rotate){
			p_vs_coe = vds_lock_context(VDS_QS_ENC_JPG_STAMP, p_in->uiVidPathID, NULL);
			if(p_vs_coe && p_vs_coe->dirty) {
				loff[0] = p_in->uiYLineOffset;
				loff[1] = p_in->uiUVLineOffset;
				vds_render_coe_grh(p_in->uiYAddr, p_in->uiCbAddr, p_vs_coe->stamp,
									p_in->uiWidth, p_in->uiHeight, vds_get_coe_palette(p_in->uiVidPathID), loff);
				p_vs_coe->dirty = 0;
			}
			vds_save_latency(VDS_QS_ENC_JPG_STAMP, p_in->uiVidPathID, 0, hwclock_get_longcounter() - start);
			vds_unlock_context(VDS_QS_ENC_JPG_STAMP, 0, 0);
		}
	} else {
		// H264/H265
		p_vs_coe = vds_lock_context(VDS_QS_ENC_COE_STAMP, p_in->uiVidPathID, NULL);
		if(p_vs_coe) {
			vds_render_enc_coe(p_in->uiVidPathID, p_vs_coe->stamp, p_in->uiWidth, 
								p_in->uiHeight, vds_get_coe_palette(p_in->uiVidPathID));
			p_vs_coe->dirty = 0;
			vds_save_latency(VDS_QS_ENC_COE_STAMP, p_in->uiVidPathID, 0, hwclock_get_longcounter() - start);
		}
	}
}

void _isf_vdoenc_finish_input_osd(ISF_UNIT *p_thisunit, UINT32 iport)
{
	UINT32 oport = iport - ISF_IN_BASE + ISF_OUT_BASE;
	UINT32 codec_type = p_thisunit->do_getportparam(p_thisunit, oport, VDOENC_PARAM_CODEC);
	// do unlock
	DBG_IND("[Unlock][%8s][Port=%d]\r\n", p_thisunit->unit_name, iport);
	
	if (codec_type == NMEDIAREC_ENC_MJPG){
		//vds_unlock_context(VDS_QS_ENC_JPG_STAMP, 0, 0);
	} else {
		vds_unlock_context(VDS_QS_ENC_COE_STAMP, 0, 0);
	}
}


