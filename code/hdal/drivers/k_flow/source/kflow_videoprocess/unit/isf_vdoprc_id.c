#include "isf_vdoprc_int.h"

//SEM_HANDLE ISF_VDOPRC_IN_SEM_ID[VDOPRC_MAX_NUM] = {0};
SEM_HANDLE ISF_VDOPRC_OUTP_SEM_ID;
//SEM_HANDLE ISF_VDOPRC_OUTQ_SEM_ID[VDOPRC_MAX_NUM*ISF_VDOPRC_OUT_NUM] = {0};
//SEM_HANDLE ISF_VDOPRC_OUT_SEM_ID[VDOPRC_MAX_NUM*ISF_VDOPRC_OUT_NUM] = {0};
SEM_HANDLE ISF_VDOPRC_PROC_SEM_ID;
THREAD_HANDLE ISF_VDOPRC_EXT_TSK_ID = 0;
ID FLG_ID_VDOPRC_EXT = 0;

void isf_vdoprc_install_id(void)
{
	UINT32 i = 0;
	UINT32 j = 0;
	OS_CONFIG_SEMPHORE(ISF_VDOPRC_OUTP_SEM_ID, 0, 0, 0);
	for (j = 0; j < DEV_MAX_COUNT; j++) {
		ISF_UNIT* p_thisunit = DEV_UNIT(j);
		VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
		for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++) {
			OS_CONFIG_SEMPHORE(p_ctx->ISF_VDOPRC_OUTQ_SEM_ID[i], 0, 0, 0);
#if (ISF_NEW_PULL == ENABLE)
#else
			OS_CONFIG_SEMPHORE(p_ctx->ISF_VDOPRC_OUT_SEM_ID[i], 0, 1, 1);
#endif
		}
	}
	SEM_CREATE(ISF_VDOPRC_PROC_SEM_ID, 1);
	OS_CONFIG_FLAG(FLG_ID_VDOPRC_EXT);
	//#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
	//OS_CONFIG_TASK(ISF_VDOPRC_EXT_TSK_ID,   ISF_VDOPRC_EXT_TSK_PRI,     ISF_VDOPRC_EXT_TSK_STKSIZE,      isf_vdoprc_ext_tsk);
	//#endif
}

void isf_vdoprc_uninstall_id(void)
{
    UINT32 i = 0;
    UINT32 j = 0;

	SEM_DESTROY(ISF_VDOPRC_OUTP_SEM_ID);
	for (j = 0; j < DEV_MAX_COUNT; j++) {
		ISF_UNIT* p_thisunit = DEV_UNIT(j);
		VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
		for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++) {
			SEM_DESTROY(p_ctx->ISF_VDOPRC_OUTQ_SEM_ID[i]);
#if (ISF_NEW_PULL == ENABLE)
#else
			SEM_DESTROY(p_ctx->ISF_VDOPRC_OUT_SEM_ID[i]);
#endif
		}
	}
	SEM_DESTROY(ISF_VDOPRC_PROC_SEM_ID);
	rel_flg(FLG_ID_VDOPRC_EXT);
}


