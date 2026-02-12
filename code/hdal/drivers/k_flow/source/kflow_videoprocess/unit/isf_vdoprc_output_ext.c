#include "isf_vdoprc_int.h"
#if (USE_OUT_EXT == ENABLE)
#if (USE_GFX == ENABLE)
#include "gximage/gximage.h"
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoprc_ox
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoprc_ox_debug_level = NVT_DBG_WRN;
//module_param_named(isf_vdoprc_ox_debug_level, isf_vdoprc_ox_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_vdoprc_ox_debug_level, "vdoprc_ox debug level");
static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)	vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)	vk_spin_unlock_irqrestore(&my_lock, flags)
///////////////////////////////////////////////////////////////////////////////

#define WATCH_PORT			ISF_OUT(0) // please fill your extend port
#define FORCE_DUMP_DATA		DISABLE

/*
HI
FRC/CROP -> NR -> STAMP -> FRC -> SCALE/CROP -> MIRROR/ROTATE -> | -> CROP/SCALE
*/

/*
RHE           -> IFE  -> DCE -> IPE                    -> IME
(CROP/MIRROR)                   BAYER2YUV          (STAMP/MASK)(SCALE/CROP)
WDR/SHDR/DEFOG   (NR)   (GDC)  (COLOR/GAMMA/ADJUST)    (3DNR)

IPE                  -> IME
                     (STAMP/MASK)(SCALE/CROP)
(COLOR/GAMMA/ADJUST)

IPE
(COLOR/GAMMA/ADJUST)

IME
(STAMP/MASK)(SCALE/CROP)


CROP/MIRROR ...  | CROP/SCALE/MIRROR
*/

#if (USE_OUT_EXT == ENABLE)

void isf_pipe_begin(ISF_PIPE* p_pipe)
{
	p_pipe->cmd_count = 0;
	p_pipe->cmd[p_pipe->cmd_count] = ISF_PIPE_CMD_BEGIN; p_pipe->cmd_count++;
}
void isf_pipe_add_scrop(ISF_PIPE* p_pipe, INT32 crop_x, INT32 crop_y, INT32 crop_w, INT32 crop_h)
{
	p_pipe->cmd[p_pipe->cmd_count] = ISF_PIPE_CMD_SCROP; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = crop_x; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = crop_y; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = crop_w; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = crop_h; p_pipe->cmd_count++;
}
/*
void isf_pipe_add_dcrop(ISF_PIPE* p_pipe, INT32 crop_x, INT32 crop_y, INT32 crop_w, INT32 crop_h)
{
	p_pipe->cmd[p_pipe->cmd_count] = ISF_PIPE_CMD_DCROP; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = crop_x; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = crop_y; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = crop_w; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = crop_h; p_pipe->cmd_count++;
}
*/
void isf_pipe_add_scale(ISF_PIPE* p_pipe, INT32 scale_w, INT32 scale_h, UINT32 pxlfmt, INT32 scale_h_align)
{
	p_pipe->cmd[p_pipe->cmd_count] = ISF_PIPE_CMD_SCALE; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = scale_w; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = scale_h; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = pxlfmt; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = scale_h_align; p_pipe->cmd_count++;
}
void isf_pipe_add_dir(ISF_PIPE* p_pipe, UINT32 dir, INT32 h_align)
{
	p_pipe->cmd[p_pipe->cmd_count] = ISF_PIPE_CMD_DIR; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = dir; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = h_align; p_pipe->cmd_count++;
}
/*
void isf_pipe_add_pxlfmt(ISF_PIPE* p_pipe, UINT32 pxlfmt)
{
	p_pipe->cmd[p_pipe->cmd_count] = ISF_PIPE_CMD_PXLFMT; p_pipe->cmd_count++;
	p_pipe->cmd[p_pipe->cmd_count] = pxlfmt; p_pipe->cmd_count++;
}*/
void isf_pipe_end(ISF_PIPE* p_pipe)
{
	p_pipe->cmd[p_pipe->cmd_count] = ISF_PIPE_CMD_END; p_pipe->cmd_count++;
}

ISF_RV isf_pipe_exec(ISF_UNIT* p_unit, UINT32 nport, ISF_PIPE* p_pipe, ISF_DATA* p_src_data, ISF_DATA* p_tmp_rotate)
{
	#if (USE_GFX == ENABLE)
    VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_unit->refdata);
	ISF_RV r = ISF_OK;
	ISF_DATA tmp_data = {0};
	VDO_FRAME* p_src_img = (VDO_FRAME*)&(p_src_data->desc[0]);
	VDO_FRAME* p_tmp_img = (VDO_FRAME*)&(tmp_data.desc[0]);
	//IPOINT dst_pt = {0};
	IRECT src_crop = {0};
	//IRECT dst_crop = {0};
	USIZE src_scale = {0};
	USIZE dst_scale = {0};
	UINT32 dir = 0;
	UINT32 src_pxlfmt = 0;
	UINT32 dst_pxlfmt = 0;
	UINT32 last_cmd = 0;
	UINT32 exe_count = 0;
	UINT32 new_img_count = 0;
	ER ret = E_OK;
	static ISF_DATA_CLASS g_vdoprc_tmp_data = {0};
	IRECT* p_src_crop = NULL;
	//IRECT* p_dst_crop = NULL;
	IPOINT* p_dst_pt = NULL;
	INT32   dest_h_align = 0;
	UINT32  dest_loff_align = p_ctx->out_loff_align[nport - ISF_OUT_BASE];
    UINT32  is_copy_meta = (p_ctx->out_vndcfg_func[nport - ISF_OUT_BASE] & VDOPRC_OFUNC_VND_COPYMETA);
    UINT32  total_meta_size = 0;
	//ISF_RV rv_new = ISF_OK;
	//ISF_RV rv_proc = ISF_OK;
	//ISF_RV rv_push = ISF_OK;

	if(p_pipe->cmd_count == 0)
		return ISF_ERR_IGNORE;
	p_src_img = (VDO_FRAME*)&(p_src_data->desc[0]);
	p_tmp_img = (VDO_FRAME*)&(tmp_data.desc[0]);
	src_scale.w = p_src_img->size.w;
	src_scale.h = p_src_img->size.h;
	src_pxlfmt = p_src_img->pxlfmt;
	p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "PIPE: in  w = %d, h=%d imgfmt=%08x", src_scale.w, src_scale.h, src_pxlfmt);
	while(exe_count < p_pipe->cmd_count) {
		UINT32 cmd = p_pipe->cmd[exe_count];
		exe_count++;
		switch(cmd) {
		case ISF_PIPE_CMD_BEGIN:
			last_cmd = cmd;
			p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "PIPE: begin");
			break;
		case ISF_PIPE_CMD_SCROP:
			src_crop.x = p_pipe->cmd[exe_count]; exe_count++;
			src_crop.y = p_pipe->cmd[exe_count]; exe_count++;
			src_crop.w = p_pipe->cmd[exe_count]; exe_count++;
			src_crop.h = p_pipe->cmd[exe_count]; exe_count++;
			if(!(src_crop.x==0 && src_crop.y==0 && src_crop.w==0 && src_crop.h==0)) {
				p_src_crop = &src_crop;
			}
			last_cmd = cmd;
			p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "PIPE: src_crop x=%d, y=%d, w=%d, h=%d", src_crop.x, src_crop.y, src_crop.w, src_crop.h);
			break;
		/*
		case ISF_PIPE_CMD_PXLFMT:
			pxlfmt = p_pipe->cmd[exe_count]; exe_count++;
			last_cmd = cmd;
			break;*/
		case ISF_PIPE_CMD_SCALE:
			dst_scale.w = p_pipe->cmd[exe_count]; exe_count++;
			dst_scale.h = p_pipe->cmd[exe_count]; exe_count++;
			dst_pxlfmt = p_pipe->cmd[exe_count]; exe_count++;
			dest_h_align= p_pipe->cmd[exe_count]; exe_count++;
			last_cmd = cmd;
			p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "PIPE: out w = %d, h=%d pxlfmt=%08x", dst_scale.w, dst_scale.h, dst_pxlfmt);
			break;
		case ISF_PIPE_CMD_DIR:
			dir = p_pipe->cmd[exe_count]; exe_count++;
			dest_h_align= p_pipe->cmd[exe_count]; exe_count++;
			last_cmd = cmd;
			p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "PIPE: direct = %08x", dir);
			break;
		/*
		case ISF_PIPE_CMD_DCROP:
			dst_crop.x = p_pipe->cmd[exe_count]; exe_count++;
			dst_crop.y = p_pipe->cmd[exe_count]; exe_count++;
			dst_crop.w = p_pipe->cmd[exe_count]; exe_count++;
			dst_crop.h = p_pipe->cmd[exe_count]; exe_count++;
			if(!(dst_crop.x==0 && dst_crop.y==0 && dst_crop.w==0 && dst_crop.h==0)) {
				p_dst_crop = &dst_crop;
			}
			p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "PIPE: dst_crop x=%d, y=%d, w=%d, h=%d", dst_crop.x, dst_crop.y, dst_crop.w, dst_crop.h);
			last_cmd = cmd;
			break;
		*/
		case ISF_PIPE_CMD_END:
			p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "PIPE: end");
			last_cmd = cmd;
			break;
		}
		switch(last_cmd) {
		case 0:
			break;
		/*
		case ISF_PIPE_CMD_PXLFMT:
			{
			//buf_size = gximg_calc_require_size(w, h, pxlfmt, lineoff);
			//addr = p_pipe->p_unit->p_base->do_new(p_pipe->p_unit, p_pipe->nport, NVTMPP_VB_INVALID_POOL, p_pipe->ddr, buf_size, p_data, ISF_OUT_PROBE_EXT);
			//gximg_copy_color_key_data(PVDO_FRAME p_src_img, IRECT *p_copy_region, PVDO_FRAME p_key_img, IPOINT *p_key_location, UINT32 color_key, BOOL is_copy_to_key_img, GXIMG_CP_ENG engine);
			//gximg_copy_blend_data(PVDO_FRAME p_src_img, IRECT *p_src_region, PVDO_FRAME p_dst_img, IPOINT *p_dst_location, UINT32 alpha, GXIMG_CP_ENG engine);
			//p_pipe->p_unit->p_base->do_release(p_pipe->p_unit, p_pipe->nport, p_data, 0);
			//p_pipe->p_unit->p_base->do_probe(p_pipe->p_unit, p_pipe->nport, 0, r);
			last_cmd = 0;
			}
			break;*/
		case ISF_PIPE_CMD_SCALE:
			{
			UINT32 buf_size;
			UINT32 buf_addr;
			//det_scale, src_crop, dst_crop;
			p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "scale begin");
			if ((dst_scale.w == 0) || (dst_scale.h == 0)) {
				//DBG_ERR("dim zero\r\n");
				ret = E_SYS;
				r = ISF_ERR_PARAM_EXCEED_LIMIT;
				goto ext_quit;
			}
			if((dst_scale.w > 0) && (dst_scale.h > 0)) { //use scale
				buf_size = gximg_calc_require_size(dst_scale.w, dest_h_align ? ALIGN_CEIL(dst_scale.h, dest_h_align) : dst_scale.h, dst_pxlfmt, dest_loff_align ? ALIGN_CEIL(dst_scale.w, dest_loff_align) : ALIGN_CEIL_16(dst_scale.w));
			} else { //no scale
				buf_size = gximg_calc_require_size(dst_scale.w, dest_h_align ? ALIGN_CEIL(dst_scale.h, dest_h_align) : dst_scale.h, dst_pxlfmt, dest_loff_align ? ALIGN_CEIL(dst_scale.w, dest_loff_align) : ALIGN_CEIL_16(dst_scale.w));
			}
			if (buf_size == 0) {
				//DBG_ERR("buf zero\r\n");
				ret = E_SYS;
				r = ISF_ERR_PARAM_EXCEED_LIMIT;
				goto ext_quit;
			}

            //META#### add meta size for new blk
            if(is_copy_meta){
                total_meta_size = _isf_vdoprc_meta_get_size(p_src_img);
                buf_size += total_meta_size;
            }

			p_unit->p_base->init_data(&tmp_data, &g_vdoprc_tmp_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
			buf_addr = p_unit->p_base->do_new(p_unit, nport, NVTMPP_VB_INVALID_POOL, 0, buf_size, &tmp_data, ISF_OUT_PROBE_EXT);
			if (buf_addr == 0) {
				//DBG_ERR("new fail\r\n");
				ret = E_SYS;
				r = ISF_ERR_NEW_FAIL;
				goto ext_quit;
			}
#if (FORCE_DUMP_DATA == ENABLE)
	if (nport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Pipe new -- Vdo: blk_id=%08x\r\n", p_unit->unit_name, nport - ISF_OUT_BASE, tmp_data.h_data);
	}
#endif
			new_img_count ++;
			p_tmp_img = (VDO_FRAME*)&(tmp_data.desc[0]);
			ret = gximg_init_buf_h_align(p_tmp_img, dst_scale.w, dst_scale.h, dest_h_align ? ALIGN_CEIL(dst_scale.h, dest_h_align) : dst_scale.h, dst_pxlfmt,  dest_loff_align ? GXIMG_LINEOFFSET_ALIGN(dest_loff_align) : GXIMG_LINEOFFSET_ALIGN(16), buf_addr, buf_size);
			if(ret != E_OK) {
				//DBG_ERR("init fail\r\n");
				r = ISF_ERR_PROCESS_FAIL;
				goto ext_quit;
			}

            //META#### copy meta to exten output frame
    	p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "is_copy_meta %d,total_meta_size=%08x", is_copy_meta,total_meta_size);
            if((is_copy_meta)&&(total_meta_size)){
                r= _isf_vdoprc_meta_copy(p_tmp_img,p_src_img,buf_addr+buf_size-total_meta_size,0,p_src_crop);
            }
			//sync count, timestamp from src frame
			p_tmp_img->count = p_src_img->count;
			p_tmp_img->timestamp = p_src_img->timestamp;
			//convert addr
			{
				int p;
				VDO_FRAME* p_vdoframe = p_tmp_img;
				for (p = 0; p < VDO_MAX_PLANE; p++) {
					if ((p_vdoframe->addr[p] != 0) && (p_vdoframe->phyaddr[p] == 0)) {
						//come from vdocap
						p_vdoframe->phyaddr[p] = nvtmpp_sys_va2pa(p_vdoframe->addr[p]);
					} else if ((p_vdoframe->phyaddr[p] != 0) && (p_vdoframe->addr[p] == 0)) {
						//come from user
						p_vdoframe->addr[p] = nvtmpp_sys_pa2va(p_vdoframe->phyaddr[p]);
					}
				}
			}
			if(((dst_scale.w != src_scale.w) || (dst_scale.h != src_scale.h)) || (p_src_crop != NULL)) {
				p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "do scale: size(%d,%d) fmt=%08x => size(%d,%d) fmt=%08x",
					p_src_img->size.w, p_src_img->size.h, p_src_img->pxlfmt, p_tmp_img->size.w, p_tmp_img->size.h, p_tmp_img->pxlfmt);
				if (!p_src_crop)
				p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "crop(%d,%d,%d,%d)",
					src_crop.x, src_crop.y, src_crop.w, src_crop.h);
				ret = gximg_scale_data_no_flush(p_src_img, p_src_crop, p_tmp_img, NULL, GXIMG_SCALE_AUTO, 0);
			} else {
				p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "copy");
				ret = gximg_copy_data_no_flush(p_src_img, p_src_crop, p_tmp_img, p_dst_pt, GXIMG_CP_ENG1, 0);
			}
			if (p_src_crop) {
				p_src_crop = NULL; //end of crop
			}
			if(ret != E_OK) {
				//DBG_ERR("scale fail\r\n");
				r = ISF_ERR_PROCESS_FAIL;
				goto ext_quit;
			}
			if(new_img_count > 1) {
#if (FORCE_DUMP_DATA == ENABLE)
	if (nport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Pipe release -- Vdo: blk_id=%08x\r\n", p_unit->unit_name, nport - ISF_OUT_BASE, p_src_data->h_data);
	}
#endif
				p_unit->p_base->do_release(p_unit, nport, p_src_data, ISF_OUT_PROBE_EXT);
			}
			p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "scale end");
			memcpy((void *)p_src_data, (void *)&tmp_data, sizeof(ISF_DATA));
			memset((void *)&tmp_data, 0, sizeof(ISF_DATA));
			p_src_img = (VDO_FRAME*)&(p_src_data->desc[0]);
			last_cmd = 0;
			src_scale.w = dst_scale.w;
			src_scale.h = dst_scale.h;
			src_pxlfmt = dst_pxlfmt;
			dst_scale.w = 0;
			dst_scale.h = 0;
			dst_pxlfmt = 0;
			// when h_align > h , need to set to vdo frame reserved info
			if (dest_h_align > 0) {
				p_src_img->reserved[0] = MAKEFOURCC('H', 'A', 'L', 'N'); // height align
 				p_src_img->reserved[1] = ALIGN_CEIL(src_scale.h, dest_h_align);
				//DBG_DUMP("scale h_align(%d), value (%d) h(%d)\r\n", dest_h_align, p_src_img->reserved[1], src_scale.h);
			}
			}
			break;
		case ISF_PIPE_CMD_DIR:
			if ((p_tmp_rotate != 0) && (p_tmp_rotate->sign != 0)) {
			//load pre_rotate image
			p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "dir (load)");
			memcpy((void *)p_src_data, (void *)p_tmp_rotate, sizeof(ISF_DATA));
			p_src_img = (VDO_FRAME*)&(p_src_data->desc[0]);
			last_cmd = 0;
			} else {
			UINT32 buf_size;
			UINT32 buf_addr;
			p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "dir begin");
			if (((dir & VDO_DIR_ROTATE_90) == VDO_DIR_ROTATE_90) ||
				((dir & VDO_DIR_ROTATE_270) == VDO_DIR_ROTATE_270)) {
				dst_scale.w = src_scale.h;
				dst_scale.h = src_scale.w;
				dst_pxlfmt = src_pxlfmt;
				buf_size = gximg_calc_require_size(ALIGN_CEIL_16(dst_scale.w), dest_h_align > 16 ? ALIGN_CEIL(dst_scale.h, dest_h_align) : ALIGN_CEIL_16(dst_scale.h), dst_pxlfmt, dest_loff_align ? ALIGN_CEIL(dst_scale.w, dest_loff_align) : ALIGN_CEIL_16(dst_scale.w));
			} else {
				dst_scale.w = src_scale.w;
				dst_scale.h = src_scale.h;
				dst_pxlfmt = src_pxlfmt;
				buf_size = gximg_calc_require_size(dst_scale.w, dest_h_align ? ALIGN_CEIL(dst_scale.h, dest_h_align) : dst_scale.h, dst_pxlfmt, dest_loff_align ? ALIGN_CEIL(dst_scale.w, dest_loff_align) : ALIGN_CEIL_16(dst_scale.w));
			}
			//dir, src_crop, dst_crop;
			if ((dst_scale.w == 0) || (dst_scale.h == 0)) {
				//DBG_ERR("dim zero\r\n");
				ret = E_SYS;
				r = ISF_ERR_PARAM_EXCEED_LIMIT;
				goto ext_quit;
			}
			if (buf_size == 0) {
				//DBG_ERR("buf zero\r\n");
				ret = E_SYS;
				r = ISF_ERR_PARAM_EXCEED_LIMIT;
				goto ext_quit;
			}
            //META#### add meta size for new blk
            if(is_copy_meta){
                total_meta_size = _isf_vdoprc_meta_get_size(p_src_img);
                buf_size += total_meta_size;
            }
			p_unit->p_base->init_data(&tmp_data, &g_vdoprc_tmp_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
			buf_addr = p_unit->p_base->do_new(p_unit, nport, NVTMPP_VB_INVALID_POOL, 0, buf_size, &tmp_data, ISF_OUT_PROBE_EXT);
			if (buf_addr == 0) {
				//DBG_ERR("new fail\r\n");
				ret = E_SYS;
				r = ISF_ERR_NEW_FAIL;
				goto ext_quit;
			}

#if (FORCE_DUMP_DATA == ENABLE)
	if (nport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Pipe new -- Vdo: blk_id=%08x\r\n", p_unit->unit_name, nport - ISF_OUT_BASE, tmp_data.h_data);
	}
#endif
			new_img_count ++;
			p_tmp_img = (VDO_FRAME*)&(tmp_data.desc[0]);
			ret = gximg_init_buf_h_align(p_tmp_img, dst_scale.w, dst_scale.h, dest_h_align ? ALIGN_CEIL(dst_scale.h, dest_h_align) : dst_scale.h, dst_pxlfmt, dest_loff_align ? GXIMG_LINEOFFSET_ALIGN(dest_loff_align) : GXIMG_LINEOFFSET_ALIGN(16), buf_addr, buf_size);
			if(ret != E_OK) {
				//DBG_ERR("init fail\r\n");
				r = ISF_ERR_PROCESS_FAIL;
				goto ext_quit;
			}
            //META#### copy meta to exten output frame
        	//p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_VDOFRM, "is_copy_meta %d,total_meta_size=%08x, p_tmp_rotate=%08", is_copy_meta,total_meta_size,p_tmp_rotate);
            if((is_copy_meta)&&(total_meta_size)){
                if (p_tmp_rotate != 0) {
                    //add pre-rotate to md
                    r= _isf_vdoprc_meta_copy(p_tmp_img,p_src_img,buf_addr+buf_size-total_meta_size,dir|0x1,p_src_crop);
                } else {
                    r= _isf_vdoprc_meta_copy(p_tmp_img,p_src_img,buf_addr+buf_size-total_meta_size,dir,p_src_crop);
                }
            }

			//sync count, timestamp from src frame
			p_tmp_img->count = p_src_img->count;
			p_tmp_img->timestamp = p_src_img->timestamp;
			//convert addr
			{
				int p;
				VDO_FRAME* p_vdoframe = p_tmp_img;
				for (p = 0; p < VDO_MAX_PLANE; p++) {
					if ((p_vdoframe->addr[p] != 0) && (p_vdoframe->phyaddr[p] == 0)) {
						//come from vdocap
						p_vdoframe->phyaddr[p] = nvtmpp_sys_va2pa(p_vdoframe->addr[p]);
					} else if ((p_vdoframe->phyaddr[p] != 0) && (p_vdoframe->addr[p] == 0)) {
						//come from user
						p_vdoframe->addr[p] = nvtmpp_sys_pa2va(p_vdoframe->phyaddr[p]);
					}
				}
			}
			if (((dir & VDO_DIR_ROTATE_90) == VDO_DIR_ROTATE_90) ||
				((dir & VDO_DIR_ROTATE_270) == VDO_DIR_ROTATE_270) ||
				 (dir == VDO_DIR_ROTATE_180) ||
				 (dir == VDO_DIR_MIRRORX) ||
				 (dir == VDO_DIR_MIRRORY) ||
				 (!p_src_crop) ) {
				p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "do rotate: size(%d,%d) fmt=%08x => size(%d,%d) fmt=%08x",
					p_src_img->size.w, p_src_img->size.h, p_src_img->pxlfmt, p_tmp_img->size.w, p_tmp_img->size.h, p_tmp_img->pxlfmt);
				if (!p_src_crop)
				p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "crop(%d,%d,%d,%d)",
					src_crop.x, src_crop.y, src_crop.w, src_crop.h);
				ret = gximg_rotate_paste_data_no_flush(p_src_img, p_src_crop, p_tmp_img, p_dst_pt, dir, GXIMG_ROTATE_ENG1, 0);
			} else {
				p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "copy");
				ret = gximg_copy_data_no_flush(p_src_img, p_src_crop, p_tmp_img, p_dst_pt, GXIMG_CP_ENG1, 0);
			}
			if (p_src_crop) {
				p_src_crop = NULL; //end of crop
			}
			if(ret != E_OK) {
				//DBG_ERR("rotate fail\r\n");
				r = ISF_ERR_PROCESS_FAIL;
				goto ext_quit;
			}
			if (p_tmp_rotate == 0) {
			if(new_img_count > 1) {
#if (FORCE_DUMP_DATA == ENABLE)
	if (nport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Pipe release -- Vdo: blk_id=%08x\r\n", p_unit->unit_name, nport - ISF_OUT_BASE, p_src_data->h_data);
	}
#endif
				p_unit->p_base->do_release(p_unit, nport, p_src_data, ISF_OUT_PROBE_EXT);
			}
			}
			p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "dir end");
			memcpy((void *)p_src_data, (void *)&tmp_data, sizeof(ISF_DATA));
			memset((void *)&tmp_data, 0, sizeof(ISF_DATA));
			if (p_tmp_rotate != 0) {
#if (FORCE_DUMP_DATA == ENABLE)
	if (nport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Pipe add -- Vdo: blk_id=%08x\r\n", p_unit->unit_name, nport - ISF_OUT_BASE, p_src_data->h_data);
	}
#endif
				p_unit->p_base->do_add(p_unit, nport, p_src_data, ISF_OUT_PROBE_EXT);
			//save pre_rotate image
			p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "dir (save)");
			memcpy((void *)p_tmp_rotate, (void *)p_src_data, sizeof(ISF_DATA));
			}
			p_src_img = (VDO_FRAME*)&(p_src_data->desc[0]);
			last_cmd = 0;
			src_scale.w = dst_scale.w;
			src_scale.h = dst_scale.h;
			src_pxlfmt = dst_pxlfmt;
			dst_scale.w = 0;
			dst_scale.h = 0;
			dst_pxlfmt = 0;
			if (dest_h_align > 0) {
				p_src_img->reserved[0] = MAKEFOURCC('H', 'A', 'L', 'N'); // height align
 				p_src_img->reserved[1] = ALIGN_CEIL(p_src_img->size.h, dest_h_align);
				//DBG_DUMP("rotate dir = 0x%x h_align(%d), value (%d) h(%d)\r\n", dir, dest_h_align, p_src_img->reserved[1], p_src_img->size.h);
			}
			}
			break;
/*
		case ISF_PIPE_CMD_END:
			if (p_dst_crop != NULL) {
				p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "crop begin");
				memcpy((void *)&tmp_data, (void *)p_src_data, sizeof(ISF_DATA));
				p_tmp_img = (VDO_FRAME*)&(tmp_data.desc[0]);
				p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "do crop: x=0, y=0, w=%d, h=%d => x=%d, y=%d, w=%d, h=%d",
					p_src_img->size.w, p_src_img->size.h, p_dst_crop->x, p_dst_crop->y, p_dst_crop->w, p_dst_crop->h);
				ret = gximg_crop_data_no_flush(p_src_img, p_dst_crop, p_tmp_img);
				DBGD(ret);
				p_unit->p_base->do_trace(p_unit, nport, ISF_OP_PARAM_VDOFRM, "crop end");
				memcpy((void *)p_src_data, (void *)&tmp_data, sizeof(ISF_DATA));
				memset((void *)&tmp_data, 0, sizeof(ISF_DATA));
				p_src_img = (VDO_FRAME*)&(p_src_data->desc[0]);
				p_tmp_img = NULL;
				last_cmd = 0;
				src_scale.w = dst_scale.w;
				src_scale.h = dst_scale.h;
				src_pxlfmt = dst_pxlfmt;
				dst_scale.w = 0;
				dst_scale.h = 0;
				dst_pxlfmt = 0;
			}
			break;
*/
		}
	}
ext_quit:
	if(ret != E_OK) {
		if(new_img_count > 1) {
#if (FORCE_DUMP_DATA == ENABLE)
if (nport == WATCH_PORT) {
	DBG_DUMP("%s.out[%d]! Pipe release -- Vdo: blk_id=%08x\r\n", p_unit->unit_name, nport - ISF_OUT_BASE, p_src_data->h_data);
}
#endif
			//release temp src
			p_unit->p_base->do_release(p_unit, nport, p_src_data, ISF_OUT_PROBE_EXT);
		}
		if ((r == ISF_ERR_NEW_FAIL) && (new_img_count > 0)) {
			//release src
			p_unit->p_base->do_release(p_unit, nport, p_src_data, ISF_OUT_PROBE_EXT);
		}
		if ((r == ISF_ERR_PARAM_EXCEED_LIMIT) && (new_img_count > 0)) {
			//release src
			p_unit->p_base->do_release(p_unit, nport, p_src_data, ISF_OUT_PROBE_EXT);
		}
		if (r == ISF_ERR_PROCESS_FAIL) {
			//release temp
			p_unit->p_base->do_release(p_unit, nport, &tmp_data, ISF_OUT_PROBE_EXT);
		}
		//r = ISF_ERR_FAILED;
		return r;
	}
	return r;
	#else
	return ISF_ERR_FAILED;
	#endif
}

BOOL _vdoprc_is_out_rotate(ISF_UNIT *p_thisunit, UINT32 pid)
{
	ISF_INFO *p_src_info = NULL;
	ISF_VDO_INFO *p_src_vdoinfo = NULL;

	p_src_info = p_thisunit->port_outinfo[pid - ISF_OUT_BASE];
	p_src_vdoinfo = (ISF_VDO_INFO *)(p_src_info);

	if(((p_src_vdoinfo->direct & VDO_DIR_ROTATE_90) == VDO_DIR_ROTATE_90)
	|| ((p_src_vdoinfo->direct & VDO_DIR_ROTATE_270) == VDO_DIR_ROTATE_270)
	|| (p_src_vdoinfo->direct == VDO_DIR_ROTATE_180)
	|| (p_src_vdoinfo->direct == VDO_DIR_MIRRORX)
	|| (p_src_vdoinfo->direct == VDO_DIR_MIRRORY) ) {
		return TRUE;
	}
	return FALSE;
}

void _vdoprc_config_out_ext(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 en)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_INFO *p_dest_info = p_thisunit->port_outinfo[pid - ISF_OUT_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
	ISF_VDO_INFO *p_dest_vdoinfo = (ISF_VDO_INFO *)(p_thisunit->p_base->get_bind_info(p_thisunit, pid));
	UINT32 src_pid = p_vdoinfo->src;
	//ISF_INFO *p_src_info = p_thisunit->port_outinfo[src_pid - ISF_OUT_BASE];
	//ISF_VDO_INFO *p_src_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
	ISF_INFO *p_src_info = NULL;
	ISF_VDO_INFO *p_src_vdoinfo = NULL;
	ISF_VDO_INFO *p_src_dest_vdoinfo = NULL;
	UINT32 pre_dir;
	BOOL do_pre_rotate = FALSE;
	UINT32 dest_h_align = p_ctx->out_h_align[pid - ISF_OUT_BASE];


	p_src_info = p_thisunit->port_outinfo[src_pid - ISF_OUT_BASE];
	p_src_vdoinfo = (ISF_VDO_INFO *)(p_src_info);

	p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM,
        "window(%d,%d) imgsize (%d,%d) dir=%08x",p_src_vdoinfo->window.w, p_src_vdoinfo->window.h,p_src_vdoinfo->imgsize.w,p_src_vdoinfo->imgsize.h, p_src_vdoinfo->direct);

	if (((p_src_vdoinfo->direct & VDO_DIR_ROTATE_90) == VDO_DIR_ROTATE_90)
	|| ((p_src_vdoinfo->direct & VDO_DIR_ROTATE_270) == VDO_DIR_ROTATE_270)
	|| (p_src_vdoinfo->direct == VDO_DIR_ROTATE_180)
	|| (p_src_vdoinfo->direct == VDO_DIR_MIRRORX)
	|| (p_src_vdoinfo->direct == VDO_DIR_MIRRORY) ) {
		do_pre_rotate = TRUE;
		pre_dir = p_src_vdoinfo->direct;
	} else {
		do_pre_rotate = FALSE;
		pre_dir = 0;
	}

	pid = pid - ISF_OUT_BASE;
	if ( (pid >= ISF_VDOPRC_PHY_OUT_NUM) && (pid < ISF_VDOPRC_OUT_NUM) ) {

		UINT32 i = pid - ISF_VDOPRC_PHY_OUT_NUM;
		ISF_PORT_PATH* p_path = p_thisunit->port_path + pid;
		ISF_PIPE tmp_pipe = {0};
		ISF_PIPE* p_pipe = &tmp_pipe ;
    	unsigned long flags;

		if (!en) {
/*
#if (USE_OUT_EXT == ENABLE)
			if (p_path->flag != 0) { //already start
				_isf_vdoprc_oqueue_do_stop(p_thisunit, ISF_OUT(pid));
			}
#endif
*/
			memset((void*)p_pipe, 0, sizeof(ISF_PIPE));
			p_pipe->cmd_count = 0; //clear
			p_path->flag = 0; //stop ext
            //fix racing (config and dispatch use global pipe)
        	loc_cpu(flags);
            memcpy((void *)&(p_ctx->out_ext[i].pipe),(void *)p_pipe,sizeof(ISF_PIPE));
        	unl_cpu(flags);
			//p_path->oport = ISF_CTRL; //config to TURN OFF
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "extend=off");
#if (USE_OUT_FRC == ENABLE)
			_isf_frc_stop(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]));
#endif
		} else if (!do_pre_rotate) {
			UINT32 w,h,imgfmt;
			UINT32 src_w,src_h,src_imgfmt;
			UINT32 dir;
			UINT32 cx,cy,cw,ch;
			BOOL do_crop = FALSE;
			BOOL do_scale = FALSE;
			BOOL do_rotate = FALSE;
			memset((void*)p_pipe, 0, sizeof(ISF_PIPE));
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "extend=on, from (src)out%d", src_pid);
			p_pipe->cmd_count = 0; //clear

			//set [i] fmt = imgfmt;
			//set [i] size.w = w;
			//set [i] size.h = h;
			//set [i] lofs = ALIGN_CEIL_16(w);
			if (p_vdoinfo->window.x == 0 && p_vdoinfo->window.y == 0 && p_vdoinfo->window.w == 0 && p_vdoinfo->window.h == 0) {
				do_crop = FALSE;
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "crop off");
				//use full image size
				//set [i] crp_window.x = 0;
				//set [i] crp_window.y = 0;
				//set [i] crp_window.w = w;
				//set [i] crp_window.h = h;
				cx = 0;
				cy = 0;
				cw = 0;
				ch = 0;
			} else {
				do_crop = TRUE;
				cx = p_vdoinfo->window.x;
				cy = p_vdoinfo->window.y;
				cw = p_vdoinfo->window.w;
				ch = p_vdoinfo->window.h;
				if (p_vdoinfo->window.w == 0 || p_vdoinfo->window.h == 0) {
					DBG_ERR("-out%d:crop(%d,%d) is zero? force off!\r\n",
						pid, p_vdoinfo->window.w, p_vdoinfo->window.h);
					do_crop = FALSE;
					cx = 0;
					cy = 0;
					cw = 0;
					ch = 0;
					p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "crop off");
				}
				//set [i] crp_window = p_vdoinfo->window;
				//set [i] lofs = ALIGN_CEIL_16(p_vdoinfo->window.w);
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "crop(%d,%d,%d,%d)",
					cx, cy, cw, ch);
			}

			if (((p_vdoinfo->direct & VDO_DIR_ROTATE_90) == VDO_DIR_ROTATE_90)
			|| ((p_vdoinfo->direct & VDO_DIR_ROTATE_270) == VDO_DIR_ROTATE_270)
        	|| (p_vdoinfo->direct == VDO_DIR_ROTATE_180)
        	|| (p_vdoinfo->direct == VDO_DIR_MIRRORX)
        	|| (p_vdoinfo->direct == VDO_DIR_MIRRORY) ) {
				do_rotate = TRUE;
				dir = p_vdoinfo->direct;
				//if ((p_vdoinfo->direct & VDO_DIR_ROTATE_90) == VDO_DIR_ROTATE_90) {
				//DBG_ERR("-out%d:dir --R-\r\n", pid);
				//} else {
				//DBG_ERR("-out%d:dir ---L\r\n", pid);
				//}
			} else {
				do_rotate = FALSE;
				dir = 0;
				//DBG_ERR("-out%d:dir ----\r\n", pid);
			}
            if(p_src_vdoinfo->window.w == 0 && p_src_vdoinfo->window.h == 0) {
    			src_w = p_src_vdoinfo->imgsize.w;
    			src_h = p_src_vdoinfo->imgsize.h;
            } else {
     			src_w = p_src_vdoinfo->window.w;
    			src_h = p_src_vdoinfo->window.h;
            }
    		src_imgfmt = p_src_vdoinfo->imgfmt;

			p_src_dest_vdoinfo = (ISF_VDO_INFO *)(p_thisunit->p_base->get_bind_info(p_thisunit, src_pid));
			if (p_src_dest_vdoinfo && (src_w == 0 && src_h == 0)) {
				//auto sync w,h from dest
	    			src_w = p_src_dest_vdoinfo->imgsize.w;
	    			src_h = p_src_dest_vdoinfo->imgsize.h;
			}
			if (p_src_dest_vdoinfo && (src_imgfmt == 0)) {
				//auto sync fmt from dest
	    			src_imgfmt = p_src_dest_vdoinfo->imgfmt;
			}

			//set [i] pid;
			//set [i] enable = TRUE;
    			w = p_vdoinfo->imgsize.w;
    			h = p_vdoinfo->imgsize.h;
    			imgfmt = p_vdoinfo->imgfmt;
			if (p_dest_vdoinfo && (w == 0 && h == 0)) {
				//auto sync w,h from dest
	    			w = p_dest_vdoinfo->imgsize.w;
	    			h = p_dest_vdoinfo->imgsize.h;
			}
			if (p_dest_vdoinfo && (imgfmt == 0)) {
				//auto sync fmt from dest
	    			imgfmt = p_dest_vdoinfo->imgfmt;
			}
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "size(%d,%d) fmt=%08x",
				w, h, imgfmt);
			if (w == 0 || h == 0) {
				DBG_ERR("-out%d:size(%d,%d) is zero?\r\n",
					pid, w, h);
			}
			if (imgfmt != src_imgfmt) {
				DBG_ERR("-out%d:fmt=%08x is not equal to src out%d:fmt=%08x!\r\n", pid, imgfmt, src_pid, src_imgfmt);
				imgfmt = 0;
			}
			if(do_rotate && (p_vdoinfo->direct == VDO_DIR_ROTATE_90 || (p_vdoinfo->direct == VDO_DIR_ROTATE_270))) {
				UINT32 t = w;
				w = h;
				h = t;
			}
			if((w == src_w) && (h == src_h) && (imgfmt == src_imgfmt)) {
				do_scale = FALSE;
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "scale off");
			} else {
				do_scale = TRUE;
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "scale on: size(%d,%d) fmt=%08x => size(%d,%d) fmt=%08x",
					src_w, src_h, src_imgfmt, w, h, imgfmt);
			}

			if ((p_vdoinfo->direct & VDO_DIR_ROTATE_90) == VDO_DIR_ROTATE_90) {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "dir R: size(%d,%d) fmt=%08x => size(%d,%d) fmt=%08x",
				w, h, src_imgfmt, h, w, imgfmt);
			} else if ((p_vdoinfo->direct & VDO_DIR_ROTATE_270) == VDO_DIR_ROTATE_270) {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "dir L: size(%d,%d) fmt=%08x => size(%d,%d) fmt=%08x",
				w, h, src_imgfmt, h, w, imgfmt);
			} else if (p_vdoinfo->direct == VDO_DIR_ROTATE_180) {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "dir 180: size(%d,%d) fmt=%08x => size(%d,%d) fmt=%08x",
				w, h, src_imgfmt, w, h, imgfmt);
			} else if (p_vdoinfo->direct == VDO_DIR_MIRRORX) {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "dir X: size(%d,%d) fmt=%08x => size(%d,%d) fmt=%08x",
				w, h, src_imgfmt, w, h, imgfmt);
			} else if (p_vdoinfo->direct == VDO_DIR_MIRRORY) {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "dir Y: size(%d,%d) fmt=%08x => size(%d,%d) fmt=%08x",
				w, h, src_imgfmt, w, h, imgfmt);
			} else {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "dir off");
			}

			if(do_crop || do_scale || do_rotate) {
				if (do_crop && (do_scale==0 && do_rotate==0))
					do_scale = 1; //do crop by scale!
				isf_pipe_begin(p_pipe);
					if(do_crop)
						isf_pipe_add_scrop(p_pipe, cx, cy, cw, ch);
					if(do_scale)
						isf_pipe_add_scale(p_pipe, w, h, imgfmt, (!do_rotate) ? dest_h_align : 0);
					if(do_rotate)
						isf_pipe_add_dir(p_pipe, dir, dest_h_align);
				isf_pipe_end(p_pipe);
			}

            //fix racing (config and dispatch use global pipe)
        	loc_cpu(flags);
            memcpy((void *)&(p_ctx->out_ext[i].pipe),(void *)p_pipe,sizeof(ISF_PIPE));
        	unl_cpu(flags);
#if (USE_OUT_FRC == ENABLE)
			if (p_vdoinfo->framepersecond == VDO_FRC_DIRECT) p_vdoinfo->framepersecond = VDO_FRC_ALL;
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "frc(%d,%d)",
				GET_HI_UINT16(p_vdoinfo->framepersecond), GET_LO_UINT16(p_vdoinfo->framepersecond));
			if (p_ctx->outfrc[pid].sample_mode == ISF_SAMPLE_OFF) {
				_isf_frc_start(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]), p_vdoinfo->framepersecond);
			} else {
				_isf_frc_update_imm(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]), p_vdoinfo->framepersecond);
			}
#endif
			p_vdoinfo->dirty &= ~(ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW);    //clear dirty

			p_path->flag = 1; //start path
			p_path->oport = ISF_OUT(src_pid); //config to TURN ON
#if (USE_OUT_EXT == ENABLE)
			_isf_vdoprc_oqueue_do_start(p_thisunit, ISF_OUT(pid));
#endif
		} else {
			UINT32 w,h,imgfmt;
			UINT32 src_w,src_h,src_imgfmt;
			UINT32 cx,cy,cw,ch;
			BOOL do_crop = FALSE;
			BOOL do_scale = FALSE;
			memset((void*)p_pipe, 0, sizeof(ISF_PIPE));
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "extend=on, from (src)out%d", src_pid);
			p_pipe->cmd_count = 0; //clear

			p_src_dest_vdoinfo = (ISF_VDO_INFO *)(p_thisunit->p_base->get_bind_info(p_thisunit, src_pid));
				src_w = p_src_vdoinfo->imgsize.w;
				src_h = p_src_vdoinfo->imgsize.h;
				src_imgfmt = p_src_vdoinfo->imgfmt;
			if (p_src_dest_vdoinfo && (src_w == 0 && src_h == 0)) {
				//auto sync w,h from dest
					src_w = p_src_dest_vdoinfo->imgsize.w;
					src_h = p_src_dest_vdoinfo->imgsize.h;
			}
			if (p_src_dest_vdoinfo && (src_imgfmt == 0)) {
				//auto sync fmt from dest
					src_imgfmt = p_src_dest_vdoinfo->imgfmt;
			}

			//set [i] pid;
			//set [i] enable = TRUE;
				w = p_vdoinfo->imgsize.w;
				h = p_vdoinfo->imgsize.h;
				imgfmt = p_vdoinfo->imgfmt;
			if (p_dest_vdoinfo && (w == 0 && h == 0)) {
				//auto sync w,h from dest
					w = p_dest_vdoinfo->imgsize.w;
					h = p_dest_vdoinfo->imgsize.h;
			}
			if (p_dest_vdoinfo && (imgfmt == 0)) {
				//auto sync fmt from dest
					imgfmt = p_dest_vdoinfo->imgfmt;
			}
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "size(%d,%d) fmt=%08x",
				w, h, imgfmt);
			if (w == 0 || h == 0) {
				DBG_ERR("-out%d:size(%d,%d) is zero?\r\n",
					pid, w, h);
			}
			if (imgfmt != src_imgfmt) {
				DBG_ERR("-out%d:fmt=%08x is not equal to src out%d:fmt=%08x!\r\n", pid, imgfmt, src_pid, src_imgfmt);
				imgfmt = 0;
			}

			if ((p_src_vdoinfo->direct & VDO_DIR_ROTATE_90) == VDO_DIR_ROTATE_90) {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "dir R: size(%d,%d) fmt=%08x => size(%d,%d) fmt=%08x",
				w, h, src_imgfmt, h, w, imgfmt);
			} else if ((p_src_vdoinfo->direct & VDO_DIR_ROTATE_270) == VDO_DIR_ROTATE_270) {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "dir L: size(%d,%d) fmt=%08x => size(%d,%d) fmt=%08x",
				w, h, src_imgfmt, h, w, imgfmt);
			} else if (p_src_vdoinfo->direct == VDO_DIR_ROTATE_180) {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "dir 180: size(%d,%d) fmt=%08x => size(%d,%d) fmt=%08x",
				w, h, src_imgfmt, w, h, imgfmt);
			} else if (p_src_vdoinfo->direct == VDO_DIR_MIRRORX) {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "dir X: size(%d,%d) fmt=%08x => size(%d,%d) fmt=%08x",
				w, h, src_imgfmt, w, h, imgfmt);
			} else if (p_src_vdoinfo->direct == VDO_DIR_MIRRORY) {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "dir Y: size(%d,%d) fmt=%08x => size(%d,%d) fmt=%08x",
				w, h, src_imgfmt, w, h, imgfmt);
			} else {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "dir off");
			}

			/*
			if(do_pre_rotate) {
				UINT32 t = w;
				w = h;
				h = t;
			}
			*/

			if((w == src_w) && (h == src_h) && (imgfmt == src_imgfmt)) {
				do_scale = FALSE;
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "scale off");
			} else {
				do_scale = TRUE;
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "scale on: size(%d,%d) fmt=%08x => size(%d,%d) fmt=%08x",
					src_w, src_h, src_imgfmt, w, h, imgfmt);
			}

			//set [i] fmt = imgfmt;
			//set [i] size.w = w;
			//set [i] size.h = h;
			//set [i] lofs = ALIGN_CEIL_16(w);
			if (p_vdoinfo->window.x == 0 && p_vdoinfo->window.y == 0 && p_vdoinfo->window.w == 0 && p_vdoinfo->window.h == 0) {
				do_crop = FALSE;
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "crop off");
				//use full image size
				//set [i] crp_window.x = 0;
				//set [i] crp_window.y = 0;
				//set [i] crp_window.w = w;
				//set [i] crp_window.h = h;
				cx = 0;
				cy = 0;
				cw = 0;
				ch = 0;
			} else {
				do_crop = TRUE;
				cx = p_vdoinfo->window.x;
				cy = p_vdoinfo->window.y;
				cw = p_vdoinfo->window.w;
				ch = p_vdoinfo->window.h;
				if (p_vdoinfo->window.w == 0 || p_vdoinfo->window.h == 0) {
					DBG_ERR("-out%d:crop(%d,%d) is zero? force off!\r\n",
						pid, p_vdoinfo->window.w, p_vdoinfo->window.h);
					do_crop = FALSE;
					cx = 0;
					cy = 0;
					cw = 0;
					ch = 0;
					p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "crop off");
				}
				//set [i] crp_window = p_vdoinfo->window;
				//set [i] lofs = ALIGN_CEIL_16(p_vdoinfo->window.w);
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "crop(%d,%d,%d,%d)",
					cx, cy, cw, ch);
			}

			//here, do_pre_rotate always 1
			if(1) { // (do_pre_rotate || do_scale || do_crop) {
				if (do_crop && (do_scale==0))
					do_scale = 1; //do crop by scale!
				isf_pipe_begin(p_pipe);
					if(1) // (do_pre_rotate)
						isf_pipe_add_dir(p_pipe, pre_dir, (!do_scale) ? dest_h_align : 0);
					if(do_crop)
						isf_pipe_add_scrop(p_pipe, cx, cy, cw, ch);
					if(do_scale)
						isf_pipe_add_scale(p_pipe, w, h, imgfmt, dest_h_align);
				isf_pipe_end(p_pipe);
			}

            //fix racing (config and dispatch use global pipe)
        	loc_cpu(flags);
            memcpy((void *)&(p_ctx->out_ext[i].pipe),(void *)p_pipe,sizeof(ISF_PIPE));
        	unl_cpu(flags);
#if (USE_OUT_FRC == ENABLE)
			if (p_vdoinfo->framepersecond == VDO_FRC_DIRECT) p_vdoinfo->framepersecond = VDO_FRC_ALL;
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "frc(%d,%d)",
				GET_HI_UINT16(p_vdoinfo->framepersecond), GET_LO_UINT16(p_vdoinfo->framepersecond));
			_isf_frc_start(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]), p_vdoinfo->framepersecond);
#endif
			p_vdoinfo->dirty &= ~(ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW);	 //clear dirty

			p_path->flag = 1; //start path
			p_path->oport = ISF_OUT(src_pid); //config to TURN ON
#if (USE_OUT_EXT == ENABLE)
			_isf_vdoprc_oqueue_do_start(p_thisunit, ISF_OUT(pid));
#endif
		}
	} else {
		DBG_ERR("invalid out %d \r\n",pid);
		return;
	}
}

ISF_RV _isf_vdoprc_oport_do_dispatch_out_ext(struct _ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 wait_ms, ISF_DATA* p_tmp_rotate)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 ext_pid = oport - ISF_OUT_BASE;
	ISF_DATA dst_data = {0};
	ISF_PORT *p_port = p_thisunit->port_out[ext_pid];
	ISF_RV r;
	UINT32 ext_i = ext_pid - ISF_VDOPRC_PHY_OUT_NUM;
	ISF_PIPE tmp_pipe = {0};
	ISF_PIPE* p_pipe = &tmp_pipe ;
	unsigned long flags;

    //fix racing (config and dispatch use global pipe)
	loc_cpu(flags);
    memcpy((void *)p_pipe,(void *)&(p_ctx->out_ext[ext_i].pipe),sizeof(ISF_PIPE));
	unl_cpu(flags);

#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! do EXTEND -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, ext_pid, p_data->h_data);
	}
#endif

	if (p_data == NULL) {
		DBG_ERR("invalid p_data 3\r\n");
		return ISF_ERR_INVALID_DATA;
	}

	//init dst = src
	memcpy(&dst_data, (void*)p_data, sizeof(ISF_DATA));

	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW, ISF_ENTER);
#if (USE_OUT_FRC == ENABLE)
	//do frc
	if (_isf_frc_is_select(p_thisunit, oport, &(p_ctx->outfrc[ext_pid])) == 0) {
		//p_thisunit->p_base->do_release(p_thisunit, oport, &dst_data, 0);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW, ISF_ERR_FRC_DROP);
		return ISF_ERR_FRC_DROP;
	}
#endif
	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW, ISF_OK);
	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC, ISF_ENTER);
	//if no copy, dst = src
	//if do copy, it will release src, and dst = new
	r = isf_pipe_exec(p_thisunit, oport, p_pipe, &dst_data, p_tmp_rotate);

    //call osg and mask after exten path OK
    if ((r == ISF_ERR_IGNORE)||(r == ISF_OK)) {
		_IPP_PM_CB_INPUT_INFO in;
		_IPP_PM_CB_OUTPUT_INFO out;
		_IPP_DS_CB_INPUT_INFO in2;
		_IPP_DS_CB_OUTPUT_INFO out2;
         VDO_FRAME *p_vdoframe = 0;
        if(r == ISF_ERR_IGNORE) {
            p_vdoframe = (VDO_FRAME*)(p_data->desc);  //share src data
        }else {
            p_vdoframe = (VDO_FRAME*)(dst_data.desc); //executed dst data
        }
		in.ctl_ipp_handle = p_ctx->dev_handle;
		in.img_size = p_vdoframe->size;
		out.p_vdoframe = p_vdoframe;
		in2.ctl_ipp_handle = p_ctx->dev_handle;
		in2.img_size = p_vdoframe->size;
		out2.p_vdoframe = p_vdoframe;
		_isf_vdoprc_do_output_mask(p_thisunit, oport, &in, &out);
		_isf_vdoprc_do_output_osd(p_thisunit, oport, &in2, &out2);

	}

	if (r == ISF_ERR_IGNORE) {
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC, ISF_OK);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH, ISF_ENTER);
		//if no copy, dst = src

		//push this data to extend out (share)
		if (p_port && p_thisunit->p_base->get_is_allow_push(p_thisunit, p_port)) {
			r = _isf_vdoprc_oqueue_do_push_with_clean(p_thisunit, oport, p_data, 1);//share+BIND: keep this data
#if (FORCE_DUMP_DATA == ENABLE)
			if (oport == WATCH_PORT) {
				DBG_DUMP("%s.out[%d]! Add -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, ext_pid, p_data->h_data);
			}
#endif
			p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_EXT);
#if (FORCE_DUMP_DATA == ENABLE)
			if (oport == WATCH_PORT) {
				DBG_DUMP("%s.out[%d]! Push -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, ext_pid, p_data->h_data);
			}
#endif
			p_thisunit->p_base->do_push(p_thisunit, oport, p_data, wait_ms);
		} else {
			r = _isf_vdoprc_oqueue_do_push_with_clean(p_thisunit, oport, p_data, 1);//share+NO_BIND: keep this data
		}
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH, ISF_OK);
	} else if (r == ISF_OK) {
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC, ISF_OK);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH, ISF_ENTER);
		//if do copy, it will release temp, and dst = new (but not release src)

		 //push new data to extend out (crop + scale + rotate)
		if (p_port && p_thisunit->p_base->get_is_allow_push(p_thisunit, p_port)) {
			r = _isf_vdoprc_oqueue_do_push_with_clean(p_thisunit, oport, &dst_data, 1);//copy+BIND: keep this data
#if (FORCE_DUMP_DATA == ENABLE)
			if (oport == WATCH_PORT) {
				DBG_DUMP("%s.out[%d]! Push -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, ext_pid, (&dst_data)->h_data);
			}
#endif
			p_thisunit->p_base->do_push(p_thisunit, oport, &dst_data, wait_ms);
		} else {
			r = _isf_vdoprc_oqueue_do_push_with_clean(p_thisunit, oport, &dst_data, 1);//copy+NO_BIND: keep this data
#if (FORCE_DUMP_DATA == ENABLE)
			if (oport == WATCH_PORT) {
				DBG_DUMP("%s.out[%d]! Release -- Vdo: blk_id=%08x\r\n", p_thisunit->unit_name, ext_pid, (&dst_data)->h_data);
			}
#endif
			p_thisunit->p_base->do_release(p_thisunit, oport, &dst_data, 0);
			p_thisunit->p_base->do_probe(p_thisunit, oport, 0, ISF_OK);
		}
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH, ISF_OK);
	} else {
		if (r == ISF_ERR_NEW_FAIL) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_EXT_DROP, r);
		} else if (r == ISF_ERR_PARAM_EXCEED_LIMIT) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_EXT_WRN, r);
		} else {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_EXT_ERR, r);
		}
		//DBG_ERR("extend out: process fail!\r\n");
	}
	return ISF_OK;
}


#endif

