/**
	@brief 		pcm_anr library.\n

	@file 		pcm_anr.c

	@note 		Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>
#include "anr/audlib_anr.h"
#include "kwrap/type.h"

typedef struct snd_pcm_anr snd_pcm_anr_t;

struct snd_pcm_anr {
	snd_pcm_extplug_t ext;
	/* privates */
	char *mem_buffer;
	int handle;
	int noise_suppress;
	int hpf_cutoff_freq;
	int bias_sensitive;
	int tone_min_time;
	int blk_size_w;
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

static int anr_run(UINT32 src, UINT32 dst, UINT32 size, snd_pcm_anr_t *ext)
{
	snd_pcm_anr_t *anr_t = (snd_pcm_anr_t *)ext;
	UINT32 blksize = anr_t->blk_size_w << 1;
	UINT32 pBufIn = src;
	UINT32 pBufOut = dst;

	if(size & (blksize-1)) {
		printf("Length must be multiples of %x, insize=%x\r\n", blksize, size);
		size &= ~(blksize-1);
	}

	while (size > 0) {
		audlib_anr_run(anr_t->handle, (short *)pBufIn, (short *) pBufOut);

		size    -= blksize;
		pBufIn  += blksize;
		pBufOut += blksize;
	}

	return 0;
}

static snd_pcm_sframes_t
anr_transfer(snd_pcm_extplug_t *ext,
	       const snd_pcm_channel_area_t *dst_areas,
	       snd_pcm_uframes_t dst_offset,
	       const snd_pcm_channel_area_t *src_areas,
	       snd_pcm_uframes_t src_offset,
	       snd_pcm_uframes_t size)
{
	snd_pcm_anr_t *anr_t = (snd_pcm_anr_t *)ext;
	short *src, *dst;

	if ((size % 1024) != 0) {
		usleep(10000);
		return 0;
	}

	src = (short *)area_addr(src_areas, src_offset);
	dst = (short *)area_addr(dst_areas, dst_offset);

	anr_run((UINT32)src, (UINT32)dst, size*2, anr_t);

	/*snd_pcm_area_copy(dst_areas, dst_offset,
				  src_areas, src_offset,
				  size, SND_PCM_FORMAT_S16);*/

	return size;
}

static int anr_init(snd_pcm_extplug_t *ext)
{
	snd_pcm_anr_t *anr_t = (snd_pcm_anr_t *)ext;
	struct ANR_CONFIG anr_conf = {0};
	int ret = 0;

	anr_conf.max_bias = 106;
	anr_conf.default_bias = 64;
	anr_conf.default_eng  = 0;

	// User Configurations

	anr_conf.sampling_rate = anr_t->ext.rate;
	anr_conf.stereo        = (anr_t->ext.channels == 1)? 0 : 1;

	anr_conf.nr_db         		  = anr_t->noise_suppress;  // 3~35 dB, The magnitude suppression in dB value of Noise segments
	anr_conf.hpf_cutoff_freq      = anr_t->hpf_cutoff_freq; // Hz
	anr_conf.bias_sensitive       = anr_t->bias_sensitive;  // 1(non-sensitive)~9(very sensitive)

	// Professional Configurations (Please don't modify)
	anr_conf.noise_est_hold_time = 3;  // 1~5 second, The minimum noise consequence in seconds of detection module

	switch (anr_conf.sampling_rate) {
	case 8000:
		anr_conf.tone_min_time = ((anr_t->tone_min_time * 8000) / 10) / 128;
		anr_conf.spec_bias_low  = 3 - 1;
		anr_conf.spec_bias_high = 128 - 1;
		anr_conf.blk_size_w = 256;
		break;
	case 11025:
		anr_conf.tone_min_time = ((anr_t->tone_min_time * 11025) / 10) / 128;
		anr_conf.spec_bias_low = 3 - 1;
		anr_conf.spec_bias_high = 128 - 1;
		anr_conf.blk_size_w = 256;
		break;
	case 16000:
		anr_conf.tone_min_time = ((anr_t->tone_min_time * 16000) / 10) / 256;
		anr_conf.spec_bias_low = 3 - 1;
		anr_conf.spec_bias_high = 256 - 1;
		anr_conf.blk_size_w = 512;
		break;
	case 22050:
		anr_conf.tone_min_time = ((anr_t->tone_min_time * 22050) / 10) / 256;
		anr_conf.spec_bias_low = 3 - 1;
		anr_conf.spec_bias_high = 256 - 1;
		anr_conf.blk_size_w = 512;
		break;
	case 32000:
		anr_conf.tone_min_time = ((anr_t->tone_min_time * 32000) / 10) / 512;
		anr_conf.spec_bias_low = 3 - 1;
		anr_conf.spec_bias_high = 512 - 1;
		anr_conf.blk_size_w = 1024;
		break;
	case 44100:
		anr_conf.tone_min_time = ((anr_t->tone_min_time * 44100) / 10) / 512;
		anr_conf.spec_bias_low = 3 - 1;
		anr_conf.spec_bias_high = 512 - 1;
		anr_conf.blk_size_w = 1024;
		break;
	case 48000:
		anr_conf.tone_min_time = ((anr_t->tone_min_time * 48000) / 10) / 512;
		anr_conf.spec_bias_low = 3 - 1;
		anr_conf.spec_bias_high = 512 - 1;
		anr_conf.blk_size_w = 1024;
		break;
	default:
		printf("Not support sample rate=%d!\r\n", anr_conf.sampling_rate);
		return 0;
		break;
	}

	if (anr_conf.stereo) {
		anr_conf.blk_size_w = anr_conf.blk_size_w << 1;
	}
	anr_t->blk_size_w = anr_conf.blk_size_w;

	anr_conf.max_bias_limit = (2 * (1 << 6));

	anr_conf.memory_needed = audlib_anr_pre_init(&anr_conf);

	anr_conf.p_mem_buffer   = calloc(anr_conf.memory_needed, sizeof(char));
	if (anr_conf.p_mem_buffer == NULL)
		return -ENOMEM;


	anr_t->mem_buffer = anr_conf.p_mem_buffer;

	ret = audlib_anr_init(&(anr_t->handle), &anr_conf);

	if (ret != 0) {
		return -EPERM;
	}

	return 0;
}

static int anr_close(snd_pcm_extplug_t *ext)
{
	snd_pcm_anr_t *anr_t = (snd_pcm_anr_t *)ext;

	if (anr_t->mem_buffer) {
		audlib_anr_destroy(&(anr_t->handle));

		free(anr_t->mem_buffer);
		anr_t->mem_buffer = 0;
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

static snd_pcm_chmap_query_t **anr_query_chmaps(snd_pcm_extplug_t *ext ATTRIBUTE_UNUSED)
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

static snd_pcm_chmap_t *anr_get_chmap(snd_pcm_extplug_t *ext)
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

static const snd_pcm_extplug_callback_t anr_callback = {
	.transfer = anr_transfer,
	.init = anr_init,
	.close = anr_close,
#if SND_PCM_EXTPLUG_VERSION >= 0x10002
	.query_chmaps = anr_query_chmaps,
	.get_chmap = anr_get_chmap,
#endif
};

SND_PCM_PLUGIN_DEFINE_FUNC(anr)
{
	snd_config_iterator_t i, next;
	snd_pcm_anr_t *anr_t;
	snd_config_t *sconf = NULL;
	int err;

	int noise_suppress   = -20;
	int hpf_cutoff_freq	 = 200;
	int bias_sensitive	 = 9;
	int tone_min_time    = 70;

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
		SNDERR("Unknown field %s", id);
		return -EINVAL;
	}

	if (! sconf) {
		SNDERR("No slave configuration for filrmix pcm");
		return -EINVAL;
	}

	anr_t = calloc(1, sizeof(*anr_t));
	if (anr_t == NULL)
		return -ENOMEM;

	anr_t->ext.version = SND_PCM_EXTPLUG_VERSION;
	anr_t->ext.name = "ANR Plugin";
	anr_t->ext.callback = &anr_callback;
	anr_t->ext.private_data = anr_t;

	anr_t->noise_suppress  = noise_suppress;
	anr_t->hpf_cutoff_freq = hpf_cutoff_freq;
	anr_t->bias_sensitive  = bias_sensitive;
	anr_t->tone_min_time   = tone_min_time,

	err = snd_pcm_extplug_create(&anr_t->ext, name, root, sconf, stream, mode);
	if (err < 0) {
		free(anr_t);
		return err;
	}

	snd_pcm_extplug_set_param_minmax(&anr_t->ext,
					 SND_PCM_EXTPLUG_HW_CHANNELS,
					 1, 2);

	snd_pcm_extplug_set_slave_param_minmax(&anr_t->ext,
					 SND_PCM_EXTPLUG_HW_CHANNELS,
					 1, 2);

	snd_pcm_extplug_set_param(&anr_t->ext, SND_PCM_EXTPLUG_HW_FORMAT,
				  SND_PCM_FORMAT_S16);
	snd_pcm_extplug_set_slave_param(&anr_t->ext, SND_PCM_EXTPLUG_HW_FORMAT,
					SND_PCM_FORMAT_S16);

	*pcmp = anr_t->ext.pcm;
	return 0;
}

SND_PCM_PLUGIN_SYMBOL(anr);
