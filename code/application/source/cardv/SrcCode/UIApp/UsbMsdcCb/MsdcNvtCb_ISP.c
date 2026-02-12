#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <umsd.h>
#include <msdcnvt/MsdcNvtApi.h>
#include "MsdcNvtCb_ISP.h"
#include "vendor_isp.h"
#if !defined(__FREERTOS)
#include <sys/ioctl.h>
#endif

#define MSDCNVT_AF_READY  0

#define CBW_TYPE_READ 0x80
#define CBW_TYPE_READ_ARG 0x40
#define CBW_TYPE_WRITE 0x00
#define CBW_TYPE_DEMON_READ      (CBW_TYPE_READ|0x1)
#define CBW_TYPE_DEMON_WRITE     (CBW_TYPE_WRITE|0x1)
#define CBW_TYPE_DEMON_READ_ARG (CBW_TYPE_READ | CBW_TYPE_READ_ARG|0x1)
#define CBW_TYPE_IO_READ         (CBW_TYPE_READ|0x2)
#define CBW_TYPE_IO_WRITE        (CBW_TYPE_WRITE|0x2)
#define CBW_TYPE_IO_READ_ARG (CBW_TYPE_READ | CBW_TYPE_READ_ARG|0x2)
#define CBW_TYPE_COMBO_CMD    (CBW_TYPE_READ | CBW_TYPE_READ_ARG|0x7)

#define CMD_PASS   0
#define CMD_FAIL   1
#define CMD_NONE   2

typedef struct _ISP_USB_CMD_HEAD{
	UINT32 type;
	UINT32 cmd;
	UINT32 size;
	UINT32 state;
} ISP_USB_CMD_HEAD;

UINT32 g_ParamBuffer[0x4000];
int active_fd = -1;

//Step 1: Declare custom functions
//Declare Gets Functions
static void read_cmd(void);
static void read_cmd_arg(void);
static void read_cmd_test(void);
//Declare Sets Functions
static void write_cmd(void);
static void write_cmd_arg(void);
static void write_cmd_test(void);

//Step 2: Create your function mapping table for 'Get' Command
static void (*msdc_isp_get[])(void) = {
	read_cmd,
	read_cmd_arg,
	read_cmd_test
};

//Step 3: Create your function mapping table for 'Set' Command
static void (*msdc_isp_set[])(void) = {
	write_cmd,
	write_cmd_arg,
	write_cmd_test
};

//Step 4: Provide API for Register Single Direction Functions
BOOL msdc_nvt_reg_si_isp(void)
{
	return MsdcNvt_AddCallback_Si(msdc_isp_get, sizeof(msdc_isp_get) / sizeof(msdc_isp_get[0]), msdc_isp_set, sizeof(msdc_isp_set) / sizeof(msdc_isp_set[0]));
}

int do_io( UINT32 cmd, UINT32 *data)
{
	int ret = CMD_PASS;
#if !defined(__FREERTOS)    
	if (ioctl(active_fd, cmd, data) < 0) {
		printf("[ISPD] Warning from %s : CMD=0x%.8x\n", __FUNCTION__, (UINT)cmd);
		return CMD_FAIL;
    }
#endif

	return ret;
}

#if defined(__FREERTOS)
#define DEMON_VER       ((0 << 28) | (0x06 << 20) | (0x00 << 12) | (914))
#else
#define DEMON_VER       ((0 << 28) | (0x07 << 20) | (0x00 << 12) | (914))    // Linux-USB:0.07.00.0914
#endif
#define DEMON_MAJOR     ((DEMON_VER >> 28) & 0x0F)
#define DEMON_MINOR     ((DEMON_VER >> 20) & 0xFF)
#define DEMON_INFO      ((DEMON_VER >> 12) & 0xFF)
#define DEMON_DATE      (DEMON_VER & 0x0FFF)

#define NOVA_TAG 0x41564F4E //AVON

typedef enum {
	DEMON_GET_VER = 0,
	DEMON_GET_NAVATAG,
	DEMON_GET_DEVICE
} DEMOM_CMD_READ;

static int demon_read(unsigned int cmd, UINT32 *data)
{
	int ret = CMD_PASS;

	switch(cmd){
		case DEMON_GET_VER:
			*data  = DEMON_VER;
			break;
		case DEMON_GET_NAVATAG:
			*data = NOVA_TAG;
			break;
		case DEMON_GET_DEVICE:
#if defined(__FREERTOS)
			*data = 0x1;
#else
			active_fd = open("/dev/nvt_isp", O_RDWR);
			*data = active_fd;
#endif		
			break;
		default:
			ret = CMD_NONE;
			printf("Error from %s : CMD=0x%.8x\n", __FUNCTION__, cmd);
			break;
	}

	return ret;
}

static void read_cmd(void)
{
	UINT32 *pData    = (UINT32 *)MsdcNvt_GetDataBufferAddress();
	UINT32  uiLength = MsdcNvt_GetTransSize();

	if(uiLength < sizeof(ISP_USB_CMD_HEAD)) {
		printf("%s, size < 16 \r\n", __FUNCTION__);
		return;
	}

	memcpy(pData, g_ParamBuffer, uiLength);
}

static void read_cmd_test(void)
{
	UINT32 *pData    = (UINT32 *)MsdcNvt_GetDataBufferAddress();

	*pData = 0xaabbccdd;
	printf("%s, %8X\r\n", __FUNCTION__, *pData);
}

static void read_cmd_arg(void)
{
	UINT32 *pData	 = (UINT32 *)MsdcNvt_GetDataBufferAddress();
	UINT32	uiLength = MsdcNvt_GetTransSize();
	UINT32 *start_addr;
	ISP_USB_CMD_HEAD *head_info;
	UINT32 cmd_type, cmd, cmd_state;
	int loop_count = 1;
	int combo_cmd = 0;
	UINT32 *cmd_info = 0;

	if(uiLength < sizeof(ISP_USB_CMD_HEAD)) {
		printf("%s, size < 16 \r\n", __FUNCTION__);
		return;
	}

	head_info = (ISP_USB_CMD_HEAD *)pData;
	start_addr = pData + (sizeof(ISP_USB_CMD_HEAD)/sizeof(UINT32));
	memcpy(pData, g_ParamBuffer, uiLength);
	cmd_type = head_info->type;
	cmd = head_info->cmd;
	cmd_state = CMD_PASS;
	do{
		switch(cmd_type){
		case CBW_TYPE_IO_READ:
		case CBW_TYPE_IO_READ_ARG:
			cmd_state = do_io(cmd, (UINT32 *)start_addr);
			loop_count --;
			break;
		case CBW_TYPE_DEMON_READ:
		case CBW_TYPE_DEMON_READ_ARG:
			cmd_state = demon_read(cmd, (UINT32 *)start_addr);
			loop_count --;
			break;
		case CBW_TYPE_COMBO_CMD:
			head_info->state = CMD_PASS;
			cmd_info = ((UINT32 *)start_addr);
			loop_count = cmd_info[0];
			cmd_type = cmd_info[2];
			combo_cmd = 1;
			break;
		default:
			cmd_state = CMD_NONE;
			loop_count --;
		}
		if (combo_cmd == 1){
			cmd_info[2] = cmd_state;
			start_addr = start_addr + (cmd_info[1] >> 2);
			cmd_info += 3;
			cmd = cmd_info[0];
		}
	}while(loop_count > 0);
	head_info->state = cmd_state; 
}

static void write_cmd_test(void)
{
	UINT32 *pData    = (UINT32 *)MsdcNvt_GetDataBufferAddress();
	UINT32  uiLength = MsdcNvt_GetTransSize();

	if (uiLength > 1) {
		printf("%s, size=%d data = %8X %8X\r\n", __FUNCTION__, uiLength, pData[0], pData[1]);
	} else {
		printf("%s, size=%d\r\n", __FUNCTION__, uiLength);
	}
}

static void write_cmd_arg(void)
{
	UINT32 *pData    = (UINT32 *)MsdcNvt_GetDataBufferAddress();
	UINT32  uiLength = MsdcNvt_GetTransSize();
	ISP_USB_CMD_HEAD *head_info;

	if(uiLength < sizeof(ISP_USB_CMD_HEAD)) {
		printf("%s, size < 16 \r\n", __FUNCTION__);
		return;
	}

	head_info = (ISP_USB_CMD_HEAD *)pData;
	memcpy(g_ParamBuffer, pData, uiLength);
	head_info->state = CMD_PASS;
}

static void write_cmd(void)
{
	UINT32 *pData	 = (UINT32 *)MsdcNvt_GetDataBufferAddress();
	UINT32	uiLength = MsdcNvt_GetTransSize();
	UINT32 *start_addr;
	ISP_USB_CMD_HEAD *head_info;
	UINT32 cmd_type, cmd, cmd_state;
	int loop_count = 1;
	int combo_cmd = 0;
	UINT32 *cmd_info = 0;

	if(uiLength < sizeof(ISP_USB_CMD_HEAD)) {
		printf("%s, size < 16 \r\n", __FUNCTION__);
		return;
	}

	head_info = (ISP_USB_CMD_HEAD *)pData;
	start_addr = pData + (sizeof(ISP_USB_CMD_HEAD)/sizeof(UINT32));	
	cmd_type = head_info->type;
	cmd = head_info->cmd;
	cmd_state = CMD_PASS;
	do{
		switch(cmd_type){
		case CBW_TYPE_IO_WRITE:
			cmd_state = do_io(cmd, (UINT32 *)start_addr);
			loop_count --;
			break;
		case CBW_TYPE_COMBO_CMD:
			head_info->state = CMD_PASS;
			cmd_info = ((UINT32 *)start_addr);
			loop_count = cmd_info[0];
			cmd_type = cmd_info[2];
			combo_cmd = 1;
			break;
		default:
			cmd_state = CMD_NONE;
			loop_count --;
		}
		if (combo_cmd == 1){
			cmd_info[2] = cmd_state;
			start_addr = start_addr + (cmd_info[1] >> 2);
			printf("%s:  offset = %d\n", __FUNCTION__, (UINT)cmd_info[1]);
			cmd_info += 3;
			cmd = cmd_info[0];
		}
	}while(loop_count > 0);

	head_info->state = cmd_state;
	memcpy(g_ParamBuffer, pData, uiLength);
}
