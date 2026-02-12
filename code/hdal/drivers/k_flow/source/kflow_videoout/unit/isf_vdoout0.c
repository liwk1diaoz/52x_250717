//#include "frammap/frammap_if.h"
#include "isf_vdoout_int.h"
#include "isf_vdoout_dbg.h"

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  mapping
//  In1 <--> Out1
///////////////////////////////////////////////////////////////////////////////

//STATIC_ASSERT(ISF_VDOOUT_INPUT_MAX == ISF_VDOOUT_OUTPUT_MAX);

static ISF_PORT_CAPS *isf_vdoout0_inputportlist_caps[ISF_VDOOUT_INPUT_MAX];
static ISF_PORT *isf_vdoout0_inputportlist[ISF_VDOOUT_INPUT_MAX];
static ISF_PORT_CAPS *isf_vdoout0_outputportlist_caps[ISF_VDOOUT_OUTPUT_MAX];

static ISF_RV isf_vdoout0_do_setportparam(ISF_UNIT  *p_thisunit, UINT32 nport, UINT32 param, UINT32 value)
{
	return isf_vdoout_do_setportparam(p_thisunit,nport,param,value);
}
static UINT32 isf_vdoout0_do_getportparam(ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param)
{
	return isf_vdoout_do_getportparam(p_thisunit,nport,param);
}
static ISF_RV isf_vdoout0_do_setportstruct(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size)
{
    return isf_vdoout_do_setportstruct(p_thisunit,nport,param,p_struct,size);
}
static ISF_RV isf_vdoout0_do_getportstruct(struct _ISF_UNIT *p_thisunit, UINT32 nport, UINT32 param, UINT32* p_struct, UINT32 size)
{
    return isf_vdoout_do_getportstruct(p_thisunit,nport, param,p_struct,size);
}
static ISF_RV isf_vdoout0_bind_input(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport)
{
	{
		VDOOUT_CONTEXT* p_ctx = (VDOOUT_CONTEXT*)(p_thisunit->refdata);
		extern int set_src_unit(struct _ISF_UNIT *thisunit, struct _ISF_UNIT *srcunit);
		
		if(p_ctx->flow_ctrl & VDOOUT_FUNC_MERGE_WIN){
			int ret = set_src_unit(p_thisunit, p_srcunit);
			if(ret)
				DBG_ERR("fail to register source unit for mergin window\n");
		}
	}
    return isf_vdoout_bind_input(p_thisunit,iport,p_srcunit,oport);
}
static ISF_RV isf_vdoout0_bind_output(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport)
{
    return isf_vdoout_bind_output(p_thisunit,iport,p_srcunit,oport);
}

static ISF_RV isf_vdoout0_inputport_do_push(ISF_PORT *p_port, ISF_DATA *p_data, INT32 wait_ms)
{
    return isf_vdoout_inputport_do_push(DEV_ID_0,p_port,p_data,wait_ms);
}

static ISF_RV isf_vdoout0_do_update(ISF_UNIT  *p_thisunit, UINT32 oport, ISF_PORT_CMD cmd)
{
    return isf_vdoout_do_update(DEV_ID_0,p_thisunit,oport,cmd);
}

static ISF_PORT_CAPS isf_vdoout_inputport1_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = isf_vdoout0_inputport_do_push,
	.do_pull = NULL,
};
static ISF_PORT_CAPS isf_vdoout_inputport2_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = isf_vdoout0_inputport_do_push,
	.do_pull = NULL,
};
static ISF_PORT_CAPS isf_vdoout_inputport3_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = isf_vdoout0_inputport_do_push,
	.do_pull = NULL,
};
static ISF_PORT_CAPS isf_vdoout_inputport4_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = isf_vdoout0_inputport_do_push,
	.do_pull = NULL,
};
static ISF_PORT_CAPS isf_vdoout_inputport5_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = isf_vdoout0_inputport_do_push,
	.do_pull = NULL,
};
static ISF_PORT_CAPS isf_vdoout_inputport6_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = isf_vdoout0_inputport_do_push,
	.do_pull = NULL,
};
static ISF_PORT_CAPS isf_vdoout_inputport7_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = isf_vdoout0_inputport_do_push,
	.do_pull = NULL,
};
static ISF_PORT_CAPS isf_vdoout_inputport8_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = isf_vdoout0_inputport_do_push,
	.do_pull = NULL,
};
static ISF_PORT_CAPS isf_vdoout_inputport9_caps = {
	.connecttype_caps = ISF_CONNECT_PUSH,
	.do_push = isf_vdoout0_inputport_do_push,
	.do_pull = NULL,
};

static ISF_PORT isf_vdoout_inputport1 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(0),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoout0,
	.p_srcunit = NULL,
};
static ISF_PORT isf_vdoout_inputport2 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(1),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoout0,
	.p_srcunit = NULL,
};
static ISF_PORT isf_vdoout_inputport3 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(2),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoout0,
	.p_srcunit = NULL,
};
static ISF_PORT isf_vdoout_inputport4 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(3),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoout0,
	.p_srcunit = NULL,
};
static ISF_PORT isf_vdoout_inputport5 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(4),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoout0,
	.p_srcunit = NULL,
};
static ISF_PORT isf_vdoout_inputport6 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(5),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoout0,
	.p_srcunit = NULL,
};
static ISF_PORT isf_vdoout_inputport7 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(6),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoout0,
	.p_srcunit = NULL,
};
static ISF_PORT isf_vdoout_inputport8 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(7),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoout0,
	.p_srcunit = NULL,
};
static ISF_PORT isf_vdoout_inputport9 = {
	.sign = ISF_SIGN_PORT,
	.iport = ISF_IN(8),
	.connecttype = ISF_CONNECT_PUSH,
	.p_destunit = &isf_vdoout0,
	.p_srcunit = NULL,
};

static ISF_PORT *isf_vdoout0_inputportlist[ISF_VDOOUT_INPUT_MAX] = {
	&isf_vdoout_inputport1,
	&isf_vdoout_inputport2,
	&isf_vdoout_inputport3,
	&isf_vdoout_inputport4,
	&isf_vdoout_inputport5,
	&isf_vdoout_inputport6,
	&isf_vdoout_inputport7,
	&isf_vdoout_inputport8,
	&isf_vdoout_inputport9,
};
static ISF_PORT_CAPS *isf_vdoout0_inputportlist_caps[ISF_VDOOUT_INPUT_MAX] = {
	&isf_vdoout_inputport1_caps,
	&isf_vdoout_inputport2_caps,
	&isf_vdoout_inputport3_caps,
	&isf_vdoout_inputport4_caps,
	&isf_vdoout_inputport5_caps,
	&isf_vdoout_inputport6_caps,
	&isf_vdoout_inputport7_caps,
	&isf_vdoout_inputport8_caps,
	&isf_vdoout_inputport9_caps,
};


static ISF_PORT_CAPS isf_vdoout_outputport1_caps = {
	.connecttype_caps = ISF_CONNECT_NONE,
	.do_push = NULL,
	.do_pull = NULL,
};

static ISF_PORT *isf_vdoout0_outputportlist[ISF_VDOOUT_OUTPUT_MAX] = {0};
static ISF_PORT *isf_vdoout0_outputportlist_cfg[ISF_VDOOUT_OUTPUT_MAX] = {0};
static ISF_PORT_CAPS *isf_vdoout0_outputportlist_caps[ISF_VDOOUT_OUTPUT_MAX] = {
	&isf_vdoout_outputport1_caps,
};

static ISF_STATE isf_vdoout_outputport1_state[ISF_VDOOUT_OUTPUT_MAX] = {0};
static ISF_STATE *isf_vdoout0_outputportlist_state[ISF_VDOOUT_OUTPUT_MAX] = {
	&isf_vdoout_outputport1_state[0],
};

static ISF_PORT_PATH isf_vdoout0_pathlist[ISF_VDOOUT_PATH_NUM] = {
	{&isf_vdoout0, ISF_IN(0), ISF_OUT(0)},
	{&isf_vdoout0, ISF_IN(1), ISF_OUT(0)},
	{&isf_vdoout0, ISF_IN(2), ISF_OUT(0)},
	{&isf_vdoout0, ISF_IN(3), ISF_OUT(0)},
	{&isf_vdoout0, ISF_IN(4), ISF_OUT(0)},
	{&isf_vdoout0, ISF_IN(5), ISF_OUT(0)},
	{&isf_vdoout0, ISF_IN(6), ISF_OUT(0)},
	{&isf_vdoout0, ISF_IN(7), ISF_OUT(0)},
	{&isf_vdoout0, ISF_IN(8), ISF_OUT(0)},
};

static ISF_INFO isf_vdoout_outputinfo1_in = {0};
static ISF_INFO isf_vdoout_outputinfo2_in = {0};
static ISF_INFO isf_vdoout_outputinfo3_in = {0};
static ISF_INFO isf_vdoout_outputinfo4_in = {0};
static ISF_INFO isf_vdoout_outputinfo5_in = {0};
static ISF_INFO isf_vdoout_outputinfo6_in = {0};
static ISF_INFO isf_vdoout_outputinfo7_in = {0};
static ISF_INFO isf_vdoout_outputinfo8_in = {0};
static ISF_INFO isf_vdoout_outputinfo9_in = {0};

static ISF_INFO *isf_vdoout0_outputinfolist_in[ISF_VDOOUT_INPUT_MAX] = {
	&isf_vdoout_outputinfo1_in,
	&isf_vdoout_outputinfo2_in,
	&isf_vdoout_outputinfo3_in,
	&isf_vdoout_outputinfo4_in,
	&isf_vdoout_outputinfo5_in,
	&isf_vdoout_outputinfo6_in,
	&isf_vdoout_outputinfo7_in,
	&isf_vdoout_outputinfo8_in,
	&isf_vdoout_outputinfo9_in,

};

static ISF_INFO isf_vdoout_outputinfo1_out = {0};
static ISF_INFO *isf_vdoout0_outputinfolist_out[ISF_VDOOUT_OUTPUT_MAX] = {
	&isf_vdoout_outputinfo1_out,
};

static VDOOUT_CONTEXT isf_vdoout0_ctx = {DEV_ID_0,0};

ISF_UNIT isf_vdoout0 = {
	.sign = ISF_SIGN_UNIT,
	.unit_name = "vdoout0",
	.unit_module = MAKE_NVTMPP_MODULE('v', 'd', 'o', 'o', 'u', 't', 0, 0),
	.port_in_count = ISF_VDOOUT_INPUT_MAX,
	.port_out_count = ISF_VDOOUT_OUTPUT_MAX,
	.port_path_count = ISF_VDOOUT_PATH_NUM,
	.port_in = isf_vdoout0_inputportlist,
	.port_out = isf_vdoout0_outputportlist,
	.port_outcfg = isf_vdoout0_outputportlist_cfg,
	.port_incaps = isf_vdoout0_inputportlist_caps,
	.port_outcaps = isf_vdoout0_outputportlist_caps,
	.port_outstate = isf_vdoout0_outputportlist_state,
	.port_ininfo = isf_vdoout0_outputinfolist_in,
	.port_outinfo = isf_vdoout0_outputinfolist_out,
	.port_path = isf_vdoout0_pathlist,
	.do_command = isf_vdopout_do_command, //device 0 only
	.do_bindinput = isf_vdoout0_bind_input,
	.do_bindoutput = isf_vdoout0_bind_output,
	.do_setportparam = isf_vdoout0_do_setportparam,
	.do_getportparam = isf_vdoout0_do_getportparam,
	.do_setportstruct = isf_vdoout0_do_setportstruct,
	.do_getportstruct = isf_vdoout0_do_getportstruct,
	.do_update = isf_vdoout0_do_update,
	.refdata = (void *)&isf_vdoout0_ctx,
	.debug = NULL,
};

///////////////////////////////////////////////////////////////////////////////


