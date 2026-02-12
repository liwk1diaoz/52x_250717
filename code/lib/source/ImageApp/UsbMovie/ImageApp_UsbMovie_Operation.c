#include "ImageApp_UsbMovie_int.h"

/// ========== Cross file variables ==========
/// ========== Noncross file variables ==========

ER _ImageApp_UsbMovie_UvacStart(UINT32 id, UINT32 path)
{
	ER ret;
	UINT32 i = id, idx = id, j = path;
	FLGPTN vflag;

	if (usbimg_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
		_LinkPortCntInc(&(UsbImgLinkStatus[idx].vproc_p_phy[1]));
	}

	_LinkPortCntInc(&(UsbImgLinkStatus[idx].vcap_p[0]));

	//if ((usbimg_vproc_param[idx][2].dim.w != usbimg_vproc_param[idx][j].dim.w) && (usbimg_vproc_param[idx][2].dim.h != usbimg_vproc_param[idx][j].dim.h)) {
	//	_LinkPortCntInc(&(UsbImgLinkStatus[idx].vproc_p_phy[j]));
	//}
	if (usbimg_vproc_param_ex[idx][j].src_path != UsbImgLink[idx].vproc_p_phy[1]) {
		_LinkPortCntInc(&(UsbImgLinkStatus[idx].vproc_p_phy[j]));
	}

	_LinkPortCntInc(&(UsbImgLinkStatus[idx].vproc_p[j]));
	_LinkPortCntInc(&(UsbImgLinkStatus[idx].venc_p[j]));

	ret = _ImageApp_UsbMovie_ImgLinkStatusUpdate(i, UPDATE_REVERSE);
	vflag = FLGIAUSBMOVIE_VE_M0 << (1 * idx + j);
	vos_flag_set(IAUSBMOVIE_FLG_ID, vflag);

	return ret;
}

ER _ImageApp_UsbMovie_UvacStop(UINT32 id, UINT32 path)
{
	ER ret;
	UINT32 i = id, idx = id, j = path;
	FLGPTN vflag;

	vflag = FLGIAUSBMOVIE_VE_M0 << (1 * idx + j);
	vos_flag_clr(IAUSBMOVIE_FLG_ID, vflag);
	vos_flag_wait_timeout(&vflag, IAUSBMOVIE_FLG_ID, FLGIAUSBMOVIE_TSK_VE_IDLE, TWF_ORW, vos_util_msec_to_tick(100));

	if (usbimg_vproc_ctrl[idx].func & HD_VIDEOPROC_FUNC_3DNR) {
		_LinkPortCntDec(&(UsbImgLinkStatus[idx].vproc_p_phy[1]));
	}

	_LinkPortCntDec(&(UsbImgLinkStatus[idx].vcap_p[0]));

	//if ((usbimg_vproc_param[idx][2].dim.w != usbimg_vproc_param[idx][j].dim.w) && (usbimg_vproc_param[idx][2].dim.h != usbimg_vproc_param[idx][j].dim.h)) {
	//	_LinkPortCntDec(&(UsbImgLinkStatus[idx].vproc_p_phy[j]));
	//}
	if (usbimg_vproc_param_ex[idx][j].src_path != UsbImgLink[idx].vproc_p_phy[1]) {
		_LinkPortCntDec(&(UsbImgLinkStatus[idx].vproc_p_phy[j]));
	}

	_LinkPortCntDec(&(UsbImgLinkStatus[idx].vproc_p[j]));
	_LinkPortCntDec(&(UsbImgLinkStatus[idx].venc_p[j]));

	ret = _ImageApp_UsbMovie_ImgLinkStatusUpdate(i, UPDATE_FORWARD);

	return ret;
}

