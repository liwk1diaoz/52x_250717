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
#include "kflow_audiodec/isf_auddec.h"
#include "audio_decode.h"
#include "audio_codec_pcm.h"
#include "audio_codec_aac.h"
#include "audio_codec_alaw.h"
#include "audio_codec_ulaw.h"
#include "nmediaplay_auddec.h"
#include "nmediaplay_api.h"
#include "isf_auddec_dbg.h"
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
#include "kflow_audiodec/isf_auddec.h"
#include "audio_decode.h"
#include "audio_codec_pcm.h"
#include "audio_codec_aac.h"
#include "audio_codec_alaw.h"
#include "audio_codec_ulaw.h"
#include "nmediaplay_auddec.h"
#include "nmediaplay_api.h"
#include "isf_auddec_dbg.h"

extern BOOL _isf_auddec_api_info(CHAR *strCmd);

#define  msecs_to_jiffies(x)	0
#endif

#if _TODO
// Perf_GetCurrent         : use do_gettimeofday() to implement
// nvtmpp_vb_block2addr    : use nvtmpp_vb_blk2va() ??
// UpdatePortInfo          : use hdal new method
// wait untill AudOut return data size: use another method to implement
// UpdatePortInfo          : use hdal new method
// ISF_UNIT_TRACE          : define NONE first ( isf_flow doesn't EXPORT_SYMBOL this , also, if it indeed EXPORT_SYMBOL, it says disagree with version )
// release lock BS data    : lock & unlock input bs data
// remove file play mode   : remove g_bISF_AudDec_FilePlayMode related flow
#else
#define Perf_GetCurrent(args...)            0

#define nvtmpp_vb_block2addr(args...)       nvtmpp_vb_blk2va(args)

#define TIMESTAMP_TODO                      0
#define AUDFMT_TODO                         0

#define ISF_UNIT_TRACE(opclass, p_thisunit, port, fmtstr, args...) (&isf_auddec)->p_base->do_trace(p_thisunit, port, opclass, fmtstr, ##args)
#endif

#if defined (DEBUG) && defined(__FREERTOS)
unsigned int isf_auddec_debug_level = THIS_DBGLVL;
#endif

// fake get_tid() function
static unsigned int get_tid ( signed int *p_tskid )
{
    *p_tskid = -99;
    return 0;
}

// flags
#define FLG_ISF_AUDDEC_AUDDONE       FLGPTN_BIT(0)

#define ISF_AUDDEC_IN_NUM           4
#define ISF_AUDDEC_OUT_NUM          4
#define AUDDEC_MAX_PATH_NUM         4
#define ISF_AUDDEC_DATA_QUE_NUM		3
#define ISF_AUDDEC_RAWDATA_BLK_MAX	256

#define ISF_AUDDEC_PULL_Q_MAX       60

#define ISF_AUDDEC_MSG_BS           ((UINT32)0x00000001 << 0)   //0x00000001
#define ISF_AUDDEC_MSG_LOCK         ((UINT32)0x00000001 << 1)   //0x00000002
#define ISF_AUDDEC_MSG_DROP         ((UINT32)0x00000001 << 2)   //0x00000004
#define ISF_AUDDEC_MSG_PULLQ        ((UINT32)0x00000001 << 3)   //0x00000008

#define DUMP_TO_FILE_TEST_KERNEL_VER        DISABLE         // pure linux version
#define SUPPORT_PULL_FUNCTION               ENABLE
#define SUPPORT_PUSH_FUNCTION               ENABLE

#if defined (__KERNEL__)
#if DUMP_TO_FILE_TEST_KERNEL_VER
#include <kwrap/file.h>
#endif
#endif

typedef struct {
	UINT32           refCnt;
	UINT32           h_data;
	MEM_RANGE		 mem;
	UINT32           blk_id;
	void            *pnext_rawdata;
} AUDDEC_RAWDATA;

typedef struct _AUDDEC_PULL_QUEUE {
	UINT32          Front;                  ///< Front pointer
	UINT32          Rear;                   ///< Rear pointer
	UINT32          bFull;                  ///< Full flag
	ISF_DATA        Queue[ISF_AUDDEC_PULL_Q_MAX];
} AUDDEC_PULL_QUEUE, *PAUDDEC_PULL_QUEUE;

typedef struct _AUDDEC_MMAP_MEM_INFO {
	UINT32          addr_virtual;           ///< memory start addr (virtual)
	UINT32          addr_physical;          ///< memory start addr (physical)
	UINT32          size;                   ///< size
} AUDDEC_MMAP_MEM_INFO, *PAUDDEC_MMAP_MEM_INFO;

// debug
static UINT32               gISF_AudDecMsgLevel[ISF_AUDDEC_OUT_NUM]             = {0};

// ISF info
NMI_UNIT					*pNMI_AudDec										= NULL;
static BOOL                 g_bISF_AudDec_FilePlayMode                          = FALSE;
static UINT32               gISF_AudDec_Open_Count                              = 0;

// Raw data (ouput)
static ISF_DATA             gISF_AudDecRawData;
static ISF_DATA             gISF_AudDec_RawData_MemBlk = {0};
static AUDDEC_RAWDATA       *g_p_auddec_rawdata_blk_pool                        = NULL;
static UINT32               *g_p_auddec_rawdata_blk_maptlb                      = NULL;

// Audio decode info
static UINT32				gISF_AudCodecFmt									= AUDDEC_DECODER_ULAW;
static UINT32				gISF_AudSampleRate									= AUDIO_SR_48000;		// default = 48K
static UINT32				gISF_AudChannel										= AUDIO_CH_STEREO;		// default = stereo
static UINT32				gISF_BitsPerSample									= 16;					// default = 16
static BOOL                 isf_auddec_init                                     = FALSE;

typedef struct _AUDDEC_CTX_MEM {
	ISF_DATA        ctx_memblk;
	UINT32          ctx_addr;
	UINT32          isf_auddec_size;
	UINT32          nm_auddec_obj_size;
	UINT32          nm_auddec_cfg_size;
	UINT32          mp_obj_size;
} AUDDEC_CTX_MEM;

typedef struct _AUDDEC_CONTEXT_COMMON {
	// CONTEXT
	AUDDEC_CTX_MEM         ctx_mem;   // alloc context memory for isf_audenc + nmedia
} AUDDEC_CONTEXT_COMMON;

typedef struct _AUDDEC_CONTEXT_PORT {
	UINT32               gISF_AudDecMaxMemSize;
	NMI_AUDDEC_MEM_RANGE gISF_AudDecMaxMem;
	ISF_DATA             gISF_AudDecMaxMemBlk;
	ISF_DATA             gISF_AudDecMemBlk;
	AUD_FRAME            gISF_AFrameBuf;


	UINT32               gISF_AudDecDropCount;
	AUDDEC_PULL_QUEUE    gISF_AudDec_PullQueue;
	AUDDEC_MMAP_MEM_INFO gISF_AudDecMem_MmapInfo;
	ISF_DATA             *gISF_AudDec_BsData;
	ISF_DATA             gISF_AudDec_BsData_MemBlk;
	BOOL                 gISF_AudDec_IsBsUsed[ISF_AUDDEC_DATA_QUE_NUM];


	AUDDEC_RAWDATA       *g_p_auddec_rawdata_link_head;
	AUDDEC_RAWDATA       *g_p_auddec_rawdata_link_tail;

	UINT32               gISF_AudDec_Started;
	BOOL                 gISF_AudDec_Opened;
	BOOL                 isf_auddec_param_dirty;
	BOOL                 gISF_AudDec_Is_AllocMem;
	SEM_HANDLE           ISF_AUDDEC_SEM_ID;
	SEM_HANDLE           ISF_AUDDEC_PULLQ_SEM_ID;
	BOOL                 put_pullq;
} AUDDEC_CONTEXT_PORT;

typedef struct _AUDDEC_CONTEXT {
	AUDDEC_CONTEXT_COMMON  comm;    // common variable for all port
	AUDDEC_CONTEXT_PORT   *port;    // individual variable for each port
} AUDDEC_CONTEXT;


#if 0
static UINT32               gISF_AudDecMaxMemSize[ISF_AUDDEC_OUT_NUM]           = {0};
static NMI_AUDDEC_MEM_RANGE gISF_AudDecMaxMem[ISF_AUDDEC_OUT_NUM]               = {0};
static ISF_DATA             gISF_AudDecMaxMemBlk[ISF_AUDDEC_OUT_NUM]            = {0};
static ISF_DATA             gISF_AudDecMemBlk[ISF_AUDDEC_OUT_NUM]               = {0};
static AUD_FRAME            gISF_AFrameBuf[ISF_AUDDEC_OUT_NUM]                  = {0};


static UINT32                 gISF_AudDecDropCount[ISF_AUDDEC_IN_NUM]           = {0};
static AUDDEC_PULL_QUEUE      gISF_AudDec_PullQueue[ISF_AUDDEC_OUT_NUM]         = {0};
static AUDDEC_MMAP_MEM_INFO   gISF_AudDecMem_MmapInfo[ISF_AUDDEC_OUT_NUM]       = {0};
static ISF_DATA             *gISF_AudDec_BsData[ISF_AUDDEC_IN_NUM]              = {0};
static ISF_DATA             gISF_AudDec_BsData_MemBlk[ISF_AUDDEC_IN_NUM]        = {0};
static BOOL                 gISF_AudDec_IsBsUsed[ISF_AUDDEC_IN_NUM][ISF_AUDDEC_DATA_QUE_NUM] = {0};

static AUDDEC_RAWDATA       *g_p_auddec_rawdata_link_head[ISF_AUDDEC_OUT_NUM]    = {0};
static AUDDEC_RAWDATA       *g_p_auddec_rawdata_link_tail[ISF_AUDDEC_OUT_NUM]    = {0};

static UINT32               gISF_AudDec_Started[ISF_AUDDEC_OUT_NUM]             = {0};
static BOOL                 gISF_AudDec_Opened[ISF_AUDDEC_OUT_NUM]              = {0};
static BOOL                 isf_auddec_param_dirty[ISF_AUDDEC_OUT_NUM]          = {0};
static BOOL                 gISF_AudDec_Is_AllocMem[ISF_AUDDEC_OUT_NUM]         = {0};
SEM_HANDLE                  ISF_AUDDEC_SEM_ID[ISF_AUDDEC_OUT_NUM] = {0};
SEM_HANDLE                  ISF_AUDDEC_PULLQ_SEM_ID[ISF_AUDDEC_OUT_NUM]     = {0};
#endif

static void 				_ISF_AudDec_InstallCmd(void);
static ISF_RV               _isf_auddec_raw_data_init(void);
static ISF_DATA_CLASS       _isf_auddec_base;

AUDDEC_CONTEXT g_ad_ctx = {0};

SEM_HANDLE                  ISF_AUDDEC_COMMON_SEM_ID							= {0};
SEM_HANDLE                  ISF_AUDDEC_PROC_SEM_ID                              = {0};

// multi-process
static UINT32               g_auddec_init[ISF_FLOW_MAX]                         = {0};

#if (SUPPORT_PULL_FUNCTION)

#endif


#if (DUMP_TO_FILE_TEST_KERNEL_VER == ENABLE)
static int g_wo_fd = 0;
static INT32 dump_sample_cnt = 150;            // record 5 seconds (30 samples/sec)
static UINT8 temp_buf[6000000] = {0};
static UINT32 temp_cnt = 0;
#endif

#if (SUPPORT_PULL_FUNCTION)
#define AUDDEC_VIRT2PHY(pathID, virt_addr) g_ad_ctx.port[pathID].gISF_AudDecMem_MmapInfo.addr_physical + (virt_addr - g_ad_ctx.port[pathID].gISF_AudDecMem_MmapInfo.addr_virtual)

BOOL _ISF_AudDec_isEmpty_PullQueue(UINT32 pathID);
BOOL _ISF_AudDec_isFull_PullQueue(UINT32 pathID);
void _ISF_AudDec_Unlock_PullQueue(UINT32 pathID);
BOOL _ISF_AudDec_Put_PullQueue(UINT32 pathID, ISF_DATA *data_in);
ISF_RV _ISF_AudDec_Get_PullQueue(UINT32 pathID, ISF_DATA *data_out, INT32 wait_ms);
BOOL _ISF_AudDec_RelaseAll_PullQueue(UINT32 pathID);
static ISF_RV _ISF_AudDec_Close(ISF_UNIT *pThisUnit, UINT32 id, UINT32 b_do_release_i);
#endif

static AUDDEC_RAWDATA *_ISF_AudDec_GET_RAWDATA(void)
{
	UINT32 i = 0;
	AUDDEC_RAWDATA *pData = NULL;

	SEM_WAIT(ISF_AUDDEC_COMMON_SEM_ID);
	while (g_p_auddec_rawdata_blk_maptlb[i]) {
		i++;
		if (i >= (PATH_MAX_COUNT * ISF_AUDDEC_RAWDATA_BLK_MAX)) {
			//DBG_ERR("No free block!!!\r\n");
			SEM_SIGNAL(ISF_AUDDEC_COMMON_SEM_ID);
			return NULL;
		}
	}

	pData = (AUDDEC_RAWDATA *)(g_p_auddec_rawdata_blk_pool + i);

	if (pData) {
		pData->pnext_rawdata = NULL;
		pData->blk_id = i;
		g_p_auddec_rawdata_blk_maptlb[i] = TRUE;
	}
	SEM_SIGNAL(ISF_AUDDEC_COMMON_SEM_ID);
	return pData;
}

// [ToDo]
static void _ISF_AudDec_FREE_BSDATA(AUDDEC_RAWDATA *pData)
{
	if (pData) {
		SEM_WAIT(ISF_AUDDEC_COMMON_SEM_ID);
		g_p_auddec_rawdata_blk_maptlb[pData->blk_id] = FALSE;
		pData->h_data = 0;
		SEM_SIGNAL(ISF_AUDDEC_COMMON_SEM_ID);
	}
}

static ISF_RV _ISF_AudDec_LockCB(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data)
{
#if 0 // old version
	UINT32 i = 0;
	AUDDEC_RAWDATA *pAudDecData = NULL;

	// search AUDDEC_RAWDATA link list by h_data
	for (i = 0; i < ISF_AUDDEC_OUT_NUM; i++) {
		pAudDecData = &g_auddec_rawdata[i];

		if (pAudDecData != NULL) {
			pAudDecData->refCnt++;
			ISF_AUDDEC_DUMP("refCnt++, %s %d\r\n", p_thisunit->unit_name, pAudDecData->refCnt);
		}
	}
	return ISF_OK;
#else
	UINT32 i = 0;
	AUDDEC_RAWDATA *pAudDecData = NULL;
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
		DBG_ERR("[ISF_AUDDEC] LOCK tid = %d h_data is NULL\r\n", tid);
		return ISF_ERR_INVALID_VALUE;
	}

	// search AUDDEC_RAWDATA link list by h_data
	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDDEC_CONTEXT_PORT   *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[i];
		AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

		if (p_ctx_c->ctx_mem.ctx_addr == 0) {
			return ISF_ERR_UNINIT;
		}

		SEM_WAIT(p_ctx_p->ISF_AUDDEC_SEM_ID);
		pAudDecData = p_ctx_p->g_p_auddec_rawdata_link_head;

		while (pAudDecData != NULL) {
			if (pAudDecData->blk_id == ((h_data - 1) & 0xFF)) {
				pAudDecData->refCnt++;
				if (gISF_AudDecMsgLevel[i] & ISF_AUDDEC_MSG_LOCK) {
					DBG_DUMP("[ISF_AUDDEC][%d] LOCK %s ref %d hData 0x%x\r\n", i, unit_name, pAudDecData->refCnt, pAudDecData->h_data);
				}
				SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);
				return ISF_OK;
			}

			pAudDecData = pAudDecData->pnext_rawdata;
		}
		SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);
	}
	return ISF_OK;
#endif
}

static ISF_RV _ISF_AudDec_UnLockCB(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data)
{
#if 0 // old version
	UINT32 i = 0;
	AUDDEC_RAWDATA *pAudDecData = NULL;

	// search AUDDEC_RAWDATA link list by h_data
	for (i = 0; i < ISF_AUDDEC_OUT_NUM; i++) {
		pAudDecData = &g_auddec_rawdata[i];

		if (pAudDecData != NULL) {
			pAudDecData->refCnt--;
			ISF_AUDDEC_DUMP("refCnt--, %s %d\r\n", p_thisunit->unit_name, pAudDecData->refCnt);
		}
	}
	return ISF_OK;
#else
	UINT32 i = 0;
	AUDDEC_RAWDATA *pAudDecData = NULL;
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
		DBG_ERR("[ISF_AUDDEC] UnLOCK tid = %d h_data is NULL\r\n", tid);
		return ISF_ERR_INVALID_VALUE;
	}

	// search AUDDEC_BSDATA link list by h_data
	for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDDEC_CONTEXT_PORT   *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[i];
		AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

		if (p_ctx_c->ctx_mem.ctx_addr == 0) {
			return ISF_ERR_UNINIT;
		}

		SEM_WAIT(p_ctx_p->ISF_AUDDEC_SEM_ID);
		pAudDecData = p_ctx_p->g_p_auddec_rawdata_link_head;

		while (pAudDecData != NULL) {
			if (pAudDecData->blk_id == ((h_data - 1) & 0xFF)) {
				if (pAudDecData->refCnt > 0) {
					pAudDecData->refCnt--;
				} else {
					get_tid(&tid);
					if (p_thisunit) {
						DBG_ERR("[ISF_AUDDEC][%d] UnLOCK %s tid = %d refCnt is 0\r\n", i, unit_name, tid);
					} else {
						DBG_ERR("[ISF_AUDDEC][%d] UnLOCK NULL tid = %d refCnt is 0\r\n", i, tid);
					}
					SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);
					return ISF_ERR_INVALID_VALUE;
				}

				if (gISF_AudDecMsgLevel[i] & ISF_AUDDEC_MSG_LOCK) {
					DBG_DUMP("[ISF_AUDDEC][%d] UNLOCK %s ref %d hData 0x%x\r\n", i, unit_name, pAudDecData->refCnt, pAudDecData->h_data);
				}

				if (pAudDecData->refCnt == 0) {
					(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(i),  ISF_USER_PROBE_REL, ISF_ENTER);  // [USER] REL_ENTER

					// Set BS data address is unlock to NMedia
					//pNMI_AudDec->SetParam(i, NMI_AUDDEC_PARAM_UNLOCK_BS_ADDR, pAudDecData->h_data); // [ToDo]

					if (pAudDecData != p_ctx_p->g_p_auddec_rawdata_link_head) {
						AUDDEC_RAWDATA *pAudDecData2 = NULL;
						get_tid(&tid);
						if (gISF_AudDecMsgLevel[i] & ISF_AUDDEC_MSG_LOCK) {
							DBG_ERR("[ISF_AUDDEC][%d] UnLOCK %s, tid = %d, not first data in queue!! 1st link data = (0x%x, 0x%x, %d), rel data = (0x%x, 0x%x, %d)\r\n",
								i,
								unit_name,
								tid,
								(UINT32)p_ctx_p->g_p_auddec_rawdata_link_head,
								(p_ctx_p->g_p_auddec_rawdata_link_head?p_ctx_p->g_p_auddec_rawdata_link_head->h_data:0),
								(p_ctx_p->g_p_auddec_rawdata_link_head?p_ctx_p->g_p_auddec_rawdata_link_head->blk_id:0),
								(UINT32)pAudDecData,
								pAudDecData->h_data,
								pAudDecData->blk_id);
						}

						// search for release
						pAudDecData2 = p_ctx_p->g_p_auddec_rawdata_link_head;

						while (pAudDecData2 != NULL) {
							if (pAudDecData2->pnext_rawdata == pAudDecData) {
								pAudDecData2->pnext_rawdata = pAudDecData->pnext_rawdata;
								if (pAudDecData2->pnext_rawdata == NULL) {
									p_ctx_p->g_p_auddec_rawdata_link_tail = pAudDecData2;
								}
								break;
							}

							pAudDecData2 = pAudDecData2->pnext_rawdata;
						}

						// free block
						_ISF_AudDec_FREE_BSDATA(pAudDecData);

						(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(i),  ISF_USER_PROBE_REL, ISF_OK);  // [USER] REL_OK ( with some error, but actually released )

					} else {
						// update link head
						p_ctx_p->g_p_auddec_rawdata_link_head = p_ctx_p->g_p_auddec_rawdata_link_head->pnext_rawdata;

						// free block
						_ISF_AudDec_FREE_BSDATA(pAudDecData);

						(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(i),  ISF_USER_PROBE_REL, ISF_OK);  // [USER] REL_OK
					}

					if (gISF_AudDecMsgLevel[i] & ISF_AUDDEC_MSG_LOCK) {
						DBG_DUMP("[ISF_AUDDEC][%d] Free h_data = 0x%x, blk_id = %d\r\n", i, pAudDecData->h_data, pAudDecData->blk_id);
					}
				}
				SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);
				return ISF_OK;
			}

			pAudDecData = pAudDecData->pnext_rawdata;
		}
		SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);
	}
	return ISF_OK;
#endif
}

static UINT32 _ISF_AudDec_RawReadyCB(NMI_AUDDEC_RAW_INFO *pMem)
{
	ISF_DATA                *pRawData       = &gISF_AudDecRawData; // input
	AUDDEC_CONTEXT_PORT     *p_ctx_p        = NULL;
	//ISF_PORT				*pDestPort		= isf_auddec.p_base->oport(&isf_auddec, ISF_OUT(0));
	PAUD_FRAME               pAFrameBuf     = NULL;
	ISF_DATA                *pBsData        = NULL; // output
	AUDDEC_RAWDATA          *pAudDecData    = NULL;
	ISF_PORT                *p_port         = NULL;
	ISF_RV                   rv             = ISF_OK;

#if (DUMP_TO_FILE_TEST_KERNEL_VER == ENABLE)
	if (!((VOS_FILE)(-1) == g_wo_fd) && dump_sample_cnt > 0 && temp_cnt < 6000000) {
		// WRITE
		DBG_DUMP("=====>>> mem copy, a=0x%x, s=0x%x, cnt=%d\r\n", pMem->Addr, pMem->Size, dump_sample_cnt);
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
		DBG_DUMP("===============================>>> write file finished!!!!!\r\n");
		dump_sample_cnt = -1;
	}
#endif

	if (pMem->PathID >= PATH_MAX_COUNT) {
		return ISF_ERR_INVALID_PORT_ID;
	}

	p_ctx_p    = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[pMem->PathID];
	pAFrameBuf = &(p_ctx_p->gISF_AFrameBuf);

	// release lock BS data
	if (pMem->Occupied == 0) {
		// get BS_ISF_DATA queue
		pBsData = &(p_ctx_p->gISF_AudDec_BsData[pMem->BufID]);

		DBG_IND("[ISF_AUDDEC][%d][%d] callback Release blk_id = 0x%08x, addr = 0x%08x, time = %d us\r\n", pMem->PathID, pMem->BufID, pBsData->h_data, nvtmpp_vb_block2addr(pBsData->h_data), Perf_GetCurrent());

		if (pMem->Addr != 0) {
			(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(pMem->PathID), ISF_IN_PROBE_REL, ISF_ENTER); // [IN] REL_ENTER   // bitstream is actually decoded, addr !=0
		}

		rv = isf_auddec.p_base->do_release(&isf_auddec, ISF_IN(pMem->PathID), pBsData, ISF_IN_PROBE_REL);

		if ((pMem->Addr != 0) && (rv == ISF_OK)) {
			(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(pMem->PathID), ISF_IN_PROBE_REL, ISF_OK); // [IN] REL_OK  // bitstream is actually decoded, addr !=0
		}

		// set BS_ISF_DATA queue empty
		SEM_WAIT(p_ctx_p->ISF_AUDDEC_SEM_ID);
		p_ctx_p->gISF_AudDec_IsBsUsed[pMem->BufID] = FALSE;
		SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);
	}

	//always push or put to pull queue to notify decode error
	/*if (pMem->Addr == 0) {
		DBG_ERR("pMem->Addr = 0\r\n");
		return ISF_OK;
	}*/

	(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH, ISF_ENTER); // [OUT] PUSH_ENTER

	{
		pAFrameBuf->sign = MAKEFOURCC('A', 'R', 'A', 'W');
		pAFrameBuf->addr[0] = pMem->Addr;
		pAFrameBuf->size = pMem->Size;
		pAFrameBuf->sound_mode = gISF_AudChannel;
		pAFrameBuf->sample_rate = gISF_AudSampleRate;
		pAFrameBuf->bit_width = gISF_BitsPerSample;
		pAFrameBuf->timestamp = pMem->TimeStamp;

#if (SUPPORT_PULL_FUNCTION)
		// convert to physical addr
		pAFrameBuf->phyaddr[0] = AUDDEC_VIRT2PHY(pMem->PathID, pMem->Addr);  // frm phy
#endif

		// get AUDDEC_RAWDATA
		pAudDecData = _ISF_AudDec_GET_RAWDATA();

		if (pAudDecData) {
			pAudDecData->h_data = pMem->Addr;
			pAudDecData->mem.addr = pMem->Addr;
			pAudDecData->mem.size = pMem->Size;

			pAudDecData->refCnt = 0;
			pAudDecData->refCnt += 1;
		} else {
			isf_auddec.p_base->do_new(&isf_auddec, ISF_OUT(pMem->PathID), NVTMPP_VB_INVALID_POOL, 0, 0, pRawData, ISF_OUT_PROBE_NEW);
			(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [OUT] PUSH_WRN
			DBG_IND("Port %d _ISF_AudDec_NEW_RAWDATA is NULL!\r\n", pMem->PathID);
			// Set RAW data address to unlock NMedia for this fail case
			//pNMI_AudDec->SetParam(pMem->PathID, NMI_AUDDEC_PARAM_UNLOCK_RAW_ADDR, pMem->Addr); // [ToDo]
			return ISF_ERR_QUEUE_FULL;
		}

		// add AUDDEC_RAWDATA link list
		SEM_WAIT(p_ctx_p->ISF_AUDDEC_SEM_ID);
		if (p_ctx_p->g_p_auddec_rawdata_link_head == NULL) {
			p_ctx_p->g_p_auddec_rawdata_link_head = pAudDecData;
			p_ctx_p->g_p_auddec_rawdata_link_tail = p_ctx_p->g_p_auddec_rawdata_link_head;
		} else {
			p_ctx_p->g_p_auddec_rawdata_link_tail->pnext_rawdata = pAudDecData;
			p_ctx_p->g_p_auddec_rawdata_link_tail = p_ctx_p->g_p_auddec_rawdata_link_tail->pnext_rawdata;
		}
		SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);

		// Set Raw ISF_DATA info
		memcpy(pRawData->desc, pAFrameBuf, sizeof(AUD_FRAME));
		pRawData->h_data = ((pAudDecData->blk_id + 1) & 0xFF);
		//new pData by user
		isf_auddec.p_base->do_new(&isf_auddec, ISF_OUT(pMem->PathID), NVTMPP_VB_INVALID_POOL, pRawData->h_data, 0, pRawData, ISF_OUT_PROBE_NEW);

		p_port = isf_auddec.port_out[ISF_OUT(pMem->PathID)];
		if (isf_auddec.p_base->get_is_allow_push(&isf_auddec, p_port)) {
			if ((pAFrameBuf->addr[0] != 0) && (pAFrameBuf->size != 0)) {
				rv = isf_auddec.p_base->do_push(&isf_auddec, ISF_OUT(pMem->PathID), &gISF_AudDecRawData, 0);
				if (rv == ISF_OK) {
					(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH, ISF_OK); // [OUT] PUSH_OK
				} else {
					(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_PROCESS_FAIL); // [OUT] PUSH_ERR
				}
			} else {
				(&isf_auddec)->p_base->do_release(&isf_auddec, ISF_OUT(pMem->PathID), pRawData, 0);
				(&isf_auddec)->p_base->do_release(&isf_auddec, ISF_OUT(pMem->PathID), 0, ISF_ERR_INACTIVE_STATE);
				(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_INVALID_DATA); // [OUT] PUSH_WRN
			}
		}
#if (SUPPORT_PULL_FUNCTION)
		else { // add to pull queue
			if (TRUE ==_ISF_AudDec_Put_PullQueue(pMem->PathID, pRawData)) {
				(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH, ISF_OK); // [OUT] PUSH_OK

				if ((pAFrameBuf->addr[0] == 0) && (pAFrameBuf->size == 0)) {
					// this will be treated as a decoded error
					(&isf_auddec)->p_base->do_release(&isf_auddec, ISF_OUT(pMem->PathID), pRawData, 0);
				}
			} else {
				(&isf_auddec)->p_base->do_release(&isf_auddec, ISF_OUT(pMem->PathID), pRawData, ISF_OUT_PROBE_PUSH);
				(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [OUT] PUSH_WRN
			}
		}
#endif
	}

	return ISF_OK;
}

static void _ISF_AudDec_PlayCB(UINT32 uiEventID, UINT32 param)
{
	switch (uiEventID) {
		case NMI_AUDDEC_EVENT_RAW_CB:
			_ISF_AudDec_RawReadyCB((NMI_AUDDEC_RAW_INFO *) param);
			break;
		default:
			break;
	}
}

static ISF_RV _ISF_AudDec_UpdatePortInfo(ISF_UNIT *pThisUnit, UINT32 oPort)
{
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

	return ISF_OK;
}

static ISF_RV _ISF_AudDec_FreeMem_RawOnly(ISF_UNIT *pThisUnit, UINT32 id)
{
	AUDDEC_RAWDATA *pAudDecData = NULL;
	AUDDEC_CONTEXT_PORT *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[id];
	// release left blocks
	pAudDecData = p_ctx_p->g_p_auddec_rawdata_link_head;
	while (pAudDecData != NULL) {
		_ISF_AudDec_FREE_BSDATA(pAudDecData);
		pAudDecData = pAudDecData->pnext_rawdata;
	}
	SEM_WAIT(p_ctx_p->ISF_AUDDEC_SEM_ID);
	p_ctx_p->g_p_auddec_rawdata_link_head = NULL;
	p_ctx_p->g_p_auddec_rawdata_link_tail = NULL;
	p_ctx_p->put_pullq = FALSE;
	SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);

	// release pull queue
	_ISF_AudDec_RelaseAll_PullQueue(id);

	return ISF_OK;
}

static ISF_RV _ISF_AudDec_AllocMem(ISF_UNIT *pThisUnit, UINT32 id)
{
	NMI_AUDDEC_MEM_RANGE	Mem			= {0};
	UINT32					uiBufAddr	= 0;
	UINT32					uiBufSize	= 0;
   	//ISF_PORT* 	            pSrcPort    = _isf_unit_out(pThisUnit, ISF_OUT(id));
   	ISF_PORT*               pSrcPort    = isf_auddec.p_base->oport(pThisUnit, ISF_OUT(id));
	AUDDEC_CONTEXT_PORT     *p_ctx_p        = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[id];

	pNMI_AudDec->GetParam(id, NMI_AUDDEC_PARAM_ALLOC_SIZE, &uiBufSize);

	//if (uiBufSize == 0) {
	if (uiBufSize == 0 && p_ctx_p->gISF_AudDecMaxMemSize == 0) { // neither run time param nor max mem info
		DBG_ERR("Get port %d buffer from NMI_AudDec is Zero!\r\n", id);
		return ISF_ERR_INVALID_VALUE;
	}

    // use max info to create buffer
    if (p_ctx_p->gISF_AudDecMaxMemSize != 0 && p_ctx_p->gISF_AudDecMaxMem.Addr == 0) {
		uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "max", p_ctx_p->gISF_AudDecMaxMemSize, &(p_ctx_p->gISF_AudDecMaxMemBlk));
		if (uiBufAddr != 0) {
			p_ctx_p->gISF_AudDecMaxMem.Addr = uiBufAddr;
			p_ctx_p->gISF_AudDecMaxMem.Size = p_ctx_p->gISF_AudDecMaxMemSize;
#if (SUPPORT_PULL_FUNCTION)
			// set mmap info
			p_ctx_p->gISF_AudDecMem_MmapInfo.addr_virtual  = uiBufAddr;
			p_ctx_p->gISF_AudDecMem_MmapInfo.addr_physical = nvtmpp_vb_blk2pa(p_ctx_p->gISF_AudDecMaxMemBlk.h_data);
			p_ctx_p->gISF_AudDecMem_MmapInfo.size          = p_ctx_p->gISF_AudDecMaxMemSize;
#endif
		} else {
			DBG_ERR("[ISF_AUDDEC][%d] get max blk failed!\r\n", id);
			return ISF_ERR_BUFFER_CREATE;
		}
	}

	if (p_ctx_p->gISF_AudDecMaxMem.Addr != 0 && p_ctx_p->gISF_AudDecMaxMem.Size != 0) { // use max buf
		//DBG_DUMP("[ISF_AUDDEC][%d] set dec buf from max addr = 0x%08x, size = %d\r\n", id, gISF_AudDecMaxMem[id].Addr, gISF_AudDecMaxMem[id].Size);
		if (uiBufSize > p_ctx_p->gISF_AudDecMaxMem.Size) {
			DBG_ERR("[ISF_AUDDEC][%d] Need mem size (%d) > max public size (%d)\r\n", id, uiBufSize, p_ctx_p->gISF_AudDecMaxMem.Size);
			pNMI_AudDec->SetParam(id, NMI_AUDDEC_PARAM_MEM_RANGE, (UINT32) &Mem);  //reset dec buf addr to 0
			return ISF_ERR_NEW_FAIL;
		}
		Mem.Addr = p_ctx_p->gISF_AudDecMaxMem.Addr;
		Mem.Size = p_ctx_p->gISF_AudDecMaxMem.Size;
	} else if (uiBufSize > 0) { // create buf from nvtmpp
		//DBG_DUMP("[ISF_AUDDEC][%d] create buffer by nvtmpp size = %d\r\n", id, uiBufSize);
		// Get blk from mempool
		uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "work", uiBufSize, &(p_ctx_p->gISF_AudDecMemBlk));
		if (uiBufAddr == 0) {
			DBG_ERR("%s get blk (%d) failed!\r\n", pThisUnit->unit_name, Mem.Size);
			return ISF_ERR_BUFFER_CREATE;
		}
		Mem.Addr = uiBufAddr;
		Mem.Size = uiBufSize;
#if (SUPPORT_PULL_FUNCTION)
		// set mmap info
		p_ctx_p->gISF_AudDecMem_MmapInfo.addr_virtual  = uiBufAddr;
		p_ctx_p->gISF_AudDecMem_MmapInfo.addr_physical = nvtmpp_vb_blk2pa(p_ctx_p->gISF_AudDecMemBlk.h_data);
		p_ctx_p->gISF_AudDecMem_MmapInfo.size          = uiBufSize;
#endif
	}
	pNMI_AudDec->SetParam(id, NMI_AUDDEC_PARAM_MEM_RANGE, (UINT32) &Mem);

	// create bs queue (input)
	uiBufSize = sizeof(ISF_DATA) * ISF_AUDDEC_DATA_QUE_NUM;
	uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "bs", uiBufSize, &(p_ctx_p->gISF_AudDec_BsData_MemBlk));
	if (uiBufAddr == 0) {
		DBG_ERR("[ISF_AUDDEC][%d] get bs blk failed!\r\n", id);
		return ISF_ERR_BUFFER_CREATE;
	}
	p_ctx_p->gISF_AudDec_BsData = (ISF_DATA *) uiBufAddr;

	// create raw queue (output)
	if (gISF_AudDec_Open_Count == 0) {
		uiBufSize = (sizeof(UINT32) + sizeof(AUDDEC_RAWDATA)) * PATH_MAX_COUNT * ISF_AUDDEC_RAWDATA_BLK_MAX;
		uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, NULL, "raw", uiBufSize, &gISF_AudDec_RawData_MemBlk);
		if (uiBufAddr == 0) {
			DBG_ERR("[ISF_AUDDEC][%d] get raw blk failed!\r\n", id);
			return ISF_ERR_BUFFER_CREATE;
		}
		g_p_auddec_rawdata_blk_maptlb = (UINT32 *) uiBufAddr;
		memset(g_p_auddec_rawdata_blk_maptlb, 0, uiBufSize);
		g_p_auddec_rawdata_blk_pool = (AUDDEC_RAWDATA *)(g_p_auddec_rawdata_blk_maptlb + (PATH_MAX_COUNT * ISF_AUDDEC_RAWDATA_BLK_MAX));
	}
	gISF_AudDec_Open_Count++;

	return ISF_OK;
}

static ISF_RV _ISF_AudDec_FreeMem(ISF_UNIT *pThisUnit, UINT32 id, UINT32 b_do_release_i)
{
	ISF_RV r = ISF_OK;
	//UINT32 i = 0, j = 0;
	//ISF_PORT *pSrcPort = NULL;
	//ISF_DATA *pBsData = NULL;
	AUDDEC_RAWDATA *pAudDecData = NULL;
	AUDDEC_CONTEXT_PORT *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[id];

#if 0 // if user call AUDDEC_PARAM_MAX_MEM_INFO with bRelease = 1,  it indeed release MAX buffer, and MAX addr/size is also set to 0..... this will cause exception here ( don't need to release dynamic alloc buffer )
	if (gISF_AudDecMaxMem[id].Addr == 0 || gISF_AudDecMaxMem[id].Size == 0) {
		// release blk to mempool
		r = pThisUnit->p_base->do_release_i(pThisUnit, &(gISF_AudDecMemBlk[id]), ISF_OK);
		if (r != ISF_OK) {
			DBG_ERR("%s release blk failed! (%d)\r\n", pThisUnit->unit_name, r);
			return ISF_ERR_BUFFER_DESTROY;
		}
	}
#endif

	// release bs queue (input)
	/*for (j = 0; j < PATH_MAX_COUNT; j++) {
		for (i = 0; i < ISF_AUDDEC_DATA_QUE_NUM; i++) {
			AUDDEC_CONTEXT_PORT *p_ctx_pj = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[j];

			//pSrcPort = isf_auddec.p_base->iport(&isf_auddec, ISF_IN(0) + j);
			if (p_ctx_pj->gISF_AudDec_IsBsUsed[i]) {
				pBsData = &(p_ctx_pj->gISF_AudDec_BsData[i]);

				// release lock BS data
				isf_auddec.p_base->do_release(&isf_auddec, ISF_IN(j), pBsData, ISF_IN_PROBE_REL);
				isf_auddec.p_base->do_probe(&isf_auddec, ISF_IN(j), 0, ISF_ERR_INACTIVE_STATE);
			}
		}
	}*/

	// release bs queue
	if (p_ctx_p->gISF_AudDec_BsData_MemBlk.h_data != 0) {
		if (TRUE == b_do_release_i) {
			r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_AudDec_BsData_MemBlk), ISF_OK);
			if (r != ISF_OK) {
				DBG_ERR("[ISF_AUDDEC][%d] release bs blk failed, ret = %d\r\n", id, r);
				return ISF_ERR_BUFFER_DESTROY;
			}
		}
		memset(&(p_ctx_p->gISF_AudDec_BsData_MemBlk), 0, sizeof(ISF_DATA));
	}

	memset(&(p_ctx_p->gISF_AudDec_IsBsUsed), 0, sizeof(p_ctx_p->gISF_AudDec_IsBsUsed));

	// release raw queue (output)
	if (gISF_AudDec_Open_Count > 0)
		gISF_AudDec_Open_Count--;

	// release left blocks
	pAudDecData = p_ctx_p->g_p_auddec_rawdata_link_head;
	while (pAudDecData != NULL) {
		_ISF_AudDec_FREE_BSDATA(pAudDecData);
		pAudDecData = pAudDecData->pnext_rawdata;
	}
	SEM_WAIT(p_ctx_p->ISF_AUDDEC_SEM_ID);
	p_ctx_p->g_p_auddec_rawdata_link_head = NULL;
	p_ctx_p->g_p_auddec_rawdata_link_tail = NULL;
	SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);

    if (gISF_AudDec_Open_Count == 0) {
		if (gISF_AudDec_RawData_MemBlk.h_data != 0) {
			if (TRUE == b_do_release_i) {
				r = pThisUnit->p_base->do_release_i(pThisUnit, &gISF_AudDec_RawData_MemBlk, ISF_OK);
				if (r != ISF_OK) {
					DBG_ERR("[ISF_AUDDEC][%d] release raw blk failed, ret = %d\r\n", id, r);
					return ISF_ERR_BUFFER_DESTROY;
				}
			}
			memset(&gISF_AudDec_RawData_MemBlk, 0, sizeof(ISF_DATA));
		}
		g_p_auddec_rawdata_blk_pool   = NULL;
		g_p_auddec_rawdata_blk_maptlb = NULL;

		{
			UINT32 i;
			for (i=0; i< PATH_MAX_COUNT; i++) {
				p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[i];

				p_ctx_p->g_p_auddec_rawdata_link_head = NULL;	//memset(p_ctx_p->g_p_auddec_rawdata_link_head, 0, sizeof(AUDDEC_RAWDATA *) * ISF_AUDDEC_OUT_NUM);
				p_ctx_p->g_p_auddec_rawdata_link_tail = NULL;	//memset(p_ctx_p->g_p_auddec_rawdata_link_tail, 0, sizeof(AUDDEC_RAWDATA *) * ISF_AUDDEC_OUT_NUM);
			}
		}
	}

	// release pull queue
	_ISF_AudDec_RelaseAll_PullQueue(id);

	return r;
}

static ISF_RV _ISF_AudDec_Reset(ISF_UNIT *pThisUnit, UINT32 oPort)
{
	AUDDEC_CONTEXT_PORT *p_ctx_p = NULL;
	AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		return ISF_ERR_UNINIT; //sitll not init
	}

	if (oPort >= PATH_MAX_COUNT) {
		return ISF_ERR_INVALID_PORT_ID;
	}

	p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[oPort];

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
	if ((oPort - ISF_OUT_BASE) < PATH_MAX_COUNT) {
		ISF_RV r;

		if ((p_ctx_p->gISF_AudDecMaxMem.Addr != 0) && (p_ctx_p->gISF_AudDecMaxMemBlk.h_data != 0)) {
			ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, pThisUnit, ISF_OUT(oPort), "(release max memory)");
			r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_AudDecMaxMemBlk), ISF_OK);
			if (r == ISF_OK) {
				p_ctx_p->gISF_AudDecMaxMem.Addr = 0;
				p_ctx_p->gISF_AudDecMaxMem.Size = 0;
				p_ctx_p->gISF_AudDecMaxMemSize = 0;
				memset(&(p_ctx_p->gISF_AudDecMaxMemBlk), 0, sizeof(ISF_DATA));
			} else {
				DBG_ERR("[ISF_AUDDEC][%d] release AudDec Max blk failed, ret = %d\r\n", (oPort-ISF_OUT_BASE), r);
			}
		}
	} else {
		DBG_ERR("[ISF_AUDDEC] invalid port index = %d\r\n", (oPort-ISF_OUT_BASE));
	}

	//if state still = OPEN (maybe Close failed due to user not release all bs yet, Close will never success...so do the close routine here to RESET )
	//         (but DO NOT call do_release_i() , this will not valid after previous nvtmpp uninit().  nvtmpp will forget everything and re-init on next time init() )
	if (p_ctx_p->gISF_AudDec_Opened == TRUE) {
		ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(reset - close)");
		_ISF_AudDec_Close(pThisUnit, oPort, FALSE);  // FALSE means DO NOT call do_release_i()
		_ISF_AudDec_Unlock_PullQueue(oPort);  // let pull_out(blocking mode) auto-unlock
		p_ctx_p->gISF_AudDec_Opened = FALSE;
	}

	return ISF_OK;
}

static ISF_RV _ISF_AudDec_Open(ISF_UNIT *pThisUnit, UINT32 id)
{
	AUDDEC_CONTEXT_PORT *p_ctx_p;
	AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[id];

	ISF_UNIT_TRACE(ISF_OP_STATE, &isf_auddec, id + ISF_OUT_BASE, "(open)\r\n");

	if (!isf_auddec_init) {
		nvtmpp_vb_add_module(pThisUnit->unit_module);
		_isf_auddec_raw_data_init();
		_ISF_AudDec_InstallCmd();
		NMP_AudDec_AddUnit();
		isf_auddec_init = TRUE;
	}

	// Check whether NMI_AudDec exists?
	if (!pNMI_AudDec) {
		if ((pNMI_AudDec = NMI_GetUnit("AudDec")) == NULL) {
			DBG_ERR("Get AudDec unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	pNMI_AudDec->SetParam(id, NMI_AUDDEC_PARAM_REG_CB, (UINT32)_ISF_AudDec_PlayCB);

	if (pNMI_AudDec->Open(id, 0) != E_OK) {
		return ISF_ERR_NOT_AVAIL;
	}

	p_ctx_p->gISF_AudDec_Opened = TRUE;

	return ISF_OK;
}

static ISF_RV _ISF_AudDec_Close(ISF_UNIT *pThisUnit, UINT32 id, UINT32 b_do_release_i)
{
	UINT32 port_no = id;
	AUDDEC_CONTEXT_PORT *p_ctx_p;
	AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[id];

	ISF_UNIT_TRACE(ISF_OP_STATE, &isf_auddec, id + ISF_OUT_BASE, "(close)\r\n");

	// Check whether NMI_AudDec exists?
	if (!pNMI_AudDec) {
		if ((pNMI_AudDec = NMI_GetUnit("AudDec")) == NULL) {
			DBG_ERR("Get AudDec unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	pNMI_AudDec->Close(port_no);

	if (p_ctx_p->gISF_AudDec_Is_AllocMem == TRUE) {
		// only call FreeMem if it indeed successfully AllocMem before, JIRA : IVOT_N01002_CO-562  ( user may only call open()->close(), never call start()....because AllocMem is doing at start(), so AllocMem never happened !! )
		if (ISF_OK == _ISF_AudDec_FreeMem(pThisUnit, id, b_do_release_i)) {
			p_ctx_p->gISF_AudDec_Is_AllocMem = FALSE;
		} else {
			DBG_ERR("[ISF_AUDDEC][%d] FreeMem failed...!!\r\n", id);
		}
	} else {
		DBG_IND("[ISF_AUDDEC][%d] close without FreeMem\r\n", id);
	}

	// reset variables
	g_bISF_AudDec_FilePlayMode = FALSE;

	return ISF_OK;
}

static ISF_RV ISF_AudDec_InputPort_PushImgBufToDest(ISF_PORT *pPort, ISF_DATA *pData, INT32 nWaitMs)
{
	AUD_BITSTREAM           *pAudBuf = (AUD_BITSTREAM *)(pData->desc);
	NMI_AUDDEC_BS_INFO      readyInfo = {0};
	UINT32					id = (pPort->iport - ISF_IN_BASE);
	UINT32					i = 0;
	ER						ret;
	UINT32                  phy_addr;
	AUDDEC_CONTEXT_PORT     *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[id];
	AUDDEC_CONTEXT_COMMON   *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		isf_auddec.p_base->do_release(&isf_auddec, ISF_IN(id), pData, ISF_IN_PROBE_REL);
		(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(id), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_INVALID_STATE); // [IN] PUSH_ERR
		return ISF_ERR_UNINIT;
	}

	(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(id), ISF_IN_PROBE_PUSH, ISF_ENTER); // [IN] PUSH_ENTER

	if (p_ctx_p->gISF_AudDec_Started == 0) {
		if (gISF_AudDecMsgLevel[id] & ISF_AUDDEC_MSG_DROP) {
			DBG_ERR("[ISF_AUDDEC][%d] unit is NOT started\r\n", id);
		}
		isf_auddec.p_base->do_release(&isf_auddec, ISF_IN(id), pData, ISF_IN_PROBE_REL);
		(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(id), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_INVALID_STATE); // [IN] PUSH_ERR
		return ISF_ERR_NOT_START;
	}

	// for FilePlay Mode, check if it's a VSTM data, then return directly
	if (g_bISF_AudDec_FilePlayMode == TRUE) {
		if (pData->desc[0] == MAKEFOURCC('V','S','T','M')) {
			isf_auddec.p_base->do_release(&isf_auddec, ISF_IN(id), pData, ISF_IN_PROBE_REL);
			isf_auddec.p_base->do_probe(&isf_auddec, ISF_IN(id), 0, ISF_OK);
			return ISF_OK;
		}
	}

	if ((pAudBuf->DataAddr == 0) || (pAudBuf->DataSize == 0)) {
		isf_auddec.p_base->do_release(&isf_auddec, ISF_IN(id), pData, ISF_IN_PROBE_REL);
		isf_auddec.p_base->do_probe(&isf_auddec, ISF_IN(id), 0, ISF_ERR_INVALID_DATA);
		return ISF_ERR_INVALID_DATA;
	}

#if (SUPPORT_PUSH_FUNCTION)
	// get virtual addr
	phy_addr = pAudBuf->DataAddr;
	pAudBuf->DataAddr = nvtmpp_sys_pa2va(phy_addr);
#endif

	//if (pAudBuf->CodecType != AUDDEC_DECODER_PCM) {
	if (1) {

		// search for BS_ISF_DATA queue empty
		for (i = 0; i < ISF_AUDDEC_DATA_QUE_NUM; i++) {
			SEM_WAIT(p_ctx_p->ISF_AUDDEC_SEM_ID);
			if (!p_ctx_p->gISF_AudDec_IsBsUsed[i]) {
				p_ctx_p->gISF_AudDec_IsBsUsed[i] = TRUE;
				SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);
				memcpy(&(p_ctx_p->gISF_AudDec_BsData[i]), pData, sizeof(ISF_DATA));
				break;
			} else {
				SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);
			}
		}

		if (i == ISF_AUDDEC_DATA_QUE_NUM) {
			if (gISF_AudDecMsgLevel[id] & ISF_AUDDEC_MSG_BS) {
				DBG_WRN("[ISF_AUDDEC][%d][%d] Full, Release blk = (0x%08x, 0x%08x), time = %d us\r\n", id, i, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
			}
			//queue is full, not allow push
			isf_auddec.p_base->do_release(&isf_auddec, ISF_IN(id), pData, ISF_IN_PROBE_REL);
			(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(id), ISF_IN_PROBE_PUSH_DROP, ISF_OK);  // [IN] PUSH_DROP
			return ISF_ERR_QUEUE_FULL;
		}

#if (SUPPORT_PULL_FUNCTION)
		if (TRUE == _ISF_AudDec_isFull_PullQueue(id)) {
			//pull queue is full, not allow push
			p_ctx_p->gISF_AudDecDropCount++;
			if (p_ctx_p->gISF_AudDecDropCount % 30 == 0) {
				p_ctx_p->gISF_AudDecDropCount = 0;
				DBG_WRN("[ISF_AUDDEC][%d][%d] Pull Queue Full, Release blk = (0x%08x, 0x%08x), time = %d us\r\n", id, i, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
			}

			pNMI_AudDec->SetParam(id, NMI_AUDDEC_PARAM_DROP_FRAME, 1);
			isf_auddec.p_base->do_release(&isf_auddec, ISF_IN(id), pData, ISF_IN_PROBE_REL);
			(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(id), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [IN] PUSH_WRN

			SEM_WAIT(p_ctx_p->ISF_AUDDEC_SEM_ID);
			p_ctx_p->gISF_AudDec_IsBsUsed[i] = FALSE;
			SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);

			return ISF_ERR_QUEUE_FULL;
		}
#endif

		readyInfo.Addr      = pAudBuf->DataAddr;
		readyInfo.Size      = pAudBuf->DataSize;
		readyInfo.TimeStamp = pAudBuf->timestamp;
		readyInfo.BufID     = i;

// Dump AAC input buffer
#if 0
	UINT8 *d = (UINT8*)ReadyBSInfo.BSSrc[0].uiBSAddr;
	UINT32 c = 0;

	DBG_DUMP("isf bs a(0x%x) s(%d)\r\n", ReadyBSInfo.BSSrc[0].uiBSAddr, ReadyBSInfo.BSSrc[0].uiBSSize);
	for (; c<ReadyBSInfo.BSSrc[0].uiBSSize;)
	{
		DBG_DUMP("%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x ",
			*(d+c), *(d+c+1), *(d+c+2), *(d+c+3), *(d+c+4), *(d+c+5), *(d+c+6), *(d+c+7), *(d+c+8), *(d+c+9), *(d+c+10), *(d+c+11), *(d+c+12), *(d+c+13), *(d+c+14), *(d+c+15));
		c+=16;
	}
	DBG_DUMP("\r\n");
#endif
		// input to AudDec
		if ((ret = pNMI_AudDec->In(id, (UINT32 *)&readyInfo)) == E_SYS) {
			if (gISF_AudDecMsgLevel[id] & ISF_AUDDEC_MSG_BS) {
				DBG_ERR("[ISF_AUDDEC][%d][%d] Stop, Release blk = (0x%08x, 0x%08x), time = %d us\r\n", id, i, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
			}
			isf_auddec.p_base->do_release(&isf_auddec, ISF_IN(id), pData, ISF_IN_PROBE_REL);
			(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_IN(id), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_PROCESS_FAIL); // [IN] PUSH_ERR
			SEM_WAIT(p_ctx_p->ISF_AUDDEC_SEM_ID);
			p_ctx_p->gISF_AudDec_IsBsUsed[i] = FALSE;
			SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);
			return ISF_ERR_PROCESS_FAIL;
		}
	} /*else {
		// push to output port directly
		for (i = 0; i < ISF_AUDDEC_OUT_NUM; i++) {
			p_port = isf_auddec.port_out[i-ISF_OUT(0)]; //pDest = _isf_unit_out(&isf_auddec, ISF_OUT(i));
			if (isf_auddec.p_base->get_is_allow_push(&isf_auddec, p_port)) { //if (pDest && _isf_unit_is_allow_push(pDest)) {
				pDest = isf_auddec.p_base->oport(&isf_auddec, ISF_OUT(id));

				(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(id), ISF_IN_PROBE_PUSH, ISF_OK); // [IN] PUSH_OK
				// bypass without [IN] PROC, REL [OUT] NEW, PROC
				(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(id), ISF_OUT_PROBE_PUSH, ISF_ENTER); // [OUT] PUSH_ENTER

				// push to next unit (bypass)
				rv = isf_auddec.p_base->do_push(&isf_auddec, ISF_OUT(id), pData, 0);
				if (rv == ISF_OK) {
					(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(id), ISF_OUT_PROBE_PUSH, ISF_OK); // [OUT] PUSH_OK
				} else {
					(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(id), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [OUT] PUSH_WRN
				}
			}
		}
	}*/

	return ISF_OK;
}

#if (SUPPORT_PULL_FUNCTION)

BOOL _ISF_AudDec_isUserPull_AllReleased(UINT32 pathID)
{
	PAUDDEC_PULL_QUEUE p_obj = NULL;
	AUDDEC_RAWDATA *pAudDecData = NULL;
	UINT32 pull_queue_num = 0;
	UINT32 link_list_num = 0;
	AUDDEC_CONTEXT_PORT *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[pathID-ISF_OUT_BASE];

	if (p_ctx_p->put_pullq == FALSE) {
		return TRUE;
	}

	SEM_WAIT(p_ctx_p->ISF_AUDDEC_SEM_ID);

	// PullQueue remain number
	p_obj = &(p_ctx_p->gISF_AudDec_PullQueue);
	if (p_obj->Rear > p_obj->Front) {
		pull_queue_num = p_obj->Rear - p_obj->Front;
	} else if (p_obj->Rear < p_obj->Front){
		pull_queue_num = ISF_AUDDEC_PULL_Q_MAX + p_obj->Rear - p_obj->Front;
	} else if (p_obj->Rear == p_obj->Front && p_obj->bFull == TRUE){
		pull_queue_num = ISF_AUDDEC_PULL_Q_MAX;
	} else {
		pull_queue_num = 0;
	}

	// link-list remain number
	pAudDecData = p_ctx_p->g_p_auddec_rawdata_link_head;
	while (pAudDecData != NULL) {
		link_list_num++;
		pAudDecData = pAudDecData->pnext_rawdata;
	}

	SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);

	if (gISF_AudDecMsgLevel[pathID] & ISF_AUDDEC_MSG_PULLQ) {
		DBG_DUMP("[ISF_AUDDEC][%d] remain number => PullQueue(%u), link-list(%u)\r\n", pathID, pull_queue_num, link_list_num);
	}

	return (pull_queue_num == link_list_num)? TRUE:FALSE;
}

static ISF_RV _ISF_AudDec_OutputPort_PullImgBufFromSrc(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	UINT32 id = oport - ISF_OUT_BASE;
	AUDDEC_CONTEXT_PORT *p_ctx_p;
	ISF_RV ret;
	AUD_FRAME *p_aud_frame = NULL;
	AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[id];

	if (p_ctx_p->gISF_AudDec_Opened == FALSE) {
		DBG_ERR("module is not open yet\r\n");
		return ISF_ERR_NOT_OPEN_YET;
    }

	(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(oport), ISF_USER_PROBE_PULL, ISF_ENTER);  // [USER] PULL_ENTER

	ret = _ISF_AudDec_Get_PullQueue(oport, p_data, wait_ms);  // [USER] do_probe  PULL_ERR, PULL_OK  will do in this function

	if (ret == ISF_OK) {
		p_aud_frame = (AUD_FRAME *)p_data->desc;

		if ((p_aud_frame->addr[0] == 0) && (p_aud_frame->size == 0)) {
			// this will be treated as a decoded error
			ret = ISF_ERR_INVALID_DATA;
		}
	}

	return ret;
}

BOOL _ISF_AudDec_isEmpty_PullQueue(UINT32 pathID)
{
	PAUDDEC_PULL_QUEUE p_obj;
	BOOL is_empty = FALSE;
	AUDDEC_CONTEXT_PORT *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[pathID];

	SEM_WAIT(p_ctx_p->ISF_AUDDEC_SEM_ID);

	p_obj = &(p_ctx_p->gISF_AudDec_PullQueue);
	is_empty = ((p_obj->Front == p_obj->Rear) && (p_obj->bFull == FALSE));

	SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);

	return is_empty;
}

BOOL _ISF_AudDec_isFull_PullQueue(UINT32 pathID)
{
	PAUDDEC_PULL_QUEUE p_obj;
	BOOL is_full = FALSE;
	AUDDEC_CONTEXT_PORT *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[pathID];

	SEM_WAIT(p_ctx_p->ISF_AUDDEC_SEM_ID);

	p_obj = &(p_ctx_p->gISF_AudDec_PullQueue);
	is_full = ((p_obj->Front == p_obj->Rear) && (p_obj->bFull == TRUE));

	SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);

	return is_full;
}

void _ISF_AudDec_Unlock_PullQueue(UINT32 pathID)
{
	AUDDEC_CONTEXT_PORT *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[pathID];

	if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_AUDDEC_PULLQ_SEM_ID, vos_util_msec_to_tick(0))) {
		DBG_IND("[ISF_AUDDEC][%d] no data in pull queue, auto unlock pull blocking mode !!\r\n", pathID);
	}

	// [case_1]   no data in pullQ, then this 0 ->   0   -> 1 to unlock HDAL pull blocking mode (fake semaphore.... but it's OK, because _ISF_AudDec_Get_PullQueue() will return FALSE, there's no actual data in queue )
	// [case_2] have data in pullQ, then this n -> (n-1) -> n to recover the semaphore count
	SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_PULLQ_SEM_ID);
}

BOOL _ISF_AudDec_Put_PullQueue(UINT32 pathID, ISF_DATA *data_in)
{
	PAUDDEC_PULL_QUEUE pObj;
	AUDDEC_CONTEXT_PORT *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[pathID];

	SEM_WAIT(p_ctx_p->ISF_AUDDEC_SEM_ID);
	pObj = &(p_ctx_p->gISF_AudDec_PullQueue);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		if (gISF_AudDecMsgLevel[pathID] & ISF_AUDDEC_MSG_PULLQ) {
			DBG_ERR("[ISF_AUDDEC][%d] Pull Queue is Full!\r\n", pathID);
		}
		SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);
		return FALSE;
	} else {
		memcpy(&pObj->Queue[pObj->Rear], data_in, sizeof(ISF_DATA));

		pObj->Rear = (pObj->Rear + 1) % ISF_AUDDEC_PULL_Q_MAX;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}
		p_ctx_p->put_pullq = TRUE;
		SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_PULLQ_SEM_ID); // PULLQ + 1
		SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);
		return TRUE;
	}
}

ISF_RV _ISF_AudDec_Get_PullQueue(UINT32 pathID, ISF_DATA *data_out, INT32 wait_ms)
{
	PAUDDEC_PULL_QUEUE pObj;
	AUDDEC_CONTEXT_PORT *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[pathID];
	AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		return ISF_ERR_UNINIT;
	}

	if (wait_ms < 0) {
		// blocking (wait until data available) , if success PULLQ - 1 , else wait forever (or until signal interrupt and return FALSE)
		if (SEM_WAIT_INTERRUPTIBLE(p_ctx_p->ISF_AUDDEC_PULLQ_SEM_ID)) {
			(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pathID),  ISF_USER_PROBE_PULL_WRN, ISF_ERR_QUEUE_EMPTY);  // [USER] PULL_WRN
			return ISF_ERR_QUEUE_EMPTY;
		}
	} else  {
		// non-blocking (wait_ms=0) , timeout (wait_ms > 0). If success PULLQ - 1 , else just return FALSE
		if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_AUDDEC_PULLQ_SEM_ID, vos_util_msec_to_tick(wait_ms))) {
			DBG_IND("[ISF_AUDDEC][%d] Pull Queue Semaphore timeout!\r\n", pathID);
			(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pathID),  ISF_USER_PROBE_PULL_WRN, ISF_ERR_WAIT_TIMEOUT);  // [USER] PULL_WRN
			return ISF_ERR_WAIT_TIMEOUT;
		}
	}

	// check state
	if (p_ctx_p->gISF_AudDec_Opened == FALSE) {
		return ISF_ERR_NOT_OPEN_YET;
	}

	SEM_WAIT(p_ctx_p->ISF_AUDDEC_SEM_ID);
	pObj = &(p_ctx_p->gISF_AudDec_PullQueue);

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);
		DBG_IND("[ISF_AUDDEC][%d] Pull Queue is Empty !!!\r\n", pathID);
		(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pathID),  ISF_USER_PROBE_PULL_WRN, ISF_ERR_QUEUE_EMPTY);  // [USER] PULL_WRN
		return ISF_ERR_QUEUE_EMPTY;
	} else {
		memcpy(data_out, &pObj->Queue[pObj->Front], sizeof(ISF_DATA));

		pObj->Front= (pObj->Front+ 1) % ISF_AUDDEC_PULL_Q_MAX;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = FALSE;
		}
		SEM_SIGNAL(p_ctx_p->ISF_AUDDEC_SEM_ID);

		(&isf_auddec)->p_base->do_probe(&isf_auddec, ISF_OUT(pathID),  ISF_USER_PROBE_PULL, ISF_OK);  // [USER] PULL_OK
		return ISF_OK;
	}
}

BOOL _ISF_AudDec_RelaseAll_PullQueue(UINT32 pathID)
{
	ISF_DATA p_data;

	while (ISF_OK == _ISF_AudDec_Get_PullQueue(pathID, &p_data, 0)) ; //consume all pull queue

	return TRUE;
}

#endif // #if (SUPPORT_PULL_FUNCTION)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static BOOL _ISF_AudDec_ShowMsg(CHAR *strCmd)
{
	UINT32 uiPathID = 0, uiMsgLevel = 0;
	sscanf_s(strCmd, "%d %d", &uiPathID, &uiMsgLevel);
	if (uiPathID < PATH_MAX_COUNT) {
		gISF_AudDecMsgLevel[uiPathID] = uiMsgLevel;
	} else {
		DBG_ERR("[ISF_AUDDEC] invalid path id = %d\r\n", uiPathID);
	}
	return TRUE;
}

static BOOL _ISF_AudDec_PullQInfo(CHAR *strCmd)
{
	UINT32 uiPathID = 0;
	sscanf_s(strCmd, "%d", &uiPathID);

	if (uiPathID < PATH_MAX_COUNT) {
		gISF_AudDecMsgLevel[uiPathID] |= ISF_AUDDEC_MSG_PULLQ;
		_ISF_AudDec_isUserPull_AllReleased(uiPathID);
	}

	return TRUE;
}

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
static BOOL _isf_auddec_cmd_debug_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_auddec_cmd_debug(dev, cmdstr);
}

static BOOL _isf_auddec_cmd_trace_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_auddec_cmd_trace(dev, cmdstr);
}

static BOOL _isf_auddec_cmd_probe_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_auddec_cmd_probe(dev, cmdstr);
}

static BOOL _isf_auddec_cmd_perf_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_auddec_cmd_perf(dev, cmdstr);
}

static BOOL _isf_auddec_cmd_save_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_auddec_cmd_save(dev, cmdstr);
}
#endif // RTOS only

static SXCMD_BEGIN(isfad, "isf_auddec command")
SXCMD_ITEM("showmsg %s", _ISF_AudDec_ShowMsg, "show msg (Param: PathId Level(0:Disable, 1:BS Data, 2:Lock, 4:PullQ) => showmsg 0 1)")
SXCMD_ITEM("pullqinfo %s", _ISF_AudDec_PullQInfo, "pullq info (Param: PathId => pullqinfo 0)")
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
SXCMD_ITEM("debug %s", _isf_auddec_cmd_debug_entry, "debug [dev] [i/o] [mask]  , debug port => debug d0 o0 mffff")
SXCMD_ITEM("trace %s", _isf_auddec_cmd_trace_entry, "trace [dev] [i/o] [mask]  , trace port => trace d0 o0 mffff")
SXCMD_ITEM("probe %s", _isf_auddec_cmd_probe_entry, "probe [dev] [i/o] [mask]  , probe port => probe d0 o0 meeee")
SXCMD_ITEM("perf  %s", _isf_auddec_cmd_perf_entry,  "perf  [dev] [i/o]         , perf  port => perf d0 i0")
SXCMD_ITEM("save  %s", _isf_auddec_cmd_save_entry,  "save  [dev] [i/o] [count] , save  port => save d0 i0 1")
SXCMD_ITEM("info  %s", _isf_auddec_api_info,        "(no param)                , show  info")
#endif
SXCMD_END()

static void _ISF_AudDec_InstallCmd(void)
{
	sxcmd_addtable(isfad);
}

int _isf_auddec_cmd_isfad_showhelp(void)
{
	UINT32 cmd_num = SXCMD_NUM(isfad);
	UINT32 loop=1;

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
	DBG_DUMP("---------------------------------------------------------------------\n");
	DBG_DUMP("  adec\n");
	DBG_DUMP("---------------------------------------------------------------------\n");
#else
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "isfad");
	DBG_DUMP("=====================================================================\n");
#endif

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		DBG_DUMP("%15s : %s\r\n", isfad[loop].p_name, isfad[loop].p_desc);
	}

	return 0;
}

int _isf_auddec_cmd_isfad(char* sub_cmd_name, char *cmd_args)
{
	UINT32 cmd_num = SXCMD_NUM(isfad);
	UINT32 loop=1;
	BOOL ret=FALSE;

	if (strncmp(sub_cmd_name, "?", 2) == 0) {
		_isf_auddec_cmd_isfad_showhelp();
		return 0;
	}

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(sub_cmd_name, isfad[loop].p_name, strlen(sub_cmd_name)) == 0) {
			ret = isfad[loop].p_func(cmd_args);
			break;
		}
	}

	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n  Usage : type  \"cat /proc/hdal/adec/help\" for help.\r\n");
		return -1;
	}


	return ret;
}

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
MAINFUNC_ENTRY(adec, argc, argv)
{
	char sub_cmd_name[33] = {0};
	char cmd_args[256]    = {0};
	UINT8 i = 0;

	if (argc == 1) {
		_isf_auddec_cmd_isfad_showhelp();
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

	_isf_auddec_cmd_isfad(sub_cmd_name, cmd_args);

	return 0;
}
#endif

int _isf_auddec_cmd_isfdbg_showhelp(void)
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
int _isf_auddec_cmd_debug(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *mask;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d')) { _isf_flow_debug_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "auddec"); // auddec only has one device, and the name is "auddec"

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
int _isf_auddec_cmd_trace(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *mask;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d')) { _isf_flow_trace_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "auddec"); // auddec only has one device, and the name is "auddec"

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
int _isf_auddec_cmd_probe(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *mask;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d')) { _isf_flow_probe_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "auddec"); // auddec only has one device, and the name is "auddec"

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
int _isf_auddec_cmd_perf(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d') || (dev[1] !='0')) { _isf_flow_perf_port(0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "auddec"); // auddec only has one device, and the name is "auddec"

	//verify i/o
	io = strsep(&cmdstr, delimiters);  // i0, i1, i2, o0, o1, o2 ...
	if((io == NULL) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_perf_port(0,0); return 0;}

	//do perf
	_isf_flow_perf_port(unit_name, io);

	return 0;
}

// cmd save d0 o0 count
extern void _isf_flow_save_port(char* unit_name, char* port_name, char* count_name);
int _isf_auddec_cmd_save(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *count;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d') || (dev[1] !='0')) { _isf_flow_save_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "auddec"); // auddec only has one device, and the name is "auddec"

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

static ISF_PORT_CAPS ISF_AudDec_Input_Caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = ISF_AudDec_InputPort_PushImgBufToDest,
};

static ISF_PORT_CAPS *ISF_AudDec_InputPortList_Caps[ISF_AUDDEC_IN_NUM] = {
	&ISF_AudDec_Input_Caps,
	&ISF_AudDec_Input_Caps,
	&ISF_AudDec_Input_Caps,
	&ISF_AudDec_Input_Caps,
};

static ISF_PORT ISF_AudDec_InputPort_IN1 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(0),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_auddec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_AudDec_InputPort_IN2 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(1),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_auddec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_AudDec_InputPort_IN3 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(2),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_auddec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_AudDec_InputPort_IN4 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(3),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_auddec,
	.p_srcunit = NULL,
};

static ISF_PORT *ISF_AudDec_InputPortList[ISF_AUDDEC_IN_NUM] = {
	&ISF_AudDec_InputPort_IN1,
	&ISF_AudDec_InputPort_IN2,
	&ISF_AudDec_InputPort_IN3,
	&ISF_AudDec_InputPort_IN4,
};

#if (SUPPORT_PULL_FUNCTION)
static ISF_PORT_CAPS ISF_AudDec_OutputPort_Caps = {
	.connecttype_caps = ISF_CONNECT_PUSH  | ISF_CONNECT_PULL | ISF_CONNECT_SYNC,
	.do_pull   = _ISF_AudDec_OutputPort_PullImgBufFromSrc,
};
#else
static ISF_PORT_CAPS ISF_AudDec_OutputPort_Caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = NULL,
};
#endif

static ISF_PORT *ISF_AudDec_OutPortList[ISF_AUDDEC_OUT_NUM] = {0};
static ISF_PORT *ISF_AudDec_OutputPortList_Cfg[ISF_AUDDEC_OUT_NUM] = {0};
static ISF_PORT_CAPS *ISF_AudDec_OutputPortList_Caps[ISF_AUDDEC_OUT_NUM] = {
	&ISF_AudDec_OutputPort_Caps,
	&ISF_AudDec_OutputPort_Caps,
	&ISF_AudDec_OutputPort_Caps,
	&ISF_AudDec_OutputPort_Caps,
};

static ISF_STATE ISF_AudDec_OutputPort_State[ISF_AUDDEC_OUT_NUM] = {0};

static ISF_STATE *ISF_AudDec_OutputPortList_State[ISF_AUDDEC_OUT_NUM] = {
	&ISF_AudDec_OutputPort_State[0],
	&ISF_AudDec_OutputPort_State[1],
	&ISF_AudDec_OutputPort_State[2],
	&ISF_AudDec_OutputPort_State[3],
};

static ISF_INFO isf_auddec_outputinfo_in[ISF_AUDDEC_IN_NUM] = {0};
static ISF_INFO *isf_auddec_outputinfolist_in[ISF_AUDDEC_IN_NUM] = {
	&isf_auddec_outputinfo_in[0],
	&isf_auddec_outputinfo_in[1],
	&isf_auddec_outputinfo_in[2],
	&isf_auddec_outputinfo_in[3],
};

static ISF_INFO isf_auddec_outputinfo_out[ISF_AUDDEC_OUT_NUM] = {0};
static ISF_INFO *isf_auddec_outputinfolist_out[ISF_AUDDEC_OUT_NUM] = {
	&isf_auddec_outputinfo_out[0],
	&isf_auddec_outputinfo_out[1],
	&isf_auddec_outputinfo_out[2],
	&isf_auddec_outputinfo_out[3],
};

static ISF_RV ISF_AudDec_BindInput(struct _ISF_UNIT *pThisUnit, UINT32 iPort, struct _ISF_UNIT *pSrcUnit, UINT32 oPort)
{
	return ISF_OK;
}

static ISF_RV ISF_AudDec_BindOutput(struct _ISF_UNIT *pThisUnit, UINT32 oPort, struct _ISF_UNIT *pDestUnit, UINT32 iPort)
{
	return ISF_OK;
}

static ISF_RV _ISF_AudDec_Start(ISF_UNIT *pThisUnit, UINT32 oPort)
{
	UINT32 id = oPort-ISF_OUT_BASE;
	AUDDEC_CONTEXT_PORT *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[id];

	ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(start)\r\n");

	// "first time" START, alloc memory
	{
		if (p_ctx_p->gISF_AudDecMaxMem.Addr == 0 || p_ctx_p->gISF_AudDecMaxMem.Size == 0) {
			if (p_ctx_p->gISF_AudDecMaxMemSize == 0) {   // [isf_flow] use MAX_MEM way only
				DBG_ERR("[ISF_AUDDEC][%d] Fatal Error, please set MAX_MEM first !!\r\n", oPort);
				return ISF_ERR_FAILED;
			}

			if (_ISF_AudDec_AllocMem(pThisUnit, oPort) != ISF_OK) {
				DBG_ERR("[ISF_AUDDEC][%d] AllocMem failed\r\n", oPort);
				p_ctx_p->gISF_AudDec_Is_AllocMem = FALSE;
				return ISF_ERR_FAILED;
			} else {
				p_ctx_p->gISF_AudDec_Is_AllocMem = TRUE;
			}
		}
	}

	_ISF_AudDec_FreeMem_RawOnly(pThisUnit, id);
	pNMI_AudDec->Action(oPort, NMI_AUDDEC_ACTION_START);

	p_ctx_p->gISF_AudDec_Started = 1;

	return ISF_OK;
}

static ISF_RV _ISF_AudDec_Stop(ISF_UNIT *pThisUnit, UINT32 oPort)
{
	UINT32 id = oPort-ISF_OUT_BASE;
	AUDDEC_CONTEXT_PORT *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[id];

	ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, id, "(stop)\r\n");

	p_ctx_p->gISF_AudDec_Started = 0;
	pNMI_AudDec->Action(oPort, NMI_AUDDEC_ACTION_STOP);

	return ISF_OK;
}

static ISF_RV ISF_AudDec_SetParam(ISF_UNIT *pThisUnit, UINT32 param, UINT32 value)
{
	if (!isf_auddec_init) {
		nvtmpp_vb_add_module(pThisUnit->unit_module);
		_isf_auddec_raw_data_init();
		_ISF_AudDec_InstallCmd();
		NMP_AudDec_AddUnit();
		isf_auddec_init = TRUE;
	}

	return ISF_OK;
}

static UINT32 ISF_AudDec_GetParam(ISF_UNIT *pThisUnit, UINT32 param)
{
	return 0;
}

static ISF_RV ISF_AudDec_SetPortParam(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param, UINT32 value)
{
	ISF_RV ret = ISF_OK;
	UINT32 id = 0;
	AUDDEC_CONTEXT_PORT *p_ctx_p = 0;
	AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

    if (nPort == ISF_CTRL)
    {
        return ISF_AudDec_SetParam(pThisUnit, param, value);
    }

	if (!isf_auddec_init) {
		nvtmpp_vb_add_module(pThisUnit->unit_module);
		_isf_auddec_raw_data_init();
		_ISF_AudDec_InstallCmd();
		NMP_AudDec_AddUnit();
		isf_auddec_init = TRUE;
	}

	// Check whether NMI_AudDec exists?
	if (!pNMI_AudDec) {
		if ((pNMI_AudDec = NMI_GetUnit("AudDec")) == NULL) {
			DBG_ERR("Get AudDec unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	DBG_IND("param=0x%X, value=%d\r\n", param, value);

	if ((nPort < (int)ISF_IN_BASE) && (nPort < pThisUnit->port_out_count)) { // Output port
		switch (param) {
		#if 0  // [isf_flow] doesn't support "ISF_APPLY_DOWNSTREAM_OFFTIME"
		case ISF_PARAM_PORT_DIRTY:
			pDest = _isf_unit_in(pThisUnit, nPort);
			pAudInfo = (ISF_AUD_INFO *) & (pDest->info);
			if (pAudInfo->channelcount != gISF_AudChannel || pAudInfo->samplepersecond != gISF_AudSampleRate) {
				ret = ISF_APPLY_DOWNSTREAM_OFFTIME;
			} else {
				ret = ISF_OK;
			}
			break;
		#endif

		case AUDDEC_PARAM_FILEPLAY_MODE:
			g_bISF_AudDec_FilePlayMode = value;
			break;

		default:
			DBG_ERR("Invalid OUT paramter (0x%x)\r\n", param);
			ret = ISF_ERR_INVALID_PARAM;
			break;
		}
	// coverity[unsigned_compare]
	} else if (nPort-ISF_IN_BASE < pThisUnit->port_in_count) { // Input port
		id = nPort-ISF_IN_BASE;
		switch (param) {
		case AUDDEC_PARAM_CODEC:
		{
			PMP_AUDDEC_DECODER p_audio_obj = NULL;
			UINT32 id = nPort - ISF_IN_BASE;

			ISF_UNIT_TRACE(ISF_OP_PARAM_AUDBS, pThisUnit, ISF_IN(id), "codec(%d)", value);

			gISF_AudCodecFmt = value;
			pNMI_AudDec->SetParam(id, NMI_AUDDEC_PARAM_DECTYPE, gISF_AudCodecFmt);

			switch (gISF_AudCodecFmt) {
				case MEDIAAUDIO_CODEC_MP4A:     p_audio_obj = MP_AACDec_getAudioObject((MP_AUDDEC_ID)id);     break;
				case MEDIAAUDIO_CODEC_ULAW:     p_audio_obj = MP_uLawDec_getAudioObject((MP_AUDDEC_ID)id);    break;
				case MEDIAAUDIO_CODEC_ALAW:     p_audio_obj = MP_aLawDec_getAudioObject((MP_AUDDEC_ID)id);    break;
				case MEDIAAUDIO_CODEC_SOWT:     p_audio_obj = MP_PCMDec_getAudioObject((MP_AUDDEC_ID)id);     break;
				case 0: // close reset to 0
					return ISF_OK;
				default:
					DBG_ERR("[ISF_AUDDEC][%d] getDecodeObject failed\r\n", id);
					return ISF_ERR_FAILED;
			}

			pNMI_AudDec->SetParam(id, NMI_AUDDEC_PARAM_DECODER_OBJ, (UINT32)p_audio_obj);
		}
		break;

		case AUDDEC_PARAM_SAMPLERATE:
			ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM, pThisUnit, ISF_IN(id), "sr(%d)", value);
			p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[id];

			gISF_AudSampleRate = value;
			pNMI_AudDec->SetParam(nPort-ISF_IN_BASE, NMI_AUDDEC_PARAM_SAMPLERATE, value);

			// set dirty
			if (value != 0) {
				p_ctx_p->isf_auddec_param_dirty = 1;
			}
			break;

		case AUDDEC_PARAM_CHANNELS:
			ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM, pThisUnit, ISF_IN(id), "chs(%d)", value);
			p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[id];

			gISF_AudChannel = value;
			pNMI_AudDec->SetParam(nPort-ISF_IN_BASE, NMI_AUDDEC_PARAM_CHANNELS, value);

			// set dirty
			if (value != 0) {
				p_ctx_p->isf_auddec_param_dirty = 1;
			}
			break;

		case AUDDEC_PARAM_BITS:
			ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM, pThisUnit, ISF_IN(id), "bit_rate(%d)", value);
			p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[id];

			gISF_BitsPerSample = value;
			pNMI_AudDec->SetParam(nPort-ISF_IN_BASE, NMI_AUDDEC_PARAM_BITS, value);

			// set dirty
			if (value != 0) {
				p_ctx_p->isf_auddec_param_dirty = 1;
			}
			break;

		case AUDDEC_PARAM_ADTS_EN:
			ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM, pThisUnit, ISF_IN(id), "adts_en(%d)", value);

			pNMI_AudDec->SetParam(nPort-ISF_IN_BASE, NMI_AUDDEC_PARAM_ADTS_EN, value);
			break;

		case AUDDEC_PARAM_RAW_BLOCK_SIZE:
			ISF_UNIT_TRACE(ISF_OP_PARAM_AUDFRM, pThisUnit, ISF_IN(id), "raw_block_size(%d)", value);

			pNMI_AudDec->SetParam(nPort-ISF_IN_BASE, NMI_AUDDEC_PARAM_RAW_BLOCK_SIZE, value);
			break;

		default:
			DBG_ERR("Invalid IN paramter (0x%x)\r\n", param);
			ret = ISF_ERR_INVALID_PARAM;
			break;
		}
	}

	return ret;
}

static UINT32 ISF_AudDec_GetPortParam(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param)
{
	UINT32 value = 0;
	AUDDEC_CONTEXT_PORT *p_ctx_p = 0;
	AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

    if (nPort == ISF_CTRL)
    {
        return ISF_AudDec_GetParam(pThisUnit, param);
    }

	if (!pNMI_AudDec) {
		if ((pNMI_AudDec = NMI_GetUnit("AudDec")) == NULL) {
			DBG_ERR("Get AudDec unit failed\r\n");
			return value;
		}
	}

	if ((nPort < (int)ISF_IN_BASE) && (nPort < pThisUnit->port_out_count)) { // Parameters on output port
		switch (param) {
		case AUDDEC_PARAM_CODEC:
			pNMI_AudDec->GetParam(0, NMI_AUDDEC_PARAM_DECTYPE, &value);
			break;

#if (SUPPORT_PULL_FUNCTION)
		case AUDDEC_PARAM_BUFINFO_PHYADDR:
			p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[nPort - ISF_OUT_BASE];
			value = p_ctx_p->gISF_AudDecMem_MmapInfo.addr_physical;
			break;

		case AUDDEC_PARAM_BUFINFO_SIZE:
			p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[nPort - ISF_OUT_BASE];
			value = p_ctx_p->gISF_AudDecMem_MmapInfo.size;
			break;
#endif

		default:
			DBG_ERR("Invalid OUT paramter (0x%x)\r\n", param);
			break;
		}
	} else if (nPort-ISF_IN_BASE < pThisUnit->port_in_count) { // Parameters on input port
		switch (param) {
		case AUDDEC_PARAM_SAMPLERATE:
			pNMI_AudDec->GetParam(nPort-ISF_IN_BASE, NMI_AUDDEC_PARAM_SAMPLERATE, &value);
			break;

		case AUDDEC_PARAM_CHANNELS:
			pNMI_AudDec->GetParam(nPort-ISF_IN_BASE, NMI_AUDDEC_PARAM_CHANNELS, &value);
			break;

		case AUDDEC_PARAM_BITS:
			pNMI_AudDec->GetParam(nPort-ISF_IN_BASE, NMI_AUDDEC_PARAM_BITS, &value);
			break;

		default:
			DBG_ERR("Invalid IN paramter (0x%x)\r\n", param);
			value = ISF_ERR_NOT_SUPPORT;
			break;
		}
	}

	return value;
}

static ISF_RV ISF_AudDec_SetPortStruct(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param, UINT32* p_struct, UINT32 size)
{
	ISF_RV ret = ISF_OK;
	UINT32 value = (UINT32)p_struct;
	AUDDEC_CONTEXT_PORT *p_ctx_p = 0;
	AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	if (!isf_auddec_init) {
		nvtmpp_vb_add_module(pThisUnit->unit_module);
		_isf_auddec_raw_data_init();
		_ISF_AudDec_InstallCmd();
		NMP_AudDec_AddUnit();
		isf_auddec_init = TRUE;
	}

	if (!pNMI_AudDec) {
		if ((pNMI_AudDec = NMI_GetUnit("AudDec")) == NULL) {
			DBG_ERR("Get AudDec unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	if ((nPort < (int)ISF_IN_BASE) && (nPort < pThisUnit->port_out_count)) {
		switch (param) {
		case AUDDEC_PARAM_MAX_MEM_INFO:
	    {
			ISF_RV r = ISF_OK;
			NMI_AUDDEC_MAX_MEM_INFO* pMaxMemInfo = (NMI_AUDDEC_MAX_MEM_INFO*) value;

			DBG_IND("[ISF_AUDDEC] set max_info: codec=%d, sr=%d, chs=%d, bits=%d, release=%d\r\n",
				pMaxMemInfo->uiAudCodec, pMaxMemInfo->uiAudioSR, pMaxMemInfo->uiAudChannels, pMaxMemInfo->uiAudioBits, pMaxMemInfo->bRelease);

			if (nPort < PATH_MAX_COUNT) {
				p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[nPort - ISF_OUT_BASE];

				if (pMaxMemInfo->bRelease) {
					if (p_ctx_p->gISF_AudDecMaxMem.Addr != 0) {
						r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_AudDecMaxMemBlk), ISF_OK);
						if (r == ISF_OK) {
							p_ctx_p->gISF_AudDecMaxMem.Addr = 0;
							p_ctx_p->gISF_AudDecMaxMem.Size = 0;
							p_ctx_p->gISF_AudDecMaxMemSize = 0;
						} else {
							DBG_ERR("[ISF_AUDDEC][%d] release AudDec Max blk failed, ret = %d\r\n", nPort, r);
						}
					}
				} else {
					if (p_ctx_p->gISF_AudDecMaxMem.Addr == 0) {
		                // Set max buf info and get calculated buf size
						pNMI_AudDec->SetParam(nPort, NMI_AUDDEC_PARAM_MAX_MEM_INFO, (UINT32)pMaxMemInfo);
						p_ctx_p->gISF_AudDecMaxMemSize = pMaxMemInfo->uiDecsize;
					} else {
						DBG_ERR("[ISF_AUDDEC][%d] max buf exist, please release it first.\r\n", nPort);
					}
				}
			} else {
				DBG_ERR("[ISF_AUDDEC] invalid port index = %d\r\n", nPort);
			}
		}
		break;

		default:
			DBG_ERR("AudDec.out[%d] set struct[%08x] = %08x\r\n", nPort-ISF_OUT_BASE, param, value);
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
static ISF_RV ISF_AudDec_GetPortStruct(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param, UINT32* p_struct, UINT32 size)
{
	DBG_WRN("Not support\r\n");

	return ISF_OK;
}

static ISF_RV ISF_AudDec_UpdatePort(struct _ISF_UNIT *pThisUnit, UINT32 oPort, ISF_PORT_CMD cmd)
{
	ISF_RV r = ISF_OK;
	AUDDEC_CONTEXT_PORT   *p_ctx_p = NULL;
	AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		DBG_ERR("context not init\r\n");
		return ISF_ERR_UNINIT;
	}

	if (oPort-ISF_OUT_BASE >= PATH_MAX_COUNT) {
		if (cmd != ISF_PORT_CMD_RESET) DBG_ERR("[AUDDEC] invalid path index = %d\r\n", oPort-ISF_OUT_BASE);
		return ISF_ERR_INVALID_PORT_ID;
	}
	p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[oPort];

	if (!isf_auddec_init) {
		nvtmpp_vb_add_module(pThisUnit->unit_module);
		_isf_auddec_raw_data_init();
		_ISF_AudDec_InstallCmd();
		NMP_AudDec_AddUnit();
		isf_auddec_init = TRUE;
	}

	if (!pNMI_AudDec) {
		if ((pNMI_AudDec = NMI_GetUnit("AudDec")) == NULL) {
			DBG_ERR("Get AudDec unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

#if 0 // [isf_flow] doesn't need to check this, we can get it work even if "no-bind"
	if (!pPort) {
		return ISF_ERR_FAILED;
	}
#endif

	switch (cmd) {
		case ISF_PORT_CMD_RESET:
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(reset)");
			_ISF_AudDec_Reset(pThisUnit, oPort);
			break;

		case ISF_PORT_CMD_OPEN:									///< off -> ready   (alloc mempool and start task)
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(open)\r\n");
			_ISF_AudDec_UpdatePortInfo(pThisUnit, oPort);
			if (_ISF_AudDec_Open(pThisUnit, oPort) != ISF_OK) {
				DBG_ERR("_ISF_AudDec_Open fail\r\n");
				return ISF_ERR_FAILED;
			}

			p_ctx_p->gISF_AudDec_Opened = TRUE;
			break;

		case ISF_PORT_CMD_OFFTIME_SYNC:							///< off -> off     (apply off-time parameter if it is dirty)
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(off-sync)\r\n");
			_ISF_AudDec_UpdatePortInfo(pThisUnit, oPort);
			break;

		case ISF_PORT_CMD_READYTIME_SYNC:						///< ready -> ready (apply ready-time parameter if it is dirty)
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(ready-sync)\r\n");
			_ISF_AudDec_UpdatePortInfo(pThisUnit, oPort);
			break;

		case ISF_PORT_CMD_START:								///< ready -> run   (initial context, engine enable and device power on, start data processing)
			_ISF_AudDec_Start(pThisUnit, oPort);
			break;

		case ISF_PORT_CMD_PAUSE:								///< run -> wait    (pause data processing, keep context, engine or device enter suspend mode)
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(pause)\r\n");
			break;

		case ISF_PORT_CMD_RESUME:								///< wait -> run    (engine or device leave suspend mode, restore context, resume data processing)
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(resume)\r\n");
			break;

		case ISF_PORT_CMD_STOP:									///< run -> stop    (stop data processing, engine disable or device power off)
			_ISF_AudDec_Stop(pThisUnit, oPort);
			break;

		case ISF_PORT_CMD_CLOSE:								///< stop -> off    (terminate task and free mempool)
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(close)\r\n");

			if (FALSE == _ISF_AudDec_isUserPull_AllReleased(oPort)) {
				DBG_ERR("Close failed !! Please release all pull_out raw before attemping to close !!\r\n");
				return ISF_ERR_NOT_REL_YET;
			}

			_ISF_AudDec_Close(pThisUnit, oPort, TRUE);

			// unlock pull_out
			_ISF_AudDec_Unlock_PullQueue(oPort);

			p_ctx_p->gISF_AudDec_Opened = FALSE;
			break;

		case ISF_PORT_CMD_RUNTIME_UPDATE: {						///< run -> run     (apply run-time parameter if it is dirty)
				UINT32 id = oPort-ISF_OUT_BASE;

				ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(run-update)\r\n");
				if (p_ctx_p->isf_auddec_param_dirty) {
					// stop
					_ISF_AudDec_Stop(pThisUnit, id);
					// start
					_ISF_AudDec_Start(pThisUnit, id);
					// reset flag
					p_ctx_p->isf_auddec_param_dirty = 0;
				}
			}
			break;

		case ISF_PORT_CMD_RUNTIME_SYNC:							///< run -> run     (run-time property is dirty)
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(run-sync)\r\n");
			break;

		default:
			DBG_ERR("Invalid port command (0x%x)\r\n", cmd);
			break;
	}

	return r;
}

void isf_auddec_install_id(void)
{
	UINT32 i = 0;

    for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDDEC_CONTEXT_PORT *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[i];

        OS_CONFIG_SEMPHORE(p_ctx_p->ISF_AUDDEC_SEM_ID, 0, 1, 1);
#if (SUPPORT_PULL_FUNCTION)
		OS_CONFIG_SEMPHORE(p_ctx_p->ISF_AUDDEC_PULLQ_SEM_ID, 0, 0, 0);
#endif
    }
	OS_CONFIG_SEMPHORE(ISF_AUDDEC_COMMON_SEM_ID, 0, 1, 1);
	OS_CONFIG_SEMPHORE(ISF_AUDDEC_PROC_SEM_ID, 0, 1, 1);
}

void isf_auddec_uninstall_id(void)
{
	UINT32 i = 0;

    for (i = 0; i < PATH_MAX_COUNT; i++) {
		AUDDEC_CONTEXT_PORT *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[i];

        SEM_DESTROY(p_ctx_p->ISF_AUDDEC_SEM_ID);
#if (SUPPORT_PULL_FUNCTION)
		SEM_DESTROY(p_ctx_p->ISF_AUDDEC_PULLQ_SEM_ID);
#endif
    }
	SEM_DESTROY(ISF_AUDDEC_COMMON_SEM_ID);
	SEM_DESTROY(ISF_AUDDEC_PROC_SEM_ID);
}

//----------------------------------------------------------------------------

static UINT32 _isf_auddec_query_context_size(void)
{
	return (PATH_MAX_COUNT * sizeof(AUDDEC_CONTEXT_PORT));
}

extern UINT32 _nmp_auddec_query_context_obj_size(void);
extern UINT32 _nmp_auddec_query_context_cfg_size(void);

static UINT32 _isf_auddec_query_context_memory(void)
{
	AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

	p_ctx_c->ctx_mem.isf_auddec_size = _isf_auddec_query_context_size();
	p_ctx_c->ctx_mem.nm_auddec_obj_size = _nmp_auddec_query_context_obj_size();
	p_ctx_c->ctx_mem.nm_auddec_cfg_size = _nmp_auddec_query_context_cfg_size();
	p_ctx_c->ctx_mem.mp_obj_size = 0;

	return (p_ctx_c->ctx_mem.isf_auddec_size) + (p_ctx_c->ctx_mem.nm_auddec_obj_size) + (p_ctx_c->ctx_mem.nm_auddec_cfg_size) + (p_ctx_c->ctx_mem.mp_obj_size);
}

//----------------------------------------------------------------------------

static void _isf_auddec_assign_context(UINT32 addr)
{
	AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

	g_ad_ctx.port = (AUDDEC_CONTEXT_PORT*)addr;
	memset((void *)g_ad_ctx.port, 0, p_ctx_c->ctx_mem.isf_auddec_size);
}

extern void _nmp_auddec_assign_context_obj(UINT32 addr);
extern void _nmp_auddec_assign_context_cfg(UINT32 addr);

static ER _isf_auddec_assign_context_memory_range(UINT32 addr)
{
	AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

	// [ isf_auddec ]
	_isf_auddec_assign_context(addr);
	addr += p_ctx_c->ctx_mem.isf_auddec_size;

	// [ nmedia_auddec obj]
	_nmp_auddec_assign_context_obj(addr);
	addr += p_ctx_c->ctx_mem.nm_auddec_obj_size;

	// [ nmedia_auddec cfg]
	_nmp_auddec_assign_context_cfg(addr);
	#if _TODO
	addr += p_ctx_c->ctx_mem.nm_auddec_cfg_size;

	// [ mp_object ]
	addr += p_ctx_c->ctx_mem.mp_obj_size;
	#endif

	return E_OK;
}

//----------------------------------------------------------------------------

static void _isf_auddec_free_context(void)
{
	g_ad_ctx.port = NULL;
}

extern void _nmp_auddec_free_context(void);

static ER _isf_auddec_free_context_memory_range(void)
{
	// [ isf_vdoenc ]
	_isf_auddec_free_context();

	// [ nmedia_audenc ]
	_nmp_auddec_free_context();

	// [ mp_object ]


	return E_OK;
}

//----------------------------------------------------------------------------

static ISF_RV _isf_auddec_do_init(UINT32 h_isf, UINT32 path_max_count)
{
	ISF_RV rv = ISF_OK;
	AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;

	if (h_isf > 0) { //client process
		if (!g_auddec_init[0]) { //is master process already init?
			rv = ISF_ERR_INIT; //not allow init from client process!
			goto _AD_INIT_ERR;
		}
		g_auddec_init[h_isf] = 1; //set init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_auddec_init[i]) { //client process still alive?
				rv = ISF_ERR_INIT; //not allow init from main process!
				goto _AD_INIT_ERR;
			}
		}
	}

	if (path_max_count > 0) {
		//update count
		PATH_MAX_COUNT = path_max_count;
		if (PATH_MAX_COUNT > AUDDEC_MAX_PATH_NUM) {
			DBG_WRN("path max count %d > max %d!\r\n", PATH_MAX_COUNT, AUDDEC_MAX_PATH_NUM);
			PATH_MAX_COUNT = AUDDEC_MAX_PATH_NUM;
		}
	}

	if (p_ctx_c->ctx_mem.ctx_addr != 0) {
		DBG_ERR("[ISF_AUDDEC] already init !!\r\n");
		rv = ISF_ERR_INIT;
		goto _AD_INIT_ERR;
	}

	{
		UINT32 buf_addr = 0;
		UINT32 buf_size = _isf_auddec_query_context_memory();

		//alloc context of all port
		nvtmpp_vb_add_module(isf_auddec.unit_module);
		buf_addr = (&isf_auddec)->p_base->do_new_i(&isf_auddec, NULL, "ctx", buf_size, &(p_ctx_c->ctx_mem.ctx_memblk));

		if (buf_addr == 0) {
			DBG_ERR("[ISF_AUDDEC] alloc context memory fail !!\r\n");
			rv = ISF_ERR_HEAP;
			goto _AD_INIT_ERR;
		}
		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_auddec, ISF_CTRL, "(alloc context memory) (%u, 0x%08x, %u)", PATH_MAX_COUNT, buf_addr, buf_size);

		p_ctx_c->ctx_mem.ctx_addr = buf_addr;

		// assign memory range
		_isf_auddec_assign_context_memory_range(p_ctx_c->ctx_mem.ctx_addr);

		//install each device's kernel object
		isf_auddec_install_id();
		nmp_auddec_install_id();

		g_auddec_init[h_isf] = 1; //set init

		return ISF_OK;
	}

_AD_INIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("init fail! %d\r\n", rv);
	}
	return rv;
}

static ISF_RV _isf_auddec_do_uninit(UINT32 h_isf)
{
	ISF_RV rv = ISF_OK;
	AUDDEC_CONTEXT_COMMON *p_ctx_c = (AUDDEC_CONTEXT_COMMON *)&g_ad_ctx.comm;
	UINT32 i;

	if (h_isf > 0) { //client process
		if (!g_auddec_init[0]) { //is master process already init?
			rv = ISF_ERR_UNINIT; //not allow uninit from client process!
			goto _AD_UNINIT_ERR;
		}
		g_auddec_init[h_isf] = 0; //clear init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_auddec_init[i]) { //client process still alive?
				rv = ISF_ERR_UNINIT; //not allow uninit from main process!
				goto _AD_UNINIT_ERR;
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
		AUDDEC_CONTEXT_PORT *p_ctx_p = (AUDDEC_CONTEXT_PORT *)&g_ad_ctx.port[i];
		if (p_ctx_p->gISF_AudDec_Opened) {
			DBG_ERR("Not all port closed\r\n");
			return ISF_ERR_ALREADY_OPEN;
		}
	}

	{
		UINT32 i;

		//reset all units of this module
		DBG_IND("unit(%s) => clear state\r\n", (&isf_auddec)->unit_name);   // equal to call UpdatePort - STOP
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_auddec)->p_base->do_clear_state(&isf_auddec, oport);
		}

		DBG_IND("unit(%s) => clear bind\r\n", (&isf_auddec)->unit_name);    // equal to call UpdatePort - CLOSE
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_auddec)->p_base->do_clear_bind(&isf_auddec, oport);
		}

		DBG_IND("unit(%s) => clear context\r\n", (&isf_auddec)->unit_name); // equal to call UpdatePort - Reset
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_auddec)->p_base->do_clear_ctx(&isf_auddec, oport);
		}


		//uninstall each device's kernel object
		isf_auddec_uninstall_id();
		nmp_auddec_uninstall_id();

		//free context of all port
		if (p_ctx_c->ctx_mem.ctx_memblk.h_data != 0) {
			rv = (&isf_auddec)->p_base->do_release_i(&isf_auddec, &(p_ctx_c->ctx_mem.ctx_memblk), ISF_OK);
			if (rv == ISF_OK) {
				memset((void*)&(p_ctx_c->ctx_mem), 0, sizeof(AUDDEC_CTX_MEM));  // reset ctx_mem to 0
			} else {
				DBG_ERR("[ISF_AUDDEC] free context memory fail !!\r\n");
				goto _AD_UNINIT_ERR;
			}
		} else {
			DBG_ERR("[ISF_AUDDEC] context h_data == NULL !!\r\n");
			rv = ISF_ERR_INVALID_DATA_ID;
			goto _AD_UNINIT_ERR;
		}

		// reset NULL to all pointer
		_isf_auddec_free_context_memory_range();

		g_auddec_init[h_isf] = 0; //clear init

		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_auddec, ISF_CTRL, "(release context memory)");
		return ISF_OK;
	}

_AD_UNINIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("uninit fail! %d\r\n", rv);
	}
	return rv;
}

ISF_RV _isf_auddec_do_command(UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2)
{
	UINT32 h_isf = (cmd >> 28); //extract h_isf
	cmd &= ~0xf0000000; //clear h_isf
	//DBG_IND("(do_command) cmd = %u, p0 = 0x%08x, p1 = %u, p2 = %u\r\n", cmd, p0, p1, p2);
	switch(cmd) {
	case 1: //INIT
		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_auddec, ISF_CTRL, "(do_init) max_path = %u", p1);
		return _isf_auddec_do_init(h_isf, p1); //p1: max path count
	case 0: //UNINIT
		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_auddec, ISF_CTRL, "(do_uninit)");
		return _isf_auddec_do_uninit(h_isf);
	default:
		DBG_ERR("unsupport cmd(%u) !!\r\n", cmd);
		break;
	}
	return ISF_ERR_NOT_SUPPORT;
}
//----------------------------------------------------------------------------

/*ISF_DATA gISF_AudDecRawData = {
	.Sign      = ISF_SIGN_DATA,                 ///< signature, equal to ISF_SIGN_DATA or ISF_SIGN_EVENT
	.h_data    = 0,                             ///< handle of real data, it will be "nvtmpp blk_id", or "custom data handle"
	.pLockCB   = _ISF_AudDec_LockCB,            ///< CB to lock "custom data handle"
	.pUnlockCB = _ISF_AudDec_UnLockCB,          ///< CB to unlock "custom data handle"
	.Event     = 0,                             ///< default 0
	.Serial    = 0,                             ///< serial id
	.TimeStamp = 0,                             ///< time-stamp
};*/

static ISF_DATA_CLASS _isf_auddec_base = {
	.this = MAKEFOURCC('A','D','E','C'),
	.base = MAKEFOURCC('A','F','R','M'),
    .do_lock   = _ISF_AudDec_LockCB,
    .do_unlock = _ISF_AudDec_UnLockCB,
};

static ISF_RV _isf_auddec_raw_data_init(void)
{
#if 0
	isf_data_regclass(&_isf_auddec_base);
	return isf_auddec.p_base->init_data(&gISF_AudDecRawData, NULL, MAKEFOURCC('A','D','E','C'), 0x00010000); // [isf_flow] use this ...
#else
	_isf_auddec_base.this = 0;
	isf_auddec.p_base->init_data(&gISF_AudDecRawData, &_isf_auddec_base, _isf_auddec_base.base, 0x00010000);
	_isf_auddec_base.this = MAKEFOURCC('A','D','E','C');
	isf_data_regclass(&_isf_auddec_base);
	return E_OK;
#endif
}

ISF_UNIT isf_auddec;
static ISF_PORT_PATH ISF_AudDec_PathList[ISF_AUDDEC_OUT_NUM] = {
	{&isf_auddec, ISF_IN(0),  ISF_OUT(0)},
	{&isf_auddec, ISF_IN(1),  ISF_OUT(1)},
	{&isf_auddec, ISF_IN(2),  ISF_OUT(2)},
	{&isf_auddec, ISF_IN(3),  ISF_OUT(3)},
};

ISF_UNIT isf_auddec = {
	.sign = ISF_SIGN_UNIT,
	.unit_name = "auddec",
	.unit_module = MAKE_NVTMPP_MODULE('a', 'u', 'd', 'd', 'e', 'c', 0, 0),
	.list_id = 1, // 1=not support Multi-Process
	.port_in_count = ISF_AUDDEC_IN_NUM,
	.port_out_count = ISF_AUDDEC_OUT_NUM,
	.port_path_count = ISF_AUDDEC_OUT_NUM,
	.port_in = ISF_AudDec_InputPortList,
	.port_out = ISF_AudDec_OutPortList,
	.port_outcfg = ISF_AudDec_OutputPortList_Cfg,
	.port_incaps = ISF_AudDec_InputPortList_Caps,
	.port_outcaps = ISF_AudDec_OutputPortList_Caps,
	.port_outstate = ISF_AudDec_OutputPortList_State,
	.port_ininfo = isf_auddec_outputinfolist_in,
	.port_outinfo = isf_auddec_outputinfolist_out,
	.port_path = ISF_AudDec_PathList,
	.do_bindinput = ISF_AudDec_BindInput,
	.do_bindoutput = ISF_AudDec_BindOutput,
#if 0
	.SetParam = ISF_AudDec_SetParam,
	.GetParam = ISF_AudDec_GetParam,
#endif
	.do_setportparam = ISF_AudDec_SetPortParam,
	.do_getportparam = ISF_AudDec_GetPortParam,
	.do_setportstruct = ISF_AudDec_SetPortStruct,
	.do_getportstruct = ISF_AudDec_GetPortStruct,
	.do_update = ISF_AudDec_UpdatePort,
	.do_command = _isf_auddec_do_command,
};


