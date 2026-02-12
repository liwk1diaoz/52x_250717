#include <stdio.h>
#include <stdlib.h>
#include "GxStrg.h"
#include "GxStrgInt.h"

#define _GXSTRG_UCTRL_TODO_ 1
#if _GXSTRG_UCTRL_TODO_
/**
    Token Unit

    each token get their argument count and argument vector
*/
typedef struct _UTOKEN_PARAM {
	char *name; ///< token name
	INT32(*fp)(INT32 argc, char *argv[]);  ///< token's argc and argv callback
} UTOKEN_PARAM, *PUTOKEN_PARAM;

/**
    Token End Info

    each token get their argument count and argument vector
*/
typedef struct _UTOKEN_END_INFO {
	char *failed_name; ///< if *fp return non-zero, it will be name of parameter, otherwise is NULL.
	INT32 err_code;    ///< if *fp return non-zero, it will equal to the value, otherwise is zero.
} UTOKEN_END_INFO, *PUTOKEN_END_INFO;

/**
    Moudle Unit

    each moudle set their token unit table and event callback
*/
typedef struct _UTOKEN_MODULE {
	char *name;
	UTOKEN_PARAM *params;
	INT32(*fp_begin)(char *str, INT32 len);
	INT32(*fp_end)(UTOKEN_END_INFO *p_info);
	INT32(*fp_unknown)(char *name);
	void *Reversed;
} UTOKEN_MODULE, *PUTOKEN_MODULE;
#endif

#define UGXSTRG_STR_READY   "ready"
#define UGXSTRG_STR_UNFMT   "unfmt"
#define UGXSTRG_STR_NODEV   "nodev"

#define GXSTRG_SXCMD_TEST  0

extern GXSTRG_LINUX_STATUS g_LnxStrgStatus[GXSTRG_STRG_NUM];

static INT32 GxStrg_UCtrl_status(INT32 argc, char *argv[])
{
	char *pMntStatus = NULL;
	char *pMntName = NULL;
	char *pDrvName = NULL;
	UINT32  DevId;
	UINT32  StrgCbVal;
	BOOL    bIsReadOnly = FALSE;
	BOOL    bIsFormatted;

	if (argc < 3) {
		DBG_ERR("argc(%d) < 3\r\n", argc);
		return -1;
	}

	//default necessary arguments
	pMntStatus = argv[0];
	pMntName = argv[1];
	pDrvName = argv[2];

	//optional arguments
	if (argc > 3) {
		bIsReadOnly = (BOOL)atoi(argv[3]);
	}

	DBG_IND("MntStatus %s, MntName %s, DrvName %s, IsReadOnly %d\r\n", pMntStatus, pMntName, pDrvName, bIsReadOnly);

	if (0 == strcmp(pMntStatus, UGXSTRG_STR_READY)) {
		StrgCbVal = STRG_CB_INSERTED;
		bIsFormatted = TRUE;
	} else if (0 == strcmp(pMntStatus, UGXSTRG_STR_NODEV)) {
		StrgCbVal = STRG_CB_REMOVED;
		bIsFormatted = FALSE;
	} else if (0 == strcmp(pMntStatus, UGXSTRG_STR_UNFMT)) {
		StrgCbVal = STRG_CB_INSERTED;
		bIsFormatted = FALSE;
	} else {
		DBG_ERR("Unknown MntStatus %s\r\n", pMntStatus);
		return -1;
	}
	DBG_IND("StrgCbVal %d\r\n", StrgCbVal);

	if (pDrvName[0] < GXSTRG_DRVNAME_FIRST || pDrvName[0] > GXSTRG_DRVNAME_LAST) {
		DBG_ERR("Invalid drv name %c\r\n", pDrvName[0]);
		return -1;
	}
	DevId = pDrvName[0] - GXSTRG_DRVNAME_FIRST;
	DBG_IND("DevId %d\r\n", DevId);

	snprintf(g_FSInitParam[DevId].FSParam.szMountPath, sizeof(g_FSInitParam[DevId].FSParam.szMountPath), "/mnt/%s", pMntName);
	g_LnxStrgStatus[DevId].IsInserted = (StrgCbVal == STRG_CB_INSERTED);
	g_LnxStrgStatus[DevId].IsReadOnly = bIsReadOnly;
	g_LnxStrgStatus[DevId].IsFormatted = bIsFormatted;

	DBG_DUMP("MntPath %s, IsInserted %d, IsReadOnly %d, bIsFormatted %d\r\n",
			 g_FSInitParam[DevId].FSParam.szMountPath,
			 g_LnxStrgStatus[DevId].IsInserted,
			 g_LnxStrgStatus[DevId].IsReadOnly,
			 g_LnxStrgStatus[DevId].IsFormatted);

	if (g_fpStrgCB) {
		(*g_fpStrgCB)(StrgCbVal, DevId, 0);
	}

	return 0;
}

static INT32 GxStrg_UCtrl_begin(char *str, INT32 len)
{
	DBG_IND("%s\r\n", str);
	return 0;
}

static INT32 GxStrg_UCtrl_end(UTOKEN_END_INFO *p_info)
{
#if !_GXSTRG_UCTRL_TODO_
	char retStr[64] = {0};

	if (p_info->err_code < 0) {
		snprintf(retStr, sizeof(retStr), "ERR: parm %s", p_info->failed_name);

		if (NVTUCTRL_RET_OK == NvtUctrl_SetRetString(retStr)) {
			DBG_ERR("%s\r\n", retStr);
		}

		return p_info->err_code;
	}
#endif
	return 0;
}

static INT32 GxStrg_UCtrl_unknown(char *name)
{
	DBG_ERR("unknown argument: %s\r\n", name);
	return 0;
}

UTOKEN_PARAM tbl_gxstrg_param[] = {
	{"status", GxStrg_UCtrl_status},
	{NULL, NULL}, //last tag, it must be
};

UTOKEN_MODULE module_gxstrg = {
	"gxstrg",
	tbl_gxstrg_param,
	GxStrg_UCtrl_begin,
	GxStrg_UCtrl_end,
	GxStrg_UCtrl_unknown,
	NULL
};

#if GXSTRG_SXCMD_TEST
//to test GxStrg with UCtrl:
//1. set GXSTRG_SXCMD_TEST to 1
//2. extern GxStrg_InstallCmd() and call this function
//3. type SxCmd to test. e.g. (gxstrg uctrl -status mounted sd A 0)
void GxStrg_InstallCmd(void);

static BOOL Cmd_GxStrg_uctrl(CHAR *strCmd)
{
	uToken_Parse(strCmd, &module_gxstrg);
	return TRUE;
}

SXCMD_BEGIN(gxstrg, "GxStrg")
SXCMD_ITEM("uctrl %", Cmd_GxStrg_uctrl, "")
SXCMD_END()

void GxStrg_InstallCmd(void)
{
	SxCmd_AddTable(gxstrg);
}
#endif

