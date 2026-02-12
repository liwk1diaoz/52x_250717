/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       ImageApp_MovieMulti_FileManage.h
    @ingroup    mIImageApp

    @note       Nothing.

    @date       2017/06/14
*/
#ifndef IA_MOVIEMULTI_FILE_MANAGE_H
#define IA_MOVIEMULTI_FILE_MANAGE_H
#include "ImageApp/ImageApp_MovieMulti.h"

#define IAMOVIEMULTI_FM_FDB_CNT   2
#define IAMOVIEMULTI_FM_DRV_FIRST  'A'
#define IAMOVIEMULTI_FM_DRV_LAST   (IAMOVIEMULTI_FM_DRV_FIRST + IAMOVIEMULTI_FM_FDB_CNT - 1)
#define IAMOVIEMULTI_FM_DIR_NUM    MOVIEMULTI_DISK_MAX_DIR_NUM

#define IAMOVIEMULTI_FM_RESV_SIZE      10*1024*1024ULL
#define IAMOVIEMULTI_FM_RO_PCT         30
#define IAMOVIEMULTI_FM_RO_NUM         80
#define IAMOVIEMULTI_FM_CHK_PCT        1
#define IAMOVIEMULTI_FM_CHK_NUM        2

typedef enum {
	IAMOVIEMULTI_FM_ROINFO_BASE = 0,
	IAMOVIEMULTI_FM_ROINFO_PCT,
	IAMOVIEMULTI_FM_ROINFO_NUM,
	IAMOVIEMULTI_FM_ROINFO_TYPE,
	IAMOVIEMULTI_FM_ROINFO_MAX,
	ENUM_DUMMY4WORD(IAMOVIEMULTI_FM_ROINFO)
} IAMOVIEMULTI_FM_ROINFO;

INT32 iamoviemulti_fm_dump(CHAR drive);
INT32 iamoviemulti_fm_open(CHAR drive);
INT32 iamoviemulti_fm_close(CHAR drive);
INT32 iamoviemulti_fm_refresh(CHAR drive);
INT32 iamoviemulti_fm_config(MOVIE_CFG_FDB_INFO *p_cfg);

INT32 iamoviemulti_fm_is_loop_del(CHAR drive);
INT32 iamoviemulti_fm_is_add_to_last(CHAR drive);
INT32 iamoviemulti_fm_is_sync_time(CHAR drive);
INT32 iamoviemulti_fm_is_ro_chk_last(CHAR drive);
INT32 iamoviemulti_fm_set_loop_del(CHAR drive, INT32 is_loop_del);
INT32 iamoviemulti_fm_set_add_to_last(CHAR drive, INT32 is_add_to_last);
INT32 iamoviemulti_fm_set_sync_time(CHAR drive, INT32 is_sync_time);
INT32 iamoviemulti_fm_set_ro_chk_last(CHAR drive, INT32 is_ro_chk_last);

INT32 iamoviemulti_fm_chk_naming(CHAR *p_path);
INT32 iamoviemulti_fm_chk_time(UINT16 cre_date, UINT16 cre_time, UINT64 *time);
INT32 iamoviemulti_fm_chk_fdb(CHAR drive);
INT32 iamoviemulti_fm_add_file(CHAR *p_path); //filedb add
INT32 iamoviemulti_fm_del_file(CHAR drive, UINT64 need_size); //fs del + filedb del
INT32 iamoviemulti_fm_reuse_file(CHAR *p_new_path);
INT32 iamoviemulti_fm_replace_file(CHAR *p_old_path, CHAR *p_new_folder, CHAR *p_new_path);
INT32 iamoviemulti_fm_move_file(CHAR *p_old_path, CHAR *p_new_folder, CHAR *p_new_path); //fs move + filedb move
INT32 iamoviemulti_fm_flush_file(CHAR *p_path);
INT32 iamoviemulti_fm_set_ro_file(CHAR *p_path, BOOL is_ro);
INT32 iamoviemulti_fm_chk_ro_file(CHAR drive);
INT32 iamoviemulti_fm_chk_ro_size(CHAR drive);
INT32 iamoviemulti_fm_del_ro_file(CHAR drive, UINT64 *size);
INT32 iamoviemulti_fm_set_remain_size(CHAR drive, UINT64 size);
INT32 iamoviemulti_fm_set_time_val(CHAR drive, UINT64 value);
INT32 iamoviemulti_fm_get_time_val(CHAR drive, UINT64 *value);
INT32 iamoviemulti_fm_set_file_del_max(CHAR drive, INT32 value);
INT32 iamoviemulti_fm_get_file_del_max(CHAR drive, INT32 *value);
INT32 iamoviemulti_fm_set_rofile_info(CHAR drive, INT32 value, INT32 type);
INT32 iamoviemulti_fm_get_rofile_info(CHAR drive, INT32 *value, INT32 type);

INT32 iamoviemulti_fm_set_resv_size(CHAR drive, UINT64 size, UINT32 is_loop); //1: loop recording on, 0: loop off
INT32 iamoviemulti_fm_get_resv_size(CHAR drive, UINT64 *p_size);
INT32 iamoviemulti_fm_get_remain_size(CHAR drive, UINT64 *p_size); //get fout remain size
INT32 iamoviemulti_fm_get_filedb_num(CHAR drive);

//time control
INT32 iamoviemulti_fm_keep_mtime(CHAR *p_path);
INT32 iamoviemulti_fm_sync_mtime(CHAR *p_path);
INT32 iamoviemulti_fm_dump_mtime(CHAR * p_path);

//format-free
INT32 iamoviemulti_fm_is_format_free(CHAR drive);
INT32 iamoviemulti_fm_set_format_free(CHAR drive, BOOL is_on);
INT32 iamoviemulti_fm_allocate(CHAR drive);
INT32 iamoviemulti_fm_scan(CHAR drive);
INT32 iamoviemulti_fm_merge(CHAR drive);
INT32 iamoviemulti_fm_package(CHAR drive, MOVIEMULTI_DISK_INFO *p_disk_info);
INT32 iamoviemulti_fm_set_dir_info(CHAR *p_dir, UINT32 ratio, UINT64 size);
INT32 iamoviemulti_fm_get_dir_info(CHAR *p_dir, UINT32 *ratio, UINT64 *size);
INT32 iamoviemulti_fm_set_cyclic_rec(CHAR *p_dir, BOOL is_on);
INT32 iamoviemulti_fm_chk_format_free_dir_full(CHAR *p_path);
INT32 iamoviemulti_fm_set_usage_size(CHAR drive, UINT64 size);
INT32 iamoviemulti_fm_get_usage_size(CHAR drive, UINT64 *p_size);
INT32 iamoviemulti_fm_set_extstr_type(CHAR drive, UINT32 file_type);
INT32 iamoviemulti_fm_get_extstr_type(CHAR drive, UINT32 *file_type);
INT32 iamoviemulti_fm_get_extstr(CHAR drive, char *extstr);
INT32 iamoviemulti_fm_is_reuse_hidden_first(CHAR drive);
INT32 iamoviemulti_fm_set_reuse_hidden_first(CHAR drive, BOOL value);
INT32 iamoviemulti_fm_set_file_unit(CHAR *p_dir, UINT32 unit);
INT32 iamoviemulti_fm_is_skip_read_only(CHAR drive);
INT32 iamoviemulti_fm_set_skip_read_only(CHAR drive, BOOL value);

INT32 ImageApp_MovieMulti_FMAllocate(CHAR drive);
INT32 ImageApp_MovieMulti_FMScan(CHAR drive, MOVIEMULTI_DISK_INFO* p_disk_info);
INT32 ImageApp_MovieMulti_FMDump(CHAR drive);
INT32 ImageApp_MovieMulti_FMOpen(CHAR drive);
INT32 ImageApp_MovieMulti_FMClose(CHAR drive);
INT32 ImageApp_MovieMulti_FMRefresh(CHAR drive);
INT32 ImageApp_MovieMulti_FMConfig(MOVIE_CFG_FDB_INFO *p_cfg);
void ImageApp_MovieMulti_FMSetSortBySN(CHAR *pDelimStr, UINT32 nDelimCount, UINT32 nNumOfSn);

#endif




