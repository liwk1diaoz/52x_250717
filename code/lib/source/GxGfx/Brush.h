#ifndef _BRUSH_H_
#define _BRUSH_H_

#define VAR_STATE(n)                (n)
#define VAR_STATE_NUM               0x40

#define VARTYPE_STATE   0
#define VARTYPE_STACK   1
#define VARARRAY_SIZE   VAR_STATE_NUM

extern UINT8 _gVAR_index;
extern BVALUE _gVAR_array[2][VARARRAY_SIZE];

#define VAR_SETTYPE(t)      (_gVAR_index) = (t)
#define VAR_GETTYPE()       (_gVAR_index)
#define VAR_SETVALUE(n, v)  _gVAR_array[_gVAR_index][(n)] = (BVALUE)(v)
#define VAR_GETVALUE(n)     (_gVAR_array[_gVAR_index][(n)])

//var
void   _VAR_Init(void);

extern BRUSH _gBRUSH;

//brush
void   _BR_Init(void);

_INLINE BRUSH *_BR(void)
{
	return &_gBRUSH;
}

#endif //_BRUSH_H_
