/**
    IPP event.

    @file       ipp_event.c
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "ipp_debug_int.h"
#include "ipp_event_int.h"
#include "ipp_event_id_int.h"
#include "ctl_ipp_util_int.h"
#include "kwrap/error_no.h"

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

static UINT32 ipp_event_flg_num;
static ID *ipp_event_flg_id;

static UINT32 event_root_num;
static IPP_EVENT_ROOT_ITEM *event_root_pool;
static UINT32 event_entry_item_num;
static IPP_EVENT_ENTRY_ITEM *event_entry_item_pool;

static CTL_IPP_LIST_HEAD event_free_root_head;
static CTL_IPP_LIST_HEAD event_free_entry_item_head;
#if 0
#endif

static IPP_EVENT_LOCK_INFO ipp_event_get_lock_info(UINT32 lock)
{
	IPP_EVENT_LOCK_INFO rt_info;

	rt_info.flag = (UINT32)(lock / CTL_IPP_UTIL_FLAG_PAGE_SIZE);
	rt_info.ptn = 1 << (lock - (rt_info.flag * CTL_IPP_UTIL_FLAG_PAGE_SIZE));

	return rt_info;
}

static void ipp_event_free_pool_lock(void)
{
	FLGPTN flag;

	vos_flag_wait(&flag, ipp_event_flg_id[0], IPP_EVENT_POOL_LOCK, TWF_CLR);
}

static void ipp_event_free_pool_unlock(void)
{
	vos_flag_set(ipp_event_flg_id[0], IPP_EVENT_POOL_LOCK);
}

static void ipp_event_lock(UINT32 lock)
{
	FLGPTN flag;
	IPP_EVENT_LOCK_INFO lock_info;

	lock_info = ipp_event_get_lock_info(lock);

	vos_flag_wait(&flag, ipp_event_flg_id[lock_info.flag], lock_info.ptn, TWF_CLR);
}

static void ipp_event_unlock(UINT32 lock)
{
	IPP_EVENT_LOCK_INFO lock_info;

	lock_info = ipp_event_get_lock_info(lock);

	vos_flag_set(ipp_event_flg_id[lock_info.flag], lock_info.ptn);
}

static UINT32 ipp_event_is_root_item(IPP_EVENT_ROOT_ITEM *p_root)
{
	UINT32 i;

	for (i = 0; i < event_root_num; i++) {
		if ((UINT32)p_root == (UINT32)&event_root_pool[i]) {
			return IPP_EVENT_OK;
		}
	}
	CTL_IPP_DBG_WRN("invalid handle (0x%.8x)\r\n", (unsigned int)p_root);
	return IPP_EVENT_NG;
}

static IPP_EVENT_ENTRY_ITEM *ipp_event_check_register_fp(IPP_EVENT_ROOT_ITEM *p_root, UINT32 event, IPP_EVENT_FP fp)
{
	CTL_IPP_LIST_HEAD *p_list;
	IPP_EVENT_ENTRY_ITEM *p_entry_item;

	vos_list_for_each(p_list, &p_root->entry_list[event]) {
		p_entry_item = vos_list_entry(p_list, IPP_EVENT_ENTRY_ITEM, list);
		if (p_entry_item->fp == fp) {
			return p_entry_item;
		}
	}
	return NULL;
}


static IPP_EVENT_ENTRY_ITEM *ipp_event_entry_item_lock(void)
{
	IPP_EVENT_ENTRY_ITEM *p_entry_item = NULL;

	ipp_event_free_pool_lock();

	//get free item
	if (!vos_list_empty(&event_free_entry_item_head)) {
		p_entry_item = vos_list_entry(event_free_entry_item_head.next, IPP_EVENT_ENTRY_ITEM, list);
		p_entry_item->sts &= ~IPP_EVENT_STS_FREE;
		vos_list_del(&p_entry_item->list);
	}

	ipp_event_free_pool_unlock();

	return p_entry_item;
}

static void ipp_event_entry_item_unlock(IPP_EVENT_ENTRY_ITEM *p_entry_item)
{
	ipp_event_free_pool_lock();

	/* clear status */
	p_entry_item->sts |= IPP_EVENT_STS_FREE;
	p_entry_item->sts &= ~IPP_EVENT_STS_CTL_MASK;
	p_entry_item->fp = NULL;

	/* add to free list */
	vos_list_add_tail(&p_entry_item->list, &event_free_entry_item_head);

	ipp_event_free_pool_unlock();
}

static void ipp_event_entry_item_unlock_isr(IPP_EVENT_ENTRY_ITEM *p_entry_item)
{
	/* clear status */
	p_entry_item->sts |= IPP_EVENT_STS_FREE;
	p_entry_item->sts &= ~IPP_EVENT_STS_CTL_MASK;
	p_entry_item->fp = NULL;

	/* add to free list */
	vos_list_add_tail(&p_entry_item->list, &event_free_entry_item_head);
}

#if 0
#endif
#if 0
void ipp_event_dump_free_list(void)
{
	CTL_IPP_LIST_HEAD *p_list;
	IPP_EVENT_ENTRY_ITEM *p_entry_item;
	IPP_EVENT_ROOT_ITEM *p_root_item;

	DBG_DUMP("-------- ipp event free root --------------------------\r\n");
	vos_list_for_each(p_list, &event_free_root_head) {
		p_root_item = vos_list_entry(p_list, IPP_EVENT_ROOT_ITEM, root_list);
		DBG_DUMP("0x%.8x\r\n", p_root_item->sts);
	}

	DBG_DUMP("-------- ipp event free entry --------------------------\r\n");
	vos_list_for_each(p_list, &event_free_entry_item_head) {
		p_entry_item = vos_list_entry(p_list, IPP_EVENT_ENTRY_ITEM, list);
		DBG_DUMP("0x%.8x\r\n", p_entry_item->sts);
	}
}
#endif

void ipp_event_dump(void)
{
	UINT32 i, j;
	CTL_IPP_LIST_HEAD *p_list;
	IPP_EVENT_ENTRY_ITEM *p_entry_item;


	DBG_DUMP("----------------- ipp event info -----------------\r\n");

	i = 0;
	vos_list_for_each(p_list, &event_free_root_head) {
		i += 1;
	}
	DBG_DUMP("max root num = %d (free = %d)\r\n", (int)(event_root_num), (int)(i));


	i = 0;
	vos_list_for_each(p_list, &event_free_entry_item_head) {
		i += 1;
	}
	DBG_DUMP("max entry item num = %d (free = %d)\r\n", (int)(event_entry_item_num), (int)(i));

	for (i = 0; i < ipp_event_flg_num; i++) {
		DBG_DUMP("flag id[%d] = %d ptn = 0x%.8x\r\n", (int)i, (int)ipp_event_flg_id[i], (unsigned int)vos_flag_chk(ipp_event_flg_id[i], FLGPTN_BIT_ALL));
	}

	for (i = 0; i < event_root_num; i++) {

		if (event_root_pool[i].sts & IPP_EVENT_STS_FREE) {
			continue;
		}
		DBG_DUMP("--------- root(%2d) %10s 0x%.8x ---------\r\n",
							(int)i, event_root_pool[i].name, (unsigned int)event_root_pool[i].sts);
		for (j = 0; j < IPP_EVENT_ENTRY_MAX; j++) {

			if (!vos_list_empty(&event_root_pool[i].entry_list[j])) {
				DBG_DUMP("entry       name  func addr     status   ProcT(us)\r\n");
				vos_list_for_each(p_list, &event_root_pool[i].entry_list[j]) {
					p_entry_item = vos_list_entry(p_list, IPP_EVENT_ENTRY_ITEM, list);
					DBG_DUMP("%5d %10s 0x%.8x 0x%.8x %11d\r\n", (int)j, p_entry_item->name,
						(unsigned int)p_entry_item->fp, (unsigned int)p_entry_item->sts, (int)(p_entry_item->exit_ts - p_entry_item->enter_ts));
				}
			}
		}
	}
}

void ipp_event_dump_proc_time(int (*dump)(const char *fmt, ...))
{
	UINT32 i, j;
	CTL_IPP_LIST_HEAD *p_list;
	IPP_EVENT_ENTRY_ITEM *p_entry_item;

	dump("---- IPP EVENT CALLBACK TIME ----\r\n");
	for (i = 0; i < event_root_num; i++) {

		if (event_root_pool[i].sts & IPP_EVENT_STS_FREE) {
			continue;
		}
		dump("--------- root(%2d) %10s 0x%.8x ---------\r\n",
							(int)i, event_root_pool[i].name, (unsigned int)event_root_pool[i].sts);
		for (j = 0; j < IPP_EVENT_ENTRY_MAX; j++) {

			if (!vos_list_empty(&event_root_pool[i].entry_list[j])) {
				dump("entry       name  func addr     status   ProcT(us)\r\n");
				vos_list_for_each(p_list, &event_root_pool[i].entry_list[j]) {
					p_entry_item = vos_list_entry(p_list, IPP_EVENT_ENTRY_ITEM, list);
					dump("%5d %10s 0x%.8x 0x%.8x %11d\r\n", (int)j, p_entry_item->name,
						(unsigned int)p_entry_item->fp, (unsigned int)p_entry_item->sts, (int)(p_entry_item->exit_ts - p_entry_item->enter_ts));
				}
			}
		}
	}
}

UINT32 ipp_event_init(UINT32 num, UINT32 buf_addr, UINT32 is_query)
{
	UINT32 i, j, entry_lock_val, root_lock_val, flg_num, buf_size;
	void *p_buf;

	/* allocate buffer
		1. root pool
		2. item pool
		3. flag pool
	*/
	flg_num = ALIGN_CEIL((num + (num * IPP_EVENT_ENTRY_MAX) + IPP_FREE_FLAG_START), CTL_IPP_UTIL_FLAG_PAGE_SIZE) / CTL_IPP_UTIL_FLAG_PAGE_SIZE;
	buf_size = (sizeof(IPP_EVENT_ROOT_ITEM) * num) + (sizeof(IPP_EVENT_ENTRY_ITEM) * num * IPP_EVENT_ENTRY_MAX) + (sizeof(ID) * flg_num);
	if (is_query) {
		ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_EVT", CTL_IPP_DBG_CTX_BUF_QUERY, buf_size, buf_addr, num);
		return buf_size;
	}

	p_buf = (void *)buf_addr;
	if (p_buf == NULL) {
		CTL_IPP_DBG_ERR("alloc ipp_event pool memory failed, num %d, want_size 0x%.8x\r\n", (int)num, (unsigned int)buf_size);
		return CTL_IPP_E_NOMEM;
	}
	CTL_IPP_DBG_TRC("alloc ipp_event pool memory, num %d, flg_num %d, size 0x%.8x, addr 0x%.8x\r\n", (int)num, (int)flg_num, (unsigned int)buf_size, (unsigned int)p_buf);

	event_root_num = num;
	event_entry_item_num = num * IPP_EVENT_ENTRY_MAX;
	ipp_event_flg_num = flg_num;

	event_root_pool = (IPP_EVENT_ROOT_ITEM *)p_buf;
	event_entry_item_pool = (IPP_EVENT_ENTRY_ITEM *)((UINT32)event_root_pool + sizeof(IPP_EVENT_ROOT_ITEM) * num);
	ipp_event_flg_id = (ID *)((UINT32)event_entry_item_pool + sizeof(IPP_EVENT_ENTRY_ITEM) * num * IPP_EVENT_ENTRY_MAX);

	/* reset & create free root pool list  */
	root_lock_val = IPP_FREE_FLAG_START;
	entry_lock_val = IPP_FREE_FLAG_START + event_root_num;

	VOS_INIT_LIST_HEAD(&event_free_root_head);
	for (i = 0; i < event_root_num; i++) {

		event_root_pool[i].name[0] = '\0';
		event_root_pool[i].sts = (i | IPP_EVENT_STS_FREE);
		event_root_pool[i].root_lock = root_lock_val;

		for (j = 0; j < IPP_EVENT_ENTRY_MAX; j++) {
			event_root_pool[i].entry_lock[j] = entry_lock_val;
			VOS_INIT_LIST_HEAD(&event_root_pool[i].entry_list[j]);

			entry_lock_val += 1;
		}

		vos_list_add_tail(&event_root_pool[i].root_list, &event_free_root_head);
		root_lock_val += 1;

	}

	/* reset & create free entry item pool list  */
	VOS_INIT_LIST_HEAD(&event_free_entry_item_head);
	for (i = 0; i < event_entry_item_num; i++) {
		event_entry_item_pool[i].sts = (i | IPP_EVENT_STS_FREE);
		event_entry_item_pool[i].fp = NULL;
		vos_list_add_tail(&event_entry_item_pool[i].list, &event_free_entry_item_head);
	}

	//reset free flag
	for (i = 0; i < ipp_event_flg_num; i++) {
		OS_CONFIG_FLAG(ipp_event_flg_id[i]);
		if (i == 0) {
			vos_flag_set(ipp_event_flg_id[i], (FLGPTN_BIT_ALL & ~(IPP_EVENT_POOL_LOCK)));
		} else {
			vos_flag_set(ipp_event_flg_id[i], FLGPTN_BIT_ALL);
		}
	}
	vos_flag_set(ipp_event_flg_id[0], IPP_EVENT_POOL_LOCK);

	ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_EVT", CTL_IPP_DBG_CTX_BUF_ALLOC, buf_size, buf_addr, num);

	return buf_size;
}

void ipp_event_uninit(void)
{
	UINT32 i;

	if (event_root_pool != NULL) {
		CTL_IPP_DBG_TRC("free ipp_event memory, addr 0x%.8x\r\n", (unsigned int)event_root_pool);
		ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_EVT", CTL_IPP_DBG_CTX_BUF_FREE, 0, (UINT32)event_root_pool, 0);

		for (i = 0; i < ipp_event_flg_num; i++) {
			rel_flg(ipp_event_flg_id[i]);
		}

		event_root_num = 0;
		event_entry_item_num = 0;
		ipp_event_flg_num = 0;

		event_root_pool = NULL;
		event_entry_item_pool = NULL;
		ipp_event_flg_id = NULL;
	}
}

UINT32 ipp_event_open(CHAR *name)
{
	UINT32 handle = 0;
	IPP_EVENT_ROOT_ITEM *p_root_item;

	ipp_event_free_pool_lock();

	if (vos_list_empty(&event_free_root_head)) {
		CTL_IPP_DBG_ERR("open fail\r\n");
	} else {
		p_root_item = vos_list_entry(event_free_root_head.next, IPP_EVENT_ROOT_ITEM, root_list);
		vos_list_del(&p_root_item->root_list);

		/* update root status */
		ipp_event_lock(p_root_item->root_lock);
		p_root_item->name[0] = '\0';
		memset((void *)&p_root_item->name[0], '\0', sizeof(CHAR) * IPP_EVENT_NAME_MAX);
		if (name != NULL) {
			strncpy(p_root_item->name, name, (IPP_EVENT_NAME_MAX - 1));
		}
		p_root_item->sts &= ~(IPP_EVENT_STS_FREE|IPP_EVENT_STS_CTL_MASK);
		ipp_event_unlock(p_root_item->root_lock);

		handle = (UINT32)p_root_item;
	}

	ipp_event_free_pool_unlock();
	return handle;
}

UINT32 ipp_event_close(UINT32 handle)
{
	unsigned long loc_cpu_flg;
	IPP_EVENT_ROOT_ITEM *p_root_item;
	CTL_IPP_LIST_HEAD *p_list, *n;
	IPP_EVENT_ENTRY_ITEM *p_entry_item;
	UINT32 i;

	p_root_item = (IPP_EVENT_ROOT_ITEM *)handle;

	if (p_root_item->sts & IPP_EVENT_STS_FREE) {
		CTL_IPP_DBG_ERR("error handle(0x%.8x)\r\n", (unsigned int)handle);
		return IPP_EVENT_NG;
	}

	if (ipp_event_is_root_item(p_root_item) == IPP_EVENT_NG) {
		CTL_IPP_DBG_ERR("close fail (0x%.8x)\r\n", (unsigned int)handle);
		return IPP_EVENT_NG;
	}

	ipp_event_lock(p_root_item->root_lock);
	p_root_item->sts |= IPP_EVENT_STS_FREE;
	p_root_item->sts &= ~IPP_EVENT_STS_CTL_MASK;
	p_root_item->name[0] = '\0';
	ipp_event_unlock(p_root_item->root_lock);

	ipp_event_free_pool_lock();

	/* flush entry item */
	for (i = 0; i < IPP_EVENT_ENTRY_MAX; i++) {

		vos_list_for_each_safe(p_list, n, &p_root_item->entry_list[i]) {

			p_entry_item = vos_list_entry(p_list, IPP_EVENT_ENTRY_ITEM, list);
			if (p_entry_item->sts & IPP_EVENT_ISR_TAG) {
				loc_cpu(loc_cpu_flg);
				vos_list_del(&p_entry_item->list);
				unl_cpu(loc_cpu_flg);
			} else {
				vos_list_del(&p_entry_item->list);
			}

			/* clear status */
			p_entry_item->sts |= IPP_EVENT_STS_FREE;
			p_entry_item->sts &= ~IPP_EVENT_STS_CTL_MASK;
			p_entry_item->fp = NULL;
			/* add to free list */
			vos_list_add_tail(&p_entry_item->list, &event_free_entry_item_head);
		}

	}

	vos_list_add_tail(&p_root_item->root_list, &event_free_root_head);

	ipp_event_free_pool_unlock();
	return IPP_EVENT_OK;
}

UINT32 ipp_event_register(UINT32 handle, UINT32 event, IPP_EVENT_FP fp, CHAR *name)
{
	unsigned long loc_cpu_flg;
	IPP_EVENT_ROOT_ITEM *p_root_item;
	IPP_EVENT_ENTRY_ITEM *p_entry_item;
	UINT32 event_idx, rt;

	event_idx = (event & (~IPL_EVENT_EVENT_CTL_MASK));
	if (event_idx >= IPP_EVENT_ENTRY_MAX) {
		CTL_IPP_DBG_ERR("event_idx(%d) overlfow\r\n", (int)(event_idx));
		return IPP_EVENT_NG;
	}

	p_root_item = (IPP_EVENT_ROOT_ITEM *)handle;
	if (p_root_item->sts & IPP_EVENT_STS_FREE) {
		CTL_IPP_DBG_ERR("error handle(0x%.8x)\r\n", (unsigned int)handle);
		return IPP_EVENT_NG;
	}

	if (fp == NULL) {
		return IPP_EVENT_NG;
	}

	ipp_event_lock(p_root_item->entry_lock[event_idx]);
	rt = IPP_EVENT_OK;
	if (ipp_event_check_register_fp(p_root_item, event_idx, fp) == NULL) {

		p_entry_item = ipp_event_entry_item_lock();
		if (p_entry_item != NULL) {
			p_entry_item->fp = fp;
			p_entry_item->sts |= (event & IPL_EVENT_EVENT_CTL_MASK);
			p_entry_item->enter_ts = 0;
			p_entry_item->exit_ts = 0;

			memset((void *)&p_entry_item->name[0], '\0', sizeof(CHAR) * IPP_EVENT_NAME_MAX);
			if (name != NULL) {
				strncpy(p_entry_item->name, name, (IPP_EVENT_NAME_MAX - 1));
			}

			if (p_entry_item->sts & IPP_EVENT_ISR_TAG) {
				loc_cpu(loc_cpu_flg);
				vos_list_add_tail(&p_entry_item->list, &p_root_item->entry_list[event_idx]);
				unl_cpu(loc_cpu_flg);
			} else {
				vos_list_add_tail(&p_entry_item->list, &p_root_item->entry_list[event_idx]);
			}
			rt = E_OK;
		} else {
			CTL_IPP_DBG_ERR("register fail(0x%.8x %d 0x%.8x)\r\n", (unsigned int)handle, (int)event, (unsigned int)fp);
		}
	}

	ipp_event_unlock(p_root_item->entry_lock[event_idx]);
	return rt;
}

UINT32 ipl_event_unregister(UINT32 handle, UINT32 event, IPP_EVENT_FP fp)
{
	unsigned long loc_cpu_flg;
	UINT32 rt;
	IPP_EVENT_ROOT_ITEM *p_root_item;
	IPP_EVENT_ENTRY_ITEM *p_entry_item;
	UINT32 event_idx;

	event_idx = (event & (~IPL_EVENT_EVENT_CTL_MASK));
	if (event_idx >= IPP_EVENT_ENTRY_MAX) {
		CTL_IPP_DBG_ERR("event_idx(%d) overlfow\r\n", (int)(event_idx));
		return IPP_EVENT_NG;
	}

	p_root_item = (IPP_EVENT_ROOT_ITEM *)handle;
	if (p_root_item->sts & IPP_EVENT_STS_FREE) {
		CTL_IPP_DBG_ERR("error handle(0x%.8x)\r\n", (unsigned int)handle);
		return IPP_EVENT_NG;
	}

	if (fp == NULL) {
		return IPP_EVENT_NG;
	}

	ipp_event_lock(p_root_item->entry_lock[event_idx]);
	rt = IPP_EVENT_OK;

	p_entry_item = ipp_event_check_register_fp(p_root_item, event_idx, fp);
	if (p_entry_item != NULL) {
		//remove from list
		if (p_entry_item->sts & IPP_EVENT_ISR_TAG) {
			loc_cpu(loc_cpu_flg);
			vos_list_del(&p_entry_item->list);
			unl_cpu(loc_cpu_flg);
		} else {
			vos_list_del(&p_entry_item->list);
		}

		//add to free list
		ipp_event_entry_item_unlock(p_entry_item);
		rt = E_OK;
	}
	ipp_event_unlock(p_root_item->entry_lock[event_idx]);
	return rt;
}

INT32 ipp_event_proc(UINT32 handle, UINT32 event, void *p_in, void *p_out)
{
	IPP_EVENT_ROOT_ITEM *p_root_item;
	CTL_IPP_LIST_HEAD *p_list, *n;
	IPP_EVENT_ENTRY_ITEM *p_entry_item;
	UINT32 event_idx;
	INT32 rt = 0;

	if (event & IPP_EVENT_ISR_TAG) {
		CTL_IPP_DBG_WRN("can't process isr event(0x%.8x)\r\n", (unsigned int)event);
		return rt;
	}

	event_idx = (event & (~IPL_EVENT_EVENT_CTL_MASK));
	if (event_idx >= IPP_EVENT_ENTRY_MAX) {
		CTL_IPP_DBG_WRN("event_idx(%d) overlfow\r\n", (int)(event_idx));
		return rt;
	}

	p_root_item = (IPP_EVENT_ROOT_ITEM *)handle;
	if (p_root_item->sts & IPP_EVENT_STS_FREE) {
		CTL_IPP_DBG_WRN("error handle(0x%.8x)\r\n", (unsigned int)handle);
		return rt;
	}

	ipp_event_lock(p_root_item->entry_lock[event_idx]);

	vos_list_for_each_safe(p_list, n, &p_root_item->entry_list[event_idx]) {
		p_entry_item = vos_list_entry(p_list, IPP_EVENT_ENTRY_ITEM, list);

		if (p_entry_item->fp != NULL) {
			p_entry_item->enter_ts = (UINT32)ctl_ipp_util_get_syst_counter();
			rt = p_entry_item->fp(event, p_in, p_out);
			p_entry_item->exit_ts = (UINT32)ctl_ipp_util_get_syst_counter();

			/* one shot flow */
			if (p_entry_item->sts & IPP_EVENT_ONE_SHOT) {

				vos_list_del(&p_entry_item->list);
				ipp_event_entry_item_unlock(p_entry_item);
			}
		}
	}

	ipp_event_unlock(p_root_item->entry_lock[event_idx]);
	return rt;
}

INT32 ipp_event_proc_isr(UINT32 handle, UINT32 event, void *p_in, void *p_out)
{
	IPP_EVENT_ROOT_ITEM *p_root_item;
	CTL_IPP_LIST_HEAD *p_list, *n;
	IPP_EVENT_ENTRY_ITEM *p_entry_item;
	UINT32 event_idx;
	INT32 rt = 0;

	if (!(event & IPP_EVENT_ISR_TAG)) {
		CTL_IPP_DBG_WRN("can't process non-isr event(0x%.8x)\r\n", (unsigned int)event);
		return rt;
	}

	event_idx = (event & (~IPL_EVENT_EVENT_CTL_MASK));
	if (event_idx >= IPP_EVENT_ENTRY_MAX) {
		CTL_IPP_DBG_WRN("event_idx(%d) overlfow\r\n", (int)(event_idx));
		return rt;
	}

	p_root_item = (IPP_EVENT_ROOT_ITEM *)handle;
	if (p_root_item->sts & IPP_EVENT_STS_FREE) {
		CTL_IPP_DBG_WRN("error handle(0x%.8x)\r\n", (unsigned int)handle);
		return rt;
	}

	vos_list_for_each_safe(p_list, n, &p_root_item->entry_list[event_idx]) {
		p_entry_item = vos_list_entry(p_list, IPP_EVENT_ENTRY_ITEM, list);
		if (p_entry_item->fp != NULL) {
			p_entry_item->enter_ts = (UINT32)ctl_ipp_util_get_syst_counter();
			rt = p_entry_item->fp(event, p_in, p_out);
			p_entry_item->exit_ts = (UINT32)ctl_ipp_util_get_syst_counter();

			if (p_entry_item->sts & IPP_EVENT_ONE_SHOT) {
				/* one shot flow */
				vos_list_del(&p_entry_item->list);
				ipp_event_entry_item_unlock_isr(p_entry_item);
			}
		}
	}

	return rt;
}

