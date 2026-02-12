#if defined(__LINUX)
#include <linux/delay.h>
#endif

#include "kdrv_videoenc/kdrv_videoenc.h"
#include "kdrv_videodec/kdrv_videodec.h"

#include "kdrv_vdocdc_dbg.h"

#include "h265enc_api.h"
#include "h26xenc_api.h"
#include "h26x_common.h"
#include "h26x.h"

#include "sim_vdocdc_mem.h"
#include "sim_vdocdc_file.h"
#include "sim_hevc_main.h"

static bool b_get_vdo_cb;

static INT32 kdrv_vdo_cb(void *info, void *usr_data)
{
	DBG_ERR("%s [%d] finish\r\n", (char *)info, (int)(*(UINT32 *)usr_data));
	b_get_vdo_cb = true;
	return 0;
}

static void gdr_test(UINT32 kdrv_id)
{
	KDRV_VDOENC_GDR gdr;

	gdr.enable = TRUE;
	gdr.number = 3;
	gdr.period = 10;

	if (kdrv_videoenc_set(kdrv_id, VDOENC_SET_GDR, &gdr) != 0)
		DBG_ERR("gdr error\r\n");	
}

static void rc_test(UINT32 kdrv_id, UINT32 width, UINT32 height)
{	
	UINT32 rc_set_mode = VDOENC_SET_CBR;

	if (rc_set_mode == VDOENC_SET_CBR) {
		KDRV_VDOENC_CBR cbr = {0};

        cbr.enable = 1;
		cbr.init_i_qp = 30;
		cbr.min_i_qp = 20;
		cbr.max_i_qp = 45;
		cbr.init_p_qp = 30;
		cbr.min_p_qp = 20;
		cbr.max_p_qp = 45;
		cbr.byte_rate = 65536;
		cbr.frame_rate = 30000;
		cbr.gop = 30;
		cbr.static_time = 1;
		cbr.ip_weight = 0;		

		if (kdrv_videoenc_set(kdrv_id, rc_set_mode, &cbr) != 0)
			DBG_ERR("set rc error\r\n");
	}	
	
}

static void rrc_test(UINT32 kdrv_id)
{
	KDRV_VDOENC_ROW_RC rrc_info = {0};

	rrc_info.enable = TRUE;
	rrc_info.i_qp_range = 2;
	rrc_info.i_qp_step = 1;
	rrc_info.p_qp_range = 4;
	rrc_info.p_qp_step = 1;
	rrc_info.min_i_qp = 15;
	rrc_info.max_i_qp = 45;
	rrc_info.min_p_qp = 15;
	rrc_info.max_p_qp =45;

	if (kdrv_videoenc_set(kdrv_id, VDOENC_SET_ROWRC, &rrc_info) != 0)
		DBG_ERR("set rrc error\r\n");	
}

static void roi_test(UINT32 kdrv_id)
{
	KDRV_VDOENC_ROI roi_info = {0};

	roi_info.st_roi[2].enable = 1;
	roi_info.st_roi[2].coord_X = 32;
	roi_info.st_roi[2].coord_Y = 32;
	roi_info.st_roi[2].width = 16;
	roi_info.st_roi[2].height = 48;
	roi_info.st_roi[2].qp = 15;
	roi_info.st_roi[2].qp_mode = 3;

	roi_info.st_roi[4].enable = 1;
	roi_info.st_roi[4].coord_X = 64;
	roi_info.st_roi[4].coord_Y = 64;
	roi_info.st_roi[4].width = 32;
	roi_info.st_roi[4].height = 16;
	roi_info.st_roi[4].qp = 5;
	roi_info.st_roi[4].qp_mode = 0;

	if (kdrv_videoenc_set(kdrv_id, VDOENC_SET_ROI, &roi_info) != 0)
		DBG_ERR("set roi error\r\n");
}

static void usr_qp_test(UINT32 kdrv_id, UINT32 width, UINT32 height){	
	UINT32 mb_w = SIZE_64X(width)/16;	
	UINT32 mb_h = SIZE_64X(height)/16;	
	UINT32 loft = (SIZE_64X(width)/16)*sizeof(UINT16);
	UINT32 w, h;	
	UINT16 *addr;	
	void *hdl_qpmap = NULL;	

	KDRV_VDOENC_USR_QP usr_qp_info = {0};
	
	struct nvt_fmem_mem_info_t info = {0};	

	if ((hdl_qpmap = vdocdc_get_mem(&info, (mb_w*mb_h*2))) == NULL) {		
		nvt_dbg(ERR, "get qpmap error\r\n");		
	}	
	
	addr = (UINT16 *)info.vaddr;		
	for (h = 0; h < mb_h; h++) {		
		for (w = 0; w < mb_w; w++) {		
			if (h < 2)
				*(addr + h*mb_w + w) = (3<<6 | 10);		// fix qp			
			else if (h < 4)
				*(addr + h*mb_w + w) = (10);			// delta qp
			else if (h < 6)
				*(addr + h*mb_w + w) = (3<<6 | 20);		// fix qp
			else
				*(addr + h*mb_w + w) = (((UINT8)(-5))&0x3f);			// delta qp
		}
	}		

	usr_qp_info.enable = TRUE;
	usr_qp_info.qp_map_addr = (UINT8 *)addr;
	usr_qp_info.qp_map_size = (mb_w*mb_h*2);
	usr_qp_info.qp_map_loft = loft;
	
	if (kdrv_videoenc_set(kdrv_id, VDOENC_SET_QPMAP, &usr_qp_info) != 0)
		DBG_DUMP("set roi error\r\n");
	
	if (hdl_qpmap != NULL) vdocdc_free_mem(hdl_qpmap);
}

static void slice_test(UINT32 kdrv_id, UINT32 height){	
	KDRV_VDOENC_SLICE_SPLIT slice_info = {0};

    slice_info.enable = 1;
    slice_info.slice_row_num = height / 64 / 2;

	if (kdrv_videoenc_set(kdrv_id, VDOENC_SET_SLICESPLIT, &slice_info) != 0)
		DBG_DUMP("set slice error\r\n");
}

static void rdo_test(UINT32 kdrv_id){
	KDRV_VDOENC_RDO rdo_info = {0};

	rdo_info.rdo_codec = VDOENC_RDO_CODEC_265;

	rdo_info.rdo_param.rdo_265.hevc_intra_32x32_cost_bias = 15;
	rdo_info.rdo_param.rdo_265.hevc_intra_16x16_cost_bias = 15;
	rdo_info.rdo_param.rdo_265.hevc_intra_8x8_cost_bias = 15;
	rdo_info.rdo_param.rdo_265.hevc_inter_skip_cost_bias = -16;
	rdo_info.rdo_param.rdo_265.hevc_inter_merge_cost_bias = -16;
	rdo_info.rdo_param.rdo_265.hevc_inter_64x64_cost_bias = 31;
	rdo_info.rdo_param.rdo_265.hevc_inter_64x32_32x64_cost_bias = 31;
	rdo_info.rdo_param.rdo_265.hevc_inter_32x32_cost_bias = 31;
	rdo_info.rdo_param.rdo_265.hevc_inter_32x16_16x32_cost_bias = 31;
	rdo_info.rdo_param.rdo_265.hevc_inter_16x16_cost_bias = 31;

	if (kdrv_videoenc_set(kdrv_id, VDOENC_SET_RDO, &rdo_info) != 0)
		DBG_DUMP("set rdo error\r\n");
}

void sim_hevc_enc_main(UINT32 width, UINT32 height, char *string)
{
	UINT32 kdrv_id = ((KDRV_VIDEOCDC_ENGINE0<<KDRV_ENGINE_OFFSET) | (KDRV_VDOENC_ID_1));
	void *hdl_mem = NULL;
	void *hdl_src = NULL;
	UINT32 frm_size = width*height;
	UINT32 buf_addr, buf_size;		
	UINT32 i;
	KDRV_VDOENC_SVC svc_layer = KDRV_VDOENC_SVC_DISABLE;
	UINT32 ltr_interval = 0;
	UINT32 rotate = 0;	// 0 : no rotate , 1 : counterclockwise rotate , 2 : clockwise rotate //
	UINT32 tile_mode_en = 0;
	UINT32 sde_en = 0;
	UINT32 d2d = 0;
	UINT32 gdc = 0;

	KDRV_VDOENC_PARAM  enc_obj;
	KDRV_CALLBACK_FUNC callback;
	struct nvt_fmem_mem_info_t info = {0};
	
	H26XFile fd_src, fd_bs, fd_bslen, fd_src_out;

	BOOL bTestGdrEn = FALSE;
	BOOL bTestRcEn  = FALSE;
	BOOL bTestRowRc = FALSE;
	BOOL bTestRoi = FALSE;
	BOOL bTestUsrQp = FALSE;
	BOOL bTestSlice = FALSE;
	BOOL bTestRdo = FALSE;

	h26xFileOpen(&fd_src, string, FST_OPEN_READ);
	#if defined(__LINUX)
	h26xFileOpen(&fd_bs, "/mnt/sd/bs.265", FST_OPEN_WRITE | FST_OPEN_ALWAYS);
	h26xFileOpen(&fd_bslen, "/mnt/sd/bslen_265.dat", FST_OPEN_WRITE | FST_OPEN_ALWAYS);	
	h26xFileOpen(&fd_src_out, "/mnt/sd/src.pak", FST_OPEN_WRITE | FST_OPEN_ALWAYS);
	#else
	h26xFileOpen(&fd_bs, "A:\\bs.265", FST_OPEN_WRITE | FST_OPEN_ALWAYS);
	h26xFileOpen(&fd_bslen, "A:\\bslen_265.dat", FST_OPEN_WRITE | FST_OPEN_ALWAYS);	
	h26xFileOpen(&fd_src_out, "A:\\src.pak", FST_OPEN_WRITE | FST_OPEN_ALWAYS);
	#endif

#ifdef VDOCDC_LL
	{
		void *hdl_ll_mem = NULL;
		KDRV_VDOENC_LL_MEM_INFO mem_info = {0};
		KDRV_VDOENC_LL_MEM mem = {0};
		
		kdrv_videoenc_get(0, VDOENC_GET_LL_MEM_SIZE, &mem_info);
	
		if (mem_info.size != 0) {
			if ((hdl_ll_mem = vdocdc_get_mem(&info, mem_info.size)) == NULL) {
				nvt_dbg(ERR, "get_mem error\r\n");
			}
	
			mem.addr = (UINT32)info.vaddr;
			mem.size = (UINT32)info.size;
	
			kdrv_videoenc_set(0, VDOENC_SET_LL_MEM, &mem);
		}
	}
#endif

	// init enc_obj //
	{				
		//struct nvt_fmem_mem_info_t info = {0};

		UINT32 addr;
		UINT32 size = frm_size*6 + 512;

		if ((hdl_src = vdocdc_get_mem(&info, size)) == NULL) {
			nvt_dbg(ERR, "get_mem error\r\n");	
		}

		addr = (UINT32)info.vaddr;
		memset((void *)addr, 0, size);
		h26x_cache_clean(addr, size);
		memset(&enc_obj, 0, sizeof(KDRV_VDOENC_PARAM));		
		
		enc_obj.y_addr = (UINT32)addr;	addr += frm_size;
		enc_obj.c_addr = (UINT32)addr;	addr += (frm_size/2);
		enc_obj.y_line_offset = width;
		enc_obj.c_line_offset = width;
		enc_obj.src_out_en = 0;
		enc_obj.src_out_y_addr = addr;	addr += (SIZE_64X(width)*SIZE_64X(height));
		enc_obj.src_out_c_addr = addr;	addr += ((SIZE_64X(width)*SIZE_64X(height))/2);
		enc_obj.src_out_y_line_offset = SIZE_64X(width);
		enc_obj.src_out_c_line_offset = SIZE_64X(width);
		enc_obj.bs_start_addr = addr; 	addr += (frm_size*4);
		enc_obj.bs_end_addr = addr;	
		enc_obj.nalu_len_addr = addr;
        if (sde_en) {
            enc_obj.st_sdc.enable = sde_en;
            enc_obj.st_sdc.width = width;
            enc_obj.st_sdc.height = height;
            enc_obj.st_sdc.y_lofst = SIZE_16X(width)*3/4;
            enc_obj.st_sdc.c_lofst = SIZE_32X(width)*3/4;
        }
		
		callback.callback = kdrv_vdo_cb;
	}		
	
	{
		KDRV_VDOENC_TYPE codec_type = VDOENC_TYPE_H265;

		if (kdrv_videoenc_set(kdrv_id, VDOENC_SET_CODEC, &codec_type) != 0)
			nvt_dbg(INFO, "set codec type error\r\n");
	}
	
	{
		//struct nvt_fmem_mem_info_t info = {0};	
		KDRV_VDOENC_MEM_INFO mem_info = {0};
		
		mem_info.width = SIZE_64X(((rotate == 1 || rotate == 2) ? height : width));
		mem_info.height = SIZE_64X(((rotate == 1 || rotate == 2) ? width : height));
		DBG_ERR("%d, %d, %d, %d\r\n", (int)(width), (int)(height), (int)(mem_info.width), (int)(mem_info.height));
		mem_info.svc_layer = svc_layer;
		mem_info.ltr_interval = ltr_interval;
		mem_info.tile_mode_en = tile_mode_en;
		mem_info.d2d_mode_en = d2d;
		mem_info.gdc_mode_en = gdc;

		if (kdrv_videoenc_get(kdrv_id, VDOENC_GET_MEM_SIZE, &mem_info) != 0)
			nvt_dbg(INFO, "query mem_size error\r\n");

		if ((hdl_mem = vdocdc_get_mem(&info, mem_info.size)) == NULL) {
			nvt_dbg(ERR, "get_mem error\r\n");
		}

		buf_addr = (UINT32)info.vaddr;
		buf_size = mem_info.size;
	}

	#if 0
	//aq
	{
		KDRV_VDOENC_AQ aq_info={0};

		if (kdrv_videoenc_get(kdrv_id, VDOENC_GET_AQ, &aq_info) != 0)
			nvt_dbg(INFO, "get aq_info error\r\n");
		printk("aq_info\r\n");
		printk("(%d,%d,%d,%d,%d)\r\n", aq_info.enable, aq_info.i_str,aq_info.p_str,aq_info.max_delta_qp,aq_info.min_delta_qp);
	}
	//rrc
	{
		KDRV_VDOENC_ROW_RC rrc_info={0};

		if (kdrv_videoenc_get(kdrv_id, VDOENC_GET_ROWRC, &rrc_info) != 0)
			nvt_dbg(INFO, "get rrc_info error\r\n");
		printk("rrc_info\r\n");
		printk("(%d,%d,%d,%d,%d,%d,%d,%d,%d)\r\n", rrc_info.enable,rrc_info.i_qp_range,rrc_info.i_qp_step,rrc_info.p_qp_range,rrc_info.p_qp_step,
						rrc_info.min_i_qp,rrc_info.max_i_qp,rrc_info.min_p_qp,rrc_info.max_p_qp);
	}

	//rdo
	{
		KDRV_VDOENC_RDO rdo_info={0};

		if (kdrv_videoenc_get(kdrv_id, VDOENC_GET_RDO, &rdo_info) != 0)
			nvt_dbg(INFO, "get rdo_info error\r\n");
		printk("rdo_info\r\n");
		printk("(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\r\n", rdo_info.rdo_codec,rdo_info.rdo_param.rdo_265.hevc_intra_32x32_cost_bias,rdo_info.rdo_param.rdo_265.hevc_intra_16x16_cost_bias,
			rdo_info.rdo_param.rdo_265.hevc_intra_8x8_cost_bias,rdo_info.rdo_param.rdo_265.hevc_inter_skip_cost_bias,rdo_info.rdo_param.rdo_265.hevc_inter_merge_cost_bias,
			rdo_info.rdo_param.rdo_265.hevc_inter_64x64_cost_bias,rdo_info.rdo_param.rdo_265.hevc_inter_64x32_32x64_cost_bias,rdo_info.rdo_param.rdo_265.hevc_inter_32x32_cost_bias,
			rdo_info.rdo_param.rdo_265.hevc_inter_32x16_16x32_cost_bias,rdo_info.rdo_param.rdo_265.hevc_inter_16x16_cost_bias);
	}
	#endif

	{
		KDRV_VDOENC_INIT init;
		memset(&init, 0, sizeof(KDRV_VDOENC_INIT));

		init.buf_addr = buf_addr;
		init.buf_size = buf_size;

		init.rotate = rotate;
		init.width  = width;
		init.height = height;		
		init.byte_rate = 0;
		init.frame_rate = 15 * 1000;
		init.gop = 30;
		init.init_i_qp = 30;
		init.min_i_qp = 0;
		init.max_i_qp = 51;
		init.init_p_qp = 30;
		init.min_p_qp = 0;
		init.max_p_qp = 51;
		init.user_qp_en = 0;
		init.static_time = 1;
		init.ip_weight = 0;
		init.e_dar = KDRV_VDOENC_DAR_DEFAULT;		
		init.e_svc = svc_layer;
		init.ltr_interval = ltr_interval;
		init.ltr_pre_ref = 0;
		init.bTileEn = tile_mode_en;
		init.bD2dEn = d2d;
		init.gdc_mode_en = gdc;

		init.fast_search = 0;
		init.e_profile = KDRV_VDOENC_PROFILE_HIGH;    // PROFILE_HIGH
		init.color_range = 1;
		init.project_mode = 1;
		init.sei_idf_en = 0;

		init.e_entropy = KDRV_VDOENC_CABAC;
		init.level_idc = 50;
		init.sar_width = width;
		init.sar_height = height;
		init.gray_en = 0;
		init.disable_db = 6;
		init.db_alpha = 0;
		init.db_beta = 0;
		init.sao_en = 1;
		init.sao_luma_flag = 1;
		init.sao_chroma_flag = 1;

		init.bVUIEn = 1;
		init.matrix_coef = 1;
		init.transfer_characteristics = 1;
		init.colour_primaries = 1;
		init.video_format = 5;
		init.time_present_flag = 1;

		init.bFBCEn = 0;
		init.chrm_qp_idx = 0;
		init.sec_chrm_qp_idx = 0;
		init.hw_padding_en = 0;
		
		// video codec not used //
		init.jpeg_yuv_format = 0;
		init.multi_layer = 0;
		// end of video codec not used //		
		
		if (kdrv_videoenc_set(kdrv_id, VDOENC_SET_INIT, &init) != 0)
			DBG_ERR("init error\r\n");

		{
			KDRV_VDOENC_DESC hdr_info;

			if (kdrv_videoenc_get(kdrv_id, VDOENC_GET_DESC, &hdr_info) != 0)
				DBG_ERR("get header error\r\n");

			//h26xFileWrite(&fd_bs, hdr_info.size, hdr_info.addr);	// I frame use kdrv auto get sps/pps //
			h26xFileWrite(&fd_bslen, sizeof(unsigned int), (UINT32)&hdr_info.size);
		}
	}

	if (bTestGdrEn) gdr_test(kdrv_id);
	if (bTestRcEn)  rc_test(kdrv_id, width, height);
	if (bTestRowRc) rrc_test(kdrv_id);
	if (bTestRoi)   roi_test(kdrv_id);
	if (bTestUsrQp) usr_qp_test(kdrv_id, width, height);
	if (bTestSlice) slice_test(kdrv_id, height);
	if (bTestRdo) rdo_test(kdrv_id);

	for (i = 0; i < 60; i++)
	{	
		//UINT32 enc_bs_len;
		b_get_vdo_cb = false;
		
		enc_obj.bs_addr_1 = enc_obj.bs_start_addr;
		enc_obj.bs_size_1 = frm_size;
		//enc_obj.frm_type = (i == 0) ? 3 : 0;

		memset((void *)enc_obj.bs_start_addr, 0, frm_size);
		h26x_cache_clean(enc_obj.bs_start_addr, frm_size);

		if (sde_en) {
			h26xFileRead(&fd_src, frm_size*3/4, enc_obj.y_addr);	
			h26xFileRead(&fd_src, frm_size/2*3/4, enc_obj.c_addr);

			h26x_cache_clean(enc_obj.y_addr, frm_size*3/4);
			h26x_cache_clean(enc_obj.c_addr, frm_size/2*3/4);
		} else {
			h26xFileRead(&fd_src, frm_size, enc_obj.y_addr);	
			h26xFileRead(&fd_src, frm_size/2, enc_obj.c_addr);

			h26x_cache_clean(enc_obj.y_addr, frm_size);
			h26x_cache_clean(enc_obj.c_addr, frm_size/2);
		}

		if (kdrv_videoenc_trigger(kdrv_id, &enc_obj, &callback, &i) != 0)
			DBG_ERR("encode error\r\n");		

#ifdef VDOCDC_LL
		while(b_get_vdo_cb == false) {			
			mdelay(10);
		}
#endif		

		//enc_bs_len = h26x_getBslen();

		//h26x_flushCache(enc_obj.bs_addr_1, enc_bs_len);	// remove to h265enc_api getResult //
		
		h26xFileWrite(&fd_bs, enc_obj.bs_size_1, enc_obj.bs_addr_1);
		h26xFileWrite(&fd_bslen, sizeof(unsigned int), (UINT32)&enc_obj.bs_size_1);
        
		if (enc_obj.src_out_en == 1)
		{
			h26x_cache_invalidate(enc_obj.src_out_y_addr, SIZE_64X(width)*SIZE_64X(height));
			h26x_cache_invalidate(enc_obj.src_out_c_addr, SIZE_64X(width)*SIZE_64X(height)/2);

			h26xFileWrite(&fd_src_out, SIZE_64X(width)*SIZE_64X(height), enc_obj.src_out_y_addr);
			h26xFileWrite(&fd_src_out, SIZE_64X(width)*SIZE_64X(height)/2, enc_obj.src_out_c_addr);
		}
	}
	{
		if (kdrv_videoenc_set(kdrv_id, VDOENC_SET_CLOSE, NULL) != 0)
			DBG_ERR("close error\r\n");
	}

	if (hdl_mem != NULL) vdocdc_free_mem(hdl_mem);
	if (hdl_src != NULL) vdocdc_free_mem(hdl_src);

	h26xFileClose(&fd_src);
	h26xFileClose(&fd_bs);
	h26xFileClose(&fd_bslen);
	h26xFileClose(&fd_src_out);
}

static void dec_ref_frm_cb(UINT32 pathID, UINT32 uiYAddr, BOOL bIsRef)
{
	;
}

void sim_hevc_dec_main(UINT32 width, UINT32 height, char *string)
{
	UINT32 kdrv_id = ((KDRV_VIDEOCDC_ENGINE0<<KDRV_ENGINE_OFFSET) | (KDRV_VDODEC_ID_1));
	void *hdl_mem = NULL;
	void *hdl_src = NULL;	
	//UINT32 frm_size = width*height;
	UINT32 rec_size = SIZE_64X(width)*SIZE_64X(height);
	UINT32 buf_addr, buf_size;
	UINT32 i;

	struct nvt_fmem_mem_info_t info = {0};
	KDRV_VDODEC_PARAM dec_obj;
	KDRV_CALLBACK_FUNC callback;
	KDRV_VDODEC_MEM_INFO mem_info = {0};

	H26XFile fd_rec, fd_bs, fd_bslen;

	h26xFileOpen(&fd_rec, string, FST_OPEN_WRITE | FST_OPEN_ALWAYS);
#if defined(__LINUX)	
	h26xFileOpen(&fd_bs, "/mnt/sd/bs.265", FST_OPEN_READ);
	h26xFileOpen(&fd_bslen, "/mnt/sd/bslen_265.dat", FST_OPEN_READ);		
#else
	h26xFileOpen(&fd_bs, "A:\\bs.265", FST_OPEN_READ);
	h26xFileOpen(&fd_bslen, "A:\\bslen_265.dat", FST_OPEN_READ);
#endif

#ifdef VDOCDC_LL
	{
		void *hdl_ll_mem = NULL;
		KDRV_VDOENC_LL_MEM_INFO mem_info = {0};
		KDRV_VDOENC_LL_MEM mem = {0};
				
		kdrv_videodec_get(0, VDODEC_GET_LL_MEM_SIZE, &mem_info);
			
		if (mem_info.size != 0) {
			if ((hdl_ll_mem = vdocdc_get_mem(&info, mem_info.size)) == NULL) {
				nvt_dbg(ERR, "get_mem error\r\n");
			}
		
			mem.addr = (UINT32)info.vaddr;
			mem.size = (UINT32)info.size;
		
			kdrv_videodec_set(0, VDODEC_SET_LL_MEM, &mem);
		}
	}
#endif

	{				
		//struct nvt_fmem_mem_info_t info = {0};

		UINT32 addr; 
		UINT32 size = rec_size*5;
		if ((hdl_src = vdocdc_get_mem(&info, size)) == NULL) {
			nvt_dbg(ERR, "get_mem error\r\n");	
		}

		addr = (UINT32)info.vaddr;
		memset((void *)addr, 0, size);
		h26x_cache_clean(addr, size);
		memset(&dec_obj, 0, sizeof(KDRV_VDODEC_PARAM));		

		dec_obj.bs_addr = addr; addr += rec_size;
		dec_obj.y_addr  = addr; addr += rec_size;
		dec_obj.c_addr  = addr;				
		
		dec_obj.vRefFrmCb.id = kdrv_id;
		dec_obj.vRefFrmCb.VdoDec_RefFrmDo = dec_ref_frm_cb;

		callback.callback = kdrv_vdo_cb;
	}

	{
		KDRV_VDODEC_TYPE codec_type = VDODEC_TYPE_H265;

		if (kdrv_videodec_set(kdrv_id, VDODEC_SET_CODEC, &codec_type) != 0)
			nvt_dbg(INFO, "set codec type error\r\n");
	}

	{
		//struct nvt_fmem_mem_info_t info = {0};			
			
		mem_info.hdr_bs_addr = dec_obj.bs_addr;
		
		h26xFileRead(&fd_bslen, sizeof(unsigned int), (UINT32)&mem_info.hdr_bs_len);
		h26xFileRead(&fd_bs, sizeof(char)*mem_info.hdr_bs_len, mem_info.hdr_bs_addr);									

		if (kdrv_videodec_get(kdrv_id, VDODEC_GET_MEM_SIZE, &mem_info) != 0)
			nvt_dbg(INFO, "query mem_size error\r\n");

		if ((hdl_mem = vdocdc_get_mem(&info, mem_info.size)) == NULL) {
			nvt_dbg(ERR, "get_mem error\r\n");
		}

		buf_addr = (UINT32)info.vaddr;
		buf_size = mem_info.size;
	}

	{
		KDRV_VDODEC_INIT init;

		init.width = mem_info.width;
		init.height = mem_info.height;
		init.desc_addr = mem_info.hdr_bs_addr;
		init.desc_size = mem_info.hdr_bs_len;
		init.display_width = init.width;
		init.buf_addr = buf_addr;
		init.buf_size = buf_size;
		
		if (kdrv_videodec_set(kdrv_id, VDODEC_SET_INIT, &init) != 0)
			DBG_ERR("init error\r\n");
	}

	for (i = 0; i < 3; i++) {
		b_get_vdo_cb = false;
		
		h26xFileRead(&fd_bslen, sizeof(unsigned int), (UINT32)&dec_obj.bs_size);

		if (i == 0) dec_obj.bs_size -= mem_info.hdr_bs_len;

		h26xFileRead(&fd_bs, sizeof(char)*dec_obj.bs_size, dec_obj.bs_addr);
		h26x_cache_clean(dec_obj.bs_addr, dec_obj.bs_size);

		if (kdrv_videodec_trigger(kdrv_id, &dec_obj, &callback, &i) != 0)
			DBG_ERR("decode error\r\n");

#ifdef VDOCDC_LL
		while(b_get_vdo_cb == false) {			
			mdelay(10);
		}
#endif

		h26x_cache_invalidate(dec_obj.y_addr, rec_size);
		h26x_cache_invalidate(dec_obj.c_addr, rec_size/2);

		h26xFileWrite(&fd_rec, rec_size, dec_obj.y_addr);
		h26xFileWrite(&fd_rec, rec_size/2, dec_obj.c_addr);
        if (i % 2 == 0) {
            dec_obj.y_addr += rec_size*3/2;
            dec_obj.c_addr += rec_size*3/2;
        } else {
            dec_obj.y_addr -= rec_size*3/2;
            dec_obj.c_addr -= rec_size*3/2;
        }
	}
	
	if (hdl_src != NULL) vdocdc_free_mem(hdl_src);
	if (hdl_mem != NULL) vdocdc_free_mem(hdl_mem);	

	h26xFileClose(&fd_rec);
	h26xFileClose(&fd_bs);
	h26xFileClose(&fd_bslen);
}

void sim_hevc_enc_main_pwm(UINT32 width, UINT32 height, char *string)
{
	UINT32 kdrv_id = ((KDRV_VIDEOCDC_ENGINE0<<KDRV_ENGINE_OFFSET) | (KDRV_VDOENC_ID_1));
	void *hdl_mem = NULL;
	void *hdl_src = NULL;
	UINT32 frm_size = width*height;
	UINT32 buf_addr, buf_size;		
	UINT32 i;
	KDRV_VDOENC_SVC svc_layer = KDRV_VDOENC_SVC_DISABLE;
	UINT32 ltr_interval = 0;
	UINT32 rotate = 0;	// 0 : no rotate , 1 : counterclockwise rotate , 2 : clockwise rotate //
	UINT32 tile_mode_en = 0;
	UINT32 sde_en = 0;
	UINT32 d2d = 0;
	UINT32 gdc = 0;
	UINT32 src_y_addr[2], src_c_addr[2];
	UINT32 frm_num = (width >= 3840) ? 600 : 2400;
	
	KDRV_VDOENC_PARAM  enc_obj;
	KDRV_CALLBACK_FUNC callback;
	struct nvt_fmem_mem_info_t info = {0};
	
	H26XFile fd_src;

	h26xFileOpen(&fd_src, string, FST_OPEN_READ);
	#if 0
	#if defined(__LINUX)
	h26xFileOpen(&fd_bs, "/mnt/sd/bs.265", FST_OPEN_WRITE | FST_OPEN_ALWAYS);
	h26xFileOpen(&fd_bslen, "/mnt/sd/bslen_265.dat", FST_OPEN_WRITE | FST_OPEN_ALWAYS);	
	h26xFileOpen(&fd_src_out, "/mnt/sd/src.pak", FST_OPEN_WRITE | FST_OPEN_ALWAYS);
	#else
	h26xFileOpen(&fd_bs, "A:\\bs.265", FST_OPEN_WRITE | FST_OPEN_ALWAYS);
	h26xFileOpen(&fd_bslen, "A:\\bslen_265.dat", FST_OPEN_WRITE | FST_OPEN_ALWAYS);	
	h26xFileOpen(&fd_src_out, "A:\\src.pak", FST_OPEN_WRITE | FST_OPEN_ALWAYS);
	#endif
	#endif

	// init enc_obj //
	{				
		//struct nvt_fmem_mem_info_t info = {0};

		UINT32 addr;
		UINT32 size = frm_size*4 + 512;

		if ((hdl_src = vdocdc_get_mem(&info, size)) == NULL) {
			nvt_dbg(ERR, "get_mem error\r\n");	
		}

		addr = (UINT32)info.vaddr;
		memset((void *)addr, 0, size);
		h26x_cache_clean(addr, size);
		memset(&enc_obj, 0, sizeof(KDRV_VDOENC_PARAM));		
		
		src_y_addr[0] = (UINT32)addr; addr += frm_size;
		src_c_addr[0] = (UINT32)addr; addr += (frm_size/2);
		src_y_addr[1] = (UINT32)addr; addr += frm_size;
		src_c_addr[1] = (UINT32)addr; addr += (frm_size/2);
		
		enc_obj.y_line_offset = width;
		enc_obj.c_line_offset = width;
		enc_obj.src_out_en = 0;
		enc_obj.src_out_y_addr = src_y_addr[0];
		enc_obj.src_out_c_addr = src_c_addr[0];
		enc_obj.src_out_y_line_offset = SIZE_64X(width);
		enc_obj.src_out_c_line_offset = SIZE_64X(width);
		enc_obj.bs_start_addr = addr; 	addr += frm_size;
		enc_obj.bs_end_addr = addr;	
		enc_obj.nalu_len_addr = addr;

		if (sde_en) {
            enc_obj.st_sdc.enable = sde_en;
            enc_obj.st_sdc.width = width;
            enc_obj.st_sdc.height = height;
            enc_obj.st_sdc.y_lofst = SIZE_16X(width)*3/4;
            enc_obj.st_sdc.c_lofst = SIZE_32X(width)*3/4;
        }
		
		callback.callback = kdrv_vdo_cb;

		for (i = 0; i < 2; i++) {
			h26xFileRead(&fd_src, width*height, src_y_addr[i]);	
			h26xFileRead(&fd_src, (width*height)/2, src_c_addr[i]);

			h26x_cache_clean(src_y_addr[i], width*height);
			h26x_cache_clean(src_c_addr[i], (width*height)/2);
		}
	}		
	
	{
		KDRV_VDOENC_TYPE codec_type = VDOENC_TYPE_H265;

		if (kdrv_videoenc_set(kdrv_id, VDOENC_SET_CODEC, &codec_type) != 0)
			nvt_dbg(INFO, "set codec type error\r\n");
	}
	
	{
		//struct nvt_fmem_mem_info_t info = {0};	
		KDRV_VDOENC_MEM_INFO mem_info = {0};
		
		mem_info.width = SIZE_64X(((rotate == 1 || rotate == 2) ? height : width));
		mem_info.height = SIZE_64X(((rotate == 1 || rotate == 2) ? width : height));
		DBG_ERR("%d, %d, %d, %d\r\n", (int)(width), (int)(height), (int)(mem_info.width), (int)(mem_info.height));
		mem_info.svc_layer = svc_layer;
		mem_info.ltr_interval = ltr_interval;
		mem_info.tile_mode_en = tile_mode_en;
		mem_info.d2d_mode_en = d2d;
		mem_info.gdc_mode_en = gdc;

		if (kdrv_videoenc_get(kdrv_id, VDOENC_GET_MEM_SIZE, &mem_info) != 0)
			nvt_dbg(INFO, "query mem_size error\r\n");

		if ((hdl_mem = vdocdc_get_mem(&info, mem_info.size)) == NULL) {
			nvt_dbg(ERR, "get_mem error\r\n");
		}

		buf_addr = (UINT32)info.vaddr;
		buf_size = mem_info.size;
	}

	
	{
		KDRV_VDOENC_INIT init;
		memset(&init, 0, sizeof(KDRV_VDOENC_INIT));

		init.buf_addr = buf_addr;
		init.buf_size = buf_size;

		init.rotate = rotate;
		init.width  = width;
		init.height = height;		
		init.byte_rate = 0;
		init.frame_rate = 15 * 1000;
		init.gop = 300;
		init.init_i_qp = 30;
		init.min_i_qp = 0;
		init.max_i_qp = 51;
		init.init_p_qp = 30;
		init.min_p_qp = 0;
		init.max_p_qp = 51;
		init.user_qp_en = 0;
		init.static_time = 1;
		init.ip_weight = 0;
		init.e_dar = KDRV_VDOENC_DAR_DEFAULT;		
		init.e_svc = svc_layer;
		init.ltr_interval = ltr_interval;
		init.ltr_pre_ref = 0;
		init.bTileEn = tile_mode_en;
		init.bD2dEn = d2d;
		init.gdc_mode_en = gdc;

		init.fast_search = 0;
		init.e_profile = KDRV_VDOENC_PROFILE_HIGH;    // PROFILE_HIGH
		init.color_range = 1;
		init.project_mode = 1;
		init.sei_idf_en = 0;

		init.e_entropy = KDRV_VDOENC_CABAC;
		init.level_idc = 50;
		init.sar_width = width;
		init.sar_height = height;
		init.gray_en = 0;
		init.disable_db = 6;
		init.db_alpha = 0;
		init.db_beta = 0;
		init.sao_en = 1;
		init.sao_luma_flag = 1;
		init.sao_chroma_flag = 1;

		init.bVUIEn = 1;
		init.matrix_coef = 1;
		init.transfer_characteristics = 1;
		init.colour_primaries = 1;
		init.video_format = 5;
		init.time_present_flag = 1;

		init.bFBCEn = 0;
		init.chrm_qp_idx = 0;
		init.sec_chrm_qp_idx = 0;
		init.hw_padding_en = 0;
		
		// video codec not used //
		init.jpeg_yuv_format = 0;
		init.multi_layer = 0;
		// end of video codec not used //		
		
		if (kdrv_videoenc_set(kdrv_id, VDOENC_SET_INIT, &init) != 0)
			DBG_ERR("init error\r\n");

		{
			KDRV_VDOENC_DESC hdr_info;

			if (kdrv_videoenc_get(kdrv_id, VDOENC_GET_DESC, &hdr_info) != 0)
				DBG_ERR("get header error\r\n");

			//h26xFileWrite(&fd_bs, hdr_info.size, hdr_info.addr);	// I frame use kdrv auto get sps/pps //
			//h26xFileWrite(&fd_bslen, sizeof(unsigned int), (UINT32)&hdr_info.size);
		}
	}


	for (i = 0; i < frm_num; i++) {	
		b_get_vdo_cb = false;
		
		enc_obj.bs_addr_1 = enc_obj.bs_start_addr;
		enc_obj.bs_size_1 = frm_size;

		enc_obj.y_addr = src_y_addr[i%2];
		enc_obj.c_addr = src_c_addr[i%2];

		if (kdrv_videoenc_trigger(kdrv_id, &enc_obj, &callback, &i) != 0)
			DBG_ERR("encode error\r\n");		

#ifdef VDOCDC_LL
		while(b_get_vdo_cb == false) {			
			mdelay(10);
		}
#endif		
	}
	
	{
		if (kdrv_videoenc_set(kdrv_id, VDOENC_SET_CLOSE, NULL) != 0)
			DBG_ERR("close error\r\n");
	}

	if (hdl_mem != NULL) vdocdc_free_mem(hdl_mem);
	if (hdl_src != NULL) vdocdc_free_mem(hdl_src);

	h26xFileClose(&fd_src);
}
