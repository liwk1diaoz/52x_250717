#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_videodec/isf_vdodec.h"

int kflow_videodec_init(void)
{
	isf_reg_unit(ISF_UNIT(VDODEC,0), &isf_vdodec);

	return 0;
}

int kflow_videodec_uninit(void)
{
	return 0;
}

