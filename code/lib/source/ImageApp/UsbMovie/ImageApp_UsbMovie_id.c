#include "ImageApp_UsbMovie_int.h"

///// Cross file variables
ID IAUSBMOVIE_FLG_ID = 0;
///// Noncross file variables
static THREAD_HANDLE iausbmovie_acpull_tsk_id;
static THREAD_HANDLE iausbmovie_vepull_tsk_id;
static UINT32 acap_tsk_run, is_acap_tsk_running;
static UINT32 venc_tsk_run, is_venc_tsk_running;

#define PRI_IAUSBMOVIE_ACPULL          10
#define STKSIZE_IAUSBMOVIE_ACPULL      8192
#define PRI_IAUSBMOVIE_VEPULL          10
#define STKSIZE_IAUSBMOVIE_VEPULL      4096

#define PHY2VIRT_VSTRM(id, ch, pa) (usbimg_venc_bs_buf_va[id][ch] + (pa - usbimg_venc_bs_buf[id][ch].buf_info.phy_addr))
#define PHY2VIRT_ASTRM(id, ch, pa) (usbaud_acap_bs_buf_va[id]     + (pa - usbaud_acap_bs_buf[id].buf_info.phy_addr))

THREAD_RETTYPE _ImageApp_UsbMovie_ACapPullTsk(void)
{
	HD_RESULT hd_ret;
	FLGPTN uiFlag;
	HD_AUDIO_FRAME acap_data;
	UINT32 idx = 0;
	UVAC_STRM_FRM strmFrm = {0};

	THREAD_ENTRY();

	is_acap_tsk_running = TRUE;
	vos_flag_wait(&uiFlag, IAUSBMOVIE_FLG_ID, FLGIAUSBMOVIE_FLOW_LINK_CFG, TWF_ORW);

	while (acap_tsk_run) {
		vos_flag_wait(&uiFlag, IAUSBMOVIE_FLG_ID, FLGIAUSBMOVIE_AC_MASK, TWF_ORW);
		vos_flag_clr(IAUSBMOVIE_FLG_ID, FLGIAUSBMOVIE_TSK_AC_IDLE);
		if ((hd_ret = hd_audiocap_pull_out_buf(UsbAudCapLink[idx].acap_p[0], &acap_data, -1)) == HD_OK) {
			strmFrm.path = UVAC_STRM_AUD;
			strmFrm.addr = acap_data.phy_addr[0];
			strmFrm.va = PHY2VIRT_ASTRM(0, 0, acap_data.phy_addr[0]);
			strmFrm.size = acap_data.size;
			strmFrm.pStrmHdr = 0;
			strmFrm.strmHdrSize = 0;

			UVAC_SetEachStrmInfo(&strmFrm);
			// release data
			hd_ret = hd_audiocap_release_out_buf(UsbAudCapLink[idx].acap_p[0], &acap_data);
		} else {
			DBG_ERR("hd_audiocap_pull_out_buf() failed(%d)\r\n", hd_ret);
			vos_util_delay_ms(100);
		}
		vos_flag_set(IAUSBMOVIE_FLG_ID, FLGIAUSBMOVIE_TSK_AC_IDLE);
	}
	is_acap_tsk_running = FALSE;
	THREAD_RETURN(0);
}

THREAD_RETTYPE _ImageApp_UsbMovie_VEncPullTsk(void)
{
	HD_RESULT hd_ret;
	FLGPTN uiFlag;
	UINT32 venc_pull_cnt = 0, i, loop;
	HD_VIDEOENC_POLL_LIST venc_poll_list[MAX_USBIMG_PATH*1] = {0};
	HD_VIDEOENC_BS  venc_data;
	UVAC_STRM_FRM strmFrm = {0};

	THREAD_ENTRY();

	is_venc_tsk_running = TRUE;
	vos_flag_wait(&uiFlag, IAUSBMOVIE_FLG_ID, FLGIAUSBMOVIE_FLOW_LINK_CFG, TWF_ORW);
	for (i = 0; i < MaxUsbImgLink; i++) {
		venc_poll_list[venc_pull_cnt+0].path_id = UsbImgLink[i].venc_p[0];
		venc_pull_cnt ++;
	}
	while (venc_tsk_run) {
		vos_flag_wait(&uiFlag, IAUSBMOVIE_FLG_ID, FLGIAUSBMOVIE_VE_MASK, TWF_ORW);
		if ((hd_ret = hd_videoenc_poll_list(venc_poll_list, venc_pull_cnt, -1)) == HD_OK) {
			for (i = 0; i < venc_pull_cnt; i++) {
				if (venc_poll_list[i].revent.event == TRUE) {
					vos_flag_clr(IAUSBMOVIE_FLG_ID, FLGIAUSBMOVIE_TSK_VE_IDLE);
					if ((hd_ret = hd_videoenc_pull_out_buf(venc_poll_list[i].path_id, &venc_data, 0)) == HD_OK) {
						strmFrm.path = (i % 2 == 0) ? UVAC_STRM_VID : UVAC_STRM_VID2;
						strmFrm.addr = venc_data.video_pack[venc_data.pack_num-1].phy_addr;
						strmFrm.va   = PHY2VIRT_VSTRM((i % 2), 0, venc_data.video_pack[venc_data.pack_num-1].phy_addr);
						strmFrm.size = venc_data.video_pack[venc_data.pack_num-1].size;
						strmFrm.pStrmHdr = 0;
						strmFrm.strmHdrSize = 0;
						if (venc_data.pack_num > 1) {
							for (loop = 0; loop < venc_data.pack_num - 1; loop ++) {
								strmFrm.strmHdrSize += venc_data.video_pack[loop].size;
							}
							strmFrm.addr -= strmFrm.strmHdrSize;
							strmFrm.va   -= strmFrm.strmHdrSize;
							strmFrm.size += strmFrm.strmHdrSize;

						}
						UVAC_SetEachStrmInfo(&strmFrm);
						hd_ret = hd_videoenc_release_out_buf(venc_poll_list[i].path_id, &venc_data);
					}
					vos_flag_set(IAUSBMOVIE_FLG_ID, FLGIAUSBMOVIE_TSK_VE_IDLE);
				}
			}
		} else {
			DBG_ERR("hd_videoenc_poll_list() failed(%d)\r\n", hd_ret);
			vos_util_delay_ms(100);
		}
	}
	is_venc_tsk_running = FALSE;

	THREAD_RETURN(0);
}

ER _ImageApp_UsbMovie_InstallID(void)
{
	ER ret = E_OK;
	T_CFLG cflg ;

	memset(&cflg, 0, sizeof(T_CFLG));
	acap_tsk_run = FALSE;
	venc_tsk_run = FALSE;
	is_acap_tsk_running = FALSE;
	is_venc_tsk_running = FALSE;

	if ((ret |= vos_flag_create(&IAUSBMOVIE_FLG_ID, &cflg, "IAUSBMOVIE_FLG")) != E_OK) {
		DBG_ERR("IAUSBMOVIE_FLG_ID fail\r\n");
	}

	if ((iausbmovie_acpull_tsk_id = vos_task_create(_ImageApp_UsbMovie_ACapPullTsk, 0, "IAUsbMovie_ACPullTsk", PRI_IAUSBMOVIE_ACPULL, STKSIZE_IAUSBMOVIE_ACPULL)) == 0) {
		DBG_ERR("IAUsbMovie_AEPullTsk create failed.\r\n");
        ret |= E_SYS;
	} else {
		acap_tsk_run = TRUE;
	    vos_task_resume(iausbmovie_acpull_tsk_id);
	}

	if ((iausbmovie_vepull_tsk_id = vos_task_create(_ImageApp_UsbMovie_VEncPullTsk, 0, "IAUsbMovie_VEPullTsk", PRI_IAUSBMOVIE_VEPULL, STKSIZE_IAUSBMOVIE_VEPULL)) == 0) {
		DBG_ERR("IAUsbMovie_VEPullTsk create failed.\r\n");
        ret |= E_SYS;
	} else {
		venc_tsk_run = TRUE;
	    vos_task_resume(iausbmovie_vepull_tsk_id);
	}
	return ret;
}

ER _ImageApp_UsbMovie_UninstallID(void)
{
	ER ret = E_OK;
	UINT32 i, delay_cnt, atsk_status = FALSE, vtsk_status = FALSE;

	delay_cnt = 50;
	acap_tsk_run = FALSE;
	venc_tsk_run = FALSE;

	if (_LinkCheckStatus(UsbAudCapLinkStatus[0].acap_p[0])) {
		atsk_status = TRUE;
	}
	for (i = 0; i < MAX_USBIMG_PATH; i++) {
		if (_LinkCheckStatus(UsbImgLinkStatus[i].venc_p[0])) {
			vtsk_status = TRUE;
			break;
		}
	}
	if (atsk_status || vtsk_status) {
		while ((is_acap_tsk_running || is_venc_tsk_running) && delay_cnt) {
			vos_util_delay_ms(10);
			delay_cnt --;
		}
	}

	if (is_acap_tsk_running) {
		ImageApp_UsbMovie_DUMP("Destroy ACapPullTsk\r\n");
		vos_task_destroy(iausbmovie_acpull_tsk_id);
	}

	if (is_venc_tsk_running) {
		ImageApp_UsbMovie_DUMP("Destroy VEncPullTsk\r\n");
		vos_task_destroy(iausbmovie_vepull_tsk_id);
	}
	if (vos_flag_destroy(IAUSBMOVIE_FLG_ID) != E_OK) {
		DBG_ERR("IAUSBMOVIE_FLG_ID destroy failed.\r\n");
		ret |= E_SYS;
	}
	return ret;
}

