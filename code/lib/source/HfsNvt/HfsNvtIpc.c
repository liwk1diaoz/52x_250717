#include <stdio.h>
#include "kwrap/type.h"
#include "HfsNvtID.h"
#include "HfsNvtInt.h"
#include "HfsNvtTsk.h"
#include "HfsNvtIpc.h"
#include "NvtIpcAPI.h"
#include "HfsNvt/hfs.h"
//#include "dma_protected.h"


#if HFS_USE_IPC
#define IPC_MSGGET()                 NvtIPC_MsgGet(NvtIPC_Ftok(HFS_IPC_TOKEN_PATH))
#define IPC_MSGREL(msqid)            NvtIPC_MsgRel(msqid)
#define IPC_MSGSND(msqid,pMsg,msgsz) NvtIPC_MsgSnd(msqid,NVTIPC_SENDTO_CORE2,pMsg,msgsz)
#define IPC_MSGRCV(msqid,pMsg,msgsz) NvtIPC_MsgRcv(msqid,pMsg,msgsz)

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          HfsNvtIpc
#define __DBGLVL__          2
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "DebugModule.h"

static ER xHfsNvt_SendIPC(HFS_IPC_CMD ipc)
{
	HFS_IPC_MSG msg = {0};
	INT32       ret;

	msg.mtype = HFS_IPC_MSG_TYPE_C2S;
	msg.uiIPC = ipc;
	HFSNVT_CTRL *pCtrl = xHfsNvt_GetCtrl();

	if ((ret = IPC_MSGSND(pCtrl->msqid, &msg, sizeof(msg))) < 0) {
		DBG_ERR("msgsnd %d\r\n", ret);
		return E_SYS;
	}
	return E_OK;
}

void HfsNvt_Ipc_Open(HFSNVT_OPEN *pOpen)
{
    FLGPTN        uiFlag;
	int           buffsize;
	int           total_len = 0, len, remain_len;
	CHAR          startDaemonCmd[192];


    HFSNVT_CTRL*    pCtrl = xHfsNvt_GetCtrl();
    pCtrl->msqid = IPC_MSGGET();
    if (pCtrl->msqid < 0)
    {
        DBG_ERR("get msq fail\r\n",pCtrl->msqid);
    }
	// set up server start up command
	buffsize = sizeof(startDaemonCmd);
	remain_len = buffsize;
	len = snprintf(startDaemonCmd, buffsize, "hfs -p %d -l %d -s %d -to %d -t %d -ma %d -ms %d -n %d -r %s", pOpen->portNum, pOpen->httpsPortNum, pOpen->sockbufSize, pOpen->timeoutCnt, pOpen->threadPriority, dma_getPhyAddr(pOpen->sharedMemAddr), pOpen->sharedMemSize, pOpen->maxClientNum, pOpen->rootdir);
	if (len < 0 || len >= (buffsize - total_len)) {
		goto len_err;
	}
	total_len += len;
	remain_len -= len;
	startDaemonCmd[sizeof(startDaemonCmd) - 1] = 0;
	if (pOpen->check_pwd) {
		len = snprintf(&startDaemonCmd[total_len], remain_len, " -w");
		if (len < 0 || len >= (remain_len)) {
			goto len_err;
		}
		total_len += len;
		remain_len -= len;
		DBG_IND("total_len=%d\r\n", total_len);
	}
	if (pOpen->customStr[0]) {
		len = snprintf(&startDaemonCmd[total_len], remain_len, " -cs %s", pOpen->customStr);
		if (len < 0 || len >= (remain_len)) {
			goto len_err;
		}
		total_len += len;
		remain_len -= len;
		DBG_IND("total_len=%d\r\n", total_len);

	}
	if (pOpen->getCustomData) {
		len = snprintf(&startDaemonCmd[total_len], remain_len, " -c");
		if (len < 0 || len >= (remain_len)) {
			goto len_err;
		}
		total_len += len;
		remain_len -= len;
		DBG_IND("total_len=%d\r\n", total_len);
	}
	if (pOpen->putCustomData) {
		len = snprintf(&startDaemonCmd[total_len], remain_len, " -put");
		if (len < 0 || len >= (remain_len)) {
			goto len_err;
		}
		total_len += len;
		remain_len -= len;
		DBG_IND("total_len=%d\r\n", total_len);

	}
	if (pOpen->clientQuery) {
		len = snprintf(&startDaemonCmd[total_len], remain_len, " -q %s", pOpen->clientQueryStr);
		if (len < 0 || len >= (remain_len)) {
			goto len_err;
		}
		total_len += len;
		remain_len -= len;
		DBG_IND("total_len=%d\r\n", total_len);
	}
	if (pOpen->uploadResultCb) {
		len = snprintf(&startDaemonCmd[total_len], remain_len, " -u");
		if (len < 0 || len >= (remain_len)) {
			goto len_err;
		}
		total_len += len;
		remain_len -= len;
		DBG_IND("total_len=%d\r\n", total_len);
	}
	if (pOpen->downloadResultCb) {
		len = snprintf(&startDaemonCmd[total_len], remain_len, " -dl");
		if (len < 0 || len >= (remain_len)) {
			goto len_err;
		}
		total_len += len;
		remain_len -= len;
		DBG_IND("total_len=%d\r\n", total_len);
	}
	if (pOpen->deleteResultCb) {
		len = snprintf(&startDaemonCmd[total_len], remain_len, " -d");
		if (len < 0 || len >= (remain_len)) {
			goto len_err;
		}
		total_len += len;
		remain_len -= len;
		DBG_IND("total_len=%d\r\n", total_len);
	}
	if (pOpen->forceCustomCallback) {
		len = snprintf(&startDaemonCmd[total_len], remain_len, " -f");
		if (len < 0 || len >= (remain_len)) {
			goto len_err;
		}
		total_len += len;
		remain_len -= len;
		DBG_IND("total_len=%d\r\n", total_len);
	}
	if (pOpen->headerCb) {
		len = snprintf(&startDaemonCmd[total_len], remain_len, " -h");
		if (len < 0 || len >= (remain_len)) {
			goto len_err;
		}
		total_len += len;
		remain_len -= len;
		DBG_IND("total_len=%d\r\n", total_len);
	}
	len = snprintf(&startDaemonCmd[total_len], remain_len, " -v 0x%x &", HFS_INTERFACE_VER);
	if (len < 0 || len >= (remain_len)) {
		goto len_err;
	}
	total_len += len;
	remain_len -= len;
	DBG_IND("total_len=%d\r\n", total_len);
	sta_tsk(HFSNVT_TSK_RCV_ID, 0);
	// start cpu2 hfs app
    {
        INT32            ret;
        NVTIPC_SYS_MSG   sysMsg;

        sysMsg.sysCmdID = NVTIPC_SYSCMD_SYSCALL_REQ;
        sysMsg.DataAddr = (UINT32)&startDaemonCmd[0];
        sysMsg.DataSize = strlen(startDaemonCmd)+1;
        if ((ret = IPC_MSGSND(NVTIPC_SYS_QUEUE_ID, &sysMsg, sizeof(sysMsg))) < 0)
        {
            DBG_ERR("NvtIPC_MsgSnd %d\r\n",ret);
            return;
        }
    }
    // wait hfs started
    DBG_IND("wait hfs start\r\n");
    wai_flg(&uiFlag, pCtrl->FlagID, FLG_HFSNVT_SERVER_STARTED, TWF_ORW);
	DBG_IND("hfs started\r\n");
	return;
len_err:
	DBG_ERR("command too long, len=%d\r\n",len);
}

void HfsNvt_Ipc_Close(void)
{
    FLGPTN          uiFlag;
    //HFS_IPC_MSG     msg;
    HFSNVT_CTRL*    pCtrl = xHfsNvt_GetCtrl();

    xHfsNvt_SendIPC(HFS_IPC_CLOSE_SERVER);

	#if 0
    // notify cpu2 curl app to close
    msg.CmdId = HFS_IPC_CLOSE_SERVER;
    IPC_MSGSND(&msg);
	#endif
    // wait server exit.
    wai_flg(&uiFlag, pCtrl->FlagID, FLG_HFSNVT_SERVER_STOPPED, TWF_ORW|TWF_CLR);

    ter_tsk(HFSNVT_TSK_RCV_ID);
    if (IPC_MSGREL(pCtrl->msqid) != NVTIPC_ER_OK)
		DBG_ERR("NvtIPC_MsgRel\r\n");

}


void HfsNvt_Ipc_RcvTsk(void)
{
	HFS_IPC_MSG   msg;
	HFSNVT_CTRL *pCtrl;
	HFSNVT_OPEN *pOpen;
	BOOL bContinue = 1;
	INT32        ret;

	kent_tsk();
	DBG_FUNC_BEGIN("\r\n");
	pCtrl = xHfsNvt_GetCtrl();
	pOpen = &pCtrl->Open;

	while (bContinue) {
		PROFILE_TASK_IDLE();
		if ((ret = IPC_MSGRCV(pCtrl->msqid, &msg, sizeof(msg))) < 0) {
			DBG_ERR("msgrcv %d\r\n", ret);
			bContinue = 0;
			continue;
		}
		PROFILE_TASK_BUSY();
		DBG_IND("ipc_cmd=%d\r\n", msg.uiIPC);
		switch (msg.uiIPC) {
		case HFS_IPC_SERVER_STARTED:
			set_flg(pCtrl->FlagID, FLG_HFSNVT_SERVER_STARTED);
			break;
		case HFS_IPC_NOTIFY_CLIENT:
			if (pOpen->serverEvent) {
				HFS_IPC_NOTIFY_MSG     *notifymsg = (HFS_IPC_NOTIFY_MSG *)&msg;
				pOpen->serverEvent(notifymsg->notifyStatus);
			}
			break;
		case HFS_IPC_GET_CUSTOM_DATA: {
				HFS_IPC_GET_CUSTOM_S *pMsg = (HFS_IPC_GET_CUSTOM_S *)dma_getNonCacheAddr(msg.shareMem);


				if (pOpen->getCustomData) {
					pMsg->returnStatus = pOpen->getCustomData(pMsg->path, pMsg->argument, (UINT32)dma_getNonCacheAddr(pMsg->bufAddr), (UINT32 *)&pMsg->bufSize, pMsg->mimeType, pMsg->segmentCount);
				} else {
					pMsg->returnStatus = CYG_HFS_CB_GETDATA_RETURN_ERROR;
				}
				if (xHfsNvt_SendIPC(HFS_IPC_GET_CUSTOM_DATA_ACK) != E_OK) {
					bContinue = 0;
				}
				break;
			}
		case HFS_IPC_PUT_CUSTOM_DATA: {
				HFS_IPC_PUT_CUSTOM_S *pMsg = (HFS_IPC_PUT_CUSTOM_S *)dma_getNonCacheAddr(msg.shareMem);

				if (pOpen->putCustomData) {
					pMsg->returnStatus = pOpen->putCustomData(pMsg->path, pMsg->argument, (UINT32)dma_getNonCacheAddr(pMsg->bufAddr), (UINT32)pMsg->bufSize, pMsg->segmentCount, pMsg->putStatus);

				} else {
					pMsg->returnStatus = HFS_PUT_STATUS_ERR;
				}
				if (xHfsNvt_SendIPC(HFS_IPC_PUT_CUSTOM_DATA_ACK) != E_OK) {
					bContinue = 0;
				}
				break;
			}
		case HFS_IPC_CLIENT_QUERY_DATA: {
				HFS_IPC_CLIENT_QUERY_S *pMsg = (HFS_IPC_CLIENT_QUERY_S *)dma_getNonCacheAddr(msg.shareMem);

				if (pOpen->clientQuery) {
					pMsg->returnStatus = pOpen->clientQuery(pMsg->path, pMsg->argument, (UINT32)dma_getNonCacheAddr(pMsg->bufAddr), (UINT32 *)&pMsg->bufSize, pMsg->mimeType, pMsg->segmentCount);
				} else {
					pMsg->returnStatus = CYG_HFS_CB_GETDATA_RETURN_ERROR;
				}
				if (xHfsNvt_SendIPC(HFS_IPC_CLIENT_QUERY_DATA_ACK) != E_OK) {
					bContinue = 0;
				}
				break;
			}
		case  HFS_IPC_CHK_PASSWD: {
				HFS_IPC_CHK_PASSWD_S *pMsg = (HFS_IPC_CHK_PASSWD_S *)dma_getNonCacheAddr(msg.shareMem);
				if (pOpen->check_pwd) {
					pMsg->returnStatus = pOpen->check_pwd(pMsg->username, pMsg->password, pMsg->key, pMsg->questionstr);
				} else {
					pMsg->returnStatus = 1;
				}
				if (xHfsNvt_SendIPC(HFS_IPC_CHK_PASSWD_ACK) != E_OK) {
					bContinue = 0;
				}
				break;
			}

		case HFS_IPC_UPLOAD_RESULT_DATA: {
				HFS_IPC_UPLOAD_RESULT_S *pMsg = (HFS_IPC_UPLOAD_RESULT_S *)dma_getNonCacheAddr(msg.shareMem);

				if (pOpen->uploadResultCb) {
					pMsg->returnStatus = pOpen->uploadResultCb(pMsg->uploadResult, (UINT32)dma_getNonCacheAddr(pMsg->bufAddr), (UINT32 *)&pMsg->bufSize, pMsg->mimeType);
				} else {
					pMsg->returnStatus = CYG_HFS_CB_GETDATA_RETURN_ERROR;
				}
				if (xHfsNvt_SendIPC(HFS_IPC_UPLOAD_RESULT_DATA_ACK) != E_OK) {
					bContinue = 0;
				}
				break;
			}
		case HFS_IPC_DOWNLOAD_RESULT_DATA: {
				HFS_IPC_DOWNLOAD_RESULT_S *pMsg = (HFS_IPC_DOWNLOAD_RESULT_S *)dma_getNonCacheAddr(msg.shareMem);

				if (pOpen->downloadResultCb) {
					pOpen->downloadResultCb(pMsg->downloadResult, pMsg->path);
				}
				if (xHfsNvt_SendIPC(HFS_IPC_DOWNLOAD_RESULT_DATA_ACK) != E_OK) {
					bContinue = 0;
				}
				break;
			}
		case HFS_IPC_DELETE_RESULT_DATA: {
				HFS_IPC_DELETE_RESULT_S *pMsg = (HFS_IPC_DELETE_RESULT_S *)dma_getNonCacheAddr(msg.shareMem);

				if (pOpen->deleteResultCb) {
					pMsg->returnStatus = pOpen->deleteResultCb(pMsg->deleteResult, (UINT32)dma_getNonCacheAddr(pMsg->bufAddr), (UINT32 *)&pMsg->bufSize, pMsg->mimeType);
				} else {
					pMsg->returnStatus = CYG_HFS_CB_GETDATA_RETURN_ERROR;
				}
				if (xHfsNvt_SendIPC(HFS_IPC_DELETE_RESULT_DATA_ACK) != E_OK) {
					bContinue = 0;
				}
				break;
			}
		case HFS_IPC_HEADER_CB: {
				HFS_IPC_HEADER_S *pMsg = (HFS_IPC_HEADER_S *)dma_getNonCacheAddr(msg.shareMem);

				if (pOpen->headerCb) {
					pMsg->returnStatus = pOpen->headerCb((UINT32)dma_getNonCacheAddr(pMsg->headerAddr), (UINT32)pMsg->headerSize, pMsg->filepath, pMsg->mimeType, 0);
				} else {
					pMsg->returnStatus = CYG_HFS_CB_HEADER_RETURN_ERROR;
				}
				if (xHfsNvt_SendIPC(HFS_IPC_HEADER_CB_ACK) != E_OK) {
					bContinue = 0;
				}
				break;
			}
		case HFS_IPC_CLOSE_SERVER_ACK:
			bContinue = 0;
			xHfsNvt_SendIPC(HFS_IPC_CLOSE_FINISH);
			break;
		default:
			DBG_ERR("ipc_cmd=%d\r\n", msg.uiIPC);
			break;
		}
	}
	set_flg(pCtrl->FlagID, FLG_HFSNVT_SERVER_STOPPED);
	DBG_FUNC_END("\r\n");
	ext_tsk();
}
#endif



