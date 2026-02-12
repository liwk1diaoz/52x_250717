#include "isf_vdocap_int.h"
#include "isf_vdocap_dbg.h"
#include "comm/hwclock.h"

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

static VK_DEFINE_SPINLOCK(flow_lock);
#define loc_cpu_flow(flags)   vk_spin_lock_irqsave(&flow_lock, flags)
#define unl_cpu_flow(flags)   vk_spin_unlock_irqrestore(&flow_lock, flags)


#define WATCH_PORT			ISF_OUT(0)
#define FORCE_DUMP_DATA		DISABLE

#define DUMP_DATA_ERROR		DISABLE

#define DUMP_PERFORMANCE		DISABLE
#define AUTO_RELEASE			ENABLE
#define DUMP_TRACK			DISABLE

#define DUMP_FPS				DISABLE

#define MANUAL_SYNC_BASE		ENABLE
#define DUMP_KICK				DISABLE
#define DUMP_KICK_ERR			DISABLE

#define QUEUE_TRACE            DISABLE

#define SHDR_QUEUE_CHECK     DISABLE

static ISF_DATA zero = {0};



#define SHDR_ALLOCATED_FLAG	0x80000000
#define SHDR_FRAME_DROPPED  0xFE

//only for shdr+direct
static ISF_DATA sie2_ring_buffer = {0};

static void _isf_vdocap_check_csi_status(VDOCAP_CONTEXT *p_ctx)
{
	if (p_ctx->csi.cb_status) {
		p_ctx->csi.cb_status = 0;
		p_ctx->csi.error = 1;
		p_ctx->csi.ok_cnt = 0;
	} else {
		if (p_ctx->csi.error) {
			UINT32 fps;
			p_ctx->csi.ok_cnt++;
			if (p_ctx->chgsenmode_info.sel == CTL_SEN_MODESEL_AUTO) {
				fps = p_ctx->chgsenmode_info.auto_info.frame_rate;
			} else {
				fps = p_ctx->chgsenmode_info.manual_info.frame_rate;
			}
			fps/=100;
			//DBGD(fps);
			if (p_ctx->csi.ok_cnt > fps/2) {
				p_ctx->csi.error = 0;
				p_ctx->csi.ok_cnt = 0;
				DBG_MSG("csi recovered\r\n");
			}
		}
	}
}

VDOCAP_SHDR_OUT_QUEUE _vdocap_shdr_queue[VDOCAP_SHDR_SET_MAX_NUM];

VDOCAP_SHDR_SET _vdocap_shdr_map_to_set(VDOCAP_SEN_HDR_MAP shdr_map)
{
	if(shdr_map < VDOCAP_SEN_HDR_SET2_MAIN) {
		return VDOCAP_SHDR_SET1;
	} else {
		return VDOCAP_SHDR_SET2;
	}
}
UINT32 _vdocap_shdr_map_to_seq(VDOCAP_SEN_HDR_MAP shdr_map)
{
	UINT32 ret;
	if(shdr_map < VDOCAP_SEN_HDR_SET2_MAIN) {
		if (shdr_map == VDOCAP_SEN_HDR_NONE) {
			ret = 0;
		} else {
			ret = shdr_map - VDOCAP_SEN_HDR_SET1_MAIN;
		}

	} else {
		ret = shdr_map - VDOCAP_SEN_HDR_SET2_MAIN;
	}
	if (ret >= SHDR_MAX_FRAME_NUM) {
		DBG_ERR("shdr_map(%d) error!\r\n", shdr_map);
		ret = SHDR_MAX_FRAME_NUM - 1;
	}
	return ret;
}
BOOL _vdocap_is_shdr_mode(VDOCAP_SEN_HDR_MAP shdr_map)
{
	if ((shdr_map != VDOCAP_SEN_HDR_NONE) && (_vdocap_shdr_frm_num[_vdocap_shdr_map_to_set(shdr_map)] > 1)) {
		return TRUE;
	} else {
		return FALSE;
	}
}
void _vdocap_shdr_oport_initqueue(VDOCAP_SHDR_SET shdr_set)
{
	UINT32 j;

	memset((void *)(&_vdocap_shdr_queue[shdr_set]), 0, sizeof(VDOCAP_SHDR_OUT_QUEUE));
	for (j = 0; j < VDOCAP_OUT_DEPTH_MAX; j++) {
		_vdocap_shdr_queue[shdr_set].frame_cnt[j] = 0xFFFFFFFF;
	}

}
#if 0
static void _vdocap_shdr_oport_dead_queue_check(ISF_UNIT *p_thisunit, UINT32 oport, VDOCAP_SHDR_OUT_QUEUE *p_outq, UINT32 curr_frm_cnt)
{
	UINT32 j, k, shdr_frm_num;
	unsigned long flow_flags;
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 shdr_set;

	shdr_set = _vdocap_shdr_map_to_set(p_ctx->shdr_map);
	shdr_frm_num = _vdocap_shdr_frm_num[shdr_set];

	loc_cpu_flow(flow_flags);
	for (j = 0; j < VDOCAP_OUT_DEPTH_MAX; j++) {
		if (p_outq->output_used[j]) {
			ISF_DATA *p_data = NULL;

			//increasing threshold to to ignore blocks in pulled queue
			if (curr_frm_cnt <= (p_outq->frame_cnt[j] + 3)) {
				continue;
			}
			//ignore case7: N0 -> N1 -> L0 -> P0 -> P1 -> U0   (ctl sie save raw)
			if (p_outq->push_cnt[j] >= shdr_frm_num) {
				continue;
			}
			for (k = 0; k < shdr_frm_num; k++) {
				if (p_outq->addr[j][k]) {
					p_data = &p_outq->output_data[j][k];
					if (unlikely(NVT_DBG_VALUE <= isf_vdocap_debug_level) || SHDR_QUEUE_CHECK) {
						DBG_DUMP("release dead blk[%d][%d] blk_id=%08x addr=%08x cnt=%d frame_cnt=%d push_cnt=%d\r\n", j, k, p_data->h_data, p_outq->addr[j][k],p_outq->output_used[j], p_outq->frame_cnt[j], p_outq->push_cnt[j]);
					}
					p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
					p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_EXT_WRN, ISF_OK);
					_vdocap_shdr_oport_releasedata(p_outq, j);
					p_outq->addr[j][k] = 0;
				}
			}
		}
	}
	unl_cpu_flow(flow_flags);
}
#endif
void _vdocap_shdr_oport_close_check(ISF_UNIT *p_thisunit, UINT32 oport, VDOCAP_SHDR_OUT_QUEUE *p_outq)
{
	UINT32 j;
	unsigned long flow_flags;
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	loc_cpu_flow(flow_flags);
	for (j = 0; j < VDOCAP_OUT_DEPTH_MAX; j++) {
		if (p_outq->output_used[j]) {
			UINT32 k, shdr_frm_num;
			UINT32 shdr_set = _vdocap_shdr_map_to_set(p_ctx->shdr_map);
			ISF_DATA *p_data = NULL;

			shdr_frm_num = _vdocap_shdr_frm_num[shdr_set];
			for (k = 0; k < shdr_frm_num; k++) {
				if (p_outq->addr[j][k]) {
					p_data = &p_outq->output_data[j][k];
					DBG_WRN("release blk[%d][%d] blk_id=%08x addr=%08x cnt=%d frame_cnt=%d push_cnt=%d\r\n", j, k, p_data->h_data, p_outq->addr[j][k],p_outq->output_used[j], p_outq->frame_cnt[j], p_outq->push_cnt[j]);
					p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
					p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_EXT_WRN, ISF_OK);
					_vdocap_shdr_oport_releasedata(p_outq, j);
					p_outq->addr[j][k] = 0;
				}
			}
		}
	}
	unl_cpu_flow(flow_flags);
}
static UINT32 _vdocap_shdr_oport_newdata(VDOCAP_SHDR_OUT_QUEUE *p_outq, UINT32 frame_cnt)
{
	UINT32 j;
	ISF_DATA *p_data = NULL;
	unsigned long flags;

	loc_cpu(flags);
	//check if queue/buffer was allocated
	for (j = 0; j < VDOCAP_OUT_DEPTH_MAX; j++) {
		if (p_outq->frame_cnt[j] == frame_cnt) {
			if (p_outq->output_used[j] == 0 || p_outq->force_drop[j]) {
				//means the first frame new buffer failed, the same frame cnt should drop
				#if (QUEUE_TRACE == ENABLE)
				DBG_DUMP("shdr dropped used[%d]=%d, frm_cnt=%d\r\n", j, p_outq->output_used[j], p_outq->frame_cnt[j]);
				#endif
				unl_cpu(flags);
				return SHDR_FRAME_DROPPED;
			} else if (p_outq->output_used[j] >= SHDR_MAX_FRAME_NUM) {
				DBG_WRN("j%d FRM[%d] overnewed!(%d)\r\n", j, p_outq->frame_cnt[j], p_outq->output_used[j]);
				return SHDR_FRAME_DROPPED;
			} else {
				p_outq->output_used[j]++;
				#if (QUEUE_TRACE == ENABLE)
				DBG_DUMP("shdr used[%d]=%d, frm_cnt=%d\r\n", j, p_outq->output_used[j], p_outq->frame_cnt[j]);
				#endif
				unl_cpu(flags);
				return SHDR_ALLOCATED_FLAG|j;
			}
		}
	}
	//allocate a new queue
	for (j = 0; j < VDOCAP_OUT_DEPTH_MAX; j++) {
		if (p_outq->output_used[j] == 0) {
			UINT32 k;
			//DBG_DUMP("^C+1\r\n");
			p_outq->output_used[j] = 1;
			p_outq->frame_cnt[j] = frame_cnt;
			p_outq->push_cnt[j] = 0;
			p_outq->force_drop[j] = 0;
			for (k = 0; k < SHDR_MAX_FRAME_NUM; k++) {
				p_data = &(p_outq->output_data[j][k]);
				p_data->desc[0] = 0; //clear
				p_data->h_data = 0; //clear
				p_outq->addr[j][k] = 0;
				p_outq->vdo_frm_addr[j][k] = 0;
			}
			//p_data->serial = 0;
			#if (QUEUE_TRACE == ENABLE)
			DBG_DUMP("shdr used[%d] new, frm_cnt=%d\r\n", j, p_outq->frame_cnt[j]);
			#endif
			unl_cpu(flags);
			return j;
		}
	}
	unl_cpu(flags);
	return 0xff;
}
static UINT32 _vdocap_shdr_oport_adddata(VDOCAP_SHDR_OUT_QUEUE *p_outq, UINT32 j)
{
	unsigned long flags;

	loc_cpu(flags);
	p_outq->output_used[j] += 1;
	#if (QUEUE_TRACE == ENABLE)
	DBG_DUMP("add j%d++ => %d\r\n", j, p_outq->output_used[j]);
	#endif
	unl_cpu(flags);
	return 1;
}
UINT32 _vdocap_shdr_oport_releasedata(VDOCAP_SHDR_OUT_QUEUE *p_outq, UINT32 j)
{
	unsigned long flags;

	loc_cpu(flags);
	if (p_outq->output_used[j] == 0) {
		DBG_WRN("-shdr j%d already released!\r\n", j);
		unl_cpu(flags);
		return 0;
	}
	//DBG_DUMP("^C-1\r\n");
	p_outq->output_used[j] -= 1;
	#if (QUEUE_TRACE == ENABLE)
	DBG_DUMP("rel j%d-%d-- => %d\r\n", j, p_outq->frame_cnt[j], p_outq->output_used[j]);
	#endif
	//if (p_ctx->outq.output_used[j] == 0)
	//	p_data->h_data = 0; //clear
	unl_cpu(flags);
	return 1;
}
/*
	SHDR buffer mechanism for new/release
	N:new P:push L:lock U:unlock number:shdr_seq
	N might be from different SIEx ISR and P/L/U are from the same task.
	U might be also form user task when ctl sie stop is invoked.

	case1: N0 -> N1 -> P0 -> P1   normal case
	case2: N0 fail -> N1 SHDR_FRAME_DROPPED
		set force_drop when new failed
	case3: N0 + N1 racing and one of them new fail -> Px drop
		check output_used and force_drop
	case4: N0 -> N1 fail -> P0 drop
	case5: N0 -> N1 -> U0 -> P1 drop
		set force_drop when U happened
	case6: N0 -> N1 -> P1 + U0 racing
		U0 check push_count and release pushed block
	case7: N0 -> N1 -> L0 -> P0 -> P1 -> U0   (ctl sie save raw)
		 when U0 comes, push_count should equal shdr_frm_num
	case8: N0 -> P0 -> N1 SHDR_FRAME_DROPPED
		when P0 comes, release N0 and set force_drop
*/
void _isf_vdocap_shdr_oport_do_new(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_size, UINT32 ddr, void *p_header_info, VDOCAP_SEN_HDR_MAP shdr_map)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 j;
	UINT32 i = oport - ISF_OUT_BASE;
	ISF_PORT *p_port = p_thisunit->port_out[i];
	ISF_DATA *p_data = NULL;
	UINT32 addr = 0;
	CTL_SIE_HEADER_INFO *p_sie_head_info = (CTL_SIE_HEADER_INFO *)p_header_info;
	static ISF_DATA_CLASS g_vdocap_out_data = {0};
	UINT32 frame_cnt;
	VDOCAP_SHDR_OUT_QUEUE *p_outq;
	UINT32 shdr_set;
	UINT32 shdr_seq;
	UINT32 shdr_frm_num;
	unsigned long flow_flags;
	//UINT32 k, m;

	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW, ISF_ENTER);

	//p_ctx->vd_count = p_sie_head_info->vdo_frm.count;
	if (FALSE == p_ctx->count_vd_by_sensor) {
		p_ctx->vd_count++;
	}

	if ((p_ctx->dev_ready == 0) || (p_ctx->sie_hdl == 0)) {
		if (p_ctx->dev_trigger_open == 1) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_DROP, ISF_ERR_NOT_START);
		} if (p_ctx->dev_trigger_close == 1) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_DROP, ISF_ERR_NOT_START);
		} else {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_ERR, ISF_ERR_INACTIVE_STATE);
		}
		p_sie_head_info->buf_id = 0;
		p_sie_head_info->buf_addr = 0;
		return; //drop frame!
	}

	if (p_port) {
		p_ctx->outq.output_connecttype[i] = p_port->connecttype;
	} else {
		p_ctx->outq.output_connecttype[i] = ISF_CONNECT_NONE;
	}
	shdr_set = _vdocap_shdr_map_to_set(shdr_map);
	shdr_seq = _vdocap_shdr_map_to_seq(shdr_map);
	p_outq = &_vdocap_shdr_queue[shdr_set];
	shdr_frm_num = _vdocap_shdr_frm_num[shdr_set];

	//special memory alloc for SHDR+direct
	if (p_sie_head_info->buf_id == 0xFFFFFFFF) {
		DBG_MSG("VDOCAP[%d] alloc ddr[%d][%d] SIE2 ring buffer\r\n", p_ctx->id, ddr, buf_size);
		p_thisunit->p_base->init_data(&sie2_ring_buffer, &g_vdocap_out_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
		addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "ring_buf", buf_size, &sie2_ring_buffer);
		if (addr == 0) {
			DBG_WRN("No buffer for SIE2\r\n");
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_WRN, ISF_ERR_IGNORE);
		} else {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW, ISF_OK);
		}
		p_sie_head_info->buf_addr = addr;
		return;
	}
	//ctl_sie use buf_id to store frame count temporally
	frame_cnt = p_sie_head_info->buf_id;
	//do frc
	if (shdr_seq == 0 && _isf_frc_is_select(p_thisunit, oport, &(p_ctx->outfrc[i])) == 0) {
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_DROP, ISF_ERR_FRC_DROP);
		p_sie_head_info->buf_id = 0;
		p_sie_head_info->buf_addr = 0;
		return; //drop frame!
	}

	loc_cpu_flow(flow_flags);
	j = _vdocap_shdr_oport_newdata(p_outq, frame_cnt);
	unl_cpu_flow(flow_flags);
	if (j == 0xff || j == SHDR_FRAME_DROPPED) {
		p_sie_head_info->buf_id = 0;
		p_sie_head_info->buf_addr = 0;
#if (DUMP_DATA_ERROR == ENABLE)
		if (oport == WATCH_PORT && j == 0xff) {
			DBG_WRN("vdocap_shdr[%d][%d]! New Q-- overflow-1!!!\r\n", shdr_map, frame_cnt);
		}
#endif
		if (unlikely(NVT_DBG_VALUE <= isf_vdocap_debug_level) || SHDR_QUEUE_CHECK) {
			if (j == SHDR_FRAME_DROPPED) {
				DBG_DUMP("[%d]N %d Drop\r\n",shdr_seq,frame_cnt);
			}
		}

		//#endif
		if (j == SHDR_FRAME_DROPPED) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_WRN, ISF_ERR_NEW_FAIL);
		} else {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_WRN, ISF_ERR_QUEUE_FULL);
		}
		return;
	}
	if (shdr_frm_num <= shdr_seq) {
		DBG_ERR("SHDR mapping error(%d<=%d)\r\n",shdr_frm_num,shdr_seq);
		return;
	}
#if 0
	if (j & SHDR_ALLOCATED_FLAG) {
		j &= ~SHDR_ALLOCATED_FLAG;

		if ((shdr_seq < shdr_frm_num) && (p_outq->output_used[j] <= shdr_frm_num)) {
			if ( buf_size > p_outq->size[j]) {
				p_sie_head_info->buf_id = 0;
				p_sie_head_info->buf_addr = 0;
				DBG_DUMP("vdocap_shdr[%d][%d] buffer size(%d) small frm_num=%d, used=%d\r\n", shdr_map, frame_cnt, buf_size, shdr_frm_num, p_outq->output_used[j]);
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_ERR, ISF_ERR_FATAL);
				_vdocap_shdr_oport_releasedata(p_outq, j);
			} else {
				p_sie_head_info->buf_id = (UINT32)(j | 0xffff0000);
				p_sie_head_info->buf_addr = p_outq->addr[j][shdr_seq];
				#if (SHDR_QUEUE_CHECK == ENABLE)
				DBG_DUMP("[%d]j%d-%d-0x%X a\r\n",shdr_seq,j,frame_cnt,p_sie_head_info->buf_addr);
				#endif
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW, ISF_OK);
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC, ISF_ENTER);
				#if SHDR_QUEUE_DEBUG
				p_outq->new_ok[shdr_seq]++;
				#endif
			}
		} else {
			p_sie_head_info->buf_id = 0;
			p_sie_head_info->buf_addr = 0;
			DBG_ERR("vdocap_shdr[%d][%d] mapping err! seq=%d, frm_num=%d, used=%d\r\n", shdr_map, frame_cnt, shdr_seq, shdr_frm_num, p_outq->output_used[j]);
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_ERR, ISF_ERR_FATAL);
			_vdocap_shdr_oport_releasedata(p_outq, j);
		}
		return;
	}
#endif
	if (j & SHDR_ALLOCATED_FLAG) {
		j &= ~SHDR_ALLOCATED_FLAG;
	}
    //DBG_DUMP("[%d]j%d-%d-%dKB\r\n",shdr_seq,j,frame_cnt,buf_size/1024);
    /* record sie out size for debuging */
	p_ctx->out_buf_size[i] = buf_size;
	if (j >= VDOCAP_OUT_DEPTH_MAX) {
		return;
	}
	//allocate individual shdr frame buffer size
	p_data = &p_outq->output_data[j][shdr_seq];
	p_thisunit->p_base->init_data(p_data, &g_vdocap_out_data, MAKEFOURCC('V','F','R','M'), 0x00010000);

	if (p_ctx->one_buf && (p_outq->force_onebuffer[shdr_seq] != 0)) { //under "single buffer mode", if try to new 2nd buffer
		p_data->h_data = p_outq->force_onebuffer[shdr_seq];
		addr = nvtmpp_vb_blk2va(p_data->h_data);
		p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to this "single buffer"
		if (addr == 0) {
			DBG_ERR("one_buf error h_data=0x%X\r\n", p_data->h_data);
		}
	} else {
		addr = p_thisunit->p_base->do_new(p_thisunit, oport, NVTMPP_VB_INVALID_POOL, ddr, buf_size, p_data, ISF_OUT_PROBE_NEW);
		if (addr && p_ctx->one_buf) {
			DBG_UNIT("%s.out[%d]! Start single blk mode => blk_id=%08x addr=%08x\r\n", p_thisunit->unit_name, oport, p_data->h_data, addr);
			p_outq->force_onebuffer[shdr_seq] = p_data->h_data; //keep "single buffer" and start "single buffer mode"
			p_outq->force_onesize[shdr_seq] = buf_size; //keep "single buffer" and start "single buffer mode"
			p_outq->force_onej = j;
			p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to this "single buffer"
		}
	}

	if (addr == 0) {
		if (unlikely(NVT_DBG_VALUE <= isf_vdocap_debug_level) || SHDR_QUEUE_CHECK) {
			DBG_DUMP("[%d]j%d-%d-%d New %d failed!\r\n", shdr_seq, j, frame_cnt, shdr_seq ,buf_size);
		}
		//p_outq->size[j] = 0;
		p_outq->force_drop[j] = 1;
		_vdocap_shdr_oport_releasedata(p_outq, j);
		p_sie_head_info->buf_id = 0;
		p_sie_head_info->buf_addr = 0;
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_WRN, ISF_ERR_IGNORE);
		return;
	}
	p_outq->addr[j][shdr_seq] = addr;
	#if SHDR_QUEUE_DEBUG
	p_outq->do_new[shdr_seq]++;
	#endif

	//p_outq->size[j] = shdr_block_size;
#if (FORCE_DUMP_DATA == ENABLE)
		//if (oport == WATCH_PORT) {
			//DBG_DUMP("vdocap%d.out[%d]! New -- Vdo: size=%d ok => blk_id=%08x addr=%08x blk[%d].cnt=%d\r\n", id, oport, buf_size, p_data->h_data, addr, j, g_output_used[id][i][j]);
		//}
#endif
	p_sie_head_info->buf_id = (UINT32)(j | 0xffff0000);
	p_sie_head_info->buf_addr = p_outq->addr[j][shdr_seq];
	#if SHDR_QUEUE_DEBUG
	p_outq->new_ok[shdr_seq]++;
	#endif
	#if (SHDR_QUEUE_CHECK == ENABLE)
	DBG_DUMP("[%d]j%d-%d-0x%X-%dKB\r\n",shdr_seq,j,frame_cnt,p_sie_head_info->buf_addr,buf_size/1024);
	#endif
	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW, ISF_OK);
	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC, ISF_ENTER);
	return;
}
void _isf_vdocap_shdr_oport_do_push(ISF_UNIT *p_thisunit, UINT32 oport, void *p_header_info, VDOCAP_SEN_HDR_MAP shdr_map)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT(0);
	UINT32 j;
	ISF_PORT *p_port = p_thisunit->port_out[i];
	ISF_DATA *p_data;
	CTL_SIE_HEADER_INFO *p_sie_head_info = (CTL_SIE_HEADER_INFO *)p_header_info;
	VDOCAP_SHDR_OUT_QUEUE *p_outq;
	UINT32 shdr_set;
	UINT32 shdr_seq;
	UINT32 shdr_frm_num;
	unsigned long flow_flags;
	UINT32 k;

	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC, ISF_OK);
	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH, ISF_ENTER);


	shdr_set = _vdocap_shdr_map_to_set(shdr_map);
	shdr_seq = _vdocap_shdr_map_to_seq(shdr_map);
	p_outq = &_vdocap_shdr_queue[shdr_set];
	shdr_frm_num = _vdocap_shdr_frm_num[shdr_set];


	j = (p_sie_head_info->buf_id & ~0xffff0000);
	#if (SHDR_QUEUE_CHECK == ENABLE)
    DBG_DUMP("-[%d]j%d-%d-0x%X-%d\r\n",shdr_seq,j,(UINT32)p_sie_head_info->vdo_frm.count,p_sie_head_info->buf_addr, p_outq->output_used[j]);
    #endif
	// NOTE! under IPL direct mode, the last push maybe enter with NULL p_port.
	if (j >= VDOCAP_OUT_DEPTH_MAX) {
		DBG_ERR("vdocap_shdr[%d] Push -- Vdo: blk[%d???] IGNORE!!!!!\r\n", shdr_map, j);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_QUEUE_ERROR);
		return;
	}

	p_data = (ISF_DATA *)&p_outq->output_data[j][shdr_seq];
	p_data->func = j;

	memcpy(p_data->desc, &p_sie_head_info->vdo_frm, sizeof(VDO_FRAME));


	if (shdr_seq < SHDR_MAX_FRAME_NUM) {
		p_outq->vdo_frm_addr[j][shdr_seq] = (UINT32)&p_sie_head_info->vdo_frm;
	} else {
		DBG_ERR("vdocap_shdr[%d] Push -- map seq(%d) err\r\n", shdr_map, shdr_seq);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_INVALID_DATA);
		return;
	}
	if (p_port) {
		p_ctx->outq.output_connecttype[i] = p_port->connecttype;
	} else {
		p_ctx->outq.output_connecttype[i] = ISF_CONNECT_NONE;
	}

	if ((p_ctx->dev_ready == 0) || (p_ctx->sie_hdl == 0)) {
		DBG_WRN("[%d]shdr_map=%d dev_ready=%d sie_hdl=0x%X\r\n", p_ctx->id, shdr_map, p_ctx->dev_ready, p_ctx->sie_hdl);
		loc_cpu_flow(flow_flags);
		p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
		_vdocap_shdr_oport_releasedata(p_outq, j);
		p_outq->addr[j][shdr_seq] = 0;
		unl_cpu_flow(flow_flags);
		if (p_ctx->dev_trigger_open == 1) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_NOT_START);
		} if (p_ctx->dev_trigger_close == 1) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_NOT_START);
		} else {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_INACTIVE_STATE);
		}
		return;
	}

	#if SHDR_QUEUE_DEBUG
	p_outq->push[shdr_seq]++;
	#endif

	#if 0
	//check dead queue every n frame_cnt
	if ((shdr_seq == (shdr_frm_num - 1)) && (0 == (p_outq->frame_cnt[j] & 0x7))) {
		_vdocap_shdr_oport_dead_queue_check(p_thisunit, oport, p_outq, p_outq->frame_cnt[j]);
	}
	#endif

	if (p_outq->output_used[j] == 0) {//this should not happen since the shdr queue has been released!
		//N0->N1->N2->P0->U1->P2 !? all frame has been released in U1 phase
		//U1 release P2 might not be safe, but this only happens to VCAP close.
		DBG_WRN("vdocap_shdr[%d][%d] Push -- dropped!\r\n", shdr_seq, (UINT32)p_sie_head_info->vdo_frm.count);
	} else {
		p_outq->push_cnt[j]++;
		if (p_outq->push_cnt[j] < shdr_frm_num) {//still need to collect "push"
			#if (FORCE_DUMP_DATA == ENABLE)
			if (oport == WATCH_PORT) {
				DBG_DUMP("vdocap_shdr[%d][%d] Push collected.\r\n", shdr_seq, (UINT32)p_sie_head_info->vdo_frm.count);
			}
			#endif
			//if one vdocap is stopped, only one frame will new and push
			loc_cpu_flow(flow_flags);

			if (p_outq->force_drop[j]) {
				p_data = &p_outq->output_data[j][shdr_seq];
				if (unlikely(NVT_DBG_VALUE <= isf_vdocap_debug_level) || SHDR_QUEUE_CHECK) {
					DBG_DUMP("[%d]P j%d-%d-0x%X force drop\r\n",shdr_seq,j,(UINT32)p_sie_head_info->vdo_frm.count, nvtmpp_vb_blk2va(p_data->h_data));
				}
				if (p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH)) {
					DBG_ERR("vdocap_shdr[%d] one frame release failed! j[%d].used=%d  vd_count=%d\r\n", shdr_seq,  j, p_outq->output_used[j],(UINT32)p_sie_head_info->vdo_frm.count);
				}
				_vdocap_shdr_oport_releasedata(p_outq, j);
				p_outq->addr[j][shdr_seq] = 0;
				#if SHDR_QUEUE_DEBUG
				p_outq->push_release[shdr_seq]++;
				#endif
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_INCOMPLETE_DATA);
			} else if (p_outq->output_used[j] < shdr_frm_num) {
			//case8: N0 -> P0 -> N1 SHDR_FRAME_DROPPED
			//	when P0 comes, release N0 and set force_drop
				p_data = &p_outq->output_data[j][shdr_seq];
				if (unlikely(NVT_DBG_VALUE <= isf_vdocap_debug_level) || SHDR_QUEUE_CHECK) {
					DBG_DUMP("[%d]P j%d-%d-0x%X push before all new done => force drop\r\n",shdr_seq,j,(UINT32)p_sie_head_info->vdo_frm.count, nvtmpp_vb_blk2va(p_data->h_data));
				}
				if (p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH)) {
					DBG_ERR("vdocap_shdr[%d] one frame release failed! j[%d].used=%d  vd_count=%d\r\n", shdr_seq,  j, p_outq->output_used[j],(UINT32)p_sie_head_info->vdo_frm.count);
				}
				_vdocap_shdr_oport_releasedata(p_outq, j);
				p_outq->addr[j][shdr_seq] = 0;
				#if SHDR_QUEUE_DEBUG
				p_outq->push_release[shdr_seq]++;
				#endif
				p_outq->force_drop[j] = 1;
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_INCOMPLETE_DATA);
			#if SHDR_QUEUE_DEBUG
			} else if (p_outq->output_used[j] > 2) {
				DBG_DUMP("IGNORE vdocap_shdr[%d] j[%d] used=%d  vd_count=%d\r\n", shdr_seq,  j, p_outq->output_used[j],(UINT32)p_sie_head_info->vdo_frm.count);
			} else {
				p_outq->push_collect[shdr_seq]++;
			#endif
			}
			unl_cpu_flow(flow_flags);
		} else {// all "push" collected done, ready to push
			VDO_FRAME *p_vdofrm;
			//add check flag for vprc debugging
			if (p_outq->push_cnt[j] > shdr_frm_num) {
				DBG_WRN("over pushed! [%d]j%d-%d-%d 0x%X used[%d]\r\n", shdr_seq, j, (UINT32)p_sie_head_info->vdo_frm.count, p_outq->push_cnt[j], nvtmpp_vb_blk2va(p_data->h_data),p_outq->output_used[j]);
				return;
			}
			for (k = 0; k < shdr_frm_num; k++) {
				p_data = &p_outq->output_data[j][k];
				p_vdofrm = (VDO_FRAME *)(p_data->desc);
				p_vdofrm->pxlfmt |= ((k & 0xF)<< 12);
				#if (FORCE_DUMP_DATA == ENABLE)
				if (oport == WATCH_PORT) {
					DBG_DUMP("j%d-%d[%d] pxlfmt=%08X\r\n", j, (UINT32)p_sie_head_info->vdo_frm.count, k, p_vdofrm->pxlfmt);
				}
				#endif
				if (_vcap_gyro_obj[0] && p_ctx->gyro_info.en && k == 0) {
					CTL_SIE_GYRO_CFG gyro_cfg = {0};

					p_vdofrm->phyaddr[VDO_MAX_PLANE-1] = nvtmpp_sys_va2pa(p_vdofrm->reserved[1]);
					ctl_sie_get(p_ctx->sie_hdl, CTL_SIE_ITEM_GYRO_CFG, (void *)&gyro_cfg);
					//use loff[3] to store latency info
					p_vdofrm->loff[VDO_MAX_PLANE-1] = gyro_cfg.latency_en;
				}
				#if 1
				if (unlikely(8 <= isf_vdocap_debug_level) && _vcap_gyro_obj[0] && p_ctx->gyro_info.en && k == 0) {
					UINT32 data_num;
					INT32 *p_data;
					ULONG rsv1 = p_vdofrm->reserved[1];
					CTL_SIE_EXT_GYRO_DATA gyro_data;

					DBG_DUMP("[%llu]gyro pa=0x%X\r\n", p_vdofrm->count, p_vdofrm->phyaddr[VDO_MAX_PLANE-1]);

					if (rsv1) {
						gyro_data.data_num = (UINT32 *)(rsv1);
						data_num = (*gyro_data.data_num);
						gyro_data.agyro_x =  (INT32 *)(rsv1 + sizeof(*gyro_data.data_num));
						gyro_data.agyro_y =  (INT32 *)(rsv1 + sizeof(*gyro_data.data_num) + (1 * (*gyro_data.data_num) * sizeof(INT32)));
						gyro_data.agyro_z =  (INT32 *)(rsv1 + sizeof(*gyro_data.data_num) + (2 * (*gyro_data.data_num) * sizeof(INT32)));
						gyro_data.ags_x = (INT32 *)(rsv1 + sizeof(*gyro_data.data_num) + (3 * (*gyro_data.data_num) * sizeof(INT32)));
						gyro_data.ags_y = (INT32 *)(rsv1 + sizeof(*gyro_data.data_num) + (4 * (*gyro_data.data_num) * sizeof(INT32)));
						gyro_data.ags_z = (INT32 *)(rsv1 + sizeof(*gyro_data.data_num) + (5 * (*gyro_data.data_num) * sizeof(INT32)));


						DBG_DUMP("data_num=%d\r\n", data_num);
						p_data = gyro_data.agyro_x;
						DBG_DUMP("agyro_x\r\n");
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																					*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
						p_data = gyro_data.ags_z;
						DBG_DUMP("ags_z\r\n");
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																					*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					}
				}
				#endif

			}
			if (p_ctx->pullq.num[i]) {
				//for shdr pull
				for (k = 0; k < shdr_frm_num; k++) {
					UINT32 p;

					p_data = &p_outq->output_data[j][k];
					p_vdofrm = (VDO_FRAME *)(p_data->desc);
					for (p = 0; p < VDO_MAX_PLANE; p++) {
						if (p_vdofrm->addr[p] != 0) {
							p_vdofrm->phyaddr[p] = nvtmpp_sys_va2pa(p_vdofrm->addr[p]);
						}
					}
					#if (FORCE_DUMP_DATA == ENABLE)
					if (oport == WATCH_PORT) {
						DBG_DUMP("j%d-%d[%d] phyaddr[0]=0x%X\r\n", j, (UINT32)p_sie_head_info->vdo_frm.count, k, p_vdofrm->phyaddr[0]);
					}
					#endif
				}
				for (k = 0; k < shdr_frm_num; k++) {
					p_data = &p_outq->output_data[j][k];
					//p_vdofrm = (VDO_FRAME *)(p_data->desc);

					if(_isf_vdocap_oqueue_do_push_with_clean(p_thisunit, oport, p_data, 0) == ISF_OK) {
						//DBG_DUMP("Q+[%d]j%d-%d-%X\r\n",k,j,(UINT32)p_vdofrm->count,p_vdofrm->pxlfmt);
						loc_cpu_flow(flow_flags);
						p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW);
						//_vdocap_shdr_oport_adddata(p_outq, j);
						unl_cpu_flow(flow_flags);
					} else {
						//DBG_DUMP("Q+ fail[%d]j%d-%d-%X\r\n",k,j,(UINT32)p_vdofrm->count,p_vdofrm->pxlfmt);
					}
				}
			}
			if (p_port && p_thisunit->p_base->get_is_allow_push(p_thisunit, p_port)) {
				#if (SHDR_QUEUE_CHECK == ENABLE)
				DBG_DUMP("#j%d-%d\r\n",j,(UINT32)p_sie_head_info->vdo_frm.count);
				#endif
				for (k = 0; k < shdr_frm_num; k++) {
					INT32 ret = 0;

					p_data = &p_outq->output_data[j][k];
					#if (SHDR_QUEUE_CHECK == ENABLE)
					DBG_DUMP("0x%X\r\n", nvtmpp_vb_blk2va(p_data->h_data));
					#endif

					#if 0
					//only for ctl_sie catching raw, these blocks should be locked until ctl_sie unlock them.
					if (p_outq->output_used[j] > 1) {
						p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW);
					}
					#endif
					ret = p_thisunit->p_base->do_push(p_thisunit, oport, p_data, 0);
					if (ret) {
						p_vdofrm = (VDO_FRAME *)(p_data->desc);
						DBG_MSG("[%d]j%d-%d-%d fmt0x%X PUSH 0x%X failed!(%d)\r\n", shdr_seq, j, (UINT32)p_sie_head_info->vdo_frm.count, k,p_vdofrm->pxlfmt,nvtmpp_vb_blk2va(p_data->h_data),ret);
					}
					_vdocap_shdr_oport_releasedata(p_outq, j);
					p_outq->addr[j][k] = 0;
					p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH, ISF_OK);
					#if SHDR_QUEUE_DEBUG
					p_outq->do_push[shdr_seq]++;
					#endif
				}
				//output rate debug
				if (output_rate_update_pause == FALSE) {
					unsigned long flags;

					loc_cpu(flags);
					p_ctx->out_dbg.t[i][p_ctx->out_dbg.idx] = hwclock_get_longcounter();
					p_ctx->out_dbg.idx = (p_ctx->out_dbg.idx + 1) % VDOCAP_DBG_TS_MAXNUM;
					unl_cpu(flags);
				}
			} else {
				#if (FORCE_DUMP_DATA == ENABLE)
				if (oport == WATCH_PORT) {
					DBG_DUMP("vdocap_shdr[%d][%d] drop2 -- Vdo: blk_id=%08x blk[%d].cnt=%d, frm_cnt=%lld\r\n", shdr_map, (UINT32)p_sie_head_info->vdo_frm.count, p_data->h_data,  j, p_outq->output_used[j], p_ctx->vd_count);
				}
				#endif
				loc_cpu_flow(flow_flags);
				for (k = 0; k < shdr_frm_num; k++) {
					p_data = &p_outq->output_data[j][k];
					p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
					_vdocap_shdr_oport_releasedata(p_outq, j);
					p_outq->addr[j][k] = 0;
					//unbind mode and no queue
					if (0 == p_ctx->pullq.num[i]) {
						p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_NOT_CONNECT_YET);
					}
					#if SHDR_QUEUE_DEBUG
					p_outq->push_drop[shdr_seq]++;
					#endif
				}
				unl_cpu_flow(flow_flags);
			}
		}
	}
}
static UINT32 _vdocap_header_to_seq(CTL_SIE_HEADER_INFO *p_sie_head_info, VDOCAP_SEN_HDR_MAP shdr_map)
{
	UINT32 k, shdr_frm_num;
	UINT32 shdr_set;
	VDOCAP_SHDR_OUT_QUEUE *p_outq;
	UINT32 j;
	UINT32 shdr_seq;

	shdr_set = _vdocap_shdr_map_to_set(shdr_map);
	shdr_frm_num = _vdocap_shdr_frm_num[shdr_set];
	p_outq = &_vdocap_shdr_queue[shdr_set];
	j = (p_sie_head_info->buf_id & ~0xffff0000);

	//set shdr_seq to un_initialized value
	shdr_seq = shdr_frm_num;
	for (k = 0; k < shdr_frm_num; k++) {
		if (p_outq->addr[j][k] == p_sie_head_info->buf_addr) {
			shdr_seq = k;
			break;
		}
	}
	if (shdr_seq < shdr_frm_num) {
		return shdr_seq;
	} else {
		//DBG_WRN("unlock addr error!(0x%X)\r\n", p_sie_head_info->buf_addr);
		//the case of shdr direct + "ctl sie save raw", all p_outq->addr have been released
		return _vdocap_shdr_map_to_seq(shdr_map);
	}
}
void _isf_vdocap_shdr_oport_do_lock(ISF_UNIT *p_thisunit, UINT32 oport, void *p_header_info, UINT32 lock, VDOCAP_SEN_HDR_MAP shdr_map)
{
	CTL_SIE_HEADER_INFO *p_sie_head_info = (CTL_SIE_HEADER_INFO *)p_header_info;
	UINT32 j;
	ISF_DATA *p_data;
	VDOCAP_SHDR_OUT_QUEUE *p_outq;
	UINT32 shdr_set;
	UINT32 shdr_seq;
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	unsigned long flow_flags;

	if (p_ctx->sie_hdl == 0) {
		DBG_ERR("ctl sie cb error!\r\n");
		return;
	}
	shdr_set = _vdocap_shdr_map_to_set(shdr_map);
	shdr_seq = _vdocap_shdr_map_to_seq(shdr_map);
	p_outq = &_vdocap_shdr_queue[shdr_set];

	j = (p_sie_head_info->buf_id & ~0xffff0000);

	//DBG_DUMP("[%d] buf_id =0x%X  fc=%d lock=%d\r\n", shdr_seq,p_sie_head_info->buf_id,(UINT32)p_sie_head_info->vdo_frm.count,lock);
	//eis trigger in direct mode
	if (_vcap_gyro_obj[0] && p_ctx->gyro_info.en && lock && (p_sie_head_info->buf_id == 0xFFFFFFFF)) {
		if (_vdocap_is_direct_flow(p_ctx) && p_sie_head_info->buf_id == 0xFFFFFFFF) {
			if (p_ctx->eis_trig_cb) {
				p_ctx->eis_trig_cb(&p_sie_head_info->vdo_frm);
			}
			return;
		}
	}

	//special memory unlock for SHDR+direct
	if (p_sie_head_info->buf_id == 0xFFFFFFFF) {
		DBG_MSG("VDOCAP[%d] unlock SIE2 ring buffer\r\n", p_ctx->id);
		if (p_thisunit->p_base->do_release_i(p_thisunit, &sie2_ring_buffer, ISF_OK)) {
			DBG_ERR("vdocap_shdr[%d] SIE2 ring buffer release failed!\r\n", shdr_map);
		}
		return;
	}

	if (j >= VDOCAP_OUT_DEPTH_MAX) {
		DBG_ERR("vdocap_shdr[%d] lock = %d -- Vdo: blk[%d???] IGNORE!!!!!\r\n", shdr_map, lock, j);
		return;
	}
	if (_vdocap_is_direct_flow(p_ctx)) {
		//shdr_map is always main path in direct mode, so we need to revise it
		shdr_seq = _vdocap_header_to_seq(p_sie_head_info, shdr_map);
	}
	p_data = &p_outq->output_data[j][shdr_seq];
	loc_cpu_flow(flow_flags);
	#if (SHDR_QUEUE_CHECK == ENABLE)
	{
		UINT32 addr;
		CHAR *lock_str[] = {
			"UnLock",
			"Lock",
		};

		addr = nvtmpp_vb_blk2va(p_data->h_data);
		DBG_DUMP("%s[%d]j%d-%d-0x%X used=%d header addr=0x%X, Qaddr[j][0]=0x%X, [j][1]=0x%X\r\n",
						lock_str[lock], shdr_seq,j,p_outq->frame_cnt[j],addr,p_outq->output_used[j],
						p_sie_head_info->buf_addr, p_outq->addr[j][0], p_outq->addr[j][1]);
	}
	#endif
	if (lock) {
		p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW);
		_vdocap_shdr_oport_adddata(p_outq, j);
		DBG_MSG("vdocap_shdr[%d][%d] lock j=%d output_used=%d,addr %x\r\n", shdr_map, shdr_seq, j, p_outq->output_used[j], p_sie_head_info->buf_addr);
		#if SHDR_QUEUE_DEBUG
		p_outq->lock[shdr_seq]++;
		#endif
	} else {
		if (p_outq->addr[j][shdr_seq]) {//ctl_sie MIGHT unlock the block which has been pushed!
			UINT32 shdr_frm_num = _vdocap_shdr_frm_num[shdr_set];
			BOOL isp_tool_unlock = FALSE;

			if (p_outq->output_used[j] > shdr_frm_num) {
				isp_tool_unlock = TRUE;
			}

			if (p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW)) {
				DBG_ERR("vdocap_shdr[%d][%d] unlock release failed! j[%d].cnt=%d\r\n", shdr_map, shdr_seq,  j, p_outq->output_used[j]);
			}
			#if SHDR_QUEUE_DEBUG
			p_outq->unlock_release[shdr_seq]++;
			#endif
			if (FALSE == _vdocap_is_direct_flow(p_ctx)) {
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC_WRN, ISF_ERR_INACTIVE_STATE);
			}
			_vdocap_shdr_oport_releasedata(p_outq, j);
			if (isp_tool_unlock == FALSE) {
				p_outq->force_drop[j] = 1;
				p_outq->addr[j][shdr_seq] = 0;
				#if SHDR_QUEUE_DEBUG
				p_outq->unlock[shdr_seq]++;
				#endif
			}
			//case7: N0 -> N1 -> L0 -> P0 -> P1 -> U0
			//(p_outq->push_cnt[j] == shdr_frm_num)


			//case6: N0 -> N1 -> P1 + U0 racing
			//U0 check push_count and release pushed block
			if (p_outq->push_cnt[j] && p_outq->push_cnt[j] < shdr_frm_num && isp_tool_unlock == FALSE) {
				UINT32 k;

				for (k = 0; k < shdr_frm_num; k++) {
					if (p_outq->addr[j][k]) {
						if (unlikely(NVT_DBG_VALUE <= isf_vdocap_debug_level) || SHDR_QUEUE_CHECK) {
							DBG_DUMP("release pushed [%d]j%d-%d-0x%X used=%d\r\n", k,j,p_outq->frame_cnt[j],p_outq->addr[j][k],p_outq->output_used[j]);
						}
						p_data = &p_outq->output_data[j][k];
						p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
						_vdocap_shdr_oport_releasedata(p_outq, j);
						p_outq->addr[j][k] = 0;
					}
				}
			}
		} else {
			DBG_MSG("[%d][%d] ctl_sie unlock the block which has been pushed! j[%d].cnt=%d\r\n", shdr_map, shdr_seq,  j, p_outq->output_used[j]);
			if (p_outq->output_used[j]) {
				if (p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW)) {
					DBG_ERR("vdocap_shdr[%d][%d] unlock release failed! j[%d].cnt=%d\r\n", shdr_map, shdr_seq,  j, p_outq->output_used[j]);
				}
				if (FALSE == _vdocap_is_direct_flow(p_ctx)) {
					p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC_WRN, ISF_ERR_INACTIVE_STATE);
				}
				_vdocap_shdr_oport_releasedata(p_outq, j);
			}
		}
	}
	unl_cpu_flow(flow_flags);
}
/*
static void _vdocap_oport_dumpdata(UINT32 id, UINT32 oport)
{
	UINT32 i = oport - ISF_OUT(0);
	UINT32 j;
	unsigned long flags;

	loc_cpu(flags);
	DBG_DUMP("OutPOOL[%d]={", i);
	for (j = 0; j < g_output_max[id][i]; j++) {
		if (g_output_used[id][i][j] == 1) {
			DBG_DUMP("[%d]=%08x", j, g_output_data[id][i][j].h_data);
		}
	}
	DBG_DUMP("}\r\n");
	unl_cpu(flags);
}
*/
void _vdocap_oport_initqueue(ISF_UNIT *p_thisunit)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 i, j;

	for (i = 0; i < ISF_VDOCAP_OUT_NUM; i++) {
		p_ctx->outq.output_max[i] = VDOCAP_OUT_DEPTH_MAX;
		for (j = 0; j < VDOCAP_OUT_DEPTH_MAX; j++) {
			p_ctx->outq.output_data[i][j] = zero;
			p_ctx->outq.output_used[i][j] = 0;
		}
	}
}
void _vdocap_oport_block_check(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	UINT32 j;
	unsigned long flags;

	loc_cpu(flags);
	for (j = 0; j < p_ctx->outq.output_max[i]; j++) {
		if (p_ctx->outq.output_used[i][j] != 0) {
			ISF_DATA *p_data = (ISF_DATA *)&(p_ctx->outq.output_data[i][j]);
			UINT32 addr = 0;

			if (p_data->h_data != 0) {
				addr = nvtmpp_vb_blk2va(p_data->h_data);
			}
			DBG_WRN("release blk[%d] blk_id=%08x addr=%08x .cnt=%d\r\n", j, p_data->h_data, addr, p_ctx->outq.output_used[i][j]);
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_EXT_WRN);
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_EXT_WRN, ISF_OK);
		}
	}
	unl_cpu(flags);
}

static UINT32 _vdocap_oport_newdata(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	UINT32 j;
	ISF_DATA *p_data = NULL;
	unsigned long flags;

	loc_cpu(flags);
	for (j = 0; j < p_ctx->outq.output_max[i]; j++) {
		if (p_ctx->outq.output_used[i][j] == 0) {
			//DBG_DUMP("^C+1\r\n");
			p_ctx->outq.output_used[i][j] = 1;
			p_data = &(p_ctx->outq.output_data[i][j]);
			p_data->desc[0] = 0; //clear
			p_data->h_data = 0; //clear
			//p_data->serial = 0;
			#if (QUEUE_TRACE == ENABLE)
			DBG_DUMP("[%d]output_used[%d][%d] new\r\n", p_ctx->id, i, j);
			#endif
			unl_cpu(flags);
			return j;
		}
	}
	#if 0
	for (j = 0; j < p_ctx->outq.output_max[i]; j++) {
		DBG_DUMP("[%d]output_used[%d][%d]=%d\r\n", p_ctx->id, i, j , p_ctx->outq.output_used[i][j]);
	}
	#endif
	unl_cpu(flags);
	return 0xff;
}

static UINT32 _vdocap_oport_adddata(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 j)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	unsigned long flags;

	loc_cpu(flags);
	p_ctx->outq.output_used[i][j] += 1;
	#if (QUEUE_TRACE == ENABLE)
	DBG_DUMP("[%d]output_used[%d][%d] ++ => %d\r\n", p_ctx->id, i, j , p_ctx->outq.output_used[i][j]);
	#endif
	unl_cpu(flags);
	return 1;
}
UINT32 _vdocap_oport_releasedata(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 j)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	unsigned long flags;

	loc_cpu(flags);
	if (p_ctx->outq.output_used[i][j] == 0) {
		DBG_WRN("-[%d]output_used[%d][%d] already released!\r\n", p_ctx->id, i, j);
		unl_cpu(flags);
		return 0;
	}
	//DBG_DUMP("^C-1\r\n");
	p_ctx->outq.output_used[i][j] -= 1;
	#if (QUEUE_TRACE == ENABLE)
	DBG_DUMP("[%d]output_used[%d][%d] -- => %d\r\n", p_ctx->id, i, j , p_ctx->outq.output_used[i][j]);
	#endif
	//if (p_ctx->outq.output_used[i][j] == 0)
	//	p_data->h_data = 0; //clear
	unl_cpu(flags);
	return 1;
}
#if 0
static void _vdocap_oport_dumpqueue(ISF_UNIT *p_thisunit)
{
	UINT32 j, i;
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	unsigned long flags;

	loc_cpu(flags);
	DBG_DUMP("^YPush Queue\r\n");
	for (i = 0; i < ISF_VDOCAP_OUT_NUM; i++)
		for (j = 0; j < VDOCAP_OUT_DEPTH_MAX; j++) {
			ISF_DATA *p_data = (ISF_DATA *)&(p_ctx->outq.output_data[i][j]);
			UINT32 addr = 0;

			if (p_data->h_data != 0)
				//addr = nvtmpp_vb_blk2pa(p_data->h_data);
				addr = nvtmpp_vb_blk2va(p_data->h_data);
			DBG_DUMP("%s.out[%d] - Vdo: blk_id=%08x addr=%08x blk[%d].cnt=%d\r\n", p_thisunit->unit_name, i, p_data->h_data, addr, j, p_ctx->outq.output_used[i][j]);
		}
	DBG_DUMP("^YCommon blk\r\n");
	nvtmpp_dump_status(debug_msg);
	unl_cpu(flags);
}
#endif
void _isf_vdocap_oport_do_sync(UINT32 id, struct _ISF_UNIT *p_thisunit, UINT32 oport, UINT64 end, UINT64 cycle)
{
}

void _isf_vdocap_oport_do_new(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_size, UINT32 ddr, void *p_header_info)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
#if (DUMP_DATA_ERROR == ENABLE)
	UINT32 id = p_ctx->id;
#endif
	UINT32 i = oport - ISF_OUT_BASE;
	UINT32 j;
	ISF_PORT *p_port = p_thisunit->port_out[i];
	ISF_DATA *p_data = NULL;
	UINT32 addr = 0;
	CTL_SIE_HEADER_INFO *p_sie_head_info = (CTL_SIE_HEADER_INFO *)p_header_info;
	static ISF_DATA_CLASS g_vdocap_out_data = {0};

	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW, ISF_ENTER);

	//p_ctx->vd_count = p_sie_head_info->vdo_frm.count;
	if (FALSE == p_ctx->count_vd_by_sensor) {
		p_ctx->vd_count++;
	}

#if (DUMP_FPS == ENABLE)
	if (i == WATCH_PORT) {
		DBG_FPS(p_thisunit, "out", oport-ISF_OUT_BASE, "New");
	}
#endif
	if ((p_ctx->dev_ready == 0) || (p_ctx->sie_hdl == 0)) {
		if (p_ctx->dev_trigger_open == 1) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_DROP, ISF_ERR_NOT_START);
		} if (p_ctx->dev_trigger_close == 1) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_DROP, ISF_ERR_NOT_START);
		} else {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_ERR, ISF_ERR_INACTIVE_STATE);
		}
		p_sie_head_info->buf_id = 0;
		p_sie_head_info->buf_addr = 0;
		return; //drop frame!
	}
	if (p_port) {
		p_ctx->outq.output_connecttype[i] = p_port->connecttype;
	} else {
		p_ctx->outq.output_connecttype[i] = ISF_CONNECT_NONE;
	}

	//do frc
	if (_isf_frc_is_select(p_thisunit, oport, &(p_ctx->outfrc[i])) == 0) {
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_DROP, ISF_ERR_FRC_DROP);
		p_sie_head_info->buf_id = 0;
		p_sie_head_info->buf_addr = 0;
		return; //drop frame!
	}

	j = _vdocap_oport_newdata(p_thisunit, oport);
	//if (i == WATCH_PORT){	DBG_DUMP("^Cnew-b %08x\r\n",j); }
	if (j == 0xff) {
		p_sie_head_info->buf_id = 0;
		p_sie_head_info->buf_addr = 0;
#if (DUMP_DATA_ERROR == ENABLE)
		if (oport == WATCH_PORT) {
			DBG_WRN("vdocap%d.out[%d]! New -- overflow-1!!!\r\n", id, oport);
			//_vdocap_sync_dumpdata(id, oport);
			//_vdocap_oport_dumpdata(id, oport);
		}
#endif
		//DBG_DUMP("[%d]j%X-%d\r\n",p_ctx->id,j,frame_cnt);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_WRN, ISF_ERR_QUEUE_FULL);
		return;
	}
	//DBG_DUMP("[%d]j%d-%d\r\n",p_ctx->id,j,frame_cnt);
	/* record sie out size for debuging */
	p_ctx->out_buf_size[i] = buf_size;

	p_data = &p_ctx->outq.output_data[i][j];

	p_thisunit->p_base->init_data(p_data, &g_vdocap_out_data, MAKEFOURCC('V','F','R','M'), 0x00010000);

	if (p_ctx->one_buf && (p_ctx->outq.force_onebuffer[i] != 0)) { //under "single buffer mode", if try to new 2nd buffer
		p_data->h_data = p_ctx->outq.force_onebuffer[i];
		addr = nvtmpp_vb_blk2va(p_data->h_data);
		p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to this "single buffer"
		if (addr == 0) {
			DBG_ERR("one_buf error h_data=0x%X\r\n", p_data->h_data);
		}
	} else {
		if (!nvtmpp_is_dynamic_map())
			addr = p_thisunit->p_base->do_new(p_thisunit, oport, NVTMPP_VB_INVALID_POOL, ddr, buf_size, p_data, ISF_OUT_PROBE_NEW);
		else
			addr = p_thisunit->p_base->do_new(p_thisunit, oport, NVTMPP_VB_VCAP_OUT_POOL, ddr, buf_size, p_data, ISF_OUT_PROBE_NEW);
		if (addr && p_ctx->one_buf) {
			DBG_UNIT("%s.out[%d]! Start single blk mode => blk_id=%08x addr=%08x\r\n", p_thisunit->unit_name, oport, p_data->h_data, addr);
			p_ctx->outq.force_onebuffer[i] = p_data->h_data; //keep "single buffer" and start "single buffer mode"
			p_ctx->outq.force_onesize[i] = buf_size; //keep "single buffer" and start "single buffer mode"
			p_ctx->outq.force_onej[i] = j;
			p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to this "single buffer"
		}
	}
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("vdocap%d.out[%d]! New from Pool -- Vdo: size=%d ok => blk_id=%08x addr=%08x\r\n", id, oport, buf_size, p_data->h_data, addr);
	}
#endif

	if (addr == 0) {
#if (DUMP_DATA_ERROR == ENABLE)
		DBG_ERR("vdocap%d.out[%d]! New -- Vdo: size=%d faild!\r\n", id, oport, buf_size);
#endif
		//p_data->serial = 0;
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_WRN, ISF_ERR_NEW_FAIL);
		_vdocap_oport_releasedata(p_thisunit, oport, j);
		p_sie_head_info->buf_id = 0;
		p_sie_head_info->buf_addr = 0;
		return;
	}

#if (FORCE_DUMP_DATA == ENABLE)
		//if (oport == WATCH_PORT) {
			//DBG_DUMP("vdocap%d.out[%d]! New -- Vdo: size=%d ok => blk_id=%08x addr=%08x blk[%d].cnt=%d\r\n", id, oport, buf_size, p_data->h_data, addr, j, g_output_used[id][i][j]);
		//}
#endif
	p_sie_head_info->buf_id = (UINT32)(j | 0xffff0000);
	p_sie_head_info->buf_addr = addr;
	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW, ISF_OK);
	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC, ISF_ENTER);
	return;
}

void _isf_vdocap_oport_do_push(ISF_UNIT *p_thisunit, UINT32 oport, void *p_header_info)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 id = p_ctx->id;
	UINT32 i = oport - ISF_OUT(0);
	UINT32 j;
	ISF_PORT *p_port = p_thisunit->port_out[i];
	UINT32 addr;
	ISF_DATA *p_data;
	CTL_SIE_HEADER_INFO *p_sie_head_info = (CTL_SIE_HEADER_INFO *)p_header_info;
	BOOL push_ok = FALSE;
	BOOL push_que = FALSE;
	ISF_RV pr = ISF_OK; //push result
	BOOL enqueue_ok = FALSE;

	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC, ISF_OK);
	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH, ISF_ENTER);

	j = (p_sie_head_info->buf_id & ~0xffff0000);
	//DBG_DUMP("-[%d]j%d-%d\r\n",p_ctx->id,j,(UINT32)p_sie_head_info->vdo_frm.count);
	// NOTE! under IPL direct mode, the last push maybe enter with NULL p_port.
	if (j >= VDOCAP_OUT_DEPTH_MAX) {
		DBG_ERR("vdocap%d.out[%d]! Push -- Vdo: blk[%d???] IGNORE!!!!!\r\n", id, oport, j);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_QUEUE_ERROR);
		return;
	}

	if (unlikely(NVT_DBG_USER <= isf_vdocap_debug_level)) {
		#ifdef __KERNEL__
		DBG_DUMP("[%d]%u\r\n", id, (UINT32)div_u64(p_sie_head_info->vdo_frm.timestamp, 1000));
		#else
		DBG_DUMP("[%d]%u\r\n", id, (UINT32)(p_sie_head_info->vdo_frm.timestamp/1000));
		#endif
	}

	addr = p_sie_head_info->vdo_frm.addr[0];
	p_data = (ISF_DATA *)&p_ctx->outq.output_data[i][j];
	p_data->func = j;

	//if (sizeof(VDO_FRAME) <= sizeof(p_data->desc) //already have STATIC_ASSERT check
	memcpy(p_data->desc, &p_sie_head_info->vdo_frm, sizeof(VDO_FRAME));

	{
		int p;
		VDO_FRAME *p_vdoframe = (VDO_FRAME *)(p_data->desc);

		for (p = 0; p < VDO_MAX_PLANE; p++) {
			if (p_vdoframe->addr[p] != 0) {
				p_vdoframe->phyaddr[p] = nvtmpp_sys_va2pa(p_vdoframe->addr[p]);
			}
		}
		if (_vcap_gyro_obj[0] && p_ctx->gyro_info.en) {
			CTL_SIE_GYRO_CFG gyro_cfg = {0};

			p_vdoframe->phyaddr[VDO_MAX_PLANE-1] = nvtmpp_sys_va2pa(p_vdoframe->reserved[1]);
			ctl_sie_get(p_ctx->sie_hdl, CTL_SIE_ITEM_GYRO_CFG, (void *)&gyro_cfg);
			//use loff[3] to store latency info
			p_vdoframe->loff[VDO_MAX_PLANE-1] = gyro_cfg.latency_en;
		}
		//DBG_DUMP("id=%X, addr=0x%X, rsv1=0x%X\r\n",p_sie_head_info->buf_id, p_sie_head_info->buf_addr, p_vdoframe->reserved[1]);
		if (unlikely(8 <= isf_vdocap_debug_level) && _vcap_gyro_obj[0] && p_ctx->gyro_info.en) {
			UINT32 data_num;
			INT32 *p_data;
			UINT32 rsv1 = p_vdoframe->reserved[1];
			CTL_SIE_EXT_GYRO_DATA gyro_data;

			DBG_DUMP("[%llu]gyro pa=0x%X\r\n", p_vdoframe->count, p_vdoframe->phyaddr[VDO_MAX_PLANE-1]);

			if (rsv1) {
				gyro_data.data_num = (UINT32 *)(rsv1);
				data_num = (*gyro_data.data_num);
				gyro_data.agyro_x =  (INT32 *)(rsv1 + sizeof(*gyro_data.data_num));
				gyro_data.agyro_y =  (INT32 *)(rsv1 + sizeof(*gyro_data.data_num) + (1 * (*gyro_data.data_num) * sizeof(INT32)));
				gyro_data.agyro_z =  (INT32 *)(rsv1 + sizeof(*gyro_data.data_num) + (2 * (*gyro_data.data_num) * sizeof(INT32)));
				gyro_data.ags_x = (INT32 *)(rsv1 + sizeof(*gyro_data.data_num) + (3 * (*gyro_data.data_num) * sizeof(INT32)));
				gyro_data.ags_y = (INT32 *)(rsv1 + sizeof(*gyro_data.data_num) + (4 * (*gyro_data.data_num) * sizeof(INT32)));
				gyro_data.ags_z = (INT32 *)(rsv1 + sizeof(*gyro_data.data_num) + (5 * (*gyro_data.data_num) * sizeof(INT32)));


				DBG_DUMP("data_num=%d\r\n", data_num);
				p_data = gyro_data.agyro_x;
				DBG_DUMP("agyro_x\r\n");
				DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
				p_data = gyro_data.ags_z;
				DBG_DUMP("ags_z\r\n");
				DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
			}
		}
	}

	if (p_port) {
		p_ctx->outq.output_connecttype[i] = p_port->connecttype;
	} else {
		p_ctx->outq.output_connecttype[i] = ISF_CONNECT_NONE;
	}

	_isf_vdocap_check_csi_status(p_ctx);
#if 1
	if (p_ctx->outq.output_used[i][j] == 0) {
		DBG_ERR("vdocap%d.out[%d]! Push -- Vdo: blk_id=%08x addr=%08x blk[%d].cnt=%d? IGNORE!!!!!\r\n", id, oport, p_data->h_data, addr, j, p_ctx->outq.output_used[i][j]);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_INVALID_DATA);
		//_vdocap_oport_dumpqueue(p_thisunit);
		return;
	}
#endif
	//if (i == WATCH_PORT){	DBG_DUMP("^Mpush-b %08x\r\n",j); }
#if (DUMP_FPS == ENABLE)
	if (i == WATCH_PORT) {
		DBG_FPS(p_thisunit, "out", oport-ISF_OUT_BASE, "Push");
	}
#endif

	if ((p_ctx->dev_ready == 0) || (p_ctx->sie_hdl == 0)) {
		//DBG_ERR("vdocap%d.out[%d]! Push NOT_START j[%d]\r\n", id, oport, j);
		p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
		_vdocap_oport_releasedata(p_thisunit, oport, j);
		if (p_ctx->dev_trigger_open == 1) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_NOT_START);
		} if (p_ctx->dev_trigger_close == 1) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_NOT_START);
		} else {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_INACTIVE_STATE);
		}
		return;
	}

	if (p_ctx->pullq.num[i]) {
		push_que = TRUE;
		p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
	}

	if (p_port && p_thisunit->p_base->get_is_allow_push(p_thisunit, p_port)) {
		#if (FORCE_DUMP_DATA == ENABLE)
		if (oport == WATCH_PORT) {
			DBG_DUMP("vdocap%d.out[%d]! Push -- Vdo: blk_id=%08x addr=%08x blk[%d].cnt=%d\r\n", id, oport, p_data->h_data, addr, j, p_ctx->outq.output_used[i][j]);
		}
		#endif
		p_thisunit->p_base->do_push(p_thisunit, oport, p_data, 0);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH, ISF_OK);
		push_ok = TRUE;
	}
	if (push_que) {
		pr = _isf_vdocap_oqueue_do_push_with_clean(p_thisunit, oport, p_data, 1); //keep this data
		if (pr == ISF_OK) {
			enqueue_ok = TRUE;
		}
		p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
	}

	if (push_ok) {
		//output rate debug
		if (output_rate_update_pause == FALSE) {
			unsigned long flags;

			loc_cpu(flags);
			p_ctx->out_dbg.t[i][p_ctx->out_dbg.idx] = hwclock_get_longcounter();
			p_ctx->out_dbg.idx = (p_ctx->out_dbg.idx + 1) % VDOCAP_DBG_TS_MAXNUM;
			unl_cpu(flags);
		}
	} else {
		#if (FORCE_DUMP_DATA == ENABLE)
		if (oport == WATCH_PORT) {
			DBG_DUMP("vdocap%d.out[%d]! drop2 -- Vdo: blk_id=%08x addr=%08x blk[%d].cnt=%d\r\n", id, oport, p_data->h_data, addr, j, p_ctx->outq.output_used[i][j]);
		}
		#endif
		if (p_ctx->outq.output_used[i][j] == 0) {
			DBG_ERR("vdocap%d.out[%d]! drop2 -- Vdo: blk_id=%08x addr=%08x blk[%d].cnt=%d? IGNORE!!!!!\r\n", id, oport, p_data->h_data, addr, j, p_ctx->outq.output_used[i][j]);
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH_ERR);
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_QUEUE_ERROR);
		} else {
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
			if ((p_port) && p_thisunit->p_base->get_is_allow_push(p_thisunit, p_port)) {
				if (!push_ok) {
					p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_NOT_ALLOW);
				}
			} else {
				if (FALSE == push_que) {
					//unbind mode and no queue
					p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_NOT_CONNECT_YET);
				} else if (!enqueue_ok) {
					if (pr == ISF_ERR_QUEUE_FULL) {
						p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_DROP, pr);
					} else {
						p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_WRN, pr);
					}
				}
			}
		}
	}
	//DBG_DUMP("r[%d]j%d-%d\r\n",p_ctx->id,j,(UINT32)p_sie_head_info->vdo_frm.count);
	_vdocap_oport_releasedata(p_thisunit, oport, j);

}

void _isf_vdocap_oport_do_lock(ISF_UNIT *p_thisunit, UINT32 oport, void *p_header_info, UINT32 lock)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 id = p_ctx->id;
	UINT32 i = oport - ISF_OUT(0);
	CTL_SIE_HEADER_INFO *p_sie_head_info = (CTL_SIE_HEADER_INFO *)p_header_info;
	UINT32 j;
	ISF_DATA *p_data;

	if (p_ctx->sie_hdl == 0) {
		DBG_ERR("ctl sie cb error!\r\n");
		return;
	}

	//eis trigger in direct mode
	if (_vcap_gyro_obj[0] && p_ctx->gyro_info.en) {
		if (_vdocap_is_direct_flow(p_ctx) && p_sie_head_info->buf_id == 0xFFFFFFFF) {
			if (p_ctx->eis_trig_cb) {
				p_ctx->eis_trig_cb(&p_sie_head_info->vdo_frm);
			}
			return;
		}
	}

	j = (p_sie_head_info->buf_id & ~0xffff0000);


	if (j >= VDOCAP_OUT_DEPTH_MAX) {
		DBG_ERR("vdocap%d.out[%d]! lock = %d -- Vdo: blk[%d???] IGNORE!!!!!\r\n", id, oport, lock, j);
		return;
	}
	p_data = (ISF_DATA *)&p_ctx->outq.output_data[i][j];
	#if (FORCE_DUMP_DATA == ENABLE)
	{
		UINT32 addr;

		addr = nvtmpp_vb_blk2va(p_data->h_data);
		DBG_DUMP("[%d]j%d-%d-0x%X lock=%d used=%d\r\n", id,j,(UINT32)p_sie_head_info->vdo_frm.count,addr,lock,p_ctx->outq.output_used[i][j]);
	}
	#endif
	if (lock) {
		p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW);
		_vdocap_oport_adddata(p_thisunit, oport, j);
//DBGD(p_sie_head_info->buf_id & ~0xffff0000);
	} else {
		p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW);
		if (FALSE == _vdocap_is_direct_flow(p_ctx)) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_DROP, ISF_ERR_INACTIVE_STATE);
		}
//DBGH(j);
		_vdocap_oport_releasedata(p_thisunit, oport, j);
	}

}
static void _isf_vdocap_oport_enable(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	//unsigned long flags;

	if (p_ctx->outq.output_cur_en[i] == TRUE)
		return;
	p_ctx->outq.output_cur_en[i] = TRUE;
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
	DBG_DUMP("^C%s.out[%d]! ----------------------------------------------- Start Begin\r\n", p_thisunit->unit_name, oport);
	}
#endif
#if 1
	_vdocap_oport_block_check(p_thisunit, oport);
#else
	{
	UINT32 j;
	UINT32 errj = 0;
	BOOL is_err = FALSE;

	loc_cpu(flags);
	//output queue
	for (j = 0; j < VDOCAP_OUT_DEPTH_MAX && !is_err; j++) {
		if (p_ctx->outq.output_used[i][j] != 0) {
			errj = j;
			is_err = TRUE;
		}
	}
	p_ctx->outq.force_onebuffer[i] = 0;
	p_ctx->outq.force_onesize[i] = 0;
	p_ctx->outq.force_onej[i] = 0;
	unl_cpu(flags);
	if (is_err) {
		DBG_ERR("%s.out[%d]! Start Begin check fail, blk[%d] is not release?\r\n", p_thisunit->unit_name, oport, errj);
		_vdocap_oport_dumpqueue(p_thisunit);
	}
	loc_cpu(flags);
	#if 0
	//sync queue
	for (j = 0; j < VDOCAP_OUT_DEPTH_MAX; j++) {
		p_ctx->outq.sync_data[i][j] = zero;
		p_ctx->outq.sync_used[i][j] = 0;
	}
	p_ctx->outq.sync_head[i] = 0;
	p_ctx->outq.sync_tail[i] = 0;
	p_ctx->outq.sync_cnt[i] = 0;
	#endif
	//if (oport == WATCH_PORT) {DBG_DUMP("G-4:");DBGD(p_ctx->outq.sync_cnt[i]);}
	//output queue
	for (j = 0; j < VDOCAP_OUT_DEPTH_MAX; j++) {
		p_ctx->outq.output_data[i][j] = zero;
		p_ctx->outq.output_used[i][j] = 0;
	}
	unl_cpu(flags);
	}

#endif
}

static void _isf_vdocap_oport_disable(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;

	if (p_ctx->outq.output_cur_en[i] == FALSE)
		return;
	p_ctx->outq.output_cur_en[i] = FALSE;
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
	DBG_DUMP("%s.out[%d]! ----------------------------------------------- Stop Begin\r\n", p_thisunit->unit_name, oport);
	}
#endif
}

BOOL _vdocap_oport_is_enable(ISF_UNIT *p_thisunit)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	return (BOOL)(p_ctx->outq.output_en);
}

void _vdocap_oport_set_enable(ISF_UNIT *p_thisunit, UINT32 out_path, ISF_PORT *p_dest, UINT32 en)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 i = out_path - ISF_OUT_BASE;

	if (en)
		_isf_vdocap_oport_enable(p_thisunit, out_path);
	else
		_isf_vdocap_oport_disable(p_thisunit, out_path);

	if (p_dest && en) {
		ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);

		if (p_ctx->outq.force_onebuffer[i]) {
			DBG_UNIT("--- %s.out[%d]! Under single blk mode, IGNORE max_buf=%d!\r\n", p_thisunit->unit_name, i, p_vdoinfo->buffercount);
			return;
		}
		p_ctx->outq.output_max[i] = p_vdoinfo->buffercount;
		if (p_ctx->outq.output_max[i] == 0) {
			p_ctx->outq.output_max[i] = VDOCAP_OUT_DEPTH_MAX;
		}
		//DBG_DUMP("--- %s.out[%d] set max_buf=%d\r\n", p_thisunit->unit_name, i, p_ctx->outq.output_max[i]);

		DBG_MSG("-%s.out[%d]:max_buf(%d)\r\n", p_thisunit->unit_name, i, p_ctx->outq.output_max[i]);
	} else {
		//do not change!
	}
}
void _isf_vdocap_direct_unlock_cb(ISF_UNIT *p_thisunit, UINT32 oport, void *p_header_info)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	if (p_ctx) {
		//DBG_UNIT("\r\n");

		if (_vdocap_is_shdr_mode(p_ctx->shdr_map)) {
			ISF_UNIT *p_main_unit;

			//in shdr direct mode, p_thisunit of all shdr frames are vcap0 so the shdr_seq can't be distinguished.
			p_main_unit = _vdocap_shdr_main_unit[_vdocap_shdr_map_to_set(p_ctx->shdr_map)];
			if ( NULL == p_main_unit) {
				DBG_ERR("SHDR map(%d) error\r\n", p_ctx->shdr_map);
				return;
			}
			_isf_vdocap_shdr_oport_do_lock(p_main_unit, oport, p_header_info, 0, p_ctx->shdr_map);
		} else {
			_isf_vdocap_oport_do_lock(p_thisunit, oport, p_header_info, 0);
		}
	} else {
		DBG_ERR("VCAP uninit!\r\n");
	}
}
