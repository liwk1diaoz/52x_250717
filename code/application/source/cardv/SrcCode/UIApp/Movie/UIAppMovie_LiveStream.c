#include "PrjInc.h"
#include "hdal.h"
#include "ImageApp/ImageApp_MovieMulti.h"
#include "nvtqrcode/nvtqrcode.h"
#include <kwrap/debug.h>
#include <kwrap/error_no.h>
#include <kwrap/task.h>
#include <kwrap/type.h>
#include <kwrap/util.h>
#include "nvtrtmpclient.h"

#define PRI_SCAN_QRCODE                21
#define STKSIZE_SCAN_QRCODE            4096
#define PRI_RUNRTMPCLIENT                21
#define STKSIZE_RUNRTMPCLIENT            4096

static THREAD_HANDLE movie_scan_qrcode_tsk_id;
static UINT32 movie_scan_qrcode_tsk_run, is_movie_scan_qrcode_tsk_running;
static UINT32 movie_scan_qrcode_result = FALSE;
static THREAD_HANDLE movie_runrtmpclient_tsk_id;
static UINT32 movie_runrtmpclient_tsk_run;
BOOL   g_bQRCodeScan = FALSE;

void MovieScanQRCodeAction(BOOL bQRCodeScan)
{
    if (bQRCodeScan){
		g_bQRCodeScan = TRUE;
    }else{
    	g_bQRCodeScan = FALSE;
    }
}

static THREAD_RETTYPE MovieScanQrcode_Tsk(void)
{
	HD_PATH_ID img_path;
	HD_VIDEO_FRAME video_frame = {0};
	HD_RESULT hd_ret;
	UINT32 err_cnt = 0;
    ER ret = E_SYS;
    char QrCodeBuf[3*128] = {0};
    char* split_request;
    UINT32 i = 0;
    NVTQRCODE_INFO QrCodeInfo = {0};

	THREAD_ENTRY();

	img_path = ImageApp_MovieMulti_GetAlgDataPort(_CFG_REC_ID_1, _CFG_ALG_PATH3);
	is_movie_scan_qrcode_tsk_running = TRUE;

	while (movie_scan_qrcode_tsk_run) {
		if ((UI_GetData(FL_COMMON_LIVESTREAM) == LIVESTREAM_ON) && (g_bQRCodeScan)) {
			if ((hd_ret = hd_videoproc_pull_out_buf(img_path, &video_frame, -1)) == HD_OK) {
				// process data
				QrCodeInfo.pVdoFrm = &video_frame;
                QrCodeInfo.pQrcodeBuf = (void *)QrCodeBuf;
                QrCodeInfo.u32QrcodeBufSize = sizeof(QrCodeBuf);
                QrCodeInfo.bEnDbgMsg = FALSE;
				ret = nvtqrcode_scan(&QrCodeInfo);
                if (ret == E_OK) {
                    DBG_DUMP("%s: QR code:%s\r\n", __func__, QrCodeBuf);
                    split_request = strtok(QrCodeBuf, "\n");
                    while(split_request != NULL) {
                       switch(i)
                       {
                            default:
                            case 0:
                                DBG_DUMP("SSID:");
                				sprintf(g_LivestreamHotSpot_SSID, "%s", split_request);
                                break;
                            case 1:
                                DBG_DUMP("PASSWD:");
                				sprintf(g_LivestreamHotSpot_PASSPHRASE, "%s", split_request);
                                break;
                            case 2:
                                DBG_DUMP("RTMP URL:");
                				sprintf(g_LivestreamRtmpAddress, "%s", split_request);
                                break;
                       }
                       DBG_DUMP("%s\r\n",split_request);  // SSID, PASSWD or RTMP URL.
                       split_request = strtok(NULL,"\n");

                       i++;
                    }
                    i = 0;
                    movie_scan_qrcode_tsk_run = FALSE;
                }

				// release data
				if ((hd_ret = hd_videoproc_release_out_buf(img_path, &video_frame)) != HD_OK) {
					DBG_ERR("hd_videoproc_release_out_buf fail(%d)\r\n", hd_ret);
				}
			} else {
				err_cnt ++;
				if (!(err_cnt % 30)) {
					DBG_ERR("hd_videoproc_pull_out_buf fail(%d),cnt = %d\r\n", hd_ret, err_cnt);
				}
			}
		} else {
			vos_util_delay_ms(300);
		}
	}
	is_movie_scan_qrcode_tsk_running = FALSE;

	THREAD_RETURN(0);
}

ER MovieScanQrcode_InstallID(void)
{
	ER ret = E_OK;

	is_movie_scan_qrcode_tsk_running = FALSE;

	if ((movie_scan_qrcode_tsk_id = vos_task_create(MovieScanQrcode_Tsk, 0, "MovieScanQrCodeTsk", PRI_SCAN_QRCODE, STKSIZE_SCAN_QRCODE)) == 0) {
		DBG_ERR("MovieAlgMDTsk create failed.\r\n");
        ret = E_SYS;
	} else {
		movie_scan_qrcode_tsk_run = TRUE;
	    vos_task_resume(movie_scan_qrcode_tsk_id);
	}
	return ret;
}

ER MovieScanQrcode_UninstallID(void)
{
	ER ret = E_OK;
	UINT32 delay_cnt;

	delay_cnt = 10;
	movie_scan_qrcode_tsk_run = FALSE;
	while (is_movie_scan_qrcode_tsk_running && delay_cnt) {
		vos_util_delay_ms(50);
		delay_cnt --;
	}

	if (is_movie_scan_qrcode_tsk_running) {
		DBG_DUMP("Destroy MovieScanQrCodeTsk\r\n");
		vos_task_destroy(movie_scan_qrcode_tsk_id);
	}

    is_movie_scan_qrcode_tsk_running = FALSE;
	return ret;
}

UINT32 MovieScanQrcode_GetResult(void)
{
	return movie_scan_qrcode_result;
}

static THREAD_RETTYPE MovieRunrtmpclient_Tsk(void)
{
    NVTRTMPCLIENT_HANDLER hRtmpClient= {0};

    if (E_OK != NvtRtmpClient_Open(g_LivestreamRtmpAddress, &hRtmpClient)) {
        DBG_ERR("Open fail !!!\n");
        goto runrtmpclient_exit;
    }

    if (E_OK != NvtRtmpClient_Start(hRtmpClient)) {
        DBG_ERR("Start fail !!!\n");
        goto runrtmpclient_exit;
    }

    movie_runrtmpclient_tsk_run = TRUE;

    while(movie_runrtmpclient_tsk_run) {
        vos_util_delay_ms(1000);
    }

    if (E_OK != NvtRtmpClient_Stop(hRtmpClient)) {
        DBG_ERR("Stop fail !!!\n");
        goto runrtmpclient_exit;
    }

    if (E_OK != NvtRtmpClient_Close(hRtmpClient)) {
        DBG_ERR("Close fail !!!\n");
        goto runrtmpclient_exit;
    }


runrtmpclient_exit:

    THREAD_RETURN(0);
}

ER MovieRunrtmpclient_InstallID(void)
{
	ER ret = E_OK;

	movie_runrtmpclient_tsk_run = FALSE;

	if ((movie_runrtmpclient_tsk_id = vos_task_create(MovieRunrtmpclient_Tsk, 0, "MovieRunrtmpclientTsk", PRI_RUNRTMPCLIENT, STKSIZE_RUNRTMPCLIENT)) == 0) {
		DBG_ERR("MovieRunrtmpclientTsk create failed.\r\n");
        ret = E_SYS;
	} else {
		movie_runrtmpclient_tsk_run = TRUE;
	    vos_task_resume(movie_runrtmpclient_tsk_id);
	}
	return ret;
}

ER MovieRunrtmpclient_UninstallID(void)
{
    return E_OK;
}