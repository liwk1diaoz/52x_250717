#include "UIControl/UIControlExt.h"



///////////////////////////////////////////////////////////////////////////////////////
// Default ctrl event table
////////////////////////////////////////////////////////////////////////////////////////

extern VControl UxWndCtrl;
extern VControl UxPanelCtrl;
extern VControl UxStaticCtrl;
extern VControl UxStateCtrl;
extern VControl UxStateGraphCtrl;
extern VControl UxTabCtrl;
extern VControl UxButtonCtrl;
extern VControl UxMenuCtrl;
extern VControl UxListCtrl;
extern VControl UxSliderCtrl;
extern VControl UxScrollbarCtrl;
extern VControl UxProgressbarCtrl;
extern VControl UxZoomCtrl;


//the order of this array should be the same as CTRL_TYPE
VControl *gUxCtrlTypeList[] = {
	0,                    //CTRL_BASE
	&UxWndCtrl,           //CTRL_WND
	&UxPanelCtrl,         //CTRL_PANEL
	&UxStaticCtrl,        //CTRL_STATIC
	&UxStateCtrl,         //CTRL_STATE
	&UxStateGraphCtrl,    //CTRL_STATEGRAPH
	&UxTabCtrl,           //CTRL_TAB
	&UxButtonCtrl,        //CTRL_BUTTON
	&UxMenuCtrl,          //CTRL_MENU
	&UxListCtrl,          //CTRL_LIST
	&UxSliderCtrl,        //CTRL_SLIDER
	&UxScrollbarCtrl,     //CTRL_SCROLLBAR
	&UxProgressbarCtrl,   //CTRL_PROGRESSBAR
	&UxZoomCtrl,          //CTRL_ZOOM
	0,                    //CTRL_USER
	0
};



