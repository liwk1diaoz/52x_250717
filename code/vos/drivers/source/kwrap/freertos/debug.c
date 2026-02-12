/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <kwrap/type.h>

#define __MODULE__    rtos_debug
#define __DBGLVL__    2
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/file.h>

#include "freertos_ext_kdrv.h"

#include <FreeRTOS.h>
#include <task.h> //FreeRTOS header file

void vos_debug_halt(void)
{
	debug_msg_isr("\r\nSystem halted\r\n");
	taskDISABLE_INTERRUPTS();
	vTaskSuspendAll();
	// coverity[no_escape]
	while(1);
}

int debug_msg_isr(const char *fmtstr, ...)
{
	CHAR strbuf[256] = {0};
	va_list args;
	int size = 0;

	va_start(args, fmtstr);
	size = vsnprintf(strbuf, sizeof(strbuf), fmtstr, args);
	va_end(args);

	if (size < 0) {
		char err[] = "debug_msg error\n";
		fwrite(err, 1, strlen(err)+1, stdout);
		//uart_putString("debug_msg error\r\n");
		return -1;
	}
	fwrite(strbuf, 1, strlen(strbuf)+1, stdout);
	//uart_putString(strbuf);
	return 0;
}

void debug_dumpmem(ULONG addr, ULONG length)
{
	ULONG   offs;
	UINT32  str_len;
	UINT32  cnt;
	CHAR    str_dumpmem[128];
	UINT32  u32array[4];
	volatile UINT32  *p_u32;
	CHAR    *p_char;

	addr = ALIGN_FLOOR_4(addr); //align to 4 bytes (UINT32)

	p_u32 = (volatile UINT32 *)addr;

	DBG_DUMP("dump va=%08lx, addr=%08lx length=%08lx to console:\r\n", (ULONG)p_u32, addr, length);

	for (offs = 0; offs < length; offs += sizeof(u32array)) {
		u32array[0] = *p_u32++;
		u32array[1] = *p_u32++;
		u32array[2] = *p_u32++;
		u32array[3] = *p_u32++;

		str_len = snprintf(str_dumpmem, sizeof(str_dumpmem), "%08lX : %08X %08X %08X %08X  ",
			(addr + offs), (UINT)u32array[0], (UINT)u32array[1], (UINT)u32array[2], (UINT)u32array[3]);

		p_char = (char *)&u32array[0];
		for (cnt = 0; cnt < sizeof(u32array); cnt++, p_char++) {
			if (*p_char < 0x20 || *p_char >= 0x80)
				str_len += snprintf(str_dumpmem+str_len, sizeof(str_dumpmem)-str_len, ".");
			else
				str_len += snprintf(str_dumpmem+str_len, sizeof(str_dumpmem)-str_len, "%c", *p_char);
		}

		DBG_DUMP("%s\r\n", str_dumpmem);
	}
	DBG_DUMP("\r\n\r\n");
}
void debug_dumpmem2file(ULONG addr, ULONG length, char *filename)
{
	int write_size;
	VOS_FILE fd;

	DBG_DUMP("dump addr=0x%08lx length=0x%08lx to file %s:\r\n", addr, length, filename);

	fd = vos_file_open(filename, O_CREAT|O_WRONLY|O_SYNC, 0);
	if ((VOS_FILE)(-1) == fd) {
		DBG_DUMP("open %s failed\r\n", filename);
		return;
	}

	write_size = vos_file_write(fd, (void *)addr, length);

	if (-1 == vos_file_close(fd)) {
		DBG_DUMP("close %s failed\r\n", filename);
	}

	if (write_size <= 0) {
		DBG_DUMP("dump file fail\r\n");
	} else {
		DBG_DUMP("dump file success\r\n");
	}
}

