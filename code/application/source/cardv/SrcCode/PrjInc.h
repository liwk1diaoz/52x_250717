#ifndef _PRJ_INC_
#define _PRJ_INC_

#include "UIWnd/UIFlow.h"
#include "UI/UIDisplay.h"
#include "System/SysMain.h"
#include "System/UICommon.h"
#include "System/AppCommon.h"
#include "System/SysCommon.h"
#include "AppControl/AppControl.h"
#include "SxTimer/SxTimer.h"
#include "kwrap/util.h"
#include "kwrap/sxcmd.h"
#include "kwrap/perf.h"
#include "Dx.h"
#include "GxVideo.h"
#include "GxDisplay.h"
#include "GxPower.h"
#include "GxSound.h"

#define Prj_GetVersionString()  "20210611_1"    //in ProjectInfo

#define Delay_DelayMs(x)    vos_util_delay_ms(x)
//#define UI_Show(iShow,bEn)  if(iShow==0) {GxDisplay_Set(LAYER_OSD1, LAYER_STATE_ENABLE, bEn);}


#define _IrRx_NONE_     1
#define SENSOR_FUNC     0
#define UI_SENSOR       0
//#NT#2019/7/29#Philex Lin-begin
#define IQ_FUNC         0
#define PHOTO_EXE_FUNC  1

#define SENSOR_MAX_NUM  4

//#NT#2019/7/29#Philex Lin-end
#endif
