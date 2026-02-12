/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_videodec_menu.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_util.h"
#include "hd_debug_menu.h"
#include "hd_videodec.h"
#define HD_MODULE_NAME HD_VIDEODEC
#include "hd_int.h"
#include <string.h>

#define OUT_COUNT       16

extern HD_RESULT _hd_videodec_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id);
extern HD_RESULT _hd_videodec_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id);
extern HD_RESULT _hd_videodec_get_state(HD_OUT_ID _out_id, UINT32* p_state);

INT _hd_videodec_menu_cvt_codec(HD_VIDEO_CODEC codec, CHAR *p_ret_string, INT max_str_len)
{
	switch (codec) {
		case HD_CODEC_TYPE_JPEG:  snprintf(p_ret_string, max_str_len, "JPEG");  break;
		case HD_CODEC_TYPE_H264:  snprintf(p_ret_string, max_str_len, "H264");  break;
		case HD_CODEC_TYPE_H265:  snprintf(p_ret_string, max_str_len, "H265");  break;
		default:
			snprintf(p_ret_string, max_str_len, "error");
			_hd_dump_printf("unknown codec(%d)\r\n", codec);
			return (-1);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int _hd_videodec_dump_info(void)
{
	#define HD_VIDEODEC_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

	int i;
	INT32 did = 0;  // videodec only 1 device
	CHAR  codec[8];
	static CHAR  s_str[3][8] = {"OFF","OPEN","START"};
	UINT32 state[OUT_COUNT] = {0};

	// get each port's state first
	{
		for (i=0; i<16; i++) {
			_hd_videodec_get_state(HD_VIDEODEC_OUT(0, i), &state[i]);
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEODEC %-2d PATH & BIND ------------------------------\r\n", did);
		_hd_dump_printf("in    out   state bind_src              bind_dest\r\n");

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				CHAR  dest_in[32] = "";
				CHAR  src_out[32] = "";
				HD_IN_ID video_src_out_id = 0;
				HD_IN_ID video_dest_in_id = 0;

				_hd_videodec_get_bind_src(HD_VIDEODEC_IN(0, i), &video_src_out_id);
				_hd_src_out_id_str(HD_GET_DEV(video_src_out_id), HD_GET_OUT(video_src_out_id), src_out, 32);

				_hd_videodec_get_bind_dest(HD_VIDEODEC_OUT(0, i), &video_dest_in_id);
				_hd_dest_in_id_str(HD_GET_DEV(video_dest_in_id), HD_GET_IN(video_dest_in_id), dest_in, 32);
				_hd_dump_printf("%-4d  %-4d  %-5s %-20s  %-20s\r\n",
					i, i, s_str[state[i]], src_out, dest_in);
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEODEC %-2d PATH CONFIG ------------------------------\r\n", did);
		_hd_dump_printf("in    out   max_w max_h  codec  ddr_id\r\n");

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEODEC_PATH_CONFIG  path_cfg;
				HD_PATH_ID video_dec_path = HD_VIDEODEC_PATH(HD_DAL_VIDEODEC(0), HD_IN(i), HD_OUT(i));
				hd_videodec_get(video_dec_path, HD_VIDEODEC_PARAM_PATH_CONFIG, &path_cfg);
				_hd_videodec_menu_cvt_codec(path_cfg.max_mem.codec_type, codec, 8);

				_hd_dump_printf("%-4d  %-4d  %-4d  %-4d   %-4s   %-4d\r\n",
					i,
					i,
					path_cfg.max_mem.dim.w,
					path_cfg.max_mem.dim.h,
					codec,
					path_cfg.max_mem.ddr_id);
			}
		}
	}

	{
		_hd_dump_printf("------------------------- VIDEODEC %-2d IN FRAME ---------------------------------\r\n", did);
		_hd_dump_printf("in    codec\r\n");

		for (i=0; i<OUT_COUNT; i++) {
			if (state[i] > 0) {
				HD_VIDEODEC_IN  dec_in = {0};
				HD_PATH_ID video_dec_path = HD_VIDEODEC_PATH(HD_DAL_VIDEODEC(0), HD_IN(i), HD_OUT(i));
				hd_videodec_get(video_dec_path, HD_VIDEODEC_PARAM_IN, &dec_in);
				_hd_videodec_menu_cvt_codec(dec_in.codec_type, codec, 8);

				_hd_dump_printf("%-4d  %-4s\r\n", i, codec);
			}
		}
	}

	return 0;
}

static int hd_videodec_show_status_p(void)
{
	system("cat /proc/hdal/vdec/info");
	return 0;
}

/*static int hd_videodec_show_decinfo_p(void)
{
	int path = _hd_videodec_menu_read_key_input_path();
	char cmd[256];

	if (path < 0) return 0;

	snprintf(cmd, 256, "echo vdodec decinfo %d > /proc/hdal/vdec/cmd", path);
	system(cmd);

	return 0;
}*/

static HD_DBG_MENU videodec_debug_menu[] = {
	{0x01, "dump status",                       hd_videodec_show_status_p,            TRUE},
	//{0x02, "dec info",                          hd_videodec_show_decinfo_p,           TRUE}, // [TODO]
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_videodec_menu(void)
{
	return hd_debug_menu_entry_p(videodec_debug_menu, "VIDEODEC");
}



