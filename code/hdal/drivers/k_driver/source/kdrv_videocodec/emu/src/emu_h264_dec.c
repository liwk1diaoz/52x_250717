#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "h264dec_api.h"

#include "emu_h26x_common.h"

#include "h26x_common.h"
#include "emu_h264_dec.h"

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#define MAX_SLICE_HDR_LEN	(128)
#define H264_AUTO_JOB_SEL	(0)

char h264d_folder_name[][32] =
{
    //[64G-2] : long
    "pat_0121_long_ov",


    //[32G-7] : 16x + some_size
    "pat_2048x2048_0",
    "pat_256_0",
    "pat_fhd_ov",
    "pat_0107_16x_ov",
    "pat_208144_0",

    //[32G-1] : small size
    "pat_smallsize_16x_1",
    "pat_sdec_allsize_0",
    "pat_min_ov_0",






    //[64G-4] : ov
    "pat_0107_ov",






    "pat_208144_0",
    "pat_fhd_ov",
    "pat_1223_ov",
    "pat_0107_16x_ov",
    "pat_2048x2048_0",
    "pat_1227_16x_ov",
    "pat_1218_2mb_fast2",
    "pat_newsdec_3",
	"PAT_1126_0",
	"PAT_1128_1",
	"PAT_1225_2",
	"PAT_FANDI",
	"PAT_0115_0",
	"PAT_0115_1",
	"PAT_0116_0",
	"PAT_0118_0",
	"PAT_0131_0",
};


static void read_input_bs(h264d_pat_t *p_pat, H264DEC_INFO *pInfo)
{
	//DBG_DUMP("[%s][%d] uiBSAddr = 0x%08x,0x%08x\r\n", __FUNCTION__, __LINE__,(int)pInfo->uiBSAddr,(int)pInfo->uiBSSize);

	//emuh26x_msg(("read_input_bs %d.\r\n", pInfo->uiBSSize));
	h26xFileReadFlush(&p_pat->file.es, pInfo->uiBSSize, pInfo->uiBSAddr);

}

static void emu_h264d_setup_folder(unsigned int folder_idx, h264_folder_t *p_folder, unsigned int end_pat_idx)
{
	H26XFile fp_pat_cnt;
	char file_name[128];

	memset(p_folder, 0, sizeof(h264_folder_t));

	p_folder->idx = folder_idx;
	sprintf(p_folder->name, "A:\\%s", h264d_folder_name[p_folder->idx]);
	sprintf(file_name, "%s\\pat_num.dat", p_folder->name);
	h26xFileOpen(&fp_pat_cnt, file_name);
	h26xFileRead(&fp_pat_cnt, sizeof(UINT32), (UINT32)&p_folder->pat_num);
	h26xFileClose(&fp_pat_cnt);

	if (end_pat_idx)
		p_folder->pat_num = end_pat_idx;

	#if 0
	emuh26x_msg(("folder_name : %s\r\n", p_folder->name));
	emuh26x_msg(("pat_num : %d\r\n", p_folder->pat_num));
	#endif
}

static void emu_h264d_setup_pat(h264_folder_t *p_folder, unsigned int pat_idx, h264d_pat_t *p_pat, unsigned int frm_num)
{
	char file_name[128];
	memset(p_pat, 0, sizeof(h264d_pat_t));

	p_pat->idx = pat_idx;
	p_pat->rand_seed = rand()%4096;
	srand(p_pat->rand_seed);

	sprintf(p_pat->name, "%s\\E_%04d", p_folder->name, p_pat->idx);
	sprintf(file_name, "%s\\info_data.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.info, file_name);
	sprintf(file_name, "%s\\es_data.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.es, file_name);

	sprintf(file_name, "%s\\seq.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.seq, file_name);
	sprintf(file_name, "%s\\pic.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.pic, file_name);
	sprintf(file_name, "%s\\chk.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.chk, file_name);

	h26xFileRead(&p_pat->file.info, sizeof(info_t), (UINT32)&p_pat->info);

	p_pat->info.frame_num = (frm_num) ? frm_num : p_pat->info.frame_num;

	h26xFileRead(&p_pat->file.seq, sizeof(unsigned int), (UINT32)&p_pat->seq_obj_size);
	h26xFileRead(&p_pat->file.pic, sizeof(unsigned int), (UINT32)&p_pat->pic_obj_size);
	h26xFileRead(&p_pat->file.chk, sizeof(unsigned int), (UINT32)&p_pat->chk_obj_size);

	sprintf(file_name, "%s\\dump_video_header.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.video_hdr_es, file_name);
	sprintf(file_name, "%s\\video_header_len.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.video_hdr_len, file_name);


}

static void emu_h264d_init(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item)
{
	int i;
	h26x_mem_t *mem = &p_job->mem;
	h264d_ctx_t *ctx = (h264d_ctx_t *)p_job->ctx_addr;
	h264d_pat_t *pat = &ctx->pat;
	h264d_emu_t *emu = &ctx->emu;
	UINT32 rec_frm_size;
	UINT32 bs_buf_size;

	p_job->mem.start = p_job->mem_bak.start;
	p_job->mem.addr  = p_job->mem_bak.addr;
	p_job->mem.size  = p_job->mem_bak.size;

	memset(emu, 0, sizeof(h264d_emu_t));

	h26xFileRead(&pat->file.seq, pat->seq_obj_size, (UINT32)&pat->seq);

	emu->init_obj.uiWidth = pat->seq.width;
	emu->init_obj.uiHeight = pat->seq.height;

	//rec_frm_size = SIZE_64X(emu->init_obj.uiWidth) * emu->init_obj.uiHeight;
	rec_frm_size = SIZE_64X(emu->init_obj.uiWidth) * SIZE_16X(emu->init_obj.uiHeight);


	bs_buf_size = pat->seq.width * pat->seq.height * 3;

	for (i = 0; i < dec_frm_buf_num; i++) {
		pat->rec_y_addr[i] = h26x_mem_malloc(mem, rec_frm_size);
		pat->rec_uv_addr[i] = h26x_mem_malloc(mem, rec_frm_size/2);
	}

	emu->info_obj.uiBSAddr = h26x_mem_malloc(mem, bs_buf_size);
    //DBG_DUMP("[%s][%d] uiBSAddr = 0x%08x - 0x%08x, size = 0x%08x\r\n", __FUNCTION__, __LINE__,(int)emu->info_obj.uiBSAddr,
    //    (int)((unsigned int)emu->info_obj.uiBSAddr+bs_buf_size),(int)bs_buf_size);

	emu->init_obj.uiYLineOffset = SIZE_64X(emu->init_obj.uiWidth) / 4;
	emu->init_obj.uiUVLineOffset = SIZE_64X(emu->init_obj.uiWidth) / 4;
	emu->init_obj.uiDecBufAddr = mem->addr;
	emu->init_obj.uiDecBufSize = mem->size;
	emu->init_obj.uiH264DescAddr = emu->info_obj.uiBSAddr;
	emu->init_obj.uiDisplayWidth = emu->init_obj.uiWidth;
	emu->init_obj.uiAPBAddr = p_job->apb_addr;

	emu->var_obj.eCodecType = VCODEC_H264;

	// first bs_len  : sps + pps //
	h26xFileRead(&pat->file.video_hdr_len, sizeof(unsigned int)*1,(UINT32)&emu->init_obj.uiH264DescSize);
	h26xFileReadFlush(&pat->file.video_hdr_es, emu->init_obj.uiH264DescSize, emu->init_obj.uiH264DescAddr);
	//emuh26x_msg(("[%s][%d] %d, %08x, %08x\r\n",__FUNCTION__,__LINE__, emu->init_obj.uiH264DescSize, emu->init_obj.uiH264DescAddr, emu->init_obj.uiDecBufAddr));
	if (h264Dec_initDecoder(&emu->init_obj, &emu->var_obj) != H26XENC_SUCCESS)
	{
        DBG_ERR("[ERR][%s][%d]\r\n", __FUNCTION__, __LINE__);
    }


	mem->addr += SIZE_32X(emu->var_obj.uiCtxSize);
	mem->size -= SIZE_32X(emu->var_obj.uiCtxSize);


	if(p_ver_item->rnd_bs_buf){
		pat->bsdma_buf_size = (1 + H26X_MAX_BSDMA_NUM*2)*4;
		pat->bsdma_buf_addr = h26x_mem_malloc(mem, pat->bsdma_buf_size);
	}

	if (p_ver_item->rnd_sw_rest)
	{
		UINT32 uiFrmSize      = emu->init_obj.uiYLineOffset*4 * SIZE_16X(emu->init_obj.uiHeight);

		p_job->tmp_rec_y_addr = h26x_getPhyAddr(h26x_mem_malloc(mem, uiFrmSize));
		p_job->tmp_rec_c_addr = h26x_getPhyAddr(h26x_mem_malloc(mem, uiFrmSize/2));
		p_job->tmp_colmv_addr =  p_job->tmp_rec_y_addr;
		p_job->tmp_sideinfo_addr =  p_job->tmp_rec_y_addr;
	}

	#if 1
	// dump pattern message //
	DBG_DUMP("\r\njob(%d)(%d) pattern_name[%d][%d] : %s\r\n", (int)p_job->idx1, (int)p_job->idx2, (int)ctx->folder.idx, (int)ctx->pat.idx, ctx->pat.name);
	DBG_DUMP("seq(%d) : (w:%d, h:%d, mem : %08x %08x ( %08x), rand_seed:%d)\r\n", (int)pat->seq_obj_size, (int)pat->seq.width, (int)pat->seq.height, (int)mem->start, (int)mem->addr,(int)(mem->addr - mem->start), pat->rand_seed);
	#endif

	if (p_ver_item->write_prot)

	{
		h26x_disable_wp();
		/*
		gH26XProtectSet = 2;
		gH26XProtectAttr.uiStartingAddr = p_job->mem_bak.start;
		gH26XProtectAttr.uiSize 	  = p_job->mem_bak.size;
		gH26XProtectAttr.uiSize 	 &= 0xFFFFFFF0; // Two Word-alignment
		emu_msg(("wp addr 0x%08x, size 0x%x\r\n", gH26XProtectAttr.uiStartingAddr, gH26XProtectAttr.uiSize));
		dma_configWPFunc(gH26XProtectSet, &gH26XProtectAttr, NULL);
		dma_enableWPFunc(gH26XProtectSet);
	*/
	}




}

static void emu_h264d_close_one_pat(h264d_pat_t *p_pat)
{
	h26xFileClose(&p_pat->file.info);
	h26xFileClose(&p_pat->file.es);
	h26xFileClose(&p_pat->file.seq);
	h26xFileClose(&p_pat->file.pic);
	h26xFileClose(&p_pat->file.chk);
	h26xFileClose(&p_pat->file.video_hdr_len);
	h26xFileClose(&p_pat->file.video_hdr_es);
}

static BOOL emu_h264d_setup_one_job(h26x_ctrl_t *p_ctrl, unsigned int start_folder_idx, unsigned int end_folder_idx,unsigned int start_pat_idx, unsigned int end_pat_idx, unsigned int end_frm_num)
{
	h26x_job_t *job = h26x_job_add(3, p_ctrl);

	if (job == NULL)
		return FALSE;

	job->ctx_addr = h26x_mem_malloc(&job->mem, sizeof(h264d_ctx_t));

	job->mem.size -= (job->mem.addr - job->mem.start);
	job->mem.start = job->mem.addr;

	job->mem_bak.start = job->mem.start;
	job->mem_bak.addr  = job->mem.addr;
	job->mem_bak.size  = job->mem.size;

	h264d_ctx_t *ctx = (h264d_ctx_t *)job->ctx_addr;
	memset(ctx, 0, sizeof(h264d_ctx_t));

	job->start_folder_idx = start_folder_idx;
	job->end_folder_idx = end_folder_idx;
	job->start_pat_idx = start_pat_idx;
	job->end_pat_idx = end_pat_idx;
	job->end_frm_num = end_frm_num;

	DBG_INFO("dec_frm_buf_num   : %d \r\n", dec_frm_buf_num);
	emu_h264d_setup_folder(job->start_folder_idx, &ctx->folder, job->end_pat_idx);
	emu_h264d_setup_pat(&ctx->folder, job->start_pat_idx, &ctx->pat, job->end_frm_num);
	emu_h264d_init(job, &p_ctrl->ver_item);

	return TRUE;
}
/*
static void emu_h264d_select_new_pat_folder(h26x_job_t *job, h26x_ver_item_t *p_ver_item, unsigned int start_folder_idx, unsigned int end_folder_idx)
{
	unsigned int job_idx = rand()%(end_folder_idx - start_folder_idx) + start_folder_idx;

	job->start_folder_idx = job_idx;
	job->end_folder_idx = job_idx + 1;

	h264d_ctx_t *ctx = (h264d_ctx_t *)job->ctx_addr;
	memset(ctx, 0, sizeof(h264d_ctx_t));

	emu_h264d_setup_folder(job->start_folder_idx, &ctx->folder, job->end_pat_idx);
	emu_h264d_setup_pat(&ctx->folder, job->start_pat_idx, &ctx->pat, job->end_frm_num);
	emu_h264d_init(job, p_ver_item);
}
*/
extern int pp_see_pat;
BOOL emu_h264d_setup(h26x_ctrl_t *p_ctrl)
{
	BOOL ret = TRUE;

    ret &= emu_h264d_setup_one_job(p_ctrl, 0, 1, 0, 0, 0);      // test pattern //
    //ret &= emu_h264d_setup_one_job(p_ctrl, 0, 1, 0, 0, 0);      // test pattern //
    //ret &= emu_h264d_setup_one_job(p_ctrl, 0, 1, 205, 0, 0);      // test pattern //

    //ret &= emu_h264d_setup_one_job(p_ctrl, 0, 1, 0, 0, 0);      // test pattern //
    //ret &= emu_h264d_setup_one_job(p_ctrl, 0, 3, 0, 0, 0);      // [32G-1] : small size

    //ret &= emu_h264d_setup_one_job(p_ctrl, 0, 1, 0, 5, 0);      // [32G-7] : 16x + some_size

    //ret &= emu_h264d_setup_one_job(p_ctrl, 1, 2, 0, 0, 0);      // test pattern //
    //ret &= emu_h264d_setup_one_job(p_ctrl, 0, 2, 0, 0, 0);      // test pattern //
    //ret &= emu_h264d_setup_one_job(p_ctrl, 0, 1, 0, 0, 0);      // test pattern //
	//ret &= emu_h264d_setup_one_job(p_ctrl, 1, 2, 5, 0, 0);		// test pattern //
	//ret &= emu_h264d_setup_one_job(p_ctrl, 2, 3, 0, 0, 0);		// test pattern //
	//ret &= emu_h264d_setup_one_job(p_ctrl, 2, 3, 23, 0, 0);		// test pattern //
	//ret &= emu_h264d_setup_one_job(p_ctrl, 0, 1, 0, 0, 0);		// test pattern //
	//ret &= emu_h264d_setup_one_job(p_ctrl, 0, 1, 1, 2, 1);		// test pattern //

	return ret;
}

static void report_perf(h26x_job_t *p_job)
{
	h264d_ctx_t *ctx = (h264d_ctx_t *)p_job->ctx_addr;
	h264d_pat_t *pat = &ctx->pat;

	unsigned int mb_num = (pat->seq.width/16) * (pat->seq.height + 15)/16;
	unsigned int frm_avg_clk = pat->perf.cycle_sum/pat->pic_num;
	UINT32 bslen_avg = pat->perf.bslen_sum/pat->pic_num;

	DBG_DUMP("\r\ncycle report (%s, qp(%d)) => max(frm:%d) : c(%d, %.2f) b(%d, %.2f)(%.2f) avg : c(%ld, %.2f) b(%ld, %.2f)(%.2f) \r\n",
		pat->info.yuv_name, pat->pic.qp, pat->perf.cycle_max_frm,
		pat->perf.cycle_max, (float)pat->perf.cycle_max/mb_num,
		pat->perf.cycle_max_bslen, (float)(pat->perf.cycle_max_bslen<<3)/mb_num, (float)(pat->perf.cycle_max_bslen<<3)/pat->perf.cycle_max,
		(long int)frm_avg_clk, (float)frm_avg_clk/mb_num,
		bslen_avg, (float)(bslen_avg*8)/mb_num, (float)(bslen_avg*8)/frm_avg_clk);
}
static void update_dec_info_obj(h264d_pat_t *p_pat, H264DEC_INFO *p_info_obj, UINT32 rec_y_addr, UINT32 rec_uv_addr)
{
	p_info_obj->uiSrcYAddr = rec_y_addr;
	p_info_obj->uiSrcUVAddr = rec_uv_addr;

	// set BS size & BS buf size
	UINT32 bs_buf_size = (p_pat->seq.width * p_pat->seq.height * 3) ;
	p_info_obj->uiBSSize = p_pat->chk.result[FPGA_BS_LEN]; // picture bs size //

	//DBG_DUMP("[%s][%d] result[FPGA_BS_LEN] = 0x%08x\r\n", __FUNCTION__, __LINE__,(int)p_info_obj->uiBSSize);
	if (p_info_obj->uiBSSize > bs_buf_size) {
		DBG_ERR("h264 sim decode bs buffer(%x) is not enough, need more than(%x).\r\n", (int)bs_buf_size, (int)p_info_obj->uiBSSize);
	}

}

UINT32  uiH264TestBSCmdBuf[12][3] = {
	//  BSCmmdNums  BSBufLength  BSBufOffset   //
	//{      0x00001,     0x01000,        0x0},
	{      0x00001,     0x01000,        0x0},
	{      0x00005,     0x01110,        0x1},
	{      0x0000A,     0x01055,        0x0},
	{      0x0000F,     0x02AAA,        0x3},
	{      0x00010,     0x03333,        0x4},
	{      0x00044,     0x05075,        0x5},
	{      0x00100,     0x060DA,        0x6},
	{      0x00120,     0x07111,        0x7},
	{      0x00006,     0x0803A,        0x8},
	{      0x00005,     0x55555,        0x9},
	{      0x00033,     0x7AAAA,        0xa},
	{      0x00002,     0x80000,        0xb},
};

static BOOL h264EmuEncTestBsDma(H26XFile *pH26XFile, unsigned int *uiPicBsLen, unsigned int uiPicNum, unsigned int uiVirBSDMAAddr, unsigned int uiVirBsAddr, unsigned int uiMaxSize)
{
	UINT32 uiBSCmmdNums;
	UINT32 uiBSBufLength;
	UINT32 uiBSBufOffset;
	UINT32 uiBsCmdBufNum;
	UINT32 i;
	UINT32 uiTotalSize = 0;
	UINT32 uiPhyBsAddr = dma_getPhyAddr(uiVirBsAddr);
	UINT32 uiOriVirBsAddr = uiVirBsAddr;
    UINT32 uiFlushSize = 0;
    UINT32 uiOriuiBSDMAAddr = uiVirBSDMAAddr;

	uiBSCmmdNums  = uiH264TestBSCmdBuf[uiPicNum % 12][0];
	uiBSBufLength = uiH264TestBSCmdBuf[uiPicNum % 12][1];
	uiBSBufOffset = uiH264TestBSCmdBuf[uiPicNum % 12][2];

	//emuh26x_msg(("uiBSCmmdNums = 0x%08x uiBSBufLength = 0x%08x uiBSBufOffset = 0x%08x\r\n",uiBSCmmdNums,uiBSBufLength,uiBSBufOffset));

	uiBsCmdBufNum = uiBSCmmdNums;

	// last cmd buffer //
	if (*uiPicBsLen == 0) {
		DBG_ERR("Error at h264EmuEncTestBsDma (bs empty)\r\n");
		while (1);
	}
	//emuh26x_msg(("(%x %x, %x)\r\n",uiPhyBsAddr,uiBSBufLength, *uiPicBsLen));
	*(UINT32 *)uiVirBSDMAAddr = uiBsCmdBufNum;
	uiVirBSDMAAddr = uiVirBSDMAAddr + 4;
	for (i = 0; i < uiBsCmdBufNum; i++) {
		// read bs //
		if (uiBSBufLength < *uiPicBsLen) {
			h26xFileRead(pH26XFile, uiBSBufLength, uiVirBsAddr);
			*uiPicBsLen = *uiPicBsLen - uiBSBufLength;
		} else {
			h26xFileRead(pH26XFile, *uiPicBsLen, uiVirBsAddr);
			*uiPicBsLen = 0;
		}

		uiVirBsAddr = uiVirBsAddr + (uiBSBufLength + uiBSBufOffset);

		// set bsdma //
		*(UINT32 *)uiVirBSDMAAddr = uiPhyBsAddr;
		uiVirBSDMAAddr = uiVirBSDMAAddr + 4;
		*(UINT32 *)uiVirBSDMAAddr = uiBSBufLength;
		uiVirBSDMAAddr = uiVirBSDMAAddr + 4;

		uiPhyBsAddr = uiPhyBsAddr + uiBSBufLength + uiBSBufOffset;
		uiTotalSize += uiBSBufLength + uiBSBufOffset;

		if (*uiPicBsLen == 0) {
			break;
		}
	}
    uiFlushSize = uiVirBsAddr- uiOriVirBsAddr;
	h26x_flushCache(uiOriVirBsAddr, SIZE_32X(uiFlushSize));
	h26x_flushCache(uiOriuiBSDMAAddr, SIZE_32X((uiBsCmdBufNum*2 + 1)*4));
    //DBG_INFO("[%s][%d]bs (0x%08x - 0x%08x = 0x%08x)\r\n", __FUNCTION__, __LINE__,(int)uiOriVirBsAddr,(int)uiVirBsAddr,(int)uiFlushSize);
    //DBG_INFO("[%s][%d]bscmd (0x%08x , %d, 0x%08x)\r\n", __FUNCTION__, __LINE__,(int)uiOriuiBSDMAAddr,(int)uiBsCmdBufNum,(int)(uiBsCmdBufNum*2 + 1)*4);

	if(uiBsCmdBufNum > H26X_MAX_BSDMA_NUM){
		DBG_ERR("Error at h264EmuEncTestBsDma (0x%x, 0x%x)\r\n", (int)uiBsCmdBufNum, (int)H26X_MAX_BSDMA_NUM);
		return FALSE;
	}

	return TRUE;
}

static void emu_h264d_set_bsbuf(h26x_job_t *p_job)
{

	h264d_ctx_t *ctx = (h264d_ctx_t *)p_job->ctx_addr;
	h264d_pat_t *pat = &ctx->pat;
	h264d_emu_t *emu = &ctx->emu;
	H264DEC_INFO *p_info_obj = &emu->info_obj;
	BOOL ret;

	// initial  random bsdma cfg
	pat->uiHwBsAddr = p_info_obj->uiBSAddr;
	pat->uiDecResPicBsLen = p_info_obj->uiBSSize;

	// Set BSDMA Buffer //
	// note that the consume bslen need > slice header, or dec slice will error
	ret = h264EmuEncTestBsDma(&pat->file.es, &pat->uiDecResPicBsLen, pat->pic_num, pat->bsdma_buf_addr, pat->uiHwBsAddr, pat->bsdma_buf_size);
	if(ret == FALSE){
		while(1);
	}
}

static void emu_h264d_fpga_pic_hack(h26x_job_t *p_job, unsigned int rnd_bs_buf)
{
    H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
	h264d_ctx_t *ctx = (h264d_ctx_t *)p_job->ctx_addr;
	h264d_pat_t *pat = &ctx->pat;

	if(rnd_bs_buf){
		save_to_reg(&pRegSet->BSDMA_CMD_BUF_ADDR, h26x_getPhyAddr(pat->bsdma_buf_addr), 0, 32);	//hack
	}

    //TILE MODE : need clear due to hw linklist read data overwrite those data
    save_to_reg(&pRegSet->TILE_CFG[0], 0, 0, 32);
    save_to_reg(&pRegSet->TILE_CFG[1], 0, 0, 32);
    save_to_reg(&pRegSet->TILE_CFG[2], 0, 0, 32);
    save_to_reg(&pRegSet->TILE_CFG[3], 0, 0, 32);

	//frame time out count, unit: 256T
	save_to_reg(&pRegSet->TIMEOUT_CNT_MAX,  100 * 270 * (pat->seq.width/16) * (pat->seq.height/16)  /256 , 0, 32);

}

void emu_h264d_set_nxt_bsbuf(h26x_job_t *p_job){
	h264d_ctx_t *ctx = (h264d_ctx_t *)p_job->ctx_addr;
	h264d_pat_t *pat = &ctx->pat;
	BOOL ret;

	//emuh26x_msg(("emu_h264d_set_nxt_bsbuf\r\n" ));

	// Set BSDMA Buffer //
	ret = h264EmuEncTestBsDma(&pat->file.es, &pat->uiDecResPicBsLen, pat->pic_num, pat->bsdma_buf_addr, pat->uiHwBsAddr, pat->bsdma_buf_size);
	if(ret == FALSE){
		while(1);
	}

	h26x_setNextBsDmaBuf(h26x_getPhyAddr(pat->bsdma_buf_addr));

}


int emu_h264d_prepare_one_pic(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item)
{
	h264d_ctx_t *ctx = (h264d_ctx_t *)p_job->ctx_addr;
	h264d_pat_t *pat = &ctx->pat;
	h264d_emu_t *emu = &ctx->emu;

	if (pat->pic_num == pat->info.frame_num)
	{
		report_perf(p_job);

		if (p_job->is_finish == 1)
			return 1;
		else{
			//h264Dec_stopDecoder(&emu->var_obj);
			emu_h264d_close_one_pat(pat);
		}

		// goto next pattern //
		if ((pat->idx + 1) < ctx->folder.pat_num)
		{
			emu_h264d_setup_pat(&ctx->folder, pat->idx + 1, &ctx->pat, p_job->end_frm_num);
			emu_h264d_init(p_job, p_ver_item);
		}
		else
		{
			if ((ctx->folder.idx + 1) < p_job->end_folder_idx)
			{
				emu_h264d_setup_folder(ctx->folder.idx + 1, &ctx->folder, p_job->end_pat_idx);
				emu_h264d_setup_pat(&ctx->folder, 0, &ctx->pat, p_job->end_frm_num);
				emu_h264d_init(p_job, p_ver_item);
			}
			else
			{
				// all pattern finish //
				#if H264_AUTO_JOB_SEL
				emu_h264d_select_new_pat_folder(p_job, p_ver_item, 8, 9);
				//emu_h264_select_new_pat_folder(p_job, p_ver_item, 4, 5);
				#else
				p_job->is_finish = 1;
				return 1;
				#endif
			}
		}
	}

	h26xFileRead(&pat->file.chk, pat->chk_obj_size, (UINT32)&pat->chk);
	h26xFileRead(&pat->file.pic, pat->pic_obj_size, (UINT32)&pat->pic);

	update_dec_info_obj(pat, &emu->info_obj, pat->rec_y_addr[pat->pic_num % dec_frm_buf_num], pat->rec_uv_addr[pat->pic_num % dec_frm_buf_num]);


	//need set bs size first
	if(p_ver_item->rnd_bs_buf == 0){
		read_input_bs(pat, &emu->info_obj);
    }else{
		emu_h264d_set_bsbuf(p_job);
	}

	if (h264Dec_decodeOnePicture(&emu->info_obj, &emu->var_obj) != H264DEC_SUCCESS) {
		return H264DEC_FAIL;
	}
	if (p_ver_item->write_prot)
	{
		#if 0
		UINT32 tmp;
		tmp = rand() % 30;
		if(!tmp){
			UINT32 test_addr = 0;
			UINT32 test_size = OS_GetMemAddr(MEM_HEAP);
			UINT32 test_chn =  rand() % WP_CHN_NUM;
			h26x_test_wp(test_chn,test_addr,test_size);
		}
		tmp = rand() % 30;
		if(!tmp){
			UINT32 test_addr = emu->info_obj.uiBSAddr;
			UINT32 test_size = emu->info_obj.uiBSSize;
			UINT32 test_chn =  rand() % WP_CHN_NUM;
			h26x_test_wp(test_chn,test_addr,test_size);
		}
		#endif
        #if 0
        UINT32 test_addr = emu->info_obj.uiBSAddr;
        UINT32 test_size = emu->info_obj.uiBSSize;
        h26x_test_wp_2(pp_see_pat/4 , test_addr, test_size, pp_see_pat%4);
        #endif
        #if 0
        UINT32 test_addr = pat->rec_y_addr[pat->pic_num % dec_frm_buf_num];
        UINT32 test_size = SIZE_64X(emu->init_obj.uiWidth) * SIZE_16X(emu->init_obj.uiHeight);
        h26x_test_wp_2(pp_see_pat/4 , test_addr, test_size, pp_see_pat%4);
		memset((void *)test_addr, 0, test_size);
		h26x_flushCache(test_addr, SIZE_32X(test_size));
        #endif
	}

	emu_h264d_fpga_pic_hack(p_job, p_ver_item->rnd_bs_buf); //hack random bsdma


#if 1
	//let file point goto next picture
	if (pat->pic.mask_en) h26xFileSeek(&pat->file.pic, pat->seq.obj_size[FF_MASK], H26XF_SET_CUR);
	if (pat->pic.roi_en)  h26xFileSeek(&pat->file.pic, pat->seq.obj_size[FF_ROI], H26XF_SET_CUR);
	if (pat->pic.rrc_en)  h26xFileSeek(&pat->file.pic, pat->seq.obj_size[FF_RRC_PIC], H26XF_SET_CUR);
	if (pat->pic.rnd_en)  h26xFileSeek(&pat->file.pic, pat->seq.obj_size[FF_RND], H26XF_SET_CUR);
	if (pat->pic.mbqp_en) {
		UINT32 total_mbs = ((pat->seq.width +15)/16) * ((pat->seq.height +15)/16);
		if((pat->seq.width+31)/32*32 == pat->seq.width)
			h26xFileSeek(&pat->file.mbqp, sizeof(UINT16)*total_mbs, H26XF_SET_CUR);
		else
		{
			unsigned int h;
			for (h = 0; h < ((pat->seq.height +15)/16); h++)
				h26xFileSeek(&pat->file.mbqp, sizeof(UINT16)*((pat->seq.width +15)/16), H26XF_SET_CUR);
		}

	}
	if (pat->pic.tmnr_en) h26xFileSeek(&pat->file.pic, pat->seq.obj_size[FF_TMNR], H26XF_SET_CUR);

	if (pat->pic.osg_en && pat->pic_num == 0) h26xFileSeek(&pat->file.pic, pat->seq.obj_size[FF_OSG], H26XF_SET_CUR);
	if (pat->pic.maq_en)  h26xFileSeek(&pat->file.pic, pat->seq.obj_size[FF_MAQ], H26XF_SET_CUR);
    h26xFileSeek(&pat->file.pic, pat->seq.obj_size[FF_BGR], H26XF_SET_CUR);
    h26xFileSeek(&pat->file.pic, pat->seq.obj_size[FF_RMD], H26XF_SET_CUR);
	if (pat->pic.tnr_en)  h26xFileSeek(&pat->file.pic, pat->seq.obj_size[FF_TNR], H26XF_SET_CUR);
    if (pat->pic.lambda_en) h26xFileSeek(&pat->file.pic, pat->seq.obj_size[FF_LAMBDA], H26XF_SET_CUR);
#endif

	if (pat->pic_num == 0)
	{
		// dump pattern message //
		DBG_INFO("\r\njob(%d) pattern_name[%d][%d] : %s\r\n", p_job->idx1, ctx->folder.idx, ctx->pat.idx, ctx->pat.name);
		DBG_INFO("seq(%d) : (w:%d, h:%d, loft:%d, rotate:%d, src_iv:%d, rand_seed:%d, mem : (0x%08x ~ 0x%08x : 0x%08x))\r\n",
					(int)pat->seq_obj_size, (int)pat->seq.width, (int)pat->seq.height, (int)(emu->init_obj.uiYLineOffset*4), 0, 0,
					(int)pat->rand_seed, (int)p_job->mem.start, (int)p_job->mem.addr, (int)(p_job->mem.addr - p_job->mem.start));
	}
	//DBG_DUMP("[%s][%d]\r\n", __FUNCTION__, __LINE__);
    h26xFileDummyRead();

	return 0;
}
void dump_dec_rec(h26x_job_t *p_job)
{
#if 1
	H26XFile fp;
	h264d_ctx_t *ctx = (h264d_ctx_t *)p_job->ctx_addr;
	h264d_pat_t *pat = &ctx->pat;

	//h264_emu_t *emu = &ctx->emu;
	H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
    static int cc = 0;
	unsigned int size = (pRegSet->REC_LINE_OFFSET&0xffff) * 4 * SIZE_16X(pat->seq.height);
	char file_name[256];

	sprintf(file_name, "A:\\rec_%d.pak", cc);
    DBG_DUMP("file_name = %s\r\n", file_name);

	h26xFileOpen(&fp, file_name);
	h26xFileWrite(&fp, size, dma_getNonCacheAddr(pRegSet->REC_Y_ADDR));
	size = ((pRegSet->REC_LINE_OFFSET>>16)&0xffff) * 4 * SIZE_16X(pat->seq.height)/2;
	h26xFileWrite(&fp, size, dma_getNonCacheAddr(pRegSet->REC_C_ADDR));
	h26xFileClose(&fp);
    cc++;
#endif
}

static void chk_one_pic_dec(h26x_job_t *p_job, UINT32 interrupt, unsigned int rec_out_en, unsigned int  rnd_bs_buf)
{
	h264d_ctx_t *ctx = (h264d_ctx_t *)p_job->ctx_addr;
	h264d_pat_t *pat = &ctx->pat;
	seq_t *seq = &pat->seq;
	chk_t *chk = &pat->chk;
	//H264DEC_INFO *info_obj = &ctx->emu.info_obj;
	BOOL result = TRUE;

	#if DIRECT_MODE
	UINT32 report[H26X_REPORT_NUMBER];
	h26x_getEncReport(report);
	#else
    //UINT32 *report = (UINT32 *)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
    //UINT32 *report = (UINT32 *)dma_getNonCacheAddr(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET - offset_1);
    //UINT32 *report = (UINT32 *)dma_getCacheAddr(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
    //UINT32 *report = (UINT32 *)dma_getNonCacheAddr(p_job->check1_addr);
    UINT32 *report = (UINT32 *)dma_getCacheAddr(p_job->check1_addr);
	#endif

#if (DIRECT_MODE == 0)

    //h26x_flushCache(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET, H26X_REPORT_NUMBER*4);
    //h26x_flushCacheRead(dma_getCacheAddr(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET), H26X_REPORT_NUMBER*4);
    h26x_flushCacheRead((UINT32)report, SIZE_32X(H26X_REPORT_NUMBER*4));
#endif

    {
        ///*
        H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
        h264d_ctx_t *ctx = (h264d_ctx_t *)p_job->ctx_addr;
        h264d_emu_t *emu = &ctx->emu;
        int rec_frm_size = SIZE_64X(emu->init_obj.uiWidth) * SIZE_16X(emu->init_obj.uiHeight);
        int col_mv_size   = ((emu->init_obj.uiWidth + 63)/64) * ((emu->init_obj.uiHeight + 15)/16) * 64;

        h26x_flushCacheRead((UINT32)dma_getCacheAddr(pRegSet->REC_Y_ADDR), rec_frm_size);
        h26x_flushCacheRead((UINT32)dma_getCacheAddr(pRegSet->REC_C_ADDR), rec_frm_size/2);
        h26x_flushCacheRead((UINT32)dma_getCacheAddr(pRegSet->COL_MVS_RD_ADDR), col_mv_size);
        //*/
    }
    DBG_ERR("check1_addr : 0x%08x 0x%08x\r\n", (int)dma_getNonCacheAddr(p_job->check1_addr),SIZE_32X(H26X_REPORT_NUMBER*4));

#if 0
	if(rec_out_en)
		result &= (report[H26X_REC_CHKSUM] == chk->result[FPGA_REC_CHKSUM]);
	else{
		result &= (report[H26X_REC_CHKSUM] == 0);
		result &= (report[H26X_SIDE_INFO_CHKSUM] == 0);
	}
#else

	if(IS_ISLICE(pat->pic.slice_type)){//hack for frame skip mode
		pat->islice_recsum = chk->result[FPGA_REC_CHKSUM];
	}

	if (rec_out_en && pat->pic.skipfrm_en == 1)//hack for frame skip mode
	{
		result &= (report[H26X_REC_CHKSUM] == pat->islice_recsum);
	}
	else if(rec_out_en)
		result &= (report[H26X_REC_CHKSUM] == chk->result[FPGA_REC_CHKSUM]);
	else{
		result &= (report[H26X_REC_CHKSUM] == 0);
		result &= (report[H26X_SIDE_INFO_CHKSUM] == 0);
	}


#endif


	result &= (report[H26X_BS_LEN] == chk->result[FPGA_BS_LEN]);

	if (result == FALSE)
	{
		char cmd[0x1000];
		unsigned int cmd_addr = 0;
		memset(cmd, 0, 0x1000);

		DBG_ERR("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] decode error (int:%08x)!\r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num, (int)interrupt);
        DBG_DUMP("H26X_BS_LEN : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_BS_LEN], (int)report[H26X_BS_LEN], (int)(chk->result[FPGA_BS_LEN] - report[H26X_BS_LEN]));

        if (rec_out_en && pat->pic.skipfrm_en == 1)//hack for frame skip mode
        {
    		DBG_DUMP("islice_recsum : sw(%08x), hw(%08x) diff(%d)\r\n", (int)pat->islice_recsum, (int)report[H26X_REC_CHKSUM], (int)(pat->islice_recsum - report[H26X_REC_CHKSUM]));
        }else if(rec_out_en){
            DBG_DUMP("rec   : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[seq->fbc_en], (int)report[H26X_REC_CHKSUM], (int)(chk->result[seq->fbc_en] - report[H26X_REC_CHKSUM]));
        }else if(rec_out_en == 0)
        {
            DBG_DUMP("rec   : sw(%08x), hw(%08x) diff(%d)\r\n", (int)0, (int)report[H26X_REC_CHKSUM], (int)(0 - report[H26X_REC_CHKSUM]));
    		DBG_DUMP("sideinfo : sw(%08x), hw(%08x) diff(%d)\r\n", (int)0, (int)report[H26X_SIDE_INFO_CHKSUM], (int)(0 - report[H26X_SIDE_INFO_CHKSUM]));
        }

        //dump_dec_rec(p_job);
		h26x_prtReg();
		h26x_getDebug();
		if (DIRECT_MODE == 0)
		{
			DBG_ERR("job(%d) apb_addr + check\r\n", p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
			h26x_prtMem(dma_getCacheAddr(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET), H26X_REPORT_NUMBER*4);
			DBG_ERR("job(%d) apb data\r\n", p_job->idx1);
			h26x_prtMem(p_job->apb_addr, h26x_getHwRegSize());
			DBG_ERR("job(%d) linklist data\r\n", p_job->idx1);
			h26x_prtMem(p_job->llc_addr, h26x_getHwRegSize());

		}

		DBG_ERR("\r\ncmd : \r\n");
		h26xFileRead(&pat->file.info, pat->info.cmd_len, (UINT32)cmd);
		while(cmd_addr < pat->info.cmd_len)
		{
			DBG_ERR("%s", cmd + cmd_addr);
			cmd_addr += 511;
		}
		DBG_ERR("\r\n");

		pat->pic_num = pat->info.frame_num;	// stop this pattern //
		//while(1);
	}
	else
	{
		//h26x_prtReg();
		//h26x_getDebug();
		if (rec_out_en)
		{
			DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] decode correct!\r\n", p_job->idx1, ctx->folder.idx, pat->idx, pat->pic_num);
            #if 0
            h26x_prtReg();
            h26x_getDebug();
            if (DIRECT_MODE == 0)
            {
                DBG_ERR("job(%d) apb_addr + check\r\n", p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
                h26x_prtMem(dma_getNonCacheAddr(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET), H26X_REPORT_NUMBER*4);
                DBG_ERR("job(%d) apb data\r\n", p_job->idx1);
                h26x_prtMem(p_job->apb_addr, h26x_getHwRegSize());
                DBG_ERR("job(%d) linklist data\r\n", p_job->idx1);
                h26x_prtMem(p_job->llc_addr, h26x_getHwRegSize());
            }
            #endif
			pat->pic_num++;
		}
	}
	if(rec_out_en == 0){
		if(rnd_bs_buf){
		H264DEC_INFO *info_obj = &ctx->emu.info_obj;
		h26xFileSeek(&pat->file.es, -info_obj->uiBSSize, H26XF_SET_CUR);//rollback es file
		emu_h264d_set_bsbuf(p_job);
		}
	}
	{
		// recoder clock cycle for performance check //
		pat->perf.cycle_sum += report[H26X_CYCLE_CNT];
		pat->perf.bslen_sum += report[H26X_BS_LEN];

		if (pat->perf.cycle_max < report[H26X_CYCLE_CNT])
		{
			pat->perf.cycle_max_frm = pat->pic_num;
			pat->perf.cycle_max = report[H26X_CYCLE_CNT];
			pat->perf.cycle_max_bslen = report[H26X_BS_LEN];
		}
	}
    //h26x_module_reset();
}

void emu_h264d_chk_one_pic(h26x_job_t *p_job, UINT32 interrupt, unsigned int rec_out_en, unsigned int  rnd_bs_buf)
{
	chk_one_pic_dec(p_job, interrupt,rec_out_en,rnd_bs_buf);
}


//#endif //if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)
