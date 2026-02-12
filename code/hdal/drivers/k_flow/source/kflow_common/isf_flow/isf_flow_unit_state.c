#include "isf_flow_int.h"
#include "isf_flow_api.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_flow_ps
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_flow_ps_debug_level = __DBGLVL__;
//module_param_named(isf_flow_ps_debug_level, isf_flow_ps_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_flow_ps_debug_level, "flow debug level");
///////////////////////////////////////////////////////////////////////////////

#define DUMP_STREAM     DISABLE
#define DUMP_SHARE      DISABLE
#define DUMP_AUTO_DROP  DISABLE
#define DUMP_DIRTY      DISABLE

#if (DUMP_DIRTY == ENABLE)
static CHAR* str_update[5]={"","RUNTIME","ds-RUNTIME","ds-READYTIME","ds-OFFTIME"};
#endif

ISF_PORT *_isf_unit_in(ISF_UNIT *p_unit, UINT32 iport)
{
	if (p_unit == NULL) {
		DBG_ERR("NULL unit!\r\n");
		return NULL;
	}
	if ((iport - ISF_IN_BASE) < p_unit->port_in_count) {
		return p_unit->port_in[iport - ISF_IN_BASE];
	}
	DBG_ERR("\"%s\".in[%d] is out of id!\r\n",
			p_unit->unit_name, iport - ISF_IN_BASE);
	return NULL;
}
//EXPORT_SYMBOL(_isf_unit_in);

ISF_PORT *_isf_unit_out(ISF_UNIT *p_unit, UINT32 oport)
{
	if (p_unit == NULL) {
		DBG_ERR("NULL unit!\r\n");
		return NULL;
	}
	if ((oport - ISF_OUT_BASE) < p_unit->port_out_count) {
		return p_unit->port_out[oport - ISF_OUT_BASE];
	}
	DBG_ERR("\"%s\".out[%d] is out of id!\r\n",
			p_unit->unit_name, oport - ISF_OUT_BASE);
	return NULL;
}
//EXPORT_SYMBOL(_isf_unit_out);

ISF_PORT *_isf_unit_port(ISF_UNIT *p_unit, UINT32 nport)
{
	if (p_unit == NULL) {
		DBG_ERR("NULL unit!\r\n");
		return NULL;
	}
	if (nport >= ISF_IN_BASE) {
		if ((nport - ISF_IN_BASE) < p_unit->port_in_count) {
			UINT32 i = nport - ISF_IN_BASE;
			ISF_PORT *p_iport = p_unit->port_in[i];
			return p_iport;
		} 
		DBG_ERR("\"%s\".in[%d] is out of id!\r\n",
				p_unit->unit_name, nport - ISF_IN_BASE);
	} else {
		if ((nport - ISF_OUT_BASE) < p_unit->port_out_count) {
			UINT32 i = nport - ISF_OUT_BASE;
			ISF_PORT *p_oport = p_unit->port_out[i];
			return p_oport;
		}
		DBG_ERR("\"%s\".out[%d] is out of id!\r\n",
				p_unit->unit_name, nport - ISF_OUT_BASE);
	}
	return NULL;
}
//EXPORT_SYMBOL(_isf_unit_port);

ISF_PORT *_isf_bind_port(ISF_UNIT *p_unit, UINT32 nport)
{
	if (p_unit == NULL) {
		DBG_ERR("NULL unit!\r\n");
		return NULL;
	}
	if (nport >= ISF_IN_BASE) {
		if ((nport - ISF_IN_BASE) < p_unit->port_in_count) {
			UINT32 i = nport - ISF_IN_BASE;
			ISF_PORT *p_iport = p_unit->port_in[i];
			if(!p_iport)
				return NULL;
			if(!p_iport->p_srcunit)
				return NULL;
			return p_iport->p_srcunit->port_out[p_iport->oport];
		} 
		DBG_ERR("\"%s\".in[%d] is out of id!\r\n",
				p_unit->unit_name, nport - ISF_IN_BASE);
	} else {
		if ((nport - ISF_OUT_BASE) < p_unit->port_out_count) {
			UINT32 i = nport - ISF_OUT_BASE;
			ISF_PORT *p_oport = p_unit->port_out[i];
			if(!p_oport)
				return NULL;
			if(!p_oport->p_destunit)
				return NULL;
			return p_oport->p_destunit->port_in[p_oport->iport];
		}
		DBG_ERR("\"%s\".out[%d] is out of id!\r\n",
				p_unit->unit_name, nport - ISF_OUT_BASE);
	}
	return NULL;
}
//EXPORT_SYMBOL(_isf_unit_port);

ISF_INFO *_isf_unit_info(ISF_UNIT *p_unit, UINT32 nport)
{
	if (p_unit == NULL) {
		DBG_ERR("NULL unit!\r\n");
		return NULL;
	}
	if (nport >= ISF_IN_BASE) {
		if ((nport - ISF_IN_BASE) < p_unit->port_in_count) {
			UINT32 i = nport - ISF_IN_BASE;
			ISF_INFO *p_iinfo = p_unit->port_ininfo[i];
			return p_iinfo;
		} 
		DBG_ERR("\"%s\".in[%d] is out of id!\r\n",
				p_unit->unit_name, nport - ISF_IN_BASE);
	} else {
		if ((nport - ISF_OUT_BASE) < p_unit->port_out_count) {
			UINT32 i = nport - ISF_OUT_BASE;
			ISF_INFO *p_oinfo = p_unit->port_outinfo[i];
			return p_oinfo;
		}
		DBG_ERR("\"%s\".out[%d] is out of id!\r\n",
				p_unit->unit_name, nport - ISF_OUT_BASE);
	}
	return NULL;
}
//EXPORT_SYMBOL(_isf_unit_port);

ISF_INFO *_isf_bind_info(ISF_UNIT *p_unit, UINT32 nport)
{
	if (p_unit == NULL) {
		DBG_ERR("NULL unit!\r\n");
		return NULL;
	}
	if (nport >= ISF_IN_BASE) {
		if ((nport - ISF_IN_BASE) < p_unit->port_in_count) {
			UINT32 i = nport - ISF_IN_BASE;
			ISF_PORT *p_iport = p_unit->port_in[i];
			if(!p_iport)
				return NULL;
			if(!p_iport->p_srcunit)
				return NULL;
			return p_iport->p_srcunit->port_outinfo[p_iport->oport - ISF_OUT_BASE];
		} 
		DBG_ERR("\"%s\".in[%d] is out of id!\r\n",
				p_unit->unit_name, nport - ISF_IN_BASE);
	} else {
		if ((nport - ISF_OUT_BASE) < p_unit->port_out_count) {
			UINT32 i = nport - ISF_OUT_BASE;
			ISF_PORT *p_oport = p_unit->port_out[i];
			if(!p_oport)
				return NULL;
			if(!p_oport->p_destunit)
				return NULL;
			return p_oport->p_destunit->port_ininfo[p_oport->iport - ISF_IN_BASE];
		}
		DBG_ERR("\"%s\".out[%d] is out of id!\r\n",
				p_unit->unit_name, nport - ISF_OUT_BASE);
	}
	return NULL;
}
//EXPORT_SYMBOL(_isf_unit_port);

ISF_PORT_STATE _isf_unit_state(ISF_UNIT *p_unit, UINT32 oport)
{
	if (!p_unit) {
		DBG_ERR("NULL unit!\r\n");
		return ISF_PORT_STATE_OFF;
	}

	if (!((oport < (int)ISF_IN_BASE) && (oport < p_unit->port_out_count))) {
		DBG_ERR(" \"%s\".out[%d] is out of id\r\n",
				p_unit->unit_name, oport - ISF_OUT_BASE);
		return ISF_PORT_STATE_OFF;
	}

	return (p_unit->port_outstate[oport - ISF_OUT_BASE])->state;
}
//EXPORT_SYMBOL(_isf_unit_state);


static ISF_RV isf_unit_default_bind_input(struct _ISF_UNIT *p_thisunit, UINT32 iport, struct _ISF_UNIT *p_srcunit, UINT32 oport)
{
	// verify connect limit
	// fill ImgInfo to port
	{
		ISF_PORT *p_inputport;
		ISF_PORT *p_outputport;
		p_inputport = _isf_unit_in(p_thisunit, iport);
		if (!p_inputport) {
			return ISF_ERR_INVALID_PORT_ID;
		}
		p_outputport = _isf_unit_out(p_thisunit, iport - ISF_IN_BASE + ISF_OUT_BASE);
		if (!p_outputport) {
			return ISF_ERR_INVALID_PORT_ID;
		}
#if _TODO
		_isf_unit_default_get_imginfo(p_outputport, (ISF_VDO_INFO *) & (p_inputport->info));
#endif
	}
	return ISF_OK;
}
static ISF_RV isf_unit_default_bind_output(struct _ISF_UNIT *p_thisunit, UINT32 oport, struct _ISF_UNIT *p_destunit, UINT32 iport)
{
	// verify connect limit
	// verify ImgInfo from port
	return ISF_OK;
}

#if 0
#define ISF_PATH_CLEAN 		0
#define ISF_PATH_SET 			1
#define ISF_PATH_ISEQUAL 		2
#define ISF_PATH_ISCLEAN 		3

ISF_RV _isf_unit_path_do(struct _ISF_UNIT *p_thisunit, UINT32 act, UINT32 nport, ISF_STREAM *p_stream)
{
	UINT32 port_path_count;
	ISF_PORT_PATH *p_path;
	if (!p_thisunit) {
		return ISF_ERR_NULL_POINTER;
	}
	port_path_count = p_thisunit->port_path_count;
	p_path = p_thisunit->port_path;
	if (!p_path) {
		return ISF_ERR_NULL_POINTER;
	}
	if (act == 1) { //bind
		if (p_thisunit->port_in_count <= p_thisunit->port_out_count) {
			if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
				//oport
				UINT32 i;
				p_path = p_thisunit->port_path;
				for (i = 0; i < port_path_count; i++) {
#if 0 //(DUMP_STREAM == ENABLE)
					DBG_MSG(" bind-stream(\"%s\") to \"%s\".path.in[%d]out[%d] ===> stream(\"%s\")\r\n",
							p_stream->unit_name, p_thisunit->unit_name, p_path->iport - ISF_IN_BASE, p_path->oport - ISF_OUT_BASE, (p_path->p_stream) ? (p_path->p_stream->unit_name) : ("x"));
#endif

					if ((p_path->oport == nport) && (p_path->p_stream == 0)) {
						p_path->p_stream = p_stream;
#if (DUMP_STREAM == ENABLE)
						DBG_MSG(" bind-stream(\"%s\") to \"%s\".path.in[%d]out[%d] ===> stream(\"%s\")\r\n",
								p_stream->stream_name, p_thisunit->unit_name, p_path->iport - ISF_IN_BASE, p_path->oport - ISF_OUT_BASE, (p_path->p_stream) ? (p_path->p_stream->unit_name) : ("x"));
#endif
						return ISF_OK;
					}
					p_path++;
				}
				DBG_ERR("bind stream(\"%s\") to \"%s\".out[%d] is failed! (out of working path)\r\n",
						p_stream->stream_name, p_thisunit->unit_name, nport - ISF_OUT_BASE);
				return ISF_ERR_ALREADY_CONNECT;
			} else {
				//iport
				return ISF_OK;
			}
		} else { //if (p_thisunit->port_in_count > p_thisunit->port_out_count)
			if ((nport >= ISF_IN_BASE) && (nport - ISF_IN_BASE < p_thisunit->port_in_count)) {
				//iport
				UINT32 i;
				p_path = p_thisunit->port_path;
				for (i = 0; i < port_path_count; i++) {
#if 0 //(DUMP_STREAM == ENABLE)
					DBG_MSG(" bind-stream(\"%s\") to \"%s\".path.in[%d]out[%d] ===> stream(\"%s\")\r\n",
							p_stream->unit_name, p_thisunit->unit_name, p_path->iport - ISF_IN_BASE, p_path->oport - ISF_OUT_BASE, (p_path->p_stream) ? (p_path->p_stream->unit_name) : ("x"));
#endif

					if ((p_path->iport == nport) && (p_path->p_stream == 0)) {
#if (DUMP_STREAM == ENABLE)
						DBG_MSG(" bind-stream(\"%s\") to \"%s\".path.in[%d]out[%d] ===> stream(\"%s\")\r\n",
								p_stream->stream_name, p_thisunit->unit_name, p_path->iport - ISF_IN_BASE, p_path->oport - ISF_OUT_BASE, (p_path->p_stream) ? (p_path->p_stream->unit_name) : ("x"));
#endif
						p_path->p_stream = p_stream;
						return ISF_OK;
					}
					p_path++;
				}
				DBG_ERR("bind stream(\"%s\") to \"%s\".in[%d] is failed! (out of working path)\r\n",
						p_stream->stream_name, p_thisunit->unit_name, nport - ISF_IN_BASE);
				return ISF_ERR_ALREADY_CONNECT;
			} else {
				//oport
				return ISF_OK;
			}
		}
	} else if (act == 0) { //unbind
		if (p_thisunit->port_in_count <= p_thisunit->port_out_count) {
			if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
				//oport
				UINT32 i;
				p_path = p_thisunit->port_path;
				for (i = 0; i < port_path_count; i++) {
#if 0 //(DUMP_STREAM == ENABLE)
					DBG_MSG(" unbind-stream(\"%s\") to \"%s\".path.in[%d]out[%d] ===> stream(\"%s\")\r\n",
							p_stream->stream_name, p_thisunit->unit_name, p_path->iport - ISF_IN_BASE, p_path->oport - ISF_OUT_BASE, (p_path->p_stream) ? (p_path->p_stream->unit_name) : ("x"));
#endif

					if ((p_path->oport == nport) && (p_path->p_stream == p_stream)) {
#if (DUMP_STREAM == ENABLE)
						DBG_MSG(" unbind-stream(\"%s\") to \"%s\".path.in[%d]out[%d] ===> stream(\"%s\")\r\n",
								p_stream->stream_name, p_thisunit->unit_name, p_path->iport - ISF_IN_BASE, p_path->oport - ISF_OUT_BASE, (p_path->p_stream) ? (p_path->p_stream->unit_name) : ("x"));
#endif
						p_path->p_stream = 0;
						return ISF_OK;
					}
					p_path++;
				}
				DBG_ERR("unbind stream(\"%s\") from \"%s\".out[%d] is fail! (no match)\r\n",
						p_stream->stream_name, p_thisunit->unit_name, nport - ISF_OUT_BASE);
				return ISF_ERR_NOT_CONNECT_YET;
			} else {
				//iport
#if (DUMP_STREAM == ENABLE)
				DBG_MSG("unbind stream(\"%s\") from \"%s\".in[%d] - skip\r\n",
						p_stream->stream_name, p_thisunit->unit_name, nport - ISF_IN_BASE);
#endif
				return ISF_OK;
			}
		} else { //if (p_thisunit->port_in_count > p_thisunit->port_out_count)
			if ((nport >= ISF_IN_BASE) && (nport - ISF_IN_BASE < p_thisunit->port_in_count)) {
				//iport
				UINT32 i;
				p_path = p_thisunit->port_path;
				for (i = 0; i < port_path_count; i++) {
#if 0// (DUMP_STREAM == ENABLE)
					DBG_MSG(" unbind-stream(\"%s\") to \"%s\".path.in[%d]out[%d] ===> stream(\"%s\")\r\n",
							p_stream->unit_name, p_thisunit->unit_name, p_path->iport - ISF_IN_BASE, p_path->oport - ISF_OUT_BASE, (p_path->p_stream) ? (p_path->p_stream->unit_name) : ("x"));
#endif

					if ((p_path->iport == nport) && (p_path->p_stream == p_stream)) {
#if (DUMP_STREAM == ENABLE)
						DBG_MSG(" unbind-stream(\"%s\") to \"%s\".path.in[%d]out[%d] ===> stream(\"%s\")\r\n",
								p_stream->stream_name, p_thisunit->unit_name, p_path->iport - ISF_IN_BASE, p_path->oport - ISF_OUT_BASE, (p_path->p_stream) ? (p_path->p_stream->unit_name) : ("x"));
#endif
						p_path->p_stream = 0;
						return ISF_OK;
					}
					p_path++;
				}
				DBG_ERR("unbind stream(\"%s\") from \"%s\".in[%d] is fail! (no macth)\r\n",
						p_stream->stream_name, p_thisunit->unit_name, nport - ISF_IN_BASE);
				return ISF_ERR_NOT_CONNECT_YET;
			} else {
				//oport
#if (DUMP_STREAM == ENABLE)
				DBG_MSG("unbind stream(\"%s\") from \"%s\".out[%d] - skip\r\n",
						p_stream->stream_name, p_thisunit->unit_name, nport - ISF_OUT_BASE);
#endif
				return ISF_OK;
			}
		}
	} else if (act == 2) { //compare p_stream
		if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
			//oport
			UINT32 i;
			//DBG_MSG(" cmp-stream(\"%s\") to \"%s\".path.out[%d]\r\n",
			//    p_stream->stream_name, p_thisunit->unit_name, nport-ISF_OUT_BASE);
			//check any stream is equal?
			p_path = p_thisunit->port_path;
			for (i = 0; i < port_path_count; i++) {
				if (p_path->oport == nport) {
					if (p_path->p_stream == p_stream) {
#if (DUMP_STREAM == ENABLE)
						DBG_MSG(" cmp-stream(\"%s\") to \"%s\".path.out[%d] ===> %d: in[%d]out[%d] ...match!\r\n",
								p_stream->stream_name, p_thisunit->unit_name, nport - ISF_OUT_BASE, i, p_path->iport - ISF_IN_BASE, p_path->oport - ISF_OUT_BASE);
#endif
						return ISF_OK;
					}
					if (p_path->p_stream != 0) {
#if (DUMP_STREAM == ENABLE)
						DBG_MSG(" cmp-stream(\"%s\") to \"%s\".path.out[%d] ===> %d: in[%d]out[%d] ...not match! (used by stream(\"%s\"))\r\n",
								p_stream->stream_name, p_thisunit->unit_name, nport - ISF_OUT_BASE, i, p_path->iport - ISF_IN_BASE, p_path->oport - ISF_OUT_BASE, p_path->p_stream->unit_name);
#endif
					}
				}
				p_path++;
			}
			//DBG_MSG(" cmp-stream(\"%s\") to \"%s\".path.out[%d] ===> x\r\n",
			//    p_stream->stream_name, p_thisunit->unit_name, nport-ISF_OUT_BASE);
			return ISF_ERR_FAILED;
		} else if ((nport >= ISF_IN_BASE) && (nport - ISF_IN_BASE < p_thisunit->port_in_count)) {
			//iport
			UINT32 i;
			//DBG_MSG(" cmp-stream(\"%s\") to \"%s\".path.in[%d]\r\n",
			//    p_stream->stream_name, p_thisunit->unit_name, nport-ISF_IN_BASE);
			//check any stream is equal?
			p_path = p_thisunit->port_path;
			for (i = 0; i < port_path_count; i++) {
				if (p_path->iport == nport) {
					if (p_path->p_stream == p_stream) {
#if (DUMP_STREAM == ENABLE)
						DBG_MSG(" cmp-stream(\"%s\") to \"%s\".path.in[%d] ===> %d: in[%d]out[%d] ...match!\r\n",
								p_stream->stream_name, p_thisunit->unit_name, nport - ISF_IN_BASE, i, p_path->iport - ISF_IN_BASE, p_path->oport - ISF_OUT_BASE);
#endif
						return ISF_OK;
					}
					if (p_path->p_stream != 0) {
#if (DUMP_STREAM == ENABLE)
						DBG_MSG(" cmp-stream(\"%s\") to \"%s\".path.in[%d] ===> %d: in[%d]out[%d] ...not match! (used by stream(\"%s\"))\r\n",
								p_stream->stream_name, p_thisunit->unit_name, nport - ISF_IN_BASE, i, p_path->iport - ISF_IN_BASE, p_path->oport - ISF_OUT_BASE, p_path->p_stream->unit_name);
#endif
					}
				}
				p_path++;
			}
			//DBG_MSG(" cmp-stream(\"%s\") to \"%s\".path.in[%d] ===> x\r\n",
			//    p_stream->stream_name, p_thisunit->unit_name, nport-ISF_IN_BASE);
			return ISF_ERR_FAILED;
		}
	} else if (act == 3) { //check is share p_stream
		if ((nport < (int)ISF_IN_BASE) && (nport < p_thisunit->port_out_count)) {
			//oport
			UINT32 i;
			if (NULL == p_stream) {
				//check all stream is zero?
				p_path = p_thisunit->port_path;
				for (i = 0; i < port_path_count; i++) {
					if (p_path->oport == nport) {
						if (p_path->p_stream != NULL) {
#if (DUMP_SHARE == ENABLE)
							DBG_DUMP(" still-share-stream(\"%s\") to \"%s\".path.out[%d] ===> %d: in[%d]out[%d] ...match!\r\n",
									 p_path->p_stream->stream_name, p_thisunit->unit_name, nport - ISF_OUT_BASE, i, p_path->iport - ISF_IN_BASE, p_path->oport - ISF_OUT_BASE);
#endif
							return ISF_ERR_FAILED;
						}
					}
					p_path++;
				}
				//DBG_MSG(" cmp-stream(\"%s\") to \"%s\".path.out[%d] ===> x\r\n",
				//    p_stream->stream_name, p_thisunit->unit_name, nport-ISF_OUT_BASE);
				return ISF_OK;
			}

			//check all stream is zero or equal this?
			p_path = p_thisunit->port_path;
			for (i = 0; i < port_path_count; i++) {
				if (p_path->oport == nport) {
					if ((p_path->p_stream != NULL) && (p_path->p_stream != p_stream)) {
#if (DUMP_SHARE == ENABLE)
						DBG_DUMP(" still-share-stream(\"%s\") to \"%s\".path.out[%d] ===> %d: in[%d]out[%d] ...match!\r\n",
								 p_stream->stream_name, p_thisunit->unit_name, nport - ISF_OUT_BASE, i, p_path->iport - ISF_IN_BASE, p_path->oport - ISF_OUT_BASE);
#endif
						return ISF_ERR_FAILED;
					}
				}
				p_path++;
			}
			//DBG_MSG(" cmp-stream(\"%s\") to \"%s\".path.out[%d] ===> x\r\n",
			//    p_stream->stream_name, p_thisunit->unit_name, nport-ISF_OUT_BASE);
			return ISF_OK;
		} else if ((nport >= ISF_IN_BASE) && (nport - ISF_IN_BASE < p_thisunit->port_in_count)) {
			return ISF_OK;
		}
	}
	return ISF_ERR_FAILED;
}
#endif

// match unit name
ISF_RV isf_unit_match_name(ISF_UNIT *p_unit, char *name)
{
	if (!p_unit) {
		return ISF_ERR_NAME_ISNOTMATCH;
	}
	if (strcmp(p_unit->unit_name, name) == 0) {
		return ISF_OK;
	}
	return ISF_ERR_NAME_ISNOTMATCH;
}

// match unit name
ISF_RV isf_unit_match_outputname(ISF_UNIT *p_unit, char *name)
{
	UINT32 i;
	if (!p_unit) {
		return ISF_ERR_NAME_ISNOTMATCH;
	}
	if (strcmp(p_unit->unit_name, name) == 0) {
		return ISF_OK;
	}
	for (i = 0; i < p_unit->port_out_count; i++) {
		ISF_PORT *p_destport = p_unit->port_out[i];
		if (p_destport && isf_unit_match_outputname(p_destport->p_destunit, name) == ISF_OK) {
			return ISF_OK;
		}
	}
	return ISF_ERR_NAME_ISNOTMATCH;
}

// match unit name
ISF_RV isf_unit_match_inputname(ISF_UNIT *p_unit, char *name)
{
	UINT32 i;
	if (!p_unit) {
		return ISF_ERR_NAME_ISNOTMATCH;
	}
	if (strcmp(p_unit->unit_name, name) == 0) {
		return ISF_OK;
	}
	for (i = 0; i < p_unit->port_in_count; i++) {
		ISF_PORT *p_srcport = p_unit->port_in[i];
		if (p_srcport && isf_unit_match_inputname(p_srcport->p_srcunit, name) == ISF_OK) {
			return ISF_OK;
		}
	}
	return ISF_ERR_NAME_ISNOTMATCH;
}

typedef ISF_RV(*isf_unit_bind_cb)
(struct _ISF_UNIT *p_srcunit, UINT32 srcPort, struct _ISF_UNIT *p_destunit, UINT32 destPort);

static ISF_RV _isf_port_veirfy(char *unit, char *act, UINT32 oport, ISF_PORT *p_port)
{
	ISF_RV r = ISF_OK;
	if (p_port->sign != ISF_SIGN_PORT) {
		r = ISF_ERR_INVALID_SIGN;
		goto P1_END;
	}
	if ((p_port->iport < ISF_IN(0)) || (p_port->iport >= ISF_IN_MAX)) {
		r = ISF_ERR_INVALID_PORT_ID;
		goto P1_END;
	}
	if (p_port->connecttype == ISF_CONNECT_NONE) {
		r = ISF_ERR_INVALID_VALUE;
		goto P1_END;
	}
	return ISF_OK;
P1_END:
	if (p_port->p_destunit) {
		DBG_ERR(" \"%s\".out[%d] %s to \"%s\".in[%d], port verify error %d\r\n",
				unit, oport - ISF_OUT_BASE, act, p_port->p_destunit->unit_name, p_port->iport, r);
	} else {
		DBG_ERR(" \"%s\".out[%d] %s to port(%08x), port verify error %d\r\n",
				unit, oport - ISF_OUT_BASE, act, (UINT32)p_port, r);
	}
	//ASSERT(r == ISF_OK);
	if (!(r == ISF_OK))
		debug_dump_current_stack();
	return r;
}

static ISF_RV _isf_unit_build_connectport(ISF_UNIT *p_srcunit, UINT32 oport)
{
	ISF_PORT *p_port;
	//ISF_STATE* p_state;
	ISF_UNIT *p_destunit;
	UINT32 r = ISF_OK;
	//DBG_IND("\r\n");
	p_port = p_srcunit->port_outcfg[oport - ISF_OUT_BASE];
	//p_state = p_srcunit->port_outstate[oport-ISF_OUT_BASE];
	if (p_port == NULL) {
		//this port is not going to connect, ignore
		return ISF_OK;
	}
	if (p_srcunit->port_out[oport - ISF_OUT_BASE] != NULL) {
		//this port is already connected
		return ISF_ERR_ALREADY_CONNECT;
	}
	p_destunit = p_port->p_destunit;
	if (p_destunit == NULL) {
		//why?
		return ISF_ERR_NULL_OBJECT;
	}
	r = _isf_port_veirfy(p_srcunit->unit_name, "connect", oport, p_port);
	_isf_unit_check_return(r);
	if (r != ISF_OK) {
		return r;
	}

	DBG_IND("-- \"%s\".out[%d] ===> \"%s\".in[%d] : ",
			p_srcunit->unit_name, oport - ISF_OUT_BASE, p_destunit->unit_name, (p_port->iport) - ISF_IN_BASE);
	//1. match port connect-type with port-caps of SrcUnit & DestUnit
	//2. copy port-caps
	//     (PUSH: should fill GetImgBufFromDest API from DestUnit port-caps to this port)
	//     (PULL: should fill GetImgBufFromSrc API from SrcUnit port-caps to this port)
	{
		ISF_PORT_CAPS *p_srccaps = p_srcunit->port_outcaps[oport - ISF_OUT_BASE];
		ISF_PORT_CAPS *p_destcaps = p_destunit->port_incaps[(p_port->iport) - ISF_IN_BASE];
		//is src port caps support all flag?
		BOOL bSrcMatch = ((p_port->connecttype & p_srccaps->connecttype_caps) == p_port->connecttype);
		//is dest port caps support all flag?
		BOOL bDestMatch = ((p_port->connecttype & p_destcaps->connecttype_caps) == p_port->connecttype);
		if (!bSrcMatch || !bDestMatch) {
			//match connect type is failed!
			DBG_MSG("....... failed! (match type)\r\n");
			DBG_ERR("bind \"%s\".out[%d] ===> \"%s\".in[%d] : failed! (match type)\r\n",
					p_srcunit->unit_name, oport - ISF_OUT_BASE, p_destunit->unit_name, (p_port->iport) - ISF_IN_BASE);
			return ISF_ERR_CONNECT_NOTMATCH;
		}
		//prepare data transfer function
		if (p_port->connecttype == ISF_CONNECT_PUSH) {
			p_port->do_push = p_destcaps->do_push;
			p_port->do_notify = NULL;
			p_port->do_pull = p_srccaps->do_pull; //allow pull even connect with push
		}
		if (p_port->connecttype == (ISF_CONNECT_PUSH | ISF_CONNECT_SYNC)) {
			p_port->do_push = p_destcaps->do_push;
			p_port->do_notify = NULL;
			p_port->do_pull = p_srccaps->do_pull; //allow pull even connect with push
		}
		if (p_port->connecttype == ISF_CONNECT_PULL) {
			p_port->do_push = NULL;
			p_port->do_notify = NULL;
			p_port->do_pull = p_srccaps->do_pull;
		}
		if (p_port->connecttype == (ISF_CONNECT_PULL | ISF_CONNECT_NOTIFY)) {
			p_port->do_push = NULL;
			p_port->do_notify = p_srccaps->do_notify;
			p_port->do_pull = p_srccaps->do_pull;
		}
		if (p_port->connecttype == (ISF_CONNECT_PUSH | ISF_CONNECT_NOTIFY)) {
			p_port->do_push = p_destcaps->do_push;
			p_port->do_notify = p_srccaps->do_notify;
			p_port->do_pull = p_srccaps->do_pull; //allow pull even connect with push
		}
		if (p_port->connecttype == (ISF_CONNECT_PUSH | ISF_CONNECT_PULL)) {
			p_port->do_push = p_destcaps->do_push;
			p_port->do_notify = NULL;
			p_port->do_pull = p_srccaps->do_pull;
		}
		if (p_port->connecttype == (ISF_CONNECT_PUSH | ISF_CONNECT_PULL | ISF_CONNECT_NOTIFY)) {
			p_port->do_push = p_destcaps->do_push;
			p_port->do_notify = p_srccaps->do_notify;
			p_port->do_pull = p_srccaps->do_pull;
		}
	}
	//3. call DestUnit->do_bindinput(DestUnit, input-port, SrcUnit, out-port)
	//     (DestUnit could verify connect limit)
	//     (DestUnit should fill ImgInfo to port)
	{
		isf_unit_bind_cb do_bindinput = NULL;
		do_bindinput = p_destunit->do_bindinput;
		if (!do_bindinput) {
			do_bindinput = isf_unit_default_bind_input;
		}
		r = do_bindinput(p_destunit, p_port->iport, p_srcunit, oport);
		if (r != ISF_OK) {
			//bind to dest unit is failed!
			DBG_MSG("....... failed! dest (bind input) error=%d\r\n", r);
			DBG_ERR("bind \"%s\".out[%d] ===> \"%s\".in[%d] : failed! dest (bind input)\r\n",
					p_srcunit->unit_name, oport - ISF_OUT_BASE, p_destunit->unit_name, (p_port->iport) - ISF_IN_BASE);
			return ISF_ERR_CONNECT_BINDDEST;
		}
		//now, image info should be already filled by dest-unit
	}
	//4. call SrcUnit->do_bindoutput(SrcUnit, out-port, DestUnit, input-port)
	//
	//     (SrcUnit could verify connect limit)
	//     (SrcUnit should verify ImgInfo from port)
	{
		isf_unit_bind_cb do_bindoutput = NULL;
		do_bindoutput = p_srcunit->do_bindoutput;
		if (!do_bindoutput) {
			do_bindoutput = isf_unit_default_bind_output;
		}
		r = do_bindoutput(p_srcunit, oport, p_destunit, p_port->iport);
		if (r != ISF_OK) {
			//bind to src unit is failed!
			DBG_MSG("....... failed! src (bind output) error=%d\r\n", r);
			DBG_ERR("bind \"%s\".out[%d] ===> \"%s\".in[%d] : failed! src (bind output)\r\n",
					p_srcunit->unit_name, oport - ISF_OUT_BASE, p_destunit->unit_name, (p_port->iport) - ISF_IN_BASE);
			return ISF_ERR_CONNECT_BINDSRC;
		}
	}
	//5. connect build
	p_srcunit->port_out[oport - ISF_OUT_BASE] = p_port;
	//p_port->attr &= ~ISF_PORT_ATTR_KEEPSRCBUF; //clean attr
	p_port->p_srcunit = p_srcunit;
	p_port->oport = oport;
	DBG_MSG("ok (type = 0x%02x)\r\n", p_port->connecttype);

	return ISF_OK;
}

static ISF_RV _isf_unit_break_connectport(ISF_UNIT *p_srcunit, UINT32 oport)
{
	ISF_PORT *p_port;
	ISF_STATE *p_state;
	ISF_UNIT *p_destunit;
	ISF_RV r;
	//DBG_IND("\r\n");
	if (p_srcunit->port_outcfg[oport - ISF_OUT_BASE] != NULL) {
		//this port is not going to disconnect, ignore
		return ISF_OK;
	}
	p_port = p_srcunit->port_out[oport - ISF_OUT_BASE];
	p_state = p_srcunit->port_outstate[oport - ISF_OUT_BASE];
	if (p_port == NULL) {
		//this port is not connected yet
		return ISF_ERR_NOT_CONNECT_YET;
	}
	p_destunit = p_port->p_destunit;
	if (p_destunit == NULL) {
		//why?
		return ISF_ERR_NULL_OBJECT;
	}
	r = _isf_port_veirfy(p_srcunit->unit_name, "disconnect", oport, p_port);
	_isf_unit_check_return(r);
	if (r != ISF_OK) {
		return r;
	}

	DBG_MSG("-- \"%s\".out[%d] =X=> \"%s\".in[%d] : ",
			p_srcunit->unit_name, oport - ISF_OUT_BASE, p_destunit->unit_name, (p_port->iport) - ISF_IN_BASE);
	//1. connect break
	//p_destunit->list_id = 0;

	if (p_state->dirty & ISF_PORT_CMD_RESET) { //isf under reset?
		goto force_break; //always pass
	}
		
	//4. call SrcUnit->do_bindoutput(SrcUnit, out-port, DestUnit, input-port)
	//
	//     (SrcUnit could verify connect limit)
	//     (SrcUnit should verify ImgInfo from port)
	{
		isf_unit_bind_cb do_bindoutput = NULL;
		do_bindoutput = p_srcunit->do_bindoutput;
		if (!do_bindoutput) {
			do_bindoutput = isf_unit_default_bind_output;
		}
		r = do_bindoutput(p_srcunit, oport | 0x80000000, p_destunit, p_port->iport | 0x80000000);
		if (r != ISF_OK) {
			//bind to src unit is failed!
			DBG_MSG("....... failed! src (unbind output) error=%d\r\n", r);
			DBG_ERR("bind \"%s\".out[%d] =x=> \"%s\".in[%d] : failed! src (unbind output)\r\n",
					p_srcunit->unit_name, oport - ISF_OUT_BASE, p_destunit->unit_name, (p_port->iport) - ISF_IN_BASE);
			return ISF_ERR_CONNECT_BINDSRC;
		}
	}

	//3. call DestUnit->do_bindinput(DestUnit, input-port, SrcUnit, out-port)
	//     (DestUnit could verify connect limit)
	//     (DestUnit should fill ImgInfo to port)
	{
		isf_unit_bind_cb do_bindinput = NULL;
		do_bindinput = p_destunit->do_bindinput;
		if (!do_bindinput) {
			do_bindinput = isf_unit_default_bind_input;
		}
		r = do_bindinput(p_destunit, p_port->iport | 0x80000000, p_srcunit, oport | 0x80000000);
		if (r != ISF_OK) {
			//bind to dest unit is failed!
			DBG_MSG("....... failed! dest (unbind input) error=%d\r\n", r);
			DBG_ERR("bind \"%s\".out[%d] =x=> \"%s\".in[%d] : failed! dest (unbind input)\r\n",
					p_srcunit->unit_name, oport - ISF_OUT_BASE, p_destunit->unit_name, (p_port->iport) - ISF_IN_BASE);
			return ISF_ERR_CONNECT_BINDDEST;
		}
		//now, image info should be already filled by dest-unit
	}

force_break:
	//break src->dest
	p_srcunit->port_out[oport - ISF_OUT_BASE] = NULL;

	//break dest->src
	p_port->p_srcunit = NULL;
	
	p_port->oport = 0;
	
	//2. clear data transfer function
	{
		p_port->do_push = NULL;
		p_port->do_pull = NULL;
	}
	//3. clear image info
	{
		_isf_unit_clear_imginfo(p_port);
	}
	//p_port->attr &= ~ISF_PORT_ATTR_KEEPSRCBUF; //clean attr
	//p_state->state = ISF_PORT_STATE_OFF; //off
	//p_state->dirty = 0; //clean
	DBG_MSG("ok\r\n");
	return ISF_OK;
}

/*
static BOOL _isf_unit_is_bind(ISF_UNIT *p_unit, UINT32 oport)
{
	return (_isf_unit_out(p_unit, oport) != NULL) ? TRUE : FALSE;
}
*/

ISF_RV _isf_unit_set_bind(UINT32 h_isf, ISF_UNIT *p_unit, UINT32 oport, ISF_UNIT *p_destunit, UINT32 iport)
{
	ISF_RV rv = ISF_OK;
	
	//ISF_PORT* p_port = _isf_unit_out(p_unit, oport);
	ISF_STATE *p_state = NULL;
	if (!p_unit)
		return ISF_ERR_NULL_OBJECT;

	if (!((oport < (int)ISF_IN_BASE) && (oport < p_unit->port_out_count))) {
		DBG_ERR(" \"%s\".out[%d] is out of id\r\n",
				p_unit->unit_name, oport - ISF_OUT_BASE);
		return ISF_ERR_INVALID_PORT_ID;
	}

	p_state = p_unit->port_outstate[oport - ISF_OUT_BASE];
	
	if((p_destunit != NULL) && (iport != ISF_PORT_NULL)) {
	
		ISF_PORT *p_dest = NULL;
		ISF_VDO_INFO *p_src_vdoinfo = (ISF_VDO_INFO *)p_unit->port_outinfo[oport - ISF_OUT_BASE];

		p_unit->p_base->do_debug(p_unit, oport, ISF_OP_BIND, "\r\nbind begin, (\"%s\".in[%d])", p_destunit->unit_name, iport - ISF_IN_BASE);
		if ((isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) && (p_unit->list_id == 0)) {
			if((h_isf != 0xff) && (p_src_vdoinfo->h_proc != 0) && (p_src_vdoinfo->h_proc != (h_isf + 0x8000))) { //already used by other process
				DBG_ERR(" \"%s\".out[%d] cannot bind! (already use by proc %d)\r\n", p_unit->unit_name, oport - ISF_OUT_BASE, p_src_vdoinfo->h_proc - 0x8000);
				rv = ISF_ERR_NOT_AVAIL;
				return rv;
			}
		}
		if (!((iport >= (int)ISF_IN_BASE) && (iport < ISF_IN_BASE + p_destunit->port_in_count))) {
			DBG_ERR(" \"%s\".in[%d] is out of id\r\n",
					p_destunit->unit_name, iport - ISF_IN_BASE);
			return ISF_ERR_INVALID_PORT_ID;
		}
		p_dest = _isf_unit_in(p_destunit, iport);
		if(p_dest == NULL) {
			DBG_ERR(" \"%s\".in[%d] is out of id\r\n",
					p_destunit->unit_name, iport - ISF_IN_BASE);
			return ISF_ERR_INVALID_PORT_ID;
		}
		
		if(p_dest->p_srcunit != NULL) {
			DBG_ERR(" \"%s\".in[%d] already bind one src, cannot bind again!\r\n",
					p_destunit->unit_name, iport - ISF_IN_BASE);
			return ISF_ERR_ALREADY_CONNECT;
		}
		/*
		if (p_dest != (ISF_PORT*)ISF_PORT_EOS) {
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_BIND, "bind begin, (\"%s\".in[%d])", p_dest->p_destunit->unit_name, p_dest->iport - ISF_IN_BASE);
		} else {
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_BIND, "bind begin, (ISF_PORT_EOS)");
		}*/

		if (p_unit->port_out[oport - ISF_OUT_BASE] == NULL) { //original is unbind, want to bind
		
			//if leaf port?
			if (_OPORT_IS_LEAF(p_unit, oport)) {
				DBG_ERR(" \"%s\".out[%d] is a leaf port, cannot bind !\r\n",
						p_unit->unit_name, oport - ISF_OUT_BASE);
				return ISF_ERR_NOT_SUPPORT;
				/*
				if (p_dest != (ISF_PORT*)ISF_PORT_EOS) {
					DBG_ERR(" \"%s\".out[%d] is a leaf port, cannot bind to this port!\r\n",
							p_unit->unit_name, oport - ISF_OUT_BASE);
					return ISF_ERR_NOT_SUPPORT;
				}
				p_dest = 0; //overwrite as 0
				*/
			}

			#if 0
			if(p_state->state == ISF_PORT_STATE_RUN) { //already start
		
				DBG_ERR(" \"%s\".out[%d] already start, cannot bind!\r\n",
						p_destunit->unit_name, oport - ISF_OUT_BASE);
				return ISF_ERR_ALREADY_START;
			}
			#endif

			p_state->dirty |= ISF_PORT_CMD_OUTPUT;
			p_state->dirty |= (ISF_PORT_CMD_CONNECT);
			p_unit->port_outcfg[oport - ISF_OUT_BASE] = p_dest;
/*
			//do bind-stream & connect
			if (_OPORT_IS_LEAF(p_unit, oport)) {
				p_state->dirty |= ISF_PORT_CMD_S1;
			} else {
				p_state->dirty |= ISF_PORT_CMD_OUTPUT;
				p_state->dirty |= (ISF_PORT_CMD_CONNECT | ISF_PORT_CMD_S1);
			}

			//do bind
			if (_OPORT_IS_LEAF(p_unit, oport)) {
				
			} else {
				p_unit->port_outcfg[oport - ISF_OUT_BASE] = p_dest;
			}
*/			
		} else { //original is bind, want to bind again
		
			DBG_ERR(" \"%s\".out[%d] already bind one dest, cannot bind again!\r\n",
					p_unit->unit_name, oport - ISF_OUT_BASE);
			return ISF_ERR_ALREADY_CONNECT;
#if 0
			if (p_dest != p_unit->port_out[oport - ISF_OUT_BASE]) {
				DBG_ERR(" \"%s\".out[%d] already bind one, cannot bind another to this port!\r\n",
						p_unit->unit_name, oport - ISF_OUT_BASE);
				return;
			}

			//compare stream
			if (_isf_unit_path_do(p_unit, ISF_PATH_ISEQUAL, oport, p_stream) != ISF_OK) {
				//do bind-stream
				p_state->dirty |= (ISF_PORT_CMD_S1);
			}
			//compare stream
			if (p_dest && _isf_unit_path_do(p_dest->p_destunit, ISF_PATH_ISEQUAL, p_dest->iport, p_stream) != ISF_OK) {
				//do bind-stream
				p_state->dirty |= (ISF_PORT_CMD_S1);
			}
#endif
		}
		
#if 0
		//do S1
		if (p_state->dirty & ISF_PORT_CMD_S1) {
			p_state->dirty &= ~ISF_PORT_CMD_S1;
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_BIND, "cmd S1");
			if (_OPORT_IS_LEAF(p_unit, oport)) {
				//leaf port
				r = _isf_unit_path_do(p_unit, ISF_PATH_SET, oport, p_stream); //set this output
				if (r != ISF_OK) {
					DBG_ERR(" \"%s\".out[%d] is used by other stream, cannot s1!\r\n",
							p_unit->unit_name, oport - ISF_OUT_BASE);
					return r;
				}
			} else {
				//normal port
				ISF_PORT *p_newdest = p_unit->port_outcfg[oport - ISF_OUT_BASE];
#if 0
				DBG_DUMP(" \"%s\".out[%d] do bind\r\n",
						 p_unit->unit_name, oport - ISF_OUT_BASE);
#endif
				r = _isf_unit_path_do(p_unit, ISF_PATH_SET, oport, p_stream); //set this output
				if (r != ISF_OK) {
					DBG_ERR(" \"%s\".out[%d] is used by other stream, cannot s1!\r\n",
							p_unit->unit_name, oport - ISF_OUT_BASE);
					return r;
				}
				r = _isf_unit_path_do(p_newdest->p_destunit, ISF_PATH_SET, p_newdest->iport, p_stream); //set next input
				if (r != ISF_OK) {
					DBG_ERR(" \"%s\".in[%d] is used by other stream, cannot s1!\r\n",
							p_newdest->p_destunit->unit_name, p_newdest->iport - ISF_IN_BASE);
					return r;
				}
			}
		}
#endif

		//do bind
		if (p_state->dirty & ISF_PORT_CMD_CONNECT
			&& (p_unit->port_out[oport - ISF_OUT_BASE] == NULL)
			&& (p_unit->port_outcfg[oport - ISF_OUT_BASE] != NULL)) {
			
			p_state->dirty &= ~ISF_PORT_CMD_CONNECT;
			
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd CONNECT");
			rv = _isf_unit_build_connectport(p_unit, oport);
			if (rv != ISF_OK) {
				DBG_MSG("bind failed! (%d)\r\n", rv);
			}
			//_isf_unit_set_inflag(p_unit, oport, 0, ISF_PORT_FLAG_PAUSE_PUSH, p_stream);
			//_isf_unit_set_outflag(p_unit, oport, 0, ISF_PORT_FLAG_PAUSE_PULL, p_stream);
			if ((isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) && (p_unit->list_id == 0)) {
				if (rv == ISF_OK) {
					p_src_vdoinfo->h_proc = h_isf + 0x8000; //set
					//DBG_DUMP(" \"%s\".out[%d] bind by proc %d!\r\n", p_unit->unit_name, oport - ISF_OUT_BASE, h_isf);
				}
			}
		}

		{
			ISF_VDO_INFO *p_src_vdoinfo = (ISF_VDO_INFO *)p_unit->port_outinfo[oport - ISF_OUT_BASE];
			ISF_VDO_INFO *p_dest_vdoinfo = (ISF_VDO_INFO *)(p_unit->p_base->get_bind_info(p_unit, oport));

			if(p_src_vdoinfo && p_dest_vdoinfo) {
				if(p_src_vdoinfo->sync & (ISF_INFO_DIRTY_IMGSIZE|ISF_INFO_DIRTY_IMGFMT)) {
					p_src_vdoinfo->dirty |= (p_dest_vdoinfo->dirty & p_src_vdoinfo->sync);
					DBG_MSG(" auto-sync dirty=%08x: src \"%s\".out[%d] <= dest \"%s\".in[%d]!\r\n",
						(p_dest_vdoinfo->dirty & p_src_vdoinfo->sync), p_unit->unit_name, oport - ISF_OUT_BASE, p_destunit->unit_name, iport - ISF_IN_BASE);
				}
				if(p_dest_vdoinfo->sync & (ISF_INFO_DIRTY_IMGSIZE|ISF_INFO_DIRTY_IMGFMT)) {
					p_dest_vdoinfo->dirty |= (p_src_vdoinfo->dirty & p_dest_vdoinfo->sync);
					DBG_MSG(" auto-sync dirty=%08x: src \"%s\".out[%d] => dest \"%s\".in[%d]!\r\n",
						(p_src_vdoinfo->dirty & p_dest_vdoinfo->sync), p_unit->unit_name, oport - ISF_OUT_BASE, p_destunit->unit_name, iport - ISF_IN_BASE);
				}
			}
		}

		if(p_state->state == ISF_PORT_STATE_READY) { //already open
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd RDYSYNC");
			if (p_unit->do_update) {
				if (oport != ISF_CTRL) {
					//_isf_unit_set_inflag(p_unit, oport, 0, ISF_PORT_FLAG_PAUSE_PUSH, p_stream);
					//_isf_unit_set_outflag(p_unit, oport, 0, ISF_PORT_FLAG_PAUSE_PULL, p_stream);
				}
				PERFORMANCE_BEGIN();
				rv = p_unit->do_update(p_unit, oport, ISF_PORT_CMD_READYTIME_SYNC);
				PERFORMANCE_END();
			}
		} else if(p_state->state == ISF_PORT_STATE_RUN) { //already start
		
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd RUNSYNC");
			if (p_unit->do_update) {
				if (oport != ISF_CTRL) {
					//_isf_unit_set_inflag(p_unit, oport, 0, ISF_PORT_FLAG_PAUSE_PUSH, p_stream);
					//_isf_unit_set_outflag(p_unit, oport, 0, ISF_PORT_FLAG_PAUSE_PULL, p_stream);
				}
				PERFORMANCE_BEGIN();
				rv = p_unit->do_update(p_unit, oport, ISF_PORT_CMD_RUNTIME_SYNC);
				PERFORMANCE_END();
				if (rv != ISF_OK) {
					goto set_bind_do_bind_end;
				}
			}
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd RUNUPDATE");
			if (p_unit->do_update) {
				PERFORMANCE_BEGIN();
				rv = p_unit->do_update(p_unit, oport, ISF_PORT_CMD_RUNTIME_UPDATE);
				PERFORMANCE_END();
				if ((oport != ISF_CTRL) && (rv == ISF_OK)) {
					//_isf_unit_set_inflag(p_unit, oport, ISF_PORT_FLAG_PAUSE_PUSH, 0, p_stream);
					//_isf_unit_set_outflag(p_unit, oport, ISF_PORT_FLAG_PAUSE_PULL, 0, p_stream);
				}
			}
		}
		
set_bind_do_bind_end:
		p_unit->p_base->do_debug(p_unit, oport, ISF_OP_BIND, "bind end");
		return rv;

	} else {

		ISF_PORT *p_dest = NULL;
		ISF_VDO_INFO *p_src_vdoinfo = (ISF_VDO_INFO *)p_unit->port_outinfo[oport - ISF_OUT_BASE];

		if(p_unit != NULL) {
			p_dest = p_unit->port_out[oport - ISF_OUT_BASE];
		}

		if(p_dest) {
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_BIND, "\r\nunbind begin, (\"%s\".in[%d])", p_dest->p_destunit->unit_name, p_dest->iport - ISF_IN_BASE);
		} else {
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_BIND, "\r\nunbind begin, (NULL)");
		}
		
		if ((isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) && (p_unit->list_id == 0)) {
			if((h_isf != 0xff) && (p_src_vdoinfo->h_proc != 0) && (p_src_vdoinfo->h_proc != (h_isf + 0x8000))) { //already used by other process
				DBG_ERR(" \"%s\".out[%d] cannot unbind! (already use by proc %d)\r\n", p_unit->unit_name, oport - ISF_OUT_BASE, p_src_vdoinfo->h_proc - 0x8000);
				rv = ISF_ERR_NOT_AVAIL;
				return rv;
			}
		}

		if (p_dest != NULL) { //original is bind, want to unbind

			#if 0
			if(p_state->state == ISF_PORT_STATE_RUN) { //already start
		
				DBG_ERR(" \"%s\".out[%d] already start, cannot unbind!\r\n",
						p_destunit->unit_name, oport - ISF_OUT_BASE);
				return ISF_ERR_ALREADY_START;
			}
			#endif

			/*
			//do unbind
			p_state->dirty |= ISF_PORT_CMD_OUTPUT;
			p_state->dirty |= (ISF_PORT_CMD_DISCONNECT | ISF_PORT_CMD_S0);
			p_unit->port_outcfg[oport - ISF_OUT_BASE] = NULL;
			*/
			p_state->dirty |= ISF_PORT_CMD_OUTPUT;
			p_state->dirty |= (ISF_PORT_CMD_DISCONNECT);
			p_unit->port_outcfg[oport - ISF_OUT_BASE] = NULL;
			
		} else { //original is unbind, want to unbind again

			DBG_ERR(" \"%s\".out[%d] is not bind yet, cannot unbind!\r\n",
					p_unit->unit_name, oport - ISF_OUT_BASE);
			return ISF_ERR_NOT_CONNECT_YET;

			/*
			//is a leaf port?
			if (!_OPORT_IS_LEAF(p_unit, oport))
				return; //ignore (do nothing)

			//do unbind
			p_state->dirty |= (ISF_PORT_CMD_DISCONNECT | ISF_PORT_CMD_S0);
			*/
		}
		

#if 0
		//do S0
		if (p_state->dirty & ISF_PORT_CMD_S0) {
			p_state->dirty &= ~ISF_PORT_CMD_S0;
			
			if (_OPORT_IS_LEAF(p_unit, oport)) {
				//if (cmd == ISF_PORT_CMD_S0)
				{
					p_unit->p_base->do_debug(p_unit, oport, ISF_OP_BIND, "cmd S0");

					//leaf port
					r = _isf_unit_path_do(p_unit, ISF_PATH_CLEAN, oport, p_stream); //clean this output
					if (r != ISF_OK) {
						DBG_ERR("\"%s\".out[%d] is not used by this stream, cannot s0!\r\n",
								p_unit->unit_name, oport - ISF_OUT_BASE);
						return r;
					}
				}
				return r;
			} else if (1
					   && (p_unit->port_out[oport - ISF_OUT_BASE] != NULL)
					   && (p_unit->port_outcfg[oport - ISF_OUT_BASE] == NULL)) {
				//if (cmd == ISF_PORT_CMD_S0)
				{
					ISF_PORT *p_curdest;
					p_unit->p_base->do_debug(p_unit, oport, ISF_OP_BIND, "cmd S0");

					//normal port
					p_curdest = p_unit->port_out[oport - ISF_OUT_BASE];
					r = _isf_unit_path_do(p_unit, ISF_PATH_CLEAN, oport, p_stream); //clean this output
					if (r != ISF_OK) {
						DBG_ERR(" \"%s\".out[%d] is not used by this stream, cannot s0!\r\n",
								p_unit->unit_name, oport - ISF_OUT_BASE);
						return r;
					}
					r = _isf_unit_path_do(p_curdest->p_destunit, ISF_PATH_CLEAN, p_curdest->iport, p_stream); //clean next input
					if (r != ISF_OK) {
						DBG_ERR(" \"%s\".in[%d] is not used by this stream, cannot s0!\r\n",
								p_curdest->p_destunit->unit_name, p_curdest->iport - ISF_IN_BASE);
						return r;
					}
				}
			}
		}
#endif
		//do unbind
		if (p_state->dirty & ISF_PORT_CMD_DISCONNECT) {
			p_state->dirty &= ~ISF_PORT_CMD_DISCONNECT;
			
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd DISCONNECT");

			#if 0
			if (p_unit->port_in_count <= p_unit->port_out_count) //if "1-to-n" unit, need to check share of iport by other stream
			{
				r = _isf_unit_path_do(p_unit, ISF_PATH_ISCLEAN, oport, NULL); //if NO stream ==> allow
				if (r != ISF_OK) {
#if (DUMP_SHARE == ENABLE)
					DBG_WRN(" \"%s\".out[%d] is still used by other stream, cannot unbind! (%d)\r\n",
							p_unit->unit_name, oport - ISF_OUT_BASE, r);
#endif

					//rollback port_outcfg by Out
					p_unit->port_outcfg[oport - ISF_OUT_BASE] = p_unit->port_out[oport - ISF_OUT_BASE];
					return ISF_OK;
				}
			}
			else { // (p_unit->port_in_count > p_unit->port_out_count) //if "n-to-1" unit, need to check share of iport is only this stream
				r = _isf_unit_path_do(p_unit, ISF_PATH_ISCLEAN, oport, p_stream); //if NOT share of oport to other stream ==> allow
				if (r != ISF_OK) {
#if (DUMP_SHARE == ENABLE)
					DBG_WRN(" \"%s\".out[%d] is still used by other stream, cannot unbind! (%d)\r\n",
							p_unit->unit_name, oport - ISF_OUT_BASE, r);
#endif

					//rollback port_outcfg by Out
					p_unit->port_outcfg[oport - ISF_OUT_BASE] = p_unit->port_out[oport - ISF_OUT_BASE];
					/*
					EX: Demux, BsMux like unit
					*/
					return ISF_OK;
				}
			}
			#endif
			rv = _isf_unit_break_connectport(p_unit, oport);
			if (rv != ISF_OK) {
				DBG_MSG("unbind failed! (%d)\r\n", rv);
			}
			if ((isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) && (p_unit->list_id == 0)) {
				if (rv == ISF_OK) {
					p_src_vdoinfo->h_proc = 0; //clear
					//DBG_DUMP(" \"%s\".out[%d] unbind by proc %d!\r\n", p_unit->unit_name, oport - ISF_OUT_BASE, h_isf);
				}
			}
		}
		p_unit->p_base->do_debug(p_unit, oport, ISF_OP_BIND, "unbind end");
		return rv;
	}
	return rv;
}

ISF_RV _isf_unit_clear_bind(ISF_UNIT *p_unit, UINT32 oport)
{
	ISF_PORT *p_dest = NULL;
	ISF_STATE *p_state = NULL;
	if (!p_unit)
		return ISF_ERR_NULL_OBJECT;

	if (!((oport < (int)ISF_IN_BASE) && (oport < p_unit->port_out_count))) {
		return ISF_ERR_INVALID_PORT_ID;
	}

	p_state = p_unit->port_outstate[oport - ISF_OUT_BASE];
	p_state->dirty |= ISF_PORT_CMD_RESET; //notify unit, now we are inside reset flow

	if(p_state->state == ISF_PORT_STATE_OFF) { //already close
		p_dest = p_unit->port_out[oport - ISF_OUT_BASE];
		if(p_dest) {
			DBG_DUMP(" \"%s\".out[%d] already bind => unbind\r\n", p_unit->unit_name, oport - ISF_OUT_BASE);
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_BIND, "clear bind begin, (\"%s\".out[%d])", p_unit->unit_name, oport - ISF_OUT_BASE);
			_isf_unit_set_bind(0xff, p_unit, oport, NULL, 0);
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_BIND, "clear bind end");
			return ISF_OK;
		}
	}
	return ISF_ERR_IGNORE;
}

ISF_RV _isf_unit_get_bind(UINT32 h_isf, ISF_UNIT *p_unit, UINT32 nport, ISF_UNIT **pp_destunit, UINT32* p_nport)
{
	ISF_PORT* p_port = NULL;
	if (!p_unit)
		return ISF_ERR_NULL_OBJECT;

	if (ISF_IS_OPORT(p_unit, nport)) {
	    	p_port = _isf_unit_out(p_unit, nport);
		if (p_port && !_OPORT_IS_LEAF(p_unit, nport)) {
	    		if (pp_destunit)
	        		pp_destunit[0] = p_port->p_destunit;
	        	if(p_nport)
	        		p_nport[0] = p_port->iport;
	        	return ISF_OK;
		} else {
	    		if (pp_destunit)
	        		pp_destunit[0] = NULL;
	        	if(p_nport)
	        		p_nport[0] = ISF_PORT_NULL;
	        	return ISF_ERR_NOT_CONNECT_YET;
		}
	} else if (ISF_IS_IPORT(p_unit, nport)) {
	    	p_port = _isf_unit_in(p_unit, nport);
		if (p_port && !_IPORT_IS_ROOT(p_unit, nport)) {
	    		if (pp_destunit)
	        		pp_destunit[0] = p_port->p_srcunit;
	        	if(p_nport)
	        		p_nport[0] = p_port->oport;
	        	return ISF_OK;
		} else {
	    		if (pp_destunit)
	        		pp_destunit[0] = NULL;
	        	if(p_nport)
	        		p_nport[0] = ISF_PORT_NULL;
	        	return ISF_ERR_NOT_CONNECT_YET;
		}
	}
	return ISF_ERR_INVALID_PORT_ID;
}

ISF_RV _isf_unit_set_state(UINT32 h_isf, ISF_UNIT *p_unit, UINT32 oport, ISF_PORT_CMD cmd)
{
	ISF_RV rv = ISF_ERR_IGNORE;
	ISF_STATE *p_state = NULL;
	if (!p_unit)
		return ISF_ERR_NULL_OBJECT;

	if (!((oport < (int)ISF_IN_BASE) && (oport < p_unit->port_out_count))) {
		DBG_ERR(" \"%s\".out[%d] is out of id\r\n",
				p_unit->unit_name, oport - ISF_OUT_BASE);
		return ISF_ERR_INVALID_PORT_ID;
	}

	p_state = p_unit->port_outstate[oport - ISF_OUT_BASE];
	switch (cmd) {
	case ISF_PORT_CMD_OPEN: //do OPEN
		p_unit->p_base->do_debug(p_unit, oport, ISF_OP_STATE, "\r\nopen begin, state=%d", p_state->state);
		if ((isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) && (p_unit->list_id == 0)) {
			if((h_isf != 0xff) && (p_state->h_proc != 0) && (p_state->h_proc != (h_isf + 0x8000))) { //already used by other process
				DBG_ERR(" \"%s\".out[%d] cannot open! (already use by proc %d)\r\n", p_unit->unit_name, oport - ISF_OUT_BASE, p_state->h_proc - 0x8000);
				rv = ISF_ERR_NOT_AVAIL;
				return rv;
			}
		}
		if((p_state->state == ISF_PORT_STATE_OFF) ||
            ((p_unit->list_id == 2)&&(p_state->state == ISF_PORT_STATE_READY))) { //already close
			p_state->statecfg = ISF_PORT_STATE_READY;
			p_state->dirty |= ISF_PORT_CMD_STATE;
			/*
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd OFFSYNC");
			if (p_unit->do_update) {
				PERFORMANCE_BEGIN();
				r = p_unit->do_update(p_unit, oport, ISF_PORT_CMD_OFFTIME_SYNC);
				PERFORMANCE_END();
				if ((oport != ISF_CTRL) && (r == ISF_OK)) {
					p_state->state = ISF_PORT_STATE_OFF;
				}
			}
			*/
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd OPEN");
			if (p_unit->do_update) {
				PERFORMANCE_BEGIN();
				rv = p_unit->do_update(p_unit, oport, ISF_PORT_CMD_OPEN);
				PERFORMANCE_END();
				if (((oport != ISF_CTRL) && (rv == ISF_OK))
				|| (p_state->dirty & ISF_PORT_CMD_RESET)) {
					p_state->state = ISF_PORT_STATE_READY;
				}
				if ((isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) && (p_unit->list_id == 0)) {
					if ((oport != ISF_CTRL) && (rv == ISF_OK)) {
						p_state->h_proc = h_isf + 0x8000; //set
						//DBG_DUMP(" \"%s\".out[%d] open by proc %d!\r\n", p_unit->unit_name, oport - ISF_OUT_BASE, h_isf);
					}
				}
			}
			p_state->dirty &= ~ISF_PORT_CMD_STATE;
		} else if(p_state->state == ISF_PORT_STATE_RUN) {
			DBG_WRN(" \"%s\".out[%d] cannot open! (state=RUN, already start)\r\n", p_unit->unit_name, oport - ISF_OUT_BASE);
			rv = ISF_ERR_ALREADY_START;
		} else {
			DBG_WRN(" \"%s\".out[%d] cannot open! (state=READY, already open)\r\n", p_unit->unit_name, oport - ISF_OUT_BASE);
			rv = ISF_ERR_ALREADY_OPEN;
		}
		p_unit->p_base->do_debug(p_unit, oport, ISF_OP_STATE, "open end, state=%d", p_state->state);
		break;
	case ISF_PORT_CMD_START: //do START
		p_unit->p_base->do_debug(p_unit, oport, ISF_OP_STATE, "\r\nstart begin, state=%d", p_state->state);
		if ((isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) && (p_unit->list_id == 0)) {
			if((h_isf != 0xff) && (p_state->h_proc != (h_isf + 0x8000))) { //already used by other process
				DBG_ERR(" \"%s\".out[%d] cannot start! (already use by proc %d)\r\n", p_unit->unit_name, oport - ISF_OUT_BASE, p_state->h_proc - 0x8000);
				rv = ISF_ERR_NOT_AVAIL;
				return rv;
			}
		}
		if(p_state->state == ISF_PORT_STATE_READY) { //already open
			p_state->statecfg = ISF_PORT_STATE_RUN;
			p_state->dirty |= ISF_PORT_CMD_STATE;
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd RDYSYNC");
			if (p_unit->do_update) {
				PERFORMANCE_BEGIN();
				rv = p_unit->do_update(p_unit, oport, ISF_PORT_CMD_READYTIME_SYNC);
				PERFORMANCE_END();
				if (rv != ISF_OK) {
					goto set_state_start1_end;
				}
			}
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd START");
			if (p_unit->do_update) {
				PERFORMANCE_BEGIN();
				rv = p_unit->do_update(p_unit, oport, ISF_PORT_CMD_START);
				PERFORMANCE_END();
				if (((oport != ISF_CTRL) && (rv == ISF_OK))
				|| (p_state->dirty & ISF_PORT_CMD_RESET)) {
					p_state->state = ISF_PORT_STATE_RUN;
					//_isf_unit_set_inflag(p_unit, oport, ISF_PORT_FLAG_PAUSE_PUSH, 0, p_stream);
					//_isf_unit_set_outflag(p_unit, oport, ISF_PORT_FLAG_PAUSE_PULL, 0, p_stream);
				}
			}
set_state_start1_end:
			p_state->dirty &= ~ISF_PORT_CMD_STATE;
		} else if(p_state->state == ISF_PORT_STATE_RUN) { //already start
			p_state->statecfg = ISF_PORT_STATE_RUN;
			p_state->dirty |= ISF_PORT_CMD_STATE;
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd RUNSYNC");
			if (p_unit->do_update) {
				if (oport != ISF_CTRL) {
					//_isf_unit_set_inflag(p_unit, oport, 0, ISF_PORT_FLAG_PAUSE_PUSH, p_stream);
					//_isf_unit_set_outflag(p_unit, oport, 0, ISF_PORT_FLAG_PAUSE_PULL, p_stream);
				}
				PERFORMANCE_BEGIN();
				rv = p_unit->do_update(p_unit, oport, ISF_PORT_CMD_RUNTIME_SYNC);
				PERFORMANCE_END();
				if (rv != ISF_OK) {
					goto set_state_start2_end;
				}
			}
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd RUNUPDATE");
			if (p_unit->do_update) {
				PERFORMANCE_BEGIN();
				rv = p_unit->do_update(p_unit, oport, ISF_PORT_CMD_RUNTIME_UPDATE);
				PERFORMANCE_END();
				if (((oport != ISF_CTRL) && (rv == ISF_OK))
				|| (p_state->dirty & ISF_PORT_CMD_RESET)) {
					//_isf_unit_set_inflag(p_unit, oport, ISF_PORT_FLAG_PAUSE_PUSH, 0, p_stream);
					//_isf_unit_set_outflag(p_unit, oport, ISF_PORT_FLAG_PAUSE_PULL, 0, p_stream);
				}
			}
set_state_start2_end:
			p_state->dirty &= ~ISF_PORT_CMD_STATE;
		} else {  //if(p_state->state == ISF_PORT_STATE_OFF)
			DBG_WRN(" \"%s\".out[%d] cannot start! (state=OFF, not open yet)\r\n", p_unit->unit_name, oport - ISF_OUT_BASE);
			rv = ISF_ERR_NOT_OPEN_YET;
		}
		p_unit->p_base->do_debug(p_unit, oport, ISF_OP_STATE, "start end, state=%d", p_state->state);
		break;
	case ISF_PORT_CMD_STOP: //do STOP
		p_unit->p_base->do_debug(p_unit, oport, ISF_OP_STATE, "\r\nstop begin, state=%d", p_state->state);
		if ((isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) && (p_unit->list_id == 0)) {
			if((h_isf != 0xff) && (p_state->h_proc != (h_isf + 0x8000))) { //already used by other process
				DBG_ERR(" \"%s\".out[%d] cannot stop! (already use by proc %d)\r\n", p_unit->unit_name, oport - ISF_OUT_BASE, p_state->h_proc - 0x8000);
				rv = ISF_ERR_NOT_AVAIL;
				return rv;
			}
		}
		if((p_state->state == ISF_PORT_STATE_RUN) ||
            ((p_unit->list_id == 2)&&(p_state->state == ISF_PORT_STATE_READY))){ //already start
			p_state->statecfg = ISF_PORT_STATE_READY;
			p_state->dirty |= ISF_PORT_CMD_STATE;
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd STOP");
			/*
			if (_isf_unit_count_flagpath(p_unit, oport, ISF_PORT_FLAG_START, p_stream) > 0) {
#if (DUMP_SHARE == ENABLE)
				DBG_WRN(" \"%s\".out[%d] is still used by other stream, cannot STOP! (%d)\r\n",
						p_unit->unit_name, oport - ISF_OUT_BASE, r);
#endif
				p_state->dirty &= ~cmd; //clean
				p_state->statecfg = p_state->state; //cancel new state
				return ISF_OK;
			}
			*/
			if (p_unit->do_update) {
				PERFORMANCE_BEGIN();
				rv = p_unit->do_update(p_unit, oport, ISF_PORT_CMD_STOP);
				PERFORMANCE_END();
				if (((oport != ISF_CTRL) && (rv == ISF_OK))
				|| (p_state->dirty & ISF_PORT_CMD_RESET)) {
					p_state->state = ISF_PORT_STATE_READY;
					//_isf_unit_set_inflag(p_unit, oport, 0, ISF_PORT_FLAG_PAUSE_PUSH, p_stream);
					//_isf_unit_set_outflag(p_unit, oport, 0, ISF_PORT_FLAG_PAUSE_PULL, p_stream);
				}
			}
			p_state->dirty &= ~ISF_PORT_CMD_STATE;
		} else if(p_state->state == ISF_PORT_STATE_OFF) {
			DBG_WRN(" \"%s\".out[%d] cannot stop! (state=OFF, not open yet)\r\n", p_unit->unit_name, oport - ISF_OUT_BASE);
			rv = ISF_ERR_NOT_OPEN_YET;
		} else {
			DBG_WRN(" \"%s\".out[%d] cannot stop! (state=READY, not start yet)\r\n", p_unit->unit_name, oport - ISF_OUT_BASE);
			rv = ISF_ERR_NOT_START;
		}
		p_unit->p_base->do_debug(p_unit, oport, ISF_OP_STATE, "stop end, state=%d", p_state->state);
		break;
	case ISF_PORT_CMD_CLOSE: //do CLOSE
		p_unit->p_base->do_debug(p_unit, oport, ISF_OP_STATE, "\r\nclose begin, state=%d", p_state->state);
		if ((isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) && (p_unit->list_id == 0)) {
			if((h_isf != 0xff) && (p_state->h_proc != 0) && (p_state->h_proc != (h_isf + 0x8000))) { //already used by other process
				DBG_ERR(" \"%s\".out[%d] cannot close! (already use by proc %d)\r\n", p_unit->unit_name, oport - ISF_OUT_BASE, p_state->h_proc - 0x8000);
				rv = ISF_ERR_NOT_AVAIL;
				return rv;
			}
		}
		if((p_state->state == ISF_PORT_STATE_READY)||
            ((p_unit->list_id == 2)&&(p_state->state == ISF_PORT_STATE_OFF))) { //already stop, or not start yet
			p_state->statecfg = ISF_PORT_STATE_OFF;
			p_state->dirty |= ISF_PORT_CMD_STATE;
			p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd CLOSE");
			/*
			if (_isf_unit_count_flagpath(p_unit, oport, ISF_PORT_FLAG_OPEN, p_stream) > 0) {
#if (DUMP_SHARE == ENABLE)
				DBG_WRN(" \"%s\".out[%d] is still used by other stream, cannot CLOSE! (%d)\r\n",
						p_unit->unit_name, oport - ISF_OUT_BASE, r);
#endif
				p_state->dirty &= ~cmd; //clean
				p_state->statecfg = p_state->state; //cancel new state
				return ISF_OK;
			}
			*/
			if (p_unit->do_update) {
				PERFORMANCE_BEGIN();
				rv = p_unit->do_update(p_unit, oport, ISF_PORT_CMD_CLOSE);
				PERFORMANCE_END();
				if (((oport != ISF_CTRL) && (rv == ISF_OK))
				|| (p_state->dirty & ISF_PORT_CMD_RESET)) {
					p_state->state = ISF_PORT_STATE_OFF;
				}
				if ((isf_get_common_cfg() & ISF_COMM_CFG_MULTIPROC) && (p_unit->list_id == 0)) {
					if ((oport != ISF_CTRL) && (rv == ISF_OK)) {
						p_state->h_proc = 0; //clear
						//DBG_DUMP(" \"%s\".out[%d] close by proc %d!\r\n", p_unit->unit_name, oport - ISF_OUT_BASE, h_isf);
					}
				}
			}
			p_state->dirty &= ~ISF_PORT_CMD_STATE;
		} else if(p_state->state == ISF_PORT_STATE_OFF) {
			DBG_WRN(" \"%s\".out[%d] cannot close! (state=OFF, not open yet)\r\n", p_unit->unit_name, oport - ISF_OUT_BASE);
			rv = ISF_ERR_NOT_OPEN_YET;
		} else {
			DBG_WRN(" \"%s\".out[%d] cannot close! (state=RUN, already start)\r\n", p_unit->unit_name, oport - ISF_OUT_BASE);
			rv = ISF_ERR_ALREADY_START;
		}
		p_unit->p_base->do_debug(p_unit, oport, ISF_OP_STATE, "close end, state=%d", p_state->state);
		break;
	default:
		DBG_ERR(" \"%s\".out[%d] invalid port cmd %d\r\n", p_unit->unit_name, oport - ISF_OUT_BASE, cmd);
		break;
	}
	return rv;
}

ISF_RV _isf_unit_get_state(UINT32 h_isf, ISF_UNIT *p_unit, UINT32 oport, ISF_PORT_STATE* p_state)
{
	if (!p_unit)
		return ISF_ERR_NULL_OBJECT;

	if (!((oport < (int)ISF_IN_BASE) && (oport < p_unit->port_out_count))) {
		DBG_ERR(" \"%s\".out[%d] is out of id\r\n",
				p_unit->unit_name, oport - ISF_OUT_BASE);
		return ISF_ERR_INVALID_PORT_ID;
	}

    	if (p_state)
    		p_state[0] = (p_unit->port_outstate[oport - ISF_OUT_BASE])->state;
	return ISF_OK;
}

ISF_RV _isf_unit_clear_state(ISF_UNIT *p_unit, UINT32 oport)
{
	ISF_STATE *p_state;
	if (!p_unit)
		return ISF_ERR_NULL_OBJECT;

	if (!((oport < (int)ISF_IN_BASE) && (oport < p_unit->port_out_count))) {
		return ISF_ERR_INVALID_PORT_ID;
	}

	p_state = p_unit->port_outstate[oport - ISF_OUT_BASE];
	p_state->dirty |= ISF_PORT_CMD_RESET; //notify unit, now we are inside reset flow
	
	if(p_state->state != ISF_PORT_STATE_OFF) {
		p_unit->p_base->do_debug(p_unit, oport, ISF_OP_STATE, "clear state begin, state=%d", p_state->state);
		if(p_state->state == ISF_PORT_STATE_RUN) { //stll start
			DBG_DUMP(" \"%s\".out[%d] already start => stop\r\n", p_unit->unit_name, oport - ISF_OUT_BASE);
			_isf_unit_set_state(0xff, p_unit, oport, ISF_PORT_CMD_STOP);
		}
		if(p_state->state == ISF_PORT_STATE_READY) { //still open
			DBG_DUMP(" \"%s\".out[%d] already open => close\r\n", p_unit->unit_name, oport - ISF_OUT_BASE);
			_isf_unit_set_state(0xff, p_unit, oport, ISF_PORT_CMD_CLOSE);
		}
		if(p_state->state == ISF_PORT_STATE_OFF) { //already close
			_isf_unit_clear_bind(p_unit, oport);
			//p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd RESET\r\n");
			if (p_unit->do_update) {
				PERFORMANCE_BEGIN();
				p_unit->do_update(p_unit, oport, ISF_PORT_CMD_RESET);
				PERFORMANCE_END();
			}
		}
		p_unit->p_base->do_debug(p_unit, oport, ISF_OP_STATE, "clear state end, state=%d", p_state->state);
		return ISF_OK;
	}
	return ISF_ERR_IGNORE;
}

ISF_RV _isf_unit_clear_context(ISF_UNIT *p_unit, UINT32 oport)
{
	ISF_STATE *p_state;
	if (!p_unit)
		return ISF_ERR_NULL_OBJECT;

	if (!((oport < (int)ISF_IN_BASE) && (oport < p_unit->port_out_count))) {
		return ISF_ERR_INVALID_PORT_ID;
	}

	p_state = p_unit->port_outstate[oport - ISF_OUT_BASE];
	
	if(p_state->state == ISF_PORT_STATE_OFF) { //already close
		//p_unit->p_base->do_debug(p_unit, oport, ISF_OP_STATE, "clear context begin, state=%d", p_state->state);
		//p_unit->p_base->do_debug(p_unit, oport, ISF_OP_COMMAND, "cmd RESET");
		if (p_unit->do_update) {
			PERFORMANCE_BEGIN();
			p_unit->do_update(p_unit, oport, ISF_PORT_CMD_RESET);
			PERFORMANCE_END();
		}
		//p_unit->p_base->do_debug(p_unit, oport, ISF_OP_STATE, "clear context end, state=%d", p_state->state);
		return ISF_OK;
	}
	return ISF_ERR_IGNORE;
}

#if 0
void _isf_unit_dump_pathinfo(ISF_UNIT* p_unit)
{
    ISF_PORT_PATH* p_path;
    UINT32 j;
    if (!p_unit)
        return;
    DBG_DUMP("---\"%s\"\r\n{\r\n", p_unit->unit_name);
    //    DBGH(p_unit->port_path_count);
    p_path = p_unit->port_path;
    for (j = 0; j < p_unit->port_path_count; j++) {
        ISF_PORT* p_srcport = p_unit->port_in[p_path->iport-ISF_IN_BASE];
        ISF_PORT* p_destport = p_unit->port_out[p_path->oport-ISF_OUT_BASE];
        DBG_DUMP(" - Path[%d]: (\"%s\"):",
            j,
            (p_path->p_stream != NULL)?(p_path->p_stream->stream_name):("-------")
            );

        if ((p_srcport != NULL) && (p_srcport->p_srcunit != NULL)) {
        DBG_DUMP("\"%s\".ISF_OUT%d => this.ISF_IN%d => (PROC) ",
            (p_srcport->p_srcunit != NULL)?(p_srcport->p_srcunit->unit_name):("-------"),
            p_srcport->oport-ISF_OUT_BASE+1,
            p_path->iport-ISF_IN_BASE+1
            );
        } else {
        DBG_DUMP("x => this.ISF_IN%d => (PROC) ",
            p_path->iport-ISF_IN_BASE+1
            );
		}
		
        if ((p_destport != NULL) && (p_destport->p_destunit != NULL))
        {
        DBG_DUMP("this.ISF_OUT%d => \"%s\".ISF_IN%d\r\n",
            p_path->oport-ISF_OUT_BASE+1,
            (p_destport->p_destunit != NULL)?(p_destport->p_destunit->unit_name):("-------"),
            p_destport->iport-ISF_IN_BASE+1
            );
        }
        else
        {
        DBG_DUMP("this.ISF_OUT%d => x\r\n",
            p_path->oport-ISF_OUT_BASE+1
            );
        }
        p_path ++;
    }
    DBG_DUMP("}\r\n");
}
#endif

/*
void _isf_unit_dump_pathinfo(ISF_UNIT *p_unit)
{
	if (!p_unit) {
		return;
	}
	UINT32 j;
	ISF_PORT_PATH *p_path = p_unit->port_path;
	for (j = 0; j < p_unit->port_path_count; j++) { //use unit's port path to find related dest port
		DBG_DUMP("---\"%s\".Path[%d] = {ISF_IN%d, ISF_OUT%d, \"%s\"}\r\n", 
			p_unit->unit_name, j, 
			p_path->iport - ISF_IN_BASE + 1, 
			p_path->oport - ISF_OUT_BASE + 1, 
			(p_path->p_stream)?(p_path->p_stream->unit_name):("x"));
	}
}
*/

#if 0
void _isf_unit_dump_connectinfo(ISF_STREAM *p_stream, ISF_UNIT *p_unit)
{
	UINT32 j;
	if (!p_unit) {
		return;
	}
	DBG_DUMP("isf_unit_begin(\"%s\", %08x);\r\n", p_unit->unit_name, p_unit->flagcfg);
	for (j = 0; j < p_unit->port_out_count; j++) {
		ISF_PORT *p_port = p_unit->port_outcfg[j];
		ISF_STATE *p_state = p_unit->port_outstate[j];
		ISF_UNIT *p_destunit;
		if (p_stream) {
			//compare stream
			if (_isf_unit_path_do(p_unit, ISF_PATH_ISEQUAL, j + ISF_OUT_BASE, p_stream) != ISF_OK) {
				continue;
			}
			if (p_port) {
				//compare stream
				if (_isf_unit_path_do(p_port->p_destunit, ISF_PATH_ISEQUAL, p_port->iport, p_stream) != ISF_OK) {
					continue;
				}
			}
		}
		if (p_port == NULL) {
			//if leaf port?
			if (_OPORT_IS_LEAF(p_unit, j+ISF_OUT_BASE)) {
				DBG_DUMP("    isf_unit_set_output(ISF_OUT%d, ISF_PORT_EOS, %08x);\r\n",
						 j + 1, p_state->state);
			} else {
				//this port is not connected
				DBG_DUMP("    isf_unit_set_output(ISF_OUT%d, NULL, %08x);\r\n",
						 j + 1, 0);
			}
		} else {
			//this port is connected
			p_destunit = p_port->p_destunit;
			DBG_DUMP("    isf_unit_set_output(ISF_OUT%d, isf_unit_in(\"%s\", ISF_IN%d), %08x);\r\n",
					 j + 1, (p_destunit == NULL) ? "(NULL)" : (p_destunit->unit_name), (p_port->iport) - ISF_IN_BASE + 1, p_state->state);
			_isf_unit_dump_imginfo(p_port);
		}
	}
	DBG_DUMP("isf_unit_end();\r\n");
}
#endif

void _isf_unit_dump_port(ISF_UNIT *p_unit, UINT32 nport)
{
	if (!p_unit) {
		return;
	}
	if ((nport < ISF_IN_BASE) && (nport < p_unit->port_out_count)) {
		UINT32 j = nport - ISF_OUT_BASE;
		ISF_PORT *p_port= p_unit->port_out[j];
		ISF_STATE *p_state = p_unit->port_outstate[j];
		ISF_UNIT *p_srcunit = p_unit;
		ISF_UNIT *p_destunit;
 		//if (p_dest) {
		//	DBG_DUMP("---\"%s\" ISF_OUT%d\r\n", p_unit->unit_name, nport - ISF_OUT_BASE + 1);
		//	_isf_unit_dump_imginfo(p_dest);
		//}
		DBG_DUMP("isf_unit_begin(\"%s\", %08x);\r\n", p_srcunit->unit_name, p_srcunit->flagcfg);
		if (p_port == NULL) {
			//if leaf port?
			if (_OPORT_IS_LEAF(p_srcunit, j+ISF_OUT_BASE)) {
				DBG_DUMP("    isf_unit_set_output(ISF_OUT%d, ISF_PORT_EOS, %08x);\r\n",
						 j + 1, p_state->state);
			} else {
				//this port is not connected
				DBG_DUMP("    isf_unit_set_output(ISF_OUT%d, NULL, %08x);\r\n",
						 j + 1, 0);
			}
		} else {
			//this port is connected
			p_destunit = p_port->p_destunit;
			DBG_DUMP("    isf_unit_set_output(ISF_OUT%d, isf_unit_in(\"%s\", ISF_IN%d), %08x);\r\n",
					 j + 1, (p_destunit == NULL) ? "(NULL)" : (p_destunit->unit_name), (p_port->iport) - ISF_IN_BASE + 1, p_state->state);
			_isf_unit_dump_imginfo(p_port);
		}
		DBG_DUMP("isf_unit_end();\r\n");
	} else if ((nport >= ISF_IN_BASE) && (nport - ISF_IN_BASE < p_unit->port_in_count)) {
		ISF_PORT *p_port = p_unit->port_in[nport - ISF_IN_BASE];
		if (p_port->p_srcunit == NULL) {
			UINT32 j = nport - ISF_IN_BASE;
			//if root port?
			if (_IPORT_IS_ROOT(p_unit, nport)) {
				//DBG_DUMP("    isf_unit_set_output(ISF_OUT%d, ISF_PORT_EOS, %08x);\r\n",
				//		 j + 1, p_state->state);
				DBG_DUMP("---\"%s\" ISF_IN%d (ROOT)\r\n", p_unit->unit_name, j + 1);
				_isf_unit_dump_imginfo(p_port);
			} else {
				//this port is not connected
				//DBG_DUMP("    isf_unit_set_output(ISF_OUT%d, NULL, %08x);\r\n",
				//		 j + 1, 0);
				DBG_DUMP("---\"%s\" ISF_IN%d (x)\r\n", p_unit->unit_name, j + 1);
				_isf_unit_dump_imginfo(p_port);
			}
		} else {
			//this port is connected
			ISF_UNIT *p_srcunit = p_port->p_srcunit;
			UINT32 j = p_port->oport - ISF_OUT_BASE;
			ISF_STATE *p_state = p_srcunit->port_outstate[j];
			DBG_DUMP("isf_unit_begin(\"%s\", %08x);\r\n", p_srcunit->unit_name, p_srcunit->flagcfg);
			DBG_DUMP("    isf_unit_set_output(ISF_OUT%d, isf_unit_in(\"%s\", ISF_IN%d), %08x);\r\n",
					 j + 1, (p_unit == NULL) ? "(NULL)" : (p_unit->unit_name), (p_port->iport) - ISF_IN_BASE + 1, p_state->state);
			_isf_unit_dump_imginfo(p_port);
			DBG_DUMP("isf_unit_end();\r\n");
		}
	}
}



