/*
    Audio Gain Control library

    This file is the driver of Audio Gain Control library.

    @file       Agc.c
    @ingroup    mIAudGC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/
//#include <string.h>
//#include <math.h>

#ifdef __KERNEL__
#include <linux/delay.h>
#include <mach/rcw_macro.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#include "audlib_agc_dbg.h"
#include "kdrv_audioio/audlib_agc.h"
#include "kdrv_audioio/kdrv_audioio.h"

#include "agc_int.h"

#define dma_getCacheAddr(x) (x)
#define dma_flushWriteCache(parm,parm2)
#define dma_flushReadCache(parm,parm2)

#else
#include <string.h>
#include "cpu.h"

#include "NvtVerInfo.h"
#include "Debug.h"
#include "dma.h"
#include "cache_protected.h"
#include "dma_protected.h"

#include "Audio.h"
#include "agc_int.h"
#include "Agc.h"

// Default debug level
#ifndef __DBGLVL__
#define __DBGLVL__  2       // Output all message by default. __DBGLVL__ will be set to 1 via make parameter when release code.
#endif

// Default debug filter
#ifndef __DBGFLT__
#define __DBGFLT__  "*"     // Display everything when debug level is 2
#endif
#include "DebugModule.h"
#endif


/**
    @addtogroup mIAudEC
*/
//@{

//NVTVER_VERSION_ENTRY(AGC, 1, 00, 000, 00)

static BOOL     bAgcOpened = FALSE;

static AGC_INFO	AgcInfo;
static BOOL		bMsgDump = DISABLE;
static INT32    iNgTarget=0;

static UINT16 iAudPkL[4];
static UINT16 iAudPkR[4];

static INT32 iStepTHDDecay  	= AGC_DB(AGC_TRESO_BASIS_200MS);
static INT32 iStepTHDAttack 	= AGC_DB(AGC_TRESO_BASIS_5MS);

static INT32 iNGStepTHDDecay  	= AGC_DB(AGC_TRESO_BASIS_5MS);
static INT32 iNGStepTHDAttack 	= AGC_DB(AGC_TRESO_BASIS_200MS);

static KDRV_AUDIOLIB_FUNC agc_func =
{
	.agc.open                     = audlib_agc_open,
	.agc.is_opened				  = audlib_agc_is_opened,
	.agc.close					  = audlib_agc_close,
	.agc.set_config				  = audlib_agc_set_config,
	.agc.init					  = audlib_agc_init,
	.agc.run					  = audlib_agc_run,
};



/* Mul table less than +-5.5db:  -5.5db,..., -0.5db, 0db, 0.5db, ..., 5.5db*/
const INT32 mulcoef6[23]={
    0x43ce,0x47d6,0x4c1b,0x50a2,0x556e,0x5a82,0x5fe4,
    0x6597,0x6ba2,0x7208,0x78d0,0x8000,0x879c,0x8fac,0x9837,
    0xa145,0xaadc,0xb504,0xbfc8,0xcb2f,0xd744,0xe411,0xf1a1,
};


/*
	one audio sample step
*/
static void agc_detect_peak(UINT32 uiAddr)
{
	UINT8 idx0,idx1,idx2,idx3;
	INT16 *pBuf16;

	//DBG_DUMP("Addr=0x%08X  AgcInfo.pk_up_curL=%d\r\n",uiAddr,AgcInfo.pk_up_curL);

	pBuf16 = (INT16 *)uiAddr;

	if(AgcInfo.pk_up_curL)
		AgcInfo.pk_up_curL--;

	idx0 = (AgcInfo.pk_idx+1)&0x3;
	idx1 = (idx0+1)&0x3;
	idx2 = (idx1+1)&0x3;
	idx3 = (idx2+1)&0x3;

	/* LEFT CHANNEL */
	iAudPkL[AgcInfo.pk_idx] = (UINT16)AGC_ABS(pBuf16[0]);

	if((iAudPkL[idx0] > AgcInfo.peakL_next))
	{
		if((iAudPkL[idx0]>iAudPkL[idx1])&&(iAudPkL[idx1]>iAudPkL[idx2])&&(iAudPkL[idx0]>iAudPkL[idx3]))
			AgcInfo.peakL_next = iAudPkL[idx0];
	}

	if((!AgcInfo.pk_up_curL) && (AgcInfo.peakL_next))
	{
		AgcInfo.peakL 		= AgcInfo.peakL_next;
		AgcInfo.peakL_next 	= 0;
		AgcInfo.pk_up_curL 	= AgcInfo.pk_up_thd;
		AgcInfo.peakL_update= 1;
	}

	/* RIGHT CHANNEL */
	if(AgcInfo.bStereo)
	{
		if(AgcInfo.pk_up_curR)
			AgcInfo.pk_up_curR--;

		iAudPkR[AgcInfo.pk_idx] = (UINT16)AGC_ABS(pBuf16[1]);

		if((iAudPkR[idx0] > AgcInfo.peakR_next))
		{
			if((iAudPkR[idx0]>iAudPkR[idx1])&&(iAudPkR[idx1]>iAudPkR[idx2])&&(iAudPkR[idx0]>iAudPkR[idx3]))
				AgcInfo.peakR_next = iAudPkR[idx0];
		}

		if((!AgcInfo.pk_up_curR)&& (AgcInfo.peakR_next))
		{
			AgcInfo.peakR 		= AgcInfo.peakR_next;
			AgcInfo.peakR_next 	= 0;
			AgcInfo.pk_up_curR 	= AgcInfo.pk_up_thd;
			AgcInfo.peakR_update= 1;
		}
	}

	AgcInfo.pk_idx = (AgcInfo.pk_idx+1)&0x3;
}


static INT32 logtab0[10]={
    -32768,-28262,-24149,-20365,
    -16862,-13600,-10549,-7683,
    -4981,-2425
};

static INT32 logtab1[10]={
    22529,20567,18920,17517,16308,15255,14330,13511,12780,12124
};

static INT32 logtab2[10]={
    16384,18022,19660,21299,22938,24576,26214,27853,29491,31130
};

INT32 log2_Q15(UINT16 x);
INT32 log2_Q15(UINT16 x)
{
    INT32 y,dy,dx;
    INT32 index;
    INT32 point1;
    INT32 point;

    point=0;
    while(x<16384){
        if(x==0) {
			x=1;
		}
        point++;
        x<<=1;
    };
    index=((x-16384)*20)>>15;
    dx=x-logtab2[index];
    dy=((INT32)dx*logtab1[index])>>13;
    y=(dy+logtab0[index])>>4;
    point1=-(point)<<11;
    y+=point1;
    return y;
};




#if 1
#endif

/**
    Open the Audio Gain Control (AGC) library.

    This API must be called to initialize the Audio Gain Control (AGC) basic settings.

    @return
     - @b E_OK:   Open Done and success.
*/
ER audlib_agc_open(void)
{
	if (bAgcOpened) {
		return E_OK;
	}

	bAgcOpened = TRUE;

	memset((void *)&AgcInfo, 0x00, sizeof(AGC_INFO));

	/* Setup default value */
	AgcInfo.iTargetLvl  =  AGC_DB(-12);
	AgcInfo.pk_up_thd   =  96;
	AgcInfo.iMaxGain    =  12*2;
	AgcInfo.iMinGain    = -12*2;
	AgcInfo.iNgTHD 		=  AGC_DB(-100);

	return E_OK;
}

/**
    Check if the Audio Gain Control (AGC) library is opened or not.

    Check if the Audio Gain Control (AGC) library is opened or not.

    @return
     - @b TRUE:  Already Opened.
     - @b FALSE: Have not opened.
*/
BOOL     audlib_agc_is_opened(void)
{
	return bAgcOpened;
}

/**
    Close the Audio Gain Control (AGC) library.

    Close the Audio Gain Control (AGC) library.
    After closing the Audio Gain Control (AGC) library,
    any accessing for the Audio Gain Control (AGC) APIs are forbidden except audlib_agc_open().

    @return
     - @b E_OK:  Close Done and success.
*/
ER audlib_agc_close(void)
{
	if (!bAgcOpened) {
		return E_OK;
	}

	bAgcOpened = FALSE;
	return E_OK;
}

/**
    Set specified Audio Gain Control (AGC) configurations.

    This API is used to set the specified Audio Gain Control (AGC) configurations.

    @param[in] agc_sel      Select which of the AEC options is selected for configuring.
    @param[in] iAecConfig  The configurations of the specified AEC option.

    @return
     - @b TRUE:  SetConfig Done and success.
     - @b FALSE: SetConfig Operation fail.
*/
void audlib_agc_set_config(AGC_CONFIG_ID agc_sel, INT32 agc_cfg_value)
{
	if (!bAgcOpened) {
		DBG_ERR("No Opened.\r\n");
		return;
	}

	switch(agc_sel)
	{
	case AGC_CONFIG_ID_SAMPLERATE: {
			AgcInfo.iSampleRate = agc_cfg_value;
		}break;

	case AGC_CONFIG_ID_CHANNEL_NO: {
			if(agc_cfg_value == 2)
				AgcInfo.bStereo = TRUE;
			else
				AgcInfo.bStereo = FALSE;
		}break;
	case AGC_CONFIG_ID_PEAKDET_SPEED: {
			if(agc_cfg_value > 0)
				AgcInfo.pk_up_thd   = AgcInfo.iSampleRate*agc_cfg_value/1000;
			else
				AgcInfo.pk_up_thd   = 64;
		}break;


	case AGC_CONFIG_ID_TARGET_LVL: {
			AgcInfo.iTargetLvl = agc_cfg_value;
			//DBG_DUMP("Target level = 0x%04X\r\n",AgcInfo.uiTargetLvl);
		}break;

	case AGC_CONFIG_ID_MAXGAIN: {
			AgcInfo.iMaxGain = agc_cfg_value>>AGC_GAINCTRL_FRACTIONAL;
		}break;
	case AGC_CONFIG_ID_MINGAIN: {
			AgcInfo.iMinGain = agc_cfg_value>>AGC_GAINCTRL_FRACTIONAL;
		}break;
	case AGC_CONFIG_ID_NG_THD: {
			AgcInfo.iNgTHD = agc_cfg_value;
		}break;

	case AGC_CONFIG_ID_DECAY_TIME: {
			iStepTHDDecay = AGC_DB(agc_cfg_value);
		}break;
	case AGC_CONFIG_ID_ATTACK_TIME: {
			iStepTHDAttack= AGC_DB(agc_cfg_value);
		}break;
	case AGC_CONFIG_ID_NG_DECAY_TIME: {
			iNGStepTHDDecay = AGC_DB(agc_cfg_value);
		}break;
	case AGC_CONFIG_ID_NG_ATTACK_TIME: {
			iNGStepTHDAttack= AGC_DB(agc_cfg_value);
		}break;
	case AGC_CONFIG_ID_NG_TARGET_RATIO: {
			iNgTarget = agc_cfg_value;
		}break;

	case AGC_CONFIG_ID_MSGDUMP: {
			bMsgDump = agc_cfg_value;
		}break;

	default:
		break;
	}


}

/**
    Initialize the Audio Gain Control (AGC) before recording start

    This API would reset the AGC internal temp buffer and variables.
    The user must call this before starting of any new BitStream.

    @return
     - @b TRUE:  Done and success.
     - @b FALSE: Operation fail.
*/
BOOL audlib_agc_init(void)
{
	memset((void *)iAudPkL,  0x00, sizeof(UINT16)*4);
	memset((void *)iAudPkR,  0x00, sizeof(UINT16)*4);

	AgcInfo.pk_up_curL 	= 0;
	AgcInfo.pk_up_curR 	= 0;
	AgcInfo.pk_idx 		= 0;

	AgcInfo.peakL		= 0;
	AgcInfo.peakL_next	= 0;
	AgcInfo.peakL_update= 0;
	AgcInfo.peakR		= 0;
	AgcInfo.peakR_next	= 0;
	AgcInfo.peakR_update= 0;

	AgcInfo.iPeakLDB	= 0;
	AgcInfo.iPeakRDB	= 0;
	AgcInfo.iCurGainL	= 0;
	AgcInfo.iCurGainR	= 0;
	AgcInfo.iActiveGainL= 0;
	AgcInfo.iActiveGainR= 0;
	AgcInfo.AccSumL		= 0;
	AgcInfo.AccSumR		= 0;

	if(bMsgDump){
		DBG_DUMP("[AGC] CH= %d       	SR= %d Hz\r\n",AgcInfo.bStereo+1, AgcInfo.iSampleRate);
		DBG_DUMP("[AGC] target= %d DB 	NG-THD= %d DB\r\n",AgcInfo.iTargetLvl>>AGC_GAINDB_FRACTIONAL_BITS, AgcInfo.iNgTHD>>AGC_GAINDB_FRACTIONAL_BITS);
		DBG_DUMP("[AGC] MaxG= %d DB	MinG= %d DB\r\n",AgcInfo.iMaxGain>>1,AgcInfo.iMinGain>>1);
		DBG_DUMP("[AGC] PEAKDET-THD= %d samples \r\n",AgcInfo.pk_up_thd);
		DBG_DUMP("[AGC] DECAY= %d DB	ATTACK= %d DB\r\n",iStepTHDDecay>>AGC_GAINDB_FRACTIONAL_BITS,iStepTHDAttack>>AGC_GAINDB_FRACTIONAL_BITS);
		DBG_DUMP("[AGC] NG-DECAY= %d DB	NG-ATTACK= %d DB\r\n",iNGStepTHDDecay>>AGC_GAINDB_FRACTIONAL_BITS,iNGStepTHDAttack>>AGC_GAINDB_FRACTIONAL_BITS);
	}

	return TRUE;
}

/**
    Apply Audio Echo Cancellation(AEC) to audio stream

    Apply specified audio Echo Cancellation to the input audio stream.

    @param[in] p_agc_io    The Input/Output BitStream information.

    @return
     - @b TRUE:  AEC Done and success.
     - @b FALSE: AEC Operation fail.
*/
BOOL audlib_agc_run(PAGC_BITSTREAM p_agc_io)
{
	UINT32 idx,samples,AddrIn;
	INT32  dist;
	INT16  *pBufOut,*pBufIn;
	INT32  temp;

	if (!bAgcOpened) {
		DBG_ERR("No Opened.\r\n");
		return FALSE;
	}

	samples = p_agc_io->bitstram_buffer_length;
	AddrIn  = dma_getCacheAddr(p_agc_io->bitstram_buffer_in);
	pBufIn  = (INT16 *)dma_getCacheAddr(p_agc_io->bitstram_buffer_in);
	pBufOut = (INT16 *)dma_getCacheAddr(p_agc_io->bitstram_buffer_out);

	dma_flushReadCache((UINT32)AddrIn,  samples<<(1+AgcInfo.bStereo));
	dma_flushWriteCache((UINT32)AddrIn, samples<<(1+AgcInfo.bStereo));

	dma_flushWriteCache((UINT32)pBufOut, samples<<(1+AgcInfo.bStereo));
	dma_flushReadCache((UINT32)pBufOut,  samples<<(1+AgcInfo.bStereo));

	idx=0;

	while(samples--)
	{

		/* Detect Peak */
		agc_detect_peak(AddrIn+(idx<<(1+AgcInfo.bStereo)));



		/* LEFT CHANNEL */
		if(AgcInfo.peakL_update)
		{
			//AgcInfo.iPeakLDB = (INT32)(log10((FLOAT)AgcInfo.peakL/32768.0)*20*AGC_GAINDB_FRACTIONAL);//10bits fractional
			AgcInfo.iPeakLDB = log2_Q15(AgcInfo.peakL)*301/100;

			AgcInfo.peakL_update = 0;
		}

		if(AgcInfo.iPeakLDB > AgcInfo.iNgTHD) {
			// Normal State

			dist = AgcInfo.iTargetLvl - AgcInfo.iPeakLDB - (AgcInfo.iCurGainL<<AGC_GAINCTRL_FRACTIONAL);
			AgcInfo.AccSumL += (dist);

			if(dist > AGC_TARGET_TOLERANCE) {
				//Decay
				if(AgcInfo.AccSumL > iStepTHDDecay) {
					AgcInfo.iCurGainL++;

					AgcInfo.AccSumL -= iStepTHDDecay;
				}

				if(AgcInfo.iCurGainL > AgcInfo.iMaxGain)
					AgcInfo.iCurGainL = AgcInfo.iMaxGain;
			}
			else if (dist < -AGC_TARGET_TOLERANCE) {
				//Attack
				if(AgcInfo.AccSumL < -iStepTHDAttack) {
					AgcInfo.iCurGainL--;

					AgcInfo.AccSumL += iStepTHDAttack;
				}

				if(AgcInfo.iCurGainL < AgcInfo.iMinGain)
					AgcInfo.iCurGainL = AgcInfo.iMinGain;

			}
			// else is stable

		}else {
			//NoiseGate State

			dist = AgcInfo.iTargetLvl - AgcInfo.iPeakLDB - (AgcInfo.iCurGainL<<AGC_GAINCTRL_FRACTIONAL) - ((AgcInfo.iNgTHD-AgcInfo.iPeakLDB)<<iNgTarget);
			AgcInfo.AccSumL += (dist);

			if(dist > AGC_TARGET_TOLERANCE) {
				//Decay
				if(AgcInfo.AccSumL > iNGStepTHDDecay) {
					AgcInfo.iCurGainL++;

					AgcInfo.AccSumL -= iNGStepTHDDecay;
				}

				if(AgcInfo.iCurGainL > AgcInfo.iMaxGain)
					AgcInfo.iCurGainL = AgcInfo.iMaxGain;
			}
			else if (dist < -AGC_TARGET_TOLERANCE) {
				//Attack
				if(AgcInfo.AccSumL < -iNGStepTHDAttack) {
					AgcInfo.iCurGainL--;

					AgcInfo.AccSumL += iNGStepTHDAttack;
				}

				if(AgcInfo.iCurGainL < AgcInfo.iMinGain)
					AgcInfo.iCurGainL = AgcInfo.iMinGain;
			}
			// else is stable


		}


		// Zero Crossing
		if((idx) && ((pBufIn[idx<<AgcInfo.bStereo]&0x8000)^(pBufIn[(idx-1)<<AgcInfo.bStereo]&0x8000)))
		{
			AgcInfo.iActiveGainL = AgcInfo.iCurGainL;
		}

		// Output Gain
		{
			INT32 integer,fractional;

			integer 	= (AgcInfo.iActiveGainL/12)-15;
			fractional	= AgcInfo.iActiveGainL%12;

			//temp = (INT32);
			temp = (INT32)(pBufIn[idx<<AgcInfo.bStereo]* mulcoef6[fractional+11]);

			if(integer>=0)
				temp <<= integer;
			else
				temp >>= (-integer);

			if(temp > 32767)
				temp = 32767;
			else if (temp < -32768)
				temp = 32768;

			pBufOut[idx<<AgcInfo.bStereo] = (INT16)temp;
		}

		//DBG_DUMP("dist = %fdB CurGain=%fdB\r\n",(FLOAT)dist/1024.0,((FLOAT)AgcInfo.iCurGainL/2.0));
		//DBG_DUMP("%f\r\n",((FLOAT)ActiveGainL/2.0));


		/* RIGHT CHANNEL */
		if(AgcInfo.bStereo)
		{
			if(AgcInfo.peakR_update)
			{
				//AgcInfo.iPeakRDB = (INT32)(log10((FLOAT)AgcInfo.peakR/32768.0)*20*AGC_GAINDB_FRACTIONAL);//10bits fractional
				AgcInfo.iPeakRDB = log2_Q15(AgcInfo.peakR)*301/100;

				AgcInfo.peakR_update = 0;
			}

			if(AgcInfo.iPeakRDB > AgcInfo.iNgTHD) {
				// Normal State

				dist = AgcInfo.iTargetLvl - AgcInfo.iPeakRDB - (AgcInfo.iCurGainR<<AGC_GAINCTRL_FRACTIONAL);
				AgcInfo.AccSumR += (dist);

				if(dist > AGC_TARGET_TOLERANCE) {
					//Decay
					if(AgcInfo.AccSumR > iStepTHDDecay) {
						AgcInfo.iCurGainR++;

						AgcInfo.AccSumR -= iStepTHDDecay;
					}

					if(AgcInfo.iCurGainR > AgcInfo.iMaxGain)
						AgcInfo.iCurGainR = AgcInfo.iMaxGain;
				}
				else if (dist < -AGC_TARGET_TOLERANCE) {
					//Attack
					if(AgcInfo.AccSumR < -iStepTHDAttack) {
						AgcInfo.iCurGainR--;

						AgcInfo.AccSumR += iStepTHDAttack;
					}

					if(AgcInfo.iCurGainR < AgcInfo.iMinGain)
						AgcInfo.iCurGainR = AgcInfo.iMinGain;

				}
				// else is stable

			}else {
				//NoiseGate State

				dist = AgcInfo.iTargetLvl - AgcInfo.iPeakRDB - (AgcInfo.iCurGainR<<AGC_GAINCTRL_FRACTIONAL) - ((AgcInfo.iNgTHD-AgcInfo.iPeakRDB)<<iNgTarget);
				AgcInfo.AccSumR += (dist);

				if(dist > AGC_TARGET_TOLERANCE) {
					//Decay
					if(AgcInfo.AccSumR > iNGStepTHDDecay) {
						AgcInfo.iCurGainR++;

						AgcInfo.AccSumR -= iNGStepTHDDecay;
					}

					if(AgcInfo.iCurGainR > AgcInfo.iMaxGain)
						AgcInfo.iCurGainR = AgcInfo.iMaxGain;
				}
				else if (dist < -AGC_TARGET_TOLERANCE) {
					//Attack
					if(AgcInfo.AccSumR < -iNGStepTHDAttack) {
						AgcInfo.iCurGainR--;

						AgcInfo.AccSumR += iNGStepTHDAttack;
					}

					if(AgcInfo.iCurGainR < AgcInfo.iMinGain)
						AgcInfo.iCurGainR = AgcInfo.iMinGain;

				}
				// else is stable
			}

			// Zero Crossing
			if((idx) && ((pBufIn[(idx<<AgcInfo.bStereo)+1]&0x8000)^(pBufIn[((idx-1)<<AgcInfo.bStereo)+1]&0x8000)))
			{
				AgcInfo.iActiveGainR = AgcInfo.iCurGainR;
			}

			// Output Gain
			{
				INT32 integer,fractional;

				integer 	= (AgcInfo.iActiveGainR/12)-15;
				fractional	= AgcInfo.iActiveGainR%12;

				//temp = (INT32);
				temp = (INT32)(pBufIn[(idx<<AgcInfo.bStereo)+1]* mulcoef6[fractional+11]);

				if(integer>=0)
					temp <<= integer;
				else
					temp >>= (-integer);

				if(temp > 32767)
					temp = 32767;
				else if (temp < -32768)
					temp = 32768;

				pBufOut[(idx<<AgcInfo.bStereo)+1] = (INT16)temp;
			}


		}

		idx++;
	}

	//coverity[result_independent_of_operands]
	dma_flushWriteCache(dma_getCacheAddr(p_agc_io->bitstram_buffer_out), samples<<(1+AgcInfo.bStereo));
	//coverity[result_independent_of_operands]
	dma_flushReadCache(dma_getCacheAddr(p_agc_io->bitstram_buffer_out), samples<<(1+AgcInfo.bStereo));

	return TRUE;
}

int kdrv_audlib_agc_init(void) {
	int ret;

	ret = kdrv_audioio_reg_audiolib(KDRV_AUDIOLIB_ID_AGC, &agc_func);

	return ret;
}



//@}
