#include "stdio.h" //
#include "stdlib.h"
#include "string.h"
#include "emu_h26x_common.h"

#include "emu_h265_dec.h"
#include "emu_h265_tool.h"

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#define H26XDEC_CHEKMEM(x) do{if(x==0){DBG_ERR("Error!!! Mallocate Fail! %x\r\n",(int)x);} }while(0)

static UINT32 emu_h265_open_dec_info(h265_dec_emu_t *emu, UINT8 *pucFpgaDir[256])
{
	H26XDecSeqCfg *pH26XDecSeqCfg = &emu->sH26XDecSeqCfg;
	H26XDecInfoCfg *pH26XInfoCfg = &pH26XDecSeqCfg->uiH26XInfoCfg;
	UINT8   *pucFileName[256];
	UINT32 uiCommandLen;
	UINT32 uiRet = 0;

	sprintf((char *)pucFileName, "%s\\%s", (char *)pucFpgaDir, "info_data.dat");


	h26xFileOpen(&emu->file.info_cfg, (char *)pucFileName);
	uiRet = h26xFileRead(&emu->file.info_cfg, sizeof(H26XDecInfoCfg), (UINT32)pH26XInfoCfg);
	if (uiRet) return uiRet;

	emu->uiWidth = pH26XInfoCfg->uiWidth;
	emu->uiHeight = pH26XInfoCfg->uiHeight;
	uiCommandLen = pH26XInfoCfg->uiCommandLen;
	uiRet = h26xFileRead(&emu->file.info_cfg, uiCommandLen*sizeof(char), (UINT32)emu->g_main_argv);
	if (uiRet) return uiRet;

	memset(&emu->g_main_argv[uiCommandLen], 0, sizeof(emu->g_main_argv) - uiCommandLen);
	h26xFileRead(&emu->file.info_cfg, sizeof(UINT32), (UINT32)&emu->uiGopSizeEqOneFlag);

	h26xFileClose(&emu->file.info_cfg);

	return uiRet;
}

static void emu_h265_open_dec_pat(h26x_job_t *p_job, UINT8 *pucFpgaDir[256])
{
	h265_dec_ctx_t *ctx = (h265_dec_ctx_t *)p_job->ctx_addr;
	h265_dec_emu_t *emu = &ctx->emu;
	UINT8   *pucFileName[256];

#if WRITE_REC
	h265_dec_pat_t *pat = &ctx->pat;
#endif

	sprintf((char *)pucFileName, "%s\\%s", (char *)pucFpgaDir, "reg_data.dat");
	h26xFileOpen(&emu->file.reg_cfg, (char *)pucFileName);

	sprintf((char *)pucFileName, "%s\\%s", (char *)pucFpgaDir, "es_data.dat");
	h26xFileOpen(&emu->file.bs, (char *)pucFileName);

#if WRITE_REC
	{
		static int id = 0;
		sprintf((char *)pucFileName, "A:\\hw_rec_%s_E%d_%d.dat", gucH26XEncTestSrc_Cut[pat->uiSrcNum][1], pat->uiPatNum, id++);
		h26xFileOpen(&emu->file.hw_rec, (char *)pucFileName);

		DBG_INFO("pucFileName = %s\n", (char *)pucFileName);
	}
#endif

}

UINT32 emu_h265d_close_one_sequence(h26x_job_t *p_job, h265_dec_ctx_t *ctx, h26x_mem_t *p_mem)
{
	h265_dec_emu_t *emu = &ctx->emu;

	h26xFileClose(&emu->file.reg_cfg);
	h26xFileClose(&emu->file.bs);

#ifdef WRITE_REC
	h26xFileClose(&emu->file.hw_rec);
#endif

	return 0;
}

UINT32 emu_h265d_prepare_one_sequence(h26x_job_t *p_job, h265_dec_ctx_t *ctx, h26x_mem_t *p_mem)
{
	UINT32 i;
	INT32 malloc_size = 0;
	INT32 uiStartAdress;
	UINT32 uiRet = 0;
	UINT32 uiVirMemAddr;
	UINT32 Y_Size;
	UINT32 UV_Size;
	UINT32 uiWidth;
	UINT32 uiHeight;
	h26x_mem_t *mem = p_mem;
	h265_dec_emu_t *emu = &ctx->emu;
	h265_dec_pat_t *pat = &ctx->pat;
	UINT32 uiSrcNum = pat->uiSrcNum;
	UINT32 uiPatNum = pat->uiPatNum;
	H26XDecSeqCfg *pH26XDecSeqCfg = &emu->sH26XDecSeqCfg;
	H26XDecAddr  *pH26XVirDecAddr = &emu->sH26XVirDecAddr;
	H26XDecInfoCfg *pH26XInfoCfg = &pH26XDecSeqCfg->uiH26XInfoCfg;
	UINT8  *pucFileDir[256], *pucFpgaDir[256];
    H26XDecRegCfg sTmpRegCfg;

	{
		h265_dec_perf_t *perf = &emu->perf;
		perf->cycle_sum = 0;
		perf->cycle_max = 0;
		perf->cycle_max_avg = 0.0;
		perf->cycle_max_frm = 0;
		perf->cycle_max_bslen = 0;
		perf->cycle_max_bitlen_avg= 0.0;
		perf->bslen_sum = 0;
	}

	DBG_INFO("\r\n\r\n###########################################################\r\n");
	uiVirMemAddr = mem->addr;
	uiStartAdress = uiVirMemAddr;

	DBG_INFO("uiVirMemAddr = 0x%08x\r\n", (int)uiVirMemAddr);
	sprintf((char *)pucFileDir, "A:\\%s\\dec_pattern\\%s\\%s", (char *)emu->ucFileDir, gucH26XEncTestSrc_Cut[uiSrcNum][0], gucH26XEncTestSrc_Cut[uiSrcNum][1]);
	sprintf((char *)pucFpgaDir, "%s\\E_%d", (char *)pucFileDir, (int)uiPatNum);

	DBG_INFO("ucFileDir = %s\r\n", (char *)pucFileDir);
	DBG_INFO("pucFpgaDir = %s\r\n", (char *)pucFpgaDir);

	uiRet = emu_h265_open_dec_info(emu, pucFpgaDir);
	if (uiRet) return uiRet;

	uiWidth = emu->uiWidth;
	uiHeight = emu->uiHeight;
	DBG_INFO("Current dec test = [ %s ][ %s ] [ E_%d ] (%d ,%d)\r\n", (char *)emu->ucFileDir, gucH26XEncTestSrc_Cut[uiSrcNum][1], (int)uiPatNum, (int)uiSrcNum, (int)uiPatNum);
	DBG_INFO("%d x %d, fn = %d, arg_len = %d\r\n", (int)uiWidth, (int)uiHeight, (int)pH26XInfoCfg->uiFrameNum, (int)pH26XInfoCfg->uiCommandLen);

	emu_h265_open_dec_pat(p_job, pucFpgaDir);
    {
        UINT32 i;
        h26xFilePreRead(&emu->file.reg_cfg,(UINT32)sizeof(H26XDecRegCfg),(UINT32)&sTmpRegCfg);

        emu->uiTileNum = (sTmpRegCfg.uiH26XTile0) & 0x7;

        emu->tile_width[0] = (sTmpRegCfg.uiH26XTile1 >> 0) & 0xFFFF;
        emu->tile_width[1] = (sTmpRegCfg.uiH26XTile1 >> 16) & 0xFFFF;
        emu->tile_width[2] = (sTmpRegCfg.uiH26XTile2 >> 0) & 0xFFFF;
        emu->tile_width[3] = (sTmpRegCfg.uiH26XTile2 >> 16) & 0xFFFF;
        emu->tile_width[4] = 0;
        if(emu->uiTileNum != 0){
            UINT32 uiSum = 0;
            for(i=0;i<emu->uiTileNum;i++)
                uiSum += emu->tile_width[i];
            emu->tile_width[emu->uiTileNum] = emu->uiWidth - uiSum;
        }
        DBG_INFO("uiTileNum = %d (%d, %d, %d, %d, %d)\r\n",(int)emu->uiTileNum,(int)emu->tile_width[0],
            (int)emu->tile_width[1],(int)emu->tile_width[2],(int)emu->tile_width[3],(int)emu->tile_width[4]);

    }
#if 0
    if(0){
        H26XDecRegCfg sTmpRegCfg;
        //int ec;
        //int osd = (emu->g_SrcInMode >>HEVC_EMU_INFO_OSD) & 0x1;
        //int grey;
        int tile;
        int last_tilewidth;
        //int tilewidth1,tilewidth2;
        //int roi,aq,mbqp,fro;
        //int var_ro_i_delta;
        //int var_ro_p_delta;
        //int t;
        //int maq;
        //int jnd;
        //int svc;
        //int tmvp;
        h26xFilePreRead(&emu->file.reg_cfg,(UINT32)sizeof(H26XDecRegCfg),(UINT32)&sTmpRegCfg);
        //int tmnr = (emu->g_SrcInMode >>HEVC_EMU_INFO_TMNR) & 0x1;
        //ec = (sTmpRegCfg.uiH26XFunc0 >> 13) & 1;
        //grey = (sTmpRegCfg.uiH26XFunc0 >> 11) & 1;
        //tmvp = (sTmpRegCfg.uiH26XFunc0 >> 11) & 1;
        //svc = (emu->g_SrcInMode >>HEVC_EMU_INFO_SVC) & 0x1;
        tile = sTmpRegCfg.uiH26XTile0 & 0x7;
        last_tilewidth = emu->tile_width[emu->uiTileNum];
        //tilewidth1 = (sTmpRegCfg.uiH26XTile0 >> 16) & 0xFFFF;
        //tilewidth2 = uiWidth - tilewidth1;
        //roi = (sTmpRegCfg.uiH26XRoi0 >> 0) & 1;
        //aq = (sTmpRegCfg.uiH26XSraq0 >> 0) & 1;
        //mbqp = (sTmpRegCfg.uiH26XFunc0 >> 8) & 1;
        //fro = (sTmpRegCfg.uiH26XFunc0 >> 7) & 1;
        //var_ro_i_delta = (sTmpRegCfg.uiH26XVar1 >> 0) & 0xFF;
        //var_ro_p_delta = (sTmpRegCfg.uiH26XVar1 >> 8) & 0xFF;
        //maq = (sTmpRegCfg.uiH26XMaq0 >> 0) & 0x3;
        //jnd = (sTmpRegCfg.uiH26X_JND_0 >> 0) & 0x1;
        //t = (fro && ((var_ro_i_delta != 0) || (var_ro_p_delta != 0));
        //DBG_INFO("(roi = %d, aq = %d, mbqp = %d, fro = %d, var_i = %d, var_p = %d) t = %d\r\n",roi,aq,mbqp,fro,var_ro_i_delta,var_ro_p_delta,t);
        //if(fro && (var_ro_i_delta == 0) && (var_ro_p_delta == 0) && (roi == 0) && (aq == 0) && (mbqp == 0) )
        //{
        //    int c;
        //    for(c= 0; c < 10;c++)
        //    {
        //        DBG_INFO("ooooooooooooooooooooooooooooooooooooooooooo\r\n");
        //    }
        //}
        //DBG_INFO(" (tile = %d, %d, %d), grey = %d\r\n",tile,tilewidth1,tilewidth2,grey);

        //DBG_INFO("(ec = %d, grey = %d,tmvp = %d, svc = %d\r\n",ec,grey,tmvp,svc);
        //if(ec == 0 && tile == 1)
        //if((tmvp == 1 || grey == 1))
        //if((grey == 1))
        //if((tile == 1 && grey == 1))
            //|| (tmvp == 1 && svc == 1))
        //if(roi || aq || mbqp || t)
        //if( tile == 1 && ((tilewidth1 > 1408) || (tilewidth2 > 1408)))
        //if( tile == 1 && maq != 0)
        //if( maq != 0)
        //if(jnd != 0)
        //if(osd != 0)
        //if(uiWidth > 4096 || tile > 0 ) //0118 ov
        if(uiWidth > 4096 || tile > 4)
        //if(tile != 0)
        {
            DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (uiWidth = %d)\r\n",uiWidth);
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (osd = %d\r\n",osd);
            DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (tile = %d\r\n",tile);
            DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (last_tilewidth = %d\r\n",last_tilewidth);
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (ec = %d, grey = %d\r\n",ec,grey);
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (tmvp = %d, svc = %d\r\n",tmvp,svc);
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (jnd = %d\r\n",jnd);
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (maq = %d, tile = %d)\r\n",maq,tile);
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now (tile = %d, %d, %d)\r\n",tile,tilewidth1,tilewidth2);
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now (tile = %d, %d, %d)\r\n",tile,tilewidth1,tilewidth2);
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (ec = %d, tile = %d)\r\n",ec,tile);
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (roi = %d, aq = %d, mbqp = %d, fro = %d, var_i = %d, var_p = %d) t = %d\r\n",roi,aq,mbqp,fro,var_ro_i_delta,var_ro_p_delta,t);
            //emu_h265_close_one_sequence( p_job, ctx, mem);
            return 1;
        }
    }
#endif

	if (H265_EMU_REC_LINEOFFSET && uiWidth != H265_LINEOFFSET_MAX) {
		UINT32 uiWidth_64X = (uiWidth + 63) / 64 * 64;
		emu->uiRecLineOffset = (((rand() % (H265_LINEOFFSET_MAX - uiWidth_64X)) / 16) * 16) + uiWidth_64X;
	}
	else {
		emu->uiRecLineOffset = (uiWidth + 63) / 64 * 64;
	}
	emu->uiRecLineOffset = (emu->uiRecLineOffset + 63) / 64 * 64;

	DBG_INFO("H265_EMU_REC_LINEOFFSET         = %d\r\n", H265_EMU_REC_LINEOFFSET);
	DBG_INFO("uiRecLineOffset  = %d\r\n", (int)emu->uiRecLineOffset);

	emu->uiSrcFrmSize = uiWidth * uiHeight;
	emu->uiFrameSize = ((uiWidth + 63) >> 6) * ((uiHeight + 63) >> 6) * 64 * 64;
	emu->ColMvs_Size = ((uiWidth + 63) >> 6) * ((uiHeight + 63) >> 6) * (16 * 4) * 4;

#if (H265_EMU_REC_LINEOFFSET)
	Y_Size = ((H265_LINEOFFSET_MAX+63)>>6) * ((H265_LINEOFFSET_MAX+63)>>6) * 64 * 64;
#else
	Y_Size = emu->uiFrameSize;
#endif
	UV_Size = (Y_Size / 2);

	DBG_INFO("Y_Size         = 0x%08x\r\n", (int)Y_Size);
	DBG_INFO("UV_Size        = 0x%08x\r\n", (int)UV_Size);

#if 0
	if (DIRECT_MODE){
		emu->uiTotalAllocateBuffer = FRM_DEC_IDX_MAX;
	}
	else{
		if (0 == emu->uiGopSizeEqOneFlag){
			emu->uiTotalAllocateBuffer = 5;
		}
		else{
			emu->uiTotalAllocateBuffer = FRM_DEC_IDX_MAX;
		}
	}
#else
	{
		H26XDecRegCfg sTmpRegCfg;
		h26xFilePreRead(&emu->file.reg_cfg,(UINT32)sizeof(H26XDecRegCfg),(UINT32)&sTmpRegCfg);
		UINT32 pic_sum_num_lt = (sTmpRegCfg.uiH26XDec0) & 0x3f;
		UINT32 stsps = (sTmpRegCfg.uiH26XDec0>>8) & 0x7f; //range : 1~16
		if( emu->uiGopSizeEqOneFlag==0 || pic_sum_num_lt ) // svc or long-tern
			emu->uiTotalAllocateBuffer = 2 + pic_sum_num_lt + 2*(emu->uiGopSizeEqOneFlag==0);
		else
			emu->uiTotalAllocateBuffer = 1 + stsps;
	}
#endif
	// Malloc Reconstruct and Reference Buffer , need 32 bits alignment //
	for (i = 0; i < emu->uiTotalAllocateBuffer; i++) {
		h26xMemSetAddr(&pH26XVirDecAddr->uiRefAndRecYAddr[i], &uiVirMemAddr, Y_Size);
		h26xMemSetAddr(&pH26XVirDecAddr->uiRefAndRecUVAddr[i], &uiVirMemAddr, UV_Size);
		H26XDEC_CHEKMEM(pH26XVirDecAddr->uiRefAndRecYAddr[i]);
		H26XDEC_CHEKMEM(pH26XVirDecAddr->uiRefAndRecUVAddr[i]);

		h26xMemSetAddr(&emu->wp[0],&uiVirMemAddr,16);
		//DBG_INFO("HWRex[%d] Y   Limit 0x%08x - 0x%08x\r\n",i,pH26XVirDecAddr->uiRefAndRecYAddr[i],pH26XVirDecAddr->uiRefAndRecYAddr[i] + Y_Size);
		//DBG_INFO("HWRex[%d] UV  Limit 0x%08x - 0x%08x\r\n",i,pH26XVirDecAddr->uiRefAndRecUVAddr[i],pH26XVirDecAddr->uiRefAndRecUVAddr[i] + UV_Size);
	}

	for (i = 0; i < emu->uiTotalAllocateBuffer; i++)
	{
		h26xMemSetAddr(&pH26XVirDecAddr->uiColMvsAddr[i], &uiVirMemAddr, emu->ColMvs_Size);
		H26XDEC_CHEKMEM(pH26XVirDecAddr->uiColMvsAddr[i]);

		h26xMemSetAddr(&emu->wp[0],&uiVirMemAddr,16);
		//DBG_INFO("uiColMvsAddr[%d] = 0x%08x\r\n",i,pH26XVirDecAddr->uiColMvsAddr[i]);
	}

	/* Bitstream Buffer */
	pH26XVirDecAddr->uiHwBSCmdNum = 0;
	emu->uiHwBsMaxLen = H265_MAX_DEC_BS_LEN;
	h26xMemSetAddr(&pH26XVirDecAddr->uiCMDBufAddr, &uiVirMemAddr, H265_CMB_BUF_MAX_SIZE);
	h26xMemSetAddr(&pH26XVirDecAddr->uiHwBsAddr, &uiVirMemAddr, H265_MAX_DEC_BS_LEN);

	H26XDEC_CHEKMEM(pH26XVirDecAddr->uiCMDBufAddr);
	H26XDEC_CHEKMEM(pH26XVirDecAddr->uiHwBsAddr);


	if (emu->uiRndSwRst){
		h26xMemSetAddr(&pH26XVirDecAddr->uiTmpRecYAddr, &uiVirMemAddr, Y_Size);
		h26xMemSetAddr(&pH26XVirDecAddr->uiTmpRecUVAddr, &uiVirMemAddr, (Y_Size / 2));
		h26xMemSetAddr(&pH26XVirDecAddr->uiTmpColMvsAddr, &uiVirMemAddr, emu->ColMvs_Size);

		H26XDEC_CHEKMEM(pH26XVirDecAddr->uiTmpRecYAddr);
		H26XDEC_CHEKMEM(pH26XVirDecAddr->uiTmpRecUVAddr);
		H26XDEC_CHEKMEM(pH26XVirDecAddr->uiTmpColMvsAddr);

		p_job->tmp_rec_y_addr = h26x_getPhyAddr(pH26XVirDecAddr->uiTmpRecYAddr);
		p_job->tmp_rec_c_addr = h26x_getPhyAddr(pH26XVirDecAddr->uiTmpRecUVAddr);
		p_job->tmp_colmv_addr = h26x_getPhyAddr(pH26XVirDecAddr->uiTmpColMvsAddr);
	}

	if (emu->uiRndBsBuf){
		h26xMemSetAddr(&pH26XVirDecAddr->uiTmpBsAddr, &uiVirMemAddr, H265_RND_CMB_BUF_MAX_SIZE);

		DBG_INFO("uiTmpBsAddr  : 0x%08X\r\n", (int)pH26XVirDecAddr->uiTmpBsAddr);

		H26XDEC_CHEKMEM(pH26XVirDecAddr->uiTmpBsAddr);
	}
    h26xMemSetAddr(&emu->wp[0],&uiVirMemAddr,16);

	pH26XVirDecAddr->uiRef0Idx = 0;
	pH26XVirDecAddr->uiRecIdx = 1;

	pH26XVirDecAddr->uiColMvWIdx = 0;
	pH26XVirDecAddr->uiColMvRIdx = 1;

	for (i = 0; i < emu->uiTotalAllocateBuffer; i++){
		pH26XVirDecAddr->uiRecordRecIdx[i] = emu->uiTotalAllocateBuffer;
		pH26XVirDecAddr->uiKeepRecFlag[i] = 0;
	}
	pH26XVirDecAddr->uiKeepRefIdx = emu->uiTotalAllocateBuffer;
	pH26XVirDecAddr->uiRundReleaseBufCnt = 0;

	malloc_size = uiVirMemAddr - uiStartAdress;
	DBG_INFO("malloc = 0x%08x = %d K = %d M (0x%08x - 0x%08x)\r\n", (int)malloc_size, (int)malloc_size / 1000, (int)malloc_size / 1000 / 1000, (int)uiStartAdress, (int)uiVirMemAddr);

	if (malloc_size > (INT32)mem->size)
	{
		DBG_INFO("malloc = 0x%08x > size (0x%08x) error\r\n", (int)malloc_size, (int)mem->size);
		return 1;
	}

	mem->addr += SIZE_32X(malloc_size);
	mem->size -= SIZE_32X(malloc_size);

	return 0;
}

INT32 emu_h265d_init(h26x_ver_item_t *p_ver_item, h26x_job_t *p_job, UINT8 pucDir[265])
{
	h26x_mem_t *mem = &p_job->mem;
	h265_dec_ctx_t *ctx = (h265_dec_ctx_t *)p_job->ctx_addr;
	h265_dec_emu_t *emu = &ctx->emu;
	h265_dec_pat_t *pat = &ctx->pat;
	INT32 ret;

	DBG_INFO("%s %d,srcnum = (%d %d %d)\r\n", __FUNCTION__, __LINE__, (int)pat->uiSrcNum, (int)pat->uiPatNum, (int)pat->uiPicNum);

	p_job->mem.start = p_job->mem_bak.start;
	p_job->mem.addr = p_job->mem_bak.addr;
	p_job->mem.size = p_job->mem_bak.size;

    DBG_INFO("%s %d, 0x%08x, size = 0x%08x\r\n",__FUNCTION__,__LINE__,(int)p_job->mem.addr,(int)p_job->mem.size);

	memset((void*)emu, 0, sizeof(h265_dec_emu_t));
	emu->uiAPBAddr = p_job->apb_addr;
	emu->pH26XRegSet = (H26XRegSet *)emu->uiAPBAddr;
	DBG_INFO("%s %d, 0x%08x, size = 0x%08x\r\n", __FUNCTION__, __LINE__, (int)emu->pH26XRegSet, (int)p_job->mem.size);
	memset((void*)emu->pH26XRegSet, 0, sizeof(H26XRegSet));

	ctx->uiDecId = p_job->idx1;
	ctx->eCodecType = VCODEC_H265;

	memcpy((void*)emu->ucFileDir, pucDir, sizeof(emu->ucFileDir));
	memset((void*)&emu->file, 0, sizeof(emu->file));

	emu->uiRndSwRst = p_ver_item->rnd_sw_rest;
	emu->uiRndBsBuf = p_ver_item->rnd_bs_buf;

	ret = (INT32)emu_h265d_prepare_one_sequence(p_job, ctx, mem);
	if (ret == 1)
	{
		emu_h265d_close_one_sequence(p_job, ctx, mem);
	}
	return ret;
}


static BOOL emu_h265_dec_setup_one_job(h26x_ctrl_t *p_ctrl, unsigned int start_src_idx, unsigned int end_src_idx, unsigned int start_pat_idx, unsigned int end_pat_idx, unsigned int frm_num, UINT8 pucDir[265])
{
	h265_dec_ctx_t *ctx;
	h265_dec_pat_t *pat;
	h26x_job_t *job = h26x_job_add(2, p_ctrl);

	if (job == NULL)
		return FALSE;

	job->ctx_addr = h26x_mem_malloc(&job->mem, sizeof(h265_dec_ctx_t));
	memset((void*)job->ctx_addr, 0, sizeof(h265_dec_ctx_t));

	DBG_INFO("%s %d, ctx_addr = 0x%08x\r\n", __FUNCTION__, __LINE__, (int)job->ctx_addr);


	job->mem.size -= (job->mem.addr - job->mem.start);
	job->mem.start = job->mem.addr;

	job->mem_bak.start = job->mem.start;
	job->mem_bak.addr = job->mem.addr;
	job->mem_bak.size = job->mem.size;

	job->start_folder_idx = start_src_idx;
	job->end_folder_idx = end_src_idx;
	job->start_pat_idx = start_pat_idx;
	job->end_pat_idx = end_pat_idx;
	job->end_frm_num = frm_num;

	ctx = (h265_dec_ctx_t *)job->ctx_addr;
	pat = (h265_dec_pat_t *)&ctx->pat;

	pat->uiSrcNum = start_src_idx;
	pat->uiPatNum = start_pat_idx;

	pat->uiPicNum = 0;
	DBG_INFO("%s %d, mem.addr = 0x%08x\r\n", __FUNCTION__, __LINE__, (int)job->mem.addr);

	while (emu_h265d_init(&p_ctrl->ver_item, job, pucDir) == 1)
	{
		pat->uiPatNum++;
		if ((int)pat->uiPatNum >= (int)end_pat_idx)
		{
			DBG_INFO("%s %d,pat max\r\n", __FUNCTION__, __LINE__);
			pat->uiPicNum = 0;
			pat->uiPatNum = start_pat_idx;
			pat->uiSrcNum++;
		}
		if ((int)pat->uiSrcNum >= (int)end_src_idx)
		{
			DBG_INFO("%s %d,src max\r\n", __FUNCTION__, __LINE__);
#if H265D_ENDLESS_RUN
            pat->uiPicNum = 0;
            pat->uiSrcNum = start_src_idx;
            pat->uiPatNum = start_pat_idx;
#else
			job->is_finish = 1;
#endif
			DBG_INFO("is finish\r\n");
			return FALSE;
		}
	}

	DBG_INFO("%s %d\r\n", __FUNCTION__, __LINE__);
	return TRUE;
}

BOOL emu_h265d_setup(h26x_ctrl_t *p_ctrl)
{
	BOOL ret = TRUE;

	DBG_INFO("%s %d\r\n", __FUNCTION__, __LINE__);


	ret &= emu_h265_dec_setup_one_job(p_ctrl, 0, 13, 0, 101, 400,(UINT8*)"PAT_0303" );//all
	//ret &= emu_h265_dec_setup_one_job(p_ctrl, 0, 13, 0, 101, 400,(UINT8*)"PAT_0303" );//fast
	//ret &= emu_h265_dec_setup_one_job(p_ctrl, 0, 6, 0, 101, 400,(UINT8*)"PAT_0303" );//FHD
	//ret &= emu_h265_dec_setup_one_job(p_ctrl, 6, 13, 0, 31, 400,(UINT8*)"PAT_0303" );//small resolution and 4k



	return ret;
}

//#endif /* if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE) */

