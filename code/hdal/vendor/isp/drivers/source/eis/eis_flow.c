#include "eis_int.h"


#if 0
//###### dummy function ###############
#if 1
BOOL eis_rsc_open(EIS_RSC_OPEN_CFG *open_cfg)
{
	return TRUE;
}
BOOL eis_rsc_close(void)
{
	return TRUE;
}

BOOL eis_rsc_process(EIS_RSC_PROC_INFO *proc_info)
{
	return TRUE;
}

BOOL eis_rsc_set(EIS_RSC_PARAM_ID param_id, VOID *p_param)
{
	return TRUE;
}

BOOL eis_rsc_get(EIS_RSC_PARAM_ID param_id, VOID *p_param)
{
	return TRUE;
}

#endif
#endif
//=============================================================================
// global
//=============================================================================
static VDOPRC_EIS_PLUGIN eis_module = {0};

#if 0
//=============================================================================
// function declaration
//=============================================================================

static void eis_open_cfg_mapping(EIS_RSC_OPEN_CFG *p_eis_open_cfg, VDOPRC_EIS_OPEN_CFG *p_vprc_eis_open_cfg)
{
	p_eis_open_cfg->cam_intrins.distor_center = p_vprc_eis_open_cfg->cam_intrins.distor_center;
	memcpy((void *)p_eis_open_cfg->cam_intrins.distor_curve, (void *)p_vprc_eis_open_cfg->cam_intrins.distor_curve, sizeof(p_eis_open_cfg->cam_intrins.distor_curve));
	memcpy((void *)p_eis_open_cfg->cam_intrins.undistor_curve, (void *)p_vprc_eis_open_cfg->cam_intrins.undistor_curve, sizeof(p_eis_open_cfg->cam_intrins.undistor_curve));
	p_eis_open_cfg->cam_intrins.focal_length = p_vprc_eis_open_cfg->cam_intrins.focal_length;
	p_eis_open_cfg->cam_intrins.calib_img_size = p_vprc_eis_open_cfg->cam_intrins.calib_img_size;
	p_eis_open_cfg->roll_shut_readout_time = p_vprc_eis_open_cfg->roll_shut_readout_time;
	memcpy((void *)p_eis_open_cfg->gyro.axes_mapping, (void *)p_vprc_eis_open_cfg->gyro.axes_mapping, sizeof(p_eis_open_cfg->gyro.axes_mapping));
	p_eis_open_cfg->gyro.sampling_rate = p_vprc_eis_open_cfg->gyro.sampling_rate;
	p_eis_open_cfg->gyro.unit_conv = p_vprc_eis_open_cfg->gyro.unit_conv;
	memcpy((void *)p_eis_open_cfg->accel.axes_mapping, (void *)p_vprc_eis_open_cfg->accel.axes_mapping, sizeof(p_eis_open_cfg->accel.axes_mapping));
	p_eis_open_cfg->accel.sampling_rate = p_vprc_eis_open_cfg->accel.sampling_rate;
	p_eis_open_cfg->accel.unit_conv = p_vprc_eis_open_cfg->accel.unit_conv;
	switch(p_vprc_eis_open_cfg->imu_type) {
	case VDOPRC_GYROSCOPE:
		p_eis_open_cfg->imu_type = GYROSCOPE;
		break;
	case VDOPRC_GYROSCOPE_ACCELEROMETER:
		p_eis_open_cfg->imu_type = GYROSCOPE_ACCELEROMETER;
		break;
	default:
		DBG_ERR("mapping error(%d)\r\n", p_vprc_eis_open_cfg->imu_type);
		break;
	}
}


static ER eis_flow_open(VDOPRC_EIS_OPEN_CFG *p_vprc_eis_open_cfg)
{
    ER rt = E_OK;
    EIS_RSC_OPEN_CFG open_cfg = {0};

    //open dis task
    rt = _eis_tsk_open();
    if (rt != E_OK) {
    	return rt;
    }

	eis_open_cfg_mapping(&open_cfg, p_vprc_eis_open_cfg);

    if (FALSE == eis_rsc_open(&open_cfg)) {
    	rt = E_SYS;
    	DBG_ERR("eis_rsc_open failed\r\n");
    }
    return rt;
}


#endif
static ER eis_flow_set(VDOPRC_EIS_PARAM_ID param_id, void *p_param)
{
    ER rt = E_OK;

	if (p_param == NULL) {
		return E_PAR;
	}

	switch(param_id) {
	case VDOPRC_EIS_PARAM_TRIG_PROC: {
		_eis_tsk_trigger_proc((VDOPRC_EIS_PROC_INFO *)p_param, FALSE);
	}
		break;
	case VDOPRC_EIS_PARAM_TRIG_PROC_IN_ISR: {
		_eis_tsk_trigger_proc((VDOPRC_EIS_PROC_INFO *)p_param, TRUE);
	}
		break;
	case VDOPRC_EIS_PARAM_GYRO_LATENCY: {
		_eis_tsk_gyro_latency(*(UINT32 *)p_param);
	}
		break;
	default:
		rt = E_PAR;
		break;
	}

    return rt;
}

static ER eis_flow_get(VDOPRC_EIS_PARAM_ID param_id, void *p_param)
{
    ER rt = E_OK;

	if (p_param == NULL) {
		return E_PAR;
	}

	switch(param_id) {
	case VDOPRC_EIS_PARAM_2DLUT: {
		rt = _eis_tsk_get_output((VDOPRC_EIS_2DLUT *)p_param);
	}
		break;
	case VDOPRC_EIS_PARAM_PATH_INFO: {
		rt = _eis_tsk_get_path_info((VDOPRC_EIS_PATH_INFO *)p_param);
	}
		break;
	default:
		rt = E_PAR;
		break;
	}

    return rt;
}
ER eis_init_module(void)
{
	DBG_IND("\r\n");
	// clean DIS module
	memset(&eis_module, 0x0, sizeof(VDOPRC_EIS_PLUGIN));

	// allocate DIS PLUGIN
	eis_module.set  =     eis_flow_set;
	eis_module.get  =     eis_flow_get;

	// register to isf
	isf_vdoprc_reg_eis_plugin(&eis_module);

	return 0;
}

ER eis_uninit_module(void)
{
	DBG_IND("\r\n");
	memset(&eis_module, 0x0, sizeof(eis_module));

	// register NULL to isp
	isf_vdoprc_reg_eis_plugin(NULL);

	return E_OK;
}


