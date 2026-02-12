
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "hdal.h"



#define ARBG_8888_RED			0xFFFF0000
#define ARBG_8888_GREEN			0xFF00FF00
#define ARBG_8888_BLUE			0xFF0000FF
#define ARBG_8888_PINK	  		0xFFFF00FF
#define ARBG_8888_PINK_50	  	0x80FF00FF  //50%

#define ARBG_4444_RED			0xFF00
#define ARBG_4444_GREEN			0xF0F0
#define ARBG_4444_BLUE			0xF00F
#define ARBG_4444_PINK	  		0xFF0F
#define ARBG_4444_PINK_50	  	0x8F0F      //50%

#define ARBG_1555_RED			0xFC00
#define ARBG_1555_GREEN			0x83E0
#define ARBG_1555_BLUE			0x801F
#define ARBG_1555_PINK	  		0xFC1F

// ARGB is 2 plane
#define ARBG_565_RED			0xF800
#define ARBG_565_GREEN			0x07E0
#define ARBG_565_BLUE			0x001F
#define ARBG_565_PINK	  		0xF81F

#define INDEX8_RED			    0
#define INDEX8_GREEN		    1
#define INDEX8_BLUE			    2
#define INDEX8_PINK	  		    3
#define INDEX8_PINK_50			4

typedef struct _VOUT_FBM_INIT {
    UINT32 width;
    UINT32 height;
    UINT32 format;
    UINT32 addr;
    UINT32 size;
} VOUT_FBM_INIT;


typedef struct _VOUT_FBM_OBJ {
    UINT32 width;
    UINT32 height;
    UINT32 format;
    UINT32 format_coef;
    char *buffer[2];
    UINT32 flag;
    UINT32 buf_index;
    UINT32 one_buf_size;
} VOUT_FBM_OBJ;



UINT32 vdoout_test_enable(HD_PATH_ID video_out_ctrl,UINT32 id,UINT32 bEn);
UINT32 vdoout_test_format(HD_PATH_ID video_out_ctrl,UINT32 id,UINT32 fmt);
UINT32 vdoout_test_attr(HD_PATH_ID video_out_ctrl,UINT32 id,UINT32 blend,UINT16 r,UINT16 g,UINT16 b);
UINT32 vdoout_test_fb_dim(HD_PATH_ID video_out_ctrl,HD_DIM *p_input_dim,HD_URECT *p_out_cur_win);
UINT32 vdoout_test_palette(HD_PATH_ID video_out_ctrl,UINT32 id);
UINT32 vdoout_fbm_init(VOUT_FBM_INIT *obj);
char *vdoout_fbm_getflip(void);
UINT32 vdoout_fbm_setflip(void);




