#ifdef __KERNEL__
//#include <linux/cdev.h>
//#include <linux/types.h>
#include <mach/fmem.h>
#elif defined(__FREERTOS)
#include <string.h>
#endif
#include "kdrv_jpeg_queue.h"
#include "jpeg_dbg.h"


static KDRV_JPEG_QUEUE_INFO g_jpeg_send_queue;


INT32 kdrv_jpeg_queue_init_p(void)
{
	memset(&g_jpeg_send_queue, 0x00, sizeof(g_jpeg_send_queue));
	return 0;
}


INT32 kdrv_jpeg_queue_exit_p(void)
{
	return 0;
}

KDRV_JPEG_QUEUE_INFO *kdrv_jpeg_get_queue_by_coreid(void)
{
	return &g_jpeg_send_queue;
}


INT32 kdrv_jpeg_queue_is_empty_p(KDRV_JPEG_QUEUE_INFO *p_queue)
{
	if (p_queue->in_idx == p_queue->out_idx) {
		return 1;
	} else {
		return 0;
	}
}

INT32 kdrv_jpeg_queue_add_p(KDRV_JPEG_QUEUE_INFO *p_queue, KDRV_JPEG_QUEUE_ELEMENT *element)
{
	UINT32              in_next_idx;

	//printk("in_idx=%d \r\n", p_queue->in_idx);

	in_next_idx = p_queue->in_idx + 1;

	if (in_next_idx >= KDRV_JPEG_QUEUE_ELEMENT_NUM) {
		in_next_idx -= KDRV_JPEG_QUEUE_ELEMENT_NUM;
	}
	if (in_next_idx == p_queue->out_idx) {
		return -3;
		//return KDRV_RPC_ER_QUEUE_FULL;
	}
	p_queue->element[p_queue->in_idx] = *element ;
	p_queue->in_idx = in_next_idx;

	//return KDRV_RPC_ER_OK;
	return 0;
}

INT32 kdrv_jpeg_queue_del_p(KDRV_JPEG_QUEUE_INFO *p_queue, KDRV_JPEG_QUEUE_ELEMENT *element)
{
#if 1
	//printk("out_idx=%d \r\n", p_queue->out_idx);

	*element = p_queue->element[p_queue->out_idx];
	p_queue->out_idx++;
	if (p_queue->out_idx >= KDRV_JPEG_QUEUE_ELEMENT_NUM) {
		p_queue->out_idx -= KDRV_JPEG_QUEUE_ELEMENT_NUM;
	}
#endif
	//return KDRV_RPC_ER_OK;
	return 0;

}




