#ifndef _LVIEWNVT_TSK_H_
#define _LVIEWNVT_TSK_H_
#include "LviewNvtInt.h"

typedef void LVIEW_OP_OPEN(LVIEWNVT_DAEMON_INFO   *pOpen);

typedef void LVIEW_OP_CLOSE(void);

typedef void LVIEW_OP_PUSH_FRAME(LVIEWNVT_FRAME_INFO *frame_info);

typedef struct
{
    LVIEW_OP_OPEN             *Open;
    LVIEW_OP_CLOSE            *Close;
	LVIEW_OP_PUSH_FRAME       *PushFrame;
}LVIEW_OPS;


LVIEWD_CTRL *xLviewNvt_GetCtrl(void);
#endif //_HFSNVT_TSK_H_

