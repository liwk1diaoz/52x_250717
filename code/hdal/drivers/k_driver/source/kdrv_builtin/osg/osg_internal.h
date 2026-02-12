#ifndef _OSG_INTERNAL_H_
#define _OSG_INTERNAL_H_

#include "osg.h"
#include "h26x_def.h"
#include "kdrv_videoenc/kdrv_videoenc.h"

typedef struct _BUILTIN_OSG_STAMP_BUF {
	OSG_BUF_TYPE           type;
	UINT32                 size;
	uintptr_t              p_addr[2];
	UINT32                 ddr_id;
	UINT8                  swap;
} BUILTIN_OSG_STAMP_BUF;

typedef struct _BUILTIN_OSG_STAMP {
	BUILTIN_OSG_STAMP_BUF  buf;
	OSG_STAMP_IMG          img;
	OSG_STAMP_ATTR         attr;
	UINT8                  valid;
} BUILTIN_OSG_STAMP;

typedef struct _BUILTIN_OSG {
	int                    num;
	BUILTIN_OSG_STAMP      stamp[BUILTIN_OSG_MAX_NUM];
} BUILTIN_OSG;

extern int osg_module_init(void);

extern int builtin_osg_setup_h26x_stamp(H26XENC_VAR *pVar, UINT32 path);

extern int builtin_osg_setup_jpeg_stamp(KDRV_VDOENC_PARAM *jpeg, UINT32 path);

#endif //_OSG_INTERNAL_H_
