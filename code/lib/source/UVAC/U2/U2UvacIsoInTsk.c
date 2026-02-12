#include "U2UvacDbg.h"
#include "U2UvacIsoInTsk.h"
#include "U2UvacVideoTsk.h"
#include "U2UvacDesc.h"
#include "UVAC.h"
#include "U2UvacID.h"
#include "hd_common.h"
#include "timer.h"

#define loc_cpu() wai_sem(SEMID_U2UVC_QUEUE)
#define unl_cpu() sig_sem(SEMID_U2UVC_QUEUE)

//====== global variable ======
const USB_EP U2UVAC_USB_INTR_EP[UVAC_EP_INTR_MAX] = {USB_EP5, USB_EP6};
const USB_EP U2UVAC_USB_EP[UVAC_TXF_QUE_MAX] = {USB_EP1, USB_EP2, USB_EP3, USB_EP4, USB_EP7};
const USB_EP U2UVAC_USB_RX_EP[UVAC_RXF_QUE_MAX] = {USB_EP7};
UINT32 gU2UvcMaxTxfSizeForUsb[UVAC_TXF_QUE_MAX] = {USB_MAX_TXF_SIZE_LIMIT, USB_MAX_TXF_SIZE_LIMIT, USB_MAX_TXF_SIZE_LIMIT, USB_MAX_TXF_SIZE_LIMIT, USB_MAX_TXF_SIZE_LIMIT};
UVAC_TXF_INFO gU2UvacTxfQue[UVAC_TXF_QUE_MAX][UVAC_TXF_MAX_QUECNT];//4*60*16=4800=4.69k
UVC_ISOIN_TXF_STATE gU2UvacIsoTxfState = UVC_ISOIN_TXF_STATE_STOP;
UINT32 gU2UvacTxfQueCurCnt[UVAC_TXF_QUE_MAX];
UINT32 gU2UvacTxfQueFrontIdx[UVAC_TXF_QUE_MAX];
UINT32 gU2UvacTxfQueRearIdx[UVAC_TXF_QUE_MAX];
UINT32 gU2UvcUsbDMAAbord = FALSE;
UINT32 gU2UacUsbDMAAbord = FALSE;
UINT32 gU2UvcNoVidStrm = FALSE;
extern FLGPTN gUvacRunningFlag[UVAC_VID_DEV_CNT_MAX];

_ALIGNED(4) UINT8 gU2UvcIsoInHdr[12] = {0x0C, 0x8F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//----------------------
extern UINT32 gU2UacAudStart[UVAC_AUD_DEV_CNT_MAX];
extern UINT32 gU2UvcVidStart[UVAC_VID_DEV_CNT_MAX];
extern UINT32 gU2UVCIsoinTxfUnitSize[UVAC_VID_DEV_CNT_MAX];
extern UINT8  gU2UvacStillImgTrigSts;
extern UINT32 gU2UvcHWPayload[UVAC_VID_DEV_CNT_MAX];
extern UINT32 gU2UvacChannel;
extern BOOL gU2UvacCdcEnabled;
extern UINT32 gU2UvacAudSampleRate[UVAC_AUD_SAMPLE_RATE_MAX_CNT];
extern UINT32 gU2UacChNum;
extern TIMER_ID gUacTimerID[UVAC_AUD_DEV_CNT_MAX];
extern UINT16 gUacMaxPacketSize;
#if 0//gU2UvacMtpEnabled
extern BOOL gU2UvacMtpEnabled;
#endif

#if (ISF_LATENCY_DEBUG)
typedef struct _UVAC_TIMESTAMP_INFO_ {
	UINT64 timestamp;
	UINT64 uvc_timestamp_start;
	UINT64 uvc_timestamp_end;
} UVAC_TIMESTAMP_INFO;
#define ISF_UVAC_MAX_ISF_DEBUG_NUM  60
static UVAC_TIMESTAMP_INFO g_isf_uvac_timestamp[ISF_UVAC_MAX_ISF_DEBUG_NUM];
static UINT32 g_isf_uvac_timestamp_idx = 0;
static BOOL is_isf_dump = FALSE;

#endif

#if (ISF_AUDIO_LATENCY_DEBUG)
#define ISF_UVAC_AUD_MAX_ISF_DEBUG_NUM  90
typedef struct _UVAC_AUD_TIMESTAMP_INFO_ {
	UINT64 timestamp;
	UINT64 uvc_timestamp_start;
	UINT64 uvc_timestamp_write;
	UINT64 uvc_timestamp_end;
	UINT32 uvc_slice_size;
} UVAC_AUD_TIMESTAMP_INFO;
static UVAC_AUD_TIMESTAMP_INFO g_isf_uvac_aud_timestamp[ISF_UVAC_AUD_MAX_ISF_DEBUG_NUM];
static UINT32 g_isf_uvac_aud_timestamp_idx = 0;
static BOOL is_aud_isf_dump = FALSE;

#endif

#if 1//(_UVC_DBG_LVL_ > _UVC_DBG_CHK_)
void U2UVAC_DbgDmp_TxfInfoByQue(UINT32 queIdx)
{
	UINT32 i = 0;
	UVAC_TXF_INFO *pTxfInfo;
	DBG_DUMP("%s ==>\r\n", __func__);
	DbgMsg_UVCIO(("Dmp Que[%d]:f=0x%x,r=0x%x,cnt=%d\r\n", queIdx, gU2UvacTxfQueFrontIdx[queIdx], gU2UvacTxfQueRearIdx[queIdx], gU2UvacTxfQueCurCnt[queIdx]));
	for (i = 0; i < UVAC_TXF_MAX_QUECNT; i++) {
		pTxfInfo = &gU2UvacTxfQue[queIdx][i];
		DBG_DUMP("[%d]=0x%x,0x%x,%d,ep=%d,timestamp=%llu\r\n",i,pTxfInfo->sAddr,pTxfInfo->size,pTxfInfo->txfCnt,pTxfInfo->usbEP,pTxfInfo->timestamp);
	}
	DBG_DUMP("<===========\r\n");
}
void U2UVAC_DbgDmp_ChkIsoInQueEmpty(UINT32 queIdx)
{
	DBG_DUMP("%s ==>\r\n", __func__);
	switch (queIdx) {
	case UVAC_TXF_QUE_V1:
		if (gU2UvcVidStart[0]) {
			DBG_DUMP("Txf[%d] EMP\r\n", queIdx);
		}
		break;
	case UVAC_TXF_QUE_V2:
		if (gU2UvcVidStart[1]) {
			DBG_DUMP("Txf[%d] EMP\r\n", queIdx);
		}
		break;
	case UVAC_TXF_QUE_A1:
		if (gU2UacAudStart[0]) {
			DBG_DUMP("Txf[%d] EMP\r\n", queIdx);
		}
		break;
	case UVAC_TXF_QUE_A2:
		if (gU2UacAudStart[1]) {
			DBG_DUMP("Txf[%d] EMP\r\n", queIdx);
		}
		break;
	case UVAC_TXF_QUE_IMG:
		DBG_DUMP("ImgTxf[%d] EMP\r\n", queIdx);
		break;
	default:
		DBG_DUMP("Unknow Que=%d\r\n", queIdx);
		break;
	}
	DBG_DUMP("<===========\r\n");
}
void U2UVAC_DbgDmp_TxfEPInfo(void)
{
	UINT32 i = 0;
	DBG_DUMP("%s:HWPayload=%d,%d ==>\r\n", __func__, gU2UvcHWPayload[UVAC_VID_DEV_CNT_1], gU2UvcHWPayload[UVAC_VID_DEV_CNT_2]);
	for (i = 0; i < UVAC_TXF_QUE_MAX; i++) {
		DBG_DUMP("EP[%d]=%d, txf-size-limit=0x%x\r\n", i, U2UVAC_USB_EP[i], gU2UvcMaxTxfSizeForUsb[i]);
	}
	DBG_DUMP("<===========\r\n");
}
void U2UVAC_DbgDmp_ChkTxfQueAlmostFull(UINT32 queIdx, UINT32 level, BOOL dmpFirst, BOOL dmpQue)
{
	DBG_DUMP("%s:Que[%d]=%d,max=%d,dmp=%d,%d ==>\r\n", __func__, queIdx, level, UVAC_TXF_MAX_QUECNT, dmpFirst, dmpQue);
	if (gU2UvacTxfQueCurCnt[queIdx] > level) {
		UINT32 itemIdx = 0;
		UVAC_TXF_INFO *pTxfInfo;
		DBG_DUMP("  Que[%d]=%d > %d \r\n", queIdx, gU2UvacTxfQueCurCnt[queIdx], level);
		if (dmpFirst) {
			itemIdx = gU2UvacTxfQueFrontIdx[queIdx];
			pTxfInfo = &gU2UvacTxfQue[queIdx][itemIdx];
			DBG_DUMP("EP[%d]=%d,queCnt=%d,size=0x%x,txfCnt=%d,txfLimit=0x%x\r\n", queIdx, U2UVAC_USB_EP[queIdx], gU2UvacTxfQueCurCnt[queIdx], pTxfInfo->size, pTxfInfo->txfCnt, gU2UvcMaxTxfSizeForUsb[queIdx]);
		}
		if (dmpQue) {
			U2UVAC_DbgDmp_TxfInfoByQue(queIdx);
		}
	}
	DBG_DUMP("<===========\r\n");
}
#else
void U2UVAC_DbgDmp_TxfInfoByQue(UINT32 queIdx) {}
void U2UVAC_DbgDmp_ChkIsoInQueEmpty(UINT32 queIdx) {}
void U2UVAC_DbgDmp_TxfEPInfo(void) {}
void U2UVAC_DbgDmp_ChkTxfQueAlmostFull(UINT32 queIdx, UINT32 level, BOOL dmpFirst, BOOL dmpQue) {}
#endif

static __inline UINT32 UVAC_GetTxfQueIdx(UINT8 ep)
{
	if (USB_EP1 == ep) {
		return UVAC_TXF_QUE_V1;
	} else if (USB_EP2 == ep) {
		return UVAC_TXF_QUE_V2;
	} else if (USB_EP3 == ep) {
		return UVAC_TXF_QUE_A1;
	} else if (USB_EP4 == ep) {
		return UVAC_TXF_QUE_A2;
	} else if (USB_EP7 == ep) {
		return UVAC_TXF_QUE_IMG;
	} else {
		DBG_ERR("^RUnknown EP=%d,uvac channel=%d\r\n", ep, gU2UvacChannel);
		return UVAC_TXF_QUE_MAX;
	}
}
static BOOL UVAC_IsoTxDbg(UINT32 queIdx, UINT32 flag)
{
	if (((1 << (queIdx + 8)) & m_uiU2UvacDbgIso) && (flag & m_uiU2UvacDbgIso)) {
		return TRUE;
	} else {
		return FALSE;
	}

}
static void UVAC_UpdateTxfQue(UVAC_TXF_INFO *pTxfInfo)
{
	UINT32 queIdx;

	if (pTxfInfo) {
		queIdx = UVAC_GetTxfQueIdx(pTxfInfo->usbEP);
		if (queIdx >= UVAC_TXF_QUE_MAX) {
			return;
		}
		DbgMsg_UVCIO(("+[%d]=%d,f=%d,%d\r\n", queIdx, gU2UvacTxfQueCurCnt[queIdx], gU2UvacTxfQueFrontIdx[queIdx], gU2UvacTxfQueRearIdx[queIdx]));
		loc_cpu();
		gU2UvacTxfQueCurCnt[queIdx]--;
		gU2UvacTxfQueFrontIdx[queIdx] = ((gU2UvacTxfQueFrontIdx[queIdx] + 1) % UVAC_TXF_MAX_QUECNT);
		unl_cpu();
		DbgMsg_UVCIO(("-[%d]=%d,f=%d,%d\r\n", queIdx, gU2UvacTxfQueCurCnt[queIdx], gU2UvacTxfQueFrontIdx[queIdx], gU2UvacTxfQueRearIdx[queIdx]));
	} else {
		DBG_ERR("^R%s pTxfInfo NULL\r\n", __func__);
	}
}
void U2UVAC_IsoInTxfStateMachine(UVC_ISOIN_TXF_ACT txfAct)
{
	DbgMsg_UVCIO(("IsoSts:%d,prvSts=%d,abort=%d,NovidSrm=%d\r\n", txfAct, gU2UvacIsoTxfState, gU2UvcUsbDMAAbord, gU2UvcNoVidStrm));
	if (UVC_ISOIN_TXF_ACT_START == txfAct) {
		//set_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_START);
		set_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_TXF);
	} else if (UVC_ISOIN_TXF_ACT_TXF == txfAct) {
		set_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_TXF);
	} else if (UVC_ISOIN_TXF_ACT_STOP == txfAct) {
		set_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_STOP);
	}
}
void U2UVAC_ResetTxfPara(void)
{
	DbgMsg_UVC(("+%s:\r\n", __func__));

	memset((void *)&gU2UvacTxfQue[0][0], 0, (UVAC_TXF_QUE_MAX * UVAC_TXF_MAX_QUECNT * sizeof(UVAC_TXF_INFO)));
	gU2UvacIsoTxfState = UVC_ISOIN_TXF_STATE_STOP;
	memset((void *)&gU2UvacTxfQueCurCnt[0], 0, sizeof(UINT32)*UVAC_TXF_QUE_MAX);
	memset((void *)&gU2UvacTxfQueFrontIdx[0], 0, sizeof(UINT32)*UVAC_TXF_QUE_MAX);
	memset((void *)&gU2UvacTxfQueRearIdx[0], 0, sizeof(UINT32)*UVAC_TXF_QUE_MAX);

	gU2UVCIsoinTxfUnitSize[UVAC_VID_DEV_CNT_1] = gU2UvcIsoInHsPacketSize * gU2UvcIsoInBandWidth;    //must be > PAYLOAD_LEN

	if (gU2UVCIsoinTxfUnitSize[UVAC_VID_DEV_CNT_1] <= PAYLOAD_LEN) {
		DBG_ERR("TxfUnitSize=%d <= %d Error !!\r\n", gU2UVCIsoinTxfUnitSize, PAYLOAD_LEN);
	}
	gU2UvcMaxTxfSizeForUsb[UVAC_TXF_QUE_V1] = USB_MAX_TXF_SIZE_LIMIT;

	if (gU2UvacChannel > UVAC_CHANNEL_1V1A) {
		gU2UvcMaxTxfSizeForUsb[UVAC_TXF_QUE_V2] = USB_MAX_TXF_SIZE_LIMIT;
		gU2UVCIsoinTxfUnitSize[UVAC_VID_DEV_CNT_2] = UVC_USB_FIFO_UNIT_SIZE; 
	}
	DbgMsg_UVC(("-%s:HWPayL=%d,%d,txfS=0x%x,0x%x,unitS[0]=0x%x,unitS[1]=0x%x\r\n", __func__, gU2UvcHWPayload[UVAC_VID_DEV_CNT_1], gU2UvcHWPayload[UVAC_VID_DEV_CNT_2], gU2UvcMaxTxfSizeForUsb[0], gU2UvcMaxTxfSizeForUsb[1], gU2UVCIsoinTxfUnitSize[UVAC_VID_DEV_CNT_1], gU2UVCIsoinTxfUnitSize[UVAC_VID_DEV_CNT_2]));
}
UINT32 U2UVAC_RemoveTxfInfo(UVAC_TXF_QUE queIdx)
{
	UINT32 i = 0;
	UINT32 removeCnt = 0;

	if (queIdx >= UVAC_TXF_QUE_MAX) {
		DBG_IND("Clr all TxfInfo:%d\r\n", queIdx);
		loc_cpu();
		for (i = 0; i < UVAC_TXF_QUE_MAX; i++) {
			removeCnt += gU2UvacTxfQueCurCnt[i];
			gU2UvacTxfQueCurCnt[i] = 0;
			gU2UvacTxfQueFrontIdx[i] = 0;
			gU2UvacTxfQueRearIdx[i] = 0;
		}
		memset((void *)&gU2UvacTxfQue[0][0], 0, (sizeof(UVAC_TXF_INFO)*UVAC_TXF_MAX_QUECNT * UVAC_TXF_QUE_MAX));
		unl_cpu();
		vos_flag_set(FLG_ID_U2UVAC_FRM, (FLGUVAC_FRM_V1 | FLGUVAC_FRM_V2 | FLGUVAC_FRM_A1 | FLGUVAC_FRM_A2));
	} else {
		loc_cpu();
		removeCnt = gU2UvacTxfQueCurCnt[queIdx];
		gU2UvacTxfQueCurCnt[queIdx] = 0;
		gU2UvacTxfQueFrontIdx[queIdx] = 0;
		gU2UvacTxfQueRearIdx[queIdx] = 0;
		memset((void *)&gU2UvacTxfQue[queIdx][0], 0, (sizeof(UVAC_TXF_INFO)*UVAC_TXF_MAX_QUECNT));
		DbgMsg_UVC(("DelQue=%d,cnt=%d\r\n", queIdx, removeCnt));
		//U2UVAC_DbgDmp_TxfInfoByQue(queIdx);
		unl_cpu();
		if (queIdx == UVAC_TXF_QUE_V1) {
			vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V1);
		} else if (queIdx == UVAC_TXF_QUE_V2) {
			vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V2);
		} else if (queIdx == UVAC_TXF_QUE_A1) {
			vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A1);
		} else if (queIdx == UVAC_TXF_QUE_A2) {
			vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A2);
		}
	}
	DbgMsg_UVCIO(("-%s:idx=%d,delC=%d\r\n", __func__, queIdx, removeCnt));
	return removeCnt;
}
static UVAC_TXF_INFO *UVAC_GetFreeTxfInfo(UVAC_TXF_QUE queIdx)
{
	UVAC_TXF_INFO *pTxfInfo = 0;;
	UINT32 idx = 0;

	DbgMsg_UVCIO(("+%s:Que[%d],fIdx=%d,%d,cnt=%d\r\n", __func__, queIdx, gU2UvacTxfQueFrontIdx[queIdx], gU2UvacTxfQueRearIdx[queIdx], gU2UvacTxfQueCurCnt[queIdx]));

	if (queIdx >= UVAC_TXF_QUE_MAX) {
		DBG_ERR("Over txfInfoQueCnt=%d > %d\r\n", queIdx, UVAC_TXF_QUE_MAX);
		return pTxfInfo;
	}
	if (gU2UvacTxfQueCurCnt[queIdx] < UVAC_TXF_MAX_QUECNT) {
		DbgMsg_UVCIO(("+AddTxfQue[%d],curCnt=%d,f=%d,%d\r\n", queIdx, gU2UvacTxfQueCurCnt[queIdx], gU2UvacTxfQueFrontIdx[queIdx], gU2UvacTxfQueRearIdx[queIdx]));
		idx = gU2UvacTxfQueRearIdx[queIdx];
		pTxfInfo = &gU2UvacTxfQue[queIdx][idx];

		DbgCode_UVC(if (0 == pTxfInfo) DBG_DUMP("TxfQue[%d]:f=%d,%d,cnt=%d,pTxfInfo=0x%x\r\n", queIdx, gU2UvacTxfQueFrontIdx[queIdx], gU2UvacTxfQueRearIdx[queIdx], gU2UvacTxfQueCurCnt[queIdx], pTxfInfo);)

		{
			idx = (idx + 1) % UVAC_TXF_MAX_QUECNT;
		}
		gU2UvacTxfQueRearIdx[queIdx] = idx;
		//loc_cpu();
		gU2UvacTxfQueCurCnt[queIdx]++;
		//unl_cpu();

		DbgMsg_UVCIO(("-AddTxfQue[%d],curCnt=%d,f=%d,%d\r\n", queIdx, gU2UvacTxfQueCurCnt[queIdx], gU2UvacTxfQueFrontIdx[queIdx], gU2UvacTxfQueRearIdx[queIdx]));
		#if 0
		DbgCode_UVC(if ((idx == gU2UvacTxfQueFrontIdx[queIdx]) || (gU2UvacTxfQueCurCnt[queIdx] >= (UVAC_TXF_MAX_QUECNT - 1))) \
					DBG_DUMP("TxfQue[%d]:f=%d,r=%d,cnt=%d almost full\r\n", queIdx, gU2UvacTxfQueFrontIdx[queIdx], gU2UvacTxfQueRearIdx[queIdx], gU2UvacTxfQueCurCnt[queIdx]);)
		#endif

	} else {
		//DbgMsg_UVC(("^RNo txfInfo[%d]=%d\r\n", queIdx, gU2UvacTxfQueCurCnt[queIdx]));
	}
	return pTxfInfo;
}
ER U2UVAC_AddIsoInTxfInfo(UVAC_TXF_INFO *pTxfInfo, UVAC_TXF_QUE queIdx)
{
	UVAC_TXF_INFO *pDstTxfInfo;
	DbgMsg_UVCIO(("+%s:0x%x,queIdx=%d\r\n", __func__, pTxfInfo, queIdx));

	if (0 == pTxfInfo) {
		DBG_ERR("In txfInfo NULL\r\n");
		return E_PAR;
	}

	loc_cpu();
	pDstTxfInfo = UVAC_GetFreeTxfInfo(queIdx);
	if (0 == pDstTxfInfo) {
		unl_cpu();
		//DBG_IND("Add:No free txfInfo:%d\r\n", queIdx);
		return E_PAR;
	}
	memcpy((void *)pDstTxfInfo, (void *)pTxfInfo, sizeof(UVAC_TXF_INFO));
	unl_cpu();
	DbgMsg_UVCIO(("AddTxf:adr=0x%x,0x%x,ep=%d,que=%d\r\n", pTxfInfo->sAddr, pTxfInfo->size, pTxfInfo->usbEP, queIdx));

	if (UVAC_TXF_QUE_V1 == queIdx) {
		set_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_VDO_TXF);
	} else if (UVAC_TXF_QUE_V2 == queIdx) {
		set_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_VDO2_TXF);
	} else if (UVAC_TXF_QUE_A1 == queIdx) {
		set_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_AUD_TXF);
	} else if (UVAC_TXF_QUE_A2 == queIdx) {
		set_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_AUD2_TXF);
	}
	return E_OK;
}
#if 0
THREAD_DECLARE(U2UVAC_IsoInTsk, arglist)
{
	FLGPTN uiFlag;
	UINT32 DMALen = 0, OriDMALen = 0;
	UVAC_TXF_INFO *pTxfInfo = 0;
	UINT32 queIdx = UVAC_TXF_QUE_A1;
	UINT32 i = 0, itemIdx = 0;
	ER retV = E_OK;
	UINT32 queTxfCnt = 0;
	UINT32 isEPStart = 0;
	UINT32 isVidEPStart = 0;

	vos_task_enter();
	DbgMsg_UVC(("+%s:QueMax=%d,Abort=%d,HWPL=%d,%d\r\n", __func__, UVAC_TXF_QUE_MAX, gU2UvcUsbDMAAbord, gU2UvcHWPayload[UVAC_VID_DEV_CNT_1], gU2UvcHWPayload[UVAC_VID_DEV_CNT_2]));
	U2UVAC_DbgDmp_TxfEPInfo();
	clr_flg(FLG_ID_U2UVAC, (FLGUVAC_ISOIN_START | FLGUVAC_ISOIN_STOP | FLGUVAC_ISOIN_TXF));
	while (1) {
		DbgMsg_UVCIO(("^R+Iso\r\n"));
		set_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_READY);
		wai_flg(&uiFlag, FLG_ID_U2UVAC, (FLGUVAC_ISOIN_START | FLGUVAC_ISOIN_STOP | FLGUVAC_ISOIN_TXF), (TWF_CLR | TWF_ORW));
		clr_flg(FLG_ID_U2UVAC, FLGUVAC_ISOIN_READY);
		DbgMsg_UVCIO(("^R-Iso:0x%x\r\n", uiFlag));

		if (uiFlag & FLGUVAC_ISOIN_STOP) {
			DbgMsg_UVCIO(("^GIsoIn Stop,Prv=%d\r\n", gU2UvacIsoTxfState));
			gU2UvacIsoTxfState = UVC_ISOIN_TXF_STATE_STOP;
			continue;
		} else {
			DbgMsg_UVCIO(("IsoIn Txf,PrvSts=%d,abort=%d,NoVid=%d\r\n", gU2UvacIsoTxfState, gU2UvcUsbDMAAbord, gU2UvcNoVidStrm));
			gU2UvacIsoTxfState = UVC_ISOIN_TXF_STATE_TXFING;
			queTxfCnt = 1;
			while ((gU2UvcUsbDMAAbord == FALSE) && (gU2UvcNoVidStrm == FALSE) && queTxfCnt) {
				queTxfCnt = 0;
				queIdx = UVAC_TXF_QUE_A1 % UVAC_TXF_QUE_MAX;
				for (i = 0; i < UVAC_TXF_QUE_MAX; i++) {
					//queTxfCnt += gUvacTxfQueCurCnt[queIdx];//cause CPU too busy
					if (UVAC_TXF_QUE_V1 == queIdx) {
						isEPStart = isVidEPStart = gU2UvcVidStart[0];
					} else if (UVAC_TXF_QUE_V2 == queIdx) {
						isEPStart = isVidEPStart = gU2UvcVidStart[1];
					} else if (UVAC_TXF_QUE_A1 == queIdx) {
						isEPStart = gU2UacAudStart[0];
						isVidEPStart = gU2UvcVidStart[0];
					} else if (UVAC_TXF_QUE_A2 == queIdx) {
						isEPStart = gU2UacAudStart[1];
						isVidEPStart = gU2UvcVidStart[1];
					} else {
						isEPStart = FALSE;
					}
					if (gU2UvacTxfQueCurCnt[queIdx] && isEPStart && isVidEPStart) {
						queTxfCnt += gU2UvacTxfQueCurCnt[queIdx];
						if (gU2UvacTxfQueCurCnt[queIdx] > 2 || m_uiU2UvacDbgIso & UVAC_DBG_ISO_QUE_CNT) {
							DBG_IND("IsoQ[%d]=%d\r\n", queIdx, gU2UvacTxfQueCurCnt[queIdx]);
						}
						if (FALSE == usb_chkEPBusy(U2UVAC_USB_EP[queIdx])) {
							DbgCode_UVC(if (UVAC_TXF_QUE_IMG == queIdx) DBG_DUMP("EP=%d Ept\r\n", U2UVAC_USB_EP[queIdx]);) {
								itemIdx = gU2UvacTxfQueFrontIdx[queIdx];
							}
							pTxfInfo = &gU2UvacTxfQue[queIdx][itemIdx];
							DbgMsg_UVCIO(("^M+Txf:0x%x,0x%x,ep=%d\r\n", pTxfInfo->sAddr, pTxfInfo->size, pTxfInfo->usbEP));
							if (((UVAC_TXF_QUE_V1 == queIdx) && gU2UvcHWPayload[UVAC_VID_DEV_CNT_1]) || ((UVAC_TXF_QUE_V2 == queIdx) && gU2UvcHWPayload[UVAC_VID_DEV_CNT_2])) {
								DbgCode_UVC(if (UVAC_TXF_STREAM_VID != pTxfInfo->streamType) DBG_DUMP("1TxfInfo of Que[%d]=%d not UVAC_TXF_STREAM_VID\r\n", queIdx, pTxfInfo->streamType);)
									if ((0 == pTxfInfo->txfCnt) && (UVAC_TXF_STREAM_VID == pTxfInfo->streamType)) {
										DbgMsg_UVCIO(("TxfQue[%d]:cnt=%d,addr=0x%x,size=0x%x Enable HW AutoGen-Header\r\n", queIdx, pTxfInfo->txfCnt, pTxfInfo->sAddr, pTxfInfo->size));
										usb_setEpConfig(U2UVAC_USB_EP[queIdx], USB_EPCFG_ID_AUTOHDR_START, ENABLE);
										DbgMsg_UVCIO(("Disable HW AutoGen-EOF of Que[%d]\r\n", queIdx));
										usb_setEpConfig(U2UVAC_USB_EP[queIdx], USB_EPCFG_ID_AUTOHDR_STOP, DISABLE);
									} else if (UVAC_TXF_STREAM_VID == pTxfInfo->streamType) {
										DbgMsg_UVCIO(("TxfQue[%d]:cnt=%d,addr=0x%x,size=0x%x Disble HW AutoGen-Header\r\n", queIdx, pTxfInfo->txfCnt, pTxfInfo->sAddr, pTxfInfo->size));
										usb_setEpConfig(U2UVAC_USB_EP[queIdx], USB_EPCFG_ID_AUTOHDR_START, DISABLE);
									}
							}
							if (UVAC_TXF_STREAM_VID == pTxfInfo->streamType) {
								UVAC_VID_DEV_CNT  thisVidIdx = 0;
								if (UVAC_TXF_QUE_V1 == queIdx) {
									thisVidIdx = UVAC_VID_DEV_CNT_1;
								} else if (UVAC_TXF_QUE_V2 == queIdx) {
									thisVidIdx = UVAC_VID_DEV_CNT_2;
								} else {
									DBG_ERR("Wrong streamType!\r\n");
									continue;
								}
#if 0

								{
									UINT32 va, len;
									CHAR         tempfilename[30];
									FILE     *fp = NULL;
									static UINT32 frame = 0;
									frame++;

									if (frame % 60 == 0) {
										snprintf(tempfilename, sizeof(tempfilename), "/mnt/sd/UVC_%d.JPG", (unsigned int)frame);
										DBG_DUMP("%s\r\n",tempfilename);
										if ((fp = fopen(tempfilename, "wb")) == NULL) {
											DBG_ERR("open write file fail\r\n");
										}
										len = pTxfInfo->size;
										//va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pTxfInfo->sAddr, len);
										va = pTxfInfo->sAddr;
										if (fp) {
											fwrite((UINT8*)va, 1, len, fp);
											fflush(fp);
											fclose(fp);
										}
										//hd_common_mem_munmap((void *)va, len);
									}

								}
#endif
								U2UVC_MakePayloadFmt(thisVidIdx, pTxfInfo);
								if (0 == pTxfInfo->size) {
									if (UVAC_TXF_STREAM_STILLIMG == pTxfInfo->streamType) {
										DbgMsg_UVCIO(("SillImg txf done,sts=%d => %d\r\n", gU2UvacStillImgTrigSts, UVC_STILLIMG_TRIG_CTRL_NORMAL));
										gU2UvacStillImgTrigSts = UVC_STILLIMG_TRIG_CTRL_NORMAL;
									}

									UVAC_UpdateTxfQue(pTxfInfo);
									continue;
								}
							}
							//skip size = 0
							if (pTxfInfo->size) {
								if (pTxfInfo->size > gU2UvcMaxTxfSizeForUsb[queIdx]) {
									DbgMsg_UVCIO(("^MOri[%d]=0x%x,Txf=0x%x\r\n", queIdx, pTxfInfo->size, gU2UvcMaxTxfSizeForUsb[queIdx]));
									OriDMALen = DMALen = gU2UvcMaxTxfSizeForUsb[queIdx];
									DbgCode_UVC(if (0 != ((UINT32)pTxfInfo->sAddr % 4))  DBG_DUMP("Not 4-align,USB,Addr=0x%x,Len=0x%x\r\n", (UINT32)pTxfInfo->sAddr, DMALen);) {
										DbgMsg_UVCIO(("^R+WA[%d]=0x%x,0x%x\r\n", pTxfInfo->usbEP, DMALen, pTxfInfo->size));
									}
									retV = usb_setEPWrite(pTxfInfo->usbEP, (UINT8 *)(pTxfInfo->sAddr), &DMALen);
									if (m_uiU2UvacDbgIso & UVAC_DBG_ISO_VID) {
										DBG_DUMP("V[%d][%d]->EP%d\r\n", queIdx, itemIdx, pTxfInfo->usbEP);
									}
									if (retV != E_OK) {
										DBG_ERR("Large fail=%d,EP=%d,Len=%d,addr=0x%x\r\n", retV, pTxfInfo->usbEP, DMALen, pTxfInfo->sAddr);
										DMALen = 0;
										break;
									} else {
										//temporarely assume this DMA will be fully transferred
										DMALen = OriDMALen;
									}
								} else {
									if (UVAC_TXF_STREAM_VID == pTxfInfo->streamType) {
										if (((UVAC_TXF_QUE_V1 == queIdx) && gU2UvcHWPayload[UVAC_VID_DEV_CNT_1]) || ((UVAC_TXF_QUE_V2 == queIdx) && gU2UvcHWPayload[UVAC_VID_DEV_CNT_2])) {
											DbgCode_UVC(if (UVAC_TXF_STREAM_VID != pTxfInfo->streamType) DBG_DUMP("2TxfInfo of Que[%d]=%d not UVAC_TXF_STREAM_VID\r\n", queIdx, pTxfInfo->streamType);) {
												DbgMsg_UVCIO(("^BHW-EOF[%d]:%d,%d\r\n", queIdx, gU2UvcHWPayload[UVAC_VID_DEV_CNT_1], gU2UvcHWPayload[UVAC_VID_DEV_CNT_2]));
											}
											usb_setEpConfig(U2UVAC_USB_EP[queIdx], USB_EPCFG_ID_AUTOHDR_STOP, ENABLE);
										}
										if (m_uiU2UvacDbgIso & UVAC_DBG_ISO_VID) {
											DBG_DUMP("V[%d][%d]->EP%d %dK\r\n", queIdx, itemIdx, pTxfInfo->usbEP, pTxfInfo->size / 1024);
										}
									} else {
										if (m_uiU2UvacDbgIso & UVAC_DBG_ISO_AUD) {
											DBG_DUMP("A[%d][%d]->EP%d\r\n", queIdx, itemIdx, pTxfInfo->usbEP);
										}
									}
									OriDMALen = DMALen = pTxfInfo->size;
									DbgMsg_UVCIO(("^R+WA[%d]=0x%x,0x%x\r\n", pTxfInfo->usbEP, DMALen, pTxfInfo->size));
									retV = usb_setEPWrite(pTxfInfo->usbEP, (UINT8 *)(pTxfInfo->sAddr), &DMALen);
									//DbgMsg_UVC(("^R-WA[%d]=0x%x,0x%x\r\n",pTxfInfo->usbEP, DMALen, pTxfInfo->size));
									if (retV != E_OK) {
										DBG_ERR(":fail=%d,EP=%d,Len=%d,addr=0x%x\r\n", retV, pTxfInfo->usbEP, DMALen, pTxfInfo->sAddr);
										DMALen = 0;
										break;
									} else {
										//temporarely assume this DMA will be fully transferred
										DMALen = OriDMALen;
									}
								}
								pTxfInfo->sAddr += DMALen;
								pTxfInfo->size -= DMALen;
								pTxfInfo->txfCnt ++;
							}
							if (0 == pTxfInfo->size) {
#if 1//(ENABLE == UVC_METHOD3)
								if (UVAC_TXF_STREAM_STILLIMG == pTxfInfo->streamType) {
									DbgMsg_UVCIO(("SillImg txf done,sts=%d => %d\r\n", gU2UvacStillImgTrigSts, UVC_STILLIMG_TRIG_CTRL_NORMAL));
									gU2UvacStillImgTrigSts = UVC_STILLIMG_TRIG_CTRL_NORMAL;
								}
#endif

								UVAC_UpdateTxfQue(pTxfInfo);

							}
							DbgMsg_UVCIO(("^M-Txf:adr=0x%x,0x%x,ep=%d,txf=0x%x,txfcnt=%d,queC=%d,%d\r\n", pTxfInfo->sAddr, pTxfInfo->size, pTxfInfo->usbEP, DMALen, pTxfInfo->txfCnt, gU2UvacTxfQueCurCnt[UVAC_TXF_QUE_V1], gU2UvacTxfQueCurCnt[UVAC_TXF_QUE_A1]));
						} else {
							U2UVAC_DbgDmp_ChkTxfQueAlmostFull(queIdx, UVAC_TXF_MAX_QUECNT * 2 / 3, 1, 0);
							if (m_uiU2UvacDbgIso & UVAC_DBG_ISO_EP_BUSY) {
								DBG_DUMP("EP%d busy\r\n", U2UVAC_USB_EP[queIdx]);
							}
							vos_util_delay_us(50);

						}
					} else {
						U2UVAC_DbgDmp_ChkIsoInQueEmpty(queIdx);
					}
					queIdx ++;
					queIdx %= UVAC_TXF_QUE_MAX;
				}
				if ((gU2UvcVidStart[0] == FALSE) && (gU2UvcVidStart[1] == FALSE) && \
					(gU2UacAudStart[0] == FALSE) && (gU2UacAudStart[1] == FALSE) && \
					(UVC_STILLIMG_TRIG_CTRL_NORMAL == gU2UvacStillImgTrigSts)) {
					DBG_DUMP("Stop Txf:%d,%d,%d,%d\r\n", gU2UvacTxfQueCurCnt[0], gU2UvacTxfQueCurCnt[1], gU2UvacTxfQueCurCnt[2], gU2UvacTxfQueCurCnt[3]);
					gU2UvcUsbDMAAbord = TRUE;
				}
			}

		}
	}
	DbgMsg_UVCIO(("-%s:0x%x\r\n", __func__, uiFlag));
	THREAD_RETURN(0);
}
#endif


#if (ISF_LATENCY_DEBUG)
static void isf_timestamp_dump(UVAC_TIMESTAMP_INFO *p_in_isf)
{
	UVAC_TIMESTAMP_INFO     *p_isf;
	UINT32       i;
	if (g_isf_uvac_timestamp_idx < ISF_UVAC_MAX_ISF_DEBUG_NUM) {
		p_isf = &g_isf_uvac_timestamp[g_isf_uvac_timestamp_idx++];
		memcpy(p_isf, p_in_isf, sizeof(UVAC_TIMESTAMP_INFO));
	} else if (is_isf_dump == FALSE) {
		is_isf_dump = TRUE;
		for (i=0; i< ISF_UVAC_MAX_ISF_DEBUG_NUM; i++) {
			p_isf = &g_isf_uvac_timestamp[i];
			DBG_DUMP("timediff %6lld \r\n", (p_isf->uvc_timestamp_end - p_isf->timestamp));
		}
	}
}

void U2UVAC_DbgDmp_TimestampNumReset(void)
{
	g_isf_uvac_timestamp_idx = 0;
	is_isf_dump = FALSE;
}
#endif

#if (ISF_AUDIO_LATENCY_DEBUG)
static void isf_aud_timestamp_dump(UVAC_AUD_TIMESTAMP_INFO *p_in_isf)
{
	UVAC_AUD_TIMESTAMP_INFO     *p_isf;
	UINT32 i;
	if (g_isf_uvac_aud_timestamp_idx < ISF_UVAC_AUD_MAX_ISF_DEBUG_NUM) {
		p_isf = &g_isf_uvac_aud_timestamp[g_isf_uvac_aud_timestamp_idx++];
		memcpy(p_isf, p_in_isf, sizeof(UVAC_AUD_TIMESTAMP_INFO));
	} else if (is_aud_isf_dump == FALSE) {
		is_aud_isf_dump = TRUE;
		DBG_DUMP("                 start-ts end-start   end-ts \r\n");
		for (i=0; i< ISF_UVAC_AUD_MAX_ISF_DEBUG_NUM; i++) {
			p_isf = &g_isf_uvac_aud_timestamp[i];
			DBG_DUMP("timediff buf[%02d] %3d %8lld  %8lld %8lld \r\n", i+1, p_isf->uvc_slice_size, (p_isf->uvc_timestamp_start - p_isf->timestamp), (p_isf->uvc_timestamp_end - p_isf->uvc_timestamp_start), (p_isf->uvc_timestamp_end - p_isf->timestamp));
		}
	}
}

void U2UVAC_DbgDmp_Aud_TimestampNumReset(void)
{
	g_isf_uvac_aud_timestamp_idx = 0;
	is_aud_isf_dump = FALSE;
}
#endif

static void video_tx(UINT32 queIdx,	UVAC_VID_DEV_CNT thisVidIdx, FLGPTN wait_flg, FLGPTN idle_flg)
{
	FLGPTN uiFlag;
	UINT32 DMALen = 0, OriDMALen = 0;
	UVAC_TXF_INFO *pTxfInfo = 0;
	UINT32 itemIdx = 0;
	ER retV = E_OK;
	VOS_TICK tt1 = 0, tt2 = 0;

	vos_task_enter();
	DbgMsg_UVC(("+%s:queIdx=%d,Abort=%d\r\n", __func__, queIdx, gU2UvcUsbDMAAbord));
	U2UVAC_DbgDmp_TxfEPInfo();
	clr_flg(FLG_ID_U2UVAC, wait_flg);
	while (1) {
		DbgMsg_UVCIO(("^R+IsoVdo\r\n"));
		set_flg(FLG_ID_U2UVAC, idle_flg);
		wai_flg(&uiFlag, FLG_ID_U2UVAC, wait_flg, (TWF_CLR | TWF_ORW));
		clr_flg(FLG_ID_U2UVAC, idle_flg);
		DbgMsg_UVCIO(("^R-IsoVdo:0x%x\r\n", uiFlag));


		while ((gU2UvcUsbDMAAbord == FALSE) && (gU2UvcNoVidStrm == FALSE) && gU2UvacTxfQueCurCnt[queIdx] && gU2UvcVidStart[queIdx]) {
			if (gUvacRunningFlag[thisVidIdx] & FLGUVAC_STOP) {
				DbgMsg_UVC(("Stop ISO Video[%d] ...\r\n", thisVidIdx));
				if (thisVidIdx == UVAC_VID_DEV_CNT_1) {
					vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V1);
				} else {
					vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V2);
				}
				break;
			}
			if (UVAC_IsoTxDbg(queIdx, UVAC_DBG_ISO_QUE_CNT)) {
				DBG_DUMP("VIsoQ[%d]=%d\r\n", queIdx, gU2UvacTxfQueCurCnt[queIdx]);
			}
			itemIdx = gU2UvacTxfQueFrontIdx[queIdx];
			pTxfInfo = &gU2UvacTxfQue[queIdx][itemIdx];
			DbgMsg_UVCIO(("^M+Txf:0x%x,0x%x,ep=%d\r\n", pTxfInfo->sAddr, pTxfInfo->size, pTxfInfo->usbEP));
			if (UVAC_IsoTxDbg(queIdx, UVAC_DBG_ISO_TX)) {
				vos_perf_mark(&tt1);
			}
			if (0 == pTxfInfo->size || 0 == pTxfInfo->sAddr || pTxfInfo->usbEP != U2UVAC_USB_EP[queIdx]) {
				DBG_WRN("zero stream 0:0x%x,0x%x,ep=%d\r\n", pTxfInfo->sAddr, pTxfInfo->size, pTxfInfo->usbEP);
				U2UVAC_DbgDmp_ChkTxfQueAlmostFull(queIdx, 0, 1, 1);
				UVAC_UpdateTxfQue(pTxfInfo);
				continue;
			}
			U2UVC_MakePayloadFmt(thisVidIdx, pTxfInfo);

			if (thisVidIdx == UVAC_VID_DEV_CNT_1) {
				vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V1);
			} else {
				vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V2);
			}

			if (UVAC_IsoTxDbg(queIdx, UVAC_DBG_ISO_TX)) {
				vos_perf_mark(&tt2);
				DBG_DUMP("PL[%d]us\r\n", vos_perf_duration(tt1, tt2));
			}
			if (0 == pTxfInfo->size || 0 == pTxfInfo->sAddr || pTxfInfo->usbEP != U2UVAC_USB_EP[queIdx] || pTxfInfo->sAddr > 0x5fffffff) {
				DBG_WRN("zero stream:0x%x,0x%x,ep=%d,timestamp=%llu\r\n", pTxfInfo->sAddr, pTxfInfo->size, pTxfInfo->usbEP, pTxfInfo->timestamp);
				DBG_DUMP("gU2UvacTxfQueCurCnt[0]=%d, gU2UvacTxfQueRearIdx[0]=%d, gU2UvacTxfQueFrontIdx[0]=%d\r\n", gU2UvacTxfQueCurCnt[0],gU2UvacTxfQueRearIdx[0], gU2UvacTxfQueFrontIdx[0] );
				U2UVAC_DbgDmp_ChkTxfQueAlmostFull(queIdx, 0, 1, 1);
				UVAC_UpdateTxfQue(pTxfInfo);
				#if 0
				if (thisVidIdx == UVAC_VID_DEV_CNT_1) {
					vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V1);
				} else {
					vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V2);
				}
				#endif
				continue;
			}
			if (pTxfInfo->size > gU2UvcMaxTxfSizeForUsb[queIdx]) {
				DbgMsg_UVCIO(("^MOri[%d]=0x%x,Txf=0x%x\r\n", queIdx, pTxfInfo->size, gU2UvcMaxTxfSizeForUsb[queIdx]));
				OriDMALen = DMALen = gU2UvcMaxTxfSizeForUsb[queIdx];
				DbgCode_UVC(if (0 != ((UINT32)pTxfInfo->sAddr % 4))  DBG_DUMP("Not 4-align,USB,Addr=0x%x,Len=0x%x\r\n", (UINT32)pTxfInfo->sAddr, DMALen);) {
					DbgMsg_UVCIO(("^R+WA[%d]=0x%x,0x%x\r\n", pTxfInfo->usbEP, DMALen, pTxfInfo->size));
				}
				retV = usb_writeEndpoint(pTxfInfo->usbEP, (UINT8 *)(pTxfInfo->sAddr), &DMALen);
				if (UVAC_IsoTxDbg(queIdx, UVAC_DBG_ISO_TX)) {
					DBG_DUMP("V[%d][%d]->EP%d\r\n", queIdx, itemIdx, pTxfInfo->usbEP);
				}
				if (retV != E_OK) {
					DBG_ERR("Large fail=%d,EP=%d,Len=%d,addr=0x%x\r\n", retV, pTxfInfo->usbEP, DMALen, pTxfInfo->sAddr);
					DMALen = 0;
					#if 0
					if (thisVidIdx == UVAC_VID_DEV_CNT_1) {
						vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V1);
					} else {
						vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V2);
					}
					#endif
					break;
				} else {
					//temporarely assume this DMA will be fully transferred
					DMALen = OriDMALen;
				}
			} else {
				if (UVAC_IsoTxDbg(queIdx, UVAC_DBG_ISO_TX)) {
					DBG_DUMP("V[%d][%d]->EP%d %dK\r\n", queIdx, itemIdx, pTxfInfo->usbEP, pTxfInfo->size / 1024);
					vos_perf_mark(&tt1);
				}
				OriDMALen = DMALen = pTxfInfo->size;
				DbgMsg_UVCIO(("^R+WA[%d]=0x%x,0x%x\r\n", pTxfInfo->usbEP, DMALen, pTxfInfo->size));
				retV = usb_writeEndpoint(pTxfInfo->usbEP, (UINT8 *)(pTxfInfo->sAddr), &DMALen);
				if (UVAC_IsoTxDbg(queIdx, UVAC_DBG_ISO_TX)) {
					UINT32 diff_ms;
					VOS_TICK tt3 = 0;

					vos_perf_mark(&tt2);
					while(usb_chkEPBusy(pTxfInfo->usbEP));
					vos_perf_mark(&tt3);
					diff_ms = vos_perf_duration(tt1, tt3)/ 1000;
					DBG_DUMP("%dms(%dKB/sec), busy %dus\r\n", diff_ms, pTxfInfo->size*1000/diff_ms/1024, vos_perf_duration(tt2, tt3));
				}
				//usb_setEPWrite(pTxfInfo->usbEP, (UINT8 *)(pTxfInfo->sAddr), &DMALen);
				//retV = usb_waitEPDone(pTxfInfo->usbEP, &DMALen);
				//DbgMsg_UVC(("^R-WA[%d]=0x%x,0x%x\r\n",pTxfInfo->usbEP, DMALen, pTxfInfo->size));
				if (retV != E_OK) {
					DBG_ERR(":fail=%d,EP=%d,Len=%d,addr=0x%x\r\n", retV, pTxfInfo->usbEP, DMALen, pTxfInfo->sAddr);
					DMALen = 0;
					#if 0
					if (thisVidIdx == UVAC_VID_DEV_CNT_1) {
						vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V1);
					} else {
						vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V2);
					}
					#endif
					break;
				} else {
					//temporarely assume this DMA will be fully transferred
					DMALen = OriDMALen;
				}
				#if 0
				if (thisVidIdx == UVAC_VID_DEV_CNT_1) {
					vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V1);
				} else {
					vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_V2);
				}
				#endif
			}
			pTxfInfo->sAddr += DMALen;
			pTxfInfo->size -= DMALen;
			pTxfInfo->txfCnt ++;
			if (0 == pTxfInfo->size) {
				//if (gfpUvacReleaseCB) {
				//	gfpUvacReleaseCB(thisVidIdx, pTxfInfo->oriAddr);
				//}
				UVAC_UpdateTxfQue(pTxfInfo);
				#if (ISF_LATENCY_DEBUG)
				{
					UVAC_TIMESTAMP_INFO  time_info;

					time_info.timestamp = pTxfInfo->timestamp;
					time_info.uvc_timestamp_end = hd_gettime_us();
					isf_timestamp_dump(&time_info);
					//printf("diff = %lld\r\n", cur_time - pTxfInfo->timestamp);
				}
				#endif
			} else {
				DbgMsg_UVC(("V[%d]abort DMA, size remain %d\r\n", queIdx, pTxfInfo->size));
			}
			DbgMsg_UVCIO(("^M-Txf:adr=0x%x,0x%x,ep=%d,txf=0x%x,txfcnt=%d,queC=%d,%d\r\n", pTxfInfo->sAddr, pTxfInfo->size, pTxfInfo->usbEP, DMALen, pTxfInfo->txfCnt, gU2UvacTxfQueCurCnt[UVAC_TXF_QUE_V1], gU2UvacTxfQueCurCnt[UVAC_TXF_QUE_A1]));
		}

	}
}
THREAD_DECLARE(U2UVAC_IsoInVdo1Tsk, arglist)
{
	video_tx(UVAC_TXF_QUE_V1, UVAC_VID_DEV_CNT_1, FLGUVAC_ISOIN_VDO_TXF, FLGUVAC_ISOIN_VDO_READY);
	THREAD_RETURN(0);
}

THREAD_DECLARE(U2UVAC_IsoInVdo2Tsk, arglist)
{
	video_tx(UVAC_TXF_QUE_V2, UVAC_VID_DEV_CNT_2, FLGUVAC_ISOIN_VDO2_TXF, FLGUVAC_ISOIN_VDO2_READY);
	THREAD_RETURN(0);
}

static void audio_tx(UINT32 queIdx,	UVAC_AUD_DEV_CNT thisAudIdx, FLGPTN wait_flg, FLGPTN idle_flg)
{
	FLGPTN uiFlag;
	UINT32 DMALen = 0, OriDMALen = 0;
	UVAC_TXF_INFO *pTxfInfo = 0;
	UINT32 itemIdx = 0;
	ER retV = E_OK;

	UINT32 UacPacketSize = 0;
	FLGPTN waiptn = 0;
	TIMER_ID timer_id = TIMER_NUM;
	char  *chip_name = getenv("NVT_CHIP_ID");
	BOOL timer_mode = FALSE;

	#if (ISF_AUDIO_LATENCY_DEBUG)
	UVAC_AUD_TIMESTAMP_INFO  time_info;
	#endif

	if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51084") == 0) {
		timer_mode = TRUE;
	}

	if (timer_mode) {
		if (thisAudIdx == UVAC_AUD_DEV_CNT_1) {
			waiptn = FLGUVAC_AUD1_CHK_BUSY;
			timer_id = gUacTimerID[UVAC_AUD_DEV_CNT_1];
		} else {
			waiptn = FLGUVAC_AUD2_CHK_BUSY;
			timer_id = gUacTimerID[UVAC_AUD_DEV_CNT_2];
		}

		UacPacketSize = gU2UvacAudSampleRate[0] * gU2UacChNum * UAC_BIT_PER_SECOND / 8 / 1000; //Audio bit rate(byte per ms).
		UacPacketSize += 4;	//fix for compatibility with MacOS
		if (UacPacketSize < 36) {
			UacPacketSize = 36;
		}
		if (gUacMaxPacketSize){
			UacPacketSize = gUacMaxPacketSize;
		}
	}

	vos_task_enter();
	DbgMsg_UVC(("+%s:QueMax=%d,Abort=%d,HWPL=%d,%d\r\n", __func__, UVAC_TXF_QUE_MAX, gU2UvcUsbDMAAbord, gU2UvcHWPayload[UVAC_VID_DEV_CNT_1], gU2UvcHWPayload[UVAC_VID_DEV_CNT_2]));
	clr_flg(FLG_ID_U2UVAC, wait_flg);
	while (1) {
		DbgMsg_UVCIO(("^R+IsoAud\r\n"));
		set_flg(FLG_ID_U2UVAC, idle_flg);
		wai_flg(&uiFlag, FLG_ID_U2UVAC, wait_flg, (TWF_CLR | TWF_ORW));
		clr_flg(FLG_ID_U2UVAC, idle_flg);
		DbgMsg_UVCIO(("^R-IsoAud:0x%x\r\n", uiFlag));

		while ((gU2UacUsbDMAAbord == FALSE) && gU2UvacTxfQueCurCnt[queIdx] && gU2UacAudStart[thisAudIdx]) {
			if (UVAC_IsoTxDbg(queIdx, UVAC_DBG_ISO_QUE_CNT)) {
				DBG_DUMP("AIsoQ[%d]=%d\r\n", queIdx, gU2UvacTxfQueCurCnt[queIdx]);
			}
			itemIdx = gU2UvacTxfQueFrontIdx[queIdx];
			pTxfInfo = &gU2UvacTxfQue[queIdx][itemIdx];
			DbgMsg_UVCIO(("^M+Txf:0x%x,0x%x,ep=%d\r\n", pTxfInfo->sAddr, pTxfInfo->size, pTxfInfo->usbEP));
			if (0 == pTxfInfo->size || 0 == pTxfInfo->sAddr || pTxfInfo->usbEP != U2UVAC_USB_EP[queIdx]) {

				DBG_WRN("zero stream:0x%x,0x%x,ep=%d\r\n", pTxfInfo->sAddr, pTxfInfo->size, pTxfInfo->usbEP);
				UVAC_UpdateTxfQue(pTxfInfo);
				if (thisAudIdx == UVAC_AUD_DEV_CNT_1) {
					vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A1);
				} else {
					vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A2);
				}
				continue;
			}
			if (pTxfInfo->size > gU2UvcMaxTxfSizeForUsb[queIdx]) {
				DbgMsg_UVCIO(("^MOri[%d]=0x%x,Txf=0x%x\r\n", queIdx, pTxfInfo->size, gU2UvcMaxTxfSizeForUsb[queIdx]));
				OriDMALen = DMALen = gU2UvcMaxTxfSizeForUsb[queIdx];
				DbgCode_UVC(if (0 != ((UINT32)pTxfInfo->sAddr % 4))  DBG_DUMP("Not 4-align,USB,Addr=0x%x,Len=0x%x\r\n", (UINT32)pTxfInfo->sAddr, DMALen);) {
					DbgMsg_UVCIO(("^R+WA[%d]=0x%x,0x%x\r\n", pTxfInfo->usbEP, DMALen, pTxfInfo->size));
				}
				retV = usb_writeEndpoint(pTxfInfo->usbEP, (UINT8 *)(pTxfInfo->sAddr), &DMALen);
				if (UVAC_IsoTxDbg(queIdx, UVAC_DBG_ISO_TX)) {
					DBG_DUMP("A[%d][%d]->EP%d\r\n", queIdx, itemIdx, pTxfInfo->usbEP);
				}
				if (retV != E_OK) {
					DBG_ERR("Large fail=%d,EP=%d,Len=%d,addr=0x%x\r\n", retV, pTxfInfo->usbEP, DMALen, pTxfInfo->sAddr);
					DMALen = 0;
					if (thisAudIdx == UVAC_AUD_DEV_CNT_1) {
						vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A1);
					} else {
						vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A2);
					}
					break;
				} else {
					//temporarely assume this DMA will be fully transferred
					DMALen = OriDMALen;
				}
			} else {
				if (timer_mode) {
					UINT32 send_size;
					UINT32 send_addr;

					if (UVAC_IsoTxDbg(queIdx, UVAC_DBG_ISO_TX)) {
						DBG_DUMP("A[%d][%d]->EP%d %dK\r\n", queIdx, itemIdx, pTxfInfo->usbEP, pTxfInfo->size / 1024);
					}
					OriDMALen = DMALen = pTxfInfo->size;
					DbgMsg_UVCIO(("^R+WA[%d]=0x%x,0x%x\r\n", pTxfInfo->usbEP, DMALen, pTxfInfo->size));

					//usb_setEPWrite(pTxfInfo->usbEP, (UINT8 *)(pTxfInfo->sAddr), &DMALen);
					//retV = usb_waitEPDone(pTxfInfo->usbEP, &DMALen);
					//DbgMsg_UVC(("^R-WA[%d]=0x%x,0x%x\r\n",pTxfInfo->usbEP, DMALen, pTxfInfo->size));

					send_addr = pTxfInfo->sAddr;

					while (DMALen > 0 && (gU2UacUsbDMAAbord == FALSE) && gU2UacAudStart[thisAudIdx]) {

						send_size = (DMALen > UacPacketSize)? UacPacketSize : DMALen;

						while ((gU2UacUsbDMAAbord == FALSE) && gU2UacAudStart[thisAudIdx]) {
							if (!usb_chkEPBusy(pTxfInfo->usbEP)) {
								break;
							}
							timer_cfg(timer_id, 500, TIMER_MODE_ONE_SHOT | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);

							wai_flg(&uiFlag, FLG_ID_U2UVAC_UAC, waiptn, (TWF_CLR | TWF_ORW));
						}

						if ((gU2UacUsbDMAAbord == TRUE) || !gU2UacAudStart[thisAudIdx]) {
							break;
						}

						retV = usb2dev_write_endpoint(pTxfInfo->usbEP, (UINT8 *)(send_addr), &send_size);

						if (retV != E_OK) {
							DBG_ERR(":fail=%d,EP=%d,Len=%d,addr=0x%x\r\n", retV, pTxfInfo->usbEP, send_size, pTxfInfo->sAddr);
							DMALen = 0;
							break;
						} else {
							//temporarely assume this DMA will be fully transferred
							//DMALen = OriDMALen;
							DMALen -= send_size;
							send_addr += send_size;
						}
					}

					if (thisAudIdx == UVAC_AUD_DEV_CNT_1) {
						vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A1);
					} else {
						vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A2);
					}

					DMALen = OriDMALen;
				} else {

					#if (ISF_AUDIO_LATENCY_DEBUG)
					time_info.uvc_timestamp_start= hd_gettime_us();
					time_info.uvc_slice_size = pTxfInfo->size;
					#endif

					if (UVAC_IsoTxDbg(queIdx, UVAC_DBG_ISO_TX)) {
						DBG_DUMP("A[%d][%d]->EP%d %dK\r\n", queIdx, itemIdx, pTxfInfo->usbEP, pTxfInfo->size / 1024);
					}
					OriDMALen = DMALen = pTxfInfo->size;
					DbgMsg_UVCIO(("^R+WA[%d]=0x%x,0x%x\r\n", pTxfInfo->usbEP, DMALen, pTxfInfo->size));
					retV = usb_writeEndpoint(pTxfInfo->usbEP, (UINT8 *)(pTxfInfo->sAddr), &DMALen);
					//usb_setEPWrite(pTxfInfo->usbEP, (UINT8 *)(pTxfInfo->sAddr), &DMALen);
					//retV = usb_waitEPDone(pTxfInfo->usbEP, &DMALen);
					//DbgMsg_UVC(("^R-WA[%d]=0x%x,0x%x\r\n",pTxfInfo->usbEP, DMALen, pTxfInfo->size));
					if (retV != E_OK) {
						DBG_ERR(":fail=%d,EP=%d,Len=%d,addr=0x%x\r\n", retV, pTxfInfo->usbEP, DMALen, pTxfInfo->sAddr);
						DMALen = 0;
						if (thisAudIdx == UVAC_AUD_DEV_CNT_1) {
							vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A1);
						} else {
							vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A2);
						}
						break;
					} else {
						//temporarely assume this DMA will be fully transferred
						DMALen = OriDMALen;
					}

					if (thisAudIdx == UVAC_AUD_DEV_CNT_1) {
						vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A1);
					} else {
						vos_flag_set(FLG_ID_U2UVAC_FRM, FLGUVAC_FRM_A2);
					}
				}
			}
			pTxfInfo->sAddr += DMALen;
			pTxfInfo->size -= DMALen;
			pTxfInfo->txfCnt ++;
			if (0 == pTxfInfo->size) {
				//if (gfpUvacReleaseCB) {
				//	gfpUvacReleaseCB(thisAudIdx, pTxfInfo->oriAddr);
				//}
				#if (ISF_AUDIO_LATENCY_DEBUG)
				{
					time_info.timestamp = pTxfInfo->timestamp;
					time_info.uvc_timestamp_end = hd_gettime_us();
					isf_aud_timestamp_dump(&time_info);
					//printf("diff = %lld\r\n", cur_time - pTxfInfo->timestamp);
				}
				#endif
				UVAC_UpdateTxfQue(pTxfInfo);
			} else {
				DbgMsg_UVC(("A[%d]abort DMA, size remain %d\r\n", queIdx, pTxfInfo->size));

			}
			DbgMsg_UVCIO(("^M-Txf:adr=0x%x,0x%x,ep=%d,txf=0x%x,txfcnt=%d,queC=%d,%d\r\n", pTxfInfo->sAddr, pTxfInfo->size, pTxfInfo->usbEP, DMALen, pTxfInfo->txfCnt, gU2UvacTxfQueCurCnt[UVAC_TXF_QUE_V1], gU2UvacTxfQueCurCnt[UVAC_TXF_QUE_A1]));
		}

	}
}
THREAD_DECLARE(U2UVAC_IsoInAud1Tsk, arglist)
{
	audio_tx(UVAC_TXF_QUE_A1, UVAC_AUD_DEV_CNT_1, FLGUVAC_ISOIN_AUD_TXF, FLGUVAC_ISOIN_AUD_READY);
	THREAD_RETURN(0);
}
THREAD_DECLARE(U2UVAC_IsoInAud2Tsk, arglist)
{
	audio_tx(UVAC_TXF_QUE_A2, UVAC_AUD_DEV_CNT_2, FLGUVAC_ISOIN_AUD2_TXF, FLGUVAC_ISOIN_AUD2_READY);
	THREAD_RETURN(0);
}

