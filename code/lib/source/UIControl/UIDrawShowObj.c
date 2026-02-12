
#include "UIControl/UIControlExt.h"
#include "UIControlInternal.h"
#include "UIControl/UIDrawShowObj.h"

#define DRAW_OSD    0
#define DRAW_VDO    1

static PALETTE_ITEM **g_PaletteTable = 0;
static MAPPING_ITEM **g_ColorMapTable = 0;
static FONT **g_fontTable = 0;
static UINT32 gGlobalColorKey = 0;
static UINT32 bUseGlobalKey = FALSE;
//#NT#2008/09/10#lincy-begin

SHOWOBJ_FUNC_PTR pShowObjFuncTable[CMD_MAX] = {0};

void Ux_DrawShowRect(UIScreen ScreenObj, ITEM_RECTANGLE *pShowObj);
void Ux_DrawShowRoundRect(UIScreen ScreenObj, ITEM_ROUNDRECT *pShowObj);
void Ux_DrawShowEllipse(UIScreen ScreenObj, ITEM_ELLIPSE *pShowObj);
void Ux_DrawShowLine(UIScreen ScreenObj, ITEM_LINE *pShowObj);
void Ux_DrawShowImage(UIScreen ScreenObj, ITEM_IMAGE *pShowObj);
void Ux_DrawShowText(UIScreen ScreenObj, ITEM_TEXT *pShowObj);


void Ux_SetShowObjFunc(UINT32 cmd, SHOWOBJ_FUNC_PTR pFunc)
{
	pShowObjFuncTable[cmd] = pFunc;
}
void Ux_SetPaletteTable(MAPPING_ITEM **pPaletteTable)
{
	g_PaletteTable = pPaletteTable;
}
PALETTE_ITEM *Ux_GetPaletteTable(UINT32 index)
{
	if (!g_PaletteTable) {
		return 0;
	} else {
		return g_PaletteTable[index];
	}
}
void Ux_SetColorMapTable(MAPPING_ITEM **pColorMapTable)
{
	g_ColorMapTable = pColorMapTable;
}
MAPPING_ITEM *Ux_GetColorMapTable(UINT32 index)
{
	if (!g_ColorMapTable) {
		return 0;
	} else {
		return g_ColorMapTable[index];
	}
}
void Ux_SetFontTable(FONT **pFontTable)
{
	g_fontTable = pFontTable;
}

FONT *Ux_GetFontTable(UINT32 index)
{
	if (!g_fontTable) {
		DBG_ERR("No font table\r\n");
		return 0;
	} else {
		return g_fontTable[index];
	}
}

void Ux_SetGlobalColorKey(UINT32 bEnable, UINT32 colorKey)
{
	bUseGlobalKey = bEnable;
	gGlobalColorKey = colorKey;
}

void Ux_GetGlobalColorKey(UINT32 *bEnable, UINT32 *colorKey)
{
	*bEnable = bUseGlobalKey ;
	*colorKey = gGlobalColorKey;
}
void Ux_DrawShowObj(UIScreen ScreenObj, ITEM_BASE *pShowObj)
{
	if (pShowObjFuncTable[(pShowObj->ItemType & CMD_MASK)]) {
		pShowObjFuncTable[(pShowObj->ItemType & CMD_MASK)](ScreenObj, pShowObj);
	} else {
		switch ((pShowObj->ItemType & CMD_MASK)) {
		case CMD_Rectangle:
			Ux_DrawShowRect(ScreenObj, (ITEM_RECTANGLE *)pShowObj);
			break;
		case CMD_RoundRect:
			Ux_DrawShowRoundRect(ScreenObj, (ITEM_ROUNDRECT *)pShowObj);
			break;
		case CMD_Ellipse:
			Ux_DrawShowEllipse(ScreenObj, (ITEM_ELLIPSE *)pShowObj);
			break;
		case CMD_Line:
			Ux_DrawShowLine(ScreenObj, (ITEM_LINE *)pShowObj);
			break;
		case CMD_Image:
			Ux_DrawShowImage(ScreenObj, (ITEM_IMAGE *)pShowObj);
			break;
		case CMD_Text:
			Ux_DrawShowText(ScreenObj, (ITEM_TEXT *)pShowObj);
			break;
		case CMD_Group:
			Ux_DrawShowTable(ScreenObj, ((ITEM_GROUP *)pShowObj)->ShowTable);
			break;
		default:
			DBG_ERR("Ux_DrawShowObj->Wrong Type\n\r");

		}
	}
}
void Ux_DrawItemByStatus(UIScreen ScreenObj, ITEM_BASE *pStatusGroup, UINT32 stringID, UINT32 iconID, UINT32 value)
{
	//status show obj must be gourp
	if ((pStatusGroup->ItemType  & CMD_MASK) == CMD_Group) {
		ITEM_BASE  **showTable;
		ITEM_BASE  *pStatusShowObj;
		UINT32      ObjIndex = 0;
		//get this status show table
		showTable = ((ITEM_GROUP *)pStatusGroup)->ShowTable;

		while (showTable[ObjIndex] != NULL) {
			pStatusShowObj = showTable[ObjIndex];
			if ((pStatusShowObj->ItemType  & CMD_MASK) == CMD_Group) {
				//draw menu item background( bg must be a show obj of goup type,and need be first one showObj)
				Ux_DrawShowObj(ScreenObj, (ITEM_BASE *)pStatusShowObj);
			} else if ((pStatusShowObj->ItemType  & CMD_MASK) == CMD_Text) {
				if ((pStatusShowObj->ItemType & CMD_EXT_MASK)& CMD_VALUE) {
					Ux_SetShowObjProperty(pStatusShowObj, PRO_STRID, value);
				} else if ((pStatusShowObj->ItemType & CMD_EXT_MASK)& CMD_ITEM) {
					Ux_SetShowObjProperty(pStatusShowObj, PRO_STRID, stringID);
				}
				Ux_DrawShowObj(ScreenObj, (ITEM_BASE *)pStatusShowObj);
			} else if ((pStatusShowObj->ItemType & CMD_MASK) == CMD_Image) {
				if ((pStatusShowObj->ItemType & CMD_EXT_MASK)& CMD_VALUE) {
					Ux_SetShowObjProperty(pStatusShowObj, PRO_ICONID, value);
				} else if ((pStatusShowObj->ItemType & CMD_EXT_MASK)& CMD_ITEM) {
					Ux_SetShowObjProperty(pStatusShowObj, PRO_ICONID, iconID);
				}
				Ux_DrawShowObj(ScreenObj, (ITEM_BASE *)pStatusShowObj);
			} else {
				Ux_DrawShowObj(ScreenObj, (ITEM_BASE *)pStatusShowObj);
			}
			ObjIndex++;
		}
	}

}

void Ux_SetOrigin(UIScreen ScreenObj, LVALUE x, LVALUE y)
{
	GxGfx_SetOrigin(((DC **)ScreenObj)[DRAW_OSD], x, y);
}

IPOINT Ux_GetOrigin(UIScreen ScreenObj)
{
	return GxGfx_GetOrigin(((DC **)ScreenObj)[DRAW_OSD]);
}

void Ux_DrawShowRect(UIScreen ScreenObj, ITEM_RECTANGLE *pShowObj)
{
	PALETTE_ITEM *pPalette = Ux_GetPaletteTable(0);
	GxGfx_SetShapeStroke(pShowObj->uiLineStyle, pShowObj->uiFillStyle);
	if (pPalette == 0) {
		GxGfx_SetShapeColor(pShowObj->uiForeColor, pShowObj->uiBackColor, pShowObj->uiColorMapTable);
	} else {
		GxGfx_SetShapeColor(pPalette[pShowObj->uiForeColor], pPalette[pShowObj->uiBackColor], pShowObj->uiColorMapTable);
	}
	GxGfx_SetShapeLayout(pShowObj->uiLayout, pShowObj->uiAlignment);
	GxGfx_Rectangle(((DC **)ScreenObj)[DRAW_OSD], pShowObj->x1, pShowObj->y1, pShowObj->x2, pShowObj->y2);
}

void Ux_DrawShowRoundRect(UIScreen ScreenObj, ITEM_ROUNDRECT *pShowObj)
{
	PALETTE_ITEM *pPalette = Ux_GetPaletteTable(0);
	GxGfx_SetShapeStroke(pShowObj->uiLineStyle, pShowObj->uiFillStyle);
	if (pPalette == 0) {
		GxGfx_SetShapeColor(pShowObj->uiForeColor, pShowObj->uiBackColor, pShowObj->uiColorMapTable);
	} else {
		GxGfx_SetShapeColor(pPalette[pShowObj->uiForeColor], pPalette[pShowObj->uiBackColor], pShowObj->uiColorMapTable);
	}
	GxGfx_SetShapeLayout(pShowObj->uiLayout, pShowObj->uiAlignment);
	GxGfx_RoundRect(((DC **)ScreenObj)[DRAW_OSD], pShowObj->x1, pShowObj->y1, pShowObj->x2, pShowObj->y2, pShowObj->RoundX, pShowObj->RoundY);
}

void Ux_DrawShowEllipse(UIScreen ScreenObj, ITEM_ELLIPSE *pShowObj)
{
	PALETTE_ITEM *pPalette = Ux_GetPaletteTable(0);
	GxGfx_SetShapeStroke(pShowObj->uiLineStyle, pShowObj->uiFillStyle);
	if (pPalette == 0) {
		GxGfx_SetShapeColor(pShowObj->uiForeColor, pShowObj->uiBackColor, pShowObj->uiColorMapTable);
	} else {
		GxGfx_SetShapeColor(pPalette[pShowObj->uiForeColor], pPalette[pShowObj->uiBackColor], pShowObj->uiColorMapTable);
	}
	GxGfx_SetShapeLayout(pShowObj->uiLayout, pShowObj->uiAlignment);
	GxGfx_Ellipse(((DC **)ScreenObj)[DRAW_OSD], pShowObj->x1, pShowObj->y1, pShowObj->x2, pShowObj->y2);

}

void Ux_DrawShowLine(UIScreen ScreenObj, ITEM_LINE *pShowObj)
{
	PALETTE_ITEM *pPalette = Ux_GetPaletteTable(0);
	GxGfx_SetShapeStroke(pShowObj->uiLineStyle, pShowObj->uiFillStyle);
	if (pPalette == 0) {
		GxGfx_SetShapeColor(pShowObj->uiLineColor, pShowObj->uiLineColor, pShowObj->uiColorMapTable);
	} else {
		GxGfx_SetShapeColor(pPalette[pShowObj->uiLineColor], pPalette[pShowObj->uiLineColor], pShowObj->uiColorMapTable);
	}
	GxGfx_Line(((DC **)ScreenObj)[DRAW_OSD], pShowObj->x1, pShowObj->y1, pShowObj->x2, pShowObj->y2);
}
void Ux_DrawShowImage(UIScreen ScreenObj, ITEM_IMAGE *pShowObj)
{
	PALETTE_ITEM *pPalette = Ux_GetPaletteTable(0);
	if (pShowObj->Content != ICONID_NULL) {
		GxGfx_SetImageLayout(pShowObj->uiLayout, pShowObj->uiAlignment);
		if ((bUseGlobalKey) && (pShowObj->uiBltROP == ROP_KEY)) {
			GxGfx_SetImageStroke(ROP_KEY, gGlobalColorKey);
		} else {
			GxGfx_SetImageStroke(pShowObj->uiBltROP, pShowObj->uiParamROP);
		}
		if (pPalette == 0) {
			GxGfx_SetImageColor(IMAGEPALETTE_DEFAULT, Ux_GetColorMapTable((UINT32)pShowObj->uiColorMapTable));
		} else {
			GxGfx_SetImageColor(pPalette, Ux_GetColorMapTable((UINT32)pShowObj->uiColorMapTable));
		}

		GxGfx_ImageInRect(((DC **)ScreenObj)[DRAW_OSD], pShowObj->x1, pShowObj->y1, pShowObj->x2, pShowObj->y2, GxGfx_GetImageID((UINT16)pShowObj->Content));
	}
}

void Ux_DrawShowText(UIScreen ScreenObj, ITEM_TEXT *pShowObj)
{
	IRECT OrgPos = {0};
	IPOINT CurPoint = {0};
	PALETTE_ITEM *pPalette = Ux_GetPaletteTable(0);
	if (pPalette == 0) {
		GxGfx_SetTextColor(pShowObj->uiTextColor, pShowObj->uiShadowColor, pShowObj->uiLineColor);
	} else {
		GxGfx_SetTextColor(pPalette[pShowObj->uiTextColor], pPalette[pShowObj->uiShadowColor], pPalette[pShowObj->uiLineColor]);
	}

    GxGfx_SetTextStroke(Ux_GetFontTable((UINT32)pShowObj->uiFontTable), pShowObj->uiFontStyle, 0);

	GxGfx_SetTextLayout(pShowObj->uiLayout, pShowObj->uiAlignment, pShowObj->uiLineHeight, pShowObj->uiLetterSpace, pShowObj->uiIndentSpace);
	if (pShowObj->uiCLipping & LAYOUT_CLIPPING) {
		OrgPos = GxGfx_GetWindow(((DC **)ScreenObj)[DRAW_OSD]);
		CurPoint = GxGfx_GetOrigin(((DC **)ScreenObj)[DRAW_OSD]);
		GxGfx_SetWindow(((DC **)ScreenObj)[DRAW_OSD],
						CurPoint.x + pShowObj->x1, CurPoint.y + pShowObj->y1, CurPoint.x + pShowObj->x2, CurPoint.y + pShowObj->y2); //clipping text by text rect
		DBG_IND("clipping %d %d %d %d  \r\n", CurPoint.x + pShowObj->x1, CurPoint.y + pShowObj->y1, CurPoint.x + pShowObj->x2, CurPoint.y + pShowObj->y2);

	}
	if (pShowObj->Content & TEXT_POINTER) {
		pShowObj->Content  = (pShowObj->Content) & ~TEXT_POINTER;
		GxGfx_TextPrintInRect(((DC **)ScreenObj)[DRAW_OSD],  pShowObj->x1, pShowObj->y1, pShowObj->x2, pShowObj->y2, "%s", pShowObj->Content);
	} else {
		GxGfx_TextInRect(((DC **)ScreenObj)[DRAW_OSD],  pShowObj->x1, pShowObj->y1, pShowObj->x2, pShowObj->y2, GxGfx_GetStringID((UINT16)pShowObj->Content));
	}

	if (pShowObj->uiCLipping & LAYOUT_CLIPPING) {
		GxGfx_SetWindow(((DC **)ScreenObj)[DRAW_OSD],
						OrgPos.x, OrgPos.y, OrgPos.w - 1, OrgPos.h - 1); //clipping text by original pos
	}
}

INT32 Ux_DrawShowTable(UIScreen ScreenObj, ITEM_BASE **ShowTable)
{
	ITEM_BASE     *pShowObj = NULL;
	UINT32 i = 0;
	if (ShowTable) {
		pShowObj = ShowTable[0];
	}

	while (pShowObj != NULL) {
		Ux_DrawShowObj(ScreenObj, pShowObj);
		i++;
		pShowObj = ShowTable[i];
	}
	return NVTEVT_CONSUME;
}

static void Ux_DrawShowRectProperty(ITEM_RECTANGLE *pShowObj, UINT32 propertyIndex, UINT32 value)
{
	switch (propertyIndex) {
	case PRO_SHAPE_COLOR:
		pShowObj->uiBackColor = value;
		break;
	default:
		DBG_ERR("Wrong Property\n\r");
	}
}

static void Ux_DrawShowRoundRectProperty(ITEM_ROUNDRECT *pShowObj, UINT32 propertyIndex, UINT32 value)
{
	switch (propertyIndex) {
	case PRO_SHAPE_COLOR:
		pShowObj->uiBackColor = value;
		break;
	default:
		DBG_ERR("Wrong Property\n\r");
	}
}

static void Ux_DrawShowEllipseProperty(ITEM_ELLIPSE *pShowObj, UINT32 propertyIndex, UINT32 value)
{
	switch (propertyIndex) {
	case PRO_SHAPE_COLOR:
		pShowObj->uiBackColor = value;
		break;
	default:
		DBG_ERR("Wrong Property\n\r");
	}
}

static void Ux_DrawShowLineProperty(ITEM_LINE *pShowObj, UINT32 propertyIndex, UINT32 value)
{
	switch (propertyIndex) {
	case PRO_SHAPE_COLOR:
		pShowObj->uiLineColor = value;
		break;
	default:
		DBG_ERR("Wrong Property\n\r");

	}
}

static void Ux_DrawShowImageProperty(ITEM_IMAGE *pShowObj, UINT32 propertyIndex, UINT32 value)
{
	switch (propertyIndex) {
	case PRO_ICONID:
		pShowObj->Content = value;
		break;
	default:
		DBG_ERR("Wrong Property\n\r");

	}
}

static void Ux_DrawShowTextProperty(ITEM_TEXT *pShowObj, UINT32 propertyIndex, UINT32 value)
{
	switch (propertyIndex) {
	case PRO_STRID:
		pShowObj->Content = value;
		break;
	case PRO_STRCOLOR:
		pShowObj->uiTextColor = value;
		break;
	default:
		DBG_ERR("Wrong Property\n\r");
	}
}

void Ux_SetShowObjProperty(ITEM_BASE *pShowObj, SHOWOBJ_PROPERTY_TYPE propertyIndex, UINT32 value)
{
	switch ((pShowObj->ItemType & CMD_MASK)) {
	case CMD_Rectangle:
		Ux_DrawShowRectProperty((ITEM_RECTANGLE *)pShowObj, propertyIndex, value);
		break;
	case CMD_RoundRect:
		Ux_DrawShowRoundRectProperty((ITEM_ROUNDRECT *)pShowObj, propertyIndex, value);
		break;
	case CMD_Ellipse:
		Ux_DrawShowEllipseProperty((ITEM_ELLIPSE *)pShowObj, propertyIndex, value);
		break;
	case CMD_Line:
		Ux_DrawShowLineProperty((ITEM_LINE *)pShowObj, propertyIndex, value);
		break;
	case CMD_Image:
		Ux_DrawShowImageProperty((ITEM_IMAGE *)pShowObj, propertyIndex, value);
		break;
	case CMD_Text:
		Ux_DrawShowTextProperty((ITEM_TEXT *)pShowObj, propertyIndex, value);
		break;
	default:
		DBG_ERR("Wrong Type\n\r");
	}
}

void Ux_GetShowObjPos(ITEM_BASE *pShowObj, INT32 *x1, INT32 *y1, INT32 *x2, INT32 *y2)
{
	* x1 = pShowObj->x1;
	* y1 = pShowObj->y1;
	* x2 = pShowObj->x2;
	* y2 = pShowObj->y2;
}

void Ux_SetShowObjPos(ITEM_BASE *pShowObj, INT32 x1, INT32 y1, INT32 x2, INT32 y2)
{
	pShowObj->x1 = x1;
	pShowObj->y1 = y1;
	pShowObj->x2 = x2;
	pShowObj->y2 = y2;
}
//#NT#2008/09/10#lincy-end


