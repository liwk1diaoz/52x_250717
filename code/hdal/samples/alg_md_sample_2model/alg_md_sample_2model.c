#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"
#include <kwrap/examsys.h>
#include "math.h"
#include "vendor_md.h"

#if defined(__LINUX)
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/task.h>
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    vos_task_delay_ms(1000*x)
#define msleep(x)   vos_task_delay_ms(x)
#define usleep(x)   vos_task_delay_us(x)
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(alg_md_sample, argc, argv)
#endif

//#define IMG_WIDTH         640//160
//#define IMG_HEIGHT        480//120
//#define IMG_BUF_SIZE      (IMG_WIDTH * IMG_HEIGHT)
#define MDBC_ALIGN(a, b) (((a) + ((b) - 1)) / (b) * (b))
#define OUTPUT_BMP 		0
#define DEBUG_FILE		1
#define MD_RUN_MODEL_NUM	2
UINT32 IMG_WIDTH[MD_RUN_MODEL_NUM];
UINT32 IMG_HEIGHT[MD_RUN_MODEL_NUM];
UINT32 IMG_BUF_SIZE[MD_RUN_MODEL_NUM];
UINT32 g_LumDiff[MD_RUN_MODEL_NUM];
UINT32 g_FrmID[MD_RUN_MODEL_NUM];
UINT32 g_Rnd[MD_RUN_MODEL_NUM];

typedef struct _MEM_RANGE {
	UINT32               va;        ///< Memory buffer starting address
	UINT32               addr;      ///< Memory buffer starting address
	UINT32               size;      ///< Memory buffer size
	HD_COMMON_MEM_VB_BLK blk;
} MEM_RANGE, *PMEM_RANGE;

typedef enum {
	MODEL0_PARA       = 0,
	MODEL1_PARA       = 1,
	ENUM_DUMMY4WORD(MDBC_PARA_SENSI)
} MDBC_PARA_SENSI;

#pragma pack(2)
struct BmpFileHeader {
    UINT16 bfType;
    UINT32 bfSize;
    UINT16 bfReserved1;
    UINT16 bfReserved2;
    UINT32 bfOffBits;
};
struct BmpInfoHeader {
    UINT32 biSize;
    UINT32 biWidth;
    UINT32 biHeight;
    UINT16 biPlanes; // 1=defeaul, 0=custom
    UINT16 biBitCount;
    UINT32 biCompression;
    UINT32 biSizeImage;
    UINT32 biXPelsPerMeter; // 72dpi=2835, 96dpi=3780
    UINT32 biYPelsPerMeter; // 120dpi=4724, 300dpi=11811
    UINT32 biClrUsed;
    UINT32 biClrImportant;
};
#pragma pack()

UINT32 g_md_sensi = MODEL0_PARA;

static INT32 share_memory_init(MEM_RANGE (*p_share_mem)[11])
{
	HD_COMMON_MEM_VB_BLK blk;
	int i,j;
	UINT32 pa, va;
	UINT32 blk_size;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_RESULT ret = HD_OK;
	
	for(j=0;j<MD_RUN_MODEL_NUM;j++){
		for(i=0;i<11;i++){
			p_share_mem[j][i].addr = 0x00;
			p_share_mem[j][i].va   = 0x00;
			p_share_mem[j][i].size = 0x00;
			p_share_mem[j][i].blk  = HD_COMMON_MEM_VB_INVALID_BLK;
		}
	}
	for(j=0;j<MD_RUN_MODEL_NUM;j++){
		printf("\r\n----- j = %d -----\r\n", j);
		for(i=0;i<11;i++){
			if(i==0)blk_size = IMG_BUF_SIZE[j];
			else if(i==1 || i==2)blk_size = IMG_BUF_SIZE[j]/2;
			else if(i==3 || i==7)blk_size = IMG_BUF_SIZE[j]*40;
			else if(i==4 || i==8)blk_size = IMG_BUF_SIZE[j]*6;
			else if(i==5 || i==9)blk_size = ((IMG_BUF_SIZE[j]+15)/16)*16*12;
			else if(i==6)blk_size = ((IMG_BUF_SIZE[j]+7)/8);
			else if(i==10)blk_size = IMG_BUF_SIZE[j]; // result transform
			else blk_size = 0; 
			blk = hd_common_mem_get_block(HD_COMMON_MEM_GLOBAL_MD_POOL, blk_size, ddr_id);
			if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
				printf("err:get block fail\r\n", blk);
				ret =  HD_ERR_NG;
				return ret;
			}
			pa = hd_common_mem_blk2pa(blk);
			if (pa == 0) {
				printf("err:blk2pa fail, blk = 0x%x\r\n", blk);
				goto blk2pa_err;
			}
			//printf("pa = 0x%x\r\n", pa);
			if (pa > 0) {
				va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size);
				if (va == 0) {
					goto map_err;
				}
			}
			p_share_mem[j][i].addr = pa;
			p_share_mem[j][i].va   = va;
			p_share_mem[j][i].size = blk_size;
			p_share_mem[j][i].blk  = blk;
			printf("share_mem[%d] pa = 0x%x, va=0x%x, size =0x%x\r\n",i, p_share_mem[j][i].addr, p_share_mem[j][i].va, p_share_mem[j][i].size);
		}
	}
	return ret;
blk2pa_err:
map_err:
	for (; j >= 0 ;) {
		for (; i > 0 ;) {
			i -= 1;
			ret = hd_common_mem_release_block(p_share_mem[j][i].blk);
			if (HD_OK != ret) {
				printf("err:release blk fail %d\r\n", ret);
				ret =  HD_ERR_NG;
				return ret;
			}
		}
		j -= 1;
		i = 11;
	}
	return ret;
}

static INT32 share_memory_exit(MEM_RANGE (*p_share_mem)[11])
{
	UINT8 i,j;
	for(j=0;j<MD_RUN_MODEL_NUM;j++){
		for(i=0;i<11;i++){
			if (p_share_mem[j][i].va != 0) {
				hd_common_mem_munmap((void *)p_share_mem[j][i].va, p_share_mem[j][i].size);
			}
			if (p_share_mem[j][i].blk != HD_COMMON_MEM_VB_INVALID_BLK) {
				hd_common_mem_release_block(p_share_mem[j][i].blk);
			}
			p_share_mem[j][i].addr = 0x00;
			p_share_mem[j][i].va   = 0x00;
			p_share_mem[j][i].size = 0x00;
			p_share_mem[j][i].blk  = HD_COMMON_MEM_VB_INVALID_BLK;
		}
	}
	return HD_OK;
}

static HD_RESULT mem_init(void)
{
	HD_RESULT ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
	
	// model 0 dram mem allocate
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[0].blk_size = IMG_BUF_SIZE[0];
	mem_cfg.pool_info[0].blk_cnt = 2;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;

	mem_cfg.pool_info[1].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[1].blk_size = IMG_BUF_SIZE[0]/2;
	mem_cfg.pool_info[1].blk_cnt = 2;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;

	mem_cfg.pool_info[2].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[2].blk_size = IMG_BUF_SIZE[0]*40;
	mem_cfg.pool_info[2].blk_cnt = 2;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;

	mem_cfg.pool_info[3].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[3].blk_size = IMG_BUF_SIZE[0]*6;
	mem_cfg.pool_info[3].blk_cnt = 2;
	mem_cfg.pool_info[3].ddr_id = DDR_ID0;

	mem_cfg.pool_info[4].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[4].blk_size = ((IMG_BUF_SIZE[0]+15)/16)*16*12;
	mem_cfg.pool_info[4].blk_cnt = 2;
	mem_cfg.pool_info[4].ddr_id = DDR_ID0;

	mem_cfg.pool_info[5].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[5].blk_size = ((IMG_BUF_SIZE[0]+7)/8);
	mem_cfg.pool_info[5].blk_cnt = 1;
	mem_cfg.pool_info[5].ddr_id = DDR_ID0;
	
	// model 1 dram mem allocate
	mem_cfg.pool_info[6].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[6].blk_size = IMG_BUF_SIZE[1];
	mem_cfg.pool_info[6].blk_cnt = 2;
	mem_cfg.pool_info[6].ddr_id = DDR_ID0;

	mem_cfg.pool_info[7].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[7].blk_size = IMG_BUF_SIZE[1]/2;
	mem_cfg.pool_info[7].blk_cnt = 2;
	mem_cfg.pool_info[7].ddr_id = DDR_ID0;

	mem_cfg.pool_info[8].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[8].blk_size = IMG_BUF_SIZE[1]*40;
	mem_cfg.pool_info[8].blk_cnt = 2;
	mem_cfg.pool_info[8].ddr_id = DDR_ID0;

	mem_cfg.pool_info[9].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[9].blk_size = IMG_BUF_SIZE[1]*6;
	mem_cfg.pool_info[9].blk_cnt = 2;
	mem_cfg.pool_info[9].ddr_id = DDR_ID0;

	mem_cfg.pool_info[10].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[10].blk_size = ((IMG_BUF_SIZE[1]+15)/16)*16*12;
	mem_cfg.pool_info[10].blk_cnt = 2;
	mem_cfg.pool_info[10].ddr_id = DDR_ID0;

	mem_cfg.pool_info[11].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[11].blk_size = ((IMG_BUF_SIZE[1]+7)/8);
	mem_cfg.pool_info[11].blk_cnt = 1;
	mem_cfg.pool_info[11].ddr_id = DDR_ID0;

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("err:hd_common_mem_init err: %d\r\n", ret);
	}
	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	hd_common_mem_uninit();
	return ret;
}

static void bc_reorgS1(UINT8* inputS, UINT8* outputS,UINT32 width, UINT32 height)
{
	UINT32 i,j,count,size;
	count=0;
	size = width*height;
	for(j = 0; j < MDBC_ALIGN(size,8)/8; j++) {
		UINT8 c = inputS[j];
        for(i = 0; i < 8; i++) {
			if(count<size)
			{
				outputS[count] = c & 0x1;
				c = c>>1;
				count++;
			}
        }
    }
}

#if OUTPUT_BMP
static void bc_writebmpfile(char* name, UINT8* raw_img,
    int width, int height, UINT16 bits)
{
    if(!(name && raw_img)) {
        printf("Error bmpWrite.");
        return;
    }
	int i,j;//,length;
    // FileHeader
    struct BmpFileHeader file_h = {
        .bfType=0x4d42,
        .bfSize=0,
        .bfReserved1=0,
        .bfReserved2=0,
        .bfOffBits=54,
    };
    file_h.bfSize = file_h.bfOffBits + width*height * bits/8;
    if(bits==8) {file_h.bfSize += 1024, file_h.bfOffBits += 1024;}
    // BmpInfoHeader
    struct BmpInfoHeader info_h = {
        .biSize=40,
        .biWidth=0,
        .biHeight=0,
        .biPlanes=1,
        .biBitCount=0,
        .biCompression=0,
        .biSizeImage=0,
        .biXPelsPerMeter=0,
        .biYPelsPerMeter=0,
        .biClrUsed=0,
        .biClrImportant=0,
    };
    info_h.biWidth = width;
    info_h.biHeight = height;
    info_h.biBitCount = bits;
    info_h.biSizeImage = width*height * bits/8;
    if(bits == 8) {
        info_h.biClrUsed=256;
        info_h.biClrImportant=256;
    }
    // Write Header
	/*
	INT32 FileHandleStatus = 0;
	FST_FILE filehdl = NULL;
	filehdl = FileSys_OpenFile(name, FST_OPEN_WRITE | FST_CREATE_ALWAYS);
	if (!filehdl) {
		emu_msg(("^ROpen file fail - %s...\r\n", name));
	}
	length = sizeof(file_h);
	FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&file_h, &length, 0, NULL);
	length = sizeof(info_h);
	FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&info_h, &length, 0, NULL);
	*/
    FILE *pFile = fopen(name,"wb+");
    if(!pFile) {
        printf("Error opening file.");
        return;
    }
    fwrite((char*)&file_h, sizeof(char), sizeof(file_h), pFile);
    fwrite((char*)&info_h, sizeof(char), sizeof(info_h), pFile);

	//length = 1;
	// Write colormap
	//printf("Write colormap...\r\n");
    if(bits == 8) {
        for(i = 0; i < 256; ++i) {
            UINT8 c = i;
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
            fwrite((char*)&c, sizeof(char), sizeof(UINT8), pFile);
            fwrite((char*)&c, sizeof(char), sizeof(UINT8), pFile);
            fwrite((char*)&c, sizeof(char), sizeof(UINT8), pFile);
            fwrite("", sizeof(char), sizeof(UINT8), pFile);
        }
    }
    // Write raw img
	//printf("Write raw img...\r\n");
    UINT8 alig = ((width*bits/8)*3) % 4;
    for(j = height-1; j >= 0; --j) {
		//printf("j : %d\r\n",j);
        for(i = 0; i < width; ++i) {
			UINT8 c;
            if(bits == 24) {
				//c = raw_img[(j*width+i)*3 + 2];
				//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
				//c = raw_img[(j*width+i)*3 + 1];
				//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
				//c = raw_img[(j*width+i)*3 + 0];
				//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
                fwrite((char*)&raw_img[(j*width+i)*3 + 2], sizeof(char), sizeof(UINT8), pFile);
                fwrite((char*)&raw_img[(j*width+i)*3 + 1], sizeof(char), sizeof(UINT8), pFile);
                fwrite((char*)&raw_img[(j*width+i)*3 + 0], sizeof(char), sizeof(UINT8), pFile);
            } else if(bits == 8) {
				if(raw_img[j*width+i]==1){ c = 255;}
				else if(raw_img[j*width+i]==0){ c = 0;}
				else{
					printf("raw_img[j*width+i] = %d, (%d,%d)\n",raw_img[j*width+i],i,j);
					c = 0;
				}
				//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
                fwrite((char*)&c, sizeof(char), sizeof(UINT8), pFile);
            }
        }
        // 4byte align
        for(i = 0; i < alig; ++i) {
			//UINT8 c = 0;
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
            fwrite("", sizeof(char), sizeof(UINT8), pFile);
        }
    }
	//FileHandleStatus = FileSys_CloseFile(filehdl);
	//if (FileHandleStatus != FST_STA_OK) {
	//	emu_msg(("^RClose file fail - %s...\r\n", name));
	//}
    fclose(pFile);
}
#endif

static UINT32 md_load_file(CHAR *p_filename, UINT32 va)
{
	FILE  *fd;
	UINT32 file_size = 0, read_size = 0;
	const UINT32 addr = va;
	//printf("addr = %08x\r\n", (int)addr);

	fd = fopen(p_filename, "rb");
	if (!fd) {
		printf("cannot read %s\r\n", p_filename);
		return 0;
	}

	fseek ( fd, 0, SEEK_END );
	file_size = ALIGN_CEIL_4( ftell(fd) );
	fseek ( fd, 0, SEEK_SET );

	read_size = fread ((void *)addr, 1, file_size, fd);
	if (read_size != file_size) {
		printf("size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)file_size);
	}
	fclose(fd);
	return read_size;
}

static void md_set_para_model0_sensitivity(VENDOR_MD_PARAM *mdbc_parm)
{
	mdbc_parm->controlEn.update_nei_en = 1;
	mdbc_parm->controlEn.deghost_en    = 1;
	mdbc_parm->controlEn.roi_en0       = 0;
	mdbc_parm->controlEn.roi_en1       = 0;
	mdbc_parm->controlEn.roi_en2       = 0;
	mdbc_parm->controlEn.roi_en3       = 0;
	mdbc_parm->controlEn.roi_en4       = 0;
	mdbc_parm->controlEn.roi_en5       = 0;
	mdbc_parm->controlEn.roi_en6       = 0;
	mdbc_parm->controlEn.roi_en7       = 0;
	mdbc_parm->controlEn.chksum_en     = 0;
	mdbc_parm->controlEn.bgmw_save_bw_en = 0;
	
	mdbc_parm->MdmatchPara.lbsp_th    = 0;
	mdbc_parm->MdmatchPara.d_colour   = 6;
	mdbc_parm->MdmatchPara.r_colour   = 0x1e;
	mdbc_parm->MdmatchPara.d_lbsp     = 3;
	mdbc_parm->MdmatchPara.r_lbsp     = 5;
	mdbc_parm->MdmatchPara.model_num  = 0x8;
	mdbc_parm->MdmatchPara.t_alpha    = 0x33;
	mdbc_parm->MdmatchPara.dw_shift   = 0x4;
	mdbc_parm->MdmatchPara.dlast_alpha= 0x28;
	mdbc_parm->MdmatchPara.min_match  = 2;
	mdbc_parm->MdmatchPara.dlt_alpha  = 0xa;
	mdbc_parm->MdmatchPara.dst_alpha  = 0x28;
	mdbc_parm->MdmatchPara.uv_thres   = 6;
	mdbc_parm->MdmatchPara.s_alpha    = 0x28;
	mdbc_parm->MdmatchPara.dbg_lumDiff= g_LumDiff[0];	/// assign last frame reg lumDiff data
	mdbc_parm->MdmatchPara.dbg_lumDiff_en = 1;			/// must enable

	mdbc_parm->MorPara.th_ero     = 0x8;
	mdbc_parm->MorPara.th_dil     = 0x0;
	mdbc_parm->MorPara.mor_sel0   = 0x0;
	mdbc_parm->MorPara.mor_sel1   = 0x1;
	mdbc_parm->MorPara.mor_sel2   = 0x2;
	mdbc_parm->MorPara.mor_sel3   = 0x3;

	mdbc_parm->UpdPara.minT           = 0x2;
	mdbc_parm->UpdPara.maxT           = 0xff;
	mdbc_parm->UpdPara.maxFgFrm       = 0xff;
	mdbc_parm->UpdPara.deghost_dth    = 5;
	mdbc_parm->UpdPara.deghost_sth    = 250;
	mdbc_parm->UpdPara.stable_frm     = 0x78;
	mdbc_parm->UpdPara.update_dyn     = 0x80;
	mdbc_parm->UpdPara.va_distth      = 32;
	mdbc_parm->UpdPara.t_distth       = 24;
	mdbc_parm->UpdPara.dbg_frmID      = g_FrmID[0];		/// assign last frame reg frmID data
	mdbc_parm->UpdPara.dbg_frmID_en   = 1;				/// must enable
	mdbc_parm->UpdPara.dbg_rnd        = g_Rnd[0];		/// assign last frame reg rnd data
	mdbc_parm->UpdPara.dbg_rnd_en     = 1;				/// must enable
}

static void md_set_para_model1_sensitivity(VENDOR_MD_PARAM *mdbc_parm)
{
	mdbc_parm->controlEn.update_nei_en = 1;
	mdbc_parm->controlEn.deghost_en    = 1;
	mdbc_parm->controlEn.roi_en0       = 0;
	mdbc_parm->controlEn.roi_en1       = 0;
	mdbc_parm->controlEn.roi_en2       = 0;
	mdbc_parm->controlEn.roi_en3       = 0;
	mdbc_parm->controlEn.roi_en4       = 0;
	mdbc_parm->controlEn.roi_en5       = 0;
	mdbc_parm->controlEn.roi_en6       = 0;
	mdbc_parm->controlEn.roi_en7       = 0;
	mdbc_parm->controlEn.chksum_en     = 0;
	mdbc_parm->controlEn.bgmw_save_bw_en = 0;
	
	mdbc_parm->MdmatchPara.lbsp_th    = 0;
	mdbc_parm->MdmatchPara.d_colour   = 6;
	mdbc_parm->MdmatchPara.r_colour   = 0x24;
	mdbc_parm->MdmatchPara.d_lbsp     = 3;
	mdbc_parm->MdmatchPara.r_lbsp     = 6;
	mdbc_parm->MdmatchPara.model_num  = 0x8;
	mdbc_parm->MdmatchPara.t_alpha    = 0x33;
	mdbc_parm->MdmatchPara.dw_shift   = 0x4;
	mdbc_parm->MdmatchPara.dlast_alpha= 0x28;
	mdbc_parm->MdmatchPara.min_match  = 2;
	mdbc_parm->MdmatchPara.dlt_alpha  = 0xa;
	mdbc_parm->MdmatchPara.dst_alpha  = 0x28;
	mdbc_parm->MdmatchPara.uv_thres   = 6;
	mdbc_parm->MdmatchPara.s_alpha    = 0x28;
	mdbc_parm->MdmatchPara.dbg_lumDiff= g_LumDiff[1];   /// assign last frame reg lumDiff data
	mdbc_parm->MdmatchPara.dbg_lumDiff_en = 1;			/// must enable

	mdbc_parm->MorPara.th_ero     = 0x8;
	mdbc_parm->MorPara.th_dil     = 0x0;
	mdbc_parm->MorPara.mor_sel0   = 0x0;
	mdbc_parm->MorPara.mor_sel1   = 0x1;
	mdbc_parm->MorPara.mor_sel2   = 0x2;
	mdbc_parm->MorPara.mor_sel3   = 0x3;

	mdbc_parm->UpdPara.minT           = 0x8;
	mdbc_parm->UpdPara.maxT           = 0xff;
	mdbc_parm->UpdPara.maxFgFrm       = 0xff;
	mdbc_parm->UpdPara.deghost_dth    = 5;
	mdbc_parm->UpdPara.deghost_sth    = 250;
	mdbc_parm->UpdPara.stable_frm     = 0x78;
	mdbc_parm->UpdPara.update_dyn     = 0x80;
	mdbc_parm->UpdPara.va_distth      = 32;
	mdbc_parm->UpdPara.t_distth       = 24;
	mdbc_parm->UpdPara.dbg_frmID      = g_FrmID[1];		/// assign last frame reg frmID data
	mdbc_parm->UpdPara.dbg_frmID_en   = 1;				/// must enable
	mdbc_parm->UpdPara.dbg_rnd        = g_Rnd[1];		/// assign last frame reg rnd data
	mdbc_parm->UpdPara.dbg_rnd_en     = 1;				/// must enable
}

static HD_RESULT md_set_para(MEM_RANGE *p_share_mem, UINT32 ping_pong_id, UINT32 mode, UINT32 sensi,int model_id)
{
	VENDOR_MD_PARAM 		   mdbc_parm;
	HD_RESULT ret = HD_OK;

	mdbc_parm.mode = mode;

	if(ping_pong_id == 0)
	{
		mdbc_parm.InInfo.uiInAddr0 = p_share_mem[0].addr;
		mdbc_parm.InInfo.uiInAddr1 = p_share_mem[1].addr;
		mdbc_parm.InInfo.uiInAddr2 = p_share_mem[2].addr;
		mdbc_parm.InInfo.uiInAddr3 = p_share_mem[3].addr;
		mdbc_parm.InInfo.uiInAddr4 = p_share_mem[4].addr;
		mdbc_parm.InInfo.uiInAddr5 = p_share_mem[5].addr;
		mdbc_parm.OutInfo.uiOutAddr0 = p_share_mem[6].addr;
		mdbc_parm.OutInfo.uiOutAddr1 = p_share_mem[7].addr;
		mdbc_parm.OutInfo.uiOutAddr2 = p_share_mem[8].addr;
		mdbc_parm.OutInfo.uiOutAddr3 = p_share_mem[9].addr;
	} else {
		mdbc_parm.InInfo.uiInAddr0 = p_share_mem[0].addr;
		mdbc_parm.InInfo.uiInAddr1 = p_share_mem[1].addr;
		mdbc_parm.InInfo.uiInAddr2 = p_share_mem[2].addr;
		mdbc_parm.InInfo.uiInAddr3 = p_share_mem[7].addr;
		mdbc_parm.InInfo.uiInAddr4 = p_share_mem[8].addr;
		mdbc_parm.InInfo.uiInAddr5 = p_share_mem[9].addr;
		mdbc_parm.OutInfo.uiOutAddr0 = p_share_mem[6].addr;
		mdbc_parm.OutInfo.uiOutAddr1 = p_share_mem[3].addr;
		mdbc_parm.OutInfo.uiOutAddr2 = p_share_mem[4].addr;
		mdbc_parm.OutInfo.uiOutAddr3 = p_share_mem[5].addr;
	}

	mdbc_parm.uiLLAddr          = 0x0;
	mdbc_parm.InInfo.uiLofs0    = MDBC_ALIGN(IMG_WIDTH[model_id],4);//160;
	mdbc_parm.InInfo.uiLofs1    = MDBC_ALIGN(IMG_WIDTH[model_id],4);//160;
	mdbc_parm.Size.uiMdbcWidth  = IMG_WIDTH[model_id];
	mdbc_parm.Size.uiMdbcHeight = IMG_HEIGHT[model_id];

	switch (sensi) {
	case MODEL1_PARA:
		md_set_para_model1_sensitivity(&mdbc_parm);
		break;
	case MODEL0_PARA:
	default:
		md_set_para_model0_sensitivity(&mdbc_parm);
		break;
	}

	ret = vendor_md_set(VENDOR_MD_PARAM_ALL, &mdbc_parm);
	if (HD_OK != ret) {
		printf("set img fail, error code = %d\r\n", ret);
	}
	return ret;
}


MAIN(argc, argv)
{
    HD_RESULT ret;
	MEM_RANGE share_mem[MD_RUN_MODEL_NUM][11] = {0};
	int frmidx = 0,modelidx = 0,i;
	UINT32 pattern_end_id = 100;
	UINT32 ping_pong_id[MD_RUN_MODEL_NUM] = {0};
	UINT32 is_Init[MD_RUN_MODEL_NUM]= {0};
	VENDOR_MD_TRIGGER_PARAM md_trig_param;
	VENDOR_MD_REG_DATA md_reg_data;
	UINT32 file_size = 0;
	char in_file1[64];
	char in_file2[64];
	int base_id[MD_RUN_MODEL_NUM];
#if DEBUG_FILE
	char out_file1[64];
	char out_file2[64];
	char out_file3[64];
	char out_file4[64];
	FILE  *fd;
#endif
	base_id[0] = 3051; /// input video0
	base_id[1] = 4751; /// input video1
	
#if OUTPUT_BMP
	char ImgFilePath[64];
#endif

	IMG_WIDTH[0] = 160;
	IMG_HEIGHT[0] = 120;
	pattern_end_id = 10;
	IMG_WIDTH[1] = 360;
	IMG_HEIGHT[1] = 240;
	printf("IMG_WIDTH[0] = %lu,IMG_HEIGHT[0] = %lu,pattern_end_id = %lu\r\n", IMG_WIDTH[0],IMG_HEIGHT[0],pattern_end_id);
	printf("IMG_WIDTH[1] = %lu,IMG_HEIGHT[1] = %lu,pattern_end_id = %lu\r\n", IMG_WIDTH[1],IMG_HEIGHT[1],pattern_end_id);
	IMG_BUF_SIZE[0] = (IMG_WIDTH[0] * IMG_HEIGHT[0]);
	IMG_BUF_SIZE[1] = (IMG_WIDTH[1] * IMG_HEIGHT[1]);
	printf("IMG_BUF_SIZE[0] = %lu\r\n", IMG_BUF_SIZE[0]);
	printf("IMG_BUF_SIZE[1] = %lu\r\n", IMG_BUF_SIZE[1]);

	/* init common module */
	ret = hd_common_init(0);
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto comm_init_fail;
    }
	/* init memory */
	ret = mem_init();
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto mem_init_fail;
    }
	/* init share memory */
	ret = share_memory_init(share_mem);
	if (ret != HD_OK) {
		printf("mem_init fail=%d\n", ret);
		goto exit;
	}

	/* init gfx for scale*/
	ret = hd_gfx_init();
    if (HD_OK != ret) {
        printf("hd_gfx_init fail\r\n");
        goto exit;
    }

	ret = vendor_md_init();
	if (HD_OK != ret) {
		printf("init fail, error code = %d\r\n", ret);
		goto exit;
	}

	/// reg data init value, LumDiff = 0, FrmID = 0, Rnd = 1
	for(i=0;i<MD_RUN_MODEL_NUM;i++)
	{
		g_LumDiff[i] = 0;
		g_FrmID[i] = 0;
		g_Rnd[i] = 1;
	}
	md_trig_param.is_nonblock = 0;
	md_trig_param.time_out_ms = 0;
	for(frmidx = 0; frmidx <= (int)pattern_end_id; frmidx++)
	{
		printf("------------ frmidx = %d ------------\r\n", frmidx);
		
		for(modelidx = 0; modelidx < MD_RUN_MODEL_NUM; modelidx++)
		{
		/// load file, y & uv
#if defined(__FREERTOS)
		snprintf(in_file1, 64, "A:\\MDBCP\\DI\\pic_%04d\\dram_in_y.bin",frmidx+base_id[modelidx]);
#else
		snprintf(in_file1, 64, "//mnt//sd//MDBCP//DI//pic_%04d//dram_in_y.bin",frmidx+base_id[modelidx]);
#endif
		file_size = md_load_file(in_file1, share_mem[modelidx][0].va);
		if (file_size == 0) {
			printf("[ERR]load dram_in_y.bin : %s\r\n", in_file1);
		}

#if defined(__FREERTOS)
		snprintf(in_file2, 64, "A:\\MDBCP\\DI\\pic_%04d\\dram_in_uv.bin",frmidx+base_id[modelidx]);
#else
		snprintf(in_file2, 64, "//mnt//sd//MDBCP//DI//pic_%04d//dram_in_uv.bin",frmidx+base_id[modelidx]);
#endif
		file_size = md_load_file(in_file2, share_mem[modelidx][1].va);
		if (file_size == 0) {
			printf("[ERR]load dram_in_uv.bin : %s\r\n", in_file2);
		}

		/// swithch parameter set for video0/video1
		if(modelidx == 1)g_md_sensi = MODEL1_PARA;
		else g_md_sensi = MODEL0_PARA;
		
		/// set to engine
		md_set_para(share_mem[modelidx], ping_pong_id[modelidx],is_Init[modelidx],g_md_sensi,modelidx);
		/// engine fire
		ret = vendor_md_trigger(&md_trig_param);
		if (HD_OK != ret) {
			printf("trigger fail, error code = %d\r\n", ret);
			break;
		}
		memcpy((UINT32 *)share_mem[modelidx][2].va , (UINT32 *)share_mem[modelidx][1].va , IMG_BUF_SIZE[modelidx]/2);
		ret = vendor_md_get(VENDOR_MD_PARAM_GET_REG, &md_reg_data);
		if (HD_OK != ret) {
			printf("get reg data fail, error code = %d\r\n", ret);
		}
		
		/// store reg data for next frame 
		g_LumDiff[modelidx] = md_reg_data.uiLumDiff;
		g_FrmID[modelidx] = md_reg_data.uiFrmID;
		g_Rnd[modelidx] = md_reg_data.uiRnd;
		printf("g_LumDiff[%d] = 0x%08x\r\n",modelidx, g_LumDiff[modelidx]);
		printf("g_FrmID[%d] = %d\r\n",modelidx, g_FrmID[modelidx]);
		printf("g_Rnd[%d] = 0x%08x\r\n",modelidx, g_Rnd[modelidx]);

		if(is_Init[modelidx] == 1) {
			hd_common_mem_flush_cache((VOID *)share_mem[modelidx][6].va, ((IMG_BUF_SIZE[modelidx]+7)/8));
			bc_reorgS1((UINT8*)share_mem[modelidx][6].va,(UINT8*)share_mem[modelidx][10].va, IMG_WIDTH[modelidx], IMG_HEIGHT[modelidx]);
#if OUTPUT_BMP
#if defined(__FREERTOS)
		snprintf(ImgFilePath, 64, "A:\\MDBCP\\Debug\\output_bmp\\output_%04d.bmp", frmidx+base_id[modelidx]);
#else
		snprintf(ImgFilePath, 64, "//mnt//sd//MDBCP//Debug//output_bmp//output_%04d.bmp", frmidx+base_id[modelidx]);
#endif
		bc_writebmpfile(ImgFilePath, (UINT8*)share_mem[modelidx][10].va, IMG_WIDTH[modelidx], IMG_HEIGHT[modelidx], 8);
#endif

#if DEBUG_FILE
#if defined(__FREERTOS)
		snprintf(out_file1, 64, "A:\\MDBCP\\DO\\pic_%04d\\dram_out_s1.bin", base_id[modelidx]+frmidx);
#else
		snprintf(out_file1, 64, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_s1.bin", base_id[modelidx]+frmidx);
#endif
			fd = fopen(out_file1, "wb");
			if (!fd) {
				printf("cannot read %s\r\n", out_file1);
			} else {
    			file_size = fwrite((const void *)share_mem[modelidx][6].va,1,((IMG_BUF_SIZE[modelidx]+7)/8),fd);
    			if (file_size == 0) {
    				printf("load dram_out_s1.bin : %s\r\n", out_file1);
    			}
    			fclose(fd);
			}

			hd_common_mem_flush_cache((VOID *)share_mem[modelidx][7].va, IMG_BUF_SIZE[modelidx]*40);
#if defined(__FREERTOS)
		snprintf(out_file2, 64, "A:\\MDBCP\\DO\\pic_%04d\\dram_out_bgYUV.bin", base_id[modelidx]+frmidx);
#else
		snprintf(out_file2, 64, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_bgYUV.bin", base_id[modelidx]+frmidx);
#endif
			fd = fopen(out_file2, "wb");
			if (!fd) {
				printf("cannot read %s\r\n", out_file2);
			} else {
				if(ping_pong_id[modelidx] == 1) file_size = fwrite((const void *)share_mem[modelidx][3].va,1,IMG_BUF_SIZE[modelidx]*40,fd);
    			else file_size = fwrite((const void *)share_mem[modelidx][7].va,1,IMG_BUF_SIZE[modelidx]*40,fd);
    			if (file_size == 0) {
    				printf("load dram_out_bgYUV.bin : %s\r\n", out_file2);
    			}
    			fclose(fd);
			}
#endif
		}
#if DEBUG_FILE
		hd_common_mem_flush_cache((VOID *)share_mem[modelidx][7].va, IMG_BUF_SIZE[modelidx]*6);
#if defined(__FREERTOS)
		snprintf(out_file3, 64, "A:\\MDBCP\\DO\\pic_%04d\\dram_out_var1.bin", base_id[modelidx]+frmidx);
#else
		snprintf(out_file3, 64, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_var1.bin", base_id[modelidx]+frmidx);
#endif
		fd = fopen(out_file3, "wb");
		if (!fd) {
			printf("cannot read %s\r\n", out_file3);
		} else {
    		if(ping_pong_id[modelidx] == 1) file_size = fwrite((const void *)share_mem[modelidx][4].va,1,IMG_BUF_SIZE[modelidx]*6,fd);
    		else file_size = fwrite((const void *)share_mem[modelidx][8].va,1,IMG_BUF_SIZE[modelidx]*6,fd);
    		if (file_size == 0) {
    			printf("load dram_out_var1.bin : %s\r\n", out_file3);
    		}
    		fclose(fd);
		}

		hd_common_mem_flush_cache((VOID *)share_mem[modelidx][7].va, ((IMG_BUF_SIZE[modelidx]+15)/16)*16*12);
#if defined(__FREERTOS)
		snprintf(out_file4, 64, "A:\\MDBCP\\DO\\pic_%04d\\dram_out_var2.bin", base_id[modelidx]+frmidx);
#else
		snprintf(out_file4, 64, "//mnt//sd//MDBCP//DO//pic_%04d//dram_out_var2.bin", base_id[modelidx]+frmidx);
#endif
		fd = fopen(out_file4, "wb");
		if (!fd) {
			printf("cannot read %s\r\n", out_file4);
		} else {
    		if(ping_pong_id[modelidx] == 1) file_size = fwrite((const void *)share_mem[modelidx][5].va,1,((IMG_BUF_SIZE[modelidx]+15)/16)*16*12,fd);
    		else file_size = fwrite((const void *)share_mem[modelidx][9].va,1,((IMG_BUF_SIZE[modelidx]+15)/16)*16*12,fd);
    		if (file_size == 0) {
    			printf("load dram_out_var2.bin : %s\r\n", out_file4);
    		}
    		fclose(fd);
		}
#endif
		ping_pong_id[modelidx] = (ping_pong_id[modelidx]+1)%2;
		if(is_Init[modelidx]==0)is_Init[modelidx]=1;
		}
	}

	ret = vendor_md_uninit();
	if (HD_OK != ret)
		printf("uninit fail, error code = %d\r\n", ret);


exit:
	ret = hd_gfx_uninit();
	if (HD_OK != ret) {
		printf("hd_gfx_uninit fail\r\n");
	}

	ret = share_memory_exit(share_mem);
	if (ret != HD_OK) {
		printf("mem_uninit fail=%d\n", ret);
	}

	ret = mem_exit();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}

mem_init_fail:
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        printf("uninit fail=%d\n", ret);
    }
comm_init_fail:
	return 0;
}