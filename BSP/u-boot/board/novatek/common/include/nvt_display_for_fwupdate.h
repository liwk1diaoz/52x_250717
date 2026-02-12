#ifndef _NVT_FW_UPDATE_DISPLAY_H_
#define _NVT_FW_UPDATE_DISPLAY_H_

#include <asm/arch/IOAddress.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/rcw_macro.h>

#define DISP_ROTATE_0   0
#define DISP_ROTATE_90  1
#define DISP_ROTATE_270 2
#define DISP_ROTATE_DIR DISP_ROTATE_0

#define DISP_CFG_TOTAL_DRAWING_BAR 1

#define DISP_DRAW_INIT                       1
#define DISP_DRAW_EXTERNAL_FRAME             2
#define DISP_DRAW_UPDATEFW_START             3
#define DISP_DRAW_UPDATING_BAR               4
#define DISP_DRAW_UPDATEFW_OK                5
#define DISP_DRAW_UPDATEFW_FAILED            6
#define DISP_DRAW_UPDATEFW_FAILED_AND_RETRY  7
#define DISP_DRAW_UPDATEFW_AGAIN             8

#define DISP_UPDATEFW_OK  (0)
#define DISP_UPDATEFW_FAILED  (-1)

extern int nvt_display_init(void);
extern int  nvt_display_decode_string(void);
extern void nvt_display_set_background(void);
extern void nvt_display_copy_pingpong_buffer(void);
extern void nvt_display_draw_init_string(void);
extern void nvt_display_draw(UINT32 u32Id);
extern void nvt_display_set_status(INT32 sts);
extern void nvt_display_config(UINT32 config_id, UINT32 vlaue);

#endif
