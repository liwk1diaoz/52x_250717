/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       ImageApp_Photo_FileNaming.c
    @ingroup    mIImageApp

    @note       Nothing.

    @date       2017/06/14
*/
#include <string.h>
//#include "SysKer.h"
#include "FileDB.h"
//#include "ipl_cmd.h"
//#include "imgcommon.h"
#include "NamingRule/NameRule_Custom.h"
#include "ImageApp_Photo_FileNaming.h"
#include "ImageApp_Photo_int.h"
#include "DCF.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          IA_Photo_FileNaming
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

// Naming rule & FileDB
typedef struct {
	BOOL                 bIsSortBySn;
	CHAR*               SortSN_Delim;                         ///<  The delimiter string, e.g. underline "_", "AA"
	INT32                SortSN_DelimCount;                    ///<  The delimiter count to find the serial number
	INT32                SortSN_CharNumOfSN;                   ///<  The character number of the serial number
} _FDB_SN;


#define _PHOTO_FDB_ROOT_MAX   NMC_ROOT_MAX_LEN
static CHAR                g_isf_photo_fdb_root_folder[_PHOTO_FDB_ROOT_MAX] = {0};
static BOOL                g_isf_photo_is_fdb_root_folder                   = FALSE;
static MEM_RANGE           g_isf_photo_fdb_pool                             = {0};

static CHAR                g_photo_write_file_Path[NMC_TOTALFILEPATH_MAX_LEN] = {0};

static _FDB_SN g_fdb_sn={
	FALSE,
	"_",
	1,
	6
};
static FILEDB_HANDLE       g_isf_photo_fdb_hdl                              = FILEDB_CREATE_ERROR;
static INT32               g_photo_id_mapping[PHOTO_CAP_ID_MAX]             = {-1,-1};

static PHOTO_FILENAME_CB   *g_fpPhotoFileNameCB = NULL;
static CHAR  g_isf_photo_file_name[NMC_TOTALFILEPATH_MAX_LEN]   = {0};

INT32 ImageApp_Photo_FileNaming_MakePath(UINT32 filetype, CHAR *pPath, UINT32 uiPathId);
INT32 ImageApp_Photo_Phy2LogID(UINT32 id);


INT32 ImageApp_Photo_FileNaming_Open(void)
{

	FILEDB_INIT_OBJ    FDBInitObj = {0};

	if (g_isf_photo_fdb_pool.size == 0) {
		DBG_ERR("ImageApp_Movie_FDB_Pool() is not set!!\r\n");
		return -1;
	}
	FDBInitObj.rootPath                     = g_isf_photo_fdb_root_folder;
	FDBInitObj.defaultfolder                = NULL;
	FDBInitObj.filterfolder                 = NULL;
	FDBInitObj.bIsRecursive                 = TRUE;
	FDBInitObj.bIsCyclic                    = TRUE;
	FDBInitObj.bIsSupportLongName           = TRUE;
	FDBInitObj.u32MaxFilePathLen            = 80;
	FDBInitObj.u32MaxFileNum                = g_photo_filedb_max_num;
	FDBInitObj.fileFilter                   = g_photo_filedb_filter;
	FDBInitObj.u32MemAddr                   = g_isf_photo_fdb_pool.addr;
	FDBInitObj.u32MemSize                   = g_isf_photo_fdb_pool.size;

	if (g_fdb_sn.bIsSortBySn){
		FDBInitObj.SortSN_Delim=g_fdb_sn.SortSN_Delim;
		FDBInitObj.SortSN_DelimCount=g_fdb_sn.SortSN_DelimCount;
		FDBInitObj.SortSN_CharNumOfSN=g_fdb_sn.SortSN_CharNumOfSN;
	}

	g_isf_photo_fdb_hdl = FileDB_Create(&FDBInitObj);
	if (g_fdb_sn.bIsSortBySn){
		FileDB_SortBy(g_isf_photo_fdb_hdl, FILEDB_SORT_BY_SN, FALSE);
	}else{
		FileDB_SortBy(g_isf_photo_fdb_hdl, FILEDB_SORT_BY_MODDATE, FALSE);
	}
	NH_Custom_SetFileHandleIDByPath(g_isf_photo_fdb_hdl, 0);

	return 0;
}

INT32 ImageApp_Photo_FileNaming_Close(void)
{
	UINT32 i;
	if (g_isf_photo_fdb_pool.size && g_isf_photo_fdb_hdl !=FILEDB_CREATE_ERROR) {

		FileDB_Release(g_isf_photo_fdb_hdl);

		g_isf_photo_fdb_hdl            = FILEDB_CREATE_ERROR;
		g_isf_photo_is_fdb_root_folder = FALSE;
		for (i=0;i<PHOTO_CAP_ID_MAX;i++){
			g_photo_id_mapping[i] = -1;
		}
	}
	return 0;
}

INT32 ImageApp_Photo_FileNaming_MakePath(UINT32 filetype, CHAR *pPath, UINT32 uiPathId)
{
	//if (filetype == IMGCAP_FSYS_RAW)
	if (filetype == HD_CODEC_TYPE_RAW)
		NH_CustomUti_MakePhotoPath(uiPathId, NAMERULECUS_FILETYPE_RAW, pPath);
	else
		NH_CustomUti_MakePhotoPath(uiPathId, NAMERULECUS_FILETYPE_JPG, pPath);
	return 0;
}

/*
    Set the record root name

    Set the folder name.

    @param[in] config_id.
    @param[in] value .

    @return INT32
*/
INT32 ImageApp_Photo_FileNaming_SetRootPath(CHAR *prootpath)
{
	if (!prootpath) {
		return -1;
	}
	memset(g_isf_photo_fdb_root_folder, 0, _PHOTO_FDB_ROOT_MAX);
	memcpy(g_isf_photo_fdb_root_folder, prootpath, _PHOTO_FDB_ROOT_MAX);
	g_isf_photo_is_fdb_root_folder = TRUE;

	//NH_Custom_SetRootPathByPath(0, g_isf_photo_fdb_root_folder);
	return 0;
}

/**
    Set buffer for FileDB

    Set buffer for FileDB

    @param[in] PHOTO_CFG_MEM_RANGE

    @return void
*/
INT32 ImageApp_Photo_FileNaming_SetFileDBPool(MEM_RANGE *pPool)
{
	if (!pPool) {
		return -1;
	}
	memcpy(&g_isf_photo_fdb_pool, pPool, sizeof(MEM_RANGE));
	return 0;
}

INT32 ImageApp_Photo_FileNaming_SetFolder(PHOTO_CAP_FOLDER_NAMING *pfolder_naming)
{
	NMC_FOLDER_INFO          nmc_folder_info = {0};

	if (!g_isf_photo_is_fdb_root_folder) {
		DBG_ERR("ImageApp_Photo_Root_Path is not set!!!!\r\n");
		return -1;
	}
	if (pfolder_naming->cap_id >= PHOTO_CAP_ID_MAX) {
		DBG_ERR("cap_id %d >= max %d\r\n",pfolder_naming->cap_id,PHOTO_CAP_ID_MAX);
		return -1;
	}
	NH_Custom_SetRootPathByPath(pfolder_naming->cap_id, g_isf_photo_fdb_root_folder);
	g_photo_id_mapping[pfolder_naming->cap_id] = pfolder_naming->ipl_id;
	nmc_folder_info.pathid  = pfolder_naming->cap_id;
	nmc_folder_info.pMovie  = NULL;
	nmc_folder_info.pEMR    = NULL;
	nmc_folder_info.pRO     = NULL;
	nmc_folder_info.pPhoto  = pfolder_naming->folder_path;
	nmc_folder_info.pEvent1 = NULL;
	nmc_folder_info.pEvent2 = NULL;
	nmc_folder_info.pEvent3 = NULL;
	NH_Custom_SetFolderPath(&nmc_folder_info);
	return 0;
}
void ImageApp_Photo_FileNaming_SetFileNameCB(PHOTO_FILENAME_CB *pfFileNameCb)
{

	if (pfFileNameCb) {
		g_fpPhotoFileNameCB = (PHOTO_FILENAME_CB *) pfFileNameCb;
	}

}

INT32 ImageApp_Photo_Phy2LogID(UINT32 id)
{

	UINT32 i;
	for (i=0;i<PHOTO_CAP_ID_MAX;i++){
		if (g_photo_id_mapping[i] == -1)
			continue;
		if (g_photo_id_mapping[i] == (INT32)id)
			return i;
	}
	DBG_ERR("id =%d\r\n",id);

	return -1;
}

#if 1
//Note: dp not export this function
static PFILEDB_FILE_ATTR _ImageApp_Photo_FindFile(UINT32 *file_index, CHAR *dir_name)
{
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	UINT32 total_num, idx;
	CHAR tmp_dir[NMC_OTHERS_MAX_LEN] = {0};
	FILEDB_HANDLE fdb_hdl = g_isf_photo_fdb_hdl;

	total_num = FileDB_GetTotalFileNum(fdb_hdl);
	if (0 == total_num) {
		DBG_WRN("FileDB is empty\r\n");
		return NULL;
	}

	for (idx = 0; idx < total_num; idx++) {

		p_file_attr = FileDB_SearhFile(fdb_hdl, idx);
		if (p_file_attr == NULL) {
			DBG_ERR("get file failed\r\n");
			return NULL;
		}

		if (FST_STA_OK != FileSys_GetParentDir(p_file_attr->filePath, tmp_dir)) {
			DBG_ERR("get dir error. file %s\r\n", p_file_attr->filePath);
		}

		if (strcmp(tmp_dir, dir_name)) {
			continue;
		}

		*file_index = idx;
		return p_file_attr;
	}

	return NULL;
}
//Note: dp not export this function
static INT32 _ImageApp_Photo_ReuseFile(CHAR *p_new_path)
{
	FILEDB_HANDLE fdb_hdl;
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	CHAR tmp_path[NMC_TOTALFILEPATH_MAX_LEN] = {0};
	CHAR tmp_dir[NMC_TOTALFILEPATH_MAX_LEN] = {0};
	CHAR tmp_name[NMC_TOTALFILEPATH_MAX_LEN] = {0};
	CHAR *pos;
	UINT32 path_len;
	UINT32 file_index = 0;
	UINT32 dir_len;
	UINT32 len;
	UINT8 attrib = 0;

	fdb_hdl = g_isf_photo_fdb_hdl;
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		return -1;
	}

	//get dir name
	if (FST_STA_OK != FileSys_GetParentDir(p_new_path, tmp_dir)) {
		DBG_ERR("get dir error. file %s\r\n", p_new_path);
		return -1;
	}
	dir_len = strlen(tmp_dir);
	path_len = strlen(p_new_path);

	//get file name
	pos = p_new_path + dir_len;
	strncpy(tmp_name, pos, path_len-dir_len);
	tmp_name[path_len-dir_len] = '\0';

	//find file from filedb
	p_file_attr = _ImageApp_Photo_FindFile(&file_index, tmp_dir);
	if (p_file_attr == NULL) {
		DBG_ERR("find file failed\r\n");
		return -1;
	}
	len = strlen(p_file_attr->filePath);
	strncpy(tmp_path, p_file_attr->filePath, len);
	tmp_path[len] = '\0';

	//delete from filedb
	if (TRUE != FileDB_DeleteFile(fdb_hdl, file_index)) {
		DBG_ERR("FileDB_DeleteFile failed\r\n");
		return -1;
	}
	DBG_IND("^G %s deleted from FileDB\r\n", tmp_path);

	FileSys_GetAttrib(tmp_path, &attrib);
	if (attrib & FST_ATTRIB_READONLY) {
		FileSys_SetAttrib(tmp_path, attrib, FALSE);
	}

	//reset file name
	if (FST_STA_OK != FileSys_RenameFile(tmp_name, tmp_path, FALSE)) {
		DBG_ERR("Reuse to %s failed\r\n", p_new_path);
		return -1;
	}

	//reset attrib
	if (FST_STA_OK != FileSys_SetAttrib(p_new_path, FST_ATTRIB_HIDDEN, FALSE)) {
		DBG_ERR("Set attrib to %s failed\r\n", p_new_path);
		return -1;
	}
	if (attrib & FST_ATTRIB_READONLY) {
		FileSys_SetAttrib(p_new_path, attrib, TRUE);
	}

	return 0;
}
#endif


INT32 ImageApp_Photo_WriteCB(UINT32 Addr, UINT32 Size, UINT32 Fmt, UINT32 uiPathId)
{

	#define SEQ_ID_MAX  10
	FST_FILE fp;
	INT32    rt;
	UINT32   Length;
	//UINT64   allocSize;
	CHAR     FilePath[NMC_TOTALFILEPATH_MAX_LEN];
	UINT32   fileType;
	UINT32   open_flag;
	INT32    logical_id;
	if (g_isf_photo_fdb_pool.size && g_isf_photo_fdb_hdl !=FILEDB_CREATE_ERROR) {


		logical_id = ImageApp_Photo_Phy2LogID(uiPathId);
		if (logical_id < 0){
			DBG_ERR("logical_id error=%d\r\n",logical_id);
			rt =FST_STA_ERROR;
			return rt;
		}

		if(g_fpPhotoFileNameCB){
			g_fpPhotoFileNameCB(logical_id, g_isf_photo_file_name);

			if (Fmt == HD_CODEC_TYPE_RAW){
				fileType=NAMERULE_FMT_RAW;
			}else{
				fileType=NAMERULE_FMT_JPG;
			}
			NameRule_getCustom_byid(logical_id)->SetInfo(MEDIANAMING_SETINFO_SETFILETYPE, fileType, 0, 0);
			NH_Custom_BuildFullPath_GiveFileName(logical_id, NMC_SECONDTYPE_PHOTO, g_isf_photo_file_name, FilePath);
		}else{
			ImageApp_Photo_FileNaming_MakePath(Fmt, FilePath, logical_id);
		}
	}else{//DCF
		UINT32 nextFolderID = 0, nextFileID = 0;
		if (Fmt == HD_CODEC_TYPE_RAW){
			fileType = DCF_FILE_TYPE_RAW;
		}else{
			fileType = DCF_FILE_TYPE_JPG;
		}

		DCF_GetNextID(&nextFolderID,&nextFileID);
		DCF_MakeObjPath(nextFolderID, nextFileID, fileType, FilePath);
	}

	strncpy(g_photo_write_file_Path, FilePath, sizeof(g_photo_write_file_Path) - 1);
	DBG_IND("Photo_WriteCB logical_id=%d, File=%s, Addr=0x%x, Size=0x%x, Fmt=%d\r\n",logical_id,FilePath, Addr, Size, Fmt);
#if 1
		if (ImageApp_Photo_IsFormatFree()) {
			if (0 != _ImageApp_Photo_ReuseFile(FilePath)) {
				DBG_ERR("^GReuse File %s failed\r\n", FilePath);
				return -1;
			}
			DBG_IND("^G[PHOTO Naming] Reuse %s\r\n", FilePath);
			open_flag = FST_OPEN_EXISTING | FST_OPEN_WRITE;
		} else {
			open_flag = FST_CREATE_ALWAYS | FST_OPEN_WRITE;
		}
#else
		open_flag = FST_CREATE_ALWAYS | FST_OPEN_WRITE;
#endif
		if ((fp = FileSys_OpenFile(FilePath, open_flag)) != NULL) {
			//UINT32 fileSizeAlgin = _CamFile_GetFileSizeAlign();
			Length = Size;

#if 0
			if (fileSizeAlgin) {
				//DBGH(Length);
				allocSize = (Length + fileSizeAlgin - 1 / fileSizeAlgin) / fileSizeAlgin * fileSizeAlgin;
				//DBGH(allocSize);
				rt = FileSys_AllocFile(fp, (UINT64)allocSize, 0, 0);
				if (rt != FST_STA_OK) {
					DBG_ERR("AllocFile size %d fail\r\n", allocSize);
					FileSys_CloseFile(fp);
					return FST_STA_ERROR;
				}
			}
#endif
		rt = FileSys_WriteFile(fp, (UINT8 *)Addr, &Length, 0, NULL);
		FileSys_CloseFile(fp);
		if (rt == FST_STA_OK) {
			//DBG_IND("photo capfile %s\r\n", FilePath);
			// keep last write file path
			if (g_isf_photo_fdb_pool.size && g_isf_photo_fdb_hdl !=FILEDB_CREATE_ERROR) {
				//if (FALSE == FileDB_AddFile(g_isf_photo_fdb_hdl, FilePath)) {
				if (FALSE == FileDB_AddFileToLast(g_isf_photo_fdb_hdl, FilePath)) {
					DBG_ERR("FileDB_AddFile %s failed\r\n", FilePath);
					//return FST_STA_ERROR;
					rt =FST_STA_ERROR;
				}else{
					DBG_DUMP("%s added to FileDB\r\n", FilePath);
				}
			}else{
				DCF_AddDBfile(FilePath);
				DBG_DUMP("%s added to DCF\r\n", FilePath);
			}
		} else {
			DBG_ERR("Addr=0x%x,Size=0x%x,Fmt=%d\r\n", Addr, Size, Fmt);
		}
		//return rt;
	}else{
		DBG_ERR("FileSys_OpenFile fail\r\n");
		rt =FST_STA_ERROR;
	}
	//if(g_PhotoCapMsgCb){
	//	g_PhotoCapMsgCb(IMG_CAP_CBMSG_FSTOK, &rt);
	//}
	//return FST_STA_ERROR;
	return rt;
}

CHAR* ImageApp_Photo_GetLastWriteFilePath(void)
{
    return g_photo_write_file_Path;
    //return NULL;
}
void ImageApp_Photo_FileNaming_SetSortBySN(CHAR *pDelimStr, UINT32 nDelimCount, UINT32 nNumOfSn)
{
    if (pDelimStr)
    {
        g_fdb_sn.SortSN_Delim       = pDelimStr;
        g_fdb_sn.SortSN_DelimCount  = nDelimCount;
        g_fdb_sn.SortSN_CharNumOfSN = nNumOfSn;

        g_fdb_sn.bIsSortBySn= TRUE;
    }
    else
        DBG_ERR("No set Delim String!!\r\n");
}


/////////////////////////////////////////////////////////////////////////////



