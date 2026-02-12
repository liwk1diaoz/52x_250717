#ifndef __KDRV_VPE_INT_H__
#define __KDRV_VPE_INT_H__


#include "kdrv_type.h"

#include "vpe_platform.h"

#include "vpe_ll_base.h"
#include "vpe_config_base.h"
#include "vpe_plat_base.h"
#include "vpe_lib.h"
#include "vpe_compression_base.h"
#include "vpe_sca_base.h"
#include "vpe_ctl_base.h"
#include "vpe_pal_base.h"
#include "vpe_shp_base.h"
#include "vpe_dma_base.h"
#include "vpe_dce_base.h"
#include "vpe_dctg_base.h"
#include "vpe_vcache_base.h"
#include "vpe_column_cal.h"

#include "vpe_int.h"


#define  VPE_RES_LIMIT                    0
#define  VPE_CPU_UTILIZATION              0


//#define VPEDRV_DEV_NAME       "kdrv_vpe"


//#define VPEDRV_MINOR        101
//#define VPEDRV_MAGIC        'i'
//#define VPE_REG_BASE       0xd2200000
//#define VPE_REG_SZ         (1 << 20)

#define MAX_PLANE           3

#define VPEDRV_VERSION     "0.0.0"

#define VPE_COL_MAX_W         2048
#define VPE_COL_MIN_W         2048
#define COLUMN_OVERLAP           48
#define COLUMN_SCE_OVERLAP       64
#define COLUMN_MAX_WIDTH         1936
#define COLUMN_MAX_PROC_W        (COLUMN_MAX_WIDTH + COLUMN_OVERLAP)



#define vpe_err(fmt, arg...)       do {printk(KERN_ERR     "[VPE_ERR]: " fmt, ## arg);} while (0)  /* error conditions                 */

#define DEBUG_M(level,fmt,args...) { \
		if ((log_level >= level)) \
			printk("debug:"fmt, ## args);}
#define LOG_ERROR     0
#define LOG_WARNING   1
#define LOG_QUIET     2
#define LOG_DEBUG     3
#define LOG_REG       4

#define VPE_RES_OUT_DUP_NUM     1

//-------------------------------------------------------------------------------

typedef void (*VPE_INT_CB)(UINT32);

// funtion type for dal_sie
typedef ER(*KDRV_VPE_SET_FP)(UINT32, void *);
typedef ER(*KDRV_VPE_GET_FP)(UINT32, void *);

typedef enum _KDRV_VPE_ID {
	KDRV_VPE_ID_0 = 0,                                  ///< ISE id 1
	KDRV_VPE_ID_MAX_NUM,
	ENUM_DUMMY4WORD(KDRV_VPE_ID)
} KDRV_VPE_ID;





//-------------------------------------------------------------------------------


typedef struct _VPE_DRV_INT_INFO_ {
	/* hardware resource */
	UINT32 vbase;
	UINT32 pbase;
	UINT32 mem_len;
	UINT32 id;
	INT32 irq_no;

	struct resource *res;
	UINT32 driver_count;
	wait_queue_head_t vpe_wait_queue;
	unsigned int vpe_flag;

	struct tasklet_struct         job_tasklet;
	vk_spinlock_t          job_lock;
	vk_spinlock_t          linklist_lock;
} VPE_DRV_INT_INFO;

#define VPE_MAX_LLCMD_SZ            128
struct vpe_reg_hwinfo {
	UINT32 regoff;
	UINT32 val;
	UINT32 llcmd_id;
};

struct vpe_llcmd_info {
	unsigned short llcmd_id[VPE_MAX_LLCMD_SZ];
	unsigned short llcmd_jobid[VPE_MAX_LLCMD_SZ];
	UINT32 llcmd_num;
	UINT32 llcmd_job_num;
};

typedef enum {
	STATUS_FRAME_DONE  = 3,
	STATUS_LL_DONE  = 7,
	STATUS_HW_IDLE  = 10,
	ENUM_DUMMY4WORD(vpe_reg_status_t)
} vpe_reg_status_t;

typedef enum {
	RET_ERROR = -1,
	RET_OK = 0,
	ENUM_DUMMY4WORD(RET_t)
} RET_t;

typedef enum {
	ALIGN_NA = -1,
	ALIGN_None = 0,
	ALIGN_1 = 1,
	ALIGN_2 = 2,
	ALIGN_4 = 4,
	ALIGN_8 = 8,
	ALIGN_16 = 16,
	ALIGN_32 = 32,
	ALIGN_MAX,
	ENUM_DUMMY4WORD(ALIGN_t)
} ALIGN_t;

typedef enum {
	ALIGM_TYPE_DES_W = 0,
	ALIGM_TYPE_DES_H,

	ALIGM_TYPE_OUT_W,
	ALIGM_TYPE_OUT_H,
	ALIGM_TYPE_OUT_X,
	ALIGM_TYPE_OUT_Y,

	ALIGM_TYPE_RLT_W,
	ALIGM_TYPE_RLT_H,
	ALIGM_TYPE_RLT_X,
	ALIGM_TYPE_RLT_Y,

	ALIGM_TYPE_PIP_W,
	ALIGM_TYPE_PIP_H,
	ALIGM_TYPE_PIP_X,
	ALIGM_TYPE_PIP_Y,

	ALIGM_TYPE_SRC_COL_W,
	ALIGM_TYPE_SRC_COL_H,

	ALIGM_TYPE_PROC_X,
	ALIGM_TYPE_PROC_Y,
	ALIGM_TYPE_PROC_W,
	ALIGM_TYPE_PROC_H,

	ALIGM_TYPE_SRC_W,
	ALIGM_TYPE_SRC_H,

	ALIGM_TYPE_SCA_W,
	ALIGM_TYPE_SCA_H,

	ALIGM_TYPE_SCA_CROP_X,
	ALIGM_TYPE_SCA_CROP_Y,
	ALIGM_TYPE_SCA_CROP_W,
	ALIGM_TYPE_SCA_CROP_H,

	ALIGM_TYPE_TC_CROP_X,
	ALIGM_TYPE_TC_CROP_Y,
	ALIGM_TYPE_TC_CROP_W,
	ALIGM_TYPE_TC_CROP_H,

	ALIGM_TYPE_MAX,
	ENUM_DUMMY4WORD(ALIGM_TYPE_t)
} ALIGM_TYPE_t;

typedef enum {
	VPE_PROC_LLADDR,
	VPE_PROC_GATINGEN,
	ENUM_DUMMY4WORD(VPE_PROC_CONFIG_NAME_t)
} VPE_PROC_CONFIG_NAME_t;

typedef struct _VPE_SW_CONFIG_ {
	UINT8     clock_en;
	INT32     proc_gating;
	INT32     proc_efuse;
} VPE_SW_CONFIG;



extern VPE_INT_CB vpe_p_int_hdl_callback;

extern int vpe_drv_get_align(KDRV_VPE_CONFIG *vpe_config, UINT32 type, UINT8 resNum, UINT8 dupNum);


extern int vpe_drv_init_procedure(VPE_INFO *pvpe_info);
extern int vpe_drv_exit_procedure(void);

#endif