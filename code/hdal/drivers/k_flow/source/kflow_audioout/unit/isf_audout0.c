#include "isf_audout_int.h"

#define AUDOUT_NOTIFY_FUNC DISABLE
#define ISF_AUDOUT_IN_NUM           1
#define ISF_AUDOUT_OUT_NUM          1

//static BOOL audout0_cb(PAUDIO_BUF_QUEUE pAudBufQue, PAUDIO_BUF_QUEUE pAudBufQue_AEC, UINT32 id, UINT64 timestamp);
//static void audout0_evtcb(WAVSTUD_ACT act, UINT32 p1, UINT32 p2);
static ISF_RV isf_audout0_inputport_push_imgbuf_to_dest(ISF_PORT *p_port, ISF_DATA *p_data, INT32 nWaitMs);

static ISF_PORT_CAPS isf_audout0_inputport1_caps = {
	#if AUDOUT_NOTIFY_FUNC == ENABLE
	.connecttype_caps = ISF_CONNECT_PUSH | ISF_CONNECT_NOTIFY,
	#else
	.connecttype_caps = ISF_CONNECT_PUSH,
	#endif
	.do_push = isf_audout0_inputport_push_imgbuf_to_dest,
};

static ISF_PORT isf_audout0_inputport1 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(0),
	#if AUDOUT_NOTIFY_FUNC == ENABLE
	.connecttype = ISF_CONNECT_PUSH | ISF_CONNECT_NOTIFY,
	#else
	.connecttype = ISF_CONNECT_PUSH,
	#endif
	.p_destunit = &isf_audout0,
	.p_srcunit = NULL,
};


static ISF_PORT *isf_audout0_inputportlist[1] = {
	&isf_audout0_inputport1,
};
static ISF_PORT_CAPS *isf_audout0_inputportlist_caps[1] = {
	&isf_audout0_inputport1_caps,
};

static ISF_PORT_CAPS isf_audout0_outputport1_caps = {
	.connecttype_caps = ISF_CONNECT_NONE,
	.do_pull = NULL,
};

static ISF_PORT *isf_audout0_outputportlist[1] = {0};
static ISF_PORT *isf_audout0_outputportlist_cfg[1] = {0};
static ISF_PORT_CAPS *isf_audout0_outputportlist_caps[1] = {
	&isf_audout0_outputport1_caps
};

static ISF_STATE isf_audout0_outputport_state[1] = {0};
static ISF_STATE *isf_audout0_outputportlist_state[1] = {
	&isf_audout0_outputport_state[0],
};

static ISF_INFO isf_audout0_outputinfo_in = {0};
static ISF_INFO *isf_audout0_outputinfolist_in[ISF_AUDOUT_IN_NUM] = {
	&isf_audout0_outputinfo_in,
};

static ISF_INFO isf_audout0_outputinfo_out = {0};
static ISF_INFO *isf_audout0_outputinfolist_out[ISF_AUDOUT_OUT_NUM] = {
	&isf_audout0_outputinfo_out,
};

static ISF_RV _isf_audout0_bindinput(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport)
{
	return _isf_audout_bindinput(p_thisunit, iport, p_srcunit, oport);
}

static ISF_RV isf_audout0_inputport_push_imgbuf_to_dest(ISF_PORT *p_port, ISF_DATA *p_data, INT32 nWaitMs)
{
	return isf_audout_inputport_push_imgbuf_to_dest(DEV_ID_0, p_port,p_data, nWaitMs);
}

static ISF_RV _isf_audout0_setportparam(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 value)
{
	return _isf_audout_setportparam(DEV_ID_0, p_thisunit, nport, param, value);
}
static UINT32 _isf_audout0_getportparam(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param)
{
	return _isf_audout_getportparam(DEV_ID_0, p_thisunit, nport, param);
}

static ISF_RV _isf_audout0_setportstruct(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 *p_struct, UINT32 size){
	return _isf_audout_setportstruct(DEV_ID_0, p_thisunit, nport, param, p_struct, size);
}

static ISF_RV _isf_audout0_getportstruct(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 *p_struct, UINT32 size)
{
	return _isf_audout_getportstruct(DEV_ID_0, p_thisunit, nport, param, p_struct, size);
}

static ISF_RV _isf_audout0_updateport(struct _ISF_UNIT *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd)
{
	return _isf_audout_updateport(DEV_ID_0, p_thisunit, oport, cmd);
}

static ISF_RV _isf_audout0_init(struct _ISF_UNIT *p_thisunit, void* p_ctx_buf)
{
	p_thisunit->refdata = p_ctx_buf;
	if (p_thisunit->refdata) {
		AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
		memset((void*)p_ctx, 0, sizeof(AUDOUT_CONTEXT_DEV));

		p_ctx->wsis.obj_count = 1;
		p_ctx->wsis.data_mode			= WAVSTUD_DATA_MODE_PUSH;
		p_ctx->wsis.aud_info.aud_sr		= AUDIO_SR_48000;
		p_ctx->wsis.aud_info.aud_ch		= AUDIO_CH_STEREO;
		p_ctx->wsis.aud_info.bitpersample = 16;
		p_ctx->wsis.aud_info.ch_num		= 2;
		p_ctx->wsis.aud_info.buf_sample_cnt = 1024;
		p_ctx->wsis.buf_count			= 10;
		p_ctx->wsis.unit_time			= 1;

		p_ctx->wsis_max.obj_count			= 1,
		p_ctx->wsis_max.data_mode			= WAVSTUD_DATA_MODE_PUSH;
		p_ctx->wsis_max.aud_info.aud_sr		= AUDIO_SR_48000;
		p_ctx->wsis_max.aud_info.aud_ch		= AUDIO_CH_STEREO;
		p_ctx->wsis_max.aud_info.bitpersample = 16;
		p_ctx->wsis_max.aud_info.ch_num		= 2;
		p_ctx->wsis_max.aud_info.buf_sample_cnt = 1024;
		p_ctx->wsis_max.buf_count			= 10;
		p_ctx->wsis_max.unit_time			= 1;

		p_ctx->g_volume = 100;
		p_ctx->g_isf_audout_lpf_en = 0;
		p_ctx->isf_audout_mono_ch = AUDIO_CH_LEFT;
		p_ctx->g_isf_audout_max_frame_sample = 1024;
		p_ctx->g_isf_audout_buf_cnt = 10;

		return ISF_OK;
	}
	return ISF_ERR_NULL_POINTER;
}
static ISF_RV _isf_audout0_uninit(struct _ISF_UNIT *p_thisunit)
{
	p_thisunit->refdata = NULL;
	return ISF_OK;
}

#if 0
BOOL audout0_cb(PAUDIO_BUF_QUEUE pAudBufQue, PAUDIO_BUF_QUEUE pAudBufQue_AEC, UINT32 id, UINT64 timestamp)
{
	if (g_dbgmsg) {
		DBG_MSG("No data buffer\r\n");
	}
	if(1) {
		//gen a dummy fail log
		isf_audout0.p_base->do_release(&isf_audout0, ISF_IN(0), NULL, 0, ISF_ERR_QUEUE_EMPTY);
	}

	return FALSE;
}

void audout0_evtcb(WAVSTUD_ACT act, UINT32 p1, UINT32 p2)
{
	ISF_PORT *p_port = isf_audout.p_base->iport(&isf_audout, ISF_IN(0));

	//if (_isf_unit_is_allow_notify(p_port)/* && p_port->p_srcunit == &ISF_AudDec*/) {
		switch (p1) {
		case WAVSTUD_CB_EVT_BUF_DONE:
				//g_isf_audout_noti_data.Event.Event = AUDOUT_EVT_BUF_DONE;
				//DBG_IND("AudOut send event=%d\r\n", g_isf_audout_noti_data.Event.Event);
				isf_audout.p_base->do_notify(&isf_audout, p_port, &g_isf_audout_noti_data, 0, ISF_OK, p_port);
				break;
		case WAVSTUD_CB_EVT_PRELOAD_DONE:
				//g_isf_audout_noti_data.Event.Event = AUDOUT_EVT_PRELOAD_DONE;
				//DBG_IND("AudOut send event=%d\r\n", g_isf_audout_noti_data.Event.Event);
				isf_audout.p_base->do_notify(&isf_audout, p_port, &g_isf_audout_noti_data, 0, ISF_OK, p_port);
				break;
		default:
			break;
		}
	//}
}
#endif

///////////////////////////////////////////////////////////////////////////////

ISF_PORT_PATH isf_audout0_pathlist[8] = {
	{&isf_audout0, ISF_IN(0), ISF_OUT(0)},
	{&isf_audout0, ISF_IN(0), ISF_OUT(0)},
	{&isf_audout0, ISF_IN(0), ISF_OUT(0)},
	{&isf_audout0, ISF_IN(0), ISF_OUT(0)},
	{&isf_audout0, ISF_IN(0), ISF_OUT(0)},
	{&isf_audout0, ISF_IN(0), ISF_OUT(0)},
	{&isf_audout0, ISF_IN(0), ISF_OUT(0)},
	{&isf_audout0, ISF_IN(0), ISF_OUT(0)}
};

ISF_UNIT isf_audout0 = {
	.sign = ISF_SIGN_UNIT,
	.unit_name = "audout0",
	.unit_module = MAKE_NVTMPP_MODULE('a', 'u', 'd', 'o', 'u', 't', 0, 0),
	.list_id = 1, // 1=not support Multi-Process
	.port_in_count = 1,
	.port_out_count = 1,
	.port_path_count = 8,
	.port_in = isf_audout0_inputportlist,
	.port_out = isf_audout0_outputportlist,
	.port_outcfg = isf_audout0_outputportlist_cfg,
	.port_incaps = isf_audout0_inputportlist_caps,
	.port_outcaps = isf_audout0_outputportlist_caps,
	.port_outstate = isf_audout0_outputportlist_state,
	.port_ininfo = isf_audout0_outputinfolist_in,
	.port_outinfo = isf_audout0_outputinfolist_out,
	.port_path = isf_audout0_pathlist,
	.do_bindinput = _isf_audout0_bindinput,
	.do_bindoutput = NULL,
	.do_setportparam = _isf_audout0_setportparam,
	.do_getportparam = _isf_audout0_getportparam,
	.do_setportstruct = _isf_audout0_setportstruct,
	.do_getportstruct = _isf_audout0_getportstruct,
	.do_update = _isf_audout0_updateport,
	.do_command = _isf_audout_do_command,
	.do_init = _isf_audout0_init,
	.do_uninit = _isf_audout0_uninit,
	.refdata = NULL,
};
///////////////////////////////////////////////////////////////////////////////



