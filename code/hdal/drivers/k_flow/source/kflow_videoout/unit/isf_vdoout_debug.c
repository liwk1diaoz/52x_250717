#include "isf_vdoout_dbg.h"
#include "isf_vdoout_int.h"

//#ifdef DEBUG
unsigned int isf_vdoout_debug_level = NVT_DBG_WRN;
module_param_named(isf_vdoout_debug_level, isf_vdoout_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoout_debug_level, "vdoout debug level");
//#endif
///////////////////////////////////////////////////////////////////////////////
void isf_vdoout_dump_status(int (*dump)(const char *fmt, ...), ISF_UNIT *p_thisunit)
{
	#define ONE_SECOND 1000000
	VDOOUT_CONTEXT* p_ctx = NULL;
	UINT32 i;
	register UINT32 k;
	UINT16 avg_cnt[16] = {0};
	BOOL do_en = FALSE;

	if(!p_thisunit || !dump)
		return;

    p_ctx = (VDOOUT_CONTEXT*)(p_thisunit->refdata);

	for (i = 0; i < ISF_VDOOUT_OUTPUT_MAX; i++)
	{
		if ((p_thisunit->port_outstate[i]->state) != (ISF_PORT_STATE_OFF)) {//port current state
			do_en = TRUE;
		}
	}
	if(!do_en)
		return;

	{
		BOOL do_wait_renew = FALSE;
		{
			if (_isf_probe_is_ready(p_thisunit, ISF_IN(0)) != ISF_OK) { //check and renew probe count
				//dump("in[%d] - work status expired!\r\n", 0);
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
	dump("------------------------- VIDEOOUT %-2d IN WORK STATUS -------------------------\r\n", p_ctx->dev);
	dump("in    PUSH  drop  wrn   err   PROC  drop  wrn   err   REL\r\n");
	{
		ISF_IN_STATUS *p_status = &(p_thisunit->port_ininfo[i]->status.in);
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

	dump("\r\n");

}

