/********************************************************************
	INCLUDE FILES
********************************************************************/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "hdal.h"
#include "hd_type.h"
#include "hd_common.h"
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include "hd_gfx.h"

#include "vendor_ai.h"
#include "kflow_ai_net/kflow_ai_net.h"

#define NN_PDCNN_DRAW           ENABLE
#define MAX_DISTANCE_MODE       2

#define PDCNN_MAX_OUTNUM        (32)

#define KBYTE                   (1024)
#define MBYTE					(1024 * 1024)

#define PD_MAX_MEM_SIZE 	    (10 * MBYTE)
#define SCALE_BUF_SIZE          (1 * MBYTE)

#define MAX_CLASS_OBJ_NUM		(1024)
#define YUV_WIDTH			960
#define YUV_HEIGHT			540
#define MAX_FRAME_WIDTH		1920
#define MAX_FRAME_HEIGHT	1080
#if (YUV_WIDTH == 1024)
#define NEON_BATCH			128
#else
#define NEON_BATCH			120 //144 or 120
#endif

#define SOURCE_PATH 			HD_VIDEOPROC_0_OUT_1
#define EXTEND_PATH				HD_VIDEOPROC_0_OUT_6
#define OSG_LCD_WIDTH       	(960)
#define OSG_LCD_HEIGHT      	(240)

#define VERSION_A  "1"
#define VERSION_B  "5"
#define VERSION_C  "3"

typedef struct _MEM_PARM {
	UINT32 pa;
	UINT32 va;
	UINT32 size;
	UINT32 blk;
} MEM_PARM;

typedef struct _BACKBONE_ONE_PRANCH_OUT {
	MEM_PARM cls_prob_layer;
	MEM_PARM bbox_pred_layer;	
} BACKBONE_ONE_PRANCH_OUT;

typedef struct _BACKBONE_OUT {
	BACKBONE_ONE_PRANCH_OUT stride32;
	BACKBONE_ONE_PRANCH_OUT stride16;
	BACKBONE_ONE_PRANCH_OUT stride8;
} BACKBONE_OUT;

typedef struct _LIMIT_PARAM {
	FLOAT score_thrs[25];
	INT32 box_sizes[25];
	INT32 limit_size;
	INT32 sm_thr_num;
	FLOAT k[25];
	INT32 limit_module;
	UINT32 max_distance;
} LIMIT_PARAM;


typedef struct _PROPOSAL_PARAM {
	INT32 backbone_out_shifts[3][2];
	INT32 backbone_out_sizes[3][2];
	INT32 backbone_out_indexs[3][2];
	FLOAT anchor_params[3][8][4];
	INT32 strides[3];
	INT32 input_size[2];
	FLOAT score_thres;
	FLOAT nms_thres;
	FLOAT ratiow;
	FLOAT ratioh;
	UINT32 run_id;
	INT32 open_pdout;
	INT32 cont_pre_rslt;
} PROPOSAL_PARAM;

typedef struct _PDCNN_RESULT
{
	INT32   category;
	FLOAT 	score;
	FLOAT 	x1;
	FLOAT 	y1;
	FLOAT 	x2;
	FLOAT 	y2;
}PDCNN_RESULT;

typedef struct _PD_IRECT {
	FLOAT  x1;                           ///< x coordinate of the top-left point of the rectangle
	FLOAT  y1;                           ///< y coordinate of the top-left point of the rectangle
	FLOAT  x2;                           ///< rectangle width
	FLOAT  y2;                           ///< rectangle height
} PD_IRECT;

typedef struct _PDCNN_MEM {
    INT32 out_num;
	MEM_PARM io_mem;
	MEM_PARM model_mem;
	MEM_PARM input_mem;
	MEM_PARM pps_result;
    MEM_PARM out_result;
	MEM_PARM final_result;
	MEM_PARM backbone_output;
	MEM_PARM reshape_cls;
	MEM_PARM scale_buf;
} PDCNN_MEM;

VOID pdcnn_get_version(VOID);
HD_RESULT pdcnn_version_check(MEM_PARM *buf);
VOID gfx_img_to_vendor(VENDOR_AI_BUF* out_img, HD_GFX_IMG_BUF* in_img, UINT32 va);
HD_RESULT ai_crop_img(HD_GFX_IMG_BUF *dst_img, VENDOR_AI_BUF *src_img, HD_GFX_SCALE_QUALITY method, PD_IRECT *roi);
HD_RESULT pdcnn_init(PROPOSAL_PARAM* proposal_params, BACKBONE_OUT* backbone_outputs, LIMIT_PARAM* limit_para, CHAR *para_file);
HD_RESULT pdcnn_process(PROPOSAL_PARAM* proposal_params, PDCNN_MEM* pdcnn_mem, LIMIT_PARAM* limit_para, BACKBONE_OUT* backbone_outputs, VENDOR_AI_BUF* src_img, UINT32 max_distance);
VOID print_pdcnn_results(PDCNN_MEM* pdcnn_mem, FLOAT ratiow, FLOAT ratioh);
HD_RESULT get_pd_mem(MEM_PARM *buf, MEM_PARM *req_mem, UINT32 req_size, UINT32 align_size);
HD_RESULT get_post_mem(MEM_PARM* buf, PDCNN_MEM* pd_mem);

