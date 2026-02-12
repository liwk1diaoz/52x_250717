#ifndef _NVT_GPIO_H_
#define _NVT_GPIO_H_

#include <asm/arch/IOAddress.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/rcw_macro.h>

//0x00 GPIO Storage Data Register
#define GPIO_STRG_DAT_REG_OFS              0x00

//0x04 GPIO Peripheral Data Register
#define GPIO_PERI_DAT_REG_OFS              0x04

//0x08 GPIO Sensor Data Register
#define GPIO_SEN_DAT_REG_OFS                0x08

//0x0C GPIO LCD Data Register
#define GPIO_LCD_DAT_REG_OFS                0x0C
//0x10 GPIO DGPIO Data Register
#define GPIO_DGPIO_DAT_REG_OFS              0x10

//0x14 GPIO HGPI Data Register
#define GPIO_HGPIO_DAT_REG_OFS              0x14

//0x18 GPIO ADC Data Register
#define GPIO_AGPIO_DAT_REG_OFS              0x18

//0x1C Reserved Register

//0x20 GPIO Storage Direction Register
#define GPIO_STRG_DIR_REG_OFS              0x20

//0x24 GPIO Peripheral Direction Register
#define GPIO_PERI_DIR_REG_OFS              0x24

//0x28 GPIO Sensor Direction Register
#define GPIO_SEN_DIR_REG_OFS                0x28

//0x2C GPIO LCD Direction Register
#define GPIO_LCD_DIR_REG_OFS                0x2C

//0x30 GPIO DGPIO Direction Register
#define GPIO_DGPIO_DIR_REG_OFS              0x30

//0x34 GPIO HGPI Direction Register
#define GPIO_HGPIO_DIR_REG_OFS              0x34

//0x38 GPIO ADC Direction Register
#define GPIO_AGPIO_DIR_REG_OFS              0x38

//0x40 GPIO Storage SET Register
#define GPIO_STRG_SET_REG_OFS              0x40

//0x44 GPIO Peripheral SET Register
#define GPIO_PERI_SET_REG_OFS              0x44

//0x48 GPIO Sensor SET Register
#define GPIO_SEN_SET_REG_OFS                0x48

//0x4C GPIO LCD SET Register
#define GPIO_LCD_SET_REG_OFS                0x4C

//0x50 GPIO DGPIO SET Register
#define GPIO_DGPIO_SET_REG_OFS              0x50

//0x54 GPIO HGPI SET Register
#define GPIO_HGPIO_SET_REG_OFS              0x54

//0x58 GPIO ADC SET Register
#define GPIO_AGPIO_SET_REG_OFS              0x58

//0x60 GPIO Storage CLEAR Register
#define GPIO_STRG_CLR_REG_OFS              0x60

//0x64 GPIO Peripheral CLEAR Register
#define GPIO_PERI_CLR_REG_OFS              0x64

//0x68 GPIO Sensor CLEAR Register
#define GPIO_SEN_CLR_REG_OFS                0x68

//0x6C GPIO LCD CLEAR Register
#define GPIO_LCD_CLR_REG_OFS                0x6C

//0x70 GPIO DGPIO CLEAR Registe
#define GPIO_DGPIO_CLR_REG_OFS              0x70

//0x74 GPIO HGPI CLEAR Register
#define GPIO_HGPIO_CLR_REG_OFS              0x74

//0x78 GPIO ADC CLEAR Register
#define GPIO_AGPIO_CLR_REG_OFS              0x78

//0x80 GPIO Interrupt Status Register
#define GPIO_INT_STS_REG_OFS                0x80

//0x90 GPIO Interrupt Enable Register
#define GPIO_INT_EN_REG_OFS                 0x90

//0xA0 GPIO Interrupt Type Register
#define GPIO_INT_TYPE_REG_OFS               0xA0

//0xA4 GPIO Interrupt Polarity Register
#define GPIO_INT_POL_REG_OFS                0xA4

//0xA8 GPIO Interrupt Edge Type Register
#define GPIO_INT_EDGE_TYPE_REG_OFS           0xA8

//0xC0 DGPIO Interrupt Status Register
#define GPIO_DINT_STS_REG_OFS                0xC0

#define GPIO_DINT_EN_REG_OFS                 0xD0

//0xE0 DGPIO Interrupt Type Register
#define GPIO_DINT_TYPE_REG_OFS                0xE0

//0xE4 DGPIO Interrupt Polarity Register
#define GPIO_DINT_POL_REG_OFS                0xE4

//0xE8 DGPIO Interrupt Edge Type Register
#define GPIO_DINT_EDGE_TYPE_REG_OFS          0xE8

#define GPIO_GETREG(ofs)	            INW(IOADDR_GPIO_REG_BASE+(ofs))
#define GPIO_SETREG(ofs, value)     	OUTW(IOADDR_GPIO_REG_BASE +(ofs),(value))


/**
    GPIO direction

    GPIO direction definition for gpio_setDir() and gpio_getDir()
*/
typedef enum {
	GPIO_DIR_INPUT =            0,      ///< GPIO is input direction
	GPIO_DIR_OUTPUT =           1,      ///< GPIO is output direction
	ENUM_DUMMY4WORD(GPIO_DIR)
} GPIO_DIR;


extern void nvt_gpio_setDir(UINT32 pin, GPIO_DIR dir);
extern GPIO_DIR nvt_gpio_getDir(UINT32 pin);
extern void nvt_gpio_setPin(UINT32 pin);
extern void nvt_gpio_clearPin(UINT32 pin);
extern UINT32 nvt_gpio_getPin(UINT32 pin);
#endif
