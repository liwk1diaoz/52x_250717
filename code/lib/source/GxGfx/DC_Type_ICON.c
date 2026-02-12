
#include "GxGfx/GxGfx.h"
#include "GxGfx_int.h"
#include "DC.h"
#include "Chunk.h"

//NOTE : single format - all image format are the same, and record in table header
//NOTE : multiple format - each image has varied format, and record in image info

static UINT32 _ICON_FMT(UINT32 fmt, UINT32 bpp)
{
	UINT32 Pxlfmt = 0;
	switch (fmt) {
	case GXFM_IMAGETABLE_INDEX1:
	case GXFM_IMAGETABLE_INDEX2:
	case GXFM_IMAGETABLE_INDEX4:
	case GXFM_IMAGETABLE_INDEX8:
		switch (bpp) {
		case 1:
			Pxlfmt = PXLFMT_INDEX1;
			break;
		case 2:
			Pxlfmt = PXLFMT_INDEX2;
			break;
		case 4:
			Pxlfmt = PXLFMT_INDEX4;
			break;
		case 8:
			Pxlfmt = PXLFMT_INDEX8;
			break;
		}
		break;
	case GXFM_IMAGETABLE_RGB565:
		Pxlfmt = PXLFMT_RGB565_PK;
		break;
	case GXFM_IMAGETABLE_RGBA5654:
		Pxlfmt = PXLFMT_RGBA5654_PK;
		break;
	case GXFM_IMAGETABLE_RGBA5658:
		Pxlfmt = PXLFMT_RGBA5658_PK;
		break;
#if (GX_SUPPPORT_YUV ==ENABLE)
#if (GX_SUPPORT_DCFMT_UVPACK)
	case GXFM_IMAGETABLE_YUV422:
		Pxlfmt = PXLFMT_YUV422_PK;
		break;
	case GXFM_IMAGETABLE_YUV420:
		Pxlfmt = PXLFMT_YUV420_PK;
		break;
#else
	case GXFM_IMAGETABLE_YUV444:
		Pxlfmt = PXLFMT_YUV444;
		break;
	case GXFM_IMAGETABLE_YUV422:
		Pxlfmt = PXLFMT_YUV422;
		break;
	case GXFM_IMAGETABLE_YUV420:
		Pxlfmt = PXLFMT_YUV420;
		break;
#endif
#endif
	case GXFM_IMAGETABLE_A4:
		Pxlfmt = PXLFMT_A4;
		break;
	case GXFM_IMAGETABLE_A8:
		Pxlfmt = PXLFMT_A8;
		break;
	case GXFM_IMAGETABLE_RGBA8888:
		Pxlfmt = PXLFMT_RGBA8888_PK;
		break;
	case GXFM_IMAGETABLE_RGBA4444:
		Pxlfmt = PXLFMT_RGBA4444_PK;
		break;
	case GXFM_IMAGETABLE_RGBA5551:
		Pxlfmt = PXLFMT_RGBA5551_PK;
		break;
	default :
		DBG_ERR("_ICON_FMT - ERROR! Image format is invalid.\r\n");
		return 0;
	}
	return Pxlfmt;
}

//condition : pImageInfoEx != null
//condition : pTable != null
//condition : id is in the range of table
RESULT _ICON_GetInfo(const IMAGE_TABLE *_pTable, IVALUE id, IMAGE_HEADER *pImageHeader)
{
	const GX_IMAGETABLE *pTable = (const GX_IMAGETABLE *)_pTable;
	const GX_TABLE *pChunkTable;
	const CHUNK *pChunk;
	IVALUE iid;
	UINT32 w, h, fmt = 0;
	UINT8 bpp = 0;

	if (!pTable) {
		DBG_ERR("_ICON_GetInfo - ERROR! Image table is null.\r\n");
		return GX_NO_IMAGETABLE;
	}
	if (!pImageHeader) {
		DBG_ERR("_ICON_GetInfo - ERROR! Info pointer is null.\r\n");
		return GX_NULL_POINTER;
	}

	if (pTable->format != 0) { //single format
		fmt = pTable->format;
		bpp = pTable->bpp;
	}

	pChunkTable = (const GX_TABLE *)(pTable + 1);
	if ((id < pChunkTable->start) || (id >= pChunkTable->start + pChunkTable->count)) {
		DBG_ERR("_ICON_GetInfo - ERROR! ID 0x%X is of of range.\r\n", id);
		return GX_OUTOF_IMAGEID;
	}

	iid = id - pChunkTable->start;
	pChunk = ((const CHUNK *)(pChunkTable + 1)) + iid;
	if ((pChunk->offset == 0xFFFFFFFF) || (pChunk->size == 0)) {
		DBG_ERR("_ICON_GetInfo - ERROR! ID 0x%X is not valid.\r\n", id);
		return GX_INVALID_IMAGEID;
	}

	if (pTable->format != 0) { //single format
		const IMAGE_INFO *pImageInfo;
		pImageInfo = (const IMAGE_INFO *)(((const UINT8 *)pChunkTable) + (pChunk->offset));
		w = pImageInfo->w;
		h = pImageInfo->h;
		pImageHeader->offset = (UINT32)(pImageInfo + 1);
		pImageHeader->size = (pChunk->size) - sizeof(IMAGE_INFO);
	} else { //multiple format
		const IMAGE_INFO_EX *pImageInfo;
		pImageInfo = (const IMAGE_INFO_EX *)(((const UINT8 *)pChunkTable) + (pChunk->offset));
		w = pImageInfo->w;
		h = pImageInfo->h;
		fmt = pImageInfo->format;
		bpp = pImageInfo->bpp;
		pImageHeader->offset = (UINT32)(pImageInfo + 1);
		pImageHeader->size = (pChunk->size) - sizeof(IMAGE_INFO_EX);
	}

	pImageHeader->w = w;
	pImageHeader->h = h;
	pImageHeader->format = _ICON_FMT(fmt, bpp);
	pImageHeader->bpp = bpp;

	return GX_OK;
}

//condition : pTable != null
//condition : id is in the range of table
RESULT _ICON_GetSize(const IMAGE_TABLE *_pTable, IVALUE id, ISIZE *pSize)
{
	const GX_IMAGETABLE *pTable = (const GX_IMAGETABLE *)_pTable;
	const GX_TABLE *pChunkTable;
	const CHUNK *pChunk;
	IVALUE iid;

	if (!pTable) {
		DBG_ERR("_ICON_GetSize - ERROR! Image table is null.\r\n");
		return GX_NO_IMAGETABLE;
	}
	if (!pSize) {
		DBG_ERR("_ICON_GetSize - ERROR! Output Size pointer is null.\r\n");
		return GX_NULL_POINTER;
	}

	pChunkTable = (const GX_TABLE *)(pTable + 1);
	if ((id < pChunkTable->start) || (id >= pChunkTable->start + pChunkTable->count)) {
		DBG_ERR("_ICON_GetSize - ERROR! ID 0x%X is of of range.\r\n", id);
		return GX_OUTOF_IMAGEID;
	}

	iid = id - pChunkTable->start;
	pChunk = ((const CHUNK *)(pChunkTable + 1)) + iid;
	if ((pChunk->offset == 0xFFFFFFFF) || (pChunk->size == 0)) {
		DBG_ERR("_ICON_GetSize - ERROR! ID 0x%X is not valid.\r\n", id);
		return GX_OUTOF_IMAGEID;
	}

	if (pTable->format != 0) { //single format
		const IMAGE_INFO *pImageInfo;
		pImageInfo = (const IMAGE_INFO *)(((const UINT8 *)pChunkTable) + (pChunk->offset));
		pSize->w = pImageInfo->w;
		pSize->h = pImageInfo->h;
	} else { //multiple format
		const IMAGE_INFO_EX *pImageInfo;
		pImageInfo = (const IMAGE_INFO_EX *)(((const UINT8 *)pChunkTable) + (pChunk->offset));
		pSize->w = pImageInfo->w;
		pSize->h = pImageInfo->h;
	}

	return GX_OK;
}

//condition : pIconDC != null
//condition : pTable != null
//condition : id is in the range of table
RESULT _ICON_MakeDC(DC *pIconDC, const IMAGE_TABLE *_pTable, IVALUE id)
{
	//const GX_IMAGETABLE* pTable = (const GX_IMAGETABLE*)_pTable;
	IMAGE_HEADER header;
	RESULT r;
	UINT32 uiFmtCase = 0;

	if (!pIconDC) {
		DBG_ERR("_ICON_MakeDC - ERROR! DC is null.\r\n");
		return GX_NULL_POINTER;
	}

	r = _ICON_GetInfo(_pTable, id, &header);
	if (r != GX_OK) {
		return r;
	}

	switch (header.format) {
	case PXLFMT_INDEX1:
	case PXLFMT_INDEX2:
	case PXLFMT_INDEX4:
	case PXLFMT_INDEX8:
		uiFmtCase = 1;
		break;
	case PXLFMT_RGB565_PK:
	case PXLFMT_RGBA5654_PK:
	case PXLFMT_RGBA5658_PK:
	case PXLFMT_RGBA8888_PK:
	case PXLFMT_RGBA4444_PK:
	case PXLFMT_RGBA5551_PK:
		uiFmtCase = 2;
		break;
#if (GX_SUPPPORT_YUV ==ENABLE)
	case PXLFMT_YUV444 :
	case PXLFMT_YUV422 :
	case PXLFMT_YUV420 :
#if (GX_SUPPORT_DCFMT_UVPACK)
	case PXLFMT_YUV422_PK:
	case PXLFMT_YUV421_PK:
	case PXLFMT_YUV420_PK:
#endif
		uiFmtCase = 3;
		break;
#endif
	}

	if (uiFmtCase == 1) {

		//create resouce DC
		return GxGfx_AttachDC(pIconDC,
							  TYPE_ICON, (UINT16)header.format,
							  header.w, header.h,
							  0, //auto pitch
							  (UINT8 *)header.offset, 0, 0);
	} else if (uiFmtCase == 2) {
		//create resouce DC
		return GxGfx_AttachDC(pIconDC,
							  TYPE_ICON, (UINT16)header.format,
							  header.w, header.h,
							  0, //auto pitch
							  (UINT8 *)header.offset, 0, 0);
	}
#if (GX_SUPPPORT_YUV ==ENABLE)
	else if (uiFmtCase == 3) {
		//create resouce DC
		return GxGfx_AttachDC(pIconDC,
							  TYPE_JPEG, (UINT16)header.format,
							  header.w, header.h,
							  header.size, //JPG bitstream size
							  (UINT8 *)header.offset, 0, 0); //JPG bitstream data
	}
#endif
	return GX_ERROR_FORMAT;
}

