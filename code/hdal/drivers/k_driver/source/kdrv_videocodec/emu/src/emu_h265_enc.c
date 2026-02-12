#include "stdio.h" //
#include "stdlib.h"
#include "string.h"
#include "emu_h26x_common.h"
//#include "SysCfg.h"

#include "emu_h265_enc.h"
#include "emu_h265_tool.h"
#include "comm/hwclock.h"  // hwclock_get_counter

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#if 1
#define H26XENC_CHEKMEM(x) do{if(x==0){DBG_ERR("Error!!! Mallocate Fail! %x\r\n",(int)x);} }while(0)
#else
#define H26XENC_CHEKMEM(x) do{if(x==0){DBG_ERR("Error!!! Mallocate Fail! %x\r\n",(int)x);} }while(0);\
{\
        \
        malloc_size = uiVirMemAddr - uiStartAdress;\
        DBG_INFO("[%s][%d]malloc = 0x%08x = %d K = %d M (0x%08x - 0x%08x)\r\n",__FUNCTION__,__LINE__,(int)malloc_size,(int)(malloc_size/1000),(int)(malloc_size/1000/1000),(int)uiStartAdress,(int)uiVirMemAddr);\
        if(malloc_size > (INT32)mem->size)\
        {\
            DBG_INFO("[%s][%d] malloc = 0x%08x > size (0x%08x) error\r\n",__FUNCTION__,__LINE__,(int)malloc_size,(int)mem->size);\
            return 1;\
        }\
 }
#endif

INT32    iH265encTestMainLoopCnt = -1;
int skip_pattern(h265_ctx_t *ctx, h26x_job_t *p_job)
{
    //h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = &ctx->pat;
    int ret = 0;

    if(pat->uiSrcNum == 34 && pat->uiPatNum == 1)
    {
        return 1;
    }

    if(p_job->idx1 == 0)
    {
        /*
        if(pat->uiSrcNum < 36)
        {
            ret = 1;
        }
        if(pat->uiSrcNum == 36 && pat->uiPatNum < 3)
        {
            ret = 1;
        }
        //*/
    }else{
        /*
        if(pat->uiSrcNum < 38)
        {
            ret = 1;
        }
        if(pat->uiSrcNum == 38 && pat->uiPatNum < 4)
        {
            ret = 1;
        }
        //*/
    }
    return ret;
}

UINT32 emu_h265_close_one_sequence(h26x_job_t *p_job, h265_ctx_t *ctx, h26x_mem_t *p_mem)
{
    h265_emu_t *emu = &ctx->emu;
	//h265_pat_t *pat = &ctx->pat;

    if(emu->uiSrccomprsEn)
    {
        h26xFileClose(&emu->file.srcbs_y);
        h26xFileClose(&emu->file.srcbs_c);
    }
    h26xFileClose(&emu->file.src);
    h26xFileClose(&emu->file.reg_cfg);
#if EMU_CMP_BS
    h26xFileClose(&emu->file.bs);
#endif

#if EMU_CMP_SIDEINFO
    h26xFileClose(&emu->file.ilf_sideinfo);
#endif

#if EMU_CMP_REC
    h26xFileClose(&emu->file.rec_y);
    h26xFileClose(&emu->file.rec_c);
    h26xFileClose(&emu->file.rec_y_len);
    h26xFileClose(&emu->file.rec_c_len);
#endif

    h26xFileClose(&emu->file.slice_header_len);
    h26xFileClose(&emu->file.slice_header);
#if EMU_CMP_NAL
    h26xFileClose(&emu->file.nal_len);
#endif

    h26xFileClose(&emu->file.mbqp);
    h26xFileClose(&emu->file.maq);

#if EMU_CMP_NROUT
    h26xFileClose(&emu->file.tnr_out);
#endif

	h26xFileClose(&emu->file.colmv);
	h26xFileClose(&emu->file.draminfo);

#ifdef WRITE_BS
    h26xFileClose(&emu->file.hw_bs);
#endif
#ifdef WRITE_REC
    h26xFileClose(&emu->file.hw_rec);
#endif
#ifdef WRITE_TNR
    h26xFileClose(&emu->file.hw_tnr);
#endif
#ifdef WRITE_SRC
    h26xFileClose(&emu->file.hw_src);
#endif

#if H26X_EMU_OSD
    UINT32  i;
    h26xFileClose(&emu->file.osd);

    for(i=0;i<32;i++)
    {
        h26xFileClose(&emu->file.osd_grp[i]);
    }
#endif
    //h26xFileDebug();

    return 0;
}

static UINT32 emu_h265_open_info(h265_emu_t *emu, UINT8 *pucFpgaDir[256])
{
    H26XEncSeqCfg *pH26XEncSeqCfg = &emu->sH26XEncSeqCfg;
    H26XEncInfoCfg *pH26XInfoCfg = &pH26XEncSeqCfg->uiH26XInfoCfg;
    UINT8   *pucFileName[256] ;
    UINT32 uiRet = 0;

    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir,"info_data.dat");


    h26xFileOpen(&emu->file.info_cfg,(char *)pucFileName);
    uiRet = h26xFileRead(&emu->file.info_cfg,sizeof(H26XEncInfoCfg),(UINT32)pH26XInfoCfg);
    if(uiRet) {
		DBG_INFO("%s %d,read patterns fail !!!\r\n",__FUNCTION__,__LINE__);
		return uiRet;
	}


    emu->uiWidth = pH26XInfoCfg->uiWidth;
    emu->uiHeight = pH26XInfoCfg->uiHeight;
    uiRet = h26xFileRead(&emu->file.info_cfg,pH26XInfoCfg->uiCommandLen*sizeof(char),(UINT32)emu->g_main_argv);
    if(uiRet) return uiRet;

    DBG_INFO("%s %d,uiFrameNum = %d\r\n",__FUNCTION__,__LINE__,(int)pH26XInfoCfg->uiFrameNum);
    memset(&emu->g_main_argv[pH26XInfoCfg->uiCommandLen],0,sizeof(emu->g_main_argv)-pH26XInfoCfg->uiCommandLen);
    h26xFileRead(&emu->file.info_cfg,sizeof(UINT32),(UINT32)&emu->g_SrcInMode);
    h26xFileRead(&emu->file.info_cfg,sizeof(UINT32),(UINT32)&emu->g_EmuInfo0);
    h26xFileRead(&emu->file.info_cfg,sizeof(UINT32),(UINT32)&emu->g_EmuInfo1);
    h26xFileRead(&emu->file.info_cfg,sizeof(UINT32),(UINT32)&emu->g_EmuInfo2);
    h26xFileRead(&emu->file.info_cfg,sizeof(UINT32),(UINT32)&emu->g_EmuInfo3);
    h26xFileRead(&emu->file.info_cfg,sizeof(UINT32),(UINT32)&emu->g_EmuInfo4);
    h26xFileRead(&emu->file.info_cfg,sizeof(UINT32),(UINT32)&emu->g_EmuInfo5);
    h26xFileRead(&emu->file.info_cfg,sizeof(UINT32),(UINT32)&emu->g_EmuInfo6);
    DBG_INFO("%s %d,g_SrcInMode = 0x%08x\r\n",__FUNCTION__,__LINE__,(int)emu->g_SrcInMode);
    DBG_INFO("%s %d,g_EmuInfo0 = 0x%08x\r\n",__FUNCTION__,__LINE__,(int)emu->g_EmuInfo0);
    DBG_INFO("%s %d,g_EmuInfo1 = 0x%08x\r\n",__FUNCTION__,__LINE__,(int)emu->g_EmuInfo1);
    DBG_INFO("%s %d,g_EmuInfo2 = 0x%08x\r\n",__FUNCTION__,__LINE__,(int)emu->g_EmuInfo2);
	/*
    DBG_INFO("srcmp = %d,osd = %d, tile = %d, ec = %d, svc = %d, mask = %d, tmnr = %d\r\n",(emu->g_SrcInMode >>HEVC_EMU_INFO_SRCMP) & 0x1,
        (emu->g_SrcInMode >>HEVC_EMU_INFO_OSD) & 0x1,(emu->g_SrcInMode >>HEVC_EMU_INFO_TILE) & 0x1,
        (emu->g_SrcInMode >>HEVC_EMU_INFO_ECLS) & 0x1,(emu->g_SrcInMode >>HEVC_EMU_INFO_SVC) & 0x1,
        (emu->g_SrcInMode >>HEVC_EMU_INFO_MASK) & 0x1,(emu->g_SrcInMode >>HEVC_EMU_INFO_TMNR) & 0x1
        ));
	*/
    //emu->uiSrcCbCrEn = (emu->g_SrcInMode >>11) & 0x1;
    //DBG_INFO("uiSrcCbCrEn = %d\r\n",emu->uiSrcCbCrEn));
    h26xFileClose(&emu->file.info_cfg);
    //h26xPrintString(g_main_argv,(INT32)uiCommandLen);
    return uiRet;
}

static void emu_h265_open_src(h26x_job_t *p_job, UINT8 *pucFpgaDir[256])
{
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = &ctx->pat;
    UINT8   isX64 = 0,is16X = 0;
    UINT8  *pucSrcName[256],*pucFileName[256];
    static int ang[4] = {0,90,-90,180};
    // OpenFile //
    if(is16X)
    {
        sprintf((char *)pucSrcName,"A:\\%s\\enc_pattern\\%s\\%s\\enc_fpga_pattern\\E_%d\\src_yuv.yuv",(char *)emu->ucFileDir,gucH26XEncTestSrc_Cut[pat->uiSrcNum][0],gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],(int)pat->uiPatNum);
    }else if(isX64)
    {
        sprintf((char *)pucSrcName,"A:\\%s\\src_x64\\%s\\%s\\src_yuv.yuv",(char *)emu->ucFileDir,gucH26XEncTestSrc_Cut[pat->uiSrcNum][0],gucH26XEncTestSrc_Cut[pat->uiSrcNum][1]);
    }else{
        sprintf((char *)pucSrcName,"A:\\%s\\src_x16\\%s\\%s\\src_yuv.yuv",(char *)emu->ucFileDir,gucH26XEncTestSrc_Cut[pat->uiSrcNum][0],gucH26XEncTestSrc_Cut[pat->uiSrcNum][1]);
    }

    if(emu->uiSrccomprsEn){
        static int ang[4] = {0,90,-90,180};
        H26XEncRegCfg sTmpRegCfg;
        h26xFilePreRead(&emu->file.reg_cfg,(UINT32)sizeof(H26XEncRegCfg),(UINT32)&sTmpRegCfg);
        emu->uiRotateVal = ( sTmpRegCfg.uiH26XFunc0 & 0x600 ) >> 9;
        emu->iRotateAngle = ang[emu->uiRotateVal];
        emu->uiRotateEn = (emu->iRotateAngle != 0);
        //DBG_INFO("uiRotateVal = %d, %d\r\n", emu->uiRotateVal,emu->iRotateAngle));
        #if 0
        sprintf((char *)pucFileName,"A:\\%s\\srcbs\\%s\\%s\\r%d_uv%d.vp",emu->ucFileDir,gucH26XEncTestSrc_Cut[pat->uiSrcNum][0],gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],emu->uiRotateVal,emu->uiSrcCbCrEn);
        h26xFileOpen(&emu->file.srcbs,(char *)pucFileName);
        DBG_INFO("ptotBinFile = %s\r\n", pucFileName));
        #else
        sprintf((char *)pucFileName,"A:\\%s\\srcbs\\%s\\%s\\r%d_uv%d_y.vp",(char *)emu->ucFileDir,gucH26XEncTestSrc_Cut[pat->uiSrcNum][0],gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],(int)emu->uiRotateVal,(int)emu->uiSrcCbCrEn);
        h26xFileOpen(&emu->file.srcbs_y,(char *)pucFileName);
        DBG_INFO("ptotBinFile = %s\r\n", (char *)pucFileName);
        sprintf((char *)pucFileName,"A:\\%s\\srcbs\\%s\\%s\\r%d_uv%d_c.vp",(char *)emu->ucFileDir,gucH26XEncTestSrc_Cut[pat->uiSrcNum][0],gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],(int)emu->uiRotateVal,(int)emu->uiSrcCbCrEn);
        h26xFileOpen(&emu->file.srcbs_c,(char *)pucFileName);
        DBG_INFO("ptotBinFile = %s\r\n", (char *)pucFileName);
        #endif
        if(is16X != 1)
        {
        //sprintf((char *)pucSrcName,"A:\\%s\\src\\%s\\%s\\r%d_uv0.yuv",(char *)emu->ucFileDir,gucH26XEncTestSrc_Cut[pat->uiSrcNum][0],gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],(int)emu->uiRotateVal);
        //if(pat->uiPatNum != 0)
        sprintf((char *)pucSrcName,"A:\\%s\\src\\%s\\%s\\r%d.yuv",(char *)emu->ucFileDir,gucH26XEncTestSrc_Cut[pat->uiSrcNum][0],gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],(int)emu->uiRotateVal);
        }
    }else{
        #if H26X_EMU_ROTATE
        //emu->uiRotateVal = h26xRand() % 4;
        //emu->uiRotateVal = (h26xRand() % 4)?0:(h26xRand() % 4);
        //emu->uiRotateVal = (rt_idx/3) % 4;
        //emu->uiRotateVal = p_job->idx;
        //emu->uiRotateVal = pp_rotate;
        //emu->uiRotateVal = 1;
        emu->uiRotateVal = (emu->g_SrcInMode >> HEVC_EMU_INFO_ROTATE) & 0x3;
        #else
        emu->uiRotateVal = 0;
        #endif
        emu->iRotateAngle = ang[emu->uiRotateVal];
        emu->uiRotateEn = (emu->iRotateAngle != 0);
    }

    DBG_INFO("pH26XSrcFile = %s\r\n", (char *)pucSrcName);
    DBG_INFO("iRotateAngle = %d\r\n", (int)emu->iRotateAngle);
    h26xFileOpen(&emu->file.src,(char *)pucSrcName);
}
static void emu_h265_open_enc_pat(h26x_job_t *p_job, UINT8 *pucFpgaDir[256])
{
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    UINT8   *pucFileName[256] ;
#if defined(WRITE_REC) || defined(WRITE_BS)  || defined(WRITE_SRC)  || defined(WRITE_TNR)
	h265_pat_t *pat = &ctx->pat;
#endif
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "reg_data.dat");
    h26xFileOpen(&emu->file.reg_cfg,(char *)pucFileName);
#if EMU_CMP_BS
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "es_data.dat");
    h26xFileOpen(&emu->file.bs,(char *)pucFileName);
#endif
#if EMU_CMP_SIDEINFO
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "dram_side_info.txt");
    h26xFileOpen(&emu->file.ilf_sideinfo,(char *)pucFileName);
#endif
#if EMU_CMP_REC
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "dram_luma.txt");
    h26xFileOpen(&emu->file.rec_y,(char *)pucFileName);
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "dram_chroma.txt");
    h26xFileOpen(&emu->file.rec_c,(char *)pucFileName);
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "Blk_enc_length_luma.txt");
    h26xFileOpen(&emu->file.rec_y_len,(char *)pucFileName);
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "Blk_enc_length_chroma.txt");
    h26xFileOpen(&emu->file.rec_c_len,(char *)pucFileName);
#endif
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "slice_header_len.dat");
    h26xFileOpen(&emu->file.slice_header_len,(char *)pucFileName);
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "dump_slice_header.es");
    h26xFileOpen(&emu->file.slice_header,(char *)pucFileName);
#if EMU_CMP_NAL
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "dump_nal_len.txt");
    h26xFileOpen(&emu->file.nal_len,(char *)pucFileName);
#endif
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "mbqp_data.dat");
    h26xFileOpen(&emu->file.mbqp,(char *)pucFileName);
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "motion_bit.dat");
    h26xFileOpen(&emu->file.maq,(char *)pucFileName);
#if EMU_CMP_NROUT
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "tnr_yuv.dat");
    h26xFileOpen(&emu->file.tnr_out,(char *)pucFileName);
#endif

	sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "colmv_data.dat");
	h26xFileOpen(&emu->file.colmv,(char *)pucFileName);

	sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "dram_info.dat");
	h26xFileOpen(&emu->file.draminfo,(char *)pucFileName);


#ifdef WRITE_BS
    {
        static int id = 0;
        //sprintf((char *)pucFileName,"%s","hwbs.dat");
        sprintf((char *)pucFileName,"hw_bs_%s_E%d_%d.dat",gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],(int)pat->uiPatNum,id++);
        h26xFileOpen(&emu->file.hw_bs,(char *)pucFileName);
    }
#endif
#ifdef WRITE_REC
    {
        static int id = 0;
        sprintf((char *)pucFileName,"A:\\hw_rec_%s_E%d_%d.dat",gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],(int)pat->uiPatNum,id++);
        //sprintf((char *)pucFileName,"hw_rec_%s_E%d_%d.dat",gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],pat->uiPatNum,id++);
        h26xFileOpen(&emu->file.hw_rec,(char *)pucFileName);
    }
#endif
#ifdef WRITE_TNR
    {
        static int tnrfileid = 0;
        sprintf((char *)pucFileName,"A:\\hw_tnrout_%s_E%d_%d.dat",gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],(int)pat->uiPatNum,tnrfileid++);
        //sprintf((char *)pucFileName,"hw_tnrout_%s_E%d_%d.dat",gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],pat->uiPatNum,tnrfileid++);
        h26xFileOpen(&emu->file.hw_tnr,(char *)pucFileName);
        DBG_INFO("pH26XHwTnrFile = %s\r\n", (char *)pucFileName);
    }
#endif
#ifdef WRITE_SRC
    {
        static int id = 0;
        //sprintf((char *)pucFileName,"A:\\hw_src_%s_E%d_%d.dat",gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],pat->uiPatNum,id++);
        sprintf((char *)pucFileName,"hw_src_%s_E%d_%d.dat",gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],(int)pat->uiPatNum,id++);
        h26xFileOpen(&emu->file.hw_src,(char *)pucFileName);
     }
#endif

}

int osd_lpm = 0;
static void emu_h265_open_osd(h265_emu_t *emu, UINT8 *pucFpgaDir[256])
{
#if H26X_EMU_OSD
    UINT8  *pucOsdDir[256];
    UINT8   *pucFileName[256] ;
    UINT32  uiRet = 0,i;
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "osd_data.dat");
    h26xFileOpen(&emu->file.osd,(char *)pucFileName);
    //DBG_INFO("pucFileName = %s\r\n", (char *)pucFileName);
    uiRet = h26xFilePreRead(&emu->file.osd,(UINT32)sizeof(i),(UINT32)&i);
    DBG_INFO("uiRet = %d\r\n", (int)uiRet);
    if(uiRet == 0){
        emu->uiOsdEn = 1;
        osd_lpm = 0;
        for(i=0;i<32;i++)
        {
            sprintf((char *)pucOsdDir,"%s\\osg_graph%d_img.vec",(char *)pucFpgaDir,(int)i);
            h26xFileOpen(&emu->file.osd_grp[i],(char *)pucOsdDir);
        }
        {
            H26XOsdSet sO;
            H26XOsdSet *pOsd = (H26XOsdSet*)&sO;
            //DBG_INFO("%s %d\r\n",__FUNCTION__,__LINE__));
            h26xFilePreRead(&emu->file.osd,sizeof(H26XOsdSet),(UINT32)pOsd);
            for(i=0;i<32;i++)
            {
                UINT32 uiW,uiH,uiType,uiSize,bit[7] = {16,32,16,16,1,2,4};
                uiW = pOsd->uiOsdUnit[i].uiGraphic1 & 0x1FFF;
                uiH = (pOsd->uiOsdUnit[i].uiGraphic1>>16) & 0x1FFF;
                uiType = (pOsd->uiOsdUnit[i].uiGlobal>>4) & 0x7;
                osd_lpm |= (pOsd->uiOsdUnit[i].uiGlobal>>20) & 0x1;
                //uiSize = uiW * uiH * 2 * (uiType+1);
                //uiSize = uiW * uiH * bit[uiType] / 8;
                uiSize = uiW;
                if(uiType == 4){
                    uiSize = SIZE_32X(uiSize);
                }else if(uiType == 5){
                    uiSize = SIZE_16X(uiSize);
                }if(uiType == 6){
                    uiSize = SIZE_8X(uiSize);
                }
                uiSize = uiSize * bit[uiType] / 8;
#if H26X_EMU_OSD_LINEOFFSET
                //uiSize += H26X_OSD_LINEOFFSET_MAX * 16;
                uiSize += H26X_OSD_LINEOFFSET_MAX;
#endif
                //DBG_INFO("uiW = %d\n",uiSize));
                //uiSize = uiSize * uiH * bit[uiType] / 8;
                uiSize = uiSize * uiH;
                emu->uiOsgGraphMaxSize[i] = uiSize;
                //DBG_INFO("[%d] uiW %d,%d,%d, size = 0x%08x\r\n",i, uiW,uiH,uiType,uiSize));
            }
        }

    }
#endif
    DBG_INFO("uiOsdEn = %d\r\n", (int)emu->uiOsdEn);

}
static void emu_h265_open_tmnr(h265_emu_t *emu, UINT8 *pucFpgaDir[256])
{
}
static void emu_h265_open_sde(h265_emu_t *emu, UINT8 *pucFpgaDir[256])
{
    UINT8   *pucFileName[256] ;
    sprintf((char *)pucFileName,"%s\\%s",(char *)pucFpgaDir, "sde_data.dat");
    h26xFileOpen(&emu->file.sde,(char *)pucFileName);
    //DBG_INFO("pucFileName = %s\r\n", (char *)pucFileName);
}

UINT32 emu_h265_prepare_one_sequence(h26x_job_t *p_job, h265_ctx_t *ctx, h26x_mem_t *p_mem, h26x_srcd2d_t *p_src_d2d)
{
    UINT32 i,j;
    UINT32 UserQp_Size/*,uiTmp*/;
    INT32 malloc_size = 0, uiStartAdress;
    UINT32 RC_SIZE;
    UINT32 uiRet = 0;
    UINT32 uiVirMemAddr;
    UINT32 uiExtraRecSize = 0;
#if H26X_EMU_OSD
    UINT32 uiOsgCacSize = 256 * 2;
    //UINT32 uiOsgGraphMaxSize = (360*360*4*2);
#endif
    UINT32 Y_Size;
    UINT32 UV_Size;
    UINT32 uiWidth;
    UINT32 uiHeight;
    h26x_mem_t *mem = p_mem;
    h265_emu_t *emu = &ctx->emu;
    h265_pat_t *pat = &ctx->pat;
    UINT32 uiSrcNum = pat->uiSrcNum;
    UINT32 uiPatNum = pat->uiPatNum;
    H26XEncSeqCfg *pH26XEncSeqCfg = &emu->sH26XEncSeqCfg;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    H26XEncInfoCfg *pH26XInfoCfg = &pH26XEncSeqCfg->uiH26XInfoCfg;
    UINT8  *pucFileDir[256],*pucFpgaDir[256];
    UINT32  uiRefBufInfo = 0xFFFFFFFF;
	UINT32  uiSvcInfo = 0;
    UINT32  uiMotionBitSize;

    if(skip_pattern(ctx,p_job))
    {
        DBG_DUMP("%s %d,skip pattern(src = %d, pat = %d)\r\n",__FUNCTION__,__LINE__,(int)pat->uiSrcNum,(int)pat->uiPatNum);
        return 1;
    }
	{
		h265_perf_t *perf = &emu->perf;
		perf->cycle_sum = 0;
		perf->cycle_max = 0;
		perf->cycle_max_avg = 0.0;
		perf->cycle_max_frm = 0;
		perf->cycle_max_bslen = 0;
		perf->cycle_max_bitlen_avg= 0.0;
		perf->bslen_sum = 0;
	}

    DBG_DUMP("\r\n\r\n###########################################################\r\n");
    uiVirMemAddr = mem->addr;
    uiStartAdress = uiVirMemAddr;

    DBG_INFO("uiVirMemAddr = 0x%08x\n",(int)uiVirMemAddr);
    sprintf((char *)pucFileDir,"A:\\%s\\enc_pattern\\%s\\%s",(char *)emu->ucFileDir,gucH26XEncTestSrc_Cut[uiSrcNum][0],gucH26XEncTestSrc_Cut[uiSrcNum][1]);
	sprintf((char *)pucFpgaDir,"%s\\enc_fpga_pattern\\E_%d",(char *)pucFileDir,(int)uiPatNum);

    DBG_DUMP("ucFileDir = %s\r\n",(char*)pucFileDir);
    DBG_DUMP("pucFpgaDir = %s\r\n",(char*)pucFpgaDir);

    uiRet = emu_h265_open_info( emu, pucFpgaDir);
    if(uiRet) return uiRet;

#if H26X_EMU_NROUT_ONLY
	emu->src_out_only = 1;
#else
	emu->src_out_only = 0;
#endif

    uiWidth = emu->uiWidth;
    uiHeight = emu->uiHeight;
    //DBG_INFO("Current enc test = [ %s ] [ E_%d ] (%d ,%d)\r\n", gucH26XEncTestSrc_Cut[uiSrcNum][1],uiPatNum,uiSrcNum,uiPatNum));
    DBG_DUMP("Current enc test = [ %s ][ %s ] [ E_%d ] (%d ,%d)\r\n", (char*)emu->ucFileDir,gucH26XEncTestSrc_Cut[uiSrcNum][1],(int)uiPatNum,(int)uiSrcNum,(int)uiPatNum);
    DBG_DUMP("%d x %d, fn = %d, arg_len = %d\r\n", (int)uiWidth, (int)uiHeight,(int)pH26XInfoCfg->uiFrameNum,(int)pH26XInfoCfg->uiCommandLen);
    UINT32 uiSeed = 34567*uiSrcNum + uiPatNum;
    h26xSrand(uiSeed);
	DBG_DUMP("Seed = %d\n",(unsigned int)uiSeed);

#if 1 //d2d
	emu->src_d2d_en = p_src_d2d->src_d2d_en;
	emu->src_d2d_mode = p_src_d2d->src_d2d_mode;
    DBG_DUMP("src_d2d_en   = %d\r\n",(unsigned int)p_src_d2d->src_d2d_en);
    DBG_DUMP("src_d2d_mode  = %d\r\n",(unsigned int)p_src_d2d->src_d2d_mode);
    DBG_DUMP("src_cmp_luma_en = %d\r\n",(unsigned int)p_src_d2d->src_cmp_luma_en);
    DBG_DUMP("src_cmp_crma_en = %d\r\n",(unsigned int)p_src_d2d->src_cmp_crma_en);
    DBG_DUMP("src_rotate = %d\r\n",(unsigned int)p_src_d2d->src_rotate);
    DBG_DUMP("src_y_addr = 0x%08x\r\n",(unsigned int)p_src_d2d->src_y_addr);
    DBG_DUMP("src_c_addr = 0x%08x\r\n",(unsigned int)p_src_d2d->src_c_addr);
    DBG_DUMP("y_lineoffset = %d\r\n",(unsigned int)p_src_d2d->src_y_lineoffset);
    DBG_DUMP("c_lineoffset = %d\r\n",(unsigned int)p_src_d2d->src_c_lineoffset);
#endif

    #if 1//hk?? (H26X_EMU_MAX_BUF == 0)
        uiRefBufInfo = emu->g_EmuInfo1;
    #endif

	//uiSvcInfo = LOG2(emu->g_EmuInfo4);
	//DBG_DUMP("emu->g_EmuInfo4 = %d uiSvcInfo = %d \r\n",(unsigned int)emu->g_EmuInfo4, (unsigned int)uiSvcInfo);
	uiSvcInfo = (emu->g_EmuInfo4 > 1);

    //emu->uiSrccomprsEn = 1;
    emu->uiSrccomprsEn = (emu->g_SrcInMode >>HEVC_EMU_INFO_SRCMP) & 0x1;

    #if 0
    int ary[4][2] = {{1,1},{0,0},{1,0},{0,1}};
    emu->uiSrccomprsLumaEn = ary[p_job->idx][0];
    emu->uiSrccomprsChromaEn = ary[p_job->idx][1];
    //emu->uiSrcCbCrEn = 1;
    #elif 1
    emu->uiSrccomprsLumaEn = h26xRand()%2;
    emu->uiSrccomprsChromaEn = h26xRand()%2;
	emu->uiSrcCbCrEn = h26xRand()%2;
    #else
    emu->uiSrccomprsLumaEn = 0;
    emu->uiSrccomprsChromaEn = 0;
    emu->uiSrcCbCrEn = 0;
    #endif
    if(emu->uiSrccomprsEn== 0)
    {
        emu->uiSrccomprsLumaEn = 0;
        emu->uiSrccomprsChromaEn = 0;
        emu->uiSrcCbCrEn = h26xRand()%2;
    }
    DBG_INFO("uiSrccomprsLumaEn = %d uiSrccomprsChromaEn = %d, cbcr = %d\r\n",(int)emu->uiSrccomprsLumaEn,(int)emu->uiSrccomprsChromaEn,(int)emu->uiSrcCbCrEn);

    emu_h265_open_enc_pat(p_job, pucFpgaDir);

#if H26X_EMU_NROUT_ONLY
    if (emu->src_out_only == 1)
    {
        int tile  = (emu->g_SrcInMode >>HEVC_EMU_INFO_TILE) & 0x1;
        if (tile == 1) {
            DBG_INFO("XXXXXXXXXXXXXXXXXXXXX source out only, not support now (tile=%d)\r\n",tile);
            return 1;
        }
    }
#endif

    {
        H26XEncRegCfg sTmpRegCfg;
        h26xFilePreRead(&emu->file.reg_cfg,(UINT32)sizeof(H26XEncRegCfg),(UINT32)&sTmpRegCfg);
        emu->uiTileNum = (sTmpRegCfg.uiH26XTile0 & 0x7);
        emu->uiTileWidth[0] = (sTmpRegCfg.uiH26XTile1 >> 0) & 0xFFFF;
        emu->uiTileWidth[1] = (sTmpRegCfg.uiH26XTile1 >> 16) & 0xFFFF;
        emu->uiTileWidth[2] = (sTmpRegCfg.uiH26XTile2 >> 0) & 0xFFFF;
        emu->uiTileWidth[3] = (sTmpRegCfg.uiH26XTile2 >> 16) & 0xFFFF;
        if(emu->uiTileNum != 0){
            UINT32 uiSum = 0;
            for(i=0;i<emu->uiTileNum;i++)
                uiSum += emu->uiTileWidth[i];
            emu->uiTileWidth[emu->uiTileNum] = uiWidth - uiSum;
        }
        DBG_INFO("uiTileNum = %d (%d, %d, %d, %d, %d)\r\n",(int)emu->uiTileNum,(int)emu->uiTileWidth[0],
            (int)emu->uiTileWidth[1],(int)emu->uiTileWidth[2],(int)emu->uiTileWidth[3],(int)emu->uiTileWidth[4]);

        emu->uiEcEnable = (sTmpRegCfg.uiH26XFunc0 >> 13) & 1;

    }

#if 1 //d2d
	if(emu->src_d2d_en){
    //if(0){
        //H26XEncRegCfg sTmpRegCfg;
        //int ec;
        //int osd = (emu->g_SrcInMode >>HEVC_EMU_INFO_OSD) & 0x1;
        //int grey;
        int tile;
        //int last_tilewidth;
        //int tilewidth1,tilewidth2;
        //int roi,aq,mbqp,fro;
        //int var_ro_i_delta;
        //int var_ro_p_delta;
        //int mbqp;
        //int t;
        //int maq;
        //int jnd;
        //int svc;
        //int tmvp;
        //h26xFilePreRead(&emu->file.reg_cfg,(UINT32)sizeof(H26XEncRegCfg),(UINT32)&sTmpRegCfg);
        //int tmnr = (emu->g_SrcInMode >>HEVC_EMU_INFO_TMNR) & 0x1;
        //ec = (sTmpRegCfg.uiH26XFunc0 >> 13) & 1;
        //grey = (sTmpRegCfg.uiH26XFunc0 >> 11) & 1;
        //tmvp = (sTmpRegCfg.uiH26XFunc0 >> 11) & 1;
        //svc = (emu->g_SrcInMode >>HEVC_EMU_INFO_SVC) & 0x1;
        //tile = sTmpRegCfg.uiH26XTile0 & 0x7;

		tile = emu->uiTileNum;
        DBG_INFO("(tile = %d at stripe = %d)\r\n",(int)tile,(unsigned int)p_src_d2d->src_d2d_mode);

        //last_tilewidth = emu->uiTileWidth[emu->uiTileNum];
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
        //t = (fro && ((var_ro_i_delta != 0) || (var_ro_p_delta != 0)));
        //DBG_INFO("(roi = %d, aq = %d, mbqp = %d, fro = %d, var_i = %d, var_p = %d) t = %d\r\n",roi,aq,mbqp,fro,var_ro_i_delta,var_ro_p_delta,t));
        //if(fro && (var_ro_i_delta == 0) && (var_ro_p_delta == 0) && (roi == 0) && (aq == 0) && (mbqp == 0) )
        //{
        //    int c;
        //    for(c= 0; c < 10;c++)
        //    {
        //        DBG_INFO("ooooooooooooooooooooooooooooooooooooooooooo\r\n"));
        //    }
        //}
        //DBG_INFO(" (tile = %d, %d, %d), grey = %d\r\n",tile,tilewidth1,tilewidth2,grey));

        //DBG_INFO("(ec = %d, grey = %d,tmvp = %d, svc = %d\r\n",ec,grey,tmvp,svc));
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
        //if(osd == 1)
        //{
        //    emu_h265_open_osd(emu, pucFpgaDir);
        //}
        //DBG_INFO("(osd = %d, osd_lpm  %d, tile = %d\r\n",osd,osd_lpm,tile));
        //DBG_INFO("(tile = %d, osd = %d, svc = %d)\r\n",tile,osd,svc));


 		if((p_src_d2d->src_d2d_mode < 2 && tile > 1) ||
           (p_src_d2d->src_d2d_mode == 2 && tile != 2) )


        //if((osd == 1 && mbqp) || ((uiWidth > 4096)))
        //if((osd == 1 && osd_lpm) || (tile == 4))
        //if(tile == 4 || tile == 0 || osd == 1)
        //if(tile == 4 || osd == 1)
        //if(tile == 4 || uiWidth > 4096)
        //if(tile == 4 || (uiWidth > 4096) || osd == 0 || (tile > 0 && last_tilewidth < 448)) // 0116 ov
        //if(tile =`= 4 || (uiWidth > 4096) || (tile > 0 && last_tilewidth < 448)) // 0116 ov
        //if(osd == 1 || (tile > 0))
        //if(((osd == 0) && (tile == 0)) || (tile == 4) || (svc == 1))
        //if(osd == 1)
        {
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now (osd = %d, mbqp = %d)(width = %d, maq = %d)\r\n",osd,mbqp,uiWidth,maq));
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now (tile = %d)\r\n",tile));

			DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now (tile = %d at stripe = %d)\r\n",(int)tile,(unsigned int)p_src_d2d->src_d2d_mode);
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now (uiWidth = %d)\r\n",uiWidth));
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now (osd = %d)\r\n",osd);
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now (svc = %d)\r\n",svc));
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (osd = %d, osd_lpm  %d, tile = %d\r\n",osd,osd_lpm,tile));
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (ec = %d, grey = %d\r\n",ec,grey));
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (tmvp = %d, svc = %d\r\n",tmvp,svc));
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (jnd = %d\r\n",jnd));
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (maq = %d, tile = %d)\r\n",maq,tile));
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now (tile = %d, %d, %d)\r\n",tile,tilewidth1,tilewidth2));
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now (tile = %d, %d, %d)\r\n",tile,tilewidth1,tilewidth2));
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (ec = %d, tile = %d)\r\n",ec,tile));
            //DBG_INFO("XXXXXXXXXXXXXXXXXXXXX not support now on V7 (roi = %d, aq = %d, mbqp = %d, fro = %d, var_i = %d, var_p = %d) t = %d\r\n",roi,aq,mbqp,fro,var_ro_i_delta,var_ro_p_delta,t));
            //emu_h265_close_one_sequence( p_job, ctx, mem);
            return 1;
        }
    }
#endif

    if(emu->src_d2d_en == 0)
    {
        emu_h265_open_src( p_job, pucFpgaDir );
    }
	else{
        static int ang[4] = {0,90,-90,180};
        emu->uiSrccomprsLumaEn = p_src_d2d->src_cmp_luma_en;
        emu->uiSrccomprsChromaEn = p_src_d2d->src_cmp_crma_en;
        emu->uiSrcCbCrEn = 0;
        emu->uiSrccomprsEn = (emu->uiSrccomprsLumaEn | emu->uiSrccomprsChromaEn);
        DBG_INFO("src_cmp_luma_en = %d %d %d\r\n",(unsigned int)emu->uiSrccomprsLumaEn,(unsigned int)emu->uiSrccomprsChromaEn,(unsigned int)emu->uiSrccomprsEn);
        emu->uiRotateVal = p_src_d2d->src_rotate;
        emu->iRotateAngle = ang[emu->uiRotateVal];
    }

    emu_h265_open_osd(emu, pucFpgaDir);
	emu_h265_open_tmnr(emu, pucFpgaDir);
	emu_h265_open_sde(emu, pucFpgaDir);
    emu_h265_test_rec_lineoffset(ctx);

    emu->uiSrcFrmSize = uiWidth * uiHeight;
    emu->uiFrameSize  = ((uiWidth+63)>>6) * ((uiHeight+63)>>6) * 64 * 64;
    emu->uiIlfLineOffset_cmodel = ((uiWidth+63)/64) * 8;
    emu->uiIlfLineOffset = ((uiWidth+63)/64) * 8;
    emu->uiIlfLineOffset = SIZE_32X(emu->uiIlfLineOffset);
    emu->uiNrOurOffset = ((uiWidth+63)/64)*64;
    emu->IlfSideInfo_Mall_Size = (emu->uiIlfLineOffset+ (64*8)) * ((uiHeight+63)/64); // tile mode maybe need more size (dependent on 32X(tile width) )
    emu->ColMvs_Size     = ((uiWidth+63) >> 6) * ((uiHeight+63) >> 6) * (16 * 4) * 4;
	UINT32 lcunum =  ((uiWidth+63)>>6)	  *  ((uiHeight+63)>>6) ;
	emu->uiTmnrMotSize =  lcunum * 32 ;
	if((lcunum%2)==1)
		emu->uiTmnrMotSize += 32;
	emu->uiTmnrInfoSize = (((uiWidth+63)>>6))*(((uiHeight+63)>>6)) * 256 * 8;//set lcux_step = lcuy_step = mbx_step = mby_step = 1, 		Each LCU gets 64 sampling of mb_cnt = 0, 		1 samping have 64bit
    uiMotionBitSize = (uiWidth+511)/512 * (SIZE_64X(uiHeight)) / 4;
    DBG_DUMP("uiMotionBitSize         = 0x%08x\r\n", (int)uiMotionBitSize);
    DBG_INFO("IlfSideInfo_Mall_Size   = 0x%08x\r\n", (int)emu->IlfSideInfo_Mall_Size);

	UserQp_Size     = ((uiWidth+63)/64) * ((uiHeight+63)/64) * 16 * 2;


    if(emu->uiEcEnable){
        uiExtraRecSize  = (emu->uiRecLineOffset+63)/64*64*128;
    }else{
        uiExtraRecSize  = (uiHeight+63)/64*64*128;
    }

#if (H26X_EMU_SRC_LINEOFFSET || H26X_EMU_REC_LINEOFFSET)
    Y_Size = ((H26X_LINEOFFSET_MAX+63)>>6) * ((H26X_LINEOFFSET_MAX+63)>>6) * 64 * 64;
#else
    Y_Size = emu->uiFrameSize;
#endif
    UV_Size = (Y_Size/2);
    RC_SIZE = (((((uiHeight+63)/64)+3) >> 2) << 2) * 16 ;
	RC_SIZE *= (emu->uiTileNum+1);  // tile mode need 2X
    emu->Y_Size = Y_Size;
    emu->UV_Size = UV_Size;
	emu->uiRCSize = RC_SIZE;
	emu->uiUserQp_Size = UserQp_Size;
#if 0
    DBG_INFO("uiSrcFrmSize   = 0x%08x, %d %d\r\n", (unsigned int)emu->uiSrcFrmSize,(unsigned int)uiWidth,(unsigned int)uiHeight);
    DBG_INFO("uiFrameSize    = 0x%08x  = %6d K = %d M \r\n",(unsigned int)emu->uiFrameSize,(unsigned int)emu->uiFrameSize/1000,(unsigned int)emu->uiFrameSize/1000/1000);
    DBG_INFO("UserQp_Size    = 0x%08x  = %6d K = %d M \r\n",(unsigned int)UserQp_Size,(unsigned int)UserQp_Size/1000,(unsigned int)UserQp_Size/1000/1000);
    DBG_INFO("Y_Size         = 0x%08x  = %6d K = %d M \r\n",(unsigned int)Y_Size,(unsigned int)Y_Size/1000,(unsigned int)Y_Size/1000/1000);
    DBG_INFO("UV_Size        = 0x%08x  = %6d K = %d M \r\n",(unsigned int)UV_Size,(unsigned int)UV_Size/1000,(unsigned int)UV_Size/1000/1000);
    DBG_INFO("IlfSideInfo_Size = 0x%08x = %6d K = %d M \r\n",(unsigned int)emu->IlfSideInfo_Mall_Size,(unsigned int)emu->IlfSideInfo_Mall_Size/1000,(unsigned int)emu->IlfSideInfo_Mall_Size/1000/1000);
    DBG_INFO("ColMvs_Size    = 0x%08x  = %6d K = %d M \r\n",(unsigned int)emu->ColMvs_Size,(unsigned int)emu->ColMvs_Size/1000,(unsigned int)emu->ColMvs_Size/1000/1000);
    DBG_INFO("UserQp_Size    = 0x%08x  = %6d K = %d M\r\n",(unsigned int)UserQp_Size,(unsigned int)UserQp_Size/1000,(unsigned int)UserQp_Size/1000/1000);
#endif

	emu->wpcnt = 0;

    // Malloc Source YUV Buffer //
    // For SW load source, not need 32 bits alignment //
    // For HW usage source buffer , need 32 bits alignment //

    if(emu->src_d2d_en == 0)
    {
        h26xMemSetAddr(&emu->uiVirEmuSrcYAddr,&uiVirMemAddr,Y_Size);
        h26xMemSetAddr(&emu->uiVirEmuSrcUVAddr,&uiVirMemAddr,UV_Size);
        h26xMemSetAddr(&emu->uiVirHwSrcYAddr,&uiVirMemAddr,Y_Size);
        h26xMemSetAddr(&emu->uiVirHwSrcUVAddr,&uiVirMemAddr,UV_Size);
        H26XENC_CHEKMEM(emu->uiVirHwSrcYAddr);
        H26XENC_CHEKMEM(emu->uiVirHwSrcUVAddr);
        H26XENC_CHEKMEM(emu->uiVirEmuSrcYAddr);
        H26XENC_CHEKMEM(emu->uiVirEmuSrcUVAddr);
    }

	for(i = 0;i < RC_NUM_FRAME_TYPES ;i++){
		if(  (uiSvcInfo && (i != RC_B_FRAME))  || ((uiSvcInfo==0) && (i == RC_I_FRAME || i == RC_P_FRAME)) ){
			h26xMemSetAddr(&pH26XVirEncAddr->uiRCStatReadfrmAddr[i],&uiVirMemAddr, RC_SIZE);
	    	H26XENC_CHEKMEM(pH26XVirEncAddr->uiRCStatReadfrmAddr[i]);
			h26xMemSetAddr(&pH26XVirEncAddr->uiRCStatWritefrmAddr[i],&uiVirMemAddr, RC_SIZE);
	    	H26XENC_CHEKMEM(pH26XVirEncAddr->uiRCStatWritefrmAddr[i]);
		}
	}

    //DBG_INFO("HWSRC Y  x%08x - 0x%08x\r\n",uiPhyHwSrcYAddr,uiVirHwSrcYAddr));
    //DBG_INFO("HWSRC U  x%08x - 0x%08x\r\n",uiPhyHwSrcUVAddr,uiVirHwSrcUVAddr));
    //DBG_INFO("HWSRC Y  Limit 0x%08x - 0x%08x\r\n",uiVirHwSrcYAddr,uiVirHwSrcYAddr + Y_Size));
    //DBG_INFO("HWSRC UV Limit 0x%08x - 0x%08x\r\n",uiVirHwSrcUVAddr,uiVirHwSrcUVAddr + UV_Size));

    // Malloc Reconstruct and Reference Buffer , need 32 bits alignment //
    //uiRefBufInfo = 0x3;
    //DBG_INFO("uiRefBufInfo 0x%08x\r\n",uiRefBufInfo));
    DBG_INFO("FRM_IDX_MAX = %d \r\n",FRM_IDX_MAX);

    for(i = 0;i < FRM_IDX_MAX ;i++)
    {
    	//if(1)//hk tmp
        if((uiRefBufInfo>>i) & 0x1)
        {
#if 0
            h26xMemSetAddr(&pH26XVirEncAddr->uiRefAndRecYAddr[i],&uiVirMemAddr,emu->uiFrameSize);
            h26xMemSetAddr(&pH26XVirEncAddr->uiRefAndRecUVAddr[i],&uiVirMemAddr,emu->uiFrameSize/2);
#else
            h26xMemSetAddr(&pH26XVirEncAddr->uiRefAndRecYAddr[i],&uiVirMemAddr,Y_Size);
            h26xMemSetAddr(&pH26XVirEncAddr->uiRefAndRecUVAddr[i],&uiVirMemAddr,Y_Size/2);
#endif
            H26XENC_CHEKMEM(pH26XVirEncAddr->uiRefAndRecYAddr[i]);
            H26XENC_CHEKMEM(pH26XVirEncAddr->uiRefAndRecUVAddr[i]);
            //DBG_INFO("HWRex[%d] Y   Limit 0x%08x - 0x%08x\r\n",i,pH26XVirEncAddr->uiRefAndRecYAddr[i],pH26XVirEncAddr->uiRefAndRecYAddr[i] + Y_Size));
            //DBG_INFO("HWRex[%d] UV  Limit 0x%08x - 0x%08x\r\n",i,pH26XVirEncAddr->uiRefAndRecUVAddr[i],pH26XVirEncAddr->uiRefAndRecUVAddr[i] + UV_Size));

			h26xMemSetAddr(&emu->wp[emu->wpcnt],&uiVirMemAddr,16);emu->wpcnt++;

        }else{
            pH26XVirEncAddr->uiRefAndRecYAddr[i] = 0;
            pH26XVirEncAddr->uiRefAndRecUVAddr[i] = 0;
        }
    }


    for(i = 0;i < FRM_IDX_MAX ;i++)
    {
    	//if(1)//hk tmp
        if((uiRefBufInfo>>i) & 0x1)
        {
            h26xMemSetAddr(&pH26XVirEncAddr->uiIlfSideInfoAddr[i],&uiVirMemAddr,emu->IlfSideInfo_Mall_Size);
            //DBG_INFO("uiIlfSideInfoAddr[%d] = 0x%08x\r\n",i,pH26XVirEncAddr->uiIlfSideInfoAddr[i]));
            h26xMemSetAddr(&pH26XVirEncAddr->uiIlfExtraSideInfoAddr[i],&uiVirMemAddr,emu->IlfSideInfo_Mall_Size);
            H26XENC_CHEKMEM(pH26XVirEncAddr->uiIlfSideInfoAddr[i]);
            H26XENC_CHEKMEM(pH26XVirEncAddr->uiIlfExtraSideInfoAddr[i]);

			h26xMemSetAddr(&emu->wp[emu->wpcnt],&uiVirMemAddr,16);emu->wpcnt++;
        }else{
            pH26XVirEncAddr->uiIlfSideInfoAddr[i] = 0;
            pH26XVirEncAddr->uiIlfExtraSideInfoAddr[i] = 0;
        }
    }


#if EMU_CMP_SIDEINFO
    h26xMemSetAddr(&emu->uiVirSwIlfInfoAddr,&uiVirMemAddr,emu->IlfSideInfo_Mall_Size);
    H26XENC_CHEKMEM(emu->uiVirSwIlfInfoAddr);
#endif


	{
        //UINT32 s = 0x400;
        malloc_size = uiVirMemAddr - uiStartAdress;
        DBG_INFO("malloc = 0x%08x = %d K = %d M (0x%08x - 0x%08x)\r\n",(int)malloc_size,(int)(malloc_size/1000),(int)(malloc_size/1000/1000),(int)uiStartAdress,(int)uiVirMemAddr);
        if(malloc_size > (INT32)mem->size)
        {
            DBG_INFO("[%s][%d] malloc = 0x%08x > size (0x%08x) error\r\n",__FUNCTION__,__LINE__,(int)malloc_size,(int)mem->size);
            return 1;
        }
        //h26xMemSetAddr(&null_buf,&uiVirMemAddr,s);
        //memset((void*)null_buf,0,s);
    }

    for(i = 0;i < FRM_IDX_MAX ;i++)
    {
    	//if(1)//hk tmp
        if((uiRefBufInfo>>i) & 0x1)
        {
            h26xMemSetAddr(&pH26XVirEncAddr->uiColMvsAddr[i],&uiVirMemAddr,emu->ColMvs_Size);
            H26XENC_CHEKMEM(pH26XVirEncAddr->uiColMvsAddr[i]);
            memset((void*)pH26XVirEncAddr->uiColMvsAddr[i],0,emu->ColMvs_Size);
			h26x_flushCache(pH26XVirEncAddr->uiColMvsAddr[i],emu->ColMvs_Size);


			h26xMemSetAddr(&emu->wp[emu->wpcnt],&uiVirMemAddr,16);emu->wpcnt++;

        }else{
            pH26XVirEncAddr->uiColMvsAddr[i] = 0;
        }
        //memset((void*)pH26XVirEncAddr->uiColMvsAddr[i],0,ColMvs_Size);
        //DBG_INFO("uiColMvsAddr[%d] = 0x%08x\r\n",i,pH26XVirEncAddr->uiColMvsAddr[i]));
    }


	{
        //UINT32 s = 0x400;
        malloc_size = uiVirMemAddr - uiStartAdress;
        DBG_INFO("malloc = 0x%08x = %d K = %d M (0x%08x - 0x%08x)\r\n",(int)malloc_size,(int)(malloc_size/1000),(int)(malloc_size/1000/1000),(int)uiStartAdress,(int)uiVirMemAddr);
        if(malloc_size > (INT32)mem->size)
        {
            DBG_INFO("[%s][%d] malloc = 0x%08x > size (0x%08x) error\r\n",__FUNCTION__,__LINE__,(int)malloc_size,(int)mem->size);
            return 1;
        }
        //h26xMemSetAddr(&null_buf,&uiVirMemAddr,s);
        //memset((void*)null_buf,0,s);
    }

#if 0 //hk?? H26X_EMU_MAX_BUF
    {
        i = 16;
        DBG_INFO("%s %d\r\n",__FUNCTION__,__LINE__);
        malloc_size = uiVirMemAddr - uiStartAdress;
        DBG_INFO("malloc = 0x%08x = %d K = %d M (0x%08x - 0x%08x)\r\n",(int)malloc_size,(int)malloc_size/1000,(int)malloc_size/1000/1000,(int)uiStartAdress,(int)uiVirMemAddr);
        if(malloc_size > (INT32)mem->size)
        {
            DBG_INFO("malloc = 0x%08x > size (0x%08x) error\r\n",(int)malloc_size,(int)mem->size);
            return 1;
        }

        memset((void*)pH26XVirEncAddr->uiRefAndRecYAddr[i],0,emu->uiFrameSize);
		h26x_flushCache(pH26XVirEncAddr->uiRefAndRecYAddr[i],emu->uiFrameSize);
        memset((void*)pH26XVirEncAddr->uiRefAndRecUVAddr[i],0,emu->uiFrameSize);
		h26x_flushCache(pH26XVirEncAddr->uiRefAndRecUVAddr[i],emu->uiFrameSize);
		memset((void*)pH26XVirEncAddr->uiIlfSideInfoAddr[i],0,emu->IlfSideInfo_Mall_Size);
		h26x_flushCache(pH26XVirEncAddr->uiIlfSideInfoAddr[i],SIZE_32X(emu->IlfSideInfo_Mall_Size));
        memset((void*)pH26XVirEncAddr->uiIlfExtraSideInfoAddr[i],0,emu->IlfSideInfo_Mall_Size);
		h26x_flushCache(pH26XVirEncAddr->uiIlfExtraSideInfoAddr[i],SIZE_32X(emu->IlfSideInfo_Mall_Size));
        memset((void*)pH26XVirEncAddr->uiColMvsAddr[i],0,emu->ColMvs_Size);
		h26x_flushCache(pH26XVirEncAddr->uiColMvsAddr[i],emu->ColMvs_Size);
    }
#endif
    h26xMemSetAddr(&pH26XVirEncAddr->uiUserQpAddr,&uiVirMemAddr,UserQp_Size);
    H26XENC_CHEKMEM(pH26XVirEncAddr->uiUserQpAddr);
    for(i = 0;i < 3;i++)
    {
        h26xMemSetAddr(&pH26XVirEncAddr->uiMotionBitAddr[i],&uiVirMemAddr,SIZE_32X(uiMotionBitSize));
        H26XENC_CHEKMEM(pH26XVirEncAddr->uiMotionBitAddr[i]);
    }

    // Bitstream Buffer //
    //VideoSetMemory32bits

//#if H26X_EMU_MAX_BUF
#if 1
    emu->uiHwBsMaxLen = H26X_MAX_BS_LEN;
#elif 1
    emu->uiHwBsMaxLen = (emu->g_EmuInfo0 + 1023)/1024*1024;;
    emu->uiHwBsMaxLen *= 4;
    #if H26X_EMU_TEST_RAN_SLICE
    emu->uiHwBsMaxLen += (128*H26X_MAX_BSCMD_NUM)*2;
    #endif
    emu->uiHwBsMaxLen = (emu->uiHwBsMaxLen > H26X_MAX_BS_LEN)? H26X_MAX_BS_LEN: emu->uiHwBsMaxLen;
#elif 0
    emu->uiHwBsMaxLen = (emu->g_EmuInfo0 + 1023)/1024*1024;;
#endif
    //DBG_INFO("%s %d, pH26XRegSet->uiHwBsMaxLen = 0x%08x (0x%08x)\r\n",__FUNCTION__,__LINE__,pH26XRegSet->uiHwBsMaxLen,H26X_MAX_BS_LEN));

    h26xMemSetAddr(&pH26XVirEncAddr->uiCMDBufAddr,&uiVirMemAddr,H26X_CMB_BUF_MAX_SIZE);
    h26xMemSetAddr(&pH26XVirEncAddr->uiHwHeaderAddr,&uiVirMemAddr, H26X_MAX_HEADER_LEN);

	//DBG_INFO("set uiCMDBufAddr 0x%08x 0x%08x \n",(unsigned int)pH26XVirEncAddr->uiCMDBufAddr,(unsigned int)&pH26XVirEncAddr->uiCMDBufAddr);

     //h26xMemSetAddr(&pH26XVirEncAddr->uiHwBsAddr,&uiVirMemAddr,   H26X_MAX_BS_LEN);
    h26xMemSetAddr(&pH26XVirEncAddr->uiHwBsAddr,&uiVirMemAddr,   emu->uiHwBsMaxLen);


    #if EMU_CMP_BS
    //h26xMemSetAddr(&pH26XVirEncAddr->uiPicBsAddr,&uiVirMemAddr,  H26X_MAX_BS_LEN);
    h26xMemSetAddr(&pH26XVirEncAddr->uiPicBsAddr,&uiVirMemAddr,  emu->uiHwBsMaxLen);
    #endif

    H26XENC_CHEKMEM(pH26XVirEncAddr->uiCMDBufAddr);
    H26XENC_CHEKMEM(pH26XVirEncAddr->uiHwBsAddr);
    #if EMU_CMP_BS
    H26XENC_CHEKMEM(pH26XVirEncAddr->uiPicBsAddr);
    #endif
    H26XENC_CHEKMEM(pH26XVirEncAddr->uiHwHeaderAddr);

    #if H26X_EMU_NROUT
    //emu->iTestNROutEn = h26xRand()%2;
    emu->iTestNROutEn = 1;
    DBG_INFO("iTestNROutEn= %d\r\n",(int)emu->iTestNROutEn);
    if(emu->iTestNROutEn)
    {
        h26xMemSetAddr(&pH26XVirEncAddr->uiNROutYAddr,&uiVirMemAddr,emu->uiFrameSize);
        H26XENC_CHEKMEM(pH26XVirEncAddr->uiNROutYAddr);
        h26xMemSetAddr(&pH26XVirEncAddr->uiNROutUVAddr,&uiVirMemAddr,emu->uiFrameSize/2);
        H26XENC_CHEKMEM(pH26XVirEncAddr->uiNROutUVAddr);

		//memset((void*)pH26XVirEncAddr->uiNROutYAddr,0,emu->uiFrameSize);
		//memset((void*)pH26XVirEncAddr->uiNROutUVAddr,0,emu->uiFrameSize/2);
    }
    #else
    emu->iTestNROutEn = 0;
    emu->iNROutInterleave = 0;
    #endif

    #if EMU_CMP_NROUT
    h26xMemSetAddr(&emu->uiVirSwTnroutYAddr,&uiVirMemAddr,emu->uiFrameSize);
    H26XENC_CHEKMEM(emu->uiVirSwTnroutYAddr);
    h26xMemSetAddr(&emu->uiVirSwTnroutUVAddr,&uiVirMemAddr,emu->uiFrameSize/2);
    H26XENC_CHEKMEM(emu->uiVirSwTnroutUVAddr);
    #endif
    //DBG_INFO("%s %d, nrout = 0x%08x (0x%08x)\r\n",__FUNCTION__,__LINE__,
    //uiVirSwTnroutYAddr,uiVirSwTnroutUVAddr));
    //DBG_INFO("%s %d, nrout = 0x%08x (0x%08x)\r\n",__FUNCTION__,__LINE__,
    //pH26XVirEncAddr->uiNROutYAddr,pH26XVirEncAddr->uiNROutUVAddr));

	h26xMemSetAddr(&emu->uiVirtmpAddr,&uiVirMemAddr,1920*8);
	H26XENC_CHEKMEM(emu->uiVirtmpAddr);

    for(i = 0;i < FRM_IDX_MAX;i++)
    {
        for(j = 0; j < (emu->uiTileNum); j++)
        {
            if((uiRefBufInfo>>i) & 0x1)
            {
                h26xMemSetAddr(&pH26XVirEncAddr->uiRecExtraYAddr[i][j],&uiVirMemAddr,uiExtraRecSize);
                H26XENC_CHEKMEM(pH26XVirEncAddr->uiRecExtraYAddr[i][j]);
                h26xMemSetAddr(&pH26XVirEncAddr->uiRecExtraCAddr[i][j],&uiVirMemAddr,uiExtraRecSize/2);
                H26XENC_CHEKMEM(pH26XVirEncAddr->uiRecExtraCAddr[i][j]);

				h26xMemSetAddr(&emu->wp[emu->wpcnt],&uiVirMemAddr,16);emu->wpcnt++;

            }else{
                pH26XVirEncAddr->uiRecExtraCAddr[i][j] = 0;
            }

        }
        //DBG_INFO("[%d] uiRecExtraCAddr = 0x%08x, 0x%08x\r\n",i,pH26XVirEncAddr->uiRecExtraYAddr[i],pH26XVirEncAddr->uiRecExtraCAddr[i]));
    }

    #if EMU_CMP_REC
    h26xMemSetAddr(&emu->uiVirTmpRecExtraYAddr,&uiVirMemAddr,uiExtraRecSize);
    H26XENC_CHEKMEM(emu->uiVirTmpRecExtraYAddr);
    h26xMemSetAddr(&emu->uiVirTmpRecExtraCAddr,&uiVirMemAddr,uiExtraRecSize/2);
    H26XENC_CHEKMEM(emu->uiVirTmpRecExtraCAddr);
    #endif


    #if H26X_EMU_TEST_SREST
    h26xMemSetAddr(&emu->uiTmpRecYAddr,&uiVirMemAddr,Y_Size);
    //h26xMemSetAddr(&emu->uiTmpRecCAddr,&uiVirMemAddr,UV_Size);
    //h26xMemSetAddr(&emu->uiTmpIlfSideInfoAddr,&uiVirMemAddr,emu->IlfSideInfo_Mall_Size);
    //h26xMemSetAddr(&emu->uiTmpColMvsAddr,&uiVirMemAddr,emu->ColMvs_Size);
    p_job->tmp_rec_y_addr = h26x_getPhyAddr(emu->uiTmpRecYAddr);
    p_job->tmp_rec_c_addr = h26x_getPhyAddr(emu->uiTmpRecYAddr);
    p_job->tmp_colmv_addr = h26x_getPhyAddr(emu->uiTmpRecYAddr);
    p_job->tmp_sideinfo_addr = h26x_getPhyAddr(emu->uiTmpRecYAddr);
    for(i=0;i<H26X_MAX_TILEN_NUM-1;i++)
    {
        p_job->tmp_sideinfo_addr2[i] = h26x_getPhyAddr(emu->uiTmpRecYAddr);
        p_job->tmp_rec_y_addr2[i] = h26x_getPhyAddr(emu->uiTmpRecYAddr);
        p_job->tmp_rec_c_addr2[i] = h26x_getPhyAddr(emu->uiTmpRecYAddr);
    }
    p_job->tmp_tmnr_mtout_addr = h26x_getPhyAddr(emu->uiTmpRecYAddr);
    p_job->tmp_tmnr_rec_y_addr = h26x_getPhyAddr(emu->uiTmpRecYAddr);
    p_job->tmp_tmnr_rec_c_addr = h26x_getPhyAddr(emu->uiTmpRecYAddr);
    p_job->tmp_osg_gcac_addr0 = h26x_getPhyAddr(emu->uiTmpRecYAddr);
    p_job->tmp_osg_gcac_addr1 = h26x_getPhyAddr(emu->uiTmpRecYAddr);
	#else
	h26xMemSetAddr(&p_job->tmp_colmv_addr,&uiVirMemAddr, emu->ColMvs_Size );
    #endif

	h26xMemSetAddr(&pH26XVirEncAddr->uiNalLenDumpAddr,&uiVirMemAddr,H26X_NAL_LEN_MAX_SIZE);
	H26XENC_CHEKMEM(pH26XVirEncAddr->uiNalLenDumpAddr);
    DBG_INFO("%s %d, uiNalLenDumpAddr = 0x%08x\r\n",__FUNCTION__,__LINE__,(int)pH26XVirEncAddr->uiNalLenDumpAddr);

    {
        //UINT32 s = 0x400;
        malloc_size = uiVirMemAddr - uiStartAdress;
        DBG_INFO("malloc = 0x%08x = %d K = %d M (0x%08x - 0x%08x)\r\n",(int)malloc_size,(int)(malloc_size/1000),(int)(malloc_size/1000/1000),(int)uiStartAdress,(int)uiVirMemAddr);
        if(malloc_size > (INT32)mem->size)
        {
            DBG_INFO("[%s][%d] malloc = 0x%08x > size (0x%08x) error\r\n",__FUNCTION__,__LINE__,(int)malloc_size,(int)mem->size);
            return 1;
        }
        //h26xMemSetAddr(&null_buf,&uiVirMemAddr,s);
        //memset((void*)null_buf,0,s);
    }


	//h26xMemSetAddr(&pH26XVirEncAddr->uiLLCmdAddr,&uiVirMemAddr,H26X_MAX_LL_SIZE);
	//H26XENC_CHEKMEM(pH26XVirEncAddr->uiLLCmdAddr);
	//h26xMemSetAddr(&pH26XVirEncAddr->uiApbRdReg0DumpAddr,&uiVirMemAddr,H26X_MAX_LL_SIZE);
	//H26XENC_CHEKMEM(pH26XVirEncAddr->uiApbRdReg0DumpAddr);
	//pH26XRegSet->ll_cmd[0].base_pa = pH26XVirEncAddr->uiLLCmdAddr; //set link list address
	//pH26XRegSet->ll_cmd[0].rd_reg_pa = pH26XVirEncAddr->uiApbRdReg0DumpAddr; // set link list apb_rd_reg_0_dump_addr
    #if H26X_EMU_OSD
    if(emu->uiOsdEn)
    {

		h26xMemSetAddr(&pH26XVirEncAddr->uiOsgCacAddr,&uiVirMemAddr,uiOsgCacSize);
        H26XENC_CHEKMEM(pH26XVirEncAddr->uiOsgCacAddr);

		memset((void*)pH26XVirEncAddr->uiOsgCacAddr,0,uiOsgCacSize);
        h26x_flushCache(pH26XVirEncAddr->uiOsgCacAddr, SIZE_32X(uiOsgCacSize));

		h26xMemSetAddr(&emu->wp[emu->wpcnt],&uiVirMemAddr,16);emu->wpcnt++;

		emu->wprcnt = emu->wpcnt; //hk: save write protect read count

		for(i = 0;i < 32;i++)
        {
            //h26xMemSetAddr(&pH26XVirEncAddr->uiOsdAddr[i],&uiVirMemAddr,uiOsgGraphMaxSize);
            h26xMemSetAddr(&pH26XVirEncAddr->uiOsdAddr[i],&uiVirMemAddr,SIZE_32X(emu->uiOsgGraphMaxSize[i]));
            H26XENC_CHEKMEM(pH26XVirEncAddr->uiOsdAddr[i]);

			//hk: note that it's 2d read, may read out range
			h26xMemSetAddr(&emu->wp[emu->wpcnt],&uiVirMemAddr,16);emu->wpcnt++;

        }

        malloc_size = uiVirMemAddr - uiStartAdress;
        DBG_INFO("malloc = 0x%08x = %d K = %d M (0x%08x - 0x%08x)\r\n",(int)malloc_size,(int)(malloc_size/1000),(int)(malloc_size/1000/1000),(int)uiStartAdress,(int)uiVirMemAddr);
        if(malloc_size > (INT32)mem->size)
        {
            DBG_INFO("[%s][%d] malloc = 0x%08x > size (0x%08x) error\r\n",__FUNCTION__,__LINE__,(int)malloc_size,(int)mem->size);
            return 1;
        }

    }
    #endif


    //DBG_INFO("uiNROutYAddr = 0x%08x, 0x%08x\r\n",pH26XVirEncAddr->uiNROutYAddr,pH26XVirEncAddr->uiNROutUVAddr));
    #if 1
    pH26XVirEncAddr->uiRef0Idx = 0;
    pH26XVirEncAddr->uiRecIdx = 1;
    #else
    pH26XVirEncAddr->uiRef0Idx = 1;
    pH26XVirEncAddr->uiRecIdx = 0;
    #endif

    pH26XVirEncAddr->uiIlfWIdx = 0;
    pH26XVirEncAddr->uiIlfRIdx = 1;

    pH26XVirEncAddr->uiColMvWIdx = 0;
    pH26XVirEncAddr->uiColMvRIdx = 1;
    pH26XVirEncAddr->uiRecExtraStatus = 0;
    pH26XVirEncAddr->uiRecExtraEnable = 0;

    emu->uiMotionBitIdx[0] = 0;
    emu->uiMotionBitIdx[1] = 1;
    emu->uiMotionBitIdx[2] = 2;

    malloc_size = uiVirMemAddr - uiStartAdress;
    DBG_INFO("malloc = 0x%08x = %d K = %d M (0x%08x - 0x%08x)\r\n",(int)malloc_size,(int)(malloc_size/1000),(int)(malloc_size/1000/1000),(int)uiStartAdress,(int)uiVirMemAddr);

    if(malloc_size > (INT32)mem->size)
    {
        DBG_INFO("malloc = 0x%08x > size (0x%08x) error\r\n",(int)malloc_size,(int)mem->size);
        return 1;
    }

    mem->addr += SIZE_32X(malloc_size);
    mem->size -= SIZE_32X(malloc_size);

    return 0;
}

int emu_h265_init(h26x_job_t *p_job, UINT8 pucDir[265], h26x_srcd2d_t *p_src_d2d)
{
    h26x_mem_t *mem = &p_job->mem;
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    h265_pat_t *pat = &ctx->pat;
    int ret;
    DBG_DUMP("%s %d,srcnum = (%d %d %d)\r\n",__FUNCTION__,__LINE__,(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);

    p_job->mem.start     = p_job->mem_bak.start;
    p_job->mem.addr      = p_job->mem_bak.addr;
    p_job->mem.size      = p_job->mem_bak.size;

    DBG_INFO("%s %d, 0x%08x, size = 0x%08x\r\n",__FUNCTION__,__LINE__,(int)p_job->mem.addr,(int)p_job->mem.size);

    memset((void*)emu,0,sizeof(h265_emu_t));
    emu->uiAPBAddr       = p_job->apb_addr;
    emu->pH26XRegSet     = (H26XRegSet *)emu->uiAPBAddr;
    DBG_INFO("%s %d, 0x%08x, size = 0x%08x\r\n",__FUNCTION__,__LINE__,(int)emu->pH26XRegSet,(int)p_job->mem.size);
    memset((void*)emu->pH26XRegSet,0,sizeof(H26XRegSet));

    ctx->uiEncId         = p_job->idx1;
    ctx->eCodecType      = VCODEC_H265;

    memcpy((void*)emu->ucFileDir,pucDir,sizeof(emu->ucFileDir));
    memset((void*)&emu->file,0,sizeof(emu->file));

    ret = emu_h265_prepare_one_sequence( p_job, ctx, mem, p_src_d2d);
    if(ret == 1)
    {
        emu_h265_close_one_sequence( p_job, ctx, mem);
    }
    return ret;
}

static BOOL emu_h265_setup_one_job(h26x_ctrl_t *p_ctrl, unsigned int start_src_idx, unsigned int end_src_idx, unsigned int start_pat_idx, unsigned int end_pat_idx, unsigned int frm_num, UINT8 pucDir[265], h26x_srcd2d_t *p_src_d2d)
{
	h26x_job_t *job = h26x_job_add(0, p_ctrl);

	if (job == NULL)
		return FALSE;

	job->ctx_addr = h26x_mem_malloc(&job->mem, sizeof(h265_ctx_t));
    memset((void*)job->ctx_addr,0,sizeof(h265_ctx_t));

    DBG_INFO("%s %d, ctx_addr = 0x%08x\r\n",__FUNCTION__,__LINE__,(int)job->ctx_addr);


	job->mem.size -= (job->mem.addr - job->mem.start);
	job->mem.start = job->mem.addr;

	job->mem_bak.start = job->mem.start;
	job->mem_bak.addr  = job->mem.addr;
	job->mem_bak.size  = job->mem.size;

	h265_ctx_t *ctx = (h265_ctx_t *)job->ctx_addr;
	h265_pat_t *pat = (h265_pat_t *)&ctx->pat;

	job->start_folder_idx = start_src_idx;
	job->end_folder_idx = end_src_idx;
	job->start_pat_idx = start_pat_idx;
	job->end_pat_idx = end_pat_idx;
	job->end_frm_num = frm_num;
    #if H265_AUTO_JOB_SEL
   {


        int diff_src = end_src_idx - start_src_idx;
        int diff_pat = end_pat_idx - start_pat_idx;
        pat->uiPicNum = 0;
        pat->uiSrcNum = (h26xRand() % diff_src) + start_src_idx;
        pat->uiPatNum = (h26xRand() % diff_pat) + start_pat_idx;
		DBG_INFO("%s %d,uiSrcNum = %d %d(%d %d %d %d)\r\n",__FUNCTION__,__LINE__,(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)diff_src,(int)diff_pat,(int)start_src_idx,(int)start_pat_idx);


    }
    #else
    pat->uiSrcNum = start_src_idx;
    pat->uiPatNum = start_pat_idx;
    #endif
    pat->uiPicNum = 0;
    DBG_INFO("%s %d, mem.addr = 0x%08x\r\n",__FUNCTION__,__LINE__,(int)job->mem.addr);
#if 0
    emu_h265_init(job,pucDir);
#else
    {
        //int ret;
        h265_ctx_t *ctx = (h265_ctx_t *)job->ctx_addr;
        h265_pat_t *pat = (h265_pat_t *)&ctx->pat;

        while(emu_h265_init(job,pucDir, p_src_d2d) == 1)
        {
            pat->uiPatNum++;
            if((int)pat->uiPatNum >= (int)end_pat_idx)
            {
                DBG_INFO("%s %d,pat max\r\n",__FUNCTION__,__LINE__);
                pat->uiPicNum = 0;
                pat->uiPatNum = start_pat_idx;
                pat->uiSrcNum++;
            }
            if((int)pat->uiSrcNum >= (int)end_src_idx)
            {
                DBG_INFO("%s %d,src max\r\n",__FUNCTION__,__LINE__);
                job->is_finish = 1;
                DBG_INFO("is finish\r\n");
                return FALSE;
            }
        }
    }
#endif

    DBG_INFO("%s %d\r\n",__FUNCTION__,__LINE__);
	return TRUE;
}


void print_hevc_test(void)
{
    DBG_INFO("H26X_EMU_ROTATE            = %d\r\n",H26X_EMU_ROTATE         );
    DBG_INFO("H26X_EMU_NROUT             = %d\r\n",H26X_EMU_NROUT          );
    DBG_INFO("H26X_EMU_OSD               = %d\r\n",H26X_EMU_OSD            );
    DBG_INFO("H26X_EMU_SRC_CMPRS         = %d\r\n",H26X_EMU_SRC_CMPRS      );
    DBG_INFO("H26X_EMU_SMARTREC          = %d\r\n",H26X_EMU_SMARTREC       );
    DBG_INFO("H26X_EMU_SCD               = %d\r\n",H26X_EMU_SCD            );
    DBG_INFO("EMU_CMP_BS                 = %d\r\n",EMU_CMP_BS              );
    DBG_INFO("EMU_CMP_REC                = %d\r\n",EMU_CMP_REC             );
    DBG_INFO("EMU_CMP_NAL                = %d\r\n",EMU_CMP_NAL             );
    DBG_INFO("EMU_CMP_SIDEINFO           = %d\r\n",EMU_CMP_SIDEINFO        );
    DBG_INFO("EMU_CMP_PSNR               = %d\r\n",EMU_CMP_PSNR            );
    DBG_INFO("EMU_CMP_NROUT              = %d\r\n",EMU_CMP_NROUT           );
    DBG_INFO("H26X_EMU_MAX_BUF           = %d\r\n",H26X_EMU_MAX_BUF        );
    DBG_INFO("H26X_EMU_SRC_LINEOFFSET    = %d\r\n",H26X_EMU_SRC_LINEOFFSET  );
    DBG_INFO("H26X_EMU_REC_LINEOFFSET    = %d\r\n",H26X_EMU_REC_LINEOFFSET  );
    DBG_INFO("H26X_EMU_OSD_LINEOFFSET    = %d\r\n",H26X_EMU_OSD_LINEOFFSET  );
    DBG_INFO("H26X_EMU_TEST_SREST        = %d\r\n",H26X_EMU_TEST_SREST      );
    DBG_INFO("H26X_EMU_TEST_BSOUT        = %d\r\n",H26X_EMU_TEST_BSOUT      );
    //DBG_INFO("H26X_EMU_TEST_WP           = %d\r\n",H26X_EMU_TEST_WP         );
    //DBG_INFO("H26X_EMU_TEST_CLK          = %d\r\n",H26X_EMU_TEST_CLK        );
    DBG_INFO("H26X_EMU_TEST_RAN_SLICE    = %d\r\n",H26X_EMU_TEST_RAN_SLICE  );
    //DBG_INFO("H26X_EMU_TEST_BW           = %d\r\n",H26X_EMU_TEST_BW         );
    DBG_INFO("H26X_EMU_DRAM_2            = %d\r\n",H26X_EMU_DRAM_2          );
    DBG_INFO("H265_AUTO_JOB_SEL          = %d\r\n",H265_AUTO_JOB_SEL        );
    DBG_INFO("H265_EMU_NROUT_ONLY        = %d\r\n",H26X_EMU_NROUT_ONLY      );

}

BOOL emu_h265_setup(h26x_ctrl_t *p_ctrl, UINT8 pucDir[265], h26x_srcd2d_t *p_src_d2d, char src_path_d2d[2][128])
{
	BOOL ret = TRUE;
    //int end_pat,max_fn;

#if H265_TEST_MAIN_LOOP
   // iH265encTestMainLoopCnt++;
   // DBG_INFO("%s %d,iTestMainLoopCnt = %d\r\n",__FUNCTION__,__LINE__,(int)iH265encTestMainLoopCnt);
#endif
	print_hevc_test();

	if(pucDir != NULL){ //d2d
		memcpy(gucH26XEncTestSrc_Cut,src_path_d2d, 2 * 128 * sizeof(char));
		ret &= emu_h265_setup_one_job(p_ctrl, 0, 1, 1, 2, 400,(UINT8*)pucDir,p_src_d2d);
	}
	else{

		ret &= emu_h265_setup_one_job(p_ctrl, 0, 13, 1, 101, 400,(UINT8*)"PAT_0303",p_src_d2d); //all
		//ret &= emu_h265_setup_one_job(p_ctrl, 0, 13, 1, 101, 3,(UINT8*)"PAT_0303",p_src_d2d); //fast
		//ret &= emu_h265_setup_one_job(p_ctrl, 0, 6,  1, 101, 400,(UINT8*)"PAT_0303",p_src_d2d); //FHD
		//ret &= emu_h265_setup_one_job(p_ctrl, 6, 13, 1, 31, 400,(UINT8*)"PAT_0303",p_src_d2d); //small resolution and 4k


	}

	return ret;
}



//#endif //if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

