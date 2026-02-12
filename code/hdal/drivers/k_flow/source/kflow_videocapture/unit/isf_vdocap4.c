#include "isf_vdocap_int.h"
#include "isf_vdocap_dbg.h"


#define VDOCAP_ID    4

static ISF_PORT_CAPS isf_vdocap4_inputport1_caps = {
	.connecttype_caps = ISF_CONNECT_NONE,
	.do_push = NULL,
};
static ISF_PORT isf_vdocap4_inputport1 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(0),
	.connecttype = ISF_CONNECT_NONE,
	.p_destunit = &isf_vdocap4,
	.p_srcunit = NULL,
};
static ISF_PORT *isf_vdocap4_inputportlist[ISF_VDOCAP_IN_NUM] = {
	&isf_vdocap4_inputport1,
};
static ISF_PORT_CAPS *isf_vdocap4_inputportlist_caps[ISF_VDOCAP_IN_NUM] = {
	&isf_vdocap4_inputport1_caps,
};

static ISF_RV _isf_vdocap4_outputport1_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdocap_oqueue_do_pull(&isf_vdocap4, oport, p_data, wait_ms);
	}
	return ISF_ERR_FAILED;
}

static ISF_PORT_CAPS isf_vdocap4_outputport1_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH|ISF_CONNECT_PULL,
	.do_pull = _isf_vdocap4_outputport1_do_pull,
};


static ISF_PORT *isf_vdocap4_outputportlist[ISF_VDOCAP_OUT_NUM] = {0};
static ISF_PORT *isf_vdocap4_outputportlist_cfg[ISF_VDOCAP_OUT_NUM] = {0};
static ISF_PORT_CAPS *isf_vdocap4_outputportlist_caps[ISF_VDOCAP_OUT_NUM] = {
	&isf_vdocap4_outputport1_caps
};

static ISF_STATE isf_vdocap4_outputport1_state[ISF_VDOCAP_OUT_NUM] = {0};
static ISF_STATE *isf_vdocap4_outputportlist_state[ISF_VDOCAP_OUT_NUM] = {
	&isf_vdocap4_outputport1_state[0]
};

static ISF_INFO isf_vdocap4_outputinfo1_in = {0};
static ISF_INFO *isf_vdocap4_outputinfolist_in[ISF_VDOCAP_IN_NUM] = {
	&isf_vdocap4_outputinfo1_in
};

static ISF_INFO isf_vdocap4_outputinfo1_out = {0};
static ISF_INFO *isf_vdocap4_outputinfolist_out[ISF_VDOCAP_OUT_NUM] = {
	&isf_vdocap4_outputinfo1_out
};


#if 0
static ISF_RV isf_vdocap4_setparam(ISF_UNIT *p_thisunit, UINT32 param, UINT32 value)
{
	_isf_vdocap_setparam(0, p_thisunit, param, value);
	return ISF_OK;
}
static UINT32 isf_vdocap4_getparam(ISF_UNIT *p_thisunit, UINT32 param)
{
	return _isf_vdocap_getparam(0, p_thisunit, param);
}
#endif
static ISF_RV isf_vdocap4_setportparam(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 value)
{
	return _isf_vdocap_do_setportparam(p_thisunit, nport, param, value);
}
static UINT32 isf_vdocap4_getportparam(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param)
{
	return _isf_vdocap_do_getportparam(p_thisunit, nport, param);
}
static ISF_RV isf_vdocap4_setportstruct(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 *p_struct, UINT32 size)
{
	return _isf_vdocap_do_setportstruct(p_thisunit, nport, param, p_struct, size);
}
static ISF_RV isf_vdocap4_getportstruct(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 *p_struct, UINT32 size)
{
	return _isf_vdocap_do_getportstruct(p_thisunit, nport, param, p_struct, size);
}
static ISF_RV isf_vdocap4_updateport(ISF_UNIT *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd)
{
	return _isf_vdocap_updateport(p_thisunit, oport, cmd);
}

ISF_UNIT isf_vdocap4;
ISF_PORT_PATH isf_vdocap4_pathlist[ISF_VDOCAP_PATH_NUM] = {
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdocap4, ISF_IN(0), ISF_OUT(0)},
};


static INT32 _vdocap4_sie_isr_cb(UINT32 msg, void *p_in, void *p_out)
{
	return _vdocap_sie_isr_cb(&isf_vdocap4, msg, p_in, p_out);
}


static INT32 _vdocap4_buf_io_cb(UINT32 sts, void *p_in, void *p_out)
{
	UINT32 path = 0;
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(isf_vdocap4.refdata);

	if (p_ctx->started == 0) {
		DBG_MSG("vdocap%d has stopped. Ignore cb(0x%X)\r\n", p_ctx->id, sts);
		return 1;
	}
	//shdr flow
	if ((p_ctx->shdr_map != VDOCAP_SEN_HDR_NONE) && (_vdocap_shdr_frm_num[_vdocap_shdr_map_to_set(p_ctx->shdr_map)] > 1)) {
		ISF_UNIT *p_main_unit;

		p_main_unit = _vdocap_shdr_main_unit[_vdocap_shdr_map_to_set(p_ctx->shdr_map)];

		if ( NULL == p_main_unit) {
			DBG_ERR("SHDR map(%d) error\r\n", p_ctx->shdr_map);
			return 1;
		}
		if (sts == CTL_SIE_BUF_IO_NEW) {
			UINT32 buf_size = *(UINT32 *)p_in;

			_isf_vdocap_shdr_oport_do_new(p_main_unit, ISF_OUT(0) + path, buf_size, 0, p_out, p_ctx->shdr_map);
		} else if (sts == CTL_SIE_BUF_IO_PUSH) {
			_isf_vdocap_shdr_oport_do_push(p_main_unit, ISF_OUT(0) + path, p_in, p_ctx->shdr_map);
		} else if (sts == CTL_SIE_BUF_IO_UNLOCK) {
			_isf_vdocap_shdr_oport_do_lock(p_main_unit, ISF_OUT(0) + path, p_in, 0, p_ctx->shdr_map);
		} else if (sts == CTL_SIE_BUF_IO_LOCK) {
			_isf_vdocap_shdr_oport_do_lock(p_main_unit, ISF_OUT(0) + path, p_in, 1, p_ctx->shdr_map);
		}

	} else {
		//ben to do, use sensor mode to distinquish output path
		if (sts == CTL_SIE_BUF_IO_NEW) {
			UINT32 buf_size = *(UINT32 *)p_in;

			_isf_vdocap_oport_do_new(&isf_vdocap4, ISF_OUT(0) + path, buf_size, 0, p_out);//ben to do, g_ddr = ?
		} else if (sts == CTL_SIE_BUF_IO_PUSH) {
			_isf_vdocap_oport_do_push(&isf_vdocap4, ISF_OUT(0) + path, p_in);
		} else if (sts == CTL_SIE_BUF_IO_UNLOCK) {
			_isf_vdocap_oport_do_lock(&isf_vdocap4, ISF_OUT(0) + path, p_in, 0);
		} else if (sts == CTL_SIE_BUF_IO_LOCK) {
			_isf_vdocap_oport_do_lock(&isf_vdocap4, ISF_OUT(0) + path, p_in, 1);
		}
	}
	return 1;
}
static ISF_RV isf_vdocap4_init(struct _ISF_UNIT *p_thisunit, void* p_ctx_buf)
{
	p_thisunit->refdata = p_ctx_buf;
	if (p_thisunit->refdata) {
		VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);

		memset((void*)p_ctx, 0, sizeof(VDOCAP_CONTEXT));
		p_ctx->id = VDOCAP_ID;
		p_ctx->sie_id = VDOCAP_ID;
		p_ctx->sie_isr_cb = _vdocap4_sie_isr_cb;
		p_ctx->buf_io_cb = _vdocap4_buf_io_cb;
		p_ctx->shdr_map = VDOCAP_SEN_HDR_NONE;
		p_ctx->flow_type = CTL_SIE_FLOW_SEN_IN;
		p_ctx->chgsenmode_info.sel = CTL_SEN_MODESEL_AUTO;
		p_ctx->chgsenmode_info.auto_info.frame_rate = 3000;
		p_ctx->chgsenmode_info.auto_info.size.w = 1920;
		p_ctx->chgsenmode_info.auto_info.size.h = 1080;
		p_ctx->chgsenmode_info.auto_info.frame_num = VDOCAP_SEN_FRAME_NUM_1;
		p_ctx->chgsenmode_info.auto_info.data_fmt = CTL_SEN_DATA_FMT_RGB;
		p_ctx->chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_8BIT;
		p_ctx->out_dest = CTL_SIE_OUT_DEST_DRAM;
		p_ctx->flip = CTL_SIE_FLIP_NONE;
		return ISF_OK;
	}
	return ISF_ERR_NULL_POINTER;
}
static ISF_RV isf_vdocap4_uninit(struct _ISF_UNIT *p_thisunit)
{
	p_thisunit->refdata = NULL;
	return ISF_OK;
}
ISF_UNIT isf_vdocap4 = {
	.sign = ISF_SIGN_UNIT,
	.unit_name = "vdocap4",
	.unit_module = MAKE_NVTMPP_MODULE('v', 'd', 'o', 'c', 'a', 'p', 0, 0),
	.port_in_count = ISF_VDOCAP_IN_NUM,
	.port_out_count = ISF_VDOCAP_OUT_NUM,
	.port_path_count = ISF_VDOCAP_PATH_NUM,
	.port_in = isf_vdocap4_inputportlist,
	.port_out = isf_vdocap4_outputportlist,
	.port_outcfg = isf_vdocap4_outputportlist_cfg,
	.port_incaps = isf_vdocap4_inputportlist_caps,
	.port_outcaps = isf_vdocap4_outputportlist_caps,
	.port_outstate = isf_vdocap4_outputportlist_state,
	.port_ininfo = isf_vdocap4_outputinfolist_in,
	.port_outinfo = isf_vdocap4_outputinfolist_out,
	.port_path = isf_vdocap4_pathlist,
	.do_init = isf_vdocap4_init,
	.do_uninit = isf_vdocap4_uninit,
	.do_bindinput = NULL,
	.do_bindoutput = _isf_vdocap_bindouput,
	.do_setportparam = isf_vdocap4_setportparam,
	.do_getportparam = isf_vdocap4_getportparam,
	.do_setportstruct = isf_vdocap4_setportstruct,
	.do_getportstruct = isf_vdocap4_getportstruct,
	.do_update = isf_vdocap4_updateport,
	.refdata = NULL,
};

///////////////////////////////////////////////////////////////////////////////

