/**
* @file fbindex.c
*
*/

/*********************
*      INCLUDES
*********************/
#include "fbdev.h"
#include "hd_gfx.h"
#include "hd_common.h"
#include "Utility/Color.h"

extern const uint32_t gDemoKit_Palette_Palette[256];

lv_color8_t  gDemoKit_Palette_RGB332[256];
uint32_t gDemoKit_Palette_BGRA8888[256];

#define icst_prom0  103
#define icst_prom1  88
#define icst_prom2  183
#define icst_prom3  198
typedef UINT8   CVALUE; ///< single color value

#define COLOR_YUVA_GET_V(c)         (CVALUE)(((c) & 0x00ff0000) >> 16) ///< get V channel value of YUVA color
#define COLOR_YUVA_GET_U(c)         (CVALUE)(((c) & 0x0000ff00) >> 8) ///< get U channel value of YUVA color
#define COLOR_YUVA_GET_Y(c)         (CVALUE)(((c) & 0x000000ff) >> 0) ///< get Y channel value of YUVA color

#define YUV_GET_R(y,u,v)    ( ((INT32)y) + ((INT32)(v)-128) + ((icst_prom0*((INT32)(v)-128))>>8) )
#define YUV_GET_G(y,u,v)    ( ((INT32)y) - ((icst_prom1*((INT32)(u)-128))>>8) - ((icst_prom2*((INT32)(v)-128))>>8) )
#define YUV_GET_B(y,u,v)    ( ((INT32)y) + ((INT32)(u)-128) + ((icst_prom3*((INT32)(u)-128))>>8) )
#define CLAMP_TO_BYTE(v)    (((v) < 0)?0:((v>255)?255:(v)))
#define YUV_GET_RGB(y,u,v,r,g,b)    \
	{   \
		r = YUV_GET_R(y,u,v);   r = CLAMP_TO_BYTE(r);   \
		g = YUV_GET_G(y,u,v);   g = CLAMP_TO_BYTE(g);   \
		b = YUV_GET_B(y,u,v);   b = CLAMP_TO_BYTE(b);   \
	}


#define COLOR_MAKE_BGRA(r,g,b,a)    \
	(((uint32_t)(b)&0x000000ff)|\
	 (((uint32_t)(g)&0x000000ff)<<8)|\
	 (((uint32_t)(r)&0x000000ff)<<16)|\
	 (((uint32_t)(a)&0x000000ff)<<24)) ///< make RGBA color by R,G,B,A channel value


UINT32 COLOR_YUV_2_BGRA8888(UINT32 yuv, UINT32 a)
{
	INT32 r, g, b;
	UINT32 argb888;
	INT32 y, u, v;

	v = COLOR_YUVA_GET_V(yuv);
	u = COLOR_YUVA_GET_U(yuv);
	y = COLOR_YUVA_GET_Y(yuv);

	YUV_GET_RGB(y, u, v, r, g, b);
	argb888 = COLOR_MAKE_BGRA(r, g, b, a);
	return argb888;
}


static inline lv_color8_t fb_color_to8(lv_color32_t color)
{
    lv_color8_t ret;
    LV_COLOR_SET_R8(ret, LV_COLOR_GET_R(color) >> 5); /* 8 - 3  = 5*/
    LV_COLOR_SET_G8(ret, LV_COLOR_GET_G(color) >> 5); /* 8 - 3  = 5*/
    LV_COLOR_SET_B8(ret, LV_COLOR_GET_B(color) >> 6); /* 8 - 2  = 6*/
    //printf("%x %x \r\n",ret.full,color.full);
    return ret;
}
#if 0
void PALETTE_PREPARE(void)
{
	UINT32 i = 0;

	for (i = 0; i < 256; i++) {
        if(i==0) {
            gDemoKit_Palette_BGRA8888[i] = COLOR_YUV_2_BGRA8888(gDemoKit_Palette_Palette[i], 0x00);
        } else {
		    gDemoKit_Palette_BGRA8888[i] = COLOR_YUV_2_BGRA8888(gDemoKit_Palette_Palette[i], 0xff);
        }

        gDemoKit_Palette_RGB332[i] = fb_color_to8((lv_color32_t)gDemoKit_Palette_BGRA8888[i]);
    }
}
#endif
UINT8 color8_toI8(lv_color8_t color)
{
    UINT8 i=0;
    printf("color %x \r\n",color.full);
    for(i=0;i<256;i++) {
        if((color.full==0xff)||(color.full==0x24)) {
            printf(" %x index %d \r\n",gDemoKit_Palette_RGB332[i].full,i);
            }
        if(color.full==gDemoKit_Palette_RGB332[i].full){
            //printf("find %x index %d \r\n",color.full,i);
            return i;
        }
        break;
    }
    if(color.full==0xb){
        return 0xe;
    }else {
        printf("%x not in palette table \r\n",color.full);
        return 0;
    }
}


UINT32 COLOR_YUV_2_RGB(UINT32 yuv, UINT32 a)
{
	INT32 r, g, b;
	UINT32 argb888;
	INT32 y, u, v;

	v = COLOR_YUVA_GET_V(yuv);
	u = COLOR_YUVA_GET_U(yuv);
	y = COLOR_YUVA_GET_Y(yuv);

	YUV_GET_RGB(y, u, v, r, g, b);
	argb888 = COLOR_MAKE_RGBA(r, g, b, a);
	return argb888;
}


void PALETTE_PREPARE_DATA(void)
{
	UINT32 i = 0;
    uint32_t gDemoKit_Palette_Index8[256];

	for (i = 0; i < 256; i++) {
		gDemoKit_Palette_Index8[i] = COLOR_YUV_2_RGB(gDemoKit_Palette_Palette[i], 0xff);
        //DBG_DUMP("%d %x \r\n",i,gDemoKit_Palette_Index8[i]);
	}
	gDemoKit_Palette_Index8[0] = COLOR_YUV_2_RGB(gDemoKit_Palette_Palette[0], 0);

	//copy convert palette to org palette table
    memcpy((void *)gDemoKit_Palette_Palette,(void *)gDemoKit_Palette_Index8,sizeof(UINT32)*256);
}
