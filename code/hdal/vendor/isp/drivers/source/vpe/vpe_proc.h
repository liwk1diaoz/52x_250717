#ifndef _VPE_PROC_H_
#define _VPE_PROC_H_

//=============================================================================
// struct & enum definition
//=============================================================================
#define PROC_MSG_BUFSIZE        256
#define MAX_CMDLINE_LENGTH      256
#define MAX_CMD_ARGUMENTS       64
#define MAX_CMD_LENGTH          32
#define MAX_CMD_TEXT_LENGTH     256

typedef enum {
	VPE_PROC_R_ITEM_NONE,
	VPE_PROC_R_ITEM_BUFFER,
	VPE_PROC_R_ITEM_PARAM,
	VPE_PROC_R_ITEM_CFG_DATA,
	VPE_PROC_R_ITEM_SHARPEN,
	VPE_PROC_R_ITEM_DCE_CTL,
	VPE_PROC_R_ITEM_GDC,
	VPE_PROC_R_ITEM_2DLUT,
	VPE_PROC_R_ITEM_DRT,
	VPE_PROC_R_ITEM_DCTG,
	VPE_PROC_R_ITEM_FLIP_ROT,
	ENUM_DUMMY4WORD(VPE_PROC_R_ITEM)
} VPE_PROC_R_ITEM;

typedef struct _VPE_PROC_CMD {
	CHAR cmd[MAX_CMD_LENGTH];

	INT32 (*execute)(VPE_MODULE *pdrv, INT32 argc, CHAR **argv);
	CHAR text[MAX_CMD_TEXT_LENGTH];
} VPE_PROC_CMD;

typedef struct _VPE_PROC_MSG_BUF {
	INT8 *buf;
	UINT32 size;
	UINT32 count;
} VPE_PROC_MSG_BUF;

//=============================================================================
// external functions
//=============================================================================
extern INT32 vpe_proc_init(VPE_DRV_INFO *pdrv_info);
extern void vpe_proc_remove(VPE_DRV_INFO *pdrv_info);

#endif

