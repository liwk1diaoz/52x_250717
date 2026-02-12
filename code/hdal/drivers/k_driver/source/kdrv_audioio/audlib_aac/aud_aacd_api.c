//#include <string.h>

#include <linux/module.h>
#include "audlib_aac_dbg.h"
#include "aud_aacd_api.h"
#include "kdrv_audioio/audlib_aac.h"
#include "kdrv_audioio/kdrv_audioio.h"



/*---------------------------------------------------------------------------
functionname:_aac_decode_init
description:  initialize a new decoder.
returns:     EN_AUDIO_ERROR_CODE
---------------------------------------------------------------------------*/

EN_AUDIO_ERROR_CODE _aac_decode_init(pST_AUDIO_AACDEC_CFG pstInputCfg)
{
	EN_AUDIO_ERROR_CODE error = EN_AUDIO_ERROR_NONE;

	if (!pstInputCfg) {
		return EN_AUDIO_ERROR_NOINITIAL;
	}

	//Check input config
	if ((pstInputCfg->u32nChannels > 2) || (pstInputCfg->u32nChannels <= 0)) {
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}

	if ((pstInputCfg->enSampleRate >= EN_AUDIO_SAMPLING_RATE_TOTAL) || (pstInputCfg->enSampleRate == EN_AUDIO_SAMPLING_RATE_NONE)) {
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}

	if (pstInputCfg->enCodingType >= EN_AUDIO_CODING_TYPE_TOTAL) {
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}

	error = AACInitDecoder(pstInputCfg);

	if (error) {
		DBG_ERR("^RAACD_Init: error = %d\r\n", error);
		return error;
	} else {
		return EN_AUDIO_ERROR_NONE;
	}

}

/*---------------------------------------------------------------------------
functionname:_aac_decode_one_frame
description:  Decode One Frame and feedback information.
returns:     EN_AUDIO_ERROR_CODE
---------------------------------------------------------------------------*/

EN_AUDIO_ERROR_CODE _aac_decode_one_frame(pST_AUDIO_AACD_BUFINFO pstBufInfo, pST_AUDIO_AACD_RTNINFO pRtnINfo)
{
	EN_AUDIO_ERROR_CODE error = EN_AUDIO_ERROR_NONE;

	// reset return information
	memset(pRtnINfo, 0, sizeof(ST_AUDIO_AACD_RTNINFO));


	//Input & output buffer address check
	if (!pstBufInfo->pu32InBufferAddr) {
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}
	if (!pstBufInfo->pu32OutBufferAddr) {
		return EN_AUDIO_ERROR_CONFIGUREFAIL;
	}
	if (!pstBufInfo->u32AvailBytes) {
		return EN_AUDIO_ERROR_STREAM_EMPTY;
	}


	// decode one frame
	error = AACDecode((int *)&pRtnINfo->u32OneFrameConsumeBytes,
					  (unsigned char *)pstBufInfo->pu32InBufferAddr,
					  (short *)pstBufInfo->pu32OutBufferAddr,
					  pstBufInfo->u32AvailBytes
					 );
	if ((int)error == -1) {
		return EN_AUDIO_ERROR_STREAM_EMPTY;
	} else if (error) {
		return EN_AUDIO_ERROR_RUNFAIL;
	}

	//Get decoder return information
	AACGetRtnInfo(pRtnINfo);

	//limit output buffer check
	if (pRtnINfo->u32DecodeOutSamples > AAC_MAX_NSAMPS * 2) {
		pRtnINfo->u32DecodeOutSamples = 0;
		return EN_AUDIO_ERROR_RUNFAIL;
	}
	if (pRtnINfo->u32OneFrameConsumeBytes > READBUF_SIZE) {
		pRtnINfo->u32DecodeOutSamples = 0;
		return EN_AUDIO_ERROR_RUNFAIL;
	}
	if (pRtnINfo->u32nChans > AAC_MAX_NCHANS) {
		pRtnINfo->u32DecodeOutSamples = 0;
		return EN_AUDIO_ERROR_RUNFAIL;
	}
	return EN_AUDIO_ERROR_NONE;
}

INT32 audlib_aac_decode_init(PAUDLIB_AAC_CFG p_decode_cfg)
{
	ST_AUDIO_AACDEC_CFG stInputCfg = {0};

	switch (p_decode_cfg->sample_rate) {
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

	stInputCfg.u32nChannels = p_decode_cfg->channel_number;
	stInputCfg.enCodingType = p_decode_cfg->header_enable;

	return (INT32)_aac_decode_init(&stInputCfg);
}

INT32 audlib_aac_decode_one_frame(PAUDLIB_AAC_BUFINFO p_decode_buf_info, PAUDLIB_AACD_RTNINFO p_return_info)
{
	ST_AUDIO_AACD_BUFINFO stBufInfo = {0};
	ST_AUDIO_AACD_RTNINFO RtnINfo = {0};
	EN_AUDIO_ERROR_CODE ret;

	stBufInfo.u32AvailBytes		= p_decode_buf_info->bitstram_buffer_length;
	stBufInfo.pu32InBufferAddr	= (unsigned int *) p_decode_buf_info->bitstram_buffer_in;
	stBufInfo.pu32OutBufferAddr	= (unsigned int *) p_decode_buf_info->bitstram_buffer_out;

	ret = _aac_decode_one_frame(&stBufInfo, &RtnINfo);

	p_return_info->output_size				= RtnINfo.u32DecodeOutSamples;
	p_return_info->one_frame_consume_bytes	= RtnINfo.u32OneFrameConsumeBytes;
	p_return_info->channel_number			= RtnINfo.u32nChans;
	p_return_info->bits_per_sample			= RtnINfo.u32bitsPerSample;

	switch (RtnINfo.u32bitRate) {
	case EN_AUDIO_BIT_RATE_16k :
		p_return_info->bit_rate = 16000;
		break;
	case EN_AUDIO_BIT_RATE_32k :
		p_return_info->bit_rate = 32000;
		break;
	case EN_AUDIO_BIT_RATE_48k :
		p_return_info->bit_rate = 48000;
		break;
	case EN_AUDIO_BIT_RATE_64k :
		p_return_info->bit_rate = 64000;
		break;
	case EN_AUDIO_BIT_RATE_80k :
		p_return_info->bit_rate = 80000;
		break;
	case EN_AUDIO_BIT_RATE_96k :
		p_return_info->bit_rate = 96000;
		break;
	case EN_AUDIO_BIT_RATE_112k :
		p_return_info->bit_rate = 112000;
		break;
	case EN_AUDIO_BIT_RATE_128k :
		p_return_info->bit_rate = 128000;
		break;
	case EN_AUDIO_BIT_RATE_144k :
		p_return_info->bit_rate = 144000;
		break;
	case EN_AUDIO_BIT_RATE_160k :
		p_return_info->bit_rate = 160000;
		break;
	case EN_AUDIO_BIT_RATE_192k :
		p_return_info->bit_rate = 192000;
		break;

	default:
		p_return_info->bit_rate = 0;
		break;
	}


	switch (RtnINfo.u32SampleFreq) {
	case EN_AUDIO_SAMPLING_RATE_8000 :
		p_return_info->sample_rate = 8000;
		break;
	case EN_AUDIO_SAMPLING_RATE_11025 :
		p_return_info->sample_rate = 11025;
		break;
	case EN_AUDIO_SAMPLING_RATE_12000 :
		p_return_info->sample_rate = 12000;
		break;
	case EN_AUDIO_SAMPLING_RATE_16000 :
		p_return_info->sample_rate = 16000;
		break;
	case EN_AUDIO_SAMPLING_RATE_22050 :
		p_return_info->sample_rate = 22050;
		break;
	case EN_AUDIO_SAMPLING_RATE_32000 :
		p_return_info->sample_rate = 32000;
		break;
	case EN_AUDIO_SAMPLING_RATE_44100 :
		p_return_info->sample_rate = 44100;
		break;
	case EN_AUDIO_SAMPLING_RATE_48000 :
		p_return_info->sample_rate = 48000;
		break;

	default:
		p_return_info->sample_rate = 0;
		break;
	}


	return (INT32)ret;
}


