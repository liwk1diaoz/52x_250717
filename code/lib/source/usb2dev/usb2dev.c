
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <kwrap/nvt_type.h>
#include <kwrap/debug.h>
#include <kwrap/error_no.h>
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <usb2dev_ioctl.h>
#include "usb2dev.h"

#define USB_MNG_MAGICNO                 0xAD5A609F

CONTROL_XFER usb2dev_control_data;


int m_fd = -1;


static USB_GENERIC_CB	usb2dev_evt_cb = NULL;
static FP				usb2dev_suspend_cb = NULL;
static FP				usb2dev_open_needed_fifo_cb = NULL;
static USB_GENERIC_CB   usb2dev_charging_cb = NULL;
static USB_GENERIC_CB   usb2dev_cx_vendor_cb = NULL;
static FP				usb2dev_cx_class_cb = NULL;
static USB_GENERIC_CB   usb2dev_set_intf_cb = NULL;
static USB_GENERIC_CB	usb2dev_std_unknown_cb = NULL;

THREAD_HANDLE USB2DEV_DRV_ID = 0;
THREAD_DECLARE(usb2dev_driver_thread, arglist)
{
	int er;
	USB2DEV_EVENT usbioc_event;

	while(1) {
		if ((er = ioctl(m_fd, USB2DEV_IOC_GET_EVENT, &usbioc_event)) < 0) {
			DBG_ERR("IOC_GET_EVENT failed.\n");
		}

		//DBG_DUMP("event got = %d %d\r\n", usbioc_event.evt_type, usbioc_event.event);


		switch (usbioc_event.evt_type) {
		case USB2DEV_EVT_TYPE_USBEVENT_CB:
			if (usb2dev_evt_cb) {
				usb2dev_evt_cb(usbioc_event.event);
			}
			break;
		case USB2DEV_EVT_TYPE_SUSPEND_CB:
			if (usb2dev_suspend_cb) {
				usb2dev_suspend_cb();
			}
			break;
		case USB2DEV_EVT_TYPE_OPEN_NEEDED_FIFO_CB:
			if (usb2dev_open_needed_fifo_cb) {
				usb2dev_open_needed_fifo_cb();
			}
			break;
		case USB2DEV_EVT_TYPE_CHARGING_CB:
			if (usb2dev_charging_cb) {
				usb2dev_charging_cb(usbioc_event.event);
			}
			break;
		case USB2DEV_EVT_TYPE_CX_VENDOR:
			if (usb2dev_cx_vendor_cb) {
				memcpy(&usb2dev_control_data.device_request, usbioc_event.device_request, 8);
				usb2dev_cx_vendor_cb(usbioc_event.event);
			}
			break;
		case USB2DEV_EVT_TYPE_CX_CLASS:
			if (usb2dev_cx_class_cb) {
				memcpy(&usb2dev_control_data.device_request, usbioc_event.device_request, 8);
				usb2dev_cx_class_cb();
			}
			break;
		case USB2DEV_EVT_TYPE_SET_INTF_CB:
			if (usb2dev_set_intf_cb) {
				memcpy(&usb2dev_control_data.device_request, usbioc_event.device_request, 8);
				usb2dev_set_intf_cb(usbioc_event.event);
			}
			break;
		case USB2DEV_EVT_TYPE_STDUNKNOWN_CB:
			if (usb2dev_std_unknown_cb) {
				memcpy(&usb2dev_control_data.device_request, usbioc_event.device_request, 8);
				usb2dev_std_unknown_cb(usbioc_event.event);
			}
			break;

		default: {
				break;
			}
		}

		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_EVENT_DONE, NULL)) < 0) {
			DBG_ERR("IOC_SET_EVENT_DONE failed.\n");
		}
	}

	THREAD_RETURN(0);
}




ER usb2dev_open(void)
{
	int er;

	if (m_fd == -1) {
		if ((m_fd = open("/dev/nvt_usb2dev0", O_RDWR)) < 0) {
			DBG_ERR("Error to open /dev/nvt_usb2dev0\n");
			return E_SYS;
		}
	}

	THREAD_CREATE(USB2DEV_DRV_ID, usb2dev_driver_thread, NULL, "usb2dev_driver_thread");
	THREAD_RESUME(USB2DEV_DRV_ID);

	if ((er = ioctl(m_fd, USB2DEV_IOC_OPEN, NULL)) < 0) {
		DBG_ERR("USB2DEV_IOC_OPEN failed.\n");
		return E_SYS;
	}

	return E_OK;
}

void usb2dev_init_management(USB_MNG *p_management)
{
	if (m_fd == -1) {
		if ((m_fd = open("/dev/nvt_usb2dev0", O_RDWR)) < 0) {
			DBG_ERR("Error to open /dev/nvt_usb2dev0\n");
		}
	}

	if (p_management) {
		memset(p_management, 0x00, sizeof(USB_MNG));
		p_management->magic_no = USB_MNG_MAGICNO;
	}
}

ER usb2dev_set_management(USB_MNG *p_management)
{
	int				er;
	USB2DEV_DESC	desc;
	USB2DEV_EPCFG   epcfg;
	UINT32			cfg;

	if (!p_management) {
		DBG_ERR("NULL PTR!\r\n");
		return E_PAR;
	}

	if (p_management->magic_no != USB_MNG_MAGICNO) {
		DBG_ERR("UN-INIT!\r\n");
		return E_CTX;
	}

	if (p_management->p_dev_desc) {
		desc.ui_length = p_management->p_dev_desc->b_length;
		memcpy(desc.desc_data, p_management->p_dev_desc, desc.ui_length);
		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_DEVICE_DESC, &desc)) < 0) {
			DBG_ERR("IOC_SET_DEVICE_DESC failed.\n");
			return E_SYS;
		}
	}

	if (p_management->p_config_desc_hs) {

		desc.ui_length = p_management->p_config_desc_hs->w_total_length;

		if(desc.ui_length > USB_DESC_MAX_SIZE) {
			DBG_ERR("cfg desc size exceed %d\r\n", USB_DESC_MAX_SIZE);
			return E_CTX;
		}

		memcpy(desc.desc_data, p_management->p_config_desc_hs, desc.ui_length);
		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_CFG_HS_DESC, &desc)) < 0) {
			DBG_ERR("IOC_SET_CFG_HS_DESC failed.\n");
			return E_SYS;
		}
	}

	if (p_management->p_config_desc_fs) {

		desc.ui_length = p_management->p_config_desc_fs->w_total_length;

		if(desc.ui_length > USB_DESC_MAX_SIZE) {
			DBG_ERR("cfg desc size exceed %d\r\n", USB_DESC_MAX_SIZE);
			return E_CTX;
		}

		memcpy(desc.desc_data, p_management->p_config_desc_fs, desc.ui_length);
		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_CFG_FS_DESC, &desc)) < 0) {
			DBG_ERR("IOC_SET_CFG_FS_DESC failed.\n");
			return E_SYS;
		}
	}

	if (p_management->p_config_desc_fs_other) {

		desc.ui_length = p_management->p_config_desc_fs_other->w_total_length;

		if(desc.ui_length > USB_DESC_MAX_SIZE) {
			DBG_ERR("cfg desc size exceed %d\r\n", USB_DESC_MAX_SIZE);
			return E_CTX;
		}

		memcpy(desc.desc_data, p_management->p_config_desc_fs_other, desc.ui_length);
		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_CFG_FS_OTHER_DESC, &desc)) < 0) {
			DBG_ERR("IOC_SET_CFG_FS_OTHER_DESC failed.\n");
			return E_SYS;
		}
	}

	if (p_management->p_config_desc_hs_other) {

		desc.ui_length = p_management->p_config_desc_hs_other->w_total_length;

		if(desc.ui_length > USB_DESC_MAX_SIZE) {
			DBG_ERR("cfg desc size exceed %d\r\n", USB_DESC_MAX_SIZE);
			return E_CTX;
		}

		memcpy(desc.desc_data, p_management->p_config_desc_hs_other, desc.ui_length);
		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_CFG_HS_OTHER_DESC, &desc)) < 0) {
			DBG_ERR("IOC_SET_CFG_HS_OTHER_DESC failed.\n");
			return E_SYS;
		}
	}

	cfg = p_management->num_of_configurations;
	if ((er = ioctl(m_fd, USB2DEV_IOC_SET_CFG_NUMBER, &cfg)) < 0) {
		DBG_ERR("IOC_SET_CFG_NUMBER failed.\n");
		return E_SYS;
	}
	cfg = p_management->num_of_strings;
	if ((er = ioctl(m_fd, USB2DEV_IOC_SET_STRING_NUMBER, &cfg)) < 0) {
		DBG_ERR("IOC_SET_STRING_NUMBER failed.\n");
		return E_SYS;
	}

	if (p_management->p_dev_quali_desc) {
		desc.ui_length = p_management->p_dev_quali_desc->b_length;
		memcpy(desc.desc_data, p_management->p_dev_quali_desc, desc.ui_length);
		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_DEV_QUALI_DESC, &desc)) < 0) {
			DBG_ERR("IOC_SET_DEV_QUALI_DESC failed.\n");
			return E_SYS;
		}
	}

	if (p_management->p_string_desc[0]) {
		desc.ui_length = p_management->p_string_desc[0]->b_length;
		memcpy(desc.desc_data, p_management->p_string_desc[0], desc.ui_length);
		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_STRING0_DESC, &desc)) < 0) {
			DBG_ERR("IOC_SET_STRING0_DESC failed.\n");
			return E_SYS;
		}
	}
	if (p_management->p_string_desc[1]) {
		desc.ui_length = p_management->p_string_desc[1]->b_length;
		memcpy(desc.desc_data, p_management->p_string_desc[1], desc.ui_length);
		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_STRING1_DESC, &desc)) < 0) {
			DBG_ERR("IOC_SET_STRING1_DESC failed.\n");
			return E_SYS;
		}
	}
	if (p_management->p_string_desc[2]) {
		desc.ui_length = p_management->p_string_desc[2]->b_length;
		memcpy(desc.desc_data, p_management->p_string_desc[2], desc.ui_length);
		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_STRING2_DESC, &desc)) < 0) {
			DBG_ERR("IOC_SET_STRING2_DESC failed.\n");
			return E_SYS;
		}
	}
	if (p_management->p_string_desc[3]) {
		desc.ui_length = p_management->p_string_desc[3]->b_length;
		memcpy(desc.desc_data, p_management->p_string_desc[3], desc.ui_length);
		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_STRING3_DESC, &desc)) < 0) {
			DBG_ERR("IOC_SET_STRING3_DESC failed.\n");
			return E_SYS;
		}
	}
	if (p_management->p_string_desc[4]) {
		desc.ui_length = p_management->p_string_desc[4]->b_length;
		memcpy(desc.desc_data, p_management->p_string_desc[4], desc.ui_length);
		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_STRING4_DESC, &desc)) < 0) {
			DBG_ERR("IOC_SET_STRING4_DESC failed.\n");
			return E_SYS;
		}
	}
	if (p_management->p_string_desc[5]) {
		desc.ui_length = p_management->p_string_desc[5]->b_length;
		memcpy(desc.desc_data, p_management->p_string_desc[5], desc.ui_length);
		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_STRING5_DESC, &desc)) < 0) {
			DBG_ERR("IOC_SET_STRING5_DESC failed.\n");
			return E_SYS;
		}
	}
	if (p_management->p_string_desc[6]) {
		desc.ui_length = p_management->p_string_desc[6]->b_length;
		memcpy(desc.desc_data, p_management->p_string_desc[6], desc.ui_length);
		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_STRING6_DESC, &desc)) < 0) {
			DBG_ERR("IOC_SET_STRING6_DESC failed.\n");
			return E_SYS;
		}
	}
	if (p_management->p_string_desc[7]) {
		desc.ui_length = p_management->p_string_desc[7]->b_length;
		memcpy(desc.desc_data, p_management->p_string_desc[7], desc.ui_length);
		if ((er = ioctl(m_fd, USB2DEV_IOC_SET_STRING7_DESC, &desc)) < 0) {
			DBG_ERR("IOC_SET_STRING7_DESC failed.\n");
			return E_SYS;
		}
	}


	memcpy(&epcfg, p_management->ep_config_hs, sizeof(USB2DEV_EPCFG));
	if ((er = ioctl(m_fd, USB2DEV_IOC_SET_EPCFG_HS, &epcfg)) < 0) {
		DBG_ERR("IOC_SET_STRING7_DESC failed.\n");
		return E_SYS;
	}
	memcpy(&epcfg, p_management->ep_config_fs, sizeof(USB2DEV_EPCFG));
	if ((er = ioctl(m_fd, USB2DEV_IOC_SET_EPCFG_FS, &epcfg)) < 0) {
		DBG_ERR("IOC_SET_STRING7_DESC failed.\n");
		return E_SYS;
	}

	usb2dev_evt_cb = p_management->fp_event_callback;
	usb2dev_suspend_cb = p_management->fp_usb_suspend;
	usb2dev_open_needed_fifo_cb = p_management->fp_open_needed_fifo;

	return E_OK;
}


ER usb2dev_close(void)
{
	int er;

	if (m_fd < 0) {
		DBG_ERR("usb2dev not opened.\n");
		return E_SYS;
	}

	if ((er = ioctl(m_fd, USB2DEV_IOC_CLOSE, NULL)) < 0) {
		DBG_ERR("USB2DEV_IOC_CLOSE failed.\n");
		return E_SYS;
	}

	close(m_fd);
	m_fd = -1;

	THREAD_DESTROY(USB2DEV_DRV_ID);

	return E_OK;
}


void usb2dev_unmask_ep_interrupt(USB_EP EPn)
{
	int er;
	UINT32 value;

	value = EPn;
	if ((er = ioctl(m_fd, USB2DEV_IOC_SET_UNMASK_EPINT, &value)) < 0) {
		DBG_ERR("IOC_SET_UNMASK_EPINT failed.\n");
	}
}

void usb2dev_mask_ep_interrupt(USB_EP EPn)
{
	int er;
	UINT32 value;

	value = EPn;
	if ((er = ioctl(m_fd, USB2DEV_IOC_SET_MASK_EPINT, &value)) < 0) {
		DBG_ERR("IOC_SET_MASK_EPINT failed.\n");
	}
}


void usb2dev_reture_setup_data(void)
{
	int er;
	USB2DEV_CX_RET_DATA RetDat;

	RetDat.ui_length = usb2dev_control_data.w_length;
	RetDat.p_data    = usb2dev_control_data.p_data;

	if ((er = ioctl(m_fd, USB2DEV_IOC_CX_RET_DATA, &RetDat)) < 0) {
		DBG_ERR("IOC_CX_RET_DATA failed.\n");
	}
}

void usb2dev_set_ep0_done(void)
{
	int er;

	if ((er = ioctl(m_fd, USB2DEV_IOC_SET_CX_DONE, NULL)) < 0) {
		DBG_ERR("IOC_SET_CX_DONE failed.\n");
	}
}


ER usb2dev_set_callback(USB_CALLBACK callBackID, USB_GENERIC_CB pCallBack)
{
	switch (callBackID) {
	case USB_CALLBACK_CHARGING_EVENT: {
			usb2dev_charging_cb = pCallBack;
		}
		break;
	case USB_CALLBACK_CX_VENDOR_REQUEST: {
			usb2dev_cx_vendor_cb = pCallBack;
		}
		break;
	case USB_CALLBACK_CX_CLASS_REQUEST: {
			usb2dev_cx_class_cb = (FP)pCallBack;
		}
		break;
	case USB_CALLBACK_SET_INTERFACE: {
			usb2dev_set_intf_cb = pCallBack;
		}
		break;
	case USB_CALLBACK_STD_UNKNOWN_REQ: {
			usb2dev_std_unknown_cb = pCallBack;
		}
		break;

	default: {
			return E_NOSPT;
		}
	}

	return E_OK;
}


void usb2dev_set_ep_config(USB_EP EPn, USB_EPCFG_ID CfgID, UINT32 uiCfgValue)
{
	int er;
	USB2DEV_SETEPCFG epcfg;

	epcfg.epn		= EPn;
	epcfg.cfgid		= CfgID;
	epcfg.cfg_value	= uiCfgValue;

	if ((er = ioctl(m_fd, USB2DEV_IOC_SET_EPCFG, &epcfg)) < 0) {
		DBG_ERR("IOC_SET_EPCFG failed.\n");
	}
}

ER usb2dev_set_config(USB_CONFIG_ID CfgID, UINT32 uiCfgValue)
{
	int er;
	USB2DEV_SETCFG epcfg;

	if (m_fd == -1) {
		if ((m_fd = open("/dev/nvt_usb2dev0", O_RDWR)) < 0) {
			DBG_ERR("Error to open /dev/nvt_usb2dev0\n");
		}
	}

	epcfg.cfgid		= CfgID;
	epcfg.cfg_value	= uiCfgValue;

	if ((er = ioctl(m_fd, USB2DEV_IOC_SET_CFG, &epcfg)) < 0) {
		DBG_ERR("IOC_SET_CFG failed.\n");
	}
}

UINT32 usb2dev_get_config(USB_CONFIG_ID configID)
{
	UINT32 ret;

	switch (configID) {

	case USB_CONFIG_ID_CHECK_CXOUT: {
		int er;
		UINT32 value = 0;

		if ((er = ioctl(m_fd, USB2DEV_IOC_CHK_CXOUT, &value)) < 0) {
			DBG_ERR("IOC_IS_HIGHSPEED failed.\n");
		}

		ret = value;
		return ret;

	}
	break;

	default: {
		return 0;
	}
	}
}

USB_CONTROLLER_STATE usb2dev_get_controller_state(void)
{
	int er;
	UINT32 value=0xFF;

	if ((er = ioctl(m_fd, USB2DEV_IOC_GET_CONTROL_STATE, &value)) < 0) {
		DBG_ERR("IOC_GET_CONTROL_STATE failed.\n");
	}

	return (USB_CONTROLLER_STATE) value;
}


ER usb2dev_abort_endpoint(USB_EP EPn)
{
	int er;
	UINT32 value;

	value = EPn;
	if ((er = ioctl(m_fd, USB2DEV_IOC_ABORT_EP, &value)) < 0) {
		DBG_ERR("IOC_ABORT_EP failed.\n");
	}

	return E_OK;
}
ER usb2dev_read_endpoint(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen)
{
	int er;
	USB2DEV_DATA_RW data_rw;

	data_rw.epn 		= EPn;
	data_rw.buffer		= pBuffer;
	data_rw.dma_length	= pDMALen;
	if ((er = ioctl(m_fd, USB2DEV_IOC_READ_EP, &data_rw)) < 0) {
		DBG_ERR("IOC_READ_EP failed.\n");
	}

	return E_OK;
}
ER usb2dev_read_endpoint_timeout(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen, UINT32 timeout_ms)
{
	int er;
	USB2DEV_DATA_RW data_rw;

	data_rw.epn 		= EPn;
	data_rw.buffer		= pBuffer;
	data_rw.dma_length	= pDMALen;
	data_rw.timeout_ms  = timeout_ms;
	if ((er = ioctl(m_fd, USB2DEV_IOC_READ_EP_TMOT, &data_rw)) < 0) {
		DBG_ERR("IOC_READ_EP_TMOT failed.\n");
	}

	return E_OK;
}

ER usb2dev_write_endpoint(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen)
{
	int er;
	USB2DEV_DATA_RW data_rw;

	data_rw.epn 		= EPn;
	data_rw.buffer		= pBuffer;
	data_rw.dma_length	= pDMALen;
	if ((er = ioctl(m_fd, USB2DEV_IOC_WRITE_EP, &data_rw)) < 0) {
		DBG_ERR("IOC_WRITE_EP failed.\n");
	}

	return E_OK;
}
ER usb2dev_write_endpoint_timeout(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen, UINT32 timeout_ms)
{
	int er;
	USB2DEV_DATA_RW data_rw;

	data_rw.epn 		= EPn;
	data_rw.buffer		= pBuffer;
	data_rw.dma_length	= pDMALen;
	data_rw.timeout_ms  = timeout_ms;
	if ((er = ioctl(m_fd, USB2DEV_IOC_WRITE_EP_TMOT, &data_rw)) < 0) {
		DBG_ERR("IOC_WRITE_EP failed.\n");
	}

	return E_OK;
}

ER usb2dev_set_ep_write(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen)
{
	int er;
	USB2DEV_DATA_RW data_rw;

	data_rw.epn 		= EPn;
	data_rw.buffer		= pBuffer;
	data_rw.dma_length	= pDMALen;
	if ((er = ioctl(m_fd, USB2DEV_IOC_SET_EP_W, &data_rw)) < 0) {
		DBG_ERR("IOC_SET_EP_W failed.\n");
	}

	return E_OK;
}

ER usb2dev_set_ep_read(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen)
{
	int er;
	USB2DEV_DATA_RW data_rw;

	data_rw.epn 		= EPn;
	data_rw.buffer		= pBuffer;
	data_rw.dma_length	= pDMALen;
	if ((er = ioctl(m_fd, USB2DEV_IOC_SET_EP_R, &data_rw)) < 0) {
		DBG_ERR("IOC_SET_EP_R failed.\n");
	}

	return E_OK;
}

/*
	return 0 	means add to queue success and no space left
	return 1 	means add to queue success at least 1 entry space left
	return 255 	means add to queue failed due to queue is full
*/
UINT32 usb2dev_set_ep_read_queue(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen)
{
	int er;
	USB2DEV_DATA_RW data_rw;

	data_rw.epn 		= EPn;
	data_rw.buffer		= pBuffer;
	data_rw.dma_length	= pDMALen;
	if ((er = ioctl(m_fd, USB2DEV_IOC_SET_EP_RQ, &data_rw)) < 0) {
		DBG_ERR("IOC_SET_EP_RQ failed.\n");
	}

	return *pDMALen;
}

UINT32 usb2dev_get_ep_bytecount(USB_EP EPn)
{
	int er;
	UINT32 value;

	value = EPn;
	if ((er = ioctl(m_fd, USB2DEV_IOC_GET_BYTECNT, &value)) < 0) {
		DBG_ERR("IOC_GET_BYTECNT failed.\n");
	}

	return value;
}

BOOL usb2dev_check_ep_busy(USB_EP EPn)
{
	int er;
	UINT32 value;

	value = EPn;
	if ((er = ioctl(m_fd, USB2DEV_IOC_CHK_EPBUSY, &value)) < 0) {
		DBG_ERR("IOC_CHK_EPBUSY failed.\n");
	}

	return value;
}

void usb2dev_set_tx0byte(USB_EP EPn)
{
	int er;
	UINT32 value;

	value = EPn;
	if ((er = ioctl(m_fd, USB2DEV_IOC_TX0_BYTE, &value)) < 0) {
		DBG_ERR("IOC_TX0_BYTE failed.\n");
	}
}

void usb2dev_set_ep_stall(USB_EP EPn)
{
	int er;
	UINT32 value;

	value = EPn;
	if ((er = ioctl(m_fd, USB2DEV_IOC_STALL_EP, &value)) < 0) {
		DBG_ERR("IOC_STALL_EP failed.\n");
	}
}

void usb2dev_clear_ep_fifo(USB_EP EPn)
{
	int er;
	UINT32 value;

	value = EPn;
	if ((er = ioctl(m_fd, USB2DEV_IOC_CLEAR_EPFIFO, &value)) < 0) {
		DBG_ERR("IOC_CLEAR_EPFIFO failed.\n");
	}
}

UINT32 usb2dev_get_sof_number(void)
{
	int er;
	UINT32 value = 0;

	if ((er = ioctl(m_fd, USB2DEV_IOC_GET_SOF, &value)) < 0) {
		DBG_ERR("IOC_GET_SOF failed.\n");
	}

	return value;
}

BOOL usb2dev_state_change(void)
{
	int er;
	UINT32 value = 0;
	BOOL ret;

	if (m_fd == -1) {
		if ((m_fd = open("/dev/nvt_usb2dev0", O_RDWR)) < 0) {
			DBG_ERR("Error to open /dev/nvt_usb2dev0\n");
		}
	}

	if ((er = ioctl(m_fd, USB2DEV_IOC_STATE_CHG, &value)) < 0) {
		DBG_ERR("IOC_STATE_CHG failed.\n");
	}

	ret = value;
	return ret;
}

USB_CHARGER_STS usb2dev_check_charger(UINT32 uiDebounce)
{
	int er;
	UINT32 value;
	USB_CHARGER_STS ret;

	value = uiDebounce;
	if ((er = ioctl(m_fd, USB2DEV_IOC_CHK_CHARGER, &value)) < 0) {
		DBG_ERR("IOC_CHK_CHARGER failed.\n");
	}

	ret = (USB_CHARGER_STS)value;
	return ret;
}

BOOL usb2dev_is_highspeed_enabled(void)
{
	int er;
	UINT32 value = 0;
	BOOL ret;

	if ((er = ioctl(m_fd, USB2DEV_IOC_IS_HIGHSPEED, &value)) < 0) {
		DBG_ERR("IOC_IS_HIGHSPEED failed.\n");
	}

	ret = value;
	return ret;
}


