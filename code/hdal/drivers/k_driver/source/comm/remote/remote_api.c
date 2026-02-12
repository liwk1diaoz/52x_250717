#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <kwrap/file.h>
#include "remote_api.h"
#include "remote_drv.h"
#include "remote_dbg.h"

static int nvt_remote_api_setConfig(REMOTE_CONFIG_ID id, UINT32 value)
{
	REMOTE_CONFIG_INFO config_info;

	config_info.id = id;
	config_info.value = value;
	return nvt_remote_drv_set_config(&config_info);
}

static int __nvt_remote_api_read_reg(UINT32 addr)
{
	UINT32  va, ret;
	BOOL    is_remap = FALSE;

	addr &= 0xFFFFFFFC;
	if ((addr & 0xF0000000) == 0xF0000000) {
		va = (UINT32)ioremap_cache(addr, 4);
		if (va == 0 || (void *)va == NULL) {
			nvt_dbg(ERR, "ioremap() failed, addr 0x%x\r\n", addr);
			return -1;
		}
		is_remap = TRUE;
	} else {
		va = addr;
	}

	ret = *(UINT32 *)va;

	if (TRUE == is_remap) {
		iounmap((void *)va);
	}

	return ret;
}

static void __nvt_remote_api_write_reg(UINT32 value, UINT32 addr)
{
	UINT32  va;
	BOOL    is_remap = FALSE;

	addr &= 0xFFFFFFFC;
	if ((addr & 0xF0000000) == 0xF0000000) {
		va = (UINT32)ioremap_cache(addr, 4);
		if (va == 0 || (void *)va == NULL) {
			nvt_dbg(ERR, "ioremap() failed, addr 0x%x\r\n", addr);
			return;
		}
		is_remap = TRUE;
	} else {
		va = addr;
	}

	*(UINT32 *)va = value;

	if (TRUE == is_remap) {
		iounmap((void *)va);
	}
}

int nvt_remote_api_write_reg(PREMOTE_MODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
	unsigned long reg_addr = 0, reg_value = 0;

	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtoul (pargv[1], 0, &reg_value)) {
		nvt_dbg(ERR, "invalid rag value:%s\n", pargv[1]);
 		return -EINVAL;

	}

	nvt_dbg(IND, "W REG 0x%lx to 0x%lx\n", reg_value, reg_addr);

	nvt_remote_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}

int nvt_remote_api_write_receiver_test(PREMOTE_MODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
#if defined(_BSP_NA51055_)
#define TOP_REGREMOTE_ADDR 0xF0010018
#define TOP_REGLGPIO0_ADDR 0xF00100B8
#define TOP_REGPGPIO0_ADDR 0xF00100A8
#define TOP_REGDEBUG_ADDR 0xF00100FC
#define TOP_UART2_ADDR 0xF0010024
#define PINMUX_DEBUGPORT_GROUP2 0x0200
#define PINMUX_DEBUGPORT_REMOTE 0x0024
	UINT32 TOP_REGREMOTE;
	UINT32 TOP_REGLGPIO0;
	UINT32 TOP_REGPGPIO0;
	UINT32 TOP_REGUART2;
	UINT32 MatL = 0xE11E6B86, MatH = 0x00;

	// top remote enable
	TOP_REGREMOTE = __nvt_remote_api_read_reg(TOP_REGREMOTE_ADDR);
	TOP_REGREMOTE |= (0x1 << 7);
	__nvt_remote_api_write_reg(TOP_REGREMOTE, TOP_REGREMOTE_ADDR);

	// top debugport setting
	TOP_REGLGPIO0 = __nvt_remote_api_read_reg(TOP_REGLGPIO0_ADDR);
	TOP_REGLGPIO0 &= ~(0x00007FF); /*LCD0~LCD10*/
	__nvt_remote_api_write_reg(TOP_REGLGPIO0, TOP_REGLGPIO0_ADDR);

	TOP_REGPGPIO0 = __nvt_remote_api_read_reg(TOP_REGPGPIO0_ADDR);
	TOP_REGPGPIO0 &= ~(0x66180C); /*P_GPIO2,3,11,12 , P_GPIO17~P_GPIO18, P_GPIO21~P_GPIO22*/
	__nvt_remote_api_write_reg(TOP_REGPGPIO0, TOP_REGPGPIO0_ADDR);

	__nvt_remote_api_write_reg(PINMUX_DEBUGPORT_GROUP2 | PINMUX_DEBUGPORT_REMOTE, TOP_REGDEBUG_ADDR);

	// top uart2 disable
	TOP_REGUART2 = __nvt_remote_api_read_reg(TOP_UART2_ADDR);
	TOP_REGUART2 &= ~(0x3 << 2);
	__nvt_remote_api_write_reg(TOP_REGUART2, TOP_UART2_ADDR);

#elif defined(_BSP_NA51000_)
#define TOP_REGREMOTE_ADDR 0xF0010018
#define TOP_REGLGPIO0_ADDR 0xF00100B8
#define TOP_REGPGPIO0_ADDR 0xF00100A8
#define TOP_REGDEBUG_ADDR 0xF00100FC
#define TOP_PI_CNT3_ADDR 0xF001001C
#define PINMUX_DEBUGPORT_GROUP2 0x0200
#define PINMUX_DEBUGPORT_REMOTE 0x0028
	UINT32 TOP_REGREMOTE;
	UINT32 TOP_REGLGPIO0;
	UINT32 TOP_REGPGPIO0;
	UINT32 TOP_REGPI_CNT3;
	UINT32 MatL = 0xE11E6B86, MatH = 0x00;

	// top remote enable
	TOP_REGREMOTE = __nvt_remote_api_read_reg(TOP_REGREMOTE_ADDR);
	TOP_REGREMOTE |= (0x1 << 12);
	__nvt_remote_api_write_reg(TOP_REGREMOTE, TOP_REGREMOTE_ADDR);

	// top P_GPIO setting
	TOP_REGPGPIO0 = __nvt_remote_api_read_reg(TOP_REGPGPIO0_ADDR);
	TOP_REGPGPIO0 &= ~(0x1 << 20); /*P_GPIO20*/
	__nvt_remote_api_write_reg(TOP_REGPGPIO0, TOP_REGPGPIO0_ADDR);

	// top debugport setting
	TOP_REGLGPIO0 = __nvt_remote_api_read_reg(TOP_REGLGPIO0_ADDR);
	TOP_REGLGPIO0 &= ~(0x007FFFF); /*LCD0~LCD18*/
	__nvt_remote_api_write_reg(TOP_REGLGPIO0, TOP_REGLGPIO0_ADDR);

	__nvt_remote_api_write_reg(PINMUX_DEBUGPORT_GROUP2 | PINMUX_DEBUGPORT_REMOTE, TOP_REGDEBUG_ADDR);

	// top pi_cnt3 disable
	TOP_REGPI_CNT3 = __nvt_remote_api_read_reg(TOP_PI_CNT3_ADDR);
	TOP_REGPI_CNT3 &= ~(0x3 << 28);
	__nvt_remote_api_write_reg(TOP_REGPI_CNT3, TOP_PI_CNT3_ADDR);

#endif

	// remote setting
	nvt_remote_drv_set_en(FALSE);

	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_DATA_LENGTH,      32);
	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_INPUT_INVERSE,    TRUE);
	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_THRESHOLD_SEL,    REMOTE_SPACE_DET);
	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_DATA_ORDER,       REMOTE_DATA_LSB);

	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_HEADER_DETECT,    REMOTE_FORCE_DETECT_HEADER);
	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_HEADER_TH,        6000);
	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_LOGIC_TH,         1120);
	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_ERROR_TH,         20000000);
	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_GSR_TH,           400);

	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_REPEAT_DET_EN,    ENABLE);
	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_REPEAT_TH,        3375);

	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_MATCH_LOW,        (UINT32)MatL);
	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_MATCH_HIGH,       (UINT32)MatH);

	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_WAKEUP_DISABLE,   REMOTE_INT_ALL);
	nvt_remote_api_setConfig(REMOTE_CONFIG_ID_WAKEUP_ENABLE,    REMOTE_INT_RD | REMOTE_INT_MATCH);

	nvt_remote_drv_set_interrupt_en(REMOTE_INT_ALL);

	nvt_remote_drv_set_en(TRUE);

	return 0;
}

int nvt_remote_api_write_chg_clk_src(PREMOTE_MODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
#if defined(_BSP_NA51055_)
	unsigned long clk_src = 0xffffffff;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (pargv[0], 0, &clk_src)) {
		nvt_dbg(ERR, "invalid clock source:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_remote_drv_set_en(FALSE);

	switch (clk_src) {
	case 0:
		nvt_remote_api_setConfig(REMOTE_CONFIG_ID_CLK_SRC_SEL,  REMOTE_CLK_SRC_RTC);
		break;
	case 1:
		nvt_remote_api_setConfig(REMOTE_CONFIG_ID_CLK_SRC_SEL,  REMOTE_CLK_SRC_OSC);
		break;
	default:
		nvt_dbg(ERR, "clock source (%lu) must be 0 or 1\r\n", clk_src);
	}

	nvt_remote_drv_set_en(TRUE);

#elif defined(_BSP_NA51000_)
	nvt_dbg(ERR, "680 don't support change remote clock\n");

#endif

	return 0;
}

int nvt_remote_api_write_pattern(PREMOTE_MODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
	mm_segment_t old_fs;
	int fp;
	int len = 0;
	unsigned char* pbuffer;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	fp = vos_file_open(pargv[0], O_RDONLY , 0);
	if (-1 == fp) {
	    nvt_dbg(ERR, "failed in file open:%s\n", pargv[0]);
		return -EFAULT;
	}

	pbuffer = kmalloc(256, GFP_KERNEL);
	if (pbuffer == NULL) {
		vos_file_close(fp);
		return -ENOMEM;
	}

	old_fs = get_fs();
	set_fs(get_ds());

	len = vos_file_read(fp, pbuffer, 256);

	/* Do something after get data from file */

	kfree(pbuffer);
	vos_file_close(fp);
	set_fs(old_fs);

	return len;
}

int nvt_remote_api_read_reg(PREMOTE_MODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
	unsigned long reg_addr = 0;
	unsigned long value;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "R REG 0x%lx\n", reg_addr);
	value = nvt_remote_drv_read_reg(pmodule_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%lx = 0x%lx\n", reg_addr, value);
	return 0;
}