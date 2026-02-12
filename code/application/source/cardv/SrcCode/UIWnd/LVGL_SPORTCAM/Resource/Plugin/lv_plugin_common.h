
#ifndef LV_PLUGIN_COMMON_H
#define LV_PLUGIN_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"


#define LV_PLUGIN_RES_ID_NONE 0
#define LV_PLUGIN_RES_ID_MAX (UINT16_MAX)

#define LV_PLUGIN_EVENT_FIRST _LV_EVENT_LAST

/* copy from lv_mem.c */
#ifdef LV_ARCH_64
    #define ALIGN_MASK 0x7
#else
    #define ALIGN_MASK 0x3
#endif

#ifdef LV_ARCH_64
    #define MEM_UNIT uint64_t
#else
    #define MEM_UNIT uint32_t
#endif

#define LV_PLUGIN_PADD MEM_UNIT padd;

typedef void (*lv_plugin_iterator_callback)(lv_obj_t* obj);

typedef struct {
	const char* ptr;
	const uint32_t size;
} lv_plugin_string_t;

typedef struct {
	const lv_img_dsc_t* img;
} lv_plugin_img_t;

typedef struct {
	const lv_font_t* font;
} lv_plugin_font_t;

typedef enum {
	LV_PLUG_SUCCESS = 0,
	LV_PLUG_ERR_INVALID_PARAM,
	LV_PLUG_ERR_INVALID_RES,
	LV_PLUG_ERR_INVALID_OBJ,
	LV_PLUG_ERR_INVALID_RES_ID,
	LV_PLUG_ERR_STYLE_LIST_NOT_FOUND,
	LV_PLUG_ERR_OBJ_EXT_NOT_ALLOCATED,
	LV_PLUG_ERR_MENU_OUT_OF_IDX,
	LV_PLUG_ERR_MENU_ALREADY_ALLOCATED,
	LV_PLUG_ERR_MENU_NOT_ALLOCATED,
} LV_PLUG_RET;

enum {
	LV_PLUGIN_EVENT_SCR_OPEN = LV_PLUGIN_EVENT_FIRST,
	LV_PLUGIN_EVENT_SCR_CLOSE,
	LV_PLUGIN_EVENT_CHILD_SCR_CLOSE,
	LV_PLUGIN_EVENT_MENU_ITEM_FOCUSED,
	LV_PLUGIN_EVENT_MENU_ITEM_DEFOCUSED,
	LV_PLUGIN_EVENT_MENU_ITEM_SELECTED,
	LV_PLUGIN_EVENT_MENU_NOTIFY_END_OF_ITEM,	
	_LV_PLUGIN_EVENT_LAST
};

typedef enum {
    LV_PLUGIN_LANGUAGE_FONT_TYPE_0 = 0,
    LV_PLUGIN_LANGUAGE_FONT_TYPE_1,
    LV_PLUGIN_LANGUAGE_FONT_TYPE_2,
    LV_PLUGIN_LANGUAGE_FONT_TYPE_3,
    LV_PLUGIN_LANGUAGE_FONT_TYPE_4,
    LV_PLUGIN_LANGUAGE_FONT_TYPE_NUM
} LV_PLUGIN_LANGUAGE_FONT_TYPE;

typedef uint16_t lv_plugin_res_id;

typedef struct {

	lv_plugin_img_t* 				img_table;
	lv_plugin_font_t*				font_table;
	lv_plugin_string_t** 			language_table;
    lv_plugin_res_id**				language_font_map;      /* language - font map, one language coulde associates multiple fonts */
	lv_plugin_res_id*				fixed_language_string_id_table;

    uint16_t						img_table_size;         /* how many images */
    uint16_t						font_table_size;        /* how many fonts */
    uint16_t						language_table_size;    /* how many languages */
    uint16_t						string_table_size;      /* how many string ids */
    uint8_t                         language_font_type_size;      /* how many fonts associated with per language */

} lv_plugin_res_t;


typedef struct {
	lv_label_ext_t base_ext;
	lv_plugin_res_id string_id;
    LV_PLUGIN_LANGUAGE_FONT_TYPE font_type;
	LV_PLUGIN_PADD
} lv_plugin_label_ext_t;

typedef struct {
	lv_img_ext_t base_ext;
	lv_plugin_res_id img_id;
	LV_PLUGIN_PADD
} lv_plugin_img_ext_t;


typedef struct {
	lv_msgbox_ext_t base_ext;
	lv_plugin_res_id string_id;
    LV_PLUGIN_LANGUAGE_FONT_TYPE font_type_bg;
    LV_PLUGIN_LANGUAGE_FONT_TYPE font_type_btn;
    LV_PLUGIN_LANGUAGE_FONT_TYPE font_type_btn_bg;
	LV_PLUGIN_PADD
} lv_plugin_msgbox_ext_t;

void						lv_plugin_init(void);

LV_PLUG_RET 				lv_plugin_install_resource(lv_plugin_res_t* res);

const lv_plugin_img_t* 		lv_plugin_get_img(lv_plugin_res_id id);
const lv_plugin_font_t* 	lv_plugin_get_font(lv_plugin_res_id id);


/**
 * Set object font, this API simplify style configuration.
 * @param obj
 * @param id font resource id
 * @param part please refer to lvgl document or just set -1.
 * @return LV_PLUG_RET return code
 */
LV_PLUG_RET					lv_plugin_obj_set_font(lv_obj_t* obj, lv_plugin_res_id font_id, int16_t part);


LV_PLUG_RET					lv_plugin_label_update_font(lv_obj_t* label, int16_t part);
LV_PLUG_RET					lv_plugin_msgbox_update_font(lv_obj_t* msgbox, int16_t part);

LV_PLUG_RET					lv_plugin_set_active_language(lv_plugin_res_id language_id);
lv_plugin_res_id			lv_plugin_get_active_language(void);
const lv_plugin_string_t*	lv_plugin_get_string(lv_plugin_res_id string_id);

LV_PLUG_RET					lv_plugin_obj_update_font(lv_obj_t* obj, lv_plugin_res_id string_id, int16_t part);
LV_PLUG_RET                 lv_plugin_obj_update_font_ex(lv_obj_t* obj, lv_plugin_res_id string_id, int16_t part, LV_PLUGIN_LANGUAGE_FONT_TYPE type);


/**
 * Internal API, check object plug-in extended attributes is allocated, the base_size should be ext size of obj.
 * @param obj
 * @param base_size generic widget ext size
 * @return bool
 */
bool 						_lv_plugin_obj_is_ext_attr_allocated(lv_obj_t* obj, uint32_t base_size);


/**
 * Internal API, allocate object plug-in extended attributes, ext_size is plug-in ext size and should be larger than generic ext size.
 * @param obj
 * @param base_size generic widget ext size
 * @return void*
 */
void* 						_lv_plugin_obj_allocate_ext_attr(lv_obj_t* obj, uint32_t ext_size);

/**
 * Set label font type.
 * @param label
 * @param font_type
 * @return LV_PLUG_RET return code
 */
LV_PLUG_RET					lv_plugin_label_set_font_type(lv_obj_t* label,  LV_PLUGIN_LANGUAGE_FONT_TYPE font_type);
LV_PLUG_RET					lv_plugin_msgbox_set_font_type(lv_obj_t* msgbox,  LV_PLUGIN_LANGUAGE_FONT_TYPE font_type, int8_t part);


/**
 * Set label text by string id of the active language.
 * @param label
 * @param string_id
 * @return LV_PLUG_RET return code
 */
LV_PLUG_RET					lv_plugin_label_set_text(lv_obj_t* label,  lv_plugin_res_id string_id);
LV_PLUG_RET					lv_plugin_msgbox_set_text(lv_obj_t* msgbox,  lv_plugin_res_id string_id);

/**
 * Update label text by string id of the active language.
 * @param label
 * @return LV_PLUG_RET return code
 */
LV_PLUG_RET					lv_plugin_label_update_text(lv_obj_t* label);
LV_PLUG_RET					lv_plugin_msgbox_update_text(lv_obj_t* msgbox);


/**
 * Check label object plug-in extended attributes is allocated.
 * @param label
 * @return bool
 */
bool 						lv_plugin_label_is_ext_attr_allocated(lv_obj_t* label);
bool 						lv_plugin_msgbox_is_ext_attr_allocated(lv_obj_t* label);

/**
 * Allocate label object plug-in extended attributes.
 * @param label
 */
void* 						lv_plugin_label_allocate_ext_attr(lv_obj_t* label);
void* 						lv_plugin_msgbox_allocate_ext_attr(lv_obj_t* msgbox);


LV_PLUG_RET					lv_plugin_img_set_src(lv_obj_t* obj,  lv_plugin_res_id img_id);



bool 						lv_plugin_scr_is_ready_to_be_closed(lv_obj_t* scr);


/**
 * Open a screen object and send LV_PLUGIN_EVENT_SCR_OPEN events.
 * @param scr
 * @return LV_PLUG_RET return code
 */
LV_PLUG_RET 				lv_plugin_scr_open(lv_obj_t* scr, const void * data);


/**
 * Close a screen object and send LV_PLUGIN_EVENT_SCR_CLOSE events.
 * @param scr
 * @return LV_PLUG_RET return code
 */
LV_PLUG_RET 				lv_plugin_scr_close(lv_obj_t* scr, const void * data);

/**
 * Internal API, attach a screen object to root screen from tmp screen.
 * @param scr
 * @return LV_PLUG_RET return code
 */
LV_PLUG_RET 				_lv_plugin_scr_attach(lv_obj_t* scr);


/**
 * Internal API, detach a screen object from root screen to tmp screen.
 * @param scr
 * @return LV_PLUG_RET return code
 */
LV_PLUG_RET 				_lv_plugin_scr_detach(lv_obj_t* scr);


/**
 * Create a plug-in screen.
 * @return lv_obj_t* plug-in screen
 */
lv_obj_t* 					lv_plugin_scr_create(void);


void lv_plugin_obj_iterator(lv_obj_t* parent, lv_plugin_iterator_callback cb, bool recursive);

typedef lv_obj_t * (*lv_plugin_obj_create_cb_t)(lv_obj_t * par, const lv_obj_t * copy);

const char* lv_plugin_label_type_name(void);
const char* lv_plugin_img_type_name(void);
const char* lv_plugin_btn_type_name(void);
const char* lv_plugin_cont_type_name(void);
const char* lv_plugin_imgbtn_type_name(void);
const char* lv_plugin_msgbox_type_name(void);

bool lv_plugin_obj_is_label(lv_obj_t* obj);
bool lv_plugin_obj_is_img(lv_obj_t* obj);
bool lv_plugin_obj_is_imgbtn(lv_obj_t* obj);
bool lv_plugin_obj_is_cont(lv_obj_t* obj);
bool lv_plugin_obj_is_btn(lv_obj_t* obj);
bool lv_plugin_obj_is_msgbox(lv_obj_t* obj);

lv_obj_t* 	lv_plugin_find_child_by_type(lv_obj_t* obj, lv_obj_t* start_child, const char* type);
lv_indev_t* lv_plugin_find_indev_by_type(lv_indev_type_t type);

lv_obj_t* lv_plugin_scr_act(void);

/**
 * Get screen handle by index.
 * @param index start from bottom(0) to top(count-1)
 * @return lv_obj_t* plug-in screen
 */
lv_obj_t* lv_plugin_scr_by_index(uint16_t index);

/**
 * Get number of opened screen.
 * @return uint16_t number of opened screen
 */
uint16_t lv_plugin_scr_count(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_PLUGIN_H*/
