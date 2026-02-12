/**
 * @file kdrv_ipp_builtin.h
 * @brief type definition of KDRV API.
 */

#ifndef __KDRV_IPP_BUILTIN_H__
#define __KDRV_IPP_BUILTIN_H__

#include "kwrap/type.h"
#include "kwrap/perf.h"
#include "kdrv_videoprocess/kdrv_ipp_utility.h"
#include "kdrv_videoprocess/kdrv_ime.h"

#define IPP_DRIVER_NEW_REG_HANDLE    ENABLE


typedef enum {
	KDRV_IPP_PARAM_BUILTIN_GET_DBGINFO     = 0,	/* Get debug information*/
	KDRV_IPP_PARAM_BUILTIN_GET_DTSI_CHKSUM = 1,	/* Get dtsi checksum to check dtsi is match hdal or not*/
	KDRV_IPP_PARAM_BUILTIN_GET_IPP_INFO    = 2,	/* For IQ builtin to get ipp info*/
	KDRV_IPP_PARAM_BUILTIN_GET_ID_MAX,
} KDRV_IPP_BUILTIN_GET_PARAM_ID;

typedef enum {
	KDRV_IPP_BUILTIN_IFE,
	KDRV_IPP_BUILTIN_DCE,
	KDRV_IPP_BUILTIN_IPE,
	KDRV_IPP_BUILTIN_IME,
	KDRV_IPP_BUILTIN_IFE2,
	KDRV_IPP_BUILTIN_ENG_MAX,
} KDRV_IPP_BUILTIN_ENG;


/* REFER TO CTL_IPP_FUNC */
typedef enum {
	KDRV_IPP_BUILTIN_FUNC_NONE              =   0x00000000,
	KDRV_IPP_BUILTIN_FUNC_WDR               =   0x00000001,
	KDRV_IPP_BUILTIN_FUNC_SHDR              =   0x00000002,
	KDRV_IPP_BUILTIN_FUNC_DEFOG             =   0x00000004,
	KDRV_IPP_BUILTIN_FUNC_3DNR              =   0x00000008,
	KDRV_IPP_BUILTIN_FUNC_DATASTAMP         =   0x00000010,
	KDRV_IPP_BUILTIN_FUNC_PRIMASK           =   0x00000020,
	KDRV_IPP_BUILTIN_FUNC_PM_PIXELIZTION    =   0x00000040,
	KDRV_IPP_BUILTIN_FUNC_YUV_SUBOUT        =   0x00000080,
	KDRV_IPP_BUILTIN_FUNC_VA_SUBOUT         =   0x00000100,
	KDRV_IPP_BUILTIN_FUNC_3DNR_STA          =   0x00000200,
	KDRV_IPP_BUILTIN_FUNC_GDC               =   0x00000400,
} KDRV_IPP_BUILTIN_FUNC;

/* INFORMATION TO BUILINT ISP */
typedef struct{
    /*input*/
    UINT32 isp_id;
    /*output*/
    KDRV_IPP_BUILTIN_FUNC func_en;
}KDRV_IPP_BUILTIN_IPP_INFO;

/* callback for push buffer */
#define KDRV_IPP_BUITLIN_MAX_PLANE (4)

typedef enum {
	KDRV_IPP_BUILTIN_PATH_ID_1 = 0,
	KDRV_IPP_BUILTIN_PATH_ID_2 = 1,
	KDRV_IPP_BUILTIN_PATH_ID_3 = 2,
	KDRV_IPP_BUILTIN_PATH_ID_4 = 3,
	KDRV_IPP_BUILTIN_PATH_ID_5 = 4,
	KDRV_IPP_BUILTIN_PATH_ID_6 = 5,
	KDRV_IPP_BUILTIN_PATH_ID_MAX = 6,
} KDRV_IPP_BUILTIN_PATH_ID;

typedef enum {
	KDRV_IPP_BUILTIN_FMT_Y8            = 0x51080400, ///< 1 plane, pixel=Y(w,h)
	KDRV_IPP_BUILTIN_FMT_YUV420        = 0x520c0420, ///< 2 plane, pixel=Y(w,h), UV(w/2,h/2)
	KDRV_IPP_BUILTIN_FMT_YUV420_PLANAR = 0x530c0420, ///< 3 plane, pixel=Y(w,h), U(w/2,h/2), and V(w/2,h/2)
	KDRV_IPP_BUILTIN_FMT_NVX2          = 0x610c2420, ///< novatek-yuv-compress-2 of YUV420 (using by NA51023)
} KDRV_IPP_BUILTIN_FMT;

typedef enum {
	KDRV_IPP_BUILTIN_ISP_EVENT_RESET = 0,
	KDRV_IPP_BUILTIN_ISP_EVENT_TRIG,
	KDRV_IPP_BUILTIN_ISP_EVENT_FRM_ED,
	KDRV_IPP_BUILTIN_ISP_EVENT_MAX,
} KDRV_IPP_BUILTIN_ISP_EVENT;

typedef struct{
	UINT32 *reg_buffer[KDRV_IPP_BUILTIN_ENG_MAX];
	KDRV_IME_PM_INFO pm_info[KDRV_IME_PM_SET_IDX_MAX];
	UINT32 dtsi_chksum;
}KDRV_IPP_BUILTIN_FDT_GET_INFO;

typedef struct {
	BOOL enable;
	ISIZE size;
	KDRV_IPP_BUILTIN_FMT fmt;
	UINT32 loff[KDRV_IPP_BUITLIN_MAX_PLANE];
	UINT32 addr[KDRV_IPP_BUITLIN_MAX_PLANE];
	UINT32 phyaddr[KDRV_IPP_BUITLIN_MAX_PLANE];
	VOS_TICK timestamp;
} KDRV_IPP_BUILTIN_IMG_INFO;

typedef struct {
	const CHAR *name;
	KDRV_IPP_BUILTIN_IMG_INFO out_img[KDRV_IPP_BUILTIN_PATH_ID_MAX];
	UINT32 release_flg;
} KDRV_IPP_BUILTIN_FMD_CB_INFO;

typedef struct {
	/* these are physical address */
	UINT32 wdr_addr;
	UINT32 defog_addr;
	UINT32 lca_addr;
	UINT32 _3dnr_mv_addr;
	UINT32 _3dnr_ms_addr;
	UINT32 path_addr_y[KDRV_IPP_BUILTIN_PATH_ID_MAX];
	UINT32 path_addr_u[KDRV_IPP_BUILTIN_PATH_ID_MAX];
	UINT32 path_addr_v[KDRV_IPP_BUILTIN_PATH_ID_MAX];

	/* info for lca */
	UINT32 gray_avg_u;
	UINT32 gray_avg_v;
} KDRV_IPP_BUILTIN_CTL_INFO;

#define KDRV_IPP_BUILTIN_ISP_HIST_NUM		(128)
typedef struct {
	UINT8 enable;
	UINT8 sel;
    UINT16 stcs[KDRV_IPP_BUILTIN_ISP_HIST_NUM];
} KDRV_IPP_BUILTIN_ISP_HIST_RST;

typedef struct {
	UINT32 localmax_max;
	UINT32 coneng_max;
	UINT32 coneng_avg;
} KDRV_IPP_BUILTIN_ISP_EDGE_STCS;

#define KDRV_IPP_BUILTIN_ISP_DEFOG_AIRLIGHT_NUM (3)
typedef struct {
	UINT16 airlight[KDRV_IPP_BUILTIN_ISP_DEFOG_AIRLIGHT_NUM];
} KDRV_IPP_BUILTIN_ISP_DEFOG_STCS;

typedef struct {
	KDRV_IPP_BUILTIN_ISP_HIST_RST hist_rst;
	KDRV_IPP_BUILTIN_ISP_EDGE_STCS edge_stcs;
	KDRV_IPP_BUILTIN_ISP_DEFOG_STCS defog_stcs;
	UINT32 defog_subout_addr;
} KDRV_IPP_BUILTIN_ISP_INFO;

typedef void (*KDRV_IPP_BUILTIN_FMD_CB)(KDRV_IPP_BUILTIN_FMD_CB_INFO *p_info, UINT32 reserved);
typedef void (*KDRV_IPP_BUILTIN_ISP_CB)(UINT32 id, UINT32 status);
INT32 kdrv_ipp_builtin_reg_fmd_cb(KDRV_IPP_BUILTIN_FMD_CB fp);
INT32 kdrv_ipp_builtin_reg_isp_cb(KDRV_IPP_BUILTIN_ISP_CB fp);

KDRV_IPP_BUILTIN_IMG_INFO kdrv_ipp_builtin_get_path_info(const CHAR *name, KDRV_IPP_BUILTIN_PATH_ID pid);
KDRV_IPP_BUILTIN_CTL_INFO *kdrv_ipp_builtin_get_ctl_info(CHAR *name);
BOOL kdrv_ipp_builtin_get_eng_enable(KDRV_IPP_BUILTIN_ENG eng);
KDRV_IPP_BUILTIN_ISP_INFO* kdrv_ipp_builtin_get_isp_info(CHAR *name);


/**
    init fastboot flow
*/
#define KDRV_IPP_MAX_HANDLE_NUM (4)
typedef struct {
    CHAR *name;
    UINT32 src_sie_id_bit;

    /* ipp internal buffer info */
    UINT32 ctrl_blk_addr;
    UINT32 ctrl_blk_size;

    /* shdr ring buffer info */
    UINT32 shdr_ring_buf_addr;
    UINT32 shdr_ring_buf_size;
} KDRV_IPP_BUILTIN_INIT_HDL;

typedef struct {
    UINT8 hdl_num;
	UINT32 valid_src_id_bit;
    KDRV_IPP_BUILTIN_INIT_HDL hdl_info[KDRV_IPP_MAX_HANDLE_NUM];
} KDRV_IPP_BUILTIN_INIT_INFO;

INT32 kdrv_ipp_builtin_init(KDRV_IPP_BUILTIN_INIT_INFO *info);
INT32 kdrv_ipp_builtin_exit(void);
void kdrv_ipp_builtin_update_timestamp(void);


/**
    record register config
    should be removed when all ipp driver updated
*/
typedef struct {
	UINT32 ofs;
	UINT32 val;
} IPP_REG_INFO;

typedef struct {
	UINT32 cnt[KDRV_IPP_BUILTIN_ENG_MAX];
	IPP_REG_INFO *p_reg[KDRV_IPP_BUILTIN_ENG_MAX];
} KDRV_IPP_REG_LOG_RESULT;

void kdrv_ipp_set_reg(UINT32 eng, UINT32 base, UINT32 ofs, UINT32 val);


/**
	dump register config for new driver
*/
typedef struct {
	void (*get_hdl_list)(UINT32 *hdl_list, UINT32 list_size);
	void (*get_reg_dtsi)(UINT32 hdl, UINT32 cnt, KDRV_IPP_BUILTIN_FDT_GET_INFO* info);
} KDRV_IPP_BUILTIN_DTSI_CB;

int kdrv_ipp_builtin_reg_dump(int fd, char *pre_fix);
void kdrv_ipp_builtin_get_reg_dtsi(UINT32 **dtsi_buffer);
UINT32 kdrv_ipp_builtin_get_reg_num(KDRV_IPP_BUILTIN_ENG eng);
void kdrv_ipp_builtin_reg_dtsi_cb(KDRV_IPP_BUILTIN_DTSI_CB cb);
void kdrv_ipp_builtin_get_hdal_hdl_list(UINT32 *hdl_list, UINT32 list_size);
void kdrv_ipp_builtin_get_hdal_reg_dtsi(UINT32 hdl, UINT32 cnt, KDRV_IPP_BUILTIN_FDT_GET_INFO* info);

/**
	dump frame rate ctrl dtsi
*/
int kdrv_ipp_builtin_frc_dump(char *frc_node, int fd, char *pre_fix);

/**
	debug api
*/
typedef enum {
	KDRV_IPP_BUILTIN_DEBUG_DUMP = 0,
	KDRV_IPP_BUILTIN_DEBUG_ITEM_MAX,
} KDRV_IPP_BUILTIN_DEBUG_ITEM;

void kdrv_ipp_builtin_debug(UINT32 item, void *data);
/**
    ipp builtin universal set/get interface
*/
INT32 kdrv_ipp_builtin_get(void *p_hdl, UINT32 param_id, void *p_data);
#endif

