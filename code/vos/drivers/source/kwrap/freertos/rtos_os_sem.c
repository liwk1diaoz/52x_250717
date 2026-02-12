/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <kwrap/semaphore.h>
#include <kwrap/spinlock.h>
#include "vos_ioctl.h"

#define __MODULE__    rtos_sem
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>

#include "freertos_ext_kdrv.h"
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define RTOS_SEM_INITED_TAG     MAKEFOURCC('R', 'S', 'E', 'M') ///< a key value
#define WAIT_TASK_NUM_MAX       3

#define VOS_STRCPY(dst, src, dst_size) do { \
	strncpy(dst, src, (dst_size)-1); \
	dst[(dst_size)-1] = '\0'; \
} while(0)

#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

#if defined(__LINUX)
#define my_alloc            vmalloc
#define my_free             vfree
#define cur_task_name       current->comm
#define my_jiffies          jiffies_64
#define my_jiff_to_ms(j)    ((j) * (1000 / HZ))

#elif defined(__FREERTOS)
#define my_alloc            malloc
#define my_free             free
#define current             xTaskGetCurrentTaskHandle()
#define cur_task_name       pcTaskGetName(xTaskGetCurrentTaskHandle())
#define my_jiffies          hwclock_get_longcounter()
#define my_jiff_to_ms(j)    ((j) / 1000)

#ifndef unlikely
#define unlikely(x) (x)
#endif

#else
#error Not supported OS
#endif

typedef struct {
	TaskHandle_t            task_hdl;
	UINT64                  wait_time_jiffies;
} SEM_WAIT_TSK_INFO;

typedef struct {
	UINT                    st_used;
	INT                     isemcnt;        ///< Semaphore counter,
	INT                     maxsem;         ///< Semaphore Max counter value,
	SemaphoreHandle_t       sem_hdl;
	StaticSemaphore_t       sem_buf;        ///< static sem buffer used by xSemaphoreCreateCountingStatic()
	char                    name[VOS_SEM_NAME_SIZE];
	TaskHandle_t            own_tsk;
	SEM_WAIT_TSK_INFO       waitsk_list[WAIT_TASK_NUM_MAX];
	int                     waitsk_count;
} SEM_CELL_T;

typedef struct {
	SEM_CELL_T              *p_sems;
} SEM_CTRL_T;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
#if ( configUSE_NVT_PATH > 0 )
extern uint32_t ulPortIsIntrruptMasked( void ); //rtos-code\driver\na51055\source\portable\port.c
#endif
/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_sem_debug_level = NVT_DBG_WRN;

static  VK_DEFINE_SPINLOCK(my_lock);

static int            gsemnum = 0;
static int            g_cur_sem_id = 1;
static SEM_CTRL_T     g_st_sem_ctrl = {0};

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
static void rtos_sem_add_waitlist_p(SEM_CELL_T   *p_semcell, TaskHandle_t task_hdl)
{
	int i;
	SEM_WAIT_TSK_INFO *p_waitsk;

	p_waitsk = p_semcell->waitsk_list;
	for (i = 0; i < WAIT_TASK_NUM_MAX; i++) {
		if (0 == p_waitsk->task_hdl) {
			p_waitsk->task_hdl = task_hdl;
			p_waitsk->wait_time_jiffies = my_jiffies;
			p_semcell->waitsk_count++;
			return;
		}
		p_waitsk++;
	}
}

static void rtos_sem_del_waitlist_p(SEM_CELL_T   *p_semcell, TaskHandle_t task_hdl)
{
	int i;
	SEM_WAIT_TSK_INFO *p_waitsk;

	p_waitsk = p_semcell->waitsk_list;
	for (i = 0; i < WAIT_TASK_NUM_MAX; i++) {
		if (task_hdl == p_waitsk->task_hdl) {
			p_waitsk->task_hdl = 0;
			p_semcell->waitsk_count--;
			return;
		}
		p_waitsk++;
	}
}

void rtos_sem_init(unsigned long max_sem_num)
{
	DBG_FUNC_BEGIN("\r\n");
	DBG_IND("max_sem_num =%ld\r\n", max_sem_num);

	// initialize value
	gsemnum = max_sem_num;
	g_cur_sem_id = 1;

	g_st_sem_ctrl.p_sems = (SEM_CELL_T *)my_alloc(sizeof(SEM_CELL_T) * gsemnum);
	if (g_st_sem_ctrl.p_sems == NULL) {
		DBG_ERR(" alloc!\n");
		goto END_INIT;
	}
	DBG_IND("p_sems =0x%lx\r\n", (ULONG)g_st_sem_ctrl.p_sems);

	memset(g_st_sem_ctrl.p_sems, 0x00, sizeof(SEM_CELL_T) * gsemnum);

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
void rtos_sem_exit(void)
{
	my_free(g_st_sem_ctrl.p_sems);
	g_st_sem_ctrl.p_sems = NULL;
}

int vos_sem_max_cnt(void)
{
	return gsemnum;
}

int vos_sem_used_cnt(void)
{
	SEM_CELL_T *p_semcell;
	unsigned long flags = 0;
	int idx;
	int used_cnt = 0;

	loc_cpu(flags);

	p_semcell = g_st_sem_ctrl.p_sems;

	for (idx = 0; idx < gsemnum; idx++) {
		if (p_semcell->st_used) {
			used_cnt++;
		}
		p_semcell++;
	}

	unl_cpu(flags);

	return used_cnt;
}

static int  vos_sem_get_free_id(ID *p_semid)
{
	SEM_CELL_T         *p_semcell;
	int                 cur_id, i;
	unsigned long       flags = 0;

	loc_cpu(flags);
	if (unlikely(g_cur_sem_id >= gsemnum)) {
		// reset id
		g_cur_sem_id = 1;
	}
	cur_id = g_cur_sem_id;
	for (i = 0; i < gsemnum; i++) {
		if (unlikely(cur_id >= gsemnum)) {
			cur_id = 1;
		}
		DBG_IND("cur_id=%d\r\n", cur_id);
		p_semcell = g_st_sem_ctrl.p_sems + cur_id;
		if (0 == p_semcell->st_used) {
			p_semcell->st_used = RTOS_SEM_INITED_TAG;
			g_cur_sem_id = cur_id;
			g_cur_sem_id++;
			*p_semid = cur_id;
			unl_cpu(flags);
			DBG_IND("g_cur_sem_id=%d\r\n", cur_id);
			return 0;
		}
		cur_id++;
	}
	*p_semid = 0;
	unl_cpu(flags);
	return -1;
}

int vos_sem_set_name(ID semid, char *name)
{
	SEM_CELL_T *p_semcell;

	if (unlikely(semid == 0 || semid >= gsemnum)) {
		DBG_ERR("Invalid semid %d\r\n", semid);
		return -1;
	}
	p_semcell = g_st_sem_ctrl.p_sems + semid;
	VOS_STRCPY(p_semcell->name, name, sizeof(p_semcell->name));
	DBG_IND("semid = %d, name = %s\r\n", semid, name);
	return 0;
}

char *vos_sem_get_name(ID semid)
{
	SEM_CELL_T *p_semcell;

	if (unlikely(semid == 0 || semid >= gsemnum)) {
		DBG_ERR("Invalid flgid %d\r\n", semid);
		return NULL;
	}
	p_semcell = g_st_sem_ctrl.p_sems + semid;
	return p_semcell->name;
}

static void vos_sem_clear_info_p(SEM_CELL_T   *p_semcell)
{
	if (RTOS_SEM_INITED_TAG == p_semcell->st_used) {
		p_semcell->name[0] = '\0';
		p_semcell->st_used = 0;
	}
}

static BOOL vos_sem_chk_valid_p(SEM_CELL_T   *p_semcell)
{
	if (RTOS_SEM_INITED_TAG != p_semcell->st_used) {
		return FALSE;
	}
	return TRUE;
}

int vos_sem_create(ID *p_semid, int init_cnt, char *name)
{
	SEM_CELL_T         *p_semcell;
	ID                 semid_new;

	if (unlikely(NULL == p_semid)) {
		DBG_ERR("p_semid is NULL\n");
		return -1;
	}

	if (unlikely(vos_sem_get_free_id(&semid_new) < 0)) {
		DBG_ERR("sem out of id\r\n");
		return -1;
	}

	if (unlikely(semid_new == 0 || semid_new >= gsemnum)) {
		DBG_ERR("Invalid semid %d\r\n", semid_new);
		return -1;
	}

	p_semcell = g_st_sem_ctrl.p_sems + semid_new;
	p_semcell->isemcnt = init_cnt;
	p_semcell->maxsem = -1;
	p_semcell->sem_hdl = xSemaphoreCreateCountingStatic((UBaseType_t)-1, init_cnt, &p_semcell->sem_buf);
	VOS_STRCPY(p_semcell->name, name, sizeof(p_semcell->name));

	*p_semid = semid_new;

	return 0;
}

void  vos_sem_destroy(ID semid)
{
	SEM_CELL_T         *p_semcell;

	if (unlikely(semid == 0 || semid >= gsemnum)) {
		DBG_ERR("Invalid semid %d, task [%s]\r\n", semid, cur_task_name);
		return;
	}
	p_semcell = g_st_sem_ctrl.p_sems + semid;
	vos_sem_clear_info_p(p_semcell);
}

static int _vos_sem_wait(ID semid, int timeout, int interruptible)
{
	SEM_CELL_T         *p_semcell;
	int                freertos_ret = 0;
	unsigned long      flags;

	if (unlikely(semid == 0 || semid >= gsemnum)) {
		DBG_ERR("Invalid semid %d, task [%s]\r\n", semid, cur_task_name);
		return -1;
	}
	p_semcell = g_st_sem_ctrl.p_sems + semid;

	// if isemcnt == 0, add to the waiting list and update task info
	loc_cpu(flags);
	rtos_sem_add_waitlist_p(p_semcell, current);
	if (0 == p_semcell->isemcnt) {
		vos_task_update_info(VOS_RES_TYPE_SEM, current, semid, 0, 0);
	}
	unl_cpu(flags);

	// start to wait
	if (interruptible) {
		freertos_ret = xSemaphoreTake(p_semcell->sem_hdl, portMAX_DELAY);
	} else if (timeout == -1) {
		freertos_ret = xSemaphoreTake(p_semcell->sem_hdl, portMAX_DELAY);
	} else {
		freertos_ret = xSemaphoreTake(p_semcell->sem_hdl, timeout);
	}

	loc_cpu(flags);

	// remove from the waiting list and update task info
	rtos_sem_del_waitlist_p(p_semcell, current);
	if (0 == p_semcell->isemcnt) {
		vos_task_update_info(VOS_RES_TYPE_INVALID, current, 0, 0, 0);
	}

	// check if got a sem successfully
	if (pdTRUE == freertos_ret) {
		p_semcell->isemcnt--;
		// set the sem owner to the last task (isemcnt == 0)
		// although owners could be different tasks (can be improved in the future)
		if (0 == p_semcell->isemcnt) {
			p_semcell->own_tsk = current;
		}
	}

	unl_cpu(flags);

	if (pdTRUE != freertos_ret) {
		return -1;
	}

	return 0;
}

int vos_sem_wait(ID semid)
{
	return _vos_sem_wait(semid, -1, 0);
}

int vos_sem_wait_timeout(ID semid, int timeout_tick)
{
	return _vos_sem_wait(semid, timeout_tick, 0);
}

int vos_sem_wait_interruptible(ID semid)
{
	return _vos_sem_wait(semid, -1, 1);
}

void vos_sem_sig(ID semid)
{
	SEM_CELL_T         *p_semcell;
	unsigned long      flags;

	if (unlikely(semid == 0 || semid >= gsemnum)) {
		DBG_ERR("Invalid semid %d, task [%s]\r\n", semid, cur_task_name);
		return;
	}
	p_semcell = g_st_sem_ctrl.p_sems + semid;

	loc_cpu(flags);
	p_semcell->isemcnt++;
	p_semcell->own_tsk = NULL;
	unl_cpu(flags);

#if ( configUSE_NVT_PATH > 0 )
	if (ulPortIsIntrruptMasked()) {
		DBG_WRN("sem sig in critical section\r\n");
		vos_dump_stack();
	}
#endif

	if (pdPASS != xSemaphoreGive(p_semcell->sem_hdl)) {
		DBG_ERR("xSemaphoreGive failed, sem_hdl 0x%lX\r\n", (ULONG)p_semcell->sem_hdl);
	}
}

void vos_sem_isig(ID semid)
{
	SEM_CELL_T         *p_semcell;
	unsigned long      flags;
	BaseType_t         xHigherPriorityTaskWoken;

	if (unlikely(semid == 0 || semid >= gsemnum)) {
		DBG_ERR("Invalid semid %d\r\n", semid);
		return;
	}
	p_semcell = g_st_sem_ctrl.p_sems + semid;

	loc_cpu(flags);
	p_semcell->isemcnt++;
	p_semcell->own_tsk = NULL;
	unl_cpu(flags);

	if (pdPASS == xSemaphoreGiveFromISR(p_semcell->sem_hdl, &xHigherPriorityTaskWoken)) {
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	} else {
		DBG_ERR("xSemaphoreGiveFromISR failed, sem_hdl 0x%lX\r\n", (ULONG)p_semcell->sem_hdl);
	}
}

void vos_sem_dump(int (*dump)(const char *fmt, ...), int level)
{
	int                i, tsk_i, task_id, sem_cnt = 0;
	SEM_CELL_T         *p_semcell;
	SEM_WAIT_TSK_INFO  *p_waitsk;
	UINT64              sleep_time_ms;

	DBG_DUMP("\r\n-------------------------SEM --------------------------------------------------------\r\n");
	for (i = 0; i < gsemnum; i++) {
		p_semcell = g_st_sem_ctrl.p_sems + i;
		if (FALSE == vos_sem_chk_valid_p(p_semcell)) {
			continue;
		}
		sem_cnt++;
		if (NULL == p_semcell->own_tsk) {
			if (0 == level && 0 == p_semcell->waitsk_count) {
				continue;
			}
			DBG_DUMP("Semaphore[%.3d, %32s] -> (Max: %.2d, Cur: %.2d), Owner: None\r\n",
					i, p_semcell->name, p_semcell->maxsem, p_semcell->isemcnt);
		} else {
			DBG_DUMP("Semaphore[%.3d, %32s] -> * (Max: %.2d, Cur: %.2d), Owner: %s\r\n",
			     i, p_semcell->name, p_semcell->maxsem, p_semcell->isemcnt, pcTaskGetName(p_semcell->own_tsk));
		}
		if (p_semcell->waitsk_count > 0) {
			DBG_DUMP("Waiting Task Queue : \r\n");
			p_waitsk = p_semcell->waitsk_list;
			for (tsk_i = 0; tsk_i < WAIT_TASK_NUM_MAX; tsk_i++) {
				if (p_waitsk->task_hdl == NULL)
					continue;
				task_id = vos_task_hdl2id(p_waitsk->task_hdl);
				sleep_time_ms = my_jiff_to_ms(my_jiffies - p_waitsk->wait_time_jiffies);
				if (0 != p_waitsk->task_hdl) {
					DBG_DUMP("  Task[%.3d, %s], sleep_time = %lld ms\r\n",
						     task_id, pcTaskGetName(p_waitsk->task_hdl), sleep_time_ms);
				}
				if (task_id < 0) {
					vos_task_dump_by_tskhdl(dump, p_waitsk->task_hdl);
				}
				p_waitsk++;
			}
		}
	}
	DBG_DUMP("\r\n Max sem_cnt = %d , used = %d\r\n", gsemnum, sem_cnt);
}

/*-----------------------------------------------------------------------------*/
/* Kernel Mode Definiton                                                       */
/*-----------------------------------------------------------------------------*/


