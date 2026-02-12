
#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/platform.h"
#include "kwrap/stdio.h"
#include "kwrap/util.h"
#include <kwrap/cpu.h>
#include "sxcmd_wrapper.h"
#include "comm/hwclock.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_videoenc/isf_vdoenc.h"
#include "kflow_videoenc/isf_vdoenc_int.h"
#include "nmediarec_vdoenc.h"
#include "nmediarec_api.h"
#include "nmediarec_vdoenc_api.h"
#include "nmediarec_imgcap.h"
#include "video_encode.h"
#include "video_codec_mjpg.h"
#include "video_codec_h264.h"
#include "video_codec_h265.h"
#include "gximage/gximage.h"   // for YUV rotate , JPEG roate encode need
#include "isf_vdoenc_internal.h"
#include "../nmediarec_vdoenc/nmediarec_vdoenc_platform.h"

#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/vdoenc_builtin.h"

#if !defined(__LINUX)
#include "kwrap/cmdsys.h"
extern BOOL _isf_vdoenc_api_info(CHAR *strCmd);
extern BOOL _isf_vdoenc_api_top(CHAR *strCmd);
#define EXPORT_SYMBOL(x)
#endif


#if _TODO
// Perf_GetCurrent         : use do_gettimeofday() to implement
// HwClock_GetLongCounter  : ??
// HwClock_DiffLongCounter : ??
// RECORD_DATA_TS          : originally #define at ImageStream.h
// nvtmpp_vb_block2addr    : use nvtmpp_vb_blk2va() ??
// riff.h                  : can't find this header file, define my RIFF_CHUNK  first
// EVENT for ImgCap trigger thumb & MANUAL_SYNC : #if _TODO now,  because ISF_DATA->Event is removed !! Jeah is working todo.
// SrcCompression          : #if _TODO now, beacuse can't find ipl header
// get_tid                 : uitron get task id.... now implement fake funciton first
// DEBUG msg               : add user proc interface for _ISF_VdoEnc_ShowMsg() later
// NMediaImgCap            : #define none first
// DBG_FPS                 : at isf_stream_core.h , this macro involve float calculation, which is NOT allowed in Linux Kernel,  define NONE first
// ISF_UNIT_TRACE          : define NONE first ( isf_flow doesn't EXPORT_SYMBOL this , also, if it indeed EXPORT_SYMBOL, it says disagree with version )
#else
// perf
#define Perf_GetCurrent(x)  hwclock_get_counter(x)

//HwClock
#define HwClock_GetLongCounter(args...)     0
//#define HwClock_DiffLongCounter(args...)    0
UINT64 HwClock_DiffLongCounter(UINT64 time_start, UINT64 time_end)
{
	UINT32 time_start_sec = 0;
	UINT32 time_start_usec = 0;
	UINT32 time_end_sec=0;
	UINT32 time_end_usec =0;
	INT32  diff_time_sec =0 ;
	INT32  diff_time_usec = 0;
	UINT64 diff_time;

	time_start_sec = (time_start >> 32) & 0xFFFFFFFF;
	time_start_usec = time_start & 0xFFFFFFFF;
	time_end_sec = (time_end >> 32) & 0xFFFFFFFF;
	time_end_usec = time_end & 0xFFFFFFFF;
	diff_time_sec = (INT32)time_end_sec - (INT32)time_start_sec;
	diff_time_usec = (INT32)time_end_usec - (INT32)time_start_usec;
	diff_time = (INT64)diff_time_sec * 1000000 + diff_time_usec;
	return diff_time;
}
#define RECORD_DATA_TS                      0

#define nvtmpp_vb_block2addr(args...)       nvtmpp_vb_blk2va(args)

// my define RIFF_CHUNK
typedef struct _RIFF_CHUNK {
	UINT32              fourcc;                     ///< MAKEFOURCC('?','?','?','?')
	UINT32              size;                      ///< chunk size = 8 + body
} RIFF_CHUNK, *PRIFF_CHUNK; ///< size = 2 words = 8 bytes


// fake get_tid() function
static unsigned int get_tid ( signed int *p_tskid )
{
    *p_tskid = -99;
    return 0;
}

// NMediaRecImgCap functions
#define NMR_ImgCap_AddUnit(args...)

// DBG_FPS
#if defined DBG_FPS
#undef DBG_FPS
#define DBG_FPS(args...)
#endif

#define ISF_UNIT_TRACE(opclass, p_thisunit, port, fmtstr, args...) (p_thisunit)->p_base->do_trace(p_thisunit, port, opclass, fmtstr, ##args)

#endif

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoenc
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoenc_debug_level = NVT_DBG_WRN;
module_param_named(debug_level_isf_vdoenc, isf_vdoenc_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_isf_vdoenc, "isf_vdoenc debug level");
///////////////////////////////////////////////////////////////////////////////

#define WATCH_PORT				ISF_IN(0)   //ISF_IN1

#define FORCE_DUMP_DATA     	DISABLE
#define DUMP_DATA_CYCLE			DISABLE
#define WATCH_DATA				DISABLE
#define DUMP_RELEASE_TIME		DISABLE

#define DUMP_FPS				DISABLE
#define MANUAL_SYNC				ENABLE
#define MANUAL_SYNC_KOFF		ENABLE
#define MANUAL_SYNC_BASE		ENABLE
#define DUMP_KICK				DISABLE

#define SUPPORT_PULL_FUNCTION   ENABLE

#if (WATCH_DATA == ENABLE)
#define cpu_watchData(id, pData) if(id+ISF_IN_BASE==WATCH_PORT) {if(pData)cpu_enableWatch(0, (UINT32)&(((ISF_DATA*)(pData))->Sign), 4);else cpu_disableWatch(0);}
#else
#define cpu_watchData(id, pData)
#endif


#define SYNC_TAG MAKEFOURCC('S','Y','N','C')
#define ISF_VDOENC_DATA_QUE_NUM    			2              // max 2 YUV queue
#define ISF_VDOENC_DATA_QUE_NUM_DEFAULT		2
#define ISF_VDOENC_BSDATA_BLK_MAX  			90             //30 fps * 3 secs
#define VDOENC_SAMPLE_RATE_UNIT       		1000

#define ISF_VDOENC_PULL_Q_MAX               90             //30 fps * 3 secs

#define ISF_VDOENC_MSG_YUV  				((UINT32)0x00000001 << 0)   //0x00000001
#define ISF_VDOENC_MSG_LOCK  				((UINT32)0x00000001 << 1)   //0x00000002
#define ISF_VDOENC_MSG_DROP  				((UINT32)0x00000001 << 2)   //0x00000004
#define ISF_VDOENC_MSG_TNR  				((UINT32)0x00000001 << 3)   //0x00000008
#define ISF_VDOENC_MSG_YUV_TMOUT			((UINT32)0x00000001 << 4)   //0x00000010
#define ISF_VDOENC_MSG_YUV_INTERVAL			((UINT32)0x00000001 << 5)   //0x00000020
#define ISF_VDOENC_MSG_PULLQ				((UINT32)0x00000001 << 6)   //0x00000040
#define ISF_VDOENC_MSG_LOWLATENCY			((UINT32)0x00000001 << 7)   //0x00000080
#define ISF_VDOENC_MSG_ISP_RATIO			((UINT32)0x00000001 << 8)   //0x00000100
#define ISF_VDOENC_MSG_MDINFO				((UINT32)0x00000001 << 9)   //0x00000200
#define ISF_VDOENC_MSG_HALIGN				((UINT32)0x00000001 << 10)  //0x00000400
#define ISF_VDOENC_MSG_BS_INTERVAL			((UINT32)0x00000001 << 11)  //0x00000800

typedef struct {
	UINT32           refCnt;
	UINT32           hData;
	NMI_VDOENC_MEM_RANGE mem;
	UINT32           blk_id;
	void            *pnext_bsdata;
} VDOENC_BSDATA;

typedef struct {
	UINT32          rate;
	UINT32          frm_counter;
	UINT32          rate_counter;
	UINT32          output_counter;
	UINT32          FramePerSecond;
	UINT32          FramePerSecond_new;
} ISF_VDOENC_FRC;

typedef struct _VDOENC_PULL_QUEUE {
	UINT32          Front;                  ///< Front pointer
	UINT32          Rear;                   ///< Rear pointer
	UINT32          bFull;                  ///< Full flag
	ISF_DATA        *Queue;
} VDOENC_PULL_QUEUE, *PVDOENC_PULL_QUEUE;

typedef struct _VDOENC_MMAP_MEM_INFO {
	UINT32          addr_virtual;           ///< memory start addr (virtual)
	UINT32          addr_physical;          ///< memory start addr (physical)
	UINT32          size;                   ///< size
} VDOENC_MMAP_MEM_INFO, *PVDOENC_MMAP_MEM_INFO;

typedef struct _VDOENC_CTX_MEM {
	ISF_DATA        ctx_memblk;
	UINT32          ctx_addr;
	UINT32          isf_vdoenc_size;
	UINT32          nm_vdoenc_size;
	UINT32          nm_vdotrig_size;
	UINT32          kdrv_h264_size;
	UINT32          kdrv_h265_size;
	UINT32          kdrv_mjpg_size;
} VDOENC_CTX_MEM;

typedef struct _VDOENC_CONTEXT_COMMON {
	// CONTEXT
	VDOENC_CTX_MEM         ctx_mem;   // alloc context memory for isf_vdoenc + nmedia

	// TNR
	ISP_EVENT_FP           kflow_videoenc_get_isp_dev_fp;
	KDRV_VDOENC_3DNR       g_h26x_enc_tnr[ISP_ID_MAX_NUM];

	// SEMAPHORE
	//SEM_HANDLE             ISF_VDOENC_COMMON_SEM_ID;
	SEM_HANDLE             ISF_VDOENC_POLL_BS_SEM_ID;

	// BS DATA
	UINT32                 gISF_VdoEnc_Open_Count;
	ISF_DATA               gISF_VdoEnc_BsData_MemBlk;
	VDOENC_BSDATA         *g_p_vdoenc_bsdata_blk_pool;
	UINT32                *g_p_vdoenc_bsdata_blk_maptlb;

	// poll bs
	UINT32                        g_isf_vdoenc_poll_bs_event_mask;
	VDOENC_POLL_LIST_BS_INFO      g_poll_bs_info;

	// Img_Cap
	UINT32                 gISF_ImgCapMaxMemSize;
	NMI_IMGCAP_MEM_RANGE   gISF_ImgCapMaxMem;
	ISF_DATA               gISF_ImgCapMaxMemBlk;
	BOOL                   bIs_NMI_ImgCap_ON;
	BOOL                   bIs_NMI_ImgCap_Opened;
	ISF_DATA               gISF_ImgCapMemBlk;


	// Event callback
	IsfVdoEncEventCb      *gfn_vdoenc_cb;
	IsfVdoEncEventCb      *gfn_vdoenc_raw_cb;
	IsfVdoEncEventCb      *gfn_vdoenc_srcout_cb;
	VDOENC_PROC_CB         gfn_vdoenc_proc_cb;

	BOOL                   g_bisf_vdoenc_addunit;
	BOOL                   g_bisf_imgcap_addunit;

	// fastboot
	BOOL                   bIs_fastboot;
#ifdef __KERNEL__
	BOOL                   bIs_fastboot_enc[ISF_VDOENC_IN_NUM];
#endif
} VDOENC_CONTEXT_COMMON;

typedef struct _VDOENC_CONTEXT_PORT {
	//FRC
#if (MANUAL_SYNC_BASE == ENABLE)
	UINT64                 gISF_VdoEnc_ts_ipl_cycle;
	UINT64                 gISF_VdoEnc_ts_enc_cycle;
#endif
#if (MANUAL_SYNC_KOFF == ENABLE)
	UINT64                 gISF_VdoEnc_ts_ipl_end;
	UINT64                 gISF_VdoEnc_ts_enc_begin;
	UINT64                 gISF_VdoEnc_ts_enc_end;
	UINT64                 gISF_VdoEnc_ts_kick_off;
	UINT64                 gISF_VdoEnc_ts_sync;
#endif
#if (MANUAL_SYNC == ENABLE)
	UINT32                 gISF_VdoEnc_ecnt;
	UINT32                 gISF_EncKick;
	UINT32                 gISF_SkipKick;
	ISF_UNIT              *ISF_VdoSrcUnit;
	ISF_PORT              *ISF_VdoSrcPort;
#endif

	ISF_VDOENC_FRC         gISF_VdoEnc_frc;

	ISF_FRC                gISF_VdoEnc_infrc;
	UINT32                 gISF_VdoEnc_framepersecond;

	UINT32   			   gISF_VdoEncMaxMemSize;
	NMI_VDOENC_MEM_RANGE   gISF_VdoEncMaxMem;
	ISF_DATA               gISF_VdoEncMaxMemBlk;
	ISF_DATA               gISF_VdoEncMemBlk;
	VDO_BITSTREAM          gISF_VStrmBuf;
	UINT32                 gISF_VdoFunc;
#if (SUPPORT_PULL_FUNCTION)
	VDOENC_PULL_QUEUE      gISF_VoEnc_PullQueue;
	VDOENC_MMAP_MEM_INFO   gISF_VdoEncMem_MmapInfo;
//	ISF_DATA               gISF_VdoEnc_PullQueue_MemBlk;
#endif
	ISF_DATA               gISF_VdoEnc_QueData_MemBlk;

	UINT32                 gISF_VdoEnc_Started;
	BOOL                   gISF_VdoEnc_Opened;
	UINT32                 gISF_VdoEnc_ISP_Id;
	BOOL                   gISF_VdoEnc_Is_AllocMem;

	UINT32                 g_raw_frame_count;

	// SEMAPHORE
	//SEM_HANDLE             ISF_VDOENC_SEM_ID;
#if (SUPPORT_PULL_FUNCTION)
	SEM_HANDLE             ISF_VDOENC_PULLQ_SEM_ID;
	SEM_HANDLE             ISF_VDOENC_PARTIAL_SEM_ID;  // protect wrap  [partial ISF_DATA] & [normal full ISF_DATA]
#endif
	SEM_HANDLE             ISF_VDOENC_YUV_SEM_ID;

	// RAW_ISF_DATA
//	ISF_DATA               gISF_VdoEnc_RawData_MemBlk;
	ISF_DATA              *gISF_VdoEnc_RawDATA;
	BOOL                   gISF_VdoEnc_IsRawUsed[ISF_VDOENC_DATA_QUE_NUM];
	UINT32                 gISF_VdoEnc_Data_Que_Num;

	// BS DATA
	VDOENC_BSDATA         *g_p_vdoenc_bsdata_link_head;
	VDOENC_BSDATA         *g_p_vdoenc_bsdata_link_tail;
	ISF_DATA               gISF_VdoEncBsData;
//	ISF_DATA               gISF_VdoEnc_NMRBsQ_MemBlk;

	// Img_Cap
	NMI_IMGCAP_JOB         gISF_ImgCap_Thumb_Job;
	VDOENC_SOURCBS_INFO    gISF_VdoEncSrcOut;

	// Trigger Snapshot (PureLinux HDAL)
	SEM_HANDLE             ISF_VDOENC_SNAP_LINUX_SEM_ID_IDLE;
	SEM_HANDLE             ISF_VDOENC_SNAP_LINUX_SEM_ID_DONE;
	UINT32                 gISF_VdoEnc_SnapAddr;
	UINT32                 gISF_VdoEnc_SnapSize;
	UINT32                 gISF_VdoEnc_SnapQuality;
	UINT64                 gISF_VdoEnc_SnapTimestamp;

	// RECONSTRUCT FRAME
	BOOL                   gISF_VdoEncCommReconBuf_Enable;
	BOOL                   gISF_VdoEncUseCommReconBuf[3];
	UINT32                 gISF_VdoEncReconFrmNum;
	ISF_DATA               gISF_VdoEncReconFrmBlk[3];
	UINT32                 gISF_VdoEncRecFrmCfgSVCLayer;
	UINT32                 gISF_VdoEncRecFrmCfgLTRInterval;

	BOOL                   gISF_VdoEncCommSrcOut_Enable;
	BOOL                   gISF_VdoEncCommSrcOut_NoJPGEnc;
	UINT32                 gISF_VdoEncCommSrcOutAddr;
	UINT32                 gISF_VdoEncCommSrcOutSize;
	BOOL                   gISF_VdoEncCommSrcOut_FromUser;
	UINT32                 gISF_VdoEncSnapShotSize;
	ISF_DATA               gISF_VdoEncSnapShotBlk;

	// Debug
	UINT32 				   gISF_VdoEncDropCount;

	// H26x descriptor
	UINT32                 gisf_vdoenc_vps_addr;
	UINT32                 gisf_vdoenc_sps_addr;
	UINT32                 gisf_vdoenc_pps_addr;

	// BS ring buff mode info
	UINT32                 gisf_vdoenc_bs_ring;
	UINT32                 gisf_vdoenc_bs_addr2;

	// META ALLOC
	NMI_VDOENC_META_ALLOC_INFO  *g_p_vdoenc_meta_alloc_info;
	UINT32                 gISF_VdoEncMetaAllocAddr;
	UINT32                 gISF_VdoEncMetaAllocSize;
	ISF_DATA               gISF_VdoEncMetaAllocBlk;
} VDOENC_CONTEXT_PORT;


typedef struct _VDOENC_CONTEXT {
	VDOENC_CONTEXT_COMMON  comm;    // common variable for all port
	VDOENC_CONTEXT_PORT   *port;    // individual variable for each port
} VDOENC_CONTEXT;



UINT32 g_vdoenc_path_max_count = 0;  // dynamic setting for actual used path count, using PATH_MAX_COUNT to access this variable
UINT32 g_vdoenc_gdc_first = 0;
VDOENC_CONTEXT g_ve_ctx = {0};



NMI_UNIT                     *pNMI_VdoEnc                                                       = NULL; // keep
NMI_UNIT                     *pNMI_ImgCap                                                       = NULL; // keep
SEM_HANDLE                    ISF_VDOENC_PROC_SEM_ID                                            = {0};  // keep, it's extern-used by otherwhere

#ifdef __KERNEL__
//Fast boot
UINT32 g_vdoenc_1st_open[BUILTIN_VDOENC_PATH_ID_MAX] = {1, 1, 1, 1, 1, 1};
UINT32 g_vdoenc_fastboot_bsdata_blk_max[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
UINT32 g_vdoenc_fastboot_pullq_max[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
UINT32 g_vdoenc_get_builtinbs_done[BUILTIN_VDOENC_PATH_ID_MAX] = {0};
#endif

// BS
static ISF_DATA               gISF_VdoEncBsData[ISF_VDOENC_OUT_NUM]                             = {0};  // keep, this will be init before do_command()->init()
static ISF_DATA               gISF_VdoEncBsDataPartial[ISF_VDOENC_OUT_NUM]                      = {0};  // keep, this will be init before do_command()->init()

// multi-process
static UINT32                 g_vdoenc_init[ISF_FLOW_MAX]                                       = {0};

// debug
static UINT32         		  gISF_VdoEncMsgLevel[ISF_VDOENC_IN_NUM]						    = {0};  // keep, so we can set debug command before running program

void _ISF_VdoEnc_InstallCmd(void);
static ISF_RV _isf_vdoenc_bs_data_init(void);
#ifdef __KERNEL__
extern void NMR_VdoEnc_GetBuiltinBsData(UINT32 pathID);
#endif
static ISF_DATA_CLASS _isf_vdoenc_base;

#if (SUPPORT_PULL_FUNCTION)
#define VDOENC_VIRT2PHY(pathID, virt_addr) g_ve_ctx.port[pathID].gISF_VdoEncMem_MmapInfo.addr_physical + (virt_addr - g_ve_ctx.port[pathID].gISF_VdoEncMem_MmapInfo.addr_virtual)

BOOL _ISF_VdoEnc_isUserPull_AllReleased(UINT32 pathID);
BOOL _ISF_VdoEnc_isEmpty_PullQueue(UINT32 pathID);
BOOL _ISF_VdoEnc_isFull_PullQueue(UINT32 pathID);
void _ISF_VdoEnc_Unlock_PullQueue(UINT32 pathID);
BOOL _ISF_VdoEnc_Put_PullQueue(UINT32 pathID, ISF_DATA *data_in);
ISF_RV _ISF_VdoEnc_Get_PullQueue(UINT32 pathID, ISF_DATA *data_out, INT32 wait_ms);
BOOL _ISF_VdoEnc_RelaseAll_PullQueue(UINT32 pathID);
void _ISF_VdoEnc_3DNR_Internal(UINT32 pathID, UINT32 p3DNRConfig);
void _ISF_VdoEnc_ISPCB_Internal(UINT32 pathID, UINT32 pISPConfig);
static ISF_RV _ISF_VdoEnc_Close(ISF_UNIT *pThisUnit, UINT32 id, UINT32 b_do_release_i);
#endif

static ISF_RV _ISF_ImgCap_LockCB(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data)
{
	return ISF_OK;
}

static ISF_RV _ISF_ImgCap_UnLockCB(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data)
{
	return ISF_OK;
}

static ISF_DATA_CLASS _isf_imgcap_base = {
    .do_lock   = _ISF_ImgCap_LockCB,
    .do_unlock = _ISF_ImgCap_UnLockCB,
};

static VDOENC_BSDATA *_ISF_VdoEnc_GET_BSDATA(UINT32 pathID)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	UINT32         i = 0, uiBsDataBlkMax = ISF_VDOENC_BSDATA_BLK_MAX;
	VDOENC_BSDATA *pData = NULL;
	unsigned long flags=0;

#ifdef __KERNEL__
	if (pathID < BUILTIN_VDOENC_PATH_ID_MAX) {
		if (p_ctx_c->bIs_fastboot_enc[pathID]) {
			if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[pathID]) {
				uiBsDataBlkMax = g_vdoenc_fastboot_bsdata_blk_max[pathID];
			}
		}
	}
#endif

	NMR_VdoEnc_Lock_cpu2(&flags);
	while (p_ctx_c->g_p_vdoenc_bsdata_blk_maptlb[i]) {
		i++;
		if (i >= (PATH_MAX_COUNT * uiBsDataBlkMax)) {
			DBG_ERR("No free block!!!\r\n");
			NMR_VdoEnc_Unlock_cpu2(&flags);
			return NULL;
		}
	}

	pData = (VDOENC_BSDATA *)(p_ctx_c->g_p_vdoenc_bsdata_blk_pool + i);

	if (pData) {
		pData->pnext_bsdata = NULL;
		pData->blk_id = i;
		p_ctx_c->g_p_vdoenc_bsdata_blk_maptlb[i] = TRUE;
	}
	NMR_VdoEnc_Unlock_cpu2(&flags);
	return pData;
}

static void _ISF_VdeEnc_FREE_BSDATA(VDOENC_BSDATA *pData)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	unsigned long flags=0;

	if (pData) {
		NMR_VdoEnc_Lock_cpu2(&flags);
		p_ctx_c->g_p_vdoenc_bsdata_blk_maptlb[pData->blk_id] = FALSE;
		pData->hData = 0;
		NMR_VdoEnc_Unlock_cpu2(&flags);
	}
}

static UINT32 isf_vdoenc_util_y2uvheight(VDO_PXLFMT fmt, UINT32 y_h)
{
	switch (fmt) {
	case VDO_PXLFMT_YUV422:     return y_h;
	case VDO_PXLFMT_YUV420:     return y_h >> 1;
	case VDO_PXLFMT_Y8:         return 0;
	default:
		DBG_ERR("unsupport image format(0x%08x)\r\n", (unsigned int)fmt);
		break;
	}

	return 0;
}

static UINT32 _ISF_VdoEnc_SetStreamHeader(NMI_VDOENC_BS_INFO *pMem)
{
	PVDO_BITSTREAM pVS = NULL;
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[pMem->PathID];

	pVS = &p_ctx_p->gISF_VStrmBuf;

	if (pVS->CodecType == MEDIAVIDENC_H264) {
		UINT32 spsAddr = 0, spsSize = 0;
		UINT32 ppsAddr = 0, ppsSize = 0;
		UINT32 a = 0;
		UINT32 q = 0;
		UINT32 c = 0;
		UINT8 *b = NULL;
		UINT32 bv = 0;
		UINT32 v1 = 0, v2 = 0;

		v1 = pMem->Addr;
		v2 = pMem->Size;

		b = (UINT8 *)v1;
		a = 3;
		for (; c < v2 && !q; c++) {
			bv |= b[0];
			if (c >= a) {
				if (bv == 0x00000001) {
					spsAddr = v1 + c - 3;
					q = 1;
				}
			}

			bv <<= 8;
			b++;
		}
		q = 0;
		a = spsAddr - v1 + 4 + 3;
		for (; c < v2 && !q; c++) {
			bv |= b[0];
			if (c >= a) {
				if (bv == 0x00000001) {
					ppsAddr = v1 + c - 3;
					q = 1;
				}
			}
			bv <<= 8;
			b++;
		}

		spsSize = ppsAddr - spsAddr;
		ppsSize = v2 - spsSize;
		p_ctx_p->gisf_vdoenc_sps_addr = spsAddr;
		pVS->resv[1] = spsSize;
		p_ctx_p->gisf_vdoenc_pps_addr = ppsAddr;
		pVS->resv[3] = ppsSize;
	} else if (pVS->CodecType == MEDIAVIDENC_H265) {
		UINT32 nalu_addr[3] = {0};
		UINT32 nalu_size[3] = {0};

		UINT32 v1 = 0, v2 = 0;
		UINT32 c   = 0;
		UINT8 *b   = NULL;
		UINT32 bv  = 0;
		UINT32 scc = 0;

		v1 = pMem->Addr;
		v2 = pMem->Size;

		b = (UINT8 *)v1;

		for (c = 0; c < v2; c++) {
			bv |= b[0];

			if (bv == 0x00000001) {
				nalu_addr[scc] = (UINT32)b - 3;

				if (scc > 0) {
					nalu_size[scc - 1] = nalu_addr[scc] - nalu_addr[scc - 1];
				}

				scc++;

				if (scc > 2) {
					nalu_size[scc - 1] = (v1 + v2) - nalu_addr[scc - 1];
					break;
				}
			}

			bv <<= 8;
			b++;
		}

		p_ctx_p->gisf_vdoenc_sps_addr = nalu_addr[1]; // sps
		pVS->resv[1]                  = nalu_size[1];
		p_ctx_p->gisf_vdoenc_pps_addr = nalu_addr[2]; // pps
		pVS->resv[3]                  = nalu_size[2];
		p_ctx_p->gisf_vdoenc_vps_addr = nalu_addr[0]; // vps
		pVS->resv[5]                  = nalu_size[0];
	}

	return 0;
}

ISF_RV _ISF_VdoEnc_OnRelease(UINT32 id, ISF_DATA *pData, ISF_RV r);

#if defined __UITRON || defined __ECOS
#include "HwClock.h"
#endif

static UINT32 _ISF_VdoEnc_YUV_TMOUT_ResourceUnlock(UINT32 id)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[id];

	// consume semaphore first
	if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_VDOENC_YUV_SEM_ID, vos_util_msec_to_tick(0))) {
		if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_YUV_TMOUT) {
			DBG_DUMP("[ISF_VDOENC][%d] Input YUV_TMOUT, Unlock, sem 0->0->1\r\n", id);
		}
	}else{
		if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_YUV_TMOUT) {
			DBG_DUMP("[ISF_VDOENC][%d] Input YUV_TMOUT, Unlock, sem 1->0->1\r\n", id);
		}
	}

	// semaphore +1
	SEM_SIGNAL(p_ctx_p->ISF_VDOENC_YUV_SEM_ID);

	return 0;
}

static BOOL _ISF_VdoEnc_YUV_TMOUT_RetryCheck(UINT32 id, MP_VDOENC_YUV_SRC *p_YuvSrc, int iDataQueNum)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[id];
	UINT32 pass_time_ms = (Perf_GetCurrent() - p_YuvSrc->uiInputTime)/1000;

	if (p_YuvSrc->iInputTimeout < 0) {
		//blocking mode
		if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_YUV_TMOUT) {
			DBG_DUMP("[ISF_VDOENC][%d][%d] Input YUV_TMOUT, wait_ms(blocking), resource waiting....\r\n", id, iDataQueNum);
		}

		if (SEM_WAIT_INTERRUPTIBLE(p_ctx_p->ISF_VDOENC_YUV_SEM_ID)) return FALSE;

		if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_YUV_TMOUT) {
			DBG_DUMP("[ISF_VDOENC][%d][%d] Input YUV_TMOUT, wait_ms(blocking), resource OK !!\r\n", id, iDataQueNum);
		}

		if (p_ctx_p->gISF_VdoEnc_Started == TRUE)
			return TRUE;   // try to find YUV queue again

	} else if (pass_time_ms < (UINT32)p_YuvSrc->iInputTimeout) {
		// timeout mode
		if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_YUV_TMOUT) {
			DBG_DUMP("[ISF_VDOENC][%d][%d] Input YUV_TMOUT, wait_ms(%d), resource waiting....\r\n", id, iDataQueNum, p_YuvSrc->iInputTimeout);
		}

		if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_VDOENC_YUV_SEM_ID, vos_util_msec_to_tick(p_YuvSrc->iInputTimeout - pass_time_ms))) {
			if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_YUV_TMOUT) {
				DBG_DUMP("[ISF_VDOENC][%d][%d] Input YUV_TMOUT, wait_ms(%d), pass_time(%u), timeout !!\r\n", id, iDataQueNum, p_YuvSrc->iInputTimeout, (Perf_GetCurrent() - p_YuvSrc->uiInputTime)/1000);
			}
		} else {
			if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_YUV_TMOUT) {
				DBG_DUMP("[ISF_VDOENC][%d][%d] Input YUV_TMOUT, wait_ms(%d), resource OK !!\r\n", id, iDataQueNum, p_YuvSrc->iInputTimeout);
			}

			if (p_ctx_p->gISF_VdoEnc_Started == TRUE)
				return TRUE;   // try to find YUV queue again
		}
	}

	return FALSE;  // should NOT try again
}

#ifdef ISF_TS
static UINT32 _ISF_VdoEnc_EncBeginCB(NMI_VDOENC_BS_INFO *pMem)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[pMem->PathID];
	ISF_DATA             *p_data    = NULL;
	p_data = &p_ctx_p->gISF_VdoEnc_RawDATA[pMem->BufID];
	p_data->ts_venc_in[1] = hwclock_get_longcounter();
	return 0;
}

static UINT32 _ISF_VdoEnc_EncEndCB(NMI_VDOENC_BS_INFO *pMem)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[pMem->PathID];
	ISF_DATA             *p_data    = NULL;
	p_data = &p_ctx_p->gISF_VdoEnc_RawDATA[pMem->BufID];
	p_data->ts_venc_in[2] = hwclock_get_longcounter();
	return 0;
}
#endif

extern void VdoCodec_JPEG_Lock(void);
extern void VdoCodec_JPEG_UnLock(void);

static UINT32 _ISF_VdoEnc_StreamReadyCB(NMI_VDOENC_BS_INFO *pMem)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[pMem->PathID];
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	ISF_DATA             *pBsData     = &gISF_VdoEncBsData[pMem->PathID];
	//[isf_stream] ISF_PORT             *pDestPort   = _isf_unit_out(&isf_vdoenc, ISF_OUT(pMem->PathID)); //ImageUnit_Out(&ISF_VdoEnc, pMem->PathID);
	//ISF_PORT             *pDestPort   = isf_vdoenc.p_base->oport(&isf_vdoenc, ISF_OUT(pMem->PathID)); // [isf_flow] use this ...
	PVDO_BITSTREAM        pVStrmBuf   = &p_ctx_p->gISF_VStrmBuf;
	ISF_DATA             *pRawData    = NULL;
	VDOENC_BSDATA        *pVdoEncData = NULL;
	ISF_RV                rv          = ISF_OK;
	unsigned long         flags = 0;

	// release lock RAW data
	if (pMem->Occupied == 0) {

		UINT32 id = pMem->PathID;
		//[isf_stream] ISF_PORT* pSrcPort = _isf_unit_in(&isf_vdoenc, ISF_IN(id));  //ImageUnit_In(&ISF_VdoEnc, id+ISF_IN_BASE);
		ISF_PORT* pSrcPort = isf_vdoenc.p_base->iport(&isf_vdoenc, ISF_IN(id)); // [isf_flow] use this ...

		#if (MANUAL_SYNC_KOFF == ENABLE)
		p_ctx_p->gISF_VdoEnc_ts_enc_end = HwClock_GetLongCounter();
		#endif

		if(!(p_ctx_p->gISF_VdoFunc & VDOENC_FUNC_NOWAIT)) { // WAIT case

			pRawData = &p_ctx_p->gISF_VdoEnc_RawDATA[pMem->BufID];
			#ifdef ISF_TS
			pRawData->ts_venc_in[3] = hwclock_get_longcounter();
			#ifdef ISF_DUMP_TS
			DBG_DUMP("venc-0: %lld\r\n", pRawData->ts_venc_in[0]);
			DBG_DUMP("venc-begin: %lld\r\n", pRawData->ts_venc_in[1]);
			DBG_DUMP("venc-end: %lld\r\n", pRawData->ts_venc_in[2]);
			DBG_DUMP("*** venc-begin to venc-end: %lld\r\n", pRawData->ts_venc_in[2]-pRawData->ts_venc_in[1]);
			#endif
			#endif

			if (p_ctx_c->gfn_vdoenc_raw_cb) // callback raw data for snapshot of IPCAM
	            p_ctx_c->gfn_vdoenc_raw_cb(0, pMem->PathID, (UINT32) pRawData);

#if ISF_LATENCY_DEBUG
			pBsData->vd_timestamp = pRawData->vd_timestamp;
			pBsData->dramend_timestamp = pRawData->dramend_timestamp;
			pBsData->ipl_timestamp_start = pRawData->ipl_timestamp_start;
			pBsData->ipl_timestamp_end = pRawData->ipl_timestamp_end;
#endif
#if (RECORD_DATA_TS == ENABLE)
			pRawData->TimeStamp_s = HwClock_GetLongCounter();
#endif
#if (FORCE_DUMP_DATA == ENABLE)
			if((pMem->PathID+ISF_IN_BASE)==WATCH_PORT) {
				DBG_DUMP("^MVdoEnc.in[%d]! Finish -- Vdo: blk_id=%08x addr=%08x\r\n", pMem->PathID, pRawData->hData, nvtmpp_vb_block2addr(pRawData->hData));
			}
#endif

			if (gISF_VdoEncMsgLevel[pMem->PathID] & ISF_VDOENC_MSG_YUV) {
				DBG_DUMP("[ISF_VDOENC][%d][%d] callback Release blk_id = 0x%08x, addr = 0x%08x, time = %u us\r\n", pMem->PathID, pMem->BufID, pRawData->h_data, nvtmpp_vb_block2addr(pRawData->h_data), Perf_GetCurrent());
			}

			if (pMem->Addr != 0) {
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(id), ISF_IN_PROBE_REL, ISF_ENTER); // [IN] REL_ENTER   // bitstream is actually encoded, addr !=0
			}

			rv = _ISF_VdoEnc_OnRelease(pMem->PathID, pRawData, ISF_OK);

			if ((pMem->Addr != 0) && (rv == ISF_OK)) {
					(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(id), ISF_IN_PROBE_REL, ISF_OK); // [IN] REL_OK  // bitstream is actually encoded, addr !=0
			}

			// set RAW_ISF_DATA queue empty
			NMR_VdoEnc_Lock_cpu(&flags);
			p_ctx_p->gISF_VdoEnc_IsRawUsed[pMem->BufID] = FALSE;
			NMR_VdoEnc_Unlock_cpu(&flags);

			_ISF_VdoEnc_YUV_TMOUT_ResourceUnlock(pMem->PathID);
		}

		#if (MANUAL_SYNC == ENABLE)
		p_ctx_p->gISF_VdoEnc_ecnt++;

		if(pSrcPort->connecttype == (ISF_CONNECT_PUSH|ISF_CONNECT_SYNC)) {
			if(p_ctx_p->gISF_VdoEnc_ecnt==1) { //first time encode finish
				DELAY_M_SEC(5); //kick timing is aligned to 2ms after encode finish~
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_START_TIMER, 0);
				DBG_IND("- VdoEnc.in[%d]! Start trigger timer\r\n", id);
				#if (MANUAL_SYNC_BASE == ENABLE)
				{
					UINT32 cycle;
					pNMI_VdoEnc->GetParam(id, NMI_VDOENC_PARAM_TIMERRATE2_IMM, &cycle);
					p_ctx_p->gISF_VdoEnc_ts_enc_cycle = cycle;
				}
				#endif
			}
		}
		#endif
	}

	if (pMem->Addr == 0) {
		return ISF_OK;
	}

	(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH, ISF_ENTER); // [OUT] PUSH_ENTER

#if _TODO
	if (pDestPort) {
#else
	// TODO: let it never push NOW, so don't check DestPort
	if (1) {
#endif
        if (pMem->SrcOutYAddr != 0) {
            if (p_ctx_c->gfn_vdoenc_srcout_cb) {
                p_ctx_p->gISF_VdoEncSrcOut.SrcOutYAddr  = pMem->SrcOutYAddr;
                p_ctx_p->gISF_VdoEncSrcOut.SrcOutUVAddr = pMem->SrcOutUVAddr;
                p_ctx_p->gISF_VdoEncSrcOut.SrcOutYLoft  = pMem->SrcOutYLoft;
                p_ctx_p->gISF_VdoEncSrcOut.SrcOutUVLoft = pMem->SrcOutUVLoft;
                p_ctx_p->gISF_VdoEncSrcOut.SrcOutHeight = pMem->SrcOutHeight;
                p_ctx_p->gISF_VdoEncSrcOut.SrcOutWidth  = pMem->SrcOutWidth;

                p_ctx_c->gfn_vdoenc_srcout_cb(0, pMem->PathID, (UINT32) &p_ctx_p->gISF_VdoEncSrcOut);
            }

			// encode srcout -> jpg
			{
				MP_VDOENC_YUV_SRCOUT src_out = {0};
				UINT32 rotate = 0;

				// get rotate setting
				pNMI_VdoEnc->GetParam(pMem->PathID, NMI_VDOENC_PARAM_ROTATE, &rotate);

				// set src_out
				src_out.width       = (rotate == MP_VDOENC_ROTATE_DISABLE || rotate == MP_VDOENC_ROTATE_180)? pVStrmBuf->Width:pVStrmBuf->Height;
				src_out.height      = (rotate == MP_VDOENC_ROTATE_DISABLE || rotate == MP_VDOENC_ROTATE_180)? pVStrmBuf->Height:pVStrmBuf->Width;
				src_out.quality     = p_ctx_p->gISF_VdoEnc_SnapQuality;
				src_out.y_addr      = pMem->SrcOutYAddr;
				src_out.c_addr      = pMem->SrcOutUVAddr;
				src_out.path_id     = pMem->PathID;
				src_out.timestamp   = pMem->TimeStamp;

				if (p_ctx_p->gISF_VStrmBuf.CodecType == MEDIAVIDENC_H264) {
					src_out.y_line_offset = ALIGN_CEIL_32(src_out.width);
					src_out.c_line_offset = ALIGN_CEIL_32(src_out.width);
				} else if (p_ctx_p->gISF_VStrmBuf.CodecType == MEDIAVIDENC_H265) {
					src_out.y_line_offset = ALIGN_CEIL_64(src_out.width);
					src_out.c_line_offset = ALIGN_CEIL_64(src_out.width);
				} else {
					DBG_ERR("unsupport codec type(%d) for snapshot\r\n", p_ctx_p->gISF_VStrmBuf.CodecType);
				}

				src_out.snap_addr = p_ctx_p->gISF_VdoEnc_SnapAddr;
				src_out.snap_size = p_ctx_p->gISF_VdoEnc_SnapSize;

				ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, &isf_vdoenc, ISF_OUT(pMem->PathID), "before jpg encode, w/h = %u/%u, Y/UV addr = %08x/%08x, Y/UV loff = %u/%u, (%08x, %u)\r\n", src_out.width, src_out.height, src_out.y_addr, src_out.c_addr, src_out.y_line_offset, src_out.c_line_offset, src_out.snap_addr, src_out.snap_size);

				// put job, put srcout
				if (E_OK != pNMI_VdoEnc->SetParam(pMem->PathID, NMI_VDOENC_PARAM_H26X_SRCOUT_JOB, (UINT32) &src_out)) {
					DBG_ERR("[ISF_VDOENC][%d] put snapshot job fail !!\r\n", pMem->PathID);
					p_ctx_p->gISF_VdoEnc_SnapSize      = 0;
					p_ctx_p->gISF_VdoEnc_SnapTimestamp = 0;

					SEM_SIGNAL(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_IDLE);
					SEM_SIGNAL(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_DONE);
				}
			}
        }

		pVStrmBuf->sign      = MAKEFOURCC('V', 'S', 'T', 'M');  //pVStrmBuf->flag
		pVStrmBuf->DataAddr = pMem->Addr;
		pVStrmBuf->DataSize = pMem->Size;
		p_ctx_p->gisf_vdoenc_bs_addr2 = pMem->Addr2;
		pVStrmBuf->resv[2]  = pMem->Size2;
		pVStrmBuf->resv[6]   = pMem->FrameType;
		pVStrmBuf->resv[7]   = pMem->IsIDR2Cut;
		pVStrmBuf->resv[8]   = pMem->SVCSize;
		pVStrmBuf->resv[9]   = pMem->TemporalId;
		pVStrmBuf->resv[10]  = pMem->IsKey;
		pVStrmBuf->resv[15]  = pMem->uiBaseQP;
		pVStrmBuf->resv[16]  = pMem->uiMotionRatio;
		pVStrmBuf->resv[17]  = pMem->y_mse;
		pVStrmBuf->resv[18]  = pMem->u_mse;
		pVStrmBuf->resv[19]  = pMem->v_mse;
		pVStrmBuf->resv[20]  = VDOENC_VIRT2PHY(pMem->PathID, pMem->nalu_info_addr);
		pVStrmBuf->resv[21]  = pMem->evbr_state;
#if (SUPPORT_PULL_FUNCTION)
		// convert to physical addr
		pVStrmBuf->resv[11]  = VDOENC_VIRT2PHY(pMem->PathID, pMem->Addr);          // bs1  phy
		if (pMem->Addr2 == 0) {
			pVStrmBuf->resv[0]  = 0;
			pVStrmBuf->resv[4]  = 0;
		} else {
			pVStrmBuf->resv[0]  = VDOENC_VIRT2PHY(pMem->PathID, pMem->Addr2);     // bs2  phy
			pVStrmBuf->resv[4]  = pMem->Addr2;                                    // use to release bs2
		}
		pVStrmBuf->resv[12]  = VDOENC_VIRT2PHY(pMem->PathID, p_ctx_p->gisf_vdoenc_sps_addr);  // sps phy
		pVStrmBuf->resv[13]  = VDOENC_VIRT2PHY(pMem->PathID, p_ctx_p->gisf_vdoenc_pps_addr);  // pps phy
		pVStrmBuf->resv[14]  = VDOENC_VIRT2PHY(pMem->PathID, p_ctx_p->gisf_vdoenc_vps_addr);  // vps phy
#endif
		// get VDOENC_BSDATA
		pVdoEncData = _ISF_VdoEnc_GET_BSDATA(pMem->PathID);

		if (pVdoEncData) {
			pVdoEncData->hData    = pMem->Addr;
			pVdoEncData->mem.Addr = pMem->Addr;
			pVdoEncData->mem.Size = pMem->Size;

			pVdoEncData->refCnt   = 0;
#if _TODO  // TODO: let it never push NOW ( is_allow_push seems not working ?? )
            if (isf_vdoenc.p_base->get_is_allow_push(&isf_vdoenc, pDestPort)) {
				pVdoEncData->refCnt   += 1;
            }
#endif

#if (SUPPORT_PULL_FUNCTION)
			pVdoEncData->refCnt   += 1;  // pull need another count
#endif
		} else {
			isf_vdoenc.p_base->do_new(&isf_vdoenc, ISF_OUT(pMem->PathID), NVTMPP_VB_INVALID_POOL, 0, 0, pBsData, ISF_OUT_PROBE_NEW);
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [OUT] PUSH_WRN
			DBG_IND("Port %d _ISF_VdeEnc_NEW_BSDATA is NULL!\r\n", pMem->PathID);
			// Set BS data address to unlock NMedia for this fail case
			pNMI_VdoEnc->SetParam(pMem->PathID, NMI_VDOENC_PARAM_UNLOCK_BS_ADDR, pMem->Addr);
			return ISF_ERR_QUEUE_FULL;
		}

		// add VDOENC_BSDATA link list
		NMR_VdoEnc_Lock_cpu(&flags);
		if (p_ctx_p->g_p_vdoenc_bsdata_link_head == NULL) {
			p_ctx_p->g_p_vdoenc_bsdata_link_head = pVdoEncData;
			p_ctx_p->g_p_vdoenc_bsdata_link_tail = p_ctx_p->g_p_vdoenc_bsdata_link_head;
		} else {
			p_ctx_p->g_p_vdoenc_bsdata_link_tail->pnext_bsdata = pVdoEncData;
			p_ctx_p->g_p_vdoenc_bsdata_link_tail = p_ctx_p->g_p_vdoenc_bsdata_link_tail->pnext_bsdata;
		}
		NMR_VdoEnc_Unlock_cpu(&flags);

		if (gISF_VdoEncMsgLevel[pMem->PathID] & ISF_VDOENC_MSG_LOCK) {
			DBG_DUMP("[ISF_VDOENC][%d] Get BS Data hdata = 0x%x, blk_id = %d\r\n", pMem->PathID, pVdoEncData->hData, pVdoEncData->blk_id);
		}

		pVStrmBuf->timestamp = pMem->TimeStamp; //pBsData->TimeStamp  = pMem->TimeStamp;

		// Set BS ISF_DATA info
		memcpy(pBsData->desc, pVStrmBuf, sizeof(VDO_BITSTREAM));

		pBsData->h_data      = pMem->Addr;
		//new pData by user
		isf_vdoenc.p_base->do_new(&isf_vdoenc, ISF_OUT(pMem->PathID), NVTMPP_VB_INVALID_POOL, pBsData->h_data, 0, pBsData, ISF_OUT_PROBE_NEW);

#if ISF_LATENCY_DEBUG
		pBsData->enc_timestamp_start = pMem->enc_timestamp_start;
		pBsData->enc_timestamp_end = pMem->enc_timestamp_end;
		//DBG_DUMP("[ISF_VDOENC][%d] enc_timestamp_start = %lld, enc_timestamp_end = %lld\r\n", pMem->PathID, pBsData->enc_timestamp_start, pBsData->enc_timestamp_end);
#endif

#if (SUPPORT_PULL_FUNCTION)
		// add to pull queue
		if (TRUE ==_ISF_VdoEnc_Put_PullQueue(pMem->PathID, pBsData)) {
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH, ISF_OK); // [OUT] PUSH_OK
		} else {
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pMem->PathID), ISF_OUT_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [OUT] PUSH_WRN
		}
#endif

#if _TODO  // TODO: let it never push NOW ( is_allow_push seems not working ?? )
		if (isf_vdoenc.p_base->get_is_allow_push(&isf_vdoenc, pDestPort)) {
			isf_vdoenc.p_base->do_push(&isf_vdoenc, ISF_OUT(pMem->PathID), pBsData, 0);
		}
#endif
	} else {
		DBG_ERR("Out port %d pDestPort is NULL!\r\n", pMem->PathID);
		return ISF_ERR_INVALID_VALUE;
	}

	return ISF_OK;
}

#if (0)//(MANUAL_SYNC == ENABLE) // [isf_flow] doesnot support "_isf_stream_query_unitupstreamoutputport" , also this is two buffer case, 520 will use IPL timer to implement sync. Remain this code, maybe some of the code could be used later.
static ISF_RV esync_lock(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data) {return ISF_OK;}
static ISF_RV esync_unlock(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data) {return ISF_OK;}

static ISF_DATA_CLASS esync_base = {
    .do_lock   = esync_lock,
    .do_unlock = esync_unlock,
};
static ISF_DATA esync[ISF_VDOENC_OUT_NUM];
#endif

#if defined __UITRON || defined __ECOS
#include "Utility.h"
#include "DbgUtApi.h"
#endif

#if 1
static void _ISF_VdoEnc_OnKick(UINT32 id)
{
#if 0  // [isf_flow] doesnot support "_isf_stream_query_unitupstreamoutputport" , also this is two buffer case, 520 will use IPL timer to implement sync. Remain this code, maybe some of the code could be used later.
	//[isf_stream] ISF_PORT* pSrcPort = _isf_unit_in(&isf_vdoenc, ISF_IN(id));    //ImageUnit_In(&ISF_VdoEnc, id+ISF_IN_BASE);
    ISF_PORT* pSrcPort = isf_vdoenc.p_base->iport(&isf_vdoenc, ISF_IN(id)); // [isf_flow] use this ...
	ISF_PORT* pDestPort = 0;
	if(pSrcPort->connecttype == (ISF_CONNECT_PUSH|ISF_CONNECT_SYNC)) {
		UINT32 oPort;
		if(p_ctx_p->ISF_VdoSrcPort == NULL) {
			if(p_ctx_p->ISF_VdoSrcUnit == NULL) {
				DBG_ERR("VdoEnc.in[%d] start Sync failed! (-1)\r\n", id);
				return;
			}
			if(p_ctx_p->ISF_VdoSrcPort == NULL) {
				_isf_stream_query_unitupstreamoutputport(p_ctx_p->ISF_VdoSrcUnit, id+ISF_IN_BASE, &isf_vdoenc, &oPort);   // oPort = ImageUnit_QueryUpstreamOutputPort(p_ctx_p->ISF_VdoSrcUnit, id+ISF_IN_BASE, &ISF_VdoEnc);
				if(oPort == ISF_CTRL) {
					DBG_ERR("VdoEnc.in[%d] start Sync failed! (-2)\r\n", id);
					return;
				}
#if (FORCE_DUMP_DATA == ENABLE)
				if(id+ISF_IN_BASE==WATCH_PORT) {
					DBG_DUMP("^MVdoEnc.in[%d]! start Sync to \'%s\'.out[%d] !\r\n", id, p_ctx_p->ISF_VdoSrcUnit->Name, oPort - ISF_OUT_BASE);
				}
#endif
				//[isf_stream] pDestPort =  _isf_unit_out(p_ctx_p->ISF_VdoSrcUnit, ISF_OUT(oPort)); //ImageUnit_Out(p_ctx_p->ISF_VdoSrcUnit, oPort);
				pDestPort =  p_ctx_p->ISF_VdoSrcUnit->p_base->oport(p_ctx_p->ISF_VdoSrcUnit, ISF_OUT(oPort)); // [isf_flow] use this ...
				p_ctx_p->ISF_VdoSrcPort = pDestPort;
#if (FORCE_DUMP_DATA == ENABLE)
				if(id+ISF_IN_BASE==WATCH_PORT) {
					DBG_DUMP("==> \'%s\'.out[%d] !\r\n", p_ctx_p->ISF_VdoSrcPort->pSrcUnit->Name, p_ctx_p->ISF_VdoSrcPort->oPort - ISF_OUT_BASE);
				}
#endif
			} else {
#if (FORCE_DUMP_DATA == ENABLE)
				if(id+ISF_IN_BASE==WATCH_PORT) {
					DBG_DUMP("^MVdoEnc.in[%d]! Sync!\r\n", id);
				}
#endif
			}
			cpu_watchData(id, 0);
			#if (MANUAL_SYNC == ENABLE)
			//esync[id].sign = ISF_SIGN_DATA;
			//esync[id].desc[0] = SYNC_TAG;
			//esync[id].p_class = &esync_base;
			//[isf_stream] _isf_unit_init_data(&esync[id], &esync_base, MAKEFOURCC('V','F','R','M'), 0x00010000);
			isf_vdoenc.p_base->init_data(&esync[id], &esync_base, MAKEFOURCC('V','F','R','M'), 0x00010000); // [isf_flow] use this ...

			DBG_IND("- VdoEnc.in[%d]! Start Manual Sync!\r\n", id);
			//if(oPort==WATCH_PORT) {DBGH(esync_list[id]);}
			if(id+ISF_IN_BASE == WATCH_PORT){
				DBG_FPS((&isf_vdoenc), "out", id, "Notify(s)");
			}
			(&isf_vdoenc)->p_base->do_notify(&isf_vdoenc, pSrcPort, &(esync[id]), 0, ISF_OK, p_ctx_p->ISF_VdoSrcPort); //start sync
			#endif
		} else {
#if (FORCE_DUMP_DATA == ENABLE)
			if(id+ISF_IN_BASE==WATCH_PORT) {
				DBG_DUMP("^MVdoEnc.in[%d]! Sync again!\r\n", id);
			}
#endif
			cpu_watchData(id, 0);
#if (DUMP_FPS == ENABLE)
			if(id+ISF_IN_BASE== WATCH_PORT){
				DBG_FPS((&isf_vdoenc), "out", id, "Notify(a)");
			}
#endif
			(&isf_vdoenc)->p_base->do_notify(&isf_vdoenc, pSrcPort, NULL, 0, ISF_OK, p_ctx_p->ISF_VdoSrcPort); //start sync
		}
	}
#endif // #if 0
}
#endif

ISF_RV _ISF_VdoEnc_OnRelease(UINT32 id, ISF_DATA *pData, ISF_RV r)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[id];
	ISF_RV rv = ISF_OK;
#if 1
	//[isf_stream] ISF_PORT* pSrcPort = _isf_unit_in(&isf_vdoenc, ISF_IN(id)); //ImageUnit_In(&ISF_VdoEnc, id+ISF_IN_BASE);
	ISF_PORT* pSrcPort = isf_vdoenc.p_base->iport(&isf_vdoenc, ISF_IN(id)); // [isf_flow] use this ...
	if(pSrcPort->connecttype == (ISF_CONNECT_PUSH|ISF_CONNECT_SYNC)) {
/*
#if (FORCE_DUMP_DATA == ENABLE)
		if(id+ISF_IN_BASE==WATCH_PORT) {
			DBG_DUMP("^MVdoEnc.in[%d] --------- end\r\n", id);
		}
#endif
*/
		if(pData && (p_ctx_p->ISF_VdoSrcPort!=0)) {
/*
#if (FORCE_DUMP_DATA == ENABLE)
			if(id+ISF_IN_BASE==WATCH_PORT) {
				//DBG_DUMP("^MVdoEnc.in[%d] Sync! blk_id=%08x to \'%s\'.out[%d] \r\n", id, pData->hData, p_ctx_p->ISF_VdoSrcPort->pSrcUnit->Name, p_ctx_p->ISF_VdoSrcPort->oPort - ISF_OUT_BASE);
			}
#endif
*/
			cpu_watchData(id, 0);
#if (DUMP_FPS == ENABLE)
			if(id+ISF_IN_BASE== WATCH_PORT){
				DBG_FPS((&isf_vdoenc), "out", id, "Notify(r)");
			}
#endif
			rv = (&isf_vdoenc)->p_base->do_notify(&isf_vdoenc, pSrcPort, pData, 0, r, p_ctx_p->ISF_VdoSrcPort); //continue sync

		} else if(pData) {

#if (DUMP_FPS == ENABLE)
			if(id+ISF_IN_BASE== WATCH_PORT){
				DBG_FPS((&isf_vdoenc), "out", id, "Notify(r2)");
			}
#endif
			rv = (&isf_vdoenc)->p_base->do_notify(&isf_vdoenc, pSrcPort, pData, 0, r, (ISF_PORT*)(pData->src)); //end sync

		} else { // p_ctx_p->ISF_VdoSrcPort==0
/*
#if (FORCE_DUMP_DATA == ENABLE)
			if(id+ISF_IN_BASE==WATCH_PORT) {
				DBG_DUMP("^MVdoEnc.in[%d] Sync!\r\n", id);
			}
#endif
*/
			_ISF_VdoEnc_OnKick(id); //first sync
		}
	} else {  //pSrcPort->ConnectType == ISF_CONNECT_PUSH
		if(pData) {
#if (DUMP_DATA_CYCLE == ENABLE)
			if(id+ISF_IN_BASE== WATCH_PORT){
				//total time
				pData->TimeStamp_n = HwClock_DiffLongCounter(pData->TimeStamp_g, pData->TimeStamp_s);
				//delta time
				pData->TimeStamp_s = HwClock_DiffLongCounter(pData->TimeStamp_e, pData->TimeStamp_s);
				pData->TimeStamp_e = HwClock_DiffLongCounter(pData->TimeStamp_p, pData->TimeStamp_e);
				pData->TimeStamp_p = HwClock_DiffLongCounter(pData->TimeStamp, pData->TimeStamp_p);
				pData->TimeStamp = HwClock_DiffLongCounter(pData->TimeStamp_g, pData->TimeStamp);
				DBG_DUMP("^CCycle: %2d.%06d %2d.%06d %2d.%06d %2d.%06d %2d.%06d %2d.%06d\r\n",
				(UINT32)(pData->TimeStamp_g >> 32), (UINT32)(pData->TimeStamp_g),
				(UINT32)(pData->TimeStamp >> 32), (UINT32)(pData->TimeStamp),
				(UINT32)(pData->TimeStamp_p >> 32), (UINT32)(pData->TimeStamp_p),
				(UINT32)(pData->TimeStamp_e >> 32), (UINT32)(pData->TimeStamp_e),
				(UINT32)(pData->TimeStamp_s >> 32), (UINT32)(pData->TimeStamp_s),
				(UINT32)(pData->TimeStamp_n >> 32), (UINT32)(pData->TimeStamp_n));
			}
#endif
/*
#if (FORCE_DUMP_DATA == ENABLE)
			if(id+ISF_IN_BASE==WATCH_PORT) {
				DBG_DUMP("^MVdoEnc.in[%d] Release! blk_id=%08x\r\n", id, pData->hData);
				DBG_DUMP("^MVdoEnc.in[%d]! drop -- Vdo: blk_id=%08x addr=%08x\r\n", id, pData->hData, nvtmpp_vb_block2addr(pData->hData));
			}
#endif
*/
			rv = (&isf_vdoenc)->p_base->do_release(&isf_vdoenc, ISF_IN(id), pData, 0);
		}
	}
#endif
	return rv;
}


static void _ISF_VDOENC_InitFrc(UINT32 oPort, ISF_VDOENC_FRC *frc, UINT32 FramePerSecond)
{
	UINT32               src_fps = 0;
	UINT32               dst_fps = 0;

	frc->frm_counter     = 0;
	frc->output_counter  = 0;
	frc->rate_counter    = 0;
	frc->FramePerSecond  = FramePerSecond;
	if (GET_LO_UINT16(FramePerSecond) == 0)
		frc->rate = VDOENC_SAMPLE_RATE_UNIT;
	else {
		src_fps              = GET_LO_UINT16 (FramePerSecond);
		dst_fps              = GET_HI_UINT16(FramePerSecond);
		frc->rate            = (dst_fps*VDOENC_SAMPLE_RATE_UNIT/src_fps)+1;
	}
	DBG_IND("[fps]framerate = 0x%x, src_fps = %d, dst_fps = %d, rate = %d\r\n", FramePerSecond,src_fps,dst_fps,frc->rate);
}

static BOOL _ISF_VDOENC_frame_rate_ctrl(UINT32 oPort, ISF_VDOENC_FRC *frc)
{
	BOOL ret = FALSE, reset = FALSE;

	if (frc->FramePerSecond == 0){
		frc->output_counter++;
		DBG_IND("[fps]%d T1\r\n",oPort-ISF_OUT_BASE);
		ret = TRUE;
	}
	else if (frc->rate >= VDOENC_SAMPLE_RATE_UNIT){
		frc->output_counter++;
		DBG_IND("[fps]%d T2\r\n",oPort-ISF_OUT_BASE);
		ret = TRUE;
	}
	else if (frc->frm_counter == 0) {
		frc->output_counter++;
		DBG_IND("[fps]%d T3\r\n",oPort-ISF_OUT_BASE);
		ret = TRUE;
	}
	else {
		frc->rate_counter+= frc->rate;
		if (frc->rate_counter > VDOENC_SAMPLE_RATE_UNIT) {
			frc->rate_counter -= VDOENC_SAMPLE_RATE_UNIT;
			frc->output_counter++;
			DBG_IND("[fps]%d T4\r\n",oPort-ISF_OUT_BASE);
			ret = TRUE;
		}
	}
	frc->frm_counter++;
	DBG_IND("[fps]%d frm_cnt %d , out_cnt %d\r\n",oPort-ISF_OUT_BASE,frc->frm_counter,frc->output_counter);
	// reset counter
	if (frc->FramePerSecond == 0 && frc->frm_counter >= 30){
		reset = TRUE;
	}
	else if ((frc->FramePerSecond != 0) && (frc->frm_counter >= GET_LO_UINT16 (frc->FramePerSecond))) {
		reset = TRUE;
	}
	if (reset && frc->FramePerSecond_new != frc->FramePerSecond){
		DBG_IND("[fps]%d reset, FramePerSecond_new = 0x%x\r\n",oPort-ISF_OUT_BASE,frc->FramePerSecond_new);
		_ISF_VDOENC_InitFrc(oPort, frc,frc->FramePerSecond_new);
	}
	return ret;
}

#if (MANUAL_SYNC_BASE == ENABLE)
/*
static UINT64 HwClock_AddLongCounter(UINT64 time_a, UINT64 time_b)
{
	UINT32 time_a_sec = 0;
	UINT32 time_a_usec = 0;
	UINT32 time_b_sec=0;
	UINT32 time_b_usec =0;
	INT32  sum_time_sec =0 ;
	INT32  sum_time_usec = 0;
	UINT64 sum_time;

	time_a_sec = (time_a >> 32) & 0xFFFFFFFF;
	time_a_usec = time_a & 0xFFFFFFFF;
	time_b_sec = (time_b >> 32) & 0xFFFFFFFF;
	time_b_usec = time_b & 0xFFFFFFFF;
	//DBG_DUMP("- a=%d.%d b=%d.%d\r\n",
	//	time_a_sec, time_a_usec,
	//	time_b_sec, time_b_usec);
	sum_time_usec = (INT32)time_a_usec + (INT32)time_b_usec;
	sum_time_sec = (INT32)time_a_sec + (INT32)time_b_sec;
	//DBG_DUMP("- c=%d.%d\r\n",
	//	sum_time_sec, sum_time_usec);
	if(sum_time_usec > 1000000) {
		sum_time_sec++;
		sum_time_usec -= 1000000;
	} else if(sum_time_usec < 0) {
		sum_time_sec--;
		sum_time_usec += 1000000;
	}
	//DBG_DUMP("- c=%d.%d\r\n",
	//	sum_time_sec, sum_time_usec);
	sum_time = ((INT64)sum_time_sec << 32) + ((INT64)sum_time_usec);
	//DBG_DUMP("- C=%d.%d\r\n",
	//	(UINT32)(sum_time>>32), (UINT32)sum_time);
	return sum_time;
}
*/

static UINT64 HwClock_SubLongCounter(UINT64 time_a, UINT64 time_b)
{
	UINT32 time_a_sec = 0;
	UINT32 time_a_usec = 0;
	UINT32 time_b_sec=0;
	UINT32 time_b_usec =0;
	INT32  sum_time_sec =0 ;
	INT32  sum_time_usec = 0;
	UINT64 sum_time;

	time_a_sec = (time_a >> 32) & 0xFFFFFFFF;
	time_a_usec = time_a & 0xFFFFFFFF;
	time_b_sec = (time_b >> 32) & 0xFFFFFFFF;
	time_b_usec = time_b & 0xFFFFFFFF;
	//DBG_DUMP("- a=%d.%d b=%d.%d\r\n",
	//	time_a_sec, time_a_usec,
	//	time_b_sec, time_b_usec);
	sum_time_usec = (INT32)time_a_usec - (INT32)time_b_usec;
	sum_time_sec = (INT32)time_a_sec - (INT32)time_b_sec;
	//DBG_DUMP("- c=%d.%d\r\n",
	//	sum_time_sec, sum_time_usec);
	if(sum_time_usec > 1000000) {
		sum_time_sec++;
		sum_time_usec -= 1000000;
	} else if(sum_time_usec < 0) {
		sum_time_sec--;
		sum_time_usec += 1000000;
	}
	//DBG_DUMP("- c=%d.%d\r\n",
	//	sum_time_sec, sum_time_usec);
	sum_time = ((INT64)sum_time_sec << 32) + ((INT64)sum_time_usec);
	//DBG_DUMP("- C=%d.%d\r\n",
	//	(UINT32)(sum_time>>32), (UINT32)sum_time);
	return sum_time;
}
#endif

static UINT32 _ISF_VdoEnc_OnSync(UINT32 id)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[id];
	UINT32 ret = 0;

#if (DUMP_FPS == ENABLE)
	if(id+ISF_IN_BASE== WATCH_PORT){
		DBG_FPS((&isf_vdoenc), "out", id, "Sync");
	}
#endif
	#if (MANUAL_SYNC_BASE == ENABLE)
	if(p_ctx_p->gISF_VdoEnc_ecnt > 2)

	if((p_ctx_p->gISF_VdoEnc_ts_ipl_cycle != 0) && (p_ctx_p->gISF_VdoEnc_ts_enc_cycle != 0)) {

		if(p_ctx_p->gISF_VdoEnc_ts_ipl_cycle == p_ctx_p->gISF_VdoEnc_ts_enc_cycle) {


		} else if(p_ctx_p->gISF_VdoEnc_ts_ipl_cycle < p_ctx_p->gISF_VdoEnc_ts_enc_cycle) {

			//cycle decrease

			if(p_ctx_p->gISF_VdoEnc_ts_ipl_cycle < p_ctx_p->gISF_VdoEnc_ts_kick_off) {
				//change period!
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_TIMERRATE2_IMM, p_ctx_p->gISF_VdoEnc_ts_ipl_cycle); //new cycle
				#if (DUMP_KICK == ENABLE)
				if((id+ISF_IN_BASE)==WATCH_PORT) {
					DBG_DUMP("^GVdoEnc.in[%d]! ........................................ kick_off=%3d.%06d\r\n",
						id,
						(UINT32)(p_ctx_p->gISF_VdoEnc_ts_kick_off>>32), (UINT32)(p_ctx_p->gISF_VdoEnc_ts_kick_off));
					DBG_DUMP("VdoEnc.in[%d]! ... (A) new=%lu, fps+, load=%lu\r\n",
						id, (UINT32)(p_ctx_p->gISF_VdoEnc_ts_ipl_cycle), (UINT32)(p_ctx_p->gISF_VdoEnc_ts_ipl_cycle));
				}
				#endif
			} else {
				UINT64 cycle_diff = HwClock_SubLongCounter(p_ctx_p->gISF_VdoEnc_ts_enc_cycle, p_ctx_p->gISF_VdoEnc_ts_ipl_cycle);
				if(cycle_diff > 100) {
					//do calibration
					ret = (UINT32)(cycle_diff);
					ret |= 0xf0000000; //add "ADJUST SIGN"
					//then change period!
					pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_TIMERRATE2_IMM, p_ctx_p->gISF_VdoEnc_ts_ipl_cycle); //new cycle
					#if (DUMP_KICK == ENABLE)
					if((id+ISF_IN_BASE)==WATCH_PORT) {
						DBG_DUMP("^GVdoEnc.in[%d]! ........................................ kick_off=%3d.%06d\r\n",
							id,
							(UINT32)(p_ctx_p->gISF_VdoEnc_ts_kick_off>>32), (UINT32)(p_ctx_p->gISF_VdoEnc_ts_kick_off));
						DBG_DUMP("VdoEnc.in[%d]! ... (B) new=%lu, fps+, diff=%lu, load=%lu\r\n",
							id, (UINT32)(p_ctx_p->gISF_VdoEnc_ts_ipl_cycle), (UINT32)(cycle_diff), (UINT32)(p_ctx_p->gISF_VdoEnc_ts_ipl_cycle));
					}
					#endif
				}
			}
			p_ctx_p->gISF_VdoEnc_ts_enc_cycle = p_ctx_p->gISF_VdoEnc_ts_ipl_cycle;
		} else {

			//cycle increase
			/*
			UINT64 enc = HwClock_GetLongCounter();
			UINT64 ipl = p_ctx_p->gISF_VdoEnc_ts_ipl_end;
			UINT64 c_err;
			ipl = HwClock_SubLongCounter(ipl, p_ctx_p->gISF_VdoEnc_ts_kick_off);
					//DBG_DUMP("- [%d] ipl=%3d.%06d\r\n",
					//	id,
					//	(UINT32)(ipl>>32), (UINT32)(ipl));
					//DBG_DUMP("- [%d] enc=%3d.%06d\r\n",
					//	id,
					//	(UINT32)(enc>>32), (UINT32)(enc));
			cycle_diff = HwClock_SubLongCounter(p_ctx_p->gISF_VdoEnc_ts_ipl_cycle, p_ctx_p->gISF_VdoEnc_ts_enc_cycle);
			if(cycle_diff > 500) {
				while (ipl < enc) {
					ipl = HwClock_AddLongCounter(ipl, p_ctx_p->gISF_VdoEnc_ts_ipl_cycle);
					//DBG_DUMP("- [%d] ipl=%3d.%06d\r\n",
					//	id,
					//	(UINT32)(ipl>>32), (UINT32)(ipl));
				}
				c_err = HwClock_SubLongCounter(ipl, enc);
				DBG_DUMP("VdoEnc.in[%d]! ... (C) new=%lu, fps-, diff=%lu\r\n",
					id, (UINT32)(p_ctx_p->gISF_VdoEnc_ts_ipl_cycle), (UINT32)(c_err));
				ret = c_err; //calibration
				ret |= 0xf0000000; //add "ADJUST SIGN"
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_TIMERRATE2_IMM, p_ctx_p->gISF_VdoEnc_ts_ipl_cycle); //new cycle
			}
			*/
			UINT32 ipl = (UINT32)p_ctx_p->gISF_VdoEnc_ts_ipl_cycle;
			UINT32 enc = (UINT32)p_ctx_p->gISF_VdoEnc_ts_enc_cycle;
			UINT32 cycle_diff = ipl-enc;
			UINT32 cycle_err;
			//DBG_DUMP("new-old=%lu-%lu \r\n",
			//	ipl, enc);
			//DBG_DUMP("diff=%lu\r\n", cycle_diff);
			if(cycle_diff > 100) {
				cycle_err = ipl - ((UINT32)p_ctx_p->gISF_VdoEnc_ts_kick_off);
				ret = cycle_err;
				ret |= 0xf0000000; //add "ADJUST SIGN"
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_TIMERRATE2_IMM, p_ctx_p->gISF_VdoEnc_ts_ipl_cycle); //new cycle
				#if (DUMP_KICK == ENABLE)
				if((id+ISF_IN_BASE)==WATCH_PORT) {
					DBG_DUMP("^GVdoEnc.in[%d]! ........................................ kick_off=%3d.%06d\r\n",
						id,
						(UINT32)(p_ctx_p->gISF_VdoEnc_ts_kick_off>>32), (UINT32)(p_ctx_p->gISF_VdoEnc_ts_kick_off));
					DBG_DUMP("VdoEnc.in[%d]! ... (C) new=%lu, fps-, diff=%lu\r\n",
						id, (UINT32)(p_ctx_p->gISF_VdoEnc_ts_ipl_cycle), (UINT32)(cycle_err));
				}
				#endif
			}

			p_ctx_p->gISF_VdoEnc_ts_enc_cycle = p_ctx_p->gISF_VdoEnc_ts_ipl_cycle;
		}
	}
	#endif

	if(p_ctx_p->gISF_VdoEnc_ecnt > 2) {

		//do FRC drop
		if (_ISF_VDOENC_frame_rate_ctrl(id, &(p_ctx_p->gISF_VdoEnc_frc)) == FALSE) {
			DBG_IND("VdoEnc.out[%d] FRC drop!\r\n", id);
			return 0;
		}

		//do time-out drop
		if(p_ctx_p->gISF_SkipKick) { //skip one time
			p_ctx_p->gISF_SkipKick	= 0;
			DBG_IND("VdoEnc.out[%d] TIME-OUT drop!\r\n", id);
			return 0;
		}
	}

	#if (MANUAL_SYNC_KOFF == ENABLE)
	p_ctx_p->gISF_VdoEnc_ts_sync = HwClock_GetLongCounter();
	#endif

	if(p_ctx_p->gISF_EncKick != 0) // if do kick
	{
		_ISF_VdoEnc_OnKick(id);
	}

	#if (MANUAL_SYNC_KOFF == ENABLE)
	if(ret == 0)
	{
		if(p_ctx_p->gISF_VdoEnc_ts_ipl_end > p_ctx_p->gISF_VdoEnc_ts_enc_begin) {

			// pt time-out
			p_ctx_p->gISF_SkipKick	= 1;
			//DBG_WRN("VdoEnc.in[%d]! ... process time-out\r\n", id);

		} else
		if(p_ctx_p->gISF_VdoEnc_ts_enc_begin > p_ctx_p->gISF_VdoEnc_ts_enc_end) {

			// et time-out
			p_ctx_p->gISF_SkipKick	= 1;
			//DBG_WRN("VdoEnc.in[%d]! ... encode time-out\r\n", id);

		} else {
			#if (DUMP_KICK == ENABLE)
			UINT64 pt = HwClock_DiffLongCounter(p_ctx_p->gISF_VdoEnc_ts_ipl_end, p_ctx_p->gISF_VdoEnc_ts_enc_begin);
			UINT64 et = HwClock_DiffLongCounter(p_ctx_p->gISF_VdoEnc_ts_enc_begin, p_ctx_p->gISF_VdoEnc_ts_enc_end);
			#endif
			UINT64 koff = HwClock_DiffLongCounter(p_ctx_p->gISF_VdoEnc_ts_ipl_end, p_ctx_p->gISF_VdoEnc_ts_sync);
			UINT32 enc = (UINT32)p_ctx_p->gISF_VdoEnc_ts_enc_cycle;

			//calibration precision-error of trigger timer
			//DBGD(p_ctx_p->gISF_VdoEnc_ecnt);
			if(p_ctx_p->gISF_VdoEnc_ecnt > 2) {
				if(p_ctx_p->gISF_VdoEnc_ts_kick_off == 0) {
					p_ctx_p->gISF_VdoEnc_ts_kick_off = koff;
					#if (DUMP_KICK == ENABLE)
					if((id+ISF_IN_BASE)==WATCH_PORT) {
						DBG_DUMP("^MVdoEnc.in[%d]! ........................................ kick_off=%3d.%06d\r\n",
							id,
							(UINT32)(koff>>32), (UINT32)(koff));
					}
					#endif
				} else {
					INT32 koff_err;
					koff_err = (INT32)HwClock_DiffLongCounter(p_ctx_p->gISF_VdoEnc_ts_kick_off,koff);
					#if (DUMP_KICK == ENABLE)
					//DBG_DUMP("VdoEnc.in[%d]! ... shift, diff = %ld\r\n", id, koff_err);
					#endif
					if(koff_err > 100) {
						if (koff_err < (INT32)(enc-100)) {
							//DBG_DUMP("VdoEnc.in[%d]! ... (~) diff=%ld\r\n", id, koff_err);
							ret = koff_err; //please correct this diff!
						} else {
							//DBG_DUMP("^RVdoEnc.in[%d]! ... (~) diff=%ld\r\n", id, koff_err);
						}
					} else if(koff_err < -100) {
						if((-koff_err) < (INT32)(enc-100)) {
							//DBG_DUMP("VdoEnc.in[%d]! ... (~) diff=%ld\r\n", id, koff_err);
							ret = koff_err; //please correct this diff!
						} else {
							//DBG_DUMP("^RVdoEnc.in[%d]! ... (~) diff=%ld\r\n", id, koff_err);
						}
					}
				}
			}
			#if (DUMP_KICK == ENABLE)
			if((id+ISF_IN_BASE)==WATCH_PORT) {
				DBG_DUMP("^CVdoEnc.in[%d]! ... process=%3d.%06d encode=%3d.%06d kick_off=%3d.%06d\r\n",
					id,
					(UINT32)(pt>>32), (UINT32)(pt),
					(UINT32)(et>>32), (UINT32)(et),
					(UINT32)(koff>>32), (UINT32)(koff));
				if(ret!=0) {
					DBG_DUMP("VdoEnc.in[%d]! ... (~) diff=%ld\r\n", id, (INT32)(ret));
				}
			}
			#endif
		}
	}
	#endif
	return ret;
}

static UINT32 _ISF_VdoEnc_RecCB(UINT32 uiEventID, UINT32 param)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;

	switch (uiEventID) {
	default:
		break;

	case NMI_VDOENC_EVENT_BS_CB:
		if (param) {
			UINT32 id = ((NMI_VDOENC_BS_INFO *)param)->PathID;
			VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[id];

			SEM_WAIT(p_ctx_p->ISF_VDOENC_PARTIAL_SEM_ID);
			_ISF_VdoEnc_StreamReadyCB((NMI_VDOENC_BS_INFO *)param);
			SEM_SIGNAL(p_ctx_p->ISF_VDOENC_PARTIAL_SEM_ID);
		} else {
			DBG_ERR("event ID 0x%X Parameter is NULL!\r\n", uiEventID);
		}
		break;

#ifdef ISF_TS
	case NMI_VDOENC_EVENT_ENC_BEGIN:
		if (param) {
			_ISF_VdoEnc_EncBeginCB((NMI_VDOENC_BS_INFO *)param);
		} else {
			DBG_ERR("event ID 0x%X Parameter is NULL!\r\n", uiEventID);
		}
		break;

	case NMI_VDOENC_EVENT_ENC_END:
		if (param) {
			_ISF_VdoEnc_EncEndCB((NMI_VDOENC_BS_INFO *)param);
		} else {
			DBG_ERR("event ID 0x%X Parameter is NULL!\r\n", uiEventID);
		}
		break;
#endif

	case NMI_VDOENC_EVENT_NOTIFY_IPL:
		{
			UINT32 id = param;
			//[isf_stream] ISF_PORT* pSrcPort = _isf_unit_in(&isf_vdoenc, ISF_IN(id));  //ImageUnit_In(&ISF_VdoEnc, id+ISF_IN_BASE);
			ISF_PORT* pSrcPort = isf_vdoenc.p_base->iport(&isf_vdoenc, ISF_IN(id));  // [isf_flow] use this ...
			if(pSrcPort->connecttype == (ISF_CONNECT_PUSH|ISF_CONNECT_SYNC)) {
				//Notify ipl to output
				return _ISF_VdoEnc_OnSync(param);
			}
		}
		break;

	case NMI_VDOENC_EVENT_INFO_READY:          // Get SPS/PPS
		if (param) {
			_ISF_VdoEnc_SetStreamHeader((NMI_VDOENC_BS_INFO *)param);
		} else {
			DBG_ERR("event ID 0x%X Parameter is NULL!\r\n", uiEventID);
		}
		break;
	case NMI_VDOENC_EVENT_RAW_FRAME_COUNT:
		{
			NMI_VDOENC_BS_INFO *bs = (NMI_VDOENC_BS_INFO *)param;
			VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[bs->PathID];
			p_ctx_p->g_raw_frame_count = bs->FrameCount;
		}
		break;

	case NMI_VDOENC_EVENT_SNAPSHOT_CB:
		{
			MP_VDOENC_YUV_SRCOUT *p_srcout = (MP_VDOENC_YUV_SRCOUT *)param;
			UINT32 id = p_srcout->path_id;
			VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[id];

			p_ctx_p->gISF_VdoEnc_SnapSize      = p_srcout->snap_size;
			p_ctx_p->gISF_VdoEnc_SnapTimestamp = p_srcout->timestamp;

			SEM_SIGNAL(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_IDLE);
			SEM_SIGNAL(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_DONE);
		}
		break;

	case NMI_VDOENC_EVENT_GET_BS_START_PA:
		{
			NMI_VDOENC_BS_INFO *bs = (NMI_VDOENC_BS_INFO *)param;
			return 	VDOENC_VIRT2PHY(bs->PathID, bs->Addr);
		}

	case NMI_VDOENC_EVENT_GET_BS_START_PA2:
		{
			NMI_VDOENC_BS_INFO *bs = (NMI_VDOENC_BS_INFO *)param;
			return	VDOENC_VIRT2PHY(bs->PathID, bs->Addr2);
		}
	}


	if (p_ctx_c->gfn_vdoenc_cb) {
		p_ctx_c->gfn_vdoenc_cb(isf_vdoenc.unit_name, uiEventID, param);
	}
	return 0;
}

UINT32 gtestCap = 0;
static void _ISF_VdoEnc_ImgCapCB(UINT32 event, UINT32 param)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;

	switch (event) {
	default:
		break;

	case NMI_IMGCAP_EVENT_ENC_OK: {
			ISF_PORT            *pDestPort    = NULL;
			ISF_DATA             RawEncData   = {0};
			VDO_BITSTREAM        VStrmBuf     = {0};
			NMI_IMGCAP_JPG      *jpg          = (NMI_IMGCAP_JPG *) param;
			VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[jpg->path_id];

			if (p_ctx_p->gISF_ImgCap_Thumb_Job.job_id == jpg->job_id) {
				// Thumbnail
				VStrmBuf.sign     = MAKEFOURCC('T', 'H', 'U', 'M');   // VStrmBuf.flag
				DBG_DUMP("thumb!\r\n");
			} else {
				VStrmBuf.sign     = MAKEFOURCC('J', 'S', 'T', 'M');   // VStrmBuf.flag
				DBG_DUMP("snapshot!\r\n");
			}

			//RawEncData.sign      = ISF_SIGN_DATA;
			//RawEncData.p_class   = &_isf_imgcap_base;
			//[isf_stream] _isf_unit_init_data(&RawEncData, &_isf_imgcap_base, MAKEFOURCC('V','F','R','M'), 0x00010000);
			isf_vdoenc.p_base->init_data(&RawEncData, &_isf_imgcap_base, MAKEFOURCC('V','F','R','M'), 0x00010000); // [isf_flow] use this ...

			VStrmBuf.DataAddr    = jpg->addr;
			VStrmBuf.DataSize    = jpg->size;

			// Set BS ISF_DATA info
			memcpy(&RawEncData.desc, &VStrmBuf, sizeof(VDO_BITSTREAM));  // _TODO : be overwrite by custom FOURCC, what happen next?? Jeah only accept default 5 type now !!

			//[isf_stream] pDestPort =  _isf_unit_out(&isf_vdoenc, ISF_OUT(jpg->path_id));  //ImageUnit_Out(&ISF_VdoEnc, jpg->path_id);
			pDestPort =  isf_vdoenc.p_base->oport(&isf_vdoenc, ISF_OUT(jpg->path_id)); // [isf_flow] use this ...

			if (pDestPort) {

                //remove 2017/11/28, add BSMUXER_CBEVENT_KICKTHUMB
                //if (p_ctx_p->gISF_ImgCap_Thumb_Job.job_id == jpg->job_id) // thumbnail
                //{
                    //if (gtestCap == 1)
                    //    break;

                    //gtestCap = 1; // do once
                //}

                isf_vdoenc.p_base->do_push(&isf_vdoenc, ISF_OUT(jpg->path_id), &RawEncData, 0);
			}
		}
		break;
	}

	if (p_ctx_c->gfn_vdoenc_cb) {
		p_ctx_c->gfn_vdoenc_cb("ImgCap", event, param);
	}
}

static ISF_RV _ISF_VdoEnc_UpdatePortInfo(ISF_UNIT *pThisUnit, UINT32 oPort, ISF_PORT_CMD cmd)
{
	UINT32        iPort    = (oPort + ISF_IN_BASE);
	///[isf_stream] ISF_VDO_INFO *pImgInfo = (ISF_VDO_INFO *) & (_isf_unit_in(pThisUnit, iPort)->info);  //ISF_IMG_INFO *pImgInfo = (ISF_IMG_INFO *) & (ImageUnit_In(pThisUnit, iPort)->Info);
	ISF_VDO_INFO *pImgInfo = (ISF_VDO_INFO *) (pThisUnit->port_ininfo[iPort-ISF_IN_BASE]); // [isf_flow] ust this....
	USIZE         imgsize  = {0};
	//[isf_stream] ISF_PORT* pSrcPort = _isf_unit_in(pThisUnit, iPort);   //ImageUnit_In(pThisUnit, iPort);
	ISF_PORT* pSrcPort = pThisUnit->p_base->iport(pThisUnit, iPort); // [isf_flow] use this ...
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[oPort];
#ifdef __KERNEL__
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
#endif

	if (oPort >= ISF_OUT(16)) {   //if (oPort >= ISF_OUT17) {
		return ISF_OK;
	}

    if (pImgInfo->dirty) {
        // check rotate first (because for JPG roate, we need to check if need rotate to set W/H -> H/W later)
        if (pImgInfo->dirty & ISF_INFO_DIRTY_DIRECT) {
            UINT32 rotate = MP_VDOENC_ROTATE_DISABLE;

            if (pImgInfo->direct == VDO_DIR_ROTATE_0)
                rotate = MP_VDOENC_ROTATE_DISABLE;
            else if (pImgInfo->direct == VDO_DIR_ROTATE_90)
                rotate = MP_VDOENC_ROTATE_CW;
#if defined(_BSP_NA51055_)
            else if (pImgInfo->direct == VDO_DIR_ROTATE_180)
                rotate = MP_VDOENC_ROTATE_180;
#endif
            else if (pImgInfo->direct == VDO_DIR_ROTATE_270)
                rotate = MP_VDOENC_ROTATE_CCW;

#ifdef __KERNEL__
            if (oPort < BUILTIN_VDOENC_PATH_ID_MAX && p_ctx_p->gISF_VStrmBuf.CodecType != MEDIAVIDENC_MJPG) {
                if (p_ctx_c->bIs_fastboot_enc[oPort]) {
                    if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[oPort]) {
                        VdoEnc_Builtin_GetParam(oPort, BUILTIN_VDOENC_INIT_PARAM_DIRECT, &rotate);
                        if (rotate == MP_VDOENC_ROTATE_DISABLE) {
                            pImgInfo->direct = VDO_DIR_ROTATE_0;
                        } else if (rotate == MP_VDOENC_ROTATE_CW) {
                            pImgInfo->direct = VDO_DIR_ROTATE_90;
                        } else if (rotate == MP_VDOENC_ROTATE_180) {
                            pImgInfo->direct = VDO_DIR_ROTATE_180;
                        } else if (rotate == MP_VDOENC_ROTATE_CCW) {
                            pImgInfo->direct = VDO_DIR_ROTATE_270;
                        }
                    }
                }
            }
#endif

			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM, pThisUnit, iPort, "DIRECT = 0x%08x , rotate =   %u", pImgInfo->direct, rotate);

            pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_ROTATE, rotate);
			pImgInfo->dirty &= ~ISF_INFO_DIRTY_DIRECT;
        }

        // for JPG rotate, because JPEG doesn't support HW rotate
        //    so we have to do YUV rotate at _ISF_VdoEnc_InputPort_PushImgBufToDest(), and set W/H -> H/W to JPEG engine
        if (pImgInfo->dirty & ISF_INFO_DIRTY_IMGSIZE) {
            UINT32 w = 0;
            UINT32 h = 0;

            w = pImgInfo->imgsize.w;
            h = pImgInfo->imgsize.h;
            ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM, pThisUnit, iPort, "IMGSIZE = %u x %u", w, h);

            // if JPG rotate, convert W/H -> H/W here
            if (p_ctx_p->gISF_VStrmBuf.CodecType == MEDIAVIDENC_MJPG) {
                if ((pImgInfo->direct == VDO_DIR_ROTATE_90) || (pImgInfo->direct == VDO_DIR_ROTATE_270)) {
                    w = pImgInfo->imgsize.h;
                    h = pImgInfo->imgsize.w;
                }
            }

            pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_WIDTH, w);
            pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_HEIGHT, h);
            p_ctx_p->gISF_VStrmBuf.Width  = w;
            p_ctx_p->gISF_VStrmBuf.Height = h;
			pImgInfo->dirty &= ~ISF_INFO_DIRTY_IMGSIZE;
        }

        if (pImgInfo->dirty & ISF_INFO_DIRTY_FRAMERATE) {
            //UINT32 id = oPort - ISF_OUT_BASE;
            INT16 dst_fr = GET_HI_UINT16(pImgInfo->framepersecond);
            INT16 src_fr = GET_LO_UINT16 (pImgInfo->framepersecond);

            if (cmd != ISF_PORT_CMD_RUNTIME_UPDATE) {
                ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM, &isf_vdoenc, iPort, "frc(%d/%d)", dst_fr, src_fr);
            }

            if (pImgInfo->framepersecond == VDO_FRC_DIRECT) {
                DBG_WRN("unsupport frc(%u/%u), set to default frc(1/1)\r\n", dst_fr, src_fr);
                p_ctx_p->gISF_VdoEnc_framepersecond = VDO_FRC_ALL;
                pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TIMELAPSE_TIME, 0);// Set TimeLapse time = 0 for direct mode
                pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TRIGGER_MODE, NMI_VDOENC_TRIGGER_DIRECT);
            }

            if((dst_fr==1) && (src_fr==-1)) { // ==> frame is triggered by user, fps = [unknown]
#if 0
                // => VdoEnc 設定為 DIRECT_TRIGGER
				ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM, &isf_vdoenc, ISF_OUT(oPort), "frc(?)");
                pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_FRAMERATE, 0);
                pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TRIGGER_MODE, NMI_VDOENC_TRIGGER_DIRECT);
#else
                DBG_WRN("unsupport frc(%u/%u), set to default frc(1/1)\r\n", dst_fr, src_fr);
                p_ctx_p->gISF_VdoEnc_framepersecond = VDO_FRC_ALL;
                pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TIMELAPSE_TIME, 0);// Set TimeLapse time = 0 for direct mode
                pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TRIGGER_MODE, NMI_VDOENC_TRIGGER_DIRECT);
#endif
            }
            else
            if((dst_fr> 1) && (src_fr>=1) && (dst_fr> src_fr)) { // ==> frame is triggered by dest port, fps = dst_fr/src_fr
#if 1
				if(pSrcPort->connecttype == (ISF_CONNECT_PUSH|ISF_CONNECT_SYNC)) {
					ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM, &isf_vdoenc, ISF_OUT(oPort), "frc(%d/%d)", dst_fr, src_fr);
					_ISF_VDOENC_InitFrc(oPort, &(p_ctx_p->gISF_VdoEnc_frc), pImgInfo->framepersecond);
					p_ctx_p->gISF_VdoEnc_frc.FramePerSecond_new = pImgInfo->framepersecond;
					pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_FRAMERATE, 1000*src_fr);
					//=> VdoEnc 設定為 NOTIFY_TRIGGER
					pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TRIGGER_MODE, NMI_VDOENC_TRIGGER_NOTIFY);
				} else {
					ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM, &isf_vdoenc, ISF_OUT(oPort), "frc(%d/%d)", dst_fr, src_fr);
					DBG_IND("frc(%u/%u), will trigger TIMER_MODE !!\r\n", dst_fr, src_fr);
					//=> VdoEnc 設定為 TIMER_TRIGGER FRAMERATE = dst_fr/src_fr;
					pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_FRAMERATE, (UINT32) (1000*dst_fr/src_fr));
					pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TRIGGER_MODE, NMI_VDOENC_TRIGGER_TIMER);
					p_ctx_p->gISF_VStrmBuf.framepersecond = (UINT32) (dst_fr/src_fr);
					p_ctx_p->gISF_VdoEnc_framepersecond = VDO_FRC_ALL;
                }
#else
                DBG_WRN("unsupport frc(%u/%u), set to default frc(1/1)\r\n", dst_fr, src_fr);
                p_ctx_p->gISF_VdoEnc_framepersecond = VDO_FRC_ALL;

                pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TRIGGER_MODE, NMI_VDOENC_TRIGGER_DIRECT);
#endif
            }
            else
            if((dst_fr>=1) && (src_fr>=1) && (dst_fr==src_fr)) { // ==> frame is triggered by src port, fps = upstream_fps*1/1
				if(pSrcPort->connecttype == (ISF_CONNECT_PUSH|ISF_CONNECT_SYNC)) {
					ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM, &isf_vdoenc, ISF_OUT(oPort), "frc(%d/%d)", dst_fr, src_fr);
					_ISF_VDOENC_InitFrc(oPort, &(p_ctx_p->gISF_VdoEnc_frc), pImgInfo->framepersecond);
					p_ctx_p->gISF_VdoEnc_frc.FramePerSecond_new = pImgInfo->framepersecond;
					pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_FRAMERATE, 1000*src_fr);
					//=> VdoEnc 設定為 NOTIFY_TRIGGER
					pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TRIGGER_MODE, NMI_VDOENC_TRIGGER_NOTIFY);
				} else {
					//=> VdoEnc 設定為 DIRECT_TRIGGER
					#if 0
					UINT32 fps;
					if(p_ctx_p->ISF_VdoSrcUnit == NULL) {
						fps = 0;
						DBG_ERR("VdoEnc.out[%d] 1:1 trace fps failed! (-1)\r\n", id);
					} else {
						fps = ImageUnit_QueryUpstreamFramerate(p_ctx_p->ISF_VdoSrcUnit, id+ISF_IN_BASE, pThisUnit);
						dst_fr = GET_HI_UINT16(fps);
						src_fr = GET_LO_UINT16 (fps);
						ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM, &isf_vdoenc, id + ISF_OUT_BASE, "frc(%d/%d)", dst_fr, src_fr);
						if (src_fr == 0){
							fps = 0;
						} else {
							fps = dst_fr/src_fr;
						}
					}
					pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_FRAMERATE, 1000*fps);
					#endif
					pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TIMELAPSE_TIME, 0);// Set TimeLapse time = 0 for direct mode
					pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TRIGGER_MODE, NMI_VDOENC_TRIGGER_DIRECT);

					p_ctx_p->gISF_VdoEnc_framepersecond = pImgInfo->framepersecond;
				}
            }
            else
            if((dst_fr>=1) && (src_fr> 1) && (dst_fr< src_fr)) { // ==> frame is triggered by src port, fps = upstream_fps*dst_fr/src_fr
				if(pSrcPort->connecttype == (ISF_CONNECT_PUSH|ISF_CONNECT_SYNC)) {
					ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM, &isf_vdoenc, ISF_OUT(oPort), "frc(%d/%d)", dst_fr, src_fr);
					_ISF_VDOENC_InitFrc(oPort, &(p_ctx_p->gISF_VdoEnc_frc), pImgInfo->framepersecond);
					p_ctx_p->gISF_VdoEnc_frc.FramePerSecond_new = pImgInfo->framepersecond;
					pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_FRAMERATE, 1000*src_fr);
					//=> VdoEnc 設定為 NOTIFY_TRIGGER
					pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TRIGGER_MODE, NMI_VDOENC_TRIGGER_NOTIFY);
				} else {
					//=> VdoEnc 設定為 DIRECT_TRIGGER
					#if 0
					UINT32 fps;
					if(p_ctx_p->ISF_VdoSrcUnit == NULL) {
						fps = 0;
						DBG_ERR("VdoEnc.out[%d] dst:src trace fps failed! (-1)\r\n", id);
					} else {
						fps = ImageUnit_QueryUpstreamFramerate(p_ctx_p->ISF_VdoSrcUnit, id+ISF_IN_BASE, pThisUnit);
						dst_fr = GET_HI_UINT16(fps);
						src_fr = GET_LO_UINT16 (fps);
						ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM, &isf_vdoenc, ISF_OUT(oPort), "frc(%d/%d)", dst_fr, src_fr);
						if (src_fr == 0){
							fps = 0;
						} else {
							fps = dst_fr/src_fr;
						}
					}
					pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_FRAMERATE, 1000*fps);
					#endif
					pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TIMELAPSE_TIME, 0);// Set TimeLapse time = 0 for direct mode
					pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TRIGGER_MODE, NMI_VDOENC_TRIGGER_DIRECT);

					p_ctx_p->gISF_VdoEnc_framepersecond = pImgInfo->framepersecond;
				}
            }
            else
            if((dst_fr==0) && (src_fr> 1)) {
                pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TIMELAPSE_TIME, 0);// Set TimeLapse time = 0 for direct mode
                pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TRIGGER_MODE, NMI_VDOENC_TRIGGER_DIRECT);
                p_ctx_p->gISF_VdoEnc_framepersecond = pImgInfo->framepersecond;
            }
            else
            {
                DBG_ERR("unexpected frc(%u/%u) !! set to default frc(1/1)\r\n", dst_fr, src_fr);
                pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TIMELAPSE_TIME, 0);// Set TimeLapse time = 0 for direct mode
                pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_TRIGGER_MODE, NMI_VDOENC_TRIGGER_DIRECT);
                p_ctx_p->gISF_VdoEnc_framepersecond = VDO_FRC_ALL;
            }

            if (cmd == ISF_PORT_CMD_RUNTIME_UPDATE) {
                ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM_IMM, &isf_vdoenc, iPort, "frc(%d/%d)", dst_fr, src_fr);

                if (p_ctx_p->gISF_VdoEnc_infrc.sample_mode == ISF_SAMPLE_OFF) {
                    _isf_frc_start(&isf_vdoenc, ISF_IN(iPort), &(p_ctx_p->gISF_VdoEnc_infrc), p_ctx_p->gISF_VdoEnc_framepersecond);
                } else {
                    // update frc_imm
                    _isf_frc_update_imm(&isf_vdoenc, ISF_IN(iPort), &(p_ctx_p->gISF_VdoEnc_infrc), p_ctx_p->gISF_VdoEnc_framepersecond);
                }
            }

			pImgInfo->dirty &= ~ISF_INFO_DIRTY_FRAMERATE;
        }

		if (pImgInfo->dirty & ISF_INFO_DIRTY_IMGFMT) {
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM, pThisUnit, iPort, "IMGFMT = 0x%08x", pImgInfo->imgfmt);
			pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_IMGFMT, pImgInfo->imgfmt);
			pImgInfo->dirty &= ~ISF_INFO_DIRTY_IMGFMT;
		}

        #if 0
        if (pImgInfo->Dirty & ISF_INFO_DIRTY_ASPECT) {
            pImgInfo->ImgAspect.w
            pImgInfo->ImgAspect.h
            pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_DAR, value);
        }
        #endif

        if (pImgInfo->dirty & ISF_INFO_DIRTY_VDOMAX) {
            if (pImgInfo->buffercount <= ISF_VDOENC_DATA_QUE_NUM) {
            	ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM, &isf_vdoenc, iPort, "[ISF_VDOENC][%d] Update queue count = %d\r\n", oPort, pImgInfo->buffercount);
            	p_ctx_p->gISF_VdoEnc_Data_Que_Num = pImgInfo->buffercount;
            }
			pImgInfo->dirty &= ~ISF_INFO_DIRTY_VDOMAX;
        }

		if (p_ctx_p->gISF_VdoEnc_Data_Que_Num == 0) {
			p_ctx_p->gISF_VdoEnc_Data_Que_Num = ISF_VDOENC_DATA_QUE_NUM_DEFAULT;
#if MP_VDOENC_SHOW_MSG
			#if 0
				DBG_DUMP("[ISF_VDOENC][%d] Default queue count = %d\r\n", oPort, p_ctx_p->gISF_VdoEnc_Data_Que_Num);
			#else
				ISF_UNIT_TRACE(ISF_OP_PARAM_VDOFRM, &isf_vdoenc, iPort, "[ISF_VDOENC][%d] Default queue count = %d\r\n", oPort, p_ctx_p->gISF_VdoEnc_Data_Que_Num);
			#endif
#endif
		}
    }

	pNMI_VdoEnc->GetParam(oPort, NMI_VDOENC_PARAM_WIDTH, &imgsize.w);
	pNMI_VdoEnc->GetParam(oPort, NMI_VDOENC_PARAM_HEIGHT, &imgsize.h);

	// if JPG rotate, rollback H/W -> W/H for pImgInfo
	if (p_ctx_p->gISF_VStrmBuf.CodecType == MEDIAVIDENC_MJPG) {
		if ((pImgInfo->direct == VDO_DIR_ROTATE_90) || (pImgInfo->direct == VDO_DIR_ROTATE_270)) {
			UINT32 tmp = 0;
			tmp = imgsize.w;
			imgsize.w = imgsize.h;
			imgsize.h = tmp;
		}
	}

	pImgInfo->max_imgsize.w = imgsize.w;
	pImgInfo->max_imgsize.h = imgsize.h;
	pImgInfo->imgsize.w    = imgsize.w;
	pImgInfo->imgsize.h    = imgsize.h;
	////pImgInfo->imgfmt       = VDO_PXLFMT_YUV420;   //GX_IMAGE_PIXEL_FMT_YUV420_PACKED;
	pImgInfo->imgaspect.w  = imgsize.w;
	pImgInfo->imgaspect.h  = imgsize.h;

	return ISF_OK;

}

static ISF_RV _ISF_VdoEnc_AllocMem(ISF_UNIT *pThisUnit, UINT32 id)
{
	ISF_RV           r          = ISF_OK;
	NMI_VDOENC_MEM_RANGE Mem    = {0};
	UINT32           uiBufAddr  = 0;
	UINT32           uiTrigBsQSize = 0;
	UINT32           uiBufSize  = 0, uiRawQueSize = 0, uiNmrBsQueSize = 0, uiPullQueSize = 0;
	UINT32           uiBsDataBlkMax = ISF_VDOENC_BSDATA_BLK_MAX;
	//[isf_stream] ISF_PORT* 		 pSrcPort 	= _isf_unit_out(pThisUnit, ISF_OUT(id));    //ImageUnit_Out(pThisUnit, ISF_OUT_BASE+id);
	ISF_PORT* 		 pSrcPort 	= pThisUnit->p_base->oport(pThisUnit, ISF_OUT(id)); // [isf_flow] use this ...
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[id];
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;

#if (1) // HDAL can't get width/height until START, so do this at START
	pNMI_VdoEnc->GetParam(id, NMI_VDOENC_PARAM_ALLOC_SIZE, &uiBufSize);
#endif

#ifdef __KERNEL__
	if (id < BUILTIN_VDOENC_PATH_ID_MAX) {
		if (p_ctx_c->bIs_fastboot_enc[id]) {
			if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[id]) {
				uiBsDataBlkMax = g_vdoenc_fastboot_bsdata_blk_max[id];
			}
		}
	}
#endif

	// [Hack] create oport... because (isf_flow + HDAL) doestn't actually having oport
	pSrcPort = (ISF_PORT*)(0xffffff00 | ISF_OUT(id));

	//if (uiBufSize == 0 && p_ctx_p->gISF_VdoEncMaxMem.Size == 0) {
	if (uiBufSize == 0 && p_ctx_p->gISF_VdoEncMaxMemSize == 0) {
		DBG_ERR("Get port %d buffer from NMI_VdoEnc is Zero!\r\n", id);
		return ISF_ERR_INVALID_VALUE;
	}

	//nvtmpp_vb_add_module(pThisUnit->unit_module);   // because AllocMem is doing at "first time START",  move this register to OPEN()

	if (p_ctx_p->gISF_VdoEncMaxMemSize != 0 && p_ctx_p->gISF_VdoEncMaxMem.Addr == 0) {
		if (uiBufSize > p_ctx_p->gISF_VdoEncMaxMemSize) {
			DBG_ERR("[ISF_VDOENC][%d] Need mem size (%d) > max public size (%d)\r\n", id, uiBufSize, p_ctx_p->gISF_VdoEncMaxMemSize);
			pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_MEM_RANGE, (UINT32) &Mem);  //reset enc buf addr to 0
			return ISF_ERR_BUFFER_CREATE;
		}
		uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "max", p_ctx_p->gISF_VdoEncMaxMemSize, &(p_ctx_p->gISF_VdoEncMaxMemBlk));
		if (uiBufAddr != 0) {
			p_ctx_p->gISF_VdoEncMaxMem.Addr = uiBufAddr;
			p_ctx_p->gISF_VdoEncMaxMem.Size = p_ctx_p->gISF_VdoEncMaxMemSize;
#if (SUPPORT_PULL_FUNCTION)
			// set mmap info
			p_ctx_p->gISF_VdoEncMem_MmapInfo.addr_virtual  = uiBufAddr;
			p_ctx_p->gISF_VdoEncMem_MmapInfo.addr_physical = nvtmpp_vb_blk2pa(p_ctx_p->gISF_VdoEncMaxMemBlk.h_data);
			p_ctx_p->gISF_VdoEncMem_MmapInfo.size          = p_ctx_p->gISF_VdoEncMaxMemSize;
#endif
		} else {
			DBG_ERR("[ISF_VDOENC][%d] get max blk failed!\r\n", id);
			return ISF_ERR_BUFFER_CREATE;
		}
	}

	// create enc buf
	if (p_ctx_p->gISF_VdoEncMaxMem.Addr != 0 && p_ctx_p->gISF_VdoEncMaxMem.Size != 0) {
		Mem.Addr = p_ctx_p->gISF_VdoEncMaxMem.Addr;
		Mem.Size = p_ctx_p->gISF_VdoEncMaxMem.Size;
	} else {
		uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "work", uiBufSize, &(p_ctx_p->gISF_VdoEncMemBlk));
		if (uiBufAddr == 0) {
			DBG_ERR("[ISF_VDOENC][%d] get enc blk failed!\r\n", id);
			return ISF_ERR_BUFFER_CREATE;
		}

		Mem.Addr = uiBufAddr;
		Mem.Size = uiBufSize;
#if (SUPPORT_PULL_FUNCTION)
		// set mmap info
		p_ctx_p->gISF_VdoEncMem_MmapInfo.addr_virtual  = uiBufAddr;
		p_ctx_p->gISF_VdoEncMem_MmapInfo.addr_physical = nvtmpp_vb_blk2pa(p_ctx_p->gISF_VdoEncMemBlk.h_data);
		p_ctx_p->gISF_VdoEncMem_MmapInfo.size          = uiBufSize;
#endif
	}

#if (1) // HDAL can't get width/height until START, so do this at START
	pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_MEM_RANGE, (UINT32) &Mem);
#endif

	// source out buffer from common pool
	if (p_ctx_p->gISF_VdoEncCommSrcOut_Enable) {
		if (p_ctx_p->gISF_VdoEncCommSrcOutAddr && p_ctx_p->gISF_VdoEncCommSrcOutSize) {
			UINT32 uiSrcOutSize= 0;

			pNMI_VdoEnc->GetParam(id, NMI_VDOENC_PARAM_SNAPSHOT_SIZE, &uiSrcOutSize);
			if (uiSrcOutSize > p_ctx_p->gISF_VdoEncCommSrcOutSize) {
				DBG_ERR("[ISF_VDOENC][%d] Need snapshot mem size (%d) > max snapshot public size (%d)\r\n", id, uiSrcOutSize, p_ctx_p->gISF_VdoEncCommSrcOutSize);
				Mem.Addr = 0;
				Mem.Size = 0;
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_SNAPSHOT_RANGE, (UINT32) &Mem);  //reset snapshot buf addr to 0
				return ISF_ERR_BUFFER_CREATE;
			}

			Mem.Addr = p_ctx_p->gISF_VdoEncCommSrcOutAddr;
			Mem.Size = p_ctx_p->gISF_VdoEncCommSrcOutSize;

			pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_SNAPSHOT_RANGE, (UINT32) &Mem);

			p_ctx_p->gISF_VdoEncCommSrcOut_FromUser = 1;
		} else {
			UINT32 uiSrcOutAddr = 0, uiSrcOutSize= 0;
			static ISF_DATA_CLASS vdoenc_srcout_data = {0};

			pNMI_VdoEnc->GetParam(id, NMI_VDOENC_PARAM_SNAPSHOT_SIZE, &uiSrcOutSize);
			if (uiSrcOutSize == 0) {
				DBG_ERR("Get port %d source out frame info fail! uiBufSize %d\r\n", id, uiSrcOutSize);
				return ISF_ERR_INVALID_VALUE;
			}

			if (uiSrcOutSize > p_ctx_p->gISF_VdoEncSnapShotSize) {
				DBG_ERR("[ISF_VDOENC][%d] Need snapshot mem size (%d) > max snapshot public size (%d)\r\n", id, uiSrcOutSize, p_ctx_p->gISF_VdoEncSnapShotSize);
				Mem.Addr = 0;
				Mem.Size = 0;
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_SNAPSHOT_RANGE, (UINT32) &Mem);  //reset snapshot buf addr to 0
				return ISF_ERR_BUFFER_CREATE;
			}

			pThisUnit->p_base->init_data(&(p_ctx_p->gISF_VdoEncSnapShotBlk), &vdoenc_srcout_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
			uiSrcOutAddr = pThisUnit->p_base->do_new(pThisUnit, ISF_OUT(id), NVTMPP_VB_INVALID_POOL, 0, uiSrcOutSize,  &(p_ctx_p->gISF_VdoEncSnapShotBlk), ISF_OUT_PROBE_NEW);
			if (uiSrcOutAddr == 0) {
				DBG_ERR("[ISF_VDOENC][%d] get source out blk failed!, uiSrcOutSize 0x%x\r\n", id, uiSrcOutSize);
				return ISF_ERR_BUFFER_CREATE;
			}

			Mem.Addr = uiSrcOutAddr;
			Mem.Size = uiSrcOutSize;

			pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_SNAPSHOT_RANGE, (UINT32) &Mem);

			p_ctx_p->gISF_VdoEncCommSrcOut_FromUser = 0;
		}
	}

	// reconstruct frame from common pool
	if (p_ctx_p->gISF_VdoEncCommReconBuf_Enable && p_ctx_p->gISF_VStrmBuf.CodecType != MEDIAVIDENC_MJPG) {
		UINT32 i = 0, uiRecBufAddr = 0, uiRecBufSize= 0, uiFrmNum = 0;
		NMI_VDOENC_RECBUF_RANGE Recbuf = {0};
		pNMI_VdoEnc->GetParam(id, NMI_VDOENC_PARAM_RECON_FRAME_SIZE, &uiRecBufSize);
		pNMI_VdoEnc->GetParam(id, NMI_VDOENC_PARAM_RECON_FRAME_NUM, &uiFrmNum);
		p_ctx_p->gISF_VdoEncReconFrmNum = uiFrmNum;

		if (uiRecBufSize == 0 || uiFrmNum == 0) {
			DBG_ERR("Get port %d reconstruct frame info fail! uiBufSize %d, uiFrmNum %d\r\n", id, uiRecBufSize, uiFrmNum);
			return ISF_ERR_INVALID_VALUE;
		}

		if (p_ctx_p->gISF_VdoEncRecFrmCfgSVCLayer != KDRV_VDOENC_SVC_4X && p_ctx_p->gISF_VdoEncRecFrmCfgLTRInterval != 0) {
			p_ctx_p->gISF_VdoEncUseCommReconBuf[1] = p_ctx_p->gISF_VdoEncUseCommReconBuf[2];
		}

		for (i = 0; i < uiFrmNum; i++) {
			static ISF_DATA_CLASS vdoenc_out_data = {0};
			if (p_ctx_p->gISF_VdoEncUseCommReconBuf[i]) {
				pThisUnit->p_base->init_data(&(p_ctx_p->gISF_VdoEncReconFrmBlk[i]), &vdoenc_out_data, MAKEFOURCC('V','F','R','M'), 0x00010000);
				uiRecBufAddr = pThisUnit->p_base->do_new(pThisUnit, ISF_OUT(id), NVTMPP_VB_INVALID_POOL, 0, uiRecBufSize,  &(p_ctx_p->gISF_VdoEncReconFrmBlk[i]), ISF_OUT_PROBE_NEW);
			} else {
				uiRecBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "reconfrm", uiRecBufSize, &(p_ctx_p->gISF_VdoEncReconFrmBlk[i]));
			}
			if (uiRecBufAddr == 0) {
				DBG_ERR("Get port %d reconstruct frame addr fail!\r\n", id);
				return ISF_ERR_INVALID_VALUE;
			}
			Recbuf.Addr = uiRecBufAddr;
			Recbuf.Size = uiRecBufSize;
			Recbuf.Idx	= i;
			pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_RECONFRM_RANGE, (UINT32) &Recbuf);
		}
	}

	// create raw queue, nmr trig bs queue and pull queue
	uiRawQueSize = sizeof(ISF_DATA) * p_ctx_p->gISF_VdoEnc_Data_Que_Num;
	if (uiRawQueSize == 0) {
		DBG_ERR("[ISF_VDOENC][%d] get raw que size failed!\r\n", id);
		return ISF_ERR_INVALID_VALUE;
	}

	pNMI_VdoEnc->GetParam(id, NMI_VDOENC_PARAM_TRIGBSQ_SIZE, &uiTrigBsQSize);
	uiNmrBsQueSize = uiTrigBsQSize * uiBsDataBlkMax;
	if (uiNmrBsQueSize == 0) {
		DBG_ERR("[ISF_VDOENC][%d] get nmr bs que size failed!\r\n", id);
		return ISF_ERR_INVALID_VALUE;
	}

#if (SUPPORT_PULL_FUNCTION)
	uiPullQueSize = sizeof(ISF_DATA) * uiBsDataBlkMax;
	if (uiNmrBsQueSize == 0) {
		DBG_ERR("[ISF_VDOENC][%d] get pull que size failed!\r\n", id);
		return ISF_ERR_INVALID_VALUE;
	}
#endif

	uiBufSize = uiRawQueSize + uiNmrBsQueSize + uiPullQueSize;
	uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, pSrcPort, "que", uiBufSize, &(p_ctx_p->gISF_VdoEnc_QueData_MemBlk));
	if (uiBufAddr == 0) {
		DBG_ERR("[ISF_VDOENC][%d] get data blk failed!\r\n", id);
		return ISF_ERR_BUFFER_CREATE;
	}

	// set raw queue address
	p_ctx_p->gISF_VdoEnc_RawDATA = (ISF_DATA *) uiBufAddr;
	uiBufAddr += uiRawQueSize;

	// set nmr trig bs queue address
	pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_BSQ_MEM, uiBufAddr);
	uiBufAddr += uiNmrBsQueSize;

#if (SUPPORT_PULL_FUNCTION)
	// set pull queue address
	{
		PVDOENC_PULL_QUEUE pObj;
		pObj = &p_ctx_p->gISF_VoEnc_PullQueue;
		pObj->Queue = (ISF_DATA *) uiBufAddr;
	}
#endif

	if (p_ctx_c->gISF_VdoEnc_Open_Count == 0) {
		// create bs queue
		uiBufSize = (sizeof(UINT32) + sizeof(VDOENC_BSDATA)) * PATH_MAX_COUNT * uiBsDataBlkMax;
		uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, NULL, "bs", uiBufSize, &p_ctx_c->gISF_VdoEnc_BsData_MemBlk);
		if (uiBufAddr == 0) {
			DBG_ERR("[ISF_VDOENC][%d] get bs blk failed!\r\n", id);
			return ISF_ERR_BUFFER_CREATE;
		}

		p_ctx_c->g_p_vdoenc_bsdata_blk_maptlb = (UINT32 *) uiBufAddr;
		memset(p_ctx_c->g_p_vdoenc_bsdata_blk_maptlb, 0, uiBufSize);
		p_ctx_c->g_p_vdoenc_bsdata_blk_pool = (VDOENC_BSDATA *)(p_ctx_c->g_p_vdoenc_bsdata_blk_maptlb + (PATH_MAX_COUNT * uiBsDataBlkMax));
	}

	p_ctx_c->gISF_VdoEnc_Open_Count++;

	return r;
}

static ISF_RV _ISF_VdoEnc_FreeMem(ISF_UNIT *pThisUnit, UINT32 id, UINT32 b_do_release_i)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[id];
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	ISF_RV r = ISF_OK;
	VDOENC_BSDATA *pVdoEncData = NULL;
	unsigned long         flags = 0;

#if 0 // if user call VDOENC_PARAM_MAX_MEM_INFO with bRelease = 1,  it indeed release MAX buffer, and MAX addr/size is also set to 0..... this will cause exception here ( don't need to release dynamic alloc buffer )
	// release enc buf
	if (p_ctx_p->gISF_VdoEncMaxMem.Addr == 0 || p_ctx_p->gISF_VdoEncMaxMem.Size == 0) {
		r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_VdoEncMemBlk), ISF_OK);
		if (r != ISF_OK) {
			DBG_ERR("[ISF_VDOENC][%d] release VdoEnc blk failed, ret = %d\r\n", id, r);
			return ISF_ERR_BUFFER_DESTROY;
		}
	}
#endif
	// release common snapshot buffer
	if (p_ctx_p->gISF_VdoEncCommSrcOut_Enable && !p_ctx_p->gISF_VdoEncCommSrcOut_FromUser) {
		r = pThisUnit->p_base->do_release(pThisUnit, id, &(p_ctx_p->gISF_VdoEncSnapShotBlk), ISF_IN_PROBE_REL);
		if (r != ISF_OK) {
			DBG_ERR("[ISF_VDOENC][%d] release snapshot blk failed, ret = %d\r\n", id, r);
			return ISF_ERR_BUFFER_DESTROY;
		}
	}

	// release common reconstruct buffer
	if (p_ctx_p->gISF_VdoEncCommReconBuf_Enable && p_ctx_p->gISF_VStrmBuf.CodecType != MEDIAVIDENC_MJPG) {
		UINT32 i = 0;
		for (i = 0; i < p_ctx_p->gISF_VdoEncReconFrmNum; i++) {
			if (p_ctx_p->gISF_VdoEncUseCommReconBuf[i]) {
				r = pThisUnit->p_base->do_release(pThisUnit, id, &(p_ctx_p->gISF_VdoEncReconFrmBlk[i]), ISF_IN_PROBE_REL);
			} else {
				r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_VdoEncReconFrmBlk[i]), ISF_OK);
			}
			if (r != ISF_OK) {
				DBG_ERR("[ISF_VDOENC][%d] release ReconFrm blk failed, ret = %d\r\n", id, r);
				return ISF_ERR_BUFFER_DESTROY;
			}
		}
	}

#ifdef __KERNEL__
	// free builtin bs queue
	if (id < BUILTIN_VDOENC_PATH_ID_MAX) {
		if (p_ctx_c->bIs_fastboot_enc[id]) {
			if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[id]) {
				VdoEnc_Builtin_FreeQueMem(id);
			}
		}
	}
#endif

    memset(&p_ctx_p->gISF_VdoEnc_IsRawUsed, 0, sizeof(p_ctx_p->gISF_VdoEnc_IsRawUsed));

	// release bs queue
	if (p_ctx_c->gISF_VdoEnc_Open_Count > 0)
		p_ctx_c->gISF_VdoEnc_Open_Count--;

	// release left blocks
	pVdoEncData = p_ctx_p->g_p_vdoenc_bsdata_link_head;
	while (pVdoEncData != NULL) {
		_ISF_VdeEnc_FREE_BSDATA(pVdoEncData);
		pVdoEncData = pVdoEncData->pnext_bsdata;
	}
	NMR_VdoEnc_Lock_cpu(&flags);
	p_ctx_p->g_p_vdoenc_bsdata_link_head = NULL;
	p_ctx_p->g_p_vdoenc_bsdata_link_tail = NULL;
	NMR_VdoEnc_Unlock_cpu(&flags);

	// release pull queue
	_ISF_VdoEnc_RelaseAll_PullQueue(id);    // JIRA : NA51055-1600 ,  user may  open->start->stop->close->open->close ,  original close() only clear link-list, but pullq remain => this will cause next time open()->close() fail , because pullq num != link_list num

	// release raw queue
	if (p_ctx_p->gISF_VdoEnc_QueData_MemBlk.h_data != 0) {
		if (TRUE == b_do_release_i) {
			r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_VdoEnc_QueData_MemBlk), ISF_OK);
			if (r != ISF_OK) {
				DBG_ERR("[ISF_VDOENC][%d] release VdoEnc_ISFDATA blk failed, ret = %d\r\n", id, r);
				return ISF_ERR_BUFFER_DESTROY;
			}
		}
		memset(&p_ctx_p->gISF_VdoEnc_QueData_MemBlk, 0, sizeof(ISF_DATA));
	}

	// release meta blk
	if (p_ctx_p->gISF_VdoEncMetaAllocBlk.h_data != 0) {
		if (TRUE == b_do_release_i) {
			r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_VdoEncMetaAllocBlk), ISF_OK);
			if (r != ISF_OK) {
				DBG_ERR("[ISF_VDOENC][%d] release VdoEncMeta blk failed, ret = %d\r\n", id, r);
				return ISF_ERR_BUFFER_DESTROY;
			}
		}
		memset(&p_ctx_p->gISF_VdoEncMetaAllocBlk, 0, sizeof(ISF_DATA));
		p_ctx_p->g_p_vdoenc_meta_alloc_info = NULL;
	}

    if (p_ctx_c->gISF_VdoEnc_Open_Count == 0) {
		if (p_ctx_c->gISF_VdoEnc_BsData_MemBlk.h_data != 0) {
			if (TRUE == b_do_release_i) {
				r = pThisUnit->p_base->do_release_i(pThisUnit, &p_ctx_c->gISF_VdoEnc_BsData_MemBlk, ISF_OK);
				if (r != ISF_OK) {
					DBG_ERR("[ISF_VDOENC][%d] release VdoEnc_BS blk failed, ret = %d\r\n", id, r);
					return ISF_ERR_BUFFER_DESTROY;
				}
			}
			memset(&p_ctx_c->gISF_VdoEnc_BsData_MemBlk, 0, sizeof(ISF_DATA));
		}

		p_ctx_c->g_p_vdoenc_bsdata_blk_pool   = NULL;
		p_ctx_c->g_p_vdoenc_bsdata_blk_maptlb = NULL;

		{
			UINT32 i;
			for (i=0; i< PATH_MAX_COUNT; i++) {
				p_ctx_p->g_p_vdoenc_bsdata_link_head = NULL;	//memset(g_p_vdoenc_bsdata_link_head, 0, sizeof(VDOENC_BSDATA *) * PATH_MAX_COUNT);
				p_ctx_p->g_p_vdoenc_bsdata_link_tail = NULL;	//memset(g_p_vdoenc_bsdata_link_tail, 0, sizeof(VDOENC_BSDATA *) * PATH_MAX_COUNT);
			}
		}
    }

	return r;
}

static ISF_RV _ISF_VdoEnc_FreeMem_BsOnly(ISF_UNIT *pThisUnit, UINT32 id)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[id];
	VDOENC_BSDATA *pVdoEncData = NULL;
	unsigned long         flags = 0;
	// release left blocks
	pVdoEncData = p_ctx_p->g_p_vdoenc_bsdata_link_head;
	while (pVdoEncData != NULL) {
		_ISF_VdeEnc_FREE_BSDATA(pVdoEncData);
		pVdoEncData = pVdoEncData->pnext_bsdata;
	}
	NMR_VdoEnc_Lock_cpu(&flags);
	p_ctx_p->g_p_vdoenc_bsdata_link_head = NULL;
	p_ctx_p->g_p_vdoenc_bsdata_link_tail = NULL;
	NMR_VdoEnc_Unlock_cpu(&flags);

	// release pull queue
	_ISF_VdoEnc_RelaseAll_PullQueue(id);

	return ISF_OK;
}

static ISF_RV _ISF_ImgCap_AllocMem(ISF_UNIT *pThisUnit)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	ISF_RV           r         = ISF_OK;
	NMI_IMGCAP_MEM_RANGE Mem;
	UINT32           uiBufAddr = 0;
	UINT32           uiBufSize = 0;

	pNMI_ImgCap->GetParam(0, NMI_IMGCAP_ALLOC_SIZE, &uiBufSize);

	if (uiBufSize == 0) {
		DBG_ERR("Get port buf from pNMI_ImgCap is 0\r\n");
		return ISF_ERR_INVALID_VALUE;
	}

	nvtmpp_vb_add_module(pThisUnit->unit_module);

	// check and create cap_max buf
	if (p_ctx_c->gISF_ImgCapMaxMemSize != 0 && p_ctx_c->gISF_ImgCapMaxMem.Addr == 0) {
		uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, NULL, "cap_max", p_ctx_c->gISF_ImgCapMaxMemSize, &(p_ctx_c->gISF_ImgCapMaxMemBlk));
		if (uiBufAddr != 0) {
			p_ctx_c->gISF_ImgCapMaxMem.Addr = uiBufAddr;
			p_ctx_c->gISF_ImgCapMaxMem.Size = p_ctx_c->gISF_ImgCapMaxMemSize;
		} else {
			DBG_ERR("ImgCap get max blk failed!\r\n");
		}
	}

	// assign mem addr and size
	if (p_ctx_c->gISF_ImgCapMaxMem.Addr != 0 && p_ctx_c->gISF_ImgCapMaxMem.Size != 0) {
		if (uiBufSize > p_ctx_c->gISF_ImgCapMaxMem.Size) {
			DBG_ERR("ImgCap Need mem size (%d) > max public size (%d)\r\n", uiBufSize, p_ctx_c->gISF_ImgCapMaxMem.Size);
			// reset buf addr and size to 0
			Mem.Addr = 0;
			Mem.Size = 0;
			pNMI_ImgCap->SetParam(0, NMI_IMGCAP_SET_MEM_RANGE, (UINT32) &Mem);
			return ISF_ERR_BUFFER_CREATE;
		}
		Mem.Addr = p_ctx_c->gISF_ImgCapMaxMem.Addr;
		Mem.Size = p_ctx_c->gISF_ImgCapMaxMem.Size;
	} else { // create cap buf
		uiBufAddr = pThisUnit->p_base->do_new_i(pThisUnit, NULL, "cap", uiBufSize, &(p_ctx_c->gISF_ImgCapMemBlk));
		if (uiBufAddr == 0) {
			DBG_ERR("ImgCap get blk failed!\r\n");
			return ISF_ERR_BUFFER_GET;
		}
		Mem.Addr = uiBufAddr;
		Mem.Size = uiBufSize;
	}

	pNMI_ImgCap->SetParam(0, NMI_IMGCAP_SET_MEM_RANGE, (UINT32) &Mem);

	return r;
}

static ISF_RV _ISF_ImgCap_FreeMem(ISF_UNIT *pThisUnit)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	ISF_RV r = ISF_OK;

	// release enc buf
	if (p_ctx_c->gISF_ImgCapMaxMem.Addr == 0 || p_ctx_c->gISF_ImgCapMaxMem.Size == 0) {
		r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_c->gISF_ImgCapMemBlk), ISF_OK);
		if (r != ISF_OK) {
			DBG_ERR("ImgCap release blk failed, ret = %d\r\n", r);
			return ISF_ERR_BUFFER_DESTROY;
		}
	}

	return r;
}

static ISF_RV _ISF_VdoEnc_Reset(ISF_UNIT *pThisUnit, UINT32 oPort)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = NULL;
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;

	if (p_ctx_c->ctx_mem.ctx_addr == 0) {
		return ISF_ERR_UNINIT; //sitll not init
	}

	if (oPort >= PATH_MAX_COUNT) {
		return ISF_ERR_INVALID_PORT_ID;
	}

	p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[oPort];

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
	{
		ISF_RV r;

		if ((p_ctx_p->gISF_VdoEncMaxMem.Addr != 0) && (p_ctx_p->gISF_VdoEncMaxMemBlk.h_data != 0)) {
			ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, pThisUnit, ISF_OUT(oPort), "(release max memory)");
			r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_VdoEncMaxMemBlk), ISF_OK);
			if (r == ISF_OK) {
				p_ctx_p->gISF_VdoEncMaxMem.Addr = 0;
				p_ctx_p->gISF_VdoEncMaxMem.Size = 0;
				p_ctx_p->gISF_VdoEncMaxMemSize = 0;
				memset(&p_ctx_p->gISF_VdoEncMaxMemBlk, 0, sizeof(ISF_DATA));
			} else {
				DBG_ERR("[ISF_VDOENC][%d] release VdoEnc Max blk failed, ret = %d\r\n", oPort, r);
			}
		}
	}

	//if state still = OPEN (maybe Close failed due to user not release all bs yet, Close will never success...so do the close routine here to RESET )
	//         (but DO NOT call do_release_i() , this will not valid after previous nvtmpp uninit().  nvtmpp will forget everything and re-init on next time init() )
	if (p_ctx_p->gISF_VdoEnc_Opened == TRUE) {
		ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(reset - close)");
		_ISF_VdoEnc_Close(pThisUnit, oPort, FALSE);  // FALSE means DO NOT call do_release_i()
		_ISF_VdoEnc_Unlock_PullQueue(oPort);  // let pull_out(blocking mode) auto-unlock
		p_ctx_p->gISF_VdoEnc_Opened = FALSE;
	}

	return ISF_OK;
}

static ISF_RV _ISF_VdoEnc_Open(ISF_UNIT *pThisUnit, UINT32 id)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = NULL;

	if (g_ve_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[id];

#if 0 //#ifdef __KERNEL__
	PMP_VDOENC_ENCODER p_video_obj = NULL;
#endif
	if (!pNMI_VdoEnc) {
		if ((pNMI_VdoEnc = NMI_GetUnit("VdoEnc")) == NULL) {
			DBG_ERR("Get VdoEnc unit failed\r\n");
			return ISF_ERR_NULL_OBJECT;
		}
	}

	nvtmpp_vb_add_module(pThisUnit->unit_module);   // because AllocMem() is doing at "first time START", so move this register from AllocMem to here.

#if 0
	pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_CODEC, p_ctx_p->gISF_VStrmBuf.CodecType);
#endif
#if 0 //#ifdef __KERNEL__
	switch (p_ctx_p->gISF_VStrmBuf.CodecType) {
		case MEDIAVIDENC_MJPG:	p_video_obj = MP_MjpgEnc_getVideoObject((MP_VDOENC_ID)id);		break;
		case MEDIAVIDENC_H264:	p_video_obj = MP_H264Enc_getVideoObject((MP_VDOENC_ID)id);		break;
		case MEDIAVIDENC_H265:	p_video_obj = MP_H265Enc_getVideoObject((MP_VDOENC_ID)id);		break;
		default:
			DBG_ERR("[ISF_VDOENC][%d] getVideoObject failed\r\n", id);
			return ISF_ERR_FAILED;
	}

	pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_ENCODER_OBJ, (UINT32)p_video_obj);
#endif

#if 0 // [isf_flow] doesn't allow set port param before Open ..... so move alloc memory to "first time START"
	if (_ISF_VdoEnc_AllocMem(pThisUnit, id) != ISF_OK) {
		DBG_ERR("[ISF_VDOENC][%d] AllocMem failed\r\n", id);
		return ISF_ERR_FAILED;
	}
#endif

	pNMI_VdoEnc->SetParam(id, NMI_VDOENC_REG_CB, (UINT32)_ISF_VdoEnc_RecCB);

	pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_3DNR_CB, (UINT32)_ISF_VdoEnc_3DNR_Internal);  // Let H26x Driver callback kflow_videoenc, then we can callback IQ's function to get 3DNR settings
	pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_ISP_CB,  (UINT32)_ISF_VdoEnc_ISPCB_Internal);  // Let H26x Driver callback kflow_videoenc, then we can callback IQ's function to set isp ratio settings


	if (E_OK != pNMI_VdoEnc->Open(id, 0)) {
		DBG_ERR("Open FAIL ... !!\r\n");
		return ISF_ERR_NULL_OBJECT;
	}

	p_ctx_p->gISF_EncKick = 0; //stop kick

	p_ctx_p->gISF_VdoEnc_Is_AllocMem = FALSE;   // set default as NOT create buffer (because AllocMem is doing at Start() now)

	return ISF_OK;
}

static ISF_RV _ISF_VdoEnc_Close(ISF_UNIT *pThisUnit, UINT32 id, UINT32 b_do_release_i)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = NULL;

	if (g_ve_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[id];

	pNMI_VdoEnc->Close(id);

	if (p_ctx_p->gISF_VdoEnc_Is_AllocMem == TRUE) {
		// only call FreeMem if it indeed successfully AllocMem before, JIRA : IVOT_N01002_CO-562  ( user may only call open()->close(), never call start()....because AllocMem is doing at start(), so AllocMem never happened !! )
		if (ISF_OK == _ISF_VdoEnc_FreeMem(pThisUnit, id, b_do_release_i)) {
			p_ctx_p->gISF_VdoEnc_Is_AllocMem = FALSE;
		} else {
			DBG_ERR("[ISF_VDOENC][%d] FreeMem failed...!!\r\n", id);
		}
	} else {
		DBG_IND("[ISF_VDOENC][%d] close without FreeMem\r\n", id);
	}

	return ISF_OK;
}

static ISF_RV _ISF_VdoEnc_ImgCap_Open(ISF_UNIT *pThisUnit)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;

	if (!p_ctx_c->bIs_NMI_ImgCap_ON) {
		//DBG_WRN("bIs_NMI_ImgCap_ON = 0\r\n");  // In fact, there's no ImgCap unit for pure linux
		return ISF_OK;
	}

	if (!p_ctx_c->bIs_NMI_ImgCap_Opened) {
		if ((pNMI_ImgCap = NMI_GetUnit("ImgCap")) == NULL) {
			DBG_ERR("Get ImgCap unit failed\r\n");
			return ISF_ERR_NULL_OBJECT;
		}

		if (_ISF_ImgCap_AllocMem(pThisUnit) != ISF_OK) {
			return ISF_ERR_BUFFER_CREATE;
		}

		// Set param
		pNMI_ImgCap->SetParam(0, NMI_IMGCAP_REG_CB, (UINT32)_ISF_VdoEnc_ImgCapCB);

		pNMI_ImgCap->Open(0, 0);

		p_ctx_c->bIs_NMI_ImgCap_Opened = TRUE;
	}

	return ISF_OK;
}

static ISF_RV _ISF_VdoEnc_ImgCap_Close(ISF_UNIT *pThisUnit)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;

	if (!p_ctx_c->bIs_NMI_ImgCap_ON) {
		//DBG_WRN("bIs_NMI_ImgCap_ON = 0\r\n");  // In fact, there's no ImgCap unit for pure linux
		return ISF_OK;
	}

	if (p_ctx_c->bIs_NMI_ImgCap_Opened) {
		pNMI_ImgCap->Close(0);
		pNMI_ImgCap = NULL;

		_ISF_ImgCap_FreeMem(pThisUnit);

		p_ctx_c->bIs_NMI_ImgCap_Opened = FALSE;
	}

	return ISF_OK;
}

#if defined __UITRON || defined __ECOS
#include "HwClock.h"
#endif

static void _ISF_VdoEnc_OutputPort_Enable(UINT32 id)
{
#if (FORCE_DUMP_DATA == ENABLE)
	if(id+ISF_IN_BASE==WATCH_PORT) {
	DBG_DUMP("^CVdoEnc.in[%d]! ----------------------------------------------- Start Begin\r\n", id);
	DBG_DUMP("^CVdoEnc.in[%d]! ----------------------------------------------- Start Begin\r\n", id);
	}
#endif
}

static void _ISF_VdoEnc_OutputPort_Disable(UINT32 id)
{
#if (FORCE_DUMP_DATA == ENABLE)
	if(id+ISF_IN_BASE==WATCH_PORT) {
	DBG_DUMP("^CVdoEnc.in[%d]! ----------------------------------------------- Stop Begin\r\n", id);
	DBG_DUMP("^CVdoEnc.in[%d]! ----------------------------------------------- Stop Begin\r\n", id);
	}
#endif
}

/*
void _ISF_VdoEnc_OutputPort_Start(UINT32 id)
{
#if (FORCE_DUMP_DATA == ENABLE)
	if(id+ISF_IN_BASE==WATCH_PORT) {
	DBG_DUMP("VdoEnc.in[%d]! ----------------------------------------------- Start End\r\n", id);
	}
#endif
}

void _ISF_VdoEnc_OutputPort_Stop(UINT32 id)
{
#if (FORCE_DUMP_DATA == ENABLE)
	if(id+ISF_IN_BASE==WATCH_PORT) {
	DBG_DUMP("^CVdoEnc.in[%d]! ----------------------------------------------- Stop end\r\n", id);
	}
#endif
}
*/

static ISF_RV _ISF_VdoEnc_InputPort_PushImgBufToDest(ISF_PORT *pPort, ISF_DATA *pData, INT32 nWaitMs)
{
	VDO_FRAME *pImgBuf   		        = (VDO_FRAME *)(pData->desc);   //ISF_VIDEO_RAW_BUF *pImgBuf = (ISF_VIDEO_RAW_BUF *)(pData->Desc);
	UINT32             id        		= (pPort->iport - ISF_IN_BASE);
	VDOENC_CONTEXT_PORT   *p_ctx_p      = NULL;
	VDOENC_CONTEXT_COMMON *p_ctx_c      = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	ER ret                              = E_OK;
	unsigned long         flags         = 0;
#if DUMP_RELEASE_TIME
	UINT32             uiReleaseTime	= Perf_GetCurrent();
#endif

	if (g_ve_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[id];

	#ifdef ISF_TS
	pData->ts_venc_in[0] = hwclock_get_longcounter();
	#endif
	(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(id), ISF_IN_PROBE_PUSH, ISF_ENTER); // [IN] PUSH_ENTER

	if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_YUV_INTERVAL) {
		static UINT32 last_time[ISF_VDOENC_IN_NUM] = {0};
		static UINT32 cur_time [ISF_VDOENC_IN_NUM] = {0};
		cur_time[id] = Perf_GetCurrent();
		DBG_DUMP("[ISF_VDOENC][%d] YUV interval = %d (us)\r\n", id, (cur_time[id] - last_time[id]));
		last_time[id] = cur_time[id];
	}

	// If unit is NOT started, just release input YUV
	if (p_ctx_p->gISF_VdoEnc_Started == 0) {
		_ISF_VdoEnc_OnRelease(id, pData, ISF_ERR_NOT_START);
		(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(id), ISF_IN_PROBE_PUSH_DROP, ISF_ERR_NOT_START); // [IN] PUSH_ERR
		return ISF_ERR_NOT_START;
	}

	// do frc
	if (_isf_frc_is_select(&isf_vdoenc, pPort->iport, &(p_ctx_p->gISF_VdoEnc_infrc)) == 0) {
		_ISF_VdoEnc_OnRelease(id, pData, ISF_ERR_FRC_DROP);
		(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, pPort->iport, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_FRC_DROP);
		return ISF_OK; //drop frame!
	}

	// save fastboot yuv source from which vproc device and path
	{
		ISF_PORT* pSrcPort = isf_vdoenc.p_base->iport(&isf_vdoenc, ISF_IN(id));
		if (pSrcPort->p_srcunit == NULL) { //VPROC not bind with VENC, set default value in fastboot venc dtsi
			if (id == 0) {
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_DEV, 0);
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_PATH, 0);
			} else if (id == 1) {
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_DEV, 0);
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_PATH, 0);
			} else if (id == 2) {
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_DEV, 0);
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_PATH, 1);
			} else if (id == 3) {
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_DEV, 1);
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_PATH, 0);
			} else if (id == 4) {
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_DEV, 1);
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_PATH, 0);
			} else if (id == 5) {
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_DEV, 1);
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_PATH, 1);
			}
		} else { //VPROC bind with VENC
			if (strcmp(pSrcPort->p_srcunit->unit_name, "vdoprc0") == 0) {
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_DEV, 0);
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_PATH, pSrcPort->oport);
			} if (strcmp(pSrcPort->p_srcunit->unit_name, "vdoprc1") == 0) {
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_DEV, 1);
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_VPRC_SRC_PATH, pSrcPort->oport);
			}
		}
	}

	// check ISF_DATA is from HDAL user or internal unit (kflow_videoproc)
	if ((pImgBuf->addr[0] == 0) && (pImgBuf->phyaddr[0] != 0) && (pImgBuf->loff[0] != 0)) {
		//--- ISF_DATA from HDAL user push_in ---

		// get virtual addr
		pImgBuf->addr[0] = nvtmpp_sys_pa2va(pImgBuf->phyaddr[0]);
		pImgBuf->addr[1] = nvtmpp_sys_pa2va(pImgBuf->phyaddr[1]);

		// skip lineoffset & width check
		pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_SKIP_LOFF_CHECK, 1);

	} else {
		//--- ISF_DATA from previous unit (kflow_videoproc) ---

		// have to do lineoffset & width check
		pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_SKIP_LOFF_CHECK, 0);
	}




	if(pData->desc[0] == MAKEFOURCC('M','E','T','A')) {
/*
		VDO_METADATA *pMetadata = (VDO_METADATA *)(pData->desc);
		RIFF_CHUNK* pChunk = (RIFF_CHUNK*)pMetadata->size;  //RIFF_CHUNK* pChunk = (RIFF_CHUNK*)pMetadata->addr;
		if(pChunk->fourcc == MAKEFOURCC('O','D','R','L')) {
		    MP_VDOENC_SMART_ROI_INFO* pInfo = (MP_VDOENC_SMART_ROI_INFO*)(((UINT8*)pChunk)+(pChunk->size));
			DBGD(pInfo->uiRoiNum);
		    pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_SMART_ROI, (UINT32) pInfo);
		}
#if 0
		else if(pChunk->fourcc == MAKEFOURCC('M','D','R','L')) {
            MP_VDOENC_MD_INFO* pInfo = (MP_VDOENC_MD_INFO*)(((UINT8*)pChunk)+(pChunk->size));
       	    pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_MD, (UINT32) pInfo);
        }
#endif

		if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_YUV) {
			DBG_DUMP("[ISF_VDOENC][%d] Meta, Release blk = (0x%08x, 0x%08x), time = %u us\r\n", id, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
		}
		_ISF_VdoEnc_OnRelease(id, pData, ISF_OK);
*/
		return ISF_OK;
	//#NT#2018/03/27#KCHong -begin
	//#NT# Use ImageUnit_SendEvent() instead of ImageUnit_SetParam() to avoid semaphore dead lock
#if _TODO
	} else if (pData->desc[0] == MAKEFOURCC('E', 'V', 'N', 'T')) {
		if (pData->Event.Event == VDOENC_PARAM_IMGCAP_THUMB) {
			if (pData->Event.Param1 && pData->Event.Param2) {
				p_ctx_p->gISF_ImgCap_Thumb_Job.path_id = id;
				p_ctx_p->gISF_ImgCap_Thumb_Job.mode    = NMI_IMGCAP_JPG_ONLY;
				p_ctx_p->gISF_ImgCap_Thumb_Job.jpg_w   = pData->Event.Param1;
				p_ctx_p->gISF_ImgCap_Thumb_Job.jpg_h   = pData->Event.Param2;
				p_ctx_p->gISF_ImgCap_Thumb_Job.thumb   = TRUE;

				pNMI_ImgCap->SetParam(0, NMI_IMGCAP_SET_OBJ, (UINT32) &p_ctx_p->gISF_ImgCap_Thumb_Job);
			}
		}
		return ISF_OK;
	//#NT#2018/03/27#KCHong -end
#endif
    #if _TODO
	#if (MANUAL_SYNC_BASE == ENABLE)
	} else if(pData->desc[0] == SYNC_TAG) {

		UINT64 last_end = (((UINT64)(pData->Event.Param1))<<32) | (pData->Event.Param2);
		UINT64 last_cycle = (UINT64)(pData->Event.Param3);
		p_ctx_p->gISF_VdoEnc_ts_ipl_end = last_end;
		p_ctx_p->gISF_VdoEnc_ts_ipl_cycle = last_cycle;
		#if (DUMP_KICK == ENABLE)
		if(id+ISF_IN_BASE==WATCH_PORT) {
			DBG_DUMP("^YVdoEnc.in[%d]! ipl_end=%3d.%06d ipl_cycle=%3d.%06d\r\n",
				id,
				(UINT32)(last_end>>32), (UINT32)(last_end),
				(UINT32)(last_cycle>>32), (UINT32)(last_cycle));
		}
		#endif
		if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_YUV) {
			DBG_DUMP("[ISF_VDOENC][%d] Sync, Release blk = (0x%08x, 0x%08x), time = %u us\r\n", id, pData->hData, nvtmpp_vb_block2addr(pData->hData), Perf_GetCurrent());
		}
		(&isf_vdoenc)->p_base->do_release(&isf_vdoenc, ISF_IN(id), pData, 0);
		(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(id), ISF_IN_PROBE_PUSH_DROP, ISF_OK);  // [IN] PUSH_DROP
	#endif
    #endif // _TODO
	} else if ((pImgBuf->addr[0] != 0) && (pImgBuf->loff[0] != 0)) {
		MP_VDOENC_YUV_SRC readyInfo = {0};
		UINT32                 i = 0;
		#if (MANUAL_SYNC_BASE == ENABLE)

		p_ctx_p->gISF_VdoEnc_ts_ipl_end = pImgBuf->timestamp; // pData->TimeStamp;
		p_ctx_p->gISF_VdoEnc_ts_enc_begin = HwClock_GetLongCounter();
		#endif
#if (FORCE_DUMP_DATA == ENABLE)
		if(id+ISF_IN_BASE==WATCH_PORT) {
			DBG_DUMP("^MVdoEnc.in[%d]! Push -- Vdo: blk_id=%08x addr=%08x\r\n", id, pData->hData, nvtmpp_vb_block2addr(pData->hData));
		}
#endif
#if (RECORD_DATA_TS == ENABLE)
		pData->TimeStamp_e = 0;
		pData->TimeStamp_s = 0;
#endif

		readyInfo.uiInputTime    = Perf_GetCurrent();
		readyInfo.iInputTimeout  = nWaitMs;  // user provided timeout

		if(!(p_ctx_p->gISF_VdoFunc & VDOENC_FUNC_NOWAIT)) { // WAIT case
TRY_FIND_RAW_USED:
			// search for RAW_ISF_DATA queue empty
			for (i = 0; i < p_ctx_p->gISF_VdoEnc_Data_Que_Num; i++) {
				NMR_VdoEnc_Lock_cpu(&flags);
				if (!p_ctx_p->gISF_VdoEnc_IsRawUsed[i]) {
					p_ctx_p->gISF_VdoEnc_IsRawUsed[i] = TRUE;
					NMR_VdoEnc_Unlock_cpu(&flags);
					memcpy(&p_ctx_p->gISF_VdoEnc_RawDATA[i], pData, sizeof(ISF_DATA));
					cpu_watchData(id, &(p_ctx_p->gISF_VdoEnc_RawDATA[i]));
					break;
				} else {
					NMR_VdoEnc_Unlock_cpu(&flags);
				}
			}

			if (i == p_ctx_p->gISF_VdoEnc_Data_Que_Num) {
				// can't find free p_ctx_p->gISF_VdoEnc_IsRawUsed[i], try again by timeout(nWaitMs) setting
				if (TRUE == _ISF_VdoEnc_YUV_TMOUT_RetryCheck(id, &readyInfo, i))
					goto TRY_FIND_RAW_USED;  // try again
			}

#if (RECORD_DATA_TS == ENABLE)
			(&p_ctx_p->gISF_VdoEnc_RawDATA[i])->TimeStamp_e = 0;
			(&p_ctx_p->gISF_VdoEnc_RawDATA[i])->TimeStamp_s = 0;
#endif

			if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_YUV) {
				DBG_DUMP("[ISF_VDOENC][%d][%d] Input blk = (0x%08x, 0x%08x), time = %u us\r\n", id, i, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
			}

			if (i == p_ctx_p->gISF_VdoEnc_Data_Que_Num) {
#if (FORCE_DUMP_DATA == ENABLE)
				if(id+ISF_IN_BASE==WATCH_PORT) {
					DBG_DUMP("^MVdoEnc.in[%d]! Queue full -- Vdo: blk_id=%08x addr=%08x\r\n", id, pData->h_data, nvtmpp_vb_block2addr(pData->h_data));
				}
#endif
			#ifdef ISF_TS
			#ifdef ISF_DUMP_TS
			DBG_DUMP("*** dt-vprc-push-venc-end [DROP!]\r\n");
			#endif
			#endif
				//queue is full, not allow push
				if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_DROP) {
					p_ctx_p->gISF_VdoEncDropCount++;
					if (p_ctx_p->gISF_VdoEncDropCount % 30 == 0) {
						p_ctx_p->gISF_VdoEncDropCount = 0;
						DBG_DUMP("[ISF_VDOENC][%d][%d] Input Queue Full, Release blk = (0x%08x, 0x%08x), time = %u us\r\n", id, i, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
					}
				}

				if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_YUV) {
					DBG_DUMP("[ISF_VDOENC][%d][%d] Full, Release blk = (0x%08x, 0x%08x), time = %u us\r\n", id, i, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
				}

				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_DROP_FRAME, 1);
				_ISF_VdoEnc_OnRelease(id, pData, ISF_ERR_QUEUE_FULL);
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(id), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [IN] PUSH_WRN
				//DBG_ERR("^YVdoEnc Drop! -- VdoEnc: %d %08x %08x %d %d\r\n", pImgBuf->PxlFmt, pImgBuf->PxlAddr[0], pImgBuf->PxlAddr[1], pImgBuf->LineOffset[0], pImgBuf->LineOffset[1]);
				return ISF_ERR_QUEUE_FULL;
			}

		}

#if (SUPPORT_PULL_FUNCTION)
		if (TRUE == _ISF_VdoEnc_isFull_PullQueue(id)) {
			//pull queue is full, not allow push
			if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_DROP) {
				p_ctx_p->gISF_VdoEncDropCount++;
				if (p_ctx_p->gISF_VdoEncDropCount % 30 == 0) {
					p_ctx_p->gISF_VdoEncDropCount = 0;
					DBG_DUMP("[ISF_VDOENC][%d][%d] Pull Queue Full, Release blk = (0x%08x, 0x%08x), time = %u us\r\n", id, i, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
				}
			}

			pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_DROP_FRAME, 1);
			_ISF_VdoEnc_OnRelease(id, pData, ISF_ERR_QUEUE_FULL);
			NMR_VdoEnc_Lock_cpu(&flags);
			p_ctx_p->gISF_VdoEnc_IsRawUsed[i] = FALSE;
			NMR_VdoEnc_Unlock_cpu(&flags);
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(id), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_QUEUE_FULL); // [IN] PUSH_WRN
			return ISF_ERR_QUEUE_FULL;
		}
#endif

		//----- if JPEG need rotate, use gximg to rotate the YUV (because JPEG HW doesn't support rotate) -----
		if (p_ctx_p->gISF_VStrmBuf.CodecType == MEDIAVIDENC_MJPG) {
			UINT32 rotate;
			pNMI_VdoEnc->GetParam(id, NMI_VDOENC_PARAM_ROTATE, &rotate);

			// call OSG to simulate STAMP_EX effect, no matter rotate or NOT-rotate
			{
				// wrap original input YUV to call mask/osd
				readyInfo.uiWidth 			= pImgBuf->size.w;
				readyInfo.uiHeight 			= pImgBuf->size.h;
				readyInfo.uiYAddr 			= pImgBuf->addr[0];
				readyInfo.uiCbAddr 			= pImgBuf->addr[1];
				readyInfo.uiCrAddr 			= pImgBuf->addr[1];
				readyInfo.uiYLineOffset 	= pImgBuf->loff[0];
				readyInfo.uiUVLineOffset 	= pImgBuf->loff[1];
				readyInfo.uiBufID 			= i;
				readyInfo.TimeStamp 		= pImgBuf->timestamp;   //pData->TimeStamp;
#if defined(_BSP_NA51000_)
				readyInfo.uiRawCount        = pImgBuf->reserved[7]; //pImgBuf->count;   // count = frame num ,  reserved[7] = SIE frame num ( if drop frame,  count < reserved[7] )
#elif defined(_BSP_NA51055_)
				readyInfo.uiRawCount        = pImgBuf->count; // NA51000(count = ipp frame count, reserved[7] = sie frame num) , NA51055(count = sie frame num)
#endif
				 _isf_vdoenc_do_input_mask(&isf_vdoenc, ISF_IN(id), (void *) &readyInfo, NULL, TRUE);
				_isf_vdoenc_finish_input_mask(&isf_vdoenc, ISF_IN(id));
				 _isf_vdoenc_do_input_osd( &isf_vdoenc, ISF_IN(id), (void *) &readyInfo, NULL, TRUE);
				_isf_vdoenc_finish_input_osd(&isf_vdoenc, ISF_IN(id));
			}
#if defined(_BSP_NA51000_)
			if ((rotate == MP_VDOENC_ROTATE_CW) || (rotate == MP_VDOENC_ROTATE_CCW)) {
#elif defined(_BSP_NA51055_)
			if ((rotate == MP_VDOENC_ROTATE_CW) || (rotate == MP_VDOENC_ROTATE_180) || (rotate == MP_VDOENC_ROTATE_CCW)) {
#endif
				ISF_DATA *p_data_rotate = &p_ctx_p->gISF_VdoEnc_RawDATA[i];
				UINT32 buf_size = gximg_calc_require_size(ALIGN_CEIL_16(pImgBuf->size.w), ALIGN_CEIL_16(pImgBuf->size.h), pImgBuf->pxlfmt, ALIGN_CEIL_16(pImgBuf->size.w));
				UINT32 buf = 0;

				// [1] try to call do_new() for new YUV common buffer
				{
					//skip init p_data_rotate, becase p_data_rotate has been copied from push p_data
					//   [Hack] just use OUT port to do_new, because ddr_id is set at output port.  It's OK, because for vdoenc port IN=OUT
					if (buf_size > 0) {
						buf = (&isf_vdoenc)->p_base->do_new(&isf_vdoenc, ISF_OUT(id), NVTMPP_VB_INVALID_POOL, 0, buf_size, p_data_rotate, ISF_IN_PROBE_EXT);
						ISF_UNIT_TRACE(ISF_IN_PROBE_EXT, (&isf_vdoenc),ISF_IN(id), "[ISF_VDOENC][%d][%d] JPG roate, alloc buf_size =  %u, new buf = 0x%08x , blk = 0x%08x", id, i, buf_size, buf, p_data_rotate->h_data);
					}

					// if do_new() fail, release input YUV & unlock IsRawUsed
					if (buf == 0) {
						_ISF_VdoEnc_OnRelease(id, pData, ISF_ERR_BUFFER_CREATE);
						(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(id), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_BUFFER_CREATE); // [IN] PUSH_WRN

						if(!(p_ctx_p->gISF_VdoFunc & VDOENC_FUNC_NOWAIT)) { // WAIT case
							NMR_VdoEnc_Lock_cpu(&flags);
							p_ctx_p->gISF_VdoEnc_IsRawUsed[i] = FALSE;
							NMR_VdoEnc_Unlock_cpu(&flags);

							_ISF_VdoEnc_YUV_TMOUT_ResourceUnlock(id);
						}

						ISF_UNIT_TRACE(ISF_IN_PROBE_EXT, (&isf_vdoenc),ISF_IN(id), "[ISF_VDOENC][%d][%d] Alloc dst rotate buffer fail, Release blk = (0x%08x, 0x%08x), time = %u us\r\n", id, i, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
						return ISF_ERR_BUFFER_CREATE;
					}
				}


				// [2] call gximage to rotate
				{
					VDO_FRAME *pImgRotate = (VDO_FRAME *)&p_data_rotate->desc[0];
					UINT32 grap_dir =0;
					ER gximg_r = 0;
					ISIZE dst_scale = {0};

					if (rotate == MP_VDOENC_ROTATE_CW) {
						grap_dir = VDO_DIR_ROTATE_90;
						dst_scale.w = pImgBuf->size.h;
						dst_scale.h = pImgBuf->size.w;
#if defined(_BSP_NA51055_)
					} else if (rotate == MP_VDOENC_ROTATE_180) {
						grap_dir = VDO_DIR_ROTATE_180;
						dst_scale.w = pImgBuf->size.w;
						dst_scale.h = pImgBuf->size.h;
#endif
					} else if (rotate == MP_VDOENC_ROTATE_CCW) {
						grap_dir = VDO_DIR_ROTATE_270;
						dst_scale.w = pImgBuf->size.h;
						dst_scale.h = pImgBuf->size.w;
					}

					if ((gximg_r = gximg_init_buf_h_align(pImgRotate, dst_scale.w, dst_scale.h, ALIGN_CEIL_16(dst_scale.h), pImgBuf->pxlfmt, GXIMG_LINEOFFSET_ALIGN(16), buf, buf_size)) != E_OK) {
						// init error, release input YUV & dst rotate YUV & unlock IsRawUsed
						_ISF_VdoEnc_OnRelease(id, pData, ISF_ERR_PROCESS_FAIL);
						_ISF_VdoEnc_OnRelease(id, p_data_rotate, ISF_ERR_PROCESS_FAIL);
						(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(id), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_PROCESS_FAIL); // [IN] PUSH_WRN

						if(!(p_ctx_p->gISF_VdoFunc & VDOENC_FUNC_NOWAIT)) { // WAIT case
							NMR_VdoEnc_Lock_cpu(&flags);
							p_ctx_p->gISF_VdoEnc_IsRawUsed[i] = FALSE;
							NMR_VdoEnc_Unlock_cpu(&flags);

							_ISF_VdoEnc_YUV_TMOUT_ResourceUnlock(id);
						}

						ISF_UNIT_TRACE(ISF_IN_PROBE_EXT, (&isf_vdoenc),ISF_IN(id), "[ISF_VDOENC][%d][%d] init YUV fail(ret = %u), Release blk = (0x%08x, 0x%08x), time = %u us\r\n", id, i, gximg_r, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
						return ISF_ERR_PROCESS_FAIL;
					}

					if ((gximg_r = gximg_rotate_paste_data_no_flush(pImgBuf, NULL, pImgRotate, NULL, grap_dir, GXIMG_ROTATE_ENG1, 0)) == E_OK) {
						// rotate OK, set HALN info if need
						if (pImgBuf->reserved[0] == MAKEFOURCC('H', 'A', 'L', 'N')) {
							pImgRotate->reserved[0] = MAKEFOURCC('H', 'A', 'L', 'N');
							pImgRotate->reserved[1] = ALIGN_CEIL_16(pImgRotate->size.h);
						}
						// rotate OK, set rotate YUV to pImgBuf
						pImgBuf = pImgRotate;
						ISF_UNIT_TRACE(ISF_IN_PROBE_EXT, (&isf_vdoenc),ISF_IN(id), "[ISF_VDOENC][%d][%d] YUV rotate OK, new YUV (w,h)=(%d,%d), loff=(%d,%d)\r\n", id, i, pImgBuf->size.w, pImgBuf->size.h, pImgBuf->loff[0], pImgBuf->loff[1]);

						// release input YUV
						_ISF_VdoEnc_OnRelease(id, pData, ISF_OK);
					} else {
						// rotate error, release input YUV & dst rotate YUV & unlock IsRawUsed
						_ISF_VdoEnc_OnRelease(id, pData, ISF_ERR_PROCESS_FAIL);
						_ISF_VdoEnc_OnRelease(id, p_data_rotate, ISF_ERR_PROCESS_FAIL);
						(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(id), ISF_IN_PROBE_PUSH_WRN, ISF_ERR_PROCESS_FAIL); // [IN] PUSH_WRN

						if(!(p_ctx_p->gISF_VdoFunc & VDOENC_FUNC_NOWAIT)) { // WAIT case
							NMR_VdoEnc_Lock_cpu(&flags);
							p_ctx_p->gISF_VdoEnc_IsRawUsed[i] = FALSE;
							NMR_VdoEnc_Unlock_cpu(&flags);

							_ISF_VdoEnc_YUV_TMOUT_ResourceUnlock(id);
						}

						ISF_UNIT_TRACE(ISF_IN_PROBE_EXT, (&isf_vdoenc),ISF_IN(id), "[ISF_VDOENC][%d][%d] rotate YUV fail(ret = %u), Release blk = (0x%08x, 0x%08x), time = %u us\r\n", id, i, gximg_r, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
						return ISF_ERR_PROCESS_FAIL;
					}
				}
			} // if ((rotate == MP_VDOENC_ROTATE_CW) || (rotate == MP_VDOENC_ROTATE_CCW))
		} // if (p_ctx_p->gISF_VStrmBuf.CodecType == MEDIAVIDENC_MJPG)

		readyInfo.uiWidth 			= pImgBuf->size.w;
		readyInfo.uiHeight 			= pImgBuf->size.h;
		readyInfo.uiYAddr 			= pImgBuf->addr[0];
		readyInfo.uiCbAddr 			= pImgBuf->addr[1];
		readyInfo.uiCrAddr 			= pImgBuf->addr[1];
		readyInfo.uiYLineOffset 	= pImgBuf->loff[0];
		readyInfo.uiUVLineOffset 	= pImgBuf->loff[1];
		readyInfo.uiBufID 			= i;
		readyInfo.TimeStamp 		= pImgBuf->timestamp;   //pData->TimeStamp;
#if defined(_BSP_NA51000_)
		readyInfo.uiRawCount        = pImgBuf->reserved[7]; //pImgBuf->count;   // count = frame num ,  reserved[7] = SIE frame num ( if drop frame,  count < reserved[7] )
#elif defined(_BSP_NA51055_)
		readyInfo.uiRawCount        = pImgBuf->count; // NA51000(count = ipp frame count, reserved[7] = sie frame num) , NA51055(count = sie frame num)
#endif
		//DBG_DUMP("count = %llu, res[7] = %u\r\n", pImgBuf->count, pImgBuf->reserved[7]);

		if (pImgBuf->p_next != NULL) {
			VDO_MD_INFO *p_md;
			p_md = (VDO_MD_INFO *)((UINT32)pImgBuf->p_next + sizeof(VDO_METADATA));
			readyInfo.MDInfo.md_width = p_md->md_size.w;
			readyInfo.MDInfo.md_height = p_md->md_size.h;
			readyInfo.MDInfo.md_lofs = p_md->md_lofs;
			readyInfo.MDInfo.md_buf_adr = ((UINT32)pImgBuf->p_next + ALIGN_CEIL((sizeof(VDO_METADATA) + sizeof(VDO_MD_INFO)), VOS_ALIGN_BYTES));
			readyInfo.MDInfo.rotation = p_md->reserved[0];
			readyInfo.MDInfo.roi_xy = p_md->reserved[1];
			readyInfo.MDInfo.roi_wh = p_md->reserved[2];
			if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_MDINFO) {
				DBG_DUMP("[ISF_VDOENC][%d] MDInfo = w(%u) h(%u) lofs(%u), buf_adr = 0x%x, rotation = 0x%x, roi_xy = 0x%x, roi_wh = 0x%x\r\n", id, readyInfo.MDInfo.md_width, readyInfo.MDInfo.md_height, readyInfo.MDInfo.md_lofs, readyInfo.MDInfo.md_buf_adr, readyInfo.MDInfo.rotation, readyInfo.MDInfo.roi_xy, readyInfo.MDInfo.roi_wh);
			}
		} else {
			readyInfo.MDInfo.md_width = 0;
			readyInfo.MDInfo.md_height = 0;
			readyInfo.MDInfo.md_lofs = 0;
			readyInfo.MDInfo.md_buf_adr = 0;
			readyInfo.MDInfo.rotation = 0;
			readyInfo.MDInfo.roi_xy = 0;
			readyInfo.MDInfo.roi_wh = 0;
			if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_MDINFO) {
				DBG_DUMP("[ISF_VDOENC][%d] p_next is NULL, MDInfo = w(0) h(0) lofs(0), buf_adr = 0x0, rotation = 0x0, roi_xy = 0x0, roi_wh = 0x0\r\n", id);
			}
		}

		//----- if JPEG with height align func ON (JIRA:IVOT_N00025-581 / IVOT_N01004_CO-287) -----
		if (p_ctx_p->gISF_VStrmBuf.CodecType == MEDIAVIDENC_MJPG) {

			// if (width != loff) need set 0, too
			if (readyInfo.uiWidth < readyInfo.uiYLineOffset) {
				if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_HALIGN) {
					DBG_DUMP("[ISF_VDOENC][%d] JPEG with width < loff, w(%u) Yloff(%u) UVloff(%u) YAddr(0x%08x) UVAddr(0x%08x) \r\n", id, readyInfo.uiWidth, readyInfo.uiYLineOffset, readyInfo.uiUVLineOffset, (UINT)readyInfo.uiYAddr, (UINT)readyInfo.uiCbAddr);
				}

				do {
					VDO_FRAME frame = {0};
					IRECT rect = {0};

					ret = gximg_init_buf_ex(&frame, pImgBuf->loff[0], pImgBuf->size.h, pImgBuf->pxlfmt, pImgBuf->loff, pImgBuf->addr); // NOTE: should input width, but input loff to cheat gximg, because we want to draw on (width ~ loff region)
					if(ret != E_OK){
						DBG_ERR("[ISF_VDOENC][%d] fail to init gximg buf\n", id);
						break;
					}

					rect.x = readyInfo.uiWidth;
					rect.y = 0;
					rect.w = readyInfo.uiYLineOffset-readyInfo.uiWidth;
					rect.h = readyInfo.uiHeight;

					ret = gximg_fill_data(&frame, &rect, 0x00808000, 0);  // for (width ~ loff region) fill 0x00 to Y, 0x80 to UV
					if(ret != E_OK){
						DBG_ERR("[ISF_VDOENC][%d] fail to fill gximg buf\n", id);
						break;
					}
				} while (0);
			}

			// if (height != y_16x_height) need set 0
			if (pImgBuf->reserved[0] == MAKEFOURCC('H', 'A', 'L', 'N')) {
				UINT32 y_16x_height = pImgBuf->reserved[1];
				UINT32 addr=0,size=0,imgfmt=0;
				static UINT32 sCountHaln = 0;

				pNMI_VdoEnc->GetParam(id, NMI_VDOENC_PARAM_IMGFMT, &imgfmt);

				if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_HALIGN) {
					DBG_DUMP("[ISF_VDOENC][%d] JPEG with height align, w(%u) h(%u) h_align(%u) imgfmt(0x%08x) YAddr(0x%08x) UVAddr(0x%08x) Yloff(%u) UVloff(%u)\r\n", id, readyInfo.uiWidth, readyInfo.uiHeight, y_16x_height, (UINT)imgfmt, (UINT)readyInfo.uiYAddr, (UINT)readyInfo.uiCbAddr, readyInfo.uiYLineOffset, readyInfo.uiUVLineOffset);
				}
				do {
					// error check
					if (y_16x_height < readyInfo.uiHeight) {
						if (sCountHaln++ %30 == 0) {
							DBG_ERR("[ISF_VDOENC][%d] Error! JPEG with height align ON, but alignH(%u) < H(%u)\r\n", id, y_16x_height, readyInfo.uiHeight);
						}
						break; // skip padding
					}

					if (y_16x_height % 16 != 0) {
						if (sCountHaln++ %30 == 0) {
							DBG_ERR("[ISF_VDOENC][%d] Error! JPEG with height align ON, but alignH(%u) is NOT 16x\r\n", id, y_16x_height);
						}
						break; // skip padding
					}

					// padding Y
					size = readyInfo.uiYLineOffset * (y_16x_height - readyInfo.uiHeight);
					if (size > 0) {
						addr = readyInfo.uiYAddr + readyInfo.uiYLineOffset*readyInfo.uiHeight;

						// error check for Y - UV addr
						if ((addr + size) > readyInfo.uiCbAddr) {
							if (sCountHaln++ %30 == 0) {
								DBG_ERR("[ISF_VDOENC][%d] Error! JPEG with height align ON, but YAddr_end(0x%08x) + padding_size(%u) > UVAddr(0x%08x)\r\n", id, (UINT)addr, size, (UINT)readyInfo.uiCbAddr);
							}
							break; // skip padding
						}

						memset((void*)addr, 0, size);
						vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE); //flush cache
					}

					// padding UV
					size = readyInfo.uiUVLineOffset * isf_vdoenc_util_y2uvheight(imgfmt, y_16x_height - readyInfo.uiHeight);
					if (size > 0) {
						addr = readyInfo.uiCbAddr + readyInfo.uiUVLineOffset*isf_vdoenc_util_y2uvheight(imgfmt, readyInfo.uiHeight);
						memset((void*)addr, 0x80, size);
						vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE); //flush cache
					}
				} while (0);
			} else {
				if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_HALIGN) {
					DBG_DUMP("[ISF_VDOENC][%d] JPEG w/o height align.\r\n", id);
				}
			}
		}

		//--- Low Latency ---
		if((p_ctx_p->gISF_VdoFunc & VDOENC_FUNC_LOWLATENCY)) { // this path is config as low-latency path
			// Low latency ON
			VDO_LOW_DLY_INFO *p_lowdly = (VDO_LOW_DLY_INFO *)&pImgBuf->reserved[0];
			readyInfo.LowLatencyInfo.enable       = 1;
			readyInfo.LowLatencyInfo.strp_num     = p_lowdly->strp_num;
			readyInfo.LowLatencyInfo.strp_size[0] = p_lowdly->strp_size[0];
			readyInfo.LowLatencyInfo.strp_size[1] = p_lowdly->strp_size[1];
			readyInfo.LowLatencyInfo.strp_size[2] = p_lowdly->strp_size[2];

			if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_LOWLATENCY) {
				DBG_DUMP("[ISF_VDOENC][%d] LowLatency = en(%u) strp_num(%u) strp_size(%u, %u, %u), time = %u us\r\n", id, readyInfo.LowLatencyInfo.enable, readyInfo.LowLatencyInfo.strp_num, readyInfo.LowLatencyInfo.strp_size[0], readyInfo.LowLatencyInfo.strp_size[1], readyInfo.LowLatencyInfo.strp_size[2], Perf_GetCurrent());
			}

			// clear all H26X JobQ & Relase YUV & stop H26x encoding task
			pNMI_VdoEnc->SetParam(0, NMI_VDOENC_PARAM_CANCEL_H26X_TASK, 0);

		} else {
			// Low latency OFF
			memset(&readyInfo.LowLatencyInfo, 0, sizeof(MP_VDOENC_LOWLATENCY));

			if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_LOWLATENCY) {
				DBG_DUMP("[ISF_VDOENC][%d] LowLatency = en(0), time = %u us\r\n", id, Perf_GetCurrent());
			}
		}

		//--- Source Compression ---
#if defined(_BSP_NA51000_)
		if (pImgBuf->pxlfmt == VDO_PXLFMT_YUV420_NVX1_H264 || pImgBuf->pxlfmt == VDO_PXLFMT_YUV420_NVX1_H265) {
			// Source Compression ON
			VDO_NVX1_INFO  *pYUVHeader = {0};

			readyInfo.uiSrcCompression = TRUE;

			pYUVHeader = (VDO_NVX1_INFO *)(pImgBuf->reserved);

			readyInfo.uiYLineOffset = pImgBuf->loff[0] / 4;

			readyInfo.SrcCompressInfo.uiVirSrcDecSideInfoAddr = pYUVHeader->sideinfo_addr;
			readyInfo.SrcCompressInfo.uiSrcDecktable0         = pYUVHeader->ktab0 + ((pYUVHeader->ktab1 & 0x7FFFF) << 13);
			readyInfo.SrcCompressInfo.uiSrcDecktable1         = ((pYUVHeader->ktab1 & 0xFFF80000) >> 19) + ((pYUVHeader->ktab2 & 0x7FFFF) << 13);
			readyInfo.SrcCompressInfo.uiSrcDecktable2         = (pYUVHeader->ktab2 & 0x7FF80000) >> 19;
			readyInfo.SrcCompressInfo.uiSrcDecStripNum        = pYUVHeader->strp_num - 1;
			readyInfo.SrcCompressInfo.uiSrcDecStrip01Size     = pYUVHeader->strp_size_01;
			readyInfo.SrcCompressInfo.uiSrcDecStrip23Size     = pYUVHeader->strp_size_23;
#elif defined(_BSP_NA51055_)
		if (pImgBuf->pxlfmt == VDO_PXLFMT_YUV420_NVX2) {
			// Source Compression ON
			readyInfo.uiSrcCompression = TRUE;

			readyInfo.uiYLineOffset = pImgBuf->loff[0] * 4/3;

			readyInfo.SrcCompressInfo.enable  = TRUE;
			readyInfo.SrcCompressInfo.width   = pImgBuf->size.w;; // TODO : are those setting correct ??
			readyInfo.SrcCompressInfo.height  = pImgBuf->size.h;
			readyInfo.SrcCompressInfo.y_lofst = pImgBuf->loff[0];
			readyInfo.SrcCompressInfo.c_lofst = pImgBuf->loff[1];
#endif
			ret = pNMI_VdoEnc->In(id, (UINT32 *)&readyInfo);
			if (ret == E_SYS) {
#if (FORCE_DUMP_DATA == ENABLE)
				if(id+ISF_IN_BASE==WATCH_PORT) {
					DBG_DUMP("^MVdoEnc.in[%d]! input full -- Vdo: blk_id=%08x addr=%08x\r\n", id, pData->h_data, nvtmpp_vb_block2addr(pData->h_data));
				}
#endif
				if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_YUV) {
					DBG_DUMP("[ISF_VDOENC][%d][%d] Compress Stop, Release blk = (0x%08x, 0x%08x), time = %u us\r\n", id, i, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
				}

				if(!(p_ctx_p->gISF_VdoFunc & VDOENC_FUNC_NOWAIT)) { // WAIT case
					_ISF_VdoEnc_OnRelease(id, &p_ctx_p->gISF_VdoEnc_RawDATA[i], ISF_ERR_PROCESS_FAIL);  // JIRA : IVOT_CHIP-2078 , modify here form "pData" -> "&p_ctx_p->gISF_VdoEnc_RawDATA[i]" , because for JPEG rotate case, "pData" had already released before, we actually want to release new alloc and rotated YUV here.
					NMR_VdoEnc_Lock_cpu(&flags);
					p_ctx_p->gISF_VdoEnc_IsRawUsed[i] = FALSE;
					NMR_VdoEnc_Unlock_cpu(&flags);

					_ISF_VdoEnc_YUV_TMOUT_ResourceUnlock(id);
				} else {
					_ISF_VdoEnc_OnRelease(id, pData, ISF_ERR_PROCESS_FAIL);
				}
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(id), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_PROCESS_FAIL); // [IN] PUSH_ERR
				return ISF_ERR_PROCESS_FAIL;
			} else if (ret == E_RLWAI) {
				_ISF_VdoEnc_OnRelease(id, pData, ISF_ERR_FRC_DROP);
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, pPort->iport, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_FRC_DROP);

				if(!(p_ctx_p->gISF_VdoFunc & VDOENC_FUNC_NOWAIT)) { // WAIT case
					NMR_VdoEnc_Lock_cpu(&flags);
					p_ctx_p->gISF_VdoEnc_IsRawUsed[i] = FALSE;
					NMR_VdoEnc_Unlock_cpu(&flags);

					_ISF_VdoEnc_YUV_TMOUT_ResourceUnlock(id);
				}
				return ISF_OK; //drop frame!
			}

#if (RECORD_DATA_TS == ENABLE)
			if(!(p_ctx_p->gISF_VdoFunc & VDOENC_FUNC_NOWAIT)) { // WAIT case
				(&p_ctx_p->gISF_VdoEnc_RawDATA[i])->TimeStamp_e = HwClock_GetLongCounter();
			} else {
				pData->TimeStamp_e = HwClock_GetLongCounter();
			}
#endif
		} else {
			// Source Compression OFF

			ret = pNMI_VdoEnc->In(id, (UINT32 *)&readyInfo);
			if (ret == E_SYS) {
#if (FORCE_DUMP_DATA == ENABLE)
				if(id+ISF_IN_BASE==WATCH_PORT) {
					DBG_DUMP("^MVdoEnc.in[%d]! input full -- Vdo: blk_id=%08x addr=%08x\r\n", id, pData->h_data, nvtmpp_vb_block2addr(pData->h_data));
				}
#endif
				if (gISF_VdoEncMsgLevel[id] & ISF_VDOENC_MSG_YUV) {
					DBG_DUMP("[ISF_VDOENC][%d][%d] Stop, Release blk = (0x%08x, 0x%08x), time = %u us\r\n", id, i, pData->h_data, nvtmpp_vb_block2addr(pData->h_data), Perf_GetCurrent());
				}

				if(!(p_ctx_p->gISF_VdoFunc & VDOENC_FUNC_NOWAIT)) { // WAIT case
					_ISF_VdoEnc_OnRelease(id, &p_ctx_p->gISF_VdoEnc_RawDATA[i], ISF_ERR_PROCESS_FAIL);  // JIRA : IVOT_CHIP-2078 , modify here form "pData" -> "&p_ctx_p->gISF_VdoEnc_RawDATA[i]" , because for JPEG rotate case, "pData" had already released before, we actually want to release new alloc and rotated YUV here.
					NMR_VdoEnc_Lock_cpu(&flags);
					p_ctx_p->gISF_VdoEnc_IsRawUsed[i] = FALSE;
					NMR_VdoEnc_Unlock_cpu(&flags);

					_ISF_VdoEnc_YUV_TMOUT_ResourceUnlock(id);
				} else {
					_ISF_VdoEnc_OnRelease(id, pData, ISF_ERR_PROCESS_FAIL);
				}
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(id), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_PROCESS_FAIL); // [IN] PUSH_ERR
				return ISF_ERR_PROCESS_FAIL;
			} else if (ret == E_RLWAI) {
				_ISF_VdoEnc_OnRelease(id, pData, ISF_ERR_FRC_DROP);
				(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, pPort->iport, ISF_IN_PROBE_PUSH_DROP, ISF_ERR_FRC_DROP);

				if(!(p_ctx_p->gISF_VdoFunc & VDOENC_FUNC_NOWAIT)) { // WAIT case
					NMR_VdoEnc_Lock_cpu(&flags);
					p_ctx_p->gISF_VdoEnc_IsRawUsed[i] = FALSE;
					NMR_VdoEnc_Unlock_cpu(&flags);

					_ISF_VdoEnc_YUV_TMOUT_ResourceUnlock(id);
				}
				return ISF_OK; //drop frame!
			}

#if (RECORD_DATA_TS == ENABLE)
			if(!(p_ctx_p->gISF_VdoFunc & VDOENC_FUNC_NOWAIT)) { // WAIT case
				(&p_ctx_p->gISF_VdoEnc_RawDATA[i])->TimeStamp_e = HwClock_GetLongCounter();
			} else {
				pData->TimeStamp_e = HwClock_GetLongCounter();
			}
#endif

			if (p_ctx_c->bIs_NMI_ImgCap_Opened) {
				NMI_IMGCAP_YUV ImgCapYuv = {0};

				ImgCapYuv.path_id = id;
				ImgCapYuv.y_addr  = pImgBuf->addr[0];
				ImgCapYuv.y_loff  = pImgBuf->loff[0];
				ImgCapYuv.u_addr  = pImgBuf->addr[1];
				ImgCapYuv.u_loff  = pImgBuf->loff[1];
				ImgCapYuv.v_addr  = pImgBuf->addr[2];
				ImgCapYuv.v_loff  = pImgBuf->loff[2];
				ImgCapYuv.fmt     = pImgBuf->pxlfmt;
				ImgCapYuv.img_w   = pImgBuf->size.w;
				ImgCapYuv.img_h   = pImgBuf->size.h;

				if (ImgCapYuv.img_w == 2880 && ImgCapYuv.img_h == 2160) {
					ImgCapYuv.dar = 1;
				} else {
					ImgCapYuv.dar = 0;
				}

				pNMI_ImgCap->In(id, (UINT32 *)&ImgCapYuv);
			}
		}

		if((p_ctx_p->gISF_VdoFunc & VDOENC_FUNC_NOWAIT)) { // NO WAIT case
#if (FORCE_DUMP_DATA == ENABLE)
				if(id+ISF_IN_BASE==WATCH_PORT) {
					DBG_DUMP("^MVdoEnc.in[%d]! NO WAIT! -- Vdo: blk_id=%08x addr=%08x\r\n", id, pData->h_data, nvtmpp_vb_block2addr(pData->h_data));
				}
#endif
			_ISF_VdoEnc_OnRelease(id, pData, ISF_OK);
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(id), ISF_IN_PROBE_PUSH, ISF_OK); // [IN] PUSH_OK

#if DUMP_RELEASE_TIME
			DBG_DUMP("[ISF_VDOENC][%d] Release time = %d us\r\n", id, Perf_GetCurrent() - uiReleaseTime);
#endif
		}
	} else {
		DBG_ERR("[ISF_VDOENC][%d] Data error, desc = 0x%08x, addr = 0x%08x, lineoffset = 0x%08x\r\n", id, pData->desc[0], pImgBuf->addr[0], pImgBuf->loff[0]);
		_ISF_VdoEnc_OnRelease(id, pData, ISF_ERR_INVALID_DATA);
		(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_IN(id), ISF_IN_PROBE_PUSH_ERR, ISF_ERR_INVALID_DATA); // [IN] PUSH_ERR
		return ISF_ERR_INVALID_DATA;
	}
	return ISF_OK;
}

#if (SUPPORT_PULL_FUNCTION)
static ISF_RV _ISF_VdoEnc_OutputPort_WrapPartialFrame(UINT32 oport, ISF_DATA *p_data)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[oport];
	ISF_DATA             *pBsData     = &gISF_VdoEncBsDataPartial[oport];
	PVDO_BITSTREAM        pVStrmBuf   = &p_ctx_p->gISF_VStrmBuf;
	UINT32 partial_len = 0, IsIFrame = 0, bs_now = 0, desc_len = 0;

	if (p_ctx_p->gISF_VStrmBuf.CodecType == MEDIAVIDENC_H265) {
		(MP_H265Enc_getVideoObject((MP_VDOENC_ID)oport)->GetInfo)(VDOENC_GET_BS_LEN, (UINT32 *)&partial_len, 0, 0);
		(MP_H265Enc_getVideoObject((MP_VDOENC_ID)oport)->GetInfo)(VDOENC_GET_ISIFRAME, (UINT32 *)&IsIFrame, 0, 0);
	} else 	if (p_ctx_p->gISF_VStrmBuf.CodecType == MEDIAVIDENC_H264) {
		(MP_H264Enc_getVideoObject((MP_VDOENC_ID)oport)->GetInfo)(VDOENC_GET_BS_LEN, (UINT32 *)&partial_len, 0, 0);
		(MP_H264Enc_getVideoObject((MP_VDOENC_ID)oport)->GetInfo)(VDOENC_GET_ISIFRAME, (UINT32 *)&IsIFrame, 0, 0);
	}

	pNMI_VdoEnc->GetParam(oport, NMI_VDOENC_PARAM_PARTIAL_BS_ADDR, &bs_now);
	pNMI_VdoEnc->GetParam(oport, NMI_VDOENC_PARAM_DESC_LEN, &desc_len);

	DBG_IND("[ISF_VDOENC][%d] partial len = %u, desc_len = %u, IsKey = %u\n", oport, partial_len, desc_len, IsIFrame);

	// if too few data available, treat as fail
	if (IsIFrame) {
		if (partial_len < desc_len) return ISF_ERR_QUEUE_EMPTY;  // Desc(vps/sps/pps) is not done yet
	}else{
		if (partial_len == 0) return ISF_ERR_QUEUE_EMPTY;
	}

	// wrap partial ISF_DATA
	pVStrmBuf->sign      = MAKEFOURCC('V', 'S', 'T', 'M');
	pVStrmBuf->DataAddr  = (IsIFrame)? (bs_now + desc_len):(bs_now); //pMem->Addr;
	pVStrmBuf->DataSize  = (IsIFrame)? (partial_len - desc_len):(partial_len);  //pMem->Size;
	pVStrmBuf->resv[6]   = (IsIFrame)? MP_VDOENC_IDR_SLICE:MP_VDOENC_P_SLICE; //pMem->FrameType;  // this should be wrong, because it's maybe kp-frame. But this can only be correctly query from driver after completely encoded. User should not reference this value !!
	pVStrmBuf->resv[7]   = (UINT32)(-1); //pMem->IsIDR2Cut;
	pVStrmBuf->resv[8]   = (UINT32)(-1); //pMem->SVCSize;
	pVStrmBuf->resv[9]   = (UINT32)(-1); //pMem->TemporalId;
	pVStrmBuf->resv[10]  = IsIFrame;     //pMem->IsKey;
	pVStrmBuf->resv[15]  = (UINT32)(-1); //pMem->uiBaseQP;

	// convert to physical addr
	pVStrmBuf->resv[11]	= VDOENC_VIRT2PHY(oport, pVStrmBuf->DataAddr); // bs1  phy
	if (p_ctx_p->gisf_vdoenc_bs_addr2 == 0) {
		pVStrmBuf->resv[0]	= 0; // bs2  phy
		pVStrmBuf->resv[4]	= 0; // bs2  phy
	} else {
		pVStrmBuf->resv[0]	= VDOENC_VIRT2PHY(oport, p_ctx_p->gisf_vdoenc_bs_addr2); // bs2  phy
		pVStrmBuf->resv[4]	= p_ctx_p->gisf_vdoenc_bs_addr2;						 // use to release bs2
	}
	pVStrmBuf->resv[12]	= VDOENC_VIRT2PHY(oport, p_ctx_p->gisf_vdoenc_sps_addr);  // sps phy
	pVStrmBuf->resv[13]	= VDOENC_VIRT2PHY(oport, p_ctx_p->gisf_vdoenc_pps_addr);  // pps phy
	pVStrmBuf->resv[14]	= VDOENC_VIRT2PHY(oport, p_ctx_p->gisf_vdoenc_vps_addr);  // vps phy

	pVStrmBuf->timestamp = (UINT32)(-1); //pMem->TimeStamp;

	// Set BS ISF_DATA info
	memcpy(pBsData->desc, pVStrmBuf, sizeof(VDO_BITSTREAM));
	pBsData->h_data      = 0; //pMem->Addr;  // partial fake wrapped ISF_DATA, don't have to release memory

	// copy wrapped ISF_DATA to user provided ISF_DATA
	memcpy(p_data, pBsData, sizeof(ISF_DATA));

	return ISF_ERR_INTERMEDIATE;  ///< intermediate return - successful but not complete
}

static ISF_RV _ISF_VdoEnc_OutputPort_PullImgBufFromSrc(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	ISF_RV ret = ISF_OK;
	VDOENC_CONTEXT_PORT   *p_ctx_p = NULL;

	if (g_ve_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[oport];

	if (p_ctx_p->gISF_VdoEnc_Opened == FALSE) {
		DBG_ERR("module is not open yet\r\n");
		return ISF_ERR_NOT_OPEN_YET;
	}

	(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(oport), ISF_USER_PROBE_PULL, ISF_ENTER);  // [USER] PULL_ENTER

	if (gISF_VdoEncMsgLevel[oport] & ISF_VDOENC_MSG_BS_INTERVAL) {
		static UINT32 last_time[ISF_VDOENC_IN_NUM] = {0};
		static UINT32 cur_time [ISF_VDOENC_IN_NUM] = {0};
		cur_time[oport] = Perf_GetCurrent();
		DBG_DUMP("[ISF_VDOENC][%d] BS interval = %d (us)\r\n", oport, (cur_time[oport] - last_time[oport]));
		last_time[oport] = cur_time[oport];
	}

	if (wait_ms == -99) {
		//--- [ partial mode ] ---
		ret = _ISF_VdoEnc_Get_PullQueue(oport, p_data, 0);  // use (timeout = 0 ms) try to get if there's already any complete frame
		if (ret == ISF_OK) {
			// return normal full result (already has full frame result in pullQ)
			return ISF_OK;
		} else {
			UINT32 is_encoding = 0;
			pNMI_VdoEnc->GetParam(oport, NMI_VDOENC_PARAM_IS_ENCODING, &is_encoding);

			if (is_encoding) {
				// return partial result (return encoding partial bs)
				SEM_WAIT(p_ctx_p->ISF_VDOENC_PARTIAL_SEM_ID);
#if 0
				// query status again (maybe in this time...it's already full done encode & callback wrap normal full ISF_DATA) ( BsNow had been updated, so this partial ISF_DATA will be wrong and is unnecessary to use partial ISF_DATA )
				pNMI_VdoEnc->GetParam(oport, NMI_VDOENC_PARAM_IS_ENCODING, &is_encoding);
				if (is_encoding == 0) {
					SEM_SIGNAL(p_ctx_p->ISF_VDOENC_PARTIAL_SEM_ID);
					return ISF_ERR_QUEUE_EMPTY;
				}
#else
				// try to query again (maybe in this time...it's already full done encode & callback wrap normal full ISF_DATA)
				if (ISF_OK == _ISF_VdoEnc_Get_PullQueue(oport, p_data, 0)) {
					SEM_SIGNAL(p_ctx_p->ISF_VDOENC_PARTIAL_SEM_ID);
					return ISF_OK;
				}
#endif
				ret = _ISF_VdoEnc_OutputPort_WrapPartialFrame(oport, p_data);
				SEM_SIGNAL(p_ctx_p->ISF_VDOENC_PARTIAL_SEM_ID);
			}else{
				// return fail (no any bs in pullQ, and is not encoding currently)
				return ISF_ERR_QUEUE_EMPTY;
			}
		}
	} else {
		//--- [ normal mode ] ---
		ret = _ISF_VdoEnc_Get_PullQueue(oport, p_data, wait_ms);  // [USER] do_probe  PULL_ERR, PULL_OK  will do in this function
	}

	return ret;
}
#endif

static ISF_RV ISF_VdoEnc_BindInput(struct _ISF_UNIT *pThisUnit, UINT32 iPort, struct _ISF_UNIT *pSrcUnit, UINT32 oPort)
{
	return ISF_OK;
}

static ISF_RV ISF_VdoEnc_BindOutput(struct _ISF_UNIT *pThisUnit, UINT32 oPort, struct _ISF_UNIT *pDestUnit, UINT32 iPort)
{
	return ISF_OK;
}

BOOL _isf_vdoenc_is_allpath_stopped(void)
{
	UINT32 i;
	BOOL b_all_stopped = TRUE;

	for (i=0; i<PATH_MAX_COUNT; i++) {
		VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[i];
		if (p_ctx_p->gISF_VdoEnc_Started == 1) {
			b_all_stopped = FALSE;
			break;
		}
	}
	return b_all_stopped;
}

static ISF_RV ISF_VdoEnc_SetParam(ISF_UNIT *pThisUnit, UINT32 param, UINT32 value)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;

	if (!p_ctx_c->g_bisf_vdoenc_addunit) {
		p_ctx_c->g_bisf_vdoenc_addunit = TRUE;
		NMR_VdoEnc_AddUnit();
		_ISF_VdoEnc_InstallCmd();
		_isf_vdoenc_bs_data_init();
	}

	if (!p_ctx_c->g_bisf_imgcap_addunit) {
		p_ctx_c->g_bisf_imgcap_addunit = TRUE;
		NMR_ImgCap_AddUnit();
	}

	if (!pNMI_VdoEnc) {
		if ((pNMI_VdoEnc = NMI_GetUnit("VdoEnc")) == NULL) {
			DBG_ERR("Get VdoEnc unit failed\r\n");
			return ISF_ERR_NULL_OBJECT;
		}
	}

	switch (param) {
	case VDOENC_PARAM_EVENT_CB:
		p_ctx_c->gfn_vdoenc_cb = (IsfVdoEncEventCb *)value;
		break;

    case VDOENC_PARAM_RAW_DATA_CB:
        p_ctx_c->gfn_vdoenc_raw_cb = (IsfVdoEncEventCb *)value;
        break;


    case VDOENC_PARAM_SRC_OUT_CB: {
        p_ctx_c->gfn_vdoenc_srcout_cb = (IsfVdoEncEventCb *)value;
        }
        break;

	case VDOENC_PARAM_PROC_CB:
		p_ctx_c->gfn_vdoenc_proc_cb = (VDOENC_PROC_CB)value;
		break;
	}

	return ISF_OK;
}


static UINT32 ISF_VdoEnc_GetParam(ISF_UNIT *pThisUnit, UINT32 param)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	UINT32 value =0;
	switch (param) {
	case VDOENC_PARAM_POLL_LIST_BS:
		value = p_ctx_c->g_isf_vdoenc_poll_bs_event_mask;
		break;
	}
	return value;
}

static ISF_RV ISF_VdoEnc_SetPortParam(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param, UINT32 value)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	ISF_RV ret = ISF_OK;

    if (nPort == ISF_CTRL)
    {
        return ISF_VdoEnc_SetParam(pThisUnit, param, value);
    }

	if (!p_ctx_c->g_bisf_vdoenc_addunit) {
		p_ctx_c->g_bisf_vdoenc_addunit = TRUE;
		NMR_VdoEnc_AddUnit();
		_ISF_VdoEnc_InstallCmd();
		_isf_vdoenc_bs_data_init();
	}

	if (!p_ctx_c->g_bisf_imgcap_addunit) {
		p_ctx_c->g_bisf_imgcap_addunit = TRUE;
		NMR_ImgCap_AddUnit();
	}

/*
	if (nPort >= ISF_OUT_MAX) {
		DBG_ERR("Set Output Port Param only!\r\n");
		return ISF_ERR_FAILED;
	}

	if (nPort >= PATH_MAX_COUNT) {
		DBG_ERR("Set Output Port Param only!\r\n");
		return ISF_ERR_FAILED;
	}
*/
	if (!pNMI_VdoEnc) {
		if ((pNMI_VdoEnc = NMI_GetUnit("VdoEnc")) == NULL) {
			DBG_ERR("Get VdoEnc unit failed\r\n");
			return ISF_ERR_NULL_OBJECT;
		}
	}

	if (g_ve_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

#if _TODO
	if (!pNMI_ImgCap) {
		if ((pNMI_ImgCap = NMI_GetUnit("ImgCap")) == NULL) {
			DBG_ERR("Get ImgCap unit failed\r\n");
			return ISF_ERR_NULL_OBJECT;
		}
	}
#endif

	if ((nPort < (int)ISF_IN_BASE) && (nPort < pThisUnit->port_out_count)) {
		VDOENC_CONTEXT_PORT   *p_ctx_p = NULL;
		if (nPort-ISF_OUT_BASE >= PATH_MAX_COUNT) {
			DBG_ERR("[VDOENC] invalid path index = %d\r\n", nPort-ISF_OUT_BASE);
			return ISF_ERR_INVALID_PORT_ID;
		}
		p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[nPort-ISF_OUT_BASE];

#if 1
	switch (param) {
	case VDOENC_PARAM_ROI: // struct
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_ROI, value);
		break;

	case VDOENC_PARAM_DIS:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "dis(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_DIS, value);
		break;

	case VDOENC_PARAM_TV_RANGE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "tv_range(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_TV_RANGE, value);
		break;

	case VDOENC_PARAM_START_TIMER_BY_MANUAL:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "start_timer_by_manual(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_START_TIMER_BY_MANUAL, value);
		break;

	case VDOENC_PARAM_START_TIMER:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "start_timer(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_START_TIMER, 0);
		break;

	case VDOENC_PARAM_STOP_TIMER:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "stop_timer(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_STOP_TIMER, 0);
		break;

   	case VDOENC_PARAM_START_SMART_ROI:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "start_smart_roi(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_START_SMART_ROI, value);
		break;

   	case VDOENC_PARAM_WAIT_SMART_ROI:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "wait_smart_roi(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_WAIT_SMART_ROI, value);
		break;

	case VDOENC_PARAM_RESET_IFRAME:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "reset_iframe(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_RESET_IFRAME, value);
		break;

	case VDOENC_PARAM_RESET_SEC:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "reset_sec(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_RESET_SEC, value);
		break;

	case VDOENC_PARAM_ENCODER_OBJ:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "encoder_obj(0x%08x)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_ENCODER_OBJ, value);
		break;

	case VDOENC_PARAM_MAX_MEM_INFO:  // struct
		{
			ISF_RV r = ISF_OK;
			NMI_VDOENC_MAX_MEM_INFO* pMaxMemInfo = (NMI_VDOENC_MAX_MEM_INFO*) value;

			if (nPort < PATH_MAX_COUNT) {
				if (pMaxMemInfo->bRelease) {
					if ((p_ctx_p->gISF_VdoEncMaxMem.Addr != 0) && (p_ctx_p->gISF_VdoEncMaxMemBlk.h_data != 0)) {
						r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_VdoEncMaxMemBlk), ISF_OK);
						if (r == ISF_OK) {
							p_ctx_p->gISF_VdoEncMaxMem.Addr = 0;
							p_ctx_p->gISF_VdoEncMaxMem.Size = 0;
							p_ctx_p->gISF_VdoEncMaxMemSize = 0;
							memset(&p_ctx_p->gISF_VdoEncMaxMemBlk, 0, sizeof(ISF_DATA));
						} else {
							DBG_ERR("[ISF_VDOENC][%d] release VdoEnc Max blk failed, ret = %d\r\n", nPort, r);
						}
					}
				} else {
					if (p_ctx_p->gISF_VdoEncMaxMem.Addr == 0) {
						pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_MAX_MEM_INFO, (UINT32)pMaxMemInfo);
						p_ctx_p->gISF_VdoEncMaxMemSize = pMaxMemInfo->uiSnapShotSize + pMaxMemInfo->uiCodecsize + pMaxMemInfo->uiEncsize;
					} else {
						DBG_ERR("[ISF_VDOENC][%d] max buf exist, please release it first.\r\n", nPort);
					}
				}
			} else {
				DBG_ERR("[ISF_VDOENC] invalid port index = %d\r\n", nPort);
			}
		}
		break;

	case VDOENC_PARAM_IMGCAP_MAX_MEM_INFO:  // struct
		{
			ISF_RV r = ISF_OK;
			NMI_IMGCAP_MAX_MEM_INFO* pMaxMemInfo = (NMI_IMGCAP_MAX_MEM_INFO*) value;

			if (nPort < PATH_MAX_COUNT) {
				if (pMaxMemInfo->bRelease) {
					if (p_ctx_c->gISF_ImgCapMaxMem.Addr != 0) {
						r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_c->gISF_ImgCapMaxMemBlk), ISF_OK);
						if (r == ISF_OK) {
							p_ctx_c->gISF_ImgCapMaxMem.Addr = 0;
							p_ctx_c->gISF_ImgCapMaxMem.Size = 0;
							p_ctx_c->gISF_ImgCapMaxMemSize = 0;
						} else {
							DBG_ERR("ImgCap release max blk failed, ret = %d\r\n", r);
						}
					} else {
						DBG_ERR("gISF_ImgCapMaxMem.Addr = 0x%x\r\n", p_ctx_c->gISF_ImgCapMaxMem.Addr);
					}
				} else {
					if (p_ctx_c->gISF_ImgCapMaxMem.Addr == 0) {
						pNMI_ImgCap->SetParam(0, NMI_IMGCAP_PARAM_MAX_MEM_INFO, (UINT32)pMaxMemInfo);
						p_ctx_c->gISF_ImgCapMaxMemSize = pMaxMemInfo->uiNeedSize;
						p_ctx_c->bIs_NMI_ImgCap_ON = TRUE;
						DBG_IND("ImgCap max buf info, w=%d, h=%d, NeedSize=%d\r\n", pMaxMemInfo->uiWidth, pMaxMemInfo->uiHeight, pMaxMemInfo->uiNeedSize);
					} else {
						DBG_ERR("ImgCap max buf exist, please release it first.\r\n");
					}
				}
			} else {
				DBG_ERR("[ISF_VDOENC] ImgCap invalid port index = %d\r\n", nPort);
			}
		}
		break;

/*
	case VDOENC_PARAM_WIDTH:
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_WIDTH, value);
		p_ctx_p->gISF_VStrmBuf.Width = value;
		ret = ISF_APPLY_DOWNSTREAM_OFFTIME;
		break;

	case VDOENC_PARAM_HEIGHT:
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_HEIGHT, value);
		p_ctx_p->gISF_VStrmBuf.Height = value;
		ret = ISF_APPLY_DOWNSTREAM_OFFTIME;
		break;
*/

	case VDOENC_PARAM_FRAMERATE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "framerate(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_FRAMERATE, value);
		p_ctx_p->gISF_VStrmBuf.framepersecond = value;
		//ret = ISF_APPLY_DOWNSTREAM_OFFTIME;  // [isf_flow] doesn't support this
		break;

	case VDOENC_PARAM_FPS_IMM:
		{
			UINT32 id = nPort - ISF_OUT_BASE;
			//[isf_stream] ISF_PORT* pSrcPort =  _isf_unit_in(&isf_vdoenc, ISF_IN(id)); //ImageUnit_In(&ISF_VdoEnc, id+ISF_IN_BASE);
			ISF_PORT* pSrcPort =  isf_vdoenc.p_base->iport(&isf_vdoenc, ISF_IN(id)); // [isf_flow] use this ...
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "fps_imm(%d)", value);

			if(id >= PATH_MAX_COUNT)
				return ISF_ERR_INVALID_PORT_ID;
			if(pSrcPort->connecttype == (ISF_CONNECT_PUSH|ISF_CONNECT_SYNC)) {
				//INT16 dst_fr = GET_HI_UINT16(value);
				//INT16 src_fr = GET_LO_UINT16 (value);
				//#if (DUMP_FRC == ENABLE)
				//DBG_DUMP("^MVdoEnc.out[%d] set fps: %d/%d\r\n", id, dst_fr, src_fr);
				//#endif
				p_ctx_p->gISF_VdoEnc_frc.FramePerSecond_new = value;
				#if (MANUAL_SYNC == ENABLE)
				#else
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_TIMERRATE_IMM, src_fr);
				#endif
			} else {
				// ...
			}
		}
		break;

	case VDOENC_PARAM_FUNC:
		{
			UINT32 id = nPort - ISF_OUT_BASE;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "param_func(%d)", value);
			if(id >= PATH_MAX_COUNT)
				return ISF_ERR_INVALID_PORT_ID;
			p_ctx_p->gISF_VdoFunc = value;

			if (p_ctx_p->gISF_VdoFunc & VDOENC_FUNC_LOWLATENCY) {
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_LOWLATENCY, 1);
			} else {
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_LOWLATENCY, 0);
			}
		}
		break;

	case VDOENC_PARAM_CODEC:
#if 0
		//pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_CODEC, value);
		// coverity[overrun-local]
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "codec(%d)", value);
		p_ctx_p->gISF_VStrmBuf.CodecType = value;
		//ret = ISF_APPLY_DOWNSTREAM_OFFTIME;  // [isf_flow] doesn't support this
#else
		{
			PMP_VDOENC_ENCODER p_video_obj = NULL;
			UINT32 id = nPort - ISF_OUT_BASE;
#ifdef __KERNEL__
			UINT32 user_codec = 0;

			if (nPort == 0) {
				user_codec = (value == MEDIAVIDENC_H265) ? 0 : 1;
				VdoEnc_Builtin_SetParam(nPort, BUILTIN_VDOENC_INIT_PARAM_DYNAMIC_CODEC, user_codec);
			}
			pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_FASTBOOT_CODEC, value);

			if (nPort < BUILTIN_VDOENC_PATH_ID_MAX) {
				if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[nPort] && p_ctx_c->bIs_fastboot_enc[nPort]) {
					VdoEnc_Builtin_GetParam(nPort, BUILTIN_VDOENC_INIT_PARAM_CODEC, &value);
					if (value == BUILTIN_VDOENC_H265) {
						value = MEDIAVIDENC_H265;
					} else if (value == BUILTIN_VDOENC_H264) {
						value = MEDIAVIDENC_H264;
					} else if (value == BUILTIN_VDOENC_MJPEG) {
						value = MEDIAVIDENC_MJPG;
					}
				}
			}
#endif
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "codec(%d)", value);

			p_ctx_p->gISF_VStrmBuf.CodecType = value;
			pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_CODEC, p_ctx_p->gISF_VStrmBuf.CodecType);

			switch (p_ctx_p->gISF_VStrmBuf.CodecType) {
				case MEDIAVIDENC_MJPG:	p_video_obj = MP_MjpgEnc_getVideoObject((MP_VDOENC_ID)id);		break;
				case MEDIAVIDENC_H264:	p_video_obj = MP_H264Enc_getVideoObject((MP_VDOENC_ID)id);		break;
				case MEDIAVIDENC_H265:	p_video_obj = MP_H265Enc_getVideoObject((MP_VDOENC_ID)id);		break;
				default:
					DBG_ERR("[ISF_VDOENC][%d] getVideoObject failed\r\n", id);
					return ISF_ERR_INVALID_PARAM;
			}

			pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_ENCODER_OBJ, (UINT32)p_video_obj);
		}
#endif
		break;

	case VDOENC_PARAM_PROFILE:
#ifdef __KERNEL__
		if (nPort < BUILTIN_VDOENC_PATH_ID_MAX) {
			if (p_ctx_c->bIs_fastboot_enc[nPort]) {
				if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[nPort]) {
					VdoEnc_Builtin_GetParam(nPort, BUILTIN_VDOENC_INIT_PARAM_PROFILE, &value);
				}
			}
		}
#endif
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "profile(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_PROFILE, value);
		//ret = ISF_APPLY_DOWNSTREAM_OFFTIME;  // [isf_flow] doesn't support this
		break;

	case VDOENC_PARAM_TARGETRATE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "targetrate(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_TARGETRATE, value);
		p_ctx_p->gISF_VStrmBuf.BytePerSecond = value;
		//ret = ISF_APPLY_DOWNSTREAM_OFFTIME;  // [isf_flow] doesn't support this
		break;

	case VDOENC_PARAM_ENCBUF_MS:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "encbuf_ms(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_ENCBUF_MS, value);
		//ret = ISF_APPLY_DOWNSTREAM_OFFTIME;  // [isf_flow] doesn't support this
		break;

	case VDOENC_PARAM_ENCBUF_RESERVED_MS:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "encbuf_reserved_ms(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_ENCBUF_RESERVED_MS, value);
		//ret = ISF_APPLY_DOWNSTREAM_OFFTIME;  // [isf_flow] doesn't support this
		break;

	case VDOENC_PARAM_GOPTYPE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "goptype(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_GOPTYPE, value);
		//ret = ISF_APPLY_DOWNSTREAM_READYTIME;  // [isf_flow] doesn't support this
		break;

	case VDOENC_PARAM_GOPNUM:
#ifdef __KERNEL__
		if (nPort < BUILTIN_VDOENC_PATH_ID_MAX) {
			if (p_ctx_c->bIs_fastboot_enc[nPort]) {
				if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[nPort]) {
					VdoEnc_Builtin_GetParam(nPort, BUILTIN_VDOENC_INIT_PARAM_GOP_NUM, &value);
				}
			}
		}
#endif
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "gop(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_GOPNUM, value);
		//ret = ISF_APPLY_DOWNSTREAM_READYTIME;  // [isf_flow] doesn't support this
		break;

	case VDOENC_PARAM_CHRM_QP_IDX:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "chrm_qp_idx(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_CHRM_QP_IDX, value);
		break;

	case VDOENC_PARAM_SEC_CHRM_QP_IDX:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "sec_chrm_qp_idx(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_SEC_CHRM_QP_IDX, value);
		break;

	case VDOENC_PARAM_GOPNUM_ONSTART:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "gop_onstart(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_GOPNUM_ONSTART, value);
		break;

	case VDOENC_PARAM_RECFORMAT:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "rec_format(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_RECFORMAT, value);
        //ret = ISF_APPLY_DOWNSTREAM_OFFTIME;  // [isf_flow] doesn't support this
		break;

	case VDOENC_PARAM_BS_RESERVED_SIZE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "bs_reserved_size(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_BS_RESERVED_SIZE, value);
		break;

	case VDOENC_PARAM_FILETYPE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "file_type(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_FILETYPE, value);
        //ret = ISF_APPLY_DOWNSTREAM_OFFTIME;  // [isf_flow] doesn't support this
		break;

	case VDOENC_PARAM_INITQP:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "init_qp(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_INITQP, value);
		break;

	case VDOENC_PARAM_MINQP:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "min_qp(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_MINQP, value);
		break;

	case VDOENC_PARAM_MAXQP:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "max_qp(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_MAXQP, value);
		break;

	case VDOENC_PARAM_DAR:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "dar(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_DAR, value);
		break;

	case VDOENC_PARAM_SKIP_FRAME:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "skip_frame(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_SKIP_FRAME, value);
		break;

	case VDOENC_PARAM_SVC:
#ifdef __KERNEL__
		if (nPort < BUILTIN_VDOENC_PATH_ID_MAX) {
			if (p_ctx_c->bIs_fastboot_enc[nPort]) {
				if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[nPort]) {
					VdoEnc_Builtin_GetParam(nPort, BUILTIN_VDOENC_INIT_PARAM_SVC, &value);
				}
			}
		}
#endif
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "svc(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_SVC, value);
		break;

	case VDOENC_PARAM_LTR: // struct
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_LTR, value);
		break;

	case VDOENC_PARAM_MULTI_TEMPORARY_LAYER:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "multi_temporary_laryer(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_MULTI_TEMPORARY_LAYER, value);
		break;

	case VDOENC_PARAM_USRQP:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "usrqp(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_USRQP, value);  // this set H265 is (0: 64x64,  1: 16x16)
		break;

	case VDOENC_PARAM_CBRINFO: // struct
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_CBRINFO, value);
		break;

	case VDOENC_PARAM_EVBRINFO: // struct
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_EVBRINFO, value);
		break;

	case VDOENC_PARAM_VBRINFO: // struct
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_VBRINFO, value);
		break;

	case VDOENC_PARAM_FIXQPINFO: //struct
		{
#ifdef __KERNEL__
			MP_VDOENC_FIXQP_INFO *pFixQpInfo = (MP_VDOENC_FIXQP_INFO *)value;
			if (nPort < BUILTIN_VDOENC_PATH_ID_MAX) {
				if (p_ctx_c->bIs_fastboot_enc[nPort]) {
					if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[nPort]) {
						VdoEnc_Builtin_GetParam(nPort, BUILTIN_VDOENC_INIT_PARAM_FRAME_RATE, &pFixQpInfo->frame_rate);
					}
				}
			}
#endif
		}
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_FIXQPINFO, value);
		break;

	case VDOENC_PARAM_AQINFO: // struct
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_AQINFO, value);
		break;

	case VDOENC_PARAM_3DNR_CB:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "3dnr cb = (0x%08x)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_3DNR_CB, value);
		break;

	case VDOENC_PARAM_IMM_CB:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "imm cb = (0x%08x)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_IMM_CB, value);
		break;

	case VDOENC_PARAM_FRAME_INTERVAL:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "frame_interval(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_FRAME_INTERVAL, value);
		break;

	case VDOENC_PARAM_IMGCAP_MAX_W:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "imgcap_max_w(%d)", value);
		pNMI_ImgCap->SetParam(nPort, NMI_IMGCAP_SET_MAX_W, value);
		p_ctx_c->bIs_NMI_ImgCap_ON = TRUE;
		break;

	case VDOENC_PARAM_IMGCAP_MAX_H:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "imgcap_max_h(%d)", value);
		pNMI_ImgCap->SetParam(nPort, NMI_IMGCAP_SET_MAX_H, value);
		p_ctx_c->bIs_NMI_ImgCap_ON = TRUE;
		break;

	case VDOENC_PARAM_IMGCAP_JPG_BUFNUM:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "imgcap_jpg_bufnum(%d)", value);
		pNMI_ImgCap->SetParam(nPort, NMI_IMGCAP_SET_JPG_BUFNUM, value);
		break;

	//#NT#2018/03/27#KCHong -begin
	//#NT# Use ImageUnit_SendEvent() instead of ImageUnit_SetParam() to avoid semaphore dead lock
	#if 0
	case VDOENC_PARAM_IMGCAP_THUMB: {
			ISIZE    *pthumb_size = (ISIZE *) value;

			if (pthumb_size) {
				// coverity[overrun-local]
				p_ctx_p->gISF_ImgCap_Thumb_Job.path_id = nPort;
				p_ctx_p->gISF_ImgCap_Thumb_Job.mode    = NMI_IMGCAP_JPG_ONLY;
				p_ctx_p->gISF_ImgCap_Thumb_Job.jpg_w   = pthumb_size->w;
				p_ctx_p->gISF_ImgCap_Thumb_Job.jpg_h   = pthumb_size->h;
				p_ctx_p->gISF_ImgCap_Thumb_Job.thumb   = TRUE;

				pNMI_ImgCap->SetParam(0, NMI_IMGCAP_SET_OBJ, (UINT32) &p_ctx_p->gISF_ImgCap_Thumb_Job);
			}
		}
		break;
	#endif
	//#NT#2018/03/27#KCHong -end

	case VDOENC_PARAM_IMGCAP_ACTION: {
			ISIZE    *psize = (ISIZE *) value;

			if (psize) {
				NMI_IMGCAP_JOB Job = {0};
				Job.path_id        = nPort;
				Job.mode           = NMI_IMGCAP_JPG_ONLY;
				Job.jpg_w          = psize->w;
				Job.jpg_h          = psize->h;
				Job.thumb          = FALSE;

				pNMI_ImgCap->SetParam(0, NMI_IMGCAP_SET_OBJ, (UINT32) &Job);
			}
		}
		break;

    case VDOENC_PARAM_TIMELAPSE_TIME: {
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "timelapse_time(%d)", value);
        pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_TIMELAPSE_TIME, value);
        }
        break;

    case VDOENC_PARAM_ALLOC_SNAPSHOT_BUF: {
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "alloc_snapshot_buf(%d)", value);
        pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_ALLOC_SNAPSHOT_BUF, value);
        }
        break;

    case VDOENC_PARAM_SNAPSHOT: {
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "snapshot(%d)", value);
        pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_SNAPSHOT, value);
        }
        break;

	case VDOENC_PARAM_MIN_I_RATIO:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "min_i_ratio(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_MIN_I_RATIO, value);
		break;

	case VDOENC_PARAM_MIN_P_RATIO:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "min_p_ratio(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_MIN_P_RATIO, value);
		break;

	case VDOENC_PARAM_MAX_FRAME_QUEUE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "max_frame_queue(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_MAX_FRAME_QUEUE, value);
		break;

	case VDOENC_PARAM_ISP_ID:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "isp_id(%d)", value);
		p_ctx_p->gISF_VdoEnc_ISP_Id = value;    // this port use which ISP ID (sensor id) settings
		break;

	case VDOENC_PARAM_LEVEL_IDC:
#ifdef __KERNEL__
		if (nPort < BUILTIN_VDOENC_PATH_ID_MAX) {
			if (p_ctx_c->bIs_fastboot_enc[nPort]) {
				if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[nPort]) {
					VdoEnc_Builtin_GetParam(nPort, BUILTIN_VDOENC_INIT_PARAM_LEVEL_IDC, &value);
				}
			}
		}
#endif
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "level_idc(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_LEVEL_IDC, value);
		break;

	case VDOENC_PARAM_ENTROPY:
#ifdef __KERNEL__
		if (nPort < BUILTIN_VDOENC_PATH_ID_MAX) {
			if (p_ctx_c->bIs_fastboot_enc[nPort]) {
				if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[nPort]) {
					VdoEnc_Builtin_GetParam(nPort, BUILTIN_VDOENC_INIT_PARAM_ENTROPY, &value);
				}
			}
		}
#endif
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "entropy(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_ENTROPY, value);
		break;

	case VDOENC_PARAM_GRAY:
#ifdef __KERNEL__
		if (nPort < BUILTIN_VDOENC_PATH_ID_MAX) {
			if (p_ctx_c->bIs_fastboot_enc[nPort]) {
				if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[nPort]) {
					VdoEnc_Builtin_GetParam(nPort, BUILTIN_VDOENC_INIT_PARAM_GRAY_EN, &value);
				}
			}
		}
#endif
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "gray(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_GRAY, value);
		break;

	case VDOENC_PARAM_JPG_QUALITY:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "jpg_quality(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_JPG_QUALITY, value);
		break;

	case VDOENC_PARAM_JPG_RESTART_INTERVAL:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "jpg_restart_interval(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_JPG_RESTART_INTERVAL, value);
		break;

	case VDOENC_PARAM_JPG_BITRATE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "jpg_bitrate(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_JPG_BITRATE, value);
		break;

	case VDOENC_PARAM_JPG_FRAMERATE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "jpg_framerate(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_JPG_FRAMERATE, value);
		break;

	case VDOENC_PARAM_JPG_VBR_MODE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "jpg_vbr_mode(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_JPG_VBR_MODE, value);
		break;

	case VDOENC_PARAM_H26X_VBR_POLICY:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "h26x_vbr_policy(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_H26X_VBR_POLICY, value);
		break;

	case VDOENC_PARAM_JPG_YUV_TRAN_ENABLE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "jpg_yuv_trans_en(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_JPG_YUV_TRAN_ENABLE, value);
		break;

	case VDOENC_PARAM_JPG_RC_MIN_QUALITY:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "jpg_rc_min_q(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_JPG_RC_MIN_QUALITY, value);
		break;

	case VDOENC_PARAM_JPG_RC_MAX_QUALITY:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "jpg_rc_max_q(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_JPG_RC_MAX_QUALITY, value);
		break;

	case VDOENC_PARAM_COLMV_ENABLE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "h26x_colmv_en(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_COLMV_ENABLE, value);
		break;

	case VDOENC_PARAM_COMM_RECFRM_ENABLE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "h26x_comm_recfrm_en(%d)", value);
		p_ctx_p->gISF_VdoEncCommReconBuf_Enable = value;
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_COMM_RECFRM_ENABLE, value);
		break;

	case VDOENC_PARAM_COMM_BASE_RECFRM:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "h26x_comm_base_recfrm(%d)", value);
		p_ctx_p->gISF_VdoEncUseCommReconBuf[0] = value;
		break;

	case VDOENC_PARAM_COMM_SVC_RECFRM:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "h26x_comm_svc_recfrm(%d)", value);
		p_ctx_p->gISF_VdoEncUseCommReconBuf[1] = value;
		break;

	case VDOENC_PARAM_COMM_LTR_RECFRM:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "h26x_comm_ltr_recfrm(%d)", value);
		p_ctx_p->gISF_VdoEncUseCommReconBuf[2] = value;
		break;

	case VDOENC_PARAM_LONG_START_CODE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "long start code(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_LONG_START_CODE, value);
		break;

	case VDOENC_PARAM_H26X_SVC_WEIGHT_MODE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "h26x_svc_weight_mode(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_H26X_SVC_WEIGHT_MODE, value);
		break;

	case VDOENC_PARAM_BS_QUICK_ROLLBACK:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "bs_quick_rollback(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_BS_QUICK_ROLLBACK, value);
		break;

	case VDOENC_PARAM_QUALITY_BASE_MODE_ENABLE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "h26x_quality_base_mode_en(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_QUALITY_BASE_MODE_ENABLE, value);
		break;

	case VDOENC_PARAM_HW_PADDING_MODE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "hw_padding_mode(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_HW_PADDING_MODE, value);
		break;

	case VDOENC_PARAM_BR_TOLERANCE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "br_tolerance(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_BR_TOLERANCE, value);
		break;

	case VDOENC_PARAM_COMM_SRCOUT_ENABLE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "comm_srcout(%d)", value);
		p_ctx_p->gISF_VdoEncCommSrcOut_Enable = value;
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_COMM_SRCOUT_ENABLE, value);
		break;

	case VDOENC_PARAM_COMM_SRCOUT_NO_JPG_ENC:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "comm_srcout_no_jpg_enc(%d)", value);
		p_ctx_p->gISF_VdoEncCommSrcOut_NoJPGEnc = value;
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_COMM_SRCOUT_NO_JPG_ENC, value);
		break;

	case VDOENC_PARAM_COMM_SRCOUT_ADDR:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "comm_srcout_addr(%d)", value);
		p_ctx_p->gISF_VdoEncCommSrcOutAddr = value;
		p_ctx_p->gISF_VdoEncCommSrcOutAddr = nvtmpp_sys_pa2va(p_ctx_p->gISF_VdoEncCommSrcOutAddr);
		break;

	case VDOENC_PARAM_COMM_SRCOUT_SIZE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "comm_srcout_size(%d)", value);
		p_ctx_p->gISF_VdoEncCommSrcOutSize = value;
		break;

	case VDOENC_PARAM_TIMER_TRIG_COMP_ENABLE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "timer_trig_comp(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_TIMER_TRIG_COMP_ENABLE, value);
		break;

	case VDOENC_PARAM_H26X_SEI_CHKSUM:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "h26x_sei_chksum(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_H26X_SEI_CHKSUM, value);
		break;

	case VDOENC_PARAM_H26X_LOW_POWER:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "h26x_low_power(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_H26X_LOW_POWER, value);
		break;

	case VDOENC_PARAM_H26X_MAQ_DIFF_ENABLE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "h26x_maq_diff_en(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_H26X_MAQ_DIFF_ENABLE, value);
		break;

	case VDOENC_PARAM_H26X_MAQ_DIFF_STR:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "h26x_maq_diff_str(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_H26X_MAQ_DIFF_STR, value);
		break;

	case VDOENC_PARAM_H26X_MAQ_DIFF_START_IDX:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "h26x_maq_diff_start_idx(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_H26X_MAQ_DIFF_START_IDX, value);
		break;

	case VDOENC_PARAM_H26X_MAQ_DIFF_END_IDX:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "h26x_maq_diff_end_idx(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_H26X_MAQ_DIFF_END_IDX, value);
		break;

	case VDOENC_PARAM_BS_RING:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "bs_ring(%d)", value);
		p_ctx_p->gisf_vdoenc_bs_ring = value;
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_BS_RING, value);
		break;

	case VDOENC_PARAM_JPG_JFIF_ENABLE:
		ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "jpg_jfif_en(%d)", value);
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_JPG_JFIF_ENABLE, value);
		break;

	default:
		DBG_ERR("VdoEnc.out[%d] set param[%08x] = %d\r\n", nPort-ISF_OUT_BASE, param, value);
		break;
	}

    return ret;
#endif
	} else if (nPort-ISF_IN_BASE < pThisUnit->port_in_count) {
#if 1
		VDOENC_CONTEXT_PORT   *p_ctx_p = NULL;
		if (nPort-ISF_IN_BASE >= PATH_MAX_COUNT) {
			DBG_ERR("[VDOENC] invalid path index = %d\r\n", nPort-ISF_IN_BASE);
			return ISF_ERR_INVALID_PORT_ID;
		}
		p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[nPort-ISF_IN_BASE];

		switch (param) {
		#if 0	// [isf_flow] doesn't support "ISF_APPLY_DOWNSTREAM_OFFTIME"
		case ISF_PARAM_PORT_DIRTY:
	        if (value & ISF_INFO_DIRTY_IMGSIZE)
	            return ISF_APPLY_DOWNSTREAM_OFFTIME;
	        if (value & ISF_INFO_DIRTY_FRAMERATE)
	            return ISF_APPLY_DOWNSTREAM_OFFTIME;
	        if (value & ISF_INFO_DIRTY_DIRECT)
	            return ISF_APPLY_DOWNSTREAM_OFFTIME;
	        if (value & ISF_INFO_DIRTY_BUFCOUNT)
	            return ISF_APPLY_DOWNSTREAM_OFFTIME;
	    		break;
        #endif

		case VDOENC_PARAM_SYNCSRC:
			p_ctx_p->ISF_VdoSrcUnit = (ISF_UNIT*)value;
			p_ctx_p->ISF_VdoSrcPort = 0;
			return ISF_OK;
			break;

		default:
			DBG_ERR("VdoEnc.in[%d] set param[%08x] = %d\r\n", nPort-ISF_IN_BASE, param, value);
			break;
		}
#endif
	}
	return ISF_ERR_NOT_SUPPORT;
}


static UINT32 ISF_VdoEnc_GetPortParam(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param)
{
	UINT32 value = 0;
	/*
	if (nPort >= ISF_OUT_MAX) {
		DBG_ERR("Get Output Port Param only!\r\n");
		return value;
	}
*/
    if (nPort == ISF_CTRL)
    {
        return ISF_VdoEnc_GetParam(pThisUnit, param);
    }

	if (!pNMI_VdoEnc) {
		if ((pNMI_VdoEnc = NMI_GetUnit("VdoEnc")) == NULL) {
			DBG_ERR("Get VdoEnc unit failed\r\n");
			return value;
		}
	}

	if (g_ve_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

#if _TODO
	if (!pNMI_ImgCap) {
		if ((pNMI_ImgCap = NMI_GetUnit("ImgCap")) == NULL) {
			DBG_ERR("Get ImgCap unit failed\r\n");
			return ISF_ERR_NULL_OBJECT;
		}
	}
#endif

	if ((nPort < (int)ISF_IN_BASE) && (nPort < pThisUnit->port_out_count)) {
		VDOENC_CONTEXT_PORT   *p_ctx_p = NULL;
		if (nPort-ISF_OUT_BASE >= PATH_MAX_COUNT) {
			DBG_ERR("[VDOENC] invalid path index = %d\r\n", nPort-ISF_OUT_BASE);
			return ISF_ERR_INVALID_PORT_ID;
		}
		p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[nPort - ISF_OUT_BASE];

	switch (param) {
	case VDOENC_PARAM_ROI:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_ROI, &value);
		break;

	case VDOENC_PARAM_LTR:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_LTR, &value);
		break;

	case VDOENC_PARAM_MULTI_TEMPORARY_LAYER:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_MULTI_TEMPORARY_LAYER, &value);
		break;

	case VDOENC_PARAM_USRQP:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_USRQP, &value);
		break;

	case VDOENC_PARAM_DIS:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_DIS, &value);
		break;

	case VDOENC_PARAM_TV_RANGE:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_TV_RANGE, &value);
		break;

	case VDOENC_PARAM_START_TIMER_BY_MANUAL:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_START_TIMER_BY_MANUAL, &value);
		break;

   	case VDOENC_PARAM_START_SMART_ROI:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_START_SMART_ROI, &value);
		break;

   	case VDOENC_PARAM_WAIT_SMART_ROI:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_WAIT_SMART_ROI, &value);
		break;
    /*
	case VDOENC_PARAM_WIDTH:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_WIDTH, &value);
		break;
	case VDOENC_PARAM_HEIGHT:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_HEIGHT, &value);
		break;
    */

	case VDOENC_PARAM_FRAMERATE:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_FRAMERATE, &value);
		break;

	case VDOENC_PARAM_CODEC:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_CODEC, &value);
		break;

	case VDOENC_PARAM_PROFILE:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_PROFILE, &value);
		break;

	case VDOENC_PARAM_TARGETRATE:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_TARGETRATE, &value);
		break;

	case VDOENC_PARAM_ENCBUF_MS:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_ENCBUF_MS, &value);
		break;

	case VDOENC_PARAM_ENCBUF_RESERVED_MS:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_ENCBUF_RESERVED_MS, &value);
		break;

	case VDOENC_PARAM_GOPTYPE:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_GOPTYPE, &value);
		break;

	case VDOENC_PARAM_RECFORMAT:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_RECFORMAT, &value);
		break;

	case VDOENC_PARAM_FILETYPE:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_FILETYPE, &value);
		break;

	case VDOENC_PARAM_ENCBUF_ADDR:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_ENCBUF_ADDR, &value);
		break;

	case VDOENC_PARAM_ENCBUF_SIZE:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_ENCBUF_SIZE, &value);
		break;

#if (SUPPORT_PULL_FUNCTION)
	case VDOENC_PARAM_BUFINFO_PHYADDR:
		value = p_ctx_p->gISF_VdoEncMem_MmapInfo.addr_physical;
		break;

	case VDOENC_PARAM_BUFINFO_SIZE:
		value = p_ctx_p->gISF_VdoEncMem_MmapInfo.size;
		break;
#endif

	case VDOENC_PARAM_JPG_QUALITY:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_JPG_QUALITY, &value);
		break;

	case VDOENC_PARAM_SNAPSHOT_PURE_LINUX:   // setportstruct to trigger, getportparam to get size
		value = p_ctx_p->gISF_VdoEnc_SnapSize;
		break;

	case VDOENC_PARAM_JPG_YUV_TRAN_ENABLE:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_JPG_YUV_TRAN_ENABLE, &value);
		break;

	case VDOENC_PARAM_JPG_RC_MIN_QUALITY:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_JPG_RC_MIN_QUALITY, &value);
		break;

	case VDOENC_PARAM_JPG_RC_MAX_QUALITY:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_JPG_RC_MAX_QUALITY, &value);
		break;

	case VDOENC_PARAM_IMGCAP_GET_JPG_ADDR:
		pNMI_ImgCap->GetParam(nPort, NMI_IMGCAP_GET_JPG_ADDR, &value);
		break;
	case VDOENC_PARAM_IMGCAP_GET_JPG_SIZE:
		pNMI_ImgCap->GetParam(nPort, NMI_IMGCAP_GET_JPG_SIZE, &value);
		break;
	case VDOENC_PARAM_BS_RING:
		value = p_ctx_p->gisf_vdoenc_bs_ring;
		break;
	case VDOENC_PARAM_JPG_JFIF_ENABLE:
		pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_JPG_JFIF_ENABLE, &value);
		break;
	}

	} else if (nPort-ISF_IN_BASE < pThisUnit->port_in_count) {
		VDOENC_CONTEXT_PORT   *p_ctx_p = NULL;
		if (nPort-ISF_IN_BASE >= PATH_MAX_COUNT) {
			DBG_ERR("[VDOENC] invalid path index = %d\r\n", nPort-ISF_IN_BASE);
			return ISF_ERR_INVALID_PORT_ID;
		}
		p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[nPort-ISF_IN_BASE];

		switch (param) {
		case VDOENC_PARAM_SYNCSRC:
			return (UINT32)(p_ctx_p->ISF_VdoSrcUnit);
			break;

		default:
			DBG_ERR("VdoEnc.in[%d] get param[%08x] = %d\r\n", nPort-ISF_IN_BASE, param, value);
			break;
		}
		return ISF_ERR_NOT_SUPPORT;
	}

	return value;
}

static ISF_RV ISF_VdoEnc_SetPortStruct(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param, UINT32* p_struct, UINT32 size)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	ISF_RV ret = ISF_OK;
	UINT32 value = (UINT32)p_struct;

	if (!p_ctx_c->g_bisf_vdoenc_addunit) {
		p_ctx_c->g_bisf_vdoenc_addunit = TRUE;
		NMR_VdoEnc_AddUnit();
		_ISF_VdoEnc_InstallCmd();
		_isf_vdoenc_bs_data_init();
	}

	if (!p_ctx_c->g_bisf_imgcap_addunit) {
		p_ctx_c->g_bisf_imgcap_addunit = TRUE;
		NMR_ImgCap_AddUnit();
	}

/*
	if (nPort >= ISF_OUT_MAX) {
		DBG_ERR("Set Output Port Param only!\r\n");
		return ISF_ERR_FAILED;
	}

	if (nPort >= ISF_VDOENC_OUT_NUM) {
		DBG_ERR("Set Output Port Param only!\r\n");
		return ISF_ERR_FAILED;
	}
*/
	if (!pNMI_VdoEnc) {
		if ((pNMI_VdoEnc = NMI_GetUnit("VdoEnc")) == NULL) {
			DBG_ERR("Get VdoEnc unit failed\r\n");
			return ISF_ERR_NULL_OBJECT;
		}
	}

	if (g_ve_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

    if (nPort == ISF_CTRL)
    {
		switch (param) {
		case VDOENC_PARAM_POLL_LIST_BS:
			{
				UINT32 path_id;
				VDOENC_POLL_LIST_BS_INFO *p_poll_info = (VDOENC_POLL_LIST_BS_INFO *)value;

				memcpy(&p_ctx_c->g_poll_bs_info, (void *)p_poll_info, sizeof(VDOENC_POLL_LIST_BS_INFO));
				ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, pThisUnit, ISF_CTRL, "[POLL_BS] Set POLL_LIST, path_mask(0x%08x), timeout(%d)", p_ctx_c->g_poll_bs_info.path_mask, p_ctx_c->g_poll_bs_info.wait_ms);

				// [1] reset event mask & reset semaphore
				p_ctx_c->g_isf_vdoenc_poll_bs_event_mask = 0;
				while (SEM_WAIT_TIMEOUT(p_ctx_c->ISF_VDOENC_POLL_BS_SEM_ID, vos_util_msec_to_tick(0)) == 0) ;  // consume all semaphore count

				// [2] check if any path already has data available, if so, return NOW.
				for (path_id=0; path_id<PATH_MAX_COUNT; path_id++) {
					if (p_ctx_c->g_poll_bs_info.path_mask & (1<<path_id)) {
						// have to check this path (by user given check mask)
						p_ctx_c->g_isf_vdoenc_poll_bs_event_mask |= (_ISF_VdoEnc_isEmpty_PullQueue(path_id)? (0):(1<<path_id));
					}
				}
				if (p_ctx_c->g_isf_vdoenc_poll_bs_event_mask > 0) { // any check path has bitstream available in PullQ
					ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, pThisUnit, ISF_CTRL, "[POLL_BS] already has bs available, mask = %08x", p_ctx_c->g_isf_vdoenc_poll_bs_event_mask);
					return ISF_OK;
				}

				// [3] if all path is stopped, just return
				if (_isf_vdoenc_is_allpath_stopped() == TRUE) {
					ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, pThisUnit, ISF_CTRL, "[POLL_BS] all path stopped, ignore poll_list !!");
					return ISF_ERR_IGNORE;
				}

				// [4] NO bs available => have to semaphore wait for bs
				if (p_ctx_c->g_poll_bs_info.wait_ms< 0) {
					// blocking (wait until bs available)
					if (SEM_WAIT_INTERRUPTIBLE(p_ctx_c->ISF_VDOENC_POLL_BS_SEM_ID)) {
						ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, pThisUnit, ISF_CTRL, "[POLL_BS] wait bs ignore !!");
						return ISF_ERR_IGNORE;
					}
				} else  {
					// non-blocking (wait_ms=0) , timeout (wait_ms > 0)
					if (SEM_WAIT_TIMEOUT(p_ctx_c->ISF_VDOENC_POLL_BS_SEM_ID, vos_util_msec_to_tick(p_ctx_c->g_poll_bs_info.wait_ms))) {
						ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, pThisUnit, ISF_CTRL, "[POLL_BS] wait bs timeout(%d).....", p_ctx_c->g_poll_bs_info.wait_ms);
						return ISF_ERR_WAIT_TIMEOUT;
					}
				}

				// [5] wait bs success(or auto-unlock) => update event mask again
				for (path_id=0; path_id<PATH_MAX_COUNT; path_id++) {
					if (p_ctx_c->g_poll_bs_info.path_mask & (1<<path_id)) {
						// have to check this path (by user given check mask)
						p_ctx_c->g_isf_vdoenc_poll_bs_event_mask |= (_ISF_VdoEnc_isEmpty_PullQueue(path_id)? (0):(1<<path_id));
					}
				}
				if (p_ctx_c->g_isf_vdoenc_poll_bs_event_mask > 0) { // any check path has bitstream available in PullQ
					ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, pThisUnit, ISF_CTRL, "[POLL_BS] wait bs success, mask = 0x%08x !!", p_ctx_c->g_isf_vdoenc_poll_bs_event_mask);
					return ISF_OK;
				} else {
					ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, pThisUnit, ISF_CTRL, "[POLL_BS] auto-unlock poll_list !!");
					return ISF_ERR_IGNORE;
				}
			}
			break;
		default:
			DBG_ERR("VdoEnc.ctrl set struct[0x%08x] = %08x\r\n", param, value);
			break;
		}

		return ret;
    } // if (nPort == ISF_CTRL)


	if ((nPort < (int)ISF_IN_BASE) && (nPort < pThisUnit->port_out_count)) {
		VDOENC_CONTEXT_PORT   *p_ctx_p = NULL;
		if (nPort-ISF_OUT_BASE >= PATH_MAX_COUNT) {
			DBG_ERR("[VDOENC] invalid path index = %d\r\n", nPort-ISF_OUT_BASE);
			return ISF_ERR_INVALID_PORT_ID;
		}
		p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[nPort-ISF_OUT_BASE];
#if 1
	switch (param) {
	case VDOENC_PARAM_ROI:
		{
			MP_VDOENC_ROI_INFO* pROI = (MP_VDOENC_ROI_INFO*) value;
			int i=0;
			for (i=0; i<DAL_VDOENC_ROI_NUM_MAX; i++) {
				if (pROI->st_roi[i].enable == 1) {
					ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "set ROI, roi[%d] = QP(%d), Mode(%u), win(%u, %u, %u, %u)",
						i,
						pROI->st_roi[i].qp,
						pROI->st_roi[i].qp_mode,
						pROI->st_roi[i].coord_X,
						pROI->st_roi[i].coord_Y,
						pROI->st_roi[i].width,
						pROI->st_roi[i].height);
				}
			}
		}
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_ROI, value);
		break;

	case VDOENC_PARAM_MAX_MEM_INFO:
		{
			ISF_RV r = ISF_OK;
			NMI_VDOENC_MAX_MEM_INFO* pMaxMemInfo = (NMI_VDOENC_MAX_MEM_INFO*) value;

			if (nPort < PATH_MAX_COUNT) {
				if (pMaxMemInfo->bRelease) {
					if ((p_ctx_p->gISF_VdoEncMaxMem.Addr != 0) && (p_ctx_p->gISF_VdoEncMaxMemBlk.h_data != 0)) {
						ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, pThisUnit, ISF_OUT(nPort), "(release max memory)");
						r = pThisUnit->p_base->do_release_i(pThisUnit, &(p_ctx_p->gISF_VdoEncMaxMemBlk), ISF_OK);
						if (r == ISF_OK) {
							p_ctx_p->gISF_VdoEncMaxMem.Addr = 0;
							p_ctx_p->gISF_VdoEncMaxMem.Size = 0;
							p_ctx_p->gISF_VdoEncMaxMemSize = 0;
							memset(&p_ctx_p->gISF_VdoEncMaxMemBlk, 0, sizeof(ISF_DATA));
						} else {
							DBG_ERR("[ISF_VDOENC][%d] release VdoEnc Max blk failed, ret = %d\r\n", nPort, r);
						}
					}
				} else {
					if (p_ctx_p->gISF_VdoEncMaxMem.Addr == 0) {
						pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_COLMV_ENABLE, pMaxMemInfo->bColmvEn);
						pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_COMM_RECFRM_ENABLE, pMaxMemInfo->bCommRecFrmEn);
						pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_QUALITY_BASE_MODE_ENABLE, pMaxMemInfo->bQualityBaseMode);
						if (pMaxMemInfo->bCommRecFrmEn) {
							p_ctx_p->gISF_VdoEncRecFrmCfgSVCLayer = pMaxMemInfo->uiSVCLayer;
							p_ctx_p->gISF_VdoEncRecFrmCfgLTRInterval = pMaxMemInfo->uiLTRInterval;
						}
						pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_MAX_MEM_INFO, (UINT32)pMaxMemInfo);

						if (pMaxMemInfo->bCommSnapShot) {
							p_ctx_p->gISF_VdoEncMaxMemSize = pMaxMemInfo->uiCodecsize + pMaxMemInfo->uiEncsize;
							p_ctx_p->gISF_VdoEncSnapShotSize = pMaxMemInfo->uiSnapShotSize;
						} else {
							p_ctx_p->gISF_VdoEncMaxMemSize = pMaxMemInfo->uiSnapShotSize + pMaxMemInfo->uiCodecsize + pMaxMemInfo->uiEncsize;
						}
					} else {
						DBG_ERR("[ISF_VDOENC][%d] max buf exist, please release it first.\r\n", nPort);
					}
				}
			} else {
				DBG_ERR("[ISF_VDOENC] invalid port index = %d\r\n", nPort);
			}
		}
		break;

	case VDOENC_PARAM_LTR:
		{
			MP_VDOENC_LTR_INFO *p_ltr = (MP_VDOENC_LTR_INFO *)value;
#ifdef __KERNEL__
		if (nPort < BUILTIN_VDOENC_PATH_ID_MAX) {
			if (p_ctx_c->bIs_fastboot_enc[nPort]) {
				if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[nPort]) {
					VdoEnc_Builtin_GetParam(nPort, BUILTIN_VDOENC_INIT_PARAM_LTR_INTERVAL, &p_ltr->uiLTRInterval);
					VdoEnc_Builtin_GetParam(nPort, BUILTIN_VDOENC_INIT_PARAM_LTR_PRE_REF, &p_ltr->uiLTRPreRef);
				}
			}
		}
#endif
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set LTR = (%u, %u)", p_ltr->uiLTRInterval, p_ltr->uiLTRPreRef);
		}
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_LTR, value);
		break;

	case VDOENC_PARAM_CBRINFO:
		{
			MP_VDOENC_CBR_INFO *pCbrInfo = (MP_VDOENC_CBR_INFO *)value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set CBR Enable = %d, StaticTime = %d, ByteRate = %d, Fr = %u.%03u, GOP = %d, IQp = (%d, %d, %d), PQp = (%d, %d, %d), IPWei = %d, KP_Prd = %d, KP_Wei = %d, P2_Wei = %d, P3_Wei = %d, LT_Wei = %d, mas = %d, max_size = %d",
				 pCbrInfo->enable,
				 pCbrInfo->static_time,
				 pCbrInfo->byte_rate,
				 (pCbrInfo->frame_rate/1000),
				 (pCbrInfo->frame_rate%1000),
				 pCbrInfo->gop,
				 pCbrInfo->init_i_qp,
				 pCbrInfo->min_i_qp,
				 pCbrInfo->max_i_qp,
				 pCbrInfo->init_p_qp,
				 pCbrInfo->min_p_qp,
				 pCbrInfo->max_p_qp,
				 pCbrInfo->ip_weight,
				 pCbrInfo->key_p_period,
				 pCbrInfo->kp_weight,
				 pCbrInfo->p2_weight,
				 pCbrInfo->p3_weight,
				 pCbrInfo->lt_weight,
				 pCbrInfo->motion_aq_str,
				 pCbrInfo->max_frame_size);
		}
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_CBRINFO, value);
		break;

	case VDOENC_PARAM_EVBRINFO:
		{
			MP_VDOENC_EVBR_INFO *pEVbrInfo = (MP_VDOENC_EVBR_INFO *)value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set EVBR Enable = %d, StaticTime = %d, ByteRate = %d, Fr = %u.%03u, GOP = %d, KeyPPeriod = %d, IQp = (%d, %d, %d), PQp = (%d, %d, %d), IPWei = %d, KPWei = %d, MotionAQ = %d, StillFr = %d, MotionR = %d, Psnr = (%d, %d, %d), P2_Wei = %d, P3_Wei = %d, LT_Wei = %d, max_size = %d",
				pEVbrInfo->enable,
				pEVbrInfo->static_time,
				pEVbrInfo->byte_rate,
				(pEVbrInfo->frame_rate/1000),
				(pEVbrInfo->frame_rate%1000),
				pEVbrInfo->gop,
				pEVbrInfo->key_p_period,
				pEVbrInfo->init_i_qp,
				pEVbrInfo->min_i_qp,
				pEVbrInfo->max_i_qp,
				pEVbrInfo->init_p_qp,
				pEVbrInfo->min_p_qp,
				pEVbrInfo->max_p_qp,
				pEVbrInfo->ip_weight,
				pEVbrInfo->kp_weight,
				pEVbrInfo->motion_aq_st,
				pEVbrInfo->still_frm_cnd,
				pEVbrInfo->motion_ratio_thd,
				pEVbrInfo->i_psnr_cnd,
				pEVbrInfo->p_psnr_cnd,
				pEVbrInfo->kp_psnr_cnd,
				pEVbrInfo->p2_weight,
				pEVbrInfo->p3_weight,
				pEVbrInfo->lt_weight,
				pEVbrInfo->max_frame_size);
		}
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_EVBRINFO, value);
		break;

	case VDOENC_PARAM_VBRINFO:
		{
			MP_VDOENC_VBR_INFO *pVbrInfo = (MP_VDOENC_VBR_INFO *)value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set VBR Enable = %d, StaticTime = %d, ByteRate = %d, Fr = %u.%03u, GOP = %d, IQp = (%d, %d, %d), PQp = (%d, %d, %d), IPWei = %d, ChangePos = %d, KP_Prd = %d, KP_Wei = %d, P2_Wei = %d, P3_Wei = %d, LT_Wei = %d, mas = %d, max_size = %d",
				pVbrInfo->enable,
				pVbrInfo->static_time,
				pVbrInfo->byte_rate,
				(pVbrInfo->frame_rate/1000),
				(pVbrInfo->frame_rate%1000),
				pVbrInfo->gop,
				pVbrInfo->init_i_qp,
				pVbrInfo->min_i_qp,
				pVbrInfo->max_i_qp,
				pVbrInfo->init_p_qp,
				pVbrInfo->min_p_qp,
				pVbrInfo->max_p_qp,
				pVbrInfo->ip_weight,
				pVbrInfo->change_pos,
				pVbrInfo->key_p_period,
				pVbrInfo->kp_weight,
				pVbrInfo->p2_weight,
				pVbrInfo->p3_weight,
				pVbrInfo->lt_weight,
				pVbrInfo->motion_aq_str,
				pVbrInfo->max_frame_size);
		}
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_VBRINFO, value);
		break;

	case VDOENC_PARAM_FIXQPINFO:
		{
			MP_VDOENC_FIXQP_INFO *pFixQpInfo = (MP_VDOENC_FIXQP_INFO *)value;
#ifdef __KERNEL__
			if (nPort < BUILTIN_VDOENC_PATH_ID_MAX) {
				if (p_ctx_c->bIs_fastboot_enc[nPort]) {
					if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[nPort]) {
						VdoEnc_Builtin_GetParam(nPort, BUILTIN_VDOENC_INIT_PARAM_FRAME_RATE, &pFixQpInfo->frame_rate);
					}
				}
			}
#endif
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set FIXQP Enable = %d, Fr = %u.%03u, IFixQP = %d, PFixQP = %d",
				 pFixQpInfo->enable,
				 (pFixQpInfo->frame_rate/1000),
				 (pFixQpInfo->frame_rate%1000),
				 pFixQpInfo->fix_i_qp,
				 pFixQpInfo->fix_p_qp);
		}
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_FIXQPINFO, value);
		break;

	case VDOENC_PARAM_ROWRC:
		{
			MP_VDOENC_ROWRC_INFO *pRowRcInfo = (MP_VDOENC_ROWRC_INFO *)value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set RowRC, en = %u, (range, step, min, max)=> I = (%u, %u, %u, %u), P = (%u, %u, %u, %u)",
				pRowRcInfo->enable,
				pRowRcInfo->i_qp_range,
				pRowRcInfo->i_qp_step,
				pRowRcInfo->min_i_qp,
				pRowRcInfo->max_i_qp,
				pRowRcInfo->p_qp_range,
				pRowRcInfo->p_qp_step,
				pRowRcInfo->min_p_qp,
				pRowRcInfo->max_p_qp);
		}
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_ROWRC, value);
		break;

	case VDOENC_PARAM_AQINFO:
		{
			MP_VDOENC_AQ_INFO *pAQInfo = (MP_VDOENC_AQ_INFO *)value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set AQ = (%d, %d, %d, %d, %d)",
				pAQInfo->enable,
				pAQInfo->i_str,
				pAQInfo->p_str,
				pAQInfo->max_delta_qp,
				pAQInfo->min_delta_qp);
		}
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_AQINFO, value);
		break;

	case VDOENC_PARAM_DEBLOCK:
		{
			MP_VDOENC_INIT *pInfo = (MP_VDOENC_INIT *)value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set DEBLOCK = (%u, %d, %d)",
				pInfo->disable_db,
				pInfo->db_alpha,
				pInfo->db_beta);
		}
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_DEBLOCK, value);
		break;

	case VDOENC_PARAM_VUI:
		{
			MP_VDOENC_INIT *pInfo = (MP_VDOENC_INIT *)value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set VUI = (%d, %d, %d, %d, %d, %d, %d, %d, %d)",
				pInfo->bVUIEn,
				pInfo->sar_width,
				pInfo->sar_height,
				pInfo->matrix_coef,
				pInfo->transfer_characteristics,
				pInfo->colour_primaries,
				pInfo->video_format,
				pInfo->color_range,
				pInfo->time_present_flag);
		}
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_VUI, value);
		break;

	case VDOENC_PARAM_GDR:
		{
			MP_VDOENC_GDR_INFO *pGdrInfo = (MP_VDOENC_GDR_INFO *)value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set GDR = (%d, %d, %d, %d, %d, %d)",
				pGdrInfo->enable,
				pGdrInfo->period,
				pGdrInfo->number,
				pGdrInfo->enable_gdr_i_frm,
				pGdrInfo->enable_gdr_qp,
				pGdrInfo->gdr_qp);
		}
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_GDR, value);
		break;

	case VDOENC_PARAM_SLICE_SPLIT:
		{
			MP_VDOENC_SLICESPLIT_INFO *pSliceInfo = (MP_VDOENC_SLICESPLIT_INFO *)value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set SLICE = (%d, %d)",
				pSliceInfo->enable,
				pSliceInfo->slice_row_num);
		}
		pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_SLICE_SPLIT, value);
		break;

	case VDOENC_PARAM_USR_QP_MAP:
		{
			// Now only linux HDAL will call SetPortStruct...so the addr is physical addr -> convert to virtual addr
			// TODO : how about uItron later ??
			MP_VDOENC_QPMAP_INFO *qpmap = (MP_VDOENC_QPMAP_INFO *)value;

			if (qpmap->enable == 1) {
				qpmap->qp_map_addr = (UINT8 *)nvtmpp_sys_pa2va((UINT32)qpmap->qp_map_addr);
			}

			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set QP_MAP = (%d, 0x%08x, %d, %d)",
				qpmap->enable,
				(UINT32)qpmap->qp_map_addr,
				qpmap->qp_map_size,
				qpmap->qp_map_loft);

			pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_USR_QP_MAP, value);  // Enable/Disable User QP Map, and QP Map buffer pointer
		}
		break;

	case VDOENC_PARAM_RDO:
		{
			MP_VDOENC_RDO_INFO *pRdoInfo = (MP_VDOENC_RDO_INFO *)value;
			if (pRdoInfo->rdo_codec == VDOENC_RDO_CODEC_264) {
				ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set RDO (H264), intra[4x4(%u) 8x8(%u) 16x16(%u)], inter[tu4(%u) tu8(%u) skip(%u)]\r\n", pRdoInfo->rdo_param.rdo_264.avc_intra_4x4_cost_bias, pRdoInfo->rdo_param.rdo_264.avc_intra_8x8_cost_bias, pRdoInfo->rdo_param.rdo_264.avc_intra_16x16_cost_bias, pRdoInfo->rdo_param.rdo_264.avc_inter_tu4_cost_bias, pRdoInfo->rdo_param.rdo_264.avc_inter_tu8_cost_bias, pRdoInfo->rdo_param.rdo_264.avc_inter_skip_cost_bias);
			} else if (pRdoInfo->rdo_codec == VDOENC_RDO_CODEC_265) {
				ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set RDO (H265), intra[32x32(%u) 16x16(%u) 8x8(%u)], inter[skip(%d) merge(%d) 64x64(%u) 64x32_32x64(%u) 32x32(%u) 32x16_16x32(%u) 16x16(%u)]\r\n", pRdoInfo->rdo_param.rdo_265.hevc_intra_32x32_cost_bias, pRdoInfo->rdo_param.rdo_265.hevc_intra_16x16_cost_bias, pRdoInfo->rdo_param.rdo_265.hevc_intra_8x8_cost_bias, pRdoInfo->rdo_param.rdo_265.hevc_inter_skip_cost_bias, pRdoInfo->rdo_param.rdo_265.hevc_inter_merge_cost_bias, pRdoInfo->rdo_param.rdo_265.hevc_inter_64x64_cost_bias, pRdoInfo->rdo_param.rdo_265.hevc_inter_64x32_32x64_cost_bias, pRdoInfo->rdo_param.rdo_265.hevc_inter_32x32_cost_bias, pRdoInfo->rdo_param.rdo_265.hevc_inter_32x16_16x32_cost_bias, pRdoInfo->rdo_param.rdo_265.hevc_inter_16x16_cost_bias);
			} else {
				return ISF_OK; // not set
			}

			pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_RDO, value);
		}
		break;

	case VDOENC_PARAM_JND:
		{
			MP_VDOENC_JND_INFO *pJndInfo = (MP_VDOENC_JND_INFO *)value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set JND = en(%u), str(%u), level(%d), threashold(%u)\r\n", pJndInfo->enable, pJndInfo->str, pJndInfo->level, pJndInfo->threshold);
			pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_JND, value);
		}
		break;

    case VDOENC_PARAM_SNAPSHOT_PURE_LINUX:  // setportstruct to trigger, getportparam to get size
		{
			VDOENC_SNAP_INFO *snap_info = (VDOENC_SNAP_INFO *)value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "[snapshot] user provide pa = 0x%08x, size = %d, quality = %d", snap_info->phyaddr, snap_info->size, snap_info->image_quality);

			p_ctx_p->gISF_VdoEnc_SnapAddr    = nvtmpp_sys_pa2va(snap_info->phyaddr);  // convert user bs buffer , phy -> virt
			p_ctx_p->gISF_VdoEnc_SnapSize    = snap_info->size; // user provided buffer size
			p_ctx_p->gISF_VdoEnc_SnapQuality = snap_info->image_quality; // user provided image quality

			if (!p_ctx_p->gISF_VdoEncCommSrcOut_NoJPGEnc) {
				if (p_ctx_p->gISF_VdoEnc_SnapAddr == 0) {
					DBG_ERR("given phyaddr is invalid !! \r\n");
					return ISF_ERR_INVALID_PARAM;
				}
			}
			//TODO: check user buffer size available
#if 0
			if (SEM_WAIT_INTERRUPTIBLE(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_IDLE)) return FALSE; // wait source out YUV buffer is idle for this path (prevent YUV overwrite)
#else
			if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_IDLE, vos_util_msec_to_tick(2000))) {
				DBG_IND("[ISF_VDOENC][%d] snapshot timeout...\r\n", nPort);
				ret = ISF_ERR_WAIT_TIMEOUT;
			}
#endif
			while (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_DONE, vos_util_msec_to_tick(0)) == 0) ;  // consume all semaphore count
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "[snapshot] start");
			pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_SNAPSHOT, 1);
#if 0
			SEM_WAIT_INTERRUPTIBLE(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_DONE); // wait until JPEG encode is done
#else
			if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_DONE, vos_util_msec_to_tick(2000))) {
				DBG_IND("[ISF_VDOENC][%d] snapshot timeout...\r\n", nPort);
				ret = ISF_ERR_WAIT_TIMEOUT;
			}
#endif
			if (!p_ctx_p->gISF_VdoEncCommSrcOut_NoJPGEnc) {
				if (p_ctx_p->gISF_VdoEnc_SnapSize == 0) {   // jpeg encode failed, return error for isf_flow's  ioctl(set)
					ret = ISF_ERR_PROCESS_FAIL;
				}
			}
			//DBG_DUMP("[BB] snapshot wait done.\r\n");
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "[snapshot] done");
#if 0
			if (snap_info->timeout < 0) {
				// blocking (wait until data available)
				if (SEM_WAIT_INTERRUPTIBLE(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_DONE)) return FALSE;
			} else  {
				// non-blocking (wait_ms=0) , timeout (wait_ms > 0). If success PULLQ - 1 , else just return FALSE
				if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_DONE, vos_util_msec_to_tick(snap_info->timeout))) {
					return FALSE;
				}
			}
#endif
        }
        break;

	case VDOENC_PARAM_REQ_TARGET_I:
		{
			NMI_VDOENC_REQ_TARGET_I_INFO *pReq_i = (NMI_VDOENC_REQ_TARGET_I_INFO *)value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set REQ_TARGET_I = target_timestamp(%llu)\r\n", pReq_i->target_timestamp);

			pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_REQ_TARGET_I, (UINT32)pReq_i);
		}
		break;

	case VDOENC_PARAM_H26X_CROP:
		{
			MP_VDOENC_FRM_CROP_INFO *pH26XCrop = (MP_VDOENC_FRM_CROP_INFO *)value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set H26X_CROP = enable(%u), display width(%u), display height(%u)\r\n", pH26XCrop->frm_crop_enable, pH26XCrop->display_w, pH26XCrop->display_h);
			pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_H26X_CROP, value);
		}
		break;

	case VDOENC_PARAM_H26X_SKIP_FRM:
		{
			NMI_VDOENC_H26X_SKIP_FRM_INFO *pSkipFrm = (NMI_VDOENC_H26X_SKIP_FRM_INFO *)value;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set SKIP_FRM = enable(%u), target_fr(%u), input_frm_cnt(%u)\r\n", pSkipFrm->enable, pSkipFrm->target_fr, pSkipFrm->input_frm_cnt);

			pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_H26X_SKIP_FRM, (UINT32)pSkipFrm);
		}
		break;

	case VDOENC_PARAM_META_ALLOC:
		{
			NMI_VDOENC_META_ALLOC_INFO *p_user_meta_alloc = (NMI_VDOENC_META_ALLOC_INFO *)value;
			NMI_VDOENC_META_ALLOC_INFO *p_venc_meta_alloc = NULL;
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS_IMM, pThisUnit, ISF_OUT(nPort), "Set META_ALLOC = value(0x%lx)\r\n", (unsigned long)p_user_meta_alloc);
			if (p_user_meta_alloc) {
				UINT32 num = size / sizeof(NMI_VDOENC_META_ALLOC_INFO);
				if (p_ctx_p->g_p_vdoenc_meta_alloc_info == NULL) {
					UINT32 buf_addr = 0;
					UINT32 buf_size = size;
					// alloc meta context of port
					buf_addr = (&isf_vdoenc)->p_base->do_new_i(&isf_vdoenc, NULL, "meta", buf_size, &(p_ctx_p->gISF_VdoEncMetaAllocBlk));
					if (buf_addr == 0) {
						DBG_ERR("[ISF_VDOENC] alloc meta memory fail !!\r\n");
					} else {
						// reset memory
						memset((void *)buf_addr, 0, buf_size);
						p_ctx_p->gISF_VdoEncMetaAllocAddr = buf_addr;
						p_ctx_p->gISF_VdoEncMetaAllocSize = buf_size;
						p_ctx_p->g_p_vdoenc_meta_alloc_info = (NMI_VDOENC_META_ALLOC_INFO *)buf_addr;
					}
				}
				if (p_ctx_p->g_p_vdoenc_meta_alloc_info) {
					if (p_ctx_p->gISF_VdoEncMetaAllocSize != size) {
						DBG_ERR("[ISF_VDOENC] meta memory 0x%x, should be 0x%x!!\r\n", size, p_ctx_p->gISF_VdoEncMetaAllocSize);
						return ISF_ERR_IGNORE;
					}
					memcpy((void *)p_ctx_p->g_p_vdoenc_meta_alloc_info, (void *)p_user_meta_alloc, size);
					// arrange meta p_next of port
					for (num = 0; num < size / sizeof(NMI_VDOENC_META_ALLOC_INFO); num++) {
						p_venc_meta_alloc = (NMI_VDOENC_META_ALLOC_INFO *)&p_ctx_p->g_p_vdoenc_meta_alloc_info[num];
						if (p_venc_meta_alloc->p_next) {
							p_venc_meta_alloc->p_next = (NMI_VDOENC_META_ALLOC_INFO *)&p_ctx_p->g_p_vdoenc_meta_alloc_info[num+1];
						}
					}
				}
				p_venc_meta_alloc = p_ctx_p->g_p_vdoenc_meta_alloc_info;
				if (p_venc_meta_alloc) {
					pNMI_VdoEnc->SetParam(nPort, NMI_VDOENC_PARAM_META_ALLOC, (UINT32)p_venc_meta_alloc);
				}
			}
		}
		break;

	default:
		DBG_ERR("VdoEnc.out[%d] set struct[%08x] = %08x\r\n", nPort-ISF_OUT_BASE, param, value);
		ret = ISF_ERR_NOT_SUPPORT;
		break;
	}

    return ret;
#endif
	} else if (nPort-ISF_IN_BASE < pThisUnit->port_in_count) {
		if (nPort-ISF_IN_BASE >= PATH_MAX_COUNT) {
			DBG_ERR("[VDOENC] invalid path index = %d\r\n", nPort-ISF_IN_BASE);
			return ISF_ERR_INVALID_PORT_ID;
		}
#if 0
		VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[nPort-ISF_IN_BASE];

		switch (param) {
		case ISF_PARAM_PORT_DIRTY:
	        if (value & ISF_INFO_DIRTY_IMGSIZE)
	            return ISF_APPLY_DOWNSTREAM_OFFTIME;
	        if (value & ISF_INFO_DIRTY_FRAMERATE)
	            return ISF_APPLY_DOWNSTREAM_OFFTIME;
	        if (value & ISF_INFO_DIRTY_DIRECT)
	            return ISF_APPLY_DOWNSTREAM_OFFTIME;
	        if (value & ISF_INFO_DIRTY_BUFCOUNT)
	            return ISF_APPLY_DOWNSTREAM_OFFTIME;
	    		break;

		case VDOENC_PARAM_SYNCSRC:
			p_ctx_p->ISF_VdoSrcUnit = (ISF_UNIT*)value;
			p_ctx_p->ISF_VdoSrcPort = 0;
			return ISF_OK;
			break;

		default:
			DBG_ERR("VdoEnc.in[%d] set struct[%08x] = %d\r\n", nPort-ISF_IN_BASE, param, value);
			break;
		}
#endif
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV ISF_VdoEnc_GetPortStruct(struct _ISF_UNIT *pThisUnit, UINT32 nPort, UINT32 param, UINT32* p_struct, UINT32 size)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	ISF_RV ret = ISF_OK;
	MP_VDOENC_ROI_INFO roi_info = {0};
	MP_VDOENC_LTR_INFO ltr_info = {0};
	MP_VDOENC_ROWRC_INFO rowrc_info = {0};
	MP_VDOENC_AQ_INFO aq_info = {0};
	VDOENC_SNAP_INFO snap_info = {0};
	MP_VDOENC_HW_LMT hw_limit_info = {0};

	/*
	if (nPort >= ISF_OUT_MAX) {
		DBG_ERR("Get Output Port Param only!\r\n");
		return value;
	}
*/

	if (!p_ctx_c->g_bisf_vdoenc_addunit) {
		p_ctx_c->g_bisf_vdoenc_addunit = TRUE;
		NMR_VdoEnc_AddUnit();
		_ISF_VdoEnc_InstallCmd();
		_isf_vdoenc_bs_data_init();
	}

	if (!pNMI_VdoEnc) {
		if ((pNMI_VdoEnc = NMI_GetUnit("VdoEnc")) == NULL) {
			DBG_ERR("Get VdoEnc unit failed\r\n");
			return ISF_ERR_NULL_OBJECT;
		}
	}

	if (g_ve_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

	if ((nPort < (int)ISF_IN_BASE) && (nPort < pThisUnit->port_out_count)) {
		VDOENC_CONTEXT_PORT   *p_ctx_p = NULL;
		if (nPort-ISF_OUT_BASE >= PATH_MAX_COUNT) {
			DBG_ERR("[VDOENC] invalid path index = %d\r\n", nPort-ISF_OUT_BASE);
			return ISF_ERR_INVALID_PORT_ID;
		}
		p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[nPort - ISF_OUT_BASE];

		if (nPort-ISF_OUT_BASE >= PATH_MAX_COUNT) {
			DBG_ERR("[VDOENC] invalid path index = %d\r\n", nPort-ISF_OUT_BASE);
			return ISF_ERR_INVALID_PORT_ID;
		}
		switch (param) {
		case VDOENC_PARAM_HW_LIMIT:
			if (size != sizeof(MP_VDOENC_HW_LMT))
				DBG_WRN("HW_LMT size(%d) != %d\r\n", size, sizeof(MP_VDOENC_HW_LMT));

			pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_HW_LIMIT, (UINT32 *)&hw_limit_info);
			memcpy((void *)p_struct, (void *)&hw_limit_info, sizeof(MP_VDOENC_HW_LMT));
			ISF_UNIT_TRACE(ISF_OP_PARAM_VDOBS, pThisUnit, ISF_OUT(nPort), "hw_limit => h264(%d,%d,%d,%d) h265(%d,%d,%d,%d) jpeg(%d,%d,%d,%d)", hw_limit_info.h264e.min_width, hw_limit_info.h264e.min_height, hw_limit_info.h264e.max_width, hw_limit_info.h264e.max_height, hw_limit_info.h265e.min_width, hw_limit_info.h265e.min_height, hw_limit_info.h265e.max_width, hw_limit_info.h265e.max_height, hw_limit_info.jpege.min_width, hw_limit_info.jpege.min_height, hw_limit_info.jpege.max_width, hw_limit_info.jpege.max_height);
			break;

		case VDOENC_PARAM_ROI:
			if (size != sizeof(MP_VDOENC_ROI_INFO))
				DBG_WRN("ROI size(%d) != %d\r\n", size, sizeof(MP_VDOENC_ROI_INFO));

			pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_ROI, (UINT32 *)&roi_info);
			memcpy((void *)p_struct, (void *)&roi_info, sizeof(MP_VDOENC_ROI_INFO));
			break;

		case VDOENC_PARAM_LTR:
			if (size != sizeof(MP_VDOENC_LTR_INFO))
				DBG_WRN("LTR size(%d) != %d\r\n", size, sizeof(MP_VDOENC_LTR_INFO));

			pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_LTR, (UINT32 *)&ltr_info);
			memcpy((void *)p_struct, (void *)&ltr_info, sizeof(MP_VDOENC_LTR_INFO));
			break;

		case VDOENC_PARAM_AQINFO:
			if (size != sizeof(MP_VDOENC_AQ_INFO))
				DBG_WRN("AQINFO size(%d) != %d\r\n", size, sizeof(MP_VDOENC_AQ_INFO));

			pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_AQINFO, (UINT32 *)&aq_info);
			memcpy((void *)p_struct, (void *)&aq_info, sizeof(MP_VDOENC_AQ_INFO));
			break;

		case VDOENC_PARAM_ROWRC:
			if (size != sizeof(MP_VDOENC_ROWRC_INFO))
				DBG_WRN("ROWRC size(%d) != %d\r\n", size, sizeof(MP_VDOENC_ROWRC_INFO));

			pNMI_VdoEnc->GetParam(nPort, NMI_VDOENC_PARAM_ROWRC, (UINT32 *)&rowrc_info);
			memcpy((void *)p_struct, (void *)&rowrc_info, sizeof(MP_VDOENC_ROWRC_INFO));
			break;

		case VDOENC_PARAM_SNAPSHOT_PURE_LINUX:
			if (size != sizeof(VDOENC_SNAP_INFO))
				DBG_WRN("SNAPSHOT size(%d) != %d\r\n", size, sizeof(VDOENC_SNAP_INFO));

			snap_info.size      = p_ctx_p->gISF_VdoEnc_SnapSize;
			snap_info.timestamp = p_ctx_p->gISF_VdoEnc_SnapTimestamp;
			memcpy((void *)p_struct, (void *)&snap_info, sizeof(VDOENC_SNAP_INFO));
			break;

		case VDOENC_PARAM_H26X_DESC: {
				if (p_ctx_p->gISF_VdoEnc_Started == 1) {
					VDOENC_H26X_DESC_INFO *p_desc = (VDOENC_H26X_DESC_INFO *)p_struct;
					PVDO_BITSTREAM        pVStrmBuf   = &p_ctx_p->gISF_VStrmBuf;

					p_desc->vps_paddr = VDOENC_VIRT2PHY(nPort, p_ctx_p->gisf_vdoenc_vps_addr);
					p_desc->vps_size  = pVStrmBuf->resv[5];
					p_desc->sps_paddr = VDOENC_VIRT2PHY(nPort, p_ctx_p->gisf_vdoenc_sps_addr);
					p_desc->sps_size  = pVStrmBuf->resv[1];
					p_desc->pps_paddr = VDOENC_VIRT2PHY(nPort, p_ctx_p->gisf_vdoenc_pps_addr);
					p_desc->pps_size  = pVStrmBuf->resv[3];
				} else {
					DBG_ERR("[ISF_VDOENC][%d] VENC not start, get VDOENC_PARAM_H26X_DESC fail!\r\n", nPort);
					return ISF_ERR_NOT_START;
				}
			}
			break;
		default:
			DBG_ERR("VdoEnc.in[%d] get struct[%08x] not support !!\r\n", nPort-ISF_OUT_BASE, param);
			ret = ISF_ERR_NOT_SUPPORT;
			break;
		}

		return ret;

	} else if (nPort-ISF_IN_BASE < pThisUnit->port_in_count) {
		if (nPort-ISF_IN_BASE >= PATH_MAX_COUNT) {
			DBG_ERR("[VDOENC] invalid path index = %d\r\n", nPort-ISF_IN_BASE);
			return ISF_ERR_INVALID_PORT_ID;
		}
#if 0
		VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[nPort-ISF_IN_BASE];

		switch (param) {
		case VDOENC_PARAM_SYNCSRC:
			return (UINT32)(p_ctx_p->ISF_VdoSrcUnit);
			break;

		default:
			DBG_ERR("VdoEnc.in[%d] get struct[%08x] = %d\r\n", nPort-ISF_IN_BASE, param, value);
			break;
		}
#endif
	}

	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV ISF_VdoEnc_UpdatePort(struct _ISF_UNIT *pThisUnit, UINT32 oPort, ISF_PORT_CMD cmd)
{
	ISF_RV r = ISF_OK;
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	VDOENC_CONTEXT_PORT   *p_ctx_p = NULL;

	if (oPort-ISF_OUT_BASE >= PATH_MAX_COUNT) {
		if (cmd != ISF_PORT_CMD_RESET) DBG_ERR("[VDOENC] invalid path index = %d\r\n", oPort-ISF_OUT_BASE);
		return ISF_ERR_INVALID_PORT_ID;
	}

	if (g_ve_ctx.port == NULL) {
		DBG_ERR("context port not init\r\n");
		return ISF_ERR_UNINIT;
	}

	p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[oPort];

	//[isf_stream] ISF_PORT *pPort = _isf_unit_out(pThisUnit, ISF_OUT(oPort));  //ImageUnit_Out(pThisUnit, oPort);
#if 0
	ISF_PORT *pPort = pThisUnit->p_base->oport(pThisUnit, ISF_OUT(oPort)); // [isf_flow] use this ...
#endif

	if (!p_ctx_c->g_bisf_vdoenc_addunit) {
		p_ctx_c->g_bisf_vdoenc_addunit = TRUE;
		NMR_VdoEnc_AddUnit();
		_ISF_VdoEnc_InstallCmd();
		_isf_vdoenc_bs_data_init();
	}

	if (!p_ctx_c->g_bisf_imgcap_addunit) {
		p_ctx_c->g_bisf_imgcap_addunit = TRUE;
		NMR_ImgCap_AddUnit();
	}

	if (!pNMI_VdoEnc) {
		if ((pNMI_VdoEnc = NMI_GetUnit("VdoEnc")) == NULL) {
			DBG_ERR("Get VdoEnc unit failed\r\n");
			return ISF_ERR_NULL_OBJECT;
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
		_ISF_VdoEnc_Reset(pThisUnit, oPort);
		break;

	case ISF_PORT_CMD_OPEN: {                              ///< off -> ready   (alloc mempool and start task)
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(open)");
			if ((r = _ISF_VdoEnc_Open(pThisUnit, oPort)) != ISF_OK) {
				return r;
			}
			if ((r = _ISF_VdoEnc_ImgCap_Open(pThisUnit)) != ISF_OK) {
				return r;
			}

			p_ctx_p->gISF_VdoEnc_Opened = TRUE;
#ifdef __KERNEL__
			if (p_ctx_c->bIs_fastboot) {
				if (p_ctx_c->bIs_fastboot_enc[oPort] && g_vdoenc_1st_open[oPort]) {
					UINT32 bsdata_blk_max = 0, pullq_max = 0, framerate = 0;
					VdoEnc_Builtin_GetParam(oPort, BUILTIN_VDOENC_INIT_PARAM_BSQ_MAX, &bsdata_blk_max);
					VdoEnc_Builtin_GetParam(oPort, BUILTIN_VDOENC_INIT_PARAM_BSQ_MAX, &pullq_max);
					VdoEnc_Builtin_GetParam(oPort, BUILTIN_VDOENC_INIT_PARAM_FRAME_RATE, &framerate);
					framerate = framerate / 1000;
					bsdata_blk_max = bsdata_blk_max >= 90 ? bsdata_blk_max + (framerate * 2) : ISF_VDOENC_BSDATA_BLK_MAX;
					pullq_max = pullq_max >= 90 ? pullq_max + (framerate * 2) : ISF_VDOENC_PULL_Q_MAX;
					g_vdoenc_fastboot_bsdata_blk_max[oPort] = bsdata_blk_max;
					g_vdoenc_fastboot_pullq_max[oPort] = bsdata_blk_max;
				} else {
					g_vdoenc_fastboot_bsdata_blk_max[oPort] = ISF_VDOENC_BSDATA_BLK_MAX;
					g_vdoenc_fastboot_pullq_max[oPort] = ISF_VDOENC_PULL_Q_MAX;
				}
			} else {
				g_vdoenc_fastboot_bsdata_blk_max[oPort] = ISF_VDOENC_BSDATA_BLK_MAX;
				g_vdoenc_fastboot_pullq_max[oPort] = ISF_VDOENC_PULL_Q_MAX;
			}
#endif
		}
		break;
	case ISF_PORT_CMD_OFFTIME_SYNC: {                       ///< off -> off (off-time property is dirty)
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(off-sync)");
			_ISF_VdoEnc_UpdatePortInfo(pThisUnit, oPort, cmd);
		}
		break;
	case ISF_PORT_CMD_READYTIME_SYNC: {                    ///< ready -> ready (apply ready-time parameter if it is dirty)
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(ready-sync)");
			_ISF_VdoEnc_UpdatePortInfo(pThisUnit, oPort, cmd);
		}
		break;
	case ISF_PORT_CMD_START: {                             ///< ready -> run   (initial context, engine enable and device power on, start data processing)
			UINT32 id = oPort-ISF_OUT_BASE;
			//[isf_stream] ISF_PORT* pSrcPort = _isf_unit_in(&isf_vdoenc, ISF_IN(id));  //ImageUnit_In(&ISF_VdoEnc, id+ISF_IN_BASE);
			ISF_PORT* pSrcPort = isf_vdoenc.p_base->iport(&isf_vdoenc, ISF_IN(id)); // [isf_flow] use this
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(start)");
#if (1)  // [isf_flow] have to do this before START
			////_ISF_VdoEnc_UpdatePortInfo(pThisUnit, oPort); // set image size/format/rotate
			//UINT32 uiBufSize = 0;
			//pNMI_VdoEnc->GetParam(oPort, NMI_VDOENC_PARAM_ALLOC_SIZE, &uiBufSize); // update alloc buffer variables
			//pNMI_VdoEnc->SetParam(oPort, NMI_VDOENC_PARAM_MEM_RANGE, (UINT32) &p_ctx_p->gISF_VdoEncMaxMem);  // [isf_flow] use MAX_MEM way only, set buffer for current settings

			// "first time" START, alloc memory
			{
				if (p_ctx_p->gISF_VdoEncMaxMem.Addr == 0 || p_ctx_p->gISF_VdoEncMaxMem.Size == 0) {
					if (p_ctx_p->gISF_VdoEncMaxMemSize == 0) {   // [isf_flow] use MAX_MEM way only
						DBG_ERR("[ISF_VDOENC][%d] Fatal Error, please set MAX_MEM first !!\r\n", oPort);
						return ISF_ERR_NO_CONFIG;
					}

					ISF_UNIT_TRACE(ISF_OP_PARAM_GENERAL, pThisUnit, oPort, "(alloc max memory)");

					if ((r = _ISF_VdoEnc_AllocMem(pThisUnit, oPort)) != ISF_OK) {
						DBG_ERR("[ISF_VDOENC][%d] AllocMem failed\r\n", oPort);
						p_ctx_p->gISF_VdoEnc_Is_AllocMem = FALSE;
						return r;
					}else{
						p_ctx_p->gISF_VdoEnc_Is_AllocMem = TRUE;
					}
				}
			}
#endif
			_ISF_VdoEnc_OutputPort_Enable(id);
			if(pSrcPort->connecttype == (ISF_CONNECT_PUSH|ISF_CONNECT_SYNC)) {
				p_ctx_p->ISF_VdoSrcPort = 0;
				#if (MANUAL_SYNC == ENABLE)
				p_ctx_p->gISF_VdoEnc_ecnt = 0;
				#if (MANUAL_SYNC_KOFF == ENABLE)
				p_ctx_p->gISF_VdoEnc_ts_ipl_end = 0;
				p_ctx_p->gISF_VdoEnc_ts_enc_begin = 0;
				p_ctx_p->gISF_VdoEnc_ts_enc_end = 0;
				p_ctx_p->gISF_VdoEnc_ts_sync = 0;
				p_ctx_p->gISF_VdoEnc_ts_kick_off = 0;
				#endif
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_START_TIMER_BY_MANUAL, 1);
				#endif

				p_ctx_p->gISF_EncKick	= 1;	 //start kick
				p_ctx_p->gISF_SkipKick	= 0;

			} else {  //pSrcPort->ConnectType == ISF_CONNECT_PUSH
				#if (MANUAL_SYNC == ENABLE)
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_START_TIMER_BY_MANUAL, 0);
				#endif
				//do nothing!
			}
			if(p_ctx_c->gfn_vdoenc_proc_cb) {
				p_ctx_c->gfn_vdoenc_proc_cb(id, ISF_PORT_CMD_START);
			}

			if (p_ctx_p->gISF_VdoEncMaxMemSize > 0) {
				NMI_VDOENC_MEM_RANGE Mem    = {0};
				UINT32           uiBufSize  = 0;
				pNMI_VdoEnc->GetParam(id, NMI_VDOENC_PARAM_ALLOC_SIZE, &uiBufSize);

				//check MAX_MEM & memory requirement for current setting
				if (uiBufSize > p_ctx_p->gISF_VdoEncMaxMem.Size) {
					DBG_ERR("[ISF_VDOENC][%d] FATAL ERROR: current encode setting need memory(%d) > MAX_MEM(%d)\r\n", oPort, uiBufSize, p_ctx_p->gISF_VdoEncMaxMem.Size);
					return ISF_ERR_NEW_FAIL; // this will mapping to HDAL => HD_ERR_NOBUF = -52,      ///< no buffer (not enough pool size for new blk)
				}

				//reassign memory layout (user may open->start->stop->change codec->start, JIRA: IPC_680-128
				Mem.Addr = p_ctx_p->gISF_VdoEncMaxMem.Addr;
				Mem.Size = p_ctx_p->gISF_VdoEncMaxMem.Size; //uiBufSize;
				pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_MEM_RANGE, (UINT32) &Mem);
			}

			if (FALSE == _ISF_VdoEnc_isUserPull_AllReleased(oPort)) {
				DBG_ERR("[ISF_VDOENC][%d] Start failed !! Please release all pull_out bitstream before attemping to next-time start !!\r\n", oPort);
				return ISF_ERR_NOT_REL_YET;
			}

			_ISF_VdoEnc_FreeMem_BsOnly(pThisUnit, id);           // free all bs in isf_unit queue , JIRA: IPC_680-120
			pNMI_VdoEnc->Action(oPort, NMI_VDOENC_ACTION_START); // free all bs in nmedia queue

#ifdef __KERNEL__
			if (id < BUILTIN_VDOENC_PATH_ID_MAX) {
				if (p_ctx_c->bIs_fastboot_enc[id]) {
					if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[id]) {
						pNMI_VdoEnc->SetParam(id, NMI_VDOENC_PARAM_BUILTIN_MAX_FRAME_QUEUE, g_vdoenc_fastboot_bsdata_blk_max[id]);
					}
				}
			}
#endif


			if(pSrcPort->connecttype == (ISF_CONNECT_PUSH|ISF_CONNECT_SYNC)) {
				#if (MANUAL_SYNC == ENABLE)
				_ISF_VdoEnc_OnSync(id); // do first kick
				#endif
			} else {  //pSrcPort->ConnectType == ISF_CONNECT_PUSH
				//do nothing!
			}

			// start frc
			_isf_frc_start(&isf_vdoenc, ISF_IN(id), &(p_ctx_p->gISF_VdoEnc_infrc), p_ctx_p->gISF_VdoEnc_framepersecond);

#ifdef __KERNEL__
			if (id < BUILTIN_VDOENC_PATH_ID_MAX) {
				if (p_ctx_c->bIs_fastboot_enc[id]) {
					if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[id] && !g_vdoenc_get_builtinbs_done[id]) {
						NMR_VdoEnc_GetBuiltinBsData(id);
						g_vdoenc_get_builtinbs_done[id] = 1;
					}
				}
			}
#endif

			p_ctx_p->gISF_VdoEnc_Started = 1;
		}
		break;
	case ISF_PORT_CMD_PAUSE: {                             ///< run -> wait    (pause data processing, keep context, engine or device enter suspend mode)
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(pause)");
		}
		break;
	case ISF_PORT_CMD_RESUME: {                            ///< wait -> run    (engine or device leave suspend mode, restore context, resume data processing)
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(resume)");
		}
		break;
	case ISF_PORT_CMD_STOP: {                              ///< run -> stop    (stop data processing, engine disable or device power off)
			UINT32 id = oPort-ISF_OUT_BASE;

			p_ctx_p->gISF_VdoEnc_Started = 0;

			_ISF_VdoEnc_YUV_TMOUT_ResourceUnlock(id);   // let push_in(blocking mode) auto-unlock, give up & release YUV (because p_ctx_p->gISF_VdoEnc_Started = 0  , so it will NOT try again)

			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(stop)");
			_ISF_VdoEnc_OutputPort_Disable(id);
			pNMI_VdoEnc->Action(oPort, NMI_VDOENC_ACTION_STOP); // nmedia will guarantee task idle, codec driver close -- no more BS can be produced after this
			if(p_ctx_c->gfn_vdoenc_proc_cb) {
				p_ctx_c->gfn_vdoenc_proc_cb(id, ISF_PORT_CMD_STOP);
			}
			gtestCap = 0;

			// maybe this should only do at Close(), but custom already know "stop() will auto-unlock." For safety..keep it, have to discuss further with Jeah.
			_ISF_VdoEnc_Unlock_PullQueue(id);  // let pull_out(blocking mode) auto-unlock

			if (_isf_vdoenc_is_allpath_stopped() == TRUE) {
				SEM_SIGNAL(p_ctx_c->ISF_VDOENC_POLL_BS_SEM_ID); // let poll_list(blocking_mode) auto-unlock, if all path are stopped
			}

			// stop frc
			_isf_frc_stop(&isf_vdoenc, ISF_IN(id), &(p_ctx_p->gISF_VdoEnc_infrc) );
		}
		break;
	case ISF_PORT_CMD_CLOSE: {                             ///< stop -> off    (terminate task and free mempool)
			ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(close)");

			if (FALSE == _ISF_VdoEnc_isUserPull_AllReleased(oPort)) {
				DBG_ERR("[ISF_VDOENC][%d] Close failed !! Please release all pull_out bitstream before attemping to close !!\r\n", oPort);
				return ISF_ERR_NOT_REL_YET;
			}

			_ISF_VdoEnc_Close(pThisUnit, oPort, TRUE); // TRUE means to call do_release_i() normally
			_ISF_VdoEnc_ImgCap_Close(pThisUnit);

			_ISF_VdoEnc_Unlock_PullQueue(oPort);  // let pull_out(blocking mode) auto-unlock

			p_ctx_p->gISF_VdoEnc_Opened = FALSE;
#ifdef __KERNEL__
			g_vdoenc_1st_open[oPort] = FALSE;
#endif
		}
		break;
	case ISF_PORT_CMD_RUNTIME_UPDATE:                      ///< run -> run     (apply run-time parameter if it is dirty)
		ISF_UNIT_TRACE(ISF_OP_STATE, pThisUnit, oPort, "(runtime-update)");
		_ISF_VdoEnc_UpdatePortInfo(pThisUnit, oPort, cmd);
		break;
	default:
		break;
	}

	return r;
}

void isf_vdoenc_install_id(void)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
    UINT32 i = 0;
    for (i = 0; i < PATH_MAX_COUNT; i++) {
		VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[i];

		//OS_CONFIG_SEMPHORE(p_ctx_p->ISF_VDOENC_SEM_ID, 0, 1, 1);
#if (SUPPORT_PULL_FUNCTION)
		OS_CONFIG_SEMPHORE(p_ctx_p->ISF_VDOENC_PULLQ_SEM_ID, 0, 0, 0);
		OS_CONFIG_SEMPHORE(p_ctx_p->ISF_VDOENC_PARTIAL_SEM_ID, 0, 1, 1);
#endif
		OS_CONFIG_SEMPHORE(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_IDLE, 0, 1, 1);
		OS_CONFIG_SEMPHORE(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_DONE, 0, 0, 0);
		OS_CONFIG_SEMPHORE(p_ctx_p->ISF_VDOENC_YUV_SEM_ID, 0, 0, 0);
    }
	//OS_CONFIG_SEMPHORE(p_ctx_c->ISF_VDOENC_COMMON_SEM_ID, 0, 1, 1);
	OS_CONFIG_SEMPHORE(p_ctx_c->ISF_VDOENC_POLL_BS_SEM_ID, 0, 0, 0);

	OS_CONFIG_SEMPHORE(ISF_VDOENC_PROC_SEM_ID, 0, 1, 1);
}

void isf_vdoenc_uninstall_id(void)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
    UINT32 i = 0;
    for (i = 0; i < PATH_MAX_COUNT; i++) {
		VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[i];

		//SEM_DESTROY(p_ctx_p->ISF_VDOENC_SEM_ID);
#if (SUPPORT_PULL_FUNCTION)
		SEM_DESTROY(p_ctx_p->ISF_VDOENC_PULLQ_SEM_ID);
		SEM_DESTROY(p_ctx_p->ISF_VDOENC_PARTIAL_SEM_ID);
#endif
		SEM_DESTROY(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_IDLE);
		SEM_DESTROY(p_ctx_p->ISF_VDOENC_SNAP_LINUX_SEM_ID_DONE);
		SEM_DESTROY(p_ctx_p->ISF_VDOENC_YUV_SEM_ID);
    }
	//SEM_DESTROY(p_ctx_c->ISF_VDOENC_COMMON_SEM_ID);
	SEM_DESTROY(p_ctx_c->ISF_VDOENC_POLL_BS_SEM_ID);

	SEM_DESTROY(ISF_VDOENC_PROC_SEM_ID);
}



ER _kflow_videoenc_query_isp_ratio(UINT32 isp_id, KDRV_VDOENC_ISP_RATIO *isp_ratio)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = 0;
	UINT32 pathID = 0, total_path_cnt = 0;
	KDRV_VDOENC_ISP_RATIO isp_ratio_tmp = {0};
	UINT32 edge_ratio_sum = 0, dn_2d_ratio_sum = 0, dn_3d_ratio_sum = 0;  // original struct only define UINT8 (256 max)

	DBG_IND("[ISF_VDOENC][B] IQ get isp_ratio, isp_id = %d\r\n", isp_id);

	// reset output result
	memset((void *)isp_ratio, 0, sizeof(KDRV_VDOENC_ISP_RATIO));

	for (pathID = 0; pathID < PATH_MAX_COUNT; pathID++) {
		p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[pathID];

		// check all path for [isp_id] & [started]
		if ((p_ctx_p->gISF_VdoEnc_ISP_Id == isp_id) && (p_ctx_p->gISF_VdoEnc_Started == 1)) {
			if (p_ctx_p->gISF_VStrmBuf.CodecType == MEDIAVIDENC_H264) {
				if ((MP_H264Enc_getVideoObject((MP_VDOENC_ID)pathID)) && (MP_H264Enc_getVideoObject((MP_VDOENC_ID)pathID)->GetInfo)) {
					(MP_H264Enc_getVideoObject((MP_VDOENC_ID)pathID)->GetInfo)(VDOENC_GET_ISP_RATIO, (UINT32 *)&isp_ratio_tmp, 0, 0);
				}
			} else if (p_ctx_p->gISF_VStrmBuf.CodecType == MEDIAVIDENC_H265) {
				if ((MP_H265Enc_getVideoObject((MP_VDOENC_ID)pathID)) && (MP_H265Enc_getVideoObject((MP_VDOENC_ID)pathID)->GetInfo)) {
					(MP_H265Enc_getVideoObject((MP_VDOENC_ID)pathID)->GetInfo)(VDOENC_GET_ISP_RATIO, (UINT32 *)&isp_ratio_tmp, 0, 0);
				}
			} else {
				continue;  // skip mjpeg path
			}

			if (gISF_VdoEncMsgLevel[pathID] & ISF_VDOENC_MSG_ISP_RATIO) {
					DBG_DUMP("[ISF_VDOENC][%d] isp_id = %d, ratio (edge/dn2d/dn3d) = (%u/%u/%u), base = %u\r\n", pathID, isp_id, isp_ratio_tmp.edge_ratio, isp_ratio_tmp.dn_2d_ratio, isp_ratio_tmp.dn_3d_ratio, isp_ratio_tmp.ratio_base);
			}

			DBG_IND("[ISF_VDOENC][%d] isp_id = %d, ratio (edge/dn2d/dn3d) = (%u/%u/%u), base = %u\r\n", pathID, isp_id, isp_ratio_tmp.edge_ratio, isp_ratio_tmp.dn_2d_ratio, isp_ratio_tmp.dn_3d_ratio, isp_ratio_tmp.ratio_base);

			// update result
			edge_ratio_sum  += isp_ratio_tmp.edge_ratio;
			dn_2d_ratio_sum += isp_ratio_tmp.dn_2d_ratio;
			dn_3d_ratio_sum += isp_ratio_tmp.dn_3d_ratio;
			isp_ratio->ratio_base   = isp_ratio_tmp.ratio_base;

			total_path_cnt++;
		}
	}

	// merge all path(same isp_id) result into one
	if (total_path_cnt > 0) {
		isp_ratio->edge_ratio  = (UINT8)(edge_ratio_sum  / total_path_cnt);
		isp_ratio->dn_2d_ratio = (UINT8)(dn_2d_ratio_sum / total_path_cnt);
		isp_ratio->dn_3d_ratio = (UINT8)(dn_3d_ratio_sum / total_path_cnt);
	}

	DBG_IND("[ISF_VDOENC][E] isp_id = %d, total_path_cnt = %u => (AVERAGE) ratio (edge/dn2d/dn3d) = (%u/%u/%u), base = %u\r\n", isp_id, total_path_cnt, isp_ratio->edge_ratio, isp_ratio->dn_2d_ratio, isp_ratio->dn_3d_ratio, isp_ratio->ratio_base);

	return E_OK;
}

ER kflow_videoenc_evt_fp_reg(CHAR *name, ISP_EVENT_FP fp, ISP_EVENT evt, KFLOW_VIDEOENC_WAIT_FLG wait_flg)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	p_ctx_c->kflow_videoenc_get_isp_dev_fp = fp;  // Let PQ team register callback function

	return E_OK;
}

ER kflow_videoenc_evt_fp_unreg(CHAR *name)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	p_ctx_c->kflow_videoenc_get_isp_dev_fp = 0;

	return E_OK;
}

ER kflow_videoenc_set(UINT32 id, KFLOW_VIDEOENC_ISP_ITEM item, void *data)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	ISP_ID ipp_id = id & 0xFF;

	if (g_ve_ctx.port == NULL) {
		DBG_IND(" [ISF_VDOENC] module NOT init yet !!\r\n");
		return E_NOEXS;
	}

	if (item == KFLOW_VIDEOENC_ISP_ITEM_TNR) {
		memcpy(&p_ctx_c->g_h26x_enc_tnr[ipp_id], data, sizeof(KDRV_VDOENC_3DNR));
	}

	return E_OK;
}

ER kflow_videoenc_get(UINT32 id, KFLOW_VIDEOENC_ISP_ITEM item, void *data)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	ISP_ID ipp_id = id & 0xFF;

	if (g_ve_ctx.port == NULL) {
		DBG_IND(" [ISF_VDOENC] module NOT init yet !!\r\n");
		return E_NOEXS;
	}

	if (item == KFLOW_VIDEOENC_ISP_ITEM_TNR) {
		memcpy(data, &p_ctx_c->g_h26x_enc_tnr[ipp_id], sizeof(KDRV_VDOENC_3DNR));
	} else if (item == KFLOW_VIDEOENC_ISP_ITEM_RATIO) {
		KDRV_VDOENC_ISP_RATIO isp_ratio = {0};
		_kflow_videoenc_query_isp_ratio(id, &isp_ratio);
		memcpy(data, &isp_ratio, sizeof(KDRV_VDOENC_ISP_RATIO));
	}
	return E_OK;
}

//VDOENC_PARAM_3DNR_CB (callback from kdrv_h26x for each frame, we then callback IQ function to update 3DNR settings, then copy to dal_h26x)
void _ISF_VdoEnc_3DNR_Internal(UINT32 pathID, UINT32 p3DNRConfig)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[pathID];
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	static UINT32 sCount = 0;
	ISP_EVENT_FP fp = p_ctx_c->kflow_videoenc_get_isp_dev_fp;
	UINT32 isp_id   = p_ctx_p->gISF_VdoEnc_ISP_Id;
	ISP_ID ipp_id   = isp_id & 0xFF;
	UINT32 raw_frame_count =  p_ctx_p->g_raw_frame_count;
	UINT32 tnr_time = 0;

	if ((ipp_id >= ISP_ID_MAX_NUM) && (isp_id != ISP_ID_IGNORE)) {
		if (sCount++ % 128 == 0) {
			DBG_ERR("isp_id(%08x), ipp_id(%d) is out of range(0~%d), and is not IGNORE(%08x)\r\n", isp_id, ipp_id, ISP_ID_MAX_NUM-1, ISP_ID_IGNORE);
		}
		return;
	}

	pNMI_VdoEnc->SetParam(pathID, NMI_VDOENC_PARAM_FASTBOOT_ISP_ID, ipp_id);

	if (isp_id == ISP_ID_IGNORE) {
		isp_id = ISP_ID_1; // TODO: should use videoproc push ISF_DATA's isp_id later;
		pNMI_VdoEnc->SetParam(pathID, NMI_VDOENC_PARAM_FASTBOOT_ISP_ID, isp_id);
	}

	if (gISF_VdoEncMsgLevel[pathID] & ISF_VDOENC_MSG_TNR) {
		tnr_time = Perf_GetCurrent();
	}

	//TODO: isp_id = 0~7 sensor id,  >7 virtual isp id,   0xffffffff  don't care
	if (fp != NULL)  {
		fp(isp_id, ISP_EVENT_ENC_TNR, raw_frame_count, NULL);   // iq will then call kflow_videoenc_set() to update 3DNR parameters
	}

	if (gISF_VdoEncMsgLevel[pathID] & ISF_VDOENC_MSG_TNR) {
		if (ipp_id < ISP_ID_MAX_NUM) {
			DBG_DUMP("[ISF_VDOENC][%d] isp_id = %08x, ipp_id = %d, SIE fn = %d,  tnr = (%d, %d, %d, %d), fp = 0x%08x, tnr_time = %d us\r\n",
				pathID,
				isp_id,
				ipp_id,
				raw_frame_count,
				p_ctx_c->g_h26x_enc_tnr[ipp_id].nr_3d_mode,
				p_ctx_c->g_h26x_enc_tnr[ipp_id].nr_3d_adp_th_p2p[0],
				p_ctx_c->g_h26x_enc_tnr[ipp_id].nr_3d_adp_th_p2p[1],
				p_ctx_c->g_h26x_enc_tnr[ipp_id].nr_3d_adp_th_p2p[2],
				(UINT32)fp,
				Perf_GetCurrent() - tnr_time);
		} else {
			DBG_ERR("ipp_id(%d) is out of range(0~%d)\r\n", ipp_id, ISP_ID_MAX_NUM-1);
			return;
		}
	}

	// copy iq 3DNR parameters to H26x driver
	if (ipp_id < ISP_ID_MAX_NUM) {
		memcpy((void *)p3DNRConfig, &p_ctx_c->g_h26x_enc_tnr[ipp_id], sizeof(KDRV_VDOENC_3DNR));
	} else {
		DBG_ERR("ipp_id(%d) is out of range(0~%d)\r\n", ipp_id, ISP_ID_MAX_NUM-1);
		return;
	}
}

//ISP_CB (callback from kdrv_h26x if isp ratio modification is required, we then callback IQ to notify the request.
void _ISF_VdoEnc_ISPCB_Internal(UINT32 pathID, UINT32 pISPConfig)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	ISP_EVENT_FP fp = p_ctx_c->kflow_videoenc_get_isp_dev_fp;
	VDOENC_CONTEXT_PORT   *p_ctx_p = 0;
	UINT32 isp_id = 0, raw_frame_count = 0;
	ISP_ID ipp_id = 0;
	static UINT32 sCount = 0;

	if (g_ve_ctx.port == NULL) {
		DBG_ERR(" [ISF_VDOENC] module NOT init yet !!\r\n");
		return;
	}

	p_ctx_p         = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[pathID];
	isp_id          = p_ctx_p->gISF_VdoEnc_ISP_Id;
	ipp_id          = isp_id & 0xFF;
	raw_frame_count = p_ctx_p->g_raw_frame_count;

	if ((ipp_id >= ISP_ID_MAX_NUM) && (isp_id != ISP_ID_IGNORE)) {
		if (sCount++ % 128 == 0) {
			DBG_ERR("isp_id(%08x), ipp_id(%d) is out of range(0~%d), and is not IGNORE(%08x)\r\n", isp_id, ipp_id, ISP_ID_MAX_NUM-1, ISP_ID_IGNORE);
		}
		return;
	}

	if (isp_id == ISP_ID_IGNORE) {
		isp_id = ISP_ID_1; // TODO: should use videoproc push ISF_DATA's isp_id later;
	}

	//TODO: isp_id = 0~7 sensor id,  >7 virtual isp id,   0xffffffff  don't care
	if (fp != NULL)  {
		fp(isp_id, ISP_EVENT_ENC_RATIO, raw_frame_count, NULL);   // iq will then call kflow_videoenc_get(isp_id, KFLOW_VIDEOENC_ISP_ITEM_RATIO, ..) to query this isp_id for ISP ratio update
	}

	if (gISF_VdoEncMsgLevel[pathID] & ISF_VDOENC_MSG_ISP_RATIO) {
		DBG_DUMP("[ISF_VDOENC][%d] isp ratio update, isp_id = %08x, fp = 0x%08x\r\n", pathID, isp_id, (UINT32)fp);
	}
}

//----------------------------------------------------------------------------------------------------------------------

//static ISF_RV _ISF_VdoEnc_LockCB(UINT64 module, UINT32 hData)
static ISF_RV _ISF_VdoEnc_LockCB(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 hData)
{
	UINT32         i = 0;
	VDOENC_BSDATA *pVdoEncData = NULL;
	ID tid;
	char unit_name[8];
	unsigned long flags=0;
	if (p_thisunit) {
		strncpy(unit_name, p_thisunit->unit_name, 8);
		unit_name[7]='\0';
	} else {
		strcpy(unit_name, "user");
	}

	if (!hData) {
		get_tid(&tid);
		DBG_ERR("[ISF_VDOENC] LOCK tid = %d hData is NULL\r\n", tid);
		return ISF_ERR_INVALID_VALUE;
	}

	// search VDOENC_BSDATA link list by hData
	for (i = 0; i < PATH_MAX_COUNT; i++) {
		VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[i];

		NMR_VdoEnc_Lock_cpu(&flags);
		pVdoEncData = p_ctx_p->g_p_vdoenc_bsdata_link_head;

		while (pVdoEncData != NULL) {
			if (pVdoEncData->hData == hData) {
				pVdoEncData->refCnt++;
				if (gISF_VdoEncMsgLevel[i] & ISF_VDOENC_MSG_LOCK) {
					DBG_DUMP("[ISF_VDOENC][%d] LOCK %s ref %d hData 0x%x\r\n", i, unit_name, pVdoEncData->refCnt, pVdoEncData->hData);
				}
				NMR_VdoEnc_Unlock_cpu(&flags);
				return ISF_OK;
			}

			pVdoEncData = pVdoEncData->pnext_bsdata;
		}
		NMR_VdoEnc_Unlock_cpu(&flags);
	}

	return ISF_OK;
}

//static ISF_RV _ISF_VdoEnc_UnLockCB(UINT64 module, UINT32 hData)
static ISF_RV _ISF_VdoEnc_UnLockCB(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 hData)
{
	UINT32         i = 0;
	UINT32         hData2 = 0;
	VDOENC_BSDATA *pVdoEncData = NULL;
	PVDO_BITSTREAM pVStrmBuf = (VDO_BITSTREAM *) p_data->desc;
	ID tid = 0;
	char unit_name[8];
	unsigned long flags=0;
	if (p_thisunit) {
		strncpy(unit_name, p_thisunit->unit_name, 8);
		unit_name[7]='\0';
	} else {
		strcpy(unit_name, "user");
	}

	if (!hData) {
		get_tid(&tid);
		DBG_ERR("[ISF_VDOENC] UNLOCK tid = %d hData is NULL\r\n", tid);
		return ISF_ERR_INVALID_VALUE;
	}

	// search VDOENC_BSDATA link list by hData
	for (i = 0; i < PATH_MAX_COUNT; i++) {
		VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[i];

		NMR_VdoEnc_Lock_cpu(&flags);
		pVdoEncData = p_ctx_p->g_p_vdoenc_bsdata_link_head;

		while (pVdoEncData != NULL) {
			if (pVdoEncData->hData == hData) {
				if (pVdoEncData->refCnt > 0) {
					pVdoEncData->refCnt--;
				} else {
					get_tid(&tid);
					if (p_thisunit) {
						DBG_ERR("[ISF_VDOENC][%d] UnLOCK %s tid = %d refCnt is 0\r\n", i, unit_name, tid);
					} else {
						DBG_ERR("[ISF_VDOENC][%d] UnLOCK NULL tid = %d refCnt is 0\r\n", i, tid);
					}
					NMR_VdoEnc_Unlock_cpu(&flags);
					return ISF_ERR_INVALID_VALUE;
				}

				if (gISF_VdoEncMsgLevel[i] & ISF_VDOENC_MSG_LOCK) {
					DBG_DUMP("[ISF_VDOENC][%d] UNLOCK %s ref %d hData 0x%x\r\n", i, unit_name, pVdoEncData->refCnt, pVdoEncData->hData);
				}

				if (pVdoEncData->refCnt == 0) {
					(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(i),  ISF_USER_PROBE_REL, ISF_ENTER);  // [USER] REL_ENTER

					if (pVdoEncData != p_ctx_p->g_p_vdoenc_bsdata_link_head) {
						VDOENC_BSDATA *pVdoEncData2 = NULL;
						get_tid(&tid);

						DBG_ERR("[ISF_VDOENC][%d] UNLOCK %s, tid = %d, not first data in queue!! 1st link data = (0x%x, 0x%x, %d), rel data = (0x%x, 0x%x, %d)\r\n",
							i,
							unit_name,
							tid,
							(UINT32)p_ctx_p->g_p_vdoenc_bsdata_link_head,
							(p_ctx_p->g_p_vdoenc_bsdata_link_head?p_ctx_p->g_p_vdoenc_bsdata_link_head->hData:0),
							(p_ctx_p->g_p_vdoenc_bsdata_link_head?p_ctx_p->g_p_vdoenc_bsdata_link_head->blk_id:0),
							(UINT32)pVdoEncData,
							pVdoEncData->hData,
							pVdoEncData->blk_id);

						// search for release
#if 0
						pVdoEncData2 = p_ctx_p->g_p_vdoenc_bsdata_link_head;

						while (pVdoEncData2 != NULL) {
							if (pVdoEncData2->pnext_bsdata == pVdoEncData) {
								pVdoEncData2->pnext_bsdata = pVdoEncData->pnext_bsdata;
								break;
							}

							pVdoEncData2 = pVdoEncData2->pnext_bsdata;
						}

						// free block
						_ISF_VdeEnc_FREE_BSDATA(pVdoEncData);
#else
						pVdoEncData2 = p_ctx_p->g_p_vdoenc_bsdata_link_head;
						while (pVdoEncData2 != pVdoEncData) {
							// update link head
							p_ctx_p->g_p_vdoenc_bsdata_link_head = p_ctx_p->g_p_vdoenc_bsdata_link_head->pnext_bsdata;

							DBG_ERR("[ISF_VDOENC] RECOVER : auto release previous bsdata, hData = 0x%08x\r\n", (UINT)pVdoEncData2->hData);

							pNMI_VdoEnc->SetParam(i, NMI_VDOENC_PARAM_UNLOCK_BS_ADDR, pVdoEncData2->hData);
							_ISF_VdeEnc_FREE_BSDATA(pVdoEncData2);

							pVdoEncData2 = p_ctx_p->g_p_vdoenc_bsdata_link_head;
						}
						DBG_ERR("[ISF_VDOENC] RECOVER : release this wrong bsdata, hdata = 0x%08x\r\n", (UINT)pVdoEncData->hData);
						p_ctx_p->g_p_vdoenc_bsdata_link_head = p_ctx_p->g_p_vdoenc_bsdata_link_head->pnext_bsdata;
						_ISF_VdeEnc_FREE_BSDATA(pVdoEncData);
#endif

						(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(i),  ISF_USER_PROBE_REL, ISF_OK);  // [USER] REL_OK ( with some error, but actually released )

					} else {
						// update link head
						p_ctx_p->g_p_vdoenc_bsdata_link_head = p_ctx_p->g_p_vdoenc_bsdata_link_head->pnext_bsdata;

						// free block
						if (gISF_VdoEncMsgLevel[i] & ISF_VDOENC_MSG_LOCK) {
							DBG_DUMP("[ISF_VDOENC][%d] Free hData = 0x%x, blk_id = %d\r\n", i, pVdoEncData->hData, pVdoEncData->blk_id);
						}
						_ISF_VdeEnc_FREE_BSDATA(pVdoEncData);

						(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(i),  ISF_USER_PROBE_REL, ISF_OK);  // [USER] REL_OK
					}

					NMR_VdoEnc_Unlock_cpu(&flags); // the following nmedia unlock will call sem_wait() , so must unlock CPU first
					// Set BS data address is unlock to NMedia
					hData2 = pVStrmBuf->resv[0];
					pNMI_VdoEnc->SetParam(i, NMI_VDOENC_PARAM_UNLOCK_BS_ADDR, hData);
					if (hData2 != 0) {
						pNMI_VdoEnc->SetParam(i, NMI_VDOENC_PARAM_UNLOCK_BS_ADDR, hData2);
					}
					return ISF_OK;
				}

				NMR_VdoEnc_Unlock_cpu(&flags);
				return ISF_OK;
			}

			pVdoEncData = pVdoEncData->pnext_bsdata;
		}
		NMR_VdoEnc_Unlock_cpu(&flags);
	}

	DBG_IND("[ISF_VDOENC] UNLOCK %s, tid = %d, data = 0x%x. Fail to find hData in queue !!\r\n", unit_name, tid, hData);
	return ISF_OK;
}

#if (SUPPORT_PULL_FUNCTION)

BOOL _ISF_VdoEnc_isUserPull_AllReleased(UINT32 pathID)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[pathID];
#ifdef __KERNEL__
		VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
#endif
	PVDOENC_PULL_QUEUE p_obj = NULL;
	VDOENC_BSDATA *pVdoEncData = NULL;
	UINT32 pull_queue_num = 0;
	UINT32 link_list_num = 0;
	UINT32 uiPullQMax = ISF_VDOENC_PULL_Q_MAX;
	unsigned long flags = 0;

	NMR_VdoEnc_Lock_cpu(&flags);

#ifdef __KERNEL__
	if (pathID < BUILTIN_VDOENC_PATH_ID_MAX) {
		if (p_ctx_c->bIs_fastboot_enc[pathID]) {
			if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[pathID]) {
				uiPullQMax = g_vdoenc_fastboot_bsdata_blk_max[pathID];
			}
		}
	}
#endif

	// PullQueue remain number
	p_obj = &p_ctx_p->gISF_VoEnc_PullQueue;
	if (p_obj->Rear > p_obj->Front) {
		pull_queue_num = p_obj->Rear - p_obj->Front;
	} else if (p_obj->Rear < p_obj->Front){
		pull_queue_num = uiPullQMax + p_obj->Rear - p_obj->Front;
	} else if (p_obj->Rear == p_obj->Front && p_obj->bFull == TRUE){
		pull_queue_num = uiPullQMax;
	} else {
		pull_queue_num = 0;
	}

	// link-list remain number
	pVdoEncData = p_ctx_p->g_p_vdoenc_bsdata_link_head;
	while (pVdoEncData != NULL) {
		link_list_num++;
		pVdoEncData = pVdoEncData->pnext_bsdata;
	}

	NMR_VdoEnc_Unlock_cpu(&flags);

	if (gISF_VdoEncMsgLevel[pathID] & ISF_VDOENC_MSG_PULLQ) {
		DBG_DUMP("[ISF_VDOENC][%d] remain number => PullQueue(%u), link-list(%u)\r\n", pathID, pull_queue_num, link_list_num);
	}

	return (pull_queue_num == link_list_num)? TRUE:FALSE;
}

BOOL _ISF_VdoEnc_isEmpty_PullQueue(UINT32 pathID)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[pathID];
	PVDOENC_PULL_QUEUE p_obj;
	BOOL is_empty = FALSE;
	unsigned long flags=0;

	NMR_VdoEnc_Lock_cpu(&flags);

	p_obj = &p_ctx_p->gISF_VoEnc_PullQueue;
	is_empty = ((p_obj->Front == p_obj->Rear) && (p_obj->bFull == FALSE));

	NMR_VdoEnc_Unlock_cpu(&flags);

	return is_empty;
}

BOOL _ISF_VdoEnc_isFull_PullQueue(UINT32 pathID)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[pathID];
	PVDOENC_PULL_QUEUE p_obj;
	BOOL is_full = FALSE;
	unsigned long flags = 0;

	NMR_VdoEnc_Lock_cpu(&flags);

	p_obj = &p_ctx_p->gISF_VoEnc_PullQueue;
	is_full = ((p_obj->Front == p_obj->Rear) && (p_obj->bFull == TRUE));

	NMR_VdoEnc_Unlock_cpu(&flags);

	return is_full;
}


void _ISF_VdoEnc_Unlock_PullQueue(UINT32 pathID)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[pathID];

	if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_VDOENC_PULLQ_SEM_ID, vos_util_msec_to_tick(0))) {
		DBG_IND("[ISF_VDOENC][%d] no data in pull queue, auto unlock pull blocking mode !!\r\n", pathID);
	}

	// [case_1]   no data in pullQ, then this 0 ->   0   -> 1 to unlock HDAL pull blocking mode (fake semaphore.... but it's OK, because _ISF_VdoEnc_Get_PullQueue() will return FALSE, there's no actual data in queue )
	// [case_2] have data in pullQ, then this n -> (n-1) -> n to recover the semaphore count
	SEM_SIGNAL(p_ctx_p->ISF_VDOENC_PULLQ_SEM_ID);
}

BOOL _ISF_VdoEnc_Put_PullQueue(UINT32 pathID, ISF_DATA *data_in)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[pathID];
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
	PVDOENC_PULL_QUEUE pObj;
	UINT32 uiPullQMax = ISF_VDOENC_PULL_Q_MAX;
	unsigned long flags = 0;

	NMR_VdoEnc_Lock_cpu(&flags);
#ifdef __KERNEL__
	if (pathID < BUILTIN_VDOENC_PATH_ID_MAX) {
		if (p_ctx_c->bIs_fastboot_enc[pathID]) {
			if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[pathID]) {
				uiPullQMax = g_vdoenc_fastboot_bsdata_blk_max[pathID];
			}
		}
	}
#endif

	pObj = &p_ctx_p->gISF_VoEnc_PullQueue;

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == TRUE)) {
		DBG_ERR("[ISF_VDOENC][%d] Pull Queue is Full!\r\n", pathID);
		NMR_VdoEnc_Unlock_cpu(&flags);
		return FALSE;
	} else {
		memcpy(&pObj->Queue[pObj->Rear], data_in, sizeof(ISF_DATA));

		pObj->Rear = (pObj->Rear + 1) % uiPullQMax;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = TRUE;
		}

		NMR_VdoEnc_Unlock_cpu(&flags);

		SEM_SIGNAL(p_ctx_p->ISF_VDOENC_PULLQ_SEM_ID); // PULLQ + 1

		if (p_ctx_c->g_isf_vdoenc_poll_bs_event_mask == 0) {      // no any event yet
			if (p_ctx_c->g_poll_bs_info.path_mask & (1<<pathID))  // this pathID is included in user provided path check mask
				SEM_SIGNAL(p_ctx_c->ISF_VDOENC_POLL_BS_SEM_ID);
		}

		return TRUE;
	}
}

ISF_RV _ISF_VdoEnc_Get_PullQueue(UINT32 pathID, ISF_DATA *data_out, INT32 wait_ms)
{
	VDOENC_CONTEXT_PORT   *p_ctx_p = (VDOENC_CONTEXT_PORT *)&g_ve_ctx.port[pathID];
#ifdef __KERNEL__
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;
#endif
	PVDOENC_PULL_QUEUE pObj;
	UINT32 uiPullQMax = ISF_VDOENC_PULL_Q_MAX;
	unsigned long flags = 0;

	if (wait_ms < 0) {
		// blocking (wait until data available) , if success PULLQ - 1 , else wait forever (or until signal interrupt and return FALSE)
		if (SEM_WAIT_INTERRUPTIBLE(p_ctx_p->ISF_VDOENC_PULLQ_SEM_ID)) {
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID),  ISF_USER_PROBE_PULL_WRN, ISF_ERR_QUEUE_EMPTY);  // [USER] PULL_WRN
			return ISF_ERR_QUEUE_EMPTY;
		}
	} else  {
		// non-blocking (wait_ms=0) , timeout (wait_ms > 0). If success PULLQ - 1 , else just return FALSE
		if (SEM_WAIT_TIMEOUT(p_ctx_p->ISF_VDOENC_PULLQ_SEM_ID, vos_util_msec_to_tick(wait_ms))) {
			DBG_MSG("[ISF_VDOENC][%d] Pull Queue Semaphore timeout!\r\n", pathID);
			(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID),  ISF_USER_PROBE_PULL_WRN, ISF_ERR_WAIT_TIMEOUT);  // [USER] PULL_WRN
			return ISF_ERR_WAIT_TIMEOUT;
		}
	}

	NMR_VdoEnc_Lock_cpu(&flags);

#ifdef __KERNEL__
	if (pathID < BUILTIN_VDOENC_PATH_ID_MAX) {
		if (p_ctx_c->bIs_fastboot_enc[pathID]) {
			if (p_ctx_c->bIs_fastboot && g_vdoenc_1st_open[pathID]) {
				uiPullQMax = g_vdoenc_fastboot_bsdata_blk_max[pathID];
			}
		}
	}
#endif

	pObj = &p_ctx_p->gISF_VoEnc_PullQueue;

	if ((pObj->Front == pObj->Rear) && (pObj->bFull == FALSE)) {
		NMR_VdoEnc_Unlock_cpu(&flags);
		DBG_IND("[ISF_VDOENC][%d] Pull Queue is Empty !!!\r\n", pathID);   // This maybe happen if user call VdoEnc Stop, because I will auto-unlock pullQ if user is waiting for hd_videoenc_pull_out_buf() for blocking mode
		(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID),  ISF_USER_PROBE_PULL_WRN, ISF_ERR_QUEUE_EMPTY);  // [USER] PULL_WRN
		return ISF_ERR_QUEUE_EMPTY;
	} else {
		memcpy(data_out, &pObj->Queue[pObj->Front], sizeof(ISF_DATA));

		pObj->Front= (pObj->Front+ 1) % uiPullQMax;

		if (pObj->Front == pObj->Rear) { // Check Queue full
			pObj->bFull = FALSE;
		}
		NMR_VdoEnc_Unlock_cpu(&flags);

		(&isf_vdoenc)->p_base->do_probe(&isf_vdoenc, ISF_OUT(pathID),  ISF_USER_PROBE_PULL, ISF_OK);  // [USER] PULL_OK
		return ISF_OK;
	}
}

BOOL _ISF_VdoEnc_RelaseAll_PullQueue(UINT32 pathID)
{
	ISF_DATA p_data;

	while (ISF_OK == _ISF_VdoEnc_Get_PullQueue(pathID, &p_data, 0)) ; //consume all pull queue

	return TRUE;
}

#endif // #if (SUPPORT_PULL_FUNCTION)

static BOOL _ISF_VdoEnc_ShowMsg(CHAR *strCmd)
{
	UINT32 uiPathID = 0, uiMsgLevel = 0;
	sscanf_s(strCmd, "%d %d", &uiPathID, &uiMsgLevel);

	if (uiPathID < ISF_VDOENC_IN_NUM)
		gISF_VdoEncMsgLevel[uiPathID] = uiMsgLevel;
	return TRUE;
}

static BOOL _ISF_VdoEnc_PullQInfo(CHAR *strCmd)
{
	UINT32 uiPathID = 0;
	sscanf_s(strCmd, "%d", &uiPathID);

	if (g_ve_ctx.port == NULL) {
		DBG_ERR(" NOT init yet, could not use debug command !!\r\n");
		return FALSE;
	}

	if (uiPathID < PATH_MAX_COUNT) {
		gISF_VdoEncMsgLevel[uiPathID] |= ISF_VDOENC_MSG_PULLQ;
		if (FALSE == _ISF_VdoEnc_isUserPull_AllReleased(uiPathID)) {
			DBG_ERR("[VDOENC][%d] isUserPull AllReleased fail\r\n", uiPathID);
			return FALSE;
		}
	} else {
		DBG_ERR("[VDOENC] pullq info, invalid path index = %d\r\n", uiPathID);
	}

	return TRUE;
}


// cmd debug d0 o0 /mask=000
extern void _isf_flow_debug_port(char* unit_name, char* port_name, char* mask_name);
int _isf_vdoenc_cmd_debug(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *mask;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d')) { _isf_flow_debug_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "vdoenc"); // vdoenc only has one device, and the name is "vdoenc"

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
int _isf_vdoenc_cmd_trace(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *mask;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d')) { _isf_flow_trace_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "vdoenc"); // vdoenc only has one device, and the name is "vdoenc"

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
int _isf_vdoenc_cmd_probe(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *mask;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d')) { _isf_flow_probe_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "vdoenc"); // vdoenc only has one device, and the name is "vdoenc"

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
int _isf_vdoenc_cmd_perf(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d') || (dev[1] !='0')) { _isf_flow_perf_port(0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "vdoenc"); // vdoenc only has one device, and the name is "vdoenc"

	//verify i/o
	io = strsep(&cmdstr, delimiters);  // i0, i1, i2, o0, o1, o2 ...
	if((io == NULL) || ((io[0]!='i') && (io[0]!='o'))) { _isf_flow_perf_port(0,0); return 0;}

	//do perf
	_isf_flow_perf_port(unit_name, io);

	return 0;
}

// cmd save d0 o0 count
extern void _isf_flow_save_port(char* unit_name, char* port_name, char* count_name);
int _isf_vdoenc_cmd_save(char* sub_cmd_name, char *cmd_args)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = cmd_args;
	char *dev, *io, *count;
	char unit_name[16] = {0};

	//verify dev
	dev = sub_cmd_name;  // d0
	if((dev == NULL) || (dev[0]!='d') || (dev[1] !='0')) { _isf_flow_save_port(0,0,0); return 0;}
	snprintf(unit_name, sizeof(unit_name), "vdoenc"); // vdoenc only has one device, and the name is "vdoenc"

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
static BOOL _isf_vdoenc_cmd_debug_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_vdoenc_cmd_debug(dev, cmdstr);
}

static BOOL _isf_vdoenc_cmd_trace_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_vdoenc_cmd_trace(dev, cmdstr);
}

static BOOL _isf_vdoenc_cmd_probe_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_vdoenc_cmd_probe(dev, cmdstr);
}

static BOOL _isf_vdoenc_cmd_perf_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_vdoenc_cmd_perf(dev, cmdstr);
}

static BOOL _isf_vdoenc_cmd_save_entry(CHAR *strCmd)
{
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *cmdstr = strCmd;
	char *dev = strsep(&cmdstr, delimiters); // d0
	return _isf_vdoenc_cmd_save(dev, cmdstr);
}
#endif // RTOS only

SXCMD_BEGIN(isfve, "ISF_VdoEnc command")
SXCMD_ITEM("showmsg %s", _ISF_VdoEnc_ShowMsg, "show msg (Param: PathId Level(0:Disable, 1:YUV, 2:Lock, 4:Drop, 8:TNR, 16:YUV_TMOUT, 32:YUV_interval, 64:PullQ, 128:LowLatency, 256:ISP_ratio) => showmsg 0 1)")
SXCMD_ITEM("pullqinfo %s", _ISF_VdoEnc_PullQInfo, "pullq info (Param: PathId => pullqinfo 0)")
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
SXCMD_ITEM("debug %s", _isf_vdoenc_cmd_debug_entry, "debug [dev] [i/o] [mask]  , debug port => debug d0 o0 mffff")
SXCMD_ITEM("trace %s", _isf_vdoenc_cmd_trace_entry, "trace [dev] [i/o] [mask]  , trace port => trace d0 o0 mffff")
SXCMD_ITEM("probe %s", _isf_vdoenc_cmd_probe_entry, "probe [dev] [i/o] [mask]  , probe port => probe d0 o0 meeee")
SXCMD_ITEM("perf  %s", _isf_vdoenc_cmd_perf_entry,  "perf  [dev] [i/o]         , perf  port => perf d0 i0")
SXCMD_ITEM("save  %s", _isf_vdoenc_cmd_save_entry,  "save  [dev] [i/o] [count] , save  port => save d0 i0 1")
SXCMD_ITEM("info  %s", _isf_vdoenc_api_info,        "(no param)                , show  info")
SXCMD_ITEM("top  %s", _isf_vdoenc_api_top,		"(no param) 			   , show  top")
#endif
SXCMD_END()

void _ISF_VdoEnc_InstallCmd(void)
{
	sxcmd_addtable(isfve);
}

int _isf_vdoenc_cmd_isfdbg_showhelp(void)
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

int _isf_vdoenc_cmd_isfve_showhelp(void)
{
	UINT32 cmd_num = SXCMD_NUM(isfve);
	UINT32 loop=1;
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
	DBG_DUMP("---------------------------------------------------------------------\n");
	DBG_DUMP("  venc\n");
	DBG_DUMP("---------------------------------------------------------------------\n");
#else
	DBG_DUMP("=====================================================================\n");
	DBG_DUMP("  %s\n", "isfve");
	DBG_DUMP("=====================================================================\n");
#endif
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		DBG_DUMP("%15s : %s\r\n", isfve[loop].p_name, isfve[loop].p_desc);
	}

	return 0;
}

int _isf_vdoenc_cmd_isfve(char* sub_cmd_name, char *cmd_args)
{
	UINT32 cmd_num = SXCMD_NUM(isfve);
	UINT32 loop=1;
	BOOL ret=FALSE;

	if (strncmp(sub_cmd_name, "?", 2) == 0) {
		_isf_vdoenc_cmd_isfve_showhelp();
		return 0;
	}

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(sub_cmd_name, isfve[loop].p_name, strlen(sub_cmd_name)) == 0) {
			ret = isfve[loop].p_func(cmd_args);
			return ret;
		}
	}

	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n  Usage :\r\n");
		_isf_vdoenc_cmd_isfve_showhelp();
		return -1;
	}

	return 0;
}


#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
MAINFUNC_ENTRY(venc, argc, argv)
{
	char sub_cmd_name[33] = {0};
	char cmd_args[256]    = {0};
	UINT8 i = 0;

	if (argc == 1) {
		_isf_vdoenc_cmd_isfve_showhelp();
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

	_isf_vdoenc_cmd_isfve(sub_cmd_name, cmd_args);

	return 0;
}
#endif

//----------------------------------------------------------------------------

static UINT32 _isf_vdoenc_query_context_size(void)
{
	return (PATH_MAX_COUNT * sizeof(VDOENC_CONTEXT_PORT));
}
extern UINT32 _nmr_vdoenc_query_context_size(void);
extern UINT32 _nmr_vdotrig_query_context_size(void);

static UINT32 _isf_vdoenc_query_kdrv_h264_context_size(void)
{
#if defined(_BSP_NA51000_)
	MP_VDOENC_CTX_SIZE h264_ctx_size;

	(MP_H264Enc_getVideoObject((MP_VDOENC_ID)0)->GetInfo)(MP_VDOENC_GETINFO_CTX_SIZE, (UINT32 *)&h264_ctx_size, 0, 0);
	return (PATH_MAX_COUNT * h264_ctx_size.size);
#else
	return 0;
#endif
}

static UINT32 _isf_vdoenc_query_kdrv_h265_context_size(void)
{
#if defined(_BSP_NA51000_)
	MP_VDOENC_CTX_SIZE h265_ctx_size;

	(MP_H265Enc_getVideoObject((MP_VDOENC_ID)0)->GetInfo)(MP_VDOENC_GETINFO_CTX_SIZE, (UINT32 *)&h265_ctx_size, 0, 0);
	return (PATH_MAX_COUNT * h265_ctx_size.size);
#else
	return 0;
#endif
}

static UINT32 _isf_vdoenc_query_kdrv_mjpg_context_size(void)
{
	return 0;
}

static UINT32 _isf_vdoenc_query_context_memory(void)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;

	// kflow
	p_ctx_c->ctx_mem.isf_vdoenc_size = _isf_vdoenc_query_context_size();
	p_ctx_c->ctx_mem.nm_vdoenc_size  = _nmr_vdoenc_query_context_size();
	p_ctx_c->ctx_mem.nm_vdotrig_size = _nmr_vdotrig_query_context_size();

	// kdrv
	p_ctx_c->ctx_mem.kdrv_h264_size  = _isf_vdoenc_query_kdrv_h264_context_size();
	p_ctx_c->ctx_mem.kdrv_h265_size  = _isf_vdoenc_query_kdrv_h265_context_size();
	p_ctx_c->ctx_mem.kdrv_mjpg_size  = _isf_vdoenc_query_kdrv_mjpg_context_size();

	return (p_ctx_c->ctx_mem.isf_vdoenc_size) + (p_ctx_c->ctx_mem.nm_vdoenc_size) + (p_ctx_c->ctx_mem.nm_vdotrig_size) + \
	       (p_ctx_c->ctx_mem.kdrv_h264_size)  + (p_ctx_c->ctx_mem.kdrv_h265_size) + (p_ctx_c->ctx_mem.kdrv_mjpg_size);
}

//----------------------------------------------------------------------------

static void _isf_vdoenc_assign_context(UINT32 addr)
{
	g_ve_ctx.port = (VDOENC_CONTEXT_PORT*)addr;
}
extern void _nmr_vdoenc_assign_context(UINT32 addr);
extern void _nmr_vdotrig_assign_context(UINT32 addr);

static void _isf_vdoenc_assign_kdrv_h264_context(UINT32 addr)
{
#if defined(_BSP_NA51000_)
	MP_VDOENC_CTX_INFO h264_ctx_info = {0};
	h264_ctx_info.uiNumber = PATH_MAX_COUNT;
	h264_ctx_info.uiAddr   = addr;
	(MP_H264Enc_getVideoObject((MP_VDOENC_ID)0)->SetInfo)(MP_VDOENC_SETINFO_CTX, (UINT32)&h264_ctx_info, 0, 0);
#endif
	return;
}

static void _isf_vdoenc_assign_kdrv_h265_context(UINT32 addr)
{
#if defined(_BSP_NA51000_)
	MP_VDOENC_CTX_INFO h265_ctx_info = {0};
	h265_ctx_info.uiNumber = PATH_MAX_COUNT;
	h265_ctx_info.uiAddr   = addr;
	(MP_H265Enc_getVideoObject((MP_VDOENC_ID)0)->SetInfo)(MP_VDOENC_SETINFO_CTX, (UINT32)&h265_ctx_info, 0, 0);
#endif
	return;
}

static void _isf_vdoenc_assign_kdrv_mjpg_context(UINT32 addr)
{
	return;
}

static ER _isf_vdoenc_assign_context_memory_range(UINT32 addr)
{
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;

	// [ isf_vdoenc ]
	_isf_vdoenc_assign_context(addr);
	addr += p_ctx_c->ctx_mem.isf_vdoenc_size;

	// [ nmedia_vdoenc ]
	_nmr_vdoenc_assign_context(addr);
	addr += p_ctx_c->ctx_mem.nm_vdoenc_size;

	// [ nmedia_vdotrig ]
	_nmr_vdotrig_assign_context(addr);
	addr += p_ctx_c->ctx_mem.nm_vdotrig_size;

	// [ kdrv_h264 ]
	_isf_vdoenc_assign_kdrv_h264_context(addr);
	addr += p_ctx_c->ctx_mem.kdrv_h264_size;

	// [ kdrv_h265 ]
	_isf_vdoenc_assign_kdrv_h265_context(addr);
	addr += p_ctx_c->ctx_mem.kdrv_h265_size;

	// [ kdrv_mjpg ]
	_isf_vdoenc_assign_kdrv_mjpg_context(addr);
	//addr += p_ctx_c->ctx_mem.kdrv_mjpg_size;  // CID 137176 (#1 of 1): Unused value (UNUSED_VALUE), comment it now

	return E_OK;
}

//----------------------------------------------------------------------------

static void _isf_vdoenc_free_context(void)
{
	g_ve_ctx.port = NULL;
}
extern void _nmr_vdoenc_free_context(void);
extern void _nmr_vdotrig_free_context(void);

static ER _isf_vdoenc_free_context_memory_range(void)
{
	// [ isf_vdoenc ]
	_isf_vdoenc_free_context();

	// [ nmedia_vdoenc ]
	_nmr_vdoenc_free_context();

	// [ nmedia_vdotrig ]
	_nmr_vdotrig_free_context();

	return E_OK;
}

//----------------------------------------------------------------------------

static ISF_RV _isf_vdoenc_do_init(UINT32 h_isf, UINT32 path_max_count)
{
	ISF_RV rv = ISF_OK;
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;

	if (h_isf > 0) { //client process
		if (!g_vdoenc_init[0]) { //is master process already init?
			rv = ISF_ERR_INIT; //not allow init from client process!
			goto _VE_INIT_ERR;
		}
		g_vdoenc_init[h_isf] = 1; //set init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_vdoenc_init[i]) { //client process still alive?
				rv = ISF_ERR_INIT; //not allow init from main process!
				goto _VE_INIT_ERR;
			}
		}
	}

	if (path_max_count > 0) {
		//update count
		PATH_MAX_COUNT = path_max_count;
		if (PATH_MAX_COUNT > VDOENC_MAX_PATH_NUM) {
			DBG_WRN("path max count %d > max %d!\r\n", PATH_MAX_COUNT, VDOENC_MAX_PATH_NUM);
			PATH_MAX_COUNT = VDOENC_MAX_PATH_NUM;
		}
	}

	if (p_ctx_c->ctx_mem.ctx_addr != 0) {
		DBG_ERR("[ISF_VDOENC] already init !!\r\n");
		rv = ISF_ERR_INIT;
		goto _VE_INIT_ERR;
	}

	{
		UINT32 buf_addr = 0;
		UINT32 buf_size = _isf_vdoenc_query_context_memory();

		nvtmpp_vb_add_module(isf_vdoenc.unit_module);
		//alloc context of all port
		buf_addr = (&isf_vdoenc)->p_base->do_new_i(&isf_vdoenc, NULL, "ctx", buf_size, &(p_ctx_c->ctx_mem.ctx_memblk));

		if (buf_addr == 0) {
			DBG_ERR("[ISF_VDOENC] alloc context memory fail !!\r\n");
			rv = ISF_ERR_HEAP;
			goto _VE_INIT_ERR;
		}
		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_vdoenc, ISF_CTRL, "(alloc context memory) (%u, 0x%08x, %u)", PATH_MAX_COUNT, buf_addr, buf_size);

		p_ctx_c->ctx_mem.ctx_addr = buf_addr;

		// reset memory
		memset((void *)buf_addr, 0, buf_size);

		// assign memory range
		_isf_vdoenc_assign_context_memory_range(p_ctx_c->ctx_mem.ctx_addr);

#ifdef __KERNEL__
		if (kdrv_builtin_is_fastboot()) {
			UINT32 i = 0;
			p_ctx_c->bIs_fastboot = TRUE;
			for (i = 0; i < BUILTIN_VDOENC_PATH_ID_MAX; i++) {
				VdoEnc_Builtin_GetParam(i, BUILTIN_VDOENC_INIT_PARAM_ENC_EN, &p_ctx_c->bIs_fastboot_enc[i]);
			}
		}
#endif

		//install each device's kernel object
		isf_vdoenc_install_id();
		nmr_vdoenc_install_id();

		g_vdoenc_init[h_isf] = 1; //set init
		return ISF_OK;
	}

_VE_INIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("init fail! %d\r\n", rv);
	}
	return rv;
}

static ISF_RV _isf_vdoenc_do_uninit(UINT32 h_isf, UINT32 reset)
{
	ISF_RV rv = ISF_OK;
	VDOENC_CONTEXT_COMMON *p_ctx_c = (VDOENC_CONTEXT_COMMON *)&g_ve_ctx.comm;

	if (h_isf > 0) { //client process
		if (!g_vdoenc_init[0]) { //is master process already init?
			rv = ISF_ERR_UNINIT; //not allow uninit from client process!
			goto _VE_UNINIT_ERR;
		}
		g_vdoenc_init[h_isf] = 0; //clear init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_vdoenc_init[i] && !reset) { //client process still alive?
				rv = ISF_ERR_UNINIT; //not allow uninit from main process!
				goto _VE_UNINIT_ERR;
			}
			if (reset) {
				g_vdoenc_init[i] = 0; //force clear client
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
		DBG_IND("unit(%s) => clear state\r\n", (&isf_vdoenc)->unit_name);   // equal to call UpdatePort - STOP
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_vdoenc)->p_base->do_clear_state(&isf_vdoenc, oport);
		}

		DBG_IND("unit(%s) => clear bind\r\n", (&isf_vdoenc)->unit_name);    // equal to call UpdatePort - CLOSE
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_vdoenc)->p_base->do_clear_bind(&isf_vdoenc, oport);
		}

		DBG_IND("unit(%s) => clear context\r\n", (&isf_vdoenc)->unit_name); // equal to call UpdatePort - Reset
		for(i = 0; i < PATH_MAX_COUNT; i++) {
			UINT32 oport = i + ISF_OUT_BASE;
			(&isf_vdoenc)->p_base->do_clear_ctx(&isf_vdoenc, oport);
		}


		//uninstall each device's kernel object
		isf_vdoenc_uninstall_id();
		nmr_vdoenc_uninstall_id();

		//free context of all port
		if (p_ctx_c->ctx_mem.ctx_memblk.h_data != 0) {
			rv = (&isf_vdoenc)->p_base->do_release_i(&isf_vdoenc, &(p_ctx_c->ctx_mem.ctx_memblk), ISF_OK);
			if (rv == ISF_OK) {
				memset((void*)&p_ctx_c->ctx_mem, 0, sizeof(VDOENC_CTX_MEM));  // reset ctx_mem to 0
			} else {
				DBG_ERR("[ISF_VDOENC] free context memory fail !!\r\n");
				goto _VE_UNINIT_ERR;
			}
		} else {
			DBG_ERR("[ISF_VDOENC] context h_data == NULL !!\r\n");
			rv = ISF_ERR_INVALID_DATA_ID;
			goto _VE_UNINIT_ERR;
		}

		// reset NULL to all pointer
		_isf_vdoenc_free_context_memory_range();

		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_vdoenc, ISF_CTRL, "(release context memory)");

		g_vdoenc_init[h_isf] = 0; //clear init
		return ISF_OK;
	}

_VE_UNINIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("uninit fail! %d\r\n", rv);
	}
	return rv;
}

ISF_RV _isf_vdoenc_do_command(UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2)
{
	//DBG_IND("(do_command) cmd = %u, p0 = 0x%08x, p1 = %u, p2 = %u\r\n", cmd, p0, p1, p2);
	UINT32 h_isf = (cmd >> 28); //extract h_isf
	cmd &= ~0xf0000000; //clear h_isf
	switch(cmd) {
	case 1: //INIT
		g_vdoenc_gdc_first = ((p2 & 0x00000010) >> 4);
#if defined(_BSP_NA51055_)
		if (!g_vdoenc_gdc_first) {
			DBG_DUMP("venc init: config tile cut if w>2048 (LL fast)");
		} else {
			DBG_DUMP("venc init: config tile cut if w>1280 (LL slow)");
		}
#endif
		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_vdoenc, ISF_CTRL, "(do_init) h_isf = %d, max_path = %u, gdc_first = %u", h_isf, p1, g_vdoenc_gdc_first);
		nvtmpp_vb_add_module(MAKE_NVTMPP_MODULE('v', 'd', 'o', 'e', 'n', 'c', 0, 0));
		return _isf_vdoenc_do_init(h_isf, p1); //p1: max path count
	case 0: //UNINIT
		g_vdoenc_gdc_first = 0;
		ISF_UNIT_TRACE(ISF_OP_PARAM_CTRL, &isf_vdoenc, ISF_CTRL, "(do_uninit) h_isf = %d", h_isf);
		return _isf_vdoenc_do_uninit(h_isf, p1); //p1: is under reset
	default:
		DBG_ERR("unsupport cmd(%u) !!\r\n", cmd);
		break;
	}
	return ISF_ERR_NOT_SUPPORT;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static ISF_PORT_CAPS ISF_VdoEnc_Input_Caps = {
	.connecttype_caps = ISF_CONNECT_PUSH|ISF_CONNECT_SYNC,
	.do_push          = _ISF_VdoEnc_InputPort_PushImgBufToDest,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN1 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(0), //ISF_IN1,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN2 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(1), //ISF_IN2,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN3 = {
	.sign = ISF_SIGN_PORT,
	.iport =  ISF_IN(2), //ISF_IN3,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN4 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(3), //ISF_IN4,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

#if (ISF_VDOENC_IN_NUM == 16)
static ISF_PORT ISF_VdoEnc_InputPort_IN5 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(4), //ISF_IN5,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN6 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(5), //ISF_IN6,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN7 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(6), //ISF_IN7,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN8 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(7), //ISF_IN8,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN9 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(8), //ISF_IN9,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN10 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(9), //ISF_IN10,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN11 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(10), //ISF_IN11,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN12 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(11), //ISF_IN12,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN13 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(12), //ISF_IN13,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN14 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(13), //ISF_IN14,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN15 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(14), //ISF_IN15,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};

static ISF_PORT ISF_VdoEnc_InputPort_IN16 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(15), //ISF_IN16,
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoenc,
	.p_srcunit = NULL,
};
#endif

static ISF_PORT *ISF_VdoEnc_InputPortList[ISF_VDOENC_IN_NUM] = {
	&ISF_VdoEnc_InputPort_IN1,
	&ISF_VdoEnc_InputPort_IN2,
	&ISF_VdoEnc_InputPort_IN3,
	&ISF_VdoEnc_InputPort_IN4,
#if (ISF_VDOENC_IN_NUM == 16)
	&ISF_VdoEnc_InputPort_IN5,
	&ISF_VdoEnc_InputPort_IN6,
	&ISF_VdoEnc_InputPort_IN7,
	&ISF_VdoEnc_InputPort_IN8,
	&ISF_VdoEnc_InputPort_IN9,
	&ISF_VdoEnc_InputPort_IN10,
	&ISF_VdoEnc_InputPort_IN11,
	&ISF_VdoEnc_InputPort_IN12,
	&ISF_VdoEnc_InputPort_IN13,
	&ISF_VdoEnc_InputPort_IN14,
	&ISF_VdoEnc_InputPort_IN15,
	&ISF_VdoEnc_InputPort_IN16,
#endif
};

static ISF_PORT_CAPS *ISF_VdoEnc_InputPortList_Caps[ISF_VDOENC_IN_NUM] = {
	&ISF_VdoEnc_Input_Caps,
	&ISF_VdoEnc_Input_Caps,
	&ISF_VdoEnc_Input_Caps,
	&ISF_VdoEnc_Input_Caps,
#if (ISF_VDOENC_IN_NUM == 16)
	&ISF_VdoEnc_Input_Caps,
	&ISF_VdoEnc_Input_Caps,
	&ISF_VdoEnc_Input_Caps,
	&ISF_VdoEnc_Input_Caps,
	&ISF_VdoEnc_Input_Caps,
	&ISF_VdoEnc_Input_Caps,
	&ISF_VdoEnc_Input_Caps,
	&ISF_VdoEnc_Input_Caps,
	&ISF_VdoEnc_Input_Caps,
	&ISF_VdoEnc_Input_Caps,
	&ISF_VdoEnc_Input_Caps,
	&ISF_VdoEnc_Input_Caps,
#endif
};

#if (SUPPORT_PULL_FUNCTION)
static ISF_PORT_CAPS ISF_VdoEnc_OutputPort_Caps = {
	.connecttype_caps = ISF_CONNECT_PUSH  | ISF_CONNECT_PULL | ISF_CONNECT_SYNC,
	.do_pull = _ISF_VdoEnc_OutputPort_PullImgBufFromSrc,
};
#else
static ISF_PORT_CAPS ISF_VdoEnc_OutputPort_Caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_pull = NULL,
};
#endif

static ISF_STATE  ISF_VdoEnc_OutputPort_State[ISF_VDOENC_OUT_NUM] = {0};
static ISF_STATE *ISF_VdoEnc_OutputPortList_State[ISF_VDOENC_OUT_NUM] = {
	&ISF_VdoEnc_OutputPort_State[0],
	&ISF_VdoEnc_OutputPort_State[1],
	&ISF_VdoEnc_OutputPort_State[2],
	&ISF_VdoEnc_OutputPort_State[3],
#if (ISF_VDOENC_OUT_NUM == 16)
	&ISF_VdoEnc_OutputPort_State[4],
	&ISF_VdoEnc_OutputPort_State[5],
	&ISF_VdoEnc_OutputPort_State[6],
	&ISF_VdoEnc_OutputPort_State[7],
	&ISF_VdoEnc_OutputPort_State[8],
	&ISF_VdoEnc_OutputPort_State[9],
	&ISF_VdoEnc_OutputPort_State[10],
	&ISF_VdoEnc_OutputPort_State[11],
	&ISF_VdoEnc_OutputPort_State[12],
	&ISF_VdoEnc_OutputPort_State[13],
	&ISF_VdoEnc_OutputPort_State[14],
	&ISF_VdoEnc_OutputPort_State[15],
#endif
};

static ISF_INFO isf_vdoenc_outputinfo_in[ISF_VDOENC_IN_NUM] = {0};
static ISF_INFO *isf_vdoenc_outputinfolist_in[ISF_VDOENC_IN_NUM] = {
	&isf_vdoenc_outputinfo_in[0],
	&isf_vdoenc_outputinfo_in[1],
	&isf_vdoenc_outputinfo_in[2],
	&isf_vdoenc_outputinfo_in[3],
#if (ISF_VDOENC_IN_NUM == 16)
	&isf_vdoenc_outputinfo_in[4],
	&isf_vdoenc_outputinfo_in[5],
	&isf_vdoenc_outputinfo_in[6],
	&isf_vdoenc_outputinfo_in[7],
	&isf_vdoenc_outputinfo_in[8],
	&isf_vdoenc_outputinfo_in[9],
	&isf_vdoenc_outputinfo_in[10],
	&isf_vdoenc_outputinfo_in[11],
	&isf_vdoenc_outputinfo_in[12],
	&isf_vdoenc_outputinfo_in[13],
	&isf_vdoenc_outputinfo_in[14],
	&isf_vdoenc_outputinfo_in[15],
#endif
};

static ISF_INFO isf_vdoenc_outputinfo_out[ISF_VDOENC_OUT_NUM] = {0};
static ISF_INFO *isf_vdoenc_outputinfolist_out[ISF_VDOENC_OUT_NUM] = {
	&isf_vdoenc_outputinfo_out[0],
	&isf_vdoenc_outputinfo_out[1],
	&isf_vdoenc_outputinfo_out[2],
	&isf_vdoenc_outputinfo_out[3],
#if (ISF_VDOENC_OUT_NUM == 16)
	&isf_vdoenc_outputinfo_out[4],
	&isf_vdoenc_outputinfo_out[5],
	&isf_vdoenc_outputinfo_out[6],
	&isf_vdoenc_outputinfo_out[7],
	&isf_vdoenc_outputinfo_out[8],
	&isf_vdoenc_outputinfo_out[9],
	&isf_vdoenc_outputinfo_out[10],
	&isf_vdoenc_outputinfo_out[11],
	&isf_vdoenc_outputinfo_out[12],
	&isf_vdoenc_outputinfo_out[13],
	&isf_vdoenc_outputinfo_out[14],
	&isf_vdoenc_outputinfo_out[15],
#endif
};

static ISF_PORT *ISF_VdoEnc_OutputPortList[ISF_VDOENC_OUT_NUM] = {0};
static ISF_PORT *ISF_VdoEnc_OutputPortList_Cfg[ISF_VDOENC_OUT_NUM] = {0};
static ISF_PORT_CAPS *ISF_VdoEnc_OutputPortList_Caps[ISF_VDOENC_OUT_NUM] = {
	&ISF_VdoEnc_OutputPort_Caps,
	&ISF_VdoEnc_OutputPort_Caps,
	&ISF_VdoEnc_OutputPort_Caps,
	&ISF_VdoEnc_OutputPort_Caps,
#if (ISF_VDOENC_OUT_NUM == 16)
	&ISF_VdoEnc_OutputPort_Caps,
	&ISF_VdoEnc_OutputPort_Caps,
	&ISF_VdoEnc_OutputPort_Caps,
	&ISF_VdoEnc_OutputPort_Caps,
	&ISF_VdoEnc_OutputPort_Caps,
	&ISF_VdoEnc_OutputPort_Caps,
	&ISF_VdoEnc_OutputPort_Caps,
	&ISF_VdoEnc_OutputPort_Caps,
	&ISF_VdoEnc_OutputPort_Caps,
	&ISF_VdoEnc_OutputPort_Caps,
	&ISF_VdoEnc_OutputPort_Caps,
	&ISF_VdoEnc_OutputPort_Caps,
#endif
};

static ISF_DATA_CLASS _isf_vdoenc_base = {
	.this = MAKEFOURCC('V','E','T','M'),
	.base = MAKEFOURCC('V','S','T','M'),
	.do_lock   = _ISF_VdoEnc_LockCB,
	.do_unlock = _ISF_VdoEnc_UnLockCB,
};

static ISF_RV _isf_vdoenc_bs_data_init(void)
{
	//[isf_stream] return _isf_unit_init_data(&gISF_VdoEncBsData, &_isf_vdoenc_base, MAKEFOURCC('V','S','T','M'), 0x00010000);

	//return isf_vdoenc.p_base->init_data(&gISF_VdoEncBsData, &_isf_vdoenc_base, MAKEFOURCC('V','S','T','M'), 0x00010000); // [isf_flow] use this ...

#if 0
	isf_data_regclass(&_isf_vdoenc_base);
	return isf_vdoenc.p_base->init_data(&gISF_VdoEncBsData, NULL, MAKEFOURCC('V','E','T','M'), 0x00010000);
#else
	_isf_vdoenc_base.this = 0;
	{
		int i;
		for (i=0; i < ISF_VDOENC_OUT_NUM ; i++) {
			isf_vdoenc.p_base->init_data(&gISF_VdoEncBsData[i], &_isf_vdoenc_base, _isf_vdoenc_base.base, 0x00010000);
			isf_vdoenc.p_base->init_data(&gISF_VdoEncBsDataPartial[i], &_isf_vdoenc_base, _isf_vdoenc_base.base, 0x00010000);
		}
	}
	_isf_vdoenc_base.this = MAKEFOURCC('V','E','T','M');
	isf_data_regclass(&_isf_vdoenc_base);
	return E_OK;
#endif
}

/*
ISF_DATA gISF_VdoEncBsData = {
	.sign      = ISF_SIGN_DATA,            ///< signature, equal to ISF_SIGN_DATA or ISF_SIGN_EVENT
	///.pLockCB   = _ISF_VdoEnc_LockCB,   ///< CB to lock "custom data handle"
	///.pUnlockCB = _ISF_VdoEnc_UnLockCB, ///< CB to unlock "custom data handle"
	.p_class   = &_isf_vdoenc_base,
#if _TODO
	.Event     = 0, ///< default 0
	.Serial    = 0, ///< serial id
	.TimeStamp = 0, ///< time-stamp
#endif
};
*/
ISF_UNIT isf_vdoenc;
static ISF_PORT_PATH ISF_VdoEnc_PathList[ISF_VDOENC_IN_NUM] = {
	{&isf_vdoenc, ISF_IN(0),  ISF_OUT(0)},
	{&isf_vdoenc, ISF_IN(1),  ISF_OUT(1)},
	{&isf_vdoenc, ISF_IN(2),  ISF_OUT(2)},
	{&isf_vdoenc, ISF_IN(3),  ISF_OUT(3)},
#if (ISF_VDOENC_IN_NUM == 16)
	{&isf_vdoenc, ISF_IN(4),  ISF_OUT(4)},
	{&isf_vdoenc, ISF_IN(5),  ISF_OUT(5)},
	{&isf_vdoenc, ISF_IN(6),  ISF_OUT(6)},
	{&isf_vdoenc, ISF_IN(7),  ISF_OUT(7)},
	{&isf_vdoenc, ISF_IN(8),  ISF_OUT(8)},
	{&isf_vdoenc, ISF_IN(9),  ISF_OUT(9)},
	{&isf_vdoenc, ISF_IN(10), ISF_OUT(10)},
	{&isf_vdoenc, ISF_IN(11), ISF_OUT(11)},
	{&isf_vdoenc, ISF_IN(12), ISF_OUT(12)},
	{&isf_vdoenc, ISF_IN(13), ISF_OUT(13)},
	{&isf_vdoenc, ISF_IN(14), ISF_OUT(14)},
	{&isf_vdoenc, ISF_IN(15), ISF_OUT(15)},
#endif
};

ISF_UNIT isf_vdoenc = {
	.sign = ISF_SIGN_UNIT,
	.unit_name = "vdoenc",
	.unit_module = MAKE_NVTMPP_MODULE('v', 'd', 'o', 'e', 'n', 'c', 0, 0),
	.port_in_count = ISF_VDOENC_IN_NUM,
	.port_out_count = ISF_VDOENC_OUT_NUM,
	.port_path_count = ISF_VDOENC_IN_NUM,
	.port_in = ISF_VdoEnc_InputPortList,
	.port_out = ISF_VdoEnc_OutputPortList,
	.port_outcfg = ISF_VdoEnc_OutputPortList_Cfg,
	.port_incaps = ISF_VdoEnc_InputPortList_Caps,
	.port_outcaps = ISF_VdoEnc_OutputPortList_Caps,
	.port_outstate = ISF_VdoEnc_OutputPortList_State,
	.port_ininfo = isf_vdoenc_outputinfolist_in,
	.port_outinfo = isf_vdoenc_outputinfolist_out,
	.port_path = ISF_VdoEnc_PathList,
	.do_command = _isf_vdoenc_do_command,
	.do_bindinput = ISF_VdoEnc_BindInput,
	.do_bindoutput = ISF_VdoEnc_BindOutput,
#if 0
	.SetParam = ISF_VdoEnc_SetParam,
	.GetParam = ISF_VdoEnc_GetParam,
#endif
	.do_setportparam  = ISF_VdoEnc_SetPortParam,
	.do_getportparam  = ISF_VdoEnc_GetPortParam,
	.do_setportstruct = ISF_VdoEnc_SetPortStruct,
	.do_getportstruct = ISF_VdoEnc_GetPortStruct,
	.do_update = ISF_VdoEnc_UpdatePort,
};

///////////////////////////////////////////////////////////////////////////////

