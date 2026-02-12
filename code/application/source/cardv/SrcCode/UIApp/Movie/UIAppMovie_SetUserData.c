////////////////////////////////////////////////////////////////////////////////
#include "UIApp/Movie/UIAppMovie.h"
#include "UIApp/MovieUdtaVendor.h"
#include "ImageApp/ImageApp_MovieMulti.h"
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UiAppMovie
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////

#define ADD_UDTA_TAG        DISABLE
#define ADD_CUST_TAG        DISABLE

#if (ADD_CUST_TAG == ENABLE)
static char customertag[] = "Custom Tag Sample";            // set custom info here
#endif // (ADD_CUST_TAG == ENABLE)

void Movie_SetUserData(UINT32 rec_id)
{
#if (ADD_UDTA_TAG == ENABLE)
	UINT32 format = _CFG_FILE_FORMAT_MP4;
	MOVIEMULTI_MOV_CUSTOM_TAG udta_tag = {0};

	if (rec_id < _CFG_REC_ID_MAX) {
		format = gMovie_Rec_Info[rec_id].file_format;
	} else if ((rec_id >=  _CFG_CLONE_ID_1) && (rec_id < _CFG_CLONE_ID_MAX)) {
		format = gMovie_Clone_Info[rec_id].file_format;
	}

	udta_tag.on = TRUE;
	udta_tag.tag = MAKEFOURCC('u', 'd', 't', 'a');
	if (MovieUdta_MakeVendorUserData(&(udta_tag.addr), &(udta_tag.size), format)) {
		ImageApp_MovieMulti_SetParam(rec_id, MOVIEMULTI_PARAM_FILE_CUST_TAG, (UINT32)&udta_tag);
	}
#endif // (ADD_UDTA_TAG == ENABLE)

#if (ADD_CUST_TAG == ENABLE)
	MOVIEMULTI_MOV_CUSTOM_TAG cust_tag = {0};

	cust_tag.on = TRUE;
	cust_tag.tag = MAKEFOURCC('c', 'u', 's', 't');          // set custom tag here
	cust_tag.addr = (UINT32)&customertag;
	cust_tag.size = sizeof(customertag);
	ImageApp_MovieMulti_SetParam(rec_id, MOVIEMULTI_PARAM_FILE_CUST_TAG, (UINT32)&cust_tag);
#endif // (ADD_CUST_TAG == ENABLE)
}

