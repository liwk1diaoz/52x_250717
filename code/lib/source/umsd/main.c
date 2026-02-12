#include <kwrap/nvt_type.h>
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <kwrap/debug.h>
#include <kwrap/cpu.h>
#include "usb2dev.h"
#include "umsd.h"
#include "sdio.h"
#include <unistd.h>
#include <hdal.h>


static _ALIGNED(4) const UINT16 myUSBManuStrDesc[] = {
	0x0310,                             // 10: size of String Descriptor = 16 bytes
	// 03: String Descriptor type
	'N', 'O', 'V', 'A', 'T', 'E', 'K'   // NOVATEK
};

static _ALIGNED(4) const UINT16 myUSBMSDCProdStrDesc[] = {
	0x0336,                             // 36: size of String Descriptor = 54 bytes
	// 03: String Descriptor type
	'U', 'S', 'B', ' ', 'M', 'a',       // USB Mass Storage
	's', 's', ' ', 'S', 't', 'o',
	'r', 'a', 'g', 'e', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' '
};

//Serial number string descriptor, the content should be updated according to serial number
static _ALIGNED(4) UINT16 myUSBSerialStrDesc3[] = {
	0x0320,                             // 20: size of String Descriptor = 32 bytes
	// 03: String Descriptor type
	'9', '6', '6', '5', '0',            // 96650-00000-001 (default)
	'0', '0', '0', '0', '0',
	'0', '0', '1', '0', '0'
};

static _ALIGNED(64) UINT8 InquiryData[36] = {
	0x00, 0x80, 0x05, 0x02, 0x20, 0x00, 0x00, 0x00,
//    //Vendor identification, PREMIER
	'N', 'O', 'V', 'A', 'T', 'E', 'K', '-',
//    //product identification, DC8365
	'N', 'T', '9', '6', '6', '8', '0', '-',
	'D', 'S', 'P', ' ', 'I', 'N', 'S', 'I',
//    //product revision level, 1.00
	'D', 'E', ' ', ' '
};

 BOOL emuusb_msdc_strg_detect(void)
 {
	return 1;
 }

static void emu_usbMakeMsdcInfo(USB_MSDC_INFO *pUSBMSDCInfo)
{
	pUSBMSDCInfo->pManuStringDesc = (USB_STRING_DESC *)myUSBManuStrDesc;
	pUSBMSDCInfo->pProdStringDesc = (USB_STRING_DESC *)myUSBMSDCProdStrDesc;
	pUSBMSDCInfo->pSerialStringDesc = (USB_STRING_DESC *)myUSBSerialStrDesc3;

	pUSBMSDCInfo->VID = 0x07B4;
	pUSBMSDCInfo->PID = 0x0109;

	pUSBMSDCInfo->pInquiryData = (UINT8 *)&InquiryData[0];
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
	//UINT32 total_sectors = 0;
	STORAGE_OBJ *pStrg = sdio_getStorageObject(STRG_OBJ_FAT1);
	PMSDC_OBJ p_msdc_object;
	USB_MSDC_INFO       MSDCInfo;
	UINT32 i;


	mem_init();

	while(1) {

	if (pStrg == NULL) {
		printf("failed to get STRG_OBJ_FAT1.n");
		return -1;
	}
	//pStrg->Open();
	//pStrg->GetParam(STRG_GET_DEVICE_PHY_SECTORS, (UINT32)&total_sectors, 0);
	//printf("sd size = %lld bytes\n", (unsigned long long)total_sectors * 512);


	p_msdc_object = Msdc_getObject(MSDC_ID_USB20);
	p_msdc_object->SetConfig(USBMSDC_CONFIG_ID_SELECT_POWER,  USBMSDC_POW_SELFPOWER);

	MSDCInfo.uiMsdcBufAddr_va= (UINT32)user_va;
	MSDCInfo.uiMsdcBufAddr_pa= (UINT32)user_pa;
	if (MSDCInfo.uiMsdcBufAddr_pa == 0) {
		DBG_ERR("malloc buffer failed\n");
	}
	MSDCInfo.uiMsdcBufSize = MSDC_MIN_BUFFER_SIZE;
	emu_usbMakeMsdcInfo(&MSDCInfo);

	MSDCInfo.msdc_check_cb = NULL;
	MSDCInfo.msdc_vendor_cb = NULL;

	MSDCInfo.msdc_RW_Led_CB = NULL;
	MSDCInfo.msdc_StopUnit_Led_CB = NULL;
	MSDCInfo.msdc_UsbSuspend_Led_CB = NULL;

	for (i = 0; i < MAX_LUN; i++) {
		MSDCInfo.msdc_type[i] = MSDC_STRG;
	}

	//pSDIOObj = sdio_getStorageObject(STRG_OBJ_FAT1);
	//pSDIOObj->Close();
	//pSDIOObj->SetParam(STRG_SET_MEMORY_REGION, (UINT32)malloc(1024), 1024);
	MSDCInfo.pStrgHandle[0] = pStrg;//pSDIOObj
	MSDCInfo.msdc_strgLock_detCB[0] = NULL;
	MSDCInfo.msdc_storage_detCB[0] = emuusb_msdc_strg_detect;
	MSDCInfo.LUNs = 1;



		if (p_msdc_object->Open(&MSDCInfo) != E_OK) {
			DBG_ERR("msdc open failed\r\n ");
		}

		usleep(12000000);


		p_msdc_object->Close();
		usleep(2000000);

	}

	return 0;
}
