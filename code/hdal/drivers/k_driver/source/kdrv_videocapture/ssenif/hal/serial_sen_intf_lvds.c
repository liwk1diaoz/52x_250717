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
#endif

#define MODULE_ID               LVDS_ID_LVDS
#define DALSSENIFID             DAL_SSENIF_ID_LVDS
#define DALLVDS_API_DEFINE(name) ssenif_##name##_lvds



static PLVDSOBJ                 glvds_obj;
static DAL_SSENIF_SENSORTYPE    sensor_type = DAL_SSENIF_SENSORTYPE_SONY_NONEHDR;


#if 1
static void  ssenif_set_default_sync(DAL_SSENIF_LANESEL lane_select)
{
	switch (sensor_type) {
	case DAL_SSENIF_SENSORTYPE_SONY_NONEHDR: {
			glvds_obj->set_config(LVDS_CONFIG_ID_DESKEW_EN, ENABLE);

			if (glvds_obj->get_config(LVDS_CONFIG_ID_PIXEL_DEPTH) == LVDS_PIXDEPTH_12BIT) {
				glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_HD,  0x0800);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_VD,  0x0AB0);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_LE,  0x09D0);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_FE,  0x0B60);
			} else if (glvds_obj->get_config(LVDS_CONFIG_ID_PIXEL_DEPTH) == LVDS_PIXDEPTH_10BIT) {
				glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_HD,  0x0200);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_VD,  0x02AC);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_LE,  0x0274);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_FE,  0x02D8);
			} else {
				DBG_ERR("no build-in information\r\n");
			}
		}
		break;

	case DAL_SSENIF_SENSORTYPE_SONY_LI_DOL: {
			glvds_obj->set_config(LVDS_CONFIG_ID_DESKEW_EN, ENABLE);

			if (glvds_obj->get_config(LVDS_CONFIG_ID_PIXEL_DEPTH) == LVDS_PIXDEPTH_12BIT) {
				glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_HD,  0x0800);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_LE,  0x09D0);
				glvds_obj->set_config(LVDS_CONFIG_ID_CHID_BIT0, 0);
				glvds_obj->set_config(LVDS_CONFIG_ID_CHID_BIT1, 1);
				glvds_obj->set_config(LVDS_CONFIG_ID_CHID_BIT2, 2);
				glvds_obj->set_config(LVDS_CONFIG_ID_CHID_BIT3, 3);
				glvds_obj->set_config(LVDS_CONFIG_ID_FSET_BIT,  10);
			} else if (glvds_obj->get_config(LVDS_CONFIG_ID_PIXEL_DEPTH) == LVDS_PIXDEPTH_10BIT) {
				glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_HD,  0x0000);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_LE,  0x09D0);
				glvds_obj->set_config(LVDS_CONFIG_ID_CHID_BIT0, 0);
				glvds_obj->set_config(LVDS_CONFIG_ID_CHID_BIT1, 1);
				glvds_obj->set_config(LVDS_CONFIG_ID_CHID_BIT2, 9);
				glvds_obj->set_config(LVDS_CONFIG_ID_FSET_BIT,  8);
			} else {
				DBG_ERR("no build-in information\r\n");
			}
		}
		break;

	case DAL_SSENIF_SENSORTYPE_ONSEMI: {
			glvds_obj->set_config(LVDS_CONFIG_ID_DESKEW_EN, ENABLE);

			/* Set default CHID_BIT 0/1 */
			glvds_obj->set_config(LVDS_CONFIG_ID_CHID_BIT0, 6);
			glvds_obj->set_config(LVDS_CONFIG_ID_CHID_BIT1, 7);

			/* Set default HD SYNC CODE */
			glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_HD,  0x0001);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_LE,  0x0005);

			/* Set default VD1/2/3/4 SYNC CODE */
			glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_VD,  0x0003);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_VD2, 0x0043);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_VD3, 0x0083);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_VD4, 0x00C3);

			glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_FE,  0x0007);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_FE2, 0x0047);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_FE3, 0x0087);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, LVDS_CONFIG_CTRLWORD_FE4, 0x00C7);
		}
		break;

	case DAL_SSENIF_SENSORTYPE_PANASONIC: {
			glvds_obj->set_config(LVDS_CONFIG_ID_DESKEW_EN, ENABLE);

			/* Set default CHID_BIT 0/1 */
			glvds_obj->set_config(LVDS_CONFIG_ID_CHID_BIT0, 8);
			glvds_obj->set_config(LVDS_CONFIG_ID_CHID_BIT1, 9);

			/* lane-0 SYNCCODE */
			glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_0) & 0xF)), LVDS_CONFIG_CTRLWORD_HD,  0x0000);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_0) & 0xF)), LVDS_CONFIG_CTRLWORD_LE,  0x0001);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_0) & 0xF)), LVDS_CONFIG_CTRLWORD_VD,  0x0002);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_0) & 0xF)), LVDS_CONFIG_CTRLWORD_VD2, 0x0202);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_0) & 0xF)), LVDS_CONFIG_CTRLWORD_VD3, 0x0102);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_0) & 0xF)), LVDS_CONFIG_CTRLWORD_VD4, 0x0302);

			glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_0) & 0xF)), LVDS_CONFIG_CTRLWORD_FE,  0x0003);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_0) & 0xF)), LVDS_CONFIG_CTRLWORD_FE2, 0x0203);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_0) & 0xF)), LVDS_CONFIG_CTRLWORD_FE3, 0x0103);
			glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_0) & 0xF)), LVDS_CONFIG_CTRLWORD_FE4, 0x0303);

			if (glvds_obj->get_config(LVDS_CONFIG_ID_DLANE_NUMBER) >= LVDS_DATLANE_2) {
				glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_1) & 0xF)), LVDS_CONFIG_CTRLWORD_HD,  0x0010);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_1) & 0xF)), LVDS_CONFIG_CTRLWORD_LE,  0x0011);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_1) & 0xF)), LVDS_CONFIG_CTRLWORD_VD,  0x0012);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_1) & 0xF)), LVDS_CONFIG_CTRLWORD_VD2, 0x0212);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_1) & 0xF)), LVDS_CONFIG_CTRLWORD_VD3, 0x0112);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_1) & 0xF)), LVDS_CONFIG_CTRLWORD_VD4, 0x0312);

				glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_1) & 0xF)), LVDS_CONFIG_CTRLWORD_FE,  0x0013);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_1) & 0xF)), LVDS_CONFIG_CTRLWORD_FE2, 0x0213);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_1) & 0xF)), LVDS_CONFIG_CTRLWORD_FE3, 0x0113);
				glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_1) & 0xF)), LVDS_CONFIG_CTRLWORD_FE4, 0x0313);

				if (glvds_obj->get_config(LVDS_CONFIG_ID_DLANE_NUMBER) >= LVDS_DATLANE_4) {
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_2) & 0xF)), LVDS_CONFIG_CTRLWORD_HD,  0x0004);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_2) & 0xF)), LVDS_CONFIG_CTRLWORD_LE,  0x0005);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_2) & 0xF)), LVDS_CONFIG_CTRLWORD_VD,  0x0006);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_2) & 0xF)), LVDS_CONFIG_CTRLWORD_VD2, 0x0206);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_2) & 0xF)), LVDS_CONFIG_CTRLWORD_VD3, 0x0106);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_2) & 0xF)), LVDS_CONFIG_CTRLWORD_VD4, 0x0306);

					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_2) & 0xF)), LVDS_CONFIG_CTRLWORD_FE,  0x0007);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_2) & 0xF)), LVDS_CONFIG_CTRLWORD_FE2, 0x0207);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_2) & 0xF)), LVDS_CONFIG_CTRLWORD_FE3, 0x0107);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_2) & 0xF)), LVDS_CONFIG_CTRLWORD_FE4, 0x0307);


					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_3) & 0xF)), LVDS_CONFIG_CTRLWORD_HD,  0x0014);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_3) & 0xF)), LVDS_CONFIG_CTRLWORD_LE,  0x0015);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_3) & 0xF)), LVDS_CONFIG_CTRLWORD_VD,  0x0016);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_3) & 0xF)), LVDS_CONFIG_CTRLWORD_VD2, 0x0216);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_3) & 0xF)), LVDS_CONFIG_CTRLWORD_VD3, 0x0116);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_3) & 0xF)), LVDS_CONFIG_CTRLWORD_VD4, 0x0316);

					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_3) & 0xF)), LVDS_CONFIG_CTRLWORD_FE,  0x0017);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_3) & 0xF)), LVDS_CONFIG_CTRLWORD_FE2, 0x0217);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_3) & 0xF)), LVDS_CONFIG_CTRLWORD_FE3, 0x0117);
					glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_3) & 0xF)), LVDS_CONFIG_CTRLWORD_FE4, 0x0317);


					if (glvds_obj->get_config(LVDS_CONFIG_ID_DLANE_NUMBER) >= LVDS_DATLANE_6) {
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_4) & 0xF)), LVDS_CONFIG_CTRLWORD_HD,  0x0008);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_4) & 0xF)), LVDS_CONFIG_CTRLWORD_LE,  0x0009);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_4) & 0xF)), LVDS_CONFIG_CTRLWORD_VD,  0x000A);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_4) & 0xF)), LVDS_CONFIG_CTRLWORD_VD2, 0x020A);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_4) & 0xF)), LVDS_CONFIG_CTRLWORD_VD3, 0x010A);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_4) & 0xF)), LVDS_CONFIG_CTRLWORD_VD4, 0x030A);

						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_4) & 0xF)), LVDS_CONFIG_CTRLWORD_FE,  0x000B);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_4) & 0xF)), LVDS_CONFIG_CTRLWORD_FE2, 0x020B);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_4) & 0xF)), LVDS_CONFIG_CTRLWORD_FE3, 0x010B);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_4) & 0xF)), LVDS_CONFIG_CTRLWORD_FE4, 0x030B);

						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_5) & 0xF)), LVDS_CONFIG_CTRLWORD_HD,  0x0018);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_5) & 0xF)), LVDS_CONFIG_CTRLWORD_LE,  0x0019);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_5) & 0xF)), LVDS_CONFIG_CTRLWORD_VD,  0x001A);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_5) & 0xF)), LVDS_CONFIG_CTRLWORD_VD2, 0x021A);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_5) & 0xF)), LVDS_CONFIG_CTRLWORD_VD3, 0x011A);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_5) & 0xF)), LVDS_CONFIG_CTRLWORD_VD4, 0x031A);

						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_5) & 0xF)), LVDS_CONFIG_CTRLWORD_FE,  0x001B);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_5) & 0xF)), LVDS_CONFIG_CTRLWORD_FE2, 0x021B);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_5) & 0xF)), LVDS_CONFIG_CTRLWORD_FE3, 0x011B);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_5) & 0xF)), LVDS_CONFIG_CTRLWORD_FE4, 0x031B);

					if (glvds_obj->get_config(LVDS_CONFIG_ID_DLANE_NUMBER) == LVDS_DATLANE_8) {
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_6) & 0xF)), LVDS_CONFIG_CTRLWORD_HD,  0x000C);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_6) & 0xF)), LVDS_CONFIG_CTRLWORD_LE,  0x000D);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_6) & 0xF)), LVDS_CONFIG_CTRLWORD_VD,  0x000E);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_6) & 0xF)), LVDS_CONFIG_CTRLWORD_VD2, 0x020E);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_6) & 0xF)), LVDS_CONFIG_CTRLWORD_VD3, 0x010E);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_6) & 0xF)), LVDS_CONFIG_CTRLWORD_VD4, 0x030E);

						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_6) & 0xF)), LVDS_CONFIG_CTRLWORD_FE,  0x000F);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_6) & 0xF)), LVDS_CONFIG_CTRLWORD_FE2, 0x020F);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_6) & 0xF)), LVDS_CONFIG_CTRLWORD_FE3, 0x010F);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_6) & 0xF)), LVDS_CONFIG_CTRLWORD_FE4, 0x030F);

						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_7) & 0xF)), LVDS_CONFIG_CTRLWORD_HD,  0x001C);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_7) & 0xF)), LVDS_CONFIG_CTRLWORD_LE,  0x001D);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_7) & 0xF)), LVDS_CONFIG_CTRLWORD_VD,  0x001E);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_7) & 0xF)), LVDS_CONFIG_CTRLWORD_VD2, 0x021E);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_7) & 0xF)), LVDS_CONFIG_CTRLWORD_VD3, 0x011E);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_7) & 0xF)), LVDS_CONFIG_CTRLWORD_VD4, 0x031E);

						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_7) & 0xF)), LVDS_CONFIG_CTRLWORD_FE,  0x001F);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_7) & 0xF)), LVDS_CONFIG_CTRLWORD_FE2, 0x021F);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_7) & 0xF)), LVDS_CONFIG_CTRLWORD_FE3, 0x011F);
						glvds_obj->set_padlane_config((LVDS_IN_VALID)(0x1 << (glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_7) & 0xF)), LVDS_CONFIG_CTRLWORD_FE4, 0x031F);
					}
					}
				}
			}
		}
		break;

	default:
		DBG_ERR("no build-in information\r\n");
		break;
	}
}
#endif

#ifdef __KERNEL__
//void ssenif_hook_lvds(void *object)
void DALLVDS_API_DEFINE(hook)(void *object)
{
	glvds_obj = (PLVDSOBJ)object;
}
EXPORT_SYMBOL(DALLVDS_API_DEFINE(hook));
#endif

//ER ssenif_init_lvds(void)
ER DALLVDS_API_DEFINE(init)(void)
{
	#ifdef __KERNEL__
	if (glvds_obj == NULL) {
		DBG_ERR("driver not loaded\r\n");
		return E_OACV;
	}
	#else
	glvds_obj = lvds_get_drv_object(MODULE_ID);
	#endif

	ssenif_install_cmd();
	return E_OK;
}

//ER ssenif_open_lvds(void)
ER DALLVDS_API_DEFINE(open)(void)
{
	UINT32 default_sync[3] = {0xFFFF, 0x0000, 0x0000};

	if (glvds_obj == NULL) {
		DBG_ERR("no init\r\n");
		return E_OACV;
	}

	glvds_obj->open();
	glvds_obj->set_sync_word(3, default_sync);
	glvds_obj->set_config(LVDS_CONFIG_ID_TIMEOUT_PERIOD, 1000);
	return E_OK;
}

//BOOL ssenif_is_opened_lvds(void)
BOOL DALLVDS_API_DEFINE(is_opened)(void)
{
	if (glvds_obj == NULL) {
		DBG_ERR("no init\r\n");
		return E_OACV;
	}

	return glvds_obj->is_opened();
}

//ER ssenif_close_lvds(void)
ER DALLVDS_API_DEFINE(close)(void)
{
	if (glvds_obj == NULL) {
		DBG_ERR("no init\r\n");
		return E_OACV;
	}

	return glvds_obj->close();
}

//ER ssenif_start_lvds(void)
ER DALLVDS_API_DEFINE(start)(void)
{
	UINT32 i, vd_mask, hd_mask;

	if (glvds_obj == NULL) {
		DBG_ERR("no init\r\n");
		return E_OACV;
	}

	ssenif_lastest_start = DALSSENIFID;

	if (glvds_obj->get_enable()) {
		goto EXIT;
	} else {

		if ((sensor_type == DAL_SSENIF_SENSORTYPE_ONSEMI) && (glvds_obj->get_config(LVDS_CONFIG_ID_DLANE_NUMBER) > LVDS_DATLANE_4)) {
			glvds_obj->set_config(LVDS_CONFIG_ID_CLANE_NUMBER, LVDS_CLKLANE_2);
		} else {
			glvds_obj->set_config(LVDS_CONFIG_ID_CLANE_NUMBER, LVDS_CLKLANE_1);
		}

		/*
		    Handle the VD/HD CONTROL MASK value
		*/
		if (glvds_obj->get_config(LVDS_CONFIG_ID_PIXEL_DEPTH) == LVDS_PIXDEPTH_10BIT) {
			vd_mask = 0x3FF;
			hd_mask = 0x3FF;
		} else if (glvds_obj->get_config(LVDS_CONFIG_ID_PIXEL_DEPTH) == LVDS_PIXDEPTH_12BIT) {
			vd_mask = 0xFFF;
			hd_mask = 0xFFF;
		} else if (glvds_obj->get_config(LVDS_CONFIG_ID_PIXEL_DEPTH) == LVDS_PIXDEPTH_14BIT) {
			vd_mask = 0x3FFF;
			hd_mask = 0x3FFF;
		} else if (glvds_obj->get_config(LVDS_CONFIG_ID_PIXEL_DEPTH) == LVDS_PIXDEPTH_8BIT) {
			vd_mask = 0xFF;
			hd_mask = 0xFF;
		} else {
			vd_mask = 0xFFFF;
			hd_mask = 0xFFFF;
		}

		hd_mask &= ~(0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT0));
		hd_mask &= ~(0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT1));
		hd_mask &= ~(0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT2));
		hd_mask &= ~(0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT3));

		if (sensor_type == DAL_SSENIF_SENSORTYPE_SONY_LI_DOL) {
			hd_mask &= ~(0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_FSET_BIT));
			vd_mask &= ~(0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_FSET_BIT));
		}

		glvds_obj->set_padlane_config(LVDS_IN_VALID_SEQ10LANE,  LVDS_CONFIG_CTRLWORD_HD_MASK, hd_mask);
		glvds_obj->set_padlane_config(LVDS_IN_VALID_SEQ10LANE,  LVDS_CONFIG_CTRLWORD_VD_MASK, vd_mask);

		/*
		    Clear the HD SYNCCODE's bit fields
		*/
		for (i = 0; i < 8; i++) {
			UINT32 temp, temp2;

			temp = glvds_obj->get_channel_config(LVDS_DATLANE_ID_0 + i, LVDS_CONFIG_CTRLWORD_HD);
			temp &= ~(0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT0));
			temp &= ~(0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT1));
			temp &= ~(0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT2));
			temp &= ~(0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT3));
			temp &= ~(0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_FSET_BIT));
			glvds_obj->set_channel_config(LVDS_DATLANE_ID_0 + i, LVDS_CONFIG_CTRLWORD_HD, temp);

			/* Set DOL VD SYNC CODE from HD SYNC CODE */
			if (sensor_type == DAL_SSENIF_SENSORTYPE_SONY_LI_DOL) {

				//VD1
				temp2 = temp;
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID) & 0x1) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT0));
				}
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID) & 0x2) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT1));
				}
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID) & 0x4) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT2));
				}
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID) & 0x8) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT3));
				}
				glvds_obj->set_channel_config(LVDS_DATLANE_ID_0 + i, LVDS_CONFIG_CTRLWORD_VD, temp2 & vd_mask);

				//VD2
				temp2 = temp;
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID2) & 0x1) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT0));
				}
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID2) & 0x2) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT1));
				}
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID2) & 0x4) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT2));
				}
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID2) & 0x8) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT3));
				}
				glvds_obj->set_channel_config(LVDS_DATLANE_ID_0 + i, LVDS_CONFIG_CTRLWORD_VD2, temp2 & vd_mask);

				//VD3
				temp2 = temp;
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID3) & 0x1) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT0));
				}
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID3) & 0x2) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT1));
				}
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID3) & 0x4) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT2));
				}
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID3) & 0x8) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT3));
				}
				glvds_obj->set_channel_config(LVDS_DATLANE_ID_0 + i, LVDS_CONFIG_CTRLWORD_VD3, temp2 & vd_mask);


				//VD4
				temp2 = temp;
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID4) & 0x1) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT0));
				}
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID4) & 0x2) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT1));
				}
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID4) & 0x4) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT2));
				}
				if (glvds_obj->get_config(LVDS_CONFIG_ID_CHID4) & 0x8) {
					temp2 |= (0x1 << glvds_obj->get_config(LVDS_CONFIG_ID_CHID_BIT3));
				}
				glvds_obj->set_channel_config(LVDS_DATLANE_ID_0 + i, LVDS_CONFIG_CTRLWORD_VD4, temp2 & vd_mask);

			}
		}

		glvds_obj->set_enable(ENABLE);
		return glvds_obj->enable_streaming();
	}

EXIT:
	return E_OK;

}

//ER ssenif_stop_lvds(void)
ER DALLVDS_API_DEFINE(stop)(void)
{
	if (glvds_obj == NULL) {
		DBG_ERR("no init\r\n");
		return E_OACV;
	}

	if (glvds_obj->get_enable()) {

		glvds_obj->set_enable(DISABLE);
		if (glvds_obj->get_config(LVDS_CONFIG_ID_FORCE_EN) == FALSE) {
			if (glvds_obj->get_config(LVDS_CONFIG_ID_DISABLE_SOURCE) == LVDS_DIS_SRC_FRAME_END2) {
				glvds_obj->wait_interrupt(LVDS_INTERRUPT_FRMEND2);
			} else if (glvds_obj->get_config(LVDS_CONFIG_ID_DISABLE_SOURCE) == LVDS_DIS_SRC_FRAME_END3) {
				glvds_obj->wait_interrupt(LVDS_INTERRUPT_FRMEND3);
			} else if (glvds_obj->get_config(LVDS_CONFIG_ID_DISABLE_SOURCE) == LVDS_DIS_SRC_FRAME_END4) {
				glvds_obj->wait_interrupt(LVDS_INTERRUPT_FRMEND4);
			} else {
				glvds_obj->wait_interrupt(LVDS_INTERRUPT_FRMEND);
			}
		}
	}
	return E_OK;
}

//DAL_SSENIF_INTERRUPT ssenif_wait_interrupt_lvds(DAL_SSENIF_INTERRUPT waited_flag)
DAL_SSENIF_INTERRUPT DALLVDS_API_DEFINE(wait_interrupt)(DAL_SSENIF_INTERRUPT waited_flag)
{
	LVDS_INTERRUPT          _waited_flag = 0;
	DAL_SSENIF_INTERRUPT    ret = 0;

	if (glvds_obj == NULL) {
		DBG_ERR("no init\r\n");
		return E_OACV;
	}

	if ((waited_flag & DAL_SSENIF_INTERRUPT_VD)||(waited_flag & DAL_SSENIF_INTERRUPT_SIE1_VD)) {
		_waited_flag |= LVDS_INTERRUPT_VD;
	}
	if ((waited_flag & DAL_SSENIF_INTERRUPT_VD2)||(waited_flag & DAL_SSENIF_INTERRUPT_SIE2_VD)) {
		_waited_flag |= LVDS_INTERRUPT_VD2;
	}
	if ((waited_flag & DAL_SSENIF_INTERRUPT_VD3)||(waited_flag & DAL_SSENIF_INTERRUPT_SIE4_VD)) {
		_waited_flag |= LVDS_INTERRUPT_VD3;
	}
	if ((waited_flag & DAL_SSENIF_INTERRUPT_VD4)||(waited_flag & DAL_SSENIF_INTERRUPT_SIE5_VD)) {
		_waited_flag |= LVDS_INTERRUPT_VD4;
	}
	if ((waited_flag & DAL_SSENIF_INTERRUPT_FRAMEEND)||(waited_flag & DAL_SSENIF_INTERRUPT_SIE1_FRAMEEND)) {
		_waited_flag |= LVDS_INTERRUPT_FRMEND;
	}
	if ((waited_flag & DAL_SSENIF_INTERRUPT_FRAMEEND2)||(waited_flag & DAL_SSENIF_INTERRUPT_SIE2_FRAMEEND)) {
		_waited_flag |= LVDS_INTERRUPT_FRMEND2;
	}
	if ((waited_flag & DAL_SSENIF_INTERRUPT_FRAMEEND3)||(waited_flag & DAL_SSENIF_INTERRUPT_SIE4_FRAMEEND)) {
		_waited_flag |= LVDS_INTERRUPT_FRMEND3;
	}
	if ((waited_flag & DAL_SSENIF_INTERRUPT_FRAMEEND4)||(waited_flag & DAL_SSENIF_INTERRUPT_SIE5_FRAMEEND)) {
		_waited_flag |= LVDS_INTERRUPT_FRMEND4;
	}

	_waited_flag = glvds_obj->wait_interrupt(_waited_flag);

	if (_waited_flag & LVDS_INTERRUPT_VD) {
		ret |= (DAL_SSENIF_INTERRUPT_VD|DAL_SSENIF_INTERRUPT_SIE1_VD);
	}
	if (_waited_flag & LVDS_INTERRUPT_VD2) {
		ret |= (DAL_SSENIF_INTERRUPT_VD2|DAL_SSENIF_INTERRUPT_SIE2_VD);
	}
	if (_waited_flag & LVDS_INTERRUPT_VD3) {
		ret |= (DAL_SSENIF_INTERRUPT_VD3|DAL_SSENIF_INTERRUPT_SIE4_VD);
	}
	if (_waited_flag & LVDS_INTERRUPT_VD4) {
		ret |= (DAL_SSENIF_INTERRUPT_VD4|DAL_SSENIF_INTERRUPT_SIE5_VD);
	}
	if (_waited_flag & LVDS_INTERRUPT_FRMEND) {
		ret |= (DAL_SSENIF_INTERRUPT_FRAMEEND|DAL_SSENIF_INTERRUPT_SIE1_FRAMEEND);
	}
	if (_waited_flag & LVDS_INTERRUPT_FRMEND2) {
		ret |= (DAL_SSENIF_INTERRUPT_FRAMEEND2|DAL_SSENIF_INTERRUPT_SIE2_FRAMEEND);
	}
	if (_waited_flag & LVDS_INTERRUPT_FRMEND3) {
		ret |= (DAL_SSENIF_INTERRUPT_FRAMEEND3|DAL_SSENIF_INTERRUPT_SIE4_FRAMEEND);
	}
	if (_waited_flag & LVDS_INTERRUPT_FRMEND4) {
		ret |= (DAL_SSENIF_INTERRUPT_FRAMEEND4|DAL_SSENIF_INTERRUPT_SIE5_FRAMEEND);
	}
	if (_waited_flag & LVDS_INTERRUPT_ABORT) {
		ret |= DAL_SSENIF_INTERRUPT_ABORT;
	}

	return ret;

}

//void ssenif_dump_debug_information_lvds(void)
void DALLVDS_API_DEFINE(dump_debug_information)(void)
{
	CHAR sensor_type[7][5] = {"SONY", "SDOL", "SVC", "OMNI", "ONSM", "PANA", "OTHR"};
	int get_type;

	if (ssenif_list_name_dump) {
		DBG_DUMP("\r\n      EN SENSOR  dl-no  depth  width  height  cksw  timeout  stmd  siex  sie2  sie3  sie4  O0  O1  O2  O3  O4  O5  O6  O7  MSb CKn\r\n");
	}

	if (glvds_obj != NULL) {
		get_type = DALLVDS_API_DEFINE(get_config)(DAL_SSENIFLVDS_CFGID_SENSORTYPE);
		if (get_type > DAL_SSENIF_SENSORTYPE_OTHERS) {
			get_type = DAL_SSENIF_SENSORTYPE_OTHERS;
		}

		DBG_DUMP("LVDS0  %d  %s     %d     %02d    %04d    %04d     %d    %04d      %d    %02d    %02d    %02d    %02d    %d   %d   %d   %d   %d   %d   %d   %d   %d   %d\r\n",
				 glvds_obj->get_enable(),
				 sensor_type[get_type],
				 (UINT)DALLVDS_API_DEFINE(get_config)(DAL_SSENIFLVDS_CFGID_DLANE_NUMBER),
				 (UINT)DALLVDS_API_DEFINE(get_config)(DAL_SSENIFLVDS_CFGID_PIXEL_DEPTH),
				 (UINT)DALLVDS_API_DEFINE(get_config)(DAL_SSENIFLVDS_CFGID_VALID_WIDTH),
				 (UINT)DALLVDS_API_DEFINE(get_config)(DAL_SSENIFLVDS_CFGID_VALID_HEIGHT),
				 (UINT)DALLVDS_API_DEFINE(get_config)(DAL_SSENIFLVDS_CFGID_CLANE_SWITCH),
				 (UINT)DALLVDS_API_DEFINE(get_config)(DAL_SSENIFLVDS_CFGID_TIMEOUT_PERIOD),
				 (UINT)DALLVDS_API_DEFINE(get_config)(DAL_SSENIFLVDS_CFGID_STOP_METHOD),
				 (UINT)DALLVDS_API_DEFINE(get_config)(DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE),
				 (UINT)DALLVDS_API_DEFINE(get_config)(DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE2),
				 (UINT)DALLVDS_API_DEFINE(get_config)(DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE3),
				 (UINT)DALLVDS_API_DEFINE(get_config)(DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE4),
				 (UINT)glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_0) & 0xF,
				 (UINT)glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_1) & 0xF,
				 (UINT)glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_2) & 0xF,
				 (UINT)glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_3) & 0xF,
				 (UINT)glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_4) & 0xF,
				 (UINT)glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_5) & 0xF,
				 (UINT)glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_6) & 0xF,
				 (UINT)glvds_obj->get_config(LVDS_CONFIG_ID_OUTORDER_7) & 0xF,
				 (UINT)DALLVDS_API_DEFINE(get_config)(DAL_SSENIFLVDS_CFGID_PIXEL_INORDER),
				 (UINT)glvds_obj->get_config(LVDS_CONFIG_ID_CLANE_NUMBER) + 1
				);
	}
}

//void ssenif_set_config_lvds(DAL_SSENIFLVDS_CFGID config_id, UINT32 value)
void DALLVDS_API_DEFINE(set_config)(DAL_SSENIFLVDS_CFGID config_id, UINT32 value)
{
	LVDS_CONFIG_ID  configuration;
	UINT32          i;

	if (glvds_obj == NULL) {
		DBG_ERR("no init\r\n");
		return;
	}

	switch (config_id) {
	case DAL_SSENIFLVDS_CFGID_SENSORTYPE: {
			sensor_type = value;
			switch (sensor_type) {
			case DAL_SSENIF_SENSORTYPE_SONY_LI_DOL:
				glvds_obj->set_config(LVDS_CONFIG_ID_VD_GEN_METHOD,  LVDS_VSYNC_GEN_MATCH_VD_BY_FSET);
				glvds_obj->set_config(LVDS_CONFIG_ID_VD_GEN_WITH_HD, DISABLE);
				glvds_obj->set_config(LVDS_CONFIG_ID_PIXEL_INORDER,  LVDS_DATAIN_BIT_ORDER_MSB);
				break;
			case DAL_SSENIF_SENSORTYPE_ONSEMI:
				glvds_obj->set_config(LVDS_CONFIG_ID_VD_GEN_METHOD,  LVDS_VSYNC_GEN_MATCH_VD);
				glvds_obj->set_config(LVDS_CONFIG_ID_VD_GEN_WITH_HD, ENABLE);
				glvds_obj->set_config(LVDS_CONFIG_ID_PIXEL_INORDER,  LVDS_DATAIN_BIT_ORDER_LSB);
				break;
			case DAL_SSENIF_SENSORTYPE_PANASONIC:
				glvds_obj->set_config(LVDS_CONFIG_ID_VD_GEN_METHOD,  LVDS_VSYNC_GEN_MATCH_VD);
				glvds_obj->set_config(LVDS_CONFIG_ID_VD_GEN_WITH_HD, ENABLE);
				glvds_obj->set_config(LVDS_CONFIG_ID_PIXEL_INORDER,  LVDS_DATAIN_BIT_ORDER_MSB);
				break;

			case DAL_SSENIF_SENSORTYPE_SONY_NONEHDR:
			default:
				glvds_obj->set_config(LVDS_CONFIG_ID_VD_GEN_METHOD,  LVDS_VSYNC_GEN_VD2HD);
				glvds_obj->set_config(LVDS_CONFIG_ID_VD_GEN_WITH_HD, DISABLE);
				break;
			}
		}
		return;

	case DAL_SSENIFLVDS_CFGID_DLANE_NUMBER:
		configuration = LVDS_CONFIG_ID_DLANE_NUMBER;
		if (value == 1) {
			value = LVDS_DATLANE_1;
		} else if (value == 2) {
			value = LVDS_DATLANE_2;
		} else if (value == 4) {
			value = LVDS_DATLANE_4;
		} else if (value == 6) {
			value = LVDS_DATLANE_6;
		} else if (value == 8) {
			value = LVDS_DATLANE_8;
		} else {
			DBG_ERR("Err parameter %d\r\n", (UINT)value);
			return;
		}
		break;
	case DAL_SSENIFLVDS_CFGID_PIXEL_DEPTH:
		configuration = LVDS_CONFIG_ID_PIXEL_DEPTH;
		if (value == 8) {
			value = LVDS_PIXDEPTH_8BIT;
		} else if (value == 10) {
			value = LVDS_PIXDEPTH_10BIT;
		} else if (value == 12) {
			value = LVDS_PIXDEPTH_12BIT;
		} else if (value == 14) {
			value = LVDS_PIXDEPTH_14BIT;
		} else if (value == 16) {
			value = LVDS_PIXDEPTH_16BIT;
		} else {
			DBG_ERR("Err parameter %d\r\n", (UINT)value);
			return;
		}
		break;
	case DAL_SSENIFLVDS_CFGID_PIXEL_INORDER:
		configuration = LVDS_CONFIG_ID_PIXEL_INORDER;
		break;
	case DAL_SSENIFLVDS_CFGID_TIMEOUT_PERIOD:
		configuration = LVDS_CONFIG_ID_TIMEOUT_PERIOD;
		break;
	case DAL_SSENIFLVDS_CFGID_STOP_METHOD:

		if (value == DAL_SSENIF_STOPMETHOD_SIE1_FRAMEEND)
			value = DAL_SSENIF_STOPMETHOD_FRAME_END;
		else if (value == DAL_SSENIF_STOPMETHOD_SIE2_FRAMEEND)
			value = DAL_SSENIF_STOPMETHOD_FRAME_END2;
		else if (value == DAL_SSENIF_STOPMETHOD_SIE4_FRAMEEND)
			value = DAL_SSENIF_STOPMETHOD_FRAME_END3;
		else if (value == DAL_SSENIF_STOPMETHOD_SIE5_FRAMEEND)
			value = DAL_SSENIF_STOPMETHOD_FRAME_END4;

		if (value) {
			glvds_obj->set_config(LVDS_CONFIG_ID_FORCE_EN,       DISABLE);
			glvds_obj->set_config(LVDS_CONFIG_ID_DISABLE_SOURCE, value - 1);
		} else {
			glvds_obj->set_config(LVDS_CONFIG_ID_FORCE_EN,       ENABLE);
		}
		return;
	case DAL_SSENIFLVDS_CFGID_VALID_WIDTH:
		configuration = LVDS_CONFIG_ID_VALID_WIDTH;
		break;
	case DAL_SSENIFLVDS_CFGID_VALID_HEIGHT:
		glvds_obj->set_config(LVDS_CONFIG_ID_VALID_HEIGHT,  value);
		glvds_obj->set_config(LVDS_CONFIG_ID_VALID_HEIGHT2, value);
		glvds_obj->set_config(LVDS_CONFIG_ID_VALID_HEIGHT3, value);
		glvds_obj->set_config(LVDS_CONFIG_ID_VALID_HEIGHT4, value);
		return;
	case DAL_SSENIFLVDS_CFGID_FSET_BIT:
		configuration = LVDS_CONFIG_ID_FSET_BIT;
		break;
	case DAL_SSENIFLVDS_CFGID_IMGID_BIT0:
		configuration = LVDS_CONFIG_ID_CHID_BIT0;
		break;
	case DAL_SSENIFLVDS_CFGID_IMGID_BIT1:
		configuration = LVDS_CONFIG_ID_CHID_BIT1;
		break;
	case DAL_SSENIFLVDS_CFGID_IMGID_BIT2:
		configuration = LVDS_CONFIG_ID_CHID_BIT2;
		break;
	case DAL_SSENIFLVDS_CFGID_IMGID_BIT3:
		configuration = LVDS_CONFIG_ID_CHID_BIT3;
		break;
	case DAL_SSENIFLVDS_CFGID_OUTORDER_0:
	case DAL_SSENIFLVDS_CFGID_OUTORDER_1:
	case DAL_SSENIFLVDS_CFGID_OUTORDER_2:
	case DAL_SSENIFLVDS_CFGID_OUTORDER_3:
	case DAL_SSENIFLVDS_CFGID_OUTORDER_4:
	case DAL_SSENIFLVDS_CFGID_OUTORDER_5:
	case DAL_SSENIFLVDS_CFGID_OUTORDER_6:
	case DAL_SSENIFLVDS_CFGID_OUTORDER_7: {
			configuration = LVDS_CONFIG_ID_OUTORDER_0 + (config_id - DAL_SSENIFLVDS_CFGID_OUTORDER_0);
			for (i = 0; i < 8; i++) {
				if (value & (DAL_SSENIF_LANESEL_HSI_D0 << i)) {
					value = i + LVDS_PIXEL_ORDER_D0;
					break;
				}
			}
		}
		break;

	/* diff area */
	case DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE:
		configuration = LVDS_CONFIG_ID_CHID;
		break;
	case DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE2:
		configuration = LVDS_CONFIG_ID_CHID2;
		break;
	case DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE4:
		configuration = LVDS_CONFIG_ID_CHID3;
		break;
	case DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE5:
		configuration = LVDS_CONFIG_ID_CHID4;
		break;
	case DAL_SSENIFLVDS_CFGID_CLANE_SWITCH:
		//configuration = LVDS_CONFIG_ID_CLANE_SWITCH;

		//if (value == DAL_SSENIF_CFGID_CLANE_LVDS_USE_C0C4) {
		//	value = FALSE;
		//} else if (value == DAL_SSENIF_CFGID_CLANE_LVDS_USE_C2C6) {
		//	value = TRUE;
		//} else {
		//	DBG_ERR("Invalid value ID-0x%08X VAL=0x%08X\r\n", (UINT)config_id, (UINT)value);
		//	return;
		//}
		//break;
		return;

	default:
		DBG_ERR("Invalid ID 0x%08X\r\n", (UINT)config_id);
		return;
	}

	glvds_obj->set_config(configuration, value);
}

//UINT32 ssenif_get_config_lvds(DAL_SSENIFLVDS_CFGID config_id)
UINT32 DALLVDS_API_DEFINE(get_config)(DAL_SSENIFLVDS_CFGID config_id)
{
	LVDS_CONFIG_ID  configuration;
	UINT32          temp;

	if (glvds_obj == NULL) {
		DBG_ERR("no init\r\n");
		return 0;
	}

	switch (config_id) {
	case DAL_SSENIFLVDS_CFGID_SENSORTYPE:
		return sensor_type;
	case DAL_SSENIFLVDS_CFGID_DLANE_NUMBER:
		temp = glvds_obj->get_config(LVDS_CONFIG_ID_DLANE_NUMBER);
		if (temp == LVDS_DATLANE_1) {
			return 1;
		} else if (temp == LVDS_DATLANE_2) {
			return 2;
		} else if (temp == LVDS_DATLANE_4) {
			return 4;
		} else if (temp == LVDS_DATLANE_6) {
			return 6;
		} else if (temp == LVDS_DATLANE_8) {
			return 8;
		}
		return 0;
	case DAL_SSENIFLVDS_CFGID_PIXEL_DEPTH:
		temp = glvds_obj->get_config(LVDS_CONFIG_ID_PIXEL_DEPTH);
		if (temp == LVDS_PIXDEPTH_8BIT) {
			return 8;
		} else if (temp == LVDS_PIXDEPTH_10BIT) {
			return 10;
		} else if (temp == LVDS_PIXDEPTH_12BIT) {
			return 12;
		} else if (temp == LVDS_PIXDEPTH_14BIT) {
			return 14;
		} else if (temp == LVDS_PIXDEPTH_16BIT) {
			return 16;
		}
		return 0;
	case DAL_SSENIFLVDS_CFGID_PIXEL_INORDER:
		configuration = LVDS_CONFIG_ID_PIXEL_INORDER;
		break;
	case DAL_SSENIFLVDS_CFGID_TIMEOUT_PERIOD:
		configuration = LVDS_CONFIG_ID_TIMEOUT_PERIOD;
		break;
	case DAL_SSENIFLVDS_CFGID_STOP_METHOD:
		if (glvds_obj->get_config(LVDS_CONFIG_ID_FORCE_EN)) {
			return DAL_SSENIF_STOPMETHOD_IMMEDIATELY;
		} else {
			return (glvds_obj->get_config(LVDS_CONFIG_ID_DISABLE_SOURCE) + 1);
		}
	case DAL_SSENIFLVDS_CFGID_VALID_WIDTH:
		configuration = LVDS_CONFIG_ID_VALID_WIDTH;
		break;
	case DAL_SSENIFLVDS_CFGID_VALID_HEIGHT:
		configuration = LVDS_CONFIG_ID_VALID_HEIGHT;
		break;
	case DAL_SSENIFLVDS_CFGID_FSET_BIT:
		configuration = LVDS_CONFIG_ID_FSET_BIT;
		break;
	case DAL_SSENIFLVDS_CFGID_IMGID_BIT0:
		configuration = LVDS_CONFIG_ID_CHID_BIT0;
		break;
	case DAL_SSENIFLVDS_CFGID_IMGID_BIT1:
		configuration = LVDS_CONFIG_ID_CHID_BIT1;
		break;
	case DAL_SSENIFLVDS_CFGID_IMGID_BIT2:
		configuration = LVDS_CONFIG_ID_CHID_BIT2;
		break;
	case DAL_SSENIFLVDS_CFGID_IMGID_BIT3:
		configuration = LVDS_CONFIG_ID_CHID_BIT3;
		break;
	case DAL_SSENIFLVDS_CFGID_OUTORDER_0:
	case DAL_SSENIFLVDS_CFGID_OUTORDER_1:
	case DAL_SSENIFLVDS_CFGID_OUTORDER_2:
	case DAL_SSENIFLVDS_CFGID_OUTORDER_3:
	case DAL_SSENIFLVDS_CFGID_OUTORDER_4:
	case DAL_SSENIFLVDS_CFGID_OUTORDER_5:
	case DAL_SSENIFLVDS_CFGID_OUTORDER_6:
	case DAL_SSENIFLVDS_CFGID_OUTORDER_7:
		configuration = LVDS_CONFIG_ID_OUTORDER_0 + (config_id - DAL_SSENIFLVDS_CFGID_OUTORDER_0);
		temp = glvds_obj->get_config(configuration) & 0xF;
		return (DAL_SSENIF_LANESEL_HSI_D0 << temp);

	/* diff area */
	case DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE:
		configuration = LVDS_CONFIG_ID_CHID;
		break;
	case DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE2:
		configuration = LVDS_CONFIG_ID_CHID2;
		break;
	case DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE3:
		configuration = LVDS_CONFIG_ID_CHID3;
		break;
	case DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE4:
		configuration = LVDS_CONFIG_ID_CHID4;
		break;
	case DAL_SSENIFLVDS_CFGID_CLANE_SWITCH:
		//if (glvds_obj->get_config(LVDS_CONFIG_ID_CLANE_SWITCH) == TRUE) {
		//	return DAL_SSENIF_CFGID_CLANE_LVDS_USE_C2C6;
		//} else {
			return DAL_SSENIF_CFGID_CLANE_LVDS_USE_C0C4;
		//}

	case DAL_SSENIFLVDS_CFGID_SENSOR_VD_CNT:
		configuration = LVDS_CONFIG_ID_VD_CNT;
		break;

	default:
		DBG_ERR("Invalid ID 0x%08X\r\n", config_id);
		return 0;
	}

	return glvds_obj->get_config(configuration);

}

//void ssenif_set_laneconfig_lvds(DAL_SSENIFLVDS_LANECFGID config_id, DAL_SSENIF_LANESEL lane_select, UINT32 value)
void DALLVDS_API_DEFINE(set_laneconfig)(DAL_SSENIFLVDS_LANECFGID config_id, DAL_SSENIF_LANESEL lane_select, UINT32 value)
{
	LVDS_CH_CONFIG_ID configuration;

	if (glvds_obj == NULL) {
		DBG_ERR("no init\r\n");
		return;
	}

	switch (config_id) {
	case DAL_SSENIFLVDS_LANECFGID_CTRLWORD_HD:
		configuration = LVDS_CONFIG_CTRLWORD_HD;
		break;
	case DAL_SSENIFLVDS_LANECFGID_CTRLWORD_VD:
		configuration = LVDS_CONFIG_CTRLWORD_VD;
		break;
	case DAL_SSENIFLVDS_LANECFGID_CTRLWORD_VD2:
		configuration = LVDS_CONFIG_CTRLWORD_VD2;
		break;
	case DAL_SSENIFLVDS_LANECFGID_CTRLWORD_VD3:
		configuration = LVDS_CONFIG_CTRLWORD_VD3;
		break;
	case DAL_SSENIFLVDS_LANECFGID_CTRLWORD_VD4:
		configuration = LVDS_CONFIG_CTRLWORD_VD4;
		break;

	case DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT:
		ssenif_set_default_sync(lane_select);
		return;

	default:
		DBG_ERR("Invalid ID 0x%08X\r\n", config_id);
		return;
	}

	glvds_obj->set_padlane_config((LVDS_IN_VALID)lane_select, configuration, value);

}








