#ifndef _PRJCFG_H_
#define _PRJCFG_H_

#ifndef ENABLE
#define ENABLE 1
#define DISABLE 0
#endif

#include <stdio.h>
#include <kwrap/type.h>

#define DEBUG_LEVEL_NONE                    0   ///< there is no debug message will show beside using debug_msg
#define DEBUG_LEVEL_ERROR                   1   ///< only debug_err() message will show
#define DEBUG_LEVEL_WARNING                 2   ///< only debug_wrn() and debug_err() message will show
#define DEBUG_LEVEL_INDICATION              3   ///< debug_ind(), debug_wrn() and debug_err() message will show

#define DEBUG_LEVEL                         DEBUG_LEVEL_ERROR   ///< debug level there are DEBUG_LEVEL_NONE/DEBUG_LEVEL_ERROR/DEBUG_LEVEL_WARNING/DEBUG_LEVEL_INDICATION can be select

#if (DEBUG_LEVEL >= DEBUG_LEVEL_ERROR)
#define debug_err(msg)                      do{debug_msg("ERR: ");debug_msg msg ;}while(0)
#else
#define debug_err(msg)
#endif

#if (DEBUG_LEVEL >= DEBUG_LEVEL_WARNING)
#define debug_wrn(msg)                      do{debug_msg("WRN: ");debug_msg msg ;}while(0)
#else
#define debug_wrn(msg)
#endif

#if (DEBUG_LEVEL >= DEBUG_LEVEL_INDICATION)
#define debug_ind(msg)                      do{debug_msg("IND: ");debug_msg msg ;}while(0)
#else
#define debug_ind(msg)
#endif

#define _Disp_IF8B_LCD1_PW35P00_HX8238D_  1

#if defined(_MODEL_CARDV_EVB_)
#include "PrjCfg_DVCam.h"
#elif defined(_MODEL_CARDV_1G_EVB_) || defined(_MODEL_CARDV_1G_EVB_LVGL_)
#include "PrjCfg_DVCam.h"
#elif defined(_MODEL_CARDV_Alexa_EVB_)
#include "PrjCfg_DVCam.h"
#elif defined(_MODEL_CARDV_ETHCAM_RX_EVB_)
#include "PrjCfg_EthCamRx.h"
#elif defined(_MODEL_580_CARDV_EVB_) || defined(_MODEL_580_CARDV_EVB_FAST_BT_)
#include "PrjCfg_DVCam.h"
#elif defined(_MODEL_580_CARDV_ETHCAM_RX_EVB_)
#include "PrjCfg_EthCamRx.h"
#elif (defined(_MODEL_529_CARDV_V70_) || defined(_MODEL_529_CARDV_V70_FAST_BT_))
#include "PrjCfg_V70.h"
#elif (defined(_MODEL_CARDV_NS2_) || defined(_MODEL_CARDV_NS2_FAST_BT_))
#include "PrjCfg_NS2.h"
#elif (defined(_MODEL_580_CARDV_Q80_) || defined(_MODEL_580_CARDV_Q80_FAST_BT_))
#include "PrjCfg_Q80.h"
#elif defined(_MODEL_CARDV_D160_)
#include "PrjCfg_D160.h"
#elif (defined(_MODEL_580_SDV_SJ10_) || defined(_MODEL_580_SDV_SJ10_FAST_BT_))
#include "PrjCfg_SJ10.h"
#elif (defined(_MODEL_580_SDV_C300_) || defined(_MODEL_580_SDV_C300_FAST_BT_))
#include "PrjCfg_C300.h"
#elif defined(_MODEL_580_SDV_DVF100_EMMC_)
#include "PrjCfg_DVF100.h"
#elif (defined(_MODEL_580_CARDV_ETHCAM_TX_EVB_))
#include "PrjCfg_EthCamTx.h"
#elif (defined(_MODEL_580_CARDV_ETHCAM_RX_F1Gyro_))
#include "PrjCfg_EthCamRx_F1Gyro.h"
#elif (defined(_MODEL_580_HUNTING_EVB_LINUX_) || defined(_MODEL_580_HUNTING_NOR_EVB_LINUX_))
#include "PrjCfg_HUNTING.h"
#else //Unknown MODEL
#warning Unknown MODEL?
#warning Please assign your "PrjCfg_MODEL.h" in "PrjCfg.h"
#error (see above)
#endif

#include "PrjCfg_Option.h"
#include "PrjCfg_Default.h"

#endif
