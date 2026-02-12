#ifndef _ETHCAM_TEST_H_
#define _ETHCAM_TEST_H_

#ifndef ENABLE
#define ENABLE 1
#define DISABLE 0
#endif

#include <stdio.h>
#include <kwrap/type.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "avfile/media_def.h"

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


#define ETH_REARCAM_CLONE_FOR_DISPLAY 	ENABLE

#endif
