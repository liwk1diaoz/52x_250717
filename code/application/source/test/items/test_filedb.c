#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <kwrap/examsys.h>
#include <kwrap/error_no.h>
#include <kwrap/perf.h>
#include <kwrap/task.h>
#include <kwrap/type.h>
#include <kwrap/util.h>
#if defined(__FREERTOS)
#include "trng.h"
#endif
#include "FileDB.h"
#include <hdal.h>

///////////////////////////////////////////////////////////////////////////////
#if defined(__FREERTOS)
#define RANDOM_RANGE(n)             ((randomUINT32() % (n)))
#else
#define RANDOM_RANGE(n)             (rand() % (n))
#endif

#define FDB_TEMP_BUF_SIZE           0x1000000
#define FDB_SORTSN_DELIM            "_"
#define FDB_SROTSN_DELIM_COUNT      2
#define FDB_SROTSN_CHAR_NUM_OF_SN   6
#define FDB_MIN_TYPE                FILEDB_SORT_BY_CREDATE
#define FDB_MAX_TYPE                FILEDB_SORT_BY_SN

static UINT32 temp_pa;
static UINT32 temp_va;
static UINT32 temp_result[5] = {0};

static UINT32 SxCmd_GetTempMem(UINT32 size)
{
	CHAR                 name[]= "FsTmp";
	void                 *va;
	HD_RESULT            ret;
    HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;

    if(temp_pa) {
    	printf("need release buf\r\n");
        return 0;
    }
	ret = hd_common_mem_alloc(name, &temp_pa, (void **)&va, size, ddr_id);
	if (ret != HD_OK) {
		printf("err:alloc size 0x%x, ddr %d\r\n", (unsigned int)(size), (int)(ddr_id));
		return 0;
	}

	printf("temp_pa = 0x%x, va = 0x%x\r\n", (unsigned int)(temp_pa), (unsigned int)(va));

	temp_va = (UINT32)va;

	return (UINT32)va;
}

static UINT32 SxCmd_RelTempMem(UINT32 addr)
{
	HD_RESULT            ret;

	printf("temp_pa = 0x%x, va = 0x%x\r\n", (unsigned int)(temp_pa), (unsigned int)(addr));

    if(!temp_pa||!addr){
        printf("not alloc\r\n");
        return HD_ERR_UNINIT;
    }
	ret = hd_common_mem_free(temp_pa, (void *)addr);
	if (ret != HD_OK) {
		printf("err:free temp_pa = 0x%x, va = 0x%x\r\n", (unsigned int)(addr), (unsigned int)(addr));
		return HD_ERR_NG;
	}else {
        temp_pa =0;
	}
    return HD_OK;
}

typedef struct {
	int     type_val;
	char    ext_str[4];
	int     file_count;
} EXAM_FILEDB_TYPE;

typedef struct {
	int     index;
	char    file_path[80];
	int     count;
} EXAM_FILEDB_PATH;

///////////////////////////////////////////////////////////////////////////////

static char g_fdb_test_dir[] = "A:\\FDB\\";

static EXAM_FILEDB_PATH g_fdb_file_tbl[] = {
	{0,  "A:\\FDB\\MOVIE\\2015_0101_000000_001.MP4", 0},
	{1,  "A:\\FDB\\MOVIE\\2015_0101_000000_002.MP4", 0},
	{2,  "A:\\FDB\\MOVIE\\2015_0101_000000_003.MP4", 0},
	{3,  "A:\\FDB\\PHOTO\\2015_0101_000000_001.JPG", 0},
	{4,  "A:\\FDB\\PHOTO\\2015_0101_000000_002.JPG", 0},
	{5,  "A:\\FDB\\PHOTO\\2015_0101_000000_003.JPG", 0},
	{6,  "A:\\FDB\\TEMP\\MYDIR\\2015_0101_000000_001.RAW", 0},
	{7,  "A:\\FDB\\TEMP\\MYDIR\\2015_0101_000000_002.RAW", 0},
	{8,  "A:\\FDB\\TEMP\\MYDIR\\2015_0101_000000_003.RAW", 0},
	{9,  "A:\\FDB\\TEMP\\MYLONGFILENAMEDIR\\2015_0101_000000_001.MOV", 0},
	{10, "A:\\FDB\\TEMP\\MYLONGFILENAMEDIR\\2015_0101_000000_002.MOV", 0},
	{11, "A:\\FDB\\TEMP\\MYLONGFILENAMEDIR\\2015_0101_000000_003.MOV", 0},
};

static EXAM_FILEDB_PATH g_fdb_sort_tbl[] = {
	{0,  "A:\\FDB\\AAAAAAA\\2015_0101_000003_111.MP4", 0},         //index 0
	{1,  "A:\\FDB\\AAAAAAA\\2015_0101_000006_22.MPG", 0},          //index 1
	{2,  "A:\\FDB\\AAAAAAA\\2015_0101_000010_3.RAW", 0},           //index 2
	{3,  "A:\\FDB\\CCCC\\2015_0101_000001_111.ASF", 0},            //index 3
	{4,  "A:\\FDB\\CCCC\\2015_0101_000007_22.WAV", 0},             //index 4
	{5,  "A:\\FDB\\CCCC\\2015_0101_000009_3.MOV", 0},              //index 5
	{6,  "A:\\FDB\\BBBBBBBBBB\\2015_0101_000008_3.JPG", 0},        //index 6
	{7,  "A:\\FDB\\BBBBBBBBBB\\2015_0101_000005_22.MP3", 0},       //index 7
	{8,  "A:\\FDB\\BBBBBBBBBB\\2015_0101_000002_111.AVI", 0},      //index 8
	{9,  "A:\\FDB\\DDDDDDDDDDDDD\\2015_0101_000004_111.TS", 0},    //index 9
	{10, "A:\\FDB\\DDDDDDDDDDDDD\\2015_0101_000011_222.AAC", 0},   //index 10
};

static EXAM_FILEDB_TYPE g_fdb_type_tbl[] = {
	{FILEDB_FMT_JPG, "JPG", 0},
	{FILEDB_FMT_MOV, "MOV", 0},
	{FILEDB_FMT_MP4, "MP4", 0},
	{FILEDB_FMT_MPG, "MPG", 0},
	{FILEDB_FMT_AVI, "AVI", 0},
	{FILEDB_FMT_RAW, "RAW", 0},
	{FILEDB_FMT_MP3, "MP3", 0},
	{FILEDB_FMT_ASF, "ASF", 0},
	{FILEDB_FMT_WAV, "WAV", 0},
	{FILEDB_FMT_BMP, "BMP", 0},
	{FILEDB_FMT_TS, "TS", 0},
	{FILEDB_FMT_AAC, "AAC", 0},
};

static int g_fdb_num_tbl[] = {100, 200, 1000};

#define NUMTBL_N_TEST   (sizeof(g_fdb_num_tbl)/sizeof(int))
#define TYPETBL_N_TEST  (sizeof(g_fdb_type_tbl)/sizeof(EXAM_FILEDB_TYPE))
#define FILETBL_N_TEST  12//(sizeof(g_fdb_file_tbl)/sizeof(EXAM_FILEDB_PATH))
#define FILETBL_N_SORT  11//(sizeof(g_fdb_sort_tbl)/sizeof(EXAM_FILEDB_PATH))

//this table records the index of g_FileTbl sorted by different sort types
static int g_order_tbl[FDB_MAX_TYPE + 1][11] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //None (No Use)
	{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, //CreDate
	{10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0}, //ModDate
	{3, 8, 0, 9, 7, 1, 4, 6, 5, 2, 10}, //Name
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //StrokeNum (No Use)
	{0, 1, 2, 8, 7, 6, 3, 4, 5, 9, 10}, //FilePath
	{5, 4, 3, 2, 1, 0, 6, 7, 8, 9, 10}, //Size
	{6, 7, 8, 3, 4, 5, 0, 1, 2, 9, 10}, //FileType
	{6, 7, 8, 3, 4, 5, 0, 1, 2, 9, 10}, //TypeCretimeSize
	{3, 8, 0, 9, 7, 1, 4, 6, 5, 2, 10}, //CretimeName
	{3, 8, 0, 9, 7, 1, 4, 6, 5, 2, 10}, //Serial Number
};

///////////////////////////////////////////////////////////////////////////////

//create a file with the content of the file full path
static int fdb_create_file(char *path)
{
	FST_FILE    filehdl = NULL;
	UINT32      size;

	if (NULL == path) {
		printf("path is NULL.\n");
		return -1;
	}
	size = (UINT32)strlen(path);

	filehdl = FileSys_OpenFile(path, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
	if (NULL == filehdl) {
		printf("Create %s fail (open).\n", path);
		return -1;
	}

	if (FST_STA_OK != FileSys_WriteFile(filehdl, (UINT8 *)path, &size, 0, NULL)) {
		printf("Create %s fail (write).\n", path);
	}

	if (FST_STA_OK != FileSys_CloseFile(filehdl)) {
		printf("Create %s fail (close).\n", path);
		return -1;
	}

	return 0;
}

//delete a directory with the content of the full path
static int fdb_delete_dir(char *path)
{
	FST_FILE    filehdl = NULL;

	filehdl = FileSys_OpenFile(path, FST_OPEN_READ);
	if (NULL == filehdl) {
		return 0;  //not exist
	}

	if (FST_STA_OK != FileSys_CloseFile(filehdl)) {
		printf("Delete %s fail (close).\n", path);
		return -1;
	}
	if (FST_STA_OK != FileSys_DeleteDir(path)) {
		printf("Delete %s fail (delete).\n", path);
		return -1;
	}

	return 0;
}

static int fdb_check_sort(FILEDB_HANDLE filedb_hdl, int sort_type)
{
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	int idx_tbl, idx_fdb;
	int num_fdb = 11;//(int)FILETBL_N_SORT;

	//check the index result
	for (idx_fdb = 0; idx_fdb < num_fdb; idx_fdb++) {
		p_file_attr = FileDB_SearhFile(filedb_hdl, (INT32)idx_fdb);
		if (NULL == p_file_attr) {
			printf("FileDB_SearhFile (%d) failed.\n", idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			return -1;
		}

		idx_tbl = g_order_tbl[sort_type][idx_fdb];

		if (0 != strcmp(g_fdb_sort_tbl[idx_tbl].file_path, p_file_attr->filePath)) {
			printf("idx_fdb(%d) should be %s, but got %s.\n",
					idx_fdb, g_fdb_sort_tbl[idx_tbl].file_path, p_file_attr->filePath);
			FileDB_DumpInfo(filedb_hdl);
			return -1;
		}
	}

	return 0;
}

//test delete files from filedb and add files to filedb
static int fdb_delete_add(FILEDB_HANDLE filedb_hdl)
{
	FILEDB_FILE_ATTR *p_file_attr;
	static CHAR tmp_path[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	int idx_fdb;
	int num_fdb = 11;//(int)FILETBL_N_SORT;
	int count;

	for (count = 0; count < 10; count++) {
		idx_fdb = RANDOM_RANGE(num_fdb);
		p_file_attr = FileDB_SearhFile(filedb_hdl, (INT32)idx_fdb);
		if (NULL == p_file_attr) {
			printf("FileDB_SearhFile (%d) failed.\n", idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			return -1;
		}

		//backup the file to be deleted
		strncpy(tmp_path, p_file_attr->filePath, KFS_LONGNAME_PATH_MAX_LENG - 1);
		tmp_path[KFS_LONGNAME_PATH_MAX_LENG - 1] = '\0';
		//printf("idx_fdb = %d, szTmpPath = %s.\n", idx_fdb, tmp_path);

		if (FALSE == FileDB_DeleteFile(filedb_hdl, (UINT32)idx_fdb)) {
			printf("FileDB_DeleteFile (%d) failed.\n", idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			return -1;
		}

		if (FALSE == FileDB_AddFile(filedb_hdl, tmp_path)) {
			printf("FileDB_AddFile (%d) failed.\n", idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			return -1;
		}
	}

	return 0;
}

//test delete files from filedb and add files to filedb's last position
static int fdb_delete_add_to_last(FILEDB_HANDLE filedb_hdl)
{
	FILEDB_FILE_ATTR *p_file_attr_1;
	FILEDB_FILE_ATTR *p_file_attr_2;
	static CHAR tmp_path[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	int idx_fdb;
	int num_fdb = (int)FILETBL_N_SORT;
	int count;
	int file_num;
	int file_index;

	file_num = (int)FileDB_GetTotalFileNum(filedb_hdl);
	file_index = file_num - 1; //0 as start

	for (count = 0; count < 10; count++) {
		idx_fdb = RANDOM_RANGE(num_fdb);
		p_file_attr_1 = FileDB_SearhFile(filedb_hdl, (INT32)idx_fdb);
		if (NULL == p_file_attr_1) {
			printf("FileDB_SearhFile (%d) failed.\n", idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			return -1;
		}

		//backup the file to be deleted
		strncpy(tmp_path, p_file_attr_1->filePath, KFS_LONGNAME_PATH_MAX_LENG - 1);
		tmp_path[KFS_LONGNAME_PATH_MAX_LENG - 1] = '\0';
		//printf("idx_fdb = %d, szTmpPath = %s.\n", idx_fdb, tmp_path);

		if (FALSE == FileDB_DeleteFile(filedb_hdl, (UINT32)idx_fdb)) {
			printf("FileDB_DeleteFile (%d) failed.\n", idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			return -1;
		}

		if (FALSE == FileDB_AddFileToLast(filedb_hdl, tmp_path)) {
			printf("FileDB_AddFileToLast (%d) failed.\n", idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			return -1;
		}

		p_file_attr_2 = FileDB_SearhFile(filedb_hdl, (INT32)file_index);
		if (NULL == p_file_attr_2) {
			printf("FileDB_SearhFile (%d) failed.\n", file_index);
			FileDB_DumpInfo(filedb_hdl);
			return -1;
		}

		if (0 != strcmp(tmp_path, p_file_attr_2->filePath)) {
			printf("idx_fdb(%d) should be %s, but got %s.\n",
				file_index, tmp_path, p_file_attr_2->filePath);
			FileDB_DumpInfo(filedb_hdl);
			return -1;
		}
	}

	return 0;
}

static int fdb_mem_init(void)
{
	int buf;

	buf = (int)SxCmd_GetTempMem(FDB_TEMP_BUF_SIZE);
	if (0 == buf) {
		printf("SxCmd_GetTempMem failed.\n");
		return -1;
	}

	return 0;
}

static int fdb_mem_uninit(void)
{
	int buf;

	buf = (int)temp_va;
	if (0 == buf) {
		printf("SxCmd_GetTempMem failed.\n");
		return -1;
	}

	SxCmd_RelTempMem(buf);

	return 0;
}

static int fdb_set_init_obj(FILEDB_INIT_OBJ *obj, char *root_path)
{
	int buf;
	int buf_size;

	buf = (int)temp_va;
	if (0 == buf) {
		printf("get temp_va failed.\n");
		return -1;
	}

	obj->rootPath = root_path;
	obj->defaultfolder = NULL;
	obj->filterfolder = NULL;
	obj->bIsRecursive = TRUE;
	obj->bIsCyclic = TRUE;
	obj->bIsMoveToLastFile = TRUE;
	obj->bIsSupportLongName = TRUE;
	obj->bIsDCFFileOnly = FALSE;
	obj->bIsFilterOutSameDCFNumFolder = TRUE;
	memcpy(obj->OurDCFDirName, "NVTIM", 5);
	memcpy(obj->OurDCFFileName, "IMAG", 4);
	obj->bIsFilterOutSameDCFNumFile = FALSE;
	obj->bIsChkHasFile = FALSE;
	obj->u32MaxFilePathLen = 80;
	obj->u32MaxFileNum = 10000;
	obj->fileFilter = FILEDB_FMT_ANY;
	obj->u32MemAddr = (UINT32)buf;
	obj->u32MemSize = FDB_TEMP_BUF_SIZE;
	obj->fpChkAbort = NULL;
	obj->bIsSkipDirForNonRecursive = FALSE;
	obj->bIsSkipHidden = FALSE;
	obj->SortSN_Delim = FDB_SORTSN_DELIM;
	obj->SortSN_DelimCount = FDB_SROTSN_DELIM_COUNT;
	obj->SortSN_CharNumOfSN = FDB_SROTSN_CHAR_NUM_OF_SN;

	buf_size = (int)FileDB_CalcBuffSize(obj->u32MaxFileNum, obj->u32MaxFilePathLen);
	printf("fdb buf size = %d\n", buf_size);

	if (FDB_TEMP_BUF_SIZE < buf_size) {
		printf("Not enough buf size (%d)\n", buf_size);
		return -1;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

int test_create_release_filedb(void)
{
	FILEDB_HANDLE   filedb_hdl;
	FILEDB_INIT_OBJ filedb_init_obj = {0};
	static char     tmp_path[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	int file_num = NUMTBL_N_TEST;
	int type_num = TYPETBL_N_TEST;
	int idx_fcount, idx_tbl, idx_type;
	int file_num_exp, file_num_got;
	VOS_TICK t1, t2;

	//clean the folder first
	if (-1 == fdb_delete_dir(g_fdb_test_dir)) {
		printf("fdb_delete_dir failed.\n");
		return -1;
	}

	//set filedb obj by default
	if (-1 == fdb_set_init_obj(&filedb_init_obj, g_fdb_test_dir)) {
		printf("fdb_set_init_obj failed.\n");
		return -1;
	}

	//create expected files (reserve one file slot)
	for (idx_tbl = 0; idx_tbl < file_num; idx_tbl++) {
		file_num_exp = g_fdb_num_tbl[idx_tbl];

		//reset the file count of each type
		for (idx_type = 0; idx_type < type_num; idx_type++) {
			g_fdb_type_tbl[idx_type].file_count = 0;
		}

		for (idx_fcount = 1; idx_fcount <= file_num_exp; idx_fcount++) {
			idx_type = RANDOM_RANGE(type_num);

			snprintf(tmp_path, sizeof(tmp_path), "%s2019_0101_%06d_001.%s",
				g_fdb_test_dir, idx_fcount, g_fdb_type_tbl[idx_type].ext_str);
			if (-1 == fdb_create_file(tmp_path)) {
				printf("fdb_create_file failed.\n");
				return -1;
			}

			g_fdb_type_tbl[idx_type].file_count++;
			//printf("%s created.\n", tmp_path);
		}

		//create filedb
		vos_perf_mark(&t1);
		filedb_hdl = FileDB_Create(&filedb_init_obj);
		if (FILEDB_CREATE_ERROR == filedb_hdl) {
			printf("FileDB_Create failed.\n");
			return -1;
		}
		vos_perf_mark(&t2);
		printf("%d CreateTime = %ld ms.\n", file_num_exp, vos_perf_duration(t1, t2) / 1000);

		//check the total file num
		file_num_got = (int)FileDB_GetTotalFileNum(filedb_hdl);
		if (file_num_exp != file_num_got) {
			printf("file_num_exp(%d), file_num_got(%d).\n", file_num_exp, file_num_got);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}

		//check the file num of different types
		for (idx_type = 0; idx_type < type_num; idx_type++) {
			file_num_exp = g_fdb_type_tbl[idx_type].file_count;
			file_num_got = (int)FileDB_GetFilesNumByFileType(filedb_hdl, g_fdb_type_tbl[idx_type].type_val);

			if (file_num_exp != file_num_got) {
				printf("Type %s: file_num_exp(%d), file_num_got(%d).\n",
					g_fdb_type_tbl[idx_type].ext_str, file_num_exp, file_num_got);
				FileDB_DumpInfo(filedb_hdl);
				FileDB_Release(filedb_hdl);
				return -1;
			}
		}

		//release filedb
		FileDB_Release(filedb_hdl);

		//remove the test folder
		if (-1 == fdb_delete_dir(g_fdb_test_dir)) {
			printf("fdb_delete_dir failed.\n");
			return -1;
		}
	}

	//printf("test_create_release_filedb pass.\n");
	return 0;
}

int test_browse_filedb(void)
{
	FILEDB_HANDLE   filedb_hdl;
	FILEDB_INIT_OBJ filedb_init_obj = {0};
	FILEDB_FILE_ATTR *p_file_attr;
	int file_num = 12;//(int)FILETBL_N_TEST;
	int idx_fdb;

	//clean the folder first
	if (-1 == fdb_delete_dir(g_fdb_test_dir)) {
		printf("fdb_delete_dir failed.\n");
		return -1;
	}

	//set filedb obj by default
	if (-1 == fdb_set_init_obj(&filedb_init_obj, g_fdb_test_dir)) {
		printf("fdb_set_init_obj failed.\n");
		return -1;
	}

	//create test files
	for (idx_fdb = 0; idx_fdb < file_num; idx_fdb++) {
		if (-1 == fdb_create_file(g_fdb_file_tbl[idx_fdb].file_path)) {
			printf("fdb_create_file failed\n");
			return -1;
		}
		//printf("%s created.\n", g_fdb_file_tbl[idx_fdb].file_path);
	}

	//create filedb
	filedb_hdl = FileDB_Create(&filedb_init_obj);
	if (FILEDB_CREATE_ERROR == filedb_hdl) {
		printf("FileDB_Create failed.\n");
		return -1;
	}

	//dump info
	//FileDB_DumpInfo(filedb_hdl);

	//Change curr index to the head
	idx_fdb = 0;
	p_file_attr = FileDB_SearhFile(filedb_hdl, (INT32)idx_fdb);
	if (NULL == p_file_attr) {
		printf("FileDB_SearhFile failed. (%d)\n", idx_fdb);
		FileDB_DumpInfo(filedb_hdl);
		FileDB_Release(filedb_hdl);
		return -1;
	}

	p_file_attr = FileDB_CurrFile(filedb_hdl);
	if (NULL == p_file_attr) {
		printf("FileDB_CurrFile failed.\n");
		FileDB_DumpInfo(filedb_hdl);
		FileDB_Release(filedb_hdl);
		return -1;
	}

	if (0 != strcmp(g_fdb_file_tbl[idx_fdb].file_path, p_file_attr->filePath)) {
		printf("idx_fdb(%d) should be %s, but got %s.\n",
				idx_fdb, g_fdb_file_tbl[idx_fdb].file_path, p_file_attr->filePath);
		FileDB_DumpInfo(filedb_hdl);
		FileDB_Release(filedb_hdl);
		return -1;
	}

	//test FileDB_Next from the head
	for (idx_fdb = 1; idx_fdb < file_num; idx_fdb++) {
		p_file_attr = FileDB_NextFile(filedb_hdl);
		if (NULL == p_file_attr) {
			printf("FileDB_NextFile failed\n");
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}

		if (0 != strcmp(g_fdb_file_tbl[idx_fdb].file_path, p_file_attr->filePath)) {
			printf("idx_fdb(%d) should be %s, but got %s\n",
					idx_fdb, g_fdb_file_tbl[idx_fdb].file_path, p_file_attr->filePath);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}
	}

	//Change curr index to the end
	idx_fdb = file_num - 1;
	p_file_attr = FileDB_SearhFile(filedb_hdl, (INT32)idx_fdb);
	if (NULL == p_file_attr) {
		printf("FileDB_SearhFile failed.\n");
		FileDB_DumpInfo(filedb_hdl);
		FileDB_Release(filedb_hdl);
		return -1;
	}

	p_file_attr = FileDB_CurrFile(filedb_hdl);
	if (NULL == p_file_attr) {
		printf("FileDB_CurrFile failed.\n");
		FileDB_DumpInfo(filedb_hdl);
		FileDB_Release(filedb_hdl);
		return -1;
	}

	if (0 != strcmp(g_fdb_file_tbl[idx_fdb].file_path, p_file_attr->filePath)) {
		printf("idx_fdb(%d) should be %s, but got %s.\n",
				idx_fdb, g_fdb_file_tbl[idx_fdb].file_path, p_file_attr->filePath);
		FileDB_DumpInfo(filedb_hdl);
		FileDB_Release(filedb_hdl);
		return -1;
	}

	//Test PrevFile from the end
	for (idx_fdb = file_num - 2; idx_fdb > 0; idx_fdb--) {
		p_file_attr = FileDB_PrevFile(filedb_hdl);
		if (NULL == p_file_attr) {
			printf("FileDB_PrevFile failed.\n");
			break;
		}

		if (0 != strcmp(g_fdb_file_tbl[idx_fdb].file_path, p_file_attr->filePath)) {
			printf("idx_fdb(%d) should be %s, but got %s.\n",
				idx_fdb, g_fdb_file_tbl[idx_fdb].file_path, p_file_attr->filePath);
			break;
		}
	}

	//release filedb
	FileDB_Release(filedb_hdl);

	//remove the test folder
	if (-1 == fdb_delete_dir(g_fdb_test_dir)) {
		printf("fdb_delete_dir failed.\n");
		return -1;
	}

	//printf("test_browse_filedb pass.\n");
	return 0;
}

int test_sort_filedb(void)
{
	FILEDB_HANDLE   filedb_hdl;
	FILEDB_INIT_OBJ filedb_init_obj = {0};
	int file_num = 11;//(int)FILETBL_N_SORT;
	int idx_tbl;
	int idx_stype;

	//clean the folder first
	if (-1 == fdb_delete_dir(g_fdb_test_dir)) {
		printf("fdb_delete_dir failed.\n");
		return -1;
	}

	//set filedb obj by default
	if (-1 == fdb_set_init_obj(&filedb_init_obj, g_fdb_test_dir)) {
		printf("fdb_set_init_obj failed.\n");
		return -1;
	}

	//create test files
	for (idx_tbl = 0; idx_tbl < file_num; idx_tbl++) {
		if (-1 == fdb_create_file(g_fdb_sort_tbl[idx_tbl].file_path)) {
			printf("fdb_create_file failed.\n");
			return -1;
		}
		//printf("%s created.\n", g_fdb_sort_tbl[idx_tbl].file_path);
	}

	//set the mod date time by reverse order
	for (idx_tbl = 0; idx_tbl < file_num; idx_tbl++) {
		UINT32  TmpCreDT[FST_FILE_DATETIME_MAX_ID] = {0};
		UINT32  TmpModDT[FST_FILE_DATETIME_MAX_ID] = {0};
		char    *tmp_path = g_fdb_sort_tbl[idx_tbl].file_path;

		if (FST_STA_OK != FileSys_GetDateTime(tmp_path, (UINT32*)TmpCreDT, (UINT32*)TmpModDT)) {
			printf("FileSys_GetDateTime(%s) failed.\n", tmp_path);
		}

		TmpModDT[0] = TmpModDT[0] + 1; //year+1 to make sure DateTime mod > cre
		TmpModDT[1] = 1; //month
		TmpModDT[2] = 1; //day
		TmpModDT[3] = 1; //hour
		TmpModDT[4] = (int)FILETBL_N_SORT - idx_tbl;   //min
		TmpModDT[5] = 0; //second

		//only change the mod time
		if (FST_STA_OK != FileSys_SetDateTime(tmp_path, (UINT32*)TmpCreDT, (UINT32*)TmpModDT)) {
			printf("FileSys_SetDateTime(%s) failed.\n", tmp_path);
		}
	}

	//create filedb
	filedb_hdl = FileDB_Create(&filedb_init_obj);
	if (FILEDB_CREATE_ERROR == filedb_hdl) {
		printf("FileDB_Create failed.\n");
		return -1;
	}

	//print the original order (FILEDB_SORT_BY_NONE)
	//printf("Sort Type: FILEDB_SORT_BY_NONE\n");
	//FileDB_DumpInfo(filedb_hdl);

	//for loop different sort types
	for (idx_stype = FDB_MIN_TYPE; idx_stype <= FDB_MAX_TYPE; idx_stype++) {
		if (idx_stype == FILEDB_SORT_BY_STROKENUM) {
			continue;    //skip FILEDB_SORT_BY_STROKENUM
		}

		printf("Sort Type: %d\r\n", idx_stype);
		FileDB_SortBy(filedb_hdl, idx_stype, 0);
		//FileDB_DumpInfo(filedb_hdl);

		if (-1 == fdb_check_sort(filedb_hdl, idx_stype)) {
			printf("fdb_check_sort failed (First), idx_stype = %d\r\n", idx_stype);
			continue;
		}

		if (idx_stype == FILEDB_SORT_BY_NAME ||
			idx_stype == FILEDB_SORT_BY_FILEPATH ||
			idx_stype == FILEDB_SORT_BY_FILETYPE) {
			if (-1 == fdb_delete_add(filedb_hdl)) {
				printf("fdb_delete_add failed, idx_stype = %d.\n", idx_stype);
				continue;
			}

			if (-1 == fdb_check_sort(filedb_hdl, idx_stype)) {
				printf("fdb_check_sort failed (DeleteAdd), idx_stype = %d.\n", idx_stype);
				continue;
			}

			//refresh here because fdb_delete_add will lost some file info
			if (E_OK != FileDB_Refresh(filedb_hdl)) {
				printf("FileDB_Refresh failed, idx_stype = %d.\n", idx_stype);
				continue;
			}
		}

		if (idx_stype ==  FILEDB_SORT_BY_SN) {
			FILEDB_FILE_ATTR *p_file_attr;
			int idx_fdb;
			int count;
			//int SN;

			//test FileDB_GetSNByName randomly
			for (count = 0; count < 50; count++) {
				idx_fdb = RANDOM_RANGE(file_num);

				p_file_attr = FileDB_SearhFile(filedb_hdl, (INT32)idx_fdb);
				if (NULL == p_file_attr) {
					printf("FileDB_SearhFile failed. (%d)\n", idx_fdb);
					FileDB_DumpInfo(filedb_hdl);
					FileDB_Release(filedb_hdl);
					return -1;
				}

				//SN = (int)FileDB_GetSNByName(filedb_hdl, p_file_attr->filePath);

				//printf("filename = %s, SN = %d\n", p_file_attr->filePath, SN);
			}
		}
	}

	//release filedb
	FileDB_Release(filedb_hdl);

	//remove the test folder
	if (-1 == fdb_delete_dir(g_fdb_test_dir)) {
		printf("fdb_delete_dir failed.\n");
		return -1;
	}

	//printf("test_sort_filedb pass.\n");
	return 0;
}

int test_index_filedb(void)
{
	FILEDB_HANDLE   filedb_hdl;
	FILEDB_INIT_OBJ filedb_init_obj = {0};
	FILEDB_FILE_ATTR *p_file_attr;
	int file_num = 12;//(int)FILETBL_N_TEST;
	int idx_fdb;
	int idx_tmp;
	int count;

	//clean the folder first
	if (-1 == fdb_delete_dir(g_fdb_test_dir)) {
		printf("fdb_delete_dir failed.\n");
		return -1;
	}

	//set filedb obj by default
	if (-1 == fdb_set_init_obj(&filedb_init_obj, g_fdb_test_dir)) {
		printf("fdb_set_init_obj failed.\n");
		return -1;
	}

	//create test files
	for (idx_fdb = 0; idx_fdb < file_num; idx_fdb++) {
		if (-1 == fdb_create_file(g_fdb_file_tbl[idx_fdb].file_path)) {
			printf("fdb_create_file failed\n");
			return -1;
		}
		//printf("%s created.\n", g_fdb_file_tbl[idx_fdb].file_path);
	}

	//create filedb
	filedb_hdl = FileDB_Create(&filedb_init_obj);
	if (FILEDB_CREATE_ERROR == filedb_hdl) {
		printf("FileDB_Create failed.\n");
		return -1;
	}

	//dump info
	//FileDB_DumpInfo(filedb_hdl);

	//test FileDB_SearhFile randomly
	for (count = 0; count < 50; count++) {
		idx_fdb = RANDOM_RANGE(file_num);

		p_file_attr = FileDB_SearhFile(filedb_hdl, (INT32)idx_fdb);
		if (NULL == p_file_attr) {
			printf("FileDB_SearhFile (%d) failed.\n", idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}

		//index should also change
		if (idx_fdb != (int)FileDB_GetCurrFileIndex(filedb_hdl)) {
			printf("FileDB_GetCurrFileIndex(%d) and idx_fdb(%d) not match.\n", (int)FileDB_GetCurrFileIndex(filedb_hdl), idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}

		if (0 != strcmp(g_fdb_file_tbl[idx_fdb].file_path, p_file_attr->filePath)) {
			printf("idx_fdb(%d) should be %s, but got %s.\n",
					idx_fdb, g_fdb_file_tbl[idx_fdb].file_path, p_file_attr->filePath);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}
	}

	//test FileDB_SearhFile2 randomly
	idx_tmp = (int)FileDB_GetCurrFileIndex(filedb_hdl);

	for (count = 0; count < 50; count++) {
		idx_fdb = RANDOM_RANGE(file_num);

		p_file_attr = FileDB_SearhFile2(filedb_hdl, (INT32)idx_fdb);
		if (NULL == p_file_attr) {
			printf("FileDB_SearhFile2 failed. (%d)\n", idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}

		//index should not change
		if (idx_tmp != (int)FileDB_GetCurrFileIndex(filedb_hdl)) {
			printf("index is changed to %d, should remains %d.\n", (int)FileDB_GetCurrFileIndex(filedb_hdl), idx_tmp);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}

		if (0 != strcmp(g_fdb_file_tbl[idx_fdb].file_path, p_file_attr->filePath)) {
			printf("idx_fdb(%d) should be %s, but got %s\n",
				idx_fdb, g_fdb_file_tbl[idx_fdb].file_path, p_file_attr->filePath);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}
	}

	//test FileDB_GetIndexByPath randomly
	for (count = 0; count < 50; count++) {
		idx_fdb = RANDOM_RANGE(file_num);

		idx_tmp = (int)FileDB_GetIndexByPath(filedb_hdl, g_fdb_file_tbl[idx_fdb].file_path);
		if (idx_tmp != idx_fdb) {
			printf("FileDB_GetIndexByPath(%s) = %d, should be %d.\n", g_fdb_file_tbl[idx_fdb].file_path, idx_tmp, idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}
	}

	//release filedb
	FileDB_Release(filedb_hdl);

	//remove the test folder
	if (-1 == fdb_delete_dir(g_fdb_test_dir)) {
		printf("fdb_delete_dir failed.\n");
		return -1;
	}

	//printf("test_index_filedb pass.\n");
	return 0;
}

int test_set_filedb(void)
{
	FILEDB_HANDLE   filedb_hdl;
	FILEDB_INIT_OBJ filedb_init_obj = {0};
	FILEDB_FILE_ATTR *p_file_attr = NULL;
	int file_num = 12;//(int)FILETBL_N_TEST;
	int idx_fdb;
	UINT64 old_size = 0;
	UINT64 new_size = 0x40000;

	//clean the folder first
	if (-1 == fdb_delete_dir(g_fdb_test_dir)) {
		printf("fdb_delete_dir failed.\n");
		return -1;
	}

	//set filedb obj by default
	if (-1 == fdb_set_init_obj(&filedb_init_obj, g_fdb_test_dir)) {
		printf("fdb_set_nit_obj failed.\n");
		return -1;
	}

	//create test files
	for (idx_fdb = 0; idx_fdb < file_num; idx_fdb++) {
		if (-1 == fdb_create_file(g_fdb_file_tbl[idx_fdb].file_path)) {
			printf("fdb_create_file failed.\n");
			return -1;
		}
		//printf("%s created.\n", g_fdb_file_tbl[idx_fdb].file_path);
	}

	//create filedb
	filedb_hdl = FileDB_Create(&filedb_init_obj);
	if (FILEDB_CREATE_ERROR == filedb_hdl) {
		printf("FileDB_Create failed.\n");
		return -1;
	}

	//dump info
	//FileDB_DumpInfo(filedb_hdl);

	//change curr index to the head
	idx_fdb = 0;
	p_file_attr = FileDB_SearhFile(filedb_hdl, (INT32)idx_fdb);
	if (NULL == p_file_attr) {
		printf("FileDB_SearhFile failed. (head)\n");
		FileDB_DumpInfo(filedb_hdl);
		FileDB_Release(filedb_hdl);
		return -1;
	}

	p_file_attr = FileDB_CurrFile(filedb_hdl);
	if (NULL == p_file_attr) {
		printf("FileDB_CurrFile failed. (head)\n");
		FileDB_DumpInfo(filedb_hdl);
		FileDB_Release(filedb_hdl);
		return -1;
	}

	if (0 != strcmp(g_fdb_file_tbl[idx_fdb].file_path, p_file_attr->filePath)) {
		printf("idx_fdb(%d) should be %s, but got %s.\n",
				idx_fdb, g_fdb_file_tbl[idx_fdb].file_path, p_file_attr->filePath);
		FileDB_DumpInfo(filedb_hdl);
		FileDB_Release(filedb_hdl);
		return -1;
	}

	//test FileDB_SetFileSize from the head
	for (idx_fdb = 0; idx_fdb < file_num; idx_fdb++) {
		p_file_attr = FileDB_SearhFile(filedb_hdl, (INT32)idx_fdb);
		if (NULL == p_file_attr) {
			printf("FileDB_SearhFile failed. (%d)\n", idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}
		old_size = p_file_attr->fileSize64;

		FileDB_SetFileSize(filedb_hdl, (INT32)idx_fdb, new_size);

		p_file_attr = FileDB_SearhFile(filedb_hdl, idx_fdb);
		if (NULL == p_file_attr) {
			printf("FileDB_SearhFile failed. (%d)\n", (int)idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}

		if (p_file_attr->fileSize64 != new_size) {
			printf("FileDB_SetFileSize(%s) = %lld, should be %lld.\n",
				p_file_attr->filePath, p_file_attr->fileSize64, new_size);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}

		FileDB_SetFileSize(filedb_hdl, idx_fdb, old_size);
	}

	//test FileDB_SetFileRO from the head
	for (idx_fdb = 0; idx_fdb < file_num; idx_fdb++) {

		FileDB_SetFileRO(filedb_hdl, (INT32)idx_fdb, TRUE);

		p_file_attr = FileDB_SearhFile(filedb_hdl, (INT32)idx_fdb);
		if (NULL == p_file_attr) {
			printf("FileDB_SearhFile failed (%d).\n", idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}
		if (!(p_file_attr->attrib & FST_ATTRIB_READONLY)) {
			printf("FileDB_SetFileRO-(TRUE) (%d) failed.\n", idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}

		FileDB_SetFileRO(filedb_hdl, (INT32)idx_fdb, FALSE);

		p_file_attr = FileDB_SearhFile(filedb_hdl, (INT32)idx_fdb);
		if (NULL == p_file_attr) {
			printf("FileDB_SearhFile failed (%d).\n", idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}
		if (p_file_attr->attrib & FST_ATTRIB_READONLY) {
			printf("FileDB_SetFileRO-(FALSE) (%d) failed.\n", idx_fdb);
			FileDB_DumpInfo(filedb_hdl);
			FileDB_Release(filedb_hdl);
			return -1;
		}
	}

	//test FileDB_AddFileToLast randomly
	if (-1 == fdb_delete_add_to_last(filedb_hdl)) {
		printf("fdb_delete_add_to_last failed.\n");
		FileDB_DumpInfo(filedb_hdl);
		FileDB_Release(filedb_hdl);
		return -1;
	}

	//release filedb
	FileDB_Release(filedb_hdl);

	//remove the test folder
	if (-1 == fdb_delete_dir(g_fdb_test_dir)) {
		printf("fdb_delete_dir failed.\n");
		return -1;
	}

	//printf("test_set_filedb pass.\n");
	return 0;
}

static UINT32 nvt_hdal_init(void)
{
	HD_RESULT ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	ret = hd_common_init(0);
	if(ret != HD_OK) {
		printf("common init fail=%d\n", (int)ret);
		return -1;
	}
	ret = hd_common_mem_init(&mem_cfg);
	if(ret != HD_OK) {
		printf("mem init fail=%d\n", (int)ret);
		return -1;
	}
	ret = hd_gfx_init();
	if(ret != HD_OK) {
		printf("gfx init fail=%d\n", (int)ret);
		return -1;
	}
	return 0;
}

static UINT32 nvt_hdal_uninit(void)
{
	HD_RESULT ret;

	ret = hd_gfx_uninit();
	if(ret != HD_OK) {
		printf("gfx uninit fail=%d\n", (int)ret);
		return -1;
	}
	ret = hd_common_mem_uninit();
	if(ret != HD_OK) {
		printf("mem uninit fail=%d\n", (int)ret);
		return -1;
	}
	ret = hd_common_uninit();
	if(ret != HD_OK) {
		printf("common uninit fail=%d\n", (int)ret);
		return -1;
	}
	return 0;
}

EXAMFUNC_ENTRY(test_filedb, argc, argv)
{
	FileDB_InstallID();

	nvt_hdal_init();

	fdb_mem_init();

	// test 0
	if (0 == test_create_release_filedb()) {
		temp_result[0] = 1; //pass
	}
	// test 1
	if (0 == test_browse_filedb()) {
		temp_result[1] = 1; //pass
	}
	// test 2
	if (0 == test_sort_filedb()) {
		temp_result[2] = 1; //pass
	}
	// test 3
	if (0 == test_index_filedb()) {
		temp_result[3] = 1; //pass
	}
	// test 4
	if (0 == test_set_filedb()) {
		temp_result[4] = 1; //pass
	}

	fdb_mem_uninit();

	nvt_hdal_uninit();

	if (temp_result[0]) {
		printf("test_create_release_filedb pass\n");
	} else {
		printf("test_create_release_filedb failed\n");
	}
	if (temp_result[1]) {
		printf("test_browse_filedb pass\n");
	} else {
		printf("test_browse_filedb failed\n");
	}
	if (temp_result[2]) {
		printf("test_sort_filedb pass\n");
	} else {
		printf("test_sort_filedb failed\n");
	}
	if (temp_result[3]) {
		printf("test_index_filedb pass\n");
	} else {
		printf("test_index_filedb failed\n");
	}
	if (temp_result[4]) {
		printf("test_set_filedb pass\n");
	} else {
		printf("test_set_filedb failed\n");
	}

 	return 0;
}

