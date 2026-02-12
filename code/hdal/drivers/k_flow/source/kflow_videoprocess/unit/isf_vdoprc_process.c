#include "isf_vdoprc_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoprc_p
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoprc_p_debug_level = NVT_DBG_WRN;
//module_param_named(isf_vdoprc_p_debug_level, isf_vdoprc_p_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_vdoprc_p_debug_level, "vdoprc_p debug level");
///////////////////////////////////////////////////////////////////////////////

void _isf_vprc_set_func_allow(ISF_UNIT *p_thisunit)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

	p_ctx->func_allow = 0;

	switch (p_ctx->new_mode) {
	case VDOPRC_PIPE_RAWALL:
		#if defined(_BSP_NA51000_)
		p_ctx->func_allow |= VDOPRC_FUNC_WDR|VDOPRC_FUNC_SHDR|VDOPRC_FUNC_DEFOG|VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->func_allow |= VDOPRC_FUNC_WDR|VDOPRC_FUNC_SHDR|VDOPRC_FUNC_DEFOG|VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT|VDOPRC_FUNC_3DNR_STA|VDOPRC_FUNC_VA_SUBOUT;
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->func_allow |= VDOPRC_FUNC_WDR|VDOPRC_FUNC_SHDR|VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT|VDOPRC_FUNC_3DNR_STA|VDOPRC_FUNC_VA_SUBOUT;
		#endif
		break;
	case VDOPRC_PIPE_RAWCAP: //running 2 times of RAWALL to gen (1) sub out (2) ime out
		#if defined(_BSP_NA51000_)
		p_ctx->func_allow |= VDOPRC_FUNC_WDR|VDOPRC_FUNC_SHDR|VDOPRC_FUNC_DEFOG|VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->func_allow |= VDOPRC_FUNC_WDR|VDOPRC_FUNC_SHDR|VDOPRC_FUNC_DEFOG|VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT|VDOPRC_FUNC_3DNR_STA|VDOPRC_FUNC_VA_SUBOUT;
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->func_allow |= VDOPRC_FUNC_WDR|VDOPRC_FUNC_SHDR|VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT|VDOPRC_FUNC_3DNR_STA|VDOPRC_FUNC_VA_SUBOUT;
		#endif
		break;

/*
	case VDOPRC_PIPE_RAWALL|VDOPRC_PIPE_WARP_360:
		#if defined(_BSP_NA51000_)
		p_ctx->func_allow |= (VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK);
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->func_allow |= VDOPRC_FUNC_WDR;
		#endif
		break;
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
	case VDOPRC_PIPE_YUVALL:
		#if defined(_BSP_NA51000_)
		p_ctx->func_allow |= VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->func_allow |= VDOPRC_FUNC_WDR|VDOPRC_FUNC_DEFOG|VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT|VDOPRC_FUNC_3DNR_STA|VDOPRC_FUNC_VA_SUBOUT;
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->func_allow |= VDOPRC_FUNC_WDR|VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT|VDOPRC_FUNC_3DNR_STA|VDOPRC_FUNC_VA_SUBOUT;
		#endif
		break;
	case VDOPRC_PIPE_YUVCAP: //running 2 times of RAWALL to gen (1) sub out (2) ime out
		#if defined(_BSP_NA51000_)
		p_ctx->func_allow |= VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->func_allow |= VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT|VDOPRC_FUNC_3DNR_STA|VDOPRC_FUNC_VA_SUBOUT;
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->func_allow |= VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT|VDOPRC_FUNC_3DNR_STA|VDOPRC_FUNC_VA_SUBOUT;
		#endif
		break;
	case VDOPRC_PIPE_SCALE:
		#if defined(_BSP_NA51000_)
		p_ctx->func_allow |= VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->func_allow |= VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT|VDOPRC_FUNC_3DNR_STA|VDOPRC_FUNC_VA_SUBOUT;
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->func_allow |= VDOPRC_FUNC_3DNR|VDOPRC_FUNC_DATASTAMP|VDOPRC_FUNC_PRIMASK|VDOPRC_FUNC_PM_PIXELIZTION|VDOPRC_FUNC_YUV_SUBOUT|VDOPRC_FUNC_3DNR_STA|VDOPRC_FUNC_VA_SUBOUT;
		#endif
		break;
	case VDOPRC_PIPE_COLOR:
		#if defined(_BSP_NA51000_)
		p_ctx->func_allow = 0;
		#endif
		#if defined(_BSP_NA51055_) 
		p_ctx->func_allow = VDOPRC_FUNC_DEFOG;
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->func_allow = 0;
		#endif
		break;
	case VDOPRC_PIPE_WARP:
		#if defined(_BSP_NA51000_)
		p_ctx->func_allow = 0;
		#endif
		#if defined(_BSP_NA51055_)
		p_ctx->func_allow = VDOPRC_FUNC_WDR;
		#endif
		#if defined(_BSP_NA51089_)
		p_ctx->func_allow = VDOPRC_FUNC_WDR;
		#endif
		break;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
	default:
		break;
	}
}

ISF_RV _isf_vprc_check_func(ISF_UNIT *p_thisunit)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	{
		UINT32 func = p_ctx->ctrl.func_max;
		UINT32 func_not_allow = 0;
		
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "max_func=%08x", func);
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, 
			"max_func(%08x) ... WDR=%d SHDR=%d DEFOG=%d 3DNR=%d 3DNR_STA=%d DATASTAMP=%d, PRIMASK=%d, MOSAIC=%d YUV_SUBOUT=%d VA_SUBOUT=%d",
				func,
				(func & VDOPRC_FUNC_WDR)!=0,
				(func & VDOPRC_FUNC_SHDR)!=0,
				(func & VDOPRC_FUNC_DEFOG)!=0,
				(func & VDOPRC_FUNC_3DNR)!=0,
				(func & VDOPRC_FUNC_3DNR_STA)!=0,
				(func & VDOPRC_FUNC_DATASTAMP)!=0,
				(func & VDOPRC_FUNC_PRIMASK)!=0,
				(func & VDOPRC_FUNC_PM_PIXELIZTION)!=0,
				(func & VDOPRC_FUNC_YUV_SUBOUT)!=0,
				(func & VDOPRC_FUNC_VA_SUBOUT)!=0
				);

		func_not_allow = func;
		func_not_allow &= ~(p_ctx->func_allow);
		if (func_not_allow != 0) {
			DBG_ERR("in[0].max_func=%08x, but some func is not support by pipe=%08x!\r\n", func, p_ctx->new_mode);
			DBG_ERR("not support => WDR=%d SHDR=%d DEFOG=%d 3DNR=%d 3DNR_STA=%d DATASTAMP=%d, PRIMASK=%d, MOSAIC=%d YUV_SUBOUT=%d VA_SUBOUT=%d\r\n",
				(func_not_allow & VDOPRC_FUNC_WDR)!=0,
				(func_not_allow & VDOPRC_FUNC_SHDR)!=0,
				(func_not_allow & VDOPRC_FUNC_DEFOG)!=0,
				(func_not_allow & VDOPRC_FUNC_3DNR)!=0,
				(func_not_allow & VDOPRC_FUNC_3DNR_STA)!=0,
				(func_not_allow & VDOPRC_FUNC_DATASTAMP)!=0,
				(func_not_allow & VDOPRC_FUNC_PRIMASK)!=0,
				(func_not_allow & VDOPRC_FUNC_PM_PIXELIZTION)!=0,
				(func_not_allow & VDOPRC_FUNC_YUV_SUBOUT)!=0,
				(func_not_allow & VDOPRC_FUNC_VA_SUBOUT)!=0
				);
			return ISF_ERR_FAILED;
		}
		
	}
	return ISF_OK;
}


///////////////////////////////////////////////////////////////////////////////

void _vdoprc_max_func(ISF_UNIT *p_thisunit, CTL_IPP_FUNC* p_func)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	CTL_IPP_FUNC func;
	UINT32 user_cfg = _isf_vdoprc_get_cfg();
	{
		//set max function
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "max_func=%08x", p_ctx->ctrl.func_max);
#if _TODO
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "max_func2=%08x", p_ctx->ctrl.func2_max);
#endif
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		func = (p_ctx->ctrl.func_max | VDOPRC_FUNC_DATASTAMP | VDOPRC_FUNC_PRIMASK);
		{
			UINT32 stripe_rule = user_cfg & HD_VIDEOPROC_CFG_STRIP_MASK; //get stripe rule
			if (stripe_rule > 3) {
				stripe_rule = 0;
			}
			if ((stripe_rule == 1) || (stripe_rule == 2)) { //if disable GDC
				//DBG_DUMP("vprc disable GDC!\r\n");
				func &= ~VDOPRC_FUNC_GDC; //not allow enable GDC
			} else {
				//DBG_DUMP("vprc enable GDC!\r\n");
				func |= VDOPRC_FUNC_GDC; //allow enable GDC
			}
		}
#else
		func = (p_ctx->ctrl.func_max | VDOPRC_FUNC_DATASTAMP | VDOPRC_FUNC_PRIMASK);
#endif
		if (p_func) {
			p_func[0] = func;
		}
	}
}

ISF_RV _vdoprc_config_func(ISF_UNIT *p_thisunit, BOOL en)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	CTL_IPP_FUNC func;
#if _TODO
	UINT32 func2;
#endif
	UINT32 user_cfg = _isf_vdoprc_get_cfg();
/*
[RHE]
	CTL_IPP_FUNC_WDR				= 0x00000001,
	CTL_IPP_FUNC_SHDR				= 0x00000002,
	CTL_IPP_FUNC_DEFOG			= 0x00000004,
[IFE]
	CTL_IPP_FUNC_3DNR				= 0x00000008,
[IME]
	CTL_IPP_FUNC_DATASTAMP		= 0x00000010,
	CTL_IPP_FUNC_PRIMASK 			= 0x00000020,
	CTL_IPP_FUNC_PM_PIXELIZTION 	= 0x00000040,
	CTL_IPP_FUNC_YUV_SUBOUT 		= 0x00000080,
*/
	if (en == DISABLE) {
		func = CTL_IPP_FUNC_NONE;
		ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_FUNCEN, (void *)&func);
		p_ctx->ctrl.cur_func = func; //apply
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "func=%08x", 0);
#if _TODO
		func2 = 0;
		p_ctx->ctrl.cur_func2 = func2; //apply
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "func2=%08x", 0);
#endif
	} else {
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		func = (p_ctx->ctrl.func | VDOPRC_FUNC_DATASTAMP | VDOPRC_FUNC_PRIMASK);
		{
			UINT32 stripe_rule = user_cfg & HD_VIDEOPROC_CFG_STRIP_MASK; //get stripe rule
			if (stripe_rule > 3) {
				stripe_rule = 0;
			}
			if ((stripe_rule == 1) || (stripe_rule == 2)) { //if disable GDC
				//DBG_DUMP("vprc disable GDC!\r\n");
				func &= ~VDOPRC_FUNC_GDC; //not allow enable GDC
			} else {
				//DBG_DUMP("vprc enable GDC!\r\n");
				func |= VDOPRC_FUNC_GDC; //allow enable GDC
			}
		}
#else
		func = (p_ctx->ctrl.func | VDOPRC_FUNC_DATASTAMP | VDOPRC_FUNC_PRIMASK);
#endif
		ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_FUNCEN, (void *)&func);
		p_ctx->ctrl.cur_func = func; //apply
#if (USE_NEW_SHDR == ENABLE)
		if (p_ctx->ctrl.func_max & VDOPRC_FUNC_SHDR) {
			p_ctx->ctrl.shdr_j = 0;
			p_ctx->ctrl.shdr_i = 0;
		}
#endif
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "func=%08x", func);
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, 
			"func(%08x) ... WDR=%d SHDR=%d DEFOG=%d 3DNR=%d 3DNR_STA=%d DATASTAMP=%d, PRIMASK=%d, MOSAIC=%d YUV_SUBOUT=%d VA_SUBOUT=%d",
				func,
				(func & VDOPRC_FUNC_WDR)!=0,
				(func & VDOPRC_FUNC_SHDR)!=0,
				(func & VDOPRC_FUNC_DEFOG)!=0,
				(func & VDOPRC_FUNC_3DNR)!=0,
				(func & VDOPRC_FUNC_3DNR_STA)!=0,
				(func & VDOPRC_FUNC_DATASTAMP)!=0,
				(func & VDOPRC_FUNC_PRIMASK)!=0,
				(func & VDOPRC_FUNC_PM_PIXELIZTION)!=0,
				(func & VDOPRC_FUNC_YUV_SUBOUT)!=0,
				(func & VDOPRC_FUNC_VA_SUBOUT)!=0
				);
#if _TODO
		func2 = p_ctx->ctrl.func2;
		p_ctx->ctrl.cur_func2 = func2; //apply
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "func2=%08x", func2);
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, 
			"func2(%08x) ... TRIGGERSINGLE=%d MD=%d%d%d VT=%d%d%d%d DIRECT=%d DROPOLDSIZE=%d",
				func2,
				(func2 & VDOPRC_FUNC2_TRIGGERSINGLE)!=0,
				(func2 & VDOPRC_FUNC2_MOTIONDETECT1)!=0,
				(func2 & VDOPRC_FUNC2_MOTIONDETECT2)!=0,
				(func2 & VDOPRC_FUNC2_MOTIONDETECT3)!=0,
				(func2 & VDOPRC_FUNC2_VIEWTRACKING1)!=0,
				(func2 & VDOPRC_FUNC2_VIEWTRACKING2)!=0,
				(func2 & VDOPRC_FUNC2_VIEWTRACKING3)!=0,
				(func2 & VDOPRC_FUNC2_VIEWTRACKING4)!=0,
				(func2 & VDOPRC_FUNC2_DIRECT)!=0,
				(func2 & VDOPRC_FUNC2_DROPOLDSIZE)!=0
				);
#endif
	}

/*
	ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_SCL_METHOD, (void *)&p_ctx->ctrl.scale);
	p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "scale(quality_th=%08x, h=%d, l=%d)",
		p_ctx->ctrl.scale.scl_th, p_ctx->ctrl.scale.method_h, p_ctx->ctrl.scale.method_l);
*/

	ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_COLORSPACE, (void *)&p_ctx->ctrl.color.space);
	p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, 	"color(space=%08x)",
		p_ctx->ctrl.color.space);

	return ISF_OK;
}

ISF_RV _vdoprc_update_func(ISF_UNIT *p_thisunit)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	CTL_IPP_FUNC func;
#if _TODO
	UINT32 func2;
#endif
	UINT32 user_cfg = _isf_vdoprc_get_cfg();

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	func = (p_ctx->ctrl.func | VDOPRC_FUNC_DATASTAMP | VDOPRC_FUNC_PRIMASK);
	{
		UINT32 stripe_rule = user_cfg & HD_VIDEOPROC_CFG_STRIP_MASK; //get stripe rule
		if (stripe_rule > 3) {
			stripe_rule = 0;
		}
		if ((stripe_rule == 1) || (stripe_rule == 2)) { //if disable GDC
			//DBG_DUMP("vprc disable GDC!\r\n");
			func &= ~VDOPRC_FUNC_GDC; //not allow enable GDC
		} else {
			//DBG_DUMP("vprc enable GDC!\r\n");
			func |= VDOPRC_FUNC_GDC; //allow enable GDC
		}
	}
#else
	func = (p_ctx->ctrl.func | VDOPRC_FUNC_DATASTAMP | VDOPRC_FUNC_PRIMASK);
#endif
#if _TODO
	func2 = p_ctx->ctrl.func2;
#endif
	if (func != p_ctx->ctrl.cur_func) {
		ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_FUNCEN, (void *)&func);
		p_ctx->ctrl.cur_func = func; //apply
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "func=%08x", func);
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, 
			"func(%08x) ... WDR=%d SHDR=%d DEFOG=%d 3DNR=%d 3DNR_STA=%d DATASTAMP=%d, PRIMASK=%d, MOSAIC=%d YUV_SUBOUT=%d VA_SUBOUT=%d",
				func,
				(func & VDOPRC_FUNC_WDR)!=0,
				(func & VDOPRC_FUNC_SHDR)!=0,
				(func & VDOPRC_FUNC_DEFOG)!=0,
				(func & VDOPRC_FUNC_3DNR)!=0,
				(func & VDOPRC_FUNC_3DNR_STA)!=0,
				(func & VDOPRC_FUNC_DATASTAMP)!=0,
				(func & VDOPRC_FUNC_PRIMASK)!=0,
				(func & VDOPRC_FUNC_PM_PIXELIZTION)!=0,
				(func & VDOPRC_FUNC_YUV_SUBOUT)!=0,
				(func & VDOPRC_FUNC_VA_SUBOUT)!=0
				);
	}
#if _TODO
	if (func2 != p_ctx->ctrl.cur_func2) {
		p_ctx->ctrl.cur_func2 = func2; //apply
		
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "func2=%08x", func2);
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, 
			"func2(%08x) ... TRIGGERSINGLE=%d MD=%d%d%d VT=%d%d%d%d DIRECT=%d DROPOLDSIZE=%d",
				func2,
				(func2 & VDOPRC_FUNC2_TRIGGERSINGLE)!=0,
				(func2 & VDOPRC_FUNC2_MOTIONDETECT1)!=0,
				(func2 & VDOPRC_FUNC2_MOTIONDETECT2)!=0,
				(func2 & VDOPRC_FUNC2_MOTIONDETECT3)!=0,
				(func2 & VDOPRC_FUNC2_VIEWTRACKING1)!=0,
				(func2 & VDOPRC_FUNC2_VIEWTRACKING2)!=0,
				(func2 & VDOPRC_FUNC2_VIEWTRACKING3)!=0,
				(func2 & VDOPRC_FUNC2_VIEWTRACKING4)!=0,
				(func2 & VDOPRC_FUNC2_DIRECT)!=0,
				(func2 & VDOPRC_FUNC2_DROPOLDSIZE)!=0
				);
	}
#endif

/*
	ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_SCL_METHOD, (void *)&p_ctx->ctrl.scale);
	p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "scale(quality_th=%08x, h=%d, l=%d)",
		p_ctx->ctrl.scale.scl_th, p_ctx->ctrl.scale.method_h, p_ctx->ctrl.scale.method_l);
*/

	ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_OUT_COLORSPACE, (void *)&p_ctx->ctrl.color.space);
	p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL,	"color(space=%08x)",
		p_ctx->ctrl.color.space);

	ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_3DNR_REFPATH_SEL, (void *)&p_ctx->ctrl._3dnr_refpath);
	p_ctx->ctrl.cur_3dnr_refpath = p_ctx->ctrl._3dnr_refpath; //apply
	p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL,	"3dnr(refpath=%d)",
		p_ctx->ctrl._3dnr_refpath);
	return ISF_OK;
}


void _isf_vdoprc_do_process(ISF_UNIT *p_thisunit, UINT32 event)
{
	switch(event) {
	case 0:
		break;
	default:
		break;
	}
}

