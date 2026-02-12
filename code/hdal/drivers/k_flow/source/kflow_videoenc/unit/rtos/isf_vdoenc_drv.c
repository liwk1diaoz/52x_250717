#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_videoenc/isf_vdoenc.h"
#include "nmediarec_api.h"

int kflow_videoenc_init(void)
{
	nmr_vdocodec_install_id();
	isf_reg_unit(ISF_UNIT(VDOENC,0), &isf_vdoenc);

	return 0;
}

int kflow_videoenc_uninit(void)
{
	nmr_vdocodec_uninstall_id();

	return 0;
}

