#include <linux/sched.h>
//#include "jpgheader.h"
#include "jpeg_file.h"
#include "jpeg_emu.h"
//#include "jpeg_enc.h"
#include "../include/jpeg.h"
#include <mach/fmem.h>
//#include "imgtrans/grph.h"
#include "../../kdrv_gfx2d/kdrv_grph/include/grph_compatible.h"
#include "../include/jpg_enc.h"
#include "../include/jpg_header.h"
#include "hal/jpg_int.h"

#include "kdrv_type.h"
#include "kdrv_videoenc/kdrv_videoenc.h"
#include "kdrv_videodec/kdrv_videodec.h"


#define KDRV_JPG  1

// Test mode string
static CHAR *pEncModeString[] = {
	"",
	"Normal",
	"Restart Marker",
	"Transform",
	"Rotate 90",
	"Rotate 180",
	"Rotate 270",
	"",
	"DIS"
};

// What mode will be tested
static UINT32  uiEncTestMode[] = {
	EMU_JPEG_ENCMODE_NORMAL,
	//EMU_JPEG_ENCMODE_RESTART,
	//EMU_JPEG_ENCMODE_TRANSFORM,
	//EMU_JPEG_ENCMODE_DIS
};

// Design-In normal, restart
static UINT32 uiFmtDesInEncOthers[] = {
	//EMU_JPEG_MCU_100,
	//EMU_JPEG_MCU_2h11,
	EMU_JPEG_MCU_411
};

// Design-In transform format
static UINT32 uiFmtDesInTransform[] = {
	EMU_JPEG_MCU_411
};

// Design-In DIS format
static UINT32 uiFmtDesInDIS[] = {
	EMU_JPEG_MCU_2h11,
	EMU_JPEG_MCU_411
};


static CHAR *pDecModeString[] = {
	"",
	"Normal",
	"Scale 1/8",
	"Scale 1/4",
	"Scale 1/2",
	"Crop",
	"Crop + Scale 1/8",
	"Crop + Scale 1/4",
	"Crop + Scale 1/2",
	"Rotate 90",
	"Rotate 180",
	"Rotate 270",
	"Rotate 90 + Scale 1/8",
	"Rotate 180 + Scale 1/4",
	"Rotate 270 + Scale 1/2",
	"Scale 1/2 [Width only]",
	"Restart Marker"
};


// What decode mode will be tested
static UINT32  uiDecTestMode[] = {
	EMU_JPEG_DECMODE_NORMAL,
	//EMU_JPEG_DECMODE_SCALE1D8,
//	EMU_JPEG_DECMODE_SCALE1D4,
//	EMU_JPEG_DECMODE_SCALE1D2,
//	EMU_JPEG_DECMODE_CROP,
//	EMU_JPEG_DECMODE_CROP_S1D8,
//	EMU_JPEG_DECMODE_CROP_S1D4,
//	EMU_JPEG_DECMODE_CROP_S1D2,
//	EMU_JPEG_DECMODE_RESTART,
//	EMU_JPEG_DECMODE_SCALE1D2_W,
};


// Design-In normal, scale, crop format
static UINT32 uiFmtDesInDecOthers[] = {
//	EMU_JPEG_MCU_100,
//	EMU_JPEG_MCU_2h11,
	EMU_JPEG_MCU_411
};

// Design-In scale 1/2 width only format
static UINT32 uiFmtDesInScale1D2W[] = {
	EMU_JPEG_MCU_2h11,
	EMU_JPEG_MCU_411
};


// Encode register set
static PEMU_JPEG_DEC_REG    pDecRegSet;

// Buffer assignment
static EMU_JPEG_DEC_BUFFER  DecBuf;

// JPEG header
static JPGHEAD_DEC_CFG      JpegCfg;

// ===================================================================
// Test mode, format...
// ===================================================================
// MCU format
static UINT32               uiDecFormat;

// Test mode
static UINT32               uiDecMode;

// Test pattern
static UINT32               uiDecPtn;


// ===================================================================
// Buffer offset
// ===================================================================
// Bit stream buffer byte offset
// Modify this value after each operation
static UINT32               uiDecBSBufOffset = 0;

// For decoded Y,UV buffer, word offset (0x0 ~ 0xC, word alignment)
// Modify this value after each operation
static UINT32               uiDecWordOffset = 0;

// ===================================================================
// Width, Height and Line offset
// ===================================================================
// Decoded Y width & height
static UINT32               uiDecYWidth, uiDecYHeight;

// Decded UV packed width & height
static UINT32               uiDecUVWidth, uiDecUVHeight;

// Decoded Y and UV line offset
static UINT32               uiDecYLineOffset, uiDecUVLineOffset;

// Extra line offset for decoded Y and UV packed (word alignment)
// Only for non-scaling mode
static UINT32               uiDecExtraLineOffset = 0;


// Encode register set
static PEMU_JPEG_ENC_REG    pEncRegSet;

// Buffer assignment
static EMU_JPEG_ENC_BUFFER  EncBuf;

// ===================================================================
// Test mode, format...
// ===================================================================
// DC out mode
static EMU_JPEG_DC_MODE     DCOutMode;

// DC out configuration
static JPEG_DC_OUT_CFG      DCOutCfg;


// MCU format
static UINT32               uiEncFormat;

// Test mode
static UINT32               uiEncMode;

// Test pattern
static UINT32               uiEncPtn;

// ===================================================================
// Buffer offset
// ===================================================================
// Bit stream buffer byte offset
// Modify this value after each operation
static UINT32               uiEncBSBufOffset = 0;

// Y source buffer byte offset
// UV source buffer offset = Y offset / 2 in DIS mode
//                         = Y offset in others mode
// Modify this value after each operation
static UINT32               uiEncYBufOffset = 0;

// For DC out buffer, word offset (0x0 ~ 0xC, word alignment)
// Modify this value after each operation
static UINT32               uiEncWordOffset = 0;


// ===================================================================
// Line offset
// ===================================================================
// Extra line offset for Y and UV packed source (word alignment)
static UINT32               uiEncExtraLineOffset = 0;

// ===================================================================
// file name
// ===================================================================
#define EMU_JPEG_MAX_FILENAME_LEN       64

// Reference BS file path + name
static CHAR cRefBSPath[EMU_JPEG_MAX_FILENAME_LEN];

// Register file path + name
static CHAR cEncRegPath[EMU_JPEG_MAX_FILENAME_LEN];

// Source Y,U,V path + name
static CHAR cSrcYPath[EMU_JPEG_MAX_FILENAME_LEN];
static CHAR cSrcUPath[EMU_JPEG_MAX_FILENAME_LEN];
static CHAR cSrcVPath[EMU_JPEG_MAX_FILENAME_LEN];

// Referenc DC out path + name
static CHAR cRefDCYPath[EMU_JPEG_MAX_FILENAME_LEN];
static CHAR cRefDCUPath[EMU_JPEG_MAX_FILENAME_LEN];
static CHAR cRefDCVPath[EMU_JPEG_MAX_FILENAME_LEN];

static UINT8            QTblY[64];
static UINT8            QTblUV[64];


// Source BS file path + name
static CHAR cSrcBSPath[EMU_JPEG_MAX_FILENAME_LEN];
// Register file path + name
static CHAR cDecRegPath[EMU_JPEG_MAX_FILENAME_LEN];

// Reference Y,U,V path + name
static CHAR cRefYPath[EMU_JPEG_MAX_FILENAME_LEN];
static CHAR cRefUPath[EMU_JPEG_MAX_FILENAME_LEN];
static CHAR cRefVPath[EMU_JPEG_MAX_FILENAME_LEN];

extern UINT32  jpegtest_getbs_checksum(void);
extern UINT32  jpegtest_getyuv_checksum(void);

extern INT32 kdrv_videoencjpg_trigger(UINT32 id, KDRV_VDOENC_PARAM *p_enc_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
extern INT32 kdrv_vdoencjpg_set(UINT32 id, KDRV_VDOENC_SET_PARAM_ID parm_id, VOID *param);
extern INT32 kdrv_vdoencjpg_get(UINT32 id, KDRV_VDOENC_GET_PARAM_ID parm_id, VOID *param);
extern INT32 kdrv_videodecjpg_trigger(UINT32 id, KDRV_VDODEC_PARAM *p_dec_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
extern INT32 kdrv_videoencjpg_open(UINT32 chip, UINT32 engine);
extern INT32 kdrv_videoencjpg_close(UINT32 chip, UINT32 engine);


static void emu_jpegAddYLineOffset(UINT32 uiSrcAddr,
								   UINT32 uiDstAddr,
								   UINT32 uiWidth,
								   UINT32 uiHeight,
								   UINT32 uiLineOffset)
{
#if 1
// Software path

      UINT32  i, j;
	UINT8  *pDstLine;
	UINT8  *pSrcLine;

	for (i = 0; i < uiHeight; i++) {
		pDstLine    = (UINT8 *)(uiDstAddr + i * uiLineOffset);
		pSrcLine   = (UINT8 *)(uiSrcAddr  + i * uiWidth);

		for (j = 0; j < uiWidth; j++) {
			pDstLine[j]  = pSrcLine[j];

		}
	}

#else

// Hardware path
	GRPH_REQUEST    Request;
	GRPH_IMG        ImgA, ImgC;

	// Width can't exceed grpahic's limitation
	if ((uiWidth > EMU_JPEG_GRPH_MAX_WIDTH) ||
		(uiLineOffset > EMU_JPEG_GRPH_MAX_LINEOFF)) {
		//emujpeg_msg(("%s: Exceed graphic's limitation\r\n", __func__));
		return;
	}

	// Open graphic 1
	graph_open(GRPH_ID_1);

	ImgA.img_id             = GRPH_IMG_ID_A;
	ImgA.lineoffset         = uiWidth;
	ImgA.width              = uiWidth;
	ImgA.p_inoutop           = NULL;
	ImgA.p_next              = &ImgC;

	ImgC.img_id              = GRPH_IMG_ID_C;
	ImgC.lineoffset       = uiLineOffset;
	ImgC.width              = uiWidth;
	ImgC.p_inoutop           = NULL;
	ImgC.p_next              = NULL;

	Request.ver_info        = DRV_VER_96650;
	Request.command         = GRPH_CMD_A_COPY;
	Request.format          = GRPH_FORMAT_8BITS;
	Request.p_images        = &ImgA;
	Request.p_property       = NULL;
	Request.p_ckeyfilter     = NULL;

	while (uiHeight) {
		ImgA.dram_addr      = uiSrcAddr;
		ImgA.height       = (uiHeight > EMU_JPEG_GRPH_MAX_HEIGHT) ? EMU_JPEG_GRPH_MAX_HEIGHT : uiHeight;

		ImgC.dram_addr      = uiDstAddr;
		ImgC.height       = (uiHeight > EMU_JPEG_GRPH_MAX_HEIGHT) ? EMU_JPEG_GRPH_MAX_HEIGHT : uiHeight;

		graph_request(GRPH_ID_1, &Request);

		uiHeight  -= ImgA.height;
		uiSrcAddr += ImgA.lineoffset * ImgA.height;
		uiDstAddr += ImgC.lineoffset * ImgC.height;
	}

	// Close graphic 1
	graph_close(GRPH_ID_1);
#endif
}


/*
    Pack UV planar data to UV packed data (software)

    Pack UV planar data to UV packed data (software).

    @param[in] uiUVAddr         Destination UV packed address
    @param[in] uiUAddr          Source U planar address
    @param[in] uiVAddr          Source V planar address
    @param[in] uiWidth          U, V planar width
    @param[in] uiHeight         U, V planar height
    @param[in] uiDstLineOffset  Destination line offset

    @note   In emulation case, source line offset is always equal to width

    @return void
*/
static void emu_jpegPackUVSoftware(UINT32 uiUVAddr,
								   UINT32 uiUAddr,
								   UINT32 uiVAddr,
								   UINT32 uiWidth,
								   UINT32 uiHeight,
								   UINT32 uiDstLineOffset)
{
	UINT32  i, j;
	UINT8  *pDstLine;
	UINT8  *pSrcULine;
	UINT8  *pSrcVLine;

	// Switch to Cacheable address
	//uiUVAddr    = dma_getCacheAddr(uiUVAddr);
	//uiUAddr     = dma_getCacheAddr(uiUAddr);
	//uiVAddr     = dma_getCacheAddr(uiVAddr);

	for (i = 0; i < uiHeight; i++) {
		pDstLine    = (UINT8 *)(uiUVAddr + i * uiDstLineOffset);
		pSrcULine   = (UINT8 *)(uiUAddr  + i * uiWidth);
		pSrcVLine   = (UINT8 *)(uiVAddr  + i * uiWidth);

		for (j = 0; j < uiWidth; j++) {
			pDstLine[(j << 1)]        = pSrcULine[j];
			pDstLine[(j << 1) + 1]    = pSrcVLine[j];
		}
	}

#if 0
	// Address is already cacheable when calling this fuction
	if (bEmuJpegCache == TRUE) {
#if (EMU_JPEG_DRIVER_HANDLE_CACHE == DISABLE)
		// Clean and invalidate data cache
		cpu_cleanInvalidateDCacheAll();
#endif
	} else {
		// Clean and invalidate data cache
		cpu_cleanInvalidateDCacheAll();
	}
#endif
}


/*
    Pack UV planar data to UV packed data

    Pack UV planar data to UV packed data.

    @param[in] uiUVAddr         Destination UV packed address
    @param[in] uiUAddr          Source U planar address
    @param[in] uiVAddr          Source V planar address
    @param[in] uiWidth          U, V planar width
    @param[in] uiHeight         U, V planar height
    @param[in] uiDstLineOffset  Destination line offset

    @note   In emulation case, source line offset is always equal to width

    @return void
*/
static void emu_jpegPackUV(UINT32 uiUVAddr,
						   UINT32 uiUAddr,
						   UINT32 uiVAddr,
						   UINT32 uiWidth,
						   UINT32 uiHeight,
						   UINT32 uiDstLineOffset)
{
#if 0
// Software + Hardware path
	// UV pack operation can't handle non-word alignment line offset
	// Width & line offset can't exceed graphic's limitation
	if ((uiWidth & 0x3) ||
		(uiDstLineOffset & 0x3) ||
		(uiWidth > EMU_JPEG_GRPH_MAX_WIDTH) ||
		(uiDstLineOffset > EMU_JPEG_GRPH_MAX_LINEOFF))
#else
// Software path
	if (1)
#endif
	{
		emu_jpegPackUVSoftware(uiUVAddr,
							   uiUAddr,
							   uiVAddr,
							   uiWidth,
							   uiHeight,
							   uiDstLineOffset);
	}
#if 0
	else {
		GRPH_REQUEST    Request;
		GRPH_IMG        ImgA, ImgB, ImgC;

		// Open graphic 1
		graph_open(GRPH_ID_1);

		ImgA.imgID              = GRPH_IMG_ID_A;
		ImgA.uiLineoffset       = uiWidth;
		ImgA.uiWidth            = uiWidth;
		ImgA.pInOutOP           = NULL;
		ImgA.pNext              = &ImgB;

		ImgB.imgID              = GRPH_IMG_ID_B;
		ImgB.uiLineoffset       = uiWidth;
		ImgB.uiWidth            = uiWidth;
		ImgB.pInOutOP           = NULL;
		ImgB.pNext              = &ImgC;

		ImgC.imgID              = GRPH_IMG_ID_C;
		ImgC.uiLineoffset       = uiDstLineOffset;
		ImgC.uiWidth            = uiWidth;
		ImgC.pInOutOP           = NULL;
		ImgC.pNext              = NULL;

		Request.ver_info        = DRV_VER_96650;
		Request.command         = GRPH_CMD_PACKING;
		Request.format          = GRPH_FORMAT_8BITS;
		Request.pImageDescript  = &ImgA;
		Request.pProperty       = NULL;
		Request.pCkeyFilter     = NULL;

		while (uiHeight) {
			ImgA.uiAddress      = uiUAddr;
			ImgA.uiHeight       = (uiHeight > EMU_JPEG_GRPH_MAX_HEIGHT) ? EMU_JPEG_GRPH_MAX_HEIGHT : uiHeight;

			ImgB.uiAddress      = uiVAddr;
			ImgB.uiHeight       = (uiHeight > EMU_JPEG_GRPH_MAX_HEIGHT) ? EMU_JPEG_GRPH_MAX_HEIGHT : uiHeight;

			ImgC.uiAddress      = uiUVAddr;
			ImgC.uiHeight       = (uiHeight > EMU_JPEG_GRPH_MAX_HEIGHT) ? EMU_JPEG_GRPH_MAX_HEIGHT : uiHeight;

			graph_request(GRPH_ID_1, &Request);

			uiHeight  -= ImgA.uiHeight;
			uiUAddr   += ImgA.uiLineoffset * ImgA.uiHeight;
			uiVAddr   += ImgB.uiLineoffset * ImgB.uiHeight;
			uiUVAddr  += ImgC.uiLineoffset * ImgC.uiHeight;
		}

		// Close graphic 1
		graph_close(GRPH_ID_1);
	}
#endif
}



/*
    Read reference BS file to memory

    Read reference BS file to memory.

    @return Read file status.
        - @b FALSE  : Read file fail
        - @b TRUE   : Read file OK
*/
static BOOL emu_jpegReadRefBSFile(void)
{
	FST_FILE    file_hdl;
	UINT32      uiFileSize;
	INT32       iSts;

	// Open file to read
	file_hdl = filesys_openfile(cRefBSPath, FST_OPEN_READ);

	if (file_hdl != NULL) {
		// Read file from position 0
		uiFileSize = pEncRegSet->JPEGEncRegData.BS_LEN;

		iSts = filesys_readfile(file_hdl,
								(UINT8 *)EncBuf.uiRefBSBuf,
								&uiFileSize,
								0,
								NULL);

		// Close file
		filesys_closefile(file_hdl);

		if (iSts != FST_STA_OK) {
			//emujpeg_msg(("Read %s error!\r\n", cRefBSPath));
			return FALSE;
		}

		return TRUE;
	} else {
		//emujpeg_msg(("Read %s error!\r\n", cRefBSPath));
		return FALSE;
	}
}

/*
    Read encode register file to memory

    Read encode register file to memory.

    @return Read file status.
        - @b FALSE  : Read file fail
        - @b TRUE   : Read file OK
*/
static BOOL emu_jpegReadEncRegFile(void)
{
	FST_FILE    file_hdl;
	UINT32      uiFileSize;
	INT32       iSts;

	// Open file to read
	file_hdl = filesys_openfile(cEncRegPath, FST_OPEN_READ);

	if (file_hdl != NULL) {
		// Read file from position 0
		uiFileSize = sizeof(EMU_JPEG_ENC_REG);

		iSts = filesys_readfile(file_hdl,
								(UINT8 *)EncBuf.uiRegFileBuf,
								&uiFileSize,
								0,
								NULL);

		// Close file
		filesys_closefile(file_hdl);

		if (iSts != FST_STA_OK) {
			//emujpeg_msg(("Read %s error!\r\n", cEncRegPath));
			return FALSE;
		}

		return TRUE;
	} else {
		//emujpeg_msg(("Read %s error!\r\n", cEncRegPath));
		// Close file
		//filesys_closefile(file_hdl);
		return FALSE;
	}
}

/*
    Read source Y, U, V file to memory

    Read source Y, U, V file to memory.

    @param[in] uiYSize      Y data size
    @param[in] uiUVSize     UV packed data size
    @param[in] uiYWidth     Y width
    @param[in] uiYHeight    Y height
    @param[in] uiUVWidth    UV packed width
    @param[in] uiUVHeight   UV packed height

    @return Read file status.
        - @b FALSE  : Read file fail
        - @b TRUE   : Read file OK
*/
static BOOL emu_jpegReadSrcYUVFile(UINT32 uiYSize,
								   UINT32 uiUVSize,
								   UINT32 uiYWidth,
								   UINT32 uiYHeight,
								   UINT32 uiUVWidth,
								   UINT32 uiUVHeight)
{
	FST_FILE    file_hdl;
	UINT32      uiFileSize;
	INT32       iSts;
	int i;
	UINT8 *pBuf;
	UINT32  fofs = 0;

	// Since UV planar buffer == Y buffer, we have to read UV first
	// Read U, V planar
	if (uiEncFormat != EMU_JPEG_MCU_100) {
#if 0//(AUTOTEST_JPEG == ENABLE)
		// Read UV
		// Open file to read
		file_hdl = filesys_openfile(cSrcUPath, FST_OPEN_READ);

		if (file_hdl != NULL) {
			// Read file from position 0
			// uiUVSize is UV packed size
			uiFileSize = uiUVSize;

			if (uiFileSize) {
				iSts = filesys_readfile(file_hdl,
										(UINT8 *)EncBuf.uiUVBuf,
										&uiFileSize,
										0,
										NULL);
			} else {
				iSts = FST_STA_OK;
			}

			// Close file
			filesys_closefile(file_hdl);

			if (iSts != FST_STA_OK) {
				emujpeg_msg(("Read %s error!\r\n", cSrcUPath));
				return FALSE;
			}
		} else {
			emujpeg_msg(("Read %s error!\r\n", cSrcUPath));
			return FALSE;
		}
#else
		// Since 650 only support UV packed format, and our patterns are U V planar,
		// We have to read U V raw to temporary buffer and pack to UV packed format.

		// Read U
		// Open file to read
		file_hdl = filesys_openfile(cSrcUPath, FST_OPEN_READ);

		if (file_hdl != NULL) {
			// Read file from position 0
			// uiUVSize is UV packed size
			uiFileSize = uiUVSize >> 1;

			if (uiFileSize) {
				iSts = filesys_readfile(file_hdl,
										(UINT8 *)EncBuf.uiUPlanarBuf,
										&uiFileSize,
										0,
										NULL);
			} else {
				iSts = FST_STA_OK;
			}

			// Close file
			filesys_closefile(file_hdl);

			if (iSts != FST_STA_OK) {
				///emujpeg_msg(("Read %s error!\r\n", cSrcUPath));
				printk("Read %s error!\r\n", cSrcUPath);
				return FALSE;
			}
		} else {
			//emujpeg_msg(("Read %s error!\r\n", cSrcUPath));
			printk("Read %s error2!\r\n", cSrcUPath);
			return FALSE;
		}

		// Read V
		// Open file to read
		file_hdl = filesys_openfile(cSrcVPath, FST_OPEN_READ);

		if (file_hdl != NULL) {
			// Read file from position 0
			// uiUVSize is UV packed size
			uiFileSize = uiUVSize >> 1;

			if (uiFileSize) {
				iSts = filesys_readfile(file_hdl,
										(UINT8 *)EncBuf.uiVPlanarBuf,
										&uiFileSize,
										0,
										NULL);
			} else {
				iSts = FST_STA_OK;
			}

			// Close file
			filesys_closefile(file_hdl);

			if (iSts != FST_STA_OK) {
				//emujpeg_msg(("Read %s error!\r\n", cSrcVPath));
				printk("Read %s error!\r\n", cSrcVPath);
				return FALSE;
			}
		} else {
			//emujpeg_msg(("Read %s error!\r\n", cSrcVPath));
			printk("Read %s error2!\r\n", cSrcVPath);
			return FALSE;
		}

		//printk("UV packed 1!\r\n");

		// U, V planar to UV packed
		emu_jpegPackUV(EncBuf.uiUVBuf,
					   EncBuf.uiUPlanarBuf,
					   EncBuf.uiVPlanarBuf,
					   uiUVWidth >> 1,
					   uiUVHeight,
					   uiUVWidth + uiEncExtraLineOffset);

		//printk("UV packed 2!\r\n");

		//emujpeg_msg(("EncBuf.uiUPlanarBuf = 0x%x!\r\n", (unsigned int)(EncBuf.uiUPlanarBuf)));
		//emujpeg_msg(("EncBuf.uiVPlanarBuf = 0x%x!\r\n", (unsigned int)(EncBuf.uiVPlanarBuf)));
		//emujpeg_msg(("EncBuf.uiUVBuf = 0x%x!\r\n", (unsigned int)(EncBuf.uiUVBuf)));
		//printk("emu_jpegReadSrcYUVFile EncBuf.uiUPlanarBuf 0x%x\r\n", (unsigned int)(fmem_lookup_pa(EncBuf.uiUPlanarBuf)));
		//printk("emu_jpegReadSrcYUVFile EncBuf.uiVPlanarBuf 0x%x\r\n", (unsigned int)(fmem_lookup_pa(EncBuf.uiVPlanarBuf)));
		//printk("emu_jpegReadSrcYUVFile EncBuf.uiUVBuf 0x%x  0x%x\r\n", (unsigned int)(fmem_lookup_pa(EncBuf.uiUVBuf)), (unsigned int)(EncBuf.uiUVBuf));
#endif
	}

	// Read Y
	// Read to uiYBuf directly
	// Extra line offset != 0 and width or line offset exceed graphic limitation
	if ((uiEncExtraLineOffset != 0) &&
		((uiYWidth > EMU_JPEG_GRPH_MAX_WIDTH) ||
		 ((uiYWidth + uiEncExtraLineOffset) > EMU_JPEG_GRPH_MAX_LINEOFF))) {
		// Open file to read
		file_hdl = filesys_openfile(cSrcYPath, FST_OPEN_READ);

		if (file_hdl != NULL) {
			UINT32 i;

//printk("emu_jpegReadSrcYUVFile emu_jpegReadSrcYUVFile1\r\n");
			// Read line by line
			for (i = 0; i < uiYHeight; i++) {
				uiFileSize = uiYWidth;

				//iSts = filesys_readfile(file_hdl,
				//                      (UINT8 *)(EncBuf.uiYBuf + (i * (uiYWidth + uiEncExtraLineOffset))),
				//                      &uiFileSize,
				//                      0,
				//                      NULL);
				iSts = filesys_readcontfile(file_hdl,
											(UINT8 *)(EncBuf.uiYBuf + (i * (uiYWidth + uiEncExtraLineOffset))),
											&uiFileSize,
											0,
											NULL,
											fofs);
				fofs += uiFileSize;


				if (iSts != FST_STA_OK) {
					// Close file
					filesys_closefile(file_hdl);

					//emujpeg_msg(("Read %s error!\r\n", cSrcYPath));
					return FALSE;
				}
			}

			// Close file
			filesys_closefile(file_hdl);
		} else {
			//emujpeg_msg(("Read %s error!\r\n", cSrcYPath));
			return FALSE;
		}
	}
	// Read to uiTempYBuf
	else {
		// Open file to read
		file_hdl = filesys_openfile(cSrcYPath, FST_OPEN_READ);

		if (file_hdl != NULL) {
			// Read file from position 0
			uiFileSize = uiYSize;

//printk(" EncBuf.uiTempYBuf =  0x%x 0x%x\r\n", (unsigned int)(fmem_lookup_pa(EncBuf.uiTempYBuf)), (unsigned int)(EncBuf.uiTempYBuf));

//fmem_dcache_sync((void*)EncBuf.uiTempYBuf, uiFileSize, DMA_BIDIRECTIONAL);

			// If extra line offset == 0, uiYBuf = uiTempYBuf.
			// If extra line offset != 0, we have to read to temp buffer to speed up.
			iSts = filesys_readfile(file_hdl,
									(UINT8 *)EncBuf.uiTempYBuf,
									&uiFileSize,
									0,
									NULL);

			// Close file
			filesys_closefile(file_hdl);




//pBuf = (UINT8 *)EncBuf.uiTempYBuf;

//for(i=0;i<0x100;i=i+4)
//    printk("data = 0x%x 0x%x 0x%x 0x%x\r\n", (unsigned int)(*(pBuf+i)), (unsigned int)(*(pBuf+1+i)), (unsigned int)(*(pBuf+2+i)), (unsigned int)(*(pBuf+3+i)));




			if (iSts != FST_STA_OK) {
				//emujpeg_msg(("Read %s error!\r\n", cSrcYPath));
				return FALSE;
			}
		} else {
			//emujpeg_msg(("Read %s error!\r\n", cSrcYPath));
			return FALSE;
		}

		// Add extra line offset
		if (EncBuf.uiTempYBuf != EncBuf.uiYBuf) {
//printk("EncBuf.uiTempYBuf != EncBuf.uiYBuf\r\n");
			emu_jpegAddYLineOffset(EncBuf.uiTempYBuf,
								   EncBuf.uiYBuf,
								   uiYWidth,
								   uiYHeight,
								   uiYWidth + uiEncExtraLineOffset);
		}
	}
//emujpeg_msg(("EncBuf.uiYBuf = 0x%x!\r\n", (unsigned int)(EncBuf.uiYBuf)));
	//  printk("emu_jpegReadSrcYUVFile EncBuf.uiYBuf 0x%x  0x%x\r\n", (unsigned int)(fmem_lookup_pa(EncBuf.uiYBuf)), (unsigned int)(EncBuf.uiYBuf));
	//  printk("Read YUV OK\r\n");

//pBuf = (UINT8 *)EncBuf.uiYBuf;
//for(i=0;i<0x100;i=i+4)
//    printk("data = 0x%x 0x%x 0x%x 0x%x\r\n", (unsigned int)(*(pBuf+i)), (unsigned int)(*(pBuf+1+i)), (unsigned int)(*(pBuf+2+i)), (unsigned int)(*(pBuf+3+i)));

	return TRUE;
}

/*
    Compare data in byte unit

    Compare data in byte unit.

    @param[in] uiSrcAddr        Source data address (pattern file)
    @param[in] uiDstAddr        Destination data address (HW output)
    @param[in] uiWidth          Width (byte)
    @param[in] uiHeight         Height (byte)
    @param[in] uiDstLineOffset  Destination (HW output) line offset

    @note   In emulation case, source (pattern file) line offset is always equal to width

    @return Compare result
        - @b FALSE  : Data is not matched
        - @b TRUE   : Data is matched
*/
BOOL emu_jpegCompareByte(UINT32 uiSrcAddr,
						 UINT32 uiDstAddr,
						 UINT32 uiWidth,
						 UINT32 uiHeight,
						 UINT32 uiDstLineOffset)
{
	UINT32  i, j;
	UINT8   *pSrc, *pDst;

	// Switch to Cacheable address if using cacheable address
#if 0
	if (bEmuJpegCache == TRUE) {
		// Clean and invalidate data cache
		cpu_cleanInvalidateDCacheAll();
		pSrc = (UINT8 *)dma_getCacheAddr(uiSrcAddr);
		pDst = (UINT8 *)dma_getCacheAddr(uiDstAddr);
	} else {
		pSrc = (UINT8 *)dma_getNonCacheAddr(uiSrcAddr);
		pDst = (UINT8 *)dma_getNonCacheAddr(uiDstAddr);
	}
#endif

	pSrc = (UINT8 *)(uiSrcAddr);
	pDst = (UINT8 *)(uiDstAddr);

	for (i = 0; i < uiHeight; i++) {
		for (j = 0; j < uiWidth; j++) {
			if (*pSrc != *pDst) {
				//emujpeg_msg(("HW: 0x%.2X, Pattern: 0x%.2X\r\n", *pDst, *pSrc));
				//emujpeg_msg(("Byte %ldx%ld (%ldx%ld) compare error!\r\n", (long)(j), (long)(i), (long)(uiWidth), (long)(uiHeight)));

				// Clean and invalidate data cache
				//cpu_cleanInvalidateDCacheAll();
				printk("HW: 0x%.2X, Pattern: 0x%.2X\r\n", *pDst, *pSrc);
				printk("Byte %ldx%ld (%ldx%ld) compare error!\r\n", (long)(j), (long)(i), (long)(uiWidth), (long)(uiHeight));

				return FALSE;
			}
			pSrc++;
			pDst++;
		}

		pDst += (uiDstLineOffset - uiWidth);
	}

	return TRUE;
}

/*
    Compare encode result

    Compare encode result.

    @return Compare result.
        - @b FALSE  : Something wrong
        - @b TRUE   : Everything OK
*/
static BOOL emu_jpegEncodeCompare(void)
{
	JPEG_BRC_INFO   BRCInfo;
	BOOL            ret;

	ret = TRUE;

	// ===================================================================
	// Compare Rho value
	// ===================================================================
	jpeg_get_brcinfo(&BRCInfo);

	if (BRCInfo.brcinfo1 != pEncRegSet->JPEGEncRegData.RHO_1_8) {
		//emujpeg_msg(("Rho 1/8Q Value error! (HW:0x%X, Pattern:0x%X)\r\n", (unsigned int)(BRCInfo.brcinfo1), (unsigned int)(pEncRegSet->JPEGEncRegData.RHO_1_8)));
		ret = FALSE;
	}

	if (BRCInfo.brcinfo2 != pEncRegSet->JPEGEncRegData.RHO_1_4) {
		//emujpeg_msg(("Rho 1/4Q Value error! (HW:0x%X, Pattern:0x%X)\r\n", (unsigned int)(BRCInfo.brcinfo2), (unsigned int)(pEncRegSet->JPEGEncRegData.RHO_1_4)));
		ret = FALSE;
	}

	if (BRCInfo.brcinfo3 != pEncRegSet->JPEGEncRegData.RHO_1_2) {
		//emujpeg_msg(("Rho 1/2Q Value error! (HW:0x%X, Pattern:0x%X)\r\n", (unsigned int)(BRCInfo.brcinfo3), (unsigned int)(pEncRegSet->JPEGEncRegData.RHO_1_2)));
		ret = FALSE;
	}

	if (BRCInfo.brcinfo4 != pEncRegSet->JPEGEncRegData.RHO) {
		//emujpeg_msg(("Rho 1Q Value error! (HW:0x%X, Pattern:0x%X)\r\n", (unsigned int)(BRCInfo.brcinfo4), (unsigned int)(pEncRegSet->JPEGEncRegData.RHO)));
		ret = FALSE;
	}

	if (BRCInfo.brcinfo5 != pEncRegSet->JPEGEncRegData.RHO_2) {
		//emujpeg_msg(("Rho 2Q Value error! (HW:0x%X, Pattern:0x%X)\r\n", (unsigned int)(BRCInfo.brcinfo5), (unsigned int)(pEncRegSet->JPEGEncRegData.RHO_2)));
		ret = FALSE;
	}

	if (BRCInfo.brcinfo6 != pEncRegSet->JPEGEncRegData.RHO_4) {
		//emujpeg_msg(("Rho 4Q Value error! (HW:0x%X, Pattern:0x%X)\r\n", (unsigned int)(BRCInfo.brcinfo6), (unsigned int)(pEncRegSet->JPEGEncRegData.RHO_4)));
		ret = FALSE;
	}

	if (BRCInfo.brcinfo7 != pEncRegSet->JPEGEncRegData.RHO_8) {
		//emujpeg_msg(("Rho 8Q Value error! (HW:0x%X, Pattern:0x%X)\r\n", (unsigned int)(BRCInfo.brcinfo7), (unsigned int)(pEncRegSet->JPEGEncRegData.RHO_8)));
		ret = FALSE;
	}

	// ===================================================================
	// Compare bit stream length
	// ===================================================================
	if (jpeg_get_bssize() != pEncRegSet->JPEGEncRegData.BS_LEN) {
		//emujpeg_msg(("BS Length error! (HW:0x%X, Pattern:0x%X)\r\n", (unsigned int)(jpeg_get_bssize()), (unsigned int)(pEncRegSet->JPEGEncRegData.BS_LEN)));
		ret = FALSE;
	}

	// ===================================================================
	// Compare YUV checksum
	// ===================================================================
	if (jpegtest_getyuv_checksum() != pEncRegSet->JPEGEncRegData.YUV_CHECKSUM) {
		//emujpeg_msg(("YUV Checksum error! (HW:0x%X, Pattern:0x%X)\r\n", (unsigned int)(jpegtest_getyuv_checksum()), (unsigned int)(pEncRegSet->JPEGEncRegData.YUV_CHECKSUM)));
		printk("YUVchecksum error\r\n");
		ret = FALSE;
	}

	// ===================================================================
	// Compare bit stream checksum
	// ===================================================================
	if (jpegtest_getbs_checksum() != pEncRegSet->JPEGEncRegData.BS_CHECKSUM) {
		//emujpeg_msg(("BS Checksum error! (HW:0x%X, Pattern:0x%X)\r\n", (unsigned int)(jpegtest_getbs_checksum()), (unsigned int)(pEncRegSet->JPEGEncRegData.BS_CHECKSUM)));
		printk("BSchecksum error\r\n");
		ret = FALSE;
	}

#if 0
	// ===================================================================
	// Check EOF (0xFFD9)
	// ===================================================================
	if (*(UINT8 *)(jpeg_get_bscurraddr() - 1) != 0xD9) {
		emujpeg_msg(("EOF Flush error!\r\n"));
		bReturn = FALSE;
	}
#else
	// ===================================================================
	// Compare bit stream (Include 0xFFD9)
	// ===================================================================
	if (emu_jpegCompareByte(EncBuf.uiRefBSBuf,
							EncBuf.uiBSBuf,
							pEncRegSet->JPEGEncRegData.BS_LEN,
							1,
							pEncRegSet->JPEGEncRegData.BS_LEN) != TRUE) {
		//emujpeg_msg(("Compare BS data error!\r\n"));
		ret = FALSE;
	}
#endif
#if 0
	// ===================================================================
	// Compare DC out YUV
	// ===================================================================
	if (DCOutMode != DC_DISABLE) {
		// DC out Y width, height and UV packed width, height
		UINT32 uiDCYWidth, uiDCYHeight, uiDCUVWidth, uiDCUVHeight;

		uiDCYWidth  = pEncRegSet->JPEGEncRegData.SRC_XB >> DCOutCfg.dc_xratio;
		uiDCYHeight = pEncRegSet->JPEGEncRegData.SRC_YB >> DCOutCfg.dc_yratio;
		// Only 2h11 and 411 support DC out
		uiDCUVWidth = uiDCYWidth;
		// For transform mode, the MCUMode in register set is EMU_JPEG_MCU_2h11.
		// But uiEncFormat is EMU_JPEG_MCU_411
		// 2h11
//        if (uiEncFormat == EMU_JPEG_MCU_2h11)
		if (pEncRegSet->MCUMode == EMU_JPEG_MCU_2h11) {
			uiDCUVHeight = uiDCYHeight;
		}
		// EMU_JPEG_MCU_411
		else {
			uiDCUVHeight = uiDCYHeight >> 1;
		}

		// Compare Y
		if (emu_jpegCompareByte(EncBuf.uiRefDCOutYBuf,
								EncBuf.uiDCOutYBuf,
								uiDCYWidth,
								uiDCYHeight,
								DCOutCfg.dc_ylineoffset) != TRUE) {
			emujpeg_msg(("Compare DC out Y data error!\r\n"));
			bReturn = FALSE;
		}

		// Compare UV
		if (emu_jpegCompareByte(EncBuf.uiRefDCOutUVBuf,
								EncBuf.uiDCOutUVBuf,
								uiDCUVWidth,
								uiDCUVHeight,
								DCOutCfg.dc_ulineoffset) != TRUE) {
			emujpeg_msg(("Compare DC out UV data error!\r\n"));
			bReturn = FALSE;
		}
	}
#endif
	return ret;
}



/*
    Encode pattern

    Encode pattern.

    @param[in] bProfiling   Profiling encode performance or not
        - @b FALSE  : Don't profiling
        - @b TRUE   : Profiling encode performance

    @return Encode result.
        - @b FALSE  : Something wrong
        - @b TRUE   : Everything OK
*/
static BOOL emu_jpegEncodePattern(BOOL bProfiling)
{
	// DC out x, y ratio.
	// Disabled: 0, 1/2: Ratio = 1, 1/4: Ratio = 2, 1/8: Ratio = 3
	// DC out width, height = Source width, height >> ratio
	UINT32  uiDCXRatio, uiDCYRatio;

	// Y width & height
	UINT32  uiYWidth, uiYHeight;

	// UV packed width & height
	UINT32  uiUVWidth, uiUVHeight;

	// Y size, and extra Y size
	UINT32  uiYSize, uiYExtraSize;

	// UV packed size, and extra UV packed size
	UINT32  uiUVSize, uiUVExtraSize;

	// DC out Y size, and extra Y size
	UINT32  uiDCYSize, uiDCYExtraSize;

	// DC out, UV packed size, and extra UV packed size
	UINT32  uiDCUVSize, uiDCUVExtraSize;

	// Bit stream length
	UINT32  uiBSLen;

	// Temporary address when allocating buffer
	UINT32  uiTempAddr;

	// For clearing memory
	UINT32 i, *pAddr;

	// Randomize buffer
	UINT32  uiRandom;

	// JPEG driver YUV format
	JPEG_YUV_FORMAT     YUVFormat;


	KDRV_VDOENC_PARAM   enc_obj;
	KDRV_CALLBACK_FUNC  callback;
	KDRV_JPEG_INFO      enc_info;


	// ===================================================================
	// Get information
	// ===================================================================
	// Y width, height
	uiYWidth    = pEncRegSet->JPEGEncRegData.SRC_XB;
	uiYHeight   = pEncRegSet->JPEGEncRegData.SRC_YB;

	// Get UV width, height
	// For transform mode, the MCUMode in register set is EMU_JPEG_MCU_2h11.
	// But uiEncFormat is EMU_JPEG_MCU_411
//    switch (uiEncFormat)
	switch (pEncRegSet->MCUMode) {
	default:
//    case EMU_JPEG_MCU_100:
		YUVFormat   = JPEG_YUV_FORMAT_100;
		uiUVWidth   = 0;
		uiUVHeight  = 0;
		printk("MCU: 100");
		break;

	case EMU_JPEG_MCU_111:
		YUVFormat   = JPEG_YUV_FORMAT_111;
		uiUVWidth   = uiYWidth << 1;
		uiUVHeight  = uiYHeight;
		printk(("MCU: 111"));
		break;

	case EMU_JPEG_MCU_222h:
		YUVFormat   = JPEG_YUV_FORMAT_222;
		uiUVWidth   = uiYWidth << 1;
		uiUVHeight  = uiYHeight;
		printk(("MCU: 222h"));
		break;

	case EMU_JPEG_MCU_222v:
		YUVFormat   = JPEG_YUV_FORMAT_222V;
		uiUVWidth   = uiYWidth << 1;
		uiUVHeight  = uiYHeight;
		printk(("MCU: 222v"));
		break;

	case EMU_JPEG_MCU_2h11:
		YUVFormat   = JPEG_YUV_FORMAT_211;
		uiUVWidth   = uiYWidth;
		uiUVHeight  = uiYHeight;
		printk(("MCU: 2h11"));
		break;

	case EMU_JPEG_MCU_422v:
		YUVFormat   = JPEG_YUV_FORMAT_422V;
		uiUVWidth   = uiYWidth;
		uiUVHeight  = uiYHeight;
		printk(("MCU: 422v"));
		break;

	case EMU_JPEG_MCU_2v11:
		YUVFormat   = JPEG_YUV_FORMAT_211V;
		uiUVWidth   = uiYWidth << 1;
		uiUVHeight  = uiYHeight >> 1;
		printk(("MCU: 2v11"));
		break;

	case EMU_JPEG_MCU_422h:
		YUVFormat   = JPEG_YUV_FORMAT_422;
		uiUVWidth   = uiYWidth << 1;
		uiUVHeight  = uiYHeight >> 1;
		printk(("MCU: 422h"));
		break;

	case EMU_JPEG_MCU_411:
		YUVFormat   = JPEG_YUV_FORMAT_411;
		uiUVWidth   = uiYWidth;
		uiUVHeight  = uiYHeight >> 1;
		printk(("MCU: 411"));
		break;
	}

	printk(", %ldx%ld\r\n", (long)(uiYWidth), (long)(uiYHeight));


	// Y size
	uiYSize         = uiYWidth * uiYHeight;
	// Y extra size
	uiYExtraSize    = uiEncExtraLineOffset * uiYHeight;

	// UV packed size
	uiUVSize        = uiUVWidth * uiUVHeight;
	// UV packed extra size
	uiUVExtraSize   = uiEncExtraLineOffset * uiUVHeight;

	// Get DC out x, y ratio
	switch (DCOutMode) {
	default:
//    case DC_DISABLE:
		uiDCXRatio = 0;
		uiDCYRatio = 0;
		break;

	case DC_2_2:
		uiDCXRatio = 1;
		uiDCYRatio = 1;
		break;

	case DC_2_4:
		uiDCXRatio = 1;
		uiDCYRatio = 2;
		break;

	case DC_2_8:
		uiDCXRatio = 1;
		uiDCYRatio = 3;
		break;

	case DC_4_2:
		uiDCXRatio = 2;
		uiDCYRatio = 1;
		break;

	case DC_4_4:
		uiDCXRatio = 2;
		uiDCYRatio = 2;
		break;

	case DC_4_8:
		uiDCXRatio = 2;
		uiDCYRatio = 3;
		break;

	case DC_8_2:
		uiDCXRatio = 3;
		uiDCYRatio = 1;
		break;

	case DC_8_4:
		uiDCXRatio = 3;
		uiDCYRatio = 2;
		break;

	case DC_8_8:
		uiDCXRatio = 3;
		uiDCYRatio = 3;
		break;
	}


	// ===================================================================
	// Allocate buffer
	// ===================================================================
	// Get available buffer address
	uiTempAddr = EncBuf.uiRegFileBuf + sizeof(EMU_JPEG_ENC_REG);
	//printk(("uiTempAddr: 0x%x",uiTempAddr));

	// Bit stream buffer
	// Align to XX + 1 bytes, so we can test buffer address from 0 to XX
	uiTempAddr      = (((uiTempAddr) + EMU_JPEG_BS_BUF_ALIGN) & ~EMU_JPEG_BS_BUF_ALIGN);
	uiTempAddr     += uiEncBSBufOffset;
	EncBuf.uiBSBuf  = uiTempAddr;
//printk(("(1)EncBuf.uiBSBuf: 0x%x",EncBuf.uiBSBuf));

	// Calculate available address for next buffer
	// Minimum bit stream length of JPEG controller is EMU_JPEG_MIN_BS_LEN
	if (pEncRegSet->JPEGEncRegData.BS_LEN < EMU_JPEG_MIN_BS_LEN) {
		uiBSLen = EMU_JPEG_MIN_BS_LEN;
	} else {
		uiBSLen = pEncRegSet->JPEGEncRegData.BS_LEN;
	}
//printk(("(2)uiBSLen: 0x%x",uiBSLen));
	uiTempAddr += uiBSLen;

	// Reference bit stream buffer
	uiTempAddr          = ALIGN_CEIL_4(uiTempAddr);
	EncBuf.uiRefBSBuf   = uiTempAddr;
//printk(("(3)EncBuf.uiRefBSBuf: 0x%x",EncBuf.uiRefBSBuf));
	//printk(("pEncRegSet->JPEGEncRegData.BS_LEN: 0x%x",pEncRegSet->JPEGEncRegData.BS_LEN));

	// Calculate available address for next buffer
	uiTempAddr += pEncRegSet->JPEGEncRegData.BS_LEN;


	// Source planar U,V buffer
	EncBuf.uiUPlanarBuf = ALIGN_CEIL_4(uiTempAddr);
	// uiUVSize is UV packed size, and we only need U or V planar size here
	EncBuf.uiVPlanarBuf = ALIGN_CEIL_4(uiTempAddr) + (uiUVSize >> 1);
	//printk(("EncBuf.uiUPlanarBuf: 0x%x",EncBuf.uiUPlanarBuf));

	// Source Y buffer
	// Align to XX + 1 bytes, so we can test buffer address from 0 to XX
	uiTempAddr      = (((uiTempAddr) + EMU_JPEG_SRC_BUF_ALIGN) & ~EMU_JPEG_SRC_BUF_ALIGN);
	uiTempAddr     += uiEncYBufOffset;
	EncBuf.uiYBuf   = uiTempAddr;

	// Calculate available address for next buffer
	uiTempAddr += (uiYSize + uiYExtraSize);

//printk(("(4) ========== EncBuf.uiYBuf: 0x%x",EncBuf.uiYBuf));
//printk(("(5) ==uiYExtraSize: 0x%x",uiYExtraSize));
//#if 1
	// Extra line offset of Y, and width + extra line offset <= Graphic limitation.
	// Use temp buffer to improve performance
	if ((uiYExtraSize != 0) &&
		((uiYWidth + uiEncExtraLineOffset) <= EMU_JPEG_GRPH_MAX_WIDTH)) {
		// Source Y temp buffer for line offset != width
		uiTempAddr = ALIGN_CEIL_4(uiTempAddr);
		EncBuf.uiTempYBuf = uiTempAddr;

		// Calculate available address for next buffer
		uiTempAddr = uiTempAddr + uiYSize;
	} else {
		EncBuf.uiTempYBuf = EncBuf.uiYBuf;
	}
//printk(("(6) ========== EncBuf.uiTempYBuf: 0x%x",EncBuf.uiTempYBuf));

	// Source UV buffer
	// Align to XX + 1 bytes, so we can test buffer address from 0 to XX in half-word alignment
	uiTempAddr      = (((uiTempAddr) + EMU_JPEG_SRC_BUF_ALIGN) & ~EMU_JPEG_SRC_BUF_ALIGN);
	uiTempAddr     += (uiEncYBufOffset & ~0x1);
	EncBuf.uiUVBuf  = uiTempAddr;

	// Calculate available address for next buffer
	uiTempAddr += uiUVSize + uiUVExtraSize;

	// DC out Y
	// Align to 16 bytes, so we can test 0x0, 0x4, 0x8, 0xC offset
	// Test word offset
	uiTempAddr          = ALIGN_CEIL_16(uiTempAddr);
	uiTempAddr         += uiEncWordOffset;
	EncBuf.uiDCOutYBuf  = uiTempAddr;

	// Calculate available address for next buffer
	uiTempAddr += (uiDCYSize + uiDCYExtraSize);

	// DC out UV buffer
	// Align to 16 bytes, so we can test 0x0, 0x4, 0x8, 0xC offset
	// Test word offset
	uiTempAddr          = ALIGN_CEIL_16(uiTempAddr);
	uiTempAddr         += uiEncWordOffset;
	EncBuf.uiDCOutUVBuf = uiTempAddr;

	// Calculate available address for next buffer
	uiTempAddr += (uiDCUVSize + uiDCUVExtraSize);

	// Reference DC out U, V planar buffer
	EncBuf.uiRefDCOutUPlanarBuf = ALIGN_CEIL_4(uiTempAddr);
	// uiUVSize is UV packed size, and we only need U or V planar size here
	EncBuf.uiRefDCOutVPlanarBuf = ALIGN_CEIL_4(uiTempAddr) + (uiDCUVSize >> 1);

	// Reference DC out Y buffer
	uiTempAddr              = ALIGN_CEIL_4(uiTempAddr);
	EncBuf.uiRefDCOutYBuf   = uiTempAddr;

	// Calculate available address for next buffer
	uiTempAddr += uiDCYSize;

	// Reference DC out UV buffer
	uiTempAddr              = ALIGN_CEIL_4(uiTempAddr);
	EncBuf.uiRefDCOutUVBuf  = uiTempAddr;

	// Calculate available address for next buffer
	uiTempAddr += uiDCUVSize;

	uiRandom = 0;

	EncBuf.uiBSBuf              += uiRandom;
	EncBuf.uiRefBSBuf           += uiRandom;
	EncBuf.uiYBuf               += uiRandom;
	EncBuf.uiTempYBuf           += uiRandom;
	EncBuf.uiUVBuf              += uiRandom;
	EncBuf.uiDCOutYBuf          += uiRandom;
	EncBuf.uiDCOutUVBuf         += uiRandom;
	EncBuf.uiUPlanarBuf         += uiRandom;
	EncBuf.uiVPlanarBuf         += uiRandom;
	EncBuf.uiRefDCOutYBuf       += uiRandom;
	EncBuf.uiRefDCOutUVBuf      += uiRandom;
	EncBuf.uiRefDCOutUPlanarBuf += uiRandom;
	EncBuf.uiRefDCOutVPlanarBuf += uiRandom;
	uiTempAddr                  += uiRandom;

	// Clear memory
	pAddr = (UINT32 *)(ALIGN_FLOOR_4(EncBuf.uiBSBuf));

	i = ALIGN_CEIL_4(uiTempAddr - (UINT32)pAddr);

	// DMA clear memory
	memset((UINT32)pAddr, 0xDEADBEEF, i);


	// Read reference bit stream file
	if (emu_jpegReadRefBSFile() == FALSE) {
		return FALSE;
	}

	// ===================================================================
	// Read YUV and encode
	// ===================================================================
	// Read Y, U, V source
	if (uiEncMode == EMU_JPEG_ENCMODE_TRANSFORM) {
		// Transform mode, uiUVSize is real size * 2, uiUVHeight is real size * 2
		if (emu_jpegReadSrcYUVFile(uiYSize, uiUVSize >> 1, uiYWidth, uiYHeight, uiUVWidth, uiUVHeight >> 1) == FALSE) {
			return FALSE;
		}
	} else {
		if (emu_jpegReadSrcYUVFile(uiYSize, uiUVSize, uiYWidth, uiYHeight, uiUVWidth, uiUVHeight) == FALSE) {
			return FALSE;
		}
	}

	UINT32 freq;
	UINT32 freq2;

       freq = 480;

       kdrv_vdoencjpg_set(0, VDOENC_SET_JPEG_FREQ, &freq);

	   kdrv_vdoencjpg_get(0, VDOENC_GET_JPEG_FREQ, &freq2);
	   printk("VDOENC_GET_JPEG_FREQ = %d\r\n", (int)(freq2));

#if 1//KDRV_JPG
//jpeg_open();
kdrv_videoencjpg_open(0,KDRV_VIDEOCDC_ENGINE_JPEG);
            enc_info.encode_width=uiYWidth;
            enc_info.encode_height=uiYHeight;
            enc_info.in_fmt=KDRV_JPEGYUV_FORMAT_420;
            enc_info.retstart_interval = pEncRegSet->JPEGEncRegData.RSTR_NUM;

            //kdrv_vdoencjpg_set(0, VDOENC_SET_JPEG, (void *)&enc_info);


            enc_obj.encode_width=uiYWidth;
            enc_obj.encode_height=uiYHeight;
            enc_obj.in_fmt=KDRV_JPEGYUV_FORMAT_420;
            enc_obj.retstart_interval = pEncRegSet->JPEGEncRegData.RSTR_NUM;
            enc_obj.quality= pEncRegSet->QP;
            enc_obj.y_addr= EncBuf.uiYBuf;
            enc_obj.c_addr= EncBuf.uiUVBuf;
            enc_obj.y_line_offset= uiYWidth + uiEncExtraLineOffset;
            enc_obj.c_line_offset= uiUVWidth + uiEncExtraLineOffset;
            enc_obj.bs_addr_1= EncBuf.uiBSBuf;
            enc_obj.bs_size_1= uiBSLen;


   			//if (kdrv_videoencjpg_trigger(0, &enc_obj, &callback, &i) != 0)
   			if (kdrv_videoencjpg_trigger(0, &enc_obj, NULL, &i) != 0)
				printk("encode error\r\n");
//jpeg_close();

#else
// Open JPEG driver
	jpeg_open();

	// Set encode Huffman and Q table
	//emu_jpegSetEncTable();
	//(2) Set encode Huffman and Q table
	//emu_jpegSetEncTable();
	jpeg_enc_set_hufftable((UINT16 *)gstd_enc_lum_actbl, (UINT16 *)gstd_enc_lum_dctbl,
						   (UINT16 *)gstd_enc_chr_actbl, (UINT16 *)gstd_enc_chr_dctbl);


	printk("pEncRegSet->QP = %ld\r\n", (long)(pEncRegSet->QP));
	//jpg_set_qtable(pEncRegSet->QP, QTblY, QTblUV);  //QP must modify
	jpg_async_set_qtable(pEncRegSet->QP, QTblY, QTblUV);  //QP must modify

	// Set format
	jpeg_set_format(uiYWidth, uiYHeight, YUVFormat);

	printk("EncBuf.uiYBuf = 0x%x\r\n", (unsigned int)(EncBuf.uiYBuf));
	printk("EncBuf.uiUVBuf = 0x%x\r\n", (unsigned int)(EncBuf.uiUVBuf));

	// Set source data address
	jpeg_set_imgstartaddr(EncBuf.uiYBuf, EncBuf.uiUVBuf, 0);

	// Set line offset
	jpeg_set_imglineoffset(uiYWidth + uiEncExtraLineOffset, uiUVWidth + uiEncExtraLineOffset, 0);

	// Set format transform, jpeg_open() will disable format transform
	if (uiEncMode == EMU_JPEG_ENCMODE_TRANSFORM) {
		jpeg_set_fmttransenable();
	}

	// Set restart marker, jpeg_open() will disable restart marker
	if (uiEncMode == EMU_JPEG_ENCMODE_RESTART) {
		// Set restart marker MCU number
		jpeg_set_restartinterval(pEncRegSet->JPEGEncRegData.RSTR_NUM);
		// Enable restart marker function
		jpeg_set_restartenable(TRUE);
	}

	// Set DC out, jpeg_open() will disable DC out
	if (DCOutMode != DC_DISABLE) {
		DCOutCfg.dc_enable       = TRUE;
		DCOutCfg.dc_xratio       = uiDCXRatio;
		DCOutCfg.dc_yratio       = uiDCYRatio;
		DCOutCfg.dc_yaddr        = EncBuf.uiDCOutYBuf;
		DCOutCfg.dc_uaddr        = EncBuf.uiDCOutUVBuf;

		jpeg_set_dcout(&DCOutCfg);
	}

	// Set encoded BS buffer and length
	// Todo: Test BS_BUF_LEN from "256" to "32MB - 1"
	jpeg_set_bsstartaddr(EncBuf.uiBSBuf, uiBSLen);

	// Enable frame end and buffer end interrupt
	jpeg_set_enableint(JPEG_INT_FRAMEEND | JPEG_INT_BUFEND);
#if 0
	// Callback before encode
	if (EncCB.Before != NULL) {
		EncCB.Before();
	}

	// Profiling
	if (bProfiling == TRUE) {
		Perf_Open();
		Perf_Mark();
	}
#endif
	// Start to encode
	jpeg_set_startencode();
#if 0
	// Abort operation
	if (bEmuJpegAbort == TRUE) {
		// Encode pattern 30 in real chip (DDR = 300MHz) took 45 ms
		// Here we delay 20 ms and abort
		Delay_DelayMs(20);

		// Abort encode
		jpeg_set_endencode();

		// Close JPEG driver
		jpeg_close();

		if (bProfiling == TRUE) {
			Perf_Close();
		}

		return TRUE;
	}
#endif

	// Wait for encode done
	while (1) {
		UINT32  int_sts;

		int_sts = jpeg_waitdone();

		// Frame end
		if (int_sts & JPEG_INT_FRAMEEND) {
			jpeg_set_endencode();
			break;
		}

#if 0
		// Slice done
		if (uiIntSts & EMU_JPEG_BIT(1)) {
		}
#endif

		// Buffer end
		if (int_sts & JPEG_INT_BUFEND) {
			// Must handle buffer end or frame end won't be issued if buffer size = bit stream length
			// Interrupt will be disabled in ISR, need to enable again
			jpeg_set_enableint(JPEG_INT_FRAMEEND | JPEG_INT_BUFEND);
			jpeg_set_bsstartaddr(jpeg_get_bscurraddr(), uiBSLen);
		}
	}

#endif

	// ===================================================================
	// Compare result
	// ===================================================================
#if 1
	// Dump some information when encode data compare fail
	if (emu_jpegEncodeCompare() == FALSE) {
#if 0
		emujpeg_msg(("BS buffer        : 0x%.8X (0x%.2X)\r\n", EncBuf.uiBSBuf, (EncBuf.uiBSBuf & EMU_JPEG_BS_BUF_ALIGN)));
		emujpeg_msg(("Ref BS buffer    : 0x%.8X\r\n", EncBuf.uiRefBSBuf));
		emujpeg_msg(("Y buffer         : 0x%.8X (0X%.2X)\r\n", EncBuf.uiYBuf, (EncBuf.uiYBuf & EMU_JPEG_SRC_BUF_ALIGN)));
		emujpeg_msg(("Y line offset    : %ld\r\n", (long)(uiYWidth + uiEncExtraLineOffset)));
		emujpeg_msg(("UV buffer        : 0x%.8X (0X%.2X)\r\n", EncBuf.uiUVBuf, (EncBuf.uiUVBuf & EMU_JPEG_SRC_BUF_ALIGN)));
		emujpeg_msg(("UV line offset   : %ld\r\n", (long)(uiUVWidth + uiEncExtraLineOffset)));
		if (DCOutMode != DC_DISABLE) {
			emujpeg_msg(("DC Y buffer      : 0x%.8X\r\n", EncBuf.uiDCOutYBuf));
			emujpeg_msg(("Ref DC Y buffer  : 0x%.8X\r\n", EncBuf.uiRefDCOutYBuf));
			emujpeg_msg(("DC Y line offset : %ld\r\n", (long)(DCOutCfg.dc_ylineoffset)));
			emujpeg_msg(("DC UV buffer     : 0x%.8X\r\n", EncBuf.uiDCOutUVBuf));
			emujpeg_msg(("Ref DC UV buffer : 0x%.8X\r\n", EncBuf.uiRefDCOutUVBuf));
			emujpeg_msg(("DC UV line offset: %ld\r\n", (long)(DCOutCfg.dc_ylineoffset)));
		}

		emu_jpegDumpRegister();
#endif
		// Close JPEG driver
		jpeg_close();

		return FALSE;
	}






	// Profiling
	//if (bProfiling == TRUE) {
	//emujpeg_msg(("JPEG encode took %ld us\r\n", (long)(Perf_GetDuration())));
	//Perf_Close();
	//}
#endif
	// Close JPEG driver
	jpeg_close();

	printk("encode pattern done~\r\n");
	printk("============================================================\r\n");
}


/*
    Encode test

    Encode test.

    @param[in] uiAddr       Available buffer address for encode test
    @param[in] uiSize       Available buffer size for encode test

    @return void
*/
void emu_jpeg_encode(UINT32 dram_addr, UINT32 dram_size)
{
	// Test mode
	UINT32  uiModeIdx;
	// Test pattern
	UINT32  uiPtnStart, uiPtnEnd;
	// Test QP
	UINT32  uiQPStart, uiQPEnd;
	// Test format
	UINT32  uiFormatNum, *pFormat;
	// Format to source YUV pattern file number
	UINT32  uiFormat2FileNum;

	// Assign register file buffer and end of buffer
	EncBuf.uiRegFileBuf = ALIGN_CEIL_4(dram_addr);
	EncBuf.uiBufEnd  = dram_addr + dram_size;
	pEncRegSet       = (PEMU_JPEG_ENC_REG)EncBuf.uiRegFileBuf;


	printk("[emu_jpeg_encode]EncBuf.uiRegFileBuf = 0x%x\r\n", (unsigned int)(EncBuf.uiRegFileBuf));

#if 0
	sprintf(cRefBSPath,  "A:\\%2.2d\\%2.2d\\%2.2d\\ecs.hex", uiEncMode, uiEncPtn, uiEncFormat);
	// Generate Register file path + name
	sprintf(cEncRegPath, "A:\\%2.2d\\%2.2d\\%2.2d\\reg.hex", uiEncMode, uiEncPtn, uiEncFormat);

	sprintf(cSrcYPath, "A:\\YUV\\%2.2d_y.raw",       uiEncPtn);
	sprintf(cSrcUPath, "A:\\YUV\\%2.2d_%2.2d_u.raw", uiEncPtn, uiFormat2FileNum);
	sprintf(cSrcVPath, "A:\\YUV\\%2.2d_%2.2d_v.raw", uiEncPtn, uiFormat2FileNum);
#endif

	sprintf(cEncRegPath, "/mnt/sd/reg.hex");

	uiEncMode = EMU_JPEG_ENCMODE_NORMAL;

	uiPtnStart  = 2;
	uiPtnEnd    = 63;

	// Test all patterns
	for (uiEncPtn = uiPtnStart; uiEncPtn <= uiPtnEnd; uiEncPtn++) {
		// DIS mode don't test pattern 62
		if ((uiEncMode == EMU_JPEG_ENCMODE_DIS) &&
			(uiEncPtn == 62)) {
			continue;
		}

		// Different test mode has different test format
		switch (uiEncMode) {
		case EMU_JPEG_ENCMODE_DIS:
			pFormat = uiFmtDesInDIS;
			uiFormatNum = sizeof(uiFmtDesInDIS) / sizeof(UINT32);
			break;

		case EMU_JPEG_ENCMODE_TRANSFORM:
			pFormat = uiFmtDesInTransform;
			uiFormatNum = sizeof(uiFmtDesInTransform) / sizeof(UINT32);
			break;

		default:
			pFormat = uiFmtDesInEncOthers;
			uiFormatNum = sizeof(uiFmtDesInEncOthers) / sizeof(UINT32);
			break;
		}

		// Test all supported MCU formats
		while (uiFormatNum--) {

			uiEncFormat = *pFormat++;
			//printk("uiFormatNum = %d\r\n", (int)(uiEncFormat));

			// Pattern 1 (8x8) only test EMU_JPEG_MCU_100 format
			if ((uiEncPtn == 1) &&
				(uiEncFormat != EMU_JPEG_MCU_100)) {
				continue;
			}

			// Todo: Confirm the pattern
			// Pattern 62 only have EMU_JPEG_MCU_100, EMU_JPEG_MCU_111 and EMU_JPEG_MCU_2v11 format
			if ((uiEncPtn == 62) &&
				(uiEncFormat != EMU_JPEG_MCU_100) &&
				(uiEncFormat != EMU_JPEG_MCU_111) &&
				(uiEncFormat != EMU_JPEG_MCU_2v11)) {
				continue;
			}

			// Todo: Confirm the pattern
			// Pattern 63 only have EMU_JPEG_MCU_100, EMU_JPEG_MCU_111 and EMU_JPEG_MCU_2h11 format
			if ((uiEncPtn == 63) &&
				(uiEncFormat != EMU_JPEG_MCU_100) &&
				(uiEncFormat != EMU_JPEG_MCU_111) &&
				(uiEncFormat != EMU_JPEG_MCU_2h11)) {
				continue;
			}


			if (uiEncMode == EMU_JPEG_ENCMODE_NORMAL) {
				uiQPStart = 1;
				uiQPEnd = 100;
			} else if (uiEncMode == EMU_JPEG_ENCMODE_RESTART) {
				uiQPStart = 1;
				uiQPEnd = 17;
			} else {
				uiQPStart = 1;
				uiQPEnd = 1;
			}

			// for(uiEncQP = uiQPStart; uiEncQP <= uiQPEnd; uiEncQP++)
			{
				// DIS mode use the same pattern as normal mode
				if (uiEncMode == EMU_JPEG_ENCMODE_DIS) {
					// Generate reference BS file path + name
					sprintf(cRefBSPath,  "/mnt/sd/%2.2d/%2.2d/%2.2d/ecs.hex", EMU_JPEG_ENCMODE_NORMAL, uiEncPtn, uiEncFormat);
					// Generate Register file path + name
					sprintf(cEncRegPath, "/mnt/sd%2.2d/%2.2d/%2.2d/reg.hex", EMU_JPEG_ENCMODE_NORMAL, uiEncPtn, uiEncFormat);
				} else if (uiEncMode == EMU_JPEG_ENCMODE_TRANSFORM) {
					// Generate reference BS file path + name
					sprintf(cRefBSPath,  "/mnt/sd/%2.2d/%2.2d/%2.2d/ecs.hex", uiEncMode, uiEncPtn, uiEncFormat);
					// Generate Register file path + name
					sprintf(cEncRegPath, "/mnt/sd/%2.2d/%2.2d/%2.2d/reg.hex", uiEncMode, uiEncPtn, uiEncFormat);
				} else {
					// Generate reference BS file path + name
					//sprintf(cRefBSPath,  "A:\\%2.2d\\%2.2d\\%2.2d\\%d\\ecs.hex", uiEncMode, uiEncPtn, uiEncFormat,uiEncQP);
					// Generate Register file path + name
					//sprintf(cEncRegPath, "A:\\%2.2d\\%2.2d\\%2.2d\\%d\\reg.hex", uiEncMode, uiEncPtn, uiEncFormat,uiEncQP);
					sprintf(cRefBSPath,  "/mnt/sd/%2.2d/%2.2d/%2.2d/ecs.hex", uiEncMode, uiEncPtn, uiEncFormat);
					// Generate Register file path + name
					sprintf(cEncRegPath, "/mnt/sd/%2.2d/%2.2d/%2.2d/reg.hex", uiEncMode, uiEncPtn, uiEncFormat);
				}

				// Find correct folder index for each MCU format
				switch (uiEncFormat) {
				case EMU_JPEG_MCU_111:
				case EMU_JPEG_MCU_222h:
				case EMU_JPEG_MCU_222v:
					uiFormat2FileNum = 2;
					break;

				case EMU_JPEG_MCU_2h11:
				case EMU_JPEG_MCU_422v:
					uiFormat2FileNum = 5;
					break;

				case EMU_JPEG_MCU_2v11:
				case EMU_JPEG_MCU_422h:
					uiFormat2FileNum = 6;
					break;

				default:
//                case EMU_JPEG_MCU_411:
					uiFormat2FileNum = 9;
					break;
				}

				// Source Y, U, V path + file name
				if (uiEncMode == EMU_JPEG_ENCMODE_TRANSFORM) {
					sprintf(cSrcYPath, "/mnt/sd/3_YUV_3/%2.2d_y.raw", uiEncPtn);
					sprintf(cSrcUPath, "/mnt/sd/3_YUV_3/%2.2d_%2.2d_u.raw", uiEncPtn, uiFormat2FileNum);
					sprintf(cSrcVPath, "/mnt/sd/3_YUV_3/%2.2d_%2.2d_v.raw", uiEncPtn, uiFormat2FileNum);
				} else {
					sprintf(cSrcYPath, "/mnt/sd/YUV/%2.2d_y.raw", uiEncPtn);
					sprintf(cSrcUPath, "/mnt/sd/YUV/%2.2d_%2.2d_u.raw", uiEncPtn, uiFormat2FileNum);
					sprintf(cSrcVPath, "/mnt/sd/YUV/%2.2d_%2.2d_v.raw", uiEncPtn, uiFormat2FileNum);
				}

				// Read register file
				if (emu_jpegReadEncRegFile() != TRUE) {
					break;
				}


				// DC out
				// Test all DC out size
				//for (DCOutMode = DC_DISABLE; DCOutMode <= DC_8_8; DCOutMode++) {
				for (DCOutMode = DC_DISABLE; DCOutMode <= DC_DISABLE; DCOutMode++) {
					// Format is 2h11 or 411 and pattern 2 ~ 61 support DC output
					if (DCOutMode != DC_DISABLE) {
						if (((uiEncFormat != EMU_JPEG_MCU_2h11) &&
							 (uiEncFormat != EMU_JPEG_MCU_411)) ||
							((uiEncPtn == 1) ||
// The DC out reference pattern is not correct after pattern 31
// Todo: Generate correct patterns
#if 0
							 (uiEncPtn >= 62)))
#else
							 (uiEncPtn >= 31)))
#endif
						{
							continue;
						}
					}

					//emujpeg_msg(("=================\r\n"));
					//emujpeg_msg(("Encode Mode: %.2ld (%s), Pattern: %.2ld\r\n", uiEncMode, pEncModeString[uiEncMode], uiEncPtn));
					printk("Encode Mode: %.2ld (%s), Pattern: %.2ld\r\n", uiEncMode, pEncModeString[uiEncMode], uiEncPtn);

					// Test the pattern
					emu_jpegEncodePattern(TRUE);
#if 1
					// Offset value
					// Extra line offset, 0 ~ 60, word alignment
					uiEncExtraLineOffset += 4;
					uiEncExtraLineOffset &= 0x3F;

					// BS buffer byte offset, 0 ~ EMU_JPEG_BS_BUF_ALIGN
					uiEncBSBufOffset++;
					uiEncBSBufOffset &= EMU_JPEG_BS_BUF_ALIGN;

					// Y source byte offset, 0 ~ EMU_JPEG_SRC_BUF_ALIGN
					if (uiEncMode == EMU_JPEG_ENCMODE_DIS) {
						uiEncYBufOffset += 1;
					} else {
						uiEncYBufOffset += 4;
					}
					uiEncYBufOffset &= EMU_JPEG_SRC_BUF_ALIGN;

					// DC out buffer's word offset, 0x0 ~ 0xC, word alignment
					uiEncWordOffset += 4;
					uiEncWordOffset &= 0xF;
#endif
				} // End of DC out modes
			}
		} // End of MCU formats
	}

}

/*
    Compare UV data in byte unit

    Compare UV data in byte unit. Source data

    @param[in] uiSrcUAddr       Source U data address (pattern file)
    @param[in] uiSrcVAddr       Source V data address (pattern file)
    @param[in] uiDstAddr        Destination UV data address (HW output)
    @param[in] uiWidth          U or V Width (byte)
    @param[in] uiHeight         U or V Height (byte)
    @param[in] uiDstLineOffset  Destination (HW output) line offset, UV packed

    @note   In emulation case, source (pattern file) line offset is always equal to width

    @return Compare result
        - @b FALSE  : Data is not matched
        - @b TRUE   : Data is matched
*/
static BOOL emu_jpegCompareUV(UINT32 uiSrcUAddr,
							  UINT32 uiSrcVAddr,
							  UINT32 uiDstAddr,
							  UINT32 uiWidth,
							  UINT32 uiHeight,
							  UINT32 uiDstLineOffset)
{
	UINT32  i, j;
	UINT8   *pSrcUAddr, *pSrcVAddr, *pDstAddr;
	UINT8   uiSrc, uiDst;

#if 0
	if (bEmuJpegCache == TRUE) {
		// Switch to Cacheable address
		pSrcUAddr   = (UINT8 *)dma_getCacheAddr(uiSrcUAddr);
		pSrcVAddr   = (UINT8 *)dma_getCacheAddr(uiSrcVAddr);
		pDstAddr    = (UINT8 *)dma_getCacheAddr(uiDstAddr);
	} else {
		// Switch to NonCacheable address
		pSrcUAddr   = (UINT8 *)dma_getNonCacheAddr(uiSrcUAddr);
		pSrcVAddr   = (UINT8 *)dma_getNonCacheAddr(uiSrcVAddr);
		pDstAddr    = (UINT8 *)dma_getNonCacheAddr(uiDstAddr);

	}
#endif
	pSrcUAddr   = (UINT8 *)uiSrcUAddr;
	pSrcVAddr   = (UINT8 *)uiSrcVAddr;
	pDstAddr    = (UINT8 *)uiDstAddr;

	for (i = 0; i < uiHeight; i++) {
		for (j = 0; j < uiWidth; j++) {
			// Compare U
			uiSrc = *pSrcUAddr;
			uiDst = *pDstAddr;

			if (uiSrc != uiDst) {
				printk(("HW: 0x%.2X, Pattern: 0x%.2X\r\n", uiDst, uiSrc));
				printk(("U Byte %ldx%ld (%ldx%ld) compare error!\r\n", (long)(j), (long)(i), (long)(uiWidth), (long)(uiHeight)));

				// Clean and invalidate data cache
				//cpu_cleanInvalidateDCacheAll();

				return FALSE;
			}

			pDstAddr++;
			pSrcUAddr++;

			// Compare V
			uiSrc = *pSrcVAddr;
			uiDst = *pDstAddr;

			if (uiSrc != uiDst) {
				printk(("HW: 0x%.2X, Pattern: 0x%.2X\r\n", uiDst, uiSrc));
				printk(("V Byte %ldx%ld (%ldx%ld) compare error!\r\n", (long)(j), (long)(i), (long)(uiWidth), (long)(uiHeight)));

				// Clean and invalidate data cache
				//cpu_cleanInvalidateDCacheAll();

				return FALSE;
			}

			pDstAddr++;
			pSrcVAddr++;
		}

		pDstAddr += (uiDstLineOffset - (uiWidth << 1));
	}
#if 0
	if (bEmuJpegCache == TRUE) {
		// Clean and invalidate data cache
		cpu_cleanInvalidateDCacheAll();
	}
#endif
	return TRUE;
}


/*
    Read reference Y, U, V file to memory

    Read reference Y, U, V file to memory.

    @param[in] uiYSize      Y data size
    @param[in] uiUVSize     UV packed data size

    @return Read file status.
        - @b FALSE  : Read file fail
        - @b TRUE   : Read file OK
*/
static BOOL emu_jpegReadRefYUVFile(UINT32 uiYSize,
								   UINT32 uiUVSize)
{
	FST_FILE    file_hdl;
	UINT32      uiFileSize;
	INT32       iSts;

	// Read Y
	file_hdl = filesys_openfile(cRefYPath, FST_OPEN_READ);

	if (file_hdl != NULL) {
		// Read file from position 0
		uiFileSize = uiYSize;

		iSts = filesys_readfile(file_hdl,
								(UINT8 *)DecBuf.uiRefYBuf,
								&uiFileSize,
								0,
								NULL);

		// Close file
		filesys_closefile(file_hdl);

		if (iSts != FST_STA_OK) {
			printk(("Read %s error!\r\n", cRefYPath));
			return FALSE;
		}
	} else {
		printk(("Read %s error!\r\n", cRefYPath));
		return FALSE;
	}

	// Read U, V planar
	if (uiDecFormat != EMU_JPEG_MCU_100) {
#if (AUTOTEST_JPEG == ENABLE)
		// Read UV
		file_hdl = filesys_openfile(cRefUPath, FST_OPEN_READ);

		if (file_hdl != NULL) {
			// uiUVSize is UV packed size
			uiFileSize = uiUVSize;

			iSts = filesys_readfile(file_hdl,
									(UINT8 *)DecBuf.uiRefUBuf,
									&uiFileSize,
									0,
									NULL);

			// Close file
			filesys_closefile(file_hdl);

			if (iSts != FST_STA_OK) {
				printk(("Read %s error!\r\n", cRefUPath));
				return FALSE;
			}
		} else {
			printk(("Read %s error!\r\n", cRefUPath));
			return FALSE;
		}
#else
		// Read U
		file_hdl = filesys_openfile(cRefUPath, FST_OPEN_READ);

		if (file_hdl != NULL) {
			// uiUVSize is UV packed size
			uiFileSize = uiUVSize >> 1;

			iSts = filesys_readfile(file_hdl,
									(UINT8 *)DecBuf.uiRefUBuf,
									&uiFileSize,
									0,
									NULL);

			// Close file
			filesys_closefile(file_hdl);

			if (iSts != FST_STA_OK) {
				printk(("Read %s error!\r\n", cRefUPath));
				printk(("UV size\r\n", uiUVSize));
				return FALSE;
			}
		} else {
			printk(("Read %s error!\r\n", cRefUPath));
			return FALSE;
		}

		// Read V
		file_hdl = filesys_openfile(cRefVPath, FST_OPEN_READ);

		if (file_hdl != NULL) {
			// uiUVSize is UV packed size
			uiFileSize = uiUVSize >> 1;

			iSts = filesys_readfile(file_hdl,
									(UINT8 *)DecBuf.uiRefVBuf,
									&uiFileSize,
									0,
									NULL);

			// Close file
			filesys_closefile(file_hdl);

			if (iSts != FST_STA_OK) {
				printk(("Read %s error!\r\n", cRefVPath));
				return FALSE;
			}
		} else {
			printk(("Read %s error!\r\n", cRefVPath));
			return FALSE;
		}
#endif
	}

	return TRUE;
}

/*
    Read source BS file to memory

    Read source BS file to memory.

    @return Read file status.
        - @b FALSE  : Read file fail
        - @b TRUE   : Read file OK
*/
static BOOL emu_jpegReadSrcBSFile(void)
{
	FST_FILE    file_hdl;
	UINT32      uiFileSize;
	INT32       iSts;

	// Open file to read
	file_hdl = filesys_openfile(cSrcBSPath, FST_OPEN_READ);

	if (file_hdl != NULL) {
		// Read header
		uiFileSize = pDecRegSet->JPEGDecRegData.HEADER_LEN;

		iSts = filesys_readfile(file_hdl,
								(UINT8 *)DecBuf.uiHeaderBuf,
								&uiFileSize,
								0,
								NULL);

		if (iSts != FST_STA_OK) {
			// Close file
			filesys_closefile(file_hdl);

			printk(("Read %s error!\r\n", cSrcBSPath));
			return FALSE;
		}

		// Read bit stream
		//uiFileSize = pDecRegSet->JPEGDecRegData.BS_LEN;
		uiFileSize = pDecRegSet->JPEGDecRegData.BS_LEN + pDecRegSet->JPEGDecRegData.HEADER_LEN;

		iSts = filesys_readfile(file_hdl,
								(UINT8 *)DecBuf.uiBSBuf,
								&uiFileSize,
								0,
								NULL);

		// Close file
		filesys_closefile(file_hdl);
		DecBuf.uiBSBuf += pDecRegSet->JPEGDecRegData.HEADER_LEN;

		if (iSts != FST_STA_OK) {
			printk(("Read %s error!\r\n", cSrcBSPath));
			return FALSE;
		}

		return TRUE;
	} else {
		printk(("Read %s error!\r\n", cSrcBSPath));
		return FALSE;
	}
}


/*
    Compare decode result

    Compare decode result.

    @return Compare result.
        - @b FALSE  : Something wrong
        - @b TRUE   : Everything OK
*/
static BOOL emu_jpegDecodeCompare(void)
{
	BOOL    bReturn;

	bReturn = TRUE;

	// When cropping mode, CODEC_BS_LEN_OUT is not usable
	if (pDecRegSet->JPEGDecRegData.CROP_EN == FALSE) {
		// ===================================================================
		// Compare bit stream length
		// ===================================================================
		// The unit of jpeg_get_bssize() in decode mode is bit, and doesn't include 0xFFD9
		// BS_LEN include 0xFFD9

		if ((((jpeg_get_bssize() + 0x7) >> 3) + 2) != pDecRegSet->JPEGDecRegData.BS_LEN) {
			//printk(("BS Length error! (HW:0x%X, Pattern:0x%X)\r\n", (unsigned int)(jpeg_get_bssize() >> 3), (unsigned int)(pDecRegSet->JPEGDecRegData.BS_LEN - 2)));
			printk("BS Length error!\r\n");
			bReturn = FALSE;
		}

		// ===================================================================
		// Compare bit stream checksum
		// ===================================================================
		if (jpegtest_getbs_checksum() != pDecRegSet->JPEGDecRegData.BS_CHECKSUM) {
			//printk(("BS Checksum error! (HW:0x%X, Pattern:0x%X)\r\n", (unsigned int)(jpegtest_getbs_checksum()), (unsigned int)(pDecRegSet->JPEGDecRegData.BS_CHECKSUM)));
			printk("BS Checksum error!\r\n");
			bReturn = FALSE;
		}
	}

	// ===================================================================
	// Compare YUV checksum
	// ===================================================================
	// NT96650: There was a bug when DMA's bandwidth isn't enough, JPEG's YUV checksum might be incorrect.
	// We skip YUV checksum in DMA stress test
	if (jpegtest_getyuv_checksum() != pDecRegSet->JPEGDecRegData.YUV_CHECKSUM)
//    if (((bEmuJpegDMAStress == FALSE) && (bEmuJpegDMAEnDis == FALSE)) &&
//        (jpegtest_getyuv_checksum() != pDecRegSet->JPEGDecRegData.YUV_CHECKSUM))
	{
		//printk(("YUV Checksum error! (HW:0x%X, Pattern:0x%X)\r\n", (unsigned int)(jpegtest_getyuv_checksum()), (unsigned int)(pDecRegSet->JPEGDecRegData.YUV_CHECKSUM)));
		printk("YUV Checksum error !\r\n");
		bReturn = FALSE;
	}

	// ===================================================================
	// Compare YUV
	// ===================================================================
	// Compare Y
	if (emu_jpegCompareByte(DecBuf.uiRefYBuf,
							DecBuf.uiYBuf,
							uiDecYWidth,
							uiDecYHeight,
							uiDecYLineOffset) != TRUE) {
		printk(("Compare Y data error!\r\n"));
		bReturn = FALSE;
	}

#if 1

	if (uiDecFormat == EMU_JPEG_MCU_100) {
		printk(("EMU_JPEG_MCU_100  no Compare UV data \r\n"));
		return bReturn;
	}

	// Compare UV
#if (AUTOTEST_JPEG == ENABLE)
	if (emu_jpegCompareByte(DecBuf.uiRefUBuf,
							DecBuf.uiUVBuf,
							uiDecUVWidth,
							uiDecUVHeight,
							uiDecUVLineOffset) != TRUE) {
		printk(("Compare UV data error!\r\n"));
		bReturn = FALSE;
	}
#else
	if (emu_jpegCompareUV(DecBuf.uiRefUBuf,
						  DecBuf.uiRefVBuf,
						  DecBuf.uiUVBuf,
						  uiDecUVWidth >> 1,
						  uiDecUVHeight,
						  uiDecUVLineOffset) != TRUE) {
		bReturn = FALSE;
	}
#endif
#endif
	return bReturn;
}


/*
    Read decode register file to memory

    Read decode register file to memory.

    @return Read file status.
        - @b FALSE  : Read file fail
        - @b TRUE   : Read file OK
*/
static BOOL emu_jpegReadDecRegFile(void)
{
	FST_FILE    file_hdl;
	UINT32      uiFileSize;
	INT32       iSts;

	// Open file to read
	file_hdl = filesys_openfile(cDecRegPath, FST_OPEN_READ);

	if (file_hdl != NULL) {
		// Read file from position 0
		uiFileSize = 53;//sizeof(EMU_JPEG_DEC_REG);

		iSts = filesys_readfile(file_hdl,
								(UINT8 *)DecBuf.uiRegFileBuf,
								&uiFileSize,
								0,
								NULL);

		// Close file
		filesys_closefile(file_hdl);

		if (iSts != FST_STA_OK) {
			//emujpeg_msg(("Read %s error!\r\n", cDecRegPath));
			return FALSE;
		}

		return TRUE;
	} else {
		//emujpeg_msg(("Read %s error!\r\n", cDecRegPath));
		return FALSE;
	}
}


UINT32  BS_lastAddr;

/*
    Decode pattern

    Decode pattern.

    @param[in] bProfiling   Profiling decode performance or not
        - @b FALSE  : Don't profiling
        - @b TRUE   : Profiling decode performance

    @return Decode result.
        - @b FALSE  : Something wrong
        - @b TRUE   : Everything OK
*/
static BOOL emu_jpegDecodePattern(BOOL bProfiling)
{
	// Y size, and extra Y size
	UINT32  uiYSize, uiYExtraSize;

	// UV packed size, and extra UV packed size
	UINT32  uiUVSize, uiUVExtraSize;

	// MCU Y, UV width and height
	UINT32  uiMCUYWidth, uiMCUYHeight;

	// Bit stream length
	UINT32  uiBSLen;

	// Temporary address when allocating buffer
	UINT32  uiTempAddr;

	// For clearing memory
	UINT32  i, *pAddr;

	// Randomize buffer
	UINT32  uiRandom;

	// JPEG driver YUV format
	JPEG_YUV_FORMAT     YUVFormat;

	KDRV_VDODEC_PARAM   dec_obj;
	KDRV_CALLBACK_FUNC  callback;

	// ===================================================================
	// Get information
	// ===================================================================
	// Get MCU Y width, height
	switch (uiDecFormat) {
	default:
//    case EMU_JPEG_MCU_100:
//    case EMU_JPEG_MCU_111:
		uiMCUYWidth     = 8;
		uiMCUYHeight    = 8;
		break;

	case EMU_JPEG_MCU_222h:
	case EMU_JPEG_MCU_2h11:
		uiMCUYWidth     = 16;
		uiMCUYHeight    = 8;
		break;

	case EMU_JPEG_MCU_222v:
	case EMU_JPEG_MCU_2v11:
		uiMCUYWidth     = 8;
		uiMCUYHeight    = 16;
		break;

	case EMU_JPEG_MCU_422v:
	case EMU_JPEG_MCU_422h:
	case EMU_JPEG_MCU_411:
		uiMCUYWidth     = 16;
		uiMCUYHeight    = 16;
		break;
	}

	// Get decoded Y width, height (after crop & scale)
	// Use register file's setting instead of uiDecMode
	// We must make sure the register file is in correct folder


	// Cropping
	if (pDecRegSet->JPEGDecRegData.CROP_EN == TRUE) {
		uiDecYWidth     = uiMCUYWidth  * pDecRegSet->JPEGDecRegData.CROP_XB;
		uiDecYHeight    = uiMCUYHeight * pDecRegSet->JPEGDecRegData.CROP_YB;

		// Scaling
		if (pDecRegSet->JPEGDecRegData.SCAL_EN == TRUE) {
			switch (pDecRegSet->JPEGDecRegData.SCAL_MOD) {
			case EMU_JPEG_SCALE_1D8:
				uiDecYWidth  >>= 3;
				uiDecYHeight >>= 3;
				break;

			case EMU_JPEG_SCALE_1D4:
				uiDecYWidth  >>= 2;
				uiDecYHeight >>= 2;
				break;

			case EMU_JPEG_SCALE_1D2:
				uiDecYWidth  >>= 1;
				uiDecYHeight >>= 1;
				break;

			default:
//            case EMU_JPEG_SCALE_1D2_W:
				uiDecYWidth  >>= 1;
				break;
			}

			// Scaling mode, line offset must be 8 word alignment
			uiDecYLineOffset = ALIGN_CEIL_32(uiDecYWidth);
		} else {
			uiDecYLineOffset = uiDecYWidth + uiDecExtraLineOffset;
		}
	} else {
		// Scaling
		if (pDecRegSet->JPEGDecRegData.SCAL_EN == TRUE) {
			uiDecYWidth         = pDecRegSet->JPEGDecRegData.SCAL_XB;
			uiDecYHeight        = pDecRegSet->JPEGDecRegData.SCAL_YB;
			// Scaling mode, line offset must be 8 word alignment
			uiDecYLineOffset    = ALIGN_CEIL_32(uiDecYWidth);
		} else {
			uiDecYWidth         = pDecRegSet->JPEGDecRegData.IMG_XB;
			uiDecYHeight        = pDecRegSet->JPEGDecRegData.IMG_YB;
			uiDecYLineOffset    = uiDecYWidth + uiDecExtraLineOffset;
		}
	}

	// Get decoded UV width, height
	switch (uiDecFormat) {
	default:
//    case EMU_JPEG_MCU_100:
		YUVFormat       = JPEG_YUV_FORMAT_100;
		uiDecUVWidth    = 0;
		uiDecUVHeight   = 0;
		printk(("MCU: 100"));
		break;

	case EMU_JPEG_MCU_111:
		YUVFormat       = JPEG_YUV_FORMAT_111;
		uiDecUVWidth    = uiDecYWidth << 1;
		uiDecUVHeight   = uiDecYHeight;
		printk(("MCU: 111"));
		break;

	case EMU_JPEG_MCU_222h:
		YUVFormat       = JPEG_YUV_FORMAT_222;
		uiDecUVWidth    = uiDecYWidth << 1;
		uiDecUVHeight   = uiDecYHeight;
		printk(("MCU: 222h"));
		break;

	case EMU_JPEG_MCU_222v:
		YUVFormat       = JPEG_YUV_FORMAT_222V;
		uiDecUVWidth    = uiDecYWidth << 1;
		uiDecUVHeight   = uiDecYHeight;
		printk(("MCU: 222v"));
		break;

	case EMU_JPEG_MCU_2h11:
		YUVFormat       = JPEG_YUV_FORMAT_211;
		uiDecUVWidth    = uiDecYWidth;
		uiDecUVHeight   = uiDecYHeight;
		printk(("MCU: 2h11"));
		break;

	case EMU_JPEG_MCU_422v:
		YUVFormat       = JPEG_YUV_FORMAT_422V;
		uiDecUVWidth    = uiDecYWidth;
		uiDecUVHeight   = uiDecYHeight;
		printk(("MCU: 422v"));
		break;

	case EMU_JPEG_MCU_2v11:
		YUVFormat       = JPEG_YUV_FORMAT_211V;
		uiDecUVWidth    = uiDecYWidth << 1;
		uiDecUVHeight   = uiDecYHeight >> 1;
		printk(("MCU: 2v11"));
		break;

	case EMU_JPEG_MCU_422h:
		YUVFormat       = JPEG_YUV_FORMAT_422;
		uiDecUVWidth    = uiDecYWidth << 1;
		uiDecUVHeight   = uiDecYHeight >> 1;
		printk(("MCU: 422h"));
		break;

	case EMU_JPEG_MCU_411:
		YUVFormat       = JPEG_YUV_FORMAT_411;
		uiDecUVWidth    = uiDecYWidth;
		uiDecUVHeight   = uiDecYHeight >> 1;
		printk(("MCU: 411"));
		break;
	}

	// Display original width x height
	//printk((", %ldx%ld\r\n", (long)(pDecRegSet->JPEGDecRegData.IMG_XB), (long)(pDecRegSet->JPEGDecRegData.IMG_YB)));
	//printk("\r\npDecRegSet->JPEGDecRegData.IMG_XB = 0x%x\r\n", (unsigned int)(pDecRegSet->JPEGDecRegData.IMG_XB));
	printk("\r\nIMG_XB x IMG_YB = %d  x %d \r\n", (int)(pDecRegSet->JPEGDecRegData.IMG_XB), (int)(pDecRegSet->JPEGDecRegData.IMG_YB));

	// Display cropping width x height
	if (pDecRegSet->JPEGDecRegData.CROP_EN == TRUE) {
		printk(("Crop size: %ldx%ld (%ldx%ld MCU)\r\n",
				uiMCUYWidth * pDecRegSet->JPEGDecRegData.CROP_XB,
				uiMCUYHeight * pDecRegSet->JPEGDecRegData.CROP_YB,
				pDecRegSet->JPEGDecRegData.CROP_XB,
				pDecRegSet->JPEGDecRegData.CROP_YB));
	}

	// Display scaling mode and width x height
	if (pDecRegSet->JPEGDecRegData.SCAL_EN == TRUE) {
		switch (pDecRegSet->JPEGDecRegData.SCAL_MOD) {
		case EMU_JPEG_SCALE_1D8:
			printk(("Scale mode: 1/8 x 1/8\r\n"));
			break;

		case EMU_JPEG_SCALE_1D4:
			printk(("Scale mode: 1/4 x 1/4\r\n"));
			break;

		case EMU_JPEG_SCALE_1D2:
			printk(("Scale mode: 1/2 x 1/2\r\n"));
			break;

		default:
//            case EMU_JPEG_SCALE_1D2_W:
			printk(("Scale mode: 1/2 x 1\r\n"));
			break;
		}
	}

	// Y size
	uiYSize     = uiDecYWidth * uiDecYHeight;

	// UV packed size
	uiUVSize    = uiDecUVWidth * uiDecUVHeight;
	printk("\r\nuiYSize = 0x%x\r\n", (unsigned int)(uiYSize));
	printk("\r\nuiUVSize = 0x%x\r\n", (unsigned int)(uiUVSize));

	// Scaling mode
	if (pDecRegSet->JPEGDecRegData.SCAL_EN == TRUE) {
		// UV line offset
		// Scaling mode, line offset must be 8 word alignment
		uiDecUVLineOffset   = ALIGN_CEIL_32(uiDecUVWidth);
		// Y extra size
		uiYExtraSize        = (uiDecYLineOffset - uiDecYWidth) * uiDecYHeight;
		// UV packed extra size
		uiUVExtraSize       = (uiDecUVLineOffset - uiDecUVWidth) * uiDecUVHeight;
	} else {
		// UV line offset
		uiDecUVLineOffset   = uiDecUVWidth + uiDecExtraLineOffset;
		// Y extra size
		uiYExtraSize        = uiDecExtraLineOffset * uiDecYHeight;
		// UV packed extra size
		uiUVExtraSize       = uiDecExtraLineOffset * uiDecUVHeight;
//printk("\r\nuiDecUVLineOffset = 0x%x\r\n", (unsigned int)(uiDecUVLineOffset));
//printk("\r\nuiYExtraSize = 0x%x\r\n", (unsigned int)(uiYExtraSize));
//printk("\r\nuiUVExtraSize = 0x%x\r\n", (unsigned int)(uiUVExtraSize));
	}

	// ===================================================================
	// Allocate buffer
	// ===================================================================
	// Get available buffer address
	uiTempAddr = DecBuf.uiRegFileBuf + sizeof(EMU_JPEG_DEC_REG);

	// Source bit stream buffer
	// Align to XX + 1 bytes, so we can test buffer address from 0 to XX
	uiTempAddr      = (((uiTempAddr) + EMU_JPEG_BS_BUF_ALIGN) & ~EMU_JPEG_BS_BUF_ALIGN);
	uiTempAddr     += uiDecBSBufOffset;
	DecBuf.uiBSBuf  = uiTempAddr;

	// Calculate available address for next buffer
	// Minimum bit stream length of JPEG controller is EMU_JPEG_MIN_BS_LEN
	if (pDecRegSet->JPEGDecRegData.BS_LEN < EMU_JPEG_MIN_BS_LEN) {
		uiBSLen = EMU_JPEG_MIN_BS_LEN;
	} else {
		uiBSLen = pDecRegSet->JPEGDecRegData.BS_LEN;
	}
	uiTempAddr += uiBSLen;

	uiTempAddr += pDecRegSet->JPEGDecRegData.HEADER_LEN;


	// Source header buffer
	uiTempAddr          = ALIGN_CEIL_4(uiTempAddr);
	DecBuf.uiHeaderBuf  = uiTempAddr;

	// Calculate available address for next buffer
	uiTempAddr += pDecRegSet->JPEGDecRegData.HEADER_LEN;

	// Decoded Y buffer
	// Align to 16 bytes, so we can test 0x0, 0x4, 0x8, 0xC offset
	uiTempAddr      = ALIGN_CEIL_16(uiTempAddr);
	uiTempAddr     += uiDecWordOffset;
	DecBuf.uiYBuf   = uiTempAddr;

	// Calculate available address for next buffer
	uiTempAddr += (uiYSize + uiYExtraSize);

	// Decoded UV buffer
	// Align to 16 bytes, so we can test 0x0, 0x4, 0x8, 0xC offset
	uiTempAddr      = ALIGN_CEIL_16(uiTempAddr);
	uiTempAddr     += uiDecWordOffset;
	DecBuf.uiUVBuf  = uiTempAddr;

	// Calculate available address for next buffer
	uiTempAddr += (uiUVSize + uiUVExtraSize);

	// Reference Y buffer
	uiTempAddr          = ALIGN_CEIL_4(uiTempAddr);
	DecBuf.uiRefYBuf    = uiTempAddr;

	// Calculate available address for next buffer
	uiTempAddr += uiYSize;

	// Reference U buffer (Planar)
	uiTempAddr          = ALIGN_CEIL_4(uiTempAddr);
	DecBuf.uiRefUBuf    = uiTempAddr;

	// Calculate available address for next buffer
	uiTempAddr += (uiUVSize >> 1);

	// Reference V buffer (Planar)
	uiTempAddr          = ALIGN_CEIL_4(uiTempAddr);
	DecBuf.uiRefVBuf    = uiTempAddr;

	// Calculate available address for next buffer
	uiTempAddr += (uiUVSize >> 1);

	// Check buffer
	if (uiTempAddr > DecBuf.uiBufEnd) {
		printk(("%s: Buffer size is not enough! Required: 0x%.8X, Current: 0x%.8X\r\n",
				__func__,
				uiTempAddr - DecBuf.uiRegFileBuf,
				DecBuf.uiBufEnd - DecBuf.uiRegFileBuf));
		return FALSE;
	}

#if 0
	debug_msg("Buffer size: 0x%.8X\r\n", uiTempAddr - DecBuf.uiRegFileBuf);
#endif

	uiRandom = 0;

	DecBuf.uiBSBuf      += uiRandom;
	DecBuf.uiHeaderBuf  += uiRandom;
	DecBuf.uiYBuf       += uiRandom;
	DecBuf.uiUVBuf      += uiRandom;
	DecBuf.uiRefYBuf    += uiRandom;
	DecBuf.uiRefUBuf    += uiRandom;
	DecBuf.uiRefVBuf    += uiRandom;
	uiTempAddr          += uiRandom;

	// Clear memory
	pAddr = (UINT32 *)(ALIGN_FLOOR_4(DecBuf.uiBSBuf));

	i = ALIGN_CEIL_4(uiTempAddr - (UINT32)pAddr);

	memset((UINT32)pAddr, 0xDEADBEEF, i);

	// ===================================================================
	// Read reference YUV file
	// ===================================================================
	if (emu_jpegReadRefYUVFile(uiYSize, uiUVSize) == FALSE) {
		return FALSE;
	}

	// ===================================================================
	// Read bit stream file
	// ===================================================================
	if (emu_jpegReadSrcBSFile() == FALSE) {
		return FALSE;
	}

	// ===================================================================
	// Parse header and decode
	// ===================================================================
	memset((void *)&JpegCfg, 0, sizeof(JPGHEAD_DEC_CFG));
	//JpegCfg.inbuf = (UINT8 *)dma_getCacheAddr(DecBuf.uiHeaderBuf);
	JpegCfg.inbuf = (UINT8 *)DecBuf.uiHeaderBuf;

	printk("DecBuf.uiHeaderBuf = 0x%x  (0x%x)\r\n", (unsigned int)(DecBuf.uiHeaderBuf), (unsigned int)(fmem_lookup_pa(DecBuf.uiHeaderBuf)));
	printk("DecBuf.uiBSBuf = 0x%x  (0x%x)\r\n", (unsigned int)(DecBuf.uiBSBuf), (unsigned int)(fmem_lookup_pa(DecBuf.uiBSBuf)));


#if 1
  //jpeg_open();
   kdrv_videoencjpg_open(0,KDRV_VIDEOCDC_ENGINE_JPEG);
            //enc_info.encode_width=uiYWidth;
            //enc_info.encode_height=uiYHeight;
            //enc_info.in_fmt=KDRV_JPEGYUV_FORMAT_420;
            //enc_info.retstart_interval = pEncRegSet->JPEGEncRegData.RSTR_NUM;

            //kdrv_vdoencjpg_set(0, VDOENC_SET_JPEG, (void *)&enc_info);

// Set line offset
        	jpeg_set_imglineoffset(uiDecYLineOffset, uiDecUVLineOffset, 0);
			dec_obj.jpeg_hdr_addr= DecBuf.uiHeaderBuf;
			dec_obj.y_addr=DecBuf.uiYBuf;
			dec_obj.c_addr= DecBuf.uiUVBuf;
			dec_obj.bs_addr= DecBuf.uiBSBuf;
			dec_obj.bs_size= uiBSLen;


   			//if (kdrv_videoencjpg_trigger(0, &enc_obj, &callback, &i) != 0)
   			if (kdrv_videodecjpg_trigger(0, &dec_obj, NULL, &i) != 0)
				printk("encode error\r\n");
 //jpeg_close();



#else


	if (jpg_parse_header(&JpegCfg, DEC_PRIMARY, NULL) != JPG_HEADER_ER_OK) {

		//if (bEmuJpegCache == FALSE) {
		//  // Clean and invalidate data cache
		//  cpu_cleanInvalidateDCacheBlock((UINT32)JpegCfg.inbuf, (UINT32)JpegCfg.inbuf + pDecRegSet->JPEGDecRegData.HEADER_LEN);
		//}
		//
		//emujpeg_msg(("Parese JPEG header error!\r\n"));
		return FALSE;
	}

	//if (bEmuJpegCache == FALSE) {
	//  // Clean and invalidate data cache
	//  cpu_cleanInvalidateDCacheBlock((UINT32)JpegCfg.inbuf, (UINT32)JpegCfg.inbuf + pDecRegSet->JPEGDecRegData.HEADER_LEN);
	//}



	// Open JPEG driver
	jpeg_open();

	// Set Huffman table
	jpeg_set_decode_hufftabhw(JpegCfg.p_huff_dc0th, JpegCfg.p_huff_dc1th, JpegCfg.p_huff_ac0th, JpegCfg.p_huff_ac1th);

#if 0
	printk("\r\n[pQTable_Y] \r\n");
	for (i = 0; i < 64; i++) {
		printk("0x%x ", (unsigned int)(*(JpegCfg.pQTabY + i)));
	}

	printk("\r\n[pQTable_UV] \r\n");
	for (i = 0; i < 64; i++) {
		printk("0x%x ", (unsigned int)(*(JpegCfg.pQTabUV + i)));
	}
#endif

	// Set Q table
	jpeg_set_hwqtable(JpegCfg.p_q_tbl_y, JpegCfg.p_q_tbl_uv);

	// Set format (Original width and height)
	jpeg_set_format(pDecRegSet->JPEGDecRegData.IMG_XB, pDecRegSet->JPEGDecRegData.IMG_YB, YUVFormat);

	// Set output data address
	jpeg_set_imgstartaddr(DecBuf.uiYBuf, DecBuf.uiUVBuf, 0);

	// Set line offset
	jpeg_set_imglineoffset(uiDecYLineOffset, uiDecUVLineOffset, 0);

	// Set restart marker, jpeg_open() will disable restart marker
	if (uiDecMode == EMU_JPEG_DECMODE_RESTART) {
		// Set restart marker MCU number
		jpeg_set_restartinterval(JpegCfg.rstinterval);
		// Enable restart marker function
		jpeg_set_restartenable(TRUE);
	}

	// Set scale, jpeg_open() will disable scaling
	if (pDecRegSet->JPEGDecRegData.SCAL_EN == TRUE) {
		// Set scaling information
		switch (pDecRegSet->JPEGDecRegData.SCAL_MOD) {
		case EMU_JPEG_SCALE_1D8:
			jpeg_set_scaleratio(JPEG_DECODE_RATIO_BOTH_1_8);
			break;

		case EMU_JPEG_SCALE_1D4:
			jpeg_set_scaleratio(JPEG_DECODE_RATIO_BOTH_1_4);
			break;

		case EMU_JPEG_SCALE_1D2:
			jpeg_set_scaleratio(JPEG_DECODE_RATIO_BOTH_1_2);
			break;

//        case EMU_JPEG_SCALE_1D2_W:
		default:
			jpeg_set_scaleratio(JPEG_DECODE_RATIO_WIDTH_1_2);
			break;
		}

		// Enable scaling
		jpeg_set_scaleenable();
	}

	// Set crop, jpeg_open() will disable cropping
	if (pDecRegSet->JPEGDecRegData.CROP_EN == TRUE) {
		// Set crop information
		jpeg_set_crop(pDecRegSet->JPEGDecRegData.CROP_STX * uiMCUYWidth,
					 pDecRegSet->JPEGDecRegData.CROP_STY * uiMCUYHeight,
					 pDecRegSet->JPEGDecRegData.CROP_XB * uiMCUYWidth,
					 pDecRegSet->JPEGDecRegData.CROP_YB * uiMCUYHeight);

		// Enable cropping
		jpeg_set_cropenable();
	}

	// Set decoded BS buffer and length
	// Todo: Test BS_BUF_LEN from "256" to "32MB - 1"
	jpeg_set_bsstartaddr(DecBuf.uiBSBuf, uiBSLen);

	// Enable frame end and buffer end interrupt
	jpeg_set_enableint(JPEG_INT_FRAMEEND | JPEG_INT_BUFEND | JPEG_INT_DECERR);
#if 0
	// Callback before decode
	if (DecCB.Before != NULL) {
		DecCB.Before();
	}

	// Profiling
	if (bProfiling == TRUE) {
		Perf_Open();
		Perf_Mark();
	}
#endif
	// Start to decode
	jpeg_set_startdecode();


#if 0
	// Abort operation
	if (bEmuJpegAbort == TRUE) {
		// Decode pattern 43 in real chip (DDR = 300MHz) took 250 ms
		// Here we delay 100 ms and abort
		Delay_DelayMs(100);

		// Abort encode
		jpeg_set_enddecode();

		// Close JPEG driver
		jpeg_close();

		if (bProfiling == TRUE) {
			Perf_Close();
		}

		return TRUE;
	}
#endif
	// Wait for decode done
	while (1) {
		UINT32  uiIntSts;

		uiIntSts = jpeg_waitdone();

		// Decode error
		if (uiIntSts & JPEG_INT_DECERR) {
			printk(("Decode ERROR!\r\n"));
		}

#if 0
		// Slice done
		if (uiIntSts & EMU_JPEG_BIT(1)) {
		}
#endif

		// Buffer end
		if (uiIntSts & JPEG_INT_BUFEND) {
			printk("JPEG_INT_BUFEND\r\n");
			// Must handle buffer end status or frame end won't be issued if buffer size = bit stream length
			// Interrupt will be disabled in ISR, need to enable again
			jpeg_set_enableint(JPEG_INT_FRAMEEND | JPEG_INT_BUFEND);
#if 1
			// Only one buffer mode
			//jpeg_set_bsstartaddr(jpeg_get_bsstartaddr(), uiBSLen);
			jpeg_set_bsstartaddr(DecBuf.uiBSBuf, uiBSLen);


#else
			// Multiple buffer mode
			jpeg_set_bsstartaddr(jpeg_get_bscurraddr(), uiBSLen);
#endif
		}

		// Frame end
		if (uiIntSts & JPEG_INT_FRAMEEND) {
			jpeg_set_enddecode();
			break;
		}
	}

#endif

#if 0
	// Profiling
	if (bProfiling == TRUE) {
		emujpeg_msg(("JPEG decode took %ld us\r\n", (long)(Perf_GetDuration())));
		Perf_Close();
	}

	// Callback after decode
	if (DecCB.After != NULL) {
		DecCB.After();
	}
#endif
	printk("Compare result~\r\n");
	// ===================================================================
	// Compare result
	// ===================================================================
#if 1
	// Dump some information when encode data compare fail
	if (emu_jpegDecodeCompare() == FALSE) {
		//emujpeg_msg(("BS buffer     : 0x%.8X (0x%.2X)\r\n", DecBuf.uiBSBuf, (DecBuf.uiBSBuf & EMU_JPEG_BS_BUF_ALIGN)));
		printk(("Y buffer : 0x%x\r\n", (unsigned int)(DecBuf.uiYBuf)));
		//emujpeg_msg(("Y line offset : %ld\r\n", (long)(uiDecYLineOffset)));
		//emujpeg_msg(("UV buffer     : 0x%.8X\r\n", DecBuf.uiUVBuf));
		//emujpeg_msg(("UV line offset: %ld\r\n", (long)(uiDecUVLineOffset)));

		//emu_jpegDumpRegister();

		// Close JPEG driver
		jpeg_close();

		return FALSE;
	}

	// Close JPEG driver
	jpeg_close();
	printk("decode pattern done~\r\n");
	printk("============================================================\r\n");

	return TRUE;
#else
	// Dump some information regardless data compare result for debugging
	emujpeg_msg(("BS buffer     : 0x%.8X (0x%.2X)\r\n", DecBuf.uiBSBuf, (DecBuf.uiBSBuf & EMU_JPEG_BS_BUF_ALIGN)));
	emujpeg_msg(("Y buffer      : 0x%.8X\r\n", DecBuf.uiYBuf));
	emujpeg_msg(("Y line offset : %ld\r\n", (long)(uiDecYLineOffset)));
	emujpeg_msg(("UV buffer     : 0x%.8X\r\n", DecBuf.uiUVBuf));
	emujpeg_msg(("UV line offset: %ld\r\n", (long)(uiDecUVLineOffset)));

	if (emu_jpegDecodeCompare() == FALSE) {
		emu_jpegDumpRegister();

		// Close JPEG driver
		jpeg_close();

		return FALSE;
	}

	// Close JPEG driver
	jpeg_close();

	return TRUE;
#endif
}




/*
    Decode test

    Decode test.

    @param[in] uiAddr       Available buffer address for decode test
    @param[in] uiSize       Available buffer size for decode test

    @return void
*/
void emu_jpeg_decode(UINT32 dram_addr, UINT32 dram_size)
{
	// Test mode
	UINT32  uiModeIdx;
	// Test pattern
	UINT32  uiPtnStart, uiPtnEnd;
	// Test format
	UINT32  uiFormatNum, *pFormat;

	int i;
	UINT8 *pBuf;

	// Assign register file buffer and end of buffer
	DecBuf.uiRegFileBuf = ALIGN_CEIL_4(dram_addr);
	DecBuf.uiBufEnd     = dram_addr + dram_size;
	pDecRegSet          = (PEMU_JPEG_DEC_REG)DecBuf.uiRegFileBuf;

	//printk("[emu_jpeg_decode]DecBuf.uiRegFileBuf = 0x%x\r\n", (unsigned int)(DecBuf.uiRegFileBuf));

	// Select test mode
	//for (uiModeIdx = 0; uiModeIdx < (sizeof(uiDecTestMode) / sizeof(UINT32)); uiModeIdx++) {
	for (uiModeIdx = 0; uiModeIdx < 1; uiModeIdx++) {
		uiDecMode = uiDecTestMode[uiModeIdx];

		// Different test mode has different test pattern
		switch (uiDecMode) {
		// Scaling 1/2 width only
		case EMU_JPEG_DECMODE_SCALE1D2_W:
			uiPtnStart  = 2;
			uiPtnEnd    = 61;
			break;

		// Others
		default:
			uiPtnStart  = 2;
			uiPtnEnd    = 30;
			break;
		}

		// Test all patterns
		for (uiDecPtn = uiPtnStart; uiDecPtn <= uiPtnEnd; uiDecPtn++) {
			// Different test mode has different test format
			switch (uiDecMode) {
			case EMU_JPEG_DECMODE_SCALE1D2_W:
				pFormat = uiFmtDesInScale1D2W;
				uiFormatNum = sizeof(uiFmtDesInScale1D2W) / sizeof(UINT32);
				break;

			default:
				pFormat = uiFmtDesInDecOthers;
				uiFormatNum = sizeof(uiFmtDesInDecOthers) / sizeof(UINT32);
				break;
			}

			// Test all supported MCU formats
			while (uiFormatNum--) {
				uiDecFormat = *pFormat++;

				// Pattern 1 (8x8), 62 (65528x32) only test EMU_JPEG_MCU_100 format
				if (((uiDecPtn == 1) ||
					 (uiDecPtn == 62)) &&
					(uiDecFormat != EMU_JPEG_MCU_100)) {
					continue;
				}

				// Pattern 63 (32x65528) only test EMU_JPEG_MCU_100 and EMU_JPEG_MCU_2h11
				if ((uiDecPtn == 63) &&
					((uiDecFormat != EMU_JPEG_MCU_100) &&
					 (uiDecFormat != EMU_JPEG_MCU_2h11))) {
					continue;
				}

				// Source BS file path + name, Restart marker use different source
				if (uiDecMode == EMU_JPEG_DECMODE_RESTART) {
					sprintf(cSrcBSPath,  "/mnt/sd/JPEG_RST/%02d_%02d.jpg", uiDecPtn, uiDecFormat);
				} else {
					sprintf(cSrcBSPath,  "/mnt/sd/JPEG/%02d_%02d.jpg",     uiDecPtn, uiDecFormat);
				}

				// Register file path + name
				sprintf(cDecRegPath, "/mnt/sd/DEC_%02d/%02d/%02d/reg.hex", uiDecMode, uiDecPtn, uiDecFormat);

				// Reference YUV file path + name
				sprintf(cRefYPath, "/mnt/sd/DEC_%02d/%02d/%02d/Y.raw", uiDecMode, uiDecPtn, uiDecFormat);
				sprintf(cRefUPath, "/mnt/sd/DEC_%02d/%02d/%02d/U.raw", uiDecMode, uiDecPtn, uiDecFormat);
				sprintf(cRefVPath, "/mnt/sd/DEC_%02d/%02d/%02d/V.raw", uiDecMode, uiDecPtn, uiDecFormat);

				// Read register file
				if (emu_jpegReadDecRegFile() != TRUE) {
					printk("Read register file fail\r\n");
					break;
				}

//pBuf = (UINT8 *)pDecRegSet;

//for(i=0;i<0x40;i=i+4)
//    printk("data = 0x%x 0x%x 0x%x 0x%x\r\n", (unsigned int)(*(pBuf+i)), (unsigned int)(*(pBuf+1+i)), (unsigned int)(*(pBuf+2+i)), (unsigned int)(*(pBuf+3+i)));


				// For crop mode EMU_JPEG_MCU_100, CROP_XB must >= 2
				if ((uiDecFormat == EMU_JPEG_MCU_100) &&
					(pDecRegSet->JPEGDecRegData.CROP_EN == TRUE) &&
					(pDecRegSet->JPEGDecRegData.CROP_XB < 2)) {
					printk("For crop mode EMU_JPEG_MCU_100, CROP_XB must bigger than 2\r\n");
					continue;
				}

				//emujpeg_msg(("=================\r\n"));
				//emujpeg_msg(("Decode Mode: %.2ld (%s), Pattern: %.2ld\r\n", uiDecMode, pDecModeString[uiDecMode], uiDecPtn));

				// Test the pattern
				emu_jpegDecodePattern(FALSE);

				// Offset value
				// Extra line offset, 0 ~ 60, word alignment
				uiDecExtraLineOffset += 4;
				uiDecExtraLineOffset &= 0x3F;

				// BS buffer byte offset, 0 ~ EMU_JPEG_BS_BUF_ALIGN
				uiDecBSBufOffset++;
				uiDecBSBufOffset &= EMU_JPEG_BS_BUF_ALIGN;

				// Decoded Y, UV buffer's word offset, 0x0 ~ 0xC, word alignment
				uiDecWordOffset += 4;
				uiDecWordOffset &= 0xF;
			} // End of MCU formats
		} // End of patterns
	} // End of modes

}

