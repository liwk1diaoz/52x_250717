#ifndef _SENSOR_CTRL_INT_H_
#define _SENSOR_CTRL_INT_H_


// sensor
#include "sen_int.h"
#include "sen_dbg_infor_int.h"
#include "sen_ctrl_drv_int.h"
#include "sen_ctrl_if_int.h"
#include "sen_ctrl_cmdif_int.h"

#if 0
extern ER sen_ctrl_chk_support_type(CTL_SEN_ID id);
extern ER sen_ctrl_cfg_clk(CTL_SEN_ID id, CTL_SEN_CFG_CLK_ITEM item);
extern ER sen_ctrl_cfg_pinmux(CTL_SEN_ID id, UINT32 cur_open_id_bit);
extern ER sen_ctrl_open(CTL_SEN_ID id);
extern ER sen_ctrl_close(CTL_SEN_ID id);
extern ER sen_ctrl_start(CTL_SEN_ID id);
extern ER sen_ctrl_pause(CTL_SEN_ID id);
extern ER sen_ctrl_sleep(CTL_SEN_ID id);
extern ER sen_ctrl_wakeup(CTL_SEN_ID id);
extern ER sen_ctrl_write_reg(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
extern ER sen_ctrl_read_reg(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
extern ER sen_ctrl_chgmode(CTL_SEN_ID id, CTL_SEN_CHGMODE_OBJ chgmode_obj, CTL_SEN_MODE *chg_sen_mode);
extern ER sen_ctrl_set_cfg(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *data);
extern ER sen_ctrl_get_cfg(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *data);
extern CTL_SEN_INTE sen_ctrl_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag);
extern void sen_ctrl_dbg_info(CTL_SEN_ID id, CTL_SEN_DBG_SEL dbg_sel, UINT32 param);
#endif
#endif //_SENSOR_CTRL_INT_H_
