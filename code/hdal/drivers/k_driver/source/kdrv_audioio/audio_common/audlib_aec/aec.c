/*
    Audio Echo Cancellation library

    This file is the driver of Audio Echo Cancellation library.

    @file       Aec.c
    @ingroup    mIAudEC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/

#ifdef __KERNEL__
#include "kdrv_audioio/audlib_aec.h"
#include "aec_private.h"
#include "aec_ns_private.h"
#include "aec_int.h"
#include "audlib_aec_dbg.h"
#include "kwrap/type.h"
#include <mach/rcw_macro.h>
#else
#define __MODULE__ rtos_aec
#define __DBGLVL__ 8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__ "*"
#include <string.h>
#include "kdrv_audioio/audlib_aec.h"
#include "kdrv_audioio/kdrv_audioio.h"
#include "aec_private.h"
#include "aec_ns_private.h"
#include "aec_int.h"
#include "kwrap/debug.h"
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "Audio.h"
#include "dma_protected.h"
#include "cache_protected.h"
unsigned int rtos_aec_debug_level = NVT_DBG_WRN;

#endif




/**
    @addtogroup mIAudEC
*/
//@{




static BOOL     bAecOpened = FALSE;
static BOOL     bAecInit   = FALSE;
#if AEC_ALIGN_BUF_EN
static UINT8    gcAecRxAlignBuf[AEC_ALIGN_BUFSIZE];
static UINT8    gcAecTxAlignBuf[AEC_ALIGN_BUFSIZE];
#endif
static AEC_INFO gAecInfo;

#if AEC_BUF_FIXED_MAP
UINT8           gcAecInBuf[AEC_INTERNAL_BUFSIZE];
INT32 			giAecBufSize = AEC_INTERNAL_BUFSIZE;
#else
UINT8           *gcAecInBuf;
INT32 			giAecBufSize;
#endif

static BOOL     gAecLeakEstiDisable = FALSE;//FALSE is enable leak estimate
static INT32    gAecLeakEstiVal= AEC_QCONSTANT16(0.99);
static INT32    gAecNotchRadius = AEC_QCONSTANT16(0.992);
static BOOL     gAecARModuleEn  = TRUE;//TRUE is enable Amplitude Rate Module

static BOOL         bAudNSOpened = FALSE;
static BOOL         bAudNSInit   = FALSE;
static AUDNS_INFO   gAudNSInfo;

static INT32	gAecDedicateFilterLen = 0;
static INT32	gAecDedicateFrameSize = 0;

static KDRV_AUDIOLIB_FUNC aec_func =
{
	.aec.open                     = audlib_aec_open,
	.aec.is_opened                = audlib_aec_is_opened,
	.aec.close                    = audlib_aec_close,
	.aec.set_config               = audlib_aec_set_config,
	.aec.init                     = audlib_aec_init,
	.aec.run                      = audlib_aec_run,
	.aec.get_config               = audlib_aec_get_config,
	.aec.get_required_buffer_size = audlib_aec_get_required_buffer_size,
};


#if 1
/**
    Open the Audio Echo Cancellation(AEC) library.

    This API must be called to initialize the Audio Echo Cancellation(AEC) basic settings.

    @return
     - @b E_OK:   Open Done and success.
     - @b E_OACV: aublib_AEC.a Version error. Please replace the aublib_AEC.a to correct version.
*/
ER audlib_aec_open(void)
{
    if(bAecOpened)
        return E_OK;

    if(AUD_AEC_GetVersion() != AEC_MSW_VERSION_CODE)
    {
        DBG_ERR("aublib_AEC.a Version error. Should be 0x%04X But 0x%04X\r\n",AEC_MSW_VERSION_CODE,AUD_AEC_GetVersion());
        return E_OACV;
    }

    if(audlib_ns_is_opened())
    {
        DBG_ERR("AEC-Lib and AudNS-Lib can not be used together\r\n");
        return E_OACV;
    }

    bAecOpened = TRUE;
    bAecInit = FALSE;

    memset((void *)&gAecInfo, 0x00, sizeof(AEC_INFO));

    /* AEC Lib default parameters */
    gAecInfo.iSpkrNo        =   1;
    gAecInfo.iNREn          =   1;
    gAecInfo.iNRLvl         = -20;
    gAecInfo.iECLvl         = -50;
    gAecInfo.bAutoCacheable =   1;
    gAecInfo.iNLPEn         =   0;
    gAecInfo.iPreLoadEn     =   0;

    gAecInfo.uiForeSize     = AEC_PRELOAD_FORE_BUFSIZE;
    gAecInfo.uiBackSize     = AEC_PRELOAD_BACK_BUFSIZE;


    //gAecInfo.uiAlignRx      =   0;
    //gAecInfo.uiAlignTx      =   0;
    //gAecInfo.DbgPrint       =   0;

    return E_OK;
}

/**
    Check if the udio Echo Cancellation(AEC) library is opened or not.

    Check if the udio Echo Cancellation(AEC) library is opened or not.

    @return
     - @b TRUE:  Already Opened.
     - @b FALSE: Have not opened.
*/
BOOL     audlib_aec_is_opened(void)
{
    return bAecOpened;
}

/**
    Close the Audio Echo Cancellation(AEC) library.

    Close the Audio Echo Cancellation(AEC) library.
    After closing the Audio Echo Cancellation(AEC) library,
    any accessing for the Audio Echo Cancellation(AEC) APIs are forbidden except audlib_aec_open().

    @return
     - @b E_OK:  Close Done and success.
*/
ER audlib_aec_close(void)
{
    if(!bAecOpened)
        return E_OK;

    bAecOpened = FALSE;
    return E_OK;
}


/**
    Get specified Audio Echo Cancellation(AEC) configurations.

    This API is used to get the specified Audio Echo Cancellation(AEC) configurations.

    @param[in] aec_sel      Select which of the AEC options is selected


*/

UINT32 audlib_aec_get_config(AEC_CONFIG_ID aec_sel)
{
    UINT32 Ret = 0;
    if(!bAecOpened)
    {
        DBG_ERR("No Opened.\r\n");
        return FALSE;
    }

    bAecInit = FALSE;


    switch (aec_sel)
    {
        case AEC_CONFIG_ID_FOREADDR:
            {
                Ret = gAecInfo.uiForeAddr;
            }break;
        case AEC_CONFIG_ID_BACKADDR:
            {
                Ret = gAecInfo.uiBackAddr;
            }break;
        case AEC_CONFIG_ID_FORESIZE:
            {
                Ret = gAecInfo.uiForeSize;
            }break;
        case AEC_CONFIG_ID_BACKSIZE:
            {
                Ret = gAecInfo.uiBackSize;
            }break;

        default:
            break;
    }
    return Ret;

}

/**
    Set specified Audio Echo Cancellation(AEC) configurations.

    This API is used to set the specified Audio Echo Cancellation(AEC) configurations.

    @param[in] aec_sel      Select which of the AEC options is selected for configuring.
    @param[in] aec_cfg_value  The configurations of the specified AEC option.

    @return
     - @b TRUE:  SetConfig Done and success.
     - @b FALSE: SetConfig Operation fail.
*/
void audlib_aec_set_config(AEC_CONFIG_ID aec_sel, INT32 aec_cfg_value)
{
    if(!bAecOpened)
    {
        DBG_ERR("No Opened.\r\n");
        return;
    }

    bAecInit = FALSE;

    switch (aec_sel)
    {
    case AEC_CONFIG_ID_SAMPLERATE:
    {
        gAecInfo.iSampleRate = aec_cfg_value;
    }
    break;
    case AEC_CONFIG_ID_RECORD_CH_NO:
    {
        gAecInfo.iMicCH = aec_cfg_value;
    }
    break;
    case AEC_CONFIG_ID_PLAYBACK_CH_NO:
    {
        gAecInfo.iSpkrCH = aec_cfg_value;
    }
    break;

    case AEC_CONFIG_ID_NOISE_CANEL_EN:
    {
        gAecInfo.iNREn = (aec_cfg_value>0);
    }
    break;
    case AEC_CONFIG_ID_NOISE_CANCEL_LVL:
    {
        gAecInfo.iNRLvl = aec_cfg_value;
    }
    break;
    case AEC_CONFIG_ID_ECHO_CANCEL_LVL:
    {
        gAecInfo.iECLvl = aec_cfg_value;
    }
    break;
	case AEC_CONFIG_ID_NONLINEAR_PROCESS_EN:
    {
			gAecInfo.iNLPEn = (aec_cfg_value > 0);
    }
	break;

	case AEC_CONFIG_ID_AR_MODULE_EN:
    {
			gAecARModuleEn= aec_cfg_value;
    }
	break;

	case AEC_CONFIG_ID_PRELOAD_EN:
    {
			gAecInfo.iPreLoadEn = (aec_cfg_value > 0);
    }
	break;

    case AEC_CONFIG_ID_RECORD_ALIGN:
    {
        gAecInfo.uiAlignRx = aec_cfg_value;
    }
    break;
    case AEC_CONFIG_ID_PLAYBACK_ALIGN:
    {
        gAecInfo.uiAlignTx = aec_cfg_value;
    }
    break;

    case AEC_CONFIG_ID_LEAK_ESTIMTAE_EN:
    {
        gAecLeakEstiDisable = !aec_cfg_value;
    }
    break;
    case AEC_CONFIG_ID_LEAK_ESTIMTAE_VAL:
    {
        gAecLeakEstiVal = aec_cfg_value;
    }
    break;

	case AEC_CONFIG_ID_FILTER_LEN:
	{
		gAecDedicateFilterLen = aec_cfg_value;
	}
	break;
	case AEC_CONFIG_ID_FRAME_SIZE:
	{
		gAecDedicateFrameSize = aec_cfg_value;
	}
	break;

	case AEC_CONFIG_ID_FOREADDR:
	{
		gAecInfo.uiForeAddr = aec_cfg_value;
	}
	break;

	case AEC_CONFIG_ID_BACKADDR:
	{
		gAecInfo.uiBackAddr = aec_cfg_value;
	}
	break;

	case AEC_CONFIG_ID_FORESIZE:
	{
		gAecInfo.uiForeSize = aec_cfg_value;
	}
	break;

	case AEC_CONFIG_ID_BACKSIZE:
	{
		gAecInfo.uiBackSize= aec_cfg_value;
	}
	break;

    case AEC_CONFIG_ID_NOTCH_RADIUS: {
            gAecNotchRadius       = aec_cfg_value;
        }
        break;

	case AEC_CONFIG_ID_BUF_ADDR: {
			#if !AEC_BUF_FIXED_MAP
			gcAecInBuf = (UINT8 *)aec_cfg_value;
			#endif
	}
		break;
	case AEC_CONFIG_ID_BUF_SIZE: {
			giAecBufSize = aec_cfg_value;
		}
		break;

    case AEC_CONFIG_ID_SPK_NUMBER:
    {
        gAecInfo.iSpkrNo = aec_cfg_value;
    }
    break;
    case AEC_CONFIG_ID_RESERVED_1:
    {
        gAecInfo.bAutoCacheable = (aec_cfg_value>0);
    }
    break;
    case AEC_CONFIG_ID_RESERVED_2:
    {
        gAecInfo.DbgPrint = (aec_cfg_value>0);
    }
    break;



    default:
        break;

    }



}

/**
	Get Minimum requirement buffer Size

	Get Minimum requirement buffer Size. This must be used before audlib_aec_init()
	and after all the other settings are done.

	return The library required buffer size in byte count
*/
INT32 audlib_aec_get_required_buffer_size(AEC_BUFINFO_ID buffer_id)
{
    ST_AUD_AEC_INFO stAecInfoPre;
    ST_AUD_AEC_RTN  stAecRtn;
    ST_AUD_AEC_PRELOAD stAecPreload;
    AEC_BUFSIZE_INFO stAECBufInfo;
    INT32               Ret=0;
    UINT32 factor;

    if((gAecInfo.iSampleRate == 0) || (gAecInfo.iMicCH==0) || (gAecInfo.iSpkrCH==0) || (gAecInfo.iSpkrNo==0)) {
        goto CFGERR;
    }

    if(gAecInfo.iSampleRate > 38400)
    {
        stAecInfoPre.u32FilterLen   = 512;//1024. Use 512 is less compute power
        stAecInfoPre.u32FrameSize   = 512;//1024
    }
    else if (gAecInfo.iSampleRate > 19200)
    {
        stAecInfoPre.u32FilterLen   = 512;
        stAecInfoPre.u32FrameSize   = 512;
    }
    else if (gAecInfo.iSampleRate >= 8000)
    {
        stAecInfoPre.u32FilterLen   = 512;
        stAecInfoPre.u32FrameSize   = 256;
    }
    else
    {
        goto CFGERR;
    }

	if(gAecDedicateFilterLen)
	{
		stAecInfoPre.u32FilterLen = gAecDedicateFilterLen;
	}

	if(gAecDedicateFrameSize)
	{
		stAecInfoPre.u32FrameSize = gAecDedicateFrameSize;
	}

    gAecInfo.iFrameSize = stAecInfoPre.u32FrameSize;

    stAecInfoPre.u32NumMic      = gAecInfo.iMicCH;
    stAecInfoPre.u32NumSpeaker  = gAecInfo.iSpkrNo;
    stAecInfoPre.u32SamplingRate= gAecInfo.iSampleRate;

    if((gAecInfo.iSpkrNo==1)&&(gAecInfo.iSpkrCH==2)) {
        stAecInfoPre.u32SpkrMixIn   = 1;
		stAecInfoPre.u32SpkrDualMono= 0;
	}
    else if ((gAecInfo.iSpkrNo==2)&&(gAecInfo.iSpkrCH==2)){
        stAecInfoPre.u32SpkrMixIn   = 0;
		stAecInfoPre.u32SpkrDualMono= 1;
	} else {
        stAecInfoPre.u32SpkrMixIn   = 0;
		stAecInfoPre.u32SpkrDualMono= 0;
	}

    AUD_AEC_PreInit(&stAecInfoPre, &stAecRtn);
    if(gAecInfo.DbgPrint){
	    DBG_WRN("BufSz= 0x%08X. %d \r\n",stAecRtn.u32InternalBufSize,stAecRtn.u32InternalBufSize);
    }


    factor = (stAecInfoPre.u32FilterLen+stAecInfoPre.u32FrameSize-1)/stAecInfoPre.u32FrameSize;

    stAecPreload.u32ForegroundSize = 2*stAecInfoPre.u32FrameSize*gAecInfo.iSpkrCH*gAecInfo.iMicCH*2*factor;
    stAecPreload.u32BackgroundSize = stAecPreload.u32ForegroundSize*2;

    stAECBufInfo.back_buf_sz     = stAecPreload.u32BackgroundSize;
    stAECBufInfo.fore_buf_sz     = stAecPreload.u32ForegroundSize;
    stAECBufInfo.internal_buf_sz = stAecRtn.u32InternalBufSize;

    switch(buffer_id)
    {
        case AEC_BUFINFO_ID_INTERNAL:
            {
                Ret = (INT32)stAECBufInfo.internal_buf_sz;
            }break;
        case AEC_BUFINFO_ID_FORESIZE:
            {
                Ret = (INT32)stAECBufInfo.fore_buf_sz;
            }break;
        case AEC_BUFINFO_ID_BACKSIZE:
            {
                Ret = (INT32)stAECBufInfo.back_buf_sz;
            }break;
        default:
            break;
    }

    return Ret;

CFGERR:
    Ret = 0;
    DBG_ERR("Config Error!\r\n");
    return Ret;
}

/**
    Initialize the Audio Echo Cancellation(AEC) before recording start

    This API would reset the AEC internal temp buffer and variables.
    The user must call this before starting of any new BitStream.

    @return
     - @b TRUE:  Done and success.
     - @b FALSE: Operation fail.
*/
BOOL audlib_aec_init(void)
{
    ST_AUD_AEC_INFO stAecInfoPre;
    ST_AUD_AEC_RTN  stAecRtn;
    ST_AUD_AEC_PRELOAD stAecPreload;
    INT32           ps32Params[2];
    UINT32 factor;

    if(!bAecOpened)
    {
        DBG_ERR("No Opened.\r\n");
        return FALSE;
    }

    if((gAecInfo.iSampleRate == 0) || (gAecInfo.iMicCH==0) || (gAecInfo.iSpkrCH==0) || (gAecInfo.iSpkrNo==0))
        goto CFGERR;

    if(gAecInfo.iSampleRate > 38400)
    {
        stAecInfoPre.u32FilterLen   = 512;//1024. Use 512 is less compute power
        stAecInfoPre.u32FrameSize   = 512;//1024
    }
    else if (gAecInfo.iSampleRate > 19200)
    {
        stAecInfoPre.u32FilterLen   = 512;
        stAecInfoPre.u32FrameSize   = 512;
    }
    else if (gAecInfo.iSampleRate >= 8000)
    {
        stAecInfoPre.u32FilterLen   = 512;
        stAecInfoPre.u32FrameSize   = 256;
    }
    else
    {
        goto CFGERR;
    }

	if(gAecDedicateFilterLen)
	{
		stAecInfoPre.u32FilterLen = gAecDedicateFilterLen;
	}

	if(gAecDedicateFrameSize)
	{
		stAecInfoPre.u32FrameSize = gAecDedicateFrameSize;
	}

    gAecInfo.iFrameSize = stAecInfoPre.u32FrameSize;

    stAecInfoPre.u32NumMic      = gAecInfo.iMicCH;
    stAecInfoPre.u32NumSpeaker  = gAecInfo.iSpkrNo;
    stAecInfoPre.u32SamplingRate= gAecInfo.iSampleRate;

    if((gAecInfo.iSpkrNo==1)&&(gAecInfo.iSpkrCH==2)) {
        stAecInfoPre.u32SpkrMixIn   = 1;
		stAecInfoPre.u32SpkrDualMono= 0;
	}
    else if ((gAecInfo.iSpkrNo==2)&&(gAecInfo.iSpkrCH==2)){
        stAecInfoPre.u32SpkrMixIn   = 0;
		stAecInfoPre.u32SpkrDualMono= 1;
	} else {
        stAecInfoPre.u32SpkrMixIn   = 0;
		stAecInfoPre.u32SpkrDualMono= 0;
	}

    AUD_AEC_PreInit(&stAecInfoPre, &stAecRtn);

    if(stAecRtn.u32InternalBufSize > giAecBufSize)
    {
        DBG_ERR("AEC Allocated Buffer not enough\r\n");
        return FALSE;
    }

	#if !AEC_BUF_FIXED_MAP
    #ifdef __KERNEL__
    fmem_dcache_sync((void *)gcAecInBuf, stAecRtn.u32InternalBufSize, DMA_BIDIRECTIONAL);
    #else
	dma_flushReadCache((UINT32)gcAecInBuf, stAecRtn.u32InternalBufSize);
	dma_flushWriteCache((UINT32)gcAecInBuf, stAecRtn.u32InternalBufSize);
	#endif
    #endif

    gAecInfo.iRxBufSize   = stAecRtn.u32MicBufSize;
    gAecInfo.iPlayBufSize = stAecRtn.u32EchoBufSize;
    gAecInfo.iOutBufSize   = stAecRtn.u32OutBufSize;

    /* Setup for preload filter coefficient*/
    stAecPreload.u32PreloadEnable = gAecInfo.iPreLoadEn;
    //DBG_ERR("<<<<<<<<<<[%d]>>>>>>>>>>>>>.\r\n",stAecPreload.u32PreloadEnable);

    stAecPreload.ps16Foreground = (short *)gAecInfo.uiForeAddr;
    stAecPreload.ps32Background = (int *)gAecInfo.uiBackAddr;
    if(stAecPreload.ps16Foreground == NULL || stAecPreload.ps32Background == NULL)
    {
        DBG_ERR("AEC preload coefficient can not be NULL\r\n");
        return FALSE;
    }

    factor = (stAecInfoPre.u32FilterLen+stAecInfoPre.u32FrameSize-1)/stAecInfoPre.u32FrameSize;

    stAecPreload.u32ForegroundSize = 2*stAecInfoPre.u32FrameSize*gAecInfo.iSpkrCH*gAecInfo.iMicCH*2*factor;
    stAecPreload.u32BackgroundSize = stAecPreload.u32ForegroundSize*2;
    if((gAecInfo.uiForeSize<(UINT32)stAecPreload.u32ForegroundSize)||(gAecInfo.uiBackSize<(UINT32)stAecPreload.u32BackgroundSize))
    {
        DBG_ERR("AEC Preload Buffer not enough\r\n");
        DBG_ERR("ForecoefSize Available= [%d]  ForecoefSize Needed= [%d]\r\n",(int)gAecInfo.uiForeSize, (int)stAecPreload.u32ForegroundSize);
        DBG_ERR("BackcoefSize Available= [%d]  BackcoefSize Needed= [%d]\r\n",(int)gAecInfo.uiBackSize, (int)stAecPreload.u32BackgroundSize);
        return FALSE;
    }
    if(stAecPreload.u32PreloadEnable)
    {
	    if(gAecInfo.DbgPrint) {
        	DBG_ERR("ForecoefSize Available= [%d]  ForecoefSize Needed= [%d]\r\n",(int)gAecInfo.uiForeSize, (int)stAecPreload.u32ForegroundSize);
        	DBG_ERR("BackcoefSize Available= [%d]  BackcoefSize Needed= [%d]\r\n",(int)gAecInfo.uiBackSize, (int)stAecPreload.u32BackgroundSize);
	    }

    }

	AUD_AEC_Init((void *)gcAecInBuf, stAecRtn.u32InternalBufSize,&stAecPreload);



    //
    //  Start AEC Configurations
    //

    /* Disable/Enable leak estimator */
    ps32Params[0] = gAecLeakEstiDisable;
    AUD_AEC_SetParam(EN_AUD_AEC_DISABLE_LEAK_ESTIMTAE, (void *)ps32Params);

    /* Set leak estimate */
    ps32Params[0] = gAecLeakEstiVal;
    AUD_AEC_SetParam(EN_AUD_AEC_LEAK_ESTIMATE, (void *)ps32Params);

    /* Amplify Signal. The bigger value would be more accurate. Default is 2 by MSW */
    ps32Params[0] = AEC_AMPLIFIED_RATIO;//  for 1st channel
    ps32Params[1] = AEC_AMPLIFIED_RATIO;//  for 2nd channel
    AUD_AEC_SetParam(EN_AUD_AEC_AMP_RATE, (void *)ps32Params);

    /* Preemphasis. Default is 0.8 by MSW */
    ps32Params[0] = (int)AEC_QCONST16(0.8, 15);  //0.75~0.9  //default=0.8
    AUD_AEC_SetParam(EN_AUD_AEC_PREEMPH, (void *)ps32Params);

    /* Notch Radius. This is for DC-Cancel. This smaller value has less convergence time, larger distortion.
       These defualt value is provided by MSW. */
    ps32Params[0] = (int)gAecNotchRadius;
    AUD_AEC_SetParam(EN_AUD_AEC_NOTCH_RADIUS, (void *)ps32Params);


    /* This parameter is for noise compression. Use 0 has less computing power. */
    ps32Params[0] = 0;  //default = 1 (linear)
    AUD_AEC_SetParam(EN_AUD_AEC_BANK_SCALE, (void *)ps32Params);

    /*This parameter is used to toggle the Non linear processing at post processing stage. Now only work for 8K MONO. */
    ps32Params[0] = gAecInfo.iNLPEn; // 0:disable 1:enable  //default=1
    AUD_AEC_SetParam(EN_AUD_AEC_NLP_ENABLE, (void *)ps32Params);


    /* This parameter is used to toggle the Amplitude Rate Module. If farend signal is saturated then disable this module. */
    ps32Params[0] = gAecARModuleEn;                 // 0:disable 1:enable     //default=1
    AUD_AEC_SetParam(EN_AUD_AEC_ENABLE_AR, (void *)ps32Params);

    /* Noise/Echo Cancellation Level Adjustment */
    ps32Params[0] = gAecInfo.iNRLvl;
    AUD_AEC_SetParam(EN_AUD_AEC_NOISE_SUPPRESS,       (void *)ps32Params);
    ps32Params[0] = gAecInfo.iECLvl;
    AUD_AEC_SetParam(EN_AUD_AEC_ECHO_SUPPRESS,        (void *)ps32Params);
    ps32Params[0] = gAecInfo.iECLvl;
    AUD_AEC_SetParam(EN_AUD_AEC_ECHO_SUPPRESS_ACTIVE, (void *)ps32Params);

    ps32Params[0] = (int)AEC_QCONST16(0.8f, 15);    //0.6~0.8//default=0.8
    AUD_AEC_SetParam(EN_AUD_AEC_ECHO_NOISE_RATIO, (void *)ps32Params);

	#if AEC_ALIGN_BUF_EN
    if((gAecInfo.uiAlignRx) && (gAecInfo.uiAlignTx))
        DBG_ERR("TX & RX Align Delay must not add together!\r\n");

    //
    //  Handle Initial Playback Align Buffer
    //
    if(gAecInfo.uiAlignRx)
    {
        if(gAecInfo.uiAlignRx > (UINT32)gAecInfo.iFrameSize)
        {
            gAecInfo.uiAlignRx = gAecInfo.iFrameSize;
            DBG_ERR("Rx Align Samples Max is %d\r\n",(int)gAecInfo.uiAlignRx);
        }

        gAecInfo.uiAlignCntRx = gAecInfo.uiAlignRx*2*gAecInfo.iMicCH;
        memset((void *) gcAecRxAlignBuf, 0x00, gAecInfo.uiAlignCntRx);
    }
    else if (gAecInfo.uiAlignTx)
    {
        if(gAecInfo.uiAlignTx > (UINT32)gAecInfo.iFrameSize)
        {
            gAecInfo.uiAlignTx = gAecInfo.iFrameSize;
            DBG_ERR("Tx Align Samples Max is %d\r\n",(int)gAecInfo.uiAlignTx);
        }

        gAecInfo.uiAlignCntTx = gAecInfo.uiAlignTx*2*gAecInfo.iSpkrCH;
        memset((void *) gcAecTxAlignBuf, 0x00, gAecInfo.uiAlignCntTx);
    }
	#endif

	#if 0//_EMULATION_
    if(gAecInfo.DbgPrint)
    {
        DBG_ERR("MSW Version Code = 0x%04X\r\n", AUD_AEC_GetVersion());
        DBG_ERR("Mic(%d)    Buf Size = %d\r\n",  gAecInfo.iMicCH,stAecRtn.u32MicBufSize);
        DBG_ERR("Echo(%d)   Buf Size = %d\r\n",  gAecInfo.iMicCH,stAecRtn.u32EchoBufSize);
        DBG_ERR("Output    Buf Size = %d\r\n",   stAecRtn.u32OutBufSize);
        DBG_ERR("External  Buf Size = %d\r\n",   stAecRtn.u32InternalBufSize);

        DBG_ERR("NoiseSup En        = %d\r\n",   gAecInfo.iNREn);
        DBG_ERR("NoiseSup Level     = %d dB\r\n",gAecInfo.iNRLvl);
        DBG_ERR("EchoSup  Level     = %d dB\r\n",gAecInfo.iECLvl);
        DBG_ERR("AlignCnt Tx= %d  Rx= %d Bytes\r\n",   gAecInfo.uiAlignCntTx,gAecInfo.uiAlignCntRx);
    }
	#endif

    bAecInit = TRUE;
    return TRUE;


CFGERR:

    DBG_ERR("Config Error!\r\n");
    return FALSE;
}



/**
    Apply Audio Echo Cancellation(AEC) to audio stream

    Apply specified audio Echo Cancellation to the input audio stream.

    @param[in] p_aec_io    The Input/Output BitStream information.

    @return
     - @b TRUE:  AEC Done and success.
     - @b FALSE: AEC Operation fail.
*/
BOOL audlib_aec_run(PAEC_BITSTREAM p_aec_io)
{
    INT32  ProcLen;
    UINT32 PlayIn,RecordIn,RecordOut;
    ST_AUD_AEC_PRELOAD stAecPreloadRT;


    if(!bAecOpened)
    {
        DBG_ERR("No Opened.\r\n");
        return FALSE;
    }

    if(!bAecInit)
    {
        DBG_ERR("No audlib_aec_init()\r\n");
        return FALSE;
    }

    if(p_aec_io->bitstram_buffer_length & 0x3FF)
    {
        DBG_ERR("Length must be multiples of 1024 samples.\r\n");
        p_aec_io->bitstram_buffer_length &= ~0x3FF;
    }

    stAecPreloadRT.u32PreloadEnable = gAecInfo.iPreLoadEn;


   // DBG_ERR("At Aec.c FORE ADDR = %x value = [%d]\r\n",stAecPreloadRT.ps16Foreground,*stAecPreloadRT.ps16Foreground);

    ProcLen     = (INT32)p_aec_io->bitstram_buffer_length;

    if(gAecInfo.bAutoCacheable)
    {
        PlayIn      = (p_aec_io->bitstream_buffer_play_in);
        RecordIn    = (p_aec_io->bitstream_buffer_record_in);
        RecordOut   = (p_aec_io->bitstram_buffer_out);

        stAecPreloadRT.ps16Foreground = (short *)(gAecInfo.uiForeAddr);
        stAecPreloadRT.ps32Background = (int *)(gAecInfo.uiBackAddr);

        //cpu_cleanInvalidateDCacheAll();
    }
    else
    {
        PlayIn      = (p_aec_io->bitstream_buffer_play_in);
        RecordIn    = (p_aec_io->bitstream_buffer_record_in);
        RecordOut   = (p_aec_io->bitstram_buffer_out);

        stAecPreloadRT.ps16Foreground =(short *)gAecInfo.uiForeAddr;
        stAecPreloadRT.ps32Background =(int *)gAecInfo.uiBackAddr;
    }



    while(ProcLen >= gAecInfo.iFrameSize)
    {
		#if AEC_ALIGN_BUF_EN
        if(gAecInfo.uiAlignRx)
        {
            memcpy((void *) (gcAecRxAlignBuf+gAecInfo.uiAlignCntRx), (void *) RecordIn, (gAecInfo.iRxBufSize - gAecInfo.uiAlignCntRx));

            AUD_AEC_Run((short *) gcAecRxAlignBuf, (short *) PlayIn, (short *) RecordOut, (short) !gAecInfo.iNREn,&stAecPreloadRT);

            memcpy((void *) gcAecRxAlignBuf, (void *) (RecordIn+(gAecInfo.iRxBufSize - gAecInfo.uiAlignCntRx)), gAecInfo.uiAlignCntRx);
        }
        else if(gAecInfo.uiAlignTx)
        {
            memcpy((void *) (gcAecTxAlignBuf+gAecInfo.uiAlignCntTx), (void *) PlayIn, (gAecInfo.iPlayBufSize - gAecInfo.uiAlignCntTx));

            AUD_AEC_Run((short *) RecordIn, (short *) gcAecTxAlignBuf, (short *) RecordOut, (short) !gAecInfo.iNREn,&stAecPreloadRT);

            memcpy((void *) gcAecTxAlignBuf, (void *) (PlayIn+(gAecInfo.iPlayBufSize - gAecInfo.uiAlignCntTx)), gAecInfo.uiAlignCntTx);
        }
        else
		#endif
        {

            AUD_AEC_Run((short *) RecordIn, (short *) PlayIn, (short *) RecordOut, (short) !gAecInfo.iNREn,&stAecPreloadRT);

        }


        RecordIn    += gAecInfo.iRxBufSize;
        PlayIn      += gAecInfo.iPlayBufSize;
        RecordOut   += gAecInfo.iOutBufSize;

        ProcLen     -= gAecInfo.iFrameSize;
    }


    gAecInfo.uiForeSize = stAecPreloadRT.u32ForegroundSize;
    gAecInfo.uiBackSize = stAecPreloadRT.u32BackgroundSize;
    gAecInfo.uiForeAddr =(UINT32) stAecPreloadRT.ps16Foreground;
    gAecInfo.uiBackAddr =(UINT32) stAecPreloadRT.ps32Background;

   // DBG_ERR("At Aec.c FORE ADDR = %x value = [%d] after AEC\r\n",stAecPreloadRT.ps16Foreground,*stAecPreloadRT.ps16Foreground);
   // DBG_ERR("FORE Size [%d] BACK size [%d]\r\n",stAecPreloadRT.u32ForegroundSize,stAecPreloadRT.u32BackgroundSize);
   // DBG_ERR("FORE Addr [%x] BACK Addr [%x]\r\n",stAecPreloadRT.ps16Foreground,stAecPreloadRT.ps32Background);





    if(gAecInfo.bAutoCacheable)
    {
        //cpu_cleanInvalidateDCacheAll();
    }

    return TRUE;
}

//@}

#endif
#if 1

/**
    Open the Audio Echo Cancellation(AEC) library.

    This API must be called to initialize the Audio Echo Cancellation(AEC) basic settings.

    @return
     - @b E_OK:   Open Done and success.
     - @b E_OACV: aublib_AEC.a Version error. Please replace the aublib_AEC.a to correct version.
*/
ER audlib_ns_open(void)
{
    if(bAudNSOpened)
        return E_OK;

    if(AUD_AEC_GetVersion() != AEC_MSW_VERSION_CODE)
    {
        DBG_ERR("aublib_NS.a Version error. Should be 0x%04X But 0x%04X\r\n",AEC_MSW_VERSION_CODE,AUD_AEC_GetVersion());
        return E_OACV;
    }

    if(audlib_aec_is_opened())
    {
        DBG_ERR("AEC-Lib and AudNS-Lib can not be used together\r\n");
        return E_OACV;
    }

    bAudNSOpened = TRUE;
    bAudNSInit = FALSE;

    memset((void *)&gAudNSInfo, 0x00, sizeof(AUDNS_INFO));

    /* AudNS Lib default parameters */
    gAudNSInfo.iNRLvl         = -6;
    gAudNSInfo.bAutoCacheable =   1;

    return E_OK;
}

/**
    Check if the udio Echo Cancellation(AEC) library is opened or not.

    Check if the udio Echo Cancellation(AEC) library is opened or not.

    @return
     - @b TRUE:  Already Opened.
     - @b FALSE: Have not opened.
*/
BOOL     audlib_ns_is_opened(void)
{
    return bAudNSOpened;
}

/**
    Close the Audio Echo Cancellation(AEC) library.

    Close the Audio Echo Cancellation(AEC) library.
    After closing the Audio Echo Cancellation(AEC) library,
    any accessing for the Audio Echo Cancellation(AEC) APIs are forbidden except audlib_aec_open().

    @return
     - @b E_OK:  Close Done and success.
*/
ER audlib_ns_close(void)
{
    if(!bAudNSOpened)
        return E_OK;

    bAudNSOpened = FALSE;
    return E_OK;
}

/**
    Set specified Audio Echo Cancellation(AEC) configurations.

    This API is used to set the specified Audio Echo Cancellation(AEC) configurations.

    @param[in] aec_sel      Select which of the AEC options is selected for configuring.
    @param[in] aec_cfg_value  The configurations of the specified AEC option.

    @return
     - @b TRUE:  SetConfig Done and success.
     - @b FALSE: SetConfig Operation fail.
*/
void audlib_ns_set_config(AUDNS_CONFIG_ID ns_sel, INT32 ns_cfg_value)
{
    if(!bAudNSOpened)
    {
        DBG_ERR("No Opened.\r\n");
        return;
    }

    bAudNSInit = FALSE;

    switch (ns_sel)
    {
    case AUDNS_CONFIG_ID_SAMPLERATE:
    {
        gAudNSInfo.iSampleRate = ns_cfg_value;
    }
    break;
    case AUDNS_CONFIG_ID_CHANNEL_NO:
    {
        gAudNSInfo.iCHANNEL = ns_cfg_value;
    }
    break;

    case AUDNS_CONFIG_ID_NOISE_CANCEL_LVL:
    {
        gAudNSInfo.iNRLvl = ns_cfg_value;
    }
    break;

	case AUDNS_CONFIG_ID_BUF_ADDR: {
			#if !AEC_BUF_FIXED_MAP
			gcAecInBuf = (UINT8 *)(ns_cfg_value);
			#endif
		}
		break;
	case AUDNS_CONFIG_ID_BUF_SIZE: {
			giAecBufSize = ns_cfg_value;
		}
		break;

    case AUDNS_CONFIG_ID_RESERVED_1:
    {
        gAudNSInfo.bAutoCacheable = (ns_cfg_value>0);
    }
    break;
    case AUDNS_CONFIG_ID_RESERVED_2:
    {
        gAudNSInfo.DbgPrint = (ns_cfg_value>0);
    }
    break;

    default:
        break;

    }

}

/**
	Get Minimum requirement buffer Size

	Get Minimum requirement buffer Size. This must be used before audlib_ns_init()
	and after all the other settings are done.

	return The library required buffer size in byte count
*/
INT32 audlib_ns_get_required_buffer_size(void)
{
    ST_AUD_NS_INFO stAudNSInfoPre;
    ST_AUD_NS_RTN  stAudNSRtn;

    if(!bAudNSOpened)
    {
        DBG_ERR("No Opened.\r\n");
        return FALSE;
    }

    if((gAudNSInfo.iSampleRate == 0) || (gAudNSInfo.iCHANNEL==0))
        goto CFGERR;

    if (gAudNSInfo.iSampleRate > 19200)
    {
        stAudNSInfoPre.s32FrameSize   = 512;
    }
    else if (gAudNSInfo.iSampleRate >= 8000)
    {
        stAudNSInfoPre.s32FrameSize   = 256;
    }
    else
    {
        goto CFGERR;
    }

    gAudNSInfo.iFrameSize = stAudNSInfoPre.s32FrameSize;

    stAudNSInfoPre.s32ChannelNum    = gAudNSInfo.iCHANNEL;
    stAudNSInfoPre.s32SamplingRate  = gAudNSInfo.iSampleRate;

    AUD_NS_PreInit(&stAudNSInfoPre, &stAudNSRtn);

    return stAudNSRtn.u32InternalBufSize;

CFGERR:

    DBG_ERR("Config Error!\r\n");
    return 0;

}

/**
    Initialize the Audio Echo Cancellation(AEC) before recording start

    This API would reset the AEC internal temp buffer and variables.
    The user must call this before starting of any new BitStream.

    @return
     - @b TRUE:  Done and success.
     - @b FALSE: Operation fail.
*/
BOOL audlib_ns_init(void)
{
    ST_AUD_NS_INFO stAudNSInfoPre;
    ST_AUD_NS_RTN  stAudNSRtn;
    INT32          ps32Params[2];

    if(!bAudNSOpened)
    {
        DBG_ERR("No Opened.\r\n");
        return FALSE;
    }

    if((gAudNSInfo.iSampleRate == 0) || (gAudNSInfo.iCHANNEL==0))
        goto CFGERR;

    if (gAudNSInfo.iSampleRate > 19200)
    {
        stAudNSInfoPre.s32FrameSize   = 512;
    }
    else if (gAudNSInfo.iSampleRate >= 8000)
    {
        stAudNSInfoPre.s32FrameSize   = 256;
    }
    else
    {
        goto CFGERR;
    }

    gAudNSInfo.iFrameSize = stAudNSInfoPre.s32FrameSize;

    stAudNSInfoPre.s32ChannelNum    = gAudNSInfo.iCHANNEL;
    stAudNSInfoPre.s32SamplingRate  = gAudNSInfo.iSampleRate;

    AUD_NS_PreInit(&stAudNSInfoPre, &stAudNSRtn);

    if(stAudNSRtn.u32InternalBufSize > giAecBufSize)
    {
        DBG_ERR("AudNS Allocated Buffer not enough\r\n");
        return FALSE;
    }

	#if !AEC_BUF_FIXED_MAP
    #ifdef __KERNEL__
    fmem_dcache_sync((void *)gcAecInBuf, stAudNSRtn.u32InternalBufSize, DMA_BIDIRECTIONAL);
    #else
	dma_flushReadCache((UINT32)gcAecInBuf, stAudNSRtn.u32InternalBufSize);
	dma_flushWriteCache((UINT32)gcAecInBuf, stAudNSRtn.u32InternalBufSize);
	#endif
    #endif

    gAudNSInfo.iBufUnitSize   = stAudNSRtn.u32OutBufSize;
    if(stAudNSRtn.u32OutBufSize != stAudNSRtn.u32InBufSize)
    {
        DBG_ERR("I/O Buffer Async\r\n");
        return FALSE;
    }

    AUD_NS_Init((void *)gcAecInBuf, stAudNSRtn.u32InternalBufSize);


    //
    //  Start AudNS Configurations
    //

    /* This parameter is for noise compression. Use 0 has less computing power. */
    ps32Params[0] = 0;  //default = 1 (linear)
    AUD_NS_SetParam(EN_AUD_NS_BANK_SCALE,       (void *)ps32Params);

    /* Noise/Echo Cancellation Level Adjustment */
    ps32Params[0] = gAudNSInfo.iNRLvl;
    AUD_NS_SetParam(EN_AUD_NS_NOISE_SUPPRESS,   (void *)ps32Params);

	#if 0//_EMULATION_
    if(gAudNSInfo.DbgPrint)
    {
        DBG_ERR("MSW Version Code = 0x%04X\r\n",    AUD_AEC_GetVersion());
        DBG_ERR("Channel(%d)    Buf Size = %d\r\n", gAudNSInfo.iCHANNEL,stAudNSRtn.u32OutBufSize);
        DBG_ERR("External  Buf Size = %d\r\n",      stAudNSRtn.u32InternalBufSize);

        DBG_ERR("Samplerate = %d FrameSize=%d\r\n", gAudNSInfo.iSampleRate,gAudNSInfo.iFrameSize);
        DBG_ERR("NoiseSup Level     = %d dB\r\n",   gAudNSInfo.iNRLvl);
    }
	#endif

    bAudNSInit = TRUE;
    return TRUE;

CFGERR:

    DBG_ERR("Config Error!\r\n");
    return FALSE;
}



/**
    Apply Audio Echo Cancellation(AEC) to audio stream

    Apply specified audio Echo Cancellation to the input audio stream.

    @param[in] p_ns_io    The Input/Output BitStream information.

    @return
     - @b TRUE:  AEC Done and success.
     - @b FALSE: AEC Operation fail.
*/
BOOL audlib_ns_run(PAUDNS_BITSTREAM p_ns_io)
{
    INT32  ProcLen;
    UINT32 RecordIn,RecordOut;

    if(!bAudNSOpened)
    {
        DBG_ERR("No Opened.\r\n");
        return FALSE;
    }

    if(!bAudNSInit)
    {
        DBG_ERR("No audlib_ns_init()\r\n");
        return FALSE;
    }

    if(p_ns_io->bitstram_buffer_length & 0x3FF)
    {
        DBG_ERR("Length must be multiples of 1024 samples.\r\n");
        p_ns_io->bitstram_buffer_length &= ~0x3FF;
    }

    ProcLen     = (INT32)p_ns_io->bitstram_buffer_length;

    if(gAudNSInfo.bAutoCacheable)
    {
        RecordIn    = (p_ns_io->bitstram_buffer_in);
        RecordOut   = (p_ns_io->bitstram_buffer_out);

        //cpu_cleanInvalidateDCacheAll();
    }
    else
    {
        RecordIn    = (p_ns_io->bitstram_buffer_in);
        RecordOut   = (p_ns_io->bitstram_buffer_out);
    }

    while(ProcLen >= gAudNSInfo.iFrameSize)
    {
        AUD_NS_Run((short *) RecordIn, (short *) RecordOut);

        RecordIn    += gAudNSInfo.iBufUnitSize;
        RecordOut   += gAudNSInfo.iBufUnitSize;

        ProcLen     -= gAudNSInfo.iFrameSize;
    }

    if(gAudNSInfo.bAutoCacheable)
    {
        //cpu_cleanInvalidateDCacheAll();
    }

    return TRUE;
}

int kdrv_audlib_aec_init(void) {
	int ret;

	ret = kdrv_audioio_reg_audiolib(KDRV_AUDIOLIB_ID_AEC, &aec_func);

	return ret;
}


#ifdef __KERNEL__
EXPORT_SYMBOL(audlib_aec_open);
EXPORT_SYMBOL(audlib_aec_is_opened);
EXPORT_SYMBOL(audlib_aec_close);
EXPORT_SYMBOL(audlib_aec_get_config);
EXPORT_SYMBOL(audlib_aec_set_config);
EXPORT_SYMBOL(audlib_aec_get_required_buffer_size);
EXPORT_SYMBOL(audlib_aec_init);
EXPORT_SYMBOL(audlib_aec_run);

EXPORT_SYMBOL(audlib_ns_open);
EXPORT_SYMBOL(audlib_ns_is_opened);
EXPORT_SYMBOL(audlib_ns_close);
EXPORT_SYMBOL(audlib_ns_set_config);
EXPORT_SYMBOL(audlib_ns_get_required_buffer_size);
EXPORT_SYMBOL(audlib_ns_init);
EXPORT_SYMBOL(audlib_ns_run);
#endif

#endif
