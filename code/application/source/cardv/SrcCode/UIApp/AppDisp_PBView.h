#include "PlaybackTsk.h"
#include <kwrap/nvt_type.h>

#define PBVIEW_2PASS_W      640     ///< Horizontal pixels
#define PBVIEW_2PASS_H      480     ///< Vertical pixels

#define THUMB_DRAW_TMP_BUFFER	0	// draw thumbnail frame in display tmp buffer then copy to new buffer
#define THUMB_DRAW_NEW_BUFFER	1	// draw thumbnail frame in new display buffer

typedef struct _PBVIEW_HD_COM_BUF{
	UINT32 pa;
	UINT32 va;
	UINT32 blk_size;
	HD_COMMON_MEM_VB_BLK  blk; //block ID
}PBVIEW_HD_COM_BUF, *PPBVIEW_HD_COM_BUF;


extern void PBView_OnDrawCB(PB_VIEW_STATE view_state, HD_VIDEO_FRAME *pHdDecVdoFrame);
extern void PBView_DrawSingleView(HD_VIDEO_FRAME *pVdoSrc);
extern void PBView_DrawThumbFrame(UINT32 idx, UINT32 mode);

#if PLAY_DEWARP == ENABLE

typedef struct {
	HD_RESULT (*push_in_buf)(HD_VIDEO_FRAME *in_frame, HD_VIDEO_FRAME *out_frame);
	HD_RESULT (*pull_out_buf)(HD_VIDEO_FRAME *out_frame);
	HD_RESULT (*release_out_buf)(HD_VIDEO_FRAME *out_frame);
} PBVIEW_DEWARP_CB;

extern void PBView_DrawSingleView_Dewarp(HD_VIDEO_FRAME *pVdoSrc);
extern void PBView_SetDewarpCB(PBVIEW_DEWARP_CB cb);
#endif

/* for MoviePlay */
extern ER 	PBView_videoout_task_create(void);
extern void PBView_videoout_task_destroy(void);

#if _TODO //refer to NA51055-840 JIRA and using new method
extern void PBView_KeepLastView(void);
#endif
extern UINT32 PBView_get_hd_phy_addr(void *va);
extern HD_RESULT PBView_get_hd_common_buf(PPBVIEW_HD_COM_BUF p_hd_view_buf);
