#include "isf_vdoprc_int.h"
//#include "nvtmpp.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoprc_i
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoprc_i_debug_level = NVT_DBG_WRN;
//module_param_named(isf_vdoprc_i_debug_level, isf_vdoprc_i_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_vdoprc_i_debug_level, "vdoprc1 debug level");

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)	vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)	vk_spin_unlock_irqrestore(&my_lock, flags)
///////////////////////////////////////////////////////////////////////////////

#define WATCH_PORT			ISF_IN(0)
#define FORCE_DUMP_DATA		DISABLE

#define SHOW_SHDR DISABLE
#define SHOW_SHDR_2 DISABLE

void _vdoprc_iport_setqueuecount(ISF_UNIT *p_thisunit, UINT32 count)
{
	ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
	p_vdoinfo->buffercount = count;
}

#define WATCH_IN_DATA	DISABLE

#if (WATCH_IN_DATA == ENABLE)
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

static void sample_hbp_handler_0(struct perf_event *bp,
           struct perf_sample_data *data,
           struct pt_regs *regs)
{
	printk(KERN_INFO "value[0] is changed\r\n");
	dump_stack();
	printk(KERN_INFO "Dump stack\r\n");
}
static void sample_hbp_handler_1(struct perf_event *bp,
           struct perf_sample_data *data,
           struct pt_regs *regs)
{
	printk(KERN_INFO "value[1] is changed\r\n");
	dump_stack();
	printk(KERN_INFO "Dump stack\r\n");
}
static void sample_hbp_handler_2(struct perf_event *bp,
           struct perf_sample_data *data,
           struct pt_regs *regs)
{
	printk(KERN_INFO "value[2] is changed\r\n");
	dump_stack();
	printk(KERN_INFO "Dump stack\r\n");
}
static void sample_hbp_handler_3(struct perf_event *bp,
           struct perf_sample_data *data,
           struct pt_regs *regs)
{
	printk(KERN_INFO "value[3] is changed\r\n");
	dump_stack();
	printk(KERN_INFO "Dump stack\r\n");
}

static struct perf_event_attr watch_attr[4] = {0};
static struct perf_event * __percpu *sample_hbp[4] = {0};

static void cpu_enable_watch(int id, unsigned int* p_data, unsigned int len)
{
	int ret;
	if(id >= 4) return;
	hw_breakpoint_init(&watch_attr[id]);
	watch_attr[id].bp_addr = (int)p_data;
	watch_attr[id].bp_len = HW_BREAKPOINT_LEN_4;
	watch_attr[id].bp_type = HW_BREAKPOINT_W;
	DBG_DUMP("\r\ncwp watch[%d] enable address=0x%x\r\n", id, (UINT32)(watch_attr[id].bp_addr));
	if(id == 0) sample_hbp[id] = register_wide_hw_breakpoint(&(watch_attr[id]), sample_hbp_handler_0, NULL);
	if(id == 1) sample_hbp[id] = register_wide_hw_breakpoint(&(watch_attr[id]), sample_hbp_handler_1, NULL);
	if(id == 2) sample_hbp[id] = register_wide_hw_breakpoint(&(watch_attr[id]), sample_hbp_handler_2, NULL);
	if(id == 3) sample_hbp[id] = register_wide_hw_breakpoint(&(watch_attr[id]), sample_hbp_handler_3, NULL);
	if (IS_ERR((void __force *)sample_hbp[id])) {
		ret = PTR_ERR((void __force *)sample_hbp[id]);
		DBG_ERR("\r\nregister_wide_hw_breakpoint fail\r\n");
		return;
	}
}
/*
static void cpu_disable_watch(int id)
{
	if(id >= 4) return;
	//DBG_DUMP("\r\ncwp watch[%d] disable\r\n", id);
	unregister_wide_hw_breakpoint(sample_hbp[id]);
}
*/
#endif

void _isf_vprc_set_ifunc_allow(ISF_UNIT *p_thisunit)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	switch (p_ctx->new_mode) {
	case VDOPRC_PIPE_RAWALL:
		p_ctx->ifunc_allow = VDOPRC_IFUNC_ONEBUF|VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_MIRRORX|VDOPRC_IFUNC_MIRRORY|VDOPRC_IFUNC_SCALEUP|VDOPRC_IFUNC_SCALEDN|VDOPRC_IFUNC_RAW|VDOPRC_IFUNC_NRX;
		#if defined(_BSP_NA51000_)
		p_ctx->ifunc_allow |= VDOPRC_IFUNC_RGB;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->ifunc_allow |= VDOPRC_IFUNC_DIRECT;
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->ifunc_allow |= VDOPRC_IFUNC_DIRECT;
		#endif
		break;
	case VDOPRC_PIPE_RAWCAP: //running 2 times of RAWALL to gen (1) sub out (2) ime out
		p_ctx->ifunc_allow = VDOPRC_IFUNC_ONEBUF|VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_MIRRORX|VDOPRC_IFUNC_MIRRORY|VDOPRC_IFUNC_SCALEUP|VDOPRC_IFUNC_SCALEDN|VDOPRC_IFUNC_RAW|VDOPRC_IFUNC_NRX;
		#if defined(_BSP_NA51000_)
		p_ctx->ifunc_allow |= VDOPRC_IFUNC_RGB;
		#endif
		#if defined(_BSP_NA51055_)
		#endif
		#if defined(_BSP_NA51089_)
		#endif
		break;

/*
	case VDOPRC_PIPE_RAWALL|VDOPRC_PIPE_WARP_360:
		break;
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case VDOPRC_PIPE_YUVALL:
		p_ctx->ifunc_allow = VDOPRC_IFUNC_ONEBUF|VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_MIRRORX|VDOPRC_IFUNC_MIRRORY|VDOPRC_IFUNC_SCALEUP|VDOPRC_IFUNC_SCALEDN|VDOPRC_IFUNC_YUV;
		#if defined(_BSP_NA51000_)
		p_ctx->ifunc_allow &= ~(VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_MIRRORX|VDOPRC_IFUNC_MIRRORY);
		p_ctx->ifunc_allow |= VDOPRC_IFUNC_RGB;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->ifunc_allow &= ~(VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_MIRRORX|VDOPRC_IFUNC_MIRRORY);
		p_ctx->ifunc_allow |= VDOPRC_IFUNC_EXTFMT; //allow input 0x0e
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->ifunc_allow &= ~(VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_MIRRORX|VDOPRC_IFUNC_MIRRORY);
		p_ctx->ifunc_allow |= VDOPRC_IFUNC_EXTFMT; //allow input 0x0e
		#endif
		break;
	case VDOPRC_PIPE_YUVCAP: //running 2 times of RAWALL to gen (1) sub out (2) ime out
		p_ctx->ifunc_allow = VDOPRC_IFUNC_ONEBUF|VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_MIRRORX|VDOPRC_IFUNC_MIRRORY|VDOPRC_IFUNC_SCALEUP|VDOPRC_IFUNC_SCALEDN|VDOPRC_IFUNC_YUV;
		#if defined(_BSP_NA51000_)
		p_ctx->ifunc_allow &= ~(VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_MIRRORX|VDOPRC_IFUNC_MIRRORY);
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->ifunc_allow &= ~(VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_MIRRORX|VDOPRC_IFUNC_MIRRORY);
		p_ctx->ifunc_allow |= VDOPRC_IFUNC_EXTFMT; //allow input 0x0e
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->ifunc_allow &= ~(VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_MIRRORX|VDOPRC_IFUNC_MIRRORY);
		p_ctx->ifunc_allow |= VDOPRC_IFUNC_EXTFMT; //allow input 0x0e
		#endif
		break;
	case VDOPRC_PIPE_SCALE:
		p_ctx->ifunc_allow = VDOPRC_IFUNC_ONEBUF|VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_MIRRORX|VDOPRC_IFUNC_MIRRORY|VDOPRC_IFUNC_SCALEUP|VDOPRC_IFUNC_SCALEDN|VDOPRC_IFUNC_YUV;
		#if defined(_BSP_NA51000_)
		p_ctx->ifunc_allow &= ~(VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_MIRRORX|VDOPRC_IFUNC_MIRRORY);
		p_ctx->ifunc_allow |= VDOPRC_IFUNC_RGB;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->ifunc_allow &= ~(VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_MIRRORX|VDOPRC_IFUNC_MIRRORY);
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->ifunc_allow &= ~(VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_MIRRORX|VDOPRC_IFUNC_MIRRORY);
		#endif
		break;
	case VDOPRC_PIPE_COLOR:
		#if defined(_BSP_NA51000_)
		p_ctx->ifunc_allow = VDOPRC_IFUNC_Y|VDOPRC_IFUNC_YUV;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->ifunc_allow = VDOPRC_IFUNC_Y|VDOPRC_IFUNC_YUV;
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->ifunc_allow = VDOPRC_IFUNC_Y|VDOPRC_IFUNC_YUV;
		#endif
		break;
	case VDOPRC_PIPE_WARP:
		#if defined(_BSP_NA51000_)
		p_ctx->ifunc_allow = VDOPRC_IFUNC_YUV;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->ifunc_allow = VDOPRC_IFUNC_YUV;
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->ifunc_allow = VDOPRC_IFUNC_YUV;
		#endif
		break;
#if (USE_VPE == ENABLE)
	case VDOPRC_PIPE_VPE:
		p_ctx->ifunc_allow = VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_YUV|VDOPRC_IFUNC_SCALEUP|VDOPRC_IFUNC_SCALEDN;
		break;
#endif
#if (USE_ISE == ENABLE)
	case VDOPRC_PIPE_ISE:
		p_ctx->ifunc_allow = VDOPRC_IFUNC_CROP|VDOPRC_IFUNC_YUV|VDOPRC_IFUNC_SCALEUP|VDOPRC_IFUNC_SCALEDN;
		break;
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	default:
		break;
	}
}

ISF_RV _isf_vprc_check_ifunc(ISF_UNIT *p_thisunit)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);

	{
		UINT32 func = p_ctx->ctrl.func_max;
		UINT32 ifunc = 0;
		UINT32 ifunc_not_allow = 0;

		switch (VDO_PXLFMT_CLASS(p_vdoinfo->max_imgfmt)) {
		case VDO_PXLFMT_CLASS_YUV: ifunc |= VDOPRC_IFUNC_YUV; break;
		case VDO_PXLFMT_CLASS_RAW: ifunc |= VDOPRC_IFUNC_RAW; break;
		case VDO_PXLFMT_CLASS_NRX: ifunc |= VDOPRC_IFUNC_NRX; break;
		case VDO_PXLFMT_CLASS_ARGB: ifunc |= VDOPRC_IFUNC_RGB; break;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		case 0xe: ifunc |= VDOPRC_IFUNC_EXTFMT; break;
#endif
		default: break;
		}

		ifunc_not_allow = ifunc;
		ifunc_not_allow &= ~(p_ctx->ifunc_allow);
		if ((ifunc_not_allow & VDOPRC_IFUNC_ALLFMT) != 0) {
			DBG_ERR("in[0].max_fmt=%08x is not support by pipe=%08x!\r\n", p_vdoinfo->max_imgfmt, p_ctx->new_mode);
			return ISF_ERR_FAILED;
		}

#if (USE_NEW_SHDR == ENABLE)
		// verify shdr limit
		p_ctx->ctrl.shdr_cnt = 0;
		if (func & VDOPRC_FUNC_SHDR) {
			int shdr_cnt = 0;
			switch (VDO_PXLFMT_CLASS(p_vdoinfo->max_imgfmt)) {
			case VDO_PXLFMT_CLASS_RAW:
			case VDO_PXLFMT_CLASS_NRX:
				shdr_cnt = VDO_PXLFMT_PLANE(p_vdoinfo->max_imgfmt);
				if ((shdr_cnt >= 2) && (shdr_cnt <= 4)) {
					//normal
				} else {
					DBG_ERR("in[0].max_fmt=%08x cannot support SHDR by pipe=%08x!\r\n", p_vdoinfo->max_imgfmt, p_ctx->new_mode);
					return ISF_ERR_FAILED;
				}
				break;
			default:
				DBG_ERR("in[0].max_fmt=%08x cannot support SHDR by pipe=%08x!\r\n", p_vdoinfo->max_imgfmt, p_ctx->new_mode);
				return ISF_ERR_FAILED;
				break;
			}
			p_ctx->ctrl.shdr_cnt = shdr_cnt;
			DBG_DUMP("in[0].max_fmt=%08x select %d-plane-SHDR by pipe=%08x!\r\n", p_vdoinfo->max_imgfmt, p_ctx->ctrl.shdr_cnt, p_ctx->new_mode);
		}
#endif
	}
	return ISF_OK;
}

// ImageAR = imgaspect of input port
// ImageAR.x = port->info.imgaspect.w
// ImageAR.y = port->info.imgaspect.h

/*
NOTE: vdoin/vdoprc's ISF_VDO_INFO.imgaspect default=0 = VDO_AR_USER
if ISF_VDO_INFO.imgaspect is modified, it means used auto mode

NOTE: vdoin/vdoprc's ISF_VDO_INFO.prewindow default=(0,0,0,0) is meaning not control, or full frame
if ISF_VDO_INFO.prewindow is modified, it refer to user's setting


under SIE auto mode, user should set zoom center(x, y), shift and w,h scale ratio, and scale factor,
	ISF_VDO_INFO.imgaspect = VDO_AR(zoom_ratio_x,zoom_ratio_y)
	ISF_VDO_INFO.prewindow = VDO_POSE(zoom_shift_x,zoom_shift_y), VDO_SIZE(zoom_scale_factor,1000)
    	(SIE will auto gen: sie crop window, ipl crop window)

under SIE user mode, user should set output crop window (if not set yet, means full frame)
	ISF_VDO_INFO.imgaspect = VDO_AR_USER
	ISF_VDO_INFO.prewindow = VDO_POSE(crop_x,crop_y), VDO_SIZE(crop_w,crop_h)

under IPL auto mode, SIE will also control IPL's crop window (through callback between them)
    	ISF_VDO_INFO.imgaspect = user is modified,
	ISF_VDO_INFO.prewindow = auto clear to(0,0,0,0), means not control

under IPL user mode, user should set input crop window (if not set yet, means full frame)
	ISF_VDO_INFO.imgaspect = VDO_AR_USER
	ISF_VDO_INFO.prewindow = VDO_POSE(crop_x,crop_y), VDO_SIZE(crop_w,crop_h)

*/
static URECT zero_crop = {0};

void _vdoprc_max_in(ISF_UNIT *p_thisunit, USIZE* p_size, VDO_PXLFMT* p_fmt)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);

	{
		p_ctx->in[0].max_size = p_vdoinfo->max_imgsize;
		p_ctx->in[0].max_pxlfmt = p_vdoinfo->max_imgfmt;
		//set input imgsize
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "max_size=(%d,%d), max_fmt=%08x", p_ctx->in[0].max_size.w, p_ctx->in[0].max_size.h, p_ctx->in[0].max_pxlfmt);
		if (p_size) {
			p_size[0] = p_ctx->in[0].max_size;
		}
		if (p_fmt) {
			p_fmt[0] = p_ctx->in[0].max_pxlfmt;
		}
	}
}

ISF_RV _vdoprc_config_in_crop(ISF_UNIT *p_thisunit, UINT32 iport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_INFO *p_src_info = p_thisunit->port_ininfo[iport - ISF_IN_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);

	if ((p_vdoinfo->imgaspect.w == 0) || (p_vdoinfo->imgaspect.h == 0)) {
		BOOL crop_en = TRUE;
		if ((p_vdoinfo->window.w == 0 && p_vdoinfo->window.h == 0)) {
			crop_en = FALSE;
		}
		if ( ((p_vdoinfo->window.x + p_vdoinfo->window.w) > p_ctx->in[0].max_size.w)
		|| ((p_vdoinfo->window.y + p_vdoinfo->window.h) > p_ctx->in[0].max_size.h) ) {
			DBG_ERR("-in:crop win=(%d,%d,%d,%d) is out of dim(%d,%d)?\r\n",
				p_vdoinfo->window.x, p_vdoinfo->window.y, p_vdoinfo->window.w, p_vdoinfo->window.h,
				p_ctx->in[0].max_size.w, p_ctx->in[0].max_size.h);
			crop_en = FALSE;
		}
		if (crop_en) {
			if (!(p_ctx->ifunc_allow & VDOPRC_IFUNC_CROP)) {
				DBG_ERR("%s.in[0]! cannot enable crop with pipe=%08x\r\n",
					p_thisunit->unit_name, p_ctx->ctrl.pipe);
				return ISF_ERR_FAILED;
			}
			//input crop is assign by user
			p_ctx->in[0].crop.mode = CTL_IPP_IN_CROP_USER;
			p_ctx->in[0].crop.crp_window.x = p_vdoinfo->window.x;
			p_ctx->in[0].crop.crp_window.y = p_vdoinfo->window.y;
			p_ctx->in[0].crop.crp_window.w = p_vdoinfo->window.w;
			p_ctx->in[0].crop.crp_window.h = p_vdoinfo->window.h;
			p_thisunit->p_base->do_trace(p_thisunit, iport, ISF_OP_PARAM_VDOFRM, "-in:crop win=(%d,%d,%d,%d)",
				p_vdoinfo->window.x,
				p_vdoinfo->window.y,
				p_vdoinfo->window.w,
				p_vdoinfo->window.h);
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_IN_CROP, (void *)&p_ctx->in[0].crop);
		} else {
			//input crop is off == full frame
			p_ctx->in[0].crop.mode = CTL_IPP_IN_CROP_NONE;
			p_ctx->in[0].crop.crp_window = zero_crop;
			p_thisunit->p_base->do_trace(p_thisunit, iport, ISF_OP_PARAM_VDOFRM, "-in:crop win=(0,0,full,full)");
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_IN_CROP, (void *)&p_ctx->in[0].crop);
		}
	} else {
		if (!(p_ctx->ifunc_allow & VDOPRC_IFUNC_CROP)) {
			DBG_ERR("%s.in[0]! cannot enable crop with pipe=%08x\r\n",
				p_thisunit->unit_name, p_ctx->ctrl.pipe);
			return ISF_ERR_FAILED;
		}
		//input crop is controlled by SIE's DZOOM
		p_ctx->in[0].crop.mode = CTL_IPP_IN_CROP_AUTO;
		p_ctx->in[0].crop.crp_window = zero_crop;
		ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_IN_CROP, (void *)&p_ctx->in[0].crop);
		p_thisunit->p_base->do_trace(p_thisunit, iport, ISF_OP_PARAM_VDOFRM, "-in:crop win=(-,-,-,-)");
	}
	return ISF_OK;
}

ISF_RV _vdoprc_update_in_crop(ISF_UNIT *p_thisunit, UINT32 iport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_INFO *p_src_info = p_thisunit->port_ininfo[iport - ISF_IN_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);

	if ((p_vdoinfo->imgaspect.w == 0) || (p_vdoinfo->imgaspect.h == 0)) {
		BOOL crop_en = TRUE;
		if ((p_vdoinfo->window.w == 0 && p_vdoinfo->window.h == 0)) {
			crop_en = FALSE;
		}
		if ( ((p_vdoinfo->window.x + p_vdoinfo->window.w) > p_ctx->in[0].max_size.w)
		|| ((p_vdoinfo->window.y + p_vdoinfo->window.h) > p_ctx->in[0].max_size.h) ) {
			DBG_ERR("-in:crop win=(%d,%d,%d,%d) is out of dim(%d,%d)?\r\n",
				p_vdoinfo->window.x, p_vdoinfo->window.y, p_vdoinfo->window.w, p_vdoinfo->window.h,
				p_ctx->in[0].max_size.w, p_ctx->in[0].max_size.h);
			crop_en = FALSE;
		}
		if (crop_en) {
			if (!(p_ctx->ifunc_allow & VDOPRC_IFUNC_CROP)) {
				DBG_ERR("%s.in[0]! cannot enable crop with pipe=%08x\r\n",
					p_thisunit->unit_name, p_ctx->ctrl.pipe);
				return ISF_ERR_FAILED;
			}
			//input crop is assign by user
			p_ctx->in[0].crop.mode = CTL_IPP_IN_CROP_USER;
			p_ctx->in[0].crop.crp_window.x = p_vdoinfo->window.x;
			p_ctx->in[0].crop.crp_window.y = p_vdoinfo->window.y;
			p_ctx->in[0].crop.crp_window.w = p_vdoinfo->window.w;
			p_ctx->in[0].crop.crp_window.h = p_vdoinfo->window.h;
			p_thisunit->p_base->do_trace(p_thisunit, iport, ISF_OP_PARAM_VDOFRM, "-in:crop win=(%d,%d,%d,%d)",
				p_vdoinfo->window.x,
				p_vdoinfo->window.y,
				p_vdoinfo->window.w,
				p_vdoinfo->window.h);
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_IN_CROP, (void *)&p_ctx->in[0].crop);
		} else {
			//input crop is off == full frame
			p_ctx->in[0].crop.mode = CTL_IPP_IN_CROP_NONE;
			p_ctx->in[0].crop.crp_window = zero_crop;
			p_thisunit->p_base->do_trace(p_thisunit, iport, ISF_OP_PARAM_VDOFRM, "-in:crop win=(0,0,full,full)");
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_IN_CROP, (void *)&p_ctx->in[0].crop);
		}
	} else {
		if (!(p_ctx->ifunc_allow & VDOPRC_IFUNC_CROP)) {
			DBG_ERR("%s.in[0]! cannot enable crop with pipe=%08x\r\n",
				p_thisunit->unit_name, p_ctx->ctrl.pipe);
			return ISF_ERR_FAILED;
		}
		//input crop is controlled by SIE's DZOOM
		p_ctx->in[0].crop.mode = CTL_IPP_IN_CROP_AUTO;
		p_ctx->in[0].crop.crp_window = zero_crop;
		ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_IN_CROP, (void *)&p_ctx->in[0].crop);
		p_thisunit->p_base->do_trace(p_thisunit, iport, ISF_OP_PARAM_VDOFRM, "-in:crop win=(-,-,-,-)");
	}
	return ISF_OK;
}

ISF_RV _vdoprc_config_in_direct(ISF_UNIT *p_thisunit, UINT32 iport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_INFO *p_src_info = p_thisunit->port_ininfo[iport - ISF_IN_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);

	if ((p_vdoinfo->direct & VDO_DIR_MIRRORX) && !(p_ctx->ifunc_allow & VDOPRC_IFUNC_MIRRORX)) {
		DBG_ERR("%s.in[0]! cannot enable mirror-x with pipe=%08x\r\n",
			p_thisunit->unit_name, p_ctx->ctrl.pipe);
		return ISF_ERR_FAILED;
	}
	if ((p_vdoinfo->direct & VDO_DIR_MIRRORY) && !(p_ctx->ifunc_allow & VDOPRC_IFUNC_MIRRORY)) {
		DBG_ERR("%s.in[0]! cannot enable mirror-y with pipe=%08x\r\n",
			p_thisunit->unit_name, p_ctx->ctrl.pipe);
		return ISF_ERR_FAILED;
	}
	if ((p_vdoinfo->direct & (VDO_DIR_ROTATE_90|VDO_DIR_ROTATE_270)) && !(p_ctx->ifunc_allow & VDOPRC_IFUNC_ROTATE)) {
		DBG_ERR("%s.in[0]! cannot enable rotate with pipe=%08x\r\n",
			p_thisunit->unit_name, p_ctx->ctrl.pipe);
		return ISF_ERR_FAILED;
	}

	if (p_vdoinfo->direct == VDO_DIR_NONE) {
		p_ctx->in[0].dir = CTL_IPP_FLIP_NONE;
	} else if (p_vdoinfo->direct == VDO_DIR_MIRRORX) {
		p_ctx->in[0].dir = CTL_IPP_FLIP_H;
	} else if (p_vdoinfo->direct == VDO_DIR_MIRRORY) {
		p_ctx->in[0].dir = CTL_IPP_FLIP_V;
	} else if (p_vdoinfo->direct == (VDO_DIR_MIRRORX|VDO_DIR_MIRRORY)) {
		p_ctx->in[0].dir = CTL_IPP_FLIP_H_V;
	}
	ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_FLIP, (void *)&p_ctx->in[0].dir);
	p_thisunit->p_base->do_trace(p_thisunit, iport, ISF_OP_PARAM_VDOFRM, "dir=(%08x)", p_ctx->in[0].dir);
	return ISF_OK;
}

ISF_RV _vdoprc_update_in_direct(ISF_UNIT *p_thisunit, UINT32 iport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_INFO *p_src_info = p_thisunit->port_ininfo[iport - ISF_IN_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);

	if ((p_vdoinfo->direct & VDO_DIR_MIRRORX) && !(p_ctx->ifunc_allow & VDOPRC_IFUNC_MIRRORX)) {
		DBG_ERR("%s.in[0]! cannot enable mirror-x with pipe=%08x\r\n",
			p_thisunit->unit_name, p_ctx->ctrl.pipe);
		return ISF_ERR_FAILED;
	}
	if ((p_vdoinfo->direct & VDO_DIR_MIRRORY) && !(p_ctx->ifunc_allow & VDOPRC_IFUNC_MIRRORY)) {
		DBG_ERR("%s.in[0]! cannot enable mirror-y with pipe=%08x\r\n",
			p_thisunit->unit_name, p_ctx->ctrl.pipe);
		return ISF_ERR_FAILED;
	}
	if ((p_vdoinfo->direct & (VDO_DIR_ROTATE_90|VDO_DIR_ROTATE_270)) && !(p_ctx->ifunc_allow & VDOPRC_IFUNC_ROTATE)) {
		DBG_ERR("%s.in[0]! cannot enable rotate with pipe=%08x\r\n",
			p_thisunit->unit_name, p_ctx->ctrl.pipe);
		return ISF_ERR_FAILED;
	}

	if (p_vdoinfo->direct == VDO_DIR_NONE) {
		p_ctx->in[0].dir = CTL_IPP_FLIP_NONE;
	} else if (p_vdoinfo->direct == VDO_DIR_MIRRORX) {
		p_ctx->in[0].dir = CTL_IPP_FLIP_H;
	} else if (p_vdoinfo->direct == VDO_DIR_MIRRORY) {
		p_ctx->in[0].dir = CTL_IPP_FLIP_V;
	} else if (p_vdoinfo->direct == (VDO_DIR_MIRRORX|VDO_DIR_MIRRORY)) {
		p_ctx->in[0].dir = CTL_IPP_FLIP_H_V;
	}
	ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_FLIP, (void *)&p_ctx->in[0].dir);
	p_thisunit->p_base->do_trace(p_thisunit, iport, ISF_OP_PARAM_VDOFRM, "dir=(%08x)", p_ctx->in[0].dir);
	return ISF_OK;
}

ISF_RV _isf_vdoprc_iport_alloc(ISF_UNIT *p_thisunit)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_RV           r          = ISF_OK;
	UINT32           bufaddr  = 0;
	UINT32           bufsize  = 0;
	UINT32 i = ISF_IN(0) - ISF_IN_BASE;
	ISF_PORT *p_src = p_thisunit->port_out[i];
	ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
	unsigned long flags;

	while (p_ctx->dev_trigger_close == 1) {
		DBG_WRN("%s.in[0] waiting for last close...\r\n", p_thisunit->unit_name);
		MSLEEP(1);
	}

    if (p_ctx->inq.input_data != 0) {
        //already alloc
        return ISF_OK;
    }

	//reset
	p_ctx->inq.input_max = 0;

	p_thisunit->p_base->do_trace(p_thisunit, ISF_IN(0), ISF_OP_PARAM_GENERAL, "user queue count=%d", (int)p_vdoinfo->buffercount);

	//load user setting from p_vdoinfo->buffercount
	if ((p_vdoinfo->buffercount > 0) && (p_vdoinfo->buffercount <= VDOPRC_IN_DEPTH_MAX)) {
		p_ctx->inq.input_max = p_vdoinfo->buffercount;
	} else {
		if (p_vdoinfo->buffercount > VDOPRC_IN_DEPTH_MAX) {
			DBG_ERR("%s.in[0] in_depth = %d > 15!\r\n", p_thisunit->unit_name, (int)p_vdoinfo->buffercount);
		}
	}

	if (p_ctx->inq.input_max == 0) {
		p_ctx->inq.input_max = VDOPRC_IN_DEPTH_DEF;
	}
#if (USE_IN_ONEBUF == ENABLE)
	if (p_ctx->new_in_cfg_func & VDOPRC_IFUNC_ONEBUF) { //under "single buffer mode"
		p_ctx->inq.input_max = 1;
	}
#endif
#if (USE_NEW_SHDR == ENABLE)
	if (p_ctx->ctrl.shdr_cnt > 0) {
		p_ctx->inq.input_max = (p_ctx->inq.input_max << 2);
	}
#endif
    	p_thisunit->p_base->do_trace(p_thisunit, ISF_IN(0), ISF_OP_PARAM_GENERAL, "queue count=%d", (int)p_ctx->inq.input_max);

#if (WATCH_IN_DATA == ENABLE)

	// create raw queue
	bufsize = sizeof(ISF_DATA) * p_ctx->inq.input_max + 0x1000;
	bufaddr = p_thisunit->p_base->do_new_i(p_thisunit, p_src, "in_list", bufsize, &(p_ctx->inq.input_pool));
	if (bufaddr == 0) {
		DBG_ERR("%s.in[0] get input queue blk failed!\r\n", p_thisunit->unit_name);
		return ISF_ERR_BUFFER_CREATE;
	}
	loc_cpu(flags);
	p_ctx->inq.input_data = (ISF_DATA *) (bufaddr + 0x1000);
	p_ctx->inq.input_cnt = 0;
	memset((char*)(bufaddr), 0xcc, 0x1000);
	unl_cpu(flags);

	memset(&p_ctx->inq.input_used, 0, sizeof(UINT32)*VDOPRC_IN_DEPTH_MAX);

	cpu_enable_watch(0, (unsigned int*)(bufaddr), 4);
#else

	// create raw queue
	bufsize = sizeof(ISF_DATA) * p_ctx->inq.input_max;
	bufaddr = p_thisunit->p_base->do_new_i(p_thisunit, p_src, "in_list", bufsize, &(p_ctx->inq.input_pool));
	p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_GENERAL, "in_list: new, size=%u, addr=%08x", bufsize, bufaddr);
	if (bufaddr == 0) {
		DBG_ERR("%s.in[0] get input queue blk failed!\r\n", p_thisunit->unit_name);
		return ISF_ERR_BUFFER_CREATE;
	}
	loc_cpu(flags);
	p_ctx->inq.input_data = (ISF_DATA *)bufaddr;
	p_ctx->inq.input_cnt = 0;
	memset(&p_ctx->inq.input_used, 0, sizeof(UINT32)*VDOPRC_IN_DEPTH_MAX);
	unl_cpu(flags);
#endif
	return r;
}

static void _vdoprc_iport_cancelalldata(ISF_UNIT *p_thisunit, UINT32 iport, UINT32 probe, UINT32 r);
static BOOL _vdoprc_iport_check_all_queue_freed(ISF_UNIT *p_thisunit)
{
	UINT32 j;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	BOOL queue_freed = TRUE;

	for (j = 0; j < p_ctx->inq.input_max; j++) {
		if (p_ctx->inq.input_used[j]) {
			queue_freed = FALSE;
			break;
		}
	}
	return queue_freed;
}
static void _vdoprc_iport_dumpqueue(ISF_UNIT *p_thisunit, BOOL force_release)
{
	UINT32 j;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

	DBG_DUMP("@@@@@Check Queue@@@@@\r\n");

	DBGD(p_ctx->inq.input_cnt);
	for (j = 0; j < p_ctx->inq.input_max; j++) {
		ISF_DATA *p_data = (ISF_DATA *)&(p_ctx->inq.input_data[j]);
		UINT32 addr = 0;
		if (p_data->h_data != 0)
			addr = nvtmpp_vb_blk2va(p_data->h_data);
		DBG_DUMP("%s.in[0] - Vdo: blk_id=%08x addr=%08x blk[%d].cnt=%d, func=%d\r\n", p_thisunit->unit_name, p_data->h_data, addr, j, p_ctx->inq.input_used[j],p_data->func);
		if (force_release && p_ctx->inq.input_used[j]) {
			DBG_DUMP("####force release blk[%d]####\r\n", j);
			_vdoprc_iport_dropdata(p_thisunit, ISF_IN(0), j, ISF_IN_PROBE_PROC_ERR, ISF_ERR_WAIT_TIMEOUT);
		}

	}
	DBG_DUMP("Dump Common blk\r\n");
	nvtmpp_dump_status(debug_msg);
}
ISF_RV _isf_vdoprc_iport_free(ISF_UNIT *p_thisunit)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_RV r = ISF_OK;
	unsigned long flags;

	// release raw queue
	p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_GENERAL, "in_list: release");
	if (p_ctx->inq.input_cnt > 0) {
#if (USE_NEW_SHDR == ENABLE)
		_vdoprc_iport_cancelalldata(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_OPEN_YET);
#endif
	}
	if (p_ctx->inq.input_cnt > 0) {
		UINT32 retry_count = 0;
		UINT32 timer_resolution = 10;//ms

		DBG_WRN("%s.in[0] waiting for release input queue...\r\n", p_thisunit->unit_name);
		while(p_ctx->inq.input_cnt > 0) {
			MSLEEP(timer_resolution); //wait for next 10 ms
			retry_count++;
			if (retry_count > 3000/timer_resolution) {
				DBG_ERR("%s.in[0] waiting for release input queue TIMEOUT!!!\r\n", p_thisunit->unit_name);
#if (USE_VPE == ENABLE)
				if (p_ctx->vpe_mode) {
					ctl_vpe_dump_all(NULL);
				}
#endif
#if (USE_ISE == ENABLE)
				if (p_ctx->ise_mode) {
					ctl_ise_dump_all(NULL);
				}
#endif
				nvtmpp_dump_status(debug_msg);
				_vdoprc_iport_dumpqueue(p_thisunit, TRUE);
				if (_vdoprc_iport_check_all_queue_freed(p_thisunit)) {
					break;
				}
				DBG_ERR("Wait input_cnt(%d) zero\r\n", p_ctx->inq.input_cnt);
				 while(p_ctx->inq.input_cnt > 0) {
					//for debug root cause
					MSLEEP(1);
				}
			}
		}
		DBG_WRN("release ok\r\n");
	}
	loc_cpu(flags);
	p_ctx->inq.input_data = 0;
	p_ctx->inq.input_cnt = 0;
	unl_cpu(flags);
	r = p_thisunit->p_base->do_release_i(p_thisunit, &(p_ctx->inq.input_pool), ISF_OK);
	if (r != ISF_OK) {
		DBG_ERR("%s.in[0] release input queue blk failed, ret = %d\r\n", p_thisunit->unit_name, r);
		return ISF_ERR_BUFFER_DESTROY;
	}
	return r;
}

static UINT32 _vdoprc_iport_newdata(ISF_UNIT *p_thisunit, UINT32 iport, ISF_DATA* p_data)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 j = 0xff;
	unsigned long flags;
#if (USE_NEW_SHDR == ENABLE)
	// search for input queue empty
	if (p_ctx->ctrl.shdr_cnt > 0) {
		UINT32 shdr_j;
#if (SHOW_SHDR == ENABLE)
		DBG_DUMP("new shdr_j=%d, shdr_i=%d\r\n", p_ctx->ctrl.shdr_j, p_ctx->ctrl.shdr_i);
#endif
		if (p_ctx->ctrl.shdr_i == 0) {
			for (shdr_j = 0; shdr_j < p_ctx->inq.input_max; shdr_j+=4) {
				j = shdr_j+p_ctx->ctrl.shdr_i;
				loc_cpu(flags);
				if (p_ctx->inq.input_data == 0) {
					unl_cpu(flags);
					j = 0xfe; //closed
					return j;
				} else
				if (p_ctx->inq.input_used[(shdr_j >> 2)] == 0) {
					memcpy(&p_ctx->inq.input_data[j], p_data, sizeof(ISF_DATA));
					(&p_ctx->inq.input_data[j])->func = 0;
					p_ctx->inq.input_used[(shdr_j >> 2)] = (1<<(p_ctx->ctrl.shdr_i));
					p_ctx->inq.input_cnt += 1;
					unl_cpu(flags);
					//cpu_watchData(id, &(p_ctx->inq.input_data[i]));
					p_ctx->ctrl.shdr_j = j;
#if (SHOW_SHDR == ENABLE)
		DBG_DUMP("ok=> (shdr_j)=%d, shdr_i=%d, j=%d\r\n", shdr_j, p_ctx->ctrl.shdr_i, j);
#endif
					break;
				} else {
					unl_cpu(flags);
#if (SHOW_SHDR == ENABLE)
		DBG_DUMP("fail => (shdr_j)=%d, shdr_i=%d, j=%d\r\n", shdr_j, p_ctx->ctrl.shdr_i, j);
#endif
				}
			}
			if (shdr_j > (p_ctx->inq.input_max-1)) {
				j = 0xff; //not found
			}
		} else {
#if (SHOW_SHDR == ENABLE)
		DBG_DUMP("use (shdr_j)=%d, shdr_i=%d\r\n", shdr_j, p_ctx->ctrl.shdr_i);
#endif
			shdr_j = p_ctx->ctrl.shdr_j;
			j = shdr_j+p_ctx->ctrl.shdr_i;
			loc_cpu(flags);
			if (p_ctx->inq.input_data == 0) {
				unl_cpu(flags);
				j = 0xfe; //closed
				return j;
			} else
			{
				memcpy(&p_ctx->inq.input_data[j], p_data, sizeof(ISF_DATA));
				(&p_ctx->inq.input_data[j])->func = 0;
				p_ctx->inq.input_used[(shdr_j >> 2)] |= (1<<(p_ctx->ctrl.shdr_i));
				p_ctx->inq.input_cnt += 1;
				unl_cpu(flags);
			}
		}
	} else
#endif
	{
		for (j = 0; j < p_ctx->inq.input_max; j++) {
			loc_cpu(flags);
			if (p_ctx->inq.input_data == 0) {
				unl_cpu(flags);
				j = 0xfe; //closed
				return j;
			} else
			if (!p_ctx->inq.input_used[j]) {
				memcpy(&p_ctx->inq.input_data[j], p_data, sizeof(ISF_DATA));
				(&p_ctx->inq.input_data[j])->func = 0;
				p_ctx->inq.input_used[j] = TRUE;
				p_ctx->inq.input_cnt ++;
				unl_cpu(flags);
				//cpu_watchData(id, &(p_ctx->inq.input_data[i]));
				break;
			} else {
				unl_cpu(flags);
			}
		}
		if (j > (p_ctx->inq.input_max-1)) {
			j = 0xff; //not found
		}
	}
	return j;
}

//release after process
void _vdoprc_iport_releasedata(ISF_UNIT *p_thisunit, UINT32 iport, UINT32 buf_handle, UINT32 probe, UINT32 r)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 j = buf_handle;
	ISF_DATA c_data;
	ISF_DATA* p_input_data    = 0;
	unsigned long flags;

#if 0
#if (FORCE_DUMP_DATA == ENABLE)
	if ((p_job->PathID+ISF_IN_BASE) == WATCH_PORT) {
		DBG_DUMP("^MVdoEnc.in[%d]! Finish -- vdo: blk_id=%08x addr=%08x\r\n", p_job->PathID, p_input_data->h_data, nvtmpp_vb_blk2va(p_input_data->h_data));
	}
#endif
#endif
#if (FORCE_DUMP_DATA == ENABLE)
	if (0+ISF_IN_BASE == WATCH_PORT) {
		DBG_DUMP("%s.in[0]! Proc OK, Release -- vdo: blk_id=%08x addr=%08x blk[%d].cnt=%d\r\n", p_thisunit->unit_name, p_input_data->h_data, nvtmpp_vb_blk2va(p_input_data->h_data), j, p_ctx->inq.input_used[j]);
	}
#endif

#if (USE_NEW_SHDR == ENABLE)
	// set input queue empty
	if (p_ctx->ctrl.shdr_cnt > 0) {
		UINT32 shdr_j = ((j >> 2) << 2);
		UINT32 shdr_i = j - shdr_j;
#if (SHOW_SHDR == ENABLE)
		DBG_DUMP("release shdr_j=%d, shdr_i=%d\r\n", shdr_j, shdr_i);
#endif
		loc_cpu(flags);
		if (p_ctx->inq.input_data == 0) {
			unl_cpu(flags);
		} else
		if (p_ctx->inq.input_used[(shdr_j >> 2)] & (1<<(shdr_i))) {
			p_input_data = &c_data;
			memcpy(p_input_data, &p_ctx->inq.input_data[j], sizeof(ISF_DATA));
			if (p_input_data->func != 1) {
				p_input_data = 0;
				unl_cpu(flags);
				DBG_ERR("not under proc!\r\n");
			} else {
				p_ctx->inq.input_used[(shdr_j >> 2)] &= ~(1<<(shdr_i));
				p_ctx->inq.input_cnt --;
				(&p_ctx->inq.input_data[j])->func = 0;
				unl_cpu(flags);
			}
		} else {
			unl_cpu(flags);
		}
	} else
#endif
	{
		loc_cpu(flags);
		if (p_ctx->inq.input_data == 0) {
			unl_cpu(flags);
		} else
		if (p_ctx->inq.input_used[j]) {
			p_input_data = &c_data;
			memcpy(p_input_data, &p_ctx->inq.input_data[j], sizeof(ISF_DATA));
			if (p_input_data->func != 1) {
				p_input_data = 0;
				unl_cpu(flags);
				DBG_ERR("not under proc!\r\n");
			} else {
				p_ctx->inq.input_used[j] = FALSE;
				p_ctx->inq.input_cnt --;
				(&p_ctx->inq.input_data[j])->func = 0;
				unl_cpu(flags);
			}
		} else {
			unl_cpu(flags);
		}
	}

	if (p_input_data) {
		p_thisunit->p_base->do_release(p_thisunit, iport, p_input_data, probe);
		p_thisunit->p_base->do_probe(p_thisunit, iport, probe, r);
	} else {
	}
}

//release after trigger
void _vdoprc_iport_dropdata(ISF_UNIT *p_thisunit, UINT32 iport, UINT32 buf_handle, UINT32 probe, UINT32 r)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 j = buf_handle;
	ISF_DATA c_data;
	ISF_DATA* p_input_data    = 0;
	unsigned long flags;

#if (FORCE_DUMP_DATA == ENABLE)
	if (0+ISF_IN_BASE == WATCH_PORT) {
		DBG_DUMP("%s.in[0]! Proc fail, Release -- vdo: blk_id=%08x addr=%08x blk[%d].cnt=%d\r\n", p_thisunit->unit_name, p_input_data->h_data, nvtmpp_vb_blk2va(p_input_data->h_data), j, p_ctx->inq.input_used[j]);
	}
#endif
	if (j == 0xff) { //data is not in queue, just do probe
		p_thisunit->p_base->do_probe(p_thisunit, iport, probe, r);
		return;
	}

#if (USE_NEW_SHDR == ENABLE)
	// set input queue empty
	if (p_ctx->ctrl.shdr_cnt > 0) {
		UINT32 shdr_j = ((j >> 2) << 2);
		UINT32 shdr_i = j - shdr_j;
#if (SHOW_SHDR == ENABLE)
		DBG_DUMP("drop shdr_j=%d, shdr_i=%d\r\n", shdr_j, shdr_i);
#endif
		loc_cpu(flags);
		if (p_ctx->inq.input_data == 0) {
			unl_cpu(flags);
		} else
		if (p_ctx->inq.input_used[(shdr_j >> 2)] & (1<<(shdr_i))) {
			p_input_data = &c_data;
			memcpy(p_input_data, &p_ctx->inq.input_data[j], sizeof(ISF_DATA));
			if (p_input_data->func != 1) {
				p_input_data = 0;
				unl_cpu(flags);
				DBG_ERR("not under proc!\r\n");
			} else {
				p_ctx->inq.input_used[(shdr_j >> 2)] &= ~(1<<(shdr_i));
				p_ctx->inq.input_cnt --;
				(&p_ctx->inq.input_data[j])->func = 0;
				unl_cpu(flags);
			}
		} else {
			unl_cpu(flags);
		}
	} else
#endif
	{
		loc_cpu(flags);
		if (p_ctx->inq.input_data == 0) {
			unl_cpu(flags);
		} else
		if (p_ctx->inq.input_used[j]) {
			p_input_data = &c_data;
			memcpy(p_input_data, &p_ctx->inq.input_data[j], sizeof(ISF_DATA));
			if (p_input_data->func != 1) {
				p_input_data = 0;
				unl_cpu(flags);
				DBG_ERR("not under proc!\r\n");
			} else {
				p_ctx->inq.input_used[j] = FALSE;
				p_ctx->inq.input_cnt --;
				(&p_ctx->inq.input_data[j])->func = 0;
				unl_cpu(flags);
			}
		} else {
			unl_cpu(flags);
		}
	}

	if (p_input_data) {
		p_thisunit->p_base->do_release(p_thisunit, iport, p_input_data, probe);
		p_thisunit->p_base->do_probe(p_thisunit, iport, probe, r);
	} else {
	}
}

//release before trigger
static void _vdoprc_iport_canceldata(ISF_UNIT *p_thisunit, UINT32 iport, UINT32 buf_handle, UINT32 probe, UINT32 r)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 j = buf_handle;
	ISF_DATA c_data;
	ISF_DATA* p_input_data    = 0;
	unsigned long flags;

#if (FORCE_DUMP_DATA == ENABLE)
	if (0+ISF_IN_BASE == WATCH_PORT) {
		DBG_DUMP("%s.in[0]! Proc fail, Release -- vdo: blk_id=%08x addr=%08x blk[%d].cnt=%d\r\n", p_thisunit->unit_name, p_input_data->h_data, nvtmpp_vb_blk2va(p_input_data->h_data), j, p_ctx->inq.input_used[j]);
	}
#endif

#if (USE_NEW_SHDR == ENABLE)
	// set input queue empty
	if (p_ctx->ctrl.shdr_cnt > 0) {
		UINT32 shdr_j = ((j >> 2) << 2);
		UINT32 shdr_i = j - shdr_j;
#if (SHOW_SHDR == ENABLE)
		DBG_DUMP("cancel shdr_j=%d, shdr_i=%d\r\n", shdr_j, shdr_i);
#endif
		loc_cpu(flags);
		if (p_ctx->inq.input_data == 0) {
			unl_cpu(flags);
		} else
		if (p_ctx->inq.input_used[(shdr_j >> 2)] & (1<<(shdr_i))) {
			p_input_data = &c_data;
			memcpy(p_input_data, &p_ctx->inq.input_data[j], sizeof(ISF_DATA));
			if (p_input_data->func == 1) {
				p_input_data = 0;
				unl_cpu(flags);
				// still under proc! do not cancel!
			} else {
				p_ctx->inq.input_used[(shdr_j >> 2)] &= ~(1<<(shdr_i));
				p_ctx->inq.input_cnt --;
				unl_cpu(flags);
			}
		} else {
			unl_cpu(flags);
		}
	} else
#endif
	{
		loc_cpu(flags);
		if (p_ctx->inq.input_data == 0) {
			unl_cpu(flags);
		} else
		if (p_ctx->inq.input_used[j]) {
			p_input_data = &c_data;
			memcpy(p_input_data, &p_ctx->inq.input_data[j], sizeof(ISF_DATA));
			if (p_input_data->func == 1) {
				p_input_data = 0;
				unl_cpu(flags);
				// still under proc! do not cancel!
			} else {
				p_ctx->inq.input_used[j] = FALSE;
				p_ctx->inq.input_cnt --;
				unl_cpu(flags);
			}
		} else {
			unl_cpu(flags);
		}
	}

	if (p_input_data) {
		p_thisunit->p_base->do_release(p_thisunit, iport, p_input_data, probe);
		p_thisunit->p_base->do_probe(p_thisunit, iport, probe, r);
	} else {
	}
}

static void _vdoprc_iport_cancelalldata(ISF_UNIT *p_thisunit, UINT32 iport, UINT32 probe, UINT32 r)
{
#if (USE_NEW_SHDR == ENABLE)
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 j = 0;
	if (p_ctx->ctrl.shdr_cnt > 0) {
		UINT32 shdr_j, shdr_i;
#if (SHOW_SHDR == ENABLE)
		DBG_DUMP("ignore shdr all\r\n");
#endif
		for (shdr_j = 0; shdr_j < p_ctx->inq.input_max; shdr_j+=4) { //queue depth
			for (shdr_i = 0; shdr_i < p_ctx->ctrl.shdr_cnt; shdr_i++) { //shdr array
				j = shdr_j + shdr_i;
				_vdoprc_iport_canceldata(p_thisunit, iport, j, probe, r);
#if (SHOW_SHDR == ENABLE)
				DBG_DUMP("ignore shdr_j=%d, shdr_i=%d\r\n", shdr_j, shdr_i);
#endif
			}
		}
	}
#endif
	else {
		for (j = 0; j < p_ctx->inq.input_max; j++) {
			_vdoprc_iport_canceldata(p_thisunit, iport, j, probe, r);
		}
	}
}

static void _vdoprc_iport_ignore(ISF_UNIT *p_thisunit, UINT32 iport, ISF_DATA *p_data, UINT32 probe, UINT32 r)
{
#if (USE_NEW_SHDR == ENABLE)
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 j = 0;
#endif
	p_thisunit->p_base->do_release(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH);
	p_thisunit->p_base->do_probe(p_thisunit, iport, probe, r);

	if (p_ctx->dev_trigger_close == 1) {
		return;
	}

#if (USE_NEW_SHDR == ENABLE)
	if (p_ctx->ctrl.shdr_cnt > 0) {
		UINT32 c;
#if (SHOW_SHDR_2 == ENABLE)
		VDO_FRAME *p_vdoframe;
		p_vdoframe = (VDO_FRAME *)(p_data->desc);
		DBG_DUMP("ignore shdr_j=%d, shdr_i=%d: pxlfmt=%08x, addr[0]=%08x, r=%d\r\n", p_ctx->ctrl.shdr_j, c, p_vdoframe->pxlfmt, p_vdoframe->addr[0], r);
		p_vdoframe->pxlfmt &= ~0x0000f000; //clear seq in pxlfmt
#endif
#if (SHOW_SHDR == ENABLE)
		DBG_DUMP("ignore shdr_j=%d, shdr_i=%d\r\n", p_ctx->ctrl.shdr_j, p_ctx->ctrl.shdr_i);
#endif
		if (p_ctx->ctrl.shdr_i == 0xff)
			return;
		if (p_ctx->ctrl.shdr_i == 0)
			return;
		for (c = p_ctx->ctrl.shdr_i-1; c > 0; c--) {
				j = p_ctx->ctrl.shdr_j + c;
				_vdoprc_iport_dropdata(p_thisunit, iport, j, probe, r);
#if (SHOW_SHDR == ENABLE)
				DBG_DUMP("ignore shdr_j=%d, i=%d\r\n", p_ctx->ctrl.shdr_j, c);
#endif
		}
	}
#endif
}

ISF_RV _isf_vdoprc_iport_do_push(ISF_UNIT *p_thisunit, UINT32 iport, ISF_DATA *p_data, INT32 wait_ms)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	VDO_FRAME *p_vdoframe = (VDO_FRAME *)(p_data->desc);
	UINT32 j = 0;
	BOOL allow_raw = FALSE, allow_nrx = FALSE, allow_yuv = FALSE, allow_rgb = FALSE, fmt_ok = FALSE;
	BOOL allow_scale = FALSE;
#if (USE_NEW_SHDR == ENABLE)
	BOOL allow_shdr = FALSE;
#endif
	ISF_DATA* p_input_data;
	INT32 kr;
	ISF_RV r;
	if (p_ctx == 0) {
		return ISF_ERR_UNINIT; //still not init
	}

	#ifdef ISF_TS
	p_data->ts_vprc_in[0] = hwclock_get_longcounter();
	#ifdef ISF_DUMP_TS
	DBG_DUMP("vprc-0: %lld\r\n",
		p_data->ts_vprc_in[0]);
	#endif
	#endif
	p_thisunit->p_base->do_probe(p_thisunit, iport, ISF_IN_PROBE_PUSH, ISF_ENTER);

	if (p_ctx->dev_trigger_close == 1) {
		_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_OPEN_YET);
		return ISF_ERR_NOT_OPEN_YET;
	}

#if 0
	if(p_data->src == ISF_CTRL) {
		//push from user
		p_vdoframe->count = p_ctx->in[0].user_count; //auto fill frame count by self counter
		p_ctx->in[0].user_count++; //increase self counter
	}
#endif

#if (FORCE_DUMP_DATA == ENABLE)
	if (0+ISF_IN_BASE == WATCH_PORT) {
		DBG_DUMP("^M%s.in[0]! Push -- vdo: blk_id=%08x addr=%08x\r\n", p_thisunit->unit_name, p_data->h_data, nvtmpp_vb_blk2va(p_data->h_data));
	}
#endif
	if (p_data->h_data == 0) {
		DBG_ERR("%s.in[0]! In Data error -- vdo: blk_id=%08x?\r\n", p_thisunit->unit_name, p_data->h_data);
		p_thisunit->p_base->do_probe(p_thisunit, iport, ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INVALID_DATA);
		//p_thisunit->p_base->do_release(p_thisunit, iport, p_data, 0, ISF_ERR_INVALID_DATA);
		return ISF_ERR_INVALID_DATA;
	}

#if (USE_IN_DIRECT == ENABLE)
	if ((p_ctx->new_in_cfg_func & VDOPRC_IFUNC_DIRECT)
 	|| (p_ctx->cur_in_cfg_func & VDOPRC_IFUNC_DIRECT)) {
		DBG_ERR("%s.in[0]! not allow! -- func=direct\r\n", p_thisunit->unit_name);
		_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INVALID_PARAM);
		return ISF_ERR_INVALID_PARAM;
	}
#endif

	switch (p_ctx->cur_mode) {
	case VDOPRC_PIPE_RAWCAP:
	case VDOPRC_PIPE_RAWALL:
	//case VDOPRC_PIPE_RAWALL|VDOPRC_PIPE_WARP_360:
	case VDOPRC_PIPE_YUVCAP:
	case VDOPRC_PIPE_YUVALL:
	//case VDOPRC_PIPE_YUVAUX:
	case VDOPRC_PIPE_SCALE:
	case VDOPRC_PIPE_COLOR:
	case VDOPRC_PIPE_WARP:
#if (USE_VPE == ENABLE)
	case VDOPRC_PIPE_VPE:
#endif
#if (USE_ISE == ENABLE)
	case VDOPRC_PIPE_ISE:
#endif
		allow_raw = (p_ctx->ifunc_allow & (VDOPRC_IFUNC_RAW))?(1):(0);
		allow_nrx = (p_ctx->ifunc_allow & (VDOPRC_IFUNC_NRX))?(1):(0);
		allow_yuv = (p_ctx->ifunc_allow & (VDOPRC_IFUNC_YUV))?(1):(0);
		allow_rgb = (p_ctx->ifunc_allow & (VDOPRC_IFUNC_RGB))?(1):(0);
		allow_scale = (p_ctx->ifunc_allow & (VDOPRC_IFUNC_SCALEUP|VDOPRC_IFUNC_SCALEDN))?(1):(0);
		break;
	case 0:
		//DBG_WRN("%s.in[0]! In Data error -- pipe=%08x\r\n", p_thisunit->unit_name, p_ctx->cur_mode);
		_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_INACTIVE_STATE);
		return ISF_ERR_NOT_START;
		break;
	default:
		DBG_ERR("%s.in[0]! Pipe error -- pipe=%08x?\r\n", p_thisunit->unit_name, p_ctx->cur_mode);
		_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INVALID_PARAM);
		return ISF_ERR_INVALID_PARAM;
		break;
	}

	switch (VDO_PXLFMT_CLASS(p_vdoframe->pxlfmt)) {
	case VDO_PXLFMT_CLASS_RAW:
		if(allow_raw) fmt_ok = TRUE;
		{
			int p;
			/*
			for (p = 0; p < VDO_MAX_PLANE; p++) {
				if ((p_vdoframe->pw[p] != 0) && (p_vdoframe->loff[p] == 0)) {
					p_vdoframe->loff[p] = ALIGN_CEIL_4(p_vdoframe->pw[p]);
				}
			}*/
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
#if (USE_NEW_SHDR == ENABLE)
		if (p_ctx->ctrl.func_max & VDOPRC_FUNC_SHDR) {
			if (VDO_PXLFMT_PLANE(p_vdoframe->pxlfmt) == p_ctx->ctrl.shdr_cnt) {
				//allow shdr, and receive shdr-frame
				allow_shdr = TRUE; p_ctx->shdr_in = p_ctx->ctrl.shdr_cnt;
			} else if (VDO_PXLFMT_PLANE(p_vdoframe->pxlfmt) == 1) {
				//allow shdr, but receive linear-frame
				allow_shdr = TRUE; p_ctx->shdr_in = 1;
			} else {
				//not allow shdr
				fmt_ok = FALSE;
			}
		}
#endif
		break;
	case VDO_PXLFMT_CLASS_NRX:
		if(allow_nrx) fmt_ok = TRUE;
		{
			int p;
			/*
			for (p = 0; p < VDO_MAX_PLANE; p++) {
				if ((p_vdoframe->pw[p] != 0) && (p_vdoframe->loff[p] == 0)) {
					p_vdoframe->loff[p] = ALIGN_CEIL_4(p_vdoframe->pw[p]);
				}
			}*/
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
#if (USE_NEW_SHDR == ENABLE)
		if (p_ctx->ctrl.func_max & VDOPRC_FUNC_SHDR) {
			if (VDO_PXLFMT_PLANE(p_vdoframe->pxlfmt) == p_ctx->ctrl.shdr_cnt) {
				//allow shdr, and receive shdr-frame
				allow_shdr = TRUE; p_ctx->shdr_in = p_ctx->ctrl.shdr_cnt;
			} else if (VDO_PXLFMT_PLANE(p_vdoframe->pxlfmt) == 1) {
				//allow shdr, but receive linear-frame
				allow_shdr = TRUE; p_ctx->shdr_in = 1;
			} else {
				//not allow shdr
				fmt_ok = FALSE;
			}
		}
#endif
		break;
	case VDO_PXLFMT_CLASS_ARGB:
		if(allow_rgb) fmt_ok = TRUE;
		switch(p_vdoframe->pxlfmt) {
		case VDO_PXLFMT_RGB888_PLANAR:
			{
				int p;
				/*
				for (p = 0; p < VDO_MAX_PLANE; p++) {
					if ((p_vdoframe->pw[p] != 0) && (p_vdoframe->loff[p] == 0)) {
						p_vdoframe->loff[p] = ALIGN_CEIL_4(p_vdoframe->pw[p]);
					}
				}*/
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
			break;
		default:
			break;
		}
		break;
	case VDO_PXLFMT_CLASS_YUV:
		if(allow_yuv) fmt_ok = TRUE;
		switch((UINT32)(p_vdoframe->pxlfmt)) {
		case VDO_PXLFMT_Y8:
		case VDO_PXLFMT_YUV420:
		case VDO_PXLFMT_YUV422:
		case VDO_PXLFMT_YUV422|VDO_PIX_YCC_BT601:
		case VDO_PXLFMT_YUV420|VDO_PIX_YCC_BT601:
		case VDO_PXLFMT_YUV422|VDO_PIX_YCC_BT709:
		case VDO_PXLFMT_YUV420|VDO_PIX_YCC_BT709:
		case VDO_PXLFMT_YUV420_PLANAR:
		case VDO_PXLFMT_YUV422_PLANAR:
		case VDO_PXLFMT_YUV444_PLANAR:
			{
				int p;
				/*
				for (p = 0; p < VDO_MAX_PLANE; p++) {
					if ((p_vdoframe->pw[p] != 0) && (p_vdoframe->loff[p] == 0)) {
						p_vdoframe->loff[p] = ALIGN_CEIL_4(p_vdoframe->pw[p]);
					}
				}*/
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
			break;
		default:
			break;
		}
		break;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	case 0xe:
		if(allow_yuv) fmt_ok = TRUE;
		switch(p_vdoframe->pxlfmt) {
		case VDO_PXLFMT_520_IR8:
		//case VDO_PXLFMT_520_IR16:
			{
				int p;
				/*
				for (p = 0; p < VDO_MAX_PLANE; p++) {
					if ((p_vdoframe->pw[p] != 0) && (p_vdoframe->loff[p] == 0)) {
						p_vdoframe->loff[p] = ALIGN_CEIL_4(p_vdoframe->pw[p]);
					}
				}*/
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
			break;
		default:
			break;
		}
		break;
#endif
	default:
		break;
	}
	if ((p_ctx->ctrl.func_max & VDOPRC_FUNC_SHDR) == 0 && p_ctx->ctrl.shdr_cnt) {
		DBG_ERR("%s ctrl_max not updated! (func_max=0x%X, shdr_cnt=%d)\r\n", p_thisunit->unit_name, p_ctx->ctrl.func_max, p_ctx->ctrl.shdr_cnt);
		return ISF_ERR_INVALID_DATA;
	}
	if(!fmt_ok) {
		//DBG_WRN("%s.in[0]! In Data error -- pxlfmt=%08x is not support with pipe=%08x\r\n", p_thisunit->unit_name, p_vdoframe->pxlfmt, p_ctx->cur_mode);
		_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_WRN, ISF_ERR_INVALID_DATA);
		return ISF_ERR_INVALID_DATA;
	}

	// if enable 3DNR: check size of input == size of output[3dnr_ref_path]?
	if ((p_ctx->func_allow & VDOPRC_FUNC_3DNR) && (p_ctx->ctrl.cur_func & VDOPRC_FUNC_3DNR)) {
		UINT32 i = p_ctx->ctrl._3dnr_refpath;
		USIZE in_dim = {0};
		if (p_ctx->out[i].enable == FALSE) {

			if (p_ctx->err_cnt == 0) // this is a 1/60 error msg
			DBG_ERR("%s.in[0]! ref_path out[%d] not start for 3DNR? drop all frames ...\r\n",
				p_thisunit->unit_name, i);
			p_ctx->err_cnt++; if (p_ctx->err_cnt == 60) p_ctx->err_cnt = 0;

			_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_WRN, ISF_ERR_NOT_START);
			return ISF_ERR_NOT_START;
		}
		if (p_ctx->in[0].crop.mode == CTL_IPP_IN_CROP_NONE) {
			//disable in crop
			in_dim.w = (UINT32)p_vdoframe->size.w;
			in_dim.h = (UINT32)p_vdoframe->size.h;
		} else if (p_ctx->in[0].crop.mode == CTL_IPP_IN_CROP_AUTO) {
			//enable in crop
			//x is (p_vdoframe->reserved[5] & 0xffff0000) >> 16;
			//y is (p_vdoframe->reserved[5] & 0x0000ffff);
			in_dim.w = (p_vdoframe->reserved[6] & 0xffff0000) >> 16;
			in_dim.h = (p_vdoframe->reserved[6] & 0x0000ffff);
		} else if (p_ctx->in[0].crop.mode == CTL_IPP_IN_CROP_USER) {
			//enable in crop
			in_dim.w = p_ctx->in[0].crop.crp_window.w;
			in_dim.h = p_ctx->in[0].crop.crp_window.h;
		} else {
			DBG_ERR("%s.in[0]! invalid crop?\r\n",
				p_thisunit->unit_name);
		}
		if ((in_dim.w != p_ctx->out[i].size.w) || (in_dim.h != p_ctx->out[i].size.h)) {
			DBG_WRN("%s.in[0]! in size(%d,%d) != ref_path out[%d] size(%d,%d) for 3DNR?\r\n",
				p_thisunit->unit_name, in_dim.w, in_dim.h, i, p_ctx->out[i].size.w, p_ctx->out[i].size.h);
		}
	}


#if (USE_NEW_SHDR == ENABLE)
	if (allow_shdr && (p_ctx->shdr_in > 1)) {
		UINT32 seq = ((p_vdoframe->pxlfmt & 0x0000f000) >> 12); //extract seq value from mask 0x0000f000.
		//p_vdoframe->pxlfmt &= ~0x0000f000; //clear seq
#if (SHOW_SHDR == ENABLE)
		DBG_DUMP("shdr PUSH: pxlfmt=%08x, addr[0]=%08x seq=%d .... shdr_cnt=%d shdr_j=%d, shdr_i=%d\r\n",
			p_vdoframe->pxlfmt, p_vdoframe->addr[0], seq,
			p_ctx->ctrl.shdr_cnt, p_ctx->ctrl.shdr_j, p_ctx->ctrl.shdr_i);
#endif
		if (seq != p_ctx->ctrl.shdr_i) {
			if (p_ctx->ctrl.shdr_i == 0xff) {
				if (seq == 0) {
					p_ctx->ctrl.shdr_i = 0; //end of drop
				} else {
        			DBG_WRN("%s.in[0]! In Data drop -- shdr seq\r\n", p_thisunit->unit_name);
					_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL);
					return ISF_ERR_QUEUE_FULL;
				}
			} else {
#if (SHOW_SHDR == ENABLE)
				DBG_ERR("shdr DROP / seq-not-match!\r\n");
#endif
				//DBG_ERR("%s.in[0]! In Data error -- sequence %d is not match with pipe=%08x\r\n", p_thisunit->unit_name, p_vdoframe->pxlfmt, p_ctx->cur_mode);
				_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_WRN, ISF_ERR_INCOMPLETE_DATA);
				return ISF_ERR_INVALID_DATA;
			}
		}
	} else if (allow_shdr && (p_ctx->shdr_in == 1)) {
		p_ctx->ctrl.shdr_i = 0;
#if (SHOW_SHDR == ENABLE)
		DBG_DUMP("shdr PUSH: pxlfmt=%08x, addr[0]=%08x seq=(n/a) .... shdr_cnt=%d shdr_j=%d, shdr_i=%d\r\n",
			p_vdoframe->pxlfmt, p_vdoframe->addr[0],
			p_ctx->ctrl.shdr_cnt, p_ctx->ctrl.shdr_j, p_ctx->ctrl.shdr_i);
#endif
	}
#endif

	// verify size of input <= size of max input?
	{
		ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
		if (((UINT32)p_vdoframe->size.w > p_vdoinfo->max_imgsize.w) || ((UINT32)p_vdoframe->size.h > p_vdoinfo->max_imgsize.h)) {
			//DBG_WRN("%s.in[0]! In Data warn -- vdo size(%dx%d) > max_size(%dx%d)\r\n",
			//	p_thisunit->unit_name, p_vdoframe->size.w, p_vdoframe->size.h, p_vdoinfo->max_imgsize.w, p_vdoinfo->max_imgsize.h, p_ctx->cur_mode);
			_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_WRN, ISF_ERR_INVALID_DATA);
			return ISF_ERR_INVALID_DATA;
		}
	}

	// if not allow scale: verify size of input == size of output[0]?
	if (!allow_scale) {
		if (((UINT32)p_vdoframe->size.w != p_ctx->out[0].size.w) || ((UINT32)p_vdoframe->size.h != p_ctx->out[0].size.h)) {
			//DBG_WRN("%s.in[0]! In Data warn -- vdo size(%dx%d) != out[0].size(%dx%d), with pipe=%08x\r\n",
			//	p_thisunit->unit_name, p_vdoframe->size.w, p_vdoframe->size.h, p_ctx->out[0].size.w, p_ctx->out[0].size.h, p_ctx->cur_mode);
			_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_WRN, ISF_ERR_INVALID_DATA);
			return ISF_ERR_INVALID_DATA;
		}
 	}

#if (USE_EIS == ENABLE)
	if (_vprc_eis_plugin_ && p_ctx->eis_func && (p_ctx->ctrl.shdr_i == 0)) {
		//VDOPRC_EIS_PROC_INFO proc_info;
		UINT32  data_num = 0, data_size;
		ULONG rsv1 = p_vdoframe->reserved[1];
		INT32 *agyro_x;
		INT32 *agyro_y;
		INT32 *agyro_z;
		INT32 *ags_x;
		INT32 *ags_y;
		INT32 *ags_z;
		UINT64 t_diff_crop;
		UINT64 t_diff_crp_end_to_vd;
		ULONG time_stamp_addr;
		ULONG gyro_addr;
		VDOCAP_EIS_GYRO_INFO *p_gyro_info = NULL;

		if (p_ctx->p_eis_info_ctx == NULL) {
			DBG_ERR("eis ctx NULL!\r\n");
			return ISF_ERR_FAILED;
		}
		if (p_ctx->eis_func == VDOPRC_EIS_FUNC_CUSTOMIZED && p_vdoframe->phyaddr[3]) {
			p_gyro_info = (VDOCAP_EIS_GYRO_INFO *)nvtmpp_sys_pa2va(p_vdoframe->phyaddr[3]);
		}
		//check latencty info in SHDR mode
		if (p_ctx->eis_func == VDOPRC_EIS_FUNC_NORMAL && allow_shdr && (p_ctx->shdr_in > 1)) {
			if (p_vdoframe->loff[3]) {
				if (vprc_gyro_latency == 0) {
					DBG_DUMP("Enable vprc_gyro_latency in SHDR\r\n");
					vprc_gyro_latency = 1;
				}
			}
		}
		if ((rsv1 && (*(UINT32 *)(rsv1))) || (p_gyro_info && p_gyro_info->data_num)) {
			if (p_ctx->eis_func == VDOPRC_EIS_FUNC_CUSTOMIZED && p_gyro_info) {
				data_num = p_gyro_info->data_num;
				vprc_gyro_data_num = data_num;
				t_diff_crop =  p_gyro_info->t_diff_crop;
				t_diff_crp_end_to_vd = p_gyro_info->t_diff_crp_end_to_vd;
				agyro_x =  (INT32 *)p_gyro_info->angular_rate_x;
				agyro_y =  (INT32 *)p_gyro_info->angular_rate_y;
				agyro_z =  (INT32 *)p_gyro_info->angular_rate_z;
				ags_x = (INT32 *)p_gyro_info->acceleration_rate_x;
				ags_y = (INT32 *)p_gyro_info->acceleration_rate_y;
				ags_z = (INT32 *)p_gyro_info->acceleration_rate_z;
				time_stamp_addr = (ULONG)p_gyro_info->gyro_timestamp;
			} else {
				data_num = *(UINT32 *)(rsv1);
				vprc_gyro_data_num = data_num;
				t_diff_crop =  *(UINT64 *)(rsv1 + sizeof(data_num));
				t_diff_crp_end_to_vd = *(UINT64 *)(rsv1 + sizeof(data_num) + sizeof(t_diff_crop));
				time_stamp_addr = rsv1 + sizeof(data_num) + sizeof(t_diff_crop) + sizeof(t_diff_crp_end_to_vd);
				//DBG_ERR("%lld ts %u\r\n",p_vdoframe->timestamp,*(UINT32 *)(time_stamp_addr));
				gyro_addr = rsv1 + sizeof(data_num) + sizeof(t_diff_crop) + sizeof(t_diff_crp_end_to_vd) +  data_num*sizeof(UINT32);
				agyro_x =  (INT32 *)(gyro_addr);
				agyro_y =  (INT32 *)(gyro_addr + (1 * data_num * sizeof(INT32)));
				agyro_z =  (INT32 *)(gyro_addr + (2 * data_num * sizeof(INT32)));
				ags_x = (INT32 *)(gyro_addr + (3 * data_num * sizeof(INT32)));
				ags_y = (INT32 *)(gyro_addr + (4 * data_num * sizeof(INT32)));
				ags_z = (INT32 *)(gyro_addr + (5 * data_num * sizeof(INT32)));
			}
			p_ctx->p_eis_info_ctx->frame_count = p_vdoframe->count;
			p_ctx->p_eis_info_ctx->t_diff_crop = t_diff_crop;
			p_ctx->p_eis_info_ctx->t_diff_crp_end_to_vd = t_diff_crp_end_to_vd;
			p_ctx->p_eis_info_ctx->data_num = data_num;
			p_ctx->p_eis_info_ctx->user_data_size = p_vdoframe->loff[2];
			//user data
			if (p_ctx->eis_func > VDOPRC_EIS_FUNC_NORMAL && p_vdoframe->phyaddr[2] && p_vdoframe->loff[2]) {
				ULONG user_va = nvtmpp_sys_pa2va(p_vdoframe->phyaddr[2]);

				//DBG_DUMP("User data pa = 0x%X, va = 0x%X, size=%d\r\n", (int)p_vdoframe->phyaddr[2], (int)user_va, (int)p_vdoframe->loff[2]);
				//debug_dumpmem(user_va, 64);//MAX_EIS_USER_DATA_SIZE)
				if (p_vdoframe->loff[2] > MAX_EIS_USER_DATA_SIZE) {
					DBG_ERR("eis user data(%d) > %d\r\n", p_vdoframe->loff[2], MAX_EIS_USER_DATA_SIZE);
					return ISF_ERR_INVALID_VALUE;
				}
				memcpy((void *)p_ctx->p_eis_info_ctx->user_data, (void *)user_va, p_vdoframe->loff[2]);
			} else {
				p_ctx->p_eis_info_ctx->user_data_size = 0;
			}
			if (data_num > MAX_GYRO_DATA_NUM) {
				DBG_ERR("Gyro data_num(%d) > %d\r\n", data_num, MAX_GYRO_DATA_NUM);
			} else if (data_num) {
				isf_vdoprc_put_gyro_frame_cnt(p_ctx->p_eis_info_ctx->frame_count);
				data_size = data_num*sizeof(UINT32);
				memcpy((void *)p_ctx->p_eis_info_ctx->angular_rate_x, agyro_x, data_size);
				memcpy((void *)p_ctx->p_eis_info_ctx->angular_rate_y, agyro_y, data_size);
				memcpy((void *)p_ctx->p_eis_info_ctx->angular_rate_z, agyro_z, data_size);
				memcpy((void *)p_ctx->p_eis_info_ctx->acceleration_rate_x, ags_x, data_size);
				memcpy((void *)p_ctx->p_eis_info_ctx->acceleration_rate_y, ags_y, data_size);
				memcpy((void *)p_ctx->p_eis_info_ctx->acceleration_rate_z, ags_z, data_size);
				memcpy((void *)p_ctx->p_eis_info_ctx->gyro_timestamp, (INT32 *)time_stamp_addr, data_size);

				if (vprc_gyro_latency == 0 && data_num > 1) {
					//check gyro timestamp vs frame start/end
					UINT32 gyro_time_interval = p_ctx->p_eis_info_ctx->gyro_timestamp[1] - p_ctx->p_eis_info_ctx->gyro_timestamp[0];
					UINT32 crp_end = t_diff_crp_end_to_vd;
					UINT32 last_gyro_time = p_ctx->p_eis_info_ctx->gyro_timestamp[data_num-1];

					if ((p_ctx->p_eis_info_ctx->gyro_timestamp[0] > (t_diff_crop + gyro_time_interval)) ||
						((crp_end > last_gyro_time) && ((crp_end - last_gyro_time) > gyro_time_interval))) {
						DBG_DUMP("Enable vprc_gyro_latency gyro[0]=%d frm_start=%d gyro[%d]=%d frm_end=%d, gyro_time_interval=%d\r\n", p_ctx->p_eis_info_ctx->gyro_timestamp[0], (UINT32)t_diff_crop, data_num-1, p_ctx->p_eis_info_ctx->gyro_timestamp[data_num-1], (UINT32)t_diff_crp_end_to_vd, gyro_time_interval);
						vprc_gyro_latency = 1;
						_vprc_eis_plugin_->set(VDOPRC_EIS_PARAM_GYRO_LATENCY, &vprc_gyro_latency);
					}
				}
				_vprc_eis_plugin_->set(VDOPRC_EIS_PARAM_TRIG_PROC, p_ctx->p_eis_info_ctx);
				if (isf_vdoprc_eis_dbg_flag & VDOPRC_EIS_DBG_IN_GYRO) {
					UINT32 n;

					DBG_DUMP("FRAME(%llu) data_num=%d crop_start=%llu end=%llu\r\n", p_ctx->p_eis_info_ctx->frame_count, data_num, t_diff_crop, t_diff_crp_end_to_vd);
 					for (n = 0; n < data_num; n++) {
						DBG_DUMP("[%d]%d %d %d %d %d %d %d\r\n", n, p_ctx->p_eis_info_ctx->angular_rate_x[n], p_ctx->p_eis_info_ctx->angular_rate_y[n], p_ctx->p_eis_info_ctx->angular_rate_z[n]
														, p_ctx->p_eis_info_ctx->acceleration_rate_x[n], p_ctx->p_eis_info_ctx->acceleration_rate_y[n], p_ctx->p_eis_info_ctx->acceleration_rate_z[n]
														, p_ctx->p_eis_info_ctx->gyro_timestamp[n]);
					}
				}
				#if 0//for debug only
				{
					INT32 *p_data;
					DBG_DUMP("data_num=%d\r\n", data_num);
					p_data = p_ctx->p_eis_info_ctx->angular_rate_x;
					DBG_DUMP("angular_rate_x\r\n");
					DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																		*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					p_data = p_ctx->p_eis_info_ctx->angular_rate_y;
					DBG_DUMP("angular_rate_y\r\n");
					DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																		*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					p_data = p_ctx->p_eis_info_ctx->angular_rate_z;
					DBG_DUMP("angular_rate_y\r\n");
					DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																		*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					p_data = p_ctx->p_eis_info_ctx->acceleration_rate_x;
					DBG_DUMP("acceleration_rate_x\r\n");
					DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																		*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					p_data = p_ctx->p_eis_info_ctx->acceleration_rate_y;
					DBG_DUMP("acceleration_rate_y\r\n");
					DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																		*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					p_data = p_ctx->p_eis_info_ctx->acceleration_rate_z;
					DBG_DUMP("acceleration_rate_z\r\n");
					DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																	*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					p_data = p_ctx->p_eis_info_ctx->gyro_timestamp;
					DBG_DUMP("gyro_timestamp\r\n");
					DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																	*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
				}
				#endif
			}
		} else {
			DBG_DUMP("[%lld]No gyro!\r\n", p_vdoframe->count);
			_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_FRC_DROP);
			return ISF_OK;
		}
	}
#endif

#if (USE_IN_FRC == ENABLE)
	//do frc
	if (_isf_frc_is_select(p_thisunit, iport, &p_ctx->infrc[0]) == 0) {
		_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_FRC_DROP);
		return ISF_OK;
	}
#endif

	//verify device state
	if ((p_ctx->dev_ready == 0) || (p_ctx->dev_handle == 0)) {
		if (p_ctx->dev_trigger_open == 1) {
			//DBG_WRN("%s.in[0]! Skip Data -- device is not start\r\n", p_thisunit->unit_name);
			_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_START);
			return ISF_ERR_NOT_START;
		} if (p_ctx->dev_trigger_close == 1) {
			//DBG_WRN("%s.in[0]! Skip Data -- device is not start\r\n", p_thisunit->unit_name);
			_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_START);
			return ISF_ERR_NOT_START;
		} else {
			//DBG_WRN("%s.in[0]! Skip Data -- device is not start\r\n", p_thisunit->unit_name);
			_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INACTIVE_STATE);
			return ISF_ERR_INVALID_STATE;
		}
	}

	//drop if pull queue full
	{
		r = _isf_vdoprc_oqueue_wait_for_push_in(p_thisunit, ISF_OUT(0), wait_ms);
		if (r != ISF_OK) {
			DBG_WRN("%s.in[0]! In Data drop -- pull queue full\r\n", p_thisunit->unit_name);
			//pull queue is full, not allow push
			_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_QUEUE_FULL);

			if (allow_shdr && (p_ctx->shdr_in > 1)) {
				p_ctx->ctrl.shdr_i = 0xff;  //begin of drop
				//DBG_DUMP("@@@cnt=%d pxlfmt=%08x, shdr_cnt=%d shdr_j=%d, shdr_i=%d\r\n",
				//	(UINT32)p_vdoframe->count, p_vdoframe->pxlfmt, p_ctx->ctrl.shdr_cnt, p_ctx->ctrl.shdr_j, p_ctx->ctrl.shdr_i);
			}
			return ISF_ERR_QUEUE_FULL;
		}
	}

	//drop if work queue full
#if (USE_VPE == ENABLE)
	if (p_ctx->vpe_mode) {
		//TBD
	} else
#endif
#if (USE_ISE == ENABLE)
	if (p_ctx->ise_mode) {
		//TBD
	} else
#endif
	{
		UINT32 num = 0;
		INT32 cr = ctl_ipp_get(p_ctx->dev_handle, CTL_IPP_ITEM_PUSHEVT_INQ, (void *)&num);
		if ((CTL_IPP_E_OK != cr) || (num >= 2)) {
			DBG_WRN("%s.in[0]! In Data drop -- work queue full\r\n", p_thisunit->unit_name);
			//work queue is full, never proc any more
			_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL);
			if (allow_shdr && (p_ctx->shdr_in > 1)) {
				p_ctx->ctrl.shdr_i = 0xff;  //begin of drop
				//DBG_DUMP("@@@cnt=%d pxlfmt=%08x, shdr_cnt=%d shdr_j=%d, shdr_i=%d\r\n",
				//	(UINT32)p_vdoframe->count, p_vdoframe->pxlfmt, p_ctx->ctrl.shdr_cnt, p_ctx->ctrl.shdr_j, p_ctx->ctrl.shdr_i);
			}
			return ISF_ERR_QUEUE_FULL;
		}
	}

	if (p_ctx->dev_trigger_close == 1) {
		_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_OPEN_YET);
		return ISF_ERR_NOT_OPEN_YET; //force abort
	}

	//push data to input queue : j
	j = _vdoprc_iport_newdata(p_thisunit, iport, p_data);
#if (USE_NEW_SHDR == ENABLE)
#if (SHOW_SHDR == ENABLE)
	DBG_DUMP("queue shdr_j=%d, shdr_i=%d to j=%d\r\n", p_ctx->ctrl.shdr_j, p_ctx->ctrl.shdr_i, j);
#endif
#endif

	if ((j == 0xff) || (j == 0xfe)) {
#if (FORCE_DUMP_DATA == ENABLE)
		if (0+ISF_IN_BASE == WATCH_PORT) {
			DBG_DUMP("^M%s.in[0]! In Queue full -- vdo: blk_id=%08x addr=%08x\r\n", p_thisunit->unit_name, p_data->h_data, nvtmpp_vb_blk2va(p_data->h_data));
		}
#endif
#if (USE_NEW_SHDR == ENABLE)
        if (p_ctx->shdr_in > 1) {
    		p_ctx->ctrl.shdr_i = 0xff;  //begin of drop
        }
#endif
		if (j == 0xfe) {
			//device is closed, not allow process
			_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_OPEN_YET);
			return ISF_ERR_NOT_OPEN_YET;
		}

		//0xff
		//queue is full, not allow process
		DBG_WRN("%s.in[0]! In Data drop -- index full\r\n", p_thisunit->unit_name);
		_vdoprc_iport_ignore(p_thisunit, iport, p_data, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_QUEUE_FULL);
		return ISF_ERR_QUEUE_FULL;
	}

	if (p_ctx->dev_trigger_close == 1) {
		_vdoprc_iport_canceldata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_OPEN_YET);
		return ISF_ERR_NOT_OPEN_YET; //force abort
	}

#if (USE_NEW_SHDR == ENABLE)
	if (allow_shdr && (p_ctx->shdr_in > 1)) {

		//allow shdr, and receive shdr-frame

		if (p_ctx->ctrl.shdr_i < (p_ctx->ctrl.shdr_cnt-1)) {
			//shdr seq is not complete, not trigger, just keep it!
			if (p_ctx->dev_trigger_close == 1) {
				_vdoprc_iport_canceldata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_OPEN_YET);
				return ISF_ERR_NOT_OPEN_YET; //force abort
			}
#if (SHOW_SHDR_2 == ENABLE)
					DBG_DUMP("skip shdr_j=%d, shdr_i=%d\r\n", p_ctx->ctrl.shdr_j, p_ctx->ctrl.shdr_i);
#endif
			//next
			p_ctx->ctrl.shdr_i ++;
			if (p_ctx->ctrl.shdr_i == p_ctx->ctrl.shdr_cnt) {
				p_ctx->ctrl.shdr_i = 0; //reset
			}
#if (SHOW_SHDR_2 == ENABLE)
			DBG_DUMP("add shdr_i=%d\r\n", p_ctx->ctrl.shdr_i);
#endif
			return ISF_OK;
		} else {
			//shdr seq is complete, trigger process with data-set (from input queue : j)
			CTL_IPP_EVT_HDR evt;
			UINT32 c;
			unsigned long flags;
			loc_cpu(flags);
			if (p_ctx->inq.input_data == 0) {
				unl_cpu(flags);
				_vdoprc_iport_canceldata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_OPEN_YET);
				return ISF_ERR_NOT_OPEN_YET; //force abort
			} else {
				for (c = 0; c < p_ctx->ctrl.shdr_cnt; c++) {
					VDO_FRAME *p_vdoframe;
					p_input_data    = &p_ctx->inq.input_data[p_ctx->ctrl.shdr_j + c];
					p_input_data->func = 1; //start proc
					evt.buf_id[c] = p_ctx->ctrl.shdr_j + c;
					p_vdoframe = (VDO_FRAME *)(p_input_data->desc);
#if (SHOW_SHDR_2 == ENABLE)
					DBG_DUMP("send shdr_j=%d, i=%d: pxlfmt=%08x, addr[0]=%08x\r\n", p_ctx->ctrl.shdr_j, c, p_vdoframe->pxlfmt, p_vdoframe->addr[0]);
#endif
					p_vdoframe->pxlfmt &= ~0x0000f000; //clear seq in pxlfmt
					evt.data_addr[c] = (UINT32)p_vdoframe;
					evt.rev = 0;
				}
				unl_cpu(flags);
			}
			//trigger
			kr = ctl_ipp_ioctl(p_ctx->dev_handle, CTL_IPP_IOCTL_SNDEVT_HDR, (void *)&evt);
		}
		//next
		p_ctx->ctrl.shdr_i ++;
		if (p_ctx->ctrl.shdr_i == p_ctx->ctrl.shdr_cnt) {
			p_ctx->ctrl.shdr_i = 0; //reset
		}
#if (SHOW_SHDR_2 == ENABLE)
			DBG_DUMP("add shdr_i=%d\r\n", p_ctx->ctrl.shdr_i);
#endif
	} else if (allow_shdr && (p_ctx->shdr_in == 1)) {

		//allow shdr, but receive linear-frame

		//trigger process with data (from input queue : j)
		CTL_IPP_EVT evt;
		unsigned long flags;
		loc_cpu(flags);
		if (p_ctx->inq.input_data == 0) {
			unl_cpu(flags);
			_vdoprc_iport_canceldata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_OPEN_YET);
			return ISF_ERR_NOT_OPEN_YET; //force abort
		} else {
			p_input_data    = &p_ctx->inq.input_data[p_ctx->ctrl.shdr_j + 0];
			p_input_data->func = 1; //start proc
			evt.buf_id = p_ctx->ctrl.shdr_j + 0;
			evt.data_addr = (UINT32)(VDO_FRAME *)(p_input_data->desc);
#if (SHOW_SHDR_2 == ENABLE)
					DBG_DUMP("send shdr_j=%d, i=%d: pxlfmt=%08x, addr[0]=%08x\r\n", p_ctx->ctrl.shdr_j, 0, p_vdoframe->pxlfmt, p_vdoframe->addr[0]);
#endif
			evt.rev = 0;
			unl_cpu(flags);
		}
		kr = ctl_ipp_ioctl(p_ctx->dev_handle, CTL_IPP_IOCTL_SNDEVT, (void *)&evt);
#if (SHOW_SHDR_2 == ENABLE)
			DBG_DUMP("curr shdr_i=%d\r\n", p_ctx->ctrl.shdr_i);
#endif
	} else
#endif

	{
		//trigger process with data (from input queue : j)
		CTL_IPP_EVT evt;
		unsigned long flags;
		loc_cpu(flags);
		if (p_ctx->inq.input_data == 0) {
			unl_cpu(flags);
			_vdoprc_iport_canceldata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_OPEN_YET);
			return ISF_ERR_NOT_OPEN_YET; //force abort
		} else {
			p_input_data    = &p_ctx->inq.input_data[j];
			p_input_data->func = 1; //start proc
			evt.buf_id = j;
			evt.data_addr = (UINT32)(VDO_FRAME *)(p_input_data->desc);
			evt.rev = 0;
			unl_cpu(flags);
		}
		//trigger
#if (USE_VPE == ENABLE)
		if (p_ctx->vpe_mode) {
			//CTL_VPE_EVT is the same with CTL_IPP_EVT
			kr = ctl_vpe_ioctl(p_ctx->dev_handle, CTL_VPE_IOCTL_SNDEVT, (void *)&evt);
			//after trigger, check result to handle current data
			if (kr != CTL_VPE_E_OK) {
				switch(kr) {
				case CTL_VPE_E_ID: ///< not open, or already close
					_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_OPEN_YET);
					return ISF_ERR_NOT_OPEN_YET;
				case CTL_VPE_E_QOVR: ///< queue full
					_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL);
					return ISF_ERR_QUEUE_FULL;
				case CTL_VPE_E_PAR: 	///< illegal parameter
					_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_ERR, ISF_ERR_PARAM_NOT_SUPPORT);
					return ISF_ERR_PARAM_NOT_SUPPORT;
				default:
					_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_ERR, ISF_ERR_FATAL);
					return (kr-9000);
				}
			}
			return ISF_OK;
		} else
#endif
#if (USE_ISE == ENABLE)
		if (p_ctx->ise_mode) {
			//CTL_ISE_EVT is the same with CTL_IPP_EVT
			kr = ctl_ise_ioctl(p_ctx->dev_handle, CTL_ISE_IOCTL_SNDEVT, (void *)&evt);
			//after trigger, check result to handle current data
			if (kr != CTL_ISE_E_OK) {
				switch(kr) {
				case CTL_ISE_E_ID: ///< not open, or already close
					_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_OPEN_YET);
					return ISF_ERR_NOT_OPEN_YET;
				case CTL_ISE_E_QOVR: ///< queue full
					_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL);
					return ISF_ERR_QUEUE_FULL;
				case CTL_ISE_E_PAR: 	///< illegal parameter
					_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_ERR, ISF_ERR_PARAM_NOT_SUPPORT);
					return ISF_ERR_PARAM_NOT_SUPPORT;
				default:
					_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_ERR, ISF_ERR_FATAL);
					return (kr-9000);
				}
			}
			return ISF_OK;
		} else
#endif
		{
			kr = ctl_ipp_ioctl(p_ctx->dev_handle, CTL_IPP_IOCTL_SNDEVT, (void *)&evt);
		}
	}

#if (USE_IN_ONEBUF == ENABLE)
	//after trigger, check func to handle current data
	if (p_ctx->cur_in_cfg_func & VDOPRC_IFUNC_ONEBUF) { //under "single buffer mode"
		///< drop it before process
		_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_REL, ISF_OK);
		j = 0xff; //mark it as "already release"
	}
#endif

	//after trigger, check result to handle current data
	if (kr != CTL_IPP_E_OK) {
		switch(kr) {
		case CTL_IPP_E_ID: ///< not open, or already close
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_OPEN_YET);
			return ISF_ERR_NOT_OPEN_YET;
		case CTL_IPP_E_STATE: ///< not start, already stop
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_START);
			return ISF_ERR_NOT_START;
		case CTL_IPP_E_QOVR: ///< queue full
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL);
			return ISF_ERR_QUEUE_FULL;
		case CTL_IPP_E_PAR: 	///< illegal parameter
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_ERR, ISF_ERR_PARAM_NOT_SUPPORT);
			return ISF_ERR_PARAM_NOT_SUPPORT;
		default:
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PUSH_ERR, ISF_ERR_FATAL);
			return (kr-9000);
		}
	}

	return ISF_OK;
}

void _isf_vdoprc_iport_do_push_fail(ISF_UNIT *p_thisunit, UINT32 iport, UINT32 buf_handle, UINT32 probe, UINT32 r)
{
	UINT32 j = buf_handle;
	//UINT32 i = iport - ISF_IN_BASE;
	//ISF_PORT *p_src = p_thisunit->port_in[i];
	_vdoprc_iport_dropdata(p_thisunit, iport, j, probe, r);
}

void _isf_vdoprc_iport_do_proc_cb(ISF_UNIT *p_thisunit, UINT32 iport, UINT32 buf_handle, UINT32 event, INT32 kr)
{
	#ifdef ISF_TS
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	#endif
	if (event == CTL_IPP_CBEVT_IN_BUF_PROCSTART) {
		#ifdef ISF_TS
		UINT32 j = buf_handle;
		if (j != 0xff) {
			ISF_DATA* p_data    = &p_ctx->inq.input_data[j];
			p_data->ts_vprc_in[1] = hwclock_get_longcounter();
		}
		#endif
		p_thisunit->p_base->do_probe(p_thisunit, iport, ISF_IN_PROBE_PUSH, ISF_OK);
		p_thisunit->p_base->do_probe(p_thisunit, iport, ISF_IN_PROBE_PROC, ISF_ENTER);
	}
	if (event == CTL_IPP_CBEVT_IN_BUF_DROP) {
		UINT32 j = buf_handle;
		switch(kr) {
		case CTL_IPP_E_ID: ///< not open, or already close
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_DROP, ISF_ERR_NOT_OPEN_YET);
			break;
		case CTL_IPP_E_STATE: ///< not start, already stop
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_DROP, ISF_ERR_NOT_START);
			break;
		case CTL_IPP_E_FLUSH: ///< force drop
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_DROP, ISF_ERR_FLUSH_DROP);
			break;
		case CTL_IPP_E_INDATA: ///< data error
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_WRN, ISF_ERR_DATA_NOT_SUPPORT);
			break;
		case CTL_IPP_E_QOVR: ///< queue full
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_WRN, ISF_ERR_QUEUE_FULL);
			break;
		case CTL_IPP_E_PAR: 	///< illegal parameter
		case CTL_IPP_E_KDRV_SET:
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_WRN, ISF_ERR_PARAM_NOT_SUPPORT);
			break;
		case CTL_IPP_E_SYS: ///< process fail
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL);
			break;
		case CTL_IPP_E_KDRV_STRP: ///< kdrv stripe cal error
		case CTL_IPP_E_KDRV_TRIG: ///< kdrv trigger error
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL);
			break;
		case CTL_IPP_E_TIMEOUT: ///< process timeout
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_ERR, ISF_ERR_WAIT_TIMEOUT);
			break;
		default:
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_ERR, ISF_ERR_FATAL);
			break;
		}
	}
	if (event == CTL_IPP_CBEVT_IN_BUF_PROCEND) {
		UINT32 j = buf_handle;
		#ifdef ISF_TS
		if (j != 0xff) {
			ISF_DATA* p_data    = &p_ctx->inq.input_data[j];
			p_data->ts_vprc_in[2] = hwclock_get_longcounter();
			p_data->ts_vprc_in[3] = hwclock_get_longcounter();
		}
		#endif
		//UINT32 i = iport - ISF_IN_BASE;
		//ISF_PORT *p_src = p_thisunit->port_in[i];
		p_thisunit->p_base->do_probe(p_thisunit, iport, ISF_IN_PROBE_PROC, ISF_OK);
		if (j != 0xff) {
			p_thisunit->p_base->do_probe(p_thisunit, iport, ISF_IN_PROBE_REL, ISF_ENTER);
			_vdoprc_iport_releasedata(p_thisunit, iport, j, ISF_IN_PROBE_REL, ISF_OK);
		}
	}
}

#if (USE_IN_DIRECT == ENABLE)
void _isf_vdoprc_iport_do_cap_cb(ISF_UNIT *p_thisunit, UINT32 iport, UINT32 event, void* evt)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	if ((event == CTL_IPP_CBEVT_IN_BUF_DIR_DROP) || (event == CTL_IPP_CBEVT_IN_BUF_DIR_PROCEND)) {
		SIE_UNLOCK_CB fp = p_ctx->sie_unl_cb; //keep this in local to avoid it is clear by unbind.

		#if 0//(USE_EIS == ENABLE)
		if (_vprc_eis_plugin_ && p_ctx->eis_func) {
			CTL_IPP_DIR_EVT *p_info = (CTL_IPP_DIR_EVT *)evt;
			VDO_FRAME *p_vdo_frm = (VDO_FRAME *)p_info->reserved;
			VDOPRC_EIS_PROC_INFO proc_info;
			UINT32  data_num = 0, data_size;
			ULONG rsv1 = p_vdo_frm->reserved[1];
			INT32 *agyro_x;
			INT32 *agyro_y;
			INT32 *agyro_z;
			INT32 *ags_x;
			INT32 *ags_y;
			INT32 *ags_z;
			UINT64 t_diff_crop;
			ULONG gyro_addr;

			//DBG_DUMP("id=%X, data_addr=0x%X, frame=%llu rsv1=0x%X\r\n", p_info->buf_id, p_info->data_addr, p_vdo_frm->count, rsv1);
			if (rsv1 && (*(UINT32 *)(rsv1))) {
				data_num = *(UINT32 *)(rsv1);
				t_diff_crop =  *(UINT32 *)(rsv1 + sizeof(data_num));
				gyro_addr = rsv1 + sizeof(data_num) + sizeof(t_diff_crop);
				agyro_x =  (INT32 *)(gyro_addr);
				agyro_y =  (INT32 *)(gyro_addr + (1 * data_num * sizeof(INT32)));
				agyro_z =  (INT32 *)(gyro_addr + (2 * data_num * sizeof(INT32)));
				ags_x = (INT32 *)(gyro_addr + (3 * data_num * sizeof(INT32)));
				ags_y = (INT32 *)(gyro_addr + (4 * data_num * sizeof(INT32)));
				ags_z = (INT32 *)(gyro_addr + (5 * data_num * sizeof(INT32)));

				p_ctx->p_eis_info_ctx->frame_count = p_vdo_frm->count;
				p_ctx->p_eis_info_ctx->t_diff_crop = t_diff_crop;
				if (data_num > MAX_GYRO_DATA_NUM) {
					DBG_ERR("Gyro data_num(%d) > %d\r\n", data_num, MAX_GYRO_DATA_NUM);
				} else {
					isf_vdoprc_put_gyro_frame_cnt(p_ctx->p_eis_info_ctx->frame_count);
					data_size = data_num*sizeof(UINT32);
					memcpy((void *)p_ctx->p_eis_info_ctx->angular_rate_x, agyro_x, data_size);
					memcpy((void *)p_ctx->p_eis_info_ctx->angular_rate_y, agyro_y, data_size);
					memcpy((void *)p_ctx->p_eis_info_ctx->angular_rate_z, agyro_z, data_size);
					memcpy((void *)p_ctx->p_eis_info_ctx->acceleration_rate_x, ags_x, data_size);
					memcpy((void *)p_ctx->p_eis_info_ctx->acceleration_rate_y, ags_y, data_size);
					memcpy((void *)p_ctx->p_eis_info_ctx->acceleration_rate_z, ags_z, data_size);
					_vprc_eis_plugin_->set(VDOPRC_EIS_PARAM_TRIG_PROC, &proc_info);

					#if 0//for debug only
					{
						INT32 *p_data;
						DBG_DUMP("data_num=%d\r\n", data_num);
						p_data = p_ctx->p_eis_info_ctx->angular_rate_x;
						DBG_DUMP("angular_rate_x\r\n");
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
						p_data = p_ctx->p_eis_info_ctx->angular_rate_y;
						DBG_DUMP("angular_rate_y\r\n");
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
						p_data = p_ctx->p_eis_info_ctx->angular_rate_z;
						DBG_DUMP("angular_rate_y\r\n");
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
						p_data = p_ctx->p_eis_info_ctx->acceleration_rate_x;
						DBG_DUMP("acceleration_rate_x\r\n");
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
						p_data = p_ctx->p_eis_info_ctx->acceleration_rate_y;
						DBG_DUMP("acceleration_rate_y\r\n");
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
						p_data = p_ctx->p_eis_info_ctx->acceleration_rate_z;
						DBG_DUMP("acceleration_rate_z\r\n");
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																		*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					}
					#endif
				}
			}
		}
		#endif

		if (fp != 0) {
			ISF_PORT *p_iport = p_thisunit->port_in[0];
			ISF_UNIT *p_srcunit = 0;
			if (p_iport) {
				p_srcunit = p_iport->p_srcunit; //keep this in local to avoid it is clear by unbind.
				if (p_srcunit)
					(fp)(p_srcunit, ISF_OUT(0), evt);
			}
		}
	}
}
#endif


