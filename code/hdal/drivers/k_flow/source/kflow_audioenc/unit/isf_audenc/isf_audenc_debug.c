//#include "isf_audenc_int.h"
//#include "nvtmpp.h"

#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "nmediarec_audenc.h"
#include "kwrap/task.h"

#define DELAY_M_SEC(x)              vos_task_delay_ms(x)
#define DELAY_U_SEC(x)              vos_task_delay_us(x)

void isf_audenc_dump_status(int (*dump)(const char *fmt, ...), ISF_UNIT *p_thisunit)
{
	#define ONE_SECOND 1000000
	//#define ISF_AUDENC_IN_NUM          			16
	//#define ISF_AUDENC_OUT_NUM         			16

	UINT32 i;
	UINT32 did=0;  // audioenc only 1 device
	register UINT32 k;
	UINT16 avg_cnt[16] = {0};
	BOOL do_en = FALSE;
	if(!p_thisunit || !dump)
		return;
	for (i = 0; i < PATH_MAX_COUNT; i++)
	{
		if ((p_thisunit->port_outstate[i]->state) != (ISF_PORT_STATE_OFF)) {//port current state
			do_en = TRUE;
		}
	}
	if(!do_en)
		return;

	{
		BOOL do_wait_renew = FALSE;
		for (i = 0; i < PATH_MAX_COUNT; i++)
		{
			if ((p_thisunit->port_outstate[i]->state) != (ISF_PORT_STATE_RUN)) {//port current state
				continue;
			}
			if (_isf_probe_is_ready(p_thisunit, ISF_IN(i)) != ISF_OK) { //check and renew probe count
				//dump("in[%d] - work status expired!\r\n", i);
				do_wait_renew = TRUE;
			}
		}
		for (i = 0; i < PATH_MAX_COUNT; i++)
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
	dump("------------------------- AUDIOENC %-2d IN WORK STATUS --------------------------\r\n", did);
	dump("in    PUSH  drop  wrn   err   PROC  drop  wrn   err   REL\r\n");
	for (i = 0; i < PATH_MAX_COUNT; i++)
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
	dump("------------------------- AUDIOENC %-2d OUT WORK STATUS -------------------------\r\n", did);
	dump("out   NEW   drop  wrn   err   PROC  drop  wrn   err   PUSH  drop  wrn   err\r\n");
	for (i = 0; i < PATH_MAX_COUNT; i++)
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
	dump("------------------------- AUDIOENC %-2d USER WORK STATUS -------------------------\r\n", did);
	dump("out   PULL   drop  wrn   err   REL\r\n");
	for (i = 0; i < PATH_MAX_COUNT; i++)
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

