#include "dis_alg_lib.h"
#include "kflow_videoprocess/isf_vdoprc.h"


//=============================================================================
// global
//=============================================================================
VDOPRC_DIS_PLUGIN dis_module;
UINT32 g_dis_sensor_w = 1920;
UINT32 g_dis_sensor_h = 1080;

UINT32 g_dis_isp_id = 0;
UINT32 g_dis_scale_up_ratio = 0;
UINT32 g_dis_subsample_level = 0;


//=============================================================================
// function declaration
//=============================================================================
VDOPRC_DIS_PLUGIN *dis_get_module(void)
{
	return &dis_module;
}

ER dis_alg_flow_open(UINT32 isp_id, UINT32 max_w, UINT32 max_h, UINT32* buffer_size)
{
    ER rt = E_OK;

    //calculate memory buffer size
    g_dis_sensor_w = max_w;
    g_dis_sensor_h = max_h;
    *buffer_size = nvt_dis_cal_buffer_size();

    //open dis task
    rt = dis_alg_task_open();

    return rt;
}

ER dis_alg_flow_start(UINT32 isp_id, UINT32 buffer_addr, UINT32 subsample_level, UINT32 scale_up_ratio)
{
    ER rt = E_OK;
    MEM_RANGE   share_mem;
    DIS_ALG_PARAM dis_parm_in;

    //allocate memory
    share_mem.addr = buffer_addr;
    share_mem.size = nvt_dis_cal_buffer_size();

    nvt_dis_buffer_set((void*)&share_mem);

    //set dis_parm_in
    dis_parm_in.subsample_sel  = subsample_level;
    dis_parm_in.scale_up_ratio = scale_up_ratio;

    rt = dis_alg_task_set(&dis_parm_in);

    if(rt != E_OK){
        DBG_ERR("dis alg set error\n\r");
        return rt;
    }
    g_dis_isp_id = isp_id;
    g_dis_scale_up_ratio = scale_up_ratio;
    g_dis_subsample_level = subsample_level;

    //init dis task
    rt = dis_alg_task_init();

    return rt;
}


ER dis_alg_flow_proc(UINT32 isp_id, UINT32 frm_cnt, UINT32* mv_valid, INT32* mv_dx, INT32* mv_dy)
{
    ER rt = E_OK;
    DIS_ALG_PARAM dis_parm_out = {0};

    dis_parm_out.frm_cnt = frm_cnt;
    rt = dis_alg_task_get(&dis_parm_out);
    if(rt == E_OK){
        DBG_IND("dis_parm_out:subsample_sel: %d\n\r",dis_parm_out.subsample_sel);
        DBG_IND("dis_parm_out:scale_up_ratio: %d\n\r",dis_parm_out.scale_up_ratio);
        DBG_IND("dis_parm_out:out_roi_mv:(%d, %d), valid: %d\n\r",dis_parm_out.global_mv.x,dis_parm_out.global_mv.y, dis_parm_out.global_mv_valid);
        DBG_IND("dis_parm_out:frame_count: %d\n\r",dis_parm_out.frm_cnt);
    }

    *mv_valid = dis_parm_out.global_mv_valid;
    *mv_dx    = dis_parm_out.global_mv.x;
    *mv_dy    = dis_parm_out.global_mv.y;

    return rt;
}

ER dis_alg_flow_stop(UINT32 isp_id)
{
    ER rt = E_OK;

    rt = dis_alg_task_uninit();

    return rt;
}

ER dis_alg_flow_close(UINT32 isp_id)
{
    ER rt = E_OK;

    rt = dis_alg_task_close();

    return rt;
}

ER dis_alg_flow_set(UINT32 isp_id, UINT32 cfg_id, VOID *p_param)
{
    ER rt = E_OK;
	
	switch (cfg_id) {
	case _VDOPRC_DIS_PARAM_RESET:
		rt = dis_alg_task_reset();
		break;
	}

    return rt;
}

ER dis_init_module(UINT32 id_list)
{
    // assert id_ist
	id_list = 1;

	// clean DIS module
	memset(&dis_module, 0x0, sizeof(VDOPRC_DIS_PLUGIN));

	// allocate DIS PLUGIN
	#if defined(__FREERTOS)
	dis_module.open  =     NULL;
	dis_module.start =     NULL;
	dis_module.proc  =     NULL;
	dis_module.stop  =     NULL;
	dis_module.close =     NULL;
	dis_module.set   =     NULL;
	#else
	dis_module.open  =     dis_alg_flow_open;
	dis_module.start =     dis_alg_flow_start;
	dis_module.proc  =     dis_alg_flow_proc;
	dis_module.stop  =     dis_alg_flow_stop;
	dis_module.close =     dis_alg_flow_close;
	dis_module.set   =     dis_alg_flow_set;
	#endif

	// register to isf
	isf_vdoprc_reg_dis_plugin(&dis_module);

	return 0;
}

ER dis_uninit_module(void)
{

	memset(&dis_module, 0x0, sizeof(dis_module));

	// register NULL to isp
	isf_vdoprc_reg_dis_plugin(NULL);

	return E_OK;
}


