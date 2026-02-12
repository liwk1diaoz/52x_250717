/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_videoenc_menu.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hdal.h"
#include "hd_util.h"
#include "hd_debug_menu.h"
#include "hd_videoout.h"
#define HD_MODULE_NAME HD_VIDEOOUT
#include "hd_int.h"
#include <string.h>

#define VIDEOOUT_DEVICE_NUM 2
#define IN_COUNT 	1
#define OUT_COUNT 	1
#define FB_COUNT 	1


extern HD_RESULT _hd_videoout_get_bind_src(HD_IN_ID _in_id, HD_IN_ID* p_src_out_id);
extern HD_RESULT _hd_videoout_get_state(HD_OUT_ID _out_id, UINT32* p_state);
extern int _hd_videoout_type_str(HD_COMMON_VIDEO_OUT_TYPE type, CHAR *p_str, INT str_len);


int _hd_videoout_dump_info(UINT32 did)
{
	// Make path0 = IN_0 - OUT_0
	#define HD_VIDEOOUT_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))
	HD_PATH_ID video_out_ctrl = 0;
	HD_PATH_ID video_out_path = 0;
	INT32 outpath =0;
	int ret1,ret2 = 0;
	static CHAR  s_str[3][8] = {"OFF","OPEN","START"};
    //for (did = 0; did < VIDEOOUT_DEVICE_NUM; did++)
    {
    	{
    		HD_IN_ID video_src_out_id = 0;
    		CHAR  src_out[32];
   			UINT32 state = 0;
        	video_out_ctrl = HD_VIDEOOUT_PATH(HD_DAL_VIDEOOUT(did), HD_CTRL, HD_CTRL);
        	video_out_path = HD_VIDEOOUT_PATH(HD_DAL_VIDEOOUT(did), HD_IN(0), HD_OUT(0));

    		_hd_videoout_get_bind_src(HD_VIDEOOUT_IN(did, 0), &video_src_out_id);
    		_hd_src_out_id_str(HD_GET_DEV(video_src_out_id), HD_GET_OUT(video_src_out_id), src_out, 32);
    		for (outpath=0; outpath<OUT_COUNT; outpath++) {

    			_hd_videoout_get_state(HD_VIDEOOUT_OUT(did, outpath), &state);
    			if (state > 0) {
                    if(outpath==0) {
            		_hd_dump_printf("------------------------- VIDEOOUT %-2d PATH & BIND -----------------------------\r\n", did);
            		_hd_dump_printf("in    out   state bind_src              bind_dest\r\n");
                    }

    			    _hd_dump_printf("%-4d  %-4d  %-5s %-20s  %-20d\r\n",
    				0, outpath, s_str[state], src_out, 0);
    			} else{
    			    return -1;
    			}
    		}

    	}
    	{
    		HD_VIDEOOUT_MODE  param_dev_mode;
    		HD_VIDEOOUT_SYSCAPS video_out_syscaps = {0};
    		CHAR  type[16];

    		_hd_dump_printf("------------------------- VIDEOOUT %-2d DEV CONFIG ------------------------------\r\n", did);
    		_hd_dump_printf("max   w     h     type\r\n");

    		ret1 = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_MODE, &param_dev_mode);
    	    ret2 = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, &video_out_syscaps);
    	    if(ret1 ==0) {
    			ret1 = _hd_videoout_type_str(param_dev_mode.output_type, type, 16);
    		}
    		if((ret1==0)&&(ret2==0)) {
    			_hd_dump_printf("%-4d  %-4d  %-4d  %-6s\r\n",
    			0,
    			video_out_syscaps.output_dim.w,
    			video_out_syscaps.output_dim.h,
    			type
    			);
    		}
    	}

    	{
    		HD_VIDEOOUT_IN  param_in;
            HD_VIDEOOUT_FUNC_CONFIG videoout_cfg;
    		CHAR  pxlfmt[16]={0}, dir[8]={0};

    		_hd_dump_printf("------------------------- VIDEOOUT %-2d IN FRAME --------------------------------\r\n", did);
    		_hd_dump_printf("in    w     h     pxlfmt  dir   ddr_id  func\r\n");

        	ret1 = hd_videoout_get(video_out_path, HD_VIDEOOUT_PARAM_FUNC_CONFIG, &videoout_cfg);
        	if(ret1!=HD_OK)
        		_hd_dump_printf("func err:%d\r\n",ret1);

    		ret1 = hd_videoout_get(video_out_path, HD_VIDEOOUT_PARAM_IN, &param_in);
    		if(ret1 ==0) {
    			ret1 = _hd_video_pxlfmt_str(param_in.pxlfmt, pxlfmt, 16);
                if(ret1!=0)
                    _hd_dump_printf("pxlfmt %x err:%d\r\n",param_in.pxlfmt,ret1);

    			ret2 = _hd_video_dir_str(param_in.dir, dir, 8);
    		}
    		if(ret2!=0) {
    			_hd_dump_printf("%-4d  %-4d  %-4d  %-6s  %-4s     %d    0x%08x\r\n",
    			0, param_in.dim.w, param_in.dim.h, pxlfmt, dir,videoout_cfg.ddr_id,videoout_cfg.in_func);
    		}

    	}

        {
    		HD_VIDEOOUT_WIN_ATTR  param_win;

    		_hd_dump_printf("------------------------- VIDEOOUT %-2d IN WIN ----------------------------------\r\n", did);
    		_hd_dump_printf("visible  x     y     w     h     \r\n");

    		ret1 = hd_videoout_get(video_out_path, HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &param_win);
    		if(ret1==0&&param_win.visible) {
    			_hd_dump_printf("%-7d  %-4d  %-4d  %-4d  %-4d  \r\n",
    			param_win.visible, param_win.rect.x, param_win.rect.y, param_win.rect.w,param_win.rect.h);
    		}
    	}


        {
            UINT32 fb_id=0;
        	HD_FB_FMT fb_fmt={0};
            HD_FB_ATTR fb_attr ={0};
            HD_FB_ENABLE fb_en ={0};
            CHAR  pxlfmt[16]={0};
            for(fb_id=0;fb_id<FB_COUNT ;fb_id++) {
        		_hd_dump_printf("------------------------- VIDEOOUT %-2d FB%d -------------------------------------\r\n", did,fb_id);
        		_hd_dump_printf("enable  pxlfmt    blend  alpha_1555  ckey_enable  color_key\r\n");

                fb_fmt.fb_id = fb_id;
                fb_fmt.fb_id = fb_id;
                fb_attr.fb_id = fb_id;
                ret1 = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_ENABLE, &fb_en);
                if((ret1==0)&&(fb_en.enable)) {
            		ret1 = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_FMT, &fb_fmt);
                    if(ret1==0){
                        ret1 = _hd_video_pxlfmt_str(fb_fmt.fmt, pxlfmt, 16);
                        if(ret1!=0)
                            _hd_dump_printf("pxlfmt %x err:%d\r\n",fb_fmt.fmt,ret1);
                    }
                    ret2 = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_FB_ATTR, &fb_attr);
                    if(ret2!=0)
                        _hd_dump_printf("attr fail %d\r\n",ret2);
                }

                if(fb_en.enable) {
        			_hd_dump_printf("%-6d  %-8s  %-5d  %-10d  %-11d  0x%08X\r\n",
        			fb_en.enable,pxlfmt,fb_attr.alpha_blend,fb_attr.alpha_1555,fb_attr.colorkey_en,
        			(fb_attr.r_ckey&0xFF)<<16|(fb_attr.g_ckey&0xFF)<<8|(fb_attr.b_ckey&0xFF));
                }
            }
    	}

        if(did < VIDEOOUT_DEVICE_NUM-1)
            _hd_dump_printf("\r\n");
    }
	return 0;
}
static int hd_videoout_show_status_p(void)
{
	char cmd[256];

	snprintf(cmd, 256, "cat /proc/hdal/vout/info");
	system(cmd);

	return 0;
}

static HD_DBG_MENU videoout_debug_menu[] = {
	{0x01, "dump status",                         hd_videoout_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_videoout_menu(void)
{
	return hd_debug_menu_entry_p(videoout_debug_menu, "VIDEOOUT");
}



