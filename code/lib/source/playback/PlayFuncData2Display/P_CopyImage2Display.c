#include "PlaybackTsk.h"
#include "PlaySysFunc.h"


// This is just for test
//--------------------------------------------------
#define TEST_DSTPOS 0
#if TEST_DSTPOS
INT32 gDstPosX = 0;
INT32 gDstPosY = 0;
#endif
//---------------------------------------------------
// test end


/*
    Copy data to display frame buffer.
    This is internal API.

    @param[in] SrcBuf
    @param[in] DstBuf
    @return void
*/
void PB_CopyImage2Display(PB_DISPLAY_BUFF SrcBuf, PB_DISPLAY_BUFF DstBuf)
{
	//#NT#2011/10/04#Lincy Lin -begin
	//#NT#650GxImage
	//PVDO_FRAME pSrcImgbuf, pDstImgbuf;
	//IPOINT    Dstpos = {0};
	// Select source buffer
	switch (SrcBuf) {
	case PLAYSYS_COPY2TMPBUF:
		//pSrcImgbuf = g_pPlayTmpImgBuf;
		break;

	case PLAYSYS_COPY2FBBUF:
		//pSrcImgbuf = &gPlayFBImgBuf;
		break;

	case PLAYSYS_COPY2IMETMPBUF:
		//pSrcImgbuf = g_pPlayIMETmpImgBuf;
		break;

	default:
		DBG_ERR("SRC NG\r\n");
		return;
	}
	// Select destination buffer
	switch (DstBuf) {
	case PLAYSYS_COPY2TMPBUF:
		//pDstImgbuf = g_pPlayTmpImgBuf;
		break;

	case PLAYSYS_COPY2FBBUF:
		//pDstImgbuf = &gPlayFBImgBuf;
		break;

	case PLAYSYS_COPY2IMETMPBUF:
		//pDstImgbuf = g_pPlayIMETmpImgBuf;
		break;

	default:
		DBG_ERR("DST NG\r\n");
		return;
	}
#if TEST_DSTPOS
	Dstpos.x = gDstPosX;
	Dstpos.y = gDstPosY;
#endif
	//gximg_copy_data(pSrcImgbuf, GXIMG_REGION_MATCH_IMG, pDstImgbuf, &Dstpos);
}
