#include "SysSensor.h"
#include <kwrap/debug.h>
#include <rtosfdt.h>
#include <libfdt.h>
#include <compiler.h>
#include "sys_fdt.h"
#include "ImageApp/ImageApp_Common.h"
#include "PrjInc.h"
#include "vendor_videocapture.h"

/* helper to stringify macro values like _SEN1_ when logging */
#ifndef STR
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif

#if (SENSOR_INSERT_FUNCTION == ENABLE)
void DetSensor(void);
int SX_TIMER_DET_SENSOR_ID = -1;
SX_TIMER_ITEM(DetSensor, DetSensor, 25, FALSE)
#endif

static UINT32 uiSensorEnableState_fw = SENSOR_DEFAULT_DISPLAY_MASK; //fw attach sensor
static UINT32 uiSensorEnableState_prev_fw = 0; //previous fw attach sensor status

UINT32 System_GetEnableSensor(void)
{
	#if (SENSOR_INSERT_FUNCTION == ENABLE)
	return uiSensorEnableState_fw;
	#else
	return SENSOR_CAPS_MASK;
	#endif
}
UINT32 System_GetPrevEnableSensor(void)
{
	return uiSensorEnableState_prev_fw;
}

void System_EnableSensor(UINT32 SensorMask)
{
	DBG_DUMP("SensorMask = %x\r\n", SensorMask);

	uiSensorEnableState_fw |= SensorMask;
}

void System_DisableSensor(UINT32 SensorMask)
{
	uiSensorEnableState_fw &= ~SensorMask;
}

#if (SENSOR_INSERT_FUNCTION == ENABLE)


void DetSensor(void)
{

	INT iCurrMode = System_GetState(SYS_STATE_CURRMODE);

	if (iCurrMode == PRIMARY_MODE_MOVIE) {

		HD_RESULT result;
		HD_PATH_ID path_id = 0;
		BOOL bChg=0;

		uiSensorEnableState_prev_fw=uiSensorEnableState_fw;
#if (SENSOR_INSERT_MASK & SENSOR_2)
		static BOOL cp = 0;
		BOOL p;
		path_id = ImageApp_MovieMulti_GetVcapCtrlPort(_CFG_REC_ID_2);
		result = vendor_videocap_get(path_id, VENDOR_VIDEOCAP_PARAM_GET_PLUG, &p);
		if (result== HD_OK) {
			if ((cp == 0) && (p == 1)) {
				uiSensorEnableState_fw |= SENSOR_2;
				bChg=1;
			} else if ((cp == 1) && (p == 0)) {
				uiSensorEnableState_fw &= ~SENSOR_2;
				bChg=1;
			}
			cp = p;
		}else{
			DBG_ERR("s2 result = %d, path_id=0x%x\r\n", result, path_id);
		}

#endif

#if (SENSOR_INSERT_MASK & SENSOR_3)
		static BOOL cp2 = 0;
		BOOL p2;
		path_id = ImageApp_MovieMulti_GetVcapCtrlPort(_CFG_REC_ID_3);
		result = vendor_videocap_get(path_id, VENDOR_VIDEOCAP_PARAM_GET_PLUG, &p2);
		if (result== HD_OK) {
			if ((cp2 == 0) && (p2 == 1)) {
				bChg=1;
				uiSensorEnableState_fw |= SENSOR_3;
			} else if ((cp2 == 1) && (p2 == 0)) {
				uiSensorEnableState_fw &= ~SENSOR_3;
				bChg=1;
			}
			cp2 = p2;
		}else{
			DBG_ERR("s3 result = %d, path_id=0x%x\r\n", result, path_id);
		}
#endif
		if(bChg){
			Ux_PostEvent(NVTEVT_EXE_MOVIE_SENSORHOTPLUG, 0);
		}
	}

}

void System_EnableSensorDet(void)
{

 if (SX_TIMER_DET_SENSOR_ID == -1) { // ?S?}?L sensor detect timer ????A add timer
         SX_TIMER_DET_SENSOR_ID = SxTimer_AddItem(&Timer_DetSensor);
    }

    if (SxTimer_GetFuncActive(SX_TIMER_DET_SENSOR_ID) == FALSE) { // ?Y timer ?w????????A???
         SxTimer_SetFuncActive(SX_TIMER_DET_SENSOR_ID, TRUE);
    }

}

void System_DisableSensorDet(void)
{

	if (SxTimer_GetFuncActive(SX_TIMER_DET_SENSOR_ID) == TRUE) { // ?Y timer ?w??????A????
         SxTimer_SetFuncActive(SX_TIMER_DET_SENSOR_ID, FALSE);
    }

}
#endif
#include <stdio.h>
ER System_GetSensorInfo(UINT32 id, UINT32 param, void *value)
{
	const void *p_fdt_sensor = fdt_get_sensor();
	const void *p_fdt_app = fdt_get_app();
	const void *pfdt_node;
	INT32 node_ofst, data_len;
	char node_path[32];
	UINT32 *p_data;
	char *p_str;
	UINT32 i;
	ER ret = E_SYS;

	switch (param) {
	case SENSOR_DRV_CFG: {
			if (value != 0) {
				HD_VIDEOCAP_DRV_CONFIG *pcfg = (HD_VIDEOCAP_DRV_CONFIG *)value;
				// driver_name
				printf("Liwk>>>>>>>>>_SEN1_ = %s\r\n", STR(_SEN1_));
				//snprintf(pcfg->sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, ((id == 0) ? SEN_DRV_NAME(_SEN1_) : SEN_DRV_NAME(_SEN2_)));
				if(id == 0){
					snprintf(pcfg->sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, SEN_DRV_NAME(_SEN1_));
				}else if(id == 1){
					snprintf(pcfg->sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, SEN_DRV_NAME(_SEN2_));
				}else if(id == 2){
					snprintf(pcfg->sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, SEN_DRV_NAME(_SEN3_));
				}else if(id == 3){
					snprintf(pcfg->sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, SEN_DRV_NAME(_SEN4_));
				}else{
					DBG_ERR("SENSOR_DRV_CFG Error id\r\n", id);
				}
				snprintf(node_path, sizeof(node_path)-1, "/sensor_ssenif/ssenif@%d", id);
				//if ((node_ofst = fdt_path_offset(p_fdt, node_path)) > 0) {
				if (p_fdt_sensor && ((node_ofst = fdt_path_offset(p_fdt_sensor, node_path)) > 0)) {
					// if_type
					pfdt_node = fdt_getprop(p_fdt_sensor, node_ofst, "if_type", (int *)&data_len);
					if ((p_data = (UINT32 *)pfdt_node) != NULL) {
						pcfg->sen_cfg.sen_dev.if_type = (UINT32)be32_to_cpu(p_data[0]);
					}
					// sensor_pinmux
					pfdt_node = fdt_getprop(p_fdt_sensor, node_ofst, "sensor_pinmux", (int *)&data_len);
					if ((p_data = (UINT32 *)pfdt_node) != NULL) {
						pcfg->sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux = (UINT32)be32_to_cpu(p_data[0]);
					}
					// serial_if_pinmux
					pfdt_node = fdt_getprop(p_fdt_sensor, node_ofst, "serial_if_pinmux", (int *)&data_len);
					if ((p_data = (UINT32 *)pfdt_node) != NULL) {
						pcfg->sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = (UINT32)be32_to_cpu(p_data[0]);
					}
					// cmd_if_pinmux
					pfdt_node = fdt_getprop(p_fdt_sensor, node_ofst, "cmd_if_pinmux", (int *)&data_len);
					if ((p_data = (UINT32 *)pfdt_node) != NULL) {
						pcfg->sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = (UINT32)be32_to_cpu(p_data[0]);
					}
					// clk_lane_sel
					pfdt_node = fdt_getprop(p_fdt_sensor, node_ofst, "clk_lane_sel", (int *)&data_len);
					if ((p_data = (UINT32 *)pfdt_node) != NULL) {
						pcfg->sen_cfg.sen_dev.pin_cfg.clk_lane_sel = (UINT32)be32_to_cpu(p_data[0]);
					}
					// sen_2_serial_pin_map
					pfdt_node = fdt_getprop(p_fdt_sensor, node_ofst, "sen_2_serial_pin_map", (int *)&data_len);
					if ((p_data = (UINT32 *)pfdt_node) != NULL) {
						for (i = 0; i < 8; i++) {
							pcfg->sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[i] = (UINT32)be32_to_cpu(p_data[i]);
						}
					}
					// shdrmap
					UINT32 shdr_info[2] = {0};
					pfdt_node = fdt_getprop(p_fdt_sensor, node_ofst, "shdr_map", (int *)&data_len);
					if ((p_data = (UINT32 *)pfdt_node) != NULL) {
						for (i = 0; i < 2; i++) {
							shdr_info[i] = (UINT32)be32_to_cpu(p_data[i]);
						}
					}
					pcfg->sen_cfg.shdr_map = HD_VIDEOCAP_SHDR_MAP(shdr_info[0], shdr_info[1]);
					// ccir_vd_hd_pin (forced set to 1)
					pcfg->sen_cfg.sen_dev.pin_cfg.ccir_vd_hd_pin = 1;
					
					DBG_DUMP("driver_name = %s\r\n", pcfg->sen_cfg.sen_dev.driver_name);
					DBG_DUMP("if_type = %x\r\n", pcfg->sen_cfg.sen_dev.if_type);
					DBG_DUMP("sensor_pinmux = %x\r\n", pcfg->sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux);
					DBG_DUMP("serial_if_pinmux = %x\r\n", pcfg->sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux);
					DBG_DUMP("cmd_if_pinmux = %x\r\n", pcfg->sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux);
					DBG_DUMP("clk_lane_sel = %x\r\n", pcfg->sen_cfg.sen_dev.pin_cfg.clk_lane_sel);
					DBG_DUMP("sen_2_serial_pin_map =");
					for (i = 0; i < 8; i++) {
						DBG_DUMP("%08x ", pcfg->sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[i]);
					}
					DBG_DUMP("\r\n");
					
					ret = E_OK;
				}
			}
			break;
		}

	case SENSOR_SENOUT_FMT: {
			if (value != 0) {
				HD_VIDEO_PXLFMT *pfmt = (HD_VIDEO_PXLFMT *)value;
				snprintf(node_path, sizeof(node_path)-1, "/sensor_ssenif/ssenif@%d", id);
				//if ((node_ofst = fdt_path_offset(p_fdt, node_path)) > 0) {
				if (p_fdt_sensor && ((node_ofst = fdt_path_offset(p_fdt_sensor, node_path)) > 0)) {
					// senout_pxlfmt
					pfdt_node = fdt_getprop(p_fdt_sensor, node_ofst, "senout_pxlfmt", (int *)&data_len);
					if ((p_data = (UINT32 *)pfdt_node) != NULL) {
						*pfmt = (UINT32)be32_to_cpu(p_data[0]);
					}
					//DBG_DUMP("senout_pxlfmt = %x\r\n", *pfmt);
					ret = E_OK;
				}
			}
			break;
		}

	case SENSOR_CAPOUT_FMT: {
			if (value != 0) {
				HD_VIDEO_PXLFMT *pfmt = (HD_VIDEO_PXLFMT *)value;
				snprintf(node_path, sizeof(node_path)-1, "/sensor_ssenif/ssenif@%d", id);
				//if ((node_ofst = fdt_path_offset(p_fdt, node_path)) > 0) {
				if (p_fdt_sensor && ((node_ofst = fdt_path_offset(p_fdt_sensor, node_path)) > 0)) {
					// senout_pxlfmt
					pfdt_node = fdt_getprop(p_fdt_sensor, node_ofst, "capout_pxlfmt", (int *)&data_len);
					if ((p_data = (UINT32 *)pfdt_node) != NULL) {
						*pfmt = (UINT32)be32_to_cpu(p_data[0]);
					}
					//DBG_DUMP("capout_pxlfmt = %x\r\n", *pfmt);
					ret = E_OK;
				}
			}
			break;
		}

	case SENSOR_DATA_LANE: {
			if (value != 0) {
				UINT32 *pdata_lane = (UINT32 *)value;
				snprintf(node_path, sizeof(node_path)-1, "/sensor_ssenif/ssenif@%d", id);
				//if ((node_ofst = fdt_path_offset(p_fdt, node_path)) > 0) {
				if (p_fdt_sensor && ((node_ofst = fdt_path_offset(p_fdt_sensor, node_path)) > 0)) {
					// data_lane
					pfdt_node = fdt_getprop(p_fdt_sensor, node_ofst, "data_lane", (int *)&data_len);
					if ((p_data = (UINT32 *)pfdt_node) != NULL) {
						*pdata_lane = (UINT32)be32_to_cpu(p_data[0]);
					}
					//DBG_DUMP("data_lane = %x\r\n", *pdata_lane);
					ret = E_OK;
				}
			}
			break;
		}

	case SENSOR_AE_PATH: {
			if (value != 0) {
				SENSOR_PATH_INFO *ppath = (SENSOR_PATH_INFO *)value;
				ppath->path[0] = 0;
				snprintf(node_path, sizeof(node_path)-1, "/isp/sensor@%d", id);
				//if ((node_ofst = fdt_path_offset(p_fdt, node_path)) > 0) {
				if (p_fdt_app && ((node_ofst = fdt_path_offset(p_fdt_app, node_path)) > 0)) {
					// ae_path
					pfdt_node = fdt_getprop(p_fdt_app, node_ofst, "ae_path", (int *)&data_len);
					p_str = (char *)pfdt_node;
					if (p_str != NULL) {
						ppath->id = id;
						strncpy(ppath->path, p_str, 31);
						ppath->addr = (UINT32)p_fdt_app;
						//DBG_DUMP("ae_path %d=%s, app_addr =%x\r\n", ppath->id, ppath->path, ppath->addr);
						ret = E_OK;
					}
				}
			}
			break;
		}

	case SENSOR_AWB_PATH: {
			if (value != 0) {
				SENSOR_PATH_INFO *ppath = (SENSOR_PATH_INFO *)value;
				ppath->path[0] = 0;
				snprintf(node_path, sizeof(node_path)-1, "/isp/sensor@%d", id);
				//if ((node_ofst = fdt_path_offset(p_fdt, node_path)) > 0) {
				if (p_fdt_app && ((node_ofst = fdt_path_offset(p_fdt_app, node_path)) > 0)) {
					// awb_path
					pfdt_node = fdt_getprop(p_fdt_app, node_ofst, "awb_path", (int *)&data_len);
					p_str = (char *)pfdt_node;
					if (p_str != NULL) {
						ppath->id = id;
						strncpy(ppath->path, p_str, 31);
						ppath->addr = (UINT32)p_fdt_app;
						//DBG_DUMP("awb_path %d=%s, app_addr =%x\r\n", ppath->id, ppath->path, ppath->addr);
						ret = E_OK;
					}
				}
			}
			break;
		}

	case SENSOR_AF_PATH: {
			if (value != 0) {
				SENSOR_PATH_INFO *ppath = (SENSOR_PATH_INFO *)value;
				ppath->path[0] = 0;
				snprintf(node_path, sizeof(node_path)-1, "/isp/sensor@%d", id);
				//if ((node_ofst = fdt_path_offset(p_fdt, node_path)) > 0) {
				if (p_fdt_app && ((node_ofst = fdt_path_offset(p_fdt_app, node_path)) > 0)) {
					// af_path
					pfdt_node = fdt_getprop(p_fdt_app, node_ofst, "af_path", (int *)&data_len);
					p_str = (char *)pfdt_node;
					if (p_str != NULL) {
						ppath->id = id;
						strncpy(ppath->path, p_str, 31);
						ppath->addr = (UINT32)p_fdt_app;
						//DBG_DUMP("af_path %d=%s, app_addr =%x\r\n", ppath->id, ppath->path, ppath->addr);
						ret = E_OK;
					}
				}
			}
			break;
		}

	case SENSOR_IQ_PATH: {
			if (value != 0) {
				SENSOR_PATH_INFO *ppath = (SENSOR_PATH_INFO *)value;
				ppath->path[0] = 0;
				snprintf(node_path, sizeof(node_path)-1, "/isp/sensor@%d", id);
				//if ((node_ofst = fdt_path_offset(p_fdt, node_path)) > 0) {
				if (p_fdt_app && ((node_ofst = fdt_path_offset(p_fdt_app, node_path)) > 0)) {
					// iq_path
					if(SysGetFlag(FL_MOVIE_HDR) == MOVIE_HDR_ON){
						DBG_DUMP("iq cap use HDR mode !!!!!");
						pfdt_node = fdt_getprop(p_fdt_app, node_ofst, "iq_hdr_path", (int *)&data_len);
					}else{
						DBG_DUMP("iq cap use CTL_SEN_MODE_LINEAR !!!!!");
						pfdt_node = fdt_getprop(p_fdt_app, node_ofst, "iq_path", (int *)&data_len);
					}
					p_str = (char *)pfdt_node;
					if (p_str != NULL) {
						ppath->id = id;
						strncpy(ppath->path, p_str, 31);
						ppath->addr = (UINT32)p_fdt_app;
						//DBG_DUMP("iq_path %d=%s, app_addr =%x\r\n", ppath->id, ppath->path, ppath->addr);
						ret = E_OK;
					}
				}
			}
			break;
		}

	case SENSOR_IQ_SHADING_PATH: {
			if (value != 0) {
				SENSOR_PATH_INFO *ppath = (SENSOR_PATH_INFO *)value;
				ppath->path[0] = 0;
				snprintf(node_path, sizeof(node_path)-1, "/isp/sensor@%d", id);
				//if ((node_ofst = fdt_path_offset(p_fdt, node_path)) > 0) {
				if (p_fdt_app && ((node_ofst = fdt_path_offset(p_fdt_app, node_path)) > 0)) {
					// iq_shading_path
					pfdt_node = fdt_getprop(p_fdt_app, node_ofst, "iq_shading_path", (int *)&data_len);
					p_str = (char *)pfdt_node;
					if (p_str != NULL) {
						ppath->id = id;
						strncpy(ppath->path, p_str, 31);
						ppath->addr = (UINT32)p_fdt_app;
						//DBG_DUMP("iq_shading_path %d=%s, app_addr =%x\r\n", ppath->id, ppath->path, ppath->addr);
						ret = E_OK;
					}
				}
			}
			break;
		}

	case SENSOR_IQ_DPC_PATH: {
			if (value != 0) {
				SENSOR_PATH_INFO *ppath = (SENSOR_PATH_INFO *)value;
				ppath->path[0] = 0;
				snprintf(node_path, sizeof(node_path)-1, "/isp/sensor@%d", id);
				//if ((node_ofst = fdt_path_offset(p_fdt, node_path)) > 0) {
				if (p_fdt_app && ((node_ofst = fdt_path_offset(p_fdt_app, node_path)) > 0)) {
					// iq_dpc_path
					pfdt_node = fdt_getprop(p_fdt_app, node_ofst, "iq_dpc_path", (int *)&data_len);
					p_str = (char *)pfdt_node;
					if (p_str != NULL) {
						ppath->id = id;
						strncpy(ppath->path, p_str, 31);
						ppath->addr = (UINT32)p_fdt_app;
						//DBG_DUMP("iq_dpc_path %d=%s, app_addr =%x\r\n", ppath->id, ppath->path, ppath->addr);
						ret = E_OK;
					}
				}
			}
			break;
		}

	case SENSOR_IQ_LDC_PATH: {
			if (value != 0) {
				SENSOR_PATH_INFO *ppath = (SENSOR_PATH_INFO *)value;
				ppath->path[0] = 0;
				snprintf(node_path, sizeof(node_path)-1, "/isp/sensor@%d", id);
				//if ((node_ofst = fdt_path_offset(p_fdt, node_path)) > 0) {
				if (p_fdt_app && ((node_ofst = fdt_path_offset(p_fdt_app, node_path)) > 0)) {
					// iq_ldc_path
					pfdt_node = fdt_getprop(p_fdt_app, node_ofst, "iq_ldc_path", (int *)&data_len);
					p_str = (char *)pfdt_node;
					if (p_str != NULL) {
						ppath->id = id;
						strncpy(ppath->path, p_str, 31);
						ppath->addr = (UINT32)p_fdt_app;
						//DBG_DUMP("iq_ldc_path %d=%s, app_addr =%x\r\n", ppath->id, ppath->path, ppath->addr);
						ret = E_OK;
					}
				}
			}
			break;
		}

	case SENSOR_IQ_CAP_PATH: {
			if (value != 0) {
				SENSOR_PATH_INFO *ppath = (SENSOR_PATH_INFO *)value;
				ppath->path[0] = 0;
				snprintf(node_path, sizeof(node_path)-1, "/isp/sensor@%d", id);
				//if ((node_ofst = fdt_path_offset(p_fdt, node_path)) > 0) {
				if (p_fdt_app && ((node_ofst = fdt_path_offset(p_fdt_app, node_path)) > 0)) {
					// iq_path
					pfdt_node = fdt_getprop(p_fdt_app, node_ofst, "iq_cap_path", (int *)&data_len);

					p_str = (char *)pfdt_node;
					if (p_str != NULL) {
						ppath->id = id;
						strncpy(ppath->path, p_str, 31);
						ppath->addr = (UINT32)p_fdt_app;
						//DBG_DUMP("iq_path %d=%s, app_addr =%x\r\n", ppath->id, ppath->path, ppath->addr);
						ret = E_OK;
					}
				}
			}
			break;
		}
	}

	return ret;
}

