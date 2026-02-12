#include "isf_vdoprc_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoprc_o
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoprc_o_debug_level = NVT_DBG_WRN;
//module_param_named(isf_vdoprc_o_debug_level, isf_vdoprc_o_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_vdoprc_o_debug_level, "vdoprc_o debug level");

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)	vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)	vk_spin_unlock_irqrestore(&my_lock, flags)

static VK_DEFINE_SPINLOCK(my_dis_lock);
#define dis_loc_cpu(flags)	vk_spin_lock_irqsave(&my_dis_lock, flags)
#define dis_unl_cpu(flags)	vk_spin_unlock_irqrestore(&my_dis_lock, flags)

///////////////////////////////////////////////////////////////////////////////

#define WATCH_PORT			ISF_OUT(0)
#define FORCE_DUMP_DATA		DISABLE

#define DUMP_DATA_ERROR		DISABLE

#define DUMP_PERFORMANCE		DISABLE
#define AUTO_RELEASE			ENABLE
#define DUMP_TRACK			DISABLE

#define DUMP_FPS				DISABLE

#define SYNC_TAG 				MAKEFOURCC('S','Y','N','C')
#define RANDOM_RANGE(n)     	(rand() % (n))

#define DUMP_KICK_ERR			DISABLE



#if (USE_OUT_DIS == ENABLE)
VDOPRC_DIS_PLUGIN* _vprc_dis_plugin_ = 0;

INT32 isf_vdoprc_reg_dis_plugin(VDOPRC_DIS_PLUGIN* p_dis_plugin)
{
	if ((_vprc_dis_plugin_ == 0) && (p_dis_plugin != 0)) {
		_vprc_dis_plugin_ = p_dis_plugin;
		DBG_DUMP("vprc dis plugin register ok!\r\n");
		return 0;
	}
	if ((_vprc_dis_plugin_ != 0) && (p_dis_plugin == 0)) {
		_vprc_dis_plugin_ = 0;
		DBG_DUMP("vprc dis plugin un-register done!\r\n");
		return 0;
	}

	return -1;
}
EXPORT_SYMBOL(isf_vdoprc_reg_dis_plugin);
static void _isf_vdoprc_oport_start_dis_track(ISF_UNIT *p_thisunit, UINT32 oport);
static void _isf_vdoprc_oport_stop_dis_track(ISF_UNIT *p_thisunit, UINT32 oport);
static void _isf_vdoprc_oport_clear_dis_track(ISF_UNIT *p_thisunit, UINT32 oport);
#endif

#if (USE_EIS == ENABLE)
#define MAX_LATENCY_NUM   (5+1)  //add vprc_gyro_latency
typedef struct vos_list_head EIS_LIST_HEAD;
typedef struct _VDOPRC_EIS_OUT {
	ISF_DATA* track_data;
	UINT32 track_j;
	EIS_LIST_HEAD list;
} VDOPRC_EIS_OUT;

typedef struct _VDOPRC_EIS_Q_INFO {
	EIS_LIST_HEAD free_head;
	EIS_LIST_HEAD used_head;
	VDOPRC_EIS_OUT out[MAX_LATENCY_NUM];
} VDOPRC_EIS_Q_INFO;

static VDOPRC_EIS_Q_INFO vdoprc_eis_q[MAX_EIS_PATH_NUM] = {0};
//Note that the following two API only for single entry
static ISF_RV _vdoprc_eis_get_frame(UINT32 id, UINT32 latency, ISF_DATA **p_out_data, UINT32* p_out_j)
{
	VDOPRC_EIS_OUT *p_event = NULL;
	UINT32 queue_num = 0;
	EIS_LIST_HEAD *p_list;

	if (!vos_list_empty(&vdoprc_eis_q[id].used_head)) {
		vos_list_for_each(p_list, &vdoprc_eis_q[id].used_head) {
			queue_num++;
		}
	}
	//DBG_DUMP("[%d]queue num = %d, latency(%d)\r\n", id, queue_num, latency);

	if (queue_num > latency) {
		p_event = vos_list_entry(vdoprc_eis_q[id].used_head.next, VDOPRC_EIS_OUT, list);
		vos_list_del(&p_event->list);
	}

	if (p_event) {
		vos_list_add_tail(&p_event->list, &vdoprc_eis_q[id].free_head);

		if (p_out_data) {
			p_out_data[0] = p_event->track_data;
		}
		if (p_out_j) {
			p_out_j[0] = p_event->track_j;
		}
		//DBG_DUMP("[%d]GET 0x%lX  j%d  0x%lX\r\n", id, (uintptr_t)p_event, p_event->track_j, (uintptr_t)p_event->track_data);
		return ISF_OK;
	} else {
		//DBG_WRN("vendor queue[%d] empty!\r\n", id);
		return ISF_ERR_QUEUE_EMPTY;
	}
}
static ISF_RV _vdoprc_eis_put_frame(UINT32 id, ISF_DATA *p_in_data, UINT32 in_j)
{
	VDOPRC_EIS_OUT *p_event = NULL;

	if (id >= MAX_EIS_PATH_NUM) {
		DBG_ERR("path id(%d) error\r\n", id);
		return ISF_ERR_PARAM_EXCEED_LIMIT;
	}
	if (!vos_list_empty(&vdoprc_eis_q[id].free_head)) {
		p_event = vos_list_entry(vdoprc_eis_q[id].free_head.next, VDOPRC_EIS_OUT, list);
		vos_list_del(&p_event->list);
	}

	if (NULL == p_event) {
		DBG_WRN("[%d]queue full!(j%d)\r\n", id, in_j);
		return ISF_ERR_QUEUE_FULL;
	}

	p_event->track_data = p_in_data;
	p_event->track_j = in_j;
	vos_list_add_tail(&p_event->list, &vdoprc_eis_q[id].used_head);
	//DBG_DUMP("[%d]PUT 0x%lX  j%d  0x%lX\r\n", id, (uintptr_t)p_event, p_event->track_j, (uintptr_t)p_event->track_data->h_data);
	#if 0
	{
		UINT32 queue_num = 0;
		EIS_LIST_HEAD *p_list;
		if (!vos_list_empty(&vdoprc_eis_q[id].used_head)) {
			vos_list_for_each(p_list, &vdoprc_eis_q[id].used_head) {
				p_event = vos_list_entry(p_list, VDOPRC_EIS_OUT, list);
				DBG_DUMP("%#.8lx%12llu%#.8lx\r\n", (uintptr_t)p_event, p_event->track_j, p_event->track_data);
				queue_num++;
			}
		}
		DBG_DUMP("[%d]queue num = %d\r\n", id, queue_num);
	}
	#endif
	return ISF_OK;
}
static void _vdoprc_eis_reset_queue(UINT32 id)
{
	VDOPRC_EIS_OUT *free_event;
	UINT32 i;

	memset(&vdoprc_eis_q[id], 0, sizeof(VDOPRC_EIS_Q_INFO));
	/* init free & proc list head */
	VOS_INIT_LIST_HEAD(&vdoprc_eis_q[id].free_head);
	VOS_INIT_LIST_HEAD(&vdoprc_eis_q[id].used_head);
	for (i = 0; i < MAX_LATENCY_NUM; i++) {
		free_event = &vdoprc_eis_q[id].out[i];
		//DBG_DUMP("[%d] 0x%lX %d\r\n", id, (uintptr_t)free_event, free_event->track_j);
		vos_list_add_tail(&free_event->list, &vdoprc_eis_q[id].free_head);
	}
}
UINT32 _vdoprc_oport_releasedata(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 j);
static void _vdoprc_eis_release_frame(ISF_UNIT *p_thisunit, UINT32 oport)
{
	UINT32 id = oport - ISF_OUT_BASE;
	EIS_LIST_HEAD *p_list;
	VDOPRC_EIS_OUT *p_event = NULL;

	if (!vos_list_empty(&vdoprc_eis_q[id].used_head)) {
		vos_list_for_each(p_list, &vdoprc_eis_q[id].used_head) {
			p_event = vos_list_entry(p_list, VDOPRC_EIS_OUT, list);
			if (p_event) {
				p_thisunit->p_base->do_release(p_thisunit, oport, p_event->track_data, ISF_OUT_PROBE_PUSH_DROP);
				_vdoprc_oport_releasedata(p_thisunit, oport, p_event->track_j);
				//DBG_DUMP("vprc.out[%d] release eis blk[0x%lx] j(%d)\r\n", id, (uintptr_t)p_event->track_data->h_data, p_event->track_j);
			}
		}
	}
}
#endif
static ISF_DATA zero={0};

#if _TODO
#if defined(_BSP_NA51000_) || defined(_BSP_NA51055_)
/*
===================================
680 YUV Compress data size
===================================

LineoffsetY = ceil(SizeH, 128) << 2;
HEVC mode:
Height = ceil(SizeV, 64) >> 2;
H264 mode:
Height = ceil(SizeV, 16) >> 2;
BufferY = LineoffsetY * Height

===================================
510 YUV Compress data size
===================================

LineoffsetC = ceil(SizeH, 128);
HEVC mode:
Height = ceil(SizeV>>1,32);
H264 mode:
Height = ceil(SizeV>>1, 8);
BufferC = LineoffsetC * Height * 0.75(fixed compress-ratio)
*/

static UINT32 _nvx1_calcsize(UINT32 codec, UINT32 w, UINT32 h)
{
    UINT32 line_y = ALIGN_CEIL(w, 128) << 2;
    UINT32 h_y = ALIGN_CEIL(h, 64) >> 2; //HEVC
    //UINT32 h_y = ALIGN_CEIL(h, 16) >> 2; //H264
    return 1024 + (line_y*h_y);
}
#endif
#if defined(_BSP_NA51023_)
/*
===================================
510 YUV Compress data size
===================================

LineoffsetC = ceil(SizeH, 128);
HEVC mode:
Height = ceil(SizeV>>1,32);
H264 mode:
Height = ceil(SizeV>>1, 8);
BufferC = LineoffsetC * Height * 0.75(fixed compress-ratio)

*/

static UINT32 _nvx2_calcsize(UINT32 codec, UINT32 w, UINT32 h)
{
    UINT32 line_y = ALIGN_CEIL(w, 128);
    UINT32 h_y = ALIGN_CEIL(h>>1, 32); //HEVC
    //UINT32 h_y = ALIGN_CEIL(h>>1, 8); //H264
    return 1024 + ((line_y*h_y*3)>>2);
}

static UINT32 _md_calcsize(UINT32 w, UINT32 h)
{
    return h * 48 * 4;
}
#endif
#endif

void _isf_vprc_set_ofunc_allow(ISF_UNIT *p_thisunit)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 allow_ext = FALSE;

	p_ctx->ofunc_allow[0] = VDOPRC_OFUNC_ONEBUF|VDOPRC_OFUNC_TWOBUF|VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;
	p_ctx->ofunc_allow[1] = VDOPRC_OFUNC_ONEBUF|VDOPRC_OFUNC_TWOBUF|VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;
	p_ctx->ofunc_allow[2] = VDOPRC_OFUNC_ONEBUF|VDOPRC_OFUNC_TWOBUF|VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;
	p_ctx->ofunc_allow[3] = VDOPRC_OFUNC_ONEBUF|VDOPRC_OFUNC_TWOBUF|VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y;
	p_ctx->ofunc_allow[4] = VDOPRC_OFUNC_ONEBUF|VDOPRC_OFUNC_TWOBUF|VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	switch (p_ctx->new_mode) {
	case VDOPRC_PIPE_RAWALL:
		#if defined(_BSP_NA51000_)
		p_ctx->ofunc_allow[0] |= VDOPRC_OFUNC_NVX|VDOPRC_OFUNC_RGB|VDOPRC_OFUNC_YUVP;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->ofunc_allow[0] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_NVX|VDOPRC_OFUNC_YUVP;
		p_ctx->ofunc_allow[1] |= VDOPRC_OFUNC_LOWLATENCY;
		p_ctx->ofunc_allow[2] |= VDOPRC_OFUNC_LOWLATENCY;
		p_ctx->ofunc_allow[4] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_NVX;
		p_ctx->ofunc_allow[4] &= ~(VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP);
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->ofunc_allow[0] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_NVX|VDOPRC_OFUNC_YUVP;
		p_ctx->ofunc_allow[1] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_YUVP2;
		p_ctx->ofunc_allow[2] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_YUVP2;
		p_ctx->ofunc_allow[3] = 0; //not support
		p_ctx->ofunc_allow[4] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_NVX;
		p_ctx->ofunc_allow[4] &= ~(VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP);
		#endif
		allow_ext = TRUE;
		break;
	case VDOPRC_PIPE_RAWCAP: //running 2 times of RAWALL to gen (1) sub out (2) ime out
		#if defined(_BSP_NA51000_)
		p_ctx->ofunc_allow[0] |= VDOPRC_OFUNC_NVX;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->ofunc_allow[0] |= VDOPRC_OFUNC_NVX;
		p_ctx->ofunc_allow[4] &= ~(VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP);
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->ofunc_allow[0] |= VDOPRC_OFUNC_NVX;
		p_ctx->ofunc_allow[3] = 0; //not support
		p_ctx->ofunc_allow[4] &= ~(VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP);
		#endif
		allow_ext = TRUE;
		break;

/*
	case VDOPRC_PIPE_RAWALL|VDOPRC_PIPE_WARP_360:
		p_ctx->ofunc_allow[1] = 0;
		p_ctx->ofunc_allow[2] = 0;
		p_ctx->ofunc_allow[3] = 0;
		p_ctx->ofunc_allow[4] = 0;
		break;
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case VDOPRC_PIPE_YUVALL:
		#if defined(_BSP_NA51000_)
		p_ctx->ofunc_allow[0] |= VDOPRC_OFUNC_NVX|VDOPRC_OFUNC_RGB|VDOPRC_OFUNC_YUVP;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->ofunc_allow[0] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_NVX|VDOPRC_OFUNC_YUVP;
		p_ctx->ofunc_allow[1] |= VDOPRC_OFUNC_LOWLATENCY;
		p_ctx->ofunc_allow[2] |= VDOPRC_OFUNC_LOWLATENCY;
		p_ctx->ofunc_allow[4] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_NVX;
		p_ctx->ofunc_allow[4] &= ~(VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP);
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->ofunc_allow[0] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_NVX|VDOPRC_OFUNC_YUVP;
		p_ctx->ofunc_allow[1] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_YUVP2;
		p_ctx->ofunc_allow[2] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_YUVP2;
		p_ctx->ofunc_allow[3] = 0; //not support
		p_ctx->ofunc_allow[4] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_NVX;
		p_ctx->ofunc_allow[4] &= ~(VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP);
		#endif
		allow_ext = TRUE;
		break;
	case VDOPRC_PIPE_YUVCAP: //running 2 times of RAWALL to gen (1) sub out (2) ime out
		#if defined(_BSP_NA51000_)
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->ofunc_allow[0] |= VDOPRC_OFUNC_NVX;
		p_ctx->ofunc_allow[4] &= ~(VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP);
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->ofunc_allow[0] |= VDOPRC_OFUNC_NVX;
		p_ctx->ofunc_allow[3] = 0; //not support
		p_ctx->ofunc_allow[4] &= ~(VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP);
		#endif
		allow_ext = TRUE;
		break;
	case VDOPRC_PIPE_SCALE:
		#if defined(_BSP_NA51000_)
		p_ctx->ofunc_allow[0] |= VDOPRC_OFUNC_NVX|VDOPRC_OFUNC_RGB;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->ofunc_allow[0] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_NVX;
		p_ctx->ofunc_allow[1] |= VDOPRC_OFUNC_LOWLATENCY;
		p_ctx->ofunc_allow[2] |= VDOPRC_OFUNC_LOWLATENCY;
		p_ctx->ofunc_allow[4] |= VDOPRC_OFUNC_LOWLATENCY;
		p_ctx->ofunc_allow[4] &= ~(VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP);
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->ofunc_allow[0] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_NVX;
		p_ctx->ofunc_allow[1] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_YUVP2;
		p_ctx->ofunc_allow[2] |= VDOPRC_OFUNC_LOWLATENCY|VDOPRC_OFUNC_YUVP2;
		p_ctx->ofunc_allow[3] = 0; //not support
		p_ctx->ofunc_allow[4] |= VDOPRC_OFUNC_LOWLATENCY;
		p_ctx->ofunc_allow[4] &= ~(VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP);
		#endif
		allow_ext = TRUE;
		break;
	case VDOPRC_PIPE_COLOR:
		#if defined(_BSP_NA51000_)
		p_ctx->ofunc_allow[0] = VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV|VDOPRC_OFUNC_SAMEFMT;
		p_ctx->ofunc_allow[1] = 0;
		p_ctx->ofunc_allow[2] = 0;
		p_ctx->ofunc_allow[3] = 0;
		p_ctx->ofunc_allow[4] = 0;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->ofunc_allow[0] = VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV|VDOPRC_OFUNC_SAMEFMT;
		p_ctx->ofunc_allow[1] = 0;
		p_ctx->ofunc_allow[2] = 0;
		p_ctx->ofunc_allow[3] = 0;
		p_ctx->ofunc_allow[4] = 0;
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->ofunc_allow[0] = VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV|VDOPRC_OFUNC_SAMEFMT;
		p_ctx->ofunc_allow[1] = 0;
		p_ctx->ofunc_allow[2] = 0;
		p_ctx->ofunc_allow[3] = 0;
		p_ctx->ofunc_allow[4] = 0;
		#endif
		break;
	case VDOPRC_PIPE_WARP:
		#if defined(_BSP_NA51000_)
		p_ctx->ofunc_allow[0] = VDOPRC_OFUNC_YUV|VDOPRC_OFUNC_SAMEFMT;
		p_ctx->ofunc_allow[1] = 0;
		p_ctx->ofunc_allow[2] = 0;
		p_ctx->ofunc_allow[3] = 0;
		p_ctx->ofunc_allow[4] = 0;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->ofunc_allow[0] = VDOPRC_OFUNC_YUV|VDOPRC_OFUNC_SAMEFMT;
		p_ctx->ofunc_allow[1] = 0;
		p_ctx->ofunc_allow[2] = 0;
		p_ctx->ofunc_allow[3] = 0;
		p_ctx->ofunc_allow[4] = 0;
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->ofunc_allow[0] = VDOPRC_OFUNC_YUV|VDOPRC_OFUNC_SAMEFMT;
		p_ctx->ofunc_allow[1] = 0;
		p_ctx->ofunc_allow[2] = 0;
		p_ctx->ofunc_allow[3] = 0;
		p_ctx->ofunc_allow[4] = 0;
		#endif
		break;
#if (USE_VPE == ENABLE)
	case VDOPRC_PIPE_VPE:
		p_ctx->ofunc_allow[0] = VDOPRC_OFUNC_YUV|VDOPRC_OFUNC_NVX|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP;
		p_ctx->ofunc_allow[1] = VDOPRC_OFUNC_YUV|VDOPRC_OFUNC_NVX|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP;
		p_ctx->ofunc_allow[2] = VDOPRC_OFUNC_YUV|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP;
		p_ctx->ofunc_allow[3] = VDOPRC_OFUNC_YUV|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP;
		break;
#endif
#if (USE_ISE == ENABLE)
	case VDOPRC_PIPE_ISE:
		p_ctx->ofunc_allow[0] = VDOPRC_OFUNC_YUV|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP;
		p_ctx->ofunc_allow[1] = VDOPRC_OFUNC_YUV|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP;
		p_ctx->ofunc_allow[2] = VDOPRC_OFUNC_YUV|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP;
		p_ctx->ofunc_allow[3] = VDOPRC_OFUNC_YUV|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_CROP;
		break;
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	default:
		break;
	}
	if (p_ctx->combine_mode) {
		//not support crop in combine mode
		p_ctx->ofunc_allow[0] &= ~(VDOPRC_OFUNC_CROP);
		p_ctx->ofunc_allow[1] &= ~(VDOPRC_OFUNC_CROP);
		p_ctx->ofunc_allow[2] &= ~(VDOPRC_OFUNC_CROP);
		p_ctx->ofunc_allow[3] &= ~(VDOPRC_OFUNC_CROP);
		p_ctx->ofunc_allow[4] &= ~(VDOPRC_OFUNC_CROP);
	}

	if (allow_ext) {
		p_ctx->ofunc_allow[5] = VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_ROTATE|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;
		p_ctx->ofunc_allow[6] = VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_ROTATE|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;
		p_ctx->ofunc_allow[7] = VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_ROTATE|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;
#if (VDOPRC_MAX_OUT_NUM > 8)
		p_ctx->ofunc_allow[8] = VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_ROTATE|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;
		p_ctx->ofunc_allow[9] = VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_ROTATE|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;
		p_ctx->ofunc_allow[10] = VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_ROTATE|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;
		p_ctx->ofunc_allow[11] = VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_ROTATE|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;
		p_ctx->ofunc_allow[12] = VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_ROTATE|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;
		p_ctx->ofunc_allow[13] = VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_ROTATE|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;
		p_ctx->ofunc_allow[14] = VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_ROTATE|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;
		p_ctx->ofunc_allow[15] = VDOPRC_OFUNC_CROP|VDOPRC_OFUNC_ROTATE|VDOPRC_OFUNC_SCALEUP|VDOPRC_OFUNC_SCALEDN|VDOPRC_OFUNC_Y|VDOPRC_OFUNC_YUV;
#endif
	} else {
		p_ctx->ofunc_allow[5] = 0;
		p_ctx->ofunc_allow[6] = 0;
		p_ctx->ofunc_allow[7] = 0;
#if (VDOPRC_MAX_OUT_NUM > 8)
		p_ctx->ofunc_allow[8] = 0;
		p_ctx->ofunc_allow[9] = 0;
		p_ctx->ofunc_allow[10] = 0;
		p_ctx->ofunc_allow[11] = 0;
		p_ctx->ofunc_allow[12] = 0;
		p_ctx->ofunc_allow[13] = 0;
		p_ctx->ofunc_allow[14] = 0;
		p_ctx->ofunc_allow[15] = 0;
#endif
	}
}
void _isf_vdoprc_push_dummy_queue_for_user_buf(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data)
{
#if (USE_VPE == ENABLE)
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i =  oport - ISF_OUT_BASE;

	if (p_ctx->vpe_mode && p_ctx->user_out_blk[i]) {
		VDO_FRAME *p_vdoframe = (VDO_FRAME *)(p_data->desc);

		DBG_DUMP("%s drop out[%d]\r\n", p_thisunit->unit_name, (int)i);
		p_data->h_data = p_ctx->user_out_blk[i];
		p_vdoframe->reserved[2] =  MAKEFOURCC('D', 'R', 'O', 'P');
		_isf_vdoprc_oqueue_do_push_wait(p_thisunit, oport, p_data, USER_OUT_BUFFER_QUEUE_TIMEOUT);
	}
#endif
}
ISF_RV _vdoprc_config_ofunc(ISF_UNIT *p_thisunit, BOOL en)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

	ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_3DNR_REFPATH_SEL, (void *)&p_ctx->ctrl._3dnr_refpath);
	p_ctx->ctrl.cur_3dnr_refpath = p_ctx->ctrl._3dnr_refpath; //apply
	p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, 	"3dnr(refpath=%d)",
		p_ctx->ctrl._3dnr_refpath);

#if (USE_IN_DIRECT == ENABLE)
	if (p_ctx->new_in_cfg_func & VDOPRC_IFUNC_DIRECT) {
		UINT32 i;
		for (i = 0; i < ISF_VDOPRC_PHY_OUT_NUM; i++) {
#if (USE_OUT_DIS == ENABLE)
			if (p_ctx->outq.dis_mode && (p_ctx->new_out_cfg_func[i] & VDOPRC_OFUNC_DIS)) {
				DBG_ERR("-%s.out[%d]:input DIRECT is not support DIS!!!\r\n", p_thisunit->unit_name, i);
			}
#endif
		}
	}
#endif
#if (USE_OUT_LOWLATENCY == ENABLE)
	if (en != DISABLE) {
		UINT32 i;
		UINT32 low_delay_path = (UINT32)-1;
		UINT32 low_delay_bp = 0;
		UINT32 user_cfg = _isf_vdoprc_get_cfg();
		UINT32 stripe_rule = user_cfg & HD_VIDEOPROC_CFG_STRIP_MASK; //get stripe rule
		UINT32 allow_ll = 1;
		if (stripe_rule > 3) {
			stripe_rule = 0;
		}
		if (stripe_rule == 3) { //LV4 is not allow LL
			allow_ll = 0;
		}
		for (i = 0; i < ISF_VDOPRC_PHY_OUT_NUM; i++) {
			if (p_ctx->new_out_cfg_func[i] & VDOPRC_OFUNC_LOWLATENCY) {
				if (low_delay_path == (UINT32)-1) {
					if (allow_ll) {
						low_delay_path = i;
					} else {
						DBG_DUMP("WRN:vprc: not allow low-lantency func at path %d!\r\n", (int)i);
						p_ctx->new_out_cfg_func[i] &= ~VDOPRC_OFUNC_LOWLATENCY;
					}
				} else {
					DBG_ERR("duplicate low-lantency path %d, cur=%d\r\n", (int)i, (int)low_delay_path);
				}
			}
		}
		if (low_delay_path != (UINT32)-1) {
			DBG_DUMP("vprc: enable low-lantency func at path %d\r\n", (int)low_delay_path);
		} else {
			//DBG_DUMP("vprc: disable low-lantency func\r\n");
		}
#if (USE_OUT_DIS == ENABLE)
		if (low_delay_path != (UINT32)-1) {
			if (p_ctx->outq.dis_mode && (p_ctx->new_out_cfg_func[low_delay_path] & VDOPRC_OFUNC_DIS)) {
				DBG_ERR("-%s.out%d:LOWLATENCY is not support DIS!!!\r\n", p_thisunit->unit_name, low_delay_path);
			}
		}
#endif
		ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_LOW_DELAY_PATH_SEL, (void *)&low_delay_path);

		low_delay_bp = p_ctx->ctrl._lowlatency_trig;
		ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_LOW_DELAY_BP, (void *)&low_delay_bp);
	}
#endif
#if (USE_OUT_ONEBUF == ENABLE)
	if (en != DISABLE) {
		UINT32 i;
		for (i = 0; i < ISF_VDOPRC_PHY_OUT_NUM; i++) {
			UINT32 is_3dnr_ref = ((p_ctx->ctrl.cur_func & VDOPRC_FUNC_3DNR) && (p_ctx->ctrl._3dnr_refpath == i));
            if((p_ctx->new_out_cfg_func[i] & VDOPRC_OFUNC_ONEBUF)&&(p_ctx->new_out_cfg_func[i] & VDOPRC_OFUNC_TWOBUF)) {
				DBG_ERR("-%s.out%d:TWOBUF and ONEBUF,default one!!!\r\n", p_thisunit->unit_name, i);
                p_ctx->new_out_cfg_func[i] &= ~VDOPRC_OFUNC_TWOBUF;
            }

			if (p_ctx->new_out_cfg_func[i] & VDOPRC_OFUNC_ONEBUF) {
				DBG_DUMP("vprc: enable one-buf func at path %d, (3ndr_ref=%d) (max_strp=%d)\r\n", (int)i, (int)is_3dnr_ref, (int)p_ctx->max_strp_num);
				p_ctx->cur_out_cfg_func[i] |= VDOPRC_OFUNC_ONEBUF;
			} else {
				p_ctx->cur_out_cfg_func[i] &= ~VDOPRC_OFUNC_ONEBUF;
			}

			if (p_ctx->new_out_cfg_func[i] & VDOPRC_OFUNC_TWOBUF) {
				DBG_DUMP("vprc: enable two-buf func at path %d, (3ndr_ref=%d) (max_strp=%d)\r\n", (int)i, (int)is_3dnr_ref, (int)p_ctx->max_strp_num);
				p_ctx->cur_out_cfg_func[i] |= VDOPRC_OFUNC_TWOBUF;
			} else {
				p_ctx->cur_out_cfg_func[i] &= ~VDOPRC_OFUNC_TWOBUF;
			}
#if (USE_OUT_DIS == ENABLE)
			if (p_ctx->new_out_cfg_func[i] & VDOPRC_OFUNC_ONEBUF) {
				if (p_ctx->outq.dis_mode && (p_ctx->new_out_cfg_func[i] & VDOPRC_OFUNC_DIS)) {
					DBG_ERR("-%s.out%d:ONEBUF is not support DIS!!!\r\n", p_thisunit->unit_name, i);
				}
			}
			if (p_ctx->new_out_cfg_func[i] & VDOPRC_OFUNC_TWOBUF) {
				if (p_ctx->outq.dis_mode && (p_ctx->new_out_cfg_func[i] & VDOPRC_OFUNC_DIS)) {
					DBG_ERR("-%s.out%d:TWOBUF is not support DIS!!!\r\n", p_thisunit->unit_name, i);
				}
			}
#endif
		}
	}
#endif
	if (en != DISABLE) {
		UINT32 i;
		for (i = 0; i < ISF_VDOPRC_PHY_OUT_NUM; i++) {
			CTL_IPP_OUT_PATH_MD md_func;

			if (p_ctx->new_out_cfg_func[i] & VDOPRC_OFUNC_MD) {
				p_ctx->cur_out_cfg_func[i] |= VDOPRC_OFUNC_MD;
				md_func.enable = 1;
                if(!(p_ctx->ctrl.cur_func & VDOPRC_FUNC_3DNR)) {
					DBG_ERR("-%s.out%d:MD need 3DNR on!\r\n", p_thisunit->unit_name, i);
                }
			} else {
				p_ctx->cur_out_cfg_func[i] &= ~VDOPRC_OFUNC_MD;
				md_func.enable = 0;
			}
			md_func.pid = i;
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH_MD, (void *)&md_func);
		}
	}
	return ISF_OK;
}

ISF_RV _vdoprc_check_out_fmt(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 imgfmt)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 vfmt = imgfmt;
	switch (vfmt) {
	case VDO_PXLFMT_Y8:
		if (!(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_Y)) {
			vfmt = 0;
		}
		break;
	case VDO_PXLFMT_YUV420:
	case VDO_PXLFMT_YUV422:
		if (!(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_YUV)) {
			vfmt = 0;
		}
		break;
	case VDO_PXLFMT_YUV444_PLANAR:
	case VDO_PXLFMT_YUV422_PLANAR:
	case VDO_PXLFMT_YUV420_PLANAR:
		if (!(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_YUVP)) {
			vfmt = 0;
		}
		break;
	case VDO_PXLFMT_YUV422_UYVY:  // == VDO_PXLFMT_YUV422_ONE
		if (!(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_YUVP) && !(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_YUVP2)) {
			vfmt = 0;
		}
		break;
	case VDO_PXLFMT_YUV422_VYUY:
	case VDO_PXLFMT_YUV422_YUYV:
	case VDO_PXLFMT_YUV422_YVYU:
		if (!(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_YUVP2)) {
			vfmt = 0;
		}
		break;
#if defined(_BSP_NA51000_)
	case VDO_PXLFMT_YUV420_NVX1_H264:
	case VDO_PXLFMT_YUV420_NVX1_H265:
#endif
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	case VDO_PXLFMT_YUV420_NVX2:
#endif
		if (!(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_NVX)) {
			vfmt = 0;
		}
		break;
	case VDO_PXLFMT_RGB888_PLANAR:
		if (!(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_RGB)) {
			vfmt = 0;
		}
		break;
	default:
		vfmt = 0;
		break;
	}
	if (!vfmt) {
		DBG_ERR("-out%d:fmt=%08x is not supported with pipe=%d!\r\n", pid, imgfmt, p_ctx->cur_mode);
		return ISF_ERR_FAILED;
	}
#if (USE_OUT_DIS == ENABLE)
	if (p_ctx->outq.dis_mode && (vfmt == VDO_PXLFMT_YUV420_NVX2)) {
		DBG_ERR("-%s.out[%d]: NVX2 is not support DIS!!!\r\n", p_thisunit->unit_name, pid);
	}
#endif
	return ISF_OK;
}


ISF_RV _vdoprc_check_out_dir(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 imgdir)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

	if (imgdir != 0) {
		if ((imgdir & VDO_DIR_MIRRORX) && !(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_MIRRORX)) {
			DBG_ERR("%s.out[%d]! cannot enable mirror-x with pipe=%08x\r\n",
				p_thisunit->unit_name, pid, p_ctx->ctrl.pipe);
			return ISF_ERR_FAILED;
		}
		if ((imgdir & VDO_DIR_MIRRORY) && !(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_MIRRORY)) {
			DBG_ERR("%s.out[%d]! cannot enable mirror-y with pipe=%08x\r\n",
				p_thisunit->unit_name, pid, p_ctx->ctrl.pipe);
			return ISF_ERR_FAILED;
		}
		/*
		if ((imgdir & (VDO_DIR_ROTATE_90|VDO_DIR_ROTATE_270)) && !(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_ROTATE)) {
			DBG_ERR("%s.out[%d]! cannot enable rotate with pipe=%08x\r\n",
				p_thisunit->unit_name, pid, p_ctx->ctrl.pipe);
			return ISF_ERR_FAILED;
		}
		*/
	}
	return ISF_OK;
}

ISF_RV _vdoprc_check_out_crop(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 imgcrop)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_INFO *p_dest_info = p_thisunit->port_outinfo[pid - ISF_OUT_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);

	if (imgcrop) {
		if (!(p_ctx->ofunc_allow[pid] & VDOPRC_OFUNC_CROP)) {
			DBG_ERR("%s.out[%d]! cannot enable crop with pipe=%08x\r\n",
				p_thisunit->unit_name, pid, p_ctx->ctrl.pipe);
			return ISF_ERR_FAILED;
		}

		if (p_vdoinfo->window.w == 0 || p_vdoinfo->window.h == 0) {
			DBG_ERR("-out%d:crop win(%d,%d) is zero?\r\n",
				pid, p_vdoinfo->window.w, p_vdoinfo->window.h);
			return ISF_ERR_FAILED;
		}

		if ( ((p_vdoinfo->window.x + p_vdoinfo->window.w) > p_ctx->out[pid].size.w)
		|| ((p_vdoinfo->window.y + p_vdoinfo->window.h) > p_ctx->out[pid].size.h) ) {
			DBG_ERR("-out%d:crop(%d,%d,%d,%d) is out of dim(%d,%d)?\r\n",
				pid,
				p_vdoinfo->window.x, p_vdoinfo->window.y, p_vdoinfo->window.w, p_vdoinfo->window.h,
				p_ctx->out[pid].size.w, p_ctx->out[pid].size.h);
			return ISF_ERR_FAILED;
		}

#if (USE_OUT_DIS == ENABLE)
		if (p_ctx->outq.dis_mode && (p_ctx->new_out_cfg_func[pid] & VDOPRC_OFUNC_DIS)) {
			DBG_ERR("-out%d:crop is conflict to DIS!\r\n",
				pid);
			return ISF_ERR_FAILED;
		}
#endif
	}
	return ISF_OK;
}

ISF_RV _vdoprc_config_out(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 en)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_INFO *p_dest_info = p_thisunit->port_outinfo[pid - ISF_OUT_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
	ISF_VDO_INFO *p_dest_vdoinfo = (ISF_VDO_INFO *)(p_thisunit->p_base->get_bind_info(p_thisunit, pid));
	ISF_RV rv;
	UINT32  dest_loff_align = p_ctx->out_loff_align[pid];

#if _TODO
	BOOL bTrigger = (p_ctx->ctrl.cur_func2 & VDOPRC_FUNC2_TRIGGERSINGLE);
#endif

	pid = pid - ISF_OUT_BASE;
	if ( pid < ISF_VDOPRC_PHY_OUT_NUM ) {
		BOOL user_out_mode;

		if (p_ctx->out_win[pid].imgaspect.w && p_ctx->out_win[pid].imgaspect.h) {
			user_out_mode = TRUE;
		} else {
			user_out_mode = FALSE;
		}
		if (!en) {
			memset((void*)&(p_ctx->out[pid]), 0, sizeof(CTL_IPP_OUT_PATH));
			p_ctx->out[pid].pid = pid;
			p_ctx->out[pid].enable = FALSE;
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "off");
#if (USE_OUT_FRC == ENABLE)
			_isf_frc_stop(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]));
#endif
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH, (void *)&p_ctx->out[pid]);
			if (p_ctx->combine_mode || user_out_mode) {
				CTL_IPP_OUT_PATH_REGION region = {0};

				region.pid = pid;
				region.enable = FALSE;
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH_REGION, (void *)&region);
			}
		} else {
			UINT32 w,h,imgfmt;

#if (USE_OUT_ONEBUF == ENABLE)
			p_ctx->bufmode[pid].pid = pid;
            if (p_ctx->new_out_cfg_func[pid] & VDOPRC_OFUNC_ONEBUF) {
			    p_ctx->bufmode[pid].one_buf_mode_en = 1;
            } else if (p_ctx->new_out_cfg_func[pid] & VDOPRC_OFUNC_TWOBUF) {
			    p_ctx->bufmode[pid].one_buf_mode_en = 2;
            }
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH_BUFMODE, (void *)&p_ctx->bufmode[pid]);
#endif

			p_ctx->out[pid].pid = pid;
			p_ctx->out[pid].enable = TRUE;
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
			if (p_ctx->combine_mode || user_out_mode) {
				w = p_ctx->out_win[pid].window.w;
				h = p_ctx->out_win[pid].window.h;
			}
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "size(%d,%d) fmt=%08x",
				w, h, imgfmt);
			if ((w == 0 || h == 0) && (p_ctx->out_win[pid].window.w == 0 || p_ctx->out_win[pid].window.h == 0) ) {
				DBG_ERR("-out%d:size(%d,%d) is zero?\r\n",
					pid, w, h);
				return ISF_ERR_FAILED;
			}
			if (imgfmt == 0) {
				DBG_ERR("-out%d:fmt is zero?\r\n", pid);
				return ISF_ERR_FAILED;
			}
#if (USE_OUT_DIS == ENABLE)
			if (p_ctx->outq.dis_mode && (p_ctx->new_out_cfg_func[pid] & VDOPRC_OFUNC_DIS)) {
				w = w * p_ctx->outq.dis_scaleratio/1000; w = ALIGN_CEIL(w, 2);
				h = h * p_ctx->outq.dis_scaleratio/1000; h = ALIGN_CEIL(h, 2);
			}
#endif
			p_ctx->out[pid].fmt = imgfmt;
			p_ctx->out[pid].size.w = w;
			p_ctx->out[pid].size.h = h;
			p_ctx->out[pid].lofs = dest_loff_align ? ALIGN_CEIL(w, dest_loff_align) : ALIGN_CEIL_16(w);

            DBG_IND("%d p_ctx->out[%d].lofs %d dest_loff_align %d\r\n",(int)p_ctx->dev,(int)pid,(int)p_ctx->out[pid].lofs,(int)dest_loff_align);

            if (p_vdoinfo->window.x == 0 && p_vdoinfo->window.y == 0 && p_vdoinfo->window.w == 0 && p_vdoinfo->window.h == 0) {
				//use full image size
				p_ctx->out_crop_mode[pid] = 0;
				p_ctx->out[pid].crp_window.x = 0;
				p_ctx->out[pid].crp_window.y = 0;
				p_ctx->out[pid].crp_window.w = w;
				p_ctx->out[pid].crp_window.h = h;
			} else {
				p_ctx->out_crop_mode[pid] = 1;
				p_ctx->out[pid].crp_window = p_vdoinfo->window;
				p_ctx->out[pid].lofs = dest_loff_align ? ALIGN_CEIL(p_vdoinfo->window.w, dest_loff_align) : ALIGN_CEIL_16(p_vdoinfo->window.w);
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "-out%d:crop(%d,%d,%d,%d)",
					pid, p_vdoinfo->window.x, p_vdoinfo->window.y, p_vdoinfo->window.w, p_vdoinfo->window.h);
			}
			rv = _vdoprc_check_out_fmt(p_thisunit, pid, imgfmt);
			if (rv != ISF_OK) {
				return rv;
			}
			rv = _vdoprc_check_out_crop(p_thisunit, pid, p_ctx->out_crop_mode[pid]);
			if (rv != ISF_OK) {
				return rv;
			}
			rv = _vdoprc_check_out_dir(p_thisunit, pid, p_vdoinfo->direct);
			if (rv != ISF_OK) {
				return rv;
			}
#if (USE_OUT_FRC == ENABLE)
			if (p_vdoinfo->framepersecond == VDO_FRC_DIRECT) p_vdoinfo->framepersecond = VDO_FRC_ALL;
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM_IMM, "frc(%d,%d)",
				pid, GET_HI_UINT16(p_vdoinfo->framepersecond), GET_LO_UINT16(p_vdoinfo->framepersecond));
			if ((GET_HI_UINT16(p_vdoinfo->framepersecond) != GET_LO_UINT16(p_vdoinfo->framepersecond)) && (p_ctx->ctrl.cur_func & VDOPRC_FUNC_3DNR) && (pid == p_ctx->ctrl._3dnr_refpath)) {
				DBG_ERR("-out%d:frc(%d,%d) is conflict to func 3DNR, ignore this frc!!!\r\n", pid, GET_HI_UINT16(p_vdoinfo->framepersecond), GET_LO_UINT16(p_vdoinfo->framepersecond));
				p_vdoinfo->framepersecond = VDO_FRC_ALL;
			}
			_isf_frc_start(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]), p_vdoinfo->framepersecond);
#endif
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH, (void *)&p_ctx->out[pid]);
			if (p_ctx->combine_mode || user_out_mode) {
				CTL_IPP_OUT_PATH_REGION region = {0};

				region.pid = pid;
				region.enable = TRUE;
				region.bgn_lofs = p_ctx->out_win[pid].imgaspect.w;
				region.bgn_size.w = p_ctx->out_win[pid].imgaspect.w;
				region.bgn_size.h = p_ctx->out_win[pid].imgaspect.h;
				region.region_ofs.x = p_ctx->out_win[pid].window.x;
				region.region_ofs.y = p_ctx->out_win[pid].window.y;
				#if 0
				DBG_DUMP("######## %s OUT[%d] REGION INFO  ########\r\n", p_thisunit->unit_name, pid);
				DBG_DUMP(".enable = %d\r\n", region.enable);
				DBG_DUMP(".bgn_lofs = %d\r\n", region.bgn_lofs);
				DBG_DUMP(".bgn_size.w h = %dx%d\r\n", region.bgn_size.w, region.bgn_size.h);
				DBG_DUMP(".region_ofs.x y = %dx%d\r\n", region.region_ofs.x, region.region_ofs.y);
				DBG_DUMP("#######################################\r\n");
				#endif
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH_REGION, (void *)&region);
				//clean blk info
				if (p_ctx->combine_mode) {
					p_ctx->user_out_blk[pid] = 0;
				}
			}
			// set h_algin
			{
				CTL_IPP_OUT_PATH_HALIGN h_aln;

				h_aln.pid = pid;
				h_aln.align = p_ctx->out_h_align[pid];
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH_HALIGN, (void *)&h_aln);
				//DBG_ERR("set ime path %d, h_aln = %d\r\n", pid, p_ctx->out_h_align[pid]);
			}
			// set out_order
			{
			    CTL_IPP_OUT_PATH_ORDER cfg;

			    cfg.pid = pid;
			    cfg.order = p_ctx->out_order[pid];
			    ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH_ORDER_IMM, (void *)&cfg);
			}
			p_vdoinfo->dirty &= ~(ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW);    //clear dirty
		}
#if (USE_OUT_EXT == ENABLE)
	} else if (pid < ISF_VDOPRC_OUT_NUM) {

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		UINT32 imgfmt = p_vdoinfo->imgfmt;
		p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "fmt=%08x",	imgfmt);
		switch (imgfmt) {
		case VDO_PXLFMT_520_LCA:
		case VDO_PXLFMT_520_VA:
		case VDO_PXLFMT_520_WDR:
		case VDO_PXLFMT_520_DEFOG:
		case VDO_PXLFMT_520_3DNR_MS:
		case VDO_PXLFMT_520_3DNR_MV:
		case VDO_PXLFMT_520_3DNR_MD:
		case VDO_PXLFMT_520_3DNR_STA:
		case VDO_PXLFMT_520_IR8:
		case VDO_PXLFMT_520_IR16:
			p_ctx->phy_mask |= (1<<pid); //use as special pxlfmt path
			break;
		default:
			p_ctx->phy_mask &= ~(1<<pid); //use as extend path
			break;
		}

		//XDATA
		if (p_ctx->phy_mask & (1<<pid)) { //special pxlfmt path

			if (!en) {
				memset((void*)&(p_ctx->out[pid]), 0, sizeof(CTL_IPP_OUT_PATH));
				p_ctx->out[pid].pid = pid;
				p_ctx->out[pid].enable = FALSE;
				p_ctx->out[pid].fmt = 0;
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "off");
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH, (void *)&p_ctx->out[pid]);
			} else {
				switch (p_ctx->cur_mode) {
				case VDOPRC_PIPE_COLOR:
				case VDOPRC_PIPE_WARP:
					if(pid != 0) {
						DBG_ERR("-out%d: not support with pipe=%d!\r\n", pid, p_ctx->cur_mode);
						imgfmt = 0;
					}
					break;
				default:
					break;
				}

				p_ctx->out[pid].pid = pid;
				p_ctx->out[pid].enable = TRUE;
				p_ctx->out[pid].fmt = imgfmt;
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH, (void *)&p_ctx->out[pid]);
				p_vdoinfo->dirty &= ~(ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW);    //clear dirty
			}

		} else
#endif
		{  //extend path

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
			if (1) {
				memset((void*)&(p_ctx->out[pid]), 0, sizeof(CTL_IPP_OUT_PATH));
				p_ctx->out[pid].pid = pid;
				p_ctx->out[pid].enable = FALSE;
				p_ctx->out[pid].fmt = 0;
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "off");
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH, (void *)&p_ctx->out[pid]);
			}
#endif
			if (1) {
				_vdoprc_config_out_ext(p_thisunit, pid, en);
			}
		}
#endif
	} else {
		DBG_ERR("invalid out\r\n");
		return ISF_ERR_INVALID_PORT_ID;
	}
	return ISF_OK;
}

//in _vdoprc_config_ofunc would refer new set and check rule
//in _vdoprc_update_out would refer new set
ISF_RV _vdoprc_update_out(ISF_UNIT *p_thisunit, UINT32 pid, UINT32 en)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_INFO *p_dest_info = p_thisunit->port_outinfo[pid - ISF_OUT_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
	ISF_VDO_INFO *p_dest_vdoinfo = (ISF_VDO_INFO *)(p_thisunit->p_base->get_bind_info(p_thisunit, pid));
	ISF_RV rv;
	UINT32  dest_loff_align = p_ctx->out_loff_align[pid];

#if _TODO
	BOOL bTrigger = (p_ctx->ctrl.cur_func2 & VDOPRC_FUNC2_TRIGGERSINGLE);
#endif

	pid = pid - ISF_OUT_BASE;
	p_thisunit->p_base->do_trace(p_thisunit, pid, ISF_OP_PARAM_VDOFRM, "update_out en=%d cur=0x%x",en,p_ctx->cur_out_cfg_func[pid - ISF_OUT_BASE]);

	if ( pid < ISF_VDOPRC_PHY_OUT_NUM ) {
		BOOL user_out_mode;

		if (p_ctx->out_win[pid].imgaspect.w && p_ctx->out_win[pid].imgaspect.h) {
			user_out_mode = TRUE;
		} else {
			user_out_mode = FALSE;
		}
		if (!en) {
			memset((void*)&(p_ctx->out[pid]), 0, sizeof(CTL_IPP_OUT_PATH));
			p_ctx->out[pid].pid = pid;
			p_ctx->out[pid].enable = FALSE;
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "off");
#if (USE_OUT_FRC == ENABLE)
			_isf_frc_stop(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]));
#endif
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH, (void *)&p_ctx->out[pid]);
			if (p_ctx->combine_mode || user_out_mode) {
				CTL_IPP_OUT_PATH_REGION region = {0};

				region.pid = pid;
				region.enable = FALSE;
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH_REGION, (void *)&region);
			}
		} else {
			UINT32 w,h,imgfmt;
#if (USE_OUT_ONEBUF == ENABLE)
			p_ctx->bufmode[pid].pid = pid;
            if (p_ctx->new_out_cfg_func[pid] & VDOPRC_OFUNC_ONEBUF) {
			    p_ctx->bufmode[pid].one_buf_mode_en = 1;
            } else if (p_ctx->new_out_cfg_func[pid] & VDOPRC_OFUNC_TWOBUF) {
			    p_ctx->bufmode[pid].one_buf_mode_en = 2;
            }
            ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH_BUFMODE, (void *)&p_ctx->bufmode[pid]);
#endif

			p_ctx->out[pid].pid = pid;
			p_ctx->out[pid].enable = TRUE;
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
			if (p_ctx->combine_mode || user_out_mode) {
				w = p_ctx->out_win[pid].window.w;
				h = p_ctx->out_win[pid].window.h;
			}
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "size(%d,%d) fmt=%08x",
				w, h, imgfmt);
			if ((w == 0 || h == 0) && (p_ctx->out_win[pid].window.w == 0 || p_ctx->out_win[pid].window.h == 0) ) {
				DBG_ERR("-out%d:size(%d,%d) is zero?\r\n",
					pid, w, h);
				return ISF_ERR_FAILED;
			}
			if (imgfmt == 0) {
				DBG_ERR("-out%d:fmt is zero?\r\n", pid);
				return ISF_ERR_FAILED;
			}
#if (USE_OUT_DIS == ENABLE)
            if ((p_ctx->new_out_cfg_func[pid] & VDOPRC_OFUNC_LOWLATENCY)&&
               (p_ctx->new_out_cfg_func[pid] & VDOPRC_OFUNC_DIS)) {
                DBG_ERR("-%s.out%d:LOWLATENCY is not support DIS!!!\r\n", p_thisunit->unit_name, pid);
            }

			if (p_ctx->outq.dis_mode && (p_ctx->new_out_cfg_func[pid] & VDOPRC_OFUNC_DIS)) {
				w = w * p_ctx->outq.dis_scaleratio/1000; w = ALIGN_CEIL(w, 2);
				h = h * p_ctx->outq.dis_scaleratio/1000; h = ALIGN_CEIL(h, 2);
			}
#endif
			p_ctx->out[pid].fmt = imgfmt;
			p_ctx->out[pid].size.w = w;
			p_ctx->out[pid].size.h = h;
			p_ctx->out[pid].lofs = dest_loff_align ? ALIGN_CEIL(w, dest_loff_align) : ALIGN_CEIL_16(w);

            DBG_IND("%d p_ctx->out[%d].lofs %d dest_loff_align %d\r\n",(int)p_ctx->dev,(int)pid,(int)p_ctx->out[pid].lofs,(int)dest_loff_align);
			#if _TODO
			if (p_ctx->codec[out_path-ISF_OUT_BASE] == MAKEFOURCC('H', '2', '6', '5')) {
				p_ctx->out[pid].lofs = ALIGN_CEIL_64(w);
			}
			#endif
			if (p_vdoinfo->window.x == 0 && p_vdoinfo->window.y == 0 && p_vdoinfo->window.w == 0 && p_vdoinfo->window.h == 0) {
				//use full image size
				p_ctx->out_crop_mode[pid] = 0;
				p_ctx->out[pid].crp_window.x = 0;
				p_ctx->out[pid].crp_window.y = 0;
				p_ctx->out[pid].crp_window.w = w;
				p_ctx->out[pid].crp_window.h = h;
			} else {
				p_ctx->out_crop_mode[pid] = 1;
				p_ctx->out[pid].crp_window = p_vdoinfo->window;
				p_ctx->out[pid].lofs = dest_loff_align ? ALIGN_CEIL(p_vdoinfo->window.w, dest_loff_align) :ALIGN_CEIL_16(p_vdoinfo->window.w);
				#if _TODO
				if (p_ctx->codec[out_path-ISF_OUT_BASE] == MAKEFOURCC('H', '2', '6', '5')) {
					p_ctx->out[pid].lofs = ALIGN_CEIL_64(p_vdoinfo->window.w);
				}
				#endif
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "-out%d:crop(%d,%d,%d,%d)",
					pid, p_vdoinfo->window.x, p_vdoinfo->window.y, p_vdoinfo->window.w, p_vdoinfo->window.h);
			}
			rv = _vdoprc_check_out_fmt(p_thisunit, pid, imgfmt);
			if (rv != ISF_OK) {
				return rv;
			}
			rv = _vdoprc_check_out_crop(p_thisunit, pid, p_ctx->out_crop_mode[pid]);
			if (rv != ISF_OK) {
				return rv;
			}
			rv = _vdoprc_check_out_dir(p_thisunit, pid, p_vdoinfo->direct);
			if (rv != ISF_OK) {
				return rv;
			}
#if (USE_OUT_FRC == ENABLE)
			if (p_vdoinfo->framepersecond == VDO_FRC_DIRECT) p_vdoinfo->framepersecond = VDO_FRC_ALL;
			p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM_IMM, "frc(%d,%d),%x",
				pid, GET_HI_UINT16(p_vdoinfo->framepersecond), GET_LO_UINT16(p_vdoinfo->framepersecond),p_vdoinfo->framepersecond);
			if ((GET_HI_UINT16(p_vdoinfo->framepersecond) != GET_LO_UINT16(p_vdoinfo->framepersecond)) && (p_ctx->ctrl.cur_func & VDOPRC_FUNC_3DNR) && (pid == p_ctx->ctrl._3dnr_refpath)) {
				DBG_ERR("-out%d:frc(%d,%d) is conflict to func 3DNR, ignore this frc!!!\r\n", pid, GET_HI_UINT16(p_vdoinfo->framepersecond), GET_LO_UINT16(p_vdoinfo->framepersecond));
				p_vdoinfo->framepersecond = VDO_FRC_ALL;
			}
			if (p_ctx->outfrc[pid].sample_mode == ISF_SAMPLE_OFF) {
				_isf_frc_start(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]), p_vdoinfo->framepersecond);
			} else {
				_isf_frc_update_imm(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]), p_vdoinfo->framepersecond);
			}
#endif
#if _TODO
#if defined(_BSP_NA51023_)
			p_modedata->IME_PATH_1.md_evt_out_en = (p_ctx->ctrl.cur_func2 & VDOPRC_FUNC2_MOTIONDETECT1) ? TRUE : FALSE;
#endif
#endif
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH, (void *)&p_ctx->out[pid]);
			if (p_ctx->combine_mode || user_out_mode) {
				CTL_IPP_OUT_PATH_REGION region = {0};

				region.pid = pid;
				region.enable = TRUE;
				region.bgn_lofs = p_ctx->out_win[pid].imgaspect.w;
				region.bgn_size.w = p_ctx->out_win[pid].imgaspect.w;
				region.bgn_size.h = p_ctx->out_win[pid].imgaspect.h;
				region.region_ofs.x = p_ctx->out_win[pid].window.x;
				region.region_ofs.y = p_ctx->out_win[pid].window.y;
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH_REGION, (void *)&region);
				//clean blk info
				if (p_ctx->combine_mode) {
					p_ctx->user_out_blk[pid] = 0;
				}
			}
			// set h_algin
			{
				CTL_IPP_OUT_PATH_HALIGN h_aln;

				h_aln.pid = pid;
				h_aln.align = p_ctx->out_h_align[pid];
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH_HALIGN, (void *)&h_aln);
				//DBG_ERR("set ime path %d, h_aln = %d\r\n", pid, p_ctx->out_h_align[pid]);
			}

			if (p_ctx->new_out_cfg_func[pid] & VDOPRC_OFUNC_MD) {
				CTL_IPP_OUT_PATH_MD md_func;

				md_func.pid = pid;
				md_func.enable = 1;
                if(!(p_ctx->ctrl.cur_func & VDOPRC_FUNC_3DNR)) {
					DBG_ERR("-%s.out%d:MD need 3DNR on!\r\n", p_thisunit->unit_name, pid);
                }
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH_MD, (void *)&md_func);

			} else {
				CTL_IPP_OUT_PATH_MD md_func;

	  			md_func.pid = pid;
				md_func.enable = 0;
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH_MD, (void *)&md_func);
			}
			// set out_order
			{
				CTL_IPP_OUT_PATH_ORDER cfg;

				cfg.pid = pid;
				cfg.order = p_ctx->out_order[pid];
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH_ORDER_IMM, (void *)&cfg);
			}
#if (USE_OUT_DIS == ENABLE)
			if (p_ctx->outq.dis_mode) {
				if (!(p_ctx->cur_out_cfg_func[pid] & VDOPRC_OFUNC_DIS) && (p_ctx->new_out_cfg_func[pid] & VDOPRC_OFUNC_DIS)) {
					p_ctx->cur_out_cfg_func[pid] |= VDOPRC_OFUNC_DIS;
					#if 0  //only update cur_out_cfg_func,if disable, would not push dis,but not stop dis
					//DBG_DUMP("-%s.out%d: enable DIS\r\n", p_thisunit->unit_name, pid);
					if (p_ctx->outq.dis_mode && (pid < ISF_VDOPRC_PHY_OUT_NUM)) {
						if (p_ctx->outq.dis_en == 0) {
							_isf_vdoprc_oport_start_dis_track(p_thisunit, pid + ISF_OUT_BASE);
						} else {
							_isf_vdoprc_oport_clear_dis_track(p_thisunit, pid + ISF_OUT_BASE);
						}
					} else {
						DBG_ERR("-%s.out%d: cannot start DIS\r\n", p_thisunit->unit_name, pid);
						return ISF_ERR_FAILED;
					}
					#endif
				} else if ((p_ctx->cur_out_cfg_func[pid] & VDOPRC_OFUNC_DIS) && !(p_ctx->new_out_cfg_func[pid] & VDOPRC_OFUNC_DIS)) {
					p_ctx->cur_out_cfg_func[pid] &= ~VDOPRC_OFUNC_DIS;
					//DBG_DUMP("-%s.out%d: disable DIS\r\n", p_thisunit->unit_name, pid);
				}
			}
#endif
		}

#if (USE_OUT_EXT == ENABLE)
	} else if (pid < ISF_VDOPRC_OUT_NUM) {

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		UINT32 imgfmt = p_vdoinfo->imgfmt;
		p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "fmt=%08x",	imgfmt);
		switch (imgfmt) {
		case VDO_PXLFMT_520_LCA:
		case VDO_PXLFMT_520_VA:
		case VDO_PXLFMT_520_WDR:
		case VDO_PXLFMT_520_DEFOG:
		case VDO_PXLFMT_520_3DNR_MS:
		case VDO_PXLFMT_520_3DNR_MV:
		case VDO_PXLFMT_520_3DNR_MD:
		case VDO_PXLFMT_520_3DNR_STA:
		case VDO_PXLFMT_520_IR8:
		case VDO_PXLFMT_520_IR16:
			p_ctx->phy_mask |= (1<<pid); //use as special pxlfmt path
			break;
		default:
			p_ctx->phy_mask &= ~(1<<pid); //use as extend path
			break;
		}

		//XDATA
		if (p_ctx->phy_mask & (1<<pid)) { //special pxlfmt path

			if (!en) {
				memset((void*)&(p_ctx->out[pid]), 0, sizeof(CTL_IPP_OUT_PATH));
				p_ctx->out[pid].pid = pid;
				p_ctx->out[pid].enable = FALSE;
				p_ctx->out[pid].fmt = 0;
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "off");
#if (USE_OUT_FRC == ENABLE)
				_isf_frc_stop(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]));
#endif
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH, (void *)&p_ctx->out[pid]);
			} else {
				switch (p_ctx->cur_mode) {
				case VDOPRC_PIPE_COLOR:
				case VDOPRC_PIPE_WARP:
					if(pid != 0) {
						DBG_ERR("-out%d: not support with pipe=%d!\r\n", pid, p_ctx->cur_mode);
						imgfmt = 0;
					}
					break;
				default:
					break;
				}

				p_ctx->out[pid].pid = pid;
				p_ctx->out[pid].enable = TRUE;
				p_ctx->out[pid].fmt = imgfmt;
#if (USE_OUT_FRC == ENABLE)
				if (p_vdoinfo->framepersecond == VDO_FRC_DIRECT) p_vdoinfo->framepersecond = VDO_FRC_ALL;
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM_IMM, "frc(%d,%d)",
					pid, GET_HI_UINT16(p_vdoinfo->framepersecond), GET_LO_UINT16(p_vdoinfo->framepersecond));
				if ((GET_HI_UINT16(p_vdoinfo->framepersecond) != GET_LO_UINT16(p_vdoinfo->framepersecond)) && (p_ctx->ctrl.cur_func & VDOPRC_FUNC_3DNR) && (pid == p_ctx->ctrl._3dnr_refpath)) {
					DBG_ERR("-out%d:frc(%d,%d) is conflict to func 3DNR, ignore this frc!!!\r\n", pid, GET_HI_UINT16(p_vdoinfo->framepersecond), GET_LO_UINT16(p_vdoinfo->framepersecond));
					p_vdoinfo->framepersecond = VDO_FRC_ALL;
				}
				if (p_ctx->outfrc[pid].sample_mode == ISF_SAMPLE_OFF) {
					_isf_frc_start(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]), p_vdoinfo->framepersecond);
				} else {
					_isf_frc_update_imm(p_thisunit, ISF_OUT(pid), &(p_ctx->outfrc[pid]), p_vdoinfo->framepersecond);
				}
#endif
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH, (void *)&p_ctx->out[pid]);
			}

		} else
#endif
		{  //extend path

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
			if (1) {
				memset((void*)&(p_ctx->out[pid]), 0, sizeof(CTL_IPP_OUT_PATH));
				p_ctx->out[pid].pid = pid;
				p_ctx->out[pid].enable = FALSE;
				p_ctx->out[pid].fmt = 0;
				p_thisunit->p_base->do_trace(p_thisunit, ISF_OUT(pid), ISF_OP_PARAM_VDOFRM, "off");
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_PATH, (void *)&p_ctx->out[pid]);
			}
#endif
			if (1) {
				_vdoprc_config_out_ext(p_thisunit, pid, en);
			}
		}
#endif
	} else {
		DBG_ERR("invalid out\r\n");
		return ISF_ERR_INVALID_PORT_ID;
	}
	return ISF_OK;
}



#if _TODO
static void _vdoprc_trigger_begin(ISF_UNIT *p_thisunit)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	p_ctx->outq.cur_trigdata_id = &p_ctx->outq.trigdata_id;
	//set id
	p_ctx->outq.cur_trigdata_id[0] = id;
	//DBG_MSG("%s trigger\r\n", p_thisunit->unit_name, p_ctx->outq.cur_trigdata_id);
}
// Note: only work under IPL_SAMPLE_RATE_ZERO mode
// Note: must enable => IPL_IME_QUICKTRIGGER (1)
static void _vdoprc_trigger_end(ISF_UNIT *p_thisunit)
{
#if (DUMP_KICK_ERR == ENABLE)
	ER ret;
#endif
#if (DUMP_PERFORMANCE == ENABLE)
	UINT64 t1, t2;
	t1 = HwClock_GetCounter();
#endif
#if _TODO
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	//DBG_MSG("%s.out[%d] trigger\r\n", p_thisunit->unit_name, p_ctx->outq.cur_trigdata_id);
	//IPL_SetCmd(IPL_SET_TRIG_IME_OUT, (void *)p_ctx->outq.cur_trigdata_id);
	UINT32 path = p_ctx->outq.cur_trigdata_id[0];
	#if (DUMP_KICK_ERR != ENABLE)
	IPL_Ctrl_Runtime_Chg_ISR((1 << id), IPC_Trig_IMEDMA, (void *)&path);
	#else
	ret = IPL_Ctrl_Runtime_Chg_ISR((1 << id), IPC_Trig_IMEDMA, (void *)&path);
	if (id+ISF_OUT_BASE == WATCH_PORT) {
		if (ret!=0) {
			DBG_DUMP("%s.out[%d]! Notify fail = %08x\r\n", p_thisunit->unit_name, path, ret);
		}
	}
	#endif
#endif
	//IPL_WaitCmdFinish();
#if (DUMP_PERFORMANCE == ENABLE)
	t2 = HwClock_GetCounter();
	DBG_DUMP("                                        IMGTRIG dt=%llu ms\r\n", (t2 - t1) >> 10);
#endif
}
#endif

/*
static void _vdoprc_oport_dumpdata(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	UINT32 j;
	unsigned long flags;
	loc_cpu(flags);
	DBG_DUMP("OutPOOL[%d]={", i);
	for (j = 0; j < p_ctx->outq.output_max[i]; j++) {
		if (p_ctx->outq.output_used[i][j] == 1) {
			DBG_DUMP("[%d]=%08x", j, p_ctx->outq.output_data[i][j].h_data);
		}
	}
	DBG_DUMP("}\r\n");
	unl_cpu(flags);
}
*/
extern void _vdoprc_oport_initqueue(ISF_UNIT *p_thisunit)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i, j;

#if (USE_OUT_ONEBUF == ENABLE)
	//init force buffer
	//DBG_DUMP("[OX]-clr-ref-3ndr\r\n");
	p_ctx->outq.force_refbuffer = 0;
	p_ctx->outq.force_refsize = 0;
	p_ctx->outq.force_refj = 0;
#endif

	for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++) {
		//XDATA
		p_ctx->outq.output_max[i] = VDOPRC_OUT_DEPTH_MAX;
		for (j = 0; j < VDOPRC_OUT_DEPTH_MAX; j++) {
			p_ctx->outq.output_data[i][j] = zero;
			p_ctx->outq.output_used[i][j] = 0;
			p_ctx->outq.output_type[i][j] = 0;
		}
	}
}

static UINT32 _vdoprc_oport_newdata(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 type)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	UINT32 j;
	ISF_DATA *p_data = NULL;
	unsigned long flags;
	loc_cpu(flags);
	for (j = 0; j < p_ctx->outq.output_max[i]; j++) {
		if (p_ctx->outq.output_used[i][j] == 0) {
			//DBG_DUMP("^C+1\r\n");
			p_ctx->outq.output_used[i][j] = 1;
			p_ctx->outq.output_type[i][j] = type;
			p_data = &(p_ctx->outq.output_data[i][j]);
			p_data->desc[0] = 0; //clear
			p_data->h_data = 0; //clear
			//p_data->serial = 0;
			unl_cpu(flags);
			return j;
		}
	}
	unl_cpu(flags);
	return 0xff;
}
static UINT32 _vdoprc_oport_adddata(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 j)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	unsigned long flags;
	loc_cpu(flags);
	p_ctx->outq.output_used[i][j] += 1;
	unl_cpu(flags);
	return 1;
}

UINT32 _vdoprc_oport_releasedata(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 j)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	unsigned long flags;
	loc_cpu(flags);
	if (p_ctx->outq.output_used[i][j] == 0) {
		unl_cpu(flags);
		return 0;
	}
	//DBG_DUMP("^C-1\r\n");
	p_ctx->outq.output_used[i][j] -= 1;
	//if (p_ctx->outq.output_used[i][j] == 0)
	//	p_data->h_data = 0; //clear
	unl_cpu(flags);
	return 1;
}

static void _vdoprc_oport_dumpqueue(ISF_UNIT *p_thisunit)
{
	UINT32 j,i;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	unsigned long flags;
	loc_cpu(flags);
	DBG_DUMP("^YPush Queue\r\n");
	for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++)
		//XDATA
		for (j = 0; j < VDOPRC_OUT_DEPTH_MAX; j++) {
			ISF_DATA *p_data = (ISF_DATA *)&(p_ctx->outq.output_data[i][j]);
			UINT32 addr = 0;
			if (p_data->h_data != 0)
				addr = nvtmpp_vb_blk2va(p_data->h_data);
			DBG_DUMP("%s.out[%d] - Vdo: blk_id=%08x addr=%08x blk[%d].cnt=%d\r\n", p_thisunit->unit_name, i, p_data->h_data, addr, j, p_ctx->outq.output_used[i][j]);
		}
	DBG_DUMP("^YCommon blk\r\n");
	nvtmpp_dump_status(debug_msg);
	unl_cpu(flags);
}

#if (USE_OUT_DIS == ENABLE)
extern void _isf_vdoprc_oport_open_dis(ISF_UNIT *p_thisunit, int w, int h);
extern void _isf_vdoprc_oport_close_dis(ISF_UNIT *p_thisunit);
void _isf_vdoprc_oport_open_dis(ISF_UNIT *p_thisunit, int w, int h)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	//DBG_DUMP("%s.out! open dis - begin\r\n", p_thisunit->unit_name);
	p_ctx->outq.dis_buf_size = 0;
	if (_vprc_dis_plugin_) {
		INT32 r;
		r = _vprc_dis_plugin_->open(p_ctx->dev, w, h, &p_ctx->outq.dis_buf_size);
		if (r != 0) {
			DBG_ERR("%s.ctrl! open DIS error %d!\r\n", p_thisunit->unit_name, r);
		} else {
			DBG_DUMP("%s.ctrl! open DIS\r\n", p_thisunit->unit_name);

			p_ctx->outq.dis_scaleratio = 1000;
			if (p_ctx->outq.dis_cfg_scaleratio != 0) {
				if ((p_ctx->outq.dis_cfg_scaleratio > 1000) && (p_ctx->outq.dis_cfg_scaleratio <= 1400)) {
					p_ctx->outq.dis_scaleratio = p_ctx->outq.dis_cfg_scaleratio;
				} else {
					DBG_ERR("%s.ctrl! open DIS fail: NOT support scaleratio w=%d!\r\n", p_thisunit->unit_name, (int)p_ctx->outq.dis_cfg_scaleratio);
				}
			} else {
				/* set to default */
				p_ctx->outq.dis_scaleratio = 1100;
			}

			p_ctx->outq.dis_subsample = 0;
			if (p_ctx->outq.dis_cfg_subsample != 0) {
				p_ctx->outq.dis_subsample = p_ctx->outq.dis_cfg_subsample  & ~VPRC_DIS_SUBSAMPLE_CFG;;
			} else {
				/* set to default */
				if ((w > 0) && (w <= 1080)) {
					p_ctx->outq.dis_subsample = 0;
				} else if ((w > 1080) && (w <= 2160)) {
					p_ctx->outq.dis_subsample = 1;
				} else if ((w > 2160) && (w <= 3840)) {
					p_ctx->outq.dis_subsample = 2;
				} else {
					DBG_ERR("%s.ctrl! open DIS fail: NOT support sensor w=%d!\r\n", p_thisunit->unit_name, (int)w);
					p_ctx->outq.dis_subsample = 0;
				}
			}

		}
	} else {
		//
		p_ctx->outq.dis_buf_size = 0;
		//DBG_DUMP("DIS-dummy open ok\r\n");
		DBG_ERR("%s.ctrl! open DIS fail: dis plugin missing!\r\n", p_thisunit->unit_name);
	}
	//DBG_DUMP("--- open dis return buf_size=%08x\r\n",
	//p_ctx->outq.dis_buf_size);
	//DBG_DUMP("%s.out! open dis - end\r\n", p_thisunit->unit_name);
}
void _isf_vdoprc_oport_close_dis(ISF_UNIT *p_thisunit)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	//DBG_DUMP("%s.out! close dis - begin\r\n", p_thisunit->unit_name);
	if (_vprc_dis_plugin_) {
		INT32 r = _vprc_dis_plugin_->close(p_ctx->dev);
		if (r != 0) {
			DBG_ERR("%s.ctrl! close DIS error %d!\r\n", p_thisunit->unit_name, r);
		}
	} else {
		//
		//DBG_DUMP("DIS-dummy close ok\r\n");
		DBG_ERR("%s.ctrl! close DIS fail: dis plugin missing!\r\n", p_thisunit->unit_name);
	}
	p_ctx->outq.dis_buf_addr = 0;
	//DBG_DUMP("%s.out! close dis - end\r\n", p_thisunit->unit_name);
}
static void _isf_vdoprc_oport_clear_dis_track(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	unsigned long flags;

	if(p_ctx->outq.track_data[i] != 0) {
		ISF_DATA* p_track_data = p_ctx->outq.track_data[i];
		UINT32 track_j = p_ctx->outq.track_j[i];

		//push
		//DBG_DUMP("vprc.out[%d] stop dis track blk[%d] => %08x\r\n", i, track_j, p_track_data->h_data);
		//p_thisunit->p_base->do_push(p_thisunit, oport, p_track_data, 0); //push to physical out
		p_thisunit->p_base->do_release(p_thisunit, oport, p_track_data, ISF_OUT_PROBE_PUSH_DROP); //remove 1 to this "track buffer"
		//if(i == WATCH_PORT){	DBG_DUMP("^Mpush-e %08x\r\n",track_j); }
		_vdoprc_oport_releasedata(p_thisunit, oport, track_j);
		dis_loc_cpu(flags);
		p_ctx->outq.track_data[i] = 0;
		dis_unl_cpu(flags);
	}
}
static void _isf_vdoprc_oport_start_dis_track(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

	if (p_ctx->outq.dis_en == 0) {
		//DBG_DUMP("%s.out! start dis - begin\r\n", p_thisunit->unit_name);
		DBG_DUMP("%s.out[%d]! start DIS => buf_add=%08x, subsample=%d, scaleratio=%d\r\n", p_thisunit->unit_name, oport,
			p_ctx->outq.dis_buf_addr, (int)p_ctx->outq.dis_subsample, (int)p_ctx->outq.dis_scaleratio);
		if (_vprc_dis_plugin_) {
			INT32 r = _vprc_dis_plugin_->start(p_ctx->dev, p_ctx->outq.dis_buf_addr, p_ctx->outq.dis_subsample, p_ctx->outq.dis_scaleratio);
			if (r != 0) {
				DBG_ERR("%s.out[%d]! start DIS error %d!\r\n", p_thisunit->unit_name, oport, r);
			}
		} else {
			//
			//DBG_DUMP("DIS-dummy start ok\r\n");
			DBG_ERR("%s.out[%d]! start DIS fail: dis plugin missing!\r\n", p_thisunit->unit_name, oport);
		}
		//DBG_DUMP("%s.out! start dis - end\r\n", p_thisunit->unit_name);
	}
	p_ctx->outq.dis_en ++;
    p_ctx->outq.dis_compensation[oport-ISF_OUT_BASE]=1;

	//DBG_DUMP("\r\nvprc.out[%d] dis start(%d) compen=%d\r\n", oport, p_ctx->outq.dis_en,p_ctx->outq.dis_compensation[oport-ISF_OUT_BASE]);
	_isf_vdoprc_oport_clear_dis_track(p_thisunit, oport);
}

static void _isf_vdoprc_oport_stop_dis_track(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

	if (p_ctx->outq.dis_en) {
		p_ctx->outq.dis_en --;
		if (p_ctx->outq.dis_en == 0) {
			//DBG_DUMP("%s.out! stop dis - begin\r\n", p_thisunit->unit_name);
			if (_vprc_dis_plugin_) {
				INT32 r = _vprc_dis_plugin_->stop(p_ctx->dev);
				if (r != 0) {
					DBG_ERR("%s.out[%d]! stop DIS error %d!\r\n", p_thisunit->unit_name, oport, r);
				}
			} else {
				//
				//DBG_DUMP("DIS-dummy stop ok\r\n");
				DBG_ERR("%s.out[%d]! stop DIS fail: dis plugin missing!\r\n", p_thisunit->unit_name, oport);
			}
			//DBG_DUMP("%s.out! stop dis - end\r\n", p_thisunit->unit_name);
		}
	}
    p_ctx->outq.dis_compensation[oport-ISF_OUT_BASE]=0;
	//DBG_DUMP("\r\nvprc.out[%d] dis stop(%d) compen=%d\r\n", oport, p_ctx->outq.dis_en,p_ctx->outq.dis_compensation[oport-ISF_OUT_BASE]);
	_isf_vdoprc_oport_clear_dis_track(p_thisunit, oport);
}

static ISF_RV _isf_vdoprc_oport_pop_dis_track(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA **p_out_data, UINT32* p_out_j)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;

	if(p_ctx->outq.track_data[i] == 0) {
		if (p_out_data)
			p_out_data[0] = 0;
		if (p_out_j)
			p_out_j[0] = 0;
		return ISF_ERR_FAILED;

	} else {
		UINT32 p;
		ISF_DATA* p_track_data = p_ctx->outq.track_data[i];
		UINT32 track_framecnt = p_ctx->outq.track_framecnt[i];
		UINT32 track_j = p_ctx->outq.track_j[i];
		ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
		ISF_VDO_INFO *p_dest_vdoinfo = (ISF_VDO_INFO *)(p_thisunit->p_base->get_bind_info(p_thisunit, oport));
		VDO_FRAME *p_vdoframe = (VDO_FRAME*)(p_track_data->desc);
		UINT32 mv_score = 0;
		INT32 mv_dx = 0, mv_dy = 0;
		//create cropped sub-image from original full-image
		UINT32 full_w = p_vdoframe->size.w;
		UINT32 full_h = p_vdoframe->size.h;
		UINT32 crop_w = p_vdoinfo->imgsize.w;
		UINT32 crop_h = p_vdoinfo->imgsize.h;
		//UINT32 scale = p_ctx->outq.dis_scale_factor[i];
		UINT32 p_count = 0;
		UINT32 org_x;
		UINT32 org_y;
		INT32 crop_x;
		INT32 crop_y;
		unsigned long flags;

		//DBG_DUMP("vprc.out[%d] pop dis track blk[%d] => %08x\r\n", i, track_j, p_track_data->h_data);

		if (p_dest_vdoinfo && (crop_w == 0 && crop_h == 0)) {
			//auto sync w,h from dest
			crop_w = p_dest_vdoinfo->imgsize.w;
			crop_h = p_dest_vdoinfo->imgsize.h;
		}

		//call dis-tracking
		if (_vprc_dis_plugin_) {
			INT32 r = 0;
            if(p_ctx->outq.dis_compensation[i]==2) {
                r = _vprc_dis_plugin_->set(p_ctx->dev, _VDOPRC_DIS_PARAM_RESET,0);
    			if (r != 0) {
    				DBG_ERR("DIS set error %d\r\n", r);
    			}
                p_ctx->outq.dis_compensation[i] = 1;
            }
            r = _vprc_dis_plugin_->proc(p_ctx->dev, track_framecnt, &mv_score, &mv_dx, &mv_dy);
			if (r != 0) {
				DBG_ERR("DIS proc error %d\r\n", r);
			}
            //disable compensation
            if(p_ctx->outq.dis_compensation[i]==0) {
                mv_score = 0;
		        mv_dx = 0;
                mv_dy = 0;
            }

		} else {
			static INT32 cc=0;
			mv_score = 1;
			if(cc & 1) {
				mv_dx = 10; mv_dy = 10;
			} else {
				mv_dx = -10; mv_dy = -10;
			}
			cc++;
		}

		switch (p_vdoframe->pxlfmt) {
		case VDO_PXLFMT_YUV420:
		case VDO_PXLFMT_YUV422:
			p_count = 2;
			break;
		case VDO_PXLFMT_Y8:
			p_count = 1;
			break;
		default:
			break;
		}

		org_x = (full_w-crop_w) >> 1;
		org_y = (full_h-crop_h) >> 1;
		//add 2048 for rounding, then divide to 4096
		crop_x = (((INT32)crop_w*mv_dx + 2048) >> 12);
		crop_y = (((INT32)crop_h*mv_dy + 2048) >> 12);
		p_vdoframe->size.w = crop_w;
		p_vdoframe->size.h = crop_h;
		//#if (DUMP_TRACK == ENABLE)
		//if (i+ISF_OUT_BASE == WATCH_PORT) {
		//DBG_DUMP("vprc.out[%d] frame_count=%d, DIS: scale(%d/1000) => (0, 0, %d, %d), mv=(%d, %d), score=%d => crop(%d, %d, %d, %d)\r\n",
		//	i, track_framecnt, scale, full_w, full_h, mv_dx, mv_dy, mv_score, crop_x, crop_y, crop_w, crop_h);
		//}
		//#endif
		for (p = 0; p < p_count; p++) {
			UINT32 ch_org_x = org_x;
			UINT32 ch_org_y = org_y;
			//add 2048 for rounding, then divide to 4096
			UINT32 ch_crop_x = crop_x;
			UINT32 ch_crop_y = crop_y;
			UINT32 ch_crop_w = crop_w;
			UINT32 ch_crop_h = crop_h;
			ch_crop_x += ch_org_x;
			ch_crop_y += ch_org_y;
			switch (p_vdoframe->pxlfmt) {
			case VDO_PXLFMT_YUV422:
				if (p > 0) {
					//ch_crop_x >>= 1;
					//ch_crop_w >>= 1;
				}
				break;
			case VDO_PXLFMT_YUV420:
				if (p > 0) {
					//ch_crop_x >>= 1;
					//ch_crop_w >>= 1;
					ch_crop_y >>= 1;
					ch_crop_h >>= 1;
				}
				break;
			default:
				break;
			}
			p_vdoframe->pw[p] = ch_crop_w;
			p_vdoframe->ph[p] = ch_crop_h;

            if(p_ctx->outq.dis_addr_align[i]) {
                ch_crop_x &= ~(p_ctx->outq.dis_addr_align[i]-1);  //for rotate panel would alig
            }
			if (p_vdoframe->addr[p]) {
				p_vdoframe->addr[p] = (UINT32)p_vdoframe->addr[p] + ch_crop_x + (p_vdoframe->loff[p] * ch_crop_y);
				if (p == 1) { //UV
					p_vdoframe->addr[p] &= ~0x01; //force 2 align
				}
			}
			if (p_vdoframe->phyaddr[p]) {
				p_vdoframe->phyaddr[p] = (UINT32)p_vdoframe->phyaddr[p] + ch_crop_x + (p_vdoframe->loff[p] * ch_crop_y);
				if (p == 1) { //UV
					p_vdoframe->phyaddr[p] &= ~0x01; //force 2 align
				}
			}
		}

		/*
			DBG_DUMP("vprc.out[%d] dim=(%d, %d), y=(%d, %d, %d, %08x, %08x), uv=(%d, %d %d, %08x, %08x)\r\n",
				i, p_vdoframe->size.w, p_vdoframe->size.h,
				p_vdoframe->pw[0], p_vdoframe->ph[0], p_vdoframe->loff[0], p_vdoframe->addr[0], p_vdoframe->phyaddr[0],
				p_vdoframe->pw[1], p_vdoframe->ph[1], p_vdoframe->loff[1], p_vdoframe->addr[1], p_vdoframe->phyaddr[1]);
		*/

		dis_loc_cpu(flags);
		p_ctx->outq.track_data[i] = 0;
		dis_unl_cpu(flags);

		if (p_out_data) {
			//DBG_DUMP("replace %08x => %08x\r\n", p_out_data[0]->h_data, p_track_data->h_data);
			p_out_data[0] = p_track_data;
		}
		if (p_out_j) {
			//DBG_DUMP("replace %08x => %08x\r\n", p_out_j[0], track_j);
			p_out_j[0] = track_j;
		}
	}
	return ISF_OK;
}

//push track frame
static ISF_RV _isf_vdoprc_oport_push_dis_track(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_in_data, UINT32 in_j, UINT32 in_frame_cnt)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	unsigned long flags;

	if (!p_in_data) {
		return ISF_ERR_FAILED;
	}

	dis_loc_cpu(flags);
	//DBG_DUMP("push frame_cnt=%d\r\n", frame_cnt);
	p_ctx->outq.track_data[i] = p_in_data;
	p_ctx->outq.track_framecnt[i] = in_frame_cnt;
	p_ctx->outq.track_j[i] = in_j;
	dis_unl_cpu(flags);
	//DBG_DUMP("vprc.out[%d] push dis track blk[%d] => %08x\r\n", i, in_j, p_in_data->h_data);
	return ISF_OK;
}

#endif


static void _isf_vdoprc_oport_enable(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	unsigned long flags;
	if (p_ctx->outq.output_cur_en[i] == TRUE)
		return;
	p_ctx->outq.output_cur_en[i] = TRUE;
#if (USE_OUT_ONEBUF == ENABLE)
	//DBG_DUMP("[OX]%s.out[%d]-clr-one\r\n", p_thisunit->unit_name, oport);
	p_ctx->outq.force_onebuffer[i] = 0;
	p_ctx->outq.force_onesize[i] = 0;
	p_ctx->outq.force_onej[i] = 0;
	p_ctx->outq.force_one_reset[i] = 0;
	p_ctx->outq.force_twobuffer[i] = 0;
#endif
	p_ctx->outq.count_new_ok[i] = 0;
	p_ctx->outq.count_new_fail[i] = 0;
	p_ctx->outq.count_push_ok[i] = 0;
	p_ctx->outq.count_push_fail[i] = 0;
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
	DBG_DUMP("^C%s.out[%d]! ----------------------------------------------- Start Begin\r\n", p_thisunit->unit_name, oport);
	}
#endif
#if 1
	{
	UINT32 j;
	UINT32 errj = 0;
	BOOL is_err = FALSE;
	loc_cpu(flags);
	//output queue
	for (j = 0; j < VDOPRC_OUT_DEPTH_MAX && !is_err; j++) {
		BOOL is_ref = p_ctx->outq.output_type[i][j];
		if (!is_ref && (p_ctx->outq.output_used[i][j] != 0)) {
			errj = j;
			is_err = TRUE;
		}
	}
	unl_cpu(flags);
	if (is_err) {
		DBG_ERR("%s.out[%d]! Start Begin check fail, blk[%d] is not release?\r\n", p_thisunit->unit_name, oport, errj);
		_vdoprc_oport_dumpqueue(p_thisunit);
	}
	loc_cpu(flags);
	//output queue
	for(j = 0; j < VDOPRC_OUT_DEPTH_MAX; j++) {
		BOOL ref = p_ctx->outq.output_type[i][j];
		if (!ref) {
		p_ctx->outq.output_data[i][j] = zero;
		p_ctx->outq.output_used[i][j] = 0;
		}
	}
	p_ctx->outq.output_cnt[i] = 0;
	//if (oport == WATCH_PORT) {DBG_DUMP("X-1:");DBGD(p_ctx->outq.output_cnt[i]);}
	unl_cpu(flags);
	}
#endif
#if (USE_OUT_DIS == ENABLE)
	//p_ctx->outq.dis_start[i] keep i path if start dis
	//i path frist start would enter start_dis for count++
	if (p_ctx->new_out_cfg_func[i] & VDOPRC_OFUNC_DIS) {
		//DBG_DUMP("-%s.out%d: enable DIS\r\n", p_thisunit->unit_name, i);
		if (p_ctx->outq.dis_mode && (i < ISF_VDOPRC_PHY_OUT_NUM)) {
            if(!(p_ctx->outq.dis_start[i] & VDOPRC_OFUNC_DIS)) {
    		    p_ctx->outq.dis_start[i] |= VDOPRC_OFUNC_DIS;
    			_isf_vdoprc_oport_start_dis_track(p_thisunit, oport);
            }
		} else {
			DBG_ERR("-%s.out%d: cannot start DIS\r\n", p_thisunit->unit_name, i);
			return;
		}
	}
#endif
#if (USE_EIS == ENABLE)
	if (_vprc_eis_plugin_ && p_ctx->eis_func) {
		if (i < MAX_EIS_PATH_NUM) {
			_vdoprc_eis_reset_queue(i);
		}
	}
#endif

}

static void _isf_vdoprc_oport_disable(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
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

void _isf_vdoprc_oport_do_start(ISF_UNIT *p_thisunit, UINT32 oport)
{
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
	DBG_DUMP("%s.out[%d]! ----------------------------------------------- Start End\r\n", p_thisunit->unit_name, oport);
	}
#endif
}

// after IME Framend, it will push buffer, then set IPL, and decide this path is stopped by runtime change, then send io stop event
void _isf_vdoprc_oport_do_stop(ISF_UNIT *p_thisunit, UINT32 oport)
{
#if (USE_OUT_ONEBUF == ENABLE)
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	UINT32 is_out_one = (p_ctx->cur_out_cfg_func[i] & VDOPRC_OFUNC_ONEBUF);
	UINT32 is_out_two = (p_ctx->cur_out_cfg_func[i] & VDOPRC_OFUNC_TWOBUF);
	UINT32 is_3dnr_ref = ((p_ctx->ctrl.func & VDOPRC_FUNC_3DNR) && (p_ctx->ctrl._3dnr_refpath == i));
#endif
	//ISF_PORT* p_src = p_thisunit->port_out[i];
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
	DBG_DUMP("^C%s.out[%d]! ----------------------------------------------- Stop end\r\n", p_thisunit->unit_name, oport);
	}
#endif
#if (USE_OUT_DIS == ENABLE)
	//p_ctx->outq.dis_start[i] keep i path if start dis
	//i path frist stop would enter stop_dis for count--
	if (p_ctx->new_out_cfg_func[i] & VDOPRC_OFUNC_DIS) {
		//DBG_DUMP("-%s.out%d: enable DIS\r\n", p_thisunit->unit_name, i);
		if (p_ctx->outq.dis_mode && (i < ISF_VDOPRC_PHY_OUT_NUM)) {
            if(p_ctx->outq.dis_start[i] & VDOPRC_OFUNC_DIS) {
		        p_ctx->outq.dis_start[i] &= ~VDOPRC_OFUNC_DIS;
    			_isf_vdoprc_oport_stop_dis_track(p_thisunit, oport);
            }
		} else {
			DBG_ERR("-%s.out%d: cannot stop DIS\r\n", p_thisunit->unit_name, i);
			return;
		}
	}
#endif
#if (USE_EIS == ENABLE)
	if (_vprc_eis_plugin_ && p_ctx->eis_func) {
		if (i < MAX_EIS_PATH_NUM) {
			_vdoprc_eis_release_frame(p_thisunit, oport);

		}
	}
#endif

#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
	//_vdoprc_oport_dumpdata(p_thisunit, oport);
	//DBG_DUMP("^C%s.out[%d]! free begin\r\n", p_thisunit->unit_name, oport);
	}
#endif
#if (USE_OUT_ONEBUF == ENABLE)
	if (is_out_one && is_3dnr_ref && (p_ctx->max_strp_num > 1)) {
		//OUT(one buffer) + IN(D2D/direct) + 3dnr effect: require two buffer for make sure 3DNR work
		if (p_ctx->outq.force_onebuffer[i] != 0) {
			UINT32 j = p_ctx->outq.force_onej[i];
			ISF_DATA* p_data = &(p_ctx->outq.output_data[i][j]);
			DBG_WRN("%s.out[%d]! Stop single blk mode => blk_id=%08x\r\n", p_thisunit->unit_name, oport, p_ctx->outq.force_onebuffer[i]);
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //remove 1 reference to this "single buffer"
			//p_data->h_data = p_ctx->outq.force_onebuffer[i];
			//DBG_DUMP("[OX]%s.out[%d]-stop-one-3dnrX\r\n", p_thisunit->unit_name, oport);
			p_ctx->outq.force_onebuffer[i] = 0;
			p_ctx->outq.force_onesize[i] = 0;
			p_ctx->outq.force_onej[i] = 0;
		}
		if (p_ctx->outq.force_refbuffer != 0) {
			UINT32 j = p_ctx->outq.force_refj;
			ISF_DATA* p_data = &(p_ctx->outq.output_data[i][j]);
			DBG_WRN("%s.out[%d]! Stop single blk mode-2 => blk_id=%08x\r\n", p_thisunit->unit_name, oport, p_ctx->outq.force_refbuffer);
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //remove 1 reference to this "single buffer"
			//DBG_DUMP("[OX]%s.out[%d]-stop-ref-3dnrX\r\n", p_thisunit->unit_name, oport);
			p_ctx->outq.force_refbuffer = 0;
			p_ctx->outq.force_refsize = 0;
			p_ctx->outq.force_refj = 0;
		}
		p_ctx->outq.output_max[i] = VDOPRC_OUT_DEPTH_MAX;
	} else if (is_out_one && is_3dnr_ref && (p_ctx->max_strp_num == 1)) {
		//OUT(one buffer) + IN(D2D/direct) + 3dnr effect: require one buffer for make sure 3DNR work
		if (p_ctx->outq.force_onebuffer[i] != 0) {
			UINT32 j = p_ctx->outq.force_onej[i];
			ISF_DATA* p_data = &(p_ctx->outq.output_data[i][j]);
			DBG_WRN("%s.out[%d]! Stop single blk mode => blk_id=%08x\r\n", p_thisunit->unit_name, oport, p_ctx->outq.force_onebuffer[i]);
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //remove 1 reference to this "single buffer"
			//p_data->h_data = p_ctx->outq.force_onebuffer[i];
			//DBG_DUMP("[OX]%s.out[%d]-stop-one-3dnr\r\n", p_thisunit->unit_name, oport);
			p_ctx->outq.force_onebuffer[i] = 0;
			p_ctx->outq.force_onesize[i] = 0;
			p_ctx->outq.force_onej[i] = 0;
		}
		if (p_ctx->outq.force_refbuffer != 0) {   //should be 1
			//DBG_DUMP("[OX]%s.out[%d]-stop-ref-3dnr\r\n", p_thisunit->unit_name, oport);
			p_ctx->outq.force_refbuffer = 0;
		}
		p_ctx->outq.output_max[i] = VDOPRC_OUT_DEPTH_MAX;
	} else if (is_out_one) {
		//OUT(one buffer) + IN(D2D/direct): require one buffer only (r/w maybe overlapping)
		if (p_ctx->outq.force_onebuffer[i] != 0) {
			UINT32 j = p_ctx->outq.force_onej[i];
			ISF_DATA* p_data = &(p_ctx->outq.output_data[i][j]);
			DBG_WRN("%s.out[%d]! Stop single blk mode => blk_id=%08x\r\n", p_thisunit->unit_name, oport, p_ctx->outq.force_onebuffer[i]);
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //remove 1 reference to this "single buffer"
			//p_data->h_data = p_ctx->outq.force_onebuffer[i];
			//DBG_DUMP("[OX]%s.out[%d]-stop-one\r\n", p_thisunit->unit_name, oport);
			p_ctx->outq.force_onebuffer[i] = 0;
			p_ctx->outq.force_onesize[i] = 0;
			p_ctx->outq.force_onej[i] = 0;
		}
		p_ctx->outq.output_max[i] = VDOPRC_OUT_DEPTH_MAX;
	} else if (is_out_two) {
	    static ISF_DATA tmp_data = {0};
        ISF_DATA* p_data = &tmp_data;
    	static ISF_DATA_CLASS g_vdoprc_out_data = {0};
	    p_thisunit->p_base->init_data(p_data, &g_vdoprc_out_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
		if (p_ctx->outq.force_onebuffer[i] != 0) {
            p_data->h_data = p_ctx->outq.force_onebuffer[i];
			DBG_WRN("%s.out[%d]! Stop 1st blk mode => blk_id=%08x\r\n", p_thisunit->unit_name, oport, p_ctx->outq.force_onebuffer[i]);
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //remove 1 reference to this "1st buffer"
			p_ctx->outq.force_onebuffer[i] = 0;
			p_ctx->outq.force_onesize[i] = 0;
			p_ctx->outq.force_onej[i] = 0;
		}
        if (p_ctx->outq.force_twobuffer[i] != 0) {
            p_data->h_data = p_ctx->outq.force_twobuffer[i];
			DBG_WRN("%s.out[%d]! Stop 2nd blk mode => blk_id=%08x\r\n", p_thisunit->unit_name, oport, p_ctx->outq.force_twobuffer[i]);
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //remove 1 reference to this "2nd buffer"
			p_ctx->outq.force_twobuffer[i] = 0;
		}
		p_ctx->outq.output_max[i] = VDOPRC_OUT_DEPTH_MAX;
	}
#endif
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
	//DBG_DUMP("^C%s.out[%d]! free end\r\n", p_thisunit->unit_name, oport);
	}
#endif
}

// reset for onebuf mode
void _isf_vdoprc_oport_do_reset(ISF_UNIT *p_thisunit, UINT32 oport)
{
#if (USE_OUT_ONEBUF == ENABLE)
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	UINT32 is_out_one = (p_ctx->cur_out_cfg_func[i] & VDOPRC_OFUNC_ONEBUF);
	UINT32 is_3dnr_ref = ((p_ctx->ctrl.cur_func & VDOPRC_FUNC_3DNR) && (p_ctx->ctrl._3dnr_refpath == i));
#endif
	//ISF_PORT* p_src = p_thisunit->port_out[i];
#if (USE_OUT_ONEBUF == ENABLE)
	if (is_out_one && is_3dnr_ref && (p_ctx->max_strp_num > 1)) {
		//OUT(one buffer) + IN(D2D/direct) + 3dnr effect: require two buffer for make sure 3DNR work
		if (p_ctx->outq.force_onebuffer[i] != 0) {
			/*
			UINT32 j = p_ctx->outq.force_onej[i];
			ISF_DATA* p_data = &(p_ctx->outq.output_data[i][j]);
			DBG_WRN("%s.out[%d]! Reset single blk mode => blk_id=%08x\r\n", p_thisunit->unit_name, oport, p_ctx->outq.force_onebuffer[i]);
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //remove 1 reference to this "single buffer"
			//p_data->h_data = p_ctx->outq.force_onebuffer[i];
			DBG_DUMP("[OX]%s.out[%d]-stop-one-3dnrX\r\n", p_thisunit->unit_name, oport);
			p_ctx->outq.force_onebuffer[i] = 0;
			p_ctx->outq.force_onesize[i] = 0;
			p_ctx->outq.force_onej[i] = 0;
			*/
		}
		if (p_ctx->outq.force_refbuffer != 0) {
            /*
			UINT32 j = p_ctx->outq.force_refj;
			ISF_DATA* p_data = &(p_ctx->outq.output_data[i][j]);
			DBG_WRN("%s.out[%d]! Reset single blk mode-2 => blk_id=%08x\r\n", p_thisunit->unit_name, oport, p_ctx->outq.force_refbuffer);
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //remove 1 reference to this "single buffer"
			//DBG_DUMP("[OX]%s.out[%d]-stop-ref-3dnrX\r\n", p_thisunit->unit_name, oport);
			p_ctx->outq.force_refbuffer = 0;
			p_ctx->outq.force_refsize = 0;
			p_ctx->outq.force_refj = 0;
			*/
		}
		//p_ctx->outq.output_max[i] = VDOPRC_OUT_DEPTH_MAX;
	} else if (is_out_one && is_3dnr_ref && (p_ctx->max_strp_num == 1)) {
		//OUT(one buffer) + IN(D2D/direct) + 3dnr effect: require one buffer for make sure 3DNR work
		if (p_ctx->outq.force_onebuffer[i] != 0) {
			/*
			UINT32 j = p_ctx->outq.force_onej[i];
			ISF_DATA* p_data = &(p_ctx->outq.output_data[i][j]);
			DBG_WRN("%s.out[%d]! Reset single blk mode => blk_id=%08x\r\n", p_thisunit->unit_name, oport, p_ctx->outq.force_onebuffer[i]);
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //remove 1 reference to this "single buffer"
			//p_data->h_data = p_ctx->outq.force_onebuffer[i];
			DBG_DUMP("[OX]%s.out[%d]-stop-one-3dnr\r\n", p_thisunit->unit_name, oport);
			p_ctx->outq.force_onebuffer[i] = 0;
			p_ctx->outq.force_onesize[i] = 0;
			p_ctx->outq.force_onej[i] = 0;
			*/
		}
		if (p_ctx->outq.force_refbuffer != 0) {   //should be 1
			//DBG_DUMP("[OX]%s.out[%d]-stop-ref-3dnr\r\n", p_thisunit->unit_name, oport);
			p_ctx->outq.force_refbuffer = 0;
		}
		//p_ctx->outq.output_max[i] = VDOPRC_OUT_DEPTH_MAX;
	} else if (is_out_one) {
		//OUT(one buffer) + IN(D2D/direct): require one buffer only (r/w maybe overlapping)
		if (p_ctx->outq.force_onebuffer[i] != 0) {
			/*
			UINT32 j = p_ctx->outq.force_onej[i];
			ISF_DATA* p_data = &(p_ctx->outq.output_data[i][j]);
			DBG_WRN("%s.out[%d]! Reset single blk mode => blk_id=%08x\r\n", p_thisunit->unit_name, oport, p_ctx->outq.force_onebuffer[i]);
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //remove 1 reference to this "single buffer"
			//p_data->h_data = p_ctx->outq.force_onebuffer[i];
			DBG_DUMP("[OX]%s.out[%d]-stop-one\r\n", p_thisunit->unit_name, oport);
			p_ctx->outq.force_onebuffer[i] = 0;
			p_ctx->outq.force_onesize[i] = 0;
			p_ctx->outq.force_onej[i] = 0;
			*/
		}
		//p_ctx->outq.output_max[i] = VDOPRC_OUT_DEPTH_MAX;
	}
#endif
}


UINT32 _isf_vdoprc_oport_do_new(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_size, UINT32 ddr, UINT32* p_addr)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	UINT32 j = 0xff;
	ISF_PORT* p_port = p_thisunit->port_out[i];
	ISF_DATA *p_data = NULL;
	UINT32 addr = 0;
	UINT32 n = p_ctx->outq.output_cnt[i];
	UINT32 ctype;
	UINT32 is_ref = FALSE;
	unsigned long flags;
#if (USE_OUT_ONEBUF == ENABLE)
#if (USE_IN_DIRECT == ENABLE)
	UINT32 is_in_dir = (p_ctx->cur_in_cfg_func & VDOPRC_IFUNC_DIRECT);
#endif
	UINT32 is_out_one = (p_ctx->cur_out_cfg_func[i] & VDOPRC_OFUNC_ONEBUF);
	UINT32 is_out_two = (p_ctx->cur_out_cfg_func[i] & VDOPRC_OFUNC_TWOBUF);
	UINT32 is_3dnr_ref = ((p_ctx->ctrl.cur_func & VDOPRC_FUNC_3DNR) && (p_ctx->ctrl._3dnr_refpath == i));
    UINT32 new_buf_size = 0;
#endif
	#ifdef ISF_TS
	UINT64 ts;
	#endif

	static ISF_DATA_CLASS g_vdoprc_out_data = {0};
    UINT32 is_copy_meta = (p_ctx->out_vndcfg_func[i] & VDOPRC_OFUNC_VND_COPYMETA);
    UINT32 total_meta_size = 0;
    VDO_FRAME* p_ipp_frame = NULL;
    VDO_FRAME *p_inq_frame = NULL;

	#ifdef ISF_TS
	ts = hwclock_get_longcounter();
	#endif
	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW, ISF_ENTER);


	if (buf_size & 0x80000000) {
		is_ref = TRUE;//this is a ref blk
		buf_size &= ~0x80000000;
	}

	if (!p_addr) {
        DBG_ERR("no p_addr\r\n");
        return j;
	}

	//p_thisunit->p_base->do_debug(p_thisunit, oport, ISF_OP_PARAM_VDOFRM, "-out%d:*p_addr %x is_copy_meta %d",oport,*p_addr,is_copy_meta);

    //META#### add meta size for new blk
    if((is_copy_meta)&&(*p_addr)) {
        p_ipp_frame =(VDO_FRAME* )(*p_addr);
        p_inq_frame = _isf_vdoprc_meta_get_frame(p_thisunit,p_ipp_frame->count);
        total_meta_size =_isf_vdoprc_meta_get_size(p_inq_frame);
        buf_size += total_meta_size;
		p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_VDOFRM, "-out%d:total_meta_size %x",oport,total_meta_size);
        *p_addr = 0;
    }

	//static UINT32 last_addr = 0;
#if (DUMP_FPS == ENABLE)
	if (i == WATCH_PORT) {
		DBG_FPS(p_thisunit, "out", i, "New");
	}
#endif
	if ((p_ctx->dev_ready == 0) || (p_ctx->dev_handle == 0)) {
		if (p_ctx->dev_trigger_open == 1) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_DROP, ISF_ERR_NOT_START);
		} if (p_ctx->dev_trigger_close == 1) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_DROP, ISF_ERR_NOT_START);
		} else {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_ERR, ISF_ERR_INACTIVE_STATE);
		}
		return j; //drop frame!
	}

#if (USE_OUT_FRC == ENABLE)
	//do frc
	if (_isf_frc_is_select(p_thisunit, oport, &(p_ctx->outfrc[i])) == 0) {
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_DROP, ISF_ERR_FRC_DROP);
		if (p_ctx->user_out_blk[i]) {
			ISF_DATA isf_data = {0};
			VDO_FRAME *p_vdoframe;

			p_data = &isf_data;
			p_thisunit->p_base->init_data(p_data, &g_vdoprc_out_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
			p_data->h_data = p_ctx->user_out_blk[i];
			p_vdoframe = (VDO_FRAME *)(p_data->desc);
			p_vdoframe->reserved[2] = MAKEFOURCC('D', 'R', 'O', 'P');
			//don't need to wait in frc mode
			_isf_vdoprc_oqueue_do_push_wait(p_thisunit, oport, p_data, 0);
		}
        *p_addr = CTL_IPP_E_DROP;
		return j; //drop frame!
	}
#endif

	if (p_port) {
		ctype = p_port->connecttype;
		p_ctx->outq.output_connecttype[i] = ctype;
	} else {
		p_ctx->outq.output_connecttype[i] = ISF_CONNECT_NONE;
	}


	//check block size only if user has set reserved[0] which is block size in HDAL
	if (p_ctx->user_out_blk[i] && p_ctx->user_out_blk_size[i]) {
		if (buf_size > p_ctx->user_out_blk_size[i]) {
			ISF_DATA isf_data = {0};
			VDO_FRAME *p_vdoframe;

			DBG_ERR("%s.out[%d] needs 0x%x > user_out_blk_size(0x%X)!\r\n", p_thisunit->unit_name, oport, buf_size, p_ctx->user_out_blk_size[i]);
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_WRN, ISF_ERR_NEW_FAIL);
			p_data = &isf_data;
			p_thisunit->p_base->init_data(p_data, &g_vdoprc_out_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
			p_data->h_data = p_ctx->user_out_blk[i];
			p_vdoframe = (VDO_FRAME *)(p_data->desc);
			p_vdoframe->reserved[2] = MAKEFOURCC('D', 'R', 'O', 'P');
			_isf_vdoprc_oqueue_do_push_wait(p_thisunit, oport, p_data, USER_OUT_BUFFER_QUEUE_TIMEOUT);
			return 0xFF;
		}
	}
	//if (oport == WATCH_PORT) {DBG_DUMP("X-3:");DBGD(p_ctx->outq.output_cnt[i]);}
	j = _vdoprc_oport_newdata(p_thisunit, oport, is_ref);
	//if (i == WATCH_PORT) {DBG_DUMP("^Cnew-b %08x\r\n",j); }
	if (j==0xff)
	{
#if (DUMP_DATA_ERROR == ENABLE)
		if (oport == WATCH_PORT) {
			DBG_WRN("%s.out[%d]! New -- overflow-1!!!\r\n", p_thisunit->unit_name, oport);
			//_vdoprc_oport_dumpdata(p_thisunit, oport);
		}
#endif
		if (p_ctx->user_out_blk[i]) {
			ISF_DATA isf_data = {0};
			VDO_FRAME *p_vdoframe;

			DBG_WRN("%s.out[%d]! New -- overflow-1!!!\r\n", p_thisunit->unit_name, oport);
			p_data = &isf_data;
			p_thisunit->p_base->init_data(p_data, &g_vdoprc_out_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
			p_data->h_data = p_ctx->user_out_blk[i];
			p_vdoframe = (VDO_FRAME *)(p_data->desc);
			p_vdoframe->reserved[2] = MAKEFOURCC('D', 'R', 'O', 'P');
			_isf_vdoprc_oqueue_do_push_wait(p_thisunit, oport, p_data, USER_OUT_BUFFER_QUEUE_TIMEOUT);
		}
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_WRN, ISF_ERR_QUEUE_FULL);
		return j;
	}
	p_data = &(p_ctx->outq.output_data[i][j]);

	p_thisunit->p_base->init_data(p_data, &g_vdoprc_out_data, MAKEFOURCC('V','F','R','M'), 0x00010000);

#if (USE_EIS == ENABLE)
	if (_vprc_eis_plugin_ && p_ctx->eis_func) {
		if (oport < MAX_EIS_PATH_NUM) {
			VDOPRC_EIS_PATH_INFO path_info = {0};
			ER ret;

			lut2d_buf_info[oport].offset = ALIGN_CEIL_4(buf_size);
			path_info.path_id = oport;
			ret = _vprc_eis_plugin_->get(VDOPRC_EIS_PARAM_PATH_INFO, &path_info);
			if (ret == E_OK && path_info.lut2d_buf_size) {
				lut2d_buf_info[oport].size = path_info.lut2d_buf_size;
				buf_size = ALIGN_CEIL_4(buf_size);
				buf_size += lut2d_buf_info[oport].size;
			} else {
				//EIS lib or path not opened
				//lut2d_buf_info[oport].size = (4*68*65);
				//DBG_WRN("%s.out[%d] uses max lut2d buf size(%d)\r\n", p_thisunit->unit_name, oport, lut2d_buf_info[oport].size);
			}

		}
	}
#endif
#if (USE_OUT_ONEBUF == ENABLE)
    if(p_ctx->outq.force_onesize_max[i]) {

        if(buf_size>p_ctx->outq.force_onesize_max[i]) {
            //DBG_ERR("%x > onesize_max: %x \r\n",buf_size,p_ctx->outq.force_onesize_max[i]);
    		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_WRN, ISF_ERR_BUFFER_CREATE);
    		_vdoprc_oport_releasedata(p_thisunit, oport, j);
    		j = 0xff;
    		return j;
        }
        new_buf_size = p_ctx->outq.force_onesize_max[i];
    } else {
        if((p_ctx->outq.force_onesize[i])&&(buf_size>p_ctx->outq.force_onesize[i])) {
            //DBG_ERR("%x > allocated: %x \r\n",buf_size,p_ctx->outq.force_onesize[i]);
    		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_WRN, ISF_ERR_BUFFER_CREATE);
    		_vdoprc_oport_releasedata(p_thisunit, oport, j);
    		j = 0xff;
    		return j;
        }
        new_buf_size = buf_size;
    }

	if (p_ctx->outq.force_one_reset[i]) { //do force reset!
		//DBG_DUMP("[OX]%s.out[%d]-reset-one-1\r\n", p_thisunit->unit_name, oport);
		_isf_vdoprc_oport_do_reset(p_thisunit, oport);
		//n = 0;
		//is_out_one = (p_ctx->cur_out_cfg_func[i] & VDOPRC_OFUNC_ONEBUF);
		//is_3dnr_ref = ((p_ctx->ctrl.cur_func & VDOPRC_FUNC_3DNR) && (p_ctx->ctrl._3dnr_refpath == i));
		p_ctx->outq.force_one_reset[i] = 0;
	}

	if (is_out_one && is_3dnr_ref && (p_ctx->max_strp_num > 1)) {
		//OUT(one buffer) + IN(D2D/direct) + 3dnr effect: require two buffer for make sure 3DNR work
		if ((p_ctx->outq.force_onebuffer[i] != 0) && (p_ctx->outq.force_refbuffer != 0)) { //under "single buffer mode", if try to new 2nd buffer
			if (n % 2) {
				//DBG_DUMP("[OX]%s.out[%d]-add-one-3dnrX-1\r\n", p_thisunit->unit_name, oport);
				p_data->h_data = p_ctx->outq.force_refbuffer;
				addr = nvtmpp_vb_blk2va(p_data->h_data);
				p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to this "single buffer"
			} else {
				//DBG_DUMP("[OX]%s.out[%d]-add-one-3dnrX-2\r\n", p_thisunit->unit_name, oport);
				p_data->h_data = p_ctx->outq.force_onebuffer[i];
				addr = nvtmpp_vb_blk2va(p_data->h_data);
				p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to this "single buffer"
			}
		} else {
			//DBG_DUMP("[OX]%s.out[%d]-new-one-3dnrX\r\n", p_thisunit->unit_name, oport);
			addr = p_thisunit->p_base->do_new(p_thisunit, oport, NVTMPP_VB_INVALID_POOL, ddr, new_buf_size, p_data, ISF_OUT_PROBE_NEW);
		}
	} else if (is_out_one && is_3dnr_ref && (p_ctx->max_strp_num == 1)) {
		//OUT(one buffer) + IN(D2D/direct) + 3dnr effect: require one buffer for make sure 3DNR work
		if ((p_ctx->outq.force_onebuffer[i] != 0)) { //under "single buffer mode", if try to new 2nd buffer
			//DBG_DUMP("[OX]%s.out[%d]-add-one-3dnr\r\n", p_thisunit->unit_name, oport);
			p_data->h_data = p_ctx->outq.force_onebuffer[i];
			addr = nvtmpp_vb_blk2va(p_data->h_data);
			p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to this "single buffer"
		} else {
			//DBG_DUMP("[OX]%s.out[%d]-new-one-3dnr\r\n", p_thisunit->unit_name, oport);
			addr = p_thisunit->p_base->do_new(p_thisunit, oport, NVTMPP_VB_INVALID_POOL, ddr, new_buf_size, p_data, ISF_OUT_PROBE_NEW);
		}
	} else if (is_out_one) {
		//OUT(one buffer) + IN(D2D/direct): require one buffer only (r/w maybe overlapping)
		if ((p_ctx->outq.force_onebuffer[i] != 0)) { //under "single buffer mode", if try to new 2nd buffer
			//DBG_DUMP("[OX]%s.out[%d]-add-one\r\n", p_thisunit->unit_name, oport);
			p_data->h_data = p_ctx->outq.force_onebuffer[i];
			addr = nvtmpp_vb_blk2va(p_data->h_data);
			p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to this "single buffer"
		} else {
			//DBG_DUMP("[OX]%s.out[%d]-new-one\r\n", p_thisunit->unit_name, oport);
			addr = p_thisunit->p_base->do_new(p_thisunit, oport, NVTMPP_VB_INVALID_POOL, ddr, new_buf_size, p_data, ISF_OUT_PROBE_NEW);
		}
	} else if (is_out_two) {
		if ((p_ctx->outq.force_onebuffer[i] != 0) && (p_ctx->outq.force_twobuffer[i] != 0)) { //under "two buffer mode"
			if (n % 2) {
				p_data->h_data = p_ctx->outq.force_twobuffer[i];
				addr = nvtmpp_vb_blk2va(p_ctx->outq.force_twobuffer[i]);
				p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to this "2nd buffer"
				//DBG_DUMP("[OX]%s.out[%d]-add-2-2 addr %x\r\n", p_thisunit->unit_name, oport,addr);

			} else {
				p_data->h_data = p_ctx->outq.force_onebuffer[i];
				addr = nvtmpp_vb_blk2va(p_data->h_data);
				p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to this "1st buffer"
				//DBG_DUMP("[OX]%s.out[%d]-add-2-1 addr %x \r\n", p_thisunit->unit_name, oport,addr);
			}
		} else {
			//DBG_DUMP("[OX]%s.out[%d]-new-2-%d\r\n", p_thisunit->unit_name, oport,n+1);
			addr = p_thisunit->p_base->do_new(p_thisunit, oport, NVTMPP_VB_INVALID_POOL, ddr, new_buf_size, p_data, ISF_OUT_PROBE_NEW);
            if(addr==0) {
                DBG_ERR("%s.out[%d]-new-2-%d fail\r\n", p_thisunit->unit_name, oport,n+1);
            }
		}
	} else
#endif
	{
		if (p_ctx->combine_mode) {
			if (p_ctx->user_out_blk[i]) {
				p_data->h_data = p_ctx->user_out_blk[i];
				addr = nvtmpp_vb_blk2va(p_data->h_data);
				if (addr) {
					//add 1 reference to simulate original "do_new"
					if (nvtmpp_vb_get_block_refcount(p_data->h_data)) {
						if (p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW)) {
							addr = 0;
							//DBG_WRN("%s drop blk0x%X\r\n", p_thisunit->unit_name, p_ctx->user_out_blk[i]);
						}
					} else {
						addr = 0;
						//DBG_WRN("%s drop blk0x%X\r\n", p_thisunit->unit_name, p_ctx->user_out_blk[i]);
					}
				}
				//clean blk info
				p_ctx->user_out_blk[i] = 0;
			} else {
				//DBG_DUMP("%s.out[%d] wait new bgn blk\r\n", p_thisunit->unit_name, oport);
			}
		} else if (p_ctx->user_out_blk[i]) {
			//user specified output buffer
			p_data->h_data = p_ctx->user_out_blk[i];
			addr = nvtmpp_vb_blk2va(p_data->h_data);
			if (addr) {
				p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to simulate original "do_new"
			}
		} else {
			//OUT(D2D) + IN(D2D)
			addr = p_thisunit->p_base->do_new(p_thisunit, oport, NVTMPP_VB_INVALID_POOL, ddr, new_buf_size, p_data, ISF_OUT_PROBE_NEW);
		}

	}
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! New from Pool -- Vdo: size=%d ok => blk_id=%08x addr=%08x\r\n", p_thisunit->unit_name, oport, buf_size, p_data->h_data, addr);
	}
#endif

	if (addr == 0) {
#if (DUMP_DATA_ERROR == ENABLE)
		DBG_ERR("%s.out[%d]! New -- Vdo: size=%d faild!\r\n", p_thisunit->unit_name, oport, new_buf_size);
#endif
		#ifdef ISF_TS
		#ifdef ISF_DUMP_TS
		DBG_DUMP("vprc ... new fail!\r\n");
		#endif
		#endif
		//p_data->serial = 0;
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW_WRN, ISF_ERR_NEW_FAIL);
		_vdoprc_oport_releasedata(p_thisunit, oport, j);
		j = 0xff;
		return j;
	}
	#ifdef ISF_TS
	p_data->ts_vprc_out[0] = ts;
	#endif

	//DBG_DUMP("[OX]%s.out[%d] check n=%d, one=%d, 3dnr=%d, strip=%d, refh=%08x, oneh=%08x\r\n", p_thisunit->unit_name, oport, (int)n, (int)is_out_one, (int)is_3dnr_ref, (int)p_ctx->max_strp_num, (UINT32)p_ctx->outq.force_refbuffer, (UINT32)p_ctx->outq.force_onebuffer[i]);
#if (USE_OUT_ONEBUF == ENABLE)
	if (is_3dnr_ref && is_out_one && (n == 1) && (p_ctx->outq.force_refbuffer == 0) && (p_ctx->max_strp_num > 1)) { //while 2nd time new data
		DBG_WRN("%s.out[%d]! Start single blk mode-2 => blk_id=%08x addr=%08x\r\n", p_thisunit->unit_name, oport, p_data->h_data, addr);
		//DBG_DUMP("[OX]%s.out[%d]-save-ref-3ndr-h\r\n", p_thisunit->unit_name, oport);
		p_ctx->outq.force_refbuffer = p_data->h_data;
		p_ctx->outq.force_refsize = new_buf_size;
		p_ctx->outq.force_refj = j;
		p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to this "single buffer"
#if (USE_IN_DIRECT == ENABLE)
		if (is_in_dir) {
			//OUT(one buffer) + IN(direct mode) + 3dnr effect: start this path, IPP will new 3 data, then release 1st data
			p_ctx->outq.output_max[i] = 3; //extend output max to 3
		}
#endif
	}
	if (is_3dnr_ref && is_out_one && (n == 1) && (p_ctx->outq.force_refbuffer == 0) && (p_ctx->max_strp_num == 1)) { //while 2nd time new data
		//DBG_DUMP("[OX]%s.out[%d]-set-ref-3dnr-1\r\n", p_thisunit->unit_name, oport);
		p_ctx->outq.force_refbuffer = 1;  //mark 1 for do_stop
#if (USE_IN_DIRECT == ENABLE)
		if (is_in_dir) {
			//OUT(one buffer) + IN(direct mode) + 3dnr effect: start this path, IPP will new 3 data, then release 1st data
			p_ctx->outq.output_max[i] = 3; //extend output max to 3
		}
#endif
	}
	if (is_out_one && (n == 0) && (p_ctx->outq.force_onebuffer[i] == 0)) { //while 1st time new data
		DBG_WRN("%s.out[%d]! Start single blk mode => blk_id=%08x addr=%08x\r\n", p_thisunit->unit_name, oport, p_data->h_data, addr);
		//DBG_DUMP("[OX]%s.out[%d]-save-one\r\n", p_thisunit->unit_name, oport);
		p_ctx->outq.force_onebuffer[i] = p_data->h_data;
		p_ctx->outq.force_onesize[i] = buf_size;
		p_ctx->outq.force_onej[i] = j;
		p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to this "single buffer"
		//OUT(one buffer)
		p_ctx->outq.output_max[i] = 1; //limit output max to 1
#if (USE_IN_DIRECT == ENABLE)
		if (is_in_dir) {
			//OUT(one buffer) + IN(direct mode): start this path, IPP will new 2 data, then release 1st data
			p_ctx->outq.output_max[i] = 2; //extend output max to 2
		}
#endif
		if (is_3dnr_ref) {
			//OUT(one buffer) + 3dnr effect: start this path, IPP will new 2 data, then release 1st data
			p_ctx->outq.output_max[i] = 2; //extend output max to 2
		}
	}

 	if (is_out_two && (n == 0) && (p_ctx->outq.force_onebuffer[i] == 0)) { //while 1st time new data
		p_ctx->outq.force_onebuffer[i] = p_data->h_data;
		p_ctx->outq.force_onesize[i] = buf_size;
		p_ctx->outq.force_onej[i] = j;
		p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to this "1st buffer"
		//DBG_DUMP("[OX]%s.out[%d]-add-2-1 addr %x \r\n", p_thisunit->unit_name, oport,addr);

        if((is_in_dir)&&(is_3dnr_ref)) {
 	        p_ctx->outq.output_max[i] = 3;
        } else {
     	    p_ctx->outq.output_max[i] = 2;
        }
 	}

 	if (is_out_two && (n == 1) && (p_ctx->outq.force_twobuffer[i] == 0)) { //while 2nd time new data
		p_ctx->outq.force_twobuffer[i] = p_data->h_data;
		p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //add 1 reference to this "2nd buffer"
		//DBG_DUMP("[OX]%s.out[%d]-add-2-2 addr %x \r\n", p_thisunit->unit_name, oport,addr);

 	}
#endif

	loc_cpu(flags);
	{
		n++;
        //reset n
        if(is_out_two) {
            //two buf,n should multiple of 2 for pin-pong
            if (n >= (p_ctx->outq.output_max[i]*2)) { n = 0; }
        } else {
    		if (n >= p_ctx->outq.output_max[i]) { n = 0; }
        }
		p_ctx->outq.output_cnt[i] = n;
	}
	unl_cpu(flags);

	if (addr) {
		//p_data->serial = 0;
#if (FORCE_DUMP_DATA == ENABLE)
		//if (oport == WATCH_PORT) {
			//DBG_DUMP("%s.out[%d]! New -- Vdo: size=%d ok => blk_id=%08x addr=%08x blk[%d].cnt=%d\r\n", p_thisunit->unit_name, oport, buf_size, p_data->h_data, addr, j, p_ctx->outq.output_used[i][j]);
		//}
#endif
		j = (UINT32)(j | 0xffff0000);
		if (is_ref)
			j |= 0x00008000;
		//if (i == WATCH_PORT) {DBG_DUMP("^Cnew-e %08x\r\n",j); }
	}

	*p_addr = addr;

    //META#### copy meta to exten output frame
    if((is_copy_meta)&&(total_meta_size)) {

        UINT32 r=_isf_vdoprc_meta_copy(p_ipp_frame, p_inq_frame,addr+buf_size-total_meta_size,0,0);
        if(r!=0) {
            DBG_ERR("r=%d \r\n",r);
        }
    }
	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_NEW, ISF_OK);
	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC, ISF_ENTER);
	//DBG_DUMP("oport = %d, buf_size = 0x%x, j = %d, addr = 0x%x\r\n", oport, buf_size, j, addr);
	return j;
}

#if _TODO
static void _isf_vdoprc_oport_skiperror(ISF_UNIT *p_thisunit, UINT32 oport, ISF_PORT *p_port, ISF_DATA* p_data, UINT32 j, ISF_RV r)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	{
		p_thisunit->p_base->do_release(p_thisunit, oport, p_data, 0);
		p_thisunit->p_base->do_probe(p_thisunit, oport, 0, r);
		_vdoprc_oport_releasedata(p_thisunit, oport, j);
		loc_cpu(flags);
		p_ctx->outq.output_cnt[i]--; //remove count
		unl_cpu(flags);
		//if (oport == WATCH_PORT) {DBG_DUMP("X-5:");DBGD(p_ctx->outq.output_cnt[i]);}
	}
}
#endif

void _isf_vdoprc_oport_do_proc_begin(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_handle)
{
	#ifdef ISF_TS
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	UINT32 j = (buf_handle & ~0xffff8000);
	ISF_DATA *p_data;
	if (j >= VDOPRC_OUT_DEPTH_MAX) {
		return;
	}
	p_data = (ISF_DATA *)&(p_ctx->outq.output_data[i][j]);
	p_data->ts_vprc_out[1] = hwclock_get_longcounter();
	#endif
}


void _isf_vdoprc_oport_do_proc_end(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_handle)
{
	#ifdef ISF_TS
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	UINT32 j = (buf_handle & ~0xffff8000);
	ISF_DATA *p_data;
	if (j >= VDOPRC_OUT_DEPTH_MAX) {
		return;
	}
	p_data = (ISF_DATA *)&(p_ctx->outq.output_data[i][j]);
	p_data->ts_vprc_out[2] = hwclock_get_longcounter();
	#ifdef ISF_DUMP_TS
#if (USE_OUT_LOWLATENCY == ENABLE)
	if ((p_ctx->cur_out_cfg_func[i] & VDOPRC_OFUNC_LOWLATENCY)) {
		DBG_DUMP("vprc-new: %lld\r\n",
			p_data->ts_vprc_out[0]);
		DBG_DUMP("vprc-begin: %lld\r\n",
			p_data->ts_vprc_out[1]);
		DBG_DUMP("vprc-push: %lld\r\n",
			p_data->ts_vprc_out[3]);
		DBG_DUMP("vprc-end: %lld\r\n",
			p_data->ts_vprc_out[2]);
		DBG_DUMP("*** vprc-begin to vprc-end: %lld\r\n", p_data->ts_vprc_out[2]-p_data->ts_vprc_out[1]);
	}
#endif
	#endif
	#endif
}

void _isf_vdoprc_oport_do_push(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_handle, VDO_FRAME* p_srcvdoframe)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	UINT32 j = (buf_handle & ~0xffff8000);
	//BOOL is_ref = ((buf_handle & 0x00008000) != 0);
	ISF_PORT* p_port = p_thisunit->port_out[i];
	ISF_DATA *p_data;
	VDO_FRAME *p_vdoframe;
#if _TODO
	IPL_MODE_DATA *p_modedata = &g_modedata[id];
#endif
	UINT32 ctype;
	BOOL fmt_ok = FALSE;
	BOOL push_ok = FALSE;
	BOOL push_que = FALSE;
	BOOL push_ext = FALSE;
	BOOL allow_ext = TRUE;
#if 0
	unsigned long flags;
#endif
#if (USE_OUT_EXT == ENABLE)
	ISF_RV pr = ISF_OK; //push result
	BOOL enqueue_ok = FALSE;
#endif
	#ifdef ISF_TS
	UINT64 ts;
	#endif
#if (USE_OUT_DIS == ENABLE)
	//UINT32 push_dis = 0;;
	ISF_DATA *p_scaleup_data = 0;
	UINT32 scaleup_j = 0;
	UINT32 scaleup_count = 0;
#endif

	//DBG_ERR("time = %lld us oport = %d\r\n", hwclock_get_longcounter(), oport);
	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC, ISF_OK);
	#ifdef ISF_TS
	ts = hwclock_get_longcounter();
	#endif
	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH, ISF_ENTER);

	if (p_srcvdoframe == 0) {
		DBG_ERR("%s.out[%d]! Push -- Vdo: blk[%d] null frame???\r\n", p_thisunit->unit_name, oport, j);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_INVALID_DATA);
		p_ctx->outq.count_push_fail[i]++;
		return;
	}

	if (j >= VDOPRC_OUT_DEPTH_MAX) {
		DBG_ERR("%s.out[%d]! Push -- Vdo: blk[%d???] IGNORE!!!!!\r\n", p_thisunit->unit_name, oport, j);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_QUEUE_ERROR);
		p_ctx->outq.count_push_fail[i]++;
		return;
	}

	p_data = (ISF_DATA *)&(p_ctx->outq.output_data[i][j]);
	//p_data->func = j;
	#if 0
	if(!p_port) {
		// NOTE! under IPL direct mode, the last push maybe enter with NULL pPort.
		p_thisunit->p_base->do_release(p_thisunit, oport, p_data, 0);
		p_thisunit->p_base->do_probe(p_thisunit, oport, 0, ISF_OK);
		_vdoprc_oport_releasedata(p_thisunit, oport, j);
		loc_cpu(flags);
		p_ctx->outq.output_cnt[i]--; //remove count
		unl_cpu(flags);
		return;
	}
	#endif

	p_vdoframe = (VDO_FRAME*)(p_data->desc);



	#if 0
	static UINT64 xfps_t1, xfps_t2, xfps_diff;
	if (1) {
        xfps_t1 = HwClock_GetLongCounter(); //first time
	}
	#endif

#if (USE_OUT_ONEBUF == ENABLE)
	if (p_ctx->cur_out_cfg_func[i] & VDOPRC_OFUNC_ONEBUF) {
		//allow_ext = FALSE;
	}
#endif
	if (p_port) {
		ctype = p_port->connecttype;
		p_ctx->outq.output_connecttype[i] = ctype;
	} else {
		p_ctx->outq.output_connecttype[i] = ISF_CONNECT_NONE;
	}

#if 1
	if (p_ctx->outq.output_used[i][j] == 0) {
		DBG_ERR("%s.out[%d]! Push -- Vdo: blk_id=%08x blk[%d].cnt=%d is already release?\r\n", p_thisunit->unit_name, oport, p_data->h_data, j, p_ctx->outq.output_used[i][j]);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_QUEUE_ERROR);
		_vdoprc_oport_dumpqueue(p_thisunit);
		p_ctx->outq.count_push_fail[i]++;
		return;
	}
#endif
	//if (i == WATCH_PORT) {	DBG_DUMP("^Mpush-b %08x\r\n",j); }
#if (DUMP_FPS == ENABLE)
	if (i == WATCH_PORT) {
		DBG_FPS(p_thisunit, "out", i, "Push");
	}
#endif
	memcpy(p_vdoframe, p_srcvdoframe, sizeof(VDO_FRAME));
//DBG_DUMP("%s.out[%d]! Push -- Vdo: blk_id=%08x blk[%d]\r\n", p_thisunit->unit_name, oport, p_data->h_data, j);
	//user trigger flow
	if (p_ctx->user_crop_trig && ((p_ctx->ctrl.cur_func & VDOPRC_FUNC_3DNR) && (p_ctx->ctrl._3dnr_refpath == i))) {
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "user_crop_trig=%d", p_ctx->user_crop_trig);
		if (p_ctx->user_crop_trig == 1) {
			UINT32 p, apply_id;
			//update all output path
			for (p = 0; p < ISF_VDOPRC_PHY_OUT_NUM; p++) {
				if ((p_thisunit->port_outstate[p]->state) == (ISF_PORT_STATE_RUN)) {
					_vdoprc_update_out(p_thisunit, ISF_OUT(p), TRUE);
				}
			}
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_APPLY, (void *)&apply_id);
			p_ctx->user_crop_trig = 2;
		} else if (p_ctx->user_crop_trig == 2) {
			//user define3 callback to switch sensor
			SEN_USER3_CB fp = p_ctx->sen_user3_cb;

			if (fp) {
				ISF_PORT *p_iport = p_thisunit->port_in[0];
				ISF_UNIT *p_srcunit = 0;

				if (p_iport) {
					p_srcunit = p_iport->p_srcunit;
					if (p_srcunit) {
						(fp)(p_srcunit, p_ctx->user_crop_param);
					}
				}
			}
			p_ctx->user_crop_trig = 0;
		}
	}

	switch (VDO_PXLFMT_CLASS(p_vdoframe->pxlfmt)) {
	case VDO_PXLFMT_CLASS_YUV:
	case VDO_PXLFMT_CLASS_NVX:
		fmt_ok = TRUE;
		switch(p_vdoframe->pxlfmt) {
		case VDO_PXLFMT_YUV420:
		case VDO_PXLFMT_YUV422:
		case VDO_PXLFMT_YUV444:
		case VDO_PXLFMT_YUV420_PLANAR:
		case VDO_PXLFMT_YUV422_PLANAR:
		case VDO_PXLFMT_YUV444_PLANAR:
		case VDO_PXLFMT_YUV444_ONE:
		case VDO_PXLFMT_Y8:
			{
				int p;
				for (p = 0; p < VDO_MAX_PLANE; p++) {
					if ((p_vdoframe->addr[p] != 0) && (p_vdoframe->phyaddr[p] == 0)) {
						//come from vdoprc
						p_vdoframe->phyaddr[p] = nvtmpp_sys_va2pa(p_vdoframe->addr[p]);
					}
				}
			}
			break;
		case VDO_PXLFMT_YUV420_NVX1_H264:
		case VDO_PXLFMT_YUV420_NVX1_H265:
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		case VDO_PXLFMT_YUV420_NVX2:
#endif
			{
				int p;
				for (p = 0; p < VDO_MAX_PLANE; p++) {
					if ((p_vdoframe->addr[p] != 0) && (p_vdoframe->phyaddr[p] == 0)) {
						//come from vdoprc
						p_vdoframe->phyaddr[p] = nvtmpp_sys_va2pa(p_vdoframe->addr[p]);
					}
				}
			}
			break;
		default:
			break;
		}
		break;
	case VDO_PXLFMT_CLASS_ARGB:
		fmt_ok = TRUE;
		switch(p_vdoframe->pxlfmt) {
		case VDO_PXLFMT_RGB888_PLANAR:
			{
				int p;
				for (p = 0; p < VDO_MAX_PLANE; p++) {
					if ((p_vdoframe->addr[p] != 0) && (p_vdoframe->phyaddr[p] == 0)) {
						//come from vdoprc
						p_vdoframe->phyaddr[p] = nvtmpp_sys_va2pa(p_vdoframe->addr[p]);
					}
				}
			}
			break;
		default:
			break;
		}
		break;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	case 0xe:
		fmt_ok = TRUE;
		switch(p_vdoframe->pxlfmt) {
		case VDO_PXLFMT_520_IR8:
		case VDO_PXLFMT_520_IR16:
			{
				int p;
				for (p = 0; p < VDO_MAX_PLANE; p++) {
					if ((p_vdoframe->addr[p] != 0) && (p_vdoframe->phyaddr[p] == 0)) {
						//come from vdoprc
						p_vdoframe->phyaddr[p] = nvtmpp_sys_va2pa(p_vdoframe->addr[p]);
					}
				}
			}
			break;
		default:
			break;
		}
		break;
#endif
	default:
		break;
	}
	if(!fmt_ok) {
		DBG_WRN("%s.out[%d]! Out Data warn -- Vdo: blk_id=%08x pxlfmt=%08x?\r\n", p_thisunit->unit_name, i, p_data->h_data, p_vdoframe->pxlfmt);
		p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_DATA_NOT_SUPPORT);
		_vdoprc_oport_releasedata(p_thisunit, oport, j);
		_isf_vdoprc_push_dummy_queue_for_user_buf(p_thisunit, oport, p_data);
		return;
	}

	{
		//record current status
		p_vdoframe->reserved[4] = p_ctx->cur_out_cfg_func[i];
		p_vdoframe->reserved[5] = 0;
#if (USE_OUT_DIS == ENABLE)
		p_vdoframe->reserved[5] |= (p_ctx->outq.dis_scaleratio & 0xffff);
#endif
	}

#if (USE_VPE == ENABLE)
	if (p_ctx->vpe_mode) {
		if (p_ctx->user_out_blk[i] == 0) {
			//revise size and pw[] info for non-user_out_blk and specified output window
			if (p_ctx->out_win[i].imgaspect.w && p_ctx->out_crop_mode[i]) {
				p_vdoframe->size.w = (INT32)p_ctx->out[i].crp_window.w;
				p_vdoframe->size.h = (INT32)p_ctx->out[i].crp_window.h;
				p_vdoframe->pw[0] = p_vdoframe->size.w;
				p_vdoframe->pw[1] = p_vdoframe->pw[0] >> 1;
				p_vdoframe->pw[2] = p_vdoframe->pw[0] >> 1;
			}
			//DBG_DUMP("out[%d] %dx%d pw[0]=%d pw[1]=%d loff[0]=%d loff[1]=%d\r\n", oport,p_vdoframe->size.w, p_vdoframe->size.h, p_vdoframe->pw[0], p_vdoframe->pw[1], p_vdoframe->loff[0], p_vdoframe->loff[1]);
		}
	} else
#endif
	{
		_IPP_PM_CB_INPUT_INFO in;
		_IPP_PM_CB_OUTPUT_INFO out;
		_IPP_DS_CB_INPUT_INFO in2;
		_IPP_DS_CB_OUTPUT_INFO out2;
		in.ctl_ipp_handle = p_ctx->dev_handle;
		in.img_size = p_vdoframe->size;
		out.p_vdoframe = p_vdoframe;
		in2.ctl_ipp_handle = p_ctx->dev_handle;
		in2.img_size = p_vdoframe->size;
		out2.p_vdoframe = p_vdoframe;
		_isf_vdoprc_do_output_mask(p_thisunit, oport, &in, &out);
		_isf_vdoprc_do_output_osd(p_thisunit, oport, &in2, &out2);
	}

#if (FORCE_DUMP_DATA == ENABLE)
	//DBG_MSG("%s.out[%d]! Push -- Vdo: %dx%d %d %08x %08x %d %d\r\n", p_thisunit->unit_name, oport,
	//		w, h, fmt, c_addr[0], c_addr[1], c_loff[0], c_loff[1]);
#endif
	//p_data->src = (UINT32)oport;
	if ((p_ctx->dev_ready == 0) || (p_ctx->dev_handle == 0)) {
		p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
		_vdoprc_oport_releasedata(p_thisunit, oport, j);
		if (p_ctx->dev_trigger_open == 1) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_NOT_START);
		} if (p_ctx->dev_trigger_close == 1) {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_NOT_START);
		} else {
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_INACTIVE_STATE);
		}
		_isf_vdoprc_push_dummy_queue_for_user_buf(p_thisunit, oport, p_data);
		return;
	}

#if (USE_OUT_DIS == ENABLE)
	if (p_ctx->outq.dis_mode && p_ctx->outq.dis_en && (p_ctx->cur_out_cfg_func[i] & VDOPRC_OFUNC_DIS)
        && (p_ctx->outq.dis_start[i] & VDOPRC_OFUNC_DIS)) {

		/*
		BOOL check_err = 0;
		if (VDO_PXLFMT_CLASS(p_vdoframe->pxlfmt) == VDO_PXLFMT_CLASS_NVX) {
			if (p_ctx->err_cnt == 0) // this is a 1/60 error msg
				DBG_ERR("%s.out[%d]! NVX is not support DIS -- Vdo: blk_id=%08x pxlfmt=%08x?\r\n", p_thisunit->unit_name, i, p_data->h_data, p_vdoframe->pxlfmt);
			p_ctx->err_cnt++; if (p_ctx->err_cnt == 60) p_ctx->err_cnt = 0;
			check_err = 1;
		}

		if (check_err) {
			p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
			p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_DATA_NOT_SUPPORT);
			_vdoprc_oport_releasedata(p_thisunit, oport, j);
			_isf_vdoprc_push_dummy_queue_for_user_buf(p_thisunit, oport, p_data);
			return;
		}
		*/

		//save current frame (keep for next time)
		p_scaleup_data = p_data;
		scaleup_j = j;
		scaleup_count = p_vdoframe->count;

		//DBG_DUMP("\r\nvprc.out[%d] new blk[%d] => %08x\r\n", i, j, p_data->h_data);
		//pop dis track frame + do crop by dis MV => current frame
		_isf_vdoprc_oport_pop_dis_track(p_thisunit, oport, &p_data, &j);

		if (p_data == 0) {   //pop failed
 			//skip first time process!
 			//just save frame to dis track frame
			_isf_vdoprc_oport_push_dis_track(p_thisunit, oport, p_scaleup_data, scaleup_j, scaleup_count);
			return;
		}
	}
#endif

	//DBG_DUMP("nvprc.out[%d] get blk[%d] => %08x\r\n", i, j, p_data->h_data);
#if (USE_EIS == ENABLE)
	if (_vprc_eis_plugin_ && p_ctx->eis_func) {
		if (oport < MAX_EIS_PATH_NUM && lut2d_buf_info[oport].size) {
			ULONG lut2d_buf;
			ULONG lut2d_buf_pa;
			VDOPRC_EIS_2DLUT eis_2dlut = {0};
			ER ret;
			VDOPRC_EIS_PATH_INFO path_info = {0};

			//if (isf_vdoprc_check_frame_with_gyro(p_vdoframe->count)) {
			if (1) {
				path_info.path_id = oport;
				ret = _vprc_eis_plugin_->get(VDOPRC_EIS_PARAM_PATH_INFO, &path_info);
				if (ret) {
					DBG_WRN("%s.out[%d] get PATH_INFO error\r\n", p_thisunit->unit_name, oport);
					goto SKIP_EIS_TABLE;
				}
				_vdoprc_eis_put_frame(oport, p_data, j);

				ret = _vdoprc_eis_get_frame(oport, path_info.frame_latency + vprc_gyro_latency, &p_data, &j);
				if (ret) {
					DBG_DUMP("%s.out[%d] skip FRM(%llu)\r\n", p_thisunit->unit_name, oport, p_vdoframe->count);
					return;
				} else {
					//update p_vdoframe
					p_vdoframe = (VDO_FRAME*)(p_data->desc);
				}

				lut2d_buf_pa = nvtmpp_vb_blk2pa(p_data->h_data) + lut2d_buf_info[oport].offset;
				lut2d_buf = nvtmpp_vb_blk2va(p_data->h_data) + lut2d_buf_info[oport].offset;
				//eis_2dlut.frame_count = p_vdoframe->count + vprc_gyro_latency;
				eis_2dlut.frame_count = p_vdoframe->count;
				//check if the "future" frame with gyro in latency mode
				//if (vprc_gyro_latency) {
				if (1) {
					if (FALSE == isf_vdoprc_check_frame_with_gyro(eis_2dlut.frame_count)) {
						DBG_WRN("%s.out[%d] drop frm(%llu) latency %d\r\n", p_thisunit->unit_name, oport, p_vdoframe->count,path_info.frame_latency);
						goto RELEASE_NO_GYRO_FRM;
					}
				}
				eis_2dlut.path_id = oport;
				eis_2dlut.wait_ms = 100;
				eis_2dlut.buf_addr = lut2d_buf;
				eis_2dlut.buf_size = lut2d_buf_info[oport].size;
				p_vdoframe->reserved[3] = 0;
				//DBG_DUMP("%s.out[%d] FRM(%llu) lut2d_buf=0x%lX size=%d\r\n", p_thisunit->unit_name, oport, p_vdoframe->count, eis_2dlut.buf_addr, eis_2dlut.buf_size);
				ret = _vprc_eis_plugin_->get(VDOPRC_EIS_PARAM_2DLUT, &eis_2dlut);
				if (ret == E_OK) {
					switch(path_info.lut2d_size_sel) {
					case VDOPRC_LUT_9x9:
						p_vdoframe->reserved[2] = CTL_VPE_ISP_2DLUT_SZ_9X9;
						p_vdoframe->reserved[3] = lut2d_buf_pa;
						break;
					case VDOPRC_LUT_65x65:
						p_vdoframe->reserved[2] = CTL_VPE_ISP_2DLUT_SZ_65X65;
						p_vdoframe->reserved[3] = lut2d_buf_pa;
						break;
					default:
						DBG_WRN("%s.out[%d] size error(%d)\r\n", p_thisunit->unit_name, oport, path_info.lut2d_size_sel);
						break;
					}
					if (isf_vdoprc_eis_dbg_flag & VDOPRC_EIS_DBG_OUT_FRM) {
						DBG_DUMP("FRM(%llu) vd(%dms) lut_time(%dms) latency(%d+%d)\r\n", p_vdoframe->count, ((UINT32)p_vdoframe->timestamp)/1000, eis_2dlut.frame_time/1000, vprc_gyro_latency, path_info.frame_latency);
					}
					#if 0
					{
						UINT32 k, size, gyro_size;
						UINT32 check_sum = 0, check_sum2 = 0;
						UINT8 *p_tmp;
						UINT32 debug_size = vprc_gyro_data_num*6*sizeof(UINT32) + sizeof(EIS_DBG_CTX);

						gyro_size = 6*vprc_gyro_data_num*sizeof(UINT32);
						size = eis_2dlut.buf_size - debug_size;
						p_tmp = (UINT8 *)eis_2dlut.buf_addr;
						for (k = 0; k < size; k++) {
							check_sum += *(p_tmp+k);
						}
						p_tmp = (UINT8 *)(eis_2dlut.buf_addr + eis_2dlut.buf_size - gyro_size);
						for (k = 0; k < gyro_size; k++) {
							check_sum2 += *(p_tmp+k);
						}
						DBG_DUMP("#%llu 2D size=%d sum=0x%X, gyro size=%d sum=0x%X\r\n",eis_2dlut.frame_count, size, check_sum, gyro_size, check_sum2);

					}
					#endif

					#if 0//for debug only
					{
						INT32 *p_data;
						DBG_DUMP("%s.out[%d] FRM(%llu) lut2d_buf=0x%lX size=%d\r\n", p_thisunit->unit_name, oport, eis_2dlut.frame_count, eis_2dlut.buf_addr, eis_2dlut.buf_size);
						p_data = (INT32 *)eis_2dlut.buf_addr;
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
						p_data = (INT32 *)(eis_2dlut.buf_addr + 32);
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
						DBG_DUMP("Last x words\r\n");
						p_data = (INT32 *)(eis_2dlut.buf_addr + eis_2dlut.buf_size - 48*4);
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
						p_data = (INT32 *)(eis_2dlut.buf_addr + eis_2dlut.buf_size - 40*4);
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
						p_data = (INT32 *)(eis_2dlut.buf_addr + eis_2dlut.buf_size - 8*4);
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));

					}
					#endif
					#if 0//for gyro debug only
					{
						typedef struct
						{
							UINT64 frame_count;
							UINT64 t_diff_crop;
							UINT64 t_diff_crp_end_to_vd;
							UINT32 exposure_time;
							UINT32 gyro_data_num;
							UINT32 vendor_eis_ver;
							UINT32 eis_rsc_ver;
						} EIS_DBG_CTX;
						EIS_DBG_CTX *p_dbg_ctx;
						INT32 *p_data;
						UINT32 debug_size = vprc_gyro_data_num*6*sizeof(UINT32) + sizeof(EIS_DBG_CTX);

						DBG_DUMP("[%llu]data_num=%d\r\n", eis_2dlut.frame_count, vprc_gyro_data_num);

						p_dbg_ctx = (EIS_DBG_CTX *)(eis_2dlut.buf_addr + eis_2dlut.buf_size - debug_size);
						DBG_DUMP("%llu %llu %llu %u %u 0x%X 0x%X\r\n",
							p_dbg_ctx->frame_count, p_dbg_ctx->t_diff_crop, p_dbg_ctx->t_diff_crp_end_to_vd,
							p_dbg_ctx->exposure_time, p_dbg_ctx->gyro_data_num, p_dbg_ctx->vendor_eis_ver, p_dbg_ctx->eis_rsc_ver);

						p_data = (INT32 *)((ULONG)p_dbg_ctx + sizeof(EIS_DBG_CTX));
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X\r\n",
							*(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),	*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7),
							*(p_data+8), *(p_data+9), *(p_data+10), *(p_data+11),	*(p_data+12), *(p_data+13), *(p_data+14), *(p_data+15));
						#if 0
						p_data += vprc_gyro_data_num;
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X\r\n",
							*(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),	*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7),
							*(p_data+8), *(p_data+9), *(p_data+10), *(p_data+11),	*(p_data+12), *(p_data+13), *(p_data+14), *(p_data+15));
						p_data += vprc_gyro_data_num;
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X\r\n",
							*(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),	*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7),
							*(p_data+8), *(p_data+9), *(p_data+10), *(p_data+11),	*(p_data+12), *(p_data+13), *(p_data+14), *(p_data+15));
						#endif

					}
					#endif


				} else {
					DBG_WRN("%s.out[%d] get 2dlut frm(%llu) fail(%d).\r\n", p_thisunit->unit_name, oport, eis_2dlut.frame_count, ret);
					goto RELEASE_NO_GYRO_FRM;
				}
			} else {
				DBG_DUMP("%s in FRM(%llu) no gyro\r\n", p_thisunit->unit_name, p_vdoframe->count);
RELEASE_NO_GYRO_FRM:
				p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_INACTIVE_STATE);
				_vdoprc_oport_releasedata(p_thisunit, oport, j);
				return;
			}
		} else {
			//DBG_ERR("%s.out[%d] NOT support EIS\r\n", p_thisunit->unit_name, oport);
		}
	}
SKIP_EIS_TABLE:

#endif

	///////////////////////////////////////////
	// physical out - begin
#if (USE_OUT_EXT == ENABLE)
	{
		UINT32 ext_pid;

		for (ext_pid = ISF_VDOPRC_PHY_OUT_NUM; ext_pid < ISF_VDOPRC_OUT_NUM; ext_pid++) {
			ISF_PORT_PATH* p_path = p_thisunit->port_path + ext_pid;
			//XDATA
			if ((p_path->oport == oport) && (p_path->flag == 1) && !(p_ctx->phy_mask & (1<<ext_pid))) {
				push_ext = TRUE;

			}
		}
		if (push_ext) {
			p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
		}
	}
#endif
#if (USE_OUT_EXT == ENABLE)
	if (p_ctx->pullq.num[i]) {
		push_que = TRUE;
		p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
	}
#endif
	/////////////////////////////////
	// physical out - push to dest
	if ((p_port) && p_thisunit->p_base->get_is_allow_push(p_thisunit, p_port) && (!p_ctx->combine_mode)) {
		//if (oport == WATCH_PORT) {
		//	DBG_DUMP("%s.out[%d] t=%08x%08x\r\n", p_thisunit->unit_name, i, (UINT32)(p_data->timestamp >> 32), (UINT32)(p_data->timestamp));
		//}
#if (FORCE_DUMP_DATA == ENABLE)
		if (oport == WATCH_PORT) {
			DBG_DUMP("%s.out[%d]! Push -- Vdo: blk_id=%08x blk[%d].cnt=%d\r\n", p_thisunit->unit_name, oport, p_data->h_data, j, p_ctx->outq.output_used[i][j]);
		}
#endif
		p_thisunit->p_base->do_push(p_thisunit, oport, p_data, 0); //push to physical out
		push_ok = TRUE;
	}
	#ifdef ISF_TS
	p_data->ts_vprc_out[3] = ts;
	#ifdef ISF_DUMP_TS
#if (USE_OUT_LOWLATENCY == ENABLE)
	if (!(p_ctx->cur_out_cfg_func[i] & VDOPRC_OFUNC_LOWLATENCY)) {
		DBG_DUMP("vprc-new: %lld\r\n",
			p_data->ts_vprc_out[0]);
		DBG_DUMP("vprc-begin: %lld\r\n",
			p_data->ts_vprc_out[1]);
		DBG_DUMP("vprc-end: %lld\r\n",
			p_data->ts_vprc_out[2]);
		DBG_DUMP("vprc-push: %lld\r\n",
			p_data->ts_vprc_out[3]);
		DBG_DUMP("*** vprc-begin to vprc-end: %lld\r\n", p_data->ts_vprc_out[2]-p_data->ts_vprc_out[1]);
	}
#endif
	#endif
	#endif


#if (USE_OUT_EXT == ENABLE)
	/////////////////////////////////
	// physical out - push to oqueue
	if (push_que) {
		pr = _isf_vdoprc_oqueue_do_push_with_clean(p_thisunit, oport, p_data, 1); //keep this data
		if (pr == ISF_OK) {
			enqueue_ok = TRUE;
		}
		p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
	}
#endif

#if (USE_OUT_EXT == ENABLE)
	///////////////////////////////////////////
	// physical out - dispatch to related extend out
	if (push_ext) {
		UINT32 ext_pid;
		for (ext_pid = ISF_VDOPRC_PHY_OUT_NUM; ext_pid < ISF_VDOPRC_OUT_NUM; ext_pid++) {
			ISF_PORT_PATH* p_path = p_thisunit->port_path + ext_pid;
			//XDATA
			if ((p_path->oport == oport) && (p_path->flag == 1) && allow_ext && !(p_ctx->phy_mask & (1<<ext_pid))) {
				_isf_vdoprc_ext_tsk_trigger_proc(p_thisunit, ISF_OUT(ext_pid), oport-ISF_OUT_BASE, p_data);
			}
		}
		p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
	}
#endif

#if 1
	///////////////////////////////////////////
	// physical out - end
	if (push_ok) {
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH, ISF_OK);
		//output rate debug
		{
			UINT32 idx = p_ctx->outq.count_push_ok[i] % VDOPRC_DBG_TS_MAXNUM;
			p_ctx->out_dbg.t[i][idx] = hwclock_get_longcounter();
		}
		p_ctx->outq.count_push_ok[i]++;
		#if 0
		if (1) {
		    xfps_t2 =  HwClock_GetLongCounter();
		    xfps_diff = HwClock_DiffLongCounter(xfps_t1,xfps_t2);
			//DBG_DUMP("^C\"%s\".out[%d] Push! -- dt=%ld.%ld\r\n",
			//    p_thisunit->unit_name, i, GET_HI_UNIT32(xfps_diff), GET_LO_UNIT32(xfps_diff));
		}
		#endif
	} else {
		p_ctx->outq.count_push_fail[i]++;
		{
#if (FORCE_DUMP_DATA == ENABLE)
			if (oport == WATCH_PORT) {
				DBG_DUMP("%s.out[%d]! drop2 -- Vdo: blk_id=%08x blk[%08x].cnt=%d\r\n", p_thisunit->unit_name, oport, p_data->h_data, j, p_ctx->outq.output_used[i][j]);
			}
#endif
			if (p_srcvdoframe == 0) {
				DBG_ERR("%s.out[%d]! drop2 -- Vdo: blk_id=%08x blk[%08x].cnt=%d IGNORE!!!!!\r\n", p_thisunit->unit_name, oport, p_data->h_data, j, p_ctx->outq.output_used[i][j]);
				p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_INVALID_DATA);
				_isf_vdoprc_push_dummy_queue_for_user_buf(p_thisunit, oport, p_data);
			} else if (p_ctx->outq.output_used[i][j] == 0) {
				DBG_ERR("%s.out[%d]! drop2 -- Vdo: blk_id=%08x blk[%08x].cnt=%d? IGNORE!!!!!\r\n", p_thisunit->unit_name, oport, p_data->h_data, j, p_ctx->outq.output_used[i][j]);
				p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_QUEUE_ERROR);
				_isf_vdoprc_push_dummy_queue_for_user_buf(p_thisunit, oport, p_data);
			} else {
				p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
				if ((p_port) && p_thisunit->p_base->get_is_allow_push(p_thisunit, p_port)) {
					if (!push_ok) {
						p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_NOT_ALLOW);
					}
				} else {
					if (!enqueue_ok) {
						if (pr == ISF_ERR_QUEUE_FULL) {
							p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_DROP, pr);
						} else {
							p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PUSH_WRN, pr);
						}
						_isf_vdoprc_push_dummy_queue_for_user_buf(p_thisunit, oport, p_data);
					}
				}
			}
		}
	}
	//if (i == WATCH_PORT) {	DBG_DUMP("^Mpush-e %08x\r\n",j); }
	_vdoprc_oport_releasedata(p_thisunit, oport, j);

#if (USE_OUT_DIS == ENABLE)
	if (p_ctx->outq.dis_mode && p_ctx->outq.dis_en && (p_ctx->cur_out_cfg_func[i] & VDOPRC_OFUNC_DIS)) {
		if (p_scaleup_data == 0) {
			DBG_ERR("%s.out[%d]! DIS blk error?\r\n", p_thisunit->unit_name, i);
			return;
		}
		//push save frame to dis track frame
		_isf_vdoprc_oport_push_dis_track(p_thisunit, oport, p_scaleup_data, scaleup_j, scaleup_count);
	}
#endif

#endif
}

void _isf_vdoprc_oport_do_lock(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_handle)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	UINT32 j = (buf_handle & ~0xffff8000);
	ISF_DATA *p_data;
#if (FORCE_DUMP_DATA == ENABLE)
	UINT32 addr;
#endif
	if (j >= VDOPRC_OUT_DEPTH_MAX) {
		DBG_ERR("%s.out[%d]! Lock -- Vdo: blk[%d???] IGNORE!!!!!\r\n", p_thisunit->unit_name, oport, j);
		return;
	}
	p_data = (ISF_DATA *)&(p_ctx->outq.output_data[i][j]);
#if (FORCE_DUMP_DATA == ENABLE)
	addr = nvtmpp_vb_blk2va(p_data->h_data);
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Lock -- Vdo: blk_id=%08x addr=%08x blk[%d].cnt=%d\r\n", p_thisunit->unit_name, oport, p_data->h_data, addr, j, p_ctx->outq.output_used[i][j]);
	}
#endif
	{
		p_thisunit->p_base->do_add(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW);
		_vdoprc_oport_adddata(p_thisunit, oport, j);
	}
}

void _isf_vdoprc_oport_do_unlock(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_handle, INT32 err)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	UINT32 j = (buf_handle & ~0xffff8000);
	//BOOL is_ref = ((buf_handle & 0x00008000) != 0);
	ISF_DATA *p_data;
	UINT32 addr;

	//DBG_DUMP("oport = %d, j = %d\r\n", oport, j);
	if (j >= VDOPRC_OUT_DEPTH_MAX) {
		DBG_ERR("%s.out[%d]! Unlock -- Vdo: blk[0x%x???] IGNORE!!!!!\r\n", p_thisunit->unit_name, oport, j);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC_ERR, ISF_ERR_QUEUE_ERROR);
		return;
	}
	p_data = (ISF_DATA *)&(p_ctx->outq.output_data[i][j]);
	addr = nvtmpp_vb_blk2va(p_data->h_data);
#if (FORCE_DUMP_DATA == ENABLE)
	if (oport == WATCH_PORT) {
		DBG_DUMP("%s.out[%d]! Unlock -- Vdo: blk_id=%08x addr=%08x blk[%d].cnt=%d\r\n", p_thisunit->unit_name, oport, p_data->h_data, addr, j, p_ctx->outq.output_used[i][j]);
	}
#endif
	if (p_ctx->outq.output_used[i][j] == 0) {
		DBG_ERR("%s.out[%d]! Unlock -- Vdo: blk_id=%08x addr=%08x blk[%d].cnt=%d? IGNORE!!!!!\r\n", p_thisunit->unit_name, oport, p_data->h_data, addr, j, p_ctx->outq.output_used[i][j]);
		p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PROC);
		p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC_ERR, ISF_ERR_QUEUE_ERROR);
		_isf_vdoprc_push_dummy_queue_for_user_buf(p_thisunit, oport, p_data);
		return;
	}
	{
		p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PROC);
		_vdoprc_oport_releasedata(p_thisunit, oport, j);
#if (USE_VPE == ENABLE)
		if (p_ctx->vpe_mode) {
			_isf_vdoprc_push_dummy_queue_for_user_buf(p_thisunit, oport, p_data);
			switch(err) {
			case CTL_VPE_E_OK:
				//normal
				break;
			case CTL_VPE_E_PAR: 	///< illegal parameter
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC_WRN, ISF_ERR_PARAM_NOT_SUPPORT);
				break;
			case CTL_VPE_E_SYS:
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL);
				break;
			default:
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC_ERR, ISF_ERR_FATAL);
				break;
			}
		} else
#endif
		{
			switch(err) {
			case CTL_IPP_E_OK:
				//normal
				break;
			case CTL_IPP_E_FLUSH:
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC_DROP, ISF_ERR_FLUSH_DROP);
				break;
			case CTL_IPP_E_INDATA:
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC_WRN, ISF_ERR_DATA_NOT_SUPPORT);
				break;
			case CTL_IPP_E_PAR: 	///< illegal parameter
			case CTL_IPP_E_KDRV_SET:
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC_WRN, ISF_ERR_PARAM_NOT_SUPPORT);
				break;
			case CTL_IPP_E_SYS:
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL);
				break;
			case CTL_IPP_E_KDRV_STRP:
			case CTL_IPP_E_KDRV_TRIG:
				//UINT32 i = iport - ISF_IN_BASE;
				//ISF_PORT *p_src = p_thisunit->port_in[i];
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC_ERR, ISF_ERR_FATAL);
				break;
			default:
				p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_OUT_PROBE_PROC_ERR, ISF_ERR_FATAL);
				break;
			}
		}
	}
}

BOOL _vdoprc_oport_is_enable(ISF_UNIT *p_thisunit)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	return (BOOL)(p_ctx->outq.output_en);
}

void _vdoprc_oport_set_enable(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 en)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i = oport - ISF_OUT_BASE;
	ISF_PORT *p_dest = p_thisunit->port_out[i];
	if (en)
		_isf_vdoprc_oport_enable(p_thisunit, oport);
	else
		_isf_vdoprc_oport_disable(p_thisunit, oport);

	if (p_dest && en) {
#if (USE_OUT_ONEBUF == ENABLE)
		ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_dest_info);
		if (p_ctx->outq.force_onebuffer[i]) {
			DBG_WRN("--- %s.out[%d]! Under single blk mode, IGNORE max_buf=%d!\r\n", p_thisunit->unit_name, i, p_vdoinfo->buffercount);
			if (p_ctx->outq.output_cur_en[i] == TRUE) {
				p_ctx->outq.force_one_reset[i] = 1; //start after start => force reset!
			}
			return;
		}
#endif
		//p_ctx->outq.output_max[i] = p_vdoinfo->buffercount;
		if (p_ctx->outq.output_max[i] == 0) {
			p_ctx->outq.output_max[i] = VDOPRC_OUT_DEPTH_MAX;
		}
		p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_GENERAL, "--- set max_buf=%d", p_ctx->outq.output_max[i]);
		p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_GENERAL, "--- max_buf(%d)", p_ctx->outq.output_max[i]);
	} else {
		//do not change!
	}
}



