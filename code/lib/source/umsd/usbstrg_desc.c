/*
    USB MSDC Descriptors

    Mass-Storage-Device-Class Task USB Descriptors.

    @file       UsbStrgDesc.c
    @ingroup    mILibUsbMSDC
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include    "usbstrg_desc.h"

/*---- string descriptor index0 is LANGID ----*/
_ALIGNED(64) const UINT8   USBStrgStrDesc0[] = {
	4,                                  /* size of String Descriptor        */
	USB_STRING_DESCRIPTOR_TYPE,         /* String Descriptor type           */
	0x09, 0x04                          /*  Primary/Sub LANGID              */
};

/*---- USB Device Manufacturer string descriptor ----*/
_ALIGNED(64) const UINT16 USBMSDCManuStrDesc1[] = {
	0x0320,                                // 20: size of size of String Descriptor = 32 bytes    // 10: size of String Descriptor = 16 bytes
	// 03: String Descriptor type
	'N', 'O', 'V', 'A', 'T', 'E', 'K', '0', '0', '0', '0', '0', '0', '0', '0',     // NOVATEK
};

/*---- USB Device Product string descriptor ----*/
_ALIGNED(64) const UINT16 USBMSDCProdStrDesc2[] = {
	0x0336,                                // 36: size of String Descriptor = 54 bytes
	// 03: String Descriptor type
	'N', 'O', 'V', 'A', 'T', 'E',          // 96680 DSP INSIDE
	'K', ' ', '9', '6', '6', '8',
	'0', ' ', 'D', 'S', 'P', ' ',
	'I', 'N', 'S', 'I', 'D', 'E',
	' ', ' '

};

/*---- USB Device SerialNumber string descriptor ----*/
_ALIGNED(64) const UINT16 USBStrgStrDesc3[] = {
	0x0320,                                // 20: size of String Descriptor = 32 bytes
	// 03: String Descriptor type
	'9', '6', '6', '8', '0',               // 96680-00000-001
	'-', '0', '0', '0', '0',
	'0', '-', '0', '0', '1'
};

#if 1

_ALIGNED(64) USB_DEVICE_DESC gUSBSSStrgDevDesc = {
	//---- device descriptor ----
	USB_DEV_LENGTH,                 /* descriptor size                  */
	USB_DEVICE_DESCRIPTOR_TYPE,     /* descriptor type                  */
	USB_SSSTRG_DEV_RELEASE_NUMBER,  /* spec. release number             */
	USB_STRG_DEV_CLASS,             /* class code                       */
	USB_STRG_DEV_SUBCLASS,          /* sub class code                   */
	USB_STRG_DEV_PROTOCOL,          /* protocol code                    */
	9,                              /* max packet size for endpoint 0   */
	USB_STRG_VENDOR_ID,             /* vendor id                        */
	USB_STRG_PRODUCT_ID,            /* product id                       */
	USB_STRG_DEV_RELASE_NUMBER,     /* device release number            */
	1,                              /* manifacturer string id           */
	2,                              /* product string id                */
	3,                              /* serial number string id          */
	USB_STRG_DEV_NUM_CONFIG         /* number of possible configuration */
};

_ALIGNED(64) USB_DEVICE_DESC gUSBHSStrgDevDesc = {
	//---- device descriptor ----
	USB_DEV_LENGTH,                 /* descriptor size                  */
	USB_DEVICE_DESCRIPTOR_TYPE,     /* descriptor type                  */
	USB_HSSTRG_DEV_RELEASE_NUMBER,    /* spec. release number           */
	USB_STRG_DEV_CLASS,             /* class code                       */
	USB_STRG_DEV_SUBCLASS,          /* sub class code                   */
	USB_STRG_DEV_PROTOCOL,          /* protocol code                    */
	USB_STRG_DEV_MAX_PACKET_SIZE0,  /* max packet size for endpoint 0   */
	USB_STRG_VENDOR_ID,             /* vendor id                        */
	USB_STRG_PRODUCT_ID,            /* product id                       */
	USB_STRG_DEV_RELASE_NUMBER,     /* device release number            */
	1,                              /* manifacturer string id           */
	2,                              /* product string id                */
	3,                              /* serial number string id          */
	USB_STRG_DEV_NUM_CONFIG         /* number of possible configuration */
};

_ALIGNED(64) USB_DEVICE_DESC gUSB2HSStrgDevDesc = {
	//---- device descriptor ----
	USB_DEV_LENGTH,                 /* descriptor size                  */
	USB_DEVICE_DESCRIPTOR_TYPE,     /* descriptor type                  */
	USB2_HSSTRG_DEV_RELEASE_NUMBER,    /* spec. release number           */
	USB_STRG_DEV_CLASS,             /* class code                       */
	USB_STRG_DEV_SUBCLASS,          /* sub class code                   */
	USB_STRG_DEV_PROTOCOL,          /* protocol code                    */
	USB_STRG_DEV_MAX_PACKET_SIZE0,  /* max packet size for endpoint 0   */
	USB_STRG_VENDOR_ID,             /* vendor id                        */
	USB_STRG_PRODUCT_ID,            /* product id                       */
	USB_STRG_DEV_RELASE_NUMBER,     /* device release number            */
	1,                              /* manifacturer string id           */
	2,                              /* product string id                */
	3,                              /* serial number string id          */
	USB_STRG_DEV_NUM_CONFIG         /* number of possible configuration */
};

_ALIGNED(64) USB_DEVICE_DESC gUSBFSStrgDevDesc = {
	//---- device descriptor ----
	USB_DEV_LENGTH,                 /* descriptor size                  */
	USB_DEVICE_DESCRIPTOR_TYPE,     /* descriptor type                  */
	USB_FSSTRG_DEV_RELEASE_NUMBER,  /* spec. release number             */
	USB_STRG_DEV_CLASS,             /* class code                       */
	USB_STRG_DEV_SUBCLASS,          /* sub class code                   */
	USB_STRG_DEV_PROTOCOL,          /* protocol code                    */
	USB_STRG_DEV_MAX_PACKET_SIZE0,  /* max packet size for endpoint 0   */
	USB_STRG_VENDOR_ID,             /* vendor id                        */
	USB_STRG_PRODUCT_ID,            /* product id                       */
	USB_STRG_DEV_RELASE_NUMBER,     /* device release number            */
	1,                              /* manifacturer string id           */
	2,                              /* product string id                */
	3,                              /* serial number string id          */
	USB_STRG_DEV_NUM_CONFIG         /* number of possible configuration */
};

#endif
#if 1

_ALIGNED(64) UINT8 gUSBSSStrgConfigDesc[USB_SS_STRG_CFGA_TOTAL_LENGTH] = {
	//---- configuration(cfg-A) -------
	USB_CFG_LENGTH,                         /* descriptor size                  */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,      /* descriptor type                  */
	USB_SS_STRG_CFGA_TOTAL_LENGTH & 0xff,   /* total length                     */
	USB_SS_STRG_CFGA_TOTAL_LENGTH >> 8,     /* total length                     */
	USB_STRG_CFGA_NUMBER_IF,                /* number of interface              */
	USB_STRG_CFGA_CFG_VALUE,                /* configuration value              */
	USB_STRG_CFGA_CFG_IDX,                  /* configuration string id          */
	USB_STRG_CFGA_CFG_ATTRIBUES_SELF,       /* characteristics   Self-Powered   */
	USB_SS_STRG_CFGA_MAX_POWER_SELF,                               /* maximum power in 8mA(0xD is 104mA)*/

	//----- cfg A interface0 ----
	//USB_INTERFACE_DESCRIPTOR, 9 bytes for Mass Storage
	// 0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,
	//---- alt0 --------
	USB_IF_LENGTH,                          /* descriptor size      */
	USB_INTERFACE_DESCRIPTOR_TYPE,          /* descriptor type      */
	USB_STRG_IF_IF0_NUMBER,                 /* interface number     */
	USB_STRG_IF_ALT0,                       /* alternate setting    */
	USB_STRG_IF_CFGA_IF0_NUMBER_EP,         /* number of endpoint   */
	USB_STRG_IF_CLASS_STORAGE,              /* interface class      */
	USB_STRG_IF_SUBCLASS_SCSI,              /* interface sub-class  */
	USB_STRG_IF_PROTOCOL_BULK_ONLY,         /* interface protocol   */
	USB_STRG_IF_IDX,                        /* interface string id  */

	//USB_ENDPOINT_DESCRIPTOR, ep1, bulk-in, packet size=64 bytes
	// 0x07, 0x05, 0x81, 0x02, 0x04, 0x00, 0x00
	//---- EP1 IN ----
	USB_EP_LENGTH,                          /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,           /* descriptor type      */
	USB_EP_EP1I_ADDRESS,                    /* Msdc_Open Runtime determined *//* endpoint address     */
	USB_EP_ATR_BULK,                        /* character address    */
	USB_SS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff,  /* max packet size      */
	USB_SS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,    /* max packet size      */
	USB_EP_BULK_INTERVAL,

	//SS_COMPANION,
	USB_SSEP_COMPANISON_LENGTH,             /* descriptor size      */
	USB_SS_EP_COMPANION_TYPE,               /* descriptor type      */
	8,                                      /* Max Burst Size       */
	0,
	0,
	0,


	//USB_ENDPOINT_DESCRIPTOR, ep2, bulk-out, packet size=64 bytes
	//0x07, 0x05, 0x02, 0x02, 0x04, 0x00, 0x00,
	//---- EP2 OUT ----
	USB_EP_LENGTH,                          /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,           /* descriptor type      */
	USB_EP_EP2O_ADDRESS,                    /* Msdc_Open Runtime determined *//* endpoint address     */
	USB_EP_ATR_BULK,                        /* character address    */
	USB_SS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE & 0xff,  /* max packet size      */
	USB_SS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE >> 8,    /* max packet size      */
	USB_EP_BULK_INTERVAL,

	//SS_COMPANION,
	USB_SSEP_COMPANISON_LENGTH,             /* descriptor size      */
	USB_SS_EP_COMPANION_TYPE,               /* descriptor type      */
	8,                                      /* Max Burst Size       */
	0,
	0,
	0
};


_ALIGNED(64) UINT8 gUSBHSStrgConfigDesc[USB_STRG_CFGA_TOTAL_LENGTH] = {
	//---- configuration(cfg-A) -------
	USB_CFG_LENGTH,                     /* descriptor size                  */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,  /* descriptor type                  */
	USB_STRG_CFGA_TOTAL_LENGTH & 0xff,  /* total length                     */
	USB_STRG_CFGA_TOTAL_LENGTH >> 8,    /* total length                     */
	USB_STRG_CFGA_NUMBER_IF,            /* number of interface              */
	USB_STRG_CFGA_CFG_VALUE,            /* configuration value              */
	USB_STRG_CFGA_CFG_IDX,              /* configuration string id          */
	USB_STRG_CFGA_CFG_ATTRIBUES_SELF,   /* characteristics                  */
	USB_STRG_CFGA_MAX_POWER_SELF,       /* maximum power in 2mA             */

	//----- cfg A interface0 ----
	//USB_INTERFACE_DESCRIPTOR, 9 bytes for Mass Storage
	// 0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,
	//---- alt0 --------
	USB_IF_LENGTH,                      /* descriptor size      */
	USB_INTERFACE_DESCRIPTOR_TYPE,      /* descriptor type      */
	USB_STRG_IF_IF0_NUMBER,             /* interface number     */
	USB_STRG_IF_ALT0,                   /* alternate setting    */
	USB_STRG_IF_CFGA_IF0_NUMBER_EP,     /* number of endpoint   */
	USB_STRG_IF_CLASS_STORAGE,          /* interface class      */
	USB_STRG_IF_SUBCLASS_SCSI,          /* interface sub-class  */
	USB_STRG_IF_PROTOCOL_BULK_ONLY,     /* interface protocol   */
	USB_STRG_IF_IDX,                    /* interface string id  */

	//USB_ENDPOINT_DESCRIPTOR, ep1, bulk-in, packet size=64 bytes
	// 0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00
	//---- EP1 IN ----
	USB_EP_LENGTH,                          /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,       /* descriptor type      */
	USB_EP_EP1I_ADDRESS, /* Msdc_Open Runtime determined *//* endpoint address     */
	USB_EP_ATR_BULK,                    /* character address    */
	USB_HS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff,  /* max packet size      */
	USB_HS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,    /* max packet size      */
	USB_EP_BULK_INTERVAL,

	//USB_ENDPOINT_DESCRIPTOR, ep2, bulk-out, packet size=64 bytes
	//0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,
	//---- EP2 OUT ----
	USB_EP_LENGTH,                          /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,       /* descriptor type      */
	USB_EP_EP2O_ADDRESS, /* Msdc_Open Runtime determined *//* endpoint address     */
	USB_EP_ATR_BULK,                    /* character address    */
	USB_HS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE & 0xff,  /* max packet size      */
	USB_HS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE >> 8,    /* max packet size      */
	USB_EP_BULK_INTERVAL
};

_ALIGNED(64) UINT8 gUSBFSStrgConfigDesc[USB_STRG_CFGA_TOTAL_LENGTH] = {
	//---- configuration(cfg-A) -------
	USB_CFG_LENGTH,                     /* descriptor size                  */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,  /* descriptor type                  */
	USB_STRG_CFGA_TOTAL_LENGTH & 0xff,  /* total length                     */
	USB_STRG_CFGA_TOTAL_LENGTH >> 8,    /* total length                     */
	USB_STRG_CFGA_NUMBER_IF,            /* number of interface              */
	USB_STRG_CFGA_CFG_VALUE,            /* configuration value              */
	USB_STRG_CFGA_CFG_IDX,              /* configuration string id          */
	USB_STRG_CFGA_CFG_ATTRIBUES_SELF,   /* characteristics                  */
	USB_STRG_CFGA_MAX_POWER_SELF,       /* maximum power in 2mA             */

	//----- cfg A interface0 ----
	//USB_INTERFACE_DESCRIPTOR, 9 bytes for Mass Storage
	// 0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,
	//---- alt0 --------
	USB_IF_LENGTH,                      /* descriptor size      */
	USB_INTERFACE_DESCRIPTOR_TYPE,      /* descriptor type      */
	USB_STRG_IF_IF0_NUMBER,             /* interface number     */
	USB_STRG_IF_ALT0,                   /* alternate setting    */
	USB_STRG_IF_CFGA_IF0_NUMBER_EP,     /* number of endpoint   */
	USB_STRG_IF_CLASS_STORAGE,          /* interface class      */
	USB_STRG_IF_SUBCLASS_SCSI,          /* interface sub-class  */
	USB_STRG_IF_PROTOCOL_BULK_ONLY,     /* interface protocol   */
	USB_STRG_IF_IDX,                    /* interface string id  */

	//USB_ENDPOINT_DESCRIPTOR, ep1, bulk-in, packet size=64 bytes
	// 0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00
	//---- EP1 IN ----
	USB_EP_LENGTH,                          /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,       /* descriptor type      */
	USB_EP_EP1I_ADDRESS,                /* endpoint address     */
	USB_EP_ATR_BULK,                    /* character address    */
	USB_FS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff,  /* max packet size      */
	USB_FS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,    /* max packet size      */
	USB_EP_BULK_INTERVAL,

	//USB_ENDPOINT_DESCRIPTOR, ep2, bulk-out, packet size=64 bytes
	//0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,
	//---- EP2 OUT ----
	USB_EP_LENGTH,                          /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,       /* descriptor type      */
	USB_EP_EP2O_ADDRESS,                /* endpoint address     */
	USB_EP_ATR_BULK,                    /* character address    */
	USB_FS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE & 0xff,  /* max packet size      */
	USB_FS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE >> 8,    /* max packet size      */
	USB_EP_BULK_INTERVAL
};

#endif
#if 1

/*
    USB3 Std 9-14. SuperSpeed has no OtherSpeed Descriptor
*/

_ALIGNED(64) UINT8 gUSBHSOtherStrgConfigDesc[USB_STRG_CFGA_TOTAL_LENGTH] = {
	//---- configuration(cfg-A) -------
	USB_CFG_LENGTH,                     /* descriptor size                  */
	USB_OTHER_SPEED_CONFIGURATION,      /* descriptor type                  */
	USB_STRG_CFGA_TOTAL_LENGTH & 0xff,  /* total length                     */
	USB_STRG_CFGA_TOTAL_LENGTH >> 8,    /* total length                     */
	USB_STRG_CFGA_NUMBER_IF,            /* number of interface              */
	USB_STRG_CFGA_CFG_VALUE,            /* configuration value              */
	USB_STRG_CFGA_CFG_IDX,              /* configuration string id          */
	USB_STRG_CFGA_CFG_ATTRIBUES_SELF,   /* characteristics                  */
	USB_STRG_CFGA_MAX_POWER_SELF,       /* maximum power in 2mA             */

	//----- cfg A interface0 ----
	//USB_INTERFACE_DESCRIPTOR, 9 bytes for Mass Storage
	// 0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,
	//---- alt0 --------
	USB_IF_LENGTH,                      /* descriptor size      */
	USB_INTERFACE_DESCRIPTOR_TYPE,      /* descriptor type      */
	USB_STRG_IF_IF0_NUMBER,             /* interface number     */
	USB_STRG_IF_ALT0,                   /* alternate setting    */
	USB_STRG_IF_CFGA_IF0_NUMBER_EP,     /* number of endpoint   */
	USB_STRG_IF_CLASS_STORAGE,          /* interface class      */
	USB_STRG_IF_SUBCLASS_SCSI,          /* interface sub-class  */
	USB_STRG_IF_PROTOCOL_BULK_ONLY,     /* interface protocol   */
	USB_STRG_IF_IDX,                    /* interface string id  */

	//USB_ENDPOINT_DESCRIPTOR, ep1, bulk-in, packet size=64 bytes
	// 0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00
	//---- EP1 IN ----
	USB_EP_LENGTH,                          /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,       /* descriptor type      */
	USB_EP_EP1I_ADDRESS,                /* endpoint address     */
	USB_EP_ATR_BULK,                    /* character address    */
	USB_FS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff,  /* max packet size      */
	USB_FS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,    /* max packet size      */
	USB_EP_BULK_INTERVAL,

	//USB_ENDPOINT_DESCRIPTOR, ep2, bulk-out, packet size=64 bytes
	//0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,
	//---- EP2 OUT ----
	USB_EP_LENGTH,                          /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,       /* descriptor type      */
	USB_EP_EP2O_ADDRESS,                /* endpoint address     */
	USB_EP_ATR_BULK,                    /* character address    */
	USB_FS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE & 0xff,  /* max packet size      */
	USB_FS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE >> 8,    /* max packet size      */
	USB_EP_BULK_INTERVAL
};


_ALIGNED(64) UINT8 gUSBFSOtherStrgConfigDesc[USB_STRG_CFGA_TOTAL_LENGTH] = {
	//---- configuration(cfg-A) -------
	USB_CFG_LENGTH,                     /* descriptor size                  */
	USB_OTHER_SPEED_CONFIGURATION,      /* descriptor type                  */
	USB_STRG_CFGA_TOTAL_LENGTH & 0xff,  /* total length                     */
	USB_STRG_CFGA_TOTAL_LENGTH >> 8,    /* total length                     */
	USB_STRG_CFGA_NUMBER_IF,            /* number of interface              */
	USB_STRG_CFGA_CFG_VALUE,            /* configuration value              */
	USB_STRG_CFGA_CFG_IDX,              /* configuration string id          */
	USB_STRG_CFGA_CFG_ATTRIBUES_SELF,   /* characteristics                  */
	USB_STRG_CFGA_MAX_POWER_SELF,       /* maximum power in 2mA             */

	//----- cfg A interface0 ----
	//USB_INTERFACE_DESCRIPTOR, 9 bytes for Mass Storage
	// 0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,
	//---- alt0 --------
	USB_IF_LENGTH,                      /* descriptor size      */
	USB_INTERFACE_DESCRIPTOR_TYPE,      /* descriptor type      */
	USB_STRG_IF_IF0_NUMBER,             /* interface number     */
	USB_STRG_IF_ALT0,                   /* alternate setting    */
	USB_STRG_IF_CFGA_IF0_NUMBER_EP,     /* number of endpoint   */
	USB_STRG_IF_CLASS_STORAGE,          /* interface class      */
	USB_STRG_IF_SUBCLASS_SCSI,          /* interface sub-class  */
	USB_STRG_IF_PROTOCOL_BULK_ONLY,     /* interface protocol   */
	USB_STRG_IF_IDX,                    /* interface string id  */

	//USB_ENDPOINT_DESCRIPTOR, ep1, bulk-in, packet size=64 bytes
	// 0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00
	//---- EP1 IN ----
	USB_EP_LENGTH,                          /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,       /* descriptor type      */
	USB_EP_EP1I_ADDRESS,                /* endpoint address     */
	USB_EP_ATR_BULK,                    /* character address    */
	USB_HS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE & 0xff,  /* max packet size      */
	USB_HS_STRG_CFGA_INTF0_ALT0_EP1_MAX_PACKET_SIZE >> 8,    /* max packet size      */
	USB_EP_BULK_INTERVAL,

	//USB_ENDPOINT_DESCRIPTOR, ep2, bulk-out, packet size=64 bytes
	//0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,
	//---- EP2 OUT ----
	USB_EP_LENGTH,                          /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,       /* descriptor type      */
	USB_EP_EP2O_ADDRESS,                /* endpoint address     */
	USB_EP_ATR_BULK,                    /* character address    */
	USB_HS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE & 0xff,  /* max packet size      */
	USB_HS_STRG_CFGA_INTF0_ALT0_EP2_MAX_PACKET_SIZE >> 8,    /* max packet size      */
	USB_EP_BULK_INTERVAL
};

#endif
#if 1

/*
    USB3 Std 9-14. SuperSpeed has no Device Qualifier Descriptor
*/

_ALIGNED(64) const USB_DEVICE_DESC gUSBHSStrgDevQualiDesc[] = {
	USB_DEV_QUALI_LENGTH,
	USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,
	USB_HSSTRG_DEV_RELEASE_NUMBER,              /* spec. release number             */
	USB_STRG_DEV_CLASS,                         /* class code                       */
	USB_STRG_DEV_SUBCLASS,                      /* sub class code                   */
	USB_STRG_DEV_PROTOCOL,                      /* protocol code                    */
	USB_STRG_DEV_MAX_PACKET_SIZE0,              /* max packet size for endpoint 0   */
	USB_STRG_DEV_NUM_CONFIG,                    /* number of possible configuration */
	0                                           /*reserved                          */
};

_ALIGNED(64) const USB_DEVICE_DESC gUSB2HSStrgDevQualiDesc[] = {
	USB_DEV_QUALI_LENGTH,
	USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,
	USB2_HSSTRG_DEV_RELEASE_NUMBER,              /* spec. release number             */
	USB_STRG_DEV_CLASS,                         /* class code                       */
	USB_STRG_DEV_SUBCLASS,                      /* sub class code                   */
	USB_STRG_DEV_PROTOCOL,                      /* protocol code                    */
	USB_STRG_DEV_MAX_PACKET_SIZE0,              /* max packet size for endpoint 0   */
	USB_STRG_DEV_NUM_CONFIG,                    /* number of possible configuration */
	0                                           /*reserved                          */
};

_ALIGNED(64) const USB_DEVICE_DESC gUSBFSStrgDevQualiDesc[] = {
	USB_DEV_QUALI_LENGTH,
	USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,
	USB_FSSTRG_DEV_RELEASE_NUMBER,             /* spec. release number             */
	USB_STRG_DEV_CLASS,                         /* class code                       */
	USB_STRG_DEV_SUBCLASS,                      /* sub class code                   */
	USB_STRG_DEV_PROTOCOL,                      /* protocol code                    */
	USB_STRG_DEV_MAX_PACKET_SIZE0,              /* max packet size for endpoint 0   */
	USB_STRG_DEV_NUM_CONFIG,                    /* number of possible configuration */
	0                                           /*reserved                          */
};

#endif
#if 1

_ALIGNED(64) UINT8 gUSBSSStrgBosDesc[] = {
	USB_STRG_BOS_LENGTH,
	USB_BOS_DESCRIPTOR_TYPE,
	USB_SS_STRG_BOS_TOTAL_LENGTH & 0xFF,
	USB_SS_STRG_BOS_TOTAL_LENGTH >> 8,
	USB_SS_STRG_BOS_DEVCAP_NUMBER,

	7,
	USB_SS_DEVCAP_DESCRIPTOR_TYPE,
	0x02,
	0x02,
	0,
	0,
	0,

	10,
	USB_SS_DEVCAP_DESCRIPTOR_TYPE,
	3,
	0,
	0x0E,
	0,
	1,
	10,
	0xFF,
	0x07
};
_ALIGNED(64) UINT8 gUSBHSStrgBosDesc[] = {
	USB_STRG_BOS_LENGTH,
	USB_BOS_DESCRIPTOR_TYPE,
	USB_SS_STRG_BOS_TOTAL_LENGTH & 0xFF,
	USB_SS_STRG_BOS_TOTAL_LENGTH >> 8,
	USB_SS_STRG_BOS_DEVCAP_NUMBER,

	7,
	USB_SS_DEVCAP_DESCRIPTOR_TYPE,
	0x02,
	0x06,
	0,
	0,
	0,

	10,
	USB_SS_DEVCAP_DESCRIPTOR_TYPE,
	3,
	0,
	0x0E,
	0,
	1,
	10,
	0xFF,
	0x07
};

#endif
