
#include <common.h>
#include <command.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <part.h>
#include <asm/hardware.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/nvt_common.h>
#include <asm/nvt-common/shm_info.h>
#include <stdlib.h>
#include <linux/arm-smccc.h>
#include "nvt_gpio.h"
#include "nvt_display_common.h"
#include <asm/arch/display.h>
#include <asm/arch/top.h>
#include <linux/libfdt.h>
#include <asm/arch/gpio.h>
#include <asm/arch/IOAddress.h>


/**
    Set module clock rate

    Set module clock rate, one module at a time.

    @param[in] id      Module ID(PLL_CLKSEL_*), one module at a time.
                          Please refer to pll.h
    @param[in] value    Moudle clock rate(PLL_CLKSEL_*_*), please refer to pll.h

    @return void
*/
void pll_set_ideclock_rate(int id, u32 value)
{
	REGVALUE reg_data;
	UINT32 ui_reg_offset;

	if (id == PLL_CLKSEL_IDE_CLKSRC_480) {
		return;
	} else if (id == PLL_CLKSEL_IDE_CLKSRC_PLL6) {
		ui_reg_offset = PLL6_OFFSET;
#if CONFIG_TARGET_NA51055
	} else if (id == PLL_CLKSEL_IDE_CLKSRC_PLL4) {
		ui_reg_offset = PLL4_OFFSET;
#endif
	} else if (id == PLL_CLKSEL_IDE_CLKSRC_PLL9) {
		ui_reg_offset = PLL9_OFFSET;
	} else {
		printf("no support soruce 0x%x.\r\n", id);
	}

	reg_data = ((value / 12000000) * 131072);

	OUTREG32(ui_reg_offset, reg_data & 0xFF);
	OUTREG32(ui_reg_offset + 0x4, (reg_data >> 8) & 0xFF);
	OUTREG32(ui_reg_offset + 0x8, (reg_data >> 16) & 0xFF);
}

int nvt_getfdt_logo_addr_size(ulong addr, ulong *fdt_addr, ulong *fdt_len)
{
	int len;
	int nodeoffset; /* next node offset from libfdt */
	const u32 *nodep; /* property node pointer */

	*fdt_addr = 0;
	*fdt_len = 0;

	nodeoffset = fdt_path_offset((const void *)addr, "/logo");
	if (nodeoffset < 0) {
		return -1;
	} else {
	    nodep = fdt_getprop((const void *)addr, nodeoffset, "enable", &len);
        if((nodep>0)&&(be32_to_cpu(nodep[0]) == 1)){
            nodep = fdt_getprop((const void *)addr, nodeoffset, "lcd_type", &len);
            if(nodep<=0){
                printf("no lcd_type\n");
                return -5;
            }
        } else {
            printf("no ebable\n");
            return -2;
        }
	}

	nodeoffset = fdt_path_offset((const void *)addr, "/nvt_memory_cfg/logo-fb");
	if (nodeoffset < 0) {
        printf("no logo-fb\n");
		return -3;
	}

	nodep = fdt_getprop((const void *)addr, nodeoffset, "reg", &len);
	if (len == 0) {
        printf("no reg\n");
		return -4;
	}

	*fdt_addr = be32_to_cpu(nodep[0]);
	*fdt_len = be32_to_cpu(nodep[1]);
	return 0;
}

//for dsi lcd
void nvt_display_dsi_reset(void)
{
	REGVALUE reg_data;

	reg_data = INREG32(DSI_RSTN_OFFSET);
	reg_data &= 0xFFFFFFF7;
	OUTREG32(DSI_RSTN_OFFSET, reg_data);
	udelay(1000);
	reg_data |= 0x8;
	OUTREG32(DSI_RSTN_OFFSET, reg_data);

}
