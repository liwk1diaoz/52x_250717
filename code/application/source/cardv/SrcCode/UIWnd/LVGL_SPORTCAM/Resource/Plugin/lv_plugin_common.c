
#include <string.h>
#include "Resource/Plugin/lv_plugin_common.h"

static const char* lv_plugin_obj_type_name(lv_plugin_obj_create_cb_t cb);

#if defined(_WIN32)
    #define STRDUP_FUNC _strdup
#else
    #define STRDUP_FUNC strdup
#endif


/*The size of this union must be 4/8 bytes (uint32_t/uint64_t)*/
typedef union {
    struct {
        MEM_UNIT used : 1;    /* 1: if the entry is used*/
        MEM_UNIT d_size : 31; /* Size of the data*/
    } s;
    MEM_UNIT header; /* The header (used + d_size)*/
} lv_mem_header_t;

typedef struct {

	lv_plugin_res_id		curr_language_id;
	lv_obj_t*				root_scr;
	lv_obj_t*				tmp_scr;

} lv_plugin_internal_t;


static lv_plugin_res_t* g_res = NULL;
static lv_plugin_internal_t g_internal_param;

static lv_signal_cb_t default_msgbox_signal_cb = NULL;

static lv_res_t new_msgbox_signal_cb(lv_obj_t * obj, lv_signal_t sign, void * param)
{
	lv_res_t ret = LV_RES_OK;

	if(default_msgbox_signal_cb == NULL){
		lv_plugin_internal_t* inter_param = &g_internal_param;
		lv_obj_t* tmp = lv_msgbox_create(inter_param->tmp_scr, NULL);
		default_msgbox_signal_cb = lv_obj_get_signal_cb(tmp);
		lv_obj_del(tmp);
	}

	lv_indev_type_t indev_type = lv_indev_get_type(lv_indev_get_act());
	if(indev_type == LV_INDEV_TYPE_KEYPAD) {

		if(sign == LV_SIGNAL_PRESSED || sign == LV_SIGNAL_RELEASED){

			lv_obj_t* btnmatrix = lv_msgbox_get_btnmatrix(obj);

			if(btnmatrix){

				lv_signal_cb_t default_btnmatrix_cb = lv_obj_get_signal_cb(btnmatrix);

				if(default_btnmatrix_cb){

					ret = default_btnmatrix_cb(btnmatrix, sign, param);

					if(sign == LV_SIGNAL_RELEASED)
						return ret;

				}
			}

		}
	}


	ret = default_msgbox_signal_cb(obj, sign, param);

	return ret;
}


void lv_plugin_init(void)
{
	static uint8_t is_init = 0;

	if(!is_init){

		is_init = 1;
		memset(&g_internal_param, 0, sizeof(lv_plugin_internal_t));

		g_internal_param.root_scr = lv_obj_create(NULL, NULL);
		g_internal_param.tmp_scr = lv_obj_create(NULL, NULL);
		g_internal_param.curr_language_id = 1;

        /* let root screen can be keyed */
#if LV_COLOR_DEPTH != 32 && LV_COLOR_DEPTH != 24
        _lv_obj_set_style_local_color(g_internal_param.root_scr, 0, LV_STYLE_BG_COLOR, LV_COLOR_TRANSP);
#else
        _lv_obj_set_style_local_opa(g_internal_param.root_scr, 0, LV_STYLE_BG_OPA, 0);
#endif
		lv_scr_load(g_internal_param.root_scr);
	}
}

LV_PLUG_RET lv_plugin_install_resource(lv_plugin_res_t* res)
{
	LV_ASSERT_NULL(res)


	if(0 == res->font_table_size ||
	   0 == res->img_table_size ||
	   0 == res->language_table_size ||
	   0 == res->string_table_size ||
	   NULL == res->language_font_map ||
	   NULL == res->language_table ||
	   NULL == res->fixed_language_string_id_table ||
	   NULL == res->font_table ||
	   NULL == res->img_table
	)
		return LV_PLUG_ERR_INVALID_RES;

	g_res = res;

	return LV_PLUG_SUCCESS;
}

const lv_plugin_img_t* lv_plugin_get_img(lv_plugin_res_id id)
{
	const lv_plugin_res_t* res = g_res;

	LV_ASSERT_NULL(res)

	const uint16_t size = g_res->img_table_size;

	if(id >= size)
	{
		LV_LOG_ERROR("resource id[%lu] is out of range(%lu)", id, size);
		return NULL;
	}

	return &res->img_table[id];
}

LV_PLUG_RET	lv_plugin_msgbox_update_font(lv_obj_t* msgbox, int16_t part)
{

	LV_ASSERT_OBJ(msgbox, lv_plugin_msgbox_type_name());

	if(!lv_plugin_msgbox_is_ext_attr_allocated(msgbox)){
		return LV_PLUG_ERR_OBJ_EXT_NOT_ALLOCATED;
	}

	lv_plugin_msgbox_ext_t* ext = lv_obj_get_ext_attr(msgbox);
    lv_plugin_res_id string_id = ext->string_id;
    LV_PLUGIN_LANGUAGE_FONT_TYPE font_type;

    switch(part)
    {
    default:
    case LV_MSGBOX_PART_BG:
        font_type = ext->font_type_bg;
        break;

    case LV_MSGBOX_PART_BTN:
        font_type = ext->font_type_btn;
        break;

    case LV_MSGBOX_PART_BTN_BG:
        font_type = ext->font_type_btn_bg;
        break;
    }

    return lv_plugin_obj_update_font_ex(msgbox, string_id, part, font_type);
}

LV_PLUG_RET	lv_plugin_label_update_font(lv_obj_t* label, int16_t part)
{
	LV_ASSERT_OBJ(label, lv_plugin_label_type_name());

	if(!lv_plugin_label_is_ext_attr_allocated(label)){
		return LV_PLUG_ERR_OBJ_EXT_NOT_ALLOCATED;
	}

    lv_plugin_label_ext_t* ext = lv_obj_get_ext_attr(label);
    lv_plugin_res_id string_id = ext->string_id;
    LV_PLUGIN_LANGUAGE_FONT_TYPE font_type = ext->font_type;

    return lv_plugin_obj_update_font_ex(label, string_id, part, font_type);
}

LV_PLUG_RET	lv_plugin_obj_update_font_ex(lv_obj_t* obj, lv_plugin_res_id string_id, int16_t part, LV_PLUGIN_LANGUAGE_FONT_TYPE font_type)
{
    const lv_plugin_res_t* res = g_res;
    const lv_plugin_internal_t* inter_param = &g_internal_param;
    lv_plugin_res_id font_id = LV_PLUGIN_RES_ID_NONE;
    lv_plugin_res_id* curr_lang_fonts = res->language_font_map[inter_param->curr_language_id];
    lv_plugin_res_id*  fonts = NULL;

    LV_ASSERT_NULL(res)

    if(string_id != LV_PLUGIN_RES_ID_NONE){

        if(string_id < res->string_table_size){

            lv_plugin_res_id language_id = res->fixed_language_string_id_table[string_id];

            if(language_id != LV_PLUGIN_RES_ID_NONE)
                fonts = res->language_font_map[language_id];
            else
                fonts = curr_lang_fonts;
        }
        /* don't care string id, just update font according to current language */
        else{
            fonts = curr_lang_fonts;
        }

        if((font_type >= res->language_font_type_size) || (font_type == LV_PLUGIN_LANGUAGE_FONT_TYPE_NUM)){
            font_type = LV_PLUGIN_LANGUAGE_FONT_TYPE_0;

            LV_LOG_WARN("font_type(%lu) is greater than current installed resource(%lu), this could be LVGLBuilder exported code not match to the lv_plugin", font_type, res->language_font_type_size);
        }

        font_id = fonts[font_type];
    }
    else{
        return LV_PLUG_SUCCESS;
    }

    LV_LOG_TRACE("update string id(%u) with font%u(%u)", string_id, font_type, font_id);

    return lv_plugin_obj_set_font(obj, font_id, part);
}

LV_PLUG_RET	lv_plugin_obj_update_font(lv_obj_t* obj, lv_plugin_res_id string_id, int16_t part)
{
    return lv_plugin_obj_update_font_ex(obj, string_id, part, LV_PLUGIN_LANGUAGE_FONT_TYPE_0);
}

LV_PLUG_RET lv_plugin_obj_set_font(lv_obj_t* obj, lv_plugin_res_id font_id, int16_t part)
{
	const lv_plugin_res_t* res = g_res;

	LV_ASSERT_NULL(res)

    const lv_plugin_font_t* font = lv_plugin_get_font(font_id);

	if(part < 0)
		part = LV_OBJ_PART_MAIN;

	lv_style_list_t * style_list = lv_obj_get_style_list(obj, part);

	if(NULL == style_list){
		LV_LOG_ERROR("style list not found!");
		return LV_PLUG_ERR_STYLE_LIST_NOT_FOUND;
	}

//	lv_style_t* last_style = style_list->style_list[style_list->style_cnt - 1];


	for(uint16_t i=0 ; i<style_list->style_cnt ; i++)
	{
		lv_style_t* style = style_list->style_list[i];

		if(style){

			lv_style_set_text_font(style, LV_STATE_DEFAULT, font->font);
			lv_style_set_text_font(style, LV_STATE_CHECKED, font->font);
			lv_style_set_text_font(style, LV_STATE_FOCUSED, font->font);
			lv_style_set_text_font(style, LV_STATE_EDITED, font->font);
			lv_style_set_text_font(style, LV_STATE_HOVERED, font->font);
			lv_style_set_text_font(style, LV_STATE_PRESSED, font->font);
			lv_style_set_text_font(style, LV_STATE_DISABLED, font->font);

		}

	}

	/* some cases obj will not refresh font without this line */
	lv_obj_refresh_style(obj, part, LV_STYLE_TEXT_FONT);

	return LV_PLUG_SUCCESS;
}

const lv_plugin_font_t* lv_plugin_get_font(lv_plugin_res_id id)
{
	const lv_plugin_res_t* res = g_res;

	LV_ASSERT_NULL(res)

	const uint16_t size = g_res->font_table_size;

	if(id >= size)
	{
		LV_LOG_ERROR("resource id[%lu] is out of range(%lu)", id, size);
		return NULL;
	}

	return &res->font_table[id];
}

const lv_plugin_string_t* lv_plugin_get_string(lv_plugin_res_id string_id)
{
	const lv_plugin_res_t* res = g_res;

	LV_ASSERT_NULL(res)

	const lv_plugin_internal_t* inter_param = &g_internal_param;
	const uint16_t size = g_res->string_table_size;
	lv_plugin_string_t* ret = NULL;

	if(string_id >= size)
	{
		LV_LOG_ERROR("resource id[%lu] is out of range(%lu)", string_id, size);
		ret = NULL;
	}
	else{

		lv_plugin_res_id language_id = res->fixed_language_string_id_table[string_id];

		if(language_id != LV_PLUGIN_RES_ID_NONE){
			ret = &res->language_table[language_id][string_id];
		}
		else if(inter_param->curr_language_id == LV_PLUGIN_RES_ID_NONE){
			ret = NULL;
		}
		else{
			ret = &res->language_table[inter_param->curr_language_id][string_id];
		}
	}

	return ret;
}

void lv_plugin_obj_iterator(lv_obj_t* parent, lv_plugin_iterator_callback cb, bool recursive)
{
	lv_obj_t* child = NULL;

    while(true)
	{
        child = lv_obj_get_child(parent, child);

        if(NULL == child)
            break;

		if(recursive)
			lv_plugin_obj_iterator(child, cb, recursive);

		if(cb)
			cb(child);
	}
}

static void _lv_plugin_obj_update_string_callback(lv_obj_t* obj)
{
	lv_obj_type_t buf;

	lv_obj_get_type(obj, &buf);

	if (!strcmp(buf.type[0], lv_plugin_label_type_name()))
	{
		if(lv_plugin_label_is_ext_attr_allocated(obj)){
			lv_plugin_label_update_font(obj, LV_LABEL_PART_MAIN);
			lv_plugin_label_update_text(obj);
		}
	}
	else if (!strcmp(buf.type[0], lv_plugin_msgbox_type_name()))
	{
		if(lv_plugin_msgbox_is_ext_attr_allocated(obj)){
			lv_plugin_msgbox_update_font(obj, LV_MSGBOX_PART_BG);
			lv_plugin_msgbox_update_text(obj);

			lv_obj_t * btnmatrix = lv_msgbox_get_btnmatrix(obj);
			if(btnmatrix){
				lv_plugin_msgbox_update_font(obj, LV_MSGBOX_PART_BTN);
                lv_plugin_msgbox_update_font(obj, LV_MSGBOX_PART_BTN_BG);
			}
		}
	}


}

LV_PLUG_RET lv_plugin_set_active_language(lv_plugin_res_id language_id)
{
	lv_plugin_internal_t* inter_param = &g_internal_param;

	const uint16_t size = g_res->language_table_size;

	if(language_id >= size || language_id == LV_PLUGIN_RES_ID_NONE)
	{
		LV_LOG_ERROR("resource id[%lu] is out of range(%lu)", language_id, size);
		return LV_PLUG_ERR_INVALID_RES_ID;
	}

	inter_param->curr_language_id = language_id;

    lv_obj_t* child = lv_obj_get_child(inter_param->root_scr, NULL);

    if(child){
        lv_plugin_obj_iterator(child, _lv_plugin_obj_update_string_callback, true);
    }

	return LV_PLUG_SUCCESS;
}

lv_plugin_res_id lv_plugin_get_active_language(void)
{
    lv_plugin_internal_t* inter_param = &g_internal_param;
    return inter_param->curr_language_id ;
}

bool _lv_plugin_obj_is_ext_attr_allocated(lv_obj_t* obj, uint32_t base_ext_size)
{
	void* ext = lv_obj_get_ext_attr(obj);

	if(ext){

		uint32_t size = _lv_mem_get_size(ext);

	    if(size == base_ext_size || size == (base_ext_size + sizeof(lv_mem_header_t)))
	    	return false;
	    else
	    	return true;
	}

	return false;
}

void* _lv_plugin_obj_allocate_ext_attr(lv_obj_t* obj, uint32_t ext_size)
{
	if(0 == ext_size){
		LV_LOG_ERROR("allocate ext size = 0!");
		return NULL;
	}

	return lv_obj_allocate_ext_attr(obj, ext_size);
}

bool lv_plugin_label_is_ext_attr_allocated(lv_obj_t* label)
{
    LV_ASSERT_OBJ(label, lv_plugin_label_type_name());

    return _lv_plugin_obj_is_ext_attr_allocated(label, sizeof(lv_label_ext_t));
}

bool lv_plugin_msgbox_is_ext_attr_allocated(lv_obj_t* msgbox)
{
    LV_ASSERT_OBJ(msgbox, lv_plugin_msgbox_type_name());

    return _lv_plugin_obj_is_ext_attr_allocated(msgbox, sizeof(lv_msgbox_ext_t));
}

void* lv_plugin_label_allocate_ext_attr(lv_obj_t* label)
{
    LV_ASSERT_OBJ(label, lv_plugin_label_type_name());
    lv_plugin_label_ext_t* ext = NULL;

    if(!lv_plugin_label_is_ext_attr_allocated(label)){

    	ext = (lv_plugin_label_ext_t*) _lv_plugin_obj_allocate_ext_attr(label, sizeof(lv_plugin_label_ext_t));

    	if(ext){
			ext->string_id = LV_PLUGIN_RES_ID_NONE;
            ext->font_type = LV_PLUGIN_LANGUAGE_FONT_TYPE_0;
    	}
    }
    else{
    	ext = lv_obj_get_ext_attr(label);
    }

    return ext;
}

void* lv_plugin_msgbox_allocate_ext_attr(lv_obj_t* msgbox)
{
    LV_ASSERT_OBJ(msgbox, lv_plugin_msgbox_type_name());
    lv_plugin_msgbox_ext_t* ext = NULL;

    if(!lv_plugin_msgbox_is_ext_attr_allocated(msgbox)){

    	ext = (lv_plugin_msgbox_ext_t*) _lv_plugin_obj_allocate_ext_attr(msgbox, sizeof(lv_plugin_msgbox_ext_t));

    	lv_obj_set_signal_cb(msgbox, new_msgbox_signal_cb);

    	if(ext){
			ext->string_id = LV_PLUGIN_RES_ID_NONE;
            ext->font_type_bg = LV_PLUGIN_LANGUAGE_FONT_TYPE_0;
            ext->font_type_btn = LV_PLUGIN_LANGUAGE_FONT_TYPE_0;
            ext->font_type_btn_bg = LV_PLUGIN_LANGUAGE_FONT_TYPE_0;
    	}



    }
    else{
    	ext = lv_obj_get_ext_attr(msgbox);
    }

    return ext;
}


LV_PLUG_RET	lv_plugin_label_update_text(lv_obj_t* label)
{
	LV_ASSERT_OBJ(label, lv_plugin_label_type_name());

	if(!lv_plugin_label_is_ext_attr_allocated(label)){
		return LV_PLUG_ERR_OBJ_EXT_NOT_ALLOCATED;
	}

	lv_plugin_label_ext_t* ext = lv_obj_get_ext_attr(label);
    const lv_plugin_res_t* res = g_res;

    if((ext->string_id != LV_PLUGIN_RES_ID_NONE) && (ext->string_id < res->string_table_size)){
        return lv_plugin_label_set_text(label, ext->string_id);
    }
    else{
        return LV_PLUG_SUCCESS;
    }
}

LV_PLUG_RET	lv_plugin_msgbox_update_text(lv_obj_t* msgbox)
{
	LV_ASSERT_OBJ(msgbox, lv_plugin_msgbox_type_name());

	if(!lv_plugin_msgbox_is_ext_attr_allocated(msgbox)){
		return LV_PLUG_ERR_OBJ_EXT_NOT_ALLOCATED;
	}

	lv_plugin_msgbox_ext_t* ext = lv_obj_get_ext_attr(msgbox);
    const lv_plugin_res_t* res = g_res;

    if((ext->string_id != LV_PLUGIN_RES_ID_NONE) && (ext->string_id < res->string_table_size)){
        return lv_plugin_msgbox_set_text(msgbox, ext->string_id);
    }
    else{
        return LV_PLUG_SUCCESS;
    }
}

LV_PLUG_RET	lv_plugin_img_set_src(lv_obj_t* img,  lv_plugin_res_id img_id)
{
	LV_ASSERT_OBJ(img, lv_plugin_img_type_name());

	const lv_plugin_img_t* src = lv_plugin_get_img(img_id);

	if(NULL == src){
		LV_LOG_ERROR("invalid img id[%lu]!", img_id);
		return LV_PLUG_ERR_INVALID_RES_ID;
	}

	if(src->img != NULL){
		lv_img_set_src(img, src->img);
	}
	else{
		LV_LOG_ERROR("NULL source of img id[%lu]!, ignore setting", img_id);
		return LV_PLUG_ERR_INVALID_RES;
	}

	return LV_PLUG_SUCCESS;
}

LV_PLUG_RET	lv_plugin_label_set_font_type(lv_obj_t* label,  LV_PLUGIN_LANGUAGE_FONT_TYPE font_type)
{
    LV_ASSERT_OBJ(label, lv_plugin_label_type_name());

    if(!lv_plugin_label_is_ext_attr_allocated(label)){
        return LV_PLUG_ERR_OBJ_EXT_NOT_ALLOCATED;
    }

    if(font_type == LV_PLUGIN_LANGUAGE_FONT_TYPE_NUM){
        LV_LOG_ERROR("font_type(%lu) is out of range!", font_type);
        return LV_PLUG_ERR_INVALID_RES_ID;
    }

    lv_plugin_label_ext_t* ext = lv_obj_get_ext_attr(label);

    ext->font_type = font_type;

    return LV_PLUG_SUCCESS;
}

LV_PLUG_RET	lv_plugin_msgbox_set_font_type(lv_obj_t* msgbox,  LV_PLUGIN_LANGUAGE_FONT_TYPE font_type, int8_t part)
{
    LV_ASSERT_OBJ(msgbox, lv_plugin_msgbox_type_name());

    if(!lv_plugin_msgbox_is_ext_attr_allocated(msgbox)){
        return LV_PLUG_ERR_OBJ_EXT_NOT_ALLOCATED;
    }

    if(font_type == LV_PLUGIN_LANGUAGE_FONT_TYPE_NUM){
        LV_LOG_ERROR("font_type(%lu) is out of range!", font_type);
        return LV_PLUG_ERR_INVALID_RES_ID;
    }

    lv_plugin_msgbox_ext_t* ext = lv_obj_get_ext_attr(msgbox);

    switch(part)
    {
    default:
    case LV_MSGBOX_PART_BG:
        ext->font_type_bg = font_type;
        break;

    case LV_MSGBOX_PART_BTN:
        ext->font_type_btn = font_type;
        break;

    case LV_MSGBOX_PART_BTN_BG:
        ext->font_type_btn_bg = font_type;
        break;
    }

    return LV_PLUG_SUCCESS;
}

LV_PLUG_RET	lv_plugin_label_set_text(lv_obj_t* label,  lv_plugin_res_id string_id)
{
	LV_ASSERT_OBJ(label, lv_plugin_label_type_name());

	if(!lv_plugin_label_is_ext_attr_allocated(label)){
		return LV_PLUG_ERR_OBJ_EXT_NOT_ALLOCATED;
	}


	lv_plugin_label_ext_t* ext = lv_obj_get_ext_attr(label);

    if(LV_PLUGIN_RES_ID_NONE == string_id){

	}
	else{

		const lv_plugin_string_t* string = lv_plugin_get_string(string_id);

		if(string){

			lv_label_set_text_static(label, string->ptr);
		}
		else{

			return LV_PLUG_ERR_INVALID_RES_ID;
		}
	}

	ext->string_id = string_id;

	return LV_PLUG_SUCCESS;
}

LV_PLUG_RET	lv_plugin_msgbox_set_text(lv_obj_t* msgbox,  lv_plugin_res_id string_id)
{
	LV_ASSERT_OBJ(msgbox, lv_plugin_msgbox_type_name());

	if(!lv_plugin_msgbox_is_ext_attr_allocated(msgbox)){
		return LV_PLUG_ERR_OBJ_EXT_NOT_ALLOCATED;
	}


	lv_plugin_msgbox_ext_t* ext = lv_obj_get_ext_attr(msgbox);

	if(LV_PLUGIN_RES_ID_NONE == string_id){

	}
	else{

		const lv_plugin_string_t* string = lv_plugin_get_string(string_id);

		if(string){

			lv_msgbox_set_text(msgbox, string->ptr);
		}
		else{

			return LV_PLUG_ERR_INVALID_RES_ID;
		}
	}

	ext->string_id = string_id;

	return LV_PLUG_SUCCESS;
}

LV_PLUG_RET _lv_plugin_scr_attach(lv_obj_t* scr)
{
	lv_plugin_internal_t* inter_param = &g_internal_param;

	lv_obj_set_parent(scr, inter_param->root_scr);

	return LV_PLUG_SUCCESS;
}

LV_PLUG_RET _lv_plugin_scr_detach(lv_obj_t* scr)
{
	lv_plugin_internal_t* inter_param = &g_internal_param;

	lv_obj_set_parent(scr, inter_param->tmp_scr);

	return LV_PLUG_SUCCESS;
}


bool _lv_plugin_scr_is_attached(lv_obj_t* scr)
{
	lv_plugin_internal_t* inter_param = &g_internal_param;
	lv_obj_t* child = NULL;

	if(scr == NULL)
		return false;


	while(true)
	{

		child = lv_obj_get_child_back(inter_param->root_scr, child);

		if(child == NULL)
			return false;

		if(child == scr)
			return true;
	}

	return false;
}


LV_PLUG_RET lv_plugin_scr_open(lv_obj_t* scr, const void * data)
{
	lv_plugin_internal_t* inter_param = &g_internal_param;
	lv_obj_t* parent = lv_obj_get_parent(scr);


	if(parent != inter_param->root_scr && parent != inter_param->tmp_scr){

		LV_LOG_ERROR("invalid obj!");
		return LV_PLUG_ERR_INVALID_OBJ;
	}

    if(parent == inter_param->root_scr){
        return LV_PLUG_SUCCESS;
    }


	lv_obj_clear_state(scr, LV_STATE_DISABLED);

	LV_PLUG_RET ret = _lv_plugin_scr_attach(scr);

    if(ret == LV_PLUG_SUCCESS){
        lv_plugin_set_active_language(inter_param->curr_language_id);
    }

    lv_event_send(scr, LV_PLUGIN_EVENT_SCR_OPEN, data);

	return ret;
}


/* tell is screen ready to be closed */
bool lv_plugin_scr_is_ready_to_be_closed(lv_obj_t* scr)
{

	lv_state_t state = lv_obj_get_state(scr, LV_OBJ_PART_MAIN);

	return (state & LV_STATE_DISABLED)? true : false;
}


LV_PLUG_RET lv_plugin_scr_close(lv_obj_t* scr, const void * data)
{
	lv_plugin_internal_t* inter_param = &g_internal_param;
	lv_obj_t *parent = lv_obj_get_parent(scr);
	lv_obj_t *child = NULL, *next_child = NULL;

	if(scr == NULL)
		return LV_PLUG_SUCCESS;

	if(parent != inter_param->root_scr && parent != inter_param->tmp_scr){

		LV_LOG_ERROR("invalid obj!");
		return LV_PLUG_ERR_INVALID_OBJ;
	}

	if(parent == inter_param->tmp_scr)
		return LV_PLUG_SUCCESS;

	if(_lv_plugin_scr_is_attached(scr) == false)
		return LV_PLUG_SUCCESS;


    /* add all screens ready to be closed LV_STATE_DISABLED */
    child = NULL;
    next_child = NULL;

    child = lv_obj_get_child(inter_param->root_scr, child);

    do
    {
        next_child = lv_obj_get_child(inter_param->root_scr, child);
        lv_obj_add_state(scr, LV_STATE_DISABLED);

        if(NULL == next_child)
            break;

        if(child == scr)
            break;

        child = next_child;

    } while(1);


	/* detach screen from specified screen to top screen */
	child = NULL;
	next_child = NULL;

	child = lv_obj_get_child(inter_param->root_scr, child);

	do
	{
		next_child = lv_obj_get_child(inter_param->root_scr, child);
		_lv_plugin_scr_detach(child);

        /* clear LV_STATE_DISABLED after screen detached */
        lv_obj_clear_state(child, LV_STATE_DISABLED);

        lv_event_send(child, LV_PLUGIN_EVENT_SCR_CLOSE, data);

		if(NULL == next_child)
			break;


        /* update new top screen language before notifying child sceen close */
        if(child == scr)
            lv_plugin_set_active_language(inter_param->curr_language_id);

        /* notify older screen the child screen is closed */
		lv_event_send(next_child, LV_PLUGIN_EVENT_CHILD_SCR_CLOSE, data);

		if(child == scr)
			break;

		child = next_child;

	} while(1);

	return LV_PLUG_SUCCESS;
}

lv_obj_t* lv_plugin_scr_create(void)
{
	lv_plugin_internal_t* inter_param = &g_internal_param;
	return lv_obj_create(inter_param->tmp_scr, inter_param->tmp_scr);
}


static const char* lv_plugin_obj_type_name(lv_plugin_obj_create_cb_t cb)
{
	lv_plugin_internal_t* inter_param = &g_internal_param;
	lv_obj_t* obj = cb(inter_param->tmp_scr, NULL);
	lv_obj_type_t buf;

	lv_obj_get_type(obj, &buf);
	lv_obj_del(obj);

	return buf.type[0];
}

const char* lv_plugin_label_type_name()
{
	static char* name = NULL;

	if(NULL == name)
        name = STRDUP_FUNC(lv_plugin_obj_type_name(lv_label_create));

	return name;
}

 const char* lv_plugin_img_type_name()
{
	static char* name = NULL;

	if(NULL == name)
        name = STRDUP_FUNC(lv_plugin_obj_type_name(lv_img_create));

	return name;
}


const char* lv_plugin_btn_type_name()
{
	static char* name = NULL;

	if(NULL == name)
        name = STRDUP_FUNC(lv_plugin_obj_type_name(lv_btn_create));

	return name;
}

const char* lv_plugin_imgbtn_type_name()
{
    static char* name = NULL;

    if(NULL == name)
        name = STRDUP_FUNC(lv_plugin_obj_type_name(lv_imgbtn_create));

    return name;
}

const char* lv_plugin_cont_type_name()
{
	static char* name = NULL;

	if(NULL == name)
        name = STRDUP_FUNC(lv_plugin_obj_type_name(lv_cont_create));

	return name;
}

const char* lv_plugin_msgbox_type_name()
{
	static char* name = NULL;

	if(NULL == name)
        name = STRDUP_FUNC(lv_plugin_obj_type_name(lv_msgbox_create));

	return name;
}

lv_obj_t* lv_plugin_find_child_by_type(lv_obj_t* obj, lv_obj_t* start_child ,const char* type)
{
	lv_obj_t* child = start_child;
	lv_obj_type_t buf;

	while(true)
	{
		child = lv_obj_get_child(obj, child);

		if(child == NULL)
			break;

		lv_obj_get_type(child, &buf);

		if(!strcmp(buf.type[0], type))
			break;
	}

	return child;
}

lv_indev_t* lv_plugin_find_indev_by_type(lv_indev_type_t type)
{
    lv_indev_t *indev = NULL;

    while(true)
    {
        indev = lv_indev_get_next(indev);

        if (indev == NULL)
            break;

        if(lv_indev_get_type(indev) == type){
            break;
        }
    }

    return indev;
}


static bool lv_plugin_obj_is_type_matched(lv_obj_t* obj, const char* name)
{
	lv_obj_type_t buf;

	lv_obj_get_type(obj, &buf);

	return (!strcmp(buf.type[0], name))? true : false;

}

bool lv_plugin_obj_is_label(lv_obj_t* obj)
{
	return lv_plugin_obj_is_type_matched(obj, lv_plugin_label_type_name() );
}

bool lv_plugin_obj_is_img(lv_obj_t* obj)
{
	return lv_plugin_obj_is_type_matched(obj, lv_plugin_img_type_name() );
}

bool lv_plugin_obj_is_imgbtn(lv_obj_t* obj)
{
	return lv_plugin_obj_is_type_matched(obj, lv_plugin_imgbtn_type_name() );
}

bool lv_plugin_obj_is_cont(lv_obj_t* obj)
{
	return lv_plugin_obj_is_type_matched(obj, lv_plugin_cont_type_name() );
}
bool lv_plugin_obj_is_btn(lv_obj_t* obj)
{
	return lv_plugin_obj_is_type_matched(obj, lv_plugin_btn_type_name() );
}

bool lv_plugin_obj_is_msgbox(lv_obj_t* obj)
{
	return lv_plugin_obj_is_type_matched(obj, lv_plugin_msgbox_type_name() );
}

lv_obj_t* lv_plugin_scr_act()
{
	lv_plugin_internal_t* inter_param = &g_internal_param;
    lv_obj_t* child = lv_obj_get_child(inter_param->root_scr, NULL);

	return child;
}

lv_obj_t* lv_plugin_scr_by_index(uint16_t index)
{
    lv_plugin_internal_t* inter_param = &g_internal_param;
    uint16_t count = lv_plugin_scr_count();
    lv_obj_t* child = NULL;
    uint16_t tmp_cnt = 0;

    if(index < count){

        while(tmp_cnt < count)
        {
            child = lv_obj_get_child_back(inter_param->root_scr, child);

            if(tmp_cnt == index)
                break;

            tmp_cnt++;
        }
    }
    else{
        LV_LOG_ERROR("out of index(%d), ", index);
    }

    return child;
}

uint16_t lv_plugin_scr_count(void)
{
    lv_plugin_internal_t* inter_param = &g_internal_param;
    return lv_obj_count_children(inter_param->root_scr);
}
