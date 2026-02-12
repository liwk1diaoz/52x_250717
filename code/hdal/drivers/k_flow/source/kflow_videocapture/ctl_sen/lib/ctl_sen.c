/**
    General Sensor API for project layer

    Sample module detailed description.

    @file       ctl_sen.c
    @ingroup    Predefined_group_name
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "sen_ctrl_int.h"
#include "sen_int.h"
#include "sen_middle_int.h"
#include "sen_dbg_infor_int.h"

/***********************************************************************/
/*                                                                     */
/*                     ctl sensor internal define                      */
/*                                                                     */
/***********************************************************************/
typedef struct {
	UINT32 cnt[CTL_SEN_ID_MAX];
	UINT32 cnt_flag_ofs;
	UINT32 lock_flag_ofs;
	UINT32 unlock_flag_ofs;
} CTL_SEN_SPECIAL_CTRL_INFO;

typedef enum {
	CTL_SEN_FUNC_GETCFG = 0,
	CTL_SEN_FUNC_MAX_NUM,
	ENUM_DUMMY4WORD(CTL_SEN_SPECIAL_FUNC),
} CTL_SEN_SPECIAL_FUNC;

/***********************************************************************/
/*                                                                     */
/*                        ctl sensor global value                      */
/*                                                                     */
/***********************************************************************/
CTL_SEN_HDL_CONTEXT ctl_sen_hdl_ctx = {0};
CTL_SEN_VOS_MEM_INFO ctl_sen_mem_info = {0};

CTL_SEN_INIT_OBJ *g_ctl_sen_init_obj[CTL_SEN_ID_MAX_SUPPORT] = {NULL};
CTL_SEN_CTRL_OBJ *g_ctl_sen_ctrl_obj[CTL_SEN_ID_MAX_SUPPORT] = {NULL};

static UINT32 g_ctl_sen_open_cnt[CTL_SEN_ID_MAX] = {0};
static CTL_SEN_SPECIAL_CTRL_INFO g_ctl_sen_func_tab[CTL_SEN_FUNC_MAX_NUM] = {
	{{0}, CTL_SEN_FLAG_GET_STATUS_CNT, CTL_SEN_FLAG_GET_STATUS_OFS_LOCK, CTL_SEN_FLAG_GET_STATUS_OFS_UNLOCK},
};

/* time record */
UINT32 proc_time_cnt = 0;
static CTL_SEN_PROC_TIME proc_time[CTL_SEN_PROC_TIME_CNT_MAX] = {0};

static BOOL sen_module_is_opened(CTL_SEN_ID id);

/***********************************************************************/
/*                                                                     */
/*                         ctl sensor module ctrl                      */
/*                                                                     */
/***********************************************************************/
static  VK_DEFINE_SPINLOCK(sen_lock_state);
#define loc_cpu_state(flags) vk_spin_lock_irqsave(&sen_lock_state, flags)
#define unl_cpu_state(flags) vk_spin_unlock_irqrestore(&sen_lock_state, flags)

#if 1
static void _ctl_sen_module_set_state(CTL_SEN_CTRL_OBJ *obj, UINT32 sen_state)
{
	unsigned long flags;

	loc_cpu_state(flags);
	obj->state |= sen_state;
	unl_cpu_state(flags);
}

static void _ctl_sen_module_clr_state(CTL_SEN_CTRL_OBJ *obj, UINT32 sen_state)
{
	unsigned long flags;

	loc_cpu_state(flags);
	obj->state &= ~sen_state;
	unl_cpu_state(flags);
}
#define ctl_sen_module_set_state(obj, sen_state) _ctl_sen_module_set_state(obj, sen_state)
#define ctl_sen_module_clr_state(obj, sen_state) _ctl_sen_module_clr_state(obj, sen_state)
#else
#define ctl_sen_module_set_state(obj, sen_state) (obj->state |= sen_state)
#define ctl_sen_module_clr_state(obj, sen_state) (obj->state &= ~sen_state)
#endif
#define ctl_sen_module_get_state(obj) (obj->state)
void sen_module_sem(SEM_HANDLE *sem_handle, BOOL b_lock);

CTL_SEN_SEM ctl_sen_sem_tab[CTL_SEN_ID_MAX] = {
	SEMID_CTL_SEN1, SEMID_CTL_SEN2, SEMID_CTL_SEN3, SEMID_CTL_SEN4,
	SEMID_CTL_SEN5, SEMID_CTL_SEN6, SEMID_CTL_SEN7, SEMID_CTL_SEN8,
};

static SEN_MAP_TBL *sen_module_get_sen_map_tbl(CTL_SEN_ID id);
static CTL_SEN_ID sen_module_get_sen_map_tbl_id(CHAR *name);
typedef enum {
	CTL_SEN_MAP_TBL_ITEM_SENREG,    ///< dont care id
	CTL_SEN_MAP_TBL_ITEM_INITCFG,
	ENUM_DUMMY4WORD(CTL_SEN_MAP_TBL_ITEM)
} CTL_SEN_MAP_TBL_ITEM;

#define CTL_SEN_NULL_STR  "NULL" // length must <= CTL_SEN_NAME_LEN
static CHAR sen_null_str[CTL_SEN_NAME_LEN] = CTL_SEN_NULL_STR;
CTL_SEN_PINMUX sen_pincfg[CTL_SEN_ID_MAX_SUPPORT][CTL_SEN_PINMUX_MAX_NUM + 1] = {
	[0 ...(CTL_SEN_ID_MAX_SUPPORT - 1)][0 ...(CTL_SEN_PINMUX_MAX_NUM - 1)] =
	{0, 0, NULL}
};
SEN_MAP_TBL sen_map_tbl[CTL_SEN_ID_MAX_SUPPORT] = {
	[0 ...(CTL_SEN_ID_MAX_SUPPORT - 1)] =
	{
		CTL_SEN_NULL_STR,   // name
		{NULL, NULL, NULL}, // CTL_SEN_REG_OBJ
		CTL_SEN_ID_MAX,     // CTL_SEN_ID
		{
			// CTL_SEN_INIT_CFG_OBJ
			{{0, 0, NULL}, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0}, // CTL_SEN_INIT_PIN_CFG
			{0, {0, 0, 0 }, 0},     // CTL_SEN_INIT_IF_CFG
			{0, 0, 0},              // CTL_SEN_INIT_CMDIF_CFG
			{0, 0, 0},              // CTL_SEN_INIT_SENDRV_CFG
			0,                      // CTL_SEN_DRVDEV
		}
	}
};

void sen_module_sem(SEM_HANDLE *sem_handle, BOOL b_lock)
{
	if (b_lock) {
		vos_sem_wait(*sem_handle);
	} else {
		vos_sem_sig(*sem_handle);
	}
}

_INLINE void sen_module_chk_global_lock(FLGPTN flag)
{
	FLGPTN ui_flag;

	vos_flag_wait(&ui_flag, ctl_sen_get_flag_id(FLG_ID_CTL_SEN_GLB), flag, TWF_ANDW);
}

_INLINE void sen_module_global_lock(FLGPTN flag)
{
	FLGPTN ui_flag;

	vos_flag_wait(&ui_flag, ctl_sen_get_flag_id(FLG_ID_CTL_SEN_GLB), flag, TWF_CLR | TWF_ANDW);
}

_INLINE void sen_module_global_unlock(FLGPTN flag)
{
	vos_flag_set(ctl_sen_get_flag_id(FLG_ID_CTL_SEN_GLB), flag);
}

_INLINE void sen_module_wait_end(FLGPTN flag)
{
	FLGPTN ui_flag;

	vos_flag_wait(&ui_flag, ctl_sen_get_flag_id(FLG_ID_CTL_SEN), flag, TWF_ANDW);
}

_INLINE void sen_module_lock(FLGPTN flag)
{
	FLGPTN ui_flag;

	vos_flag_wait(&ui_flag, ctl_sen_get_flag_id(FLG_ID_CTL_SEN), flag, TWF_CLR | TWF_ANDW);
}

_INLINE void sen_module_unlock(FLGPTN flag)
{
	vos_flag_set(ctl_sen_get_flag_id(FLG_ID_CTL_SEN), flag);
}

_INLINE void sen_module_special_reset_cnt(CTL_SEN_ID id)
{
	UINT32 i;

	for (i = 0; i < CTL_SEN_FUNC_MAX_NUM; i++) {
		g_ctl_sen_func_tab[i].cnt[id] = 0;
	}
}

_INLINE FLGPTN sen_module_special_get_all_flag(CTL_SEN_ID id)
{
	FLGPTN flag = 0;
	UINT32 i;

	for (i = 0; i < CTL_SEN_FUNC_MAX_NUM; i++) {
		flag |= FLGPTN_BIT(id + g_ctl_sen_func_tab[i].cnt_flag_ofs);
		flag |= FLGPTN_BIT(id + g_ctl_sen_func_tab[i].lock_flag_ofs);
		flag |= FLGPTN_BIT(id + g_ctl_sen_func_tab[i].unlock_flag_ofs);
	}
	return flag;
}

_INLINE FLGPTN sen_module_special_get_all_lock_flag(CTL_SEN_ID id)
{
	FLGPTN flag = 0;
	UINT32 i;

	for (i = 0; i < CTL_SEN_FUNC_MAX_NUM; i++) {
		flag |= FLGPTN_BIT(id + g_ctl_sen_func_tab[i].lock_flag_ofs);
	}
	return flag;
}

_INLINE FLGPTN sen_module_special_get_all_end_flag(CTL_SEN_ID id)
{
	FLGPTN flag = 0;
	UINT32 i;

	for (i = 0; i < CTL_SEN_FUNC_MAX_NUM; i++) {
		flag |= FLGPTN_BIT(id + g_ctl_sen_func_tab[i].unlock_flag_ofs);
	}
	return flag;
}

_INLINE void sen_module_special_cnt_inc(CTL_SEN_ID id, CTL_SEN_SPECIAL_FUNC idx)
{
	FLGPTN ui_flag, mask;

	mask = FLGPTN_BIT(id + g_ctl_sen_func_tab[idx].cnt_flag_ofs);
	vos_flag_wait(&ui_flag, ctl_sen_get_flag_id(FLG_ID_CTL_SEN), mask, TWF_CLR | TWF_ORW);
	g_ctl_sen_func_tab[idx].cnt[id] += 1;
	vos_flag_set(ctl_sen_get_flag_id(FLG_ID_CTL_SEN), mask);
}

_INLINE UINT32 sen_module_special_cnt_dec(CTL_SEN_ID id, CTL_SEN_SPECIAL_FUNC idx)
{
	FLGPTN ui_flag, mask;

	mask = FLGPTN_BIT(id + g_ctl_sen_func_tab[idx].cnt_flag_ofs);

	vos_flag_wait(&ui_flag, ctl_sen_get_flag_id(FLG_ID_CTL_SEN), mask, TWF_CLR | TWF_ORW);

	if (g_ctl_sen_func_tab[idx].cnt[id] > 0) {
		g_ctl_sen_func_tab[idx].cnt[id] -= 1;
	} else {
		CTL_SEN_DBG_ERR("lock counter error\r\n");
	}

	vos_flag_set(ctl_sen_get_flag_id(FLG_ID_CTL_SEN), mask);

	if (g_ctl_sen_func_tab[idx].cnt[id] == 0) {
		return 1;
	}
	return 0;
}

_INLINE void sen_module_special_lock(CTL_SEN_ID id, CTL_SEN_SPECIAL_FUNC idx)
{
	FLGPTN ui_flag, mask;

	mask = FLGPTN_BIT(id + g_ctl_sen_func_tab[idx].lock_flag_ofs);

	vos_flag_wait(&ui_flag, ctl_sen_get_flag_id(FLG_ID_CTL_SEN), mask, TWF_ORW);
	vos_flag_clr(ctl_sen_get_flag_id(FLG_ID_CTL_SEN), FLGPTN_BIT(id + g_ctl_sen_func_tab[idx].unlock_flag_ofs));

	sen_module_special_cnt_inc(id, idx);
}

_INLINE void sen_module_special_unlock(CTL_SEN_ID id, CTL_SEN_SPECIAL_FUNC idx)
{
	if (sen_module_special_cnt_dec(id, idx) == 1) {
		vos_flag_set(ctl_sen_get_flag_id(FLG_ID_CTL_SEN), FLGPTN_BIT(id + g_ctl_sen_func_tab[idx].unlock_flag_ofs));
	}
}

_INLINE CTL_SEN_CTRL_OBJ *sen_module_get_ctrl_obj(CTL_SEN_ID id)
{
	if (id >= CTL_SEN_ID_MAX) {
		CTL_SEN_DBG_ERR("input ID (%d) overflow\r\n", id);
		return NULL;
	}
	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input ID (%d) not init context\r\n", id);
	}

	return g_ctl_sen_ctrl_obj[id];
}

static  VK_DEFINE_SPINLOCK(sen_lock_proc_time);
#define loc_cpu_proc_time(flags) vk_spin_lock_irqsave(&sen_lock_proc_time, flags)
#define unl_cpu_proc_time(flags) vk_spin_unlock_irqrestore(&sen_lock_proc_time, flags)
void ctl_sen_set_proc_time(CHAR *func_name, CTL_SEN_ID id, CTL_SEN_PROC_TIME_ITEM item)
{
	UINT32 cur_cnt;
	unsigned long flags;

	loc_cpu_proc_time(flags);
	cur_cnt = proc_time_cnt;
	proc_time_cnt = (proc_time_cnt + 1) % CTL_SEN_PROC_TIME_CNT_MAX;
	unl_cpu_proc_time(flags);

	memcpy((void *)&proc_time[cur_cnt].func_name[0], (void *)func_name, sizeof(proc_time[cur_cnt].func_name));

	proc_time[cur_cnt].item = item;
	proc_time[cur_cnt].id = id;
	proc_time[cur_cnt].time_us_u64 = hwclock_get_longcounter();
}

CTL_SEN_PROC_TIME *ctl_sen_get_proc_time(void)
{
	return &proc_time[0];
}

UINT32 ctl_sen_get_proc_time_cnt(void)
{
	return proc_time_cnt;
}

UINT32 ctl_sen_get_cur_state(CTL_SEN_ID id)
{
	CTL_SEN_CTRL_OBJ *obj = sen_module_get_ctrl_obj(id);

	if (obj == NULL) {
		return 0;
	}
	return ctl_sen_module_get_state(obj);
}

static BOOL sen_module_chk_op_state(CTL_SEN_ID id, CTL_SEN_STATE tar_state, BOOL start, UINT32 param)
{
	BOOL op_rst = FALSE;
	CTL_SEN_CTRL_OBJ *obj = sen_module_get_ctrl_obj(id);
	CTL_SEN_STATE cur_state;

	if (obj == NULL) {
		return FALSE;
	}

	cur_state = ctl_sen_module_get_state(obj);

	switch (tar_state) {
	case CTL_SEN_STATE_INTCFG:
	case CTL_SEN_STATE_REGSEN:
		if ((cur_state & CTL_SEN_STATE_OPEN) != CTL_SEN_STATE_OPEN) {
			op_rst = TRUE;
		}
		break;
	case CTL_SEN_STATE_OPEN:
		if (start) {
			if ((cur_state & CTL_SEN_STATE_OPEN) == CTL_SEN_STATE_OPEN) {
				CTL_SEN_DBG_ERR("sensor id %d is opened\r\n", id);
			} else if (sen_module_get_sen_map_tbl(id) != NULL) {
				op_rst = TRUE;
			}
		} else {
			if ((cur_state & CTL_SEN_STATE_PWRON) != CTL_SEN_STATE_PWRON) {
				op_rst = TRUE;
			} else if ((cur_state & CTL_SEN_STATE_OPEN) != CTL_SEN_STATE_OPEN) {
				CTL_SEN_DBG_ERR("sensor id %d is not open\r\n", id);
			}
		}
		break;
	case CTL_SEN_STATE_PWRON:
	case CTL_SEN_STATE_PWRSAVE:
		if ((cur_state & CTL_SEN_STATE_OPEN) == CTL_SEN_STATE_OPEN) {
			op_rst = TRUE;
		}
		break;
	case CTL_SEN_STATE_GETCFG:
		if ((cur_state & CTL_SEN_STATE_PWRON) == CTL_SEN_STATE_PWRON) {
			op_rst = TRUE;
		} else if ((cur_state & CTL_SEN_STATE_OPEN) == CTL_SEN_STATE_OPEN) {
			if ((param == CTL_SEN_CFGID_GET_PLUG) || (param == CTL_SEN_CFGID_GET_PLUG_INFO) || (param == CTL_SEN_CFGID_GET_PROBE_SEN)) {
				// need to get after pwr on (for i2c work)
				op_rst = FALSE;
			} else {
				op_rst = TRUE;
			}
		}
		break;
	case CTL_SEN_STATE_SETCFG:
		if ((param >= CTL_SEN_CFGID_INIT_BASE) && (param < CTL_SEN_CFGID_INIT_MAX)) {
			if (((cur_state & CTL_SEN_STATE_OPEN) == CTL_SEN_STATE_OPEN) &&
				((cur_state & CTL_SEN_STATE_PWRON) != CTL_SEN_STATE_PWRON)) {
				op_rst = TRUE;
			}
		} else if ((param >= CTL_SEN_CFGID_VX1_BASE) && (param < CTL_SEN_CFGID_VX1_MAX)) {
			if ((cur_state & CTL_SEN_STATE_INTCFG) == CTL_SEN_STATE_INTCFG) {
				op_rst = TRUE;
			}
		} else {
			if ((cur_state & CTL_SEN_STATE_PWRON) == CTL_SEN_STATE_PWRON) {
				op_rst = TRUE;
			}
		}
		break;
	case CTL_SEN_STATE_SLEEP:
	case CTL_SEN_STATE_WRITEREG:
	case CTL_SEN_STATE_READREG:
	case CTL_SEN_STATE_CHGMODE:
	case CTL_SEN_STATE_WAITINTE:
		if ((cur_state & CTL_SEN_STATE_PWRON) == CTL_SEN_STATE_PWRON) {
			op_rst = TRUE;
		}
		break;
	default:
		break;
	}

	if (op_rst == FALSE) {
		if (param == 0) {
			CTL_SEN_DBG_ERR("id %d cur(0x%.8x)->tar(0x%.8x) error\r\n", id, cur_state, tar_state);
		} else {
			CTL_SEN_DBG_ERR("id %d cur(0x%.8x)->tar(0x%.8x) error, param 0x%.8x\r\n", id, cur_state, tar_state, param);
		}
	}

	if (((ctl_sen_msg_type[id] & CTL_SEN_MSG_TYPE_OP) == CTL_SEN_MSG_TYPE_OP) ||
		((ctl_sen_msg_type[id] & CTL_SEN_MSG_TYPE_OP_SIMPLE) == CTL_SEN_MSG_TYPE_OP_SIMPLE)) {
		if (((ctl_sen_msg_type[id] & CTL_SEN_MSG_TYPE_OP_SIMPLE) == CTL_SEN_MSG_TYPE_OP_SIMPLE) &&
			((cur_state & CTL_SEN_STATE_PWRON) == CTL_SEN_STATE_PWRON) &&
			((tar_state == CTL_SEN_STATE_GETCFG) || (tar_state == CTL_SEN_STATE_SETCFG) || (tar_state == CTL_SEN_STATE_WRITEREG) || (tar_state == CTL_SEN_STATE_READREG))) {
			goto MSG_END;
		}
		if (param == 0) {
			CTL_SEN_DBG_WRN("[SEN,OP]id%d 0x%.8x -> %s(%d)\r\n", id, cur_state, sen_get_opstate_str(tar_state), start);
		} else {
			CTL_SEN_DBG_WRN("[SEN,OP]id%d 0x%.8x -> %s(%d), param 0x%.8x\r\n", id, cur_state, sen_get_opstate_str(tar_state), start, param);
		}
	}
MSG_END:



	return op_rst;
}

/***********************************************************************/
/*                                                                     */
/*                         ctl sensor module                           */
/*                                                                     */
/***********************************************************************/

static INT32 sen_module_set_sen_map_tbl_senreg(CHAR *name, CTL_SEN_REG_OBJ *sendrv_reg_obj)
{
	UINT32 i, j;
	BOOL b_update = FALSE;

	if (name == NULL) {
		CTL_SEN_DBG_ERR("name NULL\r\n");
		return CTL_SEN_E_IN_PARAM;
	}

	if (sendrv_reg_obj != NULL) {
		// confirm that the registered name exists in the mapping table
		// if it exists, update reg obj
		for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
			if (strcmp(sen_map_tbl[i].name, name) == 0) {
				sen_map_tbl[i].sendrv_reg_obj = *sendrv_reg_obj;
				if (sen_map_tbl[i].sendrv_reg_obj.pwr_ctrl == NULL) {
					CTL_SEN_DBG_ERR("pwr_ctrl NULL\r\n");
				}
				if (sen_map_tbl[i].sendrv_reg_obj.drv_tab == NULL) {
					CTL_SEN_DBG_ERR("drv_tab NULL\r\n");
				}
				b_update = TRUE;
			}
		}
		if (b_update) {
			return CTL_SEN_E_OK;
		}

		// reg sendrv
		for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
			if (strcmp(sen_map_tbl[i].name, sen_null_str) == 0) {
				for (j = 0; j < CTL_SEN_NAME_LEN; j++) {
					sen_map_tbl[i].name[j] = *(CHAR *)(name + j * 1);
				}
				sen_map_tbl[i].sendrv_reg_obj = *sendrv_reg_obj;
				if (sen_map_tbl[i].sendrv_reg_obj.pwr_ctrl == NULL) {
					CTL_SEN_DBG_ERR("pwr_ctrl NULL\r\n");
				}
				if (sen_map_tbl[i].sendrv_reg_obj.drv_tab == NULL) {
					CTL_SEN_DBG_ERR("drv_tab NULL\r\n");
				}
				return CTL_SEN_E_OK;
			}
		}
	} else {
		// unreg sendrv
		for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
			if (strcmp(sen_map_tbl[i].name, name) == 0) {
				sen_map_tbl[i].sendrv_reg_obj.pwr_ctrl = NULL;
				sen_map_tbl[i].sendrv_reg_obj.drv_tab = NULL;
				sen_map_tbl[i].sendrv_reg_obj.det_plug_in = NULL;
				if (sen_map_tbl[i].id == CTL_SEN_ID_MAX) {
					for (j = 0; j < CTL_SEN_NAME_LEN; j++) {
						sen_map_tbl[i].name[j] = sen_null_str[j];
					}
				}
				b_update = TRUE;
			}
		}
		if (b_update) {
			return CTL_SEN_E_OK;
		}
	}

	if (sendrv_reg_obj != NULL) {
		// reg sendrv
		CTL_SEN_DBG_ERR("name %s reg sendrv fail\r\n", name);
	} else {
		// unreg sendrv
		CTL_SEN_DBG_ERR("name %s not reg sendrv\r\n", name);
	}
	return CTL_SEN_E_SYS;
}

static void copy_pincfg(UINT32 idx, CTL_SEN_PINMUX *pincfg_root)
{
	CTL_SEN_PINMUX *pincfg = pincfg_root;
	int i;

	i = 0;
	while ((pincfg->pnext != NULL) && (i < CTL_SEN_PINMUX_MAX_NUM)) {
		pincfg = pincfg->pnext;
		sen_pincfg[idx][i].func = pincfg->func;
		sen_pincfg[idx][i].cfg = pincfg->cfg;
		sen_pincfg[idx][i].pnext = &sen_pincfg[idx][i + 1];
		i++;
	}

	if (i == CTL_SEN_PINMUX_MAX_NUM) {
		CTL_SEN_DBG_ERR("input pinmux ovfl (max %d)\r\n", CTL_SEN_PINMUX_MAX_NUM);
	}

	pincfg_root->pnext = &sen_pincfg[idx][0];
	if (i >= 1) {
		sen_pincfg[idx][i - 1].pnext = NULL;
	}
}

static void clr_pincfg(UINT32 idx)
{
	int i;

	for (i = 0; i < CTL_SEN_PINMUX_MAX_NUM; i++) {
		sen_pincfg[idx][i].func = 0;
		sen_pincfg[idx][i].cfg = 0;
		sen_pincfg[idx][i].pnext = NULL;
	}
}

static BOOL copy_map_tbl_sendrv_reg_obj(CTL_SEN_ID id, CHAR *name, UINT32 i, CTL_SEN_INIT_CFG_OBJ *init_cfg_obj)
{
	UINT32 j, k;
	BOOL rst = FALSE;

	for (j = 0; j < CTL_SEN_ID_MAX_SUPPORT; j++) {
		if (strcmp(sen_map_tbl[j].name, sen_null_str) == 0) {
			for (k = 0; k < CTL_SEN_NAME_LEN; k++) {
				sen_map_tbl[j].name[k] = *(CHAR *)(name + k * 1);
			}
			sen_map_tbl[j].id = id;
			sen_map_tbl[j].sendrv_reg_obj = sen_map_tbl[i].sendrv_reg_obj;
			sen_map_tbl[j].init_cfg_obj = *init_cfg_obj;

			if (sen_map_tbl[j].init_cfg_obj.pin_cfg.pinmux.pnext != NULL) {
				copy_pincfg(j, &sen_map_tbl[j].init_cfg_obj.pin_cfg.pinmux);
			}
			rst = TRUE;
			break;
		}
	}
	return rst;
}


static INT32 sen_module_clr_sen_map_tbl_initcfg(CTL_SEN_ID id)
{
	UINT32 i, k;
	INT32 rt = CTL_SEN_E_OK;

	if (id >= CTL_SEN_ID_MAX_SUPPORT) {
		CTL_SEN_DBG_ERR("input error id%d\r\n", id);
		return CTL_SEN_E_IN_PARAM;
	}

	// reset current sen_map_tbl[i].id == id (id has been registered for init_cfg_obj)
	for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
		if (sen_map_tbl[i].id == id) {
			sen_map_tbl[i].id = CTL_SEN_ID_MAX;
			memset((void *)(&sen_map_tbl[i].init_cfg_obj), 0, sizeof(CTL_SEN_INIT_CFG_OBJ));
			if (sen_map_tbl[i].init_cfg_obj.pin_cfg.pinmux.pnext != NULL) {
				clr_pincfg(i);
			}
			sen_map_tbl[i].init_cfg_obj.pin_cfg.pinmux.pnext = NULL;
			if ((sen_map_tbl[i].sendrv_reg_obj.pwr_ctrl == NULL) && (sen_map_tbl[i].sendrv_reg_obj.det_plug_in == NULL) && (sen_map_tbl[i].sendrv_reg_obj.drv_tab == NULL)) {
				// sendrv not register
				for (k = 0; k < CTL_SEN_NAME_LEN; k++) {
					sen_map_tbl[i].name[k] = *(CHAR *)(sen_null_str + k * 1);
				}
			}
		}
	}

	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("tbl overflow\r\n");
	}
	return rt;
}


static INT32 sen_module_set_sen_map_tbl_initcfg(CTL_SEN_ID id, CHAR *name, CTL_SEN_INIT_CFG_OBJ *init_cfg_obj)
{
	UINT32 i, k;
	INT32 rt = CTL_SEN_E_OK;

	if ((name == NULL) || (init_cfg_obj == NULL) || (id >= CTL_SEN_ID_MAX_SUPPORT)) {
		CTL_SEN_DBG_ERR("input error name:0x%.8x,init_cfg_obj0x%.8x,id%d\r\n", (UINT32)name, (UINT32)init_cfg_obj, id);
		return CTL_SEN_E_IN_PARAM;
	}

	// reset current sen_map_tbl[i].id == id (id has been registered for init_cfg_obj)
	for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
		if (sen_map_tbl[i].id == id) {
			sen_map_tbl[i].id = CTL_SEN_ID_MAX;
		}
	}

	for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
		// sendrv or init_cfg_obj already reg
		if (strcmp(sen_map_tbl[i].name, name) == 0) {
			if (sen_map_tbl[i].id == CTL_SEN_ID_MAX) {
				// update init_cfg_obj
				sen_map_tbl[i].id = id;
				sen_map_tbl[i].init_cfg_obj = *init_cfg_obj;
				if (sen_map_tbl[i].init_cfg_obj.pin_cfg.pinmux.pnext != NULL) {
					copy_pincfg(i, &sen_map_tbl[i].init_cfg_obj.pin_cfg.pinmux);
				}
				rt = CTL_SEN_E_OK;
				goto finish;
			}
		}
	}

	for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
		// sendrv or init_cfg_obj already reg
		if (strcmp(sen_map_tbl[i].name, name) == 0) {
			// copy sendrv info, set init_cfg_obj
			if (copy_map_tbl_sendrv_reg_obj(id, name, i, init_cfg_obj) == TRUE) {
				rt = CTL_SEN_E_OK;
				goto finish;
			}
			CTL_SEN_DBG_ERR("tbl overflow\r\n");
			rt = CTL_SEN_E_MAP_TBL;
			break;
		}
	}

	// sendrv and init_cfg_obj not reg
	for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
		if (strcmp(sen_map_tbl[i].name, sen_null_str) == 0) {
			for (k = 0; k < CTL_SEN_NAME_LEN; k++) {
				sen_map_tbl[i].name[k] = *(CHAR *)(name + k * 1);
			}
			sen_map_tbl[i].id = id;
			sen_map_tbl[i].init_cfg_obj = *init_cfg_obj;
			if (sen_map_tbl[i].init_cfg_obj.pin_cfg.pinmux.pnext != NULL) {
				copy_pincfg(i, &sen_map_tbl[i].init_cfg_obj.pin_cfg.pinmux);
			}
			rt = CTL_SEN_E_OK;
			break;
		}
	}

finish:

	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("tbl overflow\r\n");
	}
	return rt;
}

static INT32 sen_module_set_sen_map_tbl(CTL_SEN_ID id, CTL_SEN_MAP_TBL_ITEM item, CHAR *name, CTL_SEN_REG_OBJ *sendrv_reg_obj, CTL_SEN_INIT_CFG_OBJ *init_cfg_obj)
{
	INT32 rt = 0;

	if (item == CTL_SEN_MAP_TBL_ITEM_SENREG) {
		// set sendrv_reg_obj
		rt += sen_module_set_sen_map_tbl_senreg(name, sendrv_reg_obj);

	} else if (item == CTL_SEN_MAP_TBL_ITEM_INITCFG) {
		// set cfg_obj
		rt += sen_module_set_sen_map_tbl_initcfg(id, name, init_cfg_obj);

	} else {
		CTL_SEN_DBG_ERR("item %d error\r\n", item);
		rt = CTL_SEN_E_IN_PARAM;
	}

	return rt;
}

static SEN_MAP_TBL *sen_module_get_sen_map_tbl(CTL_SEN_ID id)
{
	UINT32 i;

	for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
		if (sen_map_tbl[i].id == id) {
			if ((sen_map_tbl[i].sendrv_reg_obj.pwr_ctrl == NULL) &&
				(sen_map_tbl[i].sendrv_reg_obj.drv_tab == NULL) &&
				(sen_map_tbl[i].sendrv_reg_obj.det_plug_in == NULL)) {
				CTL_SEN_DBG_ERR("sendrv not reg\r\n");
				return NULL;
			}
			return &sen_map_tbl[i];
		}
	}
	CTL_SEN_DBG_ERR("id %d init_cfg_obj NULL \r\n", id);
	return NULL;
}

static CTL_SEN_ID sen_module_get_sen_map_tbl_id(CHAR *name)
{
	UINT32 i;

	for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
		if (strcmp(sen_map_tbl[i].name, name) == 0) {
			return sen_map_tbl[i].id;
		}
	}
	return CTL_SEN_ID_MAX;
}

static INT32 sen_module_init_cfg(CTL_SEN_ID id, CHAR *name, CTL_SEN_INIT_CFG_OBJ *init_cfg_obj)
{
	INT32 rt = 0;
	CTL_SEN_CTRL_OBJ *obj;

	rt += sen_module_set_sen_map_tbl(id, CTL_SEN_MAP_TBL_ITEM_INITCFG, name, NULL, init_cfg_obj);
	if (rt != E_OK) {
		CTL_SEN_DBG_ERR("id %d name %s set init_cfg_obj fail\r\n", id, name);
		return rt;
	}

	obj = sen_module_get_ctrl_obj(id);
	if (obj == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}

	if (sen_module_chk_op_state(id, CTL_SEN_STATE_INTCFG, TRUE, 0) != TRUE) {
		return CTL_SEN_E_STATE;
	}

	ctl_sen_module_set_state(obj, CTL_SEN_STATE_INTCFG);
//	sen_module_unlock(FLGPTN_BIT(id) | sen_module_special_get_all_flag(id));

	return rt;
}

static INT32 sen_module_pwr_ctrl(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag)
{
	CTL_SEN_STATE state;
	BOOL start;
	CTL_SEN_CTRL_OBJ *obj;
	INT32 rt = 0;
	UINT32 i, cur_open_id_bit, cnt, timeout = 1000, tmp;
	UINT32 dummy_data = 0;// to avoid coverity error of NULL pointer

	obj = sen_module_get_ctrl_obj(id);
	if (obj == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}

	switch (flag) {
	case CTL_SEN_PWR_CTRL_TURN_ON:
		state = CTL_SEN_STATE_PWRON;
		start = TRUE;
		break;
	case CTL_SEN_PWR_CTRL_TURN_OFF:
		state = CTL_SEN_STATE_PWRON;
		start = FALSE;
		break;
	case CTL_SEN_PWR_CTRL_SAVE_BEGIN:
		state = CTL_SEN_STATE_PWRSAVE;
		start = TRUE;
		break;
	case CTL_SEN_PWR_CTRL_SAVE_END:
		state = CTL_SEN_STATE_PWRSAVE;
		start = FALSE;
		break;
	default:
		CTL_SEN_DBG_ERR("flag error %d\r\n", flag);
		return CTL_SEN_E_IN_PARAM;
	}

	if (!start) {
		ctl_sen_module_clr_state(obj, state);
		cnt = 0;
		while (1) {
			tmp = ctl_sen_module_get_state(obj);
			if ((tmp & ~(CTL_SEN_STATE_INTCFG | CTL_SEN_STATE_OPEN)) == CTL_SEN_STATE_NONE) {
				break;
			}
			ctl_sen_delayms(5);
			cnt++;
			if (cnt >= timeout) {
				DBG_ERR("id %d timout sts=0x%.8x\r\n", id, (unsigned int)tmp);
				ctl_sen_module_set_state(obj, state);
				return CTL_SEN_E_STATE;
			}
		}
		if (sen_module_chk_op_state(id, state, start, 0) != TRUE) {
			ctl_sen_module_set_state(obj, state);
			return CTL_SEN_E_STATE;
		}
	} else {
		if (sen_module_chk_op_state(id, state, start, 0) != TRUE) {
			return CTL_SEN_E_STATE;
		}
	}
	sen_module_lock(FLGPTN_BIT(id)); // CTL_SEN_STATE_PWRSAVE ??

	if (start) {
		// mclk need to sync(enable) before power on
		if (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.mclksrc_sync) {
			rt = sen_ctrl_mclksync_init(id, CTL_SEN_MODE_PWR);
			if (rt != CTL_SEN_E_OK) {
				CTL_SEN_DBG_ERR("id %d sen_ctrl_mclksync_init fail\r\n", id);
				goto err;
			}
		}
		// cmd interface pinmux must config to function after sensor power on
		rt = sen_ctrl_pwr_ctrl(id, flag);
	} else {
		// csi sensor must be set stanby(LP) before power off
		rt = sen_ctrl_stop_if(id);
		if (rt != CTL_SEN_E_OK) {
			CTL_SEN_DBG_ERR("id %d sen_ctrl_stop_if\r\n", id);
			goto err;
		}
		// ad sendrv want to separate power_off and ad_uninit
		sen_ctrl_set_cfg(id, CTL_SEN_CFGID_AD_UNINIT, &dummy_data);
	}

	/* cfg cmdif pinmux */
	if (state == CTL_SEN_STATE_PWRON) {
		cur_open_id_bit = 0;
		for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
			if (sen_ctrl_chk_add_ctx(i)) {
				if (sen_module_is_opened(i)) {
					cur_open_id_bit |= 1 << i;
				}
			}
		}
		rt = sen_ctrl_cfg_pinmux_cmdif(id, cur_open_id_bit, start);
		if (rt != CTL_SEN_E_OK) {
			CTL_SEN_DBG_ERR("id %d cfg cmdif pinmux fail\r\n", id);
			goto err;
		}
	}

	if (!start) {
		// cmd interface pinmux must config to gpio before sensor power off
		rt = sen_ctrl_pwr_ctrl(id, flag); //
		// mclk need to disable after power off
		if (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.mclksrc_sync) {
			rt = sen_ctrl_mclksync_uninit(id, obj->cur_sen_mode);
			if (rt != CTL_SEN_E_OK) {
				CTL_SEN_DBG_ERR("id %d sen_ctrl_mclksync_uninit fail\r\n", id);
				goto err;
			}
		}
	}

	if (start) {
		ctl_sen_module_set_state(obj, state);
		// ad sendrv want to separate power_on and ad_init
		sen_ctrl_set_cfg(id, CTL_SEN_CFGID_AD_INIT, &dummy_data);
	}
err:
	sen_module_unlock(FLGPTN_BIT(id)); // CTL_SEN_STATE_PWRSAVE ??

	return rt;
}

static BOOL sen_module_is_opened(CTL_SEN_ID id)
{
	CTL_SEN_CTRL_OBJ *obj = sen_module_get_ctrl_obj(id);

	if (obj == NULL) {
		return FALSE;
	}

	if ((obj->state & CTL_SEN_STATE_OPEN) == CTL_SEN_STATE_OPEN) {
		return TRUE;
	}

	return FALSE;
}

static INT32 sen_module_open_init(CTL_SEN_ID id)
{
	INT32 rt = CTL_SEN_E_OK;
	SEN_MAP_TBL *sen_map_tbl = NULL;
	CTL_SEN_STATE tmp_state;

	tmp_state = g_ctl_sen_ctrl_obj[id]->state;

	g_ctl_sen_ctrl_obj[id]->cur_sen_mode = CTL_SEN_MODE_UNKNOWN;
	g_ctl_sen_ctrl_obj[id]->state = tmp_state;

	sen_map_tbl = sen_module_get_sen_map_tbl(id);

	if (sen_map_tbl != NULL) {
		g_ctl_sen_init_obj[id]->pwr_ctrl = sen_map_tbl->sendrv_reg_obj.pwr_ctrl;
		g_ctl_sen_init_obj[id]->drv_tab = sen_map_tbl->sendrv_reg_obj.drv_tab;
		g_ctl_sen_init_obj[id]->det_plug_in = sen_map_tbl->sendrv_reg_obj.det_plug_in;
	} else {
		CTL_SEN_DBG_ERR("id %d init fail\r\n", id);
		return CTL_SEN_E_MAP_TBL;
	}

	g_ctl_sen_init_obj[id]->init_cfg_obj = sen_map_tbl->init_cfg_obj;
	g_ctl_sen_init_obj[id]->init_ext_obj.if_info.timeout_ms = 1000;


	return rt;
}

static INT32 sen_module_open(CTL_SEN_ID id)
{
	INT32 rt;
	CTL_SEN_CTRL_OBJ *obj = sen_module_get_ctrl_obj(id);
	UINT32 i, cur_open_id_bit, mclksrc_sync;

	if (obj == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}

	if (sen_module_chk_op_state(id, CTL_SEN_STATE_OPEN, TRUE, 0) != TRUE) {
		if (sen_module_get_sen_map_tbl(id) == NULL) {
			return CTL_SEN_E_MAP_TBL;
		} else {
			return CTL_SEN_E_STATE;
		}
	}

	if ((rt = sen_module_open_init(id)) != CTL_SEN_E_OK) {
		return rt;
	}

	/* mclk sync only, init sync id information */
	mclksrc_sync = g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.mclksrc_sync;
	for (i = 0; i < CTL_SEN_ID_MAX; i++) {
		if (!mclksrc_sync) {
			break;
		}
		if ((mclksrc_sync) & (1 << i)) {

			if (sen_module_get_ctrl_obj(i) == NULL) {
				CTL_SEN_DBG_ERR("id %d mclksrc_sync 0x%.8x, id %d not init\r\n", id, mclksrc_sync, i);
				return CTL_SEN_E_IN_PARAM;
			}

			if ((rt = sen_module_open_init(i)) != CTL_SEN_E_OK) {
				return rt;
			}

		}
		mclksrc_sync &= ~(1 << i);
	}

	/* init ctrl obj */
	memset((void *)&obj->cur_chgmode, 0, sizeof(CTL_SEN_CHGMODE_OBJ));
	obj->cur_sen_mode = CTL_SEN_MODE_UNKNOWN;

	/* init sendrv (ad only) */
	rt = sen_ctrl_init_sendrv(id);
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("id %d sen_ctrl_init_sendrv fail\r\n", id);
		return rt;
	}

	/* cfg pinmux (except cmdif) */
	cur_open_id_bit = 0;
	for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
		if (sen_ctrl_chk_add_ctx(i)) {
			if (sen_module_is_opened(i)) {
				cur_open_id_bit |= 1 << i;
			}
		}
	}
	rt = sen_ctrl_cfg_pinmux(id, cur_open_id_bit);
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("id %d cfg pinmux fail\r\n", id);
		return rt;
	}

	rt = sen_ctrl_open(id);
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("id %d ctrl open fail\r\n", id);
		return rt;
	}

	/* prepare sie mclk */
	rt = sen_ctrl_clk_prepare(id, CTL_SEN_MODE_PWR);
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("id %d prepare clk fail\r\n", id);
		return rt;
	}

	ctl_sen_module_set_state(obj, CTL_SEN_STATE_OPEN);

	sen_module_special_reset_cnt(id);
	sen_module_unlock(FLGPTN_BIT(id) | sen_module_special_get_all_flag(id));

	return rt;
}

static INT32 sen_module_close(CTL_SEN_ID id)
{
	INT32 rt;
	UINT32 cnt, timeout = 1000, tmp;
	CTL_SEN_CTRL_OBJ *obj = sen_module_get_ctrl_obj(id);
	CTL_SEN_MODE senmode_tmp;

	if (obj == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}

	ctl_sen_module_clr_state(obj, CTL_SEN_STATE_OPEN);

	/* clear ctrl obj */
	senmode_tmp = obj->cur_sen_mode; // fore clk unprepare
	memset((void *) &obj->cur_chgmode, 0, sizeof(CTL_SEN_CHGMODE_OBJ));
	obj->cur_sen_mode = CTL_SEN_MODE_UNKNOWN;

	cnt = 0;
	while (1) {
		tmp = (ctl_sen_module_get_state(obj) & ~(CTL_SEN_STATE_INTCFG | CTL_SEN_STATE_PWRON | CTL_SEN_STATE_PWRSAVE));
		if (tmp == CTL_SEN_STATE_NONE) {
			break;
		}
		ctl_sen_delayms(5);
		cnt++;
		if (cnt >= timeout) {
			DBG_ERR("id %d timout sts=0x%.8x\r\n", id, (unsigned int)tmp);
			ctl_sen_module_set_state(obj, CTL_SEN_STATE_OPEN);
			return CTL_SEN_E_STATE;
		}
	}

	if (sen_module_chk_op_state(id, CTL_SEN_STATE_OPEN, FALSE, 0) != TRUE) {
		ctl_sen_module_set_state(obj, CTL_SEN_STATE_OPEN);
		return CTL_SEN_E_STATE;
	}

	//lock All setting
	sen_module_lock(FLGPTN_BIT(id) | sen_module_special_get_all_flag(id));

	/* unprepare sie mclk */
	rt = sen_ctrl_clk_unprepare(id, senmode_tmp);
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("id %d prepare clk fail\r\n", id);
		return rt;
	}

	rt = sen_ctrl_close(id);

	return rt;
}

static INT32 sen_module_sleep(CTL_SEN_ID id)
{
	INT32 rt;
	CTL_SEN_CTRL_OBJ *obj = sen_module_get_ctrl_obj(id);

	if (obj == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}

	ctl_sen_module_set_state(obj, CTL_SEN_STATE_SLEEP);
	if (sen_module_chk_op_state(id, CTL_SEN_STATE_SLEEP, TRUE, 0) != TRUE) {
		ctl_sen_module_clr_state(obj, CTL_SEN_STATE_SLEEP);
		return CTL_SEN_E_STATE;
	}

	sen_module_lock(FLGPTN_BIT(id));

	rt = sen_ctrl_sleep(id);

	sen_module_unlock(FLGPTN_BIT(id));

	return rt;
}

static INT32 sen_module_wakeup(CTL_SEN_ID id)
{
	INT32 rt;
	CTL_SEN_CTRL_OBJ *obj = sen_module_get_ctrl_obj(id);

	if (obj == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}

	if (sen_module_chk_op_state(id, CTL_SEN_STATE_SLEEP, FALSE, 0) != TRUE) {
		return CTL_SEN_E_STATE;
	}

	sen_module_lock(FLGPTN_BIT(id));

	rt = sen_ctrl_wakeup(id);

	ctl_sen_module_clr_state(obj, CTL_SEN_STATE_SLEEP);
	sen_module_unlock(FLGPTN_BIT(id));

	return rt;
}

static INT32 sen_module_write_reg(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	INT32 rt;
	CTL_SEN_CTRL_OBJ *obj = sen_module_get_ctrl_obj(id);

	if (obj == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}

	ctl_sen_module_set_state(obj, CTL_SEN_STATE_WRITEREG);
	if (sen_module_chk_op_state(id, CTL_SEN_STATE_WRITEREG, TRUE, 0) != TRUE) {
		ctl_sen_module_clr_state(obj, CTL_SEN_STATE_WRITEREG);
		return CTL_SEN_E_STATE;
	}
	sen_module_lock(FLGPTN_BIT(id));

	rt = sen_ctrl_write_reg(id, cmd);

	sen_module_unlock(FLGPTN_BIT(id));
	ctl_sen_module_clr_state(obj, CTL_SEN_STATE_WRITEREG);

	return rt;
}

static INT32 sen_module_read_reg(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	INT32 rt;
	CTL_SEN_CTRL_OBJ *obj = sen_module_get_ctrl_obj(id);

	if (obj == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}

	ctl_sen_module_set_state(obj, CTL_SEN_STATE_READREG);
	if (sen_module_chk_op_state(id, CTL_SEN_STATE_READREG, TRUE, 0) != TRUE) {
		ctl_sen_module_clr_state(obj, CTL_SEN_STATE_READREG);
		return CTL_SEN_E_STATE;
	}

	sen_module_lock(FLGPTN_BIT(id));

	rt = sen_ctrl_read_reg(id, cmd);

	sen_module_unlock(FLGPTN_BIT(id));
	ctl_sen_module_clr_state(obj, CTL_SEN_STATE_READREG);

	return rt;
}

static INT32 sen_module_chgmode(CTL_SEN_ID id, CTL_SEN_CHGMODE_OBJ chgmode_obj)
{
	INT32 rt;
	CTL_SEN_CTRL_OBJ *obj = sen_module_get_ctrl_obj(id);
	CTL_SEN_MODE  chg_sen_mode = CTL_SEN_MODE_UNKNOWN;

	if (obj == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}

	ctl_sen_module_set_state(obj, CTL_SEN_STATE_CHGMODE);

	if (sen_module_chk_op_state(id, CTL_SEN_STATE_CHGMODE, TRUE, 0) != TRUE) {
		ctl_sen_module_clr_state(obj, CTL_SEN_STATE_CHGMODE);
		return CTL_SEN_E_STATE;
	}

	sen_module_lock(FLGPTN_BIT(id) | sen_module_special_get_all_lock_flag(id));
	sen_module_wait_end(sen_module_special_get_all_end_flag(id));

	rt = sen_ctrl_chk_support_type(id);
	if (rt == CTL_SEN_E_OK) {
		rt = sen_ctrl_chgmode(id, chgmode_obj, &chg_sen_mode);
		if (rt == CTL_SEN_E_OK) {
			obj->cur_chgmode = chgmode_obj;
			obj->cur_sen_mode = chg_sen_mode;
		}
	}

	sen_module_unlock(FLGPTN_BIT(id) | sen_module_special_get_all_lock_flag(id));
	ctl_sen_module_clr_state(obj, CTL_SEN_STATE_CHGMODE);

	return rt;
}

static INT32 sen_module_set_cfg(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *value)
{
	INT32 rt;
	CTL_SEN_CTRL_OBJ *obj = sen_module_get_ctrl_obj(id);

	if (obj == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}


	if ((cfg_id >= CTL_SEN_CFGID_VX1_BASE) && (cfg_id < CTL_SEN_CFGID_VX1_MAX)) {
		if (sen_module_chk_op_state(id, CTL_SEN_STATE_SETCFG, TRUE, cfg_id) != TRUE) {
			return CTL_SEN_E_STATE;
		}
		rt = sen_ctrl_set_cfg(id, cfg_id, value);
	} else if ((cfg_id >= CTL_SEN_CFGID_TGE_BASE) && (cfg_id < CTL_SEN_CFGID_TGE_MAX)) {
		if (sen_module_chk_op_state(id, CTL_SEN_STATE_SETCFG, TRUE, cfg_id) != TRUE) {
			return CTL_SEN_E_STATE;
		}
		rt = sen_ctrl_set_cfg(id, cfg_id, value);
	} else {
		ctl_sen_module_set_state(obj, CTL_SEN_STATE_SETCFG);
		if (sen_module_chk_op_state(id, CTL_SEN_STATE_SETCFG, TRUE, cfg_id) != TRUE) {
			ctl_sen_module_clr_state(obj, CTL_SEN_STATE_SETCFG);
			return CTL_SEN_E_STATE;
		}
		sen_module_lock(FLGPTN_BIT(id));

		rt = sen_ctrl_set_cfg(id, cfg_id, value);

		sen_module_unlock(FLGPTN_BIT(id));
		ctl_sen_module_clr_state(obj, CTL_SEN_STATE_SETCFG);
	}


	return rt;
}
static INT32 sen_module_get_cfg(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *value)
{
	INT32 rt;
	CTL_SEN_CTRL_OBJ *obj = sen_module_get_ctrl_obj(id);

	if (obj == NULL) {
		return CTL_SEN_E_ID_OVFL;
	}


	if ((cfg_id >= CTL_SEN_CFGID_VX1_BASE) && (cfg_id < CTL_SEN_CFGID_VX1_MAX)) {
		if (sen_module_chk_op_state(id, CTL_SEN_STATE_GETCFG, TRUE, cfg_id) != TRUE) {
			return CTL_SEN_E_STATE;
		}
		rt = sen_ctrl_get_cfg(id, cfg_id, value);
	} else if ((cfg_id >= CTL_SEN_CFGID_TGE_BASE) && (cfg_id < CTL_SEN_CFGID_TGE_MAX)) {
		if (sen_module_chk_op_state(id, CTL_SEN_STATE_GETCFG, TRUE, cfg_id) != TRUE) {
			return CTL_SEN_E_STATE;
		}
		rt = sen_ctrl_get_cfg(id, cfg_id, value);
	} else {
		ctl_sen_module_set_state(obj, CTL_SEN_STATE_GETCFG);
		if (sen_module_chk_op_state(id, CTL_SEN_STATE_GETCFG, TRUE, cfg_id) != TRUE) {
			ctl_sen_module_clr_state(obj, CTL_SEN_STATE_GETCFG);
			return CTL_SEN_E_STATE;
		}

		// sie get frame number at vd interrupt, dont do module_lock to prevent wait flag in isr
		if (cfg_id != CTL_SEN_CFGID_GET_FRAME_NUMBER) {
			sen_module_special_lock(id, CTL_SEN_FUNC_GETCFG);
		}

		rt = sen_ctrl_get_cfg(id, cfg_id, value);

		if (cfg_id != CTL_SEN_CFGID_GET_FRAME_NUMBER) {
			sen_module_special_unlock(id, CTL_SEN_FUNC_GETCFG);
		}

		ctl_sen_module_clr_state(obj, CTL_SEN_STATE_GETCFG);
	}

	return rt;
}
static CTL_SEN_INTE sen_module_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag)
{
	CTL_SEN_INTE rt_inte;
	CTL_SEN_CTRL_OBJ *obj = sen_module_get_ctrl_obj(id);

	if (obj == NULL) {
		return CTL_SEN_INTE_NONE;
	}

	ctl_sen_module_set_state(obj, CTL_SEN_STATE_WAITINTE);
	if (sen_module_chk_op_state(id, CTL_SEN_STATE_WAITINTE, TRUE, 0) != TRUE) {
		ctl_sen_module_clr_state(obj, CTL_SEN_STATE_WAITINTE);
		return CTL_SEN_INTE_NONE;
	}

	sen_module_lock(FLGPTN_BIT(id));

	rt_inte = sen_ctrl_wait_interrupt(id, waited_flag);

	sen_module_unlock(FLGPTN_BIT(id));
	ctl_sen_module_clr_state(obj, CTL_SEN_STATE_WAITINTE);

	return rt_inte;
}

static INT32 sen_module_dbg_info(CTL_SEN_ID id, CTL_SEN_DBG_SEL dbg_sel, UINT32 param)
{
#define NOT_NEED_LOCK (CTL_SEN_DBG_SEL_DUMP_MAP_TBL | CTL_SEN_DBG_SEL_DUMP_PROC_TIME | CTL_SEN_DBG_SEL_DBG_MSG | CTL_SEN_DBG_SEL_DUMP_CTL | \
					   CTL_SEN_DBG_SEL_DUMP_ER | CTL_SEN_DBG_SEL_LV)
#define NOT_NEED_OPEN (CTL_SEN_DBG_SEL_DUMP | CTL_SEN_DBG_SEL_DUMP_EXT | CTL_SEN_DBG_SEL_DUMP_DRV)

	CTL_SEN_CTRL_OBJ *obj = sen_module_get_ctrl_obj(id);
	CTL_SEN_DBG_SEL dbg_sel_remain = dbg_sel;

	// NOT_NEED_LOCK not need add ctx, only check max id
	if (id >= CTL_SEN_ID_MAX_SUPPORT) {
		CTL_SEN_DBG_ERR("id %d overflow\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	if ((dbg_sel_remain & (NOT_NEED_LOCK)) != 0) {
		sen_ctrl_dbg_info(id, dbg_sel_remain & (NOT_NEED_LOCK), param);
		dbg_sel_remain &= (~NOT_NEED_LOCK);
	}
	if (dbg_sel_remain == 0) {
		return CTL_SEN_E_OK;
	}

	// need add ctx
	if (obj == NULL) {
		CTL_SEN_DBG_ERR("id %d not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}
	if ((dbg_sel_remain & (NOT_NEED_OPEN)) != 0) {
		if (!sen_module_is_opened(id)) {
			sen_ctrl_dbg_info(id, dbg_sel_remain & (NOT_NEED_OPEN), param);
			dbg_sel_remain &= (~NOT_NEED_OPEN);
		}
	}
	if (dbg_sel_remain == 0) {
		return CTL_SEN_E_OK;
	}

	if (!sen_module_is_opened(id)) {
		CTL_SEN_DBG_ERR("id %d not open\r\n", id);
		return CTL_SEN_E_STATE;
	}

	sen_module_lock(FLGPTN_BIT(id));

	sen_ctrl_dbg_info(id, dbg_sel_remain, param);

	sen_module_unlock(FLGPTN_BIT(id));

	return CTL_SEN_E_OK;
}

/***********************************************************************/
/*                                                                     */
/*                     ctl sensor common obj                           */
/*                                                                     */
/***********************************************************************/
BOOL b_add_ctx[CTL_SEN_ID_MAX_SUPPORT] = {0};
static INT32 sen_update_ctx(CTL_SEN_ID id, BOOL add)
{
	if (id >= CTL_SEN_ID_MAX_SUPPORT) {
		CTL_SEN_DBG_ERR("id %d overflow\r\n", id);
		return CTL_SEN_E_IN_PARAM;
	}

	if (add) {
		if (b_add_ctx[id]) {
			return CTL_SEN_E_OK;
		}

		if (ctl_sen_hdl_ctx.context_used >= ctl_sen_hdl_ctx.context_num) {
			CTL_SEN_DBG_ERR("overflow sen number, cur %d, max %d\r\n", ctl_sen_hdl_ctx.context_used, ctl_sen_hdl_ctx.context_num);
			return CTL_SEN_E_ID_OVFL;
		} else {
			g_ctl_sen_init_obj[id] = (CTL_SEN_INIT_OBJ *)(ctl_sen_hdl_ctx.buf_addr + (ctl_sen_hdl_ctx.context_used * ctl_sen_hdl_ctx.context_size));
			g_ctl_sen_ctrl_obj[id] = (CTL_SEN_CTRL_OBJ *)((ctl_sen_hdl_ctx.buf_addr + (ctl_sen_hdl_ctx.context_used * ctl_sen_hdl_ctx.context_size)) + sizeof(CTL_SEN_INIT_OBJ));
			ctl_sen_hdl_ctx.context_used++;

			memset((void *)(g_ctl_sen_ctrl_obj[id]), 0, sizeof(CTL_SEN_CTRL_OBJ));
			memset((void *)(g_ctl_sen_init_obj[id]), 0, sizeof(CTL_SEN_INIT_OBJ));
		}
		b_add_ctx[id] = TRUE;
	} else {
		if (!b_add_ctx[id]) {
			return CTL_SEN_E_OK;
		}
		if (ctl_sen_hdl_ctx.context_used == 0) {
			CTL_SEN_DBG_ERR("overflow sen number, cur %d\r\n", ctl_sen_hdl_ctx.context_used);
			return CTL_SEN_E_ID_OVFL;
		} else {
			ctl_sen_hdl_ctx.context_used--;
			g_ctl_sen_init_obj[id] = NULL;
			g_ctl_sen_ctrl_obj[id] = NULL;
		}
		b_add_ctx[id] = FALSE;
	}
	return CTL_SEN_E_OK;
}

static INT32 sen_open(CTL_SEN_ID id)
{
	INT32 rt = 0;

	ctl_sen_set_proc_time("open", id, CTL_SEN_PROC_TIME_ITEM_ENTER);

	if (ctl_sen_get_sem_id(ctl_sen_sem_tab[id]) == NULL) {
		CTL_SEN_DBG_ERR("sem id NULL\r\n");
		rt = CTL_SEN_E_SYS;
	} else {
		sen_module_sem(ctl_sen_get_sem_id(ctl_sen_sem_tab[id]), TRUE);

		if (g_ctl_sen_open_cnt[id] == 0) {
			rt = sen_module_open(id);
		}
		g_ctl_sen_open_cnt[id] += 1;

		sen_module_sem(ctl_sen_get_sem_id(ctl_sen_sem_tab[id]), FALSE);
	}

	ctl_sen_set_proc_time("open", id, CTL_SEN_PROC_TIME_ITEM_EXIT);

	ctl_sen_set_er(id, CTL_SEN_ER_OP_OPEN, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;

}

static INT32 sen_close(CTL_SEN_ID id)
{
	INT32 rt = 0;

	ctl_sen_set_proc_time("close", id, CTL_SEN_PROC_TIME_ITEM_ENTER);

	if (ctl_sen_get_sem_id(ctl_sen_sem_tab[id]) == NULL) {
		CTL_SEN_DBG_ERR("sem id NULL\r\n");
		rt = CTL_SEN_E_SYS;
	} else {
		sen_module_sem(ctl_sen_get_sem_id(ctl_sen_sem_tab[id]), TRUE);

		if (g_ctl_sen_open_cnt[id] > 0) {
			g_ctl_sen_open_cnt[id] -= 1;

			if (g_ctl_sen_open_cnt[id] == 0) {
				rt = sen_module_close(id);
			}
		}

		sen_module_sem(ctl_sen_get_sem_id(ctl_sen_sem_tab[id]), FALSE);
	}

	ctl_sen_set_proc_time("close", id, CTL_SEN_PROC_TIME_ITEM_EXIT);

	ctl_sen_set_er(id, CTL_SEN_ER_OP_CLOSE, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;
}

static BOOL sen_is_opened(CTL_SEN_ID id)
{
	return sen_module_is_opened(id);
}

static INT32 sen_init_cfg(CTL_SEN_ID id, CHAR *name, CTL_SEN_INIT_CFG_OBJ *init_cfg_obj)
{
	INT32 rt;

	ctl_sen_set_proc_time("init", id, CTL_SEN_PROC_TIME_ITEM_ENTER);

	sen_module_chk_global_lock(FLGPTN_BIT(CTL_SEN_FLAG_GLB_OFS_INIT));

	rt = sen_update_ctx(id, TRUE);
	if (rt == CTL_SEN_E_OK) {
		rt = sen_module_init_cfg(id, name, init_cfg_obj);
	} else {
		CTL_SEN_DBG_ERR("id %d sen_update_ctx fail, skip init\r\n", id);
	}

	ctl_sen_set_proc_time("init", id, CTL_SEN_PROC_TIME_ITEM_EXIT);

	ctl_sen_set_er(id, CTL_SEN_ER_OP_INIT, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;
}

static INT32 sen_pwr_ctrl(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag)
{
	INT32 rt;

	ctl_sen_set_proc_time("pwr", id, CTL_SEN_PROC_TIME_ITEM_ENTER);

	rt = sen_module_pwr_ctrl(id, flag);

	ctl_sen_set_proc_time("pwr", id, CTL_SEN_PROC_TIME_ITEM_EXIT);

	ctl_sen_set_er(id, CTL_SEN_ER_OP_PWR, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;
}

static INT32 sen_sleep(CTL_SEN_ID id)
{
	INT32 rt;

	ctl_sen_set_proc_time("slp", id, CTL_SEN_PROC_TIME_ITEM_ENTER);

	rt = sen_module_sleep(id);

	ctl_sen_set_proc_time("slp", id, CTL_SEN_PROC_TIME_ITEM_EXIT);

	ctl_sen_set_er(id, CTL_SEN_ER_OP_SLP, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;
}

static INT32 sen_wakeup(CTL_SEN_ID id)
{
	INT32 rt;

	ctl_sen_set_proc_time("wakeup", id, CTL_SEN_PROC_TIME_ITEM_ENTER);

	rt = sen_module_wakeup(id);

	ctl_sen_set_proc_time("wakeup", id, CTL_SEN_PROC_TIME_ITEM_EXIT);

	ctl_sen_set_er(id, CTL_SEN_ER_OP_WUP, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;
}

static INT32 sen_write_reg(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	INT32 rt = 0;

	ctl_sen_set_op_io(id, CTL_SEN_ER_OP_WR, CTL_SEN_PROC_TIME_ITEM_ENTER);

	rt = sen_module_write_reg(id, cmd);

	ctl_sen_set_op_io(id, CTL_SEN_ER_OP_WR, CTL_SEN_PROC_TIME_ITEM_EXIT);

	ctl_sen_set_er(id, CTL_SEN_ER_OP_WR, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;
}

static INT32 sen_read_reg(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	INT32 rt = 0;

	ctl_sen_set_op_io(id, CTL_SEN_ER_OP_RR, CTL_SEN_PROC_TIME_ITEM_ENTER);

	rt = sen_module_read_reg(id, cmd);

	ctl_sen_set_op_io(id, CTL_SEN_ER_OP_RR, CTL_SEN_PROC_TIME_ITEM_EXIT);

	ctl_sen_set_er(id, CTL_SEN_ER_OP_RR, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;
}

static INT32 sen_chgmode(CTL_SEN_ID id, CTL_SEN_CHGMODE_OBJ chgmode_obj)
{
	INT32 rt = 0;

	ctl_sen_set_proc_time("chgmode", id, CTL_SEN_PROC_TIME_ITEM_ENTER);

	rt = sen_module_chgmode(id, chgmode_obj);

	ctl_sen_set_proc_time("chgmode", id, CTL_SEN_PROC_TIME_ITEM_EXIT);

	ctl_sen_set_er(id, CTL_SEN_ER_OP_CHG, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;
}

static INT32 sen_set_cfg(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *value)
{
	INT32 rt = 0;

	ctl_sen_set_op_io(id, CTL_SEN_ER_OP_SET, CTL_SEN_PROC_TIME_ITEM_ENTER);

	rt = sen_module_set_cfg(id, cfg_id, value);

	ctl_sen_set_op_io(id, CTL_SEN_ER_OP_SET, CTL_SEN_PROC_TIME_ITEM_EXIT);

	ctl_sen_set_er(id, CTL_SEN_ER_OP_SET, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;
}

static INT32 sen_get_cfg(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *value)
{
	INT32 rt = 0;

	ctl_sen_set_op_io(id, CTL_SEN_ER_OP_GET, CTL_SEN_PROC_TIME_ITEM_ENTER);

	rt = sen_module_get_cfg(id, cfg_id, value);

	ctl_sen_set_op_io(id, CTL_SEN_ER_OP_GET, CTL_SEN_PROC_TIME_ITEM_EXIT);

	ctl_sen_set_er(id, CTL_SEN_ER_OP_GET, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;
}

static CTL_SEN_INTE sen_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag)
{
	return sen_module_wait_interrupt(id, waited_flag);
}

static INT32 sen_dbg_info(CTL_SEN_ID id, CTL_SEN_DBG_SEL dbg_sel, UINT32 param)
{
	INT32 rt = 0;

	rt = sen_module_dbg_info(id, dbg_sel, param);

	return rt;
}

/***********************************************************************/
/*                                                                     */
/*                         ctl sensor obj                              */
/*                                                                     */
/***********************************************************************/
/*
    CTL_SEN_ID_1 ctl obj
*/
static INT32 sen_init_cfg_1(CHAR *name, CTL_SEN_INIT_CFG_OBJ *init_cfg_obj)
{
	return sen_init_cfg(CTL_SEN_ID_1, name, init_cfg_obj);
}

static INT32 sen_open_1(void)
{
	return sen_open(CTL_SEN_ID_1);
}

static INT32 sen_close_1(void)
{
	return sen_close(CTL_SEN_ID_1);
}

static INT32 sen_pwr_ctrl_1(CTL_SEN_PWR_CTRL_FLAG flag)
{
	return sen_pwr_ctrl(CTL_SEN_ID_1, flag);
}

static BOOL sen_is_opened_1(void)
{
	return sen_is_opened(CTL_SEN_ID_1);
}

static INT32 sen_sleep_1(void)
{
	return sen_sleep(CTL_SEN_ID_1);
}

static INT32 sen_wakeup_1(void)
{
	return sen_wakeup(CTL_SEN_ID_1);
}

static INT32 sen_write_reg_1(CTL_SEN_CMD *cmd)
{
	return sen_write_reg(CTL_SEN_ID_1, cmd);
}

static INT32 sen_read_reg_1(CTL_SEN_CMD *cmd)
{
	return sen_read_reg(CTL_SEN_ID_1, cmd);
}

static INT32 sen_chgmode_1(CTL_SEN_CHGMODE_OBJ chgmode_obj)
{
	return sen_chgmode(CTL_SEN_ID_1, chgmode_obj);
}

static INT32 sen_set_cfg_1(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_set_cfg(CTL_SEN_ID_1, cfg_id, value);
}

static INT32 sen_get_cfg_1(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_get_cfg(CTL_SEN_ID_1, cfg_id, value);
}

static CTL_SEN_INTE sen_wait_interrupt_1(CTL_SEN_INTE waited_flag)
{
	return sen_wait_interrupt(CTL_SEN_ID_1, waited_flag);
}

static INT32 sen_dbg_info_1(CTL_SEN_DBG_SEL dbg_sel, UINT32 param)
{
	return sen_dbg_info(CTL_SEN_ID_1, dbg_sel, param);
}

/*
    CTL_SEN_ID_2 ctl obj
*/
static INT32 sen_init_cfg_2(CHAR *name, CTL_SEN_INIT_CFG_OBJ *init_cfg_obj)
{
	return sen_init_cfg(CTL_SEN_ID_2, name, init_cfg_obj);
}

static INT32 sen_open_2(void)
{
	return sen_open(CTL_SEN_ID_2);
}

static INT32 sen_close_2(void)
{
	return sen_close(CTL_SEN_ID_2);
}

static INT32 sen_pwr_ctrl_2(CTL_SEN_PWR_CTRL_FLAG flag)
{
	return sen_pwr_ctrl(CTL_SEN_ID_2, flag);
}

static BOOL sen_is_opened_2(void)
{
	return sen_is_opened(CTL_SEN_ID_2);
}

static INT32 sen_sleep_2(void)
{
	return sen_sleep(CTL_SEN_ID_2);
}

static INT32 sen_wakeup_2(void)
{
	return sen_wakeup(CTL_SEN_ID_2);
}

static INT32 sen_write_reg_2(CTL_SEN_CMD *cmd)
{
	return sen_write_reg(CTL_SEN_ID_2, cmd);
}

static INT32 sen_read_reg_2(CTL_SEN_CMD *cmd)
{
	return sen_read_reg(CTL_SEN_ID_2, cmd);
}

static INT32 sen_chgmode_2(CTL_SEN_CHGMODE_OBJ chgmode_obj)
{
	return sen_chgmode(CTL_SEN_ID_2, chgmode_obj);
}

static INT32 sen_set_cfg_2(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_set_cfg(CTL_SEN_ID_2, cfg_id, value);
}

static INT32 sen_get_cfg_2(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_get_cfg(CTL_SEN_ID_2, cfg_id, value);
}

static CTL_SEN_INTE sen_wait_interrupt_2(CTL_SEN_INTE waited_flag)
{
	return sen_wait_interrupt(CTL_SEN_ID_2, waited_flag);
}

static INT32 sen_dbg_info_2(CTL_SEN_DBG_SEL dbg_sel, UINT32 param)
{
	return sen_dbg_info(CTL_SEN_ID_2, dbg_sel, param);
}

/*
    CTL_SEN_ID_3 ctl obj
*/
static INT32 sen_init_cfg_3(CHAR *name, CTL_SEN_INIT_CFG_OBJ *init_cfg_obj)
{
	return sen_init_cfg(CTL_SEN_ID_3, name, init_cfg_obj);
}

static INT32 sen_open_3(void)
{
	return sen_open(CTL_SEN_ID_3);
}

static INT32 sen_close_3(void)
{
	return sen_close(CTL_SEN_ID_3);
}

static INT32 sen_pwr_ctrl_3(CTL_SEN_PWR_CTRL_FLAG flag)
{
	return sen_pwr_ctrl(CTL_SEN_ID_3, flag);
}

static BOOL sen_is_opened_3(void)
{
	return sen_is_opened(CTL_SEN_ID_3);
}

static INT32 sen_sleep_3(void)
{
	return sen_sleep(CTL_SEN_ID_3);
}

static INT32 sen_wakeup_3(void)
{
	return sen_wakeup(CTL_SEN_ID_3);
}

static INT32 sen_write_reg_3(CTL_SEN_CMD *cmd)
{
	return sen_write_reg(CTL_SEN_ID_3, cmd);
}

static INT32 sen_read_reg_3(CTL_SEN_CMD *cmd)
{
	return sen_read_reg(CTL_SEN_ID_3, cmd);
}

static INT32 sen_chgmode_3(CTL_SEN_CHGMODE_OBJ chgmode_obj)
{
	return sen_chgmode(CTL_SEN_ID_3, chgmode_obj);
}

static INT32 sen_set_cfg_3(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_set_cfg(CTL_SEN_ID_3, cfg_id, value);
}

static INT32 sen_get_cfg_3(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_get_cfg(CTL_SEN_ID_3, cfg_id, value);
}

static CTL_SEN_INTE sen_wait_interrupt_3(CTL_SEN_INTE waited_flag)
{
	return sen_wait_interrupt(CTL_SEN_ID_3, waited_flag);
}

static INT32 sen_dbg_info_3(CTL_SEN_DBG_SEL dbg_sel, UINT32 param)
{
	return sen_dbg_info(CTL_SEN_ID_3, dbg_sel, param);
}

/*
    CTL_SEN_ID_4 ctl obj
*/
static INT32 sen_init_cfg_4(CHAR *name, CTL_SEN_INIT_CFG_OBJ *init_cfg_obj)
{
	return sen_init_cfg(CTL_SEN_ID_4, name, init_cfg_obj);
}

static INT32 sen_open_4(void)
{
	return sen_open(CTL_SEN_ID_4);
}

static INT32 sen_close_4(void)
{
	return sen_close(CTL_SEN_ID_4);
}

static INT32 sen_pwr_ctrl_4(CTL_SEN_PWR_CTRL_FLAG flag)
{
	return sen_pwr_ctrl(CTL_SEN_ID_4, flag);
}

static BOOL sen_is_opened_4(void)
{
	return sen_is_opened(CTL_SEN_ID_4);
}

static INT32 sen_sleep_4(void)
{
	return sen_sleep(CTL_SEN_ID_4);
}

static INT32 sen_wakeup_4(void)
{
	return sen_wakeup(CTL_SEN_ID_4);
}

static INT32 sen_write_reg_4(CTL_SEN_CMD *cmd)
{
	return sen_write_reg(CTL_SEN_ID_4, cmd);
}

static INT32 sen_read_reg_4(CTL_SEN_CMD *cmd)
{
	return sen_read_reg(CTL_SEN_ID_4, cmd);
}

static INT32 sen_chgmode_4(CTL_SEN_CHGMODE_OBJ chgmode_obj)
{
	return sen_chgmode(CTL_SEN_ID_4, chgmode_obj);
}

static INT32 sen_set_cfg_4(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_set_cfg(CTL_SEN_ID_4, cfg_id, value);
}

static INT32 sen_get_cfg_4(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_get_cfg(CTL_SEN_ID_4, cfg_id, value);
}

static CTL_SEN_INTE sen_wait_interrupt_4(CTL_SEN_INTE waited_flag)
{
	return sen_wait_interrupt(CTL_SEN_ID_4, waited_flag);
}

static INT32 sen_dbg_info_4(CTL_SEN_DBG_SEL dbg_sel, UINT32 param)
{
	return sen_dbg_info(CTL_SEN_ID_4, dbg_sel, param);
}

/*
    CTL_SEN_ID_5 ctl obj
*/
static INT32 sen_init_cfg_5(CHAR *name, CTL_SEN_INIT_CFG_OBJ *init_cfg_obj)
{
	return sen_init_cfg(CTL_SEN_ID_5, name, init_cfg_obj);
}

static INT32 sen_open_5(void)
{
	return sen_open(CTL_SEN_ID_5);
}

static INT32 sen_close_5(void)
{
	return sen_close(CTL_SEN_ID_5);
}

static INT32 sen_pwr_ctrl_5(CTL_SEN_PWR_CTRL_FLAG flag)
{
	return sen_pwr_ctrl(CTL_SEN_ID_5, flag);
}

static BOOL sen_is_opened_5(void)
{
	return sen_is_opened(CTL_SEN_ID_5);
}

static INT32 sen_sleep_5(void)
{
	return sen_sleep(CTL_SEN_ID_5);
}

static INT32 sen_wakeup_5(void)
{
	return sen_wakeup(CTL_SEN_ID_5);
}

static INT32 sen_write_reg_5(CTL_SEN_CMD *cmd)
{
	return sen_write_reg(CTL_SEN_ID_5, cmd);
}

static INT32 sen_read_reg_5(CTL_SEN_CMD *cmd)
{
	return sen_read_reg(CTL_SEN_ID_5, cmd);
}

static INT32 sen_chgmode_5(CTL_SEN_CHGMODE_OBJ chgmode_obj)
{
	return sen_chgmode(CTL_SEN_ID_5, chgmode_obj);
}

static INT32 sen_set_cfg_5(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_set_cfg(CTL_SEN_ID_5, cfg_id, value);
}

static INT32 sen_get_cfg_5(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_get_cfg(CTL_SEN_ID_5, cfg_id, value);
}

static CTL_SEN_INTE sen_wait_interrupt_5(CTL_SEN_INTE waited_flag)
{
	return sen_wait_interrupt(CTL_SEN_ID_5, waited_flag);
}

static INT32 sen_dbg_info_5(CTL_SEN_DBG_SEL dbg_sel, UINT32 param)
{
	return sen_dbg_info(CTL_SEN_ID_5, dbg_sel, param);
}

/*
    CTL_SEN_ID_6 ctl obj
*/
static INT32 sen_init_cfg_6(CHAR *name, CTL_SEN_INIT_CFG_OBJ *init_cfg_obj)
{
	return sen_init_cfg(CTL_SEN_ID_6, name, init_cfg_obj);
}

static INT32 sen_open_6(void)
{
	return sen_open(CTL_SEN_ID_6);
}

static INT32 sen_close_6(void)
{
	return sen_close(CTL_SEN_ID_6);
}

static INT32 sen_pwr_ctrl_6(CTL_SEN_PWR_CTRL_FLAG flag)
{
	return sen_pwr_ctrl(CTL_SEN_ID_6, flag);
}

static BOOL sen_is_opened_6(void)
{
	return sen_is_opened(CTL_SEN_ID_6);
}

static INT32 sen_sleep_6(void)
{
	return sen_sleep(CTL_SEN_ID_6);
}

static INT32 sen_wakeup_6(void)
{
	return sen_wakeup(CTL_SEN_ID_6);
}

static INT32 sen_write_reg_6(CTL_SEN_CMD *cmd)
{
	return sen_write_reg(CTL_SEN_ID_6, cmd);
}

static INT32 sen_read_reg_6(CTL_SEN_CMD *cmd)
{
	return sen_read_reg(CTL_SEN_ID_6, cmd);
}

static INT32 sen_chgmode_6(CTL_SEN_CHGMODE_OBJ chgmode_obj)
{
	return sen_chgmode(CTL_SEN_ID_6, chgmode_obj);
}

static INT32 sen_set_cfg_6(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_set_cfg(CTL_SEN_ID_6, cfg_id, value);
}

static INT32 sen_get_cfg_6(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_get_cfg(CTL_SEN_ID_6, cfg_id, value);
}

static CTL_SEN_INTE sen_wait_interrupt_6(CTL_SEN_INTE waited_flag)
{
	return sen_wait_interrupt(CTL_SEN_ID_6, waited_flag);
}

static INT32 sen_dbg_info_6(CTL_SEN_DBG_SEL dbg_sel, UINT32 param)
{
	return sen_dbg_info(CTL_SEN_ID_6, dbg_sel, param);
}

/*
    CTL_SEN_ID_7 ctl obj
*/
static INT32 sen_init_cfg_7(CHAR *name, CTL_SEN_INIT_CFG_OBJ *init_cfg_obj)
{
	return sen_init_cfg(CTL_SEN_ID_7, name, init_cfg_obj);
}

static INT32 sen_open_7(void)
{
	return sen_open(CTL_SEN_ID_7);
}

static INT32 sen_close_7(void)
{
	return sen_close(CTL_SEN_ID_7);
}

static INT32 sen_pwr_ctrl_7(CTL_SEN_PWR_CTRL_FLAG flag)
{
	return sen_pwr_ctrl(CTL_SEN_ID_7, flag);
}

static BOOL sen_is_opened_7(void)
{
	return sen_is_opened(CTL_SEN_ID_7);
}

static INT32 sen_sleep_7(void)
{
	return sen_sleep(CTL_SEN_ID_7);
}

static INT32 sen_wakeup_7(void)
{
	return sen_wakeup(CTL_SEN_ID_7);
}

static INT32 sen_write_reg_7(CTL_SEN_CMD *cmd)
{
	return sen_write_reg(CTL_SEN_ID_7, cmd);
}

static INT32 sen_read_reg_7(CTL_SEN_CMD *cmd)
{
	return sen_read_reg(CTL_SEN_ID_7, cmd);
}

static INT32 sen_chgmode_7(CTL_SEN_CHGMODE_OBJ chgmode_obj)
{
	return sen_chgmode(CTL_SEN_ID_7, chgmode_obj);
}

static INT32 sen_set_cfg_7(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_set_cfg(CTL_SEN_ID_7, cfg_id, value);
}

static INT32 sen_get_cfg_7(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_get_cfg(CTL_SEN_ID_7, cfg_id, value);
}

static CTL_SEN_INTE sen_wait_interrupt_7(CTL_SEN_INTE waited_flag)
{
	return sen_wait_interrupt(CTL_SEN_ID_7, waited_flag);
}

static INT32 sen_dbg_info_7(CTL_SEN_DBG_SEL dbg_sel, UINT32 param)
{
	return sen_dbg_info(CTL_SEN_ID_7, dbg_sel, param);
}

/*
    CTL_SEN_ID_8 ctl obj
*/
static INT32 sen_init_cfg_8(CHAR *name, CTL_SEN_INIT_CFG_OBJ *init_cfg_obj)
{
	return sen_init_cfg(CTL_SEN_ID_8, name, init_cfg_obj);
}

static INT32 sen_open_8(void)
{
	return sen_open(CTL_SEN_ID_8);
}

static INT32 sen_close_8(void)
{
	return sen_close(CTL_SEN_ID_8);
}

static INT32 sen_pwr_ctrl_8(CTL_SEN_PWR_CTRL_FLAG flag)
{
	return sen_pwr_ctrl(CTL_SEN_ID_8, flag);
}

static BOOL sen_is_opened_8(void)
{
	return sen_is_opened(CTL_SEN_ID_8);
}

static INT32 sen_sleep_8(void)
{
	return sen_sleep(CTL_SEN_ID_8);
}

static INT32 sen_wakeup_8(void)
{
	return sen_wakeup(CTL_SEN_ID_8);
}

static INT32 sen_write_reg_8(CTL_SEN_CMD *cmd)
{
	return sen_write_reg(CTL_SEN_ID_8, cmd);
}

static INT32 sen_read_reg_8(CTL_SEN_CMD *cmd)
{
	return sen_read_reg(CTL_SEN_ID_8, cmd);
}

static INT32 sen_chgmode_8(CTL_SEN_CHGMODE_OBJ chgmode_obj)
{
	return sen_chgmode(CTL_SEN_ID_8, chgmode_obj);
}

static INT32 sen_set_cfg_8(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_set_cfg(CTL_SEN_ID_8, cfg_id, value);
}

static INT32 sen_get_cfg_8(CTL_SEN_CFGID cfg_id, void *value)
{
	return sen_get_cfg(CTL_SEN_ID_8, cfg_id, value);
}

static CTL_SEN_INTE sen_wait_interrupt_8(CTL_SEN_INTE waited_flag)
{
	return sen_wait_interrupt(CTL_SEN_ID_8, waited_flag);
}

static INT32 sen_dbg_info_8(CTL_SEN_DBG_SEL dbg_sel, UINT32 param)
{
	return sen_dbg_info(CTL_SEN_ID_8, dbg_sel, param);
}

static CTL_SEN_OBJ ctl_sen_obj[CTL_SEN_ID_MAX] = {
	{
		CTL_SEN_ID_1, sen_init_cfg_1, sen_open_1, sen_close_1, sen_is_opened_1, sen_pwr_ctrl_1, sen_sleep_1, sen_wakeup_1
		, sen_write_reg_1, sen_read_reg_1, sen_chgmode_1, sen_set_cfg_1, sen_get_cfg_1, sen_wait_interrupt_1, sen_dbg_info_1
	},
	{
		CTL_SEN_ID_2, sen_init_cfg_2, sen_open_2, sen_close_2, sen_is_opened_2, sen_pwr_ctrl_2, sen_sleep_2, sen_wakeup_2
		, sen_write_reg_2, sen_read_reg_2, sen_chgmode_2, sen_set_cfg_2, sen_get_cfg_2, sen_wait_interrupt_2, sen_dbg_info_2
	},
	{
		CTL_SEN_ID_3, sen_init_cfg_3, sen_open_3, sen_close_3, sen_is_opened_3, sen_pwr_ctrl_3, sen_sleep_3, sen_wakeup_3
		, sen_write_reg_3, sen_read_reg_3, sen_chgmode_3, sen_set_cfg_3, sen_get_cfg_3, sen_wait_interrupt_3, sen_dbg_info_3
	},
	{
		CTL_SEN_ID_4, sen_init_cfg_4, sen_open_4, sen_close_4, sen_is_opened_4, sen_pwr_ctrl_4, sen_sleep_4, sen_wakeup_4
		, sen_write_reg_4, sen_read_reg_4, sen_chgmode_4, sen_set_cfg_4, sen_get_cfg_4, sen_wait_interrupt_4, sen_dbg_info_4
	},
	{
		CTL_SEN_ID_5, sen_init_cfg_5, sen_open_5, sen_close_5, sen_is_opened_5, sen_pwr_ctrl_5, sen_sleep_5, sen_wakeup_5
		, sen_write_reg_5, sen_read_reg_5, sen_chgmode_5, sen_set_cfg_5, sen_get_cfg_5, sen_wait_interrupt_5, sen_dbg_info_5
	},
	{
		CTL_SEN_ID_6, sen_init_cfg_6, sen_open_6, sen_close_6, sen_is_opened_6, sen_pwr_ctrl_6, sen_sleep_6, sen_wakeup_6
		, sen_write_reg_6, sen_read_reg_6, sen_chgmode_6, sen_set_cfg_6, sen_get_cfg_6, sen_wait_interrupt_6, sen_dbg_info_6
	},
	{
		CTL_SEN_ID_7, sen_init_cfg_7, sen_open_7, sen_close_7, sen_is_opened_7, sen_pwr_ctrl_7, sen_sleep_7, sen_wakeup_7
		, sen_write_reg_7, sen_read_reg_7, sen_chgmode_7, sen_set_cfg_7, sen_get_cfg_7, sen_wait_interrupt_7, sen_dbg_info_7
	},
	{
		CTL_SEN_ID_8, sen_init_cfg_8, sen_open_8, sen_close_8, sen_is_opened_8, sen_pwr_ctrl_8, sen_sleep_8, sen_wakeup_8
		, sen_write_reg_8, sen_read_reg_8, sen_chgmode_8, sen_set_cfg_8, sen_get_cfg_8, sen_wait_interrupt_8, sen_dbg_info_8
	},
};


/***********************************************************************/
/*                                                                     */
/*                    ctl sensor external api                          */
/*                                                                     */
/***********************************************************************/
/**
    Get the Sensor Interface control object
*/
PCTL_SEN_OBJ ctl_sen_get_object(CTL_SEN_ID sen_id)
{
	if (sen_id >= CTL_SEN_ID_MAX_INPUT) {
		CTL_SEN_DBG_ERR("No such CTL_SEN_ID.%d\r\n", sen_id);
		return NULL;
	}

	return &ctl_sen_obj[sen_id];
}

#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
int kflow_ctl_sen_init(void)
{
	return 0;
}
int kflow_ctl_sen_uninit(void)
{
	return 0;
}
#endif

/**
    buffer ctrl
*/
UINT32 ctl_sen_buf_query(UINT32 num)
{
	INT32 rt = 0;
	UINT32 total_buf_sz = 0;

	ctl_sen_set_proc_time("buf_q", 0, CTL_SEN_PROC_TIME_ITEM_ENTER);

	if ((num > CTL_SEN_ID_MAX_INPUT) || (num == 0)) {
		CTL_SEN_DBG_ERR("input num %d overflow (>%d) or underflow\r\n", num, CTL_SEN_ID_MAX_INPUT);
		rt = CTL_SEN_E_IN_PARAM;
		goto END;
	}

	ctl_sen_hdl_ctx.context_num = num;
	ctl_sen_hdl_ctx.context_size = CTL_SEN_CONTEXT_SIZE;
	ctl_sen_hdl_ctx.kflow_size = num * ctl_sen_hdl_ctx.context_size;
	ctl_sen_hdl_ctx.kdrv_size = CTL_SEN_KDRV_BUF(num);
	total_buf_sz = ctl_sen_hdl_ctx.kflow_size + ctl_sen_hdl_ctx.kdrv_size;

	CTL_SEN_DBG_IND("total_buf_sz 0x%.8x = num(%d)*ctx_size(0x%.8x) + kdrv_size(0x%.8x)\r\n"
					, total_buf_sz, ctl_sen_hdl_ctx.context_num, ctl_sen_hdl_ctx.context_size, ctl_sen_hdl_ctx.kdrv_size);

END:
	ctl_sen_set_proc_time("buf_q", 0, CTL_SEN_PROC_TIME_ITEM_EXIT);

	ctl_sen_set_er(0, CTL_SEN_ER_OP_BUF_QUE, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return total_buf_sz;
}

INT32 ctl_sen_init(UINT32 buf_addr, UINT32 buf_size)
{
	INT32 rt = 0;

	ctl_sen_set_proc_time("g_init", 0, CTL_SEN_PROC_TIME_ITEM_ENTER);

	if (ctl_sen_hdl_ctx.b_init) {
		CTL_SEN_DBG_WRN("is init\r\n");
		rt = CTL_SEN_E_STATE;
		goto END;
	}

	// check input buffer size enough
	if ((ctl_sen_hdl_ctx.kflow_size * ctl_sen_hdl_ctx.kdrv_size) > buf_size) {
		CTL_SEN_DBG_ERR("input buf size 0x%.8x not enough (flow %d drv 0x%.8x)\r\n", buf_size, ctl_sen_hdl_ctx.kflow_size, ctl_sen_hdl_ctx.kdrv_size);
		rt = CTL_SEN_E_IN_PARAM;
		goto END;
	}

#if 1 // JIRA[NA51055-699] fast boot issue
	if (buf_addr == 0) {
		rt = ctl_sen_util_os_malloc(&ctl_sen_mem_info, buf_size);
		if (rt) {
			goto END;
		}
		if (ctl_sen_mem_info.cma_info.vaddr == 0) {
			CTL_SEN_DBG_ERR("allocate buffer fail\r\n");
			goto END;
		}
		ctl_sen_hdl_ctx.local_alloc = TRUE;
		buf_addr = (UINT32)ctl_sen_mem_info.cma_info.vaddr;
	} else {
		ctl_sen_hdl_ctx.local_alloc = FALSE;
	}
#else
	if (buf_addr == 0) {
		CTL_SEN_DBG_ERR("addr=0\r\n");
		rt = CTL_SEN_E_IN_PARAM;
		goto END;
	}
#endif

	ctl_sen_hdl_ctx.buf_addr = buf_addr;
	ctl_sen_hdl_ctx.buf_size = buf_size;
	ctl_sen_hdl_ctx.b_init = TRUE;

	ctl_sen_install_id();

	sen_module_global_unlock(FLGPTN_BIT(CTL_SEN_FLAG_GLB_OFS_INIT));

END:
	ctl_sen_set_proc_time("g_init", 0, CTL_SEN_PROC_TIME_ITEM_EXIT);

	ctl_sen_set_er(0, CTL_SEN_ER_OP_GLOBAL_INIT, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;
}

INT32 ctl_sen_uninit(void)
{
	INT32 rt = 0;
	UINT32 i;

	ctl_sen_set_proc_time("g_uninit", 0, CTL_SEN_PROC_TIME_ITEM_ENTER);

	sen_module_global_lock(FLGPTN_BIT(CTL_SEN_FLAG_GLB_OFS_INIT));

	if (!ctl_sen_hdl_ctx.b_init) {
		CTL_SEN_DBG_WRN("not init\r\n");
		rt = CTL_SEN_E_STATE;
		goto END;
	}

	for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
		if (sen_ctrl_chk_add_ctx(i)) {
			if (sen_module_is_opened(i)) {
				CTL_SEN_DBG_ERR("sensor id %d not close, uninit fail!!\r\n", i);
				rt = CTL_SEN_E_STATE;
				goto END;
			}
		}
	}

	for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
		rt = sen_update_ctx((CTL_SEN_ID)i, FALSE);
		if (rt != CTL_SEN_E_OK) {
			CTL_SEN_DBG_ERR("id %d update ctx fail\r\n", i);
			goto END;
		}
	}

	for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
		rt = sen_module_clr_sen_map_tbl_initcfg((CTL_SEN_ID)i);
		if (rt != CTL_SEN_E_OK) {
			CTL_SEN_DBG_ERR("id %d clr initcfg fail\r\n", i);
			goto END;
		}
	}

	ctl_sen_uninstall_id();

	if (ctl_sen_hdl_ctx.local_alloc) {
		rt = ctl_sen_util_os_mfree(&ctl_sen_mem_info);
		if (rt) {
			goto END;
		}
	}

	ctl_sen_hdl_ctx.buf_addr = 0;
	ctl_sen_hdl_ctx.buf_size = 0;
	ctl_sen_hdl_ctx.b_init = FALSE;

END:
	ctl_sen_set_proc_time("g_uninit", 0, CTL_SEN_PROC_TIME_ITEM_EXIT);

	ctl_sen_set_er(0, CTL_SEN_ER_OP_GLOBAL_UNINIT, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;
}

/**
    sendrv register and unregister
*/
INT32 ctl_sen_reg_sendrv(CHAR *name, CTL_SEN_REG_OBJ *reg_obj)
{
	INT32 rt;
	CTL_SEN_ID id;

	CTL_SEN_DBG_IND("name %s\r\n", name);

	id = sen_module_get_sen_map_tbl_id(name);
	if (id != CTL_SEN_ID_MAX) {
		if (sen_module_chk_op_state(id, CTL_SEN_STATE_REGSEN, TRUE, 0) != TRUE) {
			return CTL_SEN_E_STATE;
		}
	}
	rt = sen_module_set_sen_map_tbl(CTL_SEN_ID_MAX, CTL_SEN_MAP_TBL_ITEM_SENREG, name, reg_obj, NULL);

	ctl_sen_set_er(0, CTL_SEN_ER_OP_REG, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;
}

INT32 ctl_sen_unreg_sendrv(CHAR *name)
{
	INT32 rt;
	CTL_SEN_ID id;

	CTL_SEN_DBG_IND("name %s\r\n", name);

	id = sen_module_get_sen_map_tbl_id(name);
	if (id != CTL_SEN_ID_MAX) {
		if (sen_module_chk_op_state(id, CTL_SEN_STATE_REGSEN, FALSE, 0) != TRUE) {
			return CTL_SEN_E_STATE;
		}
	}

	rt = sen_module_set_sen_map_tbl(CTL_SEN_ID_MAX, CTL_SEN_MAP_TBL_ITEM_SENREG, name, NULL, NULL);

	ctl_sen_set_er(0, CTL_SEN_ER_OP_UNREG, CTL_SEN_ER_ITEM_OUTPUT, rt);

	return rt;
}

#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
#else
EXPORT_SYMBOL(ctl_sen_get_object);
EXPORT_SYMBOL(ctl_sen_buf_query);
EXPORT_SYMBOL(ctl_sen_init);
EXPORT_SYMBOL(ctl_sen_uninit);
EXPORT_SYMBOL(ctl_sen_reg_sendrv);
EXPORT_SYMBOL(ctl_sen_unreg_sendrv);
#endif


// EOF
