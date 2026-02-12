
#include "UIApp/lv_user_font_conv/lv_user_font_conv.h"
#include "UIWnd/UIFlow.h"
#include <kwrap/debug.h>
#include <kwrap/error_no.h>
#include <kwrap/task.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include "hd_common.h"
#include "FileSysTsk.h"
#include "vf_gfx.h"

static bool lv_user_dbg_write_file = false;


/* ***********************************
 * type define
 *************************************/
/* canvas output is 8332 if LV_COLOR_DEPTH == 8, 8888 if LV_COLOR_DEPTH == 32 */
typedef union {
	struct {
    	uint16_t blue : 2;
    	uint16_t green : 3;
    	uint16_t red : 3;
    	uint16_t alpha : 8;
    } ch;
    uint16_t full;
} lv_user_color8332_t;

#if LV_COLOR_DEPTH == 8
	typedef lv_user_color8332_t lv_user_color_t;
#elif LV_COLOR_DEPTH == 32 || LV_COLOR_DEPTH == 24
	typedef lv_color_t lv_user_color_t;
#else
	#error "LV_COLOR_DEPTH is currently not supported"
#endif


static inline uint16_t lv_user_color_to4444(lv_user_color_t color)
{
#if LV_COLOR_DEPTH == 8
    lv_user_color4444_t ret;
    LV_USER_COLOR_SET_R16(ret, LV_COLOR_GET_R(color) * 2);
    LV_USER_COLOR_SET_G16(ret, LV_COLOR_GET_G(color) * 2);
    LV_USER_COLOR_SET_B16(ret, LV_COLOR_GET_B(color) * 5);
    LV_USER_COLOR_SET_A16(ret, color.ch.alpha >> 4);
    return ret.full;
#elif LV_COLOR_DEPTH == 24
    lv_user_color4444_t ret;
    LV_USER_COLOR_SET_R16(ret, LV_COLOR_GET_R16(color) >> 1);
    LV_USER_COLOR_SET_G16(ret, LV_COLOR_GET_G16(color) >> 2);
    LV_USER_COLOR_SET_B16(ret, LV_COLOR_GET_B16(color) >> 1);
    LV_USER_COLOR_SET_A16(ret, color.ext_ch.alpha >> 4);
    return ret.full;
#elif LV_COLOR_DEPTH == 32
    lv_user_color4444_t ret;
    LV_USER_COLOR_SET_R16(ret, LV_COLOR_GET_R(color) >> 4);
    LV_USER_COLOR_SET_G16(ret, LV_COLOR_GET_G(color) >> 4);
    LV_USER_COLOR_SET_B16(ret, LV_COLOR_GET_B(color) >> 4);
    LV_USER_COLOR_SET_A16(ret, LV_COLOR_GET_A(color) >> 4);
    return ret.full;
#else
	#error "LV_COLOR_DEPTH is currently not supported"
#endif
}

static inline uint32_t lv_user_color_to32(lv_user_color_t color)
{
#if LV_COLOR_DEPTH == 8
	lv_color32_t ret;
    LV_COLOR_SET_R32(ret, color.ch.red * 36); /*(2^8 - 1)/(2^3 - 1) = 255/7 = 17*/
    LV_COLOR_SET_G32(ret, color.ch.green * 36); /*(2^8 - 1)/(2^3 - 1) = 255/7 = 17*/
    LV_COLOR_SET_B32(ret, color.ch.blue * 85); /*(2^8 - 1)/(2^2 - 1) = 255/3 = 17*/
    LV_COLOR_SET_A32(ret, color.ch.alpha);
    return ret.full;
#elif LV_COLOR_DEPTH == 24
    return lv_color_to32(color);
#elif LV_COLOR_DEPTH == 32
    lv_color32_t ret = color;
    return ret.full;
#else
	#error "LV_COLOR_DEPTH is currently not supported"
#endif
}

static ER lv_user_font_conv_validate_draw_cfg(const lv_user_font_conv_draw_cfg* in);

static ER lv_user_font_conv_validate_draw_cfg(const lv_user_font_conv_draw_cfg* in)
{
	ER ret = 0;

	switch(in->fmt)
	{
	case HD_VIDEO_PXLFMT_ARGB4444:
	case HD_VIDEO_PXLFMT_YUV420:
	case HD_VIDEO_PXLFMT_YUV422:
		break;

	default:
		DBG_ERR("unsupported output fmt(%lx)\r\n", in->fmt);
		ret |= -1;
		break;
	}

	if(in->mode == LV_USER_FONT_CONV_DRAW_TEXT){
		if(in->string.text && (in->string.font == NULL)){
			DBG_ERR("font can't be NULL if string is not empty in draw tex mode!\n");
			ret |= -1;
		}
	}
	else if(in->mode == LV_USER_FONT_CONV_DRAW_IMG){
		if(in->img.id == LV_PLUGIN_RES_ID_NONE){
			DBG_ERR("image resource id can't be LV_PLUGIN_RES_ID_NONE in draw image mode!\n");
			ret |= -1;
		}
	}
	else{
		DBG_ERR("unknown lv_user_font_conv_mode(%lu)!\n", in->mode);
	}

	return ret;
}

static void lv_user_working_buffer_to4444(lv_coord_t width, lv_coord_t height, void* working_buffer, void* output_buffer)
{

	lv_user_color_t* pColor1 = (lv_user_color_t*) working_buffer;
	lv_user_color4444_t* pColor2 = (lv_user_color4444_t*) output_buffer;

	for(int h=0 ; h<height ; h++)
	{
		for(int w=0 ; w<width ; w++)
		{
			pColor2->full = lv_user_color_to4444(*pColor1);
			pColor1++;
			pColor2++;
		}
	}
}

/* yuv420 semi plane */
static void lv_user_working_buffer_to_yuv420(
		lv_coord_t width, lv_coord_t height,
		void* working_buffer, void* output_buffer,
		const uint8_t key_y, const uint8_t key_u, const uint8_t key_v)
{
	lv_coord_t image_size = width * height;
	lv_coord_t uvpos = image_size;
	lv_coord_t ypos = 0;
	lv_user_color_t* pColor1 = (lv_user_color_t*) working_buffer;
	lv_color32_t pColor2;
	uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t* dest = (uint8_t*) output_buffer;

    for( lv_coord_t line = 0; line < height; ++line )
    {
        if( !(line % 2) )
        {
            for( lv_coord_t x = 0; x < width; x += 2 )
            {

            	pColor2.full = lv_user_color_to32(*pColor1);
            	a = pColor2.ch.alpha;
                r = pColor2.ch.red;
                g = pColor2.ch.green;
                b = pColor2.ch.blue;

                if(a == 0){
                	dest[ypos++] = key_y;
                	dest[uvpos++] = key_u;
                	dest[uvpos++] = key_v;
                }
                else{
					dest[ypos++] = LV_USER_RGB_TO_Y(a, r, g, b);
					dest[uvpos++] = LV_USER_RGB_TO_U(a, r, g, b);
					dest[uvpos++] = LV_USER_RGB_TO_V(a, r, g, b);
                }

                pColor1++;

                pColor2.full = lv_user_color_to32(*pColor1);
                a = pColor2.ch.alpha;
                r = pColor2.ch.red;
                g = pColor2.ch.green;
                b = pColor2.ch.blue;

                if(a == 0){
                	dest[ypos++] = key_y;
                }
                else{
                	dest[ypos++] = LV_USER_RGB_TO_Y(a, r, g, b);
                }

                pColor1++;
            }
        }
        else
        {
            for( lv_coord_t x = 0; x < width; x += 1 )
            {
            	pColor2.full = lv_user_color_to32(*pColor1);
            	a = pColor2.ch.alpha;
                r = pColor2.ch.red;
                g = pColor2.ch.green;
                b = pColor2.ch.blue;

                if(a == 0){
                	dest[ypos++] = key_y;
                }
                else{
                	dest[ypos++] = LV_USER_RGB_TO_Y(a, r, g, b);
                }

                pColor1++;
            }
        }
    }
}

static void lv_user_working_buffer_to_yuv422(
		lv_coord_t width, lv_coord_t height,
		void* working_buffer, void* output_buffer,
		const uint8_t key_y, const uint8_t key_u, const uint8_t key_v)
{
	lv_coord_t image_size = width * height;
	lv_coord_t uvpos = image_size;
	lv_coord_t ypos = 0;
	lv_user_color_t* pColor1 = (lv_user_color_t*) working_buffer;
	lv_color32_t pColor2;
	uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t* dest = (uint8_t*) output_buffer;

    for( lv_coord_t line = 0; line < height; ++line )
    {
		for( lv_coord_t x = 0; x < width; x += 2 )
		{

			pColor2.full = lv_user_color_to32(*pColor1);
			a = pColor2.ch.alpha;
			r = pColor2.ch.red;
			g = pColor2.ch.green;
			b = pColor2.ch.blue;

            if(a == 0){
            	dest[ypos++] = key_y;
            	dest[uvpos++] = key_u;
            	dest[uvpos++] = key_v;
            }
            else{
				dest[ypos++] = LV_USER_RGB_TO_Y(a, r, g, b);
				dest[uvpos++] = LV_USER_RGB_TO_U(a, r, g, b);
				dest[uvpos++] = LV_USER_RGB_TO_V(a, r, g, b);
            }

			pColor1++;

			pColor2.full = lv_user_color_to32(*pColor1);
			a = pColor2.ch.alpha;
			r = pColor2.ch.red;
			g = pColor2.ch.green;
			b = pColor2.ch.blue;

            if(a == 0){
            	dest[ypos++] = key_y;
            }
            else{
            	dest[ypos++] = LV_USER_RGB_TO_Y(a, r, g, b);
            }


			pColor1++;
		}
    }
}


/* ***********************************
 * lv_user_font_conv API start
 *************************************/
lv_point_t lv_user_font_conv_calc_string_size(const lv_user_font_conv_draw_cfg* in)
{
    char* utf8_string = in->string.text;
    lv_font_t* font = in->string.font;
    lv_point_t point = {0};
	lv_coord_t letter_space = in->string.letter_space;
	lv_coord_t line_space = in->string.line_space;

    size_t len = _lv_txt_ap_calc_bytes_cnt(utf8_string);
    char* unicode_string = (char*)malloc(len);
    LV_ASSERT_NULL(unicode_string);
    _lv_txt_ap_proc(utf8_string, unicode_string);
    _lv_txt_get_size(&point, unicode_string, font, letter_space, line_space, LV_COORD_MAX, LV_TXT_FLAG_NONE);

    free(unicode_string);

    return point;

}

void lv_user_font_conv_draw_cfg_init(lv_user_font_conv_draw_cfg* draw_cfg)
{
	draw_cfg->fmt = HD_VIDEO_PXLFMT_NONE;
	draw_cfg->ext_h = 10;
	draw_cfg->ext_w = 10;
	draw_cfg->align_h = 0;
	draw_cfg->align_w = 0;
	draw_cfg->string.text = NULL;
	draw_cfg->string.ofs_x = 0;
	draw_cfg->string.ofs_y = 0;
	draw_cfg->string.font = NULL;
	draw_cfg->string.letter_space = 3;
	draw_cfg->string.line_space = 0;
	draw_cfg->string.opa = LV_OPA_COVER;
	draw_cfg->string.color = LV_COLOR_PURPLE;
	draw_cfg->string.align = LV_ALIGN_CENTER;

	draw_cfg->bg.color = LV_COLOR_YELLOW;
	draw_cfg->bg.opa = LV_OPA_COVER;

	draw_cfg->border.width = 3;
	draw_cfg->border.opa = LV_OPA_COVER;
	draw_cfg->border.color = LV_COLOR_RED;

	draw_cfg->mode = LV_USER_FONT_CONV_DRAW_TEXT;

	draw_cfg->img.angle = 0;
	draw_cfg->img.id = LV_PLUGIN_RES_ID_NONE;
	draw_cfg->img.zoom = LV_IMG_ZOOM_NONE;
	draw_cfg->img.pivot = (lv_point_t){0, 0};
	draw_cfg->img.opa = LV_OPA_COVER;
	draw_cfg->img.palette = NULL;

}


ER lv_user_font_conv_calc_buffer_size(const lv_user_font_conv_draw_cfg* in, lv_user_font_conv_calc_buffer_size_result* out)
{
    lv_point_t point = {0};

    if(lv_user_font_conv_validate_draw_cfg(in)){
    	return -1;
    }

	/* critical section lock */
	lv_user_task_handler_lock();

    if(in->mode == LV_USER_FONT_CONV_DRAW_TEXT){
        point = lv_user_font_conv_calc_string_size(in);
        out->width = point.x + in->ext_w;
        out->height = point.y + in->ext_h;
    }
    else if(in->mode == LV_USER_FONT_CONV_DRAW_IMG){
    	lv_obj_t* dummy = lv_obj_create(NULL, NULL); /* dummy screen */
    	lv_obj_t* img = lv_img_create(dummy, NULL); /* specify parent for img to enable auto size */
    	lv_plugin_img_set_src(img, in->img.id);
        out->width = lv_obj_get_width(img);		/* auto sized img width */
        out->height =  lv_obj_get_height(img);	/* auto sized img height */
        lv_obj_del(img);
        lv_obj_del(dummy);
    }

	/* critical section unlock */
	lv_user_task_handler_unlock();

    out->bpp = HD_VIDEO_PXLFMT_BPP(in->fmt);

    /* for hardware alignment */
    if(in->align_w){

    	if(in->align_w > 0){
    		out->width = ALIGN_CEIL(out->width, in->align_w);
    	}
    	else{
    		out->width = ALIGN_FLOOR(out->width, in->align_w);
    	}
    }

    if(in->align_h){

    	if(in->align_h > 0){
    		out->height = ALIGN_CEIL(out->height, in->align_h);
    	}
    	else{
    		out->height = ALIGN_FLOOR(out->height, in->align_h);
    	}
    }

    out->output_buffer_size = ((float)out->bpp / 8) * out->width * out->height;
    out->working_buffer_size = (sizeof(lv_user_color_t)) * out->width * out->height;

    return 0;

}

static lv_draw_img_dsc_t _lv_user_font_conv_get_draw_img_dsc(const lv_user_font_conv_draw_cfg* cfg)
{
	lv_draw_img_dsc_t img_dst = {0};

	lv_draw_img_dsc_init(&img_dst);

	if(cfg){
		img_dst.angle = cfg->img.angle;
		img_dst.opa = cfg->img.opa;
		img_dst.pivot = cfg->img.pivot;
		img_dst.zoom = cfg->img.zoom;
	}

	return img_dst;
}

static lv_draw_rect_dsc_t _lv_user_font_conv_get_draw_rect_dsc(const lv_user_font_conv_draw_cfg* cfg)
{
	lv_draw_rect_dsc_t rect_dsc = { 0 };

	lv_draw_rect_dsc_init(&rect_dsc);

	rect_dsc.outline_opa = LV_OPA_TRANSP;
	rect_dsc.outline_width = 0;
	rect_dsc.shadow_opa = LV_OPA_TRANSP;
	rect_dsc.shadow_width = 0;
	rect_dsc.pattern_opa = LV_OPA_TRANSP;

	if(cfg){

		rect_dsc.radius = cfg->radius;
		rect_dsc.bg_color = cfg->bg.color;
		rect_dsc.bg_opa = cfg->bg.opa;
		rect_dsc.bg_grad_dir = LV_GRAD_DIR_NONE;

		rect_dsc.value_align = cfg->string.align;
		rect_dsc.value_font = cfg->string.font;
		rect_dsc.value_str = cfg->string.text;
		rect_dsc.value_ofs_x = cfg->string.ofs_x;
		rect_dsc.value_ofs_y = cfg->string.ofs_y;
		rect_dsc.value_color = cfg->string.color;
		rect_dsc.value_letter_space = cfg->string.letter_space;
		rect_dsc.value_line_space = cfg->string.line_space;
		rect_dsc.value_opa = cfg->string.opa;

		rect_dsc.border_opa = cfg->border.opa;
		rect_dsc.border_color = cfg->border.color;
		rect_dsc.border_width = cfg->border.width;
	}

	return rect_dsc;
}

/* convert lv_user_font_conv cfg to hd frame */
ER 	lv_user_font_conv_cfg_to_hd_frame(
	const lv_user_font_conv_draw_cfg* cfg,
	const lv_user_font_conv_mem_cfg* font_conv_mem,
	const lv_coord_t width,
	const lv_coord_t height,
	HD_VIDEO_FRAME* out_frame)
{
	UINT32 loff[2] = {0};
	UINT32 addr[2] = {0};
	HD_COMMON_MEM_VIRT_INFO virt_info = {0};

	virt_info.va = font_conv_mem->output_buffer;

	if(hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &virt_info) != HD_OK){
		DBG_ERR("HD_COMMON_MEM_PARAM_VIRT_INFO failed!\n");
		return -1;
	}

	addr[0] = virt_info.pa;

	switch(cfg->fmt)
	{
	case HD_VIDEO_PXLFMT_ARGB4444:
		loff[0] = (width * height * HD_VIDEO_PXLFMT_BPP(cfg->fmt)) / 8;
		loff[1] = 0;
		break;

	case HD_VIDEO_PXLFMT_YUV420:
		loff[0] = width;
		loff[1] = width;
		addr[1] = addr[0] + width * height ;
		break;

	default:
		DBG_ERR("unsupported output fmt(%lx)\r\n", cfg->fmt);
		return -1;
	}

	if(vf_init_ex(out_frame, width, height, cfg->fmt, loff, addr) != HD_OK){
		DBG_ERR("convert cfg to hd frame failed!\n");
		return -1;
	}

	return E_OK;
}

static ER _lv_user_font_conv_indexed_to_rgb332(
		const uint8_t * indexed_data,
		const uint32_t indexed_data_size,
		const uint32_t* palette,
		uint8_t *working_buffer,
		uint32_t working_buffer_size)
{
	const uint8_t *ptr_indexed = indexed_data;
	uint8_t *ptr_rgb332 = working_buffer;
	lv_color32_t color;

	if(working_buffer_size < indexed_data_size){
		DBG_ERR("working buffer(%lx) not enough(%lx needed)\n", working_buffer_size, indexed_data_size);
		return -1;
	}

	for(uint32_t i=0 ; i< indexed_data_size ; i++)
	{
		if(LV_COLOR_TRANSP.full == *ptr_indexed){
			/* do nothing, keep color transparent */
		}
		else{
    		color = (lv_color32_t){.full = palette[*ptr_indexed]};
    		*ptr_rgb332 = LV_COLOR_MAKE8(color.ch.red, color.ch.green, color.ch.blue).full;
		}

		ptr_indexed++;
		ptr_rgb332++;
	}

	return 0;
}



void lv_user_font_conv_write_file(
		char* fileName,
		const lv_user_font_conv_draw_cfg* cfg,
		lv_user_font_conv_mem_cfg* mem)
{
	char newFileName[256] = {'\0'};
	lv_user_font_conv_calc_buffer_size_result result;
	char extension[32];


	if(lv_user_font_conv_calc_buffer_size(cfg, &result) != 0)
		return;

	switch(cfg->fmt)
	{

	case HD_VIDEO_PXLFMT_ARGB4444:
	default:
		sprintf(extension, "argb4444");
		break;

	case HD_VIDEO_PXLFMT_YUV420:
		sprintf(extension, "yuv420");
		break;

	case HD_VIDEO_PXLFMT_YUV422:
		sprintf(extension, "yuv422");
		break;
	}

	sprintf(newFileName, "A:\\%s_(%d)x(%d).%s", fileName, result.width, result.height, extension);


	FST_FILE hFile = FileSys_OpenFile(newFileName, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
	UINT32 filesize = mem->output_buffer_size;
	FileSys_WriteFile(hFile, (UINT8 *)mem->output_buffer, &filesize, 0, NULL);
	FileSys_FlushFile(hFile);
	FileSys_CloseFile(hFile);

	DBG_DUMP("output file [%s] size [%lu]\r\n", newFileName, filesize);
}

ER lv_user_font_conv(
		const lv_user_font_conv_draw_cfg* draw_cfg,
		lv_user_font_conv_mem_cfg* mem_cfg)
{
	ER ret = 0;
	lv_user_font_conv_calc_buffer_size_result result = { 0 };
    void* working_buffer = NULL;
    uint32_t working_buffer_size = 0;
    void* output_buffer = NULL;
    uint32_t output_buffer_size = 0;

    if(lv_user_font_conv_validate_draw_cfg(draw_cfg)){
    	DBG_ERR("draw cfg is invalidate!\r\n");
    	return -1;
    }

	if(lv_user_font_conv_calc_buffer_size(draw_cfg, &result)){
		DBG_ERR("output buffer is NULL!\r\n");
		return -1;
	}

    if(mem_cfg->output_buffer == NULL){
    	DBG_ERR("output buffer is NULL!\r\n");
    	return -1;
    }

    if(mem_cfg->output_buffer_size < result.output_buffer_size){
    	DBG_ERR("output buffer size(%lx) not enough!\r\n");
    	return -1;
    }

    if(mem_cfg->working_buffer && (mem_cfg->working_buffer_size < result.working_buffer_size)){
    	DBG_ERR("working buffer size(%lx) not enough(%lx needed)!\r\n", mem_cfg->working_buffer_size, result.working_buffer_size);
    	return -1;
    }

    output_buffer = mem_cfg->output_buffer;
    output_buffer_size = mem_cfg->output_buffer_size;
    memset(output_buffer, 0, output_buffer_size);

    if(mem_cfg->working_buffer){
    	working_buffer = mem_cfg->working_buffer;
    	working_buffer_size = mem_cfg->working_buffer_size;
    }
    else{
    	working_buffer = (void* )malloc(result.working_buffer_size);
    	LV_ASSERT_NULL(working_buffer);
    	working_buffer_size = result.working_buffer_size;
    	memset(working_buffer, 0, working_buffer_size);
    }

    if(draw_cfg->mode == LV_USER_FONT_CONV_DRAW_TEXT){

        /* critical section lock */
        lv_user_task_handler_lock();
        lv_draw_rect_dsc_t draw_rect_dsc = { 0 };
        lv_obj_t* canvas = lv_canvas_create(NULL, NULL);
        lv_canvas_set_buffer(canvas, working_buffer, result.width, result.height, LV_IMG_CF_TRUE_COLOR_ALPHA);
        draw_rect_dsc = _lv_user_font_conv_get_draw_rect_dsc(draw_cfg);
        lv_canvas_draw_rect(canvas, 0, 0, result.width, result.height, &draw_rect_dsc);
        lv_obj_del(canvas);

        /* critical section unlock */
        lv_user_task_handler_unlock();
    }
    else if(draw_cfg->mode == LV_USER_FONT_CONV_DRAW_IMG){

        /* critical section lock */
        lv_user_task_handler_lock();
        lv_draw_img_dsc_t draw_img_dsc = {0};
        lv_img_dsc_t img_dsc = *((lv_img_dsc_t*)lv_plugin_get_img(draw_cfg->img.id)->img); /* copy img dsc */
        lv_obj_t* canvas = lv_canvas_create(NULL, NULL);
        lv_canvas_set_buffer(canvas, working_buffer, result.width, result.height, LV_IMG_CF_TRUE_COLOR_ALPHA);
        draw_img_dsc = _lv_user_font_conv_get_draw_img_dsc(draw_cfg);

/* convert INDEX to RGB332 */
#if LV_COLOR_DEPTH == 8
        uint8_t *data_rgb332 = NULL;

        if(draw_cfg->img.palette){
        	data_rgb332 = malloc(img_dsc.data_size);
        	memset(data_rgb332, 0, img_dsc.data_size);

        	_lv_user_font_conv_indexed_to_rgb332(
        			img_dsc.data,
					img_dsc.data_size,
					draw_cfg->img.palette,
					data_rgb332,
					img_dsc.data_size
        	);

        	img_dsc.data = data_rgb332;
        }
        else{
        	DBG_WRN("palette not found, indexed image will be renderred as RGB332\n");
        }
#endif

        lv_canvas_draw_img(canvas, 0, 0, &img_dsc, &draw_img_dsc);
        lv_obj_del(canvas);

#if LV_COLOR_DEPTH == 8
        if(data_rgb332) free(data_rgb332);
#endif

        /* critical section unlock */
        lv_user_task_handler_unlock();
    }
    else{
    	DBG_ERR("unknown lv_user_font_conv_mode(%lu)\n", draw_cfg->mode);
    	return -1;
    }

    switch(draw_cfg->fmt)
    {
	case HD_VIDEO_PXLFMT_ARGB4444:
    	lv_user_working_buffer_to4444(result.width, result.height, working_buffer, output_buffer);
    	break;
	case HD_VIDEO_PXLFMT_YUV420:
		lv_user_working_buffer_to_yuv420(result.width, result.height, working_buffer, output_buffer, draw_cfg->key_y, draw_cfg->key_u, draw_cfg->key_v);
		break;
	case HD_VIDEO_PXLFMT_YUV422:
		lv_user_working_buffer_to_yuv422(result.width, result.height, working_buffer, output_buffer, draw_cfg->key_y, draw_cfg->key_u, draw_cfg->key_v);
		break;

    default:
    	DBG_ERR("unsupported output fmt(%lx)\r\n", draw_cfg->fmt);
    	break;

    }

    if(mem_cfg->working_buffer == NULL && working_buffer){
    	free(working_buffer);
    }


    if(lv_user_dbg_write_file){

    	static uint16_t cnt = 0;
    	char fileName[64];

    	sprintf(fileName, "lv_user_font_conv_dbg_%u", cnt++);

    	lv_user_font_conv_write_file(fileName, draw_cfg, mem_cfg);
    }

	return ret;
}

void lv_user_font_conv_dbg_set_write_file(bool enabled)
{
	lv_user_dbg_write_file = enabled;
}

/* ***********************************
 * lv_user_font_conv API end
 *************************************/
