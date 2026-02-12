/*
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       UIFlow.h
    @ingroup    mIPRJAPUIFlow

    @brief      UI Flow Functions
                This file is the user interface ( for interchange flow control).

    @note       Nothing.

    @date       2005/04/01
*/

#ifndef _UIFLOW_SPORTSCAM_TOUCH_H
#define _UIFLOW_SPORTSCAM_TOUCH_H

#if (PHOTO_MODE == ENABLE)
#include "UIWnd/SPORTCAM_TOUCH/UIInfo/UIPhotoInfo.h"
#include "UIWnd/SPORTCAM_TOUCH/UIInfo/UIPhotoMapping.h"
#endif
#if (MOVIE_MODE == ENABLE)
#include "UIWnd/SPORTCAM_TOUCH/UIInfo/UIMovieInfo.h"
#include "UIWnd/SPORTCAM_TOUCH/UIInfo/UIMovieMapping.h"
#endif

#if(IPCAM_FUNC!= ENABLE)
#include "UIWnd/SPORTCAM_TOUCH/UIInfo/UIInfo.h"
#include "UIWnd/SPORTCAM_TOUCH/UIInfo/DateTimeInfo.h"
#endif

/* Resource */
#include "UIWnd/SPORTCAM_TOUCH/Resource/UIResource.h"
#include "UIWnd/SPORTCAM_TOUCH/Resource/SoundData.h"


/****************************************************************************
 * MenuCommon headers
 ***************************************************************************/

/* Common UIMenu */
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuCommon/MenuCommonItem.h"
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuCommon/MenuCommonOption.h"
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuCommon/MenuCommonConfirm.h"
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuCommon/TabMenu.h"
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuCommon/MenuId.h"
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuCommon/MenuCommon.h"
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuCommon/MenuMode.h"

/* Common UIFlow */
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowCommon/UIFlowWndWaitMoment.h"

/****************************************************************************
 * Setup headers
 ***************************************************************************/

/* Setup UIMenu */
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuSetup/MenuSetup.h"
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuSetup/UIMenuWndSetupDateTime.h"

/****************************************************************************
 * Movie mode headers
 ***************************************************************************/

/* Movie UIMenu */
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuMovie/MenuMovie.h"

/* Movie UIFlow */
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowMovie/UIFlowWndMovie.h"
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowMovie/UIFlowMovieFuncs.h"
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowMovie/UIFlowMovieIcons.h"
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowCommon/UIFlowWndWrnMsgAPI.h"

/* Movie UIInfo */
#include "UIWnd/SPORTCAM_TOUCH/UIInfo/UIMovieMapping.h"
#include "UIWnd/SPORTCAM_TOUCH/UIInfo/UIMovieInfo.h"

/****************************************************************************
 * Photo mode headers
 ***************************************************************************/

/* Photo UIFlow */
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPhoto/UIFlowWndPhoto.h"
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPhoto/UIFlowPhotoFuncs.h"
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPhoto/UIFlowPhotoIcons.h"
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPhoto/UIFlowPhotoParams.h"


/* Photo UIMenu */
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuPhoto/MenuPhoto.h"
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuPhoto/UIMenuWndPhotoColor.h"
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuPhoto/UIMenuWndPhotoExposure.h"
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuPhoto/UIMenuWndPhotoQuickSetting.h"
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuPhoto/UIMenuWndPhotoWB.h"

/* Photo UIInfo */
#include "UIWnd/SPORTCAM_TOUCH/UIInfo/UIPhotoMapping.h"
#include "UIWnd/SPORTCAM_TOUCH/UIInfo/UIPhotoInfo.h"

/****************************************************************************
 * Playback mode headers
 ***************************************************************************/

/* Playback UIFlow */
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPlay/UIFlowPlayFuncs.h"
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPlay/UIFlowPlayIcons.h"
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPlay/UIFlowWndPlay.h"
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPlay/UIFlowWndPlayMagnify.h"
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowPlay/UIFlowWndPlayThumb.h"

/* Playback UIMenu */
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuPlay/MenuPlayback.h"


/****************************************************************************
 * USB headers
 ***************************************************************************/

/* USB UIMenu */
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuUSB/UIMenuWndUSB.h"

/* USB UIFlow */
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowCommon/UIFlowWndUSB.h"
#include "UIWnd/SPORTCAM_TOUCH/UIFlow/UIFlowCommon/UIFlowWndUSBAPI.h"


/****************************************************************************
 * WiFi headers
 ***************************************************************************/

/* WiFi UIMenu */
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuWiFi/UIMenuWndWiFiWait.h"
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuWiFi/UIMenuWndWiFiModuleLink.h"
#include "UIWnd/SPORTCAM_TOUCH/UIMenu/UIMenuWiFi/UIMenuWndWiFiMobileLinkOK.h"

#endif
