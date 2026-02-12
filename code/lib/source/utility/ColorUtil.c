#include "Utility/Color.h"

/*
//float-point formula: ITU.BT-601 from Color FAQ [6.4]
[CST]
Y'= 0.299*R' + 0.587*G' + 0.114*B'
Cb=-0.169*R' - 0.331*G' + 0.500*B'
Cr= 0.500*R' - 0.419*G' - 0.081*B'

//float-point formula: ITU.BT-709 from Color FAQ [6.5]
[CST]
Y'= 0.2215*R' + 0.7154*G' + 0.0721*B'
Cb=-0.1145*R' - 0.3855*G' + 0.5000*B'
Cr= 0.5016*R' - 0.4556*G' - 0.0459*B'
*/

/*
//fix-point formula: ITU.BT 601
[CST]
Y = 77*R/256 + 150*G/256 + 29*B/256
CB = -43*R/256 - 85*G/256 + 128*B/256
CR = 128*R/256 - 107*G/256 - 21*B/256
[ICST]
R = Y + 256*CR/256 + 103*CR/256
G = Y - 88*CB/256 - 183*CR/256
B = Y + 256*CB/256 + 198*CB/256
*/

/*
//simply fix-point formula: ITU.BT 601
[CST]
Y = G + cst_prom1 * (R-G)/256 + [cst_prom2* (B-G)] / 256
Cb = 128  + [cst_prom3* (G-R)] / 256 + (B-G) /2
Cr =  128 + [cst_prom0 * (G-B)] / 256  + (R-G) /2
[ICST]
R = Y + [cst_prom0*(Cr-128)] / 256  + (Cr-128)
G = Y + cst_prom1 * (128-Cb) / 256 + cst_prom2 * (128-Cr ) / 256
B = Y + [cst_prom3*(Cb-128)] / 256 + (Cb-128)
*/


#define CLAMP_TO_BYTE(v)    (((v) < 0)?0:((v>255)?255:(v)))

//BT 601 cst
INT32 cst_prom0 = 21;
INT32 cst_prom1 = 76;
INT32 cst_prom2 = 29;
INT32 cst_prom3 = 43;
//BT 709 cst
/*
INT32 cst_prom0 = 12;
INT32 cst_prom1 = 56;
INT32 cst_prom2 = 18;
INT32 cst_prom3 = 30;
*/
//[CST] (work!)
//#define RGB_GET_Y(r,g,b)    ( ((77*((INT32)r) + 150*((INT32)g) + 29*((INT32)b) + 128)>>8) )
//#define RGB_GET_U(r,g,b)    ( ((-43*((INT32)r) - 85*((INT32)g) + 128*((INT32)b) + 128)>>8)+128 )
//#define RGB_GET_V(r,g,b)    ( ((128*((INT32)r) - 107*((INT32)g) - 21*((INT32)b) + 128)>>8)+128 )
//[CST] (work!) with prom=(21,79,29,43)
//#define RGB_GET_Y(r,g,b)    (((INT32)g) + ((cst_prom1 * ((((INT32)r)-((INT32)g)) + (cst_prom2 * (((INT32)b)-((INT32)g)) >> 7))) >> 6))
//#define RGB_GET_U(r,g,b)    ((((((INT32)b)-((INT32)g)) - ((cst_prom3 * (((INT32)r)-((INT32)g))) >> 7)) >> 1) + 128)
//#define RGB_GET_V(r,g,b)    ((((((INT32)r)-((INT32)g)) - ((cst_prom0 * (((INT32)b)-((INT32)g))) >> 7)) >> 1) + 128)
#define RGB_GET_Y(r,g,b)    (((INT32)g) + ((cst_prom1 * (((INT32)r)-((INT32)g))) >> 8) + ((cst_prom2 * (((INT32)b)-((INT32)g))) >> 8) )
#define RGB_GET_U(r,g,b)    (128 + ((cst_prom3 * (((INT32)g)-((INT32)r))) >> 8) + ((((INT32)b)-((INT32)g)) >> 1) )
#define RGB_GET_V(r,g,b)    (128 + ((cst_prom0 * (((INT32)g)-((INT32)b))) >> 8) + ((((INT32)r)-((INT32)g)) >> 1) )
#define RGB_GET_YUV(r,g,b,y,u,v)    \
	{   \
		y = RGB_GET_Y(r,g,b);   y = CLAMP_TO_BYTE(y);   \
		u = RGB_GET_U(r,g,b);   u = CLAMP_TO_BYTE(u);   \
		v = RGB_GET_V(r,g,b);   v = CLAMP_TO_BYTE(v);   \
	}


//BT 601 icst (IME default)
INT32 icst_prom0 = 103;
INT32 icst_prom1 = 88;
INT32 icst_prom2 = 183;
INT32 icst_prom3 = 198;
// BT 709 icst
/*
INT32 icst_prom0 = 146;
INT32 icst_prom1 = 48;
INT32 icst_prom2 = 119;
INT32 icst_prom3 = 219;
*/
//[CST] ???
//#define YUV_GET_R(y,u,v)    ( (256*(INT32)y) + 0*((INT32)u) + 359*((INT32)v))>>8) )
//#define YUV_GET_G(y,u,v)    ( (256*(INT32)y) - 88*((INT32)u) + 183*((INT32)v))>>8) )
//#define YUV_GET_B(y,u,v)    ( (256*(INT32)y) + 454*((INT32)u) + 0*((INT32)v))>>8) )
//[ICST] ???
#define YUV_GET_R(y,u,v)    ( ((INT32)y) + ((INT32)(v)-128) + ((icst_prom0*((INT32)(v)-128))>>8) )
#define YUV_GET_G(y,u,v)    ( ((INT32)y) - ((icst_prom1*((INT32)(u)-128))>>8) - ((icst_prom2*((INT32)(v)-128))>>8) )
#define YUV_GET_B(y,u,v)    ( ((INT32)y) + ((INT32)(u)-128) + ((icst_prom3*((INT32)(u)-128))>>8) )
#define YUV_GET_RGB(y,u,v,r,g,b)    \
	{   \
		r = YUV_GET_R(y,u,v);   r = CLAMP_TO_BYTE(r);   \
		g = YUV_GET_G(y,u,v);   g = CLAMP_TO_BYTE(g);   \
		b = YUV_GET_B(y,u,v);   b = CLAMP_TO_BYTE(b);   \
	}


//[CST]
UINT32 COLOR_RGBD_2_YUVD(UINT32 rgbd)
{
	INT32 r, g, b;
	INT32 y, u, v;
	UINT32 yuvd;
	r = COLOR_RGBD_GET_R(rgbd);
	g = COLOR_RGBD_GET_G(rgbd);
	b = COLOR_RGBD_GET_B(rgbd);
	RGB_GET_YUV(r, g, b, y, u, v);
	yuvd = COLOR_MAKE_YUVD(y, u, v);
	return yuvd;
}

//[ICST]
UINT32 COLOR_YUVD_2_RGBD(UINT32 yuvd)
{
	INT32 r, g, b;
	INT32 y, u, v;
	UINT32 rgbd;
	y = COLOR_YUVD_GET_Y(yuvd);
	u = COLOR_YUVD_GET_U(yuvd);
	v = COLOR_YUVD_GET_V(yuvd);
	YUV_GET_RGB(y, u, v, r, g, b);
	rgbd = COLOR_MAKE_RGBD(r, g, b);
	return rgbd;
}

UINT32 COLOR_RGBD_2_RGB565(UINT32 rgbd, UINT32 a)
{
	INT32 r, g, b;
	//INT32 y,u,v;
	UINT32 rgb565;
	r = COLOR_RGBA_GET_R(rgbd);
	g = COLOR_RGBA_GET_G(rgbd);
	b = COLOR_RGBA_GET_B(rgbd);
	rgb565 = COLOR_MAKE_RGB565(r, g, b, a);
	return rgb565;
}

UINT32 COLOR_RGBA_2_RGB565(UINT32 rgba)
{
	INT32 r, g, b, a;
	//INT32 y,u,v;
	UINT32 rgb565;
	r = COLOR_RGBA_GET_R(rgba);
	g = COLOR_RGBA_GET_G(rgba);
	b = COLOR_RGBA_GET_B(rgba);
	a = COLOR_RGBA_GET_A(rgba);
	rgb565 = COLOR_MAKE_RGB565(r, g, b, a);
	return rgb565;
}

UINT32 COLOR_RGBD_2_ARGB1555(UINT32 rgbd, UINT32 a)
{
	INT32 r, g, b;
	//INT32 y,u,v;
	UINT32 rgb565;
	r = COLOR_RGBA_GET_R(rgbd);
	g = COLOR_RGBA_GET_G(rgbd);
	b = COLOR_RGBA_GET_B(rgbd);
	rgb565 = COLOR_MAKE_ARGB1555(r, g, b, a);
	return rgb565;
}

UINT32 COLOR_RGBA_2_ARGB1555(UINT32 rgba)
{
	INT32 r, g, b, a;
	//INT32 y,u,v;
	UINT32 rgb565;
	r = COLOR_RGBA_GET_R(rgba);
	g = COLOR_RGBA_GET_G(rgba);
	b = COLOR_RGBA_GET_B(rgba);
	a = COLOR_RGBA_GET_A(rgba);
	rgb565 = COLOR_MAKE_ARGB1555(r, g, b, a);
	return rgb565;
}

UINT32 COLOR_RGBD_2_ARGB4444(UINT32 rgbd, UINT32 a)
{
	INT32 r, g, b;
	//INT32 y,u,v;
	UINT32 rgb565;
	r = COLOR_RGBA_GET_R(rgbd);
	g = COLOR_RGBA_GET_G(rgbd);
	b = COLOR_RGBA_GET_B(rgbd);
	rgb565 = COLOR_MAKE_ARGB4444(r, g, b, a);
	return rgb565;
}

UINT32 COLOR_RGBA_2_ARGB4444(UINT32 rgba)
{
	INT32 r, g, b, a;
	//INT32 y,u,v;
	UINT32 rgb565;
	r = COLOR_RGBA_GET_R(rgba);
	g = COLOR_RGBA_GET_G(rgba);
	b = COLOR_RGBA_GET_B(rgba);
	a = COLOR_RGBA_GET_A(rgba);
	rgb565 = COLOR_MAKE_ARGB4444(r, g, b, a);
	return rgb565;
}

