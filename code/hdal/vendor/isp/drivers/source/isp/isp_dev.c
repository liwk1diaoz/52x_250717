#if defined(__FREERTOS)
#include <string.h>
#include "isp_dbg.h"
#include "isp_dev_int.h"
#else
#include "isp_ioctl.h"
#include "isp_dbg.h"
#include "isp_lib.h"
#include "isp_main.h"
#endif

#if defined(__KERNEL__)
extern UINT32 isp_id_list;
#endif

//=============================================================================
// global variable
//=============================================================================
#if defined(__FREERTOS)
static ISP_DEV_INFO pdev_info;
#endif
//=============================================================================
// function declaration
//=============================================================================
static BOOL isp_sync_item_valid[ISP_ID_MAX_NUM][ISP_SYNC_ITEM_MAX][SYNC_INFO_BUF];
static UINT32 isp_frame_count[ISP_ID_MAX_NUM][SYNC_INFO_BUF];
//=============================================================================
// external functions
//=============================================================================
void isp_dev_reset_item_valid(ISP_ID id, UINT32 frame_cnt)
{
	UINT32 i, j = frame_cnt % SYNC_INFO_BUF;

	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return;
	}
	for (i = ISP_SYNC_AE_STATUS; i < ISP_SYNC_FRAME_CNT_MAX; i++) {
		isp_sync_item_valid[id][i][j] = FALSE;
	}
}

ER isp_dev_get_sync_item(ISP_ID id, ISP_SYNC_SEL sel, ISP_SYNC_ITEM item, void *data)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();
	BOOL emu_en = isp_api_get_emu_enable();
	UINT32 sync_id = 0, get_id = 0, frame_count = 0;
	UINT32 isp_id = id;
	UINT32 dbg_mode = isp_dbg_get_dbg_mode(isp_id);

	if (pdev_info == NULL) {
		return E_SYS;
	}
	if (isp_id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(dbg_mode & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return E_PAR;
	}

	switch (sel) {
	case ISP_SYNC_SEL_SIE:
		sync_id = (isp_sync_id_sie[isp_id]) % SYNC_INFO_BUF;
		frame_count = isp_sync_id_sie[isp_id];
		break;

	case ISP_SYNC_SEL_IPP:
		if (pdev_info->ipp_to_sie_id_map_en) {
			isp_id = pdev_info->ipp_to_sie_id_table[id];
		} else {
			isp_id = id;
		}

		sync_id = (isp_sync_id_ipp[isp_id]) % SYNC_INFO_BUF;
		frame_count = isp_sync_id_ipp[isp_id];
		break;

	case ISP_SYNC_SEL_ENC:
		if (pdev_info->enc_to_sie_id_map_en) {
			isp_id = pdev_info->enc_to_sie_id_table[id];
		} else {
			isp_id = id;
		}

		sync_id = (isp_sync_id_enc[isp_id]) % SYNC_INFO_BUF;
		frame_count = isp_sync_id_enc[isp_id];
		break;

	default:
		sync_id = 1;
		frame_count = 1;
		break;
	}

	if (sync_id == 0) {
		get_id = SYNC_INFO_BUF - 1;
	} else {
		get_id = sync_id - 1;
	}

	// check frame count
	if (((INT32)frame_count - (INT32)isp_frame_count[isp_id][get_id] >= SYNC_INFO_BUF) && (item < ISP_SYNC_FRAME_CNT_MAX)) {
		PRINT_ISP_WRN(dbg_mode & ISP_DBG_WRN_MSG, "sync info not match (%d, %d, %d, %d, %d) \r\n", isp_id, frame_count, isp_frame_count[isp_id][get_id], get_id, sel);
		return E_SYS;
	}

	if (item > ISP_SYNC_FRAME_CNT_MAX) {
		get_id = 0;        // out of frame cnt SyncInfo
	}
	if (emu_en == TRUE) {
		get_id = 0;        // fixed to setting idx = 0
	}
	if (isp_sync_item_valid[isp_id][item][get_id] == FALSE) {
		return E_SYS;
	}

	switch (item) {
	// NOTE: Start of frame cnt SyncInfo
	case ISP_SYNC_AE_STATUS:
		memcpy((UINT32 *)data, &pdev_info->sync_info[isp_id][get_id].ae_status, sizeof(UINT32));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_AE_STATUS, "ISP (%2d) FrameCnt (%2d) Get (%2d) AE_STATUS (%d)\r\n", isp_id, sync_id, get_id, pdev_info->sync_info[isp_id][get_id].ae_status);
		break;

	case ISP_SYNC_AE_TOTAL_GAIN:
		memcpy((UINT32 *)data, &pdev_info->sync_info[isp_id][get_id].total_gain, sizeof(UINT32));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_TOTAL_GAIN, "ISP (%2d) FrameCnt (%2d) Get (%2d) TOTAL_GAIN (%d)\r\n", isp_id, sync_id, get_id, pdev_info->sync_info[isp_id][get_id].total_gain);
		break;

	case ISP_SYNC_AE_DGAIN:
		memcpy((UINT32 *)data, &pdev_info->sync_info[isp_id][get_id].dgain, sizeof(UINT32));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_D_GAIN, "ISP (%2d) FrameCnt (%2d) Get (%2d) DGAIN (%d)\r\n", isp_id, sync_id, get_id, pdev_info->sync_info[isp_id][get_id].dgain);
		break;

	case ISP_SYNC_AE_LV:
		memcpy((UINT32 *)data, &pdev_info->sync_info[isp_id][get_id].lv, sizeof(UINT32));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_LV, "ISP (%2d) FrameCnt (%2d) Get (%2d) LV (%d)\r\n", isp_id, sync_id, get_id, pdev_info->sync_info[isp_id][get_id].lv);
		break;

	case ISP_SYNC_AE_LV_BASE:
		memcpy((UINT32 *)data, &pdev_info->sync_info[isp_id][get_id].lv_base, sizeof(UINT32));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_LV, "ISP (%2d) FrameCnt (%2d) Get (%2d) LV_BASE (%d)\r\n", isp_id, sync_id, get_id, pdev_info->sync_info[isp_id][get_id].lv_base);
		break;

	case ISP_SYNC_AE_LA_ENABLE:
		memcpy((BOOL *)data, &pdev_info->sync_info[isp_id][get_id].la_enable, sizeof(BOOL));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_LA_ENABLE, "ISP (%2d) FrameCnt (%2d) Get (%2d) la_enable (%d)\r\n", isp_id, sync_id, get_id, pdev_info->sync_info[isp_id][get_id].la_enable);
		break;

	case ISP_SYNC_AWB_CGAIN:
		memcpy((UINT32 *)data, &pdev_info->sync_info[isp_id][get_id].cgain[0], sizeof(UINT32) * 3);
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_C_GAIN, "ISP (%2d) FrameCnt (%2d) Get (%2d) CGAIN(%d, %d, %d)\r\n"
			, isp_id, sync_id, get_id, pdev_info->sync_info[isp_id][get_id].cgain[0], pdev_info->sync_info[isp_id][get_id].cgain[1], pdev_info->sync_info[isp_id][get_id].cgain[2]);
		break;

	case ISP_SYNC_AWB_CT:
		memcpy((UINT32 *)data, &pdev_info->sync_info[isp_id][get_id].ct, sizeof(UINT32));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_CT, "ISP (%2d) FrameCnt (%2d) Get (%2d) CT (%d)\r\n", isp_id, sync_id, get_id, pdev_info->sync_info[isp_id][get_id].ct);
		break;

	case ISP_SYNC_AWB_CA_ENABLE:
		memcpy((BOOL *)data, &pdev_info->sync_info[isp_id][get_id].ca_enable, sizeof(BOOL));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_CA_ENABLE, "ISP (%2d) FrameCnt (%2d) Get (%2d) ca_enable (%d)\r\n", isp_id, sync_id, get_id, pdev_info->sync_info[isp_id][get_id].ca_enable);
		break;

	case ISP_SYNC_IQ_FINAL_CGAIN:
		memcpy((UINT32 *)data, &pdev_info->sync_info[isp_id][get_id].final_cgain[0], sizeof(UINT32) * 3);
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_FINAL_C_GAIN, "ISP (%2d) FrameCnt (%2d) Get (%2d) FINAL_CGAIN(%d, %d, %d)\r\n", isp_id, sync_id, get_id
			, pdev_info->sync_info[isp_id][get_id].final_cgain[0], pdev_info->sync_info[isp_id][get_id].final_cgain[1], pdev_info->sync_info[isp_id][get_id].final_cgain[2]);
		break;
	// NOTE: end of frame cnt SyncInfo

	case ISP_SYNC_CAPTURE_TOTAL_GAIN:
		memcpy((UINT32 *)data, &pdev_info->capture_total_gain[isp_id], sizeof(UINT32));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_CAPTURE, "ISP (%2d) Get CAPTURE_TOTAL_GAIN (%d)\r\n", isp_id, pdev_info->capture_total_gain[isp_id]);
		break;

	case ISP_SYNC_CAPTURE_DGAIN:
		memcpy((UINT32 *)data, &pdev_info->capture_dgain[isp_id], sizeof(UINT32));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_CAPTURE, "ISP (%2d) Get CAPTURE_DGAIN (%d)\r\n", isp_id, pdev_info->capture_dgain[isp_id]);
		break;

	case ISP_SYNC_SHDR_FRAME_NUM:
		memcpy((UINT32 *)data, &pdev_info->shdr_frame_num[isp_id], sizeof(UINT32));
		break;

	case ISP_SYNC_SHDR_EV_RATIO:
		memcpy((UINT32 *)data, &pdev_info->shdr_ev_ratio[isp_id][0], sizeof(UINT32) * ISP_SEN_MFRAME_MAX_NUM);
		break;

	case ISP_SYNC_AE_INIT:
		memcpy((ISP_AE_INIT_INFO *)data, &pdev_info->ae_init_info[isp_id], sizeof(ISP_AE_INIT_INFO));
		break;

	case ISP_SYNC_AWB_INIT:
		memcpy((ISP_AWB_INIT_INFO *)data, &pdev_info->awb_init_info[isp_id], sizeof(ISP_AWB_INIT_INFO));
		break;

	case ISP_SYNC_IQ_INIT:
		memcpy((ISP_IQ_INIT_INFO *)data, &pdev_info->iq_init_info[isp_id], sizeof(ISP_IQ_INIT_INFO));
		break;

	case ISP_SYNC_CA_TH:
		memcpy((ISP_CA_TH_PARAM *)data, &pdev_info->ca_th[isp_id], sizeof(ISP_CA_TH_PARAM));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_CA_TH, "ISP (%2d), th_enable (%d), g(%d, %d), r(%d, %d), b(%d, %d), p(%d, %d)\r\n", isp_id, pdev_info->ca_th[isp_id].th_enable,
			pdev_info->ca_th[isp_id].g_th_l, pdev_info->ca_th[isp_id].g_th_u, pdev_info->ca_th[isp_id].r_th_l, pdev_info->ca_th[isp_id].r_th_u,
			pdev_info->ca_th[isp_id].b_th_l, pdev_info->ca_th[isp_id].b_th_u, pdev_info->ca_th[isp_id].p_th_l, pdev_info->ca_th[isp_id].p_th_u);
		break;

	case ISP_SYNC_CAPTURE_CGAIN:
		memcpy((UINT32 *)data, &pdev_info->capture_cgain[isp_id][0], sizeof(UINT32) * 3);
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_CAPTURE, "ISP (%2d) Get CAPTURE_CGAIN (%d, %d, %d)\r\n", isp_id, pdev_info->capture_cgain[isp_id][0], pdev_info->capture_cgain[isp_id][1], pdev_info->capture_cgain[isp_id][2]);
		break;

	case ISP_SYNC_SIE_LA_ROI:
		memcpy((UINT32 *)data, &pdev_info->sie_la_roi[id], sizeof(URECT));
		break;

	default:
		break;
	}

	return E_OK;
}

void isp_dev_set_sync_item(ISP_ID id, ISP_SYNC_SEL sel, ISP_SYNC_ITEM item, void *data)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();
	BOOL emu_en = isp_api_get_emu_enable();
	UINT32 sync_id = 0, set_id = 0;
	UINT32 dbg_mode = isp_dbg_get_dbg_mode(id);

	if (pdev_info == NULL) {
		return;
	}
	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(dbg_mode & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return;
	}

	// NOTE: remove case of ipp
	#if 1
	//sync_id = (isp_sync_id_sie[id]) % SYNC_INFO_BUF;
	sync_id = ((sel == ISP_SYNC_SEL_CURRENT) || (emu_en == TRUE)) ? 0 : (isp_sync_id_sie[id]) % SYNC_INFO_BUF;
	#else
	switch (sel) {
	case ISP_SYNC_SEL_SIE:
		sync_id = (isp_sync_id_sie[id]) % SYNC_INFO_BUF;
		break;

	case ISP_SYNC_SEL_IPP:
		sync_id = (isp_sync_id_ipp[id]) % SYNC_INFO_BUF;
		break;

	default:
		sync_id = 0;
		break;
	}
	#endif

	item = ((item == ISP_SYNC_AWB_CGAIN) && (emu_en == TRUE)) ? ISP_SYNC_IQ_FINAL_CGAIN : item;

	set_id = sync_id;
	isp_sync_item_valid[id][item][set_id] = TRUE;

	switch (item) {
	// NOTE: Start of frame cnt SyncInfo
	case ISP_SYNC_AE_STATUS:
		memcpy(&pdev_info->sync_info[id][set_id].ae_status, (UINT32 *)data, sizeof(UINT32));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_AE_STATUS, "ISP (%2d) FrameCnt (%2d) Set (%2d) AE_STATUS (%d)\r\n", id, sync_id, set_id, pdev_info->sync_info[id][set_id].ae_status);
		break;

	case ISP_SYNC_AE_TOTAL_GAIN:
		memcpy(&pdev_info->sync_info[id][set_id].total_gain, (UINT32 *)data, sizeof(UINT32));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_TOTAL_GAIN, "ISP (%2d) FrameCnt (%2d) Set (%2d) TOTAL_GAIN (%d)\r\n", id, sync_id, set_id, pdev_info->sync_info[id][set_id].total_gain);
		break;

	case ISP_SYNC_AE_DGAIN:
		memcpy(&pdev_info->sync_info[id][set_id].dgain, (UINT32 *)data, sizeof(UINT32));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_D_GAIN, "ISP (%2d) FrameCnt (%2d) Set (%2d) DGAIN (%d)\r\n", id, sync_id, set_id, pdev_info->sync_info[id][set_id].dgain);
		break;

	case ISP_SYNC_AE_LV:
		memcpy(&pdev_info->sync_info[id][set_id].lv, (UINT32 *)data, sizeof(UINT32));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_LV, "ISP (%2d) FrameCnt (%2d) Set (%2d) LV (%d)\r\n", id, sync_id, set_id, pdev_info->sync_info[id][set_id].lv);
		break;

	case ISP_SYNC_AE_LV_BASE:
		memcpy(&pdev_info->sync_info[id][set_id].lv_base, (UINT32 *)data, sizeof(UINT32));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_LV, "ISP (%2d) FrameCnt (%2d) Set (%2d) LV_BASE (%d)\r\n", id, sync_id, set_id, pdev_info->sync_info[id][set_id].lv_base);
		break;

	case ISP_SYNC_AE_LA_ENABLE:
		memcpy(&pdev_info->sync_info[id][set_id].la_enable, (BOOL *)data, sizeof(BOOL));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_LA_ENABLE, "ISP (%2d) FrameCnt (%2d) Set (%2d) la_enable (%d)\r\n", id, sync_id, set_id, pdev_info->sync_info[id][set_id].la_enable);
		break;

	case ISP_SYNC_AWB_CGAIN:
		memcpy(&pdev_info->sync_info[id][set_id].cgain[0], (UINT32 *)data, sizeof(UINT32) * 3);
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_C_GAIN, "ISP (%2d) FrameCnt (%2d) Set (%2d) CGAIN(%d, %d, %d)\r\n"
			, id, sync_id, set_id, pdev_info->sync_info[id][set_id].cgain[0], pdev_info->sync_info[id][set_id].cgain[1], pdev_info->sync_info[id][set_id].cgain[2]);
		break;

	case ISP_SYNC_AWB_CT:
		memcpy(&pdev_info->sync_info[id][set_id].ct, (UINT32 *)data, sizeof(UINT32));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_CT, "ISP (%2d) FrameCnt (%2d) Set (%2d) CT (%d)\r\n", id, sync_id, set_id, pdev_info->sync_info[id][set_id].ct);
		break;

	case ISP_SYNC_AWB_CA_ENABLE:
		memcpy(&pdev_info->sync_info[id][set_id].ca_enable, (BOOL *)data, sizeof(BOOL));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_CA_ENABLE, "ISP (%2d) FrameCnt (%2d) Set (%2d) ca_enable (%d)\r\n", id, sync_id, set_id, pdev_info->sync_info[id][set_id].ca_enable);
		break;

	case ISP_SYNC_IQ_FINAL_CGAIN:
		memcpy(&pdev_info->sync_info[id][set_id].final_cgain[0], (UINT32 *)data, sizeof(UINT32) * 3);
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_FINAL_C_GAIN, "ISP (%2d) FrameCnt (%2d) Set (%2d) FINAL_CGAIN(%d, %d, %d)\r\n", id, sync_id, set_id
			, pdev_info->sync_info[id][set_id].final_cgain[0], pdev_info->sync_info[id][set_id].final_cgain[1], pdev_info->sync_info[id][set_id].final_cgain[2]);
		break;
	// NOTE: end of frame cnt SyncInfo

	case ISP_SYNC_CAPTURE_TOTAL_GAIN:
		memcpy(&pdev_info->capture_total_gain[id], (UINT32 *)data, sizeof(UINT32));
		//PRINT_ISP(dbg_mode & ISP_DBG_SYNC_CAPTURE, "ISP (%2d) Set CAPTURE_TOTAL_GAIN (%d)\r\n", id, pdev_info->capture_total_gain[id]);
		break;

	case ISP_SYNC_CAPTURE_DGAIN:
		memcpy(&pdev_info->capture_dgain[id], (UINT32 *)data, sizeof(UINT32));
		//PRINT_ISP(dbg_mode & ISP_DBG_SYNC_CAPTURE, "ISP (%2d) Set CAPTURE_DGAIN (%d)\r\n", id, pdev_info->capture_dgain[id]);
		break;

	case ISP_SYNC_SHDR_FRAME_NUM:
		memcpy(&pdev_info->shdr_frame_num[id], (UINT32 *)data, sizeof(UINT32));
		break;

	case ISP_SYNC_SHDR_EV_RATIO:
		memcpy(&pdev_info->shdr_ev_ratio[id], (UINT32 *)data, sizeof(UINT32) * ISP_SEN_MFRAME_MAX_NUM);
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_EV_RATIO, "ISP (%2d) Set EV_RATIO(%d, %d)\r\n", id, pdev_info->shdr_ev_ratio[id][0], pdev_info->shdr_ev_ratio[id][1]);
		break;

	case ISP_SYNC_AE_INIT:
		memcpy(&pdev_info->ae_init_info[id], (ISP_AE_INIT_INFO *)data, sizeof(ISP_AE_INIT_INFO));
		break;

	case ISP_SYNC_AWB_INIT:
		memcpy(&pdev_info->awb_init_info[id], (ISP_AWB_INIT_INFO *)data, sizeof(ISP_AWB_INIT_INFO));
		break;

	case ISP_SYNC_IQ_INIT:
		memcpy(&pdev_info->iq_init_info[id], (ISP_IQ_INIT_INFO *)data, sizeof(ISP_IQ_INIT_INFO));
		break;

	case ISP_SYNC_CA_TH:
		memcpy(&pdev_info->ca_th[id], (ISP_CA_TH_PARAM *)data, sizeof(ISP_CA_TH_PARAM));
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_CA_TH, "ISP (%2d), th_enable (%d), g(%d, %d), r(%d, %d), b(%d, %d), p(%d, %d)\r\n", id, pdev_info->ca_th[id].th_enable,
			pdev_info->ca_th[id].g_th_l, pdev_info->ca_th[id].g_th_u, pdev_info->ca_th[id].r_th_l, pdev_info->ca_th[id].r_th_u,
			pdev_info->ca_th[id].b_th_l, pdev_info->ca_th[id].b_th_u, pdev_info->ca_th[id].p_th_l, pdev_info->ca_th[id].p_th_u);
		break;

	case ISP_SYNC_CAPTURE_CGAIN:
		memcpy(&pdev_info->capture_cgain[id][0], (UINT32 *)data, sizeof(UINT32) * 3);
		PRINT_ISP(dbg_mode & ISP_DBG_SYNC_CAPTURE, "ISP (%2d) Set CAPTURE_CGAIN (%d, %d, %d)\r\n", id, pdev_info->capture_cgain[id][0], pdev_info->capture_cgain[id][1], pdev_info->capture_cgain[id][2]);
		break;

	case ISP_SYNC_SIE_LA_ROI:
		memcpy(&pdev_info->sie_la_roi[id], (UINT32 *)data, sizeof(URECT));
		break;

	default:
		break;
	}
}

void isp_dev_save_frame_count(UINT32 id)
{
	UINT32 sync_id = isp_sync_id_sie[id] % SYNC_INFO_BUF;
	BOOL emu_en = isp_api_get_emu_enable();
	UINT32 i;

	if (emu_en == FALSE) {
		isp_frame_count[id][sync_id] = isp_sync_id_sie[id];
	} else {
		for (i = 0; i < SYNC_INFO_BUF; i++) {
			isp_frame_count[id][i] = isp_sync_id_ipp[id];
		}
	}
}

ISP_CA_RSLT *isp_dev_get_ca(ISP_ID id)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (!isp_get_id_valid(id)) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "ca id_list not valid (%d) \r\n", id);
	}
	if (pdev_info == NULL) {
		return NULL;
	}
	if (pdev_info->ca_rslt[id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "ca result NULL (%d) \r\n", id);
		return NULL;
	}
	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return NULL;
	}
	return pdev_info->ca_rslt[id];
}

ISP_LA_RSLT *isp_dev_get_la(ISP_ID id)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (!isp_get_id_valid(id)) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "la id_list not valid (%d) \r\n", id);
	}
	if (pdev_info == NULL) {
		return NULL;
	}
	if (pdev_info->la_rslt[id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "la result NULL (%d) \r\n", id);
		return NULL;
	}
	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return NULL;
	}
	return pdev_info->la_rslt[id];
}

ISP_VA_RSLT *isp_dev_get_va(ISP_ID id)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (!isp_get_id_valid(id)) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "va id_list not valid (%d) \r\n", id);
	}
	if (pdev_info == NULL) {
		return NULL;
	}
	if (pdev_info->va_rslt[id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "va result NULL (%d) \r\n", id);
		return NULL;
	}
	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return NULL;
	}
	return pdev_info->va_rslt[id];
}

ISP_VA_INDEP_RSLT *isp_dev_get_va_indep(ISP_ID id)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (!isp_get_id_valid(id)) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "va id_list not valid (%d) \r\n", id);
	}
	if (pdev_info == NULL) {
		return NULL;
	}
	if (pdev_info->va_indep_rslt[id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "va indep result NULL (%d) \r\n", id);
		return NULL;
	}
	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return NULL;
	}
	return pdev_info->va_indep_rslt[id];
}

ISP_MD_RSLT *isp_dev_get_md(ISP_ID id)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (!isp_get_id_valid(id)) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "md id_list not valid (%d) \r\n", id);
	}
	if (pdev_info == NULL) {
		return NULL;
	}
	if (pdev_info->md_rslt[id] == NULL) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "md result NULL (%d) \r\n", id);
		return NULL;
	}
	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return NULL;
	}
	return pdev_info->md_rslt[id];
}


ISP_HISTO_RSLT *isp_dev_get_histo(ISP_ID id)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (!isp_get_id_valid(id)) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "ca id_list not valid (%d) \r\n", id);
	}
	if (pdev_info == NULL) {
		return NULL;
	}
	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return NULL;
	}

	if (pdev_info->is_capture[id] == TRUE) {
		if (pdev_info->histo_rslt_cap[pdev_info->cap_id_mapping_table[id]] == NULL) {
			PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "histo cap result NULL (%d) (%d) \r\n", id, pdev_info->cap_id_mapping_table[id]);
			return NULL;
		}
		return pdev_info->histo_rslt_cap[pdev_info->cap_id_mapping_table[id]];
	} else {
		if (pdev_info->histo_rslt[id] == NULL) {
			PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "histo result NULL (%d) \r\n", id);
			return NULL;
		}
		return pdev_info->histo_rslt[id];
	}
}

ISP_EDGE_STCS *isp_dev_get_edge_stcs(ISP_ID id)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (pdev_info == NULL) {
		return NULL;
	}
	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return NULL;
	}

	if (pdev_info->is_capture[id] == TRUE) {
		return &pdev_info->edge_stcs_cap[pdev_info->cap_id_mapping_table[id]];
	} else {
		return &pdev_info->edge_stcs[id];
	}
}

ISP_DEFOG_STCS *isp_dev_get_defog_stcs(ISP_ID id)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (pdev_info == NULL) {
		return NULL;
	}
	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return NULL;
	}

	if (pdev_info->is_capture[id] == TRUE) {
		return &pdev_info->defog_stcs_cap[pdev_info->cap_id_mapping_table[id]];
	} else {
		return &pdev_info->defog_stcs[id];
	}
}

ISP_IPE_SUBOUT_BUF *isp_dev_get_ipe_subout_buf(ISP_ID id)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (!isp_get_id_valid(id)) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "ca id_list not valid (%d) \r\n", id);
	}
	if (pdev_info == NULL) {
		return NULL;
	}
	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return NULL;
	}

	if (pdev_info->is_capture[id] == TRUE) {
		if (pdev_info->ipe_subout_buf_cap[pdev_info->cap_id_mapping_table[id]] == NULL) {
			PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "subout cap result NULL (%d) (%d) \r\n", id, pdev_info->cap_id_mapping_table[id]);
			return NULL;
		}
		return pdev_info->ipe_subout_buf_cap[pdev_info->cap_id_mapping_table[id]];
	} else {
		if (pdev_info->ipe_subout_buf[id] == NULL) {
			PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "subout result NULL (%d) \r\n", id);
			return NULL;
		}
		return pdev_info->ipe_subout_buf[id];
	}
}

CTL_MTR_DRV_TAB *isp_dev_get_mtr_drv_tab(void)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (pdev_info == NULL) {
		return NULL;
	}
	return pdev_info->mrt_drv_tab;
}

void isp_dev_reg_ae_module(ISP_MODULE *ae_module)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (pdev_info == NULL) {
		return;
	}
	pdev_info->isp_ae = ae_module;
}

void isp_dev_reg_af_module(ISP_MODULE *af_module)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (pdev_info == NULL) {
		return;
	}
	pdev_info->isp_af = af_module;
}

void isp_dev_reg_awb_module(ISP_MODULE *awb_module)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (pdev_info == NULL) {
		return;
	}
	pdev_info->isp_awb = awb_module;
}

void isp_dev_reg_dr_module(ISP_MODULE *dr_module)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (pdev_info == NULL) {
		return;
	}
	pdev_info->isp_dr = dr_module;
}

void isp_dev_reg_iq_module(ISP_MODULE *iq_module)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (pdev_info == NULL) {
		return;
	}
	pdev_info->isp_iq = iq_module;
}

void isp_dev_reg_motor_driver(CTL_MTR_DRV_TAB *mrt_drv_tab)
{
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();

	if (pdev_info == NULL) {
		return;
	}
	pdev_info->mrt_drv_tab = mrt_drv_tab;
}

void isp_dev_wait_vd(ISP_ID id, UINT32 timeout)
{
	#if defined(__KERNEL__)
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();
	DECLARE_WAITQUEUE(wait, current);
	INT32 time;

	if (pdev_info == NULL) {
		return;
	}
	if (id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", id);
		return;
	}

	add_wait_queue(&pdev_info->wait_vd[id], &wait);
	__set_current_state(TASK_INTERRUPTIBLE);
	if (timeout == 0) {
		timeout = WAIT_VD_TIMOUT;
	}
	time = schedule_timeout(msecs_to_jiffies(timeout));
	remove_wait_queue(&pdev_info->wait_vd[id], &wait);

	if (time == 0) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(id) & ISP_DBG_WRN_MSG, "wait vd timeout (%d) (%d) \r\n", id, timeout);
		return;
	}
	#endif
}

#if defined(__FREERTOS)
ISP_DEV_INFO *isp_get_dev_info(void)
{
	return &pdev_info;
}
#else
//=============================================================================
// internal functions
//=============================================================================
INT32 isp_dev_construct(ISP_DEV_INFO *pdev_info)
{
	INT32 ret = 0;
	UINT32 i;

	// initialize synchronization mechanism
	sema_init(&pdev_info->api_mutex, 1);
	sema_init(&pdev_info->proc_mutex, 1);
	sema_init(&pdev_info->ioc_mutex, 1);

	// initialize event waitqueue
	for (i = 0; i < ISP_ID_MAX_NUM; i++) {
		init_waitqueue_head(&pdev_info->wait_vd[i]);
	}

	// registed callback function
	isp_api_reg_if(isp_id_list);
	isp_api_reg_enc_if();

	return ret;
}

void isp_dev_deconstruct(ISP_DEV_INFO *pdev_info)
{
	isp_api_unreg_if();
	isp_api_unreg_enc_if();
}

EXPORT_SYMBOL(isp_dev_get_sync_item);
EXPORT_SYMBOL(isp_dev_set_sync_item);
EXPORT_SYMBOL(isp_dev_get_ca);
EXPORT_SYMBOL(isp_dev_get_la);
EXPORT_SYMBOL(isp_dev_get_va);
EXPORT_SYMBOL(isp_dev_get_va_indep);
EXPORT_SYMBOL(isp_dev_get_histo);
EXPORT_SYMBOL(isp_dev_get_edge_stcs);
EXPORT_SYMBOL(isp_dev_get_defog_stcs);
EXPORT_SYMBOL(isp_dev_get_ipe_subout_buf);
EXPORT_SYMBOL(isp_dev_get_mtr_drv_tab);
EXPORT_SYMBOL(isp_dev_reg_ae_module);
EXPORT_SYMBOL(isp_dev_reg_af_module);
EXPORT_SYMBOL(isp_dev_reg_awb_module);
EXPORT_SYMBOL(isp_dev_reg_dr_module);
EXPORT_SYMBOL(isp_dev_reg_iq_module);
EXPORT_SYMBOL(isp_dev_reg_motor_driver);

#endif
