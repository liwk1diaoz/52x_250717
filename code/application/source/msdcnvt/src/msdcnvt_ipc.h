#ifndef _MSDCNVT_IPC_H
#define _MSDCNVT_IPC_H

#define MSDCNVT_IPC_VERSION 0x16080914

typedef struct _MSDCNVT_ICMD {
	unsigned int api_addr; ///< cast by MSDCNVT_IAPI
	unsigned int in_addr;
	unsigned int in_size;
	unsigned int out_addr;
	unsigned int out_size;
	int err;
} MSDCNVT_ICMD;

typedef void(*MSDCNVT_IAPI)(MSDCNVT_ICMD *cmd);

typedef struct _MSDCNVT_ICALL_TBL {
	MSDCNVT_IAPI open;
	MSDCNVT_IAPI close;
	MSDCNVT_IAPI verify;
	MSDCNVT_IAPI vendor;
} MSDCNVT_ICALL_TBL, *PMSDCNVT_ICALL_TBL;

/* USB Mass Storage Class Bulk only protocol Command Block Wrapper */
typedef struct _MSDCNVT_CBW {
	unsigned int  signature;
	unsigned int  tag;
	unsigned int  data_trans_len;
	unsigned char flags;
	unsigned char lun;
	unsigned char cb_len;
	unsigned char cb[16];
} MSDCNVT_CBW;

/* USB Mass Storage Class Bulk only protocol Command Status Wrapper */
typedef struct _MSDCNVT_CSW {
	unsigned int   signature;
	unsigned int   tag;
	unsigned int   residue;
	unsigned char  status;
} MSDCNVT_CSW;

typedef struct _MSDCNVT_VENDOR_OUTPUT {
	MSDCNVT_CSW     csw;
	unsigned int    out_data_buf_len;  ///< Data Buffer Length for "Host to MSDC-task" transaction.
	unsigned int    in_data_buf;      ///< Data Buffer Address for "MSDC-task to Host" transaction. MSDC-task would prepare this buffer for callback function.
	unsigned int    in_data_buf_len;   ///< Host assigned "MSDC-task to Host" transaction length in byte count.
} MSDCNVT_VENDOR_OUTPUT;

typedef struct _MSDCNVT_IPC_MSG {
	long mtype;            ///< must be 1
	unsigned int api_addr; ///< call address
} MSDCNVT_IPC_MSG;

typedef struct _MSDCNVT_IPC_CFG {
	/* common data */
	unsigned int  ipc_version;     ///< must be MSDCNVT_IPC_VERSION
	unsigned int  stop_server;     ///< stop server
	unsigned int  port;            ///< ip address port
	unsigned int  tos;             ///< socket tos value
	unsigned int  snd_buf_size;    ///< socket send buffer size
	unsigned int  rcv_buf_size;    ///< socket receive buffer size
	MSDCNVT_ICALL_TBL call_tbl;    ///< api's table
								   /* command data */
	MSDCNVT_ICMD  cmd;             ///< IPC call
	unsigned char cmd_data[128];
} MSDCNVT_IPC_CFG, *PMSDCNVT_IPC_CFG;

#endif