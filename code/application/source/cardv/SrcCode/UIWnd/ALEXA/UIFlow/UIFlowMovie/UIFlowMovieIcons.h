#ifndef __UIFlowWndMovieIcons_H
#define __UIFlowWndMovieIcons_H

extern void FlowMovie_IconDrawDscMode(VControl *pCtrl);
extern void FlowMovie_IconHideDscMode(VControl *pCtrl);
extern void FlowMovie_IconDrawMaxRecTime(VControl *pCtrl);
extern void FlowMovie_IconHideMaxRecTime(VControl *pCtrl);
extern void FlowMovie_IconDrawRecTime(VControl *pCtrl);
extern void FlowMovie_IconDrawRec(VControl *pCtrl);
extern void FlowMovie_IconHideRec(VControl *pCtrl);
extern void FlowMovie_IconDrawSize(VControl *pCtrl);
extern void FlowMovie_IconHideSize(VControl *pCtrl);
extern void FlowMovie_IconDrawStorage(VControl *pCtrl);
extern void FlowMovie_IconHideStorage(VControl *pCtrl);
extern void FlowMovie_IconDrawEV(VControl *pCtrl);
extern void FlowMovie_IconHideEV(VControl *pCtrl);
extern void FlowMovie_IconDrawBattery(VControl *pCtrl);
extern void FlowMovie_IconHideBattery(VControl *pCtrl);
extern void FlowMovie_DrawPIM(BOOL bDraw);
extern void FlowMovie_IconDrawDZoom(VControl *pCtrl);
extern void FlowMovie_IconHideDZoom(VControl *pCtrl);
extern void FlowMovie_IconDrawDateTime(void);
extern void FlowMovie_IconHideDateTime(void);
extern void FlowMovie_IconDrawMotionDet(VControl *pCtrl);
extern void FlowMovie_IconHideMotionDet(VControl *pCtrl);
extern void FlowMovie_UpdateIcons(BOOL bShow);

#endif //__UIFlowWndMovieIcons_H
