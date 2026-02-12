#include <stdio.h>
#include <string.h>
#include "kwrap/nvt_type.h"
#include "kwrap/type.h"
#include "kwrap/sxcmd.h"
#include "kwrap/cmdsys.h"
#include <kwrap/stdio.h>
#include "PrjInc.h"
#include "hd_common.h"
#ifdef __FREERTOS
#include "FreeRTOS.h"
#include <kflow_common/nvtmpp.h>
#else
#include <sys/wait.h>
#endif
#include "UIApp/AppDisp_PipView.h"
#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          SysMainCmd
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// temp memory for user cmd
///////////////////////////////////////////////////////////////////////////////
#define MAX_BUF_NUM        1
//#define TEMPBUF_GET_MAXSIZE 0xffffffff

static UINT32 temp_pa;

UINT32 SysMain_GetTempBuffer(UINT32 uiSize)
{
    CHAR                 name[]= "UsrTmp";
	void                 *va;
	HD_RESULT            ret;
    HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;

    if(temp_pa) {
    	DBG_ERR("need release buf\r\n");
        return 0;
    }
	ret = hd_common_mem_alloc(name, &temp_pa, (void **)&va, uiSize, ddr_id);
	if (ret != HD_OK) {
		DBG_ERR("err:alloc size 0x%x, ddr %d\r\n", (unsigned int)(uiSize), (int)(ddr_id));
		return 0;
	}

	DBG_IND("temp_pa = 0x%x, va = 0x%x\r\n", (unsigned int)(temp_pa), (unsigned int)(va));

	return (UINT32)va;
}

UINT32 SysMain_RelTempBuffer(UINT32 addr)
{
	HD_RESULT            ret;

	DBG_IND("temp_pa = 0x%x, va = 0x%x\r\n", (unsigned int)(temp_pa), (unsigned int)(addr));

    if(!temp_pa||!addr){
        DBG_ERR("not allo\r\n");
        return HD_ERR_UNINIT;
    }
	ret = hd_common_mem_free(temp_pa, (void *)addr);
	if (ret != HD_OK) {
		DBG_ERR("err:free temp_pa = 0x%x, va = 0x%x\r\n", (unsigned int)(addr), (unsigned int)(addr));
		return HD_ERR_NG;
	}else {
        temp_pa =0;
	}
    return HD_OK;
}

/*Please use this to replace the "system" instruction of linux*/
int SysMain_system(const char* pCommand)
{
    pid_t pid;
    int status;
    int i = 0;

    if(pCommand == NULL)
    {
        return (1);
    }

    if((pid = fork())<0)
    {
        status = -1;
    }
    else if(pid == 0)
    {
        /* close all descriptors in child sysconf(_SC_OPEN_MAX) */
        for (i = 3; i < sysconf(_SC_OPEN_MAX); i++)
        {
            close(i);
        }

        execl("/bin/sh", "sh", "-c", pCommand, (char *)0);
        _exit(127);
    }
    else
    {
        /*当进程中SIGCHLD做SIG_IGN操作时，获取不到status返回值，XOS_System无法正常工作*/
        while(waitpid(pid, &status, 0) < 0)
        {
            if(errno != EINTR)
            {
                status = -1;
                break;
            }
        }
    }

    return status;
}

static BOOL cmd_sys_mem(unsigned char argc, char **argv)
{
	nvt_cmdsys_runcmd("ker dumpmem");
#ifdef __FREERTOS
	vPortDumpHeap();
	nvt_cmdsys_runcmd("libc heap");
	nvtmpp_dump_mem_range(vk_printk);
#endif
	return TRUE;
}

static BOOL Cmd_user_pip(unsigned char argc, char **argv)
{
	UINT32 pip_style = 0;

	sscanf_s(argv[0], "%d", &pip_style);

	PipView_SetStyle(pip_style);

	return TRUE;
}

static BOOL Cmd_user_DumpFastBoot(unsigned char argc, char **argv)
{
	DBG_DUMP("dumping fastboot dtsi to sd card\r\n");
	ImageApp_MovieMulti_ImgCapLink_Pending(TRUE);

	system("mkdir -p /tmp/fastboot");
	system("echo dump_fastboot  /tmp/fastboot/nvt-na51055-fastboot-sie.dtsi > /proc/nvt_ctl_sie/cmd");
	system("echo dump_fastboot /tmp/fastboot/nvt-na51055-fastboot-ipp.dtsi > /proc/kflow_ctl_ipp/cmd");
	system("echo vdoenc dump_fastboot /tmp/fastboot/nvt-na51055-fastboot-venc.dtsi > /proc/hdal/venc/cmd");
	system("echo dump_fastboot /tmp/fastboot/nvt-na51055-fastboot-acap.dtsi > /proc/hdal/acap/cmd");
	// to solve 'Command length is too long!'
	system("echo nvtmpp fastboot_mem /tmp/hdal-mem.dtsi > /proc/hdal/comm/cmd");
	system("mv /tmp/hdal-mem.dtsi /tmp/fastboot/nvt-na51055-fastboot-hdal-mem.dtsi");
	system("cp -r /tmp/fastboot /mnt/sd");
	system("sync");
	ImageApp_MovieMulti_ImgCapLink_Pending(FALSE);
	DBG_DUMP("finish. \r\n");
	return TRUE;
}
static BOOL Cmd_user_DumpFastBoot_fdtapp(unsigned char argc, char **argv)
{
	DBG_DUMP("dumping fastboot dtsi to fdt.app\r\n");
	ImageApp_MovieMulti_ImgCapLink_Pending(TRUE);

	system("echo save > /proc/fastboot/cmd");
	//system("echo dump /mnt/sd/app.dtb > /proc/fastboot/cmd");
	ImageApp_MovieMulti_ImgCapLink_Pending(FALSE);


	DBG_DUMP("finish. \r\n");
	return TRUE;
}
#if defined(_UI_STYLE_LVGL_)

typedef struct _KeyTab {
  CHAR *name;
  NVTEVT evt;
} KEYTAB;

static KEYTAB keyTest[] = {
	{"0.LV_USER_KEY_ZOOMIN",NVTEVT_KEY_ZOOMIN},
	{"1.LV_USER_KEY_ZOOMOUT",NVTEVT_KEY_ZOOMOUT},
	{"2.LV_USER_KEY_MENU",NVTEVT_KEY_MENU},
	{"3.LV_USER_KEY_SELECT",NVTEVT_KEY_SELECT},
	{"4.LV_USER_KEY_MODE",NVTEVT_KEY_MODE},
	{"5.LV_KEY_ENTER",NVTEVT_KEY_ENTER},
	{"6.NVTEVT_CB_MOVIE_FULL",NVTEVT_CB_MOVIE_FULL},
	{"7.NVTEVT_CB_MOVIE_REC_FINISH",NVTEVT_CB_MOVIE_REC_FINISH},
	{"8.NVTEVT_CB_MOVIE_WR_ERROR",NVTEVT_CB_MOVIE_WR_ERROR},
	{"9.NVTEVT_CB_MOVIE_SLOW",NVTEVT_CB_MOVIE_SLOW},
	{"10.NVTEVT_CB_EMR_COMPLETED",NVTEVT_CB_EMR_COMPLETED},
	{"11.NVTEVT_CB_OZOOMSTEPCHG",NVTEVT_CB_OZOOMSTEPCHG},
	{"12.NVTEVT_CB_DZOOMSTEPCHG",NVTEVT_CB_DZOOMSTEPCHG},
	{"13.NVTEVT_BATTERY",NVTEVT_BATTERY},
	{"14.NVTEVT_STORAGE_CHANGE",NVTEVT_STORAGE_CHANGE},
	{"15.NVTEVT_BACKGROUND_DONE",NVTEVT_BACKGROUND_DONE},
};
static BOOL Cmd_user_EventTest(unsigned char argc, char **argv)
{
	UINT32 event = 0;

	sscanf_s(argv[0], "%d", &event);
	if (event>=(sizeof(keyTest)/sizeof(KEYTAB)))
	{
		DBG_ERR("event_no bigger than keyTest table size\r\n");
		return FALSE;
	}
	DBG_ERR("post event %s\r\n",keyTest[event].name);
	Ux_PostEvent((NVTEVT)keyTest[event].evt, 1,NVTEVT_KEY_PRESS);
	Ux_PostEvent((NVTEVT)keyTest[event].evt, 1,NVTEVT_KEY_RELEASE);

	return TRUE;
}

#endif

                              
#if (CURL_FUNC == ENABLE)
#include "CurlNvt/CurlNvtAPI.h"

static BOOL Cmd_user_CurlTest(unsigned char argc, char **argv)
{
   CURLNVT_CMD curlCmd={0};
   CHAR curlStr[]="curl -q -v --insecure --output /mnt/sd/curl_dest/002.jpg --url http://192.168.1.254/image056.jpg";

        curlCmd.isWaitFinish = TRUE;
        // download file
        curlCmd.strCmd = curlStr;
        curlCmd.cmdFinishCB = NULL;
        CurlNvt_Cmd(&curlCmd);

	return TRUE;
}
#endif

//#NT#20220712#Philex Lin - begin
// check GDC/ETH power status
#include <sys/mman.h>
static int fd = 0;

int init_mem_dev(void)
{
	int fd = 0;
	fd = open("/dev/mem", O_RDWR | O_SYNC);
	return fd;
}

void uninit_mem_dev(int fd)
{
	close(fd);
	return;
}

void* mem_mmap2(int fd, unsigned int mapped_size, off_t phy_addr)
{
	void *map_base = NULL;
	unsigned page_size = 0;

	page_size = getpagesize();
	map_base = mmap(NULL,
			mapped_size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd,
			phy_addr & ~(off_t)(page_size - 1));

	if (map_base == MAP_FAILED)
		return NULL;

	return map_base;
}

int mem_munmap2(void* map_base, unsigned int mapped_size)
{
	if (munmap(map_base, mapped_size) == -1)
		return -1;

	return 0;
}

static UINT32 Cmd_user_KernelAddrDump(UINT32 phyaddr)
{
	UINT32 virtual_addr;
	void*  map_addr;
	UINT32 page_align_addr, map_size, map_offset;
	UINT32 uiRet;

	phyaddr &= 0xFFFFFFFC;
	page_align_addr = phyaddr & ~(sysconf(_SC_PAGE_SIZE) - 1);
	map_offset = phyaddr - page_align_addr;
	map_size = map_offset +4;

	map_addr = mem_mmap2(fd, map_size, page_align_addr);
	if (map_addr == NULL) {
		DBG_ERR("page_align_addr(0x%x),map_offset(0x%x),map_size(0x%x)\r\n", page_align_addr,map_offset,map_size);
		DBG_ERR("mmap error for addr 0x%x\r\n", phyaddr);
		return 0;
	}
	//DBG_DUMP("map_addr = 0x%08X, map_size = 0x%x\r\n", map_addr, map_size);
	virtual_addr = (UINT32)(map_addr + map_offset);
	uiRet=*((UINT32 *)virtual_addr);
	mem_munmap2(map_addr, map_size);
	return uiRet;
}

static BOOL Cmd_user_ShowPowerStatus(unsigned char argc, char **argv)
{
  UINT32 uiEther[6]={0};
  UINT32 uiGDC=0;

	fd = init_mem_dev();
	if (fd < 0) {
		printf("Open /dev/mem device failed\n");
		return -1;
	}
	//GDC
	uiGDC =Cmd_user_KernelAddrDump(0xF0C20004);
	if (uiGDC&0x20) // bit5, DC_EN
	{
		DBG_DUMP("GDC ENABLE\r\n");
	} else {
		DBG_DUMP("GDC DISABLE\r\n");
	}

	//ETH
	uiEther[0] =Cmd_user_KernelAddrDump(0xF02B3AE8);
	uiEther[1] =Cmd_user_KernelAddrDump(0xF02B389C);
	uiEther[2] =Cmd_user_KernelAddrDump(0xF02B38CC);
	uiEther[3] =Cmd_user_KernelAddrDump(0xF02B3888);
	uiEther[4] =Cmd_user_KernelAddrDump(0xF02B38C8);
	uiEther[5] =Cmd_user_KernelAddrDump(0xF02B38F8);


	if (uiEther[0]==0x01&&uiEther[1]==0x33&&uiEther[2]==0xA5\
	    &&uiEther[3]==0x08&&uiEther[4]==0x03&&uiEther[5]==0x61)
	{
		DBG_DUMP("Eth Power down\r\n");
	} else {
		DBG_DUMP("Eth don't power down\r\n");
	}

	uninit_mem_dev(fd);
	return TRUE;
}
//#NT#20220712#Philex Lin - end

SXCMD_BEGIN(sys_cmd_tbl, "system command")
SXCMD_ITEM("mem %", cmd_sys_mem, "system memory layout")
SXCMD_ITEM("pip %", Cmd_user_pip, "pip view style")
SXCMD_ITEM("power", Cmd_user_ShowPowerStatus, "show gdc/eth power down status")
SXCMD_ITEM("dumpfbt", Cmd_user_DumpFastBoot, "dump fast boot setting")
SXCMD_ITEM("dumpfbt_fdtapp", Cmd_user_DumpFastBoot_fdtapp, "dump fast boot fdt app")

#if defined(_UI_STYLE_LVGL_)
SXCMD_ITEM("event %", Cmd_user_EventTest, "lvgl user event test")
#endif
#if (CURL_FUNC == ENABLE)
SXCMD_ITEM("curl", Cmd_user_CurlTest, "curl command test")
#endif
SXCMD_END()


static int sys_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(sys_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "mem");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", sys_cmd_tbl[loop].p_name, sys_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(sys, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(sys_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		sys_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], sys_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = sys_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	sys_cmd_showhelp(vk_printk);
	return 0;
}

