
#ifndef _LV_FONTCONV_H
#define _LV_FONTCONV_H


#include "FontConv/FontConv.h"
#include "UIFlowLVGL/UIFlowLVGL.h"
#include "lvgl/src/lv_misc/lv_txt_ap.h"
#include "hd_type.h"

/* for output argb4444 */
typedef union {
    struct {
    	uint16_t blue : 4;
    	uint16_t green : 4;
    	uint16_t red : 4;
    	uint16_t alpha : 4;
    } ch;
    uint16_t full;
} lv_user_color4444_t;

/* for output argb4444 */
# define LV_USER_COLOR_SET_R16(c, v) (c).ch.red = (uint8_t)((v) & 0xFU)
# define LV_USER_COLOR_SET_G16(c, v) (c).ch.green = (uint8_t)((v) & 0xFU)
# define LV_USER_COLOR_SET_B16(c, v) (c).ch.blue = (uint8_t)((v) & 0xFU)
# define LV_USER_COLOR_SET_A16(c, v) (c).ch.alpha = (uint8_t)((v) & 0xFU)

# define LV_USER_COLOR_GET_R16(c) (c).ch.red
# define LV_USER_COLOR_GET_G16(c) (c).ch.green
# define LV_USER_COLOR_GET_B16(c) (c).ch.blue
# define LV_USER_COLOR_GET_A16(c) (c).ch.alpha

/************************************************
 * RGB to YUV, drop A if A > 0 or being transparent
 ************************************************/
#define LV_USER_YUV_GET_V(YUV) (uint8_t)((YUV & 0xFF0000) >> 16)
#define LV_USER_YUV_GET_U(YUV) (uint8_t)((YUV & 0xFF00) >> 8)
#define LV_USER_YUV_GET_Y(YUV) (uint8_t)(YUV & 0xFF)
#define LV_USER_RGB_TO_Y(A, R, G, B) ( (R*0.299 + G*0.587 + B*0.114))
#define LV_USER_RGB_TO_U(A, R, G, B) ( (-0.169*R + G*-0.332 + B*0.500 + 128))
#define LV_USER_RGB_TO_V(A, R, G, B) ( (R*0.500 + G*-0.419 + B*-0.0813 + 128))

#define LV_USER_COLOR32_GET_Y(c) LV_USER_RGB_TO_Y((c).ch.alpha, (c).ch.red, (c).ch.green, (c).ch.blue)
#define LV_USER_COLOR32_GET_U(c) LV_USER_RGB_TO_U((c).ch.alpha, (c).ch.red, (c).ch.green, (c).ch.blue)
#define LV_USER_COLOR32_GET_V(c) LV_USER_RGB_TO_V((c).ch.alpha, (c).ch.red, (c).ch.green, (c).ch.blue)

typedef struct {

	lv_plugin_res_id id;
	lv_opa_t	opa;
    uint16_t angle;
    lv_point_t pivot;
    uint16_t zoom;
    uint32_t* palette; /* for LV_COLOR_DEPTH =  8 */

} lv_user_font_conv_img_cfg;

typedef struct {

	char* text;
	lv_font_t * font;
	lv_coord_t letter_space;
	lv_coord_t line_space;
	lv_opa_t opa;
	lv_color_t color;
	lv_align_t align;
    lv_style_int_t ofs_x;
    lv_style_int_t ofs_y;

} lv_user_font_conv_string_cfg;


typedef struct {

	lv_opa_t opa;
	lv_color_t color;

} lv_user_font_conv_bg_cfg;

typedef struct {

	lv_opa_t opa;
	lv_color_t color;
	lv_coord_t width;

} lv_user_font_conv_border_cfg;

typedef enum {
	LV_USER_FONT_CONV_DRAW_TEXT,
	LV_USER_FONT_CONV_DRAW_IMG
} lv_user_font_conv_mode;


/* mandatory info for calc mem & font size */
typedef struct {
	HD_VIDEO_PXLFMT fmt;

	lv_coord_t align_w;
	lv_coord_t align_h;

	/* draw text options */
	lv_coord_t ext_w; 	/* extend width, not including string text width */
	lv_coord_t ext_h;	/* extend height, not including string text height */
	lv_style_int_t radius;
	lv_user_font_conv_string_cfg string;
	lv_user_font_conv_bg_cfg bg;
	lv_user_font_conv_border_cfg border;

	/* draw img options */
	lv_user_font_conv_img_cfg img;

	/* for yuv format, transparent pixel will be filled color key */
	uint8_t key_y;
	uint8_t key_u;
	uint8_t key_v;

	lv_user_font_conv_mode mode;

} lv_user_font_conv_draw_cfg;

typedef struct {

	void* output_buffer;			/* mandatory */
	uint32_t output_buffer_size;

	void* working_buffer;			/* option */
	uint32_t working_buffer_size;

} lv_user_font_conv_mem_cfg;

typedef struct {

	lv_coord_t width;
	lv_coord_t height;
	uint8_t bpp;
	uint32_t output_buffer_size;
	uint32_t working_buffer_size;

} lv_user_font_conv_calc_buffer_size_result;


ER 			lv_user_font_conv_calc_buffer_size(const lv_user_font_conv_draw_cfg* in, lv_user_font_conv_calc_buffer_size_result* out);
lv_point_t 	lv_user_font_conv_calc_string_size(const lv_user_font_conv_draw_cfg* in);
ER 			lv_user_font_conv(const lv_user_font_conv_draw_cfg* cfg, lv_user_font_conv_mem_cfg* font_conv_mem);
void 		lv_user_font_conv_draw_cfg_init(lv_user_font_conv_draw_cfg* draw_cfg);
void 		lv_user_font_conv_write_file(char* fileName, const lv_user_font_conv_draw_cfg* cfg, lv_user_font_conv_mem_cfg* mem);

ER 			lv_user_font_conv_cfg_to_hd_frame(
				const lv_user_font_conv_draw_cfg* cfg,
				const lv_user_font_conv_mem_cfg* font_conv_mem,
				const lv_coord_t width,
				const lv_coord_t height,
				HD_VIDEO_FRAME* out_frame
			);

void 		lv_user_font_conv_dbg_set_write_file(bool enabled);

ER 			lv_user_font_conv_test_style(lv_user_font_conv_draw_cfg draw_cfg);
ER 			lv_user_font_conv_test_multi_thread(int worker_num, int max_cnt, HD_VIDEO_PXLFMT fmt);

#endif
