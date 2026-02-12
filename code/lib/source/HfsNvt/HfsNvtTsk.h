#ifndef _HFSNVT_TSK_H_
#define _HFSNVT_TSK_H_
#include "HfsNvtInt.h"

typedef void HFS_OP_OPEN(HFSNVT_OPEN *pOpen);

typedef void HFS_OP_CLOSE(void);

typedef struct
{
    HFS_OP_OPEN             *Open;
    HFS_OP_CLOSE            *Close;
}HFS_OPS;


HFSNVT_CTRL *xHfsNvt_GetCtrl(void);
void         HfsNvt_MainTsk(void);
#endif //_HFSNVT_TSK_H_

