/**
	@brief 		pcm_aec library.\n

	@file 		pcm_aec.c

	@note 		Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>
#include "aec/audlib_aec.h"
#include "kwrap/type.h"

typedef struct snd_pcm_aec snd_pcm_aec_t;

struct snd_pcm_aec {
	snd_pcm_extplug_t ext;
	/* privates */
	char *aec_internal;
	char *aec_fore;
	char *aec_back;
	int spk_ch;
	int mic_ch;
	int filter_size;
	int frame_size;
	int echo_suppress;
	int noise_suppress;
	int sample_rate;
	int iRxBufSize;
	int iPlayBufSize;
	int iOutBufSize;
};

static inline void *area_addr(const snd_pcm_channel_area_t *area,
			      snd_pcm_uframes_t offset)
{
	unsigned int bitofs = area->first + area->step * offset;
	return (char *) area->addr + bitofs / 8;
}

static inline unsigned int area_step(const snd_pcm_channel_area_t *area)
{
	return area->step / 8;
}

static snd_pcm_sframes_t
aec_transfer(snd_pcm_extplug_t *ext,
	       const snd_pcm_channel_area_t *dst_areas,
	       snd_pcm_uframes_t dst_offset,
	       const snd_pcm_channel_area_t *src_areas,
	       snd_pcm_uframes_t src_offset,
	       snd_pcm_uframes_t size)
{
	snd_pcm_aec_t *aec_t = (snd_pcm_aec_t *)ext;
	short *src[2], *dst;
	unsigned int channel;
	AEC_BITSTREAM AecIO = {0};
	unsigned int src_ch_num = area_step(src_areas);
	unsigned int dst_ch_num = area_step(dst_areas);

	if ((size % 1024) != 0) {
		usleep(10000);
		return 0;
	}

	for (channel = 0; channel < 2; channel++) {
		src[channel] = (short *)area_addr(src_areas + channel, src_offset);
	}

	dst = (short *)area_addr(dst_areas, dst_offset);


	if (src_ch_num == dst_ch_num) {
		AecIO.bitstream_buffer_play_in = (UINT32)src[0];
		AecIO.bitstream_buffer_record_in = (UINT32)src[1];
		AecIO.bitstram_buffer_out = (UINT32)dst;
		AecIO.bitstram_buffer_length = size;

		if (!audlib_aec_run(&AecIO)) {
			printf("AEC failed\r\n");
			return 0;
		}
	} else {
		printf("aec_t->mic_ch=%d, src_ch_num=%d, dst_ch_num=%d\r\n", aec_t->mic_ch, src_ch_num, dst_ch_num);
		snd_pcm_area_copy(dst_areas, dst_offset, src_areas+1, src_offset, size, SND_PCM_FORMAT_S16);
	}


	return size;
}

static int aec_init(snd_pcm_extplug_t *ext)
{
	snd_pcm_aec_t *aec_t = (snd_pcm_aec_t *)ext;
	INT32 internal_size;
	INT32 fore_size;
	INT32 back_size;

	aec_t->sample_rate = aec_t->ext.rate;

	if (audlib_aec_open() != 0) {
		printf("AEC open failed\r\n");
		return 0;
	}

	audlib_aec_set_config(AEC_CONFIG_ID_LEAK_ESTIMTAE_EN, 0);

	audlib_aec_set_config(AEC_CONFIG_ID_NOISE_CANCEL_LVL, aec_t->noise_suppress);  //Defualt is -20dB. Suggest value range -3 ~ -40. Unit in dB.
	audlib_aec_set_config(AEC_CONFIG_ID_ECHO_CANCEL_LVL, aec_t->echo_suppress);   //Defualt is -50dB. Suggest value range -30 ~ -60. Unit in dB.

	audlib_aec_set_config(AEC_CONFIG_ID_SAMPLERATE, aec_t->sample_rate);
	audlib_aec_set_config(AEC_CONFIG_ID_RECORD_CH_NO, aec_t->mic_ch);
	audlib_aec_set_config(AEC_CONFIG_ID_PLAYBACK_CH_NO, aec_t->spk_ch);
	audlib_aec_set_config(AEC_CONFIG_ID_SPK_NUMBER, 1);

	audlib_aec_set_config(AEC_CONFIG_ID_FILTER_LEN, aec_t->filter_size);
	audlib_aec_set_config(AEC_CONFIG_ID_FRAME_SIZE, aec_t->frame_size);

	audlib_aec_set_config(AEC_CONFIG_ID_PRELOAD_EN, 0);



	internal_size = ALIGN_CEIL_64(audlib_aec_get_required_buffer_size(AEC_BUFINFO_ID_INTERNAL));
	fore_size     = ALIGN_CEIL_64(audlib_aec_get_required_buffer_size(AEC_BUFINFO_ID_FORESIZE));
	back_size     = ALIGN_CEIL_64(audlib_aec_get_required_buffer_size(AEC_BUFINFO_ID_BACKSIZE));

	aec_t->aec_internal = calloc(internal_size, sizeof(char));
	aec_t->aec_fore     = calloc(fore_size, sizeof(char));
	aec_t->aec_back     = calloc(back_size, sizeof(char));

	audlib_aec_set_config(AEC_CONFIG_ID_FOREADDR, (INT32)aec_t->aec_fore);
	audlib_aec_set_config(AEC_CONFIG_ID_FORESIZE, (INT32)fore_size);

	audlib_aec_set_config(AEC_CONFIG_ID_BACKADDR, (INT32)aec_t->aec_back);
	audlib_aec_set_config(AEC_CONFIG_ID_BACKSIZE, (INT32)back_size);

	audlib_aec_set_config(AEC_CONFIG_ID_BUF_ADDR, (INT32)aec_t->aec_internal);
	audlib_aec_set_config(AEC_CONFIG_ID_BUF_SIZE, (INT32)internal_size);

	if (!audlib_aec_init()) {
		printf("AEC init failed\r\n");
	}

	return 0;
}

static int aec_close(snd_pcm_extplug_t *ext)
{
	snd_pcm_aec_t *aec_t = (snd_pcm_aec_t *)ext;

	audlib_aec_close();

	if (aec_t->aec_internal) {
		free(aec_t->aec_internal);
		aec_t->aec_internal = 0;
	}
	if (aec_t->aec_fore) {
		free(aec_t->aec_fore);
		aec_t->aec_fore = 0;
	}

	if (aec_t->aec_back) {
		free(aec_t->aec_back);
		aec_t->aec_back = 0;
	}

	return 0;
}

#if SND_PCM_EXTPLUG_VERSION >= 0x10002
static unsigned int chmap[8][8] = {
	{ SND_CHMAP_MONO },
	{ SND_CHMAP_FL, SND_CHMAP_FR },
	{ SND_CHMAP_FL, SND_CHMAP_FR, SND_CHMAP_FC },
	{ SND_CHMAP_FL, SND_CHMAP_FR, SND_CHMAP_RL, SND_CHMAP_RR },
	{ SND_CHMAP_FL, SND_CHMAP_FR, SND_CHMAP_RL, SND_CHMAP_RR, SND_CHMAP_FC },
	{ SND_CHMAP_FL, SND_CHMAP_FR, SND_CHMAP_RL, SND_CHMAP_RR, SND_CHMAP_FC, SND_CHMAP_LFE },
	{ SND_CHMAP_FL, SND_CHMAP_FR, SND_CHMAP_RL, SND_CHMAP_RR, SND_CHMAP_FC, SND_CHMAP_LFE, SND_CHMAP_UNKNOWN },
	{ SND_CHMAP_FL, SND_CHMAP_FR, SND_CHMAP_RL, SND_CHMAP_RR, SND_CHMAP_FC, SND_CHMAP_LFE, SND_CHMAP_SL, SND_CHMAP_SR },
};

static snd_pcm_chmap_query_t **aec_query_chmaps(snd_pcm_extplug_t *ext ATTRIBUTE_UNUSED)
{
	snd_pcm_chmap_query_t **maps;
	int i;

	maps = calloc(9, sizeof(void *));
	if (!maps)
		return NULL;
	for (i = 0; i < 8; i++) {
		snd_pcm_chmap_query_t *p;
		p = maps[i] = calloc(i + 1 + 2, sizeof(int));
		if (!p) {
			snd_pcm_free_chmaps(maps);
			return NULL;
		}
		p->type = SND_CHMAP_TYPE_FIXED;
		p->map.channels = i + 1;
		memcpy(p->map.pos, &chmap[i][0], (i + 1) * sizeof(int));
	}
	return maps;
}

static snd_pcm_chmap_t *aec_get_chmap(snd_pcm_extplug_t *ext)
{
	snd_pcm_chmap_t *map;

	if (ext->channels < 1 || ext->channels > 8)
		return NULL;
	map = malloc((ext->channels + 1) * sizeof(int));
	if (!map)
		return NULL;
	map->channels = ext->channels;
	memcpy(map->pos, &chmap[ext->channels - 1][0], ext->channels * sizeof(int));
	return map;
}
#endif /* SND_PCM_EXTPLUG_VERSION >= 0x10002 */

static const snd_pcm_extplug_callback_t aec_callback = {
	.transfer = aec_transfer,
	.init = aec_init,
	.close = aec_close,
#if SND_PCM_EXTPLUG_VERSION >= 0x10002
	.query_chmaps = aec_query_chmaps,
	.get_chmap = aec_get_chmap,
#endif
};

SND_PCM_PLUGIN_DEFINE_FUNC(aec)
{
	snd_config_iterator_t i, next;
	snd_pcm_aec_t *aec_t;
	snd_config_t *sconf = NULL;
	int err;

	int filter_size  = 1024;
	int frame_size   = 128;

	int mic_ch       = 1;
	int spk_ch       = 1;

	int noise_suppress = -20;
	int echo_suppress  = -30;

	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;
		if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0 || strcmp(id, "hint") == 0)
			continue;
		if (strcmp(id, "slave") == 0) {
			sconf = n;
			continue;
		}
		if (strcmp(id, "mic") == 0) {
			long val;
			err = snd_config_get_integer(n, &val);
			if (err < 0) {
				SNDERR("Invalid value for %s", id);
				return err;
			}
			mic_ch = val;
			if (val == 1 || val == 2) {
				mic_ch = val;
			} else {
				SNDERR("Invalid mic channel %d", val);
			}
			continue;
		}
		if (strcmp(id, "spk") == 0) {
			long val;
			err = snd_config_get_integer(n, &val);
			if (err < 0) {
				SNDERR("Invalid value for %s", id);
				return err;
			}
			if (val == 1 || val == 2) {
				spk_ch = val;
			} else {
				SNDERR("Invalid spk channel %d", val);
			}
			continue;
		}
		if (strcmp(id, "noise") == 0) {
			long val;
			err = snd_config_get_integer(n, &val);
			if (err < 0) {
				SNDERR("Invalid value for %s", id);
				return err;
			}
			noise_suppress = val;
			continue;
		}
		if (strcmp(id, "echo") == 0) {
			long val;
			err = snd_config_get_integer(n, &val);
			if (err < 0) {
				SNDERR("Invalid value for %s", id);
				return err;
			}
			echo_suppress = val;
			continue;
		}
		SNDERR("Unknown field %s", id);
		return -EINVAL;
	}

	if (! sconf) {
		SNDERR("No slave configuration for filrmix pcm");
		return -EINVAL;
	}

	aec_t = calloc(1, sizeof(*aec_t));
	if (aec_t == NULL)
		return -ENOMEM;

	aec_t->ext.version = SND_PCM_EXTPLUG_VERSION;
	aec_t->ext.name = "AEC Plugin";
	aec_t->ext.callback = &aec_callback;
	aec_t->ext.private_data = aec_t;

	aec_t->spk_ch          = spk_ch;
	aec_t->mic_ch          = mic_ch;
	aec_t->filter_size     = filter_size;
	aec_t->frame_size      = frame_size;
	aec_t->echo_suppress   = echo_suppress;
	aec_t->noise_suppress  = noise_suppress;

	err = snd_pcm_extplug_create(&aec_t->ext, name, root, sconf, stream, mode);
	if (err < 0) {
		free(aec_t);
		return err;
	}

	snd_pcm_extplug_set_param_minmax(&aec_t->ext,
					 SND_PCM_EXTPLUG_HW_CHANNELS,
					 1, 2);

	snd_pcm_extplug_set_slave_param_minmax(&aec_t->ext,
					 SND_PCM_EXTPLUG_HW_CHANNELS,
					 1, 2);

	snd_pcm_extplug_set_param(&aec_t->ext, SND_PCM_EXTPLUG_HW_FORMAT,
				  SND_PCM_FORMAT_S16);
	snd_pcm_extplug_set_slave_param(&aec_t->ext, SND_PCM_EXTPLUG_HW_FORMAT,
					SND_PCM_FORMAT_S16);

	*pcmp = aec_t->ext.pcm;
	return 0;
}

SND_PCM_PLUGIN_SYMBOL(aec);
