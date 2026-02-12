/**
	@brief Header file of vendor vpe module.\n
	This file contains the functions which is related to IQ/3A in the chip.

	@file hd_videocapture.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include <stdio.h>
#include <string.h>
#include <kwrap/type.h>
#include "kwrap/error_no.h"
#include "kwrap/util.h"
#include <kwrap/cpu.h>
#include "kwrap/list.h"
#include "vendor_eis.h"
#include "eis_ioctl.h"
#include "hd_type.h"
#include "hd_util.h"
#include "eis_rsc_lib.h"
#include <math.h>
#include "vendor_isp.h"
#if defined(__LINUX)
#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>
#endif
#if defined(__FREERTOS)
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#endif

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          VENDOR_EIS
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"

#if defined(__LINUX)
#include <sys/mman.h>
#include <pthread.h>
#endif
#if defined(__FREERTOS)
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#endif

#define LINK_TO_EIS_RSC_LIB 1
#define PERF_TIME 0
/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define EIS_OPEN eis_open
#define EIS_IOCTL eis_ioctl
#define EIS_CLOSE eis_close
#endif
#if defined(__LINUX)
#define EIS_OPEN  open
#define EIS_IOCTL ioctl
#define EIS_CLOSE close
#endif

#define EIS_2DLUT_BUF_TAG MAKEFOURCC('E','I','S','_')
#define EIS_2DLUT_DEBUG_SIZE sizeof(UINT32)

#define VENDOR_EIS_VER 0x20230926

#define USE_GYRO_TEST_PAT 0
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/
#define EIS_TASK_DBG_CMD_ID   1000

#define EIS_TASK_DBG_GYRO_INPUT    0x00000001
#define EIS_TASK_DBG_GYRO_RESULT   0x00000002

static UINT32 eis_dbg_flag = 0;
/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static INT32 eis_fd = -1;
static pthread_t  vendor_eis_thread_id = 0;

static UINT32 lut2d_buf_size;// = 65*65*sizeof(UINT32);
static CHAR *lut2d_buf = NULL;
#if EIS_DEBUG_INFO
static UINT32 gyro_data_num = MAX_GYRO_DATA_NUM;
#endif
static EIS_RSC_PATH_INFO path_info[PATH_NUM] = {0};

static EIS_PATH_INFO eis_path_info ={0};

static VENDOR_EIS_USER_DATA_CB user_cb = NULL;

static UINT32 gyro_latency = 0;
typedef struct vos_list_head EIS_LIST_HEAD;
typedef struct _VENDOR_EIS_OUT {
#if EIS_DEBUG_INFO
	EIS_DBG_CTX dbg_ctx;
	UINT32 angular_rate_x[MAX_GYRO_DATA_NUM];
	UINT32 angular_rate_y[MAX_GYRO_DATA_NUM];
	UINT32 angular_rate_z[MAX_GYRO_DATA_NUM];
	UINT32 acceleration_rate_x[MAX_GYRO_DATA_NUM];
	UINT32 acceleration_rate_y[MAX_GYRO_DATA_NUM];
	UINT32 acceleration_rate_z[MAX_GYRO_DATA_NUM];
	UINT32 gyro_timestamp[MAX_GYRO_DATA_NUM];
#else
	UINT64 count;					///< frame count
#endif
	EIS_LIST_HEAD list;
} VENDOR_EIS_OUT;

typedef struct _VENDOR_EIS_Q_INFO {
	EIS_LIST_HEAD free_head;
	EIS_LIST_HEAD used_head;
	VENDOR_EIS_OUT out[MAX_LATENCY_NUM];
} VENDOR_EIS_Q_INFO;

static VENDOR_EIS_Q_INFO vendor_eis_q[PATH_NUM] = {0};

#if USE_GYRO_TEST_PAT
#define GYRO_FXPT_PREC (1 << 16)

#define GYRO_DATA_NUM 60

static FLOAT gyro_test_float_data[3][GYRO_DATA_NUM] = {
	{
		0, -0.00610865, -0.0122173, -0.0183259, -0.0232129, -0.0244346, -0.0256563, -0.0244346, -0.0244346, -0.0244346,
		-0.0244346, -0.0256563, -0.0219911, -0.0183259, -0.0146608, -0.00977384, -0.00855211, -0.00488692, -0.00244346,
		0.00122173, 0.00610865, 0.00977384, 0.0158825, 0.0207694, 0.0207694, 0.0232129, 0.0232129, 0.0219911, 0.0232129,
		0.0207694, 0.0195477, 0.0146608, 0.00855211, 0.00244346, -0.00488692, -0.00733038, -0.013439, -0.0158825, -0.0183259,
		-0.0244346, -0.026878, -0.031765, -0.0378736, -0.0390953, -0.0415388, -0.0378736, -0.0403171, -0.0439823, -0.0439823,
		-0.045204, -0.045204, -0.0439823, -0.0439823, -0.0427605, -0.0427605, -0.0427605, -0.0403171, -0.0366519, -0.0342084,
		-0.0280998
	},

	{
		0.0354302, 0.0329867, 0.0329867, 0.0293215, 0.0305432, 0.031765, 0.0305432, 0.0329867, 0.0329867, 0.031765, 0.031765,
		0.031765, 0.0305432, 0.0280998, 0.0256563, 0.0219911, 0.0219911, 0.0195477, 0.0171042, 0.0158825, 0.0122173, 0.00977384,
		0.0122173, 0.013439, 0.0122173, 0.00855211, 0.0109956, 0.00977384, 0.00855211, 0.00977384, 0.00733038, 0.00610865,
		0.00244346, 0.00122173, 0.00244346, 0.00122173, 0.00122173, 0.00244346, 0.00488692, 0.00366519, 0.00366519, 0.00488692,
		0.00855211, 0.00733038, 0.00855211, 0.00855211, 0.00733038, 0.00855211, 0.00733038, 0.00855211, 0.00855211, 0.00855211,
		0.00855211, 0.00855211, 0.00855211, 0.0109956, 0.0146608, 0.0146608, 0.0207694, 0.0232129
	},

	{
		0.00244346, 0.00122173, 0.00244346, 0.00488692, 0.00366519, 0.00366519, 0.00488692, 0.00488692, 0.00488692, 0.00488692,
		0.00488692, 0.00488692, 0.00366519, 0.00122173, 0.00122173, -0.00244346, -0.00488692, -0.00733038, -0.00733038, -0.00855211,
		-0.00855211, -0.00733038, -0.00733038, -0.00366519, -0.00122173, 0, -0.00122173, -0.00122173, -0.00244346, -0.00244346,
		-0.00366519, -0.00122173, -0.00122173, -0.00122173, -0.00122173, -0.00122173, -0.00244346, -0.00488692, -0.00610865,
		-0.00855211, -0.00977384, -0.00855211, -0.00977384, -0.00733038, -0.00488692, -0.00244346, 0, 0.00366519, 0.00610865,
		0.00610865, 0.00733038, 0.00855211, 0.0109956, 0.0122173, 0.013439, 0.0146608, 0.0183259, 0.0219911, 0.0232129, 0.0256563
	}
};
static INT32 gyro_test_int_data[3][GYRO_DATA_NUM];
#endif
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
//Note that the following two API only for single entry
static void _vendor_eis_get_frame(UINT32 id, EIS_PUSH_DATA *p_push_data)
{
	VENDOR_EIS_OUT *p_event = NULL;

	if (!vos_list_empty(&vendor_eis_q[id].used_head)) {
		p_event = vos_list_entry(vendor_eis_q[id].used_head.next, VENDOR_EIS_OUT, list);
		vos_list_del(&p_event->list);
	}

	if (p_event) {
		vos_list_add_tail(&p_event->list, &vendor_eis_q[id].free_head);

		#if EIS_DEBUG_INFO
		{
			UINT32 dst = (UINT32)p_push_data->lut2d_buf + p_push_data->buf_size;
			UINT32 data_size = gyro_data_num*sizeof(UINT32);
			UINT32 dbg_ctx_size = sizeof(p_event->dbg_ctx);

			p_push_data->frame_count = p_event->dbg_ctx.frame_count;
			p_push_data->frame_time = p_event->dbg_ctx.t_diff_crop;
			memcpy((void *)dst, (void *)&p_event->dbg_ctx, sizeof(p_event->dbg_ctx));
			dst += dbg_ctx_size;

			memcpy((void *)dst, (void *)p_event->angular_rate_x, data_size);
			dst += data_size;
			memcpy((void *)dst, (void *)p_event->angular_rate_y, data_size);
			dst += data_size;
			memcpy((void *)dst, (void *)p_event->angular_rate_z, data_size);
			dst += data_size;
			memcpy((void *)dst, (void *)p_event->acceleration_rate_x, data_size);
			dst += data_size;
			memcpy((void *)dst, (void *)p_event->acceleration_rate_y, data_size);
			dst += data_size;
			memcpy((void *)dst, (void *)p_event->acceleration_rate_z, data_size);
			dst += data_size;
			memcpy((void *)dst, (void *)p_event->gyro_timestamp, data_size);

			p_push_data->buf_size += (6*data_size + dbg_ctx_size);
		}
		#else
		p_push_data->frame_count = p_event->count;
		#endif
	} else {
		DBG_WRN("vendor queue[%d] empty!\r\n", id);
	}
	//DBG_DUMP("[%d]GET FRAME %llu\r\n", id, p_push_data->frame_count);
}
static void _vendor_eis_put_frame(UINT32 id, EIS_WAIT_PROC_INFO *p_frame, UINT32 exp_time)
{
	VENDOR_EIS_OUT *p_event = NULL;

	if (id >= PATH_NUM) {
		DBG_ERR("path id(%d) error\r\n", id);
		return;
	}
	if (!vos_list_empty(&vendor_eis_q[id].free_head)) {
		p_event = vos_list_entry(vendor_eis_q[id].free_head.next, VENDOR_EIS_OUT, list);
		vos_list_del(&p_event->list);
	}

	if (NULL == p_event) {
		DBG_WRN("[%d]queue full!(%llu)\r\n", id, p_frame->frame_count);
		return;
	}

	//DBG_DUMP("[%d]PUT FRAME %llu\r\n", id, frame_cnt);


	#if EIS_DEBUG_INFO
	{
		p_event->dbg_ctx.frame_count = p_frame->frame_count;
		p_event->dbg_ctx.t_diff_crop = p_frame->t_diff_crop;
		p_event->dbg_ctx.t_diff_crp_end_to_vd = p_frame->t_diff_crp_end_to_vd;
		p_event->dbg_ctx.exposure_time = exp_time;
		p_event->dbg_ctx.gyro_data_num = gyro_data_num;
		p_event->dbg_ctx.vendor_eis_ver = VENDOR_EIS_VER;
		eis_rsc_get(EIS_RSC_PARAM_VERSION_INFO, &p_event->dbg_ctx.eis_rsc_ver);

		memcpy(p_event->angular_rate_x, p_frame->angular_rate_x, sizeof(p_event->angular_rate_x));
		memcpy(p_event->angular_rate_y, p_frame->angular_rate_y, sizeof(p_event->angular_rate_y));
		memcpy(p_event->angular_rate_z, p_frame->angular_rate_z, sizeof(p_event->angular_rate_z));
		memcpy(p_event->acceleration_rate_x, p_frame->acceleration_rate_x, sizeof(p_event->acceleration_rate_x));
		memcpy(p_event->acceleration_rate_y, p_frame->acceleration_rate_y, sizeof(p_event->acceleration_rate_y));
		memcpy(p_event->acceleration_rate_z, p_frame->acceleration_rate_z, sizeof(p_event->acceleration_rate_z));
		memcpy(p_event->gyro_timestamp, p_frame->gyro_timestamp, sizeof(p_event->gyro_timestamp));
	}
	#else
	p_event->count = p_frame->frame_count;
	#endif
	vos_list_add_tail(&p_event->list, &vendor_eis_q[id].used_head);
	#if 0
	{
		UINT32 queue_num = 0;
		EIS_LIST_HEAD *p_list;
		if (!vos_list_empty(&vendor_eis_q[id].used_head)) {
			vos_list_for_each(p_list, &vendor_eis_q[id].used_head) {
				p_event = vos_list_entry(p_list, VENDOR_EIS_OUT, list);
				DBG_DUMP("%#.8lx%12llu\r\n", (uintptr_t)p_event, p_event->count);
				queue_num++;
			}
		}
		DBG_DUMP("[%d]queue num = %d\r\n", id, queue_num);
	}
	#endif

}
static void _vendor_eis_reset_queue(void)
{
	UINT32 i, id;
	VENDOR_EIS_OUT *free_event;

	memset(vendor_eis_q, 0, sizeof(vendor_eis_q));
	for (id = 0; id < PATH_NUM; id++) {
		/* init free & proc list head */
		VOS_INIT_LIST_HEAD(&vendor_eis_q[id].free_head);
		VOS_INIT_LIST_HEAD(&vendor_eis_q[id].used_head);
		for (i = 0; i < MAX_LATENCY_NUM; i++) {
			free_event = &vendor_eis_q[id].out[i];
			//DBG_DUMP("[%d]%#.8x%12llu\r\n", id, (uintptr_t)free_event, free_event->count);
			vos_list_add_tail(&free_event->list, &vendor_eis_q[id].free_head);
		}
	}
}

 BOOL check_buf_valid(ULONG buf, UINT32 size)
{
	UINT32 tag;

	//DBG_DUMP("%s:buf=0x%lX, size=%d, tag=0x%X\r\n", __func__, (ULONG)buf, size, *(UINT32 *)(buf + size));

	memcpy((void *)&tag, (void *)(buf + size), EIS_2DLUT_DEBUG_SIZE);
	if (tag == EIS_2DLUT_BUF_TAG) {
		return TRUE;
	} else {
		return FALSE;
	}
}
 void add_buf_tag(ULONG buf, UINT32 size)
{
	UINT32 tag = EIS_2DLUT_BUF_TAG;

	//DBG_DUMP("%s:buf=0x%lX, size=%d, tag=0x%X\r\n", __func__, (ULONG)buf, size, EIS_2DLUT_BUF_TAG);
	memcpy((void *)(buf + size), (void *)&tag, EIS_2DLUT_DEBUG_SIZE);
}

static void _vendor_eis_dbg_cmd(UINT32 id, INT32 value)
{
	BOOL func_switch = (BOOL)value;
	UINT32 get_value;

	DBG_DUMP("_vendor_eis_dbg_cmd:%d %d\r\n", id, value);
	switch(id) {
	//0~49
	case 0: {
		eis_rsc_set(EIS_RSC_PARAM_SHOW_LIB_PARAM, &func_switch);
		break;
	}
	case 1: {
		eis_rsc_set(EIS_RSC_PARAM_SHOW_IMU_DATA, &value);
		break;
	}
	case 2: {
		eis_rsc_set(EIS_RSC_PARAM_SHOW_CAM_BODY_RATE, &value);
		break;
	}
	case 3: {
		eis_rsc_set(EIS_RSC_PARAM_SHOW_ACTUAL_FRM_CENTER_QUAT, &value);
		break;
	}
	case 4: {
		eis_rsc_set(EIS_RSC_PARAM_SHOW_ACTUAL_LINE_QUAT, &value);
		break;
	}
	case 5: {
		eis_rsc_set(EIS_RSC_PARAM_SHOW_STABLE_FRM_CENTER_QUAT, &value);
		break;
	}
	case 6: {
		eis_rsc_set(EIS_RSC_PARAM_SHOW_LINE_COMPEN_QUAT, &value);
		break;
	}
	case 7: {
		eis_rsc_set(EIS_RSC_PARAM_SHOW_WARPING_TABLE, &func_switch);
		break;
	}
	case 8: {
		eis_rsc_set(EIS_RSC_PARAM_SHOW_2DLUT_CHECKSUM, &func_switch);
		break;
	}
	case 9: {
		eis_rsc_set(EIS_RSC_PARAM_SHOW_EXPOSURE_TIME, &func_switch);
		break;
	}
	case 10: {
		eis_rsc_set(EIS_RSC_PARAM_SHOW_FRAME_TIMESTAMP, &func_switch);
		break;
	}

	//50~79
	case 50: {
		eis_rsc_set(EIS_RSC_PARAM_COMPEN_VERIF_ENABLE, &func_switch);
		break;
	}
	case 51: {
		eis_rsc_set(EIS_RSC_PARAM_UNDISTORT_OUTPUT, &func_switch);
		break;
	}
	case 52: {
		eis_rsc_set(EIS_RSC_PARAM_ADJUST_GYRO_TIME_SHIFT, &value);
		break;
	}
	case 53: {
		eis_rsc_set(EIS_RSC_PARAM_ADJUST_ROLL_SHUT_READOUT_TIME, &value);
		break;
	}
	case 54: {
		eis_rsc_set(EIS_RSC_PARAM_ADJUST_FOCAL_LENGTH, &value);
		break;
	}
	case 55: {
		eis_rsc_set(EIS_RSC_PARAM_ADJUST_DISTORT_CENTER_X, &value);
		break;
	}
	case 56: {
		eis_rsc_set(EIS_RSC_PARAM_ADJUST_DISTORT_CENTER_Y, &value);
		break;
	}
	case 57: {
		eis_rsc_set(EIS_RSC_PARAM_ADJUST_IMU_SYNC_SHIFT_EXPO_TIME_THRES, &value);
		break;
	}
	case 58: {
		eis_rsc_set(EIS_RSC_PARAM_ADJUST_IMU_SYNC_SHIFT_EXPO_TIME_PRECENT, &value);
		break;
	}
	case 59: {
		eis_rsc_set(EIS_RSC_PARAM_SET_STABLE_TRAJECTORY_COEFF_1, &value);
		break;
	}
	case 60: {
		eis_rsc_set(EIS_RSC_PARAM_SET_STABLE_TRAJECTORY_COEFF_2, &value);
		break;
	}
	case 61: {
		eis_rsc_set(EIS_RSC_PARAM_SET_STABLE_TRAJECTORY_COEFF_3, &value);
		break;
	}

	//80~99
	case 80: {
		eis_rsc_set(EIS_RSC_PARAM_DISABLE_ROLL_SHUT_CORRECT, &func_switch);
		break;
	}
	case 81: {
		eis_rsc_set(EIS_RSC_PARAM_DISABLE_STABILIZATION, &func_switch);
		break;
	}
	case 82: {
		eis_rsc_set(EIS_RSC_PARAM_DISABLE_WARPING_TABLE_CALCULATION, &func_switch);
		break;
	}
	case 83: {
		eis_rsc_set(EIS_RSC_PARAM_DISABLE_CAM_MODEL_UNDISTORT, &func_switch);
		break;
	}

    //100
	case 100: {
		eis_rsc_get(EIS_RSC_PARAM_VERSION_INFO, &get_value);
		DBG_DUMP("EIS version: %X", get_value);
		break;
	}

	//1000
	case EIS_TASK_DBG_CMD_ID: {
		eis_dbg_flag = value;
		DBG_DUMP("eis_dbg_flag: %X\r\n", eis_dbg_flag);
		break;
	}
	default:
		DBG_ERR("unknown dbg cmd(%d)\r\n", id);
		break;
	}
}

static void *_vendor_eis_tsk(void *arg)
{
	BOOL is_continue = 1;
	int ret;
	static EIS_WAIT_PROC_INFO proc_info;
	BOOL proc_ret = FALSE;
	EIS_RSC_PROC_INFO eis_proc_info;
	UINT32 i;
	ISPT_SENSOR_EXPT sensor_expt = {0};
	EIS_RSC_IMU_TIMESTAMP_INFO imu_timestamp;
	EIS_RSC_FRAME_LINE_TIMESTAMP_INFO frm_timestamp;

	DBG_IND("++\r\n");
#if LINK_TO_EIS_RSC_LIB
	if (lut2d_buf == NULL) {
		//temp using max size to alloc buffer
		lut2d_buf_size = eis_rsc_get_2dlut_buffer_size(LUT_65x65) + EIS_2DLUT_DEBUG_SIZE;
		lut2d_buf_size += EIS_DEBUG_SIZE;
		lut2d_buf = (CHAR *)malloc(lut2d_buf_size);
		if (lut2d_buf == NULL) {
			DBG_ERR("fail to allocate memory(%d) for lut2d_buf!\n", lut2d_buf_size);
			return 0;
		}
		DBG_DUMP("lut2d_buf=0x%lX, size=%d\r\n", (ULONG)lut2d_buf, lut2d_buf_size);
	}
#endif
	while (is_continue) {
		memset((void *)(&proc_info), 0, sizeof(proc_info));
		proc_info.wait_ms = -1;
		ret = EIS_IOCTL(eis_fd, EIS_IOC_WAIT_PROC, &proc_info);
		if (ret < 0) {
			DBG_ERR("WAIT_PROC error(%d)\r\n", ret);
	        continue;
	    }
		if (proc_info.result == EIS_E_ABORT) {
			is_continue = 0;
			continue;
		}
		if (proc_info.result == EIS_E_DEBUG) {
			//debug cmd
			_vendor_eis_dbg_cmd(proc_info.angular_rate_x[0], proc_info.angular_rate_x[1]);
			continue;
		}
		//update eis latency
		if (proc_info.result == EIS_E_GYRO_LATENCY) {
			gyro_latency = proc_info.angular_rate_x[0];
			DBG_DUMP("eis gyro_latency=%d\r\n", gyro_latency);
			continue;
		}


		//DBG_DUMP("FRAME(%llu) t_diff_crop=%llu t_diff_crp_end_to_vd=%llu\r\n", proc_info.frame_count, proc_info.t_diff_crop, proc_info.t_diff_crp_end_to_vd);
		#if 0
		{
			UINT32 k, gyro_size;
			UINT32 check_sum = 0;
			UINT8 *p_data;
			#if EIS_DEBUG_INFO
			UINT32 data_num = gyro_data_num;
			#else
			UINT32 data_num = MAX_GYRO_DATA_NUM;
			#endif

			gyro_size = data_num*sizeof(UINT32);
			p_data = (UINT8 *)proc_info.angular_rate_x;
			for (k = 0; k < gyro_size; k++) {
				check_sum += *(p_data+k);
			}
			p_data = (UINT8 *)proc_info.angular_rate_y;
			for (k = 0; k < gyro_size; k++) {
				check_sum += *(p_data+k);
			}
			p_data = (UINT8 *)proc_info.angular_rate_z;
			for (k = 0; k < gyro_size; k++) {
				check_sum += *(p_data+k);
			}
			p_data = (UINT8 *)proc_info.acceleration_rate_x;
			for (k = 0; k < gyro_size; k++) {
				check_sum += *(p_data+k);
			}
			p_data = (UINT8 *)proc_info.acceleration_rate_y;
			for (k = 0; k < gyro_size; k++) {
				check_sum += *(p_data+k);
			}
			p_data = (UINT8 *)proc_info.acceleration_rate_z;
			for (k = 0; k < gyro_size; k++) {
				check_sum += *(p_data+k);
			}
			p_data = (UINT8 *)proc_info.gyro_timestamp;
			for (k = 0; k < gyro_size; k++) {
				check_sum += *(p_data+k);
			}
			DBG_DUMP("[%llu]Gyro sum=0x%X size=%d\r\n",proc_info.frame_count, check_sum, 6*gyro_size);
		}
		#endif


	    if (eis_dbg_flag & EIS_TASK_DBG_GYRO_INPUT) {
			UINT32 n;

			DBG_DUMP("FRAME(%llu) crop start=%llu end=%llu\r\n", proc_info.frame_count, proc_info.t_diff_crop, proc_info.t_diff_crp_end_to_vd);
			#if 1
			for (n = 0; n < proc_info.data_num; n++) {
				DBG_DUMP("[%d]%d %d %d %d %d %d %d\r\n", n, proc_info.angular_rate_x[n], proc_info.angular_rate_y[n], proc_info.angular_rate_z[n]
												, proc_info.acceleration_rate_x[n], proc_info.acceleration_rate_x[n], proc_info.acceleration_rate_x[n]
												, proc_info.gyro_timestamp[n]);

			}
			#else
			for (n = 0; n < proc_info.data_num; n++) {
				DBG_DUMP("[%d]%d %d %d %d\r\n", n, proc_info.angular_rate_x[n], proc_info.angular_rate_y[n], proc_info.angular_rate_z[n], proc_info.gyro_timestamp[n]);
			}
			#endif
		}

		#if 0
	    {
	    	UINT32 i;
	    	DBG_DUMP("proc_info wait_ms(%d) result(%d) frm_cnt(%llu)\r\n", proc_info.wait_ms, proc_info.result, proc_info.frame_count);
	    	DBG_DUMP("=== angular_rate_x ===\r\n");
	    	for (i = 0; i < MAX_GYRO_DATA_NUM; i++) {
				DBG_DUMP("%08X ", proc_info.angular_rate_x[i]);
			}
			DBG_DUMP("\r\n=== angular_rate_y ===\r\n");
			for (i = 0; i < MAX_GYRO_DATA_NUM; i++) {
				DBG_DUMP("%08X ", proc_info.angular_rate_y[i]);
			}
			DBG_DUMP("\r\n=== acceleration_rate_z ===\r\n");
			for (i = 0; i < MAX_GYRO_DATA_NUM; i++) {
				DBG_DUMP("%08X ", proc_info.acceleration_rate_z[i]);
			}
			DBG_DUMP("\r\n=== gyro_timestamp ===\r\n");
			for (i = 0; i < MAX_GYRO_DATA_NUM; i++) {
				DBG_DUMP("%08X ", proc_info.gyro_timestamp[i]);
			}
			DBG_DUMP("\r\n");
		}
		#endif
		#if 0
	    {
			UINT32 i;
			UINT32 *p_tmp;

			DBG_DUMP("=== user data(%d) ===\r\n", proc_info.user_data_size);
			p_tmp = (UINT32 *)proc_info.user_data;

			for (i = 0; i < proc_info.user_data_size/4; i++) {
				DBG_DUMP("%08X ", *(p_tmp + i));
			}
			DBG_DUMP("\r\n");
		}
		#endif

		#if 0
		eis_rsc_set(EIS_RSC_PARAM_SIE_CROP_START_END_TIME, &proc_info.t_diff_crop);
		eis_rsc_set(EIS_RSC_PARAM_SIE_CROP_END_TO_VD_TIME, &proc_info.t_diff_crp_end_to_vd);
		#else
		frm_timestamp.first_line_timestamp = (UINT32)proc_info.t_diff_crop;
		frm_timestamp.last_line_timestamp = (UINT32)proc_info.t_diff_crp_end_to_vd;
		frm_timestamp.time_precision = pow(10,6);
		eis_rsc_set(EIS_RSC_PARAM_FRAME_LINE_TIMESTAMP, &frm_timestamp);
		imu_timestamp.imu_num_per_frm = proc_info.data_num;
		imu_timestamp.timestamp = proc_info.gyro_timestamp;
		imu_timestamp.time_precision = pow(10,6);
		imu_timestamp.frm_imu_latency = (INT32)gyro_latency;
		eis_rsc_set(EIS_RSC_PARAM_IMU_TIMESTAMP, &imu_timestamp);
		#endif
		if (proc_info.result != EIS_E_OK) {
			DBG_ERR("WAIT_PROC failed(%d)\r\n", proc_info.result);
			continue;
		}
		//trigger eis lib
		for (i = 0; i < PATH_NUM; i++) {
			if (path_info[i].frame_size.w && path_info[i].frame_size.h) {
				memset((void *)(&eis_proc_info), 0, sizeof(eis_proc_info));
				eis_proc_info.path_id = i;
				eis_proc_info.frame_count = proc_info.frame_count;
				sensor_expt.id = 0;
				vendor_isp_get_common(ISPT_ITEM_SENSOR_EXPT, &sensor_expt);
				//printf("exposure time (us) = %d, %d \n", sensor_expt.time[0], sensor_expt.time[1]);
				if (sensor_expt.time[0]) {
					eis_proc_info.frame_exposure_time = sensor_expt.time[0];
				} else {
					eis_proc_info.frame_exposure_time = 33333;
				}
				#if USE_GYRO_TEST_PAT
				eis_proc_info.angular_rate[0] = gyro_test_int_data[0];
				eis_proc_info.angular_rate[1] = gyro_test_int_data[1];
				eis_proc_info.angular_rate[2] = gyro_test_int_data[2];
				eis_proc_info.acceleration_rate[0] = NULL;
				eis_proc_info.acceleration_rate[1] = NULL;
				eis_proc_info.acceleration_rate[2] = NULL;
				#else
				eis_proc_info.angular_rate[0] = (INT32 *)proc_info.angular_rate_x;
				eis_proc_info.angular_rate[1] = (INT32 *)proc_info.angular_rate_y;
				eis_proc_info.angular_rate[2] = (INT32 *)proc_info.angular_rate_z;
				eis_proc_info.acceleration_rate[0] = (INT32 *)proc_info.acceleration_rate_x;
				eis_proc_info.acceleration_rate[1] = (INT32 *)proc_info.acceleration_rate_y;
				eis_proc_info.acceleration_rate[2] = (INT32 *)proc_info.acceleration_rate_z;
				#endif
				eis_proc_info.lut2d.lut2d_buf = (UINT32 *)lut2d_buf;
#if LINK_TO_EIS_RSC_LIB
				eis_proc_info.lut2d.buf_size = eis_rsc_get_2dlut_buffer_size(path_info[i].lut2d_size_sel);
#endif
				if (eis_proc_info.lut2d.buf_size > (lut2d_buf_size - EIS_2DLUT_DEBUG_SIZE - EIS_DEBUG_SIZE)) {
					DBG_ERR("buf size overflow\r\n", proc_info.result);
					continue;
				}
				if (user_cb) {
					user_cb(i, (ULONG)proc_info.user_data, proc_info.user_data_size);
				}
				add_buf_tag((ULONG)eis_proc_info.lut2d.lut2d_buf, eis_proc_info.lut2d.buf_size);
#if LINK_TO_EIS_RSC_LIB
				{
					#if PERF_TIME
					UINT64 perf1, perf2;

					DBG_DUMP("[%d] eis proc++\r\n", i);
					perf1 = hd_gettime_us();
					#endif
					_vendor_eis_put_frame(i, &proc_info, eis_proc_info.frame_exposure_time);
					proc_ret = eis_rsc_process(&eis_proc_info);
					/*
					 *if (proc_info.frame_count % 200 == 0) {
					 *    DBG_DUMP("expo time (us) %d\r\n", sensor_expt.time[0]);
					 *}
					 */
					#if PERF_TIME
					perf2 = hd_gettime_us();
					DBG_DUMP("[%d] eis proc %lluus\r\n", i, perf2 - perf1);
					#endif
				}

#endif
				//DBG_DUMP("[%d] FRM%llu proc_ret(%d), valid=%d\r\n", i, eis_proc_info.frame_count, proc_ret, eis_proc_info.lut2d.valid);
				if (proc_ret == FALSE) {
					DBG_WRN("eis_rsc_process failed\r\n");
					continue;
				} else if (eis_proc_info.lut2d.valid) {
					EIS_PUSH_DATA push_data = {0};

					push_data.path_id = i;
					push_data.lut2d_buf = eis_proc_info.lut2d.lut2d_buf;
					push_data.buf_size = eis_proc_info.lut2d.buf_size;
					if (FALSE == check_buf_valid((ULONG)push_data.lut2d_buf, push_data.buf_size)) {
						DBG_ERR("path buf[%d] corrupted!\n", i);
				//		continue;
					}
					//it will update buf_size if there is debug info
					_vendor_eis_get_frame(i, &push_data);

					//check frame time
					if (push_data.frame_time != eis_proc_info.lut2d.frame_first_line_timestamp) {
						DBG_DUMP("Push frm%llu time=%d 2DLUT_time=%d\r\n", push_data.frame_count, push_data.frame_time, eis_proc_info.lut2d.frame_first_line_timestamp);
					}
					push_data.frame_time = eis_proc_info.lut2d.frame_first_line_timestamp;
					if (eis_dbg_flag & EIS_TASK_DBG_GYRO_RESULT) {
						DBG_DUMP("Push frm%llu time=%d\r\n", push_data.frame_count, push_data.frame_time);
					}
					#if 0
					{
						UINT32 k, size;
						UINT32 check_sum = 0;
						UINT8 *p_tmp;

						p_tmp = (UINT8 *)push_data.lut2d_buf;
						#if EIS_DEBUG_INFO
						size = push_data.buf_size - 6*gyro_data_num*sizeof(UINT32);
						#else
						size = push_data.buf_size;
						#endif
						for (k = 0; k < size; k++) {
							check_sum += *(p_tmp+k);
						}
						DBG_DUMP("[%llu]2D sum=0x%X size=%d\r\n",push_data.frame_count, check_sum, size);
					}
					#endif

					//DBG_DUMP("push FRM(%llu)\r\n",push_data.frame_count);
					#if 0//for debug only
					{
						INT32 *p_data;
						DBG_DUMP("out[%d] FRM(%llu) lut2d_buf=0x%lX size=%d\r\n", i, push_data.frame_count, push_data.lut2d_buf, push_data.buf_size);
						p_data = (INT32 *)push_data.lut2d_buf;
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
						p_data = (INT32 *)(push_data.lut2d_buf + 32);
						DBG_DUMP("addr=0x%X\r\n", (UINT32)p_data);
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
						DBG_DUMP("Last x words\r\n");
						p_data = (INT32 *)((UINT32)push_data.lut2d_buf + push_data.buf_size - 48*4);
						DBG_DUMP("addr=0x%X\r\n", (UINT32)p_data);
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
						p_data = (INT32 *)((UINT32)push_data.lut2d_buf + push_data.buf_size - 40*4);
						DBG_DUMP("addr=0x%X\r\n", (UINT32)p_data);
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
						p_data = (INT32 *)((UINT32)push_data.lut2d_buf + push_data.buf_size - 8*4);
						DBG_DUMP("addr=0x%X\r\n", (UINT32)p_data);
						DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																			*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					}
					#endif
					EIS_IOCTL(eis_fd, EIS_IOC_PUSH_DATA, &push_data);
				} else {
					//temp log for debug
					DBG_DUMP("path%d FRM%llu skip.\r\n", i, eis_proc_info.frame_count);
				}
			}
		}
	}
	DBG_IND("--\r\n");
	return 0;
}

HD_RESULT vendor_eis_open(VENDOR_EIS_OPEN_CFG *p_open_cfg)
{
	HD_RESULT ret = HD_OK;
	EIS_STATE eis_state = EIS_STATE_OPEN;
	EIS_RSC_OPEN_CFG eis_open_obj = {0};
	pthread_attr_t fd_attr;
	struct sched_param fd_param;
	//ISIZE map_size = {160, 1080};

#if USE_GYRO_TEST_PAT
	UINT32 i;
	for (i = 0; i < GYRO_DATA_NUM; i++) {
		gyro_test_int_data[0][i] = (INT32)round(gyro_test_float_data[0][i] * GYRO_FXPT_PREC);
		gyro_test_int_data[1][i] = (INT32)round(gyro_test_float_data[1][i] * GYRO_FXPT_PREC);
		gyro_test_int_data[2][i] = (INT32)round(gyro_test_float_data[2][i] * GYRO_FXPT_PREC);
	}
#endif
	if (sizeof(EIS_RSC_OPEN_CFG) != sizeof(VENDOR_EIS_OPEN_CFG)) {
		DBG_ERR("open cfg version not matched!\r\n");
		return HD_ERR_SYS;
	}

	if (eis_fd >= 0) {
		return HD_ERR_ALREADY_OPEN;
	}
	gyro_latency = 0;
	eis_fd = EIS_OPEN("/dev/nvt_eis", O_RDWR|O_SYNC);
	if (eis_fd < 0) {
		DBG_ERR("open /dev/nvt_eis error\r\n");
		goto _EIS_OPEN_FAILED;
	}


	//memcpy((void *)&eis_open_obj, (void *)p_open_cfg, sizeof(EIS_RSC_OPEN_CFG));
	memcpy((void *)eis_open_obj.cam_intrins.distor_center, (void *)p_open_cfg->cam_intrins.distor_center, sizeof(eis_open_obj.cam_intrins.distor_center));
	memcpy((void *)eis_open_obj.cam_intrins.distor_curve, (void *)p_open_cfg->cam_intrins.distor_curve, sizeof(eis_open_obj.cam_intrins.distor_curve));
	memcpy((void *)eis_open_obj.cam_intrins.undistor_curve, (void *)p_open_cfg->cam_intrins.undistor_curve, sizeof(eis_open_obj.cam_intrins.undistor_curve));
	eis_open_obj.cam_intrins.focal_length = p_open_cfg->cam_intrins.focal_length;
	eis_open_obj.cam_intrins.calib_img_size = p_open_cfg->cam_intrins.calib_img_size;
	eis_open_obj.cam_intrins.max_out_radius = p_open_cfg->cam_intrins.max_out_radius;
	eis_open_obj.imu_type = p_open_cfg->imu_type;
	eis_open_obj.gyro.sync_method = 1;
	memcpy((void *)eis_open_obj.gyro.axes_mapping, (void *)p_open_cfg->gyro.axes_mapping, sizeof(eis_open_obj.gyro.axes_mapping));
	eis_open_obj.gyro.sampling_rate = p_open_cfg->gyro.sampling_rate;
	eis_open_obj.gyro.unit_conv = p_open_cfg->gyro.unit_conv;

	memcpy((void *)eis_open_obj.accel.axes_mapping, (void *)p_open_cfg->accel.axes_mapping, sizeof(eis_open_obj.accel.axes_mapping));
	eis_open_obj.accel.sampling_rate = p_open_cfg->accel.sampling_rate;
	eis_open_obj.accel.unit_conv = p_open_cfg->accel.unit_conv;
	eis_open_obj.accel.accel_gain = p_open_cfg->accel.accel_gain;

	eis_open_obj.imu_sync_shift_exposure_time_threshold = p_open_cfg->imu_sync_shift_exposure_time_threshold;
	eis_open_obj.imu_sync_shift_exposure_time_precent = p_open_cfg->imu_sync_shift_exposure_time_precent;
	eis_open_obj.imu_timing_shift_adjust = p_open_cfg->imu_timing_shift_adjust;

    eis_open_obj.stable_profile = p_open_cfg->stable_profile;

#if LINK_TO_EIS_RSC_LIB
	if (FALSE == eis_rsc_open(&eis_open_obj)) {
		DBG_ERR("eis lib failed!\r\n");
		goto _EIS_OPEN_FAILED;
	}
#endif
	ret = EIS_IOCTL(eis_fd, EIS_IOC_SET_STATE, &eis_state);

    if (ret < 0) {
    	DBG_ERR("SET STATE %d failed!\r\n", eis_state);
        goto _EIS_OPEN_FAILED;

    }
    _vendor_eis_reset_queue();
	//create task
	pthread_attr_init(&fd_attr);
	fd_param.sched_priority = 92;
	pthread_attr_setschedpolicy(&fd_attr,SCHED_RR);
	pthread_attr_setschedparam(&fd_attr,&fd_param);
	pthread_attr_setinheritsched(&fd_attr,PTHREAD_EXPLICIT_SCHED);

	ret = pthread_create(&vendor_eis_thread_id, &fd_attr, _vendor_eis_tsk, (void *)NULL);
	if (ret < 0) {
		DBG_ERR("create thread fail? %d\r\n", ret);
		goto _EIS_OPEN_FAILED;
	}
	//pthread_setname_np(vendor_eis_thread_id, "_vendor_eis_tsk");
	return ret;

_EIS_OPEN_FAILED:
	EIS_CLOSE(eis_fd);
	eis_fd = -1;
	return HD_ERR_SYS;
}

HD_RESULT vendor_eis_close(void)
{
	HD_RESULT ret = HD_OK;
	EIS_STATE eis_state;

	if (eis_fd < 0) {
		return HD_ERR_NOT_OPEN;
	}
	eis_state = EIS_STATE_IDLE;
	ret = EIS_IOCTL(eis_fd, EIS_IOC_SET_STATE, &eis_state);
    if (ret < 0) {
    	DBG_ERR("SET STATE %d failed!\r\n", eis_state);
        return HD_ERR_SYS;
    }
	// destroy thread
	pthread_join(vendor_eis_thread_id, NULL);
	eis_state = EIS_STATE_CLOSE;
	ret = EIS_IOCTL(eis_fd, EIS_IOC_SET_STATE, &eis_state);
    if (ret < 0) {
    	DBG_ERR("SET STATE %d failed!\r\n", eis_state);
        return HD_ERR_SYS;
    }

#if LINK_TO_EIS_RSC_LIB
	if (FALSE == eis_rsc_close()) {
		DBG_ERR("eis lib failed!\r\n");
        return HD_ERR_SYS;
	}
#endif
    EIS_CLOSE(eis_fd);
	eis_fd = -1;

	//THREAD_DESTROY(VENDOR_EIS_TSK_ID);  //already call THREAD_RETURN() in task, do not call THREAD_DESTROY() here.
	vendor_eis_thread_id = 0;

	if (lut2d_buf != NULL) {
		free(lut2d_buf);
		lut2d_buf = NULL;
	}
	eis_dbg_flag = 0;
	return ret;
}


BOOL vendor_eis_set(VENDOR_EIS_PARAM_ID param_id, VOID *p_param)
{
	BOOL lib_ret = FALSE;

	if (eis_fd < 0) {
		return FALSE;
	}
#if LINK_TO_EIS_RSC_LIB

	switch(param_id) {
	case VENDOR_EIS_PARAM_PATH_INFO: {
		EIS_RSC_PATH_INFO *p_user = (EIS_RSC_PATH_INFO *)p_param;

		if (sizeof(EIS_RSC_PATH_INFO) != sizeof(VENDOR_EIS_PATH_INFO)) {
			DBG_ERR("param_id(%d) size not matched!\r\n", param_id);
			break;
		}
		if (p_user == NULL || p_user->path_id >= PATH_NUM) {
			DBG_ERR("p_param error!\r\n");
			break;
		}
		if (p_user->frame_latency > MAX_LATENCY_NUM) {
			p_user->frame_latency = MAX_LATENCY_NUM;
			DBG_WRN("only support max latency %d\r\n", MAX_LATENCY_NUM);
		}
		memcpy((void *)&path_info[p_user->path_id], p_user, sizeof(EIS_RSC_PATH_INFO));

		lib_ret = eis_rsc_set(EIS_RSC_PARAM_PATH_INFO, &path_info[p_user->path_id]);

		if (eis_fd < 0) {
			return FALSE;
		} else {
			HD_RESULT ret = HD_OK;

			eis_path_info.path_id = p_user->path_id;
			eis_path_info.lut2d_buf_size = eis_rsc_get_2dlut_buffer_size(p_user->lut2d_size_sel);
			eis_path_info.lut2d_size_sel = p_user->lut2d_size_sel;
			eis_path_info.frame_latency = p_user->frame_latency;
			ret = EIS_IOCTL(eis_fd, EIS_IOC_PATH_INFO, &eis_path_info);
		    if (ret < 0) {
		    	DBG_ERR("SET PATH_INFO failed!\r\n");
		        return FALSE;
		    }
	    }

		break;
	}
	case VENDOR_EIS_PARAM_USER_DATA_CB: {
		user_cb = (VENDOR_EIS_USER_DATA_CB)p_param;
		return TRUE;
	}
	case VENDOR_EIS_PARAM_FRAME_RATE: {
		lib_ret = eis_rsc_set(EIS_RSC_PARAM_FRAME_RATE, p_param);
		break;
	}
	case VENDOR_EIS_PARAM_IMU_SAMPLE_NUM_PER_FRAME: {
		UINT32 *p_user = (UINT32 *)p_param;

		if (*p_user > MAX_GYRO_DATA_NUM) {
			DBG_ERR("Only support max %d gyro data num!\r\n", MAX_GYRO_DATA_NUM);
	        return FALSE;
	    }
		#if EIS_DEBUG_INFO
		{
			HD_RESULT ret = HD_OK;
			UINT32 dbg_size = 0;

			gyro_data_num = *p_user;

			dbg_size = (gyro_data_num*6*sizeof(UINT32)) + sizeof(EIS_DBG_CTX);
			ret = EIS_IOCTL(eis_fd, EIS_IOC_DEBUG_SIZE, &dbg_size);
		    if (ret < 0) {
				DBG_ERR("SET DEBUG_SIZE failed!\r\n");
		        return FALSE;
		    }
	    }
		#endif
		lib_ret = eis_rsc_set(EIS_RSC_PARAM_IMU_SAMPLE_NUM_PER_FRAME, p_param);
		break;
	}
	case VENDOR_EIS_PARAM_ROTATE_TYPE: {
		lib_ret = eis_rsc_set(EIS_RSC_PARAM_ROTATE_TYPE, p_param);
		break;
	}
	case VENDOR_EIS_PARAM_FRAME_CNT_RESET: {
		lib_ret = eis_rsc_set(EIS_RSC_PARAM_FRAME_CNT_RESET, p_param);
		break;
	}
	case VENDOR_EIS_PARAM_UNDISTORT_OUTPUT: {
		lib_ret = eis_rsc_set(EIS_RSC_PARAM_UNDISTORT_OUTPUT, p_param);
		break;
	}
	case VENDOR_EIS_PARAM_SHOW_LIB_PARAM: {
		lib_ret = eis_rsc_set(EIS_RSC_PARAM_SHOW_LIB_PARAM, p_param);
		break;
	}
	case VENDOR_EIS_PARAM_SHOW_IMU_DATA: {
		lib_ret = eis_rsc_set(EIS_RSC_PARAM_SHOW_IMU_DATA, p_param);
		break;
	}
	case VENDOR_EIS_PARAM_SHOW_CAM_BODY_RATE: {
		lib_ret = eis_rsc_set(EIS_RSC_PARAM_SHOW_CAM_BODY_RATE, p_param);
		break;
	}
	case VENDOR_EIS_PARAM_SHOW_2DLUT_CHECKSUM: {
		lib_ret = eis_rsc_set(EIS_RSC_PARAM_SHOW_2DLUT_CHECKSUM, p_param);
		break;
	}
	case VENDOR_EIS_PARAM_SHOW_EXPOSURE_TIME: {
		lib_ret = eis_rsc_set(EIS_RSC_PARAM_SHOW_EXPOSURE_TIME, p_param);
		break;
	}
	default:
		DBG_ERR("unknown param_id(%d)\r\n", param_id);
		break;
	}
#endif
	return lib_ret;
}

UINT32 vendor_eis_buf_query(VENDOR_EIS_LUT_SIZE_SEL lut2d_size_sel)
{
	return eis_rsc_get_2dlut_buffer_size(lut2d_size_sel);
}

