#include "string.h"
#include "FileSysTsk.h"
#include "string.h"

#include "emu_h26x_common.h"

#include "emu_h26x_file.h"

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

void h26xFileOpen(H26XFile *pH26XFile, char *string)
{
    memset((void *)pH26XFile->FileName, 0x00, sizeof(pH26XFile->FileName));
    strcpy(pH26XFile->FileName,string);
    pH26XFile->StartOfs = 0;
}

void h26xFileSeek(H26XFile *pH26XFile, UINT32 Size, H26XF_SET set)
{
	if (set == H26XF_SET_SEEK)
	    pH26XFile->StartOfs = Size;
	else if (set == H26XF_SET_CUR)
		pH26XFile->StartOfs += Size;
	else
		DBG_ERR("H26XFileSeek Not support (%d)\r\n", set);
}

int h26xFileGetSize(H26XFile *pH26XFile, UINT32 *uiSize)
{
    FST_FILE    fileHdl;

    FileSys_WaitFinish();

    fileHdl = FileSys_OpenFile(pH26XFile->FileName,FST_OPEN_READ);

    if (fileHdl != NULL)
    {
        int Size;

        FileSys_SeekFile(fileHdl, 0, FST_SEEK_END);

        Size = FileSys_TellFile(fileHdl);

        if(Size > 0)
        {
            DBG_ERR("H26X Tell File %x ==>",(int)Size);
            Size -= pH26XFile->StartOfs;
            DBG_ERR("%x (ofs %x)\r\n",(int)Size,(int)pH26XFile->StartOfs);
            *uiSize = Size;

        }else{
            DBG_ERR("H26X Tell File Error %s!\r\n",(char*)pH26XFile->FileName);
            *uiSize = 0;
            return 1;
        }
    }

    FileSys_CloseFile(fileHdl);

    return 0;
}

int h26xFileRead(H26XFile *pH26XFile,UINT32 Size,UINT32 BufAddr)
{
    FST_FILE    fileHdl;

    if(Size == 0)
    {
        DBG_ERR("H26X Read File Size Error %s! (%d)\r\n",(char*)pH26XFile->FileName,(int)Size);
        return 1;
    }

    //DBG_ERR("FileName %s! (%d)\r\n",(char*)pH26XFile->FileName,(int)Size);

    FileSys_WaitFinish();

    fileHdl = FileSys_OpenFile(pH26XFile->FileName,FST_OPEN_READ);

    if (fileHdl != NULL)
    {
        int Status;
        int ReadSize = Size;
        FileSys_SeekFile(fileHdl, pH26XFile->StartOfs, FST_SEEK_SET);
        Status = FileSys_ReadFile(fileHdl, (UINT8*)BufAddr, (UINT32*)&ReadSize, 0, NULL);
        FileSys_CloseFile(fileHdl);
        pH26XFile->StartOfs = pH26XFile->StartOfs + ReadSize;

        if (Status != FST_STA_OK)
        {
            DBG_ERR("H26X Read File Error %s!\r\n",(char*)pH26XFile->FileName);
            return 1;
        }
        if (ReadSize != (int)Size)
        {
            DBG_ERR("H26X Read File Size Error %s! (%d %d)\r\n",(char*)pH26XFile->FileName,(int)ReadSize,(int)Size);
            return 1;
        }
    }
	else
	{
        DBG_ERR("H26X Read File Error %s!\r\n",(char*)pH26XFile->FileName);
        return 1;
    }

    //h26x_flushCache(BufAddr, Size);

    return 0;

}

int h26xFileReadFlush(H26XFile *pH26XFile,UINT32 Size,UINT32 BufAddr)
{
    FST_FILE    fileHdl;

    if(Size == 0)
    {
        DBG_ERR("H26X Read File Size Error %s! (%d)\r\n",(char*)pH26XFile->FileName,(int)Size);
        return 1;
    }

    //DBG_ERR("FileName %s! (%d)\r\n",(char*)pH26XFile->FileName,(int)Size);

    FileSys_WaitFinish();

    fileHdl = FileSys_OpenFile(pH26XFile->FileName,FST_OPEN_READ);

    if (fileHdl != NULL)
    {
        int Status;
        int ReadSize = Size;
        FileSys_SeekFile(fileHdl, pH26XFile->StartOfs, FST_SEEK_SET);
        Status = FileSys_ReadFile(fileHdl, (UINT8*)BufAddr, (UINT32*)&ReadSize, 0, NULL);
        FileSys_CloseFile(fileHdl);
        pH26XFile->StartOfs = pH26XFile->StartOfs + ReadSize;

        if (Status != FST_STA_OK)
        {
            DBG_ERR("H26X Read File Error %s!\r\n",(char*)pH26XFile->FileName);
            return 1;
        }
        if (ReadSize != (int)Size)
        {
            DBG_ERR("H26X Read File Size Error %s! (%d %d)\r\n",(char*)pH26XFile->FileName,(int)ReadSize,(int)Size);
            return 1;
        }
    }
	else
	{
        DBG_ERR("H26X Read File Error %s!\r\n",(char*)pH26XFile->FileName);
        return 1;
    }

    h26x_flushCache(BufAddr, SIZE_32X(Size));

    return 0;

}

int h26xFilePreRead(H26XFile *pH26XFile,UINT32 Size,UINT32 BufAddr)
{
    FST_FILE    fileHdl;

    FileSys_WaitFinish();

    fileHdl = FileSys_OpenFile(pH26XFile->FileName,FST_OPEN_READ);

    if (fileHdl != NULL)
    {
        int Status;
        int ReadSize = Size;
        FileSys_SeekFile(fileHdl, pH26XFile->StartOfs, FST_SEEK_SET);
        Status = FileSys_ReadFile(fileHdl, (UINT8*)BufAddr, (UINT32*)&ReadSize, 0, NULL);
        FileSys_CloseFile(fileHdl);

        if (Status != FST_STA_OK)
        {
            DBG_ERR("H26X Read File Error %s!\r\n",(char*)pH26XFile->FileName);
            return 1;
        }
        if (ReadSize != (int)Size)
        {
            DBG_ERR("H26X Read File Size Error %s! (%d %d)\r\n",(char*)pH26XFile->FileName,(int)ReadSize,(int)Size);
            return 1;
        }
    }
	else
	{
        DBG_ERR("H26X Read File Error %s!\r\n",(char*)pH26XFile->FileName);
        return 1;
    }

    return 0;
}

int h26xFileWrite(H26XFile *pH26XFile,UINT32 Size,UINT32 BufAddr)
{
    FST_FILE    fileHdl;

    fileHdl = FileSys_OpenFile(pH26XFile->FileName,FST_OPEN_ALWAYS|FST_OPEN_WRITE);

    if (fileHdl != NULL)
    {
        int Status;
        FileSys_SeekFile(fileHdl, pH26XFile->StartOfs, FST_SEEK_SET);
        Status = FileSys_WriteFile(fileHdl, (UINT8*)BufAddr, &Size, 0, NULL);
        FileSys_CloseFile(fileHdl);
        pH26XFile->StartOfs = pH26XFile->StartOfs + Size;
        if (Status != FST_STA_OK)
        {
            DBG_ERR("H26X Write File Error %s!\r\n",(char*)pH26XFile->FileName);
            return 1;
        }
    }
	else
    {
        DBG_ERR("H26X Write File Error %s!\r\n",(char*)pH26XFile->FileName);
        return 1;
    }
    return 0;
}

void h26xFileClose(H26XFile *pH26XFile)
{
	;
}
void h26xFileDebug(void)
{

}

void h26xFileDummyRead(void)
{
    H26XFile fp;
    char buf[32];
    //int ret;
    int i;
    //DBG_INFO("[%s][%d] %d\r\n", __FUNCTION__, __LINE__,sizeof(buf));

    for(i=0;i<50;i++)
    {
        h26xFileOpen( &fp, "A:\\LD98528A.bin");
        /*ret = */h26xFileRead( &fp, sizeof(buf), (UINT32) buf);
        //DBG_INFO("[%s][%d] ret = %d\r\n", __FUNCTION__, __LINE__,ret);
        h26xFileClose(&fp);
    }
}


void h26xFileDump(UINT32 addr, UINT32 size, char *file_name)
{
	H26XFile fp;

    DBG_INFO("[%s][%d] addr = 0x%08x, size = 0x%08x, file %s\r\n", __FUNCTION__, __LINE__,(int)addr,(int)size,file_name);

	h26xFileOpen(&fp, file_name);

	h26xFileWrite(&fp, size, addr);

	h26xFileClose(&fp);
}

//#endif //if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

