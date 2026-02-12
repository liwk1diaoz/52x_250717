#include "UIFlowLVGL/UIFlowLVGL.h"

#include "Resource/Plugin/lvgl_plugin.h"

#include <stdlib.h>


lv_obj_t *UIFlowMovie;
lv_obj_t *UIFlowPhoto;
lv_obj_t *UIFlowWrnMsg;
lv_obj_t *UIFlowPlay;
lv_obj_t *UIFlowMenuCommonItem;
lv_obj_t *UIFlowMenuCommonOption;
lv_obj_t *UIFlowMenuCommonConfirm;
lv_obj_t *UIFlowWaitMoment;
lv_obj_t *UIFlowSetupDateTime;
lv_obj_t *UIFlowPlayThumb;
lv_obj_t *UIFlowUSB;
lv_obj_t *UIFlowUSBMenu;
lv_obj_t *UIFlowWifiLink;
lv_obj_t *UIFlowWifiLinkOK;
lv_obj_t *UIFlowWifiWait;


extern lv_plugin_string_t lv_plugin_EN_string_table[];
extern lv_plugin_string_t lv_plugin_FR_string_table[];
extern lv_plugin_string_t lv_plugin_DE_string_table[];
extern lv_plugin_string_t lv_plugin_ES_string_table[];
extern lv_plugin_string_t lv_plugin_IT_string_table[];
extern lv_plugin_string_t lv_plugin_PO_string_table[];
extern lv_plugin_string_t lv_plugin_SC_string_table[];
extern lv_plugin_string_t lv_plugin_JP_string_table[];
extern lv_plugin_string_t lv_plugin_TC_string_table[];
extern lv_plugin_string_t lv_plugin_RU_string_table[];


lv_plugin_img_t lv_plugin_UIFlowLVGL_img_table[] =
{
	{ NULL },
	{ &icon_act_push_btn },
	{ &icon_act_release_btn },
	{ &icon_adas_cal },
	{ &icon_adas_cal_m },
	{ &icon_adas_off },
	{ &icon_adas_off_m },
	{ &icon_adas_on },
	{ &icon_adas_on_m },
	{ &icon_autoshutdown },
	{ &icon_autoshutdown_10min },
	{ &icon_autoshutdown_3min },
	{ &icon_autoshutdown_5min },
	{ &icon_autoshutdown_m },
	{ &icon_autoshutdown_off },
	{ &icon_backlight },
	{ &icon_backlight_high },
	{ &icon_backlight_low },
	{ &icon_backlight_m },
	{ &icon_backlight_mid },
	{ &icon_backlight_off },
	{ &icon_battery_charge },
	{ &icon_battery_empty },
	{ &icon_battery_full },
	{ &icon_battery_low },
	{ &icon_battery_med },
	{ &icon_battery_type },
	{ &icon_battery_zero },
	{ &icon_bsd_off },
	{ &icon_bsd_off_m },
	{ &icon_bsd_on },
	{ &icon_bsd_on_m },
	{ &icon_btn_emboss },
	{ &icon_bust },
	{ &icon_bust_3shot },
	{ &icon_bust_continue },
	{ &icon_bust_m },
	{ &icon_bust_off },
	{ &icon_cam_ptz_off },
	{ &icon_cam_ptz_on },
	{ &icon_cam_ptz_on_m },
	{ &icon_cancel },
	{ &icon_cancel_m },
	{ &icon_cloud_connected },
	{ &icon_cloud_connected_m },
	{ &icon_cloud_connecting },
	{ &icon_cloud_connecting_m },
	{ &icon_cloud_download1 },
	{ &icon_cloud_download2 },
	{ &icon_cloud_fail },
	{ &icon_cloud_off },
	{ &icon_cloud_off_m },
	{ &icon_cloud_on },
	{ &icon_cloud_on_m },
	{ &icon_cloud_upload1 },
	{ &icon_cloud_upload2 },
	{ &icon_color },
	{ &icon_copy_from_card },
	{ &icon_cyclic_rec },
	{ &icon_cyclic_rec_10min },
	{ &icon_cyclic_rec_15min },
	{ &icon_cyclic_rec_1min },
	{ &icon_cyclic_rec_2min },
	{ &icon_cyclic_rec_3min },
	{ &icon_cyclic_rec_5min },
	{ &icon_cyclic_rec_m },
	{ &icon_datetime_setup },
	{ &icon_datetime_setup_m },
	{ &icon_date_print },
	{ &icon_date_print_m },
	{ &icon_date_stamping },
	{ &icon_date_time },
	{ &icon_ddd_off },
	{ &icon_ddd_off_m },
	{ &icon_ddd_on },
	{ &icon_ddd_on_m },
	{ &icon_default },
	{ &icon_default_m },
	{ &icon_delete },
	{ &icon_delete_all },
	{ &icon_delete_current },
	{ &icon_delete_m },
	{ &icon_dir_down },
	{ &icon_dir_left },
	{ &icon_dir_right },
	{ &icon_dir_up },
	{ &icon_dpof },
	{ &icon_dualcam_behind },
	{ &icon_dualcam_both },
	{ &icon_dualcam_both_m },
	{ &icon_dualcam_front },
	{ &icon_ev },
	{ &icon_ev_m },
	{ &icon_ev_m0p3 },
	{ &icon_ev_m0p6 },
	{ &icon_ev_m1p0 },
	{ &icon_ev_m1p3 },
	{ &icon_ev_m1p6 },
	{ &icon_ev_m2p0 },
	{ &icon_ev_p0p0 },
	{ &icon_ev_p0p3 },
	{ &icon_ev_p0p6 },
	{ &icon_ev_p1p0 },
	{ &icon_ev_p1p3 },
	{ &icon_ev_p1p6 },
	{ &icon_ev_p2p0 },
	{ &icon_face_off },
	{ &icon_face_on },
	{ &icon_face_on_m },
	{ &icon_fb_left },
	{ &icon_fcw_far_alert },
	{ &icon_fcw_near_alert },
	{ &icon_fcw_off },
	{ &icon_fcw_off_m },
	{ &icon_fcw_on },
	{ &icon_fcw_on_m },
	{ &icon_ff_right },
	{ &icon_file_video },
	{ &icon_firmware_update },
	{ &icon_firmware_update_m },
	{ &icon_flash_auto },
	{ &icon_flash_off },
	{ &icon_flash_on },
	{ &icon_flash_on_m },
	{ &icon_flash_rec },
	{ &icon_flash_rec_m },
	{ &icon_flash_red_eye },
	{ &icon_flick_50hz },
	{ &icon_flick_60hz },
	{ &icon_format },
	{ &icon_format_m },
	{ &icon_golf_rec },
	{ &icon_golf_rec_m },
	{ &icon_gps_off },
	{ &icon_gps_on },
	{ &icon_gps_on_m },
	{ &icon_g_sensor },
	{ &icon_g_sensor_high },
	{ &icon_g_sensor_low },
	{ &icon_g_sensor_m },
	{ &icon_g_sensor_mid },
	{ &icon_g_sensor_off },
	{ &icon_hdr },
	{ &icon_hdr_m },
	{ &icon_hdr_off },
	{ &icon_help },
	{ &icon_help_m },
	{ &icon_hz },
	{ &icon_hz_50 },
	{ &icon_hz_60 },
	{ &icon_hz_m },
	{ &icon_info },
	{ &icon_info_m },
	{ &icon_internal_flash },
	{ &icon_ircut_auto },
	{ &icon_ircut_off },
	{ &icon_ircut_on_m },
	{ &icon_iso },
	{ &icon_iso_100 },
	{ &icon_iso_1600 },
	{ &icon_iso_200 },
	{ &icon_iso_400 },
	{ &icon_iso_800 },
	{ &icon_iso_auto },
	{ &icon_iso_m },
	{ &icon_landscape },
	{ &icon_language },
	{ &icon_language_m },
	{ &icon_ldws_left_alert },
	{ &icon_ldws_off },
	{ &icon_ldws_off_m },
	{ &icon_ldws_on },
	{ &icon_ldws_on_m },
	{ &icon_ldws_right_alert },
	{ &icon_lock },
	{ &icon_lock_all },
	{ &icon_lock_all_m },
	{ &icon_lock_current },
	{ &icon_lock_current_m },
	{ &icon_lock_m },
	{ &icon_lock_unlock },
	{ &icon_lock_unlock_m },
	{ &icon_lock_unlokc_all },
	{ &icon_lock_unlokc_all_m },
	{ &icon_lpr_off },
	{ &icon_lpr_off_m },
	{ &icon_lpr_on },
	{ &icon_lpr_on_m },
	{ &icon_menu_modechang },
	{ &icon_menu_modechang_m },
	{ &icon_menu_print_off },
	{ &icon_menu_return },
	{ &icon_menu_return_m },
	{ &icon_menu_setup_on_m },
	{ &icon_menu_wdr },
	{ &icon_menu_wdr_m },
	{ &icon_menu_wdr_off },
	{ &icon_metering },
	{ &icon_metering_m },
	{ &icon_mode_capture },
	{ &icon_mode_capture_m },
	{ &icon_mode_playback },
	{ &icon_mode_playback_m },
	{ &icon_mode_video },
	{ &icon_mode_video_m },
	{ &icon_motion_det_off },
	{ &icon_motion_det_on },
	{ &icon_motion_det_on_m },
	{ &icon_no },
	{ &icon_no_m },
	{ &icon_ok },
	{ &icon_ok2 },
	{ &icon_ok_m },
	{ &icon_ok_m_2 },
	{ &icon_pd_off },
	{ &icon_pd_off_m },
	{ &icon_pd_on },
	{ &icon_pd_on_m },
	{ &icon_pictbridge_s },
	{ &icon_pim_off },
	{ &icon_pim_off_m },
	{ &icon_pim_on },
	{ &icon_pim_on_m },
	{ &icon_play_backward },
	{ &icon_play_forward },
	{ &icon_play_pause },
	{ &icon_play_play },
	{ &icon_play_step },
	{ &icon_play_stop },
	{ &icon_power_off },
	{ &icon_power_off_m },
	{ &icon_print },
	{ &icon_protect },
	{ &icon_protect_m },
	{ &icon_quality },
	{ &icon_quality_basic },
	{ &icon_quality_fine },
	{ &icon_quality_m },
	{ &icon_quality_normal },
	{ &icon_quick_review },
	{ &icon_quick_review_m },
	{ &icon_rec_ellipse },
	{ &icon_rec_recovery },
	{ &icon_rec_transpaent },
	{ &icon_resolution },
	{ &icon_resolution_m },
	{ &icon_rotate },
	{ &icon_rsc_off },
	{ &icon_rsc_on },
	{ &icon_rsc_on_m },
	{ &icon_scene_mode },
	{ &icon_scene_mode_auto },
	{ &icon_scene_mode_landscpe },
	{ &icon_scene_mode_m },
	{ &icon_scene_mode_portrial },
	{ &icon_screensaver },
	{ &icon_screensaver_10min },
	{ &icon_screensaver_3min },
	{ &icon_screensaver_5min },
	{ &icon_screensaver_m },
	{ &icon_screensaver_off },
	{ &icon_sd_card },
	{ &icon_sd_lock },
	{ &icon_selftimer },
	{ &icon_selftimer_10sec },
	{ &icon_selftimer_2sec },
	{ &icon_selftimer_5sec },
	{ &icon_selftimer_m },
	{ &icon_selftimer_off },
	{ &icon_sequence_no },
	{ &icon_shake_off },
	{ &icon_shake_on },
	{ &icon_shake_on_m },
	{ &icon_sharpness },
	{ &icon_sharpness_high },
	{ &icon_sharpness_low },
	{ &icon_sharpness_m },
	{ &icon_sharpness_med },
	{ &icon_signal_0step },
	{ &icon_signal_0step_m },
	{ &icon_signal_1step },
	{ &icon_signal_1step_m },
	{ &icon_signal_2step },
	{ &icon_signal_2step_m },
	{ &icon_slideshow },
	{ &icon_slideshow_2sec },
	{ &icon_slideshow_5sec },
	{ &icon_slideshow_8sec },
	{ &icon_slideshow_m },
	{ &icon_slideshow_off },
	{ &icon_smile },
	{ &icon_smile_m },
	{ &icon_sng_alert },
	{ &icon_sng_off },
	{ &icon_sng_off_m },
	{ &icon_sng_on },
	{ &icon_sng_on_m },
	{ &icon_sound_rec_off },
	{ &icon_sound_rec_on },
	{ &icon_sound_rec_on_m },
	{ &icon_speaker },
	{ &icon_sys_cal },
	{ &icon_sys_cal_m },
	{ &icon_time_lapse_off },
	{ &icon_time_lapse_on },
	{ &icon_time_lapse_on_m },
	{ &icon_tsr_off },
	{ &icon_tsr_off_m },
	{ &icon_tsr_on },
	{ &icon_tsr_on_m },
	{ &icon_tv_mode },
	{ &icon_tv_mode_m },
	{ &icon_tv_ntsc },
	{ &icon_tv_pal },
	{ &icon_urgent_protect_auto },
	{ &icon_urgent_protect_auto_m },
	{ &icon_urgent_protect_manual },
	{ &icon_urgent_protect_manual_m },
	{ &icon_usb_msdc },
	{ &icon_usb_off },
	{ &icon_usb_on },
	{ &icon_usb_on_m },
	{ &icon_usb_uvc },
	{ &icon_version },
	{ &icon_version_m },
	{ &icon_video_flip_off },
	{ &icon_video_flip_on },
	{ &icon_video_flip_on_m },
	{ &icon_voice },
	{ &icon_voice_m },
	{ &icon_vol_0 },
	{ &icon_vol_1 },
	{ &icon_vol_2 },
	{ &icon_wb },
	{ &icon_wb_auto },
	{ &icon_wb_cloudy },
	{ &icon_wb_daylight },
	{ &icon_wb_fluorescent },
	{ &icon_wb_m },
	{ &icon_wb_tungsten },
	{ &icon_wifi_connected },
	{ &icon_wifi_connecting1 },
	{ &icon_wifi_connecting2 },
	{ &icon_wifi_connecting3 },
	{ &icon_wifi_dot_connecting_off },
	{ &icon_wifi_dot_connecting_on },
	{ &icon_wifi_fail },
	{ &icon_wifi_off },
	{ &icon_wifi_off_m },
	{ &icon_wifi_on },
	{ &icon_wifi_on_m },
	{ &icon_yes },
	{ &icon_yes_m },
};

lv_plugin_font_t lv_plugin_UIFlowLVGL_font_table[] =
{
	{ NULL },
	{ &lv_font_montserrat_16 },
	{ &notosanscjkjp_black_16_1bpp },
	{ &notosanscjksc_black_16_1bpp },
	{ &notosanscjktc_black_16_1bpp },
	{ &notosans_black_16_1bpp },
	{ &notosans_black_32_1bpp },
	{ &notosans_black_48_1bpp },
	{ &notosans_black_64_1bpp },
	{ &notosans_black_128_1bpp },
};

lv_plugin_string_t* lv_plugin_language_table[] = {
	NULL,
	lv_plugin_EN_string_table,
	lv_plugin_FR_string_table,
	lv_plugin_DE_string_table,
	lv_plugin_ES_string_table,
	lv_plugin_IT_string_table,
	lv_plugin_PO_string_table,
	lv_plugin_SC_string_table,
	lv_plugin_JP_string_table,
	lv_plugin_TC_string_table,
	lv_plugin_RU_string_table,
};

lv_plugin_res_id lv_plugin_language_font_table_EN[] = {
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
};

lv_plugin_res_id lv_plugin_language_font_table_FR[] = {
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
};

lv_plugin_res_id lv_plugin_language_font_table_DE[] = {
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
};

lv_plugin_res_id lv_plugin_language_font_table_ES[] = {
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
};

lv_plugin_res_id lv_plugin_language_font_table_IT[] = {
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
};

lv_plugin_res_id lv_plugin_language_font_table_PO[] = {
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
};

lv_plugin_res_id lv_plugin_language_font_table_SC[] = {
	LV_PLUGIN_FONT_ID_NOTOSANSCJKSC_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANSCJKSC_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANSCJKSC_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANSCJKSC_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANSCJKSC_BLACK_16_1BPP,
};

lv_plugin_res_id lv_plugin_language_font_table_JP[] = {
	LV_PLUGIN_FONT_ID_NOTOSANSCJKJP_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANSCJKJP_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANSCJKJP_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANSCJKJP_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANSCJKJP_BLACK_16_1BPP,
};

lv_plugin_res_id lv_plugin_language_font_table_TC[] = {
	LV_PLUGIN_FONT_ID_NOTOSANSCJKTC_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANSCJKTC_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANSCJKTC_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANSCJKTC_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANSCJKTC_BLACK_16_1BPP,
};

lv_plugin_res_id lv_plugin_language_font_table_RU[] = {
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
	LV_PLUGIN_FONT_ID_NOTOSANS_BLACK_16_1BPP,
};

lv_plugin_res_id* lv_plugin_language_fonts_table[] = {
	NULL,
	lv_plugin_language_font_table_EN,
	lv_plugin_language_font_table_FR,
	lv_plugin_language_font_table_DE,
	lv_plugin_language_font_table_ES,
	lv_plugin_language_font_table_IT,
	lv_plugin_language_font_table_PO,
	lv_plugin_language_font_table_SC,
	lv_plugin_language_font_table_JP,
	lv_plugin_language_font_table_TC,
	lv_plugin_language_font_table_RU,
};

lv_plugin_res_id lv_plugin_fixed_language_string_id_table[] = {
	LV_PLUGIN_RES_ID_NONE, /* 0*/
	LV_PLUGIN_RES_ID_NONE, /* 1*/
	LV_PLUGIN_RES_ID_NONE, /* 2*/
	LV_PLUGIN_RES_ID_NONE, /* 3*/
	LV_PLUGIN_RES_ID_NONE, /* 4*/
	LV_PLUGIN_RES_ID_NONE, /* 5*/
	LV_PLUGIN_RES_ID_NONE, /* 6*/
	LV_PLUGIN_RES_ID_NONE, /* 7*/
	LV_PLUGIN_RES_ID_NONE, /* 8*/
	LV_PLUGIN_RES_ID_NONE, /* 9*/
	LV_PLUGIN_RES_ID_NONE, /* 10*/
	LV_PLUGIN_RES_ID_NONE, /* 11*/
	LV_PLUGIN_RES_ID_NONE, /* 12*/
	LV_PLUGIN_RES_ID_NONE, /* 13*/
	LV_PLUGIN_RES_ID_NONE, /* 14*/
	LV_PLUGIN_RES_ID_NONE, /* 15*/
	LV_PLUGIN_RES_ID_NONE, /* 16*/
	LV_PLUGIN_RES_ID_NONE, /* 17*/
	LV_PLUGIN_RES_ID_NONE, /* 18*/
	LV_PLUGIN_RES_ID_NONE, /* 19*/
	LV_PLUGIN_RES_ID_NONE, /* 20*/
	LV_PLUGIN_RES_ID_NONE, /* 21*/
	LV_PLUGIN_RES_ID_NONE, /* 22*/
	LV_PLUGIN_RES_ID_NONE, /* 23*/
	LV_PLUGIN_RES_ID_NONE, /* 24*/
	LV_PLUGIN_RES_ID_NONE, /* 25*/
	LV_PLUGIN_RES_ID_NONE, /* 26*/
	LV_PLUGIN_RES_ID_NONE, /* 27*/
	LV_PLUGIN_RES_ID_NONE, /* 28*/
	LV_PLUGIN_RES_ID_NONE, /* 29*/
	LV_PLUGIN_RES_ID_NONE, /* 30*/
	LV_PLUGIN_RES_ID_NONE, /* 31*/
	LV_PLUGIN_RES_ID_NONE, /* 32*/
	LV_PLUGIN_RES_ID_NONE, /* 33*/
	LV_PLUGIN_RES_ID_NONE, /* 34*/
	LV_PLUGIN_RES_ID_NONE, /* 35*/
	LV_PLUGIN_RES_ID_NONE, /* 36*/
	LV_PLUGIN_RES_ID_NONE, /* 37*/
	LV_PLUGIN_RES_ID_NONE, /* 38*/
	LV_PLUGIN_RES_ID_NONE, /* 39*/
	LV_PLUGIN_RES_ID_NONE, /* 40*/
	LV_PLUGIN_RES_ID_NONE, /* 41*/
	LV_PLUGIN_RES_ID_NONE, /* 42*/
	LV_PLUGIN_RES_ID_NONE, /* 43*/
	LV_PLUGIN_RES_ID_NONE, /* 44*/
	LV_PLUGIN_RES_ID_NONE, /* 45*/
	LV_PLUGIN_RES_ID_NONE, /* 46*/
	LV_PLUGIN_RES_ID_NONE, /* 47*/
	LV_PLUGIN_RES_ID_NONE, /* 48*/
	LV_PLUGIN_RES_ID_NONE, /* 49*/
	LV_PLUGIN_RES_ID_NONE, /* 50*/
	LV_PLUGIN_RES_ID_NONE, /* 51*/
	LV_PLUGIN_RES_ID_NONE, /* 52*/
	LV_PLUGIN_RES_ID_NONE, /* 53*/
	LV_PLUGIN_RES_ID_NONE, /* 54*/
	LV_PLUGIN_RES_ID_NONE, /* 55*/
	LV_PLUGIN_RES_ID_NONE, /* 56*/
	LV_PLUGIN_RES_ID_NONE, /* 57*/
	LV_PLUGIN_RES_ID_NONE, /* 58*/
	LV_PLUGIN_RES_ID_NONE, /* 59*/
	LV_PLUGIN_RES_ID_NONE, /* 60*/
	LV_PLUGIN_RES_ID_NONE, /* 61*/
	LV_PLUGIN_RES_ID_NONE, /* 62*/
	LV_PLUGIN_RES_ID_NONE, /* 63*/
	LV_PLUGIN_RES_ID_NONE, /* 64*/
	LV_PLUGIN_RES_ID_NONE, /* 65*/
	LV_PLUGIN_RES_ID_NONE, /* 66*/
	LV_PLUGIN_RES_ID_NONE, /* 67*/
	LV_PLUGIN_RES_ID_NONE, /* 68*/
	LV_PLUGIN_RES_ID_NONE, /* 69*/
	LV_PLUGIN_RES_ID_NONE, /* 70*/
	LV_PLUGIN_RES_ID_NONE, /* 71*/
	LV_PLUGIN_RES_ID_NONE, /* 72*/
	LV_PLUGIN_RES_ID_NONE, /* 73*/
	LV_PLUGIN_RES_ID_NONE, /* 74*/
	LV_PLUGIN_RES_ID_NONE, /* 75*/
	LV_PLUGIN_RES_ID_NONE, /* 76*/
	LV_PLUGIN_RES_ID_NONE, /* 77*/
	LV_PLUGIN_RES_ID_NONE, /* 78*/
	LV_PLUGIN_RES_ID_NONE, /* 79*/
	LV_PLUGIN_RES_ID_NONE, /* 80*/
	LV_PLUGIN_RES_ID_NONE, /* 81*/
	LV_PLUGIN_RES_ID_NONE, /* 82*/
	LV_PLUGIN_RES_ID_NONE, /* 83*/
	LV_PLUGIN_RES_ID_NONE, /* 84*/
	LV_PLUGIN_RES_ID_NONE, /* 85*/
	LV_PLUGIN_RES_ID_NONE, /* 86*/
	LV_PLUGIN_RES_ID_NONE, /* 87*/
	LV_PLUGIN_RES_ID_NONE, /* 88*/
	LV_PLUGIN_RES_ID_NONE, /* 89*/
	LV_PLUGIN_RES_ID_NONE, /* 90*/
	LV_PLUGIN_RES_ID_NONE, /* 91*/
	LV_PLUGIN_RES_ID_NONE, /* 92*/
	LV_PLUGIN_RES_ID_NONE, /* 93*/
	LV_PLUGIN_RES_ID_NONE, /* 94*/
	LV_PLUGIN_RES_ID_NONE, /* 95*/
	LV_PLUGIN_RES_ID_NONE, /* 96*/
	LV_PLUGIN_RES_ID_NONE, /* 97*/
	LV_PLUGIN_RES_ID_NONE, /* 98*/
	LV_PLUGIN_RES_ID_NONE, /* 99*/
	LV_PLUGIN_RES_ID_NONE, /* 100*/
	LV_PLUGIN_RES_ID_NONE, /* 101*/
	LV_PLUGIN_RES_ID_NONE, /* 102*/
	LV_PLUGIN_RES_ID_NONE, /* 103*/
	LV_PLUGIN_RES_ID_NONE, /* 104*/
	LV_PLUGIN_RES_ID_NONE, /* 105*/
	LV_PLUGIN_RES_ID_NONE, /* 106*/
	LV_PLUGIN_RES_ID_NONE, /* 107*/
	LV_PLUGIN_RES_ID_NONE, /* 108*/
	LV_PLUGIN_RES_ID_NONE, /* 109*/
	LV_PLUGIN_RES_ID_NONE, /* 110*/
	LV_PLUGIN_RES_ID_NONE, /* 111*/
	LV_PLUGIN_RES_ID_NONE, /* 112*/
	LV_PLUGIN_RES_ID_NONE, /* 113*/
	LV_PLUGIN_RES_ID_NONE, /* 114*/
	LV_PLUGIN_RES_ID_NONE, /* 115*/
	LV_PLUGIN_RES_ID_NONE, /* 116*/
	LV_PLUGIN_RES_ID_NONE, /* 117*/
	LV_PLUGIN_RES_ID_NONE, /* 118*/
	LV_PLUGIN_RES_ID_NONE, /* 119*/
	LV_PLUGIN_RES_ID_NONE, /* 120*/
	LV_PLUGIN_RES_ID_NONE, /* 121*/
	LV_PLUGIN_RES_ID_NONE, /* 122*/
	LV_PLUGIN_RES_ID_NONE, /* 123*/
	LV_PLUGIN_RES_ID_NONE, /* 124*/
	LV_PLUGIN_RES_ID_NONE, /* 125*/
	LV_PLUGIN_RES_ID_NONE, /* 126*/
	LV_PLUGIN_RES_ID_NONE, /* 127*/
	LV_PLUGIN_RES_ID_NONE, /* 128*/
	LV_PLUGIN_RES_ID_NONE, /* 129*/
	LV_PLUGIN_RES_ID_NONE, /* 130*/
	LV_PLUGIN_RES_ID_NONE, /* 131*/
	LV_PLUGIN_RES_ID_NONE, /* 132*/
	LV_PLUGIN_RES_ID_NONE, /* 133*/
	LV_PLUGIN_RES_ID_NONE, /* 134*/
	LV_PLUGIN_RES_ID_NONE, /* 135*/
	LV_PLUGIN_RES_ID_NONE, /* 136*/
	LV_PLUGIN_RES_ID_NONE, /* 137*/
	LV_PLUGIN_RES_ID_NONE, /* 138*/
	LV_PLUGIN_RES_ID_NONE, /* 139*/
	LV_PLUGIN_RES_ID_NONE, /* 140*/
	LV_PLUGIN_RES_ID_NONE, /* 141*/
	LV_PLUGIN_RES_ID_NONE, /* 142*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 143*/
	LV_PLUGIN_LANGUAGE_ID_FR, /* 144*/
	LV_PLUGIN_LANGUAGE_ID_ES, /* 145*/
	LV_PLUGIN_LANGUAGE_ID_DE, /* 146*/
	LV_PLUGIN_LANGUAGE_ID_IT, /* 147*/
	LV_PLUGIN_LANGUAGE_ID_TC, /* 148*/
	LV_PLUGIN_LANGUAGE_ID_SC, /* 149*/
	LV_PLUGIN_LANGUAGE_ID_JP, /* 150*/
	LV_PLUGIN_LANGUAGE_ID_PO, /* 151*/
	LV_PLUGIN_LANGUAGE_ID_RU, /* 152*/
	LV_PLUGIN_RES_ID_NONE, /* 153*/
	LV_PLUGIN_RES_ID_NONE, /* 154*/
	LV_PLUGIN_RES_ID_NONE, /* 155*/
	LV_PLUGIN_RES_ID_NONE, /* 156*/
	LV_PLUGIN_RES_ID_NONE, /* 157*/
	LV_PLUGIN_RES_ID_NONE, /* 158*/
	LV_PLUGIN_RES_ID_NONE, /* 159*/
	LV_PLUGIN_RES_ID_NONE, /* 160*/
	LV_PLUGIN_RES_ID_NONE, /* 161*/
	LV_PLUGIN_RES_ID_NONE, /* 162*/
	LV_PLUGIN_RES_ID_NONE, /* 163*/
	LV_PLUGIN_RES_ID_NONE, /* 164*/
	LV_PLUGIN_RES_ID_NONE, /* 165*/
	LV_PLUGIN_RES_ID_NONE, /* 166*/
	LV_PLUGIN_RES_ID_NONE, /* 167*/
	LV_PLUGIN_RES_ID_NONE, /* 168*/
	LV_PLUGIN_RES_ID_NONE, /* 169*/
	LV_PLUGIN_RES_ID_NONE, /* 170*/
	LV_PLUGIN_RES_ID_NONE, /* 171*/
	LV_PLUGIN_RES_ID_NONE, /* 172*/
	LV_PLUGIN_RES_ID_NONE, /* 173*/
	LV_PLUGIN_RES_ID_NONE, /* 174*/
	LV_PLUGIN_RES_ID_NONE, /* 175*/
	LV_PLUGIN_RES_ID_NONE, /* 176*/
	LV_PLUGIN_RES_ID_NONE, /* 177*/
	LV_PLUGIN_RES_ID_NONE, /* 178*/
	LV_PLUGIN_RES_ID_NONE, /* 179*/
	LV_PLUGIN_RES_ID_NONE, /* 180*/
	LV_PLUGIN_RES_ID_NONE, /* 181*/
	LV_PLUGIN_RES_ID_NONE, /* 182*/
	LV_PLUGIN_RES_ID_NONE, /* 183*/
	LV_PLUGIN_RES_ID_NONE, /* 184*/
	LV_PLUGIN_RES_ID_NONE, /* 185*/
	LV_PLUGIN_RES_ID_NONE, /* 186*/
	LV_PLUGIN_RES_ID_NONE, /* 187*/
	LV_PLUGIN_RES_ID_NONE, /* 188*/
	LV_PLUGIN_RES_ID_NONE, /* 189*/
	LV_PLUGIN_RES_ID_NONE, /* 190*/
	LV_PLUGIN_RES_ID_NONE, /* 191*/
	LV_PLUGIN_RES_ID_NONE, /* 192*/
	LV_PLUGIN_RES_ID_NONE, /* 193*/
	LV_PLUGIN_RES_ID_NONE, /* 194*/
	LV_PLUGIN_RES_ID_NONE, /* 195*/
	LV_PLUGIN_RES_ID_NONE, /* 196*/
	LV_PLUGIN_RES_ID_NONE, /* 197*/
	LV_PLUGIN_RES_ID_NONE, /* 198*/
	LV_PLUGIN_RES_ID_NONE, /* 199*/
	LV_PLUGIN_RES_ID_NONE, /* 200*/
	LV_PLUGIN_RES_ID_NONE, /* 201*/
	LV_PLUGIN_RES_ID_NONE, /* 202*/
	LV_PLUGIN_RES_ID_NONE, /* 203*/
	LV_PLUGIN_RES_ID_NONE, /* 204*/
	LV_PLUGIN_RES_ID_NONE, /* 205*/
	LV_PLUGIN_RES_ID_NONE, /* 206*/
	LV_PLUGIN_RES_ID_NONE, /* 207*/
	LV_PLUGIN_RES_ID_NONE, /* 208*/
	LV_PLUGIN_RES_ID_NONE, /* 209*/
	LV_PLUGIN_RES_ID_NONE, /* 210*/
	LV_PLUGIN_RES_ID_NONE, /* 211*/
	LV_PLUGIN_RES_ID_NONE, /* 212*/
	LV_PLUGIN_RES_ID_NONE, /* 213*/
	LV_PLUGIN_RES_ID_NONE, /* 214*/
	LV_PLUGIN_RES_ID_NONE, /* 215*/
	LV_PLUGIN_RES_ID_NONE, /* 216*/
	LV_PLUGIN_RES_ID_NONE, /* 217*/
	LV_PLUGIN_RES_ID_NONE, /* 218*/
	LV_PLUGIN_RES_ID_NONE, /* 219*/
	LV_PLUGIN_RES_ID_NONE, /* 220*/
	LV_PLUGIN_RES_ID_NONE, /* 221*/
	LV_PLUGIN_RES_ID_NONE, /* 222*/
	LV_PLUGIN_RES_ID_NONE, /* 223*/
	LV_PLUGIN_RES_ID_NONE, /* 224*/
	LV_PLUGIN_RES_ID_NONE, /* 225*/
	LV_PLUGIN_RES_ID_NONE, /* 226*/
	LV_PLUGIN_RES_ID_NONE, /* 227*/
	LV_PLUGIN_RES_ID_NONE, /* 228*/
	LV_PLUGIN_RES_ID_NONE, /* 229*/
	LV_PLUGIN_RES_ID_NONE, /* 230*/
	LV_PLUGIN_RES_ID_NONE, /* 231*/
	LV_PLUGIN_RES_ID_NONE, /* 232*/
	LV_PLUGIN_RES_ID_NONE, /* 233*/
	LV_PLUGIN_RES_ID_NONE, /* 234*/
	LV_PLUGIN_RES_ID_NONE, /* 235*/
	LV_PLUGIN_RES_ID_NONE, /* 236*/
	LV_PLUGIN_RES_ID_NONE, /* 237*/
	LV_PLUGIN_RES_ID_NONE, /* 238*/
	LV_PLUGIN_RES_ID_NONE, /* 239*/
	LV_PLUGIN_RES_ID_NONE, /* 240*/
	LV_PLUGIN_RES_ID_NONE, /* 241*/
	LV_PLUGIN_RES_ID_NONE, /* 242*/
	LV_PLUGIN_RES_ID_NONE, /* 243*/
	LV_PLUGIN_RES_ID_NONE, /* 244*/
	LV_PLUGIN_RES_ID_NONE, /* 245*/
	LV_PLUGIN_RES_ID_NONE, /* 246*/
	LV_PLUGIN_RES_ID_NONE, /* 247*/
	LV_PLUGIN_RES_ID_NONE, /* 248*/
	LV_PLUGIN_RES_ID_NONE, /* 249*/
	LV_PLUGIN_RES_ID_NONE, /* 250*/
	LV_PLUGIN_RES_ID_NONE, /* 251*/
	LV_PLUGIN_RES_ID_NONE, /* 252*/
	LV_PLUGIN_RES_ID_NONE, /* 253*/
	LV_PLUGIN_RES_ID_NONE, /* 254*/
	LV_PLUGIN_RES_ID_NONE, /* 255*/
	LV_PLUGIN_RES_ID_NONE, /* 256*/
	LV_PLUGIN_RES_ID_NONE, /* 257*/
	LV_PLUGIN_RES_ID_NONE, /* 258*/
	LV_PLUGIN_RES_ID_NONE, /* 259*/
	LV_PLUGIN_RES_ID_NONE, /* 260*/
	LV_PLUGIN_RES_ID_NONE, /* 261*/
	LV_PLUGIN_RES_ID_NONE, /* 262*/
	LV_PLUGIN_RES_ID_NONE, /* 263*/
	LV_PLUGIN_RES_ID_NONE, /* 264*/
	LV_PLUGIN_RES_ID_NONE, /* 265*/
	LV_PLUGIN_RES_ID_NONE, /* 266*/
	LV_PLUGIN_RES_ID_NONE, /* 267*/
	LV_PLUGIN_RES_ID_NONE, /* 268*/
	LV_PLUGIN_RES_ID_NONE, /* 269*/
	LV_PLUGIN_RES_ID_NONE, /* 270*/
	LV_PLUGIN_RES_ID_NONE, /* 271*/
	LV_PLUGIN_RES_ID_NONE, /* 272*/
	LV_PLUGIN_RES_ID_NONE, /* 273*/
	LV_PLUGIN_RES_ID_NONE, /* 274*/
	LV_PLUGIN_RES_ID_NONE, /* 275*/
	LV_PLUGIN_RES_ID_NONE, /* 276*/
	LV_PLUGIN_RES_ID_NONE, /* 277*/
	LV_PLUGIN_RES_ID_NONE, /* 278*/
	LV_PLUGIN_RES_ID_NONE, /* 279*/
	LV_PLUGIN_RES_ID_NONE, /* 280*/
	LV_PLUGIN_RES_ID_NONE, /* 281*/
	LV_PLUGIN_RES_ID_NONE, /* 282*/
	LV_PLUGIN_RES_ID_NONE, /* 283*/
	LV_PLUGIN_RES_ID_NONE, /* 284*/
	LV_PLUGIN_RES_ID_NONE, /* 285*/
	LV_PLUGIN_RES_ID_NONE, /* 286*/
	LV_PLUGIN_RES_ID_NONE, /* 287*/
	LV_PLUGIN_RES_ID_NONE, /* 288*/
	LV_PLUGIN_RES_ID_NONE, /* 289*/
	LV_PLUGIN_RES_ID_NONE, /* 290*/
	LV_PLUGIN_RES_ID_NONE, /* 291*/
	LV_PLUGIN_RES_ID_NONE, /* 292*/
	LV_PLUGIN_RES_ID_NONE, /* 293*/
	LV_PLUGIN_RES_ID_NONE, /* 294*/
	LV_PLUGIN_RES_ID_NONE, /* 295*/
	LV_PLUGIN_RES_ID_NONE, /* 296*/
	LV_PLUGIN_RES_ID_NONE, /* 297*/
	LV_PLUGIN_RES_ID_NONE, /* 298*/
	LV_PLUGIN_RES_ID_NONE, /* 299*/
	LV_PLUGIN_RES_ID_NONE, /* 300*/
	LV_PLUGIN_RES_ID_NONE, /* 301*/
	LV_PLUGIN_RES_ID_NONE, /* 302*/
	LV_PLUGIN_RES_ID_NONE, /* 303*/
	LV_PLUGIN_RES_ID_NONE, /* 304*/
	LV_PLUGIN_RES_ID_NONE, /* 305*/
	LV_PLUGIN_RES_ID_NONE, /* 306*/
	LV_PLUGIN_RES_ID_NONE, /* 307*/
	LV_PLUGIN_RES_ID_NONE, /* 308*/
	LV_PLUGIN_RES_ID_NONE, /* 309*/
	LV_PLUGIN_RES_ID_NONE, /* 310*/
	LV_PLUGIN_RES_ID_NONE, /* 311*/
	LV_PLUGIN_RES_ID_NONE, /* 312*/
	LV_PLUGIN_RES_ID_NONE, /* 313*/
	LV_PLUGIN_RES_ID_NONE, /* 314*/
	LV_PLUGIN_RES_ID_NONE, /* 315*/
	LV_PLUGIN_RES_ID_NONE, /* 316*/
	LV_PLUGIN_RES_ID_NONE, /* 317*/
	LV_PLUGIN_RES_ID_NONE, /* 318*/
	LV_PLUGIN_RES_ID_NONE, /* 319*/
	LV_PLUGIN_RES_ID_NONE, /* 320*/
	LV_PLUGIN_RES_ID_NONE, /* 321*/
	LV_PLUGIN_RES_ID_NONE, /* 322*/
	LV_PLUGIN_RES_ID_NONE, /* 323*/
	LV_PLUGIN_RES_ID_NONE, /* 324*/
	LV_PLUGIN_RES_ID_NONE, /* 325*/
	LV_PLUGIN_RES_ID_NONE, /* 326*/
	LV_PLUGIN_RES_ID_NONE, /* 327*/
	LV_PLUGIN_RES_ID_NONE, /* 328*/
	LV_PLUGIN_RES_ID_NONE, /* 329*/
	LV_PLUGIN_RES_ID_NONE, /* 330*/
	LV_PLUGIN_RES_ID_NONE, /* 331*/
	LV_PLUGIN_RES_ID_NONE, /* 332*/
	LV_PLUGIN_RES_ID_NONE, /* 333*/
	LV_PLUGIN_RES_ID_NONE, /* 334*/
	LV_PLUGIN_RES_ID_NONE, /* 335*/
	LV_PLUGIN_RES_ID_NONE, /* 336*/
	LV_PLUGIN_RES_ID_NONE, /* 337*/
	LV_PLUGIN_RES_ID_NONE, /* 338*/
	LV_PLUGIN_RES_ID_NONE, /* 339*/
	LV_PLUGIN_RES_ID_NONE, /* 340*/
	LV_PLUGIN_RES_ID_NONE, /* 341*/
	LV_PLUGIN_RES_ID_NONE, /* 342*/
	LV_PLUGIN_RES_ID_NONE, /* 343*/
	LV_PLUGIN_RES_ID_NONE, /* 344*/
	LV_PLUGIN_RES_ID_NONE, /* 345*/
	LV_PLUGIN_RES_ID_NONE, /* 346*/
	LV_PLUGIN_RES_ID_NONE, /* 347*/
	LV_PLUGIN_RES_ID_NONE, /* 348*/
	LV_PLUGIN_RES_ID_NONE, /* 349*/
	LV_PLUGIN_RES_ID_NONE, /* 350*/
	LV_PLUGIN_RES_ID_NONE, /* 351*/
	LV_PLUGIN_RES_ID_NONE, /* 352*/
	LV_PLUGIN_RES_ID_NONE, /* 353*/
	LV_PLUGIN_RES_ID_NONE, /* 354*/
	LV_PLUGIN_RES_ID_NONE, /* 355*/
	LV_PLUGIN_RES_ID_NONE, /* 356*/
	LV_PLUGIN_RES_ID_NONE, /* 357*/
	LV_PLUGIN_RES_ID_NONE, /* 358*/
	LV_PLUGIN_RES_ID_NONE, /* 359*/
	LV_PLUGIN_RES_ID_NONE, /* 360*/
	LV_PLUGIN_RES_ID_NONE, /* 361*/
	LV_PLUGIN_RES_ID_NONE, /* 362*/
	LV_PLUGIN_RES_ID_NONE, /* 363*/
	LV_PLUGIN_RES_ID_NONE, /* 364*/
	LV_PLUGIN_RES_ID_NONE, /* 365*/
	LV_PLUGIN_RES_ID_NONE, /* 366*/
	LV_PLUGIN_RES_ID_NONE, /* 367*/
	LV_PLUGIN_RES_ID_NONE, /* 368*/
	LV_PLUGIN_RES_ID_NONE, /* 369*/
	LV_PLUGIN_RES_ID_NONE, /* 370*/
	LV_PLUGIN_RES_ID_NONE, /* 371*/
	LV_PLUGIN_RES_ID_NONE, /* 372*/
	LV_PLUGIN_RES_ID_NONE, /* 373*/
	LV_PLUGIN_RES_ID_NONE, /* 374*/
	LV_PLUGIN_RES_ID_NONE, /* 375*/
	LV_PLUGIN_RES_ID_NONE, /* 376*/
	LV_PLUGIN_RES_ID_NONE, /* 377*/
	LV_PLUGIN_RES_ID_NONE, /* 378*/
	LV_PLUGIN_RES_ID_NONE, /* 379*/
	LV_PLUGIN_RES_ID_NONE, /* 380*/
	LV_PLUGIN_RES_ID_NONE, /* 381*/
	LV_PLUGIN_RES_ID_NONE, /* 382*/
	LV_PLUGIN_RES_ID_NONE, /* 383*/
	LV_PLUGIN_RES_ID_NONE, /* 384*/
	LV_PLUGIN_RES_ID_NONE, /* 385*/
	LV_PLUGIN_RES_ID_NONE, /* 386*/
	LV_PLUGIN_RES_ID_NONE, /* 387*/
	LV_PLUGIN_RES_ID_NONE, /* 388*/
	LV_PLUGIN_RES_ID_NONE, /* 389*/
	LV_PLUGIN_RES_ID_NONE, /* 390*/
	LV_PLUGIN_RES_ID_NONE, /* 391*/
	LV_PLUGIN_RES_ID_NONE, /* 392*/
	LV_PLUGIN_RES_ID_NONE, /* 393*/
	LV_PLUGIN_RES_ID_NONE, /* 394*/
	LV_PLUGIN_RES_ID_NONE, /* 395*/
	LV_PLUGIN_RES_ID_NONE, /* 396*/
	LV_PLUGIN_RES_ID_NONE, /* 397*/
	LV_PLUGIN_RES_ID_NONE, /* 398*/
	LV_PLUGIN_RES_ID_NONE, /* 399*/
	LV_PLUGIN_RES_ID_NONE, /* 400*/
	LV_PLUGIN_RES_ID_NONE, /* 401*/
	LV_PLUGIN_RES_ID_NONE, /* 402*/
	LV_PLUGIN_RES_ID_NONE, /* 403*/
	LV_PLUGIN_RES_ID_NONE, /* 404*/
	LV_PLUGIN_RES_ID_NONE, /* 405*/
	LV_PLUGIN_RES_ID_NONE, /* 406*/
	LV_PLUGIN_RES_ID_NONE, /* 407*/
	LV_PLUGIN_RES_ID_NONE, /* 408*/
	LV_PLUGIN_RES_ID_NONE, /* 409*/
	LV_PLUGIN_RES_ID_NONE, /* 410*/
	LV_PLUGIN_RES_ID_NONE, /* 411*/
	LV_PLUGIN_RES_ID_NONE, /* 412*/
	LV_PLUGIN_RES_ID_NONE, /* 413*/
	LV_PLUGIN_RES_ID_NONE, /* 414*/
	LV_PLUGIN_RES_ID_NONE, /* 415*/
	LV_PLUGIN_RES_ID_NONE, /* 416*/
	LV_PLUGIN_RES_ID_NONE, /* 417*/
	LV_PLUGIN_RES_ID_NONE, /* 418*/
	LV_PLUGIN_RES_ID_NONE, /* 419*/
	LV_PLUGIN_RES_ID_NONE, /* 420*/
	LV_PLUGIN_RES_ID_NONE, /* 421*/
	LV_PLUGIN_RES_ID_NONE, /* 422*/
	LV_PLUGIN_RES_ID_NONE, /* 423*/
	LV_PLUGIN_RES_ID_NONE, /* 424*/
	LV_PLUGIN_RES_ID_NONE, /* 425*/
	LV_PLUGIN_RES_ID_NONE, /* 426*/
	LV_PLUGIN_RES_ID_NONE, /* 427*/
	LV_PLUGIN_RES_ID_NONE, /* 428*/
	LV_PLUGIN_RES_ID_NONE, /* 429*/
	LV_PLUGIN_RES_ID_NONE, /* 430*/
	LV_PLUGIN_RES_ID_NONE, /* 431*/
	LV_PLUGIN_RES_ID_NONE, /* 432*/
	LV_PLUGIN_RES_ID_NONE, /* 433*/
	LV_PLUGIN_RES_ID_NONE, /* 434*/
	LV_PLUGIN_RES_ID_NONE, /* 435*/
	LV_PLUGIN_RES_ID_NONE, /* 436*/
	LV_PLUGIN_RES_ID_NONE, /* 437*/
	LV_PLUGIN_RES_ID_NONE, /* 438*/
	LV_PLUGIN_RES_ID_NONE, /* 439*/
	LV_PLUGIN_RES_ID_NONE, /* 440*/
	LV_PLUGIN_RES_ID_NONE, /* 441*/
	LV_PLUGIN_RES_ID_NONE, /* 442*/
	LV_PLUGIN_RES_ID_NONE, /* 443*/
	LV_PLUGIN_RES_ID_NONE, /* 444*/
	LV_PLUGIN_RES_ID_NONE, /* 445*/
	LV_PLUGIN_RES_ID_NONE, /* 446*/
	LV_PLUGIN_RES_ID_NONE, /* 447*/
	LV_PLUGIN_RES_ID_NONE, /* 448*/
	LV_PLUGIN_RES_ID_NONE, /* 449*/
	LV_PLUGIN_RES_ID_NONE, /* 450*/
	LV_PLUGIN_RES_ID_NONE, /* 451*/
	LV_PLUGIN_RES_ID_NONE, /* 452*/
	LV_PLUGIN_RES_ID_NONE, /* 453*/
	LV_PLUGIN_RES_ID_NONE, /* 454*/
	LV_PLUGIN_RES_ID_NONE, /* 455*/
	LV_PLUGIN_RES_ID_NONE, /* 456*/
	LV_PLUGIN_RES_ID_NONE, /* 457*/
	LV_PLUGIN_RES_ID_NONE, /* 458*/
	LV_PLUGIN_RES_ID_NONE, /* 459*/
	LV_PLUGIN_RES_ID_NONE, /* 460*/
	LV_PLUGIN_RES_ID_NONE, /* 461*/
	LV_PLUGIN_RES_ID_NONE, /* 462*/
	LV_PLUGIN_RES_ID_NONE, /* 463*/
	LV_PLUGIN_RES_ID_NONE, /* 464*/
	LV_PLUGIN_RES_ID_NONE, /* 465*/
	LV_PLUGIN_RES_ID_NONE, /* 466*/
	LV_PLUGIN_RES_ID_NONE, /* 467*/
	LV_PLUGIN_RES_ID_NONE, /* 468*/
	LV_PLUGIN_RES_ID_NONE, /* 469*/
	LV_PLUGIN_RES_ID_NONE, /* 470*/
	LV_PLUGIN_RES_ID_NONE, /* 471*/
	LV_PLUGIN_RES_ID_NONE, /* 472*/
	LV_PLUGIN_RES_ID_NONE, /* 473*/
	LV_PLUGIN_RES_ID_NONE, /* 474*/
	LV_PLUGIN_RES_ID_NONE, /* 475*/
	LV_PLUGIN_RES_ID_NONE, /* 476*/
	LV_PLUGIN_RES_ID_NONE, /* 477*/
	LV_PLUGIN_RES_ID_NONE, /* 478*/
	LV_PLUGIN_RES_ID_NONE, /* 479*/
	LV_PLUGIN_RES_ID_NONE, /* 480*/
	LV_PLUGIN_RES_ID_NONE, /* 481*/
	LV_PLUGIN_RES_ID_NONE, /* 482*/
	LV_PLUGIN_RES_ID_NONE, /* 483*/
	LV_PLUGIN_RES_ID_NONE, /* 484*/
	LV_PLUGIN_RES_ID_NONE, /* 485*/
	LV_PLUGIN_RES_ID_NONE, /* 486*/
	LV_PLUGIN_RES_ID_NONE, /* 487*/
	LV_PLUGIN_RES_ID_NONE, /* 488*/
	LV_PLUGIN_RES_ID_NONE, /* 489*/
	LV_PLUGIN_RES_ID_NONE, /* 490*/
	LV_PLUGIN_RES_ID_NONE, /* 491*/
	LV_PLUGIN_RES_ID_NONE, /* 492*/
	LV_PLUGIN_RES_ID_NONE, /* 493*/
	LV_PLUGIN_RES_ID_NONE, /* 494*/
	LV_PLUGIN_RES_ID_NONE, /* 495*/
	LV_PLUGIN_RES_ID_NONE, /* 496*/
	LV_PLUGIN_RES_ID_NONE, /* 497*/
	LV_PLUGIN_RES_ID_NONE, /* 498*/
	LV_PLUGIN_RES_ID_NONE, /* 499*/
	LV_PLUGIN_RES_ID_NONE, /* 500*/
	LV_PLUGIN_RES_ID_NONE, /* 501*/
	LV_PLUGIN_RES_ID_NONE, /* 502*/
	LV_PLUGIN_RES_ID_NONE, /* 503*/
	LV_PLUGIN_RES_ID_NONE, /* 504*/
	LV_PLUGIN_RES_ID_NONE, /* 505*/
	LV_PLUGIN_RES_ID_NONE, /* 506*/
	LV_PLUGIN_RES_ID_NONE, /* 507*/
	LV_PLUGIN_RES_ID_NONE, /* 508*/
	LV_PLUGIN_RES_ID_NONE, /* 509*/
	LV_PLUGIN_RES_ID_NONE, /* 510*/
	LV_PLUGIN_RES_ID_NONE, /* 511*/
	LV_PLUGIN_RES_ID_NONE, /* 512*/
	LV_PLUGIN_RES_ID_NONE, /* 513*/
	LV_PLUGIN_RES_ID_NONE, /* 514*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 515*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 516*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 517*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 518*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 519*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 520*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 521*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 522*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 523*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 524*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 525*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 526*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 527*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 528*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 529*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 530*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 531*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 532*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 533*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 534*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 535*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 536*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 537*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 538*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 539*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 540*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 541*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 542*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 543*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 544*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 545*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 546*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 547*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 548*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 549*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 550*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 551*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 552*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 553*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 554*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 555*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 556*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 557*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 558*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 559*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 560*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 561*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 562*/
	LV_PLUGIN_RES_ID_NONE, /* 563*/
	LV_PLUGIN_RES_ID_NONE, /* 564*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 565*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 566*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 567*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 568*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 569*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 570*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 571*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 572*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 573*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 574*/
	LV_PLUGIN_LANGUAGE_ID_EN, /* 575*/	
};

lv_plugin_res_t lv_plugin_UIFlowLVGL_res =
{
	lv_plugin_UIFlowLVGL_img_table,
	lv_plugin_UIFlowLVGL_font_table,
	lv_plugin_language_table,
	lv_plugin_language_fonts_table,
	lv_plugin_fixed_language_string_id_table,
	LV_PLUGIN_IMG_TABLE_SIZE,
	LV_PLUGIN_FONT_TABLE_SIZE,
	LV_PLUGIN_LANGUAGE_TABLE_SIZE,
	LV_PLUGIN_STRING_TABLE_SIZE,
	LV_PLUGIN_LANGUAGE_FONT_TYPE_SIZE,
};

void UIFlowLVGL(){

	lv_plugin_init();
	lv_plugin_install_resource(&lv_plugin_UIFlowLVGL_res);
	UIFlowMovie = UIFlowMovie_create();
	UIFlowPhoto = UIFlowPhoto_create();
	UIFlowWrnMsg = UIFlowWrnMsg_create();
	UIFlowPlay = UIFlowPlay_create();
	UIFlowMenuCommonItem = UIFlowMenuCommonItem_create();
	UIFlowMenuCommonOption = UIFlowMenuCommonOption_create();
	UIFlowMenuCommonConfirm = UIFlowMenuCommonConfirm_create();
	UIFlowWaitMoment = UIFlowWaitMoment_create();
	UIFlowSetupDateTime = UIFlowSetupDateTime_create();
	UIFlowPlayThumb = UIFlowPlayThumb_create();
	UIFlowUSB = UIFlowUSB_create();
	UIFlowUSBMenu = UIFlowUSBMenu_create();
	UIFlowWifiLink = UIFlowWifiLink_create();
	UIFlowWifiLinkOK = UIFlowWifiLinkOK_create();
	UIFlowWifiWait = UIFlowWifiWait_create();

}