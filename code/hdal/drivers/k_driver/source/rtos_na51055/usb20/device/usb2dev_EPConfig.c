/*
    USB device Endpoint setting functions

    Setting Endpoint and FIFO configuration etc...

    @file       usb_EPConfig.c
    @ingroup    mIDrvUSB_Device
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/

#include "usb2dev.h"
#include "usb2dev_reg.h"
#include "usb2dev_int.h"

static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

BOOL    bAbortDMA       = FALSE;

/* saving EP & FIFO mapping information */

// record EP maps to which FIFO
UINT32  gEPMap[USB_EP_MAX - 1] = {
	USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE,
	USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE, USB_FIFO_NOT_USE
};

// record IN FIFO maps to which EP
UINT32  gFIFOInMap[USB_FIFO_MAX] = {
	USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE
};

// record OUT FIFO maps to which EP
UINT32  gFIFOOutMap[USB_FIFO_MAX] = {
	USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE, USB_EP_NOT_USE
};


USB_AUTOHDR_CFG gUvcHdrCfg[USB_EP_MAX] = {
	{DISABLE,   FALSE,  FALSE, 0}, {DISABLE,   FALSE,  FALSE, 0}, {DISABLE,   FALSE,  FALSE, 0},
	{DISABLE,   FALSE,  FALSE, 0}, {DISABLE,   FALSE,  FALSE, 0}, {DISABLE,   FALSE,  FALSE, 0},
	{DISABLE,   FALSE,  FALSE, 0}, {DISABLE,   FALSE,  FALSE, 0}, {DISABLE,   FALSE,  FALSE, 0},
	{DISABLE,   FALSE,  FALSE, 0}, {DISABLE,   FALSE,  FALSE, 0}, {DISABLE,   FALSE,  FALSE, 0},
	{DISABLE,   FALSE,  FALSE, 0}, {DISABLE,   FALSE,  FALSE, 0}, {DISABLE,   FALSE,  FALSE, 0},
	{DISABLE,   FALSE,  FALSE, 0}
};


/**
    @addtogroup mIDrvUSB_Device
*/
//@{


/*
    Set endpoint and FIFO mapping

    This function is internal FIFO setting functions

    @param[in] EPn      endpoint number (1~8)
    @param[in] Dir      endpoint direction
                        - @b EP_DIR_IN: endpoint direction IN
                        - @b EP_DIR_OUT: endpoint direction OUT
    @param[in] FIFOn    FIFO number (0~3)

    @return void
*/
static void usb_setEPMap(USB_EP EPn, USB_EP_DIR Dir, USB_FIFO_NUM FIFOn)
{
	T_USB_DEVMAXPS_EP_REG   DevEpMaxPSReg;

	// Config EPx is IN or OUT
	DevEpMaxPSReg.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((EPn - USB_EP1) << 2));
	if (Dir == EP_DIR_IN) {
		DevEpMaxPSReg.bit.DIR_EP = 0;
		DevEpMaxPSReg.bit.FNO_IEP = FIFOn;
	} else if (Dir == EP_DIR_OUT) {
		DevEpMaxPSReg.bit.DIR_EP = 1;
		DevEpMaxPSReg.bit.FNO_OEP = FIFOn;
	}
	USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((EPn - USB_EP1) << 2)), DevEpMaxPSReg.reg);

}

/*
    Set FIFO configuration

    This function is internal FIFO setting functions

    @param[in] FIFOn        FIFO number (0~3)
    @param[in] BLK_TYP      transfer type
                            - @b EP_TYPE_ISOCHRONOUS: Endpoint transfer type is isochronous
                            - @b EP_TYPE_BULK: Endpoint transfer type is bulk
                            - @b EP_TYPE_INTERRUPT: Endpoint transfer type is interrupt
    @param[in] BLKNO        block number
                            - @b BLKNUM_SINGLE: use single block
                            - @b BLKNUM_DOUBLE: use double block
                            - @b BLKNUM_TRIPLE: use triple block
    @param[in] BLKSZ        block size
                            - @b TRUE: 512~1024 bytes
                            - @b FALSE: <=512 bytes

    @return void
*/
static void USB_setFIFOCfg(USB_FIFO_NUM FIFOn, USB_EP_TYPE BLK_TYP, USB_EP_BLKNUM BLKNO, BOOL BLKSZ, USB_FIFO_DIR Dir)
{
	T_USB_DEVBUFCFG0_REG FIFOCfgReg;
	T_USB_DEVBUFCFG1_REG FIFOCfg1Reg;

	FIFOCfgReg.reg  = USB_GETREG(USB_DEVBUFCFG0_REG_OFS);
	FIFOCfg1Reg.reg = USB_GETREG(USB_DEVBUFCFG1_REG_OFS);

	switch (FIFOn) {
	case USB_FIFO0: {
			FIFOCfgReg.bit.BUF0_TYP             = BLK_TYP;
			FIFOCfgReg.bit.BUF0_BLKNO           = BLKNO - 1;
			FIFOCfgReg.bit.BUF0_BLKSZ           = BLKSZ;
			FIFOCfgReg.bit.BUF0_DIR             = Dir;
			FIFOCfgReg.bit.BUF0_EN              = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfgReg.bit.BUF1_TYP         = BLK_TYP;
				FIFOCfgReg.bit.BUF1_BLKNO       = BLKNO - 1;
				FIFOCfgReg.bit.BUF1_BLKSZ       = BLKSZ;
				FIFOCfgReg.bit.BUF1_DIR         = Dir;
				FIFOCfgReg.bit.BUF1_EN          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfgReg.bit.BUF1_TYP         = BLK_TYP;
				FIFOCfgReg.bit.BUF1_BLKNO       = BLKNO - 1;
				FIFOCfgReg.bit.BUF1_BLKSZ       = BLKSZ;
				FIFOCfgReg.bit.BUF1_DIR         = Dir;
				FIFOCfgReg.bit.BUF1_EN          = 0;

				if (BLKSZ == TRUE) {
					// Block size : 512~1024 bytes
					FIFOCfgReg.bit.BUF2_TYP     = BLK_TYP;
					FIFOCfgReg.bit.BUF2_BLKNO   = BLKNO - 1;
					FIFOCfgReg.bit.BUF2_BLKSZ   = BLKSZ;
					FIFOCfgReg.bit.BUF2_DIR     = Dir;
					FIFOCfgReg.bit.BUF2_EN      = 0;

					FIFOCfgReg.bit.BUF3_TYP     = BLK_TYP;
					FIFOCfgReg.bit.BUF3_BLKNO   = BLKNO - 1;
					FIFOCfgReg.bit.BUF3_BLKSZ   = BLKSZ;
					FIFOCfgReg.bit.BUF3_DIR     = Dir;
					FIFOCfgReg.bit.BUF3_EN      = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfgReg.bit.BUF1_TYP         = BLK_TYP;
				FIFOCfgReg.bit.BUF1_BLKNO       = BLKNO - 1;
				FIFOCfgReg.bit.BUF1_BLKSZ       = BLKSZ;
				FIFOCfgReg.bit.BUF1_DIR         = Dir;
				FIFOCfgReg.bit.BUF1_EN          = 0;

				FIFOCfgReg.bit.BUF2_TYP         = BLK_TYP;
				FIFOCfgReg.bit.BUF2_BLKNO       = BLKNO - 1;
				FIFOCfgReg.bit.BUF2_BLKSZ       = BLKSZ;
				FIFOCfgReg.bit.BUF2_DIR         = Dir;
				FIFOCfgReg.bit.BUF2_EN          = 0;

				// Block size : 512~1024 bytes. Exceed boundary.
				if (BLKSZ == TRUE) {
					DBG_ERR("BLKNO %d, BLKSZ %d exceed resource\r\n", BLKNO, BLKSZ);
				}
			}
		}
		break;

	case USB_FIFO1: {
			FIFOCfgReg.bit.BUF1_TYP             = BLK_TYP;
			FIFOCfgReg.bit.BUF1_BLKNO           = BLKNO - 1;
			FIFOCfgReg.bit.BUF1_BLKSZ           = BLKSZ;
			FIFOCfgReg.bit.BUF1_DIR             = Dir;
			FIFOCfgReg.bit.BUF1_EN              = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfgReg.bit.BUF2_TYP         = BLK_TYP;
				FIFOCfgReg.bit.BUF2_BLKNO       = BLKNO - 1;
				FIFOCfgReg.bit.BUF2_BLKSZ       = BLKSZ;
				FIFOCfgReg.bit.BUF2_DIR         = Dir;
				FIFOCfgReg.bit.BUF2_EN          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfgReg.bit.BUF2_TYP         = BLK_TYP;
				FIFOCfgReg.bit.BUF2_BLKNO       = BLKNO - 1;
				FIFOCfgReg.bit.BUF2_BLKSZ       = BLKSZ;
				FIFOCfgReg.bit.BUF2_DIR         = Dir;
				FIFOCfgReg.bit.BUF2_EN          = 0;

				// Block size : 512~1024 bytes. Exceed boundary.
				if (BLKSZ == TRUE) {
					DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfgReg.bit.BUF2_TYP         = BLK_TYP;
				FIFOCfgReg.bit.BUF2_BLKNO       = BLKNO - 1;
				FIFOCfgReg.bit.BUF2_BLKSZ       = BLKSZ;
				FIFOCfgReg.bit.BUF2_DIR         = Dir;
				FIFOCfgReg.bit.BUF2_EN          = 0;

				FIFOCfgReg.bit.BUF3_TYP         = BLK_TYP;
				FIFOCfgReg.bit.BUF3_BLKNO       = BLKNO - 1;
				FIFOCfgReg.bit.BUF3_BLKSZ       = BLKSZ;
				FIFOCfgReg.bit.BUF3_DIR         = Dir;
				FIFOCfgReg.bit.BUF3_EN          = 0;

				// Block size : 512~1024 bytes. Exceed boundary.
				if (BLKSZ == TRUE) {
					DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
				}
			}
		}
		break;

	case USB_FIFO2: {
			FIFOCfgReg.bit.BUF2_TYP             = BLK_TYP;
			FIFOCfgReg.bit.BUF2_BLKNO           = BLKNO - 1;
			FIFOCfgReg.bit.BUF2_BLKSZ           = BLKSZ;
			FIFOCfgReg.bit.BUF2_DIR             = Dir;
			FIFOCfgReg.bit.BUF2_EN              = 1;


			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfgReg.bit.BUF3_TYP         = BLK_TYP;
				FIFOCfgReg.bit.BUF3_BLKNO       = BLKNO - 1;
				FIFOCfgReg.bit.BUF3_BLKSZ       = BLKSZ;
				FIFOCfgReg.bit.BUF3_DIR         = Dir;
				FIFOCfgReg.bit.BUF3_EN          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfgReg.bit.BUF3_TYP         = BLK_TYP;
				FIFOCfgReg.bit.BUF3_BLKNO       = BLKNO - 1;
				FIFOCfgReg.bit.BUF3_BLKSZ       = BLKSZ;
				FIFOCfgReg.bit.BUF3_DIR         = Dir;
				FIFOCfgReg.bit.BUF3_EN          = 0;

				// Block size : 512~1024 bytes
				if (BLKSZ == TRUE) {
					DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
			}
		}
		break;

	case USB_FIFO3: {
			FIFOCfgReg.bit.BUF3_TYP             = BLK_TYP;
			FIFOCfgReg.bit.BUF3_BLKNO           = BLKNO - 1;
			FIFOCfgReg.bit.BUF3_BLKSZ           = BLKSZ;
			FIFOCfgReg.bit.BUF3_DIR             = Dir;
			FIFOCfgReg.bit.BUF3_EN              = 1;

			if ((BLKNO > BLKNUM_SINGLE) || (BLKSZ == TRUE)) {
				DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
			}
		}
		break;

	case USB_FIFO4: {
			FIFOCfg1Reg.bit.BUF4_TYP             = BLK_TYP;
			FIFOCfg1Reg.bit.BUF4_BLKNO           = BLKNO - 1;
			FIFOCfg1Reg.bit.BUF4_BLKSZ           = BLKSZ;
			FIFOCfg1Reg.bit.BUF4_DIR             = Dir;
			FIFOCfg1Reg.bit.BUF4_EN              = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg1Reg.bit.BUF5_TYP         = BLK_TYP;
				FIFOCfg1Reg.bit.BUF5_BLKNO       = BLKNO - 1;
				FIFOCfg1Reg.bit.BUF5_BLKSZ       = BLKSZ;
				FIFOCfg1Reg.bit.BUF5_DIR         = Dir;
				FIFOCfg1Reg.bit.BUF5_EN          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg1Reg.bit.BUF5_TYP         = BLK_TYP;
				FIFOCfg1Reg.bit.BUF5_BLKNO       = BLKNO - 1;
				FIFOCfg1Reg.bit.BUF5_BLKSZ       = BLKSZ;
				FIFOCfg1Reg.bit.BUF5_DIR         = Dir;
				FIFOCfg1Reg.bit.BUF5_EN          = 0;

				if (BLKSZ == TRUE) {
					// Block size : 512~1024 bytes
					FIFOCfg1Reg.bit.BUF6_TYP     = BLK_TYP;
					FIFOCfg1Reg.bit.BUF6_BLKNO   = BLKNO - 1;
					FIFOCfg1Reg.bit.BUF6_BLKSZ   = BLKSZ;
					FIFOCfg1Reg.bit.BUF6_DIR     = Dir;
					FIFOCfg1Reg.bit.BUF6_EN      = 0;

					FIFOCfg1Reg.bit.BUF7_TYP     = BLK_TYP;
					FIFOCfg1Reg.bit.BUF7_BLKNO   = BLKNO - 1;
					FIFOCfg1Reg.bit.BUF7_BLKSZ   = BLKSZ;
					FIFOCfg1Reg.bit.BUF7_DIR     = Dir;
					FIFOCfg1Reg.bit.BUF7_EN      = 0;
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfg1Reg.bit.BUF5_TYP         = BLK_TYP;
				FIFOCfg1Reg.bit.BUF5_BLKNO       = BLKNO - 1;
				FIFOCfg1Reg.bit.BUF5_BLKSZ       = BLKSZ;
				FIFOCfg1Reg.bit.BUF5_DIR         = Dir;
				FIFOCfg1Reg.bit.BUF5_EN          = 0;

				FIFOCfg1Reg.bit.BUF6_TYP         = BLK_TYP;
				FIFOCfg1Reg.bit.BUF6_BLKNO       = BLKNO - 1;
				FIFOCfg1Reg.bit.BUF6_BLKSZ       = BLKSZ;
				FIFOCfg1Reg.bit.BUF6_DIR         = Dir;
				FIFOCfg1Reg.bit.BUF6_EN          = 0;

				// Block size : 512~1024 bytes. Exceed boundary.
				if (BLKSZ == TRUE) {
					DBG_ERR("BLKNO %d, BLKSZ %d exceed resource\r\n", BLKNO, BLKSZ);
				}
			}
		}
		break;

	case USB_FIFO5: {
			FIFOCfg1Reg.bit.BUF5_TYP             = BLK_TYP;
			FIFOCfg1Reg.bit.BUF5_BLKNO           = BLKNO - 1;
			FIFOCfg1Reg.bit.BUF5_BLKSZ           = BLKSZ;
			FIFOCfg1Reg.bit.BUF5_DIR             = Dir;
			FIFOCfg1Reg.bit.BUF5_EN              = 1;

			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg1Reg.bit.BUF6_TYP         = BLK_TYP;
				FIFOCfg1Reg.bit.BUF6_BLKNO       = BLKNO - 1;
				FIFOCfg1Reg.bit.BUF6_BLKSZ       = BLKSZ;
				FIFOCfg1Reg.bit.BUF6_DIR         = Dir;
				FIFOCfg1Reg.bit.BUF6_EN          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg1Reg.bit.BUF6_TYP         = BLK_TYP;
				FIFOCfg1Reg.bit.BUF6_BLKNO       = BLKNO - 1;
				FIFOCfg1Reg.bit.BUF6_BLKSZ       = BLKSZ;
				FIFOCfg1Reg.bit.BUF6_DIR         = Dir;
				FIFOCfg1Reg.bit.BUF6_EN          = 0;

				// Block size : 512~1024 bytes. Exceed boundary.
				if (BLKSZ == TRUE) {
					DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				FIFOCfg1Reg.bit.BUF6_TYP         = BLK_TYP;
				FIFOCfg1Reg.bit.BUF6_BLKNO       = BLKNO - 1;
				FIFOCfg1Reg.bit.BUF6_BLKSZ       = BLKSZ;
				FIFOCfg1Reg.bit.BUF6_DIR         = Dir;
				FIFOCfg1Reg.bit.BUF6_EN          = 0;

				FIFOCfg1Reg.bit.BUF7_TYP         = BLK_TYP;
				FIFOCfg1Reg.bit.BUF7_BLKNO       = BLKNO - 1;
				FIFOCfg1Reg.bit.BUF7_BLKSZ       = BLKSZ;
				FIFOCfg1Reg.bit.BUF7_DIR         = Dir;
				FIFOCfg1Reg.bit.BUF7_EN          = 0;

				// Block size : 512~1024 bytes. Exceed boundary.
				if (BLKSZ == TRUE) {
					DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
				}
			}
		}
		break;

	case USB_FIFO6: {
			FIFOCfg1Reg.bit.BUF6_TYP             = BLK_TYP;
			FIFOCfg1Reg.bit.BUF6_BLKNO           = BLKNO - 1;
			FIFOCfg1Reg.bit.BUF6_BLKSZ           = BLKSZ;
			FIFOCfg1Reg.bit.BUF6_DIR             = Dir;
			FIFOCfg1Reg.bit.BUF6_EN              = 1;


			if ((BLKNO == BLKNUM_SINGLE) && (BLKSZ == TRUE)) {
				FIFOCfg1Reg.bit.BUF7_TYP         = BLK_TYP;
				FIFOCfg1Reg.bit.BUF7_BLKNO       = BLKNO - 1;
				FIFOCfg1Reg.bit.BUF7_BLKSZ       = BLKSZ;
				FIFOCfg1Reg.bit.BUF7_DIR         = Dir;
				FIFOCfg1Reg.bit.BUF7_EN          = 0;
			} else if (BLKNO == BLKNUM_DOUBLE) {
				FIFOCfg1Reg.bit.BUF7_TYP         = BLK_TYP;
				FIFOCfg1Reg.bit.BUF7_BLKNO       = BLKNO - 1;
				FIFOCfg1Reg.bit.BUF7_BLKSZ       = BLKSZ;
				FIFOCfg1Reg.bit.BUF7_DIR         = Dir;
				FIFOCfg1Reg.bit.BUF7_EN          = 0;

				// Block size : 512~1024 bytes
				if (BLKSZ == TRUE) {
					DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
				}
			} else if (BLKNO == BLKNUM_TRIPLE) {
				DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
			}
		}
		break;

	case USB_FIFO7: {
			FIFOCfg1Reg.bit.BUF7_TYP             = BLK_TYP;
			FIFOCfg1Reg.bit.BUF7_BLKNO           = BLKNO - 1;
			FIFOCfg1Reg.bit.BUF7_BLKSZ           = BLKSZ;
			FIFOCfg1Reg.bit.BUF7_DIR             = Dir;
			FIFOCfg1Reg.bit.BUF7_EN              = 1;

			if ((BLKNO > BLKNUM_SINGLE) || (BLKSZ == TRUE)) {
				DBG_ERR("FIFOn %d, BLKNO %d, BLKSZ %d exceed resource\r\n", FIFOn, BLKNO, BLKSZ);
			}
		}
		break;

	default:
		break;
	}


	USB_SETREG(USB_DEVBUFCFG0_REG_OFS, FIFOCfgReg.reg);
	USB_SETREG(USB_DEVBUFCFG1_REG_OFS, FIFOCfg1Reg.reg);

}


/*
    Set endpoint max packet size

    This function is internal FIFO setting functions

    @param[in] EPn      endpoint number (1~8)
    @param[in] Dir      endpoint direction
     - @b EP_DIR_IN: endpoint direction IN
     - @b EP_DIR_OUT: endpoint direction OUT
    @param[in] MaxPksz  max packet size (0~0x1ffff)

    @return void
*/
static void usb_setEPMaxpktsize(USB_EP EPn, USB_EP_DIR Dir, UINT32 MaxPksz)
{
	T_USB_DEVMAXPS_EP_REG  DevEPMaxPS;

	DevEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((EPn - USB_EP1) << 2));
	DevEPMaxPS.bit.MAXPS_EP = MaxPksz;
	USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((EPn - 1) << 2)), DevEPMaxPS.reg);
}

/*
    Set transaction number for high band width endpoint

    This function is internal FIFO setting functions

    @param[in] EPn      endpoint number (1~8)
    @param[in] HBW      high band width setting
                        - @b HBW_NOT: not high bandwidth
                        - @b HBW_1: one transaction
                        - @b HBW_2: two transaction
                        - @b HBW_3: three transaction

    @return void
*/
static void usb_setEPHighBandWidth(USB_EP EPn, USB_EP_HBW HBW)
{
	T_USB_DEVMAXPS_EP_REG  DevEPMaxPS;

	DevEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((EPn - USB_EP1) << 2));
	DevEPMaxPS.bit.TX_NUM_HBW_EP = HBW;
	USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((EPn - 1) << 2)), DevEPMaxPS.reg);
}

/*
    Config ISO-IN UVC Header Function
*/
static void usb_configUvcHeader(UINT32 EPi, UINT32 FIFOn, USB_ENP_CONFIG *pEnpCfg)
{
	if ((pEnpCfg->trnsfer_type == EP_TYPE_ISOCHRONOUS) && (gUvcHdrCfg[EPi + 1].bEn)) {
		T_USB_DEVISOIN_HDRCTRL0_REG     RegHdr1Ctr0, RegHdr2Ctr0;
		T_USB_DEVISOIN_HDRSW0_REG       RegHdr1Sw0, RegHdr2Sw0;

		RegHdr1Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS);
		RegHdr2Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS);

#if 1 // Use Header Function 1 First
		if ((RegHdr1Ctr0.bit.HDR_EN == 0) && (FIFOn >= USB_FIFO4)) {
			/*Use HDR Function 1*/

			RegHdr1Ctr0.bit.HDR_EN      = 1;
			RegHdr1Ctr0.bit.FIFO_SEL    = (FIFOn - USB_FIFO4);

			RegHdr1Ctr0.bit.HDR_START   = gUvcHdrCfg[EPi + 1].bStart;
			RegHdr1Ctr0.bit.HDR_STOP    = gUvcHdrCfg[EPi + 1].bStop;

			// Two Header function cfg can not set to the same fifo
			if (RegHdr2Ctr0.bit.FIFO_SEL == RegHdr1Ctr0.bit.FIFO_SEL) {
				RegHdr2Ctr0.bit.FIFO_SEL = ~RegHdr1Ctr0.bit.FIFO_SEL;
			}

			if (gUvcHdrCfg[EPi + 1].uiLength == 12) {
				RegHdr1Ctr0.bit.HDR_LEN     = 1;

				RegHdr1Sw0.reg              = 0;
				RegHdr1Sw0.bit.HDR_LENGTH   = 12;
				RegHdr1Sw0.bit.PTS          = 1;
				RegHdr1Sw0.bit.SCR          = 1;
				RegHdr1Sw0.bit.RES          = 0;
				RegHdr1Sw0.bit.STI          = 0;
				RegHdr1Sw0.bit.ERR          = 0;
				RegHdr1Sw0.bit.EOH          = 1;

				USB_SETREG(USB_DEVISOIN_HDR1SW0_REG_OFS,  RegHdr1Sw0.reg);
			} else {
				RegHdr1Ctr0.bit.HDR_LEN  = 0;

				RegHdr1Sw0.reg              = 0;
				RegHdr1Sw0.bit.HDR_LENGTH   = 2;
				RegHdr1Sw0.bit.PTS          = 0;
				RegHdr1Sw0.bit.SCR          = 0;
				RegHdr1Sw0.bit.RES          = 0;
				RegHdr1Sw0.bit.STI          = 0;
				RegHdr1Sw0.bit.ERR          = 0;
				RegHdr1Sw0.bit.EOH          = 1;

				USB_SETREG(USB_DEVISOIN_HDR1SW0_REG_OFS,  RegHdr1Sw0.reg);
			}
		} else if ((RegHdr2Ctr0.bit.HDR_EN == 0) && (FIFOn >= USB_FIFO4)) {
			/*Use HDR Function 2*/

			RegHdr2Ctr0.bit.HDR_EN      = 1;
			RegHdr2Ctr0.bit.FIFO_SEL    = (FIFOn - USB_FIFO4);

			RegHdr2Ctr0.bit.HDR_START   = gUvcHdrCfg[EPi + 1].bStart;
			RegHdr2Ctr0.bit.HDR_STOP    = gUvcHdrCfg[EPi + 1].bStop;

			if (gUvcHdrCfg[EPi + 1].uiLength == 12) {
				RegHdr2Ctr0.bit.HDR_LEN     = 1;

				RegHdr2Sw0.reg              = 0;
				RegHdr2Sw0.bit.HDR_LENGTH   = 12;
				RegHdr2Sw0.bit.PTS          = 1;
				RegHdr2Sw0.bit.SCR          = 1;
				RegHdr2Sw0.bit.RES          = 0;
				RegHdr2Sw0.bit.STI          = 0;
				RegHdr2Sw0.bit.ERR          = 0;
				RegHdr2Sw0.bit.EOH          = 1;

				USB_SETREG(USB_DEVISOIN_HDR2SW0_REG_OFS,  RegHdr2Sw0.reg);
			} else {
				RegHdr2Ctr0.bit.HDR_LEN  = 0;

				RegHdr2Sw0.reg              = 0;
				RegHdr2Sw0.bit.HDR_LENGTH   = 2;
				RegHdr2Sw0.bit.PTS          = 0;
				RegHdr2Sw0.bit.SCR          = 0;
				RegHdr2Sw0.bit.RES          = 0;
				RegHdr2Sw0.bit.STI          = 0;
				RegHdr2Sw0.bit.ERR          = 0;
				RegHdr2Sw0.bit.EOH          = 1;

				USB_SETREG(USB_DEVISOIN_HDR2SW0_REG_OFS,  RegHdr2Sw0.reg);
			}

		} else {
			DBG_ERR("No Header Res\r\n");
		}
#else // Use Header Function 2 First
		if ((RegHdr2Ctr0.bit.HDR_EN == 0) && (FIFOn >= USB_FIFO4)) {
			/*Use HDR Function 2*/

			RegHdr2Ctr0.bit.HDR_EN      = 1;
			RegHdr2Ctr0.bit.FIFO_SEL    = (FIFOn - USB_FIFO4);

			RegHdr2Ctr0.bit.HDR_START   = gUvcHdrCfg[EPi + 1].bStart;
			RegHdr2Ctr0.bit.HDR_STOP    = gUvcHdrCfg[EPi + 1].bStop;

			// Two Header function cfg can not set to the same fifo
			if (RegHdr2Ctr0.bit.FIFO_SEL == RegHdr1Ctr0.bit.FIFO_SEL) {
				RegHdr1Ctr0.bit.FIFO_SEL = ~RegHdr2Ctr0.bit.FIFO_SEL;
			}

			if (gUvcHdrCfg[EPi + 1].uiLength == 12) {
				RegHdr2Ctr0.bit.HDR_LEN     = 1;

				RegHdr2Sw0.reg              = 0;
				RegHdr2Sw0.bit.HDR_LENGTH   = 12;
				RegHdr2Sw0.bit.PTS          = 1;
				RegHdr2Sw0.bit.SCR          = 1;
				RegHdr2Sw0.bit.RES          = 0;
				RegHdr2Sw0.bit.STI          = 0;
				RegHdr2Sw0.bit.ERR          = 0;
				RegHdr2Sw0.bit.EOH          = 1;

				USB_SETREG(USB_DEVISOIN_HDR2SW0_REG_OFS,  RegHdr2Sw0.reg);
			} else {
				RegHdr2Ctr0.bit.HDR_LEN  = 0;

				RegHdr2Sw0.reg              = 0;
				RegHdr2Sw0.bit.HDR_LENGTH   = 2;
				RegHdr2Sw0.bit.PTS          = 0;
				RegHdr2Sw0.bit.SCR          = 0;
				RegHdr2Sw0.bit.RES          = 0;
				RegHdr2Sw0.bit.STI          = 0;
				RegHdr2Sw0.bit.ERR          = 0;
				RegHdr2Sw0.bit.EOH          = 1;

				USB_SETREG(USB_DEVISOIN_HDR2SW0_REG_OFS,  RegHdr2Sw0.reg);
			}

		} else if ((RegHdr1Ctr0.bit.HDR_EN == 0) && (FIFOn >= USB_FIFO4)) {
			/*Use HDR Function 1*/

			RegHdr1Ctr0.bit.HDR_EN      = 1;
			RegHdr1Ctr0.bit.FIFO_SEL    = (FIFOn - USB_FIFO4);

			RegHdr1Ctr0.bit.HDR_START   = gUvcHdrCfg[EPi + 1].bStart;
			RegHdr1Ctr0.bit.HDR_STOP    = gUvcHdrCfg[EPi + 1].bStop;

			if (gUvcHdrCfg[EPi + 1].uiLength == 12) {
				RegHdr1Ctr0.bit.HDR_LEN     = 1;

				RegHdr1Sw0.reg              = 0;
				RegHdr1Sw0.bit.HDR_LENGTH   = 12;
				RegHdr1Sw0.bit.PTS          = 1;
				RegHdr1Sw0.bit.SCR          = 1;
				RegHdr1Sw0.bit.RES          = 0;
				RegHdr1Sw0.bit.STI          = 0;
				RegHdr1Sw0.bit.ERR          = 0;
				RegHdr1Sw0.bit.EOH          = 1;

				USB_SETREG(USB_DEVISOIN_HDR1SW0_REG_OFS,  RegHdr1Sw0.reg);
			} else {
				RegHdr1Ctr0.bit.HDR_LEN  = 0;

				RegHdr1Sw0.reg              = 0;
				RegHdr1Sw0.bit.HDR_LENGTH   = 2;
				RegHdr1Sw0.bit.PTS          = 0;
				RegHdr1Sw0.bit.SCR          = 0;
				RegHdr1Sw0.bit.RES          = 0;
				RegHdr1Sw0.bit.STI          = 0;
				RegHdr1Sw0.bit.ERR          = 0;
				RegHdr1Sw0.bit.EOH          = 1;

				USB_SETREG(USB_DEVISOIN_HDR1SW0_REG_OFS,  RegHdr1Sw0.reg);
			}
		} else {
			DBG_ERR("No Header Res\r\n");
		}
#endif

		USB_SETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS,  RegHdr1Ctr0.reg);
		USB_SETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS,  RegHdr2Ctr0.reg);
	}

}

/*
    Set the default FIFO and endpoint configuration

    This function is internal FIFO setting functions.

    @return void
*/
void usb_FIFO_EPxCfg(void)
{
	UINT32  k, EPi, FIFOn = 0, FIFO_Dir = 0;
	BOOL    bUseBiDir = FALSE;
	BOOL    bHS_En;
#if USB_FIFOMAP_FIFO4FIRST
	BOOL    ReSearch = FALSE;
#endif

	// clear all EP & FIFO setting. -1 because excluding the EP0.
	for (EPi = 0 ; EPi < (USB_EP_MAX - 1) ; EPi++) {
		gEPMap[EPi] = USB_FIFO_NOT_USE;
	}

	for (FIFOn = 0 ; FIFOn < USB_FIFO_MAX ; FIFOn++) {
		gFIFOInMap[FIFOn]  = USB_EP_NOT_USE;
		gFIFOOutMap[FIFOn] = USB_EP_NOT_USE;
	}

//#if !USB_FIFOMAP_FIFO4FIRST
//	FIFOn   = gTestFifoOffset;
//#else

#if USB_FIFOMAP_FIFO4FIRST
	FIFOn   = USB_FIFO4;
#else
	FIFOn   = USB_FIFO0;
#endif

//#endif

	bHS_En  = usb2dev_is_highspeed_enabled();


	// Clear the Header Enable before EP-Cfg.
	{
		T_USB_DEVISOIN_HDRCTRL0_REG     RegHdr1Ctr0, RegHdr2Ctr0;

		RegHdr1Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS);
		RegHdr1Ctr0.bit.HDR_EN = 0;
		USB_SETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS,  RegHdr1Ctr0.reg);

		RegHdr2Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS);
		RegHdr2Ctr0.bit.HDR_EN = 0;
		USB_SETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS,  RegHdr2Ctr0.reg);
	}

	// Clear FIFO Cfg
	USB_SETREG(USB_DEVBUFCFG0_REG_OFS, 0);
	USB_SETREG(USB_DEVBUFCFG1_REG_OFS, 0);


	// check for different g_Alternating & g_Interface if needed...,
	// set FIFO for EP1 ~ EP15 (ep_config_hs[0] ~ ep_config_hs[14])
	for (EPi = 0 ; EPi < (USB_EP_MAX - 1) ; EPi++) {
		USB_ENP_CONFIG *pEnpCfg, *pNextEnpCfg;

		pNextEnpCfg = NULL;

		if (bHS_En) {
			pEnpCfg     = &(gUSBManage.ep_config_hs[EPi]);

			for (k = EPi + 1; k < (USB_EP_MAX - 1); k++) {
				if (gUSBManage.ep_config_hs[k].enable) {
					pNextEnpCfg = &(gUSBManage.ep_config_hs[k]);
					break;
				}
			}
		} else {
			pEnpCfg     = &(gUSBManage.ep_config_fs[EPi]);

			for (k = EPi + 1; k < (USB_EP_MAX - 1); k++) {
				if (gUSBManage.ep_config_fs[k].enable) {
					pNextEnpCfg = &(gUSBManage.ep_config_fs[k]);
					break;
				}
			}
		}

		if (pEnpCfg->enable) {
#if !USB_FIFOMAP_FIFO4FIRST
			if (FIFOn >= USB_FIFO_MAX) {
				DBG_ERR("FIFO count %d exceed resource\r\n", FIFOn);
				return;
			}
#endif

			// set EP mapping
			usb_setEPMap((EPi + 1), pEnpCfg->direction, FIFOn);
			gEPMap[EPi] = FIFOn;

			// set FIFO mapping
			if (pEnpCfg->direction == EP_DIR_IN) {
				FIFO_Dir = USB_FIFO_IN;
				gFIFOInMap[FIFOn] = EPi + 1;
			} else {
				FIFO_Dir = USB_FIFO_OUT;
				gFIFOOutMap[FIFOn] = EPi + 1;
			}

			if (bUseBiDir) {
				FIFO_Dir = USB_FIFO_BI;
			}
			USB_setFIFOCfg(FIFOn, pEnpCfg->trnsfer_type, pEnpCfg->blk_num, (pEnpCfg->blk_size > 512), FIFO_Dir);

			// set EP max packet size
			usb_setEPMaxpktsize((EPi + 1), pEnpCfg->direction, pEnpCfg->max_pkt_size);

			// set high bandwidth
			if (pEnpCfg->direction == EP_DIR_IN) {
				usb_setEPHighBandWidth((EPi + 1), pEnpCfg->high_bandwidth);
			}

#if 1
			usb_configUvcHeader(EPi, FIFOn, pEnpCfg);

			// check for sharing FIFO
			// may not enough for complicated config...
			if ((gUsbUseShareFIFO)                                          &&
				(pNextEnpCfg != NULL)                                       &&
				((bUseBiDir == FALSE)                                &&
				 (pNextEnpCfg->blk_num        == pEnpCfg->blk_num)     &&
				 (pNextEnpCfg->blk_size       == pEnpCfg->blk_size)    &&
				 (pNextEnpCfg->direction     != pEnpCfg->direction)  &&
				 (pNextEnpCfg->trnsfer_type   == pEnpCfg->trnsfer_type)&&
				 (pNextEnpCfg->trnsfer_type   != EP_TYPE_ISOCHRONOUS)))

			{
				bUseBiDir = TRUE;
			} else {
				UINT32  vFIFOn;

				bUseBiDir = FALSE;

				if ((pEnpCfg->blk_num) <= BLKNUM_TRIPLE) {
					FIFOn += pEnpCfg->blk_num;

					// If max_pkt_size larger than 512. FIFO number is doubled.
					if (pEnpCfg->blk_size > 512) {
						FIFOn += pEnpCfg->blk_num;
					}
				}

				vFIFOn = FIFOn;

				if (pNextEnpCfg != NULL) {
					if ((pNextEnpCfg->blk_num) <= BLKNUM_TRIPLE) {
						vFIFOn += pNextEnpCfg->blk_num;

						// If max_pkt_size larger than 512. FIFO number is doubled.
						if (pNextEnpCfg->blk_size > 512) {
							vFIFOn += pNextEnpCfg->blk_num;
						}
					}
				}

#if USB_FIFOMAP_FIFO4FIRST
				if (ReSearch == TRUE) {
					DBG_ERR("FIFO exceed resource 2.\r\n");
					return;
				}

				if ((FIFOn <= USB_FIFO4) && (vFIFOn > USB_FIFO4)) {
					if (ReSearch == FALSE) {
						for (k = 0; k < 4; k++) {
							if ((USB_GETREG(USB_DEVBUFCFG1_REG_OFS) & (0xFF << (k << 3))) == 0x00) {
								FIFOn       = k + USB_FIFO4;
								ReSearch    = TRUE;
							}
						}
					}

					if (ReSearch == FALSE) {
						DBG_ERR("FIFO count %d exceed resource\r\n", (UINT)vFIFOn);
						return;
					}
				}

				if (vFIFOn > USB_FIFO_MAX) {
					FIFOn = USB_FIFO0;
				}

				if (FIFOn >= USB_FIFO_MAX) {
					FIFOn = (FIFOn - USB_FIFO_MAX);
				}

#else

				if ((vFIFOn > USB_FIFO4) && (FIFOn <= USB_FIFO3)) {
					FIFOn = USB_FIFO4;
				}

#endif

			}

#else

			// ISO-IN UVC Header Function
			if ((pEnpCfg->trnsfer_type == EP_TYPE_ISOCHRONOUS) && (gUvcHdrCfg[EPi + 1].bEn)) {
				T_USB_DEVISOIN_HDRCTRL0_REG     RegHdr1Ctr0, RegHdr2Ctr0;
				T_USB_DEVISOIN_HDRSW0_REG       RegHdr1Sw0, RegHdr2Sw0;

				RegHdr1Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS);
				RegHdr2Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS);

#if 1 // Use Header Function 1 First
				if ((RegHdr1Ctr0.bit.HDR_EN == 0) && (FIFOn >= USB_FIFO4)) {
					/*Use HDR Function 1*/

					RegHdr1Ctr0.bit.HDR_EN      = 1;
					RegHdr1Ctr0.bit.FIFO_SEL    = (FIFOn - USB_FIFO4);

					RegHdr1Ctr0.bit.HDR_START   = gUvcHdrCfg[EPi + 1].bStart;
					RegHdr1Ctr0.bit.HDR_STOP    = gUvcHdrCfg[EPi + 1].bStop;

					// Two Header function cfg can not set to the same fifo
					if (RegHdr2Ctr0.bit.FIFO_SEL == RegHdr1Ctr0.bit.FIFO_SEL) {
						RegHdr2Ctr0.bit.FIFO_SEL = ~RegHdr1Ctr0.bit.FIFO_SEL;
					}

					if (gUvcHdrCfg[EPi + 1].uiLength == 12) {
						RegHdr1Ctr0.bit.HDR_LEN     = 1;

						RegHdr1Sw0.reg              = 0;
						RegHdr1Sw0.bit.HDR_LENGTH   = 12;
						RegHdr1Sw0.bit.PTS          = 1;
						RegHdr1Sw0.bit.SCR          = 1;
						RegHdr1Sw0.bit.RES          = 0;
						RegHdr1Sw0.bit.STI          = 0;
						RegHdr1Sw0.bit.ERR          = 0;
						RegHdr1Sw0.bit.EOH          = 1;

						USB_SETREG(USB_DEVISOIN_HDR1SW0_REG_OFS,  RegHdr1Sw0.reg);
					} else {
						RegHdr1Ctr0.bit.HDR_LEN  = 0;

						RegHdr1Sw0.reg              = 0;
						RegHdr1Sw0.bit.HDR_LENGTH   = 2;
						RegHdr1Sw0.bit.PTS          = 0;
						RegHdr1Sw0.bit.SCR          = 0;
						RegHdr1Sw0.bit.RES          = 0;
						RegHdr1Sw0.bit.STI          = 0;
						RegHdr1Sw0.bit.ERR          = 0;
						RegHdr1Sw0.bit.EOH          = 1;

						USB_SETREG(USB_DEVISOIN_HDR1SW0_REG_OFS,  RegHdr1Sw0.reg);
					}
				} else if ((RegHdr2Ctr0.bit.HDR_EN == 0) && (FIFOn >= USB_FIFO4)) {
					/*Use HDR Function 2*/

					RegHdr2Ctr0.bit.HDR_EN      = 1;
					RegHdr2Ctr0.bit.FIFO_SEL    = (FIFOn - USB_FIFO4);

					RegHdr2Ctr0.bit.HDR_START   = gUvcHdrCfg[EPi + 1].bStart;
					RegHdr2Ctr0.bit.HDR_STOP    = gUvcHdrCfg[EPi + 1].bStop;

					if (gUvcHdrCfg[EPi + 1].uiLength == 12) {
						RegHdr2Ctr0.bit.HDR_LEN     = 1;

						RegHdr2Sw0.reg              = 0;
						RegHdr2Sw0.bit.HDR_LENGTH   = 12;
						RegHdr2Sw0.bit.PTS          = 1;
						RegHdr2Sw0.bit.SCR          = 1;
						RegHdr2Sw0.bit.RES          = 0;
						RegHdr2Sw0.bit.STI          = 0;
						RegHdr2Sw0.bit.ERR          = 0;
						RegHdr2Sw0.bit.EOH          = 1;

						USB_SETREG(USB_DEVISOIN_HDR2SW0_REG_OFS,  RegHdr2Sw0.reg);
					} else {
						RegHdr2Ctr0.bit.HDR_LEN  = 0;

						RegHdr2Sw0.reg              = 0;
						RegHdr2Sw0.bit.HDR_LENGTH   = 2;
						RegHdr2Sw0.bit.PTS          = 0;
						RegHdr2Sw0.bit.SCR          = 0;
						RegHdr2Sw0.bit.RES          = 0;
						RegHdr2Sw0.bit.STI          = 0;
						RegHdr2Sw0.bit.ERR          = 0;
						RegHdr2Sw0.bit.EOH          = 1;

						USB_SETREG(USB_DEVISOIN_HDR2SW0_REG_OFS,  RegHdr2Sw0.reg);
					}

				} else {
					DBG_ERR("No Header Res\r\n");
				}
#else // Use Header Function 2 First
				if ((RegHdr2Ctr0.bit.HDR_EN == 0) && (FIFOn >= USB_FIFO4)) {
					/*Use HDR Function 2*/

					RegHdr2Ctr0.bit.HDR_EN      = 1;
					RegHdr2Ctr0.bit.FIFO_SEL    = (FIFOn - USB_FIFO4);

					RegHdr2Ctr0.bit.HDR_START   = gUvcHdrCfg[EPi + 1].bStart;
					RegHdr2Ctr0.bit.HDR_STOP    = gUvcHdrCfg[EPi + 1].bStop;

					// Two Header function cfg can not set to the same fifo
					if (RegHdr2Ctr0.bit.FIFO_SEL == RegHdr1Ctr0.bit.FIFO_SEL) {
						RegHdr1Ctr0.bit.FIFO_SEL = ~RegHdr2Ctr0.bit.FIFO_SEL;
					}

					if (gUvcHdrCfg[EPi + 1].uiLength == 12) {
						RegHdr2Ctr0.bit.HDR_LEN     = 1;

						RegHdr2Sw0.reg              = 0;
						RegHdr2Sw0.bit.HDR_LENGTH   = 12;
						RegHdr2Sw0.bit.PTS          = 1;
						RegHdr2Sw0.bit.SCR          = 1;
						RegHdr2Sw0.bit.RES          = 0;
						RegHdr2Sw0.bit.STI          = 0;
						RegHdr2Sw0.bit.ERR          = 0;
						RegHdr2Sw0.bit.EOH          = 1;

						USB_SETREG(USB_DEVISOIN_HDR2SW0_REG_OFS,  RegHdr2Sw0.reg);
					} else {
						RegHdr2Ctr0.bit.HDR_LEN  = 0;

						RegHdr2Sw0.reg              = 0;
						RegHdr2Sw0.bit.HDR_LENGTH   = 2;
						RegHdr2Sw0.bit.PTS          = 0;
						RegHdr2Sw0.bit.SCR          = 0;
						RegHdr2Sw0.bit.RES          = 0;
						RegHdr2Sw0.bit.STI          = 0;
						RegHdr2Sw0.bit.ERR          = 0;
						RegHdr2Sw0.bit.EOH          = 1;

						USB_SETREG(USB_DEVISOIN_HDR2SW0_REG_OFS,  RegHdr2Sw0.reg);
					}

				} else if ((RegHdr1Ctr0.bit.HDR_EN == 0) && (FIFOn >= USB_FIFO4)) {
					/*Use HDR Function 1*/

					RegHdr1Ctr0.bit.HDR_EN      = 1;
					RegHdr1Ctr0.bit.FIFO_SEL    = (FIFOn - USB_FIFO4);

					RegHdr1Ctr0.bit.HDR_START   = gUvcHdrCfg[EPi + 1].bStart;
					RegHdr1Ctr0.bit.HDR_STOP    = gUvcHdrCfg[EPi + 1].bStop;

					if (gUvcHdrCfg[EPi + 1].uiLength == 12) {
						RegHdr1Ctr0.bit.HDR_LEN     = 1;

						RegHdr1Sw0.reg              = 0;
						RegHdr1Sw0.bit.HDR_LENGTH   = 12;
						RegHdr1Sw0.bit.PTS          = 1;
						RegHdr1Sw0.bit.SCR          = 1;
						RegHdr1Sw0.bit.RES          = 0;
						RegHdr1Sw0.bit.STI          = 0;
						RegHdr1Sw0.bit.ERR          = 0;
						RegHdr1Sw0.bit.EOH          = 1;

						USB_SETREG(USB_DEVISOIN_HDR1SW0_REG_OFS,  RegHdr1Sw0.reg);
					} else {
						RegHdr1Ctr0.bit.HDR_LEN  = 0;

						RegHdr1Sw0.reg              = 0;
						RegHdr1Sw0.bit.HDR_LENGTH   = 2;
						RegHdr1Sw0.bit.PTS          = 0;
						RegHdr1Sw0.bit.SCR          = 0;
						RegHdr1Sw0.bit.RES          = 0;
						RegHdr1Sw0.bit.STI          = 0;
						RegHdr1Sw0.bit.ERR          = 0;
						RegHdr1Sw0.bit.EOH          = 1;

						USB_SETREG(USB_DEVISOIN_HDR1SW0_REG_OFS,  RegHdr1Sw0.reg);
					}
				} else {
					DBG_ERR("No Header Res\r\n");
				}
#endif

				USB_SETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS,  RegHdr1Ctr0.reg);
				USB_SETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS,  RegHdr2Ctr0.reg);

			}

			// check for sharing FIFO
			// may not enough for complicated config...
			if ((gUsbUseShareFIFO)                                 &&
				(pNextEnpCfg->Enable)                              &&
				(bUseBiDir == FALSE)                               &&
				(pNextEnpCfg->blk_num        == pEnpCfg->blk_num)    &&
				(pNextEnpCfg->blk_size       == pEnpCfg->blk_size)   &&
				(pNextEnpCfg->Direction     != pEnpCfg->Direction) &&
				(pNextEnpCfg->trnsfer_type   == pEnpCfg->trnsfer_type)) {
				bUseBiDir = TRUE;
			} else {
				bUseBiDir = FALSE;

				if ((pEnpCfg->blk_num) <= BLKNUM_TRIPLE) {
					FIFOn += pEnpCfg->blk_num;

					// If max_pkt_size larger than 512. FIFO number is doubled.
					if (pEnpCfg->blk_size > 512) {
						FIFOn += pEnpCfg->blk_num;
					}
				}
			}
#endif
		}
	} // end of for loop

}

/*
    Get EP is IN or OUT

*/
USB_EP_DIR usb_getEpDirection(USB_EP EPn)
{
	T_USB_DEVMAXPS_EP_REG   devEPMaxPS;

	devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((EPn - USB_EP1) << 2));
	return devEPMaxPS.bit.DIR_EP + 1;
}

/*
    Get IN EP from FIFO

    This function is used to get EP mapped by input FIFO

    @param[in] FIFOn

    @return EP number
*/
USB_EP usb_getInEpFromFifo(USB_FIFO_NUM FIFOn)
{
	return gFIFOInMap[FIFOn];
}

/*
    Get OUT EP from FIFO

    This function is used to get EP mapped by output FIFO

    @param[in] FIFOn

    @return EP number
*/
USB_EP usb_getOutEpFromFifo(USB_FIFO_NUM FIFOn)
{
	return gFIFOOutMap[FIFOn];
}

/*
    Get FIFO from EP

    This function is used to get FIFO mapped by input EP

    @param[in] EPn

    @return FIFO number
*/
USB_FIFO_NUM usb_getFifoFromEp(USB_EP EPn)
{
	return gEPMap[EPn - 1];
}

/*
    Mask(Disable) FIFO IN interrupt

    @param[in] FIFOn    FIFO number
    @return void
*/
static void usb_maskFIFOININT(USB_FIFO_NUM FIFOn)
{
	T_USB_DEVINTMASKGROUP1_REG intGroup1Mask;
	unsigned long      flags;

	if (FIFOn >= USB_FIFO_MAX) {
		return;
	}

	loc_cpu(flags);
	intGroup1Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
	intGroup1Mask.reg |= (0x1 << (16 + FIFOn));
	USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, intGroup1Mask.reg);
	unl_cpu(flags);
}

/*
    Unmask(Enable) FIFO IN interrupt

    @param[in] FIFOn    FIFO number
    @return void
*/
static void usb_unmaskFIFOININT(USB_FIFO_NUM FIFOn)
{
	T_USB_DEVINTMASKGROUP1_REG intGroup1Mask;
	unsigned long      flags;

	if (FIFOn >= USB_FIFO_MAX) {
		return;
	}

	loc_cpu(flags);
	intGroup1Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
	intGroup1Mask.reg &= ~(0x1 << (16 + FIFOn));
	USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, intGroup1Mask.reg);
	unl_cpu(flags);
}

/*
    Mask(Disable) FIFO OUT interrupt

    @param[in] FIFOn    FIFO number
    @return void
*/
static void usb_maskFIFOOUTINT(USB_FIFO_NUM FIFOn)
{
	T_USB_DEVINTMASKGROUP1_REG intGroup1Mask;
	unsigned long      flags;

	if (FIFOn >= USB_FIFO_MAX) {
		return;
	}

	loc_cpu(flags);
	intGroup1Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
	intGroup1Mask.reg |= (0x3 << (FIFOn << 1));
	USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, intGroup1Mask.reg);
	unl_cpu(flags);
}

/*
    Unmask(Enable) FIFO OUT interrupt

    @param[in] FIFOn    FIFO number
    @return void
*/
static void usb_unmaskFIFOOUTINT(USB_FIFO_NUM FIFOn)
{
	T_USB_DEVINTMASKGROUP1_REG intGroup1Mask;
	unsigned long      flags;

	if (FIFOn >= USB_FIFO_MAX) {
		return;
	}

	loc_cpu(flags);
	intGroup1Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
	intGroup1Mask.reg &= ~(0x3 << (FIFOn << 1));
	USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, intGroup1Mask.reg);
	unl_cpu(flags);
}

/*
    Set DMA target FIFO

    This function is internal DMA target FIFO setting functions

    @param[in] EPn      endpoint number

    @return
        - @b E_OK: success
        - @b Else: fail
*/
static ER usb_setDmaTargetFifo(USB_EP EPn)
{
	USB_FIFO_NUM            fifoNum;
	T_USB_DEVACCFIFO_REG    devAccFifo = {0};

	if ((EPn > USB_EP0) && (EPn < USB_EP_MAX)) {
		fifoNum = usb_getFifoFromEp(EPn);

		devAccFifo.bit.ACC_F0_3 = fifoNum;
		USB_SETREG(USB_DEVACCFIFO_REG_OFS, devAccFifo.reg);
	}
	return E_OK;
}

/*
    If need not clear fifo. EP number cam set to USB_EP_NOT_USE
*/
void usb_abortDMA(USB_EP EPn, USB_DMACH DmaCH)
{
	T_USB_DEVDMACTRL0_REG       devDMACtl0;
	T_USB_DEVDMA2CTRL0_REG      devDMAxCtl;
	T_USB_DEVINTGROUP2_REG      devIntGrp2;
	T_USB_DEVINTMASKGROUP2_REG  devIntMskGrp2;
	T_USB_DEVMAINCTRL_REG       devMainCtl;
	T_USB_DEVDMACTRL1_REG       devDMACtl1;
	UINT32                      loop_cnt = 0;
	FLGPTN                      uiFlag = 0;
	BOOL                        bIntDis;
	UINT32                      i, uiRegOfs = 0;

	if (DmaCH & USB_DMACH_CH1) {
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		if (devDMACtl1.bit.DMA_START == 1) {
			// mask DMA_CMPLT interrupt
			devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
			bIntDis = devIntMskGrp2.bit.MDMA_CMPLT;
			if (!bIntDis) {
				devIntMskGrp2.bit.MDMA_CMPLT = 1;
				USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);
			}

			// set DMA_ABORT & CLRFIFO
			devDMACtl0.reg = USB_GETREG(USB_DEVDMACTRL0_REG_OFS);
			devDMACtl0.bit.DMA_ABORT         = 1;
			devDMACtl0.bit.CLRFIFO_DMA_ABORT = 1;
			USB_SETREG(USB_DEVDMACTRL0_REG_OFS, devDMACtl0.reg);

			#if _EMULATION_
			DBG_WRN("ABORT DMA-1\r\n");
			#endif

			// wait DMA_CMPLT and clear it
			do {
				devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);

				loop_cnt++;
				//  check if DMA abort fail...
				if (loop_cnt > 0x20) {
					USB_PutString("\r\nDMA-1 abort fail !!!\r\n");

					// force DMA abort if fail happen...
					devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
					devMainCtl.bit.FORCE_DMA_ABORT = 1;
					USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

					do {
						devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);
						DBG_WRN(" @ ");

					} while (devIntGrp2.bit.DMA_CMPLT == 0);

					loop_cnt = 0;
					break;
				}
			} while (devIntGrp2.bit.DMA_CMPLT == 0);

			devIntGrp2.bit.DMA_CMPLT = 1;
			USB_SETREG(USB_DEVINTGROUP2_REG_OFS, devIntGrp2.reg);

			// disable CLRFIFO
			devDMACtl0.bit.CLRFIFO_DMA_ABORT = 0;
			USB_SETREG(USB_DEVDMACTRL0_REG_OFS, devDMACtl0.reg);

			// unmask DMA_CMPLT interrupt
			devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
			if (!bIntDis) {
				devIntMskGrp2.bit.MDMA_CMPLT = 0;
				USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);
			}

			USB_SETREG(USB_DEVACCFIFO_REG_OFS, 0);
		}

		uiFlag |= FLGUSB_DMADONE;
	}


	// DMA CH 2/3/4/5; Total 4 CH.
	for (i = 0; i < 4; i++) {
		// REG Offset for DMA 2/3/4/5
		uiRegOfs = i << 3;
		loop_cnt = 0;

		if ((USB_DMACH_CH2 << i) & DmaCH) {
			devDMAxCtl.reg = USB_GETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs);
			if (devDMAxCtl.bit.DMA_START == 1) {
				// mask DMA_CMPLT interrupt
				devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
				bIntDis = (devIntMskGrp2.reg >> (9 + i)) & 0x1;
				if (!bIntDis) {
					devIntMskGrp2.reg |= (0x1 << (9 + i));
					USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);
				}

				// set DMA_ABORT & CLRFIFO
				devDMAxCtl.bit.DMA_ABORT   = 1;
				devDMAxCtl.bit.DMA_CLRFIFO = 1;
				devDMAxCtl.bit.DMA_START   = 0;
				USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);
				devDMAxCtl.bit.DMA_ABORT   = 0;

				#if _EMULATION_
				DBG_WRN("ABORT DMA-%d\r\n", i + 2);
				#endif

				// wait DMA_CMPLT and clear it
				do {
					devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);

					loop_cnt++;
					if (loop_cnt > 0x1800000) {
						DBG_ERR("DMA-%d Abort Fail!!\r\n", (UINT)i + 2);
						break;
					}
				} while ((devIntGrp2.reg & (0x1 << (9 + i))) == 0);


				// Clear Int Sts
				devIntGrp2.reg |= (0x1 << (9 + i));
				USB_SETREG(USB_DEVINTGROUP2_REG_OFS, devIntGrp2.reg);

				// disable CLRFIFO
				devDMAxCtl.bit.DMA_CLRFIFO = 0;
				USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);

				// unmask DMA_CMPLT interrupt
				devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
				if (!bIntDis) {
					devIntMskGrp2.reg &= ~(0x1 << (9 + i));
					USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);
				}

			}

			uiFlag |= (FLGUSB_DMA2DONE << i);

		}
	}

	// Clear EP FIFOs
	if (EPn < USB_EP_MAX) {
		usb2dev_clear_ep_fifo(EPn);
	}

	if (uiFlag) {
		bAbortDMA = TRUE;
		set_flg(FLG_ID_USBTSK, uiFlag);
	}

}

#if 1
/**
    @name USB read /write functions
*/
//@{


/**
    Return setup data to host.

    Class library can use this API to return CX IN data after receive a setup packet.

    @note Before invoke this API, usb2dev_control_data.p_data should point to return data\n
            and usb2dev_control_data.w_length should be filled with length of return data.

    @return void
*/
void usb2dev_reture_setup_data(void)
{
	UINT32  RetBytes;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		usb2dev528_reture_setup_data();
		return;
	}

	if (usb2dev_control_data.w_length < EP0_PACKET_SIZE) {
		RetBytes = usb2dev_control_data.w_length;  // this is the last packet to return host....
		usb2dev_control_data.w_length = 0;
	} else {
		T_USB_DEVINTMASKGROUP0_REG    devIntMskGrp0;

		RetBytes = EP0_PACKET_SIZE;
		usb2dev_control_data.w_length -= EP0_PACKET_SIZE;

		// CX_IN interrupt is default DISABLED
		// Only enable if still has more data to be transfered to host
		devIntMskGrp0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
		devIntMskGrp0.bit.MCX_IN_INT = 0;
		USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, devIntMskGrp0.reg);
	}

	usb2dev_write_endpoint(USB_EP0, usb2dev_control_data.p_data, &RetBytes);

	usb2dev_control_data.p_data += RetBytes;

	// only set CX_DONE at the last cx transfer
	if (usb2dev_control_data.w_length == 0) {
		gUSBCxFinishAction = USB_ACT_DONE;
	} else {
		gUSBCxFinishAction = USB_ACT_IDLE;
	}

}


/**
    Read the buffer content from FIFO

    This function is blocking mode. It will return until DMA done or
    some error occured. Check the return ER and pDMALen for actual
    transfered length.

    @param[in] EPn          endpoint number (USB_EP0~USB_EP15)
    @param[in] pBuffer      buffer pointer
    @param[in,out] pDMALen  input length to read, output actual transfered length

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_PAR: input DMA length or EP number incorrect
        - @b E_OBJ: FIFO is busy
        - @b E_SYS: USB is SUSPENDED
        - @b E_OK: read done (maybe aborted, check return pDMALen)
*/
ER usb2dev_read_endpoint(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen)
{
	ER erReturn;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		return usb2dev528_read_endpoint(EPn, pBuffer, pDMALen);
	}

	erReturn = usb2dev_set_ep_read(EPn, pBuffer, pDMALen);
	if (erReturn != E_OK) {
		return erReturn;
	}

	erReturn = usb2dev_wait_ep_done(EPn, pDMALen);

	if(EPn != USB_EP0) {
		dma_flushReadCache((UINT32)pBuffer, *pDMALen);
	}

	return erReturn;
}

ER usb2dev_read_endpoint_timeout(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen, UINT32 timeout_ms)
{
	ER erReturn;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		return usb2dev528_read_endpoint_timeout(EPn, pBuffer, pDMALen, timeout_ms);
	}

	erReturn = usb2dev_set_ep_read(EPn, pBuffer, pDMALen);
	if (erReturn != E_OK) {
		return erReturn;
	}

	erReturn = usb2dev_wait_ep_done_timeout(EPn, pDMALen, timeout_ms);

	if(EPn != USB_EP0) {
		dma_flushReadCache((UINT32)pBuffer, *pDMALen);
	}

	return erReturn;
}

/**
    Write the buffer content to FIFO

    This function is blocking mode. It will return until DMA done or
    some error occured. Check the return ER and pDMALen for actual
    transfered length.

    @param[in] EPn          endpoint number (USB_EP0~USB_EP15)
    @param[in] pBuffer      buffer pointer
    @param[in,out] pDMALen  input length to write, output actual transfered length

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_PAR: input DMA length or EP number incorrect
        - @b E_OBJ: FIFO is busy
        - @b E_SYS: USB is SUSPENDED
        - @b E_OK: read done (maybe aborted, check return pDMALen)
*/
ER usb2dev_write_endpoint(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen)
{
	ER erReturn;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		return usb2dev528_write_endpoint(EPn, pBuffer, pDMALen);
	}

	erReturn = usb2dev_set_ep_write(EPn, pBuffer, pDMALen);
	if (erReturn != E_OK) {
		return erReturn;
	}

	return usb2dev_wait_ep_done(EPn, pDMALen);
}

ER usb2dev_write_endpoint_timeout(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen, UINT32 timeout_ms)
{
	ER erReturn;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		return usb2dev528_write_endpoint_timeout(EPn, pBuffer, pDMALen, timeout_ms);
	}

	erReturn = usb2dev_set_ep_write(EPn, pBuffer, pDMALen);
	if (erReturn != E_OK) {
		return erReturn;
	}

	return usb2dev_wait_ep_done_timeout(EPn, pDMALen, timeout_ms);
}

/**
    Abort the endpoint transfer

    @param[in] EPn  endpoint number (1~15)

    @return
        - @b E_OK: abort success
        - @b Else: abort fail
*/
ER usb2dev_abort_endpoint(USB_EP EPn)
{
	USB_FIFO_NUM                fifonum;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		return usb2dev528_abort_endpoint(EPn);
	}

	if (!gUSBOpen) {
		DBG_ERR("driver not opened\r\n");
		return E_OACV;
	}

	if (EPn == USB_EP0) {
		return E_PAR;
	}

	fifonum = usb_getFifoFromEp(EPn);

	if (fifonum < USB_FIFO4) {
		usb_abortDMA(EPn, USB_DMACH_CH1);
	} else if (fifonum < USB_FIFO_MAX) {
		usb_abortDMA(EPn, (USB_DMACH_CH2 << (fifonum - USB_FIFO4)));
	}

	return E_OK;
}

/**
    Read the buffer content from FIFO

    This function is non-blocking mode and will return immediately.
    Used with usb2dev_wait_ep_done() to wait for DMA complete.

    @param[in] EPn      endpoint number (USB_EP0~USB_EP15)
    @param[in] pBuffer  buffer pointer
    @param[in] pDMALen  DMA length to be read.
                        Valid length 0x1 to 0x40 for EP0.
                        Valid length 0x1 to 0x7fffff for EP1~EP15.

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_PAR: input DMA length or EP number incorrect
        - @b E_OBJ: FIFO is busy
        - @b E_SYS: USB is SUSPENDED
        - @b E_OK: start to transfer
*/
ER usb2dev_set_ep_read(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen)
{
	T_USB_DEVDMACTRL1_REG         devDMACtl1;
	T_USB_DEVDMACTRL2_REG         devDMACtl2;
	T_USB_DEVDMA2CTRL0_REG        devDMAxCtl;
	T_USB_DEVDMA2CTRL1_REG        devDMAxCtl2;
	T_USB_DEVCXCFGFIFOSTATUS_REG  devCXCFG;
	USB_FIFO_NUM                  fifonum;
	INT32                         i, Ep0DMASize, Ep0RxSize;
	UINT32                        *pBUF, uiRegOfs;
	unsigned long      flags;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		return usb2dev528_set_ep_read(EPn, pBuffer, pDMALen);
	}

	if (!gUSBOpen) {
		DBG_ERR("driver is not opened !\r\n");
		return E_OACV;
	}

	// We must avoid 0Byte DMAtrigger. This may cause mal-functioned in MSDC application.
	if ((*pDMALen == 0) || (*pDMALen > USB_MAX_DMA_LENGTH)) {
		return E_PAR;
	}

	// check if under SUSPEND state
	if (usb2dev_get_controller_state() == USB_CONTROLLER_STATE_SUSPEND) {
		// In SUSPEND state, UCLK is stopped and USB controller will not do jobs of DMA
		// Block DMA request when usb is SUSPENDED
		DBG_ERR("usb under SUSPENDED\r\n");
		return E_SYS;
	}

	if (EPn == USB_EP0) {
		devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);

		Ep0DMASize    = *pDMALen;
		Ep0RxSize = devCXCFG.bit.CX_FNT_OUT;

		if (Ep0DMASize != Ep0RxSize) {
			DBG_WRN("CX OUT Got (%d)B. DMA assign size(%d)\r\n", (UINT)Ep0RxSize, (UINT)Ep0DMASize);
		}

		// Make sure fifo got something to start read
		while (!Ep0RxSize) {
			USB_DELAY_US(500);
			devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
			Ep0RxSize = devCXCFG.bit.CX_FNT_OUT;
		}

		// Enter critical section
		loc_cpu(flags);

		// Enable DATAPORT
		devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		devCXCFG.bit.CX_DATAPORT_EN = 1;
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);

		pBUF        = (UINT32 *)pBuffer;
		i           = 0;

		while (Ep0RxSize > 0) {
			UINT32 temp;

			temp = USB_GETREG(USB_DEVCXDATAPORT_REG_OFS);
			Ep0RxSize -= 4;

			if (Ep0DMASize >= 4) {
				pBUF[i++] = temp;
				Ep0DMASize -= 4;
			} else if (Ep0DMASize == 1) {
				pBUF[i] &= ~0xFF;
				temp    &=  0xFF;
				pBUF[i] += temp;
				Ep0DMASize = 0;
			} else if (Ep0DMASize == 2) {
				pBUF[i] &= ~0xFFFF;
				temp    &=  0xFFFF;
				pBUF[i] += temp;
				Ep0DMASize = 0;
			} else if (Ep0DMASize == 3) {
				pBUF[i] &= ~0xFFFFFF;
				temp    &=  0xFFFFFF;
				pBUF[i] += temp;
				Ep0DMASize = 0;
			}

		};

		devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		devCXCFG.bit.CX_DATAPORT_EN = 0;
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);

		// Exit critical section
		unl_cpu(flags);

		return E_OK;
	}

	fifonum = usb_getFifoFromEp(EPn);

	if (fifonum < USB_FIFO4) {
		// check if DMA being acccessed
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		if (devDMACtl1.bit.DMA_START) {
			DBG_ERR("EP%d: DMA-1 not available 0x%lx\r\n", EPn, USB_GETREG(USB_DEVACCFIFO_REG_OFS));
			return E_OBJ;
		}

		clr_flg(FLG_ID_USBTSK, FLGUSB_DMADONE);

		// Enter critical section
		loc_cpu(flags);

		// Config DMA direction & length
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_TYPE = 0; // FIFO to memory
		devDMACtl1.bit.DMA_LEN  = *pDMALen;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

		usb_setDmaTargetFifo(EPn);

		// set Dma Addr
		devDMACtl2.reg = dma_getPhyAddr((UINT32)pBuffer);
		USB_SETREG(USB_DEVDMACTRL2_REG_OFS, devDMACtl2.reg);

		//dma_flushWriteCache((UINT32)pBuffer, *pDMALen);
		dma_flushReadCache((UINT32)pBuffer, *pDMALen);

		bAbortDMA = FALSE;

		#if _FPGA_EMULATION_
		if (EPn == USB_EP6) {
			usb2dev_clear_ep_fifo(USB_EP6);
		}
		#endif

		// start Dma
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_START = 1;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

		// Exit critical section
		unl_cpu(flags);
	} else if (fifonum < USB_FIFO_MAX) {
		uiRegOfs = (fifonum - USB_FIFO4) << 3;

		if (((UINT32) pBuffer) & 0x3) {
			DBG_ERR("EP%d: Addr must be word aligned 0x%08X\r\n", EPn, ((UINT) pBuffer));
			return E_OBJ;
		}

		devDMAxCtl.reg = USB_GETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs);
		if (devDMAxCtl.bit.DMA_START == 1) {
			DBG_ERR("EP%d: DMA-%d not available\r\n", EPn, (fifonum - USB_FIFO2));
			return E_OBJ;
		}

		clr_flg(FLG_ID_USBTSK, (FLGUSB_DMA2DONE << (fifonum - USB_FIFO4)));

		// Enter critical section
		loc_cpu(flags);

		devDMAxCtl.bit.DMA_TYPE = 0;// FIFO to memory
		devDMAxCtl.bit.DMA_LEN  = *pDMALen;
		USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);

		// set Dma Addr
		devDMAxCtl2.reg = dma_getPhyAddr((UINT32)pBuffer);
		USB_SETREG(USB_DEVDMA2CTRL1_REG_OFS + uiRegOfs, devDMAxCtl2.reg);

		//dma_flushWriteCache((UINT32)pBuffer, *pDMALen);
		dma_flushReadCache((UINT32)pBuffer, *pDMALen);

		bAbortDMA = FALSE;

		// start Dma
		devDMAxCtl.bit.DMA_START = 1;
		USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);

		// Exit critical section
		unl_cpu(flags);

	}

	return E_OK;
}


/**
    Write the buffer content to FIFO

    This function is non-blocking mode and will return immediately.
    Used with usb2dev_wait_ep_done() to wait for DMA complete.

    @param[in] EPn      endpoint number (USB_EP0~USB_EP15)
    @param[in] pBuffer  buffer pointer
    @param[in] pDMALen  DMA length to be written.
                        Valid length 0x1 to 0x40 for EP0.
                        Valid length 0x1 to 0x7fffff for EP1~EP15.

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_PAR: input DMA length or EP number incorrect
        - @b E_OBJ: FIFO is busy
        - @b E_SYS: USB is SUSPENDED
        - @b E_OK: start to transfer
*/
ER usb2dev_set_ep_write(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen)
{
	T_USB_DEVDMACTRL1_REG         devDMACtl1;
	T_USB_DEVDMACTRL2_REG         devDMACtl2;
	T_USB_DEVDMA2CTRL0_REG        devDMAxCtl;
	T_USB_DEVDMA2CTRL1_REG        devDMAxCtl2;
	T_USB_DEVCXCFGFIFOSTATUS_REG  devCXCFG;
	USB_FIFO_NUM                  fifonum;
	INT32                         i, Ep0DMASize;
	UINT32                        *pBUF, uiRegOfs;
	unsigned long      flags;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		return usb2dev528_set_ep_write(EPn, pBuffer, pDMALen);
	}

	if (!gUSBOpen) {
		DBG_ERR("driver not opened\r\n");
		return E_OACV;
	}

	// We must avoid 0Byte DMAtrigger. This may cause mal-functioned in MSDC application.
	if ((*pDMALen == 0) || (*pDMALen > USB_MAX_DMA_LENGTH)) {
		return E_PAR;
	}

	// check if under SUSPEND state
	if (usb2dev_get_controller_state() == USB_CONTROLLER_STATE_SUSPEND) {
		// In SUSPEND state, UCLK is stopped and USB controller will not do jobs of DMA
		// Block DMA request when usb is SUSPENDED
		DBG_ERR("usb under SUSPENDED\r\n");
		return E_SYS;
	}

	if (EPn == USB_EP0) {
		Ep0DMASize    = *pDMALen;

		if (Ep0DMASize > EP0_PACKET_SIZE) {
			DBG_WRN("CX IN Size should not larger than 64B. (%d)\r\n", (UINT)Ep0DMASize);
			Ep0DMASize = EP0_PACKET_SIZE;
		}

		// Update the actual Tx Length
		*pDMALen = Ep0DMASize;

		// Enter critical section
		loc_cpu(flags);

		// Enable DATAPORT
		devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		devCXCFG.bit.CX_FNT_IN      = Ep0DMASize;
		devCXCFG.bit.CX_DATAPORT_EN = 1;
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);

		pBUF        = (UINT32 *)pBuffer;
		i           = 0;

		while (Ep0DMASize > 0) {
			USB_SETREG(USB_DEVCXDATAPORT_REG_OFS, pBUF[i++]);
			Ep0DMASize -= 4;
		};

		devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		devCXCFG.bit.CX_DATAPORT_EN = 0;
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);

		// Exit critical section
		unl_cpu(flags);

		return E_OK;
	}

	fifonum = usb_getFifoFromEp(EPn);

	if (fifonum < USB_FIFO4) {
		// check if DMA being acccessed
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		if (devDMACtl1.bit.DMA_START) {
			DBG_ERR("EP%d: DMA-1 not available 0x%lx\r\n", EPn, USB_GETREG(USB_DEVACCFIFO_REG_OFS));
			return E_OBJ;
		}

		// 660 ISO-IN: 512x3  Size 1536+1 and 1536+2 continuously trigger.
		// For F0-F3, pre-trigger DMA is not allowed.
		// We must wait FIFo empty and then trigger next dma.
		while (usb2dev_check_ep_busy(EPn)) {
			USB_DELAY_US(50);
		}

		clr_flg(FLG_ID_USBTSK, FLGUSB_DMADONE);

		// Enter critical section
		loc_cpu(flags);

		// Config DMA direction & length
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_TYPE = 1; // memory to FIFO
		devDMACtl1.bit.DMA_LEN  = *pDMALen;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

		usb_setDmaTargetFifo(EPn);

		// set Dma Addr
		devDMACtl2.reg = dma_getPhyAddr((UINT32)pBuffer);
		USB_SETREG(USB_DEVDMACTRL2_REG_OFS, devDMACtl2.reg);

		dma_flushWriteCache((UINT32)pBuffer, *pDMALen);

		bAbortDMA = FALSE;

		// start Dma
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_START = 1;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

		// Exit critical section
		unl_cpu(flags);
	} else if (fifonum < USB_FIFO_MAX) {
		uiRegOfs = (fifonum - USB_FIFO4) << 3;

		if (((UINT32) pBuffer) & 0x3) {
			DBG_ERR("EP%d: Addr must be word aligned 0x%08X\r\n", EPn, ((UINT) pBuffer));
			return E_OBJ;
		}

		devDMAxCtl.reg = USB_GETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs);
		if (devDMAxCtl.bit.DMA_START == 1) {
			DBG_ERR("EP%d: DMA-%d not available\r\n", EPn, (fifonum - USB_FIFO2));
			return E_OBJ;
		}

		while (usb2dev_check_ep_busy(EPn)) {
			USB_DELAY_US(50);
		}

		clr_flg(FLG_ID_USBTSK, (FLGUSB_DMA2DONE << (fifonum - USB_FIFO4)));

		// Enter critical section
		loc_cpu(flags);

		devDMAxCtl.bit.DMA_TYPE = 1;// memory to FIFO
		devDMAxCtl.bit.DMA_LEN  = *pDMALen;
		USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);

		// set Dma Addr
		devDMAxCtl2.reg = dma_getPhyAddr((UINT32)pBuffer);
		USB_SETREG(USB_DEVDMA2CTRL1_REG_OFS + uiRegOfs, devDMAxCtl2.reg);

		dma_flushWriteCache((UINT32)pBuffer, *pDMALen);

		bAbortDMA = FALSE;

		// start Dma
		devDMAxCtl.bit.DMA_START = 1;
		USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);

		// Exit critical section
		unl_cpu(flags);

	}

	return E_OK;
}


/**
    Wait for endpoint DMA done

    This function is used with usb2dev_set_ep_read() or usb2dev_set_ep_write().
    It will Wait until DMA done and return actual transfered length

    @param[in] EPn          endpoint number (USB_EP0~USB_EP8)
    @param[in,out] pDMALen  input DMA length (1~0x7fffff), output actual transfered length

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_OK: DMA done (maybe aborted, check return pDMALen)
*/
ER usb2dev_wait_ep_done(USB_EP EPn, UINT32 *pDMALen)
{
	T_USB_DEVMAINCTRL_REG       devMainCtl;
	USB_FIFO_NUM                fifonum;
	FLGPTN                      uiFlag;
	UINT32                      uiChOfs;
	BOOL                        bLocalAbort;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		return usb2dev528_wait_ep_done(EPn, pDMALen);
	}

	if (!gUSBOpen) {
		DBG_ERR("driver not opened\r\n");
		return E_OACV;
	}

	if (EPn == USB_EP0) {
		return E_OK;
	}

	fifonum = usb_getFifoFromEp(EPn);

	if (fifonum < USB_FIFO4) {
		// wait for DMA done
		wai_flg(&uiFlag, FLG_ID_USBTSK, FLGUSB_DMADONE, TWF_CLR | TWF_ORW);

		if (bAbortDMA) {
			USB_PutString1("\r\n DMA abort : EP %d, ", EPn);
			USB_PutString1("  total 0x%x , ", *pDMALen);

			*pDMALen -= USB_GETREG(USB_DEVDMACTRLSTATUS_REG_OFS);

			USB_PutString1("  received 0x%x \r\n", *pDMALen);

			if (gFIFOInMap[gEPMap[EPn - 1]] == EPn) {
				// This is a IN EP
				usb2dev_set_tx0byte(EPn);
			}
		}

		// check if DMA_ERROR/Resume/Suspend/Reset
		if (gUSBCauseGroup[2] & (BIT8 | BIT2 | BIT1 | BIT0)) {
			USB_PutString(" Abort DMA transfer.. \r\n");

			// total - remain = actual transfer size
			*pDMALen -= USB_GETREG(USB_DEVDMACTRLSTATUS_REG_OFS);

			// DMA Reset
			devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
			devMainCtl.bit.DMA_RST = 1; // bit[8]
			USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

			// clear FIFO
			usb2dev_clear_ep_fifo(EPn);
		}

		{
			T_USB_TOP_REG reg_top;

			reg_top.reg = USB_GETREG(USB_TOP_REG_OFS);
			while (!(reg_top.bit.AHBC_IDLE)) {
				USB_DELAY_US(100);
				reg_top.reg = USB_GETREG(USB_TOP_REG_OFS);
				#if _EMULATION_
				DBG_DUMP("?");
				#endif
			}
		}


		// signal USB DMA channel is ready again
		set_flg(FLG_ID_USB, FLGPTN_USB_DMA_RDY);
	} else if (fifonum < USB_FIFO_MAX) {
		uiChOfs = (fifonum - USB_FIFO4);

		// wait for DMA done
		wai_flg(&uiFlag, FLG_ID_USBTSK, (FLGUSB_DMA2DONE << uiChOfs), TWF_CLR | TWF_ORW);

		if (bAbortDMA) {
			USB_PutString1("\r\n DMA abort : EP %d, ", EPn);
			USB_PutString1("  total 0x%x , ", *pDMALen);

			*pDMALen -= USB_GETREG(USB_DEVDMA2CTRLSTATUS_REG_OFS + (uiChOfs << 2));

			USB_PutString1("  received 0x%x \r\n", *pDMALen);

			if (gFIFOInMap[gEPMap[EPn - 1]] == EPn) {
				// This is a IN EP
				usb2dev_set_tx0byte(EPn);
			}
		}

		// check if DMA_ERROR/Resume/Suspend/Reset
		if (gUSBCauseGroup[2] & (BIT2 | BIT1 | BIT0)) {
			USB_PutString(" Abort DMA transfer.. \r\n");

			// total - remain = actual transfer size
			*pDMALen -= USB_GETREG(USB_DEVDMA2CTRLSTATUS_REG_OFS + (uiChOfs << 2));

			bLocalAbort = bAbortDMA;

			// DMA Abort
			usb_abortDMA(EPn, USB_DMACH_CH2 << uiChOfs);

			// We just want to abort DMA only.
			if (bLocalAbort == FALSE) {
				bAbortDMA = FALSE;
				clr_flg(FLG_ID_USBTSK, (FLGUSB_DMA2DONE << uiChOfs));
			}
		}

		// signal USB DMA channel is ready again
		set_flg(FLG_ID_USB, FLGPTN_USB_DMA_RDY);
	}

	return E_OK;
}

ER usb2dev_wait_ep_done_timeout(USB_EP EPn, UINT32 *pDMALen, UINT32 timeout_ms)
{
	T_USB_DEVMAINCTRL_REG       devMainCtl;
	USB_FIFO_NUM                fifonum;
	FLGPTN                      uiFlag;
	UINT32                      uiChOfs;
	BOOL                        bLocalAbort;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		return usb2dev528_wait_ep_done(EPn, pDMALen);
	}

	if (!gUSBOpen) {
		DBG_ERR("driver not opened\r\n");
		return E_OACV;
	}

	if (EPn == USB_EP0) {
		return E_OK;
	}

	fifonum = usb_getFifoFromEp(EPn);

	if (fifonum < USB_FIFO4) {
		// wait for DMA done
		vos_flag_wait_timeout(&uiFlag, FLG_ID_USBTSK, FLGUSB_DMADONE, TWF_CLR | TWF_ORW, vos_util_msec_to_tick(timeout_ms));

		if (uiFlag == 0) {
			//timeout
			*pDMALen = 0;
			return E_TMOUT;
		}

		if (bAbortDMA) {
			USB_PutString1("\r\n DMA abort : EP %d, ", EPn);
			USB_PutString1("  total 0x%x , ", *pDMALen);

			*pDMALen -= USB_GETREG(USB_DEVDMACTRLSTATUS_REG_OFS);

			USB_PutString1("  received 0x%x \r\n", *pDMALen);

			if (gFIFOInMap[gEPMap[EPn - 1]] == EPn) {
				// This is a IN EP
				usb2dev_set_tx0byte(EPn);
			}
		}

		// check if DMA_ERROR/Resume/Suspend/Reset
		if (gUSBCauseGroup[2] & (BIT8 | BIT2 | BIT1 | BIT0)) {
			USB_PutString(" Abort DMA transfer.. \r\n");

			// total - remain = actual transfer size
			*pDMALen -= USB_GETREG(USB_DEVDMACTRLSTATUS_REG_OFS);

			// DMA Reset
			devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
			devMainCtl.bit.DMA_RST = 1; // bit[8]
			USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

			// clear FIFO
			usb2dev_clear_ep_fifo(EPn);
		}

		{
			T_USB_TOP_REG reg_top;

			reg_top.reg = USB_GETREG(USB_TOP_REG_OFS);
			while (!(reg_top.bit.AHBC_IDLE)) {
				USB_DELAY_US(100);
				reg_top.reg = USB_GETREG(USB_TOP_REG_OFS);
				#if _EMULATION_
				DBG_DUMP("?");
				#endif
			}
		}


		// signal USB DMA channel is ready again
		set_flg(FLG_ID_USB, FLGPTN_USB_DMA_RDY);
	} else if (fifonum < USB_FIFO_MAX) {
		uiChOfs = (fifonum - USB_FIFO4);

		// wait for DMA done
		vos_flag_wait_timeout(&uiFlag, FLG_ID_USBTSK, (FLGUSB_DMA2DONE << uiChOfs), TWF_CLR | TWF_ORW, vos_util_msec_to_tick(timeout_ms));

		if (uiFlag == 0) {
			//timeout
			*pDMALen = 0;
			return E_TMOUT;
		}

		if (bAbortDMA) {
			USB_PutString1("\r\n DMA abort : EP %d, ", EPn);
			USB_PutString1("  total 0x%x , ", *pDMALen);

			*pDMALen -= USB_GETREG(USB_DEVDMA2CTRLSTATUS_REG_OFS + (uiChOfs << 2));

			USB_PutString1("  received 0x%x \r\n", *pDMALen);

			if (gFIFOInMap[gEPMap[EPn - 1]] == EPn) {
				// This is a IN EP
				usb2dev_set_tx0byte(EPn);
			}
		}

		// check if DMA_ERROR/Resume/Suspend/Reset
		if (gUSBCauseGroup[2] & (BIT2 | BIT1 | BIT0)) {
			USB_PutString(" Abort DMA transfer.. \r\n");

			// total - remain = actual transfer size
			*pDMALen -= USB_GETREG(USB_DEVDMA2CTRLSTATUS_REG_OFS + (uiChOfs << 2));

			bLocalAbort = bAbortDMA;

			// DMA Abort
			usb_abortDMA(EPn, USB_DMACH_CH2 << uiChOfs);

			// We just want to abort DMA only.
			if (bLocalAbort == FALSE) {
				bAbortDMA = FALSE;
				clr_flg(FLG_ID_USBTSK, (FLGUSB_DMA2DONE << uiChOfs));
			}
		}

		// signal USB DMA channel is ready again
		set_flg(FLG_ID_USB, FLGPTN_USB_DMA_RDY);
	}

	return E_OK;
}

//@}

#endif
#if 1

/**
    @name USB endpoint and FIFO setting functions
*/
//@{

/**
    Set Configuration value to dedicate Endpoint

    Set Configuration value to dedicate Endpoint

    @param[in] EPn          Endpoint number.
    @param[in] CfgID        Configuration Select ID.
    @param[in] uiCfgValue   Configuration value.

    @return void
*/
void usb2dev_set_ep_config(USB_EP EPn, USB_EPCFG_ID CfgID, UINT32 uiCfgValue)
{
	T_USB_DEVADDR_REG               RegDevAdr;
	T_USB_DEVISOIN_HDRCTRL0_REG     RegHdr1Ctr0, RegHdr2Ctr0;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		usb2dev528_set_ep_config(EPn, CfgID, uiCfgValue);
		return;
	}

	// LOG the settings as global variables and activate settings after config.
	// If already config and then set to register directly.

	RegDevAdr.reg  = USB_GETREG(USB_DEVADDR_REG_OFS);

	switch (CfgID) {
	case USB_EPCFG_ID_AUTOHDR_EN: {
			gUvcHdrCfg[EPn].bEn = uiCfgValue > 0;

			if ((RegDevAdr.bit.AFT_CONF) && (usb2dev_get_controller_state() != USB_CONTROLLER_STATE_SUSPEND)) {
				RegHdr1Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS);
				RegHdr2Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS);

				if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr1Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
					/* HDR Function 1 */
					RegHdr1Ctr0.bit.HDR_EN = uiCfgValue > 0;

					if (uiCfgValue > 0) {
						RegHdr1Ctr0.bit.HDR_START = gUvcHdrCfg[EPn].bStart;
						RegHdr1Ctr0.bit.HDR_STOP  = gUvcHdrCfg[EPn].bStop;
					} else {
						RegHdr1Ctr0.bit.HDR_START = 0;
						RegHdr1Ctr0.bit.HDR_STOP  = 0;
					}

					USB_SETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS,  RegHdr1Ctr0.reg);
				} else if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr2Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
					/* HDR Function 2 */
					RegHdr2Ctr0.bit.HDR_EN = uiCfgValue > 0;

					if (uiCfgValue > 0) {
						RegHdr2Ctr0.bit.HDR_START = gUvcHdrCfg[EPn].bStart;
						RegHdr2Ctr0.bit.HDR_STOP  = gUvcHdrCfg[EPn].bStop;
					} else {
						RegHdr2Ctr0.bit.HDR_START = 0;
						RegHdr2Ctr0.bit.HDR_STOP  = 0;
					}

					USB_SETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS,  RegHdr2Ctr0.reg);
				} else {
					DBG_ERR("Not Valid Setup. %d %d\r\n", EPn, CfgID);
				}
			}
		}
		break;

	case USB_EPCFG_ID_AUTOHDR_LEN: {
			if (!((uiCfgValue == 2) || (uiCfgValue == 12))) {
				DBG_ERR("No Sprt Hdr Len = %d\r\n", (UINT)uiCfgValue);
			} else {
				// Length setting can only activate during setConfig
				gUvcHdrCfg[EPn].uiLength = uiCfgValue;
			}
		}
		break;

	case USB_EPCFG_ID_AUTOHDR_START: {
			gUvcHdrCfg[EPn].bStart = uiCfgValue > 0;

			if ((RegDevAdr.bit.AFT_CONF) && (usb2dev_get_controller_state() != USB_CONTROLLER_STATE_SUSPEND)) {
				RegHdr1Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS);
				RegHdr2Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS);

				if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr1Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
					/* HDR Function 1 */
					if (RegHdr1Ctr0.bit.HDR_EN) {
						RegHdr1Ctr0.bit.HDR_START = uiCfgValue > 0;
						USB_SETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS,  RegHdr1Ctr0.reg);
					}
				} else if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr2Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
					/* HDR Function 2 */
					if (RegHdr2Ctr0.bit.HDR_EN) {
						RegHdr2Ctr0.bit.HDR_START = uiCfgValue > 0;
						USB_SETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS,  RegHdr2Ctr0.reg);
					}
				} else {
					DBG_ERR("Not Valid Setup. %d %d\r\n", EPn, CfgID);
				}

			}

		}
		break;

	case USB_EPCFG_ID_AUTOHDR_STOP: {
			gUvcHdrCfg[EPn].bStop = uiCfgValue > 0;

			if ((RegDevAdr.bit.AFT_CONF) && (usb2dev_get_controller_state() != USB_CONTROLLER_STATE_SUSPEND)) {
				RegHdr1Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS);
				RegHdr2Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS);

				if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr1Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
					/* HDR Function 1 */
					if (RegHdr1Ctr0.bit.HDR_EN) {
						RegHdr1Ctr0.bit.HDR_STOP = uiCfgValue > 0;
						USB_SETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS,  RegHdr1Ctr0.reg);
					}
				} else if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr2Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
					/* HDR Function 2 */
					if (RegHdr2Ctr0.bit.HDR_EN) {
						RegHdr2Ctr0.bit.HDR_STOP = uiCfgValue > 0;
						USB_SETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS,  RegHdr2Ctr0.reg);
					}
				} else {
					DBG_ERR("Not Valid Setup. %d %d\r\n", EPn, CfgID);
				}

			}

		}
		break;

	case USB_EPCFG_ID_AUTOHDR_RSTCOUNTER: {
			USB_SETREG(USB_DEVISOIN_COUNT0_REG_OFS, (uiCfgValue > 0));
		}
		break;


#if _EMULATION_
	case USB_EPCFG_ID_AUTOHDR_HWFID_EN: {
			if ((RegDevAdr.bit.AFT_CONF) && (usb2dev_get_controller_state() != USB_CONTROLLER_STATE_SUSPEND)) {
				RegHdr1Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS);
				RegHdr2Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS);

				if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr1Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
					/* HDR Function 1 */
					RegHdr1Ctr0.bit.HWFID = uiCfgValue > 0;
					USB_SETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS,  RegHdr1Ctr0.reg);

				} else if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr2Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
					/* HDR Function 2 */
					RegHdr2Ctr0.bit.HWFID = uiCfgValue > 0;
					USB_SETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS,  RegHdr2Ctr0.reg);
				} else {
					DBG_ERR("Not Valid Setup. %d %d\r\n", EPn, CfgID);
				}

			}
		}
		break;

	case USB_EPCFG_ID_AUTOHDR_SWFID: {
			T_USB_DEVISOIN_HDRSW0_REG       RegHdr1Sw0, RegHdr2Sw0;


			RegHdr1Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS);
			RegHdr2Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS);

			if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr1Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
				/* HDR Function 1 */

				RegHdr1Sw0.reg = USB_GETREG(USB_DEVISOIN_HDR1SW0_REG_OFS);
				RegHdr1Sw0.bit.SWFID = uiCfgValue > 0;
				USB_SETREG(USB_DEVISOIN_HDR1SW0_REG_OFS,  RegHdr1Sw0.reg);

			} else if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr2Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
				/* HDR Function 2 */

				RegHdr2Sw0.reg = USB_GETREG(USB_DEVISOIN_HDR2SW0_REG_OFS);
				RegHdr2Sw0.bit.SWFID = uiCfgValue > 0;
				USB_SETREG(USB_DEVISOIN_HDR2SW0_REG_OFS,  RegHdr2Sw0.reg);
			} else {
				DBG_ERR("Not Valid Setup. %d %d\r\n", EPn, CfgID);
			}



		}
		break;

	case USB_EPCFG_ID_AUTOHDR_HWPTS_EN: {
			if ((RegDevAdr.bit.AFT_CONF) && (usb2dev_get_controller_state() != USB_CONTROLLER_STATE_SUSPEND)) {
				RegHdr1Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS);
				RegHdr2Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS);

				if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr1Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
					/* HDR Function 1 */
					RegHdr1Ctr0.bit.HWPTS = uiCfgValue > 0;
					USB_SETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS,  RegHdr1Ctr0.reg);

				} else if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr2Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
					/* HDR Function 2 */
					RegHdr2Ctr0.bit.HWPTS = uiCfgValue > 0;
					USB_SETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS,  RegHdr2Ctr0.reg);
				} else {
					DBG_ERR("Not Valid Setup. %d %d\r\n", EPn, CfgID);
				}

			}
		}
		break;

	case USB_EPCFG_ID_AUTOHDR_SWPTS: {
			T_USB_DEVISOIN_HDRSW0_REG       RegHdr1Sw0, RegHdr2Sw0;
			T_USB_DEVISOIN_HDRSW1_REG       RegHdr1Sw1, RegHdr2Sw1;

			while (usb2dev_check_ep_busy(EPn)) {
				USB_DELAY_US(50);
			}

			RegHdr1Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS);
			RegHdr2Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS);

			if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr1Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
				/* HDR Function 1 */

				RegHdr1Sw0.reg = USB_GETREG(USB_DEVISOIN_HDR1SW0_REG_OFS);
				RegHdr1Sw0.bit.PTS15 = uiCfgValue & 0xFFFF;
				USB_SETREG(USB_DEVISOIN_HDR1SW0_REG_OFS,  RegHdr1Sw0.reg);

				RegHdr1Sw1.reg = USB_GETREG(USB_DEVISOIN_HDR1SW1_REG_OFS);
				RegHdr1Sw1.bit.PTS31 = (uiCfgValue >> 16) & 0xFFFF;
				USB_SETREG(USB_DEVISOIN_HDR1SW1_REG_OFS,  RegHdr1Sw1.reg);
			} else if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr2Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
				/* HDR Function 2 */

				RegHdr2Sw0.reg = USB_GETREG(USB_DEVISOIN_HDR2SW0_REG_OFS);
				RegHdr2Sw0.bit.PTS15 = uiCfgValue & 0xFFFF;
				USB_SETREG(USB_DEVISOIN_HDR2SW0_REG_OFS,  RegHdr2Sw0.reg);

				RegHdr2Sw1.reg = USB_GETREG(USB_DEVISOIN_HDR2SW1_REG_OFS);
				RegHdr2Sw1.bit.PTS31 = (uiCfgValue >> 16) & 0xFFFF;
				USB_SETREG(USB_DEVISOIN_HDR2SW1_REG_OFS,  RegHdr2Sw1.reg);
			} else {
				DBG_ERR("Not Valid Setup. %d %d\r\n", EPn, CfgID);
			}
		}
		break;

#else

	case USB_EPCFG_ID_AUTOHDR_HWFID_EN:
	case USB_EPCFG_ID_AUTOHDR_SWFID:
	case USB_EPCFG_ID_AUTOHDR_HWPTS_EN:
	case USB_EPCFG_ID_AUTOHDR_SWPTS: {
			DBG_WRN("Reserved function. %d", CfgID);
		}
		break;

#endif


	default:
		DBG_ERR("EP%d No Sprt ID %d\r\n", EPn, CfgID);
		break;
	}

}

/**
    Get Configuration value from dedicate Endpoint

    Get Configuration value from dedicate Endpoint

    @param[in] EPn      Endpoint number.
    @param[in] CfgID    Configuration Select ID.

    @return Configuration value
*/
UINT32 usb2dev_get_ep_config(USB_EP EPn, USB_EPCFG_ID CfgID)
{
	UINT32                      Ret = 0xABCD1238;
	T_USB_DEVISOIN_HDRCTRL0_REG RegHdr1Ctr0, RegHdr2Ctr0;
	T_USB_DEVADDR_REG           RegDevAdr;
	T_USB_DEVISOIN_HDRSW3_REG   RegHdrSw3;
	T_USB_DEVISOIN_HDRSW4_REG   RegHdrSw4;
	T_USB_DEVISOIN_HDRSW5_REG   RegHdrSw5;
	T_USB_DEVISOIN_HDRSW6_REG   RegHdrSw6;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		return usb2dev528_get_ep_config(EPn, CfgID);
	}

	switch (CfgID) {
	case USB_EPCFG_ID_AUTOHDR_EN: {
			Ret = gUvcHdrCfg[EPn].bEn;
		}
		break;

	case USB_EPCFG_ID_AUTOHDR_LEN: {
			Ret = gUvcHdrCfg[EPn].uiLength;
		}
		break;

	case USB_EPCFG_ID_AUTOHDR_START: {
			Ret = gUvcHdrCfg[EPn].bStart;
		}
		break;

	case USB_EPCFG_ID_AUTOHDR_STOP: {
			Ret = gUvcHdrCfg[EPn].bStop;
		}
		break;

	default:
		break;
	}


	if (Ret != 0xABCD1238) {
		return Ret;
	}

	RegDevAdr.reg  = USB_GETREG(USB_DEVADDR_REG_OFS);

	if ((RegDevAdr.bit.AFT_CONF) && (usb2dev_get_controller_state() != USB_CONTROLLER_STATE_SUSPEND)) {
		UINT8 HdrNo = 2;

		RegHdr1Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR1CTRL0_REG_OFS);
		RegHdr2Ctr0.reg = USB_GETREG(USB_DEVISOIN_HDR2CTRL0_REG_OFS);

		if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr1Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
			HdrNo = 0;
		} else if (usb_getFifoFromEp(EPn) == (USB_FIFO_NUM)(RegHdr2Ctr0.bit.FIFO_SEL + USB_FIFO4)) {
			HdrNo = 1;
		}




		switch (CfgID) {
		case USB_EPCFG_ID_AUTOHDR_FID_VAL: {
				RegHdrSw3.reg = USB_GETREG(USB_DEVISOIN_HDR1SW3_REG_OFS + (HdrNo << 5));
				Ret = RegHdrSw3.bit.HWFID;
			}
			break;
		case USB_EPCFG_ID_AUTOHDR_EOF_VAL: {
				RegHdrSw3.reg = USB_GETREG(USB_DEVISOIN_HDR1SW3_REG_OFS + (HdrNo << 5));
				Ret = RegHdrSw3.bit.HWEOF;
			}
			break;
		case USB_EPCFG_ID_AUTOHDR_PTS_VAL: {
				RegHdrSw4.reg = USB_GETREG(USB_DEVISOIN_HDR1SW4_REG_OFS + (HdrNo << 5));
				Ret = RegHdrSw4.bit.PTS31;
			}
			break;
		case USB_EPCFG_ID_AUTOHDR_SCRLSB_VAL: {
				RegHdrSw5.reg = USB_GETREG(USB_DEVISOIN_HDR1SW5_REG_OFS + (HdrNo << 5));
				Ret = RegHdrSw5.bit.SCR31;
			}
			break;
		case USB_EPCFG_ID_AUTOHDR_SCRMSB_VAL: {
				RegHdrSw6.reg = USB_GETREG(USB_DEVISOIN_HDR1SW6_REG_OFS + (HdrNo << 5));
				Ret = RegHdrSw6.bit.SCR47;
			}
			break;

		default:
			DBG_ERR("EP%d No Sprt ID %d\r\n", EPn, CfgID);
			break;

		}

	} else {
		Ret = 0;
		DBG_ERR("SUSP\r\n");
	}


	return Ret;

}

/**
    Mask(Disable) the endpoint interrupt

    @param[in] EPn  endpoint number (USB_EP0~USB_EP8)

    @return void
*/
void usb2dev_mask_ep_interrupt(USB_EP EPn)
{
	if(nvt_get_chip_id() == CHIP_NA51084) {
		usb2dev528_mask_ep_interrupt(EPn);
		return;
	}

	if (EPn == USB_EP0) {
		T_USB_DEVINTMASKGROUP0_REG devIntMskGrp0;
		unsigned long      flags;

		loc_cpu(flags);
		devIntMskGrp0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
		devIntMskGrp0.bit.MCX_IN_INT  = 1;
		devIntMskGrp0.bit.MCX_OUT_INT = 1;
		USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, devIntMskGrp0.reg);
		unl_cpu(flags);
		return;
	}

	if (EPn >= USB_EP_MAX) {
		DBG_ERR("EP%d out of range\r\n", EPn);
		return;
	}

	if (gEPMap[EPn - 1] == USB_FIFO_NOT_USE) {
		DBG_ERR("no FIFO maps to EP%d\r\n", EPn);
		return;
	}

	if (gEPMap[EPn - 1] >= USB_FIFO_MAX) {
		DBG_ERR("EP%d maps to FIFO%d\r\n", EPn, (UINT)gEPMap[EPn - 1]);
		return;
	}

	if (usb_getEpDirection(EPn) == EP_DIR_IN) {
		usb_maskFIFOININT(gEPMap[EPn - 1]);
	} else {
		usb_maskFIFOOUTINT(gEPMap[EPn - 1]);
	}

}


/**
    Unmask(Enable) the endpoint interrupt

    @param[in] EPn  endpoint number (USB_EP0~USB_EP8)

    @return void
*/
void usb2dev_unmask_ep_interrupt(USB_EP EPn)
{
	if(nvt_get_chip_id() == CHIP_NA51084) {
		usb2dev528_unmask_ep_interrupt(EPn);
		return;
	}

	if (EPn == USB_EP0) {
		T_USB_DEVINTMASKGROUP0_REG devIntMskGrp0;
		unsigned long      flags;

		loc_cpu(flags);
		devIntMskGrp0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
		devIntMskGrp0.bit.MCX_IN_INT  = 0;
		devIntMskGrp0.bit.MCX_OUT_INT = 0;
		USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, devIntMskGrp0.reg);
		unl_cpu(flags);
		return;
	}

	if (EPn >= USB_EP_MAX) {
		DBG_ERR("EP%d out of range\r\n", EPn);
		return;
	}

	if (gEPMap[EPn - 1] == USB_FIFO_NOT_USE) {
		DBG_ERR("no FIFO maps to EP%d\r\n", EPn);
		return;
	}

	if (gEPMap[EPn - 1] >= USB_FIFO_MAX) {
		DBG_ERR("EP%d maps to FIFO%d\r\n", EPn, (UINT)gEPMap[EPn - 1]);
		return;
	}

	if (usb_getEpDirection(EPn) == EP_DIR_IN) {
		usb_unmaskFIFOININT(gEPMap[EPn - 1]);
	} else {
		usb_unmaskFIFOOUTINT(gEPMap[EPn - 1]);
	}

}

/**
    Set endpoint0 CX_DONE

    When class library completes transfers of CX IN/OUT, it should invoke this API to enter status stage.

    @return void
*/
void usb2dev_set_ep0_done(void)
{
	T_USB_DEVCXCFGFIFOSTATUS_REG devCXCFG;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		usb2dev528_set_ep0_done();
		return;
	}

	devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
	devCXCFG.bit.CX_DONE = 1;
	USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);
}

/**
    Set endpoint stall

    When class library finds somethins wrong, it can use this API to make a function stall for this endpoint.

    @param[in] EPn  endpoint number (USB_EP0~USB_EP15)

    @return void
*/
void usb2dev_set_ep_stall(USB_EP EPn)
{
	T_USB_DEVMAXPS_EP_REG           devEPMaxPS;
	T_USB_DEVCXCFGFIFOSTATUS_REG    devCXCFG;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		usb2dev528_set_ep_stall(EPn);
		return;
	}

	if (EPn == USB_EP0) {
		devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		devCXCFG.bit.CX_STL = 1;
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);
	} else if (EPn <= USB_EP15) {
		devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((EPn - USB_EP1) << 2));
		devEPMaxPS.bit.STL_EP = 1;
		USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((EPn - USB_EP1) << 2)), devEPMaxPS.reg);
	}
}

/**
    clear FIFO

    @return void
*/
void usb2dev_clear_fifo(void)
{
	T_USB_DEVTEST_REG devTest;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		usb2dev528_clear_fifo();
		return;
	}

	devTest.reg = USB_GETREG(USB_DEVTEST_REG_OFS);
	devTest.bit.TST_CLRFF = 1;
	USB_SETREG(USB_DEVTEST_REG_OFS, devTest.reg);
}


/**
    Clear Endpoint FIFO

    Clear FIFO of related endpoint.

    @param[in] EPn  endpoint number (USB_EP0~USB_EP15)
    @return void
*/
void usb2dev_clear_ep_fifo(USB_EP EPn)
{
	UINT32                          fifo_num;
	UINT32                          uiBufNum;
	T_USB_DEVCXCFGFIFOSTATUS_REG    devCXCFG;
	T_USB_DEVFIFOBYTECNT0_REG       devFIFOByteCnt;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		usb2dev528_clear_ep_fifo(EPn);
		return;
	}

	// clear FIFO
	switch (EPn) {
	case USB_EP0 : { // CX FIFO
			devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
			devCXCFG.bit.CX_CLR = 1;
			USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);
		}
		break;

	case USB_EP1:
	case USB_EP2:
	case USB_EP3:
	case USB_EP4:
	case USB_EP5:
	case USB_EP6:
	case USB_EP7:
	case USB_EP8:
	case USB_EP9:
	case USB_EP10:
	case USB_EP11:
	case USB_EP12:
	case USB_EP13:
	case USB_EP14:
	case USB_EP15: {
			if (gEPMap[EPn - 1] == USB_FIFO_NOT_USE) {
				DBG_ERR("no FIFO maps to EP%d\r\n", EPn);
				return;
			}

			if (gEPMap[EPn - 1] >= USB_FIFO_MAX) {
				DBG_ERR("EP%d maps to FIFO%d\r\n", EPn, (UINT)gEPMap[EPn - 1]);
				return;
			}

			if (usb2dev_is_highspeed_enabled()) {
				uiBufNum = gUSBManage.ep_config_hs[EPn - 1].blk_num;

				if (gUSBManage.ep_config_hs[EPn - 1].blk_size > 512) {
					uiBufNum = uiBufNum << 1;
				}
			} else {
				uiBufNum = gUSBManage.ep_config_fs[EPn - 1].blk_num;

				if (gUSBManage.ep_config_fs[EPn - 1].blk_size > 512) {
					uiBufNum = uiBufNum << 1;
				}
			}


			// All FIFO mapped to this EP should be reset.
			for (fifo_num = gEPMap[EPn - 1]; fifo_num < (gEPMap[EPn - 1] + uiBufNum); fifo_num++) {
				devFIFOByteCnt.reg  = USB_GETREG(USB_DEVFIFOBYTECNT0_REG_OFS + ((fifo_num >> 1) << 2));
				devFIFOByteCnt.reg |= (0x1 << (12 + ((fifo_num & 0x1) << 4)));
				USB_SETREG(USB_DEVFIFOBYTECNT0_REG_OFS + ((fifo_num >> 1) << 2), devFIFOByteCnt.reg);
			}
		}
		break;

	default :
		break;
	}
}


/**
    Set the endpoint to send a 0 byte data

    @param[in] EPn      endpoint number (USB_EP1~USB_EP15)

    @return void
*/
void usb2dev_set_tx0byte(USB_EP EPn)
{
	T_USB_DEVMAXPS_EP_REG DevEPMaxPS;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		usb2dev528_set_tx0byte(EPn);
		return;
	}

	if (EPn == USB_EP0) {
		return;
	}

	DevEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((EPn - USB_EP1) << 2));
	DevEPMaxPS.bit.TX0BYTE_IEP = 1;
	USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((EPn - USB_EP1) << 2)), DevEPMaxPS.reg);
}

/**
    Get endpoint byte count.

    Some application may not know the exactly transfering size at
    the beginning. Use this function to get the FIFO byte counts.
    This API should be used after OUT interrupt issued.

    @param[in] EPn      endpoint number (USB_EP0~USB_EP15)

    @return endpoint byte count
*/
UINT32 usb2dev_get_ep_bytecount(USB_EP EPn)
{
	T_USB_DEVFIFOBYTECNT0_REG       devFIFOByteCnt;
	T_USB_DEVCXCFGFIFOSTATUS_REG    devCXCFG;
	UINT32                          fifo_num;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		return usb2dev528_get_ep_bytecount(EPn);
	}

	if (EPn >= USB_EP_MAX) {
		DBG_ERR("EP%d out of range\r\n", EPn);
		return 0;
	}

	if (EPn) {
		if (gEPMap[EPn - 1] == USB_FIFO_NOT_USE) {
			DBG_ERR("no FIFO maps to EP%d\r\n", EPn);
			return 0;
		}

		if (gEPMap[EPn - 1] >= USB_FIFO_MAX) {
			DBG_ERR("EP%d maps to FIFO%d\r\n", EPn, (UINT)gEPMap[EPn - 1]);
			return 0;
		}
	}

	if (EPn == USB_EP0) {
		devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);

		return (UINT32)devCXCFG.bit.CX_FNT_OUT;
	} else {
		fifo_num = gEPMap[EPn - 1];

		devFIFOByteCnt.reg = USB_GETREG(USB_DEVFIFOBYTECNT0_REG_OFS + ((fifo_num >> 1) << 2));

		if (fifo_num & 0x1) {
			return (UINT32)devFIFOByteCnt.bit.BC_F1;
		} else {
			return (UINT32)devFIFOByteCnt.bit.BC_F0;
		}
	}
}

/**
    Check endpoint FIFO empty


    @param[in] EPn      endpoint number (USB_EP1~USB_EP8)

    @return
        - @b TRUE: FIFO for this endpoint is empty
        - @b FALSE: FIFO for this endpoint is NOT empty
*/
BOOL usb2dev_check_ep_empty(USB_EP EPn)
{
	T_USB_DEVCXCFGFIFOSTATUS_REG fifoStsReg;
	UINT32 uiBufNum;
	UINT32 uiMask;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		return usb2dev528_check_ep_empty(EPn);
	}

	fifoStsReg.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);

	switch (EPn) {
	case USB_EP0: {
			return fifoStsReg.bit.CX_EMP;
		}

	case USB_EP1:
	case USB_EP2:
	case USB_EP3:
	case USB_EP4:
	case USB_EP5:
	case USB_EP6:
	case USB_EP7:
	case USB_EP8:
	case USB_EP9:
	case USB_EP10:
	case USB_EP11:
	case USB_EP12:
	case USB_EP13:
	case USB_EP14:
	case USB_EP15: {
			if (gEPMap[EPn - 1] == USB_FIFO_NOT_USE) {
				DBG_ERR("no FIFO for input EP %d\r\n", EPn);
				return FALSE;
			}
			if (gEPMap[EPn - 1] >= USB_FIFO_MAX) {
				DBG_ERR("FIFO %d for EP %d is not valid\r\n", (UINT)gEPMap[EPn - 1], EPn);
				return FALSE;
			}
			if (usb2dev_is_highspeed_enabled()) {
				uiBufNum = gUSBManage.ep_config_hs[EPn - 1].blk_num;

				if (gUSBManage.ep_config_hs[EPn - 1].blk_size > 512) {
					uiBufNum = uiBufNum << 1;
				}
			} else {
				uiBufNum = gUSBManage.ep_config_fs[EPn - 1].blk_num;

				if (gUSBManage.ep_config_fs[EPn - 1].blk_size > 512) {
					uiBufNum = uiBufNum << 1;
				}
			}

			uiMask = ((1 << uiBufNum) - 1) << gEPMap[EPn - 1];

			if ((uiMask & fifoStsReg.bit.F_EMP) == uiMask) {
				// All FIFO for this EP are empty
				return TRUE;
			} else {
				// some FIFO for this EP are not empty
				return FALSE;
			}
		}
		break;

	default: {
			DBG_ERR("input EP %d out of range\r\n", EPn);
		}
		return FALSE;
	}
}


/**
    Check endpoint is Busy or not.


    @param[in] EPn      endpoint number (USB_EP1~USB_EP15)

    @return
        - @b FALSE:      FIFO for this endpoint is empty and DMA is not working.
        - @b TRUE:     FIFO for this endpoint is busy.
*/
BOOL usb2dev_check_ep_busy(USB_EP EPn)
{
	T_USB_DEVCXCFGFIFOSTATUS_REG    fifoStsReg;
	UINT32                          uiBufNum;
	UINT32                          uiMask;
	USB_FIFO_NUM                    fifonum;
	T_USB_DEVDMACTRL1_REG           devDMACtl1;
	T_USB_DEVDMA2CTRL0_REG          devDMAxCtl;
	UINT32                          uiRegOfs;

	if(nvt_get_chip_id() == CHIP_NA51084) {
		return usb2dev528_check_ep_busy(EPn);
	}

	fifoStsReg.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);

	switch (EPn) {
	case USB_EP0: {
			return fifoStsReg.bit.CX_EMP;
		}

	case USB_EP1:
	case USB_EP2:
	case USB_EP3:
	case USB_EP4:
	case USB_EP5:
	case USB_EP6:
	case USB_EP7:
	case USB_EP8:
	case USB_EP9:
	case USB_EP10:
	case USB_EP11:
	case USB_EP12:
	case USB_EP13:
	case USB_EP14:
	case USB_EP15: {
			if (gEPMap[EPn - 1] == USB_FIFO_NOT_USE) {
				DBG_ERR("no FIFO for input EP %d\r\n", EPn);
				return FALSE;
			}
			if (gEPMap[EPn - 1] >= USB_FIFO_MAX) {
				DBG_ERR("FIFO %d for EP %d is not valid\r\n", (UINT)gEPMap[EPn - 1], EPn);
				return FALSE;
			}

			fifonum = usb_getFifoFromEp(EPn);
			if (fifonum < USB_FIFO4) {
				// check if DMA being acccessed
				devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
				if (devDMACtl1.bit.DMA_START == 1) {
					return TRUE;
				}
			} else if (fifonum < USB_FIFO_MAX) {
				uiRegOfs = (fifonum - USB_FIFO4) << 3;

				devDMAxCtl.reg = USB_GETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs);
				if (devDMAxCtl.bit.DMA_START == 1) {
					return TRUE;
				}
			}

			// If is OUT EP, the EP is available if DMA_START=0,
			// Because the fifo may be written by host.
			if (usb_getEpDirection(EPn) == EP_DIR_OUT) {
				return FALSE;
			}

			if (usb2dev_is_highspeed_enabled()) {
				uiBufNum = gUSBManage.ep_config_hs[EPn - 1].blk_num;

				if (gUSBManage.ep_config_hs[EPn - 1].blk_size > 512) {
					uiBufNum = uiBufNum << 1;
				}
			} else {
				uiBufNum = gUSBManage.ep_config_fs[EPn - 1].blk_num;

				if (gUSBManage.ep_config_fs[EPn - 1].blk_size > 512) {
					uiBufNum = uiBufNum << 1;
				}
			}

			uiMask = ((1 << uiBufNum) - 1) << gEPMap[EPn - 1];

			if ((uiMask & fifoStsReg.bit.F_EMP) == uiMask) {
				// All FIFO for this EP are empty
				return FALSE; // Not busy
			} else {
				// some FIFO for this EP are not empty
				return TRUE;  // TRUE
			}
		}
		break;

	default: {
			DBG_ERR("input EP %d out of range\r\n", EPn);
		}
		return FALSE;
	}
}


/**
    Get SOF

    Return current SOF value

    @return SOF value
*/
UINT32 usb2dev_get_sof_number(void)
{
	if(nvt_get_chip_id() == CHIP_NA51084) {
		return usb2dev528_get_sof_number();
	}

	return (USB_GETREG(USB_DEVSOFNUM_REG_OFS) & 0x7FF);
}
#endif
//@}

#if _EMULATION_
// Hidden API for IC test
UINT32 usb_pauseResumeDmaTest(USB_EP EPn);

UINT32 usb_pauseResumeDmaTest(USB_EP EPn)
{
	T_USB_DEVDMACTRL0_REG       devDMACtl0;
	T_USB_DEVDMA2CTRL0_REG      devDMAxCtl;
	T_USB_DEVINTGROUP2_REG      devIntGrp2;
	T_USB_DEVINTMASKGROUP2_REG  devIntMskGrp2;
	T_USB_DEVMAINCTRL_REG       devMainCtl;
	T_USB_DEVDMACTRL1_REG       devDMACtl1;
	T_USB_DEVDMACTRLSTATUS_REG  RegRemain1;
	UINT32                      loop_cnt = 0;
	BOOL                        bIntDis;
	UINT32                      i, uiRegOfs = 0;
	USB_FIFO_NUM                fifonum;

	fifonum = usb_getFifoFromEp(EPn);

	//
	//  DMA CH1
	//
	if ((fifonum >= USB_FIFO0) && (fifonum <= USB_FIFO3)) {
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		if (devDMACtl1.bit.DMA_START == 1) {
			// mask DMA_CMPLT interrupt
			devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
			bIntDis = devIntMskGrp2.bit.MDMA_CMPLT;
			if (!bIntDis) {
				devIntMskGrp2.bit.MDMA_CMPLT = 1;
				USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);
			}

			// set DMA_ABORT & CLRFIFO
			devDMACtl0.reg = USB_GETREG(USB_DEVDMACTRL0_REG_OFS);
			devDMACtl0.bit.DMA_ABORT         = 1;
			//devDMACtl0.bit.CLRFIFO_DMA_ABORT = 1;
			USB_SETREG(USB_DEVDMACTRL0_REG_OFS, devDMACtl0.reg);

			// wait DMA_CMPLT and clear it
			do {
				devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);

				loop_cnt++;
				//  check if DMA abort fail...
				if (loop_cnt > 0x20) {
					USB_PutString("\r\nDMA-1 abort fail !!!\r\n");

					// force DMA abort if fail happen...
					devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
					devMainCtl.bit.FORCE_DMA_ABORT = 1;
					USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

					do {
						devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);
						DBG_WRN(" @ ");

					} while (devIntGrp2.bit.DMA_CMPLT == 0);

					loop_cnt = 0;
					break;
				}
			} while (devIntGrp2.bit.DMA_CMPLT == 0);

			devIntGrp2.bit.DMA_CMPLT = 1;
			USB_SETREG(USB_DEVINTGROUP2_REG_OFS, devIntGrp2.reg);

			// disable CLRFIFO
			devDMACtl0.bit.CLRFIFO_DMA_ABORT = 0;
			USB_SETREG(USB_DEVDMACTRL0_REG_OFS, devDMACtl0.reg);

			// Pause / Resume Delay
			USB_DELAY_US(200);

			// unmask DMA_CMPLT interrupt
			devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
			devIntMskGrp2.bit.MDMA_CMPLT = 0;
			USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);

			// Resume DMA
			{
				UINT32 Txed;

				RegRemain1.reg = USB_GETREG(USB_DEVDMACTRLSTATUS_REG_OFS);
				Txed = devDMACtl1.bit.DMA_LEN - RegRemain1.reg;

				USB_SETREG(USB_DEVDMACTRL2_REG_OFS, USB_GETREG(USB_DEVDMACTRL2_REG_OFS) + Txed);

				devDMACtl1.bit.DMA_START = 0;
				devDMACtl1.bit.DMA_LEN   = RegRemain1.reg;
				USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

				devDMACtl1.bit.DMA_START = 1;
				USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);
			}
			DBG_WRN("USB Remain Size = %d; ", RegRemain1.reg);
		}

	}

	if (fifonum >= USB_FIFO4) {
		i = fifonum - USB_FIFO4;
		uiRegOfs = i << 3;
		loop_cnt = 0;

		devDMAxCtl.reg = USB_GETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs);
		if (devDMAxCtl.bit.DMA_START == 1) {
			// mask DMA_CMPLT interrupt
			devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
			bIntDis = (devIntMskGrp2.reg >> (9 + i)) & 0x1;
			if (!bIntDis) {
				devIntMskGrp2.reg |= (0x1 << (9 + i));
				USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);
			}

			// set DMA_ABORT & CLRFIFO
			devDMAxCtl.bit.DMA_ABORT        = 1;
			devDMAxCtl.bit.DMA_START        = 0;
			//devDMAxCtl.bit.DMA_CLRFIFO    = 1;
			devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);
			if ((devIntGrp2.reg  & (0x1 << (9 + i))) == 0) {
				USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);
				devDMAxCtl.bit.DMA_ABORT   = 0;

				// wait DMA_CMPLT and clear it
				do {
					devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS);

					loop_cnt++;
					if (loop_cnt > 0x1800000) {
						DBG_ERR("DMA-%d Abort Fail!!\r\n", i + 2);
						break;
					}
				} while ((devIntGrp2.reg & (0x1 << (9 + i))) == 0);

				// Clear Int Sts
				devIntGrp2.reg |= (0x1 << (9 + i));
				USB_SETREG(USB_DEVINTGROUP2_REG_OFS, devIntGrp2.reg);

				// disable CLRFIFO
				devDMAxCtl.bit.DMA_CLRFIFO = 0;
				USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);
			}

			// unmask DMA_CMPLT interrupt
			devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
			if (!bIntDis) {
				devIntMskGrp2.reg &= ~(0x1 << (9 + i));
				USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);
			}

			// Pause / Resume Delay
			USB_DELAY_US(200);

			// Resume DMA
			{
				UINT32 Txed;

				RegRemain1.reg = USB_GETREG(USB_DEVDMA2CTRLSTATUS_REG_OFS + (i << 2));
				if (RegRemain1.reg > 0) {
					Txed = devDMAxCtl.bit.DMA_LEN - RegRemain1.reg;

					USB_SETREG(USB_DEVDMA2CTRL1_REG_OFS + uiRegOfs, USB_GETREG(USB_DEVDMA2CTRL1_REG_OFS + uiRegOfs) + Txed);

					devDMAxCtl.bit.DMA_START = 0;
					devDMAxCtl.bit.DMA_LEN   = RegRemain1.reg;
					USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);

					devDMAxCtl.bit.DMA_START = 1;
					USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);

					DBG_WRN("USB D%d Remain Size = %d; ", i + 2, RegRemain1.reg);
					DBG_WRN("USB D%d Txed Size = 0x%X; ", i + 2, Txed);
				} else {
					DBG_WRN("FLG ");
					set_flg(FLG_ID_USBTSK, (FLGUSB_DMA2DONE << i));
				}
			}


		}


	}

	return 0;

}

#endif
//@}

