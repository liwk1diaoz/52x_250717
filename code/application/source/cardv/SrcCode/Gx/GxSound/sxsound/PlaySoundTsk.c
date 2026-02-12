/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       PlaySoundTsk.c
    @ingroup    mIPRJAPKey

    @brief      Play Startup, keypad tone...sound
                This task handles the sound playback of startup, keypad ...

    @note       Nothing.

    @date       2006/01/23
*/

/** \addtogroup mIPRJAPKey */
//@{

#include "PlaySoundTsk.h"
#include "PlaySoundInt.h"
#include "GxSound.h"
#include "kwrap/task.h"

#define __MODULE__          gxsound_play_tsk
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int gxsound_play_tsk_debug_level = NVT_DBG_WRN;

#define FRAME_SIZE 1024

UINT32 g_uiPlaySoundStatus;

extern UINT32 gSndVol;
extern UINT32 gSoundAudSR;
extern UINT32 gSndOutDevConfigIdx;
//extern DX_HANDLE gGxSndDrvSndHdl;
extern FPSOUNDCB g_fpSoundCB;
extern SOUND_DATA *gPlaySoundData;
//extern AUDTS_CH gGxsndPlayAudCh;
extern BOOL bPlaySoundOpened;
extern UINT32 gGxSndRepPlayCnt;

HD_PATH_ID gxsound_ctrl_id = 0;
HD_PATH_ID gxsound_path_id = 0;

BOOL play_stop = FALSE;

void _GxSound_Stop(void)
{
	if (g_uiPlaySoundStatus == PLAYSOUND_STS_PLAYING) {

		play_stop = TRUE;

		if (TRUE == bPlaySoundOpened) {
			GxSound_AudAction(GXSND_AUD_ACTION_CLOSE);
		}
		g_uiPlaySoundStatus = PLAYSOUND_STS_STOPPED;
		set_flg(FLG_ID_SOUND, FLGSOUND_STOPPED);
		if (g_fpSoundCB) {
			if (gPlaySoundData) {
				g_fpSoundCB(SOUND_CB_STOP, gPlaySoundData->soundId, 0);
			} else {
				g_fpSoundCB(SOUND_CB_STOP, 0, 0);
			}
		}
	}
}

void _GxSound_Play(SOUND_DATA *pSoundData)
{
	#if _TODO
	PAUDTS_OBJ pAudPlayObj = 0;
	AUDIO_BUF_QUEUE     AudioBufQueue;
	ER retV;

	if (!pSoundData) {
		DBG_ERR("PlaySound data is NULL\r\n");
		_GxSound_Stop();
		return;
	}
	if (pSoundData->puiData && pSoundData->uiSize) {
		AudioBufQueue.uiAddress = (UINT32) pSoundData->puiData;
		AudioBufQueue.uiSize    = pSoundData->uiSize;
		if (pSoundData->uiSize % 4) {
			DBG_WRN("Snd Data Size Not Word-Alignment=%d\r\n", pSoundData->uiSize);
			//better solution:make each snd ary size is 4 byte alignment,fill "0" for those non-existing bytes in ary.
		}
		if (1 < gGxSndRepPlayCnt) {
			DBG_IND("PlayCnt=%d\r\n", gGxSndRepPlayCnt);
			AudioBufQueue.pNext = &AudioBufQueue;
		} else {
			AudioBufQueue.pNext = 0;
		}
		DBG_IND("PlaySound Addr=0x%x,Size=0x%x\r\n", pSoundData->puiData, pSoundData->uiSize);
	} else {
		DBG_ERR("PlaySound Addr=0x%x,Size=0x%x is NULL\r\n", pSoundData->puiData, pSoundData->uiSize);
		_GxSound_Stop();
		return;
	}
	if (g_fpSoundCB) {
		if (gPlaySoundData) {
			g_fpSoundCB(SOUND_CB_START, gPlaySoundData->soundId, 0);
		} else {
			g_fpSoundCB(SOUND_CB_START, 0, 0);
		}
	}
	clr_flg(FLG_ID_SOUND, FLGSOUND_STOPPED);
	clr_flg(FLG_ID_SOUND, FLGSOUND_STOP);

	retV = aud_open();
	if (retV != E_OK) {
		DBG_ERR(": aud_open fail = %d\r\n", retV);
		return;
	}
	g_uiPlaySoundStatus = PLAYSOUND_STS_PLAYING;
	pAudPlayObj = aud_getTransceiveObject(gGxsndPlayAudCh);
	if (0 == pAudPlayObj) {
		DBG_ERR("pAudPlayObj=0x%x\r\n", pAudPlayObj);
		return;
	}
	if (pAudPlayObj->isOpened()) {
		DBG_ERR("!!!pAudPlayObj Opened already\r\n");
		return;
	}
	if (E_OK != pAudPlayObj->open()) {
		DBG_ERR("!!!pAudPlayObj Open fail\r\n");
		return;
	}
	Dx_Control(gGxSndDrvSndHdl, DXSOUND_CAP_SET_OUTDEV, gSndOutDevConfigIdx, (UINT32)pAudPlayObj);
	Dx_Control(gGxSndDrvSndHdl, DXSOUND_CAP_SAMPLERATE, gSoundAudSR, (UINT32)pAudPlayObj);
	Dx_Control(gGxSndDrvSndHdl, DXSOUND_CAP_VOLUME, gSndVol, (UINT32)0);

	pAudPlayObj->setConfig(AUDTS_CFG_ID_EVENT_HANDLE, (UINT32)PlaySound_AudioHdl);

	//!!audio driver's tx2 doesn't have pAudPlayObj->setConfig(AUDTS_CFG_ID_TIMECODE_xxx,...) functions
	//aud_setTimecodeOffset(0);
	//pAudPlayObj->setConfig(AUDTS_CFG_ID_TIMECODE_OFFSET, 0);

	pAudPlayObj->resetBufferQueue(0);

	//!!audio driver's tx2 doesn't have pAudPlayObj->setConfig(AUDTS_CFG_ID_TIMECODE_xxx,...) functions
	//aud_setTimecodeTrigger(AudioBufQueue.uiSize >> 1);
	//pAudPlayObj->setConfig(AUDTS_CFG_ID_TIMECODE_TRIGGER, AudioBufQueue.uiSize >> 1);

	pAudPlayObj->addBufferToQueue(0, &AudioBufQueue);

	//aud_playback(FALSE, TRUE);
	pAudPlayObj->playback();
	#else

	HD_RESULT ret;
	HD_AUDIOOUT_DEV_CONFIG audio_cfg_param = {0};
	HD_AUDIOOUT_DRV_CONFIG audio_driver_cfg_param = {0};
	HD_AUDIOOUT_OUT audio_out_out_param = {0};
	HD_AUDIOOUT_VOLUME audio_out_vol = {0};
	UINT32 pa   = gxsound_mem.pa + FRAME_SIZE*2*2;
	UINT32 size = gxsound_mem.size - FRAME_SIZE*2*2;
	UINT32 need_size = 0;
	VENDOR_AUDIOOUT_MEM audio_mem = {0};

	//GxSound_SetSoundData(GxSound_GetSoundDataByID(GxSound_GetSoundDataIdx()));
	//pSoundData = gPlaySoundData;
	if (!pSoundData) {
		DBG_ERR("PlaySound data is NULL\r\n");
		_GxSound_Stop();
		return;
	}
	if (pSoundData->puiData && pSoundData->uiSize) {
		DBG_IND("PlaySound Addr=0x%x,Size=0x%x\r\n", (UINT32)pSoundData->puiData, pSoundData->uiSize);
	} else {
		DBG_ERR("PlaySound Addr=0x%x,Size=0x%x is NULL\r\n", (UINT32)pSoundData->puiData, pSoundData->uiSize);
		_GxSound_Stop();
		return;
	}
	if (g_fpSoundCB) {
		if (gPlaySoundData) {
			g_fpSoundCB(SOUND_CB_START, gPlaySoundData->soundId, 0);
		} else {
			g_fpSoundCB(SOUND_CB_START, 0, 0);
		}
	}
	clr_flg(FLG_ID_SOUND, FLGSOUND_STOPPED);
	clr_flg(FLG_ID_SOUND, FLGSOUND_STOP);

	clr_flg(FLG_ID_DATA, FLGDATA_STOPPED);
	clr_flg(FLG_ID_DATA, FLGDATA_STOP);
	ret = hd_audioout_open(0, HD_AUDIOOUT_1_CTRL, &gxsound_ctrl_id); //open this for device control
	if (ret != HD_OK) {
		return;
	}
	/* set audio out maximum parameters */
	audio_cfg_param.out_max.sample_rate = gSoundAudSR;
	audio_cfg_param.out_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cfg_param.out_max.mode = HD_AUDIO_SOUND_MODE_STEREO;
	audio_cfg_param.frame_sample_max = FRAME_SIZE;
	audio_cfg_param.frame_num_max = 10;
	audio_cfg_param.in_max.sample_rate = 0;
	ret = hd_audioout_set(gxsound_ctrl_id, HD_AUDIOOUT_PARAM_DEV_CONFIG, &audio_cfg_param);
	if (ret != HD_OK) {
		DBG_ERR("hd_audioout_set failed=%x\r\n", ret);
		return;
	}

	ret = vendor_audioout_get(gxsound_ctrl_id, VENDOR_AUDIOOUT_ITEM_NEEDED_BUF, (VOID *)&need_size);
	if (ret != HD_OK) {
		DBG_ERR("vendor_audioout_get failed=%x\r\n", ret);
		return;
	}

	if (size < need_size) {
		DBG_ERR("audioout buffer need %d but %d\r\n", need_size, size);
		return;
	}

	audio_mem.pa = pa;
	audio_mem.size = size;

	ret = vendor_audioout_set(gxsound_ctrl_id, VENDOR_AUDIOOUT_ITEM_ALLOC_BUF, (VOID *)&audio_mem);
	if (ret != HD_OK) {
		DBG_ERR("vendor_audioout_set failed=%x\r\n", ret);
		return;
	}

	/* set audio out driver parameters */
	audio_driver_cfg_param.mono = HD_AUDIO_MONO_LEFT;
	audio_driver_cfg_param.output = (gSndOutDevConfigIdx == 0)? HD_AUDIOOUT_OUTPUT_SPK : HD_AUDIOOUT_OUTPUT_LINE;
	ret = hd_audioout_set(gxsound_ctrl_id, HD_AUDIOOUT_PARAM_DRV_CONFIG, &audio_driver_cfg_param);
	if (ret != HD_OK) {
		return;
	}

	if((ret = hd_audioout_open(HD_AUDIOOUT_1_IN_0, HD_AUDIOOUT_1_OUT_0, &gxsound_path_id)) != HD_OK) {
		return;
	}
	g_uiPlaySoundStatus = PLAYSOUND_STS_PLAYING;

	// set hd_audioout output parameters
	audio_out_out_param.sample_rate = gSoundAudSR;
	audio_out_out_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_out_out_param.mode = (pSoundData->isMono)? HD_AUDIO_SOUND_MODE_MONO : HD_AUDIO_SOUND_MODE_STEREO;
	ret = hd_audioout_set(gxsound_path_id, HD_AUDIOOUT_PARAM_OUT, &audio_out_out_param);
	if (ret != HD_OK) {
		return;
	}

	// set hd_audioout volume
	audio_out_vol.volume = gSndVol;
	ret = hd_audioout_set(gxsound_ctrl_id, HD_AUDIOOUT_PARAM_VOLUME, &audio_out_vol);
	if (ret != HD_OK) {
		return;
	}

	//trigger push data
	{
		play_stop = FALSE;
		set_flg(FLG_ID_DATA, FLGDATA_PLAY);
	}

	hd_audioout_start(gxsound_path_id);

	#endif
}

static BOOL bStopSound = FALSE;
//fix for CID 43232 & 43039 - begin

void PlaySoundQuit(void)
{
	bStopSound = TRUE;
}
//fix for CID 43232 & 43039 - end

THREAD_RETTYPE PlaySoundTsk(void)
{
	FLGPTN uiFlag = 0;

	kent_tsk();
	DBG_IND(":sts=%d\r\n", g_uiPlaySoundStatus);
	bStopSound = FALSE;
	while (!bStopSound) {
		DBG_IND(":0x%x,sts=%d\r\n", uiFlag, g_uiPlaySoundStatus);
		//Connect to TV,timing issue,slideshow playsound stopped flag is cleared here
		//Unplug usb,no power down sound
		PROFILE_TASK_IDLE();
		vos_flag_wait_interruptible(&uiFlag, FLG_ID_SOUND, FLGSOUND_STOP | FLGSOUND_PLAY | FLGSOUND_EXIT, TWF_ORW);
		PROFILE_TASK_BUSY();
		DBG_IND(":F=0x%x,sts=%d\r\n", uiFlag, g_uiPlaySoundStatus);
		if (uiFlag & FLGSOUND_EXIT) {
			break;
		} else if (uiFlag & FLGSOUND_STOP) {
			DBG_IND("PlayTsk stop\r\n");
			_GxSound_Stop();
			clr_flg(FLG_ID_SOUND, FLGSOUND_STOP);
		} else if (uiFlag & FLGSOUND_PLAY) {
			DBG_IND("PlayTsk play\r\n");
			_GxSound_Play(gPlaySoundData);
			clr_flg(FLG_ID_SOUND, FLGSOUND_PLAY);
		}
	}
	DBG_IND(":sts=%d\r\n", g_uiPlaySoundStatus);

	set_flg(FLG_ID_SOUND, FLGSOUND_QUIT);

	THREAD_RETURN(0);
}

static HD_RESULT _gxsound_get_hd_common_buf(HD_COMMON_MEM_VB_BLK *pblk, UINT32 *pPa, UINT32 *pVa, UINT32 blk_size)
{
	#if 0
	// get memory
	*pblk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, DDR_ID0); // Get block from mem pool
	if (*pblk == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_ERR("config_vdo_frm: get blk fail, blk(0x%x)\n", *pblk);
		return HD_ERR_SYS;
	}

	*pPa = hd_common_mem_blk2pa(*pblk); // get physical addr
	if (*pPa == 0) {
		DBG_ERR("config_vdo_frm: blk2pa fail, blk(0x%x)\n", *pblk);
		return HD_ERR_SYS;
	}

	*pVa = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, *pPa, blk_size);
	if (*pVa == 0) {
		DBG_ERR("Convert to VA failed for file buffer for decoded buffer!\r\n");
		return HD_ERR_SYS;
	}
	#else
	*pVa = gxsound_mem.va;
	*pPa = gxsound_mem.pa;
	*pblk = 0;
	#endif

	return HD_OK;
}

static HD_RESULT _gxsound_release_hd_common_buf(HD_COMMON_MEM_VB_BLK *pblk, UINT32 va, UINT32 blk_size)
{
	HD_RESULT ret;

	#if 0
	hd_common_mem_munmap((void*)va, blk_size);
	ret = hd_common_mem_release_block(*pblk);
	#else
	ret = HD_OK;
	#endif

	return ret;
}

static void _gxsound_push_data(void)
{
	UINT32 va = 0, pa = 0;
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 curr_addr;
	UINT32 remain_size;
	UINT32 push_size;
	HD_AUDIO_FRAME  bs_in_buf = {0};
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_RESULT ret;
	UINT32 total_size;
	UINT32 unit_size;

	if (gPlaySoundData->isMono) {
		unit_size = FRAME_SIZE*1*2;
	} else {
		unit_size = FRAME_SIZE*2*2;
	}

	total_size = ALIGN_CEIL(gPlaySoundData->uiSize*gGxSndRepPlayCnt, unit_size);

	if (_gxsound_get_hd_common_buf(&blk, &pa, &va, unit_size) != HD_OK) {
		DBG_ERR("Get common buffer failed\r\n");
		return;
	}

	while (gGxSndRepPlayCnt > 0) {

		if (play_stop) {
			break;
		}

		curr_addr   = (UINT32)gPlaySoundData->puiData;
		remain_size = gPlaySoundData->uiSize;

		while (remain_size > 0) {
			if (play_stop) {
				break;
			}

			push_size = (remain_size > unit_size)? unit_size : remain_size;
			memcpy((void *)va, (const void *)curr_addr, push_size);

			bs_in_buf.sign = MAKEFOURCC('A','F','R','M');
			bs_in_buf.phy_addr[0] = pa; // needs to add offset
			bs_in_buf.size = push_size;
			bs_in_buf.ddr_id = ddr_id;
			bs_in_buf.timestamp = hd_gettime_us();
			bs_in_buf.bit_width = HD_AUDIO_BIT_WIDTH_16;
			bs_in_buf.sound_mode = (gPlaySoundData->isMono)? HD_AUDIO_SOUND_MODE_MONO : HD_AUDIO_SOUND_MODE_STEREO;
			bs_in_buf.sample_rate = gPlaySoundData->sampleRate;

			ret = hd_audioout_push_in_buf(gxsound_path_id, &bs_in_buf, -1);

			if (ret != HD_OK && ret != HD_ERR_NOT_OPEN) {
				DBG_ERR("hd_audioout_push_in_buf fail, ret(%d)\n", ret);
				gGxSndRepPlayCnt = 0;
				break;
			}

			remain_size -= push_size;
			curr_addr   += push_size;
		}

		if (gGxSndRepPlayCnt > 0) {
			gGxSndRepPlayCnt--;
		}
	}


	if (!play_stop) {
		UINT32 size;

		do {
			vendor_audioout_get(gxsound_ctrl_id, VENDOR_AUDIOOUT_ITEM_DONE_SIZE, (VOID *)&size);
		} while (total_size > size && (!play_stop));
	}

	set_flg(FLG_ID_SOUND, FLGSOUND_STOP);

	if (_gxsound_release_hd_common_buf(&blk, va, unit_size) != HD_OK) {
		DBG_ERR("Release common buffer failed\r\n");
		return;
	}
}

THREAD_RETTYPE PlayDataTsk(void)
{
	FLGPTN uiFlag = 0;

	kent_tsk();
	DBG_IND(":data sts=%d\r\n", g_uiPlaySoundStatus);

	while (!bStopSound) {
		DBG_IND("data:0x%x,sts=%d\r\n", uiFlag, g_uiPlaySoundStatus);
		//Connect to TV,timing issue,slideshow playsound stopped flag is cleared here
		//Unplug usb,no power down sound
		PROFILE_TASK_IDLE();
		vos_flag_wait_interruptible(&uiFlag, FLG_ID_DATA, FLGDATA_STOP | FLGDATA_PLAY | FLGDATA_EXIT, TWF_ORW);
		PROFILE_TASK_BUSY();
		DBG_IND("data:F=0x%x,sts=%d\r\n", uiFlag, g_uiPlaySoundStatus);
		if (uiFlag & FLGDATA_EXIT) {
			break;
		} if (uiFlag & FLGDATA_STOP) {
			clr_flg(FLG_ID_DATA, FLGDATA_STOP);
		} else if (uiFlag & FLGDATA_PLAY) {
			DBG_IND("trigger push\r\n");
			_gxsound_push_data();
			clr_flg(FLG_ID_DATA, FLGDATA_PLAY);
		}
	}
	DBG_IND(":sts=%d\r\n", g_uiPlaySoundStatus);

	set_flg(FLG_ID_DATA, FLGDATA_QUIT);

	THREAD_RETURN(0);
}

//@}
