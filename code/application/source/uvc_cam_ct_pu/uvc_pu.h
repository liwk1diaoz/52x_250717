
#include "hd_type.h"


// Define Bits Containing Capabilities of the control for the Get_Info request (section 4.1.2 in the UVC spec 1.1)
#define SUPPORT_GET_REQUEST                                                     0x01
#define SUPPORT_SET_REQUEST                                                     0x02
#define DISABLED_DUE_TO_AUTOMATIC_MODE                          0x04
#define AUTOUPDATE_CONTROL                                                      0x08
#define ASNCHRONOUS_CONTROL                                                     0x10
#define RESERVED_BIT5                                                           0x20
#define RESERVED_BIT6                                                           0x40
#define RESERVED_BIT7                                                           0x80

//Processing Unit Control Selectors
#define PU_CONTROL_UNDEFINED                                            0x00
#define PU_BACKLIGHT_COMPENSATION_CONTROL                       0x01
#define PU_BRIGHTNESS_CONTROL                                           0x02
#define PU_CONTRAST_CONTROL                                                     0x03
#define PU_GAIN_CONTROL                                                         0x04
#define PU_POWER_LINE_FREQUENCY_CONTROL                         0x05
#define PU_HUE_CONTROL                                                          0x06
#define PU_SATURATION_CONTROL                                           0x07
#define PU_SHARPNESS_CONTROL                                            0x08
#define PU_GAMMA_CONTROL                                                        0x09
#define PU_WHITE_BALANCE_TEMPERATURE_CONTROL            0x0A
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL       0x0B
#define PU_WHITE_BALANCE_COMPONENT_CONTROL                      0x0C
#define PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL         0x0D
#define PU_DIGITAL_MULTIPLIER_CONTROL                           0x0E
#define PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL                     0x0F
#define PU_HUE_AUTO_CONTROL                                                     0x10
#define PU_ANALOG_VIDEO_STANDARD_CONTROL                        0x11
#define PU_ANALOG_LOCK_STATUS_CONTROL                           0x12

extern BOOL xUvacPU_CB(UINT32 CS, UINT8 request, UINT8 *pData, UINT32 *pDataLen);

