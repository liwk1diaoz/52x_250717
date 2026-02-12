/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kwrap/flag.h>
#include <kwrap/task.h>
#include <kwrap/semaphore.h>
#include <kwrap/spinlock.h>
#include <kwrap/util.h> //for obsolete vos_task_delay_xxx API

#include "freertos_ext_kdrv.h"

#define __MODULE__    rtos_task
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>

#include <FreeRTOS.h>
#include <task.h> //FreeRTOS header file

#define TASK_COMM_LEN  16

#define FREERTOS_MAX_VALID_PRI (configMAX_PRIORITIES - 1) //FreeRTOS max valid priority
#define FREERTOS_MIN_VALID_PRI (tskIDLE_PRIORITY + 1) //FreeRTOS min valid priority

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define RTOS_TASK_INITED_TAG       MAKEFOURCC('R', 'T', 'S', 'K') ///< a key value

#define VOS_STRCPY(dst, src, dst_size) do { \
	strncpy(dst, src, (dst_size)-1); \
	dst[(dst_size)-1] = '\0'; \
} while(0)

#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

#if defined(__LINUX)
#define my_alloc    vmalloc
#define my_free     vfree
#define my_jiffies  jiffies_64
#define my_jiff_to_ms(j) ((j) * (1000 / HZ))

#elif defined(__FREERTOS)
#define my_alloc    malloc
#define my_free     free
#define current     xTaskGetCurrentTaskHandle()
#define my_jiffies  hwclock_get_longcounter()
#define my_jiff_to_ms(j) ((j) / 1000)

#ifndef unlikely
#define unlikely(x) (x)
#endif

#else
#error Not supported OS
#endif

typedef struct {
	UINT                         st_used;
	TaskHandle_t                 task_hdl;
	StaticTask_t                 task_buf;
	UINT32                       wait_res;
	FLGPTN                       waiptn;
	UINT                         wfmode;
	ID                           wairesid;
	UINT64                       wait_time_jiffies;
	int                          vos_prio;
} TASK_CELL_T;


typedef struct {
	TASK_CELL_T            *p_tasks;
} TASK_CTRL_T;

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_task_debug_level = NVT_DBG_WRN;
static  VK_DEFINE_SPINLOCK(my_lock);

static int            gtasknum;
static int            g_cur_task_id = 1;
static TASK_CTRL_T    g_st_task_ctrl;

static char *get_task_state_str(eTaskState state)
{
	switch (state) {
	case eRunning:
		return "eRunning";

	case eReady:
		return "eReady";

	case eBlocked:
		return "eBlocked";

	case eSuspended:
		return "eSuspended";

	case eDeleted:
		return "eDeleted";

	case eInvalid:
		return "eInvalid";

	default:
		return NULL;
	}
}

void rtos_task_init(unsigned long max_task_num)
{
	DBG_FUNC_BEGIN("\r\n");
	DBG_IND("max_task_num =%ld\r\n", max_task_num);

	// initialize value
	gtasknum = max_task_num;
	g_cur_task_id = 1;

	g_st_task_ctrl.p_tasks = (TASK_CELL_T *)my_alloc(sizeof(TASK_CELL_T) * gtasknum);
	if (g_st_task_ctrl.p_tasks == NULL) {
		DBG_ERR(" alloc!\n");
		goto END_INIT;
	}
	DBG_IND("p_tasks =0x%lx\r\n", (ULONG)g_st_task_ctrl.p_tasks);

	memset(g_st_task_ctrl.p_tasks, 0x00, sizeof(TASK_CELL_T) * gtasknum);

END_INIT:
	DBG_FUNC_END("\r\n");
	return;
}

/**
   rtos_exit_flag
 * @remarks
 * @param none
 * @code

 * @endcode
 * @return
 * @see
*/
void rtos_task_exit(void)
{
	my_free(g_st_task_ctrl.p_tasks);
	g_st_task_ctrl.p_tasks = NULL;
}

int vos_task_max_cnt(void)
{
	return gtasknum;
}

int vos_task_used_cnt(void)
{
	TASK_CELL_T *p_tskcell;
	unsigned long flags = 0;
	int idx;
	int used_cnt = 0;

	loc_cpu(flags);

	p_tskcell = g_st_task_ctrl.p_tasks;

	for (idx = 0; idx < gtasknum; idx++) {
		if (p_tskcell->st_used) {
			used_cnt++;
		}
		p_tskcell++;
	}

	unl_cpu(flags);

	return used_cnt;
}

static int vos_task_get_free_id_p(void)
{
	TASK_CELL_T        *p_tskcell;
	int                 cur_id, i;
	unsigned long       flags = 0;

	loc_cpu(flags);
	if (unlikely(g_cur_task_id >= gtasknum)) {
		// reset id
		g_cur_task_id = 1;
	}
	cur_id = g_cur_task_id;
	for (i = 0; i < gtasknum; i++) {
		if (unlikely(cur_id >= gtasknum)) {
			cur_id = 1;
		}
		p_tskcell = g_st_task_ctrl.p_tasks + cur_id;
		if (0 == p_tskcell->st_used) {
			p_tskcell->st_used = RTOS_TASK_INITED_TAG;
			g_cur_task_id = cur_id;
			g_cur_task_id++;
			unl_cpu(flags);
			DBG_IND("g_cur_task_id=%d\r\n", cur_id);
			return cur_id;
		}
		cur_id++;
	}
	unl_cpu(flags);
	return -1;
}

int vos_task_hdl2id(TaskHandle_t task_hdl)
{
	//int                   task_id;
	int                         i;
	TASK_CELL_T        *p_tskcell;

	if (task_hdl == NULL) {
		return -1;
	}
	#if 0
	if (task_hdl->comm[0] >= 0x30 && task_hdl->comm[0] <= 0x39) {
		task_id = (task_hdl->comm[0]-0x30)*10 + (task_hdl->comm[1]-0x30);
		if (task_id > 0 && task_id < gtasknum) {
			p_tskcell = g_st_task_ctrl.p_tasks + task_id;
			if (RTOS_TASK_INITED_TAG == p_tskcell->st_used && task_hdl == p_tskcell->task_hdl) {
				return task_id;
			}
		}
	}
	#endif
	p_tskcell = g_st_task_ctrl.p_tasks + 1;
	for (i = 1; i < gtasknum; i++) {
		if (task_hdl == p_tskcell->task_hdl && RTOS_TASK_INITED_TAG == p_tskcell->st_used) {
			return i;
		}
		p_tskcell++;
	}
	return -1;
}

TaskHandle_t vos_task_id2hdl(int task_id)
{
	TASK_CELL_T        *p_tskcell;

	if (0 == task_id || task_id >= gtasknum) {
		return NULL;
	}
	p_tskcell = g_st_task_ctrl.p_tasks + task_id;
	if (RTOS_TASK_INITED_TAG == p_tskcell->st_used) {
		return p_tskcell->task_hdl;
	}
	return NULL;
}

static TASK_CELL_T *vos_task_hdl2cell_p(TaskHandle_t task_hdl)
{
	//int                 task_id;
	int                       i;
	TASK_CELL_T        *p_tskcell;

	if (task_hdl == NULL) {
		return NULL;
	}

	p_tskcell = g_st_task_ctrl.p_tasks + 1;
	for (i = 1; i < gtasknum; i++) {
		if (task_hdl == p_tskcell->task_hdl && RTOS_TASK_INITED_TAG == p_tskcell->st_used) {
			return p_tskcell;
		}
		p_tskcell++;
	}
	return NULL;
}

static void vos_task_clear_info_p(TASK_CELL_T   *p_tskcell)
{
	if (RTOS_TASK_INITED_TAG == p_tskcell->st_used) {
		p_tskcell->task_hdl = 0;
		p_tskcell->waiptn = 0;
		p_tskcell->wfmode = 0;
		p_tskcell->wairesid = 0;
		p_tskcell->vos_prio = 0;
		p_tskcell->st_used = 0;
	}
}

static BOOL vos_task_chk_valid_p(TASK_CELL_T   *p_tskcell)
{
	char          *task_state_str;

	if (RTOS_TASK_INITED_TAG != p_tskcell->st_used) {
		return FALSE;
	}
	if (0 == p_tskcell->task_hdl) {
		return FALSE;
	}
	task_state_str = get_task_state_str(eTaskGetState(p_tskcell->task_hdl));
	if (NULL == task_state_str) {
		DBG_IND("\r\nInvalid state\r\n");
		return FALSE;
	}
	return TRUE;
}



int vos_task_get_tid(void)
{
	int tid;

	tid = vos_task_hdl2id(xTaskGetCurrentTaskHandle());
	if (tid < 0) {
		DBG_ERR("get tid fail\r\n");
	}

	return tid;
}

static int vos_task_check_priority(int priority)
{
	STATIC_ASSERT(VK_TASK_LOWEST_PRIORITY <= FREERTOS_MAX_VALID_PRI);
	STATIC_ASSERT(VK_TASK_LOWEST_PRIORITY >= FREERTOS_MIN_VALID_PRI);

	STATIC_ASSERT(VK_TASK_HIGHEST_PRIORITY <= FREERTOS_MAX_VALID_PRI);
	STATIC_ASSERT(VK_TASK_HIGHEST_PRIORITY >= FREERTOS_MIN_VALID_PRI);

	//1. Assume the value of VK_TASK_HIGHEST_PRIORITY is smaller, because
	//we want to use configMAX_PRIORITIES to minus the priority value
	//2. A higher priority should get a bigger value in FreeRTOS
	STATIC_ASSERT(VK_TASK_HIGHEST_PRIORITY <= VK_TASK_LOWEST_PRIORITY);

	// check the priority here
	if (priority > VK_TASK_LOWEST_PRIORITY || priority < VK_TASK_HIGHEST_PRIORITY) {
		DBG_ERR("Invalid priority %d, Lowest(%d) ~ Highest(%d)\r\n", priority, VK_TASK_LOWEST_PRIORITY, VK_TASK_HIGHEST_PRIORITY);
		return -1;
	}

	return 0;
}

void vos_task_set_priority(VK_TASK_HANDLE task_hdl, int priority)
{
	TASK_CELL_T        *p_tskcell;

	p_tskcell = vos_task_hdl2cell_p(task_hdl);
	if (NULL == p_tskcell) {
		DBG_WRN("unknown task_hdl 0x%x, skip\r\n", (int)task_hdl);
		return;
	}

	if (0 != vos_task_check_priority(priority)) {
		DBG_ERR("skip, task_hdl 0x%X\r\n", (unsigned int)task_hdl);
		return;
	}

	vTaskPrioritySet(task_hdl, FREERTOS_MAX_VALID_PRI - priority);
	p_tskcell->vos_prio = priority;
}

VK_TASK_HANDLE vos_task_get_handle(void)
{
	return xTaskGetCurrentTaskHandle();
}

int vos_task_get_name(VK_TASK_HANDLE task_hdl, char *name, unsigned int len)
{
	if (NULL == get_task_state_str(eTaskGetState(task_hdl))) {
		return -1;
	}

	if (NULL == name) {
		return -1;
	}

	//check for coverity (strncpy)
	if (NULL == pcTaskGetName(task_hdl)) {
		return -1;
	}

	VOS_STRCPY(name, pcTaskGetName(task_hdl), len);
	return 0;
}

BOOL vos_task_chk_hdl_valid(VK_TASK_HANDLE task_hdl)
{
	DBG_WRN("not supported\r\n");
	return FALSE;
}

TaskHandle_t vos_task_create(void *fp, void *parm, const char name[], int priority, int stksz)
{
	int                 task_id;
	TASK_CELL_T         *p_tskcell;
	UBaseType_t         freertos_pri;
	BaseType_t          ret;

	if (0 != vos_task_check_priority(priority)) {
		DBG_ERR("check priority %d fail\r\n", priority);
		return NULL;
	}
	freertos_pri = (UBaseType_t)FREERTOS_MAX_VALID_PRI - priority;

	task_id = vos_task_get_free_id_p();
	if (task_id < 0) {
		DBG_ERR("exceed max task num %d\r\n", gtasknum);
		return 0;
	}
	p_tskcell = g_st_task_ctrl.p_tasks + task_id;

	if ((unsigned int)stksz < configMINIMAL_STACK_SIZE * sizeof(StackType_t)) {
		DBG_WRN("stksz(%d) < min(%d), set to min\r\n", stksz, configMINIMAL_STACK_SIZE * sizeof(StackType_t));
		stksz = configMINIMAL_STACK_SIZE * sizeof(StackType_t);
	}

	//Note: we do not want a task start running before vos_task_resume
	//if scheduler is running, suspend the scheduler before the creating task is suspended
	vTaskSuspendAll();

	ret = xTaskCreate((TaskFunction_t)fp,
	                  name,
	                  ALIGN_CEIL_4(stksz)/sizeof(StackType_t), /* uint32_t array index counts, not size in bytes */
	                  parm,
	                  freertos_pri,
	                  &(p_tskcell->task_hdl));
	if (ret != pdPASS) {
		p_tskcell->st_used = 0; //release the task id
		DBG_ERR("task_create fail, ret %d\r\n", ret);
		xTaskResumeAll();
		return 0;
	}

	//if created successfully, update the vos_prio
	p_tskcell->vos_prio = priority;

	//suspend the new created task
	if (taskSCHEDULER_NOT_STARTED != xTaskGetSchedulerState()) {
		//Note: Before vTaskStartScheduler is called, a suspended task will cause assert in vTaskSwitchContext
		//So we can not vTaskSuspend a task before vTaskStartScheduler
		vTaskSuspend(p_tskcell->task_hdl);
	}

	xTaskResumeAll();

	return p_tskcell->task_hdl;
}

int vos_task_resume(VK_TASK_HANDLE task_hdl)
{
	TASK_CELL_T        *p_tskcell;

	p_tskcell = vos_task_hdl2cell_p(task_hdl);
	if (NULL == p_tskcell) {
		DBG_ERR("unknown task_hdl 0x%x\r\n", (int)task_hdl);
		return -1;
	}

	//Note: if the scheduler is not started, skip the resume
	//since we also skip the suspend in vos_task_create()
	if (taskSCHEDULER_NOT_STARTED != xTaskGetSchedulerState()) {
		vTaskResume(task_hdl);
	}

	return 0;
}

void vos_task_destroy(TaskHandle_t task_hdl)
{
	TASK_CELL_T        *p_tskcell;

	p_tskcell = vos_task_hdl2cell_p(task_hdl);
	if (NULL == p_tskcell) {
		DBG_WRN("unknown task_hdl 0x%x, skip\r\n", (int)task_hdl);
		return;
	}
	vTaskDelete(task_hdl);
	vos_task_clear_info_p(p_tskcell);
}

void vos_task_enter(void)
{
}

int vos_task_return(int rtn_val)
{
	TASK_CELL_T        *p_tskcell;
	TaskHandle_t       task_hdl;

	task_hdl = current;
	p_tskcell = vos_task_hdl2cell_p(task_hdl);
	if (NULL == p_tskcell) {
		//DBG_ERR("Invalid task_hdl 0x%x\r\n", (int)task_hdl);
		return rtn_val;
	}
	vos_task_clear_info_p(p_tskcell);
	return rtn_val;
}

void vos_task_delay_ms(int ms)
{
	//obsolete API, redirect to vos_util_delay_xxx API
	vos_util_delay_ms(ms);
}

void vos_task_delay_us(int us)
{
	//obsolete API, redirect to vos_util_delay_xxx API
	vos_util_delay_us(us);
}

void vos_task_delay_us_polling(int us)
{
	//obsolete API, redirect to vos_util_delay_xxx API
	vos_util_delay_us_polling(us);
}

void vos_task_update_info(int wait_res, TaskHandle_t task_hdl, ID wairesid, FLGPTN waiptn, UINT wfmode)
{
	TASK_CELL_T        *p_tskcell;

	p_tskcell = vos_task_hdl2cell_p(task_hdl);
	if (NULL == p_tskcell) {
		//DBG_ERR("Invalid task_hdl 0x%x\r\n", (int)task_hdl);
		return;
	}
	p_tskcell->wait_res = wait_res;
	p_tskcell->waiptn = waiptn;            // wait flag pattern
	p_tskcell->wfmode = wfmode;            // wait mode (OR,AND)
	p_tskcell->wairesid = wairesid;
	p_tskcell->wait_time_jiffies = my_jiffies;
}


void vos_task_dump_misc_tasks_p(void)
{
	TaskStatus_t *pxTaskStatusArray;
	volatile UBaseType_t uxArraySize, x;
	TaskHandle_t    task_hdl;
	char            *task_state_str;

	DBG_DUMP("\r\n----------------------MISC TASK ------------------------------------------------------\r\n");
	uxArraySize = uxTaskGetNumberOfTasks();
	pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
	if( pxTaskStatusArray == NULL ) {
		DBG_DUMP("malloc size 0x%x fail\r\n", uxArraySize * sizeof(TaskStatus_t));
		return;
	}
	uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, NULL );
	for( x = 0; x < uxArraySize; x++ ) {
		task_hdl = pxTaskStatusArray[x].xHandle;
		if (vos_task_hdl2cell_p(task_hdl) != NULL) {
			continue;
		}
		task_state_str = get_task_state_str(pxTaskStatusArray[x].eCurrentState);
		if (NULL != task_state_str) {
			DBG_DUMP("\r\nTask[%s] ->  VOS_PRI: NA, OS_PRI: %2d\r\n", pxTaskStatusArray[x].pcTaskName,
				     (int)pxTaskStatusArray[x].uxCurrentPriority);
			DBG_DUMP("Status : [%s] \r\n", task_state_str);
		}
		vos_show_stack(task_hdl, 1);
	}
	vPortFree( pxTaskStatusArray );
}


void vos_task_dump(int (*dump)(const char *fmt, ...))
{
	int             i, task_count = 0;
	char            *task_state_str;
	TASK_CELL_T     *p_tskcell;
	TaskHandle_t    task_hdl;
	UINT64          sleep_time_ms;

	DBG_DUMP("\r\n-------------------------TASK -------------------------------------------------------\r\n");
	for (i = 0; i < gtasknum; i++) {
		p_tskcell = g_st_task_ctrl.p_tasks + i;
		task_hdl = p_tskcell->task_hdl;
		if (FALSE == vos_task_chk_valid_p(p_tskcell)) {
			continue;
		}
		task_count++;

		task_state_str = get_task_state_str(eTaskGetState(task_hdl));
		if (NULL != task_state_str) {
			DBG_DUMP("\r\nTask[%2d, %16s] ->  VOS_PRI: %2d, OS_PRI: %2d\r\n", i, pcTaskGetName(task_hdl),
				     p_tskcell->vos_prio, (int)uxTaskPriorityGet(task_hdl));
			DBG_DUMP("Status : [%s] \r\n", task_state_str);
			if (0 != p_tskcell->wairesid && VOS_RES_TYPE_FLAG == p_tskcell->wait_res) {
				sleep_time_ms = my_jiff_to_ms(my_jiffies - p_tskcell->wait_time_jiffies);
				if (p_tskcell->wfmode & TWF_ORW) {
					DBG_DUMP("Wait for Flag [%.3d, %32s], Pattern: 0x%08X OR, sleep_time = %lld ms\r\n",
							p_tskcell->wairesid, vos_flag_get_name(p_tskcell->wairesid), p_tskcell->waiptn, sleep_time_ms);
				} else {
				    DBG_DUMP("Wait for Flag [%.3d, %32s], Pattern: 0x%08X AND, sleep_time = %lld ms\r\n",
							p_tskcell->wairesid, vos_flag_get_name(p_tskcell->wairesid), p_tskcell->waiptn, sleep_time_ms);
				}
			} else if (0 != p_tskcell->wairesid && VOS_RES_TYPE_SEM == p_tskcell->wait_res) {
				sleep_time_ms = my_jiff_to_ms(my_jiffies - p_tskcell->wait_time_jiffies);
				DBG_DUMP("Wait for Semaphore [%.3d, %32s], sleep_time = %lld ms \r\n", p_tskcell->wairesid,
					      vos_sem_get_name(p_tskcell->wairesid), sleep_time_ms);
			}

		} else {
			continue;
		}
		vos_show_stack(task_hdl, 1);
	}
	DBG_DUMP("\r\n Max task_count = %d , used = %d\r\n", gtasknum, task_count);
	vos_task_dump_misc_tasks_p();
}

void vos_task_dump_by_tskhdl(int (*dump)(const char *fmt, ...), TaskHandle_t task_hdl)
{
	char               *task_state_str;

	if (NULL == task_hdl) {
		return;
	}
	task_state_str = get_task_state_str(eTaskGetState(task_hdl));
	if (NULL != task_state_str) {
		DBG_DUMP("\r\nTask[%s] ->  VOS_PRI: NA, OS_PRI: %2d\r\n", pcTaskGetName(task_hdl),
			     (int)uxTaskPriorityGet(task_hdl));
		DBG_DUMP("Status : [%s] \r\n", task_state_str);
		vos_show_stack(task_hdl, 1);
		DBG_DUMP("\r\n");
	}
}
