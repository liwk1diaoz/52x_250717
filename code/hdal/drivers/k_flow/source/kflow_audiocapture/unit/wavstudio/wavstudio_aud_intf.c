#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kdrv_audioio/kdrv_audioio.h"
#include "wavstudio_aud_intf.h"
#include "wavstudio_id.h"
#include "ctl_aud_int.h"
#include "kdrv_builtin/kdrv_builtin.h"

#if defined __UITRON || defined __ECOS
extern DX_HANDLE gWavStudDxSndHdl;
#else
extern UINT32 gWavStudDxSndHdl;
#endif
extern WAVSTUD_OBJ gWavStudObj;

static UINT32 gWavStudPlayAudCh = KDRV_DEV_ID(0, KDRV_AUDOUT_ENGINE0, 0);
static UINT32 gWavStudPlay2AudCh = KDRV_DEV_ID(0, KDRV_AUDOUT_ENGINE1, 0);
static UINT32 gWavStudRecAudCh = KDRV_DEV_ID(0, KDRV_AUDCAP_ENGINE0, 0);
static UINT32 gWavStudLBAudCh = KDRV_DEV_ID(0, KDRV_AUDCAP_ENGINE1, 0);
//fix for CID 43703..43716 - begin
//static PAUDTS_OBJ gWavStudTsObj[WAVSTUD_ACT_MAX] = {0};
//fix for CID 43703..43716 - end

extern void WavStud_AudDrvExit(WAVSTUD_ACT actType);

#if defined (__KERNEL__)
extern BOOL wavstudio_first_boot;
#endif

//fix for CID 43703..43716 - begin
#if 0
static PAUDTS_OBJ WavStud_OpenAudObj(WAVSTUD_ACT actType)
{
	PAUDTS_OBJ pAudDrvObj = 0;
	UINT32 audDrvActCh = (WAVSTUD_ACT_PLAY == actType) ? gWavStudPlayAudCh : gWavStudRecAudCh;
	pAudDrvObj = aud_get_transceive_object(audDrvActCh);
	if (0 == pAudDrvObj) {
		DBG_ERR("AudDrvObj NULL:actType=%d\r\n", actType);
	}
	return pAudDrvObj;
}
static void WavStud_CloseAudObj(PAUDTS_OBJ pAudDrvObj)
{
	if (0 == pAudDrvObj) {
		DBG_ERR("AudDrvObj NULL");
		return; //fix for CID 44910
	}
	pAudDrvObj->close();
}
//fix for CID 43703..43716 - end
#endif


UINT32 WavStud_GetAudObj(WAVSTUD_ACT actType)
{
    UINT32 audDrvActCh = 0;

	if (WAVSTUD_ACT_PLAY == actType) {
		audDrvActCh = gWavStudPlayAudCh;
	} else if (WAVSTUD_ACT_PLAY2 == actType) {
		audDrvActCh = gWavStudPlay2AudCh;
	} else if (WAVSTUD_ACT_REC == actType || WAVSTUD_ACT_REC2 == actType) {
		audDrvActCh = gWavStudRecAudCh;
	} else {
		audDrvActCh = gWavStudLBAudCh;
	}

    return audDrvActCh;
}

#if 0
__inline static AUDIO_QUEUE_SEL WavStud_GetAudDrvQueSel(WAVSTUD_ACT actType, AUDIO_CH audCh)
{
	//DBG_IND("audCh=%d, actType=%d\r\n", audCh, actType);
	if (AUDIO_CH_DUAL_MONO == audCh) {
		if (WAVSTUD_ACT_REC == actType) {
			return AUDIO_QUEUE_SEL_DUALMONO_RECORDCH0;
		} else if (WAVSTUD_ACT_REC2 == actType) {
			return AUDIO_QUEUE_SEL_DUALMONO_RECORDCH1;
		}
	}
	return AUDIO_QUEUE_SEL_DEFAULT;
}
#endif
#if 1
__inline UINT32 WavStud_AudDrvGetTCValue(WAVSTUD_ACT actType)
{
	return 0;
}
#endif

static INT32 wavstud_reserve_buf(UINT32 phy_addr)
{
	return 0;
}

static INT32 wavstud_free_buf(UINT32 phy_addr)
{
	return 0;
}

void WavStud_AudDrvSetTCTrigger(WAVSTUD_ACT actType, UINT32 tcTrigVal)
{
}
ER WavStud_AudDrvPlay(WAVSTUD_ACT act_type)
{
	ER ret = E_OK;
	CTL_AUD_ID aud_id = 0;

	if (act_type == WAVSTUD_ACT_PLAY || act_type == WAVSTUD_ACT_PLAY2) {
		aud_id = CTL_AUD_ID_OUT;
	} else {
		return E_NOSPT;
	}

	ret = ctl_aud_module_start(aud_id);
	if (ret != 0) {
		DBG_ERR("ctl_aud start play fail = %d\r\n", ret);
	}

	return ret;
}
ER WavStud_AudDrvRec(WAVSTUD_ACT act_type)
{
	ER ret = E_OK;
	CTL_AUD_ID aud_id = 0;

	if (act_type == WAVSTUD_ACT_REC || act_type == WAVSTUD_ACT_REC2) {
		aud_id = CTL_AUD_ID_CAP;
	} else {
		return E_NOSPT;
	}

	ret = ctl_aud_module_start(aud_id);
	if (ret != 0) {
		DBG_ERR("ctl_aud start rec fail = %d\r\n", ret);
	}

	return ret;
}

void WavStud_AudDrvLBRec(void)
{
}

void WavStud_AudDrvStop(WAVSTUD_ACT actType)
{
	WavStud_AudDrvExit(actType);
#if (THIS_DBGLVL >= 7)
	{
		PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[actType]);
		DBG_DUMP(":currentCount=%u, %llu, %d, %d\r\n", pActObj->currentCount, pActObj->stopCount, pActObj->audInfo.aud_sr, pActObj->audInfo.aud_ch);
	}
#endif
}
void WavStud_AudDrvPause(WAVSTUD_ACT actType)
{
}
void WavStud_AudDrvResume(WAVSTUD_ACT actType)
{
}
BOOL WavStud_AudDrvIsBuffQueFull(WAVSTUD_ACT actType, AUDIO_CH audCh)
{
	return FALSE;
}
BOOL WavStud_AudDrvAddBuffQue(WAVSTUD_ACT actType, AUDIO_CH audCh, PAUDIO_BUF_QUEUE pAudBufQue, BOOL trigger)
{
	KDRV_BUFFER_INFO     buf_info = {0};
	KDRV_CALLBACK_FUNC   buf_cb = {0};
	UINT32 eid = WavStud_GetAudObj(actType);
	UINT32 queue_addr = 0;

	if (pAudBufQue == NULL) {
		kdrv_audioio_trigger(eid, 0, 0, 0);
		return FALSE;
	}

	buf_info.addr_pa = pAudBufQue->uiAddress;
	buf_info.addr_va = pAudBufQue->uiAddress;
	buf_info.size    = pAudBufQue->uiSize;

	buf_info.ddr_id  = trigger;


	switch (actType){
		case WAVSTUD_ACT_REC:{
			buf_cb.callback = wavstud_rec_buf_cb;
		}
		break;
		case WAVSTUD_ACT_REC2:{

		}
		break;
        case WAVSTUD_ACT_PLAY:{
			buf_cb.callback = wavstud_play_buf_cb;
		}
		break;
		case WAVSTUD_ACT_PLAY2:{
			buf_cb.callback = wavstud_play2_buf_cb;
		}
		break;
        case WAVSTUD_ACT_LB:{
			buf_cb.callback = wavstud_lb_buf_cb;
		}
		break;
        default:
		break;
    }
	buf_cb.reserve_buf = wavstud_reserve_buf;
	buf_cb.free_buf    = wavstud_free_buf;

	queue_addr = (UINT32)pAudBufQue;

	if (trigger) {
		if (kdrv_audioio_trigger(eid, &buf_info, &buf_cb, (VOID*)&queue_addr) != 0) {
			return FALSE;
		}
	} else {
		if (kdrv_audioio_trigger_not_start(eid, &buf_info, &buf_cb, (VOID*)&queue_addr) != 0) {
			return FALSE;
		}
	}

	return TRUE;
}
PAUDIO_BUF_QUEUE WavStud_AudDrvGetDoneBufQue(WAVSTUD_ACT actType, AUDIO_CH audCh)
{
	/*PAUDTS_OBJ pAudDrvObj = gWavStudTsObj[actType];
	return (pAudDrvObj->getDoneBufferFromQueue(WavStud_GetAudDrvQueSel(actType, audCh)));*/
	return FALSE;
}
PAUDIO_BUF_QUEUE WavStud_AudDrvGetCurrBufQue(WAVSTUD_ACT actType, AUDIO_CH audCh)
{
	/*PAUDTS_OBJ pAudDrvObj = gWavStudTsObj[actType];
	return (pAudDrvObj->getCurrentBufferFromQueue(WavStud_GetAudDrvQueSel(actType, audCh)));*/
	return FALSE;
}

ER WavStud_AudDrvInit(WAVSTUD_ACT actType, PWAVSTUD_AUD_INFO audInfo, UINT32 audVol, UINT32 fpEventCB)
{
	ER retV = E_OK;

	#if 1
	UINT32 maxVolLvl = 0;
	UINT32 audSR = audInfo->aud_sr, audCh = audInfo->aud_ch, audChNum = audInfo->ch_num;
	UINT32 eid = 0;
	UINT32 param[3] = {0};
	INT32 param_int[3] = {0};
	CTL_AUD_ID aud_id = 0;
	PWAVSTUD_ACT_OBJ pActObj = &(gWavStudObj.actObj[actType]);

	DBG_MSG("SR:%d, CH:%d\r\n", audSR, audCh);
	#endif

	eid = WavStud_GetAudObj(actType);

	if (actType == WAVSTUD_ACT_PLAY || actType == WAVSTUD_ACT_PLAY2) {
		aud_id = CTL_AUD_ID_OUT;
	} else {
		aud_id = CTL_AUD_ID_CAP;
	}

	//#NT#2015/11/10#HM Tseng -begin
	//#NT# Open driver object twice
	if (actType == WAVSTUD_ACT_REC2) {
		return retV;
	}

	#if 0 /*for 510 only*/
	if (WAVSTUD_ACT_REC == actType) {
		if (gWavStudObj.codec == WAVSTUD_AUD_CODEC_EMBEDDED) {
			Dx_Control(gWavStudDxSndHdl, DXSOUND_CAP_SWITCH_EXT_CODEC, FALSE, 0);
		} else {
			Dx_Control(gWavStudDxSndHdl, DXSOUND_CAP_SWITCH_EXT_CODEC, TRUE, 0);
		}
	}
	#endif


	//#NT#2015/11/10#HM Tseng -end
	retV = kdrv_audioio_open(0, KDRV_DEV_ID_ENGINE(eid));

	if (retV != 0) {
		DBG_ERR("pAudDrvObj->open fail = %d\r\n", retV);
		return retV;
	}


	//set for external device
	retV = ctl_aud_module_open(aud_id);
	if (retV == -1) {
		DBG_ERR("ctl_aud open fail = %d\r\n", retV);
		return retV;
	} else if (retV == -2) {
		//embedded codec set CAP path
		if (WAVSTUD_ACT_REC == actType) {
			if (gWavStudObj.rec_src == 0) {
				param[0] = KDRV_AUDIO_CAP_PATH_AMIC;
				kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_PATH, (VOID*)&param[0]);
			} else {
				param[0] = KDRV_AUDIO_CAP_PATH_DMIC;
				kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_PATH, (VOID*)&param[0]);

				if (audChNum > 2) {
					param[0] = 4;
					kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_DMIC_CH, (VOID*)&param[0]);
				} else {
					param[0] = 2;
					kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_DMIC_CH, (VOID*)&param[0]);
				}
			}
		}
	}

#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() && wavstudio_first_boot && actType == WAVSTUD_ACT_REC) {
		param[0] = gWavStudObj.default_set;
		kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_DEFAULT_CFG, (VOID*)&param[0]);
		if (gWavStudObj.ng_thd != 0) {
			param_int[0] = gWavStudObj.ng_thd;
			kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_NOISEGATE_THRESHOLD, (VOID*)&param_int[0]);
		}

		if (gWavStudObj.alc_en >= 0) {
			param[0] = gWavStudObj.alc_en;
			kdrv_audioio_set(WavStud_GetAudObj(WAVSTUD_ACT_REC), KDRV_AUDIOIO_CAP_ALC_EN, (VOID*)&param[0]);
		}
		return E_OK;
	}
#endif

	//Set OutDevIdx, SR, CH, ...?
	#if defined __UITRON || defined __ECOS
	Dx_Control(gWavStudDxSndHdl, DXSOUND_CAP_SET_OUTDEV, gWavStudObj.playOutDev, (UINT32)pAudDrvObj);
	Dx_Control(gWavStudDxSndHdl, DXSOUND_CAP_SAMPLERATE, audSR, (UINT32)pAudDrvObj);
	#else
	if (actType == WAVSTUD_ACT_PLAY || actType == WAVSTUD_ACT_PLAY2) {
		if (retV == -2 && gWavStudObj.playOutDev == 3) {
			//don't enable external codec if open failed
			DBG_IND("Open exteranl codec failed.\r\n");
		} else {
			param[0] = gWavStudObj.playOutDev;
			kdrv_audioio_set(eid, KDRV_AUDIOIO_OUT_PATH, (VOID*)&param[0]);
		}
	}
	param[0] = audSR;
	kdrv_audioio_set(eid, KDRV_AUDIOIO_GLOBAL_SAMPLE_RATE, (VOID*)&param[0]);
	#endif

	retV = ctl_aud_module_set_cfg(aud_id, CTL_AUDDRV_CFGID_SET_SAMPLE_RATE, (void *)&audSR);
	if (retV != 0) {
		DBG_IND("ctl_aud set sample rate fail = %d\r\n", retV);
		return retV;
	}

	if (WAVSTUD_ACT_REC == actType) {
		if (audChNum == 2 && audCh != AUDIO_CH_STEREO){
			param[0] = TRUE;
		} else {
			param[0] = FALSE;
		}
		kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_MONO_EXPAND, (VOID*)&param[0]);
	}

	maxVolLvl = 160;
	if (((UINT32)audVol) > maxVolLvl) { //fix for CID 42874, 44912
		DBG_WRN("Vol too large=%d, scale down to 100\r\n", audVol);
		audVol = 100;  //fix for CID 42874
	}

	//pAudDrvObj->setConfig(AUDTS_CFG_ID_EVENT_HANDLE, (UINT32)fpEventCB);
	//retV = pAudDrvObj->setSamplingRate(audSR);
	if (actType == WAVSTUD_ACT_PLAY || actType == WAVSTUD_ACT_PLAY2) {
		param[0] = 0;
		kdrv_audioio_set(eid, KDRV_AUDIOIO_OUT_MONO_EXPAND, (VOID*)&param[0]);

		if (audCh == AUDIO_CH_STEREO) {
			param[0] = audChNum;
			kdrv_audioio_set(eid, KDRV_AUDIOIO_OUT_CHANNEL_NUMBER, (VOID*)&param[0]);
		} else if (audCh == AUDIO_CH_LEFT){
			param[0] = KDRV_AUDIO_OUT_MONO_LEFT;
			kdrv_audioio_set(eid, KDRV_AUDIOIO_OUT_MONO_SEL, (VOID*)&param[0]);
		} else {
			param[0] = KDRV_AUDIO_OUT_MONO_RIGHT;
			kdrv_audioio_set(eid, KDRV_AUDIOIO_OUT_MONO_SEL, (VOID*)&param[0]);
		}
	} else {
		if (audCh == AUDIO_CH_STEREO) {
			param[0] = audChNum;
			kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_CHANNEL_NUMBER, (VOID*)&param[0]);
		} else if (audCh == AUDIO_CH_LEFT){
			param[0] = KDRV_AUDIO_OUT_MONO_LEFT;
			kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_MONO_SEL, (VOID*)&param[0]);
		} else {
			param[0] = KDRV_AUDIO_OUT_MONO_RIGHT;
			kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_MONO_SEL, (VOID*)&param[0]);
		}

		param[0] = gWavStudObj.default_set;
		kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_DEFAULT_CFG, (VOID*)&param[0]);
		if (gWavStudObj.ng_thd != 0) {
			param_int[0] = gWavStudObj.ng_thd;
			kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_NOISEGATE_THRESHOLD, (VOID*)&param_int[0]);
		}

		if (gWavStudObj.alc_en >= 0) {
			param[0] = gWavStudObj.alc_en;
			kdrv_audioio_set(WavStud_GetAudObj(WAVSTUD_ACT_REC), KDRV_AUDIOIO_CAP_ALC_EN, (VOID*)&param[0]);
			if (gWavStudObj.alc_cfg_en > 0) {
				param[0] = gWavStudObj.alc_decay_time;
				kdrv_audioio_set(WavStud_GetAudObj(WAVSTUD_ACT_REC), KDRV_AUDIOIO_CAP_ALC_DECAY_TIME, (VOID*)&param[0]);
				param[0] = gWavStudObj.alc_attack_time;
				kdrv_audioio_set(WavStud_GetAudObj(WAVSTUD_ACT_REC), KDRV_AUDIOIO_CAP_ALC_ATTACK_TIME, (VOID*)&param[0]);
				param[0] = gWavStudObj.alc_max_gain;
				kdrv_audioio_set(WavStud_GetAudObj(WAVSTUD_ACT_REC), KDRV_AUDIOIO_CAP_ALC_MAXGAIN, (VOID*)&param[0]);
				param[0] = gWavStudObj.alc_min_gain;
				kdrv_audioio_set(WavStud_GetAudObj(WAVSTUD_ACT_REC), KDRV_AUDIOIO_CAP_ALC_MINGAIN, (VOID*)&param[0]);
			}
		}

		if (gWavStudObj.rec_gain_level != 0) {
			param[0] = gWavStudObj.rec_gain_level;
			kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_GAIN_LEVEL, (VOID*)&param[0]);
		}
	}

	if (WAVSTUD_ACT_REC == actType) {
		if (pActObj->global_vol == TRUE) {
			WavStud_AudDrvSetVol(actType, WAVSTUD_PORT_GLOBAL, audVol);
		} else {
			WavStud_AudDrvSetVol(actType, WAVSTUD_PORT_0, pActObj->p0_vol);
			WavStud_AudDrvSetVol(actType, WAVSTUD_PORT_1, pActObj->p1_vol);
		}
	} else if (WAVSTUD_ACT_PLAY == actType){
		if (pActObj->global_vol == TRUE) {
			WavStud_AudDrvSetVol(actType, WAVSTUD_PORT_GLOBAL, audVol);
		} else {
			WavStud_AudDrvSetVol(actType, WAVSTUD_PORT_0, pActObj->p0_vol);
		}
	} else if (WAVSTUD_ACT_PLAY2 == actType){
		if (pActObj->global_vol == TRUE) {
			WavStud_AudDrvSetVol(actType, WAVSTUD_PORT_GLOBAL, audVol);
		} else {
			WavStud_AudDrvSetVol(actType, WAVSTUD_PORT_1, pActObj->p1_vol);
		}
	}

	return retV;
}

ER WavStud_AudLBDrvInit(AUDIO_SR audSR, AUDIO_CH audCh, UINT32 fpEventCB)
{
	ER retV = E_OK;
	UINT32 eid = 0;
	UINT32 param[3] = {0};

	DBG_MSG("SR:%d, CH:%d\r\n", audSR, audCh);

	eid = WavStud_GetAudObj(WAVSTUD_ACT_LB);

	//#NT#2015/11/10#HM Tseng -end
	retV = kdrv_audioio_open(0, KDRV_DEV_ID_ENGINE(eid));

	if (retV != 0) {
		DBG_ERR("pAudDrvObj->open fail = %d\r\n", retV);
		return retV;
	}

#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() && wavstudio_first_boot) {
		return E_OK;
	}
#endif

	param[0] = audSR;
	kdrv_audioio_set(eid, KDRV_AUDIOIO_GLOBAL_SAMPLE_RATE, (VOID*)&param[0]);

	if (audCh == AUDIO_CH_STEREO) {
		param[0] = 2;
		kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_CHANNEL_NUMBER, (VOID*)&param[0]);
	} else if (audCh == AUDIO_CH_LEFT){
		param[0] = KDRV_AUDIO_OUT_MONO_LEFT;
		kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_MONO_SEL, (VOID*)&param[0]);
	} else {
		param[0] = KDRV_AUDIO_OUT_MONO_RIGHT;
		kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_MONO_SEL, (VOID*)&param[0]);
	}


	return retV;
}

void WavStud_AudDrvExit(WAVSTUD_ACT actType)
{
	UINT32 eid = 0;
	UINT32 retV = 0;
	CTL_AUD_ID aud_id = 0;

	eid = WavStud_GetAudObj(actType);
	if (actType == WAVSTUD_ACT_PLAY || actType == WAVSTUD_ACT_PLAY2) {
		aud_id = CTL_AUD_ID_OUT;
	} else {
		aud_id = CTL_AUD_ID_CAP;
	}

	if (actType == WAVSTUD_ACT_PLAY || actType == WAVSTUD_ACT_PLAY2 || actType == WAVSTUD_ACT_REC) {
		retV = ctl_aud_module_stop(aud_id);
		if (retV != 0) {
			DBG_ERR("ctl_aud stop fail = %d\r\n", retV);
		}

		retV = ctl_aud_module_close(aud_id);
		if (retV != 0) {
			DBG_ERR("ctl_aud close fail = %d\r\n", retV);
		}
	}

	kdrv_audioio_close(0, KDRV_DEV_ID_ENGINE(eid));
}

ER WavStud_AudDrvSetVol(WAVSTUD_ACT act_type, WAVSTUD_PORT port, UINT32 vol)
{
	UINT32 ret = E_OK;
	UINT32 eid = 0;
	CTL_AUD_ID aud_id = 0;
	UINT32 param[3] = {0};

	param[0] = vol;

	if (port > WAVSTUD_PORT_1) {
		DBG_ERR("Invaild port=%d\r\n", port);
	}

	if (act_type == WAVSTUD_ACT_PLAY || act_type == WAVSTUD_ACT_PLAY2) {
		eid = WavStud_GetAudObj(act_type);
		kdrv_audioio_set(eid, KDRV_AUDIOIO_OUT_VOLUME + port, (VOID*)&param[0]);
		aud_id = CTL_AUD_ID_OUT;
	} else {
		eid = WavStud_GetAudObj(act_type);
		kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_VOLUME + port, (VOID*)&param[0]);
		aud_id = CTL_AUD_ID_CAP;
	}

	//set for external device
	ret = ctl_aud_module_set_cfg(aud_id, CTL_AUDDRV_CFGID_SET_VOLUME, (void *)&vol);
	if (ret != 0) {
		DBG_ERR("ctl_aud set volume fail = %d\r\n", ret);
		return ret;
	}

	return ret;
}