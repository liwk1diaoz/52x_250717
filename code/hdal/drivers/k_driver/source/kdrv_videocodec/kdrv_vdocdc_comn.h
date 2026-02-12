#ifndef _KDRV_VDOCDC_COMN_H_
#define _KDRV_VDOCDC_COMN_H_

#include "kwrap/type.h"
#include "kwrap/verinfo.h"

#include "kdrv_videoenc/kdrv_videoenc.h"

#include "h26x_def.h"

typedef struct _vdoenc_info_{
	H26XENC_VAR enc_var;
	BOOL   b_enable;
	BOOL   b_rst_i_frm;
	BOOL   b_is_i_frm;
	BOOL   b_emode_en;
	UINT64 setup_time;
	UINT64 start_time;
	UINT64 end_time;
	void (*vdoenc_3dnr_cb)(UINT32 path_id, UINT32 config);
} vdoenc_info;

extern vdoenc_info g_enc_info[KDRV_VDOENC_ID_MAX];

#define KDRV_VDOCDC_VERSION_0 (1)	//28
#define KDRV_VDOCDC_VERSION_1 (00)	//20,24,
#define KDRV_VDOCDC_VERSION_2 (000)	//8,12,16
#define KDRV_VDOCDC_VERSION_3 (00)	//0,4

#endif // _KDRV_VDOCDC_COMN_H_