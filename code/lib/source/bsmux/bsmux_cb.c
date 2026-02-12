/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/

// INCLUDE PROTECTED
#include "bsmux_init.h"

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static BSMUX_EVENT_CB g_event_cb = NULL;
static VOID *         g_user_data_cb = NULL;
static UINT32         g_debug_cbinfo = BSMUX_DEBUG_CBINFO;
#define BSMUX_CB_DUMP(fmtstr, args...) do { if(g_debug_cbinfo) DBG_DUMP(fmtstr, ##args); } while(0)
#define BSMUX_CB_IND(fmtstr, args...) do { if(g_debug_cbinfo) DBG_IND(fmtstr, ##args); } while(0)
extern BOOL           g_bsmux_debug_msg;
#define BSMUX_MSG(fmtstr, args...) do { if(g_bsmux_debug_msg) DBG_DUMP(fmtstr, ##args); } while(0)

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/

static INT32 _bsmux_cb_buf_callback(void *p_data)
{
	BSMUXER_OUT_BUF *CBInfo = (BSMUXER_OUT_BUF *)p_data;
	BSMUXER_FILE_BUF bsbuf = {0};
	UINT32 t_dur_us = 0;

	if (CBInfo->addr || CBInfo->fileop) {
		vos_perf_mark(&CBInfo->resv[12]);
		bsbuf.pathID = CBInfo->pathID;
		bsbuf.event  = CBInfo->event;
		bsbuf.fileop = CBInfo->fileop;
		bsbuf.addr   = CBInfo->addr;
		bsbuf.size   = CBInfo->size;
		bsbuf.pos    = CBInfo->pos;
		bsbuf.type   = CBInfo->type;
		// release return buf
		bsmux_util_release_buf(&bsbuf);
		if (CBInfo->ret_push != 0) {
			DBG_ERR("push fail %d, p%d,%x %x %x\r\n",
				CBInfo->ret_push,
				CBInfo->pathID,
				CBInfo->addr,
				(unsigned int)CBInfo->size,
				CBInfo->fileop);
		}

		bsmux_util_get_param(CBInfo->pathID, BSMUXER_PARAM_DUR_US_MAX, &t_dur_us);
		if (vos_perf_duration(CBInfo->resv[11], CBInfo->resv[12]) > (VOS_TICK)t_dur_us) {
			BSMUX_MSG("bsmux_cb_buf:[%d][%d][%d]\r\n", CBInfo->pathID, CBInfo->fileop,
				vos_perf_duration(CBInfo->resv[11], CBInfo->resv[12]) / 1000);
		}
	}
	BSMUX_CB_DUMP("callback:[%d] fileop 0x%lx addr 0x%lx size 0x%lx\r\n", CBInfo->pathID,
		(unsigned long)CBInfo->fileop, (unsigned long)CBInfo->addr, (unsigned long)CBInfo->size);

	return 0;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void bsmux_cb_register(BSMUX_EVENT_CB new_cb)
{
	if (bsmux_util_is_null_obj((void *)new_cb)) {
		DBG_ERR("new_cb is NULL, skip\r\n");
		return;
	}

	g_event_cb = new_cb;
	BSMUX_CB_IND("register callback 0x%X\r\n", g_event_cb);
}

ER bsmux_cb_buf(BSMUXER_FILE_BUF *p_buf, UINT32 id)
{
	BSMUXER_CBINFO cb_param = {0};
	BSMUXER_OUT_BUF out_buf = {0};
	INT32 ret;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id invalid\r\n");
		return E_ID;
	}

	if (bsmux_util_is_null_obj((void *)g_event_cb)) {
		DBG_ERR("callback not registered\r\n");
		return E_NOEXS;
	}

	if (bsmux_util_is_null_obj((void *)p_buf)) {
		DBG_ERR("out buf is empty\r\n");
		return E_NOEXS;
	}

	// package out buf
	out_buf.sign      = MAKEFOURCC('F', 'O', 'U', 'T');
	out_buf.event     = p_buf->event;
	out_buf.fileop    = p_buf->fileop;
	out_buf.addr      = p_buf->addr;
	out_buf.size      = p_buf->size;
	out_buf.pos       = p_buf->pos;
	out_buf.type      = p_buf->type;
	out_buf.pathID    = p_buf->pathID;
	out_buf.fp_pushed = _bsmux_cb_buf_callback;  //for release
	vos_perf_mark(&out_buf.resv[11]);
	if (g_user_data_cb) out_buf.p_user_data = g_user_data_cb;

	// package cbinfo
	cb_param.id = id;
	cb_param.out_data = &out_buf;
	cb_param.errcode  = BSMUXER_RESULT_NORMAL;
	cb_param.cb_event = BSMUXER_CBEVENT_FOUTREADY;

	// dump cbinfo
	BSMUX_CB_DUMP("cb_buf:[%d] fileop 0x%lx addr 0x%lx size 0x%lx\r\n", id,
		(unsigned long)out_buf.fileop, (unsigned long)out_buf.addr, (unsigned long)out_buf.size);

	// callback
	ret = g_event_cb("BsMux", &cb_param, NULL);
	if (0 != ret) {
		DBG_ERR("cb buf 0x%X failed, id %d\r\n", (unsigned int)p_buf, id);
		return E_SYS;
	}

	return E_OK;
}

ER bsmux_cb_event(BSMUXER_CBEVENT event, void *p_data, UINT32 id)
{
	BSMUXER_CBINFO cb_param = {0};
	BSMUXER_OUT_BUF out_buf = {0};
	INT32 ret;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id invalid\r\n");
		return E_ID;
	}

	if (bsmux_util_is_null_obj((void *)g_event_cb)) {
		DBG_ERR("callback not registered\r\n");
		return E_NOEXS;
	}

	switch (event) {
	case BSMUXER_CBEVENT_PUTBSDONE:
		{ //p0: bufID, p2: type, p3: key
			BSMUXER_DATA *pBSBuf = (BSMUXER_DATA *)p_data;
			out_buf.resv[0] = pBSBuf->bufid;
			if (pBSBuf->type == BSMUX_TYPE_VIDEO) {
				out_buf.resv[2] = MAKEFOURCC('V','S','T','M');
				out_buf.resv[3] = pBSBuf->isKey;
				out_buf.resv[5] = 0; //main-stream
			} else if (pBSBuf->type == BSMUX_TYPE_SUBVD) {
				out_buf.resv[2] = MAKEFOURCC('V','S','T','M');
				out_buf.resv[3] = pBSBuf->isKey;
				out_buf.resv[5] = 1; //sub-stream
			} else if (pBSBuf->type == BSMUX_TYPE_AUDIO) {
				out_buf.resv[2] = MAKEFOURCC('A','S','T','M');
			}
			if (pBSBuf->user_data) g_user_data_cb = pBSBuf->user_data;
			if (g_user_data_cb) out_buf.p_user_data = g_user_data_cb;
			DBG_IND("bsmux[%d] put bs done\r\n", id);
		}
		break;
	case BSMUXER_CBEVENT_COPYBSBUF:
		{
			DBG_IND("bsmux[%d] copy done\r\n", id);
		}
		break;
	case BSMUXER_CBEVENT_KICKTHUMB:
		{
			DBG_IND("bsmux[%d] kick thumbnail\r\n", id);
		}
		break;
	case BSMUXER_CBEVENT_CUT_BEGIN:
		{
			UINT32 vfn = 0;
			bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
			out_buf.resv[4] = vfn;
			DBG_IND("bsmux[%d] vfn(%d) cut begin\r\n", id, vfn);
		}
		break;
	case BSMUXER_CBEVENT_CUT_COMPLETE:
		{
			UINT32 vfn = 0;
			bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
			out_buf.resv[4] = vfn;
			if (g_user_data_cb) g_user_data_cb = NULL;
			DBG_IND("bsmux[%d] vfn(%d) cut complete\r\n", id, vfn);
		}
		break;
	default:
		break;
	}

	// package cbinfo
	cb_param.id = id;
	cb_param.out_data = &out_buf;
	cb_param.errcode  = BSMUXER_RESULT_NORMAL;
	cb_param.cb_event = event;

	// dump cbinfo
	BSMUX_CB_IND("cb event %d\r\n", cb_param.cb_event);

	// callback
	ret = g_event_cb("BsMux", &cb_param, NULL);
	if (0 != ret) {
		DBG_ERR("cb event %d failed, id %d\r\n", event, id);
		return E_SYS;
	}

	return E_OK;
}

ER bsmux_cb_result(BSMUXER_RESULT result, UINT32 id)
{
	BSMUXER_CBINFO cb_param = {0};
	BSMUXER_OUT_BUF out_buf = {0};
	INT32 ret;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("id invalid\r\n");
		return E_ID;
	}

	if (bsmux_util_is_null_obj((void *)g_event_cb)) {
		DBG_ERR("callback not registered\r\n");
		return E_NOEXS;
	}

	// package cbinfo
	cb_param.id = id;
	cb_param.out_data = &out_buf;
	cb_param.errcode = result;
	cb_param.cb_event = BSMUXER_CBEVENT_CLOSE_RESULT;

	// dump cbinfo
	BSMUX_CB_IND("cb result %d\r\n", cb_param.errcode);

	if (bsmux_util_is_not_normal()) {
		DBG_DUMP("No Need To Callback Errcode(%d)\r\n", cb_param.errcode);
		return E_OK;
	}

	// set module
	bsmux_util_set_result(result);

	bsmux_util_dump_info(id);

	// callback
	ret = g_event_cb("BsMux", &cb_param, NULL);
	if (0 != ret) {
		DBG_ERR("cb result %d failed, id %d\r\n", result ,id);
		return E_SYS;
	}

	return E_OK;
}

ER bsmux_cb_dbg(UINT32 value)
{
	g_debug_cbinfo = value;
	return E_OK;
}

