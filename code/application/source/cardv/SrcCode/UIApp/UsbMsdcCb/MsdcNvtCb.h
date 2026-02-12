#ifndef _MSDCNVTCB_H
#define _MSDCNVTCB_H
#include "umsd.h"

typedef struct _MSDCNVTCB_OPEN {
	void (*fpUSBMakerInit)(USB_MSDC_INFO *pUSBMSDCInfo);
} MSDCNVTCB_OPEN, *PMSDCNVTCB_OPEN;

extern void msdcnvt_open(MSDCNVTCB_OPEN *p_open);
extern void msdcnvt_close(void);
extern void msdcnct_attach(USB_MSDC_INFO *p_info);
extern void msdcnvt_net(BOOL b_turnon);

#endif
