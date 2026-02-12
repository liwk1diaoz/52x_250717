#include "msdcnvt_int.h"
#include "msdcnvt_adj.h"
//for MSDCVendorNVT_AddCallback_Bi
static void msdcnvtcb_adj_get_num_of_item(void *p_data);
static void msdcnvtcb_adj_get_items_desc(void *p_data);
static void msdcnvtcb_adj_set_items_data(void *p_data);
static void msdcnvtcb_adj_get_items_data(void *p_data);

static MSDCNVT_BI_CALL_UNIT m_msdcnvt_adj[] = {
	{"MsdcNvtCb_AdjGetNumOfItem", msdcnvtcb_adj_get_num_of_item},
	{"MsdcNvtCb_AdjGetItemsDesc", msdcnvtcb_adj_get_items_desc},
	{"MsdcNvtCb_AdjSetItemsData", msdcnvtcb_adj_set_items_data},
	{"MsdcNvtCb_AdjGetItemsData", msdcnvtcb_adj_get_items_data},
	{NULL, NULL},
};

BOOL msdcnvtregbi_adj(void)
{
	return msdcnvt_add_callback_bi(m_msdcnvt_adj);
}

//MsdcNvtCb_Adj.c
typedef enum _VAR_TYPE {
	VAR_TYPE_BOOL,
	VAR_TYPE_UINT32,
	VAR_TYPE_INT32,
	VAR_TYPE_UINT16,
	VAR_TYPE_INT16,
	VAR_TYPE_UINT8,
	VAR_TYPE_INT8
} VAR_TYPE;

typedef struct _ADJ_ITEM {
	void       *p_var;
	VAR_TYPE    var_type;
	INT32       max_value;
	INT32       min_value;
	CHAR        tag[64];
} ADJ_ITEM;

//Step 1: Includes
//Step 2: Variables Control
//You can extern these variables at other places for temp adjustment
UINT32 m_param_0 = 0;
UINT32 m_param_1 = 0;
UINT32 m_param_2 = 0;
UINT32 m_param_3 = 0;
UINT32 m_param_4 = 0;
UINT32 m_param_5 = 0;
//Step 3: Setup Up Item
static ADJ_ITEM m_items[] = {
	{&m_param_0, VAR_TYPE_UINT32, 255, 0, "m_param_0"},
	{&m_param_1, VAR_TYPE_UINT32, 255, 0, "m_param_1"},
	{&m_param_2, VAR_TYPE_UINT32, 255, 0, "m_param_2"},
	{&m_param_3, VAR_TYPE_UINT32, 255, 0, "m_param_3"},
	{&m_param_4, VAR_TYPE_UINT32, 255, 0, "m_param_4"},
	{&m_param_5, VAR_TYPE_UINT32, 255, 0, "m_param_5"},
};
//Step 4: (Optional) Implement Get Callback, as xxx_get_xxx to refresh your variable controls
static void item_get_data(void)
{
}
//Step 5: Implement Set Callback, as xxx_set_xxx to set variables to active
static void item_set_data(void)
{
}

static void msdcnvtcb_adj_get_num_of_item(void *p_data)
{
	MSDCEXT_UINT32 *p_desc = MSDCNVT_CAST(MSDCEXT_UINT32, p_data);

	if (p_desc == NULL) {
		DBG_ERR("Invalid input data.\r\n");
		return;
	}

	p_desc->value = sizeof(m_items) / sizeof(m_items[0]);
	p_desc->parent.b_ok = TRUE;
}

static void msdcnvtcb_adj_get_items_desc(void *p_data)
{
	MSDCEXT_ADJ_ALL_DESC *p_desc = MSDCNVT_CAST(MSDCEXT_ADJ_ALL_DESC, p_data);
	MSDCEXT_ADJ_ITEM_DESC *p_item_desc = (p_desc == NULL) ? NULL : (MSDCEXT_ADJ_ITEM_DESC *)(&p_desc[1]);
	UINT32 i, item_cnt = sizeof(m_items) / sizeof(m_items[0]);

	if (p_desc == NULL) {
		DBG_ERR("Invalid input data.\r\n");
		return;
	}

	for (i = 0; i < item_cnt; i++) {
		ADJ_ITEM *p_src = &m_items[i];
		MSDCEXT_ADJ_ITEM_DESC *p_dst = p_item_desc + i;

		memset(p_dst, 0, sizeof(MSDCEXT_ADJ_ITEM_DESC));
		switch (p_src->var_type) {
		case VAR_TYPE_INT32:
		case VAR_TYPE_INT16:
			p_dst->is_sign = TRUE;
			break;
		default:
			break;

		}
		p_dst->max_value = p_src->max_value;
		p_dst->min_value = p_src->min_value;
		strncpy(p_dst->tag, p_src->tag, sizeof(p_dst->tag) - 1);
		p_dst->tag[sizeof(p_dst->tag) - 1] = 0;
	}
	p_desc->parent.b_ok = TRUE;
}

static void msdcnvtcb_adj_set_items_data(void *p_data)
{
	MSDCEXT_ADJ_DATA *p_desc = MSDCNVT_CAST(MSDCEXT_ADJ_DATA, p_data);
	UINT32 *p_vars = (p_desc == NULL) ? NULL : (UINT32 *)(&p_desc[1]);
	UINT32 i, item_cnt = sizeof(m_items) / sizeof(m_items[0]);

	if (p_desc == NULL) {
		DBG_ERR("Invalid input data.\r\n");
		return;
	}

	for (i = 0; i < item_cnt; i++) {
		ADJ_ITEM *p_item = &m_items[i];
		void      *p_var =  &p_vars[i];

		switch (p_item->var_type) {
		case VAR_TYPE_BOOL:
			*((BOOL *)p_item->p_var) = *((BOOL *)p_var);
			DBG_DUMP("%s=%d,", p_item->tag, *((BOOL *)p_var));
			break;
		case VAR_TYPE_UINT32:
			*((UINT32 *)p_item->p_var) = *((UINT32 *)p_var);
			DBG_DUMP("%s=%d,", p_item->tag, *((UINT32 *)p_var));
			break;
		case VAR_TYPE_INT32:
			*((INT32 *)p_item->p_var) = *((INT32 *)p_var);
			DBG_DUMP("%s=%d,", p_item->tag, *((INT32 *)p_var));
			break;
		case VAR_TYPE_UINT16:
			*((UINT16 *)p_item->p_var) = *((UINT16 *)p_var);
			DBG_DUMP("%s=%d,", p_item->tag, *((UINT16 *)p_var));
			break;
		case VAR_TYPE_INT16:
			*((INT16 *)p_item->p_var) = *((INT16 *)p_var);
			DBG_DUMP("%s=%d,", p_item->tag, *((INT16 *)p_var));
			break;
		case VAR_TYPE_UINT8:
			*((UINT8 *)p_item->p_var) = *((UINT8 *)p_var);
			DBG_DUMP("%s=%d,", p_item->tag, *((UINT8 *)p_var));
			break;
		case VAR_TYPE_INT8:
			*((INT8 *)p_item->p_var) = *((INT8 *)p_var);
			DBG_DUMP("%s=%d,", p_item->tag, *((INT8 *)p_var));
			break;
		}
	}
	DBG_DUMP("\r\n");

	item_set_data();
	p_desc->parent.b_ok = TRUE;
}

static void msdcnvtcb_adj_get_items_data(void *p_data)
{
	MSDCEXT_ADJ_DATA *p_desc = MSDCNVT_CAST(MSDCEXT_ADJ_DATA, p_data);
	UINT32 *p_vars = (p_desc == NULL) ? NULL : (UINT32 *)(&p_desc[1]);
	UINT32 i, item_cnt = sizeof(m_items) / sizeof(m_items[0]);

	if (p_desc == NULL) {
		DBG_ERR("Invalid input data.\r\n");
		return;
	}

	item_get_data();

	for (i = 0; i < item_cnt; i++) {
		ADJ_ITEM *p_item = &m_items[i];
		void      *p_var = &p_vars[i];

		*((UINT32 *)p_var) = 0; //Reset to 0

		switch (p_item->var_type) {
		case VAR_TYPE_BOOL:
			*((BOOL *)p_var) = *((BOOL *)p_item->p_var);
			break;
		case VAR_TYPE_UINT32:
			*((UINT32 *)p_var) = *((UINT32 *)p_item->p_var);
			break;
		case VAR_TYPE_INT32:
			*((INT32 *)p_var) = *((INT32 *)p_item->p_var);
			break;
		case VAR_TYPE_UINT16:
			*((UINT16 *)p_var) = *((UINT16 *)p_item->p_var);
			break;
		case VAR_TYPE_INT16:
			*((INT16 *)p_var) = *((INT16 *)p_item->p_var);
			break;
		case VAR_TYPE_UINT8:
			*((UINT8 *)p_var) = *((UINT8 *)p_item->p_var);
			break;
		case VAR_TYPE_INT8:
			*((INT8 *)p_var) = *((INT8 *)p_item->p_var);
			break;
		}
	}

	p_desc->parent.b_ok = TRUE;
}
