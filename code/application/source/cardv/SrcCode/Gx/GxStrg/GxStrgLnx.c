#include <stdio.h>
#include <stdlib.h>
#include <mntent.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include "GxStrg.h"
#include "GxStrgInt.h"
#include "DxStorage.h"
#include "DxApi.h"

#define GXSTRG_STRCPY(dst, src, dst_size)  do { strncpy(dst, src, dst_size-1); dst[dst_size-1] = '\0'; } while(0)

extern GXSTRG_LINUX_STATUS g_LnxStrgStatus[GXSTRG_STRG_NUM];

static int GxStrgLnx_DetStrgCard(DX_HANDLE StrgDXH)
{
	UINT32 uiDxState = 0;
	if (Dx_GetState(StrgDXH, STORAGE_STATE_INSERT, &uiDxState) == DX_OK && uiDxState == TRUE) {
		return 1;
	}
	return 0;
}

static int GxStrgLnx_IsPathExist(char *path)
{
    int fd;
    fd = open(path, O_RDONLY);
    if(fd < 0) {
        return 0;
    }
	close(fd);
    return 1;
}

static int GxStrgLnx_FindEntryByPrefix(const char *root_pathp, const char *prefix_namep, char *out_namep, int out_size)
{
	DIR *pDIR;
	struct dirent *entry = NULL;
	int prefix_len;
	int bfound = 0;

	*out_namep = '\0';//set empty first
	prefix_len = strlen(prefix_namep);

	pDIR = opendir(root_pathp);
	if (NULL == pDIR) {
		return -1;
	}

	while (!bfound) {
		entry = readdir(pDIR);
		if (NULL == entry) {//reach the end
			break;
		}
		if (0 == strncmp(entry->d_name, prefix_namep, prefix_len)) {
			GXSTRG_STRCPY(out_namep, entry->d_name, out_size);
			bfound = 1;
		}
	}

	closedir(pDIR);

	if (bfound) {
		DBG_IND("Found %s in %s\r\n", out_namep, root_pathp);
		return 0;
	} else {
		DBG_IND("Prefix:%s in %s not found\r\n", prefix_namep, root_pathp);
		return -1;
	}
}

static int GxStrgLnx_GetSD1DevByBus(char *out_path, int out_size)
{
	#define MMC_SYS_PATH "/sys/bus/mmc/devices"
	#define MMC0BUS "mmc0"
	#define MMCDEV "mmc"

	char find_path[64] = {0};//e.g. /sys/devices/platform/nt96660_mmc.0/mmc_host/mmc0/mmc0:b368/block
	char dev_path[32] = {0};//e.g. /dev/mmcblk0p1, /dev/mmcblk0, /dev/mmcblk1p1, /dev/mmcblk1
	char mmc_bus_name[16] = {0}; //e.g. mmc0:b368
	char mmc_dev_name[16] = {0};
	int bFound = 0;

	out_path[0] = '\0';//set empty first

	//1. find the bus mmc0 to check the card is inserted or not
	if(0 != GxStrgLnx_FindEntryByPrefix(MMC_SYS_PATH, MMC0BUS, mmc_bus_name, sizeof(mmc_bus_name))) {
		DBG_IND("mmc0 bus not found\r\n");
		return -1;
	}

	//2. get the device name from mmc0 information
	snprintf(find_path, sizeof(find_path), "%s/%s/block", MMC_SYS_PATH, mmc_bus_name);
	if (!GxStrgLnx_IsPathExist(find_path)) {
		DBG_ERR("%s not found\r\n", find_path);
		return -1;
	}

	if(0 != GxStrgLnx_FindEntryByPrefix(find_path, MMCDEV, mmc_dev_name, sizeof(mmc_dev_name))) {
		DBG_IND("device not found\r\n");
		return -1;
	}

	//3. try the real device name is mmcblk0/mmcblk1 or mmcblk0p1/mmcblk1p1
	//find dev name with p1 (e.g. mmcblk0p1)
	snprintf(dev_path, sizeof(dev_path), "/dev/%sp1", mmc_dev_name);
	if (GxStrgLnx_IsPathExist(dev_path)) {
		bFound = 1;
	}

	if(!bFound) {//find dev name without p1. (e.g. mmcblk0)
		snprintf(dev_path, sizeof(dev_path), "/dev/%s", mmc_dev_name);
		if(GxStrgLnx_IsPathExist(dev_path)) {
			bFound = 1;
		}
	}

	if(bFound) {
		DBG_IND("SD1Dev = %s\r\n", out_path);
		GXSTRG_STRCPY(out_path, dev_path, out_size);
		return 0;
	} else {
		DBG_ERR("The dev partition name not found\r\n");
		return -1;
	}
}

static int GxStrgLnx_GetDevByMountPath(char *pDevPath, const char* pMountPath, int BufSize)
{
	struct mntent *ent;
	FILE *Linux_pFILE;
	int PathLen;

	PathLen = strlen(pMountPath);
	if('/' == pMountPath[PathLen-1]) {
		PathLen--;//exclude the last '/' for strncmp
	}

	Linux_pFILE = setmntent("/proc/mounts", "r");
	if (NULL == Linux_pFILE) {
		DBG_ERR("setmntent error\n");
		return -1;
	}

	while (NULL != (ent = getmntent(Linux_pFILE))) {
		DBG_IND("%s => %s\n", ent->mnt_fsname, ent->mnt_dir);
		if(0 == strncmp(pMountPath, ent->mnt_dir, PathLen)) {
			strncpy(pDevPath, ent->mnt_fsname, BufSize-1);
			pDevPath[BufSize-1] = '\0';
			endmntent(Linux_pFILE);
			return 0;
		}
	}

	*pDevPath = 0;//clean the result
	endmntent(Linux_pFILE);
	return -1;
}

static INT32 GxStrgLnx_ChkStatus(UINT32 DevId)
{
	CHAR    DevPath[132] = {0}; //e.g. /dev/mmcblk0p1
	CHAR   *pMountPath = (CHAR *)&g_FSInitParam[DevId].FSParam.szMountPath; //e.g. /mnt/sd
	INT32   ret;
	UINT32  StrgCbVal;
	BOOL    bIsReadOnly = FALSE;
	BOOL    bIsFormatted;
	static  UINT32 StrgCbValPrev = STRG_CB_UNKNOWN;

	//1. get the device name from the mount list
	ret = GxStrgLnx_GetDevByMountPath(DevPath, pMountPath, sizeof(DevPath));
	if (ret == 0) {
		//0. if the storage object is set, detect card insert
		if (g_pCurStrgDXH[DevId]) {
			if (GxStrgLnx_DetStrgCard(g_pCurStrgDXH[DevId])) {
				DBG_IND("Found %s is mounted on %s\r\n", DevPath, pMountPath);
				StrgCbVal = STRG_CB_INSERTED;
				bIsFormatted = TRUE;
				goto label_exit;
			} else {
				DBG_IND("Found %s is removed (mounted on %s)\r\n", DevPath, pMountPath);
				StrgCbVal = STRG_CB_REMOVED;
				bIsFormatted = FALSE;
				goto label_exit;
			}
		}
		DBG_IND("Found %s is mounted on %s\r\n", DevPath, pMountPath);
		StrgCbVal = STRG_CB_INSERTED;
		bIsFormatted = TRUE;
		goto label_exit;
	}

	//2. if the device is not mounted, get the dev name from the mmc0 bus
	ret = GxStrgLnx_GetSD1DevByBus(DevPath, sizeof(DevPath));
	if (ret == 0) {
		//0. if the storage object is set, detect card insert
		if (g_pCurStrgDXH[DevId]) {
			if (GxStrgLnx_DetStrgCard(g_pCurStrgDXH[DevId])) {
				DBG_IND("Found %s is unmounted but inserted\r\n", DevPath);
				StrgCbVal = STRG_CB_INSERTED;
				bIsFormatted = FALSE;
				goto label_exit;
			} else {
				DBG_IND("Found %s is unmounted and removed\r\n", DevPath);
				StrgCbVal = STRG_CB_REMOVED;
				bIsFormatted = FALSE;
				goto label_exit;
			}
		}
		DBG_IND("Found %s is unmounted\r\n", DevPath);
		StrgCbVal = STRG_CB_INSERTED;
		bIsFormatted = FALSE;
		goto label_exit;
	}
	else {
		//0. if the storage object is set, detect card insert
		if (g_pCurStrgDXH[DevId]) {
			if (GxStrgLnx_DetStrgCard(g_pCurStrgDXH[DevId])) {
				DBG_IND("Found %s is inserted\r\n", DevPath, pMountPath);
				StrgCbVal = STRG_CB_INSERTED;
				bIsFormatted = FALSE;
				goto label_exit;
			} else {
				DBG_IND("Found %s is removed\r\n", DevPath);
				StrgCbVal = STRG_CB_REMOVED;
				bIsFormatted = FALSE;
				goto label_exit;
			}
		}
		DBG_IND("Found %s is removed\r\n", DevPath);
		StrgCbVal = STRG_CB_REMOVED;
		bIsFormatted = FALSE;
		goto label_exit;
	}

label_exit:
	if (StrgCbValPrev != StrgCbVal) {
		g_LnxStrgStatus[DevId].IsInserted = (StrgCbVal == STRG_CB_INSERTED);
		g_LnxStrgStatus[DevId].IsReadOnly = bIsReadOnly;
		g_LnxStrgStatus[DevId].IsFormatted = bIsFormatted;
		DBG_DUMP("MntPath %s, IsInserted %d, IsReadOnly %d, bIsFormatted %d\r\n",
			 g_FSInitParam[DevId].FSParam.szMountPath,
			 g_LnxStrgStatus[DevId].IsInserted,
			 g_LnxStrgStatus[DevId].IsReadOnly,
			 g_LnxStrgStatus[DevId].IsFormatted);
		StrgCbValPrev = StrgCbVal;
	}
	return 0;
}

INT32 GxStrgLnx_Det(UINT32 DevId)
{
	if (0 != GxStrgLnx_ChkStatus(DevId)) {
		return DET_CARD_UNKNOWN;
	}
	if (g_LnxStrgStatus[DevId].IsInserted) {
		return DET_CARD_INSERTED;
	} else {
		return DET_CARD_REMOVED;
	}
}