#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PStore.h"
#include "ps_int.h"
#include <kwrap/examsys.h>

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          PSTcmd
#define __DBGLVL__          1 // 0=OFF, 1=ERROR, 2=TRACE
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass

#include "kwrap/debug.h"
///////////////////////////////////////////////////////////////////////////////

#define MAX_BLK_PER_SEC    (512)
#define MAX_SEC_NUM        (64)
#define SEC_NAME            "TEST"
static BOOL Cmd_PStore_format(UINT32 blkpersec, UINT32 maxsec)
{
    PSFMT FmtStruct = {MAX_BLK_PER_SEC,MAX_SEC_NUM};
	
    if((blkpersec)&&(maxsec))
    {
        FmtStruct.uiMaxBlkPerSec = blkpersec;
        FmtStruct.uiMaxSec = maxsec;
        PStore_Format(&FmtStruct);
    }
    else
    {
        PStore_Format(&FmtStruct);
    }

    DBG_DUMP("format %d blkpersec,maxsec %d \r\n",FmtStruct.uiMaxBlkPerSec,FmtStruct.uiMaxSec);

    return TRUE;
}

static BOOL Cmd_PStore_showinfo(void)
{
    PStore_DumpPStoreInfo();
    return TRUE;
}

static BOOL Cmd_PStore_dump(void)
{
    // dump size should over than max section size
    UINT32 BufSize = 0x1800000;
    UINT8 *pBuf = (UINT8*)malloc(BufSize);
    //open file system for save file
    //Dep_Fs_Init(TRUE,TRUE);
    if(pBuf)
    {
        PStore_Dump(pBuf,BufSize);
    }
    else
    {
        DBG_ERR("no buffer\r\n");
    }
    free(pBuf);
    return TRUE;
}

static BOOL Cmd_PStore_flash_life_evaluate(UINT32 cmd)
{
    PSTORE_SECTION_HANDLE* pSection;
    UINT32     i, ret;
    UINT32 BufSize = 4096;
    UINT8 *pBuf = NULL;
    char *name[3] = {"AAA", "BBB", "CCC"};
    static int data = 0x12;

    if(cmd == 1){
        for(i=0;i<3;i++){
            if ((pSection = PStore_OpenSection(name[i], PS_RDWR|PS_CREATE)) != (PSTORE_SECTION_HANDLE*)E_PS_SECHDLER)
                PStore_CloseSection(pSection);
            else{
                DBG_ERR("can't create %s\r\n", name[i]);
                ret = FALSE;
                goto clean;
            }
        }
        DBG_DUMP("done\r\n");
        return TRUE;
    }
    else if(cmd == 3){
        for(i=0;i<3;i++)
            if(PStore_DeleteSection(name[i]) != E_PS_OK)
                DBG_ERR("PStore_DeleteSection(%s) fail\r\n", name[i]);
        ret = TRUE;
clean:
        if(ret == TRUE)
            DBG_DUMP("done\r\n");
        return TRUE;
    }
    else if(cmd != 2){
        DBG_ERR("only argument 1 2 3 are valid. input is %d\r\n", cmd);
        return TRUE;
    }

    pBuf = (UINT8*)malloc(BufSize);
    if(!pBuf){
        DBG_ERR("Fail to allocate %d bytes\r\n", BufSize);
        return FALSE;
    }
    else
        for(i = 0 ; i < BufSize ; ++i)
            pBuf[i] = data;

    data++;

    for(i=0;i<3;i++)
    {
        if ((pSection = PStore_OpenSection(name[i], PS_RDWR)) != (PSTORE_SECTION_HANDLE*)E_PS_SECHDLER)
        {
            ret = PStore_WriteSection((UINT8 *)pBuf, 0, BufSize, pSection);
            PStore_CloseSection(pSection);
            if(ret){
                DBG_ERR("fail to write %s\r\n", name[i]);
                goto out;
            }
        }
        else{
            DBG_ERR("fail to open %s\r\n", name[i]);
            goto out;
        }
    }
    DBG_DUMP("done\r\n");

out:
    free(pBuf);

    return TRUE;
}

static BOOL Cmd_PStore_autotest(UINT32 SecNum, UINT32 BufSize)
{
    PSTORE_SECTION_HANDLE* pSection;
    UINT32                 i, j, ret;
    UINT8                  *pBuf = NULL;
    char                   *name[3] = {"AAA", "BBB", "CCC"};

    if(SecNum > 3){
        DBG_ERR("autotest can't handle %d sections. 3 is max\r\n", SecNum);
        return FALSE;
    }

    for(i=0 ; i < SecNum ; i++){
        if ((pSection = PStore_OpenSection(name[i], PS_RDWR|PS_CREATE)) != (PSTORE_SECTION_HANDLE*)E_PS_SECHDLER)
            PStore_CloseSection(pSection);
        else{
            DBG_ERR("can't create %s\r\n", name[i]);
            goto clean;
        }
    }
    DBG_DUMP("create_done\r\n");

    pBuf = (UINT8*)malloc(BufSize);
    if(!pBuf){
        DBG_ERR("Fail to allocate %d bytes\r\n", BufSize);
        goto clean;
    }

    for(i=0 ; i < SecNum ; i++)
    {
        for(j = 0 ; j < BufSize ; j++)
            pBuf[j] = (i + 1);

        if ((pSection = PStore_OpenSection(name[i], PS_RDWR)) != (PSTORE_SECTION_HANDLE*)E_PS_SECHDLER)
        {
            ret = PStore_WriteSection((UINT8 *)pBuf, 0, BufSize, pSection);
            PStore_CloseSection(pSection);
            if(ret){
                DBG_ERR("fail to write %s\r\n", name[i]);
                goto clean;
            }
        }
        else{
            DBG_ERR("fail to open %s\r\n", name[i]);
            goto clean;
        }
    }
    DBG_DUMP("write_done\r\n");

    for(i=0 ; i < SecNum ; i++)
    {
        if ((pSection = PStore_OpenSection(name[i], PS_RDWR)) != (PSTORE_SECTION_HANDLE*)E_PS_SECHDLER)
        {
            memset(pBuf, 0, BufSize);
            ret = PStore_ReadSection((UINT8 *)pBuf, 0, BufSize, pSection);
            PStore_CloseSection(pSection);
            if(ret){
                DBG_ERR("fail to read %s\r\n", name[i]);
                goto clean;
            }
        }
        else{
            DBG_ERR("fail to open %s\r\n", name[i]);
            goto clean;
        }

        for(j = 0 ; j < BufSize ; j++){
            if(pBuf[j] != (i + 1)){
                DBG_ERR("section[%d]'s %dth bite[%d] != %d\r\n", i, j, pBuf[j], (i + 1));
                goto clean;
            }
        }
    }
    DBG_DUMP("read_done\r\n");

clean:

    if(pBuf)
        free(pBuf);

    for(i=0 ; i < SecNum ; i++)
        if(PStore_DeleteSection(name[i]) != E_PS_OK)
            DBG_ERR("PStore_DeleteSection(%s) fail\r\n", name[i]);

    return TRUE;
}

static BOOL Cmd_PStore_save(UINT32 SecNum, char *data)
{
    PSTORE_SECTION_HANDLE* pSection;
    UINT32                 ret;
    char                   *name[3] = {"AAA", "BBB", "CCC"};

    if(SecNum > 2){
        DBG_ERR("autotest can't handle %d sections. 3 is max\r\n", SecNum);
        return FALSE;
    }

	if ((pSection = PStore_OpenSection(name[SecNum], PS_RDWR|PS_CREATE)) == (PSTORE_SECTION_HANDLE*)E_PS_SECHDLER){
		DBG_ERR("can't create %s\r\n", name[SecNum]);
		goto clean;
	}
    DBG_DUMP("create_done\r\n");

	ret = PStore_WriteSection((UINT8*)data, 0, strlen(data)+1, pSection);
	PStore_CloseSection(pSection);
	if(ret){
		DBG_ERR("fail to write %s\r\n", name[SecNum]);
		goto clean;
	}
    DBG_DUMP("write_done\r\n");

clean:

    return TRUE;
}

static BOOL Cmd_PStore_read(UINT32 SecNum, UINT32 BufSize)
{
    PSTORE_SECTION_HANDLE* pSection;
    UINT32                 ret;
    char                   *name[3] = {"AAA", "BBB", "CCC"};
    UINT8                  *pBuf = NULL;

    if(SecNum > 2){
        DBG_ERR("autotest can't handle %d sections. 3 is max\r\n", SecNum);
        return FALSE;
    }

    pBuf = (UINT8*)malloc(BufSize);
    if(!pBuf){
        DBG_ERR("Fail to allocate %d bytes\r\n", BufSize);
        goto clean;
    }
	memset(pBuf, 0, BufSize);

	if ((pSection = PStore_OpenSection(name[SecNum], PS_RDWR|PS_CREATE)) == (PSTORE_SECTION_HANDLE*)E_PS_SECHDLER){
		DBG_ERR("can't create %s\r\n", name[SecNum]);
		goto clean;
	}
    DBG_DUMP("create_done\r\n");

	ret = PStore_ReadSection(pBuf, 0, BufSize, pSection);
	PStore_CloseSection(pSection);
	if(ret){
		DBG_ERR("fail to read %s\r\n", name[SecNum]);
		goto clean;
	}
    DBG_DUMP("read=%s\r\n", pBuf);

clean:

    if(pBuf)
        free(pBuf);

    return TRUE;
}

EXAMFUNC_ENTRY(pstore, argc, argv)
{
	if(argc < 2){
		printf("usage :\n");
		printf("format blkpersec maxsec\n");
		printf("showinfo\n");
		printf("dump\n");
		printf("flash_life command\n");
		printf("autotest SecNum BufSize\n");
		printf("save SecNum data\n");
		printf("read SecNum BufSize\n");
		return -1;
	}
	
	if(!strcmp(argv[1], "format"))
		return Cmd_PStore_format(atoi(argv[2]), atoi(argv[3]));
	else if(!strcmp(argv[1], "showinfo"))
		return Cmd_PStore_showinfo();
	else if(!strcmp(argv[1], "dump"))
		return Cmd_PStore_dump();
	else if(!strcmp(argv[1], "flash_life"))
		return Cmd_PStore_flash_life_evaluate(atoi(argv[2]));
	else if(!strcmp(argv[1], "autotest"))
		return Cmd_PStore_autotest(atoi(argv[2]), atoi(argv[3]));
	else if(!strcmp(argv[1], "save"))
		return Cmd_PStore_save(atoi(argv[2]), argv[3]);
	else if(!strcmp(argv[1], "read"))
		return Cmd_PStore_read(atoi(argv[2]), atoi(argv[3]));
	else{
		printf("usage :\n");
		printf("format blkpersec maxsec\n");
		printf("showinfo\n");
		printf("dump\n");
		printf("flash_life command\n");
		printf("autotest SecNum BufSize\n");
		printf("save SecNum data\n");
		printf("read SecNum BufSize\n");
		return -1;
	}

	return 0;
}