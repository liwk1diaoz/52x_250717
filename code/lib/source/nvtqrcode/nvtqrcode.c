#include <sys/types.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zbar.h>
#include "nvtqrcode/nvtqrcode.h"
#include <kwrap/task.h>
#include <kwrap/type.h>
#include <kwrap/util.h>
#include <kwrap/nvt_type.h>

zbar_image_scanner_t *scanner = NULL;

ER nvtqrcode_scan(NVTQRCODE_INFO *pQrCodeInfo)
{
	int width = 0, height = 0;
	void *raw = NULL;
	zbar_image_t *image;
    UINT32 va;
    HD_VIDEO_FRAME *pVdoFrm;
    void   *pQrcodeBuf;
    UINT32 u32QrcodeBufSize;

    if (!pQrCodeInfo) {
        DBG_ERR("pQrCodeInfo is NULL\r\n");
        return E_PAR;
    }

    if (!pQrCodeInfo->pVdoFrm) {
        DBG_ERR("pQrCodeInfo->pVdoFrm is NULL\r\n");
        return E_PAR;
    }

    if (!pQrCodeInfo->pQrcodeBuf) {
        DBG_ERR("pQrCodeInfo->pQrcodeBuf is NULL\r\n");
        return E_PAR;
    }

    if (pQrCodeInfo->u32QrcodeBufSize == 0) {
        DBG_ERR("pQrCodeInfo->u32QrcodeBufSize is zero\r\n");
        return E_PAR;
    }

    pVdoFrm = pQrCodeInfo->pVdoFrm;
    pQrcodeBuf = pQrCodeInfo->pQrcodeBuf;
    u32QrcodeBufSize = pQrCodeInfo->u32QrcodeBufSize;

	//get video frame information
	va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pVdoFrm->phy_addr[0], pVdoFrm->loff[0]*pVdoFrm->ph[0]);

	if(va == 0) {
        DBG_ERR("Failed from PA to VA \r\n");
        return E_PAR;
	}

    if (pQrCodeInfo->bEnDbgMsg) {
        DBG_DUMP("%s: va = 0x%x, pVdoFrm->phy_addr[0] = 0x%x\r\n",__func__, va, pVdoFrm->phy_addr[0]);
    	DBG_DUMP("%s: pImg->loff[0], pImg->pw[0]=%d, pImg->ph[0]=%d\r\n",__func__, pVdoFrm->loff[0], pVdoFrm->pw[0], pVdoFrm->ph[0]);
    }

    raw = (void *)va;
    width  = pVdoFrm->loff[0];
	height = pVdoFrm->ph[0];

	/* create a reader */
	scanner = zbar_image_scanner_create();

	/* configure the reader */
	zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);

	/* wrap image data */
	image = zbar_image_create();
	zbar_image_set_format(image, *(int*)"Y800");
	zbar_image_set_size(image, width, height);
	/* data would be free in "zbar_image_free_data" */
	zbar_image_set_data(image, raw, width * height, zbar_image_free_data);

	/* scan the image for barcodes */
	int n = zbar_scan_image(scanner, image);

	if(!n) {
        hd_common_mem_munmap((void *)va, pVdoFrm->loff[0]*pVdoFrm->ph[0]);
        return E_PAR;
	}

	/* extract results */
	const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
	for(; symbol; symbol = zbar_symbol_next(symbol)) {
		/* do something useful with results */
		zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
		const char *data = zbar_symbol_get_data(symbol);
		DBG_DUMP("decoded %s symbol \"%s\"\n",
		zbar_get_symbol_name(typ), data);

		strncpy(pQrcodeBuf, data, u32QrcodeBufSize);
    }

    hd_common_mem_munmap((void *)va, pVdoFrm->loff[0]*pVdoFrm->ph[0]);

	return E_OK;
}
