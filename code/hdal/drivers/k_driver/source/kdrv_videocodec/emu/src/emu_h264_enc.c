#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "h26xenc_api.h"
#include "h264enc_int.h"
#include "emu_h26x_common.h"
#include "emu_h264_enc.h"
#include "kdrv_vdocdc_dbg.h"
#include "h26x_bitstream.h"

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#define MAX_SLICE_HDR_LEN	(128)
#define CEILING_DIV(NUM, DIV) (((NUM)+(DIV)-1) / (DIV))

#define TEST_PERFORMANCE 0 //PAT_FANDI
#define H264_AUTO_JOB_SEL	(0)
char h264_folder_name[][32] =
{
    "pat_ipp", //For IPP

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

    //[64G-2] : long
    "pat_0121_long_ov",

    //[64G-4] : ov
    "pat_0107_ov",





    "pat_0107_16x_ov",
    "pat_0107_ov",
    "pat_208144_0",
    "pat_fhd_ov",
    "pat_1223_ov",
    "pat_0107_16x_ov",
    "pat_sdec_fhd_1",

    "pat_2048x2048_0",
    "pat_1227_16x_ov",
    "pat_sourcout_3",
    "pat_1218_2mb_fast2",
    "pat_newsdec_3",
 	"pat_size_max",
    "PAT_1128_1",
	"PAT_1225_2",
	"PAT_FANDI",
	"PAT_0115_0",
	"PAT_0115_1",
    "PAT_0401_0", //6

	"PAT_0116_0",
	"PAT_0118_0",
	"PAT_0131_0",
	"PAT_1128_0",

#if 0
	"PAT_TEST", "PAT_GOLDEN_316", "PAT_RRC_0", "PAT_OSD_2", "PAT_MASK_0",
	"PAT_0524_0", "PAT_0525_0", "PAT_0526_0",
#endif
};
pic_t  tmp_pic;
//int stable_bs_length_b[1000];
//int stable_bs_length_b_idx = 0;
//BOOL bstableRet = TRUE;
extern int pp_see_pat;
//int pp_x_val = 0;
//int pp_y_val = 0;

static int setup_init_obj(h264_pat_t *p_pat, h26x_mem_t *p_mem, H264ENC_INIT *pInit, unsigned int apb_addr, h26x_ver_item_t *p_ver_item, h26x_srcd2d_t *p_src_d2d)
{
    int ret = 0;

	memset(pInit, 0, sizeof(H264ENC_INIT));

	h26xFileRead(&p_pat->file.seq, p_pat->seq_obj_size, (UINT32)&p_pat->seq);
    if(p_ver_item->src_out_en)
    {
        p_pat->tmp_big_share_mem_addr = h26x_mem_malloc(p_mem, SIZE_32X(p_pat->seq.width) * SIZE_16X(p_pat->seq.height) * 3 /2);
        //emuh26x_msg(("[%s][%d]tmp_big_share_mem_addr 0x%08x 0x%08x\r\n", __FUNCTION__, __LINE__,p_pat->tmp_big_share_mem_addr,SIZE_32X(p_pat->seq.width) * SIZE_16X(p_pat->seq.height) * 3 /2));
    }else{
        p_pat->tmp_big_share_mem_addr = 0;
    }
//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================
    {
        /*

        h26xFilePreRead(&p_pat->file.pic, p_pat->pic_obj_size, (UINT32)&tmp_pic);
        if(tmp_pic.tnr_en != 0)
        {
            printf("tnr_en = %d not suppport\r\n",tmp_pic.tnr_en);
            return 1;
        }


        if(p_pat->seq.gray_en != 0)
        {
            printf("gray_en = %d not suppport\r\n",p_pat->seq.gray_en);
            return 1;
        }

        if(p_pat->seq.width >= 3000)
        {
            printf("width = %d not suppport\r\n",p_pat->seq.width);
            return 1;
        }
        */
        /*
        ret = check_skip_pattern(p_pat, p_mem, pInit, apb_addr, p_ver_item);
        if(ret)
        {
            printf("check_skip_pattern = %d\r\n",ret);
            return 1;

        }
        */
        /*
        if (p_pat->seq.sdecmps_en == 1)
        {
            printf("not support decmps = %d\r\n",p_pat->seq.sdecmps_en);
            return 1;
        }
        */

    }
//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================


	if (p_pat->seq.sdecmps_en == 0)
	{
		if (p_ver_item->rotate_en)
		{
			if ((p_pat->seq.height % 16) == 0)
			{
				p_pat->rotate = (p_pat->seq.width > 2176) ? ((rand()%2) ? 0 : 3): (rand()%4);
			}
		}

		if (p_ver_item->src_cbcr_iv)
			p_pat->src_cbcr_iv = rand()%2;

		if (p_pat->rotate != 0)
		{
            #if 0
			p_pat->tmp_src_y_addr = h26x_mem_malloc(p_mem, p_pat->seq.width * p_pat->seq.height);
			p_pat->tmp_src_c_addr = h26x_mem_malloc(p_mem, (p_pat->seq.width * p_pat->seq.height)/2);
            #else
            if(p_pat->tmp_big_share_mem_addr == 0){
                p_pat->tmp_big_share_mem_addr = h26x_mem_malloc(p_mem, p_pat->seq.width * p_pat->seq.height * 3 /2);
                //emuh26x_msg(("[%s][%d]tmp_big_share_mem_addr 0x%08x 0x%08x\r\n", __FUNCTION__, __LINE__,p_pat->tmp_big_share_mem_addr,SIZE_32X(p_pat->seq.width) * SIZE_16X(p_pat->seq.height) * 3 /2));
            }
            p_pat->tmp_src_y_addr = p_pat->tmp_big_share_mem_addr;
            p_pat->tmp_src_c_addr = p_pat->tmp_big_share_mem_addr + (p_pat->seq.width * p_pat->seq.height);
            #endif
            if(p_pat->tmp_src_y_addr == 0 || p_pat->tmp_src_c_addr == 0)
            {
                DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
                return 1;
            }
		}
		else
		{
			p_pat->tmp_src_y_addr = 0;
			p_pat->tmp_src_c_addr = 0;
		}
	}

	pInit->uiWidth  = p_pat->seq.width;
	pInit->uiHeight = p_pat->seq.height;
	if(p_ver_item->lofs==0)
	pInit->uiRecLineOffset = SIZE_64X(p_pat->seq.width);
	else
	pInit->uiRecLineOffset = (((rand()%(H264_LINEOFFSET_MAX -  SIZE_64X(p_pat->seq.width)))/16)*16) +  SIZE_64X(p_pat->seq.width);

    //DBG_INFO("uiRecLineOffset %d ==> %d\r\n", (int)p_pat->seq.width,(int)pInit->uiRecLineOffset);
	pInit->uiEncBufAddr = p_mem->addr;
	pInit->uiEncBufSize = p_mem->size;
	pInit->ucDisLFIdc = p_pat->seq.dis_loopfilter_idc;
	pInit->cDBAlpha = p_pat->seq.dblk_alpha;
	pInit->cDBBeta = p_pat->seq.dblk_beta;

	pInit->cChrmQPIdx = p_pat->seq.chrm_qp_offset;
	pInit->cSecChrmQPIdx = p_pat->seq.sec_chrm_qp_offset;

	pInit->ucMaxIQp = p_pat->seq.max_qp;
	pInit->ucMinIQp = p_pat->seq.min_qp;
	pInit->ucMaxPQp = p_pat->seq.max_qp;
	pInit->ucMinPQp = p_pat->seq.min_qp;

	pInit->bFBCEn = p_pat->seq.fbc_en;
	pInit->bGrayEn = p_pat->seq.gray_en;
	pInit->bFastSearchEn = p_pat->seq.fastsearch_en;
	pInit->bGrayColorEn = p_pat->seq.gray_mode_color_en;
	pInit->bHwPaddingEn = p_pat->seq.hw_pad_en;
	pInit->ucRotate = p_src_d2d->src_d2d_en ? p_src_d2d->src_rotate : ((p_pat->seq.sdecmps_en) ? p_pat->seq.sdecmps_rotate : p_pat->rotate);

	pInit->uiAPBAddr = apb_addr;

	// h264 only //
	pInit->eEntropyMode = p_pat->seq.entropy_coding;
	pInit->bTrans8x8En = (p_pat->seq.tran8x8 == 1);
    return ret;
}

static int setup_info_obj(h264_pat_t *p_pat, h26x_mem_t *p_mem, H264ENC_INFO *pInfo, unsigned int src_out_en, unsigned int lofs, h26x_srcd2d_t *p_src_d2d)
{
	if ((p_pat->seq.sdecmps_rotate == 1) || (p_pat->seq.sdecmps_rotate == 2))
	{
		pInfo->uiSrcYLineOffset = lofs==0?SIZE_16X(p_pat->seq.height):H264_LINEOFFSET_MAX;
		pInfo->uiSrcCLineOffset = lofs==0?SIZE_16X(p_pat->seq.height):H264_LINEOFFSET_MAX;
		pInfo->uiSrcYAddr = h26x_mem_malloc(p_mem, pInfo->uiSrcYLineOffset * p_pat->seq.width);
		pInfo->uiSrcCAddr = h26x_mem_malloc(p_mem, (pInfo->uiSrcCLineOffset * p_pat->seq.width)/2);
        if(pInfo->uiSrcCAddr == 0 || pInfo->uiSrcYAddr == 0)
        {
            DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
            return 1;
        }
	}
	else if ((p_pat->rotate == 1) || (p_pat->rotate == 2))
	{
		pInfo->uiSrcYLineOffset = lofs==0?p_pat->seq.height:H264_LINEOFFSET_MAX;
		pInfo->uiSrcCLineOffset = lofs==0?p_pat->seq.height:H264_LINEOFFSET_MAX;
		pInfo->uiSrcYAddr = h26x_mem_malloc(p_mem, pInfo->uiSrcYLineOffset * p_pat->seq.width);
		pInfo->uiSrcCAddr = h26x_mem_malloc(p_mem, (pInfo->uiSrcCLineOffset * p_pat->seq.width)/2);
        if(pInfo->uiSrcCAddr == 0 || pInfo->uiSrcYAddr == 0)
        {
            DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
            return 1;
        }
	}
	else
	{
		pInfo->uiSrcYLineOffset = lofs==0?p_pat->seq.width:H264_LINEOFFSET_MAX;
		pInfo->uiSrcCLineOffset = lofs==0?p_pat->seq.width:H264_LINEOFFSET_MAX;
		pInfo->uiSrcYAddr = h26x_mem_malloc(p_mem, pInfo->uiSrcYLineOffset * p_pat->seq.height);
		pInfo->uiSrcCAddr = h26x_mem_malloc(p_mem, (pInfo->uiSrcCLineOffset * p_pat->seq.height)/2);
        if(pInfo->uiSrcCAddr == 0 || pInfo->uiSrcYAddr == 0)
        {
            DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
            return 1;
        }

	}
	pInfo->uiBsOutBufSize = p_pat->bs_buf_size;
	pInfo->uiBsOutBufAddr = p_pat->bs_buf_addr;

	if (src_out_en)
	{
		pInfo->uiSrcOutYLineOffset = SIZE_32X(p_pat->seq.width);
		pInfo->uiSrcOutCLineOffset = SIZE_32X(p_pat->seq.width);
        #if 0
		pInfo->uiSrcOutYAddr = h26x_mem_malloc(p_mem, pInfo->uiSrcOutYLineOffset * SIZE_16X(p_pat->seq.height));
		pInfo->uiSrcOutCAddr = h26x_mem_malloc(p_mem, (pInfo->uiSrcOutCLineOffset * SIZE_16X(p_pat->seq.height))/2);
        #else
        pInfo->uiSrcOutYAddr = p_pat->tmp_big_share_mem_addr;
        pInfo->uiSrcOutCAddr = p_pat->tmp_big_share_mem_addr + (pInfo->uiSrcOutYLineOffset * SIZE_16X(p_pat->seq.height));
        #endif
        h26x_flushCacheRead(pInfo->uiSrcOutYAddr, pInfo->uiSrcOutYLineOffset*SIZE_16X(p_pat->seq.height));
        h26x_flushCacheRead(pInfo->uiSrcOutYAddr, pInfo->uiSrcOutYLineOffset*SIZE_16X(p_pat->seq.height)/2);

	}
	else
	{
		// just malloc litter buffer for 510 address constrain shall not be set to 0 //
		pInfo->uiSrcOutYAddr = h26x_mem_malloc(p_mem, 128);
		pInfo->uiSrcOutCAddr = h26x_mem_malloc(p_mem, 128);
        if(pInfo->uiSrcOutCAddr == 0 || pInfo->uiSrcOutYAddr == 0)
        {
            DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
            return 1;
        }

	}
	pInfo->uiNalLenOutAddr = h26x_mem_malloc(p_mem, sizeof(unsigned int)*H264ENC_NAL_MAXSIZE);
    h26x_flushCacheRead(pInfo->uiNalLenOutAddr, sizeof(unsigned int)*H264ENC_NAL_MAXSIZE);
    if(pInfo->uiNalLenOutAddr == 0)
    {
        DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
        return 1;
    }
    return 0;
}

static void rotate_y(unsigned int dest_addr, unsigned int src_addr, unsigned int width, unsigned int height, unsigned int line_offset, unsigned int rotate)
{
	unsigned char *dest = (unsigned char *)dest_addr;
	unsigned char *src  = (unsigned char *)src_addr;

	unsigned int w, h;
	unsigned int idx = 0;

	for (h = 0; h < height; h++)
	{
		for (w = 0; w < width; w++)
		{
			if (rotate == 1)		// hw : counter clockwise , sw : clockwise//
				dest[w*line_offset + height - 1 - h] = src[idx];
			else if (rotate == 2)	// hw : clockwise ,  sw : counter clockwise //
				dest[(width - 1 - w)*line_offset + h] = src[idx];
			else if (rotate == 3)	// hw : 180 degree, sw : 180 degree //
				dest[(height - 1 - h)*line_offset + (width - 1 - w)] = src[idx];
			else
				DBG_INFO("rotate(%d) set error\r\n", rotate);

			idx++;
		}
	}
}

static void rotate_c(unsigned int dest_addr, unsigned int src_addr, unsigned int width, unsigned int height, unsigned int line_offset, unsigned int rotate, unsigned int src_cbcr_iv)
{
	unsigned char *dest = (unsigned char *)dest_addr;
	unsigned char *src  = (unsigned char *)src_addr;

	unsigned int w, h;
	unsigned int idx0 = 0, idx1;
	//DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__));
	for (h = 0; h < height; h++)
	{
		for (w = 0; w < width; w++)
		{
			if (rotate == 1)		// hw : counter clockwise , sw : clockwise//
			{
				idx1 = w*line_offset + (height - 1 - h)*2;
				dest[idx1]    = src[idx0 + src_cbcr_iv];
				dest[idx1+ 1] = src[idx0 + (src_cbcr_iv == 0)];
			}
			else if (rotate == 2)	// hw : clockwise ,  sw : counter clockwise //
			{
				idx1 = (width - 1 - w)*line_offset + h*2;
				dest[idx1]    = src[idx0 + src_cbcr_iv];
				dest[idx1+ 1] = src[idx0 + (src_cbcr_iv == 0)];
			}
			else if (rotate == 3)	// hw : 180 degree, sw : 180 degree //
			{
				idx1 = (height - 1 - h)*line_offset + (width - 1 - w)*2;
				dest[idx1]    = src[idx0 + src_cbcr_iv];
				dest[idx1+ 1] = src[idx0 + (src_cbcr_iv == 0)];
			}
			else
				DBG_INFO("rotate(%d) set error\r\n", rotate);

			idx0 += 2;
		}
	}
}

static void cbcr_iv(unsigned int addr, unsigned int width, unsigned int height, unsigned int line_offset)
{
	unsigned char *pxl = (unsigned char *)addr;

	unsigned int w, h, idx;
	unsigned char a, b;

	for (h = 0; h < height; h++)
	{
		for (w = 0; w < width; w++)
		{
			idx = h*line_offset + w*2;
			a = pxl[idx];
			b = pxl[idx+1];
			pxl[idx] = b;
			pxl[idx+1] = a;
		}
	}
}

static void read_src_yuv(h264_pat_t *p_pat, H264ENC_INFO *pInfo, unsigned int lofs)
{

	if (p_pat->seq.sdecmps_en == 1)
	{
        unsigned int src_width = (p_pat->seq.sdecmps_rotate == 1 || p_pat->seq.sdecmps_rotate == 2) ? p_pat->seq.height : p_pat->seq.width;
        unsigned int src_height = (p_pat->seq.sdecmps_rotate == 1 || p_pat->seq.sdecmps_rotate == 2) ? p_pat->seq.width : p_pat->seq.height;
		unsigned int src_frm_size = src_width * src_height;
		unsigned int sde_width    = (SIZE_16X(src_width)*3)/4;
		unsigned int sde_frm_size = sde_width * src_height;
		unsigned int seek_frm_size, read_frm_size, read_width;

		H26XFile *seek_file, *read_file;

		pInfo->bSrcCbCrIv = p_pat->seq.sdecmps_cbcr_iv;
		#if 1
		pInfo->bSrcYDeCmpsEn = rand()%2;
		pInfo->bSrcCDeCmpsEn = rand()%2;
		#elif 1
		pInfo->bSrcYDeCmpsEn = 1;
		pInfo->bSrcCDeCmpsEn = 1;
        #elif 1
		pInfo->bSrcYDeCmpsEn = 0;
		pInfo->bSrcCDeCmpsEn = 0;
        #elif 1
		pInfo->bSrcYDeCmpsEn = rand()%2;
		pInfo->bSrcCDeCmpsEn = rand()%2;
        if(src_width >= 2000 || src_height >= 2000)
        {
            pInfo->bSrcYDeCmpsEn = 0;
            pInfo->bSrcCDeCmpsEn = 0;        }
        #else
		pInfo->bSrcYDeCmpsEn = 0;
		pInfo->bSrcCDeCmpsEn = 0;
		#endif
		DBG_INFO("sdecmps : Rot%d Iv%d, Y%d, C%d, W%d, sdeW%d\r\n", p_pat->seq.sdecmps_rotate, pInfo->bSrcCbCrIv, pInfo->bSrcYDeCmpsEn, pInfo->bSrcCDeCmpsEn, src_width, sde_width);

		if (pInfo->bSrcYDeCmpsEn)
		{
			seek_file = &p_pat->file.src;
			read_file = &p_pat->file.sdebsy;

			seek_frm_size = src_frm_size;
			read_frm_size = sde_frm_size;
			read_width = sde_width;
		}
		else
		{
			seek_file = &p_pat->file.sdebsy;
			read_file = &p_pat->file.src;

			seek_frm_size = sde_frm_size;
			read_frm_size = src_frm_size;
			read_width = src_width;
		}

		h26xFileSeek(seek_file, seek_frm_size, H26XF_SET_CUR);

    	if(lofs==0 || read_width == H264_LINEOFFSET_MAX){
    		pInfo->uiSrcYLineOffset = read_width;
    		//pInfo->uiSrcYLineOffset = SIZE_16X(read_width);
    	}
    	else{
    		pInfo->uiSrcYLineOffset =  (((rand()%(H264_LINEOFFSET_MAX - read_width))/16)*16) + read_width;

            //DBG_INFO("uiSrcYLineOffset %d ==> %d\r\n", (int)read_width,(int)pInfo->uiSrcYLineOffset);
    	}
		if (pInfo->uiSrcYLineOffset == read_width)
		{
			h26xFileReadFlush(read_file, read_frm_size, pInfo->uiSrcYAddr);
		}
		else
		{
			unsigned int h;

			for (h = 0; h < src_height; h++){
                h26xFileRead(read_file, read_width, pInfo->uiSrcYAddr + (h*pInfo->uiSrcYLineOffset));
            }
            h26x_flushCache(pInfo->uiSrcYAddr, SIZE_32X(pInfo->uiSrcYLineOffset*src_height));
		}

		src_frm_size = src_width * src_height/2;
		sde_width    = (SIZE_32X(src_width)*3)/4;
		sde_frm_size = sde_width * src_height/2;

		if (pInfo->bSrcCDeCmpsEn)
		{
			seek_file = &p_pat->file.src;
			read_file = &p_pat->file.sdebsuv;

			seek_frm_size = src_frm_size;
			read_frm_size = sde_frm_size;
			read_width = sde_width;
		}
		else
		{
			seek_file = &p_pat->file.sdebsuv;
			read_file = &p_pat->file.src;

			seek_frm_size = sde_frm_size;
			read_frm_size = src_frm_size;
			read_width = src_width;
		}

		h26xFileSeek(seek_file, seek_frm_size, H26XF_SET_CUR);

    	if(lofs==0 || read_width == H264_LINEOFFSET_MAX){
    		pInfo->uiSrcCLineOffset = read_width;
            //DBG_INFO("uiSrcYLineOffset = %d\r\n", (int)pInfo->uiSrcYLineOffset);
    		//pInfo->uiSrcCLineOffset = SIZE_16X(read_width);
    	}
    	else{
    		pInfo->uiSrcCLineOffset =  (((rand()%(H264_LINEOFFSET_MAX - read_width))/16)*16) + read_width;;
            //DBG_INFO("uiSrcYLineOffset %d ==> %d\r\n", (int)read_width,(int)pInfo->uiSrcYLineOffset);
    	}

		if (pInfo->uiSrcCLineOffset == read_width){
            h26xFileReadFlush(read_file, read_frm_size, pInfo->uiSrcCAddr);
        }else
		{
			unsigned int h;
			for (h = 0; h < src_height/2; h++){
                h26xFileReadFlush(read_file, read_width, pInfo->uiSrcCAddr + (h*pInfo->uiSrcCLineOffset));
            }
            h26x_flushCache(pInfo->uiSrcCAddr, SIZE_32X(pInfo->uiSrcCLineOffset*src_height/2));

		}

		if (pInfo->bSrcCbCrIv == 1 && pInfo->bSrcCDeCmpsEn == 0)
		{
            cbcr_iv(pInfo->uiSrcCAddr, src_width/2, src_height/2, pInfo->uiSrcCLineOffset);
            h26x_flushCache(pInfo->uiSrcCAddr, SIZE_32X(pInfo->uiSrcCLineOffset*src_height/2));
        }
	}
	else
	{
        unsigned int src_width = (p_pat->rotate == 1 || p_pat->rotate == 2) ? p_pat->seq.height : p_pat->seq.width;
        unsigned int src_height = (p_pat->rotate == 1 || p_pat->rotate == 2) ? p_pat->seq.width : p_pat->seq.height;

		pInfo->bSrcCbCrIv = p_pat->src_cbcr_iv;
		pInfo->bSrcYDeCmpsEn = 0;
		pInfo->bSrcCDeCmpsEn = 0;
    	if(lofs==0 || src_width == H264_LINEOFFSET_MAX){
            //DBG_INFO("uiSrcYLineOffset = %d\r\n", (int)pInfo->uiSrcYLineOffset);
            //DBG_INFO("uiSrcCLineOffset = %d\r\n", (int)pInfo->uiSrcCLineOffset);
    	}
    	else{
    		pInfo->uiSrcYLineOffset =  (((rand()%(H264_LINEOFFSET_MAX - src_width))/16)*16) + src_width;
            pInfo->uiSrcCLineOffset =  (((rand()%(H264_LINEOFFSET_MAX - src_width))/16)*16) + src_width;
            //DBG_INFO("uiSrcYLineOffset src_width %d ==> %d\r\n", (int)src_width,(int)pInfo->uiSrcYLineOffset);
            //DBG_INFO("uiSrcCLineOffset src_width %d ==> %d\r\n", (int)src_width,(int)pInfo->uiSrcCLineOffset);

    	}

		if (p_pat->rotate == 0)
		{
			if (pInfo->uiSrcYLineOffset == p_pat->seq.width)
			{
				unsigned int src_frm_size = src_width * src_height;
				h26xFileRead(&p_pat->file.src, src_frm_size, pInfo->uiSrcYAddr);
			}
			else
			{
				unsigned int h;

				for (h = 0; h < src_height; h++)
					h26xFileRead(&p_pat->file.src, src_width, pInfo->uiSrcYAddr + (h*pInfo->uiSrcYLineOffset));

			}
			if (pInfo->uiSrcCLineOffset == p_pat->seq.width)
			{
				unsigned int src_frm_size = src_width * src_height;
				h26xFileRead(&p_pat->file.src, src_frm_size/2, pInfo->uiSrcCAddr);
			}
			else
			{
				unsigned int h;
				for (h = 0; h < src_height/2; h++)
					h26xFileRead(&p_pat->file.src, src_width, pInfo->uiSrcCAddr + (h*pInfo->uiSrcCLineOffset));
			}

			if (p_pat->src_cbcr_iv){
				cbcr_iv(pInfo->uiSrcCAddr, p_pat->seq.width/2, p_pat->seq.height/2, pInfo->uiSrcCLineOffset);
            }
		}
		else
		{
			unsigned int src_frm_size = src_width * src_height;

			h26xFileRead(&p_pat->file.src, src_frm_size, p_pat->tmp_src_y_addr);
			h26xFileRead(&p_pat->file.src, src_frm_size/2, p_pat->tmp_src_c_addr);

			rotate_y(pInfo->uiSrcYAddr, p_pat->tmp_src_y_addr, p_pat->seq.width, p_pat->seq.height, pInfo->uiSrcYLineOffset, p_pat->rotate);
			rotate_c(pInfo->uiSrcCAddr, p_pat->tmp_src_c_addr, p_pat->seq.width/2, p_pat->seq.height/2, pInfo->uiSrcCLineOffset, p_pat->rotate, p_pat->src_cbcr_iv);
		}
		h26x_flushCache(pInfo->uiSrcYAddr, pInfo->uiSrcYLineOffset * src_height);
		h26x_flushCache(pInfo->uiSrcCAddr, pInfo->uiSrcCLineOffset * src_height/2);
	}
}

static void emu_h264_setup_folder(unsigned int folder_idx, h264_folder_t *p_folder, unsigned int end_pat_idx)
{
	H26XFile fp_pat_cnt;
	char file_name[128];

	memset(p_folder, 0, sizeof(h264_folder_t));

	p_folder->idx = folder_idx;
	sprintf(p_folder->name, "A:\\%s", h264_folder_name[p_folder->idx]);
	sprintf(file_name, "%s\\pat_num.dat", p_folder->name);
	h26xFileOpen(&fp_pat_cnt, file_name);
	h26xFileRead(&fp_pat_cnt, sizeof(UINT32), (UINT32)&p_folder->pat_num);
	h26xFileClose(&fp_pat_cnt);

	if (end_pat_idx)
		p_folder->pat_num = end_pat_idx;

	#if 1
	DBG_INFO("folder_name : %s\r\n", (char*)p_folder->name);
	DBG_INFO("pat_num : %d\r\n", (int)p_folder->pat_num);
	#endif
}

static int emu_h264_setup_pat(h264_folder_t *p_folder, unsigned int pat_idx, h264_pat_t *p_pat, unsigned int frm_num)
{
	char file_name[128];
	unsigned int i;

	memset(p_pat, 0, sizeof(h264_pat_t));

	p_pat->idx = pat_idx;
	p_pat->rand_seed = rand()%4096;
	srand(p_pat->rand_seed);

	sprintf(p_pat->name, "%s\\E_%04d", p_folder->name, p_pat->idx);
	sprintf(file_name, "%s\\info_data.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.info, file_name);
	sprintf(file_name, "%s\\es_data.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.es, file_name);
	sprintf(file_name, "%s\\slice_header_len.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.slice_hdr_len, file_name);
	sprintf(file_name, "%s\\dump_slice_header.es", p_pat->name);
	h26xFileOpen(&p_pat->file.slice_hdr_es, file_name);
	sprintf(file_name, "%s\\slice_len.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.slice_len, file_name);
	sprintf(file_name, "%s\\mbqp_data.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.mbqp, file_name);
	sprintf(file_name, "%s\\seq.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.seq, file_name);
	sprintf(file_name, "%s\\pic.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.pic, file_name);
	sprintf(file_name, "%s\\chk.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.chk, file_name);
	sprintf(file_name, "%s\\tmnr_mt.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.tmnr_mt, file_name);
	sprintf(file_name, "%s\\motion_bit.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.motion_bit, file_name);
	sprintf(file_name, "%s\\col_mv_data.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.colmv, file_name);
	sprintf(file_name, "%s\\source_out.dat", p_pat->name);
	h26xFileOpen(&p_pat->file.source_out, file_name);

	for (i = 0; i < 32; i++)
	{
		sprintf(file_name, "%s\\osg_grap_%d.dat", p_pat->name, i);
		h26xFileOpen(&p_pat->file.osg_grap[i], file_name);
	}

	h26xFileRead(&p_pat->file.info, sizeof(info_t), (UINT32)&p_pat->info);

	if (p_pat->info.dump_src_en)
		sprintf(file_name, "%s\\src_yuv.yuv", p_pat->name);
	else{
	#if TEST_PERFORMANCE
		sprintf(file_name, "A:\\src_yuv\\fandi\\%s.pak", p_pat->info.yuv_name);
	#else
		sprintf(file_name, "A:\\src_yuv\\%dx%d\\%s.pak", p_pat->info.width, p_pat->info.height, p_pat->info.yuv_name);
	#endif
	}

	h26xFileOpen(&p_pat->file.src, file_name);

	if (p_pat->info.dump_src_en & 0x2)
	{
		sprintf(file_name, "%s\\y_out.yuv", p_pat->name);
		h26xFileOpen(&p_pat->file.sdebsy, file_name);
		sprintf(file_name, "%s\\uv_out.yuv", p_pat->name);
		h26xFileOpen(&p_pat->file.sdebsuv, file_name);
		sprintf(file_name, "%s\\sdec_reg_data.dat", p_pat->name);
		h26xFileOpen(&p_pat->file.sde_reg, file_name);
	}

	p_pat->info.frame_num = (frm_num) ? frm_num : p_pat->info.frame_num;
	DBG_INFO("frame_num : %d\r\n", (int)p_pat->info.frame_num);

	h26xFileRead(&p_pat->file.seq, sizeof(unsigned int), (UINT32)&p_pat->seq_obj_size);
	h26xFileRead(&p_pat->file.pic, sizeof(unsigned int), (UINT32)&p_pat->pic_obj_size);
	h26xFileRead(&p_pat->file.chk, sizeof(unsigned int), (UINT32)&p_pat->chk_obj_size);


	#if 0
	char cmd[0x1000];

	memset(cmd, 0, sizeof(char)*0x1000);
	h26xFileRead(&p_pat->file.info, sizeof(char)*p_pat->info.cmd_len, (UINT32)cmd);
	DBG_INFO("frm_num  : %d\r\n", p_pat->info.frame_num));
	DBG_INFO("cmd_len  : %d\r\n", p_pat->info.cmd_len));
	DBG_INFO("cmd_name : %s\r\n", cmd));
	#endif
    return 0;
}

static void emu_h264_fpga_seq_hack(h26x_job_t *p_job, h264_pat_t *p_pat)
{
	H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
	seq_t *seq = &p_pat->seq;

	save_to_reg(&pRegSet->FUNC_CFG[0], 1, 15, 1);	// enable dump nalu length //
	save_to_reg(&pRegSet->FUNC_CFG[0], p_pat->src_d2d_en, 20, 1);
	save_to_reg(&pRegSet->FUNC_CFG[0], p_pat->src_d2d_mode, 21, 1);

	save_to_reg(&pRegSet->FUNC_CFG[1], seq->icm_use_rec_pixel, 30, 1);

#if 0
	save_to_reg(&pRegSet->FUNC_CFG[1], seq->flxsr_en, 3, 1);
#elif 0
	save_to_reg(&pRegSet->FUNC_CFG[1], seq->flxsr_en, 22, 2);
	save_to_reg(&pRegSet->FUNC_CFG[1], seq->flxsr_en, 24, 2);
#elif 1
    save_to_reg(&pRegSet->FUNC_CFG[1], seq->flxsr_en == 5, 3, 1);
    save_to_reg(&pRegSet->FUNC_CFG[1], seq->flxsr_en == 9, 22, 1);
#endif
	save_to_reg(&pRegSet->ILF_CFG[0], seq->cabac_init_idc, 28, 2);

	save_to_reg(&pRegSet->AEAD_CFG, (seq->log2_max_fno - 4), 10, 4);
	save_to_reg(&pRegSet->AEAD_CFG, (seq->log2_max_poc - 4), 28, 4);

	save_to_reg(&pRegSet->SCD_CFG, 16383, 0, 13);

	save_to_reg(&pRegSet->SRAQ_CFG[0], seq->sraq_save_dqp_en, 31, 1);

	save_to_reg(&pRegSet->IME_CFG, seq->ime_left_amvp_mode, 0, 1);

	// need check //
	save_to_reg(&pRegSet->FUNC_CFG[1], 1, 0, 1);	// tmvp_en
}

static unsigned int random_gen_slice_header(unsigned char *addr, unsigned int size)
{
	unsigned int i;
	unsigned int chksum = 0;

	for (i = 0; i < size; i++)
	{
		#if 1
		if (rand()%15 == 0 && (i + 3) < size)
		{
			addr[i++] = 0;
			addr[i++] = 0;
			addr[i]   = 3;
			chksum += bit_reverse(3);
		}
		else
		#endif
		{
			addr[i] = rand()%256;
			chksum += bit_reverse(addr[i]);
		}
	}

	if (addr[size - 1] == 0)
	{
	   //printf("random slice last byte = 0\r\n");
	   addr[size - 1] = rand()%255 + 1;
	   chksum += bit_reverse(addr[size - 1]);
	}

	return chksum;
}

static unsigned int gen_buf_chksum(unsigned char *addr, unsigned int size)
{
	unsigned int i;
	unsigned int chksum = 0;

	for (i = 0; i < size; i++)
		chksum += bit_reverse(addr[i]);

	return chksum;
}

static void emu_h264_fpga_pic_hack(h26x_job_t *p_job, unsigned int rnd_slc_hdr, unsigned int cmp_bs_en, unsigned int stable_bs_len)
{
	H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;

	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *pat = &ctx->pat;
	pic_t *pic = &pat->pic;
	unsigned int i;
	unsigned int rnd_hdr_sum = 0, rnd_hdr_len = 0, rnd_hdr_len_align = 0;

	// slice header //
	unsigned int slice_hdr_addr = pat->slice_hdr_buf_addr;
	unsigned int *p_bsdma_addr = (unsigned int *)pat->bsdma_buf_addr;

	h26xFileRead(&pat->file.slice_hdr_es, pic->total_slice_hdr_len, slice_hdr_addr);
	h26xFileRead(&pat->file.slice_hdr_len, sizeof(unsigned int)*pat->slice_number, (UINT32)pat->slice_hdr_len);
	h26xFileRead(&pat->file.slice_len, sizeof(unsigned int)*pat->slice_number, (UINT32)pat->slice_len);

    pat->stable_bs_len = 0;
	if (cmp_bs_en || stable_bs_len)
	{
		pat->stable_bs_len = 0;
		h26xFileRead(&pat->file.es, pat->chk.result[FPGA_BS_LEN], p_job->picbs_addr);
        //stable_bs_length_b_idx = 0;
	}
//    i = h26x_getStableLen();
//    DBG_INFO("[%s][%d] stable_bs_len = %d %d sb = 0x%08x\r\n", __FUNCTION__, __LINE__,pat->stable_bs_len,stable_bs_length_b_idx,
//    (int)i);

	#if 0
	DBG_INFO("org_sum : %08x, %08x\r\n", pat->chk.result[FPGA_BS_CHKSUM], pat->chk.result[FPGA_BS_LEN]));
	DBG_INFO("org_slc : %08x, %08x\r\n", gen_buf_chksum((unsigned char *)slice_hdr_addr, pic->total_slice_hdr_len), pic->total_slice_hdr_len));
	#endif
	if (pat->seq.entropy_coding && rnd_slc_hdr)
	{
		pat->chk.result[FPGA_BS_CHKSUM] -= gen_buf_chksum((unsigned char *)slice_hdr_addr, pic->total_slice_hdr_len);
		pat->chk.result[FPGA_BS_LEN] -= pic->total_slice_hdr_len;

		for (i = 0; i < pat->slice_number; i++)
			pat->slice_len[i] -= (pat->slice_hdr_len[i] & 0xfffffff);
	}


	if (pat->seq.entropy_coding && rnd_slc_hdr)
		p_bsdma_addr[0] = pat->slice_number + 1;
	else
		p_bsdma_addr[0] = pat->slice_number;

	for (i = 0; i < pat->slice_number; i++)
	{
		if (pat->seq.entropy_coding && rnd_slc_hdr)
		{
			pat->slice_hdr_len[i] = rand()%121 + 8;
			rnd_hdr_sum += random_gen_slice_header((unsigned char *)slice_hdr_addr, pat->slice_hdr_len[i]);
			rnd_hdr_len += pat->slice_hdr_len[i];
			rnd_hdr_len_align += ((pat->slice_hdr_len[i] + 3 ) >>2 ) <<2;// word align
			pat->slice_len[i] += pat->slice_hdr_len[i];
			p_bsdma_addr[i*2 + 1] = h26x_getPhyAddr(slice_hdr_addr);
			p_bsdma_addr[i*2 + 2] = (pat->slice_hdr_len[i] + 0x80000000);
		}
		else
		{
			p_bsdma_addr[i*2 + 1] = h26x_getPhyAddr(slice_hdr_addr);
			p_bsdma_addr[i*2 + 2] = pat->slice_hdr_len[i];
		}
		slice_hdr_addr += (pat->slice_hdr_len[i] & 0xfffffff);
	}

	if (pat->seq.entropy_coding && rnd_slc_hdr)
	{
		rnd_hdr_len_align += 4;
		random_gen_slice_header((unsigned char *)slice_hdr_addr, 1);
		p_bsdma_addr[i*2 + 1] = h26x_getPhyAddr(slice_hdr_addr);
		p_bsdma_addr[i*2 + 2] = 1;
		slice_hdr_addr += 1;
	}

	save_to_reg(&pRegSet->BSDMA_CMD_BUF_ADDR, h26x_getPhyAddr(pat->bsdma_buf_addr), 0, 32);
	save_to_reg(&pRegSet->NAL_HDR_LEN_TOTAL_LEN, (pat->seq.entropy_coding && rnd_slc_hdr) ? rnd_hdr_len_align : pic->total_slice_hdr_len, 0, 32);

	h26x_flushCache(pat->slice_hdr_buf_addr, pat->slice_hdr_buf_size);
	h26x_flushCache(pat->bsdma_buf_addr, pat->bsdma_buf_size);
	pat->chk.result[FPGA_BS_CHKSUM] += rnd_hdr_sum;
	pat->chk.result[FPGA_BS_LEN] += rnd_hdr_len;
	#if 0
	DBG_INFO("rnd_sum : %08x, %08x\r\n", pat->chk.result[FPGA_BS_CHKSUM], pat->chk.result[FPGA_BS_LEN]));
	DBG_INFO("rnd_slc : %08x, %08x\r\n", rnd_hdr_sum, rnd_hdr_len));
	#endif
	// need to remove while rrc fix //
	save_to_reg(&pRegSet->RRC_CFG[1], pat->pic.qp, 0, 6);

	//tmp
	save_to_reg(&pRegSet->RRC_CFG[1], 0x1ff , 22, 9);

	//TILE MODE : need clear due to hw linklist read data overwrite those data
	save_to_reg(&pRegSet->TILE_CFG[0], 0, 0, 32);
	save_to_reg(&pRegSet->TILE_CFG[1], 0, 0, 32);
	save_to_reg(&pRegSet->TILE_CFG[2], 0, 0, 32);
	save_to_reg(&pRegSet->TILE_CFG[3], 0, 0, 32);

	//frame time out count, unit: 256T
	save_to_reg(&pRegSet->TIMEOUT_CNT_MAX,  100 * 270 * (pat->seq.width/16) * (pat->seq.height/16)  /256 , 0, 32);


    if((pRegSet->FUNC_CFG[0]>>6)&1){//FRAM_SKIP_EN
        int skip_opt = rand()%2;
        if(pRegSet->GDR_CFG[0]& 1)
        {
            skip_opt = 0;
        }
    	save_to_reg(&pRegSet->FUNC_CFG[0], (UINT32)skip_opt, 24, 1);//FRAM_SKIP_OPT
        //DBG_INFO("skip_opt = %d\r\n", (int)skip_opt);
    }


	save_to_reg(&pRegSet->FUNC_CFG[1], pat->bs_buf_32b, 21, 1);

//set debug unit
#if 0
if( pat->pic_num == 1)
{
    static int cc = 0;
	int x = 1;
	int y = 0;
    //int xx[7] = {226, 230, 234, 290, 294, 298, 302};
    int xx[10] = {3, 7, 226, 230, 234, 290, 294, 298, 302};
    cc = pp_see_pat;
    x = xx[cc];
    pp_x_val = x;
    pp_y_val = y;
    DBG_INFO("#########################################\r\n");
    DBG_INFO("#########################################\r\n");
	DBG_INFO("set dbg unit (x,y) = (%d,%d)\r\n",x,y);
    DBG_INFO("#########################################\r\n");
	save_to_reg(&pRegSet->RES_24C, x, 0, 8);
	save_to_reg(&pRegSet->RES_24C, y, 16, 8);
	//save_to_reg(&pRegSet->RES_24C, 1, 30, 1);//set int
	save_to_reg(&pRegSet->RES_24C, 0, 30, 1);
	save_to_reg(&pRegSet->RES_24C, 1, 31, 1);
	cc++;
}
#endif

}

static void setup_rdo_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	H264EncRdoCfg rdo_cfg;
	rdo_t rdo;

	memset(&rdo, 0, sizeof(rdo_t));
	h26xFileRead(&p_pat->file.seq, p_pat->seq.obj_size[FF_RDO], (UINT32)&rdo);

	int i, j;

	rdo_cfg.bEnable = TRUE;
	rdo_cfg.ucAVC_RATE_EST = rdo.avc_rate_est;

	for (i = 0; i < 3; i++)
		for (j = 0; j< 4; j++)
			rdo_cfg.ucSlope[i][j] = rdo.slope[i][j];

	rdo_cfg.ucI_Y4_CB = rdo.rdo_cost_bias_I4;
	rdo_cfg.ucI_Y8_CB = rdo.rdo_cost_bias_I8;
	rdo_cfg.ucI_Y16_CB = rdo.rdo_cost_bias_I16;
	rdo_cfg.ucP_Y4_CB = rdo.rdo_cost_bias_P4;
	rdo_cfg.ucP_Y8_CB = rdo.rdo_cost_bias_P8;
	rdo_cfg.ucIP_CB_SKIP = rdo.rdo_cost_bias_SKIP;

	rdo_cfg.ucI16_CT_DC = rdo.I16_cost_tweak_DC;
	rdo_cfg.ucI16_CT_H = rdo.I16_cost_tweak_H;
	rdo_cfg.ucI16_CT_V = rdo.I16_cost_tweak_V;
	rdo_cfg.ucI16_CT_P = rdo.I16_cost_tweak_PL;

	rdo_cfg.ucIP_C_CT_DC = rdo.ICm_cost_tweak_DC;
	rdo_cfg.ucIP_C_CT_H = rdo.ICm_cost_tweak_H;
	rdo_cfg.ucIP_C_CT_V = rdo.ICm_cost_tweak_V;
	rdo_cfg.ucIP_C_CT_P = rdo.ICm_cost_tweak_PL;

	rdo_cfg.ucP_Y_COEFF_COST_TH = rdo.luma_coeff_cost;
	rdo_cfg.ucP_C_COEFF_COST_TH = rdo.chroma_coeff_cost;

	h264Enc_setRdoCfg(p_var, &rdo_cfg);
}

static void setup_var_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	H26XEncVarCfg var_cfg;
	var_t var;

	memset(&var, 0, sizeof(var_t));
	h26xFileRead(&p_pat->file.seq, p_pat->seq.obj_size[FF_VAR], (UINT32)&var);

	var_cfg.usThreshold = var.var_t;
	var_cfg.ucAVGMin = var.avg_min;
	var_cfg.ucAVGMax = var.avg_max;
	var_cfg.ucDelta = var.delta;
	var_cfg.ucIRangeDelta = var.i_range_delta;
	var_cfg.ucPRangeDelta = var.p_range_delta;

	var_cfg.ucMerage = 0;	// HEVC only //

	h26XEnc_setVarCfg(p_var, &var_cfg);
}

static void setup_fro_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	H264EncFroCfg fro_cfg;
	fro_t fro;

	memset(&fro, 0, sizeof(fro_t));
	h26xFileRead(&p_pat->file.seq, p_pat->seq.obj_size[FF_FRO], (UINT32)&fro);

	int i, j;

	fro_cfg.bEnable = (BOOL)fro.enable;

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 4; j++)
		{
			fro_cfg.uiDC[i][j] = fro.dc[i][j];
			fro_cfg.ucAC[i][j] = fro.ac[i][j];
			fro_cfg.ucST[i][j] = fro.st[i][j];
			fro_cfg.ucMX[i][j] = fro.mx[i][j];
		}
	}

	h264Enc_setFroCfg(p_var, &fro_cfg);
}

static void setup_mask_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	mask_t mask;

	memset(&mask, 0, sizeof(mask_t));
	h26xFileRead(&p_pat->file.pic, p_pat->seq.obj_size[FF_MASK], (UINT32)&mask);

	unsigned char i, j;

	H26XEncMaskInitCfg mask_init_cfg;

	mask_init_cfg.ucMosaicBlkW = mask.mosaic_blkw;
	mask_init_cfg.ucMosaicBlkH = mask.mosaic_blkh;

	for (i = 0; i < 8; i++)
	{
		mask_init_cfg.ucPalY[i] = mask.pal_y[i];
		mask_init_cfg.ucPalCb[i] = mask.pal_cb[i];
		mask_init_cfg.ucPalCr[i] = mask.pal_cr[i];
	}

	h26xEnc_setMaskInitCfg(p_var, &mask_init_cfg);

	for (i = 0; i < 8; i++)
	{
		H26XEncMaskWinCfg mask_win_cfg;

		mask_win_cfg.bEnable = (mask.win[i].enable == 1);

		mask_win_cfg.ucDid = mask.win[i].did;
		mask_win_cfg.ucPalSel = mask.win[i].pal_sel;
		mask_win_cfg.ucLineHitOpt = mask.win[i].line_hit_op;
		mask_win_cfg.usAlpha = mask.win[i].alpha;

		for (j = 0; j < 4; j++)
		{
			mask_win_cfg.stLine[j].uiCoeffa = mask.win[i].line[j].coeffa;
			mask_win_cfg.stLine[j].uiCoeffb = mask.win[i].line[j].coeffb;
			mask_win_cfg.stLine[j].uiCoeffc = mask.win[i].line[j].coeffc;
			mask_win_cfg.stLine[j].ucComp   = mask.win[i].line[j].comp;
		}

		h26XEnc_setMaskWinCfg(p_var, i, &mask_win_cfg);
	}
}

static void setup_gdr_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	H26XEncGdrCfg gdr_cfg;
	gdr_t gdr;

	memset(&gdr, 0, sizeof(gdr_t));
	h26xFileRead(&p_pat->file.seq, p_pat->seq.obj_size[FF_GDR], (UINT32)&gdr);

	gdr_cfg.bEnable = (BOOL)gdr.enable;
	gdr_cfg.usPeriod = gdr.period;
	gdr_cfg.usNumber = gdr.number;
	gdr_cfg.ucGdrQpEn = gdr.gdr_qp_en;
	gdr_cfg.ucGdrQp = gdr.gdr_qp;
	h26XEnc_setGdrCfg(p_var, &gdr_cfg);
}

static void setup_roi_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	roi_t roi;

	int i;

	memset(&roi, 0, sizeof(roi_t));
	h26xFileRead(&p_pat->file.pic, p_pat->seq.obj_size[FF_ROI], (UINT32)&roi);

	for (i = 0; i < 10; i++)
	{
		H26XEncRoiCfg roi_cfg;

		roi_cfg.bEnable = (BOOL)roi.win[i].enable;

		roi_cfg.usCoord_X = roi.win[i].left;
		roi_cfg.usCoord_Y = roi.win[i].top;
		roi_cfg.usWidth   = roi.win[i].width;
		roi_cfg.usHeight  = roi.win[i].height;
		roi_cfg.cQP       = roi.win[i].qp;
		//roi_cfg.ucMode    = ((roi.win[i].qp_mode) | (roi.win[i].fro_mode<<2) | (roi.win[i].tnr_mode<<4) | (roi.win[i].lpm_mode<<6));
        roi_cfg.ucMode    = ((roi.win[i].qp_mode) | (roi.win[i].fro_mode<<2) | (roi.win[i].bgr_mode <<4) | (roi.win[i].maq_mode <<5) | (roi.win[i].lpm_mode<<6));


		h26XEnc_setRoiCfg(p_var, i, &roi_cfg);
	}
}

static void setup_rrc_seq_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat, unsigned int apb_addr)
{
	H26XEncRowRcCfg rrc_cfg;

	rrc_seq_t rrc_seq;

	memset(&rrc_seq, 0, sizeof(rrc_seq_t));
	h26xFileRead(&p_pat->file.seq, p_pat->seq.obj_size[FF_RRC_SEQ], (UINT32)&rrc_seq);

	p_pat->rrc_chk_en = rrc_seq.chk_en;

	rrc_cfg.bEnable = (BOOL)rrc_seq.enable;
	rrc_cfg.usIPredWt = 0;	// hack to set at each frame //
	rrc_cfg.usPPredWt = 4;	// hack to set at each frame //
	rrc_cfg.usScale   = rrc_seq.scale;
	rrc_cfg.usQPRange = rrc_seq.range;
	rrc_cfg.usQPStep  = rrc_seq.step;
	rrc_cfg.usMinQP   = rrc_seq.min_qp;
	rrc_cfg.usMaxQP   = rrc_seq.max_qp;
    rrc_cfg.usRRCMode = rrc_seq.rrc_mode;

	h26XEnc_setRowRcCfg(p_var, &rrc_cfg);
	// hack : for non-release parameter (usMinCostTh)//
	{
		H26XRegSet *pRegSet = (H26XRegSet *)apb_addr;

		save_to_reg(&pRegSet->RRC_CFG[0], rrc_seq.min_cost_th,   20, 4);	// non-release at api //
		save_to_reg(&pRegSet->RRC_CFG[0], rrc_seq.zero_bit_mode, 24, 3);	// non-release at api //
		save_to_reg(&pRegSet->RRC_CFG[0], 1, 0, 1);	// hack to rrc_cal_en //
		//ECO hack
		save_to_reg(&pRegSet->DEC1_CFG, rrc_seq.ndqp, 16, 1);
		save_to_reg(&pRegSet->DEC1_CFG, rrc_seq.ndqp_range, 0, 5);
		save_to_reg(&pRegSet->DEC1_CFG, rrc_seq.ndqp_step, 8, 5);
	}

}

static void rrc_pic_update(H26XENC_VAR *p_var, h264_pat_t *p_pat, unsigned int apb_addr)
{
	rrc_pic_t rrc_pic;
    H26XEncRowRc rowrc;

	memset(&rrc_pic, 0, sizeof(rrc_pic_t));
	h26xFileRead(&p_pat->file.pic, p_pat->seq.obj_size[FF_RRC_PIC], (UINT32)&rrc_pic);

	/*********************************
	// it shall be set to cfg , but protect internal parameter, then use hack method
	// shall be carefull while using driver internal update parameter
	H26XEncRowRc rrc_obj;

	rrc_obj.usInitQp      = rrc_pic.init_qp;
	rrc_obj.uiPlannedStop = rrc_pic.planned_stop;
	rrc_obj.uiPlannedTop  = rrc_pic.planned_top;
	rrc_obj.uiPlannedBot  = rrc_pic.planned_bot;
	rrc_obj.usRefFrmCoeff = rrc_pic.frm_coeff;
	rrc_obj.uiFrmCostLsb  = rrc_pic.frm_cost_lsb;
	rrc_obj.uiFrmCostMsb  = rrc_pic.frm_cost_msb;
	rrc_obj.uiFrmCmpxLsb  = rrc_pic.frm_cmpx_lsb;
	rrc_obj.uiFrmCmpxMsb  = rrc_pic.frm_cmpx_msb;

	// hack for pred_wt //
	rrc_obj.stCfg.usIPredWt = rrc_pic.pred_wt;
	rrc_obj.stCfg.usPPredWt = rrc_pic.pred_wt;
	*********************************/

	H26XRegSet *pRegSet = (H26XRegSet *)apb_addr;

	if (p_pat->pic_num > 1)
	{
		//save_to_reg(&pRegSet->RRC_CFG[0], 1, 0, 1);	// disable rrc at anytime, but enable while encode picture frame > 1 //
		save_to_reg(&pRegSet->RRC_CFG[0], rrc_pic.enable, 1, 1);	// disable rrc at anytime, but enable while encode picture frame > 1 //
	}

	save_to_reg(&pRegSet->RRC_CFG[0], rrc_pic.pred_wt, 4, 4);

	save_to_reg(&pRegSet->RRC_CFG[1], rrc_pic.init_qp, 0, 6);
	save_to_reg(&pRegSet->RRC_CFG[2], rrc_pic.planned_stop, 0, 32);
	save_to_reg(&pRegSet->RRC_CFG[3], rrc_pic.planned_top,  0, 32);
	save_to_reg(&pRegSet->RRC_CFG[4], rrc_pic.planned_bot,  0, 32);
	save_to_reg(&pRegSet->RRC_CFG[5], rrc_pic.frm_coeff,    0, 16);
	save_to_reg(&pRegSet->RRC_CFG[6], rrc_pic.frm_cost_lsb, 0, 32);
	save_to_reg(&pRegSet->RRC_CFG[7], rrc_pic.frm_cost_msb, 0, 32);
	save_to_reg(&pRegSet->RRC_CFG[8], rrc_pic.frm_cmpx_lsb, 0, 32);
	save_to_reg(&pRegSet->RRC_CFG[9], rrc_pic.frm_cmpx_msb, 0, 32);
    rowrc.uiPlannedTop = rrc_pic.planned_top_init;
    rowrc.usInitQp = rrc_pic.init_qp;
    rowrc.iBeta = rrc_pic.beta;
    rowrc.i_all_ref_pred_tmpl = rrc_pic.i_all_ref_pred_tmpl_init;
    rowrc.iTargetBitsScale = rrc_pic.target_bits_scale;
    h264Enc_setRowRcCfg2( p_var, &rowrc);
}

static void setup_sraq_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	H26XEncAqCfg aq_cfg;

	sraq_t sraq;

	int i;

	memset(&sraq, 0, sizeof(sraq_t));
	h26xFileRead(&p_pat->file.seq, p_pat->seq.obj_size[FF_SRAQ], (UINT32)&sraq);

	aq_cfg.bEnable  = (BOOL)sraq.enable;
	aq_cfg.ucIC2    = sraq.ic2;
	aq_cfg.ucIStr   = sraq.i_str;
	aq_cfg.ucPStr   = sraq.p_str;
	aq_cfg.ucMaxDQp = sraq.max_dqp;
	aq_cfg.ucMinDQp = sraq.min_dqp;
	aq_cfg.ucAslog2 = sraq.i_aslog2;
	aq_cfg.ucDepth  = 2;
	aq_cfg.ucPlaneX = sraq.plane_x;
	aq_cfg.ucPlaneY = sraq.plane_y;

	for (i = 0; i < H26X_AQ_TH_TBL_SIZE; i++)
		aq_cfg.sTh[i] = sraq.th[i];

	h26XEnc_setAqCfg(p_var, &aq_cfg);

	p_pat->aq_chk_en = sraq.enable;
}

static void setup_lpm_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	H26XEncLpmCfg lpm_cfg;

	lpm_t lpm;

	memset(&lpm, 0, sizeof(lpm_t));
	h26xFileRead(&p_pat->file.seq, p_pat->seq.obj_size[FF_LPM], (UINT32)&lpm);

	lpm_cfg.bEnable     = TRUE;
	lpm_cfg.ucRmdSadEn  = lpm.rmd_sad_en;
	lpm_cfg.ucIMEStopEn = lpm.ime_stop_en;
	lpm_cfg.ucIMEStopTh = lpm.ime_stop_th;
	lpm_cfg.ucRdoStopEn = lpm.rdo_stop_en;
	lpm_cfg.ucRdoStopTh = lpm.rdo_stop_th;
	lpm_cfg.ucChrmDmEn  = 0;

	h26XEnc_setLpmCfg(p_var, &lpm_cfg);
}

static void setup_rnd_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	H26XEncRndCfg rnd_cfg;

	rnd_t rnd;

	memset(&rnd, 0, sizeof(rnd_t));
	h26xFileRead(&p_pat->file.pic, p_pat->seq.obj_size[FF_RND], (UINT32)&rnd);

	rnd_cfg.bEnable = (BOOL)rnd.enable;
	rnd_cfg.uiSeed  = rnd.seed;
	rnd_cfg.ucRange = rnd.range;

	h26XEnc_setRndCfg(p_var, &rnd_cfg);
}

static void setup_usrqp_map(H26XENC_VAR *p_var, h264_pat_t *p_pat, unsigned int lofs)
{
	H26XEncUsrQpCfg usrqp_cfg;

	UINT32 total_mbs = ((p_pat->seq.width +15)/16) * ((p_pat->seq.height +15)/16);

	usrqp_cfg.bEnable = TRUE;
    if (lofs && p_pat->seq.width != H264_LINEOFFSET_MAX)
    {
        UINT32 uiQpMapMBWidth = rand()%((H264_LINEOFFSET_MAX/32) - (SIZE_32X(p_pat->seq.width)/32)) + (SIZE_32X(p_pat->seq.width)/32);
        usrqp_cfg.uiQpMapLineOffset = uiQpMapMBWidth*2*sizeof(UINT16);//word align
        usrqp_cfg.uiQpMapAddr = p_pat->mbqp_addr;
        h26XEnc_setUsrQpAddr(p_var,p_pat->mbqp_addr);
    }else{
        usrqp_cfg.uiQpMapAddr = h26XEnc_getUsrQpAddr(p_var);
        usrqp_cfg.uiQpMapLineOffset = (p_pat->seq.width+31)/32*32/16*sizeof(UINT16);//word align
    }

    //DBG_INFO("uiQpMapLineOffset = %d ==> %d, 0x%08x\r\n", (int)(p_pat->seq.width+31)/32*32/16*sizeof(UINT16),(int)usrqp_cfg.uiQpMapLineOffset,(int)usrqp_cfg.uiQpMapAddr);
	if((p_pat->seq.width+31)/32*32 == p_pat->seq.width && (lofs == 0))
		h26xFileReadFlush(&p_pat->file.mbqp, sizeof(UINT16)*total_mbs, usrqp_cfg.uiQpMapAddr);
	else
	{
		unsigned int h;
		for (h = 0; h < ((p_pat->seq.height +15)/16); h++)
		{
			h26xFileRead(&p_pat->file.mbqp, sizeof(UINT16)*((p_pat->seq.width +15)/16), ( usrqp_cfg.uiQpMapAddr + (h*usrqp_cfg.uiQpMapLineOffset)));
        }

        h26x_flushCache(usrqp_cfg.uiQpMapAddr, SIZE_32X(usrqp_cfg.uiQpMapLineOffset*SIZE_16X(p_pat->seq.height)));
	}

	H26XEnc_setUsrQpCfg(p_var, &usrqp_cfg);
}

static void setup_scd_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	H26XEncScdCfg scd_cfg;

	scd_t scd;
	memset(&scd, 0, sizeof(scd_t));

	h26xFileRead(&p_pat->file.seq, p_pat->seq.obj_size[FF_SCD], (UINT32)&scd);

	scd_cfg.bStop = FALSE;
	scd_cfg.usTh  = scd.th;
	scd_cfg.ucSc  = scd.sc;
	scd_cfg.ucOverrideRowRC = scd.override_rrc;

	h26XEnc_setScdCfg(p_var, &scd_cfg);
}
#if 1 // tmnr
static void setup_tmnr_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat, h26x_mem_t *p_mem)
{
	tmnr_t tmnr;
	memset(&tmnr, 0, sizeof(tmnr_t));
	h26xFileRead(&p_pat->file.pic, p_pat->seq.obj_size[FF_TMNR], (UINT32)&tmnr);

}
#endif //tmnr
static void setup_osg_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat, h26x_mem_t *p_mem, int osd_chroma_alpha, unsigned int lofs)
{
	unsigned int i, j;
	osg_t osg;

	memset(&osg, 0, sizeof(osg_t));

	h26xFileRead(&p_pat->file.pic, p_pat->seq.obj_size[FF_OSG], (UINT32)&osg);

	H26XEncOsgRgbCfg osg_rgb_cfg;
    h26XEnc_setOsgChromaAlphaCfg(p_var, osd_chroma_alpha);

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			osg_rgb_cfg.ucRgb2Yuv[i][j] = osg.rgb2yuv[i][j];
		}
	}
	h26XEnc_setOsgRgbCfg(p_var, &osg_rgb_cfg);

	for (i = 0; i < 32; i++)
	{
		unsigned int graph_size_one_line = 0;

		if(osg.win[i].graph.type == 1){
			graph_size_one_line = osg.win[i].graph.width * 4;
		}
		else if(osg.win[i].graph.type == 0 || osg.win[i].graph.type == 2 || osg.win[i].graph.type == 3){
			graph_size_one_line = osg.win[i].graph.width * 2;
		}
		else if(osg.win[i].graph.type == 4){
			graph_size_one_line = SIZE_32X(osg.win[i].graph.width) / 8;
		}
		else if(osg.win[i].graph.type == 5){
			graph_size_one_line = SIZE_16X(osg.win[i].graph.width) / 4;
		}
		else if(osg.win[i].graph.type == 6){
			graph_size_one_line = SIZE_8X(osg.win[i].graph.width) / 2;
		}

		H26XEncOsgWinCfg osg_win_cfg;

		osg_win_cfg.bEnable = osg.win[i].enable;

		osg_win_cfg.stGrap.ucType = osg.win[i].graph.type;
		osg_win_cfg.stGrap.usWidth = osg.win[i].graph.width;
		osg_win_cfg.stGrap.usHeight = osg.win[i].graph.height;
		if(lofs==0){
			unsigned int graph_size = graph_size_one_line * osg.win[i].graph.height;
    		osg_win_cfg.stGrap.usLofs = osg.win[i].graph.lofs;
    		osg_win_cfg.stGrap.uiAddr = h26x_mem_malloc(p_mem, graph_size);

    		if (osg_win_cfg.bEnable)
    		{
    			//DBG_INFO("(win%d) type%d, w%d, h%d, sz%d, %08x\r\n", i, osg_win_cfg.stGrap.ucType, osg_win_cfg.stGrap.usWidth, osg_win_cfg.stGrap.usHeight, graph_size, osg_win_cfg.stGrap.uiAddr);
    			h26xFileReadFlush(&p_pat->file.osg_grap[i], graph_size, osg_win_cfg.stGrap.uiAddr);
    			//h26x_flushCache(osg_win_cfg.stGrap.uiAddr, graph_size);
    		}
		}
		else{

            if(H264_LINEOFFSET_MAX > (osg.win[i].graph.lofs*4)){
                osg_win_cfg.stGrap.usLofs = (((rand()%(H264_LINEOFFSET_MAX - osg.win[i].graph.lofs*4))/4) ) + osg.win[i].graph.lofs ; //unit: word
                osg_win_cfg.stGrap.uiAddr =  h26x_mem_malloc(p_mem, osg_win_cfg.stGrap.usLofs*4*osg.win[i].graph.height );

            }else{
                unsigned int graph_size = graph_size_one_line * osg.win[i].graph.height;
                osg_win_cfg.stGrap.usLofs = osg.win[i].graph.lofs;
                osg_win_cfg.stGrap.uiAddr = h26x_mem_malloc(p_mem, graph_size);
            }
			DBG_INFO("(win%d) osg_win_cfg.stGrap.usLofs = %d ==> %d\r\n",(int)i,(int)osg.win[i].graph.lofs,(int)osg_win_cfg.stGrap.usLofs);


			if (osg_win_cfg.bEnable){
				unsigned int h;

				for (h = 0; h < osg.win[i].graph.height; h++){
					if(h26xFileRead(&p_pat->file.osg_grap[i], graph_size_one_line, osg_win_cfg.stGrap.uiAddr + (h*osg_win_cfg.stGrap.usLofs*4 ))==1){
						break;//read error
					}
				}
				h26x_flushCache(osg_win_cfg.stGrap.uiAddr, osg_win_cfg.stGrap.usLofs*4*osg.win[i].graph.height);
			}

		}

		osg_win_cfg.stDisp.ucMode = osg.win[i].disp.mode;
		osg_win_cfg.stDisp.usXStr = osg.win[i].disp.x_str;
		osg_win_cfg.stDisp.usYStr = osg.win[i].disp.y_str;
		osg_win_cfg.stDisp.ucBgAlpha = osg.win[i].disp.bg_alpha;
		osg_win_cfg.stDisp.ucFgAlpha = osg.win[i].disp.fg_alpha;
		osg_win_cfg.stDisp.ucMaskType = osg.win[i].disp.mask_type;
		osg_win_cfg.stDisp.ucMaskBdSize = osg.win[i].disp.mask_bd_size;
		osg_win_cfg.stDisp.ucMaskY[0] = osg.win[i].disp.mask_y[0];
		osg_win_cfg.stDisp.ucMaskY[1] = osg.win[i].disp.mask_y[1];
		osg_win_cfg.stDisp.ucMaskCb = osg.win[i].disp.mask_cb;
		osg_win_cfg.stDisp.ucMaskCr = osg.win[i].disp.mask_cr;

		osg_win_cfg.stGcac.bEnable = osg.win[i].gcac.enable;
		osg_win_cfg.stGcac.ucBlkWidth = osg.win[i].gcac.blk_width;
		osg_win_cfg.stGcac.ucBlkHeight = osg.win[i].gcac.blk_height;
		osg_win_cfg.stGcac.ucBlkHNum = osg.win[i].gcac.blk_h_num;
		osg_win_cfg.stGcac.ucOrgColorLv = osg.win[i].gcac.org_color_lv;
		osg_win_cfg.stGcac.ucInvColorLv = osg.win[i].gcac.inv_color_lv;
		osg_win_cfg.stGcac.ucNorDiffTh = osg.win[i].gcac.nor_diff_th;
		osg_win_cfg.stGcac.ucInvDiffTh = osg.win[i].gcac.inv_diff_th;
		osg_win_cfg.stGcac.ucStaOnlyMode = osg.win[i].gcac.sta_only_mode;
		osg_win_cfg.stGcac.ucFillEvalMode = osg.win[i].gcac.full_eval_mode;
		osg_win_cfg.stGcac.ucEvalLumTarg = osg.win[i].gcac.eval_lum_targ;

		osg_win_cfg.stQpmap.ucLpmMode = osg.win[i].qpmap.lpm_mode;
		osg_win_cfg.stQpmap.ucTnrMode = osg.win[i].qpmap.tnr_mode;
		osg_win_cfg.stQpmap.ucFroMode = osg.win[i].qpmap.fro_mode;
		osg_win_cfg.stQpmap.ucQpMode = osg.win[i].qpmap.qp_mode;
		osg_win_cfg.stQpmap.cQpVal = osg.win[i].qpmap.qp_val;
		osg_win_cfg.stQpmap.ucBgrMode = osg.win[i].qpmap.bgr_mode;
		osg_win_cfg.stQpmap.ucMaqMode = osg.win[i].qpmap.maq_mode;

		osg_win_cfg.stKey.bEnable = osg.win[i].key.key_en;
		osg_win_cfg.stKey.bAlphaEn = osg.win[i].key.alpha_en;
		osg_win_cfg.stKey.ucAlpha = osg.win[i].key.alpha;
		osg_win_cfg.stKey.ucRed = osg.win[i].key.red;
		osg_win_cfg.stKey.ucGreen = osg.win[i].key.green;
		osg_win_cfg.stKey.ucBlue = osg.win[i].key.blue;

		h26XEnc_setOsgWinCfg(p_var, i, &osg_win_cfg);
	}


	for (i = 0; i < 16; i++)
	{
		H26XEncOsgPalCfg osg_palette_cfg;
		osg_palette_cfg.ucAlpha = osg.palette[i].alpha;
		osg_palette_cfg.ucRed = osg.palette[i].red;
		osg_palette_cfg.ucGreen = osg.palette[i].green;
		osg_palette_cfg.ucBlue = osg.palette[i].blue;
		h26XEnc_setOsgPalCfg(p_var, i, &osg_palette_cfg);
	}

}

static void setup_maq_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat, h26x_mem_t *p_mem)
{
	H26XEncMotionAqCfg maq_cfg;
	unsigned int i;
	maq_t maq;

	memset(&maq, 0, sizeof(maq_t));
	memset(&maq_cfg, 0, sizeof(H26XEncMotionAqCfg));

	h26xFileRead(&p_pat->file.pic, p_pat->seq.obj_size[FF_MAQ], (UINT32)&maq);

	maq_cfg.ucMode = maq.mode;
	maq_cfg.uc8x8to16x16Th = maq.dqp_8x8to16x16_th;
	maq_cfg.ucDqpRoiTh = maq.roi_th;
	maq_cfg.ucDqpnum = maq.dqpnum;
	maq_cfg.ucDqpMotTh = maq.dqp_mot_th;

	for (i = 0; i < 6; i++){
        maq_cfg.cDqp[i] = maq.dqp[i];
    }

	if (p_pat->pic_num == 0)
		h26XEnc_setMotionAqInitCfg(p_var);

	int ring[3] = {0,2,1};
	int idx = 0;
	if(maq_cfg.ucDqpnum == 3)
		idx = ring[p_pat->pic_num%3];
	else if(maq_cfg.ucDqpnum == 2)
		idx = p_pat->pic_num%2;
	else if(maq_cfg.ucDqpnum == 1)
		idx = 0;

	h26xFileReadFlush(&p_pat->file.motion_bit, h26xEnc_getMaqMotSize(p_var), h26xEnc_getMaqMotAddr(p_var,idx) );

	h26XEnc_setMotionAqCfg(p_var, &maq_cfg);
}

static void setup_jnd_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	H26XEncJndCfg jnd_cfg;

	jnd_t jnd;

	memset(&jnd, 0, sizeof(jnd_t));

	h26xFileRead(&p_pat->file.seq, p_pat->seq.obj_size[FF_JND], (UINT32)&jnd);

	jnd_cfg.bEnable = jnd.enable;
	jnd_cfg.ucStr   = jnd.str;
	jnd_cfg.ucLevel = jnd.level;
	jnd_cfg.ucTh    = jnd.th;

	jnd_cfg.ucCStr    = jnd.c_str;
	jnd_cfg.ucR5Flag  = jnd.c_r5_flag;
	jnd_cfg.ucBilaFlag  = jnd.bila_flag;
	jnd_cfg.ucLsigmaTh  = jnd.lsigma_th;
	jnd_cfg.ucLsigma    = jnd.lsigma;

	h26XEnc_setJndCfg(p_var, &jnd_cfg);
}

static void setup_bgr_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	H26XEncBgrCfg bgr_cfg;
	bgr_t bgr;
	unsigned int i;

	memset(&bgr, 0, sizeof(bgr_t));

	h26xFileRead(&p_pat->file.pic, p_pat->seq.obj_size[FF_BGR], (UINT32)&bgr);

	bgr_cfg.bEnable = bgr.enable;
    bgr_cfg.bgr_typ = bgr.bgr_typ;
    for(i = 0; i < 2; i++){
        bgr_cfg.bgr_th[i] = bgr.bgr_th[i];
    }
    for(i = 0; i < 2; i++){
        bgr_cfg.bgr_qp[i] = bgr.bgr_qp[i];
    }
    for(i = 0; i < 2; i++){
        bgr_cfg.bgr_vt[i] = bgr.bgr_vt[i];
    }
    for(i = 0; i < 2; i++){
        bgr_cfg.bgr_dq[i] = bgr.bgr_dq[i];
    }
    for(i = 0; i < 2; i++){
        bgr_cfg.bgr_dth[i] = bgr.bgr_dth[i];
    }
    for(i = 0; i < 2; i++){
        bgr_cfg.bgr_bth[i] = bgr.bgr_bth[i];
    }
	h26XEnc_setBgrCfg(p_var, &bgr_cfg);
}

static void setup_sdec_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	H26XEncSdeCfg sdec_cfg;
	memset(&sdec_cfg, 0, sizeof(H26XEncSdeCfg));
	h26xFileRead(&p_pat->file.sde_reg, sizeof(H26XEncSdeCfg), (UINT32)&sdec_cfg);
	h26XEnc_setSdecCfg(p_var, &sdec_cfg);
}

static void setup_rmd_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	H26XEncRmdCfg rmd_cfg;
	rmd_t rmd;

	memset(&rmd, 0, sizeof(rmd_t));
	h26xFileRead(&p_pat->file.pic, p_pat->seq.obj_size[FF_RMD], (UINT32)&rmd);
	rmd_cfg.ucRmdVh4Y = rmd.rmd_vh4_y;
	rmd_cfg.ucRmdVh8Y = rmd.rmd_vh8_y;
	rmd_cfg.ucRmdVh16Y = rmd.rmd_vh16_y;
	rmd_cfg.ucRmdPl16Y = rmd.rmd_pl16_y;
	rmd_cfg.ucRmdOt4Y = rmd.rmd_ot4_Y;
	rmd_cfg.ucRmdOt8Y = rmd.rmd_ot8_Y;

	h26XEnc_setRmdCfg(p_var, &rmd_cfg);
}

static void setup_tnr_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	H26XEncTnrCfg tnr_cfg;
	tnr_t tnr;
    int i;
    //DBG_INFO("[ERR][%s][%d]\r\n", __FUNCTION__, __LINE__);

	memset(&tnr, 0, sizeof(tnr_t));
	h26xFileRead(&p_pat->file.pic, p_pat->seq.obj_size[FF_TNR], (UINT32)&tnr);

    tnr_cfg.nr_3d_mode = tnr.nr_3d_mode;
    tnr_cfg.tnr_osd_mode = tnr.tnr_osd_mode;
    tnr_cfg.mctf_p2p_pixel_blending = tnr.mctf_p2p_pixel_blending;
    tnr_cfg.tnr_p2p_sad_mode = tnr.tnr_p2p_sad_mode;
    tnr_cfg.tnr_mctf_sad_mode = tnr.tnr_mctf_sad_mode;
    tnr_cfg.tnr_mctf_bias_mode = tnr.tnr_mctf_bias_mode;
    for(i=0;i<3;i++)
    {
        tnr_cfg.nr_3d_adp_th_p2p[i] = tnr.nr_3d_adp_th_p2p[i];
        tnr_cfg.nr_3d_adp_weight_p2p[i] = tnr.nr_3d_adp_weight_p2p[i];
    }
    tnr_cfg.tnr_p2p_border_check_th = tnr.tnr_p2p_border_check_th;
    tnr_cfg.tnr_p2p_border_check_sc = tnr.tnr_p2p_border_check_sc;
    tnr_cfg.tnr_p2p_input = tnr.tnr_p2p_input;
    tnr_cfg.tnr_p2p_input_weight = tnr.tnr_p2p_input_weight;
    tnr_cfg.cur_p2p_mctf_motion_th = tnr.cur_p2p_mctf_motion_th;
    tnr_cfg.ref_p2p_mctf_motion_th = tnr.ref_p2p_mctf_motion_th;
    for (i = 0; i < 4; i++)
    {
        tnr_cfg.tnr_p2p_mctf_motion_wt[i] = tnr.tnr_p2p_mctf_motion_wt[i];
    }
    for (i = 0; i < 3; i++)
    {
        tnr_cfg.nr3d_temporal_spatial_y[i] = tnr.nr3d_temporal_spatial_y[i];
        tnr_cfg.nr3d_temporal_spatial_c[i] = tnr.nr3d_temporal_spatial_c[i];
        tnr_cfg.nr3d_temporal_range_y[i] = tnr.nr3d_temporal_range_y[i];
        tnr_cfg.nr3d_temporal_range_c[i] = tnr.nr3d_temporal_range_c[i];
    }

    tnr_cfg.nr3d_clampy_th = tnr.nr3d_clampy_th;
    tnr_cfg.nr3d_clampy_div = tnr.nr3d_clampy_div;
    tnr_cfg.nr3d_clampc_th = tnr.nr3d_clampc_th;
    tnr_cfg.nr3d_clampc_div = tnr.nr3d_clampc_div;
    for (i = 0; i < 3; i++)
    {
        tnr_cfg.nr3d_temporal_spatial_y_mctf[i] = tnr.nr3d_temporal_spatial_y_mctf[i];
        tnr_cfg.nr3d_temporal_spatial_c_mctf[i] = tnr.nr3d_temporal_spatial_c_mctf[i];
        tnr_cfg.nr3d_temporal_range_y_mctf[i] = tnr.nr3d_temporal_range_y_mctf[i];
        tnr_cfg.nr3d_temporal_range_c_mctf[i] = tnr.nr3d_temporal_range_c_mctf[i];

    }
    tnr_cfg.nr3d_clampy_th_mctf = tnr.nr3d_clampy_th_mctf;
    tnr_cfg.nr3d_clampy_div_mctf = tnr.nr3d_clampy_div_mctf;
    tnr_cfg.nr3d_clampc_th_mctf = tnr.nr3d_clampc_th_mctf;
    tnr_cfg.nr3d_clampc_div_mctf = tnr.nr3d_clampc_div_mctf;

    tnr_cfg.cur_motion_rat_th = tnr.cur_motion_rat_th;
    tnr_cfg.cur_motion_sad_th = tnr.cur_motion_sad_th;

    for (i = 0; i < 2; i++)
    {
        tnr_cfg.ref_motion_twr_p2p_th[i] = tnr.ref_motion_twr_p2p_th[i];
        tnr_cfg.cur_motion_twr_p2p_th[i] = tnr.cur_motion_twr_p2p_th[i];
        tnr_cfg.ref_motion_twr_mctf_th[i] = tnr.ref_motion_twr_mctf_th[i];
        tnr_cfg.cur_motion_twr_mctf_th[i] = tnr.cur_motion_twr_mctf_th[i];
    }
    for (i = 0; i < 3; i++)
    {
        tnr_cfg.nr3d_temporal_spatial_y_1[i] = tnr.nr3d_temporal_spatial_y_1[i];
        tnr_cfg.nr3d_temporal_spatial_c_1[i] = tnr.nr3d_temporal_spatial_c_1[i];
        tnr_cfg.nr3d_temporal_spatial_y_mctf_1[i] = tnr.nr3d_temporal_spatial_y_mctf_1[i];
        tnr_cfg.nr3d_temporal_spatial_c_mctf_1[i] = tnr.nr3d_temporal_spatial_c_mctf_1[i];
    }
    for (i = 0; i < 2; i++)
    {
        tnr_cfg.sad_twr_p2p_th[i] = tnr.sad_twr_p2p_th[i];
        tnr_cfg.sad_twr_mctf_th[i] = tnr.sad_twr_mctf_th[i];
    }

	h26XEnc_setTnrCfg(p_var, &tnr_cfg);
}

static void setup_lambda_obj(H26XENC_VAR *p_var, h264_pat_t *p_pat)
{
	H26XEncLambdaCfg lambda_cfg;
	lambda_t lambda;
    int i;

	memset(&lambda, 0, sizeof(lambda_t));
	h26xFileRead(&p_pat->file.pic, p_pat->seq.obj_size[FF_LAMBDA], (UINT32)&lambda);
    lambda_cfg.adaptlambda_en = lambda.adaptlambda_en;
    for(i=0;i<52;i++)
    {
        lambda_cfg.lambda_table[i] = lambda.lambda_table[i];
        lambda_cfg.sqrt_lambda_table[i] = lambda.sqrt_lambda_table[i];
    }
	h26XEnc_setLambdaCfg(p_var, &lambda_cfg);
}

static int emu_h264_init(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item, h26x_srcd2d_t *p_src_d2d)
{
	h26x_mem_t *mem = &p_job->mem;
	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *pat = &ctx->pat;
	h264_emu_t *emu = &ctx->emu;
    int ret = 0;
	p_job->mem.start = p_job->mem_bak.start;
	p_job->mem.addr  = p_job->mem_bak.addr;
	p_job->mem.size  = p_job->mem_bak.size;

	memset(emu, 0, sizeof(h264_emu_t));

	if (p_ver_item->write_prot)
	{
		h26x_disable_wp();
	}
	ret = setup_init_obj(pat, mem, &emu->init_obj, p_job->apb_addr, p_ver_item, p_src_d2d);
    {
        if(ret){
            pat->idx++;
            return ret;
        }
    }

	emu->var_obj.eCodecType = VCODEC_H264;
	emu->var_obj.uiEncId = p_job->idx1;

	if (h264Enc_initEncoder(&emu->init_obj, &emu->var_obj) != H26XENC_SUCCESS)
	{
		DBG_INFO("[ERR][%s][%d]\r\n", __FUNCTION__, __LINE__);
        return 1;
    }

	mem->addr += SIZE_32X(emu->var_obj.uiCtxSize);
	mem->size -= SIZE_32X(emu->var_obj.uiCtxSize);

	// need to modify to sync random slice header //
	unsigned int max_slice_num = (pat->seq.height + 15)/16;

	pat->slice_number = (pat->seq.slice_row_num) ? ((max_slice_num + pat->seq.slice_row_num - 1)/pat->seq.slice_row_num) : 1;
	#if 1
	if(pat->seq.entropy_coding && p_ver_item->rnd_slc_hdr)
	pat->bsdma_buf_size = SIZE_32X(((pat->slice_number+1)*2 + 1)*4);
	else
	pat->bsdma_buf_size = SIZE_32X((pat->slice_number*2 + 1)*4);
	#else
	pat->bsdma_buf_size = SIZE_32X((H26X_MAX_BSDMA_NUM*2 + 1)*4);	 // malloc  maximum bsdma size //
	#endif
	pat->bsdma_buf_addr = h26x_mem_malloc(mem, pat->bsdma_buf_size);
    if(pat->bsdma_buf_addr == 0)
    {
        DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
        return 1;
    }

	pat->slice_hdr_buf_size = pat->slice_number * MAX_SLICE_HDR_LEN;
	pat->slice_hdr_buf_addr = h26x_mem_malloc(mem, pat->slice_hdr_buf_size);
    if(pat->slice_hdr_buf_addr == 0)
    {
        DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
        return 1;
    }

/*	pat->bs_buf_size = (pat->seq.width < 1280) ? (pat->seq.width * pat->seq.height * 3) :
					   (pat->seq.width < 1920) ? (pat->seq.width * pat->seq.height * 2) : (pat->seq.width * pat->seq.height);
*/
	pat->bs_buf_size = pat->seq.width * pat->seq.height * 3;
	pat->bs_buf_size += pat->slice_hdr_buf_size;
	pat->bs_buf_addr = h26x_mem_malloc(mem, pat->bs_buf_size);
    if(pat->bs_buf_addr == 0)
    {
        DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
        return 1;
    }
	if (p_ver_item->lofs)
	{
        UINT32 uiSizeUsrQpMap = sizeof(UINT16)*(((SIZE_32X(H264_LINEOFFSET_MAX)+15)/16)*((pat->seq.height+15)/16));
        pat->mbqp_addr = h26x_mem_malloc(mem, uiSizeUsrQpMap);
        if(pat->mbqp_addr == 0)
        {
            DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
            return 1;
        }

    }
	pat->src_d2d_en = p_src_d2d->src_d2d_en;
	pat->src_d2d_mode = p_src_d2d->src_d2d_mode;
	if(p_src_d2d->is_hevc==1)DBG_INFO("codec set error!\r\n");


	if (p_ver_item->rnd_sw_rest)
	{
		UINT32 uiFrmSize      = emu->init_obj.uiRecLineOffset * SIZE_16X(emu->init_obj.uiHeight);

		p_job->tmp_rec_y_addr = h26x_getPhyAddr(h26x_mem_malloc(mem, uiFrmSize));
		p_job->tmp_rec_c_addr = h26x_getPhyAddr(h26x_mem_malloc(mem, uiFrmSize/2));
		p_job->tmp_bs_addr = h26x_getPhyAddr(h26x_mem_malloc(mem, pat->bs_buf_size));
		p_job->tmp_colmv_addr = p_job->tmp_bs_addr;
		p_job->tmp_sideinfo_addr = p_job->tmp_bs_addr;
	}
	else {

		if(p_ver_item->cmp_bs_en){
			p_job->tmp_bs_addr = h26x_getPhyAddr(h26x_mem_malloc(mem, pat->bs_buf_size));
            if(p_job->tmp_bs_addr == 0)
            {
                DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
                return 1;
            }
		}

		if(cmp_colmv==1){
			UINT32 uiSizeColMVs   = ((pat->seq.width+63)/64)*(( pat->seq.height+15)/16)*64;
			p_job->tmp_colmv_addr = h26x_mem_malloc(mem, uiSizeColMVs);
            if(p_job->tmp_colmv_addr == 0)
            {
                DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
                return 1;
            }
		}
	}
	if(p_ver_item->cmp_bs_en || p_ver_item->stable_bs_len){
		p_job->picbs_addr = h26x_mem_malloc(mem, pat->bs_buf_size);
        if(p_job->picbs_addr == 0)
        {
            DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
            return 1;
        }
	}

	if (p_ver_item->rnd_sw_rest || p_ver_item->rec_out_dis)
	{
		UINT32 uiSizeTmnrRef  = emu->init_obj.uiWidth * SIZE_16X(emu->init_obj.uiHeight);
		UINT32 uiSizeOsgGcac  = 256;

		if (p_ver_item->rnd_sw_rest)
		{
			p_job->tmp_tmnr_rec_y_addr = p_job->tmp_rec_y_addr;
			p_job->tmp_tmnr_rec_c_addr = p_job->tmp_rec_c_addr;
		}
		else
		{
			p_job->tmp_tmnr_rec_y_addr = h26x_getPhyAddr(h26x_mem_malloc(mem, uiSizeTmnrRef));
			p_job->tmp_tmnr_rec_c_addr = h26x_getPhyAddr(h26x_mem_malloc(mem, uiSizeTmnrRef/2));
            if(p_job->tmp_tmnr_rec_y_addr == 0 || p_job->tmp_tmnr_rec_c_addr == 0)
            {
                DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
                return 1;
            }
		}
		p_job->tmp_tmnr_mtout_addr = p_job->tmp_tmnr_rec_y_addr;
		p_job->tmp_osg_gcac_addr0 = h26x_getPhyAddr(h26x_mem_malloc(mem, uiSizeOsgGcac));
		p_job->tmp_osg_gcac_addr1 = h26x_getPhyAddr(h26x_mem_malloc(mem, uiSizeOsgGcac));
        if(p_job->tmp_osg_gcac_addr0 == 0 || p_job->tmp_osg_gcac_addr1 == 0)
        {
            DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
            return 1;
        }
	}
	if (p_ver_item->src_out_en == 1)
	{
#if cmp_sourceout_onebyone
        UINT32 uiSize = SIZE_16X(pat->seq.width) * SIZE_16X(pat->seq.height);
        pat->source_out_y_tmp_addr = h26x_getPhyAddr(h26x_mem_malloc(mem, uiSize));
        pat->source_out_c_tmp_addr = h26x_getPhyAddr(h26x_mem_malloc(mem, uiSize/2));
        if(pat->source_out_y_tmp_addr == 0 || pat->source_out_c_tmp_addr == 0)
        {
            DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
            return 1;
        }
#endif
    }


	// fpga only //
	emu_h264_fpga_seq_hack(p_job, pat);

	// set function //
	if (pat->seq.obj_size[FF_RDO])  setup_rdo_obj(&emu->var_obj, pat);
	if (pat->seq.obj_size[FF_VAR])  setup_var_obj(&emu->var_obj, pat);
	if (pat->seq.obj_size[FF_FRO])  setup_fro_obj(&emu->var_obj, pat);
	if (pat->seq.obj_size[FF_GDR])  setup_gdr_obj(&emu->var_obj, pat);
	if (pat->seq.obj_size[FF_RRC_SEQ]) setup_rrc_seq_obj(&emu->var_obj, pat, p_job->apb_addr);
	if (pat->seq.obj_size[FF_SRAQ]) setup_sraq_obj(&emu->var_obj, pat);
	if (pat->seq.obj_size[FF_LPM])	setup_lpm_obj(&emu->var_obj, pat);
	if (pat->seq.obj_size[FF_SCD])  setup_scd_obj(&emu->var_obj, pat);
	if (pat->seq.obj_size[FF_JND])  setup_jnd_obj(&emu->var_obj, pat);
	if (pat->info.dump_src_en & 0x2)  setup_sdec_obj(&emu->var_obj, pat);//source decompression

	H26XEnc_setPSNRCfg(&emu->var_obj, TRUE);

	if (pat->seq.slice_row_num)
	{
		H26XEncSliceSplitCfg stSliceSplit;

		stSliceSplit.uiEnable = 1;
		stSliceSplit.uiSliceRowNum = pat->seq.slice_row_num;
		h26XEnc_setSliceSplitCfg(&emu->var_obj, &stSliceSplit);
	}

	if(setup_info_obj(pat, mem, &emu->info_obj, p_ver_item->src_out_en, p_ver_item->lofs, p_src_d2d))
    {
        DBG_INFO("[%s][%d]\r\n", __FUNCTION__, __LINE__);
        return 1;
    }

	#if 0
	// dump pattern message //
	DBG_INFO("\r\njob(%d) pattern_name[%d][%d] : %s\r\n", p_job->idx, ctx->folder.idx, ctx->pat.idx, ctx->pat.name));
	DBG_INFO("seq(%d) : (w:%d, h:%d, loft:%d, rotate:%d, src_iv:%d, mem : %08x, rand_seed:%d)\r\n", pat->seq_obj_size, pat->seq.width, pat->seq.height, emu->init_obj.uiRecLineOffset, pat->rotate, pat->src_cbcr_iv, (mem->addr - mem->start), pat->rand_seed));
	#endif

    return ret;
}

static void emu_h264_close_one_pat(h264_pat_t *p_pat)
{
	int i;

	h26xFileClose(&p_pat->file.info);
	h26xFileClose(&p_pat->file.src);
	h26xFileClose(&p_pat->file.es);
	h26xFileClose(&p_pat->file.slice_hdr_len);
	h26xFileClose(&p_pat->file.slice_hdr_es);
	h26xFileClose(&p_pat->file.slice_len);
	h26xFileClose(&p_pat->file.mbqp);
	h26xFileClose(&p_pat->file.seq);
	h26xFileClose(&p_pat->file.pic);
	h26xFileClose(&p_pat->file.chk);
	h26xFileClose(&p_pat->file.sdebsy);
	h26xFileClose(&p_pat->file.sdebsuv);
	h26xFileClose(&p_pat->file.tmnr_mt);
	h26xFileClose(&p_pat->file.motion_bit);
	h26xFileClose(&p_pat->file.sde_reg);
	h26xFileClose(&p_pat->file.colmv);

	for (i = 0; i < 32; i++)
		h26xFileClose(&p_pat->file.osg_grap[i]);
}

int emu_h264_select_new_pat_folder(h26x_job_t *job, h26x_ver_item_t *p_ver_item)
{
	h264_ctx_t *ctx = (h264_ctx_t *)job->ctx_addr;
    h264_pat_t *pat = &ctx->pat;
    //h264_emu_t *emu = &ctx->emu;
    int ret;
#if H264_AUTO_JOB_SEL
    int start_src,end_src,start_pat,end_pat;
    int diff_src,diff_pat;
    int pat_idx,fold_idx;
    start_src = job->start_folder_idx;
    start_pat = job->start_pat_idx;
    end_src = job->end_folder_idx;
    end_pat = job->end_pat_idx;
    diff_src = end_src - start_src;
    diff_pat = end_pat - start_pat;
    //pat->uiPicNum = 0;
    fold_idx = (h26xRand() % diff_src) + start_src;
    pat_idx = (h26xRand() % diff_pat) + start_pat;
	memset(ctx, 0, sizeof(h264_ctx_t));
    emu_h264_setup_folder(fold_idx, &ctx->folder, job->end_pat_idx);
    ret  = emu_h264_setup_pat(&ctx->folder, pat_idx, &ctx->pat, job->end_frm_num);
    if(ret) return 1;
#else
    int jump_pat = 1;

    if ((pat->idx + jump_pat) < ctx->folder.pat_num)
    {
        ret = emu_h264_setup_pat(&ctx->folder, pat->idx + jump_pat, &ctx->pat, job->end_frm_num);
        if(ret) return 1;
    }else{

        if ((ctx->folder.idx + 1) < job->end_folder_idx)
        {
            emu_h264_setup_folder(ctx->folder.idx + 1, &ctx->folder, job->end_pat_idx);
            ret = emu_h264_setup_pat(&ctx->folder, 0, &ctx->pat, job->end_frm_num);
            if(ret) return 1;
        }else{
            job->is_finish = 1;
            return 1;
        }

    }
#endif

    return 0;
}

static BOOL emu_h264_setup_one_job(h26x_ctrl_t *p_ctrl, unsigned int start_folder_idx, unsigned int end_folder_idx,unsigned int start_pat_idx, unsigned int end_pat_idx, unsigned int end_frm_num, UINT8 pucDir[265], h26x_srcd2d_t *p_src_d2d)
{
	h26x_job_t *job = h26x_job_add(1, p_ctrl);

	if (job == NULL)
		return FALSE;

	job->ctx_addr = h26x_mem_malloc(&job->mem, sizeof(h264_ctx_t));

	job->mem.size -= (job->mem.addr - job->mem.start);
	job->mem.start = job->mem.addr;

	job->mem_bak.start = job->mem.start;
	job->mem_bak.addr  = job->mem.addr;
	job->mem_bak.size  = job->mem.size;

	h264_ctx_t *ctx = (h264_ctx_t *)job->ctx_addr;
	memset(ctx, 0, sizeof(h264_ctx_t));

	job->start_folder_idx = start_folder_idx;
	job->end_folder_idx = end_folder_idx;
	job->start_pat_idx = start_pat_idx;
	job->end_pat_idx = end_pat_idx;
	job->end_frm_num = end_frm_num;

	if(p_src_d2d->src_d2d_en){ //For IPP
		memcpy((void*)h264_folder_name[0], pucDir, sizeof(h264_folder_name[0]));
	}

	emu_h264_setup_folder(job->start_folder_idx, &ctx->folder, job->end_pat_idx);
	emu_h264_setup_pat(&ctx->folder, job->start_pat_idx, &ctx->pat, job->end_frm_num);
    h264_pat_t *pat = &ctx->pat;
#if 0
	emu_h264_init(job, &p_ctrl->ver_item);
#else

    while(emu_h264_init(job, &p_ctrl->ver_item, p_src_d2d) == 1)
    {
        emu_h264_close_one_pat(pat);
        if(emu_h264_select_new_pat_folder(job, &p_ctrl->ver_item) == 1)
        {
            emu_h264_close_one_pat(pat);
            return FALSE;
        }
    }
#endif
	return TRUE;
}
extern int pp_see_pat;
BOOL emu_h264_setup(h26x_ctrl_t *p_ctrl, UINT8 pucDir[265], h26x_srcd2d_t *p_src_d2d)
{
	BOOL ret = TRUE;
	unsigned int job_idx = 1;

    //int see = 2;
    //int cnt = 1;
	if(p_src_d2d->src_d2d_en) job_idx = 0; // For IPP
	ret &= emu_h264_setup_one_job(p_ctrl, job_idx, job_idx + 1, 0, 0, 0,(UINT8*)pucDir,p_src_d2d);
	//ret &= emu_h264_setup_one_job(p_ctrl, job_idx, job_idx + 1, 210, 0, 0,(UINT8*)pucDir,p_src_d2d);
	//ret &= emu_h264_setup_one_job(p_ctrl, job_idx, job_idx + 1, 0, 0, 0,(UINT8*)pucDir,p_src_d2d);
    //ret &= emu_h264_setup_one_job(p_ctrl, job_idx, job_idx + 3, 0, 0, 0,(UINT8*)pucDir,p_src_d2d); //[32G-1] : small size

    //ret &= emu_h264_setup_one_job(p_ctrl, job_idx, job_idx + 5, 0, 0, 0,(UINT8*)pucDir,p_src_d2d); //[32G-7] : 16x + some_size

	//ret &= emu_h264_setup_one_job(p_ctrl, job_idx, job_idx + 1, 205, 0, 3,(UINT8*)pucDir,p_src_d2d);
	//ret &= emu_h264_setup_one_job(p_ctrl, job_idx, job_idx + 1, 0, 0, 3,(UINT8*)pucDir,p_src_d2d);
	//ret &= emu_h264_setup_one_job(p_ctrl, job_idx, job_idx + 1, 198, 199, 0,(UINT8*)pucDir,p_src_d2d);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 0, 0, 3);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 0, 0, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 201, 0, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 206, 207, 1);
	//ret &= emu_h264_setup_one_job(p_ctrl, 1, 2, 0, 0, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 2, 0, 0, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 1, 2, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 2, 0, 0, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 33, 0, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 31, 32, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 30, 0, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 38, 0, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 2, 3, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 1, 3, 2);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 101, 102, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 0, 1, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 2, 6, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 20, 21, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 0, 90, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, pp_see_pat, 0, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, see, see+cnt, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 3, 4, 0);
	//ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 15, 21, 0);
    //ret &= emu_h264_setup_one_job(p_ctrl, 0, 1, 27, 28, 0);

	return ret;
}


BOOL emu_h264_do_recout_flow(h26x_ctrl_t *p_ctrl,h26x_job_t *p_job)
{
	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *p_pat = &ctx->pat;
    h26xFilePreRead(&p_pat->file.pic, p_pat->pic_obj_size, (UINT32)&tmp_pic);

    if(tmp_pic.tnr_en){
        return FALSE;
    }
    return TRUE;
}


static void report_perf(h26x_job_t *p_job)
{
	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *pat = &ctx->pat;

	unsigned int mb_num = (pat->seq.width/16) * (pat->seq.height + 15)/16;
	unsigned int frm_avg_clk = pat->perf.cycle_sum/pat->pic_num;
	UINT32 bslen_avg = (float)pat->perf.bslen_sum/pat->pic_num;

	DBG_DUMP("\r\ncycle report (%s, qp(%d)) => max(frm:%d) : c(%d, %.2f) b(%d, %.2f)(%.2f) avg : c(%f, %.2f) b(%f, %.2f)(%.2f) \r\n",
		pat->info.yuv_name, pat->pic.qp, pat->perf.cycle_max_frm,
		pat->perf.cycle_max, (float)pat->perf.cycle_max/mb_num,
		pat->perf.cycle_max_bslen, (float)(pat->perf.cycle_max_bslen<<3)/mb_num, (float)(pat->perf.cycle_max_bslen<<3)/pat->perf.cycle_max,
		(float)frm_avg_clk, (float)frm_avg_clk/mb_num,
		(float)bslen_avg, (float)(bslen_avg*8)/mb_num, (float)(bslen_avg*8)/frm_avg_clk);
}

int emu_h264_prepare_one_pic(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item, h26x_srcd2d_t *p_src_d2d)
{
	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *pat = &ctx->pat;
	h264_emu_t *emu = &ctx->emu;

	if (pat->pic_num == pat->info.frame_num)
	{
		report_perf(p_job);

        do{

            emu_h264_close_one_pat(pat);

            if(emu_h264_select_new_pat_folder(p_job, p_ver_item) == 1)
            {
                emu_h264_close_one_pat(pat);
                return 1;
            }

        }while(emu_h264_init(p_job, p_ver_item, p_src_d2d) == 1);

	}
    if(p_ver_item->write_prot){
        //int size,addr;
        //h26x_test_wp(1 , h26x_getPhyAddr(emu->info_obj.uiSrcYAddr), 256, pp_see_pat);
        //h26x_test_wp(5 , wp[1]-256-0x60000000, 256, pp_see_pat);
        //size = wp[1]-0x63000000-1024;
        //size = wp[1]-0x63000000;
        //addr = h26x_getPhyAddr(0x03000000);
        //addr = 0x03000000;
        //addr = h26x_getPhyAddr(emu->info_obj.uiSrcYAddr);
        //size = 0x00010000;
        //h26x_test_wp_2(pp_see_pat/4 , addr, size, pp_see_pat%4);
        //h26x_test_wp(5 , wp[1], 1024, pp_see_pat);
        //h26x_test_wp(5 , h26x_getPhyAddr(emu->info_obj.uiSrcYAddr), 256, pp_see_pat);
        //h26x_test_wp(5 , h26x_getPhyAddr(emu->info_obj.uiSrcYAddr)+256, 0x00100000, pp_see_pat);
    }

	if(pat->src_d2d_en == 0)
	{
		read_src_yuv(pat, &emu->info_obj,p_ver_item->lofs);
	}
	else{
		emu->info_obj.uiSrcYLineOffset = p_src_d2d->src_y_lineoffset;
		emu->info_obj.uiSrcCLineOffset = p_src_d2d->src_c_lineoffset;
		emu->info_obj.uiSrcYAddr = p_src_d2d->src_y_addr;
		emu->info_obj.uiSrcCAddr = p_src_d2d->src_c_addr;

		emu->info_obj.bSrcYDeCmpsEn = p_src_d2d->src_cmp_luma_en;
		emu->info_obj.bSrcCDeCmpsEn = p_src_d2d->src_cmp_crma_en;
		H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
		if(p_src_d2d->src_cmp_luma_en || p_src_d2d->src_cmp_crma_en){
			int i;
			for(i=0;i<45;i++){
				pRegSet->SDE_CFG[i] = p_src_d2d->sdec[i];
			}
		}
		else{
			UINT32 uiSize = sizeof(UINT32) * 45;
			memset((void*)pRegSet->SDE_CFG, 0, uiSize);
		}

		DBG_DUMP("pInfo->uiSrcYLineOffset%08x uiSrcCLineOffset%08x\r\n", (int)emu->info_obj.uiSrcYLineOffset,(int)emu->info_obj.uiSrcCLineOffset);
		DBG_DUMP("pInfo->uiSrcYAddr=0x%08x uiSrcCAddr=0x%08x\r\n", (int)emu->info_obj.uiSrcYAddr,(int)emu->info_obj.uiSrcCAddr);
		DBG_DUMP("phys pInfo->uiSrcYAddr=0x%08x uiSrcCAddr=0x%08x\r\n", (int)h26x_getPhyAddr(emu->info_obj.uiSrcYAddr), (int)h26x_getPhyAddr(emu->info_obj.uiSrcCAddr));
	}

	h26xFileRead(&pat->file.chk, pat->chk_obj_size, (UINT32)&pat->chk);
	h26xFileRead(&pat->file.pic, pat->pic_obj_size, (UINT32)&pat->pic);

	emu->info_obj.ePicType = pat->pic.slice_type;
	emu->info_obj.uiBsOutBufAddr = pat->bs_buf_addr;
	emu->info_obj.bSkipFrmEn = pat->pic.skipfrm_en;

	if (p_ver_item->src_out_en == 1)
	{
	#if 0
		emu->info_obj.bSrcOutEn = rand()%2;
		emu->info_obj.ucSrcOutMode = rand()%2;
	#elif 0
		emu->info_obj.bSrcOutEn = 0;
		emu->info_obj.ucSrcOutMode = 0;
    #elif 1
        emu->info_obj.bSrcOutEn = 1;
        //emu->info_obj.ucSrcOutMode = rand()%2;
        emu->info_obj.ucSrcOutMode = 0;
	#endif

	}
    if(emu->info_obj.bSrcOutEn)
    {
        //because rotate use tmp_src_y_addr = tmp_big_share_mem_addr = source out buffer, so need to invalid cache data
        h26x_flushCacheRead(emu->info_obj.uiSrcOutYAddr, emu->info_obj.uiSrcOutYLineOffset*SIZE_16X(pat->seq.height));
        h26x_flushCacheRead(emu->info_obj.uiSrcOutCAddr, emu->info_obj.uiSrcOutYLineOffset*SIZE_16X(pat->seq.height)/2);
    }

	if(p_ver_item->rnd_bs_buf_32b)
	{
		//pat->bs_buf_32b = 1;//rand()%2;
		pat->bs_buf_32b = rand()%2;
	}
	else{
		pat->bs_buf_32b = 0;
	}

	if(p_ver_item->rnd_slc_hdr)
	{
		//pat->rnd_slc_hdr = 1;//rand()%2;
		pat->rnd_slc_hdr = rand()%2;
	}
	else{
		pat->rnd_slc_hdr = 0;
	}

    DBG_INFO("(src_out,bs_32b,rnd_slc) = ( %d, %d, %d)\r\n",emu->info_obj.bSrcOutEn,pat->bs_buf_32b,pat->rnd_slc_hdr);


	if (p_ver_item->rnd_bs_buf)
	{
		unsigned int bs_len;
		if(pat->bs_buf_32b){
		bs_len =  96;//((rand()%(SIZE_32X(pat->chk.result[FPGA_BS_LEN])/32 + 1)) + 1)*32;	//tmppp
		bs_len = (bs_len > pat->bs_buf_size) ? SIZE_32X(pat->bs_buf_size) : bs_len;
		}else{
		bs_len = ((rand()%(SIZE_128X(pat->chk.result[FPGA_BS_LEN])/128 + 1)) + 1)*128;
		bs_len = (bs_len > pat->bs_buf_size) ? SIZE_128X(pat->bs_buf_size) : bs_len;
		}

		p_job->tmp_bs_len = bs_len; // for test random swrest
		emu->info_obj.uiBsOutBufSize = bs_len;


		if(p_ver_item->write_prot)h26x_test_wp(0, emu->info_obj.uiBsOutBufAddr, emu->info_obj.uiBsOutBufSize, rand()%4);

		//DBG_INFO("uiBsOutBufAddr = 0x%08x size: 0x%08x\r\n", emu->info_obj.uiBsOutBufAddr, emu->info_obj.uiBsOutBufSize));
	}else{
    	//if(p_ver_item->write_prot) h26x_test_wp(0, emu->info_obj.uiBsOutBufAddr + emu->info_obj.uiBsOutBufSize, 4);
        if(p_ver_item->write_prot){
            #if 1
            int size,addr;
            addr = h26x_getPhyAddr(emu->info_obj.uiBsOutBufAddr);
            size = 256;
            h26x_test_wp_2(pp_see_pat/4 , addr, size, pp_see_pat%4);
            #endif
        }

    }


	h264Enc_setPicQP(pat->pic.qp, &emu->var_obj);

	if (pat->pic.mask_en) setup_mask_obj(&emu->var_obj, pat);
	if (pat->pic.roi_en)  setup_roi_obj(&emu->var_obj, pat);
	if (pat->pic.rrc_en)  rrc_pic_update(&emu->var_obj, pat, p_job->apb_addr);
	h264Enc_prepareOnePicture(&emu->info_obj, &emu->var_obj);

	emu_h264_fpga_pic_hack(p_job, pat->rnd_slc_hdr,p_ver_item->cmp_bs_en,p_ver_item->stable_bs_len);
	if (pat->pic.rnd_en)  setup_rnd_obj(&emu->var_obj, pat);
	if (pat->pic.mbqp_en) setup_usrqp_map(&emu->var_obj, pat, p_ver_item->lofs);
	if (pat->pic.tmnr_en) setup_tmnr_obj(&emu->var_obj, pat, &p_job->mem);
	//if (pat->pic.osg_en && pat->pic_num == 0) setup_osg_obj(&emu->var_obj, pat, &p_job->mem);
	//emuh26x_msg(("(0)%08x\r\n", p_job->mem.addr));
    int osd_chroma_alpha = (pat->pic.osg_en >> 1) & 0x1;
	if (pat->pic.osg_en && pat->pic_num == 0) setup_osg_obj(&emu->var_obj, pat, &p_job->mem,osd_chroma_alpha,p_ver_item->lofs);
	//emuh26x_msg(("(1)%08x\r\n", p_job->mem.addr));
	if (pat->pic.maq_en)  setup_maq_obj(&emu->var_obj, pat, &p_job->mem);
	setup_bgr_obj(&emu->var_obj, pat);
	setup_rmd_obj(&emu->var_obj, pat);

    //DBG_INFO("[ERR][%s][%d]tnr_en = %d\r\n", __FUNCTION__, __LINE__,pat->pic.tnr_en);

	if (pat->pic.tnr_en)  setup_tnr_obj(&emu->var_obj, pat);
    if (pat->pic.lambda_en) setup_lambda_obj(&emu->var_obj, pat);

	if(pat->bs_buf_32b)
		pat->res_bs_size = pat->chk.result[FPGA_BS_LEN] + 32;
	else
		pat->res_bs_size = pat->chk.result[FPGA_BS_LEN] + 128;

#if 0
	memset((void *)pat->bs_buf_addr, 0, pat->bs_buf_size);
	memset((void *)ctx->emu.info_obj.uiNalLenOutAddr, 0, sizeof(unsigned int)*256);

	h26x_flushCache(pat->bs_buf_addr, pat->bs_buf_size);
	h26x_flushCache(ctx->emu.info_obj.uiNalLenOutAddr, sizeof(unsigned int)*256);
#endif

	//DBG_INFO("apb_addr= 0x%08x\r\n", p_job->apb_addr);
	//h26x_prtMem(p_job->apb_addr, h26x_getHwRegSize());

	#if 0
	DBG_INFO("pic : %d, %d, %d, %d\r\n", pat->pic_obj_size, pat->pic.slice_type, pat->pic.qp, pat->pic.tmnr_en));
	#endif

	#if 0
	H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
	int i;

	for (i = 0; i < 60; i++)
		DBG_INFO("(%02x) %08x\r\n", (i*4), pRegSet->TMNR_CFG[i]));
	#endif

	#if 0
	H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
	int i;

	for (i = 0; i < 68; i++)
		DBG_INFO("(%02x) %08x\r\n", (i*4), pRegSet->OSG_CFG[i]));
	#endif
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
			UINT32 test_addr = emu->info_obj.uiSrcYAddr;
			UINT32 test_size;
			if ((pat->seq.sdecmps_rotate == 1) || (pat->seq.sdecmps_rotate == 2) || (pat->rotate == 1) || (pat->rotate == 2))
				test_size = emu->info_obj.uiSrcYLineOffset * pat->seq.width +  (emu->info_obj.uiSrcCLineOffset *  pat->seq.width)/2;
			else
				test_size = emu->info_obj.uiSrcYLineOffset * pat->seq.height +  (emu->info_obj.uiSrcCLineOffset *  pat->seq.height)/2;
			UINT32 test_chn = rand() % WP_CHN_NUM;
			h26x_test_wp(test_chn,test_addr,test_size);
		}
 		tmp = 0;
		if(!tmp){
			UINT32 test_addr = emu->info_obj.uiSrcOutYAddr;
			UINT32 test_size;
			test_size = emu->info_obj.uiSrcOutYLineOffset *  SIZE_16X(pat->seq.height) +  (emu->info_obj.uiSrcCLineOffset *   SIZE_16X(pat->seq.height))/2;
			UINT32 test_chn = rand() % WP_CHN_NUM;
			h26x_test_wp(test_chn,test_addr,test_size);
		}
		#endif
	}

	if (pat->pic_num == 0)
	{
		// dump pattern message //
		DBG_INFO("\r\njob(%d) pattern_name[%d][%d] : %s\r\n", p_job->idx1, ctx->folder.idx, ctx->pat.idx, ctx->pat.name);
		DBG_INFO("seq(%d) : (w:%d, h:%d, loft:%d, rotate:%d, src_iv:%d, rand_seed:%d, mem : (0x%08x ~ 0x%08x : 0x%08x))\r\n",
					(int)pat->seq_obj_size, (int)pat->seq.width, (int)pat->seq.height, (int)emu->init_obj.uiRecLineOffset, (int)pat->rotate, (int)pat->src_cbcr_iv,
					(int)pat->rand_seed, (int)p_job->mem.start, (int)p_job->mem.addr, (int)(p_job->mem.addr - p_job->mem.start));
	}

	h26x_flushCacheRead(ctx->emu.info_obj.uiNalLenOutAddr, sizeof(unsigned int)*H264ENC_NAL_MAXSIZE);
    h26xFileDummyRead();
	return 0;
}
#if 0

static unsigned int gen_tnr_y_out_chksum(unsigned int addr, unsigned int width, unsigned int height, unsigned int lineoffset, unsigned int hw_pad_en)
{
	unsigned int w, h;
	unsigned int chksum = 0;

#if 0 //98313 block base
	unsigned int i;

	for (h = 0; h < height; h+=16)
	{
		for (w = 0; w < width; w+=16)
		{
			unsigned char *buf = (unsigned char *)(addr + h*lineoffset + (w/16)*256);

			for (i = 0; i < 256; i++)
				chksum += (buf[i] + i);
		}
	}
#else

	//if(!hw_pad_en)
		height = SIZE_16X(height);

	for (h = 0; h < height; h++)
	{
		for (w = 0; w < width; w++)
		{


			chksum += *(unsigned char *)(addr + h*lineoffset + w);


		}
	}
#endif

	return chksum;
}

static unsigned int gen_tnr_c_out_chksum(unsigned int addr, unsigned int width, unsigned int height, unsigned int lineoffset, unsigned char mode, unsigned int hw_pad_en)
{
	unsigned int w, h;
	unsigned int chksum = 0;
#if 0 //98313 block base
	unsigned int i;

	for (h = 0; h < height; h+=8)
	{
		for (w = 0; w < width; w+=8)
		{
			unsigned char *buf = (unsigned char *)(addr + h*lineoffset + (w/8)*128);

			for (i = 0; i < 64; i++)
			{
				if (mode == 1)
				{
					chksum += (buf[i*2] + i);
					chksum += bit_reverse(buf[i*2 + 1] + i);
				}
				else
				{
					chksum += bit_reverse(buf[i*2] + i);
					chksum += (buf[i*2 + 1] + i);
				}
			}
		}
	}
#else

	//if(!hw_pad_en)
		height = SIZE_8X(height);

	for (h = 0; h < height; h++)
	{
		for (w = 0; w < width*2; w++)
		{
			chksum += *(unsigned char *)(addr + h*lineoffset + w);
		}
	}
#endif
	return chksum;
}
#endif

void dump_tnr_out(h26x_job_t *p_job)
{
	H26XFile fp_tnr;

	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *pat = &ctx->pat;
	h264_emu_t *emu = &ctx->emu;
    static int cc = 0;
	unsigned int size = emu->info_obj.uiSrcOutYLineOffset * SIZE_16X(pat->seq.height);
	char file_name[256];
	sprintf(file_name, "A:\\tnr_%d.pak", cc++);


	DBG_INFO("tnr_out : %d, %d, %d (%s)\r\n", (int)size, (int)emu->info_obj.uiSrcOutYLineOffset, (int)SIZE_16X(pat->seq.height),file_name);

	h26xFileOpen(&fp_tnr, file_name);

	h26xFileWrite(&fp_tnr, size, emu->info_obj.uiSrcOutYAddr);
	h26xFileWrite(&fp_tnr, size/2, emu->info_obj.uiSrcOutCAddr);

	h26xFileClose(&fp_tnr);
}
void dump_src(h26x_job_t *p_job)
{
	H26XFile fp_src;

	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *pat = &ctx->pat;
	h264_emu_t *emu = &ctx->emu;
    static int cc = 0;
    //unsigned int src_width = (pat->rotate == 1 || pat->rotate == 2) ? pat->seq.height : pat->seq.width;
    unsigned int src_height = (pat->rotate == 1 || pat->rotate == 2) ? pat->seq.width : pat->seq.height;
	unsigned int size = emu->info_obj.uiSrcYLineOffset * src_height;
	char file_name[256];
	sprintf(file_name, "A:\\src_%d.pak", cc++);



	DBG_INFO("src : %d, %d, %d (%s)\r\n", (int)size, (int)emu->info_obj.uiSrcYLineOffset, (int)src_height,file_name);

	h26xFileOpen(&fp_src, file_name);

	h26xFileWrite(&fp_src, size, emu->info_obj.uiSrcYAddr);
	h26xFileWrite(&fp_src, size/2, emu->info_obj.uiSrcCAddr);

	h26xFileClose(&fp_src);
}
void dump_clmv(h26x_job_t *p_job)
{
	H26XFile fp;
	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *pat = &ctx->pat;
	h264_emu_t *emu = &ctx->emu;
    static int cc = 0;
	char file_name[256];
	UINT32 uiSizeFlushColMVs   = ((pat->seq.width+63)/64)*((pat->seq.height+15)/16)*64;
	H26XENC_VAR *pVar = &emu->var_obj;
	H264ENC_CTX   *pVdoCtx	= (H264ENC_CTX *)pVar->pVdoCtx;
	H26XEncAddr   *pAddr	= &pVdoCtx->stAddr;
	UINT8 *pucHwAddr = (UINT8 *)pAddr->uiColMvs[pAddr->eRecIdx];

	sprintf(file_name, "A:\\colmv_%d.pak", cc++);
	DBG_INFO("colmv : 0x%x, 0x%08x (%s)\r\n", (int)uiSizeFlushColMVs, (int)pAddr->uiColMvs[pAddr->eRecIdx], file_name);

	h26xFileOpen(&fp, file_name);

	h26xFileWrite(&fp, uiSizeFlushColMVs, (UINT32)pucHwAddr);

	h26xFileClose(&fp);
}

void dump_bs(h26x_job_t *p_job, UINT32 bs_len)
{
	H26XFile fp_bs;

	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *pat = &ctx->pat;
	//h264_emu_t *emu = &ctx->emu;
	//H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
    static int cc = 0;
	unsigned int size = bs_len;
	char file_name[256];
	sprintf(file_name, "A:\\bs_%d.dat", cc++);
	DBG_INFO("%s\r\n", (char*)file_name);

	h26xFileOpen(&fp_bs, file_name);

	h26xFileWrite(&fp_bs, size, pat->bs_buf_addr);

	h26xFileClose(&fp_bs);
}
void dump_rec(h26x_job_t *p_job)
{
#if 1
	H26XFile fp;

	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *pat = &ctx->pat;
	//h264_emu_t *emu = &ctx->emu;
	H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;

	unsigned int size = (pRegSet->REC_LINE_OFFSET&0xffff) * 4 * SIZE_16X(pat->seq.height);
	char file_name[256];
	sprintf(file_name, "A:\\rec%d.pak", pat->pic_num);

	h26xFileOpen(&fp, file_name);

	h26xFileWrite(&fp, size, h26x_getVirAddr(pRegSet->REC_Y_ADDR));
	size = ((pRegSet->REC_LINE_OFFSET>>16)&0xffff) * 4 * SIZE_16X(pat->seq.height)/2;
	h26xFileWrite(&fp, size, h26x_getVirAddr(pRegSet->REC_C_ADDR));
	h26xFileClose(&fp);
#endif
}

static void chk_nal_len(h26x_job_t *p_job, UINT32 nal_len_dump_addr)
{
	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *pat = &ctx->pat;

	unsigned int *hw_nal_len = (unsigned int *)nal_len_dump_addr;
	unsigned int i;

    if(pat->slice_number > 176){
        DBG_INFO("Job(%d) slice = %d, don't do chk_nal_len\r\n", (int)p_job->idx1, (int)pat->slice_number);
        return;
    }
	for (i = 0; i < pat->slice_number; i++)
	{
		if (pat->slice_len[i] != hw_nal_len[i])
			DBG_INFO("Job(%d) nal[%d] diff: sw(%08x), hw(%08x), diff(%d)\r\n", (int)p_job->idx1, (int)i, (int)pat->slice_len[i], (int)hw_nal_len[i], (int)(pat->slice_len[i] - hw_nal_len[i]));
	}
}
static BOOL chk_col_mv(h26x_job_t *p_job){
	BOOL bRet = TRUE;
	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_emu_t *emu = &ctx->emu;
	h264_pat_t *pat = &ctx->pat;
	//H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
	UINT32 i,j;
	UINT32 recErrCnt = 0;
	UINT32 uiSWSizeColMVs   = ((pat->seq.width+15)/16)*(( pat->seq.height+15)/16)*16;


    //DBG_INFO("uiSWSizeColMVs =%d,addr = 0x%08x %s\r\n", (int)uiSWSizeColMVs,(int)p_job->tmp_colmv_addr,(char*)pat->file.colmv.FileName);
	h26xFileRead(&pat->file.colmv, uiSWSizeColMVs, p_job->tmp_colmv_addr);


	H26XENC_VAR *pVar = &emu->var_obj;
	H264ENC_CTX   *pVdoCtx	= (H264ENC_CTX *)pVar->pVdoCtx;
	H26XEncAddr   *pAddr	= &pVdoCtx->stAddr;

	UINT8 *pucSwAddr = (UINT8 *)p_job->tmp_colmv_addr;
	UINT8 *pucHwAddr = (UINT8 *)pAddr->uiColMvs[pAddr->eRecIdx];
    //h26x_flushCache((UINT32)pucHwAddr, uiSWSizeColMVs);


	for (j = 0; j < pat->seq.height/16; j++) {
		for (i = 0; i < pat->seq.width/16 * 16; i++) { //16 bytes every MB
			UINT32 idx = i + j*SIZE_16X(pat->seq.width);
			UINT32 idx2 = i + j*SIZE_64X(pat->seq.width);
			if(pucSwAddr[idx]!= pucHwAddr[idx2] ){
               DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] compare colmv error!\r\n", p_job->idx1, ctx->folder.idx, pat->idx, pat->pic_num);
               DBG_INFO("Error at i = %d , j = %d, swaddr = %d\r\n", (int)i,(int)j,(int)idx);
               DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",pucSwAddr[idx],pucHwAddr[idx2]);
               bRet = FALSE;
               recErrCnt++;
               if(recErrCnt > 5){
               		//UINT32 uiSizeColMVs   = ((pat->seq.width+63)/64)*(( pat->seq.height+15)/16)*64;
			   		//DBG_INFO("uiSizeColMVs =%d\r\n", (int)uiSizeColMVs);

					//h26x_prtMem(p_job->tmp_colmv_addr, uiSizeColMVs);
			   		//DBG_INFO("colmv_addr=%08x\r\n",h26x_getPhyAddr(pAddr->uiColMvs[pAddr->eRecIdx])));
					//h26x_prtMem(pAddr->uiColMvs[pAddr->eRecIdx], uiSizeColMVs);
			   		return bRet;
               }
			}
		}

	}


	return bRet;



}

static BOOL emu_h264_compare_nrout(h26x_job_t *p_job, UINT32 cmodel_checksum_y, UINT32 cmodel_checksum_c)
{
	BOOL bRet = TRUE;
	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *pat = &ctx->pat;
	seq_t *seq = &pat->seq;
	H264ENC_INFO *info_obj = &ctx->emu.info_obj;
    UINT32 i,j;
    UINT32 chksum_y = 0, chksum_c = 0;
    UINT32 width,height,lineoffset,size2;
    UINT8 *hw_y_addr,*hw_c_addr;
#if cmp_sourceout_onebyone
    UINT8 *sw_y_addr,*sw_c_addr;
    UINT32 do_compare = 1,file_size,size;
#endif
    int err_cnt = 0, max_cnt = 5;
    int cbcr = info_obj->ucSrcOutMode;
    int do_crc = 1;
    width = SIZE_16X(seq->width);
    height = SIZE_16X(seq->height);
    lineoffset = info_obj->uiSrcOutYLineOffset;
    size2 = lineoffset*height;

    hw_y_addr = (UINT8 *)info_obj->uiSrcOutYAddr;
    hw_c_addr = (UINT8 *)info_obj->uiSrcOutCAddr;
#if cmp_sourceout_onebyone
    sw_y_addr = (UINT8 *)pat->source_out_y_tmp_addr;
    sw_c_addr = (UINT8 *)pat->source_out_c_tmp_addr;
    size = width*height;


    h26xFileGetSize(&pat->file.source_out, (UINT32*)&file_size);
    if(file_size != 0) do_compare = 1;

    if(do_compare)
    {
            h26xFileRead(&pat->file.source_out, SIZE_32X(size), (UINT32)sw_y_addr);
            h26xFileRead(&pat->file.source_out, SIZE_32X(size/2), (UINT32)sw_c_addr);
    }
#endif
    //h26x_flushCache((UINT32)hw_y_addr, size2);
    //h26x_flushCache((UINT32)hw_c_addr, size2/2);
    h26x_flushCacheRead((UINT32)hw_y_addr, size2);
    h26x_flushCacheRead((UINT32)hw_c_addr, size2/2);

    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
            UINT8 hw_val = 0;
			UINT32 idx2 = j*lineoffset + i;
#if cmp_sourceout_onebyone
            UINT8 sw_val = 0;
			UINT32 idx = j*width + i;
#endif
            hw_val = hw_y_addr[idx2];
            if(do_crc){
                chksum_y = h26x_crc32(chksum_y, hw_val);
            }else{
                chksum_y += hw_val;
            }
#if cmp_sourceout_onebyone
            if(do_compare)
            {
                sw_val = sw_y_addr[idx];
                if(i<5 && j<5)
                {
                    //DBG_INFO("Y[%d][%d] SW:0x%02x , HW:0x%02x, ck:0x%08x\r\n",i,j,sw_val,hw_val,chksum_y);
                }

                if(hw_val != sw_val)
                {
                    DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] compare source out Y error!\r\n", p_job->idx1, ctx->folder.idx, pat->idx, pat->pic_num);
                    DBG_INFO("Error at i = %d , j = %d\r\n", (int)i,(int)j);
                    DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",(int)sw_val,(int)hw_val);
                    err_cnt++;
                    if(err_cnt > max_cnt)
                    {
                        break;
                    }
                }
            }
#endif
        }
        if(err_cnt > max_cnt)
        {
            break;
        }
    }

    if(err_cnt != 0) bRet = FALSE;
    if(cmodel_checksum_y != chksum_y)
    {
        DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] compare source out chksum_y error!\r\n", p_job->idx1, ctx->folder.idx, pat->idx, pat->pic_num);
        DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",(unsigned int)cmodel_checksum_y,(unsigned int)chksum_y);
        bRet = FALSE;
    }
    DBG_INFO("SOURCOUT SW:0x%08x , HW:0x%08x\r\n",(unsigned int)cmodel_checksum_y,(unsigned int)chksum_y);

    err_cnt = 0;
    for(j=0;j<height/2;j++)
    {
        for(i=0;i<width/2;i++)
        {
            UINT8 hw_u = 0,hw_v = 0;
			UINT32 idx2 = j*lineoffset + (2*i);
#if cmp_sourceout_onebyone
            UINT8 sw_u = 0,sw_v = 0;
			UINT32 idx_u = j*width + (2*i);
			UINT32 idx_v = j*width + (2*i) + 1;
#endif
            hw_u = hw_c_addr[idx2];
            hw_v = hw_c_addr[idx2+1];
            if(cbcr)
            {
                H26X_SWAP(hw_u,hw_v,UINT8);
            }

            if(do_crc){
                chksum_c = h26x_crc32(chksum_c, hw_u);
                if(i<5 && j<5)
                {
                    //DBG_INFO("C[%d][%d] HW:0x%02x, ck:0x%08x\r\n",i,j,hw_u,chksum_c);
                }
                chksum_c = h26x_crc32(chksum_c, hw_v);
                if(i<5 && j<5)
                {
                    //DBG_INFO("C[%d][%d] HW:0x%02x, ck:0x%08x\r\n",i,j,hw_v,chksum_c);
                }
            }else{
                chksum_c += hw_u;
                chksum_c += hw_v;
            }
#if cmp_sourceout_onebyone
            if(do_compare)
            {
                sw_u = sw_c_addr[idx_u];
                sw_v = sw_c_addr[idx_v];
                if((hw_u != sw_u) || (hw_v != sw_v))
                {
                    DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] compare source out C error!\r\n", p_job->idx1, ctx->folder.idx, pat->idx, pat->pic_num);
                    DBG_INFO("Error at i = %d , j = %d\r\n", (int)i,(int)j);
                    DBG_INFO("U: SW:0x%02x , HW:0x%02x\r\n",hw_u,sw_u);
                    DBG_INFO("V: SW:0x%02x , HW:0x%02x\r\n",hw_v,sw_v);
                    err_cnt++;
                    if(err_cnt > max_cnt)
                    {
                        break;
                    }
                }
            }
#endif
        }
        if(err_cnt > max_cnt)
        {
            break;
        }
    }
    if(err_cnt != 0) bRet = FALSE;

    DBG_INFO("SOURCOUT SW:0x%08x , HW:0x%08x\r\n",(unsigned int)cmodel_checksum_c,(unsigned int)chksum_c);
    if(cmodel_checksum_c != chksum_c)
    {
        DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] compare source out chksumchksum_c error!\r\n", p_job->idx1, ctx->folder.idx, pat->idx, pat->pic_num);
        DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",(unsigned int)cmodel_checksum_c,(unsigned int)chksum_c);
        bRet = FALSE;
    }

    return bRet;
}
BOOL emu_h264_compare_bs(h26x_job_t *p_job,UINT32 compare_bs_size)
{
    BOOL bRet = TRUE;
    UINT32 i,j;
	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *pat = &ctx->pat;
	h264_emu_t *emu = &ctx->emu;
//	seq_t *seq = &pat->seq;
//	chk_t *chk = &pat->chk;
//	H264ENC_INFO *info_obj = &ctx->emu.info_obj;
//	H26XENC_VAR *pVar = &ctx->emu.var_obj;
//	H264ENC_CTX   *pVdoCtx	= (H264ENC_CTX *)pVar->pVdoCtx;
//	H26XEncAddr   *pAddr	= &pVdoCtx->stAddr;
//    H26XFUNC_CTX *pFuncCtx  = pVar->pFuncCtx;

    UINT8 *pucSwBsAddr,*pucHwBsAddr;
    UINT8 ErrCnt = 0,compare_pointnum = 20;

    pucSwBsAddr = (UINT8 *)p_job->picbs_addr;
    pucHwBsAddr = (UINT8 *)pat->bs_buf_addr;

    //h26x_flushCache((UINT32)pucHwBsAddr,emu->uiHwBsMaxLen); //hk
    for (i=0;i<compare_bs_size;i++){
        j = i;
        if (*(pucSwBsAddr + i) != *(pucHwBsAddr + j)){
            DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] compare bitstream error!\r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num);
            DBG_INFO("Bitstream offset: (0x%08x 0x%08x) (SW 0x%08x,HW 0x%08x)\r\n",(int)i,(int)j,(int)(pucSwBsAddr + i),(int)(pucHwBsAddr + j));
            DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",(int)(*(pucSwBsAddr + i)),(int)(*(pucHwBsAddr + j)));
            DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",(int)(*(pucSwBsAddr + i)),(int)(*(pucHwBsAddr + j)));
            DBG_INFO("uiBsOutBufSize = %d\r\n",(int)emu->info_obj.uiBsOutBufSize);
            //DBG_INFO("SW Addr:0x%08x, HW Addr:0x%08x\r\n",pH26XVirEncAddr->uiPicBsAddr,pH26XVirEncAddr->uiHwBsAddr);
            //h26xPrtMem((UINT32)pucSwBsAddr+i,0x10);
            //h26xPrtMem((UINT32)pucHwBsAddr+i,0x10);
            bRet = FALSE;
            ErrCnt++;
            if(ErrCnt > compare_pointnum)
                break;
        }
    }
    if(bRet == TRUE){
        DBG_INFO("Compare BS PASS (0x%08x)\r\n",(int)compare_bs_size);
    }
    else{
        bRet = FALSE;
        DBG_INFO("Compare BS FAIL (0x%08x)\r\n",(int)compare_bs_size);
        h26x_prtMem((UINT32)pucSwBsAddr+i,0x30);
        h26x_prtMem((UINT32)pucHwBsAddr+i,0x30);
    }

    return bRet;

}
#if 0
void ime_check(h26x_ctrl_t *p_ctrl, h26x_job_t *p_job)
{
	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
    h264_pat_t *pat = &ctx->pat;
    int ime_hw_0,ime_hw_1;
    int rec_hw_0,rec_hw_1,rec_hw_total;
    //int ime_sw_0,ime_sw_1;
    //int rec_sw_0,rec_sw_1;
    int misc_ch_0,misc_ch_1;
    int misc_bw_0,misc_bw_1;

    ime_hw_0 = h26x_getDbg2(14);
    ime_hw_1 = h26x_getDbg2(15);
    rec_hw_0 = h26x_getDbg2(54);
    rec_hw_1 = h26x_getDbg2(55);
    misc_ch_0 = h26x_getDbg1(17);
    misc_ch_1 = h26x_getDbg1(18);
    misc_bw_0 = h26x_getDbg3(17);
    misc_bw_1 = h26x_getDbg3(18);
    rec_hw_total = rec_hw_0 + rec_hw_1;

    //DBG_INFO("¡¹[%d][%d](%d,%d) ime 0x%08x 0x%08x, rec 0x%08x, misc 0x%08x 0x%08x 0x%08x 0x%08x\r\n", (int)pat->idx, (int)pat->pic_num,pp_x_val,pp_y_val,
    //    ime_hw_0, ime_hw_1,rec_hw_total,misc_ch_0,misc_ch_1, misc_bw_0,misc_bw_1);
    DBG_INFO("[%d][%d](%d,%d) ime 0x%08x 0x%08x, rec 0x%08x, misc 0x%08x 0x%08x 0x%08x 0x%08x\r\n", (int)pat->idx, (int)pat->pic_num,pp_x_val,pp_y_val,
        ime_hw_0, ime_hw_1,rec_hw_total,misc_ch_0,misc_ch_1, misc_bw_0,misc_bw_1);

/*
    DBG_INFO("ooo [%d][%d] (%d,%d) ime checksum   0x%08x, 0x%08x \r\n", (int)pat->idx, (int)pat->pic_num,pp_x_val,pp_y_val,ime_hw_0, ime_hw_1);
    DBG_INFO("ooo [%d][%d] (%d,%d) rec checksum   0x%08x = 0x%08x + 0x%08x (%d,%d)\r\n", (int)pat->idx, (int)pat->pic_num,pp_x_val,pp_y_val, rec_hw_total,rec_hw_0, rec_hw_1);
    DBG_INFO("ooo [%d][%d] (%d,%d) misc_ch checksum  0x%08x  0x%08x (%d,%d)\r\n", (int)pat->idx, (int)pat->pic_num,pp_x_val,pp_y_val, misc_ch_0,misc_ch_1);
    DBG_INFO("ooo [%d][%d] (%d,%d) misc_bw checksum  0x%08x  0x%08x (%d,%d)\r\n", (int)pat->idx, (int)pat->pic_num,pp_x_val,pp_y_val, misc_bw_0,misc_bw_1);
*/
}
#endif

static BOOL chk_one_pic(h26x_ctrl_t *p_ctrl, h26x_job_t *p_job, UINT32 interrupt, unsigned int rec_out_en, unsigned int cmp_bs_en)
{
	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *pat = &ctx->pat;
	seq_t *seq = &pat->seq;
	chk_t *chk = &pat->chk;
	H264ENC_INFO *info_obj = &ctx->emu.info_obj;
	H26XENC_VAR *pVar = &ctx->emu.var_obj;
	H264ENC_CTX   *pVdoCtx	= (H264ENC_CTX *)pVar->pVdoCtx;
	H26XEncAddr   *pAddr	= &pVdoCtx->stAddr;
    H26XFUNC_CTX *pFuncCtx  = pVar->pFuncCtx;
	int i;
	BOOL result = TRUE;
	BOOL tnr_result = TRUE;
	BOOL rrc_result = TRUE;
	BOOL psnr_result = TRUE;
	BOOL crchit_result = TRUE;
	BOOL src_result = TRUE;
	BOOL jnd_result = TRUE;
	//UINT32 uiSWSizeColMVs   = ((pat->seq.width+15)/16)*(( pat->seq.height+15)/16)*16;
	UINT32 uiSizeFlushColMVs   = ((pat->seq.width+63)/64)*((pat->seq.height+15)/16)*64;

	#if 0
	dma_flushReadCache(dma_getCacheAddr(p_job->apb_addr), h26x_getHwRegSize());
	dma_flushWriteCache(dma_getCacheAddr(p_job->apb_addr), h26x_getHwRegSize());
	dma_flushReadCache(p_job->apb_addr, h26x_getHwRegSize());
	dma_flushWriteCache(p_job->apb_addr, h26x_getHwRegSize());
	#endif

	#if DIRECT_MODE
	UINT32 report[H26X_REPORT_NUMBER];
	UINT32 report2[H26X_REPORT_2_NUMBER];
	h26x_getEncReport(report);
	h26x_getEncReport2(report2);
	#else
	//hack "+4"
	//UINT32 *report = (UINT32 *)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
    //UINT32 *report = (UINT32 *)dma_getNonCacheAddr(p_job->check1_addr);
    UINT32 *report = (UINT32 *)p_job->check1_addr;
	UINT32 *report2 = (UINT32 *)(p_job->apb_addr + H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
	//UINT32 *report = (UINT32 *)dma_getNonCacheAddr(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
	//UINT32 *report2 = (UINT32 *)dma_getNonCacheAddr(p_job->apb_addr + H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
	UINT32 *report3 = (UINT32 *)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET);
	#if 0
	UINT32 report[H26X_REPORT_NUMBER];

	if ((p_job->idx + 1) == job_num)
		h26x_getEncReport(report);
	else
		memcpy((void *)report, (void *)(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET + 4), sizeof(UINT32)*H26X_REPORT_NUMBER);
	#endif
	#if 0
	DBG_INFO("report : %p\r\n", report));
	DBG_INFO("job(%d) report\r\n", p_job->idx));
	h26x_prtMem((UINT32)report, 0x100);
	#endif
	#endif
    //DBG_ERR("apb_addr : 0x%08x 0x%08x\r\n", (int)p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET,
    //(int)dma_getNonCacheAddr(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET));
    //ime_check( p_ctrl, p_job);

#if (DIRECT_MODE == 0)
    //h26x_flushCacheRead(p_job->apb_addr + H26X_REG_REPORT_START_OFFSET - H26X_REG_BASIC_START_OFFSET, H26X_REPORT_NUMBER*4);
    //h26x_flushCacheRead(dma_getNonCacheAddr(p_job->check1_addr), SIZE_32X(H26X_REPORT_NUMBER*4));
    h26x_flushCacheRead(p_job->check1_addr, H26X_REG_REPORT_SIZE);
    h26x_flushCacheRead(p_job->apb_addr + H26X_REG_REPORT_2_START_OFFSET - H26X_REG_BASIC_START_OFFSET, H26X_REPORT_2_NUMBER*4);
	for (i = 0; i < H26X_REPORT_NUMBER; i++) {
		report3[i] = report[i];
	}
#endif

	h26x_flushCacheRead(ctx->emu.info_obj.uiNalLenOutAddr, sizeof(unsigned int)*H264ENC_NAL_MAXSIZE);
    if(pat->pic_num!=0 && rec_out_en != 0)
    {
        h26x_flushCacheRead((UINT32)pAddr->uiColMvs[pAddr->eRecIdx], SIZE_32X(uiSizeFlushColMVs));
        //DBG_INFO("uiColMvs flusize = 0x%08x 0x%08x\r\n",(int)pAddr->uiColMvs[pAddr->eRecIdx],(int)SIZE_32X(uiSizeFlushColMVs));
    }
    h26x_flushCacheRead((UINT32)pAddr->uiSideInfo[0][pAddr->eRecIdx], ((((pat->seq.width+63)/64)*4+31)/32)*32*((pat->seq.height+15)/16));
    if(pFuncCtx->stOsg.uiGcacStatAddr0 != 0){
        UINT32 uiSizeOsgGcac  = 256;
        if (rec_out_en == 0)
        {
            h26x_flushCacheRead(p_job->tmp_osg_gcac_addr0, uiSizeOsgGcac);
            h26x_flushCacheRead(p_job->tmp_osg_gcac_addr1, uiSizeOsgGcac);
        }else{
            h26x_flushCacheRead(pFuncCtx->stOsg.uiGcacStatAddr0, uiSizeOsgGcac);
            h26x_flushCacheRead(pFuncCtx->stOsg.uiGcacStatAddr1, uiSizeOsgGcac);
        }
    }
    h26x_flushCacheRead(pFuncCtx->stMAq.uiMotAddr[0], pFuncCtx->stMAq.uiMotSize);
    h26x_flushCacheRead(pFuncCtx->stMAq.uiMotAddr[1], pFuncCtx->stMAq.uiMotSize);
    h26x_flushCacheRead(pFuncCtx->stMAq.uiMotAddr[2], pFuncCtx->stMAq.uiMotSize);

    if(interrupt != 0x1)
    {
        DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] error interrupt (%08x)!\r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num, (int)interrupt);
    }

	if (pat->pic.skipfrm_en == 0)
	{
		if (rec_out_en == 1)
			result &= (report[H26X_REC_CHKSUM] == chk->result[seq->fbc_en]);

		src_result &= (report[H26X_SRC_Y_DMA_CHKSUM] == chk->result[FPGA_SRC_Y_CHKSUM]);
		src_result &= (report[H26X_SRC_C_DMA_CHKSUM] == chk->result[FPGA_SRC_C_CHKSUM]);
	}

	//DBG_INFO("bSrcOutEn%d\r\n",info_obj->bSrcOutEn));

	if (pat->aq_chk_en && pat->pic.skipfrm_en == 0) result &= (report[H26X_SRAQ_ISUM_ACT_LOG] == chk->result[FPGA_SRAQ_ISUM_ACT_LOG]);
	if (info_obj->bSrcOutEn && pat->pic.skipfrm_en == 0)
	{
		if (chk->result[FPGA_TNR_OUT_Y_CHKSUM] != 0)
		{
#if 0
			unsigned int chksum_y, chksum_c;


			chksum_y = gen_tnr_y_out_chksum(info_obj->uiSrcOutYAddr, seq->width, seq->height, info_obj->uiSrcOutYLineOffset,pat->seq.hw_pad_en);
			chksum_c = gen_tnr_c_out_chksum(info_obj->uiSrcOutCAddr, seq->width/2, seq->height/2, info_obj->uiSrcOutCLineOffset, info_obj->ucSrcOutMode,pat->seq.hw_pad_en);

			tnr_result &= (chk->result[FPGA_TNR_OUT_Y_CHKSUM] == chksum_y);
			tnr_result &= (chk->result[FPGA_TNR_OUT_C_CHKSUM] == chksum_c);

			if (tnr_result != TRUE)
			{
				DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] tnr_chk error!\r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num);
				DBG_INFO("tnr_y : sw(%08x), hw(%08x) diff(%d)\r\n",  (int)chk->result[FPGA_TNR_OUT_Y_CHKSUM], (int)chksum_y, (int)(chk->result[FPGA_TNR_OUT_Y_CHKSUM] - chksum_y));
				DBG_INFO("tnr_c : sw(%08x), hw(%08x) diff(%d)\r\n",  (int)chk->result[FPGA_TNR_OUT_C_CHKSUM], (int)chksum_c, (int)(chk->result[FPGA_TNR_OUT_C_CHKSUM] - chksum_c));
				result = FALSE;
			}
#else
            tnr_result = emu_h264_compare_nrout(p_job, chk->result[FPGA_TNR_OUT_Y_CHKSUM], chk->result[FPGA_TNR_OUT_C_CHKSUM]);
            if (tnr_result != TRUE)
            {
                //if (ctx->emu.info_obj.bSrcOutEn) dump_tnr_out(p_job);
                result = FALSE;
            }
#endif

		}

		if(rec_out_en==0){ //SRC_OUT_ONLY
			DBG_INFO("%s %d\r\n",__FUNCTION__,__LINE__);
			#if 0
			h26x_prtReg();
			h26x_getDebug();
			#endif
			BOOL srcoutonly_result = TRUE;
            UINT32 sel_18_ans = 0xAE; //((256/4)-1 )? * 2? +? (16 * 3)

            if(h26x_getDramBurstLen()==128)
            {
                sel_18_ans = 0xAC; //((128/4)-1 )??*?4?+??(16 * 3)
            }
            DBG_INFO("%s %d, tnr = %d, sel_18_ans = %d %d\r\n",__FUNCTION__,__LINE__,(int)pat->pic.tnr_en,(int)sel_18_ans,(int)h26x_getDramBurstLen());
            if (pat->pic.tnr_en)
            {
                return TRUE;
            }


			srcoutonly_result &= (0 == h26x_getDbg3(15));
			srcoutonly_result &= (0 == h26x_getDbg3(16));
			srcoutonly_result &= (0 == h26x_getDbg3(17));
			//srcoutonly_result &= (0x6f == h26x_getDbg3(18));
			srcoutonly_result &= (sel_18_ans == h26x_getDbg3(18));
			srcoutonly_result &= (0 == h26x_getDbg3(20));
			if (srcoutonly_result == FALSE){
				DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] src_out_only_chk error!\r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num);
				DBG_INFO("0x248_sel_15 : sw(%08x), hw(%08x) diff(%d)\r\n",  0, (int)h26x_getDbg3(15), (int)(0 -  h26x_getDbg3(15)));
				DBG_INFO("0x248_sel_16 : sw(%08x), hw(%08x) diff(%d)\r\n",  0, (int)h26x_getDbg3(16), (int)(0 -  h26x_getDbg3(16)));
				DBG_INFO("0x248_sel_17 : sw(%08x), hw(%08x) diff(%d)\r\n",  0, (int)h26x_getDbg3(17), (int)(0 -  h26x_getDbg3(17)));
				DBG_INFO("0x248_sel_18 : sw(%08x), hw(%08x) diff(%d)\r\n", (int)sel_18_ans, (int)h26x_getDbg3(18), (int)(0x6f -  h26x_getDbg3(18)));
				DBG_INFO("0x248_sel_20 : sw(%08x), hw(%08x) diff(%d)\r\n",  0, (int)h26x_getDbg3(20), (int)(0 -  h26x_getDbg3(20)));
				result = FALSE;
			}
			goto Rep;
		}
	}

	result &= (report[H26X_BS_LEN] == chk->result[FPGA_BS_LEN]);
	result &= (report[H26X_BS_CHKSUM] == chk->result[FPGA_BS_CHKSUM]);

	//dump_bs(p_job,chk->result[H26X_BS_LEN]);
	//if(pat->pic_num==1)dump_rec(p_job);

	if (pat->pic.skipfrm_en == 0 && pat->aq_chk_en) result &= (report[H26X_SRAQ_ISUM_ACT_LOG] == chk->result[FPGA_SRAQ_ISUM_ACT_LOG]);


	if (pat->rrc_chk_en != 0 && pat->pic.skipfrm_en == 0)
	{
		unsigned int rrc_chk_en = pat->rrc_chk_en;

		//for (i = 0; i <= (FPGA_RRC_QP_SUM - FPGA_RRC_RDOPT_COST_LSB); i++)
        for (i = 0; i <= (FPGA_RRC_FRM_COMPLEXITY_MSB - FPGA_RRC_RDOPT_COST_LSB); i++)
		{
			if (rrc_chk_en & 0x1)
			{
				rrc_result &= (report[H26X_RRC_RDOPT_COST_LSB + i] == chk->result[FPGA_RRC_RDOPT_COST_LSB + i]);
                if(report[H26X_RRC_RDOPT_COST_LSB + i] != chk->result[FPGA_RRC_RDOPT_COST_LSB + i]){
                    DBG_INFO("rrc[%d] : sw(%08x), hw(%08x) diff(%d)\r\n", (int)i, (int)chk->result[FPGA_RRC_RDOPT_COST_LSB + i], (int)report[H26X_RRC_RDOPT_COST_LSB + i], (int)(chk->result[FPGA_RRC_RDOPT_COST_LSB + i] - report[H26X_RRC_RDOPT_COST_LSB + i]));
                }

			}
			rrc_chk_en >>= 1;
		}
        if (rrc_chk_en & 0x1)
        {
            rrc_result &= (report[H26X_RRC_COEFF] == chk->result[FPGA_RRC_COEFF]);
            if(report[H26X_RRC_COEFF] != chk->result[FPGA_RRC_COEFF]){
                DBG_INFO("H26X_RRC_COEFF : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_RRC_COEFF], (int)report[H26X_RRC_COEFF], (int)(chk->result[FPGA_RRC_COEFF] - report[H26X_RRC_COEFF]));
            }
        }
        rrc_chk_en >>= 1;
        if (rrc_chk_en & 0x1)
        {
            rrc_result &= (report[H26X_RRC_QP_SUM] == chk->result[FPGA_RRC_QP_SUM]);
            if(report[H26X_RRC_QP_SUM] != chk->result[FPGA_RRC_QP_SUM]){
                DBG_INFO("H26X_RRC_QP_SUM : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_RRC_QP_SUM], (int)report[H26X_RRC_QP_SUM], (int)(chk->result[FPGA_RRC_QP_SUM] - report[H26X_RRC_QP_SUM]));
            }
        }
        rrc_chk_en >>= 1;

		if (rrc_result == FALSE)
		{
			DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] rrc_chk(%03x) error!\r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num, (int)pat->rrc_chk_en);
			//for (i = 0; i <= (FPGA_RRC_FRM_COMPLEXITY_MSB - FPGA_RRC_RDOPT_COST_LSB); i++)
			//	emuh26x_msg(("rrc[%d] : sw(%08x), hw(%08x) diff(%d)\r\n", i, chk->result[FPGA_RRC_RDOPT_COST_LSB + i], report[H26X_RRC_RDOPT_COST_LSB + i], (chk->result[FPGA_RRC_RDOPT_COST_LSB + i] - report[H26X_RRC_RDOPT_COST_LSB + i])));
		}
	}

	if (pat->pic.skipfrm_en == 0)
	{
		for (i = 0; i <= (FPGA_PSNR_ROI_V_MSB - FPGA_PSNR_FRM_Y_LSB); i++)
			psnr_result &= (chk->result[FPGA_PSNR_FRM_Y_LSB + i] == report[H26X_PSNR_FRM_Y_LSB + i]);

		for (i = 0; i <= (FPGA_PSNR_BGR_V_MSB - FPGA_PSNR_MOT_Y_LSB); i++){
            psnr_result &= (chk->result[FPGA_PSNR_MOT_Y_LSB + i] == report2[H26X_PSNR_MOTION_Y_LSB + i]);
            //DBG_INFO("psnr[%d] : sw(%08x), hw(%08x) diff(%d)\r\n", i, chk->result[FPGA_PSNR_MOT_Y_LSB + i], report2[H26X_PSNR_MOTION_Y_LSB + i], (chk->result[FPGA_PSNR_MOT_Y_LSB + i] - report2[H26X_PSNR_MOTION_Y_LSB + i]));
        }

		psnr_result &= (chk->result[FPGA_ROI_CNT] == report[H26X_ROI_CNT]);
		psnr_result &= (chk->result[FPGA_MOT_CNT] == report2[H26X_MOTION_NUM]);
		psnr_result &= (chk->result[FPGA_BGR_CNT] == report2[H26X_BGR_NUM]);
        //DBG_INFO("FPGA_ROI_CNT[%d] : sw(%08x), hw(%08x) diff(%d)\r\n", i, chk->result[FPGA_ROI_CNT], report[H26X_ROI_CNT], (chk->result[FPGA_ROI_CNT] - report[H26X_ROI_CNT]));
        //DBG_INFO("FPGA_MOT_CNT[%d] : sw(%08x), hw(%08x) diff(%d)\r\n", i, chk->result[FPGA_MOT_CNT], report2[H26X_MOTION_NUM], (chk->result[FPGA_MOT_CNT] - report2[H26X_MOTION_NUM]));
        //DBG_INFO("FPGA_BGR_CNT[%d] : sw(%08x), hw(%08x) diff(%d)\r\n", i, chk->result[FPGA_BGR_CNT], report2[H26X_BGR_NUM], (chk->result[FPGA_BGR_CNT] - report2[H26X_BGR_NUM]));

	}

	if (psnr_result == FALSE)
	{
		DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] psnr_chk error!\r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num);

		for (i = 0; i <=  (FPGA_PSNR_ROI_V_MSB - FPGA_PSNR_FRM_Y_LSB); i++){
            DBG_INFO("psnr[%d] : sw(%08x), hw(%08x) diff(%d)\r\n", i, chk->result[FPGA_PSNR_FRM_Y_LSB + i], (int)report[H26X_PSNR_FRM_Y_LSB + i], (int)(chk->result[FPGA_PSNR_FRM_Y_LSB + i] - report[H26X_PSNR_FRM_Y_LSB + i]));
        }

		for (i = 0; i <= (FPGA_PSNR_BGR_V_MSB - FPGA_PSNR_MOT_Y_LSB); i++){
            DBG_INFO("psnr2[%d] : sw(%08x), hw(%08x) diff(%d)\r\n", i, chk->result[FPGA_PSNR_MOT_Y_LSB + i], (int)report2[H26X_PSNR_MOTION_Y_LSB + i], (int)(chk->result[FPGA_PSNR_MOT_Y_LSB + i] - report2[H26X_PSNR_MOTION_Y_LSB + i]));
        }
        DBG_INFO("FPGA_ROI_CNT : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_ROI_CNT], (int)report[H26X_ROI_CNT], (int)(chk->result[FPGA_ROI_CNT] - report[H26X_ROI_CNT]));
        DBG_INFO("FPGA_MOT_CNT : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_MOT_CNT], (int)report2[H26X_MOTION_NUM], (int)(chk->result[FPGA_MOT_CNT] - report2[H26X_MOTION_NUM]));
        DBG_INFO("FPGA_BGR_CNT : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_BGR_CNT], (int)report2[H26X_BGR_NUM], (int)(chk->result[FPGA_BGR_CNT] - report2[H26X_BGR_NUM]));
	}

	if (IS_PSLICE(ctx->emu.info_obj.ePicType) && (rec_out_en == 1) && cmp_smart_rec)
	{
		crchit_result &= (chk->result[FPGA_CRC_HIT_Y_CNT] == report[H26X_CRC_HIT_Y_CNT]);
		crchit_result &= (chk->result[FPGA_CRC_HIT_C_CNT] == report[H26X_CRC_HIT_C_CNT]);
	}

	if (crchit_result == FALSE)
	{
		DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] crchit_chk error!\r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num);
		DBG_INFO("crc_hit_y : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_CRC_HIT_Y_CNT], (int)report[H26X_CRC_HIT_Y_CNT], (int)(chk->result[FPGA_CRC_HIT_Y_CNT] - report[H26X_CRC_HIT_Y_CNT]));
		DBG_INFO("crc_hit_c : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_CRC_HIT_C_CNT], (int)report[H26X_CRC_HIT_C_CNT], (int)(chk->result[FPGA_CRC_HIT_Y_CNT] - report[H26X_CRC_HIT_C_CNT]));
	}


	chk_nal_len(p_job, ctx->emu.info_obj.uiNalLenOutAddr);

	UINT32 scd_report = report[H26X_SCD_REPORT];
    UINT32 motion_count;
	if (chk->result[FPGA_SCD_REPORT] != (scd_report& 0x1) )
	{
		DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] scd error(%d, %08x, %08x)!\r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num, (int)chk->result[FPGA_SCD_REPORT], (int)(scd_report& 0x1), (int)interrupt);
	}
//  INT32 mot_bit_diff; //from Slash : avc last mb bug
//  mot_bit_diff = chk->result[FPGA_MOTION_CNT] - (scd_report >> 16);
    motion_count = (scd_report >> 16) | (((scd_report >>15) & 0x1)<<16);
    //DBG_INFO("motion count (sw: 0x%08x, hw : 0x%08x (0x%08x), interrupt: 0x%08x)! maq_en =  %d\r\n", (int)chk->result[FPGA_MOTION_CNT], (int)motion_count ,(int)scd_report, (int)interrupt, (int)pat->pic.maq_en);

	if (pat->pic.skipfrm_en == 0 && IS_PSLICE(ctx->emu.info_obj.ePicType) && pat->pic.maq_en && chk->result[FPGA_MOTION_CNT] != motion_count)
	{
		DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] motion count error(%08x, %08x, %08x)!\r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num, (int)chk->result[FPGA_MOTION_CNT], (int)motion_count , (int)interrupt);
		result = FALSE;
	}

	#if 0
	if (pat->pic.tmnr_en)
	{
		BOOL tmnr_result = TRUE;

		#if 0
		unsigned int mto_chksum = h26x_getDbg2(88) + h26x_getDbg2(89);
		unsigned int hw_his_chksum = h26x_getDbg2(84) + h26x_getDbg2(85);

		tmnr_result &= (chk->result[FPGA_TMNR_MT_OUT] == mto_chksum);
		tmnr_result &= (chk->result[FPGA_TMNR_HIS] == hw_his_chksum);

		if (tmnr_result != TRUE)
		{
			DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] tmnr_hit error!\r\n", p_job->idx, ctx->folder.idx, pat->idx, pat->pic_num));
			DBG_INFO("tmnr_mto : sw(%08x), hw(%08x) diff(%d)\r\n", chk->result[FPGA_TMNR_MT_OUT], mto_chksum, (chk->result[FPGA_TMNR_MT_OUT] - mto_chksum)));
			DBG_INFO("tmnr_his : sw(%08x), hw(%08x) diff(%d)\r\n", chk->result[FPGA_TMNR_HIS], hw_his_chksum, (chk->result[FPGA_TMNR_HIS] - hw_his_chksum)));
		}
		#else
		tmnr_result &= (chk->result[FPGA_TMNR_MT_OUT] == report[H26X_TMNR_MTOUT_CHKSUM]);
		tmnr_result &= (chk->result[FPGA_TMNR_HIS] == report[H26X_TMNR_HIS_CHKSUM]);
		tmnr_result &= (chk->result[FPGA_TMNR_Y_CHKSUM] == report[H26X_TMNR_REC_Y_CHKSUM]);
		tmnr_result &= (chk->result[FPGA_TMNR_C_CHKSUM] == report[H26X_TMNR_REC_C_CHKSUM]);
		unsigned int rec_y_chksum = h26x_getDbg2(78) + h26x_getDbg2(79);
		unsigned int rec_c_chksum = h26x_getDbg2(80) + h26x_getDbg2(81);
		if (tmnr_result != TRUE)
		{
			DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] tmnr error!\r\n", p_job->idx, ctx->folder.idx, pat->idx, pat->pic_num));
			DBG_INFO("tmnr_y   : sw(%08x), hw(%08x) diff(%d)\r\n", chk->result[FPGA_TMNR_Y_CHKSUM], report[H26X_TMNR_REC_Y_CHKSUM], (chk->result[FPGA_TMNR_Y_CHKSUM] - report[H26X_TMNR_REC_Y_CHKSUM])));
			DBG_INFO("tmnr_c   : sw(%08x), hw(%08x) diff(%d)\r\n", chk->result[FPGA_TMNR_C_CHKSUM], report[H26X_TMNR_REC_C_CHKSUM], (chk->result[FPGA_TMNR_C_CHKSUM] - report[H26X_TMNR_REC_C_CHKSUM])));
			DBG_INFO("tmnr_y_2 : sw(%08x), hw(%08x) diff(%d)\r\n", chk->result[FPGA_TMNR_Y_CHKSUM], rec_y_chksum, (chk->result[FPGA_TMNR_Y_CHKSUM] - rec_y_chksum)));
			DBG_INFO("tmnr_c_2 : sw(%08x), hw(%08x) diff(%d)\r\n", chk->result[FPGA_TMNR_C_CHKSUM], rec_c_chksum, (chk->result[FPGA_TMNR_C_CHKSUM] - rec_c_chksum)));
			DBG_INFO("tmnr_mto : sw(%08x), hw(%08x) diff(%d)\r\n", chk->result[FPGA_TMNR_MT_OUT], report[H26X_TMNR_MTOUT_CHKSUM], (chk->result[FPGA_TMNR_MT_OUT] - report[H26X_TMNR_MTOUT_CHKSUM])));
			DBG_INFO("tmnr_his : sw(%08x), hw(%08x) diff(%d)\r\n", chk->result[FPGA_TMNR_HIS], report[H26X_TMNR_HIS_CHKSUM], (chk->result[FPGA_TMNR_HIS] - report[H26X_TMNR_HIS_CHKSUM])));
		}
		#endif
	}
	#endif

	if (cmp_bs_en)
	{
        BOOL bs_ret = TRUE;
        h26x_flushCacheRead(pat->bs_buf_addr, SIZE_32X(chk->result[FPGA_BS_LEN]));
		//h26xFileRead(&pat->file.es, chk->result[FPGA_BS_LEN], p_job->tmp_bs_addr);
		//if (memcmp((void *)p_job->tmp_bs_addr, (void *)pat->bs_buf_addr, chk->result[FPGA_BS_LEN]) != 0)
#if 0
        if (memcmp((void *)p_job->picbs_addr, (void *)pat->bs_buf_addr, chk->result[FPGA_BS_LEN]) != 0){
            DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] compare bs error!\r\n", p_job->idx1, ctx->folder.idx, pat->idx, pat->pic_num);
        }else{
            DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] compare bs pass!\r\n", p_job->idx1, ctx->folder.idx, pat->idx, pat->pic_num);
        }
#else
        if(pat->rnd_slc_hdr == 0)
        {
            bs_ret = emu_h264_compare_bs(p_job,chk->result[FPGA_BS_LEN]);
        }
        if(bs_ret != TRUE)
        {
            //dump_bs(p_job,chk->result[H26X_BS_LEN]);
        }
        //dump_bs(p_job,chk->result[H26X_BS_LEN]);
#endif

	}
	if(cmp_colmv==1 && rec_out_en!= 0){
		result &= chk_col_mv(p_job);
	}

	if (pat->pic.skipfrm_en == 0){
        for (i = 0; i <= (FPGA_JND_GRAD_CNT - FPGA_JND_GRAD); i++)
		{
            jnd_result &= (report2[H26X_JND_GRAD + i] == chk->result[FPGA_JND_GRAD + i]);
            //DBG_INFO("jnd[%d] : sw(%08x), hw(%08x) diff(%d)\r\n", i, chk->result[FPGA_JND_GRAD + i], report2[H26X_JND_GRAD + i], (chk->result[FPGA_JND_GRAD + i] - report2[H26X_JND_GRAD + i]));
		}

		if (jnd_result == FALSE)
		{
			DBG_INFO("Job(%d)(%d) H264 Folder[%d]Pat[%d]Pic[%d] jnd_chk error!\r\n", p_job->idx1, p_job->idx2, ctx->folder.idx, pat->idx, pat->pic_num);

			for (i = 0; i <= (FPGA_JND_GRAD_CNT - FPGA_JND_GRAD); i++)
				DBG_INFO("jnd[%d] : sw(%08x), hw(%08x) diff(%d)\r\n", (int)i, (int)chk->result[FPGA_JND_GRAD + i], (int)report2[H26X_JND_GRAD + i], (int)(chk->result[FPGA_JND_GRAD + i] - report2[H26X_JND_GRAD + i]));
		}
	}

#if 1
	if (pat->pic.skipfrm_en == 0){
	src_result &= (report[H26X_JND_Y_CHKSUM] == chk->result[FPGA_JND_Y_CHKSUM]);
	src_result &= (report[H26X_JND_C_CHKSUM] == chk->result[FPGA_JND_C_CHKSUM]);
	src_result &= (report[H26X_OSG_0_Y_CHKSUM] == chk->result[FPGA_OSG_0_Y_CHKSUM]);
	src_result &= (report[H26X_OSG_0_C_CHKSUM] == chk->result[FPGA_OSG_0_C_CHKSUM]);
	src_result &= (report[H26X_OSG_Y_CHKSUM] == chk->result[FPGA_OSG_Y_CHKSUM]);
	src_result &= (report[H26X_OSG_C_CHKSUM] == chk->result[FPGA_OSG_C_CHKSUM]);
	}
#endif


Rep:
	//if(pat->pic_num==4){result=FALSE;}


	#if 0
	DBG_INFO("dbg_2 dump:\r\n"));
	for (i = 74; i <= 89; i++)
		DBG_INFO("[%d] %08x\r\n", i, h26x_getDbg2(i)));
	#endif
	//DBG_INFO("report : %08x, %08x\r\n", report[H26X_EC_REAL_LEN_SUM], report[H26X_EC_SKIP_LEN_SUM]));

	//if (ctx->emu.info_obj.bSrcOutEn) dump_tnr_out(p_job);
    //dump_src(p_job);

	if (result == FALSE)
	//if (pat->pic_num == 1)
	{
		char cmd[0x1000];
		unsigned int cmd_addr = 0;
		memset(cmd, 0, 0x1000);

		DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] encode error (int:%08x)!\r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num, (int)interrupt);

		DBG_INFO("rec    : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[seq->fbc_en], (int)report[H26X_REC_CHKSUM], (int)(chk->result[seq->fbc_en] - report[H26X_REC_CHKSUM]));

		DBG_INFO("src_y  : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_SRC_Y_CHKSUM], (int)report[H26X_SRC_Y_DMA_CHKSUM], (int)(chk->result[FPGA_SRC_Y_CHKSUM] - report[H26X_SRC_Y_DMA_CHKSUM]));
		DBG_INFO("src_c  : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_SRC_C_CHKSUM], (int)report[H26X_SRC_C_DMA_CHKSUM], (int)(chk->result[FPGA_SRC_C_CHKSUM] - report[H26X_SRC_C_DMA_CHKSUM]));
		DBG_INFO("jnd_y : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_JND_Y_CHKSUM], (int)report[H26X_JND_Y_CHKSUM], (int)(chk->result[FPGA_JND_Y_CHKSUM] - report[H26X_JND_Y_CHKSUM]));
		DBG_INFO("jnd_c : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_JND_C_CHKSUM], (int)report[H26X_JND_C_CHKSUM], (int)(chk->result[FPGA_JND_C_CHKSUM] - report[H26X_JND_C_CHKSUM]));
		DBG_INFO("osg_y  : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_OSG_0_Y_CHKSUM], (int)report[H26X_OSG_0_Y_CHKSUM], (int)(chk->result[FPGA_OSG_0_Y_CHKSUM] - report[H26X_OSG_0_Y_CHKSUM]));
		DBG_INFO("osg_c  : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_OSG_0_C_CHKSUM], (int)report[H26X_OSG_0_C_CHKSUM], (int)(chk->result[FPGA_OSG_0_C_CHKSUM] - report[H26X_OSG_0_C_CHKSUM]));
		DBG_INFO("osg_1_y : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_OSG_Y_CHKSUM], (int)report[H26X_OSG_Y_CHKSUM], (int)(chk->result[FPGA_OSG_Y_CHKSUM] - report[H26X_OSG_Y_CHKSUM]));
		DBG_INFO("osg_1_c : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_OSG_C_CHKSUM], (int)report[H26X_OSG_C_CHKSUM], (int)(chk->result[FPGA_OSG_C_CHKSUM] - report[H26X_OSG_C_CHKSUM]));
		DBG_INFO("bslen  : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_BS_LEN], (int)report[H26X_BS_LEN], (int)(chk->result[FPGA_BS_LEN] - report[H26X_BS_LEN]));
		DBG_INFO("bs     : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_BS_CHKSUM], (int)report[H26X_BS_CHKSUM], (int)(chk->result[FPGA_BS_CHKSUM] - report[H26X_BS_CHKSUM]));
		DBG_INFO("aslog  : sw(%08x), hw(%08x) diff(%d) aq_chk_en = %d\r\n", (int)chk->result[FPGA_SRAQ_ISUM_ACT_LOG], (int)report[H26X_SRAQ_ISUM_ACT_LOG], (int)(chk->result[FPGA_SRAQ_ISUM_ACT_LOG] - report[H26X_SRAQ_ISUM_ACT_LOG]),(int)pat->aq_chk_en);

		#if 0
		H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
		DBG_INFO("dump gcac(0x484) buffer\r\n"));
		h26x_prtMem(pRegSet->OSG_CFG[0x484/4], 256);
		DBG_INFO("dump gcac(0x480) buffer \r\n"));
		h26x_prtMem(pRegSet->OSG_CFG[0x480/4], 256);
		//DBG_INFO("dump win16 graph buffer \r\n"));
		//h26x_prtMem(pRegSet->OSG_CFG[0x244/4], 51072);
		#endif
        //dump_bs(p_job,chk->result[H26X_BS_LEN]);
        //dump_rec(p_job);

		h26x_prtReg();
		h26x_getDebug();
		if (DIRECT_MODE == 0)
		{
			DBG_INFO("job(%d) apb data\r\n", p_job->idx1);
			h26x_prtMem(p_job->apb_addr, h26x_getHwRegSize());
			DBG_INFO("job(%d) linklist data\r\n", p_job->idx1);
			h26x_prtMem(p_job->llc_addr, h26x_getHwRegSize());
		}

		DBG_INFO("\r\ncmd : \r\n");
		h26xFileRead(&pat->file.info, pat->info.cmd_len, (UINT32)cmd);
		while(cmd_addr < pat->info.cmd_len)
		{
			DBG_INFO("%s", cmd + cmd_addr);
			cmd_addr += 511;
		}
		DBG_INFO("\r\n");

		pat->pic_num = pat->info.frame_num;	// stop this pattern //
		//while(1);
	}
	else
	{

		#if 0

		h26x_prtReg();
		h26x_getDebug();
		//h26x_prtMem(pat->bs_buf_addr, pat->chk.result[FPGA_BS_LEN] + 128);
		if (DIRECT_MODE == 0)
				{
					DBG_INFO("job(%d) apb data\r\n", p_job->idx2));
					h26x_prtMem(p_job->apb_addr, h26x_getHwRegSize());
					DBG_INFO("job(%d) linklist data\r\n", p_job->idx2));
					h26x_prtMem(p_job->llc_addr, h26x_getHwRegSize());
				}



		#endif

		if (rec_out_en)
		{
			#if 0
			if (pat->pic_num % 10)
				DBG_INFO("%c", '*' + p_job->idx));
			else
				DBG_INFO("[%d][%d]", p_job->idx, pat->pic_num));

			if ((pat->pic_num % 30) == 0)
				DBG_INFO("\r\n"));
			#else
			DBG_INFO("Job(%d)(%d) H264 Folder[%d]Pat[%d]Pic[%d] encode correct!\r\n", p_job->idx1, p_job->idx2, ctx->folder.idx, pat->idx, pat->pic_num);

			#if 0
            DBG_INFO("rec    : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[seq->fbc_en], (int)report[H26X_REC_CHKSUM], (int)(chk->result[seq->fbc_en] - report[H26X_REC_CHKSUM]));
            DBG_INFO("src_y  : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_SRC_Y_CHKSUM], (int)report[H26X_SRC_Y_DMA_CHKSUM], (int)(chk->result[FPGA_SRC_Y_CHKSUM] - report[H26X_SRC_Y_DMA_CHKSUM]));
            DBG_INFO("src_c  : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_SRC_C_CHKSUM], (int)report[H26X_SRC_C_DMA_CHKSUM], (int)(chk->result[FPGA_SRC_C_CHKSUM] - report[H26X_SRC_C_DMA_CHKSUM]));
            DBG_INFO("jnd_y : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_JND_Y_CHKSUM], (int)report[H26X_JND_Y_CHKSUM], (int)(chk->result[FPGA_JND_Y_CHKSUM] - report[H26X_JND_Y_CHKSUM]));
            DBG_INFO("jnd_c : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_JND_C_CHKSUM], (int)report[H26X_JND_C_CHKSUM], (int)(chk->result[FPGA_JND_C_CHKSUM] - report[H26X_JND_C_CHKSUM]));
            DBG_INFO("osg_y  : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_OSG_Y_CHKSUM], (int)report[H26X_OSG_0_Y_CHKSUM], (int)(chk->result[FPGA_OSG_0_Y_CHKSUM] - report[H26X_OSG_0_Y_CHKSUM]));
            DBG_INFO("osg_c  : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_OSG_C_CHKSUM], (int)report[H26X_OSG_0_C_CHKSUM], (int)(chk->result[FPGA_OSG_0_C_CHKSUM] - report[H26X_OSG_0_C_CHKSUM]));
            DBG_INFO("osg_1_y : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_OSG_Y_CHKSUM], (int)report[H26X_OSG_Y_CHKSUM], (int)(chk->result[FPGA_OSG_Y_CHKSUM] - report[H26X_OSG_Y_CHKSUM]));
            DBG_INFO("osg_1_c : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_OSG_C_CHKSUM], (int)report[H26X_OSG_C_CHKSUM], (int)(chk->result[FPGA_OSG_C_CHKSUM] - report[H26X_OSG_C_CHKSUM]));
            DBG_INFO("bslen  : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_BS_LEN], (int)report[H26X_BS_LEN], (int)(chk->result[FPGA_BS_LEN] - report[H26X_BS_LEN]));
            DBG_INFO("bs     : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_BS_CHKSUM], (int)report[H26X_BS_CHKSUM], (int)(chk->result[FPGA_BS_CHKSUM] - report[H26X_BS_CHKSUM]));
            DBG_INFO("aslog  : sw(%08x), hw(%08x) diff(%d)\r\n", (int)chk->result[FPGA_SRAQ_ISUM_ACT_LOG], (int)report[H26X_SRAQ_ISUM_ACT_LOG], (int)(chk->result[FPGA_SRAQ_ISUM_ACT_LOG] - report[H26X_SRAQ_ISUM_ACT_LOG]));
			#endif

			#if 0
			if (DIRECT_MODE == 0)
			{
				h26x_prtReg();
				DBG_INFO("job(%d) apb data\r\n", p_job->idx));
				h26x_prtMem(p_job->apb_addr, h26x_getHwRegSize());
				DBG_INFO("job(%d) linklist data\r\n", p_job->idx));
				h26x_prtMem(p_job->llc_addr, h26x_getHwRegSize());
			}
			else
			{
				h26x_prtReg();
			}
			#endif
			#endif

			pat->pic_num++;
		}else{
			DBG_INFO("Job(%d)(%d) H264 Folder[%d]Pat[%d]Pic[%d] source out only encode correct!\r\n", (unsigned int)p_job->idx1, (unsigned int)p_job->idx2, (unsigned int)ctx->folder.idx, (unsigned int)pat->idx, (unsigned int)pat->pic_num);
            //h26x_prtReg();
            //h26x_getDebug();
        }
	}

	if (rec_out_en == 0)
	{
		H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;

		// for rec_out_dis + bs_out_re-trigger //
		ctx->emu.info_obj.uiBsOutBufAddr = pat->bs_buf_addr;
		ctx->emu.info_obj.uiBsOutBufSize = pRegSet->BSOUT_BUF_SIZE;

		if(pat->bs_buf_32b)
		pat->res_bs_size = pat->chk.result[FPGA_BS_LEN] + 32;
		else
		pat->res_bs_size = pat->chk.result[FPGA_BS_LEN] + 128;

		memset((void *)pat->bs_buf_addr, 0, pat->bs_buf_size);
		memset((void *)ctx->emu.info_obj.uiNalLenOutAddr, 0, sizeof(unsigned int)*256);

		h26x_flushCache(pat->bs_buf_addr, SIZE_32X(pat->bs_buf_size));
		h26x_flushCache(ctx->emu.info_obj.uiNalLenOutAddr, sizeof(unsigned int)*256);
	}
	else
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

    return result;
}

void emu_h264_chk_one_pic(h26x_ctrl_t *p_ctrl, h26x_job_t *p_job, UINT32 interrupt, unsigned int rec_out_en, unsigned int cmp_bs_en)
{
	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	//BOOL result = TRUE;
	//h264_pat_t *pat = &ctx->pat;

	/*result = */chk_one_pic( p_ctrl, p_job, interrupt, rec_out_en, cmp_bs_en);//HACK


#if 0
    if(result == TRUE && (pat->pic_num== 17))
    {
        int i;
        int loop_cnt = 300;
        for(i=0;i<loop_cnt;i++)
        {
            emuh26x_msg(("i = %d\r\n",i));
            if(i!=0 && rec_out_en)
            {
                h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
                h264_pat_t *pat = &ctx->pat;
                pat->pic_num -= 1;
            }
#if 0
            h26x_setEncDirectRegSet(p_job->apb_addr);
#else
            h26x_setEncLLRegSet2();
            h26x_setCodecClock(rand() % 2);
            h26x_setCodecPClock(rand() % 2);
#endif
            h26x_start();
            interrupt = h26x_waitINT();
            result = chk_one_pic(p_job, interrupt, rec_out_en, cmp_bs_en);//HACK
            emuh26x_msg(("result = %d\r\n",result));
            if(result != TRUE)
            {
                break;
            }
        }
    }

#endif

	h264Enc_getResult(&ctx->emu.var_obj, DIRECT_MODE);
}

void emu_h264_set_nxt_bsbuf(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item,UINT32 interrupt)
{
	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;

    //DBG_INFO(("%s %d, int = 0x%08x\r\n",__FUNCTION__,__LINE__,interrupt));
	//DBG_INFO(("(%d)(%d)addr : %08x, size : %d, res : %08x\r\n", p_job->idx1, p_job->idx2, ctx->emu.info_obj.uiBsOutBufAddr, ctx->emu.info_obj.uiBsOutBufSize, ctx->pat.res_bs_size));
	if (p_ver_item->rnd_bs_buf == 0)
    {
        //DBG_INFO("(%d)addr : %08x, size : %d, res : %08x\r\n", (int)p_job->idx1, (int)ctx->emu.info_obj.uiBsOutBufAddr, (int)ctx->emu.info_obj.uiBsOutBufSize, (int)ctx->pat.res_bs_size);
        //DBG_INFO("refill bsout size\r\n");

        UINT32 addr = ctx->emu.info_obj.uiBsOutBufAddr;
        UINT32 size;

        ctx->pat.res_bs_size -= ctx->emu.info_obj.uiBsOutBufSize;
        size = ctx->emu.info_obj.uiBsOutBufSize;
        h26x_setNextBsBuf(h26x_getPhyAddr(addr), size);

    }else{
        if (ctx->pat.res_bs_size <= ctx->emu.info_obj.uiBsOutBufSize)
        {
            UINT32 addr = ctx->emu.info_obj.uiBsOutBufAddr;
            UINT32 size = ctx->emu.info_obj.uiBsOutBufSize;
            DBG_INFO("(%d)addr : %08x, size : %d, res : %08x\r\n", (int)p_job->idx1, (int)ctx->emu.info_obj.uiBsOutBufAddr, (int)ctx->emu.info_obj.uiBsOutBufSize, (int)ctx->pat.res_bs_size);
            DBG_INFO("[ERR][FAIL] encode bitstream size shall be enough but query next buffer\r\n");
            h26x_prtReg();
            h26x_getDebug();
            if (DIRECT_MODE == 0)
            {
                DBG_INFO("job(%d) apb data\r\n", (int)p_job->idx1);
                h26x_prtMem(p_job->apb_addr, h26x_getHwRegSize());
                DBG_INFO("job(%d) linklist data\r\n", (int)p_job->idx1);
                h26x_prtMem(p_job->llc_addr, h26x_getHwRegSize());
            }
            h26x_setNextBsBuf(h26x_getPhyAddr(addr), size);
            //while(1);
        }
        else
        {
            UINT32 addr = ctx->emu.info_obj.uiBsOutBufAddr + ctx->emu.info_obj.uiBsOutBufSize;
            UINT32 size;
            UINT32 burst_unit = 128;
            int tmp = rand()%3;

            //h26x_flushCacheRead(ctx->emu.info_obj.uiBsOutBufAddr, ctx->emu.info_obj.uiBsOutBufSize);

            ctx->pat.res_bs_size -= ctx->emu.info_obj.uiBsOutBufSize;
            /*
            if(ctx->pat.bs_buf_32b){
            size = 96;// ((rand()%(SIZE_32X(ctx->pat.res_bs_size)/32 + 1)) + 1)*32; //tmppp
            size = (size > ctx->pat.res_bs_size) ? SIZE_32X(ctx->pat.res_bs_size) : size;
            }
            else{
            size = ((rand()%(SIZE_128X(ctx->pat.res_bs_size)/128 + 1)) + 1)*128;
            size = (size > ctx->pat.res_bs_size) ? SIZE_128X(ctx->pat.res_bs_size) : size;
            }
            */
            if(ctx->pat.bs_buf_32b)
                burst_unit = 32;

            if(tmp == 0){
                size = burst_unit * (rand()%3+1);
            }else{
                size = ((rand()%(ctx->pat.res_bs_size/burst_unit)) + 1)*burst_unit;
            }
            size = (size > ctx->pat.res_bs_size) ? burst_unit : size;

            ctx->emu.info_obj.uiBsOutBufAddr = addr;
            ctx->emu.info_obj.uiBsOutBufSize = size;
            //DBG_INFO("%s %d size = %d, addr = 0x%08x\r\n",__FUNCTION__,__LINE__,(int)size,(int)addr);

            //if (*(UINT32 *)addr != 0)
            //    DBG_INFO("[ERR] bsout re-trigger dummy write, (last : %08x, dummy : %08x)\r\n", (int)*(UINT32 *)(addr - 4), (int)*(UINT32 *)addr);

            h26x_setNextBsBuf(h26x_getPhyAddr(addr), size);
        }

    }


	//DBG_INFO("(1)(%d)addr : %08x, size : %08x, res : %08x\r\n", p_job->idx, ctx->emu.info_obj.uiBsOutBufAddr, ctx->emu.info_obj.uiBsOutBufSize, ctx->pat.res_bs_size);
}
void emu_h264_reset_bsbuf(h26x_job_t *p_job, UINT32 bs_len, unsigned int write_prot)
{
	h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
	h264_pat_t *pat = &ctx->pat;

	if(ctx->pat.bs_buf_32b)
	ctx->pat.res_bs_size = pat->chk.result[FPGA_BS_LEN] + 32;
	else
	ctx->pat.res_bs_size = pat->chk.result[FPGA_BS_LEN] + 128;

	UINT32 addr = pat->bs_buf_addr;
	UINT32 size = bs_len;	//initial

	ctx->emu.info_obj.uiBsOutBufAddr = addr;
	ctx->emu.info_obj.uiBsOutBufSize = size;

	H26XRegSet *pRegSet = (H26XRegSet *)p_job->apb_addr;
	pRegSet->BSOUT_BUF_ADDR = h26x_getPhyAddr(addr);
	pRegSet->BSOUT_BUF_SIZE = size;

	if(write_prot)h26x_test_wp(0, addr, size, rand()%4);
	h26x_setNextBsSize(size);
//	h26x_setNextBsBuf(h26x_getPhyAddr(addr), size);
}
BOOL emu_h264_compare_stable_len(h26x_job_t *p_job)
{
    BOOL bRet = TRUE;
    UINT32 bs_len,slice_num;
    h264_ctx_t *ctx = (h264_ctx_t *)p_job->ctx_addr;
    h264_emu_t *emu = &ctx->emu;
	h264_pat_t *pat = &ctx->pat;
	//H26XEncAddr  *pH26XVirEncAddr = &emu->sH26XVirEncAddr;
    UINT8 *pucSwBsAddr,*pucHwBsAddr;
    UINT32 i,compare_pointnum = 5;
    UINT32 uiHwNalLen[80*5];
	UINT32 sw_bslen = ((pat->chk.result[FPGA_BS_LEN]+3)>>2)<<2; // hw bs write out is 4X


    pucSwBsAddr = (UINT8 *)p_job->picbs_addr;
    pucHwBsAddr = (UINT8 *)emu->info_obj.uiBsOutBufAddr;

    slice_num = h26x_getStableSliceNum();
    bs_len = h26x_getStableLen() *4;

    if(bs_len <= 0 )
    {
        return bRet;
    }
    h26x_flushCacheRead((UINT32)pucHwBsAddr, SIZE_32X(bs_len));

    if(bs_len < pat->stable_bs_len)
    {
		DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] bs_len 0x%08x > 0x%08x error! (last stable_bs_len)\r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num, (int)bs_len, (int)pat->stable_bs_len);
        bRet = FALSE;
        //for(i=0;i<(UINT32)stable_bs_length_b_idx;i++)
        //{
        //    DBG_INFO("stable_bs_length_b[%d] = 0x%08x error!\r\n", (int)i,(int)stable_bs_length_b[i]);
        //}
        //bstableRet = bRet;
        //h26x_prtReg();
    }
    if(bs_len > sw_bslen)
    {
		DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] bs_len 0x%08x > 0x%08x error! (uiH26XBsLen)\r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num, (int)bs_len, (int)sw_bslen);
        bRet = FALSE;
    }
    pat->stable_bs_len = bs_len;
    //DBG_INFO("%s %d stable_bs_le = 0x%08x\r\n",__FUNCTION__,__LINE__,pat->stable_bs_len);
    //if(stable_bs_length_b_idx < 1000)
    //{
    //    stable_bs_length_b[stable_bs_length_b_idx] = pat->stable_bs_len;
    //    stable_bs_length_b_idx++;
    //}

    if(bs_len < compare_pointnum) return bRet;
    for(i=0;i<compare_pointnum;i++)
    {
        UINT32 j;
        j = bs_len-compare_pointnum+i;

		if(j >= pat->chk.result[FPGA_BS_LEN])
		{
			//DBG_INFO("break, bs_len = 0x%x, i = %d, j = 0x%x\r\n",bs_len,i,j);
			break;
		}

       // DBG_INFO("bs_len = 0x%x, j = 0x%x\r\n",bs_len,j);
       // DBG_INFO("SW:0x%02x , HW:0x%02x\r\n",*(pucSwBsAddr + j),*(pucHwBsAddr + j));
        if (*(pucSwBsAddr + j) != *(pucHwBsAddr + j)){
            DBG_INFO("bs_len = 0x%x, i = %d, j = 0x%x\r\n",(int)bs_len,(int)i,(int)j);
			DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d]  stable bistream error! offset = 0x%x\r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num,(int)j);
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
            total_nal_size += pat->slice_len[i];
            uiHwNalLen[i] = h26x_getNalLen(i);
            if(uiHwNalLen[i] & 0x80000000)
            {
                UINT32 sw_nal,hw_nal;
                hw_nal = uiHwNalLen[i] & 0x7fffffff;
                sw_nal = pat->slice_len[i];
                //DBG_INFO("nal_len[%d] = 0x%08x, 0x%08x\r\n",i,sw_nal,hw_nal);
                if(sw_nal != hw_nal)
                {
                    DBG_INFO("stable nal len error! [%d] sw_nal = 0x%08x, hw_nal = 0x%08x\r\n",(int)i,(int)sw_nal,(int)hw_nal);
                }
            }
        }

        //DBG_INFO("sw total_nal_size = 0x%08x, hw bs_len = 0x%08x\r\n",total_nal_size,bs_len);
        if(bs_len < total_nal_size)
        {
			DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d] stable nal error!  \r\n", (int)p_job->idx1, (int)ctx->folder.idx, (int)pat->idx, (int)pat->pic_num );
			DBG_INFO("sw total_nal_size = 0x%08x, hw bs_len = 0x%08x hw StableSliceNum =  0x%08x\r\n",(int)total_nal_size,(int)bs_len,(int)slice_num);
			h26x_prtReg();
			h26x_getDebug();
			bRet = FALSE;
        }
    }

    //bstableRet = bRet;
	//DBG_INFO("Job(%d) H264 Folder[%d]Pat[%d]Pic[%d]stable len pass!\r\n", p_job->idx1, ctx->folder.idx, pat->idx, pat->pic_num);
    return bRet;
}

//#endif //if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)


