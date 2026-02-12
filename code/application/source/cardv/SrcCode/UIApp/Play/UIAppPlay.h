#ifndef _UIVIEWPLAY_H_
#define _UIVIEWPLAY_H_

#include "PrjInc.h"
#include "hdal.h"
#include "hd_type.h"
#include "vendor_vpe.h"

#define PB_PIXEL_FORMAT  HD_VIDEO_PXLFMT_YUV420
#if (_BOARD_DRAM_SIZE_ <= 0x04000000) /* 64 MB */
#define PB_MAX_FILE_SIZE (0x600000)
#define PB_MAX_DECODE_W  ALIGN_CEIL_64(1920)
#define PB_MAX_DECODE_H  ALIGN_CEIL_64(1080)
#define PB_MAX_RAW_SIZE  ALIGN_CEIL_64((PB_MAX_DECODE_W*PB_MAX_DECODE_H*3/2)) //YUV 420
#define PB_MAX_VIDEO_W   ALIGN_CEIL_64(1920)
#define PB_MAX_VIDEO_H   ALIGN_CEIL_64(1080)
#elif (_BOARD_DRAM_SIZE_ >= 0x10000000) /* 256 MB */
#define PB_MAX_FILE_SIZE (0x1000000)
#define PB_MAX_DECODE_W  ALIGN_CEIL_64(4032)
#define PB_MAX_DECODE_H  ALIGN_CEIL_64(3072)
#define PB_MAX_RAW_SIZE  ALIGN_CEIL_64((PB_MAX_DECODE_W*PB_MAX_DECODE_H*3/2)) //YUV 420
#define PB_MAX_VIDEO_W   ALIGN_CEIL_64(4032)
#define PB_MAX_VIDEO_H   ALIGN_CEIL_64(3072)
#else /* 64 - 256 MB */
#define PB_MAX_FILE_SIZE (0x800000)
#define PB_MAX_DECODE_W  ALIGN_CEIL_64(4032)
#define PB_MAX_DECODE_H  ALIGN_CEIL_64(3072)
#define PB_MAX_RAW_SIZE  ALIGN_CEIL_64((PB_MAX_DECODE_W*PB_MAX_DECODE_H*3/2)) //YUV 420
#define PB_MAX_VIDEO_W   ALIGN_CEIL_64(2592)
#define PB_MAX_VIDEO_H   ALIGN_CEIL_64(1944)
#endif


/**
    Movie play system event.
*/
typedef enum {
	NVTEVT_PLAYBACK_EVT_START   = APPUSER_PLAYBACK_BASE, ///< Min value = 0x14003000
	//Event for single view
	/* INSERT NEW EVENT HRER */
	//Event for thumbnial view
	/* INSERT NEW EVENT HRER */
	//Event for Photo zoom view
	/* INSERT NEW EVENT HRER */
	//Event for Photo edit view
	/* INSERT NEW EVENT HRER */
	//Event for Movie playback view
	NVTEVT_EXE_OPENPLAY         = 0x14003800,
	NVTEVT_EXE_CLOSEPLAY        = 0x14003801,
	NVTEVT_EXE_PAUSEPLAY        = 0x14003802,
	NVTEVT_EXE_RESUMEPLAY       = 0x14003803,
	NVTEVT_EXE_STARTPLAY        = 0x14003804,
	NVTEVT_EXE_FWDPLAY          = 0x14003805,
	NVTEVT_EXE_BWDPLAY          = 0x14003806,
	NVTEVT_EXE_FWDSTEPPLAY      = 0x14003807,
	NVTEVT_EXE_BWDSTEPPLAY      = 0x14003808,
	NVTEVT_EXE_SWITCH_FIRSTFRAME = 0x14003809,
	NVTEVT_EXE_SWITCH_LASTFRAME = 0x1400380a,
	NVTEVT_EXE_CHANGESIZE       = 0x1400380b,
	NVTEVT_EXE_MOVIEAUDPLAYVOLUME = 0x1400380c,
	NVTEVT_CB_MOVIE_START       = 0x140038f0, ///< Movie start.
	NVTEVT_CB_MOVIE_ONE_SEC     = 0x140038f1, ///< Posted every one second on movie recoding.
	NVTEVT_CB_MOVIE_FINISH      = 0x140038f2, ///< Movie finished.
	NVTEVT_CB_MOVIE_ONE_VIDEOFRAME = 0x140038f3, ///< The first video frame done.
	NVTEVT_CB_MOVIE_ERR 		= 0x140038f4, /* play movie error */
	/* INSERT NEW EVENT HRER */
	NVTEVT_PLAYBACK_EVT_END     = APPUSER_PLAYBACK_BASE + 0x1000 - 1, ///< Max value = 0x14003000
} NVT_MOVIEPLAY_EVENT;

//Play Init
extern INT32 PlayExe_OnOpen(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray);
extern INT32 PlayExe_OnClose(VControl *pCtrl, UINT32 paramNum, UINT32 *paramArray);

#if PLAY_DEWARP == ENABLE
extern INT32 UIAppPlay_Dewarp_Init(const HD_DIM max_dim, const VPE_DCTG_PARAM* dctg_default);
extern INT32 UIAppPlay_Dewarp_Uninit(VOID);
extern INT32 UIAppPlay_Dewarp_TouchMove(INT32 delta_x, INT delta_y);
extern INT32 UIAppPlay_Dewarp_Reset(VOID);
#endif

//extern void UIAppPlay_InstallCmd(void);

extern VControl CustomPlayObjCtrl;

/**
    Register callback function for MediaPlay_Open().

    Register callback function for the events from Media Play Object

    @param[in] event_id  define Media Play Event ID

    @return void
*/
//extern void Play_MovieCB(UINT32 event_id);

//extern void UIAppPlay_MoviePlayCB(UINT32 uiEventID);

#endif //_UIVIEWPLAY_H_
