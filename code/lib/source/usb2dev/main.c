#include <kwrap/nvt_type.h>
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <kwrap/debug.h>
#include <kwrap/cpu.h>

#include "usb2dev.h"
#include <unistd.h>
#include <hdal.h>

_ALIGNED(64) USB_DEVICE_DESC gU2HsTestDevDesc = {
	//---- device descriptor ----
	18,                             /* descriptor size                  */
	USB_DEVICE_DESCRIPTOR_TYPE,     /* descriptor type                  */
	0x0200,                         /* spec. release number             */
	0,                              /* class code                       */
	0,                              /* sub class code                   */
	0,                              /* protocol code                    */
	64,                             /* max packet size for endpoint 0   */
	0x2310,                         /* vendor id                        */
	0x5678,                         /* product id                       */
	0x0002,                         /* device release number            */
	1,                              /* manifacturer string id (0x10 in dscroot)          */
	2,                              /* product string id  (0x20 in dscroot)              */
	0,                              /* serial number string id  (0x00 in dscroot)        */
	1                               /* number of possible configuration */
};


_ALIGNED(64) UINT8 gU2UvcHsTestCfgDesc[32 + 14] = {
	//---- configuration(cfg-A) -------
	9,                              /* descriptor size                  */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,/* descriptor type                  */
	46,                          /* total length                     */
	0,                              /* total length                     */
	1,                              /* number of interface              */
	1,                              /* configuration value              */
	3,                              /* configuration string id          */
	0xE0,                           /* characteristics                  */
	0x00,                           /* maximum power in 2mA             */

	//----- cfg A interface0 ----
	//USB_INTERFACE_DESCRIPTOR, 9 bytes for Mass Storage
	// 0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,
	//---- alt0 --------
	9,                              /* descriptor size      */
	USB_INTERFACE_DESCRIPTOR_TYPE,  /* descriptor type      */
	0,                              /* interface number     */
	0,                              /* alternate setting    */
	4,                              /* number of endpoint   */
	0,                              /* interface class      */
	0,                              /* interface sub-class  */
	0,                              /* interface protocol   */
	4,                              /* interface string id  */

	//USB_ENDPOINT_DESCRIPTOR

	//---- EP1 IN ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP1I_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_BULK,                /* character address    */
	0x00,                           /* max packet size      */
	0x02,                           /* max packet size      */
	0,

	//---- EP2 OUT ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP2O_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_BULK,                /* character address    */
	0x00,                           /* max packet size      */
	0x02,                           /* max packet size      */
	0,


	//---- EP3 IN ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP3I_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_INTERRUPT,           /* character address    */
	0x00,                           /* max packet size      */
	0x04,                           /* max packet size      */
	1,

	//---- EP4 OUT ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP4O_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_INTERRUPT,           /* character address    */
	0x00,                           /* max packet size      */
	0x04,                           /* max packet size      */
	1,

};


_ALIGNED(64) UINT8 gU2FsTestCfgDesc[46] = {
	//---- configuration(cfg-A) -------
	9,                              /* descriptor size                  */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,/* descriptor type                */
	46,                             /* total length                     */
	0,                              /* total length                     */
	1,                              /* number of interface              */
	1,                              /* configuration value              */
	3,                              /* configuration string id          */
	0xE0,                           /* characteristics                  */
	0x00,                           /* maximum power in 2mA             */

	//----- cfg A interface0 ----
	//USB_INTERFACE_DESCRIPTOR, 9 bytes for Mass Storage
	// 0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,
	//---- alt0 --------
	9,                              /* descriptor size      */
	USB_INTERFACE_DESCRIPTOR_TYPE,  /* descriptor type      */
	0,                              /* interface number     */
	0,                              /* alternate setting    */
	4,                              /* number of endpoint   */
	0,                              /* interface class      */
	0,                              /* interface sub-class  */
	0,                              /* interface protocol   */
	4,                              /* interface string id  */

	//USB_ENDPOINT_DESCRIPTOR, ep1
	//---- EP1 IN ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP1I_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_BULK,                /* character address    */
	0x40,                           /* max packet size      */
	0x00,                           /* max packet size      */
	0,

	//USB_ENDPOINT_DESCRIPTOR, ep2
	//---- EP2 OUT ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,   /* descriptor type      */
	USB_EP_EP2O_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_BULK,                /* character address    */
	0x40,                           /* max packet size      */
	0x00,                           /* max packet size      */
	0,

	//---- EP3 IN ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,                 /* descriptor type      */
	USB_EP_EP3I_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_INTERRUPT,           /* character address    */
	0x40,                           /* max packet size      */
	0x00,                           /* max packet size      */
	1,

	//---- EP4 OUT ----
	7,                              /* descriptor size      */
	USB_ENDPOINT_DESCRIPTOR_TYPE,                 /* descriptor type      */
	USB_EP_EP4O_ADDRESS,            /* endpoint address     */
	USB_EP_ATR_INTERRUPT,           /* character address    */
	0x40,                           /* max packet size      */
	0x00,                           /* max packet size      */
	1,
};

static _ALIGNED(4) const USB_DEVICE_DESC gU2devQualiDesc = {
	USB_DEV_QUALI_LENGTH,
	USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,
	0x0200,
	0,
	0,
	0,
	0x40,
	1,
	0
};


_ALIGNED(64) UINT8   gU2StrDesc0[] = {
	4,                                  /* size of String Descriptor        */
	USB_STRING_DESCRIPTOR_TYPE,         /* String Descriptor type           */
	0x09, 0x04                          /*  Primary/Sub LANGID              */
};

_ALIGNED(64) UINT8   gU2StrDesc1[] = {
	0x1A,                               /* size of String Descriptor        */
	USB_STRING_DESCRIPTOR_TYPE,         /* String Descriptor type           */
	0X46, 0,  //F
	0X61, 0,  //a
	0X72, 0,  //r
	0X61, 0,  //a
	0X64, 0,  //d
	0X61, 0,  //a
	0X79, 0,  //y
	0X20, 0,  //
	0X69, 0,  //i
	0X6E, 0,  //n
	0X63, 0,  //c
	0X2E, 0   //.
};

_ALIGNED(64) UINT8   gU2StrDesc2[] = {
	0x22,                               /* size of String Descriptor        */
	USB_STRING_DESCRIPTOR_TYPE,         /* String Descriptor type           */
	0X46, 0,  //F
	0X4F, 0,  //O
	0X54, 0,  //T
	0X47, 0,  //G
	0X32, 0,  //2
	0X30, 0,  //0
	0X30, 0,  //0
	0X20, 0,  //
	0X45, 0,  //E
	0X56, 0,  //V
	0X2D, 0,  //-
	0X62, 0,  //b
	0X6F, 0,  //o
	0X61, 0,  //a
	0X72, 0,  //r
	0X64, 0   //d
};

_ALIGNED(64) UINT8   gU2StrDesc3[] = {
	0x22,                               /* size of String Descriptor        */
	USB_STRING_DESCRIPTOR_TYPE,         /* String Descriptor type           */
	0X45, 0,  //E
	0X56, 0,  //V
	0X2D, 0,  //-
	0X62, 0,  //b
	0X6F, 0,  //o
	0X61, 0,  //a
	0X72, 0,  //r
	0X64, 0,  //d
	0X20, 0,  //
	0X43, 0,  //C
	0X6F, 0,  //o
	0X6E, 0,  //n
	0X66, 0,  //f
	0X69, 0,  //i
	0X67, 0,  //g
	0X75, 0,  //u
	0X72, 0,  //r
	0X61, 0,  //a
	0X74, 0,  //t
	0X69, 0,  //i
	0X6F, 0,  //o
	0X6E, 0   //n
};

_ALIGNED(64) UINT8   gU2StrDesc4[] = {
	0x22,                               /* size of String Descriptor        */
	USB_STRING_DESCRIPTOR_TYPE,         /* String Descriptor type           */
	0X45, 0,  //E
	0X56, 0,  //V
	0X2D, 0,  //-
	0X62, 0,  //b
	0X6F, 0,  //o
	0X61, 0,  //a
	0X72, 0,  //r
	0X64, 0,  //d
	0X20, 0,  //
	0X49, 0,  //I
	0X6E, 0,  //n
	0X74, 0,  //t
	0X65, 0,  //e
	0X72, 0,  //r
	0X66, 0,  //f
	0X61, 0,  //a
	0X63, 0,  //c
	0X65, 0   //e
};



#define EMUUSBTEST_FLAG_IDLE1   0x00000001
#define EMUUSBTEST_FLAG_IDLE2   0x00000002
#define EMUUSBTEST_FLAG_EP0_OUT 0x00000004
#define EMUUSBTEST_FLAG_BULK    0x00000008
#define EMUUSBTEST_FLAG_INTR    0x00000010
#define EMUUSBTEST_FLAG_IDLE3   0x00000020
#define EMUUSBTEST_FLAG_INTR_IN 0x00000040
#define EMUUSBTEST_FLAG_BULK_IN 0x00000080
#define EMUUSBTEST_FLAG_ISOC_IN 0x00000100
#define EMUUSBTEST_FLAG_ISOC    0x00000200
#define EMUUSBTEST_FLAG_IDLE4   0x00000400
#define EMUUSBTEST_FLAG_IDLE5   0x00000800
#define EMUUSBTEST_FLAG_UVC1    0x00001000
#define EMUUSBTEST_FLAG_UVC2    0x00002000
#define EMUUSBTEST_FLAG_IDLE6   0x00004000
#define EMUUSBTEST_FLAG_IDLE7   0x00008000


#define EMUUSBTEST_FLAG_RESET   0x80000000

#define USB_BULKIN_EP           USB_EP1
#define USB_BULKOUT_EP          USB_EP2
#define USB_INTRIN_EP           USB_EP3
#define USB_INTROUT_EP          USB_EP4


static UINT8   *puiU2BulkBuf,*puiU2BulkBuf_pa;
static UINT8   *puiU2BulkCbwBuf,*puiU2BulkCbwBuf_pa;
static UINT8   *puiU2BulkCswBuf,*puiU2BulkCswBuf_pa;
static UINT32   gU2TestCxTotal, gU2TestCxCurrent;
static UINT8   *puiU2ControlBuf;

ID FLG_ID_USBTEST = 0;

THREAD_HANDLE USBOTEST_TSK1 = 0;
THREAD_HANDLE USBOTEST_TSK2 = 0;


typedef struct {
	UINT32   dCBW_Signature;
	UINT32   dCBW_Tag;
	UINT32   dCBW_DataXferLen;
	UINT8    bCBW_Flag;
	UINT8    bCBW_LUN ;
	UINT8    bCBW_CDBLen ;
	UINT8    CBW_CDB[16];
} U2TESTCBW, *PU2TESTCBW;

typedef struct {
	UINT32   dCSW_Signature;
	UINT32   dCSW_Tag;
	UINT32   dCSW_DataResidue;
	UINT8    bCSW_Status;
} U2TESTCSW, *PU2TESTCSW;



THREAD_DECLARE(emu_usb_test_bulk_thread, arglist)
{
	FLGPTN          uiFlag;
	static UINT32   BulkTestState = 0;
	PU2TESTCBW      pU2Cbw;
	PU2TESTCSW      pU2Csw;
	UINT32          DMALen, AddrOfs = 0;
	ER				Ret;
	UINT8 			*pBulk_rw_buf;

	//kent_tsk();

	BulkTestState = 0;

	pBulk_rw_buf = (UINT8 *)puiU2BulkBuf_pa;

	//coverity[no_escape]
	while (1) {

		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE1);
		wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_BULK | EMUUSBTEST_FLAG_RESET, TWF_ORW | TWF_CLR);
		clr_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE1);

		if (uiFlag & EMUUSBTEST_FLAG_RESET) {
			BulkTestState       = 0;
			AddrOfs             = 0;
		}

		if (uiFlag & EMUUSBTEST_FLAG_BULK) {
			if (BulkTestState == 0) {

				while (1) {
					DMALen = usb2dev_get_ep_bytecount(USB_BULKOUT_EP);
					if (DMALen == 31) {
						break;
					}
					usleep(1000);
				}

				vos_cpu_dcache_sync((VOS_ADDR)puiU2BulkCbwBuf, 31, VOS_DMA_BIDIRECTIONAL);
				Ret = usb2dev_read_endpoint(USB_BULKOUT_EP, (UINT8 *) puiU2BulkCbwBuf_pa, &DMALen);
				vos_cpu_dcache_sync((VOS_ADDR)puiU2BulkCbwBuf, 31, VOS_DMA_BIDIRECTIONAL);

				if ((DMALen != 31)||(Ret != E_OK)) {
					DBG_DUMP("E1\r\n");
				}

				pU2Cbw = (PU2TESTCBW)puiU2BulkCbwBuf;
				pU2Csw = (PU2TESTCSW)puiU2BulkCswBuf;

				/* Check if CBW is legal */
				if (pU2Cbw->dCBW_Signature != 0x43425355) {
					DBG_DUMP("ilegal CBW Sigature\r\n");
				}

				pU2Csw->dCSW_Signature      = 0x53425355;
				pU2Csw->dCSW_Tag            = pU2Cbw->dCBW_Tag;
				pU2Csw->dCSW_DataResidue    = pU2Cbw->dCBW_DataXferLen;
				pU2Csw->bCSW_Status         = 0;//CSW_GOOD

				usb2dev_unmask_ep_interrupt(USB_BULKOUT_EP);
				wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_BULK, TWF_ORW | TWF_CLR);

				DMALen = pU2Csw->dCSW_DataResidue;
				Ret = usb2dev_read_endpoint(USB_BULKOUT_EP, (UINT8 *) pBulk_rw_buf + AddrOfs, &DMALen);

				if ((DMALen != pU2Csw->dCSW_DataResidue)||(Ret != E_OK)) {
					DBG_DUMP("E2\r\n");
				}

				usb2dev_unmask_ep_interrupt(USB_BULKIN_EP);
				wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_BULK_IN, TWF_ORW | TWF_CLR);

				DMALen = 13;
				vos_cpu_dcache_sync((VOS_ADDR)puiU2BulkCswBuf, 13, VOS_DMA_BIDIRECTIONAL);
				Ret = usb2dev_write_endpoint(USB_BULKIN_EP, (UINT8 *) puiU2BulkCswBuf_pa, &DMALen);
				vos_cpu_dcache_sync((VOS_ADDR)puiU2BulkCswBuf, 13, VOS_DMA_BIDIRECTIONAL);

				if ((DMALen != 13)||(Ret != E_OK)) {
					DBG_DUMP("E3\r\n");
				}

				BulkTestState = 1;
			} else if (BulkTestState == 1) {

				while (1) {
					DMALen = usb2dev_get_ep_bytecount(USB_BULKOUT_EP);
					if (DMALen == 31) {
						break;
					}
					usleep(1000);
				}

				vos_cpu_dcache_sync((VOS_ADDR)puiU2BulkCbwBuf, 31, VOS_DMA_BIDIRECTIONAL);
				Ret = usb2dev_read_endpoint(USB_BULKOUT_EP, (UINT8 *) puiU2BulkCbwBuf_pa, &DMALen);
				vos_cpu_dcache_sync((VOS_ADDR)puiU2BulkCbwBuf, 31, VOS_DMA_BIDIRECTIONAL);

				if ((DMALen != 31)||(Ret != E_OK)) {
					DBG_DUMP("E4\r\n");
				}

				pU2Cbw = (PU2TESTCBW)puiU2BulkCbwBuf;
				pU2Csw = (PU2TESTCSW)puiU2BulkCswBuf;

				/* Check if CBW is legal */
				if (pU2Cbw->dCBW_Signature != 0x43425355) {
					DBG_DUMP("ilegal CBW Sigature\r\n");
				}

				pU2Csw->dCSW_Signature      = 0x53425355;
				pU2Csw->dCSW_Tag            = pU2Cbw->dCBW_Tag;
				pU2Csw->dCSW_DataResidue    = pU2Cbw->dCBW_DataXferLen;
				pU2Csw->bCSW_Status         = 0;//CSW_GOOD

				usb2dev_unmask_ep_interrupt(USB_BULKIN_EP);
				wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_BULK_IN, TWF_ORW | TWF_CLR);

				DMALen = pU2Csw->dCSW_DataResidue;
				Ret = usb2dev_write_endpoint(USB_BULKIN_EP, (UINT8 *) pBulk_rw_buf + AddrOfs, &DMALen);

				if ((DMALen != pU2Csw->dCSW_DataResidue)||(Ret != E_OK)) {
					DBG_DUMP("E5\r\n");
				}

				usb2dev_unmask_ep_interrupt(USB_BULKIN_EP);
				wai_flg(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_BULK_IN, TWF_ORW | TWF_CLR);

				DMALen = 13;
				vos_cpu_dcache_sync((VOS_ADDR)puiU2BulkCswBuf, 13, VOS_DMA_BIDIRECTIONAL);
				Ret = usb2dev_write_endpoint(USB_BULKIN_EP, (UINT8 *) puiU2BulkCswBuf_pa, &DMALen);
				vos_cpu_dcache_sync((VOS_ADDR)puiU2BulkCswBuf, 13, VOS_DMA_BIDIRECTIONAL);

				if ((DMALen != 13)||(Ret != E_OK)) {
					DBG_DUMP("E6\r\n");
				}

				BulkTestState = 0;

				AddrOfs += 4;
				AddrOfs &= 1023;
			}

			usb2dev_unmask_ep_interrupt(USB_BULKOUT_EP);
		}


	}
}



THREAD_DECLARE(_usb_test_thread, arglist)
{
	FLGPTN          uiFlag;
	ER				Ret;

	gU2TestCxTotal      = 0;
	gU2TestCxCurrent    = 0;

	//coverity[no_escape]
	while (1) {

		//DBG_DUMP("task1\r\n");
		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE2);
		vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTEST, EMUUSBTEST_FLAG_EP0_OUT | EMUUSBTEST_FLAG_RESET, TWF_ORW | TWF_CLR);
		clr_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE2);
		//DBG_DUMP("task2\r\n");

		if (uiFlag & EMUUSBTEST_FLAG_RESET) {
			gU2TestCxTotal      = 0;
			gU2TestCxCurrent    = 0;
		}

		if (uiFlag & EMUUSBTEST_FLAG_EP0_OUT) {
			UINT32 uiLen;
			if ((gU2TestCxTotal - gU2TestCxCurrent) > EP0_PACKET_SIZE) {
				uiLen = EP0_PACKET_SIZE;
			} else {
				uiLen = (gU2TestCxTotal - gU2TestCxCurrent);
			}

			if (uiLen) {
				Ret = usb2dev_read_endpoint(USB_EP0, puiU2ControlBuf + gU2TestCxCurrent, &uiLen);
				if(Ret != E_OK)
					DBG_ERR("EP0ERR\r\n");

				gU2TestCxCurrent += uiLen;
			}
			if (!(gU2TestCxTotal - gU2TestCxCurrent)) {
				usb2dev_set_ep0_done();
			}
			usb2dev_unmask_ep_interrupt(USB_EP0);
		}
	}

	THREAD_RETURN(0);
}

static void emu_usb_test_open_needed_fifo(void)
{
	//DBG_DUMP("emu_usb_test_open_needed_fifo 1\n");
	usb2dev_unmask_ep_interrupt(USB_EP0);
	usb2dev_unmask_ep_interrupt(USB_BULKOUT_EP);
	usb2dev_unmask_ep_interrupt(USB_INTROUT_EP);
	//DBG_DUMP("emu_usb_test_open_needed_fifo 2\n");
}


static void emu_usb_test_dispatch_event(UINT32 uiEvent)
{
	//DBG_DUMP("emu_usb_test_dispatch_event(%d) 1\n",uiEvent);

	switch (uiEvent) {
	case USB_EVENT_OUT0: {
			usb2dev_mask_ep_interrupt(USB_EP0);
			set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_EP0_OUT);
		}
		break;
	case USB_EVENT_EP2_RX: {
			//DBG_DUMP("USB_EVENT_EP2_RX\n");
			usb2dev_mask_ep_interrupt(USB_BULKOUT_EP);
			set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_BULK);
		}
		break;
	case USB_EVENT_EP4_RX: {
			//DBG_DUMP("USB_EVENT_EP4_RX\n");
			usb2dev_mask_ep_interrupt(USB_INTROUT_EP);
			set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_INTR);
		}
		break;


	case USB_EVENT_EP1_TX: {
			usb2dev_mask_ep_interrupt(USB_BULKIN_EP);
			set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_BULK_IN);
		}
		break;
	case USB_EVENT_EP3_TX: {
			usb2dev_mask_ep_interrupt(USB_INTRIN_EP);
			set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_INTR_IN);
		}
		break;


	case USB_EVENT_CLRFEATURE:
		DBG_DUMP("USB_EVENT_CLRFEATURE\r\n");
		break;
	case USB_EVENT_RESET:
		DBG_DUMP("USB_EVENT_RESET\r\n");
		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_RESET);
		break;
	case USB_EVENT_SUSPEND:
		DBG_DUMP("USB_EVENT_SUSPEND\r\n");
		set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_RESET);
		break;
	case USB_EVENT_RESUME:
		DBG_DUMP("USB_EVENT_RESUME\r\n");
		break;
	default:
		//debug_msg("%s: evt not support: 0x%lx\r\n", __func__, uiEvent);
		break;
	}

	//DBG_DUMP("emu_usb_test_dispatch_event(%d) 2\n",uiEvent);

}

static void emu_usb_test_control_vendor_request(UINT32 uiRequest)
{
	if (uiRequest != 0) {
		usb2dev_set_ep_stall(USB_EP0);
		return;
	}
	if (usb2dev_control_data.device_request.bm_request_type & 0x80) {
		//DBG_DUMP("CI %d\n", usb2dev_control_data.device_request.w_length);
		/* CX IN */
		usb2dev_control_data.w_length  = usb2dev_control_data.device_request.w_length;
		usb2dev_control_data.p_data    = puiU2ControlBuf;
		usb2dev_reture_setup_data();

	} else {
		//DBG_DUMP("CO %d\n", usb2dev_control_data.device_request.w_length);
		/* CX OUT */
		gU2TestCxTotal   = usb2dev_control_data.device_request.w_length;
		gU2TestCxCurrent = 0;
	}
}


#define POOL_SIZE_USER_DEFINIED  0x1000000

static UINT32 user_va = 0;
static UINT32 user_pa = 0;
static HD_COMMON_MEM_VB_BLK blk;
static int mem_init(void)
{
        HD_RESULT                 ret;
        HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

		 hd_common_init(0);

        mem_cfg.pool_info[0].type = HD_COMMON_MEM_USER_DEFINIED_POOL;
        mem_cfg.pool_info[0].blk_size = POOL_SIZE_USER_DEFINIED;
        mem_cfg.pool_info[0].blk_cnt = 1;
        mem_cfg.pool_info[0].ddr_id = DDR_ID0;
        ret = hd_common_mem_init(&mem_cfg);
        if (HD_OK != ret) {
                printf("hd_common_mem_init err: %d\r\n", ret);
                return ret;
        }
        blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_DEFINIED_POOL, POOL_SIZE_USER_DEFINIED, DDR_ID0);
        if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
                DBG_ERR("hd_common_mem_get_block fail\r\n");
                return HD_ERR_NG;
        }
        user_pa = hd_common_mem_blk2pa(blk);
        if (user_pa == 0) {
                DBG_ERR("not get buffer, pa=%08x\r\n", (int)user_pa);
                return HD_ERR_NG;
        }
        user_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, user_pa, POOL_SIZE_USER_DEFINIED);
        /* Release buffer */
        if (user_va == 0) {
                DBG_ERR("mem map fail\r\n");
                ret = hd_common_mem_munmap((void *)user_va, POOL_SIZE_USER_DEFINIED);
                if (ret != HD_OK) {
                        DBG_ERR("mem unmap fail\r\n");
                        return ret;
                }
                return HD_ERR_NG;
        }

		DBG_DUMP("user_pa=0x%08X user_pa=0x%08X\r\n",user_va,user_pa);

        return HD_OK;
}

int main(void)
{
	USB_MNG	gUSBMng;
	UINT32 i;

	mem_init();

	puiU2BulkBuf 		= (UINT8* )user_va;
	puiU2BulkBuf_pa		= (UINT8* )user_pa;
	puiU2BulkCbwBuf 	= (UINT8 *)(user_va+(300*1024));
	puiU2BulkCbwBuf_pa 	= (UINT8 *)(user_pa+(300*1024));
	puiU2BulkCswBuf 	= (UINT8 *)(user_va+(301*1024));
	puiU2BulkCswBuf_pa 	= (UINT8 *)(user_pa+(301*1024));
	puiU2ControlBuf = (UINT8 *)(user_va+(350*1024));//16kb

	vos_flag_create(&FLG_ID_USBTEST, NULL,	"FLG_ID_USBTEST");

	usb2dev_set_callback(USB_CALLBACK_CX_VENDOR_REQUEST, (USB_GENERIC_CB)emu_usb_test_control_vendor_request);

	usb2dev_init_management(&gUSBMng);

	gUSBMng.p_dev_desc            = (USB_DEVICE_DESC *)&gU2HsTestDevDesc;

	gUSBMng.p_config_desc_hs       = (USB_CONFIG_DESC *)&gU2UvcHsTestCfgDesc;
	gUSBMng.p_config_desc_fs       = (USB_CONFIG_DESC *)&gU2FsTestCfgDesc;
	gUSBMng.p_config_desc_fs_other  = (USB_CONFIG_DESC *)&gU2UvcHsTestCfgDesc;
	gUSBMng.p_config_desc_hs_other  = (USB_CONFIG_DESC *)&gU2FsTestCfgDesc;

	gUSBMng.p_dev_quali_desc       = (USB_DEVICE_DESC *)&gU2devQualiDesc;
	gUSBMng.num_of_configurations  = 1;
	gUSBMng.num_of_strings         = 5;
	gUSBMng.p_string_desc[0]      = (USB_STRING_DESC *)&gU2StrDesc0;
	gUSBMng.p_string_desc[1]      = (USB_STRING_DESC *)&gU2StrDesc1;
	gUSBMng.p_string_desc[2]      = (USB_STRING_DESC *)&gU2StrDesc2;
	gUSBMng.p_string_desc[3]      = (USB_STRING_DESC *)&gU2StrDesc3;
	gUSBMng.p_string_desc[4]      = (USB_STRING_DESC *)&gU2StrDesc4;

	for (i = 0; i < 15; i++) {
		gUSBMng.ep_config_hs[i].enable      = FALSE;
		gUSBMng.ep_config_fs[i].enable      = FALSE;
	}

	// Config High Speed endpoint usage.
	gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].enable          = TRUE;
	gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].blk_size         = 512;
	gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].blk_num          = BLKNUM_DOUBLE;
	gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].direction       = EP_DIR_IN;
	gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].trnsfer_type     = EP_TYPE_BULK;
	gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].max_pkt_size      = 512;
	gUSBMng.ep_config_hs[USB_BULKIN_EP - 1].high_bandwidth   = HBW_NOT;

	gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].enable          = TRUE;
	gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].blk_size         = 512;
	gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].blk_num          = BLKNUM_DOUBLE;
	gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].direction       = EP_DIR_OUT;
	gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].trnsfer_type     = EP_TYPE_BULK;
	gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].max_pkt_size      = 512;
	gUSBMng.ep_config_hs[USB_BULKOUT_EP - 1].high_bandwidth   = HBW_NOT;

	gUSBMng.ep_config_hs[USB_INTRIN_EP - 1].enable          = TRUE;
	gUSBMng.ep_config_hs[USB_INTRIN_EP - 1].blk_size         = 1024;
	gUSBMng.ep_config_hs[USB_INTRIN_EP - 1].blk_num          = BLKNUM_SINGLE;
	gUSBMng.ep_config_hs[USB_INTRIN_EP - 1].direction       = EP_DIR_IN;
	gUSBMng.ep_config_hs[USB_INTRIN_EP - 1].trnsfer_type     = EP_TYPE_INTERRUPT;
	gUSBMng.ep_config_hs[USB_INTRIN_EP - 1].max_pkt_size      = 1024;
	gUSBMng.ep_config_hs[USB_INTRIN_EP - 1].high_bandwidth   = HBW_NOT;

	gUSBMng.ep_config_hs[USB_INTROUT_EP - 1].enable          = TRUE;
	gUSBMng.ep_config_hs[USB_INTROUT_EP - 1].blk_size         = 1024;
	gUSBMng.ep_config_hs[USB_INTROUT_EP - 1].blk_num          = BLKNUM_SINGLE;
	gUSBMng.ep_config_hs[USB_INTROUT_EP - 1].direction       = EP_DIR_OUT;
	gUSBMng.ep_config_hs[USB_INTROUT_EP - 1].trnsfer_type     = EP_TYPE_INTERRUPT;
	gUSBMng.ep_config_hs[USB_INTROUT_EP - 1].max_pkt_size      = 1024;
	gUSBMng.ep_config_hs[USB_INTROUT_EP - 1].high_bandwidth   = HBW_NOT;


	// Config Full Speed endpoint usage.
	gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].enable          = TRUE;
	gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].blk_size         = 512;
	gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].blk_num          = BLKNUM_DOUBLE;
	gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].direction       = EP_DIR_IN;
	gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].trnsfer_type     = EP_TYPE_BULK;
	gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].max_pkt_size      = 64;
	gUSBMng.ep_config_fs[USB_BULKIN_EP - 1].high_bandwidth   = HBW_NOT;

	gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].enable          = TRUE;
	gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].blk_size         = 512;
	gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].blk_num          = BLKNUM_DOUBLE;
	gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].direction       = EP_DIR_OUT;
	gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].trnsfer_type     = EP_TYPE_BULK;
	gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].max_pkt_size      = 64;
	gUSBMng.ep_config_fs[USB_BULKOUT_EP - 1].high_bandwidth   = HBW_NOT;

	gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].enable          = TRUE;
	gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].blk_size         = 512;
	gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].blk_num          = BLKNUM_SINGLE;
	gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].direction       = EP_DIR_IN;
	gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].trnsfer_type     = EP_TYPE_INTERRUPT;
	gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].max_pkt_size      = 64;
	gUSBMng.ep_config_fs[USB_INTRIN_EP - 1].high_bandwidth   = HBW_NOT;

	gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].enable          = TRUE;
	gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].blk_size         = 512;
	gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].blk_num          = BLKNUM_SINGLE;
	gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].direction       = EP_DIR_OUT;
	gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].trnsfer_type     = EP_TYPE_INTERRUPT;
	gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].max_pkt_size      = 64;
	gUSBMng.ep_config_fs[USB_INTROUT_EP - 1].high_bandwidth   = HBW_NOT;

	gUSBMng.usb_type                         = USB_TEST;
	gUSBMng.fp_open_needed_fifo              = emu_usb_test_open_needed_fifo;
	gUSBMng.fp_event_callback                = emu_usb_test_dispatch_event;

	clr_flg(FLG_ID_USBTEST, 0xFFFFFFFF);
	set_flg(FLG_ID_USBTEST, EMUUSBTEST_FLAG_IDLE1 | EMUUSBTEST_FLAG_IDLE2 | EMUUSBTEST_FLAG_IDLE3 | EMUUSBTEST_FLAG_IDLE4 |
			EMUUSBTEST_FLAG_IDLE5 | EMUUSBTEST_FLAG_IDLE6 | EMUUSBTEST_FLAG_IDLE7);


	THREAD_CREATE(USBOTEST_TSK1, _usb_test_thread, NULL, "USBOTEST_TSK1");
	THREAD_RESUME(USBOTEST_TSK1);
	THREAD_CREATE(USBOTEST_TSK2, emu_usb_test_bulk_thread, NULL, "USBOTEST_TSK2");
	THREAD_RESUME(USBOTEST_TSK2);
	//USBOTEST_TSK1 = vos_task_create(emu_usb_test_cx_out_thread,  0, "USBOTEST_TSK1",   20,	1024);
	//vos_task_resume(USBOTEST_TSK1);




	//emu_usb_test_open
	usb2dev_set_management(&gUSBMng);

	usb2dev_open();

	while(1)
	{
		usleep(10000000);
	}
	usleep(20000000);
	usb2dev_close();

	return 0;
}
