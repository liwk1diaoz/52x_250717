//#include <string.h>

#include <linux/module.h>
#include "aud_aace_api.h"
#include "audlib_aac_dbg.h"
#include "kdrv_audioio/audlib_aac.h"
#include "kdrv_audioio/kdrv_audioio.h"



static KDRV_AUDIOLIB_FUNC aac_func =
{
	.aac.encode_init      		= audlib_aac_encode_init,
	.aac.decode_init			= audlib_aac_decode_init,
	.aac.encode_one_frame		= audlib_aac_encode_one_frame,
	.aac.decode_one_frame		= audlib_aac_decode_one_frame,
};


/*---------------------------------------------------------------------------
functionname:_aac_encode_init
description:  initialize a new encoder configuration.
returns:     EN_AUDIO_ERROR_CODE
---------------------------------------------------------------------------*/

EN_AUDIO_ERROR_CODE _aac_encode_init(pST_AUDIO_AACENC_CFG pstInputCfg)
{
	EN_AUDIO_ERROR_CODE error = EN_AUDIO_ERROR_NONE;

	if (!pstInputCfg) {
		return EN_AUDIO_ERROR_NOINITIAL;
	}

	//Check input config
	if ((pstInputCfg->enBitRate >= EN_AUDIO_BIT_RATE_TOTAL) || (pstInputCfg->enBitRate == EN_AUDIO_BIT_RATE_NONE)) {
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}

	if ((pstInputCfg->u32nChannels > 2) || (pstInputCfg->u32nChannels == 0)) {
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}

	if ((pstInputCfg->enSampleRate >= EN_AUDIO_SAMPLING_RATE_TOTAL) || (pstInputCfg->enSampleRate == EN_AUDIO_SAMPLING_RATE_NONE)) {
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}

	if (pstInputCfg->enEncStopFreq >= EN_AUDIO_ENC_STOP_FREQ_TOTAL) {
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}

	if (pstInputCfg->enCodingType >= EN_AUDIO_CODING_TYPE_TOTAL) {
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}

	error = AacEncOpen(pstInputCfg);

	if (error) {
		return error;
	} else {
		return EN_AUDIO_ERROR_NONE;
	}

}

/*---------------------------------------------------------------------------
functionname:_aac_encode_one_frame
description:  Encode One Frame and feedback information.
returns:     EN_AUDIO_ERROR_CODE
---------------------------------------------------------------------------*/

EN_AUDIO_ERROR_CODE _aac_encode_one_frame(pST_AUDIO_AACENC_BUFINFO pstBufInfo, pST_AUDIO_AACENC_RTNINFO pRtnINfo)
{
	short numAncDataBytes = 0;
	unsigned char ancDataBytes[128];
	unsigned int numOutBytes = 0;
	//int i = 0;
	EN_AUDIO_ERROR_CODE error = EN_AUDIO_ERROR_NONE;
	int *nOutBytes = (int *)&numOutBytes;
	ST_AUDIO_AACENC_BUFINFO stBufInfo;

	// reset return information
	pRtnINfo->u32EncodeOutBytes = 0;

	//Input & output buffer address check
	if (!pstBufInfo->pu32InBufferAddr) {
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}

	if (!pstBufInfo->pu32OutBufferAddr) {
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}

	// a stereo encoder always needs two channel input
	/*if ( _stAACEInputCfg.u32nChannels == 1)
	{
	    unsigned short *pu16Inbuf=(unsigned short *)pstBufInfo->pu32InBufferAddr;
	    for(i=pstBufInfo->u32nSamples-1;i>=0;i--) {
	    pu16Inbuf[MAX_CHANNELS*i] = pu16Inbuf[i];
	    }
	}*/

	// encode one frame
	stBufInfo.u32nSamples = pstBufInfo->u32nSamples;
	stBufInfo.pu32InBufferAddr = pstBufInfo->pu32InBufferAddr;
	stBufInfo.pu32OutBufferAddr = pstBufInfo->pu32OutBufferAddr;
	error = AacEncEncode(
				stBufInfo,
				ancDataBytes,
				&numAncDataBytes,
				nOutBytes);

	if (error) {
		return error;
	}

	//limit output buffer check
	if (numOutBytes > MIN_OUTSIZE_BYTE) {
		return EN_AUDIO_ERROR_RUNFAIL;
	}

	pRtnINfo->u32EncodeOutBytes = numOutBytes;

	return EN_AUDIO_ERROR_NONE;
}


INT32 audlib_aac_encode_init(PAUDLIB_AAC_CFG p_encode_cfg)
{
	ST_AUDIO_AACENC_CFG stInputCfg = {0};

	switch (p_encode_cfg->sample_rate) {
	case 8000 :
		stInputCfg.enSampleRate = EN_AUDIO_SAMPLING_RATE_8000;
		break;
	case 11025 :
		stInputCfg.enSampleRate = EN_AUDIO_SAMPLING_RATE_11025;
		break;
	case 12000 :
		stInputCfg.enSampleRate = EN_AUDIO_SAMPLING_RATE_12000;
		break;
	case 16000 :
		stInputCfg.enSampleRate = EN_AUDIO_SAMPLING_RATE_16000;
		break;
	case 22050 :
		stInputCfg.enSampleRate = EN_AUDIO_SAMPLING_RATE_22050;
		break;
	case 24000 :
		stInputCfg.enSampleRate = EN_AUDIO_SAMPLING_RATE_24000;
		break;
	case 32000 :
		stInputCfg.enSampleRate = EN_AUDIO_SAMPLING_RATE_32000;
		break;
	case 44100 :
		stInputCfg.enSampleRate = EN_AUDIO_SAMPLING_RATE_44100;
		break;
	case 48000 :
		stInputCfg.enSampleRate = EN_AUDIO_SAMPLING_RATE_48000;
		break;

	default:
		DBG_ERR("samplerate err\r\n");
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}

	stInputCfg.u32nChannels = p_encode_cfg->channel_number;

	switch (p_encode_cfg->encode_bit_rate) {
	case 16000 :
		stInputCfg.enBitRate = EN_AUDIO_BIT_RATE_16k;
		break;
	case 32000 :
		stInputCfg.enBitRate = EN_AUDIO_BIT_RATE_32k;
		break;
	case 48000 :
		stInputCfg.enBitRate = EN_AUDIO_BIT_RATE_48k;
		break;
	case 64000 :
		stInputCfg.enBitRate = EN_AUDIO_BIT_RATE_64k;
		break;
	case 80000 :
		stInputCfg.enBitRate = EN_AUDIO_BIT_RATE_80k;
		break;
	case 96000 :
		stInputCfg.enBitRate = EN_AUDIO_BIT_RATE_96k;
		break;
	case 112000 :
		stInputCfg.enBitRate = EN_AUDIO_BIT_RATE_112k;
		break;
	case 128000 :
		stInputCfg.enBitRate = EN_AUDIO_BIT_RATE_128k;
		break;
	case 144000 :
		stInputCfg.enBitRate = EN_AUDIO_BIT_RATE_144k;
		break;
	case 160000 :
		stInputCfg.enBitRate = EN_AUDIO_BIT_RATE_160k;
		break;
	case 192000 :
		stInputCfg.enBitRate = EN_AUDIO_BIT_RATE_192k;
		break;

	default:
		DBG_ERR("bitrate err\r\n");
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}


	stInputCfg.enCodingType = p_encode_cfg->header_enable;

	switch (p_encode_cfg->encode_stop_freq) {
	case 8000 :
		stInputCfg.enEncStopFreq = EN_AUDIO_ENC_STOP_FREQ_08K;
		break;
	case 10000 :
		stInputCfg.enEncStopFreq = EN_AUDIO_ENC_STOP_FREQ_10K;
		break;
	case 11000 :
		stInputCfg.enEncStopFreq = EN_AUDIO_ENC_STOP_FREQ_11K;
		break;
	case 12000 :
		stInputCfg.enEncStopFreq = EN_AUDIO_ENC_STOP_FREQ_12K;
		break;
	case 14000 :
		stInputCfg.enEncStopFreq = EN_AUDIO_ENC_STOP_FREQ_14K;
		break;
	case 16000 :
		stInputCfg.enEncStopFreq = EN_AUDIO_ENC_STOP_FREQ_16K;
		break;
	case 18000 :
		stInputCfg.enEncStopFreq = EN_AUDIO_ENC_STOP_FREQ_18K;
		break;
	case 20000 :
		stInputCfg.enEncStopFreq = EN_AUDIO_ENC_STOP_FREQ_20K;
		break;
	case 24000 :
		stInputCfg.enEncStopFreq = EN_AUDIO_ENC_STOP_FREQ_24K;
		break;

	default:
		DBG_ERR("stop-freq err\r\n");
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}


	return (INT32)_aac_encode_init(&stInputCfg);
}

INT32 audlib_aac_encode_one_frame(PAUDLIB_AAC_BUFINFO p_encode_buf_info, PAUDLIB_AACE_RTNINFO p_return_info)
{
	ST_AUDIO_AACENC_BUFINFO stBufInfo = {0};
	ST_AUDIO_AACENC_RTNINFO RtnINfo = {0};
	EN_AUDIO_ERROR_CODE ret;

	stBufInfo.u32nSamples		= p_encode_buf_info->bitstram_buffer_length;
	stBufInfo.pu32InBufferAddr	= (unsigned int *) p_encode_buf_info->bitstram_buffer_in;
	stBufInfo.pu32OutBufferAddr	= (unsigned int *) p_encode_buf_info->bitstram_buffer_out;

	ret = _aac_encode_one_frame(&stBufInfo, &RtnINfo);

	p_return_info->output_size = RtnINfo.u32EncodeOutBytes;
	return (INT32)ret;
}


int kdrv_audlib_aac_init(void) {
	int ret;

	ret = kdrv_audioio_reg_audiolib(KDRV_AUDIOLIB_ID_AAC, &aac_func);

	return ret;
}


