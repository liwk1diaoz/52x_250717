/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_videoenc_menu.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_util.h"
#include "hd_debug_menu.h"
#include "hd_audiocapture.h"
#define HD_MODULE_NAME HD_AUDIOCAP
#include "hd_int.h"
#include <string.h>

#define OUT_COUNT       2
#define HD_AUDIOCAP_MAX_OUT_COUNT  OUT_COUNT

extern HD_RESULT _hd_audiocap_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id);
extern HD_RESULT _hd_audiocap_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id);
extern HD_RESULT _hd_audiocap_get_state(HD_OUT_ID _out_id, UINT32* p_state);

int _hd_audiocap_dump_info(UINT32 did)
{
	int i;
	// Make path0 = IN_0 - OUT_0
	#define HD_AUDIOCAP_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))
	HD_PATH_ID audiocap_ctrl = HD_AUDIOCAP_PATH(HD_DAL_AUDIOCAP(0), HD_CTRL, HD_CTRL);
	HD_PATH_ID audiocap_path0 = HD_AUDIOCAP_PATH(HD_DAL_AUDIOCAP(0), HD_IN(0), HD_OUT(0));
	static CHAR  s_str[3][8] = {"OFF","OPEN","START"};
	UINT32 state = 0;
	UINT32 path_en = 0;

	for (i=0; i<1; i++) {
		_hd_audiocap_get_state(HD_AUDIOCAP_OUT(0, i), &state);
		if (state > 0) {
			path_en++;
			break;
		}
	}

	if (path_en == 0) {
		return 0;
	}

	{
		HD_IN_ID audio_src_out_id = 0;
		CHAR  src_out[32] = {0};
		_hd_dump_printf("------------------------- AUDIOCAP %-2d PATH & BIND -----------------------------\r\n", did);
		_hd_dump_printf("in    out   state bind_src              bind_dest\r\n");
		_hd_audiocap_get_bind_src(HD_AUDIOCAP_IN(0, 0), &audio_src_out_id);
		_hd_src_out_id_str(HD_GET_DEV(audio_src_out_id), HD_GET_OUT(audio_src_out_id), src_out, 32);
		for (i=0; i<2; i++) {
			HD_IN_ID audio_dest_in_id = 0;
			CHAR  dest_in[32] = {0};
			_hd_audiocap_get_state(HD_AUDIOCAP_OUT(0, i), &state);
			if (state > 0) {
				_hd_audiocap_get_bind_dest(HD_AUDIOCAP_OUT(0, i), &audio_dest_in_id);
				_hd_dest_in_id_str(HD_GET_DEV(audio_dest_in_id), HD_GET_IN(audio_dest_in_id), dest_in, 32);
			_hd_dump_printf("%-4d  %-4d  %-5s %-20s  %-20s\r\n",
				0, i, s_str[state], src_out, dest_in);
			}
		}
	}

	{
		HD_AUDIOCAP_DEV_CONFIG  param_dev_cfg = {0};
		UINT32 state = 0;

		hd_audiocap_get(audiocap_ctrl, HD_AUDIOCAP_PARAM_DEV_CONFIG, &param_dev_cfg);

		_hd_dump_printf("------------------------- AUDIOCAP %-2d DEV CONFIG ------------------------------\r\n", did);
		_hd_dump_printf("max\r\n");

		_hd_audiocap_get_state(HD_AUDIOCAP_OUT(0, 0), &state);

		if (state > 0) {
			_hd_dump_printf("%-4d\r\n", 0);
		}

		_hd_dump_printf("in.sr         in.ch               in.bit        in.frm_sample       frm_num\r\n");
		if (state > 0) {
			_hd_dump_printf("%-5d         %-4d                %-4d          %-5d               %-4d\r\n",
				param_dev_cfg.in_max.sample_rate, param_dev_cfg.in_max.mode, param_dev_cfg.in_max.sample_bit, param_dev_cfg.in_max.frame_sample, param_dev_cfg.frame_num_max);
		}
		_hd_dump_printf("aec.en        aec.leak_est_en     aec.leak_est  aec.noise_lvl\r\n");
		if (state > 0) {
			_hd_dump_printf("%-4d          %-4d                %-4d          %-4d\r\n",
				param_dev_cfg.aec_max.enabled, param_dev_cfg.aec_max.leak_estimate_enabled, param_dev_cfg.aec_max.leak_estimate_value, param_dev_cfg.aec_max.noise_cancel_level);
		}
		_hd_dump_printf("aec.echo_lvl  aec.filter_len      aec.frm_size  aec.notch_radius\r\n");
		if (state > 0) {
			_hd_dump_printf("%-4d          %-4d                %-4d          %-4d\r\n",
			param_dev_cfg.aec_max.echo_cancel_level, param_dev_cfg.aec_max.filter_length, param_dev_cfg.aec_max.frame_size, param_dev_cfg.aec_max.notch_radius);
		}
		_hd_dump_printf("anr.en        anr.suppress_level  anr.hpf_freq  anr.bias_sensitive\r\n");
		if (state > 0) {
			_hd_dump_printf("%-4d          %-4d                %-4d          %-4d\r\n",
				param_dev_cfg.anr_max.enabled, param_dev_cfg.anr_max.suppress_level,
				param_dev_cfg.anr_max.hpf_cut_off_freq, param_dev_cfg.anr_max.bias_sensitive);
		}
	}

	{
		HD_AUDIOCAP_DRV_CONFIG param_drv = {0};

		hd_audiocap_get(audiocap_ctrl, HD_AUDIOCAP_PARAM_DRV_CONFIG, &param_drv);

		_hd_dump_printf("------------------------- AUDIOCAP %-2d DRV CONFIG ------------------------------\r\n", did);
		_hd_dump_printf("mono\r\n");

		_hd_audiocap_get_state(HD_AUDIOCAP_OUT(0, 0), &state);
		if (state > 0) {
			_hd_dump_printf("%-4d\r\n", param_drv.mono);
		}
	}

	{
		HD_AUDIOCAP_VOLUME param_vol = {0};
		HD_RESULT ret;

		hd_audiocap_get(audiocap_ctrl, HD_AUDIOCAP_PARAM_VOLUME, &param_vol);

		_hd_dump_printf("------------------------- AUDIOCAP %-2d VOLUME ----------------------------------\r\n", did);
		_hd_dump_printf("vol\r\n");

		_hd_audiocap_get_state(HD_AUDIOCAP_OUT(0, 0), &state);
		if (state > 0) {
			ret = _hd_audiocap_get_state(HD_AUDIOCAP_OUT(0, 1), &state);
			if (ret == HD_OK) {
				if (state > 0) {
					for (i=0; i<HD_AUDIOCAP_MAX_OUT_COUNT; i++) {
						HD_PATH_ID audio_cap_path = HD_AUDIOCAP_PATH(HD_DAL_AUDIOCAP(0), HD_IN(0), HD_OUT(i));

						ret = hd_audiocap_get(audio_cap_path, HD_AUDIOCAP_PARAM_VOLUME, &param_vol);
						if (ret == HD_OK) {
							_hd_dump_printf("out\r\n%-4d", i);
							_hd_dump_printf("  %-4d\r\n", param_vol.volume);
						}
					}
				} else {
					_hd_dump_printf("%-4d\r\n", param_vol.volume);
				}
			} else {
				_hd_dump_printf("%-4d\r\n", param_vol.volume);
			}
		}
	}

	{
		HD_AUDIOCAP_IN  param_in = {0};

		hd_audiocap_get(audiocap_path0, HD_AUDIOCAP_PARAM_IN, &param_in);

		_hd_dump_printf("------------------------- AUDIOCAP %-2d IN FRAME --------------------------------\r\n", did);
		_hd_dump_printf("in    sr     ch    bit   frm_sample\r\n");

		_hd_audiocap_get_state(HD_AUDIOCAP_OUT(0, 0), &state);
		if (state > 0) {
			_hd_dump_printf("%-4d  %-5d  %-4d  %-4d  %-4d\r\n",
				0, param_in.sample_rate, param_in.mode, param_in.sample_bit, param_in.frame_sample);
		}
	}

	{
		HD_AUDIOCAP_OUT  param_out = {0};

		hd_audiocap_get(audiocap_path0, HD_AUDIOCAP_PARAM_OUT, &param_out);

		_hd_dump_printf("------------------------- AUDIOCAP %-2d OUT FRAME -------------------------------\r\n", did);
		_hd_dump_printf("out   sr\r\n");

		_hd_audiocap_get_state(HD_AUDIOCAP_OUT(0, 0), &state);
		if (state > 0) {
			_hd_dump_printf("%-4d  %-5d\r\n", 0, param_out.sample_rate);
		}
	}

	{
		HD_AUDIOCAP_AEC param_aec = {0};
		UINT32 state = 0;

		hd_audiocap_get(audiocap_path0, HD_AUDIOCAP_PARAM_OUT_AEC, &param_aec);

		_hd_dump_printf("------------------------- AUDIOCAP %-2d AEC CONFIG ------------------------------\r\n", did);

		_hd_audiocap_get_state(HD_AUDIOCAP_OUT(0, 0), &state);

		_hd_dump_printf("out\r\n");
		if (state > 0) {
			_hd_dump_printf("%-4d\r\n", 0);
		}

		_hd_dump_printf("aec.en        aec.leak_est_en     aec.leak_est  aec.noise_lvl\r\n");
		if (state > 0) {
			_hd_dump_printf("%-4d          %-4d                %-4d          %-4d\r\n",
				param_aec.enabled, param_aec.leak_estimate_enabled, param_aec.leak_estimate_value, param_aec.noise_cancel_level);
		}
		_hd_dump_printf("aec.echo_lvl  aec.filter_len      aec.frm_size  aec.notch_radius\r\n");
		if (state > 0) {
			_hd_dump_printf("%-4d          %-4d                %-4d          %-4d\r\n",
			param_aec.echo_cancel_level, param_aec.filter_length, param_aec.frame_size, param_aec.notch_radius);
		}
	}

	{
		HD_AUDIOCAP_ANR param_anr = {0};
		UINT32 state = 0;

		hd_audiocap_get(audiocap_path0, HD_AUDIOCAP_PARAM_OUT_ANR, &param_anr);

		_hd_dump_printf("------------------------- AUDIOCAP %-2d ANR CONFIG ------------------------------\r\n", did);

		_hd_audiocap_get_state(HD_AUDIOCAP_OUT(0, 0), &state);

		_hd_dump_printf("out  anr.en  anr.suppress_level  anr.hpf_freq  anr.bias_sensitive\r\n");
		if (state > 0) {
			_hd_dump_printf("%-4d %-4d    %-4d                %-4d          %-4d\r\n",
				0, param_anr.enabled, param_anr.suppress_level,
				param_anr.hpf_cut_off_freq, param_anr.bias_sensitive);
		}
	}

	return 0;
}

static int hd_audiocap_show_status_p(void)
{
	char cmd[256];

	snprintf(cmd, 256, "cat /proc/hdal/acap/info");
	system(cmd);

	return 0;
}

static HD_DBG_MENU audiocap_debug_menu[] = {
	{0x01, "dump info",                        hd_audiocap_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_audiocap_menu(void)
{
	return hd_debug_menu_entry_p(audiocap_debug_menu, "AUDIOCAP");
}



