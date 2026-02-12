#include "UIWnd/UIFlow.h"

// --------------------------------------------------------------------------
// OPTIONS
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// ITEMS
// --------------------------------------------------------------------------
#if(MOVIE_MODE==ENABLE)
int MenuCustom_Movie(UINT32 uiMessage, UINT32 uiParam)
{
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_MOVIE) {
		Ux_SendEvent(0, NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_MOVIE);
	} else {
		lv_plugin_scr_close(UIFlowMenuCommonItem, NULL);
	}
	return TMF_PROCESSED;
}
#endif
#if(PHOTO_MODE==ENABLE)
int MenuCustom_Photo(UINT32 uiMessage, UINT32 uiParam)
{
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_PHOTO) {
		Ux_SendEvent(0, NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_PHOTO);
	} else {
		lv_plugin_scr_close(UIFlowMenuCommonItem, NULL);
	}

	return TMF_PROCESSED;
}
#endif

#if(PLAY_MODE==ENABLE)
int MenuCustom_Playback(UINT32 uiMessage, UINT32 uiParam)
{
	if (System_GetState(SYS_STATE_CURRMODE) != PRIMARY_MODE_PLAYBACK) {
		Ux_SendEvent(0, NVTEVT_SYSTEM_MODE, 1, PRIMARY_MODE_PLAYBACK);
	} else {
		lv_plugin_scr_close(UIFlowMenuCommonItem, NULL);
	}

	return TMF_PROCESSED;
}
#endif

// Movie Menu Items
TMDEF_BEGIN_ITEMS(MODE)
#if(MOVIE_MODE==ENABLE)
TMDEF_ITEM_CUSTOM(MODE_MOVIE, MenuCustom_Movie)
#endif
#if(PHOTO_MODE==ENABLE)
TMDEF_ITEM_CUSTOM(MODE_PHOTO, MenuCustom_Photo)
#endif
#if(PLAY_MODE==ENABLE)
TMDEF_ITEM_CUSTOM(MODE_PLAYBACK, MenuCustom_Playback)
#endif
TMDEF_END_ITEMS()

// --------------------------------------------------------------------------
// PAGES
// --------------------------------------------------------------------------
// Common Menu Pages
TMDEF_BEGIN_PAGES(MODE)
TMDEF_PAGE_TEXT_ICON(MODE)
TMDEF_END_PAGES()

TMDEF_EMNU(gModeMenu, MODE, Mode_MenuCallback)

// --------------------------------------------------------------------------
// Menu Callback
// --------------------------------------------------------------------------
int Mode_MenuCallback(UINT32 uiMessage, UINT32 uiParam)
{
#if 0
	UINT16  uwItemId;
	UINT16  uwOption;

	if (uiMessage == TMM_CONFIRM_OPTION) {
		uwItemId = LO_WORD(uiParam);
		uwOption = HI_WORD(uiParam);
	}
#endif

	return TMF_PROCESSED;
}
