#include "ImageApp_MovieMulti_int.h"

#include "FileSysTsk.h"
#include "FileDB.h"

#define IAMOVIE_FM_PATH_SIZE  32
#define negative(x)     (~(x) + 1)
#define FM_INVALID_IDX       0xFFFF

typedef struct {
	FILEDB_HANDLE fdb_hdl;
	CHAR root_path[IAMOVIE_FM_PATH_SIZE];
	MEM_RANGE pool;
	INT32 is_loop_del;
	INT32 is_add_to_last;
	INT32 is_sync_time;
	UINT64 rofile_size;  // ro file total size
	UINT64 file_size;    // non-ro file total size
	UINT64 remain_size;
} _FDB_INFO;

typedef struct {
	UINT64 resv_size_on;
	UINT64 resv_size_off;
	CHAR drive;
} _FILEOUT_SIZE_INFO;

typedef struct {
	INT32 size_pct;
	INT32 file_num;
	INT32 type_chk;
	INT32 chk_last;
} _FILEOUT_RO_INFO;

typedef struct {
	BOOL                 bIsSortBySn;
	CHAR*               SortSN_Delim;                         ///<  The delimiter string, e.g. underline "_", "AA"
	INT32                SortSN_DelimCount;                    ///<  The delimiter count to find the serial number
	INT32                SortSN_CharNumOfSN;                   ///<  The character number of the serial number
} _FDB_SN;

typedef struct {
	CHAR    dir_path[IAMOVIE_FM_PATH_SIZE];
	UINT32  dir_ratio;
	UINT64  dir_size;
	UINT64  file_size;
	UINT32  is_use;
	UINT32  file_num;
	UINT32  file_is_use;
	UINT32  is_cyclic_rec;
	UINT32  file_unit;
	UINT32  is_merged;    //for scan
	UINT32  is_parent;    //for scan
	INT32   parent_idx;   //for scan
} _DIR_INFO;

typedef struct {
	_DIR_INFO dir_info[IAMOVIEMULTI_FM_DIR_NUM];
	BOOL    is_format_free;
	BOOL    is_folder_loop;
	BOOL    is_reuse_hidden_first;
	BOOL    is_skip_read_only;
	BOOL    is_remove_content;
	UINT64  usage_size;
} _DISK_INFO;

typedef struct {
	UINT32 root_depth;
	UINT32 dir_count;
	UINT32 cur_depth;
	INT32  cur_idx;
	CHAR * cur_dir;
} _SCAN_INFO;

typedef struct {
	UINT32          fileType;
	char            filterStr[4];
} _EXTSTR_TYPE;

static _EXTSTR_TYPE extstr_table[FILEDB_SUPPORT_FILE_TYPE_NUM] = {
	{FILEDB_FMT_JPG, "JPG"},
	{FILEDB_FMT_MOV, "MOV"},
	{FILEDB_FMT_MP4, "MP4"},
	{FILEDB_FMT_MPG, "MPG"},
	{FILEDB_FMT_AVI, "AVI"},
	{FILEDB_FMT_RAW, "RAW"},
	{FILEDB_FMT_MP3, "MP3"},
	{FILEDB_FMT_ASF, "ASF"},
	{FILEDB_FMT_WAV, "WAV"},
	{FILEDB_FMT_BMP, "BMP"},
	{FILEDB_FMT_TS, "TS"},
	{FILEDB_FMT_AAC, "AAC"}
};

static UINT32     g_extstr_info[IAMOVIEMULTI_FM_FDB_CNT] = {FILEDB_FMT_MP4, FILEDB_FMT_MP4};
static _DISK_INFO g_disk_info[IAMOVIEMULTI_FM_FDB_CNT] = {0};
static _DISK_INFO g_disk_info2[IAMOVIEMULTI_FM_FDB_CNT] = {0};
static _SCAN_INFO g_scan_info[IAMOVIEMULTI_FM_FDB_CNT] = {0};
static CHAR       g_cur_dir_path[IAMOVIE_FM_PATH_SIZE] = {0};
static UINT32     g_ro_pct_del_cnt = 5; //suggested

static _FDB_INFO g_fdb_info[IAMOVIEMULTI_FM_FDB_CNT] = {
	{FILEDB_CREATE_ERROR, {0}, {0}, 1, 0, 0}, //Drive 'A'
	{FILEDB_CREATE_ERROR, {0}, {0}, 1, 0, 0}, //Drive 'B'
};

static _FILEOUT_SIZE_INFO g_size_info[IAMOVIEMULTI_FM_FDB_CNT] = {
	{IAMOVIEMULTI_FM_RESV_SIZE, IAMOVIEMULTI_FM_RESV_SIZE, 'A'}, //Drive 'A'
	{IAMOVIEMULTI_FM_RESV_SIZE, IAMOVIEMULTI_FM_RESV_SIZE, 'B'}, //Drive 'B'
};

static _FILEOUT_RO_INFO g_ro_file_info[IAMOVIEMULTI_FM_FDB_CNT] = {
	{IAMOVIEMULTI_FM_RO_PCT, IAMOVIEMULTI_FM_RO_NUM, IAMOVIEMULTI_FM_CHK_PCT, FALSE},
	{IAMOVIEMULTI_FM_RO_PCT, IAMOVIEMULTI_FM_RO_NUM, IAMOVIEMULTI_FM_CHK_PCT, FALSE},
};

static _FDB_SN g_fdb_sn={
	FALSE,
	"_",
	1,
	6
};
static UINT64 g_time_val[IAMOVIEMULTI_FM_FDB_CNT] = {30, 30}; //30s
static INT32 g_file_del_max[IAMOVIEMULTI_FM_FDB_CNT] = {12, 12}; //12
static UINT32 g_TmpCreDT[FST_FILE_DATETIME_MAX_ID] = {0};
static UINT32 g_TmpModDT[FST_FILE_DATETIME_MAX_ID] = {0};

//Note: do not export this function
static INT32 _iamovie_fm_drive_is_invalid(CHAR drive)
{
	if (drive < IAMOVIEMULTI_FM_DRV_FIRST || drive > IAMOVIEMULTI_FM_DRV_LAST) {
		return -1; //invalid
	}
	return 0; //valid
}

//Note: do not export this function
static _FILEOUT_RO_INFO* _iamovie_fm_get_roinfo(CHAR drive)
{
	_FILEOUT_RO_INFO *p_ro_info = NULL;

	if (drive < IAMOVIEMULTI_FM_DRV_FIRST || drive > IAMOVIEMULTI_FM_DRV_LAST) {
		DBG_ERR("Invalid drive 0x%X\r\n", (UINT32)drive);
		return NULL;
	}

	p_ro_info = &g_ro_file_info[drive - 'A'];

	return p_ro_info;
}

//Note: do not export this function
static _FILEOUT_SIZE_INFO* _iamovie_fm_get_sizeinfo(CHAR drive)
{
	_FILEOUT_SIZE_INFO *p_size_info = NULL;

	if (drive < IAMOVIEMULTI_FM_DRV_FIRST || drive > IAMOVIEMULTI_FM_DRV_LAST) {
		DBG_ERR("Invalid drive 0x%X\r\n", (UINT32)drive);
		return NULL;
	}

	p_size_info = &g_size_info[drive - 'A'];

	return p_size_info;
}

//Note: do not export this function
static _FDB_INFO* _iamovie_fm_get_fdbinfo(CHAR drive)
{
	_FDB_INFO *p_fdb_info = NULL;

	if (drive < IAMOVIEMULTI_FM_DRV_FIRST || drive > IAMOVIEMULTI_FM_DRV_LAST) {
		DBG_ERR("Invalid drive 0x%X\r\n", (UINT32)drive);
		return NULL;
	}

	p_fdb_info = &g_fdb_info[drive - 'A'];

	return p_fdb_info;
}

//Note: do not export this function
static FILEDB_HANDLE _iamovie_fm_get_fdb_hdl(CHAR drive)
{
	_FDB_INFO *p_fdb_info;

	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return FILEDB_CREATE_ERROR;
	}

	if (FILEDB_CREATE_ERROR == p_fdb_info->fdb_hdl) {
		DBG_ERR("fdb_hdl is not created, drive %c\r\n", drive);
		return FILEDB_CREATE_ERROR;
	}

	return p_fdb_info->fdb_hdl;
}

//Note: do not export this function
static UINT64* _iamovie_fm_get_timeinfo(CHAR drive)
{
	UINT64 *p_time_info = NULL;

	if (drive < IAMOVIEMULTI_FM_DRV_FIRST || drive > IAMOVIEMULTI_FM_DRV_LAST) {
		DBG_ERR("Invalid drive 0x%X\r\n", (UINT32)drive);
		return NULL;
	}

	p_time_info = &g_time_val[drive - 'A'];

	return p_time_info;
}

//Note: do not export this function
static INT32* _iamovie_fm_get_file_del_max(CHAR drive)
{
	INT32 *p_time_info = NULL;

	if (drive < IAMOVIEMULTI_FM_DRV_FIRST || drive > IAMOVIEMULTI_FM_DRV_LAST) {
		DBG_ERR("Invalid drive 0x%X\r\n", (UINT32)drive);
		return NULL;
	}

	p_time_info = &g_file_del_max[drive - 'A'];

	return p_time_info;
}

//Note: do not export this function
static UINT32* _iamovie_fm_get_extstrinfo(CHAR drive)
{
	UINT32 *p_extstr_info = NULL;

	if (_iamovie_fm_drive_is_invalid(drive)) {
		DBG_ERR("Invalid drive 0x%X\r\n", (UINT32)drive);
		return NULL;
	}

	p_extstr_info = &g_extstr_info[drive - 'A'];

	return p_extstr_info;
}

//Note: do not export this function
static _DISK_INFO* _iamovie_fm_get_diskinfo(CHAR drive)
{
	_DISK_INFO *p_disk_info = NULL;

	if (drive < IAMOVIEMULTI_FM_DRV_FIRST || drive > IAMOVIEMULTI_FM_DRV_LAST) {
		DBG_ERR("Invalid drive 0x%X\r\n", (UINT32)drive);
		return NULL;
	}

	p_disk_info = &g_disk_info[drive - 'A'];

	return p_disk_info;
}

//Note: do not export this function (for scan)
static _DISK_INFO* _iamovie_fm_get_diskinfo_for_scan(CHAR drive)
{
	_DISK_INFO *p_disk_info = NULL;

	if (_iamovie_fm_drive_is_invalid(drive)) {
		DBG_ERR("Invalid drive 0x%X\r\n", (UINT32)drive);
		return NULL;
	}

	p_disk_info = &g_disk_info2[drive - 'A'];

	return p_disk_info;
}

//Note: do not export this function (for scan)
static _SCAN_INFO* _iamovie_fm_get_scaninfo(CHAR drive)
{
	_SCAN_INFO *p_scan_info = NULL;

	if (_iamovie_fm_drive_is_invalid(drive)) {
		DBG_ERR("Invalid drive 0x%X\r\n", (UINT32)drive);
		return NULL;
	}

	p_scan_info = &g_scan_info[drive - 'A'];

	return p_scan_info;
}

//Note: do not export this function
static UINT32 _iamovie_fm_get_dir_depth(const char *dir_path)
{
	UINT32 len = strlen(dir_path);
	UINT32 depth = 0, i;
	for (i = 0; i < len; i++) {
		if (*(dir_path + i) == '\\') {
			depth++;
		}
	}
	DBG_IND("Dir %s len %d depth %d\r\n", dir_path, len, depth);
	return depth;
}

//Note: do not export this function (for scan)
static void _iamovie_fm_dir_cb(FIND_DATA *find_data, BOOL *b_continue, UINT16 *long_name, UINT32 param)
{
	_SCAN_INFO* p_san_info = NULL;
	_DISK_INFO* p_disk_info = NULL;
	_DIR_INFO*  p_dir_info = NULL;
	UINT32 path_len = IAMOVIE_FM_PATH_SIZE*4;
	CHAR path_name[IAMOVIE_FM_PATH_SIZE*4] = {0};
	CHAR drive = (CHAR)param;
	UINT32 i = 0;

	//p_san_info
	p_san_info = _iamovie_fm_get_scaninfo(drive);
	if (NULL == p_san_info) {
		DBG_ERR("get scan info failed, drive %c\r\n", drive);
		return ;
	}
	//p_disk_info
	p_disk_info = _iamovie_fm_get_diskinfo_for_scan(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return ;
	}

	//skip '.' and '..' and label entries but keep continued
	if (0 == strncmp(find_data->filename, ".", 1) || 0 == strncmp(find_data->filename, "..", 2) || M_IsVolumeLabel(find_data->attrib)) {
		return ;
	}

	// has long name
	if (long_name[0] != 0) {
		UINT32 i;
		for (i = 0; i < path_len; i++) {
			if (long_name[i] == 0) {
				break;
			}
			path_name[i] = (CHAR)long_name[i];
		}
	}
	// no long name
	else {
		strncpy(path_name, find_data->filename, path_len-1);
		path_name[path_len-1] = '\0';
	}
	DBG_IND("Cur Folder %s (%d)\r\n", p_san_info->cur_dir, p_san_info->cur_depth);
	DBG_IND("Get %s\r\n", path_name);

	// This is a folder
	if (M_IsDirectory(find_data->attrib)) {
		p_san_info->dir_count++;
		//p_dir_info
		i = 0;
		while (i != IAMOVIEMULTI_FM_DIR_NUM) {
			p_dir_info = &(p_disk_info->dir_info[i]);
			if (!p_dir_info->is_use) {
				break;
			}
			i++;
			if (i == IAMOVIEMULTI_FM_DIR_NUM) {
				DBG_ERR("dir info full, drive %c\r\n", drive);
				*b_continue = FALSE;
				return ;
			}
		}
		snprintf(p_dir_info->dir_path, sizeof(p_dir_info->dir_path), "%s%s\\", p_san_info->cur_dir, path_name);
		p_dir_info->is_use = 1;
		p_dir_info->parent_idx = p_san_info->cur_idx;
		DBG_IND("Get Folder %s [%d] parent[%d]\r\n", p_dir_info->dir_path, i, p_dir_info->parent_idx);
	}
	// This is a file
	else {
		if (p_san_info->cur_depth != 1) { // non-root
			INT32 cur_idx = p_san_info->cur_idx;
			p_dir_info = &(p_disk_info->dir_info[cur_idx]);
			if (p_dir_info == NULL) {
				DBG_ERR("dir idx error\r\n");
				return ;
			}
			if (FM_INVALID_IDX == p_dir_info->file_is_use) {
				return;
			}
			if (find_data->fileSize == 0) {
				p_dir_info->file_is_use = FM_INVALID_IDX;
				return;
			}
			p_dir_info->file_num++;
			p_dir_info->dir_size += find_data->fileSize;

			if (p_dir_info->file_size == 0) {
				p_dir_info->file_size = find_data->fileSize;
			} else {
				if (p_dir_info->file_size == find_data->fileSize) {
					p_dir_info->file_is_use = 1;
				} else {
					p_dir_info->file_is_use = FM_INVALID_IDX;
				}
			}
		} else { // root
		#if 0  // remove log
			CHAR cur_file[IAMOVIE_FM_PATH_SIZE*4] = {0};
			snprintf(cur_file, sizeof(cur_file), "%s%s", p_san_info->cur_dir, path_name);
			DBG_DUMP("Get File in Root: %s\r\n", cur_file);
		#endif // remove log
		}
	}

	return ;
}

//Note: dp not export this function
static PFILEDB_FILE_ATTR _iamovie_fm_find_hidden(FILEDB_HANDLE fdb_hdl, UINT32 *file_index, CHAR *dir_name)
{
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	UINT32 total_num, idx;
	CHAR tmp_dir[IAMOVIE_FM_PATH_SIZE] = {0};

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

		if (!M_IsHidden(p_file_attr->attrib)) {
			DBG_IND("%s normal.\r\n", p_file_attr->filePath);
			continue;
		}

		if (dir_name != NULL) {
			if (FST_STA_OK != FileSys_GetParentDir(p_file_attr->filePath, tmp_dir)) {
				//DBG_ERR("get dir error. file %s\r\n", p_file_attr->filePath);
			}

			if (strcmp(tmp_dir, dir_name)) {
				DBG_IND("%s tmp_dir %s not match %s.\r\n", p_file_attr->filePath, tmp_dir, dir_name);
				continue;
			}
		}

		*file_index = idx;
		DBG_IND("^G %s hidden file found.\r\n", p_file_attr->filePath);
		return p_file_attr;
	}

	return NULL;
}

//Note: dp not export this function
static PFILEDB_FILE_ATTR _iamovie_fm_find_file_by_dir(FILEDB_HANDLE fdb_hdl, UINT32 *file_index, CHAR *dir_name)
{
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	UINT32 total_num, idx;
	CHAR tmp_dir[IAMOVIE_FM_PATH_SIZE] = {0};
	UINT32 is_hidden = 0, is_skip_ro = 0;

	total_num = FileDB_GetTotalFileNum(fdb_hdl);
	if (0 == total_num) {
		DBG_WRN("FileDB is empty\r\n");
		return NULL;
	}

	//#NT#2020/07/13#Willy Su -begin
	//#NT# Support use hidden first
	is_hidden = iamoviemulti_fm_is_reuse_hidden_first(dir_name[0]);
	DBG_IND("^R hidden = %d\r\n", is_hidden);
	if (is_hidden) {
		p_file_attr = _iamovie_fm_find_hidden(fdb_hdl, &idx, dir_name);
		if (p_file_attr != NULL) {
			*file_index = idx;
			return p_file_attr;
		}
		DBG_IND("^G hidden file not found.\r\n");
	}
	//#NT#2020/07/13#Willy Su -end

	is_skip_ro = iamoviemulti_fm_is_skip_read_only(dir_name[0]);
	DBG_IND("^R skip = %d\r\n", is_skip_ro);

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

		if (is_skip_ro) {
			if (M_IsReadOnly(p_file_attr->attrib)) {
				DBG_IND("%s protected.\r\n", p_file_attr->filePath);
				continue;
			}
		}

		if (!(g_ia_moviemulti_delete_filter & p_file_attr->fileType)) {
			DBG_IND("path:%s type:%d pass\r\n", p_file_attr->filePath, p_file_attr->fileType);
			continue;
		}

		*file_index = idx;
		DBG_IND("^G %s file by dir %s found.\r\n", p_file_attr->filePath, dir_name);
		return p_file_attr;
	}

	// callback
	if (is_skip_ro) {
		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(0, MOVIE_USER_CB_EVENT_CARD_FULL, (UINT32) dir_name);
		}
	}

	return NULL;
}

//Note: dp not export this function
static PFILEDB_FILE_ATTR _iamovie_fm_find_file(FILEDB_HANDLE fdb_hdl, UINT32 *file_index)
{
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	UINT32 total_num, idx;

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

		if (M_IsReadOnly(p_file_attr->attrib)) {
			DBG_IND("%s protected.\r\n", p_file_attr->filePath);
			continue;
		}

		if (!(g_ia_moviemulti_delete_filter & p_file_attr->fileType)) {
			DBG_IND("path:%s type:%d pass\r\n", p_file_attr->filePath, p_file_attr->fileType);
			continue;
		}

		*file_index = idx;
		//DBG_DUMP("%s can be deleted.\r\n", p_file_attr->filePath);
		return p_file_attr;
	}

	return NULL;
}

//Note: dp not export this function
static PFILEDB_FILE_ATTR _iamovie_fm_find_ro(FILEDB_HANDLE fdb_hdl, UINT32 *file_index)
{
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	UINT32 total_num, idx;

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

		if (!M_IsReadOnly(p_file_attr->attrib)) {
			DBG_IND("%s normal.\r\n", p_file_attr->filePath);
			continue;
		}

		if (!(g_ia_moviemulti_delete_filter & p_file_attr->fileType)) {
			DBG_IND("path:%s type:%d pass\r\n", p_file_attr->filePath, p_file_attr->fileType);
			continue;
		}

		*file_index = idx;
		//DBG_DUMP("%s read-only file found.\r\n", p_file_attr->filePath);
		return p_file_attr;
	}

	return NULL;
}

//Note: dp not export this function
static INT32 _iamovie_fm_update_fdbinfo(CHAR drive)
{
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	_FDB_INFO *p_fdb_info = NULL;
	UINT32 total_num, idx;

	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return -1;
	}
	if (FILEDB_CREATE_ERROR == p_fdb_info->fdb_hdl) {
		DBG_ERR("fdb_hdl is not created, drive %c\r\n", drive);
		return -1;
	}

	p_fdb_info->rofile_size = 0;
	p_fdb_info->file_size = 0;

	total_num = FileDB_GetTotalFileNum(p_fdb_info->fdb_hdl);
	if (0 != total_num) {
		for (idx = 0; idx < total_num; idx++) {
			p_file_attr = FileDB_SearhFile(p_fdb_info->fdb_hdl, idx);
			if (p_file_attr == NULL) {
				DBG_ERR("get file failed\r\n");
				return -1;
			}
			if (M_IsReadOnly(p_file_attr->attrib)) {
				p_fdb_info->rofile_size += p_file_attr->fileSize64;
			}
			else {
				p_fdb_info->file_size += p_file_attr->fileSize64;
			}
		}
	}

	DBG_IND("^G fm_update rofile_size: %lld non_rofile_size: %lld\r\n",
		p_fdb_info->rofile_size, p_fdb_info->file_size);

	return 0;
}

//Note: dp not export this function
static INT32 _iamovie_fm_add_file_to_begin(CHAR *p_path)
{
	CHAR tmp_path[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	FILEDB_HANDLE fdb_hdl;
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	UINT32 len;
	INT32 ret;

	fdb_hdl = _iamovie_fm_get_fdb_hdl(p_path[0]);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		return -1;
	}

	p_file_attr = FileDB_SearhFile(fdb_hdl, 0);
	if (p_file_attr == NULL) {
		DBG_ERR("get file failed\r\n");
		return -1;
	}
	len = strlen(p_file_attr->filePath);
	strncpy(tmp_path, p_file_attr->filePath, len);
	tmp_path[len] = '\0';
	DBG_IND("[%s] found\r\n", tmp_path);

	//calc modDateTime
	{
		UINT32  TmpCreDT[FST_FILE_DATETIME_MAX_ID] = {0};
		UINT32  TmpModDT[FST_FILE_DATETIME_MAX_ID] = {0};
		UINT32  TmpN = FST_FILE_DATETIME_MAX_ID - 1;
		if (FST_STA_OK != FileSys_GetDateTime(tmp_path, TmpCreDT, TmpModDT)) {
			DBG_ERR("FileSys_GetDateTime of %s failed\r\n", tmp_path);
			goto l_fm_add_file_to_begin_end;
		}

		while (TmpN != FST_FILE_DATETIME_YEAR) {
			if (TmpModDT[TmpN] > 0) {
				TmpModDT[TmpN]--;
				break;
			} else {
				TmpN--;
			}
		}
		FileSys_SetDateTime(p_path, TmpCreDT, TmpModDT);
	}

l_fm_add_file_to_begin_end:
	//add to filedb
	ret = FileDB_AddFile(fdb_hdl, p_path);

	return ret;
}

INT32 iamoviemulti_fm_chk_fdb(CHAR drive)
{
	FILEDB_HANDLE fdb_hdl;
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	CHAR file_path[IAMOVIE_FM_PATH_SIZE*4] = {0};
	UINT32 file_index = 0;
	UINT64 time_cur = 0, time_min = 0, time_max = -1;
	UINT64 time_val;
	INT32 ret;
	INT32 time_flg = 0;
	BOOL bContinue = 1;
	UINT64 file_size;

	_FDB_INFO *p_fdb_info = NULL;
	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return -1;
	}

	fdb_hdl = _iamovie_fm_get_fdb_hdl(drive);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		return -1;
	}

	while (bContinue) {

		//check filedb first
		if (0 == FileDB_GetTotalFileNum(fdb_hdl)) {
			DBG_DUMP("FileDB is empty\r\n");
			return 0;
		}

		//find file from filedb
		p_file_attr = _iamovie_fm_find_file(fdb_hdl, &file_index);
		if (p_file_attr == NULL) {
			DBG_ERR("find file failed\r\n");
			return -1;
		}

		//chk time
		if (0 != iamoviemulti_fm_chk_time(p_file_attr->creDate,
				p_file_attr->creTime, &time_cur)) {
			DBG_IND("chk time failed\r\n");
		}
		if (time_cur < time_min || time_cur > time_max) {
			break;
		}

		//copy file_path
		strncpy(file_path, p_file_attr->filePath, sizeof(file_path)-1);
		file_path[sizeof(file_path)-1] = '\0';
		file_size = p_file_attr->fileSize64;

		//delete from file system
		ret = FileSys_DeleteFile(file_path);
		if (FSS_FILE_NOTEXIST == ret) {
			//try delete from filedb
			if (TRUE != FileDB_DeleteFile(fdb_hdl, file_index)) {
				DBG_ERR("FileDB_DeleteFile failed\r\n");
				return -1;
			}
			//update fdb info
			p_fdb_info->file_size -= file_size;
			continue;
		}
		if (FST_STA_OK != ret) {
			DBG_ERR("FileSys_DeleteFile %s failed\r\n", file_path);
			return -1;
		}
		DBG_DUMP("%s deleted from File System\r\n", file_path);

		//delete from filedb
		if (TRUE != FileDB_DeleteFile(fdb_hdl, file_index)) {
			DBG_ERR("FileDB_DeleteFile failed\r\n");
			return -1;
		}
		//update fdb info
		p_fdb_info->file_size -= file_size;
		DBG_DUMP("%s deleted from FileDB\r\n", file_path);

		//callback
		if (g_ia_moviemulti_usercb){
			g_ia_moviemulti_usercb(0, MOVIE_USRR_CB_EVENT_FILE_DELETED, (UINT32)file_path);
		}

		//set time interval
		if (!time_flg) {
			if (0 != iamoviemulti_fm_get_time_val(drive, &time_val)) {
				DBG_IND("get time val failed\r\n");
				time_val = 0;
			}
			time_min = time_cur - time_val;
			time_max = time_cur + time_val;
			time_flg = 1;
		}
	}

	return 0;
}

INT32 iamoviemulti_fm_chk_naming(CHAR *p_path)
{
	CHAR drive;
	const CHAR *pDot;
	INT32 ExtLen;

	if (NULL == p_path) {
		DBG_ERR("p_path is NULL\r\n");
		return -1;
	}

	drive = p_path[0];
	if (drive < IAMOVIEMULTI_FM_DRV_FIRST || drive > IAMOVIEMULTI_FM_DRV_LAST) {
		DBG_ERR("Invalid drive 0x%X\r\n", (UINT32)drive);
		return -1;
	}

	pDot = strrchr(p_path, '.');
	if (pDot) {
		ExtLen = strlen(p_path) - ((UINT32)pDot - (UINT32)p_path) - 1;//-1 is for '.'
	} else {
		ExtLen = 0;
	}
	if (ExtLen <= 0) {
		DBG_ERR("Invalid ExtName 0x%X\r\n", (UINT32)drive);
		return -1;
	}

	return 0;
}

INT32 iamoviemulti_fm_chk_time(UINT16 cre_date, UINT16 cre_time, UINT64 *time)
{
	UINT64 date_cur = (cre_date % 32) +
		(((cre_date >> 5) % 16) * 31) +
		((((cre_date >> 9) + 1980) * 12) * 31);
	UINT64 time_cur = (((cre_time >> 11) * 60) * 60) +
		(((cre_time >> 5) % 64) * 60) +
		((cre_time % 32) * 2);
	UINT64 time_ret = date_cur * 86400 + time_cur;

	DBG_IND("date_cur %lld time_cur %lld time %lld\r\n", date_cur, time_cur, time_ret);

	*time =  time_ret;

	return 0;
}

INT32 iamoviemulti_fm_set_time_val(CHAR drive, UINT64 value)
{
	UINT64 *p_time_info = NULL;

	p_time_info = _iamovie_fm_get_timeinfo(drive);
	if (NULL == p_time_info) {
		DBG_ERR("get time info failed, drive %c\r\n", drive);
		return -1;
	}
	#if 0
	if (value < 0) {
		DBG_ERR("set num error\r\n");
		return -1;
	}
	#endif
	*p_time_info = value;

	return 0;
}

INT32 iamoviemulti_fm_get_time_val(CHAR drive, UINT64 *value)
{
	UINT64 *p_time_info = NULL;

	p_time_info = _iamovie_fm_get_timeinfo(drive);
	if (NULL == p_time_info) {
		DBG_ERR("get time info failed, drive %c\r\n", drive);
		return -1;
	}

	*value = *p_time_info;

	return 0;
}

INT32 iamoviemulti_fm_set_file_del_max(CHAR drive, INT32 value)
{
	INT32 *p_file_max = NULL;

	p_file_max = _iamovie_fm_get_file_del_max(drive);
	if (NULL == p_file_max) {
		DBG_ERR("set file del max failed, drive %c\r\n", drive);
		return -1;
	}

	if (value < 0) {
		DBG_ERR("set num error\r\n");
		return -1;
	}

	*p_file_max = value;

	return 0;
}

INT32 iamoviemulti_fm_get_file_del_max(CHAR drive, INT32 *value)
{
	INT32 *p_file_max = NULL;

	p_file_max = _iamovie_fm_get_file_del_max(drive);
	if (NULL == p_file_max) {
		DBG_ERR("get file del max failed, drive %c\r\n", drive);
		return -1;
	}

	*value = *p_file_max;

	return 0;
}

INT32 iamoviemulti_fm_set_rofile_info(CHAR drive, INT32 value, INT32 type)
{
	_FILEOUT_RO_INFO *p_ro_info = NULL;

	p_ro_info = _iamovie_fm_get_roinfo(drive);
	if (NULL == p_ro_info) {
		DBG_ERR("get ro file info failed, drive %c\r\n", drive);
		return -1;
	}

	if (value < 0) {
		DBG_ERR("set num error\r\n");
		return -1;
	}

	switch (type) {
	case IAMOVIEMULTI_FM_ROINFO_PCT:
		if (value > 100) {
			DBG_ERR("set pct error\r\n");
			return -1;
		}
		p_ro_info->size_pct = value;
		break;
	case IAMOVIEMULTI_FM_ROINFO_NUM:
		p_ro_info->file_num = value;
		break;
	case IAMOVIEMULTI_FM_ROINFO_TYPE:
		if (value != IAMOVIEMULTI_FM_CHK_PCT && value != IAMOVIEMULTI_FM_CHK_NUM) {
			DBG_ERR("set type error\r\n");
			return -1;
		}
		p_ro_info->type_chk = value;
		break;
	default:
		DBG_ERR("type error\r\n");
		break;
	}

	return 0;
}

INT32 iamoviemulti_fm_get_rofile_info(CHAR drive, INT32 *value, INT32 type)
{
	_FILEOUT_RO_INFO *p_ro_info = NULL;

	p_ro_info = _iamovie_fm_get_roinfo(drive);
	if (NULL == p_ro_info) {
		DBG_ERR("get ro file info failed, drive %c\r\n", drive);
		return -1;
	}

	switch (type) {
	case IAMOVIEMULTI_FM_ROINFO_PCT:
		*value = p_ro_info->size_pct;
		break;
	case IAMOVIEMULTI_FM_ROINFO_NUM:
		*value = p_ro_info->file_num;
		break;
	case IAMOVIEMULTI_FM_ROINFO_TYPE:
		*value = p_ro_info->type_chk;
		break;
	default:
		DBG_ERR("type error\r\n");
		break;
	}

	return 0;
}

INT32 iamoviemulti_fm_set_resv_size(CHAR drive, UINT64 size, UINT32 is_loop)
{
	_FILEOUT_SIZE_INFO *p_size_info = NULL;

	p_size_info = _iamovie_fm_get_sizeinfo(drive);
	if (NULL == p_size_info) {
		DBG_ERR("get size info failed, drive %c\r\n", drive);
		return -1;
	}

	if (size >= FileSys_GetDiskInfoEx(drive, FST_INFO_DISK_SIZE)) {
		DBG_ERR("set size too big\r\n");
		return -1;
	}

	if (size < IAMOVIEMULTI_FM_RESV_SIZE) {
		size = IAMOVIEMULTI_FM_RESV_SIZE;
	}

	if (!is_loop) {
		//loop recording off
		p_size_info->resv_size_off = size;
	}
	else {
		//loop recording on
		p_size_info->resv_size_on = size;
	}

	return 0;
}

INT32 iamoviemulti_fm_get_resv_size(CHAR drive, UINT64 *p_size)
{
	_FILEOUT_SIZE_INFO *p_size_info = NULL;

	p_size_info = _iamovie_fm_get_sizeinfo(drive);
	if (NULL == p_size_info) {
		DBG_ERR("get size info failed, drive %c\r\n", drive);
		return -1;
	}

	if (!iamoviemulti_fm_is_loop_del(drive)) {
		//loop recording off
		*p_size = p_size_info->resv_size_off;
	}
	else {
		//loop recording on
		*p_size = p_size_info->resv_size_on;
	}

	return 0;
}

INT32 iamoviemulti_fm_get_remain_size(CHAR drive, UINT64 *p_size)
{
	UINT64 resv_size = 0;
	UINT64 remain_size, free_size;

	if (0 != iamoviemulti_fm_get_resv_size(drive, &resv_size)) {
		DBG_ERR("Get resv size error\r\n");
		return -1;
	}
	free_size = FileSys_GetDiskInfoEx(drive, FST_INFO_FREE_SPACE);

	if (free_size < resv_size) {
		remain_size = 0;
	}
	else {
		remain_size = free_size - resv_size;
	}

	*p_size = remain_size;

	return 0;
}

INT32 iamoviemulti_fm_get_filedb_num(CHAR drive)
{
	FILEDB_HANDLE fdb_hdl;
	UINT32 total_num;

	fdb_hdl = _iamovie_fm_get_fdb_hdl(drive);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		return -1;
	}

	total_num = FileDB_GetTotalFileNum(fdb_hdl);

	return total_num;
}

INT32 iamoviemulti_fm_keep_mtime(CHAR * p_path)
{
	INT32   ret;

	ret = FileSys_GetDateTime(p_path, g_TmpCreDT, g_TmpModDT);
	if (FST_STA_OK != ret) {
		DBG_ERR("FileSys_GetDateTime of %s failed\r\n", p_path);
		return -1;
	}

	return 0;
}

INT32 iamoviemulti_fm_sync_mtime(CHAR * p_path)
{
	INT32   ret;

	ret = FileSys_SetDateTime(p_path, g_TmpCreDT, g_TmpModDT);
	if (FST_STA_OK != ret) {
		DBG_ERR("FileSys_GetDateTime of %s failed\r\n", p_path);
	}

	return 0;
}

INT32 iamoviemulti_fm_dump_mtime(CHAR * p_path)
{
	UINT32  TmpCreDT[FST_FILE_DATETIME_MAX_ID] = {0};
	UINT32  TmpModDT[FST_FILE_DATETIME_MAX_ID] = {0};
	INT32   ret;

	ret = FileSys_GetDateTime(p_path, TmpCreDT, TmpModDT);
	if (FST_STA_OK != ret) {
		DBG_ERR("FileSys_GetDateTime of %s failed\r\n", p_path);
		return -1;
	} else {
		ImageApp_MovieMulti_DUMP("^G Path:%s MDate:%d/%d/%d MTime:%d/%d/%d\r\n", p_path,
			TmpModDT[0], TmpModDT[1], TmpModDT[2],
			TmpModDT[3], TmpModDT[3], TmpModDT[5]);
	}

	return 0;
}

INT32 iamoviemulti_fm_set_loop_del(CHAR drive, INT32 is_loop_del)
{
	_FDB_INFO *p_fdb_info = NULL;

	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return -1;
	}

	p_fdb_info->is_loop_del = is_loop_del;

	return 0;
}

INT32 iamoviemulti_fm_is_loop_del(CHAR drive)
{
	_FDB_INFO *p_fdb_info = NULL;

	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return 0;
	}

	return p_fdb_info->is_loop_del;
}

INT32 iamoviemulti_fm_set_add_to_last(CHAR drive, INT32 is_add_to_last)
{
	_FDB_INFO *p_fdb_info = NULL;

	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return -1;
	}

	p_fdb_info->is_add_to_last = is_add_to_last;

	return 0;
}

INT32 iamoviemulti_fm_is_add_to_last(CHAR drive)
{
	_FDB_INFO *p_fdb_info = NULL;

	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return 0;
	}

	return p_fdb_info->is_add_to_last;
}

INT32 iamoviemulti_fm_set_sync_time(CHAR drive, INT32 is_sync_time)
{
	_FDB_INFO *p_fdb_info = NULL;

	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return -1;
	}

	p_fdb_info->is_sync_time = is_sync_time;

	return 0;
}

INT32 iamoviemulti_fm_is_sync_time(CHAR drive)
{
	_FDB_INFO *p_fdb_info = NULL;

	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return 0;
	}

	return p_fdb_info->is_sync_time;
}

INT32 iamoviemulti_fm_set_ro_chk_last(CHAR drive, INT32 is_ro_chk_last)
{
	_FILEOUT_RO_INFO *p_ro_info = NULL;

	p_ro_info = _iamovie_fm_get_roinfo(drive);
	if (NULL == p_ro_info) {
		DBG_ERR("get ro file info failed, drive %c\r\n", drive);
		return -1;
	}

	if (is_ro_chk_last) {
		p_ro_info->chk_last = TRUE;
	} else {
		p_ro_info->chk_last = FALSE;
	}

	return 0;
}

INT32 iamoviemulti_fm_is_ro_chk_last(CHAR drive)
{
	_FILEOUT_RO_INFO *p_ro_info = NULL;

	p_ro_info = _iamovie_fm_get_roinfo(drive);
	if (NULL == p_ro_info) {
		DBG_ERR("get ro file info failed, drive %c\r\n", drive);
		return -1;
	}

	return p_ro_info->chk_last;
}

INT32 iamoviemulti_fm_set_format_free(CHAR drive, BOOL is_on)
{
	_DISK_INFO *p_disk_info = NULL;

	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}
	//reset
	memset(p_disk_info, 0, sizeof(_DISK_INFO));
	p_disk_info->is_format_free = is_on;

	if (iamoviemulti_fm_is_format_free(drive) != (INT32)is_on) {
		DBG_ERR("set format free might error\r\n");
	}

	// Set fileout
	#if 0
	ImageUnit_Begin(ImgLink[0].p_fileout, 0);
	ImageUnit_SetParam(ISF_CTRL, FILEOUT_PARAM_FORMAT_FREE, is_on); //TRUE or FALSE
	ImageUnit_End();
	#else
	{
		HD_FILEOUT_CONFIG fout_cfg = {0};
		HD_PATH_ID fileout_ctrl = 0;
		HD_RESULT hd_ret = 0;

		if ((hd_ret = hd_fileout_open(0, HD_FILEOUT_CTRL(0), &fileout_ctrl)) != HD_OK) {
			DBG_ERR("hd_fileout_open(HD_FILEOUT_CTRL) fail(%d)\n", hd_ret);
		}

		if ((hd_ret = hd_fileout_get(fileout_ctrl, HD_FILEOUT_PARAM_CONFIG, &fout_cfg)) != HD_OK) {
			DBG_ERR("hd_fileout_get(HD_FILEOUT_PARAM_CONFIG) fail(%d)\n", hd_ret);
		} else {
			DBG_DUMP("drive(%x) format_free(%d)\r\n", (unsigned int)fout_cfg.drive, (int)fout_cfg.format_free);
		}

		fout_cfg.format_free = (UINT32)is_on;

		if ((hd_ret = hd_fileout_set(fileout_ctrl, HD_FILEOUT_PARAM_CONFIG, &fout_cfg)) != HD_OK) {
			DBG_ERR("hd_fileout_set(HD_FILEOUT_PARAM_CONFIG) fail(%d)\n", hd_ret);
		}

		if ((hd_ret = hd_fileout_close(fileout_ctrl)) != HD_OK) {
			DBG_ERR("hd_fileout_close(HD_FILEOUT_CTRL) fail(%d)\n", hd_ret);
		}
	}
	#endif

	return 0;
}

INT32 iamoviemulti_fm_is_format_free(CHAR drive)
{
	_DISK_INFO *p_disk_info = NULL;

	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return 0;
	}

	return p_disk_info->is_format_free;
}

// p_dir : "A:\\Novatek\\Movie\\" , ratio : 70 , size : 180*1024*1024ULL
INT32 iamoviemulti_fm_set_dir_info(CHAR *p_dir, UINT32 ratio, UINT64 size)
{
	_DISK_INFO *p_disk_info = NULL;
	_DIR_INFO *p_dir_info = NULL;
	CHAR drive = p_dir[0];
	INT32 i = 0;
	//#NT#2020/04/06#Willy Su -begin
	//#NT# Calc the file-related nums to show the real situation of Card Space
	CHAR tmp_dir[IAMOVIE_FM_PATH_SIZE] = {0};
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	FILEDB_HANDLE fdb_hdl;
	UINT32 total_num;
	UINT32 idx;
	//#NT#2020/04/06#Willy Su -end

	//check
	#if 0
	if (!iamoviemulti_fm_is_format_free(drive)) {
		DBG_DUMP("^G[FM] Format-Free No Use\r\n");
		return 0;
	}
	#endif

	//p_disk_info
	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}

	//p_dir_info
	while (i != IAMOVIEMULTI_FM_DIR_NUM) {
		p_dir_info = &(p_disk_info->dir_info[i]);
		if (!p_dir_info->is_use) {
			break;
		}
		#if 0
		if (p_dir_info->is_use && !strcmp(p_dir, p_dir_info->dir_path)) {
			break;
		}
		#endif
		i++;
		if (i == IAMOVIEMULTI_FM_DIR_NUM) {
			DBG_ERR("dir info full, drive %c\r\n", drive);
			return -1;
		}
	}

	//copy
	#if 0
	if ((p_dir_info->dir_ratio != ratio) || (p_dir_info->file_size != size))
	#endif
	{
		memset(p_dir_info, 0, sizeof(_DIR_INFO));
		p_dir_info->dir_ratio = ratio;
		p_dir_info->file_size = size;
		snprintf(p_dir_info->dir_path, sizeof(p_dir_info->dir_path), "%s", p_dir);
		p_dir_info->is_use = 1;
		#if 0
		p_disk_info->need_format = TRUE;
		#endif
		//#NT#2020/04/06#Willy Su -begin
		//#NT# Calc the file-related nums to show the real situation of Card Space
		p_dir_info->file_num = 0;
		p_dir_info->file_is_use = 0;
		fdb_hdl = _iamovie_fm_get_fdb_hdl(drive);
		if (FILEDB_CREATE_ERROR == fdb_hdl) {
			DBG_ERR("get fdb_hdl failed\r\n");
			return -1;
		}
		total_num = FileDB_GetTotalFileNum(fdb_hdl);
		for (idx = 0; idx < total_num; idx++) {
			p_file_attr = FileDB_SearhFile(fdb_hdl, idx);
			if (p_file_attr == NULL) {
				DBG_WRN("p_file_attr NULL\r\n");
				continue;
			}
			if (FST_STA_OK != FileSys_GetParentDir(p_file_attr->filePath, tmp_dir)) {
				DBG_ERR("get dir error. file %s\r\n", p_file_attr->filePath);
			}
			if (strcmp(tmp_dir, p_dir)) {
				continue;
			}
			p_dir_info->file_num++;
			if (!M_IsHidden(p_file_attr->attrib)) {
				p_dir_info->file_is_use++;
			}
		}
		DBG_DUMP("DIR[%s] file: %d\r\n", p_dir, p_dir_info->file_is_use);
		//#NT#2020/04/06#Willy Su -end
		p_dir_info->is_cyclic_rec = 1;//default enable cyclic rec
		DBG_IND("%s copy\r\n", p_dir);
	}
	#if 0
	//create dir
	if (!p_dir_info->is_use) {
		if (FST_STA_OK != FileSys_MakeDir(p_dir_info->dir_path)) {
			DBG_ERR("MakeDir %s failed\r\n", p_dir_info->dir_path);
			return -1;
		}
		DBG_IND("%s created\r\n", p_dir);
	}
	#endif

	return 0;
}

// p_dir : "A:\\Novatek\\Movie\\" , ratio : 70 , size : 180*1024*1024ULL
INT32 iamoviemulti_fm_get_dir_info(CHAR *p_dir, UINT32 *ratio, UINT64 *size)
{
	_DISK_INFO *p_disk_info = NULL;
	_DIR_INFO *p_dir_info = NULL;
	CHAR drive = p_dir[0];
	INT32 i = 0;

	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}

	while (i != IAMOVIEMULTI_FM_DIR_NUM) {
		p_dir_info = &(p_disk_info->dir_info[i]);
		#if 0
		if (!p_dir_info->is_use) {
			break;
		}
		#endif
		if (p_dir_info->is_use && !strcmp(p_dir, p_dir_info->dir_path)) {
			break;
		}
		i++;
		if (i == IAMOVIEMULTI_FM_DIR_NUM) {
			DBG_ERR("dir not match, drive %c\r\n", drive);
			return -1;
		}
	}

	//copy
	{
		*ratio = p_dir_info->dir_ratio;
		*size = p_dir_info->file_size;
	}

	return 0;
}

INT32 iamoviemulti_fm_get_dir_size(CHAR *p_dir, UINT64 *p_size)
{
	_DISK_INFO *p_disk_info = NULL;
	_DIR_INFO *p_dir_info = NULL;
	CHAR drive = p_dir[0];
	INT32 i = 0;

	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}

	while (i != IAMOVIEMULTI_FM_DIR_NUM) {
		p_dir_info = &(p_disk_info->dir_info[i]);
		#if 0
		if (!p_dir_info->is_use) {
			break;
		}
		#endif
		if (p_dir_info->is_use && !strcmp(p_dir, p_dir_info->dir_path)) {
			break;
		}
		i++;
		if (i == IAMOVIEMULTI_FM_DIR_NUM) {
			DBG_ERR("dir info full, drive %c\r\n", drive);
			return -1;
		}
	}

	//copy
	{
		*p_size = p_dir_info->dir_size;
	}

	return 0;
}

INT32 iamoviemulti_fm_set_cur_dir_path(CHAR *p_dir)
{
	if (NULL == p_dir) {
		DBG_ERR("p_dir NULL\r\n");
		return -1;
	}

	snprintf(g_cur_dir_path, sizeof(g_cur_dir_path), "%s", p_dir);
	DBG_IND("^G Cur Dir Path %s\r\n", g_cur_dir_path);

	return 0;
}

INT32 iamoviemulti_fm_set_usage_size(CHAR drive, UINT64 size)
{
	_DISK_INFO *p_disk_info = NULL;

	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}

	p_disk_info->usage_size = size;
	return 0;
}

INT32 iamoviemulti_fm_get_usage_size(CHAR drive, UINT64 *p_size)
{
	_DISK_INFO *p_disk_info = NULL;

	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}

	*p_size = p_disk_info->usage_size;
	return 0;
}

INT32 iamoviemulti_fm_set_extstr_type(CHAR drive, UINT32 file_type)
{
	UINT32 *p_extstr_info = NULL;

	p_extstr_info = _iamovie_fm_get_extstrinfo(drive);
	if (NULL == p_extstr_info) {
		DBG_ERR("set extstr info failed, drive %c\r\n", drive);
		return -1;
	}

	*p_extstr_info = file_type;
	return 0;
}

INT32 iamoviemulti_fm_get_extstr_type(CHAR drive, UINT32* file_type)
{
	UINT32 *p_extstr_info = NULL;

	p_extstr_info = _iamovie_fm_get_extstrinfo(drive);
	if (NULL == p_extstr_info) {
		DBG_ERR("set extstr info failed, drive %c\r\n", drive);
		return -1;
	}

	*file_type = *p_extstr_info;
	return 0;
}

INT32 iamoviemulti_fm_get_extstr(CHAR drive, char *extstr)
{
	UINT32 type = 0; //CID 156519 [UNINIT]
	UINT16 i;
	iamoviemulti_fm_get_extstr_type(drive, &type);
	for (i = 0; i < FILEDB_SUPPORT_FILE_TYPE_NUM; i++) {
		if (extstr_table[i].fileType & type) {
			strcpy(extstr, extstr_table[i].filterStr);
			DBG_DUMP("^G extstr=%s\r\n", extstr);
			return 0;
		}
	}
	return -1;
}

INT32 iamoviemulti_fm_is_reuse_hidden_first(CHAR drive)
{
	_DISK_INFO *p_disk_info = NULL;

	if (!iamoviemulti_fm_is_format_free(drive)) {
		return 0; //Return FALSE since Format-Free No Use
	}
	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return 0; //Return FALSE
	}
	return p_disk_info->is_reuse_hidden_first;
}

INT32 iamoviemulti_fm_set_reuse_hidden_first(CHAR drive, BOOL value)
{
	_DISK_INFO *p_disk_info = NULL;

	if (!iamoviemulti_fm_is_format_free(drive)) {
		DBG_DUMP("^G[FM] Format-Free No Use\r\n");
		return 0;
	}
	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}
	if (value) {
		p_disk_info->is_reuse_hidden_first = TRUE;
	} else {
		p_disk_info->is_reuse_hidden_first = FALSE;
	}
	DBG_DUMP("^G reuse_hidden_first=%d\r\n", p_disk_info->is_reuse_hidden_first);
	return 0;
}

INT32 iamoviemulti_fm_is_skip_read_only(CHAR drive)
{
	_DISK_INFO *p_disk_info = NULL;

	if (!iamoviemulti_fm_is_format_free(drive)) {
		return 0; //Return FALSE since Format-Free No Use
	}
	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return 0; //Return FALSE
	}
	return p_disk_info->is_skip_read_only;
}

INT32 iamoviemulti_fm_set_skip_read_only(CHAR drive, BOOL value)
{
	_DISK_INFO *p_disk_info = NULL;

	if (!iamoviemulti_fm_is_format_free(drive)) {
		DBG_DUMP("^G[FM] Format-Free No Use\r\n");
		return 0;
	}
	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}
	if (value) {
		p_disk_info->is_skip_read_only = TRUE;
	} else {
		p_disk_info->is_skip_read_only = FALSE;
	}
	DBG_DUMP("^G skip_read_only=%d\r\n", p_disk_info->is_skip_read_only);
	return 0;
}

INT32 iamoviemulti_fm_set_file_unit(CHAR * p_dir, UINT32 unit)
{
	_DISK_INFO *p_disk_info = NULL;
	_DIR_INFO *p_dir_info = NULL;
	CHAR drive = p_dir[0];
	INT32 i = 0;

	DBG_IND("p_dir = %s\r\n", p_dir);

	//p_disk_info
	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}

	for (i = 0; i < IAMOVIEMULTI_FM_DIR_NUM; i++) {
		//p_dir_info
		p_dir_info = &(p_disk_info->dir_info[i]);
		if (strcmp(p_dir_info->dir_path, p_dir) == 0) {
			p_dir_info->file_unit = unit;
			break;
		}
		if (i == (IAMOVIEMULTI_FM_DIR_NUM - 1)) {
			DBG_ERR("no dir in disk, %s\r\n", p_dir);
			return -1;
		}
	}

	return 0;
}

INT32 iamoviemulti_fm_add_file(CHAR *p_path)
{
	FILEDB_HANDLE fdb_hdl;
	INT32 file_idx;
	INT32 ret;
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	UINT32 file_type = 0;

	if (NULL == p_path) {
		DBG_ERR("p_path is NULL\r\n");
		return -1;
	}

	_FDB_INFO *p_fdb_info = NULL;
	p_fdb_info = _iamovie_fm_get_fdbinfo(p_path[0]);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", p_path[0]);
		return -1;
	}

	fdb_hdl = _iamovie_fm_get_fdb_hdl(p_path[0]);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		return -1;
	}

	ret = FileDB_GetFileTypeByPath(p_path, &file_type);
	if (0 != ret) {
		DBG_ERR("Get file type failed, %s\r\n", p_path);
		return -1;
	}
	DBG_IND("file_type %d filedb_filter %d \r\n", file_type, g_ia_moviemulti_filedb_filter);
	//filedb filter file returm
	if (!(g_ia_moviemulti_filedb_filter & file_type)) {
		return 0;
	}

	file_idx = FileDB_GetIndexByPath(fdb_hdl, p_path);
	if (FILEDB_SEARCH_ERR == file_idx) {
		//not exist, check filedb first
		while (g_ia_moviemulti_filedb_max_num <= FileDB_GetTotalFileNum(fdb_hdl)) {
			ret = iamoviemulti_fm_chk_fdb(p_path[0]);
			//ret = iamoviemulti_fm_del_file(p_path[0], 0);
			if (0 != ret) {
				DBG_ERR("check filedb failed\r\n");
				return -1;
			}
		}

		//can add file to filedb
		if (iamoviemulti_fm_is_add_to_last(p_path[0])) {
			ret = FileDB_AddFileToLast(fdb_hdl, p_path);
		} else {
			ret = FileDB_AddFile(fdb_hdl, p_path);
		}
		if (FALSE == ret) {
			DBG_ERR("FileDB_AddFile %s failed\r\n", p_path);
			return -1;
		}
		//update fdb info
		UINT64 file_size;
		file_size = FileSys_GetFileLen(p_path);
		p_fdb_info->file_size += file_size;
		DBG_DUMP("%s added to FileDB\r\n", p_path);
		//exam_msg("normal path=%s\r\n", p_path);

	} else {
		//exist, update the size
		UINT64 old_size = 0;
		UINT64 new_size = 0;

		p_file_attr = FileDB_SearhFile(fdb_hdl, file_idx);
		if (p_file_attr == NULL) {
			DBG_ERR("get file failed\r\n");
			return -1;
		}
		old_size = p_file_attr->fileSize64;

		new_size = FileSys_GetFileLen(p_path);
		if (0 == new_size) {
			DBG_ERR("FileSys_GetFileLen failed\r\n");
			return -1;
		}
		DBG_IND("new_size %d\r\n", new_size);
		//set file size in filedb
		if (FALSE == FileDB_SetFileSize(fdb_hdl, file_idx, new_size)) {
			DBG_ERR("FileDB_SetFileSize failed\r\n");
			return -1;
		}
		//update fdb info
		p_fdb_info->file_size += (new_size - old_size);
		DBG_DUMP("%s set size to FileDB\r\n", p_path);
	}

	//dump info
	//FileDB_DumpInfo(fdb_hdl);

	return 0;
}

INT32 iamoviemulti_fm_del_file(CHAR drive, UINT64 need_size)
{
	FILEDB_HANDLE fdb_hdl;
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	CHAR file_path[IAMOVIE_FM_PATH_SIZE*4] = {0};
	UINT64 del_size = 0;
	UINT64 file_size = 0;
	UINT32 file_index = 0;
	UINT64 time_cur = 0;
	UINT64 time_min = 0;
	UINT64 time_max = -1;
	UINT64 time_val;
	INT32 time_flg = 0;
	INT32 del_num = 0, del_max = 0;
	BOOL bContinue = 1;
	INT32 ret;
	INT32 type_chk;

	DBG_IND("begin\r\n");

	_FDB_INFO *p_fdb_info = NULL;
	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return -1;
	}

	fdb_hdl = _iamovie_fm_get_fdb_hdl(drive);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		return -1;
	}

	iamoviemulti_fm_get_file_del_max(drive, &del_max);
	if (del_max < 0) {
		DBG_ERR("get del max error\r\n");
		del_max = 0;
	}

	while (bContinue) {

		//check filedb first
		if (0 == FileDB_GetTotalFileNum(fdb_hdl)) {
			DBG_DUMP("FileDB is empty\r\n");
			return 0;
		}

		//find file from filedb
		p_file_attr = _iamovie_fm_find_file(fdb_hdl, &file_index);
		if (p_file_attr == NULL) {
			if (iamoviemulti_fm_is_ro_chk_last(drive)) {
			// try to find ro (if type is IAMOVIEMULTI_FM_CHK_PCT)
				goto label_chk_ro_size;
			}
			else {
				DBG_ERR("find file failed\r\n");
				return -1;
			}
		}

		//chk time
		ret = iamoviemulti_fm_chk_time(p_file_attr->creDate, p_file_attr->creTime, &time_cur);
		if (0 != ret) {
			DBG_ERR("chk time failed\r\n");
			return -1;
		}

		if (time_cur < time_min || time_cur > time_max) {
			if (del_size < need_size) {
				time_flg = 0;
			} else {
				break;
			}
		} else {
			if (del_num >= del_max) { //deletion protect for slowcard and system hang
				DBG_DUMP("delete file (%d) reach max (%d) due to file ctime\r\n", del_num, del_max);
				DBG_DUMP("delete size (%lld) need size (%lld)\r\n", del_size, need_size);
				break;
			}
		}

		//copy file_path & file_size
		strncpy(file_path, p_file_attr->filePath, sizeof(file_path)-1);
		file_path[sizeof(file_path)-1] = '\0';
		file_size = p_file_attr->fileSize64;

		ret = FileSys_DeleteFile(file_path);
		if (FSS_FILE_NOTEXIST == ret) {
			//delete from filedb
			if (TRUE != FileDB_DeleteFile(fdb_hdl, file_index)) {
				DBG_ERR("FileDB_DeleteFile failed\r\n");
				return -1;
			}
			//update fdb info
			p_fdb_info->file_size -= file_size;
			continue;
		}
		if (FST_STA_SDIO_ERR == ret) {
			//callback
			if (g_ia_moviemulti_usercb) {
				g_ia_moviemulti_usercb(0, MOVIE_USER_CB_ERROR_CARD_WR_ERR, 0);
			}
		}
		if (FST_STA_OK != ret) {
			DBG_ERR("FileSys_DeleteFile %s failed\r\n", file_path);
			return -1;
		}
		DBG_DUMP("%s deleted from File System\r\n", file_path);

		del_size += file_size;
		del_num  ++;

		if (TRUE != FileDB_DeleteFile(fdb_hdl, file_index)) {
			DBG_ERR("FileDB_DeleteFile failed\r\n");
			return -1;
		}
		//update fdb info
		p_fdb_info->file_size -= file_size;
		DBG_DUMP("%s deleted from FileDB\r\n", file_path);

		//callback
		if (g_ia_moviemulti_usercb){
			g_ia_moviemulti_usercb(0, MOVIE_USRR_CB_EVENT_FILE_DELETED, (UINT32)file_path);
		}

		//set time interval
		if (!time_flg) {
			if (0 != iamoviemulti_fm_get_time_val(drive, &time_val)) {
				DBG_IND("get time val failed\r\n");
				time_val = 0;
			}
			time_min = time_cur - time_val;
			time_max = time_cur + time_val;
			time_flg = 1;
		}
	}

	//dump info
	//FileDB_DumpInfo(fdb_hdl);

label_chk_ro_size:
	if (iamoviemulti_fm_is_ro_chk_last(drive)) {
		//get chk ro type
		if (0 != iamoviemulti_fm_get_rofile_info(drive, &type_chk, IAMOVIEMULTI_FM_ROINFO_TYPE)) {
			DBG_ERR("get chk ro type failed\r\n");
			return -1;
		}

		if (type_chk == IAMOVIEMULTI_FM_CHK_PCT) {
			//Check RO pct
			if (0 != iamoviemulti_fm_chk_ro_size(drive)) {
				DBG_ERR("check read-only file size failed, drive %c\r\n", drive);
				return -1;
			}
			DBG_IND("Check read-only file size done.\r\n");
		}
	}

	DBG_IND("end\r\n");
	return 0;
}

INT32 iamoviemulti_fm_reuse_file(CHAR *p_new_path)
{
	FILEDB_HANDLE fdb_hdl;
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	CHAR tmp_path[IAMOVIE_FM_PATH_SIZE*4] = {0};
	CHAR tmp_dir[IAMOVIE_FM_PATH_SIZE] = {0};
	CHAR tmp_name[IAMOVIE_FM_PATH_SIZE] = {0};
	CHAR *pos;
	UINT32 path_len;
	UINT32 file_index = 0;
	UINT32 dir_len;
	UINT32 len;
	UINT32 total_num;
	UINT32 ret_val = 0;
	UINT8 attrib = 0;

	//#NT#2021/03/15#Willy Su -begin
	//#NT# Add lock-unlock for replacement flow
	vos_sem_wait(IAMOVIE_SEMID_FM);
	//#NT#2021/03/15#Willy Su -end

	fdb_hdl = _iamovie_fm_get_fdb_hdl(p_new_path[0]);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		ret_val = -1;
		goto lable_fm_reuse_file_end;
	}

	total_num = FileDB_GetTotalFileNum(fdb_hdl);
	if (0 == total_num) {
		DBG_WRN("[reuse] FileDB is empty. Refresh.\r\n");
		FileDB_Refresh(fdb_hdl);
		if (0 == FileDB_GetTotalFileNum(fdb_hdl)) {
			DBG_WRN("[reuse] FileDB is empty\r\n");
			ret_val = -1;
			goto lable_fm_reuse_file_end;
		}
	} else {
		DBG_DUMP("[reuse] total_num=%d\r\n", total_num);
	}

	//get dir name
	if (FST_STA_OK != FileSys_GetParentDir(p_new_path, tmp_dir)) {
		DBG_ERR("get dir error. file %s\r\n", p_new_path);
		ret_val = -1;
		goto lable_fm_reuse_file_end;
	}
	dir_len = strlen(tmp_dir);
	path_len = strlen(p_new_path);

	//get file name
	pos = p_new_path + dir_len;
	strncpy(tmp_name, pos, path_len-dir_len);
	tmp_name[path_len-dir_len] = '\0';

	//find file from filedb
	//#NT#2020/07/13#Willy Su -begin
	//#NT# Support use hidden first
	p_file_attr = _iamovie_fm_find_file_by_dir(fdb_hdl, &file_index, tmp_dir);
	//#NT#2020/07/13#Willy Su -end
	if (p_file_attr == NULL) {
		DBG_ERR("find file failed\r\n");
		ret_val = -1;
		goto lable_fm_reuse_file_end;
	}
	len = strlen(p_file_attr->filePath);
	strncpy(tmp_path, p_file_attr->filePath, len);
	tmp_path[len] = '\0';

	//delete from filedb
	if (TRUE != FileDB_DeleteFile(fdb_hdl, file_index)) {
		DBG_ERR("FileDB_DeleteFile failed\r\n");
		ret_val = -1;
		goto lable_fm_reuse_file_end;
	}
	DBG_DUMP("[reuse] %s deleted from FileDB\r\n", tmp_path);

	FileSys_GetAttrib(tmp_path, &attrib);
	if (attrib & FST_ATTRIB_READONLY) {
		FileSys_SetAttrib(tmp_path, attrib, FALSE);
	}

	//reset file name
	//if (FST_STA_OK != FileSys_MoveFile(tmp_path, p_new_path)) {
	if (FST_STA_OK != FileSys_RenameFile(tmp_name, tmp_path, FALSE)) {
		DBG_ERR("Reuse to %s failed\r\n", p_new_path);
		ret_val = -1;
		goto lable_fm_reuse_file_end;
	}

	//reset attrib
	if (FST_STA_OK != FileSys_SetAttrib(p_new_path, FST_ATTRIB_HIDDEN, FALSE)) {
		DBG_ERR("Set attrib to %s failed\r\n", p_new_path);
		ret_val = -1;
		goto lable_fm_reuse_file_end;
	}
	if (attrib & FST_ATTRIB_READONLY) {
		FileSys_SetAttrib(p_new_path, attrib, TRUE);
	}

	//#NT#2020/04/20#Willy Su -begin
	//#NT# Move from iamoviemulti_fm_add_file() to iamoviemulti_fm_reuse_file(),
	//#NT# since the file-related nums should be updated immediately.
	_DISK_INFO * p_disk_info = NULL;
	//p_disk_info
	p_disk_info = _iamovie_fm_get_diskinfo(p_new_path[0]);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", p_new_path[0]);
		ret_val = -1;
		goto lable_fm_reuse_file_end;
	}
	if(p_disk_info->is_format_free){
		UINT16 i=0;
		CHAR tmp_dir[IAMOVIE_FM_PATH_SIZE] = {0};
		_DIR_INFO *   p_dir_info = NULL;
		//get dir name
		if (FST_STA_OK != FileSys_GetParentDir(p_new_path, tmp_dir)) {
			DBG_ERR("get dir error. file %s\r\n", p_new_path);
			ret_val = -1;
			goto lable_fm_reuse_file_end;
		}
		for (i = 0; i < IAMOVIEMULTI_FM_DIR_NUM; i++) {
			//p_dir_info
			p_dir_info = &(p_disk_info->dir_info[i]);
			if(strcmp(p_dir_info->dir_path, tmp_dir)==0){
				if (p_dir_info->file_is_use < p_dir_info->file_num) {
					p_dir_info->file_is_use++;
				}
				break;
			}
		}
		DBG_IND("p_path=%s\r\n", p_new_path);
		DBG_IND("file_is_use=%d\r\n", p_dir_info->file_is_use);
	}
	//#NT#2020/04/20#Willy Su -end

lable_fm_reuse_file_end:
	//#NT#2021/03/03#Willy Su -begin
	//#NT# Add lock-unlock for replacement flow
	vos_sem_sig(IAMOVIE_SEMID_FM);
	//#NT#2021/03/03#Willy Su -end

	if (ret_val != 0) {
		return ret_val;
	}

	return 0;
}

INT32 iamoviemulti_fm_replace_file(CHAR *p_old_path, CHAR *p_new_folder, CHAR *p_new_path)
{
	CHAR rep_path[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	CHAR tmp_path[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	CHAR old_dir[IAMOVIE_FM_PATH_SIZE] = {0};
	CHAR *p_name;
	FILEDB_HANDLE fdb_hdl;
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	UINT32 file_index = 0;
	UINT32 len;
	INT32 file_idx;
	UINT8 attrib = 0;
	UINT32 old_ratio, new_ratio;
	UINT64 old_fsize = 0, new_fsize = 0;
	INT32 ret;
	INT32 ret_val = 0;
	UINT32 total_num;
	//#NT#2020/08/18#Willy Su -begin
	//#NT# Support set ext str with type setting
	CHAR ext[4] = {0};
	static UINT32 file_sn = 0, file_sn_tmp = 0;
	//#NT#2020/08/18#Willy Su -end

	//#NT#2021/03/03#Willy Su -begin
	//#NT# Add lock-unlock for replacement flow
	vos_sem_wait(IAMOVIE_SEMID_FM);
	//#NT#2021/03/03#Willy Su -end

	//get replace path and store old path info
	snprintf(rep_path, sizeof(rep_path), "%s", p_old_path);
	strncpy(p_new_path, rep_path, sizeof(rep_path)-1);
	p_new_path[sizeof(rep_path)-1] = '\0';

	if (NULL == p_old_path) {
		DBG_ERR("p_old_path is NULL\r\n");
		ret_val = -1;
		goto label_fm_replace_file_end_without_addfile;
	}

	if (NULL == p_new_folder) {
		DBG_ERR("p_new_folder is NULL\r\n");
		ret_val = -1;
		goto label_fm_replace_file_end_without_addfile;
	}

	if (FST_STA_OK != FileSys_GetParentDir(p_old_path, old_dir)) {
		DBG_ERR("get dir error. file %s\r\n", p_old_path);
		ret_val = -1;
		goto label_fm_replace_file_end_without_addfile;
	}
	iamoviemulti_fm_get_dir_info(old_dir, &old_ratio, &old_fsize);
	iamoviemulti_fm_get_dir_info(p_new_folder, &new_ratio, &new_fsize);
	if (old_fsize != new_fsize) {
		DBG_ERR("fsize not match (%s / %s), not move\r\n", old_dir, p_new_folder);
		ret_val = -1;
		goto label_fm_replace_file_end_without_addfile;
	}

	fdb_hdl = _iamovie_fm_get_fdb_hdl(p_old_path[0]);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		ret_val = -1;
		goto label_fm_replace_file_end_without_addfile;
	}

	total_num = FileDB_GetTotalFileNum(fdb_hdl);
	if (0 == total_num) {
		DBG_WRN("[replace] FileDB is empty. Refresh.\r\n");
		FileDB_Refresh(fdb_hdl);
		if (0 == FileDB_GetTotalFileNum(fdb_hdl)) {
			DBG_WRN("[replace] FileDB is empty\r\n");
			ret_val = -1;
			goto label_fm_replace_file_end_without_addfile;
		}
	} else {
		DBG_DUMP("[replace] total_num=%d\r\n", total_num);
	}

	//#NT#2020/08/18#Willy Su -begin
	//#NT# Support set ext str with type setting
	if (0 != iamoviemulti_fm_get_extstr(rep_path[0], ext)) {
		strcpy(ext, extstr_table[2].filterStr);
	}
	file_sn_tmp = file_sn;
label_make_rep_path:
	snprintf(rep_path, sizeof(rep_path), "%s%08d.%s", old_dir, (int)file_sn, ext);
	file_sn ++;
	if (file_sn == 9999) {
		file_sn = 0;
	}
	ret = FileSys_GetAttrib(rep_path, &attrib);
	if (FST_STA_FILE_NOT_EXIST != ret) {
		if (FST_STA_SDIO_ERR == ret) {
			DBG_ERR("sdio err\r\n");
			ret_val = -1;
			goto label_fm_replace_file_end_without_addfile;
		}
		if (FST_STA_PARAM_ERR == ret) {
			DBG_ERR("param err\r\n");
			ret_val = -1;
			goto label_fm_replace_file_end_without_addfile;
		}
		if (file_sn_tmp == file_sn) {
			DBG_ERR("rep_path exist. sn=%d\r\n", file_sn);
			ret_val = -1;
			goto label_fm_replace_file_end_without_addfile;
		}
		goto label_make_rep_path;
	}
	DBG_DUMP("rep_path = %s\r\n", rep_path);
	//#NT#2020/08/18#Willy Su -end

	//STEP1: replace new path
	{
		//1. get new path
		memset(tmp_path, 0, sizeof(tmp_path));
		p_name = strrchr(p_old_path, '\\');
		if (NULL == p_name) {
			DBG_ERR("Find name of %s failed\r\n", p_old_path);
			ret_val = -1;
			goto label_fm_replace_file_end_without_addfile;
		}
		p_name++;
		snprintf(tmp_path, sizeof(tmp_path), "%s%s", p_new_folder, p_name);
		//#NT#2020/04/28#Willy Su -begin
		//#NT# Add for cyclic rec check
		//1.1 check cyclic rec
		if (iamoviemulti_fm_chk_format_free_dir_full(tmp_path) > 0) {
			DBG_DUMP("^R format-free folder [%s] full\r\n", p_new_folder);
			if (g_ia_moviemulti_usercb) {
				g_ia_moviemulti_usercb(0, MOVIE_USER_CB_EVENT_CARD_FULL, (UINT32) p_new_folder);
			}
			ret_val = -1;
			goto label_fm_replace_file_end_without_addfile;
		}
		//#NT#2020/04/28#Willy Su -end
		DBG_IND("^G Get [%s] new path\r\n", tmp_path);

		//2. replace
		//delete from filedb
		file_idx = FileDB_GetIndexByPath(fdb_hdl, p_old_path);
		if (FILEDB_SEARCH_ERR == file_idx) {
			DBG_ERR("%s not found in FileDB\r\n", p_old_path);
			ret_val = -1;
			goto label_fm_replace_file_end_without_addfile;
		}
		if (TRUE != FileDB_DeleteFile(fdb_hdl, file_idx)) {
			DBG_ERR("FileDB_DeleteFile failed\r\n");
			ret_val = -1;
			goto label_fm_replace_file_end_without_addfile;
		}
		DBG_DUMP("^G [replace] %s deleted from FileDB\r\n", p_old_path);
		FileSys_GetAttrib(p_old_path, &attrib);
		if (attrib & FST_ATTRIB_READONLY) {
			FileSys_SetAttrib(p_old_path, FST_ATTRIB_READONLY, FALSE);
		}
		if (FST_STA_OK != FileSys_MoveFile(p_old_path, tmp_path)) {
			DBG_ERR("Reuse to %s failed\r\n", tmp_path);
			ret_val = -1;
			goto label_fm_replace_file_end_without_addfile;
		}
		//set attrib
		if (FST_STA_OK != FileSys_SetAttrib(tmp_path, FST_ATTRIB_HIDDEN, FALSE)) {
			DBG_ERR("Set attrib to %s failed\r\n", tmp_path);
			ret_val = -1;
			goto label_fm_replace_file_end_without_addfile;
		}
		DBG_IND("^G [STEP1] rename [%s] to [%s]\r\n", p_old_path, tmp_path);

		//#NT#2020/04/28#Willy Su -begin
		//#NT# The file-related nums should be updated immediately,
		//#NT# because replacement equals reuse.
		_DISK_INFO * p_disk_info = NULL;
		//p_disk_info
		p_disk_info = _iamovie_fm_get_diskinfo(tmp_path[0]);
		if (NULL == p_disk_info) {
			DBG_ERR("get disk info failed, drive %c\r\n", tmp_path[0]);
			ret_val = -1;
			goto label_fm_replace_file_end_without_addfile;
		}
		if(p_disk_info->is_format_free){
			UINT16 i=0;
			CHAR tmp_dir[IAMOVIE_FM_PATH_SIZE] = {0};
			_DIR_INFO *   p_dir_info = NULL;
			//get dir name
			if (FST_STA_OK != FileSys_GetParentDir(tmp_path, tmp_dir)) {
				DBG_ERR("get dir error. file %s\r\n", tmp_path);
				ret_val = -1;
				goto label_fm_replace_file_end_without_addfile;
			}
			for (i = 0; i < IAMOVIEMULTI_FM_DIR_NUM; i++) {
				//p_dir_info
				p_dir_info = &(p_disk_info->dir_info[i]);
				if(strcmp(p_dir_info->dir_path, tmp_dir)==0){
					if (p_dir_info->file_is_use < p_dir_info->file_num) {
						p_dir_info->file_is_use++;
					}
					break;
				}
			}
			DBG_IND("p_path=%s\r\n", tmp_path);
			DBG_IND("file_is_use=%d\r\n", p_dir_info->file_is_use);
		}
		//#NT#2020/04/28#Willy Su -end
	}

	//store new path info
	strncpy(p_new_path, tmp_path, sizeof(tmp_path)-1);
	p_new_path[sizeof(tmp_path)-1] = '\0';

	//STEP2: handle path replaced
	{
		//1. find file from filedb (p_new_folder)
		//#NT#2020/07/13#Willy Su -begin
		//#NT# Support use hidden first
		p_file_attr = _iamovie_fm_find_file_by_dir(fdb_hdl, &file_index, p_new_folder);
		//#NT#2020/07/13#Willy Su -end
		if (p_file_attr == NULL) {
			DBG_ERR("find file failed\r\n");
			ret_val = -1;
			goto label_fm_replace_file_end;
		}
		len = strlen(p_file_attr->filePath);
		strncpy(tmp_path, p_file_attr->filePath, len);
		tmp_path[len] = '\0';
		DBG_IND("^G Find [%s] from [%s]\r\n", tmp_path, p_new_folder);

		//2. being replaced
		//delete from filedb
		DBG_IND("^G Get [%s] to be placed\r\n", rep_path);
		if (TRUE != FileDB_DeleteFile(fdb_hdl, file_index)) {
			DBG_ERR("FileDB_DeleteFile failed\r\n");
			ret_val = -1;
			goto label_fm_replace_file_end;
		}
		DBG_IND("^G %s deleted from FileDB\r\n", tmp_path);
		FileSys_GetAttrib(tmp_path, &attrib);
		if (attrib & FST_ATTRIB_READONLY) {
			FileSys_SetAttrib(tmp_path, FST_ATTRIB_READONLY, FALSE);
		}
		//move file
		if (FST_STA_OK != FileSys_MoveFile(tmp_path, rep_path)) {
			DBG_ERR("Reuse to %s failed\r\n", rep_path);
			ret_val = -1;
			goto label_fm_replace_file_end;
		}
		//reset attrib
		if (FST_STA_OK != FileSys_SetAttrib(rep_path, FST_ATTRIB_HIDDEN, TRUE)) {
			DBG_ERR("Set attrib to %s failed\r\n", rep_path);
			ret_val = -1;
			goto label_fm_replace_file_end;
		}
		//#NT#2020/04/28#Willy Su -begin
		//#NT# Changed to add files to begin of FileDB
		//add to filedb
		ret = _iamovie_fm_add_file_to_begin(rep_path);
		//#NT#2020/04/28#Willy Su -end
		if (FALSE == ret) {
			DBG_ERR("FileDB_AddFile %s failed\r\n", rep_path);
			ret_val = -1;
			goto label_fm_replace_file_end;
		}
		DBG_IND("^G [STEP2] rename [%s] to [%s]\r\n", tmp_path, rep_path);

		//#NT#2020/04/28#Willy Su -begin
		//#NT# The file-related nums should be updated immediately,
		//#NT# because replacement equals reuse.
		_DISK_INFO * p_disk_info = NULL;
		//p_disk_info
		p_disk_info = _iamovie_fm_get_diskinfo(rep_path[0]);
		if (NULL == p_disk_info) {
			DBG_ERR("get disk info failed, drive %c\r\n", rep_path[0]);
			ret_val = -1;
			goto label_fm_replace_file_end;
		}
		if(p_disk_info->is_format_free){
			UINT16 i=0;
			CHAR tmp_dir[IAMOVIE_FM_PATH_SIZE] = {0};
			_DIR_INFO *   p_dir_info = NULL;
			//get dir name
			if (FST_STA_OK != FileSys_GetParentDir(rep_path, tmp_dir)) {
				DBG_ERR("get dir error. file %s\r\n", rep_path);
			}
			for (i = 0; i < IAMOVIEMULTI_FM_DIR_NUM; i++) {
				//p_dir_info
				p_dir_info = &(p_disk_info->dir_info[i]);
				if(strcmp(p_dir_info->dir_path, tmp_dir)==0){
					p_dir_info->file_is_use--;
					break;
				}
			}
			DBG_IND("p_path=%s\r\n", rep_path);
			DBG_IND("file_is_use=%d\r\n", p_dir_info->file_is_use);
		}
		//#NT#2020/04/28#Willy Su -end
	}

label_fm_replace_file_end:
	{
		//add to filedb
		#if 0
		if (iamoviemulti_fm_is_add_to_last(p_new_path[0])) {
			ret = FileDB_AddFileToLast(fdb_hdl, p_new_path);
		} else {
			ret = FileDB_AddFile(fdb_hdl, p_new_path);
		}
		#else
		if (iamoviemulti_fm_is_sync_time(p_new_path[0]))
		{
			ret = FileDB_AddFile(fdb_hdl, p_new_path);
		}
		else
		{
			if (iamoviemulti_fm_is_add_to_last(p_new_path[0])) {
				ret = FileDB_AddFileToLast(fdb_hdl, p_new_path);
			} else {
				ret = FileDB_AddFile(fdb_hdl, p_new_path);
			}
		}
		#endif
		if (FALSE == ret) {
			DBG_ERR("FileDB_AddFile %s failed\r\n", p_new_path);
			ret_val = -1;
		}
		DBG_DUMP("^G [replace] %s added to FileDB\r\n", p_new_path);
	}

label_fm_replace_file_end_without_addfile:
	//#NT#2021/03/03#Willy Su -begin
	//#NT# Add lock-unlock for replacement flow
	vos_sem_sig(IAMOVIE_SEMID_FM);
	//#NT#2021/03/03#Willy Su -end

	if (ret_val != 0) {
		return ret_val;
	}

	return 0;
}

INT32 iamoviemulti_fm_move_file(CHAR *p_old_path, CHAR *p_new_folder, CHAR *p_new_path)
{
	CHAR new_path[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	CHAR *p_name;
	FILEDB_HANDLE fdb_hdl;
	INT32 file_idx;

	if (NULL == p_old_path) {
		DBG_ERR("p_old_path is NULL\r\n");
		return -1;
	}

	if (NULL == p_new_folder) {
		DBG_ERR("p_new_folder is NULL\r\n");
		return -1;
	}

	p_name = strrchr(p_old_path, '\\');
	if (NULL == p_name) {
		DBG_ERR("Find name of %s failed\r\n", p_old_path);
		return -1;
	}
	p_name++;

	snprintf(new_path, sizeof(new_path), "%s%s", p_new_folder, p_name);

	if (FST_STA_OK != FileSys_MoveFile(p_old_path, new_path)) {
		DBG_ERR("Move %s to %s failed\r\n", p_old_path, new_path);
		return -1;
	}

	//remove old file from the old FileDB
	fdb_hdl = _iamovie_fm_get_fdb_hdl(p_old_path[0]);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		return -1;
	}

	file_idx = FileDB_GetIndexByPath(fdb_hdl, p_old_path);
	if (FILEDB_SEARCH_ERR == file_idx) {
		DBG_ERR("%s not found in FileDB\r\n", p_old_path);
		return -1;
	}

	if (TRUE != FileDB_DeleteFile(fdb_hdl, file_idx)) {
		DBG_ERR("FileDB_DeleteFile failed\r\n");
		return -1;
	}

	//add new file to the new FileDB
	fdb_hdl = _iamovie_fm_get_fdb_hdl(new_path[0]);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		return -1;
	}

	if (iamoviemulti_fm_is_add_to_last(new_path[0])) {
		if (FALSE == FileDB_AddFileToLast(fdb_hdl, new_path)) {
			DBG_ERR("FileDB_AddFile %s failed\r\n", new_path);
			return -1;
		}
	} else {
		if (FALSE == FileDB_AddFile(fdb_hdl, new_path)) {
			DBG_ERR("FileDB_AddFile %s failed\r\n", new_path);
			return -1;
		}
	}

	//store new path info
	strncpy(p_new_path, new_path, sizeof(new_path)-1);
	p_new_path[sizeof(new_path)-1] = '\0';

	return 0;
}

INT32 iamoviemulti_fm_flush_file(CHAR *p_path)
{
	FST_FILE filehdl = NULL;
	INT32 ret = 0;

	filehdl = FileSys_OpenFile(p_path, FST_OPEN_EXISTING | FST_OPEN_WRITE);
	if (NULL == filehdl) {
		DBG_ERR("OpenFile %s failed\r\n", p_path);
		return -1;
	}

	ret = FileSys_FlushFile(filehdl);
	if (FST_STA_OK != ret) {
		DBG_ERR("FlushFile %s failed\r\n", p_path);
	}
	FileSys_CloseFile(filehdl);

	return ret;
}

INT32 iamoviemulti_fm_set_ro_file(CHAR *p_path, BOOL is_ro)
{
	FILEDB_HANDLE fdb_hdl;
	_FDB_INFO *p_fdb_info;
	UINT64 file_size;
	INT32 file_idx;

	if (NULL == p_path) {
		DBG_ERR("p_path is NULL\r\n");
		return -1;
	}

	if (FST_STA_OK != FileSys_SetAttrib(p_path, FST_ATTRIB_READONLY, is_ro)) {
		DBG_ERR("Set %s attrib failed\r\n", p_path);
		return -1;
	}
	DBG_IND("Set %s attrib in FileSystem\r\n", p_path);

	fdb_hdl = _iamovie_fm_get_fdb_hdl(p_path[0]);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		return -1;
	}

	file_idx = FileDB_GetIndexByPath(fdb_hdl, p_path);
	if (-1 == file_idx) {
		DBG_ERR("get file_idx failed\r\n");
	}

	if (TRUE != FileDB_SetFileRO(fdb_hdl, file_idx, is_ro)) {
		DBG_ERR("FileDB_SetFileRO failed\r\n");
		return -1;
	}
	DBG_IND("Set %s attrib in FileDB\r\n", p_path);

	//update fdb rofile size
	file_size = FileSys_GetFileLen(p_path);
	p_fdb_info = _iamovie_fm_get_fdbinfo(p_path[0]);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", p_path[0]);
		return -1;
	}
	if (is_ro) {
		p_fdb_info->rofile_size += file_size;
		p_fdb_info->file_size -= file_size;
	} else {
		p_fdb_info->rofile_size -= file_size;
		p_fdb_info->file_size += file_size;
	}
	DBG_IND("^G set_ro_file rofile_size: %lld total_size: %lld\r\n",
		p_fdb_info->rofile_size, p_fdb_info->total_size);

	return 0;
}

INT32 iamoviemulti_fm_chk_ro_file(CHAR drive)
{
	FILEDB_HANDLE fdb_hdl;
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	INT32 rofile_num = 0, total_num;
	INT32 idx;
	INT32 rofile_limit;
	UINT64 rofile_size_cur;

	//Get FileDB handle
	fdb_hdl = _iamovie_fm_get_fdb_hdl(drive);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		return -1;
	}

	//Get FileDB RO info
	total_num = FileDB_GetTotalFileNum(fdb_hdl);
	if (0 == total_num) {
		DBG_IND("FileDB is empty\r\n");
		return 0;
	}

	for (idx = 0; idx < total_num; idx++) {
		p_file_attr = FileDB_SearhFile(fdb_hdl, idx);
		if (p_file_attr == NULL) {
			DBG_ERR("get file failed\r\n");
			return -1;
		}
		if (M_IsReadOnly(p_file_attr->attrib)) {
			//count RO file num
			rofile_num++;
		}
	}

	//Check RO Limit
	if (0 != iamoviemulti_fm_get_rofile_info(drive, &rofile_limit, IAMOVIEMULTI_FM_ROINFO_NUM)) {
		DBG_ERR("get ro file num failed\r\n");
		return -1;
	}
	DBG_IND("^G rofile_limit %d\r\n", rofile_limit);

	while (rofile_num > rofile_limit) {
		DBG_DUMP("^G Need to delete read-only file (%d > %d)\r\n", rofile_num, rofile_limit);
		if (0 != iamoviemulti_fm_del_ro_file(drive, &rofile_size_cur)) {
			DBG_ERR("iamoviemulti_fm_del_ro failed\r\n");
			return -1;
		}
		rofile_num--;
		total_num--;
	}
	DBG_IND("^G read-only: %d, total: %d\r\n", rofile_num, total_num);

	return 0;
}

INT32 iamoviemulti_fm_chk_ro_size(CHAR drive)
{
	FILEDB_HANDLE fdb_hdl;
	_FDB_INFO *p_fdb_info;
	UINT64 rofile_size = 0, file_size = 0, total_size = 0;
	INT32 rosize_limit;
	FLOAT rofile_size_p;
	UINT64 rofile_size_cur;
	UINT64 free_size;
	UINT64 disk_size;
	UINT64 remain_size;
	UINT32 count = 0;

	//Get FileDB handle
	fdb_hdl = _iamovie_fm_get_fdb_hdl(drive);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		return -1;
	}

	//Check RO Limit
	if (0 != iamoviemulti_fm_get_rofile_info(drive, &rosize_limit, IAMOVIEMULTI_FM_ROINFO_PCT)) {
		DBG_ERR("get ro file num failed\r\n");
		return -1;
	}
	DBG_IND("^G rosize_limit %d\r\n", rosize_limit);

	//Get FileDB RO info
	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return -1;
	}
	rofile_size = p_fdb_info->rofile_size;
	file_size = p_fdb_info->file_size;
	free_size = FileSys_GetDiskInfoEx(drive, FST_INFO_FREE_SPACE);
	if (free_size == 0) {
		DBG_ERR("free_size is 0\r\n");
		return -1;
	}
	remain_size = p_fdb_info->remain_size;
	total_size  = rofile_size + file_size + free_size + remain_size;
	disk_size = FileSys_GetDiskInfoEx(drive, FST_INFO_DISK_SIZE);
	if (disk_size == 0) {
		DBG_ERR("disk_size is 0\r\n");
		return -1;
	}
	if (total_size >= disk_size) {
		//option: refresh all
		FileDB_Refresh(fdb_hdl);
		if (0 != _iamovie_fm_update_fdbinfo(drive)) {
			DBG_ERR("update fdbinfo failed\r\n");
			return -1;
		}
		DBG_DUMP("^G FileDB Refresh.\r\n");

		rofile_size = p_fdb_info->rofile_size;
		file_size = p_fdb_info->file_size;
		total_size  = rofile_size + file_size + free_size + remain_size;
		if (total_size >= FileSys_GetDiskInfoEx(drive, FST_INFO_DISK_SIZE)) {
			DBG_ERR("filedb refresh error. ro %lld non-ro %lld free %lld remain %lld total %lld\r\n",
				rofile_size, file_size, free_size, remain_size, total_size);
			return -1;
		}
	}
	DBG_IND("^G ro file: %lld, remain_size: %lld, total size: %lld\r\n",
		p_fdb_info->rofile_size, remain_size, total_size);

	rofile_size_p = ((FLOAT)rofile_size / (FLOAT)total_size)*(100);

	while (rofile_size_p > (FLOAT)rosize_limit) {

		if (g_ro_pct_del_cnt) {
			if (count < g_ro_pct_del_cnt) {
				count ++;
			} else {
				break;
			}
		}

		DBG_DUMP("^G Need to delete read-only file (%.2f > %.2f)\r\n", rofile_size_p, (FLOAT)rosize_limit);
		if (-1 == iamoviemulti_fm_del_ro_file(drive, &rofile_size_cur)) {
			DBG_ERR("iamoviemulti_fm_del_ro failed\r\n");
			return -1;
		}
		rofile_size = p_fdb_info->rofile_size;
		file_size = p_fdb_info->file_size;
		free_size = FileSys_GetDiskInfoEx(drive, FST_INFO_FREE_SPACE);
		total_size  = rofile_size + file_size + free_size + remain_size;
		rofile_size_p = ((FLOAT)rofile_size / (FLOAT)total_size)*(100);
	}

	DBG_IND("^G Percent %.2f\r\n", rofile_size_p);

	return 0;
}

INT32 iamoviemulti_fm_del_ro_file(CHAR drive, UINT64 *size)
{
	FILEDB_HANDLE fdb_hdl;
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	CHAR file_path[IAMOVIE_FM_PATH_SIZE*4] = {0};
	UINT32 file_index = 0;
	UINT64 file_size;
	UINT8  file_attrib = 0;
	INT32 ret;

	DBG_IND("begin\r\n");

	_FDB_INFO *p_fdb_info = NULL;
	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return -1;
	}

	fdb_hdl = _iamovie_fm_get_fdb_hdl(drive);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		return -1;
	}

	p_file_attr = _iamovie_fm_find_ro(fdb_hdl, &file_index);
	if (p_file_attr == NULL) {
		DBG_ERR("get file failed\r\n");
		return -1;
	}
	DBG_IND("%s read-only file found from FileDB\r\n", p_file_attr->filePath);

	//copy file_path & file_size
	strncpy(file_path, p_file_attr->filePath, sizeof(file_path)-1);
	file_path[sizeof(file_path)-1] = '\0';
	file_size = p_file_attr->fileSize64;

	if (FST_STA_FILE_NOT_EXIST == FileSys_GetAttrib(file_path, &file_attrib)) {
		//option: refresh all
		FileDB_Refresh(fdb_hdl);
		if (0 != _iamovie_fm_update_fdbinfo(drive)) {
			DBG_ERR("update fdbinfo failed\r\n");
			return -1;
		}
		DBG_DUMP("^G FileDB Refresh.\r\n");
		return 0; //return for check
	}

	if (0 != iamoviemulti_fm_set_ro_file(file_path, FALSE)) {
		DBG_ERR("Set %s emr to FileDB failed\r\n", file_path);
		return -1;
	}
	DBG_IND("%s read-only attrib removed\r\n", file_path);

	if (0 != iamoviemulti_fm_flush_file(file_path)) {
		DBG_ERR("flush %s failed\r\n", file_path);
		return -1;
	}
	DBG_IND("%s flush done\r\n", file_path);

	ret = FileSys_DeleteFile(file_path);
	if (FST_STA_SDIO_ERR == ret) {
		//callback
		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(0, MOVIE_USER_CB_ERROR_CARD_WR_ERR, 0);
		}
	}
	if (FST_STA_OK != ret) {
		DBG_ERR("FileSys_DeleteFile %s failed\r\n", file_path);
		return -1;
	}
	DBG_DUMP("%s deleted from File System\r\n", file_path);

	if (TRUE != FileDB_DeleteFile(fdb_hdl, file_index)) {
		DBG_ERR("FileDB_DeleteFile failed\r\n");
		return -1;
	}
	//update fdb info
	p_fdb_info->file_size -= file_size;
	DBG_DUMP("%s deleted from FileDB\r\n", file_path);

	//callback
	if (g_ia_moviemulti_usercb){
		g_ia_moviemulti_usercb(0, MOVIE_USRR_CB_EVENT_FILE_DELETED, (UINT32)file_path);
	}

	DBG_IND("end\r\n");
	*size = file_size;
	return 0;
}

INT32 iamoviemulti_fm_set_remain_size(CHAR drive, UINT64 size)
{
	_FDB_INFO *p_fdb_info = NULL;

	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return -1;
	}

	p_fdb_info->remain_size = size;

	return 0;
}

INT32 iamoviemulti_fm_config(MOVIE_CFG_FDB_INFO *p_cfg)
{
	_FDB_INFO *p_fdb_info;
	UINT32 path_len;

	if (NULL == p_cfg) {
		DBG_ERR("p_cfg is NULL\r\n");
		return -1;
	}

	p_fdb_info = _iamovie_fm_get_fdbinfo(p_cfg->drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", p_cfg->drive);
		return -1;
	}

	if (FILEDB_CREATE_ERROR != p_fdb_info->fdb_hdl) {
		DBG_ERR("fdb_hdl is not released\r\n");
		return -1;
	}

	//store fdb info
	strncpy(p_fdb_info->root_path, p_cfg->p_root_path, sizeof(p_fdb_info->root_path)-1);
	p_fdb_info->root_path[sizeof(p_fdb_info->root_path)-1] = '\0';

	p_fdb_info->pool = p_cfg->mem_range;

	//check the end char should be '\\'
	path_len = strlen(p_fdb_info->root_path);
	if (p_fdb_info->root_path[path_len-1] != '\\') {
		DBG_ERR("root_path %s need a '\\' in the end\r\n", p_fdb_info->root_path);
		return -1;
	}

	return 0;
}

INT32 iamoviemulti_fm_dump(CHAR drive)
{
	_FDB_INFO *p_fdb_info = NULL;
	_FILEOUT_RO_INFO *p_ro_info = NULL;
	_DISK_INFO *p_disk_info = NULL;
	_DIR_INFO *p_dir_info = NULL;
	UINT64 resv_size = 0;
	UINT64 time_val = 0;
	INT32 i;

	if (_iamovie_fm_drive_is_invalid(drive)) {
		DBG_ERR("Invalid drive 0x%X\r\n", (UINT32)drive);
		return 0;
	}
	DBG_DUMP("[FM] Drive:%c\r\n", drive);

	// FM (g_fdb_info)
	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
	} else {
		DBG_DUMP("[FM] root_path:%s fdb_hdl:%d\r\n",
			p_fdb_info->root_path, p_fdb_info->fdb_hdl);
		DBG_DUMP("[FM] is_loop_del:%d is_add_to_last:%d\r\n",
			p_fdb_info->is_loop_del, p_fdb_info->is_add_to_last);
		if (0 == iamoviemulti_fm_get_time_val(drive, &time_val)) {
			DBG_DUMP("[FM] time_val:%d\r\n", (int)time_val); //CID 155562
		}
	}

	// SIZE (g_size_info)
	if (0 != iamoviemulti_fm_get_resv_size(drive, &resv_size)) {
		DBG_ERR("get resv size failed, drive %c\r\n", drive);
	} else {
		DBG_DUMP("[FM] resv_size:0x%llX\r\n", resv_size);
	}

	// RO (g_ro_file_info)
	p_ro_info = _iamovie_fm_get_roinfo(drive);
	if (NULL == p_ro_info) {
		DBG_ERR("get ro file info failed, drive %c\r\n", drive);
	} else {
		if (p_ro_info->type_chk == IAMOVIEMULTI_FM_CHK_NUM) {
			DBG_DUMP("[FM] check read-only file num:%d\r\n", p_ro_info->file_num);
		}
		else if (!iamoviemulti_fm_is_ro_chk_last(drive)) {
			DBG_DUMP("[FM] check read-only size pct:%d cnt:%d\r\n",
				p_ro_info->size_pct, g_ro_pct_del_cnt);
		} else {
			DBG_DUMP("[FM] check read-only last pct:%d cnt:%d\r\n",
				p_ro_info->size_pct, g_ro_pct_del_cnt);
		}
	}

	// format-free/dir-loop-del (g_disk_info)
	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
	} else {
		if (p_disk_info->is_format_free) {
			DBG_DUMP("[FM] Format-Free ON\r\n");
			for (i = 0; i < IAMOVIEMULTI_FM_DIR_NUM; i++) {
				p_dir_info = &(p_disk_info->dir_info[i]);
				if (NULL == p_dir_info) {
					DBG_ERR("get dir info failed, drive %c\r\n", drive);
				} else {
					if (!p_dir_info->is_use) {
						continue;
					}
					DBG_DUMP("[FM][%d] dir_path:%s\r\n",
						i, p_dir_info->dir_path);
					DBG_DUMP("[FM][%d] dir_ratio:%d\r\n",
						i, p_dir_info->dir_ratio);
					DBG_DUMP("[FM][%d] file_size:0x%llx\r\n",
						i, p_dir_info->file_size);
					if (p_dir_info->is_cyclic_rec) {
						DBG_DUMP("[FM][%d] file:%d cyclic_num:%d \r\n",
							i, p_dir_info->file_is_use, p_dir_info->file_num);
					} else {
						DBG_DUMP("[FM][%d] file:%d lock_num:%d\r\n",
							i, p_dir_info->file_is_use, p_dir_info->file_num);
					}
				}
			}
		} else if (p_disk_info->is_folder_loop) {
			DBG_DUMP("[FM] Folder-Loop usage_size:0x%llX\r\n", p_disk_info->usage_size);
			for (i = 0; i < IAMOVIEMULTI_FM_DIR_NUM; i++) {
				p_dir_info = &(p_disk_info->dir_info[i]);
				if (NULL == p_dir_info) {
					DBG_ERR("get dir info failed, drive %c\r\n", drive);
				} else {
					if (!p_dir_info->is_use) {
						continue;
					}
					DBG_DUMP("[FM][%d] dir_path:%s\r\n",
						i, p_dir_info->dir_path);
					DBG_DUMP("[FM][%d] dir_ratio:%d dir_size:0x%llX\r\n",
						i, p_dir_info->dir_ratio, p_dir_info->dir_size);
					DBG_DUMP("[FM][%d] file_size:0x%llx\r\n",
						i, p_dir_info->file_size);
				}
			}
		}
	}
	return 0;
}

INT32 iamoviemulti_fm_open(CHAR drive)
{
	FILEDB_INIT_OBJ FDBInitObj = {0};
	FILEDB_HANDLE fdb_hdl;
	_FDB_INFO *p_fdb_info;

	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return -1;
	}

	if (p_fdb_info->pool.size == 0) {
		DBG_ERR("ImageApp_MovieMulti_FMConfig() is not set!!\r\n");
		return -1;
	}

	FDBInitObj.rootPath                     = p_fdb_info->root_path;
	FDBInitObj.defaultfolder                = NULL;
	FDBInitObj.filterfolder                 = NULL;
	FDBInitObj.bIsRecursive                 = TRUE;
	FDBInitObj.bIsCyclic                    = TRUE;
	FDBInitObj.bIsSupportLongName           = TRUE;
	FDBInitObj.u32MaxFilePathLen            = 80;
	FDBInitObj.u32MaxFileNum                = g_ia_moviemulti_filedb_max_num;
	FDBInitObj.fileFilter                   = g_ia_moviemulti_filedb_filter;
	FDBInitObj.u32MemAddr                   = p_fdb_info->pool.addr;
	FDBInitObj.u32MemSize                   = p_fdb_info->pool.size;
	if (g_fdb_sn.bIsSortBySn){
		FDBInitObj.SortSN_Delim=g_fdb_sn.SortSN_Delim;
		FDBInitObj.SortSN_DelimCount=g_fdb_sn.SortSN_DelimCount;
		FDBInitObj.SortSN_CharNumOfSN=g_fdb_sn.SortSN_CharNumOfSN;
	}

	fdb_hdl = FileDB_Create(&FDBInitObj);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("FileDB create failed, drive %c\r\n", drive);
		return -1;
	}
	if (g_fdb_sn.bIsSortBySn)
		FileDB_SortBy(fdb_hdl, FILEDB_SORT_BY_SN, FALSE); // NA51000-412
	else
		FileDB_SortBy(fdb_hdl, FILEDB_SORT_BY_MODDATE, FALSE);    // NA51000-412

	iamoviemulti_fm_set_add_to_last(drive, TRUE);

	//FileDB_DumpInfo(fdb_hdl);

	p_fdb_info->fdb_hdl = fdb_hdl;

	if (0 != _iamovie_fm_update_fdbinfo(drive)) {
		DBG_ERR("update fdbinfo failed\r\n");
		return -1;
	}

	return 0;
}

INT32 iamoviemulti_fm_close(CHAR drive)
{
	_FDB_INFO *p_fdb_info;

	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return -1;
	}

	if (FILEDB_CREATE_ERROR != p_fdb_info->fdb_hdl) {
		FileDB_Release(p_fdb_info->fdb_hdl);

		memset(p_fdb_info, 0, sizeof(_FDB_INFO));
		p_fdb_info->fdb_hdl = FILEDB_CREATE_ERROR;
	}

	return 0;
}

INT32 iamoviemulti_fm_scan(CHAR drive)
{
	_SCAN_INFO* p_san_info = NULL;
	_DISK_INFO* p_disk_info = NULL;
	_DIR_INFO*  p_dir_info = NULL;
	CHAR   cur_dir[IAMOVIE_FM_PATH_SIZE] = {0};
	UINT32 cur_depth;
	BOOL   is_recursive = TRUE;
	UINT32 cb_arg = (UINT32)drive;
	UINT32  i;
	INT32  ret_fs;
	UINT64 usage_size = 0;

	DBG_IND("Scan-start\r\n");

	//p_san_info
	p_san_info = _iamovie_fm_get_scaninfo(drive);
	if (NULL == p_san_info) {
		DBG_ERR("get scan info failed, drive %c\r\n", drive);
		return -1;
	}
	//p_disk_info
	p_disk_info = _iamovie_fm_get_diskinfo_for_scan(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}

	//copy cur_dir
	snprintf(cur_dir, sizeof(cur_dir), "%c:\\", drive);
	p_san_info->cur_dir = cur_dir;
	cur_depth = _iamovie_fm_get_dir_depth(cur_dir);
	p_san_info->cur_depth = cur_depth;
	p_san_info->cur_idx = -1;
	DBG_IND("[scan]Scan Dir (%s) at (%d)\r\n", cur_dir, cur_depth);
	//scan dir
	ret_fs = FileSys_ScanDir(cur_dir, _iamovie_fm_dir_cb, is_recursive, cb_arg);
	if (ret_fs != E_OK) {
		DBG_ERR("ScanDir Error");
		return -1;
	}
	// only folder in root (default)
	p_san_info->root_depth = cur_depth +1;
	DBG_IND("[scan]Folder Count %d\r\n", p_san_info->dir_count);

	for (i = 0; i < p_san_info->dir_count; i++) {
		p_dir_info = &(p_disk_info->dir_info[i]);
		//copy cur_dir
		snprintf(cur_dir, sizeof(cur_dir), "%s", p_dir_info->dir_path);
		cur_depth = _iamovie_fm_get_dir_depth(cur_dir);
		p_san_info->cur_idx = i;
		p_san_info->cur_depth = cur_depth;
		DBG_IND("[scan]Scan Dir (%s) at (%d)\r\n", cur_dir, cur_depth);
		//scan dir
		ret_fs = FileSys_ScanDir(cur_dir, _iamovie_fm_dir_cb, is_recursive, cb_arg);
		if (ret_fs != E_OK) {
			DBG_ERR("ScanDir Error");
			//return -1;
		}
		usage_size += p_dir_info->dir_size;
	}

	//set usage size
	usage_size += FileSys_GetDiskInfoEx(drive, FST_INFO_FREE_SPACE);
	iamoviemulti_fm_set_usage_size(drive, usage_size);
	DBG_IND("^G Drive %c usage_size %lld\r\n", drive, usage_size);

	//clean
	memset(p_san_info, 0, sizeof(_SCAN_INFO));

	DBG_IND("Scan-end\r\n");
	return 0;
}

INT32 iamoviemulti_fm_merge(CHAR drive)
{
	_DISK_INFO* p_disk_info = NULL;
	_DIR_INFO *p_dir_info = NULL;
	_DIR_INFO *p_dir_info_parent = NULL;
	UINT32 dir_max_num = IAMOVIEMULTI_FM_DIR_NUM;
	INT32 parent_idx;
	UINT32 i;

	p_disk_info = _iamovie_fm_get_diskinfo_for_scan(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}

	//merge
	for (i = dir_max_num; i > 0; i--) {
		p_dir_info = &(p_disk_info->dir_info[i-1]);
		if (p_dir_info->is_use == 0) {
			continue;
		}
		parent_idx = p_dir_info->parent_idx;
		if (parent_idx < 0) {
			p_dir_info->is_merged = 0;
			continue;
		}
		p_dir_info_parent = &(p_disk_info->dir_info[parent_idx]);
		p_dir_info_parent->is_parent = 1;
		p_dir_info_parent->dir_size += p_dir_info->dir_size;
		p_dir_info_parent->file_num += p_dir_info->file_num;
		p_dir_info_parent->file_size = 0; //since parent
		p_dir_info_parent->file_is_use = 0; //since parent
		p_dir_info->is_merged = 1;
	}

	return 0;
}

INT32 iamoviemulti_fm_package(CHAR drive, MOVIEMULTI_DISK_INFO *p_disk_info)
{
	_DISK_INFO *p_disk_info_in = NULL;
	_DIR_INFO *p_dir_info = NULL;
	UINT64 disk_resv_size = IAMOVIEMULTI_FM_RESV_SIZE * 10; //100 MB
	UINT32 dir_max_num = IAMOVIEMULTI_FM_DIR_NUM;
	UINT32 dir_depth = 0;
	UINT32 cur_depth = 0;
	UINT64 total_dir_size = 0;
	UINT64 usage_size = 0;
	UINT32 i;

	p_disk_info_in = _iamovie_fm_get_diskinfo_for_scan(drive);
	if (NULL == p_disk_info_in) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}

	//get scan depth if set
	if (p_disk_info->dir_depth) {
		dir_depth = p_disk_info->dir_depth + 1; // for '\\'
		DBG_IND("[FM] scan depth: %d\r\n", dir_depth);
	}

	//package
	p_disk_info->dir_num = 0;
	p_disk_info->dir_depth = 0;
	p_disk_info->disk_size = FileSys_GetDiskInfoEx(drive, FST_INFO_DISK_SIZE);
	if (p_disk_info->disk_size == 0) {
		DBG_ERR("Get disk size error\r\n");
		return -1;
	}
	p_disk_info->free_size = FileSys_GetDiskInfoEx(drive, FST_INFO_FREE_SPACE);
	for (i = 0; i < dir_max_num; i++) {
		p_dir_info = &(p_disk_info_in->dir_info[i]);
		if (p_dir_info->is_use == 0) {
			continue;
		}
		cur_depth = _iamovie_fm_get_dir_depth(p_dir_info->dir_path);
		if (dir_depth) { //scan depth if set
			if (cur_depth > dir_depth) {
				continue;
			}
		}
		if (cur_depth > p_disk_info->dir_depth) {
			p_disk_info->dir_depth = cur_depth;
		}
		p_disk_info->dir_info[i].is_use = p_dir_info->is_use;
		snprintf(p_disk_info->dir_info[i].dir_path, sizeof(p_disk_info->dir_info[i].dir_path), "%s", p_dir_info->dir_path);
		p_disk_info->dir_info[i].dir_size = p_dir_info->dir_size;
		p_disk_info->dir_info[i].dir_ratio_of_disk = 100 * p_dir_info->dir_size / (p_disk_info->disk_size - disk_resv_size);
		if ((100 * p_dir_info->dir_size) % (p_disk_info->disk_size - disk_resv_size) != 0) {
			p_disk_info->dir_info[i].dir_ratio_of_disk += 1;
		}
		p_disk_info->dir_info[i].file_num = p_dir_info->file_num;
		if (p_dir_info->file_num != 0) {
			p_disk_info->dir_info[i].file_avg_size = p_dir_info->dir_size / p_dir_info->file_num;
		} else {
			p_disk_info->dir_info[i].file_avg_size = 0;
		}
		if (p_dir_info->file_is_use && (FM_INVALID_IDX != p_dir_info->file_is_use)) {
			p_disk_info->dir_info[i].falloc_size = p_dir_info->file_size;
		} else {
			p_disk_info->dir_info[i].falloc_size = 0;
		}
		if (FM_INVALID_IDX != p_dir_info->file_is_use) {
			p_disk_info->dir_info[i].is_falloc = p_dir_info->file_is_use;
		}
		if (p_dir_info->is_merged == 0) {
			total_dir_size += p_dir_info->dir_size;
		}
		p_disk_info->dir_num++;
		//chk format free state
		if (p_dir_info->dir_size == 0) {
			continue;
		}
		if (p_dir_info->is_parent) {
			continue;
		}
		if (p_dir_info->file_is_use && (FM_INVALID_IDX != p_dir_info->file_is_use)) {
			if (p_disk_info_in->is_format_free != 2) {
				p_disk_info_in->is_format_free = 1; //format-free
			}
		} else {
			p_disk_info_in->is_format_free = 2; //non-format-free
		}
		DBG_IND("Cal %s\r\n", p_dir_info->dir_path);
	}

	iamoviemulti_fm_get_usage_size(drive, &usage_size);
	p_disk_info->other_size = p_disk_info->disk_size - usage_size;
	p_disk_info->is_format_free = p_disk_info_in->is_format_free;

	//clean
	memset(p_disk_info_in, 0, sizeof(_DISK_INFO));

	return 0;
}

INT32 iamoviemulti_fm_allocate(CHAR drive)
{
	FST_FILE         filehdl = NULL;
	_DISK_INFO * p_disk_info = NULL;
	_DIR_INFO *   p_dir_info = NULL;
	UINT64 total_size = 0;
	CHAR file_path[IAMOVIE_FM_PATH_SIZE*4] = {0};
	UINT64 file_size;
	UINT32 file_num;
	UINT32 file_unit;
	UINT32 ratio = 0;
	UINT32 i, j;
	INT32 ret_fs;
	//#NT#2020/07/01#Willy Su -begin
	//#NT# Support set ext str with type setting
	CHAR ext[4] = {0};
	iamoviemulti_fm_get_extstr(drive, ext);
	//#NT#2020/07/01#Willy Su -end

	//time
	//UINT32 time0=0, time1=0, time2=0, time3=0, time4=0, time5=0, time6=0, time7=0;

	if (!iamoviemulti_fm_is_format_free(drive)) {
		DBG_DUMP("^G[FM] Format-Free No Use\r\n");
		return 0;
	}

	//p_disk_info
	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}

	//check
	#if 0
	if (TRUE != p_disk_info->need_format) {
		DBG_DUMP("^G [FM] No Need Format\r\n");
		return 0;
	}
	#endif

	//check ratio
	for (i = 0; i < IAMOVIEMULTI_FM_DIR_NUM; i++) {
		p_dir_info = &(p_disk_info->dir_info[i]);
		if (p_dir_info->is_use) {
			ratio += p_dir_info->dir_ratio;
		}
		if (ratio > 100) {
			DBG_ERR("dir ratio error\r\n");
			return -1;
		}
	}

	//format
	//time0 = clock();
	#if 0
	{
		DX_HANDLE   pStrgDXH = 0;
		ret_fs = FileSys_GetStrgObj(&pStrgDXH);
		ret_fs = FileSys_FormatDisk(pStrgDXH, FALSE);
		//reset filedb ?
	}
	#endif
	//time1 = clock();

	//total space (resv 100MB)
	total_size = FileSys_GetDiskInfoEx(drive, FST_INFO_FREE_SPACE) - IAMOVIEMULTI_FM_RESV_SIZE*10;

	for (i = 0; i < IAMOVIEMULTI_FM_DIR_NUM; i++) {

		//p_dir_info
		p_dir_info = &(p_disk_info->dir_info[i]);
		if (NULL == p_dir_info) {
			DBG_ERR("get dir info failed, drive %c\r\n", drive);
			return -1;
		}

		if (!p_dir_info->is_use) {
			continue;
		}

		file_size = p_dir_info->file_size;
		if (file_size == 0) {
			continue;
		}

		file_unit = p_dir_info->file_unit;
		if (file_unit) {
			file_num = (((total_size / 100) * p_dir_info->dir_ratio) / (file_size * file_unit)) * file_unit;
		} else {
			file_num = ((total_size / 100) * p_dir_info->dir_ratio) / file_size;
		}
		DBG_IND("^G file_num = %d\r\n", file_num);

		//allocate
		for (j = 0; j < file_num; j++) {

			//#NT#2020/07/01#Willy Su -begin
			//#NT# Support set ext str with type setting
			#if 1
			snprintf(file_path, sizeof(file_path), "%s00%04d.%s", p_dir_info->dir_path, (int)j, ext);
			#else
			snprintf(file_path, sizeof(file_path), "%s00%04d.MP4", p_dir_info->dir_path, j);
			#endif
			//#NT#2020/07/01#Willy Su -end

			{
				if (NULL != filehdl) {
					DBG_ERR("filehdl is not NULL\r\n");
					return -1;
				}
				//time3 = clock();
				filehdl = FileSys_OpenFile(file_path, FST_CREATE_HIDDEN|FST_OPEN_WRITE);
				//time4 = clock();
				if (filehdl == NULL) {
					DBG_ERR("allocate(O) failed\r\n");
					if (g_ia_moviemulti_usercb) {
						g_ia_moviemulti_usercb(0, MOVIE_USER_CB_ERROR_CARD_WR_ERR, 0);
					}
					return -1;
				}
				ret_fs = FileSys_AllocFile(filehdl, file_size, 0, 0);
				//time5 = clock();
				if (ret_fs != 0) {
					DBG_ERR("allocate(A) failed\r\n");
					if (g_ia_moviemulti_usercb) {
						g_ia_moviemulti_usercb(0, MOVIE_USER_CB_ERROR_CARD_WR_ERR, 0);
					}
					return -1;
				}
				ret_fs = FileSys_CloseFile(filehdl);
				//time6 = clock();
				if (ret_fs != 0) {
					DBG_ERR("allocate(C) failed\r\n");
					if (g_ia_moviemulti_usercb) {
						g_ia_moviemulti_usercb(0, MOVIE_USER_CB_ERROR_CARD_WR_ERR, 0);
					}
					return -1;
				}
				//ret_fs = FileSys_SetAttrib(file_path, FST_ATTRIB_HIDDEN, TRUE);
				//time7 = clock();
				filehdl = NULL;
			}

			//DBG_IND("O %d us / A %d us / C %d us / S %d us / Total %d us\r\n",
			//	abs(time4-time3), abs(time5-time4), abs(time6-time5), abs(time7-time6), abs(time7-time3));
			//DBG_IND("%s Total %d us\r\n", file_path, abs(time7-time3));
		}

		DBG_IND("%s : %d %lld %d\r\n",
			p_dir_info->dir_path,
			p_dir_info->dir_ratio,
			(file_size/(1024*1024)),
			file_num);

		p_dir_info->file_num=file_num;
		//#NT#2020/04/20#Willy Su -begin
		//#NT# Reset here since fmallocate
		p_dir_info->file_is_use = 0;
		//#NT#2020/04/20#Willy Su -end
	}
	//time2 = clock();

	//reset
	#if 0
	if (0 != iamoviemulti_fm_set_format_free(drive, FALSE)) {
		return -1;
	}
	#endif
	#if 0
	p_disk_info->need_format = FALSE;
	#endif

	//DBG_IND("^G Format %d us\r\n", abs(time2-time0));

	return 0;
}

INT32 iamoviemulti_fm_set_cyclic_rec(CHAR *p_dir, BOOL is_on)
{
	_DISK_INFO *p_disk_info = NULL;
	_DIR_INFO *   p_dir_info = NULL;
	CHAR drive;
	INT32 i = 0;
	if(p_dir){
		drive = p_dir[0];
	}else{
		DBG_ERR("di path is null!\r\n");
		return -1;
	}
	DBG_IND("p_dir=%s, is_on=%d\r\n",p_dir,is_on);

	//p_disk_info
	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}
	//check
	if (!p_disk_info->is_format_free) {
		DBG_DUMP("^G[FM] Format-Free No Use\r\n");
		return 0;
	}

	DBG_IND("p_dir=%s\r\n",p_dir);

	for (i = 0; i < IAMOVIEMULTI_FM_DIR_NUM; i++) {
		//p_dir_info
		p_dir_info = &(p_disk_info->dir_info[i]);
		DBG_IND("p_dir_info.dir_path=%s\r\n",p_dir_info->dir_path);
		if(strcmp(p_dir_info->dir_path, p_dir)==0){
			//#NT#2020/04/06#Willy Su -begin
			//#NT# Not reset here since files might already exist in Card Space
			#if 0
			if(is_on){
				p_dir_info->file_is_use=0;
			}
			#endif
			//#NT#2020/05/27#Willy Su -end
			p_dir_info->is_cyclic_rec=is_on;
			break;
		}
		if (i == (IAMOVIEMULTI_FM_DIR_NUM-1)) {
			DBG_ERR("no dir in disk, %s\r\n", p_dir);
			return -1;
		}
	}


	return 0;
}

INT32 iamoviemulti_fm_chk_format_free_dir_full(CHAR *p_path)
{
	FILEDB_HANDLE fdb_hdl;
	_DISK_INFO *p_disk_info = NULL;
	_DIR_INFO *   p_dir_info = NULL;
	CHAR parent_dir[IAMOVIE_FM_PATH_SIZE] = {0};
	CHAR drive;
	UINT32 total_num;
	INT32 i = 0;
	if(p_path){
		drive = p_path[0];
	}else{
		DBG_ERR("dir path is null!\r\n");
		return -1;
	}
	DBG_IND("drive=%c, p_path=%s\r\n",drive, p_path);

	fdb_hdl = _iamovie_fm_get_fdb_hdl(drive);
	if (FILEDB_CREATE_ERROR == fdb_hdl) {
		DBG_ERR("get fdb_hdl failed\r\n");
		return -1;
	}

	total_num = FileDB_GetTotalFileNum(fdb_hdl);
	if (0 == total_num) {
		DBG_WRN("[chk] FileDB is empty. Refresh.\r\n");
		FileDB_Refresh(fdb_hdl);
		if (0 == FileDB_GetTotalFileNum(fdb_hdl)) {
			DBG_WRN("[chk] FileDB is empty\r\n");
			return 1;
		}
	} else {
		DBG_DUMP("[chk] total_num=%d\r\n", total_num);
	}

	//p_disk_info
	p_disk_info = _iamovie_fm_get_diskinfo(drive);
	if (NULL == p_disk_info) {
		DBG_ERR("get disk info failed, drive %c\r\n", drive);
		return -1;
	}
	//check
	if (!p_disk_info->is_format_free) {
		DBG_DUMP("^G[FM] Format-Free No Use\r\n");
		return 0;
	}
	//get dir name
	if (FST_STA_OK != FileSys_GetParentDir(p_path, parent_dir)) {
		DBG_ERR("get dir error. file %s\r\n", p_path);
		return -1;
	}

	for (i = 0; i < IAMOVIEMULTI_FM_DIR_NUM; i++) {
		//p_dir_info
		p_dir_info = &(p_disk_info->dir_info[i]);
		DBG_IND("dir_path=%s\r\n",p_dir_info->dir_path);
		if(strcmp(p_dir_info->dir_path, parent_dir)==0){
			DBG_IND("file_num=%d, file_is_use=%d\r\n",p_dir_info->file_num,p_dir_info->file_is_use);
			if(p_dir_info->is_cyclic_rec==0  && p_dir_info->file_num == p_dir_info->file_is_use){
				DBG_IND("dir is full, %s\r\n",parent_dir);
				return 1;
			}
			//find file from filedb
			if (iamoviemulti_fm_is_skip_read_only(drive))
			{
				FILEDB_FILE_ATTR *p_file_attr = NULL;
				UINT32 file_index = 0;
				p_file_attr = _iamovie_fm_find_file_by_dir(fdb_hdl, &file_index, parent_dir);
				if (p_file_attr == NULL) {
					DBG_IND("find file failed. dir is full, %s\r\n",parent_dir);
					return 1;
				}
			}
			break;
		}
		if (i == (IAMOVIEMULTI_FM_DIR_NUM-1)) {
			DBG_ERR("no dir in disk, %s\r\n", parent_dir);
			return -1;
		}
	}
	return 0;
}

INT32 iamoviemulti_fm_refresh(CHAR drive)
{
	_FDB_INFO *p_fdb_info;

	p_fdb_info = _iamovie_fm_get_fdbinfo(drive);
	if (NULL == p_fdb_info) {
		DBG_ERR("get fdb info failed, drive %c\r\n", drive);
		return -1;
	}

	if (FILEDB_CREATE_ERROR != p_fdb_info->fdb_hdl) {
		FileDB_Refresh(p_fdb_info->fdb_hdl);
		if (0 != _iamovie_fm_update_fdbinfo(drive)) {
			DBG_ERR("update fdbinfo failed,  drive %c\r\n", drive);
			return -1;
		}
	}

	return 0;
}

INT32 ImageApp_MovieMulti_FMConfig(MOVIE_CFG_FDB_INFO *p_cfg)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (0 != iamoviemulti_fm_config(p_cfg)) {
			return -1;
		}
		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

void ImageApp_MovieMulti_FMSetSortBySN(CHAR *pDelimStr, UINT32 nDelimCount, UINT32 nNumOfSn)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
	    if (pDelimStr)
    	{
        	g_fdb_sn.SortSN_Delim       = pDelimStr;
	        g_fdb_sn.SortSN_DelimCount  = nDelimCount;
    	    g_fdb_sn.SortSN_CharNumOfSN = nNumOfSn;

        	g_fdb_sn.bIsSortBySn= TRUE;
	    }
    	else
        	DBG_ERR("No set Delim String!!\r\n");
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
	}
}

INT32 ImageApp_MovieMulti_FMDump(CHAR drive)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (0 != iamoviemulti_fm_dump(drive)) {
			return -1;
		}
		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

INT32 ImageApp_MovieMulti_FMOpen(CHAR drive)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (0 != iamoviemulti_fm_open(drive)) {
			return -1;
		}
		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

INT32 ImageApp_MovieMulti_FMClose(CHAR drive)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (0 != iamoviemulti_fm_close(drive)) {
			return -1;
		}
		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

INT32 ImageApp_MovieMulti_FMRefresh(CHAR drive)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		if (0 != iamoviemulti_fm_refresh(drive)) {
			return -1;
		}
		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

INT32 ImageApp_MovieMulti_FMAllocate(CHAR drive)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		FILEDB_HANDLE fdb_hdl;

		if (!iamoviemulti_fm_is_format_free(drive)) {
			DBG_DUMP("Format-Free OFF\r\n");
			return 0;
		}

		if (0 != iamoviemulti_fm_allocate(drive)) {
			DBG_ERR("[FM] Allocate Failed\r\n");
			return -1;
		}
		DBG_DUMP("Format-Free ON\r\n");

		//Get FileDB handle
		fdb_hdl = _iamovie_fm_get_fdb_hdl(drive);
		if (FILEDB_CREATE_ERROR == fdb_hdl) {
			DBG_ERR("get fdb_hdl failed\r\n");
		} else {
			//option: refresh all
			FileDB_Refresh(fdb_hdl);
			//FileDB_DumpInfo(fdb_hdl);
		}

		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

INT32 ImageApp_MovieMulti_FMScan(CHAR drive, MOVIEMULTI_DISK_INFO *p_disk_info)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		//chk dirve
		if (_iamovie_fm_drive_is_invalid(drive)) {
			DBG_ERR("Invalid drive 0x%X\r\n", (UINT32)drive);
			return -1;
		}

		//do scan
		if (0 != iamoviemulti_fm_scan(drive)) {
			DBG_ERR("[FM] Scan Failed\r\n");
			return -1;
		}

		//do merge
		if (0 != iamoviemulti_fm_merge(drive)) {
			DBG_ERR("[FM] Merge Failed\r\n");
			return -1;
		}

		//do package
		if (0 != iamoviemulti_fm_package(drive, p_disk_info)) {
			DBG_ERR("[FM] Package Failed\r\n");
			return -1;
		}

		return 0;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return -1;
	}
}

