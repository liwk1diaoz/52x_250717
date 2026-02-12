#include "isf_audout_int.h"

#if defined (DEBUG) && defined(__FREERTOS)
unsigned int isf_audout_debug_level = THIS_DBGLVL;
#endif

#define AUDOUT_NOTIFY_FUNC DISABLE
#define ISF_AUDOUT_IN_NUM           1
#define ISF_AUDOUT_OUT_NUM          1
#define Perf_GetCurrent(x)  hwclock_get_longcounter(x)

#define ISF_UNIT_TRACE(opclass, p_thisunit, port, fmtstr, args...) (p_thisunit)->p_base->do_trace(p_thisunit, port, opclass, fmtstr, ##args)

#define AUDOUT_DYNAMIC_CONTEXT DISABLE
#if 0
static WAVSTUD_APPOBJ wso[DEV_NUM] = {0};
static WAVSTUD_INFO_SET wsis[DEV_NUM] = {
{
	.obj_count			= 1,
	.data_mode			= WAVSTUD_DATA_MODE_PUSH,

	.aud_info.aud_sr		= AUDIO_SR_48000,
	.aud_info.aud_ch		= AUDIO_CH_STEREO,
	.aud_info.bitpersample = 16,
	.aud_info.ch_num		= 2,
	.aud_info.buf_sample_cnt     = 1024,

	.buf_count			= 10,
	.unit_time			= 1,
},
{
	.obj_count			= 1,
	.data_mode			= WAVSTUD_DATA_MODE_PUSH,

	.aud_info.aud_sr		= AUDIO_SR_48000,
	.aud_info.aud_ch		= AUDIO_CH_STEREO,
	.aud_info.bitpersample = 16,
	.aud_info.ch_num		= 2,
	.aud_info.buf_sample_cnt     = 1024,

	.buf_count			= 10,
	.unit_time			= 1,
}
};
static WAVSTUD_INFO_SET wsis_max[DEV_NUM] = {
{
	.obj_count			= 1,
	.data_mode			= WAVSTUD_DATA_MODE_PUSH,

	.aud_info.aud_sr		= AUDIO_SR_48000,
	.aud_info.aud_ch		= AUDIO_CH_STEREO,
	.aud_info.bitpersample = 16,
	.aud_info.ch_num		= 2,
	.aud_info.buf_sample_cnt     = 1024,

	.buf_count			= 10,
	.unit_time			= 1,
},
{
	.obj_count			= 1,
	.data_mode			= WAVSTUD_DATA_MODE_PUSH,

	.aud_info.aud_sr		= AUDIO_SR_48000,
	.aud_info.aud_ch		= AUDIO_CH_STEREO,
	.aud_info.bitpersample = 16,
	.aud_info.ch_num		= 2,
	.aud_info.buf_sample_cnt     = 1024,

	.buf_count			= 10,
	.unit_time			= 1,
}
};
#endif

static BOOL audout_cb(PAUDIO_BUF_QUEUE pAudBufQue, PAUDIO_BUF_QUEUE pAudBufQue_AEC, UINT32 id, UINT64 timestamp, UINT64 timestamp_aec);
static void _audout_evtcb(WAVSTUD_ACT act, UINT32 p1, UINT32 p2);
static BOOL _isf_audout_src_check(UINT32 id, UINT32 resample);

//static MEM_RANGE g_isf_audout_maxmem = {0};

static BOOL   g_bisf_audout_trace_on     = FALSE;
//static UINT32 g_isf_audout_codec   		 = 0;

static BOOL   g_isf_audout_outdev         = 0;
static BOOL   g_isf_audout_evt_en          = FALSE;
static INT32  isf_audout_prepwr_en = 0;

#if 0
static UINT32 g_volume = 100;

static ISF_DATA g_isf_audout_memblk[DEV_NUM] = {0};
static BOOL   g_isf_audout_allocmem[DEV_NUM] = {0};
static BOOL   g_isf_audout_lpf_en[DEV_NUM] = {0, 0};
static AUDIO_CH isf_audout_mono_ch[DEV_NUM] = {AUDIO_CH_LEFT, AUDIO_CH_RIGHT};
static UINT32 g_isf_audout_max_frame_sample[DEV_NUM]  = {1024, 1024};
static UINT32 g_isf_audout_buf_cnt[DEV_NUM] = {10, 10};

static AUDIO_SR               isf_audout_resample_max[DEV_NUM] = {0};
static AUDIO_SR               isf_audout_resample[DEV_NUM] = {0};
static AUDIO_SR               isf_audout_resample_update[DEV_NUM] = {0};
static BOOL                   isf_audout_resample_dirty[DEV_NUM] = {0};
static UINT32                 isf_audout_resample_in_frame[DEV_NUM] = {0};
static UINT32                 isf_audout_resample_out_frame[DEV_NUM] = {0};
static BOOL                   isf_audout_resample_en[DEV_NUM] = {0};
static UINT32                 isf_audout_resample_last_remain_in[DEV_NUM] = {0};
static UINT32                 isf_audout_resample_last_remain_out[DEV_NUM] = {0};
static int                    resample_handle_play[DEV_NUM] = {0};
static MEM_RANGE              g_isf_audout_srcmem[DEV_NUM] = {0};
static MEM_RANGE              g_isf_audout_srcbufmem[DEV_NUM] = {0};
static ISF_DATA               isf_audout_srcmemblk[DEV_NUM] = {0};
static ISF_DATA               isf_audout_srcbufmemblk[DEV_NUM] = {0};
static UINT32                 isf_audout_started[DEV_NUM] = {0};
static UINT32                 isf_audout_opened[DEV_NUM] = {0};
SEM_HANDLE                    ISF_AUDOUT_BUF_SEM_ID[DEV_NUM] = {0};
#endif

SEM_HANDLE                    ISF_AUDOUT_PROC_SEM_ID = {0};
static KDRV_AUDIOLIB_FUNC*    paudsrc_obj   = NULL;
static KDRV_AUDIOLIB_FUNC*    audfilt_kdrv_obj   = NULL;
static PAUDOUT_AUDFILT_OBJ    paudfilt_obj = NULL;

static AUDOUT_CONTEXT g_ao_ctx = {0};

#if AUDOUT_DYNAMIC_CONTEXT == FALSE
static AUDOUT_CONTEXT_DEV g_ao_ctx_dev[DEV_NUM] = {0};
#endif

UINT32 g_audout_max_count = 0;
ISF_UNIT *g_audout_list[DEV_NUM] = {
	&isf_audout0,
	&isf_audout1,
};

static UINT32 g_audout_init[ISF_FLOW_MAX] = {0};

#if defined (__KERNEL__)
static BOOL isf_audout_fastboot = TRUE;
#endif

#include "kdrv_audioio/audlib_src.h"

#include "kdrv_audioio/audlib_filt.h"

AUDFILT_CONFIG  lowpass_config = {
        AUDFILT_ORDER(2),                      // Filter Order
        65536,       // Total Gain
        // Filter Coeficients
        {
                {119328559, 86531115, 29243089, 134217728, 73734090, 27150946},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0},
                {0, 0, 0, 0, 0, 0}
        },
        // Section Gain
        {
                65536,
                0,
                0,
                0,
                0
        }
};

static BOOL isf_audout_filt_open(UINT32 uiChannels, BOOL bSmooth)
{
	AUDFILT_INIT    AudFilterInit;

	audfilt_kdrv_obj = kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID_FILT);

	if (audfilt_kdrv_obj == NULL) {
		DBG_ERR("filter obj is NULL\r\n");
		return FALSE;
	}

	if (audfilt_kdrv_obj->filter.close== NULL) {
		DBG_ERR("close is NULL\r\n");
		return FALSE;
	}

	if (audfilt_kdrv_obj->filter.open== NULL) {
		DBG_ERR("open is NULL\r\n");
		return FALSE;
	}

	if (audfilt_kdrv_obj->filter.init== NULL) {
		DBG_ERR("init is NULL\r\n");
		return FALSE;
	}

	if (uiChannels > 2) { // max support 2 channels
		uiChannels = 2;
	}

	AudFilterInit.filt_ch       = uiChannels;
	AudFilterInit.smooth_enable = bSmooth;

	audfilt_kdrv_obj->filter.close();

	// open audio filter
	if (!audfilt_kdrv_obj->filter.open(&AudFilterInit)) {
		DBG_ERR("Audio filter open err!\r\n");
		return FALSE;
	}

	// init audio filter
	if (!audfilt_kdrv_obj->filter.init()) {
		DBG_ERR("Audio filter init err!\r\n");
		return FALSE;
	}

	return TRUE;
}

static BOOL isf_audout_filt_close(void)
{
	if (audfilt_kdrv_obj == NULL) {
		DBG_ERR("filter obj is NULL\r\n");
		return FALSE;
	}

	if (audfilt_kdrv_obj->filter.close== NULL) {
		DBG_ERR("close is NULL\r\n");
		return FALSE;
	}

	return audfilt_kdrv_obj->filter.close();
}

static void isf_audout_filt_design(void)
{
	if (audfilt_kdrv_obj == NULL) {
		DBG_ERR("filter obj is NULL\r\n");
		return;
	}

	if (audfilt_kdrv_obj->filter.set_config== NULL) {
		DBG_ERR("set_config is NULL\r\n");
		return;
	}

	if (audfilt_kdrv_obj->filter.enable_filt== NULL) {
		DBG_ERR("enable_filt is NULL\r\n");
		return;
	}

	audfilt_kdrv_obj->filter.set_config(AUDFILT_SEL_FILT0, &lowpass_config);
	audfilt_kdrv_obj->filter.enable_filt(AUDFILT_SEL_FILT0, TRUE);

	return;
}

static BOOL isf_audout_filt_apply(UINT32 uiInAddr, UINT32 uiOutAddr, UINT32 uiSize)
{
	AUDFILT_BITSTREAM   AudBitStream;
	BOOL    bStatus;

	if (audfilt_kdrv_obj == NULL) {
		return FALSE;
	}

	if (audfilt_kdrv_obj->filter.run == NULL) {
		return FALSE;
	}

	AudBitStream.bitstram_buffer_in     = uiInAddr;
	AudBitStream.bitstram_buffer_out    = uiOutAddr;
	AudBitStream.bitstram_buffer_length = uiSize;

	// apply audio filter
	if (!(bStatus = audfilt_kdrv_obj->filter.run(&AudBitStream))) {
		DBG_ERR("Audio filtering err!\r\n");
	}

	return bStatus;
}

AUDOUT_AUDFILT_OBJ audfilt_obj = {isf_audout_filt_open,
                                 isf_audout_filt_close,
                                 isf_audout_filt_apply,
                                 isf_audout_filt_design
                                };



#if defined __UITRON || defined __ECOS
static BOOL   g_audoutcmd_installed      = FALSE;

static BOOL _isf_audout_sxcmd_otrace(CHAR *strCmd)
{
	g_bisf_audout_trace_on = TRUE;
	return TRUE;
}

static BOOL _isf_audout_sxcmd_ctrace(CHAR *strCmd)
{
	g_bisf_audout_trace_on = FALSE;
	return TRUE;
}

static SXCMD_BEGIN(isfao, "ISF_AudOut command")
SXCMD_ITEM("ot", _isf_audout_sxcmd_otrace, "Trace code open")
SXCMD_ITEM("ct", _isf_audout_sxcmd_ctrace, "Trace code cancel")
SXCMD_END()

static void _isf_audout_install_cmd(void)
{
	SxCmd_AddTable(isfao);
}
#endif

/*static ISF_PORT_CAPS isf_audout_inputport1_caps = {
	#if AUDOUT_NOTIFY_FUNC == ENABLE
	.connecttype_caps = ISF_CONNECT_PUSH | ISF_CONNECT_NOTIFY,
	#else
	.connecttype_caps = ISF_CONNECT_PUSH,
	#endif
	.do_push = isf_audout_inputport_push_imgbuf_to_dest,
};

static ISF_PORT isf_audout_inputport1 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(0),
	#if AUDOUT_NOTIFY_FUNC == ENABLE
	.connecttype = ISF_CONNECT_PUSH | ISF_CONNECT_NOTIFY,
	#else
	.connecttype = ISF_CONNECT_PUSH,
	#endif
	.p_destunit = &isf_audout,
	.p_srcunit = NULL,
};


static ISF_PORT *isf_audout_inputportlist[1] = {
	&isf_audout_inputport1,
};
static ISF_PORT_CAPS *isf_audout_inputportlist_caps[1] = {
	&isf_audout_inputport1_caps,
};

static ISF_PORT_CAPS isf_audout_outputport1_caps = {
	.connecttype_caps = ISF_CONNECT_NONE,
	.do_pull = NULL,
};

static ISF_PORT *isf_audout_outputportlist[1] = {0};
static ISF_PORT *isf_audout_outputportlist_cfg[1] = {0};
static ISF_PORT_CAPS *isf_audout_outputportlist_caps[1] = {
	&isf_audout_outputport1_caps
};

static ISF_STATE isf_audout_outputport_state[1] = {0};
static ISF_STATE *isf_audout_outputportlist_state[1] = {
	&isf_audout_outputport_state[0],
};

static ISF_INFO isf_audout_outputinfo_in = {0};
static ISF_INFO *isf_audout_outputinfolist_in[ISF_AUDOUT_IN_NUM] = {
	&isf_audout_outputinfo_in,
};

static ISF_INFO isf_audout_outputinfo_out = {0};
static ISF_INFO *isf_audout_outputinfolist_out[ISF_AUDOUT_OUT_NUM] = {
	&isf_audout_outputinfo_out,
};*/

void isf_audout_install_id(void)
{
	UINT32 i;

	OS_CONFIG_SEMPHORE(ISF_AUDOUT_PROC_SEM_ID, 0, 1, 1);
	for (i = 0; i < DEV_MAX_COUNT; i++) {
		ISF_UNIT* p_thisunit = DEV_UNIT(i);
		AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
		OS_CONFIG_SEMPHORE(p_ctx->ISF_AUDOUT_BUF_SEM_ID, 0, 0, 0);
		OS_CONFIG_SEMPHORE(p_ctx->ISF_AUDOUT_SIZE_SEM_ID, 0, 0, 0);
	}
}

void isf_audout_uninstall_id(void)
{
	UINT32 i;

	SEM_DESTROY(ISF_AUDOUT_PROC_SEM_ID);
	for (i = 0; i < DEV_MAX_COUNT; i++) {
		ISF_UNIT* p_thisunit = DEV_UNIT(i);
		AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
		SEM_DESTROY(p_ctx->ISF_AUDOUT_BUF_SEM_ID);
		SEM_DESTROY(p_ctx->ISF_AUDOUT_SIZE_SEM_ID);
	}
}


static ISF_RV _isf_audout_update_port_info(UINT32 id, ISF_UNIT *p_thisunit, UINT32 oport)
{
	ISF_INFO *p_out_info = p_thisunit->port_outinfo[0];
	ISF_AUD_INFO *p_audinfo_out = (ISF_AUD_INFO *)(p_out_info);

	ISF_INFO *p_in_info = p_thisunit->port_ininfo[0];
	ISF_AUD_INFO *p_audinfo_in = (ISF_AUD_INFO *)(p_in_info);

	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);

	if (!p_audinfo_out) {
		DBG_ERR("p_audinfo_out is NULL\r\n");
		return ISF_ERR_NULL_POINTER;
	}

	if (p_audinfo_out->dirty & ISF_INFO_DIRTY_AUDMAX) {
		p_ctx->wsis_max.aud_info.bitpersample = p_audinfo_out->max_bitpersample;
		p_ctx->wsis_max.aud_info.ch_num = AUD_CHANNEL_COUNT(p_audinfo_out->max_channelcount);
		p_ctx->wsis_max.aud_info.aud_sr = p_audinfo_out->max_samplepersecond;
		p_ctx->wsis_max.aud_info.aud_ch = (p_ctx->wsis_max.aud_info.ch_num == 2)? AUDIO_CH_STEREO : AUDIO_CH_RIGHT;
		p_audinfo_out->dirty &= ~ISF_INFO_DIRTY_AUDMAX;
		DBG_IND("update port max SR=%d,CH=%d,bps=%d\r\n", p_ctx->wsis_max.aud_info.aud_sr, p_ctx->wsis_max.aud_info.ch_num, p_ctx->wsis_max.aud_info.bitpersample);
		ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM_IMM, p_thisunit, ISF_IN(0), "set max sr(%d)\r\n", p_ctx->wsis_max.aud_info.aud_sr);
		ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM, p_thisunit, ISF_IN(0), "set max channel(%d), ch_num(%d), bitpersample(%d)", p_ctx->wsis_max.aud_info.aud_ch, p_ctx->wsis_max.aud_info.ch_num, p_ctx->wsis_max.aud_info.bitpersample);
	}

	if (p_audinfo_out->dirty & ISF_INFO_DIRTY_AUDBITS) {
		p_ctx->wsis.aud_info.bitpersample = p_audinfo_out->bitpersample;
		p_audinfo_out->dirty &= ~ISF_INFO_DIRTY_AUDBITS;
		DBG_IND("update port bps=%d\r\n", p_ctx->wsis.aud_info.bitpersample);
		ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM, p_thisunit, ISF_IN(0), "set bitpersample(%d)", p_ctx->wsis.aud_info.bitpersample);
	}
	if (p_audinfo_out->dirty & ISF_INFO_DIRTY_AUDCH) {
		p_ctx->wsis.aud_info.ch_num = p_audinfo_out->channelcount;
		p_audinfo_out->dirty &= ~ISF_INFO_DIRTY_AUDCH;
		DBG_IND("update port CH=%d\r\n", p_ctx->wsis.aud_info.ch_num);
		ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM, p_thisunit, ISF_IN(0), "set ch_num(%d)", p_ctx->wsis.aud_info.ch_num);
	}
	if (p_audinfo_out->dirty & ISF_INFO_DIRTY_SAMPLERATE) {
		p_ctx->wsis.aud_info.aud_sr = p_audinfo_out->samplepersecond;
		p_audinfo_out->dirty &= ~ISF_INFO_DIRTY_SAMPLERATE;
		DBG_IND("update port SR=%d\r\n", p_ctx->wsis.aud_info.aud_sr);
		ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM_IMM, p_thisunit, ISF_IN(0), "set sr(%d)", p_ctx->wsis.aud_info.aud_sr);
	}

	/*update input info*/
	if (!p_audinfo_in) {
		DBG_ERR("p_audinfo_in is NULL\r\n");
		return ISF_ERR_NULL_POINTER;
	}

	if (p_audinfo_in->dirty & ISF_INFO_DIRTY_AUDMAX) {
		p_ctx->isf_audout_resample_max = p_audinfo_in->max_samplepersecond;
		p_audinfo_in->dirty &= ~ISF_INFO_DIRTY_AUDMAX;
		if (p_ctx->isf_audout_resample_max != 0) {
			paudsrc_obj = kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID_SRC);
			if (paudsrc_obj == NULL) {
				DBG_ERR("src obj is NULL\r\n");
			}
		} else {
			paudsrc_obj = 0;
		}
		DBG_IND("output update port max SR=%d\r\n", p_ctx->isf_audout_resample_max);
		ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM_IMM, p_thisunit, ISF_IN(0), "set max resample sr(%d)", p_ctx->isf_audout_resample_max);
	}

	if (p_audinfo_in->dirty & ISF_INFO_DIRTY_SAMPLERATE) {
		p_ctx->isf_audout_resample = p_audinfo_in->samplepersecond;
		p_audinfo_in->dirty &= ~ISF_INFO_DIRTY_SAMPLERATE;
		DBG_IND("output update port SR=%d\r\n", p_ctx->isf_audout_resample);
		ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM_IMM, p_thisunit, ISF_IN(0), "set resample sr(%d)", p_ctx->isf_audout_resample);
	}

	return ISF_OK;
}

ISF_RV _isf_audout_bindinput(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport)
{

	return ISF_OK;
}

static UINT32 _isf_audout_get_src_config(UINT32 *sr_in, UINT32 *sr_out)
{
	UINT32 in_sample = 0;
	UINT32 out_sample = 0;

	switch (*sr_in) {
		case AUDIO_SR_11025:
			in_sample = *sr_in*4/100;
			out_sample = *sr_out*4/100;
			break;
		case AUDIO_SR_22050:
			switch (*sr_out) {
				case AUDIO_SR_11025:
					in_sample = *sr_in*4/100;
					out_sample = *sr_out*4/100;
					break;
				default:
					in_sample = *sr_in*2/100;
					out_sample = *sr_out*2/100;
					break;
			}
			break;
		default:
			switch (*sr_out) {
				case AUDIO_SR_11025:
					in_sample = *sr_in*4/100;
					out_sample = *sr_out*4/100;
					break;

				case AUDIO_SR_22050:
					in_sample = *sr_in*2/100;
					out_sample = *sr_out*2/100;
					break;

				default:
					in_sample = *sr_in/100;
					out_sample = *sr_out/100;
					break;
			}
	}

	*sr_in = in_sample;
	*sr_out = out_sample;

	return 0;
}

static BOOL _isf_audout_src_proc(int handle, UINT32 id, UINT32 addr, UINT32 input, UINT32 output)
{
	UINT32 buf_in, buf_out;
	ISF_UNIT* p_thisunit = DEV_UNIT(id);
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	UINT32 slice = p_ctx->isf_audout_resample_in_frame;
	//UINT32 proc_size = 0;
	UINT32 one_frame = p_ctx->isf_audout_resample_out_frame;
	UINT32 push_size;
	UINT32 remain = output;
	BOOL   ret = 0;
	UINT32 remain_in = input;
	WAVSTUD_ACT act = (id == DEV_ID_0)? WAVSTUD_ACT_PLAY : WAVSTUD_ACT_PLAY2;

	if (paudsrc_obj == NULL) {
		//DBG_ERR("AudSrc object is NULL\r\n");
		return FALSE;
	}

	if (one_frame > (p_ctx->g_isf_audout_srcbufmem.size - p_ctx->isf_audout_resample_in_frame)) {
		//DBG_ERR("Buf is not enough. Need %x, but %x\r\n", one_frame, g_isf_audout_srcbufmem[id].size);
		return FALSE;
	}

	buf_in  = addr;
	buf_out = p_ctx->g_isf_audout_srcbufmem.addr + p_ctx->isf_audout_resample_in_frame;

	//resample remained data first
	if (p_ctx->isf_audout_resample_last_remain_in != 0) {
		UINT32 this_slice = slice - p_ctx->isf_audout_resample_last_remain_in;
		UINT32 this_frame = one_frame - p_ctx->isf_audout_resample_last_remain_out;
		UINT32 this_buf_in = p_ctx->g_isf_audout_srcbufmem.addr;

		memcpy((void*) (this_buf_in + p_ctx->isf_audout_resample_last_remain_in), (const void*) buf_in, this_slice);
		if (paudsrc_obj->src.run) {
			paudsrc_obj->src.run(handle, (void *) this_buf_in, (void *) buf_out);
		}

		push_size = one_frame;

		ret = wavstudio_push_buf_to_queue(buf_out, &push_size);

		if (!ret) {
			//DBG_ERR("Push failed\r\n");
			return FALSE;
		}

		buf_in += this_slice;
		remain_in -= this_slice;
		remain -= this_frame;

		p_ctx->isf_audout_resample_last_remain_in = 0;
		p_ctx->isf_audout_resample_last_remain_out = 0;
	}

	do {
		if (remain_in < slice) {
			memcpy((void*) p_ctx->g_isf_audout_srcbufmem.addr, (const void*) buf_in, remain_in);
			p_ctx->isf_audout_resample_last_remain_in = remain_in;
			p_ctx->isf_audout_resample_last_remain_out = remain;
			break;
		} else {
			push_size = one_frame;
		}

		if (paudsrc_obj->src.run) {
			paudsrc_obj->src.run(handle, (void *) buf_in, (void *) buf_out);
		}

		if (act == WAVSTUD_ACT_PLAY) {
			ret = wavstudio_push_buf_to_queue(buf_out, &push_size);
		} else {
			ret = wavstudio_push_buf_to_queue2(buf_out, &push_size);
		}

		if (!ret) {
			//DBG_ERR("Push failed\r\n");
			return FALSE;
		}

		buf_in = buf_in + slice;
		remain -= push_size;
		remain_in -= slice;
	} while (remain > 0);

	return TRUE;
}

static BOOL _isf_audout_src_check(UINT32 id, UINT32 resample)
{
	ISF_UNIT* p_thisunit = DEV_UNIT(id);
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	UINT32 output_frame = p_ctx->wsis.aud_info.aud_sr;
	UINT32 input_frame = resample;

	if ((resample == 0) || (p_ctx->wsis.aud_info.aud_sr == resample)) {
		p_ctx->isf_audout_resample_en = FALSE;
		return TRUE;
	}

	_isf_audout_get_src_config(&input_frame, &output_frame);

	p_ctx->isf_audout_resample_in_frame = input_frame*p_ctx->wsis.aud_info.ch_num*(p_ctx->wsis.aud_info.bitpersample >> 3);
	p_ctx->isf_audout_resample_out_frame = output_frame*p_ctx->wsis.aud_info.ch_num*(p_ctx->wsis.aud_info.bitpersample >> 3);

	if (paudsrc_obj) {
		if (paudsrc_obj->src.pre_init) {
			UINT32 need = paudsrc_obj->src.pre_init(p_ctx->wsis.aud_info.ch_num, input_frame, output_frame, FALSE);

			if (p_ctx->g_isf_audout_srcmem.size < need) {
				DBG_ERR("resample mem not enough need=%x but=%x\r\n", need, p_ctx->g_isf_audout_srcmem.size);
				p_ctx->isf_audout_resample_en = FALSE;
			} else {
				if (paudsrc_obj->src.init) {
					paudsrc_obj->src.init(&(p_ctx->resample_handle_play), p_ctx->wsis.aud_info.ch_num, input_frame, output_frame, FALSE, (short *)p_ctx->g_isf_audout_srcmem.addr);
				}
				p_ctx->isf_audout_resample_en = TRUE;
			}
		}
	}

	p_ctx->isf_audout_resample_last_remain_in = 0;
	p_ctx->isf_audout_resample_last_remain_out = 0;

	return TRUE;
}

static UINT32 _isf_audout_buff_timeout_unlock(UINT32 id)
{
	ISF_UNIT* p_thisunit = DEV_UNIT(id);
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);

	// consume semaphore first
	if (SEM_WAIT_TIMEOUT(p_ctx->ISF_AUDOUT_BUF_SEM_ID, vos_util_msec_to_tick(0))) {
		//DBG_DUMP("[isf_audout][%d] Input YUV_TMOUT, Unlock, sem 0->0->1\r\n", id);
	}else{
		//DBG_DUMP("[isf_audout][%d] Input YUV_TMOUT, Unlock, sem 1->0->1\r\n", id);
	}

	// semaphore +1
	SEM_SIGNAL(p_ctx->ISF_AUDOUT_BUF_SEM_ID);

	return 0;
}

static BOOL _isf_audout_buff_timeout_retry(UINT32 id, INT32 wait_ms, UINT64 input_us)
{
	INT32 pass_time_ms = ((INT32)(Perf_GetCurrent() - input_us))/1000;
	ISF_UNIT* p_thisunit = DEV_UNIT(id);
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);

	if (wait_ms < 0) {
		int ret;
		//blocking mode
		//DBG_DUMP("[isf_audout][%d] wait_ms(blocking), resource waiting....\r\n", id);

		ret = SEM_WAIT_INTERRUPTIBLE(p_ctx->ISF_AUDOUT_BUF_SEM_ID);

		if (ret != 0) {
			return FALSE;
		}

		//DBG_DUMP("[isf_audout][%d] wait_ms(blocking), resource OK !!\r\n", id);

		if (p_ctx->isf_audout_opened == TRUE)
			return TRUE;

	} else if (pass_time_ms < wait_ms){
		// timeout mode
		if (SEM_WAIT_TIMEOUT(p_ctx->ISF_AUDOUT_BUF_SEM_ID, vos_util_msec_to_tick(wait_ms - pass_time_ms))) {
			//DBG_DUMP("[isf_audout][%d] wait_ms(%d) timeout !!\r\n", id, wait_ms);
		} else {
			//DBG_DUMP("[isf_audout][%d] wait_ms(%d), resource OK !!\r\n", id, wait_ms);

			if (p_ctx->isf_audout_opened == TRUE)
				return TRUE;
		}
	}

	return FALSE;
}

static UINT32 _isf_audout_buff_size_unlock(UINT32 id)
{
	ISF_UNIT* p_thisunit = DEV_UNIT(id);
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);

	// consume semaphore first
	if (SEM_WAIT_TIMEOUT(p_ctx->ISF_AUDOUT_SIZE_SEM_ID, vos_util_msec_to_tick(0))) {
		//DBG_DUMP("[isf_audout][%d] Input YUV_TMOUT, Unlock, sem 0->0->1\r\n", id);
	}else{
		//DBG_DUMP("[isf_audout][%d] Input YUV_TMOUT, Unlock, sem 1->0->1\r\n", id);
	}

	// semaphore +1
	SEM_SIGNAL(p_ctx->ISF_AUDOUT_SIZE_SEM_ID);

	return 0;
}

static BOOL _isf_audout_buff_size_wait(UINT32 id)
{
	ISF_UNIT* p_thisunit = DEV_UNIT(id);
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	int ret;

	ret = SEM_WAIT_INTERRUPTIBLE(p_ctx->ISF_AUDOUT_SIZE_SEM_ID);

	if (ret != 0) {
		return FALSE;
	}

	if (p_ctx->isf_audout_opened == TRUE)
		return TRUE;


	return FALSE;
}




ISF_RV isf_audout_inputport_push_imgbuf_to_dest(UINT32 id, ISF_PORT *p_port, ISF_DATA *p_data, INT32 nWaitMs)
{
	PAUD_FRAME pAudBuf = (PAUD_FRAME)p_data->desc;
	BOOL retV = FALSE;
	UINT32 size = pAudBuf->size;
	ISF_RV r = ISF_OK;
	UINT32 virt_addr = 0;
	UINT32 remain = 0;
	WAVSTUD_ACT act = (id == DEV_ID_0)? WAVSTUD_ACT_PLAY : WAVSTUD_ACT_PLAY2;
	UINT64 input_us = Perf_GetCurrent();
	ISF_UNIT* p_thisunit = DEV_UNIT(id);
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);

	p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH, ISF_ENTER);

	if (p_ctx == NULL) {
		p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_UNINIT);
		return ISF_ERR_UNINIT;
	}

	if (p_ctx->isf_audout_opened == FALSE) {
		r =  ISF_ERR_NOT_OPEN_YET;
		goto ERR_CHECK;
	}

	if(size == 0) {
		r =  ISF_ERR_INVALID_DATA;
		goto ERR_CHECK;
	}

	if (pAudBuf->addr[0] != 0) {
		virt_addr = pAudBuf->addr[0];
	} else if (pAudBuf->phyaddr[0] != 0){
		virt_addr = nvtmpp_sys_pa2va(pAudBuf->phyaddr[0]);
		if (virt_addr == 0) {
			//DBG_ERR("virt_addr is null\r\n");
			retV = FALSE;
			r =  ISF_ERR_INVALID_DATA;
			p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_INVALID_DATA);
			goto ERR_CHECK;
		}
	} else {
		//DBG_ERR("Invalid addr\r\n");
		retV = FALSE;
		r =  ISF_ERR_INVALID_DATA;
		p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_INVALID_DATA);
		goto ERR_CHECK;
	}

	if (p_ctx->isf_audout_resample_dirty) {
		//update resample sample rate
		if (paudsrc_obj && p_ctx->isf_audout_resample_en) {
			if (paudsrc_obj->src.destroy) {
				paudsrc_obj->src.destroy(p_ctx->resample_handle_play);
			}
		}
		_isf_audout_src_check(id, p_ctx->isf_audout_resample_update);
		p_ctx->isf_audout_resample = p_ctx->isf_audout_resample_update;

		p_ctx->isf_audout_resample_update = 0;
		p_ctx->isf_audout_resample_dirty = 0;
	}

	//resample
	if (p_ctx->isf_audout_resample_en){
		UINT32 outsize = size * p_ctx->wsis.aud_info.aud_sr;
		if ((outsize%p_ctx->isf_audout_resample) != 0) {
			DBG_ERR("Not supported resample size=%d\r\n", size);
			r =  ISF_ERR_INVALID_DATA;
			p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_DATA_NOT_SUPPORT);
			goto ERR_CHECK;
		}

		outsize = outsize / p_ctx->isf_audout_resample;

TRY_FIND_BUFF_SRC:
		if (p_ctx->isf_audout_opened == FALSE) {
			r =  ISF_ERR_NOT_OPEN_YET;
			goto ERR_CHECK;
		}

		if (act == WAVSTUD_ACT_PLAY) {
			remain = wavstudio_get_remain_buf(act);
		} else {
			remain = wavstudio_get_remain_buf2(act);
		}

		if ((outsize + p_ctx->isf_audout_resample_last_remain_out) > remain) {

			if (TRUE == _isf_audout_buff_timeout_retry(id, nWaitMs, input_us)) {
				goto TRY_FIND_BUFF_SRC;  // try again
			}

			if (p_ctx->isf_audout_opened == FALSE) {
				r =  ISF_ERR_NOT_OPEN_YET;
				goto ERR_CHECK;
			}

			//buffer is not enough
			r =  ISF_ERR_QUEUE_FULL;
			p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_QUEUE_FULL);
			goto ERR_CHECK;
		}

		p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH, ISF_OK);
		p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PROC, ISF_ENTER);

		retV = _isf_audout_src_proc(p_ctx->resample_handle_play, id, virt_addr, size, outsize);
	} else {

TRY_FIND_BUFF:
		if (p_ctx->isf_audout_opened == FALSE) {
			r =  ISF_ERR_NOT_OPEN_YET;
			goto ERR_CHECK;
		}

		if (act == WAVSTUD_ACT_PLAY) {
			remain = wavstudio_get_remain_buf(act);
		} else {
			remain = wavstudio_get_remain_buf2(act);
		}

		if (size > remain) {

			if (TRUE == _isf_audout_buff_timeout_retry(id, nWaitMs, input_us)) {
				goto TRY_FIND_BUFF;  // try again
			}

			if (p_ctx->isf_audout_opened == FALSE) {
				r =  ISF_ERR_NOT_OPEN_YET;
				goto ERR_CHECK;
			}

			p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_QUEUE_FULL);
			//buffer is not enough
			r =  ISF_ERR_QUEUE_FULL;
			goto ERR_CHECK;
		}
		p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PUSH, ISF_OK);
		p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PROC, ISF_ENTER);
		if (act == WAVSTUD_ACT_PLAY) {
			retV = wavstudio_push_buf_to_queue(virt_addr, &size);
		} else {
			retV = wavstudio_push_buf_to_queue2(virt_addr, &size);
		}
	}

	if (retV) {
		p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PROC, ISF_OK);
		r =  ISF_OK;
	} else {
		p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_PROC_DROP, ISF_ERR_QUEUE_FULL);
		r =  ISF_ERR_QUEUE_FULL;
	}

ERR_CHECK:

	if (!retV && g_bisf_audout_trace_on) {
		DBG_WRN("Push failed!\r\n");
	}

	/*if(size > pAudBuf->size) {
		r = ISF_ERR_QUEUE_FULL;
	} else if(!retV) {
		r = ISF_ERR_PROCESS_FAIL;
	}*/

	if (pAudBuf->addr[0] != 0) {
		p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_REL, ISF_ENTER);
		p_thisunit->p_base->do_release(p_thisunit, ISF_IN(0), p_data, ISF_OK);
		p_thisunit->p_base->do_probe(p_thisunit, ISF_IN(0), ISF_IN_PROBE_REL, ISF_OK);
	}

	return r;
}

#if 0
static ISF_RV _ISF_AudOut_UpdatePortInfo(ISF_UNIT *p_thisunit, UINT32 oport)
{
	ISF_AUD_INFO *pAudInfo = (ISF_AUD_INFO *) & (ImageUnit_In(p_thisunit, ISF_IN1)->info);

	if (!pAudInfo) {
		DBG_ERR("pAudInfo is NULL\r\n");
		return ISF_ERR_FAILED;
	}

	g_isf_audout_codec          = pAudInfo->AudFmt;
	wsis[id].audInfo.bitPerSamp  = pAudInfo->bitpersample;
	wsis[id].audInfo.ch_num      = pAudInfo->channelcount;
	wsis[id].audInfo.audSR       = pAudInfo->samplepersecond;

	DBG_IND("SR=%d,CH=%d,bps=%d,codec=%d\r\n", wsis[id].audInfo.audSR, wsis[id].audInfo.audCh, wsis[id].audInfo.bitPerSamp, g_isf_audout_codec);

	return ISF_OK;
}

static ISF_RV _ISF_AudOut_InitInfo(ISF_UNIT *p_thisunit, UINT32 oport)
{
	ISF_AUD_INFO *pAudInfo = (ISF_AUD_INFO *) & (ImageUnit_In(p_thisunit, ISF_IN1)->info);

	if (!pAudInfo) {
		DBG_ERR("pAudInfo is NULL\r\n");
		return ISF_ERR_FAILED;
	}

	g_isf_audout_codec          = pAudInfo->AudFmt;
	wsis[id].audInfo.bitPerSamp  = pAudInfo->max_bitpersample;
	wsis[id].audInfo.ch_num      = pAudInfo->max_channelcount;
	wsis[id].audInfo.audSR       = pAudInfo->max_samplepersecond;

	wavstudio_SetConfig(WAVSTUD_CFG_VOL, WAVSTUD_ACT_PLAY, g_volume);
	//DBG_IND("SR=%d,CH=%d,bps=%d,codec=%d\r\n", wsis[id].audInfo.audSR, wsis[id].audInfo.audCh, wsis[id].audInfo.bitPerSamp, wsis[id].audInfo.codec);

	return ISF_OK;
}
#endif

#if AUDOUT_DYNAMIC_CONTEXT
static UINT32 _isf_audout_query_context_size(void)
{
	return (DEV_MAX_COUNT * sizeof(AUDOUT_CONTEXT_DEV));
}

static UINT32 _isf_audout_query_context_memory(void)
{
	AUDOUT_CONTEXT_COMMON *p_ctx_c = (AUDOUT_CONTEXT_COMMON *)&g_ao_ctx.comm;

	p_ctx_c->ctx_mem.isf_audout_size = _isf_audout_query_context_size();
	p_ctx_c->ctx_mem.wavstudio_size  = 0;

	return (p_ctx_c->ctx_mem.isf_audout_size) + (p_ctx_c->ctx_mem.wavstudio_size);
}
#endif

static void _isf_audout_assign_context(UINT32 addr)
{
	AUDOUT_CONTEXT_COMMON *p_ctx_c = (AUDOUT_CONTEXT_COMMON *)&g_ao_ctx.comm;

	g_ao_ctx.dev = (AUDOUT_CONTEXT_DEV*)addr;
	memset((void *)g_ao_ctx.dev, 0, p_ctx_c->ctx_mem.isf_audout_size);
}

static ER _isf_audout_assign_context_memory_range(UINT32 addr)
{
	#if _TODO
	AUDOUT_CONTEXT_COMMON *p_ctx_c = (AUDOUT_CONTEXT_COMMON *)&g_ao_ctx.comm;
	#endif

	// [ isf_audout ]
	_isf_audout_assign_context(addr);

	#if _TODO
	addr += p_ctx_c->ctx_mem.isf_audout_size;

	// [ wavstudio ]
	#endif

	return E_OK;
}

static void _isf_audout_free_context(void)
{
	// reset ctx_mem to 0
	memset((void*)&(g_ao_ctx.comm), 0, sizeof(AUDOUT_CONTEXT_COMMON));

	g_ao_ctx.dev = NULL;
}

static ER _isf_audout_free_context_memory_range(void)
{
	// [ isf_vdoenc ]
	_isf_audout_free_context();

	// [ wavstudio ]

	return E_OK;
}


static ISF_RV _isf_audout_do_init(UINT32 h_isf, UINT32 dev_max_count)
{
	ISF_RV rv = ISF_ERR_INIT;
	AUDOUT_CONTEXT_COMMON *p_ctx_c = (AUDOUT_CONTEXT_COMMON *)&g_ao_ctx.comm;


#if defined (__KERNEL__)
	{
		if (kdrv_builtin_is_fastboot() && isf_audout_fastboot) {
			UINT32 eid = KDRV_DEV_ID(0, KDRV_AUDOUT_ENGINE0, 0);
			UINT32 drv_param[3] = {0};

			drv_param[0] = 1;

			kdrv_audioio_set(eid, KDRV_AUDIOIO_OUT_STOP_TX, (VOID*)&drv_param[0]);

			isf_audout_fastboot = FALSE;
		}
	}
#endif


	if (h_isf > 0) { //client process
		if (!g_audout_init[0]) { //is master process already init?
			rv = ISF_ERR_INIT; //not allow init from client process!
			goto _AO_INIT_ERR;
		}
		g_audout_init[h_isf] = 1; //set init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_audout_init[i]) { //client process still alive?
				rv = ISF_ERR_INIT; //not allow init from main process!
				goto _AO_INIT_ERR;
			}
		}
	}

	if (dev_max_count > 0) {
		//update count
		DEV_MAX_COUNT = dev_max_count;
		if (DEV_MAX_COUNT > DEV_NUM) {
			DBG_WRN("dev max count %d > max %d!\r\n", DEV_MAX_COUNT, DEV_NUM);
			DEV_MAX_COUNT = DEV_NUM;
		}
	}
	if (p_ctx_c->ctx_mem.ctx_addr) {
		rv = ISF_ERR_INIT;
		goto _AO_INIT_ERR;
	}
	nvtmpp_vb_add_module(isf_audout0.unit_module);
	{
		UINT32 dev;
#if AUDOUT_DYNAMIC_CONTEXT
		UINT32 buf_addr = 0;
		UINT32 buf_size = _isf_audout_query_context_memory();

		//alloc context of all device
		nvtmpp_vb_add_module(isf_audout0.unit_module);
		buf_addr = (&isf_audout0)->p_base->do_new_i(&isf_audout0, NULL, "ctx", buf_size, &(p_ctx_c->ctx_mem.ctx_memblk));

		if (buf_addr == 0) {
			rv = ISF_ERR_HEAP;
			goto _AO_INIT_ERR;
		}

		p_ctx_c->ctx_mem.ctx_addr = buf_addr;
#else
		p_ctx_c->ctx_mem.ctx_addr = (UINT32)&g_ao_ctx_dev;
		p_ctx_c->ctx_mem.isf_audout_size = sizeof(g_ao_ctx_dev);
#endif
		_isf_audout_assign_context_memory_range(p_ctx_c->ctx_mem.ctx_addr);

		//init each device's context
		for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);
			if (p_unit && (p_unit->do_init)) {
				rv = p_unit->do_init(p_unit, (void*)&(g_ao_ctx.dev[dev]));
				if (rv != ISF_OK) {
					#if AUDOUT_DYNAMIC_CONTEXT
					rv = (&isf_audout0)->p_base->do_release_i(&isf_audout0, &(g_ao_ctx.comm.ctx_mem.ctx_memblk), ISF_OK);
					#endif
					goto _AO_INIT_ERR;
				}
			}
		}

		//install each device's kernel object
		isf_audout_install_id();
		g_audout_init[h_isf] = 1; //set init
		return ISF_OK;
	}

_AO_INIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("init fail! %d\r\n", rv);
	}
	return rv;
}

static ISF_RV _isf_audout_do_uninit(UINT32 h_isf)
{
	ISF_RV rv;
	AUDOUT_CONTEXT_COMMON *p_ctx_c = (AUDOUT_CONTEXT_COMMON *)&g_ao_ctx.comm;
	UINT32 dev, i;

	if (h_isf > 0) { //client process
		if (!g_audout_init[0]) { //is master process already init?
			rv = ISF_ERR_UNINIT; //not allow uninit from client process!
			goto _AO_UNINIT_ERR;
		}
		g_audout_init[h_isf] = 0; //clear init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_audout_init[i]) { //client process still alive?
				rv = ISF_ERR_UNINIT; //not allow uninit from main process!
				goto _AO_UNINIT_ERR;
			}
		}
	}

	if (!p_ctx_c->ctx_mem.ctx_addr) {
		return ISF_ERR_UNINIT;
	}

	if (DEV_MAX_COUNT == 0) {
		return ISF_ERR_UNINIT;
	}

	for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
		ISF_UNIT* p_unit = DEV_UNIT(dev);

		if (p_unit) {
			AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_unit->refdata);
			if (p_ctx->isf_audout_opened) {
				DBG_ERR("Not all dev closed\r\n");
				return ISF_ERR_ALREADY_OPEN;
			}
		}
	}

	{

		//reset all units of this module
		for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);
			if (p_unit) {
				DBG_IND("unit(%d) = %s => clear state\r\n", dev, p_unit->unit_name);
				for(i = 0; i < p_unit->port_out_count; i++) {
					UINT32 oport = i + ISF_OUT_BASE;
					p_unit->p_base->do_clear_state(p_unit, oport);
				}
			}
		}
		for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);
			if (p_unit) {
				DBG_IND("unit(%d) = %s => clear bind\r\n", dev, p_unit->unit_name);
				for(i = 0; i < p_unit->port_out_count; i++) {
					UINT32 oport = i + ISF_OUT_BASE;
					p_unit->p_base->do_clear_bind(p_unit, oport);
				}
			}
		}
		for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);
			if (p_unit) {
				DBG_IND("unit(%d) = %s => clear context\r\n", dev, p_unit->unit_name);
				for(i = 0; i < p_unit->port_out_count; i++) {
					UINT32 oport = i + ISF_OUT_BASE;
					p_unit->p_base->do_clear_ctx(p_unit, oport);
				}
			}
		}

		//uninstall each device's kernel object
		isf_audout_uninstall_id();

		//uninit each device's context
		for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);
			if (p_unit && (p_unit->do_uninit)) {
				rv = p_unit->do_uninit(p_unit);
				if (rv != ISF_OK) {
					goto _AO_UNINIT_ERR;
				}
			}
		}
#if AUDOUT_DYNAMIC_CONTEXT
		//free context of all device
		if (p_ctx_c->ctx_mem.ctx_memblk.h_data != 0) {
			rv = (&isf_audout0)->p_base->do_release_i(&isf_audout0, &(p_ctx_c->ctx_mem.ctx_memblk), ISF_OK);
			if (rv != ISF_OK) {
				DBG_ERR("[ISF_AUDOUT] free context memory fail !!\r\n");
				goto _AO_UNINIT_ERR;
			}
		} else {
			DBG_ERR("[ISF_AUDOUT] context h_data == NULL !!\r\n");
			rv = ISF_ERR_INVALID_DATA_ID;
			goto _AO_UNINIT_ERR;
		}
#endif
		_isf_audout_free_context_memory_range();
		g_audout_init[h_isf] = 0; //clear init

		return ISF_OK;
	}

_AO_UNINIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("uninit fail! %d\r\n", rv);
	}
	return rv;
}

ISF_RV _isf_audout_do_command(UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2)
{
	UINT32 h_isf = (cmd >> 28); //extract h_isf

	cmd &= ~0xf0000000; //clear h_isf
	switch(cmd) {
	case 1: //INIT
		return _isf_audout_do_init(h_isf, p1); //p1: max dev count
		break;
	case 0: //UNINIT
		return _isf_audout_do_uninit(h_isf);
		break;
	default:
		break;
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_audout_do_open(UINT32 id, ISF_UNIT *p_thisunit, UINT32 oport)
{
	#if defined __UITRON || defined __ECOS
	if (g_audoutcmd_installed == FALSE) {
		_isf_audout_install_cmd();
		g_audoutcmd_installed = TRUE;
	}
	#endif

	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_INIT; //sitll not init
	}

	{
		#if defined __UITRON || defined __ECOS
		DX_HANDLE devAudObj = Dx_GetObject(DX_CLASS_AUDIO_EXT);
		#else
		UINT32 devAudObj = 0;
		#endif
		UINT32 addr;
		UINT32 size;
		ISF_RV r = ISF_OK;
		ISF_AUD_INFO *pAudInfo = (ISF_AUD_INFO *) & (p_thisunit->port_ininfo);
		WAVSTUD_ACT act = (id == DEV_ID_0)? WAVSTUD_ACT_PLAY : WAVSTUD_ACT_PLAY2;

		if (!pAudInfo) {
			DBG_ERR("pAudInfo is NULL\r\n");
			return ISF_ERR_NULL_POINTER;
		}

		#if 0
		if (g_isf_audout_maxmem.addr == 0 && g_isf_audout_maxmem.size != 0) {
			//allocate max buffer
			g_isf_audout_maxmem.addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "max", g_isf_audout_maxmem.size, &(p_ctx->g_isf_audout_memblk));
			if (g_isf_audout_maxmem.addr == 0) {
				DBG_ERR("get max buf failed\r\n");
				return ISF_ERR_BUFFER_CREATE;
			}
		}
		#endif

		if (p_ctx->g_isf_audout_maxmem.addr != 0 && p_ctx->g_isf_audout_maxmem.size != 0) {
			DBG_IND("SR=%d,CH=%d,ch_num=%d,bps=%d\r\n", p_ctx->wsis_max.aud_info.aud_sr, p_ctx->wsis_max.aud_info.aud_ch, p_ctx->wsis_max.aud_info.ch_num, p_ctx->wsis_max.aud_info.bitpersample);

			size = wavstudio_get_config(WAVSTUD_CFG_MEM, (UINT32)&(p_ctx->wsis_max), act);

			if (size > p_ctx->g_isf_audout_maxmem.size) {
				DBG_ERR("Needed size=%d > Maximum size=%d\r\n", size, p_ctx->g_isf_audout_maxmem.size);
				return ISF_ERR_NEW_FAIL;
			}

			p_ctx->wso.mem.uiAddr = p_ctx->g_isf_audout_maxmem.addr;
			p_ctx->wso.mem.uiSize = p_ctx->g_isf_audout_maxmem.size;
			p_ctx->wso.wavstud_cb = _audout_evtcb;

			DBG_IND("mem addr=%x, size=%x\r\n", p_ctx->wso.mem.uiAddr, p_ctx->wso.mem.uiSize);

			if (E_OK != wavstudio_open(act, &(p_ctx->wso), devAudObj, &(p_ctx->wsis_max))) {
				DBG_ERR("audout open failed\r\n");
				return ISF_ERR_PROCESS_FAIL;
			}
		} else {

			DBG_IND("SR=%d,CH=%d,ch_num=%d,bps=%d\r\n", p_ctx->wsis_max.aud_info.aud_sr, p_ctx->wsis_max.aud_info.aud_ch, p_ctx->wsis_max.aud_info.ch_num, p_ctx->wsis_max.aud_info.bitpersample);

			p_ctx->wsis_max.aud_info.buf_sample_cnt = p_ctx->g_isf_audout_max_frame_sample;
			p_ctx->wsis_max.buf_count = p_ctx->g_isf_audout_buf_cnt;

			if (p_ctx->isf_audout_tdm_ch) {
				p_ctx->wsis_max.aud_info.ch_num = p_ctx->isf_audout_tdm_ch;
			}

			size = wavstudio_get_config(WAVSTUD_CFG_MEM, (UINT32)&(p_ctx->wsis_max), act);

			DBG_IND("audout needed mem = %x\r\n", size);

			/* create a buffer pool*/
			addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "work", size, &(p_ctx->g_isf_audout_memblk));

			if (addr == 0) {
				/*r = p_thisunit->p_base->do_release_i(p_thisunit, &g_isf_audout_memblk[id], ISF_OK);
				if (r != ISF_OK) {
					DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
					return ISF_ERR_BUFFER_RELEASE;
				}*/
				return ISF_ERR_BUFFER_CREATE;
			}

			p_ctx->wso.mem.uiAddr = addr;
			p_ctx->wso.mem.uiSize = size;
			p_ctx->wso.wavstud_cb = _audout_evtcb;

			if (E_OK != wavstudio_open(act, &(p_ctx->wso), devAudObj, &(p_ctx->wsis_max))) {
				DBG_ERR("audout open failed\r\n");
				//p_thisunit->p_base->do_release(p_thisunit, NULL, &g_isf_audout_memblk[id], 0, ISF_OK);
				r = p_thisunit->p_base->do_release_i(p_thisunit, &(p_ctx->g_isf_audout_memblk), ISF_OK);
				if (r != ISF_OK) {
					DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
					return ISF_ERR_BUFFER_RELEASE;
				}
				return ISF_ERR_PROCESS_FAIL;
			}

			if (paudsrc_obj) {
				if (paudsrc_obj->src.pre_init) {
					UINT32 output = p_ctx->wsis_max.aud_info.aud_sr*4/100;
					UINT32 input = p_ctx->isf_audout_resample_max*4/100;
					UINT32 need = paudsrc_obj->src.pre_init(p_ctx->wsis_max.aud_info.ch_num, input, output, FALSE);

					p_ctx->g_isf_audout_srcmem.size = need;
					p_ctx->g_isf_audout_srcmem.addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "src", p_ctx->g_isf_audout_srcmem.size, &(p_ctx->isf_audout_srcmemblk));
					if (p_ctx->g_isf_audout_srcmem.addr == 0) {
						DBG_ERR("get SRC buf failed\r\n");
						return ISF_ERR_BUFFER_CREATE;
					}

					p_ctx->g_isf_audout_srcbufmem.size = (output + input) * p_ctx->wsis_max.aud_info.ch_num * (p_ctx->wsis_max.aud_info.bitpersample >> 3);
					p_ctx->g_isf_audout_srcbufmem.addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "srcbuf", p_ctx->g_isf_audout_srcbufmem.size, &(p_ctx->isf_audout_srcbufmemblk));
					if (p_ctx->g_isf_audout_srcbufmem.addr == 0) {
						DBG_ERR("get SRC work buf failed\r\n");
						return ISF_ERR_BUFFER_CREATE;
					}
				}
			}

			p_ctx->g_isf_audout_allocmem = TRUE;
		}
		//DBG_DUMP("[isf_audout]Open\r\n");
	}

	p_ctx->isf_audout_opened = TRUE;

	return ISF_OK;
}
static ISF_RV _isf_audout_do_close(UINT32 id, ISF_UNIT *p_thisunit)
{
	ISF_RV r = ISF_OK;
	WAVSTUD_ACT act = (id == DEV_ID_0)? WAVSTUD_ACT_PLAY : WAVSTUD_ACT_PLAY2;
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_INIT; //sitll not init
	}

	{
		//wavstudio_set_config(WAVSTUD_CFG_VOL, act, p_ctx->g_volume);

		if ((p_ctx->g_isf_audout_maxmem.addr == 0 || p_ctx->g_isf_audout_maxmem.size == 0) && p_ctx->g_isf_audout_allocmem) {
			if (E_OK != wavstudio_close(act)) {
				DBG_ERR("audout already closed\r\n");
				r = p_thisunit->p_base->do_release_i(p_thisunit, &(p_ctx->g_isf_audout_memblk), ISF_OK);
				if (r != ISF_OK) {
					DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
					//return ISF_ERR_BUFFER_RELEASE;
				}
				return ISF_ERR_PROCESS_FAIL;
			}
			r = p_thisunit->p_base->do_release_i(p_thisunit, &(p_ctx->g_isf_audout_memblk), ISF_OK);
			if (r != ISF_OK) {
				DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
				//return ISF_ERR_BUFFER_RELEASE;
			}

			if (p_ctx->g_isf_audout_srcmem.addr) {
				r = p_thisunit->p_base->do_release_i(p_thisunit, &(p_ctx->isf_audout_srcmemblk), ISF_OK);
				if (r != ISF_OK) {
					DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
					//return ISF_ERR_BUFFER_RELEASE;
				}
				p_ctx->g_isf_audout_srcmem.addr = 0;
				p_ctx->g_isf_audout_srcmem.size = 0;
			}

			if (p_ctx->g_isf_audout_srcbufmem.addr) {
				r = p_thisunit->p_base->do_release_i(p_thisunit, &(p_ctx->isf_audout_srcbufmemblk), ISF_OK);
				if (r != ISF_OK) {
					DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
					//return ISF_ERR_BUFFER_RELEASE;
				}
				p_ctx->g_isf_audout_srcbufmem.addr = 0;
				p_ctx->g_isf_audout_srcbufmem.size = 0;
			}

			p_ctx->g_isf_audout_allocmem = FALSE;
		} else {
			DBG_IND("Do not release pool\r\n");
			if (E_OK != wavstudio_close(act)) {
				DBG_ERR("audout already closed\r\n");
				return ISF_ERR_PROCESS_FAIL;
			}

		}
		//DBG_DUMP("[isf_audout]Close\r\n");
		//return ISF_OK;
	}

	p_ctx->isf_audout_opened = FALSE;

	return ISF_OK;
}

static ISF_RV _isf_audout_do_start(UINT32 id, ISF_UNIT *p_thisunit, UINT32 oport)
{
	ISF_AUD_INFO *pAudInfo = (ISF_AUD_INFO *) & (p_thisunit->port_ininfo);
	UINT32 output_dev = 0;
	WAVSTUD_ACT act = (id == DEV_ID_0)? WAVSTUD_ACT_PLAY : WAVSTUD_ACT_PLAY2;
	UINT32 needed_buf = 0;
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_INIT; //sitll not init
	}

	if (p_ctx->g_volume & WAVSTUD_PORT_VOLUME) {
		wavstudio_set_config(WAVSTUD_CFG_VOL_P0+id, act, p_ctx->g_volume);
	} else {
		wavstudio_set_config(WAVSTUD_CFG_VOL, act, p_ctx->g_volume);
	}

	if (p_ctx->isf_audout_started == FALSE) {

		if (p_ctx->isf_audout_tdm_ch) {
			p_ctx->wsis.aud_info.ch_num = p_ctx->isf_audout_tdm_ch;
		}

		if (p_ctx->wsis.aud_info.aud_sr > p_ctx->wsis_max.aud_info.aud_sr) {
			return ISF_ERR_INVALID_VALUE;
		}

		if (p_ctx->wsis.aud_info.ch_num > p_ctx->wsis_max.aud_info.ch_num) {
			return ISF_ERR_INVALID_VALUE;
		}

		if (p_ctx->wsis.aud_info.bitpersample > p_ctx->wsis_max.aud_info.bitpersample) {
			return ISF_ERR_INVALID_VALUE;
		}

		if (!pAudInfo) {
			DBG_ERR("pAudInfo is NULL\r\n");
			return ISF_ERR_NULL_POINTER;
		}

		switch (g_isf_audout_outdev) {
			case AUDOUT_OUTPUT_LINE:
				output_dev = KDRV_AUDIO_OUT_PATH_LINEOUT;
				break;
			case AUDOUT_OUTPUT_ALL:
				output_dev = KDRV_AUDIO_OUT_PATH_ALL;
				break;
			case AUDOUT_OUTPUT_I2S:
				if (p_ctx->exclusive_output) {
					output_dev = KDRV_AUDIO_OUT_PATH_I2S_ONLY;
				} else {
					output_dev = KDRV_AUDIO_OUT_PATH_I2S;
				}
				break;
			case AUDOUT_OUTPUT_SPK:
			default:
				output_dev = KDRV_AUDIO_OUT_PATH_SPEAKER;
				break;
		}

		wavstudio_set_config(WAVSTUD_CFG_PLAY_OUT_DEV, output_dev, 0);
		p_ctx->wsis.aud_info.aud_ch = (p_ctx->wsis.aud_info.ch_num == 1)? p_ctx->isf_audout_mono_ch : AUDIO_CH_STEREO;

		p_ctx->wsis.aud_info.buf_sample_cnt = p_ctx->g_isf_audout_max_frame_sample;
		p_ctx->wsis.buf_count = p_ctx->g_isf_audout_buf_cnt;

		needed_buf = wavstudio_get_config(WAVSTUD_CFG_MEM, (UINT32)&(p_ctx->wsis), act);

		if (p_ctx->wso.mem.uiSize < needed_buf) {
			DBG_ERR("Need mem size (%d) > max public size (%d)\r\n", needed_buf, p_ctx->wso.mem.uiSize);
			return ISF_ERR_NEW_FAIL;
		}

		if (p_ctx->isf_audout_resample != 0 &&  p_ctx->wsis.aud_info.aud_sr != p_ctx->isf_audout_resample && paudsrc_obj) {
			_isf_audout_src_check(id, p_ctx->isf_audout_resample);
		} else {
			p_ctx->isf_audout_resample_en = FALSE;
		}

		p_ctx->isf_audout_resample_update = 0;
		p_ctx->isf_audout_resample_dirty = 0;
		p_ctx->played_size = 0;

		if (p_ctx->g_isf_audout_lpf_en) {
			if (paudfilt_obj->open) {
				paudfilt_obj->open(p_ctx->wsis.aud_info.ch_num, 1);
			}
			if (paudfilt_obj->design) {
				paudfilt_obj->design();
			}

			wavstudio_set_config(WAVSTUD_CFG_FILT_CB, act, (UINT32)paudfilt_obj->apply);
		} else {
			wavstudio_set_config(WAVSTUD_CFG_FILT_CB, act, 0);
		}

		wavstudio_set_config(WAVSTUD_CFG_PLAY_WAIT_PUSH, act, (UINT32)p_ctx->wait_push);

		DBG_IND("SR=%d,CH=%d,ch_num=%d,bps=%d\r\n", p_ctx->wsis.aud_info.aud_sr, p_ctx->wsis.aud_info.aud_ch, p_ctx->wsis.aud_info.ch_num, p_ctx->wsis.aud_info.bitpersample);

		{
			if (FALSE == wavstudio_start(act, &(p_ctx->wsis.aud_info), WAVSTUD_NON_STOP_COUNT, audout_cb)) {
				DBG_ERR("audout play failed\r\n");
				return ISF_ERR_PROCESS_FAIL;
			}
			wavstudio_wait_start(act);
			p_ctx->isf_audout_started = TRUE;
			//DBG_DUMP("[isf_audout]Start\r\n");
		}
	}

	return ISF_OK;
}
static ISF_RV _isf_audout_do_stop(UINT32 id, ISF_UNIT *p_thisunit)
{
	WAVSTUD_ACT act = (id == DEV_ID_0)? WAVSTUD_ACT_PLAY : WAVSTUD_ACT_PLAY2;
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_INIT; //sitll not init
	}

	if (p_ctx->isf_audout_started == TRUE){
		if (FALSE == wavstudio_stop(act)) {
			DBG_ERR("audout stop play failed\r\n");
			return ISF_ERR_PROCESS_FAIL;
		}
		wavstudio_wait_stop(act);
		//DBG_DUMP("[isf_audout]Stop\r\n");

		if (paudsrc_obj && p_ctx->isf_audout_resample_en) {
			if (paudsrc_obj->src.destroy) {
				paudsrc_obj->src.destroy(p_ctx->resample_handle_play);
			}
		}

		if (p_ctx->g_isf_audout_lpf_en) {
			if (paudfilt_obj->close) {
				paudfilt_obj->close();
			}
		}

		p_ctx->isf_audout_started = FALSE;
	}

	return ISF_OK;
}

static ISF_RV _isf_audout_do_reset(UINT32 id, ISF_UNIT *p_thisunit, UINT32 oport)
{
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_INIT; //sitll not init
	}

	//DBG_UNIT("\r\n");

	//reset in parameter
	if(oport == ISF_OUT_BASE) {
		ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
		ISF_AUD_INFO *p_audinfo = (ISF_AUD_INFO *)(p_src_info);
		memset(p_audinfo, 0, sizeof(ISF_AUD_INFO));
	}
	//reset out parameter
	{
		UINT32 i = oport - ISF_OUT_BASE;
		ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
		ISF_AUD_INFO *p_audinfo2 = (ISF_AUD_INFO *)(p_dest_info);
		memset(p_audinfo2, 0, sizeof(ISF_AUD_INFO));
	}
	//reset out state
	{
		UINT32 i = oport - ISF_OUT_BASE;
		ISF_STATE *p_state = p_thisunit->port_outstate[i];
		memset(p_state, 0, sizeof(ISF_STATE));
	}

	p_ctx->g_volume = 100;

	return ISF_OK;
}

static void _isf_audout_do_runtime(UINT32 id, ISF_UNIT *p_thisunit, UINT32 cmd, UINT32 oport)
{
	WAVSTUD_ACT act = (id == DEV_ID_0)? WAVSTUD_ACT_PLAY : WAVSTUD_ACT_PLAY2;
	ISF_INFO *p_in_info = p_thisunit->port_ininfo[0];
	ISF_AUD_INFO *p_audinfo_in = (ISF_AUD_INFO *)(p_in_info);
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return; //sitll not init
	}

	if (p_ctx->g_volume & WAVSTUD_PORT_VOLUME) {
		wavstudio_set_config(WAVSTUD_CFG_VOL_P0+id, act, p_ctx->g_volume);
	} else {
		wavstudio_set_config(WAVSTUD_CFG_VOL, act, p_ctx->g_volume);
	}

	if (p_audinfo_in) {
		if (p_audinfo_in->dirty & ISF_INFO_DIRTY_SAMPLERATE) {
			p_ctx->isf_audout_resample_update = p_audinfo_in->samplepersecond;
			p_ctx->isf_audout_resample_dirty = 1;
			p_audinfo_in->dirty &= ~ISF_INFO_DIRTY_SAMPLERATE;
			DBG_IND("output update port SR=%d\r\n", p_ctx->isf_audout_resample_update);
			ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM_IMM, p_thisunit, ISF_IN(0), "set resample sr(%d)", p_ctx->isf_audout_resample_update);
		}
	}
}

static ISF_RV _isf_audout_setparam(UINT32 id, ISF_UNIT *p_thisunit, UINT32 param, UINT32 value)
{
	ISF_RV ret = ISF_OK;
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_INIT; //sitll not init
	}

	if (param < AUDOUT_PARAM_START) {
		return ISF_ERR_INVALID_PARAM;
	}
	switch (param) {
	case AUDOUT_PARAM_VOL_IMM: {
			AUDOUT_CONTEXT_DEV* p_ctx_unit = 0;

			if (value > 160) {
				DBG_WRN("Invalid volume=%d, set to 100\r\n", value);
				value = 100;
			}

			//set same volume to every unit
			p_ctx_unit = (AUDOUT_CONTEXT_DEV*)(isf_audout0.refdata);
			if (p_ctx_unit) {
				p_ctx_unit->g_volume = value;
			}

			p_ctx_unit = (AUDOUT_CONTEXT_DEV*)(isf_audout1.refdata);
			if (p_ctx_unit) {
				p_ctx_unit->g_volume = value;
			}
			//wavstudio_set_config(WAVSTUD_CFG_VOL, WAVSTUD_ACT_PLAY, g_volume);
			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set volume(%d)", p_ctx->g_volume);
			break;
		}
	case AUDOUT_PARAM_EN_EVT_CB: {
			g_isf_audout_evt_en = (value == 0)? FALSE : TRUE;
			//ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "Enable event CB(%d)\r\n", g_isf_audout_evt_en);
			//ret = ISF_APPLY_DOWNSTREAM_READYTIME;
			break;
		}
	case AUDOUT_PARAM_AUD_LPF_EN: {
			if (id == 0) {
				p_ctx->g_isf_audout_lpf_en = (value == 0)? FALSE : TRUE;

				if (p_ctx->g_isf_audout_lpf_en) {
					paudfilt_obj = &audfilt_obj;
				} else {
					paudfilt_obj = NULL;
				}
			} else {
				DBG_ERR("id=%d not support param[%08x]\r\n", id, param);
			}
		}
		break;
	case AUDOUT_PARAM_WAIT_PUSH: {
			p_ctx->wait_push = (value == 0)? FALSE : TRUE;
		}
		break;
	case AUDOUT_PARAM_AUD_PREPWR_EN: {
			UINT32 status = 0;
			UINT32 act = (id == 0)? WAVSTUD_ACT_PLAY : WAVSTUD_ACT_PLAY2;
			ER retV;

			status = wavstudio_get_config(WAVSTUD_CFG_GET_STATUS, act, 0);
			if (status == WAVSTUD_STS_CLOSED) {
				BOOL drv_param[3] = {0};
				UINT32 eid = (id == 0)? KDRV_DEV_ID(0, KDRV_AUDOUT_ENGINE0, 0) : KDRV_DEV_ID(0, KDRV_AUDOUT_ENGINE1, 0);
				isf_audout_prepwr_en = value;


				retV = kdrv_audioio_open(0, KDRV_DEV_ID_ENGINE(eid));
				if (retV != 0) {
					DBG_ERR("kdrv_audioio open fail = %d\r\n", retV);
					ret = ISF_ERR_DRV;
					return ret;
				}

				switch (g_isf_audout_outdev) {
					case AUDOUT_OUTPUT_LINE:
						drv_param[0] = KDRV_AUDIO_OUT_PATH_LINEOUT;
						break;
					case AUDOUT_OUTPUT_ALL:
						drv_param[0] = KDRV_AUDIO_OUT_PATH_ALL;
						break;
					case AUDOUT_OUTPUT_I2S:
						drv_param[0] = KDRV_AUDIO_OUT_PATH_I2S;
						break;
					case AUDOUT_OUTPUT_SPK:
					default:
						drv_param[0] = KDRV_AUDIO_OUT_PATH_SPEAKER;
						break;
				}


				if (g_isf_audout_outdev == AUDOUT_OUTPUT_LINE || g_isf_audout_outdev == AUDOUT_OUTPUT_ALL) {
					kdrv_audioio_set(eid, KDRV_AUDIOIO_OUT_PATH, (VOID*)&drv_param[0]);

					if (isf_audout_prepwr_en == 0) {
						drv_param[0] = FALSE;
					} else {
						drv_param[0] = TRUE;
					}
					kdrv_audioio_set(eid, KDRV_AUDIOIO_OUT_LINE_PWR_ALWAYS_ON, (VOID*)&drv_param[0]);
				} else if (g_isf_audout_outdev == AUDOUT_OUTPUT_SPK) {
					kdrv_audioio_set(eid, KDRV_AUDIOIO_OUT_PATH, (VOID*)&drv_param[0]);

					if (isf_audout_prepwr_en == 0) {
						drv_param[0] = FALSE;
					} else {
						drv_param[0] = TRUE;
					}
					kdrv_audioio_set(eid, KDRV_AUDIOIO_OUT_SPK_PWR_ALWAYS_ON, (VOID*)&drv_param[0]);
				}
				kdrv_audioio_close(0, KDRV_DEV_ID_ENGINE(eid));
			} else {
				ret = ISF_ERR_INVALID_STATE;
			}
			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set PREPWR en(%d)\r\n", isf_audout_prepwr_en);
			break;
		}
	case AUDOUT_PARAM_EXCLUSIVE_OUTPUT: {
			p_ctx->exclusive_output = (value == 0)? FALSE : TRUE;
		}
		break;
	default:
		DBG_ERR("param[%08x] = %d\r\n", param, value);
		break;
	}
	return ret;
}
static UINT32 _isf_audout_getparam(UINT32 id, ISF_UNIT *p_thisunit, UINT32 param)
{
	UINT32 value = 0;
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_INIT; //sitll not init
	}

	if (param < AUDOUT_PARAM_START) {
		return ISF_ERR_INVALID_PARAM;
	}
	switch (param) {
	case AUDOUT_PARAM_VOL_IMM: {
			value = wavstudio_get_config(WAVSTUD_CFG_VOL, WAVSTUD_ACT_PLAY, 0);
			break;
		}
	case AUDOUT_PARAM_PRELOAD_DONE: {
			value = p_ctx->preload_done;
			break;
		}
	case AUDOUT_PARAM_DONE_SIZE: {
			_isf_audout_buff_size_wait(id);
			value = p_ctx->played_size;
			break;
		}
	case AUDOUT_PARAM_NEEDED_BUFF: {
			WAVSTUD_ACT act = (id == DEV_ID_0)? WAVSTUD_ACT_PLAY : WAVSTUD_ACT_PLAY2;

			_isf_audout_update_port_info(id, p_thisunit, 0);

			p_ctx->wsis_max.aud_info.buf_sample_cnt = p_ctx->g_isf_audout_max_frame_sample;
			p_ctx->wsis_max.buf_count = p_ctx->g_isf_audout_buf_cnt;

			value = wavstudio_get_config(WAVSTUD_CFG_MEM, (UINT32)&(p_ctx->wsis_max), act);

			break;
		}
	default:
		DBG_ERR("param[%08x]\r\n", param);
		break;
	}
	return value;
}

ISF_RV _isf_audout_setportparam(UINT32 id, struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 value)
{
	ISF_RV ret = ISF_OK;
	ISF_AUD_INFO *pAudInfo =  NULL;
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_INIT; //sitll not init
	}

	if (param < AUDOUT_PARAM_START) {
		return ISF_ERR_NOT_SUPPORT;
	}

    if (nport == ISF_CTRL)
    {
        return _isf_audout_setparam(id, p_thisunit, param, value);
    }

	if (nport == ISF_OUT(0)) {
		switch (param) {
		case AUDOUT_PARAM_CHANNEL: {
				if (value > AUDIO_CH_STEREO) {
					DBG_ERR("Invalid channel=%d\r\n", value);
					ret = ISF_ERR_INVALID_VALUE;
				} else {
					p_ctx->isf_audout_mono_ch = value;
					ret = ISF_OK;
					ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, nport, "set mono channel(%d)", p_ctx->isf_audout_mono_ch);
				}
				break;
			}
		case AUDOUT_PARAM_BUF_UNIT_TIME:
			if (value > WAVSTUD_UNIT_TIME_1000MS) {
				DBG_ERR("Invalid buffer unit time=%d\r\n", value);
				ret = ISF_ERR_INVALID_VALUE;
			} else {
				p_ctx->wsis.unit_time = value;
				ret = ISF_OK;
				//ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, nport, "set unit time(%d)\r\n", wsis.unitTime);
			}
			break;
		case AUDOUT_PARAM_BUF_COUNT:
			if (value < 2) {
				DBG_ERR("Invalid buffer count=%d\r\n", value);
				ret = ISF_ERR_INVALID_VALUE;
			} else {
				p_ctx->g_isf_audout_buf_cnt = value;
				ret = ISF_OK;
				ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, nport, "set buff count(%d)", p_ctx->g_isf_audout_buf_cnt);
			}
			break;
		case AUDOUT_PARAM_OUT_DEV: {
			g_isf_audout_outdev = value;
			ret = ISF_OK;
			//ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "Output device(%d)\r\n", g_isf_audout_outdev);
			//ret = ISF_APPLY_DOWNSTREAM_READYTIME;
			ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, nport, "set out dev(%d)", g_isf_audout_outdev);
			break;
		}
		case AUDOUT_PARAM_MAX_FRAME_SAMPLE: {
			p_ctx->g_isf_audout_max_frame_sample = value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, nport, "set max frame sample(%d)", p_ctx->g_isf_audout_max_frame_sample);

			break;
		}

		case AUDOUT_PARAM_MAX_MEM_INFO:
			{
				ISF_RV r = ISF_OK;
				AUDOUT_MAX_MEM_INFO* pMaxMemInfo = (AUDOUT_MAX_MEM_INFO*) value;
				WAVSTUD_INFO_SET* pAudInfoSet = pMaxMemInfo->pAudInfoSet;

				//ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, nport, "set max mem info(0x%x), release(%d)\r\n", pMaxMemInfo, pMaxMemInfo->bRelease);

				if (pMaxMemInfo->bRelease) {
					if (p_ctx->g_isf_audout_maxmem.addr != 0) {
						r = p_thisunit->p_base->do_release_i(p_thisunit, &(p_ctx->g_isf_audout_memblk), ISF_OK);
						if (r == ISF_OK) {
							p_ctx->g_isf_audout_maxmem.addr = 0;
							p_ctx->g_isf_audout_maxmem.size = 0;
						} else {
							DBG_ERR("[ISF_AUDOUT] release max buf failed, ret = %d\r\n", r);
						}
					}
				} else {
					if (p_ctx->g_isf_audout_maxmem.addr == 0) {
						memcpy((void *)&(p_ctx->wsis_max), (const void *) pAudInfoSet, sizeof(WAVSTUD_INFO_SET));
						p_ctx->g_isf_audout_maxmem.size = wavstudio_get_config(WAVSTUD_CFG_MEM, (UINT32)&(p_ctx->wsis_max), WAVSTUD_ACT_PLAY);
					} else {
						DBG_ERR("[ISF_AUDOUT] max buf exist, please release it first\r\n");
					}
				}

			}
			break;
		case AUDOUT_PARAM_VOL_IMM: {
			AUDOUT_CONTEXT_DEV* p_ctx_unit = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);

			if (value > 160) {
				DBG_WRN("Invalid volume=%d, set to 100\r\n", value);
				value = 100;
			}

			if (p_ctx_unit) {
				p_ctx_unit->g_volume = value | WAVSTUD_PORT_VOLUME;
			}

			//wavstudio_set_config(WAVSTUD_CFG_VOL, WAVSTUD_ACT_PLAY, g_volume);
			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set volume(%d)", p_ctx->g_volume);
			break;
		}
		default:
			ret = ISF_ERR_NOT_SUPPORT;
			DBG_ERR("param[%08x] = %d\r\n", param, value);
			break;
		}
	} else if (nport == ISF_IN(0)) {
		switch (param) {
		case ISF_PARAM_PORT_DIRTY:
			pAudInfo = (ISF_AUD_INFO *) & (p_thisunit->port_ininfo);
			if (pAudInfo->channelcount != p_ctx->wsis.aud_info.ch_num || pAudInfo->samplepersecond != p_ctx->wsis.aud_info.aud_sr) {
				ret = ISF_OK;
			}
			break;
		default:
			ret = ISF_ERR_NOT_SUPPORT;
			DBG_ERR("Not a valid paramter (0x%x)\r\n", param);
			break;
		}
	} else {
		DBG_ERR("Not a valid port %d\r\n", nport);
		ret = ISF_ERR_INVALID_PORT_ID;
	}
	return ret;
}
UINT32 _isf_audout_getportparam(UINT32 id, struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param)
{
	UINT32 value = 0;
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_INIT; //sitll not init
	}

	if (param < AUDOUT_PARAM_START) {
		return ISF_ERR_INVALID_PARAM;
	}

    if (nport == ISF_CTRL)
    {
        return _isf_audout_getparam(id, p_thisunit, param);
    }

	if (nport == ISF_OUT(0)) {
		switch (param) {
		case AUDOUT_PARAM_CHANNEL:
			value = p_ctx->wsis.aud_info.aud_ch;
			break;
		case AUDOUT_PARAM_BUF_UNIT_TIME:
			value = p_ctx->wsis.unit_time;
			break;
		case AUDOUT_PARAM_BUF_COUNT:
			value = p_ctx->wsis.buf_count;
			break;
		default:
			DBG_ERR("Not a valid paramter (0x%x)\r\n", param);
			break;
		}
	} else {
		DBG_ERR("Not a valid port %d\r\n", nport);
	}

	return value;
}

ISF_RV _isf_audout_setportstruct(UINT32 id, struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size){
	ISF_RV ret = ISF_OK;
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	//ISF_PORT *pDest = NULL;

	if (nport == ISF_CTRL) {
		/* for common port */
		switch (param) {
		case AUDOUT_PARAM_AUD_INIT_CFG:	{
			AUDOUT_AUD_INIT_CFG *p_aud_init = (AUDOUT_AUD_INIT_CFG *)p_struct;
			CTL_AUD_INIT_CFG_OBJ cfg_obj;
			memset((void *)(&cfg_obj), 0, sizeof(CTL_AUD_INIT_CFG_OBJ));
			cfg_obj.pin_cfg.pinmux.audio_pinmux  = p_aud_init->aud_init_cfg.pin_cfg.pinmux.audio_pinmux;
			cfg_obj.pin_cfg.pinmux.cmd_if_pinmux = p_aud_init->aud_init_cfg.pin_cfg.pinmux.cmd_if_pinmux;
			cfg_obj.i2s_cfg.bit_width     = p_aud_init->aud_init_cfg.i2s_cfg.bit_width;
			cfg_obj.i2s_cfg.bit_clk_ratio = p_aud_init->aud_init_cfg.i2s_cfg.bit_clk_ratio;
			cfg_obj.i2s_cfg.op_mode       = p_aud_init->aud_init_cfg.i2s_cfg.op_mode;
			cfg_obj.i2s_cfg.tdm_ch        = p_aud_init->aud_init_cfg.i2s_cfg.tdm_ch;
			ret = ctl_aud_module_init_cfg(CTL_AUD_ID_OUT, (CHAR *)p_aud_init->driver_name, &cfg_obj);
			if(ret) {
				DBG_ERR("aud init_cfg failed(%d)!\r\n", ret);
				ret = ISF_ERR_INVALID_VALUE;
				break;
			}

			p_ctx->isf_audout_tdm_ch = p_aud_init->aud_init_cfg.i2s_cfg.tdm_ch;

			break;
		}
		case AUDOUT_PARAM_ALLOC_BUFF: {
			AUDOUT_AUD_MEM *p_aud_mem = (AUDOUT_AUD_MEM *)p_struct;

			p_ctx->g_isf_audout_maxmem.addr = nvtmpp_sys_pa2va(p_aud_mem->pa);
			p_ctx->g_isf_audout_maxmem.size = p_aud_mem->size;
			break;
		}
		default:
			ret = ISF_ERR_NOT_SUPPORT;
			DBG_ERR("audout.ctrl set param[%08x]\r\n", param);
			break;
		}
	}
	return ret;
}

ISF_RV _isf_audout_getportstruct(UINT32 id, ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 *p_struct, UINT32 size)
{
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_INIT; //sitll not init
	}

	DBG_UNIT("audout port[%d] param = 0x%X size = %d\r\n", nport-ISF_IN_BASE, param, size);
	if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
	/* for output port */
		switch (param) {
		case AUDOUT_PARAM_CHN_STATE: {
			if (p_ctx->isf_audout_started == TRUE) {
				AUDOUT_CHN_STATE *p_chn_state = (AUDOUT_CHN_STATE *)p_struct;
				WAVSTUD_ACT act = (id == DEV_ID_0)? WAVSTUD_ACT_PLAY : WAVSTUD_ACT_PLAY2;
				UINT32 remain_buf = wavstudio_get_remain_buf(act);
				UINT32 unit_size = wavstudio_get_config(WAVSTUD_CFG_BUFF_UNIT_SIZE, act, 0);

				p_chn_state->total_num = p_ctx->wsis_max.buf_count - 1;
				p_chn_state->free_num = remain_buf/unit_size;
				p_chn_state->busy_num  = p_chn_state->total_num - p_chn_state->free_num;
			} else {
				AUDOUT_CHN_STATE *p_chn_state = (AUDOUT_CHN_STATE *)p_struct;

				p_chn_state->total_num = p_ctx->wsis_max.buf_count - 1;
				p_chn_state->free_num = 0;
				p_chn_state->busy_num  = 0;
			}

			return ISF_OK;
		}

		default:
			DBG_ERR("audout.out[%d] get struct[%08x] = %08x\r\n", nport-ISF_OUT_BASE, param, (UINT32)p_struct);
			break;
		}
	} else if (nport-ISF_IN_BASE < p_thisunit->port_in_count) {
	/* for input port */
	} else if (nport == ISF_CTRL) {
		/* for common port */
		switch (param) {
		case AUDOUT_PARAM_SYS_INFO: {
			AUDOUT_SYSINFO *p_sys_info = (AUDOUT_SYSINFO *)p_struct;
			UINT8 i;

			p_sys_info->cur_out_sample_rate = p_ctx->wsis.aud_info.aud_sr;
			p_sys_info->cur_sample_bit      = p_ctx->wsis.aud_info.bitpersample;
			p_sys_info->cur_mode            = p_ctx->wsis.aud_info.ch_num;
			for (i = 0; i < AUDOUT_MAX_IN; i++) {
				p_sys_info->cur_in_sample_rate[i] = 0;
			}
			return ISF_OK;
		}

		default:
			DBG_ERR("audout.ctrl get struct[%08x] = %08x\r\n", param, (UINT32)p_struct);
			break;
		}
	}


	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_audout_do_pause(ISF_UNIT *p_thisunit)
{
	if (FALSE == wavstudio_pause(WAVSTUD_ACT_PLAY)) {
		DBG_ERR("audout pause play failed\r\n");
		return ISF_ERR_PROCESS_FAIL;
	}

	return ISF_OK;
}
static ISF_RV _isf_audout_do_resume(ISF_UNIT *p_thisunit)
{
	if (FALSE == wavstudio_resume(WAVSTUD_ACT_PLAY)) {
		DBG_ERR("audout resume play failed\r\n");
		return ISF_ERR_PROCESS_FAIL;
	}

	return ISF_OK;
}


ISF_RV _isf_audout_updateport(UINT32 id, struct _ISF_UNIT *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd)
{
	ISF_RV r = ISF_OK;
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_UNINIT; //still not init
	}

	//ISF_PORT* p_port = ImageUnit_Out(p_thisunit, oport);
	//if(!p_port)
	//    return ISF_ERR_FAILED;

	switch (cmd) {
	case ISF_PORT_CMD_OFFTIME_SYNC:   ///< off -> off     (apply off-time parameter if it is dirty)
		_isf_audout_update_port_info(id, p_thisunit, oport);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(off-sync)");
		break;
	case ISF_PORT_CMD_OPEN:             ///< off -> ready   (alloc mempool and start task)
		_isf_audout_update_port_info(id, p_thisunit, oport);
		r = _isf_audout_do_open(id, p_thisunit, oport);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(open)");
		break;
	case ISF_PORT_CMD_READYTIME_SYNC: ///< ready -> ready (apply ready-time parameter if it is dirty)
		_isf_audout_update_port_info(id, p_thisunit, oport);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(ready-sync)");
		break;
	case ISF_PORT_CMD_START:            ///< ready -> run   (initial context, engine enable and device power on, start data processing)
		_isf_audout_update_port_info(id, p_thisunit, oport);
		r = _isf_audout_do_start(id, p_thisunit, oport);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(start)");
		break;
	case ISF_PORT_CMD_RUNTIME_SYNC:     ///< run -> run     (sync port info & sync run-time parameter if it is dirty)
		//_isf_audout_update_port_info(id, p_thisunit, oport);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(run-sync)");
		break;
	case ISF_PORT_CMD_RUNTIME_UPDATE:   ///< run -> run     (apply run-time parameter if it is dirty)
		_isf_audout_do_runtime(id, p_thisunit, cmd, oport);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(run-update)");
		break;
	case ISF_PORT_CMD_PAUSE:            ///< run -> wait    (pause data processing, keep context, engine or device enter suspend mode)
		r = _isf_audout_do_pause(p_thisunit);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(pause)");
		break;
	case ISF_PORT_CMD_RESUME:           ///< wait -> run    (engine or device leave suspend mode, restore context, resume data processing)
		r = _isf_audout_do_resume(p_thisunit);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(resume)");
		break;
	case ISF_PORT_CMD_STOP:             ///< run -> stop    (stop data processing, engine disable or device power off)
		r = _isf_audout_do_stop(id, p_thisunit);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(stop)");
		break;
	case ISF_PORT_CMD_CLOSE:            ///< stop -> off    (terminate task and free mempool)
		r = _isf_audout_do_close(id, p_thisunit);
		_isf_audout_buff_timeout_unlock(id);
		_isf_audout_buff_size_unlock(id);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(close)");
		break;
	case ISF_PORT_CMD_RESET:
		_isf_audout_do_reset(id, p_thisunit, oport);
		break;
	default:
		break;
	}
	return r;
}


BOOL audout_cb(PAUDIO_BUF_QUEUE pAudBufQue, PAUDIO_BUF_QUEUE pAudBufQue_AEC, UINT32 id, UINT64 timestamp, UINT64 timestamp_aec)
{
	/*if (g_dbgmsg) {
		DBG_MSG("No data buffer\r\n");
	}
	if(1) {
		//gen a dummy fail log
		isf_audout.p_base->do_release(&isf_audout, ISF_IN(0), NULL, 0, ISF_ERR_QUEUE_EMPTY);
	}*/

	return FALSE;
}

static void _audout_evtcb(WAVSTUD_ACT act, UINT32 p1, UINT32 p2)
{
	UINT32 id = (act == WAVSTUD_ACT_PLAY)?  DEV_ID_0 : DEV_ID_1;
	AUDOUT_CONTEXT_DEV* p_ctx = (AUDOUT_CONTEXT_DEV*)(&g_ao_ctx.dev[id]);

	switch (p1) {
	case WAVSTUD_CB_EVT_BUF_DONE: {
			p_ctx->played_size = p2;
			_isf_audout_buff_timeout_unlock(id);
			_isf_audout_buff_size_unlock(id);
			break;
		}
	case WAVSTUD_CB_EVT_STARTED: {
			_isf_audout_buff_timeout_unlock(id);
			_isf_audout_buff_size_unlock(id);
			break;
		}
	case WAVSTUD_CB_EVT_PRELOAD_DONE: {
			p_ctx->preload_done = TRUE;
			break;
		}
	default:
		break;
	}
}


///////////////////////////////////////////////////////////////////////////////
/*
ISF_PORT_PATH isf_audout_pathlist[8] = {
	{&isf_audout, ISF_IN(0), ISF_OUT(0)},
	{&isf_audout, ISF_IN(0), ISF_OUT(0)},
	{&isf_audout, ISF_IN(0), ISF_OUT(0)},
	{&isf_audout, ISF_IN(0), ISF_OUT(0)},
	{&isf_audout, ISF_IN(0), ISF_OUT(0)},
	{&isf_audout, ISF_IN(0), ISF_OUT(0)},
	{&isf_audout, ISF_IN(0), ISF_OUT(0)},
	{&isf_audout, ISF_IN(0), ISF_OUT(0)}
};

ISF_UNIT isf_audout = {
	.sign = ISF_SIGN_UNIT,
	.unit_name = "audout",
	.unit_module = MAKE_NVTMPP_MODULE('a', 'u', 'd', 'o', 'u', 't', 0, 0),
	.port_in_count = 1,
	.port_out_count = 1,
	.port_path_count = 8,
	.port_in = isf_audout_inputportlist,
	.port_out = isf_audout_outputportlist,
	.port_outcfg = isf_audout_outputportlist_cfg,
	.port_incaps = isf_audout_inputportlist_caps,
	.port_outcaps = isf_audout_outputportlist_caps,
	.port_outstate = isf_audout_outputportlist_state,
	.port_ininfo = isf_audout_outputinfolist_in,
	.port_outinfo = isf_audout_outputinfolist_out,
	.port_path = isf_audout_pathlist,
	.do_bindinput = _isf_audout_bindinput,
	.do_bindoutput = NULL,
	.do_setportparam = _isf_audout_setportparam,
	.do_getportparam = _isf_audout_getportparam,
	.do_getportstruct = _isf_audout_getportstruct,
	.do_update = _isf_audout_updateport,
};
*/
///////////////////////////////////////////////////////////////////////////////



