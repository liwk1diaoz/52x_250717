/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       PlaySoundAPI.c
    @ingroup    mIPRJAPKey

    @brief      Play Sound task API
                This file includes all APIs of play sound task

    @note       Nothing.

    @date       2006/01/23
*/

/** \addtogroup mIPRJAPKey */
//@{

#include "PlaySoundInt.h"
#include "GxSound.h"
#include "vendor_common.h"
#include <kwrap/util.h> 	//for sleep API

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          gxsound_api
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass

#include "kwrap/debug.h"
unsigned int gxsound_api_debug_level = NVT_DBG_WRN;
module_param_named(gxsound_api_debug_level, gxsound_api_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(gxsound_api_debug_level, "gxsound_api debug level");
///////////////////////////////////////////////////////////////////////////////

//DX_HANDLE gGxSndDrvSndHdl = 0;
FPSOUNDCB g_fpSoundCB = NULL;
BOOL                            bPlaySoundOpened = FALSE;
PSOUND_DATA                     gpPlaySoundDataAry = NULL;
UINT32                          gpPlaySoundDataAryCnt = 0;
SOUND_DATA                      *gPlaySoundData = 0;
//UINT32                          gPlaySoundIdx = 0;
UINT32 guiSoundEn = TRUE;
UINT32 gSoundAudSR = 8000;
UINT32 gSndOutDevConfigIdx = 0;
UINT32 gSndVol = 50;
SOUND_MEM gxsound_mem = {0};

extern UINT32 g_uiPlaySoundStatus;



void GxSound_SetOutDevConfigIdx(UINT32 outDevConfigIdx)
{
  HD_RESULT ret;
  HD_AUDIOOUT_DRV_CONFIG audio_driver_cfg_param = {0};
  INT32 pwr_en = TRUE;

        DBG_IND("N=%d,Ori=%d\r\n", outDevConfigIdx, gSndOutDevConfigIdx);
        gSndOutDevConfigIdx = outDevConfigIdx;

        ret = hd_audioout_open(0, HD_AUDIOOUT_1_CTRL, &gxsound_ctrl_id); //open this for device control
        if (ret != HD_OK) {
                return;
        }

        /* set audio out driver parameters */
        audio_driver_cfg_param.mono = HD_AUDIO_MONO_LEFT;
        audio_driver_cfg_param.output = (gSndOutDevConfigIdx == 0)? HD_AUDIOOUT_OUTPUT_SPK : HD_AUDIOOUT_OUTPUT_LINE;
        ret = hd_audioout_set(gxsound_ctrl_id, HD_AUDIOOUT_PARAM_DRV_CONFIG, &audio_driver_cfg_param);
        if (ret != HD_OK) {
                return;
        }

	extern UINT32 System_IsBootFromRtos(void);
	if (System_IsBootFromRtos() == FALSE) {
		ret = vendor_audioout_set(gxsound_ctrl_id, VENDOR_AUDIOOUT_ITEM_PREPWR_ENABLE, (VOID *)&pwr_en);
		if (ret != HD_OK) {
			DBG_ERR("VENDOR_AUDIOOUT_ITEM_PREPWR_ENABLE failed=%x\r\n", ret);
			return;
		}
	}
}
void GxSound_SetAoutPrePwr(INT32 pwr_en)
{
	HD_RESULT ret;
	ret = vendor_audioout_set(gxsound_ctrl_id, VENDOR_AUDIOOUT_ITEM_PREPWR_ENABLE, (VOID *)&pwr_en);
	if (ret != HD_OK) {
		DBG_ERR("VENDOR_AUDIOOUT_ITEM_PREPWR_ENABLE failed=%x\r\n", ret);
		return;
	}
}
void GxSound_ResetParam(void)
{
	gSndOutDevConfigIdx = 0;
}

void GxSound_SetSoundSR(UINT32 audSR)
{
	DBG_IND(":%d,Ori=%d\r\n", audSR, gSoundAudSR);
	gSoundAudSR = audSR;
}
UINT32 GxSound_GetSoundSRByID(UINT32 uiSoundID)
{
	#if 1
	PSOUND_DATA pSoudData = 0;  //fix for CID 60802
	DBG_IND(": uiSoundID=%d\r\n", uiSoundID);
	if (uiSoundID >= gpPlaySoundDataAryCnt) {
		DBG_ERR(":Idx=%d out range=%d\r\n", uiSoundID, gpPlaySoundDataAryCnt);
		return (8000);
	}
	pSoudData = (PSOUND_DATA)(gpPlaySoundDataAry + uiSoundID);
	DBG_IND(": SR=%d\r\n", pSoudData->sampleRate);
	return (pSoudData->sampleRate);
	#else
	return 0;
	#endif
}

/**
  Enable/Disable keypad tone

  Enable/Disable keypad tone

  @param BOOL bEn: TRUE -> Enable, FALSE -> Disable
  @return void
*/
void GxSound_EnableSound(BOOL bEn)
{
	DBG_IND(": bEn=%d, Ori-bEn=%d\r\n", bEn, guiSoundEn);
	guiSoundEn = bEn;
	DBG_IND(": Ori-bEn=%d\r\n", guiSoundEn);
}

/**
  Check whether keypad tone is enabled or not

  Check whether keypad tone is enabled or not

  @param void
  @return BOOL: TRUE -> Enable, FALSE -> Disable
*/
BOOL GxSound_IsSoundEnabled(void)
{
	DBG_IND(": Ori-bEn=%d\r\n", guiSoundEn);
	return guiSoundEn;
}

/**
  Check whether is play or stop

  Check whether is play or stop

  @param void
  @return BOOL: TRUE -> play, FALSE -> stop
*/
BOOL GxSound_IsPlaying(void)
{
	return (g_uiPlaySoundStatus == PLAYSOUND_STS_PLAYING);
}


/**
  Open Play Sound task

  Open Play Sound task.
  The parameter is not using now, just for compliance

  Return value is listed below:
  E_SYS:Init. Audio fail
  E_OK: No error or Task is already opened

  @param PPLAYSOUND_APPOBJ pWavObj: Play Sound application object
  @return ER
*/
ER GxSound_Open(FPSOUNDCB fpSoundCB)
{
	ER retV = E_OK;
	HD_RESULT ret;

	DBG_IND(": open=%d,sts=%d,moduleId=%d\r\n", bPlaySoundOpened, g_uiPlaySoundStatus, PLAYSOUNDTSK_ID);

	ret = hd_audioout_init();
	if (ret != HD_OK) {
		DBG_ERR("hd_audioout init failed\r\n");
		return E_SYS;
	}

	GxSound_InstallID();

	if (bPlaySoundOpened == TRUE) {
		DBG_IND("--GxSound Opened already\r\n");
		return retV;
	}

	THREAD_CREATE(PLAYSOUNDTSK_ID, PlaySoundTsk, NULL, "PlaySoundTsk");
	THREAD_CREATE(PLAYDATATSK_ID, PlayDataTsk, NULL, "PlayDataTsk");

	if (PLAYSOUNDTSK_ID == 0) {
		DBG_ERR("PLAYSOUNDTSK_ID = %d\r\n", (unsigned int)PLAYSOUNDTSK_ID);
		return E_SYS;
	}
	THREAD_RESUME(PLAYSOUNDTSK_ID);

	if (PLAYDATATSK_ID == 0) {
        DBG_ERR("PLAYDATATSK_ID = %d\r\n", (unsigned int)PLAYDATATSK_ID);
        return E_SYS;
    }
    THREAD_RESUME(PLAYDATATSK_ID);

	//gGxSndDrvSndHdl = dxSndHdl;
	g_fpSoundCB = fpSoundCB;
	clr_flg(FLG_ID_SOUND, FLGSOUND_ALL);
	clr_flg(FLG_ID_DATA, FLGDATA_ALL);

	g_uiPlaySoundStatus = PLAYSOUND_STS_STOPPED;
	bPlaySoundOpened    = TRUE;
	GxSound_ResetParam();

	set_flg(FLG_ID_SOUND, FLGSOUND_STOPPED);
	//sta_tsk(PLAYSOUNDTSK_ID, 0);

	DBG_IND(": open=%d,sts=%d,retV=%d\r\n", bPlaySoundOpened, g_uiPlaySoundStatus, retV);
	return retV;
}

/**
  Close Play Sound task

  Close Play Sound task.

  Return value is listed below:
  E_SYS: Task is already closed
  E_OK: No error

  @param void
  @return ER
*/
ER GxSound_Close(void)
{
	FLGPTN uiFlag;
	HD_RESULT ret;

	DBG_IND(": open=%d,sts=%d\r\n", bPlaySoundOpened, g_uiPlaySoundStatus);

	if (bPlaySoundOpened == FALSE) {
		return E_SYS;
	}

	//Unplug usb,no power down sound
	if (kchk_flg(FLG_ID_SOUND, FLGSOUND_PLAY) || (g_uiPlaySoundStatus == PLAYSOUND_STS_PLAYING)) {
		DBG_IND(":Sts=%d\r\n", g_uiPlaySoundStatus);
		wai_flg(&uiFlag, FLG_ID_SOUND, FLGSOUND_STOPPED, TWF_ORW | TWF_CLR);
	}

	set_flg(FLG_ID_SOUND, FLGSOUND_EXIT);
	set_flg(FLG_ID_DATA, FLGDATA_EXIT);

	wai_flg(&uiFlag, FLG_ID_SOUND, FLGSOUND_QUIT, TWF_ORW | TWF_CLR);
	wai_flg(&uiFlag, FLG_ID_DATA, FLGDATA_QUIT, TWF_ORW | TWF_CLR);

	bPlaySoundOpened = FALSE;

	GxSound_UninstallID();

	ret = hd_audioout_uninit();
	if (ret != HD_OK) {
		DBG_ERR("hd_audioout uninit failed\r\n");
		return E_SYS;
	}

	DBG_IND(": open=%d,sts=%d\r\n", bPlaySoundOpened, g_uiPlaySoundStatus);
	return E_OK;
}

void GxSound_SetSoundData(SOUND_DATA *pSound)
{
	DBG_IND(": pSound=0x%x\r\n", pSound);
	gPlaySoundData = pSound;
	DBG_IND(": pSound=0x%x\r\n", gPlaySoundData);
}

#if 0
void GxSound_SetSoundDataIdx(UINT32 index)
{
	DBG_IND(": index=%d\r\n", index);
	gPlaySoundIdx = index;
	DBG_IND(": index=%d\r\n", gPlaySoundIdx);
}

UINT32 GxSound_GetSoundDataIdx(void)
{
	DBG_IND(": index=%d\r\n", gPlaySoundIdx);
	return gPlaySoundIdx;
}
#endif

void GxSound_Control(int cmd)
{
	FLGPTN uiFlag;
	UINT16 uiWaitCnt = 0;
	
	DBG_IND(": cmd=%d,soundEn=%d,open=%d,sts=%d\r\n", cmd, guiSoundEn, bPlaySoundOpened, g_uiPlaySoundStatus);
	if (bPlaySoundOpened == FALSE) {
		DBG_ERR("GxSound_Control: Sound Module Not Open\r\n");
		return;
	}
	switch (cmd) {
	case SOUND_CTRL_PLAY:
		if (guiSoundEn == FALSE) {
			DBG_IND(": Play But Sound Disable\r\n");
			return;
		}
		//Wait current sound stop playing
		DBG_IND("#Snd:%d\r\n", g_uiPlaySoundStatus);
		if (g_uiPlaySoundStatus == PLAYSOUND_STS_PLAYING) {
			wai_flg(&uiFlag, FLG_ID_SOUND, FLGSOUND_STOPPED, TWF_ORW | TWF_CLR);
		}
		set_flg(FLG_ID_SOUND, FLGSOUND_PLAY);
		while(g_uiPlaySoundStatus != PLAYSOUND_STS_PLAYING){
			if(uiWaitCnt++ > 100)
				break;
			vos_util_delay_ms(10);
		}
		break;

	case SOUND_CTRL_STOP:
		if (g_uiPlaySoundStatus == PLAYSOUND_STS_PLAYING) {
			set_flg(FLG_ID_SOUND, FLGSOUND_STOP);
			//wait util audio really stop
			wai_flg(&uiFlag, FLG_ID_SOUND, FLGSOUND_STOPPED, TWF_ORW | TWF_CLR);
		}
		DBG_IND(": open=%d,sts=%d\r\n", bPlaySoundOpened, g_uiPlaySoundStatus);
		break;
	case SOUND_CTRL_WAITSTOP:
		DBG_IND("#Snd:%d\r\n", g_uiPlaySoundStatus);
		if (g_uiPlaySoundStatus == PLAYSOUND_STS_PLAYING) {
			wai_flg(&uiFlag, FLG_ID_SOUND, FLGSOUND_STOPPED, TWF_ORW | TWF_CLR);
		}
		break;
	default:
		DBG_ERR(": Not Support cmd=%d\r\n", cmd);
		break;
	}

	DBG_IND(": cmd=%d,soundEn=%d,open=%d,sts=%d\r\n", cmd, guiSoundEn, bPlaySoundOpened, g_uiPlaySoundStatus);
}

ER GxSound_SetSoundTable(UINT32 soundAryCnt, PSOUND_DATA pSoundAry)
{
	DBG_IND(":soundAryCnt=%d,pSoundAry=0x%x\r\n", soundAryCnt, (UINT32)pSoundAry);
	if ((!soundAryCnt) || (!pSoundAry)) {
		DBG_ERR(":soundAryCnt=0x%x,pSoundAry=0x%x\r\n", soundAryCnt, (UINT32)pSoundAry);
		return E_PAR;
	}
	gpPlaySoundDataAry = pSoundAry;
	gpPlaySoundDataAryCnt = soundAryCnt;
	return E_OK;
}

SOUND_DATA *GxSound_GetSoundDataByID(UINT32 uiSoundID)
{
	DBG_IND(": uiSoundID=%d\r\n", uiSoundID);
	if (uiSoundID >= gpPlaySoundDataAryCnt) {
		DBG_ERR(":Idx=%d out range=%d\r\n", uiSoundID, gpPlaySoundDataAryCnt);
		return 0;
	}
	DBG_IND(": retV=%d\r\n", (gpPlaySoundDataAry + uiSoundID));
	return (gpPlaySoundDataAry + uiSoundID);
}


UINT32 GxSound_GetVolume(void)
{
	return gSndVol;
}
void GxSound_SetVolume(UINT32 vol)
{
	DBG_IND(":%d\r\n", vol);

	gSndVol = vol;
}

ER GxSound_TxfWav2PCM(SOUND_DATA *pSndData)
{
	#if 1//_TODO
	WAV_PCMHEADER *pWAVPCMHeader = 0;

	DBG_IND(":addr=0x%x,Ch=%d,SR=%d,Size=0x%x,sndId=0x%x\r\n", \
			pSndData->puiData, pSndData->isMono, pSndData->sampleRate, pSndData->uiSize, pSndData->soundId);
	if (0 == pSndData) {
		DBG_ERR(":SndData FULL\r\n");
		return E_PAR;
	}
	pWAVPCMHeader = (WAV_PCMHEADER *)(pSndData->puiData);
	//GxSound_DumpWavHeader(pWAVPCMHeader);
	if (GXSOUND_WAV_HEADER_RIFF != pWAVPCMHeader->uiHeaderID) {
		DBG_ERR("WavRiff:0x%x\r\n", pWAVPCMHeader->uiHeaderID);
		return E_ID;
	}
	if (GXSOUND_WAV_RIFFTYPE_WAVE != pWAVPCMHeader->uiRIFFType) {
		DBG_ERR("WavWave:0x%x\r\n", pWAVPCMHeader->uiHeaderID);
		return E_ID;
	}
	if (GXSOUND_WAV_FORMAT_ID != pWAVPCMHeader->uiFormatID) {
		DBG_ERR("WavFmt :0x%x\r\n", pWAVPCMHeader->uiHeaderID);
		return E_ID;
	}
	pSndData->puiData += sizeof(WAV_PCMHEADER);
	pSndData->uiSize = pWAVPCMHeader->uiDataSize;
	pSndData->sampleRate = pWAVPCMHeader->uiSamplingRate;
	pSndData->soundId = GXSOUND_NONTBL_SND_ID;
	DBG_IND(":addr=0x%x,Ch=%d,SR=%d,Size=0x%x,sndId=0x%x\r\n", pSndData->puiData, pSndData->isMono, pSndData->sampleRate, pSndData->uiSize, pSndData->soundId);
	#endif
	return E_OK;
}
ER GxSound_ActOnSndNotInTbl(SOUND_PLAY_EVENT sndEvt, SOUND_DATA *pSndData, UINT32 isPCM)
{
	ER retV = E_OK;
	DBG_IND(":evt=%d,addr=0x%x,isPCM=%d,isMono=%d\r\n", sndEvt, pSndData, isPCM, pSndData->isMono);

	if (bPlaySoundOpened == FALSE) {
		DBG_ERR(": Sound Module Not Open\r\n");
		return E_PAR;
	}

	if (SOUND_PLAY_START == sndEvt) {
		if (0 == pSndData) {
			DBG_ERR(":evt=%d,SndData NULL\r\n", sndEvt);
			return E_PAR;
		}
		if (FALSE == isPCM) {
			GxSound_TxfWav2PCM(pSndData);
		}
		DBG_IND(":addr=0x%x,Ch=%d,SR=%d,Size=0x%x,sndId=0x%x\r\n", pSndData->puiData, pSndData->isMono, pSndData->sampleRate, pSndData->uiSize, pSndData->soundId);
		GxSound_SetSoundSR(pSndData->sampleRate);
		GxSound_SetSoundData(pSndData);
		GxSound_Control(SOUND_CTRL_PLAY);
	} else if (SOUND_PLAY_STOP == sndEvt) {
		GxSound_Control(SOUND_CTRL_STOP);
	} else if (SOUND_PLAY_PAUSE == sndEvt) {
		GxSound_AudAction(GXSND_AUD_ACTION_PAUSE);
	} else if (SOUND_PLAY_RESUME == sndEvt) {
		GxSound_AudAction(GXSND_AUD_ACTION_RESUME);
	} else {
		DBG_ERR(":Unevt=%d,SndData BULL\r\n", sndEvt);
		return E_PAR;
	}
	DBG_IND(":evt=%d,addr=0x%x,retV=%d\r\n", sndEvt, pSndData, retV);
	return retV;
}

ER GxSound_Set_Config(SOUND_CONFIG cfg, UINT32 value) {

	UINT32 retV = E_OK;
	DBG_IND("cfg=%d, value=0x%x++\r\n", cfg, value);
	switch (cfg) {
	case SOUND_CONFIG_MEM:
		gxsound_mem = *(SOUND_MEM*)value;

		//DBG_DUMP("GxSound va=%x, pa=%x, size=%x\r\n", gxsound_mem.va, gxsound_mem.pa, gxsound_mem.size);

		break;
	default:
		retV = E_PAR;
		DBG_ERR("Not supported cfg=%d, 0x%x", cfg, value);
		break;
	}
	DBG_IND(" cfg=%d, 0x%x, retV=0x%x\r\n", cfg, value, retV);
	return retV;
}

//@}
