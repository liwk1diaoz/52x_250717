#ifndef _SYS_SENSOR
#define _SYS_SENSOR

#include "hdal.h"
#include <kwrap/nvt_type.h>
#include <kwrap/error_no.h>

enum {
	SENSOR_DRV_CFG,
	SENSOR_SENOUT_FMT,
	SENSOR_CAPOUT_FMT,
	SENSOR_DATA_LANE,
	SENSOR_AE_PATH,
	SENSOR_AWB_PATH,
	SENSOR_AF_PATH,
	SENSOR_IQ_PATH,
	SENSOR_IQ_SHADING_PATH,
	SENSOR_IQ_DPC_PATH,
	SENSOR_IQ_LDC_PATH,
	SENSOR_IQ_CAP_PATH,
};

ER System_GetSensorInfo(UINT32 id, UINT32 param, void *value);

#define _SEN_DRV_NAME(sen) STR(nvt_sen_##sen)
#define SEN_DRV_NAME(sen) _SEN_DRV_NAME(sen)
#define STR(s) #s

#define _SEN_INIT(sen, dtsi_info) sen_init_##sen(dtsi_info);
#define SEN_INIT(sen, dtsi_info) _SEN_INIT(sen, dtsi_info)

#define sen_init_off(dtsi_info)

extern void System_EnableSensorDet(void);
extern void System_DisableSensorDet(void);

#endif

