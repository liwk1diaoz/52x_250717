#include "isf_vdoprc_int.h"
#if (USE_VDS == ENABLE)
#include "videosprite/videosprite_open.h"
#include "videosprite/videosprite_ime.h"
#endif
#include "comm/hwclock.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoprc_im
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoprc_im_debug_level = NVT_DBG_WRN;
///////////////////////////////////////////////////////////////////////////////

void _isf_vdoprc_do_input_mask(ISF_UNIT *p_thisunit, UINT32 iport, void *in, void *out)
{
	#if (USE_VDS == ENABLE)
	int i;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);  
	UINT32 dev_id = p_ctx->dev, flag;
	//CTL_IPP_PM_CB_INPUT_INFO *p_in = (CTL_IPP_PM_CB_INPUT_INFO *)in;
	CTL_IPP_PM_CB_OUTPUT_INFO *p_out = (CTL_IPP_PM_CB_OUTPUT_INFO *)out;
	UINT64 start = hwclock_get_longcounter();

	VDS_TO_IME_MASK *p_vs_ctx = vds_lock_context(VDS_QS_IME_MASK, dev_id, &flag);
	
	vds_memset(p_out, 0, sizeof(CTL_IPP_PM_CB_OUTPUT_INFO));
	if(p_vs_ctx && p_vs_ctx->dirty) {
	
		for(i = 0 ; i < vds_max_ime_mask && i < CTL_IPP_PM_SET_ID_MAX ; ++i){
		
			if(!p_vs_ctx->data[i].en)
				continue;
		
			if(p_vs_ctx->data[i].is_mosaic){
			
				if(p_vs_ctx->data[i].mosaic_blk_size == 8)
					p_out->pxl_blk_size = CTL_IPP_PM_PXL_BLK_08;
				else if(p_vs_ctx->data[i].mosaic_blk_size == 16)
					p_out->pxl_blk_size = CTL_IPP_PM_PXL_BLK_16;
				else if(p_vs_ctx->data[i].mosaic_blk_size == 32)
					p_out->pxl_blk_size = CTL_IPP_PM_PXL_BLK_32;
				else if(p_vs_ctx->data[i].mosaic_blk_size == 64)
					p_out->pxl_blk_size = CTL_IPP_PM_PXL_BLK_64;
					
				p_out->mask[i].func_en          = p_vs_ctx->data[i].en;
				p_out->mask[i].msk_type         = CTL_IPP_PM_MASK_TYPE_PXL;
				p_out->mask[i].pm_coord[0].x    = p_vs_ctx->data[i].pos[0].x;
				p_out->mask[i].pm_coord[0].y    = p_vs_ctx->data[i].pos[0].y;
				p_out->mask[i].pm_coord[1].x    = p_vs_ctx->data[i].pos[1].x;
				p_out->mask[i].pm_coord[1].y    = p_vs_ctx->data[i].pos[1].y;
				p_out->mask[i].pm_coord[2].x    = p_vs_ctx->data[i].pos[2].x;
				p_out->mask[i].pm_coord[2].y    = p_vs_ctx->data[i].pos[2].y;
				p_out->mask[i].pm_coord[3].x    = p_vs_ctx->data[i].pos[3].x;
				p_out->mask[i].pm_coord[3].y    = p_vs_ctx->data[i].pos[3].y;
				p_out->mask[i].alpha_weight     = p_vs_ctx->data[i].alpha;
			}else{
				p_out->mask[i].func_en          = p_vs_ctx->data[i].en;
				p_out->mask[i].msk_type         = CTL_IPP_PM_MASK_TYPE_YUV;
				p_out->mask[i].pm_coord[0].x    = p_vs_ctx->data[i].pos[0].x;
				p_out->mask[i].pm_coord[0].y    = p_vs_ctx->data[i].pos[0].y;
				p_out->mask[i].pm_coord[1].x    = p_vs_ctx->data[i].pos[1].x;
				p_out->mask[i].pm_coord[1].y    = p_vs_ctx->data[i].pos[1].y;
				p_out->mask[i].pm_coord[2].x    = p_vs_ctx->data[i].pos[2].x;
				p_out->mask[i].pm_coord[2].y    = p_vs_ctx->data[i].pos[2].y;
				p_out->mask[i].pm_coord[3].x    = p_vs_ctx->data[i].pos[3].x;
				p_out->mask[i].pm_coord[3].y    = p_vs_ctx->data[i].pos[3].y;
				p_out->mask[i].color[0]         = p_vs_ctx->data[i].color[0];
				p_out->mask[i].color[1]         = p_vs_ctx->data[i].color[1];
				p_out->mask[i].color[2]         = p_vs_ctx->data[i].color[2];
				p_out->mask[i].alpha_weight     = p_vs_ctx->data[i].alpha;
				
				if(p_vs_ctx->data[i].type == VDS_INTERNAL_MASK_TYPE_HOLLOW){
				
					if(i != CTL_IPP_PM_SET_ID_1 && i != CTL_IPP_PM_SET_ID_3 && 
					   i != CTL_IPP_PM_SET_ID_5 && i != CTL_IPP_PM_SET_ID_7){
					    DBG_ERR("hollow mask must be numbered 0 2 4 6, current is %d\n", i);
						vds_memset(&(p_out->mask[i]), 0, sizeof(CTL_IPP_PM));
						continue;
					}
					if((i+1) < vds_max_ime_mask && (i+1) < CTL_IPP_PM_SET_ID_MAX && p_vs_ctx->data[i+1].en){
					    DBG_ERR("since hollow mask(%d) is set, mask(%d) can't be used\n", i, i+1);
						vds_memset(&(p_out->mask[i]), 0, sizeof(CTL_IPP_PM));
						continue;
					}
					if(p_vs_ctx->data[i].pos[0].x != p_vs_ctx->data[i].pos[3].x ||
					   p_vs_ctx->data[i].pos[1].x < p_vs_ctx->data[i].pos[0].x ||
					   p_vs_ctx->data[i].pos[0].y != p_vs_ctx->data[i].pos[1].y ||
					   p_vs_ctx->data[i].pos[2].y < p_vs_ctx->data[i].pos[1].y){
					    DBG_ERR("hollow mask must be a rectangle\n");
						vds_memset(&(p_out->mask[i]), 0, sizeof(CTL_IPP_PM));
						continue;
					}
					if((p_vs_ctx->data[i].pos[1].x - p_vs_ctx->data[i].pos[0].x) <= 2*p_vs_ctx->data[i].thickness){
					    DBG_ERR("hollow mask has thickness(%d) larger than width(%d)\n",
							p_vs_ctx->data[i].thickness,
							p_vs_ctx->data[i].pos[1].x - p_vs_ctx->data[i].pos[0].x);
						vds_memset(&(p_out->mask[i]), 0, sizeof(CTL_IPP_PM));
						continue;
					}
					if((p_vs_ctx->data[i].pos[2].y - p_vs_ctx->data[i].pos[1].y) <= 2*p_vs_ctx->data[i].thickness){
					    DBG_ERR("hollow mask has thickness(%d) larger than height(%d)\n",
							p_vs_ctx->data[i].thickness,
							p_vs_ctx->data[i].pos[2].y - p_vs_ctx->data[i].pos[1].y);
						vds_memset(&(p_out->mask[i]), 0, sizeof(CTL_IPP_PM));
						continue;
					}		
					
					p_out->mask[i].hollow_mask_en  = 1;
					p_out->mask[i].pm_coord_2[0].x = p_vs_ctx->data[i].pos[0].x + p_vs_ctx->data[i].thickness;
					p_out->mask[i].pm_coord_2[0].y = p_vs_ctx->data[i].pos[0].y + p_vs_ctx->data[i].thickness;
					p_out->mask[i].pm_coord_2[1].x = p_vs_ctx->data[i].pos[1].x - p_vs_ctx->data[i].thickness;
					p_out->mask[i].pm_coord_2[1].y = p_vs_ctx->data[i].pos[1].y + p_vs_ctx->data[i].thickness;
					p_out->mask[i].pm_coord_2[2].x = p_vs_ctx->data[i].pos[2].x - p_vs_ctx->data[i].thickness;
					p_out->mask[i].pm_coord_2[2].y = p_vs_ctx->data[i].pos[2].y - p_vs_ctx->data[i].thickness;
					p_out->mask[i].pm_coord_2[3].x = p_vs_ctx->data[i].pos[3].x + p_vs_ctx->data[i].thickness;
					p_out->mask[i].pm_coord_2[3].y = p_vs_ctx->data[i].pos[3].y - p_vs_ctx->data[i].thickness;
				}else{			
					if(i > 0 && p_vs_ctx->data[i-1].en && p_vs_ctx->data[i-1].type == VDS_INTERNAL_MASK_TYPE_HOLLOW){
						DBG_ERR("since hollow mask(%d) is set, mask(%d) can't be used\n", i-1, i);
						vds_memset(&(p_out->mask[i]), 0, sizeof(CTL_IPP_PM));
						continue;
					}
				}
			}
		}
		
	 	p_vs_ctx->dirty = 0;
	}
	
	vds_save_latency(VDS_QS_IME_MASK, dev_id, 0, hwclock_get_longcounter() - start);
	vds_unlock_context(VDS_QS_IME_MASK, iport - ISF_IN_BASE, flag);
	#endif
}

void _isf_vdoprc_finish_input_mask(ISF_UNIT *p_thisunit, UINT32 iport)
{
}


