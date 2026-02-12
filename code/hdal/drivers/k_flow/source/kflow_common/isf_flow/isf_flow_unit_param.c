#include "isf_flow_int.h"
#include "isf_flow_api.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_flow_pp
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_flow_pp_debug_level = __DBGLVL__;
//module_param_named(isf_flow_pp_debug_level, isf_flow_pp_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_flow_pp_debug_level, "flow debug level");
///////////////////////////////////////////////////////////////////////////////

void _isf_unit_dump_imginfo(ISF_PORT *p_port)
{
#if _TODO
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *) & (p_port->info);
	DBG_DUMP("^Y    @ connecttype = 0x%02x\r\n", p_port->connecttype);
	DBG_DUMP("^Y    @ ImgInfo = fmt(%d), max(%d, %d), size(%d, %d), fps(%d, %d)"
			 " pre(%d, %d, %d, %d), post(%d, %d, %d, %d), ar(%d, %d), dir(0x%x)\r\n",
			 p_vdoinfo->imgfmt,
			 p_vdoinfo->max_imgsize.w,
			 p_vdoinfo->max_imgsize.h,
			 p_vdoinfo->imgsize.w,
			 p_vdoinfo->imgsize.h,
			 GET_HI_UINT16(p_vdoinfo->framepersecond),
			 GET_LO_UINT16(p_vdoinfo->framepersecond),
			 p_vdoinfo->prewindow.x,
			 p_vdoinfo->prewindow.y,
			 p_vdoinfo->prewindow.w,
			 p_vdoinfo->prewindow.h,
			 p_vdoinfo->postwindow.x,
			 p_vdoinfo->postwindow.y,
			 p_vdoinfo->postwindow.w,
			 p_vdoinfo->postwindow.h,
			 p_vdoinfo->imgaspect.w,
			 p_vdoinfo->imgaspect.h,
			 p_vdoinfo->direct);
#endif
	return;
}


// connecttype == ISF_CONNECT_PUSH
void _isf_unit_default_get_imginfo(ISF_PORT *p_port, ISF_VDO_INFO *p_vdoinfo)
{
#if _TODO
	ISF_VDO_INFO *p_vdoinfo2 = (ISF_VDO_INFO *) & (p_port->info);
	p_vdoinfo->imgfmt = p_vdoinfo2->imgfmt;
	p_vdoinfo->max_imgsize = p_vdoinfo2->max_imgsize;
	p_vdoinfo->imgsize = p_vdoinfo2->imgsize;
	p_vdoinfo->imgaspect = p_vdoinfo2->imgaspect;
	p_vdoinfo->prewindow = p_vdoinfo2->prewindow;
	p_vdoinfo->postwindow = p_vdoinfo2->postwindow;
#endif
	return;
}

void _isf_unit_clear_imginfo(ISF_PORT *p_port)
{
#if _TODO
	ISF_VDO_INFO *p_vdoinfo = NULL;
	if (!p_port) {
		return;
	}

	p_vdoinfo = (ISF_VDO_INFO *) & (p_port->info);
	p_vdoinfo->dirty = 0;
	p_vdoinfo->imgfmt = 0;
	p_vdoinfo->max_imgsize.w = 0;
	p_vdoinfo->max_imgsize.h = 0;
	p_vdoinfo->imgsize.w = 0;
	p_vdoinfo->imgsize.h = 0;
	p_vdoinfo->imgaspect.w = 0;
	p_vdoinfo->imgaspect.h = 0;
	RECT_Set((IRECT *) & (p_vdoinfo->prewindow), 0, 0, 0, 0);
	RECT_Set((IRECT *) & (p_vdoinfo->postwindow), 0, 0, 0, 0);
	p_vdoinfo->framepersecond = 0;
	p_vdoinfo->direct = 0;
	p_vdoinfo->buffercount = 0;
#endif
	return;
}

#define msg_q0 "\"%s\".out[%d] is null!\r\n"
#define msg_q1 "\"%s\".in[%d] is null!\r\n"

// set port parameter
ISF_RV _isf_unit_set_struct_param(UINT32 h_isf, ISF_UNIT *p_unit, UINT32 nport, UINT32 param, UINT32* p_value, UINT32 size)
{
	ISF_RV r = ISF_OK;
	if (!p_unit) {
		DBG_ERR("?.n[%d]: set struct param(%08x)={%08x}, size=%d, NO unit begin yet!\r\n",
				nport, param, (UINT32)p_value, size);
		return ISF_ERR_NULL_OBJECT;
	}
	if (nport == ISF_CTRL) {
		switch (param) {
		case ISF_UNIT_PARAM_MULTI:
			{
				ISF_SET *p_set = (ISF_SET *)(p_value);
				UINT32 n = size/sizeof(ISF_SET);
				if (p_unit->do_setportparam) {
					UINT32 i;
					for (i=0; i<n; i++) {
						p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
							"set param(%08x)=%d", p_set[i].param, p_set[i].value);
						r = p_unit->do_setportparam(p_unit, nport, p_set[i].param, p_set[i].value);
						#if 0
						if ((r > ISF_OK) && (r < ISF_APPLY_MAX)) {
							_isf_unit_applyto_dirty(r, p_unit, nport, param);
							r = ISF_OK;
						}
						#endif
					}
				} else {
					r = ISF_ERR_NOT_SUPPORT;
				}
			}
			break;
		case ISF_UNIT_PARAM_ATTR:
			{
				ISF_ATTR *p_user = (ISF_ATTR *)p_value;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"set attr(%08x)", p_user->attr);
				if ((p_user->attr >= NVTMPP_DDR_MAX) && (p_user->attr != NVTMPP_DDR_AUTO)) {
					DBG_ERR("p_user->attr = %d\r\n", p_user->attr);
					r = ISF_ERR_INVALID_PARAM;
				} else {
					p_unit->attr = p_user->attr;
				}
			}
			break;
		default:
			//call to unit to check this param
			{
				if (p_unit->do_setportstruct) {
					p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
						"set param(%08x)=%d", param, p_value[0]);
					r = p_unit->do_setportstruct(p_unit, ISF_CTRL, param, p_value, size);
					#if 0
					if ((r > ISF_OK) && (r < ISF_APPLY_MAX)) {
						_isf_unit_applyto_dirty(r, p_unit, ISF_CTRL, param);
						r = ISF_OK;
					}
					#endif
				} else {
					r = ISF_ERR_NOT_SUPPORT;
				}
			}
			break;
		}
		if(r == ISF_ERR_NOT_SUPPORT) {
			DBG_ERR(" \"%s\".ctrl: set struct param(%08x)=%d, size=%d, is not support!\r\n",
					p_unit->unit_name, param, p_value[0], size);
		}
		return r;

	} else if (ISF_IS_OPORT(p_unit, nport)) {
		ISF_PORT *p_dest = p_unit->port_out[nport - ISF_OUT_BASE];
		ISF_INFO *p_dest_info = p_unit->port_outinfo[nport - ISF_OUT_BASE];
		ISF_STATE *p_state = p_unit->port_outstate[nport - ISF_OUT_BASE];
		if ((isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) && (p_unit->list_id == 0)) {
			if((h_isf != 0xff) && (p_state->h_proc != (h_isf + 0x8000))) { //already used by other process
				DBG_ERR(" \"%s\".out[%d] cannot set struct param(%08x), size=%d! (already use by proc %d)\r\n",
					p_unit->unit_name, nport - ISF_OUT_BASE, param, size, p_state->h_proc - 0x8000);
				r = ISF_ERR_NOT_AVAIL;
				return r;
			}
		}

		switch (param) {
		case ISF_UNIT_PARAM_ATTR:
			{
				ISF_ATTR *p_user = (ISF_ATTR *)p_value;
				ISF_VDO_INFO *p_info = (ISF_VDO_INFO *)(p_dest_info);
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"set attr(%08x)", p_user->attr);
				if ((p_user->attr >= NVTMPP_DDR_MAX) && (p_user->attr != NVTMPP_DDR_AUTO)) {
					DBG_ERR("p_user->attr = %d\r\n", p_user->attr);
					r = ISF_ERR_INVALID_PARAM;
				} else {
					p_info->attr = p_user->attr;
				}
			}
			break;
		case ISF_UNIT_PARAM_MULTI:
			{
				ISF_SET *p_set = (ISF_SET *)(p_value);
				UINT32 n = size/sizeof(ISF_SET);
				if (p_unit->do_setportparam) {
					UINT32 i;
					for (i=0; i<n; i++) {
						p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
							"set param(%08x)=%d", p_set[i].param, p_set[i].value);
						r = p_unit->do_setportparam(p_unit, nport, p_set[i].param, p_set[i].value);
						#if 0
						if ((r > ISF_OK) && (r < ISF_APPLY_MAX)) {
							_isf_unit_applyto_dirty(r, p_unit, nport, param);
							r = ISF_OK;
						}
						#endif
					}
				} else {
					r = ISF_ERR_NOT_SUPPORT;
				}
			}
			break;
		case ISF_UNIT_PARAM_VDOFUNC:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
				ISF_VDO_FUNC *p_user = (ISF_VDO_FUNC *)p_value;
				UINT32 src = p_user->src;
				UINT32 func = p_user->func;

				p_vdoinfo->src = src;
				p_vdoinfo->func = func;
				p_vdoinfo->dirty |= (ISF_INFO_DIRTY_VDOFUNC);
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"set vdo-src(%d) vdo-func(%08X)", src, func);
			}
			break;
		case ISF_UNIT_PARAM_VDOMAX:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
				ISF_VDO_MAX *p_user = (ISF_VDO_MAX *)p_value;
				UINT32 max_frame = p_user->max_frame;
				UINT32 max_img_w = p_user->max_imgsize.w;
				UINT32 max_img_h = p_user->max_imgsize.h;
				UINT32 max_fmt = p_user->max_imgfmt;

				p_vdoinfo->buffercount = max_frame;
				p_vdoinfo->max_imgsize.w = max_img_w;
				p_vdoinfo->max_imgsize.h = max_img_h;
				p_vdoinfo->max_imgfmt = max_fmt;
				p_vdoinfo->dirty |= (ISF_INFO_DIRTY_VDOMAX);
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"set vdo-max-frame(%d) vdo-max-size(%d,%d) vdo-max-fmt(%08X)", max_frame, max_img_w, max_img_h, max_fmt);
			}
			break;
		case ISF_UNIT_PARAM_VDOFRAME:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
				ISF_VDO_FRAME *p_user = (ISF_VDO_FRAME *)p_value;
				UINT32 img_w = p_user->imgsize.w;
				UINT32 img_h = p_user->imgsize.h;
				UINT32 img_fmt = p_user->imgfmt;
				UINT32 img_dir = p_user->direct;

				p_vdoinfo->imgsize.w = img_w;
				p_vdoinfo->imgsize.h = img_h;
				p_vdoinfo->dirty |= ISF_INFO_DIRTY_IMGSIZE;
				p_vdoinfo->imgfmt = img_fmt;
				p_vdoinfo->dirty |= ISF_INFO_DIRTY_IMGFMT;
				p_vdoinfo->direct = img_dir;
				p_vdoinfo->dirty |= ISF_INFO_DIRTY_DIRECT;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_VDOFRM,
					"set vdo-size(%d,%d) vdo-format(%08X) vdo-dir(%d)", img_w, img_h, img_fmt, img_dir);

				//p_state->dirty |= (ISF_PORT_CMD_OUTPUT_INFO);
				//p_unit->dirty |= ISF_PORT_CMD_OUTPUT_INFO;
				if(p_dest && p_dest->p_destunit) {
					ISF_VDO_INFO *p_dest_vdoinfo = (ISF_VDO_INFO *)(p_unit->p_base->get_bind_info(p_unit, nport));
					if(p_dest_vdoinfo && (p_dest_vdoinfo->sync & (ISF_INFO_DIRTY_IMGSIZE|ISF_INFO_DIRTY_IMGFMT))) {
						p_dest_vdoinfo->dirty |= (p_vdoinfo->dirty & p_dest_vdoinfo->sync);
						DBG_MSG(" auto-sync dirty=%08x: src \"%s\".out[%d] => dest \"%s\".in[%d]!\r\n",
							(p_vdoinfo->dirty & p_dest_vdoinfo->sync), p_unit->unit_name, nport - ISF_OUT_BASE, p_dest->p_destunit->unit_name, p_dest->iport - ISF_IN_BASE);
					}
				}
			}
			break;
		case ISF_UNIT_PARAM_VDOFPS:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
				ISF_VDO_FPS *p_user = (ISF_VDO_FPS *)p_value;
				UINT32 framerate = p_user->framepersecond;

				p_vdoinfo->framepersecond = framerate;
				p_vdoinfo->dirty |= ISF_INFO_DIRTY_FRAMERATE;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_VDOFRM_IMM,
					"set vdo-framerate(%d,%d)", GET_HI_UINT16(framerate), GET_LO_UINT16(framerate));

				//p_state->dirty |= (ISF_PORT_CMD_OUTPUT_INFO);
				//p_unit->dirty |= ISF_PORT_CMD_OUTPUT_INFO;
			}
			break;
		case ISF_UNIT_PARAM_VDOWIN:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
				ISF_VDO_WIN *p_user = (ISF_VDO_WIN *)p_value;
				UINT32 win_x = p_user->window.x;
				UINT32 win_y = p_user->window.y;
				UINT32 win_w = p_user->window.w;
				UINT32 win_h = p_user->window.h;
				UINT32 img_ratio_x = p_user->imgaspect.w;
				UINT32 img_ratio_y = p_user->imgaspect.h;

				p_vdoinfo->window.x = win_x;
				p_vdoinfo->window.y = win_y;
				p_vdoinfo->window.w = win_w;
				p_vdoinfo->window.h = win_h;
				p_vdoinfo->dirty |= ISF_INFO_DIRTY_WINDOW;
				p_vdoinfo->imgaspect.w = img_ratio_x;
				p_vdoinfo->imgaspect.h = img_ratio_y;
				p_vdoinfo->dirty |= ISF_INFO_DIRTY_ASPECT;

				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_VDOFRM,
					"set vdo-winsize(%d,%d,%d,%d) vdo-aspect(%d,%d)", win_w, win_h, img_ratio_x, img_ratio_y);

				//p_state->dirty |= (ISF_PORT_CMD_OUTPUT_INFO);
				//p_unit->dirty |= ISF_PORT_CMD_OUTPUT_INFO;
			}
			break;
		case ISF_UNIT_PARAM_AUDMAX:
			{
				ISF_AUD_INFO *p_audinfo = (ISF_AUD_INFO *)(p_dest_info);
				ISF_AUD_MAX *p_user = (ISF_AUD_MAX *)p_value;
				UINT32 max_aud_bit_per_sample = p_user->max_bitpersample;
				UINT32 max_aud_channel_count = p_user->max_channelcount;
				UINT32 max_aud_sample_per_second = p_user->max_samplepersecond;
				UINT32 max_frame = p_user->max_frame;

				p_audinfo->max_bitpersample = max_aud_bit_per_sample;
				p_audinfo->max_channelcount = max_aud_channel_count;
				p_audinfo->max_samplepersecond = max_aud_sample_per_second;
				p_audinfo->buffercount = max_frame;
				p_audinfo->dirty |= (ISF_INFO_DIRTY_AUDMAX);
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"set aud-max-frame(%d) aud-max-bitpersec(%d) aud-max-sndmode(%d) aud-max-samplerate(%d,%d)",
					max_frame, max_aud_bit_per_sample, max_aud_channel_count, max_aud_sample_per_second);

				//p_state->dirty |= (ISF_PORT_CMD_OUTPUT_INFO);
				//p_unit->dirty |= ISF_PORT_CMD_OUTPUT_INFO;
			}
			break;
		case ISF_UNIT_PARAM_AUDFRAME:
			{
				ISF_AUD_INFO *p_audinfo = (ISF_AUD_INFO *)(p_dest_info);
				ISF_AUD_FRAME *p_user = (ISF_AUD_FRAME *)p_value;
				UINT32 aud_bit_per_sample = p_user->bitpersample;
				UINT32 aud_channel_count = p_user->channelcount;
				UINT32 aud_sample_count = p_user->samplecount;

				p_audinfo->bitpersample = aud_bit_per_sample;
				p_audinfo->dirty |= (ISF_INFO_DIRTY_AUDBITS);
				p_audinfo->channelcount = aud_channel_count;
				p_audinfo->dirty |= (ISF_INFO_DIRTY_AUDCH);
				p_audinfo->samplecount = aud_sample_count;
				p_audinfo->dirty |= (ISF_INFO_DIRTY_AUDSAMPLE);
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_AUDFRM,
					"set aud-bitpersec(%d) aud-sndmode(%d) samplecnt(%d)", aud_bit_per_sample, aud_channel_count, aud_sample_count);

				//p_state->dirty |= (ISF_PORT_CMD_OUTPUT_INFO);
				//p_unit->dirty |= ISF_PORT_CMD_OUTPUT_INFO;
			}
			break;
		case ISF_UNIT_PARAM_AUDFPS:
			{
				ISF_AUD_INFO *p_audinfo = (ISF_AUD_INFO *)(p_dest_info);
				ISF_AUD_FPS *p_user = (ISF_AUD_FPS *)p_value;
				UINT32 aud_sample_per_second = p_user->samplepersecond;

				p_audinfo->samplepersecond = aud_sample_per_second;
				p_audinfo->dirty |= (ISF_INFO_DIRTY_SAMPLERATE);
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_AUDFRM,
					"set aud-samplerate(%d,%d)", aud_sample_per_second);

				//p_state->dirty |= (ISF_PORT_CMD_OUTPUT_INFO);
				//p_unit->dirty |= ISF_PORT_CMD_OUTPUT_INFO;
			}
			break;
		default:
			if (p_unit->do_setportstruct) {
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"set param(%08x)=%d", param, p_value[0]);
				r = p_unit->do_setportstruct(p_unit, nport, param, p_value, size);
				#if 0
				if ((r > ISF_OK) && (r < ISF_APPLY_MAX)) {
					_isf_unit_applyto_dirty(r, p_unit, nport, param);
					r = ISF_OK;
				}
				#endif
			} else {
				r = ISF_ERR_NOT_SUPPORT;
			}
			break;
		}
		if(r == ISF_ERR_NOT_SUPPORT) {
			DBG_ERR(" \"%s\".out[%d]: set struct param(%08x)=%d, size=%d, is not support!\r\n",
				p_unit->unit_name, nport - ISF_OUT_BASE, param, p_value[0], size);
		}
		return r;

	} else if (ISF_IS_IPORT(p_unit, nport)) {
		ISF_PORT *p_src = p_unit->port_in[nport - ISF_IN_BASE];
		ISF_INFO *p_src_info = p_unit->port_ininfo[nport - ISF_IN_BASE];
		/*
		ISF_STATE* p_state = p_unit->port_outstate[nport - ISF_OUT_BASE];
		if ((isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) && (p_unit->list_id == 0)) {
			if((h_isf != 0xff) && (p_state->h_proc != (h_isf + 0x8000))) { //already used by other process
				DBG_ERR(" \"%s\".in[%d] cannot set struct param(%08x), size=%d! (already use by proc %d)\r\n",
					p_unit->unit_name, nport - ISF_IN_BASE, param, size, p_state->h_proc - 0x8000);
				r = ISF_ERR_NOT_AVAIL;
				return r;
			}
		}
		*/

		switch (param) {
		case ISF_UNIT_PARAM_ATTR:
			{
				ISF_VDO_INFO *p_info = (ISF_VDO_INFO *)(p_src_info);
				ISF_ATTR *p_user = (ISF_ATTR *)p_value;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"set attr(%08x)", p_user->attr);
				if ((p_user->attr >= NVTMPP_DDR_MAX) && (p_user->attr != NVTMPP_DDR_AUTO)) {
					DBG_ERR("p_user->attr = %d\r\n", p_user->attr);
					r = ISF_ERR_INVALID_PARAM;
				} else {
					p_info->attr = p_user->attr;
				}
			}
			break;
		case ISF_UNIT_PARAM_MULTI:
			{
				ISF_SET *p_set = (ISF_SET *)(p_value);
				UINT32 n = size/sizeof(ISF_SET);
				if (p_unit->do_setportparam) {
					UINT32 i;
					for (i=0; i<n; i++) {
						p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
							"set param(%08x)=%d", p_set[i].param, p_set[i].value);
						r = p_unit->do_setportparam(p_unit, nport, p_set[i].param, p_set[i].value);
						#if 0
						if ((r > ISF_OK) && (r < ISF_APPLY_MAX)) {
							_isf_unit_applyto_dirty(r, p_unit, nport, param);
							r = ISF_OK;
						}
						#endif
					}
				} else {
					r = ISF_ERR_NOT_SUPPORT;
				}
			}
			break;
		case ISF_UNIT_PARAM_VDOFUNC:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
				ISF_VDO_FUNC *p_user = (ISF_VDO_FUNC *)p_value;
				UINT32 src = p_user->src;
				UINT32 func = p_user->func;

				p_vdoinfo->src = src;
				p_vdoinfo->func = func;
				p_vdoinfo->dirty |= (ISF_INFO_DIRTY_VDOFUNC);
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"set vdo-src(%d) vdo-func(%08X)", src, func);
			}
			break;
		case ISF_UNIT_PARAM_VDOMAX:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
				ISF_VDO_MAX *p_user = (ISF_VDO_MAX *)p_value;
				UINT32 max_frame = p_user->max_frame;
				UINT32 max_img_w = p_user->max_imgsize.w;
				UINT32 max_img_h = p_user->max_imgsize.h;
				UINT32 max_fmt = p_user->max_imgfmt;

				p_vdoinfo->buffercount = max_frame;
				p_vdoinfo->max_imgsize.w = max_img_w;
				p_vdoinfo->max_imgsize.h = max_img_h;
				p_vdoinfo->max_imgfmt = max_fmt;
				p_vdoinfo->dirty |= (ISF_INFO_DIRTY_VDOMAX);
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"set vdo-max-frame(%d) vdo-max-size(%d,%d) vdo-max-fmt(%08X)", max_frame, max_img_w, max_img_h, max_fmt);
			}
			break;
		case ISF_UNIT_PARAM_VDOFRAME:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
				ISF_VDO_FRAME *p_user = (ISF_VDO_FRAME *)p_value;
				UINT32 img_w = p_user->imgsize.w;
				UINT32 img_h = p_user->imgsize.h;
				UINT32 img_fmt = p_user->imgfmt;
				UINT32 img_dir = p_user->direct;

				p_vdoinfo->imgsize.w = img_w;
				p_vdoinfo->imgsize.h = img_h;
				p_vdoinfo->dirty |= ISF_INFO_DIRTY_IMGSIZE;
				p_vdoinfo->imgfmt = img_fmt;
				p_vdoinfo->dirty |= ISF_INFO_DIRTY_IMGFMT;
				p_vdoinfo->direct = img_dir;
				p_vdoinfo->dirty |= ISF_INFO_DIRTY_DIRECT;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_VDOFRM,
					"set vdo-size(%d,%d) vdo-format(%08X) vdo-dir(%d)", img_w, img_h, img_fmt, img_dir);

				//p_state->dirty |= (ISF_PORT_CMD_INPUT_INFO);
				//p_unit->dirty |= ISF_PORT_CMD_INPUT_INFO;
				if(p_src && p_src->p_srcunit) {
					ISF_VDO_INFO *p_src_vdoinfo = (ISF_VDO_INFO *)(p_unit->p_base->get_bind_info(p_src->p_srcunit, p_src->oport));
					if(p_src_vdoinfo && (p_src_vdoinfo->sync & (ISF_INFO_DIRTY_IMGSIZE|ISF_INFO_DIRTY_IMGFMT))) {
						p_src_vdoinfo->dirty |= (p_vdoinfo->dirty & p_src_vdoinfo->sync);
						DBG_MSG(" auto-sync dirty=%08x: src \"%s\".out[%d] <= dest \"%s\".in[%d]!\r\n",
							(p_vdoinfo->dirty & p_src_vdoinfo->sync), p_src->p_srcunit->unit_name, p_src->oport - ISF_OUT_BASE, p_unit->unit_name, nport - ISF_IN_BASE);

					}
				}
			}
			break;
		case ISF_UNIT_PARAM_VDOFPS:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
				ISF_VDO_FPS *p_user = (ISF_VDO_FPS *)p_value;
				UINT32 framerate = p_user->framepersecond;

				p_vdoinfo->framepersecond = framerate;
				p_vdoinfo->dirty |= ISF_INFO_DIRTY_FRAMERATE;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_VDOFRM_IMM,
					"set vdo-framerate(%d,%d)", GET_HI_UINT16(framerate), GET_LO_UINT16(framerate));

				//p_state->dirty |= (ISF_PORT_CMD_INPUT_INFO);
				//p_unit->dirty |= ISF_PORT_CMD_INPUT_INFO;
			}
			break;
		case ISF_UNIT_PARAM_VDOWIN:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
				ISF_VDO_WIN *p_user = (ISF_VDO_WIN *)p_value;
				UINT32 win_x = p_user->window.x;
				UINT32 win_y = p_user->window.y;
				UINT32 win_w = p_user->window.w;
				UINT32 win_h = p_user->window.h;
				UINT32 img_ratio_x = p_user->imgaspect.w;
				UINT32 img_ratio_y = p_user->imgaspect.h;

				p_vdoinfo->window.x = win_x;
				p_vdoinfo->window.y = win_y;
				p_vdoinfo->window.w = win_w;
				p_vdoinfo->window.h = win_h;
				p_vdoinfo->dirty |= ISF_INFO_DIRTY_WINDOW;
				p_vdoinfo->imgaspect.w = img_ratio_x;
				p_vdoinfo->imgaspect.h = img_ratio_y;
				p_vdoinfo->dirty |= ISF_INFO_DIRTY_ASPECT;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_VDOFRM,
					"set vdo-winsize(%d,%d,%d,%d) vdo-aspect(%d,%d)", win_w, win_h, img_ratio_x, img_ratio_y);

				//p_state->dirty |= (ISF_PORT_CMD_INPUT_INFO);
				//p_unit->dirty |= ISF_PORT_CMD_INPUT_INFO;
			}
			break;
		case ISF_UNIT_PARAM_AUDMAX:
			{
				ISF_AUD_INFO *p_audinfo = (ISF_AUD_INFO *)(p_src_info);
				ISF_AUD_MAX *p_user = (ISF_AUD_MAX *)p_value;
				UINT32 max_frame = p_user->max_frame;
				UINT32 max_aud_bit_per_sample = p_user->max_bitpersample;
				UINT32 max_aud_channel_count = p_user->max_channelcount;
				UINT32 max_aud_sample_per_second = p_user->max_samplepersecond;

				p_audinfo->buffercount= max_frame;
				p_audinfo->max_bitpersample = max_aud_bit_per_sample;
				p_audinfo->max_channelcount = max_aud_channel_count;
				p_audinfo->max_samplepersecond = max_aud_sample_per_second;
				p_audinfo->dirty |= (ISF_INFO_DIRTY_AUDMAX);
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"set aud-max-frame(%d) aud-max-bitpersec(%d) aud-max-sndmode(%d) aud-max-samplerate(%d,%d)",
					max_frame, max_aud_bit_per_sample, max_aud_channel_count, max_aud_sample_per_second);

				//p_state->dirty |= (ISF_PORT_CMD_INPUT_INFO);
				//p_unit->dirty |= ISF_PORT_CMD_INPUT_INFO;
			}
			break;
		case ISF_UNIT_PARAM_AUDFRAME:
			{
				ISF_AUD_INFO *p_audinfo = (ISF_AUD_INFO *)(p_src_info);
				ISF_AUD_FRAME *p_user = (ISF_AUD_FRAME *)p_value;
				UINT32 aud_bit_per_sample = p_user->bitpersample;
				UINT32 aud_channel_count = p_user->channelcount;
				UINT32 aud_sample_count = p_user->samplecount;

				p_audinfo->bitpersample = aud_bit_per_sample;
				p_audinfo->dirty |= (ISF_INFO_DIRTY_AUDBITS);
				p_audinfo->channelcount = aud_channel_count;
				p_audinfo->dirty |= (ISF_INFO_DIRTY_AUDCH);
				p_audinfo->samplecount = aud_sample_count;
				p_audinfo->dirty |= (ISF_INFO_DIRTY_AUDSAMPLE);
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_AUDFRM,
					"set aud-bitpersec(%d) aud-sndmode(%d) samplecnt(%d)", aud_bit_per_sample, aud_channel_count, aud_sample_count);

				//p_state->dirty |= (ISF_PORT_CMD_INPUT_INFO);
				//p_unit->dirty |= ISF_PORT_CMD_INPUT_INFO;
			}
			break;
		case ISF_UNIT_PARAM_AUDFPS:
			{
				ISF_AUD_INFO *p_audinfo = (ISF_AUD_INFO *)(p_src_info);
				ISF_AUD_FPS *p_user = (ISF_AUD_FPS *)p_value;
				UINT32 aud_sample_per_second = p_user->samplepersecond;

				p_audinfo->samplepersecond = aud_sample_per_second;
				p_audinfo->dirty |= (ISF_INFO_DIRTY_SAMPLERATE);
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_AUDFRM,
					"set aud-samplerate(%d,%d)", aud_sample_per_second);

				//p_state->dirty |= (ISF_PORT_CMD_INPUT_INFO);
				//p_unit->dirty |= ISF_PORT_CMD_INPUT_INFO;
			}
			break;
		default:
			if (p_unit->do_setportstruct) {
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"set param(%08x)=%d", param, p_value[0]);
				r = p_unit->do_setportstruct(p_unit, nport, param, p_value, size);
				#if 0
				if ((r > ISF_OK) && (r < ISF_APPLY_MAX)) {
					_isf_unit_applyto_dirty(r, p_unit, nport, param);
					r = ISF_OK;
				}
				#endif
			} else {
				r = ISF_ERR_NOT_SUPPORT;
			}
			break;
		}
		if(r == ISF_ERR_NOT_SUPPORT) {
			DBG_ERR(" \"%s\".in[%d]: set struct param(%08x)=%d, size=%d, is not support!\r\n",
				p_unit->unit_name, nport - ISF_IN_BASE, param, p_value[0], size);
		}
		return r;
	}
	return r;
}


// set port parameter
ISF_RV _isf_unit_set_param(UINT32 h_isf, ISF_UNIT *p_unit, UINT32 nport, UINT32 param, UINT32* p_value)
{
	ISF_RV r = ISF_OK;
	if (!p_unit) {
		DBG_ERR("?.n[%d]: set param(%08x)=%d, NO unit begin yet!\r\n",
				nport, param, p_value[0]);
		return ISF_ERR_NULL_OBJECT;
	}
	if (nport == ISF_CTRL) {
		switch (param) {
		default:
			//call to unit to check this param
			if (p_unit->do_setportparam) {
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"set param(%08x)=%d", param, p_value[0]);
				r = p_unit->do_setportparam(p_unit, ISF_CTRL, param, p_value[0]);
				#if 0
				if ((r > ISF_OK) && (r < ISF_APPLY_MAX)) {
					_isf_unit_applyto_dirty(r, p_unit, ISF_CTRL, param);
					r = ISF_OK;
				}
				#endif
			} else {
				r = ISF_ERR_NOT_SUPPORT;
			}
			break;
		}
		if(r == ISF_ERR_NOT_SUPPORT) {
			DBG_ERR(" \"%s\".ctrl: set param(%08x)=%d, is not support!\r\n",
					p_unit->unit_name, param, p_value[0]);
		}
		return r;

	} else if (ISF_IS_OPORT(p_unit, nport)) {
		//ISF_PORT *p_dest = p_unit->port_out[nport - ISF_OUT_BASE];
		//ISF_INFO *p_dest_info = p_unit->port_outinfo[nport - ISF_OUT_BASE];
		ISF_STATE *p_state = p_unit->port_outstate[nport - ISF_OUT_BASE];
		if ((isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) && (p_unit->list_id == 0)) {
			if((h_isf != 0xff) && (p_state->h_proc != (h_isf + 0x8000))) { //already used by other process
				DBG_ERR(" \"%s\".out[%d] cannot set param(%08x)=%d! (already use by proc %d)\r\n",
					p_unit->unit_name, nport - ISF_OUT_BASE, param, p_value[0], p_state->h_proc - 0x8000);
				r = ISF_ERR_NOT_AVAIL;
				return r;
			}
		}

		switch (param) {
		case ISF_UNIT_PARAM_CONNECTTYPE:
			{
				//oport work if already SetOutput()!
				UINT32 connecttype = p_value[0];
				ISF_PORT *p_curdest = p_unit->port_outcfg[nport - ISF_OUT_BASE];
				ISF_PORT_CAPS *p_outcaps;
				if (!p_curdest)
				{
					DBG_ERR(" \"%s\".out[%d] is no dest for assign bind-type %08x!\r\n",
							p_unit->unit_name, nport - ISF_OUT_BASE, connecttype);
					return ISF_ERR_INVALID_PORT_ID;
				}
				p_outcaps = p_unit->port_outcaps[nport - ISF_OUT_BASE];
				if ((connecttype & ~(p_outcaps->connecttype_caps)) != 0) {
					DBG_ERR(" \"%s\".out[%d] is not support assign bind-type %08x!\r\n",
							p_unit->unit_name, nport - ISF_OUT_BASE, connecttype);
					return ISF_ERR_NOT_SUPPORT;
				}
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_BIND,
					"bind-type(%02x=>%02x)", p_curdest->connecttype, connecttype);

				p_curdest->connecttype = connecttype;
			}
			break;
		default:
			if (p_unit->do_setportparam) {
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"set param(%08x)=%d", param, p_value[0]);
				r = p_unit->do_setportparam(p_unit, nport, param, p_value[0]);
				#if 0
				if ((r > ISF_OK) && (r < ISF_APPLY_MAX)) {
					_isf_unit_applyto_dirty(r, p_unit, nport, param);
					r = ISF_OK;
				}
				#endif
			} else {
				r = ISF_ERR_NOT_SUPPORT;
			}
			break;
		}
		if(r == ISF_ERR_NOT_SUPPORT) {
			DBG_ERR(" \"%s\".out[%d]: set param(%08x)=%d, is not support!\r\n",
				p_unit->unit_name, nport - ISF_OUT_BASE, param, p_value[0]);
		}
		return r;

	} else if (ISF_IS_IPORT(p_unit, nport)) {
		//ISF_PORT *p_src = p_unit->port_in[nport - ISF_IN_BASE];
		//ISF_INFO *p_src_info = p_unit->port_ininfo[nport - ISF_IN_BASE];
		/*
		ISF_STATE *p_state = p_unit->port_outstate[nport - ISF_OUT_BASE];
		if ((isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) && (p_unit->list_id == 0)) {
			if((h_isf != 0xff) && (p_state->h_proc != (h_isf + 0x8000))) { //already used by other process
				DBG_ERR(" \"%s\".in[%d] cannot set param(%08x)=%d! (already use by proc %d)\r\n",
					p_unit->unit_name, nport - ISF_IN_BASE, param, p_value[0], p_state->h_proc - 0x8000);
				r = ISF_ERR_NOT_AVAIL;
				return r;
			}
		}
		*/

		switch (param) {
		case ISF_UNIT_PARAM_CONNECTTYPE:
			{
				//iport always work!
				UINT32 connecttype = p_value[0];
				ISF_PORT *p_cursrc = p_unit->port_in[nport - ISF_IN_BASE];
				ISF_PORT_CAPS *p_incaps = p_unit->port_incaps[nport - ISF_IN_BASE];
				if ((connecttype & ~(p_incaps->connecttype_caps)) != 0) {
					DBG_ERR(" \"%s\".in[%d] is not support assign bind-type %08x!\r\n",
							p_unit->unit_name, nport - ISF_IN_BASE, connecttype);
					return ISF_ERR_NOT_SUPPORT;
				}
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_BIND,
					"bind-type(%02x=>%02x)", p_cursrc->connecttype, connecttype);

				p_cursrc->connecttype = connecttype;
			}
			break;
		default:
			//call to unit to check this param
			if (p_unit->do_setportparam) {
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"set param(%08x)=%d", param, p_value[0]);
				r = p_unit->do_setportparam(p_unit, nport, param, p_value[0]);
				#if 0
				if ((r > ISF_OK) && (r < ISF_APPLY_MAX)) {
					_isf_unit_applyto_dirty(r, p_unit, nport, param);
					r = ISF_OK;
				}
				#endif
			} else {
				r = ISF_ERR_NOT_SUPPORT;
			}
			break;
		}
		if(r == ISF_ERR_NOT_SUPPORT) {
			DBG_ERR(" \"%s\".in[%d]: set param(%08x)=%d, is not support!\r\n",
				p_unit->unit_name, nport - ISF_IN_BASE, param, p_value[0]);
		}
		return r;
	}
	return r;
}
//EXPORT_SYMBOL(_isf_unit_set_param);

// get port parameter
ISF_RV _isf_unit_get_struct_param(UINT32 h_isf, ISF_UNIT *p_unit, UINT32 nport, UINT32 param, UINT32* p_value, UINT32 size)
{
	ISF_RV r = ISF_OK;
	if (!p_unit) {
		DBG_ERR("?.n[%d]: get struct param(%08x)={%08x}, size=%d, NO unit begin yet!\r\n",
				nport, param, (UINT32)p_value, size);
		return ISF_ERR_NULL_OBJECT;
	}
	if(!p_value)
		return ISF_ERR_NULL_POINTER;
	if (nport == ISF_CTRL) {
		switch (param) {
		case ISF_UNIT_PARAM_MULTI:
			{
				ISF_SET *p_set = (ISF_SET *)(p_value);
				UINT32 n = size/sizeof(ISF_SET);
				if (p_unit->do_getportparam) {
					UINT32 i;
					for (i=0; i<n; i++) {
						p_set[i].value = p_unit->do_getportparam(p_unit, nport, p_set[i].param);
						p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
							"get param(%08x)=%d", p_set[i].param, p_set[i].value);
						r = ISF_OK;
					}
				} else {
					r = ISF_ERR_NOT_SUPPORT;
				}
			}
			break;
		case ISF_UNIT_PARAM_ATTR:
			{
				UINT32 attr = p_unit->attr;
				p_value[0] = attr;
				r = ISF_OK;
			}
			break;
		default:
			//call to unit to check this param
			if (p_unit->do_getportstruct) {
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"get param(%08x)=@%08x,size=%d", param, (UINT32)p_value, size);
				r = p_unit->do_getportstruct(p_unit, ISF_CTRL, param, p_value, size);
			} else {
				r = ISF_ERR_NOT_SUPPORT;
			}
			break;
		}
		if(r == ISF_ERR_NOT_SUPPORT) {
			DBG_ERR(" \"%s\".ctrl: get struct param(%08x), size=%d, is not support!\r\n",
					p_unit->unit_name, param, size);
		}
		return r;

	} else if (ISF_IS_OPORT(p_unit, nport)) {
		ISF_PORT *p_dest = p_unit->port_out[nport - ISF_OUT_BASE];
		ISF_INFO *p_dest_info = p_unit->port_outinfo[nport - ISF_OUT_BASE];
		//ISF_STATE *p_state = p_unit->port_outstate[nport - ISF_OUT_BASE];
		switch (param) {
		case ISF_UNIT_PARAM_MULTI:
			{
				ISF_SET *p_set = (ISF_SET *)(p_value);
				UINT32 n = size/sizeof(ISF_SET);
				if (p_unit->do_getportparam) {
					UINT32 i;
					for (i=0; i<n; i++) {
						p_set[i].value = p_unit->do_getportparam(p_unit, nport, p_set[i].param);
						p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
							"get param(%08x)=%d", p_set[i].param, p_set[i].value);
						r = ISF_OK;
					}
				} else {
					r = ISF_ERR_NOT_SUPPORT;
				}
			}
			break;
		case ISF_UNIT_PARAM_ATTR:
			{
				UINT32 attr = p_dest->attr;
				p_value[0] = attr;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"get attr(%08x)", p_value[0]);
				r = ISF_OK;
			}
			break;
		case ISF_UNIT_PARAM_VDOFUNC:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
				ISF_VDO_FUNC *p_user = (ISF_VDO_FUNC *)p_value;
				UINT32 src = p_vdoinfo->src;
				UINT32 func = p_vdoinfo->func;

				p_user->src = src;
				p_user->func = func;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"get vdo-src(%d) vdo-func(%08X)", src, func);
			}
			break;
		case ISF_UNIT_PARAM_VDOMAX:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
				ISF_VDO_MAX *p_user = (ISF_VDO_MAX *)p_value;
				UINT32 max_frame = p_vdoinfo->buffercount;
				UINT32 max_img_w = p_vdoinfo->max_imgsize.w;
				UINT32 max_img_h = p_vdoinfo->max_imgsize.h;
				UINT32 max_fmt = p_vdoinfo->max_imgfmt;

				p_user->max_frame = max_frame;
				p_user->max_imgsize.w = max_img_w;
				p_user->max_imgsize.h = max_img_h;
				p_user->max_imgfmt = max_fmt;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"get vdo-max-frame(%d) vdo-max-size(%d,%d) vdo-max-fmt(%08X)", max_frame, max_img_w, max_img_h, max_fmt);
			}
			break;
		case ISF_UNIT_PARAM_VDOFRAME:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
				ISF_VDO_FRAME *p_user = (ISF_VDO_FRAME *)p_value;
				UINT32 img_w = p_vdoinfo->imgsize.w;
				UINT32 img_h = p_vdoinfo->imgsize.h;
				UINT32 img_fmt = p_vdoinfo->imgfmt;
				UINT32 img_dir = p_vdoinfo->direct;

				p_user->imgsize.w = img_w;
				p_user->imgsize.h = img_h;
				p_user->imgfmt = img_fmt;
				p_user->direct = img_dir;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_VDOFRM,
					"get vdo-size(%d,%d) vdo-format(%08X) vdo-dir(%d)", img_w, img_h, img_fmt, img_dir);
			}
			break;
		case ISF_UNIT_PARAM_VDOFPS:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
				ISF_VDO_FPS *p_user = (ISF_VDO_FPS *)p_value;
				UINT32 framerate = p_vdoinfo->framepersecond;

				p_user->framepersecond = framerate;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_VDOFRM_IMM,
					"get vdo-framerate(%d,%d)", GET_HI_UINT16(framerate), GET_LO_UINT16(framerate));
			}
			break;
		case ISF_UNIT_PARAM_VDOWIN:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
				ISF_VDO_WIN *p_user = (ISF_VDO_WIN *)p_value;
				UINT32 win_w = p_vdoinfo->window.w;
				UINT32 win_h = p_vdoinfo->window.h;
				UINT32 img_ratio_x = p_vdoinfo->imgaspect.w;
				UINT32 img_ratio_y = p_vdoinfo->imgaspect.h;

				p_user->window.w = win_w;
				p_user->window.h = win_h;
				p_user->imgaspect.w = img_ratio_x;
				p_user->imgaspect.h = img_ratio_y;

				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_VDOFRM,
					"get vdo-winsize(%d,%d,%d,%d) vdo-aspect(%d,%d)", win_w, win_h, img_ratio_x, img_ratio_y);
			}
			break;
		case ISF_UNIT_PARAM_AUDMAX:
			{
				ISF_AUD_INFO *p_audinfo = (ISF_AUD_INFO *)(p_dest_info);
				ISF_AUD_MAX *p_user = (ISF_AUD_MAX *)p_value;
				UINT32 max_aud_bit_per_sample = p_audinfo->max_bitpersample;
				UINT32 max_aud_channel_count = p_audinfo->max_channelcount;
				UINT32 max_aud_sample_per_second = p_audinfo->max_samplepersecond;
				UINT32 max_frame = p_audinfo->buffercount;

				p_user->max_bitpersample = max_aud_bit_per_sample;
				p_user->max_channelcount = max_aud_channel_count;
				p_user->max_samplepersecond = max_aud_sample_per_second;
				p_user->max_frame = max_frame;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"get aud-max-frame(%d) aud-max-bitpersec(%d) aud-max-sndmode(%d) aud-max-samplerate(%d,%d)",
					max_frame, max_aud_bit_per_sample, max_aud_channel_count, max_aud_sample_per_second);
			}
			break;
		case ISF_UNIT_PARAM_AUDFRAME:
			{
				ISF_AUD_INFO *p_audinfo = (ISF_AUD_INFO *)(p_dest_info);
				ISF_AUD_FRAME *p_user = (ISF_AUD_FRAME *)p_value;
				UINT32 aud_bit_per_sample = p_audinfo->bitpersample;
				UINT32 aud_channel_count = p_audinfo->channelcount;

				p_user->bitpersample = aud_bit_per_sample;
				p_user->channelcount = aud_channel_count;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_AUDFRM,
					"get aud-bitpersec(%d) aud-sndmode(%d)", aud_bit_per_sample, aud_channel_count);
			}
			break;
		case ISF_UNIT_PARAM_AUDFPS:
			{
				ISF_AUD_INFO *p_audinfo = (ISF_AUD_INFO *)(p_dest_info);
				ISF_AUD_FPS *p_user = (ISF_AUD_FPS *)p_value;
				UINT32 aud_sample_per_second = p_audinfo->samplepersecond;

				p_user->samplepersecond = aud_sample_per_second;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_AUDFRM,
					"get aud-samplerate(%d,%d)", aud_sample_per_second);
			}
			break;
		default:
			if (p_unit->do_getportstruct) {
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"get param(%08x)=@%08x", param, (UINT32)p_value, size);
				r = p_unit->do_getportstruct(p_unit, nport, param, p_value, size);
			} else {
				r = ISF_ERR_NOT_SUPPORT;
			}
			break;
		}
		if(r == ISF_ERR_NOT_SUPPORT) {
			DBG_ERR(" \"%s\".out[%d]: get struct param(%08x), size=%d, is not support!\r\n",
				p_unit->unit_name, nport - ISF_OUT_BASE, param, size);
		}
		return r;

	} else if (ISF_IS_IPORT(p_unit, nport)) {
		ISF_PORT *p_src = p_unit->port_in[nport - ISF_IN_BASE];
		ISF_INFO *p_src_info = p_unit->port_ininfo[nport - ISF_IN_BASE];
		//ISF_STATE* p_state = p_unit->port_outstate[nport-ISF_OUT_BASE];
		switch (param) {
		case ISF_UNIT_PARAM_MULTI:
			{
				ISF_SET *p_set = (ISF_SET *)(p_value);
				UINT32 n = size/sizeof(ISF_SET);
				if (p_unit->do_getportparam) {
					UINT32 i;
					for (i=0; i<n; i++) {
						p_set[i].value = p_unit->do_getportparam(p_unit, nport, p_set[i].param);
						p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
							"get param(%08x)=%d", p_set[i].param, p_set[i].value);
						r = ISF_OK;
					}
				} else {
					r = ISF_ERR_NOT_SUPPORT;
				}
			}
			break;
		case ISF_UNIT_PARAM_ATTR:
			{
				UINT32 attr = p_src->attr;
				p_value[0] = attr;

				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_AUDFRM,
					"get attr(%08x)", attr);
				r = ISF_OK;
			}
			break;
		case ISF_UNIT_PARAM_VDOFUNC:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
				ISF_VDO_FUNC *p_user = (ISF_VDO_FUNC *)p_value;
				UINT32 src = p_vdoinfo->src;
				UINT32 func = p_vdoinfo->func;

				p_user->src = src;
				p_user->func = func;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"get vdo-src(%d) vdo-func(%08X)", src, func);
			}
			break;
		case ISF_UNIT_PARAM_VDOMAX:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
				ISF_VDO_MAX *p_user = (ISF_VDO_MAX *)p_value;
				UINT32 max_frame = p_vdoinfo->buffercount;
				UINT32 max_img_w = p_vdoinfo->max_imgsize.w;
				UINT32 max_img_h = p_vdoinfo->max_imgsize.h;
				UINT32 max_fmt = p_vdoinfo->max_imgfmt;

				p_user->max_frame = max_frame;
				p_user->max_imgsize.w = max_img_w;
				p_user->max_imgsize.h = max_img_h;
				p_user->max_imgfmt = max_fmt;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"get vdo-max-frame(%d) vdo-max-size(%d,%d) vdo-maxfmt(%08X)", max_frame, max_img_w, max_img_h, max_fmt);
			}
			break;
		case ISF_UNIT_PARAM_VDOFRAME:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
				ISF_VDO_FRAME *p_user = (ISF_VDO_FRAME *)p_value;
				UINT32 img_w = p_vdoinfo->imgsize.w;
				UINT32 img_h = p_vdoinfo->imgsize.h;
				UINT32 img_fmt = p_vdoinfo->imgfmt;
				UINT32 img_dir = p_vdoinfo->direct;

				p_user->imgsize.w = img_w;
				p_user->imgsize.h = img_h;
				p_user->imgfmt = img_fmt;
				p_user->direct = img_dir;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_VDOFRM,
					"get vdo-size(%d,%d) vdo-format(%08X) vdo-dir(%d)", img_w, img_h, img_fmt, img_dir);
			}
			break;
		case ISF_UNIT_PARAM_VDOFPS:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
				ISF_VDO_FPS *p_user = (ISF_VDO_FPS *)p_value;
				UINT32 framerate = p_vdoinfo->framepersecond;

				p_user->framepersecond = framerate;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_VDOFRM_IMM,
					"get vdo-framerate(%d,%d)", GET_HI_UINT16(framerate), GET_LO_UINT16(framerate));
			}
			break;
		case ISF_UNIT_PARAM_VDOWIN:
			{
				ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
				ISF_VDO_WIN *p_user = (ISF_VDO_WIN *)p_value;
				UINT32 win_w = p_vdoinfo->window.w;
				UINT32 win_h = p_vdoinfo->window.h;
				UINT32 img_ratio_x = p_vdoinfo->imgaspect.w;
				UINT32 img_ratio_y = p_vdoinfo->imgaspect.h;

				p_user->window.w = win_w;
				p_user->window.h = win_h;
				p_user->imgaspect.w = img_ratio_x;
				p_user->imgaspect.h = img_ratio_y;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_VDOFRM,
					"get vdo-winsize(%d,%d,%d,%d) vdo-aspect(%d,%d)", win_w, win_h, img_ratio_x, img_ratio_y);
			}
			break;
		case ISF_UNIT_PARAM_AUDMAX:
			{
				ISF_AUD_INFO *p_audinfo = (ISF_AUD_INFO *)(p_src_info);
				ISF_AUD_MAX *p_user = (ISF_AUD_MAX *)p_value;
				UINT32 max_frame = p_audinfo->buffercount;
				UINT32 max_aud_bit_per_sample = p_audinfo->max_bitpersample;
				UINT32 max_aud_channel_count = p_audinfo->max_channelcount;
				UINT32 max_aud_sample_per_second = p_audinfo->max_samplepersecond;

				p_user->max_frame= max_frame;
				p_user->max_bitpersample = max_aud_bit_per_sample;
				p_user->max_channelcount = max_aud_channel_count;
				p_user->max_samplepersecond = max_aud_sample_per_second;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"get aud-max-frame(%d) aud-max-bitpersec(%d) aud-max-sndmode(%d) aud-max-samplerate(%d,%d)",
					max_frame, max_aud_bit_per_sample, max_aud_channel_count, max_aud_sample_per_second);
			}
			break;
		case ISF_UNIT_PARAM_AUDFRAME:
			{
				ISF_AUD_INFO *p_audinfo = (ISF_AUD_INFO *)(p_src_info);
				ISF_AUD_FRAME *p_user = (ISF_AUD_FRAME *)p_value;
				UINT32 aud_bit_per_sample = p_audinfo->bitpersample;
				UINT32 aud_channel_count = p_audinfo->channelcount;

				p_user->bitpersample = aud_bit_per_sample;
				p_user->channelcount = aud_channel_count;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_AUDFRM,
					"get aud-bitpersec(%d) aud-sndmode(%d)", aud_bit_per_sample, aud_channel_count);
			}
			break;
		case ISF_UNIT_PARAM_AUDFPS:
			{
				ISF_AUD_INFO *p_audinfo = (ISF_AUD_INFO *)(p_src_info);
				ISF_AUD_FPS *p_user = (ISF_AUD_FPS *)p_value;
				UINT32 aud_sample_per_second = p_audinfo->samplepersecond;

				p_user->samplepersecond = aud_sample_per_second;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_AUDFRM,
					"get aud-samplerate(%d,%d)", aud_sample_per_second);
			}
			break;
		default:
			if (p_unit->do_getportstruct) {
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"get param(%08x)=@%d08x,size=%d", param, (UINT32)p_value, size);
				r = p_unit->do_getportstruct(p_unit, nport, param, p_value, size);
			} else {
				r = ISF_ERR_NOT_SUPPORT;
			}
			break;
		}
		if(r == ISF_ERR_NOT_SUPPORT) {
			DBG_ERR(" \"%s\".in[%d]: get struct param(%08x), size=%d, is not support!\r\n",
				p_unit->unit_name, nport - ISF_IN_BASE, param, size);
		}
		return r;
	}
	return ISF_OK;
}

// get port parameter
ISF_RV _isf_unit_get_param(UINT32 h_isf, ISF_UNIT *p_unit, UINT32 nport, UINT32 param, UINT32* p_value)
{
	ISF_RV r = ISF_OK;
	if (!p_unit) {
		DBG_ERR("?.n[%d]: get param(%08x)=%d, NO unit begin yet!\r\n",
				nport, param, p_value[0]);
		return ISF_ERR_NULL_OBJECT;
	}
	if(!p_value)
		return ISF_ERR_NULL_POINTER;
	if (nport == ISF_CTRL) {
		switch (param) {
		default:
			//call to unit to check this param
			if (p_unit->do_getportparam) {
				p_value[0] = p_unit->do_getportparam(p_unit, ISF_CTRL, param);
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"get param(%08x)=%d", param, p_value[0]);
				r = ISF_OK;
			} else {
				r = ISF_ERR_NOT_SUPPORT;
			}
			break;
		}
		if(r == ISF_ERR_NOT_SUPPORT) {
			DBG_ERR(" \"%s\".ctrl: get param(%08x), is not support!\r\n",
					p_unit->unit_name, param);
		}
		return r;

	} else if (ISF_IS_OPORT(p_unit, nport)) {
		//ISF_PORT *p_dest = p_unit->port_out[nport - ISF_OUT_BASE];
		//ISF_INFO *p_dest_info = p_unit->port_outinfo[nport - ISF_OUT_BASE];
		//ISF_STATE *p_state = p_unit->port_outstate[nport - ISF_OUT_BASE];
		switch (param) {
		case ISF_UNIT_PARAM_CONNECTTYPE:
			{
				//oport work if already SetOutput()!
				ISF_PORT *p_curdest = p_unit->port_outcfg[nport - ISF_OUT_BASE];
				UINT32 connecttype;
				ISF_PORT_CAPS *p_outcaps;
				if (!p_curdest)
				{
					DBG_ERR(" \"%s\".out[%d] is no dest for query bind-type!\r\n",
							p_unit->unit_name, nport - ISF_OUT_BASE);
					return ISF_ERR_INVALID_PORT_ID;
				}
				connecttype = p_curdest->connecttype;
				p_outcaps = p_unit->port_outcaps[nport - ISF_OUT_BASE];
				if ((connecttype & ~(p_outcaps->connecttype_caps)) != 0) {
					DBG_ERR(" \"%s\".out[%d] is not support query bind-type!\r\n",
							p_unit->unit_name, nport - ISF_OUT_BASE);
					return ISF_ERR_NOT_SUPPORT;
				}
				p_value[0] = connecttype;
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_BIND,
					"bind-type(%02x)", p_value[0]);
				r = ISF_OK;
			}
			break;
		default:
			if (p_unit->do_getportparam) {
				p_value[0] = p_unit->do_getportparam(p_unit, nport, param);
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"get param(%08x)=%d", param, p_value[0]);
				r = ISF_OK;
			} else {
				r = ISF_ERR_NOT_SUPPORT;
			}
			break;
		}
		if(r == ISF_ERR_NOT_SUPPORT) {
			DBG_ERR(" \"%s\".out[%d]: get param(%08x), is not support!\r\n",
				p_unit->unit_name, nport - ISF_OUT_BASE, param);
		}
		return r;

	} else if (ISF_IS_IPORT(p_unit, nport)) {
		//ISF_PORT *p_src = p_unit->port_in[nport - ISF_IN_BASE];
		//ISF_INFO *p_src_info = p_unit->port_ininfo[nport - ISF_IN_BASE];
		//ISF_STATE* p_state = p_unit->port_outstate[nport-ISF_OUT_BASE];
		switch (param) {
		case ISF_UNIT_PARAM_CONNECTTYPE:
			{
				//iport always work!
				ISF_PORT *p_cursrc = p_unit->port_in[nport - ISF_IN_BASE];
				UINT32 connecttype = p_cursrc->connecttype;
				ISF_PORT_CAPS *p_incaps = p_unit->port_incaps[nport - ISF_IN_BASE];
				if ((connecttype & ~(p_incaps->connecttype_caps)) != 0) {
					DBG_ERR(" \"%s\".in[%d] is not support query bind-type %08x!\r\n",
							p_unit->unit_name, nport - ISF_IN_BASE, connecttype);
					return ISF_ERR_NOT_SUPPORT;
				}
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_BIND,
					"bind-type(%02x=>%02x)", p_cursrc->connecttype, connecttype);

				p_value[0] = connecttype;
				r = ISF_OK;
			}
			break;
		default:
			//call to unit to check this param
			if (p_unit->do_getportparam) {
				p_value[0] = p_unit->do_getportparam(p_unit, nport, param);
				p_unit->p_base->do_debug(p_unit, nport, ISF_OP_PARAM_GENERAL,
					"get param(%08x)=%d", param, p_value[0]);
				r = ISF_OK;
			} else {
				r = ISF_ERR_NOT_SUPPORT;
			}
			break;
		}
		if(r == ISF_ERR_NOT_SUPPORT) {
			DBG_ERR(" \"%s\".in[%d]: get param(%08x), is not support!\r\n",
				p_unit->unit_name, nport - ISF_IN_BASE, param);
		}
		return r;
	}
	return ISF_OK;
}
//EXPORT_SYMBOL(_isf_unit_get_param);

