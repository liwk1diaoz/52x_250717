#ifdef __KERNEL__
#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/platform.h"
#include "kwrap/stdio.h"
#include "kwrap/util.h"
//#include "kwrap/sxcmd.h"
#include "sxcmd_wrapper.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_common/nvtmpp.h"
#include "audio_common/include/Audio.h"
#include "kflow_audioenc/isf_audenc.h"
#include "audio_encode.h"
#include "audio_codec_pcm.h"
#include "audio_codec_ppcm.h"
#include "audio_codec_aac.h"
#include "audio_codec_alaw.h"
#include "audio_codec_ulaw.h"
#include "nmediarec_audenc.h"
#include "nmediarec_api.h"
#include "isf_audenc_dbg.h"
#else
#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/platform.h"
#include "kwrap/stdio.h"
#include "kwrap/util.h"
#include "kwrap/cmdsys.h"
//#include "kwrap/sxcmd.h"
#include "sxcmd_wrapper.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_common/nvtmpp.h"
#include "audio_common/include/Audio.h"
#include "kflow_audioenc/isf_audenc.h"
#include "audio_encode.h"
#include "audio_codec_pcm.h"
#include "audio_codec_ppcm.h"
#include "audio_codec_aac.h"
#include "audio_codec_alaw.h"
#include "audio_codec_ulaw.h"
#include "nmediarec_audenc.h"
#include "nmediarec_api.h"
#include "isf_audenc_dbg.h"

extern BOOL _isf_audenc_api_info(CHAR *strCmd);

#define  msecs_to_jiffies(x)	0
#endif

#if _TODO
// Perf_GetCurrent         : use do_gettimeofday() to implement
// nvtmpp_vb_block2addr    : use nvtmpp_vb_blk2va() ??
// UpdatePortInfo          : use hdal new method
// ISF_UNIT_TRACE          : define NONE first ( isf_flow doesn't EXPORT_SYMBOL this , also, if it indeed EXPORT_SYMBOL, it says disagree with version )
#else
// perf
#define Perf_GetCurrent(args...)            0

#define nvtmpp_vb_block2addr(args...)       nvtmpp_vb_blk2va(args)

#define TIMESTAMP_TODO                      0
#define AUDFMT_TODO                         0

#define ISF_UNIT_TRACE(opclass, p_thisunit, port, fmtstr, args...) (&isf_audenc)->p_base->do_trace(p_thisunit, port, opclass, fmtstr, ##args)
#endif

#if defined (DEBUG) && defined(__FREERTOS)
unsigned int isf_audenc_debug_level = THIS_DBGLVL;
#endif

// fake get_tid() function
static unsigned int get_tid ( signed int *p_tskid )
{
    *p_tskid = -99;
    return 0;
}

#define ISF_AUDENC_IN_NUM           4
#define ISF_AUDENC_OUT_NUM          4
#define AUDENC_MAX_PATH_NUM         4
#define ISF_AUDENC_DATA_QUE_NUM     8
#define ISF_AUDENC_BSDATA_BLK_MAX   512

#define ISF_AUDENC_PULL_Q_MAX       60

#define ISF_AUDENC_MSG_RAW  		((UINT32)0x00000001 << 0)   //0x00000001
#define ISF_AUDENC_MSG_LOCK 		((UINT32)0x00000001 << 1)   //0x00000002
#define ISF_AUDENC_MSG_DROP         ((UINT32)0x00000001 << 2)   //0x00000004
#define ISF_AUDENC_MSG_PULLQ        ((UINT32)0x00000001 << 3)   //0x00000008

typedef struct {
	UINT32           refCnt;
	UINT32           h_data;
	NMI_AUDENC_MEM_RANGE mem;
	UINT32           blk_id;
	void            *pnext_bsdata;
} AUDENC_BSDATA;

typedef struct _AUDENC_PULL_QUEUE {
	UINT32          Front;                  ///< Front pointer
	UINT32          Rear;                   ///< Rear pointer
	UINT32          bFull;                  ///< Full flag
	ISF_DATA        Queue[ISF_AUDENC_PULL_Q_MAX];
} AUDENC_PULL_QUEUE, *PAUDENC_PULL_QUEUE;

typedef struct _AUDENC_MMAP_MEM_INFO {
	UINT32          addr_virtual;           ///< memory start addr (virtual)
	UINT32          addr_physical;          ///< memory start addr (physical)
	UINT32          size;                   ///< size
} AUDENC_MMAP_MEM_INFO, *PAUDENC_MMAP_MEM_INFO;


#define DUMP_TO_FILE_TEST                   DISABLE
#define DUMP_TO_FILE_TEST_KERNEL_VER        DISABLE         // pure linux version
#define SKIP_PCM_COPY                       DISABLE
#define SUPPORT_PULL_FUNCTION               ENABLE

#if defined(__KERNEL__)
#if DUMP_TO_FILE_TEST_KERNEL_VER
#include <kwrap/file.h>
#endif
#endif

// debug
static void 					_ISF_AudEnc_InstallCmd(void);
static UINT32    		  		gISF_AudEncMsgLevel[ISF_AUDENC_OUT_NUM]             = {0};
#if (SUPPORT_PULL_FUNCTION)
static UINT32                   gISF_AudEncDropCount[ISF_AUDENC_IN_NUM]             = {0};
#endif

static ISF_RV _ISF_AudEnc_LockCB(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data);
static ISF_RV _ISF_AudEnc_UnLockCB(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data);

NMI_UNIT                       *pNMI_AudEnc                                         = NULL;
static UINT32                   gISF_AudEncOutputCompressCount                      = 0;
static UINT32                   gISF_AudEncOutputUncompressCount                    = 0;
// BS DATA
static UINT32					gISF_AudEnc_Open_Count                              = 0;
static ISF_DATA           		gISF_AudEnc_BsData_MemBlk       					= {0};
static AUDENC_BSDATA           *g_p_audenc_bsdata_blk_pool                          = NULL;
static UINT32                  *g_p_audenc_bsdata_blk_maptlb                        = NULL;

static UINT32                   gISF_AudCodecFmt                                    = AUDENC_ENCODER_AAC;     /// default = aac
static UINT32                   gISF_AudSampleRate                                  = AUDIO_SR_48000;         /// default = 48K
static UINT32                   gISF_AudChannel                                     = 2;                      /// default = stereo
static UINT32                   gISF_ADTSHeaderEnable                               = FALSE;
static UINT32                   gISF_FileType                                       = NMEDIAREC_MP4;
static UINT32                   gISF_RecFormat                                      = MEDIAREC_AUD_VID_BOTH;
static BOOL                     isf_audenc_init                                     = FALSE;
static UINT32                   isf_reserved_size                                   = 0;
static UINT32                   isf_aac_ver                                         = 0;
static UINT32                   isf_audenc_data_queue_num                           = ISF_AUDENC_DATA_QUE_NUM;
static UINT32                   isf_target_rate                                     = 0;

SEM_HANDLE                      ISF_AUDENC_COMMON_SEM_ID							= {0};
SEM_HANDLE                      ISF_AUDENC_PROC_SEM_ID                              = {0};

typedef struct _AUDENC_CTX_MEM {
	ISF_DATA        ctx_memblk;
	UINT32          ctx_addr;
	UINT32          isf_audenc_size;
	UINT32          nm_audenc_size;
	UINT32          mp_obj_size;
} AUDENC_CTX_MEM;

typedef struct _AUDENC_CONTEXT_COMMON {
	// CONTEXT
	AUDENC_CTX_MEM         ctx_mem;   // alloc context memory for isf_audenc + nmedia
} AUDENC_CONTEXT_COMMON;

typedef struct _AUDENC_CONTEXT_PORT {
	UINT32                   gISF_AudEncMaxMemSize;
	NMI_AUDENC_MEM_RANGE     gISF_AudEncMaxMem;
	ISF_DATA                 gISF_AudEncMaxMemBlk;
	ISF_DATA                 gISF_AudEncMemBlk;
	AUD_BITSTREAM            gISF_AStrmBuf;
	UINT32                   gISF_AudEncPortOpenStatus;
	UINT32                   gISF_AudEncPortRunStatus;
	UINT32                   gISF_OutputFmt;
	AUDENC_PULL_QUEUE        gISF_AudEnc_PullQueue;
	AUDENC_MMAP_MEM_INFO     gISF_AudEncMem_MmapInfo;
	ISF_DATA           		 gISF_AudEnc_RawData_MemBlk;
	ISF_DATA                *gISF_AudEnc_RawDATA;
	ISF_DATA           		 gISF_AudEnc_RawDataUsed_MemBlk;
	BOOL                    *gISF_AudEnc_IsRawUsed;
	AUDENC_BSDATA           *g_p_audenc_bsdata_link_head;
	AUDENC_BSDATA           *g_p_audenc_bsdata_link_tail;
	UINT32                   gISF_AudEnc_Started;
	BOOL                     gISF_AudEnc_Opened;
	BOOL                     isf_audenc_param_dirty;
	BOOL                     gISF_AudEnc_Is_AllocMem;
	SEM_HANDLE               ISF_AUDENC_SEM_ID;
	SEM_HANDLE               ISF_AUDENC_PULLQ_SEM_ID;
	BOOL                     put_pullq;
} AUDENC_CONTEXT_PORT;

typedef struct _AUDENC_CONTEXT {
	AUDENC_CONTEXT_COMMON  comm;    // common variable for all port
	AUDENC_CONTEXT_PORT   *port;    // individual variable for each port
} AUDENC_CONTEXT;

static ISF_DATA gISF_AudEncBsData;
static ISF_RV _isf_audenc_bs_data_init(void);
static ISF_DATA_CLASS _isf_audenc_base;

AUDENC_CONTEXT g_ae_ctx = {0};

// multi-process
static UINT32 g_audenc_init[ISF_FLOW_MAX] = {0};


#if (DUMP_TO_FILE_TEST == ENABLE)
static FST_FILE fp_test_file = NULL;
static UINT32 dump_sample_cnt = 300;            // record 5 seconds (30 samples/sec)
static UINT32 write_size = 0;
#endif

#if (DUMP_TO_FILE_TEST_KERNEL_VER == ENABLE)
static int g_wo_fd = 0;
static INT32 dump_sample_cnt = 300;            // record 5 seconds (30 samples/sec)
static UINT8 temp_buf[6000000] = {0};
static UINT32 temp_cnt = 0;
#endif

#if (SUPPORT_PULL_FUNCTION)
#define AUDENC_VIRT2PHY(pathID, virt_addr) g_ae_ctx.port[pathID].gISF_AudEncMem_MmapInfo.addr_physical + (virt_addr - g_ae_ctx.port[pathID].gISF_AudEncMem_MmapInfo.addr_virtual)

BOOL _ISF_AudEnc_isEmpty_PullQueue(UINT32 pathID);
BOOL _ISF_AudEnc_isFull_PullQueue(UINT32 pathID);
void _ISF_AudEnc_Unlock_PullQueue(UINT32 pathID);
BOOL _ISF_AudEnc_Put_PullQueue(UINT32 pathID, ISF_DATA *data_in);
ISF_RV _ISF_AudEnc_Get_PullQueue(UINT32 pathID, ISF_DATA *data_out, INT32 wait_ms);
BOOL _ISF_AudEnc_RelaseAll_PullQueue(UINT32 pathID);
static ISF_RV _ISF_AudEnc_Close(ISF_UNIT *pThisUnit, UINT32 id, UINT32 b_do_release_i);
#endif

static AUDENC_BSDATA *_ISF_AudEnc_GET_BSDATA(void)
{
	UINT32 i = 0;
	AUDENC_BSDATA *pData = NULL;

	SEM_WAIT(ISF_AUDENC_COMMON_SEM_ID);
	while (g_p_audenc_bsdata_blk_maptlb[i]) {
		i++;
		if (i >= (PATH_MAX_COUNT * ISF_AUDENC_BSDATA_BLK_MAX)) {
			//DBG_ERR("No free block!!!\r\n");
			SEM_SIGNAL(ISF_AUDENC_COMMON_SEM_ID);
			return NULL;
		}
	}

	pData = (AUDENC_BSDATA *)(g_p_audenc_bsdata_blk_pool + i);

	if (pData) {
		pData->pnext_bsdata = NULL;
		pData->blk_id = i;
		g_p_audenc_bsdata_blk_maptlb[i] = TRUE;
	}
	SEM_SIGNAL(ISF_AUDENC_COMMON_SEM_ID);
	return pData;
}

static void _ISF_AudEnc_FREE_BSDATA(AUDENC_BSDATA *pData)
{
	if (pData) {
		SEM_WAIT(ISF_AUDENC_COMMON_SEM_ID);
		g_p_audenc_bsdata_blk_maptlb[pData->blk_id] = FALSE;
		pData->h_data = 0;
		SEM_SIGNAL(ISF_AUDENC_COMMON_SEM_ID);
	}
}

static UINT32 _ISF_AudEnc_StreamReadyCB(NMI_AUDENC_BS_INFO *pMem)
{
	AUDENC_CONTEXT_PORT *p_ctx_p                = NULL;
	ISF_DATA             *pBsData               = &gISF_AudEncBsData;
	PAUD_BITSTREAM        pAStrmBuf             = NULL;
	ISF_DATA             *pRawData              = NULL;
	AUDENC_BSDATA        *pAudEncData           = NULL;
	ISF_RV                rv                    = ISF_OK;

#if (DUMP_TO_FILE_TEST == ENABLE)
	if (fp_test_file && dump_sample_cnt) {
		DBG_DUMP("=====>>> Write file\r\n");
		write_size = pMem->Size;
		FileSys_WriteFile(fp_test_file, (UINT8 *)pMem->Addr, &write_size, FALSE, NULL);
		dump_sample_cnt --;
	} else if (fp_test_file) {
		DBG_DUMP("=====>>> Dump close file\r\n");
		FileSys_CloseFile(fp_test_file);
		fp_test_file = NULL;
	}
#endif

#if (DUMP_TO_FILE_TEST_KERNEL_VER == ENABLE)
	if (!((VOS_FILE)(-1) == g_wo_fd) && dump_sample_cnt > 0 && temp_cnt < 6000000) {
		// WRITE
		DBG_DUMP("=====>>> mem copy, cnt=%d\r\n", dump_sample_cnt);
		if (!((VOS_FILE)(-1) == g_wo_fd)) {
			memcpy((void*)(temp_buf+temp_cnt), (const void*) pMem->Addr, pMem->Size);
			temp_cnt += pMem->Size;
		}
		dump_sample_cnt--;
	} else if (!((VOS_FILE)(-1) == g_wo_fd) && dump_sample_cnt == 0) {
		// CLOSE
		if (!((VOS_FILE)(-1) == g_wo_fd)) {
			int len = 0;
			DBG_DUMP("=====>>> write file, size=%d\r\n", temp_cnt);
			len = vos_file_write(g_wo_fd, (void *)temp_buf, temp_cnt);
			vos_file_close(g_wo_fd);
		}
		DBG_DUMP("=====>>> write file finished!!!!!\r\n");
		dump_sample_cnt = -1;
	}
#endif

	if (pMem->PathID >= PATH_MAX_COUNT) {
		return ISF_ERR_INVALID_PORT_ID;
	}

	p_ctx_p   = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[pMem->PathID];
	pAStrmBuf = &(p_ctx_p->gISF_AStrmBuf);

	// release lock RAW data
	if (pMem->Occupied == 0) {
		//ISF_PORT* pSrcPort = _isf_unit_in(&isf_audenc, ISF_IN(0)); //ISF_PORT *pSrcPort = ImageUnit_In(&isf_audenc, ISF_IN1);
		//ISF_PORT* pSrcPort = isf_audenc.p_base->iport(&isf_audenc, ISF_IN(0)); //ISF_PORT *pSrcPort = ImageUnit_In(&isf_audenc, ISF_IN1);
		// get RAW_ISF_DATA queue (from ISF_AudIn)
		pRawData = &(p_ctx_p->gISF_AudEnc_RawDATA[pMem->BufID]);

		if (gISF_AudEncMsgLevel[pMem->PathID] & ISF_AUDENC_MSG_RAW) {
			DBG_DUMP("[ISF_AUDENC][%d][%d] callback Release blk_id = 0x%08x, addr = 0x%08x, time = %d us\r\n", pMem->PathID, pMem->BufID, pRawData->h_data, nvtmpp_vb_block2addr(pRawData->h_data), Perf_GetCurrent());
		}

		if (pMem->Addr != 0) {
			(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(pMem->PathID), ISF_IN_PROBE_REL, ISF_ENTER); // [IN] REL_ENTER   // bitstream is actually encoded, addr !=0
		}

		rv = isf_audenc.p_base->do_release(&isf_audenc, ISF_IN(pMem->PathID), pRawData, ISF_IN_PROBE_REL);

		if ((pMem->Addr != 0) && (rv == ISF_OK)) {
			(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(pMem->PathID), ISF_IN_PROBE_REL, ISF_OK); // [IN] REL_OK  // bitstream is actually encoded, addr !=0
		}

		// set RAW_ISF_DATA queue empty
		SEM_WAIT(p_ctx_p->ISF_AUDENC_SEM_ID);
		p_ctx_p->gISF_AudEnc_IsRawUsed[pMem->BufID] = FALSE;
		SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);
	}

	if (pMem->Addr == 0) {
		return ISF_OK;
	}

	(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH, ISF_ENTER); // [OUT] PUSH_ENTER

	// Forced relesae buffer since no output pin need compressed data
	if (gISF_AudEncOutputCompressCount == 0) {
		DBG_ERR("CompCnt = 0\r\n");
		pNMI_AudEnc->SetParam(0, NMI_AUDENC_PARAM_UNLOCK_BS_ADDR, pMem->Addr);
		return ISF_OK;
	}

	pAStrmBuf->sign     = MAKEFOURCC('A', 'S', 'T', 'M');  //pAStrmBuf->flag
	pAStrmBuf->ChannelCnt = gISF_AudChannel;
	pAStrmBuf->CodecType = gISF_AudCodecFmt;
	pAStrmBuf->DataAddr = pMem->Addr;
	pAStrmBuf->DataSize = pMem->Size;
	pAStrmBuf->SampePerSecond = gISF_AudSampleRate;
	pAStrmBuf->timestamp = pMem->TimeStamp;
	if (gISF_AudCodecFmt == AUDENC_ENCODER_PCM) {
		pAStrmBuf->BitsPerSample = 16;
	} else {
		pAStrmBuf->BitsPerSample = 0;
	}
#if (SUPPORT_PULL_FUNCTION)
	// convert to physical addr
	pAStrmBuf->resv[11]  = AUDENC_VIRT2PHY(pMem->PathID, pMem->Addr);          // bs  phy
#endif

	// get AUDENC_BSDATA
	pAudEncData = _ISF_AudEnc_GET_BSDATA();

	if (pAudEncData) {
		pAudEncData->h_data    = pMem->Addr;
		pAudEncData->mem.Addr = pMem->Addr;
		pAudEncData->mem.Size = pMem->Size;

		pAudEncData->refCnt   = 0;

#if (SUPPORT_PULL_FUNCTION)
		pAudEncData->refCnt   += 1;  // pull need another count
#endif
	} else {
		isf_audenc.p_base->do_new(&isf_audenc, ISF_OUT(pMem->PathID), NVTMPP_VB_INVALID_POOL, 0, 0, pBsData, ISF_OUT_PROBE_NEW);
		(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [OUT] PUSH_WRN
		DBG_IND("Port %d _ISF_AudEnc_NEW_BSDATA is NULL!\r\n", pMem->PathID);
		// Set BS data address to unlock NMedia for this fail case
		pNMI_AudEnc->SetParam(pMem->PathID, NMI_AUDENC_PARAM_UNLOCK_BS_ADDR, pMem->Addr);
		return ISF_ERR_QUEUE_FULL;
	}

	// add AUDENC_BSDATA link list
	SEM_WAIT(p_ctx_p->ISF_AUDENC_SEM_ID);
	if (p_ctx_p->g_p_audenc_bsdata_link_head == NULL) {
		p_ctx_p->g_p_audenc_bsdata_link_head = pAudEncData;
		p_ctx_p->g_p_audenc_bsdata_link_tail = p_ctx_p->g_p_audenc_bsdata_link_head;
	} else {
		p_ctx_p->g_p_audenc_bsdata_link_tail->pnext_bsdata = pAudEncData;
		p_ctx_p->g_p_audenc_bsdata_link_tail = p_ctx_p->g_p_audenc_bsdata_link_tail->pnext_bsdata;
	}
	SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);

	// Set BS ISF_DATA info
	memcpy(pBsData->desc, pAStrmBuf, sizeof(AUD_BITSTREAM));
	pBsData->h_data      = ((pAudEncData->blk_id + 1) & 0xFF);
	//new pData by user
	isf_audenc.p_base->do_new(&isf_audenc, ISF_OUT(pMem->PathID), NVTMPP_VB_INVALID_POOL, pBsData->h_data, 0, pBsData, ISF_OUT_PROBE_NEW);

#if (SUPPORT_PULL_FUNCTION)
	// add to pull queue
	if (TRUE ==_ISF_AudEnc_Put_PullQueue(pMem->PathID, pBsData)) {
		(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH, ISF_OK); // [OUT] PUSH_OK
	} else {
		(&isf_audenc)->p_base->do_release(&isf_audenc, ISF_OUT(pMem->PathID), pBsData, ISF_OUT_PROBE_PUSH);
		(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [OUT] PUSH_WRN
	}
#endif

	if ((p_ctx_p->gISF_AudEncPortRunStatus == TRUE) && (p_ctx_p->gISF_OutputFmt == AUDENC_OUTPUT_COMPRESSION)) {
#if _TODO  // TODO: let it never push NOW ( is_allow_push seems not working ?? )
		p_port = isf_audenc.port_out[pMem->PathID];
		if (isf_audenc.p_base->get_is_allow_push(&isf_audenc, p_port)) {
			isf_audenc.p_base->do_push(&isf_audenc, ISF_OUT(pMem->PathID), pBsData, 0);
		}
#endif
	}

	return ISF_OK;
}

static void _ISF_AudEnc_RecCB(UINT32 uiEventID, UINT32 param)
{
	switch (uiEventID) {
	case NMI_AUDENC_EVENT_BS_CB:
		_ISF_AudEnc_StreamReadyCB((NMI_AUDENC_BS_INFO *)param);
		break;
	default:
		break;
	}
}

static ISF_RV _ISF_AudEnc_UpdatePortInfo(ISF_UNIT *pThisUnit, UINT32 oPort)
{
	UINT32 i = 0;

	// [ToDo]
#if 0
	ISF_INFO *p_dest_info = p_thisunit->port_outinfo[0];
	ISF_AUD_INFO *p_aud_info = (ISF_AUD_INFO *)(p_dest_info);

	if (!p_aud_info) {
		DBG_ERR("p_aud_info is null\r\n");
		return ISF_ERR_FAILED;
	}


	if (p_aud_info->dirty & ISF_INFO_DIRTY_AUDMAX) {
	}

	if (p_aud_info->dirty & ISF_INFO_DIRTY_AUDBITS) {
	}

	if (p_aud_info->dirty & ISF_INFO_DIRTY_AUDCH) {
	}

	if (p_aud_info->dirty & ISF_INFO_DIRTY_SAMPLERATE) {
	}
#endif

	gISF_AudEncOutputCompressCount = 0;
	gISF_AudEncOutputUncompressCount = 0;

	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[i];

		if ((p_ctx_p->gISF_AudEncPortOpenStatus == TRUE) && (p_ctx_p->gISF_OutputFmt == AUDENC_OUTPUT_COMPRESSION)) {
			gISF_AudEncOutputCompressCount ++;
		} else if ((p_ctx_p->gISF_AudEncPortOpenStatus == TRUE) && (p_ctx_p->gISF_OutputFmt == AUDENC_OUTPUT_UNCOMPRESSION)) {
			gISF_AudEncOutputUncompressCount ++;
		}
	}

#if SKIP_PCM_COPY
	if (gISF_AudCodecFmt == AUDENC_ENCODER_PCM && gISF_AudEncOutputCompressCount) {
		DBG_WRN("Cannot output compressed audio data due to codec type is PCM!");
	}
#endif

	return ISF_OK;
}

static ISF_RV _ISF_AudEnc_FreeMem_BsOnly(ISF_UNIT *pThisUnit, UINT32 id)
{
	AUDENC_BSDATA *pAudEncData = NULL;
	AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[id];
	// release left blocks
	pAudEncData = p_ctx_p->g_p_audenc_bsdata_link_head;
	while (pAudEncData != NULL) {
		_ISF_AudEnc_FREE_BSDATA(pAudEncData);
		pAudEncData = pAudEncData->pnext_bsdata;
	}
	SEM_WAIT(p_ctx_p->ISF_AUDENC_SEM_ID);
	p_ctx_p->g_p_audenc_bsdata_link_head = NULL;
	p_ctx_p->g_p_audenc_bsdata_link_tail = NULL;
	p_ctx_p->put_pullq = FALSE;
	SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);

	// release pull queue
	_ISF_AudEnc_RelaseAll_PullQueue(id);

	return ISF_OK;
}

static ISF_RV _ISF_AudEnc_AllocMem(ISF_UNIT *pThisUnit, UINT32 id)
{
	ISF_RV           r          = ISF_OK;
	NMI_AUDENC_MEM_RANGE Mem    = {0};
	UINT32           uiBufAddr  = 0;
	UINT32           uiBufSize  = 0;
	//ISF_PORT* 		 pSrcPort 	= _isf_unit_out(pThisUnit, ISF_OUT(id));  //ISF_PORT* pSrcPort = ImageUnit_Out(pThisUnit, id+ISF_OUT_BASE);
	ISF_PORT* 		 pSrcPort 	= isf_audenc.p_base->oport(pThisUnit, ISF_OUT(id));  //ISF_PORT* pSrcPort = ImageUnit_Out(pThisUnit, id+ISF_OUT_BASE);
	AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[id];

	pNMI_AudEnc->GetParam(id, NMI_AUDENC_PARAM_ALLOC_SIZE, &uiBufSize);

	//nvtmpp_vb_add_module(pThisUnit->unit_module); // move to _ISF_AudEnc_Open()

	if (p_ctx_p->gISF_AudEncMaxMemSize != 0 && p_ctx_p->gISF_AudEncMaxMem.Addr == 0) {
		uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "max", p_ctx_p->gISF_AudEncMaxMemSize, &(p_ctx_p->gISF_AudEncMaxMemBlk));
		if (uiBufAddr != 0) {
			p_ctx_p->gISF_AudEncMaxMem.Addr = uiBufAddr;
			p_ctx_p->gISF_AudEncMaxMem.Size = p_ctx_p->gISF_AudEncMaxMemSize;
#if (SUPPORT_PULL_FUNCTION)
			// set mmap info
			p_ctx_p->gISF_AudEncMem_MmapInfo.addr_virtual  = uiBufAddr;
			p_ctx_p->gISF_AudEncMem_MmapInfo.addr_physical = nvtmpp_vb_blk2pa(p_ctx_p->gISF_AudEncMaxMemBlk.h_data);
			p_ctx_p->gISF_AudEncMem_MmapInfo.size          = p_ctx_p->gISF_AudEncMaxMemSize;
#endif
		} else {
			DBG_ERR("[ISF_AUDENC][%d] get max blk failed!\r\n", id);
			return ISF_ERR_BUFFER_CREATE;
		}
	}

	// create enc buf
	if (p_ctx_p->gISF_AudEncMaxMem.Addr != 0 && p_ctx_p->gISF_AudEncMaxMem.Size != 0) {
		if (uiBufSize > p_ctx_p->gISF_AudEncMaxMem.Size) {
			DBG_ERR("[ISF_AUDENC][%d] Need mem size (%d) > max public size (%d)\r\n", id, uiBufSize, p_ctx_p->gISF_AudEncMaxMem.Size);
			pNMI_AudEnc->SetParam(id, NMI_AUDENC_PARAM_MEM_RANGE, (UINT32) &Mem);  //reset enc buf addr to 0
			return ISF_ERR_NEW_FAIL;
		}
		Mem.Addr = p_ctx_p->gISF_AudEncMaxMem.Addr;
		Mem.Size = p_ctx_p->gISF_AudEncMaxMem.Size;
	} else if (uiBufSize > 0) {
		uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "work", uiBufSize, &(p_ctx_p->gISF_AudEncMemBlk));
		if (uiBufAddr == 0) {
			DBG_ERR("[ISF_AUDENC][%d] get enc blk failed!\r\n", id);
			return ISF_ERR_BUFFER_CREATE;
		}

		Mem.Addr = uiBufAddr;
		Mem.Size = uiBufSize;
#if (SUPPORT_PULL_FUNCTION)
		// set mmap info
		p_ctx_p->gISF_AudEncMem_MmapInfo.addr_virtual  = uiBufAddr;
		p_ctx_p->gISF_AudEncMem_MmapInfo.addr_physical = nvtmpp_vb_blk2pa(p_ctx_p->gISF_AudEncMemBlk.h_data);
		p_ctx_p->gISF_AudEncMem_MmapInfo.size          = uiBufSize;
#endif
	}
	pNMI_AudEnc->SetParam(id, NMI_AUDENC_PARAM_MEM_RANGE, (UINT32) &Mem);

	// create raw queue
	uiBufSize = sizeof(ISF_DATA) * isf_audenc_data_queue_num;
	uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "raw", uiBufSize, &(p_ctx_p->gISF_AudEnc_RawData_MemBlk));
	if (uiBufAddr == 0) {
		DBG_ERR("[ISF_AUDENC][%d] get raw blk failed!\r\n", id);
		return ISF_ERR_BUFFER_CREATE;
	}
	p_ctx_p->gISF_AudEnc_RawDATA = (ISF_DATA *) uiBufAddr;

	// create raw queue
	uiBufSize = sizeof(BOOL) * isf_audenc_data_queue_num;
	uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "raw_used", uiBufSize, &(p_ctx_p->gISF_AudEnc_RawDataUsed_MemBlk));
	if (uiBufAddr == 0) {
		DBG_ERR("[ISF_AUDENC][%d] get raw used blk failed!\r\n", id);
		return ISF_ERR_BUFFER_CREATE;
	}
	p_ctx_p->gISF_AudEnc_IsRawUsed = (BOOL *) uiBufAddr;

	memset(p_ctx_p->gISF_AudEnc_IsRawUsed, 0, uiBufSize);

	// create bs queue
	if (gISF_AudEnc_Open_Count == 0) {
		uiBufSize = (sizeof(UINT32) + sizeof(AUDENC_BSDATA)) * PATH_MAX_COUNT * ISF_AUDENC_BSDATA_BLK_MAX;
		uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, NULL, "bs", uiBufSize, &gISF_AudEnc_BsData_MemBlk);
		if (uiBufAddr == 0) {
			DBG_ERR("[ISF_AUDENC][%d] get bs blk failed!\r\n", id);
			return ISF_ERR_BUFFER_CREATE;
		}
		g_p_audenc_bsdata_blk_maptlb = (UINT32 *) uiBufAddr;
		memset(g_p_audenc_bsdata_blk_maptlb, 0, uiBufSize);
		g_p_audenc_bsdata_blk_pool = (AUDENC_BSDATA *)(g_p_audenc_bsdata_blk_maptlb + (PATH_MAX_COUNT * ISF_AUDENC_BSDATA_BLK_MAX));
	}
	gISF_AudEnc_Open_Count++;

	return r;
}

static ISF_RV _ISF_AudEnc_FreeMem(ISF_UNIT *pThisUnit, UINT32 id, UINT32 b_do_release_i)
{
	ISF_RV r = ISF_OK;
	AUDENC_BSDATA *pAudEncData = NULL;
	AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[id];
	//ISF_DATA        *pRawData = NULL;
	//UINT32           i = 0;
	//UINT32           j = 0;

#if 0 // if user call AUDENC_PARAM_MAX_MEM_INFO with bRelease = 1,  it indeed release MAX buffer, and MAX addr/size is also set to 0..... this will cause exception here ( don't need to release dynamic alloc buffer )
	// release enc buf
	if ((gISF_AudEncMaxMem[id].Addr == 0 || gISF_AudEncMaxMem[id].Size == 0) && gISF_AudEnc_AllocMem[id]) {
		r = pThisUnit->p_base->do_release_i(pThisUnit, &(gISF_AudEncMemBlk[id]), ISF_OK);
		if (r != ISF_OK) {
			DBG_ERR("[ISF_AUDENC][%d] release enc blk failed, ret = %d\r\n", id, r);
			return ISF_ERR_BUFFER_DESTROY;
		}

		gISF_AudEnc_AllocMem[id] = 0;
	}
#endif
	// release raw queue
	/*for (j = 0; j < ISF_AUDENC_IN_NUM; j++) {
		for (i = 0; i < ISF_AUDENC_DATA_QUE_NUM; i++) {
			//ISF_PORT* pSrcPort = _isf_unit_in(&isf_audenc, ISF_IN(0) + j);  //ISF_PORT *pSrcPort = ImageUnit_In(&isf_audenc, ISF_IN(0) + j);
			//ISF_PORT* pSrcPort = isf_audenc.p_base->iport(&isf_audenc, ISF_IN(0) + j);  //ISF_PORT *pSrcPort = ImageUnit_In(&isf_audenc, ISF_IN(0) + j);
			if (gISF_AudEnc_IsRawUsed[j][i]) {
				pRawData = &gISF_AudEnc_RawDATA[j][i];

				// release lock RAW data
				isf_audenc.p_base->do_release(&isf_audenc, ISF_IN(j), pRawData, 0);
				isf_audenc.p_base->do_probe(&isf_audenc, ISF_IN(j), 0, ISF_ERR_INACTIVE_STATE);
			}
		}
	}*/

	// release raw queue
	if (p_ctx_p->gISF_AudEnc_RawData_MemBlk.h_data != 0) {
		if (TRUE == b_do_release_i) {
			r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_AudEnc_RawData_MemBlk), ISF_OK);
			if (r != ISF_OK) {
				DBG_ERR("[ISF_AUDENC][%d] release raw blk failed, ret = %d\r\n", id, r);
				return ISF_ERR_BUFFER_DESTROY;
			}
		}
		memset(&(p_ctx_p->gISF_AudEnc_RawData_MemBlk), 0, sizeof(ISF_DATA));
	}

	if (p_ctx_p->gISF_AudEnc_RawDataUsed_MemBlk.h_data != 0) {
		if (TRUE == b_do_release_i) {
			r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_AudEnc_RawDataUsed_MemBlk), ISF_OK);
			if (r != ISF_OK) {
				DBG_ERR("[ISF_AUDENC][%d] release raw used blk failed, ret = %d\r\n", id, r);
				return ISF_ERR_BUFFER_DESTROY;
			}
		}
		memset(&(p_ctx_p->gISF_AudEnc_RawDataUsed_MemBlk), 0, sizeof(ISF_DATA));
	}

	//memset(&(p_ctx_p->gISF_AudEnc_IsRawUsed), 0, sizeof(p_ctx_p->gISF_AudEnc_IsRawUsed));

	// release bs queue
	if (gISF_AudEnc_Open_Count > 0)
		gISF_AudEnc_Open_Count--;

	// release left blocks
	pAudEncData = p_ctx_p->g_p_audenc_bsdata_link_head;
	while (pAudEncData != NULL) {
		_ISF_AudEnc_FREE_BSDATA(pAudEncData);
		pAudEncData = pAudEncData->pnext_bsdata;
	}
	SEM_WAIT(p_ctx_p->ISF_AUDENC_SEM_ID);
	p_ctx_p->g_p_audenc_bsdata_link_head = NULL;
	p_ctx_p->g_p_audenc_bsdata_link_tail = NULL;
	SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);

    if (gISF_AudEnc_Open_Count == 0) {
		if (gISF_AudEnc_BsData_MemBlk.h_data != 0) {
			if (TRUE == b_do_release_i) {
				r = pThisUnit->p_base->do_release_i(pThisUnit, &gISF_AudEnc_BsData_MemBlk, ISF_OK);
				if (r != ISF_OK) {
					DBG_ERR("[ISF_AUDENC][%d] release bs blk failed, ret = %d\r\n", id, r);
					return ISF_ERR_BUFFER_DESTROY;
				}
			}
			memset(&gISF_AudEnc_BsData_MemBlk, 0, sizeof(ISF_DATA));
		}

		g_p_audenc_bsdata_blk_pool   = NULL;
		g_p_audenc_bsdata_blk_maptlb = NULL;

		{
			UINT32 i;
			for (i=0; i< PATH_MAX_COUNT; i++) {
				p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[i];

				p_ctx_p->g_p_audenc_bsdata_link_head = NULL;	//memset(g_p_audenc_bsdata_link_head, 0, sizeof(AUDENC_BSDATA *) * ISF_AUDENC_OUT_NUM);
				p_ctx_p->g_p_audenc_bsdata_link_tail = NULL;	//memset(g_p_audenc_bsdata_link_tail, 0, sizeof(AUDENC_BSDATA *) * ISF_AUDENC_OUT_NUM);
			}
		}
    }

	// release pull queue
	_ISF_AudEnc_RelaseAll_PullQueue(id);

	return r;
}

static ISF_RV _ISF_AudEnc_Reset(ISF_UNIT *pThisUnit, UINT32 oPort)
{
	AUDENC_CONTEXT_PORT *p_ctx_p = NULL;
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		return ISF_ERR_UNINIT; //sitll not init
	}

	if (oPort >= PATH_MAX_COUNT) {
		return ISF_ERR_INVALID_PORT_ID;
	}

	p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[oPort];

	//reset in parameter
	{
		UINT32        iPort    = (oPort - ISF_OUT_BASE + ISF_IN_BASE);
		ISF_AUD_INFO *pImgInfo_In  = (ISF_AUD_INFO *) (pThisUnit->port_ininfo[iPort-ISF_IN_BASE]);
		memset(pImgInfo_In,  0, sizeof(ISF_AUD_INFO));
	}
	//reset out parameter
	{
		ISF_AUD_INFO *pImgInfo_Out = (ISF_AUD_INFO *) (pThisUnit->port_outinfo[oPort-ISF_OUT_BASE]);
		memset(pImgInfo_Out, 0, sizeof(ISF_AUD_INFO));
	}
	//reset out state
	{
		UINT32 i = oPort - ISF_OUT_BASE;
		ISF_STATE *p_state = pThisUnit->port_outstate[i];
		memset(p_state, 0, sizeof(ISF_STATE));
	}

	//release MAX memory
	{
		ISF_RV r;

		if ((p_ctx_p->gISF_AudEncMaxMem.Addr != 0) && (p_ctx_p->gISF_AudEncMaxMemBlk.h_data != 0)) {
			ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, pThisUnit, ISF_OUT(oPort), "(release max memory)");
			r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_AudEncMaxMemBlk), ISF_OK);
			if (r == ISF_OK) {
				p_ctx_p->gISF_AudEncMaxMem.Addr = 0;
				p_ctx_p->gISF_AudEncMaxMem.Size = 0;
				p_ctx_p->gISF_AudEncMaxMemSize = 0;
				memset(&(p_ctx_p->gISF_AudEncMaxMemBlk), 0, sizeof(ISF_DATA));
			} else {
				DBG_ERR("[ISF_AUDENC][%d] release AudEnc Max blk failed, ret = %d\r\n", oPort, r);
			}
		}
	}

	//if state still = OPEN (maybe Close failed due to user not release all bs yet, Close will never success...so do the close routine here to RESET )
	//         (but DO NOT call do_release_i() , this will not valid after previous nvtmpp uninit().  nvtmpp will forget everything and re-init on next time init() )
	if (p_ctx_p->gISF_AudEnc_Opened == TRUE) {
		ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(reset - close)");
		_ISF_AudEnc_Close(pThisUnit, oPort, FALSE);  // FALSE means DO NOT call do_release_i()
		_ISF_AudEnc_Unlock_PullQueue(oPort);  // let pull_out(blocking mode) auto-unlock
		p_ctx_p->gISF_AudEnc_Opened = FALSE;
	}

	return ISF_OK;
}

static ISF_RV _ISF_AudEnc_Open(ISF_UNIT *pThisUnit, UINT32 id)
{
	AUDENC_CONTEXT_PORT *p_ctx_p;
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[id];

	if (!isf_audenc_init) {
		nvtmpp_vb_add_module(pThisUnit->unit_module);
		_isf_audenc_bs_data_init();
		_ISF_AudEnc_InstallCmd();
		NMR_AudEnc_AddUnit();
		isf_audenc_init = TRUE;
	}

	/// Check whether NMI_AudEnc exists?
	if (!pNMI_AudEnc) {
		if ((pNMI_AudEnc = NMI_GetUnit("AudEnc")) == NULL) {
			DBG_ERR("Get AudEnc unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	/// Check whether port is opend?
	if (p_ctx_p->gISF_AudEncPortOpenStatus == TRUE) {
		DBG_ERR("Port %d is already opened!", id);
		return ISF_ERR_ALREADY_OPEN;
	} else {
		p_ctx_p->gISF_AudEncPortOpenStatus = TRUE;
	}

	// [isf_flow] not support this param out to user, so set default value here for temporary
	{
		pNMI_AudEnc->SetParam(id, NMI_AUDENC_PARAM_FIXED_SAMPLE, 1024);
		pNMI_AudEnc->SetParam(id, NMI_AUDENC_PARAM_MAX_FRAME_QUEUE, 0);
	}

	pNMI_AudEnc->SetParam(id, NMI_AUDENC_REG_CB, (UINT32)_ISF_AudEnc_RecCB);

	if (pNMI_AudEnc->Open(id, 0) != E_OK) {
		p_ctx_p->gISF_AudEncPortOpenStatus = FALSE;
		return ISF_ERR_NOT_AVAIL;
	}

	p_ctx_p->gISF_AudEnc_Is_AllocMem = FALSE;   // set default as NOT create buffer (because AllocMem is doing at Start() now)

#if (DUMP_TO_FILE_TEST == ENABLE)
	if (fp_test_file == NULL) {
		DBG_DUMP("=====>>> Dump open file\r\n");
		fp_test_file = FileSys_OpenFile("A:\\AudEncDump.out", FST_CREATE_ALWAYS | FST_OPEN_WRITE);
	}
#endif

	return ISF_OK;
}

static ISF_RV _ISF_AudEnc_Close(ISF_UNIT *pThisUnit, UINT32 id, UINT32 b_do_release_i)
{
	AUDENC_CONTEXT_PORT *p_ctx_p;
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[id];

	/// Check whether NMI_AudEnc exists?
	if (!pNMI_AudEnc) {
		if ((pNMI_AudEnc = NMI_GetUnit("AudEnc")) == NULL) {
			DBG_ERR("Get AudEnc unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	/// Check whether port is closed?
	if (p_ctx_p->gISF_AudEncPortOpenStatus == FALSE) {
		DBG_ERR("Port %d is already closed!", id);
		return ISF_ERR_NOT_OPEN_YET;
	} else {
		p_ctx_p->gISF_AudEncPortOpenStatus = FALSE;
	}

	pNMI_AudEnc->Close(id);

	if (p_ctx_p->gISF_AudEnc_Is_AllocMem == TRUE) {
		// only call FreeMem if it indeed successfully AllocMem before, JIRA : IVOT_N01002_CO-562  ( user may only call open()->close(), never call start()....because AllocMem is doing at start(), so AllocMem never happened !! )
		if (ISF_OK == _ISF_AudEnc_FreeMem(pThisUnit, id, b_do_release_i)) {
			p_ctx_p->gISF_AudEnc_Is_AllocMem = FALSE;
		} else {
			DBG_ERR("[ISF_AUDENC][%d] FreeMem failed...!!\r\n", id);
		}
	} else {
		DBG_IND("[ISF_AUDENC][%d] close without FreeMem\r\n", id);
	}

	return ISF_OK;
}

static ISF_RV ISF_AudEnc_InputPort_PushImgBufToDest(ISF_PORT *pPort, ISF_DATA *pData, INT32 nWaitMs)
{
	PAUD_FRAME pAudBuf = (PAUD_FRAME)(pData->desc);
	UINT32 id = (pPort->iport - ISF_IN_BASE);
#if SKIP_PCM_COPY
	ISF_PORT *pDest = NULL;
	ISF_PORT *p_port = NULL;
#endif
	NMI_AUDENC_RAW_INFO readyInfo = {0};
	UINT32 i = 0;
	AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[id];
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		isf_audenc.p_base->do_release(&isf_audenc, ISF_IN(id), pData, ISF_IN_PROBE_REL);
		(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(id), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INVALID_STATE); // [IN] PUSH_ERR
		return ISF_ERR_UNINIT;
	}
	(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(id), ISF_IN_PROBE_PUSH, ISF_ENTER); // [IN] PUSH_ENTER

	if (p_ctx_p->gISF_AudEnc_Started == 0) {
		isf_audenc.p_base->do_release(&isf_audenc, ISF_IN(id), pData, ISF_IN_PROBE_REL);
		(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(id), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_INVALID_STATE); // [IN] PUSH_ERR
		return ISF_ERR_NOT_START;
	}

	//--- ISF_DATA from HDAL user push_in ---
	{
		if ((pAudBuf->addr[0] == 0) && (pAudBuf->phyaddr[0] != 0) && (pAudBuf->size != 0)) {
			// get virtual addr
			pAudBuf->addr[0] = nvtmpp_sys_pa2va(pAudBuf->phyaddr[0]);
		}
		if ((pAudBuf->addr[1] == 0) && (pAudBuf->phyaddr[1] != 0) && (pAudBuf->size != 0)) {
			// get virtual addr
			pAudBuf->addr[1] = nvtmpp_sys_pa2va(pAudBuf->phyaddr[1]);
		}
	}

	if ((pAudBuf->addr[0] == 0) || (pAudBuf->size == 0)) {
		if (gISF_AudEncMsgLevel[id] & ISF_AUDENC_MSG_RAW) {
			DBG_ERR("[ISF_AUDENC][%d] Data error, addr = 0x%08x, size = 0x%08x\r\n", id, pAudBuf->addr[0], pAudBuf->size);
		}
		isf_audenc.p_base->do_release(&isf_audenc, ISF_IN(id), pData, 0);
		isf_audenc.p_base->do_probe(&isf_audenc, ISF_IN(id), 0, ISF_ERR_INVALID_DATA);
		return ISF_ERR_INVALID_DATA;
	}

#if SKIP_PCM_COPY
	if (gISF_AudCodecFmt != AUDENC_ENCODER_PCM)
#endif
	{
		// serach for RAW_ISF_DATA queue empty
		for (i = 0; i < isf_audenc_data_queue_num; i++) {
			SEM_WAIT(p_ctx_p->ISF_AUDENC_SEM_ID);
			if (!p_ctx_p->gISF_AudEnc_IsRawUsed[i]) {
				p_ctx_p->gISF_AudEnc_IsRawUsed[i] = TRUE;
				SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);
				memcpy(&(p_ctx_p->gISF_AudEnc_RawDATA[i]), pData, sizeof(ISF_DATA));
				break;
			} else {
				SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);
			}
		}

		if (i == isf_audenc_data_queue_num) {
			if (gISF_AudEncMsgLevel[id] & ISF_AUDENC_MSG_RAW) {
				DBG_DUMP("[ISF_AUDENC][%d][%d] Full, Release blk = (0x%08x, 0x%08x), time = %d us\r\n", id, i, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
			}
			isf_audenc.p_base->do_release(&isf_audenc, ISF_IN(id), pData, ISF_IN_PROBE_REL);
			(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(id), ISF_IN_PROBE_PUSH_DROP, ISF_OK);  // [IN] PUSH_DROP
			return ISF_ERR_QUEUE_FULL;
		}

#if (SUPPORT_PULL_FUNCTION)
		if (TRUE == _ISF_AudEnc_isFull_PullQueue(id)) {
			//pull queue is full, not allow push
			if (gISF_AudEncMsgLevel[id] & ISF_AUDENC_MSG_DROP) {
				gISF_AudEncDropCount[id]++;
				if (gISF_AudEncDropCount[id] % 30 == 0) {
					gISF_AudEncDropCount[id] = 0;
					DBG_WRN("[ISF_AUDENC][%d][%d] Pull Queue Full, Release blk = (0x%08x, 0x%08x), time = %d us\r\n", id, i, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
				}
			}

			pNMI_AudEnc->SetParam(id, NMI_AUDENC_PARAM_DROP_FRAME, 1);
			isf_audenc.p_base->do_release(&isf_audenc, ISF_IN(id), pData, ISF_IN_PROBE_REL);
			(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(id), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [IN] PUSH_WRN

			SEM_WAIT(p_ctx_p->ISF_AUDENC_SEM_ID);
			p_ctx_p->gISF_AudEnc_IsRawUsed[i] = FALSE;
			SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);

			return ISF_ERR_QUEUE_FULL;
		}
#endif

		readyInfo.Addr      = pAudBuf->addr[0];
		readyInfo.Size      = pAudBuf->size;
		readyInfo.TimeStamp = pAudBuf->timestamp;
		readyInfo.BufID   = i;

		if (pNMI_AudEnc->In(id, (UINT32 *)&readyInfo) == E_SYS) {
			if (gISF_AudEncMsgLevel[id] & ISF_AUDENC_MSG_RAW) {
				DBG_DUMP("[ISF_AUDENC][%d][%d] Stop, Release blk = (0x%08x, 0x%08x), time = %d us\r\n", id, i, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
			}
			isf_audenc.p_base->do_release(&isf_audenc, ISF_IN(id), pData, ISF_IN_PROBE_REL);
			(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(id), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_PROCESS_FAIL); // [IN] PUSH_ERR
			SEM_WAIT(p_ctx_p->ISF_AUDENC_SEM_ID);
			p_ctx_p->gISF_AudEnc_IsRawUsed[i] = FALSE;
			SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);
			return ISF_ERR_PROCESS_FAIL;
		}
	}

#if SKIP_PCM_COPY
	for (i = 0; i < PATH_MAX_COUNT; i++) {
		if ((p_ctx_p->gISF_AudEncPortRunStatus == TRUE) && (p_ctx_p->gISF_OutputFmt == AUDENC_OUTPUT_UNCOMPRESSION)) {
			//pDest = _isf_unit_out(&isf_audenc, ISF_OUT(i));  //pDest = ImageUnit_Out(&isf_audenc, (ISF_OUT_BASE + i));
			//pDest = isf_audenc.p_base->oport(&isf_audenc, ISF_OUT(i));
			//if (pDest && _isf_unit_is_allow_push(pDest)) { //if (ImageUnit_IsAllowPush(pDest)) {
			//if (isf_audenc.p_base->get_is_allow_push(&isf_audenc, ISF_OUT(i))) {
			p_port = isf_audenc.port_out[i-ISF_OUT(0)];
			if (isf_audenc.p_base->get_is_allow_push(&isf_audenc, p_port)) {
				isf_audenc.p_base->do_add(&isf_audenc, ISF_OUT(i), pData);
				isf_audenc.p_base->do_push(&isf_audenc, ISF_OUT(i), pData, 0);
			}
		}
	}

	if (gISF_AudCodecFmt != AUDENC_ENCODER_AAC) {
		if (gISF_AudEncMsgLevel[id] & ISF_AUDENC_MSG_RAW) {
			DBG_DUMP("[ISF_AUDENC][%d] Input Release blk = (0x%08x, 0x%08x), time = %d us\r\n", id, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
		}
		isf_audenc.p_base->do_release(&isf_audenc, ISF_IN(id), pData, ISF_IN_PROBE_REL);
		(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_IN(id), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INVALID_DATA); // [IN] PUSH_ERR
	}
#endif

	return ISF_OK;
}

#if (SUPPORT_PULL_FUNCTION)

BOOL _ISF_AudEnc_isUserPull_AllReleased(UINT32 pathID)
{
	PAUDENC_PULL_QUEUE p_obj = NULL;
	AUDENC_BSDATA *pAudEncData = NULL;
	UINT32 pull_queue_num = 0;
	UINT32 link_list_num = 0;
	AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[pathID-ISF_OUT_BASE];

	if (p_ctx_p->put_pullq == FALSE) {
		return TRUE;
	}

	SEM_WAIT(p_ctx_p->ISF_AUDENC_SEM_ID);

	// PullQueue remain number
	p_obj = &(p_ctx_p->gISF_AudEnc_PullQueue);
	if (p_obj->Rear > p_obj->Front) {
		pull_queue_num = p_obj->Rear - p_obj->Front;
	} else if (p_obj->Rear < p_obj->Front){
		pull_queue_num = ISF_AUDENC_PULL_Q_MAX + p_obj->Rear - p_obj->Front;
	} else if (p_obj->Rear == p_obj->Front && p_obj->bFull == TRUE){
		pull_queue_num = ISF_AUDENC_PULL_Q_MAX;
	} else {
		pull_queue_num = 0;
	}

	// link-list remain number
	pAudEncData = p_ctx_p->g_p_audenc_bsdata_link_head;
	while (pAudEncData != NULL) {
		link_list_num++;
		pAudEncData = pAudEncData->pnext_bsdata;
	}

	SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);

	if (gISF_AudEncMsgLevel[pathID] & ISF_AUDENC_MSG_PULLQ) {
		DBG_DUMP("[ISF_AUDENC][%d] remain number => PullQueue(%u), link-list(%u)\r\n", pathID, pull_queue_num, link_list_num);
	}

	return (pull_queue_num == link_list_num)? TRUE:FALSE;
}

static ISF_RV _ISF_AudEnc_OutputPort_PullImgBufFromSrc(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	UINT32 id = oport - ISF_OUT_BASE;
	AUDENC_CONTEXT_PORT *p_ctx_p;
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[id];

	if (p_ctx_p->gISF_AudEnc_Opened == FALSE) {
		DBG_ERR("module is not open yet\r\n");
		return ISF_ERR_NOT_OPEN_YET;
	}

	(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(id), ISF_USER_PROBE_PULL, ISF_ENTER);  // [USER] PULL_ENTER

	return _ISF_AudEnc_Get_PullQueue(oport, p_data, wait_ms);  // [USER] do_probe  PULL_ERR, PULL_OK  will do in this function
}

BOOL _ISF_AudEnc_isEmpty_PullQueue(UINT32 pathID)
{
	PAUDENC_PULL_QUEUE p_obj;
	BOOL is_empty = FALSE;
	AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[pathID - ISF_OUT_BASE];

	SEM_WAIT(p_ctx_p->ISF_AUDENC_SEM_ID);

	p_obj = &(p_ctx_p->gISF_AudEnc_PullQueue);
	is_empty = ((p_obj->Front == p_obj->Rear) && (p_obj->bFull == FALSE));

	SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);

	return is_empty;
}

BOOL _ISF_AudEnc_isFull_PullQueue(UINT32 pathID)
{
	PAUDENC_PULL_QUEUE p_obj;
	BOOL is_full = FALSE;
	AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[pathID - ISF_OUT_BASE];

	SEM_WAIT(p_ctx_p->ISF_AUDENC_SEM_ID);

	p_obj = &(p_ctx_p->gISF_AudEnc_PullQueue);
	is_full = ((p_obj->Front == p_obj->Rear) && (p_obj->bFull == TRUE));

	SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);

	return is_full;
}

void _ISF_AudEnc_Unlock_PullQueue(UINT32 pathID)
{
	AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[pathID - ISF_OUT_BASE];

	if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_AUDENC_PULLQ_SEM_ID, vos_util_msec_to_tick(0))) {
		DBG_IND("[ISF_AUDENC][%d] no data in pull queue, auto unlock pull blocking mode !!\r\n", pathID);
	}

	// [case_1]   no data in pullQ, then this 0 ->   0   -> 1 to unlock HDAL pull blocking mode (fake semaphore.... but it's OK, because _ISF_AudEnc_Get_PullQueue() will return FALSE, there's no actual data in queue )
	// [case_2] have data in pullQ, then this n -> (n-1) -> n to recover the semaphore count
	SEM_SIGNAL(p_ctx_p->ISF_AUDENC_PULLQ_SEM_ID);
}

BOOL _ISF_AudEnc_Put_PullQueue(UINT32 pathID, ISF_DATA *data_in)
{
	PAUDENC_PULL_QUEUE pObj;
	AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[pathID - ISF_OUT_BASE];

	SEM_WAIT(p_ctx_p->ISF_AUDENC_SEM_ID);
	pObj = &(p_ctx_p->gISF_AudEnc_PullQueue);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		if (gISF_AudEncMsgLevel[pathID] & ISF_AUDENC_MSG_PULLQ) {
			DBG_ERR("[ISF_AUDENC][%d] Pull Queue is Full!\r\n", pathID);
		}
		SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);
		return FALSE;
	} else {
		memcpy(&pObj->Queue[pObj->Rear], data_in, sizeof(ISF_DATA));

		pObj->Rear = (pObj->Rear + 1) % ISF_AUDENC_PULL_Q_MAX;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}
		p_ctx_p->put_pullq = TRUE;
		SEM_SIGNAL(p_ctx_p->ISF_AUDENC_PULLQ_SEM_ID); // PULLQ + 1
		SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);
		return TRUE;
	}
}

ISF_RV _ISF_AudEnc_Get_PullQueue(UINT32 pathID, ISF_DATA *data_out, INT32 wait_ms)
{
	PAUDENC_PULL_QUEUE pObj;
	AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[pathID - ISF_OUT_BASE];
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		return ISF_ERR_UNINIT;
	}

	if (wait_ms < 0) {
		// blocking (wait until data available) , if success PULLQ - 1 , else wait forever (or until signal interrupt and return FALSE)
		if (SEM_WAIT_INTERRUPTIBLE(p_ctx_p->ISF_AUDENC_PULLQ_SEM_ID)) {
			(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pathID),  ISF_USER_PROBE_PULL_WRN, ISF_ERR_QUEUE_EMPTY);  // [USER] PULL_WRN
			return ISF_ERR_QUEUE_EMPTY;
		}
	} else  {
		// non-blocking (wait_ms=0) , timeout (wait_ms > 0). If success PULLQ - 1 , else just return FALSE
		if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_AUDENC_PULLQ_SEM_ID, vos_util_msec_to_tick(wait_ms))) {
			DBG_IND("[ISF_AUDENC][%d] Pull Queue Semaphore timeout!\r\n", pathID);
			(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pathID),  ISF_USER_PROBE_PULL_WRN, ISF_ERR_WAIT_TIMEOUT);  // [USER] PULL_WRN
			return ISF_ERR_WAIT_TIMEOUT;
		}
	}

	// check state
	if (p_ctx_p->gISF_AudEnc_Opened == FALSE) {
		return ISF_ERR_NOT_OPEN_YET;
	}

	SEM_WAIT(p_ctx_p->ISF_AUDENC_SEM_ID);
	pObj = &(p_ctx_p->gISF_AudEnc_PullQueue);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);
		DBG_IND("[ISF_AUDENC][%d] Pull Queue is Empty !!!\r\n", pathID);
		(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pathID),  ISF_USER_PROBE_PULL_WRN, ISF_ERR_QUEUE_EMPTY);  // [USER] PULL_WRN
		return ISF_ERR_QUEUE_EMPTY;
	} else {
		memcpy(data_out, &pObj->Queue[pObj->Front], sizeof(ISF_DATA));

		pObj->Front= (pObj->Front+ 1) % ISF_AUDENC_PULL_Q_MAX;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = FALSE;
		}
		SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);

		(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(pathID),  ISF_USER_PROBE_PULL, ISF_OK);  // [USER] PULL_OK
		return ISF_OK;
	}
}

BOOL _ISF_AudEnc_RelaseAll_PullQueue(UINT32 pathID)
{
	ISF_DATA p_data;

	while (ISF_OK == _ISF_AudEnc_Get_PullQueue(pathID, &p_data, 0)) ; //consume all pull queue

	return TRUE;
}

#endif // #if (SUPPORT_PULL_FUNCTION)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL _ISF_AudEnc_SXCmd_Dump(CHAR *strCmd)
{
	UINT32 i;

	DBG_DUMP("ImageUnit_AudEnc Setting:\r\n");
	DBG_DUMP("  Codec Format : %d\r\n", gISF_AudCodecFmt);
	DBG_DUMP("  Sample Rate  : %d\r\n", gISF_AudSampleRate);
	DBG_DUMP("  Channel No   : %d\r\n", gISF_AudChannel);
	DBG_DUMP("  AAC ADTS En  : %d\r\n", gISF_ADTSHeaderEnable);
	DBG_DUMP("  File Type    : %d\r\n", gISF_FileType);
	DBG_DUMP("  Rec Format   : %d\r\n", gISF_RecFormat);
	DBG_DUMP("AudEnc port open status:\r\n");
	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[i];
		DBG_DUMP("%d ", p_ctx_p->gISF_AudEncPortOpenStatus);
	}

	DBG_DUMP("AudEnc port run status:\r\n");
	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[i];
		DBG_DUMP("%d ", p_ctx_p->gISF_AudEncPortRunStatus);
	}

	DBG_DUMP("AudEnc output format:\r\n");
	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[i];
		DBG_DUMP("%d ", p_ctx_p->gISF_OutputFmt);
	}
	DBG_DUMP("\r\n");

	return TRUE;
}

static BOOL _ISF_AudEnc_ShowMsg(CHAR *strCmd)
{
	UINT32 uiPathID = 0, uiMsgLevel = 0;
	sscanf_s(strCmd, "%d %d", &uiPathID, &uiMsgLevel);
	if (uiPathID < PATH_MAX_COUNT) {
		gISF_AudEncMsgLevel[uiPathID] = uiMsgLevel;
	} else {
		DBG_ERR("[ISF_AUDENC] invalid path id = %d\r\n", uiPathID);
	}
	return TRUE;
}

static BOOL _ISF_AudEnc_PullQInfo(CHAR *strCmd)
{
	UINT32 uiPathID = 0;
	sscanf_s(strCmd, "%d", &uiPathID);

	if (uiPathID < PATH_MAX_COUNT) {
		gISF_AudEncMsgLevel[uiPathID] |= ISF_AUDENC_MSG_PULLQ;
		_ISF_AudEnc_isUserPull_AllReleased(uiPathID);
	}

	return TRUE;
}

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
static BOOL _isf_audenc_cmd_debug_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_audenc_cmd_debug(dev, cmdstr);
}

static BOOL _isf_audenc_cmd_trace_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_audenc_cmd_trace(dev, cmdstr);
}

static BOOL _isf_audenc_cmd_probe_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_audenc_cmd_probe(dev, cmdstr);
}

static BOOL _isf_audenc_cmd_perf_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_audenc_cmd_perf(dev, cmdstr);
}

static BOOL _isf_audenc_cmd_save_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_audenc_cmd_save(dev, cmdstr);
}
#endif // RTOS only


static SXCMD_BEGIN(isfae, "isf_audenc command")
SXCMD_ITEM("dump", _ISF_AudEnc_SXCmd_Dump, "Dump AudEnc status")
SXCMD_ITEM("showmsg %s", _ISF_AudEnc_ShowMsg, "show msg (Param: PathId Level(0:Disable, 1:RAW Data, 2:Lock, 4:PullQ) => showmsg 0 1)")
SXCMD_ITEM("pullqinfo %s", _ISF_AudEnc_PullQInfo, "pullq info (Param: PathId => pullqinfo 0)")
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
SXCMD_ITEM("debug %s", _isf_audenc_cmd_debug_entry, "debug [dev] [i/o] [mask]  , debug port => debug d0 o0 mffff")
SXCMD_ITEM("trace %s", _isf_audenc_cmd_trace_entry, "trace [dev] [i/o] [mask]  , trace port => trace d0 o0 mffff")
SXCMD_ITEM("probe %s", _isf_audenc_cmd_probe_entry, "probe [dev] [i/o] [mask]  , probe port => probe d0 o0 meeee")
SXCMD_ITEM("perf  %s", _isf_audenc_cmd_perf_entry,  "perf  [dev] [i/o]         , perf  port => perf d0 i0")
SXCMD_ITEM("save  %s", _isf_audenc_cmd_save_entry,  "save  [dev] [i/o] [count] , save  port => save d0 i0 1")
SXCMD_ITEM("info  %s", _isf_audenc_api_info,        "(no param)                , show  info")
#endif
SXCMD_END()

static void _ISF_AudEnc_InstallCmd(void)
{
	sxcmd_addtable(isfae);
}

int _isf_audenc_cmd_isfae_showhelp(void)
{
	UINT32 cmd_num = SXCMD_NUM(isfae);
	UINT32 loop=1;

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
	DBG_DUMP("---------------------------------------------------------------------\n");
	DBG_DUMP("  aenc\n");
	DBG_DUMP("---------------------------------------------------------------------\n");
#else
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "isfae");
	DBG_DUMP("=====================================================================\n");
#endif

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		DBG_DUMP("%15s : %s\r\n", isfae[loop].p_name, isfae[loop].p_desc);
	}

	return 0;
}

int _isf_audenc_cmd_isfae(char* sub_cmd_name, char *cmd_args)
{
	UINT32 cmd_num = SXCMD_NUM(isfae);
	UINT32 loop=1;
	BOOL ret=FALSE;

	if (strncmp(sub_cmd_name, "?", 2) == 0) {
		_isf_audenc_cmd_isfae_showhelp();
		return 0;
	}

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(sub_cmd_name, isfae[loop].p_name, strlen(sub_cmd_name)) == 0) {
			ret = isfae[loop].p_func(cmd_args);
			break;
		}
	}

	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n  Usage : type  \"cat /proc/hdal/aenc/help\" for help.\r\n");
		return -1;
	}


	return ret;
}

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
MAINFUNC_ENTRY(aenc, argc, argv)
{
	char sub_cmd_name[33] = {0};
	char cmd_args[256]    = {0};
	UINT8 i = 0;

	if (argc == 1) {
		_isf_audenc_cmd_isfae_showhelp();
		return 0; // only "isfve"
	}

	// setup sub_cmd ( showmsg, pullqinfo )  ( debug, trace, probe, perf, save )
	if (argc >= 2) {
		strncpy(sub_cmd_name, argv[1], 32);
	}

	// setup command params ( 0 2 1920 1080 ... )   ( d0 o0 mffff )
	if (argc >=3) {
		for (i = 2 ; i < argc ; i++) {
			strncat(cmd_args, argv[i], 20);
			strncat(cmd_args, " ", 20);
		}
	}

	//DBG_ERR("subcmd = %s, args = %s\r\n", sub_cmd_name, cmd_args);

	_isf_audenc_cmd_isfae(sub_cmd_name, cmd_args);

	return 0;
}
#endif

int _isf_audenc_cmd_isfdbg_showhelp(void)
{
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "debug , trace , probe");
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("%-25s : %s\r\n", "debug [dev] [i/o] [mask]", "debug port");
	DBG_DUMP("%-25s : %s\r\n", "trace [dev] [i/o] [mask]", "trace port");
	DBG_DUMP("%-25s : %s\r\n", "probe [dev] [i/o] [mask]", "probe port");

	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "perf , save");
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("%-25s : %s\r\n", "perf  [dev] [i/o]", "perf port");
	DBG_DUMP("%-25s : %s\r\n", "save  [dev] [i/o]", "save port");

	return 0;
}

// cmd debug d0 o0 /mask=000
extern void _isf_flow_debug_port(char* unit_name, char* port_name, char* mask_name);
int _isf_audenc_cmd_debug(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *mask;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d')) { _isf_flow_debug_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "audenc"); // audenc only has one device, and the name is "audenc"

	//verify i/o
	io = strsep(&cmdstr, delimiters);  // i0, i1, i2, o0, o1, o2 ...
	if((io == NULL) || ((io[0]!='i') && (io[0]!='o') && (io[0]!='c'))) { _isf_flow_debug_port(0,0,0); return 0;}

	//mask
	mask = strsep(&cmdstr, delimiters);

	//do debug
	_isf_flow_debug_port(unit_name, io, mask);
	return 1;
}

// cmd trace d0 o0 /mask=000
extern void _isf_flow_trace_port(char* unit_name, char* port_name, char* mask_name);
int _isf_audenc_cmd_trace(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *mask;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d')) { _isf_flow_trace_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "audenc"); // audenc only has one device, and the name is "audenc"

	//verify i/o
	io = strsep(&cmdstr, delimiters);  // i0, i1, i2, o0, o1, o2 ...
	if((io == NULL) || ((io[0]!='i') && (io[0]!='o') && (io[0]!='c'))) { _isf_flow_trace_port(0,0,0); return 0;}

	//mask
	mask = strsep(&cmdstr, delimiters);

	//do trace
	_isf_flow_trace_port(unit_name, io, mask);
	return 0;
}

// cmd probe d0 o0 /mask=000
extern void _isf_flow_probe_port(char* unit_name, char* port_name, char* mask_name);
int _isf_audenc_cmd_probe(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *mask;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d')) { _isf_flow_probe_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "audenc"); // audenc only has one device, and the name is "audenc"

	//verify i/o
	io = strsep(&cmdstr, delimiters);  // i0, i1, i2, o0, o1, o2 ...
	if((io == NULL) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_probe_port(0,0,0); return 0;}

	//mask
	mask = strsep(&cmdstr, delimiters);

	//do probe
	_isf_flow_probe_port(unit_name, io, mask);

	return 0;
}

// cmd perf d0 o0
extern void _isf_flow_perf_port(char* unit_name, char* port_name);
int _isf_audenc_cmd_perf(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d') || (dev[1] !='0')) { _isf_flow_perf_port(0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "audenc"); // audenc only has one device, and the name is "audenc"

	//verify i/o
	io = strsep(&cmdstr, delimiters);  // i0, i1, i2, o0, o1, o2 ...
	if((io == NULL) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_perf_port(0,0); return 0;}

	//do perf
	_isf_flow_perf_port(unit_name, io);

	return 0;
}

// cmd save d0 o0 count
extern void _isf_flow_save_port(char* unit_name, char* port_name, char* count_name);
int _isf_audenc_cmd_save(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *count;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d') || (dev[1] !='0')) { _isf_flow_save_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "audenc"); // audenc only has one device, and the name is "audenc"

	//verify i/o
	io = strsep(&cmdstr, delimiters);  // i0, i1, i2, o0, o1, o2 ...
	if((io == NULL) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_save_port(0,0,0); return 0;}

	//count
	count = strsep(&cmdstr, delimiters);

	//do save
	_isf_flow_save_port(unit_name, io, count);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static ISF_PORT_CAPS ISF_AudEnc_Input_Caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = ISF_AudEnc_InputPort_PushImgBufToDest,
};

/*static ISF_PORT ISF_AudEnc_InputPort[ISF_AUDENC_IN_NUM] = {
	{
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(0),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_audenc,
	.p_srcunit = NULL,
	},
};*/
static ISF_PORT ISF_AudEnc_InputPort_IN1 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(0),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_audenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_AudEnc_InputPort_IN2 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(1),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_audenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_AudEnc_InputPort_IN3 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(2),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_audenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_AudEnc_InputPort_IN4 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(3),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_audenc,
	.p_srcunit = NULL,
};

static ISF_PORT *ISF_AudEnc_InputPortList[ISF_AUDENC_IN_NUM] = {
	&ISF_AudEnc_InputPort_IN1,
	&ISF_AudEnc_InputPort_IN2,
	&ISF_AudEnc_InputPort_IN3,
	&ISF_AudEnc_InputPort_IN4,
};

static ISF_PORT_CAPS *ISF_AudEnc_InputPortList_Caps[ISF_AUDENC_IN_NUM] = {
	&ISF_AudEnc_Input_Caps,
	&ISF_AudEnc_Input_Caps,
	&ISF_AudEnc_Input_Caps,
	&ISF_AudEnc_Input_Caps,
};

#if (SUPPORT_PULL_FUNCTION)
static ISF_PORT_CAPS ISF_AudEnc_OutputPort_Caps = {
	.connecttype_caps = ISF_CONNECT_PUSH  | ISF_CONNECT_PULL | ISF_CONNECT_SYNC,
	.do_pull = _ISF_AudEnc_OutputPort_PullImgBufFromSrc,
};
#else
static ISF_PORT_CAPS ISF_AudEnc_OutputPort_Caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_pull = NULL,
};
#endif

static ISF_PORT *ISF_AudEnc_OutputPortList[ISF_AUDENC_OUT_NUM] = {0};
static ISF_PORT *ISF_AudEnc_OutputPortList_Cfg[ISF_AUDENC_OUT_NUM] = {0};
static ISF_PORT_CAPS *ISF_AudEnc_OutputPortList_Caps[ISF_AUDENC_OUT_NUM] = {
	&ISF_AudEnc_OutputPort_Caps,
	&ISF_AudEnc_OutputPort_Caps,
	&ISF_AudEnc_OutputPort_Caps,
	&ISF_AudEnc_OutputPort_Caps,
};

static ISF_STATE ISF_AudEnc_OutputPort_State[ISF_AUDENC_OUT_NUM] = {0};

static ISF_STATE *ISF_AudEnc_OutputPortList_State[ISF_AUDENC_OUT_NUM] = {
	&ISF_AudEnc_OutputPort_State[0],
	&ISF_AudEnc_OutputPort_State[1],
	&ISF_AudEnc_OutputPort_State[2],
	&ISF_AudEnc_OutputPort_State[3],
};

static ISF_INFO isf_audenc_outputinfo_in[ISF_AUDENC_IN_NUM] = {0};
static ISF_INFO *isf_audenc_outputinfolist_in[ISF_AUDENC_IN_NUM] = {
	&isf_audenc_outputinfo_in[0],
	&isf_audenc_outputinfo_in[1],
	&isf_audenc_outputinfo_in[2],
	&isf_audenc_outputinfo_in[3],
};

static ISF_INFO isf_audenc_outputinfo_out[ISF_AUDENC_OUT_NUM] = {0};
static ISF_INFO *isf_audenc_outputinfolist_out[ISF_AUDENC_OUT_NUM] = {
	&isf_audenc_outputinfo_out[0],
	&isf_audenc_outputinfo_out[1],
	&isf_audenc_outputinfo_out[2],
	&isf_audenc_outputinfo_out[3],
};

static ISF_RV ISF_AudEnc_BindInput(struct _ISF_UNIT *pThisUnit, UINT32 iPort, struct _ISF_UNIT *pSrcUnit, UINT32 oPort)
{
	return ISF_OK;
}

static ISF_RV ISF_AudEnc_BindOutput(struct _ISF_UNIT *pThisUnit, UINT32 oPort, struct _ISF_UNIT *pDestUnit, UINT32 iPort)
{
	return ISF_OK;
}

static ISF_RV _ISF_AudEnc_Start(ISF_UNIT *pThisUnit, UINT32 oPort)
{
	UINT32 id = oPort-ISF_OUT_BASE;
	AUDENC_CONTEXT_PORT *p_ctx_p;
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[id];

	ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(start)\r\n");

	// "first time" START, alloc memory
	{
		if (p_ctx_p->gISF_AudEncMaxMem.Addr == 0 || p_ctx_p->gISF_AudEncMaxMem.Size == 0) {
			if (p_ctx_p->gISF_AudEncMaxMemSize == 0) {   // [isf_flow] use MAX_MEM way only
				DBG_ERR("[ISF_AUDENC][%d] Fatal Error, please set MAX_MEM first !!\r\n", oPort);
				return ISF_ERR_FAILED;
			}

			if (_ISF_AudEnc_AllocMem(pThisUnit, oPort) != ISF_OK) {
				DBG_ERR("[ISF_AUDENC][%d] AllocMem failed\r\n", oPort);
				p_ctx_p->gISF_AudEnc_Is_AllocMem = FALSE;
				return ISF_ERR_FAILED;
			} else {
				p_ctx_p->gISF_AudEnc_Is_AllocMem = TRUE;
			}
		}
	}

	if (p_ctx_p->gISF_AudEncMaxMemSize > 0) {
		NMI_AUDENC_MEM_RANGE Mem    = {0};
		UINT32           uiBufSize  = 0;
		pNMI_AudEnc->GetParam(id, NMI_AUDENC_PARAM_ALLOC_SIZE, &uiBufSize);

		//check MAX_MEM & memory requirement for current setting
		if (uiBufSize > p_ctx_p->gISF_AudEncMaxMem.Size) {
			DBG_ERR("FATAL ERROR: current encode setting need memory(%d) > MAX_MEM(%d)\r\n", uiBufSize, p_ctx_p->gISF_AudEncMaxMem.Size);
			return ISF_ERR_FAILED;
		}

		//reassign memory layout (user may open->start->stop->change codec->start, JIRA: IPC_680-128
		Mem.Addr = p_ctx_p->gISF_AudEncMaxMem.Addr;
		Mem.Size = uiBufSize;
		pNMI_AudEnc->SetParam(id, NMI_AUDENC_PARAM_MEM_RANGE, (UINT32) &Mem);
	}

	_ISF_AudEnc_FreeMem_BsOnly(pThisUnit, id);           // free all bs in isf_unit queue , JIRA: IPC_680-120
	pNMI_AudEnc->Action(oPort, NMI_AUDENC_ACTION_START); // free all bs in nmedia queue

	p_ctx_p->gISF_AudEnc_Started = 1;

	return ISF_OK;
}

static ISF_RV _ISF_AudEnc_Stop(ISF_UNIT *pThisUnit, UINT32 oPort)
{
	UINT32 id = oPort-ISF_OUT_BASE;
	AUDENC_CONTEXT_PORT *p_ctx_p;
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[id];

	ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, id, "(stop)\r\n");

	p_ctx_p->gISF_AudEnc_Started = 0;
	pNMI_AudEnc->Action(id, NMI_AUDENC_ACTION_STOP);

	return ISF_OK;
}

static ISF_RV ISF_AudEnc_SetParam(ISF_UNIT *pThisUnit, UINT32 param, UINT32 value)
{
	if (!isf_audenc_init) {
		nvtmpp_vb_add_module(pThisUnit->unit_module);
		_isf_audenc_bs_data_init();
		_ISF_AudEnc_InstallCmd();
		NMR_AudEnc_AddUnit();
		isf_audenc_init = TRUE;
	}

	return ISF_OK;
}

static UINT32 ISF_AudEnc_GetParam(ISF_UNIT *pThisUnit, UINT32 param)
{
	return 0;
}

static ISF_RV ISF_AudEnc_SetPortParam(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param, UINT32 value)
{
	ISF_RV r = ISF_OK;
	UINT32 id = 0;
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;
	ER ret;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

    if (nPort == ISF_CTRL)
    {
        return ISF_AudEnc_SetParam(pThisUnit, param, value);
    }

	if (!isf_audenc_init) {
		nvtmpp_vb_add_module(pThisUnit->unit_module);
		_isf_audenc_bs_data_init();
		_ISF_AudEnc_InstallCmd();
		NMR_AudEnc_AddUnit();
		isf_audenc_init = TRUE;
	}

	if (!pNMI_AudEnc) {
		if ((pNMI_AudEnc = NMI_GetUnit("AudEnc")) == NULL) {
			DBG_ERR("Get AudEnc unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	if ((nPort < (int)ISF_IN_BASE) && (nPort < pThisUnit->port_out_count)) { // Output port
		AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[nPort-ISF_OUT_BASE];

		switch (param) {
		case AUDENC_PARAM_CODEC:
		{
			PMP_AUDENC_ENCODER p_audio_obj = NULL;
			UINT32 id = nPort - ISF_OUT_BASE;

			ISF_UNIT_TRACE(ISF_OP_PARAM_AUDBS, pThisUnit, ISF_OUT(nPort), "codec(%d)", value);

			gISF_AudCodecFmt = value;
			pNMI_AudEnc->SetParam(id, NMI_AUDENC_PARAM_CODEC, gISF_AudCodecFmt);

			switch (gISF_AudCodecFmt) {
				case MOVAUDENC_PCM:     p_audio_obj = MP_PCMEnc_getAudioObject((MP_AUDENC_ID)id);     break;
				case MOVAUDENC_AAC:     p_audio_obj = MP_AACEnc_getAudioObject((MP_AUDENC_ID)id);     break;
				case MOVAUDENC_PPCM:    p_audio_obj = MP_PPCMEnc_getAudioObject((MP_AUDENC_ID)id);    break;
				case MOVAUDENC_ULAW:    p_audio_obj = MP_uLawEnc_getAudioObject((MP_AUDENC_ID)id);    break;
				case MOVAUDENC_ALAW:    p_audio_obj = MP_aLawEnc_getAudioObject((MP_AUDENC_ID)id);    break;
				case 0: // close reset to 0
					return ISF_OK;
				default:
					DBG_ERR("[ISF_AUDENC][%d] getEncodeObject failed\r\n", id);
					return ISF_ERR_FAILED;
			}

			pNMI_AudEnc->SetParam(id, NMI_AUDENC_PARAM_ENCODER_OBJ, (UINT32)p_audio_obj);
		}
		break;

		case AUDENC_PARAM_AAC_ADTS_HEADER:
			ISF_UNIT_TRACE(ISF_OP_PARAM_AUDBS, pThisUnit, ISF_OUT(nPort), "aac_adts(%d)", value);

			gISF_ADTSHeaderEnable = value;
			pNMI_AudEnc->SetParam(nPort-ISF_OUT_BASE, NMI_AUDENC_PARAM_AAC_ADTS_HEADER, gISF_ADTSHeaderEnable ? TRUE : FALSE);
			break;

		case AUDENC_PARAM_FILETYPE:
			gISF_FileType = value;
			pNMI_AudEnc->SetParam(nPort-ISF_OUT_BASE, NMI_AUDENC_PARAM_FILETYPE, gISF_FileType);
			break;

		case AUDENC_PARAM_RECFORMAT:
			gISF_RecFormat = value;
			pNMI_AudEnc->SetParam(nPort-ISF_OUT_BASE, NMI_AUDENC_PARAM_RECFORMAT, gISF_RecFormat);
			break;

		case AUDENC_PARAM_FIXED_SAMPLE:
			pNMI_AudEnc->SetParam(nPort-ISF_OUT_BASE, NMI_AUDENC_PARAM_FIXED_SAMPLE, value);
			r = ISF_OK;
			break;

		case AUDENC_PARAM_MAX_FRAME_QUEUE:
			pNMI_AudEnc->SetParam(nPort-ISF_OUT_BASE, NMI_AUDENC_PARAM_MAX_FRAME_QUEUE, value);
			r = ISF_OK;
			break;

		case AUDENC_PARAM_PORT_OUTPUT_FMT:
			if (p_ctx_p->gISF_OutputFmt != value) {
#if (SKIP_PCM_COPY == DISABLE)
				if (value == AUDENC_OUTPUT_UNCOMPRESSION) {
					value = AUDENC_OUTPUT_COMPRESSION;
				}
#endif
				p_ctx_p->gISF_OutputFmt = value;
				r = ISF_OK;
			} else {
				r = ISF_OK;
			}
			break;

		case AUDENC_PARAM_ENCBUF_MS:
			ISF_UNIT_TRACE(ISF_OP_PARAM_AUDBS, pThisUnit, ISF_OUT(nPort), "enc_buf_ms(%d)", value);
			pNMI_AudEnc->SetParam(nPort-ISF_OUT_BASE, NMI_AUDENC_PARAM_ENCBUF_MS, value);
			break;

		case AUDENC_PARAM_BS_RESERVED_SIZE:
			isf_reserved_size = value;
			pNMI_AudEnc->SetParam(nPort-ISF_OUT_BASE, NMI_AUDENC_PARAM_BS_RESERVED_SIZE, isf_reserved_size);
			break;

		case AUDENC_PARAM_AAC_VER:
			isf_aac_ver = value;
			pNMI_AudEnc->SetParam(nPort-ISF_OUT_BASE, NMI_AUDENC_PARAM_AAC_VER, isf_aac_ver);
			break;

		case AUDENC_PARAM_DATA_QUEUE_NUM:
			isf_audenc_data_queue_num = value;
			break;

		case AUDENC_PARAM_TARGET_RATE:
			ret = pNMI_AudEnc->SetParam(nPort-ISF_OUT_BASE, NMI_AUDENC_PARAM_TARGETRATE, value);
			if (ret != E_OK) {
				r = ISF_ERR_INVALID_VALUE;
			}
			isf_target_rate = value;
			break;

		default:
			DBG_ERR("Invalid OUT paramter (0x%x)\r\n", param);
			r = ISF_ERR_INVALID_PARAM;
			break;
		}
	// coverity[unsigned_compare]
	} else if (nPort-ISF_IN_BASE < pThisUnit->port_in_count) { // Input port
		AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[nPort-ISF_IN_BASE];

		id = nPort-ISF_IN_BASE;
		switch (param) {
		case AUDENC_PARAM_SAMPLERATE:
			ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM, pThisUnit, ISF_IN(id), "sr(%d)", value);

			pNMI_AudEnc->SetParam(nPort-ISF_IN_BASE, NMI_AUDENC_PARAM_SAMPLERATE, value);

			// set dirty
			if (value != 0) {
				p_ctx_p->isf_audenc_param_dirty = 1;
			}
			break;

		case AUDENC_PARAM_CHS:
			if (value <= 2) {
				ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM, pThisUnit, ISF_IN(id), "chs(%d)", value);
				pNMI_AudEnc->SetParam(nPort-ISF_IN_BASE, NMI_AUDENC_PARAM_CHS, value);

				// set dirty
				if (value != 0) {
					p_ctx_p->isf_audenc_param_dirty = 1;
				}
			} else {
				DBG_ERR("Invalid channel number = %d\r\n", value);
				r = ISF_ERR_INVALID_VALUE;
			}
			break;

		case AUDENC_PARAM_BITS:
			ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM, pThisUnit, ISF_IN(id), "bit_rate(%d)", value);
			pNMI_AudEnc->SetParam(nPort-ISF_IN_BASE, NMI_AUDENC_PARAM_BITS, value);

			// set dirty
			if (value != 0) {
				p_ctx_p->isf_audenc_param_dirty = 1;
			}
			break;

		default:
			DBG_ERR("invalid IN paramter (0x%x)\r\n", param);
			r = ISF_ERR_INVALID_PARAM;
			break;
		}
	}

	return r;
}

static UINT32 ISF_AudEnc_GetPortParam(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param)
{
	UINT32 value = 0;
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

    if (nPort == ISF_CTRL)
    {
        return ISF_AudEnc_GetParam(pThisUnit, param);
    }

	if (!pNMI_AudEnc) {
		if ((pNMI_AudEnc = NMI_GetUnit("AudEnc")) == NULL) {
			DBG_ERR("Get AudEnc unit failed\r\n");
			return value;
		}
	}
	if ((nPort < (int)ISF_IN_BASE) && (nPort < pThisUnit->port_out_count)) { // Parameters on output port
		AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[nPort-ISF_OUT_BASE];

		switch (param) {
		case AUDENC_PARAM_CODEC:
			pNMI_AudEnc->GetParam(nPort-ISF_OUT_BASE, NMI_AUDENC_PARAM_CODEC, &value);
			break;

		case AUDENC_PARAM_AAC_ADTS_HEADER:
			pNMI_AudEnc->GetParam(nPort-ISF_OUT_BASE, NMI_AUDENC_PARAM_AAC_ADTS_HEADER, &value);
			break;

		case AUDENC_PARAM_FIXED_SAMPLE:
			pNMI_AudEnc->GetParam(nPort-ISF_OUT_BASE, NMI_AUDENC_PARAM_FIXED_SAMPLE, &value);
			break;

#if SKIP_PCM_COPY
		case AUDENC_PARAM_PORT_OUTPUT_FMT:
			value = p_ctx_p->gISF_OutputFmt;
			break;
#endif

#if (SUPPORT_PULL_FUNCTION)
		case AUDENC_PARAM_BUFINFO_PHYADDR:
			value = p_ctx_p->gISF_AudEncMem_MmapInfo.addr_physical;
			break;

		case AUDENC_PARAM_BUFINFO_SIZE:
			value = p_ctx_p->gISF_AudEncMem_MmapInfo.size;
			break;
#endif

		case AUDENC_PARAM_ENCBUF_MS:
			pNMI_AudEnc->GetParam(nPort-ISF_OUT_BASE, NMI_AUDENC_PARAM_ENCBUF_MS, &value);
			break;

		default:
			DBG_ERR("Invalid OUTPUT paramter (0x%x)\r\n", param);
			value = ISF_ERR_NOT_SUPPORT;
			break;
		}
	} else if (nPort-ISF_IN_BASE < pThisUnit->port_in_count) { // Parameters on input port
		switch (param) {
		case AUDENC_PARAM_CHS:
			pNMI_AudEnc->GetParam(nPort-ISF_IN_BASE, NMI_AUDENC_PARAM_CHS, &value);
			break;

		case AUDENC_PARAM_SAMPLERATE:
			pNMI_AudEnc->GetParam(nPort-ISF_IN_BASE, NMI_AUDENC_PARAM_SAMPLERATE, &value);
			break;

		case AUDENC_PARAM_BITS:
			pNMI_AudEnc->GetParam(nPort-ISF_IN_BASE, NMI_AUDENC_PARAM_BITS, &value);
			break;

		default:
			DBG_ERR("Invalid INPUT paramter (0x%x)\r\n", param);
			value = ISF_ERR_NOT_SUPPORT;
			break;
		}
	}
	return value;
}

static ISF_RV ISF_AudEnc_SetPortStruct(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param, UINT32* p_struct, UINT32 size)
{
	ISF_RV ret = ISF_OK;
	UINT32 value = (UINT32)p_struct;
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	if (!isf_audenc_init) {
		nvtmpp_vb_add_module(pThisUnit->unit_module);
		_isf_audenc_bs_data_init();
		_ISF_AudEnc_InstallCmd();
		NMR_AudEnc_AddUnit();
		isf_audenc_init = TRUE;
	}

	if (!pNMI_AudEnc) {
		if ((pNMI_AudEnc = NMI_GetUnit("AudEnc")) == NULL) {
			DBG_ERR("Get AudEnc unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	if ((nPort < (int)ISF_IN_BASE) && (nPort < pThisUnit->port_out_count)) {
		AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[nPort-ISF_OUT_BASE];

		switch (param) {
		case AUDENC_PARAM_MAX_MEM_INFO:
			{
				ISF_RV r = ISF_OK;
				NMI_AUDENC_MAX_MEM_INFO* pMaxMemInfo = (NMI_AUDENC_MAX_MEM_INFO*) value;

				DBG_IND("MAX_MEM_INFO: bR=%d, AudBit=%d, AudCh=%d, AudCodec=%d, AudSR=%d, RecFmt=%d\r\n",
						pMaxMemInfo->bRelease, pMaxMemInfo->uiAudBits, pMaxMemInfo->uiAudChannels, pMaxMemInfo->uiAudCodec, pMaxMemInfo->uiAudioSR, pMaxMemInfo->uiRecFormat);

				if (nPort < PATH_MAX_COUNT) {
					if (pMaxMemInfo->bRelease) {
						if (p_ctx_p->gISF_AudEncMaxMem.Addr != 0) {
							// relase max mem
							r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_AudEncMaxMemBlk), ISF_OK);
							if (r == ISF_OK) {
								p_ctx_p->gISF_AudEncMaxMem.Addr = 0;
								p_ctx_p->gISF_AudEncMaxMem.Size = 0;
								p_ctx_p->gISF_AudEncMaxMemSize = 0;
							} else {
								DBG_ERR("[ISF_AUDENC][%d] release AudEnc Max blk failed, ret = %d\r\n", nPort, r);
							}
						}
					} else {
						if (p_ctx_p->gISF_AudEncMaxMem.Addr == 0) {
							// get max mem size
							pNMI_AudEnc->SetParam(nPort, NMI_AUDENC_PARAM_MAX_MEM_INFO, (UINT32)pMaxMemInfo);
							p_ctx_p->gISF_AudEncMaxMemSize = pMaxMemInfo->uiEncsize;
						} else {
							DBG_ERR("[ISF_AUDENC][%d] max buf exist, please release it first.\r\n", nPort);
						}
					}
				} else {
					DBG_ERR("[ISF_AUDENC] invalid port index = %d\r\n", nPort);
				}
			}
			break;

		default:
			DBG_ERR("AudEnc.out[%d] set struct[%08x] = %08x\r\n", nPort-ISF_OUT_BASE, param, value);
			ret = ISF_ERR_NOT_SUPPORT;
			break;
		}

		return ret;
	} else if (nPort-ISF_IN_BASE < pThisUnit->port_in_count) {
		// not used
	}

	return ISF_ERR_NOT_SUPPORT;
}

// [ToDo]
static ISF_RV ISF_AudEnc_GetPortStruct(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param, UINT32* p_struct, UINT32 size)
{
	DBG_WRN("Not ready yet\r\n");

	return ISF_OK;
}

static ISF_RV ISF_AudEnc_UpdatePort(struct _ISF_UNIT *pThisUnit, UINT32 oPort, ISF_PORT_CMD cmd)
{
	ISF_RV r = ISF_OK;
	AUDENC_CONTEXT_PORT *p_ctx_p = NULL;
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	if (oPort-ISF_OUT_BASE >= PATH_MAX_COUNT) {
		if (cmd != ISF_PORT_CMD_RESET) DBG_ERR("[AUDENC] invalid path index = %d\r\n", oPort-ISF_OUT_BASE);
		return ISF_ERR_INVALID_PORT_ID;
	}
	p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[oPort-ISF_OUT_BASE];

	if (!isf_audenc_init) {
		nvtmpp_vb_add_module(pThisUnit->unit_module);
		_isf_audenc_bs_data_init();
		_ISF_AudEnc_InstallCmd();
		NMR_AudEnc_AddUnit();
		isf_audenc_init = TRUE;
	}

	if (!pNMI_AudEnc) {
		if ((pNMI_AudEnc = NMI_GetUnit("AudEnc")) == NULL) {
			DBG_ERR("Get AudEnc unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	switch (cmd) {
	case ISF_PORT_CMD_RESET:
		ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(reset)");
		_ISF_AudEnc_Reset(pThisUnit, oPort);
		break;

	case ISF_PORT_CMD_OPEN:                                ///< off -> ready   (alloc mempool and start task)
		ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(open)\r\n");
		_ISF_AudEnc_UpdatePortInfo(pThisUnit, oPort);
		if (_ISF_AudEnc_Open(pThisUnit, oPort) != ISF_OK) {
			DBG_ERR("_ISF_AudEnc_Open fail\r\n");
			return ISF_ERR_FAILED;
		}

		p_ctx_p->gISF_AudEnc_Opened = TRUE;
		break;

	case ISF_PORT_CMD_OFFTIME_SYNC:                        ///< off -> off     (off-time property is dirty)
		ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(off-sync)\r\n");
		_ISF_AudEnc_UpdatePortInfo(pThisUnit, oPort);
		break;

	case ISF_PORT_CMD_READYTIME_SYNC:                      ///< ready -> ready (apply ready-time parameter if it is dirty)
		ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(ready-sync)\r\n");
		_ISF_AudEnc_UpdatePortInfo(pThisUnit, oPort);
		break;
	case ISF_PORT_CMD_START: {                             ///< ready -> run   (initial context, engine enable and device power on, start data processing)
			_ISF_AudEnc_Start(pThisUnit, oPort);
		}
		break;
	case ISF_PORT_CMD_PAUSE:                               ///< run -> wait    (pause data processing, keep context, engine or device enter suspend mode)
		ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(pause)\r\n");
		break;
	case ISF_PORT_CMD_RESUME:                              ///< wait -> run    (engine or device leave suspend mode, restore context, resume data processing)
		ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(resume)\r\n");
		break;
	case ISF_PORT_CMD_STOP: {                              ///< run -> stop    (stop data processing, engine disable or device power off)
			_ISF_AudEnc_Stop(pThisUnit, oPort);
		}
		break;
	case ISF_PORT_CMD_CLOSE:                               ///< stop -> off    (terminate task and free mempool)
		ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(close)\r\n");

		if (FALSE == _ISF_AudEnc_isUserPull_AllReleased(oPort)) {
			DBG_ERR("Close failed !! Please release all pull_out bitstream before attemping to close !!\r\n");
			return ISF_ERR_NOT_REL_YET;
		}

		_ISF_AudEnc_Close(pThisUnit, oPort, TRUE); // TRUE means to call do_release_i() normally

		// unlock pull_out
		_ISF_AudEnc_Unlock_PullQueue(oPort);

		p_ctx_p->gISF_AudEnc_Opened = FALSE;
		break;
	case ISF_PORT_CMD_RUNTIME_UPDATE: {                    ///< run -> run     (apply run-time parameter if it is dirty)
			UINT32 id = oPort-ISF_OUT_BASE;

			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(run-update)\r\n");
			if (p_ctx_p->isf_audenc_param_dirty) {
				// stop
				_ISF_AudEnc_Stop(pThisUnit, id);
				// start
				_ISF_AudEnc_Start(pThisUnit, id);
				// reset flag
				p_ctx_p->isf_audenc_param_dirty = 0;
			}
		}
		break;
	case ISF_PORT_CMD_RUNTIME_SYNC:
		ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(run-sync)\r\n");
		break;
	default:
		DBG_ERR("Invalid port command (0x%x)\r\n", cmd);
		break;
	}
	return r;
}

void isf_audenc_install_id(void)
{
	UINT32 i = 0;
    for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDENC_CONTEXT_PORT   *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[i];

        OS_CONFIG_SEMPHORE(p_ctx_p->ISF_AUDENC_SEM_ID, 0, 1, 1);
#if (SUPPORT_PULL_FUNCTION)
		OS_CONFIG_SEMPHORE(p_ctx_p->ISF_AUDENC_PULLQ_SEM_ID, 0, 0, 0);
#endif
    }
	OS_CONFIG_SEMPHORE(ISF_AUDENC_COMMON_SEM_ID, 0, 1, 1);
	OS_CONFIG_SEMPHORE(ISF_AUDENC_PROC_SEM_ID, 0, 1, 1);
}

void isf_audenc_uninstall_id(void)
{
	UINT32 i = 0;
    for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDENC_CONTEXT_PORT   *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[i];

        SEM_DESTROY(p_ctx_p->ISF_AUDENC_SEM_ID);
#if (SUPPORT_PULL_FUNCTION)
		SEM_DESTROY(p_ctx_p->ISF_AUDENC_PULLQ_SEM_ID);
#endif
    }
	SEM_DESTROY(ISF_AUDENC_COMMON_SEM_ID);
	SEM_DESTROY(ISF_AUDENC_PROC_SEM_ID);
}

static ISF_RV _ISF_AudEnc_LockCB(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data)
{
	UINT32 i = 0;
	AUDENC_BSDATA *pAudEncData = NULL;
	ID tid;
	char unit_name[8];
	if (p_thisunit) {
		strncpy(unit_name, p_thisunit->unit_name, 8);
		unit_name[7]='\0';
	} else {
		strcpy(unit_name, "user");
	}

	if (!h_data) {
		get_tid(&tid);
		DBG_ERR("[ISF_AUDENC] LOCK tid = %d h_data is NULL\r\n", tid);
		return ISF_ERR_INVALID_VALUE;
	}

	// search AUDENC_BSDATA link list by h_data
	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDENC_CONTEXT_PORT   *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[i];
		AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

		if (p_ctx_c->ctx_mem.ctx_addr == 0) {
			return ISF_ERR_UNINIT;
		}

		SEM_WAIT(p_ctx_p->ISF_AUDENC_SEM_ID);
		pAudEncData = p_ctx_p->g_p_audenc_bsdata_link_head;

		while (pAudEncData != NULL) {
			if (pAudEncData->blk_id == ((h_data - 1) & 0xFF)) {
				pAudEncData->refCnt++;
				if (gISF_AudEncMsgLevel[i] & ISF_AUDENC_MSG_LOCK) {
					DBG_DUMP("[ISF_AUDENC][%d] LOCK %s ref %d hData 0x%x\r\n", i, unit_name, pAudEncData->refCnt, pAudEncData->h_data);
				}
				SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);
				return ISF_OK;
			}

			pAudEncData = pAudEncData->pnext_bsdata;
		}
		SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);
	}
	return ISF_OK;
}

static ISF_RV _ISF_AudEnc_UnLockCB(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data)
{
	UINT32 i = 0;
	AUDENC_BSDATA *pAudEncData = NULL;
	ID tid;
	char unit_name[8];
	if (p_thisunit) {
		strncpy(unit_name, p_thisunit->unit_name, 8);
		unit_name[7]='\0';
	} else {
		strcpy(unit_name, "user");
	}

	if (!h_data) {
		get_tid(&tid);
		DBG_ERR("[ISF_AUDENC] UnLOCK tid = %d h_data is NULL\r\n", tid);
		return ISF_ERR_INVALID_VALUE;
	}

	// search AUDENC_BSDATA link list by h_data
	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDENC_CONTEXT_PORT   *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[i];
		AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

		if (p_ctx_c->ctx_mem.ctx_addr == 0) {
			return ISF_ERR_UNINIT;
		}

		SEM_WAIT(p_ctx_p->ISF_AUDENC_SEM_ID);
		pAudEncData = p_ctx_p->g_p_audenc_bsdata_link_head;

		while (pAudEncData != NULL) {
			if (pAudEncData->blk_id == ((h_data - 1) & 0xFF)) {
				if (pAudEncData->refCnt > 0) {
					pAudEncData->refCnt--;
				} else {
					get_tid(&tid);
					if (p_thisunit) {
						DBG_ERR("[ISF_AUDENC][%d] UnLOCK %s tid = %d refCnt is 0\r\n", i, unit_name, tid);
					} else {
						DBG_ERR("[ISF_AUDENC][%d] UnLOCK NULL tid = %d refCnt is 0\r\n", i, tid);
					}
					SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);
					return ISF_ERR_INVALID_VALUE;
				}

				if (gISF_AudEncMsgLevel[i] & ISF_AUDENC_MSG_LOCK) {
					DBG_DUMP("[ISF_AUDENC][%d] UNLOCK %s ref %d hData 0x%x\r\n", i, unit_name, pAudEncData->refCnt, pAudEncData->h_data);
				}

				if (pAudEncData->refCnt == 0) {
					(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(i),  ISF_USER_PROBE_REL, ISF_ENTER);  // [USER] REL_ENTER

					// Set BS data address is unlock to NMedia
					pNMI_AudEnc->SetParam(i, NMI_AUDENC_PARAM_UNLOCK_BS_ADDR, pAudEncData->h_data);

					// Free isf_audenc bs_data
					if (pAudEncData != p_ctx_p->g_p_audenc_bsdata_link_head) {
						AUDENC_BSDATA *pAudEncData2 = NULL;
						get_tid(&tid);

						if (gISF_AudEncMsgLevel[i] & ISF_AUDENC_MSG_LOCK) {
							DBG_ERR("[ISF_AUDENC][%d] UnLOCK %s, tid = %d, not first data in queue!! 1st link data = (0x%x, 0x%x, %d), rel data = (0x%x, 0x%x, %d)\r\n",
								i,
								unit_name,
								tid,
								(UINT32)p_ctx_p->g_p_audenc_bsdata_link_head,
								(p_ctx_p->g_p_audenc_bsdata_link_head?p_ctx_p->g_p_audenc_bsdata_link_head->h_data:0),
								(p_ctx_p->g_p_audenc_bsdata_link_head?p_ctx_p->g_p_audenc_bsdata_link_head->blk_id:0),
								(UINT32)pAudEncData,
								pAudEncData->h_data,
								pAudEncData->blk_id);
						}

						// search for release
						pAudEncData2 = p_ctx_p->g_p_audenc_bsdata_link_head;

						while (pAudEncData2 != NULL) {
							if (pAudEncData2->pnext_bsdata == pAudEncData) {
								pAudEncData2->pnext_bsdata = pAudEncData->pnext_bsdata;
								if (pAudEncData2->pnext_bsdata == NULL) {
									p_ctx_p->g_p_audenc_bsdata_link_tail = pAudEncData2;
								}
								break;
							}

							pAudEncData2 = pAudEncData2->pnext_bsdata;
						}

						// free block
						_ISF_AudEnc_FREE_BSDATA(pAudEncData);

						(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(i),  ISF_USER_PROBE_REL, ISF_OK);  // [USER] REL_OK ( with some error, but actually released )

					} else {
						// update link head
						p_ctx_p->g_p_audenc_bsdata_link_head = p_ctx_p->g_p_audenc_bsdata_link_head->pnext_bsdata;

						// free block
						_ISF_AudEnc_FREE_BSDATA(pAudEncData);

						(&isf_audenc)->p_base->do_probe(&isf_audenc, ISF_OUT(i),  ISF_USER_PROBE_REL, ISF_OK);  // [USER] REL_OK
					}

					if (gISF_AudEncMsgLevel[i] & ISF_AUDENC_MSG_LOCK) {
						DBG_DUMP("[ISF_AUDENC][%d] Free h_data = 0x%x, blk_id = %d\r\n", i, pAudEncData->h_data, pAudEncData->blk_id);
					}
				}
				SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);
				return ISF_OK;
			}

			pAudEncData = pAudEncData->pnext_bsdata;
		}
		SEM_SIGNAL(p_ctx_p->ISF_AUDENC_SEM_ID);
	}
	return ISF_OK;
}

//----------------------------------------------------------------------------

static UINT32 _isf_audenc_query_context_size(void)
{
	return (PATH_MAX_COUNT * sizeof(AUDENC_CONTEXT_PORT));
}

extern UINT32 _nmr_audenc_query_context_size(void);

static UINT32 _isf_audenc_query_context_memory(void)
{
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	p_ctx_c->ctx_mem.isf_audenc_size = _isf_audenc_query_context_size();
	p_ctx_c->ctx_mem.nm_audenc_size = _nmr_audenc_query_context_size();
	p_ctx_c->ctx_mem.mp_obj_size = 0;

	return (p_ctx_c->ctx_mem.isf_audenc_size) + (p_ctx_c->ctx_mem.nm_audenc_size) + (p_ctx_c->ctx_mem.mp_obj_size);
}

//----------------------------------------------------------------------------

static void _isf_audenc_assign_context(UINT32 addr)
{
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	g_ae_ctx.port = (AUDENC_CONTEXT_PORT*)addr;
	memset((void *)g_ae_ctx.port, 0, p_ctx_c->ctx_mem.isf_audenc_size);
}

extern void _nmr_audenc_assign_context(UINT32 addr);

static ER _isf_audenc_assign_context_memory_range(UINT32 addr)
{
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	// [ isf_vdoenc ]
	_isf_audenc_assign_context(addr);
	addr += p_ctx_c->ctx_mem.isf_audenc_size;

	// [ nmedia_audenc ]
	_nmr_audenc_assign_context(addr);
	#if _TODO
	addr += p_ctx_c->ctx_mem.nm_audenc_size;

	// [ mp_object ]
	addr += p_ctx_c->ctx_mem.mp_obj_size;
	#endif

	return E_OK;
}

//----------------------------------------------------------------------------

static void _isf_audenc_free_context(void)
{
	g_ae_ctx.port = NULL;
}

extern void _nmr_audenc_free_context(void);

static ER _isf_audenc_free_context_memory_range(void)
{
	// [ isf_vdoenc ]
	_isf_audenc_free_context();

	// [ nmedia_audenc ]
	_nmr_audenc_free_context();

	// [ mp_object ]


	return E_OK;
}

//----------------------------------------------------------------------------

static ISF_RV _isf_audenc_do_init(UINT32 h_isf, UINT32 path_max_count)
{
	ISF_RV rv = ISF_OK;
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;

	if (h_isf > 0) { //client process
		if (!g_audenc_init[0]) { //is master process already init?
			rv = ISF_ERR_INIT; //not allow init from client process!
			goto _AE_INIT_ERR;
		}
		g_audenc_init[h_isf] = 1; //set init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_audenc_init[i]) { //client process still alive?
				rv = ISF_ERR_INIT; //not allow init from main process!
				goto _AE_INIT_ERR;
			}
		}
	}

	if (path_max_count > 0) {
		//update count
		PATH_MAX_COUNT = path_max_count;
		if (PATH_MAX_COUNT > AUDENC_MAX_PATH_NUM) {
			DBG_WRN("path max count %d > max %d!\r\n", PATH_MAX_COUNT, AUDENC_MAX_PATH_NUM);
			PATH_MAX_COUNT = AUDENC_MAX_PATH_NUM;
		}
	}

	if (p_ctx_c->ctx_mem.ctx_addr != 0) {
		DBG_ERR("[ISF_AUDENC] already init !!\r\n");
		rv = ISF_ERR_INIT;
		goto _AE_INIT_ERR;
	}

	{
		UINT32 buf_addr = 0;
		UINT32 buf_size = _isf_audenc_query_context_memory();

		//alloc context of all port
		nvtmpp_vb_add_module(isf_audenc.unit_module);
		buf_addr = (&isf_audenc)->p_base->do_new_i(&isf_audenc, NULL, "ctx", buf_size, &(p_ctx_c->ctx_mem.ctx_memblk));

		if (buf_addr == 0) {
			DBG_ERR("[ISF_AUDENC] alloc context memory fail !!\r\n");
			rv = ISF_ERR_HEAP;
			goto _AE_INIT_ERR;
		}
		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_audenc, ISF_CTRL, "(alloc context memory) (%u, 0x%08x, %u)", PATH_MAX_COUNT, buf_addr, buf_size);

		p_ctx_c->ctx_mem.ctx_addr = buf_addr;

		// assign memory range
		_isf_audenc_assign_context_memory_range(p_ctx_c->ctx_mem.ctx_addr);

		//install each device's kernel object
		isf_audenc_install_id();
		nmr_audenc_install_id();

		g_audenc_init[h_isf] = 1; //set init

		return ISF_OK;
	}

_AE_INIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("init fail! %d\r\n", rv);
	}
	return rv;
}

static ISF_RV _isf_audenc_do_uninit(UINT32 h_isf)
{
	ISF_RV rv = ISF_OK;
	AUDENC_CONTEXT_COMMON *p_ctx_c = (AUDENC_CONTEXT_COMMON *)&g_ae_ctx.comm;
	UINT32 i;

	if (h_isf > 0) { //client process
		if (!g_audenc_init[0]) { //is master process already init?
			rv = ISF_ERR_UNINIT; //not allow uninit from client process!
			goto _AE_UNINIT_ERR;
		}
		g_audenc_init[h_isf] = 0; //clear init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_audenc_init[i]) { //client process still alive?
				rv = ISF_ERR_UNINIT; //not allow uninit from main process!
				goto _AE_UNINIT_ERR;
			}
		}
	}

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		return ISF_ERR_UNINIT;
	}

	if (PATH_MAX_COUNT == 0) {
		return ISF_ERR_UNINIT;
	}

	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDENC_CONTEXT_PORT *p_ctx_p = (AUDENC_CONTEXT_PORT *)&g_ae_ctx.port[i];
		if (p_ctx_p->gISF_AudEnc_Opened) {
			DBG_ERR("Not all port closed\r\n");
			return ISF_ERR_ALREADY_OPEN;
		}
	}

	{
		UINT32 i;

		//reset all units of this module
		DBG_IND("unit(%s) => clear state\r\n", (&isf_audenc)->unit_name);   // equal to call UpdatePort - STOP
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_audenc)->p_base->do_clear_state(&isf_audenc, oport);
		}

		DBG_IND("unit(%s) => clear bind\r\n", (&isf_audenc)->unit_name);    // equal to call UpdatePort - CLOSE
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_audenc)->p_base->do_clear_bind(&isf_audenc, oport);
		}

		DBG_IND("unit(%s) => clear context\r\n", (&isf_audenc)->unit_name); // equal to call UpdatePort - Reset
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_audenc)->p_base->do_clear_ctx(&isf_audenc, oport);
		}


		//uninstall each device's kernel object
		isf_audenc_uninstall_id();
		nmr_audenc_uninstall_id();

		//free context of all port
		if (p_ctx_c->ctx_mem.ctx_memblk.h_data != 0) {
			rv = (&isf_audenc)->p_base->do_release_i(&isf_audenc, &(p_ctx_c->ctx_mem.ctx_memblk), ISF_OK);
			if (rv == ISF_OK) {
				memset((void*)&(p_ctx_c->ctx_mem), 0, sizeof(AUDENC_CTX_MEM));  // reset ctx_mem to 0
			} else {
				DBG_ERR("[ISF_AUDENC] free context memory fail !!\r\n");
				goto _AE_UNINIT_ERR;
			}
		} else {
			DBG_ERR("[ISF_AUDENC] context h_data == NULL !!\r\n");
			rv = ISF_ERR_INVALID_DATA_ID;
			goto _AE_UNINIT_ERR;
		}

		// reset NULL to all pointer
		_isf_audenc_free_context_memory_range();

		g_audenc_init[h_isf] = 0; //clear init

		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_audenc, ISF_CTRL, "(release context memory)");
		return ISF_OK;
	}

_AE_UNINIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("uninit fail! %d\r\n", rv);
	}
	return rv;
}

ISF_RV _isf_audenc_do_command(UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2)
{
	UINT32 h_isf = (cmd >> 28); //extract h_isf
	cmd &= ~0xf0000000; //clear h_isf
	//DBG_IND("(do_command) cmd = %u, p0 = 0x%08x, p1 = %u, p2 = %u\r\n", cmd, p0, p1, p2);
	switch(cmd) {
	case 1: //INIT
		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_audenc, ISF_CTRL, "(do_init) max_path = %u", p1);
		return _isf_audenc_do_init(h_isf, p1); //p1: max path count
	case 0: //UNINIT
		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_audenc, ISF_CTRL, "(do_uninit)");
		return _isf_audenc_do_uninit(h_isf);
	default:
		DBG_ERR("unsupport cmd(%u) !!\r\n", cmd);
		break;
	}
	return ISF_ERR_NOT_SUPPORT;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static ISF_DATA_CLASS _isf_audenc_base = {
	.this = MAKEFOURCC('A','E','N','C'),
	.base = MAKEFOURCC('A','S','T','M'),
    .do_lock   = _ISF_AudEnc_LockCB,
    .do_unlock = _ISF_AudEnc_UnLockCB,
};

static ISF_RV _isf_audenc_bs_data_init(void)
{
	_isf_audenc_base.this = 0;
	isf_audenc.p_base->init_data(&gISF_AudEncBsData, &_isf_audenc_base, _isf_audenc_base.base, 0x00010000);
	_isf_audenc_base.this = MAKEFOURCC('A','E','N','C');
	isf_data_regclass(&_isf_audenc_base);

	return E_OK;
}

ISF_UNIT isf_audenc;
static ISF_PORT_PATH ISF_AudEnc_PathList[ISF_AUDENC_OUT_NUM] = {
	{&isf_audenc, ISF_IN(0),  ISF_OUT(0)},
	{&isf_audenc, ISF_IN(1),  ISF_OUT(1)},
	{&isf_audenc, ISF_IN(2),  ISF_OUT(2)},
	{&isf_audenc, ISF_IN(3),  ISF_OUT(3)},
};

ISF_UNIT isf_audenc = {
	.sign = ISF_SIGN_UNIT,
	.unit_name = "audenc",
	.unit_module = MAKE_NVTMPP_MODULE('a', 'u', 'd', 'e', 'n', 'c', 0, 0),
	.list_id = 1, // 1=not support Multi-Process
	.port_in_count = ISF_AUDENC_IN_NUM,
	.port_out_count = ISF_AUDENC_OUT_NUM,
	.port_path_count = ISF_AUDENC_IN_NUM,
	.port_in = ISF_AudEnc_InputPortList,
	.port_out = ISF_AudEnc_OutputPortList,
	.port_outcfg = ISF_AudEnc_OutputPortList_Cfg,
	.port_incaps = ISF_AudEnc_InputPortList_Caps,
	.port_outcaps = ISF_AudEnc_OutputPortList_Caps,
	.port_outstate = ISF_AudEnc_OutputPortList_State,
	.port_ininfo = isf_audenc_outputinfolist_in,
	.port_outinfo = isf_audenc_outputinfolist_out,
	.port_path = ISF_AudEnc_PathList,
	.do_bindinput = ISF_AudEnc_BindInput,
	.do_bindoutput = ISF_AudEnc_BindOutput,
#if 0
	.SetParam = ISF_AudEnc_SetParam,
	.GetParam = ISF_AudEnc_GetParam,
#endif
	.do_setportparam = ISF_AudEnc_SetPortParam,
	.do_getportparam = ISF_AudEnc_GetPortParam,
	.do_setportstruct = ISF_AudEnc_SetPortStruct,
	.do_getportstruct = ISF_AudEnc_GetPortStruct,
	.do_update = ISF_AudEnc_UpdatePort,
	.do_command = _isf_audenc_do_command,
};

///////////////////////////////////////////////////////////////////////////////

