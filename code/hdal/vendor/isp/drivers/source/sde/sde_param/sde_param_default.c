#include "kwrap/type.h"
#include "sde_alg.h"

#include "sde_param_default.h"

static SDE_IF_IO_INFO sde_if_io_info = {
	.width = 1280,
	.height = 720,
	.in0_addr = 0x0,
	.in1_addr = 0x0,
	.cost_addr = 0x0,
	.out0_addr = 0x0,
	.out1_addr = 0x0,
	.in0_lofs = 1280,
	.in1_lofs = 1280,
	.cost_lofs = 1280 * 48,
	.out0_lofs = 1280,
	.out1_lofs = 1280,
};

static SDE_IF_CTRL_PARAM sde_if_ctrl_param = {
	.disp_val_mode = FALSE,
	.disp_op_mode = SDE_IF_MAX_DISP_63,
	.disp_inv_sel = SDE_IF_INV_0,
	.conf_en = TRUE,
};

static SDE_IF_PARAM_PTR sde_param_default = {
	&sde_if_io_info,
	&sde_if_ctrl_param,
};

UINT32 sde_get_param_default(void)
{
	return (UINT32)(&sde_param_default);
}
