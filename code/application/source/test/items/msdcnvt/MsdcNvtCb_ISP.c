#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <umsd.h>
#include <msdcnvt/MsdcNvtApi.h>
#include "MsdcNvtCb_ISP.h"
#include "vendor_isp.h"
#include "MsdcNvtCb.h"

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

//Step 1: Declare custom functions
//Declare Gets Functions
static void read_cmd(void);
static void read_cmd_arg(void);
//Declare Sets Functions
static void write_cmd(void);
static void write_cmd_arg(void);

//Step 2: Create your function mapping table for 'Get' Command
static void (*msdc_isp_get[])(void) = {
	read_cmd,
	read_cmd_arg
};

//Step 3: Create your function mapping table for 'Set' Command
static void (*msdc_isp_set[])(void) = {
	write_cmd,
	write_cmd_arg
};

//Step 4: Provide API for Register Single Direction Functions
BOOL msdc_nvt_reg_si_isp(void)
{
	return MsdcNvt_AddCallback_Si(msdc_isp_get, sizeof(msdc_isp_get) / sizeof(msdc_isp_get[0]), msdc_isp_set, sizeof(msdc_isp_set) / sizeof(msdc_isp_set[0]));
}

//Step 5: Start to implement your custom function
#define _IOC_NONE  0U
#define _IOC_WRITE 1U
#define _IOC_READ  2U

#define _IOC_NRBITS 8
#define _IOC_TYPEBITS 8

#define _IOC_SIZEBITS 14
#define _IOC_DIRBITS 2

#define _IOC_NRMASK ((1 << _IOC_NRBITS)-1)
#define _IOC_TYPEMASK ((1 << _IOC_TYPEBITS)-1)
#define _IOC_SIZEMASK ((1 << _IOC_SIZEBITS)-1)
#define _IOC_DIRMASK ((1 << _IOC_DIRBITS)-1)

#define _IOC_NRSHIFT 0
#define _IOC_TYPESHIFT (_IOC_NRSHIFT+_IOC_NRBITS)
#define _IOC_SIZESHIFT (_IOC_TYPESHIFT+_IOC_TYPEBITS)
#define _IOC_DIRSHIFT (_IOC_SIZESHIFT+_IOC_SIZEBITS)

#define _IOC_DIR(nr) (((nr) >> _IOC_DIRSHIFT) & _IOC_DIRMASK)
#define _IOC_TYPE(nr) (((nr) >> _IOC_TYPESHIFT) & _IOC_TYPEMASK)
#define _IOC_NR(nr) (((nr) >> _IOC_NRSHIFT) & _IOC_NRMASK)
#define _IOC_SIZE(nr) (((nr) >> _IOC_SIZESHIFT) & _IOC_SIZEMASK)

#define ISP_IOC_COMMON 'c'
#define ISP_IOC_AE     'e'
#define ISP_IOC_AWB    'w'
#define ISP_IOC_DR     'd'
#define ISP_IOC_IQ     'q'
#define ISP_IOC_AF     'f'

static INT32 isp_common_ioctl(INT32 cmd, UINT32 *buf_addr)
{
	INT32 ret = 0;

	if ((_IOC_DIR(cmd) == _IOC_READ)||(_IOC_DIR(cmd) == (_IOC_READ|_IOC_WRITE))) {
		ret = vendor_isp_get_common(_IOC_NR(cmd), buf_addr);
		if (ret == HD_OK) {
			ret = CMD_PASS;
		} else {
			ret = CMD_FAIL;
		}
	} else if (_IOC_DIR(cmd) == _IOC_WRITE) {
		ret = vendor_isp_set_common(_IOC_NR(cmd), buf_addr);
		if (ret == HD_OK) {
			ret = CMD_PASS;
		} else {
			ret = CMD_FAIL;
		}
	} else {
		return CMD_NONE;
	}

	return ret;
}

static INT32 isp_ae_ioctl(INT32 cmd, UINT32 *buf_addr)
{
	INT32 ret = 0;

	if ((_IOC_DIR(cmd) == _IOC_READ)||(_IOC_DIR(cmd) == (_IOC_READ|_IOC_WRITE))) {
		ret = vendor_isp_get_ae(_IOC_NR(cmd), buf_addr);
		if (ret == HD_OK) {
			ret = CMD_PASS;
		} else {
			ret = CMD_FAIL;
		}
	} else if (_IOC_DIR(cmd) == _IOC_WRITE) {
		ret = vendor_isp_set_ae(_IOC_NR(cmd), buf_addr);
		if (ret == HD_OK) {
			ret = CMD_PASS;
		} else {
			ret = CMD_FAIL;
		}
	} else {
		return CMD_NONE;
	}

	return ret;
}

static INT32 isp_awb_ioctl(INT32 cmd, UINT32 *buf_addr)
{
	INT32 ret = 0;

	if ((_IOC_DIR(cmd) == _IOC_READ)||(_IOC_DIR(cmd) == (_IOC_READ|_IOC_WRITE))) {
		ret = vendor_isp_get_awb(_IOC_NR(cmd), buf_addr);
		if (ret == HD_OK) {
			ret = CMD_PASS;
		} else {
			ret = CMD_FAIL;
		}
	} else if (_IOC_DIR(cmd) == _IOC_WRITE) {
		ret = vendor_isp_set_awb(_IOC_NR(cmd), buf_addr);
		if (ret == HD_OK) {
			ret = CMD_PASS;
		} else {
			ret = CMD_FAIL;
		}
	} else {
		return CMD_NONE;
	}

	return ret;
}

static INT32 isp_iq_ioctl(INT32 cmd, UINT32 *buf_addr)
{
	INT32 ret = CMD_PASS;

	if ((_IOC_DIR(cmd) == _IOC_READ)||(_IOC_DIR(cmd) == (_IOC_READ|_IOC_WRITE))) {
		ret = vendor_isp_get_iq(_IOC_NR(cmd), buf_addr);
		if (ret == HD_OK) {
			ret = CMD_PASS;
		} else {
			ret = CMD_FAIL;
		}
	} else if (_IOC_DIR(cmd) == _IOC_WRITE) {
		ret = vendor_isp_set_iq(_IOC_NR(cmd), buf_addr);
		if (ret == HD_OK) {
			ret = CMD_PASS;
		} else {
			ret = CMD_FAIL;
		}
	} else {
		return CMD_NONE;
	}

	return ret;
}

int do_io( UINT32 cmd, UINT32 *data)
{
	int ret = 0;

	switch (_IOC_TYPE(cmd)) {
	case ISP_IOC_COMMON:
		ret = isp_common_ioctl(cmd, data);
		break;

	case ISP_IOC_AE:
		ret = isp_ae_ioctl(cmd, data);
		break;

	case ISP_IOC_AWB:
		ret = isp_awb_ioctl(cmd, data);
		break;

	case ISP_IOC_AF:
		//ret = do_ioc_af(pdev_info, cmd, arg);
		break;

	case ISP_IOC_IQ:
		ret = isp_iq_ioctl(cmd, data);
		break;

	default:
		printf("Error from %s : CMD(0x%.8x) is not support!\n", __FUNCTION__,  (UINT)cmd);
		ret = CMD_NONE;
		break;
	}

	return ret;
}

#define DEMON_VER       ((0 << 28) | (0x06 << 20) | (0x00 << 12) | (408))    // 0.05.00.0408
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
			*data = 0x1;
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
	UINT32 *pData    = (UINT32 *)MsdcNvtCb_GetTransAddr();
	UINT32  uiLength = MsdcNvtCb_GetTransSize();

	if(uiLength < sizeof(ISP_USB_CMD_HEAD)) {
		printf("%s, size < 16 \r\n", __FUNCTION__);
		return;
	}

	memcpy(pData, g_ParamBuffer, uiLength);
}

static void read_cmd_arg(void)
{
	UINT32 *pData	 = (UINT32 *)MsdcNvtCb_GetTransAddr();
	UINT32	uiLength = MsdcNvtCb_GetTransSize();
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
			//return -1;
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

static void write_cmd_arg(void)
{
	UINT32 *pData    = (UINT32 *)MsdcNvtCb_GetTransAddr();
	UINT32  uiLength = MsdcNvtCb_GetTransSize();
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
	UINT32 *pData	 = (UINT32 *)MsdcNvtCb_GetTransAddr();
	UINT32	uiLength = MsdcNvtCb_GetTransSize();
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
		/*case CBW_TYPE_IO_READ:
		case CBW_TYPE_IO_READ_ARG:
			cmd_state = do_io(cmd, (UINT32 *)start_addr);
			loop_count --;
			break;
		case CBW_TYPE_DEMON_READ:
		case CBW_TYPE_DEMON_READ_ARG:
			cmd_state = demon_read(cmd, (UINT32 *)start_addr);
			loop_count --;
			break;*/
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
			//return -1;
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
