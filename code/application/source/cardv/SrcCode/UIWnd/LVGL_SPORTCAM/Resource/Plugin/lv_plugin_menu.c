
#include "Resource/Plugin/lv_plugin_common.h"
#include "Resource/Plugin/lv_plugin_menu.h"
#include <stdlib.h>

typedef void (*btn_set_state_cb)(lv_obj_t *, lv_btn_state_t);

typedef struct {

	void* imgbtn_scr_released;	/* unfocused img for LV_BTN_STATE_RELEASED, it maps to source disabled  */
	void* imgbtn_scr_focused;	/* focused img for LV_BTN_STATE_RELEASED, it maps to source released */

} lv_plugin_menu_btn_user_data;

typedef struct {
	lv_plugin_res_id string_id[LV_PLUGIN_MENU_ITEM_VISIBLE_STATE_NUM];
	lv_plugin_res_id img_id[LV_PLUGIN_MENU_ITEM_VISIBLE_STATE_NUM];
	lv_plugin_menu_item_stat_t state;
	void* user_data;
} lv_plugin_menu_item_t;

typedef struct {
    LV_PLUGIN_MENU_BASE_EXT base_ext;

    uint8_t btn_cnt;
    lv_obj_t** btns;
    lv_plugin_menu_item_t* items;
    uint8_t item_cnt;
    uint8_t first_displayed_item;
    uint8_t selected_item;
    uint8_t old_selected_item;
    int16_t displayed_item_offset_forward;
    int16_t displayed_item_offset_backward;
    bool is_wrap;
    bool focus_event_notified;
    lv_coord_t pressed_offset_x;
    lv_coord_t pressed_offset_y;
	lv_plugin_menu_scroll_mode scroll_mode;
} lv_plugin_menu_ext_t;

static LV_PLUG_RET _lv_plugin_menu_set_first_displayed_item(lv_plugin_menu_ext_t* ext, int16_t offset);
static LV_PLUG_RET _lv_plugin_menu_select_next_item(lv_plugin_menu_ext_t* ext);
static LV_PLUG_RET _lv_plugin_menu_select_prev_item(lv_plugin_menu_ext_t* ext);
static LV_PLUG_RET _lv_plugin_menu_calculate_distance(lv_plugin_menu_ext_t* ext, uint8_t idx, int16_t* distance);
static uint8_t 	   _lv_plugin_menu_find_visible_item_index(lv_plugin_menu_ext_t* ext, uint8_t start, int16_t offset);
static LV_PLUG_RET _lv_plugin_menu_select_item(lv_plugin_menu_ext_t* ext, uint8_t index);
static LV_PLUG_RET _lv_plugin_menu_select_first_item(lv_plugin_menu_ext_t* ext);
static LV_PLUG_RET _lv_plugin_menu_select_last_item(lv_plugin_menu_ext_t* ext);
static void _lv_plugin_menu_navigate_next(lv_plugin_menu_ext_t* ext);
static void _lv_plugin_menu_navigate_prev(lv_plugin_menu_ext_t* ext);
static void _lv_plugin_menu_refresh_items(lv_plugin_menu_ext_t* ext);
static void _lv_plugin_menu_select_btn(lv_obj_t* menu, lv_obj_t* btn);
static void _lv_plugin_menu_send_item_focus_event(lv_obj_t* menu);
static void _lv_plugin_menu_send_item_selected_event(lv_obj_t* menu);
static LV_PLUG_RET _lv_plugin_menu_uninit_items(lv_plugin_menu_ext_t* ext);
static LV_PLUG_RET _lv_plugin_menu_calculate_distance_ext(lv_plugin_menu_ext_t* ext, uint8_t start, uint8_t end, int16_t* distance);

static lv_signal_cb_t old_btn_signal_cb = NULL;
static lv_signal_cb_t old_imgbtn_signal_cb = NULL;
static lv_signal_cb_t old_menu_signal_cb = NULL;

static void _lv_plugin_label_set_offset(lv_obj_t* label, lv_coord_t x, lv_coord_t y)
{
	lv_label_ext_t* ext = lv_obj_get_ext_attr(label);

	lv_label_long_mode_t mode = lv_label_get_long_mode(label);

	if (mode != LV_LABEL_LONG_SROLL && mode != LV_LABEL_LONG_SROLL_CIRC) {
        ext->offset.x = x;
        ext->offset.y = y;
		lv_obj_invalidate(label);
	}
}


static void _lv_plugin_menu_select_btn(lv_obj_t* menu, lv_obj_t* btn)
{
	lv_plugin_menu_ext_t* ext = menu->ext_attr;
	uint8_t idx;
	bool is_found = false;

	for(idx = 0 ; idx < ext->btn_cnt ; idx++)
	{
		if(ext->btns[idx] == btn){
			is_found = true;
			break;
		}
	}

	if(is_found == true){
		idx = _lv_plugin_menu_find_visible_item_index(ext, ext->first_displayed_item, idx);
		_lv_plugin_menu_select_item(ext, idx);
	}

}

static lv_res_t lv_plugin_menu_imgbtn_signal(lv_obj_t * btn, lv_signal_t sign, void * param)
{
	if(sign == LV_SIGNAL_PRESSED || sign == LV_SIGNAL_RELEASED){

		lv_obj_t* menu = lv_obj_get_parent(btn);

		if(menu){

			if(sign == LV_SIGNAL_PRESSED)
				_lv_plugin_menu_select_btn(menu, btn);

			lv_signal_cb_t cb = lv_obj_get_signal_cb(menu);
			lv_res_t res = cb(menu, sign, param);
			return res;
		}

	}


	return old_imgbtn_signal_cb(btn, sign, param);
}

static lv_res_t lv_plugin_menu_btn_signal(lv_obj_t * btn, lv_signal_t sign, void * param)
{
	if(sign == LV_SIGNAL_PRESSED || sign == LV_SIGNAL_RELEASED){

		lv_obj_t* menu = lv_obj_get_parent(btn);

		if(menu){

			if(sign == LV_SIGNAL_PRESSED)
				_lv_plugin_menu_select_btn(menu, btn);

			lv_signal_cb_t cb = lv_obj_get_signal_cb(menu);
			lv_res_t res = cb(menu, sign, param);
			return res;
		}

	}

	return old_btn_signal_cb(btn, sign, param);

}

static void _lv_plugin_menu_send_item_selected_event(lv_obj_t* menu)
{
	lv_plugin_menu_ext_t* ext = menu->ext_attr;

	if (ext->items) {

		lv_plugin_menu_event_data_t ev_data = { 0 };
		lv_plugin_menu_item_t* item = &ext->items[ext->selected_item];

		ev_data.user_data = item->user_data;
		ev_data.string_id = item->string_id[item->state];
		ev_data.img_id = item->img_id[item->state];
		ev_data.state = item->state;
		ev_data.item_idx = ext->selected_item;

		lv_event_send(menu, LV_PLUGIN_EVENT_MENU_ITEM_SELECTED, &ev_data);
	}
}

static void _lv_plugin_menu_send_item_focus_event(lv_obj_t* menu)
{
	lv_plugin_menu_ext_t* ext = menu->ext_attr;
	lv_plugin_menu_event_data_t ev_data = {0};

	if(ext->focus_event_notified == true)
		return;

	lv_plugin_menu_item_t* item = NULL;


    if(ext->selected_item != LV_PLUGIN_MENU_ITEM_NONE && ext->selected_item <= ext->item_cnt){

        item = &ext->items[ext->selected_item];

        if(item->state == LV_PLUGIN_MENU_ITEM_STATE_FOCUSED){
            ev_data.user_data = item->user_data;
            ev_data.string_id = item->string_id[item->state];
            ev_data.img_id = item->img_id[item->state];
            ev_data.state = item->state;
			ev_data.item_idx = ext->selected_item;

            lv_event_send(menu, LV_PLUGIN_EVENT_MENU_ITEM_FOCUSED, &ev_data);
        }
    }


    if(ext->old_selected_item != LV_PLUGIN_MENU_ITEM_NONE && ext->old_selected_item <= ext->item_cnt){

        item = &ext->items[ext->old_selected_item];

        if(item->state == LV_PLUGIN_MENU_ITEM_STATE_RELEASED){

            ev_data.user_data = item->user_data;
            ev_data.string_id = item->string_id[item->state];
            ev_data.img_id = item->img_id[item->state];
            ev_data.state = item->state;
			ev_data.item_idx = ext->old_selected_item;

            lv_event_send(menu, LV_PLUGIN_EVENT_MENU_ITEM_DEFOCUSED,  &ev_data);
        }
    }

	ext->focus_event_notified = true;
}

static lv_res_t lv_plugin_menu_signal(lv_obj_t * menu, lv_signal_t sign, void * param)
{
	if(NULL == menu->ext_attr)
		return LV_RES_INV;

	lv_plugin_menu_ext_t* ext = menu->ext_attr;


	if(sign == LV_SIGNAL_CONTROL) {

#if LV_USE_GROUP
	   char c = *((char *)param);
	   if(c == LV_KEY_RIGHT && (NULL != ext->items)){

		   if (lv_plugin_menu_is_last_item(menu, ext->selected_item))
			   lv_event_send(menu, LV_PLUGIN_EVENT_MENU_NOTIFY_END_OF_ITEM, NULL);

		   _lv_plugin_menu_navigate_next(ext);
		   _lv_plugin_menu_refresh_items(ext);
		   _lv_plugin_menu_send_item_focus_event(menu);
	   }
	   else if(c == LV_KEY_LEFT && (NULL != ext->items)) {

		   if (lv_plugin_menu_is_first_item(menu, ext->selected_item))
			   lv_event_send(menu, LV_PLUGIN_EVENT_MENU_NOTIFY_END_OF_ITEM, NULL);

		   _lv_plugin_menu_navigate_prev(ext);
		   _lv_plugin_menu_refresh_items(ext);
		   _lv_plugin_menu_send_item_focus_event(menu);
	   }
#endif
	}
	else if(sign == LV_SIGNAL_FOCUS){

//        if(NULL != ext->items){

//            if(ext->selected_item <= ext->item_cnt){

//				if (ext->items[ext->selected_item].state != LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE) {
//					ext->focus_event_notified = false;
//					ext->items[ext->selected_item].state = LV_PLUGIN_MENU_ITEM_STATE_FOCUSED;
//					_lv_plugin_menu_refresh_items(ext);
//					_lv_plugin_menu_send_item_focus_event(menu);
//				}
//            }
//        }
	}
	else if(sign == LV_SIGNAL_DEFOCUS){

//        if(NULL != ext->items){

//            if(ext->selected_item <= ext->item_cnt){

//				if (ext->items[ext->selected_item].state != LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE) {
//					ext->focus_event_notified = false;
//					ext->old_selected_item = ext->selected_item;
//                    ext->items[ext->selected_item].state = LV_PLUGIN_MENU_ITEM_STATE_RELEASED;
//					_lv_plugin_menu_refresh_items(ext);
//					_lv_plugin_menu_send_item_focus_event(menu);
//				}
//            }
//        }
	}
	else if(sign == LV_SIGNAL_PRESSED){

        lv_indev_type_t indev_type = lv_indev_get_type(lv_indev_get_act());
        if(indev_type == LV_INDEV_TYPE_POINTER || indev_type == LV_INDEV_TYPE_BUTTON) {

        	lv_point_t p;
            bool is_found = false;
            /*Search the pressed area*/
            lv_indev_get_point(param, &p);

            for(uint8_t i=0 ; i<ext->btn_cnt ; i++)
            {

            	if(lv_obj_is_point_on_coords(ext->btns[i], &p)){
            		is_found = true;
            		break;
            	}
            }

            if(!is_found)
            	return LV_RES_OK;
        }

        if(NULL != ext->items){

			if (ext->items[ext->selected_item].state != LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE) {
				ext->items[ext->selected_item].state = LV_PLUGIN_MENU_ITEM_STATE_PRESSED;
				_lv_plugin_menu_refresh_items(ext);
				_lv_plugin_menu_send_item_focus_event(menu);
			}
        }
	}
	else if(sign == LV_SIGNAL_RELEASED){

		lv_indev_type_t indev_type = lv_indev_get_type(lv_indev_get_act());
		if (indev_type == LV_INDEV_TYPE_POINTER || indev_type == LV_INDEV_TYPE_BUTTON) {

			lv_point_t p;
			bool is_found = false;
			/*Search the pressed area*/
			lv_indev_get_point(param, &p);

			for (uint8_t i = 0; i < ext->btn_cnt; i++)
			{

				if (lv_obj_is_point_on_coords(ext->btns[i], &p)) {
					is_found = true;
					break;
				}
			}

			if (!is_found)
				return LV_RES_OK;
		}

		if(NULL != ext->items){
			
			if (ext->items[ext->selected_item].state != LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE) {
				ext->items[ext->selected_item].state = LV_PLUGIN_MENU_ITEM_STATE_FOCUSED;
				_lv_plugin_menu_refresh_items(ext);
				_lv_plugin_menu_send_item_selected_event(menu);
			}
		}
	}
	else{

		if(sign == LV_SIGNAL_CLEANUP){

			if (ext->items) {
				_lv_plugin_menu_uninit_items(ext);
			}
		}

		return old_menu_signal_cb(menu, sign, param);
	}


	return LV_RES_OK;
}

static uint8_t _lv_plugin_menu_find_visible_item_index(lv_plugin_menu_ext_t* ext, uint8_t start, int16_t offset)
{
	uint8_t	ret = start;

	if(offset < 0){

		uint8_t idx = start;

		while(offset++)
		{

			while(true)
			{
				if(idx == 0)
					break;

				idx--;

				if((ext->items[idx].state != LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE)){
					ret = idx;
					break;
				}
			}
		}

	}
	else if(offset > 0){

		uint8_t idx = start;

		while(offset--)
		{

			while(true)
			{
				if(++idx >= ext->item_cnt)
					break;

				if((ext->items[idx].state != LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE)){
					ret = idx;
					break;
				}
			}
		}
	}

	return ret;
}

LV_PLUG_RET lv_plugin_menu_select_item(lv_obj_t* menu, uint8_t index)
{
	LV_PLUG_RET ret = LV_PLUG_SUCCESS;
	lv_plugin_menu_ext_t* ext = menu->ext_attr;
	int16_t distance;

	switch (ext->scroll_mode)
	{
	case LV_PLUGIN_MENU_SCROLL_MODE_PAGE:
	{
        /* calculate visible item distance from first to target */
        ret = _lv_plugin_menu_calculate_distance_ext(ext, lv_plugin_menu_first_item_index(menu), index, &distance);
        if (ret != LV_PLUG_SUCCESS)
            return ret;

		/* select target */
		ret = _lv_plugin_menu_select_item(ext, index);
		if (ret != LV_PLUG_SUCCESS)
			return ret;

		/* move first displayed item divided by page */
		ret = _lv_plugin_menu_set_first_displayed_item(ext, -(abs(distance) % ext->btn_cnt));
		if (ret != LV_PLUG_SUCCESS)
			return ret;

		break;
	}

	case LV_PLUGIN_MENU_SCROLL_MODE_STEP:
	case LV_PLUGIN_MENU_SCROLL_MODE_CUSTOM:
	{
		/* move selected item */
		ret = _lv_plugin_menu_select_item(ext, index);
		if (ret != LV_PLUG_SUCCESS)
			return ret;

		/* calculate  distance from selected item to first displayed item */
		ret = _lv_plugin_menu_calculate_distance(ext, ext->first_displayed_item, &distance);
		if (ret != LV_PLUG_SUCCESS)
			return ret;

		/* move first displayed item if selected item out of page */
		if (abs(distance) > (ext->btn_cnt - 1))
			ret = _lv_plugin_menu_set_first_displayed_item(ext, -(ext->btn_cnt - 1));
		break;
	}

	}

	_lv_plugin_menu_refresh_items(ext);
	_lv_plugin_menu_send_item_selected_event(menu);

	return ret;

}


LV_PLUG_RET _lv_plugin_menu_select_item(lv_plugin_menu_ext_t* ext, uint8_t index)
{
	uint8_t old_item = ext->selected_item;

	if(ext->items){

        if(old_item < ext->item_cnt &&
                (old_item != index) &&
                ext->items[old_item].state != LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE)
		{
            ext->items[old_item].state = LV_PLUGIN_MENU_ITEM_STATE_RELEASED;
		}

		if(index < ext->item_cnt)
		{
			/* cant't select an invisible item */
			if((ext->items[index].state == LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE))
				return LV_PLUG_ERR_INVALID_OBJ;

			ext->old_selected_item = old_item;
			ext->selected_item = index;
			ext->focus_event_notified = false;
			ext->items[ext->selected_item].state = LV_PLUGIN_MENU_ITEM_STATE_FOCUSED;
		}
	}
	else{
		ext->selected_item = 0;
		ext->old_selected_item = 0;
	}

	return LV_PLUG_SUCCESS;
}

LV_PLUG_RET lv_plugin_menu_select_next_item(lv_obj_t* menu)
{
	lv_plugin_menu_ext_t* ext = menu->ext_attr;

	_lv_plugin_menu_navigate_next(ext);
	_lv_plugin_menu_refresh_items(ext);
	_lv_plugin_menu_send_item_selected_event(menu);

	return LV_PLUG_SUCCESS;


}

static LV_PLUG_RET _lv_plugin_menu_select_next_item(lv_plugin_menu_ext_t* ext)
{
	uint8_t idx = _lv_plugin_menu_find_visible_item_index(ext, ext->selected_item, 1);
	return _lv_plugin_menu_select_item(ext, idx);
}

LV_PLUG_RET lv_plugin_menu_select_prev_item(lv_obj_t* menu)
{
	lv_plugin_menu_ext_t* ext = menu->ext_attr;

	_lv_plugin_menu_navigate_prev(ext);
	_lv_plugin_menu_refresh_items(ext);
	_lv_plugin_menu_send_item_selected_event(menu);
	return LV_PLUG_SUCCESS;
}

LV_PLUG_RET lv_plugin_menu_select_first_item(lv_obj_t *menu)
{
    return lv_plugin_menu_select_item(menu, lv_plugin_menu_first_item_index(menu));
}

static LV_PLUG_RET _lv_plugin_menu_select_first_item(lv_plugin_menu_ext_t* ext)
{
    LV_PLUG_RET ret = LV_PLUG_SUCCESS;

	for(uint8_t idx = 0 ; idx < ext->item_cnt ; idx++)
	{
		ret = _lv_plugin_menu_select_item(ext, idx);
		if(ret == LV_PLUG_SUCCESS)
			break;
	}

	return ret;
}

LV_PLUG_RET lv_plugin_menu_select_last_item(lv_obj_t* menu)
{
    return lv_plugin_menu_select_item(menu, lv_plugin_menu_last_item_index(menu));
}

static LV_PLUG_RET _lv_plugin_menu_select_last_item(lv_plugin_menu_ext_t* ext)
{
    LV_PLUG_RET ret = LV_PLUG_SUCCESS;

	for(int16_t idx = ext->item_cnt - 1 ; idx >= 0 ; idx--)
	{
		ret = _lv_plugin_menu_select_item(ext, idx);
		if(ret == LV_PLUG_SUCCESS)
			break;
	}

	return ret;
}

static LV_PLUG_RET _lv_plugin_menu_select_prev_item(lv_plugin_menu_ext_t* ext)
{
	uint8_t idx = _lv_plugin_menu_find_visible_item_index(ext, ext->selected_item, -1);
	return _lv_plugin_menu_select_item(ext, idx);
}


LV_PLUG_RET lv_plugin_menu_set_first_displayed_item(lv_obj_t * menu, int16_t offset)
{
	if(NULL == menu->ext_attr)
		return LV_RES_INV;

	lv_plugin_menu_ext_t* ext = menu->ext_attr;

	return _lv_plugin_menu_set_first_displayed_item(ext, offset);
}



LV_PLUG_RET lv_plugin_menu_set_displayed_item_forward_offset(lv_obj_t *menu, int16_t offset)
{
    if(NULL == menu->ext_attr)
        return LV_PLUG_ERR_OBJ_EXT_NOT_ALLOCATED;

    lv_plugin_menu_ext_t* ext = menu->ext_attr;

    ext->displayed_item_offset_forward = offset;

    return LV_PLUG_SUCCESS;
}

LV_PLUG_RET lv_plugin_menu_set_displayed_item_backward_offset(lv_obj_t *menu, int16_t offset)
{
    if(NULL == menu->ext_attr)
        return LV_PLUG_ERR_OBJ_EXT_NOT_ALLOCATED;

    lv_plugin_menu_ext_t* ext = menu->ext_attr;
    ext->displayed_item_offset_backward = offset;

    return LV_PLUG_SUCCESS;
}

uint8_t lv_plugin_menu_item_cnt(lv_obj_t *menu)
{
	if(NULL == menu->ext_attr)
		return LV_PLUG_ERR_OBJ_EXT_NOT_ALLOCATED;

	lv_plugin_menu_ext_t* ext = menu->ext_attr;

    return ext->item_cnt;
}

LV_PLUG_RET lv_plugin_menu_displayed_item_cnt(lv_obj_t *menu, uint8_t *cnt)
{
	if(NULL == menu->ext_attr)
		return LV_PLUG_ERR_OBJ_EXT_NOT_ALLOCATED;

	lv_plugin_menu_ext_t* ext = menu->ext_attr;
	*cnt = ext->btn_cnt;

	return LV_PLUG_SUCCESS;
}

int16_t lv_plugin_menu_selected_item_index(lv_obj_t *menu)
{
	if(NULL == menu->ext_attr)
		return -1;

	lv_plugin_menu_ext_t* ext = menu->ext_attr;

	return ext->selected_item;
}

int16_t lv_plugin_menu_first_item_index(lv_obj_t *menu)
{
    if (NULL == menu->ext_attr)
        return -1;

    lv_plugin_menu_ext_t* ext = menu->ext_attr;
    int16_t idx = 0;


    while(idx < ext->item_cnt)
    {

        if((ext->items[idx].state != LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE)){
            return idx;
        }

        idx++;
    }

    return -1;
}

int16_t lv_plugin_menu_last_item_index(lv_obj_t *menu)
{
    if (NULL == menu->ext_attr)
        return -1;


    lv_plugin_menu_ext_t* ext = menu->ext_attr;
    int16_t idx = ext->item_cnt - 1;

    while(idx >= 0)
    {
        if((ext->items[idx].state != LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE)){
            return idx;
        }

        idx--;
    }

    return -1;

}

bool lv_plugin_menu_is_first_item(lv_obj_t* menu, uint8_t idx)
{
	if (NULL == menu->ext_attr)
		return -1;

	lv_plugin_menu_ext_t* ext = menu->ext_attr;

	uint8_t res = _lv_plugin_menu_find_visible_item_index(ext, idx, -1);
	return (res == idx) ? true : false;

}

bool lv_plugin_menu_is_last_item(lv_obj_t* menu, uint8_t idx)
{
	if (NULL == menu->ext_attr)
		return -1;

	lv_plugin_menu_ext_t* ext = menu->ext_attr;

	uint8_t res = _lv_plugin_menu_find_visible_item_index(ext, idx, 1);

	return (res == idx) ? true : false;
}

LV_PLUG_RET lv_plugin_menu_clear_selected_item(lv_obj_t *menu)
{
    if(NULL == menu->ext_attr)
        return LV_PLUG_ERR_OBJ_EXT_NOT_ALLOCATED;

    lv_plugin_menu_ext_t* ext = menu->ext_attr;
    lv_plugin_menu_item_t* item = &ext->items[ext->selected_item];

    if(item->state == LV_PLUGIN_MENU_ITEM_STATE_FOCUSED || item->state == LV_PLUGIN_MENU_ITEM_STATE_PRESSED){

        ext->focus_event_notified = false;
        ext->old_selected_item = ext->selected_item;
        item->state = LV_PLUGIN_MENU_ITEM_STATE_RELEASED;
        _lv_plugin_menu_refresh_items(ext);
        _lv_plugin_menu_send_item_focus_event(menu);
    }

    return LV_PLUG_SUCCESS;
}

int16_t lv_plugin_menu_first_displayed_item_index(lv_obj_t *menu)
{
	if(NULL == menu->ext_attr)
		return -1;

	lv_plugin_menu_ext_t* ext = menu->ext_attr;

	return ext->first_displayed_item;
}

static LV_PLUG_RET _lv_plugin_menu_set_first_displayed_item(lv_plugin_menu_ext_t* ext, int16_t offset)
{
    LV_PLUG_RET ret = LV_PLUG_SUCCESS;
	int16_t distance;
	uint8_t idx;

	idx = _lv_plugin_menu_find_visible_item_index(ext, ext->selected_item, offset);
	ret = _lv_plugin_menu_calculate_distance(ext, idx, &distance);
	if(ret != LV_PLUG_SUCCESS)
		return ret;

	if( distance < ext->btn_cnt){
		ext->first_displayed_item = idx;
		return LV_PLUG_SUCCESS;
	}
	else{
		ext->first_displayed_item = ext->selected_item;
		return LV_PLUG_SUCCESS;
	}
}

LV_PLUG_RET lv_plugin_menu_init_items(lv_obj_t* menu, uint8_t item_cnt)
{
	LV_ASSERT_OBJ(menu, LV_PLUGIN_MENU_OBJ_NAME)

	lv_plugin_menu_ext_t* ext = menu->ext_attr;

	if (ext->items != NULL) {
		LV_LOG_WARN("already allocated!");
		return LV_PLUG_ERR_MENU_ALREADY_ALLOCATED;
	}

    if(item_cnt == 0){
        LV_LOG_ERROR("item_cnt can't be zero!");
        return LV_PLUG_ERR_INVALID_PARAM;
    }

	ext->items = (lv_plugin_menu_item_t*)lv_mem_alloc(sizeof(lv_plugin_menu_item_t) * item_cnt);
    _lv_memset_00(ext->items, sizeof(lv_plugin_menu_item_t) * item_cnt);
	ext->item_cnt = item_cnt;
	_lv_plugin_menu_select_item(ext, 0);
	_lv_plugin_menu_set_first_displayed_item(ext, 0);
	_lv_plugin_menu_refresh_items(ext);

	return LV_PLUG_SUCCESS;
}

LV_PLUG_RET _lv_plugin_menu_uninit_items(lv_plugin_menu_ext_t* ext)
{
	if (ext->items == NULL) {
		LV_LOG_WARN("not allocated!");
		return LV_PLUG_ERR_MENU_NOT_ALLOCATED;
	}

	lv_mem_free(ext->items);
	ext->items = NULL;
	ext->item_cnt = 0;

	return LV_PLUG_SUCCESS;
}

LV_PLUG_RET lv_plugin_menu_uninit_items(lv_obj_t* menu)
{
	LV_ASSERT_OBJ(menu, LV_PLUGIN_MENU_OBJ_NAME)

	lv_plugin_menu_ext_t* ext = menu->ext_attr;
	
	return _lv_plugin_menu_uninit_items(ext);
}


LV_PLUG_RET _lv_plugin_menu_set_item_hidden(lv_plugin_menu_ext_t* ext, uint8_t item_idx, bool is_hidden)
{
	if (ext->items == NULL || item_idx >= ext->item_cnt) {
		LV_LOG_ERROR("out of index!");
		return LV_PLUG_ERR_MENU_OUT_OF_IDX;
	}

	lv_plugin_menu_item_t* data = &ext->items[item_idx];

	if (is_hidden) {
		data->state = LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE;
	}
	else {
		data->state = LV_PLUGIN_MENU_ITEM_STATE_RELEASED;
	}

	return LV_PLUG_SUCCESS;
}

LV_PLUG_RET	lv_plugin_menu_set_item_hidden(lv_obj_t* menu, uint8_t item_idx, bool is_hidden)
{
	LV_ASSERT_OBJ(menu, LV_PLUGIN_MENU_OBJ_NAME)

	lv_plugin_menu_ext_t* ext = menu->ext_attr;
	LV_PLUG_RET ret = LV_PLUG_SUCCESS;

	if (item_idx == LV_PLUGIN_MENU_ITEM_MAX) {

		for (int i=0 ; i<ext->item_cnt ; i++)
		{
			ret |= _lv_plugin_menu_set_item_hidden(ext, i , is_hidden);
		}
	}
	else {

		ret = _lv_plugin_menu_set_item_hidden(ext, item_idx, is_hidden);
	}

	_lv_plugin_menu_refresh_items(ext);

	return ret;
}

const lv_plugin_menu_item_t* lv_plugin_menu_items(lv_obj_t* menu)
{
	LV_ASSERT_OBJ(menu, LV_PLUGIN_MENU_OBJ_NAME)

	lv_plugin_menu_ext_t* ext = menu->ext_attr;

	return ext->items;
}

LV_PLUG_RET lv_plugin_menu_set_item_user_data(lv_obj_t* menu, uint8_t item_idx, void* user_sata)
{
    LV_ASSERT_OBJ(menu, LV_PLUGIN_MENU_OBJ_NAME)

    lv_plugin_menu_ext_t* ext = menu->ext_attr;

    if (ext->items == NULL || item_idx >= ext->item_cnt){
        LV_LOG_ERROR("out of index!");
        return LV_PLUG_ERR_MENU_OUT_OF_IDX;
    }

    ext->items[item_idx].user_data = user_sata;

    return LV_PLUG_SUCCESS;
}

void* lv_plugin_menu_item_user_data(lv_obj_t* menu, uint8_t item_idx)
{
    LV_ASSERT_OBJ(menu, LV_PLUGIN_MENU_OBJ_NAME)

    lv_plugin_menu_ext_t* ext = menu->ext_attr;

    if (ext->items == NULL || item_idx >= ext->item_cnt)
    {
        LV_LOG_ERROR("out of index!");
        return NULL;
    }

    return ext->items[item_idx].user_data;
}


LV_PLUG_RET _lv_plugin_menu_set_item_string_id(lv_plugin_menu_ext_t* ext, uint8_t item_idx, lv_plugin_menu_item_stat_t state, lv_plugin_res_id id)
{

    if (ext->items == NULL || item_idx >= ext->item_cnt){
        LV_LOG_ERROR("out of index!");
		return LV_PLUG_ERR_MENU_OUT_OF_IDX;
    }

	lv_plugin_menu_item_t* data = &ext->items[item_idx];

	if (state >= LV_PLUGIN_MENU_ITEM_VISIBLE_STATE_NUM) {

		for (uint16_t i = 0; i < LV_PLUGIN_MENU_ITEM_VISIBLE_STATE_NUM; i++)
		{
			data->string_id[i] = id;
		}
	}
	else {
		data->string_id[state] = id;
	}

	return LV_PLUG_SUCCESS;
}

LV_PLUG_RET lv_plugin_menu_set_item_string_id(lv_obj_t* menu, uint8_t item_idx, lv_plugin_menu_item_stat_t state, lv_plugin_res_id id)
{
	LV_ASSERT_OBJ(menu, LV_PLUGIN_MENU_OBJ_NAME)

	lv_plugin_menu_ext_t* ext = menu->ext_attr;
	LV_PLUG_RET ret = LV_PLUG_SUCCESS;

	if (item_idx == LV_PLUGIN_MENU_ITEM_MAX) {

		for (int i = 0; i < ext->item_cnt; i++)
		{
			ret |= _lv_plugin_menu_set_item_string_id(ext, i, state, id);
		}
	}
	else {

		ret = _lv_plugin_menu_set_item_string_id(ext, item_idx, state, id);
	}

	_lv_plugin_menu_refresh_items(ext);

	return ret;
}

LV_PLUG_RET _lv_plugin_menu_set_item_img_id(lv_plugin_menu_ext_t* ext, uint8_t item_idx, lv_plugin_menu_item_stat_t state, lv_plugin_res_id id)
{
    if (ext->items == NULL || item_idx >= ext->item_cnt){
        LV_LOG_ERROR("out of index!");
		return LV_PLUG_ERR_MENU_OUT_OF_IDX;
    }

	lv_plugin_menu_item_t* data = &ext->items[item_idx];

	if (state >= LV_PLUGIN_MENU_ITEM_VISIBLE_STATE_NUM) {

		for (uint16_t i = 0; i < LV_PLUGIN_MENU_ITEM_VISIBLE_STATE_NUM; i++)
		{
			data->img_id[i] = id;
		}
	}
	else {
		data->img_id[state] = id;
	}

	return LV_PLUG_SUCCESS;
}

LV_PLUG_RET lv_plugin_menu_set_item_img_id(lv_obj_t* menu, uint8_t item_idx, lv_plugin_menu_item_stat_t state, lv_plugin_res_id id)
{
	LV_ASSERT_OBJ(menu, LV_PLUGIN_MENU_OBJ_NAME)

	lv_plugin_menu_ext_t* ext = menu->ext_attr;

	LV_PLUG_RET ret = LV_PLUG_SUCCESS;

    if(id == LV_PLUGIN_RES_ID_NONE){
        LV_LOG_ERROR("image id can't be NONE!");
        return LV_PLUG_ERR_INVALID_PARAM;
    }

	if (item_idx == LV_PLUGIN_MENU_ITEM_MAX) {

		for (int i = 0; i < ext->item_cnt; i++)
		{
			ret |= _lv_plugin_menu_set_item_img_id(ext, i, state, id);
		}
	}
	else {

		ret = _lv_plugin_menu_set_item_img_id(ext, item_idx, state, id);
	}

	_lv_plugin_menu_refresh_items(ext);

	return ret;
}

void lv_plugin_menu_set_wrap(lv_obj_t* menu, bool en)
{
	LV_ASSERT_OBJ(menu, LV_PLUGIN_MENU_OBJ_NAME)

	lv_plugin_menu_ext_t* ext = menu->ext_attr;

    ext->is_wrap = en;
}

void lv_plugin_menu_set_scroll_mode(lv_obj_t* menu, lv_plugin_menu_scroll_mode scroll_mode)
{
	LV_ASSERT_OBJ(menu, LV_PLUGIN_MENU_OBJ_NAME)

	lv_plugin_menu_ext_t* ext = menu->ext_attr;
	ext->scroll_mode = scroll_mode;
}

lv_plugin_menu_scroll_mode lv_plugin_menu_get_scroll_mode(lv_obj_t* menu)
{
    LV_ASSERT_OBJ(menu, LV_PLUGIN_MENU_OBJ_NAME)

    lv_plugin_menu_ext_t* ext = menu->ext_attr;
    return ext->scroll_mode;
}

void _lv_plugin_menu_apply_item_state(lv_obj_t* btn, lv_plugin_menu_item_t* item)
{
	if(item == NULL){

		lv_obj_set_hidden(btn, true);

	}
	else {

		lv_obj_set_hidden(btn, false);

		lv_plugin_res_id img_id = item->img_id[item->state];
		lv_plugin_res_id string_id = item->string_id[item->state];

		lv_obj_t* parent = lv_obj_get_parent(btn);
		lv_plugin_menu_ext_t* ext = lv_obj_get_ext_attr(parent);


		lv_obj_t* label = lv_plugin_find_child_by_type(btn, NULL, lv_plugin_label_type_name());
		lv_obj_t* img = lv_plugin_find_child_by_type(btn, NULL, lv_plugin_img_type_name());
		lv_coord_t offset_x = 0, offset_y = 0;

		if (label) {
			lv_plugin_label_allocate_ext_attr(label);
			lv_plugin_label_set_text(label, string_id);
            lv_plugin_label_update_font(label, LV_OBJ_PART_MAIN);

            switch(item->state)
            {

            case LV_PLUGIN_MENU_ITEM_STATE_FOCUSED:
                lv_obj_set_state(label, LV_STATE_FOCUSED);
                break;

            case LV_PLUGIN_MENU_ITEM_STATE_DISABLED:
                lv_obj_set_state(label,  LV_STATE_DISABLED);
                break;

            case LV_PLUGIN_MENU_ITEM_STATE_PRESSED:
                lv_obj_set_state(label, LV_STATE_PRESSED);
                break;

            case LV_PLUGIN_MENU_ITEM_STATE_RELEASED:
            default:
                lv_obj_set_state(label, LV_STATE_DEFAULT);
                break;
            }
		}

        if (img && img_id != LV_PLUGIN_RES_ID_NONE) {
			lv_plugin_img_set_src(img, img_id);

            switch(item->state)
            {

            case LV_PLUGIN_MENU_ITEM_STATE_FOCUSED:
                lv_obj_set_state(img, LV_STATE_FOCUSED);
                break;

            case LV_PLUGIN_MENU_ITEM_STATE_DISABLED:
                lv_obj_set_state(img, LV_STATE_DISABLED);
                break;

            case LV_PLUGIN_MENU_ITEM_STATE_PRESSED:
                lv_obj_set_state(img, LV_STATE_PRESSED);
                break;

            case LV_PLUGIN_MENU_ITEM_STATE_RELEASED:
            default:
                lv_obj_set_state(img, LV_STATE_DEFAULT);
                break;
            }
		}


		btn_set_state_cb set_state_cb;

		if(lv_plugin_obj_is_btn(btn)){
			set_state_cb = lv_btn_set_state;
		}
		else if(lv_plugin_obj_is_imgbtn(btn)){
			set_state_cb = lv_imgbtn_set_state;
		}
		else{
			set_state_cb = lv_btn_set_state;
		}

		switch (item->state)
		{
		case LV_PLUGIN_MENU_ITEM_STATE_RELEASED:

			if (lv_plugin_obj_is_imgbtn(btn)) 
			{
				lv_plugin_menu_btn_user_data *user_data = lv_obj_get_user_data(btn);
				lv_imgbtn_set_src(btn, LV_BTN_STATE_RELEASED, user_data->imgbtn_scr_released);
			}

			set_state_cb(btn, LV_BTN_STATE_RELEASED);
			lv_obj_clear_state(btn, LV_STATE_FOCUSED);
			break;

		case LV_PLUGIN_MENU_ITEM_STATE_FOCUSED:

			if (lv_plugin_obj_is_imgbtn(btn))
			{
				lv_plugin_menu_btn_user_data* user_data = lv_obj_get_user_data(btn);
				lv_imgbtn_set_src(btn, LV_BTN_STATE_RELEASED, user_data->imgbtn_scr_focused);
			}

			set_state_cb(btn, LV_BTN_STATE_RELEASED);
			lv_obj_add_state(btn, LV_STATE_FOCUSED);
			break;

		case LV_PLUGIN_MENU_ITEM_STATE_PRESSED:
			set_state_cb(btn, LV_BTN_STATE_PRESSED);

			offset_x = ext->pressed_offset_x;
			offset_y = ext->pressed_offset_y;
			break;

			/* disabled object can't obtain focus */
		case LV_PLUGIN_MENU_ITEM_STATE_DISABLED:
			set_state_cb(btn, LV_BTN_STATE_DISABLED);
			lv_obj_set_state(btn, LV_STATE_DISABLED);
			break;

		default:
			break;
		}

		if (img) {
			lv_img_set_offset_x(img, offset_x);
			lv_img_set_offset_y(img, offset_y);
		}

		if (label) {
			_lv_plugin_label_set_offset(label, offset_x, offset_y);
		}
	}

	lv_obj_invalidate(btn);
}

static LV_PLUG_RET _lv_plugin_menu_calculate_distance(lv_plugin_menu_ext_t* ext, uint8_t idx, int16_t* distance)
{
	uint8_t start, end ;
    int16_t factor, cnt = 0;

    if(idx >= ext->item_cnt || ext->selected_item >= ext->item_cnt){
        LV_LOG_ERROR("out of index!");
		return LV_PLUG_ERR_MENU_OUT_OF_IDX;
    }


	if(idx > ext->selected_item){
		start = ext->selected_item;
		end = idx;
		factor = -1;
	}
	else{
		start = idx;
		end = ext->selected_item;
		factor = 1;
	}

	while(start != end)
	{
		++start;

		if(ext->items[start].state != LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE)
			cnt++;
	}


	*distance = (cnt * factor);

	return LV_PLUG_SUCCESS;
}

void _lv_plugin_menu_navigate_next(lv_plugin_menu_ext_t* ext)
{
	uint8_t selected_item_idx = ext->selected_item;
	int16_t distance;

	if(_lv_plugin_menu_calculate_distance(ext, ext->first_displayed_item, &distance) != LV_PLUG_SUCCESS)
		return;

	if(selected_item_idx == _lv_plugin_menu_find_visible_item_index(ext, selected_item_idx, 1)){
        if(ext->is_wrap){
			_lv_plugin_menu_select_first_item(ext);
			_lv_plugin_menu_calculate_distance(ext, selected_item_idx, &distance);

			/* check cross page, ignore displayed_item_offset_backward when wrapping item ocurred */
			if (distance < 0 && abs(distance) >= ext->btn_cnt) {

				switch (ext->scroll_mode)
				{
				case LV_PLUGIN_MENU_SCROLL_MODE_PAGE:
				case LV_PLUGIN_MENU_SCROLL_MODE_STEP:
					_lv_plugin_menu_set_first_displayed_item(ext, 0);
					break;

				case LV_PLUGIN_MENU_SCROLL_MODE_CUSTOM:
				default:
					_lv_plugin_menu_set_first_displayed_item(ext, ext->displayed_item_offset_backward);
					break;
				}
			}
		}
	}
	else if( distance >= 0 && distance < (ext->btn_cnt - 1))
	{
		_lv_plugin_menu_select_next_item(ext);
	}
	else{
		_lv_plugin_menu_select_next_item(ext);

		switch (ext->scroll_mode)
		{
		case LV_PLUGIN_MENU_SCROLL_MODE_PAGE:
			_lv_plugin_menu_set_first_displayed_item(ext, 0);
			break;

		case LV_PLUGIN_MENU_SCROLL_MODE_STEP:
			_lv_plugin_menu_set_first_displayed_item(ext, -(ext->btn_cnt - 1));
			break;

		case LV_PLUGIN_MENU_SCROLL_MODE_CUSTOM:
		default:
			_lv_plugin_menu_set_first_displayed_item(ext, ext->displayed_item_offset_forward);
			break;
		}
	}
}

void _lv_plugin_menu_navigate_prev(lv_plugin_menu_ext_t* ext)
{
	uint8_t selected_item_idx = ext->selected_item;
	int16_t distance;

	if(_lv_plugin_menu_calculate_distance(ext, ext->first_displayed_item, &distance) != LV_PLUG_SUCCESS)
		return;

	if(selected_item_idx == _lv_plugin_menu_find_visible_item_index(ext, selected_item_idx, -1)){

        if(ext->is_wrap){
			_lv_plugin_menu_select_last_item(ext);
			_lv_plugin_menu_calculate_distance(ext, selected_item_idx, &distance);

			/* check cross page, ignore displayed_item_offset_backward when wrapping item ocurred */
			if (distance > 0 && distance >= ext->btn_cnt) {

				switch (ext->scroll_mode)
				{
				case LV_PLUGIN_MENU_SCROLL_MODE_PAGE:
				{
					int16_t offset = -(distance % ext->btn_cnt);
					_lv_plugin_menu_set_first_displayed_item(ext, offset);
				}
				break;

				case LV_PLUGIN_MENU_SCROLL_MODE_STEP:
					_lv_plugin_menu_set_first_displayed_item(ext, -(ext->btn_cnt - 1));
					break;

				case LV_PLUGIN_MENU_SCROLL_MODE_CUSTOM:
				default:
					_lv_plugin_menu_set_first_displayed_item(ext, ext->displayed_item_offset_backward);
					break;
				}
			}		
		}
	}
	else if( distance > 0 && distance < ext->btn_cnt)
	{
		_lv_plugin_menu_select_prev_item(ext);
	}
	else{
		_lv_plugin_menu_select_prev_item(ext);

		switch (ext->scroll_mode)
		{
		case LV_PLUGIN_MENU_SCROLL_MODE_PAGE:
			_lv_plugin_menu_set_first_displayed_item(ext, -(ext->btn_cnt - 1));
			break;

		case LV_PLUGIN_MENU_SCROLL_MODE_STEP:
			_lv_plugin_menu_set_first_displayed_item(ext, 0);
			break;

		case LV_PLUGIN_MENU_SCROLL_MODE_CUSTOM:
		default:
			_lv_plugin_menu_set_first_displayed_item(ext, ext->displayed_item_offset_backward);
			break;
		}
	}
}


/* won't change the menu item index */
void _lv_plugin_menu_refresh_items(lv_plugin_menu_ext_t* ext)
{
	uint8_t curr_item_idx = ext->first_displayed_item;

	for(int i=0 ; i<ext->btn_cnt ; i++)
	{
		lv_obj_t* btn = ext->btns[i];

		/* hide all buttons if no item exists */
		if(NULL == ext->items){
			_lv_plugin_menu_apply_item_state(btn, NULL);
			continue;
		}

		/**********************************************
		 * 1. skip invisible item
		 * 2. hide button if out of index
		 **********************************************/
		while(true)
		{
			if(curr_item_idx < ext->item_cnt){

				lv_plugin_menu_item_t* item = &ext->items[curr_item_idx++];

				if(item){

					if(item->state != LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE){
						_lv_plugin_menu_apply_item_state(btn, item);
						break;
					}
					else
						continue;
				}
			}
			else{
				_lv_plugin_menu_apply_item_state(btn, NULL);
				break;
			}
		}
	}
}

lv_obj_t* lv_plugin_menu_create(lv_obj_t *parent, lv_obj_t *cont)
{
	LV_ASSERT_OBJ(cont, lv_plugin_cont_type_name())

	lv_obj_t* child = NULL;
	lv_obj_t* menu = cont;
	lv_plugin_menu_ext_t* ext = _lv_plugin_obj_allocate_ext_attr(menu, sizeof(lv_plugin_menu_ext_t));

	if(NULL == old_menu_signal_cb)
		old_menu_signal_cb = lv_obj_get_signal_cb(menu);

	ext->btns = NULL;
	ext->btn_cnt = 0;
	ext->items = NULL;
	ext->item_cnt = 0;
	ext->focus_event_notified = false;
	ext->old_selected_item = 0;
	ext->selected_item = 0;
	ext->first_displayed_item = 0;
    ext->is_wrap = false;
	ext->pressed_offset_x = 3;
	ext->pressed_offset_y = 3;
	ext->scroll_mode = LV_PLUGIN_MENU_SCROLL_MODE_PAGE;

	lv_obj_t* tmp_btns[LV_PLUGIN_MENU_BTN_MAX] = { NULL };
	child = NULL;
	while(true)
	{
        child = lv_obj_get_child_back(menu, child);

		if(child == NULL)
			break;

		if(lv_plugin_obj_is_imgbtn(child) || lv_plugin_obj_is_btn(child)){

			if(lv_plugin_obj_is_imgbtn(child)){

				if(old_imgbtn_signal_cb == NULL){
					old_imgbtn_signal_cb = lv_obj_get_signal_cb(child);
				}

				lv_obj_set_signal_cb(child, lv_plugin_menu_imgbtn_signal);
			}
			else if(lv_plugin_obj_is_btn(child)){

				if(old_btn_signal_cb == NULL){
					old_btn_signal_cb = lv_obj_get_signal_cb(child);
				}

				lv_obj_set_signal_cb(child, lv_plugin_menu_btn_signal);
			}


			tmp_btns[ext->btn_cnt] = child;
			lv_obj_set_parent_event(child, true);
			ext->btn_cnt++;
		}
	}

	ext->btns = lv_mem_alloc(sizeof(lv_obj_t*) * ext->btn_cnt);
	memcpy(ext->btns, tmp_btns, sizeof(lv_obj_t*)*ext->btn_cnt);

	lv_plugin_menu_btn_user_data* btns_user_data = (lv_plugin_menu_btn_user_data*)lv_mem_alloc(sizeof(lv_plugin_menu_btn_user_data) * ext->btn_cnt);

	for (int i=0 ; i< ext->btn_cnt ; i++)
	{
		lv_obj_t* btn = ext->btns[i];
		lv_plugin_menu_btn_user_data* btn_user_data = &btns_user_data[i];

		lv_obj_set_user_data(btn, btn_user_data);

		if (lv_plugin_obj_is_imgbtn(btn)) {

			/* image source backup */
            btn_user_data->imgbtn_scr_focused = (void*)lv_imgbtn_get_src(btn, LV_BTN_STATE_RELEASED);
            btn_user_data->imgbtn_scr_released = (void*)lv_imgbtn_get_src(btn, LV_BTN_STATE_DISABLED);

			if (btn_user_data->imgbtn_scr_released == NULL)
				btn_user_data->imgbtn_scr_released = btn_user_data->imgbtn_scr_focused;
		}
	}

    ext->displayed_item_offset_forward = 0;
    ext->displayed_item_offset_backward = -((int16_t)ext->btn_cnt - 1);

	lv_obj_set_parent(menu, parent);
	lv_obj_set_signal_cb(menu, lv_plugin_menu_signal);

	_lv_plugin_menu_refresh_items(ext);

	return menu;
}

void lv_plugin_menu_set_selected_item_pressed(lv_obj_t *menu)
{
	lv_plugin_menu_signal(menu, LV_SIGNAL_PRESSED, NULL);
}

void lv_plugin_menu_set_selected_item_released(lv_obj_t *menu)
{
	lv_plugin_menu_signal(menu, LV_SIGNAL_RELEASED, NULL);
}

void lv_plugin_menu_set_pressed_offset(lv_obj_t *menu, lv_coord_t pressed_offset_x, lv_coord_t pressed_offset_y)
{
    LV_ASSERT_OBJ(menu, LV_PLUGIN_MENU_OBJ_NAME)

    lv_plugin_menu_ext_t* ext = menu->ext_attr;

    ext->pressed_offset_x = pressed_offset_x;
    ext->pressed_offset_y = pressed_offset_y;
}

int16_t lv_plugin_menu_current_page_index(lv_obj_t *menu)
{
    lv_plugin_menu_ext_t* ext = menu->ext_attr;
    int16_t distance;
    uint8_t start = lv_plugin_menu_first_item_index(menu);
    uint8_t end = ext->selected_item;

    if(ext->scroll_mode != LV_PLUGIN_MENU_SCROLL_MODE_PAGE){
        LV_LOG_WARN("current scroll mode is not page mode");
    }

    if(_lv_plugin_menu_calculate_distance_ext(ext, start, end, &distance) != LV_PLUG_SUCCESS)
        return -1;

    return (abs(distance) / ext->btn_cnt);
}

int16_t lv_plugin_menu_page_cnt(lv_obj_t *menu)
{
    lv_plugin_menu_ext_t* ext = menu->ext_attr;
    int16_t distance;
    uint8_t start = lv_plugin_menu_first_item_index(menu);
    uint8_t end = lv_plugin_menu_last_item_index(menu);


    if(ext->scroll_mode != LV_PLUGIN_MENU_SCROLL_MODE_PAGE){
        LV_LOG_WARN("current scroll mode is not page mode");
    }

    if(_lv_plugin_menu_calculate_distance_ext(ext, start, end, &distance) != LV_PLUG_SUCCESS)
        return -1;

    return (abs(distance) / ext->btn_cnt);
}

static LV_PLUG_RET _lv_plugin_menu_calculate_distance_ext(lv_plugin_menu_ext_t* ext, uint8_t start, uint8_t end, int16_t* distance)
{
    int16_t factor, cnt = 0;

    if(start >= ext->item_cnt || end >= ext->item_cnt)
        return LV_PLUG_ERR_MENU_OUT_OF_IDX;


    if(start > end){
        uint8_t tmp;

        tmp = start;
        start = end;
        end = tmp;

        factor = -1;
    }
    else{
        factor = 1;
    }

    while(start != end)
    {
        ++start;

        if(ext->items[start].state != LV_PLUGIN_MENU_ITEM_STATE_INVISIBLE)
            cnt++;
    }


    *distance = (cnt * factor);

    return LV_PLUG_SUCCESS;
}
