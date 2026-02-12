#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "emu_h26x_common.h"

#include "emu_h265_enc.h"
#include "emu_h265_tool.h"

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)


#if EMU_CMP_REC
static UINT8 sH26XEncRec[5120*5120*3/2];
static UINT16 sH26XEncRecLenY[5120*5120/64/4];
static UINT16 sH26XEncRecLenUV[5120*5120/2/64/4];

static UINT32 tbl_4x2_y[128] = {
 0,   1,   2,   3,    8,   9,  10,  11,   16,  17,  18,  19,   24,  25,  26,  27,
 32,  33,  34,  35,   40,  41,  42,  43,   48,  49,  50,  51,   56,  57,  58,  59,
 64,  65,  66,  67,   72,  73,  74,  75,   80,  81,  82,  83,   88,  89,  90,  91,
 96,  97,  98,  99,  104, 105, 106, 107,  112, 113, 114, 115,  120, 121, 122, 123,
  4,   5,   6,   7,   12,  13,  14,  15,   20,  21,  22,  23,   28,  29,  30,  31,
 36,  37,  38,  39,   44,  45,  46,  47,   52,  53,  54,  55,   60,  61,  62,  63,
 68,  69,  70,  71,   76,  77,  78,  79,   84,  85,  86,  87,   92,  93,  94,  95,
100, 101, 102, 103,  108, 109, 110, 111,  116, 117, 118, 119,  124, 125, 126, 127};

static UINT32 tbl_4x2_c[128] = {
     0,   1,   2,   3,    8,   9,  10,  11,   16,  17,  18,  19,   24,  25,  26,  27,
     32,  33,  34,  35,   40,  41,  42,  43,   48,  49,  50,  51,   56,  57,  58,  59,
      4,   5,   6,   7,   12,  13,  14,  15,   20,  21,  22,  23,   28,  29,  30,  31,
     36,  37,  38,  39,   44,  45,  46,  47,   52,  53,  54,  55,   60,  61,  62,  63,
     64,  65,  66,  67,   72,  73,  74,  75,   80,  81,  82,  83,   88,  89,  90,  91,
     96,  97,  98,  99,  104, 105, 106, 107,  112, 113, 114, 115,  120, 121, 122, 123,
     68,  69,  70,  71,   76,  77,  78,  79,   84,  85,  86,  87,   92,  93,  94,  95,
    100, 101, 102, 103,  108, 109, 110, 111,  116, 117, 118, 119,  124, 125, 126, 127
};

static void emu_h265_read_rec(h265_emu_t *emu)
{
    UINT32  uiRecLen;
    h26xFileReadFlush(&emu->file.rec_y,(UINT32)(emu->uiFrameSize),(UINT32)(&sH26XEncRec));
    h26xFileReadFlush(&emu->file.rec_c,(UINT32)(emu->uiFrameSize/2),(UINT32)(&sH26XEncRec[emu->uiFrameSize]));
    uiRecLen = ((emu->uiWidth+63)/64*64) * ((emu->uiHeight+63)/64*64) / 64 / 4;
    h26xFileReadFlush(&emu->file.rec_y_len,(UINT32)uiRecLen * sizeof(UINT16),(UINT32)(&sH26XEncRecLenY));
    h26xFileReadFlush(&emu->file.rec_c_len,(UINT32)uiRecLen/2 * sizeof(UINT16),(UINT32)(&sH26XEncRecLenUV));
}

static UINT32 calc_rec_size(UINT32 uiLcuIdx_W, UINT32 uiLcuIdx_H, UINT32 uiWidth, UINT32 uiHeight, UINT32 uiY, UINT16 *pLen, UINT32 uiTileWidth)
{
    UINT32 uiSize = 0,idx,i;
    UINT32 uiTop = (uiLcuIdx_H == 0);
    UINT32 uiButtom;
    UINT32 w = uiWidth / 64;
    UINT32 tt,mm; // top, middle
    UINT32 cc;
    UINT32 uiUnit_H = (uiY==1) ? 64 : 32;
    UINT32 offset = 0;
    int org = 0;

    uiButtom = (uiLcuIdx_H == (uiHeight/uiUnit_H - 1));

    if(uiTop)
    {
        tt = 0;
        mm = 0;
        cc = (uiY == 1)? 15 : 7;
        //DBG_INFO(" T ");
    }else if(uiButtom){
        tt = 1;
        mm = uiLcuIdx_H - 1;
        cc = (uiY == 1)? 17 : 9;
        //DBG_INFO(" B ");
    }else{
        tt = 1;
        mm = uiLcuIdx_H - 1;
        cc = (uiY == 1)? 16 : 8;
        //DBG_INFO(" M ");
    }

    if(uiTileWidth < uiWidth){
        // tile mode
        if(uiLcuIdx_W < (uiTileWidth/64)){
            // tile 0
            w = uiTileWidth / 64;

            //DBG_INFO("%s %d [%d,%d] w = %d, offset = %d\r\n",__FUNCTION__,__LINE__,
                //uiLcuIdx_W,uiLcuIdx_H,w,offset);
        }else{
            w = (uiWidth - uiTileWidth) / 64;
            // tile 1
            if(uiY)
                offset = (uiTileWidth/64) * (uiHeight/uiUnit_H) * 16;
            else
                offset = (uiTileWidth/64) * (uiHeight/uiUnit_H) * 8;

            uiLcuIdx_W = uiLcuIdx_W - (uiTileWidth/64);

            //DBG_INFO("%s %d [%d,%d] w = %d, offset = %d\r\n",__FUNCTION__,__LINE__,
             //   uiLcuIdx_W,uiLcuIdx_H,w,offset);

            //DBG_INFO("%s %d\r\n",__FUNCTION__,__LINE__);
        }
    }

    if(uiY)
        idx = ((tt * 15) + ((mm)* 16)) * w + (uiLcuIdx_W * cc);
    else
        idx = ((tt * 7) + ((mm)* 8)) * w + (uiLcuIdx_W * cc);

    org = idx + offset;

    //DBG_INFO("\n");
    for(i=0;i < cc; i++){
        uiSize += pLen[idx + i + offset];
        //DBG_INFO(" %d ",pLen[idx + i]);
    }
    //DBG_INFO("\n");

    //DBG_INFO(" idx = %3d ~ %3d  (%6d),",idx, idx + cc, uiSize);
    return uiSize;
}

static UINT32 calc_rec_addr_econ(UINT32 uiLcuIdx_W, UINT32 uiLcuIdx_H, UINT32 uiWidth, UINT32 uiHeight, UINT32 uiY)
{
    UINT32 uiAddr;
    UINT32 uiTop = (uiLcuIdx_H == 0);
    UINT32 h = uiHeight * 64;
    //UINT32 uiSize,bb;
    UINT32 tt,mm;

    if(uiY)
    {
        tt = 60;
        mm = 64;
        //bb = 68;
    }else{
        tt = 28;
        mm = 32;
        //bb = 36;
    }

    if(uiTop)
    {
        uiAddr = uiLcuIdx_W * h;
        //uiSize = tt*64;
    }else{
        uiAddr = (uiLcuIdx_W * h) + ( 64 * tt) + ((uiLcuIdx_H-1) * 64 * mm);
        //uiSize = (uiButtom)? (bb * 64) : (mm * 64);
    }
    //DBG_INFO(" addr = %6d ~ %6d (%d)\n",uiAddr,uiAddr + uiSize, uiSize);

    return uiAddr;
}


static BOOL emu_h265_compare_rec_econ(h26x_job_t *p_job)
{
    BOOL bRet = TRUE;
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    h265_pat_t *pat = &ctx->pat;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    UINT8        *pucSwRecYAddr,*pucSwRecUVAddr,*pucHwRecYAddr,*pucHwRecUVAddr;
    UINT32 uiTileWidth;
    UINT32 uiY,w,h;
	UINT32 i,j,k;
    UINT32 uiLcu_W,uiLcu_H;
    UINT32 uiUnit_H;
    UINT8 *pucSw,*pucHw;
    UINT16 *pLen;
    UINT32 uiCS = 0, uiCnt = 0;
    UINT8 recErrCnt = 0;

    //DBG_INFO("EmuH26XCompareRec_econ pucHwRecYAddr = 0x%08x 0x%08x\r\n",pucHwRecYAddr,pucHwRecUVAddr);
    //DBG_INFO("uiTileWidth = %d\r\n",uiTileWidth);
    pucSwRecYAddr = (UINT8 *)sH26XEncRec;
    pucSwRecUVAddr = (UINT8 *)((UINT32)pucSwRecYAddr + emu->uiFrameSize);
    pucHwRecYAddr = (UINT8 *)pH26XVirEncAddr->uiRefAndRecYAddr[pH26XVirEncAddr->uiRecIdx];
    pucHwRecUVAddr = (UINT8 *)pH26XVirEncAddr->uiRefAndRecUVAddr[pH26XVirEncAddr->uiRecIdx];

    for(uiY = 0; uiY < 2 ; uiY++)
    {
        uiUnit_H = ( uiY == 1) ? 64 : 32;
        w = (emu->uiWidth+63)/64*64;
        if(uiY){
            // Luma
            h = (emu->uiHeight+63)/64*64;
            pucSw = pucSwRecYAddr;
            pucHw = pucHwRecYAddr;
            pLen = (UINT16 *)&sH26XEncRecLenY;
            DBG_INFO("compare Luma\r\n");
        }else{
            // Chroma
            h = (emu->uiHeight+63)/64*64/2;
            pucSw = pucSwRecUVAddr;
            pucHw = pucHwRecUVAddr;
            pLen = (UINT16 *)&sH26XEncRecLenUV;
            DBG_INFO("compare Chroma\r\n");
        }
        uiLcu_W = (w+63)/64;
        uiLcu_H = (h + uiUnit_H - 1)/uiUnit_H;
#if 0
        DBG_INFO("========== org ============\r\n");
        DBG_INFO("========== org ============\r\n");

        for( i = 0 ; i < uiLcu_H; i++)
        {
            for( j = 0; j < uiLcu_W; j++)
            {
                UINT32 size;
                size = calc_rec_size( j, i, w, h, uiY, pLen, uiTileWidth);
                //DBG_INFO(" %d (%d)", size,org);
                DBG_INFO(" %d", org);
            }
            DBG_INFO("\r\n");
        }
        DBG_INFO("========== calc_rec_size ============\r\n");
        DBG_INFO("========== calc_rec_size ============\r\n");

        for( i = 0 ; i < uiLcu_H; i++)
        {
            for( j = 0; j < uiLcu_W; j++)
            {
                UINT32 size;
                size = calc_rec_size( j, i, w, h, uiY, pLen, uiTileWidth);
                DBG_INFO(" %d", size);
            }
            DBG_INFO("\r\n");
        }

        DBG_INFO("========== calc_rec_size ============\r\n");
        DBG_INFO("========== calc_rec_addr ============\r\n");
        for( i = 0 ; i < uiLcu_H; i++)
        {
            for( j = 0; j < uiLcu_W; j++)
            {
                UINT32 uiAddr;
                uiAddr = calc_rec_addr( j, i, w, h, uiY);
                DBG_INFO(" %6d", uiAddr);
            }
            DBG_INFO("\r\n");
        }
        DBG_INFO("=========================\r\n");
        DBG_INFO("=========================\r\n");
        DBG_INFO("=========================\r\n");
#endif
        for( i = 0 ; i < uiLcu_H; i++)
        {
            //DBG_INFO("=========================\r\n");
            for( j = 0; j < uiLcu_W; j++)
            {
                UINT32 size,uiAddr;
                size = calc_rec_size( j, i, w, h, uiY, pLen, uiTileWidth);
                uiAddr = calc_rec_addr_econ( j, i, w, h, uiY);
                //DBG_INFO("[%2d][%2d] , size = %d 0x%08x\r\n", i , j,size,uiAddr);
                if(size == 0)
                {
                    DBG_INFO("size = %d at EmuH26XCompareRec_econ\n", size);
                    bRet = FALSE;
                    return bRet;
                }
                recErrCnt = 0;

                for(k = 0; k < size; k++)
                {
                    uiCS += bit_reverse(pucSw[uiAddr+k]);
                    uiCnt++;
                    if(pucSw[uiAddr+k] != pucHw[uiAddr+k])
                    {
                        DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare Rec Error! (ec_on)\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
                        DBG_INFO("Error at uiY= %d, i = %d %d, addr = %d, k = %d , %d\r\n",
                            (int)uiY,(int)i,(int)j,(int)uiAddr,(int)k,(int)uiAddr+k);
                        DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",(int)pucSw[uiAddr+k],(int)pucHw[uiAddr+k]);
                        DBG_INFO("size = %d\r\n", (int)size);
                        bRet = FALSE;
                        recErrCnt++;
                        if(recErrCnt > 4)
                            //return bRet;
                            break;
                    }
                } // end of k

                //DBG_INFO("recErrCnt = %d\r\n", recErrCnt);
            } // end of j
        } // end of i

    } // end of y


    //DBG_INFO(" CS :0x%08x, cnt = %d\r\n",uiCS,uiCnt);

    return bRet;

}

static void copy_frame_to_buf(UINT8* rec_addr, UINT8* buf_addr, UINT32 offset1, UINT32 offset2, UINT32 size, UINT32 h)
{
	UINT32 i;
    for( i = 0 ; i < h; i++)
    {
        memcpy(buf_addr,&rec_addr[offset1],size);
        rec_addr += offset2;
        buf_addr += size;
    }

}

static void copy_frame_from_buf(UINT8* rec_addr, UINT8* buf_addr, UINT32 offset1, UINT32 offset2, UINT32 size, UINT32 h)
{
	UINT32 i;
    for( i = 0 ; i < h; i++)
    {
        memcpy(&rec_addr[offset1],buf_addr,size);
        rec_addr += offset2;
        buf_addr += size;
    }
}


static void CopyRecToTmpBuf( h26x_job_t *p_job, UINT8* pucHwRecYAddr, UINT8* pucHwRecUVAddr, UINT32 uiTileWidth, UINT32 uiEcls)
{
    UINT32 uiY,w,h;
    UINT8 *pucRec,*pucTmpBuf;
    //DBG_INFO("%s %d\r\n",__FUNCTION__,__LINE__);
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;

    //DBG_INFO("EmuH26XCompareRec_econ pucHwRecYAddr = 0x%08x 0x%08x\r\n",pucHwRecYAddr,pucHwRecUVAddr);
    //DBG_INFO("uiTileWidth = %d\r\n",uiTileWidth);
    w = (emu->uiWidth+63)/64*64;

    for(uiY = 0; uiY < 2 ; uiY++)
    {
        if(uiY){
            // Luma
            h = (emu->uiHeight+63)/64*64;
            pucRec = pucHwRecYAddr;
            pucTmpBuf = (UINT8*)emu->uiVirTmpRecExtraYAddr;
        }else{
            // Chroma
            h = (emu->uiHeight+63)/64*64/2;
            pucRec = pucHwRecUVAddr;
            pucTmpBuf = (UINT8*)emu->uiVirTmpRecExtraCAddr;
        }
        if(uiEcls)
        {
            memcpy(pucTmpBuf,pucRec + (h)*(uiTileWidth-128), h * 128);
        }else{
            copy_frame_to_buf( pucRec, pucTmpBuf, uiTileWidth-128, w, 128, h);
        }
    } // end of y
}

static void CopyExtraToRec(  h26x_job_t *p_job, UINT8* pucHwRecYAddr, UINT8* pucHwRecUVAddr, UINT32 uiTileWidth, UINT32 uiEcls)
{
    UINT32 uiY,w,h;
    UINT8 *pucRec,*pucTmpBuf;
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    H26XEncAddr *pH26XVirEncAddr = &emu->sH26XVirEncAddr;

    //DBG_INFO("EmuH26XCompareRec_econ pucHwRecYAddr = 0x%08x 0x%08x\r\n",pucHwRecYAddr,pucHwRecUVAddr);
    //DBG_INFO("uiTileWidth = %d\r\n",uiTileWidth);
    w = (emu->uiWidth+63)/64*64;

    for(uiY = 0; uiY < 2 ; uiY++)
    {
        if(uiY){
            // Luma
            h = (emu->uiHeight+63)/64*64;
            pucRec = pucHwRecYAddr;
            pucTmpBuf = (UINT8*)pH26XVirEncAddr->uiRecExtraYAddr[pH26XVirEncAddr->uiRecIdx];
        }else{
            // Chroma
            h = (emu->uiHeight+63)/64*64/2;
            pucRec = pucHwRecUVAddr;
            pucTmpBuf = (UINT8*)pH26XVirEncAddr->uiRecExtraCAddr[pH26XVirEncAddr->uiRecIdx];
        }
        if(uiEcls)
        {
            memcpy(pucRec + (h)*(uiTileWidth-128),pucTmpBuf, h * 128);
        }else{
            copy_frame_from_buf( pucRec, pucTmpBuf, uiTileWidth-128, w, 128, h);
        }

    } // end of y
}

static void CopyBackRecToTmpBuf(  h26x_job_t *p_job, UINT8* pucHwRecYAddr, UINT8* pucHwRecUVAddr, UINT32 uiTileWidth, UINT32 uiEcls)
{
    UINT32 uiY,w,h;
    UINT8 *pucRec,*pucTmpBuf;
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;

    //DBG_INFO("%s %d\r\n",__FUNCTION__,__LINE__);
    //DBG_INFO("EmuH26XCompareRec_econ pucHwRecYAddr = 0x%08x 0x%08x\r\n",pucHwRecYAddr,pucHwRecUVAddr);
    //DBG_INFO("uiTileWidth = %d\r\n",uiTileWidth);
    w = (emu->uiWidth+63)/64*64;

    for(uiY = 0; uiY < 2 ; uiY++)
    {
        if(uiY){
            // Luma
            h = (emu->uiHeight+63)/64*64;
            pucRec = pucHwRecYAddr;
            pucTmpBuf = (UINT8*)pH26XVirEncAddr->uiRecExtraYAddr[pH26XVirEncAddr->uiRecIdx];
        }else{
            // Chroma
            h = (emu->uiHeight+63)/64*64/2;
            pucRec = pucHwRecUVAddr;
            pucTmpBuf = (UINT8*)pH26XVirEncAddr->uiRecExtraCAddr[pH26XVirEncAddr->uiRecIdx];
        }
        if(uiEcls)
        {
            memcpy(pucRec + (h)*(uiTileWidth-128),pucTmpBuf, h * 128);
        }else{
            copy_frame_from_buf( pucRec, pucTmpBuf, uiTileWidth-128, w, 128, h);
        }

    } // end of y
}



static UINT32 calc_rec_sw_addr_ecoff(UINT32 uiLcuIdx_W, UINT32 uiLcuIdx_H, UINT32 uiWidth, UINT32 uiHeight, UINT32 uiY)
{
    UINT32 uiAddr;
    UINT32 h = uiHeight * 64;
    UINT32 mm;

    if(uiY)
    {
        mm = 64;
    }else{
        mm = 32;
    }
    uiAddr = (uiLcuIdx_W * h) + (uiLcuIdx_H * 64 * mm);
    //DBG_INFO(" addr = %6d ~ %6d (%d)\n",uiAddr,uiAddr + uiSize, uiSize);

    return uiAddr;
}

static UINT32 calc_rec_hw_addr_ecoff(UINT32 uiLcuIdx_W, UINT32 uiLcuIdx_H, UINT32 uiWidth, UINT32 uiHeight, UINT32 uiY)
{
    UINT32 uiAddr;
    UINT32 mm;

    if(uiY)
    {
        mm = 64;
    }else{
        mm = 32;
    }
    uiAddr = (uiLcuIdx_W * 64) + (uiLcuIdx_H * uiWidth * mm);

    //DBG_INFO(" addr = %6d ~ %6d (%d)\n",uiAddr,uiAddr + uiSize, uiSize);
    return uiAddr;
}

//Luma : n = (m % 4) + (((m%8) > 3) * 64) + (m /8 * 4);
extern UINT32 tbl_4x2_y[128];
extern UINT32 tbl_4x2_c[128];

static BOOL emu_h265_compare_rec_ecoff(h26x_job_t *p_job)
{
    BOOL bRet = TRUE;
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    h265_pat_t *pat = &ctx->pat;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    UINT32 uiY,w,h;
	UINT32 i,j,k,m,n;
    UINT32 uiLcu_W,uiLcu_H;
    UINT32 uiUnit_H;
    UINT8 *pucSw,*pucHw;
    UINT16 Lcu_h;
    UINT32 uiCS = 0, uiCnt = 0;
    UINT8 recErrCnt = 0;
    UINT8  *pucSwRecYAddr,*pucSwRecUVAddr,*pucHwRecYAddr,*pucHwRecUVAddr;

    //DBG_INFO("EmuH26XCompareRec_econ pucHwRecYAddr = 0x%08x 0x%08x\r\n",pucHwRecYAddr,pucHwRecUVAddr);
    //DBG_INFO("uiTileWidth = %d\r\n",uiTileWidth);
    pucSwRecYAddr = (UINT8 *)sH26XEncRec;
    pucSwRecUVAddr = (UINT8 *)((UINT32)pucSwRecYAddr + emu->uiFrameSize);
    pucHwRecYAddr = (UINT8 *)pH26XVirEncAddr->uiRefAndRecYAddr[pH26XVirEncAddr->uiRecIdx];
    pucHwRecUVAddr = (UINT8 *)pH26XVirEncAddr->uiRefAndRecUVAddr[pH26XVirEncAddr->uiRecIdx];
	//UINT32 tbl_4x2[128];
    //DBG_INFO("EmuH26XCompareRec_econ pucHwRecYAddr = 0x%08x 0x%08x\r\n",pucHwRecYAddr,pucHwRecUVAddr);
    //DBG_INFO("uiTileWidth = %d\r\n",uiTileWidth);
    w = (emu->uiWidth+63)/64*64;
    h = (emu->uiHeight+63)/64*64;

    // Luma
    uiY = 1;
    uiUnit_H = 64;
    pucSw = pucSwRecYAddr;
    pucHw = pucHwRecYAddr;
    Lcu_h = 64;
    DBG_INFO("compare Luma\r\n");
/*
    DBG_INFO("pucSw = 0x%08x\r\n",pucSw);
    h26xPrtMem((UINT32)pucSw,128);
    DBG_INFO("pucHw = 0x%08x\r\n",pucHw);
    h26xPrtMem((UINT32)pucHw,128);
*/
    uiLcu_W = (w+63)/64;
    uiLcu_H = (h + uiUnit_H - 1)/uiUnit_H;

    for( i = 0 ; i < uiLcu_H; i++)
    {
        //DBG_INFO("=========================\r\n");
        for( j = 0; j < uiLcu_W; j++)
        {
            UINT32 uiHwAddr,uiSwAddr;
            uiSwAddr = calc_rec_sw_addr_ecoff( j, i, w, h, uiY);
            uiHwAddr = calc_rec_hw_addr_ecoff( j, i, w, h, uiY);
            //DBG_INFO("[%2d][%2d] , size = %d 0x%08x\r\n", i , j,size,uiAddr);
            recErrCnt = 0;

            for(k = 0; k < (Lcu_h/2); k++)
            {
                UINT32 uiHwAddr1 = uiHwAddr,m1 = 0;
                for(m = 0; m < 128; m++)
                {
                    uiCS += bit_reverse(pucSw[uiSwAddr+m]);
                    uiCnt++;
                    n = tbl_4x2_y[m];
                    if(pucSw[uiSwAddr+n] != pucHw[uiHwAddr1+m1])
                    {
                        DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare Luma Rec Error! (ec_off)\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
                        DBG_INFO("Error at uiY= %d, i = %d %d, k = %d %d, swaddr = %d %d\r\n",
                            (int)uiY,(int)i,(int)j,(int)k,(int)m,(int)(uiSwAddr+n),(int)(uiHwAddr1+m1));
                        DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",(int)pucSw[uiSwAddr+n],(int)pucHw[uiHwAddr1+m1]);
                        bRet = FALSE;
                        recErrCnt++;
                        if(recErrCnt > 20)
                        {
                            //return bRet;
                            //break;
                            goto CC;
                        }

                    }
                    m1++;
                    if(m == 63){
                        uiHwAddr1 = uiHwAddr + w;
                        m1 = 0;
                    }

                } // end of m
                uiSwAddr += 128;
                uiHwAddr += (2*w);
            } // end of k
              //DBG_INFO("recErrCnt = %d\r\n", recErrCnt);
        } // end of j
    } // end of i

    DBG_INFO("=========================\r\n");

CC:
    // Chroma
    uiY = 0;
    uiUnit_H = 32;
    h = (emu->uiHeight+63)/64*64/2;
    pucSw = pucSwRecUVAddr;
    pucHw = pucHwRecUVAddr;
    Lcu_h = 32;
    DBG_INFO("compare Chroma\r\n");
    //DBG_INFO("pucSw = 0x%08x\r\n",pucSw);
    //h26xPrtMem((UINT32)pucSw,128);
    //DBG_INFO("pucHw = 0x%08x\r\n",pucHw);
    //h26xPrtMem((UINT32)pucHw,128);
    //DBG_INFO("\r\n");

    uiLcu_W = (w+63)/64;
    uiLcu_H = (h + uiUnit_H - 1)/uiUnit_H;
    for( i = 0 ; i < uiLcu_H; i++)
    {
        //DBG_INFO("=========================\r\n");
        for( j = 0; j < uiLcu_W; j++)
        {
            UINT32 uiHwAddr,uiSwAddr;
            uiSwAddr = calc_rec_sw_addr_ecoff( j, i, w, h, uiY);
            uiHwAddr = calc_rec_hw_addr_ecoff( j, i, w, h, uiY);
            //DBG_INFO("[%2d][%2d] , size = %d 0x%08x\r\n", i , j,size,uiAddr);
            recErrCnt = 0;

            for(k = 0; k < (Lcu_h/4); k++)
            {
                UINT32 uiHwAddr1 = uiHwAddr,m1 = 0;
                for(m = 0; m < 256; m++)
                {
                    uiCS += bit_reverse(pucSw[uiSwAddr+m]);
                    uiCnt++;
                    n = tbl_4x2_c[m % 128] + ((m/128)*128);
                    if(pucSw[uiSwAddr+n] != pucHw[uiHwAddr1+m1])
                    {
                        DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare Chroma Rec Error! (ec_off)\r\n",(int)pat->uiSrcNum,(int)pat->uiPatNum,(int)pat->uiPicNum);
                        DBG_INFO("Error at uiY= %d, i = %d %d, k = %d %d, swaddr = %d %d\r\n",
                            (int)uiY,(int)i,(int)j,(int)k,(int)m,(int)(uiSwAddr+n),(int)(uiHwAddr1+m1));
                        DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",(int)pucSw[uiSwAddr+n],(int)pucHw[uiHwAddr1+m1]);
                        bRet = FALSE;
                        recErrCnt++;
                        if(recErrCnt > 20)
                            return bRet;
                            //break;
                    }
                    m1+=2;
                    if((m%32) == 31)
                    {
                        UINT32 c;
                        c = m/32;
                        if(c < 3){
                            //cb
                            uiHwAddr1 = uiHwAddr + ((c+1) * w);
                            m1 = 0;
                        }else{
                            //cr
                            uiHwAddr1 = uiHwAddr + ((c-3) * w);
                            m1 = 1;
                        }
                    }
                } // end of m
                uiSwAddr += 256;
                uiHwAddr += (4*w);
            } // end of k

            //DBG_INFO("recErrCnt = %d\r\n", recErrCnt);
        } // end of j
    } // end of i

    //DBG_INFO("¡¹¡¹¡¹ CS :0x%08x, cnt = %d\r\n",uiCS,uiCnt);

    return bRet;

}
#endif

BOOL emu_h265_compare_rec(h26x_job_t *p_job)
{
    BOOL bRet = TRUE;
#if EMU_CMP_REC
    h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
    h265_emu_t *emu = &ctx->emu;
    H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
    H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
    UINT8        *pucSwRecYAddr,*pucSwRecUVAddr,*pucHwRecYAddr,*pucHwRecUVAddr;
    UINT32      uiTileWidth;

    pucSwRecYAddr = (UINT8 *)sH26XEncRec;
    pucSwRecUVAddr = (UINT8 *)((UINT32)pucSwRecYAddr + emu->uiFrameSize);
    pucHwRecYAddr = (UINT8 *)pH26XVirEncAddr->uiRefAndRecYAddr[pH26XVirEncAddr->uiRecIdx];
    pucHwRecUVAddr = (UINT8 *)pH26XVirEncAddr->uiRefAndRecUVAddr[pH26XVirEncAddr->uiRecIdx];

    emu_h265_read_rec(emu);

    uiTileWidth = (pH26XRegCfg->uiH26XTile0 & 0x1)? (pH26XRegCfg->uiH26XTile0 >> 16): SIZE_64X(emu->uiWidth);

    if(pH26XVirEncAddr->uiRecExtraEnable & 0x2)
    {
        CopyRecToTmpBuf( p_job, pucHwRecYAddr, pucHwRecUVAddr, uiTileWidth, emu->uiEcEnable);
        CopyExtraToRec( p_job,pucHwRecYAddr, pucHwRecUVAddr, uiTileWidth, emu->uiEcEnable);
    }

    if(emu->uiEcEnable == 0)
    {
        bRet = emu_h265_compare_rec_ecoff(p_job);
    }else{
        bRet = emu_h265_compare_rec_econ(p_job);
    }

    {
        //extern void emu_h265_dump_hwrec(h26x_job_t *p_job);
        //emu_h265_dump_hwrec(p_job);
    }


    if(pH26XVirEncAddr->uiRecExtraEnable & 0x2)
    {
        CopyBackRecToTmpBuf( p_job, pucHwRecYAddr, pucHwRecUVAddr, uiTileWidth, emu->uiEcEnable);
    }
#endif
    return bRet;
}
#if 0
BOOL emu_h265_compare_picdata_sum(h26x_job_t *p_job)
{
		BOOL bRet = TRUE;

		h265_ctx_t *ctx = (h265_ctx_t *)p_job->ctx_addr;
		h265_emu_t *emu = &ctx->emu;
		H26XEncPicCfg *pH26XEncPicCfg = &emu->sH26XEncPicCfg;
		//H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
		H26XEncRegCfg *pH26XRegCfg = &pH26XEncPicCfg->uiH26XRegCfg;
		h265_pat_t *pat = &ctx->pat;
    #if 0
		DBG_INFO("TMNR_Y SW:0x%08x, HW:0x%08x\r\n",pH26XRegCfg->uiH26X_TMNR_Y_SUM,h26x_getTmnrSumY());
        DBG_INFO("TMNR_C SW:0x%08x, HW:0x%08x\r\n",pH26XRegCfg->uiH26X_TMNR_C_SUM,h26x_getTmnrSumC());
		DBG_INFO("MASK_Y SW:0x%08x, HW:0x%08x\r\n",pH26XRegCfg->uiH26X_MASK_Y_SUM,h26x_getMaskSumY());
		DBG_INFO("MASK_C SW:0x%08x, HW:0x%08x\r\n",pH26XRegCfg->uiH26X_MASK_C_SUM,h26x_getMaskSumC());
		DBG_INFO("OSG_Y SW:0x%08x, HW:0x%08x\r\n",pH26XRegCfg->uiH26X_OSG_Y_SUM,h26x_getOsgSumY());
		DBG_INFO("OSG_C SW:0x%08x, HW:0x%08x\r\n",pH26XRegCfg->uiH26X_OSG_C_SUM,h26x_getOsgSumC());
    #endif

	if (pH26XRegCfg->uiH26X_TMNR_Y_SUM != h26x_getTmnrSumY()){
		DBG_INFO("\r\n");
		DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare enc_state_sel(68) TMNR Y Chksum Error!\r\n",pat->uiSrcNum,pat->uiPatNum,pat->uiPicNum);
		DBG_INFO("SW:0x%08x, HW:0x%08x\r\n",pH26XRegCfg->uiH26X_TMNR_Y_SUM,h26x_getTmnrSumY());
		bRet = FALSE;
	}
	if (pH26XRegCfg->uiH26X_TMNR_C_SUM != h26x_getTmnrSumC()){
		DBG_INFO("\r\n");
		DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare enc_state_sel(69) TMNR C Chksum Error!\r\n",pat->uiSrcNum,pat->uiPatNum,pat->uiPicNum);
		DBG_INFO("SW:0x%08x, HW:0x%08x\r\n",pH26XRegCfg->uiH26X_TMNR_C_SUM,h26x_getTmnrSumC());
		bRet = FALSE;
	}
	if (pH26XRegCfg->uiH26X_MASK_Y_SUM != h26x_getMaskSumY()){
		DBG_INFO("\r\n");
		DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare enc_state_sel(70) MASK Y Chksum Error!\r\n",pat->uiSrcNum,pat->uiPatNum,pat->uiPicNum);
		DBG_INFO("SW:0x%08x, HW:0x%08x\r\n",pH26XRegCfg->uiH26X_MASK_Y_SUM,h26x_getMaskSumY());
		bRet = FALSE;
	}
	if (pH26XRegCfg->uiH26X_MASK_C_SUM != h26x_getMaskSumC()){
		DBG_INFO("\r\n");
		DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare enc_state_sel(71) MASK C Chksum Error!\r\n",pat->uiSrcNum,pat->uiPatNum,pat->uiPicNum);
		DBG_INFO("SW:0x%08x, HW:0x%08x\r\n",pH26XRegCfg->uiH26X_MASK_C_SUM,h26x_getMaskSumC());
		bRet = FALSE;
	}
	if (pH26XRegCfg->uiH26X_OSG_Y_SUM != h26x_getOsgSumY()){
		DBG_INFO("\r\n");
		DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare enc_state_sel(72) OSG Y Chksum Error!\r\n",pat->uiSrcNum,pat->uiPatNum,pat->uiPicNum);
		DBG_INFO("SW:0x%08x, HW:0x%08x\r\n",pH26XRegCfg->uiH26X_OSG_Y_SUM,h26x_getOsgSumY());
		bRet = FALSE;
	}
	if (pH26XRegCfg->uiH26X_OSG_C_SUM != h26x_getOsgSumC()){
		DBG_INFO("\r\n");
		DBG_INFO("Src[%d] Pat[%d] Pic[%d] Compare enc_state_sel(73) OSG C Chksum Error!\r\n",pat->uiSrcNum,pat->uiPatNum,pat->uiPicNum);
		DBG_INFO("SW:0x%08x, HW:0x%08x\r\n",pH26XRegCfg->uiH26X_OSG_C_SUM,h26x_getOsgSumC());
		bRet = FALSE;
	}
	return bRet;
}
#endif

//#endif //if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)






