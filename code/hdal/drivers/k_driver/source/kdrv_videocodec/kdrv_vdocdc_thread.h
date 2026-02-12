#ifndef _KDRV_VDOCDC_THREAD_H_
#define _KDRV_VDOCDC_THREAD_H_

#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kdrv_type.h"

#include "kdrv_videoenc/kdrv_videoenc.h"
#include "kdrv_videodec/kdrv_videodec.h"

#define VDOCDC_JOB_MAX	(8)

typedef enum _KDRV_VDOCDC_MODE_{
	VDOCDC_ENC_MODE = 0,
	VDOCDC_DEC_MODE,
	VDOCDC_NOT_SUPPORT
} KDRV_VDOCDC_MODE;

typedef struct _KDRV_VDOCDC_LLC_{
	UINT32 size;
	UINT8  add_idx;
	UINT8  run_idx;
	UINT8  job_num;
	UINT8  enc_job_num;
	UINT8  dec_job_num;
	UINT32 id[VDOCDC_JOB_MAX];
	UINT32 va[VDOCDC_JOB_MAX];
	KDRV_VDOCDC_MODE codec_mode[VDOCDC_JOB_MAX];
	KDRV_CALLBACK_FUNC *callback[VDOCDC_JOB_MAX];
	void *user_data[VDOCDC_JOB_MAX];
	void *p_var[VDOCDC_JOB_MAX];
	void *p_param[VDOCDC_JOB_MAX];
	void (*vdoenc_osdmask_cb)(UINT32 path_id, UINT32 isDo);
}KDRV_VDOCDC_LLC;

int kdrv_vdocdc_create_thread(void);
int kdrv_vdocdc_remove_thread(void);
int kdrv_vdocdc_add_job(KDRV_VDOCDC_MODE codec_mode, UINT32 id, UINT32 uiVaApbAddr, KDRV_CALLBACK_FUNC *p_cb_func, void *user_data, void *p_var, void *p_param);

int kdrv_vdocdc_set_llc_mem(UINT32 addr, UINT32 size);
UINT32 kdrv_vdocdc_get_llc_mem(void);
BOOL kdrv_vdocdc_get_job_done(void);
BOOL kdrv_vdocdc_get_enc_job_done(void);
BOOL kdrv_vdocdc_get_dec_job_done(void);
void kdrv_vdocdc_set_osdmask_cb(KDRV_VDOENC_OSDMASKCB *pOsdMaskCB);

#endif // _KDRV_VDOCDC_THREAD_H_