/*
    NvtAec.

    This file is the task of Audio Echo Cancellation.

    @file       NvtAec.c
    @ingroup    mIAPPNVTAEC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#if defined __UITRON || defined __ECOS
#include "nvtaec_int.h"
#include "nvtaec_api.h"
#include "Aec.h"
#include "timer.h"
#include "NvtVerInfo.h"
#else
#include "nvtaec_int.h"
#include "../include/nvtaec_api.h"
#include "kdrv_audioio/audlib_aec.h"
#endif

///////////////////////////////////////////////////////////////////////////////
#define THIS_DBGLVL         NVT_DBG_WRN
#define __MODULE__          nvtaec
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#include "kwrap/debug.h"

unsigned int nvtaec_debug_level = THIS_DBGLVL;
///////////////////////////////////////////////////////////////////////////////

/*
    @name flags for AudStrmGetTsk

    flags for AudStrmGetTsk.
*/
//@{
#define FLGNVTAEC_IDLE  FLGPTN_BIT(0)
#define FLGNVTAEC_BUSY  FLGPTN_BIT(1)
#define FLGNVTAEC_DONE  FLGPTN_BIT(2)
//@}

#define USE_IM2 DISABLE
#define NVTAEC_QCONSTANT16(x) ((INT32)((500+((x)*(((INT32)1)<<(15))))/1000))


/**
    @addtogroup mIAPPNVTAEC
*/
//@{

BOOL                gAECisInit                              = FALSE;
BOOL                gbAECEnable                             = FALSE;
BOOL                gbAecIsOpen                             = FALSE;
BOOL                gbAecIsStart                            = FALSE;
UINT32              guiAecSampleRate                        = 8000;
NVTAEC_INFO         gNvtAecInfo                             = {
	.iECLvl             = -60,
	.iNRLvl             = -30,
	.bLeakEstiEn        = 0,
	.iLeakEstiVal       = 99,
	.iMicCH             = 1,
	.iSpkCH             = 1,
	.iSampleRate        = AUDIO_SR_16000,
	.iFilterLen         = 1024,
	.iFrameSize         = 128,
	.iNotchRadius       = 992,
	.iPreLoadEn         = 0,
	.uiForeAddr         = 0,
	.uiBackAddr         = 0,
	.uiForeSize         = 0,
	.uiBackSize         = 0,
	.uiARModuleEn       = 0,
	.uiInternalAddr     = 0,
	.uiInternalSize     = 0,
	.iSpkNum            = 1
};

static KDRV_AUDIOLIB_FUNC* aec_obj = NULL;

//_ALIGNED(64) UINT8  aecforebuf[AEC_PRELOAD_FORE_BUFSIZE]    = {0};
//_ALIGNED(64) UINT8  aecbackbuf[AEC_PRELOAD_BACK_BUFSIZE]    = {0};

#if FILEWRITE
#include        "FileSysTsk.h"
#define         FILESIZE (400000*3)
#define         FILESTARTOFFSET 0//16000*2*5//(16000*5)
FST_FILE        pFileHdl1;
FST_FILE        pFileHdl2;
FST_FILE        pFileHdl3;
UINT8           namer = 0;
char            Filer[14] = {'A', ':', '\\', 'A', 'E', 'C', 'R', '0', '0', '.', 'P', 'C', 'M', 0};
UINT8           namep = 0;
char            Filep[14] = {'A', ':', '\\', 'A', 'E', 'C', 'P', '0', '0', '.', 'P', 'C', 'M', 0};
UINT8           namea = 0;
char            Filea[14] = {'A', ':', '\\', 'A', 'E', 'C', 'A', '0', '0', '.', 'P', 'C', 'M', 0};
UINT8           tmpbuf[FILESIZE] = {0};
UINT32          tmpAddr = 0;
UINT8           tmpbufR[FILESIZE] = {0};
UINT32          tmpAddrR = 0;
UINT8           tmpbufAEC[FILESIZE] = {0};
UINT32          tmpAddrAEC = 0;
UINT8           isFull = 0;
UINT32          AECDelay = 0;
BOOL            bFileOpened = FALSE;
#endif

#if defined __UITRON || defined __ECOS
NVTVER_VERSION_ENTRY(NvtAec, 1, 00, 003, 00)
#endif

static BOOL NvtAec_InitParm(BOOL init)
{
	if (aec_obj == NULL) {
		DBG_ERR("aec obj is NULL\r\n");
		return FALSE;
	}

	if (aec_obj->aec.set_config == NULL) {
		DBG_ERR("set config is NULL\r\n");
		return FALSE;
	}

	if (aec_obj->aec.init == NULL) {
		DBG_ERR("init is NULL\r\n");
		return FALSE;
	}

	if (!gAECisInit) {
#if USE_IM2

#else
		aec_obj->aec.set_config(AEC_CONFIG_ID_LEAK_ESTIMTAE_EN, gNvtAecInfo.bLeakEstiEn);
		aec_obj->aec.set_config(AEC_CONFIG_ID_LEAK_ESTIMTAE_VAL, NVTAEC_QCONSTANT16(gNvtAecInfo.iLeakEstiVal*10));

		aec_obj->aec.set_config(AEC_CONFIG_ID_NOISE_CANCEL_LVL, gNvtAecInfo.iNRLvl);  //Defualt is -20dB. Suggest value range -3 ~ -40. Unit in dB.
		aec_obj->aec.set_config(AEC_CONFIG_ID_ECHO_CANCEL_LVL, gNvtAecInfo.iECLvl);   //Defualt is -50dB. Suggest value range -30 ~ -60. Unit in dB.

		aec_obj->aec.set_config(AEC_CONFIG_ID_SAMPLERATE, gNvtAecInfo.iSampleRate);
		aec_obj->aec.set_config(AEC_CONFIG_ID_RECORD_CH_NO, gNvtAecInfo.iMicCH);
		aec_obj->aec.set_config(AEC_CONFIG_ID_PLAYBACK_CH_NO, gNvtAecInfo.iSpkCH);
		aec_obj->aec.set_config(AEC_CONFIG_ID_SPK_NUMBER, gNvtAecInfo.iSpkNum);

		aec_obj->aec.set_config(AEC_CONFIG_ID_FILTER_LEN, gNvtAecInfo.iFilterLen);
		aec_obj->aec.set_config(AEC_CONFIG_ID_FRAME_SIZE, gNvtAecInfo.iFrameSize);

		aec_obj->aec.set_config(AEC_CONFIG_ID_NOTCH_RADIUS, NVTAEC_QCONSTANT16(gNvtAecInfo.iNotchRadius));

		aec_obj->aec.set_config(AEC_CONFIG_ID_PRELOAD_EN, gNvtAecInfo.iPreLoadEn);

		if (init) {
			aec_obj->aec.set_config(AEC_CONFIG_ID_FOREADDR, (INT32)gNvtAecInfo.uiForeAddr);
			aec_obj->aec.set_config(AEC_CONFIG_ID_FORESIZE, (INT32)gNvtAecInfo.uiForeSize);

			aec_obj->aec.set_config(AEC_CONFIG_ID_BACKADDR, (INT32)gNvtAecInfo.uiBackAddr);
			aec_obj->aec.set_config(AEC_CONFIG_ID_BACKSIZE, (INT32)gNvtAecInfo.uiBackSize);

			aec_obj->aec.set_config(AEC_CONFIG_ID_BUF_ADDR, (INT32)gNvtAecInfo.uiInternalAddr);
			aec_obj->aec.set_config(AEC_CONFIG_ID_BUF_SIZE, (INT32)gNvtAecInfo.uiInternalSize);

			if (!aec_obj->aec.init()) {
				DBG_ERR("AEC init failed\r\n");
			}
#endif

#if FILEWRITE
			if (!bFileOpened) {
				Filep[8] = (char)(namep % 10) + 0x30;
				Filep[7] = (char)((namep / 10) % 10) + 0x30;

				namep = (namep + 1) % 100;

				Filer[8] = (char)(namer % 10) + 0x30;
				Filer[7] = (char)((namer / 10) % 10) + 0x30;

				namer = (namer + 1) % 100;

				Filea[8] = (char)(namea % 10) + 0x30;
				Filea[7] = (char)((namea / 10) % 10) + 0x30;

				namea = (namea + 1) % 100;
				tmpAddr = 0;
				tmpAddrR = 0;
				tmpAddrAEC = 0;
				DBG_IND("%s\r\n", Filep);
				bFileOpened = TRUE;
			}
#endif
			gAECisInit = TRUE;
		}
	}
	return TRUE;
}

/*
    Open NvtAec library.

    Open NvtAec library.

    @return
     - @b E_OK:     Initial successfully.
     - @b E_OACV:   Open AEC failed.
     - @b E_SYS:    Initial AEC failed.
*/
ER nvtaec_open(void)
{
	ER retV = E_OK;

	/*if (FALSE == NvtAec_ChkModInstalled()) {
		DBG_ERR("module Not installed\r\n");
		return E_SYS;
	}*/

	if (gbAecIsOpen) {
		DBG_ERR("NvtAec is already opened\r\n");
		return E_SYS;
	}

	if (aec_obj == NULL) {
		aec_obj = kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID_AEC);
	}

	if (aec_obj == NULL) {
		DBG_ERR("aec obj is NULL\r\n");
		return E_NOEXS;
	}

	if (aec_obj->aec.open == NULL) {
		DBG_ERR("open is NULL\r\n");
		return E_NOEXS;
	}

	gAECisInit         = FALSE;
	//wai_sem(SEM_ID_NVTAEC_EN);
	//gbAECEnable        = TRUE;
	//sig_sem(SEM_ID_NVTAEC_EN);

#if USE_IM2

#else
	retV = aec_obj->aec.open();
	if (E_OK != retV) {
		DBG_ERR("AEC open failed\r\n");
		return retV;
	}
#endif

	gbAecIsOpen = TRUE;

	//xNvtAec_InstallCmd();

	//set_flg(FLG_ID_NVTAEC, FLGNVTAEC_IDLE);

	return E_OK;
}

/*
    Close NvtAec library.

    Close NvtAec library.

    @return
     - @b E_OK:     Close successfully.
     - @b E_SYS:    AEC is not opened.
*/
ER nvtaec_close(void)
{
	//FLGPTN uiFlag;

	if (!gbAecIsOpen) {
		DBG_ERR("NvtAec is not opened\r\n");
		return E_SYS;
	}

	if (aec_obj == NULL) {
		DBG_ERR("aec obj is NULL\r\n");
		return E_NOEXS;
	}

	if (aec_obj->aec.close == NULL) {
		DBG_ERR("open is NULL\r\n");
		return E_NOEXS;
	}

	//wai_flg(&uiFlag, FLG_ID_NVTAEC, FLGNVTAEC_IDLE, TWF_ORW);

	aec_obj->aec.close();
	gbAecIsOpen = FALSE;
	gbAecIsStart = FALSE;

	return E_OK;
}

/*
    Start NvtAec.

    Start NvtAec when record task is started.

    @param[in] iSampleRate  audio sample rate.
    @param[in] iMicCH       microphone channel number.
    @param[in] iSpkCH       speaker channel number.

    @return
     - @b E_OK:     Start successfully.
     - @b E_SYS:    Start failed.
*/
ER nvtaec_start(INT32 iSampleRate, INT32 iMicCH, INT32 iSpkCH)
{
	//FLGPTN uiFlag;

	if (!gbAecIsOpen) {
		DBG_ERR("NvtAec is not opened\r\n");
		return E_SYS;
	}

	if (gbAecIsStart) {
		DBG_ERR("NvtAec is already started\r\n");
		return E_SYS;
	}

	//wai_flg(&uiFlag, FLG_ID_NVTAEC, FLGNVTAEC_IDLE, TWF_ORW | TWF_CLR);

	gNvtAecInfo.iSampleRate = iSampleRate;
	gNvtAecInfo.iMicCH = iMicCH;
	gNvtAecInfo.iSpkCH = iSpkCH;

	//calculate buffer shift between play and record
	NvtAec_InitParm(TRUE);

	gbAecIsStart = TRUE;

	//set_flg(FLG_ID_NVTAEC, FLGNVTAEC_IDLE);

	return E_OK;
}

/*
    Stop NvtAec.

    Stop NvtAec when record task is stopped.

    @return
     - @b E_OK:     Stop successfully.
*/
ER nvtaec_stop(void)
{
	//FLGPTN uiFlag;

	//wai_flg(&uiFlag, FLG_ID_NVTAEC, FLGNVTAEC_IDLE, TWF_ORW | TWF_CLR);

	gAECisInit   = FALSE;
	gbAecIsStart = FALSE;

#if FILEWRITE
	if (bFileOpened) {
		pFileHdl1 = FileSys_OpenFile(Filep, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		pFileHdl2 = FileSys_OpenFile(Filer, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		pFileHdl3 = FileSys_OpenFile(Filea, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		FileSys_WriteFile(pFileHdl1, tmpbuf, &tmpAddr, 0, NULL);
		FileSys_WriteFile(pFileHdl2, tmpbufR, &tmpAddr, 0, NULL);
		FileSys_WriteFile(pFileHdl3, tmpbufAEC, &tmpAddr, 0, NULL);
		FileSys_CloseFile(pFileHdl1);
		FileSys_CloseFile(pFileHdl2);
		FileSys_CloseFile(pFileHdl3);
		memset((void *)tmpbuf, 0, sizeof(tmpbuf));
		memset((void *)tmpbufR, 0, sizeof(tmpbufR));
		memset((void *)tmpbufAEC, 0, sizeof(tmpbufAEC));
		isFull = 0;
		AECDelay = 0;
		bFileOpened = FALSE;
	}
#endif

	//set_flg(FLG_ID_NVTAEC, FLGNVTAEC_IDLE);



	return E_OK;
}

/*
    Run AEC.

    Run AEC.

    @param[in] addrRec      recorded audio buffer address.
    @param[in] addrPlay     played audio buffer address.
    @param[in] addrOut      out audio buffer address.
    @param[in] size         recorded audio buffer size.

    @return
     - @b E_OK:     AEC done successfully.
     - @b E_SYS:    AEC done failed.
*/
ER nvtaec_apply(UINT32 addrRec, UINT32 addrPlay, UINT32 addrOut, UINT32 size)
{
	//FLGPTN uiFlag;
	AEC_BITSTREAM AecIO = {0};

#if USE_IM2

#else

	if (!gbAecIsOpen) {
		DBG_ERR("NvtAec is not opened\r\n");
		return E_SYS;
	}

	if (aec_obj == NULL) {
		DBG_ERR("aec obj is NULL\r\n");
		return E_NOEXS;
	}

	if (aec_obj->aec.run == NULL) {
		DBG_ERR("run is NULL\r\n");
		return E_NOEXS;
	}

	//wai_flg(&uiFlag, FLG_ID_NVTAEC, FLGNVTAEC_IDLE, TWF_ORW | TWF_CLR);

	if (!gbAECEnable) {
		//set_flg(FLG_ID_NVTAEC, FLGNVTAEC_IDLE);
		#if FILEWRITE
			UINT32 bufSize = size;
			//File write
			if (AECDelay >= FILESTARTOFFSET) {
				if ((tmpAddr + bufSize) <= FILESIZE) {
					memcpy((void *)tmpbuf + tmpAddr, (void *)addrPlay, bufSize);
					tmpAddr += bufSize;
				} else if (isFull == 0) {
					DBG_DUMP("Buffer Full\r\n");
					isFull = 1;
				}
				if ((tmpAddrR + bufSize) <= FILESIZE) {
					memcpy((void *)tmpbufR + tmpAddrR, (void *)addrRec, bufSize);
					tmpAddrR += bufSize;
				}
				if ((tmpAddrAEC + bufSize) <= FILESIZE) {
					memcpy((void *)tmpbufAEC + tmpAddrAEC, (void *)addrOut, bufSize);
					tmpAddrAEC += bufSize;
				}

			} else {
				DBG_DUMP("Delay\r\n");
				AECDelay += bufSize >> 1;
			}
		#endif



		return E_OK;
	}

#endif

#if FILEWRITE
	UINT32 bufSize = size;
	//File write
	if (AECDelay >= FILESTARTOFFSET) {


		if ((tmpAddr + bufSize) <= FILESIZE) {
			memcpy((void *)tmpbuf + tmpAddr, (void *)addrPlay, bufSize);
			tmpAddr += bufSize;
		} else if (isFull == 0) {
			DBG_DUMP("Buffer Full\r\n");
			isFull = 1;
		}
		if ((tmpAddrR + bufSize) <= FILESIZE) {
			memcpy((void *)tmpbufR + tmpAddrR, (void *)addrRec, bufSize);
			tmpAddrR += bufSize;
		}
	} else {
		DBG_DUMP("Delay\r\n");
	}
#endif

#if USE_IM2


#else

	//configure AEC buffer

	//dma_flushReadCache(dma_getCacheAddr(addrPlay), size);
	//dma_flushReadCache(dma_getCacheAddr(addrRec), size);

	AecIO.bitstream_buffer_play_in = (UINT32)addrPlay;
	AecIO.bitstream_buffer_record_in = (UINT32)addrRec;
	AecIO.bitstram_buffer_out = (UINT32)addrOut;
	AecIO.bitstram_buffer_length = size >> 1;
	if (gNvtAecInfo.iMicCH == 2) {
		AecIO.bitstram_buffer_length = AecIO.bitstram_buffer_length >> 1;
	}

	if (!aec_obj->aec.run(&AecIO)) {
		DBG_ERR("AEC FAIL!\r\n");
		//set_flg(FLG_ID_NVTAEC, FLGNVTAEC_IDLE);
		return E_SYS;
	}

	//memcpy((void *)addrOut, (void *)aecOutBuf, size);

#endif

#if FILEWRITE
	if (AECDelay >= FILESTARTOFFSET) {
		if ((tmpAddrAEC + bufSize) <= FILESIZE) {
			memcpy((void *)tmpbufAEC + tmpAddrAEC, (void *)addrOut, bufSize);
			tmpAddrAEC += bufSize;
		}
	} else {
		AECDelay += bufSize >> 1;
	}
#endif

	//set_flg(FLG_ID_NVTAEC, FLGNVTAEC_IDLE);

	return E_OK;
}

/*
    Set NvtAec configuration.

    Set NvtAec configuration.

    @param[in]  NvtAecSel       Configuration type.
    @param[in]  iNvtAecConfig   Configuration value. (According to the confiuration type)
*/
void nvtaec_setconfig(NVTAEC_CONFIG_ID NvtAecSel, INT32 iNvtAecConfig)
{
	switch (NvtAecSel) {
	case NVTAEC_CONFIG_ID_NOISE_CANCEL_LVL: {
			gNvtAecInfo.iNRLvl = iNvtAecConfig;
		}
		break;
	case NVTAEC_CONFIG_ID_ECHO_CANCEL_LVL: {
			gNvtAecInfo.iECLvl = iNvtAecConfig;
		}
		break;

	case NVTAEC_CONFIG_ID_LEAK_ESTIMATE_EN: {
			gNvtAecInfo.bLeakEstiEn = iNvtAecConfig;
		}
		break;
	case NVTAEC_CONFIG_ID_LEAK_ESTIMATE_VAL: {
			gNvtAecInfo.iLeakEstiVal = iNvtAecConfig;
		}
		break;

	case NVTAEC_CONFIG_ID_FILTER_LEN: {
			gNvtAecInfo.iFilterLen = iNvtAecConfig;
		}
		break;
	case NVTAEC_CONFIG_ID_FRAME_SIZE: {
			gNvtAecInfo.iFrameSize = iNvtAecConfig;
		}
		break;
	case NVTAEC_CONFIG_ID_NOTCH_RADIUS: {
        	gNvtAecInfo.iNotchRadius= iNvtAecConfig;
    	}
    	break;
	case NVTAEC_CONFIG_ID_RECORD_CH_NO: {
			gNvtAecInfo.iMicCH = iNvtAecConfig;
		}
		break;
	case NVTAEC_CONFIG_ID_PLAYBACK_CH_NO: {
			gNvtAecInfo.iSpkCH = iNvtAecConfig;
		}
		break;
	case NVTAEC_CONFIG_ID_SAMPLERATE: {
			gNvtAecInfo.iSampleRate = iNvtAecConfig;
		}
		break;
	case NVTAEC_CONFIG_ID_SPK_NUMBER: {
			gNvtAecInfo.iSpkNum = iNvtAecConfig;
		}
	default:
		break;
	}

}

/*
    Get NvtAec configuration.

    Get NvtAec configuration.

    @param[in]  NvtAecSel       Configuration type.
*/
INT32 nvtaec_getconfig(NVTAEC_CONFIG_ID NvtAecSel)
{
	INT32 ret = 0;

	switch (NvtAecSel) {
	case NVTAEC_CONFIG_ID_NOISE_CANCEL_LVL: {
			ret = gNvtAecInfo.iNRLvl;
		}
		break;
	case NVTAEC_CONFIG_ID_ECHO_CANCEL_LVL: {
			ret = gNvtAecInfo.iECLvl;
		}
		break;

	case NVTAEC_CONFIG_ID_LEAK_ESTIMATE_EN: {
			ret = (INT32)gNvtAecInfo.bLeakEstiEn;
		}
		break;
	case NVTAEC_CONFIG_ID_LEAK_ESTIMATE_VAL: {
			ret = gNvtAecInfo.iLeakEstiVal;
		}
		break;

	case NVTAEC_CONFIG_ID_FILTER_LEN: {
			ret = gNvtAecInfo.iFilterLen;
		}
		break;
	case NVTAEC_CONFIG_ID_FRAME_SIZE: {
			ret = gNvtAecInfo.iFrameSize;
		}
		break;
	case NVTAEC_CONFIG_ID_NOTCH_RADIUS: {
        	ret = gNvtAecInfo.iNotchRadius;
    	}
    	break;
	case NVTAEC_CONFIG_ID_SPK_NUMBER: {
			ret = gNvtAecInfo.iSpkNum;
		}
	default:
		break;
	}

	return ret;
}


/*
    Enable NvtAec processing.

    Enable NvtAec processing.

    @param[in]  bEn       Eable configuration. (TRUE: enable, FALSE: disable)
*/
void nvtaec_enable(BOOL bEn)
{
	//wai_sem(SEM_ID_NVTAEC_EN);
	gbAECEnable = bEn;
	//sig_sem(SEM_ID_NVTAEC_EN);
	DBG_IND("NvtAec=%s\r\n", gbAECEnable ? "ENABLE" : "DISABLE");
}

UINT32 nvtaec_get_require_bufsize(UINT32 sample_rate)
{
	//INT32 aec_size = 0;

	if (aec_obj == NULL) {
		aec_obj = kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID_AEC);
	}

	if (aec_obj == NULL) {
		DBG_ERR("aec obj is NULL\r\n");
		return 0;
	}

	if (aec_obj->aec.open == NULL) {
		DBG_ERR("open is NULL\r\n");
		return 0;
	}

	if (aec_obj->aec.close == NULL) {
		DBG_ERR("close is NULL\r\n");
		return 0;
	}

	if (!gbAecIsOpen) {
		aec_obj->aec.open();
	}

	//gNvtAecInfo.iMicCH = (INT32)channel_num;
	//gNvtAecInfo.iSpkCH = (INT32)channel_num;
	gNvtAecInfo.iSampleRate = (INT32)sample_rate;

	NvtAec_InitParm(FALSE);

	gNvtAecInfo.uiInternalSize = ALIGN_CEIL_64(aec_obj->aec.get_required_buffer_size(AEC_BUFINFO_ID_INTERNAL));
	gNvtAecInfo.uiForeSize     = ALIGN_CEIL_64(aec_obj->aec.get_required_buffer_size(AEC_BUFINFO_ID_FORESIZE));
	gNvtAecInfo.uiBackSize     = ALIGN_CEIL_64(aec_obj->aec.get_required_buffer_size(AEC_BUFINFO_ID_BACKSIZE));

	if (!gbAecIsOpen) {
		aec_obj->aec.close();
	}

	return gNvtAecInfo.uiInternalSize + gNvtAecInfo.uiForeSize + gNvtAecInfo.uiBackSize;
}

ER nvtaec_set_require_buf(UINT32 addr, UINT32 size)
{
	if (gNvtAecInfo.uiInternalSize + gNvtAecInfo.uiForeSize + gNvtAecInfo.uiBackSize > size) {
		DBG_ERR("AEC buffer needs %x, but %x\r\n", gNvtAecInfo.uiInternalSize + gNvtAecInfo.uiForeSize + gNvtAecInfo.uiBackSize, size);
		return E_NOMEM;
	}

	gNvtAecInfo.uiInternalAddr = addr;
	DBG_IND("Internal addr=0x%x, size=0x%x\r\n", gNvtAecInfo.uiInternalAddr, gNvtAecInfo.uiInternalSize);

	gNvtAecInfo.uiForeAddr = gNvtAecInfo.uiInternalAddr + gNvtAecInfo.uiInternalSize;
	DBG_IND("Fore addr    =0x%x, size=0x%x\r\n", gNvtAecInfo.uiForeAddr, gNvtAecInfo.uiForeSize);

	gNvtAecInfo.uiBackAddr = gNvtAecInfo.uiForeAddr + gNvtAecInfo.uiForeSize;
	DBG_IND("Back addr    =0x%x, size=0x%x\r\n", gNvtAecInfo.uiBackAddr, gNvtAecInfo.uiBackSize);

	return E_OK;
}
//@}


