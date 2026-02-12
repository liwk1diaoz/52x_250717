/**
	@brief Source file of graph debug core.

	@file graph_debug_core.c

	@ingroup graph_debug

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "kwrap/nvt_type.h"
#include "kwrap/error_no.h"
#include "kwrap/stdio.h"
#include "kwrap/file.h"

#if defined(__LINUX) && defined(__KERNEL__)
//=============================================================
#define __CLASS__ 				"[ai][dump][draw]"
#include "kflow_ai_debug.h"
//=============================================================
#elif defined(__FREERTOS_KFLOW)
//=============================================================
#define __CLASS__ 				"[ai][dump][draw]"
#include "kflow_ai_debug.h"
//=============================================================
#else //__FREERTOS
//=============================================================
#define __CLASS__ 				"[ai][dump][draw]"
#include "vendor_ai_debug.h"
//=============================================================
#endif

#include "graph_debug_core.h"

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
#define vos_file_fprintf(fd, fmtstr, args...) \
	do { \
		int len;\
		char tmp_buf[512]; \
		len = snprintf(tmp_buf, sizeof(tmp_buf), fmtstr, ##args); \
		vos_file_write(fd, (void *)tmp_buf, len); \
	} while(0);
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

BOOL _graph_debug_core_chk_zero_cfg(GRAPH_DEBUG_CORE_OPEN_CFG *open_cfg)
{
	return (open_cfg->top_margin == 0 &&
		open_cfg->bottom_margin == 0 &&
		open_cfg->left_margin == 0 &&
		open_cfg->right_margin == 0 &&
		open_cfg->target_min_w == 0 &&
		open_cfg->target_min_h == 0 &&
		open_cfg->reverse_y == 0 &&
		open_cfg->show_detail_range_x == 0 &&
		open_cfg->show_detail_range_y == 0 &&
		open_cfg->show_draw_line == 0 &&
		open_cfg->max_canvas_width == 0 &&
		open_cfg->max_canvas_height == 0 )? TRUE:FALSE;
}

void _graph_debug_core_write_file_head(VOS_FILE fd, GRAPH_DEBUG_CORE_OPEN_CFG *open_cfg)
{
	vos_file_fprintf(fd, \
		"<html>\r\n"
		" <head>\r\n"
		"    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\r\n"
		" </head>\r\n"
		"<body>\r\n"
		"\r\n"
		"<canvas id=\"myCanvas\"></canvas>\r\n"
		"\r\n"
		"<script type=\"text/javascript\">\r\n"
		"//---------------- settings -----------------\r\n");

	vos_file_fprintf(fd, \
		"let TOP_MARGIN = %lu;  // code gen\r\n"
		"let BOTTOM_MARGIN = %lu; // code gen\r\n"
		"let LEFT_MARGIN = %lu; // code gen\r\n"
		"let RIGHT_MARGIN = %lu; // code gen\r\n"
		"\r\n"
		"let TARGET_MIN_W = %lu; // code gen\r\n"
		"let TARGET_MIN_H = %lu; // code gen\r\n"
		"let REVERSE_Y = %lu;    // code gen // 0: disable, 1: enable\r\n"
		"\r\n",
		(unsigned long)open_cfg->top_margin,
		(unsigned long)open_cfg->bottom_margin,
		(unsigned long)open_cfg->left_margin,
		(unsigned long)open_cfg->right_margin,
		(unsigned long)open_cfg->target_min_w,
		(unsigned long)open_cfg->target_min_h,
		(unsigned long)open_cfg->reverse_y);

	vos_file_fprintf(fd, \
		"let SHOW_DETAIL_RANGE_X = %lu; // code gen // 0: disable, 1: enable\r\n"
		"let SHOW_DETAIL_RANGE_Y = %lu; // code gen // 0: disable, 1: enable\r\n"
		"\r\n"
		"let SHOW_DRAW_LINE      = %lu; // code gen // 0: disable, 1: enable\r\n"
		"\r\n"
		"let MAX_CANVAS_WIDTH  = %lu;  // code gen  // depend on browser\r\n"
		"let MAX_CANVAS_HEIGHT = %lu;  // code gen  // depend on browser\r\n",
		(unsigned long)open_cfg->show_detail_range_x,
		(unsigned long)open_cfg->show_detail_range_y,
		(unsigned long)open_cfg->show_draw_line,
		(unsigned long)open_cfg->max_canvas_width,
		(unsigned long)open_cfg->max_canvas_height);

	vos_file_fprintf(fd, \
		"//---------------- variables -----------------\r\n"
		"let canvas = document.getElementById(\"myCanvas\");\r\n"
		"let ctx = canvas.getContext(\"2d\");\r\n"
		"\r\n"
		"let scale_factor_w_base = 0;\r\n"
		"let scale_factor_w_incr = 0;\r\n"
		"let scale_factor_h_base = 0;\r\n"
		"let scale_factor_h_incr = 0;\r\n"
		"\r\n"
		"let min_x=0xffffffff;\r\n"
		"let min_y=0xffffffff;\r\n"
		"let max_x=0;\r\n"
		"let max_y=0;\r\n"
		"let min_w=0xffffffff;\r\n"
		"let min_h=0xffffffff;\r\n"
		"\r\n"
		"let total_width=0;\r\n"
		"let total_height=0;\r\n");

	vos_file_fprintf(fd, \
		"//---------------- function -----------------\r\n"
		"\r\n"
		"function drawRect_color_exe(x, y, w, h, text, color, o_x1, o_x2, o_y1, o_y2) {\r\n"
		"	ctx.beginPath()\r\n"
		"	ctx.lineWidth = 3;              // border\r\n"
		"	ctx.strokeStyle = \"#000000\"     // border color\r\n"
		"	ctx.fillStyle = color           // fill color\r\n"
		"	ctx.rect(x,y,w,h);\r\n"
		"	ctx.closePath()\r\n"
		"	ctx.fill()\r\n"
		"	ctx.stroke()\r\n"
		"\r\n"
		"	//--- rect text ---\r\n"
		"	ctx.lineWidth = 2; \r\n"
		"	text_len = text.length;\r\n"
		"	ctx.font = \"18px Arial\";\r\n");

	vos_file_fprintf(fd, \
		"	if (REVERSE_Y) {\r\n"
		"		ctx.strokeText(text, x+w/2-(5*text_len), y+h/2+5);	// on center\r\n"
		"	} else {\r\n"
		"		ctx.strokeText(text, x+w/2-(5*text_len), y+h/2-5);	// on center\r\n"
		"	}\r\n"
		"\r\n"
		"	//--- X detail range ---\r\n"
		"	if (SHOW_DETAIL_RANGE_X) {\r\n"
		"		x1_text = String(o_x1);\r\n"
		"		x1_text_len = x1_text.length;\r\n"
		"		ctx.font = \"18px Arial\";\r\n"
		"		x2_text = String(o_x2);\r\n"
		"		x2_text_len = x2_text.length;\r\n"
		"		ctx.font = \"18px Arial\";\r\n");

	vos_file_fprintf(fd, \
		"		if (REVERSE_Y) {\r\n"
		"			ctx.strokeStyle = \"#0000FF\"\r\n"
		"			ctx.strokeText(x1_text, x-5, y-50);\r\n"
		"			ctx.strokeText(x2_text, x+w+2, y-30);\r\n"
		"		} else {\r\n"
		"			ctx.strokeStyle = \"#0000FF\"\r\n"
		"			ctx.strokeText(x1_text, x-5, y+30);\r\n"
		"			ctx.strokeText(x2_text, x+w+2, y+50);\r\n"
		"		}\r\n"
		"	}\r\n"
		"	\r\n"
		"	//--- Y detail range ---\r\n"
		"	if (SHOW_DETAIL_RANGE_Y) {\r\n"
		"		y1_text = String(o_y1);\r\n"
		"		y1_text_len = y1_text.length;\r\n"
		"		ctx.font = \"18px Arial\";\r\n"
		"		y2_text = String(o_y2);\r\n"
		"		y2_text_len = y2_text.length;\r\n"
		"		ctx.font = \"18px Arial\";\r\n");

	vos_file_fprintf(fd, \
		"		if (REVERSE_Y) {\r\n"
		"			ctx.strokeStyle = \"#FF00FF\"\r\n"
		"			ctx.strokeText(y1_text, x+w/2-(5*y1_text_len), y-10);\r\n"
		"			ctx.strokeText(y2_text, x+w/2-(5*y2_text_len), y+h+20);\r\n"
		"		} else {\r\n"
		"			ctx.strokeStyle = \"#FF00FF\"\r\n"
		"			ctx.strokeText(y1_text, x+w/2-(5*y1_text_len), y+15);\r\n"
		"			ctx.strokeText(y2_text, x+w/2-(5*y2_text_len), y+h-15);\r\n"
		"		}\r\n"
		"	}\r\n"
		"}\r\n"
		"\r\n");

	vos_file_fprintf(fd, \
		"function drawRect_color(x1, x2, y1, y2, text, color) {\r\n"
		"	o_x1 = x1;\r\n"
		"	o_x2 = x2;\r\n"
		"	o_y1 = y1;\r\n"
		"	o_y2 = y2;\r\n"
		"	\r\n"
		"	x1 = (x1-min_x);\r\n"
		"	x2 = (x2-min_x);\r\n"
		"	y1 = (y1-min_y);\r\n"
		"	y2 = (y2-min_y);\r\n"
		"\r\n"
		"	if (REVERSE_Y) {\r\n"
		"		y1 = total_height - y1;\r\n"
		"		y2 = total_height - y2;\r\n"
		"	}\r\n"
		"\r\n");

	vos_file_fprintf(fd, \
		"	x1 = x1 * scale_factor_w_base/scale_factor_w_incr   + LEFT_MARGIN;\r\n"
		"	x2 = x2 * scale_factor_w_base/scale_factor_w_incr   + LEFT_MARGIN;\r\n"
		"	y1 = y1 * scale_factor_h_base/scale_factor_h_incr   + TOP_MARGIN;\r\n"
		"	y2 = y2 * scale_factor_h_base/scale_factor_h_incr   + TOP_MARGIN;\r\n"
		"\r\n"
		"	x = x1;\r\n"
		"	y = y1;\r\n"
		"	w = x2-x1;\r\n"
		"	h = y2-y1;\r\n"
		"\r\n"
		"	drawRect_color_exe(x, y, w, h, text, color, o_x1, o_x2, o_y1, o_y2);\r\n"
		"}\r\n"
		"\r\n");

	vos_file_fprintf(fd, \
		"function update_var(x1, x2, y1, y2) {\r\n"
		"    w = (x2-x1);\r\n"
		"    h = (y2-y1);\r\n"
		"	\r\n"
		"    min_x = (min_x > x1)? x1:min_x;\r\n"
		"    max_x = (max_x < x2)? x2:max_x;\r\n"
		"    min_y = (min_y > y1)? y1:min_y;\r\n"
		"    max_y = (max_y < y2)? y2:max_y;\r\n"
		"    min_w = (min_w >  w)?  w:min_w;\r\n"
		"    min_h = (min_h >  h)?  h:min_h;\r\n"
		"}\r\n"
		"\r\n"
		"function drawRectMain(x1, x2, y1, y2, text, color, update_only) {\r\n"
		"	if (update_only) {\r\n"
		"		update_var(x1, x2, y1, y2);\r\n"
		"	} else {\r\n"
		"		drawRect_color(x1, x2, y1, y2, text, color);\r\n"
		"	}\r\n"
		"}\r\n"
		"\r\n");

	vos_file_fprintf(fd, \
		"//-----------------------\r\n"
		"function drawLine_color_exe(x1, y1, x2, y2, color) {\r\n"
		"	ctx.beginPath()\r\n"
		"	ctx.lineWidth = 3;\r\n"
		"	ctx.strokeStyle = color\r\n"
		"	ctx.moveTo(x1, y1);\r\n"
		"	ctx.lineTo(x2, y2);\r\n"
		"	ctx.stroke()\r\n"
		"}\r\n"
		"\r\n");

	vos_file_fprintf(fd, \
		"function drawLine_color(x1, y1, x2, y2, color) {\r\n"
		"	x1 = (x1-min_x);\r\n"
		"	x2 = (x2-min_x);\r\n"
		"	y1 = (y1-min_y);\r\n"
		"	y2 = (y2-min_y);\r\n"
		"\r\n"
		"	if (REVERSE_Y) {\r\n"
		"		y1 = total_height - y1;\r\n"
		"		y2 = total_height - y2;\r\n"
		"	}\r\n"
		"\r\n");

	vos_file_fprintf(fd, \
		"	x1 = x1 * scale_factor_w_base/scale_factor_w_incr   + LEFT_MARGIN;\r\n"
		"	x2 = x2 * scale_factor_w_base/scale_factor_w_incr   + LEFT_MARGIN;\r\n"
		"	y1 = y1 * scale_factor_h_base/scale_factor_h_incr   + TOP_MARGIN;\r\n"
		"	y2 = y2 * scale_factor_h_base/scale_factor_h_incr   + TOP_MARGIN;\r\n"
		"\r\n"
		"	if (SHOW_DRAW_LINE) {\r\n"
		"		drawLine_color_exe(x1, y1, x2, y2, color);\r\n"
		"	}\r\n"
		"}\r\n"
		"\r\n");

	vos_file_fprintf(fd, \
		"function draw_axis(x_title, y_title) {\r\n"
		"	x1 = min_x;\r\n"
		"	x2 = max_x;\r\n"
		"	y1 = min_y;\r\n"
		"	y2 = max_y;\r\n"
		"\r\n"
		"	x1 = (x1-min_x);\r\n"
		"	x2 = (x2-min_x);\r\n"
		"	y1 = (y1-min_y);\r\n"
		"	y2 = (y2-min_y);\r\n"
		"\r\n"
		"	if (REVERSE_Y) {\r\n"
		"		y1 = total_height - y1;\r\n"
		"		y2 = total_height - y2;\r\n"
		"	}\r\n"
		"\r\n");

	vos_file_fprintf(fd, \
		"	x1 = x1 * scale_factor_w_base/scale_factor_w_incr   + LEFT_MARGIN;\r\n"
		"	x2 = x2 * scale_factor_w_base/scale_factor_w_incr   + LEFT_MARGIN;\r\n"
		"	y1 = y1 * scale_factor_h_base/scale_factor_h_incr   + TOP_MARGIN;\r\n"
		"	y2 = y2 * scale_factor_h_base/scale_factor_h_incr   + TOP_MARGIN;\r\n"
		"\r\n"
		"	// x-axis\r\n"
		"	drawLine_color_exe(x1, y1, x2, y1, \"#000000\");\r\n"
		"\r\n"
		"	// x-title\r\n"
		"	ctx.font = \"30px Arial\";\r\n"
		"	ctx.strokeText(x_title, x2+20, y1+10);\r\n"
		"\r\n");

	vos_file_fprintf(fd, \
		"	// y-axis\r\n"
		"	drawLine_color_exe(x1, y1, x1, y2, \"#000000\");\r\n"
		"\r\n"
		"	// y-title\r\n"
		"	ctx.font = \"30px Arial\";\r\n"
		"	if (REVERSE_Y) {\r\n"
		"		ctx.strokeText(y_title, x1-LEFT_MARGIN+10, y2-20);	\r\n"
		"	} else {\r\n"
		"		ctx.strokeText(y_title, x1-LEFT_MARGIN+10, y2+30);\r\n"
		"	}\r\n"
		"}\r\n"
		"\r\n"
		"function drawAll(update_only) {\r\n");
}

void _graph_debug_core_write_file_tail(VOS_FILE fd, GRAPH_DEBUG_CORE_CLOSE_CFG *close_cfg)
{	
	vos_file_fprintf(fd, \
		"\r\n"
		"	if (update_only == 0) {\r\n"
		"		// draw x-axis + y-axis\r\n");

	//---- [ x-axis title & y-axis title ] ----
	vos_file_fprintf(fd, "		y_range = max_y-min_y;\r\n");
	vos_file_fprintf(fd, "		draw_axis(\"");
	if (close_cfg->x_axis.axis_name != NULL && close_cfg->x_axis.max_axis_name_len > 0) {
		close_cfg->x_axis.axis_name[close_cfg->x_axis.max_axis_name_len-1] = '\0';
		vos_file_fprintf(fd, "%s", close_cfg->x_axis.axis_name);
	}
	if (close_cfg->x_axis.b_show_range == TRUE) {
		vos_file_fprintf(fd, " (\"+ min_x + \" ~ \" + max_x + \")");
	}
	vos_file_fprintf(fd, "\", \"");
	if (close_cfg->y_axis.axis_name != NULL && close_cfg->y_axis.max_axis_name_len > 0) {
		close_cfg->y_axis.axis_name[close_cfg->y_axis.max_axis_name_len-1] = '\0';
		vos_file_fprintf(fd, "%s", close_cfg->y_axis.axis_name);
	}
	if (close_cfg->y_axis.b_show_range == TRUE) {
		vos_file_fprintf(fd, " (\"+ min_y + \" ~ \" + max_y + \") ( \" + y_range + \" )");
	}
	vos_file_fprintf(fd, "\");\r\n");
	//-------------------------------------------

	vos_file_fprintf(fd, \
		"	}\r\n"
		"\r\n"
		"}\r\n"
		"// update (min_w / min_h / min_x / min_y / max_x / max_y)\r\n"
		"drawAll(1);\r\n"
		"\r\n"
		"scale_factor_w_base = TARGET_MIN_W;\r\n"
		"scale_factor_w_incr = min_w;\r\n"
		"\r\n"
		"scale_factor_h_base = TARGET_MIN_H;\r\n"
		"scale_factor_h_incr = min_h;\r\n"
		"\r\n"
		"total_width  = (max_x-min_x);\r\n"
		"total_height = (max_y-min_y);\r\n"
		"fin_width = (total_width *scale_factor_w_base/scale_factor_w_incr)+LEFT_MARGIN+RIGHT_MARGIN;\r\n"
		"fin_height = (total_height*scale_factor_h_base/scale_factor_h_incr)+TOP_MARGIN+BOTTOM_MARGIN;\r\n"
		"\r\n");

	vos_file_fprintf(fd, \
		"if (fin_width > MAX_CANVAS_WIDTH) {\r\n"
		"	scale_factor_w_base = (MAX_CANVAS_WIDTH - LEFT_MARGIN - RIGHT_MARGIN) * scale_factor_w_incr / total_width;\r\n"
		"}\r\n"
		"\r\n"
		"if (fin_height > MAX_CANVAS_HEIGHT) {\r\n"
		"	scale_factor_h_base = (MAX_CANVAS_HEIGHT - TOP_MARGIN - BOTTOM_MARGIN) * scale_factor_h_incr / total_height;\r\n"
		"}\r\n"
		"canvas.width  = (total_width *scale_factor_w_base/scale_factor_w_incr)+LEFT_MARGIN+RIGHT_MARGIN;\r\n"
		"canvas.height = (total_height*scale_factor_h_base/scale_factor_h_incr)+TOP_MARGIN+BOTTOM_MARGIN;\r\n");

	vos_file_fprintf(fd, \
		"\r\n"
		"// start draw\r\n"
		"drawAll(0);\r\n"
		"\r\n"
		"</script>\r\n"
		"\r\n"
		" </body>\r\n"
		"</html>\r\n");

}
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

INT graph_debug_core_set_default_open_cfg(GRAPH_DEBUG_CORE_OPEN_CFG *open_cfg)
{
	open_cfg->top_margin     = 100;
	open_cfg->bottom_margin  = 100;
	open_cfg->left_margin    = 100;
	open_cfg->right_margin   = 100;
	open_cfg->target_min_w   = 50;
	open_cfg->target_min_h   = 50;
	open_cfg->reverse_y      = 1;
	open_cfg->show_detail_range_x = 0;
	open_cfg->show_detail_range_y = 0;
	open_cfg->show_draw_line      = 1;
	open_cfg->max_canvas_width    = 12000;
	open_cfg->max_canvas_height   = 12000;
	return 0;
}

INT graph_debug_core_open(GRAPH_DEBUG_CORE_OPEN_CFG *open_cfg, CHAR *filepath, GRAPH_DEBUG_CORE_HANDLER *handler) 
{
	VOS_FILE fd;

	*handler = (-1);

	if (filepath == NULL) {
		DBG_ERR("filepath is null ?\r\n");
		return (-1);
	}

	fd = vos_file_open(filepath, O_CREAT|O_WRONLY|O_SYNC|O_TRUNC, 0666);
	if ((VOS_FILE)(-1) == fd) {
		DBG_ERR("open filepath(%s) failed !!!!!\r\n", filepath);
		return (-1);
	}

	*handler = (GRAPH_DEBUG_CORE_HANDLER)fd;
	
	// if cfg is zero, set to default cfg
	if (TRUE == _graph_debug_core_chk_zero_cfg(open_cfg)) {
		graph_debug_core_set_default_open_cfg(open_cfg);
	}

	// write file header
	_graph_debug_core_write_file_head(fd, open_cfg);

	return 0;
}

INT graph_debug_core_add_block(GRAPH_DEBUG_CORE_HANDLER handler, UINT32 x1, UINT32 x2, UINT32 y1, UINT32 y2, CHAR *block_name, UINT max_block_name_len, CHAR *color_name, UINT max_color_name_len)
{
	VOS_FILE fd;

	if (handler == (VOS_FILE)(-1)) {
		DBG_ERR("handler is (-1) ?\r\n");
		return (-1);
	}
	
	fd = (VOS_FILE)handler;

	block_name[max_block_name_len-1] = '\0';
	color_name[max_color_name_len-1] = '\0';

	vos_file_fprintf(fd, "    drawRectMain(%lu, %lu, %lu, %lu, \"%s\", \"%s\", update_only);\r\n", (unsigned long)x1, (unsigned long)x2, (unsigned long)y1, (unsigned long)y2, block_name, color_name);
	return 0;
}


INT graph_debug_core_set_var(GRAPH_DEBUG_CORE_HANDLER handler, CHAR *var_name, UINT max_var_name_len, UINT32 value)
{
	VOS_FILE fd;

	if (handler == (VOS_FILE)(-1)) {
		DBG_ERR("handler is (-1) ?\r\n");
		return (-1);
	}

	if (var_name == NULL || max_var_name_len == 0) {
		DBG_ERR("var_name is NULL or len is 0 !! \r\n");
		return (-1);
	}

	fd = (VOS_FILE)handler;

	var_name[max_var_name_len-1] = '\0';

	vos_file_fprintf(fd, "    %s = %lu;\r\n", var_name, (unsigned long)value);
	return 0;
}

INT graph_debug_core_add_block_with_var_x1(GRAPH_DEBUG_CORE_HANDLER handler, CHAR *var_name, UINT max_var_name_len, UINT32 x2, UINT32 y1, UINT32 y2, CHAR *block_name, UINT max_block_name_len, CHAR *color_name, UINT max_color_name_len)
{
	VOS_FILE fd;

	if (handler == (VOS_FILE)(-1)) {
		DBG_ERR("handler is (-1) ?\r\n");
		return (-1);
	}

	if (var_name == NULL || max_var_name_len == 0) {
		DBG_ERR("var_name is NULL or len is 0 !! \r\n");
		return (-1);
	}

	fd = (VOS_FILE)handler;

	var_name[max_var_name_len-1] = '\0';

	vos_file_fprintf(fd, "    drawRectMain(%s, %lu, %lu, %lu, \"%s\", \"%s\", update_only);\r\n", var_name, (unsigned long)x2, (unsigned long)y1, (unsigned long)y2, block_name, color_name);
	return 0;
}

INT graph_debug_core_add_block_with_var_y1(GRAPH_DEBUG_CORE_HANDLER handler, UINT32 x1, UINT32 x2, CHAR *var_name, UINT max_var_name_len, UINT32 y2, CHAR *block_name, UINT max_block_name_len, CHAR *color_name, UINT max_color_name_len)
{
	VOS_FILE fd;

	if (handler == (VOS_FILE)(-1)) {
		DBG_ERR("handler is (-1) ?\r\n");
		return (-1);
	}

	if (var_name == NULL || max_var_name_len == 0) {
		DBG_ERR("var_name is NULL or len is 0 !! \r\n");
		return (-1);
	}
	
	fd = (VOS_FILE)handler;

	var_name[max_var_name_len-1] = '\0';

	vos_file_fprintf(fd, "    drawRectMain(%lu, %lu, %s, %lu, \"%s\", \"%s\", update_only);\r\n", (unsigned long)x1, (unsigned long)x2, var_name, (unsigned long)y2, block_name, color_name);
	return 0;
}

INT graph_debug_core_add_line(GRAPH_DEBUG_CORE_HANDLER handler, UINT32 x1, UINT32 y1, UINT32 x2, UINT32 y2, CHAR *color_name, UINT max_color_name_len)
{
	VOS_FILE fd;

	if (handler == (VOS_FILE)(-1)) {
		DBG_ERR("handler is (-1) ?\r\n");
		return (-1);
	}

	fd = (VOS_FILE)handler;

	color_name[max_color_name_len-1] = '\0';

	vos_file_fprintf(fd, "    drawLine_color(%lu, %lu, %lu, %lu, \"%s\", update_only);\r\n", (unsigned long)x1, (unsigned long)y1, (unsigned long)x2, (unsigned long)y2, color_name);
	return 0;
}


INT graph_debug_core_close(GRAPH_DEBUG_CORE_HANDLER handler, GRAPH_DEBUG_CORE_CLOSE_CFG *close_cfg)
{
	VOS_FILE fd;

	if (handler == (VOS_FILE)(-1)) {
		DBG_ERR("handler is (-1) ?\r\n");
		return (-1);
	}
	
	fd = (VOS_FILE)handler;

	// write file tail
	_graph_debug_core_write_file_tail(fd, close_cfg);

	vos_file_close(fd);
	return 0;
}

