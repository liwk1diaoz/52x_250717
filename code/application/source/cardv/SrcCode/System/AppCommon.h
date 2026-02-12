/*
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       AppCommon.h
    @ingroup    mIPRJAPUIFlow

    @brief      App Common Interfaces
                This file is the user interface ( for LIB callback function).

    @note       Nothing.

    @date       2005/04/01
*/

#ifndef _APPCOMMON_H
#define _APPCOMMON_H

//Built-in App Object
#include "UIApp/AppLib.h"         //include EVENT of Common Obj

//Mode
#include "Mode/UIMode.h"

//M (Info)

#include "ProjectInfo.h"
//#if(PHOTO_MODE==ENABLE)
//#include "UIPhotoInfo.h"
//#include "UIPhotoMapping.h"
//#endif
#if(PLAY_MODE==ENABLE)
//#include "UIPlayInfo.h"
//#include "UIPlayMapping.h"
#endif

//V (App)
#if(MOVIE_MODE==ENABLE)
#include "UIApp/Movie/UIAppMovie.h"      //include EVENT of Movie App
#endif
#if 1//(PHOTO_MODE==ENABLE)
#include "UIApp/Photo/UIAppPhoto.h"      //include EVENT of Photo App
#endif
#if(PLAY_MODE==ENABLE)
#include "UIApp/Play/UIAppPlay.h"       //include EVENT of Play App
#include "UIApp/Play/UIPlayComm.h"       //include EVENT of Play App
#endif
#if 1//(USB_MODE==ENABLE)
#include "UIApp/UsbCam/UIAppUsbCam.h"
//#include "UIAppUsbCharge.h"
#include "UIApp/UsbDisk/UIAppUsbDisk.h"
//#include "UIAppUsbPrint.h"   //include EVENT of Print App
#include "UIApp/UsbMsdcCb/MsdcNvtCb.h"
#endif

#include "UIApp/Background/UIBackgroundObj.h" //include EVENT of Worker Task
#include "UIApp/Setup/UISetup.h"         //include EVENT of System Setup

#endif //_APPCOMMON_H_

