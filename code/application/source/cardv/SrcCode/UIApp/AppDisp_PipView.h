#ifndef _PIPVIEW_H
#define _PIPVIEW_H

#include "hd_type.h"

#define REGION_MATCH_IMG	NULL	///< use in region w,h,x,y match image buffer

typedef struct {
	HD_VIDEO_FRAME			*p_src_img[4];
	HD_VIDEO_FRAME			*p_dst_img;
	UINT32					viewcnt;
} APPDISP_VIEW_DRAW;

#if 0
typedef struct {
	UINT32					w;
	UINT32					h;
} APPDISP_VIEW_SCALE;

typedef struct {
	APPDISP_VIEW_SCALE		scale[4];
} APPDISP_VIEW_INFO;


typedef void  APPDISP_VIEW_SETSTYLE_CB(UINT32 uiStyle);
typedef INT32 APPDISP_VIEW_GETINFO_CB(APPDISP_VIEW_INFO *info);
typedef INT32 APPDISP_VIEW_DRAW_CB(APPDISP_VIEW_DRAW *pDraw);

typedef struct {
	APPDISP_VIEW_SETSTYLE_CB         *SetStyle;
	APPDISP_VIEW_GETINFO_CB          *GetInfo;
	APPDISP_VIEW_DRAW_CB             *OnDraw;
} APPDISP_VIEW_OPS;
#endif

extern void		PipView_SetStyle(UINT32 uiStyle);
//extern INT32	PipView_GetInfo(APPDISP_VIEW_INFO *info);
extern INT32	PipView_OnDraw(APPDISP_VIEW_DRAW *pDraw);
#if (DUALCAM_PIP_BEHIND_FLIP == ENABLE)
extern UINT32	PipView_BFLIP_GetBufAddr(UINT32 blk_size);
extern void		PipView_BFLIP_DestroyBuff(void);
extern void		PipView_BFLIP_SetBuffer(UINT32 uiAddr, UINT32 uiSize);
#endif
extern INT32 PipView_CopyData(HD_VIDEO_FRAME *pSrcImg, IRECT *pSrcRegion, HD_VIDEO_FRAME *pDstImg, IRECT *pDstRegion, UINT32 bFlush);
extern INT32 PipView_ScaleData(HD_VIDEO_FRAME *pSrcImg, IRECT *pSrcRegion, HD_VIDEO_FRAME *pDstImg, IRECT *pDstRegion);

#endif
