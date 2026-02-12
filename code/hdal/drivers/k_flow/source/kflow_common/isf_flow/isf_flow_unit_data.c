#include "isf_flow_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_flow_pd
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_flow_pd_debug_level = __DBGLVL__;
//module_param_named(isf_flow_pd_debug_level, isf_flow_pd_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_flow_pd_debug_level, "flow debug level");
///////////////////////////////////////////////////////////////////////////////

#define DUMP_DATA				DISABLE
#define DUMP_DATA_NEW_FAIL		DISABLE
#define DUMP_DATA_ERROR		ENABLE

static NVTMPP_MODULE    g_module = MAKE_NVTMPP_MODULE('u', 's', 'e', 'r', 0, 0, 0, 0);
static NVTMPP_DDR       g_ddr = NVTMPP_DDR_1;

static ISF_RV _isf_user_lock(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data) {return ISF_OK;}
static ISF_RV _isf_user_unlock(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data) {return ISF_OK;}
//vfrm common blk, protected by nvtmpp
static ISF_DATA_CLASS _isf_vfrm_common_base = {0};
//vfrm user blk, no protected
static ISF_DATA_CLASS _isf_vfrm_base = {
    .do_lock   = _isf_user_lock,
    .do_unlock = _isf_user_unlock,
};
//vstm user blk, no protected
static ISF_DATA_CLASS _isf_vstm_base = {
    .do_lock   = _isf_user_lock,
    .do_unlock = _isf_user_unlock,
};
//afrm user blk, no protected
static ISF_DATA_CLASS _isf_afrm_base = {
    .do_lock   = _isf_user_lock,
    .do_unlock = _isf_user_unlock,
};
//astm user blk, no protected
static ISF_DATA_CLASS _isf_astm_base = {
    .do_lock   = _isf_user_lock,
    .do_unlock = _isf_user_unlock,
};

static ISF_DATA_CLASS* _isf_vfrm_common_class = 0;
static ISF_DATA_CLASS* _isf_vfrm_class = 0;
static ISF_DATA_CLASS* _isf_vstm_class = 0;
static ISF_DATA_CLASS* _isf_afrm_class = 0;
static ISF_DATA_CLASS* _isf_astm_class = 0;
static ISF_DATA_CLASS* _isf_user_class = 0;


void _isf_data_reguser(void)
{
	nvtmpp_vb_add_module(g_module);
}

static ISF_RV _isf_data_verify(char *unit, char *act, ISF_DATA *p_data)
{
	ISF_RV r = ISF_OK;
	if (p_data->sign != ISF_SIGN_DATA) {
		r = ISF_ERR_INVALID_SIGN;
		goto D1_END;
	}
	if (p_data->p_class) {
		ISF_DATA_CLASS* p_thisclass = (ISF_DATA_CLASS*)(p_data->p_class);
		if(p_thisclass->do_verify) {
			r = p_thisclass->do_verify(p_data);
			if(r != ISF_OK) {
				goto D2_END;
			}
		}
	}
	return ISF_OK;
D1_END:
	DBG_ERR(" \"%s\" %s data(%08x), data verify error %d!\r\n",
			unit, act, (UINT32)p_data, r);
	//ASSERT(r == ISF_OK);
	if (!(r == ISF_OK))
		debug_dump_current_stack();
	return r;
D2_END:
	DBG_ERR(" \"%s\" %s data(%08x), data content(%d) verify error %d!\r\n",
			unit, act, (UINT32)p_data, p_data->desc[0], r);
	//ASSERT(r == ISF_OK);
	if (!(r == ISF_OK))
		debug_dump_current_stack();
	return r;
}

static UINT32 _isf_base_unit_create(ISF_UNIT *p_thisunit, ISF_PORT *p_srcport, CHAR* name, UINT32 blk_size, UINT32 blk_cnt)
{
	NVTMPP_VB_POOL pool;
	CHAR pool_name[34] = {0};
	NVTMPP_DDR ddr = NVTMPP_DDR_1;
	ISF_VDO_INFO *p_info = NULL;
	UINT32 i;

	if (!p_thisunit)
		return 0;
	strncpy(pool_name, p_thisunit->unit_name, 33);
	if (p_srcport == NULL) {  //this is ctrl
		ddr = p_thisunit->attr;
		snprintf(pool_name, 33, "%s.ctrl.%s", p_thisunit->unit_name, name);
	} else if ((((UINT32)p_srcport) & 0xffffff00) == 0xffffff00) {
		UINT32 nport = (((UINT32)p_srcport) & ~0xffffff00);
		/*if (nport == ISF_CTRL) {  //this is ctrl
			ddr = p_thisunit->attr;
			snprintf(pool_name, 33, "%s.ctrl.%s", p_thisunit->unit_name, name);
		} else*/
		if (ISF_IS_OPORT(p_thisunit, nport)) {  //this is a oport
			UINT32 i = nport - ISF_OUT_BASE;
			p_info = (ISF_VDO_INFO *)(p_thisunit->port_outinfo[i]);
			if (p_info) {
				ddr = p_info->attr;
			}
			snprintf(pool_name, 33, "%s.o[%d].%s", p_thisunit->unit_name, i, name);
		} else if (ISF_IS_IPORT(p_thisunit, nport)) {  //this is a iport
			UINT32 i = nport - ISF_IN_BASE;
			p_info = (ISF_VDO_INFO *)(p_thisunit->port_ininfo[i]);
			if (p_info) {
				ddr = p_info->attr;
			}
			snprintf(pool_name, 33, "%s.i[%d].%s", p_thisunit->unit_name, i, name);
		}
	} else { //this is a oport
		if (p_srcport != NULL) {
			if (p_thisunit != p_srcport->p_srcunit)
				return 0;
		}
		i = p_srcport->oport - ISF_OUT_BASE;
		p_info = (ISF_VDO_INFO *)(p_thisunit->port_outinfo[i]);
		if (p_info) {
			ddr = p_info->attr;
		}
		snprintf(pool_name, 33, "%s.o[%d].%s", p_thisunit->unit_name, i, name);
	}
	pool = nvtmpp_vb_create_pool(pool_name, blk_size, blk_cnt, ddr);
	if (pool == NVTMPP_VB_INVALID_POOL) {
		//gen ISF_ERR_BUFFER_CREATE log
	}
	return (UINT32)pool;
}

static ISF_RV _isf_base_unit_destory(ISF_UNIT *p_thisunit, ISF_PORT *p_srcport, UINT32 pool)
{
	NVTMPP_ER mr = NVTMPP_ER_NOBUF;
	if (p_thisunit && pool) {
		mr = nvtmpp_vb_destroy_pool(pool);
	}
	if (mr != NVTMPP_ER_OK) {
		//gen ISF_ERR_BUFFER_DESTORY log
		return ISF_ERR_BUFFER_DESTROY;
	}
	return ISF_OK;
}

ISF_DATA_CLASS* _isf_data_init_class(ISF_DATA_CLASS* p_thisclass, UINT32 base_class)
{
	ISF_DATA_CLASS* p_base;
	UINT32 null_func;

	if(!p_thisclass) {
		//using base class only
		p_base = isf_data_class(base_class);
		if(!p_base) {
			DBG_ERR("data class %08x::%08x is invalid!\r\n", (UINT32)p_thisclass, base_class);
			return NULL;
		}
		return p_base;
	}

	//using this class
	if(p_thisclass->this) {

		return p_thisclass; //it is a complete class
	}

	//fill this class by its related base class
	p_base = isf_data_class(base_class);
	if(!p_base) {
		DBG_ERR("data class %08x::%08x is invalid!\r\n", (UINT32)p_thisclass, base_class);
		return NULL;
	}

	DBG_MSG("data class %08x::%08x init!\r\n", (UINT32)p_thisclass, base_class);

	do {

		if(!p_thisclass->this) {
			p_thisclass->this = base_class;
		}
		if(!p_thisclass->do_init) {
			p_thisclass->do_init = p_base->do_init;
		}
		if(!p_thisclass->do_verify) {
			p_thisclass->do_verify = p_base->do_verify;
		}
		if(!p_thisclass->do_new) {
			p_thisclass->do_new = p_base->do_new;
		}
		if(!p_thisclass->do_lock) {
			p_thisclass->do_lock = p_base->do_lock;
		}
		if(!p_thisclass->do_unlock) {
			p_thisclass->do_unlock = p_base->do_unlock;
		}
		if(!p_thisclass->do_load) {
			p_thisclass->do_load = p_base->do_load;
		}
		if(!p_thisclass->do_save) {
			p_thisclass->do_save = p_base->do_save;
		}
		if(!p_thisclass->do_probe) {
			p_thisclass->do_probe = p_base->do_probe;
		}
		if(!p_thisclass->do_perf) {
			p_thisclass->do_perf = p_base->do_perf;
		}

		null_func = 0;

		if(!p_thisclass->do_init)null_func++;
		if(!p_thisclass->do_verify)null_func++;
		if(!p_thisclass->do_new)null_func++;
		if(!p_thisclass->do_lock)null_func++;
		if(!p_thisclass->do_unlock)null_func++;
		if(!p_thisclass->do_load)null_func++;
		if(!p_thisclass->do_save)null_func++;
		if(!p_thisclass->do_probe)null_func++;
		if(!p_thisclass->do_perf)null_func++;

		p_base = isf_data_class(p_base->base);

	} while((null_func>0) && (p_base != 0));

	if(null_func > 0) {
		DBG_WRN("data class %08x::%08x is not complete!\r\n", (UINT32)p_thisclass, base_class);
	} else {
		DBG_MSG("data class %08x::%08x is init complete!\r\n", (UINT32)p_thisclass, base_class);
	}

	return p_thisclass;
}

UINT32 _isf_unit_init_data(ISF_DATA *p_data, ISF_DATA_CLASS* p_thisclass, UINT32 base_class, UINT32 version)
{
	ISF_DATA_CLASS* p_thisclass_fill;
	//this new
	if(!p_data)
		return ISF_OK;

	p_thisclass_fill = _isf_data_init_class(p_thisclass, base_class);
	if(!p_thisclass_fill) {
		return ISF_ERR_INVALID_DATA;
	}

	memset(p_data, 0, sizeof(ISF_DATA));
	p_data->sign = ISF_SIGN_DATA;
	p_data->p_class = p_thisclass_fill;
	p_data->desc[0] = p_thisclass_fill->this;

	if(p_thisclass_fill->do_init) {
		p_thisclass_fill->do_init(p_data, version);
	}

	return ISF_OK;
}
//EXPORT_SYMBOL(_isf_unit_init_data);


static UINT32 _isf_base_unit_new(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 pool, UINT32 attr, UINT32 blk_size, ISF_DATA *p_data, UINT32 probe)
{
	NVTMPP_VB_BLK blk_id = NVTMPP_VB_INVALID_BLK;
	NVTMPP_DDR ddr = NVTMPP_DDR_1;
	UINT32 addr = 0;
	ISF_DATA_CLASS* p_thisclass;

	if (p_thisunit) {
		if (nport == ISF_CTRL) {
			ddr = p_thisunit->attr;
		} else if (ISF_IS_OPORT(p_thisunit, nport)) {
			UINT32 i = nport - ISF_OUT_BASE;
			ISF_VDO_INFO * p_info = (ISF_VDO_INFO *)(p_thisunit->port_outinfo[i]);
			if (p_info) {
				ddr = p_info->attr;
			}
		} else if (ISF_IS_IPORT(p_thisunit, nport)) {
			UINT32 i = nport - ISF_IN_BASE;
			ISF_VDO_INFO * p_info = (ISF_VDO_INFO *)(p_thisunit->port_ininfo[i]);
			if (p_info) {
				ddr = p_info->attr;
			}
		} else {
			//TBD
		}
	} else {
		ddr = g_ddr;
	}
	if (!p_data) {
		return blk_id;
	}

	p_thisclass = (ISF_DATA_CLASS*)(p_data->p_class);

	if (p_thisunit) {
		if ((pool == (UINT32)NVTMPP_VB_INVALID_POOL) && (blk_size == 0)) {
			//(blk_size == 0) meangs:
			//this ISF_DATA is not a common blk
			//it is already manual created by unit (address is pass from attr)

			//just do_new() to show debug msg
			addr = attr;
			p_data->sign = ISF_SIGN_DATA;
			_isf_unit_debug_new(p_thisunit, nport, p_data, addr, blk_size, probe); //do probe
			return addr;
		}
	}

	if(p_thisclass->do_new) {
		addr = p_thisclass->do_new(p_data, p_thisunit, pool, blk_size, ddr);
	} else {
#if (DUMP_DATA_ERROR == ENABLE)
		DBG_ERR("\"%8s\" not support new!\r\n", (p_thisunit) ? (p_thisunit->unit_name) : ("user"));
#endif
	}

	if (addr == 0) {
#if (DUMP_DATA_NEW_FAIL == ENABLE)
		DBG_ERR("\"%8s\" fail, size=%0x\r\n", (p_thisunit) ? (p_thisunit->unit_name) : ("user"));
#endif
		_isf_unit_debug_new(p_thisunit, nport, p_data, addr, blk_size, probe); //do probe
		return (UINT32)addr;
	}
#if (DUMP_DATA == ENABLE)
	blk_id = p_data->h_data;
	DBG_IND("\"%8s\" blk=0x%x, size=%0x, addr=0x%x\r\n", (p_thisunit) ? (p_thisunit->unit_name) : ("user"), (UINT32)blk_id, addr, blk_size);
#endif
	_isf_unit_debug_new(p_thisunit, nport, p_data, addr, blk_size, probe); //do probe
	return addr;
}


static ISF_RV _isf_data_lock(ISF_UNIT *p_thisunit, ISF_DATA *p_data)
{
	ISF_RV r = ISF_OK;
	ISF_DATA_CLASS* p_thisclass;
	if (!p_data) {
		return ISF_ERR_NULL_POINTER;
	}

	p_thisclass = (ISF_DATA_CLASS*)(p_data->p_class);

#if (DUMP_DATA == ENABLE)
	DBG_IND("\"%8s\" blk=%08x\r\n", (p_thisunit) ? (p_thisunit->unit_name) : ("user"), (UINT32)p_data->h_data);
#endif
	if(p_thisclass->do_lock) {
		r = p_thisclass->do_lock(p_data, p_thisunit, p_data->h_data);
	} else {
#if (DUMP_DATA_ERROR == ENABLE)
		DBG_ERR("\"%8s\" not support lock!\r\n", (p_thisunit) ? (p_thisunit->unit_name) : ("user"));
#endif
		r = ISF_ERR_BUFFER_ADD;
	}
	if (ISF_OK != r) {
#if (DUMP_DATA_ERROR == ENABLE)
		DBG_ERR("\"%8s\" blk=%08x fail %d\r\n", (p_thisunit) ? (p_thisunit->unit_name) : ("user"), p_data->h_data, r);
#endif
		r = ISF_ERR_BUFFER_ADD;
	}
	return r;
}

static ISF_RV _isf_data_unlock(ISF_UNIT *p_thisunit, ISF_DATA *p_data)
{
	ISF_RV r = ISF_OK;
	ISF_DATA_CLASS* p_thisclass;
	if (!p_data) {
		return ISF_ERR_NULL_POINTER;
	}

	p_thisclass = (ISF_DATA_CLASS*)(p_data->p_class);

#if (DUMP_DATA == ENABLE)
	DBG_IND("\"%8s\" blk=%08x\r\n", (p_thisunit) ? (p_thisunit->unit_name) : ("user"), (UINT32)p_data->h_data);
#endif
	if(p_thisclass->do_lock) {
		r = p_thisclass->do_unlock(p_data, p_thisunit, p_data->h_data);
	} else {
#if (DUMP_DATA_ERROR == ENABLE)
		DBG_ERR("\"%8s\" not support unlock!\r\n", (p_thisunit) ? (p_thisunit->unit_name) : ("user"));
#endif
		r = ISF_ERR_BUFFER_RELEASE;
	}
	if (ISF_OK != r) {
#if (DUMP_DATA_ERROR == ENABLE)
		DBG_ERR("\"%8s\" blk=%08x fail %d\r\n", (p_thisunit) ? (p_thisunit->unit_name) : ("user"), p_data->h_data, r);
#endif
		r = ISF_ERR_BUFFER_RELEASE;
	}
	return r;
}


UINT32 _isf_unit_new_data(UINT32 blk_size, ISF_DATA *p_data)
{
	//this new
	return _isf_base_unit_new(0, 0, NVTMPP_VB_INVALID_POOL, g_ddr, blk_size, p_data, 0);
}
//EXPORT_SYMBOL(_isf_unit_new_data);

static ISF_RV _isf_base_unit_add(ISF_UNIT *p_thisunit, UINT32 nport, ISF_DATA *p_data, UINT32 probe)
{
	ISF_RV r = ISF_OK;
	if (!p_data) {
		return ISF_ERR_NULL_POINTER;
	}
	r = _isf_data_verify((p_thisunit) ? (p_thisunit->unit_name) : ("user"), "add", p_data);
	_isf_unit_check_return(r);
	if (r != ISF_OK) {
		return r;
	}

	r = _isf_data_lock(p_thisunit, p_data); //this lock
	if (r != ISF_OK) {
		_isf_unit_debug_add(p_thisunit, nport, p_data, probe, r); //do probe
		return r;
	}

	_isf_unit_debug_add(p_thisunit, nport, p_data, probe, r); //do probe
	return r;
}

ISF_RV _isf_unit_add_data(ISF_DATA *p_data)
{
	return _isf_base_unit_add(0, 0, p_data, ISF_IN_PROBE_PUSH);
}
//EXPORT_SYMBOL(_isf_unit_add_data);

static ISF_RV _isf_base_unit_release(ISF_UNIT *p_thisunit, UINT32 nport, ISF_DATA *p_data, UINT32 probe)
{
	ISF_RV r = ISF_OK;
	if (!p_data) {
		return ISF_ERR_NULL_POINTER;
	}
	r = _isf_data_verify((p_thisunit) ? (p_thisunit->unit_name) : ("user"), "release", p_data);
	_isf_unit_check_return(r);
	if (r != ISF_OK) {
		return r;
	}

	_isf_unit_debug_release(p_thisunit, nport, p_data, probe, r); //do probe

	r = _isf_data_unlock(p_thisunit, p_data); //this release
	return r;
}

ISF_RV _isf_unit_release_data_from_user(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data)
{
	if(p_data->sign != ISF_SIGN_DATA) {
		return ISF_ERR_INVALID_SIGN;
	}
	if(p_data->desc[0] == 0) {
		return ISF_ERR_INVALID_DATA;
	}
	if(p_data->p_class == 0) { //this should be a user generated data
		ISF_DATA_CLASS* p_thisclass_fill;
		if(p_data->desc[0] == MAKEFOURCC('V','F','R','M')) {
			if(p_data->h_data == 0) {
				return ISF_ERR_INVALID_DATA;
			} if(p_data->h_data == (UINT32)-1) {
				return ISF_ERR_INVALID_DATA;
			} else if(p_data->h_data == (UINT32)-2) {
				//vfrm user blk, no protected
				if(_isf_vfrm_class == 0) {
					_isf_vfrm_class = _isf_data_init_class(&_isf_vfrm_base, p_data->desc[0]);
				}
				p_thisclass_fill = _isf_vfrm_class;
			} else {
				//vfrm common blk, protected by nvtmpp
				if(_isf_vfrm_common_class == 0) {
					_isf_vfrm_common_class = _isf_data_init_class(&_isf_vfrm_common_base, p_data->desc[0]);
				}
				p_thisclass_fill = _isf_vfrm_common_class;
			}
		} else if(p_data->desc[0] == MAKEFOURCC('V','S','T','M')) {
			//vstm user blk, no protected
			if(_isf_vstm_class == 0) {
				_isf_vstm_class = _isf_data_init_class(&_isf_vstm_base, p_data->desc[0]);
			}
			p_thisclass_fill = _isf_vstm_class;
		} else if(p_data->desc[0] == MAKEFOURCC('A','F','R','M')) {
			//afrm user blk, no protected
			if(_isf_afrm_class == 0) {
				_isf_afrm_class = _isf_data_init_class(&_isf_afrm_base, p_data->desc[0]);
			}
			p_thisclass_fill = _isf_afrm_class;
		} else if(p_data->desc[0] == MAKEFOURCC('A','S','T','M')) {
			//astm user blk, no protected
			if(_isf_astm_class == 0) {
				_isf_astm_class = _isf_data_init_class(&_isf_astm_base, p_data->desc[0]);
			}
			p_thisclass_fill = _isf_astm_class;
		} else {
			//it should be a registered private blk:
			//..maybe vstm private blk, protected by videoenc
			//..maybe afrm private blk, protected by audiocap or audiodec
			//..maybe astm private blk, protected by audioenc
			if(p_data->h_data == 0) {
				return ISF_ERR_INVALID_DATA;
			}
			_isf_user_class = _isf_data_init_class(NULL, p_data->desc[0]); //NULL to lookup registered class
			p_thisclass_fill = _isf_user_class;
		}
		if(!p_thisclass_fill) {
			return ISF_ERR_INVALID_DATA;
		}
		p_data->p_class = p_thisclass_fill;
	}

	if(p_data->desc[0] == MAKEFOURCC('V','F','R','M')) {
		return _isf_base_unit_release(NULL, 0, p_data, ISF_USER_PROBE_REL);
	}

	return _isf_base_unit_release(NULL, 0, p_data, ISF_USER_PROBE_REL);
}
//EXPORT_SYMBOL(_isf_unit_release_data);

BOOL _isf_base_unit_is_allow_push(ISF_UNIT *p_thisunit, ISF_PORT *p_destport)
{
	if (!(p_destport && (p_destport->connecttype & ISF_CONNECT_PUSH))) {
		//DBG_ERR("not allow PUSH!!\r\n");
		return FALSE;
	}
	if (p_destport->flag & ISF_PORT_FLAG_PAUSE_PUSH) {
		return FALSE;
	}
	return TRUE;
}

BOOL _isf_unit_is_allow_push(ISF_PORT *p_destport)
{
	return _isf_base_unit_is_allow_push(NULL, p_destport);
}
//EXPORT_SYMBOL(_isf_unit_is_allow_push);

static ISF_RV _isf_base_unit_push(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 wait_ms)
{
	ISF_RV r = ISF_OK;
	ISF_RV pr = ISF_OK;
	UINT32 i = oport - ISF_OUT_BASE;
	ISF_PORT *p_destport;
	ISF_UNIT *p_nextunit;
	ISF_RV (*p_do_push)(struct _ISF_PORT_* p_port, ISF_DATA *p_data, INT32 wait_ms);
	UINT32 iport;
	if (!p_data) {
		return ISF_ERR_NULL_POINTER;
	}
	if(!p_thisunit){
		return ISF_ERR_NULL_POINTER;
	}
	p_destport = p_thisunit->port_out[i];
	if (p_destport) {
		p_do_push = p_destport->do_push;
		p_nextunit = p_destport->p_destunit;
	}
	r = _isf_data_verify(p_thisunit->unit_name, "Push", p_data);
	_isf_unit_check_return(r);
	if (r != ISF_OK) {
		return r;
	}

	if (!p_destport) { //push to null-port
		goto push_fail;
	} else if (p_do_push && p_nextunit) { //push to dest-port and lock/unlock
#if (DUMP_DATA == ENABLE)
		UINT64 c_module;
		c_module = p_thisunit->unit_module;
#endif
		iport = p_destport->iport;
#if (DUMP_DATA == ENABLE)
		DBG_IND("\"%8s\" blk=%08x\r\n", (CHAR *)&c_module, (UINT32)p_data->h_data);
#endif
#if 0
		if ((p_destport) && (p_destport->attr & ISF_PORT_ATTR_READYSRCBUF)) {
			r = ISF_OK;    //skip
		} else
#endif
			r = _isf_data_lock(p_nextunit, p_data); //dest lock
		if (ISF_OK != r) {
			return r;
		}

        _isf_unit_debug_prepush(p_nextunit, iport, p_data); //do probe
		p_data->src = oport; //mark src's oport as push from binding unit
		pr = p_do_push(p_destport, p_data, wait_ms); //do push
        _isf_unit_debug_postpush(p_thisunit, oport, p_data); //do probe

		//if ((p_destport) && (p_destport->attr & ISF_PORT_ATTR_KEEPSRCBUF)) {
		//	r = ISF_OK;    //skip
		//} else {
			r = _isf_data_unlock(p_thisunit, p_data);    //this unlock
		//}
		if (ISF_OK == r) {
			r = pr;
		}

		if ((r != ISF_OK) && (r != ISF_ERR_QUEUE_FULL) && (r != ISF_ERR_WAIT_TIMEOUT) && (r != ISF_ERR_NOT_OPEN_YET) && (r != ISF_ERR_NOT_START)) {
			DBG_ERR(" \"%s\".out[%d] push to \"%s\".in[%d] is failed (%d)\r\n",
				p_thisunit->unit_name, oport - ISF_OUT_BASE, p_nextunit->unit_name, iport - ISF_IN_BASE, r);
		}
		return r;
	}
push_fail:
	if (!p_destport) {
		r = _isf_data_unlock(p_thisunit, p_data);    //this unlock
		if (r != ISF_OK) {
			DBG_ERR(" \"%s\" push to (unknown), unlock failed (%d)\r\n", p_thisunit->unit_name, r);
		}
		return ISF_ERR_NOT_CONNECT_YET;
	}
	if (!p_destport->do_push || !p_destport->p_destunit) {
		r = _isf_data_unlock(p_thisunit, p_data);    //this unlock
		if (r != ISF_OK) {
			DBG_ERR(" \"%s\" push to (unknown), unlock failed (%d)\r\n", p_thisunit->unit_name, r);
		}
		return ISF_ERR_NOT_CONNECT_YET;
	}
	//if (p_destport->p_destunit) {
	//	DBG_ERR(" port \"%s\".in[%d] is not support!\r\n",
	//			p_destport->p_destunit->unit_name, p_destport->iport - ISF_IN_BASE);
	//}
	return ISF_ERR_FAILED;
}

static ISF_RV _isf_base_unit_push_from_user(ISF_UNIT *p_thisunit, UINT32 iport, ISF_DATA *p_data, INT32 wait_ms)
{
	ISF_RV r = ISF_OK;
	ISF_RV pr = ISF_OK;
	ISF_PORT_CAPS *p_destportcaps;
	ISF_PORT *p_destport;
	ISF_RV (*p_do_push)(struct _ISF_PORT_* p_port, ISF_DATA *p_data, INT32 wait_ms);
	if (!p_data) {
		return ISF_ERR_NULL_POINTER;
	}
	if(!p_thisunit){
		return ISF_ERR_NULL_POINTER;
	}
	r = _isf_data_verify("user", "push", p_data);
	_isf_unit_check_return(r);
	if (r != ISF_OK) {
		return r;
	}

	p_destport = p_thisunit->port_in[iport - ISF_IN_BASE];
	p_destportcaps = p_thisunit->port_incaps[iport - ISF_IN_BASE];
	if (p_destportcaps) {
		p_do_push = p_destportcaps->do_push;
	}
	if (!p_destport || !p_destportcaps) { //push to null-port
		goto push2_fail;
	} else if (p_do_push) { //push to dest-port and lock/unlock
#if (DUMP_DATA == ENABLE)
		UINT64 c_module;
		c_module = g_module;
#endif
#if (DUMP_DATA == ENABLE)
		DBG_IND("\"%8s\" blk=%08x\r\n", (CHAR *)&c_module, (UINT32)p_data->h_data);
#endif
		r = _isf_data_lock(p_thisunit, p_data); //dest lock
		if (ISF_OK != r) {
			return r;
		}

		_isf_unit_debug_prepush(p_thisunit, iport, p_data); //do probe
		p_data->src = ISF_CTRL; //mark ISF_CTRL as push from user
		pr = p_do_push(p_destport, p_data, wait_ms); //do push
		_isf_unit_debug_postpush(NULL, 0, p_data); //do probe
		r = _isf_data_unlock(NULL, p_data);    //this unlock
		if (ISF_OK == r) {
			r = pr;
		}

		if ((r != ISF_OK) && (r != ISF_ERR_QUEUE_FULL) && (r != ISF_ERR_WAIT_TIMEOUT) && (r != ISF_ERR_NOT_OPEN_YET) && (r != ISF_ERR_NOT_START)) {
			DBG_ERR(" [user] push to \"%s\".in[%d] is failed (%d)\r\n", p_thisunit->unit_name, iport - ISF_IN_BASE, r);
		}

		return r;
	}
push2_fail:
	if (!p_destport || !p_destportcaps) {
		r = _isf_data_unlock(NULL, p_data);    //this unlock
		if (r != ISF_OK) {
			DBG_ERR(" [user] push to \"%s\".in[%d], unlock failed (%d)\r\n", p_thisunit->unit_name, iport - ISF_IN_BASE, r);
		}
		return ISF_ERR_NOT_SUPPORT;
	}
	if (!p_destportcaps->do_push) {
		r = _isf_data_unlock(NULL, p_data);    //this unlock
		if (r != ISF_OK) {
			DBG_ERR(" [user] push to \"%s\".in[%d], unlock failed (%d)\r\n", p_thisunit->unit_name, iport - ISF_IN_BASE, r);
		}
		return ISF_ERR_NOT_SUPPORT;
	}
	//if (p_destport->p_destunit)
	//	DBG_ERR(" port \"%s\".in[%d] is not support!\r\n",
	//			p_destport->p_destunit->unit_name, p_destport->iport - ISF_IN_BASE);
	//else {
	//	DBG_ERR(" port is not connect yet!\r\n");
	//}
	return ISF_ERR_FAILED;
}


ISF_RV _isf_unit_push_data_from_user(ISF_UNIT *p_thisunit, UINT32 iport, ISF_DATA *p_data, INT32 wait_ms)
{
	ISF_RV ret;
	NVTMPP_VB_BLK  blk = NVTMPP_VB_INVALID_BLK;


	if(p_data->sign != ISF_SIGN_DATA) {
		return ISF_ERR_INVALID_SIGN;
	}
	if(p_data->desc[0] == 0) {
		return ISF_ERR_INVALID_DATA;
	}
	if(p_data->p_class == 0) { //this should be a user generated data
		ISF_DATA_CLASS* p_thisclass_fill;
		if(p_data->desc[0] == MAKEFOURCC('V','F','R','M')) {
			if(p_data->h_data == 0) {
				return ISF_ERR_INVALID_DATA;
			} if(p_data->h_data == (UINT32)-1) {
				return ISF_ERR_INVALID_DATA;
			} else if(p_data->h_data == (UINT32)-2) {
				//vfrm user blk, no protected
				if(_isf_vfrm_class == 0) {
					_isf_vfrm_class = _isf_data_init_class(&_isf_vfrm_base, p_data->desc[0]);
				}
				p_thisclass_fill = _isf_vfrm_class;
			} else {
				//vfrm common blk, protected by nvtmpp
				if(_isf_vfrm_common_class == 0) {
					_isf_vfrm_common_class = _isf_data_init_class(&_isf_vfrm_common_base, p_data->desc[0]);
				}
				p_thisclass_fill = _isf_vfrm_common_class;
			}
			//validate phyaddr come from user
			{
				VDO_FRAME *p_vdoframe = (VDO_FRAME *)(p_data->desc);
				int p, n=0;

				if (nvtmpp_is_dynamic_map()) {
					blk = nvtmpp_sys_get_blk_by_pa(p_vdoframe->phyaddr[0]);
					if (blk == NVTMPP_VB_INVALID_BLK) {
						DBG_ERR("fail to get blk by pa 0x%08x\n", (int)p_vdoframe->phyaddr[0]);
						return ISF_ERR_INVALID_DATA;
					}
					if (nvtmpp_vb_blk_map_va(blk) == NULL) {
						DBG_ERR("fail to map blk 0x%08x\n", (int)blk);
						ret = ISF_ERR_INVALID_DATA;
						goto vfrm_exit;
					}
				}
				for (p = 0; p < VDO_MAX_PLANE; p++) {
					if (p_vdoframe->phyaddr[p] != 0)  {
						//come from user
						UINT32 va = nvtmpp_sys_pa2va(p_vdoframe->phyaddr[p]);
						if (!va) {
							DBG_ERR("fail to map address of phyaddr[%d]=%08x\n", p, p_vdoframe->phyaddr[p]);
							ret = ISF_ERR_INVALID_DATA;
							goto vfrm_exit;
						}
						n++;
					}
				}
				if (n == 0) {
					ret = ISF_ERR_INVALID_DATA;
					goto vfrm_exit;
 				}
			}
		} else if(p_data->desc[0] == MAKEFOURCC('V','S','T','M')) {
			//vstm user blk, no protected
			if(_isf_vstm_class == 0) {
				_isf_vstm_class = _isf_data_init_class(&_isf_vstm_base, p_data->desc[0]);
			}
			p_thisclass_fill = _isf_vstm_class;
			//validate phyaddr come from user
			{
				VDO_BITSTREAM *p_vdobs = (VDO_BITSTREAM *)(p_data->desc);
				//come from user
				if (p_vdobs->DataAddr == 0) {
					return ISF_ERR_INVALID_DATA;
 				} else {
					UINT32 va = nvtmpp_sys_pa2va(p_vdobs->DataAddr);
					if (!va) {
						DBG_ERR("fail to map address of phy_addr=%08x\n", p_vdobs->DataAddr);
						return ISF_ERR_INVALID_DATA;
					}
				}
			}
		} else if(p_data->desc[0] == MAKEFOURCC('A','F','R','M')) {
			//afrm user blk, no protected
			if(_isf_afrm_class == 0) {
				_isf_afrm_class = _isf_data_init_class(&_isf_afrm_base, p_data->desc[0]);
			}
			p_thisclass_fill = _isf_afrm_class;
			//validate phyaddr come from user
			{
				AUD_FRAME *p_audframe = (AUD_FRAME *)(p_data->desc);
				int p, n=0;
				for (p = 0; p < 2; p++) {
	 				if (p_audframe->phyaddr[p] != 0) {
						//come from user
						UINT32 va = nvtmpp_sys_pa2va(p_audframe->phyaddr[p]);
						if (!va) {
							DBG_ERR("fail to map address of phy_addr[%d]=%08x\n", p, p_audframe->phyaddr[p]);
							return ISF_ERR_INVALID_DATA;
						}
						n++;
					}
				}
				if (n == 0) {
					return ISF_ERR_INVALID_DATA;
 				}
			}
		} else if(p_data->desc[0] == MAKEFOURCC('A','S','T','M')) {
			//astm user blk, no protected
			if(_isf_astm_class == 0) {
				_isf_astm_class = _isf_data_init_class(&_isf_astm_base, p_data->desc[0]);
			}
			p_thisclass_fill = _isf_astm_class;
			//validate phyaddr come from user
			{
				AUD_BITSTREAM *p_audbs = (AUD_BITSTREAM *)(p_data->desc);
				//come from user
				if (p_audbs->DataAddr == 0) {
					return ISF_ERR_INVALID_DATA;
 				} else {
					UINT32 va = nvtmpp_sys_pa2va(p_audbs->DataAddr);
					if (!va) {
						DBG_ERR("fail to map address of phy_addr=%08x\n", p_audbs->DataAddr);
						return ISF_ERR_INVALID_DATA;
					}
				}
			}
		} else {
			//it should be a registered private blk:
			//..maybe vstm private blk, protected by videoenc
			//..maybe afrm private blk, protected by audiocap or audiodec
			//..maybe astm private blk, protected by audioenc
			if(p_data->h_data == 0) {
				return ISF_ERR_INVALID_DATA;
			}
			_isf_user_class = _isf_data_init_class(NULL, p_data->desc[0]); //NULL to lookup registered class
			p_thisclass_fill = _isf_user_class;
		}
		if(!p_thisclass_fill) {
			return ISF_ERR_INVALID_DATA;
		}
		p_data->p_class = p_thisclass_fill;
	}

	_isf_unit_add_data(p_data); //user's ref++

	ret = _isf_base_unit_push_from_user(p_thisunit, iport, p_data, wait_ms);

vfrm_exit:
	if (blk != NVTMPP_VB_INVALID_BLK) {
		nvtmpp_vb_blk_unmap_va(blk);
	}
	return ret;
}
//EXPORT_SYMBOL(_isf_unit_push_data_from_user);

#define EVENT_TAG MAKEFOURCC('E','V','N','T')
static ISF_RV _isf_event_lock(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data) {return ISF_OK;}
static ISF_RV _isf_event_unlock(ISF_DATA *p_data, ISF_UNIT *p_thisunit, UINT32 h_data) {return ISF_OK;}
static ISF_DATA_CLASS _isf_event_base = {
	.do_lock = _isf_event_lock,
	.do_unlock = _isf_event_unlock,
};

ISF_RV _isf_unit_send_event(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 event, UINT32 param1, UINT32 param2, UINT32 param3)
{
	ISF_DATA data = {0};
	ISF_DATA *p_data = &data;
	if (!p_thisunit || !p_data)
		return ISF_ERR_NULL_POINTER;
	p_data->sign = ISF_SIGN_DATA;
	p_data->desc[0] = EVENT_TAG;
	p_data->p_class = &_isf_event_base;
	{
		ISF_EVENT_MSG *p_event = (ISF_EVENT_MSG *)(p_data->desc);
		p_event->event = event;
		p_event->param1 = param1;
		p_event->param2 = param2;
		p_event->param3 = param3;
	}
	return _isf_base_unit_push(p_thisunit, nport, p_data, 0);
}
//EXPORT_SYMBOL(_isf_unit_send_event);

BOOL _isf_base_unit_is_allow_pull(ISF_UNIT *p_thisunit, ISF_PORT *p_srcport)
{
	if (!(p_srcport && ((p_srcport->connecttype & ISF_CONNECT_PULL) || (p_srcport->do_pull != 0)))) {
		//DBG_ERR("not allow PULL!!\r\n");
		return FALSE;
	}
	if (p_srcport->flag & ISF_PORT_FLAG_PAUSE_PULL) {
		return FALSE;
	}
	return TRUE;
}

BOOL _isf_unit_is_allow_pull(ISF_PORT *p_srcport)
{
	return _isf_base_unit_is_allow_pull(NULL, p_srcport);
}
//EXPORT_SYMBOL(_isf_unit_is_allow_pull);

static ISF_RV _isf_base_unit_pull(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 wait_ms)
{
	ISF_RV r = ISF_OK;
	ISF_RV rr = ISF_OK;
	ISF_PORT_CAPS *p_srcport;
	ISF_RV (*p_do_pull)(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms);
	if (!p_data) {
		return ISF_ERR_NULL_POINTER;
	}
	if(!p_thisunit){
		return ISF_ERR_NULL_POINTER;
	}
	p_srcport = p_thisunit->port_outcaps[oport - ISF_OUT_BASE];
	if (p_srcport) {
		p_do_pull = p_srcport->do_pull;
	}
	if (!p_srcport) { //pull from null-port
		return ISF_ERR_NULL_POINTER;
	} else if (p_do_pull) { //pull from src-port and auto lock/unlock
		rr = p_do_pull(oport, p_data, p_thisunit, wait_ms); //do pull
		if (rr == ISF_OK) {
#if (DUMP_DATA == ENABLE)
			UINT64 c_module;
			c_module = p_thisunit->unit_module;
#endif
			r = _isf_data_verify(p_thisunit->unit_name, "Pull", p_data);
			_isf_unit_check_return(r);
			if (r != ISF_OK) {
				return r;
			}

#if (DUMP_DATA == ENABLE)
			DBG_IND("\"%8s\" blk=%08x\r\n", (CHAR *)&c_module, (UINT32)p_data->h_data);
#endif
			//if ((p_srcport) && (p_srcport->attr & ISF_PORT_ATTR_READYSRCBUF)) {
			//	r = ISF_OK;    //skip
			//} else {
			//	r = _isf_data_lock(p_thisunit, p_data);    //this lock
			//}
			//if (ISF_OK != r) {
			//	return r;
			//}
			//if ((p_srcport) && (p_srcport->attr & ISF_PORT_ATTR_KEEPSRCBUF)) {
			//	r = ISF_OK;    //skip
			//} else {
			//	r = _isf_data_unlock(p_srcport->p_srcunit, p_data);    //src unlock
			//}
		} else {
			r = rr;
		}

		if ((r != ISF_OK) && (r != ISF_ERR_QUEUE_EMPTY) && (r != ISF_ERR_WAIT_TIMEOUT) && (r != ISF_ERR_NOT_OPEN_YET) && (r != ISF_ERR_NOT_START)) {
			if (p_thisunit)
				DBG_ERR(" port \"%s\".out[%d] is failed (%d)\r\n",
						p_thisunit->unit_name, oport - ISF_OUT_BASE, r);
			else {
				DBG_ERR(" port is failed (%d)\r\n", r);
			}
		}
		return r;
	}
	//if (p_srcport->p_srcunit)
	//	DBG_ERR(" port \"%s\".out[%d] is not support!\r\n",
	//			p_srcport->p_srcunit->unit_name, p_srcport->oport - ISF_OUT_BASE);
	//else {
	//	DBG_ERR(" port is not connect yet!\r\n");
	//}
	return ISF_ERR_NOT_CONNECT_YET;
}

static ISF_RV _isf_base_unit_pull_from_user(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 wait_ms)
{
	ISF_RV r = ISF_OK;
	ISF_RV rr = ISF_OK;
	ISF_PORT_CAPS *p_srcport;
	ISF_RV (*p_do_pull)(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms);
	if (!p_data) {
		return ISF_ERR_NULL_POINTER;
	}
	if(!p_thisunit){
		return ISF_ERR_NULL_POINTER;
	}
	p_srcport = p_thisunit->port_outcaps[oport - ISF_OUT_BASE];
	if (p_srcport) {
		p_do_pull = p_srcport->do_pull;
	}
	if (!p_srcport) { //pull from null-port
		return ISF_ERR_NULL_POINTER;
	} else if (p_do_pull) { //pull from src-port and auto lock/unlock
		rr = p_do_pull(oport, p_data, NULL, wait_ms); //do pull
		if (rr == ISF_OK) {
#if (DUMP_DATA == ENABLE)
			UINT64 c_module;
			c_module = g_module;
#endif
			r = _isf_data_verify("user", "pull", p_data);
			_isf_unit_check_return(r);
			if (r != ISF_OK) {
				return r;
			}

#if (DUMP_DATA == ENABLE)
			DBG_IND("\"%8s\" blk=%08x\r\n", (CHAR *)&c_module, (UINT32)p_data->h_data);
#endif
/*
			if(p_data->sign != ISF_SIGN_DATA) {
				r = ISF_ERR_INVALID_SIGN;
			}
*/
			if(p_data->desc[0] == 0) {
				r = ISF_ERR_INVALID_DATA;
			}
			if(p_data->desc[0] == MAKEFOURCC('V','F','R','M')) {
				if(p_data->h_data == 0) {
					r = ISF_ERR_INVALID_DATA;
				} if(p_data->h_data == (UINT32)-1) {
					r = ISF_ERR_INVALID_DATA;
				} else if(p_data->h_data == (UINT32)-2) {
					//vfrm user blk, no protected
				} else {
					//vfrm common blk, protected by nvtmpp
					r = _isf_data_lock(NULL, p_data);    //this lock
					if (ISF_OK != r) {
						return r;
					}

					r = _isf_data_unlock(p_thisunit, p_data);    //src unlock
				}
			} else {
				//it should be a registered private blk:
				//..maybe vstm private blk, protected by videoenc
				//..maybe afrm private blk, protected by audiocap or audiodec
				//..maybe astm private blk, protected by audioenc
				if(p_data->h_data == 0) {
					r = ISF_ERR_INVALID_DATA;
				} else {
					r = _isf_data_lock(NULL, p_data);    //this lock
					if (ISF_OK != r) {
						return r;
					}

					r = _isf_data_unlock(p_thisunit, p_data);    //src unlock
				}
			}

		} else {
			r = rr;
		}

		if ((r != ISF_OK) && (r != ISF_ERR_QUEUE_EMPTY) && (r != ISF_ERR_WAIT_TIMEOUT) && (r != ISF_ERR_NOT_OPEN_YET) && (r != ISF_ERR_NOT_START)) {
			if (p_thisunit)
				DBG_ERR(" [user] pull from \"%s\".out[%d] is failed (%d)\r\n", p_thisunit->unit_name, oport - ISF_OUT_BASE, r);
		    else
				DBG_ERR(" [user] pull from \"?\" is failed (%d)\r\n", r);
		}
		return r;
	}
	//if (p_srcport->p_srcunit)
	//	DBG_ERR(" port \"%s\".out[%d] is not support!\r\n",
	//			p_srcport->p_srcunit->unit_name, p_srcport->oport - ISF_OUT_BASE);
	//else {
	//	DBG_ERR(" port is not connect yet!\r\n");
	//}
	return ISF_ERR_NOT_CONNECT_YET;
}

ISF_RV _isf_unit_pull_data_from_user(ISF_UNIT *p_thisunit, UINT32 oport, ISF_DATA *p_data, INT32 wait_ms)
{
	return _isf_base_unit_pull_from_user(p_thisunit, oport, p_data, wait_ms);
}
//EXPORT_SYMBOL(_isf_unit_pull_data_from_user);

BOOL _isf_base_unit_is_allow_notify(ISF_UNIT *p_thisunit, ISF_PORT *p_srcport)
{
	if (!(p_srcport && (p_srcport->connecttype & ISF_CONNECT_NOTIFY))) {
		//DBG_ERR("not allow NOTIFY!!\r\n");
		return FALSE;
	}
	if (p_srcport->flag & ISF_PORT_FLAG_PAUSE_PUSH) {
		return FALSE;
	}
	return TRUE;
}

BOOL _isf_unit_is_allow_notify(ISF_PORT *p_srcport)
{
	return _isf_base_unit_is_allow_notify(NULL, p_srcport);
}
//EXPORT_SYMBOL(_isf_unit_is_allow_notify);

static ISF_RV _isf_base_unit_notify(ISF_UNIT *p_thisunit, ISF_PORT *p_srcport, ISF_DATA *p_data, INT32 wait_ms, ISF_RV result, ISF_PORT *p_destport)
{
	ISF_RV r = ISF_OK;
	ISF_RV nr = ISF_OK;
	if (!p_data) {
		return p_destport->do_notify(p_destport, 0, wait_ms);
	}
	r = _isf_data_verify((p_thisunit) ? (p_thisunit->unit_name) : ("user"), "notify", p_data);
	_isf_unit_check_return(r);
	if (r != ISF_OK) {
		return r;
	}

	if (!p_destport) { //push to null-port
		DBG_ERR(" port is null!\r\n");
		return ISF_ERR_NULL_POINTER;
	} else if (p_destport->do_notify && p_data) { //push to dest-port and lock/unlock
#if (DUMP_DATA == ENABLE)
		UINT64 c_module;
		if (p_thisunit) {
			c_module = p_thisunit->unit_module;
		} else {
			c_module = g_module;
		}
#endif
#if (DUMP_DATA == ENABLE)
		DBG_IND("\"%8s\" blk=%08x\r\n", (CHAR *)&c_module, (UINT32)p_data->h_data);
#endif
#if 0
		if ((p_destport) && (p_destport->attr & ISF_PORT_ATTR_READYSRCBUF)) {
			r = ISF_OK;    //skip
		} else
#endif
			r = _isf_data_lock(p_destport->p_srcunit, p_data); //dest lock
		if (ISF_OK != r) {
			return r;
		}
		nr = p_destport->do_notify(p_destport, p_data, wait_ms);
		#if _TODO
		_isf_unit_debug_release(p_thisunit, 0, p_data, result); //do probe
		#endif

		//if ((p_destport) && (p_destport->attr & ISF_PORT_ATTR_KEEPSRCBUF)) {
		//	r = ISF_OK;    //skip
		//} else {
			r = _isf_data_unlock(p_thisunit, p_data);    //this unlock
		//}
		if (ISF_OK != nr) {
			return nr;
		}
		return r;
	}
	if (p_destport->p_destunit)
		DBG_ERR(" port \"%s\".in[%d] is not support!\r\n",
				p_destport->p_destunit->unit_name, p_destport->iport - ISF_IN_BASE);
	else {
		DBG_ERR(" port is not connect yet!\r\n");
	}
	return ISF_ERR_NOT_CONNECT_YET;
}

ISF_RV _isf_unit_notify(ISF_PORT *p_srcport, ISF_DATA* p_syncdata)
{
	if (!p_srcport) {
		DBG_ERR("cannot NOTIFY!!\r\n");
		return ISF_ERR_NOT_CONNECT_YET;
	}
	if (!p_srcport->do_notify) {
		DBG_ERR("cannot NOTIFY!!\r\n");
		return ISF_ERR_NOT_SUPPORT;
	}
	return _isf_base_unit_notify(0, NULL, p_syncdata, 0, ISF_OK, p_srcport);
}
//EXPORT_SYMBOL(_isf_unit_notify);

static UINT32 _isf_base_unit_new_i(struct _ISF_UNIT *p_thisunit, ISF_PORT *p_srcport, CHAR* name, UINT32 size, ISF_DATA *p_data)
{
	NVTMPP_VB_POOL pool;
	UINT32 addr = 0;
	if (!p_data)
		return 0;
	pool = _isf_base_unit_create(p_thisunit, p_srcport, name, size, 1);
	// create pool
	if (pool == NVTMPP_VB_INVALID_POOL)
		return 0;

	_isf_unit_init_data(p_data, NULL, MAKEFOURCC('C','O','M','M'), 0x00010000);

	// get blk from pool
	if(p_srcport == NULL) {  //this is ctrl
		addr = _isf_base_unit_new(p_thisunit, ISF_CTRL, (UINT32)pool, 0, size, p_data, 0);
		if (p_data && addr) {p_data->func = pool; p_data->src = (UINT32)ISF_CTRL;}
	} else if ((((UINT32)p_srcport) & 0xffffff00) == 0xffffff00) {
		UINT32 nport = (((UINT32)p_srcport) & ~0xffffff00);
		/*if (nport == ISF_CTRL) {  //this is ctrl
			addr = _isf_base_unit_new(p_thisunit, ISF_CTRL, (UINT32)pool, 0, size, p_data, 0);
			if (p_data && addr) {p_data->func = pool; p_data->src = (UINT32)ISF_CTRL;}
		} else */
		if (ISF_IS_OPORT(p_thisunit, nport)) {  //this is a oport
			UINT32 i = nport - ISF_OUT_BASE;
			addr = _isf_base_unit_new(p_thisunit, ISF_OUT(i), (UINT32)pool, 0, size, p_data, 0);
			if (p_data && addr) {p_data->func = pool; p_data->src = (UINT32)ISF_OUT(i);}
		} else if (ISF_IS_IPORT(p_thisunit, nport)) {  //this is a iport
			UINT32 i = nport - ISF_IN_BASE;
			addr = _isf_base_unit_new(p_thisunit, ISF_IN(i), (UINT32)pool, 0, size, p_data, 0);
			if (p_data && addr) {p_data->func = pool; p_data->src = (UINT32)ISF_IN(i);}
		}
	} else { //this is a oport
		addr = _isf_base_unit_new(p_thisunit, p_srcport->oport, (UINT32)pool, 0, size, p_data, 0);
		if (p_data && addr) {p_data->func = pool; p_data->src = (UINT32)p_srcport->oport;}
	}
	return addr;
}

static ISF_RV _isf_base_unit_release_i(struct _ISF_UNIT *p_thisunit, ISF_DATA *p_data, ISF_RV result)
{
	NVTMPP_VB_POOL pool;
	ISF_PORT *p_srcport = NULL;
	ISF_RV r;
	UINT32 nport;
	if (!p_data)
		return ISF_ERR_NULL_POINTER;
	pool = (NVTMPP_VB_POOL)p_data->func;
	nport = p_data->src;
	// release blk to pool
	r = _isf_base_unit_release(p_thisunit, nport, p_data, 0);
	if (r != ISF_OK)
		return r;
	if(nport != ISF_CTRL) {
		p_srcport = p_thisunit->port_out[nport - ISF_OUT_BASE];
	}
	// destroy pool
	r = _isf_base_unit_destory(p_thisunit, p_srcport, (UINT32)pool);
	return r;
}

ISF_PORT *_isf_unit_out(ISF_UNIT *p_unit, UINT32 oport);
ISF_PORT *_isf_unit_in(ISF_UNIT *p_unit, UINT32 iport);

ISF_BASE_UNIT _isf_unit_base = {
	.reg_data = isf_data_regclass,
	.init_data = _isf_unit_init_data,
	.oport = _isf_unit_out,
	.iport = _isf_unit_in,
	.get_port = _isf_unit_port,
	.get_bind_port = _isf_bind_port,
	.get_info = _isf_unit_info,
	.get_bind_info = _isf_bind_info,
	.do_create = _isf_base_unit_create,
	.do_destory = _isf_base_unit_destory,
	.do_new_i = _isf_base_unit_new_i,
	.do_release_i = _isf_base_unit_release_i,
	.do_new = _isf_base_unit_new,
	.do_add = _isf_base_unit_add,
	.do_release = _isf_base_unit_release,
	.do_push = _isf_base_unit_push,
	.do_pull = _isf_base_unit_pull,
	.do_notify = _isf_base_unit_notify,
	.get_is_allow_push = _isf_base_unit_is_allow_push,
	.get_is_allow_pull = _isf_base_unit_is_allow_pull,
	.get_is_allow_notify = _isf_base_unit_is_allow_notify,
	.do_probe = _isf_probe,
	.do_trace = _isf_trace,
	.do_debug = _isf_debug,
	.do_clear_ctx = _isf_unit_clear_context,
	.do_clear_bind = _isf_unit_clear_bind,
	.do_clear_state = _isf_unit_clear_state,
};


