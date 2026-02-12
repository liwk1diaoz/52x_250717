//#include "frammap/frammap_if.h"
#include "isf_vdoout_int.h"
#include "isf_vdoout_dbg.h"
#if (DEV_ID_MAX > 1)

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  mapping
//  In1 <--> Out1
///////////////////////////////////////////////////////////////////////////////

STATIC_ASSERT(ISF_VDOOUT_INPUT_MAX == ISF_VDOOUT_OUTPUT_MAX);

static ISF_PORT_CAPS *isf_vdoout1_inputportlist_caps[ISF_VDOOUT_INPUT_MAX];
static ISF_PORT *isf_vdoout1_inputportlist[ISF_VDOOUT_INPUT_MAX];
static ISF_PORT_CAPS *isf_vdoout1_outputportlist_caps[ISF_VDOOUT_OUTPUT_MAX];

static ISF_RV isf_vdoout1_do_setportparam(ISF_UNIT  *p_thisunit, UINT32 nport, UINT32 param, UINT32 value)
{
	return isf_vdoout_do_setportparam(p_thisunit,nport,param,value);
}
static UINT32 isf_vdoout1_do_getportparam(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param)
{
	return isf_vdoout_do_getportparam(p_thisunit,nport,param);
}
static ISF_RV isf_vdoout1_do_setportstruct(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size)
{
    return isf_vdoout_do_setportstruct(p_thisunit,nport,param,p_struct,size);
}
static ISF_RV isf_vdoout1_do_getportstruct(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size)
{
    return isf_vdoout_do_getportstruct(p_thisunit,nport, param,p_struct,size);
}
static ISF_RV isf_vdoout1_bind_input(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport)
{
    return isf_vdoout_bind_input(p_thisunit,iport,p_srcunit,oport);
}
static ISF_RV isf_vdoout1_bind_output(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport)
{
    return isf_vdoout_bind_output(p_thisunit,iport,p_srcunit,oport);
}

static ISF_RV isf_vdoout1_inputport_do_push(ISF_PORT *p_port, ISF_DATA *p_data, INT32 wait_ms)
{
    return isf_vdoout_inputport_do_push(DEV_ID_1,p_port,p_data,wait_ms);
}

static ISF_RV isf_vdoout1_do_update(ISF_UNIT  *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd)
{
    return isf_vdoout_do_update(DEV_ID_1,p_thisunit,oport,cmd);
}

static ISF_PORT_CAPS isf_vdoout_inputport1_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = isf_vdoout1_inputport_do_push,
	.do_pull = NULL,
};
static ISF_PORT isf_vdoout_inputport1 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(0),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoout1,
	.p_srcunit = NULL,
};

static ISF_PORT *isf_vdoout1_inputportlist[ISF_VDOOUT_INPUT_MAX] = {
	&isf_vdoout_inputport1,
};
static ISF_PORT_CAPS *isf_vdoout1_inputportlist_caps[ISF_VDOOUT_INPUT_MAX] = {
	&isf_vdoout_inputport1_caps,
};


static ISF_PORT_CAPS isf_vdoout_outputport1_caps = {
	.connecttype_caps = ISF_CONNECT_NONE,
	.do_push = NULL,
	.do_pull = NULL,
};

static ISF_PORT *isf_vdoout1_outputportlist[ISF_VDOOUT_OUTPUT_MAX] = {0};
static ISF_PORT *isf_vdoout1_outputportlist_cfg[ISF_VDOOUT_OUTPUT_MAX] = {0};
static ISF_PORT_CAPS *isf_vdoout1_outputportlist_caps[ISF_VDOOUT_OUTPUT_MAX] = {
	&isf_vdoout_outputport1_caps,
};

static ISF_STATE isf_vdoout_outputport1_state[ISF_VDOOUT_OUTPUT_MAX] = {0};
static ISF_STATE *isf_vdoout1_outputportlist_state[ISF_VDOOUT_OUTPUT_MAX] = {
	&isf_vdoout_outputport1_state[0],
};

static ISF_PORT_PATH isf_vdoout1_pathlist[ISF_VDOOUT_PATH_NUM] = {
	{&isf_vdoout1, ISF_IN(0), ISF_OUT(0)},
};

static ISF_INFO isf_vdoout_outputinfo1_in = {0};
static ISF_INFO *isf_vdoout1_outputinfolist_in[ISF_VDOOUT_INPUT_MAX] = {
	&isf_vdoout_outputinfo1_in,
};

static ISF_INFO isf_vdoout_outputinfo1_out = {0};
static ISF_INFO *isf_vdoout1_outputinfolist_out[ISF_VDOOUT_OUTPUT_MAX] = {
	&isf_vdoout_outputinfo1_out,
};

static VDOOUT_CONTEXT isf_vdoout1_ctx = {DEV_ID_1,0};

ISF_UNIT isf_vdoout1 = {
	.sign = ISF_SIGN_UNIT,
	.unit_name = "vdoout1",
	.unit_module = MAKE_NVTMPP_MODULE('v', 'd', 'o', 'o', 'u', 't', 0, 0),
	.port_in_count = ISF_VDOOUT_INPUT_MAX,
	.port_out_count = ISF_VDOOUT_OUTPUT_MAX,
	.port_path_count = ISF_VDOOUT_PATH_NUM,
	.port_in = isf_vdoout1_inputportlist,
	.port_out = isf_vdoout1_outputportlist,
	.port_outcfg = isf_vdoout1_outputportlist_cfg,
	.port_incaps = isf_vdoout1_inputportlist_caps,
	.port_outcaps = isf_vdoout1_outputportlist_caps,
	.port_outstate = isf_vdoout1_outputportlist_state,
	.port_ininfo = isf_vdoout1_outputinfolist_in,
	.port_outinfo = isf_vdoout1_outputinfolist_out,
	.port_path = isf_vdoout1_pathlist,
	.do_bindinput = isf_vdoout1_bind_input,
	.do_bindoutput = isf_vdoout1_bind_output,
	.do_setportparam = isf_vdoout1_do_setportparam,
	.do_getportparam = isf_vdoout1_do_getportparam,
	.do_setportstruct = isf_vdoout1_do_setportstruct,
	.do_getportstruct = isf_vdoout1_do_getportstruct,
	.do_update = isf_vdoout1_do_update,
	.refdata = (void *)&isf_vdoout1_ctx,
	.debug = NULL,
};

///////////////////////////////////////////////////////////////////////////////

#endif
