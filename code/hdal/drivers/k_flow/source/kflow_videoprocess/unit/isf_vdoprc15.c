#include "isf_vdoprc_int.h"
//#include "nvtmpp.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_vdoprc15
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoprc15_debug_level = NVT_DBG_WRN;
//module_param_named(isf_vdoprc15_debug_level, isf_vdoprc15_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_vdoprc15_debug_level, "vdoprc15 debug level");
///////////////////////////////////////////////////////////////////////////////

#if (VDOPRC_MAX_NUM > 15)

extern ISF_UNIT isf_vdoprc15;

static ISF_RV isf_vdoprc15_inputport1_do_push(ISF_PORT *p_port, ISF_DATA *p_data, INT32 wait_ms)
{
	return _isf_vdoprc_iport_do_push(&isf_vdoprc15, ISF_IN(0), p_data, wait_ms);
}
static ISF_PORT_CAPS isf_vdoprc15_inputport1_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = isf_vdoprc15_inputport1_do_push,
};

static ISF_RV isf_vdoprc15_outputport1_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
static ISF_RV isf_vdoprc15_outputport2_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
static ISF_RV isf_vdoprc15_outputport3_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
static ISF_RV isf_vdoprc15_outputport4_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
static ISF_RV isf_vdoprc15_outputport5_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
static ISF_RV isf_vdoprc15_outputport6_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
static ISF_RV isf_vdoprc15_outputport7_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
static ISF_RV isf_vdoprc15_outputport8_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}

#if (VDOPRC_MAX_OUT_NUM > 8)

static ISF_RV isf_vdoprc15_outputport9_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
static ISF_RV isf_vdoprc15_outputport10_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
static ISF_RV isf_vdoprc15_outputport11_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
static ISF_RV isf_vdoprc15_outputport12_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
static ISF_RV isf_vdoprc15_outputport13_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
static ISF_RV isf_vdoprc15_outputport14_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
static ISF_RV isf_vdoprc15_outputport15_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
static ISF_RV isf_vdoprc15_outputport16_do_notify(struct _ISF_PORT_* p_port, ISF_DATA* p_syncdata, INT32 wait_ms)
{
	return ISF_ERR_NOT_SUPPORT;
}
#endif

#if (USE_PULL == ENABLE)
static ISF_RV _isf_vdoprc15_outputport1_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_vdoprc15_outputport2_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_vdoprc15_outputport3_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_vdoprc15_outputport4_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_vdoprc15_outputport5_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_vdoprc15_outputport6_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_vdoprc15_outputport7_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_vdoprc15_outputport8_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

#if (VDOPRC_MAX_OUT_NUM > 8)

static ISF_RV _isf_vdoprc15_outputport9_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_vdoprc15_outputport10_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_vdoprc15_outputport11_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_vdoprc15_outputport12_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_vdoprc15_outputport13_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_vdoprc15_outputport14_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_vdoprc15_outputport15_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}

static ISF_RV _isf_vdoprc15_outputport16_do_pull(UINT32 oport, ISF_DATA *p_data, struct _ISF_UNIT *p_destunit, INT32 wait_ms)
{
	if(p_destunit == NULL) {
		return _isf_vdoprc_oqueue_do_pull(&isf_vdoprc15, oport, p_data, wait_ms);
	}
	return ISF_ERR_NOT_SUPPORT;
}
#endif
#endif

#if (USE_PULL == ENABLE)
#define O_CONNECT_TYPE	ISF_CONNECT_PUSH|ISF_CONNECT_PULL|ISF_CONNECT_NOTIFY
#else
#define O_CONNECT_TYPE	ISF_CONNECT_PUSH|ISF_CONNECT_NOTIFY
#endif

static ISF_PORT_CAPS isf_vdoprc15_outputport1_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport1_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport1_do_notify,
};

static ISF_PORT_CAPS isf_vdoprc15_outputport2_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport2_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport2_do_notify,
};

static ISF_PORT_CAPS isf_vdoprc15_outputport3_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport3_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport3_do_notify,
};

static ISF_PORT_CAPS isf_vdoprc15_outputport4_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport4_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport4_do_notify,
};

static ISF_PORT_CAPS isf_vdoprc15_outputport5_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport5_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport5_do_notify,
};

static ISF_PORT_CAPS isf_vdoprc15_outputport6_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport6_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport6_do_notify,
};

static ISF_PORT_CAPS isf_vdoprc15_outputport7_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport7_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport7_do_notify,
};

static ISF_PORT_CAPS isf_vdoprc15_outputport8_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport8_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport8_do_notify,
};

#if (VDOPRC_MAX_OUT_NUM > 8)

static ISF_PORT_CAPS isf_vdoprc15_outputport9_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport9_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport9_do_notify,
};

static ISF_PORT_CAPS isf_vdoprc15_outputport10_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport10_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport10_do_notify,
};

static ISF_PORT_CAPS isf_vdoprc15_outputport11_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport11_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport11_do_notify,
};

static ISF_PORT_CAPS isf_vdoprc15_outputport12_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport12_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport12_do_notify,
};

static ISF_PORT_CAPS isf_vdoprc15_outputport13_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport13_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport13_do_notify,
};

static ISF_PORT_CAPS isf_vdoprc15_outputport14_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport14_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport14_do_notify,
};

static ISF_PORT_CAPS isf_vdoprc15_outputport15_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport15_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport15_do_notify,
};

static ISF_PORT_CAPS isf_vdoprc15_outputport16_caps = {
	.connecttype_caps = O_CONNECT_TYPE,
#if (USE_PULL == ENABLE)
	.do_pull = _isf_vdoprc15_outputport16_do_pull,
#else
	.do_pull = NULL,
#endif
	.do_notify = isf_vdoprc15_outputport16_do_notify,
};
#endif

static ISF_PORT isf_vdoprc15_inputport1 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(0),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoprc15,
	.p_srcunit = NULL,
};
static ISF_PORT *isf_vdoprc15_inputport_list[ISF_VDOPRC_IN_NUM] = {
	&isf_vdoprc15_inputport1,
};
static ISF_PORT *isf_vdoprc15_outputport_list[ISF_VDOPRC_OUT_NUM] = {0};
static ISF_PORT *isf_vdoprc15_outputport_list_cfg[ISF_VDOPRC_OUT_NUM] = {0};
static ISF_PORT_CAPS *isf_vdoprc15_inputport_list_caps[ISF_VDOPRC_IN_NUM] = {
	&isf_vdoprc15_inputport1_caps,
};
static ISF_PORT_CAPS *isf_vdoprc15_outputport_list_caps[ISF_VDOPRC_OUT_NUM] = {
	&isf_vdoprc15_outputport1_caps,
	&isf_vdoprc15_outputport2_caps,
	&isf_vdoprc15_outputport3_caps,
	&isf_vdoprc15_outputport4_caps,
	&isf_vdoprc15_outputport5_caps,
	&isf_vdoprc15_outputport6_caps,
	&isf_vdoprc15_outputport7_caps,
	&isf_vdoprc15_outputport8_caps,
#if (VDOPRC_MAX_OUT_NUM > 8)
	&isf_vdoprc15_outputport9_caps,
	&isf_vdoprc15_outputport10_caps,
	&isf_vdoprc15_outputport11_caps,
	&isf_vdoprc15_outputport12_caps,
	&isf_vdoprc15_outputport13_caps,
	&isf_vdoprc15_outputport14_caps,
	&isf_vdoprc15_outputport15_caps,
	&isf_vdoprc15_outputport16_caps,
#endif
};

static ISF_STATE isf_vdoprc15_outputport_state[ISF_VDOPRC_OUT_NUM] = {0};
static ISF_STATE *isf_vdoprc15_outputport_list_state[ISF_VDOPRC_OUT_NUM] = {
	&isf_vdoprc15_outputport_state[0],
	&isf_vdoprc15_outputport_state[1],
	&isf_vdoprc15_outputport_state[2],
	&isf_vdoprc15_outputport_state[3],
	&isf_vdoprc15_outputport_state[4],
	&isf_vdoprc15_outputport_state[5],
	&isf_vdoprc15_outputport_state[6],
	&isf_vdoprc15_outputport_state[7],
#if (VDOPRC_MAX_OUT_NUM > 8)
	&isf_vdoprc15_outputport_state[8],
	&isf_vdoprc15_outputport_state[9],
	&isf_vdoprc15_outputport_state[10],
	&isf_vdoprc15_outputport_state[11],
	&isf_vdoprc15_outputport_state[12],
	&isf_vdoprc15_outputport_state[13],
	&isf_vdoprc15_outputport_state[14],
	&isf_vdoprc15_outputport_state[15],
#endif
};

static ISF_INFO isf_vdoprc15_outputinfo1_in = {0};
static ISF_INFO *isf_vdoprc15_outputinfolist_in[ISF_VDOPRC_IN_NUM] = {
	&isf_vdoprc15_outputinfo1_in,
};

static ISF_INFO isf_vdoprc15_outputinfo_out[ISF_VDOPRC_OUT_NUM] = {0};
static ISF_INFO *isf_vdoprc15_outputinfolist_out[ISF_VDOPRC_OUT_NUM] = {
	&isf_vdoprc15_outputinfo_out[0],
	&isf_vdoprc15_outputinfo_out[1],
	&isf_vdoprc15_outputinfo_out[2],
	&isf_vdoprc15_outputinfo_out[3],
	&isf_vdoprc15_outputinfo_out[4],
	&isf_vdoprc15_outputinfo_out[5],
	&isf_vdoprc15_outputinfo_out[6],
	&isf_vdoprc15_outputinfo_out[7],
#if (VDOPRC_MAX_OUT_NUM > 8)
	&isf_vdoprc15_outputinfo_out[8],
	&isf_vdoprc15_outputinfo_out[9],
	&isf_vdoprc15_outputinfo_out[10],
	&isf_vdoprc15_outputinfo_out[11],
	&isf_vdoprc15_outputinfo_out[12],
	&isf_vdoprc15_outputinfo_out[13],
	&isf_vdoprc15_outputinfo_out[14],
	&isf_vdoprc15_outputinfo_out[15],
#endif
};


static ISF_PORT_PATH isf_vdoprc15_path_list[ISF_VDOPRC_PATH_NUM];

extern void _vdoprc_do_updateall(UINT32 i);

static ISF_RV isf_vdoprc15_do_bindinput(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport)
{
	return _isf_vdoprc_do_bindinput(p_thisunit, iport, p_srcunit, oport);
}
static ISF_RV isf_vdoprc15_do_setparam(struct _ISF_UNIT *p_thisunit, UINT32 nPort, UINT32 param, UINT32 value)
{
	return _isf_vdoprc_do_setparam(p_thisunit, nPort, param, value);
}
static UINT32 isf_vdoprc15_do_getparam(struct _ISF_UNIT *p_thisunit, UINT32 nPort, UINT32 param)
{
	return _isf_vdoprc_do_getparam(p_thisunit, nPort, param);
}
static ISF_RV isf_vdoprc15_do_updateport(struct _ISF_UNIT *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd)
{
	return _isf_vdoprc_do_updateport(p_thisunit, oport, cmd);
}

ISF_UNIT isf_vdoprc15;
static ISF_PORT_PATH isf_vdoprc15_path_list[ISF_VDOPRC_PATH_NUM] = {
	{&isf_vdoprc15, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdoprc15, ISF_IN(0), ISF_OUT(1)},
	{&isf_vdoprc15, ISF_IN(0), ISF_OUT(2)},
	{&isf_vdoprc15, ISF_IN(0), ISF_OUT(3)},
	{&isf_vdoprc15, ISF_IN(0), ISF_OUT(4)},
	{&isf_vdoprc15, ISF_IN(0), ISF_CTRL}, //EXT ISF_OUT(0)
	{&isf_vdoprc15, ISF_IN(0), ISF_CTRL}, //EXT ISF_OUT(1)
	{&isf_vdoprc15, ISF_IN(0), ISF_CTRL}, //EXT ISF_OUT(2)
#if (VDOPRC_MAX_OUT_NUM > 8)
	{&isf_vdoprc15, ISF_IN(0), ISF_CTRL}, //EXT ISF_OUT(3)
	{&isf_vdoprc15, ISF_IN(0), ISF_CTRL}, //EXT ISF_OUT(4)
	{&isf_vdoprc15, ISF_IN(0), ISF_CTRL}, //EXT ISF_OUT(5)
	{&isf_vdoprc15, ISF_IN(0), ISF_CTRL}, //EXT ISF_OUT(6)
	{&isf_vdoprc15, ISF_IN(0), ISF_CTRL}, //EXT ISF_OUT(7)
	{&isf_vdoprc15, ISF_IN(0), ISF_CTRL}, //EXT ISF_OUT(8)
	{&isf_vdoprc15, ISF_IN(0), ISF_CTRL}, //EXT ISF_OUT(9)
	{&isf_vdoprc15, ISF_IN(0), ISF_CTRL}, //EXT ISF_OUT(10)
#endif
};

//hook CTL_IPP_CBEVT_IN_BUF
static INT32 _vdoprc15_input_cb(UINT32 event, void *p_in, void *p_out)
{
	ISF_UNIT *p_thisunit = &isf_vdoprc15;
	CTL_IPP_EVT *evt = (CTL_IPP_EVT *)p_in;
	UINT32 j = evt->buf_id;
#if (USE_VPE == ENABLE)
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

	if (p_ctx->vpe_mode) {
		_isf_vdoprc_vpe_iport_do_proc_cb(p_thisunit, ISF_IN(0), j, event, evt->err_msg);
		return 0;
	}
#endif
#if (USE_ISE == ENABLE)
	{
		VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

		if (p_ctx->ise_mode) {
			_isf_vdoprc_ise_iport_do_proc_cb(p_thisunit, ISF_IN(0), j, event, evt->err_msg);
			return 0;
		}
	}
#endif
	_isf_vdoprc_iport_do_proc_cb(p_thisunit, ISF_IN(0), j, event, evt->err_msg);

	return 0;
}

//hook CTL_IPP_CBEVT_DATASTAMP
static INT32 _vdoprc15_osd_cb(UINT32 event, void *p_in, void *p_out)
{
	ISF_UNIT *p_thisunit = &isf_vdoprc15;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

	//CTL_IPP_DS_CB_INPUT_INFO in = *(CTL_IPP_DS_CB_INPUT_INFO *)p_in;
	//CTL_IPP_DS_CB_OUTPUT_INFO out = *(CTL_IPP_DS_CB_OUTPUT_INFO *)p_out;

	p_ctx->proc.st_osdmask |= 0x01;
	_isf_vdoprc_do_input_osd(p_thisunit, ISF_IN(0), p_in, p_out);

	return 0;
}

//hook CTL_IPP_CBEVT_PRIMASK
static INT32 _vdoprc15_mask_cb(UINT32 event, void *p_in, void *p_out)
{
	ISF_UNIT *p_thisunit = &isf_vdoprc15;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);

	//CTL_IPP_PM_CB_INPUT_INFO in = *(CTL_IPP_PM_CB_INPUT_INFO *)p_in;
	// CTL_IPP_PM_CB_OUTPUT_INFO out = *( CTL_IPP_PM_CB_OUTPUT_INFO *)p_out;

	p_ctx->proc.st_osdmask |= 0x02;
	_isf_vdoprc_do_input_mask(p_thisunit, ISF_IN(0), p_in, p_out);

	return 0;
}

//hook CTL_IPP_CBEVT_ENG_IME_ISR
static INT32 _vdoprc15_process_cb(UINT32 event, void *p_in, void *p_out)
{
	ISF_UNIT *p_thisunit = &isf_vdoprc15;

	//CTL_IPP_IME_INTE_STS state = *(CTL_IPP_IME_INTE_STS *)p_in;
	_isf_vdoprc_do_process(p_thisunit, 0);

	return 0;
}

//hook CTL_IPP_CBEVT_OUT_BUF
INT32 _vdoprc15_output_cb(UINT32 event, void *p_in, void *p_out)
{
	ISF_UNIT *p_thisunit = &isf_vdoprc15;
	VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
	CTL_IPP_OUT_BUF_INFO *p_info = (CTL_IPP_OUT_BUF_INFO *)p_in;
	VDO_FRAME* p_vdoframe = 0;
	UINT32 path, j;

#if (USE_VPE == ENABLE)
	if (p_ctx->vpe_mode) {
		_isf_vdoprc_vpe_oport_do_out_cb(p_thisunit, event, p_in, p_out);
		return 0;
	}
#endif
#if (USE_ISE == ENABLE)
	if (p_ctx->ise_mode) {
		_isf_vdoprc_ise_oport_do_out_cb(p_thisunit, event, p_in, p_out);
		return 0;
	}
#endif

	if (event == CTL_IPP_BUF_IO_NEW) { //new buffer

		UINT32 buf_size = p_info->buf_size;
		UINT32 buf_addr = (UINT32)&(p_info->vdo_frm);
		path = p_info->pid;
#if defined(_BSP_NA51023_)
		if (path == 4) {path = 0; buf_size |= 0x80000000;}
#endif
		j = _isf_vdoprc_oport_do_new(p_thisunit, ISF_OUT(path), buf_size, p_ctx->ddr, &buf_addr);
		if (j == 0xff) {
			p_info->buf_id = 0xff;
			p_info->buf_addr = 0;
            if(buf_addr==(UINT32)CTL_IPP_E_DROP) {
                p_info->err_msg = CTL_IPP_E_DROP;
            }

		} else {
			p_info->buf_id = j;
			p_info->buf_addr = buf_addr;
		}
		return 0;
	} else if (event == CTL_IPP_BUF_IO_PUSH) { //push buffer
		path = p_info->pid;
		j = p_info->buf_id;
		p_vdoframe = &(p_info->vdo_frm);
#if defined(_BSP_NA51023_)
		if (path == 4) path = 0;
#endif
		if(p_ctx->proc.st_osdmask & 0x01) {
			_isf_vdoprc_finish_input_osd(p_thisunit, ISF_IN(0));
			p_ctx->proc.st_osdmask &= ~0x01;
		}
		if(p_ctx->proc.st_osdmask & 0x02) {
			_isf_vdoprc_finish_input_mask(p_thisunit, ISF_IN(0));
			p_ctx->proc.st_osdmask &= ~0x02;
		}
		_isf_vdoprc_oport_do_push(p_thisunit, ISF_OUT(path), j, p_vdoframe);
		return 0;
	} else if (event == CTL_IPP_BUF_IO_LOCK) { //lock buffer
		path = p_info->pid;
		j = p_info->buf_id;
#if defined(_BSP_NA51023_)
		if (path == 4) path = 0;
#endif
		_isf_vdoprc_oport_do_lock(p_thisunit, ISF_OUT(path), j);
		return 0;
	} else if (event == CTL_IPP_BUF_IO_UNLOCK) { //unlock buffer
		path = p_info->pid;
		j = p_info->buf_id;
#if defined(_BSP_NA51023_)
		if (path == 4) path = 0;
#endif
		_isf_vdoprc_oport_do_unlock(p_thisunit, ISF_OUT(path), j, p_info->err_msg);
		return 0;
	} else if (event == CTL_IPP_BUF_IO_START) { //start buffer
		path = p_info->pid;
#if defined(_BSP_NA51023_)
		if (path == 4) path = 0;
#endif
		_isf_vdoprc_oport_do_start(p_thisunit, ISF_OUT(path));
		return 0;
	} else if (event == CTL_IPP_BUF_IO_STOP) { //start buffer
		path = p_info->pid;
#if defined(_BSP_NA51023_)
		if (path == 4) path = 0;
#endif
		_isf_vdoprc_oport_do_stop(p_thisunit, ISF_OUT(path));
		return 0;
	}
	return 0;
}

static void _vdoprc15_sync_cb(UINT32 path, UINT64 end, UINT64 cycle)
{
#if defined(_BSP_NA51023_)
	if (path == 4)path = 0;
#endif
}

/*
IPP: if sndevent data input to IPP (or after DIRECT MODE start), IPP will callback events in following:

ISR=>
	CTL_IPP_CBEVT_OUT_BUF cb => CTL_IPP_BUF_IO_NEW (output)
	  :
	CTL_IPP_CBEVT_OUT_BUF cb => CTL_IPP_BUF_IO_LOCK (output)
	CTL_IPP_CBEVT_OUT_BUF cb => CTL_IPP_BUF_IO_UNLOCK (output)
	  :
	CTL_IPP_CBEVT_ENG_IME_ISR cb => some engine ISR events (process)
	CTL_IPP_CBEVT_IN_BUF cb=> CTL_IPP_CBEVT_IN_BUF_PROCEND or CTL_IPP_CBEVT_IN_BUF_DROP (input)
Task=>
	CTL_IPP_CBEVT_OUT_BUF cb => CTL_IPP_BUF_IO_PUSH path 0 (output)
	CTL_IPP_CBEVT_OUT_BUF cb => CTL_IPP_BUF_IO_PUSH path 1 (output)
	CTL_IPP_CBEVT_OUT_BUF cb => CTL_IPP_BUF_IO_PUSH path 2 (output)
	CTL_IPP_CBEVT_OUT_BUF cb => CTL_IPP_BUF_IO_PUSH path 3 (output)
	CTL_IPP_CBEVT_OUT_BUF cb => CTL_IPP_BUF_IO_PUSH path 4 (output)
*/

static ISF_RV isf_vdoprc15_init(struct _ISF_UNIT *p_thisunit, void* p_ctx_buf)
{
	p_thisunit->refdata = p_ctx_buf;
	if (p_thisunit->refdata) {
		VDOPRC_CONTEXT* p_ctx = (VDOPRC_CONTEXT*)(p_thisunit->refdata);
		memset((void*)p_ctx, 0, sizeof(VDOPRC_CONTEXT));
		p_ctx->dev = 15;
		p_ctx->p_sem_poll = &(ISF_VDOPRC_OUTP_SEM_ID); //share the same sem
		p_ctx->on_input = _vdoprc15_input_cb;
		p_ctx->on_process = _vdoprc15_process_cb;
		p_ctx->on_output = _vdoprc15_output_cb;
		p_ctx->on_sync = _vdoprc15_sync_cb;
		p_ctx->on_osd = _vdoprc15_osd_cb;
		p_ctx->on_mask = _vdoprc15_mask_cb;
		return ISF_OK;
	}
	return ISF_ERR_NULL_POINTER;
}
static ISF_RV isf_vdoprc15_uninit(struct _ISF_UNIT *p_thisunit)
{
	p_thisunit->refdata = NULL;
	return ISF_OK;
}

ISF_UNIT isf_vdoprc15 = {
	.sign = ISF_SIGN_UNIT,
	.unit_name = "vdoprc15",
	.unit_module = MAKE_NVTMPP_MODULE('v', 'd', 'o', 'p', 'r', 'c', 0, 0),
	.port_in_count = ISF_VDOPRC_IN_NUM,
	.port_out_count = ISF_VDOPRC_OUT_NUM,
	.port_path_count = ISF_VDOPRC_PATH_NUM,
	.port_in = isf_vdoprc15_inputport_list,
	.port_out = isf_vdoprc15_outputport_list,
	.port_outcfg = isf_vdoprc15_outputport_list_cfg,
	.port_incaps = isf_vdoprc15_inputport_list_caps,
	.port_outcaps = isf_vdoprc15_outputport_list_caps,
	.port_outstate = isf_vdoprc15_outputport_list_state,
	.port_ininfo = isf_vdoprc15_outputinfolist_in,
	.port_outinfo = isf_vdoprc15_outputinfolist_out,
	.port_path = isf_vdoprc15_path_list,
	.do_init = isf_vdoprc15_init,
	.do_uninit = isf_vdoprc15_uninit,
	.do_bindinput = isf_vdoprc15_do_bindinput,
	.do_bindoutput = NULL,
	.do_setportparam = isf_vdoprc15_do_setparam,
	.do_getportparam = isf_vdoprc15_do_getparam,
	.do_update = isf_vdoprc15_do_updateport,
	.do_setportstruct = _isf_vdoprc_do_setparamstruct,
	.debug = NULL,
	.refdata = 0,
};

#endif
