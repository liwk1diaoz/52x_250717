#include "isf_vdoprc_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoprc_d
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoprc_d_debug_level = NVT_DBG_WRN;
//module_param_named(isf_vdoprc_d_debug_level, isf_vdoprc_d_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_vdoprc_d_debug_level, "vdoprc1 debug level");
///////////////////////////////////////////////////////////////////////////////

void isf_vdoprc_dump_status(int (*dump)(const char *fmt, ...), ISF_UNIT *p_thisunit)
{
	#define ONE_SECOND 1000000
	VDOPRC_CONTEXT* p_ctx = 0;
	UINT32 i;
	register UINT32 k;
	UINT16 avg_cnt[16] = {0};
	BOOL do_en = FALSE;
	if(!p_thisunit || !dump)
		return;
	p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++)
	{
		if ((p_thisunit->port_outstate[i]->state) != (ISF_PORT_STATE_OFF)) {//port current state
			do_en = TRUE;
		}
	}
	if(!do_en)
		return;

	{
		BOOL do_wait_renew = FALSE;
		{
			if (_isf_probe_is_ready(p_thisunit, ISF_IN(0)) != ISF_OK) { //check and renew probe count
#if (USE_IN_DIRECT == ENABLE)
				//dump("in[%d] - work status expired!\r\n", 0);
				if (!(p_ctx->cur_in_cfg_func & VDOPRC_IFUNC_DIRECT)) { //NO input data under direct mode
					do_wait_renew = TRUE;
				}
#else
				do_wait_renew = TRUE;
#endif
			}
		}
		for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++)
		{
			if ((p_thisunit->port_outstate[i]->state) != (ISF_PORT_STATE_RUN)) {//port current state
				continue;
			}
			if (_isf_probe_is_ready(p_thisunit, ISF_OUT(i)) != ISF_OK) { //check and renew probe count
				//dump("out[%d] - work status expired!\r\n", i);
				do_wait_renew = TRUE;
			}
		}
		if (do_wait_renew) {
			dump("\r\nforce reset and wait 1 secnod...\r\n");
			MSLEEP(1000);
		}
	}

	#if 0
	dump("%s.in[%d]! push-queue cnt/max=%lu:%lu\r\n",
		p_thisunit->unit_name, 0, p_ctx->inq.input_cnt, p_ctx->inq.input_max);
	dump("%s.out[%d]! pull-queue cnt/max=%lu:%lu\r\n",
		p_thisunit->unit_name, i, p_ctx->pullq.output[i].cnt, p_ctx->pullq.output[i].max);
	#endif

	//dump in status
	i = 0;
	dump("------------------------- VIDEOPROC %-2d IN WORK STATUS -------------------------\r\n", p_ctx->dev);
	dump("in    PUSH  drop  wrn   err   PROC  drop  wrn   err   REL\r\n");
	{
		ISF_IN_STATUS *p_status = &(p_thisunit->port_ininfo[i]->status.in);
		//calc average count in 1 second
		for(k=0; k<16; k++) {
			if(p_status->ts_push[ISF_TS_SECOND] > 0) {
				avg_cnt[k] = _isf_div64(((UINT64)p_status->total_cnt[k]) * ONE_SECOND + (p_status->ts_push[ISF_TS_SECOND]>>1), p_status->ts_push[ISF_TS_SECOND]);
			} else {
				avg_cnt[k] = 0;
			}
		}
		dump("%-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d\r\n",
			i,
			avg_cnt[12], avg_cnt[13], avg_cnt[14], avg_cnt[15],
			avg_cnt[4], avg_cnt[5], avg_cnt[6], avg_cnt[7],
			avg_cnt[0]
			);
	}
	//dump out status
	dump("------------------------- VIDEOPROC %-2d OUT WORK STATUS ------------------------\r\n", p_ctx->dev);
	dump("out   NEW   drop  wrn   err   PROC  drop  wrn   err   PUSH  drop  wrn   err\r\n");
	for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++)
	{
		ISF_OUT_STATUS *p_status = &(p_thisunit->port_outinfo[i]->status.out);
		if ((p_thisunit->port_outstate[i]->state) != (ISF_PORT_STATE_RUN)) {//port current state
			continue;
		}
		//calc average count in 1 second
		for(k=0; k<16; k++) {
			if(p_status->ts_new[ISF_TS_SECOND] > 0) {
				avg_cnt[k] = _isf_div64(((UINT64)p_status->total_cnt[k]) * ONE_SECOND + (p_status->ts_new[ISF_TS_SECOND]>>1), p_status->ts_new[ISF_TS_SECOND]);
			} else {
				avg_cnt[k] = 0;
			}
		}
		dump("%-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d\r\n",
			i,
			avg_cnt[12], avg_cnt[13], avg_cnt[14], avg_cnt[15],
			avg_cnt[8], avg_cnt[9], avg_cnt[10], avg_cnt[11],
			avg_cnt[0], avg_cnt[1], avg_cnt[2], avg_cnt[3]
			);
	}
	//dump user status
	dump("------------------------- VIDEOPROC %-2d USER WORK STATUS ------------------------\r\n", p_ctx->dev);
	dump("out   PULL   drop  wrn   err   REL\r\n");
	for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++)
	{
		ISF_USER_STATUS *p_status = &(p_thisunit->port_outinfo[i]->status.out);
		if ((p_thisunit->port_outstate[i]->state) != (ISF_PORT_STATE_RUN)) {//port current state
			continue;
		}
		//calc average count in 1 second
		for(k=0; k<8; k++) {
			if(p_status->ts_pull[ISF_TS_SECOND] > 0) {
				avg_cnt[k] = _isf_div64(((UINT64)p_status->total_cnt[16+k]) * ONE_SECOND + (p_status->ts_pull[ISF_TS_SECOND]>>1), p_status->ts_pull[ISF_TS_SECOND]);
			} else {
				avg_cnt[k] = 0;
			}
		}
		dump("%-4d  %-4d  %-4d  %-4d  %-4d  %-4d\r\n",
			i,
			avg_cnt[4], avg_cnt[5], avg_cnt[6], avg_cnt[7],
			avg_cnt[0]
			);
	}
	dump("\r\n");
}
#include "kwrap/cpu.h"

// MD
#define MD_HEAD_BUFSIZE()	            (0x40)
// Note , the md info w, h is vprc input w, h not out w, h
#define MD_INFO_BUFSIZE(w, h)           (ALIGN_CEIL_64(((((w >> 7) + 3) >> 2) << 2) * ((h + 15) >> 4)))
UINT32 ctl_ipp_util_yuvsize(VDO_PXLFMT fmt, UINT32 y_width, UINT32 y_height);
#define VOS_ALIGN_BYTES     64
#define MD_ROIXY(x,y) ((x&0xFFFF) | ((y&0xFFFF)<<16))
#define MD_ROIWH(w,h) ((w&0xFFFF) | ((h&0xFFFF)<<16))

void md_dumpinfo(VDO_MD_INFO *pinfo)
{
    if(pinfo){
        DBG_DUMP("src (%d,%d)\r\n",pinfo->src_img_size.w,pinfo->src_img_size.h);
        DBG_DUMP("md (%d,%d)\r\n",pinfo->md_size.w,pinfo->md_size.h);
        DBG_DUMP("md_lofs %d\r\n",pinfo->md_lofs);
        DBG_DUMP("dir 0x%x\r\n",pinfo->reserved[0]);
        DBG_DUMP("roi_xy %x\r\n",pinfo->reserved[1]);
        DBG_DUMP("roi_wh %x\r\n",pinfo->reserved[2]);
    }
}

UINT32 md_getdir(UINT32 dir)
{
	UINT32 md_dir = 0;
    UINT32 pre_dir =0;

    if(!dir) {
        return 0;
    }
    pre_dir= (dir & 0x01)<<8;
    DBG_IND("pre_dir %d \r\n",pre_dir);

	if((dir & VDO_DIR_ROTATE_90) == VDO_DIR_ROTATE_90) {
        md_dir= 0x102+pre_dir;
	} else if((dir & VDO_DIR_ROTATE_270) == VDO_DIR_ROTATE_270) {
	    md_dir= 0x101+pre_dir;
    } else if((dir & VDO_DIR_ROTATE_180) == VDO_DIR_ROTATE_180) {
        md_dir= 0x103+pre_dir;
    }

	return md_dir;
}
UINT32 _isf_vdoprc_meta_get_size(VDO_FRAME *pframe)
{
    VDO_METADATA* p_next = NULL;
    UINT32 total_size = 0,size = 0;

    if(!pframe) {
        DBG_ERR("pframe is NULL \r\n");
        return 0;
    }

    p_next = pframe->p_next;

    while(p_next!=NULL)
    {
        DBG_IND("p_next %x sign %x \r\n",(unsigned int)p_next,p_next->sign);
        switch(p_next->sign)
        {
            case MAKEFOURCC('M','V','I','F'):
            {
                VDO_MD_INFO *p_md_old = (VDO_MD_INFO *)((UINT32)p_next+sizeof(VDO_METADATA));
                size = MD_HEAD_BUFSIZE()+MD_INFO_BUFSIZE(p_md_old->src_img_size.w, p_md_old->src_img_size.h);
            }
            break;
            default:
                DBG_ERR("not sup %x \r\n",p_next->sign);
            break;
        }

        total_size+=size;
        DBG_IND("total_size %x \r\n",total_size);
        p_next = p_next->p_next;
    }

    return total_size ;

}

UINT32 _isf_vdoprc_meta_copy(VDO_FRAME *pframe_new,VDO_FRAME *pframe_old,UINT32 addr,UINT32 dir,IRECT* p_crop)
{
    VDO_METADATA* p_next = pframe_old->p_next;
    UINT32 meta_size =0;

    if(addr==0){
        DBG_ERR("addr %d\r\n",addr);
        return ISF_ERR_INVALID_VALUE;
    }
    while(p_next!=NULL)
    {
        DBG_IND("p_next %x sign %x \r\n",(unsigned int)p_next,p_next->sign);
        switch(p_next->sign)
        {
            case MAKEFOURCC('M','V','I','F'):
            {
                VDO_MD_INFO *p_md_old = (VDO_MD_INFO *)((UINT32)p_next+sizeof(VDO_METADATA));
                VDO_MD_INFO *p_md_new = 0;

                pframe_new->p_next = (VDO_METADATA *)addr;
                meta_size = MD_HEAD_BUFSIZE()+MD_INFO_BUFSIZE(p_md_old->src_img_size.w, p_md_old->src_img_size.h);
                memcpy((void *)(pframe_new->p_next),(void *)(pframe_old->p_next),meta_size);
                pframe_new->p_next->sign = MAKEFOURCC('M','V','I','F');
                pframe_new->p_next->p_next = NULL;
                //set dir if rotate
                p_md_new = (VDO_MD_INFO *)(((UINT32)(pframe_new->p_next))+sizeof(VDO_METADATA));

                if((dir)&&(p_md_old->reserved[0])){
                    DBG_ERR("rotate twice\r\n");
                    return ISF_ERR_PARAM_EXCEED_LIMIT;
                }
                if( (p_crop) && ((p_md_old->reserved[1])||(p_md_old->reserved[2])) ){
                    DBG_ERR("crop twice\r\n");
                    return ISF_ERR_PARAM_EXCEED_LIMIT;
                }
                if(dir) {
                    p_md_new->reserved[0] = md_getdir(dir);
                }

                if(p_crop) {
                    p_md_new->reserved[1] = MD_ROIXY(p_crop->x,p_crop->y);
                    p_md_new->reserved[2] = MD_ROIWH(p_crop->w,p_crop->h);
                }

                vos_cpu_dcache_sync((VOS_ADDR)(pframe_new->p_next), meta_size, VOS_DMA_TO_DEVICE);
                //md_dumpinfo(p_md_new);

            }
            break;
            default:
                DBG_ERR("not sup %x \r\n",p_next->sign);
            break;
        }
        p_next = p_next->p_next;
        addr+= meta_size;  //for next meta addr
    }
    return 0;
}

VDO_FRAME *_isf_vdoprc_meta_get_frame(ISF_UNIT *p_thisunit, UINT64 fcnt)
{
	UINT32 j;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
    VDO_FRAME *p_frame = NULL;
	//DBG_DUMP("@@@@@count@@@@@ %llu\r\n",fcnt);

	//DBGD(p_ctx->inq.input_cnt);
	for (j = 0; j < p_ctx->inq.input_max; j++) {
		ISF_DATA *p_data = (ISF_DATA *)&(p_ctx->inq.input_data[j]);

		if (p_ctx->inq.input_used[j]) {
            p_frame = (VDO_FRAME *)p_data->desc;
            //DBG_DUMP("j %d fcnt %llu %x \r\n",j,p_frame->count,p_data->h_data);
            if(p_frame->count == fcnt){
			    #if 0
                if(p_frame->p_next) {
                    DBGH(p_frame->p_next->sign);
                }
				#endif
                return p_frame;
            }
		}
	}
    DBG_ERR("no fcnt %llu\r\n",fcnt);
    return NULL;
}
