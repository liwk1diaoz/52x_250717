/*
    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

    @file       LogFile.c
    @author     Lincy Lin
    @date       2016/2/1
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "LogFileInt.h"
#include <time.h>
#include <comm/hwclock.h>

#define __MODULE__          LogFileTsk
#define __DBGLVL__          2
#include "kwrap/debug.h"
extern unsigned int LogFileTsk_debug_level;

#if defined(_BSP_NA51000_) || defined(_BSP_NA51023_)
#define DBGLVL_NORMAL       2
#else
#define DBGLVL_NORMAL       1
#endif

#if defined (__UITRON) || defined (__ECOS)
#include "uart.h"
#elif defined(__FREERTOS)
#include "uart.h"
#include "console.h"
#define console            CONSOLE
#else
// for compiling, because has no uart.h
static ER uart_putChar(CHAR cData)
{
	return 0;
}
static ER uart_putString(CHAR *pString)
{
	return 0;
}
static ER uart_getChar(CHAR *pcData)
{
	return 0;
}
static void uart_abortGetChar(void)
{

}
// for compiling, because has no console.h
typedef struct _CONSOLE {
	void (*hook)(void); ///< start this object
	void (*unhook)(void); ///< terminal this object
	void (*puts)(const char *s); ///< console input funciton pointer
	int (*gets)(char *s, int len); ///< console output function point
} CONSOLE;
#define console CONSOLE
static int console_set_curr(CONSOLE *p_console)
{
	// todo: bridge between kernel space and user space
	return 0;
}
#endif

static char* LogFile_SkipHeadCtrlChar(const char *s)
{
	char *in = (char*)s;
	if (strncmp(s,"\033[0;3",5) == 0 || strncmp(s,"\033[1;3",5) == 0){
		in+= 6;
		if (*in == 'm'){
			return in+1;
		}
	}
	return (char*)s;
}

static UINT32 LogFile_SkipTailCtrlCharLen(const char *s)
{
	char   *in = (char*)s;
	UINT32 len = 0;
	while (*in != '\0'){
		in++;
		len++;
	}
	if (len >= 4 && strncmp(in-4,"\033[0m",4) == 0){
		return len-4;
	}
	return len;
}

static BOOL LogFile_IsKernelDump(const char *s)
{
	// check if "Sem["
	if (s[0] == 'S' && s[1] == 'e' && s[2] == 'm' && s[3] == '[')
		return TRUE;
	// check if "Flag["
	else if (s[0] == 'F' && s[1] == 'l' && s[3] == 'g' && s[4] == '[')
		return TRUE;
	// check if "Waiting Task Queue:"
	else if (s[0] == 'W' && s[1] == 'a' && s[17] == 'e' && s[18] == ':')
		return TRUE;
	// check if "Status:"
	else if (s[0] == 'S' && s[1] == 't' &&  s[2] == 'a' && s[3] == 't' && s[6] == ':')
		return TRUE;
	// check if "\r\n"
	else if (s[0] == '\r' && s[1] == '\n')
		return TRUE;
	return FALSE;
}

static void LogFile_GenTimeStr(LOGFILE_CTRL *pCtrl, CHAR* timeStr, UINT32 buflen)
{
	// DateTime
	if (pCtrl->TimeType == LOGFILE_TIME_TYPE_DATETIME){
		struct tm     CurDateTime;

		CurDateTime = hwclock_get_time(TIME_ID_CURRENT);
		snprintf(timeStr, buflen, "[%04d/%02d/%02d %02d:%02d:%02d]", CurDateTime.tm_year, CurDateTime.tm_mon, CurDateTime.tm_mday, CurDateTime.tm_hour, CurDateTime.tm_min, CurDateTime.tm_sec);
	}
	// HW timer counter
	else {
		if (pCtrl->isLongCounterReady == TRUE) {
			UINT64 counter;
			counter = hwclock_get_longcounter();
			snprintf(timeStr, buflen, "[%5ld.%06ld]", (unsigned long)((counter>>32) & 0xFFFFFFFF),(unsigned long)(counter & 0xFFFFFFFF));
		}
		else {
			UINT32 counter;
			counter = hwclock_get_counter();
			// Suggest after power on 20sec , long counter is ready
			//if (counter > 20000000)
			if (hwclock_is_longcounter_ready() == TRUE) {
				pCtrl->isLongCounterReady = TRUE;
			}
			snprintf(timeStr, buflen, "[%5ld.%06ld]", (unsigned long)counter/1000000,(unsigned long)counter%1000000);
		}
	}
}

static void LogFile_SaveStr(const char* s)
{
#if (__DBGLVL__ > DBGLVL_NORMAL)
	CHAR          tempbuf[90];
#endif
	CHAR          timeStr[40];
	UINT32        timeLen = 0, strLen;
	UINT32        DataIn, DataOut;
	LOGFILE_CTRL *pCtrl = LogFile_GetCtrl();
	LOGFILE_RW_CTRL *p_rw_ctrl = &pCtrl->rw_ctrl[LOG_CORE1];
	char         *instr;
	UINT32        instrLen, remainLen, DataSize;
	BOOL          isAddTimeStrNext = FALSE;
	BOOL          isStore = FALSE;
	LOGFILE_RINGBUF_HEAD *pbuf;

	if (p_rw_ctrl->hasFileErr) {
#if (__DBGLVL__ > DBGLVL_NORMAL)
		uart_putString("LogFile_SaveStr hasFileErr\r\n");
#endif
		return;
	}
	if (pCtrl->state == LOGFILE_STATE_CLOSING) {
		return;
	}
	pbuf = p_rw_ctrl->pbuf;
	DataSize = LogFile_GetDataSize(pbuf);
	// suspend case , and buffer is near full, need to break
	if (pCtrl->state != LOGFILE_STATE_NORMAL && (DataSize >
		pbuf->BufferSize * 3 / 4)) {
		return;
	}
	if (pCtrl->ConType & LOGFILE_CON_STORE){
		isStore = TRUE;
	}
	instr = LogFile_SkipHeadCtrlChar(s);
	instrLen = LogFile_SkipTailCtrlCharLen(instr);
	strLen = instrLen;
	if (pCtrl->TimeType != LOGFILE_TIME_TYPE_NONE && instr[instrLen-1] == '\n') {
		isAddTimeStrNext = TRUE;
	}
	if (pCtrl->isAddTimeStr && LogFile_IsKernelDump(instr) == FALSE){
		LogFile_GenTimeStr(pCtrl,timeStr,sizeof(timeStr));
		timeLen = strlen(timeStr);
		strLen  = (UINT32)instrLen+timeLen;
	}
	DataIn = pbuf->DataIn;
	DataOut = pbuf->DataOut;
	if (isStore && DataIn < DataOut && ((DataIn + strLen) > DataOut)) {
		p_rw_ctrl->hasOverflow = TRUE;
#if (__DBGLVL__ > DBGLVL_NORMAL)
		snprintf(tempbuf, sizeof(tempbuf), "\033[0;35mLogFile_SaveStr overflow I=0x%lx,O=0x%lx,Dsize=0x%lx,len=0x%lx\033[0m\r\n", (unsigned long)p_rw_ctrl->pbuf->DataIn, (unsigned long)p_rw_ctrl->pbuf->DataOut, (unsigned long)DataSize, (unsigned long)strLen);
		uart_putString(tempbuf);
#endif
	}
	if (DataIn + strLen < pbuf->BufferSize) {
		if (pCtrl->isAddTimeStr == TRUE) {
			memcpy((void *)(DataIn + pbuf->BufferStartAddr), timeStr, timeLen);
			memcpy((void *)(DataIn + pbuf->BufferStartAddr+ timeLen), instr, strLen - timeLen);
		} else {
			memcpy((void *)(DataIn + pbuf->BufferStartAddr), instr, strLen);
		}
		DataIn += strLen;
	} else {
		remainLen = pbuf->BufferSize - DataIn;
		if (pCtrl->isAddTimeStr == TRUE) {
			if (remainLen >= timeLen) {
				memcpy((void *)(DataIn + pbuf->BufferStartAddr), timeStr, timeLen);
				DataIn += timeLen;
				remainLen -= timeLen;
				memcpy((void *)(DataIn + pbuf->BufferStartAddr), instr, remainLen);
				// rollback
				DataIn = 0;
				memcpy((void *)(DataIn + pbuf->BufferStartAddr), instr+remainLen, instrLen - remainLen);
				DataIn += (instrLen - remainLen);
			} else {
				memcpy((void *)(DataIn + pbuf->BufferStartAddr), timeStr, remainLen);
				// rollback
				DataIn = 0;
				memcpy((void *)(DataIn + pbuf->BufferStartAddr), timeStr+remainLen, timeLen - remainLen);
				DataIn += (timeLen - remainLen);
				memcpy((void *)(DataIn + pbuf->BufferStartAddr + timeLen), instr, strLen - timeLen);
				DataIn += instrLen;
			}
		} else {
			memcpy((void *)(DataIn + pbuf->BufferStartAddr), instr, remainLen);
			// rollback
			DataIn = 0;
			memcpy((void *)(DataIn + pbuf->BufferStartAddr), instr+remainLen, instrLen - remainLen);
			DataIn += (instrLen - remainLen);
		}
		pbuf->flags |= LOG_ROLLBACK;
	}
	pbuf->DataIn = DataIn;
	pCtrl->isAddTimeStr = isAddTimeStrNext;
}

static void LogFile_hook(void)
{
}

static void LogFile_unhook(void)
{
	LOGFILE_CTRL *pCtrl = LogFile_GetCtrl();
	if (pCtrl->ConType == LOGFILE_CON_NONE) {
		return;
	}
	uart_abortGetChar(); //abort: for release waiting inside "gets" immediately
}

static void LogFile_puts(const char *s)
{
	LOGFILE_CTRL *pCtrl = LogFile_GetCtrl();
	#if 0
	{
		UINT32 len;
		//copy string to my buffer
		LogFile_SaveStr(s);
		uart_putHex(*s);
	    uart_putHex(*(s+1));
		uart_putHex(*(s+2));
		uart_putHex(*(s+3));
		uart_putHex(*(s+4));
		uart_putHex(*(s+5));
		uart_putHex(*(s+6));

		len = strlen(s);
		uart_putHex(*(s+len-6));
		uart_putHex(*(s+len-5));
		uart_putHex(*(s+len-4));
		uart_putHex(*(s+len-3));
		uart_putHex(*(s+len-2));
		uart_putHex(*(s+len-1));
		uart_putHex(*(s+len));
		uart_putString((CHAR *)(UINT32)s);
	}
	#endif
	if (pCtrl->ConType & LOGFILE_CON_UART) {
		uart_putString((CHAR *)(UINT32)s);
	}
	if ((pCtrl->ConType & LOGFILE_CON_MEM) || (pCtrl->ConType & LOGFILE_CON_STORE)) {
		//copy string to my buffer
		LogFile_SaveStr(s);
	}
}

static int LogFile_gets(char *s, int len)
{
#if defined (__UITRON) || defined (__ECOS)
	LOGFILE_CTRL *pCtrl = LogFile_GetCtrl();
	UINT32 strLen = (UINT32)len;

	if ((pCtrl->ConType & LOGFILE_CON_UART) == 0) {
		return 0;
	}
	uart_getString(s, &strLen);
	return (int)strLen;
#else
	char ch = 0;
	int n = 0;

	#if 0  //fix disable uart case
	if ((m_LogFile_Ctrl.ConType & LOGFILE_CON_UART) == 0) {
		return 0;
	}
	#endif //fix disable uart case

	while (n < len-1) {
		uart_getChar(&ch);
		if (ch == '\r') {
			// fgets detect '\n' for end char, but terminal only send '\r'
			uart_putChar(ch);
			uart_putChar('\n');
			s[n++] = 0;
			return n;
		} else if (ch == '\b') {
			if (n > 0) {
				n--;
				// process backspace, replace previous char with a space
				uart_putString("\b \b");
			}
		} else {
			uart_putChar(ch);
			s[n++] = ch;
		}
	}

	s[n++] = 0;
	return n;
#endif
}

console CON_FILE = {LogFile_hook, LogFile_unhook, LogFile_puts, LogFile_gets};

void LogFile_SetConsole(void)
{
	// register and set console
#if defined (__UITRON) || defined (__ECOS)
	debug_reg_console(CON_1, &CON_FILE);
	debug_set_console(CON_1);
#else
	//console_get_curr(&pDbgSys->DefaultConsole);
	console_set_curr(&CON_FILE);
#endif
}

