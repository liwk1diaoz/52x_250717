/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/

// INCLUDE PROTECTED
#include "fileout_init.h"

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef struct {
	FST_FILE filehdl;                 //file handel for sysinfo
	CHAR     path[FILEOUT_PATH_LEN];  //path for sysinfo
	UINT32   alloc_size;              //sysinfo fallocate size
	UINT32   is_alloc;                //sysinfo allocate or net
} FILEOUT_SYSINFO;

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static INT32 g_tsk_info_last_qidx[FILEOUT_DRV_NUM] = {0};
static INT32 g_tsk_func_count[FILEOUT_DRV_NUM] = {0};
static FILEOUT_SYSINFO g_tsk_sysinfo[FILEOUT_DRV_NUM] = {
	{NULL, "A:\\sysinfo", FILEOUT_DEFAULT_STRGBLK_SIZE, 0},
	{NULL, "B:\\sysinfo", FILEOUT_DEFAULT_STRGBLK_SIZE, 0},
};

extern BOOL g_log_dbginfo_is_on;
#define FILEOUT_MSG(fmtstr, args...) do { if(g_log_dbginfo_is_on) DBG_DUMP(fmtstr, ##args); } while(0)

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
static FILEOUT_SYSINFO* fileout_tsk_get_sysinfo(CHAR drive)
{
	FILEOUT_SYSINFO* p_sysinfo = NULL;

	if (drive < FILEOUT_DRV_NAME_FIRST || drive > FILEOUT_DRV_NAME_LAST) {
		DBG_ERR("Invalid Drive 0x%X\r\n", (UINT32)drive);
		return NULL;
	}
	p_sysinfo = &g_tsk_sysinfo[drive - FILEOUT_DRV_NAME_FIRST];
	return p_sysinfo;
}

static INT32 fileout_tsk_set_last_qidx(CHAR drive, INT32 qidx)
{
	INT32 tsk_idx;

	if (drive < FILEOUT_DRV_NAME_FIRST || drive > FILEOUT_DRV_NAME_LAST) {
		DBG_ERR("Invalid Drive 0x%X\r\n", (UINT32)drive);
		return -1;
	}
	tsk_idx = drive - FILEOUT_DRV_NAME_FIRST;
	g_tsk_info_last_qidx[tsk_idx] = qidx;

	return 0;
}

static INT32 fileout_tsk_get_last_qidx(CHAR drive, INT32 *qidx)
{
	INT32 tsk_idx;

	if (drive < FILEOUT_DRV_NAME_FIRST || drive > FILEOUT_DRV_NAME_LAST) {
		DBG_ERR("Invalid Drive 0x%X\r\n", (UINT32)drive);
		return -1;
	}
	tsk_idx = drive - FILEOUT_DRV_NAME_FIRST;
	*qidx = g_tsk_info_last_qidx[tsk_idx];

	return 0;
}

static INT32 fileout_tsk_do_sysinfo(ISF_FILEOUT_BUF *p_buf, INT32 queue_idx)
{
	FILEOUT_SYSINFO *p_sys_info = NULL;
	FST_FILE filehdl = NULL;
	UINT32 fileop = p_buf->fileop;
	UINT32 addr = p_buf->addr;
	UINT32 size = (UINT32)p_buf->size;
	UINT64 pos = p_buf->pos;
	UINT32 ret = -1;

	p_sys_info = fileout_tsk_get_sysinfo(p_buf->p_fpath[0]);
	if (NULL == p_sys_info) {
		DBG_ERR("Get sysinfo failed\r\n");
		goto label_fileout_tsk_do_sysinfo_end;
	}
	filehdl = p_sys_info->filehdl;
	if (filehdl == NULL) {
		DBG_ERR("Get filehdl failed\r\n");
		goto label_fileout_tsk_do_sysinfo_end;
	}

	DBG_IND("^G fileop %d addr %d size %d pos %lld\r\n", fileop, addr, size, pos);

	if (fileop & ISF_FILEOUT_FOP_CONT_WRITE) {
		ret = FileSys_WriteFile(filehdl, (UINT8 *)addr, &size, 0, NULL);
		if (ret != FST_STA_OK) {
			DBG_ERR("WRTIE (0x%X/%d) failed\r\n", addr, size);
			goto label_fileout_tsk_do_sysinfo_end;
		}
		DBG_IND("WRTIE (0x%X/%d) done\r\n", addr, size);
	}
	if (fileop & ISF_FILEOUT_FOP_SEEK_WRITE) {
		ret = FileSys_SeekFile(filehdl, pos, FST_SEEK_SET);
		if (ret != FST_STA_OK) {
			DBG_ERR("SEEK %lld failed\r\n", pos);
			goto label_fileout_tsk_do_sysinfo_end;
		}
		DBG_IND("SEEK %lld done\r\n", pos);
	}
	if (fileop & ISF_FILEOUT_FOP_FLUSH) {
		ret = FileSys_FlushFile(filehdl);
		if (ret != FST_STA_OK) {
			DBG_ERR("FLUSH failed\r\n");
			goto label_fileout_tsk_do_sysinfo_end;
		}
		DBG_IND("FLUSH done\r\n");
	}

label_fileout_tsk_do_sysinfo_end:
	return ret;
}

static INT32 fileout_tsk_do_ops(ISF_FILEOUT_BUF *p_buf, INT32 queue_idx)
{
	FST_FILE filehdl = NULL;
	UINT32 write_size = (UINT32)p_buf->size;
	UINT32 write_addr = p_buf->addr;
	UINT64 write_pos = p_buf->pos;
	CHAR *write_path = p_buf->p_fpath;
	INT32 ret_fs;
	UINT32 old_time, new_time;
	UINT32 dur_ms = 0;
	INT32 slowcard_is_on = fileout_util_is_slowcard();
	UINT64 alloc_size = 0;
	UINT32 open_flag = 0;

	fileout_ops_lock();

	//show size info
	fileout_util_show_quesize();

	if (fileout_util_is_skip_ops(queue_idx)) {
		ret_fs = FST_STA_OK;
		goto label_fileout_tsk_do_ops_end;
	}

	//check storage
	if (FileSys_GetDiskInfoEx(write_path[0], FST_INFO_IS_SDIO_ERR)) {
		ret_fs = FST_STA_SDIO_ERR;
		goto label_fileout_tsk_write_card_err;
	}

	//specific usage
	if (p_buf->event == FILEOUT_FEVENT_BSINCARD) {
		if (fileout_util_is_usememblk()) {
			DBG_IND("^G BSMUXER_FEVENT_BSINCARD -start\r\n");
			ret_fs = fileout_tsk_do_sysinfo(p_buf, queue_idx);
			DBG_IND("^G BSMUXER_FEVENT_BSINCARD -end\r\n");
			goto label_fileout_tsk_do_ops_end;
		} else {
			DBG_WRN("FILEOUT_PARAM_READ_STRGBLK not set\r\n");
		}
	}

	//trigger delete before ops buf
	ret_fs = fileout_cb_call_delete(queue_idx, p_buf);
	if (0 != ret_fs) {
		if (FST_STA_NOFREE_SPACE == ret_fs) {
			if (p_buf->fileop & ISF_FILEOUT_FOP_CREATE) {
				DBG_WRN("Q[%d] no more free space to create file\r\n", queue_idx);
				goto label_fileout_tsk_do_ops_end;
			}
		} else {
			DBG_ERR("Q[%d] call delete failed\r\n", queue_idx);
			goto label_fileout_tsk_do_ops_end;
		}
	}

	//DISCARD
	if (p_buf->fileop == ISF_FILEOUT_FOP_DISCARD) {
		DBG_IND("^G [FOUT][%d] DISCARD received.\r\n", queue_idx);

		filehdl = fileout_queue_get_filehdl(queue_idx);
		if (NULL != filehdl) {
			//try to close file
			ret_fs = FileSys_CloseFile(filehdl);
			if (ret_fs != FST_STA_OK) {
				DBG_ERR("Q[%d] DISCARD %s failed\r\n", queue_idx, write_path);
				if (FST_STA_SDIO_ERR == ret_fs) {
					goto label_fileout_tsk_write_card_err;
				}
				goto label_fileout_tsk_do_ops_end;
			}
			filehdl = NULL;
			ret_fs = fileout_queue_set_filehdl(queue_idx, filehdl);
			if (ret_fs != FST_STA_OK) {
				DBG_ERR("Q[%d] DISCARD set filehdl failed\r\n", queue_idx);
				goto label_fileout_tsk_do_ops_end;
			}

			//try to delete file
			ret_fs = FileSys_DeleteFile(write_path);
			if (FSS_FILE_NOTEXIST == ret_fs) {
				ret_fs = 0;
			}
			if (FST_STA_OK != ret_fs) {
				DBG_ERR("Q[%d] DISCARD %s failed\r\n", queue_idx, write_path);
				goto label_fileout_tsk_do_ops_end;
			}
			DBG_DUMP("^GQ[%d] DISCARD %s deleted.\r\n", queue_idx, write_path);
		}

		ret_fs = 0;
		goto label_fileout_tsk_do_ops_end;
	}

	//SNAPSHOT
	if (p_buf->fileop == ISF_FILEOUT_FOP_SNAPSHOT) {

		DBG_IND("SNAPSHOT [%d] addr:%lu size:%lu\r\n", write_addr, write_size);

		if (fileout_util_is_formatfree()) {
			open_flag = FST_OPEN_EXISTING | FST_OPEN_WRITE;
		} else {
			open_flag = FST_CREATE_ALWAYS | FST_OPEN_WRITE;
		}

		if (fileout_util_is_close_on_exec()) {
			open_flag |= FST_CLOSE_ON_EXEC;
		}

		fileout_util_time_mark(&old_time);
		filehdl = FileSys_OpenFile(write_path, open_flag);
		if (NULL == filehdl) {
			DBG_ERR("Q[%d] SNAPSHOT open %s failed\r\n", queue_idx, write_path);
			goto label_fileout_tsk_write_card_err;
		}
		//error check
		if (NULL == (UINT8 *)write_addr) {
			DBG_ERR("Q[%d] SNAPSHOT write_addr error\r\n", queue_idx);
			goto label_fileout_tsk_do_ops_end;
		}
		ret_fs = FileSys_WriteFile(filehdl, (UINT8 *)write_addr, &write_size, 0, NULL);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] SNAPSHOT write %s failed\r\n", queue_idx, write_path);
			goto label_fileout_tsk_do_ops_end;
		}

		ret_fs = FileSys_CloseFile(filehdl);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] SNAPSHOT close %s failed\r\n", queue_idx, write_path);
			goto label_fileout_tsk_write_card_err;
		}
		fileout_util_time_mark(&new_time);

		fileout_util_time_dur_ms(old_time, new_time, &dur_ms);
		fileout_util_show_opsinfo(dur_ms, ISF_FILEOUT_FOP_SNAPSHOT, p_buf, queue_idx);

		ret_fs = fileout_cb_call_closed(queue_idx, p_buf);
		if (ret_fs != 0) {
			DBG_ERR("Q[%d] SNAPSHOT call closed failed\r\n", queue_idx);
			goto label_fileout_tsk_do_ops_end;
		}

		//process end (success)
		ret_fs = 0;
		goto label_fileout_tsk_do_ops_end;
	}

	//create
	if (p_buf->fileop & ISF_FILEOUT_FOP_CREATE) {
		filehdl = fileout_queue_get_filehdl(queue_idx);
		if (NULL != filehdl) {
			DBG_ERR("Q[%d] CREATE filehdl is not NULL\r\n", queue_idx);
			goto label_fileout_tsk_do_ops_end;
		}

		if (fileout_util_is_fallocate()) {
			//trigger callback opened
			ret_fs = fileout_cb_call_opened(queue_idx, p_buf);
			if (ret_fs != 0) {
				DBG_ERR("Q[%d] CREATE call opened failed\r\n", queue_idx);
				goto label_fileout_tsk_do_ops_end;
			}
			//get need size for fallocate
			ret_fs = fileout_queue_get_need_size(queue_idx, &alloc_size);
			if (ret_fs != 0) {
				DBG_ERR("Q[%d] get need size opened failed\r\n", queue_idx);
				goto label_fileout_tsk_do_ops_end;
			}
		}

		if (fileout_util_is_formatfree()) {
			open_flag = FST_OPEN_EXISTING | FST_OPEN_WRITE;
		} else {
			open_flag = FST_CREATE_ALWAYS | FST_OPEN_WRITE;
		}

		if (fileout_util_is_close_on_exec()) {
			open_flag |= FST_CLOSE_ON_EXEC;
		}

		fileout_util_time_mark(&old_time);
		filehdl = FileSys_OpenFile(write_path, open_flag);
		fileout_util_time_mark(&new_time);
		if (NULL == filehdl) {
			DBG_ERR("Q[%d] CREATE %s failed\r\n", queue_idx, write_path);
			goto label_fileout_tsk_write_card_err;
		}

		//fallocate
		if (fileout_util_is_fallocate() && alloc_size > 0) {
			ret_fs = FileSys_AllocFile(filehdl, alloc_size, 0, 0);
			if (ret_fs != FST_STA_OK) {
				DBG_ERR("Alloc %s failed\r\n", write_path);
				if (FST_STA_SDIO_ERR == ret_fs) {
					goto label_fileout_tsk_write_card_err;
				}
				goto label_fileout_tsk_do_ops_end;
			}
			FILEOUT_DUMP("Q[%d] %s allocate size %lld\r\n", queue_idx, write_path, alloc_size);
		}

		ret_fs = fileout_queue_set_filehdl(queue_idx, filehdl);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] CREATE set filehdl failed\r\n", queue_idx);
			goto label_fileout_tsk_do_ops_end;
		}

		fileout_util_time_dur_ms(old_time, new_time, &dur_ms);
		fileout_util_show_opsinfo(dur_ms, ISF_FILEOUT_FOP_CREATE, p_buf, queue_idx);
	} else {
		filehdl = fileout_queue_get_filehdl(queue_idx);
		if (NULL == filehdl) {
			DBG_ERR("Q[%d] get filehdl failed\r\n", queue_idx);
			goto label_fileout_tsk_do_ops_end;
		}
	}

	//read (specific usage)
	if (p_buf->fileop & ISF_FILEOUT_FOP_READ) {
		CHAR *path = fileout_tsk_get_sysinfo(write_path[0])->path;
		FST_FILE filehdl2 = NULL;
		UINT32 file_size;
		UINT32 get_pos, put_pos;
		UINT32 use_buf = fileout_util_get_blk_addr();
		UINT32 blk_size = fileout_util_get_blk_size();
		UINT32 i, count, mod;

		DBG_IND("^G Read sysinfo %s -start\r\n", path);

		if (!fileout_util_is_usememblk()) {
			DBG_ERR(" Use MemBlk did not set\r\n");
			goto label_fileout_tsk_do_ops_end;
		}

		filehdl2 = FileSys_OpenFile(path, FST_OPEN_READ);
		if (NULL == filehdl2) {
			DBG_ERR("Q[%d] MemBlk open failed\r\n", queue_idx);
			goto label_fileout_tsk_do_ops_end;
		}

		file_size = FileSys_GetFileLen(path);
		if (file_size == 0) {
			DBG_ERR("Q[%d] MemBlk file size 0\r\n", queue_idx);
			goto label_fileout_tsk_do_ops_end;
		}
		DBG_IND("^G file_size %d\r\n", file_size);

		get_pos = write_addr;
		put_pos = (UINT32)write_pos;
		DBG_DUMP("^G get_pos %d put_pos %d\r\n", get_pos, put_pos);

		count = write_size / blk_size;
		mod   = write_size % blk_size;
		if (mod) {
			count += 1;
		}
		DBG_IND("^G count %d mod %d (%d/%d)\r\n", count, mod, write_size, blk_size);

		for (i = 0; i < count; i++) {
			ret_fs = FileSys_SeekFile(filehdl2, get_pos, FST_SEEK_SET);
			if (ret_fs != FST_STA_OK) {
				DBG_ERR(" MemBlk seek failed\r\n");
				goto label_fileout_tsk_do_ops_end;
			}

			DBG_IND("^G get_pos 0x%X\r\n", get_pos);

			if (write_size < blk_size) {
				blk_size = write_size;
			}

			if (get_pos + blk_size > file_size) {
				UINT32 size1 = file_size - get_pos;
				UINT32 size2 = blk_size - size1;
				ret_fs = FileSys_ReadFile(filehdl2, (UINT8 *)use_buf, &size1, 0, NULL);
				if (ret_fs != FST_STA_OK) {
					DBG_ERR(" ReadFile failed\r\n");
					goto label_fileout_tsk_do_ops_end;
				}
				ret_fs = FileSys_SeekFile(filehdl2, 0, FST_SEEK_SET);
				if (ret_fs != FST_STA_OK) {
					DBG_ERR(" SeekFile failed\r\n");
					goto label_fileout_tsk_do_ops_end;
				}
				DBG_IND("^G count %d back to 0\r\n", i);
				ret_fs = FileSys_ReadFile(filehdl2, (UINT8 *)(use_buf + size1), &size2, 0, NULL);
			} else {
				ret_fs = FileSys_ReadFile(filehdl2, (UINT8 *)use_buf, &blk_size, 0, NULL);
			}
			if (ret_fs != FST_STA_OK) {
				DBG_ERR(" MemBlk read failed\r\n");
				goto label_fileout_tsk_do_ops_end;
			}

			ret_fs = FileSys_WriteFile(filehdl, (UINT8 *)use_buf, &blk_size, 0, NULL);
			if (ret_fs != FST_STA_OK) {
				DBG_ERR("write failed\r\n");
				goto label_fileout_tsk_do_ops_end;
			}
#if 1 //20191221 fixed
			ret_fs = fileout_queue_written(queue_idx, blk_size);
			if (ret_fs != FST_STA_OK) {
				DBG_ERR("Q[%d] READ_WRITE %s written failed\r\n", queue_idx, write_path);
				goto label_fileout_tsk_do_ops_end;
			}
#endif
			if (get_pos + blk_size > file_size) {
				get_pos -= (file_size - blk_size);
			} else {
				get_pos += blk_size;
			}
			write_size -= blk_size;
		}

		ret_fs = FileSys_CloseFile(filehdl2);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] MemBlk close failed\r\n", queue_idx);
			goto label_fileout_tsk_do_ops_end;
		}

		DBG_IND("^G Read sysinfo %s -end\r\n", path);;
		goto label_fileout_tsk_do_ops_end;
	}

	//write (choose only 1)
	if (p_buf->fileop & ISF_FILEOUT_FOP_CONT_WRITE) {
		//error check
		if (NULL == (UINT8 *)write_addr) {
			DBG_ERR("Q[%d] CONT_WRITE write_addr error\r\n", queue_idx);
			goto label_fileout_tsk_do_ops_end;
		}

		fileout_util_time_mark(&old_time);
		if (fileout_util_is_append()) {
			UINT32 internal_tell_size = 0;
			UINT32 internal_alloc_size = 0;
			UINT32 internal_nullblk_addr = fileout_util_get_blk_addr(); //internal
			UINT32 internal_nullblk_size = fileout_util_get_blk_size(); //internal
			UINT32 config_append_size = fileout_util_get_append_size(); //from user
#if 1  //speedup
			UINT32 internal_lastblk = 0;
#endif //speedup

			// check: if tell_size >= alloc_size
			internal_tell_size = FileSys_TellFile(filehdl);
			internal_alloc_size = FileSys_GetFileLen(write_path);
			if (internal_tell_size + write_size > internal_alloc_size) {
				FILEOUT_MSG("[append]-tell=0x%lx-write=0x%lx\r\n", (unsigned long)internal_tell_size, (unsigned long)write_size);

#if 1  //speedup: special case (last blk)
				if (write_size % FILEOUT_MIN_APPEND_SIZE) {
					FILEOUT_MSG("[append]-lastblk\r\n");
					config_append_size = FILEOUT_MIN_APPEND_SIZE * ((write_size / FILEOUT_MIN_APPEND_SIZE) + 1);
					internal_lastblk = 1;
				}
#endif //speedup: special case (last blk)

				if (config_append_size < write_size) {
					//append not enough, use write_size
					config_append_size = write_size;
				}
				internal_alloc_size = internal_tell_size + config_append_size;
				FILEOUT_MSG("[append]-alloc=0x%lx-append-0x%lx\r\n", (unsigned long)internal_alloc_size, (unsigned long)config_append_size);

				//alloc
				ret_fs = FileSys_AllocFile(filehdl, internal_alloc_size, 0, 0);
				if (ret_fs != FST_STA_OK) {
					DBG_ERR("[append][%s] alloc failed\r\n", write_path);
					goto label_fileout_tsk_write_card_err;
				}

#if 1 //speedup, write wrblksize first if normal, or clear and write lastblk
				//seek back and write null data
				ret_fs = FileSys_SeekFile(filehdl, internal_tell_size, FST_SEEK_SET);
				if (ret_fs != FST_STA_OK) {
					DBG_ERR("[append][%s] seek failed\r\n", write_path);
					goto label_fileout_tsk_write_card_err;
				}
				//use writeblk as first blk
				if (internal_lastblk == 0) {
					ret_fs = FileSys_WriteFile(filehdl, (UINT8 *)write_addr, &write_size, 0, NULL);
					if (ret_fs != FST_STA_OK) {
						DBG_ERR("[append][%s] write failed\r\n", write_path);
						goto label_fileout_tsk_write_card_err;
					}
					config_append_size -= write_size;
					internal_tell_size += write_size;
				}
				if (internal_nullblk_addr) // error handling
				{
					while (config_append_size >= internal_nullblk_size) {
						ret_fs = FileSys_WriteFile(filehdl, (UINT8 *)internal_nullblk_addr, &internal_nullblk_size, 0, NULL);
						if (ret_fs != FST_STA_OK) {
							DBG_ERR("[append][%s] write1 failed\r\n", write_path);
							goto label_fileout_tsk_write_card_err;
						}
						config_append_size -= internal_nullblk_size;
					}
					if (config_append_size) {
						ret_fs = FileSys_WriteFile(filehdl, (UINT8 *)internal_nullblk_addr, &config_append_size, 0, NULL);
						if (ret_fs != FST_STA_OK) {
							DBG_ERR("[append][%s] write2 failed\r\n", write_path);
							goto label_fileout_tsk_write_card_err;
						}
					}
				}

				//seek back
				ret_fs = FileSys_SeekFile(filehdl, internal_tell_size, FST_SEEK_SET);
				if (ret_fs != FST_STA_OK) {
					DBG_ERR("[append][%s] seekb failed\r\n", write_path);
					goto label_fileout_tsk_write_card_err;
				}

				//write lastblk
				if (internal_lastblk == 1) {
					ret_fs = FileSys_WriteFile(filehdl, (UINT8 *)write_addr, &write_size, 0, NULL);
					if (ret_fs != FST_STA_OK) {
						DBG_ERR("[append][%s] write failed\r\n", write_path);
						goto label_fileout_tsk_write_card_err;
					}
				}

				internal_tell_size = FileSys_TellFile(filehdl);
				internal_alloc_size = FileSys_GetFileLen(write_path);
				FILEOUT_MSG("[append]-pos=0x%lx\r\n", (unsigned long)internal_tell_size);
				FILEOUT_MSG("[append]-len=0x%lx\r\n", (unsigned long)internal_alloc_size);

#else //clear all and then write wrblk
				//seek back and write null data
				ret_fs = FileSys_SeekFile(filehdl, internal_tell_size, FST_SEEK_SET);
				if (ret_fs != FST_STA_OK) {
					DBG_ERR("[append][%s] seek failed\r\n", write_path);
					goto label_fileout_tsk_write_card_err;
				}
				while (config_append_size >= internal_nullblk_size) {
					ret_fs = FileSys_WriteFile(filehdl, (UINT8 *)internal_nullblk_addr, &internal_nullblk_size, 0, NULL);
					if (ret_fs != FST_STA_OK) {
						DBG_ERR("[append][%s] write1 failed\r\n", write_path);
						goto label_fileout_tsk_write_card_err;
					}
					config_append_size -= internal_nullblk_size;
				}
				if (config_append_size) {
					ret_fs = FileSys_WriteFile(filehdl, (UINT8 *)internal_nullblk_addr, &config_append_size, 0, NULL);
					if (ret_fs != FST_STA_OK) {
						DBG_ERR("[append][%s] write2 failed\r\n", write_path);
						goto label_fileout_tsk_write_card_err;
					}
				}

				//seek back
				ret_fs = FileSys_SeekFile(filehdl, internal_tell_size, FST_SEEK_SET);
				if (ret_fs != FST_STA_OK) {
					DBG_ERR("[append][%s] seek failed\r\n", write_path);
					goto label_fileout_tsk_write_card_err;
				}

				ret_fs = FileSys_WriteFile(filehdl, (UINT8 *)write_addr, &write_size, 0, NULL);

				internal_tell_size = FileSys_TellFile(filehdl);
				internal_alloc_size = FileSys_GetFileLen(write_path);
				DBG_DUMP("ur_pos-0x%lx\r\n", (unsigned long)internal_tell_size);
				DBG_DUMP("ur_alloc-0x%lx\r\n", (unsigned long)internal_alloc_size);
#endif
			} else {
				ret_fs = FileSys_WriteFile(filehdl, (UINT8 *)write_addr, &write_size, 0, NULL);
			}
		} else {
			ret_fs = FileSys_WriteFile(filehdl, (UINT8 *)write_addr, &write_size, 0, NULL);
		}
		fileout_util_time_mark(&new_time);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] CONT_WRITE %s failed\r\n", queue_idx, write_path);
			if (FST_STA_SDIO_ERR == ret_fs) {
				goto label_fileout_tsk_write_card_err;
			}
			goto label_fileout_tsk_do_ops_end;
		}

		ret_fs = fileout_queue_written(queue_idx, write_size);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] CONT_WRITE %s written failed\r\n", queue_idx, write_path);
			goto label_fileout_tsk_do_ops_end;
		}

		fileout_util_time_dur_ms(old_time, new_time, &dur_ms);
		fileout_util_show_opsinfo(dur_ms, ISF_FILEOUT_FOP_CONT_WRITE, p_buf, queue_idx);
	} else if (p_buf->fileop & ISF_FILEOUT_FOP_SEEK_WRITE) {

		alloc_size = FileSys_TellFile(filehdl);

		ret_fs = FileSys_SeekFile(filehdl, write_pos, FST_SEEK_SET);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] SEEK_WRITE %s failed, pos %llu\r\n", queue_idx, write_path, write_pos);
			if (FST_STA_SDIO_ERR == ret_fs) {
				goto label_fileout_tsk_write_card_err;
			}
			goto label_fileout_tsk_do_ops_end;
		}

		fileout_util_time_mark(&old_time);
		//error check
		if (NULL == (UINT8 *)write_addr) {
			DBG_ERR("Q[%d] SEEK_WRITE write_addr error\r\n", queue_idx);
			goto label_fileout_tsk_do_ops_end;
		}
		ret_fs = FileSys_WriteFile(filehdl, (UINT8 *)write_addr, &write_size, 0, NULL);
		fileout_util_time_mark(&new_time);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] SEEK_WRITE %s failed\r\n", queue_idx, write_path);
			if (FST_STA_SDIO_ERR == ret_fs) {
				goto label_fileout_tsk_write_card_err;
			}
			goto label_fileout_tsk_do_ops_end;
		}

		ret_fs = FileSys_SeekFile(filehdl, alloc_size, FST_SEEK_SET);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] SEEK_WRITE %s failed, pos %llu\r\n", queue_idx, write_path, alloc_size);
			if (FST_STA_SDIO_ERR == ret_fs) {
				goto label_fileout_tsk_write_card_err;
			}
			goto label_fileout_tsk_do_ops_end;
		}

		//#NT#2020/09/22#Willy Su -begin
		//#NT# Since file_size is not added, write_size of SEEK_WRITE is not calculated
		#if 0
		ret_fs = fileout_queue_written(queue_idx, write_size);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] SEEK_WRITE %s written failed\r\n", queue_idx, write_path);
			goto label_fileout_tsk_do_ops_end;
		}
		#endif
		//#NT#2020/09/22#Willy Su -end

		fileout_util_time_dur_ms(old_time, new_time, &dur_ms);
		fileout_util_show_opsinfo(dur_ms, ISF_FILEOUT_FOP_SEEK_WRITE, p_buf, queue_idx);
	}

	//flush
	if (p_buf->fileop & ISF_FILEOUT_FOP_FLUSH) {

		if (p_buf->fileop & ISF_FILEOUT_FOP_CLOSE || (slowcard_is_on == 1)) {
			FILEOUT_DUMP("Q[%d] FLUSH skip\r\n", queue_idx);
		} else {

			fileout_util_time_mark(&old_time);
			ret_fs = FileSys_FlushFile(filehdl);
			fileout_util_time_mark(&new_time);
			if (ret_fs != FST_STA_OK) {
				DBG_ERR("Q[%d] FLUSH %s failed\r\n", queue_idx, write_path);
				if (FST_STA_SDIO_ERR == ret_fs) {
					goto label_fileout_tsk_write_card_err;
				}
				goto label_fileout_tsk_do_ops_end;
			}

			fileout_util_time_dur_ms(old_time, new_time, &dur_ms);
			fileout_util_show_opsinfo(dur_ms, ISF_FILEOUT_FOP_FLUSH, p_buf, queue_idx);
		}
	}

	//close
	if (p_buf->fileop & ISF_FILEOUT_FOP_CLOSE) {

		fileout_util_time_mark(&old_time);
		ret_fs = FileSys_CloseFile(filehdl);
		fileout_util_time_mark(&new_time);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] CLOSE %s failed\r\n", queue_idx, write_path);
			if (FST_STA_SDIO_ERR == ret_fs) {
				goto label_fileout_tsk_write_card_err;
			}
			goto label_fileout_tsk_do_ops_end;
		}
		filehdl = NULL;

		ret_fs = fileout_queue_set_filehdl(queue_idx, filehdl);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] CLOSE set filehdl failed\r\n", queue_idx);
			goto label_fileout_tsk_do_ops_end;
		}

		ret_fs = fileout_cb_call_closed(queue_idx, p_buf);
		if (ret_fs != 0) {
			DBG_ERR("Q[%d] CLOSE call closed failed\r\n", queue_idx);
			goto label_fileout_tsk_do_ops_end;
		}

		ret_fs = fileout_queue_written(queue_idx, 0);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] CLOSE %s written failed\r\n", queue_idx, write_path);
			goto label_fileout_tsk_do_ops_end;
		}

		fileout_util_time_dur_ms(old_time, new_time, &dur_ms);
		fileout_util_show_opsinfo(dur_ms, ISF_FILEOUT_FOP_CLOSE, p_buf, queue_idx);
	}

	//show size info
	fileout_util_show_quesize();

	//process end (success)
	ret_fs = 0;
	fileout_util_time_delay_ms();

label_fileout_tsk_do_ops_end:
	fileout_ops_unlock();
	if (fileout_util_is_skip_ops(queue_idx)) {
		if (p_buf->fileop & ISF_FILEOUT_FOP_CLOSE) {
			ret_fs = fileout_cb_call_closed(queue_idx, p_buf);
			if (ret_fs != 0) {
				DBG_ERR("Q[%d] CLOSE call closed failed\r\n", queue_idx);
			}
		}
	}
	if (ret_fs != FST_STA_OK) {

		return -1;
	}

	return 0;

label_fileout_tsk_write_card_err:
	fileout_ops_unlock();
	//callback error code
	if (0 != fileout_cb_call_fs_err(queue_idx, p_buf, FILEOUT_CB_ERRCODE_CARD_WR_ERR)) {
		DBG_ERR("wrtie card callback error\r\n");
	}
	//try to close file if necessary
	if (p_buf->fileop & ISF_FILEOUT_FOP_CLOSE) {

		fileout_util_time_mark(&old_time);
		ret_fs = FileSys_CloseFile(filehdl);
		fileout_util_time_mark(&new_time);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] CLOSE %s failed\r\n", queue_idx, write_path);
		}
		filehdl = NULL;

		ret_fs = fileout_queue_set_filehdl(queue_idx, filehdl);
		if (ret_fs != FST_STA_OK) {
			DBG_ERR("Q[%d] CLOSE set filehdl failed\r\n", queue_idx);
		}

		ret_fs = fileout_cb_call_closed(queue_idx, p_buf);
		if (ret_fs != 0) {
			DBG_ERR("Q[%d] CLOSE call closed failed\r\n", queue_idx);
		}

		fileout_util_time_dur_ms(old_time, new_time, &dur_ms);
		fileout_util_show_opsinfo(dur_ms, ISF_FILEOUT_FOP_CLOSE, p_buf, queue_idx);
	}
	return -1;
}

static THREAD_RETTYPE ISF_FileOut_Tsk_Func(CHAR drive)
{
	FILEOUT_CTRL_OBJ *p_ctrlobj;
    FLGPTN ret_flgptn = 0;
	ISF_FILEOUT_BUF cur_buf = {0};
	CHAR fpath[FILEOUT_PATH_LEN];
	INT32 ret = 0;
	INT32 ret_qidx = 0;
	INT32 last_qidx = 0;

    THREAD_ENTRY();
    DBG_FUNC_BEGIN("[TSK]\r\n");

	p_ctrlobj = fileout_get_ctrl_obj(drive);
	if (NULL == p_ctrlobj) {
		DBG_ERR("Get ctrl obj failed, drive %c\r\n", drive);
		goto label_fileout_tsk_func_end;
	}

	vos_flag_clr(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_STOPPED);
	vos_flag_set(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_READY);
	DBG_IND("Drive %c FILEOUT_FLG_TSK_READY\r\n", drive);

    while (1) {
        PROFILE_TASK_IDLE();
		DBG_IND("Drive %c tsk wait trigger\r\n", drive);
		// vos_flag_wait is identical to vos_flag_wait_interruptible
        vos_flag_wait(&ret_flgptn, p_ctrlobj->FlagID, FILEOUT_FLG_TSK_OPS_IN | FILEOUT_FLG_TSK_STOP | FILEOUT_FLG_TSK_RELQUE, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
        DBG_IND("ret_flgptn = 0x%x\r\n", ret_flgptn);

        if (ret_flgptn & FILEOUT_FLG_TSK_STOP) {
			DBG_IND("Drive %c FILEOUT_FLG_TSK_STOP\r\n", drive);
            break;
        }

		if (ret_flgptn & FILEOUT_FLG_TSK_OPS_IN) {
			DBG_IND("Drive %c FILEOUT_FLG_TSK_OPS_IN\r\n", drive);

			if (fileout_util_is_wait_ready()) {
				DBG_DUMP("--- Not Write File Until Ready ---\r\n");
				continue;
			}

			while ((ret_qidx = fileout_queue_pick_idx(drive, last_qidx)) >= 0) {

				if (-1 == fileout_tsk_set_last_qidx(drive, ret_qidx)) {
					DBG_ERR("Set tsk qidx failed\r\n");
					break;
				}

				//loop until all queues are empty
				memset(&cur_buf, 0, sizeof(cur_buf));
				memset(fpath, 0, sizeof(fpath));
				cur_buf.p_fpath = fpath;
				cur_buf.fpath_size = sizeof(fpath);

				ret = fileout_queue_pop(&cur_buf, ret_qidx);
				if (ret != 0) {
					DBG_ERR("FileOut queue pop failed\r\n");
					break;
				}

				ret = fileout_tsk_do_ops(&cur_buf, ret_qidx);
				if (ret == 0) {
					cur_buf.ret_push = ISF_OK;
				}
				else {
					DBG_ERR("FileOut do ops failed -B\r\n");
					fileout_util_show_bufinfo(&cur_buf);
					DBG_ERR("FileOut do ops failed -E\r\n");
					cur_buf.ret_push = ISF_ERR_FAILED;
				}

				if (cur_buf.fp_pushed) {
					DBG_IND("fp_pushed 0x%X before\r\n", cur_buf.fp_pushed);
					cur_buf.fp_pushed(&cur_buf);
					DBG_IND("fp_pushed 0x%X after\r\n", cur_buf.fp_pushed);
				}

				if (-1 == fileout_tsk_get_last_qidx(drive, &last_qidx)) {
					DBG_ERR("Get tsk qidx failed\r\n");
					break;
				}
			}
		}

		if (ret_flgptn & FILEOUT_FLG_TSK_RELQUE) {
			DBG_IND("Drive %c FILEOUT_FLG_TSK_RELQUE\r\n", drive);
			if (-1 != fileout_port_need_to_rel(drive)) {
				DBG_IND("Port release success\r\n");
			}
		}
    }

	vos_flag_clr(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_READY);
	vos_flag_set(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_STOPPED);

label_fileout_tsk_func_end:
    DBG_FUNC_END("[TSK]\r\n");
    THREAD_RETURN(0);
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void ISF_FileOut_Tsk_A(void)
{
    ISF_FileOut_Tsk_Func('A');
    return;
}

void ISF_FileOut_Tsk_B(void)
{
    ISF_FileOut_Tsk_Func('B');
    return;
}

void fileout_tsk_start(UINT32 drive)
{
	FILEOUT_CTRL_OBJ *p_ctrlobj;

	p_ctrlobj = fileout_get_ctrl_obj(drive);
	if (NULL == p_ctrlobj) {
		DBG_ERR("Get ctrl obj failed, drive %c\r\n", drive);
		return;
	}

	if (g_tsk_func_count[(drive - FILEOUT_DRV_NAME_FIRST)] != 0) {
		return;
	}

	if (0 == vos_flag_chk(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_READY)) {
		if ((drive - FILEOUT_DRV_NAME_FIRST) == 0) {
			p_ctrlobj->TaskID = vos_task_create(ISF_FileOut_Tsk_A, 0, "ISF_FileOut_Tsk_A", PRI_ISF_FILEOUT, STKSIZE_ISF_FILEOUT);
		} else {
			p_ctrlobj->TaskID = vos_task_create(ISF_FileOut_Tsk_B, 0, "ISF_FileOut_Tsk_B", PRI_ISF_FILEOUT, STKSIZE_ISF_FILEOUT);
		}
		THREAD_RESUME(p_ctrlobj->TaskID);
		g_tsk_func_count[(drive - FILEOUT_DRV_NAME_FIRST)] = 1;
	}
}

void fileout_tsk_destroy(UINT32 drive)
{
	FILEOUT_CTRL_OBJ *p_ctrlobj;
	UINT32 fout_status = TRUE;
	UINT32 delay_cnt = 50;

	p_ctrlobj = fileout_get_ctrl_obj(drive);
	if (NULL == p_ctrlobj) {
		DBG_ERR("Get ctrl obj failed, drive %c\r\n", drive);
		return;
	}

	if (g_tsk_func_count[(drive - FILEOUT_DRV_NAME_FIRST)] != 1) {
		return;
	}

	vos_flag_set(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_STOP);

	while (fout_status) {
		if (0 == vos_flag_chk(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_READY)) {
			fout_status = FALSE;
			DBG_DUMP("task is stopped\r\n");
		} else {
			DBG_WRN("task is not stopped\r\n");
		}
		if (fout_status == FALSE) break;
		vos_util_delay_ms(10);
		delay_cnt--;
		if (delay_cnt == 0) {
			fout_status = FALSE;
		}
	}

	if (0 == vos_flag_chk(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_STOPPED)) {
		DBG_DUMP("FileOut vos_task_destroy (%x)\r\n", p_ctrlobj->TaskID);
		vos_task_destroy(p_ctrlobj->TaskID);
	}
	g_tsk_func_count[(drive - FILEOUT_DRV_NAME_FIRST)] = 0;
	return;
}

void fileout_tsk_destroy_multi(void)
{
	UINT32 drive;
	FILEOUT_CTRL_OBJ *p_ctrlobj;
	UINT32 fout_status = TRUE;
	UINT32 delay_cnt = 50;

	for (drive = FILEOUT_DRV_NAME_FIRST; drive <= FILEOUT_DRV_NAME_LAST; drive++) {
		p_ctrlobj = fileout_get_ctrl_obj(drive);
		if (NULL == p_ctrlobj) {
			continue;
		}

		if (g_tsk_func_count[(drive - FILEOUT_DRV_NAME_FIRST)] != 1) {
			continue;
		}

		vos_flag_set(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_STOP);
	}

	while (fout_status) {
		for (drive = FILEOUT_DRV_NAME_FIRST; drive <= FILEOUT_DRV_NAME_LAST; drive++) {
			p_ctrlobj = fileout_get_ctrl_obj(drive);
			if (NULL == p_ctrlobj) {
				continue;
			}
			if (0 == vos_flag_chk(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_READY)) {
				continue;
			} else {
				break;
			}
		}
		if (drive == (FILEOUT_DRV_NAME_FIRST + FILEOUT_DRV_NUM)) {
			break;
		}
		vos_util_delay_ms(10);
		delay_cnt--;
		if (delay_cnt == 0) {
			fout_status = FALSE;
		}
	}

	for (drive = FILEOUT_DRV_NAME_FIRST; drive <= FILEOUT_DRV_NAME_LAST; drive++) {
		p_ctrlobj = fileout_get_ctrl_obj(drive);
		if (NULL != p_ctrlobj) {
			if (0 == vos_flag_chk(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_STOPPED)) {
				DBG_DUMP("FileOut vos_task_destroy (%x)\r\n", p_ctrlobj->TaskID);
				vos_task_destroy(p_ctrlobj->TaskID);
			}
		}
		g_tsk_func_count[(drive - FILEOUT_DRV_NAME_FIRST)] = 0;
	}
	return;
}

INT32 fileout_tsk_trigger_ops(CHAR drive)
{
	FILEOUT_CTRL_OBJ *p_ctrlobj;

	p_ctrlobj = fileout_get_ctrl_obj(drive);
	if (NULL == p_ctrlobj) {
		DBG_ERR("Get ctrl obj failed, drive 0x%X\r\n", (UINT32)drive);
		return -1;
	}

	if (0 == vos_flag_chk(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_READY)) {
		DBG_WRN("Drive %c task is not ready\r\n", drive);
	}

	vos_flag_set(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_OPS_IN);
	return 0;
}

INT32 fileout_tsk_trigger_relque(CHAR drive)
{
	FILEOUT_CTRL_OBJ *p_ctrlobj;

	p_ctrlobj = fileout_get_ctrl_obj(drive);
	if (NULL == p_ctrlobj) {
		DBG_ERR("Get ctrl obj failed, drive 0x%X\r\n", (UINT32)drive);
		return -1;
	}

	if (0 == vos_flag_chk(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_READY)) {
		DBG_WRN("Drive %c task is not ready\r\n", drive);
	}

	vos_flag_set(p_ctrlobj->FlagID, FILEOUT_FLG_TSK_RELQUE);
	return 0;
}

INT32 fileout_tsk_sysinfo_init(CHAR drive)
{
	FILEOUT_SYSINFO *p_sys_info = NULL;
	FST_FILE filehdl = NULL;
	UINT32 ret;

	p_sys_info = fileout_tsk_get_sysinfo(drive);
	if (NULL == p_sys_info) {
		DBG_ERR("Get sysinfo failed, drive 0x%X\r\n", (UINT32)drive);
		return -1;
	}

	if (p_sys_info->is_alloc == 0) {
		filehdl = FileSys_OpenFile(p_sys_info->path, FST_CREATE_HIDDEN | FST_OPEN_WRITE);
		if (filehdl == NULL) {
			DBG_ERR("allocate failed\r\n");
			return -1;
		} else {
			ret = FileSys_AllocFile(filehdl, p_sys_info->alloc_size, 0, 0);
			if (ret != FST_STA_OK) {
				DBG_ERR("sysinfo alloc failed, drive 0x%X\r\n", (UINT32)drive);
				return -1;
			}
			ret = FileSys_CloseFile(filehdl);
			if (ret != FST_STA_OK) {
				DBG_ERR("sysinfo close failed, drive 0x%X\r\n", (UINT32)drive);
				return -1;
			}
		}
		filehdl = FileSys_OpenFile(p_sys_info->path, FST_OPEN_EXISTING | FST_OPEN_WRITE);
		if (NULL == filehdl) {
			DBG_ERR("filehdl NULL, %s\r\n", p_sys_info->path);
			return -1;
		} else {
			ret = FileSys_SeekFile(filehdl, 0, FST_SEEK_SET);
			p_sys_info->filehdl = filehdl;
			p_sys_info->is_alloc = 1;
			if (ret != FST_STA_OK) {
				DBG_ERR("sysinfo seek 0 failed, drive 0x%X\r\n", (UINT32)drive);
				return -1;
			}
		}
		DBG_DUMP("^G ALLOCATE SYSINFO DONE\r\n");
		DBG_DUMP("^G SYSINFO SIZE: 0x%X\r\n", p_sys_info->alloc_size);
	}
	if (p_sys_info->is_alloc == 1) {
		filehdl = p_sys_info->filehdl;
		if (NULL == filehdl) {
			DBG_ERR("filehdl NULL\r\n");
			return -1;
		} else {
			ret = FileSys_SeekFile(filehdl, 0, FST_SEEK_SET);
			if (ret != FST_STA_OK) {
				DBG_ERR("sysinfo seek 0 failed, drive 0x%X\r\n", (UINT32)drive);
				return -1;
			}
		}
		DBG_DUMP("^G SEEK0 SYSINFO DONE\r\n");
	}

	return 0;
}

INT32 fileout_tsk_sysinfo_uninit(CHAR drive)
{
	FILEOUT_SYSINFO *p_sys_info = NULL;
	FST_FILE filehdl = NULL;
	UINT32 ret;

	p_sys_info = fileout_tsk_get_sysinfo(drive);
	if (NULL == p_sys_info) {
		DBG_ERR("Get sysinfo failed, drive 0x%X\r\n", (UINT32)drive);
		return -1;
	}

	if (p_sys_info->is_alloc == 1) {
		filehdl = p_sys_info->filehdl;
		if (filehdl == NULL) {
			DBG_DUMP("sysinfo already released, drive 0x%X\r\n", (UINT32)drive);
		} else {
			ret = FileSys_CloseFile(filehdl);
			filehdl = NULL;
			p_sys_info->filehdl = filehdl;
			if (ret != FST_STA_OK) {
				DBG_ERR("sysinfo close failed, drive 0x%X\r\n", (UINT32)drive);
				return -1;
			}
		}
		p_sys_info->is_alloc = 0;
		DBG_DUMP("^G FREE SYSINFO DONE\r\n");
	}

	return 0;
}

INT32 fileout_tsk_init(void)
{
	memset((void *)g_tsk_info_last_qidx, 0, sizeof(INT32)*FILEOUT_DRV_NUM);
	return 0;
}

