//#include "SysKer.h"
#include "MovieStampAPI.h"
#include "MovieStampInt.h"
#include "kwrap/error_no.h"
#include "kwrap/flag.h"
#include <kwrap/util.h>

#define __MODULE__          MovieStampTask
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

static BOOL     g_bMovieStampTskOpened = FALSE;
static BOOL     g_bMovieStampTskClosing = FALSE;
static MOVIESTAMP_UPDATE_CB g_MovieStampUpdateCb = NULL;
static MOVIESTAMP_TRIGGER_UPDATE_CHECK_CB g_MovieStampTrigUpdateChkCb = NULL;
#define PRI_MOVIESTAMP          9//10
#define STKSIZE_MOVIESTAMP      4096
static UINT32 g_moviestamp_tsk_run= 0;
static UINT32 g_is_moviestamp_tsk_running= 0;

ER MovieStampTsk_Open(void)
{
	if (g_bMovieStampTskOpened) {
		return E_SYS;
	}
	MovieStamp_InstallID();

	vos_flag_clr(FLG_ID_MOVIESTAMP, FLGMOVIESTAMP_ALL);
	g_bMovieStampTskOpened = TRUE;
	g_bMovieStampTskClosing = FALSE;
	g_moviestamp_tsk_run =1;
	MOVIESTAMPTSK_ID=vos_task_create(MovieStampTsk,  0, "MovieStampTsk",   PRI_MOVIESTAMP,	STKSIZE_MOVIESTAMP);
	vos_task_resume(MOVIESTAMPTSK_ID);
	return E_OK;
}

ER MovieStampTsk_Close(void)
{
	UINT32  delay_cnt=0;

	if (!g_bMovieStampTskOpened) {
		return E_SYS;
	}
	g_bMovieStampTskClosing = TRUE;

	if (g_is_moviestamp_tsk_running) {

		g_moviestamp_tsk_run = 0;       // set this flag and task will return by itself, no need to destroy
		vos_flag_set(FLG_ID_MOVIESTAMP, FLGMOVIESTAMP_IDLE);

		delay_cnt = 50;
		while (g_is_moviestamp_tsk_running && delay_cnt) {
			CHKPNT;
			vos_util_delay_ms(10);
			delay_cnt --;
		}
		if (g_is_moviestamp_tsk_running == TRUE) {
			DBG_WRN("Destroy DispTsk\r\n");
			vos_task_destroy(MOVIESTAMPTSK_ID);
		}

	}

	MovieStamp_UnInstallID();
	g_bMovieStampTskOpened = FALSE;

	return E_OK;
}

void MovieStampTsk_TrigUpdate(void)
{
	set_flg(FLG_ID_MOVIESTAMP, FLGMOVIESTAMP_UPDATE);
}

void MovieStampTsk_RegTrigUpdateChkCB(MOVIESTAMP_TRIGGER_UPDATE_CHECK_CB fpMovieStampTrigUpdateChkCb)
{
	g_MovieStampTrigUpdateChkCb = fpMovieStampTrigUpdateChkCb;
}

void MovieStampTsk_RegUpdateCB(MOVIESTAMP_UPDATE_CB fpMovieStampUpdate)
{
	g_MovieStampUpdateCb = fpMovieStampUpdate;
}

BOOL MovieStampTsk_IsOpen(void)
{
	return g_bMovieStampTskOpened;
}

THREAD_RETTYPE MovieStampTsk(void)
{
	FLGPTN  FlgPtn;

	THREAD_ENTRY();
	g_is_moviestamp_tsk_running = 1;

	while (g_moviestamp_tsk_run) {
		vos_flag_wait(&FlgPtn, FLG_ID_MOVIESTAMP, FLGMOVIESTAMP_UPDATE| FLGMOVIESTAMP_IDLE, TWF_ORW | TWF_CLR);

		if (FlgPtn & FLGMOVIESTAMP_UPDATE) {
			if (g_MovieStampUpdateCb && (g_bMovieStampTskClosing == FALSE)) {
				if (g_MovieStampTrigUpdateChkCb && g_MovieStampTrigUpdateChkCb()==1) {
					g_MovieStampUpdateCb();
				}
			}
		}
	}
	g_is_moviestamp_tsk_running = 0;

	THREAD_RETURN(0);
}

//@}