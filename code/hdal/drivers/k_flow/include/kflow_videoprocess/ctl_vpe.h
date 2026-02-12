/**
    VPE_Ctrl Layer

    @file       ctl_vpe.h
    @ingroup    mIVPE_Ctrl
    @note

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _CTL_VPE_H_
#define _CTL_VPE_H_

#include "kwrap/error_no.h"
#include "kflow_common/type_vdo.h"

/* vpe callback fp type */
typedef INT32 (*CTL_VPE_EVENT_FP)(UINT32 event, void *p_in, void *p_out);

/* vpe error type */
#define CTL_VPE_E_OK				(E_OK)
#define CTL_VPE_E_SYS				(E_SYS)
#define CTL_VPE_E_PAR				(E_PAR)
#define CTL_VPE_E_NOMEM				(E_NOMEM)
#define CTL_VPE_E_QOVR				(E_QOVR)
#define CTL_VPE_E_ID				(E_ID)
#define CTL_VPE_E_INDATA			(-101)
#define CTL_VPE_E_FLUSH				(-102)
#define CTL_VPE_E_DROP_INPUT_ONLY	(-103)
#define CTL_VPE_E_ISP_PAR			(-104)
#define CTL_VPE_E_STATE				(-105)
#define CTL_VPE_E_KDRV_OPEN			(-200)	///< kdrv open error
#define CTL_VPE_E_KDRV_CLOSE		(-201)	///< kdrv close error
#define CTL_VPE_E_KDRV_SET			(-202)	///< kdrv set error
#define CTL_VPE_E_KDRV_GET			(-203)	///< kdrv get error
#define CTL_VPE_E_KDRV_TRIG			(-204)	///< kdrv trigger error
#define CTL_VPE_E_KDRV_STRP			(-205)	///< kdrv stripe cal error
#define CTL_VPE_E_KDRV_DROP			(-206)	///< kdrv stripe cal error

/* init/query config */
typedef struct {
	UINT32 handle_num;			/* number of handles */
	UINT32 queue_num;			/* number of queue nodes */
} CTL_VPE_CTX_BUF_CFG;

/* vpe callback event type */
typedef enum {
	CTL_VPE_CBEVT_IN_BUF = 0,
	CTL_VPE_CBEVT_OUT_BUF,
	CTL_VPE_CBEVT_MAX,
} CTL_VPE_CBEVT_TYPE;

typedef struct {
	CTL_VPE_CBEVT_TYPE cbevt;
	CTL_VPE_EVENT_FP fp;
} CTL_VPE_REG_CB_INFO;

/* buffer io operation for both in/out buf callback */
typedef enum {
	CTL_VPE_BUF_NEW = 0,
	CTL_VPE_BUF_PUSH,
	CTL_VPE_BUF_LOCK,
	CTL_VPE_BUF_UNLOCK,
	CTL_VPE_BUF_IO_MAX,
} CTL_VPE_BUF_IO;


/* snd event information, also for in bufio callback */
/*
	data_addr = pointer of input VDO_FRAME
	VDO_FRAME.reserved[0] = MAKEFOURCC('H', 'A', 'L', 'N'), (opt.)
			 .reserved[1] = input buffer height alignment, (opt.)
			 .reserved[2] = (CTL_VPE_ISP_2DLUT_SZ)2d_lut_size[0...3], (opt.), user input 2dlut size and address
			 .reserved[3] = (UINT32)2d_lut virtual address, (opt.)
			 .reserved[5] = source in crop start y[0...15]|x[16...31], (opt.)
			 .reserved[6] = source in crop start h[0...15]|w[16...31], (opt.)
*/
typedef struct {
	UINT32	buf_id;
	UINT32	data_addr;
	UINT32	rev;
	INT32   err_msg;
} CTL_VPE_EVT;

/* in crop config */
typedef enum {
	CTL_VPE_IN_CROP_AUTO = 0,	/* reference input header information */
	CTL_VPE_IN_CROP_NONE,		/* no crop, process full image */
	CTL_VPE_IN_CROP_USER,		/* user define crop window */
	CTL_VPE_IN_CROP_MODE_MAX,
} CTL_VPE_IN_CROP_MODE;

typedef struct {
	CTL_VPE_IN_CROP_MODE mode;
	URECT crp_window;			/* only effect when CTL_VPE_IN_CROP_USER */
} CTL_VPE_IN_CROP;

typedef enum {
	CTL_VPE_DCS_DEFAULT = 0, //general case
	CTL_VPE_DCS_MF,          //mirror/flip
	CTL_VPE_DCS_WIDE,        //wide angle
	CTL_VPE_DCS_WALL,        //wall mount fish-eye
	CTL_VPE_DCS_EIS,         //eis
	CTL_VPE_DCS_DIS,         //dis
	CTL_VPE_DCS_ROT,	      //rot
	CTL_VPE_DCS_CEIL_FLOOR,  //ceil-floor fish-eye
	CTL_VPE_DCS_MAX
} CTL_VPE_DCS_MODE;//dc scenes

/* out path config */
typedef enum {
	CTL_VPE_OUT_PATH_ID_1 = 0,
	CTL_VPE_OUT_PATH_ID_2,
	CTL_VPE_OUT_PATH_ID_3,
	CTL_VPE_OUT_PATH_ID_4,
	CTL_VPE_OUT_PATH_ID_MAX,
} CTL_VPE_OUT_PATH_ID;

typedef struct {
	CTL_VPE_OUT_PATH_ID pid;
	BOOL enable;
	VDO_PXLFMT fmt;
	USIZE bg_size;		/* background size(buffer size) */
	USIZE scl_size;		/* scale out size, output to dst_pos */
	URECT pre_scl_crop;	/* crop before scale */
	URECT post_scl_crop;/* crop after scale */
	URECT out_window;	/* output window base on background */
	UPOINT dst_pos;		/* destination position base on out window */
	URECT hole_region;	/* hole region */
} CTL_VPE_OUT_PATH;

/* buffer info for out bufio_callback used */
typedef struct {
	CTL_VPE_OUT_PATH_ID pid;
	UINT32 buf_size;
	UINT32 buf_id;
	UINT32 buf_addr;
	VDO_FRAME vdo_frm;
	UINT32 lock_cnt;
	INT32 err_msg;
} CTL_VPE_OUT_BUF_INFO;

/* private buffer query info */
typedef enum {
	CTL_VPE_2DLUT_SZ_ZERO = 0,
	CTL_VPE_2DLUT_SZ_9X9,
	CTL_VPE_2DLUT_SZ_65X65,
	CTL_VPE_2DLUT_SZ_129X129,
	CTL_VPE_2DLUT_SZ_257X257,
} CTL_VPE_2DLUT_SZ_SEL;

typedef struct {
	UINT32 buf_addr;					/* only use in set api */
	UINT32 buf_size;					/* set: check buffer enough; get: return buffer size */
} CTL_VPE_PRIVATE_BUF;

/* vpe flush config for CTL_VPE_ITEM_FLUSH
	NULL -> flush all path
	else, flush pid in config
*/
typedef struct {
	CTL_VPE_OUT_PATH_ID pid;
} CTL_VPE_FLUSH_CONFIG;

/**
	output path yuv buffer height alignment
	will use vdoframe.reserve[0], [1]
	VDO_FRAME.reserved[0] = MAKEFOURCC('H', 'A', 'L', 'N'); // height align
	VDO_FRAME.reserved[1] = y_height after align
*/
typedef struct {
	CTL_VPE_OUT_PATH_ID pid;
	UINT32 align;
} CTL_VPE_OUT_PATH_HALIGN;

/* vpe get/set item */
typedef enum {
	CTL_VPE_ITEM_REG_CB_IMM = 0,    /* Callback function register.      SET-Only,   data_type: CTL_VPE_REG_CB_INFO          */
	CTL_VPE_ITEM_ALGID_IMM,         /* vpe ALG ISP id                   GET/SET,    data_type: UINT32                       */
	CTL_VPE_ITEM_FLUSH,             /* vpe flush all buffer(in&out)     SET-Only,   data_type: CTL_VPE_FLUSH_CONFIG or NULL */
	CTL_VPE_ITEM_APPLY,             /* Apply parameters to next trigger SET-Only,   data_type: UINT32                       */
	CTL_VPE_ITEM_IN_CROP,           /* vpe input crop config            GET/SET,    data_type: CTL_VPE_IN_CROP              */
	CTL_VPE_ITEM_OUT_PATH,          /* vpe output path config           GET/SET,    data_type: CTL_VPE_OUT_PATH             */
	CTL_VPE_ITEM_PUSHEVT_INQ,       /* vpe get push event number in que GET-Only,   data_type: UINT32                       */
	CTL_VPE_ITEM_PRIVATE_BUF,       /* vpe get/set private buffer       GET-Only,   data_type: CTL_VPE_PRIVATE_BUF          */
	CTL_VPE_ITEM_OUT_PATH_HALIGN,   /* vpe output path config           GET/SET,    data_type: CTL_VPE_OUT_PATH_HALIGN      */
	CTL_VPE_ITEM_DCS,   			/* vpe dc scenes           			GET/SET,    data_type: CTL_VPE_DCS_MODE    			*/
	CTL_VPE_ITEM_MAX
} CTL_VPE_ITEM;

/* vpe ioctl item */
typedef enum {
	CTL_VPE_IOCTL_SNDEVT = 0,       /* Send event.          data_type: CTL_VPE_EVT      */
	CTL_VPE_IOCTL_MAX
} CTL_VPE_IOCTL;

UINT32 ctl_vpe_query(CTL_VPE_CTX_BUF_CFG ctx_buf_cfg);
INT32 ctl_vpe_init(CTL_VPE_CTX_BUF_CFG ctx_buf_cfg, UINT32 buf_addr, UINT32 buf_size);
INT32 ctl_vpe_uninit(void);
UINT32 ctl_vpe_open(CHAR *name);
INT32 ctl_vpe_close(UINT32 hdl);
INT32 ctl_vpe_set(UINT32 hdl, UINT32 item, void *data);
INT32 ctl_vpe_get(UINT32 hdl, UINT32 item, void *data);
INT32 ctl_vpe_ioctl(UINT32 hdl, UINT32 item, void *data);

void ctl_vpe_dump_all(int (*dump)(const char *fmt, ...));

#endif
