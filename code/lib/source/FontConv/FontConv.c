#include "FontConv/FontConv.h"
#include <kwrap/error_no.h>
#include <kwrap/verinfo.h>
#include "FileSysTsk.h"
#include "vf_gfx.h"
#include "hd_common.h"

#define CFG_DUMP_FONTCONV_RAW DISABLE
#define CFG_DUMP_RESULT DISABLE

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          Font
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#define __DBGFLT__          "[init]"
//#define __DBGFLT__          "[dump]"
//#define __DBGFLT__          "[fill]"
//#define __DBGFLT__          "[copy]"
//#define __DBGFLT__          "[scale]"
//#define __DBGFLT__          "[rotate]"
//#define __DBGFLT__          "[disp]"
#include <kwrap/debug.h>

VOS_MODULE_VERSION(FontConv, 1, 00, 001, 00);

#define SUP_ROTATE  0

static RESULT IndexToYuv(F2Y_DC *pDestDC,  const DC *pSrcDC, const PALETTE_ITEM *pTable);

static RESULT F2Y_AttachDC(F2Y_DC *pDC,
						   UINT32 uiPxlfmt,
						   UINT32 uiWidth, UINT32 uiHeight,
						   UINT32 pBuf);

static UINT32 GxImg2GxgfxFmt(HD_VIDEO_PXLFMT fmt);

#define IDX_FONT_TRANSPARETN    0
#define IDX_FONT_SOLID          1
#define IDX_FONT_FRAME          2

#define MAKE_ALIGN(s, bytes)    ((((UINT32)(s))+((bytes)-1)) & ~((UINT32)((bytes)-1)))
#define MAKE_ALIGN_S(s, bytes)  (((UINT32)(s)) & ~((UINT32)((bytes)-1)))

static DC m_StampDC;        //INDEX8 DC for draw string
static F2Y_DC m_TempF2YDC;  //YUV DC for index to YUV by IndexToYuv()
static DC m_TempDC;         //ARGB DC for index to ARGB by GxGfx
static DC m_Temp2DC;        //scaled ARGB DC for ARGB scale by GxGfx

ER FontConv(FONT_CONV_IN *pIn, FONT_CONV_OUT *pOut)
{
	ISIZE szStamp;
	UINT32 pMemPool;
	INT32  w_Stamp, h_Stamp;
	UINT32 memszStampDC;
	UINT32 addrStamp;
	INT32  w_TempDC, h_TempDC;
	UINT32 memszTempDC;
	UINT32 addrTemp;
	INT32  w_Temp2DC, h_Temp2DC;
	UINT32 memszTemp2DC;
	UINT32 addrTemp2;
	UINT32 uiAddrY = 0, uiAddrCB = 0;
    UINT32 uiLineOffsetY =0;
	UINT32 w_rt, h_rt, size_rt; //rotated uv420 convert to rotated uv422
	UINT32 addr_rt, addr_gx_rot;
	UINT32 r =0;
	UINT32 MemAddr = pIn->MemAddr;
	PALETTE_ITEM FontPalette[3];
	PALETTE_ITEM *Palette = 0 ;
	UINT32 gximg_fmt; //for gximg API
	if ((pIn->Format != HD_VIDEO_PXLFMT_YUV422) && (pIn->Format != HD_VIDEO_PXLFMT_YUV420) &&
        (pIn->Format != HD_VIDEO_PXLFMT_ARGB8888)&& (pIn->Format != HD_VIDEO_PXLFMT_ARGB1555) &&
        (pIn->Format != HD_VIDEO_PXLFMT_ARGB4444) && (pIn->Format != HD_VIDEO_PXLFMT_RGB565) ){
		DBG_ERR("not support 0x%x !\r\n",pIn->Format);
		return E_PAR;
	}
    // 1.protect FontConv multi-access
    // 2.protect font and color map,when UI use
    if(GxGfx_Lock()!=0){
        return E_SYS;
    }

	gximg_fmt = pIn->Format;

	memset(pOut, 0, sizeof(FONT_CONV_OUT));

	//#Need initial local variable at beginning
	memset(&m_StampDC, 0x00, sizeof(DC));
	memset(&m_TempF2YDC, 0x00, sizeof(F2Y_DC));
	memset(&m_Temp2DC, 0x00, sizeof(DC));

	//--------------------------------------------------------------------------
	// Calc Memory Requirement Size
	//--------------------------------------------------------------------------
	// 1. Calc Size of INDEX8 Memorey Requirement
	if (pIn->pStringCB) {
		if ((pIn->StampSize.w == 0) || (pIn->StampSize.h == 0)) {
			DBG_ERR("no StampSize w %d or h %d\r\n", pIn->StampSize.w, pIn->StampSize.h);
			r = E_PAR;
            goto FontConv_ret;
		}
		w_Stamp = pIn->StampSize.w;
		h_Stamp = pIn->StampSize.h;
		DBG_MSG("[font]String w,h,str = (%ld,%ld)\r\n", w_Stamp, h_Stamp);

	} else {
		if (pIn->pStr == NULL
			|| pIn->pFont == NULL) {
			DBG_ERR("no string/font\r\n");
			r = E_PAR;
            goto FontConv_ret;
		}

		GxGfx_SetTextStroke((const FONT *)pIn->pFont, FONTSTYLE_NORMAL, SCALE_1X);
		GxGfx_SetTextColor(IDX_FONT_SOLID, IDX_FONT_FRAME, IDX_FONT_TRANSPARETN);
		FontPalette[0] = pIn->ciTransparet;
		FontPalette[1] = pIn->ciSolid;
		FontPalette[2] = pIn->ciFrame;

		r=GxGfx_Text(0, 0, 0, pIn->pStr); //not really draw
		if(r!=GX_OK){
            DBG_ERR("pre-text %d\r\n",r);
            goto FontConv_ret;
		}
		szStamp = GxGfx_GetTextLastSize(); //just get text size
		w_Stamp = szStamp.w;
		w_Stamp = MAKE_ALIGN(w_Stamp, 8); //make w to 8 byte alignment
		h_Stamp = szStamp.h;
		h_Stamp = MAKE_ALIGN(h_Stamp, 2); //make h to 2 byte alignment
		DBG_MSG("[font]String w,h,str = (%ld,%ld),%s\r\n", w_Stamp, h_Stamp, pIn->pStr);
	}

	//prepare stamp DC
	memszStampDC = GxGfx_CalcDCMemSize(w_Stamp, h_Stamp, TYPE_BITMAP, PXLFMT_INDEX8);
	memszStampDC = MAKE_ALIGN(memszStampDC, 32);


	// 2. Calc Size of YUV422 Memorey Requirement
	w_TempDC = w_Stamp;
	h_TempDC = h_Stamp;
    {
	HD_VIDEO_FRAME      frame;

	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = gximg_fmt;
	frame.dim.h   = h_TempDC;
	frame.dim.w   = w_TempDC;
	frame.loff[0] = ALIGN_CEIL_16(w_TempDC);
	frame.loff[1] = ALIGN_CEIL_16(w_TempDC);
	memszTempDC = hd_common_mem_calc_buf_size(&frame);
	if(!memszTempDC){
		DBG_ERR("TempDC calc size fail\n");
        r=E_NOMEM;
        goto FontConv_ret;

	}

    }
    DBG_IND("2. Calc Size of out Memorey Requirement 0x%x  gximg_fmt 0x%x\r\n",memszTempDC,gximg_fmt);

	// 3. Calc Size of YUV422 Memorey Requirement
	w_Temp2DC = (pIn->ScaleFactor * w_TempDC) >> 16;
	h_Temp2DC = (pIn->ScaleFactor * h_TempDC) >> 16;
	//stamp image is prepared for HW bitblt
	//  each plane need to 4 bytes alignment, total YCC need to 8 bytes alignment!
	w_Temp2DC = MAKE_ALIGN(w_Temp2DC, 8);
	//prepare temp2 DC
    {
	HD_VIDEO_FRAME      frame;

	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = gximg_fmt;
	frame.dim.h   = h_Temp2DC;
	frame.dim.w   = w_Temp2DC;
	frame.loff[0] = ALIGN_CEIL_16(w_Temp2DC);
	frame.loff[1] = ALIGN_CEIL_16(w_Temp2DC);
	memszTemp2DC = hd_common_mem_calc_buf_size(&frame);
	if(!memszTemp2DC){
		DBG_ERR("Temp2DC calc size fail\n");
        r=E_NOMEM;
        goto FontConv_ret;
	}

    }
    DBG_IND("3. Calc scale Size of out Memorey Requirement 0x%x  gximg_fmt 0x%x\r\n",memszTemp2DC,gximg_fmt);

	switch (pIn->Deg) {
	case FONT_CONV_DEG_90:
	case FONT_CONV_DEG_270:
		w_rt = MAKE_ALIGN(h_Temp2DC, 2);
		w_Temp2DC = MAKE_ALIGN(w_Temp2DC, 16); //grph rotate need to alignment 16
		h_rt = w_Temp2DC;
        {
    	HD_VIDEO_FRAME      frame;

    	frame.sign = MAKEFOURCC('V','F','R','M');
    	frame.pxlfmt  = gximg_fmt;
    	frame.dim.h   = h_rt;
    	frame.dim.w   = w_rt;
    	frame.loff[0] = ALIGN_CEIL_16(w_rt);
    	frame.loff[1] = ALIGN_CEIL_16(w_rt);
    	size_rt = hd_common_mem_calc_buf_size(&frame);
    	if(!size_rt){
    		DBG_ERR("rt calc size fail\n");
            r=E_NOMEM;
            goto FontConv_ret;
    	}

        }

		break;
	case FONT_CONV_DEG_180:
		w_rt = w_Temp2DC;
		h_rt = h_Temp2DC;
        {
    	HD_VIDEO_FRAME      frame;

    	frame.sign = MAKEFOURCC('V','F','R','M');
    	frame.pxlfmt  = gximg_fmt;
    	frame.dim.h   = h_rt;
    	frame.dim.w   = w_rt;
    	frame.loff[0] = ALIGN_CEIL_16(w_rt);
    	frame.loff[1] = ALIGN_CEIL_16(w_rt);
    	size_rt = hd_common_mem_calc_buf_size(&frame);
    	if(!size_rt){
    		DBG_ERR("rt calc size fail\n");
            r=E_NOMEM;
            goto FontConv_ret;
    	}

        }

		break;
	default:
		w_rt = 0;
		h_rt = 0;
		size_rt = 0;
		break;
	}


	//--------------------------------------------------------------------------
	// Assign Memory
	//--------------------------------------------------------------------------
	pMemPool = MAKE_ALIGN(MemAddr, 32);
	addrTemp2 = pMemPool;
	pMemPool  += memszTemp2DC;
	addrTemp  =  MAKE_ALIGN(pMemPool, 32);
	pMemPool  += memszTempDC;
	addrStamp =  MAKE_ALIGN(pMemPool, 32);
	pMemPool  += memszStampDC;

	//use addrTemp is ok, because of released.
	addr_rt = addrTemp;
	addr_gx_rot = addr_rt + size_rt;
	addr_gx_rot = MAKE_ALIGN(addr_gx_rot, 32);

	if (pMemPool - MemAddr > pIn->MemSize) {
		DBG_ERR("Memory Require 0x%08X!\r\n", pMemPool - MemAddr);
		pOut->UsedMemSize = 0;
        r=E_NOMEM;
        goto FontConv_ret;
	}

	//--------------------------------------------------------------------------
	// Start to Stamp Genernate
	//--------------------------------------------------------------------------
	// 1. get src text size by Text(index -> YUV422)
	DBG_MSG("[font]StampDC attach-1 size=%08x, address=%08x\r\n", memszStampDC, addrStamp);
	r = GxGfx_AttachDC(&m_StampDC,
					   TYPE_BITMAP, PXLFMT_INDEX8,
					   w_Stamp, h_Stamp,
					   0, //auto calculate pitch
					   (UINT8 *)addrStamp, 0, 0); //auto calulate buf1 and buf2
	if (r != GX_OK) {
		DBG_ERR("StampDC attach-1 fail %d\r\n",r);
        goto FontConv_ret;
	}

	//clear stamp DC
	//Use SW to clean memory instead of HW clean for using CACHE
	memset((void *)addrStamp, 0x00, memszStampDC);

#if 0//CFG_DUMP_FONTCONV_RAW
	{
		FST_FILE hFile = FileSys_OpenFile("A:\\STAMI_1.RAW", FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		UINT32 filesize;
		DBG_DUMP("Dump Stamp clear INDEX8 %d %d ...", w_Stamp, h_Stamp);
		filesize = w_Stamp * h_Stamp;
		FileSys_WriteFile(hFile, (UINT8 *)addrStamp, &filesize, 0, NULL);
		FileSys_CloseFile(hFile);
		DBG_DUMP("ok\r\n ");
	}
#endif
	// 2. draw text by Text(font -> INDEX8)
	if (pIn->pStringCB) {
		pIn->pStringCB((UINT32)&m_StampDC, (UINT32)pIn->pStr, 0, 0);
	} else {
		r=GxGfx_Text(&m_StampDC, 0, 0, pIn->pStr); //draw
		if(r!=GX_OK){
            DBG_ERR("text %d\r\n",r);
            goto FontConv_ret;
		}
	}

	if(pIn->pPalette) {
		Palette = pIn->pPalette;
	} else {
		Palette = FontPalette;
	}

#if CFG_DUMP_FONTCONV_RAW
	{
        char filename[32]={0};
        static UINT32 count=0;
        sprintf(filename,"A:\\STAMI_%d.RAW",count);
        count++;
		FST_FILE hFile = FileSys_OpenFile(filename, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
		UINT32 filesize;
		DBG_DUMP("Dump Stamp INDEX8 %d %d %s...", w_Stamp, h_Stamp,filename);
		filesize = w_Stamp * h_Stamp;
		FileSys_WriteFile(hFile, (UINT8 *)addrStamp, &filesize, 0, NULL);
		FileSys_CloseFile(hFile);
		DBG_DUMP("ok\r\n ");
	}
#endif

    if((pIn->Format == HD_VIDEO_PXLFMT_ARGB8888)||(pIn->Format == HD_VIDEO_PXLFMT_ARGB1555)||
        (pIn->Format == HD_VIDEO_PXLFMT_ARGB4444)||(pIn->Format == HD_VIDEO_PXLFMT_RGB565) )
    {
    	// 3. bit blt (INDEX8 -> HD_VIDEO_PXLFMT_ARGBxxx)
    	//prepare temp DC
    	DBG_MSG("TempDC attach-2 size=%08x, address=%08x\r\n", memszTempDC, addrTemp);
        r = GxGfx_AttachDC(&m_TempDC,
                TYPE_BITMAP, GxImg2GxgfxFmt(pIn->Format),
                w_TempDC, h_TempDC,
                0, //auto calculate pitch
                (UINT8*)addrTemp, 0, 0); //auto calulate buf1 and buf2

    	if (r != GX_OK) {
    		DBG_ERR("TempDC attach-2 fail %d\r\n",r);
    		goto FontConv_ret;
    	}

    	r= GxGfx_ConvertFont(&m_TempDC,&m_StampDC,Palette);

        if (r != GX_OK) {
    		DBG_ERR("convrt1 fail %d\r\n",r);
    		goto FontConv_ret;
    	}

        GxGfx_GetAddr(&m_TempDC,&uiAddrY,0,0);
        GxGfx_GetLineOffset(&m_TempDC,&uiLineOffsetY,0,0);

#if CFG_DUMP_FONTCONV_RAW
    	{
    		FST_FILE hFile = NULL;
    		UINT32 filesize;
            char filename[32]={0};
            static UINT32 count=0;
            sprintf(filename,"A:\\STAMC_%d.RAW",count);
            count++;
    		DBG_ERR("Dump Stamp ConvertFont %d %d %s...", w_TempDC, h_TempDC,filename);
    		filesize = uiLineOffsetY * h_TempDC;
    		hFile = FileSys_OpenFile(filename, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
    		FileSys_WriteFile(hFile, (UINT8 *)uiAddrY, &filesize, 0, NULL);
    		FileSys_CloseFile(hFile);
    		DBG_DUMP("ok\r\n ");
    	}
#endif

        if(pIn->ScaleFactor!= 0x10000) {
        	DBG_MSG("Temp2DC scale memory attach-3 size=%08x, address=%08x\r\n", memszTemp2DC, addrTemp2);
            r = GxGfx_AttachDC(&m_Temp2DC,
                    TYPE_BITMAP, GxImg2GxgfxFmt(pIn->Format),
                    w_Temp2DC, h_Temp2DC,
                    0, //auto calculate pitch
                    (UINT8*)addrTemp2, 0, 0); //auto calulate buf1 and buf2

        	if (r != GX_OK) {
        		DBG_ERR("Temp2DC attach-3 fail %d\r\n",r);
        		goto FontConv_ret;
        	}

        	r=GxGfx_Convert(&m_Temp2DC,&m_TempDC,0);

            if (r != GX_OK) {
        		DBG_ERR("convrt2 fail %d\r\n",r);
        		goto FontConv_ret;
        	}
            GxGfx_GetAddr(&m_Temp2DC,&uiAddrY,0,0);
            GxGfx_GetLineOffset(&m_Temp2DC,&uiLineOffsetY,0,0);
        }

        // rotate image
        if(pIn->Deg != FONT_CONV_DEG_0) {
            DBG_ERR("not support rotate%d\r\n",pIn->Deg);
            r = E_NOSPT;
            goto FontConv_ret;
        } else {

        	pOut->ColorKeyY = pIn->ciTransparet;
        	pOut->ColorKeyCb = 0;
        	pOut->ColorKeyCr = 0;
        	pOut->UsedMemSize = addrTemp2 + memszTemp2DC - MemAddr;
        	pOut->UsedMaxMemSize = (pMemPool - MemAddr);

            //config  GxImg buffer
            {
        		UINT32 LineOffs[2] = {uiLineOffsetY, 0};
        		UINT32 PxlAddrs[2] = {uiAddrY, 0};

            	r =  vf_init_ex(&pOut->GenImg, w_Temp2DC, h_Temp2DC, gximg_fmt, LineOffs, PxlAddrs);
        		if (r != HD_OK) {
        			DBG_ERR("GxImg ini buffer fail %d\r\n",r);
        			goto FontConv_ret;
        		}

            }

            DBG_MSG("[font]Stamp Y  = %08x\r\n", pOut->GenImg.phy_addr[0]);
        	DBG_MSG("[font]Stamp LineOffset = %08x\r\n", pOut->GenImg.loff[0]);
        	DBG_MSG("[font]Stamp w,h = (%ld,%ld)\r\n", pOut->GenImg.dim.w, pOut->GenImg.ph[0]);

        }
    }
    else
    {
    	// 3. bit blt (INDEX8 -> VDO_PXLFMT_YUV422)
    	//prepare temp DC
    	DBG_MSG("F2Y attach size=%08x, address=%08x\r\n", memszTempDC, addrTemp);
    	r = F2Y_AttachDC(&m_TempF2YDC,
    					 gximg_fmt,
    					 w_TempDC, h_TempDC,
    					 addrTemp);
    	if (r != GX_OK) {
    		DBG_ERR("F2Y attach fail %d\r\n",r);
    		goto FontConv_ret;
    	}

    	uiAddrY = m_TempF2YDC.uiAddrY;
    	uiAddrCB = m_TempF2YDC.uiAddrUV;

    	//GxGfx_ConvertFont(&m_TempDC,&m_StampDC,Palette);
        IndexToYuv(&m_TempF2YDC, &m_StampDC, Palette);


#if CFG_DUMP_FONTCONV_RAW
    	{
    		FST_FILE hFile = NULL;
    		UINT32 filesize;
            char filenamey[32]={0};
            char filenameuv[32]={0};
            static UINT32 count=0;
            sprintf(filenamey,"A:\\STAMY_%d.RAW",count);
            sprintf(filenameuv,"A:\\STAMUV_%d.RAW",count);
            count++;
    		DBG_DUMP("Dump Stamp index2yuv %d %d %s...", w_TempDC, h_TempDC,filenamey );
    		filesize = w_TempDC * h_TempDC;
    		hFile = FileSys_OpenFile(filenamey, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
    		FileSys_WriteFile(hFile, (UINT8 *)uiAddrY, &filesize, 0, NULL);
    		FileSys_CloseFile(hFile);
    		hFile = FileSys_OpenFile(filenameuv, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
    		FileSys_WriteFile(hFile, (UINT8 *)uiAddrCB, &filesize, 0, NULL);
    		FileSys_CloseFile(hFile);
    		DBG_DUMP("ok\r\n ");
    	}
#endif

    	//4. stretch blt
    	{
    		UINT32 LineOffs[2] = {w_TempDC, w_TempDC};
    		UINT32 PxlAddrs[2] = {uiAddrY, uiAddrCB};
    		//HD_VIDEO_FRAME ImgSrc = {0};
 	        VF_GFX_SCALE  gfx_scale = {0};

	        HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};

            memset((void *)&vir_meminfo,0,sizeof(HD_COMMON_MEM_VIRT_INFO));
            vir_meminfo.va = (void *)(uiAddrY);
            r = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
        	if ( r != HD_OK) {
        		DBG_ERR("uiAddrY map fail %d\n",r);
        		return GX_OUTOF_MEMORY;
        	}
            PxlAddrs[0]= vir_meminfo.pa;

            memset((void *)&vir_meminfo,0,sizeof(HD_COMMON_MEM_VIRT_INFO));
            vir_meminfo.va = (void *)(uiAddrCB);
            r = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
        	if ( r != HD_OK) {
        		DBG_ERR("uiAddrCB map fail %d\n",r);
        		return GX_OUTOF_MEMORY;
        	}
            PxlAddrs[1]= vir_meminfo.pa;

    		DBG_MSG("[font]gfx_scale.src_img size=%08x, address=%08x\r\n", memszTemp2DC, addrTemp2);
           	r =  vf_init_ex(&gfx_scale.src_img,
    							w_TempDC,
    							h_TempDC,
    							gximg_fmt,
    							LineOffs,
    							PxlAddrs);

    		if (r != HD_OK) {
    			DBG_ERR("gfx_scale.src_img fail %d\r\n",r);
    			goto FontConv_ret;
    		}
            memset((void *)&vir_meminfo,0,sizeof(HD_COMMON_MEM_VIRT_INFO));
            vir_meminfo.va = (void *)(addrTemp2);
            r = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
        	if ( r != HD_OK) {
        		DBG_ERR("uiAddrCB map fail %d\n",r);
        		return GX_OUTOF_MEMORY;
        	}
           	r =  vf_init(&gfx_scale.dst_img, w_Temp2DC, h_Temp2DC, gximg_fmt, (VF_GFX_LINEOFFSET_PATTERN|16), vir_meminfo.pa, memszTemp2DC);

    		if (r != GX_OK) {
    			DBG_ERR("gfx_scale.dst_img fail\r\n");
    			goto FontConv_ret;
    		}
    		DBG_MSG("In w %d h %d Out w %d h %d\r\n",w_TempDC, h_TempDC,w_Temp2DC, h_Temp2DC);

        	gfx_scale.engine = 0;
        	gfx_scale.src_region.x = 0;
        	gfx_scale.src_region.y = 0;
        	gfx_scale.src_region.w = w_TempDC;
        	gfx_scale.src_region.h = h_TempDC;
        	gfx_scale.dst_region.x = 0;
        	gfx_scale.dst_region.y = 0;
        	gfx_scale.dst_region.w = w_Temp2DC;
        	gfx_scale.dst_region.h = h_Temp2DC;
        	gfx_scale.quality = HD_GFX_SCALE_QUALITY_NEAREST;
        	r = vf_gfx_scale(&gfx_scale, 1);
            if(r ==HD_OK){
                memcpy((void *)&pOut->GenImg,(void *)&gfx_scale.dst_img,sizeof(HD_VIDEO_FRAME));
            } else {
                DBG_ERR("scale r %d\r\n",r);
                goto FontConv_ret;
            }
    	}
    	uiAddrY = pOut->GenImg.phy_addr[0]; //get Y address
    	uiAddrCB = pOut->GenImg.phy_addr[1]; //get Cb address

    	//for Quality Up
    	if (pIn->bEnableSmooth) {
    		UINT32 i, j;
    		UINT8 *pY = (UINT8 *)uiAddrY;
    		UINT8 *pU = (UINT8 *)uiAddrCB;
    		UINT8  Y2 = (UINT8)(Palette[pIn->ciFrame] & 0xFF);
    		UINT8  U1 = (UINT8)((Palette[pIn->ciSolid] >> 8) & 0xFF);
    		UINT8  U2 = (UINT8)((Palette[pIn->ciFrame] >> 8) & 0xFF);

    		for (j = 0; j < (UINT32)pOut->GenImg.pw[0]; j++)
    			for (i = 0; i < (UINT32)pOut->GenImg.loff[0] / 2U; i++) {
    				UINT8 u0 = pU[0];
    				if (u0 == U2) {
    					pY[1] = Y2;
    				} else if (u0 == U1 && pY[1] == Y2) {
    					pY[2] = Y2;
    				}

    				pY += 2;
    				pU += 2; //uv pack, next U
    			}
    	}

    	pOut->ColorKeyY = COLOR_YUVD_GET_Y(pIn->ciTransparet);
    	pOut->ColorKeyCb = COLOR_YUVD_GET_U(pIn->ciTransparet);
    	pOut->ColorKeyCr = COLOR_YUVD_GET_V(pIn->ciTransparet);
    	pOut->UsedMemSize = addrTemp2 + memszTemp2DC - MemAddr;
    	pOut->UsedMaxMemSize = (pMemPool - MemAddr);

    	DBG_MSG("[font]Stamp Y  = %08x\r\n", pOut->GenImg.phy_addr[0]);
    	DBG_MSG("[font]Stamp CbCr = %08x\r\n", pOut->GenImg.phy_addr[1]);
    	DBG_MSG("[font]Stamp w,h = (%ld,%ld)\r\n", pOut->GenImg.dim.w, pOut->GenImg.ph[0]);

#if CFG_DUMP_FONTCONV_RAW
    	{
    		FST_FILE hFile = NULL;
    		UINT32 filesize;
            char filenamey[32]={0};
            char filenameuv[32]={0};
            static UINT32 count=0;
            sprintf(filenamey,"A:\\STASY_%d.RAW",count);
            sprintf(filenameuv,"A:\\STASUV_%d.RAW",count);
            count++;
    		DBG_DUMP("Dump scale Stamp scale %d %d %s...", pOut->GenImg.loff[0], h_Temp2DC,filenamey);
    		filesize = pOut->GenImg.loff[0] * h_Temp2DC;
    		hFile = FileSys_OpenFile(filenamey, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
    		FileSys_WriteFile(hFile, (UINT8 *)uiAddrY, &filesize, 0, NULL);
    		FileSys_CloseFile(hFile);
    		filesize = pOut->GenImg.loff[1] * h_Temp2DC;
    		hFile = FileSys_OpenFile(filenameuv, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
    		FileSys_WriteFile(hFile, (UINT8 *)uiAddrCB, &filesize, 0, NULL);
    		FileSys_CloseFile(hFile);
    		DBG_DUMP("ok\r\n ");
    	}
#endif

    	if (pIn->Deg != FONT_CONV_DEG_0) {
            HD_VIDEO_FRAME ImgDst = {0};
            VF_GFX_ROTATE param={0};
    		UINT32 FreeMemForGxRot = pIn->MemAddr + pIn->MemSize - addr_gx_rot;
    		UINT32 Rt = HD_VIDEO_DIR_ROTATE_90;

    		switch (pIn->Deg) {
    		case FONT_CONV_DEG_90:
    			Rt = HD_VIDEO_DIR_ROTATE_90;
    			break;
    		case FONT_CONV_DEG_270:
    			Rt = HD_VIDEO_DIR_ROTATE_270;
    			break;
    		case FONT_CONV_DEG_180:
    			Rt = HD_VIDEO_DIR_ROTATE_180;
    			break;
    		default:
    			r = E_SYS;//exception
                goto FontConv_ret;
    		}
        	//use gfx engine to rotate the image
        	memset(&param, 0, sizeof(VF_GFX_ROTATE));
        	r = vf_init_ex(&param.src_img, pOut->GenImg.dim.w, pOut->GenImg.dim.h, gximg_fmt, pOut->GenImg.loff,pOut->GenImg.phy_addr);
        	if (r != HD_OK) {
        		DBG_ERR("param.src_img fail %d \r\n",r);
        		goto FontConv_ret;
        	}
        	r = vf_init(&param.dst_img, w_rt, h_rt, HD_VIDEO_PXLFMT_YUV420, (VF_GFX_LINEOFFSET_PATTERN|16), addr_gx_rot, FreeMemForGxRot);
        	if(r){
        		DBG_ERR("param.dst_img fail %d \r\n",r);
        		goto FontConv_ret;
        	}

        	param.src_region.x             = 0;
        	param.src_region.y             = 0;
        	param.src_region.w             = pOut->GenImg.dim.w;
        	param.src_region.h             = pOut->GenImg.dim.h;
        	param.dst_pos.x                = 0;
        	param.dst_pos.y                = 0;
        	param.angle                    = Rt;

        	r= vf_gfx_rotate(&param);
    		if (r != HD_OK) {
    			DBG_ERR("rotate %d\r\n",r);
        		goto FontConv_ret;
    		}

            if(r ==HD_OK){
                memcpy((void *)&ImgDst,(void *)&param.dst_img,sizeof(HD_VIDEO_FRAME));
            } else {
                DBG_ERR("scale %d\r\n",r);
        		goto FontConv_ret;
            }
#if CFG_DUMP_FONTCONV_RAW
    		{
    			FST_FILE hFile = NULL;
    			UINT32 filesize;
                char filenamey[32]={0};
                char filenameuv[32]={0};
                static UINT32 count=0;
                sprintf(filenamey,"A:\\ROTY_%d.RAW",count);
                sprintf(filenameuv,"A:\\ROTYUV_%d.RAW",count);
                count++;
    			DBG_DUMP("Dump rotate Stamp YUV %d %d,Pitch=%d %s...", ImgDst.dim.w, ImgDst.dim.h, ImgDst.loff[0],filenamey);
    			filesize = ImgDst.loff[0] * ImgDst.dim.h;
    			hFile = FileSys_OpenFile(filenamey, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
    			FileSys_WriteFile(hFile, (UINT8 *)ImgDst.phy_addr[0], &filesize, 0, NULL);
    			FileSys_CloseFile(hFile);
    			hFile = FileSys_OpenFile(filenameuv, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
    			FileSys_WriteFile(hFile, (UINT8 *)ImgDst.phy_addr[1], &filesize, 0, NULL);
    			FileSys_CloseFile(hFile);
    			DBG_DUMP("ok\r\n ");
    		}
#endif
    		//GXIMG_LINEOFFSET_ALIGN(8) has to match pitch_rt

            if(r ==HD_OK){
                memcpy((void *)&pOut->GenImg,(void *)&ImgDst,sizeof(HD_VIDEO_FRAME));
            } else {
                DBG_ERR("rotate r %d\r\n",r);
                goto FontConv_ret;
            }

#if CFG_DUMP_FONTCONV_RAW
    		{
    			FST_FILE hFile = NULL;
    			UINT32 filesize;
                char filenamey[32]={0};
                char filenameuv[32]={0};
                static UINT32 count=0;
                sprintf(filenamey,"A:\\ROTSY_%d.RAW",count);
                sprintf(filenameuv,"A:\\ROTYSUV_%d.RAW",count);
                count++;
    			DBG_DUMP("Dump Stamp Rotate scale YUV %d %d,Pitch=%d %s...", pOut->GenImg.pw[0], pOut->GenImg.ph[0], pOut->GenImg.loff[0],filenamey);
    			filesize = pOut->GenImg.loff[0] * pOut->GenImg.ph[0];
    			hFile = FileSys_OpenFile(filenamey, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
    			FileSys_WriteFile(hFile, (UINT8 *)pOut->GenImg.phy_addr[0], &filesize, 0, NULL);
    			FileSys_CloseFile(hFile);
    			hFile = FileSys_OpenFile(filenameuv, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
    			FileSys_WriteFile(hFile, (UINT8 *)pOut->GenImg.phy_addr[1], &filesize, 0, NULL);
    			FileSys_CloseFile(hFile);
    			DBG_DUMP("ok\r\n ");
    		}
#endif
    	}
    }
	DBG_MSG("Stamp memory end address = %08x\r\n", pMemPool);
	DBG_MSG("Stamp memory usage size = %08x \r\n ", pMemPool - MemAddr);


#if CFG_DUMP_RESULT
	if ((pIn->Format == HD_VIDEO_PXLFMT_YUV422) || (pIn->Format == HD_VIDEO_PXLFMT_YUV420) )
    		{
    			FST_FILE hFile = NULL;
    			UINT32 filesize;
                char filenamey[32]={0};
                char filenameuv[32]={0};
                static UINT32 count=0;
                sprintf(filenamey,"A:\\GENY_%d.RAW",count);
                sprintf(filenameuv,"A:\\GENUV_%d.RAW",count);
                count++;
    			DBG_DUMP("Dump gen Stamp YUV %d %d,Pitch=%d %s...", pOut->GenImg.pw[0], pOut->GenImg.ph[0], pOut->GenImg.loff[0],filenamey);
    			filesize = pOut->GenImg.loff[0] * pOut->GenImg.ph[0];
    			hFile = FileSys_OpenFile(filenamey, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
    			FileSys_WriteFile(hFile, (UINT8 *)pOut->GenImg.phy_addr[0], &filesize, 0, NULL);
    			FileSys_CloseFile(hFile);
    			hFile = FileSys_OpenFile(filenameuv, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
    			FileSys_WriteFile(hFile, (UINT8 *)pOut->GenImg.phy_addr[1], &filesize, 0, NULL);
    			FileSys_CloseFile(hFile);
    			DBG_DUMP("ok\r\n ");
    		} else {
    		FST_FILE hFile = NULL;
    		UINT32 filesize;
            char filename[32]={0};
            static UINT32 count=0;
            sprintf(filename,"A:\\GEN_%d.RAW",count);
            count++;
    		DBG_DUMP("Dump gen Stamp ConvertFont %d %d %s...", w_TempDC, h_TempDC,filename);
    		filesize = uiLineOffsetY * h_TempDC;
    		hFile = FileSys_OpenFile(filename, FST_CREATE_ALWAYS | FST_OPEN_WRITE);
    		FileSys_WriteFile(hFile, (UINT8 *)uiAddrY, &filesize, 0, NULL);
    		FileSys_CloseFile(hFile);
    		DBG_DUMP("ok\r\n ");
    	}
#endif

FontConv_ret:
    GxGfx_Unlock();
	return r;
}
static RESULT IndexToYuv(F2Y_DC *pDestDC,  const DC *pSrcDC, const PALETTE_ITEM *pTable)
{
	LVALUE i, j;
	UINT32 DestLineOffsetY, DestLineOffsetUV, SrcLineOffset = 0;
	UINT32 DestAddrY, DestAddrUV, SrcAddr;
	GxGfx_GetAddr(pSrcDC, &SrcAddr, 0, 0);
	GxGfx_GetLineOffset(pSrcDC, &SrcLineOffset, 0, 0);

	DestAddrY = pDestDC->uiAddrY;
	DestAddrUV = pDestDC->uiAddrUV;
	DestLineOffsetY = pDestDC->uiLineOffsetY;
	DestLineOffsetUV = pDestDC->uiLineOffsetUV;


	//HD_VIDEO_PXLFMT_YUV422
	{
		UINT8 *pDestY[1];
		UINT8 *pDestUV[2];
		UINT8 *pSrcValue;
		UINT32 yshift = 0 ;

		if (pDestDC->uiPxlfmt == HD_VIDEO_PXLFMT_YUV420) {
			yshift = 1;
		}

		for (j = 0; j < pSrcDC->size.h; j++) {
			for (i = 0; i < pSrcDC->size.w; i++) {
				pSrcValue = ((UINT8 *)SrcAddr) + SrcLineOffset * j + i;

				pDestY[0] = ((UINT8 *)DestAddrY) + DestLineOffsetY * j + i;
				if (i % 2 == 0) {
					pDestUV[0] = ((UINT8 *)DestAddrUV) + DestLineOffsetUV * (j >> yshift) + i;
					pDestUV[1] = pDestUV[0] + 1;
				}
				{
					PALETTE_ITEM DestValue = pTable[*pSrcValue];

					pDestY[0][0] = COLOR_YUVD_GET_Y(DestValue);
					if (i % 2 == 0) {
						pDestUV[0][0] = COLOR_YUVD_GET_U(DestValue);
						pDestUV[1][0] = COLOR_YUVD_GET_V(DestValue);
					}

				}
			}
		}
	}
	return GX_OK;
}

static RESULT F2Y_AttachDC(F2Y_DC *pDC,
						   UINT32 uiPxlfmt,
						   UINT32 uiWidth, UINT32 uiHeight,
						   UINT32 pBuf)
{
	if (!pDC) {
		return E_PAR;
	}

	if (!pBuf) {
		return E_PAR;
	}

	//check zero
	if ((uiWidth == 0) || (uiHeight == 0)) {
		return E_PAR;
	}

	//clear whole dc
	memset(pDC, 0, sizeof(F2Y_DC));

	pDC->uiPxlfmt = uiPxlfmt;
	pDC->uiLineOffsetY = MAKE_ALIGN(uiWidth, 4);
	pDC->uiAddrY = pBuf;
	pDC->uiLineOffsetUV = MAKE_ALIGN(uiWidth, 4);
	pDC->uiAddrUV = pBuf + uiHeight * pDC->uiLineOffsetY;


	DBG_IND("uiPxlfmt %x \r\n", pDC->uiPxlfmt);
	DBG_IND("uiLineOffsetY  %d uiAddrY 0x%x\r\n", pDC->uiLineOffsetY, pDC->uiAddrY);
	DBG_IND("uiLineOffsetUV %d uiAddrUV 0x%x\r\n", pDC->uiLineOffsetUV, pDC->uiAddrUV);


	return E_OK;
}


static UINT32 GxImg2GxgfxFmt(HD_VIDEO_PXLFMT fmt)
{
    UINT32 GxgfxFmt =0;
    switch(fmt){

        case HD_VIDEO_PXLFMT_ARGB8888:
            GxgfxFmt = PXLFMT_RGBA8888_PK;
        break;
        case HD_VIDEO_PXLFMT_ARGB1555:
            GxgfxFmt = PXLFMT_RGBA5551_PK;
        break;
        case HD_VIDEO_PXLFMT_ARGB4444:
            GxgfxFmt = PXLFMT_RGBA4444_PK;
        break;
        case HD_VIDEO_PXLFMT_RGB565:
            GxgfxFmt = PXLFMT_RGB565_PK;
        break;
        default:
            DBG_ERR("not support %x\r\n",fmt);

    }
    DBG_IND("%x to GxgfxFmt %x\r\n",fmt,GxgfxFmt);
    return GxgfxFmt;

}
