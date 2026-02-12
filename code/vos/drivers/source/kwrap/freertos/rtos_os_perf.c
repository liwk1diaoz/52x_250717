/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>

#define __MODULE__    rtos_perf
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/cpu.h>
#include <kwrap/perf.h>
#include <kwrap/spinlock.h>

#include <FreeRTOS.h>
#include <task.h> //FreeRTOS header file

#include "freertos_ext_kdrv.h"

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define RTOS_PERF_INITED_TAG       MAKEFOURCC('R', 'P', 'E', 'R') ///< a key value


#define VOS_PERF_LIST_NUM       32
#define VOS_PERF_LIST_NAME_SIZE 32
#define VOS_GET_TM0_CNT()       (VOS_TICK)hwclock_get_counter()

#define VOS_STRCPY(dst, src, dst_size) do { \
	strncpy(dst, src, (dst_size)-1); \
	dst[(dst_size)-1] = '\0'; \
} while(0)

typedef struct _VOS_PERF_LIST_ITEM {
    CHAR name[VOS_PERF_LIST_NAME_SIZE];
    VOS_TICK tick;
    UINT32 line_no;
    UINT32 cus_val;
    UINT32 diff_time; //diff time to the first same name
    INT column;
} VOS_PERF_LIST_ITEM;

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_perf_debug_level = NVT_DBG_WRN;
static VK_DEFINE_SPINLOCK(g_perf_lock);

static VOS_PERF_LIST_ITEM g_perf_list[VOS_PERF_LIST_NUM] = {0};
static UINT32 g_perf_list_idx = 0;
static UINT32 g_perf_list_skip_count = 0;
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void vos_perf_init(void *param)
{
}

void vos_perf_exit(void)
{
}

void vos_perf_mark(VOS_TICK *p_tick)
{
	if (NULL == p_tick) {
		DBG_ERR("p_tick is NULL\r\n");
		return;
	}

	*p_tick = VOS_GET_TM0_CNT();
}

VOS_TICK vos_perf_duration(VOS_TICK t_begin, VOS_TICK t_end)
{
	return (t_end - t_begin);
}

void vos_perf_list_reset(void)
{
	unsigned long flags;

	vk_spin_lock_irqsave(&g_perf_lock, flags);

	g_perf_list_idx = 0;
	g_perf_list_skip_count = 0;

	vk_spin_unlock_irqrestore(&g_perf_lock, flags);
}

void vos_perf_list_mark(const CHAR *p_name, UINT32 line_no, UINT32 cus_val)
{
	unsigned long flags;

	vk_spin_lock_irqsave(&g_perf_lock, flags);

	if (g_perf_list_idx < VOS_PERF_LIST_NUM) {
		VOS_STRCPY(g_perf_list[g_perf_list_idx].name, p_name, sizeof(g_perf_list[g_perf_list_idx].name));
		g_perf_list[g_perf_list_idx].tick = VOS_GET_TM0_CNT();
		g_perf_list[g_perf_list_idx].line_no = line_no;
		g_perf_list[g_perf_list_idx].cus_val = cus_val;
		g_perf_list_idx++;
	} else {
		g_perf_list_skip_count++;
	}

	vk_spin_unlock_irqrestore(&g_perf_lock, flags);
}

static void vos_perf_list_dump_by_idx(void)
{
	INT list_idx;
	INT max_idx;

	DBG_DUMP("\r\n===== %s =====\r\n", __func__);

	max_idx = g_perf_list_idx;

	for(list_idx = 0; list_idx < max_idx; list_idx++) {
		DBG_DUMP("[%02d] %s() ln %d ts %d df %d cus %d\r\n",
			list_idx,
			g_perf_list[list_idx].name,
			(UINT)g_perf_list[list_idx].line_no,
			(UINT)g_perf_list[list_idx].tick,
			(list_idx < 1) ? 0 : (UINT)vos_perf_duration(g_perf_list[list_idx-1].tick, g_perf_list[list_idx].tick),
			(UINT)g_perf_list[list_idx].cus_val);
    }
}

static void vos_perf_list_dump_by_name(void)
{
	INT item_idx, item_idx2;
	INT column_cur, column_max;
	INT row_idx;
	INT max_idx;

	DBG_DUMP("\r\n===== %s =====\r\n", __func__);

	max_idx = g_perf_list_idx;

	//reset the column to recount the newest column
	for (item_idx = 0; item_idx < max_idx; item_idx++) {
		g_perf_list[item_idx].column = 0;
	}

	//setup the column of each item
	column_cur = 0;
	for (item_idx = 0; item_idx < max_idx; item_idx++) {
		if (0 != g_perf_list[item_idx].column) {
			continue;
		}
		column_cur++;
		g_perf_list[item_idx].column = column_cur;
		g_perf_list[item_idx].diff_time = 0;

		for (item_idx2 = item_idx + 1; item_idx2 < max_idx; item_idx2++) {
			if (!strcmp(g_perf_list[item_idx2].name, g_perf_list[item_idx].name)) {
				g_perf_list[item_idx2].column = column_cur;
				g_perf_list[item_idx2].diff_time = g_perf_list[item_idx2].tick - g_perf_list[item_idx].tick;
			}
		}
	}
	column_max = column_cur;

#if 0 //for debug
	DBG_DUMP("max_idx %d, column_max %d\r\n", max_idx, column_max);
	for (item_idx = 0; item_idx < max_idx; item_idx++) {
		DBG_DUMP("item[%d], column %d, df %u, %u, %s\r\n",
			item_idx,
			g_perf_list[item_idx].column,
			g_perf_list[item_idx].tick,
			g_perf_list[item_idx].diff_time,
			g_perf_list[item_idx].name);
	}
#endif

	//print column name
	DBG_DUMP("Name");
	for (column_cur = 1; column_cur <= column_max; column_cur++) {
		for (item_idx = 0; item_idx < max_idx; item_idx++) {
			if (column_cur == g_perf_list[item_idx].column) {
				DBG_DUMP(" %10s", g_perf_list[item_idx].name);
				break;
			}
		}
	}
	DBG_DUMP("\n");

	//print item data by column
	for (row_idx = 0; row_idx < max_idx; row_idx++) {
		DBG_DUMP("[%02d]", row_idx);
		for (column_cur = 1; column_cur <= column_max; column_cur++) {
			if (column_cur == g_perf_list[row_idx].column) {
				DBG_DUMP(" %10u", g_perf_list[row_idx].tick);
			} else {
				DBG_DUMP(" %10c", '-');
			}
		}
		DBG_DUMP("\n");
	}

	//print diff time
	DBG_DUMP("-----------------------------------------------\n");
	DBG_DUMP("Diff");
	for (column_cur = 1; column_cur <= column_max; column_cur++) {
		for (item_idx = (max_idx-1); item_idx >= 0; item_idx--) {
			if (column_cur == g_perf_list[item_idx].column) {
				DBG_DUMP(" %10u", g_perf_list[item_idx].diff_time);
				break;
			}
		}
	}
	DBG_DUMP("\n");
}

void vos_perf_list_dump(void)
{
	unsigned long flags;

	vk_spin_lock_irqsave(&g_perf_lock, flags);

	vos_perf_list_dump_by_idx();
	vos_perf_list_dump_by_name();

	if (g_perf_list_skip_count) {
		DBG_WRN("skip count %d\r\n", (UINT)g_perf_list_skip_count);
	}

	vk_spin_unlock_irqrestore(&g_perf_lock, flags);
}
