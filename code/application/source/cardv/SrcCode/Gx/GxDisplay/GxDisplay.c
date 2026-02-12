
#include "GxDisplay.h"
#include "GxDisplay_int.h"
#include "DL.h"
#if !defined(__FREERTOS)
#include "vendor_videoout.h"
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/mman.h>
static struct fb_var_screeninfo vinfo;
static int fb_fd = 0;
#endif

#define COLOR_YUV_BLACK             0x00808000 ///< built-in color
#define COLOR_BLACK                 COLOR_YUV_BLACK

#define REMOVE              0
#define _DL_SetAutoWait(a,b)
#define _DL_Wait_VSync(a)
#define _DL_Wait_Load(a)
#define _DL_UpdateOutput(a)
#define _DL_Dirty(a,b)
//#define _DL_SetOSDMix(a,b,c,d,e,f,g,h,i,j)
//#define _DL_SetVDOMix(a,b,c,d,e,f,g,h,i,j)
#define _DL_SetCtrl(a,b,c)
//#define _DL_GetOSDCk(a,b,c)     0
#define _DL_BufferExit(a)
#define _DD_GetLayerEn(a)     0
#define _DD_SetLayerEn(a,b)

//disp-1
LAYER_BUFMAN *_gBM_1[LAYER_NUM] = {0};
DISPLAY_MIXER _gDM_1;
DISPLAY_LAYER _gDL_1[LAYER_NUM];

#if (DEVICE_COUNT >= 2)
//disp-2
LAYER_BUFMAN *_gBM_2[LAYER_NUM] = {0};
DISPLAY_MIXER _gDM_2;
DISPLAY_LAYER _gDL_2[LAYER_NUM];
#endif

UINT32 _gDisp_RenderOBJ = 0;



void  GxDisplay_RegBufMan(UINT32 LayerID, LAYER_BUFMAN *pBufMan)
{
	UINT32 cDevID = _DD(LayerID);
	UINT32 cLayerID = _DL(LayerID);

	if (cDevID == 0) {
		_gBM_1[cLayerID] = pBufMan;
	}
#if (DEVICE_COUNT >= 2)
	if (cDevID == 1) {
		_gBM_2[cLayerID] = pBufMan;
	}
#endif
}

static LAYER_BUFMAN *_DL_GetBufMan(UINT32 LayerID)
{
	LAYER_BUFMAN *pthisBM = 0;
	UINT32 cDevID = _DD(LayerID);
	UINT32 cLayerID = _DL(LayerID);
	//GX_ASSERT(LayerID < LAYER_NUM);

	if (cDevID == 0) {
		pthisBM = _gBM_1[cLayerID];
	}
#if (DEVICE_COUNT >= 2)
	if (cDevID == 1) {
		pthisBM = _gBM_2[cLayerID];
	}
#endif
	return pthisBM;
}


static DISPLAY_LAYER *_DL_GetLayer(UINT32 LayerID)
{
	DISPLAY_LAYER *pthisDL = 0;
	UINT32 cDevID = _DD(LayerID);
	UINT32 cLayerID = _DL(LayerID);
	//GX_ASSERT(LayerID < LAYER_NUM);

	if (cDevID == 0) {
		pthisDL = &(_gDL_1[cLayerID]);
	}
#if (DEVICE_COUNT >= 2)
	if (cDevID == 1) {
		pthisDL = &(_gDL_2[cLayerID]);
	}
#endif
	return pthisDL;
}

static DISPLAY_MIXER *_DL_GetMixer(UINT32 LayerID)
{
	DISPLAY_MIXER *pthisDM = 0;
	UINT32 cDevID = _DD(LayerID);

	if (cDevID == 0) {
		pthisDM = &(_gDM_1);
	}
#if defined(_BSP_NA51000_)
	if (cDevID == 1) {
		pthisDM = &(_gDM_2);
	}
#endif

	return pthisDM;
}

static void *_DL_GetBufferDC(UINT32 LayerID, UINT32 BufferID)
{
	void *pDC = 0;
	INT32 DevID = _DD(LayerID);
	LayerID = _DL(LayerID);

	if (DevID == 0) {
		if ((LayerID == 0) | (LayerID == 1) | (LayerID == 2) | (LayerID == 3)) {
			pDC = _gBM_1[LayerID]->get(BufferID);
		}
	}
#if (DEVICE_COUNT >= 2)
	if (DevID == 1) {
		if ((LayerID == 0) | (LayerID == 2)) {
			pDC = _gBM_2[LayerID]->get(BufferID);
		}
	}
#endif

	return pDC;
}

static UINT8 _GxDisplay_GetNextBufID(UINT32 LayerID, UINT8 uiBufIndex)
{
	DISPLAY_LAYER *pthisDL = _DL_GetLayer(LayerID);
	if (pthisDL == NULL) {
		return 0;
	}
	//get next buffer index
	uiBufIndex ++;
	if (uiBufIndex > pthisDL->uiBufCount) {
		uiBufIndex = 0;
	}

	return uiBufIndex;
}
#if !defined(__FREERTOS)
UINT8 *GxDisplay_InitFB(UINT32 LayerID,UINT32 uiBufAddr,UINT32 uiBufSize,UINT32 uiBufCount)
{
    VENDOR_FB_INIT fb_init;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    char *fbp = 0;
    int count =1000;

    fb_init.fb_id = HD_FB0;
    fb_init.pa_addr = uiBufAddr;
    fb_init.buf_len = uiBufSize*(uiBufCount+1);

	vendor_videoout_set(VENDOR_VIDEOOUT_ID0, VENDOR_VIDEOOUT_ITEM_FB_INIT, &fb_init);

    while(count){
    	fb_fd = open("/dev/fb0", O_RDWR);
    	if (fb_fd<0)	{
            usleep(100);
            count--;
    	} else {
    	    break;
    	}
    }
    if(fb_fd<0){
		DBG_ERR("Error: Cannot open framebuffer device.\n");
		return 0;
	}

    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo)) {
        DBG_ERR("Error reading fixed information.\n");
		return 0;
    }

    screensize =  finfo.smem_len;
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,fb_fd, 0);

    if(uiBufCount)//ping-pong buffer should set yres_virtual for pan disp
    {
        if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo)) {
            DBG_ERR("Error reading variable information\n");
        }
        vinfo.xres_virtual = vinfo.xres;
        vinfo.yres_virtual = vinfo.yres * 2;

        if (ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo) == -1) {
            DBG_ERR("Could not set variable screen info!\n");
    	}
    }
    return (UINT8 *)fbp;
}
#endif
RESULT GxDisplay_InitLayer(UINT32 LayerID, const LAYER_INIT *pInit, BOOL bClear)
{
	DISPLAY_LAYER *pthisDL = 0;
	LAYER_BUFMAN *pthisBM = 0;
	UINT16 fmt;
	register INT8 i;

	DBG_FUNC_BEGIN("\r\n");
    DBG_IND("uiSwapEffect %x\r\n",pInit->uiSwapEffect);

	pthisDL = _DL_GetLayer(LayerID);
	if (pthisDL == 0) {
		return GX_NULL_DL;
	}

	pthisBM = _DL_GetBufMan(LayerID);
	if (pthisBM == 0) {
		DBG_ERR("Lyr(0x%02x) - Buffer manager is not registered!\r\n", LayerID);
		return GX_NULL_BUFMAN;
	}

	//*((LAYER_INIT *)pthisDL) = *pInit;
	memcpy((void *)pthisDL,(void *)pInit,sizeof(LAYER_INIT));

	fmt = pthisDL->uiPxlfmt;

	//check buffer count
	if (pthisDL->uiBufCount + 1 > LAYER_BUFFER_NUM) {
		DBG_ERR("Lyr(0x%02x) - Too may buffers!\r\n", LayerID);
		return GX_OUTOF_BUFCOUNT;
	}
	//check swap effect
	if ((pthisDL->uiBufCount == 0) &&
		((pthisDL->uiSwapEffect & BUFATTR_SWAPEFFECT) != SWAPEFFECT_DISCARD)) {
		DBG_ERR("Lyr(0x%02x) - Single buffer only DISCARD swapping effect!\r\n", LayerID);
		return GX_FAIL_SWAPEFFECT;
	}

	//check buf size
	if (pthisDL->uiBufSize < pthisBM->calcsize((LAYER_INIT *)pthisDL)) {
		DBG_ERR("Lyr(0x%02x) - Buffer size is not enough!\r\n", LayerID);
		return GX_NOTENOUGH_BUFSIZE;
	}

	//check buf address
	for (i = 0; i <= pthisDL->uiBufCount; i++)
		if (pthisDL->pBufAddr[i] == 0) {
			DBG_ERR("Lyr(0x%02x) - Some buffer address is not assigned!\r\n", LayerID);
			return GX_NULL_BUF;
		}

	//clear all flag
	pthisDL->uiFlag = 0;

	//init general state //valid, but not draw, clear dirty
	pthisDL->uiFlag &= ~LRFLAG_SIGN_MASK;
	pthisDL->uiFlag |= LRFLAG_SIGN;

	//init palette state
	pthisDL->uiPalCount = 0;
	if (fmt == DISP_PXLFMT_INDEX8) {
		pthisDL->uiPalCount = 0x100;
	}

	//update dc
	pthisDL->uiFlag |= LRFLAG_INITDC;
	//assign first buffer ID
	pthisDL->uiBufShowID = 0;
	//update window state
	pthisDL->uiFlag |= LRFLAG_INITID;

	//update window state
	pthisDL->uiFlag |= LRFLAG_SETWIN;

#if 0
	pthisDL->uiEnable = 0;  //initial with disable
	pthisDL->uiFlag |= LRFLAG_SETEN;
#else
	//do NOT control enable state by GxDisplay_InitLayer()!
	//only control enable state by GxDisplay_Set()!
#endif

	if (bClear) {
		pthisDL->uiFlag |= LRFLAG_CLEAR;
	}

	return GX_OK;
}

void GxDisplay_ExitLayer(UINT32 LayerID)
{
	DISPLAY_LAYER *pthisDL = 0;
	LAYER_BUFMAN *pthisBM = 0;
	register INT8 i;

	DBG_FUNC_BEGIN("\r\n");
	pthisDL = _DL_GetLayer(LayerID);
	if (pthisDL == 0) {
		return;
	}
	pthisBM = _DL_GetBufMan(LayerID);
	if (pthisBM == 0) {
		return;
	}
	if (!((pthisDL->uiFlag & LRFLAG_SIGN_MASK) == LRFLAG_SIGN)) {
		DBG_WRN("Lyr(0x%02x) - Layer is not initialized, ignore.\r\n", LayerID);
		return; //ignore
	}

	//del DC
	for (i = 0; i <= pthisDL->uiBufCount; i++) {
		pthisBM->detach(_DL_GetBufferDC(LayerID, i));
	}

	pthisDL->uiFlag = 0;   //invalid it, clear all dirty
}

void GxDisplay_Init(UINT32 DevID, UINT32 uiDispWidth, UINT32 uiDispHeight)
{
	UINT32 cDevID = _DD(DevID);
	DBG_FUNC_BEGIN("\r\n");
#if (DEVICE_COUNT < 2)
	if (cDevID >= 1) {
		DBG_ERR("GxDisplay_Init() fail, DevID%d no support\r\n", cDevID);
		return;
	}
#endif

	//install debug cmd
	//GxDisplay_InstallCmd();

	//clear MIXER object
	{
		DISPLAY_MIXER *pthisDM = _DL_GetMixer(_LayerID(cDevID, 0));
		if (pthisDM == 0) {
			return;
		}
		memset(pthisDM, 0, sizeof(DISPLAY_MIXER));

		//init OSD mix method
		pthisDM->uiOSD1KeyOp = COMPARE_KEY_NEVER;
		pthisDM->uiOSD1KeyColor = 0;
		pthisDM->uiOSD1BlendOp = BLEND_PIXELALPHA;
		pthisDM->uiOSD1ConstAlpha = 0xff;
		pthisDM->uiOSD2KeyOp = COMPARE_KEY_NEVER;
		pthisDM->uiOSD2KeyColor = 0;
		pthisDM->uiOSD2BlendOp = BLEND_PIXELALPHA;
		pthisDM->uiOSD2ConstAlpha = 0xff;

		//init VDO mix method
		pthisDM->uiVDO1KeyOp = COMPARE_KEY_NEVER;
		pthisDM->uiVDO1KeyColor = COLOR_BLACK;
		pthisDM->uiVDO1BlendOp = BLEND_CONSTALPHA;
		pthisDM->uiVDO1ConstAlpha = 0xff;
		pthisDM->uiVDO2KeyOp = COMPARE_KEY_NEVER;
		pthisDM->uiVDO2KeyColor = COLOR_BLACK;
		pthisDM->uiVDO2BlendOp = BLEND_CONSTALPHA;
		pthisDM->uiVDO2ConstAlpha = 0xff;

		//init background - black color
		pthisDM->uiBackColor = COLOR_BLACK;
		pthisDM->uiAllCtrl = 0;

		//_DL_Init(DevID, uiDispWidth, uiDispHeight);

		pthisDM->uiFlag = 0;

		pthisDM->uiFlag |= LRFLAG_SETOSDMIX;
		pthisDM->uiFlag |= LRFLAG_SETVDOMIX;
		pthisDM->uiFlag |= LRFLAG_SETCTRL;

		pthisDM->PalStart[LAYER_OSD1] = 0;
		pthisDM->PalCount[LAYER_OSD1] = 0;
		memset(&(pthisDM->Pal[LAYER_OSD1]), 0x00, 256 * sizeof(PALETTE_ITEM));
		pthisDM->PalStart[LAYER_OSD2] = 0;
		pthisDM->PalCount[LAYER_OSD2] = 0;
		memset(&(pthisDM->Pal[LAYER_OSD2]), 0x00, 256 * sizeof(PALETTE_ITEM));
	}

	//clear all LAYER objects
	if (cDevID == 0) {
		DISPLAY_LAYER *pthisDL;
		pthisDL = _DL_GetLayer(_LayerID(cDevID, LAYER_OSD1));
		if (pthisDL != 0) {
			memset(pthisDL, 0, sizeof(DISPLAY_LAYER));
		}
		pthisDL = _DL_GetLayer(_LayerID(cDevID, LAYER_OSD2));
		if (pthisDL != 0) {
			memset(pthisDL, 0, sizeof(DISPLAY_LAYER));
		}
		pthisDL = _DL_GetLayer(_LayerID(cDevID, LAYER_VDO1));
		if (pthisDL != 0) {
			memset(pthisDL, 0, sizeof(DISPLAY_LAYER));
		}
		pthisDL = _DL_GetLayer(_LayerID(cDevID, LAYER_VDO2));
		if (pthisDL != 0) {
			memset(pthisDL, 0, sizeof(DISPLAY_LAYER));
		}
	}
#if (DEVICE_COUNT >= 2)
	if (cDevID == 1) {
		DISPLAY_LAYER *pthisDL;
		pthisDL = _DL_GetLayer(_LayerID(cDevID, LAYER_OSD1));
		if (pthisDL != 0) {
			memset(pthisDL, 0, sizeof(DISPLAY_LAYER));
		}
		pthisDL = _DL_GetLayer(_LayerID(cDevID, LAYER_VDO1));
		if (pthisDL != 0) {
			memset(pthisDL, 0, sizeof(DISPLAY_LAYER));
		}
	}
#endif
}

void GxDisplay_Exit(UINT32 DevID)
{
	UINT32 cDevID = _DD(DevID);
	DBG_FUNC_BEGIN("\r\n");
	if (cDevID == 0) {
		GxDisplay_ExitLayer(_LayerID(cDevID, LAYER_OSD1));
		GxDisplay_ExitLayer(_LayerID(cDevID, LAYER_OSD2));
		GxDisplay_ExitLayer(_LayerID(cDevID, LAYER_VDO1));
		GxDisplay_ExitLayer(_LayerID(cDevID, LAYER_VDO2));
	}
#if (DEVICE_COUNT >= 2)
	if (cDevID == 1) {
		GxDisplay_ExitLayer(_LayerID(cDevID, LAYER_OSD1));
		GxDisplay_ExitLayer(_LayerID(cDevID, LAYER_VDO1));
	}
#endif
	//_DL_Exit();
}
PALETTE_ITEM _gTempPalette[256];

void GxDisplay_SetPalette(UINT32 LayerID, UINT16 uiStartID, UINT16 uiCount, const PALETTE_ITEM *pTable)
{
	UINT16 i;
	DISPLAY_LAYER *pthisDL = 0;
	DISPLAY_MIXER *pthisDM = 0;
	UINT32 cLayerID = _DL(LayerID);

	DBG_FUNC_BEGIN("\r\n");
	pthisDL = _DL_GetLayer(LayerID);
	if (pthisDL == 0) {
		return;
	}
	pthisDM = _DL_GetMixer(LayerID);
	if (pthisDM == 0) {
		return;
	}
	if (pTable == 0) {
		DBG_ERR("Lyr(0x%02x) - no palette , ignore.\r\n", LayerID);
		return;
	}

	if ((cLayerID == LAYER_OSD1)
		|| (cLayerID == LAYER_OSD2)) {
		for (i = uiStartID; i < uiStartID + uiCount; i++) {
			pthisDM->Pal[cLayerID][i] = pTable[i];
		}
		if (pthisDL->uiFlag & LRFLAG_SETPAL) {
			pthisDM->PalStart[cLayerID] = 0;
			pthisDM->PalCount[cLayerID] = 256;
		} else {
			pthisDL->uiFlag |= LRFLAG_SETPAL;
			pthisDM->PalStart[cLayerID] = uiStartID;
			pthisDM->PalCount[cLayerID] = uiCount;
		}
	}
}

void GxDisplay_GetPalette(UINT32 LayerID, UINT16 uiStartID, UINT16 uiCount, PALETTE_ITEM *pTable)
{
	UINT16 i;
	DISPLAY_LAYER *pthisDL = 0;
	DISPLAY_MIXER *pthisDM = 0;
	UINT32 cLayerID = _DL(LayerID);

	DBG_FUNC_BEGIN("\r\n");
	pthisDL = _DL_GetLayer(LayerID);
	if (pthisDL == 0) {
		return;
	}
	pthisDM = _DL_GetMixer(LayerID);
	if (pthisDM == 0) {
		return;
	}

	switch (pthisDL->uiPxlfmt) {
	case DISP_PXLFMT_INDEX8:
		break;
	default:
		DBG_ERR("Lyr(0x%02x) - Cannot get palette in this format!\r\n", LayerID);
		return;
	}

	if ((cLayerID == LAYER_OSD1)
		|| (cLayerID == LAYER_OSD2)) {
		_DL_GetPalette(LayerID,
					   uiStartID,
					   uiCount,
					   pthisDM->Pal[cLayerID] + uiStartID);
		for (i = uiStartID; i < uiStartID + uiCount; i++) {
			pTable[i - uiStartID] = pthisDM->Pal[cLayerID][i];
		}
	}
}

void GxDisplay_SetColorKey(UINT32 LayerID, UINT32 enable,UINT32 color)
{
	DISPLAY_LAYER *pthisDL = 0;
	DISPLAY_MIXER *pthisDM = 0;
	UINT32 cLayerID = _DL(LayerID);

	DBG_FUNC_BEGIN("\r\n");
	pthisDL = _DL_GetLayer(LayerID);
	if (pthisDL == 0) {
		return;
	}
	pthisDM = _DL_GetMixer(LayerID);
	if (pthisDM == 0) {
		return;
	}

	if ((cLayerID == LAYER_OSD1)
		|| (cLayerID == LAYER_OSD2)) {
		pthisDM->uiOSD1KeyOp = enable;
		pthisDM->uiOSD1KeyColor = color;
		pthisDL->uiFlag |= LRFLAG_SETKEY;
	}
}

#if REMOVE
BOOL GxDisplay_IsReady(UINT32 LayerID)
{
	UINT32 cLayerID = _DL(LayerID);
	DBG_FUNC_BEGIN("\r\n");

	if (cLayerID < LAYER_NUM) {
		DISPLAY_LAYER *pthisDL = 0;

		pthisDL = _DL_GetLayer(LayerID);
		if (pthisDL == 0) {
			return FALSE;
		}

		return ((pthisDL->uiFlag & LRFLAG_SIGN_MASK) == LRFLAG_SIGN);
	} else {
		return TRUE;
	}
}

UINT32 _DD_GetLayerEn(UINT32 DevID);
void _DD_SetLayerEn(UINT32 DevID, UINT32 EnMask);
#endif

void GxDisplay_Set(UINT32 LayerID, UINT16 nState, UINT32 nValue)
{
	UINT32 cLayerID = _DL(LayerID);
	UINT32 *pState;

	DBG_IND("nState 0x%x nValue 0x%x\r\n",nState,nValue);

	if (cLayerID < LAYER_NUM) {
		DISPLAY_LAYER *pthisDL = 0;

		pthisDL = _DL_GetLayer(LayerID);
		if (pthisDL == 0) {
			return;
		}
		pState = (UINT32 *)pthisDL;

		//allow disable even this layer is never initialized.
		if ((nState == LAYER_STATE_ENABLE) && (nValue == 0)) {
			if (!((pthisDL->uiFlag & LRFLAG_SIGN_MASK) == LRFLAG_SIGN)) {
				pthisDL->uiFlag = 0;
			} else {
				//GX_ASSERT((pthisDL->uiFlag & LRFLAG_SIGN_MASK) == LRFLAG_SIGN);
			}
		}

		if (!(nState < LAYER_STATE_NUM)) {
			DBG_WRN("Lyr(0x%02x) - Out of state!\r\n", LayerID);
			return;
		}

		//apply state
		switch (nState) {
		case LAYER_STATE_ENABLE :
			nValue = (nValue != 0);
			{
				pState[nState] = nValue;
				pthisDL->uiFlag |= LRFLAG_SETEN;

			}
			break;
		case LAYER_STATE_BUFADDR0:
		case LAYER_STATE_BUFADDR1:
		case LAYER_STATE_BUFADDR2:
			//if(pState[nState] != nValue)
			if ((pthisDL->uiSwapEffect & BUFATTR_SWAPEFFECT) == SWAPEFFECT_DISCARD) {
				//allow modify buffer address if swapeffect = DISCARD
				pState[nState] = nValue;
				pthisDL->uiFlag |= LRFLAG_INITDC;
				pthisDL->uiFlag |= LRFLAG_SETWIN;
			}
			break;
		case LAYER_STATE_WINX :
		case LAYER_STATE_WINY :
		case LAYER_STATE_WINW :
		case LAYER_STATE_WINH :
		case LAYER_STATE_WINATTR :
			{
				pState[nState] = nValue;
				pthisDL->uiFlag |= LRFLAG_SETWIN;
			}
			break;
		case LAYER_STATE_BUFATTR :
			{
				pState[nState] = nValue;
				pthisDL->uiFlag |= LRFLAG_SETSWAP;
			}
			break;
		case LAYER_STATE_INFO :
			if (pthisDL->uiFlag & LRFLAG_INITID) { //before init layer flushed
				pState[nState] = nValue; //allow user to modifid show id (only)
			}
			break;
		}
	} else { //control
		DISPLAY_MIXER *pthisDM = 0;
		UINT32 cDevID = _DD(LayerID);
		INT32 iState = (INT32)nState;

		pthisDM = _DL_GetMixer(LayerID);
		if (pthisDM == 0) {
			return;
		}
		pState = (UINT32 *)pthisDM;

		if (!(iState < CTRL_STATE_NUM)) {
			DBG_WRN("Lyr(0x%02x) - Out of state!\r\n", LayerID);
			return;
		}

		//apply state
		if ((iState >= CTRL_STATE_BLEND_MIN) && (iState <= CTRL_STATE_BLEND_MAX)) {
			{
				pState[nState] = nValue;
				pthisDM->uiFlag |= LRFLAG_SETCTRL;
				if(iState == CTRL_STATE_ALLENABLE) {
					if(cDevID == 0) {
						_DD_SetLayerEn(DOUT1, nValue);
					} else if(cDevID == 1) {
						_DD_SetLayerEn(DOUT2, nValue);
					}
				}
			}
		}
		if ((iState >= CTRL_STATE_OSD_MIN) && (iState <= CTRL_STATE_OSD_MAX)) {
			if (pthisDM->uiFlag & LRFLAG_SETCTRL) {
				pState[nState] = nValue;
				pthisDM->uiFlag |= LRFLAG_SETOSDMIX;
			}
		}
		if ((iState >= CTRL_STATE_VDO_MIN) && (iState <= CTRL_STATE_VDO_MAX)) {
			if (pthisDM->uiFlag & LRFLAG_SETCTRL) {
				pState[nState] = nValue;
				pthisDM->uiFlag |= LRFLAG_SETVDOMIX;
			}
		}
	}
}

UINT32 GxDisplay_Get(UINT32 LayerID, UINT16 nState)
{
	UINT32 cLayerID = _DL(LayerID);
	UINT32 *pState;

	DBG_FUNC_BEGIN("\r\n");

	if (cLayerID < LAYER_NUM) {
		DISPLAY_LAYER *pthisDL = 0;

		pthisDL = _DL_GetLayer(LayerID);
		if (pthisDL == 0) {
			return 0;
		}
		pState = (UINT32 *)pthisDL;

		if (!(nState < LAYER_STATE_NUM)) {
			DBG_WRN("Lyr(0x%02x) - Out of state!\r\n", LayerID);
			return 0;
		}

		if (nState == LAYER_STATE_ENABLE) {
			pState[nState] = (_DL_GetEnable(LayerID) != 0); //update state from h/w
		}
	} else { //control
		DISPLAY_MIXER *pthisDM = 0;
		UINT32 cDevID = _DD(LayerID);

		pthisDM = _DL_GetMixer(LayerID);
		if (pthisDM == 0) {
			return 0;
		}
		pState = (UINT32 *)pthisDM;
		if(nState == CTRL_STATE_ALLENABLE) {
			if(cDevID == 0) {
				pState[nState] = _DD_GetLayerEn(DOUT1);
			} else if(cDevID == 1) {
				pState[nState] = _DD_GetLayerEn(DOUT2);
			}
		}

		if (!(nState < CTRL_STATE_NUM)) {
			DBG_WRN("Lyr(0x%02x) - Out of state!\r\n", LayerID);
			return 0;
		}
	}
	return pState[nState];
}

static void _GxDisplay_Flush(UINT32 LayerID)
{
	UINT32 cLayerID = _DL(LayerID);
	//BOOL bDirty = FALSE;
	DBG_FUNC_BEGIN("\r\n");

	if (cLayerID < LAYER_NUM) {
		DISPLAY_LAYER *pthisDL = 0;
		BOOL bDelayEnable = FALSE;

		pthisDL = _DL_GetLayer(LayerID);
		if (pthisDL == 0) {
			return;
		}

		if (!((pthisDL->uiFlag & LRFLAG_SIGN_MASK) == LRFLAG_SIGN)) {
			//allow disable even this layer is never initialized.
			if ((pthisDL->uiFlag == LRFLAG_SETEN) && (pthisDL->uiEnable == 0)) {
				//allow
			} else {
				DBG_IND("Lyr(0x%02x) - layer not init yet, ignore.\r\n", LayerID);
				return; //ignore
			}
		}

		//apply state
		if (pthisDL->uiFlag & LRFLAG_SETEN) {
			if (pthisDL->uiEnable == 0) {
				DBG_IND("set disable\r\n");
				_DL_SetEnable(LayerID, pthisDL->uiEnable);

				pthisDL->uiFlag &= ~LRFLAG_SETEN;
			} else {
				bDelayEnable = TRUE;
			}
		}
		if (pthisDL->uiFlag & LRFLAG_SETSWAP) {
			if (pthisDL->uiBufCount > 0) {
				if ((pthisDL->uiSwapEffect & BUFATTR_SWAPEFFECT) == SWAPEFFECT_DISCARD) {
					pthisDL->uiBufDrawID = pthisDL->uiBufShowID;
				} else {
					pthisDL->uiBufDrawID = _GxDisplay_GetNextBufID(LayerID, pthisDL->uiBufShowID);
				}
			}
			DBG_IND("swap uiBufDrawID %x\r\n",pthisDL->uiBufDrawID);
			pthisDL->uiFlag &= ~LRFLAG_SETSWAP;
		}
		if (pthisDL->uiFlag & LRFLAG_INITID) {
			//init buffer state
			if ((pthisDL->uiSwapEffect & BUFATTR_SWAPEFFECT) == SWAPEFFECT_DISCARD) {
				pthisDL->uiBufDrawID = pthisDL->uiBufShowID;
			} else {
				pthisDL->uiBufDrawID = _GxDisplay_GetNextBufID(LayerID, pthisDL->uiBufShowID);
			}
			DBG_IND("init uiBufDrawID %x\r\n",pthisDL->uiBufDrawID);
			pthisDL->uiFlag &= ~LRFLAG_INITID;
		}
		if (pthisDL->uiFlag & LRFLAG_INITDC) {
			LAYER_BUFMAN *pthisBM = 0;
			register INT8 i;
			//UINT16 w,h;
			//UINT16 fmt;
			DBG_IND("init DC\r\n");

			pthisBM = _DL_GetBufMan(LayerID);
			if (pthisBM == 0) {
				DBG_ERR("Lyr(0x%02x) - Buffer manager is not registered!\r\n", LayerID);
				return;
			}

			if (pthisDL->uiFlag & LRFLAG_DRAW) {
				//if DC is on drawing, cannot change window immediately
				DBG_ERR("Lyr(0x%02x) - Still under drawing, cannot init DC!\r\n", LayerID);
				goto _close_initdc;
			}

			//w = (UINT16)pthisDL->uiWidth;
			//h = (UINT16)pthisDL->uiHeight;
			//fmt = pthisDL->uiPxlfmt;

			//init DC
			for (i = 0; i <= pthisDL->uiBufCount; i++) {
				//create DC at buffer
				pthisBM->attach(_DL_GetBufferDC(LayerID, i), (LAYER_INIT *)pthisDL, i);

				if (pthisDL->uiFlag & LRFLAG_CLEAR) {
					//clear DC
					pthisBM->clear(_DL_GetBufferDC(LayerID, i));
				}
			}
_close_initdc:
			pthisDL->uiFlag &= ~LRFLAG_INITDC;
			pthisDL->uiFlag &= ~LRFLAG_CLEAR;
		}
        //not only discard should set addr, after init GxDisplay_InitLayer flush should set
		//if ((pthisDL->uiSwapEffect & BUFATTR_SWAPEFFECT) == SWAPEFFECT_DISCARD) {
		{
			DBG_IND("set win & buf\r\n");
			//do window setup (first time)
			if (pthisDL->uiFlag & LRFLAG_SETWIN) {
				UINT32 w, h;
				UINT32 fmt;
				UINT8 *pBufAddr;
				IRECT rwin;


				if(pthisDL->uiSwapEffect & SWAPEFFECT_XY) {
				h = pthisDL->uiWidth;
				w = pthisDL->uiHeight;
				rwin.x = pthisDL->win.y;
				rwin.y = pthisDL->win.x;
				rwin.w = pthisDL->win.h;
				rwin.h = pthisDL->win.w;
				} else {
				w = pthisDL->uiWidth;
				h = pthisDL->uiHeight;
				rwin.x = pthisDL->win.x;
				rwin.y = pthisDL->win.y;
				rwin.w = pthisDL->win.w;
				rwin.h = pthisDL->win.h;
				}


				fmt = pthisDL->uiPxlfmt;
				pBufAddr = pthisDL->pBufAddr[(UINT8)pthisDL->uiBufShowID];

				//destroy buffer
				_DL_BufferExit(LayerID);
				//create buffer
				_DL_BufferInit(LayerID,
							   w, h, fmt,
							   (UINT32)pBufAddr,
							   &rwin,
							   pthisDL->uiWinAttr);
            #if !defined(__FREERTOS)
            {
                UINT8 *fb_buf=0;
                HD_RESULT ERcode=0;
                HD_COMMON_MEM_VIRT_INFO vir_meminfo = {0};

                vir_meminfo.va = (void *)(pthisDL->pBufAddr[0]);
                ERcode = hd_common_mem_get(HD_COMMON_MEM_PARAM_VIRT_INFO, &vir_meminfo);
            	if ( ERcode != HD_OK) {
                	DBG_ERR("map fail %d\n",ERcode);
                    return ;
            	}

                fb_buf = GxDisplay_InitFB(LayerID,(UINT32)vir_meminfo.pa,pthisDL->uiBufSize,pthisDL->uiBufCount);
                if(fb_buf) {
                    pthisDL->pBufAddr[0] = fb_buf;
                    if(pthisDL->uiBufCount) {
                        pthisDL->pBufAddr[1] = fb_buf+pthisDL->uiBufSize;
                    }
                } else {
                    DBG_ERR("fb init fail\r\n");
                }
            }
            #endif
				pthisDL->uiFlag &= ~LRFLAG_SETWIN;
			}
		}
		if (pthisDL->uiFlag & LRFLAG_SETEN) {
			if (bDelayEnable) {
				DBG_IND("set enable\r\n");
				_DL_SetEnable(LayerID, pthisDL->uiEnable);

				pthisDL->uiFlag &= ~LRFLAG_SETEN;
			}
		}
	} else { //control
		DISPLAY_MIXER *pthisDM = 0;

		pthisDM = _DL_GetMixer(LayerID);
		if (pthisDM == 0) {
			return;
		}
        #if REMOVE
		if (pthisDM->uiFlag & LRFLAG_SETOSDMIX) {
			UINT32 O1KeyColor, O2KeyColor;
			DISPLAY_LAYER *pthisDL = 0;
			UINT32 O1Fmt, O2Fmt;
			DBG_IND("set osd mix\r\n");
			O1KeyColor = 0;
			pthisDL = _DL_GetLayer(LAYER_OSD1);
			if(pthisDL) {
				O1Fmt = pthisDL->uiPxlfmt;
				switch(O1Fmt) {
				//case DISP_PXLFMT_INDEX1 :
				//case DISP_PXLFMT_INDEX2 :
				//case DISP_PXLFMT_INDEX4 :
				case DISP_PXLFMT_INDEX8 :
					//it is a index value
					O1KeyColor = pthisDM->Pal[LAYER_OSD1][(UINT8)pthisDM->uiOSD1KeyColor];
					break;
				case DISP_PXLFMT_ARGB1555_PK :
					//it is a 16bits RGB value
					O1KeyColor = pthisDM->uiOSD1KeyColor & 0x0000ffff;
					O1KeyColor = _DL_GetOSDCk(LayerID, DISP_PXLFMT_ARGB1555_PK, O1KeyColor);
					break;
				case DISP_PXLFMT_ARGB4444_PK :
					//it is a 16bits RGB value
					O1KeyColor = pthisDM->uiOSD1KeyColor & 0x0000ffff;
					O1KeyColor = _DL_GetOSDCk(LayerID, DISP_PXLFMT_ARGB4444_PK, O1KeyColor);
					break;
				//case DISP_PXLFMT_ARGB4565_PK :
				case DISP_PXLFMT_ARGB8565_PK :
					//it is a 16bits RGB value
					O1KeyColor = pthisDM->uiOSD1KeyColor & 0x0000ffff;
					O1KeyColor = _DL_GetOSDCk(LayerID, DISP_PXLFMT_ARGB8565_PK, O1KeyColor);
					break;
				case DISP_PXLFMT_ARGB8888_PK :
					//it is a 24bits RGB value
					O1KeyColor = pthisDM->uiOSD1KeyColor & 0x00ffffff;
					O1KeyColor = _DL_GetOSDCk(LayerID, DISP_PXLFMT_ARGB8888_PK, O1KeyColor);
					break;
				default:
					break;
				}
			}
			O2KeyColor = 0;
			pthisDL = _DL_GetLayer(LAYER_OSD2);
			if(pthisDL) {
				O2Fmt = pthisDL->uiPxlfmt;
				switch(O2Fmt) {
				//case DISP_PXLFMT_INDEX1 :
				//case DISP_PXLFMT_INDEX2 :
				//case DISP_PXLFMT_INDEX4 :
				case DISP_PXLFMT_INDEX8 :
					//it is a index value
					O2KeyColor = pthisDM->Pal[LAYER_OSD2][(UINT8)pthisDM->uiOSD2KeyColor];
					break;
				default:
					break;
				}
			}
			_DL_SetOSDMix(LayerID, pthisDM->uiAllCtrl,
						  (UINT8)pthisDM->uiOSD1KeyOp, O1KeyColor, (UINT8)pthisDM->uiOSD1BlendOp, (UINT8)pthisDM->uiOSD1ConstAlpha,
						  (UINT8)pthisDM->uiOSD2KeyOp, O2KeyColor, (UINT8)pthisDM->uiOSD2BlendOp, (UINT8)pthisDM->uiOSD2ConstAlpha
						 );

			pthisDM->uiFlag &= ~LRFLAG_SETOSDMIX;
		}
		if (pthisDM->uiFlag & LRFLAG_SETVDOMIX) {
			DBG_IND("set vdo mix\r\n");
			_DL_SetVDOMix(LayerID, pthisDM->uiAllCtrl,
						  (UINT8)pthisDM->uiVDO1KeyOp, pthisDM->uiVDO1KeyColor, (UINT8)pthisDM->uiVDO1BlendOp, (UINT8)pthisDM->uiVDO1ConstAlpha,
						  (UINT8)pthisDM->uiVDO2KeyOp, pthisDM->uiVDO2KeyColor, (UINT8)pthisDM->uiVDO2BlendOp, (UINT8)pthisDM->uiVDO2ConstAlpha
						 );

			pthisDM->uiFlag &= ~LRFLAG_SETVDOMIX;
		}
        #endif
		if (pthisDM->uiFlag & LRFLAG_SETCTRL) {
			DBG_IND("set ctrl\r\n");
			_DL_SetCtrl(LayerID, pthisDM->uiAllCtrl,
						pthisDM->uiBackColor
					   );

			pthisDM->uiFlag &= ~LRFLAG_SETCTRL;
		}
	}

#if 0
	if (bDirty) {
		_DL_UpdateOutput(LayerID);
		//bDirty = FALSE;
	}
#endif
}

void  *GxDisplay_BeginDraw(UINT32 LayerID)
{
	DISPLAY_LAYER *pthisDL = 0;
	LAYER_BUFMAN *pthisBM = 0;
	BOOL bCopy;

	DBG_FUNC_BEGIN("\r\n");

	pthisDL = _DL_GetLayer(LayerID);
	if (pthisDL == 0) {
		return NULL;
	}
	pthisBM = _DL_GetBufMan(LayerID);
	if (pthisBM == 0) {
		return NULL;
	}
	//deal with BeginDraw re-entry case
	if (pthisDL->uiFlag & LRFLAG_DRAW) {
		DBG_ERR("Lyr(0x%02x) - BeginDraw before EndDraw!\r\n", LayerID);
		return _DL_GetBufferDC(LayerID, pthisDL->uiBufDrawID);
	}
	//do copy effect
	bCopy = 0;
    DBG_IND("uiSwapEffect %x\r\n",pthisDL->uiSwapEffect);
	switch (pthisDL->uiSwapEffect & BUFATTR_SWAPEFFECT) {
	case SWAPEFFECT_DISCARD :
		bCopy = 0;
		break;
	case SWAPEFFECT_COPY :
	case SWAPEFFECT_ROTATE:
		bCopy = 1;
		break;
	case SWAPEFFECT_FLIP :
		bCopy = 0;
		break;
	}

	if (bCopy) {
		DBG_IND("copy buf uiBufShowID:%x to uiBufShowID:%x uiSwapEffect:%x\r\n",pthisDL->uiBufShowID,pthisDL->uiBufDrawID,pthisDL->uiSwapEffect);
		//copy show buffer to draw buffer
		pthisBM->copy(
			_DL_GetBufferDC(LayerID, pthisDL->uiBufDrawID),
			_DL_GetBufferDC(LayerID, pthisDL->uiBufShowID),pthisDL->uiSwapEffect,0
		);

	}

	pthisDL->uiFlag |= LRFLAG_DRAW;

	//return DC of drawing buffer
	return _DL_GetBufferDC(LayerID, pthisDL->uiBufDrawID);
}
void GxDisplay_EndDraw(UINT32 LayerID, void *pLayerBuf)
{
	DISPLAY_LAYER *pthisDL = 0;
	DBG_FUNC_BEGIN("\r\n");

	pthisDL = _DL_GetLayer(LayerID);
	//GX_ASSERT((pthisDL->uiFlag & LRFLAG_SIGN_MASK) == LRFLAG_SIGN);
	if (pthisDL == 0) {
		return;
	}
	if (pLayerBuf == NULL) {
		DBG_ERR("Lyr(0x%02x) - EndDraw without buf!\r\n", LayerID);
		return;
	}

	//deal with BeginDraw re-entry case
	if (!(pthisDL->uiFlag & LRFLAG_DRAW)) {
		DBG_ERR("Lyr(0x%02x) - EndDraw before BeginDraw!\r\n", LayerID);
		return;
	}

	pthisDL->uiFlag &= ~LRFLAG_DRAW;
	pthisDL->uiFlag |= LRFLAG_FLIP;

	_DL_Dirty(LayerID, TRUE);
}
#if REMOVE
void GxDisplay_ForceFlip(UINT32 LayerID)
{
	DISPLAY_LAYER *pthisDL = 0;

	DBG_FUNC_BEGIN("\r\n");

	pthisDL = _DL_GetLayer(LayerID);
	//GX_ASSERT((pthisDL->uiFlag & LRFLAG_SIGN_MASK) == LRFLAG_SIGN);
	if (pthisDL == 0) {
		return;
	}

	pthisDL->uiFlag |= LRFLAG_FLIP;

	_DL_Dirty(LayerID, TRUE);
}
#endif
static void _GxDisplay_Flip(UINT32 LayerID)
{
	UINT32 cLayerID = _DL(LayerID);
	DISPLAY_LAYER *pthisDL = 0;
	DISPLAY_MIXER *pthisDM = 0;
	BOOL bSwap;



	pthisDL = _DL_GetLayer(LayerID);
	if (pthisDL == 0) {
		return;
	}

	DBG_IND("uiFlag  %x\r\n",pthisDL->uiFlag);

	pthisDM = _DL_GetMixer(LayerID);
	if (pthisDM == 0) {
		return;
	}
	//check palette flag
	if (pthisDL->uiFlag & LRFLAG_SETPAL) {
		if ((cLayerID == LAYER_OSD1)
			|| (cLayerID == LAYER_OSD2)) {
    	    DBG_IND("set pal \r\n");
			_DL_SetPalette(LayerID,
						   pthisDM->PalStart[cLayerID],
						   pthisDM->PalCount[cLayerID],
						   pthisDM->Pal[cLayerID] + pthisDM->PalStart[cLayerID]);
		}
		pthisDL->uiFlag &= ~LRFLAG_SETPAL;
	}

    //check key flag,should be set format first
	if (pthisDL->uiFlag & LRFLAG_SETKEY) {
		if ((cLayerID == LAYER_OSD1)
			|| (cLayerID == LAYER_OSD2)) {
    	    DBG_IND("set color key %x \r\n",pthisDM->uiOSD1KeyColor);
			_DL_SetColorKey(LayerID,
                           pthisDL->uiPxlfmt,
						   pthisDM->uiOSD1KeyOp,
						   pthisDM->uiOSD1KeyColor);
		}
		pthisDL->uiFlag &= ~LRFLAG_SETKEY;
	}

	//check draw flag
	if (pthisDL->uiFlag & LRFLAG_DRAW) {
		DBG_WRN("Lyr(0x%02x) - Flip before EndDraw, ignore this Flip.\r\n", LayerID);
		return; //cannot flip when drawing
	}
	//check draw flag
	if (!(pthisDL->uiFlag & LRFLAG_FLIP) && !(pthisDL->uiFlag & LRFLAG_SETWIN)) {
		if (pthisDL->uiEnable) {
			DBG_WRN("Lyr(0x%02x) - No BeginDraw~EndDraw, ignore this Flip.\r\n", LayerID);
		}
		return; //not need to flip
	}

	//do swap effect
	bSwap = 0;

	switch (pthisDL->uiSwapEffect & BUFATTR_SWAPEFFECT) {
	case SWAPEFFECT_DISCARD :
		bSwap = 0;
		break;
	case SWAPEFFECT_COPY :
	case SWAPEFFECT_ROTATE:
		bSwap = 1;
		break;
	case SWAPEFFECT_FLIP :
		bSwap = 1;
		break;
	}

	if (bSwap) {
		//change new show buffer to old draw buffer
		pthisDL->uiBufShowID = pthisDL->uiBufDrawID;
		//change old draw buffer to new draw buffer
		pthisDL->uiBufDrawID = _GxDisplay_GetNextBufID(LayerID, pthisDL->uiBufDrawID);
	    DBG_IND("swap uiBufShowID: %x uiBufDrawID: %x \r\n",pthisDL->uiBufShowID,pthisDL->uiBufDrawID);

		//do window setup (first time)
		if (pthisDL->uiFlag & LRFLAG_SETWIN) {
			UINT16 w, h;
			UINT16 fmt;
			UINT8 *pBufAddr;
			IRECT rwin;

			DBG_IND("set win & buf\r\n");

			if(pthisDL->uiSwapEffect & SWAPEFFECT_XY) {
				h = (UINT16)pthisDL->uiWidth;
				w = (UINT16)pthisDL->uiHeight;
				rwin.x = pthisDL->win.y;
				rwin.y = pthisDL->win.x;
				rwin.w = pthisDL->win.h;
				rwin.h = pthisDL->win.w;
				} else {
				w = (UINT16)pthisDL->uiWidth;
				h = (UINT16)pthisDL->uiHeight;
				rwin.x = pthisDL->win.x;
				rwin.y = pthisDL->win.y;
				rwin.w = pthisDL->win.w;
				rwin.h = pthisDL->win.h;
			}


			fmt = pthisDL->uiPxlfmt;
			pBufAddr = pthisDL->pBufAddr[(UINT8)pthisDL->uiBufShowID];

			//destroy buffer
			_DL_BufferExit(LayerID);
			//create buffer
			_DL_BufferInit(LayerID,
						   w, h, fmt,
						   (UINT32)pBufAddr,
						   &rwin,
						   pthisDL->uiWinAttr);

			pthisDL->uiFlag &= ~LRFLAG_SETWIN;
		} else { //do buffer swap
			UINT16 w, h;
			UINT16 fmt;
			UINT8 *pBufAddr;

			if(pthisDL->uiSwapEffect & SWAPEFFECT_XY) {
			h = (UINT16)pthisDL->uiWidth;
			w = (UINT16)pthisDL->uiHeight;
			} else {
			w = (UINT16)pthisDL->uiWidth;
			h = (UINT16)pthisDL->uiHeight;
			}
			fmt = pthisDL->uiPxlfmt;
			pBufAddr = pthisDL->pBufAddr[(UINT8)pthisDL->uiBufShowID];
			DBG_IND("set uiBufShowID:%d addr:%x \r\n",pthisDL->uiBufShowID,pBufAddr);

			//swap display layer buffer to new show buffer

			_DL_BufferSwitch(LayerID,
							 w, h, fmt,
							 (UINT32)pBufAddr);

#if !defined(__FREERTOS)
            if(fb_fd)
            {

                if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo)) {
                    DBG_ERR("Error reading variable information\n");
                }

                vinfo.xoffset = 0;
                if(pthisDL->uiBufShowID)
        			vinfo.yoffset = vinfo.yres;
                else
                    vinfo.yoffset =0;

    			if (ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo)) {
    				DBG_ERR("Error to do pan display. %s\n",strerror(errno));
			    }
            }
#endif
		}
	} else {
		//do window setup (first time)
		if (pthisDL->uiFlag & LRFLAG_SETWIN) {
			UINT16 w, h;
			UINT16 fmt;
			UINT8 *pBufAddr;
			IRECT rwin;

			DBG_IND("set win & buf\r\n");

			if(pthisDL->uiSwapEffect & SWAPEFFECT_XY) {
			h = (UINT16)pthisDL->uiWidth;
			w = (UINT16)pthisDL->uiHeight;

			} else {
			w = (UINT16)pthisDL->uiWidth;
			h = (UINT16)pthisDL->uiHeight;
			}
			rwin.y = pthisDL->win.x;
			rwin.x = pthisDL->win.y;
			rwin.w = pthisDL->win.w;
			rwin.h = pthisDL->win.h;
			fmt = pthisDL->uiPxlfmt;
			pBufAddr = pthisDL->pBufAddr[(UINT8)pthisDL->uiBufShowID];

			//destroy buffer
			_DL_BufferExit(LayerID);
			//create buffer
			_DL_BufferInit(LayerID,
						   w, h, fmt,
						   (UINT32)pBufAddr,
						   &rwin,
						   pthisDL->uiWinAttr);

			pthisDL->uiFlag &= ~LRFLAG_SETWIN;
		}
	}

    //check key flag,should be set format first
	if (pthisDL->uiFlag & LRFLAG_SETKEY) {
		if ((cLayerID == LAYER_OSD1)
			|| (cLayerID == LAYER_OSD2)) {
    	    DBG_IND("set color key %x \r\n",pthisDM->uiOSD1KeyColor);
			_DL_SetColorKey(LayerID,
                           pthisDL->uiPxlfmt,
						   pthisDM->uiOSD1KeyOp,
						   pthisDM->uiOSD1KeyColor);
		}
		pthisDL->uiFlag &= ~LRFLAG_SETKEY;
	}

	pthisDL->uiFlag &= ~LRFLAG_FLIP;
}

void GxDisplay_FlipEx(UINT32 LayerID, BOOL bWait)
{
	UINT32 cDevID = _DD(LayerID);

	DBG_FUNC_BEGIN("\r\n");

	if (cDevID == 0) {
		//begin send Command
		_GxDisplay_Flip(_LayerID(cDevID, LAYER_OSD1));
		_GxDisplay_Flip(_LayerID(cDevID, LAYER_OSD2));
		_GxDisplay_Flip(_LayerID(cDevID, LAYER_VDO1));
		_GxDisplay_Flip(_LayerID(cDevID, LAYER_VDO2));
		_DL_UpdateOutput(LayerID);
		//end send Command

		_DL_Dirty(_LayerID(cDevID, LAYER_OSD1), FALSE);
		_DL_Dirty(_LayerID(cDevID, LAYER_OSD2), FALSE);
		_DL_Dirty(_LayerID(cDevID, LAYER_VDO1), FALSE);
		_DL_Dirty(_LayerID(cDevID, LAYER_VDO2), FALSE);
	}
#if (DEVICE_COUNT >= 2)
	if (cDevID == 1) {
		//begin send Command
		_GxDisplay_Flip(_LayerID(cDevID, LAYER_OSD1));
		_GxDisplay_Flip(_LayerID(cDevID, LAYER_VDO1));
		_DL_UpdateOutput(LayerID);
		//end send Command

		_DL_Dirty(_LayerID(cDevID, LAYER_OSD1), FALSE);
		_DL_Dirty(_LayerID(cDevID, LAYER_VDO1), FALSE);
	}
#endif
	if (bWait) {
		//WAIT

		_DL_Wait_Load(LayerID); //Wait until last command is loaded completely.
		//_DL_Wait_VSync(LayerID); //Wait until last frame is updated completely.
	}
}

void GxDisplay_WaitVDEx(UINT32 LayerID)
{
	DBG_FUNC_BEGIN("\r\n");
	//_DL_Wait_VSync(LayerID);
}
#if REMOVE

/////////////////////////////////////
// Macro function


//void GxDisplay_SetSrcWindow(UINT32 LayerID, LVALUE x, LVALUE y, LVALUE w, LVALUE h)
void GxDisplay_SetSrcWindow(UINT32 LayerID, INT32 x, INT32 y, INT32 w, INT32 h)
{
}

//void GxDisplay_SetDestWindow(UINT32 LayerID, LVALUE x, LVALUE y, LVALUE w, LVALUE h, UINT32 uiMirror)
void GxDisplay_SetDestWindow(UINT32 LayerID, INT32 x, INT32 y, INT32 w, INT32 h, UINT32 uiMirror)
{
	GxDisplay_Set(LayerID, LAYER_STATE_WINX, x);
	GxDisplay_Set(LayerID, LAYER_STATE_WINY, y);
	GxDisplay_Set(LayerID, LAYER_STATE_WINW, w);
	GxDisplay_Set(LayerID, LAYER_STATE_WINH, h);
	GxDisplay_Set(LayerID, LAYER_STATE_WINATTR, uiMirror);
}

void GxDisplay_SetSwapEffect(UINT32 LayerID, UINT32 uiSwapEffect)
{
	UINT32 v, v2;
	v = GxDisplay_Get(LayerID, LAYER_STATE_BUFATTR);
	v2 = BUFATTR_MAKE(uiSwapEffect, BUFATTR_GET_BUFCOUNT(v));
	GxDisplay_Set(LayerID, LAYER_STATE_BUFATTR, v2);
}
#endif
void GxDisplay_FlushEx(UINT32 LayerID, BOOL bWait)
{
	UINT32 cDevID = _DD(LayerID);
	UINT32 cLayerID = _DL(LayerID);

	DBG_FUNC_BEGIN("\r\n");
	if (cLayerID == LAYER_ALL) {
		if (!bWait) {
			_DL_SetAutoWait(LayerID, FALSE);
		}
		if (cDevID == 0) {
//			_GxDisplay_Flush(_LayerID(cDevID, LAYER_OUTPUT)); //not support control
			_GxDisplay_Flush(_LayerID(cDevID, LAYER_VDO1));
			_GxDisplay_Flush(_LayerID(cDevID, LAYER_VDO2));
			_GxDisplay_Flush(_LayerID(cDevID, LAYER_OSD1));
			_GxDisplay_Flush(_LayerID(cDevID, LAYER_OSD2));
		}
#if (DEVICE_COUNT >= 2)
		if (cDevID == 1) {
//			_GxDisplay_Flush(_LayerID(cDevID, LAYER_OUTPUT)); //not support control
			_GxDisplay_Flush(_LayerID(cDevID, LAYER_VDO1));
			_GxDisplay_Flush(_LayerID(cDevID, LAYER_OSD1));
		}
#endif
		if (!bWait) {
			_DL_SetAutoWait(LayerID, TRUE);
		}
	} else { //Only 1 LAYER
		if (!bWait) {
			_DL_SetAutoWait(LayerID, FALSE);
		}
		_GxDisplay_Flush(LayerID);
		if (!bWait) {
			_DL_SetAutoWait(LayerID, TRUE);
		}
	}
}




//--------------------------------------------------------------------------------------
//  function - others (internal render function)
//--------------------------------------------------------------------------------------
UINT32 GxRender_GetRenderObject(void)
{
	return _gDisp_RenderOBJ;
}

void GxRender_SetScale(int scale)
{
}
#if REMOVE

void GxRender_TriggerInput(UINT32 LayerID)
{
	_DL_Dirty(LayerID, TRUE);
	//_GxDisplay_Flip(LayerID);
}
void GxRender_SetInputCB(UINT32 LayerID, GXRENDER_INPUT_CB pfInputCB)
{
}
#endif
//thread-safe API
void GxRender_Reflash(void)
{
	_DL_UpdateOutput(DOUT1);
}









