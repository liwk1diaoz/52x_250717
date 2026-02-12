#include "isf_flow_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_flow_c
#define __DBGLVL__          NVT_DBG_WRN // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_flow_c_debug_level = __DBGLVL__;
//module_param_named(isf_flow_c_debug_level, isf_flow_c_debug_level, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(isf_flow_c_debug_level, "flow_c debug level");
///////////////////////////////////////////////////////////////////////////////

#define DUMP_LIST       DISABLE
#define DUMP_DIRTY      DISABLE
#define DUMP_SELECT     DISABLE

#define DEBUG_FRC	DISABLE

/*
	SEM_WAIT(ISF_SEM_ID);
	SEM_WAIT(ISF_SEM_CFG_ID);
	SEM_SIGNAL(ISF_SEM_CFG_ID);
*/


/*
for "all connected units", do connect of input port (in reverse order)

[connect port] "SrcUnit -> port -> DestUnit"

1. match port connect-type with port-caps of SrcUnit & DestUnit
2. copy port-caps
     (PUSH: should fill GetImgBufFromDest API from DestUnit port-caps to this port)
     (PULL: should fill GetImgBufFromSrc API from SrcUnit port-caps to this port)

3. call DestUnit->do_bindinput(DestUnit, input-port, SrcUnit, out-port)
     (DestUnit could verify connect limit)
     (DestUnit should fill ImgInfo to port)

4. call SrcUnit->do_bindoutput(SrcUnit, out-port, DestUnit, input-port)
     (SrcUnit could verify connect limit)
     (SrcUnit should verify ImgInfo from port)

5. connect finish

*/

/////////////////////////////////////

/*
	SEM_WAIT(ISF_SEM_ID);
	_isf_data_reguser();
	SEM_SIGNAL(ISF_SEM_ID);
*/
UINT32 _isf_div64(UINT64 dividend, UINT64 divisor)
{
	if (divisor == 0) {
		return 0;
	}
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	return (UINT32)(dividend / divisor);
#else
	do_div(dividend, divisor);
	return (UINT32)(dividend);
#endif
}
EXPORT_SYMBOL(_isf_div64);

ISF_RV _isf_unit_verify(ISF_UNIT *p_unit, char *act)
{
	ISF_RV r = ISF_OK;
	if (p_unit->sign != ISF_SIGN_UNIT) {
		r = ISF_ERR_INVALID_SIGN;
		goto V1_END;
	}
	if (p_unit->unit_name == 0) {
		r = ISF_ERR_INVALID_NAME;
		goto V1_END;
	}
	if (p_unit->unit_module == 0) {
		r = ISF_ERR_INVALID_NAME;
		goto V1_END;
	}
	if (p_unit->port_in_count == 0) {
		r = ISF_ERR_INVALID_VALUE;
		goto V1_END;
	} else {
		if (p_unit->port_in == 0) {
			r = ISF_ERR_NULL_POINTER;
			goto V1_END;
		}
		if (p_unit->port_incaps == 0) {
			r = ISF_ERR_NULL_POINTER;
			goto V1_END;
		}
	}
	if (p_unit->port_out_count == 0) {
		r = ISF_ERR_INVALID_VALUE;
		goto V1_END;
	} else {
		if (p_unit->port_out == 0) {
			r = ISF_ERR_NULL_POINTER;
			goto V1_END;
		}
		if (p_unit->port_outcaps == 0) {
			r = ISF_ERR_NULL_POINTER;
			goto V1_END;
		}
		if (p_unit->port_outcfg == 0) {
			r = ISF_ERR_NULL_POINTER;
			goto V1_END;
		}
		if (p_unit->port_outstate == 0) {
			r = ISF_ERR_NULL_POINTER;
			goto V1_END;
		}
	}
	/*
	if (p_unit->port_path_count == 0) {
		r = ISF_ERR_INVALID_VALUE;
		goto V1_END;
	} else {
		if (p_unit->port_path == 0) {
			r = ISF_ERR_NULL_POINTER;
			goto V1_END;
		}
	}
	*/
	return ISF_OK;
V1_END:
	if (p_unit->unit_name)
		DBG_ERR(" %s unit \"%s\", verify error %d\r\n",
				act, p_unit->unit_name, r);
	else
		DBG_ERR(" %s unit(%08x), verify error %d\r\n",
				act, (UINT32)p_unit, r);
	//ASSERT(r == ISF_OK);
	if (!(r == ISF_OK))
		debug_dump_current_stack();
	return r;
}

void _isf_unit_check_return(ISF_RV r)
{
	if (r != ISF_OK) {
		DBG_ERR("failed %d\r\n", r);
	}
}

/////////////////////////////////////////////////////////////
// util

BOOL _isf_frc_start(ISF_UNIT* p_unit, UINT32 nport, ISF_FRC *p_frc, UINT32 framepersecond)
{
	INT16 src_fr, dst_fr;
	if((!p_frc)) 
		return FALSE;
	DBG_IND("-%s%d: [frc] start\r\n", p_unit->unit_name, nport);
	if (p_frc->sample_mode != ISF_SAMPLE_OFF) {
		DBG_ERR("-%s%d: [frc] is under control, cannot start.\r\n", p_unit->unit_name, nport);
		return FALSE;
	}
	
	if ((framepersecond == VDO_FRC_OFF) || (framepersecond == VDO_FRC_DIRECT)) {
		// fps = [not control]
		p_frc->sample_mode = ISF_SAMPLE_ALL;
	}
	src_fr = GET_LO_UINT16(framepersecond);
	dst_fr = GET_HI_UINT16(framepersecond);
	DBG_MSG("-%s%d: [frc] frc(%d, %d)\r\n", p_unit->unit_name, nport, dst_fr, src_fr);

	if ((dst_fr > ISF_FRC_UNIT) || (src_fr > ISF_FRC_UNIT)) {
		DBG_ERR("-%s%d: [frc] frc(%d, %d) is out of limit! stop output.\r\n", p_unit->unit_name, nport, src_fr, dst_fr);
		p_frc->sample_mode = ISF_SAMPLE_OFF;
	}
	if ((dst_fr >= 1) && (src_fr == -1)) {
		// fps = [capture]
		p_frc->sample_mode = ISF_SAMPLE_CAPTURE;
		_isf_frc_update(p_unit, nport, p_frc, dst_fr); //it will only output dst_fr frame then stop
		p_frc->framepersecond_new = p_frc->framepersecond; //finish
		return TRUE;
	}
	else if ((dst_fr > 1) && (src_fr >= 1) && (dst_fr > src_fr)) {
		// fps = dst_fr/src_fr > 1.0
		p_frc->sample_mode = ISF_SAMPLE_PREVIEW;
		_isf_frc_update(p_unit, nport, p_frc, framepersecond); //it will equal to _isf_util_set_frc(&frc, 0);
		p_frc->framepersecond_new = p_frc->framepersecond; //finish
		return TRUE;
	}
	else if ((dst_fr >= 1) && (src_fr >= 1) && (dst_fr == src_fr)) {
		// fps = dst_fr/src_fr = 1.0
		p_frc->sample_mode = ISF_SAMPLE_PREVIEW;
		_isf_frc_update(p_unit, nport, p_frc, framepersecond);
		p_frc->framepersecond_new = p_frc->framepersecond; //finish
		return TRUE;
	}
	else if ((dst_fr >= 1) && (src_fr > 1) && (dst_fr < src_fr)) {
		// fps = dst_fr/src_fr < 1.0
		p_frc->sample_mode = ISF_SAMPLE_PREVIEW;
		_isf_frc_update(p_unit, nport, p_frc, framepersecond);
		p_frc->framepersecond_new = p_frc->framepersecond; //finish
		return TRUE;
	}
	
	DBG_WRN("-%s%d: [frc] frc(%d, %d) is invalid! stop output.\r\n", p_unit->unit_name, nport, src_fr, dst_fr);
	p_frc->sample_mode = ISF_SAMPLE_OFF;
	return TRUE;
}
EXPORT_SYMBOL(_isf_frc_start);

BOOL _isf_frc_stop(ISF_UNIT* p_unit, UINT32 nport, ISF_FRC *p_frc)
{
	if((!p_frc)) 
		return FALSE;
	DBG_IND("-%s%d: [frc] stop\r\n", p_unit->unit_name, nport);
	p_frc->sample_mode = ISF_SAMPLE_OFF;
	return TRUE;
}
EXPORT_SYMBOL(_isf_frc_stop);

BOOL _isf_frc_update(ISF_UNIT* p_unit, UINT32 nport, ISF_FRC *p_frc, UINT32 framepersecond)
{
	UINT32               src_fr = 0;
	UINT32               dst_fr = 0;
	if((!p_frc)) 
		return FALSE;

	if (p_frc->sample_mode == ISF_SAMPLE_OFF) {
		return TRUE;
	}
	if (p_frc->sample_mode == ISF_SAMPLE_ALL) {
		return TRUE;
	}

	p_frc->frm_counter     = 0;
	p_frc->output_counter  = 0;
	p_frc->rate_counter    = 0;
	p_frc->framepersecond  = framepersecond;
	DBG_IND("-%s%d: [frc] update, frc = 0x%x\r\n", p_unit->unit_name, nport, framepersecond);
	if (GET_LO_UINT16(framepersecond) == 0) {
		p_frc->rate = ISF_FRC_UNIT;
	} else {
		src_fr              = GET_LO_UINT16(framepersecond);
		dst_fr              = GET_HI_UINT16(framepersecond);
		
		if (src_fr > 7680) {  // if src=p30, it is equal to 1/256s
			DBG_ERR("-%s%d: [frc] src=%d > 7680! stop output.\r\n", p_unit->unit_name, nport, src_fr);
			p_frc->sample_mode = ISF_SAMPLE_OFF;
		}
		if (dst_fr > (0xffffffff/ISF_FRC_UNIT)) {
			DBG_ERR("-%s%d: [frc] dst=%d > %u! stop output.\r\n", p_unit->unit_name, nport, dst_fr, (0xffffffff/ISF_FRC_UNIT));
			p_frc->sample_mode = ISF_SAMPLE_OFF;
		}
		/*
		if (src_fr > ISF_FRC_UNIT) {
			DBG_ERR("-%s%d: [frc] frc(%d, %d) is out of limit! stop output.\r\n", p_unit->unit_name, nport, dst_fr, src_fr);
			p_frc->sample_mode = ISF_SAMPLE_OFF;
			return FALSE;
		}
		*/
		if ((src_fr <= 0) || (dst_fr <= 0)) {
			//DBG_ERR("-%s%d: [frc] frc(%d, %d) is invalid! stop output.\r\n", p_unit->unit_name, nport, dst_fr, src_fr);
			p_frc->sample_mode = ISF_SAMPLE_OFF;
			return FALSE;
		}
		if (src_fr > 240) {
			p_frc->rate = ((dst_fr*ISF_FRC_UNIT +(src_fr/2))/src_fr);   //rate=ROUND(dst*UNIT/src) ...see IVOT_N12089_CO-38 
		} else {
			p_frc->rate = (dst_fr*ISF_FRC_UNIT/src_fr)+1;   //rate=(dst*UNIT/src)
		}
	}
	#if (DEBUG_FRC == TRUE)
	if (nport==0) {
	DBG_DUMP("-%s%d: [frc] dst_fps*FRC_UNIT = %u\r\n", 
		p_unit->unit_name, nport,	dst_fr*ISF_FRC_UNIT);
	DBG_DUMP("-%s%d: [frc] set frc = 0x%x, dst_fps = %d, src_fps = %d, rate = %d\r\n", 
		p_unit->unit_name, nport,	framepersecond,dst_fr,src_fr,p_frc->rate);
	}
	#endif

	return TRUE;
}
EXPORT_SYMBOL(_isf_frc_update);

BOOL _isf_frc_update_imm(ISF_UNIT* p_unit, UINT32 nport, ISF_FRC *p_frc, UINT32 framepersecond_new)
{
	if((!p_frc)) 
		return FALSE;

	DBG_IND("-%s%d: [frc] update imm, frc = 0x%x\r\n", p_unit->unit_name, nport, framepersecond_new);
	p_frc->framepersecond_new = framepersecond_new;
	return TRUE;
}
EXPORT_SYMBOL(_isf_frc_update_imm);

#if (DEBUG_FRC == TRUE)
#include "comm/hwclock.h"
UINT64 t0 = 0;
UINT64 t1 = 0;
#endif

UINT32 _isf_frc_is_select(ISF_UNIT* p_unit, UINT32 nport, ISF_FRC *p_frc)
{
	UINT32 ret = 0, reset = FALSE;
	if((!p_frc)) 
		return 0;

	if (p_frc->sample_mode == ISF_SAMPLE_OFF) {
		#if (DEBUG_FRC == TRUE)
		if (nport == 0)
			DBG_DUMP("-%s%d: [frc] off\r\n", p_unit->unit_name, nport);
		#endif
		return 0;
	}
	if (p_frc->sample_mode == ISF_SAMPLE_ALL) {
		//rate = 1.0
		p_frc->output_counter++;
		#if (DEBUG_FRC == TRUE)
		if (nport == 0)
			DBG_DUMP("-%s%d: [frc] T1\r\n", p_unit->unit_name, nport);
		if (nport == 0) {
			UINT64 ct = hwclock_get_longcounter();
			t0 = t1;
			t1 = ct;
			DBG_DUMP("-%s%d: [diff T] %u ms\r\n", p_unit->unit_name, nport, _isf_div64(t1-t0,1000));
		}
		#endif
		ret = 1;
		return ret;
	}

	if (p_frc->rate >= ISF_FRC_UNIT) {
		//rate > 1.0
		p_frc->output_counter++;
		#if (DEBUG_FRC == TRUE)
		if (nport == 0)
			DBG_DUMP("-%s%d: [frc] T2\r\n",	p_unit->unit_name, nport);
		if (nport == 0) {
			UINT64 ct = hwclock_get_longcounter();
			t0 = t1;
			t1 = ct;
			DBG_DUMP("-%s%d: [diff T] %u ms\r\n", p_unit->unit_name, nport, _isf_div64(t1-t0,1000));
		}
		#endif
		ret = 1;
	}
	else if (p_frc->frm_counter == 0) {
		p_frc->output_counter++;
		#if (DEBUG_FRC == TRUE)
		if (nport == 0)
			DBG_DUMP("-%s%d: [frc] T3\r\n",	p_unit->unit_name, nport);
		if (nport == 0) {
			UINT64 ct = hwclock_get_longcounter();
			t0 = t1;
			t1 = ct;
			DBG_DUMP("-%s%d: [diff T] %u ms\r\n", p_unit->unit_name, nport, _isf_div64(t1-t0,1000));
		}
		#endif
		ret = 1;
	}
	else {
		p_frc->rate_counter += p_frc->rate;
		if (p_frc->rate_counter >= ISF_FRC_UNIT) {
			p_frc->rate_counter -= ISF_FRC_UNIT;
			p_frc->output_counter++;
			#if (DEBUG_FRC == TRUE)
			if (nport == 0)
				DBG_DUMP("-%s%d: [frc] T4\r\n",	p_unit->unit_name, nport);
			if (nport == 0) {
				UINT64 ct = hwclock_get_longcounter();
				t0 = t1;
				t1 = ct;
				DBG_DUMP("-%s%d: [diff T] %u ms\r\n", p_unit->unit_name, nport, _isf_div64(t1-t0,1000));
			}
			#endif
			ret = 1;
		}
	}
	p_frc->frm_counter++;
	DBG_MSG("-%s%d: [frc] frm_cnt %d , out_cnt %d\r\n", p_unit->unit_name, nport, p_frc->frm_counter,p_frc->output_counter);
	// reset counter
	if (p_frc->framepersecond == 0 && p_frc->frm_counter >= 30) {
		reset = TRUE;
	}
	else if ((p_frc->framepersecond != 0) && (p_frc->frm_counter >= GET_LO_UINT16(p_frc->framepersecond))) {
		reset = TRUE;
	}
	#if (DEBUG_FRC == TRUE)
	DBG_DUMP("-%s%d: [frc] framepersecond= %d, counter = %d\r\n", p_unit->unit_name, nport, GET_LO_UINT16(p_frc->framepersecond), p_frc->frm_counter);
	#endif
	if (reset) {
		if(p_frc->framepersecond_new != p_frc->framepersecond) {
			#if (DEBUG_FRC == TRUE)
			if (nport == 0)
				DBG_DUMP("-%s%d: [frc] reset, frc_new = 0x%x\r\n", p_unit->unit_name, nport, p_frc->framepersecond_new);
			#endif
			_isf_frc_update(p_unit, nport, p_frc, p_frc->framepersecond_new);
		} else {
			p_frc->frm_counter     = 0;
			p_frc->output_counter  = 0;
			p_frc->rate_counter    = 0;
			#if (DEBUG_FRC == TRUE)
			if (nport == 0)
				DBG_DUMP("-%s%d: [frc] reset\r\n", p_unit->unit_name, nport);
			#endif
		}
	}
	return ret;
}
EXPORT_SYMBOL(_isf_frc_is_select);

