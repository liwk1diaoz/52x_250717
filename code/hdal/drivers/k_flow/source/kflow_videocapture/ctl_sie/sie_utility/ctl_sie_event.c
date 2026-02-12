/**
    SIE event.

    @file       ctl_sie_event.c
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "ctl_sie_event_id_int.h"
#include "ctl_sie_dbg.h"
#include "ctl_sie_event_int.h"
#define clock(a) (0)
static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

static SIE_EVENT_ROOT_ITEM event_root_pool[SIE_EVENT_ROOT_MAX];
static SIE_EVENT_ENTRY_ITEM event_entry_item_pool[SIE_EVENT_FREE_ENTRY_ITEM_NUM];

static CTL_SIE_LIST_HEAD event_free_root_head;
static CTL_SIE_LIST_HEAD event_free_entry_item_head;
#if 0
#endif

static SIE_EVENT_LOCK_INFO __sie_event_get_lock_info(UINT32 lock)
{
	SIE_EVENT_LOCK_INFO rt_info;

	rt_info.flag = (UINT32)(lock / SIE_FREE_FLAG_PAGE_SIZE);
	rt_info.ptn = 1 << (lock - (rt_info.flag * SIE_FREE_FLAG_PAGE_SIZE));
	return rt_info;
}

static void __sie_event_free_pool_lock(void)
{
	FLGPTN wai_flag = 0;

	if (vos_flag_wait_timeout(&wai_flag, sie_event_flg_id[0], SIE_EVENT_POOL_LOCK, TWF_CLR, vos_util_msec_to_tick(1000)) != 0) {
		ctl_sie_dbg_err("event free pool lock timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(sie_event_flg_id[0], 0xffffffff), SIE_EVENT_POOL_LOCK);
	}
}

static void __sie_event_free_pool_unlock(void)
{
	vos_flag_set(sie_event_flg_id[0], SIE_EVENT_POOL_LOCK);
}

static void __sie_event_lock(UINT32 lock)
{
	FLGPTN wai_flag = 0;
	SIE_EVENT_LOCK_INFO lock_info;

	lock_info = __sie_event_get_lock_info(lock);

	if (vos_flag_wait_timeout(&wai_flag, sie_event_flg_id[lock_info.flag], lock_info.ptn, TWF_CLR, vos_util_msec_to_tick(1000)) != 0) {
		ctl_sie_dbg_err("wait event lock timeout, cur_flg = 0x%x, wait_flg = 0x%x\r\n", vos_flag_chk(sie_event_flg_id[lock_info.flag], 0xffffffff), lock_info.ptn);
	}
}

static void __sie_event_unlock(UINT32 lock)
{
	SIE_EVENT_LOCK_INFO lock_info;

	lock_info = __sie_event_get_lock_info(lock);
	vos_flag_set(sie_event_flg_id[lock_info.flag], lock_info.ptn);
}

static UINT32 __sie_event_is_root_item(SIE_EVENT_ROOT_ITEM *p_root)
{
	UINT32 i;

	for (i = 0; i < SIE_EVENT_ROOT_MAX; i++) {
		if ((UINT32)p_root == (UINT32)&event_root_pool[i]) {
			return CTL_SIE_EVENT_OK;
		}
	}
	ctl_sie_dbg_err("invalid hdl (0x%.8x)\r\n", (UINT32)p_root);
	return CTL_SIE_EVENT_NG;
}

static SIE_EVENT_ENTRY_ITEM *__sie_event_check_register_fp(SIE_EVENT_ROOT_ITEM *p_root, UINT32 event, CTL_SIE_EVENT_FP fp)
{
	CTL_SIE_LIST_HEAD *p_list;
	SIE_EVENT_ENTRY_ITEM *p_entry_item;

	vos_list_for_each(p_list, &p_root->entry_list[event]) {
		p_entry_item = vos_list_entry(p_list, SIE_EVENT_ENTRY_ITEM, list);
		if (p_entry_item->fp == fp) {
			return p_entry_item;
		}
	}
	return NULL;
}


static SIE_EVENT_ENTRY_ITEM *__sie_event_entry_item_lock(void)
{
	SIE_EVENT_ENTRY_ITEM *p_entry_item = NULL;

	__sie_event_free_pool_lock();
	//get free item
	if (!vos_list_empty(&event_free_entry_item_head)) {
		p_entry_item = vos_list_entry(event_free_entry_item_head.next, SIE_EVENT_ENTRY_ITEM, list);
		p_entry_item->sts &= ~SIE_EVENT_STS_FREE;
		vos_list_del_init(&p_entry_item->list);
	}
	__sie_event_free_pool_unlock();
	return p_entry_item;
}

static void __sie_event_entry_item_unlock(SIE_EVENT_ENTRY_ITEM *p_entry_item)
{
	__sie_event_free_pool_lock();

	/* clear status */
	p_entry_item->sts |= SIE_EVENT_STS_FREE;
	p_entry_item->sts &= ~SIE_EVENT_STS_CTL_MASK;
	p_entry_item->fp = NULL;

	/* add to free list */
	vos_list_add_tail(&p_entry_item->list, &event_free_entry_item_head);
	__sie_event_free_pool_unlock();
}

static void __sie_event_entry_item_unlock_isr(SIE_EVENT_ENTRY_ITEM *p_entry_item)
{
	/* clear status */
	p_entry_item->sts |= SIE_EVENT_STS_FREE;
	p_entry_item->sts &= ~SIE_EVENT_STS_CTL_MASK;
	p_entry_item->fp = NULL;

	/* add to free list */
	vos_list_add_tail(&p_entry_item->list, &event_free_entry_item_head);
}

#if 0
#endif
#if 0
void sie_event_dump_free_list(void)
{
	CTL_SIE_LIST_HEAD *p_list;
	SIE_EVENT_ENTRY_ITEM *p_entry_item;
	SIE_EVENT_ROOT_ITEM *p_root_item;

	DBG_DUMP("-------- sie event free root --------------------------\r\n");
	vos_list_for_each(p_list, &event_free_root_head) {
		p_root_item = vos_list_entry(p_list, SIE_EVENT_ROOT_ITEM, root_list);
		DBG_DUMP("0x%.8x\r\n", p_root_item->sts);
	}

	DBG_DUMP("-------- sie event free entry --------------------------\r\n");
	vos_list_for_each(p_list, &event_free_entry_item_head) {
		p_entry_item = vos_list_entry(p_list, SIE_EVENT_ENTRY_ITEM, list);
		DBG_DUMP("0x%.8x\r\n", p_entry_item->sts);
	}
}
#endif

void sie_event_dump(int (*dump)(const char *fmt, ...))
{
	UINT32 i, j;
	CTL_SIE_LIST_HEAD *p_list;
	SIE_EVENT_ENTRY_ITEM *p_entry_item;


	if (dump == NULL) {
		DBG_ERR("dump funct NULL\r\n");
		return;
	}

	dump("\r\n-----sie event info-----\r\n");

	i = 0;
	vos_list_for_each(p_list, &event_free_root_head) {
		i += 1;
	}
	dump("max root num = %d (free = %d)\r\n", (int)(SIE_EVENT_ROOT_MAX), (int)(i));


	i = 0;
	vos_list_for_each(p_list, &event_free_entry_item_head) {
		i += 1;
	}
	dump("max entry item num %d (free %d)\r\n", (int)(SIE_EVENT_FREE_ENTRY_ITEM_NUM), (int)(i));

	for (i = 0; i < SIE_EVENT_FLAG_MAX; i++) {
		dump("flag id[%d] %d ptn 0x%.8x\r\n", (int)i, (int)sie_event_flg_id[i], (unsigned int)vos_flag_chk(sie_event_flg_id[i], FLGPTN_BIT_ALL));
	}

	for (i = 0; i < SIE_EVENT_ROOT_MAX; i++) {

		if (event_root_pool[i].sts & SIE_EVENT_STS_FREE) {
			continue;
		}
		dump("\r\nroot(%2d) %10s 0x%.8x-----\r\n",
							(int)i, event_root_pool[i].name, (unsigned int)event_root_pool[i].sts);
		for (j = 0; j < SIE_EVENT_ENTRY_MAX; j++) {

			if (!vos_list_empty(&event_root_pool[i].entry_list[j])) {
				dump("entry       name  func addr     status   ProcT(us)\r\n");
				vos_list_for_each(p_list, &event_root_pool[i].entry_list[j]) {
					p_entry_item = vos_list_entry(p_list, SIE_EVENT_ENTRY_ITEM, list);
					dump("%5d %10s 0x%.8x 0x%.8x %11d\r\n", (int)j, p_entry_item->name, (unsigned int)p_entry_item->fp, (unsigned int)p_entry_item->sts, (unsigned int)(p_entry_item->exit_ts - p_entry_item->enter_ts));
				}
			}
		}
	}
}

void sie_event_reset(void)
{
	UINT32 i, j, entry_lock_val, root_lock_val;

	/* reset & create free root pool list  */
	entry_lock_val = SIE_FREE_FLAG_START;
	root_lock_val = 1;

	VOS_INIT_LIST_HEAD(&event_free_root_head);
	for (i = 0; i < SIE_EVENT_ROOT_MAX; i++) {

		event_root_pool[i].name[0] = '\0';
		event_root_pool[i].sts = (i | SIE_EVENT_STS_FREE);
		event_root_pool[i].root_lock = root_lock_val;

		for (j = 0; j < SIE_EVENT_ENTRY_MAX; j++) {
			event_root_pool[i].entry_lock[j] = entry_lock_val;
			VOS_INIT_LIST_HEAD(&event_root_pool[i].entry_list[j]);

			entry_lock_val += 1;
		}
		vos_list_add_tail(&event_root_pool[i].root_list, &event_free_root_head);
		root_lock_val += 1;
	}

	/* reset & create free entry item pool list  */
	VOS_INIT_LIST_HEAD(&event_free_entry_item_head);
	for (i = 0; i < SIE_EVENT_FREE_ENTRY_ITEM_NUM; i++) {
		event_entry_item_pool[i].sts = (i | SIE_EVENT_STS_FREE);
		event_entry_item_pool[i].fp = NULL;
		vos_list_add_tail(&event_entry_item_pool[i].list, &event_free_entry_item_head);
	}
	//reset free flag
	for (i = 0; i < SIE_EVENT_FLAG_MAX; i++) {
		if (i == 0) {
			vos_flag_set(sie_event_flg_id[i], (FLGPTN_BIT_ALL & ~(SIE_EVENT_POOL_LOCK)));
		} else {
			vos_flag_set(sie_event_flg_id[i], FLGPTN_BIT_ALL);
		}
	}
	vos_flag_set(sie_event_flg_id[0], SIE_EVENT_POOL_LOCK);
}

UINT32 sie_event_open(CHAR *name)
{
	UINT32 handle = 0;
	SIE_EVENT_ROOT_ITEM *p_root_item;

	__sie_event_free_pool_lock();
	if (vos_list_empty(&event_free_root_head)) {
		ctl_sie_dbg_err("open fail\r\n");
	} else {
		p_root_item = vos_list_entry(event_free_root_head.next, SIE_EVENT_ROOT_ITEM, root_list);
		vos_list_del_init(&p_root_item->root_list);

		/* update root status */
		__sie_event_lock(p_root_item->root_lock);
		memset((void *)&p_root_item->name[0], '\0', sizeof(p_root_item->name));
		if (name != NULL) {
			strncpy(p_root_item->name, name, sizeof(p_root_item->name)-1);
		}
		p_root_item->sts &= ~(SIE_EVENT_STS_FREE|SIE_EVENT_STS_CTL_MASK);
		__sie_event_unlock(p_root_item->root_lock);
		handle = (UINT32)p_root_item;
	}
	__sie_event_free_pool_unlock();
	return handle;
}

UINT32 sie_event_close(UINT32 handle)
{
	SIE_EVENT_ROOT_ITEM *p_root_item;
	CTL_SIE_LIST_HEAD *p_list, *n;
	SIE_EVENT_ENTRY_ITEM *p_entry_item;
	UINT32 i;
	unsigned long flags;

	p_root_item = (SIE_EVENT_ROOT_ITEM *)handle;
	if (p_root_item->sts & SIE_EVENT_STS_FREE) {
		ctl_sie_dbg_err("error hdl(0x%.8x)\r\n", handle);
		return CTL_SIE_EVENT_NG;
	}

	if (__sie_event_is_root_item(p_root_item) == CTL_SIE_EVENT_NG) {
		ctl_sie_dbg_err("close fail (0x%.8x)\r\n", handle);
		return CTL_SIE_EVENT_NG;
	}

	__sie_event_lock(p_root_item->root_lock);
	p_root_item->sts |= SIE_EVENT_STS_FREE;
	p_root_item->sts &= ~SIE_EVENT_STS_CTL_MASK;
	p_root_item->name[0] = '\0';
	__sie_event_unlock(p_root_item->root_lock);
	__sie_event_free_pool_lock();

	/* flush entry item */
	for (i = 0; i < SIE_EVENT_ENTRY_MAX; i++) {
		vos_list_for_each_safe(p_list, n, &p_root_item->entry_list[i]) {
			p_entry_item = vos_list_entry(p_list, SIE_EVENT_ENTRY_ITEM, list);
			if (p_entry_item->sts & CTL_SIE_EVENT_ISR_TAG) {
				loc_cpu(flags);
				vos_list_del_init(&p_entry_item->list);
				unl_cpu(flags);
			} else {
				vos_list_del_init(&p_entry_item->list);
			}
			/* clear status */
			p_entry_item->sts |= SIE_EVENT_STS_FREE;
			p_entry_item->sts &= ~SIE_EVENT_STS_CTL_MASK;
			p_entry_item->fp = NULL;
			/* add to free list */
			vos_list_add_tail(&p_entry_item->list, &event_free_entry_item_head);
		}
	}
	vos_list_add_tail(&p_root_item->root_list, &event_free_root_head);
	__sie_event_free_pool_unlock();
	return CTL_SIE_EVENT_OK;
}

UINT32 sie_event_register(UINT32 handle, UINT32 event, CTL_SIE_EVENT_FP fp, CHAR *name)
{
	SIE_EVENT_ROOT_ITEM *p_root_item;
	SIE_EVENT_ENTRY_ITEM *p_entry_item;
	UINT32 event_idx, rt;
	unsigned long flags;

	event_idx = (event & (~SIE_EVENT_EVENT_CTL_MASK));
	if (event_idx >= SIE_EVENT_ENTRY_MAX) {
		ctl_sie_dbg_err("evt_idx(%d) ovf\r\n", (int)(event_idx));
		return CTL_SIE_EVENT_NG;
	}

	p_root_item = (SIE_EVENT_ROOT_ITEM *)handle;
	if (p_root_item->sts & SIE_EVENT_STS_FREE) {
		ctl_sie_dbg_err("err hdl(0x%.8x)\r\n", handle);
		return CTL_SIE_EVENT_NG;
	}

	if (fp == NULL) {
		return CTL_SIE_EVENT_NG;
	}

	__sie_event_lock(p_root_item->entry_lock[event_idx]);
	rt = CTL_SIE_EVENT_OK;
	if (__sie_event_check_register_fp(p_root_item, event_idx, fp) == NULL) {
		p_entry_item = __sie_event_entry_item_lock();
		if (p_entry_item != NULL) {
			p_entry_item->fp = fp;
			p_entry_item->sts |= (event & SIE_EVENT_EVENT_CTL_MASK);
			p_entry_item->enter_ts = 0;
			p_entry_item->exit_ts = 0;
			memset((void *)&p_entry_item->name[0], '\0', sizeof(p_entry_item->name));
			if (name != NULL) {
				strncpy(p_entry_item->name, name, sizeof(p_entry_item->name)-1);
			}

			if (p_entry_item->sts & CTL_SIE_EVENT_ISR_TAG) {
				loc_cpu(flags);
				vos_list_add_tail(&p_entry_item->list, &p_root_item->entry_list[event_idx]);
				unl_cpu(flags);
			} else {
				vos_list_add_tail(&p_entry_item->list, &p_root_item->entry_list[event_idx]);
			}
			rt = E_OK;
		} else {
			ctl_sie_dbg_err("ret. fail(0x%.8x %d 0x%.8x)\r\n", handle, event, (UINT32)fp);
		}
	}
	__sie_event_unlock(p_root_item->entry_lock[event_idx]);
	return rt;
}

UINT32 sie_event_unregister(UINT32 handle, UINT32 event, CTL_SIE_EVENT_FP fp)
{
	UINT32 rt;
	SIE_EVENT_ROOT_ITEM *p_root_item;
	SIE_EVENT_ENTRY_ITEM *p_entry_item;
	UINT32 event_idx;
	unsigned long flags;

	event_idx = (event & (~SIE_EVENT_EVENT_CTL_MASK));
	if (event_idx >= SIE_EVENT_ENTRY_MAX) {
		ctl_sie_dbg_err("event_idx(%d) ovf\r\n", (int)(event_idx));
		return CTL_SIE_EVENT_NG;
	}

	p_root_item = (SIE_EVENT_ROOT_ITEM *)handle;
	if (p_root_item->sts & SIE_EVENT_STS_FREE) {
		ctl_sie_dbg_err("err hdl(0x%.8x)\r\n", handle);
		return CTL_SIE_EVENT_NG;
	}

	if (fp == NULL) {
		return CTL_SIE_EVENT_NG;
	}

	__sie_event_lock(p_root_item->entry_lock[event_idx]);
	rt = CTL_SIE_EVENT_OK;
	p_entry_item = __sie_event_check_register_fp(p_root_item, event_idx, fp);
	if (p_entry_item != NULL) {
		//remove from list
		if (p_entry_item->sts & CTL_SIE_EVENT_ISR_TAG) {
			loc_cpu(flags);
			vos_list_del_init(&p_entry_item->list);
			unl_cpu(flags);
		} else {
			vos_list_del_init(&p_entry_item->list);
		}
		//add to free list
		__sie_event_entry_item_unlock(p_entry_item);
		rt = E_OK;
	}
	__sie_event_unlock(p_root_item->entry_lock[event_idx]);
	return rt;
}

INT32 sie_event_proc(UINT32 handle, UINT32 event, void *p_in, void *p_out)
{
	SIE_EVENT_ROOT_ITEM *p_root_item;
	CTL_SIE_LIST_HEAD *p_list, *n;
	SIE_EVENT_ENTRY_ITEM *p_entry_item;
	UINT32 event_idx;
	INT32 rt = 0;

	if (event & CTL_SIE_EVENT_ISR_TAG) {
		ctl_sie_dbg_err("can't process isr evt(0x%.8x)\r\n", event);
		return rt;
	}

	event_idx = (event & (~SIE_EVENT_EVENT_CTL_MASK));
	if (event_idx >= SIE_EVENT_ENTRY_MAX) {
		ctl_sie_dbg_err("evt idx(%d) ovf\r\n", (int)(event_idx));
		return rt;
	}

	p_root_item = (SIE_EVENT_ROOT_ITEM *)handle;
	if (p_root_item->sts & SIE_EVENT_STS_FREE) {
		ctl_sie_dbg_err("err hdl(0x%.8x)\r\n", handle);
		return rt;
	}

	__sie_event_lock(p_root_item->entry_lock[event_idx]);
	vos_list_for_each_safe(p_list, n, &p_root_item->entry_list[event_idx]) {
		p_entry_item = vos_list_entry(p_list, SIE_EVENT_ENTRY_ITEM, list);
		if (p_entry_item->fp != NULL) {
			p_entry_item->enter_ts = (UINT32)clock();
			rt = p_entry_item->fp(event, p_in, p_out);
			p_entry_item->exit_ts = (UINT32)clock();

			/* one shot flow */
			if (p_entry_item->sts & CTL_SIE_EVENT_ONE_SHOT) {
				vos_list_del_init(&p_entry_item->list);
				__sie_event_entry_item_unlock(p_entry_item);
			}
		}
	}
	__sie_event_unlock(p_root_item->entry_lock[event_idx]);
	return rt;
}

INT32 sie_event_proc_isr(UINT32 handle, UINT32 event, void *p_in, void *p_out)
{
	SIE_EVENT_ROOT_ITEM *p_root_item;
	CTL_SIE_LIST_HEAD *p_list, *n;
	SIE_EVENT_ENTRY_ITEM *p_entry_item;
	UINT32 event_idx;
	INT32 rt = 0;

	if (!(event & CTL_SIE_EVENT_ISR_TAG)) {
		ctl_sie_dbg_err("can't process non-isr evt(0x%.8x)\r\n", event);
		return rt;
	}

	event_idx = (event & (~SIE_EVENT_EVENT_CTL_MASK));
	if (event_idx >= SIE_EVENT_ENTRY_MAX) {
		ctl_sie_dbg_err("evt idx(%d) ovf\r\n", (int)(event_idx));
		return rt;
	}

	p_root_item = (SIE_EVENT_ROOT_ITEM *)handle;
	if (p_root_item->sts & SIE_EVENT_STS_FREE) {
		ctl_sie_dbg_err("err hdl(0x%.8x)\r\n", handle);
		return rt;
	}
	vos_list_for_each_safe(p_list, n, &p_root_item->entry_list[event_idx]) {
		p_entry_item = vos_list_entry(p_list, SIE_EVENT_ENTRY_ITEM, list);
		if (p_entry_item->fp != NULL) {
			p_entry_item->enter_ts = (UINT32)clock();
			rt = p_entry_item->fp(event, p_in, p_out);
			p_entry_item->exit_ts = (UINT32)clock();

			if (p_entry_item->sts & CTL_SIE_EVENT_ONE_SHOT) {
				/* one shot flow */
				vos_list_del_init(&p_entry_item->list);
				__sie_event_entry_item_unlock_isr(p_entry_item);
			}
		}
	}
	return rt;
}

