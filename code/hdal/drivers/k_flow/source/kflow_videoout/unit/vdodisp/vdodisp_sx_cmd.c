/**
    vdodisp, export commands to SxCmd

    @file       vdodisp_sx_cmd.c
    @ingroup    mVDODISP

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "vdodisp_int.h"
#include "vdodisp_sx_cmd.h"

#define CFG_FPS_BASE 15

BOOL x_vdodisp_sx_cmd_dump(unsigned char argc, char **argv)
{
	vdodisp_dump_info(vk_printk);
	return TRUE;
}

BOOL x_vdodisp_sx_cmd_dump_buf(unsigned char argc, char **argv)
{
	VDODISP_CMD cmd = {0};
    UINT32 result=0;
    UINT32 device =0;
	unsigned char idx = 1;

	if (argc > idx) {
		sscanf(argv[idx++], "%d", (int *)&device);
        if(device >= VDODISP_DEVID_MAX_NUM){
            device = VDODISP_DEVID_MAX_NUM-1;
        }
        DBG_DUMP("device %d \r\n",device);

	}

    if(argc > idx) {
		VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device);
		VDODISP_DBGINFO  *p_info = &p_ctrl->dbg_info;
        UINT32 bcontinue_dump = 0;

        sscanf(argv[idx++], "%d", (int *)&bcontinue_dump);
        if(bcontinue_dump) {
            p_info->reserved |= DBG_CON_DUMP_BUF;
        }else {
            p_info->reserved &= ~DBG_CON_DUMP_BUF;
        }

        DBG_DUMP("bcontinue_dump %d reserved %x \r\n",bcontinue_dump,p_info->reserved);
        return TRUE;
    }

	cmd.device_id = device;
	cmd.idx = VDODISP_CMD_IDX_DUMP_FLIP;
	cmd.prop.b_exit_cmd_finish = TRUE;
	result = vdodisp_cmd(&cmd);
	if(result!=0)
		return FALSE;

	return TRUE;
}

BOOL x_vdodisp_sx_cmd_fps(unsigned char argc, char **argv)
{
	INT32 i;
	INT32 fps[VDODISP_DEVID_MAX_NUM] = {0};

	// reset
	for (i = 0; i < VDODISP_DEVID_MAX_NUM; i++) {
		x_vdodisp_get_ctrl(i)->flip_ctrl.frame_cnt = 0;
	}
	DBG_DUMP("... need to wait %d sec ...\r\n", CFG_FPS_BASE);
	Delay_DelayMs(CFG_FPS_BASE * 1000);
	// calc
	for (i = 0; i < VDODISP_DEVID_MAX_NUM; i++) {
		fps[i] = (INT32)x_vdodisp_get_ctrl(i)->flip_ctrl.frame_cnt*100 / CFG_FPS_BASE;
	}
	// show
	for (i = 0; i < VDODISP_DEVID_MAX_NUM; i++) {
		DBG_DUMP("fps[%d] = %d.%d \r\n", i, fps[i]/100,fps[i]%100);
	}
	return TRUE;
}

BOOL x_vdodisp_sx_cmd_vd(unsigned char argc, char **argv)
{
    UINT32 device =0;
	unsigned char idx = 1;

	if (argc > idx) {
		sscanf(argv[idx++], "%d", (int *)&device);
        if(device >= VDODISP_DEVID_MAX_NUM){
            device = VDODISP_DEVID_MAX_NUM-1;
        }
        DBG_DUMP("device %d \r\n",device);
	}

    if(argc > idx) {
		VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device);
		VDODISP_DBGINFO  *p_info = &p_ctrl->dbg_info;
        UINT32 b_dump_vd = 0;
        sscanf(argv[idx++], "%d", (int *)&b_dump_vd);
        if(b_dump_vd) {
            p_info->reserved |= DBG_DUMP_VD;
        }else {
            p_info->reserved &= ~DBG_DUMP_VD;
        }

        DBG_DUMP("b_dump_vd %d reserved %x\r\n",b_dump_vd,p_info->reserved);
    }

    if(argc > idx) {
		VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device);
		VDODISP_DBGINFO  *p_info = &p_ctrl->dbg_info;
        UINT32 vd_threshold = 0;
        sscanf(argv[idx++], "%d", (int *)&vd_threshold);
        if(vd_threshold) {
            p_info->reserved &= ~DBG_VALUE_MASK;
            p_info->reserved |= (vd_threshold << DBG_VALUE_SHIFT)&DBG_VALUE_MASK ;
        } else {
            p_info->reserved &= ~DBG_VALUE_MASK;

        }

        DBG_DUMP("vd_threshold %d reserved %x\r\n",vd_threshold,p_info->reserved);
    }

    return TRUE;
}

