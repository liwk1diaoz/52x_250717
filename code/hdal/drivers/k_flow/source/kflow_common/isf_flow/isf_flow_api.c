#include "isf_flow_int.h"
#include "isf_flow_api.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_flow
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_flow_debug_level = NVT_DBG_WRN;
//module_param_named(isf_flow_debug_level, isf_flow_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_flow_debug_level, "flow debug level");
///////////////////////////////////////////////////////////////////////////////

static ISF_UNIT* _isf_unit[ISF_MAX_UNIT] = {0};

#define GET_UNIT_ID(v)	((((UINT32)(v)) >> 16) & 0x0000ffff)
#define GET_PORT_ID(v)	(((UINT32)(v)) & 0x0000ffff)

#define MAKE_UNITPORT(unit_id, port_id)	\
	(((((UINT32)(unit_id)) & 0x0000ffff) << 16) | ((((UINT32)(port_id)) & 0x0000ffff)))

#define GET_UNIT_PTR(p_unit, unit_id) \
	if (((unit_id) - ISF_UNIT_BASE) < ISF_MAX_UNIT) { \
		(p_unit) = _isf_unit[((unit_id) - ISF_UNIT_BASE)]; \
	}

#define SEEK_UNIT_ID(p_unit, unit_id) \
	{ \
		int i; \
		(unit_id) = 0; \
		for (i = 0; i < ISF_MAX_UNIT; i++) { \
			if ((p_unit) == _isf_unit[i]) { \
				(unit_id) = ISF_UNIT_BASE + (i); \
			} \
		} \
	}

ISF_UNIT* isf_unit_ptr(UINT32 unit_id)
{
	ISF_UNIT* ptr = 0;
	GET_UNIT_PTR(ptr, unit_id);
	return ptr;
}
EXPORT_SYMBOL(isf_unit_ptr);

ISF_RV isf_reg_unit(UINT32 unit_id, ISF_UNIT* reg_unit)
{
	if ((unit_id - ISF_UNIT_BASE) < ISF_MAX_UNIT) {
		ISF_RV r;
		DBG_IND("register unit(%d) = %s.\r\n", (unit_id - ISF_UNIT_BASE), reg_unit->unit_name);
		r = _isf_unit_verify(reg_unit, NULL);
		if(r != ISF_OK) {
			DBG_ERR("unit(%d) %s verify fail = %d.\r\n", (unit_id - ISF_UNIT_BASE), reg_unit->unit_name, r);
		}

		_isf_unit[(unit_id - ISF_UNIT_BASE)] = reg_unit;
		reg_unit->p_base = &_isf_unit_base; //install base

		return ISF_OK;
	}
	return ISF_ERR_INVALID_UNIT_ID;
}
EXPORT_SYMBOL(isf_reg_unit);

//		DBG_IND("get %08x as unit(%d) = %s.\r\n", unit, (unit - ISF_UNIT_BASE), p_unit->unit_name);

///////////////////////////////////////////////////////////////////////////////
#define ISF_MAX_DATA_CLASS		0x10 ///< end
static UINT32 _isf_data_class_count = 0;
static ISF_DATA_CLASS* _isf_data_class[ISF_MAX_DATA_CLASS] = {0};

ISF_DATA_CLASS* isf_data_class(UINT32 fourcc)
{
	UINT32 i;
	if(fourcc == 0)
		return NULL;
	for(i=0; i<_isf_data_class_count; i++){
		ISF_DATA_CLASS* _this_data_class = _isf_data_class[i];
		if(_this_data_class->this == fourcc) {
			return _this_data_class;
		}
	}
	return NULL;
}
EXPORT_SYMBOL(isf_data_class);

ISF_RV isf_data_regclass(ISF_DATA_CLASS* p_data_class)
{
	if(!p_data_class)
		return ISF_ERR_NULL_POINTER;
	if(isf_data_class(p_data_class->this) != NULL) {
		return ISF_OK;
	}
	if (_isf_data_class_count < ISF_MAX_DATA_CLASS) {
		//DBG_DUMP("register base_data(%d) = %08x.\r\n", _isf_data_class_count, p_data_class->this);
		_isf_data_class[_isf_data_class_count] = p_data_class;
		_isf_data_class_count++;
		return ISF_OK;
	}
	return ISF_ERR_INVALID_DATA_ID;
}
EXPORT_SYMBOL(isf_data_regclass);


///////////////////////////////////////////////////////////////////////////////
static UINT32 _isf_common_cfg = 0; //0x80 for "multi-process mode"
static UINT32 _isf_cfg[ISF_FLOW_MAX] = {0};
static int _isf_init[ISF_FLOW_MAX] = {0};

UINT32 isf_get_common_cfg(void) { return _isf_common_cfg; }

UINT32 isf_get_cfg(UINT32 h_isf) { return _isf_cfg[h_isf]; }



// h_isf is the process id that init by hd_common_init(HD_COMMON_CFG_MULTIPROC | process_id)
// h_isf 0 means server process, else means client process
ISF_RV isf_reset(UINT32 h_isf)
{
	UINT32 i=0, j = 0;
	ISF_RV r;
	ISF_STATE *p_state;
	BOOL   is_clear_all = FALSE;

	if (h_isf == ISF_MULTIPROC_ID_SERVER) {
		is_clear_all = TRUE;
	}
	DBG_DUMP("hd_reset process %d - begin\r\n", h_isf);
	for(j = 0; j< ISF_MAX_UNIT; j++) {
		ISF_UNIT* p_unit = _isf_unit[j];
		if (p_unit) {
			DBG_IND("unit(%d) = %s => clear state\r\n", j, p_unit->unit_name);
			for(i = 0; i < p_unit->port_out_count; i++) {
				UINT32 oport = i + ISF_OUT_BASE;
				p_state = p_unit->port_outstate[i];
				if (p_state->h_proc)
					DBG_IND("port(%d) h_proc 0x%x\r\n", i, p_state->h_proc);
				if ((!is_clear_all) && p_state->h_proc != (h_isf + 0x8000)) {
					continue;
				}
				r = _isf_unit_clear_state(p_unit, oport);
				if ((r != ISF_OK) && (r != ISF_ERR_IGNORE)) {
					DBG_ERR(" \"%s\" clear state fail!\r\n", p_unit->unit_name);
				}
				r = _isf_unit_clear_context(p_unit, oport);
				if ((r != ISF_OK) && (r != ISF_ERR_IGNORE)) {
					DBG_ERR(" \"%s\" clear context fail!\r\n", p_unit->unit_name);
				}
			}
		}
	}
	for(j = 0; j< ISF_MAX_UNIT; j++) {
		ISF_UNIT* p_unit = _isf_unit[j];
		if (p_unit) {
			DBG_IND("unit(%d) = %s => clear bind\r\n", j, p_unit->unit_name);
			for(i = 0; i < p_unit->port_out_count; i++) {
				UINT32 oport = i + ISF_OUT_BASE;
				ISF_VDO_INFO *p_src_vdoinfo = (ISF_VDO_INFO *)p_unit->port_outinfo[i];
				if (p_src_vdoinfo->h_proc)
					DBG_IND("port(%d) h_proc 0x%x\r\n", i, p_src_vdoinfo->h_proc);
				if ((!is_clear_all) && p_src_vdoinfo->h_proc != (h_isf + 0x8000)) {
					continue;
				}
				r = _isf_unit_clear_bind(p_unit, oport);
				if ((r != ISF_OK) && (r != ISF_ERR_IGNORE)) {
					DBG_ERR(" \"%s\" clear bind fail!\r\n", p_unit->unit_name);
				}
			}
		}
	}
	for(j = 0; j< ISF_MAX_UNIT; j++) {
		ISF_UNIT* p_unit = _isf_unit[j];
		if (p_unit) {
			DBG_IND("unit(%d) = %s => clear context\r\n", j, p_unit->unit_name);
			for(i = 0; i < p_unit->port_out_count; i++) {
				UINT32 oport = i + ISF_OUT_BASE;
				p_state = p_unit->port_outstate[i];
				if (p_state->h_proc)
					DBG_IND("port(%d) h_proc 0x%x\r\n", i, p_state->h_proc);
				if ((!is_clear_all) && p_state->h_proc != (h_isf + 0x8000)) {
					continue;
				}
				r = _isf_unit_clear_context(p_unit, oport);
				if ((r != ISF_OK) && (r != ISF_ERR_IGNORE)) {
					DBG_ERR(" \"%s\" clear context fail!\r\n", p_unit->unit_name);
				}
			}
		}
	}
	if (is_clear_all) {
		for(j = 0; j< ISF_MAX_UNIT; j++) {
			ISF_UNIT* p_unit = _isf_unit[j];
			if (p_unit && (p_unit->do_command)) {
				DBG_IND("unit(%d) = %s => uninit\r\n", j, p_unit->unit_name);
				r = p_unit->do_command(0, 0, 1, 0); //uninit (with force reset)
				if (r == ISF_ERR_UNINIT) {
					//still not init, do nothing
				} else if (r == ISF_OK) {
					DBG_DUMP(" \"%s\" already init! => uninit\r\n", p_unit->unit_name);
				} else {
					DBG_DUMP("\"%s\" already init! => uninit (failed %d)\r\n", p_unit->unit_name, r);
					return r;
				}
			}
		}
	}

	DBG_DUMP("hd_reset - end\r\n");
	return ISF_OK;
}

ISF_RV isf_init(UINT32 h_isf, UINT32 p0, UINT32 p1, UINT32 p2)
{
	//UINT32 i=0, j = 0;
	//ISF_RV r;

	_isf_data_reguser();

	isf_data_regclass(&_common_base_data);
	isf_data_regclass(&_vdofm_base_data);
	isf_data_regclass(&_audfm_base_data);
	isf_data_regclass(&_vdobs_base_data);
	isf_data_regclass(&_audbs_base_data);

 	DBG_DUMP("hd_common_init\r\n");
	DBG_DUMP("process id %d _isf_init %d\r\n", h_isf, _isf_init[h_isf]);


	if (h_isf >= ISF_FLOW_MAX) {
		DBG_DUMP("process id %d exceeds limit \r\n", h_isf);
		return ISF_ERR_PARAM_EXCEED_LIMIT;
	}
	if (h_isf > ISF_MULTIPROC_ID_SERVER) { //client process
		if (!_isf_init[ISF_MULTIPROC_ID_SERVER]) { //is master process already init?
			DBG_DUMP("hd_commom_init: server is not init yet?\r\n");
			return ISF_ERR_INIT;
		}
		if (isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) {
			if (!((p2 >> 24) & ISF_COMM_CFG_MULTIPROC)) {
				DBG_ERR("hd_common_init: proc %d MUST enable MULTIPROC mode!\r\n", (int)h_isf);
				return ISF_ERR_NOT_AVAIL;
			}
		} else {
			if (((p2 >> 24) & ISF_COMM_CFG_MULTIPROC)) {
				DBG_ERR("hd_common_init: proc %d MUST disable MULTIPROC mode!\r\n", (int)h_isf);
				return ISF_ERR_NOT_AVAIL;
			}
		}
		_isf_init[h_isf] = 1; //set init
		_isf_cfg[h_isf] = p2;
		return ISF_OK;
	} else { //master process
		UINT32 i;
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (_isf_init[i]) { //client process still alive?
				DBG_DUMP("hd_commom_init: client is still alive?\r\n");
				return ISF_ERR_INIT;
			}
		}
		_isf_cfg[h_isf] = p2;
		if ((_isf_cfg[h_isf] >> 24) & ISF_COMM_CFG_MULTIPROC) {
			_isf_common_cfg |= ISF_COMM_CFG_MULTIPROC;
			DBG_DUMP("hd_common_init: proc %d enter MULTIPROC mode\r\n", (int)h_isf);
		}
	}

	if (_isf_init[ISF_MULTIPROC_ID_SERVER] > 0) {
	DBG_DUMP("hd_common_init: already init?\r\n");
	DBG_DUMP("hd_reset - begin\r\n");
	#if 0
	isf_exit();
	#else

	isf_reset(h_isf);
	/*
	for(j = 0; j< ISF_MAX_UNIT; j++) {
		ISF_UNIT* p_unit = _isf_unit[j];
		if (p_unit) {
			DBG_IND("unit(%d) = %s => clear state\r\n", j, p_unit->unit_name);
			for(i = 0; i < p_unit->port_out_count; i++) {
				UINT32 oport = i + ISF_OUT_BASE;
				r = _isf_unit_clear_state(p_unit, oport);
				if ((r != ISF_OK) && (r != ISF_ERR_IGNORE)) {
					DBG_ERR(" \"%s\" clear state fail!\r\n", p_unit->unit_name);
				}
			}
		}
	}
	for(j = 0; j< ISF_MAX_UNIT; j++) {
		ISF_UNIT* p_unit = _isf_unit[j];
		if (p_unit) {
			DBG_IND("unit(%d) = %s => clear bind\r\n", j, p_unit->unit_name);
			for(i = 0; i < p_unit->port_out_count; i++) {
				UINT32 oport = i + ISF_OUT_BASE;
				r = _isf_unit_clear_bind(p_unit, oport);
				if ((r != ISF_OK) && (r != ISF_ERR_IGNORE)) {
					DBG_ERR(" \"%s\" clear bind fail!\r\n", p_unit->unit_name);
				}
			}
		}
	}
	for(j = 0; j< ISF_MAX_UNIT; j++) {
		ISF_UNIT* p_unit = _isf_unit[j];
		if (p_unit) {
			DBG_IND("unit(%d) = %s => clear context\r\n", j, p_unit->unit_name);
			for(i = 0; i < p_unit->port_out_count; i++) {
				UINT32 oport = i + ISF_OUT_BASE;
				r = _isf_unit_clear_context(p_unit, oport);
				if ((r != ISF_OK) && (r != ISF_ERR_IGNORE)) {
					DBG_ERR(" \"%s\" clear context fail!\r\n", p_unit->unit_name);
				}
			}
		}
	}
	for(j = 0; j< ISF_MAX_UNIT; j++) {
		ISF_UNIT* p_unit = _isf_unit[j];
		if (p_unit && (p_unit->do_command)) {
			DBG_IND("unit(%d) = %s => uninit\r\n", j, p_unit->unit_name);
			r = p_unit->do_command(0, 0, 1, 0); //uninit (with force reset)
			if (r == ISF_ERR_UNINIT) {
				//still not init, do nothing
			} else if (r == ISF_OK) {
				DBG_DUMP(" \"%s\" already init! => uninit\r\n", p_unit->unit_name);
			} else {
				DBG_DUMP("\"%s\" already init! => uninit (failed %d)\r\n", p_unit->unit_name, r);
				return r;
			}
		}
	}
	*/
	#endif

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
	debug_log_exit();
#else
	debug_log_exit();
#endif

	DBG_DUMP("hd_reset - end\r\n");
	_isf_init[ISF_MULTIPROC_ID_SERVER] = 0;
	}

	//////////////////////////////////////////////////////////

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
	debug_log_init();
#else
	debug_log_init();
#endif
	_isf_init[ISF_MULTIPROC_ID_SERVER] = 1;

	return ISF_OK;
}
ISF_RV isf_abort(UINT32 h_isf)
{
	if ((h_isf == ISF_MULTIPROC_ID_SERVER)) {
		if (_isf_init[h_isf] == 1) {  //main process is abort
			DBG_DUMP("hd: server is abort!\r\n");
			//_isf_init[0] = 0; //force clear init
		}
	}
	if ((h_isf > ISF_MULTIPROC_ID_SERVER) && (h_isf < ISF_FLOW_MAX)) {
		if (_isf_init[h_isf] == 1) { //client process is abort
			DBG_DUMP("hd: client %d is abort!\r\n", h_isf);
			isf_reset(h_isf);
			_isf_init[h_isf] = 0; //force clear init
		}
	}
	return ISF_OK;
}
ISF_RV isf_exit(UINT32 h_isf, UINT32 p0, UINT32 p1, UINT32 p2)
{
	DBG_DUMP("hd_commom_uninit\r\n");

	if (h_isf > ISF_MULTIPROC_ID_SERVER) { //client process
		if (!_isf_init[ISF_MULTIPROC_ID_SERVER]) { //is master process already init?
			DBG_DUMP("hd_commom_uninit: server is not init yet?\r\n");
			return ISF_ERR_UNINIT;
		}
		_isf_init[h_isf] = 0; //clear init
		_isf_cfg[h_isf] = 0;
		return ISF_OK;
	} else { //master process
		UINT32 i;
		if ((_isf_cfg[h_isf] >> 24) & ISF_COMM_CFG_MULTIPROC) {
			_isf_common_cfg &= ~ISF_COMM_CFG_MULTIPROC;
			DBG_DUMP("hd_commom_uninit: proc %d leave MULTIPROC mode\r\n", (int)h_isf);
		}
		for (i = 1; i < ISF_FLOW_MAX; i++) {
			if (_isf_init[i]) { //client process still alive?
				DBG_DUMP("hd_commom_uninit: client is still alive?\r\n");
				return ISF_ERR_UNINIT;
			}
		}
		_isf_cfg[h_isf] = 0;
	}

	//int i=0, j = 0;
	if (!_isf_init[h_isf]) {
		DBG_DUMP("hd_commom_uninit: not init yet?\r\n");
		return ISF_ERR_UNINIT;
	}

	/*
	//do reset
	DBG_DUMP("hd_reset - begin\r\n");
	#if 0
	isf_exit();
	#else
	for(j = 0; j< ISF_MAX_UNIT; j++) {
		ISF_UNIT* p_unit = _isf_unit[j];
		if (p_unit) {
			DBG_IND("unit(%d) = %s => clear state\r\n", j, p_unit->unit_name);
			for(i = 0; i < p_unit->port_out_count; i++) {
				UINT32 oport = i + ISF_OUT_BASE;
				_isf_unit_clear_state(p_unit, oport);
			}
		}
	}
	for(j = 0; j< ISF_MAX_UNIT; j++) {
		ISF_UNIT* p_unit = _isf_unit[j];
		if (p_unit) {
			DBG_IND("unit(%d) = %s => clear bind\r\n", j, p_unit->unit_name);
			for(i = 0; i < p_unit->port_out_count; i++) {
				UINT32 oport = i + ISF_OUT_BASE;
				_isf_unit_clear_bind(p_unit, oport);
			}
		}
	}
	for(j = 0; j< ISF_MAX_UNIT; j++) {
		ISF_UNIT* p_unit = _isf_unit[j];
		if (p_unit) {
			DBG_IND("unit(%d) = %s => clear context\r\n", j, p_unit->unit_name);
			for(i = 0; i < p_unit->port_out_count; i++) {
				UINT32 oport = i + ISF_OUT_BASE;
				_isf_unit_clear_context(p_unit, oport);
			}
		}
	}
	#endif
	debug_log_exit();

	DBG_DUMP("hd_reset - end\r\n");
*/
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
	debug_log_exit();
#else
	debug_log_exit();
#endif

	_isf_init[ISF_MULTIPROC_ID_SERVER] = 0;
	return ISF_OK;
}

ISF_RV isf_cmd(UINT32 h_isf, UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2)
{
	ISF_UNIT *p_thisunit = 0;
	UINT32 uid = GET_UNIT_ID(p0);
	GET_UNIT_PTR(p_thisunit, uid);

	if (p_thisunit && (p_thisunit->do_command)) {
		return p_thisunit->do_command(cmd | (h_isf << 28), p0, p1, p2);
	}
	return ISF_ERR_INVALID_UNIT_ID;
}
EXPORT_SYMBOL(isf_cmd);

ISF_RV isf_unit_set_param(UINT32 h_isf, UINT32 unit_nport, UINT32 param, UINT32 value)
{
	UINT32 uid = GET_UNIT_ID(unit_nport);
	UINT32 pid = GET_PORT_ID(unit_nport);
	UINT32 this_value = value;
	ISF_UNIT *p_unit = 0;
	GET_UNIT_PTR(p_unit, uid);
	if (!p_unit) {
		DBG_ERR("unit %04x is invalid!\r\n", uid);
		return ISF_ERR_INVALID_UNIT_ID;
	}
	switch (pid) {
	case ISF_PORT_NULL:
	case ISF_PORT_EOS:
	case ISF_PORT_KEEP:
		{
			DBG_ERR("port %04x is not support!\r\n", pid);
			return ISF_ERR_NOT_SUPPORT;
		}
		break;
	default:
		if ((pid <= ISF_OUT_MAX)) {
		} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
		} else if (pid == ISF_CTRL) {
		} else {
			DBG_ERR("port %04x is invalid!\r\n", pid);
			return ISF_ERR_INVALID_PORT_ID;
		}
		break;
	}
	return _isf_unit_set_param(h_isf, p_unit, pid, param, &this_value);
}
ISF_RV isf_unit_get_param(UINT32 h_isf, UINT32 unit_nport, UINT32 param, UINT32* p_value)
{
	UINT32 uid = GET_UNIT_ID(unit_nport);
	UINT32 pid = GET_PORT_ID(unit_nport);
	ISF_UNIT *p_unit = 0;
	GET_UNIT_PTR(p_unit, uid);
	if (!p_unit) {
		DBG_ERR("unit %04x is invalid!\r\n", uid);
		return ISF_ERR_INVALID_UNIT_ID;
	}
	switch (pid) {
	case ISF_PORT_NULL:
	case ISF_PORT_EOS:
	case ISF_PORT_KEEP:
		{
			DBG_ERR("port %04x is not support!\r\n", pid);
			return ISF_ERR_NOT_SUPPORT;
		}
		break;
	default:
		if ((pid <= ISF_OUT_MAX)) {
		} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
		} else if (pid == ISF_CTRL) {
		} else {
			DBG_ERR("port %04x is invalid!\r\n", pid);
			return ISF_ERR_INVALID_PORT_ID;
		}
		break;
	}
	return _isf_unit_get_param(h_isf, p_unit, pid, param, p_value);
}

ISF_RV isf_unit_set_struct(UINT32 h_isf, UINT32 unit_nport, UINT32 param, UINT32* p_struct, UINT32 size)
{
	UINT32 uid = GET_UNIT_ID(unit_nport);
	UINT32 pid = GET_PORT_ID(unit_nport);
	ISF_UNIT *p_unit = 0;
	GET_UNIT_PTR(p_unit, uid);
	if (!p_unit) {
		DBG_ERR("unit %04x is invalid!\r\n", uid);
		return ISF_ERR_INVALID_UNIT_ID;
	}
	switch (pid) {
	case ISF_PORT_NULL:
	case ISF_PORT_EOS:
	case ISF_PORT_KEEP:
		{
			DBG_ERR("port %04x is not support!\r\n", pid);
			return ISF_ERR_NOT_SUPPORT;
		}
		break;
	default:
		if ((pid <= ISF_OUT_MAX)) {
		} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
		} else if (pid == ISF_CTRL) {
		} else {
			DBG_ERR("port %04x is invalid!\r\n", pid);
			return ISF_ERR_INVALID_PORT_ID;
		}
		break;
	}
	return _isf_unit_set_struct_param(h_isf, p_unit, pid, param, p_struct, size);
}
ISF_RV isf_unit_get_struct(UINT32 h_isf, UINT32 unit_nport, UINT32 param, UINT32* p_struct, UINT32 size)
{
	UINT32 uid = GET_UNIT_ID(unit_nport);
	UINT32 pid = GET_PORT_ID(unit_nport);
	ISF_UNIT *p_unit = 0;
	GET_UNIT_PTR(p_unit, uid);
	if (!p_unit) {
		DBG_ERR("unit %04x is invalid!\r\n", uid);
		return ISF_ERR_INVALID_UNIT_ID;
	}
	switch (pid) {
	case ISF_PORT_NULL:
	case ISF_PORT_EOS:
	case ISF_PORT_KEEP:
		{
			DBG_ERR("port %04x is not support!\r\n", pid);
			return ISF_ERR_NOT_SUPPORT;
		}
		break;
	default:
		if ((pid <= ISF_OUT_MAX)) {
		} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
		} else if (pid == ISF_CTRL) {
		} else {
			DBG_ERR("port %04x is invalid!\r\n", pid);
			return ISF_ERR_INVALID_PORT_ID;
		}
		break;
	}
	return _isf_unit_get_struct_param(h_isf, p_unit, pid, param, p_struct, size);
}

///////////////////////////////////////////////////////////////////////////////
// data operation
UINT32 isf_unit_init_data(ISF_DATA *p_data, UINT32 fourcc, UINT32 version)
{
	ISF_DATA_CLASS* base = isf_data_class(fourcc);
	return _isf_unit_init_data(p_data, base, 0, version);
}
UINT32 isf_unit_new_data(UINT32 size, ISF_DATA *p_data)
{
	return _isf_unit_new_data(size, p_data);
}
ISF_RV isf_unit_add_data(ISF_DATA *p_data)
{
	return _isf_unit_add_data(p_data);
}
ISF_RV isf_unit_release_data(UINT32 unit_nport, ISF_DATA *p_data)
{
	UINT32 uid = GET_UNIT_ID(unit_nport);
	UINT32 pid = GET_PORT_ID(unit_nport);
	ISF_UNIT *p_unit = 0;
	//ISF_PORT *p_port = 0;
	GET_UNIT_PTR(p_unit, uid);
	if (!p_unit) {
		DBG_ERR("unit %04x is invalid!\r\n", uid);
		return ISF_ERR_INVALID_UNIT_ID;
	}
	switch (pid) {
	case ISF_CTRL:
	case ISF_PORT_NULL:
	case ISF_PORT_EOS:
	case ISF_PORT_KEEP:
		{
			DBG_ERR("port %04x is not support!\r\n", pid);
			return ISF_ERR_NOT_SUPPORT;
		}
		break;
	default:
		if ((pid <= ISF_OUT_MAX)) {
			//p_port = _isf_unit_out(p_unit, pid);
		} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
			//p_port = _isf_unit_in(p_unit, pid);
		} else {
			DBG_ERR("port %04x is invalid!\r\n", pid);
			return ISF_ERR_INVALID_PORT_ID;
		}
		break;
	}
	return _isf_unit_release_data_from_user(p_unit, pid, p_data);
}
BOOL isf_unit_is_allow_push(UINT32 unit_nport)
{
	UINT32 uid = GET_UNIT_ID(unit_nport);
	UINT32 pid = GET_PORT_ID(unit_nport);
	ISF_UNIT *p_unit = 0;
	ISF_PORT *p_port = 0;
	GET_UNIT_PTR(p_unit, uid);
	if (!p_unit) {
		DBG_ERR("unit %04x is invalid!\r\n", uid);
		return ISF_ERR_INVALID_UNIT_ID;
	}
	switch (pid) {
	case ISF_CTRL:
	case ISF_PORT_NULL:
	case ISF_PORT_EOS:
	case ISF_PORT_KEEP:
		{
			DBG_ERR("port %04x is not support!\r\n", pid);
			return ISF_ERR_NOT_SUPPORT;
		}
		break;
	default:
		if ((pid <= ISF_OUT_MAX)) {
			p_port = _isf_unit_out(p_unit, pid);
		} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
			p_port = _isf_unit_in(p_unit, pid);
		} else {
			DBG_ERR("port %04x is invalid!\r\n", pid);
			return ISF_ERR_INVALID_PORT_ID;
		}
		break;
	}
	return _isf_unit_is_allow_push(p_port);
}
ISF_RV isf_unit_push_data(UINT32 unit_nport, ISF_DATA *p_data, INT32 wait_ms)
{
	UINT32 uid = GET_UNIT_ID(unit_nport);
	UINT32 pid = GET_PORT_ID(unit_nport);
	ISF_UNIT *p_unit = 0;
	//ISF_PORT *p_port = 0;
	GET_UNIT_PTR(p_unit, uid);
	if (!p_unit) {
		DBG_ERR("unit %04x is invalid!\r\n", uid);
		return ISF_ERR_INVALID_UNIT_ID;
	}
	switch (pid) {
	case ISF_CTRL:
	case ISF_PORT_NULL:
	case ISF_PORT_EOS:
	case ISF_PORT_KEEP:
		{
			DBG_ERR("port %04x is not support!\r\n", pid);
			return ISF_ERR_NOT_SUPPORT;
		}
		break;
	default:
		if ((pid <= ISF_OUT_MAX)) {
			//p_port = _isf_unit_out(p_unit, pid);
		} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
			//p_port = _isf_unit_in(p_unit, pid);
		} else {
			DBG_ERR("port %04x is invalid!\r\n", pid);
			return ISF_ERR_INVALID_PORT_ID;
		}
		break;
	}
	return _isf_unit_push_data_from_user(p_unit, pid, p_data, wait_ms);
}
BOOL isf_unit_is_allow_pull(UINT32 unit_nport)
{
	UINT32 uid = GET_UNIT_ID(unit_nport);
	UINT32 pid = GET_PORT_ID(unit_nport);
	ISF_UNIT *p_unit = 0;
	ISF_PORT *p_port = 0;
	GET_UNIT_PTR(p_unit, uid);
	if (!p_unit) {
		DBG_ERR("unit %04x is invalid!\r\n", uid);
		return ISF_ERR_INVALID_UNIT_ID;
	}
	switch (pid) {
	case ISF_CTRL:
	case ISF_PORT_NULL:
	case ISF_PORT_EOS:
	case ISF_PORT_KEEP:
		{
			DBG_ERR("port %04x is not support!\r\n", pid);
			return ISF_ERR_NOT_SUPPORT;
		}
		break;
	default:
		if ((pid <= ISF_OUT_MAX)) {
			p_port = _isf_unit_out(p_unit, pid);
		} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
			p_port = _isf_unit_in(p_unit, pid);
		} else {
			DBG_ERR("port %04x is invalid!\r\n", pid);
			return ISF_ERR_INVALID_PORT_ID;
		}
		break;
	}
	return _isf_unit_is_allow_pull(p_port);
}
ISF_RV isf_unit_pull_data(UINT32 unit_nport, ISF_DATA *p_data, INT32 wait_ms)
{
	UINT32 uid = GET_UNIT_ID(unit_nport);
	UINT32 pid = GET_PORT_ID(unit_nport);
	ISF_UNIT *p_unit = 0;
	//ISF_PORT *p_port = 0;
	GET_UNIT_PTR(p_unit, uid);
	if (!p_unit) {
		DBG_ERR("unit %04x is invalid!\r\n", uid);
		return ISF_ERR_INVALID_UNIT_ID;
	}
	switch (pid) {
	case ISF_CTRL:
	case ISF_PORT_NULL:
	case ISF_PORT_EOS:
	case ISF_PORT_KEEP:
		{
			DBG_ERR("port %04x is not support!\r\n", pid);
			return ISF_ERR_NOT_SUPPORT;
		}
		break;
	default:
		if ((pid <= ISF_OUT_MAX)) {
		} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
		} else {
			DBG_ERR("port %04x is invalid!\r\n", pid);
			return ISF_ERR_INVALID_PORT_ID;
		}
		break;
	}
	return _isf_unit_pull_data_from_user(p_unit, pid, p_data, wait_ms);
}
BOOL isf_unit_is_allow_notify(UINT32 unit_nport)
{
	UINT32 uid = GET_UNIT_ID(unit_nport);
	UINT32 pid = GET_PORT_ID(unit_nport);
	ISF_UNIT *p_unit = 0;
	ISF_PORT *p_port = 0;
	GET_UNIT_PTR(p_unit, uid);
	if (!p_unit) {
		DBG_ERR("unit %04x is invalid!\r\n", uid);
		return ISF_ERR_INVALID_UNIT_ID;
	}
	switch (pid) {
	case ISF_CTRL:
	case ISF_PORT_NULL:
	case ISF_PORT_EOS:
	case ISF_PORT_KEEP:
		{
			DBG_ERR("port %04x is not support!\r\n", pid);
			return ISF_ERR_NOT_SUPPORT;
		}
		break;
	default:
		if ((pid <= ISF_OUT_MAX)) {
			p_port = _isf_unit_out(p_unit, pid);
		} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
			p_port = _isf_unit_in(p_unit, pid);
		} else {
			DBG_ERR("port %04x is invalid!\r\n", pid);
			return ISF_ERR_INVALID_PORT_ID;
		}
		break;
	}
	return _isf_unit_is_allow_notify(p_port);
}
ISF_RV isf_unit_notify_data(UINT32 unit_nport, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	UINT32 uid = GET_UNIT_ID(unit_nport);
	UINT32 pid = GET_PORT_ID(unit_nport);
	ISF_UNIT *p_unit = 0;
	ISF_PORT *p_port = 0;
	GET_UNIT_PTR(p_unit, uid);
	if (!p_unit) {
		DBG_ERR("unit %04x is invalid!\r\n", uid);
		return ISF_ERR_INVALID_UNIT_ID;
	}
	switch (pid) {
	case ISF_CTRL:
	case ISF_PORT_NULL:
	case ISF_PORT_EOS:
	case ISF_PORT_KEEP:
		{
			DBG_ERR("port %04x is not support!\r\n", pid);
			return ISF_ERR_NOT_SUPPORT;
		}
		break;
	default:
		if ((pid <= ISF_OUT_MAX)) {
			p_port = _isf_unit_out(p_unit, pid);
		} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
			p_port = _isf_unit_in(p_unit, pid);
		} else {
			DBG_ERR("port %04x is invalid!\r\n", pid);
			return ISF_ERR_INVALID_PORT_ID;
		}
		break;
	}
	return _isf_unit_notify(p_port, p_syncdata);
}
ISF_RV isf_unit_send_event(UINT32 unit_nport, UINT32 event, UINT32 param1, UINT32 param2, UINT32 param3)
{
	UINT32 uid = GET_UNIT_ID(unit_nport);
	UINT32 pid = GET_PORT_ID(unit_nport);
	ISF_UNIT *p_unit = 0;
	//ISF_PORT *p_port = 0;
	GET_UNIT_PTR(p_unit, uid);
	if (!p_unit) {
		DBG_ERR("unit %04x is invalid!\r\n", uid);
		return ISF_ERR_INVALID_UNIT_ID;
	}
	switch (pid) {
	case ISF_CTRL:
	case ISF_PORT_NULL:
	case ISF_PORT_EOS:
	case ISF_PORT_KEEP:
		{
			DBG_ERR("port %04x is not support!\r\n", pid);
			return ISF_ERR_NOT_SUPPORT;
		}
		break;
	default:
		if ((pid <= ISF_OUT_MAX)) {
		} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
		} else {
			DBG_ERR("port %04x is invalid!\r\n", pid);
			return ISF_ERR_INVALID_PORT_ID;
		}
		break;
	}
	return _isf_unit_send_event(p_unit, pid, event, param1, param2, param3);
}


///////////////////////////////////////////////////////////////////////////////
// stream operation

ISF_RV isf_unit_set_bind(UINT32 h_isf, UINT32 this_oport, UINT32 dest_iport)
{
	ISF_UNIT *p_srcunit = 0;
	UINT32 src_pid = 0;
	ISF_UNIT *p_destunit = 0;
	UINT32 dest_pid = 0;
	{
		UINT32 uid = GET_UNIT_ID(this_oport);
		UINT32 pid = GET_PORT_ID(this_oport);
		ISF_UNIT *p_unit = 0;
		GET_UNIT_PTR(p_unit, uid);
		switch (pid) {
		case ISF_CTRL:
		case ISF_PORT_NULL:
		case ISF_PORT_EOS:
		case ISF_PORT_KEEP:
			{
				DBG_ERR("port %04x is not support!\r\n", pid);
				return ISF_ERR_NOT_SUPPORT;
			}
			break;
		default:
			if ((pid <= ISF_OUT_MAX)) {
				if (!p_unit) {
					DBG_ERR("unit %04x is invalid!\r\n", uid);
					return ISF_ERR_INVALID_UNIT_ID;
				}
			} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
				DBG_ERR("port %04x is not support!\r\n", pid);
				return ISF_ERR_NOT_SUPPORT;
			} else if ((pid >= ISF_ROOT_BASE) && (pid <= ISF_ROOT_MAX)) {
			} else {
				DBG_ERR("port %04x is invalid!\r\n", pid);
				return ISF_ERR_INVALID_PORT_ID;
			}
			break;
		}
		p_srcunit = p_unit;
		src_pid = pid;
	}
	{
		UINT32 uid = GET_UNIT_ID(dest_iport);
		UINT32 pid = GET_PORT_ID(dest_iport);
		ISF_UNIT *p_unit = 0;
		//ISF_PORT *p_port = 0;
		GET_UNIT_PTR(p_unit, uid);
		switch (pid) {
		case ISF_CTRL:
			{
				DBG_ERR("port %04x is not support!\r\n", pid);
				return ISF_ERR_NOT_SUPPORT;
			}
			break;
		default:
			if ((pid <= ISF_OUT_MAX)) {
				DBG_ERR("port %04x is not support!\r\n", pid);
				return ISF_ERR_NOT_SUPPORT;
			} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
				if (!p_unit) {
					DBG_ERR("unit %04x is invalid!\r\n", uid);
					return ISF_ERR_INVALID_UNIT_ID;
				}
			} else if ((pid == ISF_PORT_NULL) || (pid == ISF_PORT_EOS) || (pid == ISF_PORT_KEEP)) {
				//p_port = (ISF_PORT *)pid;
				if(pid == ISF_PORT_NULL) {
					p_unit = NULL;
				}
			} else {
				DBG_ERR("port %04x is invalid!\r\n", pid);
				return ISF_ERR_INVALID_PORT_ID;
			}
			break;
		}
		p_destunit = p_unit;
		dest_pid = pid;
	}
	return _isf_unit_set_bind(h_isf, p_srcunit, src_pid, p_destunit, dest_pid);
}

ISF_RV isf_unit_get_bind(UINT32 h_isf, UINT32 this_nport, UINT32* dest_nport)
{
	ISF_RV r = ISF_OK;
	ISF_UNIT *p_thisunit = 0;
	UINT32 this_pid = 0;
	ISF_UNIT *p_destunit = 0;
	UINT32 dest_pid = 0;
	ISF_UNIT *p_srcunit = 0;
	UINT32 src_pid = 0;
	if(!dest_nport) {
		return ISF_ERR_NULL_POINTER;
	}
	{
		UINT32 uid = GET_UNIT_ID(this_nport);
		UINT32 pid = GET_PORT_ID(this_nport);
		ISF_UNIT *p_unit = 0;
		GET_UNIT_PTR(p_unit, uid);
		switch (pid) {
		case ISF_CTRL:
		case ISF_PORT_NULL:
		case ISF_PORT_EOS:
		case ISF_PORT_KEEP:
			{
				DBG_ERR("port %04x is not support!\r\n", pid);
				return ISF_ERR_NOT_SUPPORT;
			}
			break;
		default:
			if ((pid <= ISF_OUT_MAX)) {
				if (!p_unit) {
					DBG_ERR("unit %04x is invalid!\r\n", uid);
					return ISF_ERR_INVALID_UNIT_ID;
				}
			} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
				if (!p_unit) {
					DBG_ERR("unit %04x is invalid!\r\n", uid);
					return ISF_ERR_INVALID_UNIT_ID;
				}
			} else if ((pid >= ISF_ROOT_BASE) && (pid <= ISF_ROOT_MAX)) {
				DBG_ERR("port %04x is invalid!\r\n", pid);
				return ISF_ERR_INVALID_PORT_ID;
			} else {
				DBG_ERR("port %04x is invalid!\r\n", pid);
				return ISF_ERR_INVALID_PORT_ID;
			}
			break;
		}
		p_thisunit = p_unit;
		this_pid = pid;

		if ((pid < ISF_OUT_BASE + ISF_OUT_MAX)) {
			UINT32 dest_uid = 0;
			r = _isf_unit_get_bind(h_isf, p_thisunit, this_pid, &p_destunit, &dest_pid); //get dest & iport
			if(r != ISF_OK) {
				dest_nport[0] = 0;
				return r;
			}
			if(!p_destunit) {
				dest_nport[0] = 0;
				return ISF_OK; //not bind
			}
			SEEK_UNIT_ID(p_destunit, dest_uid);
			dest_nport[0] = MAKE_UNITPORT(dest_uid, dest_pid);
		} else if ((pid >= ISF_IN_BASE) && (pid < ISF_IN_BASE + ISF_IN_MAX)) {
			UINT32 src_uid = 0;
			r = _isf_unit_get_bind(h_isf, p_thisunit, this_pid, &p_srcunit, &src_pid); //get src & oport
			if(r != ISF_OK) {
				dest_nport[0] = 0;
				return r;
			}
			if(!p_srcunit) {
				dest_nport[0] = 0;
				return ISF_OK; //not bind
			}
			SEEK_UNIT_ID(p_srcunit, src_uid);
			dest_nport[0] = MAKE_UNITPORT(src_uid, src_pid);
		}
	}
	return ISF_OK;
}

//0:OPEN, 1:START, 2:STOP, 3:CLOSE
ISF_RV isf_unit_set_state(UINT32 h_isf, UINT32 this_oport, UINT32 state)
{
	ISF_UNIT *p_srcunit = 0;
	UINT32 src_pid = 0;
	ISF_PORT_CMD cmd = 0;
	{
		UINT32 uid = GET_UNIT_ID(this_oport);
		UINT32 pid = GET_PORT_ID(this_oport);
		ISF_UNIT *p_unit = 0;
		GET_UNIT_PTR(p_unit, uid);
		switch (pid) {
		case ISF_CTRL:
		case ISF_PORT_NULL:
		case ISF_PORT_EOS:
		case ISF_PORT_KEEP:
			{
				DBG_ERR("port %04x is not support!\r\n", pid);
				return ISF_ERR_NOT_SUPPORT;
			}
			break;
		default:
			if ((pid <= ISF_OUT_MAX)) {
				if (!p_unit) {
					DBG_ERR("unit %04x is invalid!\r\n", uid);
					return ISF_ERR_INVALID_UNIT_ID;
				}
			} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
				DBG_ERR("port %04x is not support!\r\n", pid);
				return ISF_ERR_NOT_SUPPORT;
			} else if ((pid >= ISF_ROOT_BASE) && (pid <= ISF_ROOT_MAX)) {
			} else {
				DBG_ERR("port %04x is invalid!\r\n", pid);
				return ISF_ERR_INVALID_PORT_ID;
			}
			break;
		}
		p_srcunit = p_unit;
		src_pid = pid;
	}
	switch(state) {
	case 0: cmd = ISF_PORT_CMD_OPEN; break;
	case 1: cmd = ISF_PORT_CMD_START; break;
	case 2: cmd = ISF_PORT_CMD_STOP; break;
	case 3: cmd = ISF_PORT_CMD_CLOSE; break;
	}
	return _isf_unit_set_state(h_isf, p_srcunit, src_pid, cmd);
}

ISF_RV isf_unit_get_state(UINT32 h_isf, UINT32 this_oport, UINT32* state)
{
	ISF_RV r;
	ISF_UNIT *p_srcunit = 0;
	UINT32 src_pid = 0;
	ISF_PORT_STATE isf_state = ISF_PORT_STATE_OFF;
	if(!state) {
		return ISF_ERR_NULL_POINTER;
	}
	{
		UINT32 uid = GET_UNIT_ID(this_oport);
		UINT32 pid = GET_PORT_ID(this_oport);
		ISF_UNIT *p_unit = 0;
		GET_UNIT_PTR(p_unit, uid);
		switch (pid) {
		case ISF_CTRL:
		case ISF_PORT_NULL:
		case ISF_PORT_EOS:
		case ISF_PORT_KEEP:
			{
				DBG_ERR("port %04x is not support!\r\n", pid);
				return ISF_ERR_NOT_SUPPORT;
			}
			break;
		default:
			if ((pid <= ISF_OUT_MAX)) {
				if (!p_unit) {
					DBG_ERR("unit %04x is invalid!\r\n", uid);
					return ISF_ERR_INVALID_UNIT_ID;
				}
			} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
				DBG_ERR("port %04x is not support!\r\n", pid);
				return ISF_ERR_NOT_SUPPORT;
			} else if ((pid >= ISF_ROOT_BASE) && (pid <= ISF_ROOT_MAX)) {
				DBG_ERR("port %04x is invalid!\r\n", pid);
				return ISF_ERR_INVALID_PORT_ID;
			} else {
				DBG_ERR("port %04x is invalid!\r\n", pid);
				return ISF_ERR_INVALID_PORT_ID;
			}
			break;
		}
		p_srcunit = p_unit;
		src_pid = pid;
	}
	r = _isf_unit_get_state(h_isf, p_srcunit, src_pid, &isf_state);
	switch(isf_state) {
	case ISF_PORT_STATE_OFF: state[0] = 0; break;
	case ISF_PORT_STATE_READY: state[0] = 1; break;
	case ISF_PORT_STATE_RUN: state[0] = 2; break;
	default:
		break;
	}
	return r;
}

#if 0
ISF_RV isf_stream_get_output(UINT32 unit_oport, UINT32* dest_iport, ISF_PORT_STATE* p_state)
{
	ISF_UNIT *p_srcunit = 0;
	UINT32 src_pid = 0;
	ISF_PORT *p_destport = 0;
	UINT32 dest_pid = 0;
	{
		UINT32 uid = GET_UNIT_ID(unit_oport);
		UINT32 pid = GET_PORT_ID(unit_oport);
		ISF_UNIT *p_unit = 0;
		ISF_PORT *p_port = 0;
		GET_UNIT_PTR(p_unit, uid);
		if (!p_unit) {
			DBG_ERR("unit %04x is invalid!\r\n", uid);
			return ISF_ERR_INVALID_UNIT_ID;
		}
		switch (pid) {
		case ISF_CTRL:
		case ISF_PORT_NULL:
		case ISF_PORT_EOS:
		case ISF_PORT_KEEP:
			{
				DBG_ERR("port %04x is not support!\r\n", pid);
				return ISF_ERR_NOT_SUPPORT;
			}
			break;
		default:
			if ((pid <= ISF_OUT_MAX)) {
				p_port = _isf_unit_out(p_unit, pid);
			} else if ((pid >= ISF_IN_BASE) && (pid <= ISF_IN_MAX)) {
				DBG_ERR("port %04x is not support!\r\n", pid);
				return ISF_ERR_NOT_SUPPORT;
			} else {
				DBG_ERR("port %04x is invalid!\r\n", pid);
				return ISF_ERR_INVALID_PORT_ID;
			}
			break;
		}
		p_srcunit = p_unit;
		src_pid = pid;
	}

	_isf_stream_get_output(p_srcunit, src_pid, &p_destport, p_state);

	{
		if (p_destport == NULL) {
			dest_pid = ISF_PORT_NULL;
		} else {
			register UINT32 i;
			for (i=0;i<ISF_MAX_UNIT;i++) {
				if (_isf_unit[i] == p_destport->p_destunit) {
					dest_pid = ISF_PORT_ID(i+ISF_UNIT_BASE, p_destport->iport);
				}
			}
}
		*(dest_iport) = dest_pid;
	}
	return ISF_OK;
}
#endif

