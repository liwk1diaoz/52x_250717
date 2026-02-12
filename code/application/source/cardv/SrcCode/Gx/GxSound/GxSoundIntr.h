//XXXXXXXXXXXXXXX 2009/06/09 Lily Kao -begin

#ifndef _GXSOUNDINTR_H
#define _GXSOUNDINTR_H

#if defined(__KERNEL__)
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/stdio.h"
#include "kwrap/sxcmd.h"
#include <kwrap/spinlock.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include "GxSound.h"

#define kent_tsk()
#else
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/stdio.h"
#include "kwrap/sxcmd.h"
#include "hdal.h"
#include "hd_audioout.h"
#include <string.h>
#include "GxSound.h"

#define kent_tsk()

#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)
#endif

typedef enum _GXSND_AUD_ACTION {
	GXSND_AUD_ACTION_PAUSE,
	GXSND_AUD_ACTION_RESUME,
	GXSND_AUD_ACTION_CLOSE,
	GXSND_AUD_ACTION_MAX
} GXSND_AUD_ACTION;

//move from GxSoundID.h
#if defined __UITRON || defined __ECOS
extern UINT32 _SECTION(".kercfg_data") PLAYSOUNDTSK_ID;
extern UINT32 _SECTION(".kercfg_data") FLG_ID_SOUND;
#else
extern THREAD_HANDLE PLAYSOUNDTSK_ID;
extern THREAD_HANDLE PLAYDATATSK_ID;
extern ID FLG_ID_SOUND;
extern ID FLG_ID_DATA;
#endif

extern void     GxSound_Control(int cmd);
extern void     GxSound_SetSoundData(SOUND_DATA *pSound);
extern SOUND_DATA *GxSound_GetSoundDataByID(UINT32 uiSoundID);
extern UINT32 GxSound_GetSoundSRByID(UINT32 uiSoundID);
extern void GxSound_SetSoundSR(UINT32 audSR);
extern ER GxSound_AudAction(GXSND_AUD_ACTION audAct);
extern void PlaySoundQuit(void);
extern void GxSound_InstallID(void);
extern void GxSound_UninstallID(void);
extern BOOL GxSound_ChkModInstalled(void);


#endif
