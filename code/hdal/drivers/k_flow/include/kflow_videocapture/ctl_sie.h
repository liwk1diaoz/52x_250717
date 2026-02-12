/**
    SIE CTRL Layer

    @file       sie_ctrl.h
    @ingroup    mILibIPH
    @note

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _CTL_SIE_H_
#define _CTL_SIE_H_

#include "kwrap/type.h"
#include "kflow_videocapture/ctl_sie_utility.h"

#define CTL_SIE_BUF_CB_IGNOR 0xffffffff

typedef enum {
	CTL_SIE_CBEVT_BUFIO			= 0 | CTL_SIE_EVENT_ISR_TAG,	//sts ref. to CTL_SIE_BUF_IO_CFG
	CTL_SIE_CBEVT_DIRECT		= 1 | CTL_SIE_EVENT_ISR_TAG,	//sie direct to ipp cb event
	CTL_SIE_CBEVT_ENG_SIE_ISR	= 2 | CTL_SIE_EVENT_ISR_TAG,	//sts ref. to CTL_SIE_INTE
	CTL_SIE_CBEVT_MAX = 3,
} CTL_SIE_CBEVT_TYPE;

/**
	SIE In/Out buffer config
*/
typedef enum _CTL_SIE_BUF_IO_CFG {
	CTL_SIE_BUF_IO_NEW		= 0x00000001,
	CTL_SIE_BUF_IO_PUSH		= 0x00000002,
	CTL_SIE_BUF_IO_LOCK		= 0x00000004,
	CTL_SIE_BUF_IO_UNLOCK	= 0x00000008,
	CTL_SIE_BUF_IO_ALL		= 0xffffffff,
	ENUM_DUMMY4WORD(CTL_SIE_BUF_IO_CFG)
} CTL_SIE_BUF_IO_CFG;

/**
	SIE Direct CB Cfg
*/
typedef enum _CTL_SIE_DIRECT_CFG {
	CTL_SIE_DIRECT_TRIG_START		= 0x00000001,	//trigger ipp start
	CTL_SIE_DIRECT_TRIG_STOP		= 0x00000002,	//trigger ipp stop
	CTL_SIE_DIRECT_FRM_CFG_START	= 0x00000004,	//trigger ipp when bp3(center of crop height)
	CTL_SIE_DIRECT_PUSH_RDY_BUF		= 0x00000008,	//push ready buffer
	CTL_SIE_DIRECT_DROP_BUF			= 0x00000010,	//drop public buffer
	CTL_SIE_DIRECT_SKIP_CFG			= 0x00000020,	//skip cfg next frame
	CTL_SIE_DIRECT_CFG_ALL			= 0xffffffff,
	ENUM_DUMMY4WORD(CTL_SIE_DIRECT_CFG)
} CTL_SIE_DIRECT_CFG;

typedef struct {
	CTL_SIE_CBEVT_TYPE cbevt;
	CTL_SIE_EVENT_FP fp;
	UINT32 sts;					//wait event status
} CTL_SIE_REG_CB_INFO;

#define GYRO_DATA_ITEM_MAX 6 //agyro_x,agyro_y,agyro_z,ags_x,ags_y,ags_z
typedef struct {
	UINT32 *data_num;				//output gyro data number
	UINT64 *t_crop_start;	//timestamp for crop_start
	UINT64 *t_crp_end;		//timestamp for crop_end
    UINT32 *time_stamp;
	INT32 *agyro_x;
	INT32 *agyro_y;
	INT32 *agyro_z;
	INT32 *ags_x;
	INT32 *ags_y;
	INT32 *ags_z;
} CTL_SIE_EXT_GYRO_DATA;

/* reserved[4] */
#define ctl_sie_vdofrm_reserved_mask_sieid(sieid, reserved)             (((sieid << 0) & 0xff) | (reserved & ~(0xff)))              // bit 0..7
#define ctl_sie_vdofrm_reserved_mask_ifechkdata(ifechkdata, reserved)   ((((ifechkdata << 8) & 0x100) | (reserved & ~(0x100))))     // bit 8
#define ctl_sie_vdofrm_reserved_mask_senfc(fc, reserved) 				((fc & 0x0000ffff) << 16) | (reserved & ~(0xffff0000))		// bit 16..31
/**
	vdo_frm reserved info
	resv1: direct mode get raw flag
	reserved[0]: [52X]:for proc cmd out buf wp using (ipp_dis_wp_en << 31 | sie_wp_en << 30 | set_idx << 4 | ddr_id)
	reserved[1]: [52X]: gyro data address, plz parsing to CTL_SIE_EXT_GYRO_DATA
	reserved[2]: [52X]:public buf id(direct mode only)
	reserved[3]: [52X]:public buf addr(direct mode only)
	reserved[4]:
        - bit 0..7 : [52X] ctl_sie_id (direct mode only)
        - bit 8 : [52X] ife_chk_data (direct mode only, CTL_SIE_DIRECT_TRIG_START only)
            * sie1 case : ife_chk_mode(52x/56x hw only support 0, 530 hw support 0,1 default 1)
            * sie2~x case :ife_chk_en
        - bit 16...31 sensor frame count (get from sensor drv)
	reserved[5]: [52X]:direct: ring buffer length, dram: dest crop construction of (x << 16 | y)
	reserved[6]: [52X]:dest crop construction of (w << 16 | h)
	reserved[7]: [52X]:encode rate
*/
typedef struct {
	UINT32 buf_id;		///for public buffer control
	UINT32 buf_addr;
	VDO_FRAME vdo_frm;
} CTL_SIE_HEADER_INFO;

void kflow_ctl_sie_init(void);
void kflow_ctl_sie_uninit(void);
UINT32 ctl_sie_buf_query(UINT32 num);
INT32 ctl_sie_init(UINT32 buf_addr, UINT32 buf_size);
INT32 ctl_sie_uninit(void);
UINT32 ctl_sie_open(void *open_cfg);	// CTL_SIE_OPEN_CFG
INT32 ctl_sie_close(UINT32 hdl);
INT32 ctl_sie_set(UINT32 hdl, CTL_SIE_ITEM item, void *data);
INT32 ctl_sie_get(UINT32 hdl, CTL_SIE_ITEM item, void *data);
INT32 ctl_sie_suspend(UINT32 hdl, void *data);
INT32 ctl_sie_resume(UINT32 hdl, void *data);

#endif //_CTL_SIE_H_