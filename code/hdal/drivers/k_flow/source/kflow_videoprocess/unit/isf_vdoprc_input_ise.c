#include "isf_vdoprc_int.h"
//#include "nvtmpp.h"

#if (USE_ISE == ENABLE)
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoprc_ise
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoprc_ii_debug_level = NVT_DBG_WRN;
//module_param_named(isf_vdoprc_ii_debug_level, isf_vdoprc_ii_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_vdoprc_ii_debug_level, "vdoprc1 debug level");


///////////////////////////////////////////////////////////////////////////////

#define WATCH_PORT			ISF_IN(0)
#define FORCE_DUMP_DATA		DISABLE


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
static void _isf_vdoprc_ise_push_dummy_queue_for_all_output(ISF_UNIT *p_thisunit)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 i;
	ISF_DATA *p_data = NULL;
	static ISF_DATA_CLASS g_vdoprc_out_data = {0};
	UINT32 oport;
	VDO_FRAME *p_vdoframe;
	//only for ISE mode
	if (FALSE == p_ctx->ise_mode) {
		return;
	}
	//force to return all pull_out
	for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++) {
		if (p_ctx->user_out_blk[i]) {
			ISF_DATA isf_data = {0};

			oport = ISF_OUT(i);
			p_data = &isf_data;
			p_thisunit->p_base->init_data(p_data, &g_vdoprc_out_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
			p_data->h_data = p_ctx->user_out_blk[i];
			p_vdoframe = (VDO_FRAME *)(p_data->desc);
			p_vdoframe->reserved[2] = MAKEFOURCC('D', 'R', 'O', 'P');

			DBG_DUMP("%s drop all out[%d]\r\n", p_thisunit->unit_name, (int)i);
			_isf_vdoprc_oqueue_do_push_wait(p_thisunit, oport, p_data, USER_OUT_BUFFER_QUEUE_TIMEOUT);
		}
	}
}

ISF_RV _vdoprc_config_ise_in_crop(ISF_UNIT *p_thisunit, UINT32 iport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_INFO *p_src_info = p_thisunit->port_ininfo[iport - ISF_IN_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
	CTL_ISE_IN_CROP in_crop = {0};

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
			p_ctx->in[0].crop.mode = CTL_ISE_IN_CROP_USER;
			p_ctx->in[0].crop.crp_window.x = p_vdoinfo->window.x;
			p_ctx->in[0].crop.crp_window.y = p_vdoinfo->window.y;
			p_ctx->in[0].crop.crp_window.w = p_vdoinfo->window.w;
			p_ctx->in[0].crop.crp_window.h = p_vdoinfo->window.h;
			p_thisunit->p_base->do_trace(p_thisunit, iport, ISF_OP_PARAM_VDOFRM, "-in:crop win=(%d,%d,%d,%d)",
				p_vdoinfo->window.x,
				p_vdoinfo->window.y,
				p_vdoinfo->window.w,
				p_vdoinfo->window.h);

			in_crop.mode = p_ctx->in[0].crop.mode;
			in_crop.crp_window.x = p_ctx->in[0].crop.crp_window.x;
			in_crop.crp_window.y = p_ctx->in[0].crop.crp_window.y;
			in_crop.crp_window.w = p_ctx->in[0].crop.crp_window.w;
			in_crop.crp_window.h = p_ctx->in[0].crop.crp_window.h;

		} else {
			//input crop is off == full frame
			p_ctx->in[0].crop.mode = CTL_IPP_IN_CROP_NONE;
			p_ctx->in[0].crop.crp_window = zero_crop;
			in_crop.mode = p_ctx->in[0].crop.mode;
			p_thisunit->p_base->do_trace(p_thisunit, iport, ISF_OP_PARAM_VDOFRM, "-in:crop win=(0,0,full,full)");
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
		in_crop.mode = p_ctx->in[0].crop.mode;
		p_thisunit->p_base->do_trace(p_thisunit, iport, ISF_OP_PARAM_VDOFRM, "-in:crop win=(-,-,-,-)");
	}
	ctl_ise_set(p_ctx->dev_handle, CTL_ISE_ITEM_IN_CROP, (void *)&in_crop);
	return ISF_OK;
}

void _isf_vdoprc_ise_iport_do_proc_cb(ISF_UNIT *p_thisunit, UINT32 iport, UINT32 buf_handle, UINT32 event, INT32 kr)
{
	#ifdef ISF_TS
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	#endif
	//DBG_DUMP("%s[in]event=%d, err_msg=%d \r\n", p_thisunit->unit_name, event, kr);

	if (event == CTL_ISE_BUF_UNLOCK) {
		UINT32 j = buf_handle;
		switch(kr) {
		case CTL_ISE_E_OK:
			p_thisunit->p_base->do_probe(p_thisunit, iport, ISF_IN_PROBE_PROC, ISF_OK);
			if (j != 0xff) {
				p_thisunit->p_base->do_probe(p_thisunit, iport, ISF_IN_PROBE_REL, ISF_ENTER);
				_vdoprc_iport_releasedata(p_thisunit, iport, j, ISF_IN_PROBE_REL, ISF_OK);
			}
			break;
		case CTL_ISE_E_DROP_INPUT_ONLY: ///< not open, or already close
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_DROP, ISF_ERR_NOT_OPEN_YET);
			_isf_vdoprc_ise_push_dummy_queue_for_all_output(p_thisunit);
			break;
		#if 0//TBD
		case CTL_IPP_E_STATE: ///< not start, already stop
			_vdoprc_ise_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_DROP, ISF_ERR_NOT_START);
			break;
		case CTL_IPP_E_FLUSH: ///< force drop
			_vdoprc_ise_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_DROP, ISF_ERR_FLUSH_DROP);
			break;
		case CTL_IPP_E_INDATA: ///< data error
			_vdoprc_ise_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_WRN, ISF_ERR_DATA_NOT_SUPPORT);
			break;
		case CTL_IPP_E_QOVR: ///< queue full
			_vdoprc_ise_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_WRN, ISF_ERR_QUEUE_FULL);
			break;
		case CTL_IPP_E_PAR: 	///< illegal parameter
		case CTL_IPP_E_KDRV_SET:
			_vdoprc_ise_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_WRN, ISF_ERR_PARAM_NOT_SUPPORT);
			break;
		case CTL_IPP_E_SYS: ///< process fail
			_vdoprc_ise_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL);
			break;
		case CTL_IPP_E_KDRV_STRP: ///< kdrv stripe cal error
		case CTL_IPP_E_KDRV_TRIG: ///< kdrv trigger error
			_vdoprc_ise_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_ERR, ISF_ERR_PROCESS_FAIL);
			break;
		case CTL_IPP_E_TIMEOUT: ///< process timeout
			_vdoprc_ise_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_ERR, ISF_ERR_WAIT_TIMEOUT);
			break;
		#endif
		default:
			_vdoprc_iport_dropdata(p_thisunit, iport, j, ISF_IN_PROBE_PROC_ERR, ISF_ERR_FATAL);
			break;
		}
	} else {
		//TBD
	}
}
#endif
