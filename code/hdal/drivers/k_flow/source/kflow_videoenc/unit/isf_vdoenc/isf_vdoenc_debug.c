
#include "isf_vdoenc_internal.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__    	  isf_vdoenc_d
#define __DBGLVL__		  8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_vdoenc_d_debug_level = NVT_DBG_WRN;
module_param_named(debug_level_isf_vdoenc_d, isf_vdoenc_d_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_isf_vdoenc_d, "isf_vdoenc_debug debug level");
///////////////////////////////////////////////////////////////////////////////

void isf_vdoenc_dump_status(int (*dump)(const char *fmt, ...), ISF_UNIT *p_thisunit)
{
	#define ONE_SECOND 1000000

	UINT32 i;
	UINT32 did=0;  // videoenc only 1 device
	register UINT32 k;
	UINT16 avg_cnt[16] = {0};
	BOOL do_en = FALSE;
	if(!p_thisunit || !dump)
		return;
	for (i = 0; i < ISF_VDOENC_OUT_NUM; i++)
	{
		if ((p_thisunit->port_outstate[i]->state) != (ISF_PORT_STATE_OFF)) {//port current state
			do_en = TRUE;
		}
	}
	if(!do_en)
		return;

	{
		BOOL do_wait_renew = FALSE;
		for (i = 0; i < ISF_VDOENC_IN_NUM; i++)
		{
			if ((p_thisunit->port_outstate[i]->state) != (ISF_PORT_STATE_RUN)) {//port current state
				continue;
			}
			if (_isf_probe_is_ready(p_thisunit, ISF_IN(i)) != ISF_OK) { //check and renew probe count
				//dump("in[%d] - work status expired!\r\n", i);
				do_wait_renew = TRUE;
			}
		}
		for (i = 0; i < ISF_VDOENC_OUT_NUM; i++)
		{
			if ((p_thisunit->port_outstate[i]->state) != (ISF_PORT_STATE_RUN)) {//port current state
				continue;
			}
			if (_isf_probe_is_ready(p_thisunit, ISF_OUT(i)) != ISF_OK) { //check and renew probe count
				//dump("out[%d] - work status expired!\r\n", i);
				do_wait_renew = TRUE;
			}
		}
		if (do_wait_renew) {
			dump("\r\nforce reset and wait 1 secnod...\r\n");
			DELAY_M_SEC(1000);
		}
	}
	
	//dump in status
	i = 0;
	dump("------------------------- VIDEOENC %-2d IN WORK STATUS --------------------------\r\n", did);
	dump("in    PUSH  drop  wrn   err   PROC  drop  wrn   err   REL\r\n");
	for (i = 0; i < ISF_VDOENC_IN_NUM; i++)
	{
		ISF_IN_STATUS *p_status = &(p_thisunit->port_ininfo[i]->status.in);
		if ((p_thisunit->port_outstate[i]->state) != (ISF_PORT_STATE_RUN)) {//port current state
			continue;
		}
		//calc average count in 1 second
		for(k=0; k<16; k++) {
			if(p_status->ts_push[ISF_TS_SECOND] > 0) {
				avg_cnt[k] = _isf_div64(((UINT64)p_status->total_cnt[k]) * ONE_SECOND + (p_status->ts_push[ISF_TS_SECOND]>>1), p_status->ts_push[ISF_TS_SECOND]);
			} else {
				avg_cnt[k] = 0;
			}
		}
		dump("%-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d\r\n",
			i,
			avg_cnt[12], avg_cnt[13], avg_cnt[14], avg_cnt[15],
			avg_cnt[4], avg_cnt[5], avg_cnt[6], avg_cnt[7],
			avg_cnt[0]
			);
	}

	//dump out status
	dump("------------------------- VIDEOENC %-2d OUT WORK STATUS -------------------------\r\n", did);
	dump("out   NEW   drop  wrn   err   PROC  drop  wrn   err   PUSH  drop  wrn   err\r\n");
	for (i = 0; i < ISF_VDOENC_OUT_NUM; i++)
	{
		ISF_OUT_STATUS *p_status = &(p_thisunit->port_outinfo[i]->status.out);
		if ((p_thisunit->port_outstate[i]->state) != (ISF_PORT_STATE_RUN)) {//port current state
			continue;
		}
		//calc average count in 1 second
		for(k=0; k<16; k++) {
			if(p_status->ts_new[ISF_TS_SECOND] > 0) {
				avg_cnt[k] = _isf_div64(((UINT64)p_status->total_cnt[k]) * ONE_SECOND + (p_status->ts_new[ISF_TS_SECOND]>>1), p_status->ts_new[ISF_TS_SECOND]);
			} else {
				avg_cnt[k] = 0;
			}
		}
		dump("%-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d  %-4d\r\n",
			i,
			avg_cnt[12], avg_cnt[13], avg_cnt[14], avg_cnt[15],
			avg_cnt[8], avg_cnt[9], avg_cnt[10], avg_cnt[11],
			avg_cnt[0], avg_cnt[1], avg_cnt[2], avg_cnt[3]
			);
	}

	//dump user status
	dump("------------------------- VIDEOENC %-2d USER WORK STATUS -------------------------\r\n", did);
	dump("out   PULL   drop  wrn   err   REL\r\n");
	for (i = 0; i < ISF_VDOENC_OUT_NUM; i++)
	{
		ISF_USER_STATUS *p_status = &(p_thisunit->port_outinfo[i]->status.out);
		if ((p_thisunit->port_outstate[i]->state) != (ISF_PORT_STATE_RUN)) {//port current state
			continue;
		}
		//calc average count in 1 second
		for(k=0; k<8; k++) {
			if(p_status->ts_pull[ISF_TS_SECOND] > 0) {
				avg_cnt[k] = _isf_div64(((UINT64)p_status->total_cnt[16+k]) * ONE_SECOND + (p_status->ts_pull[ISF_TS_SECOND]>>1), p_status->ts_pull[ISF_TS_SECOND]);
			} else {
				avg_cnt[k] = 0;
			}
		}
		dump("%-4d  %-4d  %-4d  %-4d  %-4d  %-4d\r\n",
			i,
			avg_cnt[4], avg_cnt[5], avg_cnt[6], avg_cnt[7],
			avg_cnt[0]
			);
	}
	dump("\r\n");
}

