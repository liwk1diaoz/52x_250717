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
#include "hd_videoprocess.h"
#define HD_MODULE_NAME HD_VIDEOPROC
#include "hd_int.h"
#include <string.h>

#define IN_COUNT		1
#define OUT_COUNT 	16
#define OUT_PHY_COUNT 5
#define OUT_VPE_COUNT 4

#define HD_VIDEOPROC_PIPE_COMBINE       0x00001000

extern HD_RESULT _hd_videoproc_get_bind_dest(HD_OUT_ID _out_id, HD_IN_ID* p_dest_in_id);
extern HD_RESULT _hd_videoproc_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id);
extern HD_RESULT _hd_videoproc_get_state(HD_OUT_ID _out_id, UINT32* p_state);
extern UINT32 _hd_videoproc_get_3dnr_ref(HD_PATH_ID path_id);
extern UINT32 _hd_videoproc_get_src_out(HD_PATH_ID path_id);
extern UINT32 _hd_videoproc_get_max_out(void);

int _hd_videoproc_dump_info(UINT32 did)
{
	int i;
	// Make path0 = IN_0 - OUT_0
	#define HD_VIDEOPROC_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))
	HD_PATH_ID video_proc_ctrl = HD_VIDEOPROC_PATH(HD_DAL_VIDEOPROC(did), HD_CTRL, HD_CTRL);
	static CHAR  s_str[3][8] = {"OFF","OPEN","START"};
	static CHAR  m_str[3][8] = {"OFF","ON","AUTO"};
	INT32 out_count = (INT32)_hd_videoproc_get_max_out();

		{
			int b_first = 1;
			HD_IN_ID video_src_out_id = 0;
			CHAR  src_out[32] = "";
			_hd_videoproc_get_bind_src(HD_VIDEOPROC_IN(did, 0), &video_src_out_id);
			_hd_src_out_id_str(HD_GET_DEV(video_src_out_id), HD_GET_OUT(video_src_out_id), src_out, 32);
			for (i = 0; i < out_count; i++) {
				UINT32 state = 0;
				HD_IN_ID video_dest_in_id = 0;
				CHAR  dest_in[32] = "";
				_hd_videoproc_get_state(HD_VIDEOPROC_OUT(did, i), &state);
				if (state > 0) {
					if(b_first) {
					_hd_dump_printf("------------------------- VIDEOPROC %-2d PATH & BIND ----------------------------\r\n", did);
					_hd_dump_printf("in    out   state bind_src              bind_dest\r\n");
					b_first = 0;
					}
					_hd_videoproc_get_bind_dest(HD_VIDEOPROC_OUT(did, i), &video_dest_in_id);
					_hd_dest_in_id_str(HD_GET_DEV(video_dest_in_id), HD_GET_IN(video_dest_in_id), dest_in, 32);
					_hd_dump_printf("%-4d  %-4d  %-5s %-20s  %-20s\r\n",
						0, i, s_str[state], src_out, dest_in);
				}
			}
		}

		{
			int b_first = 1;
			for (i = 0; i < out_count; i++) {
				UINT32 state = 0;
				HD_PATH_ID video_proc_path = HD_VIDEOPROC_PATH(HD_DAL_VIDEOPROC(did), HD_IN(0), HD_OUT(i));
				CHAR  pxlfmt[16], dir[8];
				_hd_videoproc_get_state(HD_VIDEOPROC_OUT(did, i), &state);
				if (state > 0) {
					if(b_first) {
						HD_VIDEOPROC_DEV_CONFIG  param_dev_cfg;
						CHAR  pxlfmt[16];
						CHAR  pipe[16];
						hd_videoproc_get(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &param_dev_cfg);
						_hd_video_pxlfmt_str(param_dev_cfg.in_max.pxlfmt, pxlfmt, 16);
						_hd_video_pipe_str(param_dev_cfg.pipe, pipe, 16);

						_hd_dump_printf("------------------------- VIDEOPROC %-2d DEV CONFIG -----------------------------\r\n", did);
						_hd_dump_printf("mode  pipe    isp_id\r\n");
						_hd_dump_printf("      %-6s  %-4d\r\n",
							pipe,
							param_dev_cfg.isp_id
							);
						_hd_dump_printf("ctrl_max   func \r\n");
						_hd_dump_printf("           %08x \r\n",
							param_dev_cfg.ctrl_max.func
							);
						_hd_dump_printf("in_max   w     h     pxlfmt\r\n");
						_hd_dump_printf("         %-4d  %-4d  %-6s\r\n",
							param_dev_cfg.in_max.dim.w,
							param_dev_cfg.in_max.dim.h,
							pxlfmt
							);
					}
					if(b_first) {
						HD_VIDEOPROC_CTRL  param_ctrl;
						HD_VIDEOPROC_LL_CONFIG  param_ll_cfg;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
						HD_VIDEOPROC_FUNC_CONFIG param_path_cfg;
#endif
						UINT32 _3dnr_ref = 0;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
						UINT32 _lowlatency_trig = 0;
#endif
						hd_videoproc_get(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &param_ctrl);
						_3dnr_ref = _hd_videoproc_get_3dnr_ref(param_ctrl.ref_path_3dnr);
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
						hd_videoproc_get(video_proc_ctrl, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &param_path_cfg);
#endif
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
						hd_videoproc_get(video_proc_ctrl, HD_VIDEOPROC_PARAM_LL_CONFIG, &param_ll_cfg);
						_lowlatency_trig = param_ll_cfg.delay_trig_lowlatency;
#endif

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
						_hd_dump_printf("------------------------- VIDEOPROC %-2d CTRL -----------------------------------\r\n", did);
						_hd_dump_printf("ctrl  ddr_id  func      3dnr_ref  lltn_trig\r\n");
						_hd_dump_printf("      %-6d  %08x  %-8d  %-8d\r\n",
							param_path_cfg.ddr_id,
							param_ctrl.func,
							_3dnr_ref,
							_lowlatency_trig
							);
#else
						_hd_dump_printf("------------------------- VIDEOPROC %-2d CTRL -----------------------------------\r\n", did);
						_hd_dump_printf("ctrl  ddr_id  func      3dnr_ref\r\n");
						_hd_dump_printf("      %-6d  %08x  %-8d\r\n",
							0,
							param_ctrl.func,
							_3dnr_ref);
#endif
					}
					if(b_first) {
						HD_VIDEOPROC_IN  param_in;
						HD_VIDEOPROC_CROP param_in_crop;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
						HD_VIDEOPROC_FUNC_CONFIG param_path_cfg;
#endif
						CHAR  pxlfmt[16], dir[8];
						hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_IN, &param_in);
						hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_IN_CROP, &param_in_crop);
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
						hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &param_path_cfg);
#endif
						_hd_video_pxlfmt_str(param_in.pxlfmt, pxlfmt, 16);
						_hd_video_dir_str(param_in.dir, dir, 8);
	 					if (param_in_crop.mode >= HD_CROP_MODE_MAX) {
	 						param_in_crop.mode = 0;
	 					}
						_hd_dump_printf("------------------------- VIDEOPROC %-2d IN FRAME -------------------------------\r\n", did);
						_hd_dump_printf("in    w     h     pxlfmt  frc     dir   crop           ddr_id  func\r\n");
						_hd_dump_printf("%-4d  %-4d  %-4d  %-6s  %-2d/%-2d   %-4s  %s:{%d,%d,%d,%d}  %-6s  %08x\r\n",
							0, param_in.dim.w, param_in.dim.h, pxlfmt,
							GET_HI_UINT16(param_in.frc), GET_LO_UINT16(param_in.frc),
							dir,
							m_str[param_in_crop.mode], param_in_crop.win.rect.x, param_in_crop.win.rect.y, param_in_crop.win.rect.w, param_in_crop.win.rect.h,
							"n/a",
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
							 param_path_cfg.in_func);
#else
							 param_in.func);
#endif
						_hd_dump_printf("------------------------- VIDEOPROC %-2d OUT FRAME ------------------------------\r\n", did);
						_hd_dump_printf("out   w     h     pxlfmt  frc     dir   crop           ddr_id  func/src    depth\r\n");
					}
					b_first = 0;
					if(i < OUT_PHY_COUNT) {
						HD_VIDEOPROC_OUT  param_out;
						HD_VIDEOPROC_CROP param_out_crop;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
						HD_VIDEOPROC_FUNC_CONFIG param_path_cfg;
#endif
						hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &param_out);
						hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_OUT_CROP, &param_out_crop);
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
						hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &param_path_cfg);
#endif
						_hd_video_pxlfmt_str(param_out.pxlfmt, pxlfmt, 16);
						_hd_video_dir_str(param_out.dir, dir, 8);
						if (param_out.frc == 0) {
							param_out.frc = HD_VIDEO_FRC_RATIO(1,1);
						}
						if (param_out_crop.mode >= HD_CROP_MODE_MAX) {
							param_out_crop.mode = 0;
						}
						_hd_dump_printf("%-4d  %-4d  %-4d  %-6s  %-2d/%-2d   %-4s  %s:{%d,%d,%d,%d}  %-6d  %08x:%-1d  %-4d\r\n",
							i, param_out.dim.w, param_out.dim.h, pxlfmt,
							GET_HI_UINT16(param_out.frc), GET_LO_UINT16(param_out.frc),
							dir,
							m_str[param_out_crop.mode], param_out_crop.win.rect.x, param_out_crop.win.rect.y, param_out_crop.win.rect.w, param_out_crop.win.rect.h,
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
							param_path_cfg.ddr_id,
#else
							0,
#endif
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
							param_path_cfg.out_func, 0, param_out.depth);
#else
							param_out.func, 0, param_out.depth);
#endif
					} else {
						HD_VIDEOPROC_OUT_EX  param_out;
						HD_VIDEOPROC_CROP param_out_crop;
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
						HD_VIDEOPROC_FUNC_CONFIG param_path_cfg;
#endif
						UINT32 src_out = 0;
						hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_OUT_EX, &param_out);
						hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_OUT_EX_CROP, &param_out_crop);
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
						hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &param_path_cfg);
#endif
						_hd_video_pxlfmt_str(param_out.pxlfmt, pxlfmt, 16);
						_hd_video_dir_str(param_out.dir, dir, 8);
						if (param_out.frc == 0) {
							param_out.frc = HD_VIDEO_FRC_RATIO(1,1);
						}
						if (param_out_crop.mode >= HD_CROP_MODE_MAX) {
							param_out_crop.mode = 0;
						}
						src_out = _hd_videoproc_get_src_out(param_out.src_path);
						_hd_dump_printf("%-4d  %-4d  %-4d  %-6s  %-2d/%-2d   %-4s  %s:{%d,%d,%d,%d}  %-6d  %08x:%-1d  %-4d\r\n",
							i, param_out.dim.w, param_out.dim.h, pxlfmt,
							GET_HI_UINT16(param_out.frc), GET_LO_UINT16(param_out.frc),
							dir,
							m_str[param_out_crop.mode], param_out_crop.win.rect.x, param_out_crop.win.rect.y, param_out_crop.win.rect.w, param_out_crop.win.rect.h,
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
							param_path_cfg.ddr_id,
#else
							0,
#endif
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
							param_path_cfg.out_func, src_out, param_out.depth);
#else
							0, src_out, param_out.depth);
#endif
					}
				}
			}
			/*
			if(b_first) {
				_hd_dump_printf("------------------------- VIDEOPROC %-2d DEV CONFIG -----------------------------\r\n", did);
				_hd_dump_printf("mode  pipe    isp_id\r\n");
				_hd_dump_printf("ctrl_max   func 3dnr_ref\r\n");
				_hd_dump_printf("in_max   w     h     pxlfmt\r\n");
				_hd_dump_printf("------------------------- VIDEOPROC %-2d CTRL -----------------------------------\r\n", did);
				_hd_dump_printf("ctrl   func 3dnr_ref\r\n");
				_hd_dump_printf("------------------------- VIDEOPROC %-2d IN FRAME -------------------------------\r\n", did);
				_hd_dump_printf("in    w     h     pxlfmt  frc   dir   crop func\r\n");
				_hd_dump_printf("------------------------- VIDEOPROC %-2d OUT FRAME ------------------------------\r\n", did);
				_hd_dump_printf("out   w     h     pxlfmt  frc   dir   crop func  depth\r\n");
			}*/
		}
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		{
			int b_first = 1;
			for (i = 0; i < OUT_VPE_COUNT; i++) {
				UINT32 state = 0;
				HD_PATH_ID video_proc_path = HD_VIDEOPROC_PATH(HD_DAL_VIDEOPROC(did), HD_IN(0), HD_OUT(i));
				HD_VIDEOPROC_DEV_CONFIG  param_dev_cfg;

				_hd_videoproc_get_state(HD_VIDEOPROC_OUT(did, i), &state);
				hd_videoproc_get(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &param_dev_cfg);
				if (state > 0 && param_dev_cfg.pipe == HD_VIDEOPROC_PIPE_VPE) {
					if(b_first) {
						_hd_dump_printf("------------------------- VIDEOPROC %-2d OUT FRAME VPE ---------------------------\r\n", did);
						_hd_dump_printf("out   bg.w  bg.h  x     y     w     h     in_crop_psr             out_crop_psr\r\n");
					}
					HD_VIDEOPROC_CROP param_in_crop_psr;
					HD_VIDEOPROC_OUT  param_out;
					HD_VIDEOPROC_CROP param_out_crop_psr;
					hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &param_out);
					hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_IN_CROP_PSR, &param_in_crop_psr);
					hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_OUT_CROP_PSR, &param_out_crop_psr);
					if (param_out.frc == 0) {
						param_out.frc = HD_VIDEO_FRC_RATIO(1,1);
					}
					if (param_in_crop_psr.mode >= HD_CROP_MODE_MAX) {
						param_in_crop_psr.mode = 0;
					}
					if (param_out_crop_psr.mode >= HD_CROP_MODE_MAX) {
						param_out_crop_psr.mode = 0;
					}
					_hd_dump_printf("%-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-3s:{%3d,%3d,%4d,%4d} %s:{%d,%d,%d,%d}\r\n",
						i, (int)param_out.bg.w, (int)param_out.bg.h,
						(int)param_out.rect.x, (int)param_out.rect.y, (int)param_out.rect.w, (int)param_out.rect.h,
						m_str[param_in_crop_psr.mode], param_in_crop_psr.win.rect.x, param_in_crop_psr.win.rect.y, param_in_crop_psr.win.rect.w, param_in_crop_psr.win.rect.h,
						m_str[param_out_crop_psr.mode], param_out_crop_psr.win.rect.x, param_out_crop_psr.win.rect.y, param_out_crop_psr.win.rect.w, param_out_crop_psr.win.rect.h);
					b_first = 0;
				}
			}

		}
#endif
		{
			int b_first = 1;
			for (i = 0; i < 1; i++) {
				UINT32 state = 0;
				HD_PATH_ID video_proc_path = HD_VIDEOPROC_PATH(HD_DAL_VIDEOPROC(did), HD_IN(0), HD_OUT(i));
				HD_VIDEOPROC_DEV_CONFIG  param_dev_cfg;

				_hd_videoproc_get_state(HD_VIDEOPROC_OUT(did, i), &state);
				hd_videoproc_get(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &param_dev_cfg);
				if (state > 0 && param_dev_cfg.pipe & HD_VIDEOPROC_PIPE_COMBINE) {
					if(b_first) {
						_hd_dump_printf("------------------------- VIDEOPROC %-2d COMBINE FRAME ---------------------------\r\n", did);
						_hd_dump_printf("out   bg.w  bg.h  x     y     w     h   \r\n");
					}
					HD_VIDEOPROC_OUT  param_out;
					hd_videoproc_get(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &param_out);
					if (param_out.frc == 0) {
						param_out.frc = HD_VIDEO_FRC_RATIO(1,1);
					}
					_hd_dump_printf("%-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d \r\n",
						i, (int)param_out.bg.w, (int)param_out.bg.h,
						(int)param_out.rect.x, (int)param_out.rect.y, (int)param_out.rect.w, (int)param_out.rect.h);
					b_first = 0;
				}
			}

		}
	return 0;
}

int _hd_videoproc_show_status_p(void)
{
	char cmd[256];

	snprintf(cmd, 256, "cat /proc/hdal/vprc/info");
	system(cmd);
	return 0;
}

static HD_DBG_MENU videoproc_debug_menu[] = {
	{0x01, "dump info",                         _hd_videoproc_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_videoproc_menu(void)
{
	return hd_debug_menu_entry_p(videoproc_debug_menu, "VIDEOPROC");
}



