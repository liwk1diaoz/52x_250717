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
#include "hd_audioout.h"
#define HD_MODULE_NAME HD_AUDIOOUT
#include "hd_int.h"
#include <string.h>

extern HD_RESULT _hd_audioout_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id);
extern HD_RESULT _hd_audioout_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id);
extern HD_RESULT _hd_audioout_get_state(HD_OUT_ID _out_id, UINT32* p_state);

int _hd_audioout_dump_info(UINT32 did)
{
	int i;
	// Make path0 = IN_0 - OUT_0
	#define HD_AUDIOOUT_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))
	HD_PATH_ID audioout_ctrl = HD_AUDIOOUT_PATH(HD_DAL_AUDIOOUT(did), HD_CTRL, HD_CTRL);
	HD_PATH_ID audioout_path0 = HD_AUDIOOUT_PATH(HD_DAL_AUDIOOUT(did), HD_IN(0), HD_OUT(0));
	static CHAR  s_str[3][8] = {"OFF","OPEN","START"};
	UINT32 state = 0;
	UINT32 path_en = 0;

	for (i=0; i<1; i++) {
		_hd_audioout_get_state(HD_AUDIOOUT_OUT(did, i), &state);
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
		_hd_dump_printf("------------------------- AUDIOOUT %-2d PATH & BIND -----------------------------\r\n", did);
		_hd_dump_printf("in    out   state bind_src              bind_dest\r\n");
		_hd_audioout_get_bind_src(HD_AUDIOOUT_IN(did, 0), &audio_src_out_id);
		_hd_src_out_id_str(HD_GET_DEV(audio_src_out_id), HD_GET_OUT(audio_src_out_id), src_out, 32);
		for (i=0; i<1; i++) {
			//HD_IN_ID audio_dest_in_id = 0;
			CHAR  dest_in[32] = {0};
			_hd_audioout_get_state(HD_AUDIOOUT_OUT(did, i), &state);
			if (state > 0) {
				//_hd_audioout_get_bind_dest(HD_AUDIOOUT_OUT(0, i), &audio_dest_in_id);
				_hd_dest_in_id_str(0, 0, dest_in, 32);
			_hd_dump_printf("%-4d  %-4d  %-5s %-20s  %-20s\r\n",
				0, i, s_str[state], src_out, dest_in);
			}
		}
	}

	{
		HD_AUDIOOUT_DEV_CONFIG  param_dev_cfg = {0};


		hd_audioout_get(audioout_ctrl, HD_AUDIOOUT_PARAM_DEV_CONFIG, &param_dev_cfg);

		_hd_dump_printf("------------------------- AUDIOOUT %-2d DEV CONFIG ------------------------------\r\n", did);
		_hd_dump_printf("max   out.sr     out.ch    out.bit   frm_sample  frm_num\r\n");

		_hd_audioout_get_state(HD_AUDIOOUT_OUT(did, 0), &state);
		if (state > 0) {
			_hd_dump_printf("%-4d  %-5d      %-4d      %-4d      %-5d       %-4d\r\n",
				0, param_dev_cfg.out_max.sample_rate, param_dev_cfg.out_max.mode, param_dev_cfg.out_max.sample_bit,
				param_dev_cfg.frame_sample_max, param_dev_cfg.frame_num_max);
		}
	}

	{
		HD_AUDIOOUT_DRV_CONFIG param_drv = {0};

		hd_audioout_get(audioout_ctrl, HD_AUDIOOUT_PARAM_DRV_CONFIG, &param_drv);

		_hd_dump_printf("------------------------- AUDIOOUT %-2d DRV CONFIG ------------------------------\r\n", did);
		_hd_dump_printf("mono  output\r\n");

		_hd_audioout_get_state(HD_AUDIOOUT_OUT(did, 0), &state);
		if (state > 0) {
			_hd_dump_printf("%-4d  %-4d\r\n", param_drv.mono, param_drv.output);
		}
	}

	{
		HD_AUDIOOUT_VOLUME param_vol = {0};

		hd_audioout_get(audioout_ctrl, HD_AUDIOOUT_PARAM_VOLUME, &param_vol);

		_hd_dump_printf("------------------------- AUDIOOUT %-2d VOLUME ----------------------------------\r\n", did);
		_hd_dump_printf("vol\r\n");

		_hd_audioout_get_state(HD_AUDIOOUT_OUT(did, 0), &state);
		if (state > 0) {
			_hd_dump_printf("%-4d\r\n", param_vol.volume);
		}
	}

	{
		HD_AUDIOOUT_OUT  param_out = {0};

		hd_audioout_get(audioout_path0, HD_AUDIOOUT_PARAM_OUT, &param_out);

		_hd_dump_printf("------------------------- AUDIOOUT %-2d OUT FRAME -------------------------------\r\n", did);
		_hd_dump_printf("out   sr     ch    bit\r\n");
		_hd_audioout_get_state(HD_AUDIOOUT_OUT(did, 0), &state);
		if (state > 0) {
			_hd_dump_printf("%-4d  %-5d  %-4d  %-4d\r\n",
				0, param_out.sample_rate, param_out.mode, param_out.sample_bit);
		}
	}

	{
		HD_AUDIOOUT_IN  param_in = {0};

		hd_audioout_get(audioout_path0, HD_AUDIOOUT_PARAM_IN, &param_in);

		_hd_dump_printf("------------------------- AUDIOOUT %-2d IN FRAME --------------------------------\r\n", did);
		_hd_dump_printf("in    sr\r\n");

		_hd_audioout_get_state(HD_AUDIOOUT_OUT(did, 0), &state);
		if (state > 0) {
			_hd_dump_printf("%-4d  %-5d\r\n", 0, param_in.sample_rate);
		}
	}

	return 0;
}

static int hd_audioout_show_status_p(void)
{
	char cmd[256];

	snprintf(cmd, 256, "cat /proc/hdal/aout/info");
	system(cmd);

	return 0;
}

static HD_DBG_MENU audioout_debug_menu[] = {
	{0x01, "dump info",                         hd_audioout_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_audioout_menu(void)
{
	return hd_debug_menu_entry_p(audioout_debug_menu, "AUDIOOUT");
}



