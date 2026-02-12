#ifndef _UIDISPLAY_H
#define _UIDISPLAY_H

#include <kwrap/type.h>

#define OSD_ATTR_ROTATE          0

#define OSD_ROTATE_0_DEGREE      HD_VIDEO_DIR_ROTATE_0
#define OSD_ROTATE_90_DEGREE     HD_VIDEO_DIR_ROTATE_90
#define OSD_ROTATE_180_DEGREE    HD_VIDEO_DIR_ROTATE_180
#define OSD_ROTATE_270_DEGREE    HD_VIDEO_DIR_ROTATE_270

extern void View_Window_ConfigAttr(UINT32 did, UINT32 attr, UINT32 value);
extern DC *View_Window_BeginRotate(DC *pDC);
extern void View_Window_EndRotate(void);

#endif // _UIDISPLAY_H

