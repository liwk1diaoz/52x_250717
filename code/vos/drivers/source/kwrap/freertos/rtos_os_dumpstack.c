#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kwrap/task.h>
#include <kwrap/spinlock.h>

#include "freertos_ext_kdrv.h"

#define __MODULE__    rtos_dumpstack
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>

#include <FreeRTOS.h>
#include <task.h> //FreeRTOS header file

/*
\033[0m ---- reset to default
\033[0;30m --- dark gray (black)
\033[0;31m --- dark red
\033[0;32m --- dark green
\033[0;33m --- dark yellow
\033[0;34m --- dark blue
\033[0;35m --- dark magenta
\033[0;36m --- dark cyan
\033[0;37m --- dark white (light gray)
\033[1;30m --- gray (dark gray)
\033[1;31m --- red
\033[1;32m --- green
\033[1;33m --- yellow
\033[1;34m --- blue
\033[1;35m --- magenta
\033[1;36m --- cyan
\033[1;37m --- white
*/

typedef int  KER_PRINT_FUNC(char *fmtstr, ...);

typedef struct {
	UINT32           frame_index;
	UINT32           frame_begin;
	UINT32           frame_end;
} STACK_FRAME_INFO;

#define OS_LogSaveStr   vk_print_isr
#define STACK_CHECK_TAG 0xA5A5A5A5

typedef void TCB_t;
extern volatile TCB_t * volatile pxCurrentTCB;
extern char _section_zi_addr[];
extern unsigned char _stack_end[];
extern unsigned char _stack[];


/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_dumpstack_debug_level = NVT_DBG_WRN;
#if 0
#define PUSH_TYPE_WITH_FP_ONLY    0
#define PUSH_TYPE_WITH_LR_ONLY    1
#define PUSH_TYPE_WITH_FP_AND_LR  2

static UINT32 get_push_type(UINT32 *lr)
{
	//if (((*lr) & 0xFFFFFF00) == 0xe92d4f00) {
		return PUSH_TYPE_WITH_FP_AND_LR;
	//}
	//return PUSH_TYPE_WITH_LR_ONLY;
}
#endif

//static void vos_print_frame_detail_p(UINT32 frame_index, UINT32 frame_begin, UINT32 frame_end, UINT32 stack_end, UINT32 pc ,KER_PRINT_FUNC  print_func)
static void vos_print_frame_detail_p(STACK_FRAME_INFO *p_stack_frame, UINT32 stack_end, UINT32 pc, KER_PRINT_FUNC  print_func)
{
	UINT32 index;
	UINT32 frame_begin = p_stack_frame->frame_begin;
	UINT32 frame_end = p_stack_frame->frame_end;


	if (frame_end + 4 >= stack_end) {
		frame_end = stack_end;
	}

	print_func("  %2ld frame(0x%08lx - 0x%08lx) ............................ $pc : 0x%08lx\r\n",
			   p_stack_frame->frame_index, frame_begin, frame_end, pc);
	{
		UINT32 *stack_word = (UINT32 *)(((UINT32)frame_begin) & 0xfffffff0);
		index = 0;
		while ((UINT32)stack_word < (frame_end)) {
			if ((index % 4) == 0) {
				print_func("%4s + 0x%08lx : ", "", (stack_word));
			}
			if ((UINT32)stack_word < frame_begin) {
				print_func("           ");
			} else {
				print_func("0x%08x ", *(stack_word));
			}
			if ((index % 4) == 3) {
				print_func("\r\n");
			} else if ((UINT32)(stack_word + 1) == (frame_end)) {
				print_func("\r\n");
			}
			index++;
			stack_word++;
		}
	}
}

static void vos_print_backtrace_p(UINT32 epc, UINT32 sp, UINT32 topfp, UINT32 h_stack_base, UINT32 h_stack_size, BOOL print_detail, UINT32 max_frame, KER_PRINT_FUNC  print_func)
{
	UINT32 cur_frame = 0;
	UINT32 curfp = topfp;
	UINT32 fp;
	UINT32 lr;
	UINT32 stack_end = h_stack_base + h_stack_size;
	BOOL   trace_until_stack_end = 0;
	STACK_FRAME_INFO    stackframe;
	UINT32 sys_stack_base;
	UINT32 sys_stack_end;

	sys_stack_base = (UINT32)_stack_end;
	sys_stack_end  = (UINT32)_stack;
	if (!print_detail) {
		print_func("0x%08lx\r\n", epc);
	} else {
		if ((sp < h_stack_base || sp > stack_end) && (sp < sys_stack_base || sp > sys_stack_end) ) {
			print_func("\033[1;31msp(0x%x) out of stack (0x%08lx~0x%08lx)\x1B[0m\r\n", sp, h_stack_base, stack_end);
		} else {
			stackframe.frame_index = cur_frame;
			stackframe.frame_begin = sp;
			stackframe.frame_end = topfp;
			vos_print_frame_detail_p(&stackframe, stack_end, epc, print_func);
		}
	}
	cur_frame = 1;
	while (curfp && cur_frame < max_frame) {

		#if 1
		if ((curfp < h_stack_base || curfp > stack_end) && (curfp < sys_stack_base || curfp > sys_stack_end)) {
			print_func("\033[1;31mfp(0x%x) out of stack(0x%08lx~0x%08lx)\x1B[0m\r\n", curfp, h_stack_base, stack_end);
			break;
		}
		if (curfp & 0x3) {
			print_func("\033[1;31mInvalid fp(0x%x)\x1B[0m\r\n", curfp);
			//debug_dumpmem(curfp-0x200,0x300);
			break;
		}
		#else
		if (curfp < h_stack_base || curfp > stack_end) {
			print_func("\033[1;31mfp(0x%x) out of stack(0x%08lx~0x%08lx)\x1B[0m\r\n", curfp, h_stack_base, stack_end);
			break;
		}
		#endif
		fp = *(((UINT32 *)curfp) - 1);
		lr = *(((UINT32 *)curfp));
		#if 0
		push_type = get_push_type((UINT32 *)lr);
		//DBG_DUMP("*lr = 0x%x, push_type = %d\r\n", (int)(*(UINT32 *)lr), push_type);
		if (PUSH_TYPE_WITH_FP_ONLY == push_type) {
			fp = *(((UINT32 *)curfp));
			continue;
		} else if (PUSH_TYPE_WITH_FP_AND_LR == push_type) {
			fp = *(((UINT32 *)curfp) - 1);
			lr = *(((UINT32 *)curfp));
		} else if (PUSH_TYPE_WITH_LR_ONLY == push_type) {
			lr = *(((UINT32 *)curfp));
			fp = 0;
		}
		#endif
		// end of stack
		if (fp == 0 || fp == 0x11111111) {
			break;
		}
		//print_func("fp = 0x%08lx, lr  = 0x%08lx\r\n", fp, lr );
		if (!print_detail) {
			print_func("0x%08lx\r\n", (int)lr);
		}
		if (print_detail) {
			stackframe.frame_index = cur_frame;
			stackframe.frame_begin = curfp;
			stackframe.frame_end = fp;
			vos_print_frame_detail_p(&stackframe, stack_end, lr, print_func);
		}
		curfp = fp;
		cur_frame++;
	}
	if (curfp + 4 >= stack_end) {
		trace_until_stack_end = 1;
	}
	if (trace_until_stack_end) {
		if (print_detail) {
			print_func("  end\r\n");
		} else {
			print_func("end\r\n");
		}
	}
}
void vos_dump_stack_backtrace(UINT32 *info, UINT32 h_stack_base, INT32 h_stack_size, UINT32 level)
{
	UINT32     *pc, *ra, *sp, *fp;
	BOOL       print_detail;
	UINT32     max_frame = 20;
	KER_PRINT_FUNC  *print_func;

	//#NT#2019/07/24#Nestor Yang -begin
	//#NT# The same code (Warning by Coverity)
	//if (level >= 2) {
		print_func = (KER_PRINT_FUNC *)vk_printk;
	//} else {
	//	print_func = (KER_PRINT_FUNC *)vk_printk; //should log to file ?
	//}
	//#NT#2019/07/24#Nestor Yang -end

	if (level >= 2) {
		print_func("stack      : \r\n");
		print_func("    range(0x%08lx - 0x%08lx)\r\n",
				   (void *)h_stack_base,
				   (void *)(h_stack_base + h_stack_size));
	}
	/* scan code and dump call stack */
	//print_func("call stack :\r\n");
	print_func("\033[1;33mcall stack :\r\n");
	sp = (UINT32 *)(*(info + 13)); //current sp
	//pc = (UINT32 *)(*(info + 15)); //current epc
	fp = (UINT32 *)(*(info + 11)); //current fp
	ra = (UINT32 *)(*(info + 14)); //current ra
	//DBG_DUMP("sp = 0x%08x, pc = 0x%08x, fp = 0x%08x\r\n", (int)sp, (int)pc, (int)fp);
	pc = ra;
	print_detail = 0;
	// print stack backtrace
	vos_print_backtrace_p((UINT32)pc, (UINT32)sp, (UINT32)fp, h_stack_base, h_stack_size, print_detail, max_frame, print_func);
	print_func("\x1B[0m");
	// print frame detail
	print_detail = 1;
	if (level >= 2) {
		vos_print_backtrace_p((UINT32)pc, (UINT32)sp, (UINT32)fp, h_stack_base, h_stack_size, print_detail, max_frame, print_func);
	} else if (level >= 1) {
		max_frame = 3;
		vos_print_backtrace_p((UINT32)pc, (UINT32)sp, (UINT32)fp, h_stack_base, h_stack_size, print_detail, max_frame, print_func);
	}

}

void vos_dump_stack_content(UINT32 h_stack_pointer, UINT32 h_stack_base, INT32 h_stack_size, UINT32 level)
{
	UINT32          StackSize = (UINT32)h_stack_size;
	UINT32          *pStackTop = (UINT32 *)h_stack_base;
	UINT32          *pStackBottom = (UINT32 *)(h_stack_base + h_stack_size - sizeof(UINT32));
	UINT32          *pStackPointer = (UINT32 *)h_stack_pointer;
	UINT32 *pMem;
	UINT32          k, l, m;
	BOOL   bBreakForLoop;
	UINT32   uiCodeBase = 0;
	UINT32   uiCodeLimit = 0;


	uiCodeBase =  0; //first section start
	uiCodeLimit = (UINT32)_section_zi_addr; //last section end

	DBG_DUMP("Stack Content:\r\n");
	if ((level == 0) && (StackSize > 16)) {
		//dump from stack top until 16 bytes
		pMem = pStackTop;
		bBreakForLoop   = FALSE;

		for (l = 0, m = 0; (l < 1) && (bBreakForLoop == FALSE) ; l++) {
			DBG_DUMP("0x%.8X: ", (int)pMem);

			for (k = 0; k < 4; k++, pMem++) {
				if ((UINT32)pStackPointer == (UINT32)pMem) {
					DBG_DUMP("\033[1;32m[0x%.8X] \x1B[0m", (int)*pMem);
				} else if ((UINT32)pStackTop == (UINT32)pMem) {
					DBG_DUMP("\033[1;35m[0x%.8X> \x1B[0m", (int)*pMem);
				} else if ((UINT32)pStackBottom == (UINT32)pMem) {
					DBG_DUMP("\033[1;36m<0x%.8X] \x1B[0m", (int)*pMem);
					bBreakForLoop = TRUE;
				} else {
					DBG_DUMP("[0x%.8X] ", (int)*pMem);
				}
			}
			DBG_DUMP("\r\n");
		}
		DBG_DUMP("             ..........   ..........   ..........   .......... \r\n");
	}

	if (level == 0) {
		// Dump from stack pointer to stack bottom
		pMem = (UINT32 *)ALIGN_FLOOR_16(((UINT32)pStackPointer)); //dump from 16 bytes align
	} else { //level==1
		// Dump from stack top to stack bottom
		pMem = pStackTop; //dump from 16 bytes align
	}
	bBreakForLoop   = FALSE;

	for (l = 0, m = 0; (l < 1024) && (bBreakForLoop == FALSE) ; l++) {
		DBG_DUMP("0x%.8X: ", (int)pMem);

		for (k = 0; k < 4; k++, pMem++, m++) {
			if ((UINT32)pStackPointer == (UINT32)pMem) {
				DBG_DUMP("\033[1;32m[0x%.8X] \x1B[0m", (int)*pMem);
			} else if ((UINT32)pStackTop == (UINT32)pMem) {
				DBG_DUMP("\033[1;35m[0x%.8X> \x1B[0m", (int)*pMem);
			} else if ((UINT32)pStackBottom == (UINT32)pMem) {
				DBG_DUMP("\033[1;36m<0x%.8X] \x1B[0m", (int)*pMem);
				bBreakForLoop = TRUE;
			}
			else {
				if (
					((*pMem) >= ((UINT32)uiCodeBase))
					&& ((*pMem) < ((UINT32)uiCodeLimit))
				) {
					DBG_DUMP("\033[0;33m<0x%.8X> \x1B[0m", (int)*pMem);
				} else {
					DBG_DUMP("[0x%.8X] ", (int)*pMem);
				}
			}
		}
		DBG_DUMP("\r\n");
	}
}

static int vos_stack_get_percent(UINT32 numerator, UINT32 denominator)
{
	int ret;

	if (0 == denominator) {
		return -1; //invalid denominator
	}

	ret = (int)( (100 * numerator + denominator - 1) / denominator );

	return ret;
}

void vos_show_stack(VK_TASK_HANDLE takhdl, UINT32 level)
{
	UINT32          StackSize;
	UINT32          StackUsed;
	UINT32          StackMaxUsed;
	UINT32          *pStackTop;
	UINT32          *pStackBottom;
	UINT32          *pStackPointer;
	UINT32          backtrace_level = 0;
	TaskDetailStatus_t   taskinfo, *pTask;
	eTaskState      tskstate;
	UINT32          fp, lr;
	UINT32          xOS_tskctx[16] = {0};
	UINT32          fpu_offset;
	UINT32          sys_stack_base;
	UINT32          sys_stack_end;

	sys_stack_base = (UINT32)_stack_end;
	sys_stack_end  = (UINT32)_stack;
#if (configUSE_TASK_FPU_SUPPORT == 2)
	fpu_offset = 65; //FreeRTOS: portFPU_REGISTER_WORDS
#else
	fpu_offset = 0;
#endif

	pTask = &taskinfo;
	if (NULL == takhdl) {
		takhdl = (VK_TASK_HANDLE)pxCurrentTCB;
	}
	tskstate = eTaskGetState(takhdl);
	vTaskGetDetailInfo( (TaskHandle_t)takhdl, &taskinfo, pdTRUE, tskstate );
	pStackTop = (UINT32 *)(pTask->pxStackBase);
	pStackBottom = (UINT32 *)(pTask->pxEndOfStack);
	pStackPointer = (UINT32 *)(pTask->pxTopOfStack);
	StackSize = (UINT32)pStackBottom - (UINT32)pStackTop;
	StackMaxUsed = StackSize - (((UINT32)pTask->usStackHighWaterMark) * sizeof(StackType_t));
	OS_LogSaveStr("pStackTop = 0x%x, pStackBottom = 0x%x, pStackPointer= 0x%x, StackSize = 0x%x\r\n", (int)pStackTop, (int)pStackBottom, (int)pStackPointer, (int)StackSize);
	if (pTask->pxStackBase != 0) {
		StackUsed = (UINT32)pStackBottom - (UINT32)pStackPointer;
	} else {
		StackUsed = 0;
	}

	// Display stack information
	OS_LogSaveStr("Stack Area: ");
	OS_LogSaveStr("\033[1;35m[0x%.8X>\x1B[0m", (int)pStackTop);
	OS_LogSaveStr(" ~ ");
	OS_LogSaveStr("\033[1;36m<0x%.8X]\x1B[0m", (int)pStackBottom);
	OS_LogSaveStr(", Pointer 0x%.8X\r\n", (int)pStackPointer);
	OS_LogSaveStr("Stack Size: 0x%08X, Curr Used: 0x%08X, Max Used: 0x%08X (%d%%)\r\n",
				  (int)StackSize, (int)StackUsed, (int)StackMaxUsed, vos_stack_get_percent(StackMaxUsed, StackSize));

	if (level == 0) {
		return;
	}
	// Check if not start or not exist
	if (tskstate >= eSuspended) {
		OS_LogSaveStr("(Ignore)\r\n");
		return;
	}

	// Check if stack range is valid
	if ((StackSize == 0)
		|| (StackSize > 0x00100000) //1MB
		|| ((UINT32)pStackTop == 0)
		|| ((UINT32)pStackBottom == 0)) {
		OS_LogSaveStr("\033[1;31m###Invalid stack range!###\x1B[0m\r\n");
		return;
	}
	if (((UINT32)pStackPointer == 0)
		|| (((UINT32)pStackPointer < (UINT32)pStackTop || ((UINT32)pStackPointer > (UINT32)pStackBottom))
		&& ((UINT32)pStackPointer < (UINT32)sys_stack_base || ((UINT32)pStackPointer > (UINT32)sys_stack_end)))){
		OS_LogSaveStr("\033[1;31m###Invalid stack point!###\x1B[0m\r\n");
		return;
	}
	#if 0
	// Check if stack is overflow
	if ((pStackTop[0] != STACK_CHECK_TAG) ||
		((UINT32)pStackPointer < (UINT32)pStackTop)) {
		OS_LogSaveStr("^R###Stack overflow!###\r\n");
	}
	#endif

	if (level == 2) {
		vos_dump_stack_content((UINT32)pStackPointer, (UINT32)pStackTop, (UINT32)StackSize, 1);
		return;
	}
	// check if this is running task
	if (takhdl == (VK_TASK_HANDLE)pxCurrentTCB) {
		__asm__ __volatile__("mov %0, r14\n\t" : "=r"(lr));  //get $ra
		__asm__ __volatile__("mov %0, r11\n\t" : "=r"(fp));  //get $fp
	} else {
		fp = pStackPointer[13 + fpu_offset];
		lr = pStackPointer[16 + fpu_offset];
	}
	xOS_tskctx[11] = fp;
	if (tskstate == eRunning) {
		xOS_tskctx[14] = lr;
		xOS_tskctx[15] = lr;
	} else {
		xOS_tskctx[14] = lr;
		xOS_tskctx[15] = lr;
		backtrace_level = 0;
	}
	xOS_tskctx[13] = (UINT32)(pStackPointer + 0 + fpu_offset);
	OS_LogSaveStr("registers : $pc  - 0x%08x  $ra  - 0x%08x  $sp  - 0x%08x  $fp  - 0x%08x\r\n",
				  (int)xOS_tskctx[15], (int)xOS_tskctx[14], (int)xOS_tskctx[13], (int)xOS_tskctx[11]);
	vos_dump_stack_backtrace(xOS_tskctx, (UINT32)pStackTop, (UINT32)StackSize, backtrace_level);
}

void vos_dump_stack(void)
{
	vos_show_stack(NULL, 1);
}
