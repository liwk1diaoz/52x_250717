#ifndef _VRC_H_
#define _VRC_H_

#define SUPPORT_BUILTIN_RC	(1)

#include "h26xenc_rc_param.h"
#include "h26xenc_rate_control.h"

typedef struct _rc_cb_{
	int (*h26xEnc_RcInit)(H26XEncRC *pRc, H26XEncRCParam *pRCParam);
	int (*h26xEnc_RcPreparePicture)(H26XEncRC *pRc, H26XEncRCPreparePic *pPic);
	int (*h26xEnc_RcUpdatePicture)(H26XEncRC *pRc, H26XEncRCUpdatePic *pUPic);
    int (*h26xEnc_RcGetLog)(H26XEncRC *pRc, unsigned int *log_addr);
}rc_cb;

rc_cb *rc_cb_init(void);

#if SUPPORT_BUILTIN_RC
	#if H26X_RC_LOG_VERSION
	extern int h26xEnc_getRcVersion(char *str);
	#endif
	#if H26X_LOG_RC_INIT_INFO
	extern int h26xEnc_RcGetInitInfo(int chn, H26XEncRCInitInfo *pInfo);
	#endif
	#if H26X_RC_BR_MEASURE
	extern int h26xEnc_RcSetDumpBR(int log_chn);
	#endif
	#if H26X_RC_DBG_LEVEL
	extern int h26xEnc_RcSetDbgLv(unsigned int uiDbgLv);
	extern int h26xEnc_RcGetDbgLv(unsigned int *uiDbgLv);
	#endif
	#if H26X_RC_DBG_CMD
	extern int h26xEnc_RcGetCmd(int cmd_idx, char *cmd);
	extern int h26xEnc_RcSetCmd(char *str);
	#endif
#endif


#endif
