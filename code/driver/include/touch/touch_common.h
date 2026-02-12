#ifndef _MTR_COMMON_H_
#define _MTR_COMMON_H_

#ifdef __KERNEL__
#include <linux/spi/spi.h>
#endif
#if defined(__FREERTOS)
#include <libfdt.h>
#endif

#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include "kwrap/util.h"
#include "kwrap/perf.h"
//#include "isp_mtr.h"
#include "touch_dbg.h"


//=============================================================================
// compile switch
//=============================================================================
#define TOUCH_DTSI_FROM_FILE    0
#define TOUCH_SHOW_DBG_MSG      0


//=============================================================================
// macro definition
//=============================================================================
#define TOUCH_ABS(X)          (((X) < 0) ? -(X) : (X))
#define TOUCH_MAX(A,B)        (((A) >= (B)) ? (A) : (B))
#define TOUCH_MIN(A,B)        (((A) <= (B)) ? (A) : (B))
#define TOUCH_CLAMP(X,L,H)    (((X) >= (H)) ? (H) : (((X) <= (L)) ? (L) : (X)))
#define TOUCH_ARRAY_SIZE(X)   (sizeof((X)) / sizeof((X[0])))


//=============================================================================
// constant define
//=============================================================================


//=============================================================================
// struct & definition
//=============================================================================


typedef struct {
	UINT32 gpio_pwr;
	UINT32 gpio_rst;
	UINT32 gpio_int;
} TOUCH_GPIO_CONFIG;

typedef struct {
	UINT32 i2c_id;
	UINT32 i2c_addr;
} TOUCH_I2C_CONFIG;

typedef struct {
	UINT32 rst_time;
} TOUCH_RST_TIME_CONFIG;

typedef struct {

	TOUCH_I2C_CONFIG i2c_cfg;

	TOUCH_GPIO_CONFIG gpio_cfg;    // for power , reset, interrupt

	TOUCH_RST_TIME_CONFIG reset_time;
	//UINT32 pin_def[HW_GPIO_MAX];
} TOUCH_HW_PIN;

typedef struct {
	TOUCH_HW_PIN tp_hw;

} TOUCH_DRV_INFO;



extern ER touch_common_load_dtsi_file(UINT8 *node_path, UINT8 *file_path, UINT8 *buf_addr, void *param);

#endif

