#ifndef __KDRV_IPP_BUILTIN_INT_H__
#define __KDRV_IPP_BUILTIN_INT_H__

#include "kwrap/spinlock.h"
#include "kwrap/list.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"

#define KDRV_IPP_BUILTIN_MSG_ENABLE (0)
#if KDRV_IPP_BUILTIN_MSG_ENABLE
#define KDRV_IPP_BUILTIN_DUMP(fmt, args...) DBG_DUMP(fmt, ##args)
#else
#define KDRV_IPP_BUILTIN_DUMP(fmt, args...)
#endif

#define KDRV_IPP_BUILTIN_ENG_BIT(eng) (1 << eng)

#define KDRV_IPP_BUILTIN_BUF_LCA_NUM 2
#define KDRV_IPP_BUILTIN_BUF_3DNR_NUM 2
#define KDRV_IPP_BUILTIN_BUF_3DNR_STA_NUM 1
#define KDRV_IPP_BUILTIN_BUF_DFG_NUM 2
#define KDRV_IPP_BUILTIN_BUF_WDR_NUM 2
#define KDRV_IPP_BUILTIN_BUF_PM_NUM 3
#define KDRV_IPP_BUILTIN_BUF_VA_NUM 2
#define KDRV_IPP_BUILTIN_BUF_DUMMY_UV_NUM 1
#define KDRV_IPP_BUILTIN_BUF_SIE 2

#define KDRV_IPP_BUILTIN_FMD_CB_NUM	(2)

/* CTL_IPP_FLOW_TYPE */
typedef enum {
	KDRV_IPP_BUILTIN_FLOW_UNKNOWN = 0,
	KDRV_IPP_BUILTIN_FLOW_RAW,
	KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW,
	KDRV_IPP_BUILTIN_FLOW_CCIR,
	KDRV_IPP_BUILTIN_FLOW_DIRECT_CCIR,
	KDRV_IPP_BUILTIN_FLOW_IME_D2D,
	KDRV_IPP_BUILTIN_FLOW_IPE_D2D,
	KDRV_IPP_BUILTIN_FLOW_VR360,
	KDRV_IPP_BUILTIN_FLOW_DCE_D2D,
	KDRV_IPP_BUILTIN_FLOW_CAPTURE_RAW,
	KDRV_IPP_BUILTIN_FLOW_CAPTURE_CCIR,
	KDRV_IPP_BUILTIN_FLOW_MAX
} KDRV_IPP_BUILTIN_FLOW_TYPE;

typedef struct {
	UINT32 addr;
	UINT32 size;
	UINT32 phy_addr;
} KDRV_IPP_BUILTIN_BLK;

typedef struct {
	KDRV_IPP_BUILTIN_BLK lca[KDRV_IPP_BUILTIN_BUF_LCA_NUM];
	KDRV_IPP_BUILTIN_BLK _3dnr_mv[KDRV_IPP_BUILTIN_BUF_3DNR_NUM];
	KDRV_IPP_BUILTIN_BLK _3dnr_ms[KDRV_IPP_BUILTIN_BUF_3DNR_NUM];
	KDRV_IPP_BUILTIN_BLK _3dnr_ms_roi[KDRV_IPP_BUILTIN_BUF_3DNR_NUM];
	KDRV_IPP_BUILTIN_BLK _3dnr_sta[KDRV_IPP_BUILTIN_BUF_3DNR_STA_NUM];
	KDRV_IPP_BUILTIN_BLK defog[KDRV_IPP_BUILTIN_BUF_DFG_NUM];
	KDRV_IPP_BUILTIN_BLK pm[KDRV_IPP_BUILTIN_BUF_PM_NUM];
	KDRV_IPP_BUILTIN_BLK wdr[KDRV_IPP_BUILTIN_BUF_WDR_NUM];
	KDRV_IPP_BUILTIN_BLK va[KDRV_IPP_BUILTIN_BUF_VA_NUM];
	KDRV_IPP_BUILTIN_BLK dummy_uv[KDRV_IPP_BUILTIN_BUF_DUMMY_UV_NUM];
} KDRV_IPP_BUILTIN_PRI_BUF;

#define KDRV_IPP_BUILTIN_FRC_BASE (4096)
typedef struct {
	UINT8 skip;
	UINT16 src;
	UINT16 dst;
	UINT32 drop_rate;
	UINT32 rate_cnt;
} KDRV_IPP_BUILTIN_RATE_CTL;

#define KDRV_IPP_BUILTIN_PRI_MASK_NUM (8)
#define KDRV_IPP_BUILTIN_PRI_MASK_REG_BASE03 (0x420)
#define KDRV_IPP_BUILTIN_PRI_MASK_REG_BASE47 (0x4B4)
#define KDRV_IPP_BUILTIN_PRI_MASK_REG_OFS  (0x24)
typedef struct {
	UINT8 enable;
	UINT8 hlw_enable;
	IPOINT coord[4];
	IPOINT coord2[4];
	UINT8 color[3];
	UINT8 weight;
} KDRV_IPP_BUILTIN_PRI_MASK;

typedef union {
	UINT32 val;
	struct {
		UINT32 type : 1;
		UINT32 pxlsize : 2;
		UINT32 : 1;
		UINT32 color_y : 8;
		UINT32 color_u : 8;
		UINT32 color_v : 8;
		UINT32 : 3;
		UINT32 hlw_en : 1;
	} bit;
} KDRV_IPP_BUILTIN_PRI_MASK_REG_CTRL;

typedef union {
	UINT32 val;
	struct {
		UINT32 comp : 2;
		UINT32 : 2;
		UINT32 coefa : 13;
		UINT32 signa : 1;
		UINT32 coefb : 13;
		UINT32 signb : 1;
	} bit_set0;
	struct {
		UINT32 coefc : 26;
		UINT32 signc : 1;
	} bit_set1;
} KDRV_IPP_BUILTIN_PRI_MASK_REG_LINE;

#define KDRV_IPP_BUILTIN_DBG_TS_NUM (10)
typedef enum {
	KDRV_IPP_BUILTIN_DBG_TS_FST = 0,
	KDRV_IPP_BUILTIN_DBG_TS_FMD,
	KDRV_IPP_BUILTIN_DBG_TS_PUSH_IN,
	KDRV_IPP_BUILTIN_DBG_TS_EXIT,
	KDRV_IPP_BUILTIN_DBG_TS_TYPE_MAX
} KDRV_IPP_BUILTIN_DBG_TS_TYPE;

typedef struct {
	KDRV_IPP_BUILTIN_DBG_TS_TYPE type;
	VOS_TICK timestamp;
} KDRV_IPP_BUILTIN_DBG_TS_INFO;

typedef struct {
	vk_spinlock_t lock;
	UINT8 ts_log_cnt;
	KDRV_IPP_BUILTIN_DBG_TS_INFO ts[KDRV_IPP_BUILTIN_DBG_TS_NUM];
} KDRV_IPP_BUILTIN_DBG_INFO;

typedef struct {
	const CHAR *name;
	CHAR node[64];
	UINT32 src_sie_id_bit;
	UINT32 flow;
	UINT32 isp_id;
	KDRV_IPP_BUILTIN_BLK vprc_blk[KDRV_IPP_BUILTIN_PATH_ID_MAX][2];
	KDRV_IPP_BUILTIN_BLK vprc_shdr_blk;
	KDRV_IPP_BUILTIN_BLK vprc_ctrl;
	KDRV_IPP_BUILTIN_PRI_BUF pri_buf;
	KDRV_IPP_BUILTIN_FUNC func_en;
	KDRV_IPP_BUILTIN_CTL_INFO last_ctl_info;
	KDRV_IPP_BUILTIN_ISP_INFO isp_info;
	UINT32 _3dnr_ref_path;
	UINT32 fs_cnt;
	VOS_TICK fs_timestamp;
	UINT32 fmd_cnt;
	UINT32 push_in_cnt;

	KDRV_IPP_BUILTIN_IMG_INFO path_info[KDRV_IPP_BUILTIN_PATH_ID_MAX];
	void *p_cur_job;
	UINT8 in_buf_release_cnt;
	UINT8 out_buf_release_cnt;
	UINT8 get_phy_out_cnt;
	UINT8 get_isp_info_cnt;

	KDRV_IPP_BUILTIN_RATE_CTL frc[KDRV_IPP_BUILTIN_PATH_ID_MAX];
	KDRV_IPP_BUILTIN_PRI_MASK mask[KDRV_IPP_BUILTIN_PRI_MASK_NUM];

	/* debug info */
	KDRV_IPP_BUILTIN_DBG_INFO dbg;
} KDRV_IPP_BUILTIN_HDL;

typedef struct {
	/* todo: check sie push job struct */
	KDRV_IPP_BUILTIN_HDL *p_owner;
	UINT32 blk[2];		/* blk_addr, used for release buffer */
	UINT32 va[2];		/* va[0]: raw/y, va[1]: raw/uv */
	UINT32 buf_ctrl;	/* refer to SIE_BUILTIN_HEADER_CTL_LOCK/SIE_BUILTIN_HEADER_CTL_PUSH */
	UINT32 timestamp;
	UINT32 count;
	struct vos_list_head pool_list;
	struct vos_list_head proc_list;
} KDRV_IPP_BUILTIN_JOB;

typedef struct {
	UINT32 start_addr;
	UINT32 total_size;
	UINT32 blk_size;
	UINT8 blk_num;
	vk_spinlock_t lock;
	struct vos_list_head free_list_root;
	struct vos_list_head used_list_root;
} KDRV_IPP_BUILTIN_MEM_POOL;

typedef struct {
	void *reg_base[KDRV_IPP_BUILTIN_ENG_MAX];
	KDRV_IPP_BUILTIN_FMD_CB fmd_cb[KDRV_IPP_BUILTIN_FMD_CB_NUM];
	UINT32 dtsi_ver;	/* 0: old version for one direct mode, 1: new version for multi handle */
	UINT32 hdl_num;
	KDRV_IPP_BUILTIN_HDL *p_hdl;
	KDRV_IPP_BUILTIN_HDL *p_trig_hdl;	/* hdl that been trig start */
	UINT32 *p_dtsi_buf;					/* buffer to read dtsi array */
	UINT8 is_init;

	KDRV_IPP_BUILTIN_MEM_POOL job_pool;
	THREAD_HANDLE proc_tsk_id;
	ID proc_tsk_flg_id;
	vk_spinlock_t job_list_lock;
	struct vos_list_head job_list_root;

	UINT32 valid_src_id_bit;
} KDRV_IPP_BUILTIN_CTL;

INT32 kdrv_ipp_builtin_plat_init_clk(void);
void* kdrv_ipp_builtin_plat_ioremap_nocache(UINT32 addr, UINT32 size);
void kdrv_ipp_builtin_plat_iounmap(void *addr);
INT32 kdrv_ipp_builtin_plat_read_dtsi_array(CHAR *node, CHAR *tag, UINT32 *buf, UINT32 num);
INT32 kdrv_ipp_builtin_plat_read_dtsi_string(CHAR *node, CHAR *tag, const CHAR **str);
void *kdrv_ipp_builtin_plat_malloc(UINT32 want_size);
void kdrv_ipp_builtin_plat_free(void *p_buf);

/* debug */
void kdrv_ipp_builtin_debug_set_ts(UINT32 type, KDRV_IPP_BUILTIN_DBG_INFO *p_dbg);

#if 0
#endif

#define KDRV_IPP_BUILTIN_TSK_PRIORITY (3)
#define KDRV_IPP_BUILTIN_QUE_DEPTH (8)

#define KDRV_IPP_BUILTIN_TSK_PAUSE			FLGPTN_BIT(0)
#define KDRV_IPP_BUILTIN_TSK_PAUSE_END		FLGPTN_BIT(1)
#define KDRV_IPP_BUILTIN_TSK_RESUME			FLGPTN_BIT(2)
#define KDRV_IPP_BUILTIN_TSK_RESUME_END		FLGPTN_BIT(3)
#define KDRV_IPP_BUILTIN_TSK_TRIGGER		FLGPTN_BIT(4)
#define KDRV_IPP_BUILTIN_TSK_EXIT			FLGPTN_BIT(5)
#define KDRV_IPP_BUILTIN_TSK_EXIT_END		FLGPTN_BIT(6)
#define KDRV_IPP_BUILTIN_TSK_CHK			FLGPTN_BIT(7)
#define KDRV_IPP_BUILTIN_TSK_JOBDONE		FLGPTN_BIT(8)
#define KDRV_IPP_BUILTIN_TSK_FASTBOOT_DONE	FLGPTN_BIT(9)

#endif
