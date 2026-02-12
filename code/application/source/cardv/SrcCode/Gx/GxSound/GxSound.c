#include "GxSoundIntr.h"
#include "GxSound.h"
#include "sxsound/PlaySoundInt.h"

//========== Glaval Variables =======
//AUDTS_CH gGxsndPlayAudCh = AUDTS_CH_TX2;
UINT32 gGxSndRepPlayCnt = 0;//Play a sound repeatly for maximum 10 times

#define __MODULE__          gxsound
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int gxsound_debug_level = NVT_DBG_WRN;

//NVTVER_VERSION_ENTRY(GxSound, 1, 00, 000, 00)

//========== Functions =======
ER GxSound_AudAction(GXSND_AUD_ACTION audAct)
{
	#if _TODO
	PAUDTS_OBJ pAudPlayObj = aud_getTransceiveObject(gGxsndPlayAudCh);
	DBG_IND(":%d\r\n", audAct);
	if (0 == pAudPlayObj) {
		DBG_ERR("pAudPlayObj NULL\r\n");
		return E_OACV;
	} else if (FALSE == pAudPlayObj->isOpened()) {
		DBG_ERR("pAudPlayObj Not Opened\r\n");
		return E_OBJ;
	} else if (audAct >= GXSND_AUD_ACTION_MAX) {
		DBG_ERR("%d > GXSND_AUD_ACTION_MAX=%d\r\n", audAct, GXSND_AUD_ACTION_MAX);
		return E_PAR;
	}
	if (GXSND_AUD_ACTION_PAUSE == audAct) {
		pAudPlayObj->pause();
	}
	if (GXSND_AUD_ACTION_RESUME == audAct) {
		pAudPlayObj->resume();
	}
	if (GXSND_AUD_ACTION_CLOSE == audAct) {
		pAudPlayObj->stop();
		pAudPlayObj->close();
		//pAudPlayObj = 0; //fix for CID 60999
		aud_close();
	}
	#else
	HD_RESULT ret;

	DBG_IND(":%d\r\n", audAct);

	if (GXSND_AUD_ACTION_CLOSE == audAct) {
		if((ret = hd_audioout_stop(gxsound_path_id)) != HD_OK) {
			return ret;
		}
		if((ret = hd_audioout_close(gxsound_path_id)) != HD_OK) {
			return ret;
		}
	} else {
		DBG_ERR("Not supported act:%d\r\n", audAct);
	}
	#endif
	return E_OK;
}

void GxSound_Play_Repeat(int index, UINT32 times)
{
	DBG_IND(":idx=%d,%d\r\n", index, times);
	gGxSndRepPlayCnt = (times > SOUND_PLAY_MAX_TIMES) ? SOUND_PLAY_MAX_TIMES : times;
	GxSound_SetSoundData(GxSound_GetSoundDataByID(index));
	GxSound_SetSoundSR(GxSound_GetSoundSRByID(index));
	GxSound_Control(SOUND_CTRL_PLAY);
}
void GxSound_Play(int index)
{
	GxSound_Play_Repeat(index, 1);
}

void GxSound_Stop(void)
{
	GxSound_Control(SOUND_CTRL_STOP);
}

void GxSound_WaitStop(void)
{
	GxSound_Control(SOUND_CTRL_WAITSTOP);
}

extern FPSOUNDCB g_fpSoundCB;

#if _TODO
static BOOL bLastDet[1] = {FALSE};
static BOOL bLastPlug[1] = {FALSE};
#endif

/**
  Detect video is plugging in or unplugged

  Detect video is plugging in or unplugged.

  @param void
  @return void
*/
void GxSound_DetInsert(UINT32 DevID, UINT32 context)
{
	#if _TODO
	BOOL bCurDet, bCurPlug;
	bCurDet = FALSE;

#if 0
	if (Dx_Getcaps((DX_HANDLE)DevID, DISPLAY_CAPS_BASE, 0) & DISPLAY_BF_DETPLUG)
#endif
		bCurDet = Dx_Getcaps((DX_HANDLE)DevID, DXSOUND_CAP_GET_AUDOUTDEV, 0); //detect current plug status

	bCurPlug = (BOOL)(bCurDet & bLastDet[context]); //if det is TRUE twice => plug is TRUE

	//if ((bCurPlug != bLastPlug[context]) || (bForceDetIns)) //if plug is changed
	if (bCurPlug != bLastPlug[context]) {
#if 0
		if (bForceDetIns) {
			DBG_IND("force detect ins\r\n");
		}
#endif
		// audio is plugging in
		if (bCurPlug == TRUE) {
			DBG_IND("Audio plug!\r\n");
			if (g_fpSoundCB) {
				g_fpSoundCB(SOUND_CB_PLUG, DevID, 0);
			}
		}
		// audio is unplugged
		else {
			DBG_IND("Audio unplug!\r\n");
			if (g_fpSoundCB) {
				g_fpSoundCB(SOUND_CB_UNPLUG, DevID, 0);
			}
		}
#if 0
		bForceDetIns = 0;
#endif
	}

	bLastDet[context] = bCurDet; //save det
	bLastPlug[context] = bCurPlug;//save plug
	#endif
}

void GxSound_SetPlayCount(UINT32 count)
{
	gGxSndRepPlayCnt = count;
}
