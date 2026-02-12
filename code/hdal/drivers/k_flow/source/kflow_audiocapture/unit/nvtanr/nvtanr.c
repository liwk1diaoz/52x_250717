/*
    NvtAnr.

    This file is the task of Audio Noise Reduction.

    @file       nvtanr.c
    @ingroup    mIAPPNVTANR
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#if defined __UITRON || defined __ECOS
#include "nvtanr_int.h"
#include "nvtanr_api.h"
#include "audlib_ANR.h"
#include "timer.h"
#include "NvtVerInfo.h"
#include "dma.h"
#include "cache_protected.h"
#else
#include "nvtanr_int.h"
#include "../include/nvtanr_api.h"
#include "kdrv_audioio/audlib_anr.h"
#include "kdrv_audioio/kdrv_audioio.h"
#include "kwrap/cpu.h"
#endif

///////////////////////////////////////////////////////////////////////////////
#define THIS_DBGLVL         NVT_DBG_WRN
#define __MODULE__          nvtanr
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#include "kwrap/debug.h"

unsigned int nvtanr_debug_level = THIS_DBGLVL;
///////////////////////////////////////////////////////////////////////////////

#define ANR_INTERNAL_BUFSIZE    519424
#define ANR_OUT_BUFSIZE    		4096
#define ANR_FIX_BUF				DISABLE

static struct ANR_CONFIG anr_conf = {0};
static int handle = 0;
static MEM_RANGE anrbuf = {0};
static MEM_RANGE outbuf = {0};
BOOL anr_en = FALSE;
static BOOL anr_open = FALSE;

static KDRV_AUDIOLIB_FUNC* anr_obj = NULL;

NVTANR_AUD_INFO anr_info = {
	.nr_db			 = 13,
	.hpf_cutoff_freq	 = 200,
	.bias_sensitive	 = 9,
	.tone_min_time     = 70,
};

#if (ANR_FIX_BUF==ENABLE)
static UINT8 anrbuf_fix[ANR_INTERNAL_BUFSIZE];
static UINT8 outbuf_fix[ANR_OUT_BUFSIZE];
#endif

#if defined __UITRON || defined __ECOS
NVTVER_VERSION_ENTRY(NvtAnr, 1, 00, 004, 00)
#endif

#if 0
static int GetSpecLen(int SR)
{
	int Len = 0;

	switch (SR) {
	case 8000:
		Len = 129;
		break;
	case 11025:
		Len = 129;
		break;
	case 16000:
		Len = 257;
		break;
	case 22050:
		Len = 257;
		break;
	case 32000:
		Len = 513;
		break;
	case 44100:
		Len = 513;
		break;
	case 48000:
		Len = 513;
		break;
	default:
		break;
	}
	return Len;
}
#endif

static BOOL config_anr(PNVTANR_AUD_INFO info, struct ANR_CONFIG *sANR)
{
	//int SpecLen;

	// Note: Replace the following 4 lines by the result of AUD_ANR_Detect()
	sANR->max_bias = 106;
	sANR->default_bias = 64;
	sANR->default_eng  = 0;

	// User Configurations

	sANR->sampling_rate = info->sampling_rate;
	sANR->stereo   = info->stereo;

	sANR->nr_db         		  = info->nr_db; 			 // 3~35 dB, The magnitude suppression in dB value of Noise segments
	sANR->hpf_cutoff_freq      = info->hpf_cutoff_freq; // Hz
	sANR->bias_sensitive       = info->bias_sensitive;  // 1(non-sensitive)~9(very sensitive)

	// Professional Configurations (Please don't modify)
	sANR->noise_est_hold_time = 3;  // 1~5 second, The minimum noise consequence in seconds of detection module

	switch (sANR->sampling_rate) {
	case 8000:
		sANR->tone_min_time = ((info->tone_min_time * 8000) / 10) / 128;
		sANR->spec_bias_low  = 3 - 1;
		sANR->spec_bias_high = 128 - 1;
		sANR->blk_size_w = 256;
		break;
	case 11025:
		sANR->tone_min_time = ((info->tone_min_time * 11025) / 10) / 128;
		sANR->spec_bias_low = 3 - 1;
		sANR->spec_bias_high = 128 - 1;
		sANR->blk_size_w = 256;
		break;
	case 16000:
		sANR->tone_min_time = ((info->tone_min_time * 16000) / 10) / 256;
		sANR->spec_bias_low = 3 - 1;
		sANR->spec_bias_high = 256 - 1;
		sANR->blk_size_w = 512;
		break;
	case 22050:
		sANR->tone_min_time = ((info->tone_min_time * 22050) / 10) / 256;
		sANR->spec_bias_low = 3 - 1;
		sANR->spec_bias_high = 256 - 1;
		sANR->blk_size_w = 512;
		break;
	case 32000:
		sANR->tone_min_time = ((info->tone_min_time * 32000) / 10) / 512;
		sANR->spec_bias_low = 3 - 1;
		sANR->spec_bias_high = 512 - 1;
		sANR->blk_size_w = 1024;
		break;
	case 44100:
		sANR->tone_min_time = ((info->tone_min_time * 44100) / 10) / 512;
		sANR->spec_bias_low = 3 - 1;
		sANR->spec_bias_high = 512 - 1;
		sANR->blk_size_w = 1024;
		break;
	case 48000:
		sANR->tone_min_time = ((info->tone_min_time * 48000) / 10) / 512;
		sANR->spec_bias_low = 3 - 1;
		sANR->spec_bias_high = 512 - 1;
		sANR->blk_size_w = 1024;
		break;
	default:
		DBG_ERR("Not support sample rate=%d!\r\n", sANR->sampling_rate);
		return 0;
		break;
	}

	if (sANR->stereo) {
		sANR->blk_size_w = sANR->blk_size_w << 1;
	}

	//SpecLen = GetSpecLen(sANR->sampling_rate);
	sANR->max_bias_limit = (2 * (1 << 6));

	return 1;
}

ER nvtanr_open(INT32 samplerate, INT32 channelnum)
{
	int ret = 0;

	//xNvtAnr_InstallCmd();

	if (anr_obj == NULL) {
		anr_obj = kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID_ANR);
	}

	if (anr_obj == NULL) {
		DBG_ERR("anr obj is NULL\r\n");
		return E_NOEXS;
	}

	if (anr_obj->anr.pre_init == NULL) {
		DBG_ERR("pre_init is NULL\r\n");
		return E_NOEXS;
	}

	if (anr_obj->anr.init == NULL) {
		DBG_ERR("init is NULL\r\n");
		return E_NOEXS;
	}

	anr_info.sampling_rate = samplerate;
	anr_info.stereo  	  = (channelnum == 1)? 0 : 1;

	if (!config_anr(&anr_info, &anr_conf)) {
		DBG_ERR("NvtAnr open failed\r\n");
		return E_SYS;
	}

	anr_conf.memory_needed = anr_obj->anr.pre_init(&anr_conf);
	#if (ANR_FIX_BUF==ENABLE)
	anr_conf.p_mem_buffer   = (void *)&anrbuf_fix;
	if (anr_conf.memory_needed > ANR_INTERNAL_BUFSIZE) {
		DBG_ERR("Buffer is not enough. Needed=%x\r\n", anr_conf.memory_needed);
		return E_SYS;
	}

	//DBGH(anr_conf.p_mem_buffer);
	#else
	anr_conf.p_mem_buffer   = (void *)dma_getCacheAddr(anrbuf.addr);
	//anr_conf.p_mem_buffer   = (void *)anrbuf.addr;
	if ((UINT32)anr_conf.memory_needed > anrbuf.size) {
		DBG_ERR("Internal buffer is not enough. Needed=%x, but %x\r\n", anr_conf.memory_needed, anrbuf.size);
		return E_SYS;
	}

	outbuf.addr = (UINT32)anr_conf.p_mem_buffer + anr_conf.memory_needed;
	outbuf.size = anrbuf.size - anr_conf.memory_needed;

	if ((UINT32)(anr_conf.blk_size_w << 1) > outbuf.size) {
		DBG_ERR("Working buffer is not enough. Needed=%x, but %x\r\n", (anr_conf.blk_size_w << 1), outbuf.size);
		return E_SYS;
	}
	#endif

	ret = anr_obj->anr.init(&handle, &anr_conf);

	if (ret == 0) {
		anr_open = TRUE;
		return E_OK;
	} else {
		return E_SYS;
	}
}

ER nvtanr_close(void)
{
	if (anr_obj == NULL) {
		DBG_ERR("anr obj is NULL\r\n");
		return E_NOEXS;
	}

	if (anr_obj->anr.destroy == NULL) {
		DBG_ERR("destroy is NULL\r\n");
		return E_NOEXS;
	}

	anr_obj->anr.destroy(&handle);
	anr_open = FALSE;

	return E_OK;
}

ER nvtanr_apply(UINT32 inaddr, UINT32 insize)
{
	UINT32 pBufIn = 0;
	UINT32 pBufOut = 0;
	UINT32 blksize = anr_conf.blk_size_w << 1;

	if (anr_obj == NULL) {
		DBG_ERR("anr obj is NULL\r\n");
		return E_NOEXS;
	}

	if (anr_obj->anr.run == NULL) {
		DBG_ERR("run is NULL\r\n");
		return E_NOEXS;
	}

	if (!anr_open) {
		DBG_ERR("NvtAnr is not opened\r\n");
		return E_SYS;
	}

	if (!anr_en) {
		return E_OK;
	}

	if(insize & (blksize-1)) {
		DBG_ERR("Length must be multiples of %x, insize=%x\r\n", blksize, insize);
		insize &= ~(blksize-1);
	}

	pBufIn = dma_getCacheAddr(inaddr);
	#if (ANR_FIX_BUF==ENABLE)
	pBufOut = dma_getCacheAddr((UINT32)&outbuf_fix);
	#else
	pBufOut = dma_getCacheAddr((UINT32)outbuf.addr);
	#endif

	while (insize > 0) {
		anr_obj->anr.run(handle, (short *)pBufIn, (short *) pBufOut);
		memcpy((void *)pBufIn, (const void *)pBufOut, blksize);

		insize -= blksize;
		pBufIn += blksize;
		//DBGD(insize);
	}

	return E_OK;
}

void nvtanr_enable(BOOL enable)
{
	anr_en = enable;
}

void nvtanr_setconfig(NVTANR_CONFIG_ID config_id, INT32 value)
{
	switch (config_id) {
	case NVTANR_CONFIG_ID_NRDB: {
			if (value < 3) {
				DBG_ERR("dB value=%d < 3. Set to 3.\r\n", value);
				value = 3;
			} else if (value > 35) {
				DBG_ERR("dB value=%d > 35. Set to 35.\r\n", value);
				value = 35;
			}
			anr_info.nr_db = (int)value;
		}
		break;
	case NVTANR_CONFIG_ID_HPF_FREQ: {
			if (value < 0) {
				DBG_ERR("dB value=%d < 0. Set to 50Hz.\r\n", value);
				value = 50;
			}
			anr_info.hpf_cutoff_freq = (int)value;
		}
		break;

	case NVTANR_CONFIG_ID_BIAS_SENSITIVE: {
			if (value < 1) {
				DBG_ERR("Bias sensitive value=%d < 1. Set to 1.\r\n", value);
				value = 1;
			} else if (value > 9) {
				DBG_ERR("Bias sensitive value=%d > 9. Set to 9.\r\n", value);
				value = 9;
			}
			anr_info.bias_sensitive = (int)value;
		}
		break;
	case NVTANR_CONFIG_ID_TONE_MIN_TIME: {
			anr_info.tone_min_time = (int)value;
		}
		break;
	default:
		break;
	}

}

INT32 nvtanr_getconfig(NVTANR_CONFIG_ID config_id)
{
	INT32 ret = 0;

	switch (config_id) {
	case NVTANR_CONFIG_ID_NRDB: {
			ret = (INT32)anr_info.nr_db;
		}
		break;
	case NVTANR_CONFIG_ID_HPF_FREQ: {
			ret = (INT32)anr_info.hpf_cutoff_freq;
		}
		break;

	case NVTANR_CONFIG_ID_BIAS_SENSITIVE: {
			ret = (INT32)anr_info.bias_sensitive;
		}
		break;
	case NVTANR_CONFIG_ID_TONE_MIN_TIME: {
			ret = anr_info.tone_min_time;
		}
		break;
	default:
		break;
	}

	return ret;
}

UINT32 nvtanr_getmem(INT32 samplerate, INT32 channelnum)
{
	UINT32 mem_size = 0;

	if (anr_obj == NULL) {
		anr_obj = kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID_ANR);
	}

	if (anr_obj == NULL) {
		DBG_ERR("anr obj is NULL\r\n");
		return 0;
	}

	if (anr_obj->anr.pre_init == NULL) {
		DBG_ERR("pre_init is NULL\r\n");
		return 0;
	}

	anr_info.sampling_rate = samplerate;
	anr_info.stereo  	  = (channelnum == 1)? 0 : 1;

	if (!config_anr(&anr_info, &anr_conf)) {
		DBG_ERR("Config ANR failed\r\n");
		return 0;
	}

	anr_conf.memory_needed = anr_obj->anr.pre_init(&anr_conf);
	mem_size = anr_conf.memory_needed + (anr_conf.blk_size_w << 1);

	return mem_size;
}

void nvtanr_setmem(UINT32 addr, UINT32 size)
{
	anrbuf.addr = addr;
	anrbuf.size = size;

	DBG_IND("NvtAnr mem addr=0x%x, size=0x%x\r\n", anrbuf.addr, anrbuf.size);
}

