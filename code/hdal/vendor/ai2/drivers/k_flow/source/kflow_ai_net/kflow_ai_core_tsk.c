
//=============================================================
#define __CLASS__ 				"[ai][kflow][task]"
#include "kflow_ai_debug.h"
//=============================================================
#include "kflow_ai_net/kflow_ai_net_list.h" //for LIST_HEAD
#include "kflow_ai_net/kflow_ai_core_task.h" //for KFLOW_CORE_TASK_FUNC

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)	vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)	vk_spin_unlock_irqrestore(&my_lock, flags)

THREAD_HANDLE KFLOW_AI_CORE_TSK_ID = 0;
ID FLG_ID_AI_CORE = 0;

#define FLG_ID_AI_CORE_ALL           0xFFFFFFFF
#define FLG_ID_AI_CORE_DATA_IN       FLGPTN_BIT(0)
#define FLG_ID_AI_CORE_STOP          FLGPTN_BIT(30)
#define FLG_ID_AI_CORE_IDLE          FLGPTN_BIT(31)

static UINT32 g_ai_core_tsk_open = 0;
static UINT32 g_ai_core_tsk_drop = 0;

static LIST_HEAD kflow_func_list = {0};

int kflow_ai_core_tsk_get_cnt(void)
{
	return g_ai_core_tsk_open;
}

int kflow_ai_core_tsk_open(void)
{
	if (g_ai_core_tsk_open) {
		g_ai_core_tsk_open ++;
		return 0;
	}
	
	OS_CONFIG_FLAG(FLG_ID_AI_CORE);

	clr_flg(FLG_ID_AI_CORE, FLG_ID_AI_CORE_ALL); //clear all flag
    THREAD_CREATE(KFLOW_AI_CORE_TSK_ID, kflow_ai_core_tsk, NULL, "kflow_ai_core_tsk");
	if (KFLOW_AI_CORE_TSK_ID == 0) {
		//DBG_ERR("task create fail!\r\n");
		return -1;
	}
    THREAD_SET_PRIORITY(KFLOW_AI_CORE_TSK_ID, KFLOW_AI_CORE_TSK_PRI);
    THREAD_RESUME(KFLOW_AI_CORE_TSK_ID);
	INIT_LIST_HEAD(&kflow_func_list);
	g_ai_core_tsk_open ++;
	return 0;
}

int kflow_ai_core_tsk_close(void)
{
	FLGPTN          flag = 0;

	if (g_ai_core_tsk_open == 0) {
		//DBG_ERR("already close or not start yet!\r\n");
		return -1;
	}
	if (g_ai_core_tsk_open > 1) {
		g_ai_core_tsk_open--;
		return 0;
	}

	clr_flg(FLG_ID_AI_CORE, FLG_ID_AI_CORE_IDLE); //clear IDLE before send cmd
	set_flg(FLG_ID_AI_CORE, FLG_ID_AI_CORE_STOP); //send STOP cmd
	
	if (g_ai_core_tsk_drop == 1) {
		//reset cast
		//THREAD_DESTROY(KFLOW_AI_CORE_TSK_ID);  //already call THREAD_RETURN() in task, do not call THREAD_DESTROY() here.
		if (wai_flg(&flag, FLG_ID_AI_CORE, FLG_ID_AI_CORE_IDLE, TWF_ORW) != E_OK) {
			//wait IDLE (stopped)
			DBG_WRN("ctrl-c break, wai_flg() is forced quit!\r\n");
			return -1;
		}
		g_ai_core_tsk_drop = 0; //end drop
	} else {
		//normal cast
		if (wai_flg(&flag, FLG_ID_AI_CORE, FLG_ID_AI_CORE_IDLE, TWF_ORW) != E_OK) { //wait IDLE (stopped)
			DBG_WRN("ctrl-c break, wai_flg() is forced quit!\r\n");
			return -1;
		}
	}
	KFLOW_AI_CORE_TSK_ID = 0; //clear task id

	rel_flg(FLG_ID_AI_CORE);

	g_ai_core_tsk_open--;
	return 0;
}

void kflow_ai_core_tsk_init_func(KFLOW_CORE_TASK_FUNC* p_func, void (*p_exec)(void* p_param), void* p_param)
{
	p_func->p_exec = p_exec;
	p_func->p_param = p_param;
}

void kflow_ai_core_tsk_put_func(KFLOW_CORE_TASK_FUNC* p_func)
{
	unsigned long flags = 0;
	if (!g_ai_core_tsk_open) {
		return;
	}

	loc_cpu(flags);
	//§â JOB ¥[¤J free list
	list_add_tail(&(p_func->list), &kflow_func_list);
	unl_cpu(flags);
	iset_flg(FLG_ID_AI_CORE, FLG_ID_AI_CORE_DATA_IN); //send DATA_IN cmd
	//set_flg(FLG_ID_AI_CORE, FLG_ID_AI_CORE_DATA_IN); //send DATA_IN cmd
}

static int kflow_ai_core_tsk_get_func(KFLOW_CORE_TASK_FUNC* p_get_func)
{
	unsigned long flags = 0;
	KFLOW_CORE_TASK_FUNC* p_func = 0;
	loc_cpu(flags);
	if (!list_empty(&kflow_func_list)) {
		p_func = list_first_entry(&kflow_func_list, KFLOW_CORE_TASK_FUNC, list);
		list_del(&(p_func->list));
		p_func->list.prev = 0;
		p_func->list.next = 0;
		if (p_get_func)
			p_get_func[0] = p_func[0];
		unl_cpu(flags);
		return 0;
	}
	unl_cpu(flags);
	return -1;
}

int kflow_ai_core_tsk_reset(void)
{
	if (g_ai_core_tsk_open == 0) {
		//DBG_ERR("already close or not start yet!\r\n");
		return -1;
	}

	g_ai_core_tsk_drop = 1; //begin drop
	return 0;
}

THREAD_DECLARE(kflow_ai_core_tsk, arglist)
{
	FLGPTN flag = 0;
	BOOL is_continue = 1;
	UINT32 n_drop = 0;

	THREAD_ENTRY();

	//clr_flg(FLG_ID_AI_CORE, FLG_ID_AI_CORE_IDLE);

	while (is_continue) {
		//set_flg(FLG_ID_AI_CORE, FLG_ID_AI_CORE_IDLE);
		//PROFILE_TASK_IDLE();
		wai_flg(&flag, FLG_ID_AI_CORE, FLG_ID_AI_CORE_DATA_IN | FLG_ID_AI_CORE_STOP, TWF_ORW | TWF_CLR);
		//PROFILE_TASK_BUSY();
		//clr_flg(FLG_ID_AI_CORE, FLG_ID_AI_CORE_IDLE);
		//DBG_IND("flag=0x%x\r\n", flag);
		if (flag & FLG_ID_AI_CORE_STOP) {
			is_continue = 0;
			continue;
		}
		if (flag & FLG_ID_AI_CORE_DATA_IN) {
			KFLOW_CORE_TASK_FUNC do_func = {0};
			while (kflow_ai_core_tsk_get_func(&do_func) == 0) {
				if (do_func.p_exec) {
					do_func.p_exec(do_func.p_param);
				}
				if (g_ai_core_tsk_drop) {
					while (kflow_ai_core_tsk_get_func(&do_func) == 0) {
						n_drop ++;
					}
					//set_flg(FLG_ID_AI_CORE, FLG_ID_AI_CORE_STOP); // mark this to fix reset flow after ctrl+c
				}
			}
		}
	}
	set_flg(FLG_ID_AI_CORE, FLG_ID_AI_CORE_IDLE);

	if (n_drop > 0) {
		DBG_DUMP("job task: drop %u deferred func!\r\n", n_drop);
	}

	THREAD_RETURN(0);
}



