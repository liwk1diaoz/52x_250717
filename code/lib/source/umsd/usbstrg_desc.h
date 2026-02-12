/*
    USB MSDC Descriptors Header

    Mass-Storage-Device-Class Task USB Descriptors Header File.

    @file       UsbStrgDesc.h
    @ingroup    mILibUsbMSDC
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _USBSTRGDESC_H
#define _USBSTRGDESC_H

#include "kwrap/type.h"
#include "usb2dev.h"

#define USB_FS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE        64
#define USB_FS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE        64

#define USB_HS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE        0x0200
#define USB_HS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE        0x0200

#define USB_SS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE        0x0400
#define USB_SS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE        0x0400

/* number of string descriptor */
#define USB_STRG_STR_NUM_STRING             4
#define USB_STRG_IF_CFGA_IF0_NUMBER_EP      2
#define USB_STRG_IF_IF0_NUMBER              0
#define USB_STRG_CFG_A_IF0_ALT_MAX          1
#define USB_STRG_CFGA_NUMBER_IF             1

/* device descriptor */
#define USB_SSSTRG_DEV_RELEASE_NUMBER       0x0310         /* spec. release number             */
#define USB_HSSTRG_DEV_RELEASE_NUMBER       0x0210         /* spec. release number             */
#define USB_FSSTRG_DEV_RELEASE_NUMBER       0x0110         /* spec. release number             */
#define USB2_HSSTRG_DEV_RELEASE_NUMBER      0x0200         /* spec. release number             */
#define USB_STRG_DEV_CLASS                  0              /* class code                       */
#define USB_STRG_DEV_SUBCLASS               0              /* sub class code                   */
#define USB_STRG_DEV_PROTOCOL               0              /* protocol code                    */
#define USB_STRG_DEV_MAX_PACKET_SIZE0       0x40           /* max packet size for endpoint 0   */
#define USB_STRG_VENDOR_ID                  0x0D64         /* vendor id                        */
#define USB_STRG_PRODUCT_ID                 0x6680         /* product id                       */
#define USB_STRG_DEV_RELASE_NUMBER          0x0100         /* device release number            */
#define USB_STRG_DEV_NUM_CONFIG             1              /* number of possible configuration */

/*
    configuration descriptor
*/
#define USB_SS_STRG_CFGA_TOTAL_LENGTH       (USB_CFG_LENGTH + (USB_IF_LENGTH * 1) + ((USB_EP_LENGTH+USB_SSEP_COMPANISON_LENGTH) * 2))
#define USB_STRG_CFGA_TOTAL_LENGTH          (USB_CFG_LENGTH + (USB_IF_LENGTH * 1) + (USB_EP_LENGTH * 2))
/* total length                     */
#define USB_STRG_CFGA_NUMBER_IF             1                   /* number of interfaces             */
#define USB_STRG_CFGA_CFG_VALUE             1                   /* configuration value              */
#define USB_STRG_CFGA_CFG_IDX               0                   /* configuration string id          */

#define USB_STRG_CFGA_CFG_ATTRIBUES_IDX     0x07                /* Attribute index in configuration */
#define USB_STRG_CFGA_CFG_ATTRIBUES_SELF    0xC0                /* characteristics: Self Powered    */
#define USB_STRG_CFGA_CFG_ATTRIBUES_BUS     0x80                /* characteristics: Bus Powered     */

#define USB_STRG_CFGA_MAX_POWER_IDX         0x08                /* maximum power index in configuration*/
#define USB_SS_STRG_CFGA_MAX_POWER_SELF     0x0D                /* 104mA maximum power. Unit in 8mA */
#define USB_STRG_CFGA_MAX_POWER_SELF        0x32                /* 100mA maximum power. Unit in 2mA */
#define USB_STRG_CFGA_MAX_POWER_BUS         0xFA                /* 500mA maximum power. Unit in 2mA */


/*
    Interface Descriptor
*/

/* interface number */
/*---- alternate setting */
#define USB_STRG_IF_ALT0                    0
/* number of endpoint */
/* b_interface_class */
#define USB_STRG_IF_CLASS_STORAGE           8
/* b_interface_subclass */
#define USB_STRG_IF_SUBCLASS_SCSI           6
/* b_interface_protocol */
#define USB_STRG_IF_PROTOCOL_BULK_ONLY      0x50
#define USB_STRG_IF_IDX                     0

/*
    Binary device Object Store (BOS) Descriptor
*/
#define USB_STRG_BOS_LENGTH                 5
#define USB_SS_STRG_BOS_TOTAL_LENGTH        22
#define USB_HS_STRG_BOS_TOTAL_LENGTH        12
#define USB_SS_STRG_BOS_DEVCAP_NUMBER       2
#define USB_HS_STRG_BOS_DEVCAP_NUMBER       1





extern USB_DEVICE_DESC gUSBSSStrgDevDesc, gUSBHSStrgDevDesc, gUSBFSStrgDevDesc, gUSB2HSStrgDevDesc;
extern UINT8 gUSBSSStrgConfigDesc[], gUSBHSStrgConfigDesc[], gUSBFSStrgConfigDesc[];
extern UINT8 gUSBHSOtherStrgConfigDesc[], gUSBFSOtherStrgConfigDesc[];
extern UINT8 gUSBSSStrgBosDesc[],gUSBHSStrgBosDesc[];

extern const UINT8 USBStrgStrDesc0[];
extern const UINT16 USBMSDCManuStrDesc1[], USBMSDCProdStrDesc2[], USBStrgStrDesc3[];
extern const USB_DEVICE_DESC gUSBHSStrgDevQualiDesc[], gUSBFSStrgDevQualiDesc[],gUSB2HSStrgDevQualiDesc[];
#endif
