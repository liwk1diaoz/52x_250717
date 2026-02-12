#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFileList.h"
#include "hdal.h"
#include "hd_type.h"

PB_ERR PB_Close(PB_WAIT_MODE TimeOut)
{
	FLGPTN          PlayCMD;
    HD_RESULT hd_ret;
	PPBX_FLIST_OBJ  pFlist = g_PBSetting.pFileListObj;

	if (!guiOnPlaybackMode) {
		return PBERR_FAIL;
	}

	if (TimeOut == PB_WAIT_INFINITE) {
		wai_flg(&PlayCMD, FLG_PB, FLGPB_NOTBUSY, TWF_ORW | TWF_CLR);
	}
	// un-init file list
	if (pFlist) {
		pFlist->UnInit();
	}
	// Playback system un-initial
	PB_SysUnInit();

	THREAD_DESTROY(PLAYBAKTSK_ID);
	THREAD_DESTROY(PLAYBAKDISPTSK_ID);

	PB_UninstallID();

	guiOnPlaybackMode = FALSE;
	gucPlayTskWorking = FALSE;

	hd_ret = hd_common_mem_munmap((void *)g_hd_file_buf.va, g_hd_file_buf.blk_size);
	if (hd_ret != HD_OK){
		DBG_ERR("munmap g_hd_file_buf.va failed\r\n");
		return PBERR_FAIL;
	}
	hd_ret = hd_common_mem_munmap((void *)g_hd_raw_buf.va, g_hd_raw_buf.blk_size);
	if (hd_ret != HD_OK){
		DBG_ERR("munmap g_hd_raw_buf.va failed\r\n");
		return PBERR_FAIL;
	}
	hd_ret = hd_common_mem_munmap((void *)g_hd_tmp1buf.va, g_hd_tmp1buf.blk_size);
	if (hd_ret != HD_OK){
		DBG_ERR("munmap g_hd_tmp1buf.va failed\r\n");
		return PBERR_FAIL;
	}
	hd_ret = hd_common_mem_munmap((void *)g_hd_tmp2buf.va, g_hd_tmp2buf.blk_size);
	if (hd_ret != HD_OK){
		DBG_ERR("munmap g_hd_tmp2buf.va failed\r\n");
		return PBERR_FAIL;
	}
	hd_ret = hd_common_mem_munmap((void *)g_hd_tmp3buf.va, g_hd_tmp3buf.blk_size);
	if (hd_ret != HD_OK){
		DBG_ERR("munmap g_hd_tmp3buf.va failed\r\n");
		return PBERR_FAIL;
	}
	hd_ret = hd_common_mem_munmap((void *)g_hd_exif_buf.va, g_hd_exif_buf.blk_size);
	if (hd_ret != HD_OK){
		DBG_ERR("munmap g_hd_exif_buf.va failed\r\n");
		return PBERR_FAIL;
	}
	hd_ret = hd_common_mem_munmap((void *)g_PBSetting.VidDecryptBuf.va, g_PBSetting.VidDecryptBuf.blk_size);
	if (hd_ret != HD_OK){
		DBG_ERR("munmap g_PBSetting.VidDecryptBuf.addr failed\r\n");
		return PBERR_FAIL;
	}

    hd_ret = hd_common_mem_release_block(g_hd_file_buf.blk);
	if (hd_ret != HD_OK){
		DBG_ERR("release g_hd_file_buf.blk failed\r\n");
		return PBERR_FAIL;
	}
	hd_ret = hd_common_mem_release_block(g_hd_raw_buf.blk);
	if (hd_ret != HD_OK){
		DBG_ERR("release g_hd_raw_buf.blk failed\r\n");
		return PBERR_FAIL;
	}
	hd_ret = hd_common_mem_release_block(g_hd_tmp1buf.blk);
	if (hd_ret != HD_OK){
		DBG_ERR("release g_hd_tmp1buf.blk failed\r\n");
		return PBERR_FAIL;
	}
	hd_ret = hd_common_mem_release_block(g_hd_tmp2buf.blk);
	if (hd_ret != HD_OK){
		DBG_ERR("release g_hd_tmp2buf.blk failed\r\n");
		return PBERR_FAIL;
	}
	hd_ret = hd_common_mem_release_block(g_hd_tmp3buf.blk);
	if (hd_ret != HD_OK){
		DBG_ERR("release g_hd_tmp3buf.blk failed\r\n");
		return PBERR_FAIL;
	}
	hd_ret = hd_common_mem_release_block(g_hd_exif_buf.blk);
	if (hd_ret != HD_OK){
		DBG_ERR("release g_hd_exif_buf.blk failed\r\n");
		return PBERR_FAIL;
	}
	hd_ret = hd_common_mem_release_block(g_PBSetting.VidDecryptBuf.blk);
	if (hd_ret != HD_OK){
		DBG_ERR("release g_PBSetting.VidDecryptBuf.blk failed\r\n");
		return PBERR_FAIL;
	}

	return PBERR_OK;
}
