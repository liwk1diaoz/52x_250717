
#ifndef LV_PLUGIN_MENU_H
#define LV_PLUGIN_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

#define LV_PLUGIN_MENU_BASE_EXT lv_cont_ext_t

typedef enum {

	LV_PLUGIN_MENU_SCROLL_MODE_PAGE,  /* shift 1 page(btn_cnt) */
	LV_PLUGIN_MENU_SCROLL_MODE_STEP,  /* shift 1 item */
	LV_PLUGIN_MENU_SCROLL_MODE_CUSTOM /* fully controlled by backward & forward offset */

} lv_plugin_menu_scroll_mode;

typedef enum {
	LV_PLUGIN_MENU_ITEM_STATE_RELEASED,
	LV_PLUGIN_MENU_ITEM_STATE_FOCUSED,
	LV_PLUGIN_MENU_ITEM_STATE_PRESSED,
	LV_PLUGIN_MENU_ITEM_STATE_DISABLED,
	LV_PLUGIN_MENU_ITEM_VISIBLE_STATE_NUM,
	LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE = LV_PLUGIN_MENU_ITEM_VISIBLE_STATE_NUM,
	LV_PLUGIN_MENU_ITEM_STATE_NUM
} lv_plugin_menu_item_stat_t;

#define LV_PLUGIN_MENU_ITEM_STATE_ALL LV_PLUGIN_MENU_ITEM_VISIBLE_STATE_NUM

typedef struct {
	lv_plugin_res_id string_id;
	lv_plugin_res_id img_id;
	lv_plugin_menu_item_stat_t state;
	void* user_data;
	uint8_t item_idx;
} lv_plugin_menu_event_data_t;


#define LV_PLUGIN_MENU_BTN_MAX 0xFF
#define LV_PLUGIN_MENU_ITEM_MAX 0xFF
#define LV_PLUGIN_MENU_BTN_NONE LV_PLUGIN_MENU_BTN_MAX
#define LV_PLUGIN_MENU_ITEM_NONE LV_PLUGIN_MENU_BTN_NONE
#define LV_PLUGIN_MENU_OBJ_NAME "lv_cont"

lv_obj_t* 	lv_plugin_menu_create(lv_obj_t *parent, lv_obj_t *cont);

LV_PLUG_RET 	lv_plugin_menu_init_items(lv_obj_t* menu, uint8_t item_cnt);
LV_PLUG_RET 	lv_plugin_menu_uninit_items(lv_obj_t* menu);
LV_PLUG_RET		lv_plugin_menu_set_item_hidden(lv_obj_t* menu, uint8_t item_idx, bool is_hidden);


/**
 *
 * Get item count, it useful to check is item already init
 *
 * @param menu
 * @return item count
 */
uint8_t lv_plugin_menu_item_cnt(lv_obj_t *menu);


/**
 *
 * Set item string id by state, state = LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE will set all state's string id
 *
 * @param menu
 * @param item_idx
 * @param state         state which string id maps to.
 * @param id            string id.
 * @return
 */
LV_PLUG_RET 	lv_plugin_menu_set_item_string_id(lv_obj_t* menu, uint8_t item_idx, lv_plugin_menu_item_stat_t state, lv_plugin_res_id id);


/**
 *
 * Set item user data
 *
 * @param menu
 * @param item_idx
 * @param user_data
 * @return
 */
LV_PLUG_RET lv_plugin_menu_set_item_user_data(lv_obj_t* menu, uint8_t item_idx, void* user_data);

/**
 *
 * Get item user data
 *
 * @param menu
 * @param item_idx
 * @return user data pointer
 */
void* lv_plugin_menu_item_user_data(lv_obj_t* menu, uint8_t item_idx);



/**
 *
 * Set item image id by state, state = LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE will set all state's image id
 *
 * @param menu
 * @param item_idx
 * @param state         state which image id maps to.
 * @param id            image id, should not be LV_PLUGIN_RES_ID_NONE.
 * @return
 */
LV_PLUG_RET 	lv_plugin_menu_set_item_img_id(lv_obj_t* menu, uint8_t item_idx, lv_plugin_menu_item_stat_t state, lv_plugin_res_id id);

/**
 *
 * Set whether focus next/prev will allow wrapping from first->last or last->first item
 *
 * @param menu
 * @param en
 */
void 		lv_plugin_menu_set_wrap(lv_obj_t* menu, bool en);

/* backward compatibility */
#define lv_plugin_menu_set_loop_item lv_plugin_menu_set_wrap

/***********************************
 * Displayed Item API
************************************/

/**
 * Scroll custom mode only
 *
 * Set first displayed item offset , it must be a negative number which means backward from selected item idx.
 * This API is unnessecary unless some special use cases like you want to change the displayed item inner page.
 *
 * @param menu
 * @param offset
 * @return LV_PLUG_RET
 */
LV_PLUG_RET lv_plugin_menu_set_first_displayed_item(lv_obj_t *menu, int16_t offset);

/** 
 * Scroll custom mode only
 *
 *
 * Set displayed item forward offset , it determines how items displayed if switching to next page and must be a negative number
 *
 * for e.g. , here are 4 displayed items and total 7 items.
 *
 *                  1
 *                  2
 *                  3
 * selected item->  4
 *
 * when you call select next item and offset equals to 0 , it would be displayed below :
 *
 * selected item->  5
 *                  6
 *                  7
 *
 * when you call select next item and offset equals to -1 , it would be displayed below :
 *
 *                  4
 * selected item->  5
 *                  6
 *                  7
 *
 * @param menu
 * @param offset
 * @return LV_PLUG_RET
 */
LV_PLUG_RET lv_plugin_menu_set_displayed_item_forward_offset(lv_obj_t *menu, int16_t offset);

/**
 * Scroll custom mode only
 *
 * Set displayed item forward offset , it determines how items displayed if switching to previous page and must be a negative number
 *
 * @param menu
 * @param offset
 * @return LV_PLUG_RET
 */
LV_PLUG_RET lv_plugin_menu_set_displayed_item_backward_offset(lv_obj_t *menu, int16_t offset);


/**
 * Set current scroll mode
 *
 * @param menu
 * @param scroll_mode
 */
void lv_plugin_menu_set_scroll_mode(lv_obj_t* menu, lv_plugin_menu_scroll_mode scroll_mode);


/**
 * Get current scroll mode
 *
 * @param menu
 * @return lv_plugin_menu_scroll_mode
 */
lv_plugin_menu_scroll_mode lv_plugin_menu_get_scroll_mode(lv_obj_t* menu);

/**
 * Focus the item
 *
 * @param menu
 * @param idx the item index to be focused
 * @return LV_PLUG_RET
 */
LV_PLUG_RET lv_plugin_menu_select_item(lv_obj_t *menu, uint8_t idx);

/**
 * Get selected item index
 *
 * @param menu
 * @return int16_t selected index, -1 is no selected item or invalid menu handle
 */
int16_t lv_plugin_menu_selected_item_index(lv_obj_t *menu);

int16_t lv_plugin_menu_first_item_index(lv_obj_t *menu);
int16_t lv_plugin_menu_last_item_index(lv_obj_t *menu);

bool lv_plugin_menu_is_first_item(lv_obj_t* menu, uint8_t idx);
bool lv_plugin_menu_is_last_item(lv_obj_t* menu, uint8_t idx);

/**
 * Deselect current item, it can be used for defocus event to deselect current item
 *
 * @param menu
 * @return LV_PLUG_RET
 */
LV_PLUG_RET lv_plugin_menu_clear_selected_item(lv_obj_t *menu);

/**
 * Get max button count can be displayed on the menu (not item count)
 *
 * @param menu
 * @param cnt return value of menu button count
 * @return LV_PLUG_RET
 */
LV_PLUG_RET lv_plugin_menu_displayed_item_cnt(lv_obj_t *menu, uint8_t *cnt);

/**
 * Get first displayed item index
 *
 * @param menu
 * @return item index
 */
int16_t lv_plugin_menu_first_displayed_item_index(lv_obj_t *menu);

/**
 * Get item state
 *
 * @param menu
 * @return lv_plugin_menu_item_stat_t
 */
lv_plugin_menu_item_stat_t lv_plugin_menu_get_item_state(lv_obj_t *menu, uint8_t idx);

/**
 * Send pressed control signal to focused item, this API is useful when the menu is not direcltly controled by lv_driver siganls.
 * The most common scenario is a screen object accepts keypad input(prev/next/press/release) and controls the menu object in the screen event callback.
 *
 * @param menu
 */
void lv_plugin_menu_set_selected_item_pressed(lv_obj_t *menu);

/**
 * Send released control signal to focused item, this API is useful when the menu is not direcltly controled by lv_driver siganls.
 * The most common scenario is a screen object accepts keypad input(prev/next/press/release) and controls the menu object in the screen event callback.
 *
 * @param menu
 */
void lv_plugin_menu_set_selected_item_released(lv_obj_t *menu);

/**
 * Select next item
 * The most common scenario is a screen object accepts keypad input(prev/next/press/release) and controls the menu object in the screen event callback.
 *
 * @param menu
 * @return LV_PLUG_RET
 */
LV_PLUG_RET lv_plugin_menu_select_next_item(lv_obj_t* menu);

/**
 * Select previous item
 * The most common scenario is a screen object accepts keypad input(prev/next/press/release) and controls the menu object in the screen event callback.
 *
 * @param menu
 * @return LV_PLUG_RET
 */
LV_PLUG_RET lv_plugin_menu_select_prev_item(lv_obj_t* menu);

/**
 * Select first item
 *
 * @param menu
 * @return LV_PLUG_RET
 */
LV_PLUG_RET lv_plugin_menu_select_first_item(lv_obj_t* menu);

/**
 * Select lasr item
 *
 * @param menu
 * @return LV_PLUG_RET
 */
LV_PLUG_RET lv_plugin_menu_select_last_item(lv_obj_t* menu);

/**
 * Set pressed offset, it affects label and image pressed offset (if exists)
 *
 * IMPORTANT:
 *
 * LVGL image (lv_img) has built-in tile prop and can't be disabled, if your image source foreground just fit background size,
 * you'll see duplicate foreground images on the top-left, to resolve it, the image source should extend background size.
 *
 * @param menu
 * @param pressed_offset_x
 * @param pressed_offset_y
 * @return LV_PLUG_RET
 */
void lv_plugin_menu_set_pressed_offset(lv_obj_t *menu, lv_coord_t pressed_offset_x, lv_coord_t pressed_offset_y);

/**
 * Scroll page mode only, get current page index.
 * This API might be helpful to show the page number on the ui
 *
 * @param menu
 * @param page index
 */
int16_t lv_plugin_menu_current_page_index(lv_obj_t *menu);

/**
 * Scroll page mode only, get page count.
 * This API might be helpful to show the total page number on the ui
 *
 * @param menu
 * @param page count
 */
int16_t lv_plugin_menu_page_cnt(lv_obj_t *menu);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_PLUGIN_MENU_H*/
