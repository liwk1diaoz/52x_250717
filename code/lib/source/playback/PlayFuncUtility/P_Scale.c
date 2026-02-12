#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

//#NT#2010/08/12#Scottie -begin
//#NT#Unify the scaling functions (IME/WARP/FW) to Utility
/*
    Struct (IME/Graphic-Warp/FW) setting for scaling.
    This is internal API.

    @param[in] pScale
    @param[in] bUseIME
    @return void
*/
static void PB_ImageScale(PPLAY_SCALE_INFO pScale, PB_SCALE_WAY ScaleWay)
{
	//#NT#2011/10/20#Lincy Lin -begin
	//#NT#Mark because it will use GxImage instead
#if 0
	tUT_SCALE_SET Scale = {0};

	//Input Settings
	Scale.InAddrY  = pScale->in_addr.y_addr;
	Scale.InAddrCb = pScale->in_addr.cb_addr;
	Scale.InAddrCr = pScale->in_addr.cr_addr;
	Scale.InWidth  = pScale->io_size.in_h;
	Scale.InHeight = pScale->io_size.in_v;
	Scale.InLineOffsetY = pScale->InputLineOffset_Y;
	Scale.InLineOffsetUV = pScale->InputLineOffset_UV;
	Scale.InFormat = pScale->in_format;

	//OutPut Settings
	Scale.OutAddrY  = pScale->out_addr.y_addr;
	Scale.OutAddrCb = pScale->out_addr.cb_addr;
	Scale.OutAddrCr = pScale->out_addr.cr_addr;
	Scale.OutWidth  = pScale->io_size.out_h;
	Scale.OutHeight = pScale->io_size.out_v;
	Scale.OutLineOffsetY = pScale->OutputLineOffset_Y;
	Scale.OutLineOffsetUV = pScale->OutputLineOffset_UV;
	Scale.OutFormat = pScale->out_format;

	if ((Scale.InAddrCb == 0) || (Scale.InAddrCr == 0) || (Scale.InLineOffsetUV == 0) ||
		(Scale.OutAddrCb == 0) || (Scale.OutAddrCr == 0) || (Scale.OutLineOffsetUV == 0)) {
		// Special case for Y only image
		UINT32 uiBufSZ;

		DBG_MSG("Clear Cb&Cr buffer for Y ONLY image..\r\n");
		uiBufSZ = (Scale.OutWidth * Scale.OutHeight) >> 1;
		hwmem_open();
		hwmem_memset(Scale.OutAddrCb, 0x80, uiBufSZ);
		hwmem_memset(Scale.OutAddrCr, 0x80, uiBufSZ);
		hwmem_close();
	}

	switch (ScaleWay) {
	case PB_SCALE_IME:
		UtImageScaleByIme(&Scale);
		break;

	case PB_SCALE_WARP:
		UtImageScaleByWarp(&Scale);
		break;

	case PB_SCALE_FW:
	default:
		UtImageScaleByFW(&Scale);
		break;
	}
#endif
	//#NT#2011/10/20#Lincy Lin -end
}

/*
    IME setting for scaling.
    This is internal API.

    @param[in] pScale
    @return void
*/
void PB_IMEScaleSetting(PPLAY_SCALE_INFO pScale)
{
	PB_ImageScale(pScale, PB_SCALE_IME);
}

/*
    Graphic-Warp setting for scaling.
    This is internal API.

    @param[in] pScale
    @return void
*/
void PB_WARPScaleSetting(PPLAY_SCALE_INFO pScale)
{
	PB_ImageScale(pScale, PB_SCALE_WARP);
}

/*
    FW scaling.
    This is internal API.

    @param[in] pScale
    @return void
*/
void PB_FWScale2Display(PPLAY_SCALE_INFO pScale)
{
	PB_ImageScale(pScale, PB_SCALE_FW);
}
//#NT#2010/08/12#Scottie -end