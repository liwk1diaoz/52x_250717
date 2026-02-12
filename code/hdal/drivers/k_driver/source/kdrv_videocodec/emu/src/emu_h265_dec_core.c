#include "stdio.h" //
#include "stdlib.h"
#include "string.h"
#include "emu_h26x_common.h"
#include "h26x_def.h"
#include "emu_h265_dec.h"
#include "emu_h265_tool.h"

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#define CW_DBG_ROUND_TEST		0


#if CW_DBG_ROUND_TEST
static int pic_count = -1;
static int error_pic = 1;
static int lcu_num = 0;	/* 30 : 384x320 */
#endif

extern UINT8 item_name[H26X_REPORT_NUMBER][256];

UINT32 uiH265TestBSCmdBuf[12][3] = {
	//  BSCmmdNums  BSBufLength  BSBufOffset   //
	//{      0x00001,     0x01000,        0x0},
	{ 0x00001, 0x01000, 0x0 },
	{ 0x00005, 0x01110, 0x1 },
	{ 0x0000A, 0x01055, 0x0 },
	{ 0x0000F, 0x02AAA, 0x3 },
	{ 0x00010, 0x03333, 0x4 },
	{ 0x00044, 0x05075, 0x5 },
	{ 0x00100, 0x060DA, 0x6 },
	{ 0x00120, 0x07111, 0x7 },
	{ 0x00006, 0x0803A, 0x8 },
	{ 0x00005, 0x55555, 0x9 },
	{ 0x00033, 0x7AAAA, 0xa },
	{ 0x00002, 0x80000, 0xb },
};

void h265d_setBsCmdSize(UINT32 uiBsLen, UINT32 *uiCmdSize, UINT32 *uiCmdNum)
{
	UINT32 i;
	*uiCmdNum = (uiBsLen + 0x7ffff) / H265_CMB_BUF_MAX_SIZE;
	for (i = 1; i<*uiCmdNum; i++){
		*(volatile UINT32 *)(uiCmdSize) = H265_CMB_BUF_MAX_SIZE; uiCmdSize++;
		uiBsLen = uiBsLen - H265_CMB_BUF_MAX_SIZE;
	}

	*(volatile UINT32 *)(uiCmdSize) = uiBsLen;
}

void h265d_testBsDma(H26XFile *pH26XFile, UINT32 *uiPicBsLen, UINT32 uiPicNum, UINT32 uiVirBSDMAAddr, UINT32 uiVirBsAddr)
{
	UINT32 uiBSCmmdNums;
	UINT32 uiBSBufLength;
	UINT32 uiBSBufOffset;
	UINT32 uiBsCmdBufNum;
	UINT32 i;
	UINT32 uiTotalSize = 0;
	UINT32 uiPhyBsAddr = h26x_getPhyAddr(uiVirBsAddr);
	UINT32 uiOriVirBSDMAAddr = uiVirBSDMAAddr;
	UINT32 uiOriVirBsAddr = uiVirBsAddr; //hk : for flush cache

	uiBSCmmdNums = uiH265TestBSCmdBuf[uiPicNum % 12][0];
	uiBSBufLength = uiH265TestBSCmdBuf[uiPicNum % 12][1];
	uiBSBufOffset = uiH265TestBSCmdBuf[uiPicNum % 12][2];

	//DBG_INFO("uiBSCmmdNums = 0x%08x uiBSBufLength = 0x%08x uiBSBufOffset = 0x%08x\r\n", uiBSCmmdNums, uiBSBufLength, uiBSBufOffset);
	//DBG_INFO("uiPicBsLen = 0x%08x\r\n", *uiPicBsLen);
	uiBsCmdBufNum = uiBSCmmdNums;

	// last cmd buffer //
	if (*uiPicBsLen == 0) {
		DBG_INFO("Error at h265d_testBsDma (bs empty)\r\n");
		while (1);
	}
	//DBG_INFO("(%x %x, %x)\r\n",uiPhyBsAddr,uiBSBufLength, *uiPicBsLen);
	*(volatile UINT32 *)uiVirBSDMAAddr = uiBsCmdBufNum;
	uiVirBSDMAAddr = uiVirBSDMAAddr + 4;
	for (i = 0; i < uiBsCmdBufNum; i++) {
		// read bs //
		if (uiBSBufLength < *uiPicBsLen) {
			h26xFileRead(pH26XFile, uiBSBufLength, uiVirBsAddr);
			//h26x_flushCache(uiVirBsAddr,SIZE_32X(uiBSBufLength));
			*uiPicBsLen = *uiPicBsLen - uiBSBufLength;
		}
		else {
			h26xFileRead(pH26XFile, *uiPicBsLen, uiVirBsAddr);
			//h26x_flushCache(uiVirBsAddr,SIZE_32X(*uiPicBsLen));
			*uiPicBsLen = 0;
		}

		uiVirBsAddr = uiVirBsAddr + (uiBSBufLength + uiBSBufOffset);

		// set bsdma //
		*(volatile UINT32 *)uiVirBSDMAAddr = uiPhyBsAddr;
		uiVirBSDMAAddr = uiVirBSDMAAddr + 4;
		*(volatile UINT32 *)uiVirBSDMAAddr = uiBSBufLength;
		uiVirBSDMAAddr = uiVirBSDMAAddr + 4;

		//DBG_INFO("(%x %x, %x)\r\n",uiPhyBsAddr,uiBSBufLength, *uiPicBsLen);
		uiPhyBsAddr = uiPhyBsAddr + uiBSBufLength + uiBSBufOffset;
		uiTotalSize += uiBSBufLength + uiBSBufOffset;

		if (*uiPicBsLen == 0) {
			break;
		}
	}
	h26x_flushCache(uiOriVirBsAddr, SIZE_32X(uiVirBsAddr-uiOriVirBsAddr));
	h26x_flushCache(uiOriVirBSDMAAddr, SIZE_32X((uiBsCmdBufNum * 2 + 1) * 4));

}

void emu_h265d_set_nxt_bsbuf(h26x_job_t *p_job)
{
	h265_dec_ctx_t *ctx = (h265_dec_ctx_t *)p_job->ctx_addr;
	h265_dec_pat_t *pat = &ctx->pat;
	h265_dec_emu_t *emu = &ctx->emu;
	H26XDecPicCfg *pH26XDecPicCfg = &emu->sH26XDecPicCfg;
	H26XDecAddr  *pH26XVirDecAddr = &emu->sH26XVirDecAddr;
	H26XDecRegCfg *pH26XRegCfg = &pH26XDecPicCfg->uiH26XRegCfg;

	//DBG_INFO("emu_h265d_set_nxt_bsbuf\r\n");

	// Set BSDMA Buffer //
	h265d_testBsDma(&emu->file.bs, &pH26XRegCfg->uiH26XBsLen, pat->uiPicNum, pH26XVirDecAddr->uiTmpBsAddr, pH26XVirDecAddr->uiHwBsAddr);
	h26x_setNextBsDmaBuf(h26x_getPhyAddr(pH26XVirDecAddr->uiTmpBsAddr));
}

static void report_perf(h26x_job_t *p_job)
{

	h265_dec_ctx_t *ctx = (h265_dec_ctx_t *)p_job->ctx_addr;
	h265_dec_emu_t *emu = &ctx->emu;
	h265_dec_perf_t *perf = &emu->perf;
	h265_dec_pat_t *pat = &ctx->pat;
	H26XDecSeqCfg *pH26XDecSeqCfg = &emu->sH26XDecSeqCfg;
	H26XDecPicCfg *pH26XDecPicCfg = &emu->sH26XDecPicCfg;
	H26XDecRegCfg *pH26XRegCfg = &pH26XDecPicCfg->uiH26XRegCfg;

	UINT32 frm_avg_clk = (UINT32)(perf->cycle_sum/pat->uiPicNum);
	float mbs = (SIZE_64X(emu->uiWidth)/16) * (SIZE_64X(emu->uiHeight)/16);
	float mb_avg_clk = (float)frm_avg_clk/mbs;
	float  max_ratio   = (float)(perf->cycle_max_bslen*8)/perf->cycle_max;
	UINT32 bslen_avg   = perf->bslen_sum/pat->uiPicNum;
	float  bitlen_avg  = (float)(bslen_avg*8)/mbs;
	float  avg_ratio   = (float)(bslen_avg*8)/frm_avg_clk;

	DBG_DUMP("\r\ncycle report (%s, qp(%d)) => max(frm:%d) : c(%d, %.2f) b(%d, %.2f)(%.2f) avg : c(%ld, %.2f) b(%ld, %.2f)(%.2f) \r\n",
		(unsigned char*)pH26XDecSeqCfg->uiH26XInfoCfg.uiYuvName, (unsigned int)pH26XRegCfg->uiH26XPic1&0x3f, (unsigned int)perf->cycle_max_frm,
		(unsigned int)perf->cycle_max, perf->cycle_max_avg, (unsigned int)perf->cycle_max_bslen, perf->cycle_max_bitlen_avg, max_ratio,
		(unsigned long)frm_avg_clk, mb_avg_clk, (unsigned long)bslen_avg, bitlen_avg, avg_ratio);
}

static void emu_h265_dec_init_regset(H26XRegSet *pH26XRegSet, H26XDecAddr *pH26XVirDecAddr)
{
	pH26XRegSet->COL_MVS_WR_ADDR = h26x_getPhyAddr(pH26XVirDecAddr->uiColMvsAddr[pH26XVirDecAddr->uiColMvWIdx]);
	pH26XRegSet->COL_MVS_RD_ADDR = h26x_getPhyAddr(pH26XVirDecAddr->uiColMvsAddr[pH26XVirDecAddr->uiColMvRIdx]);

	/* Rec and Ref Buffer Addr */
	pH26XRegSet->REC_Y_ADDR = h26x_getPhyAddr(pH26XVirDecAddr->uiRefAndRecYAddr[pH26XVirDecAddr->uiRecIdx]);
	pH26XRegSet->REC_C_ADDR = h26x_getPhyAddr(pH26XVirDecAddr->uiRefAndRecUVAddr[pH26XVirDecAddr->uiRecIdx]);

	pH26XRegSet->REF_Y_ADDR = h26x_getPhyAddr(pH26XVirDecAddr->uiRefAndRecYAddr[pH26XVirDecAddr->uiRef0Idx]);
	pH26XRegSet->REF_C_ADDR = h26x_getPhyAddr(pH26XVirDecAddr->uiRefAndRecUVAddr[pH26XVirDecAddr->uiRef0Idx]);

}

static void emu_h265_dec_set_rec_and_ref(h265_dec_ctx_t *ctx)
{
	h265_dec_emu_t *emu = &ctx->emu;
	H26XDecPicCfg *pH26XDecPicCfg = &emu->sH26XDecPicCfg;
	H26XDecAddr  *pH26XVirDecAddr = &emu->sH26XVirDecAddr;
	H26XDecRegCfg *pH26XRegCfg = &pH26XDecPicCfg->uiH26XRegCfg;
	unsigned int rec = (pH26XRegCfg->uiH26X_DIST_FACTOR & 0x03e00000) >> 21;
	unsigned int ref = (pH26XRegCfg->uiH26X_DIST_FACTOR & 0x7c000000) >> 26;
	unsigned int buffer_idx = emu->uiTotalAllocateBuffer;
	unsigned int finial_idx = buffer_idx - 1;
	unsigned int shift_idx = buffer_idx + 2;
	int get_flag = 0;
	unsigned int i;

	if (ref >= buffer_idx){ DBG_INFO("err: h265d frame buffer num too small! ref(%d) frmbuf_num(%d)\r\n", ref, buffer_idx); }

	//DBG_INFO("gop_size_flag : %d\r\n", emu->uiGopSizeEqOneFlag);
	//DBG_INFO("ref(%d) rec(%d)\r\n", ref, rec);

	rec += shift_idx;
	ref += shift_idx;

	if (1 == emu->uiGopSizeEqOneFlag){
		int has_buffer_flag = 0;

		for (i = 0; i < buffer_idx; i++){
			if (ref == pH26XVirDecAddr->uiRecordRecIdx[i]){
				pH26XVirDecAddr->uiRef0Idx = pH26XVirDecAddr->uiColMvRIdx = i;
				if (rec == ref)
					pH26XVirDecAddr->uiKeepRecFlag[i] = 0;
				break;
			}

			/* for first frame */
			if (i == finial_idx){
				pH26XVirDecAddr->uiRef0Idx = pH26XVirDecAddr->uiColMvRIdx = i;
			}
		}

		/* force release one buffer */
		for (i = 0; i < buffer_idx; i++){
			if (1 == pH26XVirDecAddr->uiKeepRecFlag[i]){
				continue;
			}
			else{
				has_buffer_flag = 1;
				break;
			}
		}

		if (0 == has_buffer_flag){
			unsigned int old_buf_idx;
			old_buf_idx = (pH26XVirDecAddr->uiRundReleaseBufCnt + pH26XVirDecAddr->uiKeepRefIdx + ref) % buffer_idx;

			if (pH26XVirDecAddr->uiKeepRefIdx == (old_buf_idx + shift_idx)){
				old_buf_idx = shift_idx % buffer_idx;
				pH26XVirDecAddr->uiRundReleaseBufCnt++;
				pH26XVirDecAddr->uiRundReleaseBufCnt %= buffer_idx;
			}
			pH26XVirDecAddr->uiKeepRecFlag[old_buf_idx] = 0;
			pH26XVirDecAddr->uiRecordRecIdx[old_buf_idx] = buffer_idx;
		}

		/* rec */
		for (i = 0; i < buffer_idx; i++){
			if (0 == get_flag
				&& 0 == pH26XVirDecAddr->uiKeepRecFlag[i]
				&& (buffer_idx == pH26XVirDecAddr->uiRecordRecIdx[i]
				|| (ref != rec && rec == pH26XVirDecAddr->uiRecordRecIdx[i])))
			{
				pH26XVirDecAddr->uiRecIdx = pH26XVirDecAddr->uiColMvWIdx = i;
				pH26XVirDecAddr->uiRecordRecIdx[i] = rec;
				get_flag = 1;
				pH26XVirDecAddr->uiKeepRecFlag[i] = 1;

				continue;
			}

			if (rec == pH26XVirDecAddr->uiRecordRecIdx[i] || 0 == pH26XVirDecAddr->uiKeepRecFlag[i]){
				pH26XVirDecAddr->uiRecordRecIdx[i] = buffer_idx;
				if (rec == ref)
					pH26XVirDecAddr->uiKeepRefIdx = ref;
				else
					pH26XVirDecAddr->uiKeepRecFlag[i] = 0;
			}
		}

	}
	else{
		unsigned int non_ref_idx = shift_idx + 16;

		/*
		*  CUR : 0 ,  ST_0 : 2 ,  LT_0 : 4 , ST_1 : 8 , NON_REF : 16
		*/

		/* ref */
		for (i = 0; i < buffer_idx; i++){
			if (ref == pH26XVirDecAddr->uiRecordRecIdx[i]){
				pH26XVirDecAddr->uiRef0Idx = pH26XVirDecAddr->uiColMvRIdx = i;
				break;
			}

			/* for first frame */
			if (i == finial_idx){
				pH26XVirDecAddr->uiRef0Idx = pH26XVirDecAddr->uiColMvRIdx = i;
			}
		}

		/* rec */
		for (i = 0; i < buffer_idx; i++){
			if (0 == get_flag
				&& (buffer_idx == pH26XVirDecAddr->uiRecordRecIdx[i]
				|| non_ref_idx == pH26XVirDecAddr->uiRecordRecIdx[i]
				|| (ref != rec && rec == pH26XVirDecAddr->uiRecordRecIdx[i]))){
				pH26XVirDecAddr->uiRecIdx = pH26XVirDecAddr->uiColMvWIdx = i;
				pH26XVirDecAddr->uiRecordRecIdx[i] = rec;
				get_flag = 1;
				continue;
			}

			if ((rec == ref || 1 == get_flag) && rec == pH26XVirDecAddr->uiRecordRecIdx[i]){
				pH26XVirDecAddr->uiRecordRecIdx[i] = buffer_idx;
			}
		}
	}
	//DBG_INFO("uiKeepRefIdx(%d)\r\n", pH26XVirDecAddr->uiKeepRefIdx);
	//DBG_INFO("uiRef0Idx(%d) uiRecIdx(%d)\r\n", pH26XVirDecAddr->uiRef0Idx, pH26XVirDecAddr->uiRecIdx);

	//{
	//	unsigned int i;
	//	for (i = 0; i < emu->uiTotalAllocateBuffer; i++){
	//		if (i % 5 == 0)
	//			DBG_INFO("\r\n");
	//		DBG_INFO("c[%d] : %d  ", i, pH26XVirDecAddr->uiRecordRecIdx[i]);

	//	}
	//	for (i = 0; i < emu->uiTotalAllocateBuffer; i++){
	//		if (i % 5 == 0)
	//			DBG_INFO("\r\n");
	//		DBG_INFO("k[%d] : %d  ", i, pH26XVirDecAddr->uiKeepRecFlag[i]);
	//	}
	//	DBG_INFO("\r\n");
	//}

#if CW_DBG_ROUND_TEST
	if (pic_count < error_pic - 1){
		pH26XVirDecAddr->uiRef0Idx = pH26XVirDecAddr->uiColMvRIdx = 0;
		pH26XVirDecAddr->uiRecIdx = pH26XVirDecAddr->uiColMvWIdx = 0;
	}
	else{
		pH26XVirDecAddr->uiRef0Idx = pH26XVirDecAddr->uiColMvRIdx = 0;
		pH26XVirDecAddr->uiRecIdx = pH26XVirDecAddr->uiColMvWIdx = 1;
	}
#endif
}

static void emu_h265_dec_test_write_protect(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item){
	h265_dec_ctx_t *ctx = (h265_dec_ctx_t *)p_job->ctx_addr;
	h265_dec_emu_t *emu = &ctx->emu;
	H26XDecAddr  *pH26XVirDecAddr = &emu->sH26XVirDecAddr;
    UINT32 en = 0;
    //UINT32 flag = 0xF;
    UINT32 test_addr;
    UINT32 test_size;
    UINT32 test_chn = 4;
    //UINT32 cnt = 0;
    //UINT32 test_item_num = 4;

#if 0

    do{
        int choose;
        choose = rand()% (test_item_num);
        if((((en >> choose) & 1) == 0) && ((flag >> choose) & 1))
        {
            en |= (1 << choose);
            cnt++;
        }
    }while(cnt < 3);
#endif

#if 0
    en = 1<<5; //XXXXXXXXXXXXX
    en |= 1<<1; //XXXXXXXXXXXXX
    en = en & flag;
#endif

    en |= 1<<1; //XXXXXXXXXXXXX
    en |= 1<<2; //XXXXXXXXXXXXX

    //DBG_INFO("%s %d en = 0x%08x, flag = 0x%08x\r\n",__FUNCTION__,__LINE__,en,flag);
    if((en >> 0) & 1){
        DBG_INFO("%s %d stack\r\n",__FUNCTION__,__LINE__);
        test_addr = 0x1000;
        //test_size = OS_GetMemAddr(MEM_HEAP); //to HK: need to do //hk tmp : TODO
        h26x_test_wp(test_chn,test_addr,test_size,  rand()%4);
        test_chn++;
    }

    if((en >> 1) & 1){
        //DBG_INFO("%s %d colmv addr = 0x%08x + 0x%08x\r\n",__FUNCTION__,__LINE__,
        //    pH26XVirDecAddr->uiColMvsAddr[pH26XVirDecAddr->uiColMvWIdx], emu->ColMvs_Size);
        test_addr = (pH26XVirDecAddr->uiColMvsAddr[pH26XVirDecAddr->uiColMvWIdx] + emu->ColMvs_Size);
        test_size = 16;
        //test_addr -= 16; //xx
        DBG_INFO("%s %d colmv, 0x%08x, 0x%08x\r\n",__FUNCTION__,__LINE__,(int)test_addr,(int)test_size);
        h26x_test_wp(test_chn,test_addr,test_size,  rand()%4);
        test_chn++;
    }
    if((en >> 2) & 1){
        test_addr = (pH26XVirDecAddr->uiRefAndRecUVAddr[pH26XVirDecAddr->uiRecIdx] + emu->uiFrameSize/2);
        test_size = 16;
        //test_addr -= 16;//xx
        DBG_INFO("%s %d rec, 0x%08x, 0x%08x\r\n",__FUNCTION__,__LINE__,(int)test_addr,(int)test_size);
        h26x_test_wp(test_chn,test_addr,test_size,  rand()%4);
        test_chn++;
    }
    if((en >> 3) & 1){
        test_addr = emu->wp[0];
        test_size = 16;
        DBG_INFO("%s %d all buffer, 0x%08x, 0x%08x\r\n",__FUNCTION__,__LINE__,(int)test_addr,(int)test_size);
        h26x_test_wp(test_chn,test_addr,test_size,  rand()%4);
        test_chn++;
    }
}

static void emu_h265_dec_gen_regset(h26x_job_t *p_job)
{
	h265_dec_ctx_t *ctx = (h265_dec_ctx_t *)p_job->ctx_addr;
	h265_dec_emu_t *emu = &ctx->emu;
	H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
	H26XDecPicCfg *pH26XDecPicCfg = &emu->sH26XDecPicCfg;
	H26XDecAddr  *pH26XVirDecAddr = &emu->sH26XVirDecAddr;
	H26XDecRegCfg *pH26XRegCfg = &pH26XDecPicCfg->uiH26XRegCfg;
	H26XTileSet *pTile = (H26XTileSet*)pH26XRegSet->TILE_CFG;
    //UINT32 tile_width[H26X_MAX_TILEN_NUM];
    UINT32 tilenum_min1;
    //UINT32 i;
	pH26XRegSet->DEC0_CFG = pH26XRegCfg->uiH26XDec0;

    tilenum_min1 = (pH26XRegCfg->uiH26XTile0) & 0x7;
    pTile->TILE_0_MODE = tilenum_min1;
    /*
    tile_width[0] = (pH26XRegCfg->uiH26XTile1 >> 0) & 0xFFFF;
    tile_width[1] = (pH26XRegCfg->uiH26XTile1 >> 16) & 0xFFFF;
    tile_width[2] = (pH26XRegCfg->uiH26XTile2 >> 0) & 0xFFFF;
    tile_width[3] = (pH26XRegCfg->uiH26XTile2 >> 16) & 0xFFFF;
    tile_width[4] = 0;
    if(tilenum_min1 != 0){
        UINT32 uiSum = 0;
        for(i=0;i<tilenum_min1;i++)
            uiSum += tile_width[i];
        tile_width[tilenum_min1] = emu->uiWidth - uiSum;
    }
    */
    //emu_msg(("uiTileNum = %d (%d, %d, %d, %d, %d)\r\n",tilenum_min1,tile_width[0],
    //    tile_width[1],tile_width[2],tile_width[3],tile_width[4]);

    pTile->TILE_WIDTH_0 = emu->tile_width[0] | (emu->tile_width[1] << 16);
    pTile->TILE_WIDTH_1 = emu->tile_width[2] | (emu->tile_width[3] << 16);

	pH26XRegSet->FUNC_CFG[0] = 0;	/* CODEC_TYPE */
	pH26XRegSet->FUNC_CFG[0] |= (1 << 1);	/* CODEC_MODE */
	pH26XRegSet->FUNC_CFG[0] |= (1 << 4);	/* AE enable */

#if 0
	pH26XRegSet->FUNC_CFG[1] = pH26XRegCfg->uiH26XFunc1 & 0xFFFFFFF7;
	pH26XRegSet->FUNC_CFG[1] |= ((emu->uiWidth & 0xFFFF) <= 1920) ? (1 << 3) : 0;
#else
    pH26XRegSet->FUNC_CFG[1] = pH26XRegCfg->uiH26XFunc1;
#endif

	pH26XRegSet->SRC_LINE_OFFSET = pH26XDecPicCfg->uiH26XSrcLineOffset | (pH26XDecPicCfg->uiH26XSrcLineOffset << 16);
	pH26XRegSet->REC_LINE_OFFSET = pH26XDecPicCfg->uiH26XRecLineOffset | (pH26XDecPicCfg->uiH26XRecLineOffset << 16);

	pH26XRegSet->SEQ_CFG[0] = (emu->uiWidth & 0xFFFF) | ((emu->uiHeight & 0xFFFF) << 16);
	pH26XRegSet->SEQ_CFG[1] = (emu->uiWidth & 0xFFFF) | ((emu->uiHeight & 0xFFFF) << 16);

	pH26XRegSet->PIC_CFG = (pH26XRegCfg->uiH26XPic0 & 0x1) | (0x3 << 4);

	pH26XRegSet->QP_CFG[0] = pH26XRegCfg->uiH26XPic1;

	pH26XRegSet->ILF_CFG[0] = pH26XRegCfg->uiH26XPic3 & 0x71FFF;

	pH26XRegSet->AEAD_CFG = pH26XRegCfg->uiH26XPic5;

	pH26XRegSet->DSF_CFG = pH26XRegCfg->uiH26X_DIST_FACTOR & 0x00131FFF;

	pH26XRegSet->DEC1_CFG = pH26XRegCfg->uiH26XDec1 & 0xFFFFFFFF;

    if(pH26XRegCfg->uiH26XBsLen > 0xFFFFFF){
        pH26XRegSet->NAL_HDR_LEN_TOTAL_LEN = 0xFFFFFFFF;
    }else{
        pH26XRegSet->NAL_HDR_LEN_TOTAL_LEN = pH26XRegCfg->uiH26XBsLen;
    }

	pH26XRegSet->REC_Y_ADDR = h26x_getPhyAddr(pH26XVirDecAddr->uiRefAndRecYAddr[pH26XVirDecAddr->uiRecIdx]);
	pH26XRegSet->REC_C_ADDR = h26x_getPhyAddr(pH26XVirDecAddr->uiRefAndRecUVAddr[pH26XVirDecAddr->uiRecIdx]);

	pH26XRegSet->REF_Y_ADDR = h26x_getPhyAddr(pH26XVirDecAddr->uiRefAndRecYAddr[pH26XVirDecAddr->uiRef0Idx]);
	pH26XRegSet->REF_C_ADDR = h26x_getPhyAddr(pH26XVirDecAddr->uiRefAndRecUVAddr[pH26XVirDecAddr->uiRef0Idx]);

	/* set bs dma cmd */
	if (emu->uiRndBsBuf){
		h265_dec_pat_t *pat = &ctx->pat;
		pH26XRegSet->BSDMA_CMD_BUF_ADDR = h26x_getPhyAddr(pH26XVirDecAddr->uiTmpBsAddr);
		h265d_testBsDma(&emu->file.bs, &pH26XRegCfg->uiH26XBsLen, pat->uiPicNum, pH26XVirDecAddr->uiTmpBsAddr, pH26XVirDecAddr->uiHwBsAddr);
	}
	else{
		pH26XRegSet->BSDMA_CMD_BUF_ADDR = h26x_getPhyAddr(pH26XVirDecAddr->uiCMDBufAddr);
		h26x_setBSDMA(pH26XVirDecAddr->uiCMDBufAddr, pH26XVirDecAddr->uiHwBSCmdNum,
			h26x_getPhyAddr(pH26XVirDecAddr->uiHwBsAddr), pH26XVirDecAddr->uiHwBsCmdSize);
	}

	pH26XRegSet->COL_MVS_WR_ADDR = h26x_getPhyAddr(pH26XVirDecAddr->uiColMvsAddr[pH26XVirDecAddr->uiColMvWIdx]);
	pH26XRegSet->COL_MVS_RD_ADDR = h26x_getPhyAddr(pH26XVirDecAddr->uiColMvsAddr[pH26XVirDecAddr->uiColMvRIdx]);


	//frame time out count, unit: 256T
	pH26XRegSet->TIMEOUT_CNT_MAX =  100 * 270 * (emu->uiWidth/16) *  (emu->uiHeight/16) /256 ;

#if CW_DBG_ROUND_TEST
	{
		//hk: x pos, y pos
		UINT32 uiVal, x, y;
		x = 0;
		y = 0;
		uiVal = 1 << 31;
		//uiVal |= ((uiTestMainLoopCnt) & 0xFFFF);

		if (pic_count >= error_pic){
			UINT32 tmp_count = pic_count - error_pic;
			//352x288
			x = tmp_count % 6;	//hk : hardcode for ctb_w = 6
			y = tmp_count / 6;
		}

		uiVal |= (x & 0x1FF);
		uiVal |= (y & 0x1FF) << 16;
		DBG_INFO("uictbx = %d uictby = %d uiVal=%8x\r\n", (int)x, (int)y, (int)uiVal);
		pH26XRegSet->RES_24C = uiVal;
	}
#endif
}

static void emu_h265_read_regcfg(h265_dec_emu_t *emu)
{
	H26XDecPicCfg *pH26XDecPicCfg = &emu->sH26XDecPicCfg;
	H26XDecRegCfg *pH26XRegCfg = &pH26XDecPicCfg->uiH26XRegCfg;
	h26xFileRead(&emu->file.reg_cfg, (UINT32)sizeof(H26XDecRegCfg), (UINT32)pH26XRegCfg);


	if (emu->uiTileNum)
		DBG_INFO("uiTileEn = %d\r\n", (int)emu->uiTileNum);

	pH26XDecPicCfg->uiH26XSrcLineOffset = (emu->uiRecLineOffset >> 2);
	pH26XDecPicCfg->uiH26XRecLineOffset = (emu->uiRecLineOffset >> 2);

}

static void emu_h265_read_bs(h265_dec_emu_t *emu)
{
	H26XDecPicCfg *pH26XDecPicCfg = &emu->sH26XDecPicCfg;
	H26XDecAddr  *pH26XVirDecAddr = &emu->sH26XVirDecAddr;
	H26XDecRegCfg *pH26XRegCfg = &pH26XDecPicCfg->uiH26XRegCfg;
	UINT32    uiPicBsLen = 0;

	uiPicBsLen = pH26XRegCfg->uiH26XBsLen;

	if (0 == emu->uiRndBsBuf){
		h26xFileRead(&emu->file.bs, uiPicBsLen, pH26XVirDecAddr->uiHwBsAddr);
		h26x_flushCache(pH26XVirDecAddr->uiHwBsAddr, SIZE_32X(uiPicBsLen));
		h265d_setBsCmdSize(uiPicBsLen, pH26XVirDecAddr->uiHwBsCmdSize, &pH26XVirDecAddr->uiHwBSCmdNum);
	}

	if (pH26XVirDecAddr->uiHwBSCmdNum > H265_MAX_BSCMD_NUM) {
		DBG_INFO("Err at dec, uiHwBSCmdNum = %d > %d\r\n", (int)pH26XVirDecAddr->uiHwBSCmdNum, (int)H265_MAX_BSCMD_NUM);
	}

}

INT32 emu_h265_dec_prepare_one_pic_core(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item)
{
	h265_dec_ctx_t *ctx = (h265_dec_ctx_t *)p_job->ctx_addr;
	h265_dec_emu_t *emu = &ctx->emu;
	h265_dec_pat_t *pat = (h265_dec_pat_t *)&ctx->pat;
	H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
	H26XDecAddr  *pH26XVirDecAddr = &emu->sH26XVirDecAddr;

#if CW_DBG_ROUND_TEST
	if (pic_count < error_pic){
		emu_h265_read_regcfg(emu);
		emu_h265_read_bs(emu);
	}
#else
	emu_h265_read_regcfg(emu);
	emu_h265_read_bs(emu);
#endif

	/* Set Initialize Register */
	if (pat->uiPicNum == 0){
		/* gen Memory Buffer(Ref and Temprol Buffer) Register */
		emu_h265_dec_init_regset(pH26XRegSet, pH26XVirDecAddr);
	}

	emu_h265_dec_set_rec_and_ref(ctx);

	emu_h265_dec_gen_regset(p_job);

    if (p_ver_item->write_prot)
    {
        emu_h265_dec_test_write_protect( p_job, p_ver_item);
    }


	return H265_EMU_PASS;
}


INT32 emu_h265d_prepare_one_pic(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item)
{
	int end_src, start_pat, end_pat, max_fn;
	h26x_mem_t *mem = &p_job->mem;
	h265_dec_ctx_t *ctx = (h265_dec_ctx_t *)p_job->ctx_addr;
	h265_dec_emu_t *emu = &ctx->emu;
	h265_dec_pat_t *pat = (h265_dec_pat_t *)&ctx->pat;
	H26XDecSeqCfg *pH26XDecSeqCfg = &emu->sH26XDecSeqCfg;
#if (H265D_ENDLESS_RUN)
    int start_src;
    start_src = p_job->start_folder_idx;
#endif

	start_pat = p_job->start_pat_idx;
	end_src = p_job->end_folder_idx;
	end_pat = p_job->end_pat_idx;
	max_fn = (pH26XDecSeqCfg->uiH26XInfoCfg.uiFrameNum < p_job->end_frm_num) ? pH26XDecSeqCfg->uiH26XInfoCfg.uiFrameNum : p_job->end_frm_num;

	if (p_job->is_finish == 1)
	{
		return 1;
	}

	if ((int)pat->uiPicNum >= max_fn)
	{
		int ret;
		UINT8 pucDir[265];
		DBG_INFO("%s %d,pic max\r\n", __FUNCTION__, __LINE__);
		report_perf(p_job);
		emu_h265d_close_one_sequence(p_job, ctx, mem);
		pat->uiPicNum = 0;

		do{
			pat->uiPatNum++;
			if ((int)pat->uiPatNum >= end_pat)
			{
				DBG_INFO("%s %d,pat max\r\n", __FUNCTION__, __LINE__);
				pat->uiPicNum = 0;
				pat->uiPatNum = start_pat;
				pat->uiSrcNum++;
			}
			if ((int)pat->uiSrcNum >= end_src)
			{
				DBG_INFO("%s %d,src max\r\n", __FUNCTION__, __LINE__);
#if H265D_ENDLESS_RUN
                pat->uiPicNum = 0;
                pat->uiSrcNum = start_src;
                pat->uiPatNum = start_pat;
                p_job->end_frm_num = p_job->end_frm_num;
#else
				p_job->is_finish = 1;
				DBG_INFO("is finish\r\n");
				return 1;
#endif
			}
			memcpy((void*)pucDir, emu->ucFileDir, sizeof(emu->ucFileDir));
			ret = emu_h265d_init(p_ver_item, p_job, pucDir);
		} while (ret == 1);
	}

	emu_h265_dec_prepare_one_pic_core(p_job,p_ver_item);

	return 0;
}

#if WRITE_REC
static void emu_h265_write_rec(h265_dec_emu_t *emu)
{
	H26XDecAddr  *pH26XVirDecAddr = &emu->sH26XVirDecAddr;

	h26x_flushCache(pH26XVirDecAddr->uiRefAndRecYAddr[pH26XVirDecAddr->uiRecIdx], emu->uiFrameSize);
	h26x_flushCache(pH26XVirDecAddr->uiRefAndRecUVAddr[pH26XVirDecAddr->uiRecIdx], (emu->uiFrameSize / 2);

	h26xFileWrite(&emu->file.hw_rec, emu->uiFrameSize, pH26XVirDecAddr->uiRefAndRecYAddr[pH26XVirDecAddr->uiRecIdx]);
	h26xFileWrite(&emu->file.hw_rec, emu->uiFrameSize / 2, pH26XVirDecAddr->uiRefAndRecUVAddr[pH26XVirDecAddr->uiRecIdx]);
}
#endif

#if DUMP_ERROR_REC
static void emu_h265_dump_err_rec(h26x_job_t *p_job)
{
	h265_dec_ctx_t *ctx = (h265_dec_ctx_t *)p_job->ctx_addr;
	h265_dec_emu_t *emu = &ctx->emu;
	h265_dec_pat_t *pat = &ctx->pat;
	H26XDecAddr  *pH26XVirDecAddr = &emu->sH26XVirDecAddr;
	UINT8	*pucFileName[256], *pucFileDir[256], *pucFpgaDir[256];;

	H26XFile fp;
	sprintf((char *)pucFileDir, "A:\\%s\\dec_pattern\\%s\\%s", emu->ucFileDir, gucH26XEncTestSrc_Cut[pat->uiSrcNum][0], gucH26XEncTestSrc_Cut[pat->uiSrcNum][1]);
	sprintf((char *)pucFpgaDir, "%s\\E_%d", pucFileDir, pat->uiPatNum);
	sprintf((char *)pucFileName, "%s\\hw_err_rec_%s_E%d_%d.dat", pucFpgaDir, gucH26XEncTestSrc_Cut[pat->uiSrcNum][1], pat->uiPatNum, pat->uiPicNum);
	h26xFileOpen(&fp, (char *)pucFileName);

	h26x_flushCache(pH26XVirDecAddr->uiRefAndRecYAddr[pH26XVirDecAddr->uiRecIdx], emu->uiFrameSize);
	h26x_flushCache(pH26XVirDecAddr->uiRefAndRecUVAddr[pH26XVirDecAddr->uiRecIdx], (emu->uiFrameSize / 2);

	h26xFileWrite(&fp, emu->uiFrameSize, pH26XVirDecAddr->uiRefAndRecYAddr[pH26XVirDecAddr->uiRecIdx]);
	h26xFileWrite(&fp, emu->uiFrameSize / 2, pH26XVirDecAddr->uiRefAndRecUVAddr[pH26XVirDecAddr->uiRecIdx]);

	h26xFileClose(&fp);
}
#endif

static UINT32 emu_cmp(UINT32 sw_val, UINT32 hw_val, UINT32 enable, UINT8 item[256]){
	UINT32 result = TRUE;

	if (enable == 0) return result;
	result = (sw_val == hw_val);
	if (result == 0)
	{
		DBG_INFO("SW:0x%08x, HW:0x%08x, %30s diff\r\n", (int)sw_val, (int)hw_val, (char*)item);
	}
	else{
		//DBG_INFO("SW:0x%08x, HW:0x%08x, %30s\r\n",sw_val,hw_val,item);
	}
	return result;
}

// Check Resutlt //
static BOOL EmuH26XDecCheckResult(h26x_job_t *p_job, UINT32 interrupt)
{
	h265_dec_ctx_t *ctx = (h265_dec_ctx_t *)p_job->ctx_addr;
	h265_dec_emu_t *emu = &ctx->emu;
	h265_dec_pat_t *pat = &ctx->pat;
	H26XDecSeqCfg *pH26XDecSeqCfg = &emu->sH26XDecSeqCfg;
	H26XDecPicCfg *pH26XDecPicCfg = &emu->sH26XDecPicCfg;
	H26XDecRegCfg *pH26XRegCfg = &pH26XDecPicCfg->uiH26XRegCfg;
	//UINT32 uiPicType = pH26XRegCfg->uiH26XPic0 & 0x1;
	//UINT32 uiSrcNum = pat->uiSrcNum;
	//UINT32 uiPatNum = pat->uiPatNum;
	//UINT32 uiPicNum = pat->uiPicNum;

	UINT32 item_id;
	BOOL result = TRUE;
	H26XRegSet	 *pH26XRegSet = emu->pH26XRegSet;

#if DIRECT_MODE

	 unsigned int *CHK_REPORT = ( unsigned int *) pH26XRegSet->CHK_REPORT;
#else
#if 0
	 unsigned int *CHK_REPORT = ( unsigned int *)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
//dma_getNonCacheAddr
#else //hk tmp hack
unsigned int *CHK_REPORT = (unsigned int *)(p_job->check1_addr);
//unsigned int *CHK_REPORT_2 = (unsigned int *)(p_job->tmp_report2_addr);

h26x_flushCacheRead((UINT32)p_job->check1_addr , H26X_REG_REPORT_SIZE); //hk/
//h26x_flushCacheRead((UINT32)p_job->check2_addr , H26X_REG_REPORT_SIZE); //hk/
h26x_flushCacheRead(p_job->apb_addr + H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET, H26X_REG_REPORT_SIZE);



//DBG_INFO("CHK_REPORT 0x%08x \r\n",(unsigned int)CHK_REPORT);

#endif

#endif


	//hk
	{
 		H26XDecAddr  *pH26XVirDecAddr = &emu->sH26XVirDecAddr;
		UINT32 Y_Size;
		#if (H265_EMU_REC_LINEOFFSET)
			Y_Size = ((H265_LINEOFFSET_MAX+63)>>6) * ((H265_LINEOFFSET_MAX+63)>>6) * 64 * 64;
		#else
			Y_Size = emu->uiFrameSize;
		#endif

		if( (pH26XRegSet->PIC_CFG>>4) & 1 ){ //REC_OUT_EN
 			h26x_flushCacheRead(pH26XVirDecAddr->uiRefAndRecYAddr[pH26XVirDecAddr->uiRecIdx], Y_Size);
	 		h26x_flushCacheRead(pH26XVirDecAddr->uiRefAndRecUVAddr[pH26XVirDecAddr->uiRecIdx], (Y_Size / 2));
		}
		if( (pH26XRegSet->PIC_CFG>>5) & 1 ){ //COL_MVS_OUT_EN
			h26x_flushCacheRead(pH26XVirDecAddr->uiColMvsAddr[pH26XVirDecAddr->uiColMvWIdx], emu->ColMvs_Size);
		}
	}


	if ((interrupt & H26X_FINISH_INT) == 0){
		DBG_INFO("Src[%d] Pat[%d] Pic[%d] Error Interrupt[0x%08x]!\r\n", (int)pat->uiSrcNum,(int) pat->uiPatNum, (int)pat->uiPicNum, (int)interrupt);
		//bRet = FALSE;
	}
	else{
		//DBG_INFO("Src[%d] Pat[%d] Pic[%d] Correct Interrupt[0x%08x]!\r\n", uiSrcNum, uiPatNum, uiPicNum, interrupt);
	}

	item_id = H26X_REC_CHKSUM;
	result &= emu_cmp(pH26XRegCfg->uiH26XRecChkSum, CHK_REPORT[item_id], 1, (UINT8*)&item_name[item_id]);

	{
		h265_dec_perf_t *perf = &emu->perf;
		UINT32 cycle = CHK_REPORT[H26X_CYCLE_CNT];
		float mbs = (SIZE_64X(emu->uiWidth)/16) * (SIZE_64X(emu->uiHeight)/16);
		float avg = (float)cycle/mbs;
		perf->cycle_sum += cycle;
		perf->bslen_sum += pH26XRegCfg->uiH26XBsLen;
		if (perf->cycle_max < cycle)
		{
			perf->cycle_max = cycle;
			perf->cycle_max_avg = avg;
			perf->cycle_max_bslen = pH26XRegCfg->uiH26XBsLen;
			perf->cycle_max_bitlen_avg = (perf->cycle_max_bslen*8)/mbs;
			perf->cycle_max_frm = pat->uiPicNum;
		}
		//DBG_INFO("cycle = (%d, %f)\r\n",(unsigned int)cycle,avg);
	}

	if (result == TRUE){
		//DBG_INFO("\r\n");
		//DBG_INFO("^C\033[1m.................................... h265  [%d][%d][%d] Correct\r\n",src_num,pat_num,pic_num);

        DBG_DUMP("^C\033[1m.................................... h265dec (%d)(%d) [%d][%d][%d] Correct\033[m\r\n",(int)p_job->idx1,(int)p_job->idx2,(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
		//h26x_prtReg();
		//h26x_getDebug();
		//h26xPrintString(g_main_argv,(INT32)pH26XDecSeqCfg->uiH26XInfoCfg.uiCommandLen);
	}
	else{
		//UINT8  type_name[2][64] = { "I", "P" };
		DBG_DUMP(".................................... h265  [%d][%d][%d] Fail\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
        DBG_DUMP("^C\033[1m.................................... h265dec (%d)(%d) [%d][%d][%d] Fail\r\n",(int)p_job->idx1,(int)p_job->idx2,(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
		//DBG_ERR("[%s]\r\n", type_name[uiPicType]);
#if DUMP_ERROR_REC
		emu_h265_dump_err_rec(p_job);
#endif
		//while(1){};
		h26x_prtReg();
		h26x_getDebug();

		if (DIRECT_MODE == 0)
		{
			DBG_INFO("job(%d) apb data\r\n", p_job->idx1);
			h26x_prtMem(p_job->apb_addr, h26x_getHwRegSize());
			DBG_INFO("job(%d) linklist data\r\n", p_job->idx1);
			h26x_prtMem(p_job->llc_addr, h26x_getHwRegSize());
		}

		h26xPrintString(emu->g_main_argv, (INT32)pH26XDecSeqCfg->uiH26XInfoCfg.uiCommandLen);
#if 1
        h26x_module_reset();
#endif


	}

#if WRITE_REC
	emu_h265_write_rec(emu);
#endif

	return result;
}

void emu_h265d_chk_one_pic(h26x_job_t *p_job, UINT32 interrupt)
{
	BOOL bH26XDecCheckRet = TRUE;
	h265_dec_ctx_t *ctx = (h265_dec_ctx_t *)p_job->ctx_addr;
	h265_dec_pat_t *pat = (h265_dec_pat_t *)&ctx->pat;

#if DIRECT_MODE
	h265_dec_emu_t *emu = &ctx->emu;
#endif

#if DIRECT_MODE
	H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
	h26x_getEncReport(pH26XRegSet->CHK_REPORT);	// shall remove while link-list //
	h26x_getEncReport2(pH26XRegSet->CHK_REPORT_2);	// shall remove while link-list //
#endif

	bH26XDecCheckRet = EmuH26XDecCheckResult(p_job, interrupt);

	if (bH26XDecCheckRet == TRUE)
	{
		pat->uiPicNum++;
#if CW_DBG_ROUND_TEST
		pic_count++;
#endif
	}
	else{
#if CW_DBG_ROUND_TEST
		if (pic_count == (lcu_num + error_pic - 1)){
			pat->uiPicNum = 65536;
			pic_count = 0;
		}
		else{
			pic_count++;
		}
		DBG_INFO("pic_count : %d\r\n", pic_count);
#else
		pat->uiPicNum = 65536;
#endif
	}
}

//#endif	/* if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE) */
