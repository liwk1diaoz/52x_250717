#ifndef __MSDCNVT_API_H
#define __MSDCNVT_API_H
#if !defined(__UITRON)
#include <kwrap/type.h>
typedef enum {
	USBMSDC_CONFIG_ID_CHGVENINBUGADDR, ///< Change Vendor command IN data buffer address
	ENUM_DUMMY4WORD(USBMSDC_CONFIG_ID)
} USBMSDC_CONFIG_ID;
typedef struct _MSDC_OBJ {
	ER(*set_config)(USBMSDC_CONFIG_ID config_id, UINT32 value);
} MSDC_OBJ, *PMSDC_OBJ;
typedef struct {
	UINT32 out_data_buf_len; ///< Data Buffer Length for "Host to MSDC-task" transaction.
	UINT32 in_data_buf;      ///< Data Buffer Address for "MSDC-task to Host" transaction. MSDC-task would prepare this buffer for callback function.
	UINT32 in_data_buf_len;  ///< Host assigned "MSDC-task to Host" transaction length in byte count.
	UINT32 vendor_cmd_buf;   ///< Vendor CBW buffer that device will receive from host for CBW buffer. Fixed 31 bytes.
	UINT32 vendor_csw_buf;   ///< Vendor CSW buffer that device will return to host for CSW buffer, MSDC task should prepare this buffer for callback function, Fixed 13 bytes.
} MSDCVENDOR_PARAM, *PMSDCVENDOR_PARAM;
#else
#include "Type.h"
#endif

/**
     API Version
*/
#define MSDCNVT_API_VERSION 0x16080511U

/**
     The minimun stack size requirement for msdcnvt_tsk
*/
#if defined(MSDC_MIN_BUFFER_SIZE) && (MSDC_MIN_BUFFER_SIZE > 0x240BC)
#define MSDCNVT_REQUIRE_MIN_SIZE MSDC_MIN_BUFFER_SIZE
#else
#define MSDCNVT_REQUIRE_MIN_SIZE 0x240BC
#endif

/**
     @name Macro Tools
*/
//@{
///< Check Begin Parameters from Host. Used in Bi-Direction functions.
#define MSDCNVT_CAST(type, p_data) (type *)msdcnvt_check_params(p_data, sizeof(type))
//@}

/**
     @name BiDirection function register
*/
//@{
#define MSDCNVT_REG_BI_BEGIN(tbl) static MSDCNVT_BI_CALL_UNIT (tbl)[] = { ///< declare bi-direction function table begin
#define MSDCNVT_REG_BI_ITEM(x)  {#x, (x)},                                ///< append a bi-direction function
#define MSDCNVT_REG_BI_END() {NULL, NULL} };                              ///< bi-direction function table end
//@}

/**
    Event

    MsdcNvt reports some events
*/
typedef enum _MSDCNVT_EVENT {
	MSDCNVT_EVENT_HOOK_DBG_MSG_ON,      ///< turn on usb debug message
	MSDCNVT_EVENT_HOOK_DBG_MSG_OFF,     ///< turn off usb debug message
	ENUM_DUMMY4WORD(MSDCNVT_EVENT)
} MSDCNVT_EVENT;

typedef void (*MSDCNVT_CB_EVENT)(MSDCNVT_EVENT event);

/**
    Handler type

    MsdcNvt allows to register following handler
*/
typedef enum _MSDCNVT_HANDLER_TYPE {
	MSDCNVT_HANDLER_TYPE_UNKNOWN,               ///< reversed.
	MSDCNVT_HANDLER_TYPE_VIRTUAL_MEM_MAP,      ///< MSDCNVT_CB_VIRTUAL_MEM_READ, Handler that PC read memory 0xF0000000~0xFFFFFFFF via registered handler
	ENUM_DUMMY4WORD(MSDCNVT_HANDLER_TYPE)
} MSDCNVT_HANDLER_TYPE;

typedef UINT32(*MSDCNVT_CB_VIRTUAL_MEM_READ)(UINT32 mem_offset, UINT32 mem_size);

/**
     MsdcNvt configuration strucutre

     Set working buffer, semaphore, task id for Msdc vendor system using via msdcnvt_init() before calling Msdc_Open()
     For pure linux, p_cache shall be NULL because memory alloc by internal
*/
typedef struct _MSDCNVT_INIT {
	UINT32  api_ver;                        ///< just assign to MSDCNVT_API_VERSION
	UINT8 *p_cache;                         ///< Cache-able Memory (MSDCNVT_REQUIRE_MIN_SIZE Bytes Requirement) Front-End:0x10000,Back-End:0x10000,Msg:0x4000,CBW:0x40
	UINT32  cache_size;                     ///< n Bytes of Cache Memory
	BOOL b_hook_dbg_msg;                 ///< Indicate use USB to seeing message and sending command
	UINT32  uart_index;                     ///< Select to 0:uart_putString or 1:uart2_putString to real uart output
	MSDCNVT_CB_EVENT event_cb;              ///< export MsdcNvt events
	MSDC_OBJ *p_msdc;                       ///< via USB 2.0 or USB 3.0
	//net supporting
	UINT32 port;                            ///< TCP port (set 0 for default 1968)
	UINT32 tos;                             ///< TCP tos value
	UINT32 snd_buf_size;                    ///< Send Socket Buffer Size (0: system default)
	UINT32 rcv_buf_size;                    ///< Recv Socket Buffer Size (0: system default)
} MSDCNVT_INIT, *PMSDCNVT_INIT;

/**
     Null Lun

     Msdc-Nvt provide a null lun for easy to setting msdc structure
*/
#if defined(__UITRON)
typedef struct _MSDCNVT_LUN {
	MSDC_TYPE type;                    ///< MsdcNvt provide suggest msdc type
	DX_HANDLE strg;                    ///< MsdcNvt provide suggest dx handle of storage
	MSDC_STORAGE_DET_CB detect_cb;     ///< MsdcNvt provide suggest card detection callback
	MSDC_STORAGE_LOCK_DET_CB lock_cb;  ///< MsdcNvt provide suggest card lock detection callback
	UINT32 luns;                       ///< MsdcNvt provide suggest the number of lun.
} MSDCNVT_LUN, *PMSDCNVT_LUN;
#else
typedef struct _MSDCNVT_LUN {
	UINT32 reserved;
} MSDCNVT_LUN, *PMSDCNVT_LUN;
#endif

//Bi-Direction Call Function Unit
/**
     Bi Unit

     Msdc-Nvt Bi-Direction Call Function Unit
*/
typedef struct _MSDCNVT_BI_CALL_UNIT {
	char  *p_name;               ///< Function Name
	void (*fp)(void *p_data);    ///< Handler Callback
} MSDCNVT_BI_CALL_UNIT;

/**
     Install task,flag and semaphore id
*/
//#if defined(__UITRON)
extern void msdcnvt_install_id(void) _SECTION(".kercfg_text");
extern void msdcnvt_uninstall_id(void) _SECTION(".kercfg_text");
//#endif

/**
     Msdc-Nvt vendor configuration

     Configure Msdc-Nvt for system initial

     @param[in] p_init       Configuration strucutre
     @return
		- @b TRUE:   configuration success.
		- @b FALSE:  configuration failed.
*/
extern BOOL msdcnvt_init(MSDCNVT_INIT *p_init);

/**
     Msdc-Nvt vendor exit

     Quit Msdc-Nvt

     @return
		- @b TRUE:   exit success.
		- @b FALSE:  failed to exit.
*/
extern BOOL msdcnvt_exit(void);

/**
     Msdc verify callback

     A callback for plug-in USB_MSDC_INFO.msdc_check_cb
*/
extern BOOL msdcnvt_verify_cb(UINT32 p_cmd_buf, UINT32 *p_data_buf);

/**
     Msdc verify callback

     A callback for plug-in USB_MSDC_INFO.msdc_vendor_cb
*/
extern void msdcnvt_vendor_cb(PMSDCVENDOR_PARAM p_buf);

/**
     Get host-device common data buffer address

     Get data address for single directtion callback function

     @return
		- @b NULL:     failed to get buffer address.
		- @b NON-NULL: buffer address.
*/
extern UINT8 *msdcnvt_get_data_addr(void);

/**
     Get host-device common data buffer size (byte unit)

     Get data size for single directtion callback function

     @return
		- @b 0:        failed to get buffer size.
		- @b Non-Zero: buffer size.
*/
extern UINT32 msdcnvt_get_data_size(void);

/**
     Register bi-direction type callback function

     PlugIn vendor function(bi-direction, form PC set then get automatic)

     @param[in] p_unit the structure pointer that bi-direction needs
     @return
		- @b TRUE:    success to register.
		- @b FALSE:   failed to register callback function.
*/
extern BOOL msdcnvt_add_callback_bi(MSDCNVT_BI_CALL_UNIT *p_unit);

/**
     Register single-direction type callback function

     Plugin vendor function(single-direction,from PC-Get and PC-Set)

     @param[in] fp_tbl_get function table for host get data
     @param[in] num_gets    number of functions in fp_tbl_get table
     @param[in] fp_tbl_set function table for host set data
     @param[in] num_sets    number of functions in fp_tbl_set table
     @return
		- @b TRUE:    success to register.
		- @b FALSE:   failed to register callback function.
*/
extern BOOL msdcnvt_add_callback_si(FP *fp_tbl_get, UINT8 num_gets, FP *fp_tbl_set, UINT8 num_sets);

/**
     Get transmitted data size

     For MSDCVendorNVT_AddCallback_Si functions, get pc how many data is transmitted

     @return data size (byte unit).
*/
extern UINT32 msdcnvt_get_trans_size(void);

/**
     Msdc-Nvt Turn ON / OFF Net Service

     After initiating, with CPU2 started, call it on for NET supporting.
     before CPU2 closed, call it off for next NET supporting.

     @param[in] b_turn_on Turn it ON / OFF
     @return
		- @b TRUE:   success.
		- @b FALSE:  failed.
*/
extern BOOL msdcnvt_net(BOOL b_turn_on);


//------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------
/**
     Check Parameters

     for all registered callback as MsdcNvtCb_xxxxx, to check data valid.


     @param[in] p_data the implemented callback input include MSDCEXT_PARENT.
     @param[in] size the size of data that you except
     @return
		- @b Non-Zero: valid data
		- @b NULL: invalid data
*/
void *msdcnvt_check_params(void *p_data, UINT32 size);

/**
     NULL Lun

     Due to attach MSDC application need plug-in a stroage, here provide a virtual
     stroage for user plug-in.

     @return
		- @b virtual stroage information
*/
MSDCNVT_LUN *msdcnvt_get_null_lun(void);

/**
     Handler to read memory 0xF0000000 ~ 0xFFFFFFFF

     @return
		- @b TRUE:   success.
		- @b FALSE:  failed.
*/
extern BOOL msdcnvt_reg_handler(MSDCNVT_HANDLER_TYPE type, void *p_cb);

/**
     Set Msdc Object for current use

     @param[in] p_msdc Msdc Object
     @return
		- @b TRUE:   success.
		- @b FALSE:  failed.
*/
extern BOOL msdcnvt_set_msdcobj(MSDC_OBJ *p_msdc);

/**
     Get current Msdc Object

     @param[in] p_msdc Msdc Object
     @return
		- @b Non-Zero: pointer to Msdc Object.
		- @b NULL:  failed.
*/
extern MSDC_OBJ *msdcnvt_get_msdcobj(void);

#endif
