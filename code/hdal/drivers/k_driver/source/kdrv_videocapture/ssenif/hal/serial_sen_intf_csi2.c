/*
    Serial Sensor Interface DAL library

    Exported Serial Sensor Interface DAL library.

    @file       dal_ssenif.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "ssenif_int.h"

#ifndef __KERNEL__
#define __MODULE__    rtos_ssenif
#include <kwrap/debug.h>
extern unsigned int rtos_ssenif_debug_level;
#else
#include <kdrv_builtin/kdrv_builtin.h>
#endif
#include "kwrap/util.h"
#include <kwrap/task.h>

#define MODULE_ID               CSI_ID_CSI2
#define DALSSENIFID             DAL_SSENIF_ID_CSI2
#define DALCSI_API_DEFINE(name) ssenif_##name##_csi2



static PCSIOBJ                  gcsi_obj;
static DAL_SSENIF_SENSORTYPE    sensor_type = DAL_SSENIF_SENSORTYPE_SONY_NONEHDR;
static UINT32                   gsensor_hsclk = 1000000;

#ifdef __KERNEL__
//void ssenif_hook_csi(void *object)
void DALCSI_API_DEFINE(hook)(void *object)
{
	gcsi_obj = (PCSIOBJ)object;
}
EXPORT_SYMBOL(DALCSI_API_DEFINE(hook));
#endif

//ER ssenif_init_csi(void)
ER DALCSI_API_DEFINE(init)(void)
{
	#ifdef __KERNEL__
	if (gcsi_obj == NULL) {
		DBG_ERR("driver not loaded\r\n");
		return E_OACV;
	}
	#else
	gcsi_obj = csi_get_drv_object(MODULE_ID);
	#endif

	ssenif_install_cmd();
	return E_OK;
}
static int fast_boot = 0;
#if defined (__KERNEL__)
static int first_open = 1;
#endif
//ER ssenif_open_csi(void)
ER DALCSI_API_DEFINE(open)(void)
{
	ER ret;

	if (gcsi_obj == NULL) {
		DBG_ERR("no init\r\n");
		return E_OACV;
	}
#if defined (__KERNEL__)
	if ((kdrv_builtin_is_fastboot()) && (first_open)){
		first_open = 0;
		fast_boot = 1;
	}
#endif

	ret = gcsi_obj->open();

	if (fast_boot == 0) {

		gcsi_obj->set_config(CSI_CONFIG_ID_HD_GEN_METHOD,   CSI_LINESYNC_PKTHEAD);

		gcsi_obj->set_config(CSI_CONFIG_ID_VIRTUAL_ID,      0);
		gcsi_obj->set_config(CSI_CONFIG_ID_VIRTUAL_ID2,         15);
		gcsi_obj->set_config(CSI_CONFIG_ID_VIRTUAL_ID3,         15);
		gcsi_obj->set_config(CSI_CONFIG_ID_VIRTUAL_ID4,         15);
	}
	gcsi_obj->set_config(CSI_CONFIG_ID_TIMEOUT_PERIOD,   1000);

	if (fast_boot == 0) {
		senphy_set_config(SENPHY_CONFIG_ID_IADJ, 0x0);
	}

	return ret;
}

//BOOL ssenif_is_opened_csi(void)
BOOL DALCSI_API_DEFINE(is_opened)(void)
{
	if (gcsi_obj == NULL) {
		DBG_ERR("no init\r\n");
		return E_OACV;
	}

	return gcsi_obj->is_opened();
}

//ER ssenif_close_csi(void)
ER DALCSI_API_DEFINE(close)(void)
{
	if (gcsi_obj == NULL) {
		DBG_ERR("no init\r\n");
		return E_OACV;
	}

	fast_boot = 0;

	return gcsi_obj->close();
}

//ER ssenif_start_csi(void)
ER DALCSI_API_DEFINE(start)(void)
{
	if (gcsi_obj == NULL) {
		DBG_ERR("no init\r\n");
		return E_OACV;
	}

	ssenif_lastest_start = DALSSENIFID;

	if (gcsi_obj->get_enable()) {
		return E_OK;
	} else {
		return gcsi_obj->set_enable(ENABLE);
	}
}

//ER ssenif_stop_csi(void)
ER DALCSI_API_DEFINE(stop)(void)
{
	if (gcsi_obj == NULL) {
		DBG_ERR("no init\r\n");
		return E_OACV;
	}

	if (gcsi_obj->get_enable()) {

		gcsi_obj->set_enable(DISABLE);
		if (gcsi_obj->get_config(CSI_CONFIG_ID_DIS_METHOD) == FALSE) {
			if (gcsi_obj->get_config(CSI_CONFIG_ID_DIS_SRC) == CSI_DIS_SRC_FRAME_END2) {
				gcsi_obj->wait_interrupt(CSI_INTERRUPT_FRAME_END2);
			} else if (gcsi_obj->get_config(CSI_CONFIG_ID_DIS_SRC) == CSI_DIS_SRC_FRAME_END3) {
				gcsi_obj->wait_interrupt(CSI_INTERRUPT_FRAME_END3);
			} else if (gcsi_obj->get_config(CSI_CONFIG_ID_DIS_SRC) == CSI_DIS_SRC_FRAME_END4) {
				gcsi_obj->wait_interrupt(CSI_INTERRUPT_FRAME_END4);
			} else {
				gcsi_obj->wait_interrupt(CSI_INTERRUPT_FRAME_END);
			}
		}
	}
	if (gcsi_obj->get_enable()) {
		DBG_DUMP("csi stop, but get enable is 0, delay 100ms\n");
		vos_task_delay_ms(100);
	}
	return E_OK;
}

//DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_csi(DAL_SSENIF_INTERRUPT waited_flag)
DAL_SSENIF_INTERRUPT DALCSI_API_DEFINE(wait_interrupt)(DAL_SSENIF_INTERRUPT waited_flag)
{
	CSI_INTERRUPT           _waited_flag = 0;
	DAL_SSENIF_INTERRUPT    ret = 0;

	if (gcsi_obj == NULL) {
		DBG_ERR("no init\r\n");
		return E_OACV;
	}

	if (waited_flag & (DAL_SSENIF_INTERRUPT_VD | DAL_SSENIF_INTERRUPT_VD2 | DAL_SSENIF_INTERRUPT_VD3 | DAL_SSENIF_INTERRUPT_VD4
					|DAL_SSENIF_INTERRUPT_SIE2_VD|DAL_SSENIF_INTERRUPT_SIE4_VD|DAL_SSENIF_INTERRUPT_SIE5_VD)) {
		_waited_flag |= CSI_INTERRUPT_FS_GOT;
	}
	if ((waited_flag & DAL_SSENIF_INTERRUPT_FRAMEEND)||(waited_flag & DAL_SSENIF_INTERRUPT_SIE2_FRAMEEND)) {
		_waited_flag |= CSI_INTERRUPT_FRAME_END;
	}
	if ((waited_flag & DAL_SSENIF_INTERRUPT_FRAMEEND2)||(waited_flag & DAL_SSENIF_INTERRUPT_SIE4_FRAMEEND)) {
		_waited_flag |= CSI_INTERRUPT_FRAME_END2;
	}
	if ((waited_flag & DAL_SSENIF_INTERRUPT_FRAMEEND3)||(waited_flag & DAL_SSENIF_INTERRUPT_SIE5_FRAMEEND)) {
		_waited_flag |= CSI_INTERRUPT_FRAME_END3;
	}
	if (waited_flag & DAL_SSENIF_INTERRUPT_FRAMEEND4) {
		_waited_flag |= CSI_INTERRUPT_FRAME_END4;
	}

	_waited_flag = gcsi_obj->wait_interrupt(_waited_flag);

	if (_waited_flag & CSI_INTERRUPT_FS_GOT) {
		ret |= (DAL_SSENIF_INTERRUPT_VD | DAL_SSENIF_INTERRUPT_VD2 | DAL_SSENIF_INTERRUPT_VD3 | DAL_SSENIF_INTERRUPT_VD4);
	}
	if (_waited_flag & CSI_INTERRUPT_FRAME_END) {
		ret |= (DAL_SSENIF_INTERRUPT_FRAMEEND|DAL_SSENIF_INTERRUPT_SIE2_FRAMEEND);
	}
	if (_waited_flag & CSI_INTERRUPT_FRAME_END2) {
		ret |= (DAL_SSENIF_INTERRUPT_FRAMEEND2|DAL_SSENIF_INTERRUPT_SIE4_FRAMEEND);
	}
	if (_waited_flag & CSI_INTERRUPT_FRAME_END3) {
		ret |= (DAL_SSENIF_INTERRUPT_FRAMEEND3|DAL_SSENIF_INTERRUPT_SIE5_FRAMEEND);
	}
	if (_waited_flag & CSI_INTERRUPT_FRAME_END4) {
		ret |= DAL_SSENIF_INTERRUPT_FRAMEEND4;
	}
	if (_waited_flag & CSI_INTERRUPT_ABORT) {
		ret |= DAL_SSENIF_INTERRUPT_ABORT;
	}

	return ret;

}

//void ssenif_dump_debug_information_csi(void)
void DALCSI_API_DEFINE(dump_debug_information)(void)
{
	CHAR sensor_type[7][5] = {"SONY", "SDOL", "SVC", "OMNI", "ONSM", "PANA", "OTHR"};
	int get_type;

	if (ssenif_list_name_dump) {
		DBG_DUMP("\r\n     EN SENSOR  t-mclk  r-mclk  dl-no  height  cksw  timeout  stmd  siex  sie2  sie4  sie5  D0  D1  D2  D3\r\n");
	}

	if (gcsi_obj != NULL) {
		get_type = DALCSI_API_DEFINE(get_config)(DAL_SSENIFCSI_CFGID_SENSORTYPE);
		if (get_type > DAL_SSENIF_SENSORTYPE_OTHERS) {
			get_type = DAL_SSENIF_SENSORTYPE_OTHERS;
		}

		DBG_DUMP("CSI1  %d  %s     %02d      %02d      %d     %04d     %d    %04d      %d    %02d    %02d    %02d    %02d    %d   %d   %d   %d\r\n",
				 gcsi_obj->get_enable(),
				 sensor_type[get_type],
				 (int)(DALCSI_API_DEFINE(get_config)(DAL_SSENIFCSI_CFGID_SENSOR_TARGET_MCLK) / 1000000),
				 (int)(DALCSI_API_DEFINE(get_config)(DAL_SSENIFCSI_CFGID_SENSOR_REAL_MCLK) / 1000000),
				 (int)DALCSI_API_DEFINE(get_config)(DAL_SSENIFCSI_CFGID_DLANE_NUMBER),
				 (int)DALCSI_API_DEFINE(get_config)(DAL_SSENIFCSI_CFGID_VALID_HEIGHT),
				 (int)DALCSI_API_DEFINE(get_config)(DAL_SSENIFCSI_CFGID_CLANE_SWITCH),
				 (int)DALCSI_API_DEFINE(get_config)(DAL_SSENIFCSI_CFGID_TIMEOUT_PERIOD),
				 (int)DALCSI_API_DEFINE(get_config)(DAL_SSENIFCSI_CFGID_STOP_METHOD),
				 99,
				 (int)DALCSI_API_DEFINE(get_config)(DAL_SSENIFCSI_CFGID_IMGID_TO_SIE2),
				 (int)DALCSI_API_DEFINE(get_config)(DAL_SSENIFCSI_CFGID_IMGID_TO_SIE4),
				 (int)DALCSI_API_DEFINE(get_config)(DAL_SSENIFCSI_CFGID_IMGID_TO_SIE5),
				 (int)gcsi_obj->get_config(CSI_CONFIG_ID_DATALANE0_PIN),
				 (int)gcsi_obj->get_config(CSI_CONFIG_ID_DATALANE1_PIN),
				 (int)gcsi_obj->get_config(CSI_CONFIG_ID_DATALANE2_PIN),
				 (int)gcsi_obj->get_config(CSI_CONFIG_ID_DATALANE3_PIN)
				);
	}

}

//void ssenif_set_config_csi(DAL_SSENIFCSI_CFGID config_id, UINT32 value)
void DALCSI_API_DEFINE(set_config)(DAL_SSENIFCSI_CFGID config_id, UINT32 value)
{
	CSI_CONFIG_ID   configuration;
	UINT32          i;

	if (gcsi_obj == NULL) {
		DBG_ERR("no init\r\n");
		return;
	}

	switch (config_id) {
	case DAL_SSENIFCSI_CFGID_DLANE_NUMBER:
		configuration = CSI_CONFIG_ID_DLANE_NUMBER;
		if (value == 1) {
			value = CSI_DATLANE_NUM_1;
		} else if (value == 2) {
			value = CSI_DATLANE_NUM_2;
		} else if (value == 4) {
			value = CSI_DATLANE_NUM_4;
		} else {
			DBG_ERR("Err parameter %d\r\n", (int)(value));
			return;
		}
		break;
	case DAL_SSENIFCSI_CFGID_SENSORTYPE:
		sensor_type = value;
		if (sensor_type == DAL_SSENIF_SENSORTYPE_SONY_LI_DOL) {
			gcsi_obj->set_config(CSI_CONFIG_ID_LI_CHID_BIT0,     0);
			gcsi_obj->set_config(CSI_CONFIG_ID_LI_CHID_BIT1,     1);
			gcsi_obj->set_config(CSI_CONFIG_ID_LI_CHID_BIT2,     2);
			gcsi_obj->set_config(CSI_CONFIG_ID_LI_CHID_BIT3,     3);
			gcsi_obj->set_config(CSI_CONFIG_ID_VD_ISSUE_TWOSIE,  TRUE);
			gcsi_obj->set_config(CSI_CONFIG_ID_CHID_MODE,       CSI_CHID_SONYLI);
		} else {
			gcsi_obj->set_config(CSI_CONFIG_ID_VD_ISSUE_TWOSIE,  FALSE);
			gcsi_obj->set_config(CSI_CONFIG_ID_CHID_MODE,       CSI_CHID_MIPIDI);
		}
		return;
	case DAL_SSENIFCSI_CFGID_VALID_HEIGHT:
		gcsi_obj->set_config(CSI_CONFIG_ID_VALID_HEIGHT,        value);
		gcsi_obj->set_config(CSI_CONFIG_ID_VALID_HEIGHT2,   value);
		gcsi_obj->set_config(CSI_CONFIG_ID_VALID_HEIGHT3,   value);
		gcsi_obj->set_config(CSI_CONFIG_ID_VALID_HEIGHT4,   value);
		return;
	case DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE2:
		gcsi_obj->set_config(CSI_CONFIG_ID_VALID_HEIGHT,    value);
		return;
	case DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE4:
		gcsi_obj->set_config(CSI_CONFIG_ID_VALID_HEIGHT2,    value);
		return;
	case DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE5:
		gcsi_obj->set_config(CSI_CONFIG_ID_VALID_HEIGHT3,    value);
		return;
	case DAL_SSENIFCSI_CFGID_DATALANE0_PIN:
	case DAL_SSENIFCSI_CFGID_DATALANE1_PIN:
	case DAL_SSENIFCSI_CFGID_DATALANE2_PIN:
	case DAL_SSENIFCSI_CFGID_DATALANE3_PIN: {
			configuration = CSI_CONFIG_ID_DATALANE0_PIN + (config_id - DAL_SSENIFCSI_CFGID_DATALANE0_PIN);

			for (i = 0; i < 8; i++) {
				if (value & (DAL_SSENIF_LANESEL_HSI_D0 << i)) {
					value = i;
					break;
				}
			}
		}
		break;
	case DAL_SSENIFCSI_CFGID_SENSOR_TARGET_MCLK:
		configuration = CSI_CONFIG_ID_SENSOR_TARGET_MCLK;
		break;
	case DAL_SSENIFCSI_CFGID_SENSOR_REAL_MCLK:
		configuration = CSI_CONFIG_ID_SENSOR_REAL_MCLK;
		break;
	case DAL_SSENIFCSI_CFGID_TIMEOUT_PERIOD:
		configuration = CSI_CONFIG_ID_TIMEOUT_PERIOD;
		break;
	case DAL_SSENIFCSI_CFGID_STOP_METHOD:

		if (value == DAL_SSENIF_STOPMETHOD_SIE2_FRAMEEND)
			value = DAL_SSENIF_STOPMETHOD_FRAME_END;
		else if (value == DAL_SSENIF_STOPMETHOD_SIE4_FRAMEEND)
			value = DAL_SSENIF_STOPMETHOD_FRAME_END2;
		else if (value == DAL_SSENIF_STOPMETHOD_SIE5_FRAMEEND)
			value = DAL_SSENIF_STOPMETHOD_FRAME_END3;

		if (value) {
			gcsi_obj->set_config(CSI_CONFIG_ID_DIS_METHOD, DISABLE);
			gcsi_obj->set_config(CSI_CONFIG_ID_DIS_SRC,    value - 1);
		} else {
			gcsi_obj->set_config(CSI_CONFIG_ID_DIS_METHOD, ENABLE);
		}
		return;
	case DAL_SSENIFCSI_CFGID_LPKT_MANUAL:
		configuration = CSI_CONFIG_ID_LPKT_MANUAL;
		break;
	case DAL_SSENIFCSI_CFGID_LPKT_MANUAL2:
		configuration = CSI_CONFIG_ID_LPKT_MANUAL2;
		break;
	case DAL_SSENIFCSI_CFGID_LPKT_MANUAL3:
		configuration = CSI_CONFIG_ID_LPKT_MANUAL3;
		break;
	case DAL_SSENIFCSI_CFGID_MANUAL_FORMAT:
		configuration = CSI_CONFIG_ID_MANUAL_FORMAT;
		break;
	case DAL_SSENIFCSI_CFGID_MANUAL2_FORMAT:
		configuration = CSI_CONFIG_ID_MANUAL2_FORMAT;
		break;
	case DAL_SSENIFCSI_CFGID_MANUAL3_FORMAT:
		configuration = CSI_CONFIG_ID_MANUAL3_FORMAT;
		break;
	case DAL_SSENIFCSI_CFGID_MANUAL_DATA_ID:
		configuration = CSI_CONFIG_ID_MANUAL_DATA_ID;
		break;
	case DAL_SSENIFCSI_CFGID_MANUAL2_DATA_ID:
		configuration = CSI_CONFIG_ID_MANUAL2_DATA_ID;
		break;
	case DAL_SSENIFCSI_CFGID_MANUAL3_DATA_ID:
		configuration = CSI_CONFIG_ID_MANUAL3_DATA_ID;
		break;

	/* diff area */
	case DAL_SSENIFCSI_CFGID_IMGID_TO_SIE2:
		configuration = CSI_CONFIG_ID_VIRTUAL_ID;
		break;
	case DAL_SSENIFCSI_CFGID_IMGID_TO_SIE4:
		configuration = CSI_CONFIG_ID_VIRTUAL_ID2;
		break;
	case DAL_SSENIFCSI_CFGID_IMGID_TO_SIE5:
		configuration = CSI_CONFIG_ID_VIRTUAL_ID3;
	  break;
	//case DAL_SSENIFCSI_CFGID_IMGID_TO_SIE4:
	//  configuration = CSI_CONFIG_ID_VIRTUAL_ID4;
	//  break;
	case DAL_SSENIFCSI_CFGID_CLANE_SWITCH:
		//configuration = CSI_CONFIG_ID_CLANE_SWITCH;

		//if (value == DAL_SSENIF_CFGID_CLANE_CSI2_USE_C4) {
		//	value = FALSE;
		//} else if (value == DAL_SSENIF_CFGID_CLANE_CSI2_USE_C6) {
		//	value = TRUE;
		//} else {
		//	DBG_ERR("Invalid value ID-0x%08X VAL=0x%08X\r\n", (unsigned int)config_id, (unsigned int)value);
		//	return;
		//}
		//break;
		return;

	case DAL_SSENIFCSI_CFGID_SENSOR_REAL_HSCLK:
		gsensor_hsclk = value;

		if (gsensor_hsclk > 1000000) { // > 1000000Kbps
			senphy_set_config(SENPHY_CONFIG_ID_IADJ, 0x1);
		} else {
			senphy_set_config(SENPHY_CONFIG_ID_IADJ, 0x0);
		}
		return;

	case DAL_SSENIFCSI_CFGID_IADJ:
		senphy_set_config(SENPHY_CONFIG_ID_IADJ, (value & 0x03));
		return;

	case DAL_SSENIFCSI_CFGID_CLANE_FORCE_HS:
		configuration = CSI_CONFIG_ID_A_FORCE_CLK_HS;
		break;

	case DAL_SSENIFCSI_CFGID_HSDATAO_DLY:
		configuration = CSI_CONFIG_ID_HSDATAO_DELAY;
		break;

	case DAL_SSENIFCSI_CFGID_PATGEN_EN:
		configuration = CSI_CONFIG_ID_PATGEN_EN;
		break;

	case DAL_SSENIFCSI_CFGID_PATGEN_MODE:
		configuration = CSI_CONFIG_ID_PATGEN_MODE;
		break;

	case DAL_SSENIFCSI_CFGID_PATGEN_VALUE:
		configuration = CSI_CONFIG_ID_PATGEN_VAL;
		break;

	default:
		DBG_ERR("Invalid ID 0x%08X\r\n", config_id);
		return;
	}

	gcsi_obj->set_config(configuration, value);
}

//UINT32 ssenif_get_config_csi(DAL_SSENIFCSI_CFGID config_id)
UINT32 DALCSI_API_DEFINE(get_config)(DAL_SSENIFCSI_CFGID config_id)
{
	CSI_CONFIG_ID   configuration;
	UINT32          temp;

	if (gcsi_obj == NULL) {
		DBG_ERR("no init\r\n");
		return 0;
	}

	switch (config_id) {
	case DAL_SSENIFCSI_CFGID_DLANE_NUMBER: {
			temp = gcsi_obj->get_config(CSI_CONFIG_ID_DLANE_NUMBER);
			if (temp == CSI_DATLANE_NUM_4) {
				return 4;
			} else if (temp == CSI_DATLANE_NUM_2) {
				return 2;
			} else if (temp == CSI_DATLANE_NUM_1) {
				return 1;
			}
		}
		return 0;
	case DAL_SSENIFCSI_CFGID_SENSORTYPE:
		return sensor_type;
	case DAL_SSENIFCSI_CFGID_VALID_HEIGHT:
		configuration = CSI_CONFIG_ID_VALID_HEIGHT;
		break;
	case DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE2:
		configuration = CSI_CONFIG_ID_VALID_HEIGHT;
		break;
	case DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE4:
		configuration = CSI_CONFIG_ID_VALID_HEIGHT2;
		break;
	case DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE5:
		configuration = CSI_CONFIG_ID_VALID_HEIGHT3;
		break;
	case DAL_SSENIFCSI_CFGID_DATALANE0_PIN:
	case DAL_SSENIFCSI_CFGID_DATALANE1_PIN:
	case DAL_SSENIFCSI_CFGID_DATALANE2_PIN:
	case DAL_SSENIFCSI_CFGID_DATALANE3_PIN:
		configuration = CSI_CONFIG_ID_DATALANE0_PIN + (config_id - DAL_SSENIFCSI_CFGID_DATALANE0_PIN);
		temp = gcsi_obj->get_config(configuration);
		return (DAL_SSENIF_LANESEL_HSI_D0 << temp);
	case DAL_SSENIFCSI_CFGID_SENSOR_TARGET_MCLK:
		configuration = CSI_CONFIG_ID_SENSOR_TARGET_MCLK;
		break;
	case DAL_SSENIFCSI_CFGID_SENSOR_REAL_MCLK:
		configuration = CSI_CONFIG_ID_SENSOR_REAL_MCLK;
		break;
	case DAL_SSENIFCSI_CFGID_TIMEOUT_PERIOD:
		configuration = CSI_CONFIG_ID_TIMEOUT_PERIOD;
		break;
	case DAL_SSENIFCSI_CFGID_STOP_METHOD:
		if (gcsi_obj->get_config(CSI_CONFIG_ID_DIS_METHOD)) {
			return DAL_SSENIF_STOPMETHOD_IMMEDIATELY;
		} else {
			return (gcsi_obj->get_config(CSI_CONFIG_ID_DIS_SRC) + 1);
		}
	case DAL_SSENIFCSI_CFGID_LPKT_MANUAL:
		configuration = CSI_CONFIG_ID_LPKT_MANUAL;
		break;
	case DAL_SSENIFCSI_CFGID_LPKT_MANUAL2:
		configuration = CSI_CONFIG_ID_LPKT_MANUAL2;
		break;
	case DAL_SSENIFCSI_CFGID_LPKT_MANUAL3:
		configuration = CSI_CONFIG_ID_LPKT_MANUAL3;
		break;
	case DAL_SSENIFCSI_CFGID_MANUAL_FORMAT:
		configuration = CSI_CONFIG_ID_MANUAL_FORMAT;
		break;
	case DAL_SSENIFCSI_CFGID_MANUAL2_FORMAT:
		configuration = CSI_CONFIG_ID_MANUAL2_FORMAT;
		break;
	case DAL_SSENIFCSI_CFGID_MANUAL3_FORMAT:
		configuration = CSI_CONFIG_ID_MANUAL3_FORMAT;
		break;
	case DAL_SSENIFCSI_CFGID_MANUAL_DATA_ID:
		configuration = CSI_CONFIG_ID_MANUAL_DATA_ID;
		break;
	case DAL_SSENIFCSI_CFGID_MANUAL2_DATA_ID:
		configuration = CSI_CONFIG_ID_MANUAL2_DATA_ID;
		break;
	case DAL_SSENIFCSI_CFGID_MANUAL3_DATA_ID:
		configuration = CSI_CONFIG_ID_MANUAL3_DATA_ID;
		break;

	/* diff area */
	case DAL_SSENIFCSI_CFGID_IMGID_TO_SIE2:
		configuration = CSI_CONFIG_ID_VIRTUAL_ID;
		break;
	case DAL_SSENIFCSI_CFGID_IMGID_TO_SIE4:
		configuration = CSI_CONFIG_ID_VIRTUAL_ID2;
		break;
	case DAL_SSENIFCSI_CFGID_IMGID_TO_SIE5:
		configuration = CSI_CONFIG_ID_VIRTUAL_ID3;
	  break;
	//case DAL_SSENIFCSI_CFGID_IMGID_TO_SIE4:
	//  configuration = CSI_CONFIG_ID_VIRTUAL_ID4;
	//  break;

	case DAL_SSENIFCSI_CFGID_SENSOR_REAL_HSCLK:
		return gsensor_hsclk;

	case DAL_SSENIFCSI_CFGID_CLANE_SWITCH:
		//if (gcsi_obj->get_config(CSI_CONFIG_ID_CLANE_SWITCH) == TRUE) {
		//	return DAL_SSENIF_CFGID_CLANE_CSI2_USE_C4;
		//} else {
		//	return DAL_SSENIF_CFGID_CLANE_CSI2_USE_C6;
		//}
		return DAL_SSENIF_CFGID_CLANE_CSI2_USE_C1;

	case DAL_SSENIFCSI_CFGID_SENSOR_FS_CNT:
		configuration = CSI_CONFIG_ID_FS_CNT;
		break;

	case DAL_SSENIFCSI_CFGID_FRAME_NUMBER:
		configuration = CSI_CONFIG_ID_FRAME_NUMBER;
		break;

	case DAL_SSENIFCSI_CFGID_SENSOR_ERR_CNT:
		configuration = CSI_CONFIG_ID_ERR_CNT;
		break;

	default:
		DBG_ERR("Invalid ID 0x%08X\r\n", config_id);
		return 0;
	}

	return gcsi_obj->get_config(configuration);

}

