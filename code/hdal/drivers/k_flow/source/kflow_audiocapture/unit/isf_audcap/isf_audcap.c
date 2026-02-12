#include "isf_audcap_int.h"

///////////////////////////////////////////////////////////////////////////////

#if defined (DEBUG) && defined(__FREERTOS)
unsigned int isf_audcap_debug_level = THIS_DBGLVL;
#endif

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

#define TEST_SIN_PATT DISABLE
#define ANR_FUNC ENABLE
#define AEC_FUNC ENABLE
#define SUPPORT_PULL_FUNCTION ENABLE
#define DUMP_TO_FILE_TEST_KERNEL_VER DISABLE
#define AGC_FUNC ENABLE

#define PERF_CHK DISABLE

#if (DUMP_TO_FILE_TEST_KERNEL_VER == ENABLE)
#include <kwrap/file.h>

static int g_wo_fd = 0;
static UINT8 temp_buf_r[2000000] = {0};
static UINT8 temp_buf_l[2000000] = {0};
static UINT32 temp_cnt_r = 0;
static UINT32 temp_cnt_l = 0;
static UINT32 file_cnt = 0;
#endif

#define ISF_UNIT_TRACE(opclass, p_thisunit, port, fmtstr, args...) (p_thisunit)->p_base->do_trace(p_thisunit, port, opclass, fmtstr, ##args)
///////////////////////////////////////////////////////////////////////////////

static ISF_RV _isf_audcap_lock_cb(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 hData);
static ISF_RV _isf_audcap_unlock_cb(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 hData);
extern ISF_UNIT isf_audcap;
static WAVSTUD_APPOBJ wso = {0};
static WAVSTUD_INFO_SET wsis = {
	.obj_count			= 1,
	.data_mode			= WAVSTUD_DATA_MODE_PUSH,

	.aud_info.aud_sr	= AUDIO_SR_48000,
	.aud_info.aud_ch	= AUDIO_CH_STEREO,
	.aud_info.bitpersample = 16,
	.aud_info.ch_num		= 2,
	.aud_info.buf_sample_cnt     = 1024,

	.buf_count			= 5,
	.unit_time			= 10,
};

static WAVSTUD_INFO_SET wsis_max = {
	.obj_count			= 1,
	.data_mode			= WAVSTUD_DATA_MODE_PUSH,

	.aud_info.aud_sr		= AUDIO_SR_48000,
	.aud_info.aud_ch		= AUDIO_CH_STEREO,
	.aud_info.bitpersample = 16,
	.aud_info.ch_num		= 2,
	.aud_info.buf_sample_cnt     = 1024,

	.buf_count			= 5,
	.unit_time			= 10,
};

static ISF_DATA_CLASS _isf_audcap_base = {
	.this = MAKEFOURCC('A','C','A','P'),
	.base = MAKEFOURCC('A','F','R','M'),
	.do_lock   = _isf_audcap_lock_cb,
	.do_unlock = _isf_audcap_unlock_cb,
};

static BOOL _isf_audcap_cb(PAUDIO_BUF_QUEUE pAudBufQue, PAUDIO_BUF_QUEUE pAudBufQue_AEC, UINT32 id, UINT64 timestamp, UINT64 timestamp_aec);
static void _isf_audcap_audproc_cb(UINT32 addr, UINT32 addr_aec, UINT32 size, UINT32 size_aec);
static void _isf_audcap_evt_cb(WAVSTUD_ACT act, UINT32 p1, UINT32 p2);
static ISF_DATA isf_audcap_memblk = {0};
static ISF_DATA isf_audcap_bsmemblk[ISF_AUDCAP_IN_NUM] = {0};
static ISF_DATA isf_audcap_bufinfomemblk[ISF_AUDCAP_IN_NUM] = {0};
static ISF_DATA isf_audcap_aecmemblk = {0};
static ISF_DATA isf_audcap_anrmemblk = {0};
static ISF_DATA isf_audcap_srcmemblk = {0};
static ISF_DATA isf_audcap_srcbufmemblk = {0};
static ISF_DATA isf_audcap_aec_pre_init_bufmemblk = {0};
static ISF_DATA isf_audcap_aec_ref_bypass_bufmemblk = {0};
static ISF_DATA isf_audcap_bsdata = {
	.sign      = ISF_SIGN_DATA,            ///< signature, equal to ISF_SIGN_DATA or ISF_SIGN_EVENT
	.h_data     = 1,
	.p_class   = &_isf_audcap_base,
};
static UINT32 g_volume = 100;
static BOOL   g_isf_audcap_trace_on     = FALSE;
//static UINT32 g_isf_audcap_codec         = 0;
static UINT32 g_isf_audcap_audcodec      = 0;
static UINT32 g_isf_audcap_frame_sample  = 1024;
static UINT32 g_isf_audcap_frame_block_sample  = 0;
static UINT32 g_isf_audcap_max_frame_sample  = 1024;
static UINT32 g_isf_audcap_buf_cnt = 5;


static AUDCAP_BSDATA           *g_p_audcap_bsdata_blk_pool                          = NULL;
static UINT32                 *g_p_audcap_bsdata_blk_maptlb                        = NULL;
static AUDCAP_BUF_INFO         *g_p_audcap_bufinfo_blk_pool                          = NULL;
static UINT32                 *g_p_audcap_bufinfo_blk_maptlb                        = NULL;
static PAUDCAP_AEC_OBJ         p_aec_obj   = NULL;
static PAUDCAP_ANR_OBJ         p_anr_obj   = NULL;
static PAUDCAP_AUDFILT_OBJ     p_audfilt_obj   = NULL;
static BOOL                   audfilt_en   = FALSE;
static MEM_RANGE              g_isf_audcap_maxmem = {0};
static BOOL                   g_isf_audcap_allocmem = FALSE;
static MEM_RANGE              g_isf_audcap_aecmem = {0};
static MEM_RANGE              g_isf_audcap_anrmem = {0};
static MEM_RANGE              g_isf_audcap_srcmem = {0};
static MEM_RANGE              g_isf_audcap_aec_ref_bypass_mem = {0};
static MEM_RANGE              g_isf_audcap_srcbufmem = {0};
static MEM_RANGE              g_isf_audcap_aec_pre_init_mem = {0};
static AUDIO_CH               isf_audcap_mono_ch = AUDIO_CH_RIGHT;
static KDRV_AUDIOLIB_FUNC*    paudsrc_obj   = NULL;
static KDRV_AUDIOLIB_FUNC*    paudagc_obj   = NULL;
static int                    resample_handle_rec = 0;
static MEM_RANGE              isf_audcap_last_left[ISF_AUDCAP_IN_NUM] = {0};
static AUDCAP_BUF_INFO         *p_isf_audcap_remain_info[ISF_AUDCAP_IN_NUM] = {0};
static AUDCAP_TIMESTAMP        isf_audcap_timestamp = {0};
static BOOL                   isf_audcap_init = FALSE;
static UINT32                 isf_audcap_path_open_count = 0;
static UINT32                 isf_audcap_path_start_count = 0;
static BOOL                   isf_audcap_dual_mono = FALSE;
static BOOL                   isf_audcap_aec_lb_swap = FALSE;
static INT32                  isf_audcap_alc_en = -1;
static UINT32                 isf_audcap_rec_src = 0;
static INT32                  isf_audcap_prepwr_en = 0;
static MEM_RANGE              isf_audcap_aec_last_left[ISF_AUDCAP_IN_NUM] = {0};
static INT32                  isf_audcap_tdm_ch = 0;
static UINT32                 isf_audcap_gain_level = KDRV_AUDIO_CAP_GAIN_LEVEL8;
static AUDCAP_AEC_CONFIG      aec_config_max = {0};
static AUDCAP_ALC_CONFIG      isf_audcap_alc_cfg ={0};
static BOOL                   mono_expand = FALSE;
static BOOL                   g_isf_audcap_aec_ref_bypass_func = FALSE;
static BOOL                   g_isf_audcap_aec_ref_bypass_enable = FALSE;
static BOOL                   g_isf_audcap_aec_ref_bypass = FALSE;
static BOOL                   g_isf_audcap_aec_pre_init = FALSE;
static BOOL                   g_isf_audcap_aec_pre_done = FALSE;

static UINT32                 g_audcap_bsdata_num = 0;
static UINT32                 g_audcap_pull_failed_count = 0;

static UINT32 g_audcap_init[ISF_FLOW_MAX] = {0};

#if PERF_CHK == ENABLE
#include "kwrap/perf.h"
static VOS_TICK aec_time = 0;
static UINT32 aec_count = 0;
static VOS_TICK anr_time = 0;
static UINT32 anr_count = 0;
#endif

#if 0
static AUDCAP_BSDATA           *g_p_audcap_bsdata_link_head[ISF_AUDCAP_OUT_NUM]      = {0};
static AUDCAP_BSDATA           *g_p_audcap_bsdata_link_tail[ISF_AUDCAP_OUT_NUM]      = {0};
static BOOL                   aec_en[ISF_AUDCAP_OUT_NUM]   = {0};
static BOOL                   anr_en   = FALSE;
static MEM_RANGE              g_isf_audcap_pathmem[ISF_AUDCAP_OUT_NUM] = {0};
static AUDIO_CH               isf_audout_ch[ISF_AUDCAP_OUT_NUM] = {0};
static AUDIO_CH               isf_audout_ch_num[ISF_AUDCAP_OUT_NUM] = {0};
static AUDIO_SR               isf_audcap_resample_max[ISF_AUDCAP_OUT_NUM] = {0};
static AUDIO_SR               isf_audcap_resample[ISF_AUDCAP_OUT_NUM] = {0};
static AUDIO_SR               isf_audcap_resample_update[ISF_AUDCAP_OUT_NUM] = {0};
static BOOL                   isf_audcap_resample_dirty[ISF_AUDCAP_OUT_NUM] = {0};
static UINT32                 isf_audcap_resample_frame[ISF_AUDCAP_OUT_NUM] = {0};
static BOOL                   isf_audcap_resample_en[ISF_AUDCAP_OUT_NUM] = {0};
static UINT32                 isf_audcap_started[ISF_AUDCAP_OUT_NUM] = {0};
static UINT32                 isf_audcap_opened[ISF_AUDCAP_OUT_NUM] = {0};
static MEM_RANGE              isf_audcap_output_mem[ISF_AUDCAP_OUT_NUM] = {0};
static AUDCAP_AGC_CONFIG      isf_audcap_agc_cfg[ISF_AUDCAP_OUT_NUM] = {0};
static KDRV_AUDIO_CAP_DEFSET  isf_audcap_defset[ISF_AUDCAP_OUT_NUM] = {KDRV_AUDIO_CAP_DEFSET_20DB, KDRV_AUDIO_CAP_DEFSET_20DB};
static INT32                  isf_audcap_ng_thd[ISF_AUDCAP_OUT_NUM] = {0}; //default seting 20 DB
static AUDCAP_PULL_QUEUE      isf_audcap_pull_que[ISF_AUDCAP_OUT_NUM]              = {0};
static AUDCAP_MMAP_MEM_INFO   isf_audcap_mmap_info[ISF_AUDCAP_OUT_NUM]           = {0};
SEM_HANDLE                      ISF_AUDCAP_SEM_ID[ISF_AUDCAP_OUT_NUM]         = {0};
SEM_HANDLE                      ISF_AUDCAP_PULLQ_SEM_ID[ISF_AUDCAP_OUT_NUM]         = {0};
#endif

static AUDCAP_CONTEXT g_ac_ctx = {0};
UINT32 g_audcap_max_count;


#define AUDCAP_VIRT2PHY(pathID, virt_addr) (g_ac_ctx.port[pathID].isf_audcap_mmap_info.addr_physical + (virt_addr - g_ac_ctx.port[pathID].isf_audcap_mmap_info.addr_virtual))

#if (SUPPORT_PULL_FUNCTION)
BOOL _isf_audcap_is_pullque_full(UINT32 pathID);
BOOL _isf_audcap_put_pullque(UINT32 pathID, ISF_DATA *data_in);
ISF_RV _isf_audcap_get_pullque(UINT32 pathID, ISF_DATA *data_out, INT32 wait_ms);
BOOL _isf_audcap_release_all_pullque(UINT32 pathID);
#endif
SEM_HANDLE ISF_AUDCAP_PROC_SEM_ID = {0};
SEM_HANDLE ISF_AUDCAP_COMM_SEM_ID = {0};

static ISF_RV _isf_audcap_bs_data_init(void);
static void _isf_audcap_bs_queue_init(void);
static BOOL _isf_audcap_src_check(UINT32 resample);

#if 1
static const short sin_table[48] = {
	0,2138,4240,6269,8191,9973,
	11585,12998,14188,15136,15825,16243,
	16383,16243,15825,15136,14188,12998,
	11585,9973,8191,6269,4240,2138,
	0,-2138,-4240,-6269,-8191,-9973,
	-11585,-12998,-14188,-15136,-15825,-16243,
	-16383,-16243,-15825,-15136,-14188,-12998,
	-11585,-9973,-8192,-6269,-4240,-2138
};

static UINT32 sin_count = 0;
#endif

#if AEC_FUNC
#include "../include/nvtaec_api.h"

AUDCAP_AEC_OBJ aec_obj = {nvtaec_open,
							   nvtaec_close,
							   nvtaec_start,
							   nvtaec_stop,
							   nvtaec_apply,
							   nvtaec_enable,
							   nvtaec_get_require_bufsize,
							   nvtaec_set_require_buf,
							  };
#endif

#if (ANR_FUNC == ENABLE)
#include "../include/nvtanr_api.h"
AUDCAP_ANR_OBJ anr_obj = {nvtanr_open,
						 nvtanr_close,
						 nvtanr_apply,
						 nvtanr_enable,
						 nvtanr_getmem,
						 nvtanr_setmem
						};
#endif

#include "kdrv_audioio/audlib_src.h"

#if (AGC_FUNC == ENABLE)
#include "kdrv_audioio/audlib_agc.h"
#endif

#include "kdrv_audioio/kdrv_audioio.h"

BOOL _isf_audcap_sxcmd_otrace(CHAR *strCmd)
{
	g_isf_audcap_trace_on = TRUE;
	return TRUE;
}

BOOL _isf_audcap_sxcmd_ctrace(CHAR *strCmd)
{
	g_isf_audcap_trace_on = FALSE;
	return TRUE;
}

BOOL _isf_audcap_sxcmd_dumpque(CHAR *strCmd)
{
	AUDCAP_BSDATA *pAudCapData = NULL;
	UINT32 i;

	//loc_cpu();
	for (i = 0; i < ISF_AUDCAP_IN_NUM; i++) {
		AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];

		if (p_ctx_p->g_p_audcap_bsdata_link_head == NULL) {
			break;
		}

		pAudCapData = p_ctx_p->g_p_audcap_bsdata_link_head;
		DBG_DUMP("========Stream%d========\r\n", i);
		while (pAudCapData != NULL) {
			DBG_DUMP("Queue hdata = 0x%x, blk_id = %d, size = %d\r\n", pAudCapData->hData, pAudCapData->blk_id, pAudCapData->mem.size);
			if (pAudCapData->pnext_bsdata) {
				DBG_DUMP("next = 0x%x\r\n", ((AUDCAP_BSDATA *)pAudCapData->pnext_bsdata)->hData);
			}
			pAudCapData = pAudCapData->pnext_bsdata;
		}

		DBGH(p_ctx_p->g_p_audcap_bsdata_link_tail->hData);
		DBG_DUMP("======================\r\n");
	}
	//unl_cpu();
	return TRUE;
}

BOOL _isf_audcap_sxcmd_dumpbuf(CHAR *strCmd)
{
	UINT32 i;

	if (g_p_audcap_bsdata_blk_maptlb == NULL) {
		return TRUE;
	}

	for (i = 0; i < (PATH_MAX_COUNT * ISF_AUDCAP_BSDATA_BLK_MAX); i++) {
		if (i > 0 && (i % 16 == 0)) {
			DBG_DUMP("\r\n");
		}
		DBG_DUMP("0x%02x ", g_p_audcap_bsdata_blk_maptlb[i]);
	}
	return TRUE;
}

BOOL _isf_audcap_sxcmd_dump_que_num(CHAR *strCmd)
{
	DBG_DUMP("BS queue num = %d\r\n", g_audcap_bsdata_num);

	return TRUE;
}

#if defined __UITRON || defined __ECOS
static BOOL   g_audcapcmd_installed      = FALSE;

static SXCMD_BEGIN(isfai, "isf_audcap command")
SXCMD_ITEM("ot", _isf_audcap_sxcmd_otrace, "Trace code open")
SXCMD_ITEM("ct", _isf_audcap_sxcmd_ctrace, "Trace code cancel")
SXCMD_ITEM("que", _isf_audcap_sxcmd_dumpque, "Dump queue")
SXCMD_END()

static void _isf_audcap_install_cmd(void)
{
	SxCmd_AddTable(isfac);
}
#endif

#if (SUPPORT_PULL_FUNCTION)
static ISF_RV _isf_audcap_outputport_pullbuf(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	ISF_RV ret = ISF_OK;
	ISF_UNIT *p_thisunit = &isf_audcap;
	UINT32 id = oport - ISF_OUT_BASE;
	AUDCAP_CONTEXT_PORT *p_ctx_p;
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[id];

	if (p_ctx_p->isf_audcap_opened == FALSE) {
		return ISF_ERR_NOT_OPEN_YET;
	}

	ret = _isf_audcap_get_pullque(oport, p_data, wait_ms);

	p_thisunit->p_base->do_probe(p_thisunit, oport, ISF_USER_PROBE_PULL, ret);

	return ret;
}

BOOL _isf_audcap_is_pullque_all_released(UINT32 pathID)
{
	PAUDCAP_PULL_QUEUE p_obj = NULL;
	AUDCAP_BSDATA *pAudCapData = NULL;
	UINT32 pull_queue_num = 0;
	UINT32 link_list_num = 0;
	AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[pathID];
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;
	UINT32 pull_queue_max = p_ctx_c->ctx_mem.pull_queue_max;

	if (p_ctx_p->put_pullq == FALSE) {
		return TRUE;
	}

	SEM_WAIT(p_ctx_p->ISF_AUDCAP_SEM_ID);

	// PullQueue remain number
	p_obj = &(p_ctx_p->isf_audcap_pull_que);
	if (p_obj->Rear > p_obj->Front) {
		pull_queue_num = p_obj->Rear - p_obj->Front;
	} else if (p_obj->Rear < p_obj->Front){
		pull_queue_num = pull_queue_max + p_obj->Rear - p_obj->Front;
	} else if (p_obj->Rear == p_obj->Front && p_obj->bFull == TRUE){
		pull_queue_num = pull_queue_max;
	} else {
		pull_queue_num = 0;
	}

	// link-list remain number
	pAudCapData = p_ctx_p->g_p_audcap_bsdata_link_head;
	while (pAudCapData != NULL) {
		link_list_num++;
		pAudCapData = pAudCapData->pnext_bsdata;
	}

	SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_SEM_ID);

	DBG_IND("[ISF_AUDCAP][%d] remain number => PullQueue(%u), link-list(%u)\r\n", pathID, pull_queue_num, link_list_num);

	return (pull_queue_num == link_list_num)? TRUE:FALSE;
}

BOOL _isf_audcap_is_pullque_empty(UINT32 pathID)
{
	PAUDCAP_PULL_QUEUE p_obj;
	BOOL is_empty = FALSE;
	AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[pathID];

	SEM_WAIT(p_ctx_p->ISF_AUDCAP_SEM_ID);

	p_obj = &(p_ctx_p->isf_audcap_pull_que);
	is_empty = ((p_obj->Front == p_obj->Rear) && (p_obj->bFull == FALSE));

	SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_SEM_ID);

	return is_empty;
}

BOOL _isf_audcap_is_pullque_full(UINT32 pathID)
{
	PAUDCAP_PULL_QUEUE p_obj;
	BOOL is_full = FALSE;
	AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[pathID];

	SEM_WAIT(p_ctx_p->ISF_AUDCAP_SEM_ID);

	p_obj = &(p_ctx_p->isf_audcap_pull_que);
	is_full = ((p_obj->Front == p_obj->Rear) && (p_obj->bFull == TRUE));

	SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_SEM_ID);

	return is_full;
}

void _isf_audcap_unlock_pullqueue(UINT32 pathID)
{
	AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[pathID];

	if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_AUDCAP_PULLQ_SEM_ID, vos_util_msec_to_tick(0))) {
		DBG_IND("[ISF_AUDCAP][%d] no data in pull queue, auto unlock pull blocking mode !!\r\n", pathID);
	}

	// [case_1]   no data in pullQ, then this 0 ->   0   -> 1 to unlock HDAL pull blocking mode (fake semaphore.... but it's OK, because _ISF_AudDec_Get_PullQueue() will return FALSE, there's no actual data in queue )
	// [case_2] have data in pullQ, then this n -> (n-1) -> n to recover the semaphore count
	SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_PULLQ_SEM_ID);
}

BOOL _isf_audcap_put_pullque(UINT32 pathID, ISF_DATA *data_in)
{
	PAUDCAP_PULL_QUEUE pObj;
	AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[pathID];
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;
	UINT32 pull_queue_max = p_ctx_c->ctx_mem.pull_queue_max;

	SEM_WAIT(p_ctx_p->ISF_AUDCAP_SEM_ID);
	pObj = &(p_ctx_p->isf_audcap_pull_que);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		//DBG_ERR("[ISF_AUDCAP][%d] Pull Queue is Full!\r\n", pathID);
		SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_SEM_ID);
		return FALSE;
	} else {
		memcpy(&pObj->Queue[pObj->Rear], data_in, sizeof(ISF_DATA));

		pObj->Rear = (pObj->Rear + 1) % pull_queue_max;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}

		p_ctx_p->put_pullq = TRUE;

		SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_PULLQ_SEM_ID); // PULLQ + 1
		SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_SEM_ID);
		return TRUE;
	}
}

ISF_RV _isf_audcap_get_pullque(UINT32 pathID, ISF_DATA *data_out, INT32 wait_ms)
{
	PAUDCAP_PULL_QUEUE pObj;
	AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[pathID];
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;
	UINT32 pull_queue_max = p_ctx_c->ctx_mem.pull_queue_max;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	if (wait_ms < 0) {
		// blocking (wait until data available) , if success PULLQ - 1 , else wait forever (or until signal interrupt and return FALSE)
		if (SEM_WAIT_INTERRUPTIBLE(p_ctx_p->ISF_AUDCAP_PULLQ_SEM_ID)) {
			return ISF_ERR_QUEUE_EMPTY;
		}
	} else  {
		// non-blocking (wait_ms=0) , timeout (wait_ms > 0). If success PULLQ - 1 , else just return FALSE
		if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_AUDCAP_PULLQ_SEM_ID, vos_util_msec_to_tick(wait_ms))) {

			if (g_isf_audcap_trace_on && g_audcap_pull_failed_count == 40) {
				_isf_audcap_sxcmd_dumpque("");
				_isf_audcap_sxcmd_dumpbuf("");
				_isf_audcap_sxcmd_dump_que_num("");
				wavstudio_dump_module_info(WAVSTUD_ACT_REC);
				wavstudio_dump_module_info(WAVSTUD_ACT_LB);
				g_audcap_pull_failed_count++;
			} else if (g_audcap_pull_failed_count < 40) {
				g_audcap_pull_failed_count++;

				//DBG_DUMP("g_audcap_pull_failed_count=%d\r\n", g_audcap_pull_failed_count);
			}

			DBG_IND("[ISF_AUDCAP][%d] Pull Queue Semaphore timeout!\r\n", pathID);
			return ISF_ERR_WAIT_TIMEOUT;
		}
	}

	// check state
	if (p_ctx_p->isf_audcap_opened == FALSE) {
		return ISF_ERR_NOT_OPEN_YET;
	}

	SEM_WAIT(p_ctx_p->ISF_AUDCAP_SEM_ID);
	pObj = &(p_ctx_p->isf_audcap_pull_que);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_SEM_ID);
		//DBG_ERR("[ISF_AUDCAP][%d] Pull Queue is Empty !!!\r\n", pathID);   // This should normally never happen !! If so, BUG exist. ( Because we already check & wait data available at upper code. )
		return ISF_ERR_QUEUE_EMPTY;
	} else {
		memcpy(data_out, &pObj->Queue[pObj->Front], sizeof(ISF_DATA));

		pObj->Front= (pObj->Front+ 1) % pull_queue_max;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = FALSE;
		}
		SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_SEM_ID);

		g_audcap_pull_failed_count = 0;

		return ISF_OK;
	}
}

BOOL _isf_audcap_release_all_pullque(UINT32 pathID)
{
	ISF_DATA p_data;

	//consume all pull queue
	while (ISF_OK == _isf_audcap_get_pullque(pathID, &p_data, 0));

	return TRUE;
}

#endif // #if (SUPPORT_PULL_FUNCTION)


static ISF_PORT_CAPS isf_audcap_inputport1_caps = {
	.connecttype_caps = ISF_CONNECT_NONE,
	.do_push = NULL,
};
static ISF_PORT isf_audcap_inputport1 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(0),
	.connecttype = ISF_CONNECT_NONE,
	.p_destunit = &isf_audcap,
	.p_srcunit = NULL,
};
static ISF_PORT *isf_audcap_inputportlist[1] = {
	&isf_audcap_inputport1,
};
static ISF_PORT_CAPS *isf_audcap_inputportlist_caps[1] = {
	&isf_audcap_inputport1_caps,
};

static ISF_PORT_CAPS isf_audcap_outputport1_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH  | ISF_CONNECT_PULL | ISF_CONNECT_SYNC,
	.do_pull = _isf_audcap_outputport_pullbuf,
};

static ISF_PORT_CAPS isf_audcap_outputport2_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH  | ISF_CONNECT_PULL | ISF_CONNECT_SYNC,
	.do_pull = _isf_audcap_outputport_pullbuf,
};

#if (ISF_AUDCAP_OUT_NUM == 8)
static ISF_PORT_CAPS isf_audcap_outputport3_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH  | ISF_CONNECT_PULL | ISF_CONNECT_SYNC,
	.do_pull = _isf_audcap_outputport_pullbuf,
};

static ISF_PORT_CAPS isf_audcap_outputport4_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH  | ISF_CONNECT_PULL | ISF_CONNECT_SYNC,
	.do_pull = _isf_audcap_outputport_pullbuf,
};

static ISF_PORT_CAPS isf_audcap_outputport5_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH  | ISF_CONNECT_PULL | ISF_CONNECT_SYNC,
	.do_pull = _isf_audcap_outputport_pullbuf,
};

static ISF_PORT_CAPS isf_audcap_outputport6_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH  | ISF_CONNECT_PULL | ISF_CONNECT_SYNC,
	.do_pull = _isf_audcap_outputport_pullbuf,
};

static ISF_PORT_CAPS isf_audcap_outputport7_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH  | ISF_CONNECT_PULL | ISF_CONNECT_SYNC,
	.do_pull = _isf_audcap_outputport_pullbuf,
};

static ISF_PORT_CAPS isf_audcap_outputport8_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH  | ISF_CONNECT_PULL | ISF_CONNECT_SYNC,
	.do_pull = _isf_audcap_outputport_pullbuf,
};
#endif

static ISF_PORT *isf_audcap_outputportlist[ISF_AUDCAP_OUT_NUM] = {0};
static ISF_PORT *isf_audcap_outputportlist_cfg[ISF_AUDCAP_OUT_NUM] = {0};
static ISF_PORT_CAPS *isf_audcap_outputportlist_caps[ISF_AUDCAP_OUT_NUM] = {
	&isf_audcap_outputport1_caps,
	&isf_audcap_outputport2_caps,
#if (ISF_AUDCAP_OUT_NUM == 8)
	&isf_audcap_outputport3_caps,
	&isf_audcap_outputport4_caps,
	&isf_audcap_outputport5_caps,
	&isf_audcap_outputport6_caps,
	&isf_audcap_outputport7_caps,
	&isf_audcap_outputport8_caps
#endif
};

static ISF_STATE isf_audcap_outputport_state[ISF_AUDCAP_OUT_NUM] = {0};
static ISF_STATE *isf_audcap_outputportlist_state[ISF_AUDCAP_OUT_NUM] = {
	&isf_audcap_outputport_state[0],
	&isf_audcap_outputport_state[1],
#if (ISF_AUDCAP_OUT_NUM == 8)
	&isf_audcap_outputport_state[2],
	&isf_audcap_outputport_state[3],
	&isf_audcap_outputport_state[4],
	&isf_audcap_outputport_state[5],
	&isf_audcap_outputport_state[6],
	&isf_audcap_outputport_state[7]
#endif
};

static ISF_INFO isf_audcap_outputinfo_in = {0};
static ISF_INFO *isf_audcap_outputinfolist_in[ISF_AUDCAP_IN_NUM] = {
	&isf_audcap_outputinfo_in,
};

static ISF_INFO isf_audcap_outputinfo_out[ISF_AUDCAP_OUT_NUM] = {0};
static ISF_INFO *isf_audcap_outputinfolist_out[ISF_AUDCAP_OUT_NUM] = {
	&isf_audcap_outputinfo_out[0],
	&isf_audcap_outputinfo_out[1],
#if (ISF_AUDCAP_OUT_NUM == 8)
	&isf_audcap_outputinfo_out[2],
	&isf_audcap_outputinfo_out[3],
	&isf_audcap_outputinfo_out[4],
	&isf_audcap_outputinfo_out[5],
	&isf_audcap_outputinfo_out[6],
	&isf_audcap_outputinfo_out[7],
#endif
};

void isf_audcap_install_id(void)
{
	UINT32 i = 0;
	AUDCAP_CONTEXT_PORT *p_ctx_p = 0;

    for (i = 0; i < PATH_MAX_COUNT; i++) {
		p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];

        OS_CONFIG_SEMPHORE(p_ctx_p->ISF_AUDCAP_SEM_ID, 0, 1, 1);
#if (SUPPORT_PULL_FUNCTION)
		OS_CONFIG_SEMPHORE(p_ctx_p->ISF_AUDCAP_PULLQ_SEM_ID, 0, 0, 0);
#endif
    }
	OS_CONFIG_SEMPHORE(ISF_AUDCAP_PROC_SEM_ID, 0, 1, 1);
	OS_CONFIG_SEMPHORE(ISF_AUDCAP_COMM_SEM_ID, 0, 1, 1);
}

void isf_audcap_uninstall_id(void)
{
	UINT32 i = 0;
	AUDCAP_CONTEXT_PORT *p_ctx_p = 0;

    for (i = 0; i < PATH_MAX_COUNT; i++) {
		p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];

        SEM_DESTROY(p_ctx_p->ISF_AUDCAP_SEM_ID);
#if (SUPPORT_PULL_FUNCTION)
		SEM_DESTROY(p_ctx_p->ISF_AUDCAP_PULLQ_SEM_ID);
#endif
    }
	SEM_DESTROY(ISF_AUDCAP_PROC_SEM_ID);
	SEM_DESTROY(ISF_AUDCAP_COMM_SEM_ID);
}

static UINT64 _isf_audcap_do_div(UINT64 dividend, UINT64 divisor)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	dividend = (dividend / divisor);
#else
	do_div(dividend, divisor);
#endif

	return dividend;
}

#if 0
static UINT64 _isf_audcap_add_timestamp(UINT64 timestamp, UINT64 time_diff)
{
	UINT32 ts_high = 0, ts_low = 0, diff_high = 0, diff_low = 0;

	ts_high   = (UINT32)(timestamp >> 32);
	ts_low    = (UINT32)(timestamp & 0xffffffff);

	if (time_diff > 1000000) {
		diff_high = (UINT32)_isf_audcap_do_div(time_diff, 1000000);
		diff_low  = (UINT32)(time_diff - (diff_high*1000000));
	} else {
		diff_high = 0;
		diff_low  = (UINT32)time_diff;
	}
	ts_high = ts_high + diff_high;
	ts_low  = ts_low + diff_low;

	if (ts_low > 1000000) {
		ts_high++;
		ts_low -= 1000000;
	}

	return (((UINT64)ts_high) << 32) | ((UINT64)ts_low);
}

static UINT64 _isf_audcap_minus_timestamp(UINT64 timestamp, UINT64 time_diff)
{
	UINT32 ts_high = 0, ts_low = 0, diff_high = 0, diff_low = 0;

	ts_high   = (UINT32)(timestamp >> 32);
	ts_low    = (UINT32)(timestamp & 0xffffffff);

	if (time_diff > 1000000) {
		diff_high = (UINT32)_isf_audcap_do_div(time_diff, 1000000);
		diff_low  = (UINT32)(time_diff - (diff_high*1000000));
	} else {
		diff_high = 0;
		diff_low  = (UINT32)time_diff;
	}

	if (ts_low < diff_low) {
		ts_high--;
		ts_low += 1000000;
	}

	return (((UINT64)(ts_high - diff_high)) << 32) | ((UINT64)(ts_low - diff_low));
}
#endif

static AUDCAP_BSDATA *_isf_audcap_get_bsdata(void)
{
	UINT32 i = 0;
	AUDCAP_BSDATA *pData = NULL;
	unsigned long flags;

	SEM_WAIT(ISF_AUDCAP_COMM_SEM_ID);
	while (g_p_audcap_bsdata_blk_maptlb[i]) {
		if (i > (PATH_MAX_COUNT * ISF_AUDCAP_BSDATA_BLK_MAX)) {
			//DBG_ERR("No free block!!!\r\n");
			SEM_SIGNAL(ISF_AUDCAP_COMM_SEM_ID);
			return NULL;
		}
		i++;
	}

	/*if (i > 15) {
		DBG_WRN("New block number is %d!!!\r\n", i);
	}*/

	pData = (AUDCAP_BSDATA *)(g_p_audcap_bsdata_blk_pool + i);

	if (pData) {
		pData->blk_id = i;
		pData->pnext_bsdata = 0;
		loc_cpu(flags);
		if (g_p_audcap_bsdata_blk_maptlb != NULL) {
			g_p_audcap_bsdata_blk_maptlb[i] = TRUE;
		}
		g_audcap_bsdata_num++;
		unl_cpu(flags);
	}
	SEM_SIGNAL(ISF_AUDCAP_COMM_SEM_ID);
	return pData;
}

static void _isf_audcap_free_bsdata(AUDCAP_BSDATA *pData)
{
	unsigned long flags;

	if (pData) {
		SEM_WAIT(ISF_AUDCAP_COMM_SEM_ID);
		pData->hData = 0;
		pData->pnext_bsdata = 0;
		pData->p_buf_info = 0;
		pData->p_remain_buf_info = 0;
		loc_cpu(flags);
		if (g_p_audcap_bsdata_blk_maptlb != NULL) {
			g_p_audcap_bsdata_blk_maptlb[pData->blk_id] = FALSE;
		}
		g_audcap_bsdata_num--;
		unl_cpu(flags);
		SEM_SIGNAL(ISF_AUDCAP_COMM_SEM_ID);
	}

}

static AUDCAP_BUF_INFO *_isf_audcap_get_buf_info(void)
{
	UINT32 i = 0;
	AUDCAP_BUF_INFO *pData = NULL;
	unsigned long flags;

	SEM_WAIT(ISF_AUDCAP_COMM_SEM_ID);
	while (g_p_audcap_bufinfo_blk_maptlb[i]) {
		if (i > (PATH_MAX_COUNT * ISF_AUDCAP_BSDATA_BLK_MAX)) {
			//DBG_ERR("No free block!!!\r\n");
			SEM_SIGNAL(ISF_AUDCAP_COMM_SEM_ID);
			return NULL;
		}
		i++;
	}

	/*if (i > 15) {
		DBG_WRN("New block number is %d!!!\r\n", i);
	}*/

	pData = (AUDCAP_BUF_INFO *)(g_p_audcap_bufinfo_blk_pool + i);

	if (pData) {
		pData->blk_id = i;
		loc_cpu(flags);
		if (g_p_audcap_bufinfo_blk_maptlb != NULL) {
			g_p_audcap_bufinfo_blk_maptlb[i] = TRUE;
		}
		unl_cpu(flags);
	}
	SEM_SIGNAL(ISF_AUDCAP_COMM_SEM_ID);
	return pData;
}

static void _isf_audcap_free_buf_info(AUDCAP_BUF_INFO *pData)
{
	unsigned long flags;

	if (pData) {
		SEM_WAIT(ISF_AUDCAP_COMM_SEM_ID);
		loc_cpu(flags);
		if (g_p_audcap_bufinfo_blk_maptlb != NULL) {
			g_p_audcap_bufinfo_blk_maptlb[pData->blk_id] = FALSE;
		}
		unl_cpu(flags);
		SEM_SIGNAL(ISF_AUDCAP_COMM_SEM_ID);
	}
}

static ISF_RV _isf_audcap_update_port_info(ISF_UNIT *p_thisunit, UINT32 oport)
{
	ISF_INFO *p_in_info = p_thisunit->port_ininfo[0];
	ISF_AUD_INFO *p_audinfo_in = (ISF_AUD_INFO *)(p_in_info);

	ISF_INFO *p_out_info = p_thisunit->port_outinfo[0];
	ISF_AUD_INFO *p_audinfo_out = (ISF_AUD_INFO *)(p_out_info);

	//same value for all path
	AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

	/*update input info*/
	if (!p_audinfo_in) {
		DBG_ERR("p_audinfo_in is NULL\r\n");
		return ISF_ERR_NULL_POINTER;
	}

	if (p_audinfo_in->dirty & ISF_INFO_DIRTY_AUDMAX) {
		//g_isf_audcap_codec = p_audinfo_in->audfmt;
		wsis_max.aud_info.bitpersample = p_audinfo_in->max_bitpersample;
		wsis_max.aud_info.ch_num = AUD_CHANNEL_COUNT(p_audinfo_in->max_channelcount);
		wsis_max.aud_info.aud_sr = p_audinfo_in->max_samplepersecond;
		wsis_max.aud_info.aud_ch = (wsis_max.aud_info.ch_num == 2)? AUDIO_CH_STEREO : AUDIO_CH_LEFT;
		p_audinfo_in->dirty &= ~ISF_INFO_DIRTY_AUDMAX;
		DBG_IND("update port max SR=%d,CH=%d,bps=%d\r\n", wsis_max.aud_info.aud_sr, wsis_max.aud_info.ch_num, wsis_max.aud_info.bitpersample);
		ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM_IMM, p_thisunit, ISF_IN(0), "set max sr(%d)\r\n", wsis_max.aud_info.aud_sr);
		ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM, p_thisunit, ISF_OUT(0), "set max channel(%d), ch_num(%d), bitpersample(%d)", wsis_max.aud_info.aud_ch, wsis_max.aud_info.ch_num, wsis_max.aud_info.bitpersample);
	}

	if (p_audinfo_in->dirty & ISF_INFO_DIRTY_AUDBITS) {
		wsis.aud_info.bitpersample = p_audinfo_in->bitpersample;
		p_audinfo_in->dirty &= ~ISF_INFO_DIRTY_AUDBITS;
		DBG_IND("update port bps=%d\r\n", wsis.aud_info.bitpersample);
		ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM, p_thisunit, ISF_OUT(0), "set bitpersample(%d)", wsis.aud_info.bitpersample);
	}
	if (p_audinfo_in->dirty & ISF_INFO_DIRTY_AUDCH) {
		wsis.aud_info.ch_num = p_audinfo_in->channelcount;
		p_audinfo_in->dirty &= ~ISF_INFO_DIRTY_AUDCH;
		DBG_IND("update port CH=%d\r\n", wsis.aud_info.ch_num);
		ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM, p_thisunit, ISF_OUT(0), "set ch_num(%d)", wsis.aud_info.ch_num);
	}
	if (p_audinfo_in->dirty & ISF_INFO_DIRTY_SAMPLERATE) {
		wsis.aud_info.aud_sr = p_audinfo_in->samplepersecond;
		p_audinfo_in->dirty &= ~ISF_INFO_DIRTY_SAMPLERATE;
		DBG_IND("update port SR=%d\r\n", wsis.aud_info.aud_sr);
		ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM_IMM, p_thisunit, ISF_OUT(0), "set sr(%d)", wsis.aud_info.aud_sr);
	}

	/*update output info*/
	if (!p_audinfo_out) {
		DBG_ERR("p_audinfo_in is NULL\r\n");
		return ISF_ERR_NULL_POINTER;
	}

	if (p_audinfo_out->dirty & ISF_INFO_DIRTY_AUDMAX) {
		p_ctx_p0->isf_audcap_resample_max = p_audinfo_out->max_samplepersecond;
		p_audinfo_out->dirty &= ~ISF_INFO_DIRTY_AUDMAX;
		if (p_ctx_p0->isf_audcap_resample_max != 0) {
			paudsrc_obj = kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID_SRC);
			if (paudsrc_obj == NULL) {
				DBG_ERR("src obj is NULL\r\n");
			}
		} else {
			paudsrc_obj = 0;
		}
		DBG_IND("output update port max SR=%d\r\n", p_ctx_p0->isf_audcap_resample_max);
		ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM_IMM, p_thisunit, ISF_OUT(oport), "set max resample sr(%d)", p_ctx_p0->isf_audcap_resample_max);
	}



	if (p_audinfo_out->dirty & ISF_INFO_DIRTY_SAMPLERATE) {
		p_ctx_p0->isf_audcap_resample = p_audinfo_out->samplepersecond;
		p_audinfo_out->dirty &= ~ISF_INFO_DIRTY_SAMPLERATE;
		DBG_IND("output update port SR=%d\r\n", p_ctx_p0->isf_audcap_resample);
		ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM_IMM, p_thisunit, ISF_OUT(oport), "set resample sr(%d)", p_ctx_p0->isf_audcap_resample);
	}

	return ISF_OK;
}


ISF_RV _isf_audcap_bindouput(struct _ISF_UNIT *p_thisunit, UINT32 oport, struct _ISF_UNIT *p_destunit, UINT32 iport)
{
	return ISF_OK;
}

static ISF_RV _isf_audcap_do_open(ISF_UNIT *p_thisunit, UINT32 oport)
{
	ISF_RV r = ISF_OK;
	unsigned long flags;
	UINT32 id = oport - ISF_OUT_BASE;
	AUDCAP_CONTEXT_PORT *p_ctx_p;
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[id];

	if (!isf_audcap_init) {
		nvtmpp_vb_add_module(p_thisunit->unit_module);
		_isf_audcap_bs_data_init();
		isf_audcap_init = TRUE;
	}

	loc_cpu(flags);
	isf_audcap_path_open_count++;
	unl_cpu(flags);

	if (isf_audcap_path_open_count == 1){
		#if defined __UITRON || defined __ECOS
		DX_HANDLE devAudObj = Dx_GetObject(DX_CLASS_AUDIO_EXT);
		#else
		UINT32 devAudObj = 0;
		#endif
		UINT32 addr = 0;
		UINT32 size = 0;
		ISF_RV r = ISF_OK;
		ISF_PORT* pSrcPort = isf_audcap.p_base->oport(p_thisunit, ISF_OUT(0));
		UINT32 mpp_size   = 0;
		UINT32 block_size = 0;
		UINT32 i;
		AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

		if (isf_audcap_tdm_ch) {
			wsis_max.aud_info.ch_num = isf_audcap_tdm_ch;
		}

		if (isf_audcap_dual_mono && (wsis_max.aud_info.ch_num > PATH_MAX_COUNT)) {
			DBG_ERR("Invalid channel number=%d, max=%d\r\n", wsis_max.aud_info.ch_num, PATH_MAX_COUNT);
			loc_cpu(flags);
			isf_audcap_path_open_count = 0;
			unl_cpu(flags);
			return ISF_ERR_BUFFER_CREATE;
		}

		{
			DBG_IND("SR=%d,CH=%d,ch_num=%d, bps=%d\r\n", wsis_max.aud_info.aud_sr, wsis_max.aud_info.aud_ch, wsis_max.aud_info.ch_num, wsis_max.aud_info.bitpersample);

			wsis_max.aud_info.buf_sample_cnt = g_isf_audcap_max_frame_sample;
			wsis_max.buf_count = g_isf_audcap_buf_cnt;

			if (p_aec_obj || p_ctx_p0->isf_audcap_lb_en) {
				wavstudio_set_config(WAVSTUD_CFG_AEC_EN, TRUE, 0);
			} else {
				wavstudio_set_config(WAVSTUD_CFG_AEC_EN, FALSE, 0);
			}

			block_size = wavstudio_get_config(WAVSTUD_CFG_MEM, (UINT32)&wsis_max, WAVSTUD_ACT_REC);

			if (isf_audcap_dual_mono) {
				//2 more block for 2 output channels
				size = block_size * (1 + wsis_max.aud_info.ch_num);
			} else {
				size = block_size;
			}

			DBG_IND("audcap needed mem = %x\r\n", size);

			/* create a buffer pool*/
			addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "work", size, &isf_audcap_memblk);

			if (addr == 0) {
				DBG_ERR("get work buf failed\r\n");
				loc_cpu(flags);
				isf_audcap_path_open_count = 0;
				unl_cpu(flags);
				return ISF_ERR_BUFFER_CREATE;
			}

			wso.mem.uiAddr = addr;
			wso.mem.uiSize = block_size;
			wso.wavstud_cb = _isf_audcap_evt_cb;

			if (isf_audcap_dual_mono) {
				UINT32 path_addr = wso.mem.uiAddr + block_size;
				for (i = 0; i < PATH_MAX_COUNT; i++) {
					AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];
					p_ctx_pi->g_isf_audcap_pathmem.addr = path_addr + block_size*i;
					p_ctx_pi->g_isf_audcap_pathmem.size = block_size;
				}
			}

			if (p_aec_obj && p_aec_obj->setbuf && p_aec_obj->getbuf) {
				UINT32 out_channel_num = 0;

				switch (aec_config_max.lb_channel) {
				case AUDIOCAP_LB_CH_LEFT:
					out_channel_num = 1;
					break;
				case AUDIOCAP_LB_CH_RIGHT:
					out_channel_num = 1;
					break;
				case AUDIOCAP_LB_CH_STEREO:
					out_channel_num = 2;
					break;
				default:
					out_channel_num = 2;
					break;
				}

				#if AEC_FUNC
				nvtaec_setconfig(NVTAEC_CONFIG_ID_LEAK_ESTIMATE_EN	,(INT32)aec_config_max.leak_estimate_enabled);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_LEAK_ESTIMATE_VAL	,(INT32)aec_config_max.leak_estimate_value);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_NOISE_CANCEL_LVL	,(INT32)aec_config_max.noise_cancel_level);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_ECHO_CANCEL_LVL	,(INT32)aec_config_max.echo_cancel_level);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_FILTER_LEN	,(INT32)aec_config_max.filter_length);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_FRAME_SIZE	,(INT32)aec_config_max.frame_size);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_NOTCH_RADIUS	,(INT32)aec_config_max.notch_radius);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_SPK_NUMBER, out_channel_num);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_RECORD_CH_NO, wsis_max.aud_info.ch_num);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_PLAYBACK_CH_NO, out_channel_num);
				#endif

				g_isf_audcap_aecmem.size = p_aec_obj->getbuf(AUDIO_SR_8000);
				g_isf_audcap_aecmem.addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "aec", g_isf_audcap_aecmem.size, &isf_audcap_aecmemblk);
				if (g_isf_audcap_aecmem.addr == 0) {
					DBG_ERR("get AEC buf failed\r\n");
					loc_cpu(flags);
					isf_audcap_path_open_count = 0;
					unl_cpu(flags);
					return ISF_ERR_BUFFER_CREATE;
				}
				p_aec_obj->setbuf(g_isf_audcap_aecmem.addr, g_isf_audcap_aecmem.size);

				{
					UINT32 unit_size = wsis_max.aud_info.buf_sample_cnt * wsis_max.aud_info.ch_num * (wsis_max.aud_info.bitpersample >> 3);

					g_isf_audcap_aec_pre_init_mem.size = unit_size*2;
					g_isf_audcap_aec_pre_init_mem.addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "aec_pre_init", g_isf_audcap_aec_pre_init_mem.size, &isf_audcap_aec_pre_init_bufmemblk);
					if (g_isf_audcap_aec_pre_init_mem.addr == 0) {
						DBG_ERR("get AEC pre init buf failed\r\n");
						loc_cpu(flags);
						isf_audcap_path_open_count = 0;
						unl_cpu(flags);
						return ISF_ERR_BUFFER_CREATE;
					}
				}
			}

			if (p_anr_obj && p_anr_obj->setbuf && p_anr_obj->getbuf) {
				g_isf_audcap_anrmem.size = p_anr_obj->getbuf(wsis_max.aud_info.aud_sr, wsis_max.aud_info.ch_num);
				g_isf_audcap_anrmem.addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "anr", g_isf_audcap_anrmem.size, &isf_audcap_anrmemblk);
				if (g_isf_audcap_anrmem.addr == 0) {
					DBG_ERR("get ANR buf failed\r\n");
					loc_cpu(flags);
					isf_audcap_path_open_count = 0;
					unl_cpu(flags);
					return ISF_ERR_BUFFER_CREATE;
				}
				p_anr_obj->setbuf(g_isf_audcap_anrmem.addr, g_isf_audcap_anrmem.size);
			}

			if (E_OK != wavstudio_open(WAVSTUD_ACT_REC, &wso, devAudObj, &wsis_max)) {
				DBG_ERR("audcap open failed\r\n");
				r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_memblk, ISF_OK);
				loc_cpu(flags);
				isf_audcap_path_open_count = 0;
				unl_cpu(flags);
				if (r != ISF_OK) {
					DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
					return ISF_ERR_BUFFER_RELEASE;
				}
				return ISF_ERR_PROCESS_FAIL;
			}

			if (paudsrc_obj) {
				if (paudsrc_obj->src.pre_init) {
					UINT32 input = wsis_max.aud_info.buf_sample_cnt;
					UINT32 output = p_ctx_p0->isf_audcap_resample_max * wsis_max.aud_info.buf_sample_cnt/wsis_max.aud_info.aud_sr;
					UINT32 need = paudsrc_obj->src.pre_init(wsis_max.aud_info.ch_num, input, output, FALSE);

					g_isf_audcap_srcmem.size = need;
					g_isf_audcap_srcmem.addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "src", g_isf_audcap_srcmem.size, &isf_audcap_srcmemblk);
					if (g_isf_audcap_srcmem.addr == 0) {
						DBG_ERR("get SRC buf failed\r\n");
						loc_cpu(flags);
						isf_audcap_path_open_count = 0;
						unl_cpu(flags);
						return ISF_ERR_BUFFER_CREATE;
					}

					g_isf_audcap_srcbufmem.size = wsis_max.aud_info.buf_sample_cnt * wsis_max.aud_info.ch_num * (wsis_max.aud_info.bitpersample >> 3);
					g_isf_audcap_srcbufmem.addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "srcbuf", g_isf_audcap_srcbufmem.size, &isf_audcap_srcbufmemblk);
					if (g_isf_audcap_srcbufmem.addr == 0) {
						DBG_ERR("get SRC work buf failed\r\n");
						loc_cpu(flags);
						isf_audcap_path_open_count = 0;
						unl_cpu(flags);
						return ISF_ERR_BUFFER_CREATE;
					}
				}
			}

			if (g_isf_audcap_aec_ref_bypass_func) {
				UINT32 need = wsis_max.aud_info.buf_sample_cnt * wsis_max.aud_info.ch_num * (wsis_max.aud_info.bitpersample >> 3);

				g_isf_audcap_aec_ref_bypass_mem.size = need;
				g_isf_audcap_aec_ref_bypass_mem.addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "aec_ref", g_isf_audcap_aec_ref_bypass_mem.size, &isf_audcap_aec_ref_bypass_bufmemblk);

				if (g_isf_audcap_aec_ref_bypass_mem.addr == 0) {
					DBG_ERR("get ref bypass work buf failed\r\n");
					loc_cpu(flags);
					isf_audcap_path_open_count = 0;
					unl_cpu(flags);
					return ISF_ERR_BUFFER_CREATE;
				}

				memset((void*)g_isf_audcap_aec_ref_bypass_mem.addr, 0, g_isf_audcap_aec_ref_bypass_mem.size);
			}

			g_isf_audcap_allocmem = TRUE;
		}

		#if (SUPPORT_PULL_FUNCTION)
			// set mmap info
			p_ctx_p0->isf_audcap_mmap_info.addr_virtual  = addr;
			p_ctx_p0->isf_audcap_mmap_info.addr_physical = nvtmpp_vb_blk2pa(isf_audcap_memblk.h_data);
			p_ctx_p0->isf_audcap_mmap_info.size          = size;
		#endif

		// Create pool for bs
		mpp_size = (sizeof(UINT32) + sizeof(AUDCAP_BSDATA)) * PATH_MAX_COUNT * ISF_AUDCAP_BSDATA_BLK_MAX;
		addr = p_thisunit->p_base->do_new_i(p_thisunit, pSrcPort, "bs", mpp_size, &isf_audcap_bsmemblk[0]);
		if (addr == 0) {
			r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_bsmemblk[0], ISF_OK);
			loc_cpu(flags);
			isf_audcap_path_open_count = 0;
			unl_cpu(flags);
			if (r != ISF_OK) {
				DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
				return ISF_ERR_BUFFER_RELEASE;
			}
			return ISF_ERR_BUFFER_CREATE;
		}

		g_p_audcap_bsdata_blk_maptlb = (UINT32 *) addr;
		memset(g_p_audcap_bsdata_blk_maptlb, 0, mpp_size);
		g_p_audcap_bsdata_blk_pool = (AUDCAP_BSDATA *)(g_p_audcap_bsdata_blk_maptlb + (PATH_MAX_COUNT * ISF_AUDCAP_BSDATA_BLK_MAX));


		// Create pool for buf info
		mpp_size = (sizeof(UINT32) + sizeof(AUDCAP_BUF_INFO)) * PATH_MAX_COUNT * ISF_AUDCAP_BSDATA_BLK_MAX;
		addr = p_thisunit->p_base->do_new_i(p_thisunit, pSrcPort, "buf_info", mpp_size, &isf_audcap_bufinfomemblk[0]);
		if (addr == 0) {
			r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_bufinfomemblk[0], ISF_OK);
			loc_cpu(flags);
			isf_audcap_path_open_count = 0;
			unl_cpu(flags);
			if (r != ISF_OK) {
				DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
				return ISF_ERR_BUFFER_RELEASE;
			}
			return ISF_ERR_BUFFER_CREATE;
		}

		g_p_audcap_bufinfo_blk_maptlb = (UINT32 *) addr;
		memset(g_p_audcap_bufinfo_blk_maptlb, 0, mpp_size);
		g_p_audcap_bufinfo_blk_pool = (AUDCAP_BUF_INFO *)(g_p_audcap_bufinfo_blk_maptlb + (PATH_MAX_COUNT * ISF_AUDCAP_BSDATA_BLK_MAX));

		//DBG_DUMP("[isf_audcap]Open\r\n");
	}

	p_ctx_p->isf_audcap_opened = TRUE;

	return r;
}

static ISF_RV _isf_audcap_do_close(ISF_UNIT *p_thisunit, UINT32 oport, BOOL b_do_release_i)
{
	ISF_RV r = ISF_OK;
	unsigned long flags;
	UINT32 id = oport - ISF_OUT_BASE;
	AUDCAP_CONTEXT_PORT *p_ctx_p;
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[id];

	loc_cpu(flags);
	isf_audcap_path_open_count--;
	unl_cpu(flags);

	if (isf_audcap_path_open_count == 0){
		if ((g_isf_audcap_maxmem.addr == 0 || g_isf_audcap_maxmem.size == 0) && g_isf_audcap_allocmem) {
			if (E_OK != wavstudio_close(WAVSTUD_ACT_REC)) {
				DBG_ERR("audcap already closed\r\n");
				if (b_do_release_i) {
					r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_memblk, ISF_OK);
					if (r != ISF_OK) {
						DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
						//return ISF_ERR_BUFFER_RELEASE;
					}
				}
			}

			if (b_do_release_i) {
				r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_memblk, ISF_OK);
				if (r != ISF_OK) {
					DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
					//return ISF_ERR_BUFFER_RELEASE;
				}
			}

			if (g_isf_audcap_aecmem.addr) {
				if (b_do_release_i) {
					r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_aecmemblk, ISF_OK);
					if (r != ISF_OK) {
						DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
						//return ISF_ERR_BUFFER_RELEASE;
					}
				}
				g_isf_audcap_aecmem.addr = 0;
				g_isf_audcap_aecmem.size = 0;
			}

			if (g_isf_audcap_anrmem.addr) {
				if (b_do_release_i) {
					r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_anrmemblk, ISF_OK);
					if (r != ISF_OK) {
						DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
						//return ISF_ERR_BUFFER_RELEASE;
					}
				}
				g_isf_audcap_anrmem.addr = 0;
				g_isf_audcap_anrmem.size = 0;
			}

			if (g_isf_audcap_srcmem.addr) {
				if (b_do_release_i) {
					r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_srcmemblk, ISF_OK);
					if (r != ISF_OK) {
						DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
						//return ISF_ERR_BUFFER_RELEASE;
					}
				}
				g_isf_audcap_srcmem.addr = 0;
				g_isf_audcap_srcmem.size = 0;
			}

			if (g_isf_audcap_srcbufmem.addr) {
				if (b_do_release_i) {
					r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_srcbufmemblk, ISF_OK);
					if (r != ISF_OK) {
						DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
						//return ISF_ERR_BUFFER_RELEASE;
					}
				}
				g_isf_audcap_srcbufmem.addr = 0;
				g_isf_audcap_srcbufmem.size = 0;
			}

			if (g_isf_audcap_aec_ref_bypass_mem.addr) {
				if (b_do_release_i) {
					r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_aec_ref_bypass_bufmemblk, ISF_OK);
					if (r != ISF_OK) {
						DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
						//return ISF_ERR_BUFFER_RELEASE;
					}
				}
				g_isf_audcap_aec_ref_bypass_mem.addr = 0;
				g_isf_audcap_aec_ref_bypass_mem.size = 0;
			}

			if (g_isf_audcap_aec_pre_init_mem.addr) {
				if (b_do_release_i) {
					r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_aec_pre_init_bufmemblk, ISF_OK);
					if (r != ISF_OK) {
						DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
						//return ISF_ERR_BUFFER_RELEASE;
					}
				}
				g_isf_audcap_aec_pre_init_mem.addr = 0;
				g_isf_audcap_aec_pre_init_mem.size = 0;
			}

			g_isf_audcap_allocmem = FALSE;
		} else {
			DBG_IND("Do not release pool\r\n");
			if (E_OK != wavstudio_close(WAVSTUD_ACT_REC)) {
				DBG_ERR("audcap already closed\r\n");
				return ISF_ERR_PROCESS_FAIL;
			}
		}

		if (b_do_release_i) {
			r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_bsmemblk[0], ISF_OK);
			if (r != ISF_OK) {
				DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
				//return ISF_ERR_BUFFER_RELEASE;
			}
		}
		g_p_audcap_bsdata_blk_pool   = NULL;
		g_p_audcap_bsdata_blk_maptlb = NULL;

		//memset(p_ctx_p->g_p_audcap_bsdata_link_head, 0, sizeof(AUDCAP_BSDATA *) * ISF_AUDCAP_OUT_NUM);
		//memset(p_ctx_p->g_p_audcap_bsdata_link_tail, 0, sizeof(AUDCAP_BSDATA *) * ISF_AUDCAP_OUT_NUM);

		p_ctx_p->g_p_audcap_bsdata_link_head = NULL;
		p_ctx_p->g_p_audcap_bsdata_link_tail = NULL;

		if (b_do_release_i) {
			r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_bufinfomemblk[0], ISF_OK);
			if (r != ISF_OK) {
				DBG_ERR("%s release blk failed! (%d)\r\n", p_thisunit->unit_name, r);
				//return ISF_ERR_BUFFER_RELEASE;
			}
		}
		g_p_audcap_bufinfo_blk_pool   = NULL;
		g_p_audcap_bufinfo_blk_maptlb = NULL;

		//DBG_DUMP("[isf_audcap]Close\r\n");
	}

	_isf_audcap_unlock_pullqueue(id);
	_isf_audcap_release_all_pullque(id);

	p_ctx_p->isf_audcap_opened = FALSE;
	p_ctx_p->aec_en = FALSE;
	p_ctx_p->anr_en = FALSE;

	return ISF_OK;
}

static ISF_RV _isf_audcap_do_start(ISF_UNIT *p_thisunit, UINT32 path)
{
	ER ret = E_OK;
	AUDCAP_CONTEXT_PORT *p_ctx_p;
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[path];

	if (p_ctx_p->volume & WAVSTUD_PORT_VOLUME) {
		wavstudio_set_config(WAVSTUD_CFG_VOL_P0+path, WAVSTUD_ACT_REC, p_ctx_p->volume);
	} else {
		wavstudio_set_config(WAVSTUD_CFG_VOL, WAVSTUD_ACT_REC, p_ctx_p->volume);
	}

	g_isf_audcap_aec_ref_bypass = g_isf_audcap_aec_ref_bypass_enable;

	if (p_ctx_p->isf_audcap_started == FALSE) {
		AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];
		AUDCAP_CONTEXT_PORT *p_ctx_p1 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[1];

		if (isf_audcap_tdm_ch) {
			wsis.aud_info.ch_num = isf_audcap_tdm_ch;
		}

		if (wsis.aud_info.aud_sr > wsis_max.aud_info.aud_sr) {
			return ISF_ERR_INVALID_VALUE;
		}

		if (wsis.aud_info.ch_num > wsis_max.aud_info.ch_num) {
			return ISF_ERR_INVALID_VALUE;
		}

		if (wsis.aud_info.bitpersample > wsis_max.aud_info.bitpersample) {
			return ISF_ERR_INVALID_VALUE;
		}

		if (g_isf_audcap_frame_sample > g_isf_audcap_max_frame_sample) {
			return ISF_ERR_INVALID_VALUE;
		}

		if (FALSE == _isf_audcap_is_pullque_all_released(path)) {
			DBG_ERR("Start failed !! Not all pulled bitstream are released\r\n");
		}

		isf_audcap_path_start_count++;

		if (isf_audcap_path_start_count == 1) {
			UINT32 needed_buf = 0;
			BOOL aec_enable = FALSE;
			UINT32 i;

			wsis.aud_info.buf_sample_cnt = g_isf_audcap_frame_sample;

			if (mono_expand && wsis.aud_info.ch_num == 2) {
				wsis.aud_info.aud_ch = isf_audcap_mono_ch;
			} else {
				wsis.aud_info.aud_ch = (wsis.aud_info.ch_num == 1)? isf_audcap_mono_ch : AUDIO_CH_STEREO;
			}
			wsis.buf_count = g_isf_audcap_buf_cnt;

			needed_buf = wavstudio_get_config(WAVSTUD_CFG_MEM, (UINT32)&wsis, WAVSTUD_ACT_REC);

			if (wso.mem.uiSize < needed_buf) {
				DBG_ERR("Need mem size (%d) > max public size (%d)\r\n", needed_buf, wso.mem.uiSize);
				return ISF_ERR_NEW_FAIL;
			}

			//_isf_audcap_release_all_pullque(path);
			_isf_audcap_bs_queue_init();

			for (i = 0; i < PATH_MAX_COUNT; i++) {
				AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];

				if (p_ctx_pi->aec_en) {
					aec_enable = TRUE;
					break;
				}
			}

			if (aec_enable && (wsis.aud_info.aud_sr == AUDIO_SR_44100)) {
				aec_enable = FALSE;
				DBG_ERR("AEC not support 44.1K sample rate\r\n");
			}

			if (p_aec_obj && p_aec_obj->open && aec_enable && !g_isf_audcap_aec_pre_done) {
				UINT32 aout_channel;
				UINT32 aout_channel_num;
				INT32 aout_speaker_num;
				#if AEC_FUNC
				ret = p_aec_obj->open();
				if (ret != E_OK) {
					DBG_ERR("Open AEC failed. Disable AEC\r\n");
					for (i = 0; i < PATH_MAX_COUNT; i++) {
						AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];

						p_ctx_pi->aec_en = FALSE;
					}
				}

				isf_audcap_aec_lb_swap = FALSE;

				//set aout channel
				if (isf_audcap_dual_mono) {
					if (p_ctx_p0->isf_audout_ch == p_ctx_p1->isf_audout_ch && p_ctx_p0->isf_audout_ch_num == 1) {
						aout_channel = p_ctx_p0->isf_audout_ch;
						aout_channel_num = p_ctx_p0->isf_audout_ch_num;
						aout_speaker_num = 1;
					} else if (p_ctx_p0->isf_audout_ch != p_ctx_p1->isf_audout_ch) {
						if (p_ctx_p0->isf_audout_ch_num != 1 || p_ctx_p1->isf_audout_ch_num != 1) {
							DBG_ERR("Invalid AEC channel setting\r\n");
							aout_channel = p_ctx_p0->isf_audout_ch;
							aout_channel_num = p_ctx_p0->isf_audout_ch_num;
							aout_speaker_num = 1;
						} else {
							aout_channel = AUDIO_CH_STEREO;
							aout_channel_num = 2;
							aout_speaker_num = 2;

							if (p_ctx_p0->isf_audout_ch != AUDIO_CH_LEFT && p_ctx_p1->isf_audout_ch != AUDIO_CH_RIGHT) {
								isf_audcap_aec_lb_swap = TRUE;
							}
						}
					} else {
						DBG_ERR("Invalid AEC channel setting\r\n");
						aout_channel = p_ctx_p0->isf_audout_ch;
						aout_channel_num = p_ctx_p0->isf_audout_ch_num;
						aout_speaker_num = 1;
					}
				} else {
					aout_channel = p_ctx_p0->isf_audout_ch;
					aout_channel_num = p_ctx_p0->isf_audout_ch_num;
					aout_speaker_num = 1;
				}

				wavstudio_set_config(WAVSTUD_CFG_AEC_EN, TRUE, 0);
				wavstudio_set_config(WAVSTUD_CFG_PLAY_LB_CHANNEL, aout_channel, 0);

				nvtaec_setconfig(NVTAEC_CONFIG_ID_LEAK_ESTIMATE_EN	,(INT32)p_ctx_p->aec_config.leak_estimate_enabled);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_LEAK_ESTIMATE_VAL	,(INT32)p_ctx_p->aec_config.leak_estimate_value);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_NOISE_CANCEL_LVL	,(INT32)p_ctx_p->aec_config.noise_cancel_level);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_ECHO_CANCEL_LVL	,(INT32)p_ctx_p->aec_config.echo_cancel_level);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_FILTER_LEN	,(INT32)p_ctx_p->aec_config.filter_length);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_FRAME_SIZE	,(INT32)p_ctx_p->aec_config.frame_size);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_NOTCH_RADIUS	,(INT32)p_ctx_p->aec_config.notch_radius);

				nvtaec_setconfig(NVTAEC_CONFIG_ID_RECORD_CH_NO, wsis.aud_info.ch_num);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_PLAYBACK_CH_NO, aout_channel_num);
				nvtaec_setconfig(NVTAEC_CONFIG_ID_SPK_NUMBER, aout_speaker_num);

				if (p_aec_obj->getbuf) {
					needed_buf = p_aec_obj->getbuf(wsis.aud_info.aud_sr);
					if (needed_buf > g_isf_audcap_aecmem.size) {
						DBG_ERR("Need AEC mem size (%d) > max public size (%d)\r\n", needed_buf, g_isf_audcap_aecmem.size);
						return ISF_ERR_NEW_FAIL;
					}
				}

				if (p_aec_obj->start) {
					p_aec_obj->start((INT32)wsis.aud_info.aud_sr, wsis.aud_info.ch_num, aout_channel_num);
				}

				if (g_isf_audcap_aec_pre_init && (g_isf_audcap_aec_pre_init_mem.addr != 0)) {
					INT16 *temp_addr;
					INT16 *temp_addr_cap;
					UINT32 k, l;

					UINT32 size = g_isf_audcap_aec_pre_init_mem.size/2;
					UINT32 addr_cap = g_isf_audcap_aec_pre_init_mem.addr;
					UINT32 addr_aec = g_isf_audcap_aec_pre_init_mem.addr + size;

					UINT32 count =  5000 /(((size / (wsis.aud_info.bitpersample >> 3)) / wsis.aud_info.ch_num)*1000 / wsis.aud_info.aud_sr);

					UINT32 size_aec;
					UINT32 table_offset = 48000 / wsis.aud_info.aud_sr;

					/*temp_addr = (INT16 *)addr;

					for(k = 0; k < ((size / (wsis.aud_info.bitpersample >> 3)) / wsis.aud_info.ch_num);k++){
					    *(temp_addr+k*2) = sin_table[sin_count];
						*(temp_addr+k*2+1) = sin_table[sin_count];
						sin_count = (sin_count + 1)%48;
					}*/
					memset((void*)addr_cap, 0, size);
					memset((void*)addr_aec, 0, size);

					if (wsis.aud_info.ch_num > p_ctx_p0->isf_audout_ch_num) {
						size_aec = size / (wsis.aud_info.ch_num/p_ctx_p0->isf_audout_ch_num);
					} else {
						size_aec = size;
					}

					temp_addr = (INT16 *)addr_aec;
					temp_addr_cap = (INT16 *)addr_cap;

					for(k = 0; k < count; k++) {
						for(l = 0; l < ((size_aec / (wsis.aud_info.bitpersample >> 3)) / p_ctx_p0->isf_audout_ch_num);l++){
							*(temp_addr+l*p_ctx_p0->isf_audout_ch_num) = sin_table[sin_count];
							*(temp_addr_cap+l*wsis.aud_info.ch_num) = sin_table[sin_count];

							if (p_ctx_p0->isf_audout_ch_num > 1) {
								*(temp_addr+l*p_ctx_p0->isf_audout_ch_num+1) = sin_table[sin_count];
							}

							if (wsis.aud_info.ch_num > 1) {
								*(temp_addr_cap+l*wsis.aud_info.ch_num+1) = sin_table[sin_count];
							}

							sin_count = (sin_count + table_offset)%48;
						}

						p_aec_obj->apply(addr_cap, addr_aec, addr_cap, size);
					}

					g_isf_audcap_aec_pre_done = TRUE;
				}

				#endif
			} else {
				wavstudio_set_config(WAVSTUD_CFG_AEC_EN, FALSE, 0);
			}

			if (p_anr_obj && p_anr_obj->open && p_ctx_p0->anr_en) {
				ret = p_anr_obj->open(wsis.aud_info.aud_sr, wsis.aud_info.ch_num);
				if (ret != E_OK) {
					DBG_ERR("Open ANR failed. Disable ANR\r\n");
					p_ctx_p0->anr_en = FALSE;
				}
			}

			if (p_audfilt_obj && audfilt_en) {
				if (p_audfilt_obj->open) {
					p_audfilt_obj->open(wsis.aud_info.ch_num, ENABLE);
				}

				if (p_audfilt_obj->design) {
					p_audfilt_obj->design();
				}
			}

			#if AGC_FUNC == ENABLE
			if (p_ctx_p->isf_audcap_agc_cfg.enable) {
				paudagc_obj = kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID_AGC);

				if (paudagc_obj) {
					if (paudagc_obj->agc.open && paudagc_obj->agc.set_config && paudagc_obj->agc.init) {
						paudagc_obj->agc.open();
						paudagc_obj->agc.set_config(AGC_CONFIG_ID_SAMPLERATE, wsis.aud_info.aud_sr);
						paudagc_obj->agc.set_config(AGC_CONFIG_ID_CHANNEL_NO, wsis.aud_info.ch_num);

						paudagc_obj->agc.set_config(AGC_CONFIG_ID_TARGET_LVL, AGC_DB(p_ctx_p->isf_audcap_agc_cfg.target_lvl));
						paudagc_obj->agc.set_config(AGC_CONFIG_ID_NG_THD,     AGC_DB(p_ctx_p->isf_audcap_agc_cfg.ng_threshold));

						//audlib_agc_set_config(AGC_CONFIG_ID_DECAY_TIME, isf_audcap_agc_cfg[path].decay_time);
						//audlib_agc_set_config(AGC_CONFIG_ID_ATTACK_TIME, isf_audcap_agc_cfg[path].attack_time);
						//audlib_agc_set_config(AGC_CONFIG_ID_MAXGAIN, 	AGC_DB(isf_audcap_agc_cfg[path].max_gain));
						//audlib_agc_set_config(AGC_CONFIG_ID_MINGAIN,    AGC_DB(isf_audcap_agc_cfg[path].min_gain));

						paudagc_obj->agc.init();
					}
				} else {
					DBG_ERR("agc obj is NULL\r\n");
				}
			}
			#endif

			#if 0
			if (aec_en && wsis.aud_info.aud_sr*wsis.aud_info.ch_num > 48000) {
				aec_en = FALSE;
				if (p_aec_obj && p_aec_obj->enable) {
					p_aec_obj->enable(aec_en);
				}
				DBG_ERR("AEC cannot handle sample rate=%d, channel num=%d. Disable AEC\r\n", wsis.aud_info.aud_sr, wsis.aud_info.ch_num);
			}
			#endif

			if (p_ctx_p->isf_audcap_lb_en) {
				wavstudio_set_config(WAVSTUD_CFG_AEC_EN, TRUE, 0);
				wavstudio_set_config(WAVSTUD_CFG_PLAY_LB_CHANNEL, p_ctx_p0->isf_audout_ch, 0);
			} else if (aec_enable == FALSE) {
				wavstudio_set_config(WAVSTUD_CFG_AEC_EN, FALSE, 0);
			}

			if (g_isf_audcap_frame_block_sample != 0) {
				isf_audcap_timestamp.frame_time = _isf_audcap_do_div(((UINT64)g_isf_audcap_frame_sample)*1000000, (UINT64)wsis.aud_info.aud_sr);
				isf_audcap_timestamp.block_time = _isf_audcap_do_div(((UINT64)g_isf_audcap_frame_block_sample)*1000000, (UINT64)wsis.aud_info.aud_sr);

				//DBG_DUMP("ft=%d, bt=%d\r\n", (UINT32)(isf_audcap_timestamp.frame_time & 0xffffffff), (UINT32)(isf_audcap_timestamp.block_time & 0xffffffff));
			} else {
				isf_audcap_timestamp.frame_time = 0;
				isf_audcap_timestamp.block_time = 0;
			}

			if (p_ctx_p0->isf_audcap_resample != 0 &&  p_ctx_p0->isf_audcap_resample != wsis.aud_info.aud_sr && paudsrc_obj) {
				_isf_audcap_src_check(p_ctx_p0->isf_audcap_resample);
			} else {
				p_ctx_p0->isf_audcap_resample_en = FALSE;
			}

			p_ctx_p0->isf_audcap_resample_update = 0;
			p_ctx_p0->isf_audcap_resample_dirty = 0;

			g_audcap_pull_failed_count = 0;

			wavstudio_set_config(WAVSTUD_CFG_DEFAULT_SETTING, p_ctx_p->isf_audcap_defset, 0);
			wavstudio_set_config(WAVSTUD_CFG_NG_THRESHOLD, (UINT32)&(p_ctx_p->isf_audcap_ng_thd), 0);
			wavstudio_set_config(WAVSTUD_CFG_ALC_EN, (UINT32)&isf_audcap_alc_en, 0);
			wavstudio_set_config(WAVSTUD_CFG_REC_SRC, (UINT32)&isf_audcap_rec_src, 0);
			wavstudio_set_config(WAVSTUD_CFG_REC_GAIN_LEVEL, (UINT32)&isf_audcap_gain_level, 0);
			if ((isf_audcap_alc_en == 1) && (isf_audcap_alc_cfg.enable == 1)) {
				wavstudio_set_config(WAVSTUD_CFG_ALC_DECAY_TIME, (UINT32)&isf_audcap_alc_cfg.decay_time, 0);
				wavstudio_set_config(WAVSTUD_CFG_ALC_ATTACK_TIME, (UINT32)&isf_audcap_alc_cfg.attack_time, 0);
				wavstudio_set_config(WAVSTUD_CFG_ALC_MAXGAIN, (UINT32)&isf_audcap_alc_cfg.max_gain, 0);
				wavstudio_set_config(WAVSTUD_CFG_ALC_MINGAIN, (UINT32)&isf_audcap_alc_cfg.min_gain, 0);
				wavstudio_set_config(WAVSTUD_CFG_ALC_CFG_EN, (UINT32)&isf_audcap_alc_cfg.enable, 0);
			}

			#if PERF_CHK == ENABLE
			aec_time = 0;
			aec_count = 0;
			anr_time = 0;
			anr_count = 0;
			#endif

			//set volume for all port
			if (p_ctx_p->volume & WAVSTUD_PORT_VOLUME) {
				for (i = 0; i < PATH_MAX_COUNT; i++) {
					AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];

					wavstudio_set_config(WAVSTUD_CFG_VOL_P0+i, WAVSTUD_ACT_REC, p_ctx_pi->volume);
				}
			}

			DBG_IND("SR=%d,CH=%d,ch_num=%d, bps=%d\r\n", wsis.aud_info.aud_sr, wsis.aud_info.aud_ch, wsis.aud_info.ch_num, wsis.aud_info.bitpersample);

			if (FALSE == wavstudio_start(WAVSTUD_ACT_REC, &wsis.aud_info, WAVSTUD_NON_STOP_COUNT, _isf_audcap_cb)) {
				DBG_ERR("audcap rec failed\r\n");
				isf_audcap_path_start_count--;
				return ISF_ERR_PROCESS_FAIL;
			}
			wavstudio_wait_start(WAVSTUD_ACT_REC);

			//DBG_DUMP("[isf_audcap]Start\r\n");
		}

		p_ctx_p->isf_audcap_output_mem.addr = p_ctx_p->g_isf_audcap_pathmem.addr;
		p_ctx_p->isf_audcap_output_mem.size = p_ctx_p->g_isf_audcap_pathmem.size;
		p_ctx_p->isf_audcap_started = TRUE;
	}
	return ISF_OK;
}

static ISF_RV _isf_audcap_do_stop(ISF_UNIT *p_thisunit, UINT32 path)
{
	UINT32 i;
	BOOL aec_enable = FALSE;
	AUDCAP_CONTEXT_PORT *p_ctx_p;
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[path];

	if (p_ctx_p->isf_audcap_started == TRUE){

		isf_audcap_path_start_count--;

		if (isf_audcap_path_start_count == 0) {
			AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

			if (FALSE == wavstudio_stop(WAVSTUD_ACT_REC)) {
				DBG_ERR("audcap stop rec failed\r\n");
				return ISF_ERR_PROCESS_FAIL;
			}
			wavstudio_wait_stop(WAVSTUD_ACT_REC);

			for (i = 0; i < PATH_MAX_COUNT; i++) {
				AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];

				if (p_ctx_pi->aec_en) {
					aec_enable = TRUE;
					break;
				}
			}

			if (wsis.aud_info.aud_sr == AUDIO_SR_44100) {
				aec_enable = FALSE;
			}

			if (aec_enable && p_aec_obj && p_aec_obj->stop && p_aec_obj->close && !g_isf_audcap_aec_pre_done) {
				p_aec_obj->stop();
				p_aec_obj->close();
			}
			if (p_anr_obj && p_anr_obj->close) {
				p_anr_obj->close();
			}
			if (p_audfilt_obj && audfilt_en) {
				if (p_audfilt_obj->close) {
					p_audfilt_obj->close();
				}
			}

			if (paudsrc_obj && p_ctx_p0->isf_audcap_resample_en) {
				if (paudsrc_obj->src.destroy) {
					paudsrc_obj->src.destroy(resample_handle_rec);
				}
			}

			#if AGC_FUNC == ENABLE
			if (p_ctx_p->isf_audcap_agc_cfg.enable) {
				if (paudagc_obj) {
					if (paudagc_obj->agc.close) {
						paudagc_obj->agc.close();
					}
				}
			}
			#endif

			#if (DUMP_TO_FILE_TEST_KERNEL_VER == ENABLE)
			{
				CHAR filename[64] = {0};

				sprintf(filename, "/mnt/sd/aec_rec%d.pcm", file_cnt);

				g_wo_fd = vos_file_open(filename, O_CREAT|O_WRONLY|O_SYNC, 0);

				if (!((VOS_FILE)(-1) == g_wo_fd)) {
					int len = 0;
					len = vos_file_write(g_wo_fd, (void *)temp_buf_r, temp_cnt_r);
				}

				if (!((VOS_FILE)(-1) == g_wo_fd)) {
					vos_file_close(g_wo_fd);
				}

				sprintf(filename, "/mnt/sd/aec_lb%d.pcm", file_cnt);

				g_wo_fd = vos_file_open(filename, O_CREAT|O_WRONLY|O_SYNC, 0);

				if (!((VOS_FILE)(-1) == g_wo_fd)) {
					int len = 0;
					len = vos_file_write(g_wo_fd, (void *)temp_buf_l, temp_cnt_l);
				}

				if (!((VOS_FILE)(-1) == g_wo_fd)) {
					vos_file_close(g_wo_fd);
				}

				temp_cnt_r = 0;
				temp_cnt_l = 0;
				file_cnt++;
			}
			#endif
			//DBG_DUMP("[isf_audcap]Stop\r\n");
		}
		p_ctx_p->isf_audcap_started = FALSE;
		#if 0
		_isf_audcap_release_all_pullque(path);
		#endif
	}

	return ISF_OK;
}

static ISF_RV _isf_audcap_do_pause(ISF_UNIT *p_thisunit, UINT32 oport)
{
	if (FALSE == wavstudio_pause(WAVSTUD_ACT_REC)) {
		DBG_ERR("audcap pause rec failed\r\n");
		return ISF_ERR_PROCESS_FAIL;
	}
	return ISF_OK;
}

static ISF_RV _isf_audcap_do_resume(ISF_UNIT *p_thisunit, UINT32 oport)
{
	if (FALSE == wavstudio_resume(WAVSTUD_ACT_REC)) {
		DBG_ERR("audcap resume rec failed\r\n");
		return ISF_ERR_PROCESS_FAIL;
	}
	return ISF_OK;
}

static ISF_RV _isf_audcap_do_reset(ISF_UNIT *p_thisunit, UINT32 oport)
{
	AUDCAP_CONTEXT_PORT *p_ctx_p = NULL;
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

	//DBG_UNIT("\r\n");

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		return ISF_ERR_UNINIT; //sitll not init
	}

	if (oport >= PATH_MAX_COUNT) {
		return ISF_ERR_INVALID_PORT_ID;
	}

	p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[oport];

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

	if (p_ctx_p->isf_audcap_opened) {
		DBG_IND("reset - close path %d\r\n", oport);
		_isf_audcap_do_close(p_thisunit, oport, FALSE);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(reset - close)");
	}

	//g_volume = 100;
	isf_audcap_alc_en = -1;
	isf_audcap_rec_src = 0;
	isf_audcap_alc_cfg.enable = -1;

	p_aec_obj   = NULL;
	p_anr_obj   = NULL;
	p_audfilt_obj   = NULL;
	paudsrc_obj   = NULL;
	isf_audcap_tdm_ch = 0;
	isf_audcap_gain_level = KDRV_AUDIO_CAP_GAIN_LEVEL8;
	mono_expand = FALSE;

	g_isf_audcap_aec_ref_bypass_func = FALSE;
	g_isf_audcap_aec_ref_bypass_enable = FALSE;
	g_isf_audcap_aec_ref_bypass = FALSE;

	return ISF_OK;
}

static void _isf_audcap_do_runtime(ISF_UNIT *p_thisunit, UINT32 cmd, UINT32 oport)
{
	ISF_INFO *p_out_info = p_thisunit->port_outinfo[0];
	ISF_AUD_INFO *p_audinfo_out = (ISF_AUD_INFO *)(p_out_info);
	AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];
	AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[oport];

	if (p_ctx_p->volume & WAVSTUD_PORT_VOLUME) {
		wavstudio_set_config(WAVSTUD_CFG_VOL_P0+oport, WAVSTUD_ACT_REC, p_ctx_p->volume);
	} else {
		wavstudio_set_config(WAVSTUD_CFG_VOL, WAVSTUD_ACT_REC, p_ctx_p->volume);
	}

	g_isf_audcap_aec_ref_bypass = g_isf_audcap_aec_ref_bypass_enable;

	/*update output info*/
	if (p_audinfo_out) {
		if (p_audinfo_out->dirty & ISF_INFO_DIRTY_SAMPLERATE) {
			p_ctx_p0->isf_audcap_resample_update = p_audinfo_out->samplepersecond;
			p_ctx_p0->isf_audcap_resample_dirty = 1;
			p_audinfo_out->dirty &= ~ISF_INFO_DIRTY_SAMPLERATE;
			DBG_IND("output update port SR=%d\r\n", p_ctx_p0->isf_audcap_resample_update);
			ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM_IMM, p_thisunit, ISF_OUT(oport), "set resample sr(%d)", p_ctx_p0->isf_audcap_resample_update);
		}
	}
}


static ISF_RV _isf_audcap_setparam(ISF_UNIT *p_thisunit, UINT32 param, UINT32 value)
{
	ISF_RV ret = ISF_OK;

	if (param < AUDCAP_PARAM_START) {
		return ISF_ERR_NOT_SUPPORT;
	}

	switch (param) {
	case AUDCAP_PARAM_VOL_IMM: {
			UINT32 i;

			if (value > 160) {
				DBG_WRN("Invalid volume=%d, set to 100\r\n", value);
				value = 100;
			}
			for (i = 0; i < PATH_MAX_COUNT; i++) {
				AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];
				p_ctx_pi->volume = value;
			}

			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set volume(%d)", g_volume);
			break;
		}
	case AUDCAP_PARAM_AEC_OBJ: {
			//p_aec_obj = (PWAVSTUD_AUD_AEC_OBJ)value;
			//ret = ISF_APPLY_DOWNSTREAM_OFFTIME;
			break;
		}
	case AUDCAP_PARAM_AEC_EN: {
			AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

			p_ctx_p0->aec_en = value;
			if (p_aec_obj && p_aec_obj->enable) {
				p_aec_obj->enable(value);
			}
			//ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set AEC(%s)\r\n", aec_en ? "Enable" : "Disable");
			//ret = ISF_APPLY_DOWNSTREAM_OFFTIME;
			break;
		}
	case AUDCAP_PARAM_AEC_PLAY_CHANNEL: {
			/*AUDIO_CH channel = (value == 2)? AUDIO_CH_STEREO : isf_audout_mono_ch;

			wavstudio_set_config(WAVSTUD_CFG_PLAY_LB_CHANNEL, channel, 0);

			isf_audout_ch_num = value;

			#if AEC_FUNC
			//nvtaec_setconfig(NVTAEC_CONFIG_ID_PLAYBACK_CH_NO, value);
			#endif

			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set AEC playback channel(%d)", value);
			*/
			break;
		}
	case AUDCAP_PARAM_ANR_OBJ: {
			p_anr_obj = (PAUDCAP_ANR_OBJ)value;
			//ret = ISF_APPLY_DOWNSTREAM_OFFTIME;
			break;
		}
	case AUDCAP_PARAM_ANR_EN: {
			AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

			p_ctx_p0->anr_en = value;
			if (p_anr_obj && p_anr_obj->enable) {
				p_anr_obj->enable(value);
			}
			//ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set ANR(%s)\r\n", aec_en ? "Enable" : "Disable");
			//ret = ISF_APPLY_DOWNSTREAM_OFFTIME;
			break;
		}
	case AUDCAP_PARAM_AEC_BUFSIZE: {
			g_isf_audcap_aecmem.size = value;
			//ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set AEC buf size(%08x)\r\n", g_isf_audcap_aecmem.size);
			break;
		}
	case AUDCAP_PARAM_ANR_BUFSIZE: {
			g_isf_audcap_anrmem.size = value;
			//ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set ANR buf size(%08x)\r\n", g_isf_audcap_anrmem.size);
			break;
		}
	case AUDCAP_PARAM_AUDFILT_OBJ: {
		p_audfilt_obj = (PAUDCAP_AUDFILT_OBJ)value;
		//ret = ISF_APPLY_DOWNSTREAM_OFFTIME;
		break;
		}
	case AUDCAP_PARAM_AUDFILT_EN: {
		audfilt_en = value;
		break;
		}
	case AUDCAP_PARAM_FRAME_SAMPLE: {
			#if 0
			if ((value >= 1024) && (value%1024 == 0)) {
				g_isf_audcap_frame_sample = value;
				g_isf_audcap_frame_block_sample = 0;
			} else {
				g_isf_audcap_frame_sample = ((value/1024)+1)*1024;
				g_isf_audcap_frame_block_sample = value;
			}
			#else
			if (value == g_isf_audcap_max_frame_sample) {
				g_isf_audcap_frame_sample = value;
				g_isf_audcap_frame_block_sample = 0;
			} else {
				g_isf_audcap_frame_sample = ((value/1024)+1)*1024;
				g_isf_audcap_frame_block_sample = value;
			}
			#endif
			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set frame sample(%d)\r\n", g_isf_audcap_frame_sample);
			break;
		}
	case AUDCAP_PARAM_MAX_FRAME_SAMPLE: {
			g_isf_audcap_max_frame_sample = value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set max frame sample(%d)\r\n", g_isf_audcap_max_frame_sample);
			break;
		}
	case AUDCAP_PARAM_AEC_MONO_CHANNEL: {
			/*if (value >= AUDIO_CH_STEREO) {
				DBG_ERR("Invalid channel=%d\r\n", value);
				ret = ISF_ERR_INVALID_VALUE;
			} else {
				isf_audout_mono_ch = value;
				ret = ISF_OK;
				ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set AEC channel(%d)\r\n", isf_audout_mono_ch);
			}*/
			break;
		}
	case AUDCAP_PARAM_AUD_ALC_EN: {
			isf_audcap_alc_en = value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set ALC en(%d)\r\n", isf_audcap_alc_en);
			break;
		}
	case AUDCAP_PARAM_REC_SRC: {
			if (value == 0) {
				//AMIC
				isf_audcap_rec_src = value;
			} else {
				//DMIC
				isf_audcap_rec_src = 1;
			}
			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set rec_src(%d)\r\n", isf_audcap_rec_src);
			break;
		}
	case AUDCAP_PARAM_AUD_PREPWR_EN: {
			UINT32 status = 0;
			ER retV;

			status = wavstudio_get_config(WAVSTUD_CFG_GET_STATUS, WAVSTUD_ACT_REC, 0);
			if (status == WAVSTUD_STS_CLOSED) {
				BOOL drv_param[3] = {0};
				UINT32 eid = KDRV_DEV_ID(0, KDRV_AUDCAP_ENGINE0, 0);
				isf_audcap_prepwr_en = value;
				if (isf_audcap_prepwr_en == 1) {
					drv_param[0] = TRUE;
				} else {
					drv_param[0] = FALSE;
				}
				retV = kdrv_audioio_open(0, KDRV_DEV_ID_ENGINE(eid));
				if (retV != 0) {
					DBG_ERR("kdrv_audioio open fail = %d\r\n", retV);
					ret = ISF_ERR_DRV;
					return ret;
				}
				kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_PDVCMBIAS_ALWAYS_ON, (VOID*)&drv_param[0]);
				kdrv_audioio_close(0, KDRV_DEV_ID_ENGINE(eid));
			} else {
				ret = ISF_ERR_INVALID_STATE;
			}
			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set PREPWR en(%d)\r\n", isf_audcap_prepwr_en);
			break;
		}
	case AUDCAP_PARAM_GAIN_LEVEL: {
			if (value == KDRV_AUDIO_CAP_GAIN_LEVEL8 || value == KDRV_AUDIO_CAP_GAIN_LEVEL16 || value == KDRV_AUDIO_CAP_GAIN_LEVEL32) {
				isf_audcap_gain_level = value;
			}
			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set gain level(%d)\r\n", isf_audcap_gain_level);
			break;
		}
	case AUDCAP_PARAM_TDM_CH: {
			if (value == 2 || value == 4 || value == 6 || value == 8) {
				isf_audcap_tdm_ch = value;
			}
			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set TDM channel(%d)\r\n", isf_audcap_tdm_ch);
			break;
		}
	case AUDCAP_PARAM_MONO_EXPAND: {
			if (value == 0) {
				mono_expand = value;
			} else {
				mono_expand = 1;
			}
			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set mono expand(%d)\r\n", mono_expand);
			break;
		}
	case AUDCAP_PARAM_AEC_REF_BYPASS_FUNC: {
			if (value == 0) {
				g_isf_audcap_aec_ref_bypass_func = value;
			} else {
				g_isf_audcap_aec_ref_bypass_func = TRUE;
			}

			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set AEC_REF_BYPASS_FUNC (%d)\r\n", g_isf_audcap_aec_ref_bypass_func);
			break;
		}
	case AUDCAP_PARAM_AEC_REF_BYPASS_ENABLE: {
			if (value == 0) {
				g_isf_audcap_aec_ref_bypass_enable= value;
			} else {
				g_isf_audcap_aec_ref_bypass_enable = TRUE;
			}

			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set AEC_REF_BYPASS_ENABLE (%d)\r\n", g_isf_audcap_aec_ref_bypass_enable);
			break;
		}
	case AUDCAP_PARAM_AEC_PRE_INIT: {

			if (value) {
				g_isf_audcap_aec_pre_init = TRUE;
			} else {
				BOOL aec_enable = FALSE;
				UINT32 i;

				for (i = 0; i < PATH_MAX_COUNT; i++) {
					AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];

					if (p_ctx_pi->aec_en) {
						aec_enable = TRUE;
						break;
					}
				}

				if (aec_enable && p_aec_obj && p_aec_obj->stop && p_aec_obj->close && g_isf_audcap_aec_pre_init) {
					p_aec_obj->stop();
					p_aec_obj->close();
				}

				g_isf_audcap_aec_pre_init = FALSE;
				g_isf_audcap_aec_pre_done = FALSE;
			}

			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set AEC pre init(%d)\r\n", value);
			break;
		}
	default:
		DBG_ERR("param[%08x] = %d\r\n", param, value);
		break;
	}
	return ret;
}
static UINT32 _isf_audcap_getparam(ISF_UNIT *p_thisunit, UINT32 param)
{
	UINT32 value = 0;

	if (param < AUDCAP_PARAM_START) {
		return 0;
	}
	switch (param) {
	case AUDCAP_PARAM_VOL_IMM: {
			value = wavstudio_get_config(WAVSTUD_CFG_VOL, WAVSTUD_ACT_REC, 0);
			break;
		}
	case AUDCAP_PARAM_AEC_EN: {
			AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

			value = p_ctx_p0->aec_en;
			break;
		}
	case AUDCAP_PARAM_ANR_EN: {
			AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

			value = p_ctx_p0->anr_en;
			break;
		}
	case AUDCAP_PARAM_AUDFILT_EN: {
			value = audfilt_en;
			break;
		}
	case AUDCAP_PARAM_FRAME_SAMPLE: {
			value = g_isf_audcap_frame_sample;
			break;
		}
#if (SUPPORT_PULL_FUNCTION)
	case AUDCAP_PARAM_BUFINFO_PHYADDR: {
		AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

		value = p_ctx_p0->isf_audcap_mmap_info.addr_physical;
		break;
		}
	case AUDCAP_PARAM_BUFINFO_SIZE:{
		AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

		value = p_ctx_p0->isf_audcap_mmap_info.size;
		break;
		}
#endif
	default:
		DBG_ERR("param[%08x]\r\n", param);
		break;
	}
	return value;
}


static ISF_RV _isf_audcap_setportparam(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 value)
{
	ISF_RV ret = ISF_OK;
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

    if (nport == ISF_CTRL)
    {
        return _isf_audcap_setparam(p_thisunit, param, value);
    }

	if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
		switch (param) {
		case AUDCAP_PARAM_AUD_DEFAULT_SETTING: {
			UINT32 path = nport - ISF_OUT_BASE;
			AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[path];

			p_ctx_p->isf_audcap_defset = value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, nport, "set default setting(%d)\r\n", p_ctx_p->isf_audcap_defset);
			break;
		}
		case AUDCAP_PARAM_VOL_IMM: {
			UINT32 path = nport - ISF_OUT_BASE;
			AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[path];

			if (value > 160) {
				DBG_WRN("Invalid volume=%d, set to 100\r\n", value);
				value = 100;
			}

			p_ctx_p->volume = value | WAVSTUD_PORT_VOLUME;

			ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, nport, "set volume(%d)", p_ctx_p->volume);
			break;
		}
		case ISF_PARAM_PORT_DIRTY:
			{
				return ISF_OK;
			}
			break;
		default:
			ret = ISF_ERR_NOT_SUPPORT;
			DBG_ERR("audcap.out[%d] set param[%08x] = %d\r\n", nport-ISF_OUT_BASE, param, value);
			break;
		}
	} else if (nport-ISF_IN_BASE < p_thisunit->port_in_count) {
		switch (param) {
		case ISF_PARAM_PORT_DIRTY:
			{

				//pAudInfo = (ISF_AUD_INFO *) & (p_thisunit->port_outinfo);
				//if (AUD_CHANNEL_COUNT(pAudInfo->channelcount) != wsis.aud_info.ch_num || pAudInfo->samplepersecond != wsis.aud_info.aud_sr) {
					ret = ISF_OK;
				//}
			}
			break;
		case AUDCAP_PARAM_CHANNEL: {
				if (value > AUDIO_CH_STEREO) {
					DBG_ERR("Invalid channel=%d\r\n", value);
					ret = ISF_ERR_INVALID_VALUE;
				} else {
					isf_audcap_mono_ch = value;
					ret = ISF_OK;
					ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, nport, "set mono channel(%d)", isf_audcap_mono_ch);
				}
				break;
			}
		case AUDCAP_PARAM_BUF_UNIT_TIME:
			if (value > WAVSTUD_UNIT_TIME_1000MS) {
				DBG_ERR("Invalid buffer unit time=%d\r\n", value);
				ret = ISF_ERR_INVALID_VALUE;
			} else {
				wsis.unit_time = value;
				ret = ISF_OK;
				//ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, nport, "set unit time(%d)\r\n", wsis.unitTime);
			}
			break;
		case AUDCAP_PARAM_BUF_COUNT:
			if (value < 2) {
				DBG_ERR("Invalid buffer count=%d\r\n", value);
				ret = ISF_ERR_INVALID_VALUE;
			} else {
				g_isf_audcap_buf_cnt = value;
				ret = ISF_OK;
				ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, nport, "set buff count(%d)", g_isf_audcap_buf_cnt);
			}
			break;
		case AUDCAP_PARAM_MAX_MEM_INFO: {
				/*ISF_RV r = ISF_OK;
				AUDCAP_MAX_MEM_INFO* pMaxMemInfo = (AUDCAP_MAX_MEM_INFO*) value;
				WAVSTUD_INFO_SET* pAudInfoSet = pMaxMemInfo->pAudInfoSet;

				memcpy((void *)&wsis_max, (const void *) pAudInfoSet, sizeof(WAVSTUD_INFO_SET));

				//ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, nport, "set max mem info(0x%x), release(%d)\r\n", pMaxMemInfo, pMaxMemInfo->bRelease);

				if (pMaxMemInfo->bRelease) {
					if (g_isf_audcap_maxmem.addr != 0) {
						r = p_thisunit->p_base->do_release_i(p_thisunit, &(isf_audcap_memblk), ISF_OK);
						if (r == ISF_OK) {
							g_isf_audcap_maxmem.addr = 0;
							g_isf_audcap_maxmem.size = 0;
						} else {
							DBG_ERR("[ISF_AUDCAP] release max buf failed, ret = %d\r\n", r);
						}

						if (g_isf_audcap_aecmem.addr) {
							r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_aecmemblk, ISF_OK);
							if (r != ISF_OK) {
								DBG_ERR("[ISF_AUDCAP] release AEC buf failed, ret = %d\r\n", r);
								return ISF_ERR_BUFFER_RELEASE;
							}
							g_isf_audcap_aecmem.addr = 0;
							g_isf_audcap_aecmem.size = 0;
						}

						if (g_isf_audcap_anrmem.addr) {
							r = p_thisunit->p_base->do_release_i(p_thisunit, &isf_audcap_anrmemblk, ISF_OK);
							if (r != ISF_OK) {
								DBG_ERR("[ISF_AUDCAP] release ANR buf failed, ret = %d\r\n", r);
								return ISF_ERR_BUFFER_RELEASE;
							}
							g_isf_audcap_anrmem.addr = 0;
							g_isf_audcap_anrmem.size = 0;
						}
					}
					g_isf_audcap_maxmem.size = 0;
				} else {
					if (g_isf_audcap_maxmem.addr == 0) {
						if (p_aec_obj) {
							wavstudio_set_config(WAVSTUD_CFG_AEC_OBJ, (UINT32)p_aec_obj, 0);
						} else {
							wavstudio_set_config(WAVSTUD_CFG_AEC_OBJ, 0, 0);
						}
						g_isf_audcap_maxmem.size = wavstudio_get_config(WAVSTUD_CFG_MEM, (UINT32)&wsis_max, WAVSTUD_ACT_REC);
					} else {
						DBG_ERR("[ISF_AUDCAP] max buf exist, please release it first.\r\n");
					}
				}*/
			}
			break;
		case AUDCAP_PARAM_AUD_CODEC:
			if (value == AUDCAP_AUD_CODEC_EMBEDDED) {
				wavstudio_set_config(WAVSTUD_CFG_AUD_CODEC, WAVSTUD_AUD_CODEC_EMBEDDED, 0);
				g_isf_audcap_audcodec = AUDCAP_AUD_CODEC_EMBEDDED;
			} else {
				wavstudio_set_config(WAVSTUD_CFG_AUD_CODEC, WAVSTUD_AUD_CODEC_EXTERNAL, 0);
				g_isf_audcap_audcodec = AUDCAP_AUD_CODEC_EXTERNAL;
			}
			ret = ISF_OK;
			break;
		case AUDCAP_PARAM_DUAL_MONO: {
				isf_audcap_dual_mono = (value)? TRUE : FALSE;
				ret = ISF_OK;
				ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, nport, "set dual mono(%d)", isf_audcap_dual_mono);
				break;
			}
		default:
			ret = ISF_ERR_NOT_SUPPORT;
			DBG_ERR("audcap.in[%d] set param[%08x] = %d\r\n", nport-ISF_IN_BASE, param, value);
			break;
		}
	}

	return ret;
}
static UINT32 _isf_audcap_getportparam(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param)
{
	UINT32 value = 0;
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

    if (nport == ISF_CTRL)
    {
        return _isf_audcap_getparam(p_thisunit, param);
    }

	if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
		switch (param) {
		default:
			DBG_ERR("audcap.out[%d] get param[%08x]\r\n", nport-ISF_OUT_BASE, param);
			break;
		}
	} else if (nport-ISF_IN_BASE < p_thisunit->port_in_count) {
		switch (param) {
		case AUDCAP_PARAM_CHANNEL:
			value = wsis.aud_info.aud_ch;
			break;
		case AUDCAP_PARAM_BUF_UNIT_TIME:
			value = wsis.unit_time;
			break;
		case AUDCAP_PARAM_BUF_COUNT:
			value = wsis.buf_count;
			break;
		case AUDCAP_PARAM_AUD_CODEC:
			value = g_isf_audcap_audcodec;
			break;
		default:
			DBG_ERR("audcap.in[%d] get param[%08x]\r\n", nport-ISF_IN_BASE, param);
			break;
		}
	}

	return value;
}

static ISF_RV _isf_audcap_setportstruct(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size)
{
	ISF_RV ret = ISF_OK;
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}


	if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
		switch (param) {
		case AUDCAP_PARAM_AEC_CONFIG:
			{
				AUDCAP_AEC_CONFIG* aec_config = (AUDCAP_AEC_CONFIG*)p_struct;
				UINT32 port = nport - ISF_OUT_BASE;
				AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[port];

				if (port < PATH_MAX_COUNT) {
					p_ctx_p->aec_en = aec_config->enabled;

					if (p_ctx_p->aec_en) {
						if (p_aec_obj && p_aec_obj->enable) {
							p_aec_obj->enable(aec_config->enabled);
						}

						switch (aec_config->lb_channel) {
						case AUDIOCAP_LB_CH_LEFT:
							p_ctx_p->isf_audout_ch = AUDIO_CH_LEFT;
							p_ctx_p->isf_audout_ch_num = 1;
							break;
						case AUDIOCAP_LB_CH_RIGHT:
							p_ctx_p->isf_audout_ch = AUDIO_CH_RIGHT;
							p_ctx_p->isf_audout_ch_num = 1;
							break;
						case AUDIOCAP_LB_CH_STEREO:
							p_ctx_p->isf_audout_ch = AUDIO_CH_STEREO;
							p_ctx_p->isf_audout_ch_num = 2;
							break;
						default:
							p_ctx_p->isf_audout_ch = AUDIO_CH_STEREO;
							p_ctx_p->isf_audout_ch_num = 2;
							break;
						}

						#if AEC_FUNC
						p_ctx_p->aec_config.leak_estimate_enabled = aec_config->leak_estimate_enabled;
						p_ctx_p->aec_config.leak_estimate_value   = aec_config->leak_estimate_value;
						p_ctx_p->aec_config.noise_cancel_level    = aec_config->noise_cancel_level;
						p_ctx_p->aec_config.echo_cancel_level     = aec_config->echo_cancel_level;
						p_ctx_p->aec_config.filter_length         = aec_config->filter_length;
						p_ctx_p->aec_config.frame_size            = aec_config->frame_size;
						p_ctx_p->aec_config.notch_radius          = aec_config->notch_radius;
						p_ctx_p->aec_config.lb_channel            = aec_config->lb_channel;
						//nvtaec_setconfig(NVTAEC_CONFIG_ID_SPK_NUMBER, isf_audout_ch_num[port]);
						//nvtaec_setconfig(NVTAEC_CONFIG_ID_RECORD_CH_NO, wsis_max.aud_info.ch_num);
						//nvtaec_setconfig(NVTAEC_CONFIG_ID_PLAYBACK_CH_NO, isf_audout_ch_num[port]);
						#endif
						ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, ISF_OUT(nport), "set AEC en(%d), leak_en(%d), leak(%d), noise(%d), echo(%d), filter(%d), frame(%d), notch(%d)",
						aec_config->enabled,
						aec_config->leak_estimate_enabled,
						aec_config->leak_estimate_value,
						aec_config->noise_cancel_level,
						aec_config->echo_cancel_level,
						aec_config->filter_length,
						aec_config->frame_size,
						aec_config->notch_radius);
					} else {
						if (p_aec_obj && p_aec_obj->enable) {
							p_aec_obj->enable(aec_config->enabled);
						}
					}

					return ISF_OK;
				} else {
					DBG_ERR("audcap invalid port index = %d\r\n", nport);
					return ISF_ERR_INVALID_PORT_ID;
				}
			}
			break;
		case AUDCAP_PARAM_AEC_MAX_CONFIG:
			{
				AUDCAP_AEC_CONFIG* aec_config = (AUDCAP_AEC_CONFIG*)p_struct;

				if (aec_config->enabled) {
					p_aec_obj = &aec_obj;

					//wavstudio_set_config(WAVSTUD_CFG_AEC_EN, TRUE, 0);

					#if AEC_FUNC
					aec_config_max.leak_estimate_enabled = aec_config->leak_estimate_enabled;
					aec_config_max.leak_estimate_value   = aec_config->leak_estimate_value;
					aec_config_max.noise_cancel_level    = aec_config->noise_cancel_level;
					aec_config_max.echo_cancel_level     = aec_config->echo_cancel_level;
					aec_config_max.filter_length         = aec_config->filter_length;
					aec_config_max.frame_size            = aec_config->frame_size;
					aec_config_max.notch_radius          = aec_config->notch_radius;
					aec_config_max.lb_channel            = aec_config->lb_channel;
					#endif
					ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, ISF_OUT(nport), "set AEC max, leak_en(%d), leak(%d), noise(%d), echo(%d), filter(%d), frame(%d), notch(%d)",
					aec_config->enabled,
					aec_config->leak_estimate_enabled,
					aec_config->leak_estimate_value,
					aec_config->noise_cancel_level,
					aec_config->echo_cancel_level,
					aec_config->filter_length,
					aec_config->frame_size,
					aec_config->notch_radius);
				}

				return ISF_OK;
			}
			break;
		case AUDCAP_PARAM_ANR_MAX_CONFIG:
			{
				AUDCAP_ANR_CONFIG* anr_config = (AUDCAP_ANR_CONFIG*)p_struct;

				if (anr_config->enabled) {
					p_anr_obj = &anr_obj;
					if (p_anr_obj && p_anr_obj->enable) {
						p_anr_obj->enable(anr_config->enabled);
					}

					#if ANR_FUNC
					nvtanr_setconfig(NVTANR_CONFIG_ID_NRDB, anr_config->suppress_level);
					nvtanr_setconfig(NVTANR_CONFIG_ID_HPF_FREQ, anr_config->hpf_cut_off_freq);
					nvtanr_setconfig(NVTANR_CONFIG_ID_BIAS_SENSITIVE, anr_config->bias_sensitive);
					#endif
					ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, ISF_OUT(nport), "set ANR en(%d), suppress(%d), HPF(%d), bias(%d)",
					anr_config->enabled,
					anr_config->suppress_level,
					anr_config->hpf_cut_off_freq,
					anr_config->bias_sensitive);
				} else {
					if (p_anr_obj && p_anr_obj->enable) {
						p_anr_obj->enable(anr_config->enabled);
					}
					p_anr_obj = NULL;
				}

				return ISF_OK;
			}
			break;
		case AUDCAP_PARAM_ANR_CONFIG:
			{
				AUDCAP_ANR_CONFIG* anr_config = (AUDCAP_ANR_CONFIG*)p_struct;
				AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

				p_ctx_p0->anr_en = anr_config->enabled;

				if (p_ctx_p0->anr_en) {
					if (p_anr_obj && p_anr_obj->enable) {
						p_anr_obj->enable(anr_config->enabled);
					}

					#if ANR_FUNC
					nvtanr_setconfig(NVTANR_CONFIG_ID_NRDB, anr_config->suppress_level);
					nvtanr_setconfig(NVTANR_CONFIG_ID_HPF_FREQ, anr_config->hpf_cut_off_freq);
					nvtanr_setconfig(NVTANR_CONFIG_ID_BIAS_SENSITIVE, anr_config->bias_sensitive);
					#endif
					ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, p_thisunit, ISF_OUT(nport), "set ANR en(%d), suppress(%d), HPF(%d), bias(%d)",
					anr_config->enabled,
					anr_config->suppress_level,
					anr_config->hpf_cut_off_freq,
					anr_config->bias_sensitive);
				} else {
					if (p_anr_obj && p_anr_obj->enable) {
						p_anr_obj->enable(anr_config->enabled);
					}
				}

				return ISF_OK;
			}
			break;
		case AUDCAP_PARAM_AGC_CFG: {
			AUDCAP_AGC_CONFIG *p_aud_agc = (AUDCAP_AGC_CONFIG *)p_struct;
			UINT32 port = nport - ISF_OUT_BASE;
			AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[port];

			p_ctx_p->isf_audcap_agc_cfg.enable       = p_aud_agc->enable;
			p_ctx_p->isf_audcap_agc_cfg.target_lvl   = p_aud_agc->target_lvl;
			p_ctx_p->isf_audcap_agc_cfg.ng_threshold = p_aud_agc->ng_threshold;

			//p_ctx_p->isf_audcap_agc_cfg.decay_time   = p_aud_agc->decay_time;
			//p_ctx_p->isf_audcap_agc_cfg.attack_time  = p_aud_agc->attack_time;
			//p_ctx_p->isf_audcap_agc_cfg.max_gain     = p_aud_agc->max_gain;
			//p_ctx_p->isf_audcap_agc_cfg.min_gain     = p_aud_agc->min_gain;
			}
			break;
		case AUDCAP_PARAM_AUD_NG_THRESHOLD: {
			INT32 threshold = *(INT32*)p_struct;
			UINT32 port = nport - ISF_OUT_BASE;
			AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[port];

			if ((threshold > -77 && threshold < -30) || (threshold == 0)) {
				p_ctx_p->isf_audcap_ng_thd = threshold;
			} else {
				DBG_ERR("Invalid threshold=%d\r\n", threshold);
				ret = ISF_ERR_INVALID_VALUE;
			}

			ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, p_thisunit, ISF_CTRL, "set noise gate threshold(%d)\r\n", *(INT32*)p_struct);
			break;
			}
		default:
			ret = ISF_ERR_NOT_SUPPORT;
			DBG_ERR("audcap.out[%d] set param[%08x]\r\n", nport-ISF_OUT_BASE, param);
			break;
		}
	} else if (nport-ISF_IN_BASE < p_thisunit->port_in_count) {
		switch (param) {
		default:
			ret = ISF_ERR_NOT_SUPPORT;
			DBG_ERR("audcap.in[%d] set param[%08x]\r\n", nport-ISF_IN_BASE, param);
			break;
		}
	} else if (nport == ISF_CTRL) {
		/* for common port */
		switch (param) {
		case AUDCAP_PARAM_AUD_INIT_CFG:	{
			AUDCAP_AUD_INIT_CFG *p_aud_init = (AUDCAP_AUD_INIT_CFG *)p_struct;
			CTL_AUD_INIT_CFG_OBJ cfg_obj;
			memset((void *)(&cfg_obj), 0, sizeof(CTL_AUD_INIT_CFG_OBJ));
			cfg_obj.pin_cfg.pinmux.audio_pinmux  = p_aud_init->aud_init_cfg.pin_cfg.pinmux.audio_pinmux;
			cfg_obj.pin_cfg.pinmux.cmd_if_pinmux = p_aud_init->aud_init_cfg.pin_cfg.pinmux.cmd_if_pinmux;
			cfg_obj.i2s_cfg.bit_width     = p_aud_init->aud_init_cfg.i2s_cfg.bit_width;
			cfg_obj.i2s_cfg.bit_clk_ratio = p_aud_init->aud_init_cfg.i2s_cfg.bit_clk_ratio;
			cfg_obj.i2s_cfg.op_mode       = p_aud_init->aud_init_cfg.i2s_cfg.op_mode;
			cfg_obj.i2s_cfg.tdm_ch        = p_aud_init->aud_init_cfg.i2s_cfg.tdm_ch;
			ret = ctl_aud_module_init_cfg(CTL_AUD_ID_CAP, (CHAR *)p_aud_init->driver_name, &cfg_obj);
			if(ret) {
				DBG_ERR("aud init_cfg failed(%d)!\r\n", ret);
				ret = ISF_ERR_INVALID_VALUE;
				break;
			}

			isf_audcap_tdm_ch = p_aud_init->aud_init_cfg.i2s_cfg.tdm_ch;
		}
		break;
		case AUDCAP_PARAM_LB_CONFIG: {
			AUDCAP_LB_CONFIG *p_lb = (AUDCAP_LB_CONFIG *)p_struct;
			UINT32 port = 0;
			AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[port];

			if (port < ISF_AUDCAP_OUT_NUM) {
				//DBG_DUMP("[isf_audcap] set lb [%d] %d\r\n", port, p_lb->enabled);

				if (p_lb->enabled) {
					p_ctx_p->isf_audcap_lb_en = TRUE;
					//wavstudio_set_config(WAVSTUD_CFG_AEC_EN, TRUE, 0);

					switch (p_lb->lb_channel) {
					case AUDIOCAP_LB_CH_LEFT:
						p_ctx_p->isf_audout_ch = AUDIO_CH_LEFT;
						p_ctx_p->isf_audout_ch_num = 1;
						break;
					case AUDIOCAP_LB_CH_RIGHT:
						p_ctx_p->isf_audout_ch = AUDIO_CH_RIGHT;
						p_ctx_p->isf_audout_ch_num = 1;
						break;
					case AUDIOCAP_LB_CH_STEREO:
						p_ctx_p->isf_audout_ch = AUDIO_CH_STEREO;
						p_ctx_p->isf_audout_ch_num = 2;
						break;
					default:
						p_ctx_p->isf_audout_ch = AUDIO_CH_STEREO;
						p_ctx_p->isf_audout_ch_num = 2;
						break;
					}
				} else {
					p_ctx_p->isf_audcap_lb_en = FALSE;
					//wavstudio_set_config(WAVSTUD_CFG_AEC_EN, FALSE, 0);
				}

				return ISF_OK;
			} else {
				DBG_ERR("audcap invalid port index = %d\r\n", nport);
				return ISF_ERR_INVALID_PORT_ID;
			}
		}
		break;
		case AUDCAP_PARAM_AUD_ALC_CONFIG:	{
			AUDCAP_ALC_CONFIG *p_aud_alc = (AUDCAP_ALC_CONFIG *)p_struct;
			isf_audcap_alc_cfg.decay_time = p_aud_alc->decay_time;
			isf_audcap_alc_cfg.attack_time = p_aud_alc->attack_time;
			isf_audcap_alc_cfg.max_gain = p_aud_alc->max_gain;
			isf_audcap_alc_cfg.min_gain = p_aud_alc->min_gain;
			isf_audcap_alc_cfg.enable = 1;
			DBG_IND("decay_time=%d attack_time=%d max_gain=%d min_gain=%d enable%d\r\n",
				(int)isf_audcap_alc_cfg.decay_time, (int)isf_audcap_alc_cfg.attack_time,
				(int)isf_audcap_alc_cfg.max_gain, (int)isf_audcap_alc_cfg.min_gain, (int)isf_audcap_alc_cfg.enable);
		}
		break;
		default:
			ret = ISF_ERR_NOT_SUPPORT;
			DBG_ERR("audcap.ctrl set param[%08x]\r\n", param);
			break;
		}
	}

	return ret;
}

static ISF_RV _isf_audcap_getportstruct(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 *p_struct, UINT32 size)
{
	DBG_UNIT("audcap port[%d] param = 0x%X size = %d\r\n", nport-ISF_IN_BASE, param, size);
	if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
	/* for output port */
	} else if (nport-ISF_IN_BASE < p_thisunit->port_in_count) {
	/* for input port */
	} else if (nport == ISF_CTRL) {
		/* for common port */
		switch (param) {
		case AUDCAP_PARAM_SYS_INFO: {
			AUDCAP_SYSINFO *p_sys_info = (AUDCAP_SYSINFO *)p_struct;
			UINT8 i;

			p_sys_info->cur_in_sample_rate  = wsis.aud_info.aud_sr;
			p_sys_info->cur_sample_bit      = wsis.aud_info.bitpersample;
			p_sys_info->cur_mode            = wsis.aud_info.ch_num;
			for (i = 0; i < AUDCAP_MAX_OUT; i++) {
				p_sys_info->cur_out_sample_rate[i] = 0;
			}
			return ISF_OK;
		}
		default:
			DBG_ERR("audcap.out[%d] get struct[%08x] = %08x\r\n", nport-ISF_OUT_BASE, param, (UINT32)p_struct);
			break;
		}
	}


	return ISF_ERR_NOT_SUPPORT;
}



ISF_RV _isf_audcap_updateport(struct _ISF_UNIT *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd)
{
	ISF_RV r = ISF_OK;
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}
	//AUDCAP_CONTEXT_PORT *p_ctx_p = NULL;

	if (oport-ISF_OUT_BASE >= PATH_MAX_COUNT) {
		if (cmd != ISF_PORT_CMD_RESET) DBG_ERR("[AUDCAP] invalid path index = %d\r\n", oport-ISF_OUT_BASE);
		return ISF_ERR_INVALID_PORT_ID;
	}
	//p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[oport-ISF_OUT_BASE];

	switch (cmd) {
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case ISF_PORT_CMD_OFFTIME_SYNC:     ///< off -> off     (sync port info & sync off-time parameter if it is dirty)
		_isf_audcap_update_port_info(p_thisunit, oport);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(off-sync)");
		break;
	case ISF_PORT_CMD_OPEN:             ///< off -> ready   (alloc mempool and start task)
		_isf_audcap_update_port_info(p_thisunit, oport);
		r = _isf_audcap_do_open(p_thisunit, oport);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(open)");
		break;
	case ISF_PORT_CMD_READYTIME_SYNC:   ///< ready -> ready (sync port info & sync ready-time parameter if it is dirty)
		_isf_audcap_update_port_info(p_thisunit, oport);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(ready-sync)");
		break;
	case ISF_PORT_CMD_START:            ///< ready -> run   (initial context, engine enable and device power on, start data processing)
		_isf_audcap_update_port_info(p_thisunit, oport);
		r = _isf_audcap_do_start(p_thisunit, oport);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(start)");
		break;
	case ISF_PORT_CMD_RUNTIME_SYNC:     ///< run -> run     (sync port info & sync run-time parameter if it is dirty)
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(run-sync)");
		break;
	case ISF_PORT_CMD_RUNTIME_UPDATE:   ///< run -> run     (update change)
		_isf_audcap_do_runtime(p_thisunit, cmd, oport);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(run-update)");
		break;
	case ISF_PORT_CMD_STOP:             ///< run -> stop    (stop data processing, engine disable or device power off)
		r = _isf_audcap_do_stop(p_thisunit, oport);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(stop)");
		break;
	case ISF_PORT_CMD_CLOSE:            ///< stop -> off    (terminate task and free mempool)
		if (FALSE == _isf_audcap_is_pullque_all_released(oport)) {
			DBG_ERR("Close failed !! Please release all pull_out bitstream before attemping to close !!\r\n");
			return ISF_ERR_NOT_REL_YET;
		}
		r = _isf_audcap_do_close(p_thisunit, oport, TRUE);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(close)");
		break;
	case ISF_PORT_CMD_RESET:
		_isf_audcap_do_reset(p_thisunit, oport);
		break;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case ISF_PORT_CMD_PAUSE:            ///< run -> wait    (pause data processing, keep context)
		r = _isf_audcap_do_pause(p_thisunit, oport);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(pause)");
		break;
	case ISF_PORT_CMD_RESUME:           ///< wait -> run    (restore context, resume data processing)
		r = _isf_audcap_do_resume(p_thisunit, oport);
		ISF_UNIT_TRACE(ISF_OP_STATE, p_thisunit, oport, "(resume)");
		break;
	case ISF_PORT_CMD_SLEEP:            ///< run -> sleep   (pause data processing, keep context, engine or device enter suspend mode)
		break;
	case ISF_PORT_CMD_WAKEUP:           ///< sleep -> run   (engine or device leave suspend mode, restore context, resume data processing)
		break;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	default:
		break;
	}
	return r;
}

void _isf_audcap_depack(UINT32 path, UINT32 input_addr, UINT32 output_addr, PWAVSTUD_AUD_INFO info, UINT32 size)
{
	UINT32 i;
	UINT32 in_sample_cnt = size / info->ch_num / (info->bitpersample>>3);

	for(i = 0; i < in_sample_cnt; i++) {
		*(((UINT16 *)output_addr) + i) = *(((UINT16 *)input_addr) + ((i<<(info->ch_num >> 1)) + path));
	}
}

void _isf_audcap_swap_channel(UINT32 addr, UINT32 size)
{
	UINT32 i;
	UINT32 in_sample_cnt = size / 2;
	UINT16 tmp_sample = 0;

	for(i = 0; i < in_sample_cnt; i = i + 2) {
		tmp_sample = *(((UINT16 *)addr) + i);
		*(((UINT16 *)addr) + i) = *(((UINT16 *)addr) + i + 1);
		*(((UINT16 *)addr) + i + 1) = tmp_sample;
	}
}

static UINT32 _isf_audcap_src_proc(int handle, UINT32 addr, UINT32 input, UINT32 output)
{
	UINT32 buf_in, buf_out;
	//UINT32 slice = input;
	UINT32 proc_size = 0;
	UINT32 one_frame = output;

	if (paudsrc_obj == NULL) {
		DBG_ERR("src obj is NULL\r\n");
		return input;
	}

	if (output > g_isf_audcap_srcbufmem.size) {
		DBG_ERR("Buf is not enough. Need %x, but %x\r\n", output, g_isf_audcap_srcbufmem.size);
		return input;
	}

	buf_in  = addr;
	buf_out = g_isf_audcap_srcbufmem.addr;

	do {
		buf_in  = buf_in + proc_size;

		if (paudsrc_obj->src.run) {
			paudsrc_obj->src.run(handle, (void *) buf_in, (void *) buf_out);
		}

		memcpy((void *)(addr + proc_size), (const void *) buf_out, one_frame);
		proc_size += input;
	} while (proc_size < input);

	return output;
}

static BOOL _isf_audcap_src_check(UINT32 resample)
{
	UINT32 input_frame = 0;
	AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

	if ((resample == 0) || (resample == wsis.aud_info.aud_sr)) {
		p_ctx_p0->isf_audcap_resample_en = FALSE;
		return TRUE;
	}

	if (resample > wsis.aud_info.aud_sr) {
		DBG_ERR("Not support up sampling. input sr=%d, output sr=%d\r\n", wsis.aud_info.aud_sr, resample);
		p_ctx_p0->isf_audcap_resample_en = FALSE;
	} else {
		if (g_isf_audcap_frame_block_sample != 0) {
			input_frame = g_isf_audcap_frame_block_sample;
		} else {
			input_frame = wsis.aud_info.buf_sample_cnt;
		}

		if (((resample * input_frame)%wsis.aud_info.aud_sr) != 0) {
			DBG_ERR("Not supported resample parameter, input sr=%d, output sr=%d, input frame sample=%d\r\n", wsis.aud_info.aud_sr, resample, input_frame);
			p_ctx_p0->isf_audcap_resample_en = FALSE;
		} else {
			p_ctx_p0->isf_audcap_resample_frame = resample*input_frame/wsis.aud_info.aud_sr;

			if (paudsrc_obj) {
				if (paudsrc_obj->src.pre_init) {
					UINT32 need = paudsrc_obj->src.pre_init(wsis.aud_info.ch_num, input_frame, p_ctx_p0->isf_audcap_resample_frame, FALSE);

					if (g_isf_audcap_srcmem.size < need) {
						DBG_ERR("resample mem not enough need=%x but=%x\r\n", need, g_isf_audcap_srcmem.size);
						p_ctx_p0->isf_audcap_resample_en = FALSE;
					} else {
						if (paudsrc_obj->src.init) {
							paudsrc_obj->src.init(&resample_handle_rec, wsis.aud_info.ch_num, input_frame, p_ctx_p0->isf_audcap_resample_frame, FALSE, (short *)g_isf_audcap_srcmem.addr);
						}
						p_ctx_p0->isf_audcap_resample_en = TRUE;
					}
				}
			}
		}
	}

	return TRUE;
}

void _isf_audcap_audproc_cb(UINT32 addr, UINT32 addr_aec, UINT32 size, UINT32 size_aec)
{
	#if PERF_CHK == ENABLE
	VOS_TICK proc_tick_begin, proc_tick_end;
	#endif

	#if TEST_SIN_PATT
	INT16 *temp_addr;
	UINT32 k;

	temp_addr = (INT16 *)addr;

	for(k = 0; k < ((size / (wsis.aud_info.bitpersample >> 3)) / wsis.aud_info.ch_num);k++){
	    *(temp_addr+k*2) = sin_table[sin_count];
		*(temp_addr+k*2+1) = sin_table[sin_count];
		sin_count = (sin_count + 1)%48;
    }
	return;
	#endif
	BOOL aec_enable = FALSE;
	UINT32 i;
	AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];

		if (p_ctx_pi->aec_en) {
			aec_enable = TRUE;
			break;
		}
	}

	if (wsis.aud_info.aud_sr == AUDIO_SR_44100) {
		aec_enable = FALSE;
	}

	#if (DUMP_TO_FILE_TEST_KERNEL_VER == ENABLE)
	if ((temp_cnt_r + size) < 2000000) {
		memcpy((void*)(temp_buf_r + temp_cnt_r), (const void*)addr, size);
		temp_cnt_r += size;
	}
	#endif

	if (aec_enable && p_aec_obj && p_aec_obj->apply) {
		if (isf_audcap_aec_lb_swap) {
			_isf_audcap_swap_channel(addr_aec, size_aec);
		}

		if (addr != 0 && addr_aec != 0 && size != 0) {
			#if (DUMP_TO_FILE_TEST_KERNEL_VER == ENABLE)
			if ((temp_cnt_l + size) < 2000000) {
				memcpy((void*)(temp_buf_l + temp_cnt_l), (const void*)addr_aec, size);
				temp_cnt_l += size;
			}
			#endif

			#if PERF_CHK == ENABLE
			if (aec_count < 200) {
				vos_perf_mark(&proc_tick_begin);
			}
			#endif

			if (g_isf_audcap_aec_ref_bypass && (g_isf_audcap_aec_ref_bypass_mem.addr != 0) && (size <= g_isf_audcap_aec_ref_bypass_mem.size)) {
				p_aec_obj->apply(addr, g_isf_audcap_aec_ref_bypass_mem.addr, addr, size);
			} else {
				p_aec_obj->apply(addr, addr_aec, addr, size);
			}

			#if PERF_CHK == ENABLE
			if (aec_count < 200) {
				vos_perf_mark(&proc_tick_end);
				aec_time += vos_perf_duration(proc_tick_begin, proc_tick_end);
				aec_count++;

				if (aec_count == 200) {
					UINT32 cpu_usage = ((aec_time/200) * (wsis.aud_info.aud_sr/1000) * 10) / g_isf_audcap_frame_sample;

					DBG_DUMP("[isf_audcap] AEC average process usage = %d,%d%%\r\n", cpu_usage/100, cpu_usage%100);
				}
			}
			#endif
		}
	}

	if (p_ctx_p0->anr_en && p_anr_obj && p_anr_obj->apply) {
		#if PERF_CHK == ENABLE
		if (anr_count < 200) {
			vos_perf_mark(&proc_tick_begin);
		}
		#endif

		p_anr_obj->apply(addr, size);

		#if PERF_CHK == ENABLE
		if (anr_count < 200) {
			vos_perf_mark(&proc_tick_end);
			anr_time += vos_perf_duration(proc_tick_begin, proc_tick_end);
			anr_count++;

			if (anr_count == 200) {
				UINT32 cpu_usage = ((anr_time/200) * (wsis.aud_info.aud_sr/1000) * 10) / g_isf_audcap_frame_sample;

				DBG_DUMP("[isf_audcap] ANR average process usage = %d,%d%%\r\n", cpu_usage/100, cpu_usage%100);
			}
		}
		#endif
	}

	if (p_audfilt_obj && audfilt_en && p_audfilt_obj->apply) {
		p_audfilt_obj->apply(addr, addr, size);
	}

	#if AGC_FUNC == ENABLE
	if (p_ctx_p0->isf_audcap_agc_cfg.enable) {
		if (paudagc_obj) {
			if (paudagc_obj->agc.run) {
				AGC_BITSTREAM agc_io;
				agc_io.bitstram_buffer_in     = addr;
				agc_io.bitstram_buffer_out    = addr;
				agc_io.bitstram_buffer_length = size >> 1; //sample num
				paudagc_obj->agc.run(&agc_io);
			}
		}
	}
	#endif
}

BOOL _isf_audcap_cb(PAUDIO_BUF_QUEUE pAudBufQue, PAUDIO_BUF_QUEUE pAudBufQue_AEC, UINT32 id, UINT64 timestamp, UINT64 timestamp_aec)
{
	PAUD_FRAME pAudBuf = (PAUD_FRAME)isf_audcap_bsdata.desc;
	ISF_PORT *pDest = isf_audcap.p_base->oport(&isf_audcap, ISF_OUT(0));
	//ISF_PORT *pSrc = isf_audcap.p_base->iport(&isf_audcap, ISF_IN(0));
	AUDCAP_BSDATA *pAudCapData = NULL;
	UINT32 frame_count = 0;
	UINT8 i = 0;
	UINT32 out_start_addr = 0;
	UINT32 block_start_addr = 0;
	UINT32 this_size = 0;
	AUDCAP_BUF_INFO *p_aud_bufinfo = NULL;
	UINT32 block_size = 0;
	UINT64 this_ts = 0;
	UINT64 this_aec_ts = 0;
	UINT64 remain_ts = 0;
	unsigned long flags;
	UINT32 path;
	ISF_UNIT *p_thisunit = &isf_audcap;
	UINT32 out_sr = wsis.aud_info.aud_sr;
	ISF_RV ret;
	UINT32 aec_out_start_addr = 0;
	UINT32 aec_block_start_addr = 0;
	UINT32 aec_this_size = 0;
	UINT32 aec_block_size = 0;

	AUDCAP_CONTEXT_PORT *p_ctx_p0 = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

	//proc cb only valid for path 0
	_isf_audcap_audproc_cb(pAudBufQue->uiAddress, pAudBufQue_AEC->uiAddress, pAudBufQue->uiValidSize, pAudBufQue_AEC->uiValidSize);

	if (p_ctx_p0->isf_audcap_resample_dirty) {
		//update resample sample rate
		if (paudsrc_obj && p_ctx_p0->isf_audcap_resample_en) {
			if (paudsrc_obj->src.destroy) {
				paudsrc_obj->src.destroy(resample_handle_rec);
			}
		}
		_isf_audcap_src_check(p_ctx_p0->isf_audcap_resample_update);
		p_ctx_p0->isf_audcap_resample = p_ctx_p0->isf_audcap_resample_update;

		p_ctx_p0->isf_audcap_resample_update = 0;
		p_ctx_p0->isf_audcap_resample_dirty = 0;
	}

	if (g_isf_audcap_frame_block_sample != 0) {
		//frame size is not multiple of 1024
		if (isf_audcap_dual_mono && wsis.aud_info.ch_num > 1) {
			this_size = pAudBufQue->uiValidSize + isf_audcap_last_left[0].size;
			block_size = g_isf_audcap_frame_block_sample * wsis.aud_info.ch_num *(wsis.aud_info.bitpersample>>3);
			frame_count = this_size / block_size;
			out_start_addr = pAudBufQue->uiAddress;

			if (isf_audcap_last_left[0].size != 0) {
				out_start_addr -= isf_audcap_last_left[0].size;
				if (isf_audcap_last_left[0].addr > out_start_addr) {
					memcpy((void*)out_start_addr, (const void*)isf_audcap_last_left[0].addr, isf_audcap_last_left[0].size);
				}

				remain_ts = _isf_audcap_do_div((UINT64)(isf_audcap_last_left[0].size/(wsis.aud_info.bitpersample>>3)/wsis.aud_info.ch_num)*1000000, (UINT64)wsis.aud_info.aud_sr);
			} else {
				remain_ts = 0;
			}

			p_aud_bufinfo = _isf_audcap_get_buf_info();

			if (p_aud_bufinfo == 0) {
				isf_audcap.p_base->do_new(&isf_audcap, ISF_OUT(0), NVTMPP_VB_INVALID_POOL, 0, 0, &isf_audcap_bsdata, ISF_OUT_PROBE_NEW);
				isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL);
				//DBG_ERR("_ISF_AudCap_NEW_BSDATA is NULL!\r\n");
				return ISF_ERR_QUEUE_FULL;
			}

			p_aud_bufinfo->buf_addr = pAudBufQue->uiAddress;
			p_aud_bufinfo->occupy = 0;

			if (this_size - block_size*frame_count > 0) {
				for (path = 0; path < wsis.aud_info.ch_num; path++) {
					AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[path];

					if (p_ctx_pi->isf_audcap_started) {
						p_aud_bufinfo->occupy += frame_count + 1;
					}
				}
			} else {
				for (path = 0; path < wsis.aud_info.ch_num; path++) {
					AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[path];

					if (p_ctx_pi->isf_audcap_started) {
						p_aud_bufinfo->occupy += frame_count;
					}
				}
			}

			block_start_addr = out_start_addr;
			this_ts = timestamp - (isf_audcap_timestamp.frame_time + remain_ts);
			this_aec_ts = timestamp_aec - (isf_audcap_timestamp.frame_time + remain_ts);

			if (p_ctx_p0->isf_audcap_lb_en == TRUE && pAudBufQue_AEC->uiAddress != 0 && pAudBufQue_AEC->uiValidSize != 0) {
				UINT32 lb_ch_num = (p_ctx_p0->isf_audout_ch == AUDIO_CH_STEREO)? 2 : 1;

				aec_this_size = pAudBufQue_AEC->uiValidSize + isf_audcap_aec_last_left[0].size;
				aec_block_size = g_isf_audcap_frame_block_sample * lb_ch_num *(wsis.aud_info.bitpersample>>3);
				aec_out_start_addr = pAudBufQue_AEC->uiAddress;

				if (isf_audcap_aec_last_left[0].size != 0) {
					aec_out_start_addr -= isf_audcap_aec_last_left[0].size;
					if (isf_audcap_aec_last_left[0].addr > aec_out_start_addr) {
						memcpy((void*)aec_out_start_addr, (const void*)isf_audcap_aec_last_left[0].addr, isf_audcap_aec_last_left[0].size);
					}
				}

				aec_block_start_addr = aec_out_start_addr;
			}

			for (i = 0; i < frame_count; i++) {
				UINT32 output_size, buf_size;

				//do resampling
				if (p_ctx_p0->isf_audcap_resample_en) {
					buf_size = _isf_audcap_src_proc(resample_handle_rec, block_start_addr, block_size, p_ctx_p0->isf_audcap_resample_frame * wsis.aud_info.ch_num * (wsis.aud_info.bitpersample>>3));
					out_sr = p_ctx_p0->isf_audcap_resample;
				} else {
					buf_size = block_size;
				}

				output_size = buf_size / wsis.aud_info.ch_num;
				this_ts = this_ts + isf_audcap_timestamp.block_time;
				this_aec_ts = this_aec_ts + isf_audcap_timestamp.block_time;
				for (path = 0; path < wsis.aud_info.ch_num; path++) {
					AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[path];

					if (p_ctx_pi->isf_audcap_started) {

						p_thisunit->p_base->do_probe(p_thisunit, ISF_OUT(path), ISF_OUT_PROBE_PUSH, ISF_ENTER);

						if ((p_ctx_pi->isf_audcap_output_mem.addr + output_size) > (p_ctx_pi->g_isf_audcap_pathmem.addr + p_ctx_pi->g_isf_audcap_pathmem.size)) {
							p_ctx_pi->isf_audcap_output_mem.addr = p_ctx_pi->g_isf_audcap_pathmem.addr;
						}

						_isf_audcap_depack(path, block_start_addr, p_ctx_pi->isf_audcap_output_mem.addr, &wsis.aud_info, buf_size);


						pAudCapData = _isf_audcap_get_bsdata();

						if (pAudCapData != 0) {
							pAudCapData->hData    = p_ctx_pi->isf_audcap_output_mem.addr;
							pAudCapData->mem.addr = p_ctx_pi->isf_audcap_output_mem.addr;
							pAudCapData->mem.size = output_size;
							pAudCapData->refCnt   = 1;
							pAudCapData->p_buf_info = p_aud_bufinfo;

							if (p_isf_audcap_remain_info[0] != 0) {
								pAudCapData->p_remain_buf_info = p_isf_audcap_remain_info[0];
								//DBG_DUMP("n remain = %x\r\n", (UINT32)pAudCapData->p_remain_buf_info);
							} else {
								pAudCapData->p_remain_buf_info = 0;
							}
							//DBG_DUMP("AudCap lock hData = %x\r\n", pAudCapData->hData);
						} else {
							//new pData by user
							isf_audcap.p_base->do_new(&isf_audcap, ISF_OUT(path), NVTMPP_VB_INVALID_POOL, 0, 0, &isf_audcap_bsdata, ISF_OUT_PROBE_NEW);
							isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(path), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL);
							//DBG_ERR("_ISF_AudCap_NEW_BSDATA is NULL!\r\n");
							return ISF_ERR_QUEUE_FULL;
						}

						// add AUDCAP_BSDATA link list
						loc_cpu(flags);
						if (p_ctx_pi->g_p_audcap_bsdata_link_head == NULL) {
							p_ctx_pi->g_p_audcap_bsdata_link_head = pAudCapData;
							p_ctx_pi->g_p_audcap_bsdata_link_tail = p_ctx_pi->g_p_audcap_bsdata_link_head;
						} else {
							p_ctx_pi->g_p_audcap_bsdata_link_tail->pnext_bsdata = pAudCapData;
							p_ctx_pi->g_p_audcap_bsdata_link_tail               = pAudCapData;
						}
						unl_cpu(flags);

						isf_audcap_bsdata.sign = ISF_SIGN_DATA;
						//isf_audcap_bsdata.TimeStamp = timestamp;
						isf_audcap_bsdata.h_data = ((path << 8) & 0xFF00) | ((pAudCapData->blk_id + 1) & 0xFF);
						//new pData by user
						isf_audcap.p_base->do_new(&isf_audcap, ISF_OUT(path), NVTMPP_VB_INVALID_POOL, isf_audcap_bsdata.h_data, 0, &isf_audcap_bsdata, ISF_OUT_PROBE_NEW);

						//pAudBuf->sign = MAKEFOURCC('A', 'R', 'A', 'W');
						pAudBuf->addr[0] = pAudCapData->mem.addr;
						pAudBuf->size = pAudCapData->mem.size;
						pAudBuf->sound_mode = 1;
						pAudBuf->sample_rate = out_sr;
						pAudBuf->bit_width= wsis.aud_info.bitpersample;


						pAudBuf->timestamp = this_ts;

						//DBG_DUMP("ts = %d %d\r\n", (UINT32)(pAudBuf->timestamp >> 32), (UINT32)(pAudBuf->timestamp & 0xffffffff));
#if (SUPPORT_PULL_FUNCTION)
						// convert to physical addr
						pAudBuf->phyaddr[0] = AUDCAP_VIRT2PHY(0, pAudBuf->addr[0]);
						if (path == 0) {
							pAudBuf->reserved[0] = AUDCAP_VIRT2PHY(0, aec_block_start_addr);
							pAudBuf->reserved[1] = aec_block_size;
							pAudBuf->reserved[2] = (UINT32)(this_aec_ts & 0xFFFFFFFF);
							pAudBuf->reserved[3] = (UINT32)(this_aec_ts >> 32);
							aec_block_start_addr += aec_block_size;
						}
#endif
						pDest = isf_audcap.p_base->oport(&isf_audcap, ISF_OUT(path));

						if (pDest) {
							//bind
							if (isf_audcap.p_base->get_is_allow_push(&isf_audcap, pDest)) {
								ret = isf_audcap.p_base->do_push(&isf_audcap, ISF_OUT(path), &isf_audcap_bsdata, 0);

								if (ret != ISF_OK) {
									//DBG_ERR("push failed %d\r\n", ret);
									p_thisunit->p_base->do_probe(p_thisunit, ISF_OUT(path), ISF_OUT_PROBE_PUSH_WRN, ret);
								} else {
									p_thisunit->p_base->do_probe(p_thisunit, ISF_OUT(path), ISF_OUT_PROBE_PUSH, ISF_OK);
								}
							} else {
								if (g_isf_audcap_trace_on) {
									DBG_WRN("Cannot push\r\n");
								}
								isf_audcap.p_base->do_release(&isf_audcap, ISF_OUT(path), &isf_audcap_bsdata, ISF_OUT_PROBE_PUSH);
								isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(path), ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_NOT_CONNECT_YET);
							}
						} else {
							//not bind
							#if (SUPPORT_PULL_FUNCTION)
							// add to pull queue
							if (_isf_audcap_put_pullque(path, &isf_audcap_bsdata)) {
								isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(path), ISF_OUT_PROBE_PUSH, ISF_OK);
							} else {
								isf_audcap.p_base->do_release(&isf_audcap, ISF_OUT(path), &isf_audcap_bsdata, ISF_OUT_PROBE_PUSH);
								isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(path), ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_QUEUE_FULL);
							}
							#endif
						}
						p_ctx_pi->isf_audcap_output_mem.addr += output_size;
					}
				}
				block_start_addr += block_size;
				if (p_isf_audcap_remain_info[0] != 0) {
					p_isf_audcap_remain_info[0] = 0;
				}
			}
			//update last left address
			isf_audcap_last_left[0].addr = out_start_addr + block_size*frame_count;
			isf_audcap_last_left[0].size = this_size - block_size*frame_count;

			if (isf_audcap_last_left[0].size > 0){
				p_isf_audcap_remain_info[0] = p_aud_bufinfo;
				//DBG_DUMP("a remain = %x\r\n", (UINT32)p_isf_audcap_remain_info[0]);
			}

			if (p_ctx_p0->isf_audcap_lb_en == TRUE && pAudBufQue_AEC->uiAddress != 0 && pAudBufQue_AEC->uiValidSize != 0) {
				isf_audcap_aec_last_left[0].addr = aec_out_start_addr + aec_block_size*frame_count;
				isf_audcap_aec_last_left[0].size = aec_this_size - aec_block_size*frame_count;
			}

		} else {
			AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

			this_size = pAudBufQue->uiValidSize + isf_audcap_last_left[0].size;
			block_size = g_isf_audcap_frame_block_sample * wsis.aud_info.ch_num *(wsis.aud_info.bitpersample>>3);
			frame_count = this_size / block_size;
			out_start_addr = pAudBufQue->uiAddress;

			if (isf_audcap_last_left[0].size != 0) {
				out_start_addr -= isf_audcap_last_left[0].size;
				if (isf_audcap_last_left[0].addr > out_start_addr) {
					memcpy((void*)out_start_addr, (const void*)isf_audcap_last_left[0].addr, isf_audcap_last_left[0].size);
				}

				remain_ts = _isf_audcap_do_div((UINT64)(isf_audcap_last_left[0].size/(wsis.aud_info.bitpersample>>3)/wsis.aud_info.ch_num)*1000000, (UINT64)wsis.aud_info.aud_sr);
			} else {
				remain_ts = 0;
			}

			p_aud_bufinfo = _isf_audcap_get_buf_info();

			if (p_aud_bufinfo == 0) {
				isf_audcap.p_base->do_new(&isf_audcap, ISF_OUT(0), NVTMPP_VB_INVALID_POOL, 0, 0, &isf_audcap_bsdata, ISF_OUT_PROBE_NEW);
				isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL);
				//DBG_ERR("_ISF_AudCap_NEW_BSDATA is NULL!\r\n");
				return ISF_ERR_QUEUE_FULL;
			}

			p_aud_bufinfo->buf_addr = pAudBufQue->uiAddress;

			if (this_size - block_size*frame_count > 0) {
				p_aud_bufinfo->occupy = frame_count + 1;
			} else {
				p_aud_bufinfo->occupy = frame_count;
			}

			block_start_addr = out_start_addr;
			this_ts = timestamp - (isf_audcap_timestamp.frame_time + remain_ts);
			this_aec_ts = timestamp_aec - (isf_audcap_timestamp.frame_time + remain_ts);

			if (p_ctx_p0->isf_audcap_lb_en == TRUE && pAudBufQue_AEC->uiAddress != 0 && pAudBufQue_AEC->uiValidSize != 0) {
				UINT32 lb_ch_num = (p_ctx_p0->isf_audout_ch == AUDIO_CH_STEREO)? 2 : 1;

				aec_this_size = pAudBufQue_AEC->uiValidSize + isf_audcap_aec_last_left[0].size;
				aec_block_size = g_isf_audcap_frame_block_sample * lb_ch_num *(wsis.aud_info.bitpersample>>3);
				aec_out_start_addr = pAudBufQue_AEC->uiAddress;

				if (isf_audcap_aec_last_left[0].size != 0) {
					aec_out_start_addr -= isf_audcap_aec_last_left[0].size;
					if (isf_audcap_aec_last_left[0].addr > aec_out_start_addr) {
						memcpy((void*)aec_out_start_addr, (const void*)isf_audcap_aec_last_left[0].addr, isf_audcap_aec_last_left[0].size);
					}
				}

				aec_block_start_addr = aec_out_start_addr;
			}

			for (i = 0; i < frame_count; i++) {

				p_thisunit->p_base->do_probe(p_thisunit, ISF_OUT(0), ISF_OUT_PROBE_PUSH, ISF_ENTER);

				pAudCapData = _isf_audcap_get_bsdata();

				if (pAudCapData != 0) {
					pAudCapData->hData    = block_start_addr;
					pAudCapData->mem.addr = block_start_addr;
					pAudCapData->mem.size = block_size;
					pAudCapData->refCnt   = 1;
					pAudCapData->p_buf_info = p_aud_bufinfo;

					if (p_isf_audcap_remain_info[0] != 0) {
						pAudCapData->p_remain_buf_info = p_isf_audcap_remain_info[0];
						p_isf_audcap_remain_info[0] = 0;
						//DBG_DUMP("n remain = %x\r\n", (UINT32)pAudCapData->p_remain_buf_info);
					} else {
						pAudCapData->p_remain_buf_info = 0;
					}
					//DBG_DUMP("AudCap lock hData = %x\r\n", pAudCapData->hData);
				} else {
					//new pData by user
					isf_audcap.p_base->do_new(&isf_audcap, ISF_OUT(0), NVTMPP_VB_INVALID_POOL, 0, 0, &isf_audcap_bsdata, ISF_OUT_PROBE_NEW);
					isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL);
					//DBG_ERR("_ISF_AudCap_NEW_BSDATA is NULL!\r\n");
					return ISF_ERR_QUEUE_FULL;
				}

				// add AUDCAP_BSDATA link list
				loc_cpu(flags);
				if (p_ctx_pi->g_p_audcap_bsdata_link_head == NULL) {
					p_ctx_pi->g_p_audcap_bsdata_link_head = pAudCapData;
					p_ctx_pi->g_p_audcap_bsdata_link_tail = p_ctx_pi->g_p_audcap_bsdata_link_head;
				} else {
					p_ctx_pi->g_p_audcap_bsdata_link_tail->pnext_bsdata = pAudCapData;
					p_ctx_pi->g_p_audcap_bsdata_link_tail               = pAudCapData;
				}
				unl_cpu(flags);

				isf_audcap_bsdata.sign = ISF_SIGN_DATA;
				//isf_audcap_bsdata.TimeStamp = timestamp;
				isf_audcap_bsdata.h_data = ((pAudCapData->blk_id + 1) & 0xFF);
				//new pData by user
				isf_audcap.p_base->do_new(&isf_audcap, ISF_OUT(0), NVTMPP_VB_INVALID_POOL, isf_audcap_bsdata.h_data, 0, &isf_audcap_bsdata, ISF_OUT_PROBE_NEW);

				//pAudBuf->sign = MAKEFOURCC('A', 'R', 'A', 'W');
				pAudBuf->addr[0] = pAudCapData->mem.addr;
				pAudBuf->size = pAudCapData->mem.size;
				pAudBuf->sound_mode = wsis.aud_info.ch_num;
				pAudBuf->sample_rate = wsis.aud_info.aud_sr;
				pAudBuf->bit_width = wsis.aud_info.bitpersample;

				this_ts = this_ts + isf_audcap_timestamp.block_time;
				pAudBuf->timestamp = this_ts;

				this_aec_ts = this_aec_ts + isf_audcap_timestamp.block_time;

				//DBG_DUMP("ts = %d %d\r\n", (UINT32)(pAudBuf->timestamp >> 32), (UINT32)(pAudBuf->timestamp & 0xffffffff));
#if (SUPPORT_PULL_FUNCTION)
				// convert to physical addr
				pAudBuf->phyaddr[0] = AUDCAP_VIRT2PHY(0, block_start_addr);

				if (p_ctx_p0->isf_audcap_lb_en == TRUE && pAudBufQue_AEC->uiAddress != 0 && pAudBufQue_AEC->uiValidSize != 0) {
					pAudBuf->reserved[0] = AUDCAP_VIRT2PHY(0, aec_block_start_addr);
					pAudBuf->reserved[1] = aec_block_size;
					pAudBuf->reserved[2] = (UINT32)(this_aec_ts & 0xFFFFFFFF);
					pAudBuf->reserved[3] = (UINT32)(this_aec_ts >> 32);
					aec_block_start_addr += aec_block_size;
				}
#endif
				block_start_addr += pAudCapData->mem.size;

				//do resampling
				if (p_ctx_p0->isf_audcap_resample_en) {
					pAudBuf->size = _isf_audcap_src_proc(resample_handle_rec, pAudBuf->addr[0], pAudBuf->size, p_ctx_p0->isf_audcap_resample_frame * wsis.aud_info.ch_num * (wsis.aud_info.bitpersample>>3));
					pAudBuf->sample_rate = p_ctx_p0->isf_audcap_resample;
				}

				if (pDest) {
					//bind
					if (isf_audcap.p_base->get_is_allow_push(&isf_audcap, pDest)) {
						ret = isf_audcap.p_base->do_push(&isf_audcap, ISF_OUT(0), &isf_audcap_bsdata, 0);

						if (ret != ISF_OK) {
							//DBG_ERR("push failed %d\r\n", ret);
							p_thisunit->p_base->do_probe(p_thisunit, ISF_OUT(0), ISF_OUT_PROBE_PUSH_WRN, ret);
						} else {
							p_thisunit->p_base->do_probe(p_thisunit, ISF_OUT(0), ISF_OUT_PROBE_PUSH, ISF_OK);
						}
					} else {
						if (g_isf_audcap_trace_on) {
							DBG_WRN("Cannot push\r\n");
						}
						isf_audcap.p_base->do_release(&isf_audcap, ISF_OUT(0), &isf_audcap_bsdata, ISF_OUT_PROBE_PUSH);
						isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_NOT_CONNECT_YET);
					}
				} else {
					//not bind
					#if (SUPPORT_PULL_FUNCTION)
					// add to pull queue
					if (_isf_audcap_put_pullque(0, &isf_audcap_bsdata)) {
						isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PUSH, ISF_OK);
					} else {
						isf_audcap.p_base->do_release(&isf_audcap, ISF_OUT(0), &isf_audcap_bsdata, ISF_OUT_PROBE_PUSH);
						isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_QUEUE_FULL);
					}
					#endif
				}

			}
			//update last left address
			isf_audcap_last_left[0].addr = out_start_addr + block_size*frame_count;
			isf_audcap_last_left[0].size = this_size - block_size*frame_count;

			if (isf_audcap_last_left[0].size > 0){
				p_isf_audcap_remain_info[0] = p_aud_bufinfo;
				//DBG_DUMP("a remain = %x\r\n", (UINT32)p_isf_audcap_remain_info[0]);
			}

			if (p_ctx_p0->isf_audcap_lb_en == TRUE && pAudBufQue_AEC->uiAddress != 0 && pAudBufQue_AEC->uiValidSize != 0) {
				isf_audcap_aec_last_left[0].addr = aec_out_start_addr + aec_block_size*frame_count;
				isf_audcap_aec_last_left[0].size = aec_this_size - aec_block_size*frame_count;
			}
		}
	} else {
		//frame size is multiple of 1024

		if (isf_audcap_dual_mono && wsis.aud_info.ch_num > 1) {
			UINT32 output_size, buf_size;

			//do resampling
			if (p_ctx_p0->isf_audcap_resample_en) {
				buf_size = _isf_audcap_src_proc(resample_handle_rec, pAudBufQue->uiAddress, pAudBufQue->uiSize, p_ctx_p0->isf_audcap_resample_frame * wsis.aud_info.ch_num * (wsis.aud_info.bitpersample>>3));
				out_sr = p_ctx_p0->isf_audcap_resample;
			} else {
				buf_size = pAudBufQue->uiValidSize;
			}

			output_size = buf_size / wsis.aud_info.ch_num;

			p_aud_bufinfo = _isf_audcap_get_buf_info();

			if (p_aud_bufinfo == 0) {
				isf_audcap.p_base->do_new(&isf_audcap, ISF_OUT(0), NVTMPP_VB_INVALID_POOL, 0, 0, &isf_audcap_bsdata, ISF_OUT_PROBE_NEW);
				isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL);
				//DBG_ERR("_ISF_AudCap_NEW_BSDATA is NULL!\r\n");
				return ISF_ERR_QUEUE_FULL;
			}

			//for release origin audio buffer
			p_aud_bufinfo->buf_addr = pAudBufQue->uiAddress;
			p_aud_bufinfo->occupy = 0;
			for (path = 0; path < wsis.aud_info.ch_num; path++) {
				AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[path];

				if (p_ctx_pi->isf_audcap_started) {
					p_aud_bufinfo->occupy++;
				}
			}

			for (path = 0; path < wsis.aud_info.ch_num; path++) {
				AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[path];

				if (p_ctx_pi->isf_audcap_started) {

					p_thisunit->p_base->do_probe(p_thisunit, ISF_OUT(path), ISF_OUT_PROBE_PUSH, ISF_ENTER);

					if ((p_ctx_pi->isf_audcap_output_mem.addr + output_size) > (p_ctx_pi->g_isf_audcap_pathmem.addr + p_ctx_pi->g_isf_audcap_pathmem.size)) {
						p_ctx_pi->isf_audcap_output_mem.addr = p_ctx_pi->g_isf_audcap_pathmem.addr;
					}

					_isf_audcap_depack(path, pAudBufQue->uiAddress, p_ctx_pi->isf_audcap_output_mem.addr, &wsis.aud_info, buf_size);

					pAudCapData = _isf_audcap_get_bsdata();

					if (pAudCapData) {
						pAudCapData->hData    = p_ctx_pi->isf_audcap_output_mem.addr;
						pAudCapData->mem.addr = p_ctx_pi->isf_audcap_output_mem.addr;
						pAudCapData->mem.size = output_size;
						pAudCapData->refCnt   = 1;
						pAudCapData->p_buf_info = p_aud_bufinfo;
						//DBG_DUMP("AudCap lock hData = %x\r\n", pAudCapData->hData);
					} else {
						//new pData by user
						isf_audcap.p_base->do_new(&isf_audcap, ISF_OUT(path), NVTMPP_VB_INVALID_POOL, 0, 0, &isf_audcap_bsdata, ISF_OUT_PROBE_NEW);
						isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(path), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL);
						//DBG_ERR("_ISF_AudCap_NEW_BSDATA is NULL!\r\n");
						return ISF_ERR_QUEUE_FULL;
					}

					// add AUDCAP_BSDATA link list
					loc_cpu(flags);
					if (p_ctx_pi->g_p_audcap_bsdata_link_head == NULL) {
						p_ctx_pi->g_p_audcap_bsdata_link_head = pAudCapData;
						p_ctx_pi->g_p_audcap_bsdata_link_tail = p_ctx_pi->g_p_audcap_bsdata_link_head;
					} else {
						p_ctx_pi->g_p_audcap_bsdata_link_tail->pnext_bsdata = pAudCapData;
						p_ctx_pi->g_p_audcap_bsdata_link_tail               = pAudCapData;
					}
					unl_cpu(flags);

					isf_audcap_bsdata.sign = ISF_SIGN_DATA;
					//isf_audcap_bsdata.TimeStamp = timestamp;
					isf_audcap_bsdata.h_data = ((path << 8) & 0xFF00) | ((pAudCapData->blk_id + 1) & 0xFF);
					//new pData by user
					isf_audcap.p_base->do_new(&isf_audcap, ISF_OUT(path), NVTMPP_VB_INVALID_POOL, isf_audcap_bsdata.h_data, 0, &isf_audcap_bsdata, ISF_OUT_PROBE_NEW);

					//pAudBuf->sign = MAKEFOURCC('A', 'R', 'A', 'W');
					pAudBuf->addr[0] = pAudCapData->mem.addr;
					pAudBuf->size= pAudCapData->mem.size;
					pAudBuf->sound_mode = 1;
					pAudBuf->sample_rate = out_sr;
					pAudBuf->bit_width = wsis.aud_info.bitpersample;
					pAudBuf->timestamp = timestamp;
#if (SUPPORT_PULL_FUNCTION)
					// convert to physical addr
					pAudBuf->phyaddr[0] = AUDCAP_VIRT2PHY(0, pAudBuf->addr[0]);

					if (path == 0) {
						pAudBuf->reserved[0] = AUDCAP_VIRT2PHY(0, pAudBufQue_AEC->uiAddress);
						pAudBuf->reserved[1] = pAudBufQue_AEC->uiValidSize;
						pAudBuf->reserved[2] = (UINT32)(timestamp_aec & 0xFFFFFFFF);
						pAudBuf->reserved[3] = (UINT32)(timestamp_aec >> 32);
					}
#endif
					pDest = isf_audcap.p_base->oport(&isf_audcap, ISF_OUT(path));

					if (pDest) {
						//bind
						if (isf_audcap.p_base->get_is_allow_push(&isf_audcap, pDest)) {
							ret = isf_audcap.p_base->do_push(&isf_audcap, ISF_OUT(path), &isf_audcap_bsdata, 0);

							if (ret != ISF_OK) {
								//DBG_ERR("push failed %d\r\n", ret);
								p_thisunit->p_base->do_probe(p_thisunit, ISF_OUT(path), ISF_OUT_PROBE_PUSH_WRN, ret);
							} else {
								p_thisunit->p_base->do_probe(p_thisunit, ISF_OUT(path), ISF_OUT_PROBE_PUSH, ISF_OK);
							}
						} else {
							if (g_isf_audcap_trace_on) {
								DBG_WRN("Cannot push\r\n");
							}
							isf_audcap.p_base->do_release(&isf_audcap, ISF_OUT(path), &isf_audcap_bsdata, ISF_OUT_PROBE_PUSH);
							isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(path), ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_NOT_CONNECT_YET);
						}
					} else {
						//not bind
						#if (SUPPORT_PULL_FUNCTION)
						// add to pull queue
						if (_isf_audcap_put_pullque(path, &isf_audcap_bsdata)) {
							isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(path), ISF_OUT_PROBE_PUSH, ISF_OK);
						} else {
							isf_audcap.p_base->do_release(&isf_audcap, ISF_OUT(path), &isf_audcap_bsdata, ISF_OUT_PROBE_PUSH);
							isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(path), ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_QUEUE_FULL);
						}
						#endif
					}

					p_ctx_pi->isf_audcap_output_mem.addr += output_size;
				}
			}

		} else {
			AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[0];

			p_thisunit->p_base->do_probe(p_thisunit, ISF_OUT(0), ISF_OUT_PROBE_PUSH, ISF_ENTER);

			pAudCapData = _isf_audcap_get_bsdata();

			if (pAudCapData) {
				pAudCapData->hData    = pAudBufQue->uiAddress;
				pAudCapData->mem.addr = pAudBufQue->uiAddress;
				pAudCapData->mem.size = pAudBufQue->uiValidSize;
				pAudCapData->refCnt   = 1;
				//DBG_DUMP("AudCap lock hData = %x\r\n", pAudCapData->hData);
			} else {
				//new pData by user
				isf_audcap.p_base->do_new(&isf_audcap, ISF_OUT(0), NVTMPP_VB_INVALID_POOL, 0, 0, &isf_audcap_bsdata, ISF_OUT_PROBE_NEW);
				isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL);
				//DBG_ERR("_ISF_AudCap_NEW_BSDATA is NULL!\r\n");
				return ISF_ERR_QUEUE_FULL;
			}

			// add AUDCAP_BSDATA link list
			loc_cpu(flags);
			if (p_ctx_pi->g_p_audcap_bsdata_link_head == NULL) {
				p_ctx_pi->g_p_audcap_bsdata_link_head = pAudCapData;
				p_ctx_pi->g_p_audcap_bsdata_link_tail = p_ctx_pi->g_p_audcap_bsdata_link_head;
			} else {
				p_ctx_pi->g_p_audcap_bsdata_link_tail->pnext_bsdata = pAudCapData;
				p_ctx_pi->g_p_audcap_bsdata_link_tail               = pAudCapData;
			}
			unl_cpu(flags);

			isf_audcap_bsdata.sign = ISF_SIGN_DATA;
			//isf_audcap_bsdata.TimeStamp = timestamp;
			isf_audcap_bsdata.h_data = ((pAudCapData->blk_id + 1) & 0xFF);
			//new pData by user
			isf_audcap.p_base->do_new(&isf_audcap, ISF_OUT(0), NVTMPP_VB_INVALID_POOL, isf_audcap_bsdata.h_data, 0, &isf_audcap_bsdata, ISF_OUT_PROBE_NEW);

			//pAudBuf->sign = MAKEFOURCC('A', 'R', 'A', 'W');
			pAudBuf->addr[0] = pAudBufQue->uiAddress;
			pAudBuf->size= pAudBufQue->uiValidSize;
			pAudBuf->sound_mode = wsis.aud_info.ch_num;
			pAudBuf->sample_rate = wsis.aud_info.aud_sr;
			pAudBuf->bit_width = wsis.aud_info.bitpersample;
			pAudBuf->timestamp = timestamp;
#if (SUPPORT_PULL_FUNCTION)
			// convert to physical addr
			pAudBuf->phyaddr[0] = AUDCAP_VIRT2PHY(0, pAudBufQue->uiAddress);

			pAudBuf->reserved[0] = AUDCAP_VIRT2PHY(0, pAudBufQue_AEC->uiAddress);
			pAudBuf->reserved[1] = pAudBufQue_AEC->uiValidSize;
			pAudBuf->reserved[2] = (UINT32)(timestamp_aec & 0xFFFFFFFF);
			pAudBuf->reserved[3] = (UINT32)(timestamp_aec >> 32);
#endif

			//do resampling
			if (p_ctx_p0->isf_audcap_resample_en) {
				pAudBuf->size = _isf_audcap_src_proc(resample_handle_rec, pAudBuf->addr[0], pAudBuf->size, p_ctx_p0->isf_audcap_resample_frame * wsis.aud_info.ch_num * (wsis.aud_info.bitpersample>>3));
				pAudBuf->sample_rate = p_ctx_p0->isf_audcap_resample;
			}

			if (pDest) {
				//bind
				if (isf_audcap.p_base->get_is_allow_push(&isf_audcap, pDest)) {
					ret = isf_audcap.p_base->do_push(&isf_audcap, ISF_OUT(0), &isf_audcap_bsdata, 0);

					if (ret != ISF_OK) {
						//DBG_ERR("push failed %d\r\n", ret);
						isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PUSH_WRN, ret);
					} else {
						isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PUSH, ISF_OK);
					}
				} else {
					if (g_isf_audcap_trace_on) {
						DBG_WRN("Cannot push\r\n");
					}
					isf_audcap.p_base->do_release(&isf_audcap, ISF_OUT(0), &isf_audcap_bsdata, ISF_OUT_PROBE_PUSH);
					isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_NOT_CONNECT_YET);
				}
			} else {
				//not bind
				#if (SUPPORT_PULL_FUNCTION)
				// add to pull queue
				if (_isf_audcap_put_pullque(0, &isf_audcap_bsdata)) {
					isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PUSH, ISF_OK);
				} else {
					isf_audcap.p_base->do_release(&isf_audcap, ISF_OUT(0), &isf_audcap_bsdata, ISF_OUT_PROBE_PUSH);
					isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_QUEUE_FULL);
				}
				#endif
			}
		}
	}

	return TRUE;
}

static void _isf_audcap_evt_cb(WAVSTUD_ACT act, UINT32 p1, UINT32 p2)
{

	switch (p1) {
	case WAVSTUD_CB_EVT_NEW_ENTER: {
			isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_NEW, ISF_ENTER);
			break;
		}
	case WAVSTUD_CB_EVT_NEW_OK: {
			isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_NEW, ISF_OK);
			break;
		}
	case WAVSTUD_CB_EVT_NEW_FAIL: {
			isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_NEW_WRN, ISF_ERR_QUEUE_FULL);
			break;
		}
	case WAVSTUD_CB_EVT_PROC_ENTER: {
			isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PROC, ISF_ENTER);
			break;
		}
	case WAVSTUD_CB_EVT_PROC_OK: {
			isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PROC, ISF_OK);
			break;
		}
	case WAVSTUD_CB_EVT_PROC_FAIL: {
			isf_audcap.p_base->do_probe(&isf_audcap, ISF_OUT(0), ISF_OUT_PROBE_PROC_WRN, ISF_ERR_QUEUE_FULL);
			break;
		}
	default:
		break;
	}
}

static ISF_RV _isf_audcap_lock_cb(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 hData)
{
	UINT32 i = 0;
	AUDCAP_BSDATA *pAudCapData = NULL;
	UINT32 oport = ISF_OUT((hData >> 8) & 0xFF);

	if (p_thisunit == NULL) {
		isf_audcap.p_base->do_probe(&isf_audcap, oport, ISF_USER_PROBE_PULL, ISF_ENTER);
	}

	// search AUDCAP_BSDATA link list by hData
	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];
		AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

		if (p_ctx_c->ctx_mem.ctx_addr == 0) {
			return ISF_ERR_UNINIT;
		}

		SEM_WAIT(p_ctx_p->ISF_AUDCAP_SEM_ID);
		pAudCapData = p_ctx_p->g_p_audcap_bsdata_link_head;

		while (pAudCapData != NULL) {
			if (pAudCapData->blk_id == ((hData - 1) & 0xFF)) {
				pAudCapData->refCnt++;
				if (g_isf_audcap_trace_on) {
					if (p_thisunit) {
						DBG_DUMP("LOCK %s ref %d\r\n", p_thisunit->unit_name, pAudCapData->refCnt);
					}
				}
				isf_audcap.p_base->do_probe(&isf_audcap, oport, ISF_USER_PROBE_PULL, ISF_OK);
				SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_SEM_ID);
				return ISF_OK;
			}
			pAudCapData = pAudCapData->pnext_bsdata;
		}
		SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_SEM_ID);
	}

	return ISF_OK;
}

static ISF_RV _isf_audcap_unlock_cb(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 hData)
{
	UINT32 i = 0;
	AUDCAP_BSDATA *pAudcapData = NULL;
	unsigned long flags;
	UINT32 oport = ISF_OUT((hData >> 8) & 0xFF);

	if (p_thisunit == NULL) {
		isf_audcap.p_base->do_probe(&isf_audcap, oport, ISF_USER_PROBE_REL, ISF_ENTER);
	}

	// search AUDCAP_BSDATA link list by hData
	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];
		AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

		if (p_ctx_c->ctx_mem.ctx_addr == 0) {
			isf_audcap.p_base->do_probe(&isf_audcap, oport, ISF_USER_PROBE_REL, ISF_ERR_UNINIT);
			return ISF_ERR_UNINIT;
		}

		SEM_WAIT(p_ctx_p->ISF_AUDCAP_SEM_ID);

		pAudcapData = p_ctx_p->g_p_audcap_bsdata_link_head;

		while (pAudcapData != NULL) {
			if (pAudcapData->blk_id == ((hData - 1) & 0xFF)) {

				if (pAudcapData->refCnt > 0) {
					pAudcapData->refCnt--;
				} else {
					DBG_ERR("UNLOCK ref %d\r\n", pAudcapData->refCnt);
					SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_SEM_ID);
					return ISF_ERR_INVALID_VALUE;
				}

				isf_audcap.p_base->do_probe(&isf_audcap, oport, ISF_USER_PROBE_REL, ISF_OK);

				if (g_isf_audcap_trace_on) {
					if (p_thisunit) {
						//DBG_DUMP("UNLOCK %s ref %d\r\n", p_thisunit->unit_name, pAudcapData->refCnt);
					}
				}

				if (pAudcapData->refCnt == 0) {
					if (g_isf_audcap_trace_on) {
						DBG_DUMP("Free BS Data hdata = 0x%x, blk_id = %d, addr = %x, size = %d\r\n", pAudcapData->hData, pAudcapData->blk_id, pAudcapData->mem.addr, pAudcapData->mem.size);
					}

					if (pAudcapData->p_buf_info != 0) {
						//free remain buffer
						if (pAudcapData->p_remain_buf_info != 0) {
							loc_cpu(flags);
							pAudcapData->p_remain_buf_info->occupy--;
							unl_cpu(flags);
							if (pAudcapData->p_remain_buf_info->occupy == 0) {
								// Set BS data address is unlock to WavStudio
								if (g_isf_audcap_trace_on) {
									DBG_DUMP("rel remain_buf_info hdata = 0x%x, blk_id = %d, addr = %x\r\n", pAudcapData->hData, pAudcapData->blk_id, pAudcapData->p_remain_buf_info->buf_addr);
								}
								wavstudio_set_config(WAVSTUD_CFG_UNLOCK_ADDR, WAVSTUD_ACT_REC, pAudcapData->p_remain_buf_info->buf_addr);
								_isf_audcap_free_buf_info(pAudcapData->p_remain_buf_info);
							}
						}
						loc_cpu(flags);
						pAudcapData->p_buf_info->occupy--;
						unl_cpu(flags);
						if (pAudcapData->p_buf_info->occupy == 0) {
							// Set BS data address is unlock to WavStudio
							if (g_isf_audcap_trace_on) {
								DBG_DUMP("rel buf_info hdata = 0x%x, blk_id = %d, addr = %x\r\n", pAudcapData->hData, pAudcapData->blk_id, pAudcapData->p_buf_info->buf_addr);
							}
							wavstudio_set_config(WAVSTUD_CFG_UNLOCK_ADDR, WAVSTUD_ACT_REC, pAudcapData->p_buf_info->buf_addr);
							_isf_audcap_free_buf_info(pAudcapData->p_buf_info);
						}
					} else {
						// Set BS data address is unlock to WavStudio
						wavstudio_set_config(WAVSTUD_CFG_UNLOCK_ADDR, WAVSTUD_ACT_REC, pAudcapData->hData);
						if (g_isf_audcap_trace_on) {
							DBG_DUMP("rel  hdata = 0x%x, blk_id = %d\r\n", pAudcapData->hData, pAudcapData->blk_id);
						}
					}

					if (pAudcapData != p_ctx_p->g_p_audcap_bsdata_link_head) {
						AUDCAP_BSDATA *pAudcapData2 = NULL;

						if (g_isf_audcap_trace_on) {
							DBG_WRN("Not first data in queue!! hData = %x, head = %x\r\n", pAudcapData->hData, p_ctx_p->g_p_audcap_bsdata_link_head->hData);
						}

						// search for release
						pAudcapData2 = p_ctx_p->g_p_audcap_bsdata_link_head;

						while (pAudcapData2 != NULL) {
							if (pAudcapData2->pnext_bsdata == pAudcapData) {
								pAudcapData2->pnext_bsdata = pAudcapData->pnext_bsdata;
								loc_cpu(flags);
								if (pAudcapData2->pnext_bsdata == NULL) {
									p_ctx_p->g_p_audcap_bsdata_link_tail = pAudcapData2;
								}
								unl_cpu(flags);
								break;
							}

							pAudcapData2 = pAudcapData2->pnext_bsdata;
						}

						// free block
						_isf_audcap_free_bsdata(pAudcapData);
					} else {

						// update link head
						loc_cpu(flags);
						if (p_ctx_p->g_p_audcap_bsdata_link_head != NULL) {
							p_ctx_p->g_p_audcap_bsdata_link_head = p_ctx_p->g_p_audcap_bsdata_link_head->pnext_bsdata;
						}
						unl_cpu(flags);
						// free block
						_isf_audcap_free_bsdata(pAudcapData);

					}
				}

				SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_SEM_ID);
				return ISF_OK;
			}

			pAudcapData = pAudcapData->pnext_bsdata;
		}
		SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_SEM_ID);
	}

	return ISF_OK;
}

static ISF_RV _isf_audcap_bs_data_init(void)
{
	_isf_audcap_base.this = 0;
	isf_audcap.p_base->init_data(&isf_audcap_bsdata, &_isf_audcap_base, _isf_audcap_base.base, 0x00010000); // [isf_flow] use this ...
	_isf_audcap_base.this = MAKEFOURCC('A','C','A','P'),
	isf_data_regclass(&_isf_audcap_base);

	return ISF_OK;
}

static void _isf_audcap_bs_queue_init(void)
{
	UINT32 mpp_size = 0;
	UINT32 i = 0;
	AUDCAP_BSDATA *pAudCapData = NULL;

	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDCAP_CONTEXT_PORT *p_ctx_p = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];

		// release left blocks
		SEM_WAIT(p_ctx_p->ISF_AUDCAP_SEM_ID);
		pAudCapData = p_ctx_p->g_p_audcap_bsdata_link_head;
		while (pAudCapData != NULL) {
			_isf_audcap_free_bsdata(pAudCapData);
			pAudCapData = pAudCapData->pnext_bsdata;
		}
		p_ctx_p->g_p_audcap_bsdata_link_head = NULL;
		p_ctx_p->g_p_audcap_bsdata_link_tail = NULL;
		SEM_SIGNAL(p_ctx_p->ISF_AUDCAP_SEM_ID);

		_isf_audcap_release_all_pullque(i);
	}

	mpp_size = (sizeof(UINT32) + sizeof(AUDCAP_BSDATA)) * PATH_MAX_COUNT * ISF_AUDCAP_BSDATA_BLK_MAX;
	memset((void *)g_p_audcap_bsdata_blk_maptlb, 0, mpp_size);

	mpp_size = (sizeof(UINT32) + sizeof(AUDCAP_BUF_INFO)) * PATH_MAX_COUNT * ISF_AUDCAP_BSDATA_BLK_MAX;
	memset((void *)g_p_audcap_bufinfo_blk_maptlb, 0, mpp_size);

	g_audcap_bsdata_num = 0;

	isf_audcap_last_left[0].addr = 0;
	isf_audcap_last_left[0].size = 0;

	isf_audcap_aec_last_left[0].addr = 0;
	isf_audcap_aec_last_left[0].size = 0;

	return;
}


//----------------------------------------------------------------------------

static UINT32 _isf_audcap_query_context_size(void)
{
	return (PATH_MAX_COUNT * sizeof(AUDCAP_CONTEXT_PORT));
}

static UINT32 _isf_audcap_query_context_pull_queue_size(void)
{
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

	return (PATH_MAX_COUNT * (p_ctx_c->ctx_mem.pull_queue_max * sizeof(ISF_DATA)));
}

static UINT32 _isf_audcap_query_context_memory(void)
{
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

	p_ctx_c->ctx_mem.pull_queue_max = ISF_AUDCAP_PULL_Q_MAX;

#ifdef __KERNEL__

	if (kdrv_builtin_is_fastboot()) {
		UINT32 builtin_init = 0;
		audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_INIT_DONE, &builtin_init);

		if (builtin_init) {
			AUDCAP_BUILTIN_INIT_INFO init_info = {0};

			audcap_builtin_get_param(BUILTIN_AUDCAP_PARAM_INFO, (UINT32*)&init_info);

			if (init_info.aud_que_cnt > ISF_AUDCAP_PULL_Q_MAX) {
				p_ctx_c->ctx_mem.pull_queue_max = init_info.aud_que_cnt;
			}
		}
	}
#endif

	p_ctx_c->ctx_mem.isf_audcap_size = _isf_audcap_query_context_size();
	p_ctx_c->ctx_mem.pull_queue_size = _isf_audcap_query_context_pull_queue_size();
	p_ctx_c->ctx_mem.wavstudio_size = 0;

	return (p_ctx_c->ctx_mem.isf_audcap_size) + (p_ctx_c->ctx_mem.wavstudio_size) + (p_ctx_c->ctx_mem.pull_queue_size);
}

//----------------------------------------------------------------------------

static void _isf_audcap_assign_context(UINT32 addr)
{
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;
	UINT32 i;
	UINT32 one_pull_queue_size = (p_ctx_c->ctx_mem.pull_queue_size/PATH_MAX_COUNT);

	g_ac_ctx.port = (AUDCAP_CONTEXT_PORT*)addr;
	memset((void *)g_ac_ctx.port, 0, p_ctx_c->ctx_mem.isf_audcap_size);

	addr += p_ctx_c->ctx_mem.isf_audcap_size;

	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];

		p_ctx_pi->isf_audcap_pull_que.Queue = (ISF_DATA*)addr;
		memset((void *)p_ctx_pi->isf_audcap_pull_que.Queue, 0, one_pull_queue_size);

		addr += one_pull_queue_size;
	}
}

static ER _isf_audcap_assign_context_memory_range(UINT32 addr)
{
	#if _TODO
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;
	#endif

	// [ isf_audcap ]
	_isf_audcap_assign_context(addr);
	#if _TODO
	addr += p_ctx_c->ctx_mem.isf_audcap_size;

	// [ wavstudio ]
	addr += p_ctx_c->ctx_mem.wavstudio_size;
	#endif

	return E_OK;
}

//----------------------------------------------------------------------------

static void _isf_audcap_free_context(void)
{
	g_ac_ctx.port = NULL;
}

static ER _isf_audcap_free_context_memory_range(void)
{
	// [ isf_audcap ]
	_isf_audcap_free_context();

	return E_OK;
}

//----------------------------------------------------------------------------

static void _isf_audcap_init_context(void)
{
	UINT32 i;

	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDCAP_CONTEXT_PORT *p_ctx_pi = (AUDCAP_CONTEXT_PORT *)&g_ac_ctx.port[i];

		p_ctx_pi->isf_audcap_defset = KDRV_AUDIO_CAP_DEFSET_20DB;
		p_ctx_pi->volume = 100;
	}
}

//----------------------------------------------------------------------------

static ISF_RV _isf_audcap_do_init(UINT32 h_isf, UINT32 path_max_count)
{
	ISF_RV rv = ISF_OK;
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

	if (h_isf > 0) { //client process
		if (!g_audcap_init[0]) { //is master process already init?
			rv = ISF_ERR_INIT; //not allow init from client process!
			goto _AC_INIT_ERR;
		}
		g_audcap_init[h_isf] = 1; //set init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_audcap_init[i]) { //client process still alive?
				rv = ISF_ERR_INIT; //not allow init from main process!
				goto _AC_INIT_ERR;
			}
		}
	}

	if (path_max_count > 0) {
		//update count
		PATH_MAX_COUNT = path_max_count;
		if (PATH_MAX_COUNT > ISF_AUDCAP_OUT_NUM) {
			DBG_WRN("path max count %d > max %d!\r\n", PATH_MAX_COUNT, ISF_AUDCAP_OUT_NUM);
			PATH_MAX_COUNT = ISF_AUDCAP_OUT_NUM;
		}
	}

	if (p_ctx_c->ctx_mem.ctx_addr != 0) {
		DBG_ERR("[ISF_AUDCAP] already init !!\r\n");
		rv = ISF_ERR_INIT;
		goto _AC_INIT_ERR;
	}

	{
		UINT32 buf_addr = 0;
		UINT32 buf_size = _isf_audcap_query_context_memory();

		//alloc context of all port
		nvtmpp_vb_add_module(isf_audcap.unit_module);
		buf_addr = (&isf_audcap)->p_base->do_new_i(&isf_audcap, NULL, "ctx", buf_size, &(p_ctx_c->ctx_mem.ctx_memblk));

		if (buf_addr == 0) {
			DBG_ERR("[ISF_AUDCAP] alloc context memory fail !!\r\n");
			rv = ISF_ERR_HEAP;
			goto _AC_INIT_ERR;
		}
		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_audcap, ISF_CTRL, "(alloc context memory) (%u, 0x%08x, %u)", PATH_MAX_COUNT, buf_addr, buf_size);

		p_ctx_c->ctx_mem.ctx_addr = buf_addr;

		// assign memory range
		_isf_audcap_assign_context_memory_range(p_ctx_c->ctx_mem.ctx_addr);
		_isf_audcap_init_context();

		//install each device's kernel object
		isf_audcap_install_id();
		g_audcap_init[h_isf] = 1; //set init

		return ISF_OK;
	}

_AC_INIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("init fail! %d\r\n", rv);
	}
	return rv;
}

static ISF_RV _isf_audcap_do_uninit(UINT32 h_isf)
{
	ISF_RV rv = ISF_OK;
	AUDCAP_CONTEXT_COMMON *p_ctx_c = (AUDCAP_CONTEXT_COMMON *)&g_ac_ctx.comm;

	if (h_isf > 0) { //client process
		if (!g_audcap_init[0]) { //is master process already init?
			rv = ISF_ERR_UNINIT; //not allow uninit from client process!
			goto _AC_UNINIT_ERR;
		}
		g_audcap_init[h_isf] = 0; //clear init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_audcap_init[i]) { //client process still alive?
				rv = ISF_ERR_UNINIT; //not allow uninit from main process!
				goto _AC_UNINIT_ERR;
			}
		}
	}

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		return ISF_ERR_UNINIT;
	}

	if (PATH_MAX_COUNT == 0) {
		return ISF_ERR_UNINIT;
	}

	if (isf_audcap_path_open_count > 0) {
		DBG_ERR("Not all port closed\r\n");
		return ISF_ERR_ALREADY_OPEN;
	}

	{
		UINT32 i;

		//reset all units of this module
		DBG_IND("unit(%s) => clear state\r\n", (&isf_audcap)->unit_name);   // equal to call UpdatePort - STOP
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_audcap)->p_base->do_clear_state(&isf_audcap, oport);
		}

		DBG_IND("unit(%s) => clear bind\r\n", (&isf_audcap)->unit_name);    // equal to call UpdatePort - CLOSE
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_audcap)->p_base->do_clear_bind(&isf_audcap, oport);
		}

		DBG_IND("unit(%s) => clear context\r\n", (&isf_audcap)->unit_name); // equal to call UpdatePort - Reset
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_audcap)->p_base->do_clear_ctx(&isf_audcap, oport);
		}


		//uninstall each device's kernel object
		isf_audcap_uninstall_id();

		//free context of all port
		if (p_ctx_c->ctx_mem.ctx_memblk.h_data != 0) {
			rv = (&isf_audcap)->p_base->do_release_i(&isf_audcap, &(p_ctx_c->ctx_mem.ctx_memblk), ISF_OK);

			if (rv == ISF_OK) {
				memset((void*)&(p_ctx_c->ctx_mem), 0, sizeof(AUDCAP_CTX_MEM));  // reset ctx_mem to 0
			} else {
				DBG_ERR("[ISF_AUDCAP] free context memory fail !!\r\n");
				goto _AC_UNINIT_ERR;
			}
		} else {
			DBG_ERR("[ISF_AUDCAP] context h_data == NULL !!\r\n");
			rv = ISF_ERR_INVALID_DATA_ID;
			goto _AC_UNINIT_ERR;
		}

		// reset NULL to all pointer
		_isf_audcap_free_context_memory_range();
		g_audcap_init[h_isf] = 0; //clear init

		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_audcap, ISF_CTRL, "(release context memory)");
		return ISF_OK;
	}

_AC_UNINIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("uninit fail! %d\r\n", rv);
	}
	return rv;
}

ISF_RV _isf_audcap_do_command(UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2)
{
	UINT32 h_isf = (cmd >> 28); //extract h_isf
	//DBG_IND("(do_command) cmd = %u, p0 = 0x%08x, p1 = %u, p2 = %u\r\n", cmd, p0, p1, p2);

	cmd &= ~0xf0000000; //clear h_isf
	switch(cmd) {
	case 1: //INIT
		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_audcap, ISF_CTRL, "(do_init) max_path = %u", p1);
		return _isf_audcap_do_init(h_isf, p1); //p1: max path count
	case 0: //UNINIT
		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_audcap, ISF_CTRL, "(do_uninit)");
		return _isf_audcap_do_uninit(h_isf);
	default:
		DBG_ERR("unsupport cmd(%u) !!\r\n", cmd);
		break;
	}
	return ISF_ERR_NOT_SUPPORT;
}

void isf_audcap_get_audinfo(PWAVSTUD_INFO_SET info)
{
	memcpy((void*)info, (const void*)&wsis, sizeof(WAVSTUD_INFO_SET));
}

UINT32 isf_audcap_get_vol(UINT32 path)
{
	return g_volume;
}

UINT32 isf_audcap_get_recsrc(void)
{
	return isf_audcap_rec_src;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


ISF_PORT_PATH isf_audcap_pathlist[ISF_AUDCAP_OUT_NUM] = {
	{&isf_audcap, ISF_IN(0), ISF_OUT(0)},
	{&isf_audcap, ISF_IN(0), ISF_OUT(1)},
#if (ISF_AUDCAP_OUT_NUM == 8)
	{&isf_audcap, ISF_IN(0), ISF_OUT(2)},
	{&isf_audcap, ISF_IN(0), ISF_OUT(3)},
	{&isf_audcap, ISF_IN(0), ISF_OUT(4)},
	{&isf_audcap, ISF_IN(0), ISF_OUT(5)},
	{&isf_audcap, ISF_IN(0), ISF_OUT(6)},
	{&isf_audcap, ISF_IN(0), ISF_OUT(7)}
#endif
};

ISF_UNIT isf_audcap = {
	.sign = ISF_SIGN_UNIT,
	.unit_name = "audcap",
	.unit_module = MAKE_NVTMPP_MODULE('a', 'u', 'd', 'c', 'a', 'p', 0, 0),
	.list_id = 1, // 1=not support Multi-Process
	.port_in_count = 1,
	.port_out_count = ISF_AUDCAP_OUT_NUM,
	.port_path_count = ISF_AUDCAP_OUT_NUM,
	.port_in = isf_audcap_inputportlist,
	.port_out = isf_audcap_outputportlist,
	.port_outcfg = isf_audcap_outputportlist_cfg,
	.port_incaps = isf_audcap_inputportlist_caps,
	.port_outcaps = isf_audcap_outputportlist_caps,
	.port_outstate = isf_audcap_outputportlist_state,
	.port_ininfo = isf_audcap_outputinfolist_in,
	.port_outinfo = isf_audcap_outputinfolist_out,
	.port_path = isf_audcap_pathlist,
	.do_bindinput = NULL,
	.do_bindoutput = _isf_audcap_bindouput,
	.do_setportparam = _isf_audcap_setportparam,
	.do_getportparam = _isf_audcap_getportparam,
	.do_setportstruct = _isf_audcap_setportstruct,
	.do_getportstruct = _isf_audcap_getportstruct,
	.do_update = _isf_audcap_updateport,
	.do_command = _isf_audcap_do_command,
};


///////////////////////////////////////////////////////////////////////////////


