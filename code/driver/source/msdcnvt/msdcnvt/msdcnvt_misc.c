#include "msdcnvt_int.h"

#define CFG_MSDC_VENDOR_NVT_VERSION  0x12011315 //Year:Month:Day:Hour
#define CFG_MSDC_VENDOR_NVT_TAG      MAKEFOURCC('N', 'O', 'V', 'A')

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_MISC Message
//------------------------------------------------------------------------------
#define CDB_11_MISC_UNKNOWN                     0x00 //!< Invalid Cmd
#define CDB_11_MISC_VERSION                     0x01 //!< Get Version
#define CDB_11_MISC_NOVATAG                     0x02 //!< Get Tag
#define CDB_11_MISC_BK_IS_LOCK                  0x03 //!< Query Background is Lock
#define CDB_11_MISC_BK_IS_FINISH                0x04 //!< Query Background is finish his work
#define CDB_11_MISC_BK_LOCK                     0x05 //!< Lock Background Service by Host
#define CDB_11_MISC_BK_UNLOCK                   0x06 //!< UnLock Background Service by Host
#define CDB_11_MISC_UPD_FW_GET_INFO             0x07 //!< Update Firmware (PC Get Info)
#define CDB_11_MISC_UPD_FW_SET_BLOCK            0x08 //!< Update Firmware (PC Set Block Data)

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_MISC Data Type
//------------------------------------------------------------------------------
//parent for Host <- Device
typedef struct _MSDCNVT_MISC_RESPONSE {
	UINT32 size;      //!< Structure Size
	UINT32 b_handled;    //!< Indicate Device Handle this function (Don't Use BOOL, for 433 Compatiable)
	UINT32 b_ok;         //!< Indicate Function Action is OK or Not (Don't Use BOOL, for 433 Compatiable)
	UINT8  reversed[2]; //!< reversed value for Structure DWORD Alignment
} MSDCNVT_MISC_RESPONSE;

//parent for Host -> PC
typedef struct _MSDCNVT_MISC_DELIVER {
	UINT32 size;      //!< Structure Size
} MSDCNVT_MISC_DELIVER;

//PC Get Version or NovaTag
typedef struct _MSDCNVT_MISC_GET_UINT32 {
	MSDCNVT_MISC_RESPONSE response;   //!< parent
	UINT32               value;       //!< UINT32 value
} MSDCNVT_MISC_GET_UINT32;

//PC Get Nand Block Information
typedef struct _MSDCNVT_MISC_UPDFW_GET_BLOCK_INFO {
	MSDCNVT_MISC_RESPONSE response;   //!< parent
	UINT32 bytes_per_block;      //!< Each Block Size
	UINT32 bytes_temp_buf;       //!< Transmit Swap Buffer Size (inc structure size)
} MSDCNVT_MISC_UPDFW_GET_BLOCK_INFO;

//PC Send Nand Block Data
typedef struct _MSDCNVT_MISC_UPDFW_SET_BLOCK_INFO {
	MSDCNVT_MISC_DELIVER deliver;     //!< parent
	UINT32 blk_idx;              //!< Block Index of this Packet
	UINT32 b_last_block;         //!< Indicate Last Block for Reset System
	UINT32 effect_data_size;      //!< Current Effective Data Size
} MSDCNVT_MISC_UPDFW_SET_BLOCK_INFO;

//------------------------------------------------------------------------------
// CDB_10_FUNCTION_MISC Handler
//------------------------------------------------------------------------------
static void msdcnvt_misc_get_version(void *p_info);          //!< Get MSDC Vendor Nvt Version
static void msdcnvt_misc_get_nova_tag(void *p_info);          //!< Get 'N','O','V','A' Tag
static void msdcnvt_misc_get_bk_is_lock(void *p_info);        //!< Host Query Background Service is lock
static void msdcnvt_misc_get_bk_is_finish(void *p_info);      //!< Host Query Background Service is Finish his work
static void msdcnvt_misc_get_bk_lock(void *p_info);          //!< Host Lock Background Service
static void msdcnvt_misc_get_bk_unlock(void *p_info);        //!< Host UnLock Background Service
static void msdcnvt_misc_set_upd_fw_getinfo(void *p_info);    //!< Update Firmware (PC Get Info)
static void msdcnvt_misc_set_upd_fw_set_block(void *p_info);   //!< Update Firmware (PC Set Block Data)

//Local Variables
static MSDCNVT_FUNCTION_CMD_HANDLE m_call_map[] = {
	{CDB_11_MISC_UNKNOWN, NULL},
	{CDB_11_MISC_VERSION,           msdcnvt_misc_get_version},
	{CDB_11_MISC_NOVATAG,           msdcnvt_misc_get_nova_tag},
	{CDB_11_MISC_BK_IS_LOCK,        msdcnvt_misc_get_bk_is_lock},
	{CDB_11_MISC_BK_IS_FINISH,      msdcnvt_misc_get_bk_is_finish},
	{CDB_11_MISC_BK_LOCK,           msdcnvt_misc_get_bk_lock},
	{CDB_11_MISC_BK_UNLOCK,         msdcnvt_misc_get_bk_unlock},
	{CDB_11_MISC_UPD_FW_GET_INFO,   msdcnvt_misc_set_upd_fw_getinfo},
	{CDB_11_MISC_UPD_FW_SET_BLOCK,  msdcnvt_misc_set_upd_fw_set_block},
};

void msdcnvt_function_misc(void)
{
	MSDCNVT_CTRL *p_ctrl = msdcnvt_get_ctrl();
	MSDCNVT_HOST_INFO *p_host = &p_ctrl->host_info;
	UINT8 cmd_id = p_host->p_cbw->cb[11];
	void *p_info = p_host->p_pool;

	if (cmd_id < sizeof(m_call_map) / sizeof(MSDCNVT_FUNCTION_CMD_HANDLE)
		&& cmd_id == m_call_map[cmd_id].call_id
		&& m_call_map[cmd_id].fp_call != NULL) {
		m_call_map[cmd_id].fp_call(p_info);
	} else {
		DBG_ERR("Call Map Wrong!\r\n");
	}
}

static void msdcnvt_misc_get_version(void *p_info)
{
	MSDCNVT_MISC_GET_UINT32 *p_value = (MSDCNVT_MISC_GET_UINT32 *)p_info;

	p_value->response.size = sizeof(MSDCNVT_MISC_GET_UINT32);
	p_value->response.b_handled = TRUE;

	p_value->value = CFG_MSDC_VENDOR_NVT_VERSION;

	p_value->response.b_ok = TRUE;
}

static void msdcnvt_misc_get_nova_tag(void *p_info)
{
	UINT32 *p_tag = (UINT32 *)p_info;
	*p_tag = CFG_MSDC_VENDOR_NVT_TAG;
}

static void msdcnvt_misc_get_bk_is_lock(void *p_info)
{
	MSDCNVT_MISC_RESPONSE *p_response = (MSDCNVT_MISC_RESPONSE *)p_info;

	memset(p_response, 0, sizeof(MSDCNVT_MISC_RESPONSE));
	p_response->size = sizeof(MSDCNVT_MISC_RESPONSE);
	p_response->b_handled = TRUE;
	p_response->b_ok = msdcnvt_bk_host_is_lock();
}

static void msdcnvt_misc_get_bk_is_finish(void *p_info)
{
	MSDCNVT_MISC_RESPONSE *p_response = (MSDCNVT_MISC_RESPONSE *)p_info;

	memset(p_response, 0, sizeof(MSDCNVT_MISC_RESPONSE));
	p_response->size = sizeof(MSDCNVT_MISC_RESPONSE);
	p_response->b_handled = TRUE;
	p_response->b_ok = msdcnvt_bk_is_finish();
}

static void msdcnvt_misc_get_bk_lock(void *p_info)
{
	MSDCNVT_MISC_RESPONSE *p_response = (MSDCNVT_MISC_RESPONSE *)p_info;

	memset(p_response, 0, sizeof(MSDCNVT_MISC_RESPONSE));
	p_response->size = sizeof(MSDCNVT_MISC_RESPONSE);
	p_response->b_handled = TRUE;
	p_response->b_ok = msdcnvt_bk_host_lock();
}

static void msdcnvt_misc_get_bk_unlock(void *p_info)
{
	MSDCNVT_MISC_RESPONSE *p_response = (MSDCNVT_MISC_RESPONSE *)p_info;

	memset(p_response, 0, sizeof(MSDCNVT_MISC_RESPONSE));
	p_response->size = sizeof(MSDCNVT_MISC_RESPONSE);
	p_response->b_handled = TRUE;
	p_response->b_ok = msdcnvt_bk_host_unlock();
}

static void msdcnvt_misc_set_upd_fw_getinfo(void *p_info)
{
	MSDCNVT_MISC_UPDFW_GET_BLOCK_INFO *p_blk_info = (MSDCNVT_MISC_UPDFW_GET_BLOCK_INFO *)p_info;

	DBG_ERR("NvUSB.dll is version too old\r\n");
	p_blk_info->response.b_ok = FALSE; //unsupport after CFG_MSDC_VENDOR_NVT_VERSION
}

static void msdcnvt_misc_set_upd_fw_set_block(void *p_info)
{
	DBG_ERR("NvUSB.dll is version too old\r\n");
	//unsupport after CFG_MSDC_VENDOR_NVT_VERSION
}
