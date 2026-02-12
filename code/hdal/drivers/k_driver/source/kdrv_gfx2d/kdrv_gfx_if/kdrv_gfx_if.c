#include "kwrap/error_no.h"
#include "kdrv_gfx2d/kdrv_gfx_if.h"

INT32 (*kdrv_gfx_if_affine_open)(UINT32 chip, UINT32 engine) = NULL;
INT32 (*kdrv_gfx_if_affine_trigger)(UINT32 id, KDRV_AFFINE_TRIGGER_PARAM *p_param,
						  KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data) = NULL;
INT32 (*kdrv_gfx_if_affine_close)(UINT32 chip, UINT32 engine) = NULL;

INT32 kdrv_gfx_if_register_affine_open(INT32 (*open)(UINT32 chip, UINT32 engine))
{
	kdrv_gfx_if_affine_open = open;
	return 0;
}

INT32 kdrv_gfx_if_register_affine_trigger(INT32 (*trigger)(UINT32 id, KDRV_AFFINE_TRIGGER_PARAM *p_param,
						  KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data))

{
	kdrv_gfx_if_affine_trigger = trigger;
	return 0;
}

INT32 kdrv_gfx_if_register_affine_close(INT32 (*close)(UINT32 chip, UINT32 engine))

{
	kdrv_gfx_if_affine_close = close;
	return 0;
}

INT32 kdrv_gfx_if_init(void)
{
	kdrv_gfx_if_affine_open = NULL;
	kdrv_gfx_if_affine_trigger = NULL;
	kdrv_gfx_if_affine_close = NULL;
	return 0;
}

INT32 kdrv_gfx_if_exit(void)
{
	return 0;
}