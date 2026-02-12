#ifndef __MODULE_DBG_H_
#define __MODULE_DBG_H_

#define NUE_EMU_DBG_EN DISABLE

#if (NUE_EMU_DBG_EN == DISABLE)
#define DBG_EMU(fmt, args...)
#else
#define DBG_EMU(fmt, args...) DBG_ERR(fmt, ##args)
#endif

#define DBG_EMU_ERR(fmt, args...) DBG_ERR(fmt, ##args)
#define DBG_EMU_IND(fmt, args...) DBG_IND(fmt, ##args)
#define DBG_EMU_WRN(fmt, args...) DBG_WRN(fmt, ##args)

#endif
