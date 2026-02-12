#ifndef GX_GRPH
#define GX_GRPH

extern UINT32 _Repeat4Bytes(UINT8 byte);
extern UINT32 _Repeat2Words(UINT16 word);
extern void _grph_open_dummy(void);
extern void _grph_close_dummy(void);
extern void _grph_setImage1(const DC *_pSrcDC, UINT8 iCn, const IRECT *pSrcRect);
extern void _grph_setImage2(DC *_pDestDC, UINT8 iCn, const IRECT *pDestRect);
extern void _grph_setImage3(const DC *_pSrcDC, UINT8 iCn, const IRECT *pSrcRect);
extern void grph_bindImageA(int image);
extern void grph_bindImageB(int image);
extern void grph_bindImageC(int image);
extern void _grph_setImage1_dummy(const DC *_pSrcDC, UINT8 iCn, const IRECT *pSrcRect);
extern void _grph_setImage2_dummy(DC *_pDestDC, UINT8 iCn, const IRECT *pDestRect);
extern RESULT _grph_chkImage1(void);
extern RESULT _grph_chkImage2(void);
extern RESULT _grph_chkImage3(void);
extern void rotation_bindImageSrc(void);
extern void rotation_bindImageDst(void);
extern void _ime_setImage1(const DC *_pSrcDC, int fmt, const IRECT *pSrcRect);
extern void _ime_setImage2(DC *_pDestDC, int fmt, const IRECT *pDestRect);
extern RESULT _ime_chkImage1(void);
extern RESULT _ime_chkImage2(void);
extern void ime_bindImageIN(int image, int Cn);
extern void ime_bindImageOUT(int image, int Cn);
extern ER _sw_setAOP(BOOL b16BitPrc, GRPH_DST_SEL dstSel, GRPH_AOP_MODE aopMode, UINT32 uiProperty);
extern RESULT _grph_do_rotation(UINT32 cmd,UINT32 format);


#endif //GX_GRPH

