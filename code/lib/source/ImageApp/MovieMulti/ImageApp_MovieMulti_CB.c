#include "ImageApp_MovieMulti_int.h"
#include "FileSysTsk.h"
#include "NamingRule/NameRule_Custom.h"

#define MOVIE_PATH_INFO_MAX_NUM 2
MOVIE_CB_PATH_INFO g_movie_path_info[HD_LIB_FILEOUT_COUNT][2] = {0};

///// Cross file variables
#if (IAMOVIE_BRANCH != IAMOVIE_BR_1_25)
UINT32 iamovie_movie_path_info = FALSE;
#else
UINT32 iamovie_movie_path_info = TRUE;
#endif
MovieUserEventCb *g_ia_moviemulti_usercb = NULL;
UINT32            g_vdo_rec_sec = 0; //store the current record second

///// Noncross file variables
static MOVIE_CB_CRASH_INFO   g_crash_info[MAX_IMG_PATH * 2 + MAX_ETHCAM_PATH * 1] = {0};
static CHAR                  g_isf_dvcam_file_name[80]                            = {0};
static CHAR                  g_isf_dvcam_full_path[80]                            = {0};

MOVIE_CB_CRASH_INFO *_MovieMulti_Get_CrashInfo(UINT32 idx)
{
	if (idx < MAX_IMG_PATH * 2 + MAX_ETHCAM_PATH * 1) {
		return &(g_crash_info[idx]);
	}
	return NULL;
}

ER ImageApp_MovieMulti_RegUserCB(MovieUserEventCb *fncb)
{
	g_ia_moviemulti_usercb = fncb;

	return E_OK;
}

static MOVIE_CB_PATH_INFO *_MovieMulti_Get_Path_Info(UINT32 id, UINT32 status)
{
	MOVIE_CB_PATH_INFO *p_info = NULL;
	UINT32 num = 0;
	p_info = &(g_movie_path_info[id][0]);
	while (num < MOVIE_PATH_INFO_MAX_NUM) {
		if (p_info->status == (status - 1)) { //last step
			DBG_IND("GetPathInfo: [%d][status=%d][last=%d] return\r\n", id, status, p_info->status);
			return p_info;
		} else {
			DBG_IND("GetPathInfo: [%d][status=%d][last=%d]\r\n", id, status, p_info->status);
		}
		p_info++;
		num++;
	}
	return NULL;
}

static INT32 _MovieMulti_Path_Handle(UINT32 path_id, MOVIE_CB_PATH_INFO *p_info)
{
	MOVIE_CB_PATH_INFO *p_info_inner = NULL;
	UINT32 status = p_info->status;

	p_info_inner = _MovieMulti_Get_Path_Info(path_id, status);
	if (NULL == p_info_inner) {
		DBG_ERR("p_info_inner null\r\n");
		return -1;
	}

	if (status == CB_PATH_STATUS_RECORD)//need status closed
	{
		// naming (input p_fpath)
		p_info_inner->p_fpath = p_info->p_fpath;
		p_info_inner->status = p_info->status;
		DBG_DUMP("handle: (%s) recording\r\n", p_info_inner->p_fpath);
	}
	else if (status == CB_PATH_STATUS_CUT)//need status record
	{
		// cut complete (input duration)
		p_info_inner->duration = p_info->duration;
		p_info_inner->status = p_info->status;
		DBG_DUMP("handle: (%d) cutting\r\n", p_info_inner->duration);
	}
	else if (status == CB_PATH_STATUS_CLOSE)//need status cut
	{
		// closed (output duration)
		p_info->duration = p_info_inner->duration;
		p_info_inner->status = CB_PATH_STATUS_INIT;
		DBG_DUMP("handle: (%s:%d) closed\r\n", p_info->p_fpath, p_info->duration);
	}
	return 0;
}

static INT32 _MovieMulti_BsMux_CB_Result(void *p_evdata)
{
	HD_BSMUX_CBINFO *p_data = (HD_BSMUX_CBINFO *)p_evdata;
	HD_BSMUX_ERRCODE errcode = p_data->errcode;
	UINT32 id = p_data->id;

	switch(errcode) {
	case HD_BSMUX_ERRCODE_NONE:
		break;
	case HD_BSMUX_ERRCODE_SLOWMEDIA:
		DBG_DUMP("<== BSMUX SLOW MEDIA ==>\r\n");
		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(id, MOVIE_USER_CB_ERROR_CARD_SLOW, 0);
		}
		break;
	case HD_BSMUX_ERRCODE_LOOPREC_FULL:
		DBG_DUMP("<== BSMUX LOOPREC FULL ==>\r\n");
		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(id, MOVIE_USER_CB_ERROR_SEAMLESS_REC_FULL, 0);
		}
		break;
	case HD_BSMUX_ERRCODE_OVERTIME:
		DBG_DUMP("<== BSMUX OVERTIME ==>\r\n");
		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(id, MOVIE_USER_CB_EVENT_OVERTIME, 0);
		}
		break;
	case HD_BSMUX_ERRCODE_MAXSIZE:
		DBG_DUMP("<== BSMUX MAXSIZE ==>\r\n");
		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(id, MOVIE_USER_CB_ERROR_SEAMLESS_REC_FULL, 0);
		}
		break;
	case HD_BSMUX_ERRCODE_VANOTSYNC:
		DBG_IND("<== V/A Not Sync ==>\r\n");
		break;
	case HD_BSMUX_ERRCODE_GOPMISMATCH:
		DBG_DUMP("<== GOP MISMATCH ==>\r\n");
		break;
	case HD_BSMUX_ERRCODE_PROCDATAFAIL:
		DBG_DUMP("<== PROC DATA FAIL ==>\r\n");
		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(id, MOVIE_USER_CB_ERROR_PROC_DATA_FAIL, 0);
		}
		break;
	default:
		DBG_ERR("Invalid errcode id 0x%X\r\n", errcode);
		break;
	}

	return 0;
}

INT32 _ImageApp_MovieMulti_BsMux_CB(CHAR *p_name, HD_BSMUX_CBINFO *cbinfo, UINT32 *param)
{
	HD_BSMUX_CB_EVENT event = cbinfo->cb_event;
	HD_PATH_ID path_id = (HD_PATH_ID)cbinfo->out_data->pathID;
	HD_RESULT rv;
	ER ret;

	switch (event) {
	case HD_BSMUX_CB_EVENT_PUTBSDONE: {
			UINT32 need_process_callback = TRUE;
			UINT32 type = cbinfo->out_data->resv[2];
			if (type == MAKEFOURCC('V','S','T','M')) {
				UINT32 vfr = 0;
				MOVIE_TBL_IDX tb;
				vos_flag_set(IAMOVIE_FLG_BSMUX_ID, (1 << cbinfo->id));
				if (_ImageApp_MovieMulti_BsPortGetTableIndex(cbinfo->id, &tb) == E_OK) {
					if (tb.link_type == LINKTYPE_REC) {
						vfr = ImgLinkRecInfo[tb.idx][tb.tbl].frame_rate;
						if (img_bsmux_2v1a_mode[tb.idx] && cbinfo->out_data->resv[5] == 1) {    // resv[5]: 0 indicates main path and 1 indicates sub path
							need_process_callback = FALSE;
						} else {
							if (img_bsmux_cutnow_with_period_cnt[tb.idx][tb.tbl * 2] && (img_bsmux_cutnow_with_period_cnt[tb.idx][tb.tbl * 2] != UNUSED)) {
								img_bsmux_cutnow_with_period_cnt[tb.idx][tb.tbl * 2]--;
								if (img_bsmux_cutnow_with_period_cnt[tb.idx][tb.tbl * 2] == 0) {
									img_bsmux_cutnow_with_period_cnt[tb.idx][tb.tbl * 2] = UNUSED;
									if ((ret = ImageApp_MovieMulti_SetCutNow(ImageApp_MovieMulti_BsPort2Recid(cbinfo->id))) != E_OK) {
										DBG_ERR("ImageApp_MovieMulti_SetCutNow failed(%d)\r\n", ret);
									}
								}
							}
						}
					} else if (tb.link_type == LINKTYPE_ETHCAM) {
						vfr = EthCamLinkRecInfo[tb.idx].vfr;
						if (ethcam_bsmux_cutnow_with_period_cnt[tb.idx][0] && (ethcam_bsmux_cutnow_with_period_cnt[tb.idx][0] != UNUSED)) {
							ethcam_bsmux_cutnow_with_period_cnt[tb.idx][0]--;
							if (ethcam_bsmux_cutnow_with_period_cnt[tb.idx][0] == 0) {
								ethcam_bsmux_cutnow_with_period_cnt[tb.idx][0] = UNUSED;
								if ((ret = ImageApp_MovieMulti_SetCutNow(ImageApp_MovieMulti_BsPort2Recid(cbinfo->id))) != E_OK) {
									DBG_ERR("ImageApp_MovieMulti_SetCutNow failed(%d)\r\n", ret);
								}
							}
						}
					}
					if (vfr && BsMuxFrmCnt[cbinfo->id] % vfr == 0) {
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == DISABLE)
						if (cbinfo->id == FirstBsMuxPath && g_ia_moviemulti_usercb && need_process_callback) {
							UINT32 sec = 0;
							UINT32 is_crash_ext = ImageApp_MovieMulti_IsExtendCrash(ImageApp_MovieMulti_BsPort2Recid(cbinfo->id));
							UINT32 is_crash_ext_cur = 0;
							if (tb.link_type == LINKTYPE_REC) {
								is_crash_ext_cur = g_crash_info[2 * tb.idx + tb.tbl].is_crash_curr;
							} else if (tb.link_type == LINKTYPE_ETHCAM) {
								is_crash_ext_cur = g_crash_info[2 * MaxLinkInfo.MaxImgLink + tb.idx].is_crash_curr;
							}
							sec = BsMuxFrmCnt[cbinfo->id] / vfr;
							if (is_crash_ext && is_crash_ext_cur) {
								// ext_crash now
							} else {
								if (ImgLinkFileInfo.end_type == MOVREC_ENDTYPE_CUTOVERLAP || ImgLinkFileInfo.end_type == MOVREC_ENDTYPE_CUT_TILLCARDFULL) {
									sec %= ImgLinkFileInfo.seamless_sec;
								}
							}
							g_vdo_rec_sec = sec;
							if (tb.link_type == LINKTYPE_REC) {
								if ((tb.tbl == MOVIETYPE_MAIN) && (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP)) {
									if (img_emr_loop_start[tb.idx][tb.tbl] == FALSE) {
										sec = 0;
									}
								} else if ((tb.tbl == MOVIETYPE_CLONE) && (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_LOOP)) {
									if (img_emr_loop_start[tb.idx][tb.tbl] == FALSE) {
										sec = 0;
									}
								}
							} else if (tb.link_type == LINKTYPE_ETHCAM) {
								if (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP) {
									if (ethcam_emr_loop_start[tb.idx][0] == FALSE) {
										sec = 0;
									}
								}
							}
							g_ia_moviemulti_usercb(cbinfo->id, MOVIE_USER_CB_EVENT_REC_ONE_SECOND, sec);
						}
#endif
					}
				}
				if (need_process_callback) {
					BsMuxFrmCnt[cbinfo->id]++;
				}
			}
			break;
		}

	case HD_BSMUX_CB_EVENT_COPYBSBUF: {
			break;
		}

	case HD_BSMUX_CB_EVENT_FOUTREADY: {
			HD_FILEOUT_BUF *fout_buf = (HD_FILEOUT_BUF *)cbinfo->out_data;
			if (iamovie_encrypt_type & MOVIEMULTI_ENCRYPT_TYPE_CONTAINER) {
				if ((fout_buf->type == MEDIA_FILEFORMAT_MP4 || fout_buf->type == MEDIA_FILEFORMAT_MOV) && (fout_buf->fileop & HD_BSMUX_FOP_CLOSE)) {
					_ImageApp_MovieMulti_Crypto(fout_buf->addr, IAMOVIE_CRYPTO_SIZE);
				}
			}
			rv = hd_fileout_push_in_buf(path_id, fout_buf, -1);
			if (rv != HD_OK) {
				DBG_WRN("fileout push_in_buf fail\r\n");
			} else {
				DBG_IND("fileout push_in_buf done\r\n");
			}
			break;
		}

	case HD_BSMUX_CB_EVENT_KICKTHUMB: {
			break;
		}

	case HD_BSMUX_CB_EVENT_CUT_BEGIN: {
			MOVIE_TBL_IDX tb;
			path_id = cbinfo->id;
			if (_ImageApp_MovieMulti_BsPortGetTableIndex(path_id, &tb) == E_OK) {
#if (_IAMOVIE_ONE_SEC_CB_USING_VENC == DISABLE)
				if (tb.link_type == LINKTYPE_REC) {
					BsMuxFrmCnt[path_id] %= ImgLinkRecInfo[tb.idx][tb.tbl].frame_rate;
					if ((tb.tbl == MOVIETYPE_MAIN) && (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_PAUSE)) {
						if (path_id % 4 == 1) {
							BsMuxFrmCnt[path_id-1] %= ImgLinkRecInfo[tb.idx][tb.tbl].frame_rate;
						}
					} else if ((tb.tbl == MOVIETYPE_CLONE) && (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_PAUSE)) {
						if (path_id % 4 == 3) {
							BsMuxFrmCnt[path_id-1] %= ImgLinkRecInfo[tb.idx][tb.tbl].frame_rate;
						}
					}
				} else if (tb.link_type == LINKTYPE_ETHCAM) {
					BsMuxFrmCnt[path_id] %= EthCamLinkRecInfo[tb.idx].vfr;
					if (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_PAUSE) {
						if (path_id % 2 == 1) {
							BsMuxFrmCnt[path_id-1] %= EthCamLinkRecInfo[tb.idx].vfr;
						}
					}
				}
#else
				if (tb.link_type == LINKTYPE_REC) {
					if (((tb.tbl == MOVIETYPE_MAIN) && (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_PAUSE)) ||
						((tb.tbl == MOVIETYPE_CLONE) && (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_PAUSE))) {
						img_venc_frame_cnt_clr[tb.idx][tb.tbl] = 1;
					}
				}
#endif
			}
			if (iamovie_movie_path_info == TRUE) {
				UINT32 frame_num = cbinfo->out_data->resv[4];
				UINT32 frame_rate = 0;
				UINT32 dur_sec = 0;
				if (_ImageApp_MovieMulti_BsPortGetTableIndex(path_id, &tb) == E_OK) {
					if (tb.link_type == LINKTYPE_REC) {
						frame_rate = ImgLinkRecInfo[tb.idx][tb.tbl].frame_rate;
					} else if (tb.link_type == LINKTYPE_ETHCAM) {
						frame_rate = EthCamLinkRecInfo[tb.idx].vfr;
					}
					if (frame_rate) {
						dur_sec = (frame_num / frame_rate);
					} else {
						DBG_ERR("[%d][%d] frame_rate zero\r\n", tb.idx, tb.tbl);
					}
					DBG_DUMP("[%d] frame_num=%d frame_rate=%d\r\n", path_id, frame_num, frame_rate);
					MOVIE_CB_PATH_INFO path_info = {0};
					path_info.duration = dur_sec;
					path_info.status = CB_PATH_STATUS_CUT;
					if (_MovieMulti_Path_Handle(path_id, &path_info) != E_OK) {
						DBG_ERR("[%d] handle cut failed\r\n", path_id);
					} else {
						DBG_IND("[%d] handle cut (duration=%d)\r\n", path_id, path_info.duration);
					}
				}
			}
			break;
		}

	case HD_BSMUX_CB_EVENT_CUT_COMPLETE: {
			if (g_ia_moviemulti_usercb) {
				g_ia_moviemulti_usercb(cbinfo->id, MOVIE_USER_CB_EVENT_CUTFILE, 0);
			}
			break;
		}

	case HD_BSMUX_CB_EVENT_CLOSE_RESULT: {
			_MovieMulti_BsMux_CB_Result((void *)cbinfo);
			break;
		}

	default: {
			DBG_DUMP("Invalid event %d\r\n", (int)event);
			break;
		}
	}
	return 0;
}

static INT32 _MovieMulti_FileOut_HandleCrash(CHAR *p_cur_path, UINT32 iport)
{
	MOVIE_CB_CRASH_INFO *p_cinfo;

	static CHAR ro_folder[80] = {0};
	CHAR *p_ro_path;

	UINT32 is_crash_curr;
	UINT32 is_crash_prev;
	UINT32 is_crash_next;
	UINT32 is_crash_next_old;

	UINT32 idx;
	CHAR p_new_path[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	CHAR pnewPath[80] = {0};
	#if 1
	BOOL bCurrCrash = FALSE, bPrevCrash = FALSE;
	CHAR prev_crash_path[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	CHAR curr_crash_path[KFS_LONGNAME_PATH_MAX_LENG] = {0};
	#else
	BOOL bCrash = FALSE;
	#endif
	INT32 is_sync_time = 0;

	if (EthCamRxFuncEn && (idx = ImageApp_MovieMulti_BsPort2EthCamlink(iport)) != UNUSED) {
		idx = MaxLinkInfo.MaxImgLink * 2 + (idx - _CFG_ETHCAM_ID_1);
	} else if (ImageApp_MovieMulti_BsPort2Recid(iport) != UNUSED) {
		idx = iport / 2;
	} else {
		DBG_ERR("Get rec_id failed, iport %d\r\n", iport);
		return -1;
	}

	// current idx is table id
	p_cinfo = &g_crash_info[idx];
	// set idx to port id
	idx = iport;

	//loc_cpu(); //lock ---------
	//get the current status
	is_crash_curr = p_cinfo->is_crash_curr;
	is_crash_prev = p_cinfo->is_crash_prev;
	is_crash_next = p_cinfo->is_crash_next;
	is_crash_next_old = p_cinfo->is_crash_next_old;

	//update status
	p_cinfo->is_crash_curr = 0; //it will be done this time, reset to 0
	p_cinfo->is_crash_prev = 0; //it will be done this time, reset to 0
	p_cinfo->is_crash_next = 0; //move to is_crash_next_old, reset to 0
	p_cinfo->is_crash_next_old = is_crash_next; //backup old is_crash_next
	//unl_cpu(); //unlock ---------

	//handle the previous crash file
	if (is_crash_prev && p_cinfo->path_prev[0]) {
		p_ro_path = p_cinfo->path_prev;
		DBG_DUMP("Set prev crash %s, ro %d\r\n", p_ro_path, (!g_ia_moviemulti_crash_not_ro));

		is_sync_time = iamoviemulti_fm_is_sync_time(p_ro_path[0]);

		if (is_sync_time) {
			iamoviemulti_fm_dump_mtime(p_ro_path);
			iamoviemulti_fm_keep_mtime(p_ro_path);
		}
		memset(pnewPath, 0, 80);
		memcpy(pnewPath, p_ro_path, 80);
		NameRule_getCustom_byid(idx)->CustomFunc(MEDIANR_CUSTOM_CHANGECRASH, (UINT32)pnewPath, 0, 0);

		//get newpath folder
		if (FST_STA_OK != FileSys_GetParentDir(pnewPath, ro_folder)) {
			DBG_ERR("Find parent dir of %s failed\r\n", pnewPath);
			return -1;
		}

		if (!iamoviemulti_fm_is_format_free(p_ro_path[0])) {
			if (0 != iamoviemulti_fm_move_file(p_ro_path, ro_folder, p_new_path)) {
				DBG_ERR("move %s to %s failed\r\n", p_ro_path, ro_folder);
				return -1;
			}
		} else {
			if (0 != iamoviemulti_fm_replace_file(p_ro_path, ro_folder, p_new_path)) {
				DBG_WRN("move %s to %s failed\r\n", p_ro_path, ro_folder);
			}
		}

		if (!g_ia_moviemulti_crash_not_ro) {
			if (0 != iamoviemulti_fm_set_ro_file(p_new_path, TRUE)) {
				DBG_ERR("Set %s RO failed\r\n", p_new_path);
				return -1;
			}
		}

		if (0 != iamoviemulti_fm_flush_file(p_new_path)) {
			DBG_ERR("flush %s failed\r\n", p_new_path);
			return -1;
		}

		if (is_sync_time) {
			iamoviemulti_fm_sync_mtime(p_new_path);
			iamoviemulti_fm_dump_mtime(p_new_path);
		}

		#if 1
		bPrevCrash = TRUE;
		//store new path info
		strncpy(prev_crash_path, p_new_path, sizeof(p_new_path)-1);
		prev_crash_path[sizeof(p_new_path)-1] = '\0';
		#else
		bCrash = TRUE;
		#endif
	}

	//handle the current crash file
	if (is_crash_curr || is_crash_next_old) {
		p_ro_path = p_cur_path;
		DBG_DUMP("Set crash %s, ro %d\r\n", p_ro_path, (!g_ia_moviemulti_crash_not_ro));

		is_sync_time = iamoviemulti_fm_is_sync_time(p_ro_path[0]);

		if (is_sync_time) {
			iamoviemulti_fm_dump_mtime(p_ro_path);
			iamoviemulti_fm_keep_mtime(p_ro_path);
		}

		memset(pnewPath, 0, 80);
		memcpy(pnewPath, p_ro_path, 80);
		NameRule_getCustom_byid(idx)->CustomFunc(MEDIANR_CUSTOM_CHANGECRASH, (UINT32)pnewPath, 0, 0);
		memcpy(ro_folder, pnewPath, 80);

		//get newpath folder
		if (FST_STA_OK != FileSys_GetParentDir(pnewPath, ro_folder)) {
			DBG_ERR("Find parent dir of %s failed\r\n", pnewPath);
			return -1;
		}

		if (!iamoviemulti_fm_is_format_free(p_ro_path[0])) {
			if (0 != iamoviemulti_fm_move_file(p_ro_path, ro_folder, p_new_path)) {
				DBG_ERR("move %s to %s failed\r\n", p_ro_path, ro_folder);
				return -1;
			}
		} else {
			if (0 != iamoviemulti_fm_replace_file(p_ro_path, ro_folder, p_new_path)) {
				DBG_WRN("move %s to %s failed\r\n", p_ro_path, ro_folder);
			}
		}

		if (!g_ia_moviemulti_crash_not_ro) {
			if (0 != iamoviemulti_fm_set_ro_file(p_new_path, TRUE)) {
				DBG_ERR("Set %s RO failed\r\n", p_new_path);
				return -1;
			}
		}

		if (0 != iamoviemulti_fm_flush_file(p_new_path)) {
			DBG_ERR("flush %s failed\r\n", p_new_path);
			return -1;
		}

		if (is_sync_time) {
			iamoviemulti_fm_sync_mtime(p_new_path);
			iamoviemulti_fm_dump_mtime(p_new_path);
		}

		//curr file is moved to RO, clear path_prev
		p_cinfo->path_prev[0] = '\0';

		#if 1
		bCurrCrash = TRUE;
		//store new path info
		strncpy(curr_crash_path, p_new_path, sizeof(p_new_path)-1);
		curr_crash_path[sizeof(p_new_path)-1] = '\0';
		#else
		bCrash = TRUE;
		#endif
	} else {
		strncpy(p_cinfo->path_prev, p_cur_path, sizeof(p_cinfo->path_prev)-1);
		p_cinfo->path_prev[sizeof(p_cinfo->path_prev)-1] = '\0';
	}

	#if 1 //changed to callback file path
	if (bPrevCrash && g_ia_moviemulti_usercb) {
		g_ia_moviemulti_usercb(idx, MOVIE_USER_CB_EVENT_PREV_CARSH_FILE_COMPLETED, (UINT32)prev_crash_path);
	}
	if (bCurrCrash && g_ia_moviemulti_usercb) {
		g_ia_moviemulti_usercb(idx, MOVIE_USER_CB_EVENT_CARSH_FILE_COMPLETED, (UINT32)curr_crash_path);
	}
	#else
	if (bCrash && g_ia_moviemulti_usercb){
		g_ia_moviemulti_usercb(idx, MOVIE_USER_CB_EVENT_CARSH_FILE_COMPLETED, 0);
	}
	#endif

	return 0;
}

static INT32 _MovieMulti_FileOut_HandleEMR(CHAR *p_cur_path, UINT32 iport)
{
	UINT32 idx;
	CHAR *p_ro_path;
	UINT32 isEthCam = 0;

	if (EthCamRxFuncEn && (idx = ImageApp_MovieMulti_BsPort2EthCamlink(iport)) != UNUSED) {
		idx -= _CFG_ETHCAM_ID_1;
		isEthCam = 1;
	} else if ((idx = ImageApp_MovieMulti_BsPort2Imglink(iport)) >= MaxLinkInfo.MaxImgLink) {
		DBG_ERR("Get rec_id failed, iport %d\r\n", iport);
		return -1;
	}

	if (isEthCam && (EthCamLink[idx].fileout_i[1] != HD_FILEOUT_IN(0, iport))) {
		return 0;
	}

	// Not EMR path
	if ((ImgLink[idx].fileout_i[1] != HD_FILEOUT_IN(0, iport)) && (ImgLink[idx].fileout_i[3] != HD_FILEOUT_IN(0, iport))) {
		return 0;
	}

	//handle the emr file
	p_ro_path = p_cur_path;
	DBG_DUMP("Set emr %s, ro %d\r\n", p_ro_path, (!g_ia_moviemulti_emr_not_ro));

	if (!g_ia_moviemulti_emr_not_ro) {
		if (0 != iamoviemulti_fm_set_ro_file(p_ro_path, TRUE)) {
			DBG_ERR("Set %s emr to FileDB failed\r\n", p_ro_path);
			return -1;
		}
	}
#if 0  //EMR create file already in EMR Folder
	if (0 != iamoviemulti_fm_move_file(p_ro_path, ro_folder)) {
		DBG_ERR("move %s to %s failed\r\n", p_ro_path, ro_folder);
		return -1;
	}
#endif
	if (g_ia_moviemulti_usercb){
		idx = ImageApp_MovieMulti_BsPort2Imglink(iport);
		g_ia_moviemulti_usercb(idx, MOVIE_USER_CB_EVENT_EMR_FILE_COMPLETED, 0);
	}
	return 0;
}

static INT32 _MovieMulti_FileOut_GetSize(UINT32 iport, UINT64 *p_size)
{
	UINT32 idx;
	BOOL bEMR;
	UINT64 size, sec, bitrate;

	if ((idx = ImageApp_MovieMulti_BsPort2Imglink(iport)) >= MaxLinkInfo.MaxImgLink) {
		DBG_ERR("Get rec_id failed, iport %d\r\n", iport);
		return -1;
	}

	//get rec sec
	if (((ImgLinkFileInfo.emr_on & MASK_EMR_MAIN)  && (ImgLink[idx].fileout_i[1] == HD_FILEOUT_IN(0, iport))) ||
		((ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) && (ImgLink[idx].fileout_i[3] == HD_FILEOUT_IN(0, iport)))) {
		bEMR = 1;
	} else {
		bEMR = 0;
	}

	//get rec sec
	if (bEMR) {
		sec = ImgLinkFileInfo.emr_sec + ImgLinkFileInfo.keep_sec;
	} else {
		sec = ImgLinkFileInfo.seamless_sec;
	}
	//get rec bitrate
	bitrate = ImgLinkRecInfo[idx][0].target_bitrate;
	//cal allocate size
	size = sec * bitrate;

	*p_size = size;

	return 0;
}

static INT32 _MovieMulti_FileOut_CB_Naming(void *p_evdata)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		HD_FILEOUT_CBINFO *p_data = (HD_FILEOUT_CBINFO *)p_evdata;
		static UINT32 file_sn= 0;
		//const CHAR sample_name[] = "A:\\MOVIE\\20000101";
		//const CHAR sample_ext[] = "MP4";
		UINT32  idx_recid, idx_port;
		BOOL bEMR = 0;
		CHAR temppath[128] = {0};
		UINT32 file_type=p_data->type;
#if 0
		if(file_type==0){
			file_type=MEDIA_FILEFORMAT_MP4;
		}
#endif
		DBG_IND("event=%d\r\n", p_data->event);
		idx_recid = ImageApp_MovieMulti_BsPort2Recid(p_data->iport);
		if (idx_recid == UNUSED) {
			DBG_ERR("Invalid rec_id 0x%X\r\n", idx_recid);
			return E_PAR;
		}
		idx_port = p_data->iport;// - ISF_IN_BASE;

		DBG_IND("iport=%d, idx_recid=%d,idx_port=%d\r\n", p_data->iport,idx_recid,idx_port);
		DBG_IND("type=%d\r\n",p_data->type);

		switch (p_data->event) {
		default:
			case HD_BSMUX_FEVENT_NORMAL: {
/*
				if (((ImgLinkFileInfo.emr_on & MASK_EMR_MAIN)  && (ImgLink[idx_link].fileout_i2 == p_data->iport)) ||
					((ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) && (ImgLink[idx_link].fileout_i4 == p_data->iport))) {
					DBG_DUMP("EMR event\r\n");
					bEMR = 1;
				}
*/
				if (ImageApp_MovieMulti_isFileNaming(idx_recid)) {
					if (g_ia_moviemulti_usercb) {
						//mark for CID 139389
						//if (bEMR) {
						//	g_ia_moviemulti_usercb(idx_recid, MOVIE_USER_CB_EVENT_FILENAMING_EMR_CB, (UINT32) g_isf_dvcam_file_name);
						//} else
						{
							if (file_type == MEDIA_FILEFORMAT_JPG) {
								g_ia_moviemulti_usercb(idx_recid, MOVIE_USER_CB_EVENT_FILENAMING_SNAPSHOT_CB, (UINT32) g_isf_dvcam_file_name);
							} else {
								if ((idx_recid >= _CFG_REC_ID_1) && (idx_recid < MAX_IMG_PATH) && (ImgLinkFileInfo.emr_on & _CFG_MAIN_EMR_LOOP)) {
									_ImageApp_MovieMulti_TriggerThumb(idx_recid, TRUE);
								} else if ((idx_recid >= _CFG_CLONE_ID_1) && (idx_recid < _CFG_CLONE_ID_MAX) && (ImgLinkFileInfo.emr_on & _CFG_CLONE_EMR_LOOP)) {
									_ImageApp_MovieMulti_TriggerThumb(idx_recid, TRUE);
								} else {
									_ImageApp_MovieMulti_TriggerThumb(idx_recid, FALSE);
								}
								g_ia_moviemulti_usercb(idx_recid, MOVIE_USER_CB_EVENT_FILENAMING_MOV_CB, (UINT32) g_isf_dvcam_file_name);
							}
						}
					}
				}
			}
			break;
			case HD_BSMUX_FEVENT_EMR: {//for small buffer: main path stop ,and EMR start
				//p_folder = folder_emr;
				if (ImageApp_MovieMulti_isFileNaming(idx_recid)) {
					if (g_ia_moviemulti_usercb) {
						_ImageApp_MovieMulti_TriggerThumb(idx_recid, TRUE);
						g_ia_moviemulti_usercb(idx_recid, MOVIE_USER_CB_EVENT_FILENAMING_EMR_CB, (UINT32) g_isf_dvcam_file_name);
					}
					bEMR = 1;
				}
			}
			break;
		}

		//set ext name
		NameRule_getCustom_byid(idx_port)->SetInfo(MEDIANAMING_SETINFO_SETFILETYPE, file_type, 0, 0);

		if (ImageApp_MovieMulti_isFileNaming(idx_recid)) {
			//get full name
			if (file_type==MEDIA_FILEFORMAT_JPG) {
				NH_Custom_BuildFullPath_GiveFileName(idx_port, NMC_SECONDTYPE_PHOTO, g_isf_dvcam_file_name, g_isf_dvcam_full_path);
			} else {
				if (bEMR == 0) {//p_data->event == BSMUXER_FEVENT_NORMAL)
					NH_Custom_BuildFullPath_GiveFileName(idx_port, NMC_SECONDTYPE_MOVIE, g_isf_dvcam_file_name, g_isf_dvcam_full_path);
				} else {
					NH_Custom_BuildFullPath_GiveFileName(idx_port - 1, NMC_SECONDTYPE_EMR, g_isf_dvcam_file_name, g_isf_dvcam_full_path);
				}
			}
		}

		// Get Serial Number
		file_sn++;
		if (file_sn > 999) {
			file_sn = 1;
		}

		NameRule_getCustom_byid(idx_port)->CreateAndGetPath(0, temppath);
		DBG_IND(">>> temppath: %s\r\n", temppath);

		// File Naming
		p_data->fpath_size = 128;
		#if 0
		snprintf(p_data->p_fpath, p_data->fpath_size, "%s_%03d.%s",
			sample_name, file_sn, sample_ext);
		#else
		// File Naming
		snprintf(p_data->p_fpath, p_data->fpath_size,
			"%s",temppath);
		#endif

		if (p_data->is_reuse) { //rename existed file to new file and reuse it
			if (!iamoviemulti_fm_is_format_free(p_data->p_fpath[0])) {
				DBG_ERR("format-free failed\r\n");
				return -1;
			}
			if (iamoviemulti_fm_chk_format_free_dir_full(p_data->p_fpath) >0 ) {
				DBG_ERR("format-free folder full\r\n");
				if (g_ia_moviemulti_usercb) {
					g_ia_moviemulti_usercb(idx_recid, MOVIE_USER_CB_EVENT_CARD_FULL, (UINT32) p_data->p_fpath);
				}
				return -1;
			}
			DBG_IND("[CB_Naming] Format-Free Set. Need to reuse existed file.\r\n");

			if (0 != iamoviemulti_fm_reuse_file(p_data->p_fpath)) {
				DBG_ERR("^GReuse File %s failed\r\n", p_data->p_fpath);
				return -1;
			}
			DBG_IND("^G[CB_Naming] Reuse %s\r\n", p_data->p_fpath);
		}

		if (0 != iamoviemulti_fm_chk_naming(p_data->p_fpath)) {
			DBG_WRN("cb naming might wrong, path=%s\r\n", p_data->p_fpath);
		}

		DBG_DUMP(">>> NAMING: %s\r\n", p_data->p_fpath);

		if (iamovie_movie_path_info == TRUE) {
			// handle path
			if (file_type != MEDIA_FILEFORMAT_JPG)
			{
				UINT32 path_id = p_data->iport;
				MOVIE_CB_PATH_INFO path_info = {0};
				path_info.p_fpath = p_data->p_fpath;
				path_info.status = CB_PATH_STATUS_RECORD;
				if (_MovieMulti_Path_Handle(path_id, &path_info) != E_OK) {
					DBG_ERR("[%d] handle record failed\r\n", path_id);
				} else {
					DBG_IND("[%d] handle record (path=%s)\r\n", path_id, path_info.p_fpath);
				}
			}
		}
		return 0;
	} else {
		HD_FILEOUT_CBINFO *p_data = (HD_FILEOUT_CBINFO *)p_evdata;
		UINT32  idx_recid;
		CHAR temppath[128] = {0};
		UINT32 file_type=p_data->type;

		DBG_IND("event=%d\r\n", p_data->event);

		idx_recid = ImageApp_MovieMulti_BsPort2Recid(p_data->iport);
		if (idx_recid == UNUSED) {
			DBG_ERR("Invalid rec_id 0x%X\r\n", idx_recid);
			return E_PAR;
		}

		DBG_IND("iport=%d, idx_recid=%d\r\n", p_data->iport,idx_recid);
		DBG_IND("type=%d\r\n",p_data->type);

		switch (p_data->event) {
		default:
			case HD_BSMUX_FEVENT_NORMAL: {
				{
					if (g_ia_moviemulti_usercb) {
						{
							if (file_type==MEDIA_FILEFORMAT_JPG) {
								g_ia_moviemulti_usercb(idx_recid, MOVIE_USER_CB_EVENT_FILENAMING_SNAPSHOT_CB, (UINT32) temppath);
							} else {
								_ImageApp_MovieMulti_TriggerThumb(idx_recid, FALSE);
								g_ia_moviemulti_usercb(idx_recid, MOVIE_USER_CB_EVENT_FILENAMING_MOV_CB, (UINT32) temppath);
							}
						}
					}
				}
			}
			break;
			case HD_BSMUX_FEVENT_EMR: {//for small buffer: main path stop ,and EMR start
				{
					if (g_ia_moviemulti_usercb) {
						_ImageApp_MovieMulti_TriggerThumb(idx_recid, TRUE);
						g_ia_moviemulti_usercb(idx_recid, MOVIE_USER_CB_EVENT_FILENAMING_EMR_CB, (UINT32) temppath);
					}
				}
			}
			break;
		}

		// File Naming
		p_data->fpath_size = 128;

		// File Naming
		snprintf(p_data->p_fpath, p_data->fpath_size,
			"%s",temppath);
		DBG_DUMP(">>> NAMING: %s\r\n", p_data->p_fpath);

		if (iamovie_movie_path_info == TRUE) {
			// handle path
			if (file_type != MEDIA_FILEFORMAT_JPG)
			{
				UINT32 path_id = p_data->iport;
				MOVIE_CB_PATH_INFO path_info = {0};
				path_info.p_fpath = p_data->p_fpath;
				path_info.status = CB_PATH_STATUS_RECORD;
				if (_MovieMulti_Path_Handle(path_id, &path_info) != E_OK) {
					DBG_ERR("[%d] handle record failed\r\n", path_id);
				} else {
					DBG_IND("[%d] handle record (path=%s)\r\n", path_id, path_info.p_fpath);
				}
			}
		}
		return 0;
	}
}

static INT32 _MovieMulti_FileOut_CB_Opened(void *p_evdata)
{
	HD_FILEOUT_CBINFO *p_data = (HD_FILEOUT_CBINFO *)p_evdata;
	UINT32 iport;
	UINT32 idx;
	UINT64 size;

	DBG_IND("begin\r\n");

	iport = p_data->iport;
	if ((idx = ImageApp_MovieMulti_BsPort2Imglink(iport)) >= MaxLinkInfo.MaxImgLink) {
		DBG_ERR("Get rec_id failed, iport %d\r\n", iport);
		return -1;
	}

	//only check emr
	if (((ImgLinkFileInfo.emr_on & MASK_EMR_MAIN)  && (ImgLink[idx].fileout_i[1] == HD_FILEOUT_IN(0, iport))) ||
		((ImgLinkFileInfo.emr_on & MASK_EMR_CLONE) && (ImgLink[idx].fileout_i[3] == HD_FILEOUT_IN(0, iport)))) {
		if (0 != _MovieMulti_FileOut_GetSize(iport, &size)){
			DBG_ERR("GetSize iport %d failed\r\n", p_data->iport);
			return -1;
		}
	} else {
		size = 0;
	}

	//reserved more
	size += (size/10);
	//return allocate size
	p_data->alloc_size = size;

	DBG_IND("end\r\n");
	return 0;
}

static INT32 _MovieMulti_FileOut_CB_Closed(void *p_evdata)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		HD_RESULT hd_ret;
		HD_FILEOUT_CBINFO *p_data = (HD_FILEOUT_CBINFO *)p_evdata;
		UINT32 idx;
		BOOL bjpg_file = 0;
		CHAR *g_p_name;
		CHAR *g_p_ext_name;

		DBG_IND("begin\r\n");

		if (0 != iamoviemulti_fm_add_file(p_data->p_fpath)) {
			DBG_ERR("FileDB add file %s failed\r\n", p_data->p_fpath);
			return -1;
		}

		if ((idx = ImageApp_MovieMulti_BsPort2Recid(p_data->iport)) == UNUSED) {
			DBG_ERR("BsPort2Recid(%d) == UNUSED\r\n", p_data->iport);
			return -1;
		}

		g_p_name = strrchr(p_data->p_fpath, '\\');
		if (NULL == g_p_name) {
			DBG_ERR("Find name of %s failed\r\n", p_data->p_fpath);
			return -1;
		}
		g_p_name++;

		g_p_ext_name = strrchr(g_p_name, '.');
		if (NULL == g_p_ext_name) {
			DBG_ERR("Find name of %s failed\r\n", g_p_name);
			return -1;
		}
		g_p_ext_name++;

		DBG_IND("p_name=%s p_ext_name=%s\r\n", g_p_name, g_p_ext_name);

		if (strcmp(g_p_ext_name, "JPG") == 0 || strcmp(g_p_ext_name, "jpg") == 0) {
			if (imgcap_rawenc_va[idx]) {
				if ((hd_ret = hd_common_mem_free(imgcap_rawenc_pa[idx], (void *)imgcap_rawenc_va[idx])) != HD_OK) {
					DBG_ERR("hd_common_mem_free failed(%d)\r\n", hd_ret);
				}
				imgcap_rawenc_va[idx] = 0;
				imgcap_rawenc_pa[idx] = 0;
				imgcap_rawenc_size[idx] = 0;
			}
		}

		if (g_ia_moviemulti_usercb){
			if (iamovie_movie_path_info == TRUE) {
				MOVIEMULTI_CLOSE_FILE_INFO info = {0};
				UINT32 path_id = p_data->iport;
				CHAR *p_ext_name;
				MOVIE_CB_PATH_INFO path_info = {0};
				path_info.p_fpath = p_data->p_fpath;
				path_info.status = CB_PATH_STATUS_CLOSE;
				p_ext_name = strrchr(p_data->p_fpath, '.');
				if (NULL == p_ext_name) {
					DBG_ERR("Find ext name of %s failed\r\n", p_data->p_fpath);
				} else {
					p_ext_name++;
				}
				if ((NULL != p_ext_name) && (strcmp(p_ext_name, "JPG") != 0) && (strcmp(p_ext_name, "jpg") != 0))
				{
					if (_MovieMulti_Path_Handle(path_id, &path_info) != E_OK) {
						DBG_ERR("[%d] handle close failed\r\n", path_id);
					} else {
						DBG_IND("[%d] handle close (path=%s)(duration=%d)\r\n",
							path_id, path_info.p_fpath, path_info.duration);
						info.duration = path_info.duration;
						strncpy(info.path, path_info.p_fpath, sizeof(info.path)-1);
						info.path[sizeof(info.path)-1] = '\0';
					}
				}
				else {
					//JPG case
					DBG_IND("[%d] JPG  handle close (path=%s)(duration=%d)\r\n",
						path_id, path_info.p_fpath, path_info.duration);
					info.duration = path_info.duration;
					strncpy(info.path, path_info.p_fpath, sizeof(info.path)-1);
					info.path[sizeof(info.path)-1] = '\0';
				}
				g_ia_moviemulti_usercb(idx, MOVIE_USER_CB_EVENT_CLOSE_FILE_COMPLETED, (UINT32)&info);
			} else {
				g_ia_moviemulti_usercb(idx, MOVIE_USER_CB_EVENT_CLOSE_FILE_COMPLETED, (UINT32)g_p_ext_name);
			}
		}

		if(strcmp(g_p_ext_name, "JPG") == 0 || strcmp(g_p_ext_name, "jpg") == 0){
			bjpg_file = 1;
		}

		//if ((ImgLinkFileInfo.emr_on & _CFG_EMR_SET_CRASH) && (bjpg_file == 0)) {
		if ((bjpg_file == 0)) {
			if (0 != _MovieMulti_FileOut_HandleCrash(p_data->p_fpath, p_data->iport)) {
				DBG_ERR("HandleCrash %s iport %d failed\r\n", p_data->p_fpath, p_data->iport);
				return -1;
			}
		}

		if ((ImgLinkFileInfo.emr_on & (MASK_EMR_MAIN | MASK_EMR_CLONE)) && (bjpg_file == 0)) {
			if (0 != _MovieMulti_FileOut_HandleEMR(p_data->p_fpath, p_data->iport)) {
				DBG_ERR("HandleEMR %s iport %d failed\r\n", p_data->p_fpath, p_data->iport);
				return -1;
			}
		}

		DBG_IND("end\r\n");
		return 0;
	} else {
		HD_FILEOUT_CBINFO *p_data = (HD_FILEOUT_CBINFO *)p_evdata;
		UINT32 idx;

		DBG_IND("begin\r\n");

		if ((idx = ImageApp_MovieMulti_BsPort2Recid(p_data->iport)) == UNUSED) {
			DBG_ERR("BsPort2Recid(%d) == UNUSED\r\n", p_data->iport);
			return -1;
		}

		DBG_IND("p_path=%s\r\n", p_data->p_fpath);

		if (g_ia_moviemulti_usercb){
			if (iamovie_movie_path_info == TRUE) {
				MOVIEMULTI_CLOSE_FILE_INFO info = {0};
				UINT32 path_id = p_data->iport;
				CHAR *p_ext_name;
				MOVIE_CB_PATH_INFO path_info = {0};
				path_info.p_fpath = p_data->p_fpath;
				path_info.status = CB_PATH_STATUS_CLOSE;
				p_ext_name = strrchr(p_data->p_fpath, '.');
				if (NULL == p_ext_name) {
					DBG_ERR("Find ext name of %s failed\r\n", p_data->p_fpath);
				} else {
					p_ext_name++;
				}
				if ((NULL != p_ext_name) && (strcmp(p_ext_name, "JPG") != 0) && (strcmp(p_ext_name, "jpg") != 0))
				{
					if (_MovieMulti_Path_Handle(path_id, &path_info) != E_OK) {
						DBG_ERR("[%d] handle close failed\r\n", path_id);
					} else {
						DBG_IND("[%d] handle close (path=%s)(duration=%d)\r\n",
							path_id, path_info.p_fpath, path_info.duration);
						info.duration = path_info.duration;
						strncpy(info.path, path_info.p_fpath, sizeof(info.path)-1);
						info.path[sizeof(info.path)-1] = '\0';
					}
				}
				else {
					//JPG case
					DBG_IND("[%d] JPG  handle close (path=%s)(duration=%d)\r\n",
						path_id, path_info.p_fpath, path_info.duration);
					info.duration = path_info.duration;
					strncpy(info.path, path_info.p_fpath, sizeof(info.path)-1);
					info.path[sizeof(info.path)-1] = '\0';
				}
				g_ia_moviemulti_usercb(idx, MOVIE_USER_CB_EVENT_CLOSE_FILE_COMPLETED, (UINT32)&info);
			} else {
				g_ia_moviemulti_usercb(idx, MOVIE_USER_CB_EVENT_CLOSE_FILE_COMPLETED, (UINT32)p_data->p_fpath); // use full path
			}
		}

		DBG_IND("end\r\n");
		return 0;
	}
}

static INT32 _MovieMulti_FileOut_CB_Delete(void *p_evdata)
{
	HD_FILEOUT_CBINFO *p_data = (HD_FILEOUT_CBINFO *)p_evdata;

	UINT32 port_active;
	UINT64 remain_size;
	UINT64 resv_size = 0;
	UINT64 free_size;
	UINT64 need_size = 0;
	CHAR drive;
	INT32 type_chk;

	//error check
	//
	if (iamoviemulti_fm_is_format_free(p_data->drive)) {
		DBG_IND("[CB_Delete] Format-Free Set.\r\n");
		return 0;
	}

	drive = p_data->drive;
	port_active = p_data->port_num;
	remain_size = p_data->remain_size;
	free_size = FileSys_GetDiskInfoEx(drive, FST_INFO_FREE_SPACE);
	if (free_size == 0) {
		DBG_ERR("free_size is 0\r\n");
		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(0, MOVIE_USER_CB_EVENT_CLOSE_RESULT, MOVREC_EVENT_RESULT_FULL);
		}
		p_data->ret_val = HD_FILEOUT_RETVAL_NOFREE_SPACE;
		return -1;
	}
	iamoviemulti_fm_set_remain_size(drive, remain_size);

	//get chk ro type
	if (0 != iamoviemulti_fm_get_rofile_info(drive, &type_chk, IAMOVIEMULTI_FM_ROINFO_TYPE)) {
		DBG_ERR("get chk ro type failed\r\n");
		return -1;
	}

	if (type_chk == IAMOVIEMULTI_FM_CHK_NUM) {
		//Check RO num
		if (0 != iamoviemulti_fm_chk_ro_file(drive)) {
			DBG_ERR("check read-only file failed, drive %c\r\n", drive);
			return -1;
		}
		DBG_IND("Check read-only file done.\r\n");
	}
	else {
		//Check RO pct
		if (0 != iamoviemulti_fm_chk_ro_size(drive)) {
			DBG_ERR("check read-only file size failed, drive %c\r\n", drive);
			return -1;
		}
		DBG_IND("Check read-only file size done.\r\n");
	}

	//Check disk space
	if (0 != iamoviemulti_fm_get_resv_size(drive, &resv_size)) {
		DBG_ERR("get resv size failed, drive %c\r\n", drive);
		return -1;
	}

	DBG_IND("Fout resv size %llu\r\n", resv_size);

	if (resv_size > free_size) {
		//card full
		if (!iamoviemulti_fm_is_loop_del(drive)) {
			//stop
			DBG_DUMP("Card full. Stop.\r\n");
			if (g_ia_moviemulti_usercb) {
				g_ia_moviemulti_usercb(0, MOVIE_USER_CB_EVENT_CLOSE_RESULT, MOVREC_EVENT_RESULT_FULL);
			}
			p_data->ret_val = HD_FILEOUT_RETVAL_NOFREE_SPACE;
			return 0;
		}
		else {
			//need deletion
			DBG_DUMP("Card full. Need deletion. Port num %d\r\n", port_active);
			need_size = resv_size - free_size;
		}
		DBG_DUMP("remain %lld, free %lld, resv %lld, need %lld\r\n",
			remain_size, free_size, resv_size, need_size);
	}

	if (need_size > 0 && iamoviemulti_fm_is_loop_del(drive)) {
		//Check filedb num
		if (0 == iamoviemulti_fm_get_filedb_num(drive)) {
			DBG_ERR("get filedb num is 0, drive %c\r\n", drive);
			if (g_ia_moviemulti_usercb) {
				g_ia_moviemulti_usercb(0, MOVIE_USER_CB_EVENT_CLOSE_RESULT, MOVREC_EVENT_RESULT_FULL);
			}
			p_data->ret_val = HD_FILEOUT_RETVAL_NOFREE_SPACE;
			return -1;
		}

		if (0 != iamoviemulti_fm_del_file(drive, need_size)) {
			DBG_ERR("FileDB del file failed, drive %c\r\n", drive);
			return -1;
		}
	}

	return 0;
}

static INT32 _MovieMulti_FileOut_CB_Fs_Err(void *p_evdata)
{
	HD_FILEOUT_CBINFO *p_data = (HD_FILEOUT_CBINFO *)p_evdata;
	HD_FILEOUT_ERRCODE errcode = p_data->errcode;
	INT32 iport;
	CHAR drive;
	CHAR type[16] = {0};

	//take iport
	iport = p_data->iport;

	//take drive
	drive = p_data->drive;

	switch(errcode) {
	case HD_FILEOUT_ERRCODE_CARD_SLOW:
		DBG_DUMP("<== FOUT SLOW CARD drive %c port %d ==>\r\n", drive, iport);
		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(0, MOVIE_USER_CB_ERROR_CARD_SLOW, 0);
		}
		break;
	case HD_FILEOUT_ERRCODE_CARD_WR_ERR:
		DBG_DUMP("<== WRITE CARD ERROR drive %c port %d ==>\r\n", drive, iport);
		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(0, MOVIE_USER_CB_ERROR_CARD_WR_ERR, 0);
		}
		break;
	case HD_FILEOUT_ERRCODE_LOOPREC_FULL:
		DBG_DUMP("<== LOOPREC FULL ERROR drive %c port %d ==>\r\n", drive, iport);
		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(0, MOVIE_USER_CB_ERROR_SEAMLESS_REC_FULL, 0);
		}
		break;
	case HD_FILEOUT_ERRCODE_SNAPSHOT_ERR:
		DBG_DUMP("<== SNAPSHOT ERROR drive %c port %d ==>\r\n", drive, iport);
		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(0, MOVIE_USER_CB_ERROR_SNAPSHOT_ERR, 0);
		}
		break;
	case HD_FILEOUT_ERRCODE_FOP_SLOW:
		switch (p_data->fop) {
		case HD_FILEOUT_FOP_CREATE:
			strncpy(type, "CREATE", (sizeof(type)-1));
			break;
		case HD_FILEOUT_FOP_CLOSE:
			strncpy(type, "CLOSE", (sizeof(type)-1));
			break;
		case HD_FILEOUT_FOP_CONT_WRITE:
			strncpy(type, "CONT_WRITE", (sizeof(type)-1));
			break;
		case HD_FILEOUT_FOP_SEEK_WRITE:
			strncpy(type, "SEEK_WRITE", (sizeof(type)-1));
			break;
		case HD_FILEOUT_FOP_FLUSH:
			strncpy(type, "FLUSH", (sizeof(type)-1));
			break;
		case HD_FILEOUT_FOP_SNAPSHOT:
			strncpy(type, "SNAPSHOT", (sizeof(type)-1));
			break;
		default:
			strncpy(type, "NULL", (sizeof(type)-1));
			break;
		}
		//DBG_DUMP("<== FOP %s SLOW drive %c port %d dur %d ==>\r\n", type, drive, iport, p_data->dur);
		if (g_ia_moviemulti_usercb) {
			g_ia_moviemulti_usercb(0, MOVIE_USRR_CB_ERROR_FOUT_SLOW_STATUS, (UINT32)p_data);
		}
		break;
	default:
		DBG_ERR("Invalid errcode id 0x%X\r\n", errcode);
		break;
	}

	return 0;
}

INT32 _ImageApp_MovieMulti_FileOut_CB(CHAR *p_name, HD_FILEOUT_CBINFO *cbinfo, UINT32 *param)
{
	HD_FILEOUT_CB_EVENT event = cbinfo->cb_event;

	switch (event) {
	case HD_FILEOUT_CB_EVENT_NAMING: {
			_MovieMulti_FileOut_CB_Naming((void *)cbinfo);
			break;
		}

	case HD_FILEOUT_CB_EVENT_OPENED: {
			if (iamovie_use_filedb_and_naming == TRUE) {
				_MovieMulti_FileOut_CB_Opened((void *)cbinfo);
			}
			break;
		}

	case HD_FILEOUT_CB_EVENT_CLOSED: {
			_MovieMulti_FileOut_CB_Closed((void *)cbinfo);
			break;
		}

	case HD_FILEOUT_CB_EVENT_DELETE: {
			if (iamovie_use_filedb_and_naming == TRUE) {
				_MovieMulti_FileOut_CB_Delete((void *)cbinfo);
			}
			break;
		}

	case HD_FILEOUT_CB_EVENT_FS_ERR: {
			_MovieMulti_FileOut_CB_Fs_Err((void *)cbinfo);
			break;
		}

	default: {
			DBG_DUMP("Invalid event %d\r\n", (int)event);
			break;
		}
	}
	return 0;
}

// =================== Temporary area start ===================
UINT32 ImageApp_MovieMulti_EthCam_TxTrigThumb(UINT32 portid)
{
	return 0;
}

ER ImageApp_MovieMulti_InitCrash(MOVIE_CFG_REC_ID rec_id)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		MOVIE_CB_CRASH_INFO *p_cinfo;
		UINT32 i = rec_id, idx;

		if (i < (_CFG_REC_ID_1 + MaxLinkInfo.MaxImgLink)) {
			idx = (i - _CFG_REC_ID_1) * 2;
		} else if ((i >= _CFG_CLONE_ID_1) && (i < (_CFG_CLONE_ID_1 + MaxLinkInfo.MaxImgLink))) {
			idx = (i - _CFG_CLONE_ID_1) * 2 + 1;
		} else if (EthCamRxFuncEn && (i >= _CFG_ETHCAM_ID_1) && (i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink))) {
			idx = MaxLinkInfo.MaxImgLink * 2 + (i - _CFG_ETHCAM_ID_1);
		} else {
			return E_PAR;
		}
		p_cinfo = &g_crash_info[idx];
		p_cinfo->is_crash_next = 0; //record stop, no next to be set
		memset(p_cinfo, 0, sizeof(MOVIE_CB_CRASH_INFO));

		return E_OK;
	} else {
		DBG_ERR("Function not support due to FileDB and Naming is not used.\r\n");
		return E_NOSPT;
	}
}

ER ImageApp_MovieMulti_SetCrash(MOVIE_CFG_REC_ID rec_id, UINT32 is_set)
{
	if (iamovie_use_filedb_and_naming == TRUE) {
		MOVIE_CB_CRASH_INFO *p_cinfo;
		UINT32 i = rec_id, idx;
		UINT32 sec_cur;
		UINT32 sec_rollback;
		UINT32 sec_forward;
		UINT32 sec_seamless;

		UINT32 is_crash_curr = 0;
		UINT32 is_crash_next = 0;
		UINT32 is_crash_prev = 0;
		UINT32 is_crash_ext  = 0;

		if (i < (_CFG_REC_ID_1 + MaxLinkInfo.MaxImgLink)) {
			idx = (i - _CFG_REC_ID_1) * 2;
		} else if ((i >= _CFG_CLONE_ID_1) && (i < (_CFG_CLONE_ID_1 + MaxLinkInfo.MaxImgLink))) {
			idx = (i - _CFG_CLONE_ID_1) * 2 + 1;
		} else if (EthCamRxFuncEn && (i >= _CFG_ETHCAM_ID_1) && (i < (_CFG_ETHCAM_ID_1 + MaxLinkInfo.MaxEthCamLink))) {
			idx = MaxLinkInfo.MaxImgLink * 2 + (i - _CFG_ETHCAM_ID_1);
		} else {
			return E_PAR;
		}

		p_cinfo = &g_crash_info[idx];

		sec_cur = g_vdo_rec_sec;
		sec_seamless = ImgLinkFileInfo.seamless_sec;
		sec_rollback = ImgLinkFileInfo.rollback_sec;
		sec_forward = ImgLinkFileInfo.forward_sec;

		if (sec_rollback > sec_seamless ||
			sec_forward > sec_seamless ||
			sec_cur > sec_seamless) {
			DBG_ERR("Invalid param seamless %d cur %d rollback %d forward %d\r\n",
				sec_seamless, sec_cur, sec_rollback, sec_forward);
			return E_PAR;
		}

		DBG_DUMP("CRASH rec_id %x seamless %d, cur %d rollback %d forward %d\r\n",
				rec_id, sec_seamless, sec_cur, sec_rollback, sec_forward);

		//set the new status
		if (is_set) {
			is_crash_curr = 1;
			is_crash_ext  = ImageApp_MovieMulti_IsExtendCrash(i);

			if (sec_cur < sec_rollback) {
				is_crash_prev = 1;
			}
			if (is_crash_ext) {
				ImageApp_MovieMulti_SetExtendCrash(i);
			} else {
				if ((sec_seamless - sec_cur) < sec_forward) {
					is_crash_next = 1;
				}
			}
		} else {
			is_crash_curr = 0;
			is_crash_prev = 0;
			is_crash_next = 0;
		}

		//loc_cpu(); //lock ---------

		//store the status
		p_cinfo->is_crash_curr = is_crash_curr;
		p_cinfo->is_crash_prev = is_crash_prev;
		p_cinfo->is_crash_next = is_crash_next;

		//unl_cpu(); //unlock ---------

		DBG_DUMP("CRASH is_crash_curr %d is_crash_prev %d is_crash_next %d is_crash_ext %d\r\n",
			is_crash_curr, is_crash_prev, is_crash_next, is_crash_ext);

		return E_OK;
	} else {
		// only support ExtendCrash and no cancel if iamovie_use_filedb_and_naming == DISABLE
		ER ret = E_SYS;
		UINT32 i = rec_id;
		UINT32 is_crash_ext  = 0;

		DBG_DUMP("CRASH rec_id %x cur %d\r\n", rec_id, g_vdo_rec_sec);

		//set the new status
		if (is_set) {
			is_crash_ext  = ImageApp_MovieMulti_IsExtendCrash(i);

			if (is_crash_ext) {
				ret = ImageApp_MovieMulti_SetExtendCrash(i);
			}
		}

		DBG_DUMP("CRASH is_set=%d, is_crash_ext=%d\r\n", is_set, is_crash_ext);

		return ret;
	}
}

