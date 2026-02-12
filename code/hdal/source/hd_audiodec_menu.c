/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_audiodec_menu.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_util.h"
#include "hd_debug_menu.h"
#include "hd_audiodec.h"
#define HD_MODULE_NAME HD_AUDIODEC
#include "hd_int.h"
#include <string.h>

#define OUT_COUNT       16

extern HD_RESULT _hd_audiodec_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id);
extern HD_RESULT _hd_audiodec_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id);
extern HD_RESULT _hd_audiodec_get_state(HD_OUT_ID _out_id, UINT32* p_state);

INT _hd_audiodec_menu_cvt_codec(HD_AUDIO_CODEC codec, CHAR *p_ret_string, INT max_str_len)
{
	switch (codec) {
		case HD_AUDIO_CODEC_AAC:   snprintf(p_ret_string, max_str_len, "AAC");   break;
		case HD_AUDIO_CODEC_ULAW:  snprintf(p_ret_string, max_str_len, "ULAW");  break;
		case HD_AUDIO_CODEC_ALAW:  snprintf(p_ret_string, max_str_len, "ALAW");  break;
		case HD_AUDIO_CODEC_PCM:  snprintf(p_ret_string, max_str_len, "PCM");  break;
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown codec(%d)\r\n", codec);
			return (-1);
	}
	return 0;
}

int _hd_audiodec_menu_read_key_input_path(void)
{
	UINT32 path;

	_hd_dump_printf("Please enter which path (0~%d)  =>\r\n", OUT_COUNT-1);
	path = hd_read_decimal_key_input("");

	if (path > OUT_COUNT) {
		_hd_dump_printf("error input, path should be (0~%d)\r\n", OUT_COUNT-1 );
		return (-1);
	}
	return (int)path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int _hd_audiodec_dump_info(void)
{
	#define HD_AUDIODEC_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

	int i;
	INT32 did = 0;  // audiodec only 1 device
	CHAR  codec[8];
	static CHAR  s_str[3][8] = {"OFF","OPEN","START"};
	UINT32 state[OUT_COUNT] = {0};

	// get each port's state first
	{
		for (i=0; i<16; i++) {
			_hd_audiodec_get_state(HD_AUDIODEC_OUT(0, i), &state[i]);
		}
	}

	{
		_hd_dump_printf("-------------------------AUDIODEC %-2d PATH & BIND -------------------------------\r\n", did);
		_hd_dump_printf("in     out    state   bind_src              bind_dest\r\n");

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				CHAR  dest_in[32] = {0};
				CHAR  src_out[32] = {0};
				HD_IN_ID audio_src_out_id = 0;
				HD_IN_ID audio_dest_in_id = 0;

				_hd_audiodec_get_bind_src(HD_AUDIODEC_IN(0, i), &audio_src_out_id);
				_hd_src_out_id_str(HD_GET_DEV(audio_src_out_id), HD_GET_OUT(audio_src_out_id), src_out, 32);

				_hd_audiodec_get_bind_dest(HD_AUDIODEC_OUT(0, i), &audio_dest_in_id);
				_hd_dest_in_id_str(HD_GET_DEV(audio_dest_in_id), HD_GET_IN(audio_dest_in_id), dest_in, 32);
				_hd_dump_printf("%-5d  %-5d  %-5s   %-20s  %-20s\r\n",
					i, i, s_str[state[i]], src_out, dest_in);
			}
		}
	}

	{
		_hd_dump_printf("-------------------------AUDIODEC %-2d PATH CONFIG -------------------------------\r\n", did);
		_hd_dump_printf("in     out    sr     ch     bit    codec  \r\n");

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_AUDIODEC_PATH_CONFIG  path_cfg;
				HD_PATH_ID audio_dec_path = HD_AUDIODEC_PATH(HD_DAL_AUDIODEC(0), HD_IN(i), HD_OUT(i));
				hd_audiodec_get(audio_dec_path, HD_AUDIODEC_PARAM_PATH_CONFIG, &path_cfg);
				_hd_audiodec_menu_cvt_codec(path_cfg.max_mem.codec_type, codec, 8);

				_hd_dump_printf("%-5d  %-5d  %-5d  %-5d  %-5d  %-5s\r\n",
					i,
					i,
					path_cfg.max_mem.sample_rate,
					path_cfg.max_mem.mode,
					path_cfg.max_mem.sample_bit,
					codec);
			}
		}
	}

	{
		_hd_dump_printf("-------------------------AUDIODEC %-2d IN FRAME ----------------------------------\r\n", did);
		_hd_dump_printf("in     sr     ch     bit    codec\r\n");

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_AUDIODEC_IN  dec_in = {0};
				HD_PATH_ID audio_dec_path = HD_AUDIODEC_PATH(HD_DAL_AUDIODEC(0), HD_IN(i), HD_OUT(i));
				hd_audiodec_get(audio_dec_path, HD_AUDIODEC_PARAM_IN, &dec_in);
				_hd_audiodec_menu_cvt_codec(dec_in.codec_type, codec, 8);

				_hd_dump_printf("%-5d  %-5d  %-5d  %-5d  %-5s\r\n",
					i,
					dec_in.sample_rate,
					dec_in.mode,
					dec_in.sample_bit,
					codec);
			}
		}
	}

	return 0;
}

static int hd_audiodec_show_status_p(void)
{
	system("cat /proc/hdal/adec/info");
	return 0;
}

/*static int hd_audiodec_show_decinfo_p(void)
{
	int path = 0;  // only test path0 now
	char cmd[256];

	snprintf(cmd, 256, "echo auddec decinfo %d > /proc/isf_auddec/cmd", path);
	system(cmd);

	return 0;
}*/

static HD_DBG_MENU audiodec_debug_menu[] = {
	{0x01, "dump status",                         hd_audiodec_show_status_p,              TRUE},
	//{0x02, "dec info",                          hd_audiodec_show_decinfo_p,           TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_audiodec_menu(void)
{
	return hd_debug_menu_entry_p(audiodec_debug_menu, "AUDIODEC");
}



