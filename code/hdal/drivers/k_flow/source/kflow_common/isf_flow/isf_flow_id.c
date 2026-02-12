#include "isf_flow_int.h"

SEM_HANDLE ISF_SEM_ID = {0};
SEM_HANDLE ISF_SEM_CFG_ID = {0};

void isf_flow_install_id(void)
{
	SEM_CREATE(ISF_SEM_ID, 1);
	SEM_CREATE(ISF_SEM_CFG_ID, 1);
}

void isf_flow_uninstall_id(void)
{
	SEM_DESTROY(ISF_SEM_ID);
	SEM_DESTROY(ISF_SEM_CFG_ID);
}

