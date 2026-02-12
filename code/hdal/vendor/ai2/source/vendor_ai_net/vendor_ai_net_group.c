/**
	@brief Source file of vendor ai net group.

	@file vendor_ai_net_group.c

	@ingroup vendor_ai_net

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "hd_type.h"
#include "hd_common.h"
#include "vendor_ai_net_flow.h"
#include "vendor_ai_net_gen.h"
#include "vendor_ai_net_mem.h"
#include "vendor_ai_net_group.h"
#include "vendor_ai_net_debug.h"
#include "kflow_ai_net/kflow_ai_net_comm.h"
#include "kflow_ai_net/nn_net.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
extern UINT32 g_ai_support_net_max;
#define MIN_LL_CMD_US		20
#define CNN_ENGINE_CLK		600
#define NUE_ENGINE_CLK		600
#define NUE2_ENGINE_CLK		480

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
/*-----------------------------------------------------------------------------*/
#define DBG_ERR(fmtstr, args...) printf("\033[0;31mERR:%s() \033[0m" fmtstr, __func__, ##args)
#define DBG_WRN(fmtstr, args...) printf("\033[0;33mWRN:%s() \033[0m" fmtstr, __func__, ##args)
#define DBG_DUMP(fmtstr, args...) printf(fmtstr, ##args)

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
extern BOOL _vendor_ai_net_is_linear_job(VENDOR_AI_NET_JOB_OPT job_opt);
extern BOOL _vendor_ai_net_is_graph_job(VENDOR_AI_NET_JOB_OPT job_opt);

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
#if (NET_GRP_HEAP == 1)
#else
VENDOR_AI_NET_GROUP_MEM *g_ai_net_group_priv = {0};
#endif

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

UINT32 vendor_ai_net_group_calcbuffersize(UINT32 proc_id, UINT32 user_parm_addr)
{
#if (NET_GRP_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(p_proc->group_priv);
#else
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(g_ai_net_group_priv[proc_id]);
#endif
	NN_GEN_NET_INFO net_info = {0};
	UINT32 mode_ctrl_num = 0;
	UINT32 total_size = 0;

	if (user_parm_addr == 0) {
		DBG_ERR("user_parm_addr is NULL\r\n");
		return 0;
	}

	if (vendor_ais_get_net_info(&net_info, user_parm_addr) != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail\r\n");
		return 0;
	}
	mode_ctrl_num = net_info.p_head->mode_ctrl_num;

	p_group_priv->mctrl_entry_size = sizeof(VENDOR_AI_NET_MCTRL_ENTRY) * mode_ctrl_num;
	total_size += p_group_priv->mctrl_entry_size;
	p_group_priv->llgroup_size = sizeof(VENDOR_AI_NET_LLGROUP) * mode_ctrl_num;
	total_size += p_group_priv->llgroup_size;
	p_group_priv->mem_entry_size = sizeof(VENDOR_AI_NET_MEM_ENTRY) * mode_ctrl_num * 2;
	total_size += p_group_priv->mem_entry_size;

	return total_size;

}

HD_RESULT vendor_ai_net_group_setbuffer(UINT32 proc_id, UINT32 working_buf_addr, UINT32 user_parm_addr)
{
#if (NET_GRP_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(p_proc->group_priv);
#else
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(g_ai_net_group_priv[proc_id]);
#endif
	UINT32 mctrl_entry_size = 0;
	UINT32 llgroup_size = 0;

	if (p_group_priv->mctrl_entry_size == 0) {
		DBG_ERR("mctrl_entry_size is zero\r\n");
		return HD_ERR_NG;
	}
	if (p_group_priv->llgroup_size == 0) {
		DBG_ERR("llgroup_size is zero\r\n");
		return HD_ERR_NG;
	}
	if (p_group_priv->mem_entry_size == 0) {
		DBG_ERR("mem_entry_size is zero\r\n");
		return HD_ERR_NG;
	}

	mctrl_entry_size = p_group_priv->mctrl_entry_size;
	llgroup_size = p_group_priv->llgroup_size;

	p_group_priv->mctrl_entry_addr = working_buf_addr;
	p_group_priv->llgroup_addr = p_group_priv->mctrl_entry_addr + mctrl_entry_size;
	p_group_priv->mem_entry_addr = p_group_priv->llgroup_addr + llgroup_size;
	p_group_priv->user_parm_addr = user_parm_addr;

	return HD_OK;
}

VOID vendor_ai_net_group_set_llcmd(UINT64 cmd, UINT64 addr, UINT64 *p_llcmd)
{
	UINT64 table = *p_llcmd & 0xff;
	*p_llcmd = ((cmd & 0xf) << 60) | ((addr & 0xffffffff) << 8) | (table & 0xff);
	return;
}

HD_RESULT vendor_ai_net_group_llcmd_fix(UINT32 proc_id)
{
#if (NET_GRP_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(p_proc->group_priv);
#else
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(g_ai_net_group_priv[proc_id]);
#endif
	HD_RESULT ret = HD_OK;
	UINT32 i = 0, group_num = 0, parm_addr = 0, parm_va_ofs = 0;
	VENDOR_AI_NET_LLGROUP *p_group = NULL;
	VENDOR_AI_NET_MCTRL_ENTRY *p_mctrl_entry = NULL;
	VENDOR_AI_LL_HEAD *p_llhead = NULL;
	UINT64 *p_ll_end_cmd = NULL;

	parm_va_ofs = p_group_priv->user_parm_addr;
	p_group = p_group_priv->group_list.p_group;
	if (p_group == NULL) {
		DBG_ERR("group_list.p_group is NULL\n");
		ret = HD_ERR_NG;
		goto exit;
	}
	group_num = p_group_priv->group_list.group_num;

	vendor_ai_net_trace(proc_id, AI_GRP, "llcmd_fix() - begin\r\n");

	for (i = 0; i < group_num; i++) {
		if ((p_group[i].addr == 0) || (p_group[i].cnt <= 1) || (p_group[i].trig_src != NN_GEN_TRIG_LL_AI_DRV)) {
			continue;
		}
		if (p_group[i].eng != NN_GEN_ENG_CNN && p_group[i].eng != NN_GEN_ENG_CNN2 &&
			p_group[i].eng != NN_GEN_ENG_NUE && p_group[i].eng != NN_GEN_ENG_NUE2) {
			continue;
		}
		p_group[i].p_head->idea_cycle = p_group[i].idea_cycle;
		p_ll_end_cmd = NULL;

		list_for_each_entry(p_mctrl_entry, &p_group[i].mctrl_listhead, list) {
			if (p_mctrl_entry->p_data->addr < parm_va_ofs) {
				p_llhead = (VENDOR_AI_LL_HEAD *)(p_mctrl_entry->p_data->addr + parm_va_ofs);
			} else {
				p_llhead = (VENDOR_AI_LL_HEAD *)p_mctrl_entry->p_data->addr;
			}
			parm_addr = p_llhead->parm_addr;
			if (parm_addr < parm_va_ofs) {
				parm_addr += parm_va_ofs;
			}
			if (p_ll_end_cmd) {
				vendor_ai_net_group_set_llcmd(VENDOR_AI_NET_LLCMD_NEXT_LL, p_llhead->parm_addr, p_ll_end_cmd);
			}
			p_ll_end_cmd = (UINT64 *)(parm_addr + p_llhead->parm_size - sizeof(UINT64));
		}
	}

exit:
	vendor_ai_net_trace(proc_id, AI_GRP, "llcmd_fix() - end\r\n");
	return ret;
}

HD_RESULT vendor_ai_net_group_linear(UINT32 proc_id)
{
#if (NET_GRP_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(p_proc->group_priv);
#else
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(g_ai_net_group_priv[proc_id]);
#endif
	HD_RESULT ret = HD_OK;
	UINT32 i = 0, j = 0, k = 0, mem_idx = 0, step_idx = 0, step_max = 1, group_num = 0;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head = NULL;
	NN_GEN_MODE_CTRL *p_mctrl = NULL;
	VENDOR_AI_NET_MCTRL_ENTRY *p_mctrl_entry = NULL;
	VENDOR_AI_NET_LLGROUP *p_group = NULL, *p_group_tmp = NULL;
	VENDOR_AI_NET_MEM_ENTRY *p_mem_entry = NULL;
	NN_GEN_ENG_TYPE curr_eng = AI_ENG_UNKNOWN;

	if (p_group_priv->user_parm_addr == 0) {
		DBG_ERR("user_parm_addr is NULL\r\n");
		ret = HD_ERR_NG;
		goto exit;
	}

	ret = vendor_ais_get_net_info(&net_info, p_group_priv->user_parm_addr);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail\r\n");
		goto exit;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	vendor_ai_net_trace(proc_id, AI_GRP, "linear() - begin\r\n");

	/* Create p_mctrl_entry: mctrl entry array */
	p_mctrl_entry = (VENDOR_AI_NET_MCTRL_ENTRY *)p_group_priv->mctrl_entry_addr;
	if (p_mctrl_entry == NULL) {
		DBG_ERR("mctrl_entry_addr is NULL\n");
		ret = HD_ERR_NG;
		goto exit;
	} else {
		memset(p_mctrl_entry, 0, p_group_priv->mctrl_entry_size);
		for (i = 0; i < p_head->mode_ctrl_num; i++) {
			INIT_LIST_HEAD(&p_mctrl_entry[i].list);
			p_mctrl_entry[i].p_data = &p_mctrl[i];
			p_mctrl_entry[i].mc_idx = i;
		}
	}

	/* Create p_group: ll cmd group array */
	p_group = (VENDOR_AI_NET_LLGROUP *)p_group_priv->llgroup_addr;
	if (p_group == NULL) {
		DBG_ERR("malloc llgroup fail\n");
		ret = HD_ERR_NG;
		goto exit;
	} else {
		memset(p_group, 0, p_group_priv->llgroup_size);
		for (i = 0; i < p_head->mode_ctrl_num; i++) {
			INIT_LIST_HEAD(&p_group[i].mctrl_listhead);
			p_group[i].g_idx = i;
		}
	}

	/* Create p_mem_entry: mem entry array */
	p_mem_entry = (VENDOR_AI_NET_MEM_ENTRY *)p_group_priv->mem_entry_addr;
	if (p_mem_entry == NULL) {
		DBG_ERR("malloc p_mem_entry fail\n");
		ret = HD_ERR_NG;
		goto exit;
	} else {
		memset(p_mem_entry, 0, p_group_priv->mem_entry_size);
	}

	/* Create p_mem_entry: mem entry array */
	p_mem_entry = (VENDOR_AI_NET_MEM_ENTRY *)p_group_priv->mem_entry_addr;
	if (p_mem_entry == NULL) {
		DBG_ERR("malloc p_mem_entry fail\n");
		ret = HD_ERR_NG;
		goto exit;
	} else {
		memset(p_mem_entry, 0, p_group_priv->mem_entry_size);
	}

	/* Set position of mctrl to p_mctrl_entry */
	for (i = 0; i < p_head->mode_ctrl_num; i++) {

		/* Head: if it is first layer */
		if (i == 0) {
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD);

		/* Tail: if it is last layer */
		} else if (i == p_head->mode_ctrl_num - 1) {
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL);

		/* Middle: the others */
		} else {
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_MIDDLE);
		}

		/* Head & Tail: if engine type is not hardware support */
		if ((p_mctrl[i].eng == NN_GEN_ENG_VPE || p_mctrl[i].eng == NN_GEN_ENG_CPU ||
			 p_mctrl[i].eng == NN_GEN_ENG_DSP)) {
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD);
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL);
		}

		/* Head: if engine type is differ from previous mctrl,
		 *       set previous layer to be Tail */
		if ((curr_eng != p_mctrl[i].eng) && i > 0) {
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD);
			j = i - 1;
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[j].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL);
		}
		curr_eng = p_mctrl[i].eng;
	}
	p_group_priv->mctrl_list.p_mctrl_entry = p_mctrl_entry;
	p_group_priv->mctrl_list.mctrl_num = p_head->mode_ctrl_num;

	/* Add mctrl to ll cmd group */
	for (i = 0; i < p_head->mode_ctrl_num; i++) {
		if (p_group[i].addr) {
			continue;
		}
		for (j = 0; j < p_head->mode_ctrl_num; j++) {

			/* Find Head mctrl entry */
			if (VENDOR_AI_NET_IS_BMP(p_mctrl_entry[j].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD) &&
				(p_mctrl_entry[j].p_group == NULL)) {
				p_group[i].addr = (UINT32)p_mctrl_entry[j].p_data;
				p_group[i].prev_num = 1;
				p_group[i].next_num = 1;
				p_group[i].p_head = p_mctrl_entry[j].p_data;
				p_group[i].eng = p_mctrl_entry[j].p_data->eng;
				p_group[i].trig_src = p_mctrl_entry[j].p_data->trig_src;
				p_group[i].step = i + 1;
				group_num++;
				step_max++;
				k = j;

				/* Find next mctrl entry to Tail */
				while (1) {
					list_add_tail(&p_mctrl_entry[k].list, &p_group[i].mctrl_listhead);
					p_mctrl_entry[k].p_group = &p_group[i];
					p_group[i].cnt++;
					p_group[i].idea_cycle += p_mctrl_entry[k].p_data->idea_cycle;
					if (VENDOR_AI_NET_IS_BMP(p_mctrl_entry[k].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL)) {
						p_group[i].p_tail = p_mctrl_entry[k].p_data;
						break;
					}
					k++;
				}
				break;
			}
		}
	}
	p_group_priv->group_list.p_group = p_group;
	p_group_priv->group_list.group_num = group_num;

	/* Fill p_mem_entry by group */
	mem_idx = 0;
	for (step_idx = 1; step_idx <= step_max; step_idx++) {
		for (i = 0; i < p_head->mode_ctrl_num; i++) {
			if (p_group[i].addr == 0) {
				continue;
			}
			if (p_group[i].step != step_idx) {
				continue;
			}
			/* Release previous group */
			if (i > 0) {
				j = i - 1;
				p_group_tmp = &p_group[j];
				if (p_group_tmp == NULL) {
					DBG_ERR("mctrl idx(%lu) is NOT grouping\r\n", j);
					ret = HD_ERR_NG;
					goto exit;
				}

				/* Set step_end_mid for in buffers */
				if (p_group_tmp->step_end == 0) {
					p_group_tmp->step_end = step_idx;
					p_mem_entry[mem_idx].p_group = p_group_tmp;
					p_mem_entry[mem_idx].is_alloc = 0;
					mem_idx++;
				}
			}

			/* Allocate group out buffer */
			p_mem_entry[mem_idx].p_group = &p_group[i];
			p_mem_entry[mem_idx].is_alloc = 1;
			mem_idx++;
		}
	}

	p_group_priv->mem_list.p_mem = p_mem_entry;
	p_group_priv->mem_list.mem_num = mem_idx;

exit:
	vendor_ai_net_trace(proc_id, AI_GRP, "linear() - end\r\n");
	return ret;
}

HD_RESULT vendor_ai_net_group_graph_optimize(UINT32 proc_id, UINT32 step_max)
{
#if (NET_GRP_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(p_proc->group_priv);
#else
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(g_ai_net_group_priv[proc_id]);
#endif
	HD_RESULT ret = HD_OK;
	UINT32 i = 0, step_idx = 0, group_num = 0, group_of_step = 0;
	UINT32 cycle_1st = 0, cycle_2nd = 0, cycle_cnn = 0;
	VENDOR_AI_NET_LLGROUP *group = NULL, *p_group = NULL, *p_group_tmp = NULL;
	VENDOR_AI_NET_MCTRL_ENTRY *p_entry = NULL, *p_entry_tmp = NULL;
	struct list_head cnn_listhead = {0}, nue_listhead = {0};
	struct list_head listhead_1st = {0}, listhead_2nd = {0};

	group = p_group_priv->group_list.p_group;
	group_num = p_group_priv->group_list.group_num;
	if (group == NULL) {
		DBG_ERR("llgroup is NULL\n");
		ret = HD_ERR_NG;
		goto exit;
	}
	if (step_max < 1) {
		DBG_ERR("step_max(%lu) < 1\n", step_max);
		ret = HD_ERR_NG;
		goto exit;
	}

	vendor_ai_net_trace(proc_id, AI_GRP, "optimize() - begin\r\n");

	for (step_idx = 1; step_idx <= step_max; step_idx++) {
		INIT_LIST_HEAD(&cnn_listhead);
		INIT_LIST_HEAD(&nue_listhead);
		INIT_LIST_HEAD(&listhead_1st);
		INIT_LIST_HEAD(&listhead_2nd);

		group_of_step = 0;
		cycle_cnn = 0;
		/* sort cycle into nue and cnn listhead */
		for (i = 0; i < group_num; i++) {
			if (group[i].addr == 0) {
				continue;
			}
			if (group[i].step != step_idx) {
				continue;
			}
			group_of_step++;

			if (group[i].eng == NN_GEN_ENG_NUE || group[i].eng == NN_GEN_ENG_NUE2) {
				if (list_empty(&nue_listhead)) {
					list_add_tail(&group[i].list, &nue_listhead);
				} else {
					list_for_each_entry(p_group, &nue_listhead, list) {
						if (group[i].idea_cycle >= p_group->idea_cycle) {
							break;
						}
					}
					list_add_tail(&group[i].list, &p_group->list);
				}
			}
			if (group[i].eng == NN_GEN_ENG_CNN || group[i].eng == NN_GEN_ENG_CNN2) {
				cycle_cnn += group[i].idea_cycle;
				if (list_empty(&cnn_listhead)) {
					list_add_tail(&group[i].list, &cnn_listhead);
				} else {
					list_for_each_entry(p_group, &cnn_listhead, list) {
						if (group[i].idea_cycle >= p_group->idea_cycle) {
							break;
						}
					}
					list_add_tail(&group[i].list, &p_group->list);
				}
			}
		}

		/* No need to shrink if it's only one group */
		if (group_of_step == 1) {
			continue;
		}

		/* No need to shrink if the process time is too small */
		if (cycle_cnn < (CNN_ENGINE_CLK * MIN_LL_CMD_US)) {

			/* merge cnn group */
			p_group_tmp = NULL;
			list_for_each_entry(p_group, &cnn_listhead, list) {
				if (p_group_tmp == NULL) {
					p_group_tmp = p_group;
					continue;
				}
				if (p_group_tmp == p_group) {
					continue;
				}
				list_for_each_entry_safe(p_entry, p_entry_tmp, &p_group->mctrl_listhead, list) {
					list_move_tail(&p_entry->list, &p_group_tmp->mctrl_listhead);
					p_entry->p_group = p_group_tmp;
					p_group_tmp->cnt++;
					p_group_tmp->idea_cycle += p_entry->p_data->idea_cycle;
				}
				p_group->addr = 0;
				p_group->cnt = 0;
			}
		} else {

			/* divide into 2 groups of step */
			cycle_1st = cycle_2nd = 0;
			list_for_each_entry_safe(p_group, p_group_tmp, &cnn_listhead, list) {
				if (cycle_1st < cycle_2nd) {
					list_move_tail(&p_group->list, &listhead_1st);
					cycle_1st += p_group->idea_cycle;
				} else {
					list_move_tail(&p_group->list, &listhead_2nd);
					cycle_2nd += p_group->idea_cycle;
				}
			}

			/* merge group */
			p_group_tmp = NULL;
			list_for_each_entry(p_group, &listhead_1st, list) {
				if (p_group_tmp == NULL) {
					p_group_tmp = p_group;
					continue;
				}
				if (p_group_tmp == p_group) {
					continue;
				}
				list_for_each_entry_safe(p_entry, p_entry_tmp, &p_group->mctrl_listhead, list) {
					list_move_tail(&p_entry->list, &p_group_tmp->mctrl_listhead);
					p_entry->p_group = p_group_tmp;
					p_group_tmp->cnt++;
					p_group_tmp->idea_cycle += p_entry->p_data->idea_cycle;
				}
				p_group->addr = 0;
				p_group->cnt = 0;
			}

			p_group_tmp = NULL;
			list_for_each_entry(p_group, &listhead_2nd, list) {
				if (p_group_tmp == NULL) {
					p_group_tmp = p_group;
					continue;
				}
				if (p_group_tmp == p_group) {
					continue;
				}
				list_for_each_entry_safe(p_entry, p_entry_tmp, &p_group->mctrl_listhead, list) {
					list_move_tail(&p_entry->list, &p_group_tmp->mctrl_listhead);
					p_entry->p_group = p_group_tmp;
					p_group_tmp->cnt++;
					p_group_tmp->idea_cycle += p_entry->p_data->idea_cycle;
				}
				p_group->addr = 0;
				p_group->cnt = 0;
			}
		}

		/* merge nue group */
		p_group_tmp = NULL;
		list_for_each_entry(p_group, &nue_listhead, list) {
			if (p_group_tmp == NULL) {
				p_group_tmp = p_group;
				continue;
			}
			if (p_group_tmp == p_group) {
				continue;
			}
			list_for_each_entry_safe(p_entry, p_entry_tmp, &p_group->mctrl_listhead, list) {
				list_move_tail(&p_entry->list, &p_group_tmp->mctrl_listhead);
				p_entry->p_group = p_group_tmp;
				p_group_tmp->cnt++;
				p_group_tmp->idea_cycle += p_entry->p_data->idea_cycle;
			}
			p_group->addr = 0;
			p_group->cnt = 0;
		}
	}

exit:
	vendor_ai_net_trace(proc_id, AI_GRP, "optimize() - end\r\n");
	return ret;
}

HD_RESULT vendor_ai_net_group_graph(UINT32 proc_id)
{
#if (NET_GRP_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(p_proc->group_priv);
#else
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(g_ai_net_group_priv[proc_id]);
#endif
	HD_RESULT ret = HD_OK;
	UINT32 i = 0, j = 0, k = 0, mem_idx = 0, step_idx = 0, step_max = 1, group_num = 0;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head = NULL;
	NN_GEN_MODE_CTRL *p_mctrl = NULL, *p_mctrl_tmp = NULL;
	VENDOR_AI_NET_MCTRL_ENTRY *p_mctrl_entry = NULL, *p_entry = NULL;
	VENDOR_AI_NET_LLGROUP *p_group = NULL, *p_group_tmp = NULL;
	VENDOR_AI_NET_MEM_ENTRY *p_mem_entry = NULL;
	UINT32 id_list_start_addr = 0, start_ofs = 0, end_ofs = 0;
	NN_GEN_ENG_TYPE curr_eng = AI_ENG_UNKNOWN;

	if (p_group_priv->user_parm_addr == 0) {
		DBG_ERR("user_parm_addr is NULL\r\n");
		ret = HD_ERR_NG;
		goto exit;
	}

	ret = vendor_ais_get_net_info(&net_info, p_group_priv->user_parm_addr);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail\r\n");
		goto exit;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	id_list_start_addr = (UINT32)net_info.p_id_list;

	vendor_ai_net_trace(proc_id, AI_GRP, "graph() - begin\r\n");

	/* Create p_mctrl_entry: mctrl entry array */
	p_mctrl_entry = (VENDOR_AI_NET_MCTRL_ENTRY *)p_group_priv->mctrl_entry_addr;
	if (p_mctrl_entry == NULL) {
		DBG_ERR("mctrl_entry_addr is NULL\n");
		ret = HD_ERR_NG;
		goto exit;
	} else {
		memset(p_mctrl_entry, 0, p_group_priv->mctrl_entry_size);
		for (i = 0; i < p_head->mode_ctrl_num; i++) {
			INIT_LIST_HEAD(&p_mctrl_entry[i].list);
			p_mctrl_entry[i].p_data = &p_mctrl[i];
			p_mctrl_entry[i].mc_idx = i;
		}
	}
	/* Create p_group: ll cmd group array */
	p_group = (VENDOR_AI_NET_LLGROUP *)p_group_priv->llgroup_addr;
	if (p_group == NULL) {
		DBG_ERR("malloc llgroup fail\n");
		ret = HD_ERR_NG;
		goto exit;
	} else {
		memset(p_group, 0, p_group_priv->llgroup_size);
		for (i = 0; i < p_head->mode_ctrl_num; i++) {
			INIT_LIST_HEAD(&p_group[i].list);
			INIT_LIST_HEAD(&p_group[i].mctrl_listhead);
			p_group[i].g_idx = i;
		}
	}

	/* Create p_mem_entry: mem entry array */
	p_mem_entry = (VENDOR_AI_NET_MEM_ENTRY *)p_group_priv->mem_entry_addr;
	if (p_mem_entry == NULL) {
		DBG_ERR("malloc p_mem_entry fail\n");
		ret = HD_ERR_NG;
		goto exit;
	} else {
		memset(p_mem_entry, 0, p_group_priv->mem_entry_size);
	}

	/* Set position of mctrl to p_mctrl_entry */
	for (i = 0; i < p_head->mode_ctrl_num; i++) {

		/* Head: if there is no previous layer
		 *       if there are multi next layer, set next layer to be Head */
		if (p_mctrl[i].prev_num == 0) {
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD);

			if (p_mctrl[i].next_num > 1) {
				VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL);

				start_ofs = p_mctrl[i].next_layer_idx_addr;
				end_ofs = start_ofs + sizeof(UINT32) * p_mctrl[i].next_num;
				for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
					k = *(UINT32 *)(id_list_start_addr + j);
					VENDOR_AI_NET_SET_BMP(p_mctrl_entry[k].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD);
				}
			}

		/* Tail: if there is no next layer
	     *       if there are multi previous layer, set previous layer to be Tail */
		} else if (p_mctrl[i].next_num == 0) {
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL);

			if (p_mctrl[i].prev_num > 1) {
				VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD);

				start_ofs = p_mctrl[i].prev_layer_idx_addr;
				end_ofs = start_ofs + sizeof(UINT32) * p_mctrl[i].prev_num;
				for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
					k = *(UINT32 *)(id_list_start_addr + j);
					VENDOR_AI_NET_SET_BMP(p_mctrl_entry[k].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL);
				}
			}

		/* Middle: if there is only one previous layer & only one next layer */
		} else if ((p_mctrl[i].prev_num == 1) && (p_mctrl[i].next_num == 1)) {
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_MIDDLE);

		/* Head & Tail: if there are multi previous layer & multi next layer or
		 *              set previous layer to be Tail and
		 *              set next layer to be Head */
		} else if ((p_mctrl[i].prev_num > 1) && (p_mctrl[i].next_num > 1)) {
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD);
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL);

			start_ofs = p_mctrl[i].prev_layer_idx_addr;
			end_ofs = start_ofs + sizeof(UINT32) * p_mctrl[i].prev_num;
			for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
				k = *(UINT32 *)(id_list_start_addr + j);
				VENDOR_AI_NET_SET_BMP(p_mctrl_entry[k].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL);
			}
			start_ofs = p_mctrl[i].next_layer_idx_addr;
			end_ofs = start_ofs + sizeof(UINT32) * p_mctrl[i].next_num;
			for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
				k = *(UINT32 *)(id_list_start_addr + j);
				VENDOR_AI_NET_SET_BMP(p_mctrl_entry[k].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD);
			}

		/* Tail: if there is only one previous layer & multi next layer
		 *       set next layer to be Head */
		} else if ((p_mctrl[i].prev_num == 1) && (p_mctrl[i].next_num > 1)) {
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL);

			start_ofs = p_mctrl[i].next_layer_idx_addr;
			end_ofs = start_ofs + sizeof(UINT32) * p_mctrl[i].next_num;
			for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
				k = *(UINT32 *)(id_list_start_addr + j);
				VENDOR_AI_NET_SET_BMP(p_mctrl_entry[k].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD);
			}

		/* Head: if there are multi previous layer & only one next layer
		 *       set previous layer to be Tail */
		} else if ((p_mctrl[i].prev_num > 1) && (p_mctrl[i].next_num == 1)) {
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD);
			start_ofs = p_mctrl[i].prev_layer_idx_addr;
			end_ofs = start_ofs + sizeof(UINT32) * p_mctrl[i].prev_num;
			for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
				k = *(UINT32 *)(id_list_start_addr + j);
				VENDOR_AI_NET_SET_BMP(p_mctrl_entry[k].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL);
			}

		} else {
			DBG_ERR("no this mctrl case\r\n");
			ret = HD_ERR_NG;
			goto exit;
		}

		/* Head & Tail: if engine type is not hardware support,
		 *              set previous layer to be Tail and
		 *              set next layer to be Head */
		if (p_mctrl[i].eng != NN_GEN_ENG_CNN && p_mctrl[i].eng != NN_GEN_ENG_CNN2 &&
			p_mctrl[i].eng != NN_GEN_ENG_NUE && p_mctrl[i].eng != NN_GEN_ENG_NUE2) {
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD);
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL);

			start_ofs = p_mctrl[i].prev_layer_idx_addr;
			end_ofs = start_ofs + sizeof(UINT32) * p_mctrl[i].prev_num;
			for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
				k = *(UINT32 *)(id_list_start_addr + j);
				VENDOR_AI_NET_SET_BMP(p_mctrl_entry[k].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL);
			}
			start_ofs = p_mctrl[i].next_layer_idx_addr;
			end_ofs = start_ofs + sizeof(UINT32) * p_mctrl[i].next_num;
			for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
				k = *(UINT32 *)(id_list_start_addr + j);
				VENDOR_AI_NET_SET_BMP(p_mctrl_entry[k].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD);
			}
		}

		/* Head: if engine type is differ from previous mctrl,
		 *       set previous layer to be Tail */
		if (curr_eng != p_mctrl[i].eng) {
			VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD);

			start_ofs = p_mctrl[i].prev_layer_idx_addr;
			end_ofs = start_ofs + sizeof(UINT32) * p_mctrl[i].prev_num;
			for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
				k = *(UINT32 *)(id_list_start_addr + j);
				VENDOR_AI_NET_SET_BMP(p_mctrl_entry[k].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL);
			}
		}
		curr_eng = p_mctrl[i].eng;

		/* Head: if there is previous layer & engine type is differ from previous mctrl
		 *       set previous layer to be Tail */
		if ((p_mctrl[i].prev_num) &&
			(p_mctrl[i].eng == NN_GEN_ENG_CNN || p_mctrl[i].eng == NN_GEN_ENG_CNN2 ||
			 p_mctrl[i].eng == NN_GEN_ENG_NUE || p_mctrl[i].eng == NN_GEN_ENG_NUE2)) {
			start_ofs = p_mctrl[i].prev_layer_idx_addr;
			end_ofs = start_ofs + sizeof(UINT32) * p_mctrl[i].prev_num;
			for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
				k = *(UINT32 *)(id_list_start_addr + j);
				if (p_mctrl[k].eng != p_mctrl[i].eng) {
					VENDOR_AI_NET_SET_BMP(p_mctrl_entry[k].pos_bmp, VENDOR_AI_NET_MCTRL_TAIL);
					VENDOR_AI_NET_SET_BMP(p_mctrl_entry[i].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD);
				}
			}
		}
	}
	p_group_priv->mctrl_list.p_mctrl_entry = p_mctrl_entry;
	p_group_priv->mctrl_list.mctrl_num = p_head->mode_ctrl_num;

	/* Add mctrl to ll cmd group */
	for (i = 0; i < p_head->mode_ctrl_num; i++) {
		if (p_group[i].addr) {
			continue;
		}
		for (j = 0; j < p_head->mode_ctrl_num; j++) {

			/* Find Head mctrl entry */
			if (VENDOR_AI_NET_IS_BMP(p_mctrl_entry[j].pos_bmp, VENDOR_AI_NET_MCTRL_HEAD) &&
				(p_mctrl_entry[j].p_group == NULL)) {
				p_group[i].addr = (UINT32)p_mctrl_entry[j].p_data;
				p_group[i].prev_num = p_mctrl_entry[j].p_data->prev_num;
				p_group[i].p_head = p_mctrl_entry[j].p_data;
				p_group[i].eng = p_mctrl_entry[j].p_data->eng;
				p_group[i].trig_src = p_mctrl_entry[j].p_data->trig_src;
				p_entry = &p_mctrl_entry[j];
				group_num++;

				/* Find next mctrl entry to Tail */
				while (1) {
					list_add_tail(&p_entry->list, &p_group[i].mctrl_listhead);
					p_entry->p_group = &p_group[i];
					p_group[i].cnt++;
					p_group[i].idea_cycle += p_entry->p_data->idea_cycle;
					if (VENDOR_AI_NET_IS_BMP(p_entry->pos_bmp, VENDOR_AI_NET_MCTRL_TAIL)) {
						p_group[i].next_num = p_entry->p_data->next_num;
						p_group[i].p_tail = p_entry->p_data;
						break;
					}
					start_ofs = p_entry->p_data->next_layer_idx_addr;
					k = *(UINT32 *)(id_list_start_addr + start_ofs);
					p_entry = &p_mctrl_entry[k];
				}
				break;
			}
		}
	}
	p_group_priv->group_list.p_group = p_group;
	p_group_priv->group_list.group_num = group_num;

	/* Set step of group */
	for (i = 0; i < p_head->mode_ctrl_num; i++) {
		if (p_group[i].addr == 0) {
			continue;
		}
		if (p_group[i].prev_num == 0) {
			p_group[i].step = 1;
		}

		p_mctrl_tmp = p_group[i].p_tail;
		if (p_mctrl_tmp == NULL) {
			DBG_ERR("group idx(%lu) is NOT set tail\r\n", i);
			ret = HD_ERR_NG;
			goto exit;
		}

		start_ofs = p_mctrl_tmp->next_layer_idx_addr;
		end_ofs = start_ofs + sizeof(UINT32) * p_mctrl_tmp->next_num;
		for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
			k = *(UINT32 *)(id_list_start_addr + j);
			p_group_tmp = (VENDOR_AI_NET_LLGROUP *)p_mctrl_entry[k].p_group;
			if (p_group_tmp == NULL) {
				DBG_ERR("mctrl idx(%lu) is NOT grouping\r\n", k);
				ret = HD_ERR_NG;
				goto exit;
			}

			p_group_tmp->step = VENDOR_AI_NET_MAX(p_group_tmp->step, p_group[i].step + 1);
			step_max = VENDOR_AI_NET_MAX(step_max, p_group[i].step + 1);
		}
	}


	if (p_group_priv->method == VENDOR_AI_NET_JOB_OPT_GRAPH_O2) {
		ret = vendor_ai_net_group_graph_optimize(proc_id, step_max);
		if (ret != HD_OK) {
			DBG_ERR("Fail to graph optimize by VENDOR_AI_NET_JOB_OPT_GRAPH_O2\r\n");
		}
	}

	/* Fill p_mem_entry by group */
	mem_idx = 0;
	for (step_idx = 1; step_idx <= step_max; step_idx++) {
		for (i = 0; i < p_head->mode_ctrl_num; i++) {
			if (p_group[i].addr == 0) {
				continue;
			}
			if (p_group[i].step != step_idx) {
				continue;
			}
			/* Release previous group */
			p_mctrl_tmp = (NN_GEN_MODE_CTRL *)p_group[i].addr;
			start_ofs = p_mctrl_tmp->prev_layer_idx_addr;
			end_ofs = start_ofs + sizeof(UINT32) * p_mctrl_tmp->prev_num;
			for (j = start_ofs; j < end_ofs; j += sizeof(UINT32)) {
				k = *(UINT32 *)(id_list_start_addr + j);
				p_group_tmp = (VENDOR_AI_NET_LLGROUP *)p_mctrl_entry[k].p_group;
				if (p_group_tmp == NULL) {
					DBG_ERR("mctrl idx(%lu) is NOT grouping\r\n", k);
					ret = HD_ERR_NG;
					goto exit;
				}

				/* Set step_end_mid for in buffers */
				if (p_group_tmp->step_end == 0) {
					p_group_tmp->step_end = step_idx;
					p_mem_entry[mem_idx].p_group = p_group_tmp;
					p_mem_entry[mem_idx].is_alloc = 0;
					mem_idx++;
				}
			}

			/* Allocate group out buffer */
			p_mem_entry[mem_idx].p_group = &p_group[i];
			p_mem_entry[mem_idx].is_alloc = 1;
			mem_idx++;
		}
	}

	p_group_priv->mem_list.p_mem = p_mem_entry;
	p_group_priv->mem_list.mem_num = mem_idx;

exit:
	vendor_ai_net_trace(proc_id, AI_GRP, "graph() - end\r\n");
	return ret;
}

HD_RESULT vendor_ai_net_group_proc(UINT32 proc_id, VENDOR_AI_NET_JOB_OPT job_opt_method)
{
#if (NET_GRP_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(p_proc->group_priv);
#else
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(g_ai_net_group_priv[proc_id]);
#endif
	HD_RESULT ret = HD_OK;

	p_group_priv->method = job_opt_method;

	if (_vendor_ai_net_is_linear_job(job_opt_method)) {
		ret = vendor_ai_net_group_linear(proc_id);
	} else if (_vendor_ai_net_is_graph_job(job_opt_method)) {
		ret = vendor_ai_net_group_graph(proc_id);
	}

	return ret;
}

VENDOR_AI_NET_MCTRL_LIST *vendor_ai_net_group_getmctrlresult(UINT32 proc_id)
{
#if (NET_GRP_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(p_proc->group_priv);
#else
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(g_ai_net_group_priv[proc_id]);
#endif
	return &p_group_priv->mctrl_list;
}

VENDOR_AI_NET_LLGROUP_LIST *vendor_ai_net_group_getgroupresult(UINT32 proc_id)
{
#if (NET_GRP_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(p_proc->group_priv);
#else
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(g_ai_net_group_priv[proc_id]);
#endif
	return &p_group_priv->group_list;
}

VENDOR_AI_NET_MEM_LIST *vendor_ai_net_group_getmemresult(UINT32 proc_id)
{
#if (NET_GRP_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(p_proc->group_priv);
#else
	VENDOR_AI_NET_GROUP_MEM *p_group_priv = &(g_ai_net_group_priv[proc_id]);
#endif
	return &p_group_priv->mem_list;
}

HD_RESULT vendor_ai_net_group_init(VOID)
{
	HD_RESULT ret = HD_OK;

#if (NET_GRP_HEAP == 1)
#else
	g_ai_net_group_priv = (VENDOR_AI_NET_GROUP_MEM *)vendor_ai_malloc(sizeof(VENDOR_AI_NET_GROUP_MEM) * g_ai_support_net_max);
	if (g_ai_net_group_priv == NULL) {
		DBG_ERR("vendor_ai_net_group_init malloc fail\r\n");
		return HD_ERR_NOMEM;
	}
	memset(g_ai_net_group_priv, 0, sizeof(VENDOR_AI_NET_GROUP_MEM) * g_ai_support_net_max);
#endif

	return ret;
}

HD_RESULT vendor_ai_net_group_uninit(VOID)
{
	HD_RESULT ret = HD_OK;

#if (NET_GRP_HEAP == 1)
#else
	if (g_ai_net_group_priv) {
		vendor_ai_free(g_ai_net_group_priv, sizeof(VENDOR_AI_NET_GROUP_MEM) * g_ai_support_net_max);
	}
#endif

	return ret;
}
