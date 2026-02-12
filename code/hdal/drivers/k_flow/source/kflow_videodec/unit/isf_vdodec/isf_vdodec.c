#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/platform.h"
#include "kwrap/stdio.h"
#include "kwrap/util.h"
#include "sxcmd_wrapper.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_videodec/isf_vdodec.h"
#include "nmediaplay_vdodec.h"
#include "isf_vdodec_internal.h"
#include "nmediaplay_api.h"
#include "video_decode.h"
#include "video_codec_mjpg.h"
#include "video_codec_h264.h"
#include "video_codec_h265.h"
#include "kdrv_videodec/kdrv_videodec_lmt.h"
#include "kdrv_videodec/kdrv_videodec_jpeg_lmt.h"
#if defined(__LINUX)
#include "kflow_videodec/isf_vdodec_platform.h"
#include <plat/top.h>
#else
#include "kwrap/cmdsys.h"
#include <rtos_na51055/top.h>
extern BOOL _isf_vdodec_api_info(CHAR *strCmd);
#endif

#if _TODO
// Perf_GetCurrent         : use do_gettimeofday() to implement
// nvtmpp_vb_block2addr    : use nvtmpp_vb_blk2va() ??
// UpdatePortInfo          : use hdal new method
// wait untill VdoOut return data size: use another method to implement
// UpdatePortInfo          : use hdal new method
// ISF_UNIT_TRACE          : define NONE first ( isf_flow doesn't EXPORT_SYMBOL this , also, if it indeed EXPORT_SYMBOL, it says disagree with version )
// release lock BS data    : lock & unlock input bs data
// remove file play mode   : remove g_bISF_VdoDec_FilePlayMode related flow
#else
#define Perf_GetCurrent(args...)            0

#define nvtmpp_vb_block2addr(args...)       nvtmpp_vb_blk2va(args)

#define TIMESTAMP_TODO                      0
#define ISF_UNIT_TRACE(opclass, p_thisunit, port, fmtstr, args...) (p_thisunit)->p_base->do_trace(p_thisunit, port, opclass, fmtstr, ##args)
#endif

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdodec
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdodec_debug_level = NVT_DBG_WRN;
module_param_named(debug_level_isf_vdodec, isf_vdodec_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_isf_vdodec, "isf_vdodec debug level");

///////////////////////////////////////////////////////////////////////////////

#define WATCH_PORT				ISF_IN(0)   //ISF_IN1

#define FORCE_DUMP_DATA     	DISABLE

#define ISF_VDODEC_IN_NUM           16
#define ISF_VDODEC_OUT_NUM          16
#define ISF_VDODEC_DATA_QUE_NUM		3
#define ISF_VDODEC_DATA_RAWQUE_NUM	3
#define ISF_VDODEC_RAWDATA_BLK_MAX	256
#define ISF_VDODEC_REF_RAWDATA_NUM	10

#define ISF_VDODEC_PULL_Q_MAX       60

#define ISF_VDODEC_MSG_BS           ((UINT32)0x00000001 << 0)   //0x00000001
#define ISF_VDODEC_MSG_RAW          ((UINT32)0x00000001 << 1)   //0x00000002
#define ISF_VDODEC_MSG_LOCK         ((UINT32)0x00000001 << 2)   //0x00000004
#define ISF_VDODEC_MSG_DROP         ((UINT32)0x00000001 << 3)   //0x00000008
#define ISF_VDODEC_MSG_BSQ_FULL     ((UINT32)0x00000001 << 4)   //0x00000010
#define ISF_VDODEC_MSG_RAWQ_FULL    ((UINT32)0x00000001 << 5)   //0x00000020

#define SUPPORT_PULL_FUNCTION               ENABLE
#define SUPPORT_PUSH_FUNCTION               ENABLE

// yuv buf calculation
#define HD_VIDEO_PXLFMT_BPP_MASK		0x00ff0000
#define HD_VIDEO_PXLFMT_BPP(pxlfmt) 	(((pxlfmt) & HD_VIDEO_PXLFMT_BPP_MASK) >> 16)
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	(ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))

typedef struct {
	UINT32           refCnt;
	UINT32           h_data;
	MEM_RANGE		 mem;
	UINT32           blk_id;
	void            *pnext_rawdata;
} VDODEC_RAWDATA;

typedef struct _VDODEC_PULL_QUEUE {
	UINT32          Front;                  ///< Front pointer
	UINT32          Rear;                   ///< Rear pointer
	UINT32          bFull;                  ///< Full flag
	ISF_DATA        Queue[ISF_VDODEC_PULL_Q_MAX];
} VDODEC_PULL_QUEUE, *PVDODEC_PULL_QUEUE;

typedef struct _VDODEC_MMAP_MEM_INFO {
	UINT32          addr_virtual;           ///< memory start addr (virtual)
	UINT32          addr_physical;          ///< memory start addr (physical)
	UINT32          size;                   ///< size
} VDODEC_MMAP_MEM_INFO, *PVDODEC_MMAP_MEM_INFO;

typedef struct _VDODEC_CTX_MEM {
	ISF_DATA        ctx_memblk;
	UINT32          ctx_addr;
	UINT32          isf_vdodec_size;
	UINT32          nm_vdodec_obj_size;
	UINT32          nm_vdodec_cfg_size;
} VDODEC_CTX_MEM;

typedef struct _VDODEC_CONTEXT_COMMON {
	// CONTEXT
	VDODEC_CTX_MEM         ctx_mem;   // alloc context memory for isf_vdodec + nmedia

	// SEMAPHORE
	SEM_HANDLE			   ISF_VDODEC_COMMON_SEM_ID;

	// RAW DATA
	ISF_DATA             gISF_VdoDec_RawData_MemBlk;
	UINT32               gISF_VdoDec_Open_Count;
#if !_USE_COMMON_FOR_RAW
	VDODEC_RAWDATA       *g_p_vdodec_rawdata_blk_pool;
	UINT32               *g_p_vdodec_rawdata_blk_maptlb;
#endif

	// Event callback
	IsfVdoDecEventCb            *gfn_vdodec_cb;

	BOOL                 isf_vdodec_init;
} VDODEC_CONTEXT_COMMON;

typedef struct _VDODEC_CONTEXT_PORT {
	UINT32				gISF_VdoDecMaxMemSize;
	NMI_VDODEC_MEM_RANGE gISF_VdoDecMaxMem;
	ISF_DATA             gISF_VdoDecMaxMemBlk;
	ISF_DATA				gISF_VdoDecMemBlk;
	VDO_FRAME            gISF_VFrameBuf;

#if (SUPPORT_PULL_FUNCTION)
	VDODEC_PULL_QUEUE	   gISF_VdoDec_PullQueue;
	VDODEC_MMAP_MEM_INFO   gISF_VdoDecMem_MmapInfo;
#endif

	UINT32				 gISF_VdoDec_Started;
	BOOL				isf_vdodec_opened;
	BOOL                gISF_VdoDec_Is_AllocMem;

	// SEMAPHORE
	SEM_HANDLE					ISF_VDODEC_SEM_ID;
#if (SUPPORT_PULL_FUNCTION)
	SEM_HANDLE					ISF_VDODEC_PULLQ_SEM_ID;
#endif

	// BS DATA (INPUT)
	ISF_DATA			 *gISF_VdoDec_BsData;
	ISF_DATA			 gISF_VdoDec_BsData_MemBlk;
	BOOL				 gISF_VdoDec_IsBsUsed[ISF_VDODEC_DATA_QUE_NUM];

	// RAW DATA (OUTPUT)
	ISF_DATA			gISF_VdoDecRefRawData; // record 'V','F','R','M' for reference frame lock/unlock
	UINT32				gISF_VdoDecRefRawBlk[ISF_VDODEC_REF_RAWDATA_NUM]; // record refernce video frame ISF_DATA blk for release if vdodec stop
#if _USE_COMMON_FOR_RAW
	UINT32				gISF_VdoDecRawQue;
	ISF_DATA 			*gISF_VdoDec_RawData;
	ISF_DATA             gISF_VdoDec_RawQue_MemBlk;
	BOOL                 *gISF_VdoDec_IsRawUsed;
#else
	VDODEC_RAWDATA       *g_p_vdodec_rawdata_link_head;
	VDODEC_RAWDATA       *g_p_vdodec_rawdata_link_tail;
#endif

	// VIDEO DECODE INFO
	VDO_FRAME			g_vdodec_user_buf;
	UINT8				g_vdodec_trig_user_buf;

	// eth cam mode
	BOOL				isf_vdodec_yuv_auto_drop;

	// DEBUG
	UINT32                          gISF_VdoDecBSFullCount;
	UINT32                          gISF_VdoDecRawFullCount;
#if (SUPPORT_PULL_FUNCTION)
	UINT32				gISF_VdoDecDropCount;
#endif
} VDODEC_CONTEXT_PORT;

typedef struct _VDODEC_CONTEXT {
	VDODEC_CONTEXT_COMMON  comm;    // common variable for all port
	VDODEC_CONTEXT_PORT   *port;    // individual variable for each port
} VDODEC_CONTEXT;

UINT32 g_vdodec_path_max_count = 0;  // dynamic setting for actual used path count, using PATH_MAX_COUNT to access this variable
VDODEC_CONTEXT g_vd_ctx = {0};
static UINT32 g_vdodec_init[ISF_FLOW_MAX] = {0};

// debug
static UINT32         		  gISF_VdoDecMsgLevel[ISF_VDODEC_IN_NUM]						    = {0};  // keep, so we can set debug command before running program

static void 				_ISF_VdoDec_InstallCmd(void);

// ISF info
NMI_UNIT					*pNMI_VdoDec										= NULL;

// Raw data (ouput)
#if !_USE_COMMON_FOR_RAW
static ISF_DATA             gISF_VdoDecRawData;
#endif

// H.26X desc
_ALIGNED(32) UINT8          g_vdodec_desc[ISF_VDODEC_IN_NUM][NMI_VDODEC_H26X_NAL_MAXSIZE] = {0};
static BOOL                 g_vdodec_desc_is_set[ISF_VDODEC_IN_NUM]             = {0};  // 1: user set already(need run time sync), 0: user do not set

static BOOL                 bValidBs[ISF_VDODEC_IN_NUM] = {FALSE};

static ISF_RV               _isf_vdodec_raw_data_init(void);

static ISF_DATA_CLASS       _isf_vdodec_base;

SEM_HANDLE                  ISF_VDODEC_PROC_SEM_ID                              = {0};

// Video decode info
static UINT32				gISF_VdoCodecFmt[ISF_VDODEC_OUT_NUM]                = {0};

#if (SUPPORT_PULL_FUNCTION)
//#define VDODEC_VIRT2PHY(pathID, virt_addr) gISF_VdoDecMem_MmapInfo[pathID].addr_physical + (virt_addr - gISF_VdoDecMem_MmapInfo[pathID].addr_virtual)

BOOL _ISF_VdoDec_isFull_PullQueue(UINT32 pathID);
void _ISF_VdoDec_Unlock_PullQueue(UINT32 pathID);
BOOL _ISF_VdoDec_Put_PullQueue(UINT32 pathID, ISF_DATA *data_in);
ISF_RV _ISF_VdoDec_Get_PullQueue(UINT32 pathID, ISF_DATA *data_out, INT32 wait_ms);
BOOL _ISF_VdoDec_RelaseAll_PullQueue(UINT32 pathID);
#endif
void _ISF_VdoDec_Check_Ref_Frm(UINT32 kdrv_dev_id, UINT32 uiYAddr, BOOL bIsRef);

// fake get_tid() function
static unsigned int get_tid ( signed int *p_tskid )
{
    *p_tskid = -99;
    return 0;
}

#if (!_USE_COMMON_FOR_RAW)
static VDODEC_RAWDATA *_ISF_VdoDec_GET_RAWDATA(void)
{
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;
	UINT32         i = 0;
	VDODEC_RAWDATA *pData = NULL;

	SEM_WAIT(p_ctx_c->ISF_VDODEC_COMMON_SEM_ID);
	while (p_ctx_c->g_p_vdodec_rawdata_blk_maptlb[i]) {
		i++;
		if (i >= (PATH_MAX_COUNT * ISF_VDODEC_RAWDATA_BLK_MAX)) {
			DBG_ERR("No free block!!!\r\n");
			SEM_SIGNAL(p_ctx_c->ISF_VDODEC_COMMON_SEM_ID);
			return NULL;
		}
	}

	pData = (VDODEC_RAWDATA *)(p_ctx_c->g_p_vdodec_rawdata_blk_pool + i);

	if (pData) {
		pData->pnext_rawdata = NULL;
		pData->blk_id = i;
		p_ctx_c->g_p_vdodec_rawdata_blk_maptlb[i] = TRUE;
	}
	SEM_SIGNAL(p_ctx_c->ISF_VDODEC_COMMON_SEM_ID);
	return pData;
}


static void _ISF_VdoDec_FREE_RAWDATA(VDODEC_RAWDATA *pData)
{
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;

	if (pData) {
		SEM_WAIT(p_ctx_c->ISF_VDODEC_COMMON_SEM_ID);
		p_ctx_c->g_p_vdodec_rawdata_blk_maptlb[pData->blk_id] = FALSE;
		pData->h_data = 0;
		SEM_SIGNAL(p_ctx_c->ISF_VDODEC_COMMON_SEM_ID);
	}
}
#endif

void _ISF_VdoDec_Check_Ref_Frm(UINT32 kdrv_dev_id, UINT32 uiYAddr, BOOL bIsRef)
{
	UINT32 pathID = kdrv_dev_id & 0xFF;
	VDODEC_CONTEXT_PORT *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[pathID];
	ISF_DATA *pRefRawData = &p_ctx_p->gISF_VdoDecRefRawData;
	UINT32 blk, i;

	if (gISF_VdoCodecFmt[pathID] == MEDIAPLAY_VIDEO_H264 && bIsRef == FALSE) {
		uiYAddr = nvtmpp_sys_pa2va(uiYAddr);
	}
	blk = (UINT32)nvtmpp_vb_va2blk(uiYAddr);

	SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
	pRefRawData->h_data = blk;

	DBG_IND("[%d] bIsRef %d, uiYAddr 0x%x, blk 0x%x\r\n", ISF_OUT(pathID), bIsRef, uiYAddr, blk);

	if (bIsRef) {
		// lock blk if this video frame be a reference frame
		isf_vdodec.p_base->do_add(&isf_vdodec, ISF_OUT(pathID), pRefRawData, 0);
		for (i = 0; i < ISF_VDODEC_REF_RAWDATA_NUM; i++) {
			if (p_ctx_p->gISF_VdoDecRefRawBlk[i] == 0) {
				p_ctx_p->gISF_VdoDecRefRawBlk[i] = pRefRawData->h_data;
				break;
			}
		}
	} else {
		// unlock blk if this video frame not be a reference frame
		for (i = 0; i < ISF_VDODEC_REF_RAWDATA_NUM; i++) {
			if (p_ctx_p->gISF_VdoDecRefRawBlk[i] == pRefRawData->h_data) {
				isf_vdodec.p_base->do_release(&isf_vdodec, ISF_OUT(pathID), pRefRawData, 0);
				p_ctx_p->gISF_VdoDecRefRawBlk[i] = 0;
				break;
			}
		}
	}
	SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
}

static ISF_RV _ISF_VdoDec_LockCB(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data)
{
#if (!_USE_COMMON_FOR_RAW)
	UINT32 i = 0;
	VDODEC_RAWDATA *pVdoDecData = NULL;
#endif
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
		DBG_ERR("[ISF_VDODEC] LOCK tid = %d h_data is NULL\r\n", tid);
		return ISF_ERR_INVALID_VALUE;
	}

#if (!_USE_COMMON_FOR_RAW)
	// search VDODEC_RAWDATA link list by h_data
	for (i = 0; i < PATH_MAX_COUNT; i++) {
		VDODEC_CONTEXT_PORT   *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[i];

		SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
		pVdoDecData = p_ctx_p->g_p_vdodec_rawdata_link_head;

		while (pVdoDecData != NULL) {
			if (pVdoDecData->h_data == h_data) {
				pVdoDecData->refCnt++;
				if (gISF_VdoDecMsgLevel[i] & ISF_VDODEC_MSG_LOCK) {
					DBG_DUMP("[ISF_VDODEC][%d] LOCK %s ref %d h_data 0x%x\r\n", i, unit_name, pVdoDecData->refCnt, pVdoDecData->h_data);
				}
				SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
				return ISF_OK;
			}

			pVdoDecData = pVdoDecData->pnext_rawdata;
		}
		SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
	}
#endif
	return ISF_OK;
}

static ISF_RV _ISF_VdoDec_UnLockCB(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data)
{
#if (!_USE_COMMON_FOR_RAW)
	UINT32 i = 0;
	VDODEC_RAWDATA *pVdoDecData = NULL;
#endif
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
		DBG_ERR("[ISF_VDODEC] UnLOCK tid = %d h_data is NULL\r\n", tid);
		return ISF_ERR_INVALID_VALUE;
	}

#if (!_USE_COMMON_FOR_RAW)
	// search VDODEC_RAWDATA link list by h_data
	for (i = 0; i < PATH_MAX_COUNT; i++) {
		VDODEC_CONTEXT_PORT   *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[i];

		SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
		pVdoDecData = p_ctx_p->g_p_vdodec_rawdata_link_head;

		while (pVdoDecData != NULL) {
			if (pVdoDecData->h_data == h_data) {
				if (pVdoDecData->refCnt > 0) {
					pVdoDecData->refCnt--;
				} else {
					get_tid(&tid);
					if (p_thisunit) {
						DBG_ERR("[ISF_VDODEC][%d] UnLOCK %s tid = %d refCnt is 0\r\n", i, unit_name, tid);
					} else {
						DBG_ERR("[ISF_VDODEC][%d] UnLOCK NULL tid = %d refCnt is 0\r\n", i, tid);
					}
					SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
					return ISF_ERR_INVALID_VALUE;
				}

				if (gISF_VdoDecMsgLevel[i] & ISF_VDODEC_MSG_LOCK) {
					DBG_DUMP("[ISF_VDODEC][%d] UNLOCK %s ref %d h_data 0x%x\r\n", i, unit_name, pVdoDecData->refCnt, pVdoDecData->h_data);
				}

				if (pVdoDecData->refCnt == 0) {
					(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(i),  ISF_USER_PROBE_REL, ISF_ENTER);  // [USER] REL_ENTER

					// Set RAW data address is unlock to NMedia
					//pNMI_VdoDec->SetParam(i, NMI_VDODEC_PARAM_UNLOCK_RAW_ADDR, h_data); // [ToDo]

					// Free isf_vdodec raw_data
					if (pVdoDecData != p_ctx_p->g_p_vdodec_rawdata_link_head) {
						VDODEC_RAWDATA *pVdoDecData2 = NULL;
						get_tid(&tid);

						DBG_ERR("[ISF_VDODEC][%d] UnLOCK %s, tid = %d, not first data in queue!! 1st link data = (0x%x, 0x%x, %d), rel data = (0x%x, 0x%x, %d)\r\n",
							i,
							unit_name,
							tid,
							(UINT32)p_ctx_p->g_p_vdodec_rawdata_link_head,
							(p_ctx_p->g_p_vdodec_rawdata_link_head?p_ctx_p->g_p_vdodec_rawdata_link_head->h_data:0),
							(p_ctx_p->g_p_vdodec_rawdata_link_head?p_ctx_p->g_p_vdodec_rawdata_link_head->blk_id:0),
							(UINT32)pVdoDecData,
							pVdoDecData->h_data,
							pVdoDecData->blk_id);

						// search for release
						pVdoDecData2 = p_ctx_p->g_p_vdodec_rawdata_link_head;

						while (pVdoDecData2 != NULL) {
							if (pVdoDecData2->pnext_rawdata == pVdoDecData) {
								pVdoDecData2->pnext_rawdata = pVdoDecData->pnext_rawdata;
								break;
							}

							pVdoDecData2 = pVdoDecData2->pnext_rawdata;
						}

						// free block
						_ISF_VdoDec_FREE_RAWDATA(pVdoDecData);

						(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(i),  ISF_USER_PROBE_REL, ISF_OK);  // [USER] REL_OK ( with some error, but actually released )
					} else {
						// update link head
						p_ctx_p->g_p_vdodec_rawdata_link_head = p_ctx_p->g_p_vdodec_rawdata_link_head->pnext_rawdata;

						// free block
						_ISF_VdoDec_FREE_RAWDATA(pVdoDecData);

						(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(i),  ISF_USER_PROBE_REL, ISF_OK);  // [USER] REL_OK
					}

					if (gISF_VdoDecMsgLevel[i] & ISF_VDODEC_MSG_LOCK) {
						DBG_DUMP("[ISF_VDODEC][%d] Free h_data = 0x%x, blk_id = %d\r\n", i, pVdoDecData->h_data, pVdoDecData->blk_id);
					}
				}
				SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
				return ISF_OK;
			}

			pVdoDecData = pVdoDecData->pnext_rawdata;
		}
		SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
	}
#endif

	return ISF_OK;
}

static ISF_RV _ISF_VdoDec_RawReadyCB(NMI_VDODEC_RAW_INFO *pRawInfo)
{
	VDODEC_CONTEXT_PORT   *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[pRawInfo->PathID];
	ISF_DATA                *pBsData        = NULL; // input
#if _USE_COMMON_FOR_RAW
	ISF_DATA				*pRawData		= &p_ctx_p->gISF_VdoDec_RawData[pRawInfo->RawBufID];
#else
	ISF_DATA				*pRawData		= &gISF_VdoDecRawData; // output
	VDODEC_RAWDATA          *pVdoDecData    = NULL;
#endif
	ISF_PORT             	*pDestPort   	= isf_vdodec.p_base->oport(&isf_vdodec, ISF_OUT(pRawInfo->PathID));
	PVDO_FRAME               pVFrameBuf     = &p_ctx_p->gISF_VFrameBuf;
	ISF_RV                   rv             = ISF_OK;
	UINT32                   i = 0;

	if (pRawInfo->isStop) {
		SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
		p_ctx_p->gISF_VdoDec_IsRawUsed[pRawInfo->RawBufID] = FALSE;
		pRawData->h_data = pRawInfo->uiCommBufBlkID;
		isf_vdodec.p_base->do_release(&isf_vdodec, ISF_OUT(pRawInfo->PathID), pRawData, 0);
		SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
		return ISF_OK;
	}

	// release lock BS data
	if (pRawInfo->Occupied == 0) {
		pBsData = &p_ctx_p->gISF_VdoDec_BsData[pRawInfo->BsBufID];

		if (gISF_VdoDecMsgLevel[pRawInfo->PathID] & ISF_VDODEC_MSG_BS) {
			DBG_DUMP("[ISF_VDODEC][%d][%d] callback Release blk_id = 0x%08x, addr = 0x%08x, time = %d us\r\n", pRawInfo->PathID, pRawInfo->BsBufID, pBsData->h_data, nvtmpp_vb_block2addr(pBsData->h_data), Perf_GetCurrent());
		}

		if (pRawInfo->uiRawAddr != 0) {
			(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pRawInfo->PathID), ISF_IN_PROBE_REL, ISF_ENTER); // [IN] REL_ENTER   // bitstream is actually decoded, addr !=0
		}

		rv = isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(pRawInfo->PathID), pBsData, 0);

		if ((pRawInfo->uiRawAddr != 0) && (rv == ISF_OK)) {
			(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(pRawInfo->PathID), ISF_IN_PROBE_REL, ISF_OK); // [IN] REL_OK  // bitstream is actually decoded, addr !=0
		}

		// unlock BS
		SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
		p_ctx_p->gISF_VdoDec_IsBsUsed[pRawInfo->BsBufID] = FALSE;
		SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
		if (pRawInfo->bReleaseBsOnly) {
			pRawData->h_data = pRawInfo->uiCommBufBlkID;
			isf_vdodec.p_base->do_release(&isf_vdodec, ISF_OUT(pRawInfo->PathID), pRawData, 0);
			return ISF_OK;
		}
	}

	if (pRawInfo->uiRawAddr == 0) {
		DBG_ERR("raw queue abnormal, raw_addr = 0, i=%llu\r\n", pRawInfo->uiThisFrmIdx);
		// unlock RAW
		SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
		p_ctx_p->gISF_VdoDec_IsRawUsed[pRawInfo->RawBufID] = FALSE;
		SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
		return ISF_OK;
	}

	(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pRawInfo->PathID), ISF_OUT_PROBE_PUSH, ISF_ENTER); // [OUT] PUSH_ENTER

	{
		pVFrameBuf->sign      = MAKEFOURCC('V', 'F', 'R', 'M');
		pVFrameBuf->addr[0]   = pRawInfo->uiYAddr;
		pVFrameBuf->addr[1]   = pRawInfo->uiUVAddr;
		pVFrameBuf->size.w    = pRawInfo->uiWidth; //gISF_VdoWidth;
		pVFrameBuf->size.h    = pRawInfo->uiHeight; //gISF_VdoHeight;
		pVFrameBuf->count     = pRawInfo->uiThisFrmIdx;
		pVFrameBuf->timestamp = pRawInfo->TimeStamp;
		pVFrameBuf->loff[0]   = pRawInfo->uiYLineOffset; //YLineOffset;
		pVFrameBuf->loff[1]   = pRawInfo->uiUVLineOffset; //UVLineOffset;
		pVFrameBuf->pxlfmt    = pRawInfo->uiYuvFmt; //VDO_PXLFMT_YUV420;
		// Y
		pVFrameBuf->pw[0] = pRawInfo->uiWidth; //gISF_VdoWidth;
		pVFrameBuf->ph[0] = pRawInfo->uiHeight; //gISF_VdoHeight;
		// UV
		if (pVFrameBuf->pxlfmt == VDO_PXLFMT_YUV420) {
			pVFrameBuf->pw[1] = pRawInfo->uiWidth >> 1; //gISF_VdoWidth >> 1;
			pVFrameBuf->ph[1] = pRawInfo->uiHeight >> 1; //gISF_VdoHeight >> 1;
		} else if (pVFrameBuf->pxlfmt == VDO_PXLFMT_YUV422) {
			pVFrameBuf->pw[1] = pRawInfo->uiWidth >> 1; //gISF_VdoWidth >> 1;
			pVFrameBuf->ph[1] = pRawInfo->uiHeight; //gISF_VdoHeight;
		}
		pVFrameBuf->reserved[1] = pRawInfo->dec_er;
		pVFrameBuf->reserved[2] = pRawInfo->uiCommBufBlkID;
#ifdef RESOLUTION_LIMIT
		if (gISF_VdoCodecFmt[pRawInfo->PathID] != MEDIAPLAY_VIDEO_MJPG) {
			//H26X
			UINT32 loff = pVFrameBuf->loff[0];
			UINT32 width = pVFrameBuf->size.w;
			UINT32 height = pVFrameBuf->size.h;
			VDO_PXLFMT pxlfmt = pVFrameBuf->pxlfmt;

			if (pxlfmt != VDO_PXLFMT_YUV420) {
				rv = ISF_ERR_INVALID_VALUE;
				goto VD_OUT_ERR_VALUE;
			}

			if (gISF_VdoCodecFmt[pRawInfo->PathID] == MEDIAPLAY_VIDEO_H264) {
				UINT32 min_width = 0, min_height = 0, max_width = 0, max_height = 0;
				if (nvt_get_chip_id() == CHIP_NA51055) {
					min_width = H264D_WIDTH_MIN;
					min_height = H264D_HEIGHT_MIN;
					max_width = H264D_WIDTH_MAX;
					max_height = H264D_HEIGHT_MAX;
				} else if (nvt_get_chip_id() == CHIP_NA51084) {
					min_width = H264D_WIDTH_MIN_528;
					min_height = H264D_HEIGHT_MIN_528;
					max_width = H264D_WIDTH_MAX_528;
					max_height = H264D_HEIGHT_MAX_528;
				}

				if (loff < min_width || loff > max_width) {
					rv = ISF_ERR_INVALID_VALUE;
					goto VD_OUT_ERR_VALUE;
				}

				if (width < min_width || width > max_width) {
					rv = ISF_ERR_INVALID_VALUE;
					goto VD_OUT_ERR_VALUE;
				} else if (height < min_height || height > max_height) {
					rv = ISF_ERR_INVALID_VALUE;
					goto VD_OUT_ERR_VALUE;
				}
			} else {
				UINT32 min_width = 0, min_height = 0, max_width = 0, max_height = 0;
				if (nvt_get_chip_id() == CHIP_NA51055) {
					min_width = H265D_WIDTH_MIN;
					min_height = H265D_HEIGHT_MIN;
					max_width = H265D_WIDTH_MAX;
					max_height = H265D_HEIGHT_MAX;
				} else if (nvt_get_chip_id() == CHIP_NA51084) {
					min_width = H265D_WIDTH_MIN_528;
					min_height = H265D_HEIGHT_MIN_528;
					max_width = H265D_WIDTH_MAX_528;
					max_height = H265D_HEIGHT_MAX_528;
				}
				if (loff < min_width || loff > max_width) {
					rv = ISF_ERR_INVALID_VALUE;
					goto VD_OUT_ERR_VALUE;
				}

				if (width < min_width || width > max_width) {
					rv = ISF_ERR_INVALID_VALUE;
					goto VD_OUT_ERR_VALUE;
				} else if (height < min_height || height > max_height) {
					rv = ISF_ERR_INVALID_VALUE;
					goto VD_OUT_ERR_VALUE;
				}
			}
		} else {
			//MJPG
			UINT32 loff = pVFrameBuf->loff[0];
			UINT32 width = pVFrameBuf->size.w;
			UINT32 height = pVFrameBuf->size.h;
			VDO_PXLFMT pxlfmt = pVFrameBuf->pxlfmt;

			if (pxlfmt != VDO_PXLFMT_YUV420 && pxlfmt != VDO_PXLFMT_YUV422) {
				rv = ISF_ERR_INVALID_VALUE;
				goto VD_OUT_ERR_VALUE;
			}

			if (loff == 0 || loff > JPEG_W_MAX) {
				rv = ISF_ERR_INVALID_VALUE;
				goto VD_OUT_ERR_VALUE;
			}

			if (width == 0 || width > JPEG_W_MAX) {
				rv = ISF_ERR_INVALID_VALUE;
				goto VD_OUT_ERR_VALUE;
			} else if (height == 0 || height > JPEG_H_MAX) {
				rv = ISF_ERR_INVALID_VALUE;
				goto VD_OUT_ERR_VALUE;
			}
		}
#endif
		if (gISF_VdoDecMsgLevel[pRawInfo->PathID] & ISF_VDODEC_MSG_RAW) {
			DBG_DUMP("[%d] Push VFRM:\r\n", pRawInfo->PathID);
			DBG_DUMP("     addr=0x%x, addr=0x%x\r\n", pVFrameBuf->addr[0], pVFrameBuf->addr[1]);
			DBG_DUMP("     w=%d, h=%d, count=%lld, timestamp=%lld\r\n", pVFrameBuf->size.w, pVFrameBuf->size.h, pVFrameBuf->count, pVFrameBuf->timestamp);
			DBG_DUMP("     loff[0]=%d, loff[1]=%d, pxlfmt=%d\r\n", pVFrameBuf->loff[0], pVFrameBuf->loff[1], pVFrameBuf->pxlfmt);
			DBG_DUMP("     pw[0]=%d, ph[0]=%d, pw[1]=%d, ph[1]=%d\r\n", pVFrameBuf->pw[0], pVFrameBuf->ph[0], pVFrameBuf->pw[1], pVFrameBuf->ph[1]);
			DBG_DUMP("     dec_er=%d\r\n", pVFrameBuf->reserved[1]);
		}

#if (SUPPORT_PULL_FUNCTION)
		// convert to physical addr
		//pVFrameBuf->phyaddr[0] = VDODEC_VIRT2PHY(pRawInfo->PathID, pRawInfo->uiYAddr);XXXXXX
		//pVFrameBuf->phyaddr[1] = VDODEC_VIRT2PHY(pRawInfo->PathID, pRawInfo->uiUVAddr);XXXXX
		pVFrameBuf->phyaddr[0] = nvtmpp_sys_va2pa(pRawInfo->uiYAddr);
		pVFrameBuf->phyaddr[1] = nvtmpp_sys_va2pa(pRawInfo->uiUVAddr);
#endif

#if (!_USE_COMMON_FOR_RAW)
		// get VDODEC_RAWDATA
		pVdoDecData = _ISF_VdoDec_GET_RAWDATA();

		if (pVdoDecData) {
			pVdoDecData->h_data = pRawInfo->uiRawAddr;
			pVdoDecData->mem.addr = pRawInfo->uiRawAddr;
			pVdoDecData->mem.size = pRawInfo->uiRawSize;

			pVdoDecData->refCnt = 0;
			if (isf_vdodec.p_base->get_is_allow_push(&isf_vdodec, pDestPort)) {
				pVdoDecData->refCnt += 1;
            }
#if (SUPPORT_PULL_FUNCTION)
			else { // Add refCnt(for user pull) when output is no-binding
				pVdoDecData->refCnt += 1;
			}
#endif
		} else {
			isf_vdodec.p_base->do_new(&isf_vdodec, ISF_OUT(pRawInfo->PathID), NVTMPP_VB_INVALID_POOL, 0, 0, pRawData, ISF_OUT_PROBE_NEW);
			(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pRawInfo->PathID), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [OUT] PUSH_WRN
			DBG_ERR("Port %d _ISF_VdoDec_NEW_BSDATA is NULL!\r\n", pRawInfo->PathID);
			return ISF_ERR_FAILED;
		}

		// add VDODEC_RAWDATA link list
		SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
		if (p_ctx_p->g_p_vdodec_rawdata_link_head == NULL) {
			p_ctx_p->g_p_vdodec_rawdata_link_head = pVdoDecData;
			p_ctx_p->g_p_vdodec_rawdata_link_tail = p_ctx_p->g_p_vdodec_rawdata_link_head;
		} else {
			p_ctx_p->g_p_vdodec_rawdata_link_tail->pnext_rawdata = pVdoDecData;
			p_ctx_p->g_p_vdodec_rawdata_link_tail = p_ctx_p->g_p_vdodec_rawdata_link_tail->pnext_rawdata;
		}
		SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);

		// Set Raw ISF_DATA info
		memcpy(pRawData->desc, pVFrameBuf, sizeof(VDO_FRAME));
		pRawData->h_data     = pRawInfo->uiRawAddr;
		//new pData by user
		isf_vdodec.p_base->do_new(&isf_vdodec, ISF_OUT(pRawInfo->PathID), NVTMPP_VB_INVALID_POOL, pRawData->h_data, 0, pRawData, ISF_OUT_PROBE_NEW);
#else
		// Set Raw ISF_DATA info
		memcpy(pRawData->desc, pVFrameBuf, sizeof(VDO_FRAME));
		pRawData->func = pRawInfo->RawBufID; // for pull out

		// user_buf need h_data to record "nvtmpp blk_id"
		if (p_ctx_p->g_vdodec_trig_user_buf) {
			pRawData->h_data = pRawInfo->uiCommBufBlkID;
		}
#endif

		if (pRawInfo->dec_er != E_OK) {
			// release all locked RAW for this GOP
			for (i = 0; i < ISF_VDODEC_REF_RAWDATA_NUM; i++) {
				if (p_ctx_p->gISF_VdoDecRefRawBlk[i] != 0) {
					pRawData->h_data = p_ctx_p->gISF_VdoDecRefRawBlk[i];
					isf_vdodec.p_base->do_release(&isf_vdodec, ISF_OUT(pRawInfo->PathID), pRawData, 0);
					p_ctx_p->gISF_VdoDecRefRawBlk[i] = 0;
				}
			}
		}

		// Push
		if (isf_vdodec.p_base->get_is_allow_push(&isf_vdodec, pDestPort)) {
			if (pRawInfo->dec_er == E_OK) { // push when decode_ok only
				rv = isf_vdodec.p_base->do_push(&isf_vdodec, ISF_OUT(pRawInfo->PathID), pRawData, 0);
				if (rv == ISF_OK) {
					(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pRawInfo->PathID), ISF_OUT_PROBE_PUSH, ISF_OK); // [OUT] PUSH_OK
				} else {
					(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pRawInfo->PathID), ISF_OUT_PROBE_PUSH_ERR, ISF_ERR_PROCESS_FAIL); // [OUT] PUSH_ERR
				}
			} else {
				// notify nvtmpp to release raw
				pRawData->h_data = pRawInfo->uiCommBufBlkID;
				isf_vdodec.p_base->do_release(&isf_vdodec, ISF_OUT(pRawInfo->PathID), pRawData, 0);
				isf_vdodec.p_base->do_release(&isf_vdodec, ISF_OUT(pRawInfo->PathID), 0, ISF_ERR_INACTIVE_STATE);
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pRawInfo->PathID), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_INVALID_DATA); // [OUT] PUSH_WRN
			}
			// unlock RAW
			SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
			p_ctx_p->gISF_VdoDec_IsRawUsed[pRawInfo->RawBufID] = FALSE;
			SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
		}
#if (SUPPORT_PULL_FUNCTION)
		else { // Add Pull Queue
			if (TRUE ==_ISF_VdoDec_Put_PullQueue(pRawInfo->PathID, pRawData)) {
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pRawInfo->PathID), ISF_OUT_PROBE_PUSH, ISF_OK); // [OUT] PUSH_OK
			} else {
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pRawInfo->PathID), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [OUT] PUSH_WRN
			}
		}
#endif
	}

	return ISF_OK;
#ifdef RESOLUTION_LIMIT
VD_OUT_ERR_VALUE:
#if (!_USE_COMMON_FOR_RAW)
	pRawData->h_data     = pRawInfo->uiRawAddr;
#else
	pRawData->h_data = pRawInfo->uiCommBufBlkID;
#endif
	SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
	p_ctx_p->gISF_VdoDec_IsRawUsed[pRawInfo->RawBufID] = FALSE;
	pRawData->h_data = pRawInfo->uiCommBufBlkID;
	isf_vdodec.p_base->do_release(&isf_vdodec, ISF_OUT(pRawInfo->PathID), pRawData, 0);
	SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
	//DBG_ERR("error yuv info wxh=%dx%d, loff=%d, pxlfmt=0x%x\r\n", pVFrameBuf->size.w, pVFrameBuf->size.h, pVFrameBuf->loff[0], pVFrameBuf->pxlfmt);
	(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pRawInfo->PathID), ISF_OUT_PROBE_PUSH_DROP, ISF_ERR_INVALID_VALUE); // [OUT] PUSH_WRN
	return rv;
#endif
}

static void _ISF_VdoDec_PlayCB(UINT32 uiEventID, UINT32 param)
{
	switch (uiEventID) {
		case NMI_VDODEC_EVENT_RAW_READY:
			_ISF_VdoDec_RawReadyCB((NMI_VDODEC_RAW_INFO *) param);
			break;
		/*case NMI_VDODEC_EVENT_CUR_VDOBS: // ---> remove, only for carDV
			if (gfn_vdodec_cb) {
				gfn_vdodec_cb("VdoDec", ISF_VDODEC_EVENT_CUR_VDOBS, param);
			} else {
				DBG_ERR("gfn_vdodec_cb is NULL\r\n");
			}
			break;*/
		default:
			break;
	}
}

/*static ISF_RV _ISF_VdoDec_UpdatePortInfo(ISF_UNIT *pThisUnit, UINT32 oPort)
{
	ISF_IN			iPort				= ISF_IN1;           /// Only has one video in ISF_IN1
	ISF_IMG_INFO	*pImgInfo			= NULL;
	USIZE         	imgsize  			= {0};
	UINT32          codec_type          = 0;
	ISF_PORT		*p_port				= NULL;

	// Check whether NMI_VdoDec exists?
	if (!pNMI_VdoDec) {
		if ((pNMI_VdoDec = NMI_GetUnit("VdoDec")) == NULL) {
			DBG_ERR("Get VdoDec unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	pNMI_VdoDec->GetParam(oPort, NMI_VDODEC_PARAM_WIDTH, &imgsize.w);
	pNMI_VdoDec->GetParam(oPort, NMI_VDODEC_PARAM_HEIGHT, &imgsize.h);
	pNMI_VdoDec->GetParam(oPort, NMI_VDODEC_PARAM_CODEC, &codec_type);
	ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM, &ISF_VdoDec, oPort + ISF_OUT_BASE, "w=%d, h=%d, codec=%d\r\n", imgsize.w, imgsize.h, codec_type);

	// sync info for gISF_VdoStrmBuf
	gISF_VdoStrmBuf[oPort].Width     = imgsize.w;
	gISF_VdoStrmBuf[oPort].Height    = imgsize.h;
	gISF_VdoStrmBuf[oPort].CodecType = codec_type;

	// sync info to iPort
	p_port = ImageUnit_In(pThisUnit, iPort);
	if (p_port) {
		pImgInfo = (ISF_IMG_INFO *) &(p_port->Info);
		pImgInfo->MaxImgSize.w = imgsize.w;
		pImgInfo->MaxImgSize.h = imgsize.h;
		pImgInfo->ImgSize.w    = imgsize.w;
		pImgInfo->ImgSize.h    = imgsize.h;
		pImgInfo->ImgFmt       = GX_IMAGE_PIXEL_FMT_YUV420_PACKED;
		pImgInfo->ImgAspect.w  = imgsize.w;
		pImgInfo->ImgAspect.h  = imgsize.h;
	}

	// sync info to oPort
	p_port = ImageUnit_Out(pThisUnit, oPort);
	if (p_port) {
		pImgInfo = (ISF_IMG_INFO *) &(p_port->Info);
		pImgInfo->MaxImgSize.w = imgsize.w;
		pImgInfo->MaxImgSize.h = imgsize.h;
		pImgInfo->ImgSize.w    = imgsize.w;
		pImgInfo->ImgSize.h    = imgsize.h;
		pImgInfo->ImgFmt       = GX_IMAGE_PIXEL_FMT_YUV420_PACKED;
		pImgInfo->ImgAspect.w  = imgsize.w;
		pImgInfo->ImgAspect.h  = imgsize.h;
	}

	return ISF_OK;
}*/
static ISF_RV _ISF_VdoDec_UpdatePortInfo(ISF_UNIT *pThisUnit, UINT32 oPort)
{
	UINT32        iPort    = (oPort + ISF_IN_BASE);
	USIZE         imgsize  = {0};
	ISF_VDO_INFO *pImgInfo = (ISF_VDO_INFO *) (pThisUnit->port_ininfo[iPort-ISF_IN_BASE]); // [isf_flow] ust this....

	if (oPort >= ISF_OUT(PATH_MAX_COUNT)) {
		return ISF_OK;
	}

	if (pImgInfo->dirty) {
		if (pImgInfo->dirty & ISF_INFO_DIRTY_IMGSIZE) {
            pNMI_VdoDec->SetParam(oPort, NMI_VDODEC_PARAM_WIDTH, pImgInfo->imgsize.w);
            pNMI_VdoDec->SetParam(oPort, NMI_VDODEC_PARAM_HEIGHT, pImgInfo->imgsize.h);
            //gISF_VFrameBuf[oPort].Width  = pImgInfo->imgsize.w;
            //gISF_VFrameBuf[oPort].Height = pImgInfo->imgsize.h;
			pImgInfo->dirty &= ~ISF_INFO_DIRTY_IMGSIZE;
        }

		if (pImgInfo->dirty & ISF_INFO_DIRTY_FRAMERATE) {
			// [TODO]
		}
	}

	pNMI_VdoDec->GetParam(oPort, NMI_VDODEC_PARAM_WIDTH, &imgsize.w);
	pNMI_VdoDec->GetParam(oPort, NMI_VDODEC_PARAM_HEIGHT, &imgsize.h);

	pImgInfo->max_imgsize.w = imgsize.w;
	pImgInfo->max_imgsize.h = imgsize.h;
	pImgInfo->imgsize.w    = imgsize.w;
	pImgInfo->imgsize.h    = imgsize.h;
	////pImgInfo->imgfmt       = VDO_PXLFMT_YUV420;   //GX_IMAGE_PIXEL_FMT_YUV420_PACKED;
	pImgInfo->imgaspect.w  = imgsize.w;
	pImgInfo->imgaspect.h  = imgsize.h;

	return ISF_OK;
}

/*static ISF_RV _isf_vdodec_oport_do_new(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 buf_size, UINT32 ddr, UINT32* p_addr, ISF_DATA *p_data)
{
	UINT32 addr = 0;
	//ISF_DATA *p_data = //&gISF_VdoDecRawData;

	if (!p_thisunit) {
		DBG_ERR("out[%d] p_thisunit is null\r\n", oport);
		return ISF_ERR_FAILED;
	}

	if (!p_addr) {
		DBG_ERR("out[%d] p_addr is null\r\n", oport);
		return ISF_ERR_FAILED;
	}

	if (!p_data) {
		DBG_ERR("out[%d] p_data is null\r\n", oport);
		return ISF_ERR_FAILED;
	}

	{
		DBG_DUMP("%s.out[%d]! New -- Vdo: ddr=%d, size=%d\r\n", p_thisunit->unit_name, oport, buf_size);
	}

	addr = p_thisunit->p_base->do_new(p_thisunit, oport, NVTMPP_VB_INVALID_POOL, ddr, buf_size, p_data, ISF_OUT_PROBE_NEW);
	if (addr == 0) {
		DBG_ERR("[%d] do new failed, size=%d\r\n", oport, buf_size);
		return ISF_ERR_BUFFER_GET;
	}

	*p_addr = addr;

	return ISF_OK;
}*/

static ISF_RV _ISF_VdoDec_AllocMem(ISF_UNIT *pThisUnit, UINT32 id)
{
	NMI_VDODEC_MEM_RANGE	Mem			= {0};
	UINT32 uiRawQueSize = 0, uiRawQueFlgSize = 0;
	UINT32					uiBufAddr	= 0;
	UINT32					uiBufSize	= 0;
   	ISF_PORT*               pSrcPort    = isf_vdodec.p_base->oport(pThisUnit, ISF_OUT(id));
	VDODEC_CONTEXT_PORT   *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[id];
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;

	// [Hack] create oport... because (isf_flow + HDAL) doestn't actually having oport
	pSrcPort = (ISF_PORT*)(0xffffff00 | ISF_OUT(id));

	pNMI_VdoDec->GetParam(id, NMI_VDODEC_PARAM_ALLOC_SIZE, &uiBufSize);

	if (gISF_VdoCodecFmt[id] != MEDIAPLAY_VIDEO_MJPG) { // only H264, H265 need allocate private buffer
		if (uiBufSize == 0 && p_ctx_p->gISF_VdoDecMaxMemSize == 0) { // neither run time param nor max mem info
			DBG_ERR("Get port %d buffer from NMI_VdoDec is Zero!\r\n", id);
			return ISF_ERR_INVALID_VALUE;
		}
	}

	// Create mempool
	nvtmpp_vb_add_module(pThisUnit->unit_module);

	if (gISF_VdoCodecFmt[id] != MEDIAPLAY_VIDEO_MJPG) { // only H264, H265 need allocate private buffer
	    // use max info to create buffer
	    if (p_ctx_p->gISF_VdoDecMaxMemSize != 0 && p_ctx_p->gISF_VdoDecMaxMem.Addr == 0) {
			uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "max", p_ctx_p->gISF_VdoDecMaxMemSize, &(p_ctx_p->gISF_VdoDecMaxMemBlk));
			if (uiBufAddr != 0) {
				p_ctx_p->gISF_VdoDecMaxMem.Addr = uiBufAddr;
				p_ctx_p->gISF_VdoDecMaxMem.Size = p_ctx_p->gISF_VdoDecMaxMemSize;
			} else {
				DBG_ERR("[ISF_VDODEC][%d] get max blk failed!\r\n", id);
				return ISF_ERR_BUFFER_CREATE;
			}
		}

		if (p_ctx_p->gISF_VdoDecMaxMem.Addr != 0 && p_ctx_p->gISF_VdoDecMaxMem.Size != 0) { // use max buf
			if (uiBufSize > p_ctx_p->gISF_VdoDecMaxMem.Size) {
				DBG_ERR("[ISF_VDODEC][%d] Need mem size (%d) > max public size (%d)\r\n", id, uiBufSize, p_ctx_p->gISF_VdoDecMaxMem.Size);
				pNMI_VdoDec->SetParam(id, NMI_VDODEC_PARAM_MEM_RANGE, (UINT32) &Mem);  //reset dec buf addr to 0
				return ISF_ERR_NEW_FAIL;
			}
			Mem.Addr = p_ctx_p->gISF_VdoDecMaxMem.Addr;
			Mem.Size = p_ctx_p->gISF_VdoDecMaxMem.Size;
		} else if (uiBufSize > 0) { // create buf from nvtmpp
			// Get blk from mempool
			uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "work", uiBufSize, &(p_ctx_p->gISF_VdoDecMemBlk));
			if (uiBufAddr == 0) {
				DBG_ERR("%s get blk (%d) failed!\r\n", pThisUnit->unit_name, Mem.Size);
				return ISF_ERR_BUFFER_CREATE;
			}

			Mem.Addr = uiBufAddr;
			Mem.Size = uiBufSize;
		}

		pNMI_VdoDec->SetParam(id, NMI_VDODEC_PARAM_MEM_RANGE, (UINT32) &Mem);
	}

	// create bs queue (input)
	uiBufSize = sizeof(ISF_DATA) * ISF_VDODEC_DATA_QUE_NUM;
	uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "bs", uiBufSize, &(p_ctx_p->gISF_VdoDec_BsData_MemBlk));
	if (uiBufAddr == 0) {
		DBG_ERR("[ISF_VDODEC][%d] get bs blk failed!\r\n", id);
		return ISF_ERR_BUFFER_CREATE;
	}
	p_ctx_p->gISF_VdoDec_BsData = (ISF_DATA *) uiBufAddr;

	// create raw queue (output)
#if _USE_COMMON_FOR_RAW
	uiRawQueSize = sizeof(ISF_DATA) * p_ctx_p->gISF_VdoDecRawQue;
	uiRawQueFlgSize = sizeof(BOOL) * p_ctx_p->gISF_VdoDecRawQue;
	uiBufSize = uiRawQueSize + uiRawQueFlgSize;
	uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "raw", uiBufSize, &(p_ctx_p->gISF_VdoDec_RawQue_MemBlk));
	if (uiBufAddr == 0) {
		DBG_ERR("[ISF_VDODEC][%d] get raw blk failed!\r\n", id);
		return ISF_ERR_BUFFER_CREATE;
	}
	p_ctx_p->gISF_VdoDec_RawData = (ISF_DATA *) uiBufAddr;
	uiBufAddr += uiRawQueSize;
	p_ctx_p->gISF_VdoDec_IsRawUsed = (BOOL *)uiBufAddr;
#else
	if (p_ctx_c->gISF_VdoDec_Open_Count == 0) {
		uiBufSize = (sizeof(UINT32) + sizeof(VDODEC_RAWDATA)) * PATH_MAX_COUNT * ISF_VDODEC_RAWDATA_BLK_MAX;
		uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, NULL, "raw", uiBufSize, &(p_ctx_c->gISF_VdoDec_RawData_MemBlk));
		if (uiBufAddr == 0) {
			DBG_ERR("[ISF_VDODEC][%d] get raw blk failed!\r\n", id);
			return ISF_ERR_BUFFER_CREATE;
		}
		p_ctx_c->g_p_vdodec_rawdata_blk_maptlb = (UINT32 *) uiBufAddr;
		memset(p_ctx_c->g_p_vdodec_rawdata_blk_maptlb, 0, uiBufSize);
		p_ctx_c->g_p_vdodec_rawdata_blk_pool = (VDODEC_RAWDATA *)(p_ctx_c->g_p_vdodec_rawdata_blk_maptlb + (PATH_MAX_COUNT * ISF_VDODEC_RAWDATA_BLK_MAX));
	}
#endif

	p_ctx_c->gISF_VdoDec_Open_Count++;

	return ISF_OK;
}

static ISF_RV _ISF_VdoDec_FreeMem(ISF_UNIT *pThisUnit, UINT32 id)
{
	VDODEC_CONTEXT_PORT   *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[id];
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;
	ISF_RV r = ISF_OK;
#if (!_USE_COMMON_FOR_RAW)
	VDODEC_RAWDATA *pVdoDecData = NULL;
#endif
	ISF_DATA *pRefRawData = &p_ctx_p->gISF_VdoDecRefRawData;
	UINT32 i = 0;

	// release bs queue
	if (p_ctx_p->gISF_VdoDec_BsData_MemBlk.h_data != 0) {
		r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_VdoDec_BsData_MemBlk), ISF_OK);
		if (r != ISF_OK) {
			DBG_ERR("[ISF_VDODEC][%d] release bs blk failed, ret = %d\r\n", id, r);
			return ISF_ERR_BUFFER_DESTROY;
		}
		memset(&(p_ctx_p->gISF_VdoDec_BsData_MemBlk), 0, sizeof(ISF_DATA));
	}
	memset(&p_ctx_p->gISF_VdoDec_IsBsUsed, 0, sizeof(p_ctx_p->gISF_VdoDec_IsBsUsed));

	// release raw queue
	if (p_ctx_c->gISF_VdoDec_Open_Count > 0)
		p_ctx_c->gISF_VdoDec_Open_Count--;

#if _USE_COMMON_FOR_RAW
	if (p_ctx_p->gISF_VdoDec_RawQue_MemBlk.h_data != 0) {
		r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_VdoDec_RawQue_MemBlk), ISF_OK);
		if (r != ISF_OK) {
			DBG_ERR("[ISF_VDODEC][%d] release raw blk failed, ret = %d\r\n", id, r);
			return ISF_ERR_BUFFER_DESTROY;
		}
		memset(&(p_ctx_p->gISF_VdoDec_RawQue_MemBlk), 0, sizeof(ISF_DATA));
	}
#else
	// release left blocks
	pVdoDecData = p_ctx_p->g_p_vdodec_rawdata_link_head;
	while (pVdoDecData != NULL) {
		_ISF_VdoDec_FREE_RAWDATA(pVdoDecData);
		pVdoDecData = pVdoDecData->pnext_rawdata;
	}
	SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
	p_ctx_p->g_p_vdodec_rawdata_link_head = NULL;
	p_ctx_p->g_p_vdodec_rawdata_link_tail = NULL;
	SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);

	if (p_ctx_c->gISF_VdoDec_Open_Count == 0) {
		if (p_ctx_c->gISF_VdoDec_RawData_MemBlk.h_data != 0) {
			r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_c->gISF_VdoDec_RawData_MemBlk), ISF_OK);
			if (r != ISF_OK) {
				DBG_ERR("[ISF_VDODEC][%d] release raw blk failed, ret = %d\r\n", id, r);
				return ISF_ERR_BUFFER_DESTROY;
			}
			memset(&(p_ctx_c->gISF_VdoDec_RawData_MemBlk), 0, sizeof(ISF_DATA));
		}

		p_ctx_c->g_p_vdodec_rawdata_blk_pool   = NULL;
		p_ctx_c->g_p_vdodec_rawdata_blk_maptlb = NULL;

		int i;
		for (i=0; i< PATH_MAX_COUNT; i++) {
			p_ctx_p->g_p_vdodec_rawdata_link_head = NULL;	//memset(g_p_vdodec_rawdata_link_head, 0, sizeof(VDODEC_BSDATA *) * PATH_MAX_COUNT);
			p_ctx_p->g_p_vdodec_rawdata_link_tail = NULL;	//memset(g_p_vdodec_rawdata_link_tail, 0, sizeof(VDODEC_BSDATA *) * PATH_MAX_COUNT);
		}
    }
#endif

	// release pull queue
	_ISF_VdoDec_RelaseAll_PullQueue(id);

	if (gISF_VdoCodecFmt[id] != MEDIAPLAY_VIDEO_MJPG) {
		for (i = 0; i < ISF_VDODEC_REF_RAWDATA_NUM; i++) {
			if (p_ctx_p->gISF_VdoDecRefRawBlk[i] != 0) {
				SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
				pRefRawData->h_data = p_ctx_p->gISF_VdoDecRefRawBlk[i];
				isf_vdodec.p_base->do_release(&isf_vdodec, ISF_OUT(id), pRefRawData, 0);
				p_ctx_p->gISF_VdoDecRefRawBlk[i] = 0;
				SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
			}
		}
	}

	return r;
}

static ISF_RV _ISF_VdoDec_FreeMem_RawOnly(ISF_UNIT *pThisUnit, UINT32 id)
{
	VDODEC_CONTEXT_PORT   *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[id];
	ISF_DATA *pRefRawData = &p_ctx_p->gISF_VdoDecRefRawData;
	UINT32 i = 0;

#if _USE_COMMON_FOR_RAW
	// release raw queue
	memset(p_ctx_p->gISF_VdoDec_IsRawUsed, 0, sizeof(BOOL) * p_ctx_p->gISF_VdoDecRawQue);
#else
	VDODEC_RAWDATA *pVdoDecData = NULL;
	// release left blocks
	pVdoDecData = p_ctx_p->g_p_vdodec_rawdata_link_head;
	while (pVdoDecData != NULL) {
		_ISF_VdoDec_FREE_RAWDATA(pVdoDecData);
		pVdoDecData = pVdoDecData->pnext_rawdata;
	}
	SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
	p_ctx_p->g_p_vdodec_rawdata_link_head = NULL;
	p_ctx_p->g_p_vdodec_rawdata_link_tail = NULL;
	SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
#endif

	// release pull queue
	_ISF_VdoDec_RelaseAll_PullQueue(id);

	if (gISF_VdoCodecFmt[id] != MEDIAPLAY_VIDEO_MJPG) {
		for (i = 0; i < ISF_VDODEC_REF_RAWDATA_NUM; i++) {
			if (p_ctx_p->gISF_VdoDecRefRawBlk[i] != 0) {
				SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
				pRefRawData->h_data = p_ctx_p->gISF_VdoDecRefRawBlk[i];
				isf_vdodec.p_base->do_release(&isf_vdodec, ISF_OUT(id), pRefRawData, 0);
				p_ctx_p->gISF_VdoDecRefRawBlk[i] = 0;
				SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
			}
		}
	}

	return ISF_OK;
}

static ISF_RV _ISF_VdoDec_Reset(ISF_UNIT *pThisUnit, UINT32 oPort)
{
	VDODEC_CONTEXT_PORT   *p_ctx_p = NULL;
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		return ISF_ERR_UNINIT; //sitll not init
	}

	p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[oPort-ISF_OUT_BASE];

	//reset in parameter
	{
		UINT32        iPort    = (oPort - ISF_OUT_BASE + ISF_IN_BASE);
		ISF_VDO_INFO *pImgInfo_In  = (ISF_VDO_INFO *) (pThisUnit->port_ininfo[iPort-ISF_IN_BASE]);
		memset(pImgInfo_In,  0, sizeof(ISF_VDO_INFO));
	}
	//reset out parameter
	{
		ISF_VDO_INFO *pImgInfo_Out = (ISF_VDO_INFO *) (pThisUnit->port_outinfo[oPort-ISF_OUT_BASE]);
		memset(pImgInfo_Out, 0, sizeof(ISF_VDO_INFO));
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

		if ((p_ctx_p->gISF_VdoDecMaxMem.Addr != 0) && (p_ctx_p->gISF_VdoDecMaxMemBlk.h_data != 0)) {
			r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_VdoDecMaxMemBlk), ISF_OK);
			if (r == ISF_OK) {
				p_ctx_p->gISF_VdoDecMaxMem.Addr = 0;
				p_ctx_p->gISF_VdoDecMaxMem.Size = 0;
				p_ctx_p->gISF_VdoDecMaxMemSize = 0;
				memset(&(p_ctx_p->gISF_VdoDecMaxMemBlk), 0, sizeof(ISF_DATA));
			} else {
				DBG_ERR("[ISF_VDODEC][%d] release VdoDec Max blk failed, ret = %d\r\n", (oPort-ISF_OUT_BASE), r);
			}
		}
	} else {
		DBG_ERR("[ISF_VDODEC] invalid port index = %d\r\n", (oPort-ISF_OUT_BASE));
	}

	return ISF_OK;
}

static ISF_RV _ISF_VdoDec_Open(ISF_UNIT *pThisUnit, UINT32 id)
{
	VDODEC_CONTEXT_PORT   *p_ctx_p = NULL;

	if (g_vd_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[id];

	if (!pNMI_VdoDec) {
		if ((pNMI_VdoDec = NMI_GetUnit("VdoDec")) == NULL) {
			DBG_ERR("Get VdoDec unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	nvtmpp_vb_add_module(pThisUnit->unit_module);   // because AllocMem() is doing at "first time START", so move this register from AllocMem to here.

	pNMI_VdoDec->SetParam(id, NMI_VDODEC_PARAM_REG_CB, (UINT32)_ISF_VdoDec_PlayCB);

	pNMI_VdoDec->SetParam(id, NMI_VDODEC_PARAM_REFFRM_CB, (UINT32)_ISF_VdoDec_Check_Ref_Frm);  // Let H26x Driver callback kflow_videodec, then we can check this frame still be a reference

	if (E_OK != pNMI_VdoDec->Open(id, 0)) {
		DBG_ERR("Open FAIL ... !!\r\n");
		return ISF_ERR_NULL_OBJECT;
	}

	p_ctx_p->isf_vdodec_opened = TRUE;

	p_ctx_p->gISF_VdoDec_Is_AllocMem = FALSE;   // set default as NOT create buffer (because AllocMem is doing at Start() now)

	p_ctx_p->gISF_VdoDecRawQue = ISF_VDODEC_DATA_RAWQUE_NUM;

	return ISF_OK;
}

static ISF_RV _ISF_VdoDec_Close(ISF_UNIT *pThisUnit, UINT32 id)
{
	VDODEC_CONTEXT_PORT   *p_ctx_p = NULL;

	if (g_vd_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[id];

	pNMI_VdoDec->Close(id);
	if (p_ctx_p->gISF_VdoDec_Is_AllocMem == TRUE) {
		if (ISF_OK == _ISF_VdoDec_FreeMem(pThisUnit, id)) {
			p_ctx_p->gISF_VdoDec_Is_AllocMem = FALSE;
		} else {
			DBG_ERR("[ISF_VDODEC][%d] FreeMem failed...!!\r\n", id);
		}
	} else {
		DBG_IND("[ISF_VDOENC][%d] close without FreeMem\r\n", id);
	}

	p_ctx_p->isf_vdodec_opened = FALSE;

	return ISF_OK;
}

/*static ISF_RV _ISF_VdoDec_Start(ISF_UNIT *pThisUnit, UINT32 id)
{
	UINT32 port_no = id;

	ISF_UNIT_TRACE(ISF_OP_STATE, &isf_vdodec, id + ISF_OUT_BASE, "(start)\r\n");

	// Check whether port is running?
	if (gISF_VdoDecPortRunStatus[port_no] == TRUE) {
		DBG_ERR("Port %d is already running!\r\n", port_no);
		return ISF_ERR_FAILED;
	} else {
		gISF_VdoDecPortRunStatus[port_no] = TRUE;
	}

	// Check whether is first time running?
	if (gISF_VdoDecPortRunCount) {
		gISF_VdoDecPortRunCount ++;
		return ISF_OK;
	} else {
		port_no = 0;                                                // Since only have one video input
		pNMI_VdoDec->Action(port_no, NMI_VDODEC_ACTION_START);
		gISF_VdoDecPortRunCount ++;
	}

	return ISF_OK;
}*/

/*static ISF_RV _ISF_VdoDec_Stop(ISF_UNIT *pThisUnit, UINT32 id)
{
	UINT32 port_no = id;

	ISF_UNIT_TRACE(ISF_OP_STATE, &ISF_VdoDec, id + ISF_OUT_BASE, "(stop)\r\n");

	// Check whether port is stopped?
	if (gISF_VdoDecPortRunStatus[port_no] == FALSE) {
		DBG_ERR("Port %d is already stopped!\r\n", port_no);
		return ISF_ERR_FAILED;
	} else {
		gISF_VdoDecPortRunStatus[port_no] = FALSE;
	}

	// Check whether is last time closing?
	if (gISF_VdoDecPortRunCount > 1) {
		gISF_VdoDecPortRunCount --;
		return ISF_OK;
	} else {
		port_no = 0;                                                // Since only have one video input
		pNMI_VdoDec->Action(port_no, NMI_VDODEC_ACTION_STOP);
		gISF_VdoDecPortRunCount = 0;
	}
	return ISF_OK;
}*/
static void _ISF_VdoDec_OutputPort_Enable(UINT32 id)
{
#if (FORCE_DUMP_DATA == ENABLE)
	if(id+ISF_IN_BASE==WATCH_PORT) {
	DBG_DUMP("^CVdoDec.in[%d]! ----------------------------------------------- Start Begin\r\n", id);
	DBG_DUMP("^CVdoDec.in[%d]! ----------------------------------------------- Start Begin\r\n", id);
	}
#endif
}

static void _ISF_VdoDec_OutputPort_Disable(UINT32 id)
{
#if (FORCE_DUMP_DATA == ENABLE)
	if(id+ISF_IN_BASE==WATCH_PORT) {
	DBG_DUMP("^CVdoDec.in[%d]! ----------------------------------------------- Stop Begin\r\n", id);
	DBG_DUMP("^CVdoDec.in[%d]! ----------------------------------------------- Stop Begin\r\n", id);
	}
#endif
}

static ISF_RV _isf_vdodec_get_buf_from_user(UINT32 id, UINT32 bs_idx, MEM_RANGE *p_mem)
{
	VDODEC_CONTEXT_PORT   *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[id];
	VDO_FRAME *p_buf = &p_ctx_p->g_vdodec_user_buf;
	UINT32 need_size = 0;

	if (p_mem == NULL) {
		DBG_IND("p_mem is null\r\n");
		return ISF_ERR_FAILED;
	}

	// check buf validation
	if (p_buf->sign != MAKEFOURCC('V', 'F', 'R', 'M')) {
		DBG_IND("invalid sign(0x%x)\r\n", p_buf->sign);
		return ISF_ERR_FAILED;
	}
	if (!(p_buf->pxlfmt == VDO_PXLFMT_YUV420 || p_buf->pxlfmt == VDO_PXLFMT_YUV422)) {
		DBG_IND("invalid pxlfmt(0x%x)\r\n", p_buf->pxlfmt);
		return ISF_ERR_FAILED;
	}
	if ((p_buf->size.w == 0) || (p_buf->size.h == 0)) {
		DBG_IND("invalid w/h (%d,%d)\r\n", p_buf->size.w, p_buf->size.h);
		return ISF_ERR_FAILED;
	}
	if (p_buf->phyaddr[0] == 0) {
		DBG_IND("invalid phyaddr(0x%x)\r\n", p_buf->phyaddr[0]);
		return ISF_ERR_FAILED;
	}

	// check need size
	pNMI_VdoDec->GetParam(id, NMI_VDODEC_PARAM_YUV_SIZE, &need_size);
	if (need_size == 0) {
		DBG_ERR("[%d] get yuv size failed\r\n", id);
		return ISF_ERR_FAILED;
	}

	// calculate user buf size
	p_mem->size = VDO_YUV_BUFSIZE(ALIGN_CEIL_64(p_buf->size.w), ALIGN_CEIL_64(p_buf->size.h), p_buf->pxlfmt);

	if (p_mem->size >= need_size) {
		p_mem->addr  = nvtmpp_sys_pa2va(p_buf->phyaddr[0]);
	} else {
		DBG_ERR("[%d] user buf size not enough, need_size(0x%x) buf_size(0x%x)\r\n", id, need_size, p_mem->size);
		return ISF_ERR_FAILED;
	}

	return ISF_OK;
}

static ISF_RV _isf_vdodec_get_buf_from_do_new(UINT32 id, UINT32 bs_idx, ISF_DATA *p_raw_data, MEM_RANGE *p_mem)
{
	ISF_UNIT *p_thisunit = &isf_vdodec;

	pNMI_VdoDec->GetParam(id, NMI_VDODEC_PARAM_YUV_SIZE, &p_mem->size);
	if (p_mem->size == 0) {
		DBG_ERR("[%d] get yuv size failed\r\n", id);
		return ISF_ERR_FAILED;
	}

	p_mem->addr = p_thisunit->p_base->do_new(p_thisunit, ISF_OUT(id), NVTMPP_VB_INVALID_POOL, 0, p_mem->size, p_raw_data, ISF_OUT_PROBE_NEW);
	if (p_mem->addr == 0 || p_mem->addr == 0xffffffff) {
		DBG_IND("[%d] do new failed, addr=0x%x size=%d\r\n", id, p_mem->addr, p_mem->size);
		return ISF_ERR_FAILED;
	}

	return ISF_OK;
}

static ISF_RV ISF_VdoDec_InputPort_PushImgBufToDest(ISF_PORT *pPort, ISF_DATA *pData, INT32 nWaitMs)
{
	ISF_UNIT                *p_thisunit = &isf_vdodec;
	ISF_PORT                *p_port = NULL;
	ISF_DATA                *pRawData = NULL;
	static ISF_DATA_CLASS   g_vdodec_out_data = {0};
	NMI_VDODEC_BS_INFO		ReadyBSInfo = {0};
	UINT32					id = (pPort->iport - ISF_IN_BASE);
	UINT32					bs_idx = 0, raw_idx = 0, i = 0;
	UINT32                  phy_addr;
	UINT8                    *ptrbs;
	MEM_RANGE               raw_buf = {0};
	VDODEC_CONTEXT_PORT   *p_ctx_p = NULL;
	ISF_DATA				*pRefRawData = NULL;

	if (g_vd_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[id];
	pRefRawData = &p_ctx_p->gISF_VdoDecRefRawData;

	if(pData->desc[0] == MAKEFOURCC('V','F','R','M')) { // for user push video out buf
		VDO_FRAME *p_user_out_buf = (VDO_FRAME *)(pData->desc);

		if (p_ctx_p->gISF_VdoDec_Started == 0) {
			DBG_ERR("[ISF_VDODEC][%d] VFRM unit is NOT started\r\n", id);
			isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pData, 0);
			(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(id), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INVALID_STATE); // [IN] PUSH_ERR
			return ISF_ERR_INVALID_STATE;
		}

		SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
		memcpy(&p_ctx_p->g_vdodec_user_buf, p_user_out_buf, sizeof(VDO_FRAME));
		memcpy(&p_ctx_p->gISF_VdoDecRefRawData, pData, sizeof(ISF_DATA));
		p_ctx_p->g_vdodec_user_buf.reserved[0] = pData->h_data;
		SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);

		DBG_IND("[%d] push_in user_buf: sign=0x%x, ddr=%d, pxlfmt=0x%x, w=%d, h=%d, pa=0x%x, blk=0x%x\r\n",
				id,
				p_ctx_p->g_vdodec_user_buf.sign,
				p_ctx_p->g_vdodec_user_buf.resv1,
				p_ctx_p->g_vdodec_user_buf.pxlfmt,
				p_ctx_p->g_vdodec_user_buf.size.w,
				p_ctx_p->g_vdodec_user_buf.size.h,
				p_ctx_p->g_vdodec_user_buf.phyaddr[0],
				p_ctx_p->g_vdodec_user_buf.reserved[0]);

		p_ctx_p->g_vdodec_trig_user_buf = 1;
	} else if (pData->desc[0] == MAKEFOURCC('V','S','T','M')) {
		VDO_BITSTREAM           *pVdoBuf = (VDO_BITSTREAM *)(pData->desc);

		(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(id), ISF_IN_PROBE_PUSH, ISF_ENTER); // [IN] PUSH_ENTER

		ReadyBSInfo.uiCommBufBlkID = p_ctx_p->g_vdodec_user_buf.reserved[0];

		if (pData->h_data == 0) {
			DBG_ERR("[ISF_VDODEC][%d] VSTM unit blk is NULL !!!\r\n", id);

			// release YUV
			SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
			pRefRawData->h_data = ReadyBSInfo.uiCommBufBlkID;
			isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pRefRawData, 0);
			SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);

			(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(id), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INVALID_STATE); // [IN] PUSH_ERR
			return ISF_ERR_NULL_POINTER;
		}

		if (p_ctx_p->gISF_VdoDec_Started == 0) {
			DBG_ERR("[ISF_VDODEC][%d] VSTM unit is NOT started\r\n", id);
			// release BS
			isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pData, 0);

			// release YUV
			SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
			pRefRawData->h_data = ReadyBSInfo.uiCommBufBlkID;
			isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pRefRawData, 0);
			SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);

			(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(id), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INVALID_STATE); // [IN] PUSH_ERR
			return ISF_ERR_INVALID_STATE;
		}

		if ((pVdoBuf->DataAddr == 0) || (pVdoBuf->DataSize == 0)) {
			DBG_ERR("vdo buf addr=0x%x, size=%d\r\n", pVdoBuf->DataAddr, pVdoBuf->DataSize);
			// release BS
			isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pData, 0);

			// release YUV
			SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
			pRefRawData->h_data = ReadyBSInfo.uiCommBufBlkID;
			isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pRefRawData, 0);
			SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);

			isf_vdodec.p_base->do_probe(&isf_vdodec, ISF_IN(id), 0, ISF_ERR_INVALID_DATA);
			return ISF_ERR_INVALID_DATA;
		}

#if (SUPPORT_PUSH_FUNCTION)
		// get virtual addr
		phy_addr = pVdoBuf->DataAddr;
		pVdoBuf->DataAddr = nvtmpp_sys_pa2va(phy_addr);
#endif

		if (gISF_VdoCodecFmt[id] != MEDIAPLAY_VIDEO_MJPG && nWaitMs != -99) {
			ptrbs = (UINT8 *)pVdoBuf->DataAddr;
			if ((*ptrbs != 0x0 || *(ptrbs+1) != 0x0 || *(ptrbs+2) != 0x0 || *(ptrbs+3) != 0x1)) {
				// release BS
				isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pData, 0);
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(id), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_PROCESS_FAIL); // [IN] PUSH_ERR

				// release RAW
				SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
				pRawData = &p_ctx_p->gISF_VdoDecRefRawData;
				pRawData->h_data = ReadyBSInfo.uiCommBufBlkID;
				isf_vdodec.p_base->do_release(&isf_vdodec, ISF_OUT(id), pRawData, 0);
				SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
				bValidBs[id] = FALSE;

				// release all locked RAW for this GOP
				for (i = 0; i < ISF_VDODEC_REF_RAWDATA_NUM; i++) {
					if (p_ctx_p->gISF_VdoDecRefRawBlk[i] != 0) {
						SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
						pRawData->h_data = p_ctx_p->gISF_VdoDecRefRawBlk[i];
						isf_vdodec.p_base->do_release(&isf_vdodec, ISF_OUT(id), pRawData, 0);
						p_ctx_p->gISF_VdoDecRefRawBlk[i] = 0;
						SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
					}
				}

				return ISF_ERR_INVALID_DATA;
			}

			if (!bValidBs[id]) {
				if (gISF_VdoCodecFmt[id] == MEDIAPLAY_VIDEO_H264) {
					if (*ptrbs == 0x0 && *(ptrbs+1) == 0x0 && *(ptrbs+2) == 0x0 && *(ptrbs+3) == 0x1 && *(ptrbs+4) == 0x65) {
						bValidBs[id] = TRUE;
					} else {
						// release BS
						isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pData, 0);
						(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(id), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_PROCESS_FAIL); // [IN] PUSH_ERR

						// release RAW
						SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
						pRawData = &p_ctx_p->gISF_VdoDecRefRawData;
						pRawData->h_data = ReadyBSInfo.uiCommBufBlkID;
						isf_vdodec.p_base->do_release(&isf_vdodec, ISF_OUT(id), pRawData, 0);
						SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);

						bValidBs[id] = FALSE;
						return ISF_ERR_INVALID_DATA;
					}
				} else if (gISF_VdoCodecFmt[id] == MEDIAPLAY_VIDEO_H265) {
					if (*ptrbs == 0x0 && *(ptrbs+1) == 0x0 && *(ptrbs+2) == 0x0 && *(ptrbs+3) == 0x1 && *(ptrbs+4) == 0x26) {
						bValidBs[id] = TRUE;
					} else {
						// release BS
						isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pData, 0);
						(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(id), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_PROCESS_FAIL); // [IN] PUSH_ERR

						// release RAW
						SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
						pRawData = &p_ctx_p->gISF_VdoDecRefRawData;
						pRawData->h_data = ReadyBSInfo.uiCommBufBlkID;
						isf_vdodec.p_base->do_release(&isf_vdodec, ISF_OUT(id), pRawData, 0);
						SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);

						bValidBs[id] = FALSE;
					return ISF_ERR_INVALID_DATA;
					}
				}
			}
		}

		if (pVdoBuf->CodecType != MEDIAPLAY_VIDEO_YUV) {
FIND_BS_USED:
			// search for BS ISF_DATA queue empty
			for (bs_idx = 0; bs_idx < ISF_VDODEC_DATA_QUE_NUM; bs_idx++) {
				SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
				if (!p_ctx_p->gISF_VdoDec_IsBsUsed[bs_idx]) {
					p_ctx_p->gISF_VdoDec_IsBsUsed[bs_idx] = TRUE;
					SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
					memcpy(&p_ctx_p->gISF_VdoDec_BsData[bs_idx], pData, sizeof(ISF_DATA));
					break;
				} else {
					SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
				}
			}
			if (bs_idx == ISF_VDODEC_DATA_QUE_NUM) {
				//DBG_WRN("[ISF_VDODEC][%d][%d] BsQue Full, Release blk = (0x%08x, 0x%08x), time = %d us\r\n", id, bs_idx, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
				if (gISF_VdoDecMsgLevel[id] & ISF_VDODEC_MSG_BSQ_FULL) {
					p_ctx_p->gISF_VdoDecBSFullCount++;
					if (p_ctx_p->gISF_VdoDecBSFullCount % 30 == 0) {
						p_ctx_p->gISF_VdoDecBSFullCount = 0;
						DBG_WRN("[%d][%d] BsQue Full, try again\r\n", id, bs_idx);
					}
				}
				vos_util_delay_ms(5);
				goto FIND_BS_USED;	// try again
			}

#if (SUPPORT_PULL_FUNCTION)
			// check pull queue
			if (TRUE == _ISF_VdoDec_isFull_PullQueue(id)) {
				//pull queue is full, not allow push
				p_ctx_p->gISF_VdoDecDropCount++;
				if (p_ctx_p->gISF_VdoDecDropCount % 30 == 0) {
					p_ctx_p->gISF_VdoDecDropCount = 0;
					DBG_WRN("[ISF_VDODEC][%d][%d] Pull Queue Full, Release blk = (0x%08x, 0x%08x), time = %d us\r\n", id, bs_idx, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
				}

				pNMI_VdoDec->SetParam(id, NMI_VDODEC_PARAM_DROP_FRAME, 1);
				//release BS
				isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pData, 0);

				//release YUV
				SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
				pRefRawData->h_data = ReadyBSInfo.uiCommBufBlkID;
				isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pRefRawData, 0);
				SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);

				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(id), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [IN] PUSH_WRN
				// unlock BS
				SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
				p_ctx_p->gISF_VdoDec_IsBsUsed[bs_idx] = FALSE;
				SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
				return ISF_ERR_QUEUE_FULL;
			}
#endif

FIND_RAW_USED:
			if (nWaitMs != -99) {
				// search for RAW ISF_DATA queue empty
				for (raw_idx = 0; raw_idx < p_ctx_p->gISF_VdoDecRawQue; raw_idx++) {
					SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
					if (!p_ctx_p->gISF_VdoDec_IsRawUsed[raw_idx]) {
						p_ctx_p->gISF_VdoDec_IsRawUsed[raw_idx] = TRUE;
						SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
						// new raw isf_data from private pool
						pRawData = &p_ctx_p->gISF_VdoDec_RawData[raw_idx];
						p_thisunit->p_base->init_data(pRawData, &g_vdodec_out_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
						break;
					} else {
						SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
					}
				}
				if (raw_idx == p_ctx_p->gISF_VdoDecRawQue) {
					if (p_ctx_p->isf_vdodec_yuv_auto_drop) {
#if (SUPPORT_PULL_FUNCTION)
						ISF_DATA p_rel_data;
						ISF_RV ret;

						ret = _ISF_VdoDec_Get_PullQueue(id, &p_rel_data, 0); //consume all pull queue
						if (ret == ISF_OK) {
							// release the earliest raw in que
							isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), &p_rel_data, 0);

							// set RAW ISF_DATA queue empty (raw use common buffer, nvtmpp will handle the release mechanism)
							SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
							p_ctx_p->gISF_VdoDec_IsRawUsed[p_rel_data.func] = FALSE;
							SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
						}
#endif
					} else {
						vos_util_delay_ms(5);
					}
					if (gISF_VdoDecMsgLevel[id] & ISF_VDODEC_MSG_RAWQ_FULL) {
						p_ctx_p->gISF_VdoDecRawFullCount++;
						if (p_ctx_p->gISF_VdoDecRawFullCount % 30 == 0) {
							p_ctx_p->gISF_VdoDecRawFullCount = 0;
							DBG_WRN("[ISF_VDODEC][%d][%d] RawQue Full, try again\r\n", id, raw_idx);
						}
					}
					goto FIND_RAW_USED;  // try again
				}
			} else {
				ptrbs = (UINT8 *)pVdoBuf->DataAddr;
				if (*ptrbs == 0x0 && *(ptrbs+1) == 0x0 && *(ptrbs+2) == 0x0 && *(ptrbs+3) == 0x1) {
					// search for RAW ISF_DATA queue empty
					for (raw_idx = 0; raw_idx < p_ctx_p->gISF_VdoDecRawQue; raw_idx++) {
						SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
						if (!p_ctx_p->gISF_VdoDec_IsRawUsed[raw_idx]) {
							p_ctx_p->gISF_VdoDec_IsRawUsed[raw_idx] = TRUE;
							SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
							// new raw isf_data from private pool
							pRawData = &p_ctx_p->gISF_VdoDec_RawData[raw_idx];
							p_thisunit->p_base->init_data(pRawData, &g_vdodec_out_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
							break;
						} else {
							SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
						}
					}
					if (raw_idx == p_ctx_p->gISF_VdoDecRawQue) {
						if (p_ctx_p->isf_vdodec_yuv_auto_drop) {
#if (SUPPORT_PULL_FUNCTION)
							ISF_DATA p_rel_data;
							ISF_RV ret;

							ret = _ISF_VdoDec_Get_PullQueue(id, &p_rel_data, 0); //consume all pull queue
							if (ret == ISF_OK) {
								// release the earliest raw in que
								isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), &p_rel_data, 0);

								// set RAW ISF_DATA queue empty (raw use common buffer, nvtmpp will handle the release mechanism)
								SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
								p_ctx_p->gISF_VdoDec_IsRawUsed[p_rel_data.func] = FALSE;
								SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
							}
#endif
						} else {
							vos_util_delay_ms(5);
						}
						if (gISF_VdoDecMsgLevel[id] & ISF_VDODEC_MSG_RAWQ_FULL) {
							p_ctx_p->gISF_VdoDecRawFullCount++;
							if (p_ctx_p->gISF_VdoDecRawFullCount % 30 == 0) {
								p_ctx_p->gISF_VdoDecRawFullCount = 0;
								DBG_WRN("[ISF_VDODEC][%d][%d] RawQue Full, try again\r\n", id, raw_idx);
							}
						}
						goto FIND_RAW_USED;  // try again
					}
				}
			}

			if (p_ctx_p->g_vdodec_trig_user_buf) {
				if(_isf_vdodec_get_buf_from_user(id, bs_idx, &raw_buf) != ISF_OK) {
					DBG_ERR("[%d] get buf from user failed\r\n", id);
					(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(id), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_NEW_FAIL); // [IN] PUSH_WRN
					// release BS
					isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pData, 0);
					// unlock BS
					SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
					p_ctx_p->gISF_VdoDec_IsBsUsed[bs_idx] = FALSE;
					SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);

					// release RAW
					SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
					pRefRawData->h_data = ReadyBSInfo.uiCommBufBlkID;
					isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pRefRawData, 0);
					SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
					// unlock RAW
					SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
					p_ctx_p->gISF_VdoDec_IsRawUsed[raw_idx] = FALSE;
					SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
					return ISF_ERR_BUFFER_GET;
				}
			} else {
				if(_isf_vdodec_get_buf_from_do_new(id, bs_idx, pRawData, &raw_buf) != ISF_OK) {
					DBG_ERR("[%d] get buf from do_new failed\r\n", id);
					(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(id), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_NEW_FAIL); // [IN] PUSH_WRN
					// release BS
					isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pData, 0);
					// unlock BS
					SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
					p_ctx_p->gISF_VdoDec_IsBsUsed[bs_idx] = FALSE;
					SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
					// unlock RAW
					SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
					p_ctx_p->gISF_VdoDec_IsRawUsed[raw_idx] = FALSE;
					SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
					return ISF_ERR_BUFFER_GET;
				}
			}

			ReadyBSInfo.uiBSAddr     = pVdoBuf->DataAddr;
			ReadyBSInfo.uiBSSize     = pVdoBuf->DataSize;
			ReadyBSInfo.uiRawAddr    = raw_buf.addr; //buf_addr;
			ReadyBSInfo.uiRawSize    = raw_buf.size; //buf_size;
			ReadyBSInfo.uiThisFrmIdx = pVdoBuf->count;
			ReadyBSInfo.bIsEOF       = 0; // need this?
			ReadyBSInfo.TimeStamp    = pVdoBuf->timestamp;
			ReadyBSInfo.BsBufID      = bs_idx;
			ReadyBSInfo.RawBufID     = raw_idx;
			if (nWaitMs == -99) {
				ReadyBSInfo.uiBSTotalSizeOneFrm = pVdoBuf->count;
				ReadyBSInfo.WaitMs		 = nWaitMs;
			}

			// input to VdoDec
			if (pNMI_VdoDec->In(id, (UINT32 *)&ReadyBSInfo) == E_SYS) {
				if (gISF_VdoDecMsgLevel[id] & ISF_VDODEC_MSG_BS) {
					DBG_ERR("[ISF_VDODEC][%d][%d] Stop, Release blk = (0x%08x, 0x%08x), time = %d us\r\n", id, bs_idx, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
				}
				// release BS
				isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pData, 0);
				(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_IN(id), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_PROCESS_FAIL); // [IN] PUSH_ERR
				SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
				p_ctx_p->gISF_VdoDec_IsBsUsed[bs_idx] = FALSE;
				SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
				// release RAW
				if (p_ctx_p->g_vdodec_trig_user_buf) {
					SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
					pRawData = &p_ctx_p->gISF_VdoDecRefRawData;
					pRawData->h_data = ReadyBSInfo.uiCommBufBlkID;
					isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pRawData, 0);
					SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
					isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), 0, ISF_ERR_INACTIVE_STATE);
					//g_vdodec_trig_user_buf[id] = FALSE;
				} else {
					isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), pRawData, 0);
					isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), 0, ISF_ERR_INACTIVE_STATE);
				}
				SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
				p_ctx_p->gISF_VdoDec_IsRawUsed[i] = FALSE;
				SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);

				return ISF_ERR_PROCESS_FAIL;
			}
		} else {
			// push to output port directly
			UINT32 i = 0;
			for (i = 0; i < PATH_MAX_COUNT; i++) {
				p_port = isf_vdodec.port_out[i-ISF_OUT(0)];
				if (isf_vdodec.p_base->get_is_allow_push(&isf_vdodec, p_port)) {
					isf_vdodec.p_base->do_push(&isf_vdodec, ISF_OUT(id), pData, 0);
				}
			}
		}
	} else {
		DBG_ERR("[%d] Invalid fourcc(0x%x)\r\n", id, pData->desc[0]);
		return ISF_ERR_INVALID_SIGN;
	}

	return ISF_OK;
}

#if (SUPPORT_PULL_FUNCTION)
static ISF_RV _ISF_VdoDec_OutputPort_PullImgBufFromSrc(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	VDODEC_CONTEXT_PORT   *p_ctx_p = NULL;
	ISF_RV ret;
	UINT32 id = oport - ISF_OUT_BASE;
	UINT32 i = 0;
	VDO_FRAME *p_vdo_frame = NULL;

	if (g_vd_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[oport];

	if (p_ctx_p->isf_vdodec_opened == FALSE) {
		DBG_ERR("module is not open yet\r\n");
		return ISF_ERR_NOT_OPEN_YET;
    }

	(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(oport), ISF_USER_PROBE_PULL, ISF_ENTER);  // [USER] PULL_ENTER

	ret = _ISF_VdoDec_Get_PullQueue(id, p_data, wait_ms);
	if (ret == ISF_OK) {
		// check decoded error code
		p_vdo_frame = (VDO_FRAME *)p_data->desc;
		if (p_vdo_frame->reserved[1] != E_OK) {
			// this will be treated as a decoded error
			p_data->h_data = p_vdo_frame->reserved[2]; // reserved[2] as yuv common blk
			isf_vdodec.p_base->do_release(&isf_vdodec, ISF_IN(id), p_data, 0);
			ret = ISF_ERR_INVALID_DATA;
		}

		// set RAW ISF_DATA queue empty (raw use common buffer, nvtmpp will handle the release mechanism)
		i = p_data->func; // we don't use private, so func will be used to save buf_id
		SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
		p_ctx_p->gISF_VdoDec_IsRawUsed[i] = FALSE;
		SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
	}

	return ret;
}

BOOL _ISF_VdoDec_isEmpty_PullQueue(UINT32 pathID)
{
	VDODEC_CONTEXT_PORT   *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[pathID];
	PVDODEC_PULL_QUEUE p_obj;
	BOOL is_empty = FALSE;

	SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);

	p_obj = &p_ctx_p->gISF_VdoDec_PullQueue;
	is_empty = ((p_obj->Front == p_obj->Rear) && (p_obj->bFull == FALSE));

	SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);

	return is_empty;
}

BOOL _ISF_VdoDec_isFull_PullQueue(UINT32 pathID)
{
	VDODEC_CONTEXT_PORT   *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[pathID];
	PVDODEC_PULL_QUEUE p_obj;
	BOOL is_full = FALSE;

	SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);

	p_obj = &p_ctx_p->gISF_VdoDec_PullQueue;
	is_full = ((p_obj->Front == p_obj->Rear) && (p_obj->bFull == TRUE));

	SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);

	return is_full;
}

void _ISF_VdoDec_Unlock_PullQueue(UINT32 pathID)
{
	VDODEC_CONTEXT_PORT   *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[pathID];

	if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_VDODEC_PULLQ_SEM_ID, vos_util_msec_to_tick(0))) {
		if (gISF_VdoDecMsgLevel[pathID] & ISF_VDODEC_MSG_RAW) {
			DBG_DUMP("[ISF_VDODEC][%d] no data in pull queue, auto unlock pull blocking mode\r\n", pathID);
		}
	}

	// [case_1]   no data in pullQ, then this 0 ->   0   -> 1 to unlock HDAL pull blocking mode (fake semaphore.... but it's OK, because _ISF_VdoDec_Get_PullQueue() will return FALSE, there's no actual data in queue )
	// [case_2] have data in pullQ, then this n -> (n-1) -> n to recover the semaphore count
	SEM_SIGNAL(p_ctx_p->ISF_VDODEC_PULLQ_SEM_ID);
}

BOOL _ISF_VdoDec_Put_PullQueue(UINT32 pathID, ISF_DATA *data_in)
{
	VDODEC_CONTEXT_PORT   *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[pathID];
	PVDODEC_PULL_QUEUE pObj;

	SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
	pObj = &p_ctx_p->gISF_VdoDec_PullQueue;

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		DBG_ERR("[ISF_VDODEC][%d] Pull Queue is Full!\r\n", pathID);
		SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
		return FALSE;
	} else {
		memcpy(&pObj->Queue[pObj->Rear], data_in, sizeof(ISF_DATA));

		pObj->Rear = (pObj->Rear + 1) % ISF_VDODEC_PULL_Q_MAX;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}
		SEM_SIGNAL(p_ctx_p->ISF_VDODEC_PULLQ_SEM_ID); // PULLQ + 1
		SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
		return TRUE;
	}
}

ISF_RV _ISF_VdoDec_Get_PullQueue(UINT32 pathID, ISF_DATA *data_out, INT32 wait_ms)
{
	VDODEC_CONTEXT_PORT   *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[pathID];
	PVDODEC_PULL_QUEUE pObj;

	if (wait_ms < 0) {
		// blocking (wait until data available) , if success PULLQ - 1 , else wait forever (or until signal interrupt and return FALSE)
		if (SEM_WAIT_INTERRUPTIBLE(p_ctx_p->ISF_VDODEC_PULLQ_SEM_ID)) {
			(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID),  ISF_USER_PROBE_PULL_WRN, ISF_ERR_QUEUE_EMPTY);  // [USER] PULL_WRN
			return ISF_ERR_QUEUE_EMPTY;
		}
	} else  {
		// non-blocking (wait_ms=0) , timeout (wait_ms > 0). If success PULLQ - 1 , else just return FALSE
		if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_VDODEC_PULLQ_SEM_ID, vos_util_msec_to_tick(wait_ms))) {
			DBG_IND("[ISF_VDODEC][%d] Pull Queue Semaphore timeout!\r\n", pathID);
			(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID),  ISF_USER_PROBE_PULL_WRN, ISF_ERR_WAIT_TIMEOUT);  // [USER] PULL_WRN
			return ISF_ERR_WAIT_TIMEOUT;
		}
	}

	// check state
	if (p_ctx_p->isf_vdodec_opened == FALSE) {
		return ISF_ERR_NOT_OPEN_YET;
	}

	SEM_WAIT(p_ctx_p->ISF_VDODEC_SEM_ID);
	pObj = &p_ctx_p->gISF_VdoDec_PullQueue;

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);
		if (p_ctx_p->gISF_VdoDec_Started == 1) { // only print when running
			DBG_ERR("[ISF_VDODEC][%d] Pull Queue is Empty\r\n", pathID);   // This should normally never happen !! If so, BUG exist. ( Because we already check & wait data available at upper code. )
			(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID),  ISF_USER_PROBE_PULL_WRN, ISF_ERR_QUEUE_EMPTY);  // [USER] PULL_WRN
		}
		return ISF_ERR_QUEUE_EMPTY;
	} else {
		memcpy(data_out, &pObj->Queue[pObj->Front], sizeof(ISF_DATA));

		pObj->Front= (pObj->Front+ 1) % ISF_VDODEC_PULL_Q_MAX;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = FALSE;
		}
		SEM_SIGNAL(p_ctx_p->ISF_VDODEC_SEM_ID);

		(&isf_vdodec)->p_base->do_probe(&isf_vdodec, ISF_OUT(pathID),  ISF_USER_PROBE_PULL, ISF_OK);  // [USER] PULL_OK
		return ISF_OK;
	}
}

BOOL _ISF_VdoDec_RelaseAll_PullQueue(UINT32 pathID)
{
	ISF_DATA data;

	while (ISF_OK == _ISF_VdoDec_Get_PullQueue(pathID, &data, 0)); //consume all pull queue

	return TRUE;
}

#endif // #if (SUPPORT_PULL_FUNCTION)

////////////////////////////////////////////////////////////////////////////////

static BOOL _ISF_VdoDec_ShowMsg(CHAR *strCmd)
{
	UINT32 uiPathID = 0, uiMsgLevel = 0;

	sscanf_s(strCmd, "%d %d", &uiPathID, &uiMsgLevel);

	if (uiPathID < ISF_VDODEC_IN_NUM) {
		gISF_VdoDecMsgLevel[uiPathID] = uiMsgLevel;
	} else {
		DBG_ERR("[ISF_VDODEC] invalid path id = %d\r\n", uiPathID);
	}

	return TRUE;
}

// cmd debug d0 o0 /mask=000
extern void _isf_flow_debug_port(char* unit_name, char* port_name, char* mask_name);
int _isf_vdodec_cmd_debug(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *mask;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d')) { _isf_flow_debug_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "vdodec"); // vdodec only has one device, and the name is "vdodec"

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
int _isf_vdodec_cmd_trace(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *mask;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d')) { _isf_flow_trace_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "vdodec"); // vdodec only has one device, and the name is "vdodec"

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
int _isf_vdodec_cmd_probe(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *mask;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d')) { _isf_flow_probe_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "vdodec"); // vdodec only has one device, and the name is "vdodec"

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
int _isf_vdodec_cmd_perf(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d') || (dev[1] !='0')) { _isf_flow_perf_port(0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "vdodec"); // vdodec only has one device, and the name is "vdodec"

	//verify i/o
	io = strsep(&cmdstr, delimiters);  // i0, i1, i2, o0, o1, o2 ...
	if((io == NULL) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_perf_port(0,0); return 0;}

	//do perf
	_isf_flow_perf_port(unit_name, io);

	return 0;
}

// cmd save d0 o0 count
extern void _isf_flow_save_port(char* unit_name, char* port_name, char* count_name);
int _isf_vdodec_cmd_save(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *count;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d') || (dev[1] !='0')) { _isf_flow_save_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "vdodec"); // vdodec only has one device, and the name is "vdodec"

	//verify i/o
	io = strsep(&cmdstr, delimiters);  // i0, i1, i2, o0, o1, o2 ...
	if((io == NULL) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_save_port(0,0,0); return 0;}

	//count
	count = strsep(&cmdstr, delimiters);

	//do save
	_isf_flow_save_port(unit_name, io, count);

	return 0;
}

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
static BOOL _isf_vdodec_cmd_debug_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_vdodec_cmd_debug(dev, cmdstr);
}

static BOOL _isf_vdodec_cmd_trace_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_vdodec_cmd_trace(dev, cmdstr);
}

static BOOL _isf_vdodec_cmd_probe_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_vdodec_cmd_probe(dev, cmdstr);
}

static BOOL _isf_vdodec_cmd_perf_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_vdodec_cmd_perf(dev, cmdstr);
}

static BOOL _isf_vdodec_cmd_save_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_vdodec_cmd_save(dev, cmdstr);
}
#endif // RTOS only

SXCMD_BEGIN(isfvd, "ISF_VdoDec command")
SXCMD_ITEM("showmsg %s", _ISF_VdoDec_ShowMsg, "show msg (Param: PathId Level(0:Disable, 1:bs_data, 2:lock) => showmsg 0 1)")
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
SXCMD_ITEM("debug %s", _isf_vdodec_cmd_debug_entry, "debug [dev] [i/o] [mask]  , debug port => debug d0 o0 mffff")
SXCMD_ITEM("trace %s", _isf_vdodec_cmd_trace_entry, "trace [dev] [i/o] [mask]  , trace port => trace d0 o0 mffff")
SXCMD_ITEM("probe %s", _isf_vdodec_cmd_probe_entry, "probe [dev] [i/o] [mask]  , probe port => probe d0 o0 meeee")
SXCMD_ITEM("perf  %s", _isf_vdodec_cmd_perf_entry,  "perf  [dev] [i/o]         , perf  port => perf d0 i0")
SXCMD_ITEM("save  %s", _isf_vdodec_cmd_save_entry,  "save  [dev] [i/o] [count] , save  port => save d0 i0 1")
SXCMD_ITEM("info  %s", _isf_vdodec_api_info,		"(no param) 			   , show  info")
#endif
SXCMD_END()

void _ISF_VdoDec_InstallCmd(void)
{
	sxcmd_addtable(isfvd);
}

int _isf_vdodec_cmd_isfdbg_showhelp(void)
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

int _isf_vdodec_cmd_isfvd_showhelp(void)
{
	UINT32 cmd_num = SXCMD_NUM(isfvd);
	UINT32 loop=1;

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
	DBG_DUMP("---------------------------------------------------------------------\n");
	DBG_DUMP("	vdec\n");
	DBG_DUMP("---------------------------------------------------------------------\n");
#else
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "isfvd");
	DBG_DUMP("=====================================================================\n");
#endif

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		DBG_DUMP("%15s : %s\r\n", isfvd[loop].p_name, isfvd[loop].p_desc);
	}

	return 0;
}

int _isf_vdodec_cmd_isfvd(char* sub_cmd_name, char *cmd_args)
{
	UINT32 cmd_num = SXCMD_NUM(isfvd);
	UINT32 loop=1;
	BOOL ret=FALSE;

	if (strncmp(sub_cmd_name, "?", 2) == 0) {
		_isf_vdodec_cmd_isfvd_showhelp();
		return 0;
	}

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(sub_cmd_name, isfvd[loop].p_name, strlen(sub_cmd_name)) == 0) {
			ret = isfvd[loop].p_func(cmd_args);
			return ret;
		}
	}

	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n  Usage :\r\n");
		_isf_vdodec_cmd_isfvd_showhelp();
		return -1;
	}

	return 0;
}


#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
MAINFUNC_ENTRY(vdec, argc, argv)
{
	char sub_cmd_name[33] = {0};
	char cmd_args[256]    = {0};
	UINT8 i = 0;

	if (argc == 1) {
		_isf_vdodec_cmd_isfvd_showhelp();
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

	_isf_vdodec_cmd_isfvd(sub_cmd_name, cmd_args);

	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////
static ISF_PORT_CAPS ISF_VdoDec_Input_Caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = ISF_VdoDec_InputPort_PushImgBufToDest,
};

static ISF_PORT_CAPS *ISF_VdoDec_InputPortList_Caps[ISF_VDODEC_IN_NUM] = {
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
	&ISF_VdoDec_Input_Caps,
};

/*static ISF_PORT ISF_VdoDec_InputPort[ISF_VDODEC_IN_NUM] = {
	{
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(0),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
	},
};*/
static ISF_PORT ISF_VdoDec_InputPort_IN1 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(0),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN2 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(1),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN3 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(2),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN4 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(3),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN5 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(4),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN6 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(5),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN7 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(6),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN8 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(7),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN9 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(8),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN10 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(9),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN11 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(10),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN12 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(11),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN13 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(12),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN14 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(13),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN15 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(14),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoDec_InputPort_IN16 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(15),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdodec,
	.p_srcunit = NULL,
};

static ISF_PORT *ISF_VdoDec_InputPortList[ISF_VDODEC_IN_NUM] = {
	&ISF_VdoDec_InputPort_IN1,
	&ISF_VdoDec_InputPort_IN2,
	&ISF_VdoDec_InputPort_IN3,
	&ISF_VdoDec_InputPort_IN4,
	&ISF_VdoDec_InputPort_IN5,
	&ISF_VdoDec_InputPort_IN6,
	&ISF_VdoDec_InputPort_IN7,
	&ISF_VdoDec_InputPort_IN8,
	&ISF_VdoDec_InputPort_IN9,
	&ISF_VdoDec_InputPort_IN10,
	&ISF_VdoDec_InputPort_IN11,
	&ISF_VdoDec_InputPort_IN12,
	&ISF_VdoDec_InputPort_IN13,
	&ISF_VdoDec_InputPort_IN14,
	&ISF_VdoDec_InputPort_IN15,
	&ISF_VdoDec_InputPort_IN16,
};

#if (SUPPORT_PULL_FUNCTION)
static ISF_PORT_CAPS ISF_VdoDec_OutputPort_Caps = {
	.connecttype_caps = ISF_CONNECT_PUSH  | ISF_CONNECT_PULL | ISF_CONNECT_SYNC,
	.do_pull   = _ISF_VdoDec_OutputPort_PullImgBufFromSrc,
};
#else
static ISF_PORT_CAPS ISF_VdoDec_OutputPort_Caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = NULL,
};
#endif

static ISF_PORT *ISF_VdoDec_OutputPortList[ISF_VDODEC_OUT_NUM] = {0};
static ISF_PORT *ISF_VdoDec_OutputPortList_Cfg[ISF_VDODEC_OUT_NUM] = {0};
static ISF_PORT_CAPS *ISF_VdoDec_OutputPortList_Caps[ISF_VDODEC_OUT_NUM] = {
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
	&ISF_VdoDec_OutputPort_Caps,
};

static ISF_STATE ISF_VdoDec_OutputPort_State[ISF_VDODEC_OUT_NUM] = {0};
static ISF_STATE *ISF_VdoDec_OutputPortList_State[ISF_VDODEC_OUT_NUM] = {
	&ISF_VdoDec_OutputPort_State[0],
	&ISF_VdoDec_OutputPort_State[1],
	&ISF_VdoDec_OutputPort_State[2],
	&ISF_VdoDec_OutputPort_State[3],
	&ISF_VdoDec_OutputPort_State[4],
	&ISF_VdoDec_OutputPort_State[5],
	&ISF_VdoDec_OutputPort_State[6],
	&ISF_VdoDec_OutputPort_State[7],
	&ISF_VdoDec_OutputPort_State[8],
	&ISF_VdoDec_OutputPort_State[9],
	&ISF_VdoDec_OutputPort_State[10],
	&ISF_VdoDec_OutputPort_State[11],
	&ISF_VdoDec_OutputPort_State[12],
	&ISF_VdoDec_OutputPort_State[13],
	&ISF_VdoDec_OutputPort_State[14],
	&ISF_VdoDec_OutputPort_State[15],
};

static ISF_INFO isf_vdodec_outputinfo_in[ISF_VDODEC_IN_NUM] = {0};
static ISF_INFO *isf_vdodec_outputinfolist_in[ISF_VDODEC_IN_NUM] = {
	&isf_vdodec_outputinfo_in[0],
	&isf_vdodec_outputinfo_in[1],
	&isf_vdodec_outputinfo_in[2],
	&isf_vdodec_outputinfo_in[3],
	&isf_vdodec_outputinfo_in[4],
	&isf_vdodec_outputinfo_in[5],
	&isf_vdodec_outputinfo_in[6],
	&isf_vdodec_outputinfo_in[7],
	&isf_vdodec_outputinfo_in[8],
	&isf_vdodec_outputinfo_in[9],
	&isf_vdodec_outputinfo_in[10],
	&isf_vdodec_outputinfo_in[11],
	&isf_vdodec_outputinfo_in[12],
	&isf_vdodec_outputinfo_in[13],
	&isf_vdodec_outputinfo_in[14],
	&isf_vdodec_outputinfo_in[15],
};

static ISF_INFO isf_vdodec_outputinfo_out[ISF_VDODEC_OUT_NUM] = {0};
static ISF_INFO *isf_vdodec_outputinfolist_out[ISF_VDODEC_OUT_NUM] = {
	&isf_vdodec_outputinfo_out[0],
	&isf_vdodec_outputinfo_out[1],
	&isf_vdodec_outputinfo_out[2],
	&isf_vdodec_outputinfo_out[3],
	&isf_vdodec_outputinfo_out[4],
	&isf_vdodec_outputinfo_out[5],
	&isf_vdodec_outputinfo_out[6],
	&isf_vdodec_outputinfo_out[7],
	&isf_vdodec_outputinfo_out[8],
	&isf_vdodec_outputinfo_out[9],
	&isf_vdodec_outputinfo_out[10],
	&isf_vdodec_outputinfo_out[11],
	&isf_vdodec_outputinfo_out[12],
	&isf_vdodec_outputinfo_out[13],
	&isf_vdodec_outputinfo_out[14],
	&isf_vdodec_outputinfo_out[15],
};

static ISF_RV ISF_VdoDec_BindInput(struct _ISF_UNIT *pThisUnit, UINT32 iPort, struct _ISF_UNIT *pSrcUnit, UINT32 oPort)
{
	return ISF_OK;
}

static ISF_RV ISF_VdoDec_BindOutput(struct _ISF_UNIT *pThisUnit, UINT32 oPort, struct _ISF_UNIT *pDestUnit, UINT32 iPort)
{
	return ISF_OK;
}

static ISF_RV ISF_VdoDec_SetParam(ISF_UNIT *pThisUnit, UINT32 param, UINT32 value)
{
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;

	if (!p_ctx_c->isf_vdodec_init) {
		nvtmpp_vb_add_module(pThisUnit->unit_module);
		_isf_vdodec_raw_data_init();
		_ISF_VdoDec_InstallCmd();
		NMP_VdoDec_AddUnit();
		p_ctx_c->isf_vdodec_init = TRUE;
	}

	if (!pNMI_VdoDec) {
		if ((pNMI_VdoDec = NMI_GetUnit("VdoDec")) == NULL) {
			DBG_ERR("Get VdoDec unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	switch (param) {
	case VDODEC_PARAM_EVENT_CB:
		p_ctx_c->gfn_vdodec_cb = (IsfVdoDecEventCb *)value;
		break;
	default:
		DBG_ERR("Invalid param = 0x%x\r\n", param);
		break;
	}

	return ISF_OK;
}

static UINT32 ISF_VdoDec_GetParam(ISF_UNIT *pThisUnit, UINT32 param)
{
	return 0;
}

static ISF_RV ISF_VdoDec_SetPortParam(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param, UINT32 value)
{
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;
	VDODEC_CONTEXT_PORT   *p_ctx_p;
	ISF_RV 		  ret = ISF_OK;

    if (nPort == ISF_CTRL)
    {
        return ISF_VdoDec_SetParam(pThisUnit, param, value);
    }

	if (!p_ctx_c->isf_vdodec_init) {
		nvtmpp_vb_add_module(pThisUnit->unit_module);
		_isf_vdodec_raw_data_init();
		_ISF_VdoDec_InstallCmd();
		NMP_VdoDec_AddUnit();
		p_ctx_c->isf_vdodec_init = TRUE;
	}

	// Check whether NMI_VdoDec exists?
	if (!pNMI_VdoDec) {
		if ((pNMI_VdoDec = NMI_GetUnit("VdoDec")) == NULL) {
			DBG_ERR("Get VdoDec unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	if (g_vd_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

	if ((nPort < (int)ISF_IN_BASE) && (nPort < pThisUnit->port_out_count)) { // Output port
		switch (param) {
		/*case VDODEC_PARAM_PORT_OUTPUT_FMT:
			if (gISF_OutputFmt[nPort - ISF_OUT_BASE] != value) {
				gISF_OutputFmt[nPort - ISF_OUT_BASE] = value;
				ret = ISF_APPLY_DOWNSTREAM_READYTIME;
				ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, pThisUnit, nPort, "output_fmt=%d\r\n", value); // typedef VDODEC_OUTPUT
			} else {
				ret = ISF_OK;
			}
			break;*/

		#if 0  // [isf_flow] doesn't support "ISF_APPLY_DOWNSTREAM_OFFTIME"
		case ISF_PARAM_PORT_DIRTY:
		{
			//#NT#2018/08/07#Adam Su -begin
            //#NT# mark dirty checking, or it will cause close->start(offtime update)
			/*ISF_PORT 	 *pDest = NULL;
			ISF_IMG_INFO *pImgInfo = NULL;

			pDest = ImageUnit_Out(pThisUnit, nPort);
			pImgInfo = (ISF_IMG_INFO *) & (pDest->Info);
			DBG_DUMP("w=%d, h=%d\r\n", pImgInfo->ImgSize.w, pImgInfo->ImgSize.h);
			DBG_DUMP("Vdo W=%d, H=%d\r\n", gISF_VdoWidth, gISF_VdoHeight);
			if (pImgInfo->ImgSize.w != gISF_VdoWidth || pImgInfo->ImgSize.h != gISF_VdoHeight) {
				ret = ISF_APPLY_DOWNSTREAM_OFFTIME;
			} else {
				ret = ISF_OK;
			}*/
			//#NT#2018/08/07#Adam Su -end
			break;
		}
		#endif

		default:
			DBG_ERR("[%d] Invalid OUT paramter (0x%x)\r\n", nPort - ISF_OUT_BASE, param);
			ret = ISF_ERR_INVALID_PARAM;
			break;
		}
	} else if (nPort-ISF_IN_BASE < pThisUnit->port_in_count) { // Input port
		p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[nPort - ISF_IN_BASE];

		switch (param) {
		case VDODEC_PARAM_CODEC:
			{
				PMP_VDODEC_DECODER p_video_obj = NULL;
				UINT32 id = nPort - ISF_IN_BASE;

				gISF_VdoCodecFmt[id] = value;
				pNMI_VdoDec->SetParam(id, NMI_VDODEC_PARAM_CODEC, gISF_VdoCodecFmt[id]);

				switch (gISF_VdoCodecFmt[id]) {
				case MEDIAPLAY_VIDEO_MJPG:	p_video_obj = MP_MjpgDec_getVideoObject((MP_VDODEC_ID)id);		break;
				case MEDIAPLAY_VIDEO_H264:	p_video_obj = MP_H264Dec_getVideoObject((MP_VDODEC_ID)id);		break;
				case MEDIAPLAY_VIDEO_H265:	p_video_obj = MP_H265Dec_getVideoObject((MP_VDODEC_ID)id);		break;
				case 0: // close reset to 0
					return ISF_OK;
				default:
					DBG_ERR("[ISF_VDODEC][%d] getVideoObject failed\r\n", id);
					return ISF_ERR_FAILED;
				}

				pNMI_VdoDec->SetParam(id, NMI_VDODEC_PARAM_DECODER_OBJ, (UINT32)p_video_obj);
			}
			break;

		case VDODEC_PARAM_WIDTH:
			// user do not need to set this
			/*gISF_VdoWidth =	value;
			pNMI_VdoDec->SetParam(0, NMI_VDODEC_PARAM_WIDTH, gISF_VdoWidth);
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM, pThisUnit, nPort, "w=%d\r\n", gISF_VdoWidth);*/
			break;

		case VDODEC_PARAM_HEIGHT:
			// user do not need to set this
			/*gISF_VdoHeight = value;
			pNMI_VdoDec->SetParam(0, NMI_VDODEC_PARAM_HEIGHT, gISF_VdoHeight);
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM, pThisUnit, nPort, "h=%d\r\n", gISF_VdoHeight);*/
			break;

		case VDODEC_PARAM_GOP:
			{
				UINT32 id = nPort - ISF_IN_BASE;

				pNMI_VdoDec->SetParam(id, NMI_VDODEC_PARAM_GOP, value);
				ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, pThisUnit, nPort, "gop=%d\r\n", value);
			}
			break;

		case VDODEC_PARAM_YUV_AUTO_DROP:
			{
				p_ctx_p->isf_vdodec_yuv_auto_drop = TRUE;
			}
			break;

		case VDODEC_PARAM_RAWQUE_MAX_NUM:
			p_ctx_p->gISF_VdoDecRawQue = value;
			break;
		/*case VDODEC_PARAM_MAX_MEM_INFO:
		    {
				ISF_RV r = ISF_OK;
				NMI_VDODEC_MAX_MEM_INFO* pMaxMemInfo = (NMI_VDODEC_MAX_MEM_INFO*) value;

				if (pMaxMemInfo->bRelease) {
					if (gISF_VdoDecMaxMem[nPort-ISF_IN_BASE].Addr != 0) {
						r = pThisUnit->pBase->ReleaseI(pThisUnit, &(gISF_VdoDecMaxMemBlk[nPort-ISF_IN_BASE]), ISF_OK);
						if (r == ISF_OK) {
							gISF_VdoDecMaxMem[nPort-ISF_IN_BASE].Addr = 0;
							gISF_VdoDecMaxMem[nPort-ISF_IN_BASE].Size = 0;
							gISF_VdoDecMaxMemSize[nPort-ISF_IN_BASE] = 0;
							ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, pThisUnit, nPort, "(release buf)\r\n");
						} else {
							DBG_ERR("[ISF_VDODEC][%d] release VdoDec Max blk failed, ret = %d\r\n", nPort-ISF_IN_BASE, r);
						}
					}
				} else {
					if (gISF_VdoDecMaxMem[nPort-ISF_IN_BASE].Addr == 0) {
                        // Set max buf info and get calculated buf size
						pNMI_VdoDec->SetParam(nPort-ISF_IN_BASE, NMI_VDODEC_PARAM_MAX_MEM_INFO, (UINT32)pMaxMemInfo);
						gISF_VdoDecMaxMemSize[nPort-ISF_IN_BASE] = pMaxMemInfo->uiDecsize;
						ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, pThisUnit, nPort, "maxbuf=0x%x\r\n", pMaxMemInfo->uiDecsize);
					} else {
						DBG_ERR("[ISF_VDODEC][%d] max buf exist, please release it first.\r\n", nPort-ISF_IN_BASE);
					}
				}
			}
            break;*/

		default:
			DBG_ERR("[%d] Invalid IN paramter (0x%x)\r\n", nPort-ISF_IN_BASE, param);
			break;
		}
	}

	return ret;
}

static UINT32 ISF_VdoDec_GetPortParam(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param)
{
	UINT32 value = 0;

    if (nPort == ISF_CTRL)
    {
        return ISF_VdoDec_GetParam(pThisUnit, param);
    }

	if (!pNMI_VdoDec) {
		if ((pNMI_VdoDec = NMI_GetUnit("VdoDec")) == NULL) {
			DBG_ERR("Get VdoDec unit failed\r\n");
			return value;
		}
	}

	if (g_vd_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

	if ((nPort < (int)ISF_IN_BASE) && (nPort < pThisUnit->port_out_count)) { // Output port
		VDODEC_CONTEXT_PORT   *p_ctx_p = NULL;
		if (nPort-ISF_OUT_BASE >= PATH_MAX_COUNT) {
			DBG_ERR("[VDODEC] invalid path index = %d\r\n", nPort-ISF_OUT_BASE);
			return ISF_ERR_INVALID_PORT_ID;
		}
		p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[nPort - ISF_OUT_BASE];

		switch (param) {
		case VDODEC_PARAM_CODEC:
			{
				UINT32 id = nPort - ISF_IN_BASE;
				pNMI_VdoDec->GetParam(id, NMI_VDODEC_PARAM_CODEC, &value);
			}
			break;

		case VDODEC_PARAM_WIDTH:
			{
				UINT32 id = nPort - ISF_IN_BASE;
				pNMI_VdoDec->GetParam(id, NMI_VDODEC_PARAM_WIDTH, &value);
			}
			break;

		case VDODEC_PARAM_HEIGHT:
			{
				UINT32 id = nPort - ISF_IN_BASE;
				pNMI_VdoDec->GetParam(id, NMI_VDODEC_PARAM_HEIGHT, &value);
			}
			break;

		case VDODEC_PARAM_GOP:
			{
				UINT32 id = nPort - ISF_IN_BASE;
				pNMI_VdoDec->GetParam(id, NMI_VDODEC_PARAM_GOP, &value);
			}
			break;

#if (SUPPORT_PULL_FUNCTION)
		case VDODEC_PARAM_BUFINFO_PHYADDR:
			value = p_ctx_p->gISF_VdoDecMem_MmapInfo.addr_physical;
			break;

		case VDODEC_PARAM_BUFINFO_SIZE:
			value = p_ctx_p->gISF_VdoDecMem_MmapInfo.size;
			break;
#endif
		case VDODEC_PARAM_DEC_STATUS:
			value = p_ctx_p->gISF_VdoDec_Started;
			break;

		default:
			DBG_ERR("Invalid OUTPUT paramter (0x%x)\r\n", param);
			break;
		}
	} else if (nPort-ISF_IN_BASE < pThisUnit->port_in_count) { // Input port
		switch (param) {
		default:
			DBG_ERR("Invalid INPUT paramter (0x%x)\r\n", param);
			value = ISF_ERR_NOT_SUPPORT;
			break;
		}
	}

	return value;
}

static ISF_RV ISF_VdoDec_SetPortStruct(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param, UINT32* p_struct, UINT32 size)
{
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;
	ISF_RV ret = ISF_OK;
	UINT32 value = (UINT32)p_struct;

	if (!p_ctx_c->isf_vdodec_init) {
		nvtmpp_vb_add_module(pThisUnit->unit_module);
		_isf_vdodec_raw_data_init();
		_ISF_VdoDec_InstallCmd();
		NMP_VdoDec_AddUnit();
		p_ctx_c->isf_vdodec_init = TRUE;
	}

	if (!pNMI_VdoDec) {
		if ((pNMI_VdoDec = NMI_GetUnit("VdoDec")) == NULL) {
			DBG_ERR("Get VdoDec unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	if (g_vd_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

	if ((nPort < (int)ISF_IN_BASE) && (nPort < pThisUnit->port_out_count)) { // OUT port
		VDODEC_CONTEXT_PORT   *p_ctx_p = NULL;
		if (nPort-ISF_OUT_BASE >= PATH_MAX_COUNT) {
			DBG_ERR("[VDODEC] invalid path index = %d\r\n", nPort-ISF_OUT_BASE);
			return ISF_ERR_INVALID_PORT_ID;
		}
		p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[nPort-ISF_OUT_BASE];

		switch (param) {
		case VDODEC_PARAM_MAX_MEM_INFO:
		    {
				ISF_RV r = ISF_OK;
				NMI_VDODEC_MAX_MEM_INFO* pMaxMemInfo = (NMI_VDODEC_MAX_MEM_INFO*) value;

				if (pMaxMemInfo->bRelease) {
					if (p_ctx_p->gISF_VdoDecMaxMem.Addr != 0) {
						r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_VdoDecMaxMemBlk), ISF_OK);
						if (r == ISF_OK) {
							p_ctx_p->gISF_VdoDecMaxMem.Addr = 0;
							p_ctx_p->gISF_VdoDecMaxMem.Size = 0;
							p_ctx_p->gISF_VdoDecMaxMemSize = 0;
						} else {
							DBG_ERR("[ISF_VDODEC][%d] release VdoDec Max blk failed, ret = %d\r\n", nPort-ISF_OUT_BASE, r);
							return ISF_ERR_BUFFER_DESTROY;
						}
					}
				} else {
					if (p_ctx_p->gISF_VdoDecMaxMem.Addr == 0) {
                        // Set max buf info and get calculated buf size
						pNMI_VdoDec->SetParam(nPort-ISF_OUT_BASE, NMI_VDODEC_PARAM_MAX_MEM_INFO, (UINT32)pMaxMemInfo);
						p_ctx_p->gISF_VdoDecMaxMemSize = pMaxMemInfo->uiDecsize;
					} else {
						DBG_ERR("[ISF_VDODEC][%d] max buf exist, please release it first.\r\n", nPort-ISF_OUT_BASE);
						return ISF_ERR_BUFFER_ADD;
					}
				}
			}
			break;

		case VDODEC_PARAM_USER_OUT_BUF:
			{
				VDO_FRAME *p_user_out_buf = (VDO_FRAME *)value;
				UINT32 id = nPort-ISF_OUT_BASE;

				memcpy(&p_ctx_p->g_vdodec_user_buf, p_user_out_buf, sizeof(VDO_FRAME));

				DBG_DUMP("[%d] set user_buf: sign=0x%x, ddr=%d, pxlfmt=0x%x, w=%d, h=%d, pa=0x%x, blk=0x%x\r\n",
						id,
						p_ctx_p->g_vdodec_user_buf.sign,
						p_ctx_p->g_vdodec_user_buf.resv1,
						p_ctx_p->g_vdodec_user_buf.pxlfmt,
						p_ctx_p->g_vdodec_user_buf.size.w,
						p_ctx_p->g_vdodec_user_buf.size.h,
						p_ctx_p->g_vdodec_user_buf.phyaddr[0],
						p_ctx_p->g_vdodec_user_buf.reserved[0]);

					p_ctx_p->g_vdodec_trig_user_buf = 1;
			}
			break;

		default:
			DBG_ERR("vdodec.out[%d] set struct[%08x] = %08x\r\n", nPort-ISF_OUT_BASE, param, value);
			ret = ISF_ERR_NOT_SUPPORT;
			break;
		}

		return ret;
	} else if (nPort-ISF_IN_BASE < pThisUnit->port_in_count) {
		switch (param) {
		case VDODEC_PARAM_DESC: {
			UINT32 id = nPort - ISF_IN_BASE;
			NMI_VDODEC_MEM_RANGE *p_mem = (NMI_VDODEC_MEM_RANGE *)value;
#ifdef __KERNEL__
			UINT32 __user *p_user = (UINT32 __user *)p_mem->Addr;

			//DBG_DUMP("[%d]desc a=0x%x, s=0x%x, nPort=%d\r\n", id, p_mem->Addr, p_mem->Size, nPort);

			if (p_mem->Size > NMI_VDODEC_H26X_NAL_MAXSIZE) {
				DBG_ERR("[%d] invalid nal size (%d\r\n)", id, p_mem->Size);
				break;
			}

			// copy from user
			if (isf_vdodec_copy_from_user((void *)&g_vdodec_desc[id], (void *)p_user, p_mem->Size) == FALSE) {
				DBG_ERR("[%d] copy_from_user(0x%X, 0x%X, 0x%X)\r\n", id, (UINT32)&g_vdodec_desc[id], (UINT32)p_user, p_mem->Size);
				break;
			}
#if 0
			{
				UINT32 i = 0;
				UINT8 *ptr = (UINT8 *)&g_vdodec_desc[id];
				for (i = 0; i<2; i++) {
					DBG_DUMP("%x %x %x %x ", *(ptr), *(ptr + 1), *(ptr + 2), *(ptr + 3));
				}
				DBG_DUMP("\r\n");
			}
#endif
			pNMI_VdoDec->SetParam(id, NMI_VDODEC_PARAM_DESC_ADDR, (UINT32)&g_vdodec_desc[id]);
			pNMI_VdoDec->SetParam(id, NMI_VDODEC_PARAM_DESC_LEN, p_mem->Size);
#else
			if (p_mem->Size > NMI_VDODEC_H26X_NAL_MAXSIZE) {
				DBG_ERR("[%d] invalid nal size (%d\r\n)", id, p_mem->Size);
				break;
			}

			pNMI_VdoDec->SetParam(id, NMI_VDODEC_PARAM_DESC_ADDR, p_mem->Addr);
			pNMI_VdoDec->SetParam(id, NMI_VDODEC_PARAM_DESC_LEN, p_mem->Size);
#endif
			// set flag to update (in ISF_PORT_CMD_RUNTIME_UPDATE)
			if (g_vdodec_desc_is_set[id] == 0) {
				if (p_mem->Addr && p_mem->Size) { // only update when mem has value
					g_vdodec_desc_is_set[id] = 1;
				}
			} else {
				DBG_ERR("[%d] h.26x desc already set, but not apply\r\n", id);
				break;
			}
		}
		break;

		default:
			DBG_ERR("vdodec.in[%d] set struct[%08x] = %08x\r\n", nPort-ISF_IN_BASE, param, value);
			ret = ISF_ERR_NOT_SUPPORT;
			break;
		}

		return ret;
	} else {
		return ISF_ERR_NOT_SUPPORT;
	}
}

// [ToDo]
static ISF_RV ISF_VdoDec_GetPortStruct(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param, UINT32* p_struct, UINT32 size)
{
	if (!pNMI_VdoDec) {
		if ((pNMI_VdoDec = NMI_GetUnit("VdoDec")) == NULL) {
			DBG_ERR("Get VdoDec unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	if ((nPort < (int)ISF_IN_BASE) && (nPort < pThisUnit->port_out_count)) { // Output port
		if (nPort-ISF_OUT_BASE >= PATH_MAX_COUNT) {
			DBG_ERR("[VDODEC] invalid path index = %d\r\n", nPort-ISF_OUT_BASE);
			return ISF_ERR_INVALID_PORT_ID;
		}

		switch (param) {
		case VDODEC_PARAM_VUI_INFO:
			pNMI_VdoDec->GetParam(nPort, NMI_VDODEC_PARAM_VUI_INFO, p_struct);
			break;
		case VDODEC_PARAM_JPEGINFO:
			pNMI_VdoDec->GetParam(nPort, NMI_VDODEC_PARAM_JPEGINFO, p_struct);
			break;
		}
	}
	return ISF_OK;
}

static ISF_RV ISF_VdoDec_UpdatePort(struct _ISF_UNIT *pThisUnit, UINT32 oPort, ISF_PORT_CMD cmd)
{
	ISF_RV r = ISF_OK;
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;
	VDODEC_CONTEXT_PORT   *p_ctx_p = NULL;

	if (oPort-ISF_OUT_BASE >= PATH_MAX_COUNT) {
		if (cmd != ISF_PORT_CMD_RESET) DBG_ERR("[VDODEC] invalid path index = %d\r\n", oPort-ISF_OUT_BASE);
		return ISF_ERR_INVALID_PORT_ID;
	}

	if (g_vd_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[oPort];

	if (!p_ctx_c->isf_vdodec_init) {
		nvtmpp_vb_add_module(pThisUnit->unit_module);
		_isf_vdodec_raw_data_init();
		_ISF_VdoDec_InstallCmd();
		NMP_VdoDec_AddUnit();
		p_ctx_c->isf_vdodec_init = TRUE;
	}

	if (!pNMI_VdoDec) {
		if ((pNMI_VdoDec = NMI_GetUnit("VdoDec")) == NULL) {
			DBG_ERR("Get VdoDec unit failed\r\n");
			return ISF_ERR_FAILED;
		}
	}

	switch (cmd) {
		case ISF_PORT_CMD_RESET:
			_ISF_VdoDec_Reset(pThisUnit, oPort);
			break;

		case ISF_PORT_CMD_OPEN:									///< off -> ready   (alloc mempool and start task)
			//_ISF_VdoDec_UpdatePortInfo(pThisUnit, oPort);
			if (_ISF_VdoDec_Open(pThisUnit, oPort) != ISF_OK) {
				DBG_ERR("_ISF_VdoDec_Open fail\r\n");
				return ISF_ERR_FAILED;
			}
			break;

		case ISF_PORT_CMD_READYTIME_SYNC:						///< ready -> ready (apply ready-time parameter if it is dirty)
			_ISF_VdoDec_UpdatePortInfo(pThisUnit, oPort);
			break;

		case ISF_PORT_CMD_START:								///< ready -> run   (initial context, engine enable and device power on, start data processing)
			{
				UINT32 id = oPort-ISF_OUT_BASE;
				ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(start)\r\n");

				bValidBs[id] = FALSE;
				// "first time" START, alloc memory
				{
					//if (p_ctx_p->gISF_VdoDecMaxMem.Addr == 0 || p_ctx_p->gISF_VdoDecMaxMem.Size == 0) {
					if (p_ctx_p->gISF_VdoDec_Is_AllocMem == FALSE) { // jira: NA51055-725, repeat allocate bs mem
						if (p_ctx_p->gISF_VdoDecMaxMemSize == 0 && gISF_VdoCodecFmt[id] != MEDIAPLAY_VIDEO_MJPG) {  // jpeg use yuv common buffer only.
							DBG_ERR("[ISF_VDODEC][%d] Fatal Error, please set MAX_MEM first !!\r\n", oPort);
							return ISF_ERR_FAILED;
						}

						ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, pThisUnit, oPort, "(alloc max memory)");
						if ((r = _ISF_VdoDec_AllocMem(pThisUnit, oPort)) != ISF_OK) {
							DBG_ERR("[ISF_VDODEC][%d] AllocMem failed\r\n", oPort);
							p_ctx_p->gISF_VdoDec_Is_AllocMem = FALSE;
							return r;
						} else {
							p_ctx_p->gISF_VdoDec_Is_AllocMem = TRUE;
						}
					}
				}

				_ISF_VdoDec_OutputPort_Enable(id);

				if (p_ctx_p->gISF_VdoDecMaxMemSize > 0 && gISF_VdoCodecFmt[id] != MEDIAPLAY_VIDEO_MJPG) {
					NMI_VDODEC_MEM_RANGE Mem    = {0};
					UINT32           uiBufSize  = 0;
					pNMI_VdoDec->GetParam(id, NMI_VDODEC_PARAM_ALLOC_SIZE, &uiBufSize);

					//check MAX_MEM & memory requirement for current setting
					if (uiBufSize > p_ctx_p->gISF_VdoDecMaxMem.Size || uiBufSize == 0) {
						DBG_ERR("Invalid need memory(%d), allocated MAX_MEM(%d)\r\n", uiBufSize, p_ctx_p->gISF_VdoDecMaxMem.Size);
						return ISF_ERR_FAILED;
					}

					//reassign memory layout (user may open->start->stop->change codec->start, like vdoenc in JIRA: IPC_680-128
					Mem.Addr = p_ctx_p->gISF_VdoDecMaxMem.Addr;
					Mem.Size = uiBufSize;
					pNMI_VdoDec->SetParam(id, NMI_VDODEC_PARAM_MEM_RANGE, (UINT32) &Mem);
				}

				_ISF_VdoDec_FreeMem_RawOnly(pThisUnit, id);           // free all raw in isf_unit queue , like vdoenc in JIRA: IPC_680-120
				pNMI_VdoDec->Action(oPort, NMI_VDODEC_ACTION_START); // free all bs in nmedia queue

				p_ctx_p->gISF_VdoDec_Started = 1;
				g_vdodec_desc_is_set[id] = 0; // reset after apply
			}
			break;

		case ISF_PORT_CMD_PAUSE:								///< run -> wait    (pause data processing, keep context, engine or device enter suspend mode)
			break;

		case ISF_PORT_CMD_RESUME:								///< wait -> run    (engine or device leave suspend mode, restore context, resume data processing)
			break;

		case ISF_PORT_CMD_STOP:	{								///< run -> stop    (stop data processing, engine disable or device power off)
				UINT32 id = oPort-ISF_OUT_BASE;

				p_ctx_p->gISF_VdoDec_Started = 0;

				memset(&p_ctx_p->gISF_VdoDec_IsBsUsed, 0, sizeof(p_ctx_p->gISF_VdoDec_IsBsUsed));

				ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(stop)\r\n");
				_ISF_VdoDec_OutputPort_Disable(id);
				_ISF_VdoDec_Unlock_PullQueue(id);
				pNMI_VdoDec->Action(oPort, NMI_VDODEC_ACTION_STOP);
			}
			break;

		case ISF_PORT_CMD_CLOSE:								///< stop -> off    (terminate task and free mempool)
			_ISF_VdoDec_Close(pThisUnit, oPort);

			// let pull_out(blocking mode) auto-unlock
			_ISF_VdoDec_Unlock_PullQueue(oPort);
			break;

		case ISF_PORT_CMD_OFFTIME_SYNC:							///< off -> off     (apply off-time parameter if it is dirty)
			_ISF_VdoDec_UpdatePortInfo(pThisUnit, oPort);
			break;

		case ISF_PORT_CMD_RUNTIME_UPDATE: {						///< run -> run     (apply run-time parameter if it is dirty)
			UINT32 id = oPort-ISF_OUT_BASE;
			if (g_vdodec_desc_is_set[id] == 1) {
				// do stop & start [ToDo]

				// reset flag
				g_vdodec_desc_is_set[id] = 0;
			}
			break;
		}

		case ISF_PORT_CMD_RUNTIME_SYNC:							///< run -> run     (run-time property is dirty)
			break;

		default:
			DBG_ERR("Invalid port command (0x%x)\r\n", cmd);
			break;
	}

	return r;
}

void isf_vdodec_install_id(void)
{
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;
	UINT32 i = 0;

    for (i = 0; i < PATH_MAX_COUNT; i++) {
        VDODEC_CONTEXT_PORT   *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[i];

        OS_CONFIG_SEMPHORE(p_ctx_p->ISF_VDODEC_SEM_ID, 0, 1, 1);
#if (SUPPORT_PULL_FUNCTION)
		OS_CONFIG_SEMPHORE(p_ctx_p->ISF_VDODEC_PULLQ_SEM_ID, 0, 0, 0);
#endif
    }
	OS_CONFIG_SEMPHORE(p_ctx_c->ISF_VDODEC_COMMON_SEM_ID, 0, 1, 1);

	OS_CONFIG_SEMPHORE(ISF_VDODEC_PROC_SEM_ID, 0, 1, 1);
}

void isf_vdodec_uninstall_id(void)
{
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;
	UINT32 i = 0;

    for (i = 0; i < PATH_MAX_COUNT; i++) {
        VDODEC_CONTEXT_PORT   *p_ctx_p = (VDODEC_CONTEXT_PORT *)&g_vd_ctx.port[i];

        SEM_DESTROY(p_ctx_p->ISF_VDODEC_SEM_ID);
#if (SUPPORT_PULL_FUNCTION)
		SEM_DESTROY(p_ctx_p->ISF_VDODEC_PULLQ_SEM_ID);
#endif
    }
	SEM_DESTROY(p_ctx_c->ISF_VDODEC_COMMON_SEM_ID);

	SEM_DESTROY(ISF_VDODEC_PROC_SEM_ID);
}


/*ISF_DATA gISF_VdoDecRawData = {
	.Sign      = ISF_SIGN_DATA,                 ///< signature, equal to ISF_SIGN_DATA or ISF_SIGN_EVENT
	.hData     = 0,                             ///< handle of real data, it will be "nvtmpp blk_id", or "custom data handle"
	.pLockCB   = _ISF_VdoDec_LockCB,            ///< CB to lock "custom data handle"
	.pUnlockCB = _ISF_VdoDec_UnLockCB,          ///< CB to unlock "custom data handle"
	.Event     = 0,                             ///< default 0
	.Serial    = 0,                             ///< serial id
	.TimeStamp = 0,                             ///< time-stamp
};*/

static ISF_DATA_CLASS _isf_vdodec_base = {
	.this = MAKEFOURCC('V','D','E','C'),
	.base = MAKEFOURCC('V','F','R','M'),
    .do_lock   = _ISF_VdoDec_LockCB,
    .do_unlock = _ISF_VdoDec_UnLockCB,
};

static ISF_RV _isf_vdodec_raw_data_init(void)
{
	isf_data_regclass(&_isf_vdodec_base);
#if _USE_COMMON_FOR_RAW
	return ISF_OK;
#else
	return isf_vdodec.p_base->init_data(&gISF_VdoDecRawData, NULL, MAKEFOURCC('V','D','E','C'), 0x00010000); // [isf_flow] use this ...
#endif
}

//----------------------------------------------------------------------------

static UINT32 _isf_vdodec_query_context_size(void)
{
	return (PATH_MAX_COUNT * sizeof(VDODEC_CONTEXT_PORT));
}
extern UINT32 _nmp_vdodec_query_context_obj_size(void);
extern UINT32 _nmp_vdodec_query_context_cfg_size(void);

static UINT32 _isf_vdodec_query_context_memory(void)
{
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;

	p_ctx_c->ctx_mem.isf_vdodec_size = _isf_vdodec_query_context_size();
	p_ctx_c->ctx_mem.nm_vdodec_obj_size = _nmp_vdodec_query_context_obj_size();
	p_ctx_c->ctx_mem.nm_vdodec_cfg_size = _nmp_vdodec_query_context_cfg_size();

	return (p_ctx_c->ctx_mem.isf_vdodec_size) + (p_ctx_c->ctx_mem.nm_vdodec_obj_size) + (p_ctx_c->ctx_mem.nm_vdodec_cfg_size);
}

//----------------------------------------------------------------------------

static void _isf_vdodec_assign_context(UINT32 addr)
{
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;

	g_vd_ctx.port = (VDODEC_CONTEXT_PORT*)addr;
	memset((void *)g_vd_ctx.port, 0, p_ctx_c->ctx_mem.isf_vdodec_size);
}
extern void _nmp_vdodec_assign_context_obj(UINT32 addr);
extern void _nmp_vdodec_assign_context_cfg(UINT32 addr);

static ER _isf_vdodec_assign_context_memory_range(UINT32 addr)
{
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;

	// [ isf_vdodec ]
	_isf_vdodec_assign_context(addr);
	addr += p_ctx_c->ctx_mem.isf_vdodec_size;

	// [ nmedia_vdodec obj ]
	_nmp_vdodec_assign_context_obj(addr);
	addr += p_ctx_c->ctx_mem.nm_vdodec_obj_size;

	// [ nmedia_vdodec config ]
	_nmp_vdodec_assign_context_cfg(addr);

	return E_OK;
}

//----------------------------------------------------------------------------

static void _isf_vdodec_free_context(void)
{
	g_vd_ctx.port = NULL;
}
extern void _nmp_vdodec_free_context(void);

static ER _isf_vdodec_free_context_memory_range(void)
{
	// [ isf_vdodec ]
	_isf_vdodec_free_context();

	// [ nmedia_vdodec ]
	_nmp_vdodec_free_context();

	return E_OK;
}

//----------------------------------------------------------------------------

static ISF_RV _isf_vdodec_do_init(UINT32 h_isf, UINT32 path_max_count)
{
	ISF_RV rv = ISF_OK;
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;

	if (h_isf > 0) { //client process
		if (!g_vdodec_init[0]) { //is master process already init?
			rv = ISF_ERR_INIT; //not allow init from client process!
			goto _VD_INIT_ERR;
		}
		g_vdodec_init[h_isf] = 1; //set init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_vdodec_init[i]) { //client process still alive?
				rv = ISF_ERR_INIT; //not allow init from main process!
				goto _VD_INIT_ERR;
			}
		}
	}

	if (path_max_count > 0) {
		//update count
		PATH_MAX_COUNT = path_max_count;
		if (PATH_MAX_COUNT > VDODEC_MAX_PATH_NUM) {
			DBG_WRN("path max count %d > max %d!\r\n", PATH_MAX_COUNT, VDODEC_MAX_PATH_NUM);
			PATH_MAX_COUNT = VDODEC_MAX_PATH_NUM;
		}
	}

	if (p_ctx_c->ctx_mem.ctx_addr != 0) {
		DBG_ERR("[ISF_VDODEC] already init !!\r\n");
		rv = ISF_ERR_INIT;
		goto _VD_INIT_ERR;
	}

	{
		UINT32 buf_addr = 0;
		UINT32 buf_size = _isf_vdodec_query_context_memory();

		//alloc context of all port
		nvtmpp_vb_add_module(isf_vdodec.unit_module);
		buf_addr = (&isf_vdodec)->p_base->do_new_i(&isf_vdodec, NULL, "ctx", buf_size, &(p_ctx_c->ctx_mem.ctx_memblk));

		if (buf_addr == 0) {
			DBG_ERR("[ISF_VDODEC] alloc context memory fail !!\r\n");
			rv = ISF_ERR_HEAP;
			goto _VD_INIT_ERR;
		}

		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_vdodec, ISF_CTRL, "(alloc context memory) (%u, 0x%08x, %u)", PATH_MAX_COUNT, buf_addr, buf_size);

		p_ctx_c->ctx_mem.ctx_addr = buf_addr;

		// assign memory range
		_isf_vdodec_assign_context_memory_range(p_ctx_c->ctx_mem.ctx_addr);

		//install each device's kernel object
		isf_vdodec_install_id();
		nmp_vdodec_install_id();

		g_vdodec_init[h_isf] = 1; //set init
		return ISF_OK;
	}

_VD_INIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("init fail! %d\r\n", rv);
	}
	return rv;
}

static ISF_RV _isf_vdodec_do_uninit(UINT32 h_isf, UINT32 reset)
{
	ISF_RV rv = ISF_OK;
	VDODEC_CONTEXT_COMMON *p_ctx_c = (VDODEC_CONTEXT_COMMON *)&g_vd_ctx.comm;

	if (h_isf > 0) { //client process
		if (!g_vdodec_init[0]) { //is master process already init?
			rv = ISF_ERR_UNINIT; //not allow uninit from client process!
			goto _VD_UNINIT_ERR;
		}
		g_vdodec_init[h_isf] = 0; //clear init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_vdodec_init[i] && !reset) { //client process still alive?
				rv = ISF_ERR_UNINIT; //not allow uninit from main process!
				goto _VD_UNINIT_ERR;
			}
			if (reset) {
				g_vdodec_init[i] = 0; //force clear client
			}
		}
	}

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		return ISF_ERR_UNINIT;
	}

	if (PATH_MAX_COUNT == 0) {
		return ISF_ERR_UNINIT;
	}

	{
		UINT32 i;

		//reset all units of this module
		DBG_IND("unit(%s) => clear state\r\n", (&isf_vdodec)->unit_name);   // equal to call UpdatePort - STOP
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_vdodec)->p_base->do_clear_state(&isf_vdodec, oport);
		}

		DBG_IND("unit(%s) => clear bind\r\n", (&isf_vdodec)->unit_name);    // equal to call UpdatePort - CLOSE
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_vdodec)->p_base->do_clear_bind(&isf_vdodec, oport);
		}

		DBG_IND("unit(%s) => clear context\r\n", (&isf_vdodec)->unit_name); // equal to call UpdatePort - Reset
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_vdodec)->p_base->do_clear_ctx(&isf_vdodec, oport);
		}


		//uninstall each device's kernel object
		isf_vdodec_uninstall_id();
		nmp_vdodec_uninstall_id();

		//free context of all port
		if (p_ctx_c->ctx_mem.ctx_memblk.h_data != 0) {
			rv = (&isf_vdodec)->p_base->do_release_i(&isf_vdodec, &(p_ctx_c->ctx_mem.ctx_memblk), ISF_OK);
			if (rv == ISF_OK) {
				memset((void*)&p_ctx_c->ctx_mem, 0, sizeof(VDODEC_CTX_MEM));  // reset ctx_mem to 0
			} else {
				DBG_ERR("[ISF_VDODEC] free context memory fail !!\r\n");
				goto _VD_UNINIT_ERR;
			}
		} else {
			DBG_ERR("[ISF_VDODEC] context h_data == NULL !!\r\n");
			rv = ISF_ERR_INVALID_DATA_ID;
			goto _VD_UNINIT_ERR;
		}

		// reset NULL to all pointer
		_isf_vdodec_free_context_memory_range();

		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_vdodec, ISF_CTRL, "(release context memory)");
		g_vdodec_init[h_isf] = 0; //clear init
		return ISF_OK;
	}

_VD_UNINIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("uninit fail! %d\r\n", rv);
	}
	return rv;
}

ISF_RV _isf_vdodec_do_command(UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2)
{
	//DBG_IND("(do_command) cmd = %u, p0 = 0x%08x, p1 = %u, p2 = %u\r\n", cmd, p0, p1, p2);
	UINT32 h_isf = (cmd >> 28); //extract h_isf
	cmd &= ~0xf0000000; //clear h_isf

	switch(cmd) {
	case 1: //INIT
		nvtmpp_vb_add_module(MAKE_NVTMPP_MODULE('v', 'd', 'o', 'd', 'e', 'c', 0, 0));
		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_vdodec, ISF_CTRL, "(do_init) max_path = %u", p1);
		return _isf_vdodec_do_init(h_isf, p1); //p1: max path count
	case 0: //UNINIT
		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_vdodec, ISF_CTRL, "(do_uninit)");
		return _isf_vdodec_do_uninit(h_isf, p1); //p1: is under reset
	default:
		DBG_ERR("unsupport cmd(%u) !!\r\n", cmd);
		break;
	}
	return ISF_ERR_NOT_SUPPORT;
}

ISF_UNIT isf_vdodec;
static ISF_PORT_PATH ISF_VdoDec_PathList[ISF_VDODEC_OUT_NUM] = {
	{&isf_vdodec, ISF_IN(0),  ISF_OUT(0) },
	{&isf_vdodec, ISF_IN(1),  ISF_OUT(1) },
	{&isf_vdodec, ISF_IN(2),  ISF_OUT(2) },
	{&isf_vdodec, ISF_IN(3),  ISF_OUT(3) },
	{&isf_vdodec, ISF_IN(4),  ISF_OUT(4) },
	{&isf_vdodec, ISF_IN(5),  ISF_OUT(5) },
	{&isf_vdodec, ISF_IN(6),  ISF_OUT(6) },
	{&isf_vdodec, ISF_IN(7),  ISF_OUT(7) },
	{&isf_vdodec, ISF_IN(8),  ISF_OUT(8) },
	{&isf_vdodec, ISF_IN(9),  ISF_OUT(9) },
	{&isf_vdodec, ISF_IN(10), ISF_OUT(10)},
	{&isf_vdodec, ISF_IN(11), ISF_OUT(11)},
	{&isf_vdodec, ISF_IN(12), ISF_OUT(12)},
	{&isf_vdodec, ISF_IN(13), ISF_OUT(13)},
	{&isf_vdodec, ISF_IN(14), ISF_OUT(14)},
	{&isf_vdodec, ISF_IN(15), ISF_OUT(15)},
};

ISF_UNIT isf_vdodec = {
	.sign = ISF_SIGN_UNIT,
	.unit_name = "vdodec",
	.unit_module = MAKE_NVTMPP_MODULE('v', 'd', 'o', 'd', 'e', 'c', 0, 0),
	.list_id = 1, // 1=not support Multi-Process
	.port_in_count = ISF_VDODEC_IN_NUM,
	.port_out_count = ISF_VDODEC_OUT_NUM,
	.port_path_count = ISF_VDODEC_OUT_NUM,
	.port_in = ISF_VdoDec_InputPortList,
	.port_out = ISF_VdoDec_OutputPortList,
	.port_outcfg = ISF_VdoDec_OutputPortList_Cfg,
	.port_incaps = ISF_VdoDec_InputPortList_Caps,
	.port_outcaps = ISF_VdoDec_OutputPortList_Caps,
	.port_outstate = ISF_VdoDec_OutputPortList_State,
	.port_ininfo = isf_vdodec_outputinfolist_in,
	.port_outinfo = isf_vdodec_outputinfolist_out,
	.port_path = ISF_VdoDec_PathList,
	.do_command = _isf_vdodec_do_command,
	.do_bindinput = ISF_VdoDec_BindInput,
	.do_bindoutput = ISF_VdoDec_BindOutput,
#if 0
	.SetParam = ISF_VdoDec_SetParam,
	.GetParam = ISF_VdoDec_GetParam,
#endif
	.do_setportparam = ISF_VdoDec_SetPortParam,
	.do_getportparam = ISF_VdoDec_GetPortParam,
	.do_setportstruct = ISF_VdoDec_SetPortStruct,
	.do_getportstruct = ISF_VdoDec_GetPortStruct,
	.do_update = ISF_VdoDec_UpdatePort,
};


