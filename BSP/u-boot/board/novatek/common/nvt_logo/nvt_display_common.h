#ifndef _NVT_DISPLAY_COMMON_H_
#define _NVT_DISPLAY_COMMON_H_

#include <asm/arch/IOAddress.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/rcw_macro.h>

#define HEAVY_LOAD_CTRL_OFS(ch)         (DMA_CHANNEL0_HEAVY_LOAD_CTRL_OFS + ((ch) * 0x10))
#define HEAVY_LOAD_ADDR_OFS(ch)         (DMA_CHANNEL0_HEAVY_LOAD_START_ADDR_OFS + ((ch) * 0x10))
#define HEAVY_LOAD_SIZE_OFS(ch)         (DMA_CHANNEL0_HEAVY_LOAD_DMA_SIZE_OFS + ((ch) * 0x10))
#define HEAVY_LOAD_WAIT_CYCLE_OFS(ch)   (DMA_CHANNEL0_HEAVY_LOAD_WAIT_CYCLE_OFS + ((ch) * 0x10))

#define PROTECT_START_ADDR_OFS(ch)      (DMA_PROTECT_STARTADDR0_REG0_OFS+(ch)*8)
#define PROTECT_END_ADDR_OFS(ch)        (DMA_PROTECT_STOPADDR0_REG0_OFS+(ch)*8)
#define PROTECT_CH_MSK0_OFS(ch)         (DMA_PROTECT_RANGE0_MSK0_REG_OFS+(ch)*32)
#define PROTECT_CH_MSK1_OFS(ch)         (DMA_PROTECT_RANGE0_MSK1_REG_OFS+(ch)*32)
#define PROTECT_CH_MSK2_OFS(ch)         (DMA_PROTECT_RANGE0_MSK2_REG_OFS+(ch)*32)
#define PROTECT_CH_MSK3_OFS(ch)         (DMA_PROTECT_RANGE0_MSK3_REG_OFS+(ch)*32)
#define PROTECT_CH_MSK4_OFS(ch)         (DMA_PROTECT_RANGE0_MSK4_REG_OFS+(ch)*32)
#define PROTECT_CH_MSK5_OFS(ch)         (DMA_PROTECT_RANGE0_MSK5_REG_OFS+(ch)*32)

#define INREG32(x)          			(*((volatile UINT32*)(x)))
#define OUTREG32(x, y)      			(*((volatile UINT32*)(x)) = (y))    ///< Write 32bits IO register
#define SETREG32(x, y)      			OUTREG32((x), INREG32(x) | (y))     ///< Set 32bits IO register
#define CLRREG32(x, y)      			OUTREG32((x), INREG32(x) & ~(y))    ///< Clear 32bits IO register

#define LOGO_DBG_MSG 0
#if LOGO_DBG_MSG
#define _Y_LOG(fmt, args...)         	printf(DBG_COLOR_YELLOW fmt DBG_COLOR_END, ##args)
#define _R_LOG(fmt, args...)         	printf(DBG_COLOR_RED fmt DBG_COLOR_END, ##args)
#define _M_LOG(fmt, args...)         	printf(DBG_COLOR_MAGENTA fmt DBG_COLOR_END, ##args)
#define _G_LOG(fmt, args...)         	printf(DBG_COLOR_GREEN fmt DBG_COLOR_END, ##args)
#define _W_LOG(fmt, args...)         	printf(DBG_COLOR_WHITE fmt DBG_COLOR_END, ##args)
#define _X_LOG(fmt, args...)         	printf(DBG_COLOR_HI_GRAY fmt DBG_COLOR_END, ##args)
#else
#define _Y_LOG(fmt, args...)
#define _W_LOG(fmt, args...)
#endif

#if CONFIG_TARGET_NA51055
#define PLL_CLKSEL_IDE_CLKSRC_480   (0x00)    //< Select IDE clock source as 480 MHz
#define PLL_CLKSEL_IDE_CLKSRC_PLL6  (0x01)    //< Select IDE clock source as PLL6 (for IDE/ETH)
#define PLL_CLKSEL_IDE_CLKSRC_PLL4  (0x02)    //< Select IDE clock source as PLL4 (for SSPLL)
#define PLL_CLKSEL_IDE_CLKSRC_PLL9  (0x03)    //< Select IDE clock source as PLL9 (for IDE/ETH backup)

#define PLL4_OFFSET 0xF0021318
#define PLL6_OFFSET 0xF0021288
#define PLL9_OFFSET 0xF002134c


#elif CONFIG_TARGET_NA51089
#define PLL_CLKSEL_IDE_CLKSRC_480   (0x00)    //< Select IDE clock source as 480 MHz
#define PLL_CLKSEL_IDE_CLKSRC_PLL6  (0x01)    //< Select IDE clock source as PLL6 (for IDE/ETH)
#define PLL_CLKSEL_IDE_CLKSRC_PLL9  (0x03)    //< Select IDE clock source as PLL9 (for IDE/ETH backup)

#define PLL6_OFFSET 0xF00244E0
#define PLL9_OFFSET 0xF0024520

#endif

#define DSI_RSTN_OFFSET 0xF0020088

extern uint8_t *nvt_fdt_buffer;
extern unsigned char inbuf[];
extern unsigned char percent0_str[]; //jpg format
extern unsigned char percent100_str[]; //jpg format
extern unsigned char updatefw_str[];  //jpg format
extern unsigned char updatefwok_str[];  //jpg format
extern unsigned char updatefwfailed_str[];  //jpg format
extern unsigned char updatefwfailed_updatefwagain_str[];  //jpg format
extern unsigned char percent0_rotate90_str[]; //jpg format
extern unsigned char percent100_rotate90_str[]; //jpg format
extern unsigned char updatefw_rotate90_str[];  //jpg format
extern unsigned char updatefwok_rotate90_str[];  //jpg format
extern unsigned char updatefwfailed_rotate90_str[];  //jpg format
extern unsigned char updatefwfailed_updatefwagain_rotate90_str[];  //jpg format
extern unsigned char percent0_rotate270_str[]; //jpg format
extern unsigned char percent100_rotate270_str[]; //jpg format
extern unsigned char updatefw_rotate270_str[];  //jpg format
extern unsigned char updatefwok_rotate270_str[];  //jpg format
extern unsigned char updatefwfailed_rotate270_str[];  //jpg format
extern unsigned char updatefwfailed_updatefwagain_rotate270_str[];  //jpg format

extern void jpeg_setfmt(unsigned int fmt);
extern void jpeg_decode(unsigned char* inbuf, unsigned char* outbuf);
extern void jpeg_getdim(unsigned int* width, unsigned int* height);
extern void pll_set_ideclock_rate(int id, u32 value);
extern int nvt_getfdt_logo_addr_size(ulong addr, ulong *fdt_addr, ulong *fdt_len);
extern void nvt_display_dsi_reset(void);
#endif
