/*
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       UIFlow.h
    @ingroup    mIPRJAPUIFlow

    @brief      UI Flow Functions
                This file is the user interface ( for interchange flow control).

    @note       Nothing.

    @date       2005/04/01
*/

#ifndef _UIFLOW_ALEXA_H
#define _UIFLOW_ALEXA_H

#if (PHOTO_MODE == ENABLE)
#include "UIWnd/ALEXA/UIInfo/UIPhotoInfo.h"
#include "UIWnd/ALEXA/UIInfo/UIPhotoMapping.h"
#endif
#if (MOVIE_MODE == ENABLE)
#include "UIWnd/ALEXA/UIInfo/UIMovieInfo.h"
#include "UIWnd/ALEXA/UIInfo/UIMovieMapping.h"
#endif

#if(IPCAM_FUNC!= ENABLE)
#include "UIWnd/ALEXA/UIInfo/UIInfo.h"
#include "UIWnd/ALEXA/UIInfo/DateTimeInfo.h"
#endif

/* Resource */
#include "UIWnd/ALEXA/Resource/UIResource.h"
#include "UIWnd/ALEXA/Resource/SoundData.h"


/****************************************************************************
 * MenuCommon headers
 ***************************************************************************/

/* Common UIMenu */
#include "UIWnd/ALEXA/UIMenu/UIMenuCommon/MenuCommonItem.h"
#include "UIWnd/ALEXA/UIMenu/UIMenuCommon/MenuCommonOption.h"
#include "UIWnd/ALEXA/UIMenu/UIMenuCommon/MenuCommonConfirm.h"
#include "UIWnd/ALEXA/UIMenu/UIMenuCommon/TabMenu.h"
#include "UIWnd/ALEXA/UIMenu/UIMenuCommon/MenuId.h"
#include "UIWnd/ALEXA/UIMenu/UIMenuCommon/MenuCommon.h"
#include "UIWnd/ALEXA/UIMenu/UIMenuCommon/MenuMode.h"

/* Common UIFlow */
#include "UIWnd/ALEXA/UIFlow/UIFlowCommon/UIFlowWndWaitMoment.h"

/****************************************************************************
 * Setup headers
 ***************************************************************************/

/* Setup UIMenu */
#include "UIWnd/ALEXA/UIMenu/UIMenuSetup/MenuSetup.h"
#include "UIWnd/ALEXA/UIMenu/UIMenuSetup/UIMenuWndSetupDateTime.h"

/****************************************************************************
 * Movie mode headers
 ***************************************************************************/

/* Movie UIMenu */
#include "UIWnd/ALEXA/UIMenu/UIMenuMovie/MenuMovie.h"

/* Movie UIFlow */
#include "UIWnd/ALEXA/UIFlow/UIFlowMovie/UIFlowWndMovie.h"
#include "UIWnd/ALEXA/UIFlow/UIFlowMovie/UIFlowMovieFuncs.h"
#include "UIWnd/ALEXA/UIFlow/UIFlowMovie/UIFlowMovieIcons.h"
#include "UIWnd/ALEXA/UIFlow/UIFlowCommon/UIFlowWndWrnMsgAPI.h"

/* Movie UIInfo */
#include "UIWnd/ALEXA/UIInfo/UIMovieMapping.h"
#include "UIWnd/ALEXA/UIInfo/UIMovieInfo.h"

/****************************************************************************
 * Photo mode headers
 ***************************************************************************/

/* Photo UIFlow */
#include "UIWnd/ALEXA/UIFlow/UIFlowPhoto/UIFlowWndPhoto.h"
#include "UIWnd/ALEXA/UIFlow/UIFlowPhoto/UIFlowPhotoFuncs.h"
#include "UIWnd/ALEXA/UIFlow/UIFlowPhoto/UIFlowPhotoIcons.h"
#include "UIWnd/ALEXA/UIFlow/UIFlowPhoto/UIFlowPhotoParams.h"


/* Photo UIMenu */
#include "UIWnd/ALEXA/UIMenu/UIMenuPhoto/MenuPhoto.h"
#include "UIWnd/ALEXA/UIMenu/UIMenuPhoto/UIMenuWndPhotoColor.h"
#include "UIWnd/ALEXA/UIMenu/UIMenuPhoto/UIMenuWndPhotoExposure.h"
#include "UIWnd/ALEXA/UIMenu/UIMenuPhoto/UIMenuWndPhotoQuickSetting.h"
#include "UIWnd/ALEXA/UIMenu/UIMenuPhoto/UIMenuWndPhotoWB.h"

/* Photo UIInfo */
#include "UIWnd/ALEXA/UIInfo/UIPhotoMapping.h"
#include "UIWnd/ALEXA/UIInfo/UIPhotoInfo.h"

/****************************************************************************
 * Playback mode headers
 ***************************************************************************/

/* Playback UIFlow */
#include "UIWnd/ALEXA/UIFlow/UIFlowPlay/UIFlowPlayFuncs.h"
#include "UIWnd/ALEXA/UIFlow/UIFlowPlay/UIFlowPlayIcons.h"
#include "UIWnd/ALEXA/UIFlow/UIFlowPlay/UIFlowWndPlay.h"
#include "UIWnd/ALEXA/UIFlow/UIFlowPlay/UIFlowWndPlayMagnify.h"
#include "UIWnd/ALEXA/UIFlow/UIFlowPlay/UIFlowWndPlayThumb.h"

/* Playback UIMenu */
#include "UIWnd/ALEXA/UIMenu/UIMenuPlay/MenuPlayback.h"


/****************************************************************************
 * USB headers
 ***************************************************************************/

/* USB UIMenu */
#include "UIWnd/ALEXA/UIMenu/UIMenuUSB/UIMenuWndUSB.h"

/* USB UIFlow */
#include "UIWnd/ALEXA/UIFlow/UIFlowCommon/UIFlowWndUSB.h"
#include "UIWnd/ALEXA/UIFlow/UIFlowCommon/UIFlowWndUSBAPI.h"


/****************************************************************************
 * WiFi headers
 ***************************************************************************/

/* WiFi UIMenu */
#include "UIWnd/ALEXA/UIMenu/UIMenuWiFi/UIMenuWndWiFiWait.h"
#include "UIWnd/ALEXA/UIMenu/UIMenuWiFi/UIMenuWndWiFiModuleLink.h"
#include "UIWnd/ALEXA/UIMenu/UIMenuWiFi/UIMenuWndWiFiMobileLinkOK.h"

#endif
