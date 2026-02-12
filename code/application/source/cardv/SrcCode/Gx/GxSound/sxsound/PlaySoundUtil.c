/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       PlaySoundUtil.c
    @ingroup    mIPRJAPKey

    @brief      Play Sound task API
                Internal functions of play sound task

    @note       Nothing.
    @date       2006/01/23

*/

/** \addtogroup mIPRJAPKey */
//@{

#include "PlaySoundInt.h"

#define __MODULE__          gxsound_util
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int gxsound_util_debug_level = NVT_DBG_WRN;
module_param_named(gxsound_util_debug_level, gxsound_util_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(gxsound_util_debug_level, "gxsound_util debug level");


extern UINT32 gGxSndRepPlayCnt;


/**
  Audio event handler of play sound task

  Audio event handler of play sound task

  @param UINT32 uiEventID: Audio event ID
  @return void
*/
void PlaySound_AudioHdl(UINT32 uiEventID)
{
	#if _TODO
	DBG_IND("=0x%x\r\n", uiEventID);
	if (uiEventID & AUDIO_EVENT_DMADONE) {
		if (gGxSndRepPlayCnt) {
			gGxSndRepPlayCnt--;
		}
		if (0 == gGxSndRepPlayCnt) {
			set_flg(FLG_ID_SOUND, FLGSOUND_STOP);
		} else {
			DBG_IND(":Play Again[%d]\r\n", gGxSndRepPlayCnt);
		}
	}
	DBG_IND(":=0x%x\r\n", uiEventID);
	#endif
}

//@}
