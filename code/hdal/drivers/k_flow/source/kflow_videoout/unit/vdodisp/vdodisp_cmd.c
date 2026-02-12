/**
    vdodisp, Service command function implementation

    @file       vdodisp_cmd.c
    @ingroup    mVDODISP

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "vdodisp_int.h"
#include "kdrv_videoout/kdrv_vdoout.h"
#include "kwrap/file.h"
#include "kwrap/cpu.h"
#include <kwrap/perf.h>
#define SAVE_KEEPLAST   (0)
VDODISP_ER x_vdodisp_dump_buf(CHAR *filePath, VDO_FRAME *pImgBuf);

#if !defined(__LINUX)
#include <stdio.h>
#include <string.h>
#endif
//_TODO
#define dma_flushWriteCache(uiAddr, uiSize)

#if (DEVICE_COUNT >= 2)
#define CFG_FAKE_IDE2 DISABLE
#else
#define CFG_FAKE_IDE2 ENABLE
#endif

static const VDODISP_CMD_DESC m_vdodisp_call_tbl[] = {
	// idx ----------------------- Command Pointer ------- Input Data Size ----------- Output Data Size -------
	{VDODISP_CMD_IDX_UNKNOWN,    NULL,                  0U,                         0U},
	{VDODISP_CMD_IDX_SET_DISP,   x_vdodisp_cmd_set_disp, sizeof(VDODISP_DISP_DESC),  0U},
	{VDODISP_CMD_IDX_GET_DISP,   x_vdodisp_cmd_get_disp, 0U,                         sizeof(VDODISP_DISP_DESC)},
	{VDODISP_CMD_IDX_SET_FLIP,   x_vdodisp_cmd_set_flip, sizeof(VDO_FRAME *),           0U},
	{VDODISP_CMD_IDX_DUMP_FLIP,  x_vdodisp_cmd_dump_flip, 0U,                        0U},
	{VDODISP_CMD_IDX_RELEASE,    x_vdodisp_cmd_release, sizeof(VDO_FRAME *),           0U},
	{VDODISP_CMD_IDX_SET_EVENT_CB, x_vdodisp_cmd_set_event_cb, sizeof(VDODISP_EVENT_CB), 0U},
	{VDODISP_CMD_IDX_GET_EVENT_CB, x_vdodisp_cmd_get_event_cb, 0U,                   sizeof(VDODISP_EVENT_CB)},
};
/******************************************************************************/

#if(CFG_FAKE_IDE2)
static KDRV_DEV_ENGINE m_tbl_disp_eng_id[2] ={KDRV_VDOOUT_ENGINE0,KDRV_VDOOUT_ENGINE0};
#else
static KDRV_DEV_ENGINE m_tbl_disp_eng_id[2] ={KDRV_VDOOUT_ENGINE0,KDRV_VDOOUT_ENGINE1};
#endif

/**
    Get command function table

    @param[out] p_num total function number
    @return command function table
*/
const VDODISP_CMD_DESC *x_vdodisp_get_call_tbl(UINT32 *p_num)
{
	*p_num = sizeof(m_vdodisp_call_tbl) / sizeof(m_vdodisp_call_tbl[0]);
	return m_vdodisp_call_tbl;
}

VDODISP_ER x_vdodisp_cmd_set_disp(const VDODISP_CMD *p_cmd)
{
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(p_cmd->device_id);
	p_ctrl->flip_ctrl.disp_desc = *(VDODISP_DISP_DESC *)p_cmd->in.p_data;
	p_ctrl->flip_ctrl.is_need_update = TRUE;
	return VDODISP_ER_OK;
}

VDODISP_ER x_vdodisp_cmd_get_disp(const VDODISP_CMD *p_cmd)
{
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(p_cmd->device_id);
	*(VDODISP_DISP_DESC *)p_cmd->out.p_data = p_ctrl->flip_ctrl.disp_desc;
	return VDODISP_ER_OK;
}

VDODISP_ER x_vdodisp_cmd_dump_flip(const VDODISP_CMD *p_cmd)
{
    VDODISP_ER ret = 0 ;
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(p_cmd->device_id);
	if (p_ctrl->flip_ctrl.p_img) {
    	static char path[48];
        static UINT32 count =0;
		char root_dir[11];

#if defined (__LINUX)
		snprintf(root_dir, sizeof(root_dir), "//mnt//sd//");
#elif defined (__FREERTOS)
		snprintf(root_dir, sizeof(root_dir), "A:\\");
#endif
		snprintf(path, sizeof(path) - 1, "%sDisp%d_%d_%d_%d_%x_%d.raw"
		         , root_dir
				 , p_cmd->device_id
				 , (int)p_ctrl->flip_ctrl.p_img->loff[0]
				 , (int)p_ctrl->flip_ctrl.p_img->size.w
				 , (int)p_ctrl->flip_ctrl.p_img->size.h
				 , p_ctrl->flip_ctrl.p_img->pxlfmt
				 , (int)count);
		DBG_DUMP("%s\r\n", path);
        count++;
		ret = x_vdodisp_dump_buf(path, p_ctrl->flip_ctrl.p_img);

	} else {
		DBG_ERR("device[%d] has no image can dump.\r\n", p_cmd->device_id);
        return VDODISP_ER_SYS;
	}
	return ret;
}
KDRV_VDDO_DISPBUFFORMAT x_vdodisp_get_pxlfmt(VDO_PXLFMT pxlfmt)
{
	switch (pxlfmt) {
	case VDO_PXLFMT_YUV420:
		return VDDO_DISPBUFFORMAT_YUV420PACK;
	case VDO_PXLFMT_YUV422:
		return VDDO_DISPBUFFORMAT_YUV422PACK;
	default:
		DBG_ERR("invalid pxlfmt(%x).\r\n",pxlfmt);
		return VDDO_DISPBUFFORMAT_YUV420PACK;
	}
}
VDODISP_ER x_vdodisp_cmd_set_event_cb(const VDODISP_CMD *p_cmd)
{
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(p_cmd->device_id);
	p_ctrl->event_cb = *(VDODISP_EVENT_CB *)p_cmd->in.p_data;
	return VDODISP_ER_OK;
}

VDODISP_ER x_vdodisp_cmd_get_event_cb(const VDODISP_CMD *p_cmd)
{
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(p_cmd->device_id);
	*(VDODISP_EVENT_CB *)p_cmd->out.p_data = p_ctrl->event_cb;
	return VDODISP_ER_OK;
}

VDODISP_ER x_vdodisp_cmd_release(const VDODISP_CMD *p_cmd)
{
	VDODISP_ER er = VDODISP_ER_OK;
	VDODISP_DEVID device_id = p_cmd->device_id;
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device_id);
	VDO_FRAME *p_img_next = *(VDO_FRAME **)p_cmd->in.p_data;
	VDO_FRAME *p_img_curr = p_ctrl->flip_ctrl.p_img;
	KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer = {0};
	KDRV_DEV_ENGINE disp_eng = m_tbl_disp_eng_id[device_id];
	VDODISP_DISP_DESC *p_desc = &p_ctrl->flip_ctrl.disp_desc;
	UINT32 handler = KDRV_DEV_ID(0, disp_eng, 0);

	while (p_img_next != NULL && p_img_curr != NULL) {
		// change current display to this frame
		if (p_img_next->size.w != p_img_curr->size.w || p_img_next->size.h != p_img_curr->size.h) {
			DBG_ERR("temp image size isn't equal to release image (%d,%d) != (%d,%d)\r\n",
					p_img_next->size.w,
					p_img_next->size.h,
					p_img_curr->size.w,
					p_img_curr->size.h);
			er = VDODISP_ER_PARAM;
			break;
		}
        DBG_IND("y %x uv %x \r\n",p_img_next->addr[0],p_img_next->addr[1]);
		// copy data
		if (gximg_copy_data_ex(p_img_curr, NULL, p_img_next, NULL, GXIMG_CP_ENG2) != 0) {
			DBG_ERR("failed to copy data.\r\n");
			er = VDODISP_ER_PARAM;
			break;
		}
		memset(&kdrv_disp_layer, 0, sizeof(KDRV_VDDO_DISPLAYER_PARAM));
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_y = p_img_next->addr[0];
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_cbcr =  p_img_next->addr[1];
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.layer = VDDO_DISPLAYER_VDO1;
		er=kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFADDR, &kdrv_disp_layer);
		if(er!=0) {
     		DBG_ERR("failed BUFADDR.%d\r\n",er);
			goto exit;

		} else {
			er = kdrv_vddo_trigger(handler, NULL);

            #if SAVE_KEEPLAST
            {
            	static char path[48];
                static UINT32 count =0;
            	char root_dir[11];

#if defined (__LINUX)
            	snprintf(root_dir, sizeof(root_dir), "//mnt//sd//");
#elif defined (__FREERTOS)
            	snprintf(root_dir, sizeof(root_dir), "A:\\");
#endif
            	snprintf(path, sizeof(path) - 1, "%sDispkeep%d_%d_%d_%d_%x_%d.raw"
            	         , root_dir
            			 , p_cmd->device_id
            			 , (int)p_img_next->loff[0]
            			 , (int)p_img_next->size.w
            			 , (int)p_img_next->size.h
            			 , p_img_next->pxlfmt
            			 , (int)count);
            	DBG_DUMP("%s\r\n", path);
                count++;
            	x_vdodisp_dump_buf(path, p_img_next);

            }
            #endif
            break;
		}
	}

	if (p_img_next == NULL || er != VDODISP_ER_OK) {
		// use a small buffer with a black color to display on screen
		// don't disalbe ide, it may cause image unit conflict.
		memset(p_ctrl->black_buf.m_y, 0x00, BLACK_BUF_SIZE);
		memset(p_ctrl->black_buf.m_uv, 0x80, BLACK_BUF_SIZE);

        vos_cpu_dcache_sync((VOS_ADDR)p_ctrl->black_buf.m_y, BLACK_BUF_SIZE, VOS_DMA_TO_DEVICE);
        vos_cpu_dcache_sync((VOS_ADDR)p_ctrl->black_buf.m_uv, BLACK_BUF_SIZE, VOS_DMA_TO_DEVICE);

		memset(&kdrv_disp_layer, 0, sizeof(KDRV_VDDO_DISPLAYER_PARAM));

		kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_y = (UINT32)p_ctrl->black_buf.m_y;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_cbcr = (UINT32)p_ctrl->black_buf.m_uv;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.layer = VDDO_DISPLAYER_VDO1;
		er=kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFADDR, &kdrv_disp_layer);
		if(er!=0)
			goto exit;

		memset(&kdrv_disp_layer, 0, sizeof(KDRV_VDDO_DISPLAYER_PARAM));

        if(p_img_curr!=0) {
		    kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.format = x_vdodisp_get_pxlfmt(p_img_curr->pxlfmt);
        } else {
            kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.format = VDDO_DISPBUFFORMAT_YUV420PACK;
        }
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_width = 2;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_height = 2;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_line_ofs = 4;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_width = p_desc->wnd.w;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_height = p_desc->wnd.h;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_x = p_desc->wnd.x;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_y = p_desc->wnd.y;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.layer = VDDO_DISPLAYER_VDO1;
		er = kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFWINSIZE, &kdrv_disp_layer);
		if(er==0)
			er = kdrv_vddo_trigger(handler, NULL);
	}
exit:
	// release buffer
	if (p_ctrl->event_cb.fp_release) {
		p_ctrl->event_cb.fp_release(device_id, p_img_curr);
	}

	if (p_img_next != NULL) {
		if (er != VDODISP_ER_OK) {
			// because of not ok, we direct to release temp buffer
			if (p_ctrl->event_cb.fp_release) {
				p_ctrl->event_cb.fp_release(device_id, p_img_next);
			}
			p_ctrl->flip_ctrl.p_img = NULL;
		} else {
			p_ctrl->flip_ctrl.p_img = p_img_next;
		}
	} else {
		p_ctrl->flip_ctrl.p_img = NULL;
	}

	return x_vdodisp_err(device_id, er);
}

VDODISP_ER x_vdodisp_cmd_set_flip(const VDODISP_CMD *p_cmd)
{
	BOOL b_update_dispobj = FALSE;
	VDODISP_ER er = VDODISP_ER_OK;
	VDODISP_DEVID device_id = p_cmd->device_id;
	VDODISP_CTRL *p_ctrl = x_vdodisp_get_ctrl(device_id);
	VDO_FRAME *p_img_next = *(VDO_FRAME **)p_cmd->in.p_data;
	VDO_FRAME *p_img_curr = p_ctrl->flip_ctrl.p_img;
	KDRV_VDDO_DISPLAYER_PARAM kdrv_disp_layer = {0};
	KDRV_DEV_ENGINE disp_eng = m_tbl_disp_eng_id[device_id];
	UINT32 handler = KDRV_DEV_ID(0, disp_eng, 0);
    VDODISP_DBGINFO  *p_info = &p_ctrl->dbg_info;
    VOS_TICK tigger_s,tigger_e=0;

	if (p_img_next == NULL) {
#if 0
		if (p_ctrl->event_cb.fp_frm_end) {
			p_ctrl->event_cb.fp_frm_end(device_id);
		}
#endif
		return E_OK;
	}

	if (p_ctrl->flip_ctrl.is_need_update) {
		b_update_dispobj = TRUE;
	} else if (p_img_curr == NULL) {
		b_update_dispobj = TRUE;
	} else if (p_img_next->size.w != p_img_curr->size.w ||
			   p_img_next->size.h != p_img_curr->size.h ||
			   p_img_next->pxlfmt != p_img_curr->pxlfmt ||
			   p_img_next->loff[0] != p_img_curr->loff[0] ||
			   p_img_next->loff[1] != p_img_curr->loff[1]) {
		b_update_dispobj = TRUE;
	}

	if (b_update_dispobj) {
		VDODISP_DISP_DESC *p_desc = &p_ctrl->flip_ctrl.disp_desc;

		memset(&kdrv_disp_layer, 0, sizeof(KDRV_VDDO_DISPLAYER_PARAM));
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_y = p_img_next->addr[0];
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_cbcr = p_img_next->addr[1];
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.layer = VDDO_DISPLAYER_VDO1;
		DBG_IND("addr %x %x %x\r\n",p_img_next->addr[0],p_img_next->addr[1],p_img_next->addr[2]);
		er=kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFADDR, &kdrv_disp_layer);
		if(er!=0)
			goto exit;
		DBG_IND("win x:%d y:%d w:%d h:%d\r\n",p_desc->wnd.x,p_desc->wnd.y,p_desc->wnd.w,p_desc->wnd.h);
		DBG_IND("buf w:%d h:%d loff %d\r\n",p_img_next->size.w, p_img_next->size.h,p_img_next->loff[0]);
		DBG_IND("degree %d\r\n",p_desc->degree);

		memset(&kdrv_disp_layer, 0, sizeof(KDRV_VDDO_DISPLAYER_PARAM));

		if (p_img_next->reserved[2] == 0 || p_img_next->reserved[3] == 0) {
			//no dis
			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_width = p_img_next->size.w;
			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_height = p_img_next->size.h;
		} else {
			// with dis
			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_width = p_img_next->reserved[2];
			kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_height = p_img_next->reserved[3];
		}
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.buf_line_ofs = p_img_next->loff[0];
		// set pixel format
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.format = x_vdodisp_get_pxlfmt(p_img_next->pxlfmt);
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_width = p_desc->wnd.w;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_height = p_desc->wnd.h;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_x = p_desc->wnd.x;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.win_ofs_y = p_desc->wnd.y;
		kdrv_disp_layer.SEL.KDRV_VDDO_BUFWINSIZE.layer = VDDO_DISPLAYER_VDO1;
		er=kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFWINSIZE, &kdrv_disp_layer);
		if(er!=0)
			goto exit;
		// set dir
		memset(&kdrv_disp_layer, 0, sizeof(KDRV_VDDO_DISPLAYER_PARAM));
		switch (p_desc->degree) {
		case VDODISP_ROTATE_0:
			kdrv_disp_layer.SEL.KDRV_VDDO_OUTDIR.buf_out_dir = VDDO_DISPOUTDIR_NORMAL;
			break;
		case VDODISP_ROTATE_180:
			kdrv_disp_layer.SEL.KDRV_VDDO_OUTDIR.buf_out_dir = VDDO_DISPOUTDIR_ROT_180;
			break;
		case VDODISP_ROTATE_HORZ:
			kdrv_disp_layer.SEL.KDRV_VDDO_OUTDIR.buf_out_dir = VDDO_DISPOUTDIR_HRZ_FLIP;
			break;
		case VDODISP_ROTATE_VERT:
			kdrv_disp_layer.SEL.KDRV_VDDO_OUTDIR.buf_out_dir = VDDO_DISPOUTDIR_VTC_FLIP;
			break;
		default:
			break;
		}
		if (p_desc->degree != VDODISP_ROTATE_NO_HANDLE) {
            kdrv_disp_layer.SEL.KDRV_VDDO_OUTDIR.layer = VDDO_DISPLAYER_VDO1;
			er=kdrv_vddo_set(handler, VDDO_DISPLAYER_OUTDIR, &kdrv_disp_layer);
			if(er!=0)
				goto exit;
		}

#if 0	//roi disable
		// reset roi to default no roi
		memset(&lyr_param, 0, sizeof(DISPLAYER_PARAM));
		p_dispobj->disp_lyr_ctrl(dispobj_lyr, DISPLAYER_OP_SET_BUFXY, &lyr_param);
#endif
		p_ctrl->flip_ctrl.is_need_update = FALSE;

		kdrv_disp_layer.SEL.KDRV_VDDO_ENABLE.layer = VDDO_DISPLAYER_VDO1;
		kdrv_disp_layer.SEL.KDRV_VDDO_ENABLE.en = TRUE;
		er=kdrv_vddo_set(handler, VDDO_DISPLAYER_ENABLE, &kdrv_disp_layer);
		if(er!=0)
			goto exit;
	}
#if 0	//roi disable
	// change display roi
	if (p_img_next->reserved[2] != 0 && p_img_next->reserved[3] != 0) {
		// dis
		memset(&lyr_param, 0, sizeof(DISPLAYER_PARAM));
		lyr_param.SEL.SET_BUFXY.ui_buf_ofs_x = p_img_next->reserved[0];
		lyr_param.SEL.SET_BUFXY.ui_buf_ofs_y = p_img_next->reserved[1];
		DBG_IND("ofs_x %d ofs_y %d \r\n",lyr_param.SEL.SET_BUFXY.ui_buf_ofs_x ,lyr_param.SEL.SET_BUFXY.ui_buf_ofs_y);
		p_dispobj->disp_lyr_ctrl(dispobj_lyr, DISPLAYER_OP_SET_BUFXY, &lyr_param);
	}
#endif
	memset(&kdrv_disp_layer, 0, sizeof(KDRV_VDDO_DISPLAYER_PARAM));
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_y = p_img_next->addr[0];
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.addr_cbcr = p_img_next->addr[1];
	kdrv_disp_layer.SEL.KDRV_VDDO_BUFADDR.layer = VDDO_DISPLAYER_VDO1;
	DBG_IND("addr %x %x %x\r\n",p_img_next->addr[0],p_img_next->addr[1],p_img_next->addr[2]);
	er=kdrv_vddo_set(handler, VDDO_DISPLAYER_BUFADDR, &kdrv_disp_layer);
	if(er!=0){
        DBG_ERR("er %d%x %d\r\n",er,disp_eng,device_id);
		goto exit;
	}
	else {
        vos_perf_mark(&tigger_s);
		er=kdrv_vddo_trigger(handler, NULL);
        vos_perf_mark(&tigger_e);

        if(p_info->reserved &DBG_DUMP_VD) {
            if(p_info->reserved&DBG_VALUE_MASK){
                UINT32 thredhold = (p_info->reserved&DBG_VALUE_MASK)>>DBG_VALUE_SHIFT;
                if(((tigger_e-tigger_s)/1000)>thredhold)
                    DBG_DUMP("%d ms > %d \r\n",(tigger_e-tigger_s)/1000,thredhold);

            } else {
                DBG_DUMP("%d ms\r\n",(tigger_e-tigger_s)/1000);
            }

        }
        if(p_info->reserved &DBG_CON_DUMP_BUF) {
            x_vdodisp_cmd_dump_flip(p_cmd);
        }
	}


	if (b_update_dispobj && p_ctrl->event_cb.fp_stable != NULL) {
		p_ctrl->event_cb.fp_stable(device_id);
	}

    #if IDE_CHK_ADDR
    //check ide Video1 YUV Current Buffer Address
    er=x_vdodisp_check_addr(p_img_next);
    if(er!=0){
        DBG_ERR("er %d\r\n",er);
		goto exit;
	}
    #endif

#if 0
	// notify frame-end
	if (p_ctrl->event_cb.fp_frm_end) {
		p_ctrl->event_cb.fp_frm_end(device_id);
	}
#endif

	// add count
	p_ctrl->flip_ctrl.frame_cnt++;
exit:
	// release previous buffer
	if(er==0) {
	//if (p_ctrl->event_cb.fp_release && p_img_curr != NULL) {
	if ((p_ctrl->event_cb.fp_release)&&(p_img_curr)) {
		p_ctrl->event_cb.fp_release(device_id, p_img_curr);
	}
	p_ctrl->flip_ctrl.p_img = p_img_next;
	}
	return x_vdodisp_err(device_id, er);
}

VDODISP_ER x_vdodisp_dump_buf(CHAR *filePath, VDO_FRAME *pImgBuf)
{
	UINT32 uiBufAddr, uiBufSize;
	VOS_FILE     filehdl = 0;
	CHAR         *path;
	IRECT        TmpRect = {0};

	if (!pImgBuf) {
		DBG_ERR("pImgBuf is Null\r\n");
		return VDODISP_ER_PARAM;
	}
	DBG_FUNC_BEGIN("[dump]\r\n");
	DBG_MSG("[dump]pImgBuf w = %lu, h = %lu, fmt = %lu\r\n", pImgBuf->pw[0], pImgBuf->ph[0], pImgBuf->pxlfmt);
	DBG_MSG("[dump]pImgBuf PxlAddr = 0x%08X 0x%08X 0x%08X, LineOffset = %lu %lu %lu\r\n",
			pImgBuf->addr[0], pImgBuf->addr[1], pImgBuf->addr[2], pImgBuf->loff[0], pImgBuf->loff[1], pImgBuf->loff[2]);

	path = filePath;
	TmpRect.w = pImgBuf->pw[0];
	TmpRect.h = pImgBuf->ph[0];

	DBG_MSG("[dump]filepath =%s\r\n", path);
	switch (pImgBuf->pxlfmt) {
	case VDO_PXLFMT_YUV422:
	case VDO_PXLFMT_YUV420:		/* Y data */
		uiBufAddr   = pImgBuf->addr[0];
		//GxImg_CalcPlaneRect(pImgBuf, 0, &TmpRect, &TmpPlaneRect);
		uiBufSize   = (pImgBuf->loff[0] * TmpRect.h);
		filehdl = vos_file_open(path, O_CREAT | O_WRONLY | O_SYNC,0);
		if (filehdl == (VOS_FILE)(-1)) {
			DBG_ERR("open fail %s\n", path);
			return VDODISP_ER_SYS;
		}
		vos_file_write(filehdl, (UINT8 *)uiBufAddr, uiBufSize);

		DBG_MSG("[dump]BuffY saved. uiBufAddr = 0x%08X, uiBufSize = %lu\r\n", uiBufAddr, uiBufSize);

		// UV packed
		uiBufAddr   = pImgBuf->addr[1];
		//GxImg_CalcPlaneRect(pImgBuf, 1, &TmpRect, &TmpPlaneRect);
        if(pImgBuf->pxlfmt==VDO_PXLFMT_YUV422) {
		    uiBufSize   = (pImgBuf->loff[1] * TmpRect.h);
		} else {
		    uiBufSize   = (pImgBuf->loff[1] * TmpRect.h / 2);
		}		vos_file_write(filehdl, (UINT8 *)uiBufAddr, uiBufSize);
		vos_file_close(filehdl);
		DBG_MSG("[dump]BuffUV saved. uiBufAddr = 0x%08X, uiBufSize = %lu\r\n", uiBufAddr, uiBufSize);
    	break;
    default:
    	DBG_ERR("pixel format %d\r\n", pImgBuf->pxlfmt);
    	return VDODISP_ER_PARAM;
    	break;
    }
	DBG_FUNC_END("[dump]\r\n");
	return VDODISP_ER_OK;
}
