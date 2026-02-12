#ifndef _MOVIESTAMPINT_H
#define _MOVIESTAMPINT_H
#include "kwrap/task.h"
#include "kwrap/type.h"

// Flag
#define FLGMOVIESTAMP_IDLE          FLGPTN_BIT(0)
#define FLGMOVIESTAMP_UPDATE        FLGPTN_BIT(1)
#define FLGMOVIESTAMP_ALL           0xFFFFFFFF

extern THREAD_HANDLE MOVIESTAMPTSK_ID;
extern ID FLG_ID_MOVIESTAMP;

extern THREAD_RETTYPE MovieStampTsk(void);

#endif //_MOVIESTAMPINT_H
