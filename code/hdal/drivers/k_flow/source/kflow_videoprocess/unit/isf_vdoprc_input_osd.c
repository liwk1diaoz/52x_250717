#include "isf_vdoprc_int.h"
#if (USE_VDS == ENABLE)
#include "videosprite/videosprite_open.h"
#include "videosprite/videosprite_ime.h"
#endif
#include "kdrv_videoprocess/kdrv_ime_lmt.h"
#include "comm/hwclock.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoprc_io
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoprc_io_debug_level = NVT_DBG_WRN;
///////////////////////////////////////////////////////////////////////////////

#if (USE_VDS == ENABLE)
static void transform_palette(UINT8 *a, UINT8 *r, UINT8 *g, UINT8 *b, UINT32 *src, int num)
{
	int i;
	UINT32 c;
	
	if(!a || !r || !g || !b || !src){
		DBG_ERR("a(%p) r(%p) g(%p) b(%p) src(%p) is NULL\n", a, r, b, g, src);
		return ;
	}
		
	vds_memset(a, 0, sizeof(UINT8) * num);
	vds_memset(r, 0, sizeof(UINT8) * num);
	vds_memset(g, 0, sizeof(UINT8) * num);
	vds_memset(b, 0, sizeof(UINT8) * num);
		
	for(i = 0 ; i < num ; ++i){
		c    = src[i];
		a[i] = ((c & 0xFF000000) >> 24);
		r[i] = ((c & 0x00FF0000) >> 16);
		g[i] = ((c & 0x0000FF00) >> 8);
		b[i] = ((c & 0x000000FF));
	}
}
#endif

void _isf_vdoprc_do_input_osd(ISF_UNIT *p_thisunit, UINT32 iport, void *in, void *out)
{
	#if (USE_VDS == ENABLE)
	int i;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);  
	UINT32 dev_id = p_ctx->dev;
	//CTL_IPP_DS_CB_INPUT_INFO *p_in = (CTL_IPP_DS_CB_INPUT_INFO *)in;
	CTL_IPP_DS_CB_OUTPUT_INFO *p_out = (CTL_IPP_DS_CB_OUTPUT_INFO *)out;
	UINT64 start = hwclock_get_longcounter();

	VDS_TO_IME_STAMP *p_vs_ctx = vds_lock_context(VDS_QS_IME_STAMP, dev_id, NULL);
	
	if(!p_vs_ctx || !p_vs_ctx->dirty) {
		p_out->stamp[0].func_en = FALSE;
		p_out->stamp[1].func_en = FALSE;
		p_out->stamp[2].func_en = FALSE;
		p_out->stamp[3].func_en = FALSE;
	}
	else{
		p_out->cst_info.func_en      = 1;
		p_out->cst_info.auto_mode_en = 1;
		for(i = 0 ; i < vds_max_ime_stamp && i < CTL_IPP_DS_SET_ID_MAX ; ++i){
			p_out->stamp[i].func_en                  = p_vs_ctx->data[i].en;
			if(p_out->stamp[i].func_en){
				p_out->stamp[i].img_info.size.w          = p_vs_ctx->data[i].size.w;
				p_out->stamp[i].img_info.size.h          = p_vs_ctx->data[i].size.h;
				p_out->stamp[i].img_info.pos.x           = p_vs_ctx->data[i].pos.x;
				p_out->stamp[i].img_info.pos.y           = p_vs_ctx->data[i].pos.y;
				p_out->stamp[i].img_info.addr            = p_vs_ctx->data[i].addr;
				p_out->stamp[i].iq_info.color_key_en     = p_vs_ctx->data[i].ckey_en;
				p_out->stamp[i].iq_info.color_key_val    = p_vs_ctx->data[i].ckey_val;
				p_out->stamp[i].iq_info.bld_wt_0         = p_vs_ctx->data[i].bweight0;
				p_out->stamp[i].iq_info.bld_wt_1         = p_vs_ctx->data[i].bweight1;
				
				if(p_vs_ctx->data[i].fmt == VDO_PXLFMT_I1){
					p_out->stamp[i].img_info.fmt   = VDO_PXLFMT_ARGB4444;
					p_out->stamp[i].img_info.lofs  = (p_vs_ctx->data[i].size.w >> 3);
					p_out->stamp[i].iq_info.plt_en = 1;
					p_out->plt_info.mode = CTL_IPP_DS_PLT_MODE_1BIT;
					transform_palette(p_out->plt_info.plt_a, p_out->plt_info.plt_r, 
										p_out->plt_info.plt_g, p_out->plt_info.plt_b,
										vds_get_ime_palette(dev_id), 16);
				}else if(p_vs_ctx->data[i].fmt == VDO_PXLFMT_I2){
					p_out->stamp[i].img_info.fmt   = VDO_PXLFMT_ARGB4444;
					p_out->stamp[i].img_info.lofs  = (p_vs_ctx->data[i].size.w >> 2);
					p_out->stamp[i].iq_info.plt_en = 1;
					p_out->plt_info.mode = CTL_IPP_DS_PLT_MODE_2BIT;
					transform_palette(p_out->plt_info.plt_a, p_out->plt_info.plt_r, 
										p_out->plt_info.plt_g, p_out->plt_info.plt_b,
										vds_get_ime_palette(dev_id), 16);
				}else if(p_vs_ctx->data[i].fmt == VDO_PXLFMT_I4){
					p_out->stamp[i].img_info.fmt   = VDO_PXLFMT_ARGB4444;
					p_out->stamp[i].img_info.lofs  = (p_vs_ctx->data[i].size.w >> 1);
					p_out->stamp[i].iq_info.plt_en = 1;
					p_out->plt_info.mode = CTL_IPP_DS_PLT_MODE_4BIT;
					transform_palette(p_out->plt_info.plt_a, p_out->plt_info.plt_r, 
										p_out->plt_info.plt_g, p_out->plt_info.plt_b,
										vds_get_ime_palette(dev_id), 16);
				}else{ //rgb565/argb1555/argb4444
					p_out->stamp[i].img_info.fmt   = p_vs_ctx->data[i].fmt;
					if (p_out->stamp[i].img_info.fmt == VDO_PXLFMT_ARGB8888) {
						p_out->stamp[i].img_info.lofs  = (p_vs_ctx->data[i].size.w * 4);
					} else {
						p_out->stamp[i].img_info.lofs  = (p_vs_ctx->data[i].size.w * 2);
					}
					p_out->stamp[i].iq_info.plt_en = 0;
					if(p_out->stamp[i].iq_info.color_key_en){
						p_out->stamp[i].iq_info.color_key_mode = CTL_IPP_DS_CKEY_MODE_ARGB;
						if(p_out->stamp[i].img_info.fmt == VDO_PXLFMT_ARGB1555)
							p_out->stamp[i].iq_info.color_key_val = (
								((p_vs_ctx->data[i].ckey_val & 0x8000) << 9) |
								((p_vs_ctx->data[i].ckey_val & 0x7C00) << 6)  |
								((p_vs_ctx->data[i].ckey_val & 0x03E0) << 3)  |
								((p_vs_ctx->data[i].ckey_val & 0x001F)));
						else if(p_out->stamp[i].img_info.fmt == VDO_PXLFMT_ARGB4444)
							p_out->stamp[i].iq_info.color_key_val = (
								((p_vs_ctx->data[i].ckey_val & 0xF000) << 12) |
								((p_vs_ctx->data[i].ckey_val & 0x0F00) << 8)  |
								((p_vs_ctx->data[i].ckey_val & 0x00F0) << 4)  |
								((p_vs_ctx->data[i].ckey_val & 0x000F)));
						else if(p_out->stamp[i].img_info.fmt == VDO_PXLFMT_ARGB8888)
							p_out->stamp[i].iq_info.color_key_val = (
								(p_vs_ctx->data[i].ckey_val & 0xFF000000) |
								(p_vs_ctx->data[i].ckey_val & 0x00FF0000) |
								(p_vs_ctx->data[i].ckey_val & 0x0000FF00) |
								(p_vs_ctx->data[i].ckey_val & 0x000000FF));
					}
				}

				//limit check : w
				if(p_out->stamp[i].img_info.size.w < IME_OSDBUF_WMIN){
					DBG_ERR("videoprocess osd w(%d) < min(%d)\n",
								p_out->stamp[i].img_info.size.w, IME_OSDBUF_WMIN);
					vds_memset(&(p_out->stamp[i]), 0, sizeof(CTL_IPP_DS_INFO));
				}
				if(p_out->stamp[i].img_info.size.w > IME_OSDBUF_WMAX){
					DBG_ERR("videoprocess osd w(%d) > max(%d)\n",
								p_out->stamp[i].img_info.size.w, IME_OSDBUF_WMAX);
					vds_memset(&(p_out->stamp[i]), 0, sizeof(CTL_IPP_DS_INFO));
				}
				if(IME_OSDBUF_WALIGN && (p_out->stamp[i].img_info.size.w & (IME_OSDBUF_WALIGN - 1))){
					DBG_ERR("videoprocess osd w(%d) is not %d aligned\n",
								p_out->stamp[i].img_info.size.w, IME_OSDBUF_WALIGN);
					vds_memset(&(p_out->stamp[i]), 0, sizeof(CTL_IPP_DS_INFO));
				}

				//limit check : h
				if(p_out->stamp[i].img_info.size.h < IME_OSDBUF_HMIN){
					DBG_ERR("videoprocess osd h(%d) < min(%d)\n",
								p_out->stamp[i].img_info.size.h, IME_OSDBUF_HMIN);
					vds_memset(&(p_out->stamp[i]), 0, sizeof(CTL_IPP_DS_INFO));
				}
				if(p_out->stamp[i].img_info.size.h > IME_OSDBUF_HMAX){
					DBG_ERR("videoprocess osd h(%d) > max(%d)\n",
								p_out->stamp[i].img_info.size.h, IME_OSDBUF_HMAX);
					vds_memset(&(p_out->stamp[i]), 0, sizeof(CTL_IPP_DS_INFO));
				}
				if(IME_OSDBUF_HALIGN && (p_out->stamp[i].img_info.size.h & (IME_OSDBUF_HALIGN - 1))){
					DBG_ERR("videoprocess osd h(%d) is not %d aligned\n",
								p_out->stamp[i].img_info.size.h, IME_OSDBUF_HALIGN);
					vds_memset(&(p_out->stamp[i]), 0, sizeof(CTL_IPP_DS_INFO));
				}

				//limit check : buffer
				if(IME_OSDBUF_LOFFALIGN && (p_out->stamp[i].img_info.lofs & (IME_OSDBUF_LOFFALIGN - 1))){
					DBG_ERR("videoprocess osd lineoffset(%d) is not %d aligned\n",
								p_out->stamp[i].img_info.lofs, IME_OSDBUF_LOFFALIGN);
					vds_memset(&(p_out->stamp[i]), 0, sizeof(CTL_IPP_DS_INFO));
				}
				if(IME_OSDBUF_ADDRALIGN && (p_out->stamp[i].img_info.addr & (IME_OSDBUF_ADDRALIGN - 1))){
					DBG_ERR("videoprocess osd buf addr(0x%x) is not %d aligned\n",
								p_out->stamp[i].img_info.addr, IME_OSDBUF_ADDRALIGN);
					vds_memset(&(p_out->stamp[i]), 0, sizeof(CTL_IPP_DS_INFO));
				}
			}else
				vds_memset(&(p_out->stamp[i]), 0, sizeof(CTL_IPP_DS_INFO));
		}
		
	 	p_vs_ctx->dirty = 0;
	}
	
	//YongChang : move vds_unlock_context() to _isf_vdoprc_finish_input_osd()
	vds_save_latency(VDS_QS_IME_STAMP, dev_id, 0, hwclock_get_longcounter() - start);
	vds_unlock_context(VDS_QS_IME_STAMP, iport - ISF_IN_BASE, 0);
	#endif
}

void _isf_vdoprc_finish_input_osd(ISF_UNIT *p_thisunit, UINT32 iport)
{
}


