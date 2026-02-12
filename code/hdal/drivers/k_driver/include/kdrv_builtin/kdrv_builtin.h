/**
 * @file kdrv_builtin.h
 * @brief type definition of KDRV API.
 */

#ifndef __KDRV_BUILTIN_H__
#define __KDRV_BUILTIN_H__

typedef enum {
	FBOOT_SRAMCTRL_CNN2 = 0, ///< 0: sram is used by CNN2
	FBOOT_SRAMCTRL_DCE  = 1, ///< 1: sram is used by DCE
	FBOOT_SRAMCTRL_CPU  = 2, ///< 2: sram is used by CPU
} FBOOT_SRAMCTRL;

/*!
 * @fn int kdrv_builtin_is_fastboot(void)
 * @brief indicate start linux with fastboot mode
 * @return return 0: start linux by uboot, 1: start linux by rtos with fastboot
 */
extern int kdrv_builtin_is_fastboot(void);
extern int kdrv_builtin_ai_is_fastboot(void);
extern int kdrv_builtin_set_sram_ctrl(FBOOT_SRAMCTRL eng);
extern FBOOT_SRAMCTRL kdrv_builtin_get_sram_ctrl(void);

#endif
