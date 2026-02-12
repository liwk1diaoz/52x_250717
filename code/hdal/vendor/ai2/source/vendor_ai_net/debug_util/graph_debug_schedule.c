/**
	@brief Source file of graph debug schedule.

	@file graph_debug_schedule.c

	@ingroup graph_debug

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "graph_debug_core.h"
#include "graph_debug_schedule.h"
#if defined(__LINUX) && defined(__KERNEL__)
//=============================================================
#define __CLASS__ 				"[ai][dump][schd]"
#include "kflow_ai_debug.h"
//=============================================================
#elif defined(__FREERTOS_KFLOW)
//=============================================================
#define __CLASS__ 				"[ai][dump][schd]"
#include "kflow_ai_debug.h"
//=============================================================
#else //__FREERTOS
//=============================================================
#define __CLASS__ 				"[ai][dump][schd]"
#include "vendor_ai_debug.h"
//=============================================================
#endif

#if defined(__LINUX)
#include <linux/kernel.h>   // for snprintf
#elif defined(__FREERTOS)
#include <stdio.h>          // for snprintf
#else
#include <stdio.h>          // for snprintf
#endif

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                         */
/*-----------------------------------------------------------------------------*/

INT _graph_debug_schedule_get_color_and_x(SCHE_ENG eng, CHAR *color_name, UINT max_color_name_len, UINT32 *x1, UINT32 *x2)
{
	switch (eng) {
		case SCHE_ENG_NUE2:
			snprintf(color_name, max_color_name_len, "#EE77EE"); // purple
			*x1 = 10;  // 10~30;
			*x2 = 30;
			break;
		case SCHE_ENG_CNN:
			snprintf(color_name, max_color_name_len, "#2288FF"); // blue
			*x1 = 50;  // 50~70;
			*x2 = 70;
			break;
		case SCHE_ENG_CNN2:
			snprintf(color_name, max_color_name_len, "#2288FF"); // blue
			*x1 = 90;  // 90~110;
			*x2 = 110;
			break;
		case SCHE_ENG_NUE:
			snprintf(color_name, max_color_name_len, "#00FF00"); // green
			*x1 = 130;  // 130~150;
			*x2 = 150;
			break;
		case SCHE_ENG_CPU:
			snprintf(color_name, max_color_name_len, "#CCCCCC"); // gray
			*x1 = 170;  // 170~190;
			*x2 = 190;
			break;
		default:
			break;
	}
	return 0;
}

INT _graph_debug_schedule_get_x_center(SCHE_ENG eng, UINT32 *x_center)
{
	switch (eng) {
		case SCHE_ENG_NUE2:  *x_center =  20;    break;
		case SCHE_ENG_CNN:   *x_center =  60;    break;
		case SCHE_ENG_CNN2:  *x_center = 100;    break;
		case SCHE_ENG_NUE:   *x_center = 140;    break;
		case SCHE_ENG_CPU:   *x_center = 180;    break;
		default:
			break;
	}
	return 0;
}

INT _graph_debug_schedule_get_x_shift(SCHE_ENG eng, INT32 *x_side)
{
	#define next_cnt(cnt) (((cnt+1)>= SCHE_ENG_MAX)? 0: cnt+1)
	static UINT cnt[SCHE_ENG_MAX] = {0};
	const INT shift_fix[7] = {13, 19, 25, 16, 22, 28, 34};

	switch (eng) {
		case SCHE_ENG_NUE2:
		case SCHE_ENG_CNN2:
		case SCHE_ENG_NUE:
		case SCHE_ENG_DSP:
		case SCHE_ENG_CPU:
			*x_side  = shift_fix[cnt[eng]];
			break;

		case SCHE_ENG_CNN:
		default:
			*x_side  = shift_fix[cnt[eng]] * (-1);
			break;
	}
	cnt[eng] = next_cnt(cnt[eng]);

	return 0;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

INT graph_debug_schedule_open(CHAR *filepath, GRAPH_DEBUG_SCHEDULE_HANDLER *handler)
{
	GRAPH_DEBUG_CORE_OPEN_CFG open_cfg = {0};
	
	graph_debug_core_set_default_open_cfg(&open_cfg);
	open_cfg.reverse_y           = 0;
	open_cfg.target_min_w        = 150;
	return graph_debug_core_open(&open_cfg, filepath, handler);
}

INT graph_debug_schedule_add_block(GRAPH_DEBUG_SCHEDULE_HANDLER handler, SCHE_ENG eng, UINT32 start_t, UINT32 end_t, CHAR *block_name, UINT max_block_name_len)
{
	CHAR color_name[8]="";
	UINT32 x1=0, x2=0;
	
	_graph_debug_schedule_get_color_and_x(eng, color_name, 8, &x1, &x2);
	return graph_debug_core_add_block(handler, x1, x2, start_t, end_t, block_name, max_block_name_len, color_name, 8);
}

INT graph_debug_schedule_add_connection(GRAPH_DEBUG_SCHEDULE_HANDLER handler, SCHE_ENG eng_src, UINT32 end_t, SCHE_ENG eng_dst, UINT32 start_t)
{
	CHAR color_name[8]="";
	INT rv = 0;

	if (eng_src != eng_dst) {
		// diff engine, we draw line middle-to-middle
		UINT32 x_src=0, x_dst=0;

		_graph_debug_schedule_get_x_center(eng_src, &x_src);
		_graph_debug_schedule_get_x_center(eng_dst, &x_dst);

		snprintf(color_name, 8, "%s", "#55CC33");
		rv = graph_debug_core_add_line(handler, x_src, end_t, x_dst, start_t, color_name, 8);
	} else {
		// same engine, we draw line on side
		UINT32 x_center=0;
		INT32 x_shift=0, shift_y=0;

		_graph_debug_schedule_get_x_center(eng_src, &x_center);
		_graph_debug_schedule_get_x_shift(eng_src, &x_shift);

		shift_y = (start_t - end_t) / 10;

		snprintf(color_name, 8, "%s", "#55CC33");

		rv = graph_debug_core_add_line(handler, x_center, end_t, x_center+x_shift, end_t+shift_y, color_name, 8);
		if (rv !=0) goto exit;
		rv = graph_debug_core_add_line(handler, x_center+x_shift, end_t+shift_y, x_center+x_shift, start_t-shift_y, color_name, 8);
		if (rv !=0) goto exit;
		rv = graph_debug_core_add_line(handler, x_center+x_shift, start_t-shift_y, x_center, start_t, color_name, 8);
	}
exit:
	return rv;
}

INT graph_debug_schedule_set_var_start_t(GRAPH_DEBUG_SCHEDULE_HANDLER handler, CHAR *var_name, UINT max_var_name_len, UINT32 value)
{
	return graph_debug_core_set_var(handler, var_name, max_var_name_len, value);
}

INT graph_debug_schedule_add_block_with_var_start_t(GRAPH_DEBUG_SCHEDULE_HANDLER handler, SCHE_ENG eng, CHAR *var_name, UINT max_var_name_len, UINT32 end_t, CHAR *block_name, UINT max_block_name_len)
{
	CHAR color_name[8]="";
	UINT32 x1=0, x2=0;
	
	_graph_debug_schedule_get_color_and_x(eng, color_name, 8, &x1, &x2);
	return graph_debug_core_add_block_with_var_y1(handler, x1, x2, var_name, max_var_name_len, end_t, block_name, max_block_name_len, color_name, 8);
}

INT graph_debug_schedule_close(GRAPH_DEBUG_SCHEDULE_HANDLER handler)
{
	GRAPH_DEBUG_CORE_CLOSE_CFG close_cfg = {0};
	CHAR x_name[16] = "eng";
	CHAR y_name[16] = "time";

	close_cfg.x_axis.axis_name         = x_name;
	close_cfg.x_axis.max_axis_name_len = 16;
	close_cfg.x_axis.b_show_range      = 0;

	close_cfg.y_axis.axis_name         = y_name;
	close_cfg.y_axis.max_axis_name_len = 16;
	close_cfg.y_axis.b_show_range      = 1;
	
	return graph_debug_core_close(handler, &close_cfg);
}

