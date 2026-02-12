#include "../hal/jpg_int.h"
#include "../../include/jpg_enc.h"
#include "../../include/jpg_dec.h"
#include "../../include/jpg_header.h"
#include "jpeg_file.h"
#include <kwrap/file.h>
#define __KERNEL_SYSCALLS__

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/vfs.h>
#include <linux/uaccess.h>
#include <mach/fmem.h>
#endif

static JPG_DEC_INFO     g_dec_one_info;
static JPGHEAD_DEC_CFG     g_header_dec_cfg;

#define TEST_FILE_PATH "/mnt/sd/TEST0001.JPG"

#define TEMP_BUF_SIZE  0x3000000

#define FILE_BUF_SIZE 0x800000

static UINT32 RawBuf;


static void save_result_to_sd(CHAR *out_path, UINT32 addr, UINT32 size)
{
#ifdef __KERNEL__
	//struct file *fp;
	int fp;
	mm_segment_t old_fs;
	int len = 0;

	//fp = filp_open(out_path, O_CREAT | O_WRONLY | O_SYNC, 0);
	fp = vos_file_open(out_path, O_CREAT | O_WRONLY | O_SYNC, 0);
	//if (IS_ERR_OR_NULL(fp)) {
	if(-1 == fp) {
		printk("failed in file open:%s\n", out_path);
		return;
	}
	old_fs = get_fs();
	set_fs(get_ds());
	//len = vfs_write(fp, (void *)addr, size, &fp->f_pos);
	len = vos_file_read(fp, (void *)addr, size);
	//filp_close(fp, NULL);
	vos_file_close(fp);
	set_fs(old_fs);
#endif
}

void exam_jpg_dal_encode(void)
{
	DAL_VDOENC_INIT dal_enc_init;
	DAL_VDOENC_PARAM enc_param;
	ER              er_code;
	UINT32 i;

	DBG_FUNC("\r\n");
	memset((VOID *)&dal_enc_init, 0, sizeof(dal_enc_init));
	dal_enc_init.uiWidth = g_dec_one_info.p_dec_cfg->imagewidth;
	dal_enc_init.uiHeight = g_dec_one_info.p_dec_cfg->imageheight;
	dal_enc_init.uiFrameRate = 30;
	dal_enc_init.uiByteRate = dal_enc_init.uiWidth*dal_enc_init.uiHeight*30/10;
	dal_enc_init.uiYuvFormat = g_dec_one_info.p_dec_cfg->fileformat;
	dal_jpegenc_init(DAL_VDOENC_ID_1, &dal_enc_init);

	for (i=0; i<5; i++) {
		enc_param.uiYAddr = g_dec_one_info.out_addr_y;
		enc_param.uiUVAddr = g_dec_one_info.out_addr_uv;
		enc_param.uiYLineOffset = g_dec_one_info.p_dec_cfg->lineoffset_y;
		enc_param.uiUVLineOffset = g_dec_one_info.p_dec_cfg->lineoffset_uv;
		enc_param.uiBsAddr = RawBuf + TEMP_BUF_SIZE - FILE_BUF_SIZE;
		enc_param.uiBsEndAddr = RawBuf + TEMP_BUF_SIZE;
		enc_param.uiQuality = 80;
		er_code = dal_jpegenc_encodeone(DAL_VDOENC_ID_1, &enc_param);
		if (er_code != E_OK) {
			DBG_ERR("Encode failed!(%d), Bitstream = %d", er_code, enc_param.uiBsSize);

		} else {
			//save result
			CHAR out_path[36];

			snprintf(out_path, sizeof(out_path), "//mnt//sd//DAL_OUT%d.JPG", i);

			DBG_DUMP("%s,  Addr = 0x%08X, size = %d\r\n", out_path, enc_param.uiBsAddr, enc_param.uiBsSize);
			save_result_to_sd(out_path, enc_param.uiBsAddr, enc_param.uiBsSize);
		}
	}

	dal_jpegenc_close(DAL_VDOENC_ID_1);



}
void exam_jpg_encode(void)
{
	JPG_ENC_CFG EXIFParam;
	UINT32 EXIFBufSize = FILE_BUF_SIZE, EXIFBufAddr, uiPoolAddr, Quality = 90;
	ER EncResult;

	//sscanf_s(strCmd, "%d %d %x", &Quality, &DCDownLevel, &EXIFBufSize);

	DBG_FUNC("EncByQuality Quality=%d,  EXIFBufSize=0x%X\r\n", (int)(Quality), (unsigned int)(EXIFBufSize));

 	memset((VOID *)&EXIFParam, 0, sizeof(EXIFParam));

	uiPoolAddr = RawBuf;

	EXIFParam.image_addr[0] =   g_dec_one_info.out_addr_y;
	EXIFParam.image_addr[1] =  g_dec_one_info.out_addr_uv;
	EXIFBufAddr = uiPoolAddr + TEMP_BUF_SIZE - FILE_BUF_SIZE;
	EXIFParam.bs_addr = EXIFBufAddr;
	EXIFParam.p_bs_buf_size = &EXIFBufSize;
	EXIFParam.width = g_dec_one_info.p_dec_cfg->imagewidth;
	EXIFParam.height = g_dec_one_info.p_dec_cfg->imageheight;
	EXIFParam.lineoffset[0]    = g_dec_one_info.p_dec_cfg->lineoffset_y;
	EXIFParam.lineoffset[1] = g_dec_one_info.p_dec_cfg->lineoffset_uv;//gDecodedImgBuf.LineOffset[1];
	EXIFParam.yuv_fmt = g_dec_one_info.p_dec_cfg->fileformat;
	EncResult = jpg_enc_one_img(&EXIFParam, Quality);
	if (EncResult != E_OK) {
		DBG_ERR("Encode failed!(%d), Bitstream = %d", EncResult, *EXIFParam.p_bs_buf_size);

	} else {
		//save result
		CHAR out_path[] = "//mnt//sd//TEST_OUT.JPG";

		DBG_DUMP("Bitstream Addr = 0x%08X, size = %d\r\n", EXIFParam.bs_addr, *EXIFParam.p_bs_buf_size);
		save_result_to_sd(out_path, EXIFParam.bs_addr, *EXIFParam.p_bs_buf_size);
	}
}

#ifdef __KERNEL__
void exam_jpg_dec_primary(void)
{
	//UINT32 Curindex = 1, EncodeResult = 0, uiFileNum, uiDirNum, uiFileType;
	CHAR    src_path[36] = { 0 };
	UINT32 RawBufSize, FileBuf;
	ER Ret;
//[temporary mark]	frammap_buf_t      buf_info ={0};
	FST_FILE    fileHdl;
	UINT32      uiFileSize;
	INT32       iSts;
	struct kstat stat;

	DBG_FUNC("\r\n");
	//sscanf_s(strCmd, "%d %d %d %d", &Curindex, &m_uiTestScaleDownLevel, &EncodeResult, &EnableTimeOut);

	memset((VOID *)&g_dec_one_info, 0, sizeof(g_dec_one_info));
	memset((VOID *)&g_header_dec_cfg, 0, sizeof(g_header_dec_cfg));

//[temporary mark]
#if 0
	//Allocate memory
    buf_info.size = TEMP_BUF_SIZE;
    buf_info.align = 64;      ///< address alignment
    buf_info.name = "nvtmpp";
    buf_info.alloc_type = ALLOC_CACHEABLE;
    frm_get_buf_ddr(DDR_ID0, &buf_info);

    DBG_DUMP("buf_info.va_addr = 0x%08X, buf_info.phy_addr = 0x%08X\r\n", (UINT32)buf_info.va_addr, (UINT32)buf_info.phy_addr);

	RawBuf = (UINT32)buf_info.va_addr;
#else
	RawBuf = 0x500000;
#endif


	snprintf(src_path, sizeof(src_path), TEST_FILE_PATH);

	fileHdl = filesys_openfile(src_path, FST_OPEN_READ);
	if (fileHdl != NULL) {
		mm_segment_t old_fs;

        stat.size = 0; //for coverity (init setting)
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		if(vfs_stat(src_path, &stat)) {
			printk("get size of %s error!\r\n", src_path);
			filesys_closefile(fileHdl);
			goto JPG_DEC_PRI_END;
		}
		set_fs(old_fs);
		//DBGD(stat.size);

		// Read file from position 0
		uiFileSize = stat.size;
		FileBuf =  ALIGN_FLOOR_4(RawBuf + TEMP_BUF_SIZE - uiFileSize);

		iSts = filesys_readfile(fileHdl,
								(UINT8 *)FileBuf,
								&uiFileSize,
								0,
								NULL);

		// Close file
		filesys_closefile(fileHdl);

		if (iSts != FST_STA_OK) {
			printk("Read %s error!\r\n", src_path);
			goto JPG_DEC_PRI_END;
		}
	} else {
		printk("Read %s error!\r\n", src_path);
		goto JPG_DEC_PRI_END;
	}


	RawBufSize = FileBuf - RawBuf ;
	printk("RawBufSize=%dKB\r\n", (int)(RawBufSize/1024));

	g_dec_one_info.p_dst_addr          = (UINT8 *)RawBuf;
	g_dec_one_info.p_dec_cfg           = &g_header_dec_cfg;
	g_dec_one_info.decode_type     = DEC_PRIMARY;
	g_dec_one_info.p_src_addr      = (UINT8 *)FileBuf;
	g_dec_one_info.jpg_file_size   = uiFileSize;
	Ret = jpg_dec_one(&g_dec_one_info);
	if (E_OK == Ret) {
		#if 1
		CHAR out_path[] = "//mnt//sd//TEST_OUT.YUV";
		UINT32 out_size;

		printk("WxH=%d x %d, LineOffsetY=%d, LineOffsetUV=%d, FileFormat=%x\r\n", (int)(g_dec_one_info.p_dec_cfg->imagewidth), (int)(g_dec_one_info.p_dec_cfg->imageheight), (int)(g_dec_one_info.p_dec_cfg->lineoffset_y), (int)(g_dec_one_info.p_dec_cfg->lineoffset_uv), (unsigned int)(g_dec_one_info.p_dec_cfg->fileformat));
		printk("PxlAddrY=0x%X, PxlAddr_UV=0x%X \r\n", (unsigned int)(g_dec_one_info.out_addr_y), (unsigned int)(g_dec_one_info.out_addr_uv));
		out_size = g_dec_one_info.p_dec_cfg->lineoffset_y * g_dec_one_info.p_dec_cfg->imageheight * 2;

		save_result_to_sd(out_path, g_dec_one_info.out_addr_y, out_size);

		#else
		//save result
		CHAR *out_path = "//mnt//sd//TEST_OUT.YUV";
		//struct file *fp;
		int fp;
		UINT32 out_size;
		mm_segment_t old_fs;

		printk("WxH=%d x %d, LineOffsetY=%d, LineOffsetUV=%d, FileFormat=%x\r\n", (int)(g_dec_one_info.p_dec_cfg->imagewidth), (int)(g_dec_one_info.p_dec_cfg->imageheight), (int)(g_dec_one_info.p_dec_cfg->lineoffset_y), (int)(g_dec_one_info.p_dec_cfg->lineoffset_uv), (unsigned int)(g_dec_one_info.p_dec_cfg->fileformat));
		printk("PxlAddrY=0x%X, PxlAddr_UV=0x%X \r\n", (unsigned int)(g_dec_one_info.out_addr_y), (unsigned int)(g_dec_one_info.out_addr_uv));

		out_size = g_dec_one_info.p_dec_cfg->lineoffset_y * g_dec_one_info.p_dec_cfg->imageheight * 2;

		//fp = filp_open(out_path, O_CREAT | O_WRONLY | O_SYNC, 0);
		fp = vos_file_open(out_path, O_CREAT | O_WRONLY | O_SYNC, 0);
		//if (IS_ERR_OR_NULL(fp)) {
		if(-1==fp) {
			printk("failed in file open:%s\n", out_path);
			goto JPG_DEC_PRI_END;
		}
		old_fs = get_fs();
		set_fs(get_ds());
		//len = vfs_write(fp, (void *)g_dec_one_info.out_addr_y, out_size, &fp->f_pos);
		len = vos_file_write(fp, (void *)g_dec_one_info.out_addr_y, out_size);
		//filp_close(fp, NULL);
		vos_file_close(fp);
		set_fs(old_fs);
		#endif
	} else {
		printk("Decode Error! Ret=0x%X\r\n", (unsigned int)(Ret));
	}

JPG_DEC_PRI_END:
	//exam_jpg_encode();
	exam_jpg_dal_encode();

//[temporary mark]
#if 0
	frm_free_buf_ddr((void *)buf_info.va_addr);
#endif
}
#endif

