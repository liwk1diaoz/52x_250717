#ifndef _SDE_PROC_H_
#define _SDE_PROC_H_

//=============================================================================
// struct & enum definition
//=============================================================================
#define PROC_MSG_BUFSIZE        256
#define MAX_CMDLINE_LENGTH      256
#define MAX_CMD_ARGUMENTS       64
#define MAX_CMD_LENGTH          32
#define MAX_CMD_TEXT_LENGTH     256

typedef enum {
	SDE_PROC_R_ITEM_NONE,
	SDE_PROC_R_ITEM_BUFFER,
	SDE_PROC_R_ITEM_PARAM,
	SDE_PROC_R_ITEM_CFG_DATA,
	ENUM_DUMMY4WORD(SDE_PROC_R_ITEM)
} SDE_PROC_R_ITEM;

typedef struct _SDE_PROC_CMD {
	CHAR cmd[MAX_CMD_LENGTH];

	INT32 (*execute)(SDE_MODULE *pdrv, INT32 argc, CHAR **argv);
	CHAR text[MAX_CMD_TEXT_LENGTH];
} SDE_PROC_CMD;

typedef struct _SDE_PROC_MSG_BUF {
	INT8 *buf;
	UINT32 size;
	UINT32 count;
} SDE_PROC_MSG_BUF;

//=============================================================================
// external functions
//=============================================================================
extern INT32 sde_proc_init(SDE_DRV_INFO *pdrv_info);
extern void sde_proc_remove(SDE_DRV_INFO *pdrv_info);

#endif

