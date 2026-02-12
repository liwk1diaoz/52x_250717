/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       SxTimer.c
    @ingroup    mIPRJAPKeyIO

    @brief      Execute function by timer count

    @note       Nothing.

    @date       2009/04/22
*/

/** \addtogroup mIPRJAPKeyIO */
//@{


#include <kwrap/nvt_type.h>
#include <kwrap/error_no.h>
#include <kwrap/task.h>
#include <kwrap/util.h>
#include <kwrap/cmdsys.h>
#include "kwrap/sxcmd.h"
#include "SxTimer/SxTimer.h"
#include "Utility/SwTimer.h"
#include "SxTimer_int.h"
#include <unistd.h>

//--------------------- Debug Definition -----------------------------
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          SxTmr
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

#define SX_TIMER_CNT_INTERVAL   20//ms

static SX_TIMER_ENTRY *g_pSxTimerHead = NULL;
static SX_TIMER_ENTRY *g_pSxTimerTail = NULL;
static BOOL g_bSxTimerOpened = FALSE;
static BOOL g_bSxTimerBeginClose = FALSE;
static UINT32 g_uiSxTimerNum = 0;
static SWTIMER_ID g_uiSxTimerID = -1;
static INT32      g_uiCurrSx = 0;
static UINT32     g_uiKeyScanTimerCnt = 0;

void SxTimer_Init(void)
{
	SXTIMERTSK_ID = vos_task_create(SxTimerTsk, 0, "sxtimer_tsk", SXTIMER_PRIORITY, SXTIMER_STKSIZE);
	if (0 == SXTIMERTSK_ID) {
		DBG_ERR("SwTimer create failed\r\n");
        return;
	}
	vos_task_resume(SXTIMERTSK_ID);
}

/**
  Open DetectSrv task

  Open DetectSrv task.

  Return value is listed below:
  E_SYS: Task is already opened
  E_OK: No error

  @param PSX_TIMER pSxTimer: SxTimer detecting function object
  @return ER
*/
ER SxTimer_Open(void)
{
	DBG_FUNC_BEGIN("\r\n");
	if (g_bSxTimerOpened) {
		DBG_ERR("already opened!!! \r\n");
		return E_SYS;
	}

	g_bSxTimerOpened = TRUE;
	//THREAD_RESUME(SXTIMERTSK_ID);
	DBG_FUNC_END("\r\n");
	return E_OK;

}

/**
  Close DetectSrv task

  Close DetectSrv task.

  Return value is listed below:
  E_SYS: Task is already closed
  E_OK: No error

  @param void
  @return ER
*/
static void _SxTimer_Close(void)
{
	SwTimer_Close(g_uiSxTimerID);

	g_bSxTimerOpened = FALSE;
	g_uiSxTimerNum = 0;
	//ter_tsk(SXTIMERTSK_ID);
	g_pSxTimerHead = NULL;
	g_pSxTimerTail = NULL;
	//THREAD_DESTROY(SXTIMERTSK_ID);
	return;
}
ER SxTimer_Close(void)
{
	DBG_FUNC_BEGIN("\r\n");
	if (g_bSxTimerOpened == FALSE) {
		DBG_ERR("already closed!!! \r\n");
		return E_SYS;
	}
	SxTimer_Suspend();
	g_bSxTimerBeginClose = TRUE;
	return E_OK;
}

void SxTimer_Dump(void);

INT32 SxTimer_AddItem(SX_TIMER_ENTRY *pEntry)
{
	if (!pEntry) {
		return -1;
	}
	if (g_pSxTimerHead == NULL) {
		pEntry->id = g_uiSxTimerNum;
		pEntry->pNext = NULL;

		g_pSxTimerHead = pEntry;
		g_pSxTimerTail = pEntry;
		g_uiSxTimerNum++;
	} else {
		pEntry->id = g_uiSxTimerNum;
		pEntry->pNext = NULL;

		g_pSxTimerTail->pNext = pEntry;
		g_pSxTimerTail = pEntry;
		g_uiSxTimerNum++;
	}
#if (__DBGLVL__ >= NVT_DBG_MSG)
	SxTimer_Dump();
#endif
	return pEntry->id;
}

//#NT#2010/10/19#Lily Kao -begin
/**
  Get detected function active-value

  Get detected function active-value.

  @param UINT32 FuncID: function ID registered by SxTimer_Open

  @return BOOL Active: [OUTPUT] TRUE for active, FALSE for inactive

*/
BOOL SxTimer_GetFuncActive(UINT32 FuncID)
{
	BOOL bActive = FALSE;
	SX_TIMER_ENTRY *pEntry = g_pSxTimerHead;
	while (pEntry) {
		if (pEntry->id == (INT32)FuncID) {
			bActive = pEntry->bEnable;
			DBG_IND("SXTIMER[%s]=%d\r\n", pEntry->pName, pEntry->bEnable);
			break;
		}
		pEntry = pEntry->pNext;
	}
	return bActive;
}
//#NT#2010/10/19#Lily Kao -end

/**
  Set detected function active

  Set detected function active.
  If a function is set inactive, the function will not be serviced in SxTimer.

  @param UINT32 FuncID: function ID registered by SxTimer_Open
         BOOL Active: TRUE for active, FALSE for inactive
  @return void

*/
void SxTimer_SetFuncActive(UINT32 FuncID, BOOL Active)
{
	SX_TIMER_ENTRY *pEntry = g_pSxTimerHead;
	while (pEntry) {
		if (pEntry->id == (INT32)FuncID) {
			pEntry->bEnable = Active;
			DBG_IND("SXTIMER[%s]=%d\r\n", pEntry->pName, pEntry->bEnable);
			break;
		}
		pEntry = pEntry->pNext;
	}
}


/**
  Suspend DetectSrv task

  Suspend DetectSrv task. The DetectSrv timer interrupt will be disabled.
  All scan function will be disabled.

  @param void
  @return void
*/
void SxTimer_Suspend(void)
{
	SwTimer_Stop(g_uiSxTimerID);
}

/**
  Resume DetectSrv task

  Resume DetectSrv task.

  @param void
  @return void
*/
void SxTimer_Resume(void)
{
	SwTimer_Start(g_uiSxTimerID);
}

UINT32 SxTimer_GetData(SXTIMER_DATA_SET attribute)
{
	switch (attribute) {
	case SXTIMER_TIMER_BASE:
		return SX_TIMER_CNT_INTERVAL;
	case SXTIMER_CURR_CNT:
		return g_uiKeyScanTimerCnt;
	default:
		DBG_ERR("no this attribute");
		return 0;
	}

}


void SxTimer_Dump(void)
{
	SX_TIMER_ENTRY *pEntry = g_pSxTimerHead;
	DBG_DUMP("\r\n######## SxTimer (CntBase=%d ms)########\r\n", SX_TIMER_CNT_INTERVAL);
	while (pEntry) {
		if (pEntry->id == g_uiCurrSx) {
			DBG_DUMP("########################################################################\r\n");
			DBG_DUMP("^RID=%02d: Active=%d, TimerCnt=%02d(%d ms), this=%08x next=%08x, Func=%s, \r\n",
					 pEntry->id,
					 pEntry->bEnable,
					 pEntry->uiPeriodCnt,
					 pEntry->uiPeriodCnt * SX_TIMER_CNT_INTERVAL,
					 pEntry,
					 pEntry->pNext,
					 pEntry->pName);
			DBG_DUMP("########################################################################\r\n");
		} else {
			DBG_DUMP("ID=%02d: Active=%d, TimerCnt=%02d(%d ms), this=%08x next=%08x, Func=%s, \r\n",
					 pEntry->id,
					 pEntry->bEnable,
					 pEntry->uiPeriodCnt,
					 pEntry->uiPeriodCnt * SX_TIMER_CNT_INTERVAL,
					 pEntry,
					 pEntry->pNext,
					 pEntry->pName);
		}
		pEntry = pEntry->pNext;
	}
}

//fix for CID 43253 - begin
static BOOL gLoopQuit = FALSE;
//fix for CID 43253 - end
extern void DetectSrvQuit(void);
void DetectSrvQuit(void)
{
	gLoopQuit = TRUE;
}

THREAD_DECLARE(SxTimerTsk, arglist)
{
	BOOL quit = FALSE;
	THREAD_ENTRY();

	if (SwTimer_Open(&g_uiSxTimerID, NULL) != E_OK) {
		DBG_ERR("open failed\r\n");
		//fix for CID 43253 - begin
		while (!gLoopQuit)
			//fix for CID 43253 - end
		{
			;
		}
	}
	SwTimer_Cfg(g_uiSxTimerID, SX_TIMER_CNT_INTERVAL, SWTIMER_MODE_FREE_RUN);
	SwTimer_Start(g_uiSxTimerID);
	while (!quit) {
		SX_TIMER_ENTRY *pEntry = g_pSxTimerHead;
		PROFILE_TASK_IDLE();

		SwTimer_WaitTimeup(g_uiSxTimerID);
		PROFILE_TASK_BUSY();
		g_uiKeyScanTimerCnt++;

		g_uiCurrSx = 0;
		while (pEntry) {
			if (pEntry->id == g_uiCurrSx) {
				if (pEntry->pfFunc != NULL &&
					pEntry->uiPeriodCnt != 0 &&
					pEntry->bEnable == TRUE) {
					if (g_uiKeyScanTimerCnt % pEntry->uiPeriodCnt == 0) {
						pEntry->pfFunc();    //call func
					}
				}
			}
			pEntry = pEntry->pNext;
			g_uiCurrSx++;
		}
		if (g_bSxTimerBeginClose == TRUE) {
			quit = TRUE;
			g_bSxTimerBeginClose = FALSE;
		}
	}
	_SxTimer_Close();
	THREAD_RETURN(0);
}







#if 0
#define MAX_DETECT_FUNCTION     100

static SX_TIMER_SRV g_SxTimer[MAX_DETECT_FUNCTION] = {0};

void SxTimerTest(UINT32 uiKeyScanTimerCnt)
{
	UINT32 i;
	for (i = 0; i < g_uiFuncNum; i++) {

		if (g_pSxTimer[i].fpDetFunction != NULL &&
			g_pSxTimer[i].uiDetTimerCnt != 0 &&
			g_pSxTimer[i].bActive == TRUE) {
			if (uiKeyScanTimerCnt % g_pSxTimer[i].uiDetTimerCnt == 0) {
				g_pSxTimer[i].fpDetFunction();
			}
		}
	}
#if 0
	for (i = 0; i < MAX_DETECT_FUNCTION; i++) {

		if (g_SxTimer[i].fpDetFunction != NULL &&
			g_SxTimer[i].uiDetTimerCnt != 0 &&
			g_SxTimer[i].bActive == TRUE) {
			if (uiKeyScanTimerCnt % g_SxTimer[i].uiDetTimerCnt == 0) {
				g_SxTimer[i].fpDetFunction();
			}
		} else {
			break;
		}
	}
#endif
}

void DumpSxTimer(void)
{
	UINT32 i;
	DBG_DUMP("\r\n######## Sx Timer (Cnt Base = %d ms), Function Num=%d ########\r\n", SX_TIMER_CNT_INTERVAL, g_uiFuncNum);
	for (i = 0; i < g_uiFuncNum; i++) {
		DBG_DUMP("ID=%02d, Active=%d, TimerCnt=%02d(interval=%d ms), FunctionName= %s \r\n",
				 i,
				 g_SxTimer[i].bActive,
				 g_SxTimer[i].uiDetTimerCnt,
				 g_SxTimer[i].uiDetTimerCnt * SX_TIMER_CNT_INTERVAL,
				 g_SxTimer[i].pFuncName);
	}
}


void DetectSrvTsk(void)
{
	UINT32 i;
	UINT32 uiFlag;
	if (timer_open(&g_uiSxTimerID, NULL) != E_OK) {
		DBG_ERR("open failed\r\n");
		while (1) {
			;
		}
	}
	while (1) {
		timer_waitTimeup(g_uiSxTimerID);
		g_uiKeyScanTimerCnt++;
		for (i = 0; i < MAX_DETECT_FUNCTION; i++) {

			if (g_SxTimer[i].fpDetFunction != NULL &&
				g_SxTimer[i].uiDetTimerCnt != 0 &&
				g_SxTimer[i].bActive == TRUE) {
				if (g_uiKeyScanTimerCnt % g_SxTimer[i].uiDetTimerCnt == 0) {
					g_SxTimer[i].fpDetFunction();
				}
			} else {
				break;
			}
		}
	}
}




void SxTimer_SetFuncActive(UINT32 FuncID, BOOL Active)
{
	if (FuncID == SX_FUNC_ID_UNKNOWN) {
		DBG_ERR("FuncID UNKNOWN\r\n");
	} else {
		g_SxTimer[FuncID].bActive = Active;
	}
}


/**
  Register function into SxTimer


  @param FP      fpDetFunction:[INPUT], function pointer
         UINT32  uiDetTimerCnt:[INPUT], base on 20ms, time interval to service the fpDetFunction,
         BOOL    bActive:[INPUT], TRUE for active, FALSE for inactive
         INT8    *pFuncName:[INPUT], for debug

  @return UINT32 uiFuncID:[OUTPUT] ID value, if register failed, the value will be SX_FUNC_ID_UNKNOWN

*/
UINT32 SxTimer_RegisterFunc(PSX_TIMER_SRV pSxTimer)
{
	UINT32 uiFunID = SX_FUNC_ID_UNKNOWN;
	if (g_bSxTimerOpened) {
		DBG_ERR("can NOT be called after SxTimer opened!!! \r\n");
		return SX_FUNC_ID_UNKNOWN;
	}

	if (pSxTimer->fpDetFunction == NULL || pSxTimer->uiDetTimerCnt == 0) {
		DBG_ERR("DetFunction and DetTimerCnt should NOT be NULL.\r\n");
		return SX_FUNC_ID_UNKNOWN;
	}


	if (g_uiFuncNum < MAX_DETECT_FUNCTION) {
		uiFunID = g_uiFuncNum;
		g_SxTimer[g_uiFuncNum].fpDetFunction = pSxTimer->fpDetFunction;
		g_SxTimer[g_uiFuncNum].uiDetTimerCnt = pSxTimer->uiDetTimerCnt;
		g_SxTimer[g_uiFuncNum].bActive = pSxTimer->bActive;
		g_SxTimer[g_uiFuncNum].pFuncName = pSxTimer->pFuncName;

		g_uiFuncNum++;

		return uiFunID;

	} else {
		DBG_ERR("item is full !!!\r\n");
		return SX_FUNC_ID_UNKNOWN;
	}

}

#endif

static BOOL SxTimerSxCmd_Dump(unsigned char argc, char **argv)
{
	SxTimer_Dump();
	return TRUE;
}

static SXCMD_BEGIN(sxt, "SxTimer")
SXCMD_ITEM("dump", SxTimerSxCmd_Dump, "dump all entries")
SXCMD_END()

int sxt_cmd_showhelp(void)
{
	UINT32 cmd_num = SXCMD_NUM(sxt);
	UINT32 loop = 1;

	DBG_DUMP("---------------------------------------------------------------------\r\n");
	DBG_DUMP("  %s\n", "SxTimer");
	DBG_DUMP("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		DBG_DUMP("%15s : %s\r\n", sxt[loop].p_name, sxt[loop].p_desc);
	}
	return 0;
}

#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
MAINFUNC_ENTRY(sxt, argc, argv)
#else
int sxt_cmd_execute(unsigned char argc, char **argv)
#endif
{
	UINT32 cmd_num = SXCMD_NUM(sxt);
	UINT32 loop;
	int    ret;
	unsigned char ucargc = 0;

	//DBG_DUMP("%d, %s, %s, %s, %s\r\n", (int)argc, argv[0], argv[1], argv[2], argv[3]);

	if (strncmp(argv[1], "?", 2) == 0) {
		sxt_cmd_showhelp();
		return -1;
	}

	if (argc < 1) {
		DBG_ERR("input param error\r\n");
		return -1;
	}
	ucargc = argc - 2;
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], sxt[loop].p_name, strlen(argv[1])) == 0) {
			ret = sxt[loop].p_func(ucargc, &argv[2]);
			return ret;
		}
	}

	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		sxt_cmd_showhelp();
		return -1;
	}
	return 0;
}


