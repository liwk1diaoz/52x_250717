#include "h26xenc_rc.h"

#if defined(__LINUX)
rc_cb g_rc_cb = {0};
#elif defined(__FREERTOS)
rc_cb g_rc_cb = {h26xEnc_RcInit, h26xEnc_RcPreparePicture, h26xEnc_RcUpdatePicture, h26xEnc_RcGetLog};
#endif

int g_rc_dump_log = 0;

rc_cb *rc_cb_init(void)
{
	return &g_rc_cb;
}
