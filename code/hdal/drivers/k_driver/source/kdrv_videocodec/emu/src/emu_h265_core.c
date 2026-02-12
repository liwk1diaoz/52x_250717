#include "stdio.h" //
#include "stdlib.h"
#include "string.h"
#include "emu_h26x_common.h"

#include "emu_h265_enc.h"
#include "emu_h265_tool.h"
#include "h26x_def.h"
#include "comm/hwclock.h"  // hwclock_get_counter

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

extern UINT32 uiOriRecYAddr,uiOriRecCAddr;
extern UINT32 uiOriIlfSideInfoAddr;
extern UINT32 uiOriColMvsAddr;
//UINT8 bs_reset_value = 0xAB;
extern UINT32 buf_chk_sum(UINT8 *buf, UINT32 size, UINT32 format);

static void report_perf(h26x_job_t *p_job)
{

	h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
	h265_emu_t *emu = &ctx->emu;
	h265_perf_t *perf = &emu->perf;
	h265_pat_t *pat = &ctx->pat;
	H26XEncSeqCfg *pH26XEncSeqCfg = &emu->sH26XEncSeqCfg;
	H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;

	UINT32 frm_avg_clk = (UINT32)(perf->cycle_sum/pat->uiPicNum);
	float mbs = (SIZE_64X(emu->uiWidth)/16) * (SIZE_64X(emu->uiHeight)/16);
	float mb_avg_clk = (float)frm_avg_clk/mbs;
	float  max_ratio   = (float)(perf->cycle_max_bslen*8)/perf->cycle_max;
	UINT32 bslen_avg   = perf->bslen_sum/pat->uiPicNum;
	float  bitlen_avg  = (float)(bslen_avg*8)/mbs;
	float  avg_ratio   = (float)(bslen_avg*8)/frm_avg_clk;

	DBG_DUMP("\r\ncycle report (%s, qp(%d)) => max(frm:%d) : c(%d, %.2f) b(%d, %.2f)(%.2f) avg : c(%ld, %.2f) b(%ld, %.2f)(%.2f) \r\n",
		(unsigned char*)pH26XEncSeqCfg->uiH26XInfoCfg.uiYuvName, (unsigned int)pH26XRegCfg->uiH26XPic1&0x3f, (unsigned int)perf->cycle_max_frm,
		(unsigned int)perf->cycle_max, perf->cycle_max_avg, (unsigned int)perf->cycle_max_bslen, perf->cycle_max_bitlen_avg, max_ratio,
		(unsigned long)frm_avg_clk, mb_avg_clk, (unsigned long)bslen_avg, bitlen_avg, avg_ratio);

}

#if H265_TEST_MAIN_LOOP
extern INT32 iH265encTestMainLoopCnt;
static void emu_h265_test_loopcnt(h265_emu_t *emu){
    H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
    //hk: x pos, y pos
    UINT32 uiVal,x,y;
    uiVal = 1<<31;
    //uiVal |= ((uiTestMainLoopCnt) & 0xFFFF);

	iH265encTestMainLoopCnt++;
	DBG_INFO("%s %d,iTestMainLoopCnt = %d\r\n",__FUNCTION__,__LINE__,(int)iH265encTestMainLoopCnt);

	#if 1
    x = iH265encTestMainLoopCnt % 6;  //hk : hardcode for ctb_w = 6
    y = iH265encTestMainLoopCnt / 6;
    #else
    x = 0 ;
    y = iH265encTestMainLoopCnt ;
    #endif
    uiVal |= (x & 0x1FF);
    uiVal |= (y & 0x1FF)<<16;
    DBG_INFO("%s %d uictbx = %d uictby = %d uiVal=%8x\r\n",__FUNCTION__,__LINE__,(unsigned int)x,(unsigned int)y,(unsigned int)uiVal);
    pH26XRegSet->RES_24C = uiVal;



}
#endif
static void emu_h265_init_regset(h265_emu_t *emu){
    H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    UINT32 qp_map_offset,motion_bit_offset;
    qp_map_offset = ((emu->uiWidth+63)/64) * 2;
    motion_bit_offset = (emu->uiWidth+511)/512;

    // Set Memory Address //
    pH26XRegSet->QP_MAP_ADDR       = h26x_getPhyAddr(pH26XVirEncAddr->uiUserQpAddr);
    pH26XRegSet->TNR_OUT_Y_ADDR    = h26x_getPhyAddr(pH26XVirEncAddr->uiNROutYAddr);
    pH26XRegSet->TNR_OUT_C_ADDR    = h26x_getPhyAddr(pH26XVirEncAddr->uiNROutUVAddr);
	pH26XRegSet->NAL_LEN_OUT_ADDR  = h26x_getPhyAddr(pH26XVirEncAddr->uiNalLenDumpAddr);
    pH26XRegSet->QP_MAP_LINE_OFFSET = (motion_bit_offset << 16) + qp_map_offset;
}

static void emu_h265_test_write_protect(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item){
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
	H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
	#if 0
	H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
    H26XTileSet *pTile = (H26XTileSet*)pH26XRegSet->TILE_CFG;
	#endif

	UINT32 en = 0;
    UINT32 flag = 0;
    UINT32 test_addr;
    UINT32 test_size = 0;
    UINT32 test_chn = 0;
    UINT32 cnt = 0;
    UINT32 test_item_num = 7;
    int osd = (emu->g_SrcInMode >>HEVC_EMU_INFO_OSD) & 0x1;
    flag = 0x7F;
	UINT32 i;

    if((pH26XVirEncAddr->uiRecExtraEnable & 0x2) == 0)
    {
        flag &= ~(1<<4);
    }
    if(osd == 0)
    {
        flag &= ~(1<<5);
    }

    //flag &= ~(1<<1); //XXXXXXXXXXXXX

    do{
        int choose;
        choose = rand()% (test_item_num);
        if((((en >> choose) & 1) == 0) && ((flag >> choose) & 1))
        {
            en |= (1 << choose);
            cnt++;
        }
    }while(cnt < 4);

#if 0
    en = 1<<5; //XXXXXXXXXXXXX
    en |= 1<<1; //XXXXXXXXXXXXX
    en = en & flag;
#endif



#if 0 //hk: wp test
{
	static UINT32 cn = 0;
	en = 0;
	//test_addr = pH26XVirEncAddr->uiHwBsAddr;
	//test_size = SIZE_32X(pH26XRegCfg->uiH26XBsLen);
	test_addr = pH26XVirEncAddr->uiRefAndRecUVAddr[pH26XVirEncAddr->uiRef0Idx]  ;
	test_size = emu->uiFrameSize;

	DBG_INFO("%s %d test_chn %d addr, 0x%08x, 0x%08x\r\n",__FUNCTION__,__LINE__,(int)cn%6,(int)test_addr,(int)test_size);
	h26x_test_wp(cn%6,test_addr ,test_size);
	if(p_job->idx1==1 && (pH26XRegCfg->uiH26XPic0 & 0x1) == 1)cn++;


}
#endif

	#if 0
    //DBG_INFO("%s %d en = 0x%08x, flag = 0x%08x\r\n",__FUNCTION__,__LINE__,en,flag);
    if((en >> 0) & 1){
        test_addr = 0;
		test_size = 0x1000;
        //test_size = OS_GetMemAddr(MEM_HEAP);
        DBG_INFO("%s %d stack, 0x%08x, 0x%08x\r\n",__FUNCTION__,__LINE__,(int)test_addr,(int)test_size);
        h26x_test_wp(test_chn,test_addr,test_size);
        test_chn++;
    }
    if((en >> 1) & 1){
        UINT32 s,align;
        s = pH26XRegCfg->uiH26XBsLen;
        s = (s+3)/4*4;
        test_addr = pH26XVirEncAddr->uiHwBsAddr + s;
        align = (test_addr & 0xF);
        if(align != 0) test_addr += 0x10-align;
        /*
        if(p_ver_item->rnd_bs_buf)
            test_size = emu->uiHwBsMaxLen;
        else
            test_size = pH26XRegCfg->uiH26XBsLen;
        */
        test_size = 16;
        //test_addr -= 16; //xx
        DBG_INFO("%s %d bs, 0x%08x, 0x%08x\r\n",__FUNCTION__,__LINE__,(int)test_addr,(int)test_size);
        h26x_test_wp(test_chn,test_addr,test_size);
        test_chn++;
    }
    if((en >> 2) & 1){
        //DBG_INFO("%s %d colmv addr = 0x%08x + 0x%08x\r\n",__FUNCTION__,__LINE__,
        //    pH26XVirEncAddr->uiColMvsAddr[pH26XVirEncAddr->uiColMvWIdx], emu->ColMvs_Size);
        test_addr = (pH26XVirEncAddr->uiColMvsAddr[pH26XVirEncAddr->uiColMvWIdx] + emu->ColMvs_Size);
        test_size = 16;
        //test_addr -= emu->ColMvs_Size/2; //xx
        DBG_INFO("%s %d colmv, 0x%08x, 0x%08x\r\n",__FUNCTION__,__LINE__,(int)test_addr,(int)test_size);
        h26x_test_wp(test_chn,test_addr,test_size);
        test_chn++;
    }
    if((en >> 3) & 1){
        test_addr = (pH26XVirEncAddr->uiRefAndRecUVAddr[pH26XVirEncAddr->uiRecIdx] + emu->uiFrameSize/2);
        test_size = 16;
        //test_addr -= emu->uiFrameSize/4; //xx

		//test_addr =  pH26XVirEncAddr->uiRefAndRecUVAddr[pH26XVirEncAddr->uiRecIdx] ; //xx
        //test_size = SIZE_32X(emu->uiFrameSize/2); //xx

        DBG_INFO("%s %d rec, 0x%08x, 0x%08x\r\n",__FUNCTION__,__LINE__,(int)test_addr,(int)test_size);
        h26x_test_wp(test_chn,test_addr,test_size);
        test_chn++;
    }
    if((en >> 4) & 1){
        UINT32 uiExtraRecSize  = (emu->uiHeight+63)/64*64*128;
        test_addr = (pTile->TILE_EXTRA_WR_C_ADDR[emu->uiTileNum-1]+ (uiExtraRecSize/2));
        test_size = 16;
        //test_addr -= uiExtraRecSize/4; //xx
        DBG_INFO("%s %d extrea, 0x%08x, 0x%08x\r\n",__FUNCTION__,__LINE__,(int)test_addr,(int)test_size);
        h26x_test_wp(test_chn,test_addr,test_size);
        test_chn++;
    }
    if((en >> 5) & 1){
        test_addr = emu->wp[0];
        test_size = 16;
        //test_addr -= (16+512); //xx
        DBG_INFO("%s %d osd, 0x%08x, 0x%08x\r\n",__FUNCTION__,__LINE__,(int)test_addr,(int)test_size);
        h26x_test_wp(test_chn,test_addr,test_size);
        test_chn++;
    }
    if((en >> 6) & 1){
        test_addr = emu->wp[1];
        test_size = 16;
        DBG_INFO("%s %d all buffer, 0x%08x, 0x%08x\r\n",__FUNCTION__,__LINE__,(int)test_addr,(int)test_size);
        h26x_test_wp(test_chn,test_addr,test_size);
        test_chn++;
    }
	#else

    //DBG_INFO("%s %d en = 0x%08x, flag = 0x%08x\r\n",__FUNCTION__,__LINE__,en,flag);
    if((en >> 0) & 1){
        test_addr = 0;
		test_size = 0x1000;
        //test_size = OS_GetMemAddr(MEM_HEAP);
        DBG_INFO("%s %d stack, 0x%08x, 0x%08x\r\n",__FUNCTION__,__LINE__,(int)test_addr,(int)test_size);
        h26x_test_wp(test_chn,test_addr,test_size, rand()%4);
        test_chn++;
    }

	if(emu->wpcnt>H26X_MAX_WPBUF_NUM){
		DBG_INFO("%s %d emu->wpcnt = %d (%d) out of index error!! \r\n",__FUNCTION__,__LINE__,(int)emu->wpcnt, H26X_MAX_WPBUF_NUM);
	}
	for(i = 1; i<7; i++){
		if((en >> i) & 1){
			UINT32 wpidx = rand()%emu->wpcnt;
			UINT32 test_rp = (wpidx >= emu->wprcnt) ? 0 : 1;
	      	test_addr = emu->wp[wpidx];
	        test_size = 16;
	        DBG_INFO("%s %d en %d chn %d 0x%08x(idx:%d), 0x%08x\r\n",__FUNCTION__,__LINE__,(int)i,(int)test_chn, (int)test_addr,(unsigned int)wpidx,(int)test_size);
	        h26x_test_wp(test_chn,test_addr,test_size, (test_rp == 0) ?  (rand()%2) :  (rand()%4));
	        test_chn++;
    	}

	}
	#endif
}
#define LOG2(X) ((unsigned) (8*sizeof (unsigned long long) - __builtin_clzll((X)) - 1))

static void emu_h265_gen_regset(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item){
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = (h265_pat_t *)&ctx->pat;
    H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
    //UINT32 uiHeight;
    UINT32 uiRefidx;
    UINT32 uiH26XRecLineOffset,i,uiH26XNrOutLineOffset;
    UINT32 uiHwBsLen = 0;
    UINT32 uiEc;
    H26XTileSet *pTile = (H26XTileSet*)pH26XRegSet->TILE_CFG;

	memcpy((void*)&(pH26XRegSet->FUNC_CFG[0]), pH26XRegCfg, sizeof(UINT32)* (WDATA_PATTERN_SIZE/4+1)	);
	//memcpy((void*)&(pH26XRegSet->MASK_CFG[0]), (void*)&(pH26XRegCfg->uiH26X_MASK_CFG[0]), sizeof(UINT32)* (MASK_REG_NUM)	);//MASK
	//memcpy((void*)&(pH26XRegSet->TMNR_CFG[0]), (void*)&(pH26XRegCfg->uiH26X_TMNR_CFG[0]), sizeof(UINT32)* (TMNR_REG_NUM)	);//TMNR

	//pH26XRegSet->INT_EN = (H26X_FINISH_INT | H26X_BSDMA_INT | H26X_ERR_INT | H26X_BSOUT_INT | H26X_FBC_ERR_INT);


	//hk: 528 add
	pH26XRegSet->SRAQ_CFG_17 = pH26XRegCfg->uiH26XSraq17;
	pH26XRegSet->RMD_1 = pH26XRegCfg->uiH26XRmd1;
	pH26XRegSet->JND_CFG_1 = pH26XRegCfg->uiH26XJnd1;
	pH26XRegSet->BGR_CFG[0] = pH26XRegCfg->uiH26XBgr0;
	pH26XRegSet->BGR_CFG[1] = pH26XRegCfg->uiH26XBgr1;
	pH26XRegSet->BGR_CFG[2] = pH26XRegCfg->uiH26XBgr2;
	pH26XRegSet->BGR_CFG[3] = pH26XRegCfg->uiH26XBgr3;
	pH26XRegSet->RRC_CFG_11[0] = pH26XRegCfg->uiH26XRrc11;
	pH26XRegSet->RRC_CFG_11[1] = pH26XRegCfg->uiH26XRrc12;
	pH26XRegSet->RRC_CFG_11[2] = pH26XRegCfg->uiH26XRrc13;
	pH26XRegSet->RRC_CFG_11[3] = pH26XRegCfg->uiH26XRrc14;
	pH26XRegSet->RRC_CFG_11[4] = pH26XRegCfg->uiH26XRrc15;
	pH26XRegSet->RRC_CFG_11[5] = pH26XRegCfg->uiH26XRrc16;



	if (pat->uiPicNum == 0){ //pic[0] cannot enable 3dnr
		pH26XRegCfg->uiH26XTnrCfg0 &= ~1;
	}

	pH26XRegSet->TNR_CFG[0] = pH26XRegCfg->uiH26XTnrCfg0;
	pH26XRegSet->TNR_CFG[1] = pH26XRegCfg->uiH26XTnrP2PRefCfg;
	pH26XRegSet->TNR_CFG[2] = pH26XRegCfg->uiH26XTnrP2PCfg0;
	pH26XRegSet->TNR_CFG[3] = pH26XRegCfg->uiH26XTnrP2PCfg1;
	pH26XRegSet->TNR_CFG[4] = pH26XRegCfg->uiH26XTnrP2PCfg2;
	pH26XRegSet->TNR_CFG[5] = pH26XRegCfg->uiH26XTnrP2PCfg3;
	pH26XRegSet->TNR_CFG[6] = pH26XRegCfg->uiH26XTnrP2PCfg4;
	pH26XRegSet->TNR_CFG[7] = pH26XRegCfg->uiH26XTnrMctfCfg0;
	pH26XRegSet->TNR_CFG[8] = pH26XRegCfg->uiH26XTnrMctfCfg1;
	pH26XRegSet->TNR_CFG[9] = pH26XRegCfg->uiH26XTnrMctfCfg;
	pH26XRegSet->TNR_CFG[10] = pH26XRegCfg->uiH26XTnrP2PClampCfg;
	pH26XRegSet->TNR_CFG[11] = pH26XRegCfg->uiH26XTnrBoarderCheck;
	pH26XRegSet->TNR_CFG[12] = pH26XRegCfg->uiH26XTnrMCTFClampCfg;
	pH26XRegSet->TNR_CFG[13] = pH26XRegCfg->uiH26X_TNR_MOTION_TH;
	pH26XRegSet->TNR_CFG[14] = pH26XRegCfg->uiH26X_TNR_P2P_TH;
	pH26XRegSet->TNR_CFG[15] = pH26XRegCfg->uiH26X_TNR_MCTF_TH;
	pH26XRegSet->TNR_CFG[16] = pH26XRegCfg->uiH26X_TNR_P2P_WEIGHT;
	pH26XRegSet->TNR_CFG[17] = pH26XRegCfg->uiH26X_TNR_MCTF_WEIGHT;
	pH26XRegSet->TNR_CFG[18] = pH26XRegCfg->uiH26X_TNR_P2P_CFG_6;

	//emuh26x_msg(("uiH26XTnrCfg0   = 0x%x\r\n",pH26XRegCfg->uiH26XTnrCfg0));
	//emuh26x_msg(("uiH26XTnrP2PRefCfg   = 0x%x\r\n",pH26XRegCfg->uiH26XTnrP2PRefCfg));
	//emuh26x_msg(("uiH26XTnrP2PCfg0   = 0x%x\r\n",pH26XRegCfg->uiH26XTnrP2PCfg0));
	//emuh26x_msg(("uiH26XTnrP2PCfg1   = 0x%x\r\n",pH26XRegCfg->uiH26XTnrP2PCfg1));


	for(i=0;i<26;i++){
		pH26XRegSet->LMTBL_CFG[i] = pH26XRegCfg->uiH26X_LMTBL[i];
	}

	for(i=0;i<18;i++){
		pH26XRegSet->SLMTBL_CFG[i] = pH26XRegCfg->uiH26X_SLMTBL[i];
	}


	//pPicUnit->uiSrcHeight                = uiHeight;
	pH26XEncPicCfg->uiH26XRecLineOffset  = emu->uiRecLineOffset;
    uiH26XRecLineOffset                  = pH26XEncPicCfg->uiH26XRecLineOffset / 4;
    uiH26XNrOutLineOffset                = pH26XEncPicCfg->uiNROutLineOffset /4;
    //uiH26XSrcLineOffset                  = pH26XEncPicCfg->uiH26XSrcLineOffset / 4;

    if(pH26XRegCfg->uiH26XFunc0 & (1<<13))
    {
        pH26XRegSet->REC_LINE_OFFSET     = (uiH26XRecLineOffset << 15) + (uiH26XRecLineOffset);
        //pH26XRegSet->REC_LINE_OFFSET     = (uiH26XRecLineOffset << 16) + (uiH26XRecLineOffset);
    }else{
        pH26XRegSet->REC_LINE_OFFSET     = (uiH26XRecLineOffset << 16) + (uiH26XRecLineOffset);
    }
#if 0
	uiH26XSrcLineOffset 				 = pH26XEncPicCfg->uiH26XSrcLineOffset / 4;
    if(emu->uiSrccomprsEn){
        pH26XRegSet->SRC_LINE_OFFSET         = pH26XEncPicCfg->uiH26XSrcLineOffset;
    }else{
        pH26XRegSet->SRC_LINE_OFFSET         = (uiH26XSrcLineOffset << 16) + uiH26XSrcLineOffset;
    }
#else
	pH26XRegSet->SRC_LINE_OFFSET		 = pH26XEncPicCfg->uiH26XSrcLineOffset;
#endif


    pH26XRegSet->TNR_OUT_LINE_OFFSET     = (uiH26XNrOutLineOffset << 16) + uiH26XNrOutLineOffset;

	//hk tmp hack
	//pH26XRegSet->TNR_OUT_LINE_OFFSET	 = ((emu->uiWidth/4) << 16) + (emu->uiWidth/4);


    pH26XRegSet->SIDE_INFO_LINE_OFFSET   = (pH26XEncPicCfg->uiIlfLineOffset >> 2);
	pH26XRegSet->SIDE_INFO_LINE_OFFSET  |= (pH26XEncPicCfg->uiTileIlfLineOffset[0]>>2)<<16;
    pH26XRegSet->SIDE_INFO_LINE_OFFSET2   = (pH26XEncPicCfg->uiTileIlfLineOffset[1] >> 2);
	pH26XRegSet->SIDE_INFO_LINE_OFFSET2  |= (pH26XEncPicCfg->uiTileIlfLineOffset[2]>>2)<<16;
    pH26XRegSet->SIDE_INFO_LINE_OFFSET3   = (pH26XEncPicCfg->uiTileIlfLineOffset[3] >> 2);

    //DBG_INFO("uiSrcLineOffset   = 0x%x\r\n",uiH26XSrcLineOffset*4);
    //DBG_INFO("uiRecLineOffset   = 0x%08x (%d)\r\n",(int)pH26XRegSet->REC_LINE_OFFSET,(int)uiH26XRecLineOffset);
    //DBG_INFO("uiRecLineOffset   = 0x%08x\r\n",pPicUnit->uiRecLineOffset);
    //DBG_INFO("uiSrcLineOffset   = 0x%08x\r\n",pH26XRegSet->SRC_LINE_OFFSET);
    //DBG_INFO("uiIlfLineOffset   = 0x%08x\r\n",pPicUnit->uiIlfLineOffset);

	if(pH26XRegSet->RRC_CFG[0] & 0x1){ //RC_ENABLE
		pH26XRegSet->QP_CFG[0] = (pH26XRegSet->QP_CFG[0] & 0xffffffc0)|(pH26XRegSet->RRC_CFG[1] & 0x3f);
	}

    if(emu->src_out_only == 0)
    {
	    pH26XRegSet->RRC_CFG[1] |= (0x1ff<<22);   //AE_EN
		pH26XRegSet->FUNC_CFG[0] |= (1<<4);   //AE_EN
		pH26XRegSet->FUNC_CFG[0] |= (1<<12);  //PSNR_EN,  compare psnr
		pH26XRegSet->FUNC_CFG[0] |= (1<<14);  // STATS_RW_EN
		pH26XRegSet->FUNC_CFG[0] |= (1<<15);  // DUMP_NAL_LEN_EN
    }

#if 1 //d2d
		pH26XRegSet->FUNC_CFG[0] |= ((emu->src_d2d_en & 0x1)<<20);	// SRC_D2D_EN
		pH26XRegSet->FUNC_CFG[0] |= ((emu->src_d2d_mode & 0x1)<<21);  // SRC_D2D_MODE
		pH26XRegSet->FUNC_CFG[0] |= ((emu->uiTileNum & 0x3)<<22);  // SRC_D2D_MODE

		DBG_INFO("%s %d,d2d FUNC_CFG[0] = 0x%08x, en = %d, mode = %d, tile = %d\r\n",__FUNCTION__,__LINE__, (unsigned int)pH26XRegSet->FUNC_CFG[0],
			(unsigned int)emu->src_d2d_en,(unsigned int)emu->src_d2d_mode,(unsigned int)emu->uiTileNum);
#endif

    uiEc = (pH26XRegSet->FUNC_CFG[0] >> 13) & 0x1;
    pH26XRegSet->FUNC_CFG[0] |= (uiEc<<19);
    if(pat->uiPicNum == 0)
    {
		pH26XRegSet->PIC_CFG &= ~(1 << 5); // disable colmv write
        //DBG_INFO("PIC_CFG   = 0x%08x\r\n",pH26XRegSet->PIC_CFG);
    }
	if((pH26XRegSet->FUNC_CFG[0]>>6)&1){//FRAM_SKIP_EN
		pH26XRegSet->FUNC_CFG[0] |= ((rand()%2)<<24);//FRAM_SKIP_OPT
	}
    //DBG_INFO("%s %d,FUNC_CFG[0] = 0x%08x\r\n",__FUNCTION__,__LINE__, (unsigned int)pH26XRegSet->FUNC_CFG[0]);

	pH26XRegSet->FUNC_CFG[1] &= ~(1 << 21);
	pH26XRegSet->FUNC_CFG[1] |= ((emu->bs_buf_32b)<<21);  // bs_buf_32b

    // Set Source YUV Addr //
    pH26XRegSet->SRC_Y_ADDR             = h26x_getPhyAddr(pH26XVirEncAddr->uiSrcYAddr);
    pH26XRegSet->SRC_C_ADDR             = h26x_getPhyAddr(pH26XVirEncAddr->uiSrcUVAddr);

    // Rec and Ref Buffer Addr //
    pH26XRegSet->REC_Y_ADDR             = h26x_getPhyAddr(pH26XVirEncAddr->uiRefAndRecYAddr[pH26XVirEncAddr->uiRecIdx]);
    pH26XRegSet->REC_C_ADDR             = h26x_getPhyAddr(pH26XVirEncAddr->uiRefAndRecUVAddr[pH26XVirEncAddr->uiRecIdx]);

    uiRefidx = pH26XVirEncAddr->uiRef0Idx;
    pH26XRegSet->REF_Y_ADDR             = h26x_getPhyAddr(pH26XVirEncAddr->uiRefAndRecYAddr[uiRefidx]);
    pH26XRegSet->REF_C_ADDR             = h26x_getPhyAddr(pH26XVirEncAddr->uiRefAndRecUVAddr[uiRefidx]);

    // H/W Bitstream Addr //
    for(i = 0; i < pH26XVirEncAddr->uiHwHeaderNum; i++)
    {
        uiHwBsLen += ((pH26XVirEncAddr->uiHwHeaderSize[i]+3)>>2)<<2; // word align
    }
    //YH : hack for bsdma interrupt
    pH26XVirEncAddr->uiHwHeaderSize[i] = 1;
    uiHwBsLen += 4;
    h26x_setBSDMA(pH26XVirEncAddr->uiCMDBufAddr,pH26XVirEncAddr->uiHwHeaderNum+1,
        h26x_getPhyAddr(pH26XVirEncAddr->uiHwHeaderAddr),pH26XVirEncAddr->uiHwHeaderSize);
    pH26XRegSet->BSDMA_CMD_BUF_ADDR     = h26x_getPhyAddr(pH26XVirEncAddr->uiCMDBufAddr);
    pH26XRegSet->BSOUT_BUF_ADDR         = h26x_getPhyAddr(pH26XVirEncAddr->uiHwBsAddr);
	pH26XRegSet->BSOUT_BUF_SIZE 	 = emu->uiHwBsMaxLen;
	pH26XRegSet->NAL_HDR_LEN_TOTAL_LEN  = uiHwBsLen;


	//set HEVC_REC_REF_EXTRA_ENABLE
    pTile->TILE_0_MODE = emu->uiTileNum;
    pTile->TILE_0_MODE &= 0xfffffeef;
    pTile->TILE_0_MODE |= (pH26XVirEncAddr->uiRecExtraEnable & 0x1) << 8;
    pTile->TILE_0_MODE |= (pH26XVirEncAddr->uiRecExtraEnable & 0x2) << 3;
    #if 0
    pTile->TILE_WIDTH_0 = pH26XRegCfg->uiH26XTile1;
    pTile->TILE_WIDTH_1 = pH26XRegCfg->uiH26XTile2;
    #else
    pTile->TILE_WIDTH_0 = emu->uiTileWidth[0];
    pTile->TILE_WIDTH_0 |= (emu->uiTileWidth[1])<<16;
    pTile->TILE_WIDTH_1 = emu->uiTileWidth[2];
    pTile->TILE_WIDTH_1 |= (emu->uiTileWidth[3])<<16;
    #endif
    pTile->TILE_23_RRC = pH26XRegCfg->uiH26XRrc5_2;

    pH26XRegSet->COL_MVS_WR_ADDR = h26x_getPhyAddr(pH26XVirEncAddr->uiColMvsAddr[pH26XVirEncAddr->uiColMvWIdx]);
    pH26XRegSet->COL_MVS_RD_ADDR = h26x_getPhyAddr(pH26XVirEncAddr->uiColMvsAddr[pH26XVirEncAddr->uiColMvRIdx]);

    //DBG_INFO("COL_MVS_WR_ADDR = 0x%08x 0x%08x\r\n",pH26XRegSet->COL_MVS_WR_ADDR,pH26XRegSet->COL_MVS_RD_ADDR);

    pH26XRegSet->SIDE_INFO_WR_ADDR_0 = h26x_getPhyAddr(pH26XVirEncAddr->uiIlfSideInfoAddr[pH26XVirEncAddr->uiIlfWIdx]);
    pH26XRegSet->SIDE_INFO_RD_ADDR_0 = h26x_getPhyAddr(pH26XVirEncAddr->uiIlfSideInfoAddr[pH26XVirEncAddr->uiIlfRIdx]);

#if 0
    //test sde interrupt
    memset((void*)pH26XVirEncAddr->uiIlfSideInfoAddr[pH26XVirEncAddr->uiIlfRIdx], 0xFF, 0x10);
#endif
#if 1
    if(pH26XVirEncAddr->uiRecExtraEnable & 0x1)
    {
        for(i=0;i<emu->uiTileNum;i++)
        {
            pTile->TILE_EXTRA_RD_Y_ADDR[i]      = h26x_getPhyAddr(pH26XVirEncAddr->uiRecExtraYAddr[pH26XVirEncAddr->uiRef0Idx][i]);
            pTile->TILE_EXTRA_RD_C_ADDR[i]      = h26x_getPhyAddr(pH26XVirEncAddr->uiRecExtraCAddr[pH26XVirEncAddr->uiRef0Idx][i]);
            //DBG_INFO("TILE_EXTRA_RD_Y_ADDR[%d]  = 0x%x  0x%x\r\n",i,pTile->TILE_EXTRA_RD_Y_ADDR[i],pTile->TILE_EXTRA_RD_C_ADDR[i]);
        }
        pH26XRegSet->SIDE_INFO_RD_ADDR_0        = h26x_getPhyAddr(pH26XVirEncAddr->uiIlfExtraSideInfoAddr[pH26XVirEncAddr->uiIlfRIdx]);
    }

    if (pH26XVirEncAddr->uiRecExtraEnable & 0x2){
        for(i=0;i<emu->uiTileNum;i++)
        {
            pTile->TILE_EXTRA_WR_Y_ADDR[i]       = h26x_getPhyAddr(pH26XVirEncAddr->uiRecExtraYAddr[pH26XVirEncAddr->uiRecIdx][i]);
            pTile->TILE_EXTRA_WR_C_ADDR[i]       = h26x_getPhyAddr(pH26XVirEncAddr->uiRecExtraCAddr[pH26XVirEncAddr->uiRecIdx][i]);
            //DBG_INFO("TILE_EXTRA_WR_Y_ADDR[%d]  = 0x%x  0x%x\r\n",i,pTile->TILE_EXTRA_WR_Y_ADDR[i],pTile->TILE_EXTRA_WR_C_ADDR[i]);
        }
        pH26XRegSet->SIDE_INFO_WR_ADDR_0         = h26x_getPhyAddr(pH26XVirEncAddr->uiIlfExtraSideInfoAddr[pH26XVirEncAddr->uiIlfWIdx]);
    }
#else
    for(i=0;i<emu->uiTileNum;i++)
    {
        pTile->TILE_EXTRA_RD_Y_ADDR[i]      = h26x_getPhyAddr(pH26XVirEncAddr->uiRecExtraYAddr[pH26XVirEncAddr->uiRef0Idx][i]);
        pTile->TILE_EXTRA_RD_C_ADDR[i]      = h26x_getPhyAddr(pH26XVirEncAddr->uiRecExtraCAddr[pH26XVirEncAddr->uiRef0Idx][i]);
        pTile->TILE_EXTRA_WR_Y_ADDR[i]       = h26x_getPhyAddr(pH26XVirEncAddr->uiRecExtraYAddr[pH26XVirEncAddr->uiRecIdx][i]);
        pTile->TILE_EXTRA_WR_C_ADDR[i]       = h26x_getPhyAddr(pH26XVirEncAddr->uiRecExtraCAddr[pH26XVirEncAddr->uiRecIdx][i]);
    }
    pH26XRegSet->SIDE_INFO_RD_ADDR_0        = h26x_getPhyAddr(pH26XVirEncAddr->uiIlfExtraSideInfoAddr[pH26XVirEncAddr->uiIlfRIdx]);
    pH26XRegSet->SIDE_INFO_WR_ADDR_0         = h26x_getPhyAddr(pH26XVirEncAddr->uiIlfExtraSideInfoAddr[pH26XVirEncAddr->uiIlfWIdx]);

#endif
	/*

    DBG_INFO("========================================================\r\n");
    for(i=0;i<(H26X_MAX_TILEN_NUM-1);i++)
    {
        DBG_INFO("TILE_EXTRA_RD_Y_ADDR[%d]  = 0x%08x  0x%08x\r\n",i,pTile->TILE_EXTRA_RD_Y_ADDR[i],pTile->TILE_EXTRA_RD_C_ADDR[i]);
        DBG_INFO("TILE_EXTRA_WR_Y_ADDR[%d]  = 0x%08x  0x%08x\r\n",i,pTile->TILE_EXTRA_WR_Y_ADDR[i],pTile->TILE_EXTRA_WR_C_ADDR[i]);
    }
	//*/

	RC_FRAME_TYPE curr_slice_type = pH26XRegSet->PIC_CFG & 0x3 ? RC_P_FRAME : RC_I_FRAME;
	UINT32 p_type = pH26XRegSet->PIC_CFG & 0x3 ; //0: i type, 1: p type
	UINT32 m_svcLayer = LOG2(emu->g_EmuInfo4);

	//hk: Cmodel RcInitPicture()
	if (m_svcLayer > 0){
		if (pat->uiPicNum % 4 == 1 || pat->uiPicNum % 4 == 3)
			curr_slice_type = RC_P_FRAME;
		if (pat->uiPicNum % 4 == 2){
			if (m_svcLayer == 2)
				curr_slice_type = RC_P3_FRAME;
			else
				curr_slice_type = RC_P2_FRAME;
		}
		if (pat->uiPicNum % 4 == 0 && p_type)
			curr_slice_type = RC_P2_FRAME;
	}

	H26X_SWAP(pH26XVirEncAddr->uiRCStatReadfrmAddr[curr_slice_type], pH26XVirEncAddr->uiRCStatWritefrmAddr[curr_slice_type], UINT32);
	pH26XRegSet->RRC_WR_ADDR = h26x_getPhyAddr(pH26XVirEncAddr->uiRCStatWritefrmAddr[curr_slice_type]);
	pH26XRegSet->RRC_RD_ADDR = h26x_getPhyAddr(pH26XVirEncAddr->uiRCStatReadfrmAddr[curr_slice_type]);

	if(p_type){emu->p_cnt++;}else{emu->i_cnt++;}
    UINT32 uiOri_W,uiOri_R;
    uiOri_W = pH26XRegSet->SIDE_INFO_WR_ADDR_0;
    uiOri_R = pH26XRegSet->SIDE_INFO_RD_ADDR_0;
    for(i=0;i<emu->uiTileNum;i++)
    {
        uiOri_W += pH26XEncPicCfg->uiTileSize[i];
        uiOri_R += pH26XEncPicCfg->uiTileSize[i];
        pTile->TILE_SIDE_INFO_WR_ADDR[i] = uiOri_W;
        pTile->TILE_SIDE_INFO_RD_ADDR[i] = uiOri_R;
    }
    //DBG_INFO("uiRecExtraEnable = %d, %d\r\n",pH26XRegSet->uiRecExtraEnable,pH26XVirEncAddr->uiRecExtraEnable));

	//TMNR //
	//pH26XRegSet->TMNR_CFG[52]   = h26x_getPhyAddr(pH26XVirEncAddr->uiTmnrRefAndRecYAddr [0]);//TMNR_REFR_Y_ADDR
	//pH26XRegSet->TMNR_CFG[54]   = h26x_getPhyAddr(pH26XVirEncAddr->uiTmnrRefAndRecYAddr [1]);//TMNR_REFW_Y_ADDR
	//pH26XRegSet->TMNR_CFG[53]   = h26x_getPhyAddr(pH26XVirEncAddr->uiTmnrRefAndRecCAddr [0]);//TMNR_REFR_C_ADDR
	//pH26XRegSet->TMNR_CFG[55]   = h26x_getPhyAddr(pH26XVirEncAddr->uiTmnrRefAndRecCAddr [1]);//TMNR_REFW_C_ADDR
	//pH26XRegSet->TMNR_CFG[56] 	= h26x_getPhyAddr(pH26XVirEncAddr->uiTmnrRefAndRecMotAddr [0]);//TMNR_MOTR_ADDR
	//pH26XRegSet->TMNR_CFG[57] 	= h26x_getPhyAddr(pH26XVirEncAddr->uiTmnrRefAndRecMotAddr [1]);//TMNR_MOTW_ADDR
	//pH26XRegSet->TMNR_CFG[58]   = h26x_getPhyAddr(pH26XVirEncAddr->uiTmnrInfoAddr);				//TMNR_INFO_ADDR

    for(i=0;i<3;i++)
    {
        pH26XRegSet->MOTION_BIT_ADDR[i] = h26x_getPhyAddr(pH26XVirEncAddr->uiMotionBitAddr[emu->uiMotionBitIdx[i]]);

        //DBG_INFO("pH26XRegSet->MOTION_BIT_ADDR[%d] = 0x%08x\r\n",i,pH26XRegSet->MOTION_BIT_ADDR[i]));
    }
    {
       //void tile_read_checksum(h26x_job_t *p_job);
        //tile_read_checksum(p_job);
    }

    if (p_ver_item->write_prot)
    {
        emu_h265_test_write_protect( p_job, p_ver_item);
    }

	//frame time out count, unit: 256T
	pH26XRegSet->TIMEOUT_CNT_MAX =  100 * 270 * (emu->uiWidth/16) *  (emu->uiHeight/16) /256 ;

#if 0
    h26x_setRrcLimit(pH26XRegCfg->uiH26X_520ECO & 0x1);
#else
    //if(p_job->idx1 == 1)
    {
        #if 0
        UINT32 limited_rrc_ndqp;
        limited_rrc_ndqp = (pH26XRegCfg->uiH26X_520ECO & 0x1);
        DBG_INFO("RES_1BC = 0x%08x\r\n",pH26XRegSet->RES_1BC);
        //pH26XRegSet->DBG_CFG = (pH26XRegCfg->uiH26X_520ECO & 0x1) << 7;
        pH26XRegSet->RES_1BC = (limited_rrc_ndqp << 0) | (limited_rrc_ndqp << 8) | (limited_rrc_ndqp << 16);
        DBG_INFO("RES_1BC = 0x%08x\r\n",pH26XRegSet->RES_1BC);
        #else
        pH26XRegSet->DEC1_CFG = pH26XRegCfg->uiH26X_520ECO;
        #endif
        //DBG_INFO("RES_1BC = 0x%08x  0x%08x\r\n",(int)pH26XRegSet->DEC1_CFG,(int)pH26XRegCfg->uiH26X_520ECO);
    }

#endif
    //h26x_prtMem((UINT32)pH26XRegSet,WDATA_SIZE);
#if H265_TEST_MAIN_LOOP
    emu_h265_test_loopcnt(emu);
#endif
}
///////////////////////////////////////////////////////////////////////////////////////

static void emu_h265_read_regcfg(h265_emu_t *emu)
{
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
    h26xFileRead(&emu->file.reg_cfg,(UINT32)sizeof(H26XEncRegCfg),(UINT32)pH26XRegCfg);
    //h26x_prtMem((UINT32)pH26XRegCfg,sizeof(H26XEncRegCfg));
    //DBG_INFO("uiH26X_DIST_FACTOR = 0x%08x\r\n",pH26XRegCfg->uiH26X_DIST_FACTOR));
}

static void emu_h265_read_draminfo(h265_emu_t *emu)
{
	H26XEncDramInfo *pH26XEncDramInfo = &emu->sH26XEncDramInfo;
	h26xFileRead(&emu->file.draminfo,(UINT32)sizeof(H26XEncDramInfo),(UINT32)pH26XEncDramInfo);
}

void emu_h265_read_srcyuv(h265_emu_t *emu)
{
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
	UINT32 uiWidth2 = emu->uiWidth;
	UINT32 uiHeight2 = emu->uiHeight;
    UINT32 uiSrcLineOffsetY,uiSrcLineOffsetC;
    UINT32 uiW,uiH,write_out,uiW2Y,uiW2C,uiH2Y,uiH2C;
    UINT32 swBstSizeY,swBstSizeUV;
    UINT32 uiSLoY = 0,uiSLoC = 0;
    UINT32 uiSrcLineOffset;
//#ifdef WRITE_SRC
    UINT32 uiSrc_Size = 0;
    UINT32 uiSrc_SizeC = 0;//chroma src size#endif
//#endif
    //if(emu->uiWidth==1920 && emu->uiHeight==1088)
	if(emu->uiWidth==1088)
    {
        uiWidth2 = pH26XRegCfg->uiH26X_SRC_FRAME_SIZE & 0xffff;
    }
    if(emu->uiHeight==1088)
    {
        uiHeight2 = pH26XRegCfg->uiH26X_SRC_FRAME_SIZE >> 16;
    }
    uiW = (emu->uiRotateVal == 0 || emu->uiRotateVal == 3) ? uiWidth2 : uiHeight2;
    uiH = (emu->uiRotateVal == 0 || emu->uiRotateVal == 3) ? uiHeight2 : uiWidth2;
    //uiW = (emu->uiRotateVal == 0 || emu->uiSrccomprsEn == 0) ? uiW : SIZE_32X(uiW);

    uiW2Y = SIZE_16X(uiW)*3/4;
    uiW2C = SIZE_32X(uiW)*3/4;
    uiH2Y = uiH;
    uiH2C = (uiH2Y/2);
    swBstSizeY = uiW2Y*uiH2Y;   //source compression src size y
    swBstSizeUV = uiW2C*uiH2C;  //source compression src size uv

    //DBG_INFO("uiW = %d, %d, uiW2Y = %d, %d\r\n",uiW,uiH,uiW2Y,uiH2Y);

    // decide lineoffset
    uiSrcLineOffset = uiW;
    uiSrcLineOffsetY = (uiW2Y);  // source compression lineoffset y
    uiSrcLineOffsetC = (uiW2C);  // source compression lineoffset c
    if(H26X_EMU_SRC_LINEOFFSET)
    {
        if(emu->uiRotateVal == 0 || emu->uiSrccomprsEn == 0)
        {
            uiSrcLineOffset = h26xRand()%(H26X_LINEOFFSET_MAX - SIZE_16X(uiSrcLineOffset)) + uiSrcLineOffset;
            uiSrcLineOffsetY = h26xRand()%(H26X_LINEOFFSET_MAX - SIZE_16X(uiSrcLineOffsetY)) + uiSrcLineOffsetY;
            uiSrcLineOffsetC = h26xRand()%(H26X_LINEOFFSET_MAX - SIZE_16X(uiSrcLineOffsetC)) + uiSrcLineOffsetC;
            uiSrcLineOffset = SIZE_16X(uiSrcLineOffset);
            uiSrcLineOffsetY = SIZE_16X(uiSrcLineOffsetY);
            uiSrcLineOffsetC = SIZE_16X(uiSrcLineOffsetC);
        }else{
            uiSrcLineOffset = h26xRand()%(H26X_LINEOFFSET_MAX - SIZE_32X(uiSrcLineOffset)) + uiSrcLineOffset;
            uiSrcLineOffsetY = h26xRand()%(H26X_LINEOFFSET_MAX - SIZE_32X(uiSrcLineOffsetY)) + uiSrcLineOffsetY;
            uiSrcLineOffsetC = h26xRand()%(H26X_LINEOFFSET_MAX - SIZE_32X(uiSrcLineOffsetC)) + uiSrcLineOffsetC;
            uiSrcLineOffset = SIZE_32X(uiSrcLineOffset);
            uiSrcLineOffsetY = SIZE_32X(uiSrcLineOffsetY);
            uiSrcLineOffsetC = SIZE_32X(uiSrcLineOffsetC);
        }
    }

    //DBG_INFO("uiSrccomprsLumaEn = %d uiSrccomprsChromaEn = %d, cbcr = %d\r\n",emu->uiSrccomprsLumaEn,emu->uiSrccomprsChromaEn,emu->uiSrcCbCrEn));
    //DBG_INFO("(srcmp = %d %d %d)(r = %d)(srclft = %d %d %d)\r\n",emu->uiSrccomprsLumaEn,emu->uiSrccomprsChromaEn,emu->uiSrcCbCrEn,
    //emu->uiRotateVal,uiSrcLineOffset,uiSrcLineOffsetY,uiSrcLineOffsetC);
    if((emu->uiSrccomprsLumaEn == 0) || (emu->uiSrccomprsChromaEn == 0))
    {
        write_out = (emu->uiSrccomprsLumaEn == 0);
        h26xEmuRotateSrc(&emu->file.src,(UINT8 *)emu->uiVirHwSrcYAddr,(UINT8 *)emu->uiVirEmuSrcYAddr,emu->uiSrcFrmSize,uiWidth2,uiHeight2,uiSrcLineOffset,emu->Y_Size,emu->iRotateAngle, 1, write_out, 0);
        write_out = (emu->uiSrccomprsChromaEn == 0);
        h26xEmuRotateSrc(&emu->file.src,(UINT8 *)emu->uiVirHwSrcUVAddr,(UINT8 *)emu->uiVirEmuSrcUVAddr,(emu->uiSrcFrmSize/2),uiWidth2,(uiHeight2/2),uiSrcLineOffset,emu->UV_Size,emu->iRotateAngle, 0, write_out, emu->uiSrcCbCrEn);
    }

    if(emu->uiSrccomprsLumaEn == 1)
    {
        write_out = emu->uiSrccomprsLumaEn;
        h26xEmuRotateSrc(&emu->file.srcbs_y,(UINT8 *)emu->uiVirHwSrcYAddr,(UINT8 *)emu->uiVirEmuSrcYAddr,swBstSizeY,uiW2Y,uiH2Y,uiSrcLineOffsetY,emu->Y_Size,0, 1, write_out, 0);
    }
    if(emu->uiSrccomprsChromaEn == 1)
    {
        write_out = emu->uiSrccomprsChromaEn;
        h26xEmuRotateSrc(&emu->file.srcbs_c,(UINT8 *)emu->uiVirHwSrcUVAddr,(UINT8 *)emu->uiVirEmuSrcUVAddr,swBstSizeUV,uiW2C,uiH2C,uiSrcLineOffsetC,emu->UV_Size,0, 0, write_out, 0);
    }

    if(emu->uiSrccomprsEn)
    {
        uiSLoY = (emu->uiSrccomprsLumaEn)?uiSrcLineOffsetY : uiSrcLineOffset;
        uiSLoC = (emu->uiSrccomprsChromaEn)?uiSrcLineOffsetC : uiSrcLineOffset;
        pH26XEncPicCfg->uiH26XSrcLineOffset = (( uiSLoC /4) << 16) + (uiSLoY /4);
    }else{
        //pH26XEncPicCfg->uiH26XSrcLineOffset = uiSrcLineOffset;
		pH26XEncPicCfg->uiH26XSrcLineOffset = ((uiSrcLineOffset/4) << 16) + (uiSrcLineOffset/4);
    }

/*
    pH26XRegCfg->uiH26XFunc0 |= (emu->uiSrccomprsChromaEn<<3);
    pH26XRegCfg->uiH26XFunc0 |= (emu->uiSrccomprsLumaEn<<2);
    pH26XRegCfg->uiH26XFunc0 |= (emu->uiSrcCbCrEn<<17);
	pH26XRegCfg->uiH26XFunc0 &= ~(3 << 9); //add
    pH26XRegCfg->uiH26XFunc0 |= (((emu->uiRotateVal) & 0x3)<<9);
*/

    //DBG_INFO("uiH26XPicCfg1 = 0x%08x\r\n",pH26XRegCfg->uiH26XPicCfg1);
    //DBG_INFO("uiH26XSrcLineOffset = 0x%08x\r\n",pH26XEncPicCfg->uiH26XSrcLineOffset);
    if(emu->uiSrccomprsEn)
    {
        uiSrc_Size = uiSLoY * uiH2Y;
        uiSrc_SizeC = uiSLoC * uiH2C;
    }else{
        uiSrc_Size = uiSrcLineOffset * uiH2Y;
        uiSrc_SizeC = uiSrcLineOffset * uiH2C;
    }

    emu->uiWriteSrcY = uiSrc_Size;
    emu->uiWriteSrcC = uiSrc_SizeC;
    //DBG_INFO("uiSrc_Size = %d %d\r\n",uiSrc_Size,uiSrc_SizeC);
#ifdef WRITE_SRC
    h26xFileWrite( &emu->file.hw_src, uiSrc_Size, emu->uiVirHwSrcYAddr);
    h26xFileWrite( &emu->file.hw_src, uiSrc_SizeC, emu->uiVirHwSrcUVAddr);
#endif
    // Read Source YUV //
    pH26XVirEncAddr->uiSrcYAddr  = emu->uiVirHwSrcYAddr;
    pH26XVirEncAddr->uiSrcUVAddr = emu->uiVirHwSrcUVAddr;
}

static UINT32 emu_h265_read_mbqp(h265_emu_t *emu)
{
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    UINT32  uiRet = 0;
    UINT32 uiTmp;
    uiTmp = ((emu->uiWidth+63)/64) * ((emu->uiHeight+63)/64) * 16 * 2;
    uiRet = h26xFileReadFlush(&emu->file.mbqp,uiTmp,(UINT32)pH26XVirEncAddr->uiUserQpAddr);
    if(uiRet) return uiRet;
    //H26xPrintQPMap(((uiWidth+63)/64),((uiHeight+63)/64), (UINT16 *)pH26XVirEncAddr->uiUserQpAddr);
    //h26xPrtMem((UINT32)pH26XVirEncAddr->uiUserQpAddr,uiTmp);
    return uiRet;
}

static UINT32 emu_h265_read_maq(h265_ctx_t *ctx)
{
    h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = (h265_pat_t *)&ctx->pat;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    UINT32  uiRet = 0,i;
    UINT32  uiMotionBitSize = (emu->uiWidth+511)/512 * (SIZE_64X(emu->uiHeight)) / 4;

    //DBG_INFO("%s %d, emu->uiMotionBitNum = %d\r\n",__FUNCTION__,__LINE__,emu->uiMotionBitNum);

    if(emu->uiMotionBitNum == 0)
        return uiRet;
    if (pat->uiPicNum != 0)
    {
        for( i = (emu->uiMotionBitNum-1);i>0;i--)
        {
            //DBG_INFO("%s %d, swap [%d] = %d, [%d] = %d\r\n",__FUNCTION__,__LINE__,i,emu->uiMotionBitIdx[i], i-1,emu->uiMotionBitIdx[i-1]);
            H26X_SWAP(emu->uiMotionBitIdx[i], emu->uiMotionBitIdx[i-1],UINT8);
            //DBG_INFO("%s %d, swap [%d] = %d, [%d] = %d\r\n",__FUNCTION__,__LINE__,i,emu->uiMotionBitIdx[i], i-1,emu->uiMotionBitIdx[i-1]);
        }
    }
//    DBG_INFO("%s %d, emu->uiMotionBitNum = %d\r\n",__FUNCTION__,__LINE__,emu->uiMotionBitNum);
#if 0
    for( i=0;i<3;i++)
    {
        DBG_INFO(" %d ",emu->uiMotionBitIdx[i]);
    }
    DBG_INFO("\r\n");
#endif
    uiRet = h26xFileRead(&emu->file.maq,uiMotionBitSize,(UINT32)pH26XVirEncAddr->uiMotionBitAddr[emu->uiMotionBitIdx[0]]);
	h26x_flushCache((UINT32)pH26XVirEncAddr->uiMotionBitAddr[emu->uiMotionBitIdx[0]], SIZE_32X(uiMotionBitSize));
	if(uiRet) return uiRet;
    //H26xPrintQPMap(((uiWidth+63)/64),((uiHeight+63)/64), (UINT16 *)pH26XVirEncAddr->uiUserQpAddr);
    //h26xPrtMem((UINT32)pH26XVirEncAddr->uiUserQpAddr,uiTmp);
    return uiRet;
}

static void emu_h265_read_slice_header(h265_ctx_t *ctx)
{
    h265_emu_t *emu = &ctx->emu;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
    UINT32 uiSliceSize = 0,i;

	//hk: check reg.
	if(pH26XRegCfg->uiH26X_SliceNum==0){
		DBG_INFO("uiH26X_SliceNum = 0x%08x error! \r\n",(unsigned int)pH26XRegCfg->uiH26X_SliceNum);
	}

    // Read Slice Header Len
    h26xFileRead(&emu->file.slice_header_len,pH26XRegCfg->uiH26X_SliceNum * sizeof(UINT32),(UINT32)(pH26XVirEncAddr->uiHwHeaderSize));
	//h26x_flushCache((UINT32)(pH26XVirEncAddr->uiHwHeaderSize),SIZE_32X(pH26XRegCfg->uiH26X_SliceNum * sizeof(UINT32)));

    for(i = 0; i < pH26XRegCfg->uiH26X_SliceNum; i++)
    {
        uiSliceSize += (pH26XVirEncAddr->uiHwHeaderSize[i] & 0xfffffff);
        //DBG_INFO("uiSliceSize[%d] = 0x%08x (0x%08x)\r\n",i,uiSliceSize,pH26XVirEncAddr->uiHwHeaderSize[i]);
    }
    h26xFileRead(&emu->file.slice_header,(UINT32)(uiSliceSize),(UINT32)(pH26XVirEncAddr->uiHwHeaderAddr));
	h26x_flushCache((UINT32)(pH26XVirEncAddr->uiHwHeaderAddr),SIZE_32X(uiSliceSize));
    //DBG_INFO("pH26XVirEncAddr->uiHwHeaderAddr = 0x%08x\r\n",pH26XVirEncAddr->uiHwHeaderAddr);
    //H26XPrtMem(pH26XVirEncAddr->uiHwHeaderAddr, 30);
    emu->uiSliceSize = uiSliceSize;
}



static void emu_h265_read_osd(h265_ctx_t *ctx)
{
#if H26X_EMU_OSD
    h265_emu_t *emu = &ctx->emu;
	//h265_pat_t *pat = (h265_pat_t *)&ctx->pat;
    H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    UINT32 i;
    UINT32  uiRet = 0;

    if(emu->uiOsdEn)
    {
        UINT32 test_osd_loft = 0;
        //if(pat->uiPicNum == 0)
        {
            H26XOsdSet *pOsd = (H26XOsdSet*)pH26XRegSet->OSG_CFG;
            UINT32 graph_size_one_line = 0;
            //DBG_INFO("%s %d\r\n",__FUNCTION__,__LINE__);
            h26xFileRead(&emu->file.osd,sizeof(H26XOsdSet),(UINT32)pOsd);
            //h26x_prtMem((UINT32)pOsd,sizeof(H26XOsdSet));
            for(i=0;i<32;i++)
            {
#if H26X_EMU_OSD_LINEOFFSET
                /*
                if(i == 1)
                    test_osd_loft = 1;
                else
                    test_osd_loft = 0;
                */
                test_osd_loft = rand()%2;
#endif

                //DBG_INFO("%s %d, [%d] en = %d, test_loft = %d, lofs = %d\r\n",__FUNCTION__,__LINE__,i,(pOsd->uiOsdUnit[i].uiGlobal & 0x1),test_osd_loft,(pOsd->uiOsdUnit[i].uiDisplay1 >> 19)*4);

                pOsd->uiOsdUnit[i].uiGraphic0 = h26x_getPhyAddr(pH26XVirEncAddr->uiOsdAddr[i]);
                //DBG_INFO("uiOsdUnit[%d].uiGlobal = 0x%08x\r\n", i,pOsd->uiOsdUnit[i].uiGlobal));
                //DBG_INFO("uiOsdUnit[%d].uiMask = 0x%08x\r\n", i,pOsd->uiOsdUnit[i].uiMask));
                if(pOsd->uiOsdUnit[i].uiGlobal & 0x1)
                {
                    UINT32 uiW,uiH,uiType,uiSize,bit[7] = {16,32,16,16,1,2,4};
                    UINT32 lofs;
                    uiW = pOsd->uiOsdUnit[i].uiGraphic1 & 0x1FFF;
                    uiH = (pOsd->uiOsdUnit[i].uiGraphic1>>16) & 0x1FFF;
                    uiType = (pOsd->uiOsdUnit[i].uiGlobal>>4) & 0x7;
                    lofs = pOsd->uiOsdUnit[i].uiDisplay1 >> 19;
                    //uiSize = uiW * uiH * 2 * (uiType+1);

                    uiSize = uiW;
                    graph_size_one_line = uiW;
                    if(uiType == 4){
                        uiSize = SIZE_32X(uiSize);
                        graph_size_one_line = SIZE_32X(graph_size_one_line);
                    }else if(uiType == 5){
                        uiSize = SIZE_16X(uiSize);
                        graph_size_one_line = SIZE_16X(graph_size_one_line);
                    }if(uiType == 6){
                        uiSize = SIZE_8X(uiSize);
                        graph_size_one_line = SIZE_8X(graph_size_one_line);
                    }
                    //DBG_INFO("[%2d] uiW %5d, %5d,type = %d, size = %5d, en = %d\r\n",i, uiW,uiH,uiType,uiSize,(pOsd->uiOsdUnit[i].uiGlobal & 0x1)));

                    graph_size_one_line = (graph_size_one_line * bit[uiType]) / 8;
                    if(test_osd_loft == 0)
                    {
                        //uiSize = uiSize * uiH * bit[uiType] / 8;
                        uiSize = graph_size_one_line * uiH;
                        h26xFileRead(&emu->file.osd_grp[i],uiSize,pH26XVirEncAddr->uiOsdAddr[i]);
						h26x_flushCache(pH26XVirEncAddr->uiOsdAddr[i],SIZE_32X(uiSize));

                    }else{
                        UINT32 h;
                        lofs = (((rand()%(H26X_OSD_LINEOFFSET_MAX))/4) ) + lofs ; //unit: word
                        //graph_size_one_line = SIZE_4X(graph_size_one_line);
                        pOsd->uiOsdUnit[i].uiDisplay1 = pOsd->uiOsdUnit[i].uiDisplay1 & 0x0007FFFF;
                        //pOsd->uiOsdUnit[i].uiDisplay1 |= ((uiSize/4)& 0x1FFF)<<19;
                        pOsd->uiOsdUnit[i].uiDisplay1 |= ( lofs & 0x1FFF)<<19;

                        DBG_INFO("w = %d, uiDisplay1 = %d, gsol = %d, lofs = %d\r\n",(int)uiW,(int)pOsd->uiOsdUnit[i].uiDisplay1>>19,(int)graph_size_one_line,(int)lofs*4);

                        //DBG_INFO("uiDisplay1 = %d\r\n",pOsd->uiOsdUnit[i].uiDisplay1>>19));
                        //DBG_INFO("[%d] uiW2 = %d, uiW3 = %d\r\n",i,uiW2,uiW3 ));
                        for (h = 0; h < uiH; h++){
                            uiRet = h26xFileRead(&emu->file.osd_grp[i], graph_size_one_line, pH26XVirEncAddr->uiOsdAddr[i] + (h * lofs * 4));
                            if(uiRet){
                                break;
                            }
                        }
						h26x_flushCache(pH26XVirEncAddr->uiOsdAddr[i],SIZE_32X(uiH * lofs * 4));
                    }

                    if(uiRet){
                        //DBG_INFO("error at = pOsdGrpFile[%d]\r\n", i));
                    }
                    //DBG_INFO("uiOsdAddr[%d] = 0x%08x\r\n",i,pH26XVirEncAddr->uiOsdAddr[i]));
                    //if(uiSize > 0x40) uiSize= 0x40;
                    //if(i == 0)
                    //h26x_prtMem((UINT32)pH26XVirEncAddr->uiOsdAddr[i],emu->uiOsgGraphMaxSize[i]);
                }
            }
            //for(i=0;i<16;i++)
            //    DBG_INFO("uiOsdUnit[%d].uiPALETTE = 0x%08x\r\n", i,pOsd->uiPALETTE[i]));
            pOsd->uiGCAC1 = h26x_getPhyAddr(pH26XVirEncAddr->uiOsgCacAddr);
            pOsd->uiGCAC0 = h26x_getPhyAddr(pH26XVirEncAddr->uiOsgCacAddr) + 256;
            //DBG_INFO("uiOsgCacAddr = 0x%08x\r\n",pH26XVirEncAddr->uiOsgCacAddr));
            //h26x_prtMem((UINT32)pH26XVirEncAddr->uiOsgCacAddr,512);
        }
    }else{
        H26XOsdSet *pOsd = (H26XOsdSet*)pH26XRegSet->OSG_CFG;
        //DBG_INFO("%s %d\r\n",__FUNCTION__,__LINE__));
        for(i=0;i<8;i++)
            pOsd->uiOsdUnit[i].uiGlobal = 0;
        //h26x_prtMem((UINT32)pOsd,sizeof(H26XOsdSet));
    }
#endif
}

static void emu_h265_read_tmnr(h265_ctx_t *ctx)
{
}

int ceiling(int n, int v){
    return ((n%v)>0) ? (int) ((n/v) + 1) :  (int)(n/v);
}

static void emu_h265_read_sde(h265_ctx_t *ctx)
{
    h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = (h265_pat_t *)&ctx->pat;
    H26XRegSet *pH26XRegSet = emu->pH26XRegSet;

    if(pat->uiPicNum == 0)
    {
        UINT32 uiSize = sizeof(UINT32) * 45;
        if(emu->uiSrccomprsEn){
            UINT32 *pSde = (UINT32*)pH26XRegSet->SDE_CFG;
            //DBG_INFO("%s %d\r\n",__FUNCTION__,__LINE__));
            h26xFileRead(&emu->file.sde,uiSize,(UINT32)pSde);
        }else{
            memset((void*)pH26XRegSet->SDE_CFG, 0, uiSize);
        }
    }
}


void emu_h265_test_dram_sel(void)
{
    if(h26xRand() & 1)
    {
        DBG_INFO("dram 256\r\n");
        h26x_setDramBurstLen(0);
    } else{
        DBG_INFO("dram 128\r\n");
        h26x_setDramBurstLen(1);
    }
}

void emu_h265_test_rec_lineoffset(h265_ctx_t *ctx)
{
    h265_emu_t *emu = &ctx->emu;
	//h265_pat_t *pat = (h265_pat_t *)&ctx->pat;
    //H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    //if(pat->uiPicNum == 0)
    {
        DBG_INFO("uiEcEnable = %d\r\n", (int)emu->uiEcEnable );
        if(emu->uiEcEnable != 0)
        {
            //UINT32 ori;
            //ori = (emu->uiHeight+63)/64*64; // ecls on
            //DBG_INFO("ori = %d\r\n", ori);
            if (H26X_EMU_REC_LINEOFFSET && (emu->uiHeight != H26X_LINEOFFSET_MAX))
            {
                emu->uiRecLineOffset = (((h26xRand()%(H26X_LINEOFFSET_MAX - emu->uiHeight))/16)*16) + emu->uiHeight;
            }else{
                emu->uiRecLineOffset = (emu->uiHeight+63)/64*64; // ecls on
            }
        }else{
            if (H26X_EMU_REC_LINEOFFSET && (emu->uiWidth != H26X_LINEOFFSET_MAX))
            {
                emu->uiRecLineOffset = (((h26xRand()%(H26X_LINEOFFSET_MAX - emu->uiWidth))/16)*16) + emu->uiWidth;
            }else{
                emu->uiRecLineOffset = (emu->uiWidth+63)/64*64; // ecls off
            }
        }
        emu->uiRecLineOffset = (emu->uiRecLineOffset+63)/64*64;
    }

    DBG_INFO("uiRecLineOffset   = %d\r\n",(unsigned int)emu->uiRecLineOffset);

}

static void emu_h265_test_slice_header(h265_emu_t *emu)
{
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
    UINT32 uiSliceSize = emu->uiSliceSize,i;
    UINT32 slice_hdr_chk_sum;
    UINT8 *addr = (UINT8 *)pH26XVirEncAddr->uiHwHeaderAddr;

    // get orignial check sum //
    slice_hdr_chk_sum = buf_chk_sum( addr, uiSliceSize, 1);
    pH26XRegCfg->uiH26XBsChkSum -= slice_hdr_chk_sum;
    pH26XRegCfg->uiH26XBsLen -= uiSliceSize;

    // rand generate slice header //
    uiSliceSize = 0;
    for(i = 0; i < pH26XVirEncAddr->uiHwHeaderNum; i++)
    {
        //UINT32 ran_slice_hdr_len = len_tab[i];
        //UINT32 ran_slice_hdr_len = (h26xRand()%2048 + 1);
        UINT32 ran_slice_hdr_len = (h26xRand()%128 + 1);
        //emuh26x_msg(("RAN_SLICE_HDR[%d] : %d => %d\r\n", i, pH26XVirEncAddr->uiHwHeaderSize[i], ran_slice_hdr_len));
        uiSliceSize += ran_slice_hdr_len;
		pH26XVirEncAddr->uiHwHeaderSizeTmp[i] = pH26XVirEncAddr->uiHwHeaderSize[i];//save origin slice header len
        pH26XVirEncAddr->uiHwHeaderSize[i] = ran_slice_hdr_len;
    }
    for( i = 0; i < uiSliceSize; i++)
    {
        *(addr + i) = h26xRand()%256;
    }

	#if 1
	int three_btye_test = h26xRand()%2;
	if(three_btye_test && uiSliceSize>=3){
		int ofs = (uiSliceSize-3) % 128;
		*(addr + ofs) = 0;
		*(addr + ofs + 1) = 0;
		*(addr + ofs + 2) = 3;
	}
	#endif
	//check every slice header last byte is not zero
	uiSliceSize = 0;
	for(i = 0; i < pH26XVirEncAddr->uiHwHeaderNum; i++)
    {
    	uiSliceSize += pH26XVirEncAddr->uiHwHeaderSize[i];
		if(*(addr + uiSliceSize-1) == 0){
			*(addr + uiSliceSize-1) = h26xRand()%255 + 1;
		}
	}

    h26x_flushCache(pH26XVirEncAddr->uiHwHeaderAddr, SIZE_32X(uiSliceSize));

    // calcaulate random slice header check sum //
    slice_hdr_chk_sum = buf_chk_sum( addr, uiSliceSize, 1);
    pH26XRegCfg->uiH26XBsChkSum += slice_hdr_chk_sum;
    pH26XRegCfg->uiH26XBsLen += uiSliceSize;
    emu->uiSliceSize = uiSliceSize;
}

void emu_h265_test_bsout(h265_emu_t *emu)
{

	H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    UINT32 unit = 128;
    UINT32 bs_len = pH26XEncPicCfg->uiH26XRegCfg.uiH26XBsLen;
    UINT32 nxt_size;
    UINT32 min_size = 0x100;
	//H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;


    if(emu->bs_buf_32b) unit = 32;

    nxt_size = (bs_len > min_size) ? (rand()%bs_len + 1) : bs_len;
    nxt_size += (unit - (nxt_size % unit));
    //nxt_size = 31872;
#if 1
    //nxt_size = (bs_len > 32) ? 32 : bs_len;
    //nxt_size = (bs_len > 0x1000) ? 0x1000 : bs_len;
#endif
#if 1
    emu->uiBsSegmentOffset = (h26xRand()%64) * 4;
	//emu->uiBsSegmentOffset = (h26xRand()%8) * 32;

#elif 0
    //uiBsSegmentOffset = 0x1000;
    emu->uiBsSegmentOffset = 0x0;
    nxt_size = 0x1000;
#else
	emu->uiBsSegmentOffset = 0x0;
#endif

    emu->uiBsSegmentSize = nxt_size;
    emu->uiHwBsMaxLen = nxt_size;//128;//rand()%pH26XEncPicCfg->uiH26XRegCfg.uiH26XBsLen + 1;

    {
       // UINT32 m_addr,m_size = 32;
        //DBG_INFO("memset = 0x%08x , 0x%08x\r\n",pH26XVirEncAddr->uiHwBsAddr,pH26XEncPicCfg->uiH26XRegCfg.uiH26XBsLen));
        //memset((void*)pH26XVirEncAddr->uiHwBsAddr,bs_reset_value,pH26XEncPicCfg->uiH26XRegCfg.uiH26XBsLen + 64);
        //m_addr = pH26XVirEncAddr->uiHwBsAddr + emu->uiBsSegmentSize;
        //memset((void*)m_addr,0,m_size);
        //h26x_prtMem(m_addr,m_size);
        //DBG_INFO("uiHwBsAddr = 0x%08x + 0x%08x = 0x%08x\r\n",pH26XVirEncAddr->uiHwBsAddr,emu->uiBsSegmentSize,m_addr));
    }
	DBG_INFO("uiBsSegmentSize = %d, offset = %d \r\n",(int)emu->uiBsSegmentSize,(int)emu->uiBsSegmentOffset);

}

void emu_h265_set_nxt_bsbuf(h26x_job_t *p_job)
{
	h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
	h265_emu_t *emu = &ctx->emu;
	//h265_pat_t *pat = &ctx->pat;
	//H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
	H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
	//H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
	H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
	INT32 bs_len = pH26XEncPicCfg->uiH26XRegCfg.uiH26XBsLen;
	UINT32 limit = pH26XVirEncAddr->uiHwBsAddr + H26X_MAX_BS_LEN;
	UINT32 base = pH26XVirEncAddr->uiHwBsAddr;

	UINT32 addr = dma_getCacheAddr(h26x_getVirAddr(h26x_getBsOutAddr())); //hk TODO
    UINT32 bs_out_size = h26x_getBsOutSize();

    //DBG_INFO("=======================================================\r\n"));

    emu->uiBsCurrestSize += bs_out_size;
    emu_h265_compare_bs(p_job,emu->uiBsCurrestSize);

    {
        //DBG_INFO("%s %d, 0x%08x 0x%08x\r\n",__FUNCTION__,__LINE__,addr,h26x_getBsOutSize()));
        UINT32 m_addr;
        //UINT32 m_addrm_size = 32;
        UINT8 *ptr;
        m_addr = addr + bs_out_size;
        //h26x_prtMem(m_addr-16,m_size);
        ptr = (UINT8*)m_addr;
        if(*ptr != bs_reset_value)
        {
            DBG_INFO("%s %d, 0x%08x 0x%08x, m_addr = 0x%08x, 0x%08x error!\r\n",__FUNCTION__,__LINE__,( unsigned int)addr,( unsigned int)h26x_getBsOutSize(),( unsigned int)m_addr, (int)*ptr);
        }else{
            //DBG_INFO("%s %d, 0x%08x 0x%08x, m_addr = 0x%08x, 0x%08x\r\n",__FUNCTION__,__LINE__,( unsigned int)addr,( unsigned int)h26x_getBsOutSize(),( unsigned int)m_addr,(int)*ptr);
        }
	}

	bs_len -= emu->uiHwBsMaxLen;

	if ((bs_len - emu->uiBsCurrestSize) < 0)
	{
		DBG_INFO("H26X_EMU_TEST_BSOUT res_size is zero, but get interrupt(0x1000)(bslen = 0x%08x, current = 0x%08x)\r\n",
            (int)bs_len,(int)emu->uiBsCurrestSize);
		return;
	}
    //DBG_INFO("===============\r\n"));

	h26x_clearIntStatus(0x1000);
#if 0
	UINT32 nxt_size = (bs_len > 0x100) ? (h26xRand()%bs_len + 1) : bs_len;
	nxt_size += (128 - (nxt_size % 128));

	UINT32 offset = (h26xRand()%64) * 4;
	//offset = 0;
#else
//UINT32 nxt_size = (*bs_len > uiBsSegmentSize) ? uiBsSegmentSize : *bs_len;
UINT32 nxt_size = emu->uiBsSegmentSize;
UINT32 offset  = emu->uiBsSegmentOffset;
#endif
	if ((addr + offset + nxt_size) >= limit)
	{
		DBG_INFO("limit : 0x%08x, addr = 0x%08x, offset = %d, nxt_size = 0x%08x\r\n", (int)limit, (int)addr, (int)offset, (int)nxt_size);
		addr = pH26XVirEncAddr->uiHwBsAddr;
	}
	else
		addr += (emu->uiHwBsMaxLen + offset);

	if(addr < base || addr > limit)
	{
		DBG_INFO("Error at %s %d,addr = 0x%08x, base = 0x%08x, 0x%08x\r\n",__FUNCTION__,__LINE__,(int)addr,(int)base,(int)limit);
	}else{
		//DBG_INFO("addr = 0x%08x, base = 0x%08x, 0x%08x\r\n",addr,base,limit));
		//DBG_INFO("uiHwBsMaxLen = 0x%08x, offset = 0x%08x \r\n",emu->uiHwBsMaxLen,offset));
	}

	//DBG_INFO("addr =(0x%08x, 0x%08x) addr = 0x%08x, offset = %d, nxt_size = 0x%08x (bslen = 0x%08x)\r\n",
		//(unsigned int)h26x_getBsOutAddr(), (unsigned int)emu->uiHwBsMaxLen, (unsigned int)addr, (unsigned int)offset, (unsigned int)nxt_size, (unsigned int)bs_len);

	emu->uiHwBsMaxLen = nxt_size;
    {
        //DBG_INFO("%s %d, %x\r\n",__FUNCTION__,__LINE__));
        //UINT32 m_addr,m_size = 32;
        //m_addr = addr + nxt_size;
        //memset((void*)m_addr-16,0,m_size);
        //h26x_prtMem(m_addr-16,m_size);
	}
	h26x_setNextBsBuf(h26x_getPhyAddr(addr), nxt_size);
	//DBG_INFO("H26X_EMU_TEST_BSOUT one pic done!!!\r\n"));
}

void emu_h265_set_rec_and_ref(h265_ctx_t *ctx)
{
    h265_emu_t *emu = &ctx->emu;
	//h265_pat_t *pat = (h265_pat_t *)&ctx->pat;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
#if 0 //hk tmp
    // swap rec and ref
    h26xSwapRecAndRefIdx( &pH26XVirEncAddr->uiRef0Idx, &pH26XVirEncAddr->uiRecIdx);
    h26xSwapRecAndRefIdx(&pH26XVirEncAddr->uiIlfWIdx,&pH26XVirEncAddr->uiIlfRIdx);
    h26xSwapRecAndRefIdx(&pH26XVirEncAddr->uiColMvRIdx,&pH26XVirEncAddr->uiColMvWIdx);
#else
    int rec = (pH26XRegCfg->uiH26X_DIST_FACTOR & 0x03e00000)>>21;
    int ref = (pH26XRegCfg->uiH26X_DIST_FACTOR & 0x7c000000)>>26;
    if(rec >= FRM_IDX_MAX || ref >= FRM_IDX_MAX ){ DBG_INFO("err: frame buffer num too small! ref(%d) rec(%d) frmbuf_num(%d)\r\n",(int)ref,(int)rec,FRM_IDX_MAX); }
    pH26XVirEncAddr->uiRef0Idx = pH26XVirEncAddr->uiIlfRIdx = pH26XVirEncAddr->uiColMvRIdx = ref ;
    pH26XVirEncAddr->uiRecIdx = pH26XVirEncAddr->uiIlfWIdx = pH26XVirEncAddr->uiColMvWIdx = rec ;
    //DBG_INFO("pic(%d) ref(%d) rec(%d) \r\n",pat->uiPicNum,ref,rec));
#endif

    //h26x_setSmarRecOutEn(0);
    pH26XRegCfg->uiH26XPic0 &= ~(1 << 8);

    emu->uiSmartRecEn = 0;
    if(pH26XVirEncAddr->uiRef0Idx == pH26XVirEncAddr->uiRecIdx)
    {
#if H26X_EMU_SMARTREC
        // enable smart rec
        //h26x_setSmarRecOutEn(1);
        pH26XRegCfg->uiH26XPic0 |= (1 << 8);
        emu->uiSmartRecEn = 1;
#endif
    }

    //if(pH26XRegCfg->uiH26X_TILE_MODE & 0x1 && (pH26XRegCfg->uiH26XPicCfg & 0x3 ))
    if(pH26XRegCfg->uiH26XTile0 & 0x7)
    {
        UINT32 read_extra, write_extra, recout;
        recout = pH26XRegCfg->uiH26XPic0 & 0x10;
        read_extra = (pH26XVirEncAddr->uiRecExtraStatus >> pH26XVirEncAddr->uiRef0Idx) & 0x1;
        write_extra = (pH26XVirEncAddr->uiRecExtraStatus >> pH26XVirEncAddr->uiRecIdx) & 0x1;
        if(pH26XVirEncAddr->uiRef0Idx == pH26XVirEncAddr->uiRecIdx)
        {
            write_extra = (read_extra == 0);
        }
        if(recout == 0)
        {
            write_extra = 1;
        }
        pH26XVirEncAddr->uiRecExtraEnable = (write_extra << 1) | read_extra;
        pH26XVirEncAddr->uiRecExtraStatus &= ~(1 << pH26XVirEncAddr->uiRecIdx);
        pH26XVirEncAddr->uiRecExtraStatus |= (write_extra << pH26XVirEncAddr->uiRecIdx);
        //DBG_INFO("pic(%d) ref(%d) rec(%d) st = 0x%08x, 0x%08x\r\n", pat->uiPicNum,pH26XVirEncAddr->uiRef0Idx,
        //    pH26XVirEncAddr->uiRecIdx,pH26XVirEncAddr->uiRecExtraStatus,pH26XVirEncAddr->uiRecExtraEnable));
    }else{
        pH26XVirEncAddr->uiRecExtraEnable = 0;
        pH26XVirEncAddr->uiRecExtraStatus = 0;
        //DBG_INFO("pic(%d) ref(%d) rec(%d) st = 0x%08x, 0x%08x\r\n", pat->uiPicNum,pH26XVirEncAddr->uiRef0Idx,
        //    pH26XVirEncAddr->uiRecIdx,pH26XVirEncAddr->uiRecExtraStatus,pH26XVirEncAddr->uiRecExtraEnable));
    }
}

int emu_h265_prepare_one_pic_core(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item, h26x_srcd2d_t *p_src_d2d)
{
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = (h265_pat_t *)&ctx->pat;
    //UINT8  type_name[2][64] = {"I","P"};
    H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;

    if(emu->iTestNROutEn)
    {
        #if 1
        emu->iNROutEn = h26xRand()%2;
        emu->iNROutInterleave = h26xRand()%2;
        #else //hack
        emu->iNROutEn = 0;
        emu->iNROutInterleave = 0;
        #endif
    }else{
        emu->iNROutEn = 0;
        emu->iNROutInterleave = 0;
    }

    DBG_INFO("iNROutEn= %d, %d (iTestNROutEn %d)\r\n",(int)emu->iNROutEn,(int)emu->iNROutInterleave,(int)emu->iTestNROutEn);

    emu_h265_read_regcfg(emu);
	emu_h265_read_draminfo(emu);

    if(emu->src_d2d_en == 0)
    {
		emu_h265_read_srcyuv(emu);
    }
	else{
        UINT32 uiSLoY = 0,uiSLoC = 0;
        uiSLoY = p_src_d2d->src_y_lineoffset;
        uiSLoC = p_src_d2d->src_c_lineoffset;
        pH26XEncPicCfg->uiH26XSrcLineOffset = (( uiSLoC /4) << 16) + (uiSLoY /4);
        pH26XVirEncAddr->uiSrcYAddr      = p_src_d2d->src_y_addr;
        pH26XVirEncAddr->uiSrcUVAddr     = p_src_d2d->src_c_addr;
        {
            //emuh26x_msg(("reset source buffer (addr = 0x%08x, size = 0x%08x )\r\n",pH26XVirEncAddr->uiSrcYAddr,emu->uiSrcFrmSize));
            //memset((void*)pH26XVirEncAddr->uiSrcYAddr,0x5A,emu->uiSrcFrmSize);
            //memset((void*)pH26XVirEncAddr->uiSrcUVAddr,0x5A,emu->uiSrcFrmSize/2);
        }
        if(emu->uiTileNum > 2)
        {
            DBG_INFO("error! src d2d mode , not support tile = %d\r\n",(unsigned int)emu->uiTileNum);
            return H26X_EMU_FILEERR;
        }
    }

    pH26XRegCfg->uiH26XFunc0 |= (emu->uiSrccomprsChromaEn<<3);
    pH26XRegCfg->uiH26XFunc0 |= (emu->uiSrccomprsLumaEn<<2);
    pH26XRegCfg->uiH26XFunc0 |= (emu->uiSrcCbCrEn<<17);
	pH26XRegCfg->uiH26XFunc0 &= ~(3 << 9); //add
    pH26XRegCfg->uiH26XFunc0 |= (((emu->uiRotateVal) & 0x3)<<9);

    if(emu->src_out_only == 0) emu_h265_read_mbqp(emu);
    if(emu->src_out_only == 0) emu_h265_read_slice_header(ctx);

    if(pat->uiPicNum==0) //hk
		emu_h265_read_osd(ctx);

	emu_h265_read_tmnr(ctx);

    if(emu->src_out_only)
    {
        UINT32 uiCnt;
        UINT32 uiH26XRnd0 = pH26XRegCfg->uiH26XRnd0;
        UINT32 uiH26XRnd1 = pH26XRegCfg->uiH26XRnd1;

        uiCnt = (UINT32)&pH26XRegCfg->uiH26XMaq1 - (UINT32)&pH26XRegCfg->uiH26XPic1;
        DBG_INFO("uiCnt = 0x%08x (0x%08x - 0x%08x)\r\n",(unsigned int)uiCnt,(unsigned int)&pH26XRegCfg->uiH26XMaq1,(unsigned int)&pH26XRegCfg->uiH26XPic1);
        memset((void*)&pH26XRegCfg->uiH26XPic1,0,uiCnt);
        pH26XRegCfg->uiH26XRnd0 = uiH26XRnd0;
        pH26XRegCfg->uiH26XRnd1 = uiH26XRnd1;
        emu->iNROutEn = 1;
        pH26XRegCfg->uiH26XPic0 &= ~(1<<0); // hack to I-slice
        pH26XRegCfg->uiH26XPic0 &= ~(1<<4); // disable rec out
        pH26XRegCfg->uiH26XPic0 &= ~(1<<5); // disable colmv out
        //pH26XRegCfg->uiH26XPic0 &= ~(1<<6); // disable sideinfo out

        DBG_INFO("%s %d,uiH26XPic0 = 0x%08x\r\n",__FUNCTION__,__LINE__,(unsigned int)pH26XRegCfg->uiH26XPic0);

        pH26XRegCfg->uiH26XFunc0 &= ~(1<<4); // disable ae
        pH26XRegCfg->uiH26XFunc0 &= ~(1<<14); // disable rc write
        pH26XRegCfg->uiH26XFunc0 &= ~(1<<15); // disable nal write
        pH26XRegCfg->uiH26XFunc0 &= ~(1<<7); // disable fro
        pH26XRegCfg->uiH26XFunc0 &= ~(1<<8); // disable mbqp

        DBG_INFO("%s %d,uiH26XFunc0 = 0x%08x\r\n",__FUNCTION__,__LINE__,(unsigned int)pH26XRegCfg->uiH26XFunc0);

        pH26XRegCfg->uiH26XRrc0 &= ~(1<<0); // disable rrc
        pH26XRegCfg->uiH26XTile0 &= ~(1<<0); // disable tile

        //pH26XRegCfg->uiH26XFunc1 = 0;
        pH26XRegCfg->uiH26XFunc1 &= (1<<31);

    }

	emu->uiMotionBitNum = (pH26XRegCfg->uiH26XMaq0 >> 6) & 0x3;
    if((pH26XRegCfg->uiH26XMaq0 & 0x3) != 0)
    {
        emu_h265_read_maq(ctx);
    }

    if(emu->src_d2d_en == 0)
    {
        emu_h265_read_sde(ctx);
    }
	else{
        int i;
        if(p_src_d2d->src_cmp_luma_en || p_src_d2d->src_cmp_crma_en){
            for(i=0;i<45;i++){
                pH26XRegSet->SDE_CFG[i] = p_src_d2d->sdec[i];
            }
        }else{
            UINT32 uiSize = sizeof(UINT32) * 45;
            memset((void*)pH26XRegSet->SDE_CFG, 0, uiSize);
        }
    }

    emu_h265_read_bs(emu);
    emu_h265_read_nal(emu);

    emu->uiEcEnable = (pH26XRegCfg->uiH26XFunc0 >> 13) & 1;

    if(emu->iNROutEn)
    {
        pH26XRegCfg->uiH26XPic0 |= 1 << 7;
        if(emu->iNROutInterleave)
            pH26XRegCfg->uiH26XFunc0 |= 1 << 16;
        else
            pH26XRegCfg->uiH26XFunc0 &= ~(1<<16);
    }else{
        pH26XRegCfg->uiH26XPic0 &= ~(1<<7);//TNR OUT ENABLE
        pH26XRegCfg->uiH26XFunc0 &= ~(1<<16);
    }

	#if 0  // RATE_CONTROL_RATE_MODE
	pH26XRegCfg->uiH26XFunc1 |= (1 << 19);
	DBG_INFO("RATE_CONTROL_RATE_MODE enable\r\n"));
	#endif

    pH26XVirEncAddr->uiHwHeaderNum = pH26XRegCfg->uiH26X_SliceNum;
    if( pH26XVirEncAddr->uiHwHeaderNum > H26X_MAX_BSCMD_NUM)
    {
        DBG_INFO("Err at enc, uiHwBSCmdNum = %d > %d\r\n", (int)pH26XVirEncAddr->uiHwHeaderNum, H26X_MAX_BSCMD_NUM);
    }

    // Set Initialize Register //
    if (pat->uiPicNum == 0){
        pH26XRegSet->SEQ_CFG[1] = pH26XRegCfg->uiH26XFrameSize;
        //if(emu->uiRotateEn == 1 && (emu->iRotateAngle == 90 ||emu->iRotateAngle == -90) && H26X_EMU_SRC_CMPRS){
        if(emu->uiRotateEn == 1 && (emu->iRotateAngle == 90 ||emu->iRotateAngle == -90) && emu->uiSrccomprsEn){
            pH26XRegSet->SEQ_CFG[0] = pH26XRegCfg->uiH26XFrameSize;
        }
        else{
            pH26XRegSet->SEQ_CFG[0] = ((pH26XRegSet->SEQ_CFG[1]>>16)==1088)?((pH26XRegSet->SEQ_CFG[1]&0xffff)|(1080<<16)):(pH26XRegSet->SEQ_CFG[1]);
            pH26XRegSet->SEQ_CFG[0] = ((pH26XRegSet->SEQ_CFG[1]&0xffff)==1088)?((pH26XRegSet->SEQ_CFG[1]&0xffff0000)|(1080)):(pH26XRegSet->SEQ_CFG[1]);
        }

        // gen Memory Buffer(Ref and Temprol Buffer) Register //
        emu_h265_init_regset(emu);

        pH26XEncPicCfg->uiIlfLineOffset = emu->uiIlfLineOffset;
        pH26XEncPicCfg->uiNROutLineOffset = emu->uiNrOurOffset;

        if(emu->uiTileNum)
        {
            UINT32 uiUseIlfSize,i,sum = 0,sum2 = 0;

            pH26XEncPicCfg->uiIlfLineOffset = ((emu->uiTileWidth[0]+63)/64*8+31)/32*32;
            sum = emu->uiTileWidth[0];
            sum2 = pH26XEncPicCfg->uiIlfLineOffset;
            pH26XEncPicCfg->uiTileSize[0] = pH26XEncPicCfg->uiIlfLineOffset * ((emu->uiHeight+63)/64);


            //for(i=0;i<emu->uiTileNum - 1;i++)
            for(i=0;i<emu->uiTileNum;i++)
            {
                sum += emu->uiTileWidth[i+1];
                pH26XEncPicCfg->uiTileIlfLineOffset[i] = ((emu->uiTileWidth[i+1]+63)/64*8+31)/32*32;
                sum2 += pH26XEncPicCfg->uiTileIlfLineOffset[i];
                pH26XEncPicCfg->uiTileSize[i+1] = pH26XEncPicCfg->uiTileIlfLineOffset[i] * ((emu->uiHeight+63)/64);
            	//emuh26x_msg(("uiTileIlfLineOffset = %d,tilesize = 0x%08x\r\n", (int)pH26XEncPicCfg->uiTileIlfLineOffset[0], (int)pH26XEncPicCfg->uiTileSize[1]));
            }
            /*
            pH26XEncPicCfg->uiTileIlfLineOffset[i] = ((emu->uiWidth - sum+63)/64*8+31)/32*32;
            emu_msg(("i = %d , uiTileIlfLineOffset = %d = %d - %d\r\n",
                i,pH26XEncPicCfg->uiTileIlfLineOffset[i],emu->uiWidth,sum));
            sum2 += pH26XEncPicCfg->uiTileIlfLineOffset[i];
            pH26XEncPicCfg->uiTileSize[i+1] = pH26XEncPicCfg->uiTileIlfLineOffset[i] * ((emu->uiHeight+63)/64);

            emu_msg(("LineOffset = (%d , %d, %d, %d)\r\n",pH26XEncPicCfg->uiIlfLineOffset, pH26XEncPicCfg->uiTileIlfLineOffset[0],
               pH26XEncPicCfg->uiTileIlfLineOffset[1],pH26XEncPicCfg->uiTileIlfLineOffset[2]));
            */
            uiUseIlfSize = (sum2)*((emu->uiHeight+63)/64);
            //emu_msg(("uiUseIlfSize = %d\r\n",uiUseIlfSize));
            if(uiUseIlfSize > emu->IlfSideInfo_Mall_Size)
            {
                DBG_INFO("Error!!! uiUseIlfSize 0x%08x > 0x%08x\r\n",(int)uiUseIlfSize,(int)emu->IlfSideInfo_Mall_Size);
            }
        }
        pH26XVirEncAddr->uiRecExtraEnable = 0;
//#if H26X_EMU_DRAM_SEL
//        emu_h265_test_dram_sel();
//#endif
    }
    // Get Picture Info //
    emu->uiPicType = pH26XRegCfg->uiH26XPic0 & 0x1;



    //DBG_INFO("[%s]\r\n",type_name[uiPicType]));
    emu_h265_set_rec_and_ref(ctx);
	if(p_ver_item->rnd_bs_buf_32b)
	{
        emu->bs_buf_32b = rand()%2;
	}else{
		emu->bs_buf_32b = 0;
	}
    emu->stable_bs_len = 0;
    {
        extern int stablelen_idx;
        stablelen_idx = 0;

    }

    emu->uiBsCurrestSize = 0;
    if (H26X_EMU_TEST_RAN_SLICE){
        emu_h265_test_slice_header(emu);
    }

    if (H26X_EMU_TEST_BSOUT)
    {
		UINT32 total_bs_buf_size = 0;

		emu_h265_test_bsout(emu);

		total_bs_buf_size = SIZE_32X(  pH26XEncPicCfg->uiH26XRegCfg.uiH26XBsLen + (pH26XEncPicCfg->uiH26XRegCfg.uiH26XBsLen +  (emu->uiBsSegmentSize - 1)) /emu->uiBsSegmentSize * 	emu->uiBsSegmentOffset) ;

		memset((void*)pH26XVirEncAddr->uiHwBsAddr,bs_reset_value,total_bs_buf_size + 64);
		h26x_flushCache(pH26XVirEncAddr->uiHwBsAddr, SIZE_32X(total_bs_buf_size + 64));


    }else{
        emu->uiBsSegmentSize = H26X_MAX_BS_LEN;
        emu->uiBsSegmentOffset = 0;
		//emu->uiHwBsMaxLen = H26X_MAX_BS_LEN;

		memset((void*)pH26XVirEncAddr->uiHwBsAddr,bs_reset_value,pH26XEncPicCfg->uiH26XRegCfg.uiH26XBsLen + 64);
		h26x_flushCache(pH26XVirEncAddr->uiHwBsAddr, SIZE_32X(pH26XEncPicCfg->uiH26XRegCfg.uiH26XBsLen + 64));

    }

#if H26X_EMU_SCD
    pH26XEncPicCfg->uiH26XRegCfg.uiH26XScene0 |= (1<<20);    //scene change stop encode bit
#endif

    emu_h265_gen_regset(p_job,p_ver_item);
#if 0
    DBG_INFO("job(%d) report\r\n", p_job->idx));
    DBG_INFO("report : %p\r\n", (p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 4)));
    h26x_prtMem((UINT32)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 4), sizeof(UINT32)*H26X_REPORT_NUMBER);
#endif
    if(0)
    {
        emu->y_chk_sum[1] = buf_chk_sum((UINT8 *)emu->uiVirHwSrcYAddr, emu->uiWriteSrcY, 1);
        emu->c_chk_sum[1] = buf_chk_sum((UINT8 *)emu->uiVirHwSrcUVAddr, emu->uiWriteSrcC, 1);
    }


    return H26X_EMU_PASS;
}

int emu_h265_prepare_one_pic(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item, h26x_srcd2d_t *p_src_d2d)
{
    int end_src,start_pat,end_pat,max_fn;
    h26x_mem_t *mem = &p_job->mem;
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = (h265_pat_t *)&ctx->pat;
    H26XEncSeqCfg *pH26XEncSeqCfg = &emu->sH26XEncSeqCfg;

#if (H265_AUTO_JOB_SEL || H265_ENDLESS_RUN)
	int start_src;
    start_src = p_job->start_folder_idx;
#endif

    start_pat = p_job->start_pat_idx;
    end_src = p_job->end_folder_idx;
    end_pat = p_job->end_pat_idx;
    max_fn = (pH26XEncSeqCfg->uiH26XInfoCfg.uiFrameNum < p_job->end_frm_num)? pH26XEncSeqCfg->uiH26XInfoCfg.uiFrameNum:p_job->end_frm_num;

    //DBG_INFO("%s %d, pic = %d, framenum = %d\r\n",__FUNCTION__,__LINE__,pat->uiPicNum,max_fn));
    if(p_job->is_finish == 1)
    {
        return 1;
    }

    if( (int)pat->uiPicNum >= max_fn)
    {
        int ret;
        UINT8 pucDir[265];
        DBG_INFO("%s %d,pic max\r\n",__FUNCTION__,__LINE__);
		report_perf(p_job);
        emu_h265_close_one_sequence( p_job, ctx, mem);
        pat->uiPicNum = 0;
        #if H265_AUTO_JOB_SEL
        do{
            int diff_src = end_src - start_src;
            int diff_pat = end_pat - start_pat;
            pat->uiPicNum = 0;
            pat->uiSrcNum = (h26xRand() % diff_src) + start_src;
            pat->uiPatNum = (h26xRand() % diff_pat) + start_pat;

			DBG_INFO("%s %d,uiSrcNum = %d %d(%d %d %d %d)\r\n",__FUNCTION__,__LINE__,(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)diff_src,(int)diff_pat,(int)start_src,(int)start_pat);


            memcpy((void*)pucDir,emu->ucFileDir,sizeof(emu->ucFileDir));

			ret = emu_h265_init(p_job,pucDir,p_src_d2d);

        }while(ret == 1);
        #else
        do{
            pat->uiPatNum++;
            if((int)pat->uiPatNum >= end_pat)
            {
                DBG_INFO("%s %d,pat max\r\n",__FUNCTION__,__LINE__);
                pat->uiPicNum = 0;
                pat->uiPatNum = start_pat;
                pat->uiSrcNum++;
            }
            if((int)pat->uiSrcNum >= end_src)
            {
                DBG_INFO("%s %d,src max\r\n",__FUNCTION__,__LINE__);
#if H265_ENDLESS_RUN
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
            memcpy((void*)pucDir,emu->ucFileDir,sizeof(emu->ucFileDir));

			ret = emu_h265_init(p_job,pucDir,p_src_d2d);

        }while(ret == 1);
        #endif
    }

    emu_h265_prepare_one_pic_core(p_job,p_ver_item, p_src_d2d);

    return 0;
}



//#endif //if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)


