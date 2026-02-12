#include <kwrap/flag.h>
#include <kwrap/util.h>
#include "sys_fwload.h"

#if defined(_FW_TYPE_PARTIAL_) || defined(_FW_TYPE_PARTIAL_COMPRESS_)

static ID fwload_flg_id = 0;

void fwload_init(void)
{
	vos_flag_create(&fwload_flg_id, NULL, "fwload_flg_id");
	vos_flag_clr(fwload_flg_id, (FLGPTN)-1);
	// CODE_SECTION_01 has loaded by loader or u-boot
	vos_flag_set(fwload_flg_id, (FLGPTN)(1 << CODE_SECTION_01));
}

void fwload_set_done(CODE_SECTION section)
{
	vos_flag_set(fwload_flg_id, (FLGPTN)(1 << section));
}

void fwload_wait_done(CODE_SECTION section)
{
	FLGPTN flgptn;
	vos_flag_wait(&flgptn, fwload_flg_id, (FLGPTN)(1 << section), TWF_ANDW);
}

#else

void fwload_init(void)
{
}

void fwload_set_done(CODE_SECTION section)
{
}

void fwload_wait_done(CODE_SECTION section)
{
}

#endif