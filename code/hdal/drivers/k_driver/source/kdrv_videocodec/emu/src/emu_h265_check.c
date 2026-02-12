#include "stdio.h" //
#include "stdlib.h"
#include "string.h"
#include "emu_h26x_common.h"

#include "emu_h265_enc.h"
#include "emu_h265_tool.h"

#include "comm/hwclock.h"  // hwclock_get_counter



#if 0
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#else
#include <kwrap/task.h>
#define sleep(x)  vos_task_delay_us(1000*1000*x);
#define sleep_polling(x)  vos_task_delay_us_polling(1000*1000*sx);


#endif


//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

static UINT32 src_num,pat_num,pic_num;
extern UINT8 item_name[H26X_REPORT_NUMBER][256];
extern UINT8 item_name_2[H26X_REPORT_2_NUMBER][256];

extern BOOL emu_h265_compare_rec(h26x_job_t *p_job);
extern BOOL emu_h265_compare_picdata_sum(h26x_job_t *p_job);
#if 0
static UINT32 emu_cmp( UINT32 sw_val, unsigned int hw_val, UINT32 enable, UINT8 item[256]){
    UINT32 result = TRUE;

    if(enable == 0) return result;
    result = (sw_val == hw_val);
    if(result == 0)
    {
#if 0
        DBG_INFO("Src[%d] Pat[%d] Pic[%d] %s Error!\r\n",src_num,pat_num,pic_num,item);
        DBG_INFO("SW:0x%08x, HW:0x%08x\r\n",sw_val,hw_val);
#else
        DBG_INFO("SW:0x%08x, HW:0x%08x, %30s diff\r\n", (int)sw_val,(int)hw_val, item);
#endif
    }else{
        //DBG_INFO("SW:0x%08x, HW:0x%08x, %30s\r\n",sw_val,hw_val,item);
    }
	return result;
}
#else

#define emu_cmp(a,b,en,item) (en==1? (a==b) : 1);\
	if((en==1? (a==b) : 1)){\
	}\
	else{\
		DBG_INFO("SW:0x%08x, HW:0x%08x, %30s diff\r\n", (int)a,(int)b, item);\
	}\


#endif

static void emu_printf( UINT32 sw_val, unsigned int hw_val, UINT32 enable, UINT8 item[256]){
    DBG_INFO(" %s  SW:0x%08x, HW:0x%08x\r\n", item,(int)sw_val,(int)hw_val);
}

UINT32 emu_h265_read_bs(h265_emu_t *emu)
{
#if EMU_CMP_BS
#if(H26X_EMU_TEST_RAN_SLICE == 0 )
    UINT32  uiRet = 0;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
    uiRet = h26xFileRead(&emu->file.bs,pH26XRegCfg->uiH26XBsLen,(UINT32)pH26XVirEncAddr->uiPicBsAddr);
	h26x_flushCache((UINT32)pH26XVirEncAddr->uiPicBsAddr, SIZE_32X(pH26XRegCfg->uiH26XBsLen));
    if(uiRet) return H26X_EMU_FILEERR;
#endif
#endif
    return H26X_EMU_PASS;
}


#if EMU_CMP_SIDEINFO
static UINT32 emu_h265_read_sideinfo(h265_emu_t *emu)
{
    UINT32  IlfSideInfo_Size = emu->uiIlfLineOffset_cmodel * ((emu->uiHeight+63)/64);
    UINT32  uiRet = 0;
    memset((void*)emu->uiVirSwIlfInfoAddr, 0, IlfSideInfo_Size);
    uiRet = h26xFileRead(&emu->file.ilf_sideinfo,IlfSideInfo_Size,(UINT32)emu->uiVirSwIlfInfoAddr);
	h26x_flushCache((UINT32)emu->uiVirSwIlfInfoAddr, SIZE_32X(IlfSideInfo_Size));
    if(uiRet) return H26X_EMU_FILEERR;
    return H26X_EMU_PASS;
}
#endif

#if EMU_CMP_NROUT
static UINT32 emu_h265_read_nrout(h265_emu_t *emu)
{
    UINT32  uiRet = 0;
    uiRet = h26xFileReadFlush(&emu->file.tnr_out,(UINT32)(emu->uiSrcFrmSize),emu->uiVirSwTnroutYAddr);
    if(uiRet) return H26X_EMU_FILEERR;
    uiRet = h26xFileReadFlush(&emu->file.tnr_out,(UINT32)(emu->uiSrcFrmSize/2),emu->uiVirSwTnroutUVAddr);
    if(uiRet) return H26X_EMU_FILEERR;
    return H26X_EMU_PASS;
}
#endif

static void emu_h265_write_hwbs(h26x_job_t *p_job)
{
#ifdef WRITE_BS
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
#if 0
    unsigned int *CHK_REPORT = (unsigned int *)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
//dma_getNonCacheAddr
#else//hk tmp hack
	unsigned int *CHK_REPORT = (unsigned int *)(p_job->check1_addr);
#endif

	//h26xFileWrite(&emu->file.hw_bs,pH26XRegSet->CHK_REPORT[H26X_BS_LEN],pH26XVirEncAddr->uiHwBsAddr);
    h26xFileWrite(&emu->file.hw_bs,CHK_REPORT[H26X_BS_LEN],pH26XVirEncAddr->uiHwBsAddr);
    DBG_INFO("[%s][%d] 0x%08x\r\n", __FUNCTION__, __LINE__,pH26XRegSet->CHK_REPORT[H26X_BS_LEN]);
#endif
}

static void emu_h265_write_hwtnr(h265_emu_t *emu)
{
#ifdef WRITE_TNR
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    h26xFileWrite( &emu->file.hw_tnr, emu->uiFrameSize, pH26XVirEncAddr->uiNROutYAddr);
    h26xFileWrite( &emu->file.hw_tnr, emu->uiFrameSize/2, pH26XVirEncAddr->uiNROutUVAddr);
#endif
}

static void emu_h265_write_rec(h265_emu_t *emu)
{
#ifdef WRITE_REC
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    h26xFileWrite( &emu->file.hw_rec, emu->uiFrameSize, pH26XVirEncAddr->uiRefAndRecYAddr[pH26XVirEncAddr->uiRecIdx]);
    h26xFileWrite( &emu->file.hw_rec, emu->uiFrameSize/2, pH26XVirEncAddr->uiRefAndRecUVAddr[pH26XVirEncAddr->uiRecIdx]);
#endif
}


static void emu_h265_dump_hwbs(h26x_job_t *p_job)
{
#ifdef WRITE_DEBUG_BS
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    //H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
	h265_pat_t *pat = &ctx->pat;
	#if 0
     unsigned int *CHK_REPORT = ( unsigned int *)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
	//dma_getNonCacheAddr
	#else //hk tmp hack
	unsigned int *CHK_REPORT = (unsigned int *)(p_job->check1_addr);
	#endif

	UINT8   *pucFileName[256] ;
    static int id = 0;
    H26XFile fp;
    sprintf((char *)pucFileName,"%shw_bs_%s_E%d_%d.dat","A:\\",gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],pat->uiPatNum,id++);
    h26xFileOpen(&fp,(char *)pucFileName);
    //h26xFileWrite(&emu->file.hw_bs,pH26XRegSet->CHK_REPORT[H26X_BS_LEN],pH26XVirEncAddr->uiHwBsAddr);
    h26xFileWrite(&fp,CHK_REPORT[H26X_BS_LEN],pH26XVirEncAddr->uiHwBsAddr);
    DBG_INFO("[%s][%d] %s\r\n", __FUNCTION__, __LINE__,(char *)pucFileName);
    h26xFileClose(&fp);
#endif
}

static void emu_h265_dump_hwsrcout(h26x_job_t *p_job)
{
#ifdef WRITE_DEBUG_SRCOUT
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    //H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
	h265_pat_t *pat = &ctx->pat;
    UINT8   *pucFileName[256] ;
    static int id = 0;
    H26XFile fp;
    if(emu->iNROutEn==1){
        sprintf((char *)pucFileName,"%shw_srcout_%s_E%d_%d.dat","A:\\",gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],(int)pat->uiPatNum,id++);
        h26xFileOpen(&fp,(char *)pucFileName);
        h26xFileWrite( &fp, emu->uiFrameSize, pH26XVirEncAddr->uiNROutYAddr);
        h26xFileWrite( &fp, emu->uiFrameSize/2, pH26XVirEncAddr->uiNROutUVAddr);
        DBG_INFO("[%s][%d] %s\r\n", __FUNCTION__, __LINE__,(char *)pucFileName);
        h26xFileClose(&fp);
    }
#endif
}

static void emu_h265_dump_hwsrc(h26x_job_t *p_job)
{
#ifdef WRITE_DEBUG_SRC
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    //H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
	h265_pat_t *pat = &ctx->pat;
    UINT8   *pucFileName[256] ;
    static int id = 0;
    H26XFile fp;
    sprintf((char *)pucFileName,"%shw_src_%s_E%d_%d.dat","A:\\",gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],pat->uiPatNum,id++);
    h26xFileOpen(&fp,(char *)pucFileName);
#if 1
		h26xFileWrite( &fp, emu->uiFrameSize, pH26XVirEncAddr->uiSrcYAddr);
		h26xFileWrite( &fp, emu->uiFrameSize/2, pH26XVirEncAddr->uiSrcUVAddr);
#else
		UINT32 size_y,size_c;
		size_y = emu->uiSrcFrmSize;
		size_y = 1920*1080;
		size_c = size_y/2;
		emuh26x_msg(("uiSrc_Size = %d %d (0x%08x,0x%08x)\r\n",size_y,size_c, pH26XVirEncAddr->uiSrcYAddr, pH26XVirEncAddr->uiSrcUVAddr));
		h26xFileWrite( &fp, size_y, pH26XVirEncAddr->uiSrcYAddr);
		h26xFileWrite( &fp, size_c, pH26XVirEncAddr->uiSrcUVAddr);
#endif

    DBG_INFO("[%s][%d] %s\r\n", __FUNCTION__, __LINE__,(char *)pucFileName);
    h26xFileClose(&fp);

#endif
}

static void emu_h265_dump_hwrec(h26x_job_t *p_job)
{
#ifdef WRITE_DEBUG_REC
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    //H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
	h265_pat_t *pat = &ctx->pat;
    UINT8   *pucFileName[256] ;
    static int id = 0;
    H26XFile fp;
    sprintf((char *)pucFileName,"%shw_rec_%s_E%d_%d.dat","A:\\",gucH26XEncTestSrc_Cut[pat->uiSrcNum][1],pat->uiPatNum,id++);
    h26xFileOpen(&fp,(char *)pucFileName);
    h26xFileWrite( &fp, emu->uiFrameSize, pH26XVirEncAddr->uiRefAndRecYAddr[pH26XVirEncAddr->uiRecIdx]);
    h26xFileWrite( &fp, emu->uiFrameSize/2, pH26XVirEncAddr->uiRefAndRecUVAddr[pH26XVirEncAddr->uiRecIdx]);
    DBG_INFO("[%s][%d] %s\r\n", __FUNCTION__, __LINE__,(char *)pucFileName);
    h26xFileClose(&fp);
#endif
}

UINT32 emu_h265_read_nal(h265_emu_t *emu)
{
#if EMU_CMP_NAL
    UINT32  uiRet = 0;
	uiRet = h26xFileRead(&emu->file.nal_len,4,(UINT32)&emu->uiSliceNum);
    if(uiRet) return H26X_EMU_FILEERR;
	DBG_INFO("Slice Num = %d\r\n",(int)emu->uiSliceNum);
	uiRet = h26xFileRead(&emu->file.nal_len,sizeof(UINT32)*emu->uiSliceNum,(UINT32)&emu->uiNalLen[0]);
    if(uiRet) return H26X_EMU_FILEERR;
#endif
    return H26X_EMU_PASS;
}
#define max_stablelen 1000
int stablelen_idx = 0;
UINT32 stablelen_value[max_stablelen];

BOOL emu_h265_compare_stable_len(h26x_job_t *p_job)
{
    BOOL bRet = TRUE;
    UINT32 bs_len,slice_num;
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = &ctx->pat;
	H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    UINT8 *pucSwBsAddr,*pucHwBsAddr;
    UINT32 i,compare_pointnum = 5;
    UINT32 uiHwNalLen[H26X_MAX_BSCMD_NUM];
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
    UINT32 sw_bslen = ((pH26XRegCfg->uiH26XBsLen+3)>>2)<<2; // hw bs write out is 4X
    UINT32 uiIntStatus;
	H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;

    slice_num = h26x_getStableSliceNum();
    bs_len = h26x_getStableLen() *4;
    stablelen_value[stablelen_idx] = bs_len;
    stablelen_idx++;
    if(stablelen_idx >= max_stablelen)
        stablelen_idx = 0;
    uiIntStatus = h26x_getIntStatus();
    if(uiIntStatus != 0)
        return bRet;
    if(bs_len == 0 && slice_num == 0)
    {
        //DBG_INFO("stable len zero\r\n");
        return bRet;
    }
    pucSwBsAddr = (UINT8 *)pH26XVirEncAddr->uiPicBsAddr;
    pucHwBsAddr = (UINT8 *)pH26XVirEncAddr->uiHwBsAddr;

	h26x_flushCacheRead((UINT32)pH26XVirEncAddr->uiHwBsAddr,SIZE_32X(bs_len));
	if((pRegSet->FUNC_CFG[0]>>15)&1){ // DUMP_NAL_LEN_EN
		h26x_flushCacheRead(pH26XVirEncAddr->uiNalLenDumpAddr,SIZE_32X(slice_num));
	}

	if(EMU_CMP_BS == 0){
		DBG_ERR("EMU_CMP_BS need enable\r\n");
	}

    DBG_INFO("bs_len = 0x%x, last = 0x%x\r\n",(unsigned int)bs_len,(unsigned int)emu->stable_bs_len);
    if(bs_len < emu->stable_bs_len)
    {
        DBG_INFO("Src[%d] Pat[%d] Pic[%d] bs_len 0x%08x > 0x%08x (last stable_bs_len)\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum,
            (int)bs_len,(int)emu->stable_bs_len);
        bRet = FALSE;

    }

    if(bs_len > sw_bslen)
    {
        DBG_INFO("Src[%d] Pat[%d] Pic[%d] bs_len 0x%08x > 0x%08x (uiH26XBsLen)\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum,
            (int)bs_len,(int)sw_bslen);
        bRet = FALSE;
    }
    emu->stable_bs_len = bs_len;
    if(bRet == FALSE)
    {
        int i;
		DBG_INFO("emu_h265_compare_stable_len fail \r\n");
        DBG_INFO("uiIntStatus = 0x%x\r\n",(int)uiIntStatus);
        DBG_INFO("stablelen_idx = %d\r\n",(int)stablelen_idx);
        for(i=0;i<stablelen_idx;i++)
        {
            DBG_INFO("stablelen_value[%d] = 0x%x\r\n",(int)i,(int)stablelen_value[i]);
        }
        DBG_INFO("h26x_getStableSliceNum = 0x%x\r\n",(int)h26x_getStableSliceNum());
        DBG_INFO("h26x_getStableLen = 0x%x\r\n",(int)h26x_getStableLen() *4);
        DBG_INFO("uiIntStatus = 0x%x\r\n",(int)h26x_getIntStatus());
    }

    if(bs_len < compare_pointnum) return bRet;
    for(i=0;i<compare_pointnum;i++)
    {
        UINT32 j;
        j = bs_len-compare_pointnum+i;
        if(j >= pH26XRegCfg->uiH26XBsLen)
        {
            //DBG_INFO("break, bs_len = 0x%x, i = %d, j = 0x%x\r\n",bs_len,i,j);
            break;
        }

        //DBG_INFO("bs_len = 0x%x, j = 0x%x\r\n",bs_len,j);
        //DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",*(pucSwBsAddr + j),*(pucHwBsAddr + j));
        if (*(pucSwBsAddr + j) != *(pucHwBsAddr + j)){
            DBG_INFO("bs_len = 0x%x, i = %d, j = 0x%x\r\n",(int)bs_len,(int)i,(int)j);
            DBG_INFO("Src[%d] Pat[%d] Pic[%d] stable bistream error! offset = 0x%x\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum,(int)j);
            DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",*(pucSwBsAddr + j),*(pucHwBsAddr + j));
            //DBG_INFO("SW Addr:0x%08x, HW Addr:0x%08x\r\n",pH26XVirEncAddr->uiPicBsAddr,pH26XVirEncAddr->uiHwBsAddr);
            //h26xPrtMem((UINT32)pucSwBsAddr+i,0x10
            bRet = FALSE;
        }
    }

    if(slice_num != 0)
    {
        UINT32 total_nal_size = 0;
        for(i=0;i<slice_num;i++)
        {
            total_nal_size += emu->uiNalLen[i];
            uiHwNalLen[i] = h26x_getNalLen(i);
            if(uiHwNalLen[i] & 0x80000000)
            {
                UINT32 sw_nal,hw_nal;
                sw_nal = uiHwNalLen[i] & 0x7fffffff;
                hw_nal = emu->uiNalLen[i];
                //DBG_INFO("nal_len[%d] = 0x%08x, 0x%08x\r\n",i,sw_nal,hw_nal);
                if(sw_nal != hw_nal)
                {
                    DBG_INFO("stable nal len error! [%d] sw_nal = 0x%08x, hw_nal = 0x%08x\r\n",(int)i,(int)sw_nal,(int)hw_nal);
                    bRet = FALSE;
                }
            }
        }

        //DBG_INFO("sw total_nal_size = 0x%08x, hw bs_len = 0x%08x\r\n",total_nal_size,bs_len);
        if(bs_len < total_nal_size)
        {
            DBG_INFO("Src[%d] Pat[%d] Pic[%d] stable nal error!\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
            DBG_INFO("sw total_nal_size = 0x%08x, hw bs_len = 0x%08x\r\n",(int)total_nal_size,(int)bs_len);
            bRet = FALSE;
        }
    }
    if(bRet == FALSE)
    {
        int i;
        DBG_INFO("uiIntStatus = 0x%x\r\n",(int)uiIntStatus);
        DBG_INFO("stablelen_idx = %d\r\n",(int)stablelen_idx);
        for(i=0;i<stablelen_idx;i++)
        {
            DBG_INFO("stablelen_value[%d] = 0x%x\r\n",(int)i,(int)stablelen_value[i]);
        }
        DBG_INFO("h26x_getStableSliceNum = 0x%x\r\n",(int)h26x_getStableSliceNum());
        DBG_INFO("h26x_getStableLen = 0x%x\r\n",(int)h26x_getStableLen() *4);
        DBG_INFO("uiIntStatus = 0x%x\r\n",(int)h26x_getIntStatus());


		#if 0
		h26x_prtReg();
        h26x_getDebug();
		if (DIRECT_MODE == 0)
		{
			DBG_INFO("job(%d) apb data\r\n", p_job->idx1);
			h26x_prtMem(p_job->apb_addr, h26x_getHwRegSize());
			DBG_INFO("job(%d) linklist data\r\n", p_job->idx1);
			h26x_prtMem(p_job->llc_addr, h26x_getHwRegSize());
		}
		#endif

    }

    return bRet;
}


BOOL emu_h265_compare_nal(h26x_job_t *p_job, UINT32 uiSliceNum)
{
    BOOL bRet = TRUE;



#if (EMU_CMP_NAL==0 && H26X_EMU_NROUT_ONLY == 0)
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
	//h265_pat_t *pat = &ctx->pat;
	H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;

	H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;

	if((pRegSet->FUNC_CFG[0]>>15)&1){ // DUMP_NAL_LEN_EN
		h26x_flushCacheRead(pH26XVirEncAddr->uiNalLenDumpAddr,H26X_NAL_LEN_MAX_SIZE);
	}
	else{
		return bRet;
	}

#elif EMU_CMP_NAL
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = &ctx->pat;
	H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
	UINT32 i;
	//UINT32 uiLen[H26X_MAX_BSCMD_NUM];
	UINT32 uiVal;
	UINT32 *hw_nal_len = (UINT32 *)pH26XVirEncAddr->uiNalLenDumpAddr;
    UINT32 total_nal_len = 0;
	H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;

	if((pRegSet->FUNC_CFG[0]>>15)&1){ // DUMP_NAL_LEN_EN
		h26x_flushCacheRead(pH26XVirEncAddr->uiNalLenDumpAddr,H26X_NAL_LEN_MAX_SIZE);
	}
	else{
		return bRet;
	}

	//h26xFileRead(&emu->file.nal_len,4,(UINT32)&uiSliceNum);
	//DBG_INFO("Slice Num = %d\r\n",uiSliceNum);
	//h26xFileRead(&emu->file.nal_len,sizeof(UINT32)*uiSliceNum,(UINT32)&uiLen);

	if( uiSliceNum > 176){
		DBG_INFO("skip nal len compare, hw limit\r\n");
		return bRet;
	}

	for(i=0;i<uiSliceNum;i++)
	{
		UINT32 uiDiff;

        uiVal =  hw_nal_len[i];

		#if H26X_EMU_TEST_RAN_SLICE
		emu->uiNalLen[i] -= pH26XVirEncAddr->uiHwHeaderSizeTmp[i];
		emu->uiNalLen[i]+= pH26XVirEncAddr->uiHwHeaderSize[i];
		#endif
        total_nal_len += emu->uiNalLen[i];

		uiDiff = uiVal - emu->uiNalLen[i];

        //DBG_INFO("NalLen[%d] sw : 0x%x, hw : 0x%x) (0x%08x)\r\n",i,emu->uiNalLen[i],uiVal,total_nal_len);
		if(uiDiff != 0)
		{
            DBG_INFO("Slice Num = %d\r\n",(int)uiSliceNum);
			DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare Nal Len Error!(%d, sw : %d, hw : %d)\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum,
				(int)i,(int)emu->uiNalLen[i],(int)uiVal);
			bRet = FALSE;
		}
	}
#endif
    return bRet;
}

UINT32 CalDmaSum(UINT32 uiEmuSrcAddr, UINT32 len){
	UINT32 i,sum = 0;
	for (i = 0; i < len; i++){
		sum += *(UINT8*)(uiEmuSrcAddr + i);
	}
	return sum;
}
static BOOL emu_h265_compare_rc(h26x_job_t *p_job)
{
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
    UINT32 item_id;
    BOOL result = TRUE;
    UINT32 check;
    UINT32 mbqp_en,rowrc_en,rc_en,rc2_en;

#if DIRECT_MODE
    H26XRegSet   *pH26XRegSet = emu->pH26XRegSet;
    unsigned int *CHK_REPORT = (unsigned int *)pH26XRegSet->CHK_REPORT;
#else
#if 0
	 unsigned int *CHK_REPORT = ( unsigned int *)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
	//dma_getNonCacheAddr
#else //hk tmp hack
	unsigned int *CHK_REPORT = (unsigned int *)(p_job->check1_addr);
#endif


#endif

    item_id = H26X_RRC_BIT_LEN;
    check = 1;
    result &= emu_cmp( pH26XRegCfg->uiH26X_RC_EST_BITLEN, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);

    mbqp_en = (pH26XRegCfg->uiH26XFunc0 >> 8) & 1;
    rowrc_en = (pH26XRegCfg->uiH26XRrc0 >> 1) & 1;
    rc_en = pH26XRegCfg->uiH26XRrc0 & 1;
    rc2_en = (pH26XRegCfg->uiH26XRrc0>>27) & 1;

    item_id = H26X_RRC_RDOPT_COST_LSB;
    check = mbqp_en;
    result &= emu_cmp( pH26XRegCfg->uiH26X_RC_RDO_COST_LOW, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    item_id = H26X_RRC_RDOPT_COST_MSB;
    result &= emu_cmp( pH26XRegCfg->uiH26X_RC_RDO_COST_HIGH, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);

    item_id = H26X_RRC_SSE_DIST_LSB;
    check = 1;
    result &= emu_cmp( pH26XRegCfg->uiH26X_RC_Frm_SSE_DIST_Low, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    item_id = H26X_RRC_SSE_DIST_MSB;
    result &= emu_cmp( pH26XRegCfg->uiH26X_RC_Frm_SSE_DIST_High, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);

    item_id = H26X_RRC_SIZE;
    check = (rowrc_en == 1 && mbqp_en == 0);
    result &= emu_cmp( pH26XRegCfg->uiH26X_RC_Frm_Size, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    item_id = H26X_RRC_FRM_COST_LSB;
    result &= emu_cmp( pH26XRegCfg->uiH26X_RC_Frm_cost_Low, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    item_id = H26X_RRC_FRM_COST_MSB;
    result &= emu_cmp( pH26XRegCfg->uiH26X_RC_Frm_cost_High, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    item_id = H26X_RRC_FRM_COMPLEXITY_LSB;
    result &= emu_cmp( pH26XRegCfg->uiH26X_RC_Frm_cmplxty_Low, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    item_id = H26X_RRC_FRM_COMPLEXITY_MSB;
    result &= emu_cmp( pH26XRegCfg->uiH26X_RC_Frm_cmplxty_High, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);

    item_id = H26X_RRC_COEFF;
    check = (rc_en == 1 && mbqp_en == 0 && !rc2_en);
    result &= emu_cmp( pH26XRegCfg->uiH26X_RC_Frm_Coeff, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    item_id = H26X_RRC_QP_SUM;
    result &= emu_cmp( pH26XRegCfg->uiH26X_RC_Frm_qp_sum, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    item_id = H26X_RRC_COEFF2;
    result &= emu_cmp( pH26XRegCfg->uiH26X_RC_Frm_Coeff2, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    item_id = H26X_RRC_COEFF3;
    result &= emu_cmp( pH26XRegCfg->uiH26X_RC_Frm_Coeff3, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    //emu_printf( pH26XRegCfg->uiH26X_RC_Frm_Coeff2, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);

    return result;
}

static BOOL emu_h265_compare_nrout(h26x_job_t *p_job)
{
    BOOL bRet = TRUE;
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = &ctx->pat;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
	UINT8 *pucHwYAddr,*pucHwUVAddr;

	pucHwYAddr = (UINT8 *)pH26XVirEncAddr->uiNROutYAddr;
	pucHwUVAddr = (UINT8 *)pH26XVirEncAddr->uiNROutUVAddr;

	if(emu->iNROutEn==0){
		return TRUE;
	}
	else{ //hk
		h26x_flushCacheRead((UINT32)pucHwYAddr, emu->uiFrameSize);
		h26x_flushCacheRead((UINT32)pucHwUVAddr, emu->uiFrameSize/2);
	}

#if EMU_CMP_DRAM_CHKSUM
{
	UINT32 *pucHw = (UINT32 *)pH26XVirEncAddr->uiNROutYAddr;
	UINT32 uiHwAddr = 0;
	UINT32 uiCS = 0;
	UINT32 line_offset = SIZE_64X(emu->uiWidth);
	H26XEncDramInfo *pH26XEncDramInfo = &emu->sH26XEncDramInfo;
	UINT32 i,j;
	//UINT32 st = (unsigned int)hwclock_get_counter();

	for( i = 0 ; i < emu->uiHeight; i++)
    {
        for( j = 0; j < emu->uiWidth/4; j++)
        {
			uiCS += pucHw[uiHwAddr+j];		
		}
		uiHwAddr += line_offset/4;
	}
	
	uiHwAddr = 0;
	pucHw = (UINT32 *)pH26XVirEncAddr->uiNROutUVAddr;
	for( i = 0 ; i < (emu->uiHeight/2); i++)
    {
        for( j = 0; j < emu->uiWidth/4; j++)
        {
            uiCS += pucHw[uiHwAddr+j];	
		}
		uiHwAddr += line_offset/4;
	}

	//DBG_INFO("NrOut Y SW:0x%08x, HW:0x%08x\r\n", (unsigned int)pH26XEncDramInfo->uiH26XSrcOutDramSum, (unsigned int)uiCS);
	if(uiCS!= pH26XEncDramInfo->uiH26XSrcOutDramSum){
		DBG_INFO("\r\n");
		DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare NrOut Y Dma Check Sum Error!\r\n", (int)pat->uiSrcNum, (int)pat->uiPatNum, (int)pat->uiPicNum);
		DBG_INFO("SW:0x%08x, HW:0x%08x\r\n", (unsigned int)pH26XEncDramInfo->uiH26XSrcOutDramSum, (unsigned int)uiCS);
		bRet = FALSE;
	}
	//DBG_INFO("emu_h265_compare_nrout time = %d\r\n", (unsigned int)hwclock_get_counter() - (unsigned int) st);
}
#endif

#if (EMU_CMP_NROUT_CHKSUM==0 && H26X_EMU_NROUT_ONLY == 0)
	//
	
#elif EMU_CMP_NROUT_CHKSUM

    UINT32 i,j;
    UINT8 *pucHw;
    UINT32 uiCS = 0;
#if EMU_CMP_NROUT
    UINT8 recErrCnt = 0;
    UINT8 *pucSw;
    UINT8 *pucSwYAddr,*pucSwUVAddr;
#endif
    UINT32 uiHwAddr,uiSwAddr;
    INT16 val;

    UINT32 chksum_y, chksum_c;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
    UINT32 line_offset;
    chksum_y = pH26XRegCfg->uiH26X_TnrOutChkSumY;
    chksum_c = pH26XRegCfg->uiH26X_TnrOutChkSumC;

#if EMU_CMP_NROUT
    pucSwYAddr = (UINT8 *)emu->uiVirSwTnroutYAddr;
    pucSwUVAddr = (UINT8 *)emu->uiVirSwTnroutUVAddr;
#endif

    #if EMU_CMP_NROUT
    if( emu_h265_read_nrout(emu) != H26X_EMU_PASS) return FALSE;
    #endif

#if 0 //compare heavyload chksum
		{
			H26XEncDramInfo *pH26XEncDramInfo = &emu->sH26XEncDramInfo;
			UINT32 hw_dram_chksum=0;
			UINT32 sw_dram_chksum=0;
			UINT32 compare_size = 0;
			line_offset = SIZE_64X(emu->uiWidth);

			compare_size = emu->uiFrameSize * 3 / 2; // line_offset * emu->uiHeight;

			//hw_dram_chksum = h26x_getDramChkSum(pH26XVirEncAddr->uiNROutYAddr,  emu->uiFrameSize  );// 2 byte as unit, 4 byte align
			//DBG_INFO("hw_dram_chksum_y 0x%08x\r\n" ,(unsigned int)hw_dram_chksum);

			DBG_INFO("w 0x%08x h 0x%08x l 0x%08x\r\n" ,(unsigned int)emu->uiWidth,(unsigned int)emu->uiHeight,(unsigned int)line_offset );
			DBG_INFO("uiNROutYAddr 0x%08x 0x%08x \r\n" ,(unsigned int)pH26XVirEncAddr->uiNROutYAddr, (unsigned int)pH26XVirEncAddr->uiNROutUVAddr);

#if 1
			DBG_INFO("start_time %d \r\n" ,(unsigned int)hwclock_get_counter());
			hw_dram_chksum = h26x_getDramChkSum(pH26XVirEncAddr->uiNROutYAddr, compare_size  );// 2 byte as unit, 4 byte align
			DBG_INFO("end_time %d \r\n" ,(unsigned int)hwclock_get_counter());
			DBG_INFO("hw_dram_chksum 0x%08x\r\n" ,(unsigned int)hw_dram_chksum);

			DBG_INFO("start_time %d \r\n" ,(unsigned int)hwclock_get_counter());
			sw_dram_chksum = add_float_neon (pH26XVirEncAddr->uiNROutYAddr, compare_size );
			DBG_INFO("end_time %d \r\n" ,(unsigned int)hwclock_get_counter());
			DBG_INFO("sw_neon_chksum 0x%08x\r\n" ,(unsigned int)sw_dram_chksum);

			DBG_INFO("start_time %d \r\n" ,(unsigned int)hwclock_get_counter());
			sw_dram_chksum = h26x_calDram32ChkSum(pH26XVirEncAddr->uiNROutYAddr, compare_size );
			DBG_INFO("end_time %d \r\n" ,(unsigned int)hwclock_get_counter());
			DBG_INFO("sw_dram32_chksum 0x%08x\r\n" ,(unsigned int)sw_dram_chksum);

			DBG_INFO("start_time %d \r\n" ,(unsigned int)hwclock_get_counter());
			sw_dram_chksum = h26x_calDramChkSum(pH26XVirEncAddr->uiNROutYAddr, compare_size );
			DBG_INFO("end_time %d \r\n" ,(unsigned int)hwclock_get_counter());
			DBG_INFO("sw_dram_chksum 0x%08x\r\n" ,(unsigned int)sw_dram_chksum);

#endif

			DBG_INFO("uiH26XSrcOutDramSum 0x%08x\r\n" ,(unsigned int)pH26XEncDramInfo->uiH26XSrcOutDramSum);

			if(hw_dram_chksum != pH26XEncDramInfo->uiH26XSrcOutDramSum){
				DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare Source Out Dram Error!\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
				DBG_INFO("compare_size = 0x%08x sw:0x%08x, hw:0x%08x fail\r\n" ,(unsigned int)( compare_size) ,(unsigned int)pH26XEncDramInfo->uiH26XSrcOutDramSum, (unsigned int)hw_dram_chksum);
				bRet = FALSE;
			}
		}
#endif

	//DBG_INFO("start_time %d \r\n" ,(unsigned int)hwclock_get_counter());

    // Luma
#if EMU_CMP_NROUT
    pucSw = pucSwYAddr;
#endif
    pucHw = pucHwYAddr;
    uiHwAddr = 0;
    uiSwAddr = 0;
    //DBG_INFO("compare Luma\r\n");
    line_offset = SIZE_64X(emu->uiWidth);

#if EMU_CMP_NROUT
	//emuh26x_msg(("SW:0x%02x 0x%02x 0x%02x 0x%02x \r\n",pucSw[uiSwAddr],pucSw[uiSwAddr+1],pucSw[uiSwAddr+2],pucSw[uiSwAddr+3]));
	//emuh26x_msg(("HW:0x%02x 0x%02x 0x%02x 0x%02x \r\n",pucHw[uiHwAddr],pucHw[uiHwAddr+1],pucHw[uiHwAddr+2],pucHw[uiHwAddr+3]));
#endif

    for( i = 0 ; i < emu->uiHeight; i++)
    {
        //DBG_INFO("=========================\r\n");
        for( j = 0; j < emu->uiWidth; j++)
        {
#if EMU_CMP_NROUT
            if(pucSw[uiSwAddr+j] != pucHw[uiHwAddr+j])
            {
                DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare Luma Tnrout Error!\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
                DBG_INFO("Error at i = %d %d, swaddr = %d %d\r\n",
                   (int) i,(int)j,(unsigned int)(uiSwAddr+j),(unsigned int)(uiHwAddr+j));
                DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",(unsigned int)pucSw[uiSwAddr+j],(unsigned int)pucHw[uiHwAddr+j]);
                bRet = FALSE;
                recErrCnt++;
                if(recErrCnt > 20)
                    //return bRet;
                    break;
            }
			#if 0
			else{
				if(i==6 && j==184){
				 DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare Luma Tnrout Correct!\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
                DBG_INFO(" at i = %d %d, swaddr = %d %d\r\n",
                   (int) i,(int)j,(unsigned int)(uiSwAddr+j),(unsigned int)(uiHwAddr+j));
                DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",(unsigned int)pucSw[uiSwAddr+j],(unsigned int)pucHw[uiHwAddr+j]);


				}
			}
			#endif

#endif
            val = (pucHw[uiHwAddr+j]);
            uiCS += (UINT32)val;

              //DBG_INFO("recErrCnt = %d\r\n", recErrCnt);
        } // end of j
        //DBG_INFO("[%d] , ck = 0x%08x\r\n", i ,uiCS);
        uiHwAddr += line_offset;
        uiSwAddr += emu->uiWidth;
    } // end of i

    DBG_INFO("NrOut Y SW:0x%08x, HW:0x%08x\r\n", (int)chksum_y, (int)uiCS);
	if(uiCS!= chksum_y){
		DBG_INFO("\r\n");
		DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare NrOut Y Check Sum Error!\r\n", (int)pat->uiSrcNum, (int)pat->uiPatNum, (int)pat->uiPicNum);
		DBG_INFO("SW:0x%08x, HW:0x%08x\r\n", (int)chksum_y, (int)uiCS);
		bRet = FALSE;
	}
    // Chroma
    uiHwAddr = 0;
    uiSwAddr = 0;
    uiCS = 0;
#if EMU_CMP_NROUT
    recErrCnt = 0;
    pucSw = pucSwUVAddr;
#endif
    pucHw = pucHwUVAddr;
    //DBG_INFO("compare Chroma\r\n");
    //DBG_INFO("pucSw = 0x%08x\r\n",pucSw);
    //h26xPrtMem((UINT32)pucSw,128);
    //DBG_INFO("pucHw = 0x%08x\r\n",pucHw);
    //h26xPrtMem((UINT32)pucHw,128);
    //DBG_INFO("\r\n");

#if EMU_CMP_NROUT
	//emuh26x_msg(("SW:0x%02x 0x%02x 0x%02x 0x%02x \r\n",pucSw[uiSwAddr],pucSw[uiSwAddr+1],pucSw[uiSwAddr+2],pucSw[uiSwAddr+3]));
	//emuh26x_msg(("HW:0x%02x 0x%02x 0x%02x 0x%02x \r\n",pucHw[uiHwAddr],pucHw[uiHwAddr+1],pucHw[uiHwAddr+2],pucHw[uiHwAddr+3]));
#endif

    for( i = 0 ; i < (emu->uiHeight/2); i++)
    {
        //DBG_INFO("=========================\r\n");
        for( j = 0; j < emu->uiWidth; j++)
        {

#if EMU_CMP_NROUT
			if(emu->iNROutInterleave){
				if(j%2==0){
					//UINT32 u = pucSw[uiSwAddr+j];
					//UINT32 v = pucSw[uiSwAddr+j+1];

					if( (pucSw[uiSwAddr+j] != pucHw[uiHwAddr+j+1])
						|| (pucSw[uiSwAddr+j+1] != pucHw[uiHwAddr+j]) )
					{
						DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare Chroma Tnrout Error!\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
						DBG_INFO("Error at i = %d %d, swaddr = %d %d\r\n",
							(int)i,(int)j,(unsigned int)(uiSwAddr+j),(unsigned int)(uiHwAddr+j));
						DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",(unsigned int)pucSw[uiSwAddr+j],(unsigned int)pucHw[uiHwAddr+j]);
						bRet = FALSE;
						recErrCnt++;
						if(recErrCnt > 20)
							return bRet;
							//break;

					}

				}

			}
			else if(pucSw[uiSwAddr+j] != pucHw[uiHwAddr+j])
			{
						DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare Chroma Tnrout Error!\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
						DBG_INFO("Error at i = %d %d, swaddr = %d %d\r\n",
							(int)i,(int)j,(unsigned int)(uiSwAddr+j),(unsigned int)(uiHwAddr+j));
						DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",(unsigned int)pucSw[uiSwAddr+j],(unsigned int)pucHw[uiHwAddr+j]);
						bRet = FALSE;
						recErrCnt++;
						if(recErrCnt > 20)
							return bRet;
							//break;
			}
			else{
				//emuh26x_msg(("Src[%d] Pat[%d] Pic[%d] Compare Chroma Tnrout Correct!\r\n",pat->uiSrcNum,pat->uiPatNum,pat->uiPicNum));
			}
#endif


            val = (pucHw[uiHwAddr+j]);
            uiCS += (UINT32)val;

              //DBG_INFO("recErrCnt = %d\r\n", recErrCnt);
        } // end of j
        //DBG_INFO("[%d] , ck = 0x%08x\r\n", i ,uiCS);
        uiHwAddr += line_offset;
        uiSwAddr += emu->uiWidth;
    } // end of i
    //DBG_INFO("NrOut C SW:0x%08x, HW:0x%08x\r\n", chksum_c, uiCS);
	if(uiCS!= chksum_c){
		DBG_INFO("\r\n");
		DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare NrOut C Check Sum Error!\r\n", (int)pat->uiSrcNum, (int)pat->uiPatNum, (int)pat->uiPicNum);
		DBG_INFO("SW:0x%08x, HW:0x%08x\r\n", (int)chksum_c, (int)uiCS);
		bRet = FALSE;
	}
#endif


	//DBG_INFO("end_time %d \r\n" ,(unsigned int)hwclock_get_counter());

    return bRet;
}

static BOOL emu_h265_compare_saonum(h26x_job_t *p_job)
{
    BOOL bRet = TRUE;
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = &ctx->pat;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
#if DIRECT_MODE
    H26XRegSet   *pH26XRegSet = emu->pH26XRegSet;
     unsigned int *CHK_REPORT = (unsigned int *) pH26XRegSet->CHK_REPORT;
#else
#if 0
     unsigned int *CHK_REPORT = ( unsigned int *)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 0);
	//dma_getNonCacheAddr
	#else //hk tmp hack
unsigned int *CHK_REPORT = (unsigned int *)(p_job->check1_addr);
	#endif

#endif

    if((pH26XRegCfg->uiH26XPic3 >> 16) & 1)//ILF_SAO_EN
    {
        //sao enable
        UINT32 uiSaoNum_SW = pH26XRegCfg->uiH26XSaoInfo;
        UINT32 uiSaoNum_HW = CHK_REPORT[H26X_ILF_DIS_CTB];
        UINT32 uiLumaNum_SW,uiChromaNum_SW;
        UINT32 uiLumaNum_HW,uiChromaNum_HW;
        UINT32 uiLumaMask = 0xFFFF;
        UINT32 uiChromaMask = 0xFFFF;
        uiLumaNum_SW = uiSaoNum_SW & uiLumaMask;
        uiLumaNum_HW = uiSaoNum_HW & uiLumaMask;
        uiChromaNum_SW = (uiSaoNum_SW >> 16) & uiChromaMask;
        uiChromaNum_HW = (uiSaoNum_HW >> 16) & uiChromaMask;
        if(((pH26XRegCfg->uiH26XPic3 >> 17) & 1) == 0)
        {
            uiLumaNum_SW = SIZE_64X(emu->uiWidth) * SIZE_64X(emu->uiHeight)/64/64;
            uiLumaNum_SW = uiLumaNum_SW & uiLumaMask;
        }
        if(((pH26XRegCfg->uiH26XPic3 >> 18) & 1) == 0)
        {
            uiChromaNum_SW = SIZE_64X(emu->uiWidth) * SIZE_64X(emu->uiHeight)/64/64;
            uiChromaNum_SW = uiChromaNum_SW & uiChromaMask;
        }
        //luma
        if (uiLumaNum_SW != uiLumaNum_HW)
        {
            DBG_INFO("\r\n");
            DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare SaoNum Luma Error!\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
            DBG_INFO("SW:0x%08x, HW:0x%08x\r\n",(int)uiLumaNum_SW,(int)uiLumaNum_HW);
            DBG_INFO("uiH26XILFCfg:0x%08x (SW:0x%08x, HW:0x%08x)\r\n",(int)pH26XRegCfg->uiH26XPic3,(int)uiSaoNum_SW,(int)uiSaoNum_HW);
            bRet = FALSE;
        }
        //chorma
        if (uiChromaNum_SW != uiChromaNum_HW)
        {
            DBG_INFO("\r\n");
            DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare SaoNum Chroma Error!\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
            DBG_INFO("SW:0x%08x, HW:0x%08x\r\n",(int)uiChromaNum_SW,(int)uiChromaNum_HW);
            DBG_INFO("uiH26XILFCfg:0x%08x (SW:0x%08x, HW:0x%08x)\r\n",(int)pH26XRegCfg->uiH26XPic3,(int)uiSaoNum_SW,(int)uiSaoNum_HW);
            bRet = FALSE;
        }

    }
    return bRet;

}

static BOOL emu_h265_compare_psnr(h26x_job_t *p_job)
{
    BOOL result = TRUE;
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
    UINT32       i;
    UINT32 item_id;
    UINT32  check = 1;
    UINT32  *result_sw;
#if DIRECT_MODE
    H26XRegSet   *pH26XRegSet = emu->pH26XRegSet;
     unsigned int *CHK_REPORT = (unsigned int *)pH26XRegSet->CHK_REPORT;
	 unsigned int *CHK_REPORT_2 = (unsigned int *)pH26XRegSet->CHK_REPORT_2;
#else
#if 0
     unsigned int *CHK_REPORT = ( unsigned int *)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 0);
	 unsigned int *CHK_REPORT_2 = ( unsigned int *)(p_job->apb_addr + H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
//dma_getNonCacheAddr
#else //hk tmp hack
unsigned int *CHK_REPORT = (unsigned int *)(p_job->check1_addr);

//unsigned int *CHK_REPORT_2 = (unsigned int *)(p_job->check2_addr);
unsigned int *CHK_REPORT_2 = ( unsigned int *)(p_job->apb_addr + H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET);

#endif


#endif

#if EMU_CMP_PSNR
    result_sw = &pH26XRegCfg->uiH26X_YMSE_Low;
	for (i = H26X_PSNR_FRM_Y_LSB; i <=  H26X_PSNR_FRM_V_MSB; i++)
	{
        item_id = i;
        result &= emu_cmp( result_sw[i-H26X_PSNR_FRM_Y_LSB], CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    }
#endif
    result_sw = &pH26XRegCfg->uiH26X_YMSE_ROI_Low;
	for (i = H26X_PSNR_ROI_Y_LSB; i <=  H26X_PSNR_ROI_V_MSB; i++)
	{
        item_id = i;
        result &= emu_cmp( result_sw[i-H26X_PSNR_ROI_Y_LSB], CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    }

    result_sw = &pH26XRegCfg->uiH26X_YMSE_MOTION_Low;
	for (i = H26X_PSNR_MOTION_Y_LSB; i <=  H26X_PSNR_MOTION_V_MSB; i++)
	{
        item_id = i;
        result &= emu_cmp( result_sw[i-H26X_PSNR_MOTION_Y_LSB], CHK_REPORT_2[item_id], check, (UINT8*)&item_name_2[item_id]);
    }

    result_sw = &pH26XRegCfg->uiH26X_YMSE_BGR_Low;
	for (i = H26X_PSNR_BGR_Y_LSB; i <=  H26X_PSNR_BGR_V_MSB; i++)
	{
        item_id = i;
        result &= emu_cmp( result_sw[i-H26X_PSNR_BGR_Y_LSB], CHK_REPORT_2[item_id], check, (UINT8*)&item_name_2[item_id]);
    }

    return result;
}

static BOOL emu_h265_compare_sideinfo(h26x_job_t *p_job)
{
    BOOL bRet = TRUE;

#if (EMU_CMP_SIDEINFO==0 && H26X_EMU_NROUT_ONLY == 0)

	h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
	h265_emu_t *emu = &ctx->emu;
	H26XRegSet	 *pH26XRegSet = emu->pH26XRegSet;
	UINT8 *pucHwIlfInfoAddr[H26X_MAX_TILE_NUM];
	pucHwIlfInfoAddr[0] = (UINT8*)pH26XRegSet->SIDE_INFO_WR_ADDR_0;
	h26x_flushCacheRead((UINT32)pucHwIlfInfoAddr[0], emu->IlfSideInfo_Mall_Size); //hk

#elif EMU_CMP_SIDEINFO
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
  h265_pat_t *pat = &ctx->pat;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    //H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
    H26XRegSet   *pH26XRegSet = emu->pH26XRegSet;
    //UINT32  iIlfSideInfo_Size = ((uiHeight+63)/64) * ((uiWidth+63)/64) * 8;
    UINT32 i,j,k,uiTileNum = 1;
    UINT32 uiTileLineoffset[H26X_MAX_TILE_NUM];
    UINT32 uiTileLineoffset_cmodel[H26X_MAX_TILE_NUM];
    UINT8 *pucSwIlfInfoAddr[H26X_MAX_TILE_NUM],*pucHwIlfInfoAddr[H26X_MAX_TILE_NUM];
    UINT8 ErrCnt = 0,compare_pointnum = 5;
    UINT32 sideinfo_out_en = (pH26XRegCfg->uiH26XPic0 >> 6) & 0x1;

    pucSwIlfInfoAddr[0] = (UINT8*)emu->uiVirSwIlfInfoAddr;
    //pucHwIlfInfoAddr[0] = (UINT8*)pH26XVirEncAddr->uiIlfSideInfoAddr[pH26XVirEncAddr->uiIlfWIdx];
    pucHwIlfInfoAddr[0] = (UINT8*)pH26XRegSet->SIDE_INFO_WR_ADDR_0;
    h26x_flushCacheRead((UINT32)pucHwIlfInfoAddr[0], emu->IlfSideInfo_Mall_Size);

    if( emu_h265_read_sideinfo(emu) != H26X_EMU_PASS) return FALSE;

    if(sideinfo_out_en == 0) return TRUE;

    //DBG_INFO("dump_image rec_y.dat 0x%08x 0x%08x \r\n",pucHwRecYAddr,uiSrcFrmSize);
    //DBG_INFO("dump_image rec_c.dat 0x%08x 0x%08x \r\n",pucHwRecUVAddr,uiSrcFrmSize/2);
    //DBG_INFO("dump_image sideinfo.dat 0x%08x 0x%08x \r\n",pucHwIlfInfoAddr,iIlfSideInfo_Size);
    uiTileLineoffset[0] = emu->uiIlfLineOffset;
    uiTileLineoffset_cmodel[0] = emu->uiIlfLineOffset_cmodel;

    if(emu->uiTileNum)
    {
        UINT32 sum1 = 0,sum2 = 0;
        uiTileNum = emu->uiTileNum+1;
        uiTileLineoffset[0] = pH26XEncPicCfg->uiIlfLineOffset;
        uiTileLineoffset_cmodel[0] = ((emu->uiTileWidth[0]+63)/64) * 8;
        sum1 = uiTileLineoffset_cmodel[0];
        sum2 = uiTileLineoffset[0];

        k = 0;
        //DBG_INFO("uiTileLineoffset[%d] : %d, %d\r\n",k,uiTileLineoffset[k],uiTileLineoffset_cmodel[k]);
        for(i = 0; i < emu->uiTileNum;i++)
        {
            uiTileLineoffset[i+1] = pH26XEncPicCfg->uiTileIlfLineOffset[i];
            uiTileLineoffset_cmodel[i+1] = ((emu->uiTileWidth[i+1]+63)/64) * 8;
            pucSwIlfInfoAddr[i+1] = (UINT8*)((UINT32)pucSwIlfInfoAddr[0]+(sum1 * ((emu->uiHeight+63)/64)));
            pucHwIlfInfoAddr[i+1] = (UINT8*)((UINT32)pucHwIlfInfoAddr[0]+(sum2 * ((emu->uiHeight+63)/64)));
            sum1 += uiTileLineoffset_cmodel[i+1];
            sum2 += uiTileLineoffset[i+1];
            k = i+1;
            //DBG_INFO("uiTileLineoffset[%d] : %d, %d, 0x%08x, 0x%08x (0%08x 0%08x)\r\n",
            //    k,uiTileLineoffset[k],uiTileLineoffset_cmodel[k],
            //    pucSwIlfInfoAddr[i+1],pucHwIlfInfoAddr[i+1],(sum1 * ((emu->uiHeight+63)/64)),(sum2 * ((emu->uiHeight+63)/64)));
        }
        /*
        uiTileLineoffset[i+1] = pH26XEncPicCfg->uiTileIlfLineOffset[i];
        uiTileLineoffset_cmodel[i+1] = emu->uiIlfLineOffset_cmodel - sum1;
        pucSwIlfInfoAddr[i+1] = (UINT8*)((UINT32)pucSwIlfInfoAddr[0]+(sum1 * ((emu->uiHeight+63)/64));
        pucHwIlfInfoAddr[i+1] = (UINT8*)((UINT32)pucHwIlfInfoAddr[0]+(sum2 * ((emu->uiHeight+63)/64));
        k = i+1;
        DBG_INFO("uiTileLineoffset[%d] : %d, %d\r\n",k,uiTileLineoffset[k],uiTileLineoffset_cmodel[k]);
        */
    }

    for(k=0;k<uiTileNum;k++)
    {
        //DBG_INFO("uiTileLineoffset[%d] : %d, %d\r\n",k,uiTileLineoffset[k],uiTileLineoffset_cmodel[k]);
        for (i=0;i<((emu->uiHeight+63)/64);i++)
        {
            for(j=0;j<uiTileLineoffset_cmodel[k];j++)
            {
                if (*(pucSwIlfInfoAddr[k] + j) != *(pucHwIlfInfoAddr[k] + j))
                {
                    DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare ILF Side Info Error!\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
                    DBG_INFO("[%d][%d][%d] SW:0x%02x , HW:0x%02x\r\n",(int)k,(int)i,(int)j,(int)(*(pucSwIlfInfoAddr[k] + j)),(int)(*(pucHwIlfInfoAddr[k] + j)));
                    bRet = FALSE;
                    ErrCnt++;
                    if(ErrCnt > compare_pointnum)
                        break;
                }
            }
            pucSwIlfInfoAddr[k] += uiTileLineoffset_cmodel[k];
            pucHwIlfInfoAddr[k] += uiTileLineoffset[k];
        }
        ErrCnt = 0;
        if(bRet == FALSE) break;
    }
    if(bRet == TRUE){
        //DBG_INFO("Compare SIDEINFO PASS\r\n");
    }
#endif
    return bRet;
}

BOOL emu_h265_compare_bs(h26x_job_t *p_job,UINT32 compare_bs_size)
{
    BOOL bRet = TRUE;

#if (EMU_CMP_BS == 0 && EMU_CMP_BS_CHKSUM == 0 && H26X_EMU_NROUT_ONLY == 0)

	h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
	h265_emu_t *emu = &ctx->emu;
	H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
	h26x_flushCacheRead((UINT32)pH26XVirEncAddr->uiHwBsAddr,emu->uiHwBsMaxLen); //hk

#elif (EMU_CMP_BS == 1 || EMU_CMP_BS_CHKSUM == 1)
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = &ctx->pat;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
    UINT32 i = 0,j = 0;
	#if EMU_CMP_BS
    UINT8 *pucSwBsAddr = (UINT8 *)pH26XVirEncAddr->uiPicBsAddr;
	UINT8 ErrCnt = 0,compare_pointnum = 5;
	#endif
	UINT8 *pucHwBsAddr = (UINT8 *)pH26XVirEncAddr->uiHwBsAddr;
	#if EMU_CMP_BS_CHKSUM
	UINT32 uiCS = 0; 
	#endif
	
    //compare_bs_size = pH26XRegCfg->uiH26XBsLen;

    //DBG_INFO("%s %d\r\n",__FUNCTION__,__LINE__);
    if(H26X_EMU_TEST_RAN_SLICE == 0 && H26X_EMU_TEST_BSOUT == 0)
    {
		h26x_flushCacheRead((UINT32)pucHwBsAddr,emu->uiHwBsMaxLen); //hk

		#if 0 //compare heavyload chksum
		{
			H26XEncDramInfo *pH26XEncDramInfo = &emu->sH26XEncDramInfo;
			UINT32 hw_dram_chksum=0;
            //UINT32 sw_dram_chksum=0;
            hw_dram_chksum = h26x_getDramChkSum(pH26XVirEncAddr->uiHwBsAddr, (compare_bs_size+3)/4*4);// 2 byte as unit, 4 byte align
			//DBG_INFO("hw_dram_chksum 0x%08x\r\n" ,(unsigned int)hw_dram_chksum);

            //sw_dram_chksum = h26x_calDramChkSum(pH26XVirEncAddr->uiHwBsAddr, (compare_bs_size+3)/4*4);
			//DBG_INFO("sw_dram_chksum 0x%08x\r\n" ,(unsigned int)sw_dram_chksum);

			DBG_INFO("uiH26XBsDramSum 0x%08x\r\n" ,(unsigned int)pH26XEncDramInfo->uiH26XBsDramSum);

			if(hw_dram_chksum != pH26XEncDramInfo->uiH26XBsDramSum){
				DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare Bitstream Dram Error!\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
				DBG_INFO("compare_bs_size = 0x%08x compare_bs_dma_sum sw:0x%08x, hw:0x%08x fail\r\n" ,(unsigned int)compare_bs_size ,(unsigned int)pH26XEncDramInfo->uiH26XBsDramSum, (unsigned int)hw_dram_chksum);
				bRet = FALSE;
			}
		}
		#endif

		#if EMU_CMP_DRAM_CHKSUM
		{
			H26XEncDramInfo *pH26XEncDramInfo = &emu->sH26XEncDramInfo;
			UINT32 uiCS=0;
			UINT32 *pucHw = (UINT32 *)pH26XVirEncAddr->uiHwBsAddr;
			//UINT32 st = (unsigned int)hwclock_get_counter();
			
			for (i=0;i< (compare_bs_size+3)/4;i++){
				uiCS += pucHw[i];
			}

			if(uiCS != pH26XEncDramInfo->uiH26XBsDramSum){
				DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare Bitstream Dram Error!\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
				DBG_INFO("compare_bs_size = 0x%08x compare_bs_dma_sum sw:0x%08x, hw:0x%08x fail\r\n" ,(unsigned int)compare_bs_size ,(unsigned int)pH26XEncDramInfo->uiH26XBsDramSum, (unsigned int)uiCS);
				bRet = FALSE;
			}
			else{
				//DBG_INFO("compare_bs_size pass\r\n");
			}

			//DBG_INFO("emu_h265_compare_bs time = %d\r\n", (unsigned int) hwclock_get_counter() - (unsigned int) st);
		}
		#endif
		
		for (i=0;i<compare_bs_size;i++){
            j = i;

			#if EMU_CMP_BS
            if (*(pucSwBsAddr + i) != *(pucHwBsAddr + j)){
                DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare Bitstream Error!\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
                DBG_INFO("Bitstream offset: (0x%08x 0x%08x) (SW 0x%08x,HW 0x%08x)\r\n",(int)i,(int)j,(int)(pucSwBsAddr + i),(int)(pucHwBsAddr + j));
                DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",(int)(*(pucSwBsAddr + i)),(int)(*(pucHwBsAddr + j)));
                //DBG_INFO("SW Addr:0x%08x, HW Addr:0x%08x\r\n",pH26XVirEncAddr->uiPicBsAddr,pH26XVirEncAddr->uiHwBsAddr);
                //h26xPrtMem((UINT32)pucSwBsAddr+i,0x10);
                //h26xPrtMem((UINT32)pucHwBsAddr+i,0x10);
                bRet = FALSE;
                ErrCnt++;
                if(ErrCnt > compare_pointnum)
                    break;
            }
			#endif

			#if EMU_CMP_BS_CHKSUM
			uiCS += emu_bit_reverse(pucHwBsAddr[j]);
			#endif
			
        }

		#if EMU_CMP_BS_CHKSUM
		if(uiCS != pH26XRegCfg->uiH26XBsChkSum){
			DBG_INFO("\r\n");
			DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare BS Check Sum Error!\r\n", (int)pat->uiSrcNum, (int)pat->uiPatNum, (int)pat->uiPicNum);
			DBG_INFO("SW:0x%08x, HW:0x%08x\r\n", (unsigned int)pH26XRegCfg->uiH26XBsChkSum, (unsigned int)uiCS);
			bRet = FALSE;
		}
		else{
			DBG_INFO("Compare BS CHKSUM PASS (0x%08x)\r\n",(int)pH26XRegCfg->uiH26XBsLen);
		}
		#endif

		#if EMU_CMP_BS
        if(bRet == TRUE){
            DBG_INFO("Compare BS PASS (0x%08x)\r\n",(int)pH26XRegCfg->uiH26XBsLen);
        }
        else{
            bRet = FALSE;
            DBG_INFO("Compare BS FAIL (0x%08x)\r\n",(int)pH26XRegCfg->uiH26XBsLen);
            h26x_prtMem((UINT32)pucSwBsAddr+i,0x30);
            h26x_prtMem((UINT32)pucHwBsAddr+j,0x30);
        }
		#endif
		
    }
    else if(H26X_EMU_TEST_RAN_SLICE == 0 && H26X_EMU_TEST_BSOUT != 0)
    {
        UINT32 k = 0,j = 0;
        DBG_INFO("%s %d 0x%08x 0x%08x\r\n",__FUNCTION__,__LINE__,(unsigned int)pucHwBsAddr,(unsigned int)compare_bs_size);

		h26x_flushCacheRead((UINT32)pucHwBsAddr, SIZE_32X(  compare_bs_size + (compare_bs_size +  (emu->uiBsSegmentSize - 1)) /emu->uiBsSegmentSize *	   emu->uiBsSegmentOffset)) ; //hk

		
        for (i=0;i<compare_bs_size;i++){
            if(k == emu->uiBsSegmentSize)
            {
                j += emu->uiBsSegmentOffset;
                k = 0;
                //DBG_INFO("%s %d, i = 0x%08x 0x%08x\r\n",__FUNCTION__,__LINE__,i,j);
            }
			#if EMU_CMP_BS
            if (*(pucSwBsAddr + i) != *(pucHwBsAddr + j)){
                DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare Bitstream Error!\r\n",(int)src_num,(int)pat_num,(int)pic_num);
                DBG_INFO("Bitstream offset: (0x%08x 0x%08x) (SW 0x%08x,HW 0x%08x)\r\n",(int)i,(int)j,(int)(pucSwBsAddr + i),(int)(pucHwBsAddr + j));
                DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",(int)(*(pucSwBsAddr + i)),(int)(*(pucHwBsAddr + j)));
                DBG_INFO("SW Addr:0x%08x, HW Addr:0x%08x\r\n",(int)pH26XVirEncAddr->uiPicBsAddr,(int)pH26XVirEncAddr->uiHwBsAddr);
                h26x_prtMem((UINT32)pucSwBsAddr+i,0x10);
                h26x_prtMem((UINT32)pucHwBsAddr+j,0x10);
                bRet = FALSE;
                ErrCnt++;
                if(ErrCnt > compare_pointnum)
                    break;
            }
			#endif
			#if EMU_CMP_BS_CHKSUM
			uiCS += emu_bit_reverse(pucHwBsAddr[j]);
			#endif
            k++;
            j++;
        }
		
		#if EMU_CMP_BS_CHKSUM
		if(uiCS != pH26XRegCfg->uiH26XBsChkSum){
			DBG_INFO("\r\n");
			DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare BS Check Sum Error!\r\n", (int)pat->uiSrcNum, (int)pat->uiPatNum, (int)pat->uiPicNum);
			DBG_INFO("SW:0x%08x, HW:0x%08x\r\n", (unsigned int)pH26XRegCfg->uiH26XBsChkSum, (unsigned int)uiCS);
			bRet = FALSE;
		}
		else{
			DBG_INFO("Compare BS CHKSUM PASS (0x%08x)\r\n",(int)pH26XRegCfg->uiH26XBsLen);
		}
		#endif
		
		#if EMU_CMP_BS
        if(bRet == TRUE){
            DBG_INFO("Compare BS PASS (0x%08x)\r\n",(int)compare_bs_size);
        }else{
            DBG_INFO("Compare BS FAIL (0x%08x)\r\n",(int)compare_bs_size);
            h26x_prtMem((UINT32)pucSwBsAddr+i,0x30);
            h26x_prtMem((UINT32)pucHwBsAddr+j,0x30);
        }
		#endif
    }
#endif
    return bRet;
}

void tile_read_checksum(h26x_job_t *p_job)
{

    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    UINT32 y_chk_sum = 0;
    UINT32 c_chk_sum = 0;
    UINT32 uiExtraRecSize = (emu->uiHeight+63)/64*64*128;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;

    DBG_ERR("R uiRecExtraYAddr = 0x%08x 0x%08x\r\n",(unsigned int)pH26XVirEncAddr->uiRecExtraYAddr[pH26XVirEncAddr->uiRef0Idx],(unsigned int)pH26XVirEncAddr->uiRecExtraCAddr[pH26XVirEncAddr->uiRef0Idx]);
    y_chk_sum = emu_buf_chk_sum((UINT8 *)pH26XVirEncAddr->uiRecExtraYAddr[pH26XVirEncAddr->uiRef0Idx], uiExtraRecSize, 1);
    c_chk_sum = emu_buf_chk_sum((UINT8 *)pH26XVirEncAddr->uiRecExtraCAddr[pH26XVirEncAddr->uiRef0Idx], uiExtraRecSize, 1);
    DBG_ERR("y_chk_sum 0x%08x 0x%08x\r\n",(unsigned int)y_chk_sum,(unsigned int)c_chk_sum);
    DBG_ERR("R uiIlfExtraSideInfoAddr = 0x%08x\r\n",(unsigned int)pH26XVirEncAddr->uiIlfExtraSideInfoAddr[pH26XVirEncAddr->uiIlfRIdx]);
    y_chk_sum = emu_buf_chk_sum((UINT8 *)pH26XVirEncAddr->uiIlfExtraSideInfoAddr[pH26XVirEncAddr->uiIlfRIdx], emu->IlfSideInfo_Mall_Size, 1);
    DBG_ERR("y_chk_sum 0x%08x\r\n",(unsigned int)y_chk_sum);

}
void tile_write_checksum(h26x_job_t *p_job)
{

    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    UINT32 y_chk_sum = 0;
    UINT32 c_chk_sum = 0;
    UINT32 uiExtraRecSize = (emu->uiHeight+63)/64*64*128;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    DBG_INFO("W uiRecExtraYAddr = 0x%08x 0x%08x\r\n",(unsigned int)pH26XVirEncAddr->uiRecExtraYAddr[pH26XVirEncAddr->uiRecIdx],(unsigned int)pH26XVirEncAddr->uiRecExtraCAddr[pH26XVirEncAddr->uiRecIdx]);
    y_chk_sum = emu_buf_chk_sum((UINT8 *)pH26XVirEncAddr->uiRecExtraYAddr[pH26XVirEncAddr->uiRecIdx], uiExtraRecSize, 1);
    c_chk_sum = emu_buf_chk_sum((UINT8 *)pH26XVirEncAddr->uiRecExtraCAddr[pH26XVirEncAddr->uiRecIdx], uiExtraRecSize, 1);
    DBG_INFO("y_chk_sum 0x%08x 0x%08x\r\n",(unsigned int)y_chk_sum,(unsigned int)c_chk_sum);
    DBG_INFO("W uiIlfExtraSideInfoAddr = 0x%08x\r\n",(unsigned int)pH26XVirEncAddr->uiIlfExtraSideInfoAddr[pH26XVirEncAddr->uiIlfWIdx]);
    y_chk_sum = emu_buf_chk_sum((UINT8 *)pH26XVirEncAddr->uiIlfExtraSideInfoAddr[pH26XVirEncAddr->uiIlfWIdx], emu->IlfSideInfo_Mall_Size, 1);
    DBG_INFO("y_chk_sum 0x%08x\r\n",(unsigned int)y_chk_sum);

}

//#if EMU_CMP_COLMV
static BOOL chk_col_mv(h26x_job_t *p_job){
	BOOL bRet = TRUE;


	#if (EMU_CMP_COLMV == 0 && H26X_EMU_NROUT_ONLY == 0)
	h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
	h265_emu_t *emu = &ctx->emu;

	H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
	UINT8 *pucHwAddr = (UINT8 *)pH26XVirEncAddr->uiColMvsAddr[pH26XVirEncAddr->uiColMvWIdx];
	H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;

	if( ((pRegSet->PIC_CFG>>5)& 1) == 0){//colmv_wt en
		return bRet;
	}
	else{
		h26x_flushCacheRead((UINT32)pucHwAddr,emu->ColMvs_Size); //hk
	}

	#elif EMU_CMP_COLMV
	h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
	h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = &ctx->pat;
	H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
	UINT32 i;
	UINT32 recErrCnt = 0;

	h26xFileReadFlush(&emu->file.colmv, emu->ColMvs_Size, p_job->tmp_colmv_addr);

	H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
	UINT8 *pucSwAddr = (UINT8 *)p_job->tmp_colmv_addr;
	UINT8 *pucHwAddr = (UINT8 *)pH26XVirEncAddr->uiColMvsAddr[pH26XVirEncAddr->uiColMvWIdx];
	if( ((pRegSet->PIC_CFG>>5)& 1) == 0){//colmv_wt en
		return bRet;
	}
	else{
		h26x_flushCacheRead((UINT32)pucHwAddr,emu->ColMvs_Size); //hk
	}

	for (i = 0; i < emu->ColMvs_Size; i++) {
		if(pucSwAddr[i]!= pucHwAddr[i] ){
			DBG_INFO("Job(%d) H265 Src[%d]Pat[%d]Pic[%d] compare colmv error!\r\n", (int)p_job->idx1, (int)pat->uiSrcNum, (int)pat->uiPatNum, (int)pat->uiPicNum);
			DBG_INFO("Error at i = %d\r\n", (int)i);
            DBG_INFO("SW:0x%02x , HW:0x%02x diff\r\n",(unsigned int)pucSwAddr[i],(unsigned int)pucHwAddr[i]);
            bRet = FALSE;
            recErrCnt++;
            if(recErrCnt > 20){
            	#if 0
				emuh26x_msg(("dump sw data\r\n"));
				h26x_prtMem(p_job->tmp_colmv_addr, emu->ColMvs_Size);
				emuh26x_msg(("dump hw data\r\n"));
				emuh26x_msg(("colmv_addr=%08x\r\n",h26x_getPhyAddr(pH26XVirEncAddr->uiColMvsAddr[pH26XVirEncAddr->uiColMvWIdx])));
				h26x_prtMem(pH26XVirEncAddr->uiColMvsAddr[pH26XVirEncAddr->uiColMvWIdx], emu->ColMvs_Size);
				#endif
				return bRet;
			}
		}
		#if 0
		else{
			if(i==1008 || i==1009 || i==1010){
				DBG_INFO("Job(%d) H265 Src[%d]Pat[%d]Pic[%d] compare colmv Correct!\r\n", (int)p_job->idx1, (int)pat->uiSrcNum, (int)pat->uiPatNum, (int)pat->uiPicNum);
							DBG_INFO(" at i = %d\r\n", (int)i);
							DBG_INFO("SW:0x%02x , HW:0x%02x \r\n",(unsigned int)pucSwAddr[i],(unsigned int)pucHwAddr[i]);
			}
		}
		#endif
	}


	if(recErrCnt==0)DBG_INFO("Job(%d) H265 Src[%d]Pat[%d]Pic[%d] compare colmv pass!\r\n",(int) p_job->idx1,(int) pat->uiSrcNum, (int)pat->uiPatNum,(int) pat->uiPicNum);

	#endif

	return bRet;

}
//#endif

// Check Resutlt //
static BOOL EmuH26XEncCheckResult(h26x_job_t *p_job, UINT32 interrupt){
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = &ctx->pat;
    H26XEncSeqCfg *pH26XEncSeqCfg = &emu->sH26XEncSeqCfg;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
//  UINT32 uiPicType = pH26XRegCfg->uiH26XPic0 & 0x1;
    UINT32 uiSrcNum = pat->uiSrcNum,uiPatNum = pat->uiPatNum,uiPicNum = pat->uiPicNum;
    UINT32 i,item_id;
    BOOL result = TRUE, result2 = TRUE;
    UINT32 check;
    //UINT32 smartrec_en;
    UINT32  *result_sw;
#if DIRECT_MODE
    H26XRegSet   *pH26XRegSet = emu->pH26XRegSet;
    unsigned int *CHK_REPORT = (unsigned int *)pH26XRegSet->CHK_REPORT;
	unsigned int *CHK_REPORT_2 = (unsigned int *)pH26XRegSet->CHK_REPORT_2;

//hk:disable_codec_flow return TRUE;


#else


#if 0
    unsigned int *CHK_REPORT = (unsigned int *)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 0);
	unsigned int *CHK_REPORT_2 = (unsigned int *)(p_job->apb_addr + H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 0);
//dma_getNonCacheAddr
#else //hk tmp hack
unsigned int *CHK_REPORT = (unsigned int *)(p_job->check1_addr);

//unsigned int *CHK_REPORT_2 = (unsigned int *)(p_job->check2_addr);
unsigned int *CHK_REPORT_2 = (unsigned int *)(p_job->apb_addr + H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 0);

//DBG_INFO("report addr 0x%08x 0x%08x\r\n",(unsigned int)p_job->check1_addr, (unsigned int)p_job->check2_addr );
h26x_flushCacheRead((UINT32)p_job->check1_addr, H26X_REG_REPORT_SIZE); //hk/

h26x_flushCacheRead(p_job->apb_addr + H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET, H26X_REG_REPORT_SIZE); //hk
//h26x_flushCacheRead((UINT32)p_job->check2_addr , 64 * 4 + 32); //hk/

#endif


#if 0
h26x_flushCacheRead((UINT32)p_job->apb_addr, H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET + H26X_REPORT_2_NUMBER * 4); //hk

#endif

#if 0
if(uiPicNum==0){
for(i=0;i<1;i++){
	DBG_INFO("delay1\n");
	sleep(1);
	h26x_prtMem((unsigned int)CHK_REPORT, 64*4);
}
}
#endif

#if 0
//hk tmp
//sleep(1);
//h26x_prtMem((UINT32)CHK_REPORT, 64*4);
h26x_flushCacheRead((UINT32)CHK_REPORT - 16 , H26X_REPORT_NUMBER * 4 + 4); //hk/
h26x_flushCacheRead((UINT32)CHK_REPORT_2, H26X_REPORT_2_NUMBER * 4); //hk

DBG_INFO("CHK_REPORT 0x%08x 0x%08x 0x%08x\r\n",(unsigned int)CHK_REPORT,(unsigned int)CHK_REPORT - 16,(unsigned int)(CHK_REPORT - 16));


h26x_prtMem((UINT32)CHK_REPORT, 64*4);

#endif

//DBG_INFO("CHK_REPORT 0x%08x 0x%08x 0x%08x\r\n",(unsigned int)CHK_REPORT,(unsigned int)h26x_getVirAddr((UINT32)CHK_REPORT),(unsigned int)h26x_getPhyAddr((UINT32)CHK_REPORT));
#endif


	{
		H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
 	    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
		UINT32 Y_Size;
		#if (H26X_EMU_SRC_LINEOFFSET || H26X_EMU_REC_LINEOFFSET)
    		Y_Size = ((H26X_LINEOFFSET_MAX+63)>>6) * ((H26X_LINEOFFSET_MAX+63)>>6) * 64 * 64;
		#else
    		Y_Size = emu->uiFrameSize;
		#endif
		if( (pRegSet->PIC_CFG>>4) & 1 ){ //REC_OUT_EN
 			h26x_flushCacheRead(pH26XVirEncAddr->uiRefAndRecYAddr[pH26XVirEncAddr->uiRecIdx], Y_Size);
	 		h26x_flushCacheRead(pH26XVirEncAddr->uiRefAndRecUVAddr[pH26XVirEncAddr->uiRecIdx], (Y_Size / 2));
		}
		if( (pRegSet->RRC_CFG[0]) & 3 ){ //RRC_EN
			h26x_flushCacheRead(pRegSet->RRC_WR_ADDR, emu->uiRCSize);
		}
	}

    src_num = pat->uiSrcNum;
    pat_num = pat->uiPatNum;
    pic_num = pat->uiPicNum;

#if 0
    DBG_INFO("job(%d,%d) report\r\n", p_job->idx1, p_job->idx2);
    DBG_INFO("report 1 : %p\r\n", (p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 0));
    h26x_prtMem((UINT32)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 0), sizeof(UINT32)*H26X_REPORT_NUMBER);
#endif

    if ((interrupt & H26X_FINISH_INT) == 0){
        DBG_INFO("Src[%d] Pat[%d] Pic[%d] Error Interrupt[0x%08x]!\r\n",(int)uiSrcNum,(int)uiPatNum,(int)uiPicNum,(int)interrupt);
        result = FALSE;
    }

    item_id = H26X_BS_CHKSUM;
    result &= emu_cmp( pH26XRegCfg->uiH26XBsChkSum, CHK_REPORT[item_id], 1, (UINT8*)&item_name[item_id]);

	if((pH26XRegCfg->uiH26XFunc0>>6)&1){ //frame skip mode only check bs
		item_id = H26X_BS_LEN;
		check = 1;
		result &= emu_cmp( pH26XRegCfg->uiH26XBsLen, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
		if( CHK_REPORT[item_id] > emu->uiHwBsMaxLen)
		{
			DBG_INFO("error!!! bslen 0x%08x > uiHwBsMaxLen (0x%08x)\r\n",(int)CHK_REPORT[item_id],(int)emu->uiHwBsMaxLen);
		}
		result2 &= emu_h265_compare_nal(p_job,emu->uiSliceNum);
		result2 &= emu_h265_compare_bs(p_job,pH26XRegCfg->uiH26XBsLen);
		goto Rep;
	}


    item_id = H26X_REC_CHKSUM;
    check = pH26XRegCfg->uiH26XPic0 & 0x10;
    if( (pH26XRegCfg->uiH26XFunc0 >> 11) & 1){
        //DBG_INFO("grey case!\r\n");
        //result2 &= emu_cmp( pH26XRegCfg->uiH26XRecChkSum, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    }else{
        result &= emu_cmp( pH26XRegCfg->uiH26XRecChkSum, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    }


    item_id = H26X_IME_CHKSUM_LSB;
    check = (((emu->uiWidth/64*64) == emu->uiWidth) && ((emu->uiHeight/64*64) == emu->uiHeight));
    result2 &= emu_cmp( pH26XRegCfg->uiH26X_ImeChkSum_lo, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    item_id = H26X_IME_CHKSUM_MSB;
    check = (((emu->uiWidth/64*64) == emu->uiWidth) && ((emu->uiHeight/64*64) == emu->uiHeight));
    result2 &= emu_cmp( pH26XRegCfg->uiH26X_ImeChkSum_hi, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);


    if((pH26XRegCfg->uiH26XTile0 & 0x7) == 0)
    {
        //Slash : if tile enable, cmodel's ec checkum is the sum of the last tile, not all frame
        item_id = H26X_EC_CHKSUM;
        check = 1;
        result2 &= emu_cmp( pH26XRegCfg->uiH26X_ECChkSum, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    }
#if 1
    item_id = H26X_SRC_Y_DMA_CHKSUM;
    result2 &= emu_cmp( pH26XRegCfg->uiH26X_SrcChkSum_Y, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    item_id = H26X_SRC_C_DMA_CHKSUM;
    result2 &= emu_cmp( pH26XRegCfg->uiH26X_SrcChkSum_C, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
#endif
#if 1
    result_sw = &pH26XRegCfg->uiH26X_OSG_Y0_SUM;
	for (i = H26X_OSG_0_Y_CHKSUM; i <=  H26X_OSG_0_C_CHKSUM; i++)
	{
        item_id = i;
        result2 &= emu_cmp( result_sw[i-H26X_OSG_0_Y_CHKSUM], CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
        //emu_printf( result_sw[i-H26X_OSG_0_Y_CHKSUM], CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    }
    result_sw = &pH26XRegCfg->uiH26X_OSG_Y_SUM;
	for (i = H26X_OSG_Y_CHKSUM; i <=  H26X_OSG_C_CHKSUM; i++)
	{
        item_id = i;
        result2 &= emu_cmp( result_sw[i-H26X_OSG_Y_CHKSUM], CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
        //emu_printf( result_sw[i-H26X_OSG_Y_CHKSUM], CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    }
#endif

    item_id = H26X_SIDE_INFO_CHKSUM;
    check = (emu->uiEcEnable == 1 && (pH26XRegCfg->uiH26XPic0 & 0x10));
    result2 &= emu_cmp( pH26XRegCfg->uiH26X_SideInfoChkSum, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);

    item_id = H26X_SCD_REPORT;
    check = 1;
    result &= emu_cmp( (pH26XRegCfg->uiH26X_HW_INTER_SCD > 0 || pH26XRegCfg->uiH26X_HW_IRAnG_SCD > 0), (CHK_REPORT[item_id]&0x1), check, (UINT8*)&item_name[item_id]);
    item_id = H26X_BS_LEN;
    check = 1;
    result &= emu_cmp( pH26XRegCfg->uiH26XBsLen, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    if( CHK_REPORT[item_id] > emu->uiHwBsMaxLen && H26X_EMU_TEST_BSOUT == 0)
    {
        DBG_INFO("error!!! bslen 0x%08x > uiHwBsMaxLen (0x%08x)\r\n",(int)CHK_REPORT[item_id],(int)emu->uiHwBsMaxLen);
    }
    result2 &= emu_h265_compare_rc(p_job);
    //result &= emu_h265_compare_rc(p_job);

    check = 1;
    result_sw = &pH26XRegCfg->uiH26X_stat_Num_INTER;
	for (i = H26X_STATS_INTER_CNT; i <=  H26X_STATS_MERGE_CNT; i++)
	{
        item_id = i;
        result2 &= emu_cmp( result_sw[i-H26X_STATS_INTER_CNT], CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    }

    result_sw = &pH26XRegCfg->uiH26X_stat_Num_IRA8;
	for (i = H26X_STATS_IRA8_CNT; i <=  H26X_STATS_CU16_CNT; i++)
	{
        item_id = i;
        result2 &= emu_cmp( result_sw[i-H26X_STATS_IRA8_CNT], CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    }
    check = (pH26XRegCfg->uiH26X_JND_0 & 0x1);
    result_sw = &pH26XRegCfg->uiH26X_JND_Y_SUM;
    for (i = H26X_JND_Y_CHKSUM; i <=  H26X_JND_C_CHKSUM; i++)
    {
         item_id = i;
         result2 &= emu_cmp( result_sw[i-H26X_JND_Y_CHKSUM], CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
         //emu_printf( result_sw[i-H26X_JND_Y_CHKSUM], CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    }


#if H26X_EMU_SMARTREC
    smartrec_en = emu->uiSmartRecEn;
    item_id = H26X_CRC_HIT_Y_CNT;
    check = ((smartrec_en == 1) && ((pH26XRegCfg->uiH26XPic0 & 0x1) != 0);
    result2 &= emu_cmp( pH26XRegCfg->uiH26X_CRC_Y_HIT_SUM, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    item_id = H26X_CRC_HIT_C_CNT;
    result2 &= emu_cmp((pH26XRegCfg->uiH26X_CRC_YC_HIT_SUM - pH26XRegCfg->uiH26X_CRC_Y_HIT_SUM), CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);

    item_id = H26X_EC_REAL_LEN_SUM;
    check = (pH26XRegCfg->uiH26XPic0 & 0x10);
    if(smartrec_en == 0 || ((pH26XRegCfg->uiH26XPic0 & 0x1 ) == 0) )
    {
        pH26XRegCfg->EC_RealLenSum += pH26XRegCfg->EC_SkipLenSum;
        pH26XRegCfg->EC_SkipLenSum = 0;
    }
    //DBG_INFO("smartrec_en = %d, pictype = %d\r\n",smartrec_en,(pH26XRegCfg->uiH26XPic0 & 0x1 ));
    //DBG_INFO("EC_RealLenSum SW:0x%08x, HW:0x%08x\r\n",pH26XRegCfg->EC_RealLenSum,CHK_REPORT[H26X_EC_REAL_LEN_SUM] * 4);
    //DBG_INFO("EC_SkipLenSum SW:0x%08x, HW:0x%08x\r\n",pH26XRegCfg->EC_SkipLenSum,CHK_REPORT[H26X_EC_SKIP_LEN_SUM] * 4);
    result2 &= emu_cmp( pH26XRegCfg->EC_RealLenSum, CHK_REPORT[item_id] * 4, check, (UINT8*)&item_name[item_id]);
    item_id = H26X_EC_SKIP_LEN_SUM;
    result2 &= emu_cmp(pH26XRegCfg->EC_SkipLenSum, CHK_REPORT[item_id] * 4, check, (UINT8*)&item_name[item_id]);
#endif
    item_id = H26X_ROI_CNT;
    check = 1;
    result2 &= emu_cmp( pH26XRegCfg->uiH26X_ROI_NUMBER, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);

    item_id = H26X_MOTION_NUM;
    check = 1;
    result2 &= emu_cmp( pH26XRegCfg->uiH26X_MOTION_NUMBER, CHK_REPORT_2[item_id], check, (UINT8*)&item_name_2[item_id]);

    item_id = H26X_BGR_NUM;
    check = 1;
    result2 &= emu_cmp( pH26XRegCfg->uiH26X_BGR_NUMBER, CHK_REPORT_2[item_id], check, (UINT8*)&item_name_2[item_id]);

    item_id = H26X_JND_GRAD;
    check = 1;
    result2 &= emu_cmp( pH26XRegCfg->uiH26X_JND_GRAD, CHK_REPORT_2[item_id], check, (UINT8*)&item_name_2[item_id]);

    item_id = H26X_JND_GRAD_CNT;
	check = 1;
    result2 &= emu_cmp( pH26XRegCfg->uiH26X_JND_GRAD_CNT, CHK_REPORT_2[item_id], check, (UINT8*)&item_name_2[item_id]);

	if( (pH26XRegCfg->uiH26XPic0 & 0x1) == 1 && (pH26XRegCfg->uiH26XMaq0 & 0x3) > 0){
    item_id = H26X_SCD_REPORT;
	check = 1;
    result2 &= emu_cmp( pH26XRegCfg->uiH26X_Motion_Cunt, ((CHK_REPORT[item_id])&0xffff8000), check, (UINT8*)&item_name[item_id]);
	//DBG_INFO("Mount Count = 0x%08x\r\n",(unsigned int) pH26XRegCfg->uiH26X_Motion_Cunt);
	}

#if 0
	if((pH26XRegCfg->uiH26XPic0 & 0x1) == 0){
		if(h26x_getDbg1(91) != 0){
			DBG_INFO("h26x_getDbg1(91) = 0x%08x error! \r\n",(unsigned int) h26x_getDbg1(91));
		}
		else{
			DBG_INFO("h26x_getDbg1(91) = 0x%08x correct! \r\n",(unsigned int) h26x_getDbg1(91));
		}
	}
#endif

    result2 &= emu_h265_compare_saonum(p_job);
    result2 &= emu_h265_compare_psnr(p_job);

    //if(emu->iNROutEn==1)
	{
        //result2 &= emu_h265_compare_nrout(p_job);
		result &= emu_h265_compare_nrout(p_job);
    }

    if(emu->uiEcEnable == 1)
    {
        //result2 &= emu_h265_compare_sideinfo(p_job);
		result &= emu_h265_compare_sideinfo(p_job);
    }

    //result2 &= emu_h265_compare_bs(p_job);

    //result2 &= emu_h265_compare_rec(p_job);
	result &= emu_h265_compare_rec(p_job);

	//result2 &= emu_h265_compare_nal(p_job,emu->uiSliceNum);
	result &= emu_h265_compare_nal(p_job,emu->uiSliceNum);

//#if EMU_CMP_COLMV
	result &= chk_col_mv(p_job);
//#endif

#if 0
	//result2 &= emu_h265_compare_picdata_sum(p_job);
	check = 1;
    result_sw = &pH26XRegCfg->uiH26X_TMNR_Y_SUM;
	for (i = H26X_TMNR_REC_Y_CHKSUM; i <=  H26X_OSG_C_CHKSUM; i++)
	{
        item_id = i;
        result2 &= emu_cmp( result_sw[i-H26X_TMNR_REC_Y_CHKSUM], CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
        emu_printf( result_sw[i-H26X_TMNR_REC_Y_CHKSUM], CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    }
#endif

    //if(pic_num == 2 && p_job->idx!= 0)
    if(0)
    {
        result2 = FALSE;
        result = FALSE;
        DBG_INFO("hack error XXXXXXXXXXXXXXXXXXXXX\r\n");
        DBG_INFO("hack error XXXXXXXXXXXXXXXXXXXXX\r\n");
        DBG_INFO("hack error XXXXXXXXXXXXXXXXXXXXX\r\n");
    }

    result2 &= emu_h265_compare_bs(p_job,pH26XRegCfg->uiH26XBsLen);

    if(result != TRUE || result2 != TRUE)
    //if(1)
    {
        emu_h265_dump_hwbs(p_job);
        //emu_h265_dump_hwsrcout(p_job);
        emu_h265_dump_hwrec(p_job);
        emu_h265_dump_hwsrc(p_job);

    }else{
        //result2 &= emu_h265_compare_bs(p_job,pH26XRegCfg->uiH26XBsLen);
    }

    if(result != TRUE)
    {
        emu_h265_dump_hwsrcout(p_job);
    }else{
        //emu_h265_dump_hwsrcout(p_job);
    }

    //if(result != TRUE || result2 != TRUE)
    if(0)
    {
        /*
        UINT32 y_chk_sum = 0;
        UINT32 c_chk_sum = 0;
        DBG_INFO("uiSrc_Size = %d %d\r\n",emu->uiWriteSrcY,emu->uiWriteSrcC);
        DBG_INFO("y_chk_sum[0] 0x%08x 0x%08x\r\n",emu->y_chk_sum[0],emu->c_chk_sum[0]);
        DBG_INFO("y_chk_sum[1] 0x%08x 0x%08x\r\n",emu->y_chk_sum[1],emu->c_chk_sum[1]);
        y_chk_sum = buf_chk_sum((UINT8 *)emu->uiVirHwSrcYAddr, emu->uiWriteSrcY, 1);
        c_chk_sum = buf_chk_sum((UINT8 *)emu->uiVirHwSrcUVAddr, emu->uiWriteSrcC, 1);
        DBG_INFO("y_chk_sum[2] 0x%08x 0x%08x\r\n",y_chk_sum,c_chk_sum);
        */
    }

#if 1
    //if (result != TRUE )
    if(result != TRUE || result2 != TRUE)
    //if(1)
    {

		h26x_prtMem((unsigned int)CHK_REPORT, 64*4);
		h26x_prtMem((unsigned int)CHK_REPORT_2, 64*4);


        check = 1;
        item_id = H26X_BS_CHKSUM;
        emu_printf( pH26XRegCfg->uiH26XBsChkSum, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
        item_id = H26X_REC_CHKSUM;
        emu_printf( pH26XRegCfg->uiH26XRecChkSum, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
        item_id = H26X_SRC_Y_DMA_CHKSUM;
        emu_printf( pH26XRegCfg->uiH26X_SrcChkSum_Y, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
        item_id = H26X_SRC_C_DMA_CHKSUM;
        emu_printf( pH26XRegCfg->uiH26X_SrcChkSum_C, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
        item_id = H26X_BS_LEN;
        emu_printf( pH26XRegCfg->uiH26XBsLen, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    }
#endif

Rep:
{
	h265_perf_t *perf = &emu->perf;
	UINT32 cycle = CHK_REPORT[H26X_CYCLE_CNT];
	float mbs = (SIZE_64X(emu->uiWidth)/16) * (SIZE_64X(emu->uiHeight)/16);
	float avg = (float)cycle/mbs;
	perf->cycle_sum += cycle;
	perf->bslen_sum += CHK_REPORT[H26X_BS_LEN];
	if (perf->cycle_max < cycle)
	{
		perf->cycle_max = cycle;
		perf->cycle_max_avg = avg;
		perf->cycle_max_bslen = CHK_REPORT[H26X_BS_LEN];
		perf->cycle_max_bitlen_avg = (perf->cycle_max_bslen*8)/mbs;
		perf->cycle_max_frm = uiPicNum;
	}
	//DBG_INFO("cycle = %d, %f bs = %d %f\r\n",(unsigned int)cycle,avg,(unsigned int)CHK_REPORT[H26X_BS_LEN],CHK_REPORT[H26X_BS_LEN]/mbs);
}
    //if (result == TRUE && result2 != TRUE)
    if(0)
    {
        h26x_prtReg();
        h26x_getDebug();
        h26xPrintString(emu->g_main_argv,(INT32)pH26XEncSeqCfg->uiH26XInfoCfg.uiCommandLen);
    }

#if 1
    if (result == TRUE ){
        //DBG_INFO("\r\n");
        //DBG_INFO("^C\033[1m.................................... h265  [%d][%d][%d] Correct\r\n",src_num,pat_num,pic_num);
        DBG_INFO("^C\033[1m.................................... h265enc (%d)(%d) [%d][%d][%d] Correct\033[m\r\n",(int)p_job->idx1,(int)p_job->idx2,(int)src_num,(int)pat_num,(int)pic_num);

	   #if 0
	   h26x_prtReg();
	   h26x_getDebug();
	   if (DIRECT_MODE == 0)
	   {
		   DBG_INFO("job(%d) apb data\r\n", (unsigned int)p_job->idx1);
		   h26x_prtMem(p_job->apb_addr, h26x_getHwRegSize());
		   DBG_INFO("job(%d) linklist data\r\n", (unsigned int)p_job->idx1);
		   h26x_prtMem(p_job->llc_addr, h26x_getHwRegSize());
	   }
	   h26xPrintString(emu->g_main_argv,(INT32)pH26XEncSeqCfg->uiH26XInfoCfg.uiCommandLen);
	   #endif

    }
#else
    if (result == TRUE ){
        if(uiPicNum % 10 == 0)
        {
            DBG_INFO("%d",uiPicNum);
            if(uiPicNum % 30 == 0)
                DBG_INFO("\r\n");
        }else{
            DBG_INFO(".");
        }
    }
#endif
    else{
        //UINT8  type_name[2][64] = {"I","P"};
        //DBG_INFO(".................................... h265  [%d][%d][%d] Fail\r\n",uiSrcNum,uiPatNum,uiPicNum);
        DBG_INFO(".................................... h265enc (%d)(%d) [%d][%d][%d] Fail\r\n",(int)p_job->idx1,(int)p_job->idx2,(int)uiSrcNum,(int)uiPatNum,(int)uiPicNum);
        //DBG_INFO("[%s]\r\n",type_name[uiPicType]);

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

        h26xPrintString(emu->g_main_argv,(INT32)pH26XEncSeqCfg->uiH26XInfoCfg.uiCommandLen);
        //h26x_prtReg();
#if 0
        h26x_module_reset(240);
#endif
        //h26x_prtReg();

    }
    //H26XPrtCheckSum( pH26XRegSet, uiSrcFrmSize);

	if(pH26XRegCfg->uiH26XFunc1 & (1 << 19)){//RATE_CONTROL_RATE_MODE
		if(
			(pH26XRegCfg->uiH26X_RC_EST_BITLEN>CHK_REPORT[H26X_RRC_BIT_LEN] && pH26XRegCfg->uiH26X_RC_EST_BITLEN - CHK_REPORT[H26X_RRC_BIT_LEN] > CHK_REPORT[H26X_RRC_BIT_LEN]>>2 )||
			(pH26XRegCfg->uiH26X_RC_EST_BITLEN>CHK_REPORT[H26X_RRC_BIT_LEN] && pH26XRegCfg->uiH26X_RC_EST_BITLEN - CHK_REPORT[H26X_RRC_BIT_LEN] > pH26XRegCfg->uiH26X_RC_EST_BITLEN>>2)||
			(pH26XRegCfg->uiH26X_RC_EST_BITLEN<CHK_REPORT[H26X_RRC_BIT_LEN] && CHK_REPORT[H26X_RRC_BIT_LEN] - pH26XRegCfg->uiH26X_RC_EST_BITLEN> CHK_REPORT[H26X_RRC_BIT_LEN]>>2 )||
			(pH26XRegCfg->uiH26X_RC_EST_BITLEN<CHK_REPORT[H26X_RRC_BIT_LEN] && CHK_REPORT[H26X_RRC_BIT_LEN] - pH26XRegCfg->uiH26X_RC_EST_BITLEN> pH26XRegCfg->uiH26X_RC_EST_BITLEN>>2 )
		){
			DBG_INFO("Warn: RRC Bit Len diff too large, SW:0x%08x HW:0x%08x   \r\n",(int)pH26XRegCfg->uiH26X_RC_EST_BITLEN,(int)CHK_REPORT[H26X_RRC_BIT_LEN] );
		}
		return TRUE;
	}

    return result;
    //DBG_INFO("\r\n");
}


// Check Resutlt //
static BOOL EmuH26XEncCheckResult_srcout_only(h26x_job_t *p_job, UINT32 interrupt){
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = &ctx->pat;
    H26XEncSeqCfg *pH26XEncSeqCfg = &emu->sH26XEncSeqCfg;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
    UINT32 uiPicType = pH26XRegCfg->uiH26XPic0 & 0x1;
    UINT32 uiSrcNum = pat->uiSrcNum,uiPatNum = pat->uiPatNum,uiPicNum = pat->uiPicNum;
    UINT32 i,item_id;
    BOOL result = TRUE, result2 = TRUE;
    UINT32 check = 1;
    //UINT32 smartrec_en;
    UINT32  *result_sw;
    //BOOL tmnr_en = (pH26XRegCfg->uiH26X_TMNR_CFG[2] & 1);
#if DIRECT_MODE
    H26XRegSet   *pH26XRegSet = emu->pH26XRegSet;
     unsigned int *CHK_REPORT = (unsigned int *)pH26XRegSet->CHK_REPORT;
#else
#if 0
     unsigned int *CHK_REPORT = ( unsigned int *)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 0);
//dma_getNonCacheAddr
#else//hk tmp hack

unsigned int *CHK_REPORT = (unsigned int *)(p_job->check1_addr);

//unsigned int *CHK_REPORT_2 = (unsigned int *)(p_job->check2_addr);
//unsigned int *CHK_REPORT_2 = (unsigned int *)(p_job->apb_addr + H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 0);

//DBG_INFO("report addr 0x%08x 0x%08x\r\n",(unsigned int)p_job->check1_addr, (unsigned int)p_job->check2_addr );
h26x_flushCacheRead((UINT32)p_job->check1_addr, H26X_REG_REPORT_SIZE); //hk/

h26x_flushCacheRead(p_job->apb_addr + H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET, H26X_REG_REPORT_SIZE); //hk
//h26x_flushCacheRead((UINT32)p_job->check2_addr , 64 * 4 + 32); //hk/


#endif

#endif
    UINT32 result_sel[8];

    src_num = pat->uiSrcNum;
    pat_num = pat->uiPatNum;
    pic_num = pat->uiPicNum;


#if 0
    emuh26x_msg(("job(%d,%d) report\r\n", p_job->idx1, p_job->idx2));
    emuh26x_msg(("report 1 : %p\r\n", (p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 0)));
    h26x_prtMem((UINT32)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 0), sizeof(UINT32)*H26X_REPORT_NUMBER);
#endif
    if ((interrupt & H26X_FINISH_INT) == 0){
        DBG_INFO("Src[%d] Pat[%d] Pic[%d] Error Interrupt[0x%08x]!\r\n",(unsigned int)uiSrcNum,(unsigned int)uiPatNum,(unsigned int)uiPicNum,(unsigned int)interrupt);
        //bRet = FALSE;
    }

#if 1
    item_id = H26X_SRC_Y_DMA_CHKSUM;
    result &= emu_cmp( pH26XRegCfg->uiH26X_SrcChkSum_Y, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    item_id = H26X_SRC_C_DMA_CHKSUM;
    result &= emu_cmp( pH26XRegCfg->uiH26X_SrcChkSum_C, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
#endif
#if 1
    result_sw = &pH26XRegCfg->uiH26X_OSG_Y_SUM;
	for (i = H26X_OSG_Y_CHKSUM; i <=  H26X_OSG_C_CHKSUM; i++)
	{
        item_id = i;
        result &= emu_cmp( result_sw[i-H26X_OSG_Y_CHKSUM], CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
        emu_printf( result_sw[i-H26X_OSG_Y_CHKSUM], CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    }
#endif

    if(emu->iNROutEn==1){
        result &= emu_h265_compare_nrout(p_job);
    }

    for(i=14;i<22;i++)
    {
        result_sel[i-14] = h26x_getDbg3(i);
        DBG_INFO("[%02d] 0x%08x\r\n",(unsigned int)i,(unsigned int)result_sel[i-14]);
    }

    //if (tmnr_en == 0)
	{
        result2 &= emu_cmp( 0, h26x_getDbg3(15), check, (UINT8*)"sel_15");
    }
    result2 &= emu_cmp( 0, h26x_getDbg3(16), check, (UINT8*)"sel_16");
    result2 &= emu_cmp( 0, h26x_getDbg3(17), check, (UINT8*)"sel_17");
    result2 &= emu_cmp( 0xae   /* + 0x3f 528 linklist add 1 rd data*/, h26x_getDbg3(18), check, (UINT8*)"sel_18");
    // [321] refR + OSD + TMNRW, disable for 321
    //result2 &= emu_cmp( 0, h26x_getDbg3(20), check, (UINT8*)"sel_20");

    //if(pic_num == 2 && p_job->idx!= 0)
    if(0)
    {
        result2 = FALSE;
        result = FALSE;
        DBG_INFO("hack error XXXXXXXXXXXXXXXXXXXXX\r\n");
        DBG_INFO("hack error XXXXXXXXXXXXXXXXXXXXX\r\n");
        DBG_INFO("hack error XXXXXXXXXXXXXXXXXXXXX\r\n");
    }

    //if(result != TRUE || result2 != TRUE)
    if(result != TRUE)
    {
        emu_h265_dump_hwsrcout(p_job);
    }else{
        //emu_h265_dump_hwsrcout(p_job);
    }

#if 1
    //if (result != TRUE )
    if(result != TRUE || result2 != TRUE)
    //if(1)
    {
        check = 1;
        item_id = H26X_BS_CHKSUM;
        emu_printf( pH26XRegCfg->uiH26XBsChkSum, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
        item_id = H26X_REC_CHKSUM;
        emu_printf( pH26XRegCfg->uiH26XRecChkSum, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
        item_id = H26X_SRC_Y_DMA_CHKSUM;
        emu_printf( pH26XRegCfg->uiH26X_SrcChkSum_Y, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
        item_id = H26X_SRC_C_DMA_CHKSUM;
        emu_printf( pH26XRegCfg->uiH26X_SrcChkSum_C, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
        item_id = H26X_BS_LEN;
        emu_printf( pH26XRegCfg->uiH26XBsLen, CHK_REPORT[item_id], check, (UINT8*)&item_name[item_id]);
    }
#endif

    if (result == TRUE && result2 != TRUE)
    //if(0)
    {
        h26x_prtReg();
        h26x_getDebug();
        h26xPrintString(emu->g_main_argv,(INT32)pH26XEncSeqCfg->uiH26XInfoCfg.uiCommandLen);
    }

#if 1
    if (result == TRUE ){
        //emuh26x_msg(("\r\n"));
        //emuh26x_msg(("^C\033[1m.................................... h265  [%d][%d][%d] Correct\r\n",src_num,pat_num,pic_num));
        //emuh26x_msg(("^C\033[1m.................................... h265enc (%d)(%d) [%d][%d][%d] Correct\r\n",p_job->idx1,p_job->idx2,src_num,pat_num,pic_num));
        DBG_INFO(".................................... h265enc (%d)(%d) [%d][%d][%d] Correct\r\n",(int)p_job->idx1,(int)p_job->idx2,(unsigned int)src_num,(unsigned int)pat_num,(unsigned int)pic_num);
        //h26x_prtReg();
        //h26x_getDebug();
        //h26xPrintString(g_main_argv,(INT32)pH26XEncSeqCfg->uiH26XInfoCfg.uiCommandLen);
    }
#else
    if (result == TRUE ){
        if(uiPicNum % 10 == 0)
        {
            emuh26x_msg(("%d",uiPicNum));
            if(uiPicNum % 30 == 0)
                emuh26x_msg(("\r\n"));
        }else{
            emuh26x_msg(("."));
        }
    }
#endif
    else{
        UINT8  type_name[2][64] = {"I","P"};
        //emuh26x_msg((".................................... h265  [%d][%d][%d] Fail\r\n",uiSrcNum,uiPatNum,uiPicNum));
        DBG_INFO(".................................... h265enc (%d)(%d) [%d][%d][%d] Fail\r\n",(int)p_job->idx1,(int)p_job->idx2,(unsigned int)uiSrcNum,(unsigned int)uiPatNum,(unsigned int)uiPicNum);
        DBG_INFO("[%s]\r\n",type_name[uiPicType]);

        //while(1){};
        h26x_prtReg();
        h26x_getDebug();
        h26xPrintString(emu->g_main_argv,(INT32)pH26XEncSeqCfg->uiH26XInfoCfg.uiCommandLen);
    }
    //H26XPrtCheckSum( pH26XRegSet, uiSrcFrmSize);


    return result;
    //emuh26x_msg(("\r\n"));
}

BOOL emu_h265_chk_one_pic(h26x_job_t *p_job, UINT32 interrupt, unsigned int job_num, unsigned int rec_out_en)
{
    BOOL bH26XEncCheckRet = TRUE;
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
	h265_pat_t *pat = (h265_pat_t *)&ctx->pat;

#if DIRECT_MODE
    H26XRegSet *pH26XRegSet = emu->pH26XRegSet;
	h26x_getEncReport(pH26XRegSet->CHK_REPORT);	// shall remove while link-list //
	h26x_getEncReport2(pH26XRegSet->CHK_REPORT_2);	// shall remove while link-list //
#else
    #if 0
	if ((p_job->idx + 1) == job_num)
		h26x_getEncReport(pH26XRegSet->CHK_REPORT);
	else{
		memcpy((void *)pH26XRegSet->CHK_REPORT, (void *)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 4), sizeof(UINT32)*H26X_REPORT_NUMBER);
    	DBG_INFO("job(%d) report\r\n", p_job->idx);
        DBG_INFO("report 1 : %p\r\n", (p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 4));
        h26x_prtMem((UINT32)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 4), sizeof(UINT32)*H26X_REPORT_NUMBER);
    	DBG_INFO("report 2 : %p\r\n", pH26XRegSet->CHK_REPORT);
    	h26x_prtMem((UINT32)pH26XRegSet->CHK_REPORT, sizeof(UINT32)*H26X_REPORT_NUMBER);
    }
    #endif
	//dma_flushReadCacheWidthNEQLineOffset(dma_getCacheAddr(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 4), sizeof(UINT32)*H26X_REPORT_NUMBER);
	//dma_flushReadCacheWidthNEQLineOffset(dma_getCacheAddr(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET), sizeof(UINT32)*H26X_REPORT_NUMBER);
#endif


    if(emu->src_out_only)
    {
        bH26XEncCheckRet = EmuH26XEncCheckResult_srcout_only(p_job,interrupt);
    }else{
    	bH26XEncCheckRet = EmuH26XEncCheckResult(p_job,interrupt);
    }

    emu_h265_write_hwbs(p_job);
    emu_h265_write_hwtnr(emu);
    emu_h265_write_rec(emu);
/*
#ifdef WRITE_SRC
    if(bH26XEncCheckRet != TRUE)
    {
        DBG_INFO("uiSrc_Size = %d %d\r\n",emu->uiWriteSrcY,emu->uiWriteSrcC);
        h26xFileWrite( &emu->file.hw_src, emu->uiWriteSrcY, emu->uiVirHwSrcYAddr);
        h26xFileWrite( &emu->file.hw_src, emu->uiWriteSrcC, emu->uiVirHwSrcUVAddr);
        DBG_INFO("hw_src = %s\r\n",emu->file.hw_src.FileName);
    }
#endif
*/
    if(bH26XEncCheckRet == TRUE)
    {
        pat->uiPicNum++;
    }else{
        pat->uiPicNum = 65536;
    }
    return bH26XEncCheckRet;

}


//#endif //if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)






