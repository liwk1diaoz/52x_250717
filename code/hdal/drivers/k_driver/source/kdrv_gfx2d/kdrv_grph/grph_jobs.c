#include "kwrap/type.h"
#include "grph_platform.h"
#include "grph_int.h"
#if defined(__FREERTOS)
#include <stdlib.h>

#define ARRAY_SIZE(X) (sizeof((X)) / sizeof((X[0])))
#define EXPORT_SYMBOL(x)
#endif

#define GRPH_REQ_POLL_SIZE	4	// non-blockding depth
static GRPH_REQ_LIST_NODE g_req_pool[GRPH_ID_2+1][GRPH_REQ_POLL_SIZE];
static UINT32 v_req_front[GRPH_ID_2+1];
static UINT32 v_req_tail[GRPH_ID_2+1];

/*
	Cp contents from KDRV_GRPH_TRIGGER_PARAM to GRPH_REQ_LL_NODE

	A GRPH_REQ_LIST_NODE consists multiple GRPH_REQ_LL_NODE.
	This function help param_to_queue_element() to backup
	KDRV_GRPH_TRIGGER_PARAM into GRPH_REQ_LL_NODE.

	KDRV_GRPH_TRIGGER_PARAM: structure passed from kdrv_grph_trigger()
	GRPH_REQ_LL_NODE: internal structure to ll node in a queue jobs
*/
static ER param_to_ll_node(KDRV_GRPH_TRIGGER_PARAM *p_param,
				GRPH_REQ_LL_NODE *p_node)
{
	UINT32 i, j;
	GRPH_IMG *p_img;
	GRPH_IMG *p_pre_img;
	GRPH_PROPERTY *p_property;
	GRPH_PROPERTY *p_pre_property;

	memcpy(&p_node->trig_param, p_param, sizeof(KDRV_GRPH_TRIGGER_PARAM));

	i = 0;
	j = 0;
	p_pre_img = NULL;
	p_img = p_param->p_images;
	if (p_node->trig_param.p_images) {
		p_node->trig_param.p_images = &p_node->imges[0];
	}
	while ((p_img != NULL) &&
		(i < ARRAY_SIZE(p_node->imges)) &&
		(j < ARRAY_SIZE(p_node->inout_ops))) {
		GRPH_INOUTOP *p_inout;
		GRPH_INOUTOP *p_pre_inout;

		if (p_pre_img) {
			p_pre_img->p_next = &p_node->imges[i];
		}
		p_pre_img = &p_node->imges[i];
		memcpy(&p_node->imges[i], p_img, sizeof(GRPH_IMG));

		p_pre_inout = NULL;
		p_inout = p_img->p_inoutop;
		if (p_node->imges[i].p_inoutop) {
			p_node->imges[i].p_inoutop = &p_node->inout_ops[j];
		}
		while (p_inout && (j < ARRAY_SIZE(p_node->inout_ops))) {
			if (p_pre_inout) {
				p_pre_inout->p_next = &p_node->inout_ops[j];
			}
			p_pre_inout = &p_node->inout_ops[j];
			memcpy(&p_node->inout_ops[j], p_inout,
				sizeof(GRPH_INOUTOP));

			p_inout = p_inout->p_next;
			j++;
		}

		p_img = p_img->p_next;
		i++;
	}
	if (p_img != NULL) {
		// still have data not copied
		DBG_ERR("%s: img exceed\r\n", __func__);
		return E_SYS;
	}

	// property
	i = 0;
	p_pre_property = NULL;
	p_property = p_param->p_property;
	if (p_node->trig_param.p_property) {
		p_node->trig_param.p_property = &p_node->properties[0];
	}
	while ((p_property != NULL) &&
		(i < ARRAY_SIZE(p_node->properties))) {
		if (p_pre_property) {
			p_pre_property->p_next = &p_node->properties[i];
		}
		p_pre_property = &p_node->properties[i];

		memcpy(&p_node->properties[i], p_property,
			sizeof(GRPH_PROPERTY));
		if (p_property->id == GRPH_PROPERTY_ID_QUAD_PTR) {
			if (p_property->property != 0) {
				memcpy(&p_node->quad_desc,
					(void*)p_property->property,
					sizeof(GRPH_QUAD_DESC));
				p_node->properties[i].property =
					(UINT32)(&p_node->quad_desc);
			}
		}

		p_property = p_property->p_next;
		i++;
	}
	if (p_property != NULL) {
		// still have data not copied
		DBG_ERR("%s: property exceed\r\n", __func__);
		return E_SYS;
	}

	// color key filter
	if (p_param->p_ckeyfilter) {
		memcpy(&p_node->ckeyfilter, p_param->p_ckeyfilter,
			sizeof(GRPH_CKEYFILTER));
		p_node->trig_param.p_ckeyfilter = &p_node->ckeyfilter;
	}

	return E_OK;
}

/*
	Cp contents from KDRV_GRPH_TRIGGER_PARAM to GRPH_REQ_LIST_NODE

	Because pointer in GRPH_REQ_LIST_NODE may points to stack of caller,
	these data may be not accessed by graphic ISR.
	Thus we should copy data pointed by pointer.

	KDRV_GRPH_TRIGGER_PARAM: structure passed from kdrv_grph_trigger()
	GRPH_REQ_LIST_NODE: internal structure to queue jobs
*/
static ER param_to_queue_element(KDRV_GRPH_TRIGGER_PARAM *p_param,
				GRPH_REQ_LIST_NODE *p_node)
{
    UINT32 next_ll = 0;
    KDRV_GRPH_TRIGGER_PARAM *p_next_param = p_param;

    if (p_next_param == NULL) return -1;

    do {
        if (next_ll >= ARRAY_SIZE(p_node->v_ll_req)) {
            DBG_ERR("%s: p_param's link-list len too long: %d\r\n", __func__, (int)next_ll);
            return E_SYS;
        }
        if (param_to_ll_node(p_next_param, &p_node->v_ll_req[next_ll]) != E_OK) {
            return E_SYS;
        }

	if ((next_ll+1) == ARRAY_SIZE(p_node->v_ll_req)) {
		p_node->v_ll_req[next_ll].trig_param.p_next = NULL;
	} else {
	        p_node->v_ll_req[next_ll].trig_param.p_next =
	                    &(p_node->v_ll_req[next_ll+1].trig_param);
	}

        next_ll++;
        p_next_param = p_next_param->p_next;
    } while (p_next_param != NULL);

    // finalize ll queue
    p_node->v_ll_req[next_ll-1].trig_param.p_next = NULL;

    return E_OK;
}

/*
        Check if service queue is empty
*/
BOOL grph_platform_list_empty(GRPH_ID id)
{
	if (id > GRPH_ID_2) {
                DBG_ERR("%s: invalid id %d\r\n", __func__, (int)id);
                return E_SYS;
        }

	if (v_req_front[id] == v_req_tail[id]) {
		// queue empty
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
    Add request descriptor to service queue
*/
ER grph_platform_add_list(GRPH_ID id, KDRV_GRPH_TRIGGER_PARAM *p_param,
			KDRV_CALLBACK_FUNC *p_callback)
{
	UINT32 next;
	const UINT32 tail = v_req_tail[id];

	if (id > GRPH_ID_2) {
		DBG_ERR("%s: invalid id %d\r\n", __func__, (int)id);
		return E_SYS;
	}

	next = (tail+1) % GRPH_REQ_POLL_SIZE;
//	printk("%s: next %d\r\n", __func__, next);

	if (next == v_req_front[id]) {
		// queue full
		DBG_ERR("%s: queue full, front %d, tail %d\r\n", __func__,
			(int)v_req_front[id], (int)tail);
		return E_SYS;
	}

	if (param_to_queue_element(p_param, &g_req_pool[id][tail]) != E_OK)
		return E_SYS;

	if (p_callback) {
		memcpy(&g_req_pool[id][tail].callback,
			p_callback,
			sizeof(KDRV_CALLBACK_FUNC));
	} else {
		memset(&g_req_pool[id][tail].callback,
                        0,
                        sizeof(KDRV_CALLBACK_FUNC));
	}

	v_req_tail[id] = next;

        return E_OK;
}

/*
	Get head request descriptor from service queue
*/
GRPH_REQ_LIST_NODE* grph_platform_get_head(GRPH_ID id)
{
	GRPH_REQ_LIST_NODE *p_node;

	p_node = &g_req_pool[id][v_req_front[id]];

	if (id > GRPH_ID_2) {
		DBG_ERR("%s: invalid id %d\r\n", __func__, id);
		return NULL;
	}

	if (v_req_front[id] == v_req_tail[id]) {
		// queue empty
		DBG_ERR("%s: queue empty\r\n", __func__);
		return NULL;
	}

	return p_node;
}

/*
	Delete request descriptor from service queue
*/
ER grph_platform_del_list(GRPH_ID id)
{
	if (id > GRPH_ID_2) {
		DBG_ERR("%s: invalid id %d\r\n", __func__, id);
		return E_SYS;
	}

	if (v_req_front[id] == v_req_tail[id]) {
		DBG_ERR("%s: queue already empty, front %d, tail %d\r\n",
			__func__, (int)v_req_front[id], (int)v_req_tail[id]);
		return E_SYS;
	}

	v_req_front[id] = (v_req_front[id]+1) % GRPH_REQ_POLL_SIZE;

	return E_OK;
}

/*
	Reset jobs API internal resource
*/
void grph_jobs_init(void)
{
	v_req_front[GRPH_ID_1] = 0;
	v_req_front[GRPH_ID_2] = 0;
	v_req_tail[GRPH_ID_1] = 0;
	v_req_tail[GRPH_ID_2] = 0;
}
