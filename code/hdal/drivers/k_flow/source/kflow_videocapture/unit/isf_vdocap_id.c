#include "isf_vdocap_int.h"

//SEM_HANDLE ISF_VDOCAP_OUTQ_SEM_ID[VDOCAP_MAX_NUM*ISF_VDOCAP_OUT_NUM] = {0};
//SEM_HANDLE ISF_VDOCAP_OUT_SEM_ID[VDOCAP_MAX_NUM*ISF_VDOCAP_OUT_NUM] = {0};
SEM_HANDLE ISF_VDOCAP_PROC_SEM_ID;

void isf_vdocap_install_id(void)
{
    UINT32 i = 0;
    UINT32 j = 0;

    for (j = 0; j < VDOCAP_MAX_NUM; j++) {
    	ISF_UNIT* p_thisunit = DEV_UNIT(j);
		VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);

		if (((1 << j) & _vdocap_active_list && p_ctx)) {
		    for (i = 0; i < ISF_VDOCAP_OUT_NUM; i++) {
				OS_CONFIG_SEMPHORE(p_ctx->ISF_VDOCAP_OUTQ_SEM_ID[i], 0, 0, 0);
#if (ISF_NEW_PULL == ENABLE)
#else
				OS_CONFIG_SEMPHORE(p_ctx->ISF_VDOCAP_OUT_SEM_ID[i], 0, 1, 1);
#endif
			}
		}
	}
	SEM_CREATE(ISF_VDOCAP_PROC_SEM_ID, 1);
}

void isf_vdocap_uninstall_id(void)
{
    UINT32 i = 0;
    UINT32 j = 0;

    for (j = 0; j < VDOCAP_MAX_NUM; j++) {
    	ISF_UNIT* p_thisunit = DEV_UNIT(j);
		VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);

		if (((1 << j) & _vdocap_active_list && p_ctx)) {
		    for (i = 0; i < ISF_VDOCAP_OUT_NUM; i++) {
				SEM_DESTROY(p_ctx->ISF_VDOCAP_OUTQ_SEM_ID[i]);
#if (ISF_NEW_PULL == ENABLE)
#else
				SEM_DESTROY(p_ctx->ISF_VDOCAP_OUT_SEM_ID[i]);
#endif
			}
		}
	}
	SEM_DESTROY(ISF_VDOCAP_PROC_SEM_ID);
}

