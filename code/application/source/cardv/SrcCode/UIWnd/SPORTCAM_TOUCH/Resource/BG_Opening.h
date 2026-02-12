#ifndef _BG_OPENING_H
#define _BG_OPENING_H

#include "kwrap/type.h"
#define LOGO_SIZE_W    320
#define LOGO_SIZE_H    240
#define LOGO_FMT       HD_VIDEO_PXLFMT_YUV420

#define LOGO_BS_BLK_SIZE     (LOGO_SIZE_W*LOGO_SIZE_H*2)
#define LOGO_YUV_BLK_SIZE    ALIGN_CEIL_4(((LOGO_SIZE_W) * (LOGO_SIZE_H) * HD_VIDEO_PXLFMT_BPP(LOGO_FMT)) / 8)

extern const UINT8 g_ucBGOpening[];
extern const UINT8 g_ucBGGoodbye[];
extern int Logo_getBGOpening_size(void);
extern int Logo_getBGGoodbye_size(void);

extern int Logo_getBGOpening_size(void);
extern int Logo_getBGGoodbye_size(void);

#endif
