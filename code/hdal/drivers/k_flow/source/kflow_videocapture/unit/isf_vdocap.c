#include "isf_vdocap_int.h"
#include "isf_vdocap_dbg.h"
#include "comm/hwclock.h"


#define CTL_SIE_RATIO(w, h) (((UINT32)(UINT16)(w) << 16) | (UINT32)(UINT16)(h))

#define PATGEN_MAX_DIM_W     3840
#define PATGEN_MAX_DIM_H     2160
#define PATGEN_MAX_FPS         30

#define GET_AD_CHIPID(map)      ((((UINT32)(map)) >> 24) & 0x000000ff)
#define GET_AD_IN(map)          ((((UINT32)(map)) >> 16) & 0x000000ff)

#define ALLOC_MEM_FOR_KFLOW  1  //should equal SIE_NUM_CFG_BY_HDAL

#define CTL_SEN_USE_INT_MEM  0 //set addr=0 to let ctl_sen_init use internal kmalloc

static CTL_SEN_OUTPUT_DEST sen_out_dest[VDOCAP_SHDR_SET_MAX_NUM] = {0}; //sen_out_dest[0] for SHDR set1, sen_out_dest[1] for set2
ISF_UNIT *_vdocap_shdr_main_unit[VDOCAP_SHDR_SET_MAX_NUM] = {NULL};
UINT32 _vdocap_shdr_frm_num[VDOCAP_SHDR_SET_MAX_NUM] = {VDOCAP_SEN_FRAME_NUM_1};
BOOL output_rate_update_pause = FALSE;

static VDOCAP_COMMON_MEM g_vdocap_mem = {0};
static VDOCAP_CONTEXT* g_vdocap_context = 0;

//referring to isp.h
typedef enum _ISP_SENSOR_PRESET_MODE {
	ISP_SENSOR_PRESET_DEFAULT,
	ISP_SENSOR_PRESET_CHGMODE,
	ISP_SENSOR_PRESET_AE,
	ENUM_DUMMY4WORD(ISP_SENSOR_PRESET_MODE)
} ISP_SENSOR_PRESET_MODE;

typedef struct _ISP_SENSOR_PRESET {
	ISP_SENSOR_PRESET_MODE mode;
	UINT32 exp_time[2];    // ISP_SENSOR_PRESET_AE usage
	UINT32 gain_ratio[2];  // ISP_SENSOR_PRESET_AE usage
} ISP_SENSOR_PRESET_CTRL;

UINT32 _vdocap_max_count = 2;
UINT32 _vdocap_active_list = 0x03;

//temp solution
#if defined(_BSP_NA51000_)
#undef _BSP_NA51000_
#endif

ISF_UNIT *g_vdocap_list[VDOCAP_MAX_NUM] = {
	&isf_vdocap0,
	&isf_vdocap1,
	&isf_vdocap2,
#if defined(_BSP_NA51055_)
	&isf_vdocap3,
	&isf_vdocap4,
#endif
};
#include "comm/gyro_drv.h"

#define MAX_GYRO_OBJ_NUM 1
PGYRO_OBJ _vcap_gyro_obj[MAX_GYRO_OBJ_NUM] = {0};

INT32 isf_vdocap_reg_gyro_obj(UINT32 id, PGYRO_OBJ p_gyro_obj)
{
	if (id >= MAX_GYRO_OBJ_NUM) {
		DBG_ERR("only support max %d gyro obj\r\n", MAX_GYRO_OBJ_NUM);
		return -1;
	}
	if ((_vcap_gyro_obj[id] == 0) && (p_gyro_obj != 0)) {
		_vcap_gyro_obj[id] = p_gyro_obj;
		DBG_DUMP("vcap gyro obj register ok!\r\n");
		return 0;
	}
	if ((_vcap_gyro_obj[id] != 0) && (p_gyro_obj == 0)) {
		_vcap_gyro_obj[id] = 0;
		DBG_DUMP("vcap gyro obj un-register done!\r\n");
		return 0;
	}

	return -1;
}
EXPORT_SYMBOL(isf_vdocap_reg_gyro_obj);


static UINT32 g_vdocap_init[ISF_FLOW_MAX] = {0};

static ER _vdocap_sensor_ctl(PCTL_SEN_OBJ p_sen_ctl_obj, VDOCAP_CONTEXT *p_ctx, BOOL open);

void _isf_vdocap_user_define3_cb(ISF_UNIT *p_thisunit, UINT32 param)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	if (p_ctx) {
		UINT32 user_param = param;
		PCTL_SEN_OBJ p_sen_obj;

		DBG_UNIT("param=%d\r\n", param);

		p_sen_obj = ctl_sen_get_object(p_ctx->id);
		if ((p_sen_obj == NULL) || (p_sen_obj->get_cfg == NULL)) {
			DBG_ERR("get sen_obj[%d] failed!\r\n", p_ctx->id);
			return;
		}
		p_sen_obj->set_cfg(CTL_SEN_CFGID_USER_DEFINE3, &user_param);

	} else {
		DBG_ERR("VCAP uninit!\r\n");
	}
}

static BOOL _vdocap_check_enc_rate(UINT32 enc_rate)
{
	if (enc_rate == 50 || enc_rate == 58 || enc_rate == 66) {
		return TRUE;
	}
	return FALSE;
}
static CTL_SIE_ENC_RATE_SEL _vdocap_enc_rate_mapping(UINT32 enc_rate)
{
	CTL_SIE_ENC_RATE_SEL sie_enc_rate;

	switch (enc_rate) {
	case 50:
		sie_enc_rate = CTL_SIE_ENC_50;
	break;
	case 58:
		sie_enc_rate = CTL_SIE_ENC_58;
	break;
	case 66:
		sie_enc_rate = CTL_SIE_ENC_66;
	break;
	default:
#if defined(_BSP_NA51089_)
		sie_enc_rate = CTL_SIE_ENC_50;
#else
		sie_enc_rate = CTL_SIE_ENC_58;
#endif
	break;
	}
	return sie_enc_rate;
}


static CTL_SEN_DRVDEV _vdocap_lvds_csi_mapping(CTL_SEN_IF_TYPE if_cfg_type, CTL_SEN_CLANE_SEL clk_lane_sel)
{
	CTL_SEN_DRVDEV ret = 0;
	if (if_cfg_type == CTL_SEN_IF_TYPE_LVDS) {
		if (clk_lane_sel == CTL_SEN_CLANE_SEL_LVDS0_USE_C0C4 || clk_lane_sel == CTL_SEN_CLANE_SEL_LVDS0_USE_C2C6) {
			ret = CTL_SEN_DRVDEV_LVDS_0;
		} else {
			ret = CTL_SEN_DRVDEV_LVDS_1;
		}
	} else if (if_cfg_type == CTL_SEN_IF_TYPE_MIPI) {
		if (clk_lane_sel == CTL_SEN_CLANE_SEL_CSI0_USE_C0 || clk_lane_sel == CTL_SEN_CLANE_SEL_CSI0_USE_C2) {
			ret = CTL_SEN_DRVDEV_CSI_0;
		} else {
			ret = CTL_SEN_DRVDEV_CSI_1;
		}
	}
	return ret;
}

static ISF_RV _isf_vdocap_do_init(UINT32 h_isf, UINT32 active_list)
{
	ISF_RV rv = ISF_OK;

	UINT32 i, temp;
	UINT32 dev_num = 0;
	UINT32 ctx_idx;
	VDOCAP_COMMON_MEM* p_mem = &g_vdocap_mem;

	if (h_isf > 0) { //client process
		if (!g_vdocap_init[0]) { //is master process already init?
			rv = ISF_ERR_INIT; //not allow init from client process!
			goto _VP_INIT_ERR;
		}
		g_vdocap_init[h_isf] = 1; //set init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_vdocap_init[i]) { //client process still alive?
				rv = ISF_ERR_INIT; //not allow init from main process!
				goto _VP_INIT_ERR;
			}
		}
	}

	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		temp  = active_list >> i;
		if (temp & 0x1) {
			dev_num++;
		}
	}
	_vdocap_max_count = dev_num;
	_vdocap_active_list = active_list;

	if (g_vdocap_context) {
		rv = ISF_ERR_INIT; //already init
		goto _VP_INIT_ERR;
	}
	{
		UINT32 dev;
#if ALLOC_MEM_FOR_KFLOW
		INT32 rt;
#endif

		nvtmpp_vb_add_module(g_vdocap_list[0]->unit_module);
		memset((void*)p_mem, 0, sizeof(VDOCAP_COMMON_MEM));
		p_mem->total_buf.size = 0;
		p_mem->total_buf.addr = 0;
		//calculate conext buffer size of unit
		p_mem->unit_buf.size = ALIGN_CEIL(_vdocap_max_count * sizeof(VDOCAP_CONTEXT), 64); //align to cache line
		p_mem->total_buf.size += p_mem->unit_buf.size;
#if ALLOC_MEM_FOR_KFLOW
		//calculate conext buffer size of kflow/kdrv
		p_mem->ctl_sie_buf.size = ALIGN_CEIL(ctl_sie_buf_query(_vdocap_max_count), 64); //align to cache line
		p_mem->total_buf.size += p_mem->ctl_sie_buf.size;
		p_mem->ctl_sen_buf.size = ALIGN_CEIL(ctl_sen_buf_query(_vdocap_max_count), 64); //align to cache line
		#if (CTL_SEN_USE_INT_MEM == 0)
		p_mem->total_buf.size += p_mem->ctl_sen_buf.size;
		#endif
#endif
		//alloc context of all device
		p_mem->total_buf.addr = DEV_UNIT(0)->p_base->do_new_i(DEV_UNIT(0), NULL, "ctx", p_mem->total_buf.size, &(p_mem->memblk));
		if (p_mem->total_buf.addr == 0) {
			DBG_ERR("alloc context memory fail !!\r\n");
			rv = ISF_ERR_HEAP;
			goto _VP_INIT_ERR;
		}
		//init each device's context of unit
		ctx_idx = 0;
		g_vdocap_context = (VDOCAP_CONTEXT*)p_mem->total_buf.addr;
		for (dev = 0; dev < VDOCAP_MAX_NUM; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);

			if (((1 << dev) & _vdocap_active_list) && p_unit && (p_unit->do_init)) {
				rv = p_unit->do_init(p_unit, (void*)&(g_vdocap_context[ctx_idx]));
				if (rv != ISF_OK) {
					rv = DEV_UNIT(0)->p_base->do_release_i(DEV_UNIT(0), &(p_mem->memblk), ISF_OK);
					g_vdocap_context = 0;
					goto _VP_INIT_ERR;
				}
				ctx_idx++;
			}
		}
#if ALLOC_MEM_FOR_KFLOW
		//init context of kflow/kdrv
		p_mem->ctl_sie_buf.addr = p_mem->total_buf.addr + p_mem->unit_buf.size;
		rt = ctl_sie_init(p_mem->ctl_sie_buf.addr, p_mem->ctl_sie_buf.size);
		if (rt) {
			DBG_ERR("ctl sie init fail!(%d)\r\n", rt);
			goto _VP_INIT_ERR;
		}
		#if (CTL_SEN_USE_INT_MEM == 0)
		p_mem->ctl_sen_buf.addr = p_mem->ctl_sie_buf.addr + p_mem->ctl_sie_buf.size;
		#else
		//set addr 0 to let ctl_set use internal kmalloc
		p_mem->ctl_sen_buf.addr = 0;
		#endif

		rt = ctl_sen_init(p_mem->ctl_sen_buf.addr, p_mem->ctl_sen_buf.size);
		if (rt != CTL_SEN_E_OK && rt != CTL_SEN_E_STATE) {
			DBG_ERR("ctl sen init fail!(%d)\r\n", rt);
			goto _VP_INIT_ERR;
		}
#endif
		DBG_MSG("_vdocap_max_count=%d\r\n", _vdocap_max_count);
		DBG_MSG("unit    @0x%X~0x%X(%d)\r\n", p_mem->total_buf.addr, p_mem->total_buf.addr+p_mem->unit_buf.size, p_mem->unit_buf.size);
		DBG_MSG("ctl_sie @0x%X~0x%X(%d)\r\n", p_mem->ctl_sie_buf.addr, p_mem->ctl_sie_buf.addr+p_mem->ctl_sie_buf.size, p_mem->ctl_sie_buf.size);
		DBG_MSG("ctl_sen @0x%X~0x%X(%d)\r\n", p_mem->ctl_sen_buf.addr, p_mem->ctl_sen_buf.addr+p_mem->ctl_sen_buf.size, p_mem->ctl_sen_buf.size);

		//install each device's kernel object
		isf_vdocap_install_id();
		g_vdocap_init[h_isf] = 1; //set init
		return ISF_OK;
	}

_VP_INIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("init fail! %d\r\n", rv);
	}
	return rv;
}

static ISF_RV _isf_vdocap_do_uninit(UINT32 h_isf, UINT32 reset)
{
	ISF_RV rv;
	VDOCAP_COMMON_MEM* p_mem = &g_vdocap_mem;

	if (h_isf > 0) { //client process
		if (!g_vdocap_init[0]) { //is master process already init?
			rv = ISF_ERR_UNINIT; //not allow uninit from client process!
			goto _VP_UNINIT_ERR;
		}
		g_vdocap_init[h_isf] = 0; //clear init
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (g_vdocap_init[i] && !reset) { //client process still alive?
				rv = ISF_ERR_UNINIT; //not allow uninit from main process!
				goto _VP_UNINIT_ERR;
			}
			if (reset) {
				g_vdocap_init[i] = 0; //force clear client
			}
		}
	}

	if (!g_vdocap_context) {
		return ISF_ERR_UNINIT; //still not init
	}

	{
		UINT32 dev, i;

		//reset all units of this module
		for (dev = 0; dev < VDOCAP_MAX_NUM; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);
			if (((1 << dev) & _vdocap_active_list) && p_unit) {
				DBG_IND("unit(%d) = %s => clear state\r\n", dev, p_unit->unit_name);
				for(i = 0; i < p_unit->port_out_count; i++) {
					UINT32 oport = i + ISF_OUT_BASE;
					p_unit->p_base->do_clear_state(p_unit, oport);
				}
			}
		}
		for (dev = 0; dev < VDOCAP_MAX_NUM; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);
			if (((1 << dev) & _vdocap_active_list) && p_unit) {
				DBG_IND("unit(%d) = %s => clear bind\r\n", dev, p_unit->unit_name);
				for(i = 0; i < p_unit->port_out_count; i++) {
					UINT32 oport = i + ISF_OUT_BASE;
					p_unit->p_base->do_clear_bind(p_unit, oport);
				}
			}
		}
		for (dev = 0; dev < VDOCAP_MAX_NUM; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);
			if (((1 << dev) & _vdocap_active_list) && p_unit) {
				DBG_IND("unit(%d) = %s => clear context\r\n", dev, p_unit->unit_name);
				for(i = 0; i < p_unit->port_out_count; i++) {
					UINT32 oport = i + ISF_OUT_BASE;
					p_unit->p_base->do_clear_ctx(p_unit, oport);
				}
			}
		}

		//uninstall each device's kernel object
		isf_vdocap_uninstall_id();
#if ALLOC_MEM_FOR_KFLOW
		//uninit kflow/kdrv's context
		ctl_sie_uninit();
		ctl_sen_uninit();
#endif
		//uninit each device's context
		for (dev = 0; dev < VDOCAP_MAX_NUM; dev++) {
			ISF_UNIT* p_unit = DEV_UNIT(dev);
			if (((1 << dev) & _vdocap_active_list) && p_unit && (p_unit->do_uninit)) {
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
		g_vdocap_context = 0;
		g_vdocap_init[h_isf] = 0; //clear init
		return ISF_OK;
	}

_VP_UNINIT_ERR:
	if (rv != ISF_OK) {
		DBG_ERR("uninit fail! %d\r\n", rv);
	}
	return rv;
}
ISF_RV _isf_vdocap_do_command(UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2)
{
	UINT32 h_isf = (cmd >> 28); //extract h_isf

	DBG_FUNC("cmd=%d p1=0x%02X\r\n", cmd, p1);
	cmd &= ~0xf0000000; //clear h_isf
	switch(cmd) {
	case 1: //INIT
		nvtmpp_vb_add_module(MAKE_NVTMPP_MODULE('v', 'd', 'o', 'c', 'a', 'p', 0, 0));
		return _isf_vdocap_do_init(h_isf, p1); //p1: max dev count
		break;
	case 0: //UNINIT
		return _isf_vdocap_do_uninit(h_isf, p1); //p1: is under reset
		break;
	default:
		break;
	}
	return ISF_ERR_NOT_SUPPORT;
}
BOOL _vdocap_is_direct_flow(VDOCAP_CONTEXT *p_ctx)
{
	if (p_ctx->out_dest == CTL_SIE_OUT_DEST_DRAM) {
		return FALSE;
	} else {
		return TRUE;
	}
}
BOOL _vdocap_is_shdr_main_path(VDOCAP_SEN_HDR_MAP shdr_map)
{
	switch (shdr_map) {
	case VDOCAP_SEN_HDR_SET1_SUB1:
	case VDOCAP_SEN_HDR_SET1_SUB2:
	case VDOCAP_SEN_HDR_SET1_SUB3:
	case VDOCAP_SEN_HDR_SET2_SUB1:
		return FALSE;
	default:
		return TRUE;
	}
}
#if 0
static BOOL _vdocap_is_ccir_mux_mode(VDOCAP_CONTEXT *p_ctx)
{
	if (p_ctx->ccir_mux_map) {
		return TRUE;
	} else {
		return FALSE;
	}
}
static BOOL _vdocap_is_target_ccir_mux_id(VDOCAP_CONTEXT *p_ctx, BOOL first)
{
	UINT32 i, temp, candidate_id = 0xFF;

	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		temp  = (p_ctx->ccir_mux_map >> i);
		if (temp & 0x1) {
			candidate_id = i;
			if (first) {
				break;
			}
		}
	}
	if (first) {
		DBG_UNIT("first ccir_mux id = %d\r\n", candidate_id);
	} else {
		DBG_UNIT("last ccir_mux id = %d\r\n", candidate_id);
	}
	if (p_ctx->id == candidate_id) {
		return TRUE;
	} else {
		return FALSE;
	}
}
static BOOL _vdocap_get_main_ccir_mux_id(VDOCAP_CONTEXT *p_ctx)
{
	UINT32 i, temp;

	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		temp  = (p_ctx->ccir_mux_map >> i);
		if (temp & 0x1) {
			break;
		}
	}
	return i;
}
#endif
static UINT32 _vdocap_get_main_mux_id(VDOCAP_CONTEXT *p_ctx)
{
	UINT32 i, temp;
	UINT32 map = p_ctx->ad_map & 0xFF;

	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		temp  = (map >> i);
		if (temp & 0x1) {
			break;
		}
	}
	return i;
}
static BOOL _vdocap_is_mux_mode(VDOCAP_CONTEXT *p_ctx)
{
	UINT32 i, temp;
	UINT32 dev_num = 0;
	UINT32 map = p_ctx->ad_map & 0xFF;

	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		temp  = map >> i;
		if (temp & 0x1) {
			dev_num++;
		}
	}
	if (dev_num < 2) {
		return FALSE;
	} else {
		return TRUE;
	}
}
static BOOL _vdocap_is_sw_vd_sync_mode(VDOCAP_CONTEXT *p_ctx)
{
	if (p_ctx->sw_vd_sync) {
		return TRUE;
	} else {
		return FALSE;
	}
}
static UINT32 _vdocap_get_main_sw_vd_sync_id(VDOCAP_CONTEXT *p_ctx)
{
	UINT32 i, temp;
	UINT32 map = p_ctx->sw_vd_sync & 0xFF;

	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		temp  = (map >> i);
		if (temp & 0x1) {
			break;
		}
	}
	return i;
}
static UINT32 _vdocap_get_main_pdaf_id(VDOCAP_CONTEXT *p_ctx)
{
	UINT32 i, temp;
	UINT32 map = p_ctx->pdaf_map & 0xFF;

	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		temp  = (map >> i);
		if (temp & 0x1) {
			break;
		}
	}
	return i;
}
static BOOL _vdocap_is_pdaf_mode(VDOCAP_CONTEXT *p_ctx)
{
	if (p_ctx->pdaf_map) {
		return TRUE;
	} else {
		return FALSE;
	}
}
static BOOL _vdocap_is_tge_sync_mode(VDOCAP_CONTEXT *p_ctx)
{
	if (p_ctx->tge_sync_id) {
		return TRUE;
	} else {
		return FALSE;
	}
}
static BOOL _vdocap_is_last_sync_id(VDOCAP_CONTEXT *p_ctx, UINT32 sync_id)
{
	UINT32 i, temp, condidate_id = 0xFF;

	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		temp  = (sync_id >> i);
		if (temp & 0x1) {
			condidate_id = i;
		}
	}
	DBG_UNIT("[%d]last sync id = %d, shdr_map=%d\r\n", p_ctx->id, condidate_id,p_ctx->shdr_map);


	if (_vdocap_is_shdr_mode(p_ctx->shdr_map)) {
		ISF_UNIT *p_main_unit = _vdocap_shdr_main_unit[_vdocap_shdr_map_to_set(p_ctx->shdr_map)];
		VDOCAP_CONTEXT *p_main_ctx;
		UINT32 shdr_set;
		UINT32 shdr_seq;
		UINT32 shdr_frm_num;

		shdr_set = _vdocap_shdr_map_to_set(p_ctx->shdr_map);
		shdr_seq = _vdocap_shdr_map_to_seq(p_ctx->shdr_map);
		shdr_frm_num = _vdocap_shdr_frm_num[shdr_set];

		//tge_sync_id is only for main path
		if (p_main_unit) {
			p_main_ctx = (VDOCAP_CONTEXT *)(p_main_unit->refdata);

			if (p_main_ctx->id == condidate_id) {
				DBG_UNIT("[%d]shdr_map(%d),shdr_set(%d),shdr_seq(%d), shdr_frm_num(%d)\r\n",p_ctx->id, p_ctx->shdr_map, shdr_set, shdr_seq,shdr_frm_num);
				if (shdr_seq == (shdr_frm_num - 1)) {
					return TRUE;
				} else {
					return FALSE;
				}
			} else {
				return FALSE;
			}

		}
		else {
			return FALSE;
		}
	} else {
		if (p_ctx->id == condidate_id) {
			return TRUE;
		} else {
			return FALSE;
		}
	}
}
static VDOCAP_CONTEXT * _vdocap_id_to_ctx(UINT32 id)
{
	ISF_UNIT * p_unit = NULL;
	VDOCAP_CONTEXT *p_ctx;

	switch (id) {
	case 0:
		p_unit = &isf_vdocap0;
		break;
	case 1:
		p_unit = &isf_vdocap1;
		break;
	case 2:
		p_unit = &isf_vdocap2;
		break;
	case 3:
		p_unit = &isf_vdocap3;
		break;
	case 4:
		p_unit = &isf_vdocap4;
		break;
#if defined(_BSP_NA51000_)
	case 5:
		p_unit = &isf_vdocap5;
		break;
	case 6:
		p_unit = &isf_vdocap6;
		break;
	case 7:
		p_unit = &isf_vdocap7;
		break;
#endif
	default:
		DBG_ERR("error id(%d)\r\n", id);
		return NULL;
	}
	p_ctx = (VDOCAP_CONTEXT *)(p_unit->refdata);
	return p_ctx;
}
static INT32 _vdocap_start_all_sync_sie(UINT32 sync_id)
{
	CTL_SIE_TRIG_INFO trig_info = {0};
	INT32 ret = CTL_SIE_E_OK;
	UINT32 i, temp;

	trig_info.trig_type = CTL_SIE_TRIG_START;
	trig_info.trig_frame_num = 0xffffffff; /*continue mode*/
	trig_info.b_wait_end = 0;

	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		temp  = sync_id >> i;
		if (temp & 0x1) {
			VDOCAP_CONTEXT *p_ctx;

			p_ctx = _vdocap_id_to_ctx(i);
			if (NULL == p_ctx) {
				DBG_ERR("VDOCAP[%d] sync_set(0x%X) NOT in active list!(0x%X)\r\n", i, sync_id, _vdocap_active_list);
				return ISF_ERR_PROCESS_FAIL;
			}
			DBG_UNIT("VDOCAP[%d] start++\r\n", p_ctx->id);
			p_ctx->started = 1;
			ret |= ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_TRIG_IMM, (void *)&trig_info);
			if (ret != CTL_SIE_E_OK) {
				DBG_ERR("[%d]failed(%d)\r\n", p_ctx->id, ret);
				//p_ctx->started = 0;
				if (ret == CTL_SIE_E_NOMEM) {
					DBG_ERR("RAW buf insufficient(0x%X)\r\n", p_ctx->out_buf_size[0]);
					return ISF_OK;
				} else {
					return ISF_ERR_PROCESS_FAIL;//map to HD_ERR_FAIL;
				}
			}
			DBG_UNIT("VDOCAP[%d] start--\r\n", p_ctx->id);
			//start HDR sub-path
			if (_vdocap_is_shdr_mode(p_ctx->shdr_map)) {
				UINT32 shdr_set, shdr_frm_num;
				UINT32 j, tp;
				UINT32 out_dest;
				UINT32 seq = 0;

				shdr_set = _vdocap_shdr_map_to_set(p_ctx->shdr_map);
				out_dest = sen_out_dest[shdr_set];
				shdr_frm_num = _vdocap_shdr_frm_num[shdr_set];

				for (j = 0; j < VDOCAP_MAX_NUM; j++) {
					tp  = out_dest >> j;
					if (tp & 0x1) {
						//only for sub path (seq = 0 is main path)
						if (seq) {
							p_ctx = _vdocap_id_to_ctx(j);
							if (NULL == p_ctx) {
								return ISF_ERR_PROCESS_FAIL;
							}
							DBG_UNIT("VDOCAP[%d] start+\r\n", p_ctx->id);
							p_ctx->started = 1;
							ret |= ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_TRIG_IMM, (void *)&trig_info);
							if (ret != CTL_SIE_E_OK) {
								DBG_ERR("[%d]failed(%d)\r\n", j, ret);
								//p_ctx->started = 0;
								if (ret == CTL_SIE_E_NOMEM) {
									DBG_ERR("RAW buf insufficient(0x%X)\r\n", p_ctx->out_buf_size[0]);
									return ISF_OK;
								} else {
									return ISF_ERR_PROCESS_FAIL;//map to HD_ERR_FAIL;
								}
							}
							DBG_UNIT("VDOCAP[%d] start-\r\n", p_ctx->id);
							////reset frame count for shdr flow
							if (seq == (shdr_frm_num - 1)) {
								DBG_UNIT("VDOCAP[%d] reset frame cnt(%d) +\r\n", p_ctx->id, ret);
								ret = ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_RESET_FC_IMM, NULL);
								DBG_UNIT("VDOCAP[%d] reset frame cnt(%d) -\r\n", p_ctx->id, ret);
							}
						}
						seq++;
					}
				}
			}
		}
	}
	return ret;
}
static BOOL _vdocap_is_last_opened_shdr(UINT32 id, UINT32 shdr_set)
{
	UINT32 temp, i;
	//check if other shdr vcap is opened
	for (i = id+1; i < VDOCAP_MAX_NUM; i++) {
		temp  = sen_out_dest[shdr_set] >> i;
		if (temp & 0x1) {
			VDOCAP_CONTEXT *p_ctx;

			p_ctx = _vdocap_id_to_ctx(i);
			if (NULL == p_ctx) {
				DBG_ERR("VDOCAP[%d] shdr_set(0x%X) NOT in active list!(0x%X)\r\n", i, sen_out_dest[shdr_set], _vdocap_active_list);
				continue;
			}
			if (p_ctx->sie_hdl) {
				//if other vcap is opened, it means this vcap id is not the last shdr vcap
				return FALSE;
			}
		}
	}
	return TRUE;
}
static INT32 _vdocap_start_shdr_direct_sie(VDOCAP_SEN_HDR_MAP shdr_map)
{
	CTL_SIE_TRIG_INFO trig_info = {0};
	INT32 ret = CTL_SIE_E_OK;
	INT32 i;
	UINT32 temp;
	UINT32 shdr_set;
	UINT32 shdr_seq;
	UINT32 shdr_frm_num;
	VDOCAP_CONTEXT *p_ctx = NULL;

	shdr_set = _vdocap_shdr_map_to_set(shdr_map);
	shdr_seq = _vdocap_shdr_map_to_seq(shdr_map);
	shdr_frm_num = _vdocap_shdr_frm_num[shdr_set];

	if (shdr_seq != (shdr_frm_num - 1)) {
		return ret;
	}

	trig_info.trig_type = CTL_SIE_TRIG_START;
	trig_info.trig_frame_num = 0xffffffff; /*continue mode*/
	trig_info.b_wait_end = 0;

	for (i = (VDOCAP_MAX_NUM-1); i >= 0; i--) {
		temp  = sen_out_dest[shdr_set] >> i;
		if (temp & 0x1) {


			p_ctx = _vdocap_id_to_ctx((UINT32)i);
			if (NULL == p_ctx) {
				DBG_ERR("VDOCAP[%d] NOT in active list!(0x%X)\r\n", i, _vdocap_active_list);
				return ISF_ERR_PROCESS_FAIL;
			}
			DBG_UNIT("VDOCAP[%d] start++\r\n", p_ctx->id);
			p_ctx->started = 1;
			ret |= ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_TRIG_IMM, (void *)&trig_info);
			if (ret != CTL_SIE_E_OK) {
				//p_ctx->started = 0;
				DBG_ERR("failed(%d)\r\n", ret);
				if (ret == CTL_SIE_E_NOMEM) {
					DBG_ERR("RAW buf insufficient(0x%X)\r\n", p_ctx->out_buf_size[0]);
					return ISF_ERR_NEW_FAIL;
				} else {
					return ISF_ERR_PROCESS_FAIL;//map to HD_ERR_FAIL;
				}
			}
			DBG_UNIT("VDOCAP[%d] start--\r\n", p_ctx->id);
		}
	}
	if (p_ctx) {
		DBG_UNIT("VDOCAP[%d] reset frame cnt(%d) ++\r\n", p_ctx->id, ret);
		ret = ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_RESET_FC_IMM, NULL);
		DBG_UNIT("VDOCAP[%d] reset frame cnt(%d) --\r\n", p_ctx->id, ret);
	}
	return ret;
}
static UINT32 _vdocap_get_mclksrc_sync(VDOCAP_CONTEXT *p_ctx)
{
	UINT32 mclksrc_sync = 0;

	//if it's in SHSR mode, use main SHDR vcap to check it's mclk_src_sync
	if (p_ctx->shdr_map) {
		ISF_UNIT *p_main_unit = _vdocap_shdr_main_unit[_vdocap_shdr_map_to_set(p_ctx->shdr_map)];
		VDOCAP_CONTEXT *p_main_ctx;

		if (p_main_unit) {
			p_main_ctx = (VDOCAP_CONTEXT *)(p_main_unit->refdata);
			mclksrc_sync = p_main_ctx->mclksrc_sync;
		}
	} else {
		mclksrc_sync = p_ctx->mclksrc_sync;
	}
	return mclksrc_sync;
}
static BOOL _vdocap_is_mclk_sync_mode(VDOCAP_CONTEXT *p_ctx)
{
	if (_vdocap_get_mclksrc_sync(p_ctx)) {
		return TRUE;
	} else {
		return FALSE;
	}
}
static BOOL _vdocap_is_last_mclk_sync_id(VDOCAP_CONTEXT *p_ctx)
{
	UINT32 i, temp, candidate_id = 0xFF, mclksrc_sync;

	mclksrc_sync = _vdocap_get_mclksrc_sync(p_ctx);
	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		temp  = (mclksrc_sync >> i);
		if (temp & 0x1) {
			candidate_id = i;
		}
	}
	DBG_UNIT("last mclk sync id = %d\r\n", candidate_id);
	if (p_ctx->id == candidate_id) {
		return TRUE;
	} else {
		return FALSE;
	}
}
static INT32 _vdocap_open_all_mclk_sync_ctl_sen(VDOCAP_CONTEXT *p_ctx_)
{
	INT32 ret;
	UINT32 i, temp;
	UINT32 sync_id = _vdocap_get_mclksrc_sync(p_ctx_);

	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		temp  = sync_id >> i;
		if (temp & 0x1) {
			VDOCAP_CONTEXT *p_ctx;
			PCTL_SEN_OBJ p_sen_ctl_obj;

			p_ctx = _vdocap_id_to_ctx(i);
			if (NULL == p_ctx) {
				DBG_ERR("VDOCAP[%d] mclksrc_sync(0x%X) NOT in active list!(0x%X)\r\n", i, sync_id, _vdocap_active_list);
				return ISF_ERR_PROCESS_FAIL;
			}
			if (p_ctx->flow_type == CTL_SIE_FLOW_SEN_IN) {
				p_sen_ctl_obj = ctl_sen_get_object(p_ctx->id);
				DBG_UNIT("VDOCAP[%d] ctl_sen open ++\r\n", p_ctx->id);
				ret = p_sen_ctl_obj->open();
				DBG_UNIT("VDOCAP[%d] ctl_sen open --\r\n", p_ctx->id);
				if (ret != CTL_SEN_E_OK) {
					return ret;
				}
			}
		}
	}
	return ret;
}
static INT32 _vdocap_open_all_mclk_sync_rest(VDOCAP_CONTEXT *p_ctx_)
{
	INT32 ret = ISF_OK;
	UINT32 i, temp;
	UINT32 sync_id = _vdocap_get_mclksrc_sync(p_ctx_);

	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		temp  = sync_id >> i;
		if (temp & 0x1) {
			VDOCAP_CONTEXT *p_ctx;
			PCTL_SEN_OBJ p_sen_ctl_obj;
			CTL_SIE_OPEN_CFG open_cfg = {0};
			CTL_SIE_REG_CB_INFO cb_info;
			CTL_SIE_MCLK sie_mclk;

			//main path
			p_ctx = _vdocap_id_to_ctx(i);
			if (NULL == p_ctx) {
				DBG_ERR("VDOCAP[%d] mclksrc_sync(0x%X) NOT in active list!(0x%X)\r\n", i, sync_id, _vdocap_active_list);
				return ISF_ERR_PROCESS_FAIL;
			}
			p_sen_ctl_obj = ctl_sen_get_object(p_ctx->id);
			// Must set the same clock rate as ssenif reference rate
			open_cfg.id = p_ctx->id;
			open_cfg.flow_type = p_ctx->flow_type;
			open_cfg.sen_id = p_ctx->id;
			open_cfg.isp_id = p_ctx->id;
			open_cfg.clk_src_sel = CTL_SIE_CLKSRC_DEFAULT;
			DBG_MSG("open_cfg:id=%d,flow_type=%d,dupl_src_id=%d,sen_id=%d,isp_id=%d,clk_src_sel=%d\r\n", open_cfg.id, open_cfg.flow_type, open_cfg.dupl_src_id, open_cfg.sen_id, open_cfg.isp_id, open_cfg.clk_src_sel);
			p_ctx->dev_trigger_open = 1;
			p_ctx->sie_hdl = ctl_sie_open((void *)&open_cfg);
			if (p_ctx->sie_hdl == 0) {
				DBG_ERR("ctl_sie_open open fail\r\n");
				return ISF_ERR_NOT_AVAIL;
			}
			p_ctx->dev_ready = 1; //after open
			p_ctx->dev_trigger_close = 0;

			if (p_ctx->flow_type == CTL_SIE_FLOW_SEN_IN) {
				if(E_OK != _vdocap_sensor_ctl(p_sen_ctl_obj, p_ctx, TRUE)) {
					return ISF_ERR_INVALID_VALUE;
				}
			} else if (p_ctx->flow_type == CTL_SIE_FLOW_PATGEN) {
				sie_mclk.mclk_en = ENABLE;
				ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_MCLK_IMM, (void *)&sie_mclk);
				ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_LOAD, NULL);
			}
			cb_info.cbevt = CTL_SIE_CBEVT_ENG_SIE_ISR;
			cb_info.fp = p_ctx->sie_isr_cb;
			cb_info.sts = CTL_SIE_INTE_DRAM_OUT0_END | CTL_SIE_INTE_VD;
			ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_REG_CB_IMM, (void *)&cb_info);

			cb_info.cbevt = CTL_SIE_CBEVT_BUFIO;
			cb_info.fp = p_ctx->buf_io_cb;
			cb_info.sts = CTL_SIE_BUF_IO_ALL;
			ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_REG_CB_IMM, (void *)&cb_info);

			//start HDR sub-path
			if (p_ctx->shdr_map) {
				UINT32 shdr_set;
				UINT32 j, tp;
				UINT32 out_dest;
				UINT32 seq = 0;

				shdr_set = _vdocap_shdr_map_to_set(p_ctx->shdr_map);
				out_dest = sen_out_dest[shdr_set];
				for (j = 0; j < VDOCAP_MAX_NUM; j++) {
					tp  = out_dest >> j;
					if (tp & 0x1) {
						//only for sub path (seq = 0 is main path)
						if (seq) {
							p_ctx = _vdocap_id_to_ctx(j);
							if (NULL == p_ctx) {
								DBG_ERR("VDOCAP[%d] mclksrc_sync(0x%X) NOT in active list!(0x%X)\r\n", i, sync_id, _vdocap_active_list);
								return ISF_ERR_PROCESS_FAIL;
							}
							// Must set the same clock rate as ssenif reference rate
							open_cfg.id = p_ctx->id;
							open_cfg.flow_type = p_ctx->flow_type;
							open_cfg.sen_id = p_ctx->id;
							open_cfg.isp_id = p_ctx->id;
							if (p_ctx->flow_type == CTL_SIE_FLOW_SIG_DUPL) {
								ISF_UNIT *p_main_unit = _vdocap_shdr_main_unit[_vdocap_shdr_map_to_set(p_ctx->shdr_map)];
								VDOCAP_CONTEXT *p_main_ctx;

								if (p_main_unit) {
									p_main_ctx = (VDOCAP_CONTEXT *)(p_main_unit->refdata);
									open_cfg.dupl_src_id = p_main_ctx->id;
									open_cfg.sen_id = open_cfg.dupl_src_id;
									//since SEN_INIT_CFG is only for HDR main path, tge_sync_id needs to sync for HDR sub-path
									p_ctx->tge_sync_id = p_main_ctx->tge_sync_id;
								}

							} else {
								DBG_ERR("sync_id(0x%X) error\r\n", sync_id);
								return ISF_ERR_INVALID_VALUE;
							}
							open_cfg.clk_src_sel = CTL_SIE_CLKSRC_DEFAULT;
							DBG_MSG("open_cfg:id=%d,flow_type=%d,dupl_src_id=%d,sen_id=%d,isp_id=%d,clk_src_sel=%d\r\n", open_cfg.id, open_cfg.flow_type, open_cfg.dupl_src_id, open_cfg.sen_id, open_cfg.isp_id, open_cfg.clk_src_sel);
							p_ctx->dev_trigger_open = 1;
							p_ctx->sie_hdl = ctl_sie_open((void *)&open_cfg);
							if (p_ctx->sie_hdl == 0) {
								DBG_ERR("ctl_sie_open open fail\r\n");
								return ISF_ERR_NOT_AVAIL;
							}
							p_ctx->dev_ready = 1; //after open
							p_ctx->dev_trigger_close = 0;

							cb_info.cbevt = CTL_SIE_CBEVT_ENG_SIE_ISR;
							cb_info.fp = p_ctx->sie_isr_cb;
							cb_info.sts = CTL_SIE_INTE_DRAM_OUT0_END | CTL_SIE_INTE_VD;
							ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_REG_CB_IMM, (void *)&cb_info);

							cb_info.cbevt = CTL_SIE_CBEVT_BUFIO;
							cb_info.fp = p_ctx->buf_io_cb;
							cb_info.sts = CTL_SIE_BUF_IO_ALL;
							ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_REG_CB_IMM, (void *)&cb_info);
						}
						seq++;
					}
				}
			}
		}
	}
	return ret;
}
static INT32 _vdocap_close_shdr_sie(VDOCAP_SEN_HDR_MAP shdr_map)
{
	UINT32 shdr_set;
	UINT32 i, temp;

	shdr_set = _vdocap_shdr_map_to_set(shdr_map);

	for (i = 0; i < VDOCAP_MAX_NUM; i++) {
		temp  = sen_out_dest[shdr_set] >> i;
		if (temp & 0x1) {
			VDOCAP_CONTEXT *p_ctx;

			p_ctx = _vdocap_id_to_ctx(i);
			if (NULL == p_ctx) {
				DBG_ERR("VDOCAP[%d] shdr_map(0x%X) NOT in active list!(0x%X)\r\n", i, sen_out_dest[shdr_set], _vdocap_active_list);
				return ISF_ERR_PROCESS_FAIL;
			}
			p_ctx->dev_trigger_close = 1;
			DBG_UNIT("VDOCAP[%d] ctl_sie_close ++\r\n", p_ctx->id);
			if (ctl_sie_close(p_ctx->sie_hdl) != E_OK) {
				DBG_ERR("- VDOCAP[%d] close ctl_sie failed!\r\n", p_ctx->id);
				return ISF_ERR_FAILED;
			}
			DBG_UNIT("VDOCAP[%d] ctl_sie_close --\r\n", p_ctx->id);
			p_ctx->sie_hdl = 0;
			p_ctx->dev_trigger_close = 0;
		}
	}
	return ISF_OK;
}
static void _vdocap_dump_sen_init(ISF_UNIT *p_thisunit, VDOCAP_SEN_INIT_CFG *p_data)
{
	UINT32 *p_map;
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	DBG_MSG("######## VDOCAP[%d]sensor init  ########\r\n", p_ctx->id);
	DBG_MSG("driver_name = %s\r\n", p_data->driver_name);
	DBG_MSG("pin_cfg.pinmux.sensor_pinmux = 0x%X\r\n",
																	p_data->sen_init_cfg.pin_cfg.pinmux.sensor_pinmux);
	DBG_MSG("       .pinmux.serial_if_pinmux = 0x%X\r\n",
																	p_data->sen_init_cfg.pin_cfg.pinmux.serial_if_pinmux);
	DBG_MSG("       .pinmux.cmd_if_pinmux = 0x%X\r\n", p_data->sen_init_cfg.pin_cfg.pinmux.cmd_if_pinmux);
	DBG_MSG("       .clk_lane_sel = %d\r\n", p_data->sen_init_cfg.pin_cfg.clk_lane_sel);
	DBG_MSG("       .sen_2_serial_pin_map[] =\r\n");
	p_map = p_data->sen_init_cfg.pin_cfg.sen_2_serial_pin_map;
	DBG_MSG("{%d, %d, %d, %d, %d, %d, %d, %d}\r\n", p_map[0], p_map[1], p_map[2], p_map[3], p_map[4], p_map[5], p_map[6], p_map[7]);
	DBG_MSG("       .ccir_msblsb_switch = %d\r\n", p_data->sen_init_cfg.pin_cfg.ccir_msblsb_switch);
	DBG_MSG("       .ccir_vd_hd_pin = %d\r\n", p_data->sen_init_cfg.pin_cfg.ccir_vd_hd_pin);
	DBG_MSG("       .vx1_tx241_cko_pin = %d\r\n", p_data->sen_init_cfg.pin_cfg.vx1_tx241_cko_pin);
	DBG_MSG("       .vx1_tx241_cfg_2lane_mode = %d\r\n", p_data->sen_init_cfg.pin_cfg.vx1_tx241_cfg_2lane_mode);

	DBG_MSG("if_cfg.type = %d\r\n", p_data->sen_init_cfg.if_cfg.type);
	DBG_MSG("      .tge.tge_en = %d\r\n", p_data->sen_init_cfg.if_cfg.tge.tge_en);
	DBG_MSG("      .tge.swap = %d\r\n", p_data->sen_init_cfg.if_cfg.tge.swap);
	DBG_MSG("      .tge.sie_vd_src = %d\r\n", p_data->sen_init_cfg.if_cfg.tge.sie_vd_src);
	DBG_MSG("      .tge.sie_sync_set = %d\r\n", p_data->sen_init_cfg.if_cfg.tge.sie_sync_set);
	DBG_MSG("      .mclksrc_sync = 0x%X\r\n", p_ctx->mclksrc_sync);
	DBG_MSG("      .pdaf_map = 0x%X\r\n", p_ctx->pdaf_map);

	DBG_MSG("cmd_if_cfg.vx1.en = %d\r\n", p_data->sen_init_cfg.cmd_if_cfg.vx1.en);
	DBG_MSG("          .vx1.if_sel = %d\r\n", p_data->sen_init_cfg.cmd_if_cfg.vx1.if_sel);
	DBG_MSG("          .vx1.ctl_sel = %d\r\n", p_data->sen_init_cfg.cmd_if_cfg.vx1.ctl_sel);
	DBG_MSG("          .vx1.tx_type = %d\r\n", p_data->sen_init_cfg.cmd_if_cfg.vx1.tx_type);

	DBG_MSG("option.en_mask = %d\r\n", p_data->sen_init_option.en_mask);
	DBG_MSG("      .sen_map_if = %d\r\n", p_data->sen_init_option.sen_map_if);
	DBG_MSG("      .if_time_out = %d\r\n", p_data->sen_init_option.if_time_out);

	DBG_MSG("#######################################\r\n");
}
static void _vdocap_dump_chgsenmode_info(ISF_UNIT *p_thisunit, CTL_SIE_CHGSENMODE_INFO *p_data)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	DBG_MSG("######## VDOCAP[%d]chgsenmode_info  ########\r\n", p_ctx->id);
	if (p_data->sel == CTL_SEN_MODESEL_AUTO)  {
		DBG_MSG("sel = AUTO\r\n");
	} else {
		DBG_MSG("sel = MANUAL\r\n");
	}
	DBG_MSG("auto_info.frame_rate = %d\r\n", p_data->auto_info.frame_rate);
	DBG_MSG("         .size.w = %d\r\n", p_data->auto_info.size.w);
	DBG_MSG("              .h = %d\r\n", p_data->auto_info.size.h);
	DBG_MSG("         .frame_num = %d\r\n", p_data->auto_info.frame_num);
	DBG_MSG("         .data_fmt = %d\r\n", p_data->auto_info.data_fmt);
	DBG_MSG("         .pixdepth = %d\r\n", p_data->auto_info.pixdepth);
	DBG_MSG("         .ccir.fmt = %d\r\n", p_data->auto_info.ccir.fmt);
	DBG_MSG("              .interlace = %d\r\n", p_data->auto_info.ccir.interlace);
	DBG_MSG("         .mux_singnal_en = %d\r\n", p_data->auto_info.mux_singnal_en);
	DBG_MSG("         .mux_data_num = refer to DTS!\r\n");
	DBG_MSG("         .mode_type_sel = %lld\r\n", p_data->auto_info.mode_type_sel);
	DBG_MSG("         .data_lane = %d\r\n", p_data->auto_info.data_lane);
	DBG_MSG("manual_info.frame_rate = %d\r\n", p_data->manual_info.frame_rate);
	DBG_MSG("           .sen_mode = %d\r\n", p_data->manual_info.sen_mode);
	DBG_MSG("output_dest = 0x%X\r\n", p_data->output_dest);
	DBG_MSG("ccir field sel = %d, mux index = %d\r\n", p_ctx->ccir_field_sel, p_ctx->mux_data_index);
	DBG_MSG("#######################################\r\n");
}
static void _vdocap_dump_io_size(ISF_UNIT *p_thisunit, CTL_SIE_IO_SIZE_INFO *p_data, BOOL after_setting)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	if (after_setting) {
		DBG_MSG("######## VDOCAP[%d]SIZE INFO RESULT ########\r\n", p_ctx->id);
	} else {
		DBG_MSG("######## VDOCAP[%d]SIZE INFO  ########\r\n", p_ctx->id);
	}
	if (p_data->iosize_sel == CTL_SIE_IOSIZE_AUTO)  {
		DBG_MSG("sel = AUTO\r\n");
	} else {
		DBG_MSG("sel = MANUAL\r\n");
	}
	DBG_MSG("auto_info.crp_sel = %d\r\n", p_data->auto_info.crp_sel);
	DBG_MSG("         .ratio_h_v = 0x%X\r\n", p_data->auto_info.ratio_h_v);
	DBG_MSG("         .factor = %d\r\n", p_data->auto_info.factor);
	DBG_MSG("         .sft.x = %d\r\n", p_data->auto_info.sft.x);
	DBG_MSG("             .y = %d\r\n", p_data->auto_info.sft.y);
	DBG_MSG("         .sie_crp_min.w = %d\r\n", p_data->auto_info.sie_crp_min.w);
	DBG_MSG("                     .h = %d\r\n", p_data->auto_info.sie_crp_min.h);
	DBG_MSG("         .sie_scl_max_sz.w = %X\r\n", p_data->auto_info.sie_scl_max_sz.w);
	DBG_MSG("                     .h = %X\r\n", p_data->auto_info.sie_scl_max_sz.h);
	DBG_MSG("         .dest_ext_crp_prop.w = %d\r\n", p_data->auto_info.dest_ext_crp_prop.w);
	DBG_MSG("                     .h = %d\r\n", p_data->auto_info.dest_ext_crp_prop.h);
	DBG_MSG("size_info.sie_crp.x = %d\r\n", p_data->size_info.sie_crp.x);
	DBG_MSG("                 .y = %d\r\n", p_data->size_info.sie_crp.y);
	DBG_MSG("                 .w = %d\r\n", p_data->size_info.sie_crp.w);
	DBG_MSG("                 .h = %d\r\n", p_data->size_info.sie_crp.h);
	DBG_MSG("         .sie_scl_out.w = %d\r\n", p_data->size_info.sie_scl_out.w);
	DBG_MSG("                     .h = %d\r\n", p_data->size_info.sie_scl_out.h);
	DBG_MSG("         .dest_crp.x = %d\r\n", p_data->size_info.dest_crp.x);
	DBG_MSG("                  .y = %d\r\n", p_data->size_info.dest_crp.y);
	DBG_MSG("                  .w = %d\r\n", p_data->size_info.dest_crp.w);
	DBG_MSG("                  .h = %d\r\n", p_data->size_info.dest_crp.h);
	DBG_MSG("align.w = %X\r\n", p_data->align.w);
	DBG_MSG("     .h = %X\r\n", p_data->align.h);
	DBG_MSG("dest_align.w = %X\r\n", p_data->dest_align.w);
	DBG_MSG("          .h = %X\r\n", p_data->dest_align.h);
	DBG_MSG("#######################################\r\n");
}
static void _vdocap_dump_pat_gen_info(ISF_UNIT *p_thisunit, CTL_SIE_PAG_GEN_INFO *p_data)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	DBG_MSG("######## VDOCAP[%d]PATTERN GEN INFO  ########\r\n", p_ctx->id);
	DBG_MSG("act_win = [%d %d %d %d]\r\n", p_data->act_win.x,
																		p_data->act_win.y,
																		p_data->act_win.w,
																		p_data->act_win.h);
	DBG_MSG("crp_win = [%d %d %d %d]\r\n", p_data->crp_win.x,
																		p_data->crp_win.y,
																		p_data->crp_win.w,
																		p_data->crp_win.h);
	DBG_MSG("pat_gen_mode = %d\r\n", p_data->pat_gen_mode);
	DBG_MSG("pat_gen_val = 0x%X\r\n", p_data->pat_gen_val);
	DBG_MSG("frame_rate = %d\r\n", p_data->frame_rate);
	DBG_MSG("out_dest = 0x%X\r\n", p_data->out_dest);
	DBG_MSG("#######################################\r\n");
}
static void _vdocap_dump_sw_vd_sync_info(ISF_UNIT *p_thisunit, CTL_SIE_SYNC_INFO *p_data)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	DBG_MSG("######## VDOCAP[%d]SW VD SYNC(0x%X) INFO  ########\r\n", p_ctx->id, p_ctx->sw_vd_sync);
	DBG_MSG("mode = %d\r\n", p_data->mode);
	DBG_MSG("sync_id = %d\r\n", p_data->sync_id);
	DBG_MSG("det_frm_int = %d\r\n", p_data->det_frm_int);
	DBG_MSG("adj_thres = %d\r\n", p_data->adj_thres);
	DBG_MSG("adj_auto = %d\r\n", p_data->adj_auto);
	DBG_MSG("#######################################\r\n");
}
ISF_RV _isf_vdocap_bindouput(struct _ISF_UNIT *p_thisunit, UINT32 oport, struct _ISF_UNIT *p_destunit, UINT32 iport)
{
	// verify connect limit
	VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);
	//DBG_MSG("[%d] p_destunit=0x%X\r\n", p_ctx->id, (UINT32)p_destunit);
	//direct mode only support single sensor with SIE1 or single SHDR with SIE1/2
	if (p_thisunit != &isf_vdocap0) {
		return ISF_OK;
	}
	if ((iport & 0x80000000) || (oport & 0x80000000)) { //unbind
		//iport &= ~0x80000000; //clear flag
		oport &= ~0x80000000; //clear flag
		if (oport == ISF_OUT(0)) {
			//DBG_DUMP(" \"%s\".out[%d] unbind to \"%s\".in[%d] \r\n",
			//	p_thisunit->unit_name, oport - ISF_OUT_BASE, p_destunit->unit_name, iport - ISF_IN_BASE);
			if (_vdocap_is_direct_flow(p_ctx)) {
				ISF_STATE *p_state = p_thisunit->port_outstate[0];
				if(p_state->state == ISF_PORT_STATE_RUN) {  // vcap is still start
					DBG_ERR("[direct] vcap cannot unbind before vcap stop!\r\n");
					return ISF_ERR_ALREADY_START;
				}
			}
			p_ctx->ipp_dir_cb = NULL;
			p_ctx->p_destunit = NULL;
		} else {
			return ISF_ERR_INVALID_PORT_ID;
		}
	} else	{ //bind
		if (oport == ISF_OUT(0)) {
			//DBG_DUMP(" \"%s\".out[%d] bind to \"%s\".in[%d] \r\n",
			//	p_thisunit->unit_name, oport - ISF_OUT_BASE, p_destunit->unit_name, iport - ISF_IN_BASE);
			if (_vdocap_is_direct_flow(p_ctx)) {

				UINT32 cb = p_destunit->do_getportparam(p_destunit, ISF_IN(0), 0x80001010+0x00000100);//VDOPRC_PARAM_DIRECT_CB
				p_ctx->ipp_dir_cb = (CTL_SIE_EVENT_FP)cb;
				if (p_ctx->ipp_dir_cb) {
					DBG_MSG("ipp_dir_cb = 0x%X\r\n", (UINT32)p_ctx->ipp_dir_cb);
				} else {
					DBG_ERR("[direct] vcap cannot bind before vprc open!\r\n");
					return ISF_ERR_NOT_OPEN_YET;
				}

				if (_vcap_gyro_obj[0] && p_ctx->gyro_info.en) {
					cb = p_destunit->do_getportparam(p_destunit, ISF_IN(0), 0x80001010+0x00000102);//VDOPRC_PARAM_EIS_TRIG_CB
					p_ctx->eis_trig_cb = (EIS_TRIG_FP)cb;
				}
			}
			p_ctx->p_destunit = p_destunit;

		} else {
			return ISF_ERR_INVALID_PORT_ID;
		}
	}
	return ISF_OK;
}

INT32 _vdocap_sie_isr_cb(ISF_UNIT *p_thisunit, UINT32 msg, void *p_in, void *p_out)
{
	//ben to do,
	#if 0
	if (g_vdocap_p_func[id]) {
		g_vdocap_p_func[id](msg, p_in, p_out);
	}
	#endif
	return ISF_OK;
}

ISF_RV _isf_vdocap_do_offsync(ISF_UNIT *p_thisunit, UINT32 oport)
{

	return ISF_OK;
}
static ER _vdocap_sensor_ctl(PCTL_SEN_OBJ p_sen_ctl_obj, VDOCAP_CONTEXT *p_ctx, BOOL open)
{
	ER ret = E_OK;

	DBG_FUNC("sensor[%d] open/close %d ++\r\n", p_ctx->id, open);

	if (p_sen_ctl_obj->set_cfg == NULL || p_sen_ctl_obj->pwr_ctrl == NULL) {
		DBG_ERR("ctl_sen obj error\r\n");
		return ISF_ERR_NOT_AVAIL;
	}
	if (open) {
		if (p_ctx->sen_option_en & VDOCAP_SEN_ENABLE_MAP_IF) {
//			ret |= p_sen_ctl_obj->set_cfg(CTL_SEN_CFGID_INIT_IF_MAP, (void *)&p_ctx->sen_map_if);
		}
		if (p_ctx->sen_option_en & VDOCAP_SEN_ENABLE_IF_TIMEOUT) {
			ret |= p_sen_ctl_obj->set_cfg(CTL_SEN_CFGID_INIT_IF_TIMEOUT_MS, (void *)&p_ctx->sen_timeout_ms);
			if (ret != CTL_SEN_E_OK) {
				DBG_ERR("VDOCAP[%d] INIT_IF_TIMEOUT_MS failed(%d)\r\n", p_ctx->id, ret);
				return ISF_ERR_INVALID_VALUE;
			}
		}
		#if 0
		if (p_ctx->csi.cb) {
			ret |= p_sen_ctl_obj->set_cfg(CTL_SEN_CFGID_INIT_MIPI_CB, (void *)&p_ctx->csi.cb);
		}
		#endif
		if (_vdocap_is_tge_sync_mode(p_ctx)) {
						//signal sync
			DBG_MSG("VDOCAP[%d] SIGNAL_SYNC = 0x%X\r\n", p_ctx->id, p_ctx->tge_sync_id);
			ret = p_sen_ctl_obj->set_cfg(CTL_SEN_CFGID_INIT_SIGNAL_SYNC, (void *)&p_ctx->tge_sync_id);
			if (ret){
				DBG_ERR("VDOCAP[%d] INIT_SIGNAL_SYNC failed(%d)\r\n", p_ctx->id, ret);
				return ISF_ERR_INVALID_VALUE;
			}
		}
		ret = p_sen_ctl_obj->pwr_ctrl(CTL_SEN_PWR_CTRL_TURN_ON);
		if (ret != CTL_SEN_E_OK) {
			DBG_ERR("VDOCAP[%d] PWR_CTRL_TURN_ON failed(%d)\r\n", p_ctx->id, ret);
			return ISF_ERR_NOT_AVAIL;
		}
		//for ccir sensor
		if (p_ctx->ad_map) {
			CTL_SEN_AD_ID_MAP_PARAM id_map;
//			BOOL act;

			id_map.chip_id = GET_AD_CHIPID(p_ctx->ad_map);
			id_map.vin_id = GET_AD_IN(p_ctx->ad_map);
			ret = p_sen_ctl_obj->set_cfg(CTL_SEN_CFGID_AD_ID_MAP, (void *)&id_map);

			if (ret != CTL_SEN_E_OK) {
//				DBG_ERR("[%d] set id map(%d,%d) fail\r\n", (UINT32)p_ctx->id, id_map.vin_id, id_map.vout_id);
				DBG_ERR("[%d] set id map(%d) fail(%d)\r\n", (UINT32)p_ctx->id, id_map.vin_id, ret);
				return E_SYS;
			}

#if 0
			act = TRUE;
			if (p_sen_ctl_obj->set_cfg(CTL_SEN_CFGID_AD_ACTIVE, (void *)&act) != E_OK) {
				DBG_ERR("[%d] set active fail\r\n", (UINT32)p_ctx->id);
				return E_SYS;
			}
#endif
		}
		if (p_ctx->ad_type) {
			CTL_SEN_AD_IMAGE_PARAM ad_type = {0};

			ad_type.vin_id = GET_AD_IN(p_ctx->ad_map);
			ad_type.val = p_ctx->ad_type;
			ret = p_sen_ctl_obj->set_cfg(CTL_SEN_CFGID_AD_TYPE, (void *)&ad_type);
			if (ret != CTL_SEN_E_OK) {
				DBG_ERR("[%d] set ad_type(%d) fail(%d)\r\n", (UINT32)p_ctx->id, p_ctx->ad_type, ret);
				return E_SYS;
			}
		}
	} else {
		ret = p_sen_ctl_obj->pwr_ctrl(CTL_SEN_PWR_CTRL_TURN_OFF);
		if (ret != CTL_SEN_E_OK) {
			DBG_ERR("VDOCAP[%d] PWR_CTRL_TURN_OFF failed(%d)\r\n", p_ctx->id, ret);
			return ISF_ERR_NOT_AVAIL;
		}
	}
	DBG_FUNC("sensor[%d] open/close %d --\r\n", p_ctx->id, open);
	return ret;
}
static void _vdocap_pixfmt_to_siefmt(UINT32 pxlfmt, CTL_SIE_DATAFORMAT *p_data_fmt, BOOL *p_compressed)
{
	CTL_SIE_DATAFORMAT data_fmt = CTL_SIE_BAYER_8;

	*p_compressed = FALSE;

	switch (pxlfmt & 0xFFFF0000) {
	case VDO_PXLFMT_RAW8:
	case VDO_PXLFMT_RAW8_SHDR2:
	case VDO_PXLFMT_RAW8_SHDR3:
	case VDO_PXLFMT_RAW8_SHDR4:
		data_fmt = CTL_SIE_BAYER_8;
		break;
	case VDO_PXLFMT_RAW10:
	case VDO_PXLFMT_RAW10_SHDR2:
	case VDO_PXLFMT_RAW10_SHDR3:
	case VDO_PXLFMT_RAW10_SHDR4:
		data_fmt = CTL_SIE_BAYER_10;
		break;
	case VDO_PXLFMT_RAW12:
	case VDO_PXLFMT_RAW12_SHDR2:
	case VDO_PXLFMT_RAW12_SHDR3:
	case VDO_PXLFMT_RAW12_SHDR4:
		data_fmt = CTL_SIE_BAYER_12;
		break;
	case VDO_PXLFMT_RAW16:
	case VDO_PXLFMT_RAW16_SHDR2:
	case VDO_PXLFMT_RAW16_SHDR3:
	case VDO_PXLFMT_RAW16_SHDR4:
		data_fmt = CTL_SIE_BAYER_16;
		break;
	case VDO_PXLFMT_NRX8:
	case VDO_PXLFMT_NRX10:
	case VDO_PXLFMT_NRX14:
	case VDO_PXLFMT_NRX16:
		DBG_ERR("compress fmt(0x%X) only support VDO_PXLFMT_NRX12\r\n", pxlfmt);
		break;
	case VDO_PXLFMT_NRX12:
	case VDO_PXLFMT_NRX12_SHDR2:
	case VDO_PXLFMT_NRX12_SHDR3:
	case VDO_PXLFMT_NRX12_SHDR4:
		data_fmt = CTL_SIE_BAYER_12;
		*p_compressed = TRUE;
		break;
	default:
		if (pxlfmt == VDO_PXLFMT_YUV422) {
			data_fmt = CTL_SIE_YUV_422_SPT;
		} else if (pxlfmt == VDO_PXLFMT_YUV420) {
			data_fmt = CTL_SIE_YUV_420_SPT;
		} else if (pxlfmt == VDO_PXLFMT_YUV422_ONE) {
			data_fmt = CTL_SIE_YUV_422_NOSPT;
		} else {
			DBG_ERR("-VdoIN:incorrect fmt(0x%X)!\r\n", pxlfmt);
		}
		break;
	}

	*p_data_fmt = data_fmt;
}

static void _vdocap_pixfmt_to_senfmt(UINT32 pxlfmt, CTL_SEN_DATA_FMT *p_data_fmt, CTL_SEN_PIXDEPTH *p_pixdepth)
{
	if (pxlfmt == VDO_PXLFMT_YUV422) {
		*p_data_fmt = CTL_SEN_DATA_FMT_YUV;
		*p_pixdepth = CTL_SEN_IGNORE;

	} else {
		switch (pxlfmt & 0xFFFF0000) {
		case VDO_PXLFMT_RAW8:
		case VDO_PXLFMT_RAW8_SHDR2:
		case VDO_PXLFMT_RAW8_SHDR3:
		case VDO_PXLFMT_RAW8_SHDR4:
			*p_pixdepth = CTL_SEN_PIXDEPTH_8BIT;
			break;
		case VDO_PXLFMT_RAW10:
		case VDO_PXLFMT_RAW10_SHDR2:
		case VDO_PXLFMT_RAW10_SHDR3:
		case VDO_PXLFMT_RAW10_SHDR4:
			*p_pixdepth = CTL_SEN_PIXDEPTH_10BIT;
			break;
		case VDO_PXLFMT_RAW12:
		case VDO_PXLFMT_RAW12_SHDR2:
		case VDO_PXLFMT_RAW12_SHDR3:
		case VDO_PXLFMT_RAW12_SHDR4:
		case VDO_PXLFMT_NRX12:
		case VDO_PXLFMT_NRX12_SHDR2:
		case VDO_PXLFMT_NRX12_SHDR3:
		case VDO_PXLFMT_NRX12_SHDR4:
			*p_pixdepth = CTL_SEN_PIXDEPTH_12BIT;
			break;
		case VDO_PXLFMT_RAW16:
		case VDO_PXLFMT_RAW16_SHDR2:
		case VDO_PXLFMT_RAW16_SHDR3:
		case VDO_PXLFMT_RAW16_SHDR4:
			*p_pixdepth = CTL_SEN_PIXDEPTH_16BIT;
			break;
		default:
			*p_pixdepth = CTL_SEN_PIXDEPTH_8BIT;
			DBG_ERR("-VdoIN:incorrect pxlfmt(0x%X)!\r\n", pxlfmt);
			break;
		}
		switch (pxlfmt & 0x0000FFFF) {
		case VDO_PIX_RCCB:
			*p_data_fmt = CTL_SEN_DATA_FMT_RCCB;
			break;
		case VDO_PIX_RGBIR:
			*p_data_fmt = CTL_SEN_DATA_FMT_RGBIR;
			break;
		case VDO_PIX_Y:
			*p_data_fmt = CTL_SEN_DATA_FMT_Y_ONLY;
			break;
		default: //default VDO_PIX_RGB
			*p_data_fmt = CTL_SEN_DATA_FMT_RGB;
			break;
		}
	}
}
static void _vdocap_senfmt_to_pixfmt(CTL_SEN_DATA_FMT data_fmt, CTL_SEN_PIXDEPTH pixdepth, UINT32 *p_pxlfmt)
{
	if (data_fmt == CTL_SEN_DATA_FMT_YUV) {
		*p_pxlfmt = VDO_PXLFMT_YUV422;
	} else if (data_fmt == CTL_SEN_DATA_FMT_Y_ONLY) {
		*p_pxlfmt = VDO_PXLFMT_Y8;
	} else {
		switch (pixdepth) {
		case CTL_SEN_PIXDEPTH_8BIT:
			*p_pxlfmt = VDO_PXLFMT_RAW8;
			break;
		case CTL_SEN_PIXDEPTH_10BIT:
			*p_pxlfmt = VDO_PXLFMT_RAW10;
			break;
		case CTL_SEN_PIXDEPTH_12BIT:
			*p_pxlfmt = VDO_PXLFMT_RAW12;
			break;
		case CTL_SEN_PIXDEPTH_14BIT:
			*p_pxlfmt = VDO_PXLFMT_RAW14;
			break;
		case CTL_SEN_PIXDEPTH_16BIT:
			*p_pxlfmt = VDO_PXLFMT_RAW16;
			break;
		default:
			*p_pxlfmt = CTL_SEN_PIXDEPTH_8BIT;
			DBG_ERR("-VdoIN:incorrect pixdepth(0x%X)!\r\n", pixdepth);
			break;
		}
	}
}
static void _vdocap_shdr_map_outdest(UINT32 id, UINT32 shdr_map)
{
	switch (shdr_map) {
	case VDOCAP_SEN_HDR_SET1_MAIN:
	case VDOCAP_SEN_HDR_SET1_SUB1:
	case VDOCAP_SEN_HDR_SET1_SUB2:
	case VDOCAP_SEN_HDR_SET1_SUB3:
		sen_out_dest[VDOCAP_SHDR_SET1] |= (1 << id);
		break;
	case VDOCAP_SEN_HDR_SET2_MAIN:
	case VDOCAP_SEN_HDR_SET2_SUB1:
		sen_out_dest[VDOCAP_SHDR_SET2] |= (1 << id);
		break;
	default:
		break;
	}
}

static CTL_SIE_FLIP_TYPE _vdocap_direction_mapping(VDOCAP_OUT_DIR dir)
{
	CTL_SIE_FLIP_TYPE ret = CTL_SIE_FLIP_NONE;

	if (dir == VDOCAP_OUT_DIR_H) {
		ret = CTL_SIE_FLIP_H;
	} else if (dir == VDOCAP_OUT_DIR_V) {
		ret = CTL_SIE_FLIP_V;
	} else if (dir == VDOCAP_OUT_DIR_H_V) {
		ret = CTL_SIE_FLIP_H_V;
	}

	DBG_VALUE("set DIR from 0x%X to 0x%X\r\n", dir, ret);
	return ret;
}
static UINT64 _vdocap_mode_type_mapping(VDOCAP_SEN_MODE_TYPE type)
{
	UINT64 value = CTL_SEN_IGNORE;

	switch (type) {
	case VDOCAP_SEN_MODE_TYPE_UNKNOWN:
		value = CTL_SEN_IGNORE;
		break;
	case VDOCAP_SEN_MODE_LINEAR:
		value = CTL_SEN_MODE_LINEAR;
		break;
	case VDOCAP_SEN_MODE_BUILTIN_HDR:
		value = CTL_SEN_MODE_BUILTIN_HDR;
		break;
	case VDOCAP_SEN_MODE_CCIR:
		value = CTL_SEN_MODE_CCIR;
		break;
	case VDOCAP_SEN_MODE_CCIR_INTERLACE:
		value = CTL_SEN_MODE_CCIR_INTERLACE;
		break;
	case VDOCAP_SEN_MODE_BUILTIN_DCG_HDR:
		value = CTL_SEN_MODE_BUILTIN_DCG_HDR;
		break;
	case VDOCAP_SEN_MODE_STAGGER_HDR:
		value = CTL_SEN_MODE_STAGGER_HDR;
		break;
	case VDOCAP_SEN_MODE_PDAF:
		value = CTL_SEN_MODE_PDAF;
		break;
	case VDOCAP_SEN_MODE_BUILTIN_DCG_SHDR:
		value = CTL_SEN_MODE_BUILTIN_DCG_SHDR;
		break;
	case VDOCAP_SEN_MODE_DCG_HDR:
		value = CTL_SEN_MODE_DCG_HDR;
		break;
	default:
		DBG_ERR("unsupported mode type(%d)!\r\n", type);
		break;
	}

	return value;
}
static void _vdocap_set_alg_func(UINT32 sie_hdl, CTL_SIE_ALG_TYPE type, BOOL enable)
{
	CTL_SIE_ALG_FUNC alg_func = {0};

	DBG_VALUE("sie_hdl[%d] ALG ID TYPE[%d] = %d\r\n", sie_hdl, type, enable);
	alg_func.type = type;
	alg_func.func_en = enable;
	ctl_sie_set(sie_hdl, CTL_SIE_ITEM_ALG_FUNC_IMM, (void *)&alg_func);

}
static void _vdocap_config_alg_func(ISF_UNIT *p_thisunit)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 sie_hdl = p_ctx->sie_hdl;

	DBG_MSG("alg_func = 0x%X\r\n", p_ctx->alg_func);
	_vdocap_set_alg_func(sie_hdl, CTL_SIE_ALG_TYPE_AWB, (p_ctx->alg_func & VDOCAP_ALG_FUNC_AWB) != 0);
	_vdocap_set_alg_func(sie_hdl, CTL_SIE_ALG_TYPE_AE, (p_ctx->alg_func & VDOCAP_ALG_FUNC_AE) != 0);
	_vdocap_set_alg_func(sie_hdl, CTL_SIE_ALG_TYPE_AF, (p_ctx->alg_func & VDOCAP_ALG_FUNC_AF) != 0);
	_vdocap_set_alg_func(sie_hdl, CTL_SIE_ALG_TYPE_SHDR, (p_ctx->alg_func & VDOCAP_ALG_FUNC_SHDR) != 0);
	_vdocap_set_alg_func(sie_hdl, CTL_SIE_ALG_TYPE_WDR, (p_ctx->alg_func & VDOCAP_ALG_FUNC_WDR) != 0);
	_vdocap_set_alg_func(sie_hdl, CTL_SIE_ALG_TYPE_ETH, (p_ctx->alg_func & VDOCAP_ALG_FUNC_ETH) != 0);
}
static void _vdocap_config_raw_compress(ISF_UNIT *p_thisunit)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 sie_hdl = p_ctx->sie_hdl;
	//CTL_SIE_IO_SIZE_INFO io_size;
//	UINT32 ch0_lofs = 0;
	UINT32 enc_rate = _vdocap_enc_rate_mapping(p_ctx->enc_rate);
	CTL_SIE_DATAFORMAT data_fmt = CTL_SIE_BAYER_12;

	ctl_sie_get(sie_hdl, CTL_SIE_ITEM_DATAFORMAT, (void *)&data_fmt);
	if (data_fmt == CTL_SIE_YUV_422_SPT || data_fmt == CTL_SIE_YUV_422_NOSPT || data_fmt == CTL_SIE_YUV_420_SPT) {
		return;
	}


	ctl_sie_set(sie_hdl, CTL_SIE_ITEM_ENC_RATE, (void *)&enc_rate);

	ctl_sie_set(sie_hdl, CTL_SIE_ITEM_ENCODE, (void *)&p_ctx->raw_compress);
	DBG_MSG("raw_compress = 0x%X, enc_rate=%d\r\n", p_ctx->raw_compress, enc_rate);
	ctl_sie_set(sie_hdl, CTL_SIE_ITEM_LOAD, NULL);
}
static void _vdocap_config_bp3_ratio(ISF_UNIT *p_thisunit)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	if (_vdocap_is_direct_flow(p_ctx)) {
		ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_BP3_RATIO_DIRECT, (void *)&p_ctx->bp3_ratio);
	} else {
		ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_BP3_RATIO, (void *)&p_ctx->bp3_ratio);
	}
	DBG_MSG("BP3 RATIO = %d\r\n", p_ctx->bp3_ratio);
	ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_LOAD, NULL);

}
static INT32 _vdocap_config_sensor_mode(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 sie_hdl = p_ctx->sie_hdl;
	INT32 ret;
	CTL_SIE_CCIR_INFO ccir_info;
#if defined(_DVS_FUNC_)
	if (p_ctx->dvs_info.enable) {
		CTL_SIE_DVS_CODE dvs_code;

		dvs_code.positive = p_ctx->dvs_info.positive; // default : 0xff
		dvs_code.negative = p_ctx->dvs_info.negative; // default : 0x00
		dvs_code.nochange = p_ctx->dvs_info.nochange; // default : 0x80

		DBG_MSG("positive=0x%X, negative=0x%X, nochange=0x%X\r\n", dvs_code.positive, dvs_code.negative, dvs_code.nochange);

		ctl_sie_set(sie_hdl, CTL_SIE_ITEM_DVS_CODE, (void *)&dvs_code);
		ctl_sie_set(sie_hdl, CTL_SIE_ITEM_LOAD, NULL);

		p_ctx->chgsenmode_info.auto_info.data_fmt = CTL_SEN_DATA_FMT_DVS;
		p_ctx->chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_8BIT;
	}
#endif
	if (p_ctx->shdr_map == VDOCAP_SEN_HDR_SET1_MAIN) {
		p_ctx->chgsenmode_info.output_dest = sen_out_dest[VDOCAP_SHDR_SET1];
	} else if (p_ctx->shdr_map == VDOCAP_SEN_HDR_SET2_MAIN) {
		p_ctx->chgsenmode_info.output_dest = sen_out_dest[VDOCAP_SHDR_SET2];
	} else if (_vdocap_is_pdaf_mode(p_ctx)) {
		if (p_ctx->id == _vdocap_get_main_pdaf_id(p_ctx)) {
			p_ctx->chgsenmode_info.output_dest = p_ctx->pdaf_map;
		} else {
			return ISF_OK;
		}
	} else {
		p_ctx->chgsenmode_info.output_dest = 1 << p_ctx->sie_id;
	}
	if (p_ctx->mode_type) {
		p_ctx->chgsenmode_info.auto_info.mode_type_sel = _vdocap_mode_type_mapping(p_ctx->mode_type);
	} else if (p_ctx->builtin_hdr) {
		p_ctx->chgsenmode_info.auto_info.mode_type_sel = CTL_SEN_MODE_BUILTIN_HDR;
	} else {
		p_ctx->chgsenmode_info.auto_info.mode_type_sel = CTL_SEN_IGNORE;
	}
	if (p_ctx->data_lane) {
		p_ctx->chgsenmode_info.auto_info.data_lane = p_ctx->data_lane;
	} else {
		p_ctx->chgsenmode_info.auto_info.data_lane = CTL_SEN_IGNORE;
	}
	if (_vdocap_is_mux_mode(p_ctx)) {
		p_ctx->chgsenmode_info.auto_info.mux_singnal_en = TRUE;
	} else {
		p_ctx->chgsenmode_info.auto_info.mux_singnal_en = FALSE;
	}
	_vdocap_dump_chgsenmode_info(p_thisunit, &p_ctx->chgsenmode_info);

	ccir_info.field_sel = p_ctx->ccir_field_sel;
	ctl_sie_set(sie_hdl, CTL_SIE_ITEM_CCIR, (void *)&ccir_info);
	ctl_sie_set(sie_hdl, CTL_SIE_ITEM_MUX_DATA, (void *)&p_ctx->mux_data_index);

	if (p_ctx->ae_preset.enable && p_ctx->flow_type == CTL_SIE_FLOW_SEN_IN) {
		ISP_SENSOR_PRESET_CTRL preset_ctrl = {0};
		PCTL_SEN_OBJ p_sen_obj;

		p_sen_obj = ctl_sen_get_object(p_ctx->id);
		if ((p_sen_obj == NULL) || (p_sen_obj->get_cfg == NULL)) {
			DBG_ERR("get sen_obj[%d] failed!\r\n", p_ctx->id);
			return ISF_ERR_INVALID_VALUE;
		}

		preset_ctrl.mode = ISP_SENSOR_PRESET_AE;
		//preset_ctrl.mode = ISP_SENSOR_PRESET_DEFAULT;
		preset_ctrl.exp_time[0] = p_ctx->ae_preset.exp_time;
		preset_ctrl.gain_ratio[0] = p_ctx->ae_preset.gain_ratio;
		// for HDR
		preset_ctrl.exp_time[1] = (p_ctx->ae_preset.exp_time>>4);
		preset_ctrl.gain_ratio[1] = p_ctx->ae_preset.gain_ratio;
		p_sen_obj->set_cfg(CTL_SEN_CFGID_USER_DEFINE1, &preset_ctrl);
		DBG_MSG("preset exp_time = %d, gain_ratio = %d\r\n\r\n", preset_ctrl.exp_time[0], preset_ctrl.gain_ratio[0]);
	}
	ret = ctl_sie_set(sie_hdl, CTL_SIE_ITEM_CHGSENMODE, (void *)&p_ctx->chgsenmode_info);
	if (ret) {
		DBG_ERR("failed(%d)\r\n", ret);
		return ret;
	}
	ret = ctl_sie_set(sie_hdl, CTL_SIE_ITEM_LOAD, NULL);
	if (ret) {
		DBG_ERR("sie chgmode load failed(%d)\r\n", ret);
		return ret;
	}

	ctl_sie_get(sie_hdl, CTL_SIE_ITEM_CHGSENMODE, (void *)&p_ctx->chgsenmode_info);
	//DBG_MSG("change to sensor mode:%d\r\n", p_ctx->chgsenmode_info.cfg_sen_mode);
	//check frame_num for shdr flow
	if (p_ctx->flow_type == CTL_SIE_FLOW_SEN_IN) {
		CTL_SEN_GET_MODE_BASIC_PARAM mode_param;
		PCTL_SEN_OBJ p_sen_obj;
		ISF_INFO *p_src_info = p_thisunit->port_outinfo[oport - ISF_OUT_BASE];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);

		p_sen_obj = ctl_sen_get_object(p_ctx->id);
		if ((p_sen_obj == NULL) || (p_sen_obj->get_cfg == NULL)) {
			DBG_ERR("get sen_obj[%d] failed!\r\n", p_ctx->id);
			return ISF_ERR_INVALID_VALUE;
		}
		memset((void *)(&mode_param), 0, sizeof(CTL_SEN_GET_MODE_BASIC_PARAM));
		mode_param.mode = CTL_SEN_MODE_CUR;
		if (p_sen_obj->get_cfg(CTL_SEN_CFGID_GET_MODE_BASIC, (void *)(&mode_param))) {
			DBG_ERR("get sen[%d] mode failed!\r\n", p_ctx->id);
			return ISF_ERR_INVALID_VALUE;
		}
		p_ctx->sen_dim.w = mode_param.act_size[0].w;
		p_ctx->sen_dim.h = mode_param.act_size[0].h;
		if (VDO_PXLFMT_CLASS(p_vdoinfo->imgfmt) == VDO_PXLFMT_CLASS_YUV) {
			CTL_SEN_GET_DVI_PARAM mode_dvi_param = {0};

			mode_dvi_param.mode = CTL_SEN_MODE_CUR;
			if (p_sen_obj->get_cfg(CTL_SEN_CFGID_GET_DVI, (void *)(&mode_dvi_param))) {
				DBG_WRN("get dvi fail\r\n");
			} else {
				if (mode_dvi_param.data_mode == CTL_SEN_DVI_DATA_MODE_SD) {
					p_ctx->sen_dim.w /= 2;
				}
			}
		}
		if (p_ctx->shdr_map == VDOCAP_SEN_HDR_SET1_MAIN || p_ctx->shdr_map == VDOCAP_SEN_HDR_SET2_MAIN) {
			//check pixel format with frame_num
			if (((p_vdoinfo->imgfmt & 0x0F000000) >> 24) != mode_param.frame_num) {
				DBG_ERR("pixel fmt num error 0x%X\r\n", p_vdoinfo->imgfmt);
				return ISF_ERR_INVALID_VALUE;
			}
			if (p_ctx->shdr_map == VDOCAP_SEN_HDR_SET1_MAIN) {
				_vdocap_shdr_frm_num[VDOCAP_SHDR_SET1] = mode_param.frame_num;
				//_vdocap_shdr_main_unit[VDOCAP_SHDR_SET1] = p_thisunit;
				_vdocap_shdr_oport_initqueue(VDOCAP_SHDR_SET1);
			} else {
				_vdocap_shdr_frm_num[VDOCAP_SHDR_SET2] = mode_param.frame_num;
				//_vdocap_shdr_main_unit[VDOCAP_SHDR_SET2] = p_thisunit;
				_vdocap_shdr_oport_initqueue(VDOCAP_SHDR_SET2);
			}
		}
		if (p_ctx->chgsenmode_info.sel == CTL_SEN_MODESEL_AUTO) {
			//check resolution
			if (p_ctx->sen_dim.w != p_ctx->chgsenmode_info.auto_info.size.w || p_ctx->sen_dim.h != p_ctx->chgsenmode_info.auto_info.size.h) {
				DBG_WRN("[SEN_OUT]target %dx%d -> real %dx%d\r\n", p_ctx->chgsenmode_info.auto_info.size.w, p_ctx->chgsenmode_info.auto_info.size.h,p_ctx->sen_dim.w, p_ctx->sen_dim.h);
			}
		}
	}
	return ret;
}
static void _vdocap_config_out_size(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	ISF_INFO *p_src_info = p_thisunit->port_outinfo[oport - ISF_OUT_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
	UINT32 sie_hdl = p_ctx->sie_hdl;
	UINT32 w, h, cx, cy, cw, ch, aw, ah;
	CTL_SIE_DATAFORMAT data_fmt = CTL_SIE_BAYER_12;
//	UINT32 ch0_lofs;

	ctl_sie_get(sie_hdl, CTL_SIE_ITEM_DATAFORMAT, (void *)&data_fmt);

	w = p_vdoinfo->imgsize.w;
	h = p_vdoinfo->imgsize.h;
	cx = p_vdoinfo->window.x;
	cy = p_vdoinfo->window.y;
	cw = p_vdoinfo->window.w;
	ch = p_vdoinfo->window.h;
	aw = p_vdoinfo->imgaspect.w;
    ah = p_vdoinfo->imgaspect.h;

	DBG_MSG("-out:size(%d,%d) aspect(%d,%d) win(%d,%d,%d,%d) fmt=0x%X\r\n",
				 w, h, aw, ah, cx, cy, cw, ch, p_vdoinfo->imgfmt);
#if defined(_DVS_FUNC_)
	if (p_ctx->dvs_info.enable) {
		if (cx || cy || cw || ch) {
			DBG_ERR("Not support cropping in DVS mode\r\n");
		}
		p_ctx->io_size.iosize_sel = CTL_SIE_IOSIZE_DVS_AUTO;
		ctl_sie_set(sie_hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&p_ctx->io_size);
		ctl_sie_set(sie_hdl, CTL_SIE_ITEM_LOAD, NULL);

		ctl_sie_get(sie_hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&p_ctx->io_size);
		_vdocap_dump_io_size(p_thisunit, &p_ctx->io_size, TRUE);
		return;
	}
#endif
	/* update size, framerate, crop */
	if (p_ctx->flow_type == CTL_SIE_FLOW_PATGEN) {
		CTL_SIE_PAG_GEN_INFO pat_gen_info = {0};
		ISF_INFO *p_src_info = p_thisunit->port_outinfo[0];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
		UINT32 start_pxl;

		if (w || h) {
			if (cx || cy || cw || ch) {
				p_ctx->pat_gen_info.crp_win.x = cx;
				p_ctx->pat_gen_info.crp_win.y = cy;
				p_ctx->pat_gen_info.crp_win.w = cw;
				p_ctx->pat_gen_info.crp_win.h = ch;
			} else {
				p_ctx->pat_gen_info.crp_win.x = 0;
				p_ctx->pat_gen_info.crp_win.y = 0;
				p_ctx->pat_gen_info.crp_win.w = w;
				p_ctx->pat_gen_info.crp_win.h = h;
			}
		} else {
			p_ctx->pat_gen_info.crp_win = p_ctx->pat_gen_info.act_win;
		}
		start_pxl = p_vdoinfo->imgfmt & VDO_PIX_BAYER_MASK;
		if (start_pxl == VDO_PIX_RGGB_GR) {
			p_ctx->pat_gen_info.act_win.x = 1;
			p_ctx->pat_gen_info.act_win.y = 0;
		} else if (start_pxl == VDO_PIX_RGGB_GB) {
			p_ctx->pat_gen_info.act_win.x = 0;
			p_ctx->pat_gen_info.act_win.y = 1;
		} else if (start_pxl == VDO_PIX_RGGB_B) {
			p_ctx->pat_gen_info.act_win.x = 1;
			p_ctx->pat_gen_info.act_win.y = 1;
		}
		if (p_vdoinfo->imgfmt == VDO_PXLFMT_YUV422 || p_vdoinfo->imgfmt == VDO_PXLFMT_YUV420) {
			p_ctx->pat_gen_info.act_win.x *= 2;
			p_ctx->pat_gen_info.act_win.w *= 2;
			p_ctx->pat_gen_info.crp_win.x *= 2;
			p_ctx->pat_gen_info.crp_win.w *= 2;
			if (p_ctx->pat_gen_info.pat_gen_mode == CTL_SIE_PAT_COLORBAR) {
				p_ctx->pat_gen_info.pat_gen_val *= 2;
			}
		}
		memcpy((void *)&pat_gen_info, (void *)&p_ctx->pat_gen_info, sizeof(pat_gen_info));
		if (p_ctx->shdr_map) {
			pat_gen_info.out_dest = sen_out_dest[_vdocap_shdr_map_to_set(p_ctx->shdr_map)];
		}
		_vdocap_dump_pat_gen_info(p_thisunit, &pat_gen_info);
		ctl_sie_set(sie_hdl, CTL_SIE_ITEM_PATGEN_INFO, (void *)&pat_gen_info);
#if 0	//Jarkko tmp
		if (data_fmt == CTL_SIE_BAYER_10) {
			ch0_lofs = p_ctx->pat_gen_info.crp_win.w * 10 / 8;
		} else if (data_fmt == CTL_SIE_BAYER_12) {
			ch0_lofs = p_ctx->pat_gen_info.crp_win.w * 12 / 8;
		} else if (data_fmt == CTL_SIE_BAYER_16) {
			ch0_lofs = p_ctx->pat_gen_info.crp_win.w * 16 / 8;
		} else { //ben to do, YUV ?
			ch0_lofs = p_ctx->pat_gen_info.crp_win.w;
		}
		ch0_lofs = ALIGN_CEIL_4(ch0_lofs);
		ctl_sie_set(sie_hdl, CTL_SIE_ITEM_CH0_LOF, (void *)&ch0_lofs);
#endif
	} else {

		if (aw || ah) {
		/* crop auto mode */
			/*
			 * referring to ISF_VDO_INFO.window = VDO_POSE(zoom_shift_x,zoom_shift_y), VDO_SIZE(zoom_scale_factor,1000)
			 * zoom_scale_factor is stored in window.w
			 */
			if (cw > 1000 || cw < 1) {
				DBG_ERR("zoom_scale_factor(%d) shouled be 1 to 1000.\r\n", cw);
				cw = 1000;
			}
			p_ctx->io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
			p_ctx->io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
			p_ctx->io_size.auto_info.ratio_h_v = CTL_SIE_RATIO(aw, ah);
			p_ctx->io_size.auto_info.factor = cw;
			p_ctx->io_size.auto_info.sft.x = cx;
			p_ctx->io_size.auto_info.sft.y = cy;
			p_ctx->io_size.auto_info.sie_crp_min.w = 0;
			p_ctx->io_size.auto_info.sie_crp_min.h = 0;
			if (w || h) { //indicate outsize
				p_ctx->io_size.auto_info.sie_scl_max_sz.w = w;
				p_ctx->io_size.auto_info.sie_scl_max_sz.h = h;
			} else {  //auto outsize
				p_ctx->io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
				p_ctx->io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
			}
		} else if (cw || ch) {
		/*
		 * crop manual mode
		 * ISF_VDO_INFO.window = VDO_POSE(crop_x,crop_y), VDO_SIZE(crop_w,crop_h)
		 */
			p_ctx->io_size.iosize_sel = CTL_SIE_IOSIZE_MANUAL;
			//p_ctx->io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
			p_ctx->io_size.size_info.sie_crp.x = cx;
			p_ctx->io_size.size_info.sie_crp.y = cy;
			p_ctx->io_size.size_info.sie_crp.w = cw;
			p_ctx->io_size.size_info.sie_crp.h = ch;
			if (p_vdoinfo->imgfmt == VDO_PXLFMT_YUV422 || p_vdoinfo->imgfmt == VDO_PXLFMT_YUV420) {
				if (p_ctx->io_size.size_info.sie_crp.x != 0xffffffff) {
					p_ctx->io_size.size_info.sie_crp.x *= 2;
				}
				p_ctx->io_size.size_info.sie_crp.w *= 2;
			}
			if (w || h) { //scaling
				p_ctx->io_size.size_info.sie_scl_out.w = w;
				p_ctx->io_size.size_info.sie_scl_out.h = h;
				p_ctx->io_size.size_info.dest_crp.w = w;
				p_ctx->io_size.size_info.dest_crp.h = h;
			} else {  //no scaling
				p_ctx->io_size.size_info.sie_scl_out.w = cw;
				p_ctx->io_size.size_info.sie_scl_out.h = ch;
				p_ctx->io_size.size_info.dest_crp.w = cw;
				p_ctx->io_size.size_info.dest_crp.h = ch;
			}
			p_ctx->io_size.size_info.dest_crp.x = 0;
			p_ctx->io_size.size_info.dest_crp.y = 0;
		////////temp sulution for pdaf size //////////////
		} else if (_vdocap_is_pdaf_mode(p_ctx) && p_ctx->id != _vdocap_get_main_pdaf_id(p_ctx)) {
			UINT32 pdaf_w, pdaf_h;
			pdaf_w = p_ctx->chgsenmode_info.auto_info.size.w;
			pdaf_h=	 p_ctx->chgsenmode_info.auto_info.size.h;
			p_ctx->io_size.iosize_sel = CTL_SIE_IOSIZE_MANUAL;
			//p_ctx->io_size.auto_info.crp_sel = CTL_SIE_CRP_ON;
			p_ctx->io_size.size_info.sie_crp.x = 0;
			p_ctx->io_size.size_info.sie_crp.y = 0;
			p_ctx->io_size.size_info.sie_crp.w = pdaf_w;
			p_ctx->io_size.size_info.sie_crp.h = pdaf_h;

			p_ctx->io_size.size_info.sie_scl_out.w = pdaf_w;
			p_ctx->io_size.size_info.sie_scl_out.h = pdaf_h;
			p_ctx->io_size.size_info.dest_crp.w = pdaf_w;
			p_ctx->io_size.size_info.dest_crp.h = pdaf_h;

			p_ctx->io_size.size_info.dest_crp.x = 0;
			p_ctx->io_size.size_info.dest_crp.y = 0;

		} else {
		/* full frame */
			p_ctx->io_size.iosize_sel = CTL_SIE_IOSIZE_AUTO;
	        p_ctx->io_size.auto_info.crp_sel = CTL_SIE_CRP_OFF;
    		p_ctx->io_size.auto_info.ratio_h_v = CTL_SIE_DFT;
    		p_ctx->io_size.auto_info.factor = 1000;
			p_ctx->io_size.auto_info.sft.x = 0;
			p_ctx->io_size.auto_info.sft.y = 0;
			p_ctx->io_size.auto_info.sie_crp_min.w = 0;
			p_ctx->io_size.auto_info.sie_crp_min.h = 0;
			if (w || h) { //scaling
				p_ctx->io_size.auto_info.sie_scl_max_sz.w = w;
				p_ctx->io_size.auto_info.sie_scl_max_sz.h = h;
			} else {  //no scaling
				p_ctx->io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
				p_ctx->io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
			}
		}
		//p_ctx->io_size.align.w = CTL_SIE_DFT;
		//p_ctx->io_size.align.h = CTL_SIE_DFT;

		p_ctx->io_size.auto_info.dest_ext_crp_prop.w = 0;
		p_ctx->io_size.auto_info.dest_ext_crp_prop.h = 0;
		//p_ctx->io_size.auto_info.sie_scl_max_sz.w = CTL_SIE_DFT;
		//p_ctx->io_size.auto_info.sie_scl_max_sz.h = CTL_SIE_DFT;
		p_ctx->io_size.dest_align.w = CTL_SIE_DFT;
		p_ctx->io_size.dest_align.h = CTL_SIE_DFT;

		_vdocap_dump_io_size(p_thisunit, &p_ctx->io_size, FALSE);

		ctl_sie_set(sie_hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&p_ctx->io_size);
		/* in order to get real crop size */
		ctl_sie_set(sie_hdl, CTL_SIE_ITEM_LOAD, NULL);

		ctl_sie_get(sie_hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&p_ctx->io_size);
		//_vdocap_dump_io_size(p_thisunit, &p_ctx->io_size, TRUE);
		if (p_vdoinfo->imgfmt != VDO_PXLFMT_YUV422 && p_vdoinfo->imgfmt != VDO_PXLFMT_YUV420) {
			if (!aw && !ah && (cw || ch)) {//check manu crop size
				if (cw != p_ctx->io_size.size_info.dest_crp.w || ch != p_ctx->io_size.size_info.dest_crp.h) {
					DBG_WRN("[IN_CROP]target(%d,%d,%d,%d) -> real(%d,%d,%d,%d)\r\n", cx, cy, cw, ch,
						p_ctx->io_size.size_info.dest_crp.x, p_ctx->io_size.size_info.dest_crp.y,
						p_ctx->io_size.size_info.dest_crp.w, p_ctx->io_size.size_info.dest_crp.h);
				}
			}
		}
		#if 0
		ctl_sie_get(sie_hdl, CTL_SIE_ITEM_IO_SIZE, (void *)&p_ctx->io_size);
		//DBGD(p_ctx->io_size.size_info.sie_crp.w);
		if (data_fmt == CTL_SIE_BAYER_10) {
			ch0_lofs = p_ctx->io_size.size_info.sie_scl_out.w * 10 / 8;
		} else if (data_fmt == CTL_SIE_BAYER_12) {
			ch0_lofs = p_ctx->io_size.size_info.sie_scl_out.w * 12 / 8;
		} else if (data_fmt == CTL_SIE_BAYER_16) {
			ch0_lofs = p_ctx->io_size.size_info.sie_scl_out.w * 16 / 8;
		} else { //ben to do, YUV ?
			ch0_lofs = p_ctx->io_size.size_info.sie_scl_out.w;
		}
		ch0_lofs = ALIGN_CEIL_4(ch0_lofs);
		ctl_sie_set(sie_hdl, CTL_SIE_ITEM_CH0_LOF, (void *)&ch0_lofs);
		#endif
	}
	ctl_sie_set(sie_hdl, CTL_SIE_ITEM_LOAD, NULL);
	return;
}
static void _vdocap_config_direct(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	ISF_INFO *p_src_info = p_thisunit->port_outinfo[oport - ISF_OUT_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
	UINT32 sie_hdl = p_ctx->sie_hdl;

	if (p_vdoinfo->direct == VDO_DIR_NONE) {
		p_ctx->flip = CTL_SIE_FLIP_NONE;
	} else if (p_vdoinfo->direct == VDO_DIR_MIRRORX) {
		p_ctx->flip = CTL_SIE_FLIP_H;
	} else if (p_vdoinfo->direct == VDO_DIR_MIRRORY) {
		p_ctx->flip = CTL_SIE_FLIP_V;
	} else if (p_vdoinfo->direct == (VDO_DIR_MIRRORX|VDO_DIR_MIRRORY)) {
		p_ctx->flip = CTL_SIE_FLIP_H_V;
	}
	ctl_sie_set(sie_hdl, CTL_SIE_ITEM_FLIP, (void *)&p_ctx->flip);
	ctl_sie_set(sie_hdl, CTL_SIE_ITEM_LOAD, NULL);
	DBG_MSG("-out:flip=(%08x)\r\n", p_ctx->flip);
}
static void _vdocap_config_format(ISF_UNIT *p_thisunit, UINT32 oport)
{

	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	ISF_INFO *p_src_info = p_thisunit->port_outinfo[oport - ISF_OUT_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
	UINT32 sie_hdl = p_ctx->sie_hdl;
	CTL_SIE_DATAFORMAT data_fmt;

	_vdocap_pixfmt_to_siefmt(p_vdoinfo->imgfmt, &data_fmt, &p_ctx->raw_compress);
#if defined(_DVS_FUNC_)
	if (p_ctx->dvs_info.enable) {
		data_fmt = CTL_SIE_Y_8;
	}
#endif
	ctl_sie_set(sie_hdl, CTL_SIE_ITEM_DATAFORMAT, (void *)&data_fmt);
	ctl_sie_set(sie_hdl, CTL_SIE_ITEM_LOAD, NULL);
	DBG_MSG("-out:fmt=(%08x)\r\n", data_fmt);
}
static void _vdocap_config_out_dest(ISF_UNIT *p_thisunit)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	if (_vdocap_is_direct_flow(p_ctx)) {
		CTL_SIE_REG_CB_INFO cb_info;

		if (_vdocap_is_shdr_mode(p_ctx->shdr_map)) {//shdr mode
			UINT32 shdr_set = _vdocap_shdr_map_to_set(p_ctx->shdr_map);
			ISF_UNIT *p_main_unit = _vdocap_shdr_main_unit[shdr_set];
			VDOCAP_CONTEXT *p_main_ctx;

			if (p_main_unit) {
				p_main_ctx = (VDOCAP_CONTEXT *)(p_main_unit->refdata);
				cb_info.fp = p_main_ctx->ipp_dir_cb; //register direct mode cb fp from ipp
			} else {
				DBG_ERR("shdr fail:map=%d", (UINT32)p_ctx->shdr_map);
			}
		} else {
			cb_info.fp = p_ctx->ipp_dir_cb; //register direct mode cb fp from ipp
		}
		cb_info.cbevt = CTL_SIE_CBEVT_DIRECT;
		cb_info.sts = CTL_SIE_DIRECT_CFG_ALL;
		DBG_MSG("ipp_dir_cb=(0x%08X)\r\n", (UINT32)p_ctx->ipp_dir_cb);
		ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_REG_CB_IMM, (void *)&cb_info);
	}
	DBG_MSG("out_dest=%d\r\n", p_ctx->out_dest);
	ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_OUT_DEST, (void *)&p_ctx->out_dest);
	ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_LOAD, NULL);
}
static void _vdocap_config_gyro(ISF_UNIT *p_thisunit)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	if (_vcap_gyro_obj[0] && p_ctx->gyro_info.en) {
		CTL_SIE_GYRO_CFG gyro_cfg = {0};

		gyro_cfg.en = TRUE;
		gyro_cfg.p_gyro_obj = _vcap_gyro_obj[0];
		if (p_ctx->gyro_info.data_num == 0) {
			gyro_cfg.data_num = MAX_GYRO_DATA_NUM;
		} else {
			gyro_cfg.data_num = p_ctx->gyro_info.data_num;
		}

		ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_GYRO_CFG, (void *)&gyro_cfg);

		DBG_DUMP("VCAP[%d] enable gyro, data_num=%d\r\n", p_ctx->id, gyro_cfg.data_num);
		ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_LOAD, NULL);
	}
}
static CTL_SEN_MODE vdocap_get_max_dim_mode(ISF_UNIT *p_thisunit, PCTL_SEN_OBJ p_sen_obj)
{
	CTL_SEN_GET_MODE_BASIC_PARAM mode_param = {0};
	CTL_SEN_GET_ATTR_PARAM attr_param = {0};
	UINT32 senmode_num, i;
	UINT32 dim_size, max_dim_size = 0, max_dim_mode = 0;


	if (p_sen_obj->get_cfg(CTL_SEN_CFGID_GET_ATTR, (void *)(&attr_param))) {
		DBG_ERR("get attr failed!\r\n");
		return CTL_SEN_MODE_UNKNOWN;
	}
	senmode_num = attr_param.max_senmode;

	for (i = 0; i < senmode_num; i++) {
		mode_param.mode = i;
		if (p_sen_obj->get_cfg(CTL_SEN_CFGID_GET_MODE_BASIC, (void *)(&mode_param))) {
			DBG_ERR("get mode(%d) info failed!\r\n", mode_param.mode);
			continue;
		}
		DBG_MSG("senmode[%d] %dx%d\r\n", i, mode_param.crp_size.w, mode_param.crp_size.h);
		dim_size = mode_param.crp_size.w * mode_param.crp_size.h;
		if (dim_size > max_dim_size) {
			max_dim_size = dim_size;
			max_dim_mode = i;
		}
	}
	return max_dim_mode;
}
static ISF_RV _isf_vdocap_do_reset(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);

	DBG_UNIT("\r\n");
	if (p_ctx == 0) {
		return ISF_ERR_UNINIT; //still not init
	}

	//reset ctrl state and ctrl parameter
	if(oport == ISF_OUT_BASE) {
		VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);

		//p_ctx->sie_hdl = 0;
		p_ctx->shdr_map = VDOCAP_SEN_HDR_NONE;
		p_ctx->flow_type = CTL_SIE_FLOW_SEN_IN;
		p_ctx->csi.cb_status = 0;
		p_ctx->csi.ok_cnt = 0;
		p_ctx->csi.error = 0;
		//p_ctx->vd_count = 0;
		p_ctx->vd_count_prev = 0;
		p_ctx->sen_dim.w = 0;
		p_ctx->sen_dim.h = 0;
		memset(&(p_ctx->chgsenmode_info), 0, sizeof(CTL_SIE_CHGSENMODE_INFO));
		p_ctx->chgsenmode_info.sel = CTL_SEN_MODESEL_AUTO;
		p_ctx->chgsenmode_info.auto_info.frame_rate = 3000;
		p_ctx->chgsenmode_info.auto_info.size.w = 1920;
		p_ctx->chgsenmode_info.auto_info.size.h = 1080;
		p_ctx->chgsenmode_info.auto_info.frame_num = VDOCAP_SEN_FRAME_NUM_1;
		p_ctx->chgsenmode_info.auto_info.data_fmt = CTL_SEN_DATA_FMT_RGB;
		p_ctx->chgsenmode_info.auto_info.pixdepth = CTL_SEN_PIXDEPTH_8BIT;
		p_ctx->ad_map = 0;
		p_ctx->ad_type = 0;
		p_ctx->ccir_field_sel = CTL_SIE_FIELD_DISABLE;
		p_ctx->mux_data_index = 0;
		p_ctx->out_dest = CTL_SIE_OUT_DEST_DRAM;
		p_ctx->flip = CTL_SIE_FLIP_NONE;
		p_ctx->mode_type = 0;
		p_ctx->alg_func = 0;
		p_ctx->started = 0;
		p_ctx->tge_sync_id = 0;
		p_ctx->mclksrc_sync = 0;
		p_ctx->ipp_dir_cb = NULL;
		p_ctx->one_buf = FALSE;
		p_ctx->builtin_hdr = FALSE;
		p_ctx->data_lane = 0;
		memset(&(p_ctx->pullq), 0, sizeof(VDOCAP_PULL_QUEUE));
		memset(&(p_ctx->out_dbg), 0, sizeof(VDOCAP_OUT_DBG));
		memset(&(p_ctx->pat_gen_info), 0, sizeof(CTL_SIE_PAG_GEN_INFO));
		memset(&(p_ctx->gyro_info), 0, sizeof(VDOCAP_GYRO_INFO));
	}
	//reset output rate debug
	output_rate_update_pause = FALSE;

	//reset in parameter
	if(oport == ISF_OUT_BASE) {
		ISF_INFO *p_src_info = p_thisunit->port_ininfo[0];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
		memset(p_vdoinfo, 0, sizeof(ISF_VDO_INFO));
	}
	//reset out parameter
	{
		UINT32 i = oport - ISF_OUT_BASE;
		ISF_INFO *p_dest_info = p_thisunit->port_outinfo[i];
		ISF_VDO_INFO *p_vdocapfo2 = (ISF_VDO_INFO *)(p_dest_info);
		memset(p_vdocapfo2, 0, sizeof(ISF_VDO_INFO));
	}
	//reset out state
	{
		UINT32 i = oport - ISF_OUT_BASE;
		ISF_STATE *p_state = p_thisunit->port_outstate[i];
		memset(p_state, 0, sizeof(ISF_STATE));
	}
#if 0
	//reset out dest
	memset((void *)sen_out_dest, 0, sizeof(sen_out_dest));
	//reset shdr frame number
	for (set = VDOCAP_SHDR_SET1; set < VDOCAP_SHDR_SET_MAX_NUM; set++) {
		_vdocap_shdr_frm_num[set] = VDOCAP_SEN_FRAME_NUM_1;
	}
	//reset shdr main unit
	memset((void *)_vdocap_shdr_main_unit, 0, sizeof(_vdocap_shdr_main_unit));
#endif
	return ISF_OK;
}
static ISF_RV _isf_vdocap_do_open(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	UINT32 id;

	if (p_ctx == NULL) {
		DBG_UNIT("VDOCAP buf not init!\r\n");
		return ISF_ERR_INIT; //sitll not init
	}
	id = p_ctx->id;
	DBG_UNIT("VDOCAP[%d] port[%d]\r\n", p_ctx->id, oport);

	nvtmpp_vb_add_module(p_thisunit->unit_module);

	if (p_ctx->sie_hdl == 0) {
		CTL_SIE_OPEN_CFG open_cfg = {0};
		CTL_SIE_REG_CB_INFO cb_info;
		CTL_SIE_MCLK sie_mclk;
		PCTL_SEN_OBJ p_sen_ctl_obj = ctl_sen_get_object(id);
		BOOL ctl_sen_is_opened = FALSE;

		if (_vdocap_is_pdaf_mode(p_ctx)) {
			if (id != _vdocap_get_main_pdaf_id(p_ctx)) {
				p_ctx->flow_type = CTL_SIE_FLOW_SIG_DUPL;
			}
		}
		//only main path connect to real sensor
		//if ((p_ctx->flow_type == CTL_SIE_FLOW_SEN_IN) && (_vdocap_is_main_path(p_ctx->shdr_map))) {
		if (p_ctx->flow_type == CTL_SIE_FLOW_SEN_IN || _vdocap_is_mclk_sync_mode(p_ctx)) {
			INT32 ret;
			if (p_sen_ctl_obj == NULL || p_sen_ctl_obj->open == NULL) {
				DBG_ERR("no ctl_sen obj(%d)\r\n", (UINT32)id);
				return ISF_ERR_NOT_AVAIL;
			}
			if (FALSE == p_sen_ctl_obj->is_opened()) {
				if (_vdocap_is_mclk_sync_mode(p_ctx)) {
					if (_vdocap_is_last_mclk_sync_id(p_ctx)) {
						ret = _vdocap_open_all_mclk_sync_ctl_sen(p_ctx);
					} else {
						ret = CTL_SEN_E_OK;
					}
				} else {
					DBG_UNIT("VDOCAP[%d] ctl_sen open ++\r\n", p_ctx->id);
					ret = p_sen_ctl_obj->open();
					DBG_UNIT("VDOCAP[%d] ctl_sen open --\r\n", p_ctx->id);
				}
				if (ret != CTL_SEN_E_OK) {
					switch(ret) {
					case CTL_SEN_E_MAP_TBL:
						DBG_ERR("no sen driver\r\n");
						ret = ISF_ERR_NOT_AVAIL;
						break;
					case CTL_SEN_E_SENDRV:
						DBG_ERR("sensor driver error\r\n");
						ret = ISF_ERR_PROCESS_FAIL;//map to HD_ERR_FAIL
						break;
					default:
						DBG_ERR("sensor open fail(%d)\r\n", ret);
						ret = ISF_ERR_INVALID_VALUE;
						break;
					}
					return ret;

				}
			} else {
				ctl_sen_is_opened = TRUE;
			}
		}
		//specail flow for mclk sync
		if (_vdocap_is_mclk_sync_mode(p_ctx)) {
			INT32 ret;

			if (_vdocap_is_last_mclk_sync_id(p_ctx)) {
						//open ctl_sie and sensor power
				ret = _vdocap_open_all_mclk_sync_rest(p_ctx);
			} else {
				ret = ISF_OK;
			}
			_vdocap_oport_initqueue(p_thisunit);
			_isf_vdocap_oqueue_do_open(p_thisunit, oport);
			return ret;
		}
		if (p_ctx->sie_id != id && p_ctx->shdr_map != VDOCAP_SEN_HDR_NONE) {
			DBG_ERR("[%d]Not support SIE mapping(%d) in SHDR mode!\r\n", id, p_ctx->sie_id);
			return ISF_ERR_NOT_AVAIL;
		}
		// Must set the same clock rate as ssenif reference rate
		open_cfg.id = p_ctx->sie_id;
		open_cfg.flow_type = p_ctx->flow_type;
		open_cfg.sen_id = id;
		if (p_ctx->sie_id != id) {
			open_cfg.isp_id = ((1 << 31) | (id << 24) | p_ctx->sie_id);
		} else {
			open_cfg.isp_id = id;
		}
		if (p_ctx->flow_type == CTL_SIE_FLOW_SIG_DUPL) {
			if (p_ctx->shdr_map) {
				ISF_UNIT *p_main_unit = _vdocap_shdr_main_unit[_vdocap_shdr_map_to_set(p_ctx->shdr_map)];
				VDOCAP_CONTEXT *p_main_ctx;

				if (p_main_unit) {
					p_main_ctx = (VDOCAP_CONTEXT *)(p_main_unit->refdata);
					open_cfg.dupl_src_id = p_main_ctx->id;
					open_cfg.sen_id = open_cfg.dupl_src_id;
					//since SEN_INIT_CFG is only for HDR main path, tge_sync_id needs to sync for HDR sub-path
					p_ctx->tge_sync_id = p_main_ctx->tge_sync_id;
				}
			} else if (_vdocap_is_pdaf_mode(p_ctx)) {
				open_cfg.dupl_src_id = _vdocap_get_main_pdaf_id(p_ctx);
				open_cfg.sen_id = open_cfg.dupl_src_id;
			} else {
				//should not be here!
				DBG_ERR("cfg error!\r\n");
			}
		}
		open_cfg.clk_src_sel = CTL_SIE_CLKSRC_DEFAULT;
		DBG_MSG("open_cfg:id=%d,flow_type=%d,dupl_src_id=%d,sen_id=%d,isp_id=%X,clk_src_sel=%d\r\n", open_cfg.id, open_cfg.flow_type, open_cfg.dupl_src_id, open_cfg.sen_id, open_cfg.isp_id, open_cfg.clk_src_sel);
		p_ctx->dev_trigger_open = 1;
		p_ctx->sie_hdl = ctl_sie_open((void *)&open_cfg);
		if (p_ctx->sie_hdl == 0) {
			DBG_ERR("ctl_sie_open open fail\r\n");
			return ISF_ERR_NOT_AVAIL;
		}
		p_ctx->dev_ready = 1; //after open
		p_ctx->dev_trigger_close = 0;

		if (p_ctx->flow_type == CTL_SIE_FLOW_SEN_IN) {
			if (FALSE == ctl_sen_is_opened) {
				if(E_OK != _vdocap_sensor_ctl(p_sen_ctl_obj, p_ctx, TRUE)) {
					return ISF_ERR_INVALID_VALUE;
				}
			}
			#if defined(CONFIG_NVT_FPGA_EMULATION) || defined(_NVT_FPGA_)
			// [FPGA CASE] sensor used extern mclk, SIE_IO_PXCLKSRC (pad) cannot use, need to enable mclk for SIE_PXCLKSRC
			sie_mclk.mclk_en = ENABLE;
			ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_MCLK_IMM, (void *)&sie_mclk);
			ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_LOAD, NULL);
			#endif
		} else if (p_ctx->flow_type == CTL_SIE_FLOW_PATGEN) {
			sie_mclk.mclk_en = ENABLE;
			ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_MCLK_IMM, (void *)&sie_mclk);
			ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_LOAD, NULL);
		}
		cb_info.cbevt = CTL_SIE_CBEVT_ENG_SIE_ISR;
		cb_info.fp = p_ctx->sie_isr_cb;
		cb_info.sts = CTL_SIE_INTE_DRAM_OUT0_END | CTL_SIE_INTE_VD;
		ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_REG_CB_IMM, (void *)&cb_info);

		cb_info.cbevt = CTL_SIE_CBEVT_BUFIO;
		cb_info.fp = p_ctx->buf_io_cb;
		cb_info.sts = CTL_SIE_BUF_IO_ALL;
		ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_REG_CB_IMM, (void *)&cb_info);

		_vdocap_oport_initqueue(p_thisunit);
		_isf_vdocap_oqueue_do_open(p_thisunit, oport);
		return ISF_OK;
	} else if (_vdocap_is_mclk_sync_mode(p_ctx) && p_ctx->flow_type == CTL_SIE_FLOW_SIG_DUPL) {
		//the last sub shdr sie has been opened
		return ISF_OK;
	} else {
		return ISF_ERR_ALREADY_OPEN;
	}

}

static ISF_RV _isf_vdocap_check_statelimit(struct _ISF_UNIT *p_thisunit, UINT32 oport, UINT32 out_en)
{
	VDOCAP_CONTEXT* p_ctx = (VDOCAP_CONTEXT*)(p_thisunit->refdata);
	ISF_STATE *p_this_state = p_thisunit->port_outstate[oport - ISF_OUT_BASE];

	//only vcap0 support direct mode
	if (p_ctx->id) {
		return ISF_OK;
	}

	if (p_this_state->dirty & ISF_PORT_CMD_RESET) { //if under reset?
		return ISF_OK; //always pass
	}

	if ((_vdocap_is_direct_flow(p_ctx)) && (out_en == 0)) {
		ISF_UNIT *p_destunit = p_ctx->p_destunit;
		if (!p_destunit) {
			DBG_ERR("[direct] vcap cannot stop after unbind to vprc!\r\n");
			return ISF_ERR_NOT_CONNECT_YET;
		} else {
			ISF_RV r = p_destunit->do_update(p_destunit, ISF_CTRL, ISF_PORT_CMD_QUERY); //query dest is start?
			if(r == ISF_OK) { // vprc is still start
				DBG_ERR("[direct] vcap cannot stop before vprc stop!\r\n");
				return ISF_ERR_STOP_FAIL;
			}
		}
		DBG_DUMP("vcap: disable direct func at out[%d]\r\n", 0);
	}
	if ((_vdocap_is_direct_flow(p_ctx)) && (out_en == 1)) {
		ISF_UNIT *p_destunit = p_ctx->p_destunit;
		if (!p_destunit) {
			DBG_ERR("[direct] vcap cannot start before bind to vprc!\r\n");
			return ISF_ERR_NOT_CONNECT_YET;
		} else {
			ISF_RV r = p_destunit->do_update(p_destunit, ISF_CTRL, ISF_PORT_CMD_QUERY); //query dest is start?
			if(r != ISF_OK) { // vprc is not start yet
				DBG_ERR("[direct] vcap cannot start before vprc start!\r\n");
				return ISF_ERR_START_FAIL;
			}
		}
		DBG_DUMP("vcap: enable direct func at out[%d]\r\n", 0);
	}
	return ISF_OK;
}


int _isf_vdocap_do_start(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	CTL_SIE_TRIG_INFO trig_info = {0};
	ISF_INFO *p_src_info = p_thisunit->port_outinfo[oport - ISF_OUT_BASE];
	ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);
	INT32 ret = CTL_SIE_E_OK;
	UINT32 shdr_set;
	UINT32 shdr_seq;
	UINT32 shdr_frm_num;

	ret = _isf_vdocap_check_statelimit(p_thisunit, oport, 1);
	if (ret != ISF_OK)
		return ret;
	p_ctx->outq.output_cur_en = p_ctx->outq.output_en;

	_vdocap_oport_set_enable(p_thisunit, ISF_OUT(0) + oport, p_thisunit->port_out[oport], 1);

	//start input frame rate
	if (p_vdoinfo->framepersecond == VDO_FRC_DIRECT) {
		p_vdoinfo->framepersecond = VDO_FRC_ALL;
	}
	DBG_UNIT("VDOCAP[%d] frc(%d,%d)\r\n", p_ctx->id, GET_HI_UINT16(p_vdoinfo->framepersecond), GET_LO_UINT16(p_vdoinfo->framepersecond));
	_isf_frc_start(p_thisunit, ISF_OUT(0), &(p_ctx->outfrc[0]), p_vdoinfo->framepersecond);

	if (_vdocap_is_direct_flow(p_ctx) && p_ctx->one_buf) {
		DBG_WRN("[%d]direct mode don't need one_buf!\r\n", p_ctx->id);
		p_ctx->one_buf = FALSE;
	}

	if (p_ctx->flow_type != CTL_SIE_FLOW_PATGEN) {
		if (_vdocap_config_sensor_mode(p_thisunit, oport)) {
			return ISF_ERR_INVALID_VALUE;
		}
	}
	//check
	if (_vdocap_is_direct_flow(p_ctx) && _vdocap_is_shdr_mode(p_ctx->shdr_map)) {
		if (p_ctx->id != 0 && p_ctx->id != 1) {
			DBG_ERR("SHDR+Direct only support VCAP0/1.\r\n");
			return ISF_ERR_INVALID_VALUE;
		}
	}
	#if defined(CONFIG_NVT_FPGA_EMULATION) || defined(_NVT_FPGA_)
	if (_vdocap_is_shdr_mode(p_ctx->shdr_map) || _vdocap_is_direct_flow(p_ctx)) {
		ISF_INFO *p_src_info = p_thisunit->port_outinfo[oport - ISF_OUT_BASE];
		ISF_VDO_INFO *p_vdoinfo = (ISF_VDO_INFO *)(p_src_info);

		p_vdoinfo->window.w = 960;
		p_vdoinfo->window.h = 540;
		if (p_vdoinfo->imgfmt != VDO_PXLFMT_NRX12_SHDR2) {
			p_vdoinfo->imgfmt = VDO_PXLFMT_RAW8_SHDR2;
		}
		DBG_WRN("Force to crop(%dx%d) and use RAW8/NRX12(0x%X) in SHDR mode or direct mode for FPGA.\r\n", p_vdoinfo->window.w, p_vdoinfo->window.h, p_vdoinfo->imgfmt);
	}
	#endif
	_vdocap_config_alg_func(p_thisunit);
	//this should be prior to _vdocap_config_raw_compress
	_vdocap_config_format(p_thisunit, oport);
	_vdocap_config_out_size(p_thisunit, oport);
	_vdocap_config_direct(p_thisunit, oport);
	_vdocap_config_out_dest(p_thisunit);
#if defined(_BSP_NA51089_)
	if (p_ctx->raw_compress && p_ctx->sie_id == 0 && !_vdocap_is_direct_flow(p_ctx)) {
		if (_vdocap_is_shdr_mode(p_ctx->shdr_map)) {
			p_ctx->raw_compress = FALSE;
		} else {
			DBG_ERR("SIE1 NOT support RAW compression!\r\n");
			return ISF_ERR_INVALID_VALUE;
		}
	}
#endif
	_vdocap_config_raw_compress(p_thisunit);
	_vdocap_config_bp3_ratio(p_thisunit);
	_vdocap_config_gyro(p_thisunit);

	if (p_ctx->pullq.num[oport - ISF_OUT_BASE]) {
		if (p_ctx->one_buf) {
			DBG_ERR("VDOCAP[%d] not support pull under one buffer!\r\n", p_ctx->id);
			return ISF_ERR_INVALID_VALUE;
		}
		_isf_vdocap_oqueue_do_start(p_thisunit, oport); /* open queue before start */
	}

	//special starting-sequence flow for SHDR direct mode
	if (_vdocap_is_direct_flow(p_ctx) && _vdocap_is_shdr_mode(p_ctx->shdr_map)) {
		ret = _vdocap_start_shdr_direct_sie(p_ctx->shdr_map);
		return ret;
	}
	if (_vdocap_is_sw_vd_sync_mode(p_ctx)) {
		CTL_SIE_SYNC_INFO sync_info = {0};

		if (p_ctx->id == _vdocap_get_main_sw_vd_sync_id(p_ctx)) {
			sync_info.mode = 1;
		} else {
			sync_info.mode = 2;
		}
		sync_info.sync_id = _vdocap_get_main_sw_vd_sync_id(p_ctx);
		sync_info.det_frm_int = 500;
		sync_info.adj_thres = 100;
		sync_info.adj_auto = 1;
		_vdocap_dump_sw_vd_sync_info(p_thisunit, &sync_info);
		ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_SW_SYNC, (void *)&sync_info);
		ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_LOAD, NULL);
	}
	if (_vdocap_is_tge_sync_mode(p_ctx)) {
		DBG_UNIT("VDOCAP[%d] tge_sync_id 0x%X\r\n", p_ctx->id, p_ctx->tge_sync_id);
		//all SIEs should start after all tge-sync sensors change mode
		if (_vdocap_is_last_sync_id(p_ctx, p_ctx->tge_sync_id)) {
			//start all SIE
			ret = _vdocap_start_all_sync_sie(p_ctx->tge_sync_id);
		}
	} else {
		trig_info.trig_type = CTL_SIE_TRIG_START;
		trig_info.trig_frame_num = 0xffffffff; /*continue mode*/
		trig_info.b_wait_end = 0;
		p_ctx->started = 1;
		DBG_UNIT("VDOCAP[%d] port[%d] start ++\r\n", p_ctx->id, oport);
		ret = ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_TRIG_IMM, (void *)&trig_info);
		DBG_UNIT("VDOCAP[%d] port[%d] start --\r\n", p_ctx->id, oport);
		////reset frame count for shdr flow
		shdr_set = _vdocap_shdr_map_to_set(p_ctx->shdr_map);
		shdr_seq = _vdocap_shdr_map_to_seq(p_ctx->shdr_map);
		shdr_frm_num = _vdocap_shdr_frm_num[shdr_set];
		//only need to reset once after the last shdr path triggered
		if (_vdocap_is_shdr_mode(p_ctx->shdr_map) && (shdr_seq == (shdr_frm_num - 1)) && ret == CTL_SIE_E_OK) {
			DBG_UNIT("VDOCAP[%d] reset frame cnt(%d) ++\r\n", p_ctx->id, ret);
				ret = ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_RESET_FC_IMM, NULL);
			DBG_UNIT("VDOCAP[%d] reset frame cnt(%d) --\r\n", p_ctx->id, ret);
		}
	}
	if (ret != CTL_SIE_E_OK) {
		DBG_ERR("failed(%d)\r\n", ret);
		//started: 0->1->0
		//p_ctx->started = 0;
		if (ret == CTL_SIE_E_NOMEM) {
			DBG_ERR("RAW buf insufficient(0x%X)\r\n", p_ctx->out_buf_size[oport - ISF_OUT_BASE]);
			return ISF_OK;
		} else {
			return ISF_ERR_PROCESS_FAIL;//map to HD_ERR_FAIL;
		}
	} else {
		return ISF_OK;
	}
}
int _isf_vdocap_do_stop(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	CTL_SIE_TRIG_INFO trig_info = {0};
	INT32 ret;
	UINT32 i = oport - ISF_OUT_BASE;

	ret = _isf_vdocap_check_statelimit(p_thisunit, oport, 0);
	if (ret != ISF_OK)
		return ret;
	p_ctx->outq.output_cur_en = p_ctx->outq.output_en;

	//stop input frame rate
	_isf_frc_stop(p_thisunit, ISF_OUT(0), &(p_ctx->outfrc[0]));
	_vdocap_oport_set_enable(p_thisunit, ISF_OUT(0) + oport, p_thisunit->port_out[oport], 0);

	trig_info.trig_type = CTL_SIE_TRIG_STOP;
	trig_info.b_wait_end = 1;
	DBG_UNIT("VDOCAP[%d] port[%d] stop ++\r\n", p_ctx->id, oport);
	ret = ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_TRIG_IMM, (void *)&trig_info);
	DBG_UNIT("VDOCAP[%d] port[%d] stop --\r\n", p_ctx->id, oport);

	if (p_ctx->outq.force_onebuffer[i]!= 0 && FALSE == _vdocap_is_shdr_mode(p_ctx->shdr_map)) {
		UINT32 j = p_ctx->outq.force_onej[i];
		ISF_DATA* p_data = &(p_ctx->outq.output_data[i][j]);
		DBG_UNIT("%s.out[%d]! Stop single blk mode => blk_id=%08x\r\n", p_thisunit->unit_name, oport, p_ctx->outq.force_onebuffer[i]);
		p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_NEW); //remove 1 reference to this "single buffer"
		//p_data->h_data = p_ctx->outq.force_onebuffer[i];
		p_ctx->outq.force_onebuffer[i] = 0;
		p_ctx->outq.force_onesize[i] = 0;
		p_ctx->outq.output_max[i] = VDOCAP_OUT_DEPTH_MAX;
		p_ctx->outq.force_onej[i] = 0;
	}
	/* stop queue after start */
	if (p_ctx->pullq.num[oport - ISF_OUT_BASE]) {
		_isf_vdocap_oqueue_do_stop(p_thisunit, oport);
	}

	if (_vdocap_is_shdr_mode(p_ctx->shdr_map)) {
		VDOCAP_SHDR_OUT_QUEUE *p_outq;
		UINT32 shdr_set = _vdocap_shdr_map_to_set(p_ctx->shdr_map);
		UINT32 shdr_seq = _vdocap_shdr_map_to_seq(p_ctx->shdr_map);

		if (_vdocap_is_last_opened_shdr(p_ctx->id, shdr_set)) {
			ISF_UNIT *p_main_unit = _vdocap_shdr_main_unit[shdr_set];

			p_outq = &_vdocap_shdr_queue[shdr_set];
			if (p_outq->force_onebuffer[shdr_seq] != 0) {
				UINT32 j = p_outq->force_onej;
				UINT32 k, shdr_frm_num = _vdocap_shdr_frm_num[shdr_set];
				ISF_DATA *p_data = NULL;

				for (k = 0; k < shdr_frm_num; k++) {
					p_data = &p_outq->output_data[j][k];
					DBG_UNIT("%s Stop single blk mode =>j=%d blk_id[%d]=%08x\r\n", p_thisunit->unit_name, j, k, p_outq->force_onebuffer[k]);
					p_thisunit->p_base->do_release(p_thisunit, oport, p_data, ISF_OUT_PROBE_PUSH);
					p_outq->force_onebuffer[k] = 0;
					p_outq->force_onesize[k] = 0;

				}
				p_outq->force_onej = 0;
			}

			if (p_main_unit) {
				_vdocap_shdr_oport_close_check(p_main_unit, oport, p_outq);
			} else {
				DBG_ERR("check shdr fail:map=%d", (UINT32)p_ctx->shdr_map);
				return ISF_ERR_FAILED;
			}
		}
	}
	if (ret != CTL_SIE_E_OK) {
		DBG_ERR("failed(%d)\r\n", ret);
		if (ret == CTL_SIE_E_NOMEM) {
			return ISF_ERR_NEW_FAIL;
		} else {
			return ISF_ERR_PROCESS_FAIL;//map to HD_ERR_FAIL;
		}
	} else {
		//started: 1->0
		p_ctx->started = 0;
		return 0;
	}
}
static void _isf_vdocap_do_runtime(ISF_UNIT *p_thisunit, UINT32 cmd, UINT32 oport, UINT32 en)
{
#if 1
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	DBG_UNIT("VDOCAP[%d] cmd=0x%X port[%d]=%d\r\n", p_ctx->id, cmd, oport, en);

	if (cmd == ISF_PORT_CMD_RUNTIME_UPDATE) {
		UINT32 i, check_dirty_flg = (ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW | ISF_INFO_DIRTY_DIRECT);

		DBG_FUNC("%s.out[%d]=%08x rt-update\r\n", p_thisunit->unit_name, oport - ISF_OUT_BASE, en);
		{
			p_ctx->outq.output_cur_en = p_ctx->outq.output_en;
			//set output image size, fmt
			i = oport - ISF_OUT_BASE;

			if (i < ISF_VDOCAP_OUT_NUM) {
				ISF_INFO *p_src_info = p_thisunit->port_outinfo[0];
				ISF_VDO_INFO *p_imginfo = (ISF_VDO_INFO *)(p_src_info);

				DBG_MSG("dirty=0x%X\r\n", p_thisunit->port_outstate[i]->dirty);
				if ((p_thisunit->port_outstate[i]->dirty & (ISF_PORT_CMD_OUTPUT | ISF_PORT_CMD_STATE))
					|| (p_imginfo->dirty & check_dirty_flg)) {
					BOOL do_en = ((p_thisunit->port_outstate[i]->state) == (ISF_PORT_STATE_RUN)); //port current state

					if (en != 0xffffffff)
						do_en = en;

					_vdocap_config_format(p_thisunit, oport);
					_vdocap_config_out_size(p_thisunit, i);
					_vdocap_config_direct(p_thisunit, i);
					_vdocap_config_raw_compress(p_thisunit);

					_vdocap_oport_set_enable(p_thisunit, ISF_OUT(0) + i, p_thisunit->port_out[i], do_en);
					if (p_imginfo->framepersecond == VDO_FRC_DIRECT) {
						p_imginfo->framepersecond = VDO_FRC_ALL;
					}
					//update input frame rate
					if (p_ctx->outfrc[0].sample_mode == ISF_SAMPLE_OFF) {
						_isf_frc_start(p_thisunit, ISF_OUT(0), &(p_ctx->outfrc[0]), p_imginfo->framepersecond);
					} else {
						_isf_frc_update_imm(p_thisunit, ISF_OUT(0), &(p_ctx->outfrc[0]), p_imginfo->framepersecond);
					}
					if (p_thisunit->port_out[i]) {
						p_imginfo->dirty &= ~check_dirty_flg;    //clear dirty
					}
				}
				#if 0//has been set in _vdocap_oport_set_enable(), need clear ?
				if (p_imginfo->dirty & ISF_INFO_DIRTY_BUFCOUNT) {

					p_imginfo->dirty &= ~ISF_INFO_DIRTY_BUFCOUNT;
				}
				#endif

			}


		}
	}


	//ben to do, update others
	#if 0
	//need to check buffer count
	if (p_vdoinfo->dirty & ISF_INFO_DIRTY_BUFCOUNT) {
		_vdocap_iport_setqueuecount(p_thisunit, p_vdoinfo->buffercount);
		p_vdoinfo->dirty &= ~ISF_INFO_DIRTY_BUFCOUNT;
	}
	#endif

	//ben to do, check this
	ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_LOAD, NULL);

#endif
}


ISF_RV _isf_vdocap_do_setportparam(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 value)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	if (p_ctx == NULL) {
		DBG_UNIT("VDOCAP buf not init!\r\n");
		return ISF_ERR_INIT; //sitll not init
	}
	DBG_UNIT("VDOCAP[%d] port[%d] param[%08x] = %d\r\n", p_ctx->id, nport-ISF_IN_BASE, param, value);
	if ((nport < (int)ISF_IN_BASE) && ((nport-ISF_OUT_BASE) < p_thisunit->port_out_count)) {
		/* for output port */
		switch (param) {
		case VDOCAP_PARAM_OUT_DEPTH: {
			UINT32 outport = nport - ISF_OUT_BASE;

			if (outport < ISF_VDOCAP_OUT_NUM) {
				if (value > ISF_VDOCAP_OUTQ_MAX) {
					p_ctx->pullq.num[outport] = ISF_VDOCAP_OUTQ_MAX;
					DBG_WRN("max pullq num is %d!\r\n", ISF_VDOCAP_OUTQ_MAX);
				} else {
					p_ctx->pullq.num[outport] = value;
				}
				return ISF_OK;
			} else {
				DBG_ERR("nport = %d, ISF_OUT_BASE=%d \r\n", nport, ISF_OUT_BASE);
				return ISF_ERR_FAILED;
			}

		}
		case ISF_PARAM_PORT_DIRTY:
#if 0
			if (value & (ISF_INFO_DIRTY_IMGSIZE | ISF_INFO_DIRTY_FRAMERATE | ISF_INFO_DIRTY_WINDOW | ISF_INFO_DIRTY_DIRECT)) {
				return ISF_APPLY_DOWNSTREAM_READYTIME;
			} else {
				return ISF_OK;
			}
#else
			return ISF_OK;
#endif
#if 0
		case VDOCAP_PARAM_CODEC:
			p_ctx->codec[nport-ISF_OUT_BASE] = value;
			return ISF_OK;
#endif
		default:
			DBG_ERR("%s.out[%d] set param[%08x] = %d\r\n", p_thisunit->unit_name, nport-ISF_OUT_BASE, param, value);
			break;
		}
	} else if (nport-ISF_IN_BASE < p_thisunit->port_in_count) {
		/* for input port */
		DBG_ERR("%s.in[%d] set param[%08x] = %d\r\n", p_thisunit->unit_name, nport-ISF_IN_BASE, param, value);
	} else if (nport == ISF_CTRL) {
		/* for common port */
		switch (param) {
		case VDOCAP_PARAM_FLOW_TYPE:
			//DBG_FUNC("%s set mode=%08x\r\n", p_thisunit->unit_name, value);

			if (value == VDOCAP_FLOW_SEN_IN) {
				p_ctx->flow_type = CTL_SIE_FLOW_SEN_IN;
			} else if (value == VDOCAP_FLOW_PATGEN) {
				p_ctx->flow_type = CTL_SIE_FLOW_PATGEN;
			} else {
				p_ctx->flow_type = CTL_SIE_FLOW_UNKNOWN;
				DBG_ERR("%s.unkonwn flow type = %d\r\n", p_thisunit->unit_name, value);
			}
			return ISF_OK;
		case VDOCAP_PARAM_SHDR_MAP:
			p_ctx->shdr_map = value;
			_vdocap_shdr_map_outdest(p_ctx->id, p_ctx->shdr_map);
			if (_vdocap_is_shdr_main_path(value)) {
				p_ctx->flow_type = CTL_SIE_FLOW_SEN_IN;
			} else {
				p_ctx->flow_type = CTL_SIE_FLOW_SIG_DUPL;
			}
			if (p_ctx->shdr_map == VDOCAP_SEN_HDR_SET1_MAIN) {
				_vdocap_shdr_main_unit[VDOCAP_SHDR_SET1] = p_thisunit;
			} else if (p_ctx->shdr_map == VDOCAP_SEN_HDR_SET2_MAIN) {
				_vdocap_shdr_main_unit[VDOCAP_SHDR_SET2] = p_thisunit;
			}
			return ISF_OK;
		case VDOCAP_PARAM_AD_MAP:
			p_ctx->ad_map = value;
			#if 0 //alwaye use CTL_SIE_FLOW_SEN_IN for all mux ccir ctl_sie
			//ccir mux and shdr should be exclusive
			if (_vdocap_is_mux_mode(p_ctx) && FALSE == _vdocap_is_shdr_mode(p_ctx->shdr_map)) {
				if (_vdocap_get_main_mux_id(p_ctx) == p_ctx->id) {
					//main path
					p_ctx->flow_type = CTL_SIE_FLOW_SEN_IN;
				} else {
					p_ctx->flow_type = CTL_SIE_FLOW_SIG_DUPL;
				}
			}
			#endif
			DBG_MSG("ad_map = 0x%08X\r\n", p_ctx->ad_map);
			return ISF_OK;
		case VDOCAP_PARAM_AD_TYPE:
			p_ctx->ad_type = value;
			DBG_MSG("ad_type = 0x%08X\r\n", p_ctx->ad_type);
			return ISF_OK;
#if defined(_BSP_NA51089_)
		case VDOCAP_PARAM_DMA_ABORT: {
			CTL_SEN_STATUS sen_sts = CTL_SEN_STATUS_DMA_ABORT;
			PCTL_SEN_OBJ p_sen_ctl_obj = ctl_sen_get_object(p_ctx->id);
			UINT64 t1 = 0, t2 = 0, t3 = 0;

			t1 = hwclock_get_longcounter();
			ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_DMA_ABORT, NULL);
			t2 = hwclock_get_longcounter();
			p_sen_ctl_obj->set_cfg(CTL_SEN_CFGID_SET_STATUS, (void *)&sen_sts);
			t3 = hwclock_get_longcounter();
			DBG_DUMP("ABORT: ctl sie[%lld]us, ctl sen[%lld]us\r\n", t2-t1, t3-t2);
			return ISF_OK;
		}
#endif
		case VDOCAP_PARAM_ALG_FUNC:
			p_ctx->alg_func = value;
			return ISF_OK;
		case VDOCAP_PARAM_OUT_DIR:
			p_ctx->flip = _vdocap_direction_mapping(value);
			DBG_MSG("set DIR from 0x%X to 0x%X\r\n", value, p_ctx->flip);
			ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_FLIP, (void *)&p_ctx->flip);
			ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_LOAD, NULL);
			return ISF_OK;
		case VDOCAP_PARAM_OUT_DEST:
			if (nvtmpp_is_dynamic_map() && value == VDOCAP_OUT_DEST_DIRECT) {
				DBG_ERR("Dynamic map memory not support VCAP-VPRC Direct\r\n");
				return ISF_ERR_NOT_SUPPORT;
			}
			if (value < VDOCAP_OUT_DEST_MAX) {
				p_ctx->out_dest = value;
			}
			return ISF_OK;
		case VDOCAP_PARAM_ONE_BUFF:
			p_ctx->one_buf = value;
			return ISF_OK;
		case VDOCAP_PARAM_BUILTIN_HDR:
			p_ctx->builtin_hdr = (BOOL)value;
			return ISF_OK;
		case VDOCAP_PARAM_DATA_LANE: {
			if ((CTL_SEN_DATALANE)value > CTL_SEN_DATALANE_8) {
				p_ctx->data_lane = CTL_SEN_DATALANE_8;
				DBG_WRN("max data_lane is %d!\r\n", CTL_SEN_DATALANE_8);
			} else {
				p_ctx->data_lane = (CTL_SEN_DATALANE)value;
			}
			return ISF_OK;
		}
		case VDOCAP_PARAM_RESET_FC: {
			INT32 ret;

			DBG_MSG("RESET_FC 0x%X\r\n", value);
			ret = ctl_sie_set(p_ctx->sie_hdl, CTL_SIE_ITEM_RESET_FC_IMM, (void *)(&value));
			if (ret) {
				DBG_ERR("RESET_FC_IMM[0x%X] failed(%d)!\r\n", value, ret);
				return ISF_ERR_NOT_SUPPORT;
			} else {
				return ISF_OK;
			}
		}
		case VDOCAP_PARAM_ENC_RATE:
			if (FALSE == _vdocap_check_enc_rate(value)) {
				DBG_ERR("not support enc_rate(%d)\r\n", value);
				return ISF_ERR_FAILED;
			}
			p_ctx->enc_rate= value;
			return ISF_OK;
		case VDOCAP_PARAM_BP3_RATIO: {
			p_ctx->bp3_ratio = value;
			return ISF_OK;
		}
		case VDOCAP_PARAM_QUEUE_FLUSH_SCHEME:
			p_ctx->queue_scheme = value;
			DBG_UNIT("FLUSH_SCHEME=%d\r\n", p_ctx->queue_scheme);
			return ISF_OK;
		case VDOCAP_PARAM_MCLK_SRC_SYNC_SET:
			p_ctx->mclksrc_sync = value;
			return ISF_OK;
		case VDOCAP_PARAM_SET_FPS: {
			PCTL_SEN_OBJ p_sen_obj = ctl_sen_get_object(p_ctx->id);
			UINT32 fps = 100*GET_HI_UINT16(value)/GET_LO_UINT16(value);
			INT32 ret;

			if ((p_sen_obj == NULL) || (p_sen_obj->set_cfg == NULL)) {
				DBG_ERR("set sen_obj[%d] failed!\r\n", p_ctx->id);
				return ISF_ERR_FAILED;
			}
			if (p_sen_obj->is_opened() && p_ctx->started) {
				if (fps > p_ctx->chgsenmode_info.auto_info.frame_rate) {
					DBG_WRN("could not greater than HD_VIDEOCAP_IN.frc(%d)\r\n", p_ctx->chgsenmode_info.auto_info.frame_rate);
					fps = p_ctx->chgsenmode_info.auto_info.frame_rate;
				}
				ret = p_sen_obj->set_cfg(CTL_SEN_CFGID_SET_FPS, (void *)(&fps));
	    		if (ret) {
	    			DBG_ERR("set sen[%d] fps failed!(%d)\r\n", p_ctx->id, ret);
	    			return ISF_ERR_FAILED;
    			}

			} else {
				DBG_ERR("Only for VCAP[%d] is started!\r\n", p_ctx->id);
				return ISF_ERR_FAILED;
			}
			return ISF_OK;
		}
		case VDOCAP_PARAM_PDAF_MAP:
			p_ctx->pdaf_map = value;
			return ISF_OK;
		case VDOCAP_PARAM_SW_VD_SYNC:
			p_ctx->sw_vd_sync = value;
			return ISF_OK;
		case VDOCAP_PARAM_SIE_MAP:
			if (value >= VDOCAP_MAX_NUM) {
				DBG_ERR("SIE_MAP(%d) should < %d\r\n", value, VDOCAP_MAX_NUM);
				return ISF_ERR_FAILED;
			}
			p_ctx->sie_id = value;
			return ISF_OK;
		case VDOCAP_PARAM_CHK_SHDR_MAP: {
			UINT32 sensor_id = GET_HI_UINT16(value);
			UINT32 shdr_map = GET_LO_UINT16(value);

			DBG_MSG("shdr map = 0x%08X\r\n", value);
			if (nvt_get_chip_id() == CHIP_NA51084) {
				if (sensor_id == 0) {
					shdr_map &=~ NA51084_CSI0_SPT_SIE_MAP;
					if (shdr_map) {
						DBG_ERR("SHDR1 only support VIDEOCAP0/1/3/4.\r\n");
						return ISF_ERR_INVALID_VALUE;
					}
				} else {//SENSOR2
					shdr_map &=~ NA51084_CSI1_SPT_SIE_MAP;
					if (shdr_map) {
						DBG_ERR("SHDR2 only support VIDEOCAP1/3/4.\r\n");
						return ISF_ERR_INVALID_VALUE;
					}
				}
			} else {
				if (sensor_id == 0) {
					shdr_map &=~ NA51055_CSI0_SPT_SIE_MAP;
					if (shdr_map) {
						DBG_ERR("SHDR1 only support VIDEOCAP0/1.\r\n");
						return ISF_ERR_INVALID_VALUE;
					}
				} else {
					DBG_ERR("Only SENSOR1 support SHDR.\r\n");
					return ISF_ERR_INVALID_VALUE;
				}
			}
			return ISF_OK;
		}
		case VDOCAP_PARAM_CHK_PIXEL_FMT: {

			return ISF_OK;
		}
		case VDOCAP_PARAM_MODE_TYPE:
			p_ctx->mode_type = (UINT32)value ;
			return ISF_OK;
		default:
			DBG_ERR("%s.ctrl set param[%08x] = %d\r\n", p_thisunit->unit_name, param, value);
			break;
		}
		return ISF_ERR_NOT_SUPPORT;
	}
	return ISF_ERR_NOT_SUPPORT;
}
UINT32 _isf_vdocap_do_getportparam(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	if (p_ctx == NULL) {
		DBG_UNIT("VDOCAP buf not init!\r\n");
		return ISF_ERR_INIT; //sitll not init
	}
	DBG_UNIT("[%d] port[%d] param[%08x]\r\n", p_ctx->id, nport-ISF_IN_BASE, param);
	if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
		switch (param) {
		case VDOCAP_PARAM_DIRECT_CB:
			if (_vdocap_is_direct_flow((VDOCAP_CONTEXT *)(p_thisunit->refdata))) {
				DBG_UNIT("get _isf_vdocap_direct_unlock_cb\r\n");
				return (UINT32)_isf_vdocap_direct_unlock_cb;
			} else {
				return 0;
			}
			break;
		case VDOCAP_PARAM_USER3_CB:
			if (_vdocap_is_direct_flow((VDOCAP_CONTEXT *)(p_thisunit->refdata))) {
				DBG_UNIT("get _isf_vdocap_user_define3_cb\r\n");
				return (UINT32)_isf_vdocap_user_define3_cb;
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
		default:
			DBG_ERR("%s.in[%d] get param[%08x]\r\n", p_thisunit->unit_name, nport-ISF_IN_BASE, param);
			break;
		}
	} else if (nport == ISF_CTRL) {

		switch (param) {
		case VDOCAP_PARAM_GET_PLUG: {
			PCTL_SEN_OBJ p_sen_obj;
			BOOL det_rst = FALSE;
			INT ret;

			p_sen_obj = ctl_sen_get_object(p_ctx->id);
			if ((p_sen_obj == NULL) || (p_sen_obj->get_cfg == NULL)) {
				DBG_ERR("get sen_obj[%d] failed!\r\n", p_ctx->id);
				break;
			}
			if (p_sen_obj->is_opened()) {
				ret = p_sen_obj->get_cfg(CTL_SEN_CFGID_GET_PLUG, (void *)(&det_rst));
				if (ret) {
					DBG_ERR("get sen_obj[%d] GET_PLUG failed!(%d)\r\n", p_ctx->id, ret);
				}
			}
			return det_rst;
		}
			break;
		case VDOCAP_PARAM_CSI_ERR_CNT: {
			PCTL_SEN_OBJ p_sen_obj;
			UINT32 err_cnt = 0;
			INT ret;

			p_sen_obj = ctl_sen_get_object(p_ctx->id);
			if ((p_sen_obj == NULL) || (p_sen_obj->get_cfg == NULL)) {
				DBG_ERR("get sen_obj[%d] failed!\r\n", p_ctx->id);
				break;
			}
			if (p_sen_obj->is_opened()) {
				ret = p_sen_obj->get_cfg(CTL_SEN_CFGID_GET_SSENIF_ERR_CNT, (void *)(&err_cnt));
				if (ret) {
					DBG_ERR("get sen_obj[%d] GET_PLUG failed!(%d)\r\n", p_ctx->id, ret);
				}
			}
			return err_cnt;
		}
			break;
		default:
			DBG_ERR("%s.ctrl get param[%08x]\r\n", p_thisunit->unit_name, param);
			break;
		}
		return 0;
	}


	return ISF_ERR_NOT_SUPPORT;
}
ISF_RV _isf_vdocap_do_setportstruct(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 *p_struct, UINT32 size)
{
	ISF_RV ret = ISF_OK;
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	if (p_ctx == NULL) {
		DBG_UNIT("VDOCAP buf not init!\r\n");
		return ISF_ERR_INIT; //sitll not init
	}
	DBG_UNIT("VDOCAP[%d] port[%d] param = 0x%X size = %d\r\n", p_ctx->id, nport-ISF_IN_BASE, param, size);
	if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
	/* for output port */
	} else if (nport-ISF_IN_BASE < p_thisunit->port_in_count) {
	/* for input port */
	} else if (nport == ISF_CTRL) {
		/* for common port */
		switch (param) {
		case VDOCAP_PARAM_SEN_INIT_CFG:	{
			VDOCAP_SEN_INIT_CFG *p_sen_init = (VDOCAP_SEN_INIT_CFG *)p_struct;
			PCTL_SEN_OBJ p_sen_obj;
			CTL_SEN_INIT_CFG_OBJ cfg_obj;
			CTL_SEN_PINMUX pinmux[3] = {0};

			#if defined(CONFIG_NVT_FPGA_EMULATION) || defined(_NVT_FPGA_)
			DBG_WRN("Force to use I2C_1 for FPGA.\r\n");
			//p_sen_init->sen_init_cfg.pin_cfg.pinmux.serial_if_pinmux = 0x301;
			p_sen_init->sen_init_cfg.pin_cfg.pinmux.cmd_if_pinmux = 0x1;
			//cfg_obj.pin_cfg.sen_2_serial_pin_map[2] = 0xFFFFFFFF;
			//cfg_obj.pin_cfg.sen_2_serial_pin_map[3] = 0xFFFFFFFF;
			#endif
			_vdocap_dump_sen_init(p_thisunit, p_sen_init);

			memset((void *)(&cfg_obj), 0, sizeof(CTL_SEN_INIT_CFG_OBJ));
			p_sen_obj = ctl_sen_get_object(p_ctx->id);
			if ((p_sen_obj == NULL) || (p_sen_obj->init_cfg == NULL)) {
				DBG_ERR("get sen_obj[%d] failed!\r\n", p_ctx->id);
				ret = ISF_ERR_NOT_AVAIL;
				break;
			}
			if (FALSE == p_sen_obj->is_opened()) {
				CTL_SEN_IF_TYPE if_type;

				if (_vdocap_is_mux_mode(p_ctx)) {
					if (_vdocap_get_main_mux_id(p_ctx) == 0) {
						pinmux[0].func = PIN_FUNC_SENSOR;
					} else {
						pinmux[0].func = PIN_FUNC_SENSOR2;
					}
				} else {
					if (p_ctx->id == 0) {
						pinmux[0].func = PIN_FUNC_SENSOR;
					} else {
						pinmux[0].func = PIN_FUNC_SENSOR2;
					}
				}
				pinmux[0].cfg = p_sen_init->sen_init_cfg.pin_cfg.pinmux.sensor_pinmux;
				pinmux[0].pnext = &pinmux[1];

				pinmux[1].func = PIN_FUNC_MIPI_LVDS;
				pinmux[1].cfg = p_sen_init->sen_init_cfg.pin_cfg.pinmux.serial_if_pinmux;
				pinmux[1].pnext = &pinmux[2];

				pinmux[2].func = PIN_FUNC_I2C;
				pinmux[2].cfg = p_sen_init->sen_init_cfg.pin_cfg.pinmux.cmd_if_pinmux;
				pinmux[2].pnext = NULL;

				cfg_obj.pin_cfg.pinmux.func = pinmux[0].func;
				cfg_obj.pin_cfg.pinmux.cfg = pinmux[0].cfg;
				cfg_obj.pin_cfg.pinmux.pnext = pinmux[0].pnext;

				cfg_obj.pin_cfg.clk_lane_sel = p_sen_init->sen_init_cfg.pin_cfg.clk_lane_sel;
				if (sizeof(cfg_obj.pin_cfg.sen_2_serial_pin_map) == sizeof(p_sen_init->sen_init_cfg.pin_cfg.sen_2_serial_pin_map)) {
					memcpy(cfg_obj.pin_cfg.sen_2_serial_pin_map, p_sen_init->sen_init_cfg.pin_cfg.sen_2_serial_pin_map, sizeof(cfg_obj.pin_cfg.sen_2_serial_pin_map));
				} else {
					DBG_ERR("VDOCAP_SEN_SER_MAX_DATALANE size not match!\r\n");
					ret = ISF_ERR_PROCESS_FAIL;
					break;
				}
				cfg_obj.pin_cfg.ccir_msblsb_switch = p_sen_init->sen_init_cfg.pin_cfg.ccir_msblsb_switch;
				cfg_obj.pin_cfg.ccir_vd_hd_pin = p_sen_init->sen_init_cfg.pin_cfg.ccir_vd_hd_pin;
				cfg_obj.pin_cfg.vx1_tx241_cko_pin = p_sen_init->sen_init_cfg.pin_cfg.vx1_tx241_cko_pin;
				cfg_obj.pin_cfg.vx1_tx241_cfg_2lane_mode = p_sen_init->sen_init_cfg.pin_cfg.vx1_tx241_cfg_2lane_mode;

				cfg_obj.if_cfg.type = p_sen_init->sen_init_cfg.if_cfg.type;
				cfg_obj.if_cfg.tge.tge_en = p_sen_init->sen_init_cfg.if_cfg.tge.tge_en;
				cfg_obj.if_cfg.tge.swap = p_sen_init->sen_init_cfg.if_cfg.tge.swap;
				cfg_obj.if_cfg.tge.sie_vd_src = p_sen_init->sen_init_cfg.if_cfg.tge.sie_vd_src;
				cfg_obj.if_cfg.mclksrc_sync = p_ctx->mclksrc_sync;
				cfg_obj.cmd_if_cfg.vx1.en = p_sen_init->sen_init_cfg.cmd_if_cfg.vx1.en;
				cfg_obj.cmd_if_cfg.vx1.if_sel = p_sen_init->sen_init_cfg.cmd_if_cfg.vx1.if_sel;
//				cfg_obj.cmd_if_cfg.vx1.ctl_sel = p_sen_init->sen_init_cfg.cmd_if_cfg.vx1.ctl_sel;
				cfg_obj.cmd_if_cfg.vx1.tx_type = p_sen_init->sen_init_cfg.cmd_if_cfg.vx1.tx_type;

				cfg_obj.drvdev |= _vdocap_lvds_csi_mapping(cfg_obj.if_cfg.type, cfg_obj.pin_cfg.clk_lane_sel);

				if (cfg_obj.if_cfg.tge.tge_en) {
					if (p_ctx->id == 0) {
						cfg_obj.drvdev |= CTL_SEN_DRVDEV_TGE_0;
					} else {
						cfg_obj.drvdev |= CTL_SEN_DRVDEV_TGE_1;
					}
				}
				if (p_ctx->ad_map) {
					cfg_obj.sendrv_cfg.ad_id_map.chip_id = GET_AD_CHIPID(p_ctx->ad_map);
					cfg_obj.sendrv_cfg.ad_id_map.vin_id = GET_AD_IN(p_ctx->ad_map);
				}
				ret = p_sen_obj->init_cfg((CHAR *)p_sen_init->driver_name, &cfg_obj);
				if(ret) {
					DBG_ERR("sen init_cfg failed(%d)!\r\n", ret);
					ret = ISF_ERR_INVALID_VALUE;
					break;
				}
				if_type = cfg_obj.if_cfg.type;
				if ((if_type == CTL_SEN_IF_TYPE_MIPI) || (if_type == CTL_SEN_IF_TYPE_LVDS) || (if_type == CTL_SEN_IF_TYPE_SLVSEC)) {
					p_ctx->count_vd_by_sensor = TRUE;
				}

			} else {
				DBG_UNIT("ctl_sen is opened.\r\n");
			}
			p_ctx->sen_option_en = p_sen_init->sen_init_option.en_mask;
//			p_ctx->sen_map_if = p_sen_init->sen_init_option.sen_map_if;
			p_ctx->sen_timeout_ms = p_sen_init->sen_init_option.if_time_out;
			if (cfg_obj.if_cfg.tge.tge_en) {
				p_ctx->tge_sync_id = p_sen_init->sen_init_cfg.if_cfg.tge.sie_sync_set;
			} else {
				p_ctx->tge_sync_id = 0;
			}

		}
			break;
		case VDOCAP_PARAM_SEN_MODE_INFO: {
			VDOCAP_SEN_MODE_INFO *p_sen_mode_info = (VDOCAP_SEN_MODE_INFO *)p_struct;
			UINT32 fps;
			CTL_SEN_DATA_FMT sen_data_fmt;
			CTL_SEN_PIXDEPTH sen_pixdepth;

			#if 0//defined(CONFIG_NVT_FPGA_EMULATION) || defined(_NVT_FPGA_)
				p_sen_mode_info->frc = MAKE_UINT16_UINT16(15, 1);
				DBG_WRN("Force to reduce fps(15) for FPGA.\r\n");
			#endif
			if (_vdocap_is_pdaf_mode(p_ctx)) {
				UINT32 main_pdaf_id = _vdocap_get_main_pdaf_id(p_ctx);

				///////temp solution for pdaf size//////
				p_ctx->chgsenmode_info.auto_info.size.w =  p_sen_mode_info->dim.w;
				p_ctx->chgsenmode_info.auto_info.size.h = p_sen_mode_info->dim.h;

				if (p_ctx->id != main_pdaf_id) {
					#if 0
					VDOCAP_CONTEXT *p_main_ctx;

					p_main_ctx = _vdocap_id_to_ctx(p_ctx->id);
					//to do
					//set pdaf size here
					#endif
					return ISF_OK;
				}
			}
			fps = 100*GET_HI_UINT16(p_sen_mode_info->frc)/GET_LO_UINT16(p_sen_mode_info->frc);
			if (p_ctx->flow_type == CTL_SIE_FLOW_PATGEN) {

				p_ctx->pat_gen_info.act_win.x = 0;
				p_ctx->pat_gen_info.act_win.y = 0;
				p_ctx->pat_gen_info.act_win.w = p_sen_mode_info->dim.w;
				p_ctx->pat_gen_info.act_win.h = p_sen_mode_info->dim.h;
				p_ctx->pat_gen_info.pat_gen_mode = GET_HI_UINT16(p_sen_mode_info->sen_mode);
				if (p_ctx->pat_gen_info.pat_gen_mode == 0 || p_ctx->pat_gen_info.pat_gen_mode > CTL_SIE_PAT_HVINCREASE) {
					DBG_WRN("pat_gen_mode(%d) not support!\r\n", p_ctx->pat_gen_info.pat_gen_mode);
					p_ctx->pat_gen_info.pat_gen_mode = CTL_SIE_PAT_COLORBAR;
				}
				p_ctx->pat_gen_info.pat_gen_val = GET_LO_UINT16(p_sen_mode_info->sen_mode);
				p_ctx->pat_gen_info.frame_rate = fps;
				if (p_ctx->shdr_map == VDOCAP_SEN_HDR_SET1_MAIN) {
					_vdocap_shdr_frm_num[VDOCAP_SHDR_SET1] = p_sen_mode_info->out_frame_num;
					_vdocap_shdr_oport_initqueue(VDOCAP_SHDR_SET1);
				} else if (p_ctx->shdr_map == VDOCAP_SEN_HDR_SET2_MAIN) {
					_vdocap_shdr_frm_num[VDOCAP_SHDR_SET2] = p_sen_mode_info->out_frame_num;;
					_vdocap_shdr_oport_initqueue(VDOCAP_SHDR_SET2);
				}

			} else {
				if (VDOCAP_SEN_MODE_AUTO == p_sen_mode_info->sen_mode) {
					p_ctx->chgsenmode_info.sel = CTL_SEN_MODESEL_AUTO;
					p_ctx->chgsenmode_info.auto_info.frame_rate = fps;
					p_ctx->chgsenmode_info.auto_info.size.w =  p_sen_mode_info->dim.w;
					p_ctx->chgsenmode_info.auto_info.size.h = p_sen_mode_info->dim.h;
					p_ctx->chgsenmode_info.auto_info.frame_num = p_sen_mode_info->out_frame_num;
					_vdocap_pixfmt_to_senfmt(p_sen_mode_info->pxlfmt, &sen_data_fmt, &sen_pixdepth);
					p_ctx->chgsenmode_info.auto_info.data_fmt = sen_data_fmt;
					p_ctx->chgsenmode_info.auto_info.pixdepth = sen_pixdepth;
				} else {
					p_ctx->chgsenmode_info.sel = CTL_SEN_MODESEL_MANUAL;
					p_ctx->chgsenmode_info.manual_info.sen_mode = p_sen_mode_info->sen_mode;
					p_ctx->chgsenmode_info.manual_info.frame_rate = fps;
				}
			}

		}
			break;
		case VDOCAP_PARAM_CCIR_INFO:	{
			VDOCAP_CCIR_INFO *p_ccir_info = (VDOCAP_CCIR_INFO *)p_struct;

			p_ctx->chgsenmode_info.auto_info.ccir.fmt = p_ccir_info->fmt;
			p_ctx->chgsenmode_info.auto_info.ccir.interlace = p_ccir_info->interlace;
			p_ctx->ccir_field_sel = p_ccir_info->field_sel;
			p_ctx->mux_data_index = p_ccir_info->mux_data_index;
		}
			break;
		case VDOCAP_PARAM_GYRO_INFO:	{
			VDOCAP_GYRO_INFO *p_gyro_info = (VDOCAP_GYRO_INFO *)p_struct;

			DBG_MSG("Gyro en=%d data_num=%d\r\n", p_gyro_info->en, p_gyro_info->data_num);
			if (p_gyro_info->en && _vcap_gyro_obj[0] == 0) {
				DBG_ERR("No Gryo driver!\r\n");
				return ISF_ERR_UNINIT;
			} else if (p_gyro_info->en && p_gyro_info->data_num > MAX_GYRO_DATA_NUM) {
				DBG_ERR("Only support max Gyro data num = %d\r\n", MAX_GYRO_DATA_NUM);
				return ISF_ERR_NOT_SUPPORT;
			} else {
				memcpy(&p_ctx->gyro_info, p_gyro_info, sizeof(VDOCAP_GYRO_INFO));
			}
		}
			break;
		case VDOCAP_PARAM_AE_PRESET:	{
			VDOCAP_AE_PRESET *p_ae_preset = (VDOCAP_AE_PRESET *)p_struct;

			p_ctx->ae_preset.enable = p_ae_preset->enable;
			p_ctx->ae_preset.exp_time = p_ae_preset->exp_time;
			p_ctx->ae_preset.gain_ratio = p_ae_preset->gain_ratio;
		}
			break;
#if defined(_DVS_FUNC_)
		case VDOCAP_PARAM_DVS_INFO:	{
			memcpy((void *)&p_ctx->dvs_info, (void *)p_struct, sizeof(VDOCAP_DVS_INFO));
		}
			break;
#endif
		case VDOCAP_PARAM_CROP_ALIGN:	{
			USIZE *p_align = (USIZE *)p_struct;

			if (p_align->w) {
				p_ctx->io_size.align.w = p_align->w;
			} else {
				p_ctx->io_size.align.w = CTL_SIE_DFT;
			}
			if (p_align->h) {
				p_ctx->io_size.align.h = p_align->h;
			} else {
				p_ctx->io_size.align.h = CTL_SIE_DFT;
			}
		}
			break;
		default:
			DBG_ERR("VdoIn.out[%d] set struct[%08x] = %08x\r\n", nport-ISF_OUT_BASE, param, (UINT32)p_struct);
			ret = ISF_ERR_NOT_SUPPORT;
			break;
		}

	    return ret;
	}
	return ISF_ERR_NOT_SUPPORT;
}
ISF_RV _isf_vdocap_do_getportstruct(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32 *p_struct, UINT32 size)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);

	if (p_ctx == NULL) {
		DBG_UNIT("VDOCAP buf not init!\r\n");
		return ISF_ERR_INIT; //sitll not init
	}
	DBG_UNIT("VDOCAP[%d] port[%d] param = 0x%X size = %d\r\n", p_ctx->id, nport, param, size);
	if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
	/* for output port */
	} else if (nport-ISF_IN_BASE < p_thisunit->port_in_count) {
	/* for input port */
	} else if (nport == ISF_CTRL) {
		/* for common port */
		switch (param) {
		case VDOCAP_PARAM_SYS_INFO: {
			VDOCAP_SYSINFO *p_sys_info = (VDOCAP_SYSINFO *)p_struct;
			CTL_SEN_GET_FPS_PARAM fps_param;
			PCTL_SEN_OBJ p_sen_obj;
			INT32 ret;

			if (size < sizeof(VDOCAP_SYSINFO)) {
				DBG_ERR("error size(%d)\r\n", size);
				break;
			}

			p_sen_obj = ctl_sen_get_object(p_ctx->id);
			if ((p_sen_obj == NULL) || (p_sen_obj->get_cfg == NULL)) {
				DBG_ERR("get sen_obj[%d] failed!\r\n", p_ctx->id);
				break;
			}
			memset((void *)(&fps_param), 0, sizeof(CTL_SEN_GET_FPS_PARAM));
			if (p_sen_obj->is_opened()) {
				ret = p_sen_obj->get_cfg(CTL_SEN_CFGID_GET_FPS, (void *)(&fps_param));
	    		if (ret) {
	    			DBG_ERR("get sen[%d] fps failed!(%d)\r\n", p_ctx->id, ret);
	    			//return ISF_ERR_NOT_START;
					if (0 == p_ctx->started) {
						DBG_ERR("VCAP NOT START\r\n");
					}
    			}

				if (p_ctx->count_vd_by_sensor) {
					UINT32 cnt = 0;

					ret = p_sen_obj->get_cfg(CTL_SEN_CFGID_GET_VD_CNT, (void *)(&cnt));
					if (CTL_SEN_E_OK == ret) {
						p_ctx->vd_count = cnt;
					}
				}
    		}

			//DBG_DUMP("%5d %8d %8d %8d\r\n", fps_param.mode, fps_param.dft_fps, fps_param.chg_fps, fps_param.cur_fps);
    		p_sys_info->cur_fps = MAKE_UINT16_UINT16(fps_param.cur_fps, 100);
			#if 0
			if (p_ctx->csi.error == 0) {
				p_sys_info->vd_count = p_ctx->vd_count;
				p_ctx->vd_count_prev = p_ctx->vd_count;
				DBG_UNIT("VDOCAP[%d] curr fps = %d  count=%llu output_started=%d\r\n", p_ctx->id, fps_param.cur_fps, p_sys_info->vd_count, p_ctx->started);
			} else {
				p_sys_info->vd_count = p_ctx->vd_count_prev;
				DBG_UNIT("VDOCAP[%d]##count=%llu(%llu)##\r\n", p_ctx->id, p_ctx->vd_count, p_ctx->vd_count_prev);
			}
			#else
			p_sys_info->vd_count = p_ctx->vd_count;
			#endif
			p_sys_info->output_started = (BOOL)p_ctx->started;
			p_sys_info->cur_dim.w = p_ctx->sen_dim.w;
			p_sys_info->cur_dim.h = p_ctx->sen_dim.h;
		}
			return ISF_OK;
		case VDOCAP_PARAM_GET_PLUG_INFO: {
			VDOCAP_SEN_GET_PLUG_INFO *p_get_plug_info = (VDOCAP_SEN_GET_PLUG_INFO *)p_struct;
			CTL_SEN_GET_PLUG_INFO_PARAM get_plug_info = {0};
			PCTL_SEN_OBJ p_sen_obj;
			INT32 ret;

			if (size < sizeof(VDOCAP_SEN_GET_PLUG_INFO)) {
				DBG_ERR("error size(%d)\r\n", size);
				break;
			}

			p_sen_obj = ctl_sen_get_object(p_ctx->id);
			if ((p_sen_obj == NULL) || (p_sen_obj->get_cfg == NULL)) {
				DBG_ERR("get sen_obj[%d] failed!\r\n", p_ctx->id);
				break;
			}
			if (p_sen_obj->is_opened()) {
				ret = p_sen_obj->get_cfg(CTL_SEN_CFGID_GET_PLUG_INFO, (void *)(&get_plug_info));
				if (ret) {
					DBG_ERR("get sen[%d] GET_PLUG_INFO failed!(%d)\r\n", p_ctx->id, ret);
				}
			}

			if (sizeof(CTL_SEN_GET_PLUG_INFO_PARAM) == sizeof(VDOCAP_SEN_GET_PLUG_INFO)) {
				memcpy(p_get_plug_info, &get_plug_info, sizeof(VDOCAP_SEN_GET_PLUG_INFO));
			} else {
				DBG_ERR("VDOCAP_SEN_GET_PLUG_INFO size not match!\r\n");
				return ISF_ERR_PROCESS_FAIL;
			}

		}
			return ISF_OK;
		case VDOCAP_PARAM_SYS_CAPS: {
			VDOCAP_SYSCAPS *p_sys_caps = (VDOCAP_SYSCAPS *)p_struct;
			CTL_SEN_DATA_FMT data_fmt;
			CTL_SEN_PIXDEPTH pixel_depth;
			CTL_SEN_GET_MODE_BASIC_PARAM mode_param = {0};
			PCTL_SEN_OBJ p_sen_obj;


			if (size < sizeof(VDOCAP_SYSCAPS)) {
				DBG_ERR("error size(%d)\r\n", size);
				break;
			}
			if (p_ctx->flow_type == CTL_SIE_FLOW_PATGEN) {
				p_sys_caps->max_dim.w = PATGEN_MAX_DIM_W;
				p_sys_caps->max_dim.h = PATGEN_MAX_DIM_H;
				p_sys_caps->max_frame_rate = MAKE_UINT16_UINT16(PATGEN_MAX_FPS, 100);
				p_sys_caps->pxlfmt = VDO_PXLFMT_RAW16;
			} else {
				p_sen_obj = ctl_sen_get_object(p_ctx->id);
				if ((p_sen_obj == NULL) || (p_sen_obj->get_cfg == NULL)) {
					DBG_ERR("get sen_obj[%d] failed!\r\n", p_ctx->id);
					break;
				}
				mode_param.mode = vdocap_get_max_dim_mode(p_thisunit, p_sen_obj);
				if (p_sen_obj->get_cfg(CTL_SEN_CFGID_GET_MODE_BASIC, (void *)(&mode_param))) {
					DBG_ERR("get sen[%d] dim failed!\r\n", p_ctx->id);
					return ISF_ERR_NOT_SUPPORT;
				}
				p_sys_caps->max_dim.w = mode_param.crp_size.w;
				p_sys_caps->max_dim.h = mode_param.crp_size.h;
				p_sys_caps->max_frame_rate = MAKE_UINT16_UINT16(mode_param.dft_fps, 100);
				data_fmt = mode_param.data_fmt;
				pixel_depth = mode_param.pixel_depth;
				_vdocap_senfmt_to_pixfmt(data_fmt, pixel_depth, &p_sys_caps->pxlfmt);
			}
			DBG_MSG("sen max dim mode(%d) caps WxH=(%dx%d) fps=0x%X fmt=0x%X\r\n", mode_param.mode, p_sys_caps->max_dim.w, p_sys_caps->max_dim.h, p_sys_caps->max_frame_rate, p_sys_caps->pxlfmt);
		}
			return ISF_OK;
		default:
			DBG_ERR("VdoIn.out[%d] get struct[%08x] = %08x\r\n", nport-ISF_OUT_BASE, param, (UINT32)p_struct);
			break;
		}
	}


	return ISF_ERR_NOT_SUPPORT;
}



static ISF_RV _isf_vdocap_do_close(ISF_UNIT *p_thisunit, UINT32 oport)
{
	VDOCAP_CONTEXT *p_ctx = (VDOCAP_CONTEXT *)(p_thisunit->refdata);
	VDOCAP_SHDR_OUT_QUEUE *p_outq;
	UINT32 shdr_set = _vdocap_shdr_map_to_set(p_ctx->shdr_map);

	PCTL_SEN_OBJ p_sen_ctl_obj = ctl_sen_get_object(p_ctx->id);

	p_ctx->dev_ready = 0; //start close
	DBG_UNIT("VDOCAP[%d] port[%d]++\r\n", p_ctx->id, oport);
	if (p_ctx->sie_hdl) {
		PCTL_SEN_OBJ p_shdr_main_sen_ctl_obj = NULL;

		_isf_vdocap_oqueue_do_close(p_thisunit, oport);
		if (p_ctx->flow_type == CTL_SIE_FLOW_SEN_IN || p_ctx->flow_type == CTL_SIE_FLOW_SIG_DUPL) {
			if (p_sen_ctl_obj == NULL || p_sen_ctl_obj->close == NULL) {
				DBG_ERR("error id:%d", (UINT32)p_ctx->id);
				return ISF_ERR_FAILED;
			}
			if (_vdocap_is_shdr_mode(p_ctx->shdr_map)) {//shdr mode
				//only the last shdr vdocap could close sensor to prevent from sie waiting no VD
				if (_vdocap_is_last_opened_shdr(p_ctx->id, shdr_set)) {
					ISF_UNIT *p_main_unit = _vdocap_shdr_main_unit[shdr_set];
					VDOCAP_CONTEXT *p_main_ctx;


					if (p_main_unit) {
						p_main_ctx = (VDOCAP_CONTEXT *)(p_main_unit->refdata);

						p_shdr_main_sen_ctl_obj = ctl_sen_get_object(p_main_ctx->id);
						//set p_thisunit to close MCLK, since the p_main_unit's ctl_sie has been closed
						_vdocap_sensor_ctl(p_shdr_main_sen_ctl_obj, p_ctx, FALSE);
					} else {
						DBG_ERR("shdr fail:map=%d", (UINT32)p_ctx->shdr_map);
						return ISF_ERR_FAILED;
					}
				} else {
					//do nothing
				}
			} else {//normal mode
				_vdocap_sensor_ctl(p_sen_ctl_obj, p_ctx, FALSE);
			}
		}
		if (_vdocap_is_shdr_mode(p_ctx->shdr_map)) {//shdr mode
			//close all sie after sensor power down
			if (_vdocap_is_last_opened_shdr(p_ctx->id, shdr_set)) {
				_vdocap_close_shdr_sie((UINT32)p_ctx->shdr_map);
			} else {
				//do nothing
			}
		} else {
			ER ret;

			p_ctx->dev_trigger_close = 1;
			DBG_UNIT("VDOCAP[%d] ctl_sie_close ++\r\n", p_ctx->id);
			ret = ctl_sie_close(p_ctx->sie_hdl);
			if (ret != E_OK) {
				DBG_ERR("- %s close failed(%d)!\r\n", p_thisunit->unit_name, ret);
				return ISF_ERR_FAILED;
			}
			DBG_UNIT("VDOCAP[%d] ctl_sie_close --\r\n", p_ctx->id);
			p_ctx->sie_hdl = 0;
			p_ctx->dev_trigger_close = 0;
		}
		if (p_ctx->flow_type == CTL_SIE_FLOW_SEN_IN || p_ctx->flow_type == CTL_SIE_FLOW_SIG_DUPL) {
			ER ret = ISF_OK;

			if (_vdocap_is_shdr_mode(p_ctx->shdr_map)) {//shdr mode
				//only the last shdr vdocap could close sensor to prevent from sie waiting no VD
				if (_vdocap_is_last_opened_shdr(p_ctx->id, shdr_set)) {
					ret = p_shdr_main_sen_ctl_obj->close();
				} else {
					//do nothing
				}
			} else {//normal mode
				ret = p_sen_ctl_obj->close();
			}
			if (ret) {
				DBG_ERR("sen close failed(%d)", ret);
				return ISF_ERR_FAILED;
			}
		}
		if (_vdocap_is_shdr_mode(p_ctx->shdr_map)) {//shdr flow
			if (_vdocap_is_last_opened_shdr(p_ctx->id, shdr_set)) {
				ISF_UNIT *p_main_unit = _vdocap_shdr_main_unit[shdr_set];

				p_outq = &_vdocap_shdr_queue[shdr_set];
				if (p_main_unit) {
					_vdocap_shdr_oport_close_check(p_main_unit, oport, p_outq);
				} else {
					DBG_ERR("check shdr fail:map=%d", (UINT32)p_ctx->shdr_map);
					return ISF_ERR_FAILED;
				}
			}
		} else {
			_vdocap_oport_block_check(p_thisunit, oport);
		}
		DBG_UNIT("VDOCAP[%d] port[%d]--\r\n", p_ctx->id, oport);
		return ISF_OK;
	} else {
		return ISF_ERR_NOT_OPEN_YET;
	}
}

ISF_RV _isf_vdocap_updateport(ISF_UNIT *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd)
{
	ISF_RV r = ISF_OK;

	switch (cmd) {
	case ISF_PORT_CMD_RESET:
		_isf_vdocap_do_reset(p_thisunit, oport);
		break;
	case ISF_PORT_CMD_OFFTIME_SYNC:     ///< off -> off     (sync port info & sync off-time parameter if it is dirty)
		_isf_vdocap_do_offsync(p_thisunit, oport);
		break;
	case ISF_PORT_CMD_OPEN:             ///< off -> ready   (alloc mempool and start task)
		r = _isf_vdocap_do_open(p_thisunit, oport);
		break;
	case ISF_PORT_CMD_READYTIME_SYNC:   ///< ready -> ready (sync port info & sync ready-time parameter if it is dirty)
		_isf_vdocap_do_runtime(p_thisunit, cmd, oport, 0xffffffff);
		break;
	case ISF_PORT_CMD_START:            ///< ready -> run   (initial context, engine enable and device power on, start data processing)
		r = _isf_vdocap_do_start(p_thisunit, oport);
		break;
	case ISF_PORT_CMD_RUNTIME_SYNC:     ///< run -> run     (sync port info & sync run-time parameter if it is dirty)
		_isf_vdocap_do_runtime(p_thisunit, cmd, oport, 0xffffffff);
		break;
	case ISF_PORT_CMD_RUNTIME_UPDATE:   ///< run -> run     (update change)
		_isf_vdocap_do_runtime(p_thisunit, cmd, oport, 0xffffffff);
		break;
	case ISF_PORT_CMD_STOP:             ///< run -> stop    (stop data processing, engine disable or device power off)
		r = _isf_vdocap_do_stop(p_thisunit, oport);
		break;
	case ISF_PORT_CMD_CLOSE:            ///< stop -> off    (terminate task and free mempool)
		r = _isf_vdocap_do_close(p_thisunit, oport);
		break;
	case ISF_PORT_CMD_PAUSE:            ///< run -> wait    (pause data processing, keep context)
		break;
	case ISF_PORT_CMD_RESUME:           ///< wait -> run    (restore context, resume data processing)
		break;
	case ISF_PORT_CMD_SLEEP:            ///< run -> sleep   (pause data processing, keep context, engine or device enter suspend mode)
		break;
	case ISF_PORT_CMD_WAKEUP:           ///< sleep -> run   (engine or device leave suspend mode, restore context, resume data processing)
	//ben to do ?
		break;
	default:
		break;
	}
	return r;
}

///////////////////////////////////////////////////////////////////////////////


