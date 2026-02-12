#ifndef _EMU_H26X_FILE_H_
#define _EMU_H26X_FILE_H_


#include "kwrap/type.h"
//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

typedef enum{
	H26XF_SET_SEEK = 0,
	H26XF_SET_CUR,
	H26XF_SET_NO_USED
}H26XF_SET;

typedef struct _H26XFile_{
    char   FileName[256];
    UINT32 StartOfs;
}H26XFile;

void h26xFileOpen(H26XFile *pH26XFile, char *string);
void h26xFileSeek(H26XFile *pH26XFile, UINT32 Size, H26XF_SET set);
int  h26xFileGetSize(H26XFile *pH26XFile, UINT32 *uiSize);
int  h26xFileRead(H26XFile *pH26XFile,UINT32 Size,UINT32 BufAddr);
int  h26xFilePreRead(H26XFile *pH26XFile,UINT32 Size,UINT32 BufAddr);
int  h26xFileWrite(H26XFile *pH26XFile,UINT32 Size,UINT32 BufAddr);
void h26xFileClose(H26XFile *pH26XFile);
void h26xFileDebug(void);
int h26xFileReadFlush(H26XFile *pH26XFile,UINT32 Size,UINT32 BufAddr);
void h26xFileDummyRead(void);

//#endif // (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#endif
