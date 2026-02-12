
#ifndef _TOUCH_INC_H_
#define _TOUCH_INC_H_

#include "kwrap/ioctl.h"
#include "kwrap/type.h"

#if defined __FREERTOS
#define TOUCH_OPEN(...) 0
#define TOUCH_IOCTL touch_ioctl
#define TOUCH_CLOSE(...)
#endif

#if defined __LINUX
#define TOUCH_OPEN  open
#define TOUCH_IOCTL ioctl
#define TOUCH_CLOSE close
#endif

#define IOC_TOUCH_PARKING_MODE            	_VOS_IOW('w', 8, unsigned int)
#define IOC_TOUCH_PARKING_SENSITIVITY         _VOS_IOW('w', 9, unsigned int)


#define IOC_TOUCH_ISPENDOWN			_IOWR('r', 1, unsigned int)
#define IOC_TOUCH_GET_XY_VALUE       	_IOWR('r', 2, unsigned int)


#if defined(__FREERTOS)
int touch_ioctl (int fd, unsigned int cmd, unsigned long arg);
#endif

/**
    @addtogroup mIDxInput
*/
//@{
#if 0
/**
    DX status key group.
*/
typedef enum _DX_STATUS_KEY_GROUP {
	DX_STATUS_KEY_GROUP1 = 0,
	DX_STATUS_KEY_GROUP2,
	DX_STATUS_KEY_GROUP3,
	DX_STATUS_KEY_GROUP4,
	DX_STATUS_KEY_GROUP5,
	DX_STATUS_KEY_GROUP_MAX,
	ENUM_DUMMY4WORD(DX_STATUS_KEY_GROUP)
} DX_STATUS_KEY_GROUP;

///////////////////////////////////////////////////////////////////////////////
// Key
///////////////////////////////////////////////////////////////////////////////
/**
    DrvKey initialization.

    Hardware initialization.

    @note In most case, this API is not used, since the GPIO init is implemented in Dx_InitIO().
*/
extern void DrvKey_Init(void);


/**
    DrvKey detect normal keys.

    Detect physical HW and return the flag of pressed keys. One bit represents a specified key.
*/
extern UINT32 DrvKey_DetNormalKey(void);

/**
    DrvKey detect power key.

    Detect power status and return the specified power flag.

    @note The power key bitwise could NOT be the same with any normal key, \n
          so there are at most 32(UINT32) keys for power and normal key.
*/
extern UINT32 DrvKey_DetPowerKey(void);

/**
    DrvKey detect status keys.

    Detect physical HW and return the key state of the specified KeyGroup.

    @note Each status key could have at most 32 states and there are at most five status keys.
    @param[in] KeyGroup DX status key group: #_DX_STATUS_KEY_GROUP.
*/
extern UINT32 DrvKey_DetStatusKey(DX_STATUS_KEY_GROUP KeyGroup);
#endif
///////////////////////////////////////////////////////////////////////////////
// Touch
///////////////////////////////////////////////////////////////////////////////
/**
    DetTP initialization.

    Hardware initialization.
*/
extern int DetTP_Init(void);

/**
    DetTP get point.

    This API should return the coordinate of current touched point.

    @param[out] pX The coordinate x.
    @param[out] pY The coordinate y.
*/
extern void DetTP_GetXY(INT32 *pX, INT32 *pY);

/**
    DetTP check if the panel is being touched.

    @return
         - @b TRUE: The Panel is being touched.
         - @b FALSE: The Panel is released.
*/
extern BOOL DetTP_IsPenDown(void);

#endif  //_GSEN_INC_H_
