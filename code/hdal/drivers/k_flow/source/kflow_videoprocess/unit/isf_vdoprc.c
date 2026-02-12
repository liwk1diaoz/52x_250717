#include "isf_vdoprc_int.h"
#if defined(__LINUX)
#include "linux/isf_vdoprc_drv.h"
#else
#include "rtos/isf_vdoprc_drv.h"
#endif
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoprc
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoprc_debug_level = NVT_DBG_WRN;
//module_param_named(isf_vdoprc_debug_level, isf_vdoprc_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_vdoprc_debug_level, "vdoprc debug level");
///////////////////////////////////////////////////////////////////////////////

#define DUMP_PARAM			DISABLE

#define DUMP_PERFORMANCE	DISABLE

#define DUMP_FRC			ENABLE
#define STRESS_TEST			DISABLE

/*

--------------------------------------------
//input
typedef struct {
	UINT32	buf_id;
	UINT32	data_addr;
	UINT32	rev;
} CTL_IPP_EVT;
--------------------------------------------
//output
typedef struct {
	CTL_IPP_OUT_PATH_ID pid;
	UINT32 buf_size;
	UINT32 buf_id;
	UINT32 buf_addr;
	VDO_FRAME vdo_frm;
} CTL_IPP_OUT_BUF_INFO;

--------------------------------------------
ctl_ipp_set(hdl, CTL_IPP_ITEM_APPLY, (void *)&apply_id);

ctl_ipp_ioctl(g_ctrl_hdl_list[hdl_idx], CTL_IPP_IOCTL_SNDEVT, (void *)&evt);
										<=== unit require alloc CTL_IPP_EVT.buf_id
                                                 unit require send in blk by CTL_IPP_EVT.data_addr

CTL_IPP_CBEVT_BUFIO --- CTL_IPP_BUF_IO_NEW <=== in IPP will fill CTL_IPP_OUT_BUF_INFO.p_id, buf_size,
                                                unit require new out blk
                                                unit require set CTL_IPP_OUT_BUF_INFO.buf_id, buf_addr, and vdo_frm
CTL_IPP_CBEVT_ENG_IME_ISR

CTL_IPP_CBEVT_PROCEND / CTL_IPP_CBEVT_DROP ===> in will pass CTL_IPP_EVT
											unit require release in blk
											unit require free CTL_IPP_EVT.buf_id

CTL_IPP_CBEVT_BUFIO --- CTL_IPP_BUF_IO_PUSH ===> in will pass CTL_IPP_OUT_BUF_INFO
											unit require push blk or release blk
											unit require free CTL_IPP_OUT_BUF_INFO.buf_id

CTL_IPP_CBEVT_BUFIO --- CTL_IPP_BUF_IO_LOCK ===> in will pass CTL_IPP_OUT_BUF_INFO
											unit require add blk
											unit require free CTL_IPP_OUT_BUF_INFO.buf_id

CTL_IPP_CBEVT_BUFIO --- CTL_IPP_BUF_IO_UNLOCK ===> in will pass CTL_IPP_OUT_BUF_INFO
											unit require release blk
											unit require free CTL_IPP_OUT_BUF_INFO.buf_id

*/

//state
static UINT32 g_vdoprc_open = 0;
static UINT32 g_vdoprc_count = 0;
#if (USE_OUT_DIS == ENABLE)
extern void _isf_vdoprc_oport_open_dis(ISF_UNIT *p_thisunit, int w, int h);
extern void _isf_vdoprc_oport_close_dis(ISF_UNIT *p_thisunit);
#endif

static VDOPRC_COMMON_MEM g_vdoprc_mem = {0};
static VDOPRC_CONTEXT* g_vdoprc_context = 0;
static UINT32 g_vdoprc_init[ISF_FLOW_MAX] = {0};

UINT32 g_vdoprc_max_count = 0;
ISF_UNIT *g_vdoprc_list[VDOPRC_MAX_NUM] = {
#if (VDOPRC_MAX_NUM > 0)
	&isf_vdoprc0,
#endif
#if (VDOPRC_MAX_NUM > 1)
	&isf_vdoprc1,
#endif
#if (VDOPRC_MAX_NUM > 2)
	&isf_vdoprc2,
#endif
#if (VDOPRC_MAX_NUM > 3)
	&isf_vdoprc3,
#endif
#if (VDOPRC_MAX_NUM > 4)
	&isf_vdoprc4,
#endif
#if (VDOPRC_MAX_NUM > 5)
	&isf_vdoprc5,
#endif
#if (VDOPRC_MAX_NUM > 6)
	&isf_vdoprc6,
#endif
#if (VDOPRC_MAX_NUM > 7)
	&isf_vdoprc7,
#endif
#if (VDOPRC_MAX_NUM > 8)
	&isf_vdoprc8,
#endif
#if (VDOPRC_MAX_NUM > 9)
	&isf_vdoprc9,
#endif
#if (VDOPRC_MAX_NUM > 10)
	&isf_vdoprc10,
#endif
#if (VDOPRC_MAX_NUM > 11)
	&isf_vdoprc11,
#endif
#if (VDOPRC_MAX_NUM > 12)
	&isf_vdoprc12,
#endif
#if (VDOPRC_MAX_NUM > 13)
	&isf_vdoprc13,
#endif
#if (VDOPRC_MAX_NUM > 14)
	&isf_vdoprc14,
#endif
#if (VDOPRC_MAX_NUM > 15)
	&isf_vdoprc15,
#endif
};
#if (USE_EIS == ENABLE)
unsigned int isf_vdoprc_eis_dbg_flag = 0;
//only for direct mode
static void _isf_vdoprc_eis_trig_cb(void* evt)
{
	ISF_UNIT *p_thisunit = &isf_vdoprc0;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

	if (_vprc_eis_plugin_ && p_ctx->eis_func) {
		VDO_FRAME *p_vdo_frm = (VDO_FRAME *)evt;
		//VDOPRC_EIS_PROC_INFO proc_info;
		UINT32  data_num = 0, data_size;
		ULONG rsv1 = p_vdo_frm->reserved[1];
		INT32 *agyro_x;
		INT32 *agyro_y;
		INT32 *agyro_z;
		INT32 *ags_x;
		INT32 *ags_y;
		INT32 *ags_z;
		UINT64 t_diff_crop;
		UINT64 t_diff_crp_end_to_vd;
		ULONG time_stamp_addr;
		ULONG gyro_addr;

		if (p_ctx->p_eis_info_ctx == NULL) {
			DBG_ERR("eis ctx NULL!\r\n");
			return;
		}
		//DBG_DUMP("_isf_vdoprc_eis_trig_cb: frame=%llu\r\n", p_vdo_frm->count);
		if (rsv1 && (*(UINT32 *)(rsv1))) {
			data_num = *(UINT32 *)(rsv1);
			//DBG_DUMP("data_num=%d\r\n", data_num);
			vprc_gyro_data_num = data_num;
			t_diff_crop =  *(UINT64 *)(rsv1 + sizeof(data_num));
			t_diff_crp_end_to_vd = *(UINT64 *)(rsv1 + sizeof(data_num) + sizeof(t_diff_crop));
			time_stamp_addr = rsv1 + sizeof(data_num) + sizeof(t_diff_crop) + sizeof(t_diff_crp_end_to_vd);
			gyro_addr = rsv1 + sizeof(data_num) + sizeof(t_diff_crop) + sizeof(t_diff_crp_end_to_vd) +  data_num*sizeof(UINT32);

			agyro_x =  (INT32 *)(gyro_addr);
			agyro_y =  (INT32 *)(gyro_addr + (1 * data_num * sizeof(INT32)));
			agyro_z =  (INT32 *)(gyro_addr + (2 * data_num * sizeof(INT32)));
			ags_x = (INT32 *)(gyro_addr + (3 * data_num * sizeof(INT32)));
			ags_y = (INT32 *)(gyro_addr + (4 * data_num * sizeof(INT32)));
			ags_z = (INT32 *)(gyro_addr + (5 * data_num * sizeof(INT32)));

			p_ctx->p_eis_info_ctx->frame_count = p_vdo_frm->count;
			p_ctx->p_eis_info_ctx->t_diff_crop = t_diff_crop;
			p_ctx->p_eis_info_ctx->t_diff_crp_end_to_vd = t_diff_crp_end_to_vd;
			p_ctx->p_eis_info_ctx->data_num = data_num;
			p_ctx->p_eis_info_ctx->user_data_size = p_vdo_frm->loff[2];
			if (data_num > MAX_GYRO_DATA_NUM) {
				DBG_ERR("Gyro data_num(%d) > %d\r\n", data_num, MAX_GYRO_DATA_NUM);
			} else if (data_num) {
				isf_vdoprc_put_gyro_frame_cnt(p_ctx->p_eis_info_ctx->frame_count);
				data_size = data_num*sizeof(UINT32);
				memcpy((void *)p_ctx->p_eis_info_ctx->angular_rate_x, agyro_x, data_size);
				memcpy((void *)p_ctx->p_eis_info_ctx->angular_rate_y, agyro_y, data_size);
				memcpy((void *)p_ctx->p_eis_info_ctx->angular_rate_z, agyro_z, data_size);
				memcpy((void *)p_ctx->p_eis_info_ctx->acceleration_rate_x, ags_x, data_size);
				memcpy((void *)p_ctx->p_eis_info_ctx->acceleration_rate_y, ags_y, data_size);
				memcpy((void *)p_ctx->p_eis_info_ctx->acceleration_rate_z, ags_z, data_size);
				memcpy((void *)p_ctx->p_eis_info_ctx->gyro_timestamp, (INT32 *)time_stamp_addr, data_size);
				if (vprc_gyro_latency == 0 && data_num > 1) {
					//check gyro timestamp vs frame start/end
					UINT32 gyro_time_interval = p_ctx->p_eis_info_ctx->gyro_timestamp[1] - p_ctx->p_eis_info_ctx->gyro_timestamp[0];
					UINT32 crp_end = t_diff_crp_end_to_vd;
					UINT32 last_gyro_time = p_ctx->p_eis_info_ctx->gyro_timestamp[data_num-1];

					if ((p_ctx->p_eis_info_ctx->gyro_timestamp[0] > t_diff_crop) ||
						((crp_end > last_gyro_time) && ((crp_end - last_gyro_time) > gyro_time_interval))) {
						DBG_DUMP("Enable vprc_gyro_latency gyro[0]=%d frm_start=%d gyro[%d]=%d frm_end=%d, gyro_time_interval=%d\r\n", p_ctx->p_eis_info_ctx->gyro_timestamp[0], (UINT32)t_diff_crop, data_num-1, p_ctx->p_eis_info_ctx->gyro_timestamp[data_num-1], (UINT32)t_diff_crp_end_to_vd, gyro_time_interval);
						vprc_gyro_latency = 1;
						_vprc_eis_plugin_->set(VDOPRC_EIS_PARAM_GYRO_LATENCY, &vprc_gyro_latency);
					}
				}
				_vprc_eis_plugin_->set(VDOPRC_EIS_PARAM_TRIG_PROC_IN_ISR, p_ctx->p_eis_info_ctx);
				if (isf_vdoprc_eis_dbg_flag & VDOPRC_EIS_DBG_IN_GYRO) {
					UINT32 n;

					DBG_DUMP("FRAME(%llu) data_num=%d crop_start=%llu end=%llu\r\n", p_ctx->p_eis_info_ctx->frame_count, data_num, t_diff_crop, t_diff_crp_end_to_vd);
					for (n = 0; n < data_num; n++) {
						DBG_DUMP("[%d]%d %d %d %d %d %d %d\r\n", n, p_ctx->p_eis_info_ctx->angular_rate_x[n], p_ctx->p_eis_info_ctx->angular_rate_y[n], p_ctx->p_eis_info_ctx->angular_rate_z[n]
														, p_ctx->p_eis_info_ctx->acceleration_rate_x[n], p_ctx->p_eis_info_ctx->acceleration_rate_y[n], p_ctx->p_eis_info_ctx->acceleration_rate_z[n]
														, p_ctx->p_eis_info_ctx->gyro_timestamp[n]);
					}
				}
				#if 0//for debug only
				{
					INT32 *p_data;
					DBG_DUMP("data_num=%d\r\n", data_num);
					p_data = p_ctx->p_eis_info_ctx->angular_rate_x;
					DBG_DUMP("angular_rate_x\r\n");
					DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																		*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					p_data = p_ctx->p_eis_info_ctx->angular_rate_y;
					DBG_DUMP("angular_rate_y\r\n");
					DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																		*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					p_data = p_ctx->p_eis_info_ctx->angular_rate_z;
					DBG_DUMP("angular_rate_y\r\n");
					DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																		*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					p_data = p_ctx->p_eis_info_ctx->acceleration_rate_x;
					DBG_DUMP("acceleration_rate_x\r\n");
					DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																		*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					p_data = p_ctx->p_eis_info_ctx->acceleration_rate_y;
					DBG_DUMP("acceleration_rate_y\r\n");
					DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																		*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					p_data = p_ctx->p_eis_info_ctx->acceleration_rate_z;
					DBG_DUMP("acceleration_rate_z\r\n");
					DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																	*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
					p_data = p_ctx->p_eis_info_ctx->gyro_timestamp;
					DBG_DUMP("gyro_timestamp\r\n");
					DBG_DUMP("%08X %08X %08X %08X %08X %08X %08X %08X\r\n", *(p_data+0), *(p_data+1), *(p_data+2), *(p_data+3),
																	*(p_data+4), *(p_data+5), *(p_data+6), *(p_data+7));
				}
				#endif
			}
		} else {
			DBG_DUMP("[%lld]No gyro!\r\n", p_vdo_frm->count);
		}
	}
}
#endif

BOOL _isf_vdoprc_is_init(void)
{
	return (g_vdoprc_context != 0)? TRUE: FALSE;
}

static ISF_RV _isf_vdoprc_do_init(UINT32 h_isf, UINT32 dev_max_count)
{
	ISF_RV rv = ISF_OK;
	VDOPRC_COMMON_MEM* p_mem = &g_vdoprc_mem;
	UINT32             cur_buf;

	if (h_isf > 0) { //client process
		if (!g_vdoprc_init[0]) { //is master process already init?
			rv = ISF_ERR_INIT; //not allow init from client process!
			goto _VP_INIT_ERR;
		}
		g_vdoprc_init[h_isf] = 1; //set init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_vdoprc_init[i]) { //client process still alive?
				rv = ISF_ERR_INIT; //not allow init from main process!
				goto _VP_INIT_ERR;
			}
		}
	}

	if (dev_max_count > 0) {
		//update count
		DEV_MAX_COUNT = dev_max_count;
		if (DEV_MAX_COUNT > VDOPRC_MAX_NUM) {
			DBG_WRN("dev max count %d > max %d!\r\n", DEV_MAX_COUNT, VDOPRC_MAX_NUM);
			DEV_MAX_COUNT = VDOPRC_MAX_NUM;
		}
	}
	if (g_vdoprc_context) {
		rv = ISF_ERR_INIT; //already init
		goto _VP_INIT_ERR;
	}
	{
		UINT32 dev;
		CTL_IPP_CTX_BUF_CFG ctx_buf;
		INT32 rt;
#if (USE_VPE == ENABLE)
		CTL_VPE_CTX_BUF_CFG ctx_buf_cfg = {0};
#endif
#if (USE_ISE == ENABLE)
		CTL_ISE_CTX_BUF_CFG ctx_ise_buf_cfg = {0};
#endif

		nvtmpp_vb_add_module(g_vdoprc_list[0]->unit_module);
		memset((void*)p_mem, 0, sizeof(VDOPRC_COMMON_MEM));
		p_mem->total_buf.size = 0;
		p_mem->total_buf.addr = 0;
		//calculate conext buffer size of unit
		p_mem->unit_buf.size = ALIGN_CEIL(DEV_MAX_COUNT * sizeof(VDOPRC_CONTEXT), 64); //align to cache line
		p_mem->total_buf.size += p_mem->unit_buf.size;
		//calculate conext buffer size of kflow/kdrv
		ctx_buf.n = DEV_MAX_COUNT;
		p_mem->kflow_buf.size = ALIGN_CEIL(ctl_ipp_query(ctx_buf), 64); //align to cache line
		p_mem->total_buf.size += p_mem->kflow_buf.size;

#if (USE_VPE == ENABLE)
		ctx_buf_cfg.handle_num = DEV_MAX_COUNT;
		ctx_buf_cfg.queue_num = 8;
		p_mem->kflow_vpe_buf.size = ALIGN_CEIL(ctl_vpe_query(ctx_buf_cfg), 64); //align to cache line
		p_mem->total_buf.size += p_mem->kflow_vpe_buf.size;
#endif
#if (USE_ISE == ENABLE)
		ctx_ise_buf_cfg.handle_num = DEV_MAX_COUNT;
		ctx_ise_buf_cfg.queue_num = 8;
		p_mem->kflow_ise_buf.size = ALIGN_CEIL(ctl_ise_query(ctx_ise_buf_cfg), 64); //align to cache line
		p_mem->total_buf.size += p_mem->kflow_ise_buf.size;
#endif

		//alloc context of all device
		p_mem->total_buf.addr = DEV_UNIT(0)->p_base->do_new_i(DEV_UNIT(0), NULL, "ctx", p_mem->total_buf.size, &(p_mem->memblk));
		if (p_mem->total_buf.addr == 0) {
			DBG_ERR("alloc context memory fail !!\r\n");
			rv = ISF_ERR_HEAP;
			goto _VP_INIT_ERR;
		}

		//init each device's context of unit
		g_vdoprc_context = (VDOPRC_CONTEXT*)p_mem->total_buf.addr;
		for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);
			if (p_unit && (p_unit->do_init)) {
				rv = p_unit->do_init(p_unit, (void*)&(g_vdoprc_context[dev]));
				if (rv != ISF_OK) {
					rv = DEV_UNIT(0)->p_base->do_release_i(DEV_UNIT(0), &(p_mem->memblk), ISF_OK);
					g_vdoprc_context = 0;
					goto _VP_INIT_ERR;
				}
			}
		}


		//init context of kflow/kdrv
		p_mem->kflow_buf.addr = p_mem->total_buf.addr + p_mem->unit_buf.size;
		rt = ctl_ipp_init(ctx_buf, p_mem->kflow_buf.addr, p_mem->kflow_buf.size);
		if (rt != CTL_IPP_E_OK) {
			rv = ISF_ERR_DRV;
			goto _VP_INIT_ERR;
		}
		cur_buf = p_mem->total_buf.addr + p_mem->unit_buf.size + p_mem->kflow_buf.size;
		//DBG_ERR("total_buf= 0x%x, kflow_buf = 0x%x\r\n",
				//p_mem->total_buf.addr, p_mem->kflow_buf.addr);
#if (USE_VPE == ENABLE)
		p_mem->kflow_vpe_buf.addr = cur_buf;
		rt = ctl_vpe_init(ctx_buf_cfg, p_mem->kflow_vpe_buf.addr, p_mem->kflow_vpe_buf.size);
		if (rt != CTL_VPE_E_OK) {
			rv = ISF_ERR_DRV;
			goto _VP_INIT_ERR;
		}
		cur_buf += p_mem->kflow_vpe_buf.size;
		//DBG_ERR("vpe_buf= 0x%x\r\n", p_mem->kflow_vpe_buf.addr);
#endif
#if (USE_ISE == ENABLE)
		p_mem->kflow_ise_buf.addr = cur_buf;
		rt = ctl_ise_init(ctx_ise_buf_cfg, p_mem->kflow_ise_buf.addr, p_mem->kflow_ise_buf.size);
		if (rt != CTL_ISE_E_OK) {
			rv = ISF_ERR_DRV;
			goto _VP_INIT_ERR;
		}
		//cur_buf += p_mem->kflow_ise_buf.size; //for coverity 162658,if there is other buf, unmark again
		//DBG_ERR("ise_buf= 0x%x\r\n", p_mem->kflow_ise_buf.addr);
#endif
		//install each device's kernel object
		isf_vdoprc_install_id();

		g_vdoprc_init[h_isf] = 1; //set init
		return ISF_OK;
	}

_VP_INIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("init fail! %d\r\n", rv);
	}
	return rv;
}

static ISF_RV _isf_vdoprc_do_uninit(UINT32 h_isf, UINT32 reset)
{
	ISF_RV rv = ISF_OK;
	VDOPRC_COMMON_MEM* p_mem = &g_vdoprc_mem;

	if (h_isf > 0) { //client process
		if (!g_vdoprc_init[0]) { //is master process already init?
			rv = ISF_ERR_UNINIT; //not allow uninit from client process!
			goto _VP_UNINIT_ERR;
		}
		g_vdoprc_init[h_isf] = 0; //clear init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_vdoprc_init[i] && !reset) { //client process still alive?
				rv = ISF_ERR_UNINIT; //not allow uninit from main process!
				goto _VP_UNINIT_ERR;
			}
			if (reset) {
				g_vdoprc_init[i] = 0; //force clear client
			}
		}
	}

	if (!g_vdoprc_context) {
		return ISF_ERR_UNINIT; //still not init
	}

	{
		UINT32 dev, i;
		INT32 rt;

		//reset all units of this module
		for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);
			if (p_unit) {
				DBG_IND("unit(%d) = %s => clear state\r\n", dev, p_unit->unit_name);
				for(i = 0; i < p_unit->port_out_count; i++) {
					UINT32 oport = i + ISF_OUT_BASE;
					p_unit->p_base->do_clear_state(p_unit, oport);
				}
			}
		}
		for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);
			if (p_unit) {
				DBG_IND("unit(%d) = %s => clear bind\r\n", dev, p_unit->unit_name);
				for(i = 0; i < p_unit->port_out_count; i++) {
					UINT32 oport = i + ISF_OUT_BASE;
					p_unit->p_base->do_clear_bind(p_unit, oport);
				}
			}
		}
		for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);
			if (p_unit) {
				DBG_IND("unit(%d) = %s => clear context\r\n", dev, p_unit->unit_name);
				for(i = 0; i < p_unit->port_out_count; i++) {
					UINT32 oport = i + ISF_OUT_BASE;
					p_unit->p_base->do_clear_ctx(p_unit, oport);
				}
			}
		}

		//uninstall each device's kernel object
		isf_vdoprc_uninstall_id();
		//uninit kflow/kdrv's context
		rt = ctl_ipp_uninit();
		if (rt != CTL_IPP_E_OK) {
			goto _VP_UNINIT_ERR;
		}
#if (USE_VPE == ENABLE)
		rt = ctl_vpe_uninit();
		if (rt != CTL_VPE_E_OK) {
			goto _VP_UNINIT_ERR;
		}
#endif
#if (USE_ISE == ENABLE)
		rt = ctl_ise_uninit();
		if (rt != CTL_ISE_E_OK) {
			goto _VP_UNINIT_ERR;
		}
#endif

		//uninit each device's context
		for (dev = 0; dev < DEV_MAX_COUNT; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);
			if (p_unit && (p_unit->do_uninit)) {
				rv = p_unit->do_uninit(p_unit);
				if (rv != ISF_OK) {
					goto _VP_UNINIT_ERR;
				}
			}
		}

		//free context of all device
		if (p_mem->memblk.h_data != 0) {
			rv = DEV_UNIT(0)->p_base->do_release_i(DEV_UNIT(0), &(p_mem->memblk), ISF_OK);
			if (rv != ISF_OK) {
				DBG_ERR("free context memory fail !!\r\n");
				goto _VP_UNINIT_ERR;
			}
		}
		g_vdoprc_context = 0;
		g_vdoprc_init[h_isf] = 0; //clear init
		return ISF_OK;
	}

_VP_UNINIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("uninit fail! %d\r\n", rv);
	}
	return rv;
}

static UINT32 g_cfg = 0;

UINT32 _isf_vdoprc_get_cfg(void)
{
	return g_cfg;
}

ISF_RV _isf_vdoprc_do_command(UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2)
{
	UINT32 h_isf = (cmd >> 28); //extract h_isf
	cmd &= ~0xf0000000; //clear h_isf
	switch(cmd) {
	case 1: //INIT
		g_cfg = p2; //vprc basic config
		nvtmpp_vb_add_module(MAKE_NVTMPP_MODULE('v', 'd', 'o', 'p', 'r', 'c', 0, 0));
		return _isf_vdoprc_do_init(h_isf, p1); //p1: max dev count
		break;
	case 0: //UNINIT
		g_cfg = 0; //vprc basic config
		return _isf_vdoprc_do_uninit(h_isf, p1); //p1: is under reset
		break;
	default:
		break;
	}
	return ISF_ERR_NOT_SUPPORT;
}

static UINT32 _vdoprc_get_start(ISF_UNIT *p_thisunit, UINT32 oport);

ISF_RV _isf_vdoprc_do_bindinput(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport)
{
	// verify connect limit
	if ((iport & 0x80000000) || (oport & 0x80000000)) { //unbind
		iport &= ~0x80000000; //clear flag
		//oport &= ~0x80000000; //clear flag
		if (iport == ISF_IN(0)) {
#if (USE_IN_DIRECT == ENABLE)
			VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
			//DBG_DUMP(" \"%s\".out[%d] unbind to \"%s\".in[%d] \r\n",
			//	p_srcunit->unit_name, oport - ISF_OUT_BASE, p_thisunit->unit_name, iport - ISF_IN_BASE);
			if (p_ctx->cur_in_cfg_func & VDOPRC_IFUNC_DIRECT) {
				if (_vdoprc_get_start(p_thisunit, ISF_CTRL-1) > 0) { // vprc is still start some phy path
					DBG_ERR("[direct] vprc cannot be unbind before vprc stop all phy path!\r\n");
					return ISF_ERR_ALREADY_START;
				}
			}
			p_ctx->sie_unl_cb = NULL;
			p_ctx->p_srcunit = NULL;
			p_ctx->sen_user3_cb = NULL;
#endif
		} else {
			return ISF_ERR_INVALID_PORT_ID;
		}
	} else { //bind
		if (iport == ISF_IN(0)) {
#if (USE_IN_DIRECT == ENABLE)
			VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
			//DBG_DUMP(" \"%s\".out[%d] bind to \"%s\".in[%d] \r\n",
			//	p_srcunit->unit_name, oport - ISF_OUT_BASE, p_thisunit->unit_name, iport - ISF_IN_BASE);
			if (p_ctx->new_in_cfg_func & VDOPRC_IFUNC_DIRECT) {
				UINT32 cb = p_srcunit->do_getportparam(p_srcunit, ISF_OUT(0), 0x80001010+0x00000100);
				p_ctx->sie_unl_cb = (SIE_UNLOCK_CB)cb;
				p_ctx->sen_user3_cb = (SEN_USER3_CB)p_srcunit->do_getportparam(p_srcunit, ISF_OUT(0), 0x80001010+0x00000101);
			}
			p_ctx->p_srcunit = p_srcunit;
#endif
		} else {
			return ISF_ERR_INVALID_PORT_ID;
		}
	}
	return ISF_OK;
}

static ISF_RV _isf_vdoprc_check_statelimit(struct _ISF_UNIT *p_thisunit, UINT32 oport, UINT32 in_en, UINT32 out_en)
{
#if (USE_IN_DIRECT == ENABLE)
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 func = (in_en==0)?(p_ctx->new_in_cfg_func):(p_ctx->cur_in_cfg_func);
	ISF_STATE *p_this_state = p_thisunit->port_outstate[oport - ISF_OUT_BASE];

	if (p_this_state->dirty & ISF_PORT_CMD_RESET) { //if under reset?
		return ISF_OK; //always pass
	}

	if ((func & VDOPRC_IFUNC_DIRECT) && (out_en == 0)) {
		ISF_UNIT *p_srcunit = p_ctx->p_srcunit;
		if (!p_srcunit) {
			DBG_ERR("[direct] vprc cannot stop after unbind from vcap!\r\n");
			return ISF_ERR_NOT_CONNECT_YET;
		} else {
			ISF_STATE *p_state = p_srcunit->port_outstate[0];
			if(p_state->state != ISF_PORT_STATE_RUN) {  // vcap is already stop
				DBG_ERR("[direct] vprc cannot stop any path after vcap stop!\r\n");
				return ISF_ERR_STOP_FAIL;
			}
		}
		if (_vdoprc_get_start(p_thisunit, ISF_CTRL-1) == 1) {
			DBG_DUMP("vprc: disable direct func at in[%d]\r\n", 0);
		}
	}
	if ((func & VDOPRC_IFUNC_DIRECT) && (out_en == 1)) {
		ISF_UNIT *p_srcunit = p_ctx->p_srcunit;
		if (!(p_ctx->ifunc_allow & VDOPRC_IFUNC_DIRECT)) {
			DBG_ERR("%s.in[0]! cannot enable direct with pipe=%08x\r\n",
				p_thisunit->unit_name, p_ctx->ctrl.pipe);
			return ISF_ERR_FAILED;
		}
		if (!p_srcunit) {
			DBG_ERR("[direct] vprc cannot start before bind from vcap start!\r\n");
			return ISF_ERR_NOT_CONNECT_YET;
		} else {
			ISF_STATE *p_state = p_srcunit->port_outstate[0];
			if(p_state->state == ISF_PORT_STATE_RUN) { // vcap is already start
				if (_vdoprc_get_start(p_thisunit, ISF_CTRL-1) == 0) { // vprc is not start any phy path yet
					DBG_ERR("[direct] vprc cannot start 1st phy path after vcap start!\r\n");
					return ISF_ERR_START_FAIL;
				}
			}
		}
		if (_vdoprc_get_start(p_thisunit, ISF_CTRL-1) == 0) {
			DBG_DUMP("vprc: enable direct func at in[%d]\r\n", 0);
		}
	}
#endif
	return ISF_OK;
}

#if 0
ISF_RV _isf_vdoprc_do_offsync(ISF_UNIT *p_thisunit, UINT32 oport)
{
	UINT32 iport = oport - ISF_OUT_BASE + ISF_IN_BASE;
	UINT32 i = iport - ISF_IN_BASE;
	if (1) {
		// This is just a dummy port for binding isf_vdoprc to isf_vdoprc !
		// isf_vdoprc will direct access output port info of isf_vdoprc!

		ISF_INFO *p_src_info = p_thisunit->port_ininfo[i];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
		if((p_vdoinfo->max_imgsize.w == 0) && (p_vdoinfo->max_imgsize.h == 0)) {
			p_vdoinfo->max_imgsize.w = 640;
			p_vdoinfo->max_imgsize.h = 480;
			DBG_WRN("in[0].max_size set to default (%d, %d)\r\n", p_vdoinfo->max_imgsize.w, p_vdoinfo->max_imgsize.h);
		}
		if((p_vdoinfo->imgsize.w == 0) && (p_vdoinfo->imgsize.h == 0)) {
			p_vdoinfo->imgsize.w = 640;
			p_vdoinfo->imgsize.h = 480;
			DBG_WRN("in[0].size set to default (%d, %d)\r\n", p_vdoinfo->imgsize.w, p_vdoinfo->imgsize.h);
		}
		if(p_vdoinfo->imgfmt == 0) {
			p_vdoinfo->imgfmt = VDO_PXLFMT_RAW8;
			DBG_WRN("in[0].fmt set to default %08x\r\n", p_vdoinfo->imgfmt);
		}
		//p_vdoinfo->imgaspect.w = 0;
		//p_vdoinfo->imgaspect.h = 0;
	}
	return ISF_OK;
}
#endif


static ISF_RV _vdoprc_set_init(ISF_UNIT *p_thisunit)
{
	ISF_RV r = ISF_OK;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	//ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
	//ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
	CTL_IPP_PRIVATE_BUF init_buf;
	CTL_IPP_BUFCFG buf_cfg;
	CTL_IPP_ALGID algid;

	DBG_FUNC("%s\r\n", p_thisunit->unit_name);



	if (1) {

		algid.type = CTL_IPP_ALGID_IQ;
		algid.id = p_ctx->ctrl.iq_id;
		ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_ALGID_IMM, (void *)&algid);

		memset((void*)&init_buf, 0, sizeof(CTL_IPP_PRIVATE_BUF));

		_vdoprc_max_func(p_thisunit, &init_buf.func_en);

		_vdoprc_max_in(p_thisunit, &init_buf.max_size, &init_buf.pxlfmt);

		#if 0
		//set input frame rate
		//_vdoprc_config_in_framerate(p_thisunit, p_thisunit->port_in[0]);
		#endif
	}

	if (1) {
		//query require size
		UINT32 buf_addr, buf_size;

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		{
			UINT32 user_cfg = _isf_vdoprc_get_cfg();
			UINT32 stripe_rule = user_cfg & HD_VIDEOPROC_CFG_STRIP_MASK; //get stripe rule
			UINT32 stripe_num = 0;
			if (stripe_rule > 3) {
				stripe_rule = 0;
			}

            //if sysconfig stipe is 0, check vendor stripe
            if(stripe_rule==0) {
                stripe_rule = (((p_ctx->ctrl.stripe_rule & HD_VIDEOPROC_CFG) >> 16) & HD_VIDEOPROC_CFG_STRIP_MASK);

				// if ctrl stripe_rule is default value 0, check stripe number setting
				if (stripe_rule == 0) {
					stripe_num = (p_ctx->ctrl.stripe_rule & HD_VIDEOPROC_CFG_STRIPN_MASK) >> 20;
					if (stripe_num) {
						stripe_rule = CTL_IPP_STRP_RULE_NSTRP_OFS + stripe_num;
					}
				}
            }

            //check AI enable
    		if (user_cfg & VENDOR_AI_CFG_ENABLE_CNN) {
    			if ((stripe_rule == 0)||(stripe_rule == 3)) { //GDC is enable?
    				DBG_DUMP("%s CNN on => stripe LV%x, force to LV2!",p_thisunit->unit_name,stripe_rule+1);
    				stripe_rule = (HD_VIDEOPROC_CFG_STRIP_LV2 >> 16); //disable GDC for AI
    			} else {
    				//DBG_DUMP("%s CNN on => stripe LV%x!",p_thisunit->unit_name,stripe_rule+1);
    			}
    		}

            //dump info for user
    		{
    			if (stripe_rule == 0) { //LV1
    				DBG_DUMP("%s init: stripe LV1 = cut w>1280, GDC =  on, 2D_LUT off after cut (LL slow)\r\n",p_thisunit->unit_name);
    			} else if (stripe_rule == 1){ //LV2
    				DBG_DUMP("%s init: stripe LV2 = cut w>2048, GDC = off, 2D_LUT off after cut (LL fast)\r\n",p_thisunit->unit_name);
    			} else if (stripe_rule == 2){ //LV3
    				DBG_DUMP("%s init: stripe LV3 = cut w>2688, GDC = off, 2D_LUT off after cut (LL middle)(2D_LUT best)\r\n",p_thisunit->unit_name);
    			} else if (stripe_rule == 3){ //LV4
    				DBG_DUMP("%s init: stripe LV4 = cut w> 720, GDC =  on, 2D_LUT off after cut (LL not allow)(GDC best)\r\n",p_thisunit->unit_name);
    			} else {
					DBG_DUMP("%s init: stripe NUM%u\r\n",p_thisunit->unit_name, stripe_num);
    			}
    		}

            //DBG_DUMP("user_cfg  %x dev_handle:%x #real# stripe_rule:%x vnedor stripe_rule :%x\r\n",user_cfg,p_ctx->dev_handle,stripe_rule,p_ctx->ctrl.stripe_rule);
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_STRP_RULE, (void *)&stripe_rule);
		}
#endif

		{
#if (DUMP_PERFORMANCE == ENABLE)
			UINT64 t1, t2;
			t1 = hwclock_get_longcounter();
#endif
			//DBG_DUMP("%s max_func=%08x max_size=(%d,%d)\r\n", p_thisunit->unit_name,
			//	init_buf.func_en, init_buf.max_size.w, init_buf.max_size.h);
			ctl_ipp_get(p_ctx->dev_handle, CTL_IPP_ITEM_BUFQUY, (void *)&init_buf);
#if (USE_OUT_ONEBUF == ENABLE)
			p_ctx->max_strp_num = init_buf.max_strp_num;
#endif
#if (DUMP_PERFORMANCE == ENABLE)
			t2 = hwclock_get_longcounter();
			DBG_DUMP("                                        BUF_QUERY dt=%llu us\r\n", (t2 - t1));
#endif
		}

#if (USE_OUT_DIS == ENABLE)
		if (p_ctx->outq.dis_mode) {
			_isf_vdoprc_oport_open_dis(p_thisunit, init_buf.max_size.w, init_buf.max_size.h);
		}
#endif

		buf_size = ALIGN_CEIL_64(init_buf.buf_size);
#if (USE_OUT_DIS == ENABLE)
		if (p_ctx->outq.dis_mode) {
			buf_size += ALIGN_CEIL_64(p_ctx->outq.dis_buf_size);
		}
#endif

		//create buffer
		buf_addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "work", buf_size, &(p_ctx->memblk));
#if (USE_OUT_DIS == ENABLE)
		if (p_ctx->outq.dis_mode) {
			p_ctx->outq.dis_buf_addr = buf_addr + ALIGN_CEIL_64(init_buf.buf_size);
		} else {
			p_ctx->outq.dis_buf_addr = 0;
		}
#endif

		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_GENERAL, "work, new size=%u, addr=%08x", buf_size, buf_addr);
		if (buf_addr == 0) {
			DBG_ERR("%s create buffer failed! (%d)\r\n",  p_thisunit->unit_name, buf_size);
			return ISF_ERR_BUFFER_CREATE;
		}

		p_ctx->mem.addr = buf_addr;
		p_ctx->mem.size = buf_size;
		{
			UINT32 apply_id;
#if (DUMP_PERFORMANCE == ENABLE)
			UINT64 t1, t2;
			t1 = hwclock_get_longcounter();
#endif
			buf_cfg.start_addr = buf_addr;
			buf_cfg.size = init_buf.buf_size; //buf_size should the same with query size
			r=ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_BUFCFG, (void *)&buf_cfg);
            if(r!=0) {
    			DBG_ERR("%s bufcfg failed! (%d)\r\n",  p_thisunit->unit_name,r);
    			return ISF_ERR_BUFFER_CREATE;
		    }
			r=ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_APPLY, (void *)&apply_id);
            if(r!=0){
    			DBG_ERR("%s apply failed! (%d)\r\n",  p_thisunit->unit_name,r);
    			return ISF_ERR_BUFFER_CREATE;
		    }
#if (DUMP_PERFORMANCE == ENABLE)
			t2 = hwclock_get_longcounter();
			DBG_DUMP("                                        BUF_INFOR dt=%llu us\r\n", (t2 - t1));
#endif
		}
	}

	return r;
}

static ISF_RV _vdoprc_set_exit(ISF_UNIT *p_thisunit)
{
	ISF_RV r = ISF_OK;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	DBG_FUNC("%s\r\n", p_thisunit->unit_name);

	if (1) {
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_GENERAL, "work, release");
		//release buffer
		r = p_thisunit->p_base->do_release_i(p_thisunit, &(p_ctx->memblk), ISF_OK);
		if (r != ISF_OK) {
			DBG_ERR("%s destroy buffer failed! (%d)\r\n", p_thisunit->unit_name, r);
			return ISF_ERR_BUFFER_DESTROY;
		}

		p_ctx->mem.addr = 0;
		p_ctx->mem.size = 0;
	}

	return r;
}

static UINT32 _vdoprc_get_start(ISF_UNIT *p_thisunit, UINT32 oport)
{
	UINT32 c = 0;

	if (oport == ISF_CTRL) {
		int i;
		for (i=0; i<ISF_VDOPRC_OUT_NUM; i++)
			if (p_thisunit->port_outstate[i]->state == ISF_PORT_STATE_RUN) c++;
		//DBG_MSG("%s start count=%d\r\n", p_thisunit->unit_name, c);
	}
	if (oport == ISF_CTRL-1) {
		int i;
		for (i=0; i<ISF_VDOPRC_PHY_OUT_NUM; i++)
			if (p_thisunit->port_outstate[i]->state == ISF_PORT_STATE_RUN) c++;
	}

	return c;
}

static UINT32 _vdoprc_get_open(ISF_UNIT *p_thisunit, UINT32 oport)
{
	UINT32 c = 0;

	if (oport == ISF_CTRL) {
		int i;
		for (i=0; i<ISF_VDOPRC_OUT_NUM; i++)
			if (p_thisunit->port_outstate[i]->state > ISF_PORT_STATE_OFF) c++;
		//DBG_MSG("%s open count=%d\r\n", p_thisunit->unit_name, c);
	}
	if (oport == ISF_CTRL-1) {
		int i;
		for (i=0; i<ISF_VDOPRC_PHY_OUT_NUM; i++)
			if (p_thisunit->port_outstate[i]->state > ISF_PORT_STATE_OFF) c++;
		//DBG_MSG("%s open count=%d\r\n", p_thisunit->unit_name, c);
	}

	return c;
}
#if (USE_VPE == ENABLE)
static ISF_RV _vdoprc_set_vpe_init(ISF_UNIT *p_thisunit)
{
	ISF_RV r = ISF_OK;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	CTL_VPE_PRIVATE_BUF pri_buf = {0};

	DBG_FUNC("%s\r\n", p_thisunit->unit_name);

	ctl_vpe_set(p_ctx->dev_handle, CTL_VPE_ITEM_ALGID_IMM, (void *)&p_ctx->ctrl.iq_id);
	{
#if (DUMP_PERFORMANCE == ENABLE)
		UINT64 t1, t2;
		t1 = hwclock_get_longcounter();
#endif
		ctl_vpe_get(p_ctx->dev_handle, CTL_VPE_ITEM_PRIVATE_BUF, (void *)&pri_buf);
#if (DUMP_PERFORMANCE == ENABLE)
		t2 = hwclock_get_longcounter();
		DBG_DUMP("                                        BUF_QUERY dt=%llu us\r\n", (t2 - t1));
#endif
	}
	if (pri_buf.buf_size) {
		//create buffer
		pri_buf.buf_addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "work", pri_buf.buf_size, &(p_ctx->memblk));
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_GENERAL, "work, new size=%u, addr=%08x", pri_buf.buf_size, pri_buf.buf_addr);
		if (pri_buf.buf_addr == 0) {
			DBG_ERR("%s create buffer failed! (%d)\r\n",  p_thisunit->unit_name, pri_buf.buf_size);
			return ISF_ERR_BUFFER_CREATE;
		}
		ctl_vpe_set(p_ctx->dev_handle, CTL_VPE_ITEM_PRIVATE_BUF, (void *)&pri_buf);
	}
	p_ctx->mem.addr = pri_buf.buf_addr;
	p_ctx->mem.size = pri_buf.buf_size;
	ctl_vpe_set(p_ctx->dev_handle, CTL_VPE_ITEM_APPLY, NULL);
	return r;
}
static ISF_RV _vdoprc_set_vpe_exit(ISF_UNIT *p_thisunit)
{
	ISF_RV r = ISF_OK;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	DBG_FUNC("%s\r\n", p_thisunit->unit_name);

	if (p_ctx->mem.size) {
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_GENERAL, "work, release");
		//release buffer
		r = p_thisunit->p_base->do_release_i(p_thisunit, &(p_ctx->memblk), ISF_OK);
		if (r != ISF_OK) {
			DBG_ERR("%s destroy buffer failed! (%d)\r\n", p_thisunit->unit_name, r);
			return ISF_ERR_BUFFER_DESTROY;
		}

		p_ctx->mem.addr = 0;
		p_ctx->mem.size = 0;
	}

	return r;
}
static ISF_RV _vdoprc_config_vpe_scene(ISF_UNIT *p_thisunit)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	CTL_VPE_DCS_MODE dcs_mode = p_ctx->vpe_scene;

	p_thisunit->p_base->do_trace(p_thisunit, ISF_IN(0), ISF_OP_PARAM_VDOFRM_IMM, "VPE SCENE = %d", dcs_mode);

	ctl_vpe_set(p_ctx->dev_handle, CTL_VPE_ITEM_DCS, (void *)&dcs_mode);
	return ISF_OK;
}
static ISF_RV _vdoprc_set_vpe_mode(ISF_UNIT *p_thisunit, UINT32 ipp_mode, UINT32 oport, UINT32 en)
{
	ISF_RV r;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	//UINT32 mode = ipp_mode;
	p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_GENERAL, "enable=%08x mode=%d", en, ipp_mode);

	if (1) {
#if (USE_IN_FRC == ENABLE)
		ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
#endif
		//ISF_UNIT* p_dest = NULL;
		p_ctx->outq.output_cur_en = p_ctx->outq.output_en;

		//set output image size, fmt
		if (ipp_mode == VDOPRC_PIPE_OFF) {
			UINT32 i;
#if (USE_IN_FRC == ENABLE)
			//stop input frame rate
			_isf_frc_stop(p_thisunit, ISF_IN(0), &(p_ctx->infrc[0]));
#endif
			for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++)
			{
				BOOL do_en = FALSE;
				r = _vdoprc_config_out_vpe(p_thisunit, ISF_OUT(i), do_en, FALSE);
				if (r != ISF_OK)
					return r;
				_vdoprc_oport_set_enable(p_thisunit, ISF_OUT(i), do_en);
			}
		} else {
			UINT32 i;

			_vdoprc_config_vpe_scene(p_thisunit);
			//init phy mask
			for (i = 0; i < ISF_VDOPRC_PHY_OUT_NUM; i++) {
				p_ctx->phy_mask |= (1<<i);
			}
			//set input image crop ratio
			r = _vdoprc_config_vpe_in_crop(p_thisunit, ISF_IN(0));
			if (r != ISF_OK)
				return r;

#if (USE_IN_FRC == ENABLE)
			//start input frame rate
			if (p_vdoinfo->framepersecond == VDO_FRC_DIRECT) p_vdoinfo->framepersecond = VDO_FRC_ALL;
			p_thisunit->p_base->do_trace(p_thisunit, ISF_IN(0), ISF_OP_PARAM_VDOFRM_IMM, "frc(%d,%d)",
				GET_HI_UINT16(p_vdoinfo->framepersecond), GET_LO_UINT16(p_vdoinfo->framepersecond));
			_isf_frc_start(p_thisunit, ISF_IN(0), &(p_ctx->infrc[0]), p_vdoinfo->framepersecond);
#endif

			for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++)
			{
				if (p_ctx->phy_mask & (1<<i)) {
					BOOL do_en = FALSE;
					do_en = ((p_thisunit->port_outstate[i]->state) == (ISF_PORT_STATE_RUN)); //port current state
					if ((i + ISF_OUT_BASE) == oport) {
						do_en = en; //port new state
					}
					r = _vdoprc_config_out_vpe(p_thisunit, ISF_OUT(i), do_en, FALSE);
					if (r != ISF_OK)
						return r;
					_vdoprc_oport_set_enable(p_thisunit, ISF_OUT(i), do_en);
				}
			}
		}

		if ((oport - ISF_OUT_BASE) < ISF_VDOPRC_OUT_NUM) {
			//XDATA
			VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
#if (DUMP_PERFORMANCE == ENABLE)
			UINT64 t1, t2;
			t1 = hwclock_get_longcounter();
#endif
			p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "do change mode=%d", ipp_mode);

			ctl_vpe_set(p_ctx->dev_handle, CTL_VPE_ITEM_APPLY, NULL);

			if (ipp_mode == VDOPRC_PIPE_OFF) {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "flush and wait finish");
				ctl_vpe_set(p_ctx->dev_handle, CTL_VPE_ITEM_FLUSH, NULL);
			}
#if (DUMP_PERFORMANCE == ENABLE)
			t2 = hwclock_get_longcounter();
			DBG_DUMP("										  CHGMODE dt=%llu us\r\n", (t2 - t1));
#endif
		}
		//if (ipp_mode == VDOPRC_PIPE_OFF)
		{
			//DBG_DUMP("~G%s\r\n", g_cur_modedata->proc_sen_id+1);
			//DBG_DUMP("~G-mode=%d\r\n", g_cur_modedata->mode);
			//    IPL_WaitCmdFinish();
		}

		//clear dirty
		{
			ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
			ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
			UINT32 i;
			//clear dirty of in[0]
			p_vdoinfo->dirty &= ~(ISF_INFO_DIRTY_ASPECT|ISF_INFO_DIRTY_IMGSIZE|ISF_INFO_DIRTY_DIRECT|ISF_INFO_DIRTY_FRAMERATE); //clear dirty
			//clear dirty of out[i]
			for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++)
			{
				if (p_ctx->phy_mask & (1<<i)) {
					//XDATA
					ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
					ISF_VDO_INFO *p_vdoinfo2 = (ISF_VDO_INFO *)(p_dest_info);
					if (p_vdoinfo2) {
					         p_vdoinfo2->dirty &= ~(ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW);    //clear dirty
					}
				}
			}
		}
	}

	//p_ctx->cur_mode = ipp_mode;
	return ISF_OK;
}
#endif
#if (USE_ISE == ENABLE)
static ISF_RV _vdoprc_set_ise_init(ISF_UNIT *p_thisunit)
{
	ISF_RV r = ISF_OK;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	CTL_ISE_PRIVATE_BUF pri_buf = {0};

	DBG_FUNC("%s\r\n", p_thisunit->unit_name);

	ctl_ise_set(p_ctx->dev_handle, CTL_ISE_ITEM_ALGID_IMM, (void *)&p_ctx->ctrl.iq_id);
	{
#if (DUMP_PERFORMANCE == ENABLE)
		UINT64 t1, t2;
		t1 = hwclock_get_longcounter();
#endif
		ctl_ise_get(p_ctx->dev_handle, CTL_ISE_ITEM_PRIVATE_BUF, (void *)&pri_buf);
#if (DUMP_PERFORMANCE == ENABLE)
		t2 = hwclock_get_longcounter();
		DBG_DUMP("                                        BUF_QUERY dt=%llu us\r\n", (t2 - t1));
#endif
	}
	if (pri_buf.buf_size) {
		//create buffer
		pri_buf.buf_addr = p_thisunit->p_base->do_new_i(p_thisunit, NULL, "work", pri_buf.buf_size, &(p_ctx->memblk));
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_GENERAL, "work, new size=%u, addr=%08x", pri_buf.buf_size, pri_buf.buf_addr);
		if (pri_buf.buf_addr == 0) {
			DBG_ERR("%s create buffer failed! (%d)\r\n",  p_thisunit->unit_name, pri_buf.buf_size);
			return ISF_ERR_BUFFER_CREATE;
		}
		ctl_ise_set(p_ctx->dev_handle, CTL_ISE_ITEM_PRIVATE_BUF, (void *)&pri_buf);
	}
	p_ctx->mem.addr = pri_buf.buf_addr;
	p_ctx->mem.size = pri_buf.buf_size;
	ctl_ise_set(p_ctx->dev_handle, CTL_ISE_ITEM_APPLY, NULL);
	return r;
}
static ISF_RV _vdoprc_set_ise_exit(ISF_UNIT *p_thisunit)
{
	ISF_RV r = ISF_OK;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	DBG_FUNC("%s\r\n", p_thisunit->unit_name);

	if (p_ctx->mem.size) {
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_GENERAL, "work, release");
		//release buffer
		r = p_thisunit->p_base->do_release_i(p_thisunit, &(p_ctx->memblk), ISF_OK);
		if (r != ISF_OK) {
			DBG_ERR("%s destroy buffer failed! (%d)\r\n", p_thisunit->unit_name, r);
			return ISF_ERR_BUFFER_DESTROY;
		}

		p_ctx->mem.addr = 0;
		p_ctx->mem.size = 0;
	}

	return r;
}
static ISF_RV _vdoprc_set_ise_mode(ISF_UNIT *p_thisunit, UINT32 ipp_mode, UINT32 oport, UINT32 en)
{
	ISF_RV r;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	//UINT32 mode = ipp_mode;
	p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_GENERAL, "enable=%08x mode=%d", en, ipp_mode);

	if (1) {
#if (USE_IN_FRC == ENABLE)
		ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
#endif
		//ISF_UNIT* p_dest = NULL;
		p_ctx->outq.output_cur_en = p_ctx->outq.output_en;

		//set output image size, fmt
		if (ipp_mode == VDOPRC_PIPE_OFF) {
			UINT32 i;
#if (USE_IN_FRC == ENABLE)
			//stop input frame rate
			_isf_frc_stop(p_thisunit, ISF_IN(0), &(p_ctx->infrc[0]));
#endif
			for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++)
			{
				BOOL do_en = FALSE;
				r = _vdoprc_config_out_ise(p_thisunit, ISF_OUT(i), do_en, FALSE);
				if (r != ISF_OK)
					return r;
				_vdoprc_oport_set_enable(p_thisunit, ISF_OUT(i), do_en);
			}
		} else {
			UINT32 i;

			//init phy mask
			for (i = 0; i < ISF_VDOPRC_PHY_OUT_NUM; i++) {
				p_ctx->phy_mask |= (1<<i);
			}
			//set input image crop ratio
			r = _vdoprc_config_ise_in_crop(p_thisunit, ISF_IN(0));
			if (r != ISF_OK)
				return r;

#if (USE_IN_FRC == ENABLE)
			//start input frame rate
			if (p_vdoinfo->framepersecond == VDO_FRC_DIRECT) p_vdoinfo->framepersecond = VDO_FRC_ALL;
			p_thisunit->p_base->do_trace(p_thisunit, ISF_IN(0), ISF_OP_PARAM_VDOFRM_IMM, "frc(%d,%d)",
				GET_HI_UINT16(p_vdoinfo->framepersecond), GET_LO_UINT16(p_vdoinfo->framepersecond));
			_isf_frc_start(p_thisunit, ISF_IN(0), &(p_ctx->infrc[0]), p_vdoinfo->framepersecond);
#endif

			for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++)
			{
				if (p_ctx->phy_mask & (1<<i)) {
					BOOL do_en = FALSE;
					do_en = ((p_thisunit->port_outstate[i]->state) == (ISF_PORT_STATE_RUN)); //port current state
					if ((i + ISF_OUT_BASE) == oport) {
						do_en = en; //port new state
					}
					r = _vdoprc_config_out_ise(p_thisunit, ISF_OUT(i), do_en, FALSE);
					if (r != ISF_OK)
						return r;
					_vdoprc_oport_set_enable(p_thisunit, ISF_OUT(i), do_en);
				}
			}
		}

		if ((oport - ISF_OUT_BASE) < ISF_VDOPRC_OUT_NUM) {
			//XDATA
			VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
#if (DUMP_PERFORMANCE == ENABLE)
			UINT64 t1, t2;
			t1 = hwclock_get_longcounter();
#endif
			p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "do change mode=%d", ipp_mode);

			ctl_ise_set(p_ctx->dev_handle, CTL_ISE_ITEM_APPLY, NULL);

			if (ipp_mode == VDOPRC_PIPE_OFF) {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "flush and wait finish");
				ctl_ise_set(p_ctx->dev_handle, CTL_ISE_ITEM_FLUSH, NULL);
			}
#if (DUMP_PERFORMANCE == ENABLE)
			t2 = hwclock_get_longcounter();
			DBG_DUMP("										  CHGMODE dt=%llu us\r\n", (t2 - t1));
#endif
		}
		//if (ipp_mode == VDOPRC_PIPE_OFF)
		{
			//DBG_DUMP("~G%s\r\n", g_cur_modedata->proc_sen_id+1);
			//DBG_DUMP("~G-mode=%d\r\n", g_cur_modedata->mode);
			//    IPL_WaitCmdFinish();
		}

		//clear dirty
		{
			ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
			ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
			UINT32 i;
			//clear dirty of in[0]
			p_vdoinfo->dirty &= ~(ISF_INFO_DIRTY_ASPECT|ISF_INFO_DIRTY_IMGSIZE|ISF_INFO_DIRTY_DIRECT|ISF_INFO_DIRTY_FRAMERATE); //clear dirty
			//clear dirty of out[i]
			for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++)
			{
				if (p_ctx->phy_mask & (1<<i)) {
					//XDATA
					ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
					ISF_VDO_INFO *p_vdoinfo2 = (ISF_VDO_INFO *)(p_dest_info);
					if (p_vdoinfo2) {
					         p_vdoinfo2->dirty &= ~(ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW);    //clear dirty
					}
				}
			}
		}
	}

	//p_ctx->cur_mode = ipp_mode;
	return ISF_OK;
}
#endif
static ISF_RV _vdoprc_set_mode(ISF_UNIT *p_thisunit, UINT32 ipp_mode, UINT32 oport, UINT32 en)
{
	ISF_RV r;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	//UINT32 mode = ipp_mode;
	p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_GENERAL, "enable=%08x mode=%d", en, ipp_mode);

	if (1) {
#if (USE_IN_FRC == ENABLE)
		ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
#endif
		//ISF_UNIT* p_dest = NULL;
		p_ctx->outq.output_cur_en = p_ctx->outq.output_en;

		//set output image size, fmt
		if (ipp_mode == VDOPRC_PIPE_OFF) {
			UINT32 i;
#if (USE_IN_FRC == ENABLE)
			//stop input frame rate
			_isf_frc_stop(p_thisunit, ISF_IN(0), &(p_ctx->infrc[0]));
#endif
			for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++)
			{
				BOOL do_en = FALSE;
				_vdoprc_config_out(p_thisunit, ISF_OUT(i), do_en);
				_vdoprc_oport_set_enable(p_thisunit, ISF_OUT(i), do_en);
			}
#if (USE_IN_DIRECT == ENABLE)
			if (p_ctx->new_in_cfg_func & VDOPRC_IFUNC_DIRECT) {
				INT32 kr;
				kr = ctl_ipp_ioctl(p_ctx->dev_handle, CTL_IPP_IOCTL_SNDSTOP, NULL);

				if (kr != CTL_IPP_E_OK) {
					DBG_ERR("direct mode stop failed!(%d)\r\n",kr);
					return ISF_ERR_FAILED;
				}
			}
#endif
		} else {
			UINT32 i;

			//init phy mask
			for (i = 0; i < ISF_VDOPRC_PHY_OUT_NUM; i++) {
				p_ctx->phy_mask |= (1<<i);
			}

#if (USE_IN_DIRECT == ENABLE)
			if (p_ctx->new_in_cfg_func & VDOPRC_IFUNC_DIRECT) {
				INT32 kr;
				kr = ctl_ipp_ioctl(p_ctx->dev_handle, CTL_IPP_IOCTL_SNDSTART, NULL);

				if (kr != CTL_IPP_E_OK) {
					DBG_ERR("direct mode start failed!(%d)\r\n",kr);
					return ISF_ERR_FAILED;
				}
			}
#endif
			//set input image crop ratio
			r = _vdoprc_config_in_crop(p_thisunit, ISF_IN(0));
			if (r != ISF_OK)
				return r;
			//set input image direct
			r = _vdoprc_config_in_direct(p_thisunit, ISF_IN(0));
			if (r != ISF_OK)
				return r;

#if (USE_IN_FRC == ENABLE)
			//start input frame rate
			if (p_vdoinfo->framepersecond == VDO_FRC_DIRECT) p_vdoinfo->framepersecond = VDO_FRC_ALL;
			p_thisunit->p_base->do_trace(p_thisunit, ISF_IN(0), ISF_OP_PARAM_VDOFRM_IMM, "frc(%d,%d)",
				GET_HI_UINT16(p_vdoinfo->framepersecond), GET_LO_UINT16(p_vdoinfo->framepersecond));
			_isf_frc_start(p_thisunit, ISF_IN(0), &(p_ctx->infrc[0]), p_vdoinfo->framepersecond);
#endif

			for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++)
			{
				if (p_ctx->phy_mask & (1<<i)) {
					BOOL do_en = FALSE;
					do_en = ((p_thisunit->port_outstate[i]->state) == (ISF_PORT_STATE_RUN)); //port current state
					if ((i + ISF_OUT_BASE) == oport) {
						do_en = en; //port new state
					}
					_vdoprc_config_out(p_thisunit, ISF_OUT(i), do_en);
					_vdoprc_oport_set_enable(p_thisunit, ISF_OUT(i), do_en);
				}
			}
		}

#if defined(_BSP_NA51023_)
		//set motion-detect CB function
		g_cur_modedata->md_cb_fp = p_ctx->md_cb;
#endif

		if (ipp_mode == VDOPRC_PIPE_OFF) {
			r = _vdoprc_config_func(p_thisunit, FALSE);
			if (r != ISF_OK)
				return r;
			r = _vdoprc_config_ofunc(p_thisunit, FALSE);
			if (r != ISF_OK)
				return r;
		} else {
			r = _vdoprc_config_func(p_thisunit, TRUE);
			if (r != ISF_OK)
				return r;
			r = _vdoprc_config_ofunc(p_thisunit, TRUE);
			if (r != ISF_OK)
				return r;
		}

		if ((oport - ISF_OUT_BASE) < ISF_VDOPRC_OUT_NUM) {
			//XDATA
			VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
			UINT32 apply_id;
#if (DUMP_PERFORMANCE == ENABLE)
			UINT64 t1, t2;
			t1 = hwclock_get_longcounter();
#endif
			p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "do change mode=%d", ipp_mode);
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_APPLY, (void *)&apply_id);
			if (ipp_mode == VDOPRC_PIPE_OFF) {
				p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "flush and wait finish");
				//clear all waiting data in ctl_ipp input queue of all path
				//clear all waiting data in ctl_ipp output queue of all path
				ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_FLUSH, NULL);
			}
#if (DUMP_PERFORMANCE == ENABLE)
			t2 = hwclock_get_longcounter();
			DBG_DUMP("										  CHGMODE dt=%llu us\r\n", (t2 - t1));
#endif
		}
		//if (ipp_mode == VDOPRC_PIPE_OFF)
		{
			//DBG_DUMP("~G%s\r\n", g_cur_modedata->proc_sen_id+1);
			//DBG_DUMP("~G-mode=%d\r\n", g_cur_modedata->mode);
			//    IPL_WaitCmdFinish();
		}

		//clear dirty
		{
			ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
			ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
			UINT32 i;
			//clear dirty of in[0]
			p_vdoinfo->dirty &= ~(ISF_INFO_DIRTY_ASPECT|ISF_INFO_DIRTY_IMGSIZE|ISF_INFO_DIRTY_DIRECT|ISF_INFO_DIRTY_FRAMERATE); //clear dirty
			//clear dirty of out[i]
			for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++)
			{
				if (p_ctx->phy_mask & (1<<i)) {
					//XDATA
					ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
					ISF_VDO_INFO *p_vdoinfo2 = (ISF_VDO_INFO *)(p_dest_info);
					if (p_vdoinfo2) {
					         p_vdoinfo2->dirty &= ~(ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW);    //clear dirty
					}
				}
			}
		}
	}

	//p_ctx->cur_mode = ipp_mode;
	return ISF_OK;
}

static void _vdoprc_update_begin(ISF_UNIT *p_thisunit, UINT32 oport)
{
#if _TODO
	p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_GENERAL, "update");
#endif
}
static void _vdoprc_update_end(ISF_UNIT *p_thisunit, UINT32 oport, UINT32 do_clean)
{
	p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_VDOFRM, "do runtime change");
	if((oport - ISF_OUT_BASE) < ISF_VDOPRC_OUT_NUM) {
			//XDATA
		VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
		UINT32 apply_id;
#if (DUMP_PERFORMANCE == ENABLE)
		UINT64 t1, t2;
		t1 = hwclock_get_longcounter();
#endif
		ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_APPLY, (void *)&apply_id);
		if(do_clean) {
			CTL_IPP_FLUSH_CONFIG flush_cfg;
			UINT32 i = oport - ISF_OUT_BASE;
			//clear all waiting data in ctl_ipp output queue of path i
			flush_cfg.pid = i;
			p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_VDOFRM, "flush and wait finish - begin");
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_FLUSH, &flush_cfg);
			p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_VDOFRM, "flush and wait finish - end");
		}
#if (DUMP_PERFORMANCE == ENABLE)
		t2 = hwclock_get_longcounter();
		DBG_DUMP("										  IMGINOUT dt=%llu us\r\n", (t2 - t1));
#endif
	}
}

static ISF_RV _isf_vdoprc_do_runtime(ISF_UNIT *p_thisunit, UINT32 cmd, UINT32 oport, UINT32 en)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_RV r;
	/*
	if (cmd == ISF_PORT_CMD_RUNTIME_SYNC) {
		DBG_FUNC("%s.out[%d]=%08x rt-sync\r\n", p_thisunit->unit_name, oport - ISF_OUT_BASE, en);
	} else
	*/
	p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_GENERAL, "cmd=%x en=%x cfg=%x new=%x cur=%x", cmd,en,p_ctx->ctrl.out_cfg_func[oport - ISF_OUT_BASE],p_ctx->new_out_cfg_func[oport-ISF_OUT_BASE],p_ctx->cur_out_cfg_func[oport-ISF_OUT_BASE]);
	if (cmd == ISF_PORT_CMD_RUNTIME_UPDATE) {
		DBG_FUNC("%s.out[%d]=%08x rt-update\r\n", p_thisunit->unit_name, oport - ISF_OUT_BASE, en);
#if (USE_VPE == ENABLE)
		if (p_ctx->vpe_mode) {
			UINT32 i;
			UINT32 state, statecfg;
			ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
			ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);

			_vdoprc_config_vpe_in_crop(p_thisunit, ISF_IN(0));

#if (USE_IN_FRC == ENABLE)
			if (p_vdoinfo->framepersecond == VDO_FRC_DIRECT) p_vdoinfo->framepersecond = VDO_FRC_ALL;
			//update input frame rate
			if (p_ctx->infrc[0].sample_mode == ISF_SAMPLE_OFF) {
				_isf_frc_start(p_thisunit, ISF_IN(0), &(p_ctx->infrc[0]), p_vdoinfo->framepersecond);
			} else {
				_isf_frc_update_imm(p_thisunit, ISF_IN(0), &(p_ctx->infrc[0]), p_vdoinfo->framepersecond);
			}
#endif
			//set output image size, fmt
			i = oport - ISF_OUT_BASE;
			state = (p_thisunit->port_outstate[i]->state);
			statecfg = (p_thisunit->port_outstate[i]->statecfg);
			if (i < ISF_VDOPRC_OUT_NUM) {
				ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
				ISF_VDO_INFO *p_vdoinfo2 = (ISF_VDO_INFO *)(p_dest_info);
				if ((p_thisunit->port_outstate[i]->dirty & (ISF_PORT_CMD_OUTPUT | ISF_PORT_CMD_STATE))
					|| ((p_thisunit->port_out[i]) && (p_vdoinfo2->dirty & (ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW)))) {
					BOOL do_en = (state == (ISF_PORT_STATE_RUN)); //port current state
					if (en != 0xffffffff)
						do_en = en;
					r = _vdoprc_config_out_vpe(p_thisunit, ISF_OUT(i), do_en, TRUE);
					if (r != ISF_OK)
						return r;
					_vdoprc_oport_set_enable(p_thisunit, ISF_OUT(i), do_en);
					//clear dirty
					if (p_thisunit->port_outstate[i]->dirty & (ISF_PORT_CMD_OUTPUT | ISF_PORT_CMD_STATE)) {
						p_thisunit->port_outstate[i]->dirty &= ~(ISF_PORT_CMD_OUTPUT | ISF_PORT_CMD_STATE);
					}
					if (p_thisunit->port_out[i]) {
						p_vdoinfo2->dirty &= ~(ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW);    //clear dirty
					}
				}
			}
			if (1 || (p_vdoinfo->dirty & (ISF_INFO_DIRTY_ASPECT|ISF_INFO_DIRTY_IMGSIZE|ISF_INFO_DIRTY_DIRECT|ISF_INFO_DIRTY_FRAMERATE)) ) {
#if (USE_PULL == ENABLE)
				UINT32 do_set = (state != (ISF_PORT_STATE_RUN)) && (statecfg == (ISF_PORT_STATE_RUN));
#endif
				UINT32 do_clean = (state == (ISF_PORT_STATE_RUN)) && (statecfg != (ISF_PORT_STATE_RUN));
#if (DUMP_PERFORMANCE == ENABLE)
				UINT64 t1, t2;
				t1 = hwclock_get_longcounter();
#endif
				p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_GENERAL, "do update, do clean=%d", do_clean);

#if (USE_PULL == ENABLE)
				if (do_set) {
					_isf_vdoprc_oqueue_do_start(p_thisunit, oport);
				}
#endif
				//like _vdoprc_update_end
				ctl_vpe_set(p_ctx->dev_handle, CTL_VPE_ITEM_APPLY, NULL);
				if (do_clean) {
					CTL_VPE_FLUSH_CONFIG flush_cfg;
					//clear all waiting data in ctl_ipp output queue of path i
					flush_cfg.pid = i;
					p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "flush and wait finish - begin");
					ctl_vpe_set(p_ctx->dev_handle, CTL_VPE_ITEM_FLUSH, (void *)&flush_cfg);
					p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "flush and wait finish - end");
				}
#if (DUMP_PERFORMANCE == ENABLE)
				t2 = hwclock_get_longcounter();
				DBG_DUMP("										  IMGINOUT dt=%llu us\r\n", (t2 - t1));
#endif
#if (USE_PULL == ENABLE)
				if (do_clean) {
					_isf_vdoprc_oqueue_do_stop(p_thisunit, oport);
				}
#endif
			}
			return ISF_OK;
		}
#endif
#if (USE_ISE == ENABLE)
		if (p_ctx->ise_mode) {
			UINT32 i;
			UINT32 state, statecfg;
			ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
			ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);

			_vdoprc_config_ise_in_crop(p_thisunit, ISF_IN(0));

#if (USE_IN_FRC == ENABLE)
			if (p_vdoinfo->framepersecond == VDO_FRC_DIRECT) p_vdoinfo->framepersecond = VDO_FRC_ALL;
			//update input frame rate
			if (p_ctx->infrc[0].sample_mode == ISF_SAMPLE_OFF) {
				_isf_frc_start(p_thisunit, ISF_IN(0), &(p_ctx->infrc[0]), p_vdoinfo->framepersecond);
			} else {
				_isf_frc_update_imm(p_thisunit, ISF_IN(0), &(p_ctx->infrc[0]), p_vdoinfo->framepersecond);
			}
#endif
			//set output image size, fmt
			i = oport - ISF_OUT_BASE;
			state = (p_thisunit->port_outstate[i]->state);
			statecfg = (p_thisunit->port_outstate[i]->statecfg);
			if (i < ISF_VDOPRC_OUT_NUM) {
				ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
				ISF_VDO_INFO *p_vdoinfo2 = (ISF_VDO_INFO *)(p_dest_info);
				if ((p_thisunit->port_outstate[i]->dirty & (ISF_PORT_CMD_OUTPUT | ISF_PORT_CMD_STATE))
					|| ((p_thisunit->port_out[i]) && (p_vdoinfo2->dirty & (ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW)))) {
					BOOL do_en = (state == (ISF_PORT_STATE_RUN)); //port current state
					if (en != 0xffffffff)
						do_en = en;
					r = _vdoprc_config_out_ise(p_thisunit, ISF_OUT(i), do_en, TRUE);
					if (r != ISF_OK)
						return r;
					_vdoprc_oport_set_enable(p_thisunit, ISF_OUT(i), do_en);
					//clear dirty
					if (p_thisunit->port_outstate[i]->dirty & (ISF_PORT_CMD_OUTPUT | ISF_PORT_CMD_STATE)) {
						p_thisunit->port_outstate[i]->dirty &= ~(ISF_PORT_CMD_OUTPUT | ISF_PORT_CMD_STATE);
					}
					if (p_thisunit->port_out[i]) {
						p_vdoinfo2->dirty &= ~(ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW);    //clear dirty
					}
				}
			}
			if (1 || (p_vdoinfo->dirty & (ISF_INFO_DIRTY_ASPECT|ISF_INFO_DIRTY_IMGSIZE|ISF_INFO_DIRTY_DIRECT|ISF_INFO_DIRTY_FRAMERATE)) ) {
#if (USE_PULL == ENABLE)
				UINT32 do_set = (state != (ISF_PORT_STATE_RUN)) && (statecfg == (ISF_PORT_STATE_RUN));
#endif
				UINT32 do_clean = (state == (ISF_PORT_STATE_RUN)) && (statecfg != (ISF_PORT_STATE_RUN));
#if (DUMP_PERFORMANCE == ENABLE)
				UINT64 t1, t2;
				t1 = hwclock_get_longcounter();
#endif
				p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_GENERAL, "do update, do clean=%d", do_clean);

#if (USE_PULL == ENABLE)
				if (do_set) {
					_isf_vdoprc_oqueue_do_start(p_thisunit, oport);
				}
#endif
				//like _vdoprc_update_end
				ctl_ise_set(p_ctx->dev_handle, CTL_ISE_ITEM_APPLY, NULL);
				if (do_clean) {
					CTL_ISE_FLUSH_CONFIG flush_cfg;
					//clear all waiting data in ctl_ipp output queue of path i
					flush_cfg.pid = i;
					p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "flush and wait finish - begin");
					ctl_ise_set(p_ctx->dev_handle, CTL_ISE_ITEM_FLUSH, (void *)&flush_cfg);
					p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "flush and wait finish - end");
				}
#if (DUMP_PERFORMANCE == ENABLE)
				t2 = hwclock_get_longcounter();
				DBG_DUMP("										  IMGINOUT dt=%llu us\r\n", (t2 - t1));
#endif
#if (USE_PULL == ENABLE)
				if (do_clean) {
					_isf_vdoprc_oqueue_do_stop(p_thisunit, oport);
				}
#endif
			}
			return ISF_OK;
		}
#endif


		if (1) {
			UINT32 i;
			UINT32 state, statecfg;
			ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
			ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);

			r = _vdoprc_update_func(p_thisunit);
			if (r != ISF_OK)
				return r;
//#if (USE_OUT_DIS == ENABLE)
			if (p_ctx->new_out_cfg_func[oport - ISF_OUT_BASE]!=p_ctx->ctrl.out_cfg_func[oport - ISF_OUT_BASE]) {
				p_ctx->new_out_cfg_func[oport - ISF_OUT_BASE] = p_ctx->ctrl.out_cfg_func[oport - ISF_OUT_BASE];
			}
//#endif
			p_ctx->outq.output_cur_en = p_ctx->outq.output_en;
			{
				_vdoprc_update_begin(p_thisunit, oport);

				//set input image crop ratio
				r = _vdoprc_update_in_crop(p_thisunit, ISF_IN(0));
				if (r != ISF_OK)
					return r;
				//set input image direct
			    	r = _vdoprc_update_in_direct(p_thisunit, ISF_IN(0));
				if (r != ISF_OK)
					return r;

#if (USE_IN_FRC == ENABLE)
				if (p_vdoinfo->framepersecond == VDO_FRC_DIRECT) p_vdoinfo->framepersecond = VDO_FRC_ALL;
				//update input frame rate
				if (p_ctx->infrc[0].sample_mode == ISF_SAMPLE_OFF) {
					_isf_frc_start(p_thisunit, ISF_IN(0), &(p_ctx->infrc[0]), p_vdoinfo->framepersecond);
				} else {
					_isf_frc_update_imm(p_thisunit, ISF_IN(0), &(p_ctx->infrc[0]), p_vdoinfo->framepersecond);
				}
#endif

				//set output image size, fmt
				i = oport - ISF_OUT_BASE;
				state = (p_thisunit->port_outstate[i]->state);
				statecfg = (p_thisunit->port_outstate[i]->statecfg);
				if (i < ISF_VDOPRC_OUT_NUM) {
					ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
					ISF_VDO_INFO *p_vdoinfo2 = (ISF_VDO_INFO *)(p_dest_info);
					if ((p_thisunit->port_outstate[i]->dirty & (ISF_PORT_CMD_OUTPUT | ISF_PORT_CMD_STATE))
						|| ((p_thisunit->port_out[i]) && (p_vdoinfo2->dirty & (ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW)))) {
						BOOL do_en = (state == (ISF_PORT_STATE_RUN)); //port current state
						if (en != 0xffffffff)
							do_en = en;
						_vdoprc_update_out(p_thisunit, ISF_OUT(i), do_en);
						_vdoprc_oport_set_enable(p_thisunit, ISF_OUT(i), do_en);
						//clear dirty
						if (p_thisunit->port_outstate[i]->dirty & (ISF_PORT_CMD_OUTPUT | ISF_PORT_CMD_STATE)) {
							p_thisunit->port_outstate[i]->dirty &= ~(ISF_PORT_CMD_OUTPUT | ISF_PORT_CMD_STATE);
						}
						if (p_thisunit->port_out[i]) {
							p_vdoinfo2->dirty &= ~(ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW);    //clear dirty
						}
					}
				}
				if (1 || (p_vdoinfo->dirty & (ISF_INFO_DIRTY_ASPECT|ISF_INFO_DIRTY_IMGSIZE|ISF_INFO_DIRTY_DIRECT|ISF_INFO_DIRTY_FRAMERATE)) ) {
#if (USE_PULL == ENABLE)
					UINT32 do_set = (state != (ISF_PORT_STATE_RUN)) && (statecfg == (ISF_PORT_STATE_RUN));
#endif
					UINT32 do_clean = (state == (ISF_PORT_STATE_RUN)) && (statecfg != (ISF_PORT_STATE_RUN));
					p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_GENERAL, "do update, do clean=%d", do_clean);

#if (USE_PULL == ENABLE)
					if (do_set) {
						_isf_vdoprc_oqueue_do_start(p_thisunit, oport);
					}
#endif
					_vdoprc_update_end(p_thisunit, oport, do_clean);
#if (USE_PULL == ENABLE)
					if (do_clean) {
						_isf_vdoprc_oqueue_do_stop(p_thisunit, oport);
					}
#endif
				}
			}

//#if (USE_OUT_DIS == ENABLE)
			//if (p_ctx->outq.dis_mode) {
				p_ctx->cur_out_cfg_func[oport - ISF_OUT_BASE] = p_ctx->new_out_cfg_func[oport - ISF_OUT_BASE];
			//}
//#endif
		}
		//clear dirty
		{
			ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
			ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
			UINT32 i;
			//clear dirty of in[0]
			p_vdoinfo->dirty &= ~(ISF_INFO_DIRTY_ASPECT|ISF_INFO_DIRTY_IMGSIZE|ISF_INFO_DIRTY_DIRECT|ISF_INFO_DIRTY_FRAMERATE); //clear dirty
			//clear dirty of out[i]
			i = oport - ISF_OUT_BASE;
			if (p_ctx->phy_mask & (1<<i)) { //if physical
				//XDATA?
				ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
				ISF_VDO_INFO *p_vdoinfo2 = (ISF_VDO_INFO *)(p_dest_info);
				if (p_vdoinfo2) {
				         p_vdoinfo2->dirty &= ~(ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_IMGFMT | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW);    //clear dirty
				}
			}
		}
	}
	return ISF_OK;
}

static void _vdoprc_sleep(ISF_UNIT *p_thisunit, BOOL is_wait_finish)
{
	#if _TODO
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	#endif
	//Stop all IPL engines except SIE
	#if _TODO
	IPL_SLEEP_INFO sleepdata;
	#endif
	DBG_FUNC("%s\r\n", p_thisunit->unit_name);

	if (1) {
		#if _TODO
		//ipl_disp_start(id, DISABLE, DISABLE); //pause
		sleepdata.id = id;
		sleepdata.pnext = NULL;
		IPL_SetCmd(IPL_SET_SLEEP, (void *)&sleepdata);
		//if (is_wait_finish)
		//    IPL_WaitCmdFinish();
		#endif
	}
}
static void _vdoprc_wakeup(ISF_UNIT *p_thisunit, BOOL is_wait_finish)
{
	#if _TODO
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	#endif
	//Start all IPL engines except SIE
	#if _TODO
	IPL_WAKEUP_INFO wakeupdata;
	#endif
	DBG_FUNC("%s\r\n", p_thisunit->unit_name);

	if (1) {
		#if _TODO
		wakeupdata.id = id;
		wakeupdata.pnext = NULL;
		IPL_SetCmd(IPL_SET_WAKEUP, (void *)&wakeupdata);
		//if (is_wait_finish)
		//    IPL_WaitCmdFinish();
		//ipl_disp_start(id, ENABLE, DISABLE); //resume
		#endif
	}
}

///////////////////////////////////////////////////////////////////////////////

static ISF_RV _isf_vdoprc_do_setmode(ISF_UNIT *p_thisunit, UINT32 mode, UINT32 oport, UINT32 en);

static ISF_RV _isf_vdoprc_do_reset(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	BOOL b_reset_dev = FALSE;
	//DBG_UNIT("\r\n");
	if (p_ctx == 0) {
		return ISF_ERR_UNINIT; //still not init
	}

	//init parameter
	//if(oport == ISF_OUT_BASE) {
	//	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	//	p_ctx->p_sem_poll = &(ISF_VDOPRC_OUTP_SEM_ID); //share the same sem
	//}

	if ((oport - ISF_OUT_BASE) == (ISF_VDOPRC_OUT_NUM-1)) { //last one
		if ((_vdoprc_get_start(p_thisunit, ISF_CTRL) == 0) && (_vdoprc_get_start(p_thisunit, ISF_CTRL) == 0)) {
			b_reset_dev = TRUE;
		} else {
			DBG_ERR(" \"%s\" reset fail!\r\n", p_thisunit->unit_name);
			return ISF_ERR_FAILED;
		}
	}

	//reset ctrl state and ctrl parameter
	if(b_reset_dev) {
		//reset all id
		//isf_vdoprc_uninstall_id();
		//_isf_vdoprc_do_uninit(); //uninit all
	}
	//reset in parameter
	if(b_reset_dev) {
		ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
		memset(p_vdoinfo, 0, sizeof(ISF_VDO_INFO));
	}
	//reset out parameter
	{
		UINT32 i = oport - ISF_OUT_BASE;
		ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
		ISF_VDO_INFO *p_vdoinfo2 = (ISF_VDO_INFO *)(p_dest_info);
		memset(p_vdoinfo2, 0, sizeof(ISF_VDO_INFO));
	}
	//reset out state
	{
		UINT32 i = oport - ISF_OUT_BASE;
		ISF_STATE *p_state = p_thisunit->port_outstate[i];
		memset(p_state, 0, sizeof(ISF_STATE));
	}
	return ISF_OK;
}

static ISF_RV _isf_vdoprc_do_open(ISF_UNIT *p_thisunit, UINT32 mode, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_RV rv;
	//MEM_RANGE* p_mem_used = &(p_thisunit->mem_used);
	ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
	//DBG_UNIT("\r\n");

	nvtmpp_vb_add_module(p_thisunit->unit_module);

	if (g_vdoprc_count == 0) {
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "first open");
		if (!g_vdoprc_open) {
			g_vdoprc_open = 1;
		}
	}

	_isf_vdoprc_oqueue_do_open(p_thisunit, oport);

	p_ctx->user_out_blk[oport - ISF_OUT_BASE] = 0;
	if (mode & PIPE_COMBINE_MASK) {
		p_ctx->combine_mode = 1;
		mode &= ~PIPE_COMBINE_MASK;
	} else {
		p_ctx->combine_mode = 0;
	}
#if (USE_OUT_DIS == ENABLE)
	if (mode & 0x800) {
		p_ctx->outq.dis_mode = 1;
		mode &= ~0x800;
	} else {
		p_ctx->outq.dis_mode = 0;
	}
#if (USE_VPE == ENABLE)
	if ((mode == VDOPRC_PIPE_VPE)
		|| (mode == VDOPRC_PIPE_COLOR)
		|| (mode == VDOPRC_PIPE_WARP)) {
		p_ctx->outq.dis_mode = 0;
	}
#endif
#if (USE_ISE == ENABLE)
	if ((mode == VDOPRC_PIPE_ISE)
		|| (mode == VDOPRC_PIPE_COLOR)
		|| (mode == VDOPRC_PIPE_WARP)) {
		p_ctx->outq.dis_mode = 0;
	}
#endif
#endif

	p_ctx->new_mode = mode;
	p_ctx->new_in_cfg_func =  p_ctx->ctrl.in_cfg_func;

	if (p_ctx->cur_mode == VDOPRC_PIPE_OFF) {
		int i;
		CTL_IPP_FLOW_TYPE flow = CTL_IPP_FLOW_UNKNOWN;
		UINT32 dev_handle;

		//open iport
		CTL_IPP_REG_CB_INFO cb_info = {0};
		VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

		if (p_ctx->dev_handle != 0) {
			//DBG_ERR("- %s already opened!\r\n", p_thisunit->unit_name);
			return ISF_OK;
		}

		rv = _isf_vdoprc_ext_tsk_open();
		if (rv != ISF_OK) {
			return rv;
		}

		//support iport with auto-sync
		p_vdoinfo->sync = (ISF_INFO_DIRTY_IMGSIZE|ISF_INFO_DIRTY_IMGFMT);
		//support oport with auto-sync
		for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++) {
			ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
			ISF_VDO_INFO *p_vdoinfo2 = (ISF_VDO_INFO *)(p_dest_info);
			p_vdoinfo2->sync = (ISF_INFO_DIRTY_IMGSIZE|ISF_INFO_DIRTY_IMGFMT);
		}

#if (USE_VPE == ENABLE)
		p_ctx->vpe_mode = FALSE;
#endif
#if (USE_ISE == ENABLE)
		p_ctx->ise_mode = FALSE;
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		switch (p_ctx->new_mode) {
		case VDOPRC_PIPE_RAWALL:
			if (p_ctx->new_in_cfg_func & VDOPRC_IFUNC_DIRECT) {
				flow = CTL_IPP_FLOW_DIRECT_RAW;
			    ///< RAW * [RHE/IFE/DCE/IPE/IME] => YUV
			} else {
				flow = CTL_IPP_FLOW_RAW;
			    ///< RAW => [RHE/IFE/DCE/IPE/IME] => YUV
			}
			break;
		case VDOPRC_PIPE_RAWCAP: //running 2 times of RAWALL to gen (1) sub out (2) ime out
			flow = CTL_IPP_FLOW_CAPTURE_RAW;
			///< RAW => [RHE/IFE/DCE/IPE/IME] => YUV
			break;

/*
		case VDOPRC_PIPE_RAWALL|VDOPRC_PIPE_WARP_360:
			flow = CTL_IPP_FLOW_VR360;
			///< RAW => [RHEx2/IFEx2/DCEx2/IPE/IME] => YUV
			break;
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case VDOPRC_PIPE_YUVALL:
			if (p_ctx->new_in_cfg_func & VDOPRC_IFUNC_DIRECT) {
				flow = CTL_IPP_FLOW_DIRECT_CCIR;
			    ///< YUV * [---/---/---/IPE/IME] => YUV, cannot do in-crop, in-direct, WDR/SHDR/DEFOG, NR, GDC
			} else {
				flow = CTL_IPP_FLOW_CCIR;
			    ///< YUV => [---/---/---/IPE/IME] => YUV, cannot do in-crop, in-direct, WDR/SHDR/DEFOG, NR, GDC
			}
			break;
		case VDOPRC_PIPE_YUVCAP: //running 2 times of RAWALL to gen (1) sub out (2) ime out
			flow = CTL_IPP_FLOW_CAPTURE_CCIR;
			///< YUV => [---/---/---/IPE/IME] => YUV, cannot do in-crop, in-direct, WDR/SHDR/DEFOG, NR, GDC
			break;
		case VDOPRC_PIPE_SCALE:
			flow = CTL_IPP_FLOW_IME_D2D;
			///< YUV => [---/---/---/---/IME] => YUV, cannot do in-crop, in-direct, WDR/SHDR/DEFOG, NR, GDC, do scale only.
			break;
		case VDOPRC_PIPE_COLOR:
			flow = CTL_IPP_FLOW_IPE_D2D;
			///< YUV => [---/---/---/IPE/---] => YUV, do color only, only support 1 out.
			break;
		case VDOPRC_PIPE_WARP:
			flow = CTL_IPP_FLOW_DCE_D2D;
			///< YUV => [---/---/DCE/---/---] => YUV, do warp only, only support 1 out.
			break;
#if (USE_VPE == ENABLE)
		case VDOPRC_PIPE_VPE:
			p_ctx->vpe_mode = TRUE;
			break;
#endif
#if (USE_ISE == ENABLE)
		case VDOPRC_PIPE_ISE:
			p_ctx->ise_mode = TRUE;
			break;
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		default:
			DBG_ERR("not support pipe=%08x!\r\n", p_ctx->new_mode);
			return ISF_ERR_FAILED;
			break;
		}

		_isf_vprc_set_func_allow(p_thisunit);
		_isf_vprc_set_ifunc_allow(p_thisunit);
		_isf_vprc_set_ofunc_allow(p_thisunit);

		rv = _isf_vprc_check_func(p_thisunit);
		if (rv != ISF_OK) {
			return rv;
		}
		rv = _isf_vprc_check_ifunc(p_thisunit);
		if (rv != ISF_OK) {
			return rv;
		}

		rv = _isf_vdoprc_iport_alloc(p_thisunit);
		if (rv != ISF_OK) {
			p_ctx->dev_handle = 0; //after close
			rv = _isf_vdoprc_ext_tsk_close();
			if (rv != ISF_OK) {
				return rv;
			}
			return ISF_ERR_BUFFER_CREATE;
		}

		p_ctx->dev_trigger_open = 1;
#if 0
		p_ctx->in[0].user_count = 0; //reset self counter
#endif
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "max_fmt=%08x, config pipe=%08x", p_vdoinfo->max_imgfmt, p_ctx->new_mode);

#if (USE_VPE == ENABLE)
		if (p_ctx->vpe_mode) {

			dev_handle = ctl_vpe_open(p_thisunit->unit_name);
			if (dev_handle == 0) {
				DBG_ERR("- %s open failed!\r\n", p_thisunit->unit_name);
				return ISF_ERR_FAILED;
			}
			p_ctx->dev_handle = dev_handle;  //open end
			p_ctx->dev_ready = 1; //after open
			p_ctx->dev_trigger_close = 0;
			//set data input CB function
			cb_info.cbevt = CTL_VPE_CBEVT_IN_BUF;
			cb_info.fp = p_ctx->on_input;
			ctl_vpe_set(dev_handle, CTL_VPE_ITEM_REG_CB_IMM, (void *)&cb_info);
			//set data output CB function
			cb_info.cbevt = CTL_VPE_CBEVT_OUT_BUF;
			cb_info.fp = p_ctx->on_output;
			ctl_vpe_set(dev_handle, CTL_VPE_ITEM_REG_CB_IMM, (void *)&cb_info);
			//set max in info
			p_ctx->in[0].max_size = p_vdoinfo->max_imgsize;
			p_ctx->in[0].max_pxlfmt = p_vdoinfo->max_imgfmt;
			rv = _vdoprc_set_vpe_init(p_thisunit);
			if (rv != ISF_OK) {
				p_ctx->dev_ready = 0; //start close
				if (ctl_vpe_close(dev_handle) != CTL_IPP_E_OK) {
					DBG_ERR("- %s close vpe failed!\r\n", p_thisunit->unit_name);
				}
				p_ctx->dev_handle = 0; //after close
				_isf_vdoprc_iport_free(p_thisunit);
				return rv;
			}
		} else
#endif
#if (USE_ISE == ENABLE)
		if (p_ctx->ise_mode) {

			dev_handle = ctl_ise_open(p_thisunit->unit_name);
			if (dev_handle == 0) {
				DBG_ERR("- %s open failed!\r\n", p_thisunit->unit_name);
				return ISF_ERR_FAILED;
			}
			p_ctx->dev_handle = dev_handle;  //open end
			p_ctx->dev_ready = 1; //after open
			p_ctx->dev_trigger_close = 0;
			//set data input CB function
			cb_info.cbevt = CTL_ISE_CBEVT_IN_BUF;
			cb_info.fp = p_ctx->on_input;
			ctl_ise_set(dev_handle, CTL_ISE_ITEM_REG_CB_IMM, (void *)&cb_info);
			//set data output CB function
			cb_info.cbevt = CTL_ISE_CBEVT_OUT_BUF;
			cb_info.fp = p_ctx->on_output;
			ctl_ise_set(dev_handle, CTL_ISE_ITEM_REG_CB_IMM, (void *)&cb_info);
			//set max in info
			p_ctx->in[0].max_size = p_vdoinfo->max_imgsize;
			p_ctx->in[0].max_pxlfmt = p_vdoinfo->max_imgfmt;
			rv = _vdoprc_set_ise_init(p_thisunit);
			if (rv != ISF_OK) {
				p_ctx->dev_ready = 0; //start close
				if (ctl_ise_close(dev_handle) != CTL_IPP_E_OK) {
					DBG_ERR("- %s close ise failed!\r\n", p_thisunit->unit_name);
				}
				p_ctx->dev_handle = 0; //after close
				_isf_vdoprc_iport_free(p_thisunit);
				return rv;
			}
		} else
#endif
		{
			dev_handle = ctl_ipp_open(p_thisunit->unit_name, flow);
			if (dev_handle == 0) {
				DBG_ERR("- %s open failed!\r\n", p_thisunit->unit_name);
				return ISF_ERR_FAILED;
			}
			p_ctx->dev_handle = dev_handle;  //open end
			p_ctx->dev_ready = 1; //after open
			p_ctx->dev_trigger_close = 0;

			//set data input CB function
			cb_info.cbevt = CTL_IPP_CBEVT_IN_BUF;
			cb_info.fp = p_ctx->on_input;
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);
			//set data process CB function
			cb_info.cbevt = CTL_IPP_CBEVT_ENG_IME_ISR;
			cb_info.fp = p_ctx->on_process;
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);
			//set data output CB function
			cb_info.cbevt = CTL_IPP_CBEVT_OUT_BUF;
			cb_info.fp = p_ctx->on_output;
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);
			//set osd input CB function
			cb_info.cbevt = CTL_IPP_CBEVT_DATASTAMP;
			cb_info.fp = p_ctx->on_osd;
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);
			//set mask input CB function
			cb_info.cbevt = CTL_IPP_CBEVT_PRIMASK;
			cb_info.fp = p_ctx->on_mask;
			ctl_ipp_set(p_ctx->dev_handle, CTL_IPP_ITEM_REG_CB_IMM, (void *)&cb_info);

			rv = _vdoprc_set_init(p_thisunit);
			if (rv != ISF_OK) {
				p_ctx->dev_ready = 0; //start close
				if (ctl_ipp_close(p_ctx->dev_handle) != CTL_IPP_E_OK) {
					DBG_ERR("- %s close failed!\r\n", p_thisunit->unit_name);
				}
				p_ctx->dev_handle = 0; //after close
				rv = _isf_vdoprc_ext_tsk_close();
				if (rv != ISF_OK) {
					return rv;
				}
				_isf_vdoprc_iport_free(p_thisunit);
				return rv;
			}
		}
#if (USE_EIS == ENABLE)
		if (_vprc_eis_plugin_ && p_ctx->eis_func) {
			if (p_ctx->p_eis_info_ctx) {
				DBG_WRN("%s: p_eis_info_ctx not freed !? \r\n", p_thisunit->unit_name);
			} else {
				p_ctx->p_eis_info_ctx = isf_vdoprc_drv_malloc(sizeof(VDOPRC_EIS_PROC_INFO));
				if (p_ctx->p_eis_info_ctx == NULL) {
					DBG_ERR("%s: malloc eis ctx failed!\r\n", p_thisunit->unit_name);
					return ISF_ERR_FAILED;
				}
			}
		}
#endif
#if (STRESS_TEST == ENABLE)
		srand(time(NULL) + clock());
#endif
		g_vdoprc_count++;

		//output queue
		_vdoprc_oport_initqueue(p_thisunit);

		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "open, count=%d", g_vdoprc_count);
	}

	{
		//open oport
		//_isf_vdoprc_do_setmode(p_thisunit, p_ctx->cur_mode);
	}

	return ISF_OK;
}

#if _TODO
static ISF_RV _isf_vdoprc_do_start(ISF_UNIT *p_thisunit, UINT32 nport)
{
	if (nport-ISF_IN_BASE < p_thisunit->port_in_count) {
		//iport
	} else if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
		//oport
	}
	return ISF_OK;
}

static ISF_RV _isf_vdoprc_do_stop(ISF_UNIT *p_thisunit, UINT32 nport)
{
	if (nport-ISF_IN_BASE < p_thisunit->port_in_count) {
		//iport
	} else if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
		//oport
	}
	return ISF_OK;
}
#endif

static ISF_RV _isf_vdoprc_do_close(ISF_UNIT *p_thisunit, UINT32 mode, UINT32 oport)
{
	ISF_RV rv = ISF_OK;
#if (USE_IN_DIRECT == ENABLE)
	ISF_STATE *p_this_state = p_thisunit->port_outstate[oport - ISF_OUT_BASE];

	if (!(p_this_state->dirty & ISF_PORT_CMD_RESET)) { //if NOT under reset .... check state limit!
		VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
		UINT32 phy_open_cnt = _vdoprc_get_open(p_thisunit, ISF_CTRL-1); //get phy open count
		if (((oport - ISF_OUT_BASE) < ISF_VDOPRC_PHY_OUT_NUM) && (phy_open_cnt == 1)) { // close last phy-path
			if (p_ctx->sie_unl_cb != 0) { // vcap is still bind this vprc
				DBG_ERR("[direct] vprc cannot close last phy path before unbind from vcap!\r\n");
				return ISF_ERR_ALREADY_CONNECT;
			}
		}
	}
#endif
	//DBG_UNIT("\r\n");
	_isf_vdoprc_oqueue_do_close(p_thisunit, oport);

	if (_vdoprc_get_open(p_thisunit, ISF_CTRL) > 1)	{
		return ISF_OK; //ignore
	}
	{
		VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
		UINT32 i;

		p_ctx->dev_ready = 0; //start close
		if(p_ctx->cur_mode != VDOPRC_PIPE_OFF) {
			DBG_WRN("- %s is not stop yet???\r\n", p_thisunit->unit_name);
			_isf_vdoprc_do_setmode(p_thisunit, VDOPRC_PIPE_OFF, oport, 0);
		}
		#if (USE_OUT_DIS == ENABLE)
		if (p_ctx->outq.dis_mode) {
			_isf_vdoprc_oport_close_dis(p_thisunit);
		}
		#endif

		#if (USE_EIS == ENABLE)
		if (_vprc_eis_plugin_ && p_ctx->eis_func) {
			vprc_gyro_latency = 0;
			isf_vdoprc_drv_free(p_ctx->p_eis_info_ctx);
			p_ctx->p_eis_info_ctx = NULL;
		}
		#endif
///
		p_ctx->cur_mode = mode;
		p_ctx->cur_in_cfg_func = 0;
		for (i=0; i<ISF_VDOPRC_PHY_OUT_NUM; i++) {
			//XDATA?
			p_ctx->cur_out_cfg_func[i] = 0;
		}

		if (p_ctx->dev_handle == 0) {
			DBG_ERR("- %s is not opened!\r\n", p_thisunit->unit_name);
			rv =  ISF_ERR_NOT_OPEN_YET;
			goto vproc_release_priv_buf;
		}

		p_ctx->dev_trigger_close = 1;
#if (USE_VPE == ENABLE)
		if (p_ctx->vpe_mode) {
			if (ctl_vpe_close(p_ctx->dev_handle) != CTL_VPE_E_OK) {
				DBG_ERR("- %s close failed!\r\n", p_thisunit->unit_name);
				rv = ISF_ERR_FAILED;
				goto vproc_release_priv_buf;
			}
		} else
#endif
#if (USE_ISE == ENABLE)
		if (p_ctx->ise_mode) {
			if (ctl_ise_close(p_ctx->dev_handle) != CTL_ISE_E_OK) {
				DBG_ERR("- %s close failed!\r\n", p_thisunit->unit_name);
				rv =  ISF_ERR_FAILED;
				goto vproc_release_priv_buf;
			}
		} else
#endif

		{
			if (ctl_ipp_close(p_ctx->dev_handle) != CTL_IPP_E_OK) {
				DBG_ERR("- %s close failed!\r\n", p_thisunit->unit_name);
				rv = ISF_ERR_FAILED;
				goto vproc_release_priv_buf;
			}
		}

vproc_release_priv_buf:
#if (USE_VPE == ENABLE)
		if (p_ctx->vpe_mode) {
			_vdoprc_set_vpe_exit(p_thisunit);
		} else
#endif
#if (USE_ISE == ENABLE)
		if (p_ctx->ise_mode) {
			_vdoprc_set_ise_exit(p_thisunit);
		} else
#endif
		{
			_vdoprc_set_exit(p_thisunit);
		}
		if (rv != ISF_OK) {
			return rv;
		}
		p_ctx->dev_handle = 0; //after close

		g_vdoprc_count--;
		rv = _isf_vdoprc_ext_tsk_close();
		if (rv != ISF_OK) {
			return rv;
		}

		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "close, count=%d", g_vdoprc_count);

		_isf_vdoprc_iport_free(p_thisunit);
		p_ctx->dev_trigger_close = 0;
	}


	if (g_vdoprc_count == 0) {
		g_vdoprc_open = 0;
		p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "last close");
	}

	return ISF_OK;
}


static ISF_RV _isf_vdoprc_do_setmode(ISF_UNIT *p_thisunit, UINT32 mode, UINT32 oport, UINT32 en)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	UINT32 new_mode =  mode & ~VDOPRC_PIPE_DIRTY;
	UINT32 ipp_start_count = 0;
	ISF_RV r;
	//UINT32 new_in_cfg_func;
	//UINT32 new_out_cfg_func[5];

	UINT32 i;

#if (USE_OUT_DIS == ENABLE)
	new_mode &= ~0x800;
#endif
	p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_GENERAL, "setmode=(%d,%d) cfg=%x new=%x cur=%x", mode,en,p_ctx->ctrl.out_cfg_func[oport - ISF_OUT_BASE],p_ctx->new_out_cfg_func[oport-ISF_OUT_BASE],p_ctx->cur_out_cfg_func[oport-ISF_OUT_BASE]);

	p_ctx->new_in_cfg_func =  p_ctx->ctrl.in_cfg_func;
	for (i=0; i<ISF_VDOPRC_PHY_OUT_NUM; i++) {
		//XDATA?
		p_ctx->new_out_cfg_func[i] = p_ctx->ctrl.out_cfg_func[i];
	}

	ipp_start_count = _vdoprc_get_start(p_thisunit, ISF_CTRL);

	if(en == 0) {
		if ((p_thisunit->port_outstate[oport - ISF_OUT_BASE]->state == ISF_PORT_STATE_RUN)) {
			_isf_vdoprc_oqueue_force_stop(p_thisunit, oport); //force unlock to cancel pull_out()
			//here call _isf_queue2_force_stop()
			//{if still pull_out() => must wait pull_out() finish first!}
			// wait _isf_queue2_get_data() finish first! or .... force it finish first!
		}
		if ((ipp_start_count == 1) && (p_thisunit->port_outstate[oport - ISF_OUT_BASE]->state == ISF_PORT_STATE_RUN)) {
			new_mode = VDOPRC_PIPE_OFF;
		}
	} else {
	}
#if (USE_VPE == ENABLE)
	if (p_ctx->vpe_mode) {
		if ((oport - ISF_OUT_BASE) >= VDOPRC_MAX_VPE_OUT_NUM && (oport - ISF_OUT_BASE) < VDOPRC_MAX_PHY_OUT_NUM) {
			DBG_ERR("%s with vpe NOT support out[%d]\r\n", p_thisunit->unit_name, VDOPRC_MAX_VPE_OUT_NUM);
			return ISF_ERR_NOT_SUPPORT;
		}
	}
#endif
	p_ctx->err_cnt = 0; //reset err

	//DBG_UNIT("\r\n");
	p_thisunit->p_base->do_trace(p_thisunit, oport, ISF_OP_PARAM_GENERAL, "enable=%d mode=(%d->%d)", en, p_ctx->cur_mode, new_mode);
	if ((p_ctx->cur_mode == new_mode) && !(mode & VDOPRC_PIPE_DIRTY)) {
		//DBG_MSG("^Y(UPDATE)\r\n");
		r = _isf_vdoprc_check_statelimit(p_thisunit, oport, 1, en);
		if (r != ISF_OK)
			return r;
		_isf_vdoprc_do_runtime(p_thisunit, ISF_PORT_CMD_RUNTIME_UPDATE, oport, en);
		return ISF_OK;
	}

	if (new_mode == VDOPRC_PIPE_OFF) {
		r = _isf_vdoprc_check_statelimit(p_thisunit, oport, 1, en);
		if (r != ISF_OK)
			return r;
		//DBG_MSG("^Y(OFF)\r\n");
#if (USE_VPE == ENABLE)
		if (p_ctx->vpe_mode) {
			r = _vdoprc_set_vpe_mode(p_thisunit, VDOPRC_PIPE_OFF, oport, en);
		} else
#endif
#if (USE_ISE == ENABLE)
		if (p_ctx->ise_mode) {
			r = _vdoprc_set_ise_mode(p_thisunit, VDOPRC_PIPE_OFF, oport, en);
		} else
#endif
		{
			r = _vdoprc_set_mode(p_thisunit, VDOPRC_PIPE_OFF, oport, en);
		}
		if (r != ISF_OK)
			return r;
		p_ctx->cur_mode = new_mode;
		p_ctx->cur_in_cfg_func = 0;
		for (i=0; i<ISF_VDOPRC_PHY_OUT_NUM; i++) {
			//XDATA?
			p_ctx->cur_out_cfg_func[i] = 0;
		}

		_isf_vdoprc_oqueue_cancel_poll(p_thisunit); //auto-unlock poll, if no path started
#if (USE_PULL == ENABLE)
		{
			_isf_vdoprc_oqueue_do_stop(p_thisunit, oport);
		}
#endif
	} else {
		//DBG_MSG("^Y(ON)\r\n");

		{
			UINT32 func_max, func;
			func_max = p_ctx->ctrl.func_max;
			func = p_ctx->ctrl.cur_func;
			if ((func & (~func_max)) != 0) {
				DBG_ERR("- func=%08x is more then func_max=%08x???\r\n", func, func_max);
				return ISF_ERR_FAILED;
			}
		}
		r = _isf_vdoprc_check_statelimit(p_thisunit, oport, 0, en);
		if (r != ISF_OK)
			return r;
#if (USE_PULL == ENABLE)
		{
			_isf_vdoprc_oqueue_do_start(p_thisunit, oport);
		}
#endif
#if (USE_VPE == ENABLE)
		if (p_ctx->vpe_mode) {
			r = _vdoprc_set_vpe_mode(p_thisunit, new_mode, oport, en);
		} else
#endif
#if (USE_ISE == ENABLE)
		if (p_ctx->ise_mode) {
			r = _vdoprc_set_ise_mode(p_thisunit, new_mode, oport, en);
		} else
#endif
		{
			r = _vdoprc_set_mode(p_thisunit, new_mode, oport, en);
		}

		if (r != ISF_OK)
			return r;

		p_ctx->cur_mode = new_mode;
		p_ctx->cur_in_cfg_func = p_ctx->new_in_cfg_func;
		for (i=0; i<ISF_VDOPRC_PHY_OUT_NUM; i++) {
			//XDATA?
			p_ctx->cur_out_cfg_func[i] = p_ctx->new_out_cfg_func[i];
		}
	}
	/*
	*p_mem = g_IPPFree; //load free memory
	*/
	return ISF_OK;
}

static ISF_RV _isf_vdoprc_do_sleep(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	//DBG_UNIT("\r\n");

	if (p_ctx->sleep != TRUE) {
#if _TODO
		g_cur_modedata = &g_modedata[id];
		if (g_cur_modedata->mode != IPL_MODE_OFF) {
#else
		if (0) {
#endif
			UINT32 run_n = 0;
			//get total running count
#if defined(_BSP_NA51000_) || defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
			UINT32 i;
			for (i = 0; i < ISF_VDOPRC_PHY_OUT_NUM; i++)
#endif
#if defined(_BSP_NA51023_)
			UINT32 i;
			for (i = 0; i < (ISF_VDOPRC_PHY_OUT_NUM-1); i++)
#endif
			{
				if (p_thisunit->port_out[i] != NULL)
					if ((p_thisunit->port_outstate[i]->state) == (ISF_PORT_STATE_RUN)) {
						run_n++;
					}
			}
			//if this path is last running path
			i = oport - ISF_OUT_BASE;
			if ((run_n == 1) && (p_thisunit->port_out[i] != NULL) && ((p_thisunit->port_outstate[i]->state) == (ISF_PORT_STATE_RUN))) {
				_vdoprc_sleep(p_thisunit, TRUE);
				p_ctx->sleep = TRUE;
			}
		}
	}
	return ISF_OK;
}
static ISF_RV _isf_vdoprc_do_wakeup(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	//DBG_UNIT("\r\n");
	if (p_ctx->sleep == TRUE) {
		_vdoprc_wakeup(p_thisunit, TRUE);
		p_ctx->sleep = FALSE;
	}
	return ISF_OK;
}

static ISF_RV _isf_vdoprc_do_abort(ISF_UNIT *p_thisunit)
{
#if defined(_BSP_NA51089_)
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	//DBG_UNIT("\r\n");
	if (p_ctx) {
		INT32 kr;
		kr = ctl_ipp_ioctl(p_ctx->dev_handle, CTL_IPP_IOCTL_DMA_ABORT, NULL);
		if (kr != CTL_IPP_E_OK) {
			DBG_ERR("DMA abort failed!(%d)\r\n", kr);
			return ISF_ERR_FAILED;
		}
	}
	return ISF_OK;
#else
	DBG_ERR("not support DMA abort!\r\n");
	return ISF_ERR_FAILED;
#endif
}

ISF_RV _isf_vdoprc_do_setparam(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 value)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_UNINIT; //still not init
	}
	p_thisunit->p_base->do_trace(p_thisunit, nport, ISF_OP_PARAM_VDOFRM, "setparam=0x%x 0x%x", param,value);
	if ((nport < (int)ISF_IN_BASE) && ((nport-ISF_OUT_BASE) < p_thisunit->port_out_count)) {
		switch (param) {
		case ISF_PARAM_PORT_DIRTY:
			{
#if 0
			if (value & ISF_INFO_DIRTY_FRAMERATE) {
				return >ISF_APPLY_DOWNSTREAM_READYTIME;
			} else {
				return ISF_OK;
			}
#else
				return ISF_OK;
#endif
			}
			break;
#if 0
		case VDOPRC_PARAM_SNAPSHOT_IMM:
			break;
#endif
		case VDOPRC_PARAM_CODEC:
			if((nport-ISF_OUT_BASE) < ISF_VDOPRC_OUT_NUM) {
				p_ctx->codec[nport-ISF_OUT_BASE] = value;
			}
			return ISF_OK;
			break;
		case VDOPRC_PARAM_OUT_FRC_IMM:
#if (USE_OUT_FRC == ENABLE)
			if((nport-ISF_OUT_BASE) < ISF_VDOPRC_OUT_NUM) {
				if (value == VDO_FRC_DIRECT) value = VDO_FRC_ALL;
				_isf_frc_update_imm(p_thisunit, nport, &(p_ctx->outfrc[nport-ISF_OUT_BASE]), value);
			}
#endif
			return ISF_OK;
			break;
		case VDOPRC_PARAM_OUT_CFG_FUNC:
			if((nport-ISF_OUT_BASE) < ISF_VDOPRC_OUT_NUM) {
				p_ctx->ctrl.out_cfg_func[nport-ISF_OUT_BASE] = value;
			}
			return ISF_OK;
			break;
		case VDOPRC_PARAM_HEIGHT_ALIGN:
			if((nport-ISF_OUT_BASE) < ISF_VDOPRC_OUT_NUM) {
				p_ctx->out_h_align[nport-ISF_OUT_BASE] = value;
			}
			return ISF_OK;
			break;
		case VDOPRC_PARAM_TRIG_USER_CROP:
			p_thisunit->p_base->do_trace(p_thisunit, ISF_CTRL, ISF_OP_PARAM_CTRL, "user_crop_param=%d", value);
			if ((p_ctx->cur_in_cfg_func & VDOPRC_IFUNC_DIRECT) && (p_ctx->ctrl.cur_func & VDOPRC_FUNC_3DNR)) {
				p_ctx->user_crop_trig = 1;
				p_ctx->user_crop_param = value;
				return ISF_OK;
			} else {
				DBG_ERR("%s.out[%d] USER_CROP_TRIG only support in direct mode!\r\n", p_thisunit->unit_name, nport-ISF_OUT_BASE);
				return ISF_ERR_NOT_SUPPORT;
			}
		case VDOPRC_PARAM_OUT_ORDER:
			if((nport-ISF_OUT_BASE) < ISF_VDOPRC_OUT_NUM) {
				p_ctx->out_order[nport-ISF_OUT_BASE] = value;
			}
			return ISF_OK;
			break;
		case VDOPRC_PARAM_DIS_CROP_ALIGN:
			if((nport-ISF_OUT_BASE) <  ISF_VDOPRC_PHY_OUT_NUM) {
				p_ctx->outq.dis_addr_align[nport-ISF_OUT_BASE] = value;
			}
			return ISF_OK;
			break;
		case VDOPRC_PARAM_COMBINE_BLK:
			if((nport-ISF_OUT_BASE) < ISF_VDOPRC_OUT_NUM) {
				p_ctx->user_out_blk[nport-ISF_OUT_BASE] = (INT32)value;
			}
			return ISF_OK;
		case VDOPRC_PARAM_LOFF_ALIGN:
			if((nport-ISF_OUT_BASE) < ISF_VDOPRC_OUT_NUM) {
				p_ctx->out_loff_align[nport-ISF_OUT_BASE] = value;
			}
			return ISF_OK;
			break;
		case VDOPRC_PARAM_OUT_VNDCFG_FUNC:
			if((nport-ISF_OUT_BASE) < ISF_VDOPRC_OUT_NUM) {
				p_ctx->out_vndcfg_func[nport-ISF_OUT_BASE] = value;

			}
			return ISF_OK;
			break;
		case VDOPRC_PARAM_DIS_COMPENSATE:
			if((nport-ISF_OUT_BASE) < ISF_VDOPRC_PHY_OUT_NUM) {
				p_ctx->outq.dis_compensation[nport-ISF_OUT_BASE] = value;
			}
			return ISF_OK;
			break;
			
		default:
			DBG_ERR("%s.out[%d] set param[%08x] = %d\r\n", p_thisunit->unit_name, nport-ISF_OUT_BASE, param, value);
			break;
		}
	} else if (nport-ISF_IN_BASE < p_thisunit->port_in_count) {
		switch (param) {
		case ISF_PARAM_PORT_DIRTY:
			{
				return ISF_OK;
			}
			break;
		case VDOPRC_PARAM_IN_DEPTH:
			if((nport-ISF_IN_BASE) < ISF_VDOPRC_IN_NUM) {
				_vdoprc_iport_setqueuecount(p_thisunit, value); //set max in depth
			}
			return ISF_OK;
			break;
#if (USE_OUT_DIS == ENABLE)
		case VDOPRC_PARAM_DIS_SCALERATIO:
			if((nport-ISF_IN_BASE) < ISF_VDOPRC_IN_NUM) {
				p_ctx->outq.dis_cfg_scaleratio = value;
			}
			return ISF_OK;
			break;
		case VDOPRC_PARAM_DIS_SUBSAMPLE:
			if((nport-ISF_IN_BASE) < ISF_VDOPRC_IN_NUM) {
                // for distinguish set subsample 0
				p_ctx->outq.dis_cfg_subsample = value|VPRC_DIS_SUBSAMPLE_CFG;
			}
			return ISF_OK;
			break;
#endif
		case VDOPRC_PARAM_IN_FRC_IMM:
#if (USE_IN_FRC == ENABLE)
			if((nport-ISF_IN_BASE) < ISF_VDOPRC_IN_NUM) {
				if (value == VDO_FRC_DIRECT) value = VDO_FRC_ALL;
				_isf_frc_update_imm(p_thisunit, nport, &(p_ctx->infrc[nport-ISF_IN_BASE]), value);
			}
#endif
			return ISF_OK;
			break;
		case VDOPRC_PARAM_IN_CFG_FUNC:
			if (nvtmpp_is_dynamic_map() && (value & VDOPRC_IFUNC_DIRECT)) {
				DBG_ERR("Dynamic map memory not support VCAP-VPRC Direct\r\n");
				return ISF_ERR_NOT_SUPPORT;
			}
			if((nport-ISF_IN_BASE) < ISF_VDOPRC_IN_NUM) {
				p_ctx->ctrl.in_cfg_func = value;
			}
			return ISF_OK;
			break;
		default:
			DBG_ERR("%s.in[%d] set param[%08x] = %d\r\n", p_thisunit->unit_name, nport-ISF_IN_BASE, param, value);
			break;
		}
	} else if (nport == ISF_CTRL) {

		switch (param) {
		case VDOPRC_PARAM_DMA_ABORT:
			return _isf_vdoprc_do_abort(p_thisunit);
			break;
		case VDOPRC_PARAM_IQ_ID:
			p_ctx->ctrl.iq_id = value;
			return ISF_OK;
			break;
		case VDOPRC_PARAM_PIPE:
			p_ctx->ctrl.pipe = value;
#if (DUMP_PARAM == ENABLE)
			DBG_FUNC("%s set pipe=%08x\r\n", p_thisunit->unit_name, value);
#endif
			return ISF_OK;
			break;
		case VDOPRC_PARAM_NVX_CODEC:
			p_ctx->nvxcodec = value;
			return ISF_OK;
			break;
#if defined(_BSP_NA51023_)
		case VDOPRC_PARAM_VIEWTRACKING_CB:
			#if _TODO
			p_ctx->on_viewtrack = (IPL_VIEWTRACKING_CB)value;
			#endif
			return ISF_OK;
			break;
#endif
		case VDOPRC_PARAM_FUNC:
			p_ctx->ctrl.func = (CTL_IPP_FUNC)value;
			return ISF_OK;
			break;
#if _TODO
		case VDOPRC_PARAM_FUNC2:
			p_ctx->ctrl.func2 = value;
			return ISF_OK;
			break;
#endif
		case VDOPRC_PARAM_MAX_FUNC:
			p_ctx->ctrl.func_max = (CTL_IPP_FUNC)value;
			return ISF_OK;
			break;
#if _TODO
		case VDOPRC_PARAM_MAX_FUNC2:
			p_ctx->ctrl.func2_max = value;
			return ISF_OK;
			break;
#endif
		case VDOPRC_PARAM_COLORSPACE:
			p_ctx->ctrl.color.space = value;
			return ISF_OK;
			break;
		case VDOPRC_PARAM_SCALEMATHOD_CTRL:
			p_ctx->ctrl.scale.scl_th = (UINT32)value;
			return ISF_OK;
			break;
		case VDOPRC_PARAM_SCALEMATHOD_SMALL:
			p_ctx->ctrl.scale.method_l = (UINT32)value;
			return ISF_OK;
			break;
		case VDOPRC_PARAM_SCALEMATHOD_LARGE:
			p_ctx->ctrl.scale.method_h = (UINT32)value;
			return ISF_OK;
			break;
		case VDOPRC_PARAM_3DNR_REFPATH:
			p_ctx->ctrl._3dnr_refpath = value;
			return ISF_OK;
			break;
		case VDOPRC_PARAM_LOWLATENCY_TRIG:
			p_ctx->ctrl._lowlatency_trig = value;
			return ISF_OK;
			break;
		case VDOPRC_PARAM_STRIP:
			p_ctx->ctrl.stripe_rule = (UINT32)value;
			return ISF_OK;
			break;
#if (USE_EIS == ENABLE)
		case VDOPRC_PARAM_EIS_FUNC:
			p_ctx->eis_func = (UINT32)value;
			return ISF_OK;
			break;
#endif
#if (USE_VPE == ENABLE)
		case VDOPRC_PARAM_VPE_SCENE:
			p_ctx->vpe_scene = (UINT32)value;
			return ISF_OK;
			break;
#endif
#if _TODO
		case VDOPRC_PARAM_IPL_CB:
			p_ctx->on_input = (IPP_EVENT_FP)value;
			return ISF_OK;
			break;
		case VDOPRC_PARAM_EVENT_CB:
			p_ctx->on_proc = (IPP_EVENT_FP)value;
			return ISF_OK;
			break;
#endif
#if defined(_BSP_NA51023_)
		case VDOPRC_PARAM_MOTIONDETECT_CB:
			p_ctx->md_cb = (IPL_MD_CB)value;
			return ISF_OK;
			break;
#endif
		default:
			DBG_ERR("%s.ctrl set param[%08x] = %d\r\n", p_thisunit->unit_name, param, value);
			break;
		}
		return ISF_ERR_NOT_SUPPORT;
	}
	return ISF_ERR_NOT_SUPPORT;
}

#if _TODO
static FPS_PARAM _ipl_fps_param;
#endif
UINT32 _isf_vdoprc_do_getparam(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_UNINIT; //still not init
	}
	if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
		switch (param) {
		case VDOPRC_PARAM_OUT_CFG_FUNC:
			if((nport-ISF_OUT_BASE) < ISF_VDOPRC_OUT_NUM) {
				return (UINT32)p_ctx->cur_out_cfg_func[nport-ISF_OUT_BASE];
			} else {
				return 0;
			}
			break;
		default:
			DBG_ERR("%s.out[%d] get param[%08x]\r\n", p_thisunit->unit_name, nport-ISF_OUT_BASE, param);
			break;
		}
	} else if (nport-ISF_IN_BASE < p_thisunit->port_in_count) {
		switch (param) {
#if (USE_IN_DIRECT == ENABLE)
		case VDOPRC_PARAM_DIRECT_CB:
			if (p_ctx->new_in_cfg_func & VDOPRC_IFUNC_DIRECT) {
				return (UINT32)ctl_ipp_get_dir_fp(0);
			} else {
				return 0;
			}
			break;
#endif
#if (USE_EIS == ENABLE)
		case VDOPRC_PARAM_EIS_TRIG_CB:
			if (p_ctx->new_in_cfg_func & VDOPRC_IFUNC_DIRECT) {
				return (ULONG)_isf_vdoprc_eis_trig_cb;
			} else {
				return 0;
			}
			break;
#endif
		case VDOPRC_PARAM_IN_CFG_FUNC:
			if((nport-ISF_IN_BASE) < ISF_VDOPRC_IN_NUM) {
				return (UINT32)p_ctx->cur_in_cfg_func;
			} else {
				return 0;
			}
			break;
		default:
			DBG_ERR("%s.in[%d] get param[%08x]\r\n", p_thisunit->unit_name, nport-ISF_IN_BASE, param);
			break;
		}
	} else if (nport == ISF_CTRL) {

		switch (param) {
		case VDOPRC_PARAM_PIPE:
			return p_ctx->cur_mode;
			break;
#if 0
		case VDOPRC_PARAM_SENSORID_IMM:
			break;
#endif
		case VDOPRC_PARAM_FUNC:
			return (UINT32)p_ctx->ctrl.func;
			break;
#if _TODO
		case VDOPRC_PARAM_FUNC2:
			return p_ctx->ctrl.func2;
			break;
#endif
		default:
			DBG_ERR("%s.ctrl get param[%08x]\r\n", p_thisunit->unit_name, param);
			break;
		}
		return 0;
	}
	return ISF_ERR_NOT_SUPPORT;
}

ISF_RV _isf_vdoprc_do_setparamstruct(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size)
{
	ISF_RV ret = ISF_OK;
	UINT32 value = (UINT32)p_struct;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

	if (p_ctx == 0) {
		return ISF_ERR_UNINIT; //still not init
	}
	if ((nport < (int)ISF_IN_BASE) && ((nport-ISF_OUT_BASE) < p_thisunit->port_out_count)) {
		UINT32 path = nport-ISF_OUT_BASE;

		/* for output port */
		switch (param) {
#if (USE_VPE == ENABLE)
		case VDOPRC_PARAM_VPE_PRE_SCL_CROP:
			if (!p_ctx->vpe_mode) {
				DBG_ERR("pre_scl_crop only for VPE\r\n");
				return ISF_ERR_NOT_SUPPORT;
			} else {
				PURECT p_crop = (PURECT)p_struct;

				if (path >= ISF_VDOPRC_VPE_OUT_NUM) {
					return ISF_ERR_INVALID_PARAM;
				}
				p_ctx->pre_scl_crop[path].x = p_crop->x;
				p_ctx->pre_scl_crop[path].y = p_crop->y;
				p_ctx->pre_scl_crop[path].w = p_crop->w;
				p_ctx->pre_scl_crop[path].h = p_crop->h;
				return ISF_OK;
			}
		case VDOPRC_PARAM_VPE_HOLE_REGION:
			if (!p_ctx->vpe_mode) {
				DBG_ERR("hole_region only for VPE\r\n");
				return ISF_ERR_NOT_SUPPORT;
			} else {
				PURECT p_hole = (PURECT)p_struct;

				if (path >= ISF_VDOPRC_VPE_OUT_NUM) {
					return ISF_ERR_INVALID_PARAM;
				}
				p_ctx->hole_region[path].x = p_hole->x;
				p_ctx->hole_region[path].y = p_hole->y;
				p_ctx->hole_region[path].w = p_hole->w;
				p_ctx->hole_region[path].h = p_hole->h;
				return ISF_OK;
			}
#endif
		case VDOPRC_PARAM_OUT_REGION: {
				ISF_VDO_WIN *p_out_win = (ISF_VDO_WIN *)p_struct;

				if (path >= ISF_VDOPRC_OUT_NUM) {
					return ISF_ERR_INVALID_PARAM;
				}
				p_ctx->out_win[path].imgaspect.w = p_out_win->imgaspect.w;
				p_ctx->out_win[path].imgaspect.h = p_out_win->imgaspect.h;
				p_ctx->out_win[path].window.x = p_out_win->window.x;
				p_ctx->out_win[path].window.y = p_out_win->window.y;
				p_ctx->out_win[path].window.w = p_out_win->window.w;
				p_ctx->out_win[path].window.h = p_out_win->window.h;
				return ISF_OK;
			}
		default:
			DBG_ERR("vdoprc.ctrl set struct[0x%08x] = %08x\r\n", param, value);
			break;
		}


	} else if (nport == ISF_CTRL) {
		switch (param) {
		case VDOPRC_PARAM_POLL_LIST:
			return _isf_vdoprc_oqueue_do_poll_list(p_thisunit, (VDOPRC_POLL_LIST *)value);
			break;
		case VDOPRC_PARAM_USER_OUT_BUF:
			if (size != (sizeof(p_ctx->user_out_blk) + sizeof(p_ctx->user_out_blk_size))) {
				DBG_ERR("user out context size(%d+%d) not matched(%d)\r\n", sizeof(p_ctx->user_out_blk), sizeof(p_ctx->user_out_blk_size), size);
				return ISF_ERR_FAILED;
			} else {
				VDOPRC_USER_OUT_BUF *p_user_out = (VDOPRC_USER_OUT_BUF *)p_struct;
				memcpy(p_ctx->user_out_blk, p_user_out->blk, sizeof(p_ctx->user_out_blk));
				memcpy(p_ctx->user_out_blk_size, p_user_out->size, sizeof(p_ctx->user_out_blk_size));
				#if 0//debug dump
				{
					UINT32 i;
					for (i = 0; i < ISF_VDOPRC_OUT_NUM; i++) {
						if (p_ctx->user_out_blk[i]) {
							UINT32 va, pa;
							va = nvtmpp_vb_blk2va(p_ctx->user_out_blk[i]);
							pa = nvtmpp_sys_va2pa(va);
							DBG_DUMP("[%d]out[%d] blk=0x%X, va=0x%X, pa=0x%X\r\n", p_ctx->dev ,i , p_ctx->user_out_blk[i], va, pa);
						} else {
							break;
						}
					}
				}
				#endif
				return ISF_OK;
			}
		default:
			DBG_ERR("vdoprc.ctrl set struct[0x%08x] = %08x\r\n", param, value);
			break;
		}

		return ret;
    } // if (nport == ISF_CTRL)

	return ISF_ERR_NOT_SUPPORT;
}

ISF_RV _isf_vdoprc_do_getparamstruct(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size)
{
	ISF_RV ret = ISF_OK;
	UINT32 value = (UINT32)p_struct;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	if (p_ctx == 0) {
		return ISF_ERR_UNINIT; //still not init
	}

    if (nport == ISF_CTRL)
    {
		switch (param) {
		case VDOPRC_PARAM_POLL_LIST:
			return _isf_vdoprc_oqueue_get_poll_mask(p_thisunit, (VDOPRC_POLL_LIST *)value);
			break;
		case VDOPRC_PARAM_SYS_CAPS:
			{
				VDOPRC_SYS_CAPS* p_caps = (VDOPRC_SYS_CAPS *)value;
				p_caps->max_in_count = 1;
				p_caps->max_out_count = VDOPRC_MAX_OUT_NUM;
			}
			return ISF_OK;
			break;
		default:
			DBG_ERR("vdoprc.ctrl set struct[0x%08x] = %08x\r\n", param, value);
			break;
		}

		return ret;
    } // if (nport == ISF_CTRL)

	return ISF_ERR_NOT_SUPPORT;
}


ISF_RV _isf_vdoprc_do_updateport(struct _ISF_UNIT *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd)
{
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	ISF_RV r = ISF_OK;
	if (p_ctx == 0) {
		return ISF_ERR_UNINIT; //still not init
	}
	if (oport == ISF_CTRL) {
		switch (cmd) {
		case ISF_PORT_CMD_QUERY:
			r = (_vdoprc_get_start(p_thisunit, ISF_CTRL-1) > 0)?(ISF_OK):(ISF_ERR_NOT_SUPPORT); //query count of start phy path
			break;
		default:
			break;
		}
		return r;
	}
	switch (cmd) {
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case ISF_PORT_CMD_RESET:
		r = _isf_vdoprc_do_reset(p_thisunit, oport);
		break;
	case ISF_PORT_CMD_OFFTIME_SYNC:     ///< off -> off     (sync port info & sync off-time parameter if it is dirty)
		//DBG_DUMP("~C%s OSYNC\r\n", p_thisunit->unit_name);
		#if 0
		_isf_vdoprc_do_offsync(p_thisunit, oport);
		#endif
		break;
	case ISF_PORT_CMD_OPEN:             ///< off -> ready   (alloc mempool and start task)
		//DBG_DUMP("~C%s OPEN\r\n", p_thisunit->unit_name);
		r = _isf_vdoprc_do_open(p_thisunit, p_ctx->ctrl.pipe, oport);
		break;
	case ISF_PORT_CMD_READYTIME_SYNC:   ///< ready -> ready (sync port info & sync ready-time parameter if it is dirty)
		break;
	case ISF_PORT_CMD_START:            ///< ready -> run   (initial context, engine enable and device power on, start data processing)
		//DBG_DUMP("~C%s START mode=%d\r\n", p_thisunit->unit_name, p_ctx->new_mode);
		r = _isf_vdoprc_do_setmode(p_thisunit, p_ctx->new_mode, oport, 1);
		break;
	case ISF_PORT_CMD_RUNTIME_SYNC:     ///< run -> run     (sync port info & sync run-time parameter if it is dirty)
		//DBG_DUMP("~C%s RSYNC\r\n", p_thisunit->unit_name);
		#if 0
		r = _isf_vdoprc_do_runtime(p_thisunit, cmd, oport, 0xffffffff);
		#endif
		break;
	case ISF_PORT_CMD_RUNTIME_UPDATE:   ///< run -> run     (update change)
		//DBG_DUMP("~C%s vdoprcd RUPD\r\n", p_thisunit->unit_name);
		r = _isf_vdoprc_do_runtime(p_thisunit, cmd, oport, 0xffffffff);
		break;
	case ISF_PORT_CMD_STOP:             ///< run -> stop    (stop data processing, engine disable or device power off)
		//DBG_DUMP("~C%s STOP\r\n", p_thisunit->unit_name);
		r = _isf_vdoprc_do_setmode(p_thisunit, p_ctx->cur_mode, oport, 0);
		break;
	case ISF_PORT_CMD_CLOSE:            ///< stop -> off    (terminate task and free mempool)
		//DBG_DUMP("~C%s CLOSE\r\n", p_thisunit->unit_name);
		r = _isf_vdoprc_do_close(p_thisunit, VDOPRC_PIPE_OFF, oport);
		break;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case ISF_PORT_CMD_PAUSE:            ///< run -> wait    (pause data processing, keep context)
		break;
	case ISF_PORT_CMD_RESUME:           ///< wait -> run    (restore context, resume data processing)
		break;
	case ISF_PORT_CMD_SLEEP:            ///< run -> sleep   (pause data processing, keep context, engine or device enter suspend mode)
		r = _isf_vdoprc_do_sleep(p_thisunit, oport);
		break;
	case ISF_PORT_CMD_WAKEUP:           ///< sleep -> run   (engine or device leave suspend mode, restore context, resume data processing)
		r = _isf_vdoprc_do_wakeup(p_thisunit, oport);
		break;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	default:
		break;
	}
	return r;
}

#if (USE_EIS == ENABLE)
VDOPRC_EIS_PLUGIN *_vprc_eis_plugin_ = 0;
VDOPRC_LUT2D_INFO lut2d_buf_info[MAX_EIS_PATH_NUM] = {0};
UINT32 vprc_gyro_latency = 0;
UINT32 vprc_gyro_data_num = 0;
static UINT64 gyro_frame_cnt[VDOPRC_IN_DEPTH_MAX] = {0};
static UINT32 gyro_queue_idx = 0;
void isf_vdoprc_put_gyro_frame_cnt(UINT64 frame_cnt)
{
	gyro_frame_cnt[gyro_queue_idx] = frame_cnt;
	//DBG_DUMP("[PUT]gyro_frame_cnt[%d]=%llu\r\n", gyro_queue_idx, gyro_frame_cnt[gyro_queue_idx]);
	gyro_queue_idx++;
	if (gyro_queue_idx >= VDOPRC_IN_DEPTH_MAX) {
		gyro_queue_idx = 0;
	}
}
BOOL isf_vdoprc_check_frame_with_gyro(UINT64 frame_cnt)
{
	UINT32 i;

	//DBG_DUMP("[CHECK %llu] gyro_frame_cnt=", frame_cnt);
	for (i = 0; i < VDOPRC_IN_DEPTH_MAX; i++) {
		//DBG_DUMP("%llu ", gyro_frame_cnt[i]);
		if (gyro_frame_cnt[i] == frame_cnt) {
			return TRUE;
		}
	}
	//DBG_DUMP("\r\n");
	return FALSE;
}
#endif

INT32 isf_vdoprc_reg_eis_plugin(VDOPRC_EIS_PLUGIN* p_eis_plugin)
{
#if (USE_EIS == ENABLE)
	if ((_vprc_eis_plugin_ == 0) && (p_eis_plugin != 0)) {
		_vprc_eis_plugin_ = p_eis_plugin;
		DBG_DUMP("vprc eis plugin register ok!\r\n");
		return 0;
	}
	if ((_vprc_eis_plugin_ != 0) && (p_eis_plugin == 0)) {
		_vprc_eis_plugin_ = 0;
		DBG_DUMP("vprc eis plugin un-register done!\r\n");
		return 0;
	}
#else
	DBG_ERR("NOT support eis!\r\n");
#endif
	return -1;
}
EXPORT_SYMBOL(isf_vdoprc_reg_eis_plugin);

