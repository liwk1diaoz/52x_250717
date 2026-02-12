#include "isf_flow_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_flow_cmm
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_flow_cmm_debug_level = __DBGLVL__;
//module_param_named(isf_flow_cmm_debug_level, isf_flow_cmm_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_flow_cmm_debug_level, "flow debug level");
///////////////////////////////////////////////////////////////////////////////

#define DUMP_DATA_NEW_FAIL		DISABLE

static ISF_RV _common_init(struct _ISF_DATA *p_data, UINT32 version)
{
	p_data->h_data = 0;
	return ISF_OK;
}

static ISF_RV _common_verify(struct _ISF_DATA *p_data)
{
	return ISF_OK;
}

static NVTMPP_MODULE    g_module = MAKE_NVTMPP_MODULE('u', 's', 'e', 'r', 0, 0, 0, 0);

static UINT32 _common_new(struct _ISF_DATA *p_data, struct _ISF_UNIT *p_thisunit, UINT32 pool, UINT32 blk_size, UINT32 ddr)
{
	NVTMPP_VB_BLK blk_id;
	UINT32 addr = 0;
	UINT64 module;
	if (p_thisunit) {
		module = p_thisunit->unit_module;
	} else {
		module = g_module;
	}

	p_data->h_data = 0;

	blk_id = nvtmpp_vb_get_block(module, pool, blk_size, ddr);
	if (NVTMPP_VB_INVALID_BLK == blk_id) {
#if (DUMP_DATA_NEW_FAIL == ENABLE)
		DBG_ERR("\"%s\" getblk fail-1\r\n", (p_thisunit) ? (p_thisunit->unit_name) : ("user"));
#endif
#if (DUMP_DATA_NEW_FAIL == ENABLE)
		//loc_cpu();
		nvtmpp_dump_status(debug_msg);
		//unl_cpu();
#endif
		return addr;
	}
	addr = nvtmpp_vb_blk2va(blk_id);
	if (addr == 0) {
#if (DUMP_DATA_NEW_FAIL == ENABLE)
		DBG_ERR("\"%s\" block2addr fail-1, blk=0x%x\r\n", (p_thisunit) ? (p_thisunit->unit_name) : ("user"), blk_id);
#endif
		return (UINT32)addr;
	}
	p_data->h_data = (UINT32)blk_id;
	return addr;
}
static ISF_RV _common_lock(struct _ISF_DATA *p_data, struct _ISF_UNIT *p_thisunit, UINT32 h_data)
{
	ISF_RV r = ISF_OK;
	NVTMPP_ER          ret;
	UINT64 module;
	if (p_thisunit) {
		module = p_thisunit->unit_module;
	} else {
		module = g_module;
	}
	ret = nvtmpp_vb_lock_block(module, p_data->h_data);
	if (NVTMPP_ER_OK != ret) {
		DBG_ERR("%s: fail = %d\r\n", (p_thisunit) ? (p_thisunit->unit_name) : ("user"), ret);
		r = ISF_ERR_BUFFER_ADD;
	}
	return r;
}
static ISF_RV _common_unlock(struct _ISF_DATA *p_data, struct _ISF_UNIT *p_thisunit, UINT32 h_data)
{
	ISF_RV r = ISF_OK;
	NVTMPP_ER          ret;
	UINT64 module;
	if (p_thisunit) {
		module = p_thisunit->unit_module;
	} else {
		module = g_module;
	}
	ret = nvtmpp_vb_unlock_block(module, p_data->h_data);
	if (NVTMPP_ER_OK != ret) {
		DBG_ERR("%s: fail = %d\r\n", (p_thisunit) ? (p_thisunit->unit_name) : ("user"), ret);
		r = ISF_ERR_BUFFER_RELEASE;
	}
	if (nvtmpp_vb_check_mem_corrupt() < 0) {
		DBG_ERR("%s: check_mem_corrupt fail\r\n", (p_thisunit) ? (p_thisunit->unit_name) : ("user"));
		debug_dump_current_stack();
		// coverity[no_escape]
		while (1);
	}
	return r;
}

ISF_DATA_CLASS _common_base_data =
{
	.this = MAKEFOURCC('C','O','M','M'),
	.base = 0,

	.do_init = _common_init,
	.do_verify = _common_verify,
	.do_new = _common_new,
	.do_lock = _common_lock,
	.do_unlock = _common_unlock,
};


