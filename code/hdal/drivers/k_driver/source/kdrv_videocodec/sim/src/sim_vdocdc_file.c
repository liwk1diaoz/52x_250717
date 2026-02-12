#if defined(__LINUX)
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "h26x.h"
#include "sim_vdocdc_file.h"

static FST_FILE FileSys_OpenFile(char *filename, UINT32 flag)
{
	int fd;
	int fs_flg = 0;

	if (flag & FST_OPEN_READ) {
		fs_flg |=  O_RDONLY;
	}
	if (flag & FST_OPEN_WRITE) {
		fs_flg |= O_WRONLY;
	}
	if (flag & FST_OPEN_ALWAYS) {
		fs_flg |= O_CREAT;
	}

	fd = vos_file_open(filename, fs_flg, 0);

	printk("file %s flg 0x%x flag 0x%x open result 0x%x\r\n",
				filename, fs_flg, flag, fd);

	if ((VOS_FILE)(-1) == fd)
		return 0;
	else
		return fd;

}

static INT32 FileSys_CloseFile(FST_FILE pFile)
{
	INT32 ret;

	ret = vos_file_close(pFile);

	printk("%s: close 0x%x, result %d\r\n", __func__, pFile, ret);

	return ret;
}

static INT32 FileSys_ReadFile(FST_FILE pFile, UINT8 *pBuf, UINT32 *pBufSize, UINT32 Flag, INT32 *CB, UINT32 StartOfs)
{
	INT32 ret;
	INT32 size = *pBufSize;

	ret = vos_file_read(pFile, pBuf, size);

	if (ret < 0) {
	} else {
		*pBufSize = ret;
		ret = 0;
	}

	return ret;
}

static INT32 FileSys_WriteFile(FST_FILE pFile, UINT8 *pBuf, UINT32 *pBufSize, UINT32 Flag, INT32 *CB, UINT32 StartOfs)
{
	INT32 ret;
	INT32 size = *pBufSize;

	printk("fd 0x%x write length %d, buf 0x%p\r\n",
			pFile, size, pBuf);

	ret = vos_file_write(pFile, pBuf, size);

	printk("file write length %d\r\n", (int)(ret));

	if (ret < 0) {
	} else {
		*pBufSize = ret;
		ret = 0;
	}

	return ret;
}


void h26xFileOpen(H26XFile *pH26XFile, char *string, UINT32 flag){
    memset((void *)pH26XFile->FileName, 0x00, sizeof(pH26XFile->FileName));
    strcpy(pH26XFile->FileName,string);
    //emuh26x_msg(("h26xFileOpen %s! 0x%08x\r\n",pH26XFile->FileName,pH26XFile));
    pH26XFile->StartOfs = 0;
	pH26XFile->fileHdl = FileSys_OpenFile(pH26XFile->FileName, flag);
}

int h26xFileRead(H26XFile *pH26XFile, UINT32 Size, UINT32 BufAddr){

    //emuh26x_msg(("h26xFileRead 0x%08x size %x, addr 0x%08x!\r\n",pH26XFile,Size,BufAddr));

    if (pH26XFile->fileHdl != 0)
    {
        int Status;
        int ReadSize = Size;
        Status = FileSys_ReadFile(pH26XFile->fileHdl, (UINT8*)BufAddr, (UINT32*)&ReadSize, 0, NULL, pH26XFile->StartOfs);

        pH26XFile->StartOfs = pH26XFile->StartOfs + ReadSize;

        if (Status != FST_STA_OK)
        {
            printk("H26X Read File Error %s!\r\n",pH26XFile->FileName);
            return 1;
        }
        if (ReadSize != (int)Size)
        {
            printk("H26X Read File Size Error %s! (%d %d)\r\n", pH26XFile->FileName, (int)(ReadSize), (int)(Size));
            return 1;
        }

    }
    else{

        printk("H26X Read File Error %s!\r\n",pH26XFile->FileName);
        return 1;
    }

	h26x_flushCache(BufAddr, Size);

    return 0;

}

void h26xFileSeek(H26XFile *pH26XFile, UINT32 Size)
{
    pH26XFile->StartOfs = Size;
}

int h26xFileWrite(H26XFile *pH26XFile, UINT32 Size, UINT32 BufAddr)
{
	h26x_flushCache(BufAddr, Size);

	if (pH26XFile->fileHdl != 0)
	{
        int Status;
        Status = FileSys_WriteFile(pH26XFile->fileHdl, (UINT8*)BufAddr, &Size, 0, NULL, pH26XFile->StartOfs);
        pH26XFile->StartOfs = pH26XFile->StartOfs + Size;

        if (Status != FST_STA_OK) {
            printk("H26X Write File Error %s!\r\n",pH26XFile->FileName);
            return 1;
        }
    }
	else{
        printk("H26X Write File Error %s!\r\n",pH26XFile->FileName);
        return 1;
    }


	return 0;

}


void h26xFileClose(H26XFile *pH26XFile)
{
	if (pH26XFile->fileHdl != 0)
		FileSys_CloseFile(pH26XFile->fileHdl);
}
#else
#include <string.h>
#include "kdrv_vdocdc_dbg.h"
#include "sim_vdocdc_file.h"

void h26xFileOpen(H26XFile *pH26XFile, char *string, UINT32 flag)
{
    memset((void *)pH26XFile->FileName, 0x00, sizeof(pH26XFile->FileName));
    strcpy(pH26XFile->FileName,string);
    pH26XFile->StartOfs = 0;
}

int h26xFileRead(H26XFile *pH26XFile,UINT32 Size,UINT32 BufAddr)
{
    FST_FILE    fileHdl;

    if(Size == 0)
    {
        DBG_ERR("H26X Read File Size Error %s! (%d)\r\n",pH26XFile->FileName,Size);
        return 1;
    }

    FileSys_WaitFinish();

    //emuh26x_msg(("h26xFileRead(%s) size(%x) addr(0x%08x)\r\n", pH26XFile->FileName, Size, BufAddr));
    fileHdl = FileSys_OpenFile(pH26XFile->FileName,FST_OPEN_READ);

    if (fileHdl != NULL) {
        int Status;
        int ReadSize = Size;
        FileSys_SeekFile(fileHdl, pH26XFile->StartOfs, FST_SEEK_SET);
        Status = FileSys_ReadFile(fileHdl, (UINT8*)BufAddr, (UINT32*)&ReadSize, 0, NULL);
        FileSys_CloseFile(fileHdl);
        pH26XFile->StartOfs = pH26XFile->StartOfs + ReadSize;

        if (Status != FST_STA_OK)
        {
            DBG_ERR("H26X Read File Error %s!\r\n",pH26XFile->FileName);
            return 1;
        }
        if (ReadSize != (int)Size)
        {
            DBG_ERR("H26X Read File Size Error %s! (%d %d)\r\n",pH26XFile->FileName,ReadSize,Size);
            return 1;
        }
    }
	else {
        DBG_ERR("H26X Read File Error %s!\r\n",pH26XFile->FileName);
        return 1;
    }

    return 0;

}

void h26xFileSeek(H26XFile *pH26XFile, UINT32 Size)
{
	pH26XFile->StartOfs = Size;
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
        //emuh26x_msg(("StartOfs %x += %x!\r\n",pH26XFile->StartOfs , Size);
        if (Status != FST_STA_OK)
        {
            DBG_ERR("H26X Write File Error %s!\r\n",pH26XFile->FileName);
            return 1;
        }
    }
        else
    {
        DBG_ERR("H26X Write File Error %s!\r\n",pH26XFile->FileName);
        return 1;
    }
    return 0;
}

void h26xFileClose(H26XFile *pH26XFile){}

#endif
