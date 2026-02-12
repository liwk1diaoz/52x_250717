/*
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    @file       kdrv_vdocdc_main.h

    @brief      kdrv_vdocdc main header file

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2019/01/21
*/
#ifndef _KDRV_VDOCDC_MAIN_H
#define _KDRV_VDOCDC_MAIN_H

#include <linux/cdev.h>

//#define H26XE_VER_STR			"0.2.45"	/*add initial pq parameters*/
//#define H26XE_VER_STR			"0.3.0"		/*support motion ratio & jnd*/
//#define H26XE_VER_STR			"0.3.1"		/*(2020/05/12): update tile settings. remove unused condition.
//									correct search range setting for h264
//									correct h/w limitation definition for 520 and add h/w limitation definition for 528
//									update register map to 528 format
//									support osg chroma alpha for 528 */
//#define H26XE_VER_STR			"0.3.2"		/*(2020/05/19): update h/w limitation description
//									support 528 feature */
//#define H26XE_VER_STR			"0.3.3"	/*(2020/05/27): set rrc ndqp and rrc stop = 0.05 */
//#define H26XE_VER_STR			"0.3.5"	/*(2020/06/08): support dynamic change gop */
//#define H26XE_VER_STR			"0.3.7"	/*(2020/10/20): support fix bistream buffer size 128 bytes alignment (IVOT_N01004_CO-822)*/
//#define H26XE_VER_STR			"0.3.8"	/*(2020/10/29): support slice long start code (IVOT_N01004_CO-301)*/
//#define H26XE_VER_STR			"0.3.9"	/*(2021/03/15): sync svc, ltr patch(NA51089-244), and fix MMCO=1 case (IVOT_N12087_CO-132/IVOT_N12099_CO-407)*/
//#define H26XE_VER_STR			"0.3.10"	/*(2021/03/16): fix sw reset (IVOT_N12087_CO-148)*/
//#define H26XE_VER_STR			"0.3.11"	/*(2021/11/10): add h264 vui max_dec_frame_buf (IVOT_N12070_CO-449)*/
//#define H26XE_VER_STR			"0.3.12"	/*(2022/02/21): support sei payload for bs checksum (IVOT_N12089_CO-1350)*/
//#define H26XE_VER_STR			"0.3.13"	/*(2022/03/02): update gdr behavior (IVOT_N12089_CO-1245)*/
//#define H26XE_VER_STR			"0.3.14"	/*(2022/07/15): support LPM mode setting (IVOT_N12038_CO-241)*/
//#define H26XE_VER_STR			"0.3.15"	/*(2022/08/05): support frame crop vendor cmd (IVOT_N12069_CO-336) */
//#define H26XE_VER_STR			"0.3.16"	/*(2022/08/29): provide osd position align limit definition (IVOT_N12037_CO-937) */
#define H26XE_VER_STR			"0.3.17"	/*(2023/04/25): support bsout buffer full re-trigger (NA51084-597) */



#define MODULE_NAME			"kdrv_h26x"
#define MODULE_MINOR_ID		0
#define MODULE_MINOR_COUNT	1

typedef struct kdrv_vdocdc_drv_info {
	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct cdev cdev;
	dev_t dev_id;
} KDRV_VDOCDC_DRV_INFO;

#endif	// _KDRV_VDOCDC_MAIN_H

