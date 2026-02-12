#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <umsd.h>
#include <msdcnvt/MsdcNvtApi.h>
#include "MsdcNvtCb_CustomSi.h"

#if defined(__FREERTOS)
#include <uart.h>
#else
#include "uart.h"
#include <termios.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <hd_common.h>
#endif

//for EVB, best to use uart3, because uart2 pin muxed many important pins
//uart3-tx: P_GP13 ; uart3-rx: P_GP14 ; uart pinmux 0x21
#define COM_INDEX 2 // 0: com1, 1: com2, and so on

#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __MODULE__          MsdcNvtCbCom
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((unsigned int)(unsigned char)(ch0) | ((unsigned int)(unsigned char)(ch1) << 8) | ((unsigned int)(unsigned char)(ch2) << 16) | ((unsigned int)(unsigned char)(ch3) << 24 ))
#endif

#ifndef ALIGN_FLOOR
#define ALIGN_FLOOR(value, base)  ((value) & ~((base)-1))                   ///< Align Floor
#define ALIGN_ROUND(value, base)  ALIGN_FLOOR((value) + ((base)/2), base)   ///< Align Round
#define ALIGN_CEIL(value, base)   ALIGN_FLOOR((value) + ((base)-1), base)   ///< Align Ceil
#endif

/**
    Flag Pattern
*/
#define FLGMSDCNVTCBCOM_OPENED    FLGPTN_BIT(0)  //indicate task can start to recevie command
#define FLGMSDCNVTCBCOM_STOPPED   FLGPTN_BIT(1)  //indicate task start to skip all command to wait terminate task
#define FLGMSDCNVTCBCOM_WAIT_INFINATE   FLGPTN_BIT(2)  //use dummy bit to wait infinate

#define NVUSB_FOURCC MAKEFOURCC('N','V','U', 'B')

typedef enum _NVUSB_TRANS_TYPE {
	NVUSB_TRANS_TYPE_UNKNOWN,
	NVUSB_TRANS_TYPE_AUTH,
	NVUSB_TRANS_TYPE_SET,
	NVUSB_TRANS_TYPE_GET,
	NVUSB_TRANS_TYPE_READ,
} NVUSB_TRANS_TYPE;

typedef struct _NVUSB_TRANS_INFO {
	int fourcc; // must be NVUSB_FOURCC
	int trans_type; // NVUSB_TRANS_TYPE
	int trans_hdr_size; // sizeof(NVUSB_TRANS_INFO)
	int data_size; // follow data size without trans_hdr_size
	int err; // error code
	unsigned int sum; // check sum for debug
	unsigned int sn; // sequence number for debug
	unsigned int cmd_id; // for NVUSB_TRANS_TYPE_SET, NVUSB_TRANS_TYPE_GET
	unsigned int addr; // for NVUSB_TRANS_TYPE_READ
}NVUSB_TRANS_INFO;


//Single-Direction Function Control
typedef struct _MSDCNVTCB_SI_HANDLE {
        FP     *fpTblGet;
        UINT8   uiGets;
        FP     *fpTblSet;
        UINT8   uiSets;
} MSDCNVTCB_SI_HANDLE;

typedef struct _COM_OBJ {
	ER (*open)(void);
	ER (*close)(void);
	void (*init)(UART_BAUDRATE BaudRate, UART_LENGTH Length, UART_PARITY Parity);
	UINT32 (*write)(UINT8 *pBuffer, UINT32 Length);
	UINT32 (*read)(UINT8 *pBuffer, UINT32 Length);
	void   (*abort)(void);
	UINT32 (*status)(void);
} COM_OBJ, *PCOM_OBJ;

static UINT32 m_TransSize = 0;
static UINT8 *m_pTransMem = NULL;
static MSDCNVTCB_SI_HANDLE m_SiHandle = {0};
static unsigned char *m_shm = NULL; //share memory
static BOOL m_Loop = TRUE;
static VK_TASK_HANDLE MSDCNVTCBCOM_TSK_ID = 0;
static ID MSDCNVTCBCOM_FLG_ID = 0;
static UART_BAUDRATE  m_BaudRate = UART_BAUDRATE_3000000; //only 115200 and 3M allowed
#if defined(__FREERTOS)
#if (COM_INDEX == 0)
static COM_OBJ m_coms = {uart_open, uart_close, uart_init, uart_write, uart_read, uart_abortGetChar, uart_checkIntStatus};
#elif (COM_INDEX == 1)
static COM_OBJ m_coms = {uart2_open, uart2_close, uart2_init, uart2_write, uart2_read, uart2_abortGetChar, uart2_checkIntStatus};
#elif (COM_INDEX == 2)
static COM_OBJ m_coms = {uart3_open, uart3_close, uart3_init, uart3_write, uart3_read, uart3_abortGetChar, uart3_checkIntStatus};
#elif (COM_INDEX == 3)
static COM_OBJ m_coms = {uart4_open, uart4_close, uart4_init, uart4_write, uart4_read, uart4_abortGetChar, uart4_checkIntStatus};
#elif (COM_INDEX == 4)
static COM_OBJ m_coms = {uart5_open, uart5_close, uart5_init, uart5_write, uart5_read, uart5_abortGetChar, uart5_checkIntStatus};
#elif (COM_INDEX == 5)
static COM_OBJ m_coms = {uart6_open, uart6_close, uart6_init, uart6_write, uart6_read, uart6_abortGetChar, uart6_checkIntStatus};
#endif
#else
#define CFG_DEVICE "/dev/ttyS2"
static int m_fd = -1;
ER uart3_open(void)
{
	// open device
	m_fd = open(CFG_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
	if (m_fd < 0) {
		fprintf(stderr, "msdcnvt_cdc: failed to open %s.\n", CFG_DEVICE);
		return -1;
	}
	fcntl(m_fd, F_SETFL, 0); //blocking
	return E_OK;
}
ER uart3_close(void)
{
	close(m_fd);
	m_fd = -1;
	return E_OK;
}

void uart3_init(UART_BAUDRATE BaudRate, UART_LENGTH Length, UART_PARITY Parity)
{

	// set baudrate
	struct termios newtio = {0};
	newtio.c_cflag = CS8 | CREAD | CLOCAL;
	switch(BaudRate) {
	case UART_BAUDRATE_115200:
		newtio.c_cflag |= B115200;
		break;
	case UART_BAUDRATE_3000000:
	default:
		newtio.c_cflag |= B3000000;
		break;
	}
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;
	newtio.c_cc[VMIN] = 0;
	newtio.c_cc[VTIME] = 10; //1 sec interval
	tcflush(m_fd, TCIFLUSH);
	tcsetattr(m_fd, TCSANOW, &newtio);
}
UINT32 uart3_read(UINT8 *pBuffer, UINT32 Length)
{
	return read(m_fd, pBuffer, Length);
}
UINT32 uart3_write(UINT8 *pBuffer, UINT32 Length)
{
	return write(m_fd, pBuffer, Length);
}
void uart3_abortGetChar(void)
{

}
UINT32 uart3_checkIntStatus(void)
{
	return UART_INT_STATUS_TX_EMPTY;
}
static COM_OBJ m_coms = {uart3_open, uart3_close, uart3_init, uart3_write, uart3_read, uart3_abortGetChar, uart3_checkIntStatus};
#endif

static COM_OBJ *m_com = &m_coms;

UINT8 *MsdcNvtCbCom_GetTransAddr(void)
{
    return m_pTransMem;
}

UINT32 MsdcNvtCbCom_GetTransSize(void)
{
    return m_TransSize;
}

static unsigned int check_sum(unsigned int addr, unsigned int size)
{
	unsigned int i, sum = 0;
	unsigned short *p = (unsigned short *)addr;
	for (i = 0; i < (size >> 1); i++) {
		sum += (*(p + i) + i);
	}
	sum &= 0x0000FFFF;
	return sum;
}

static int com_open(void)
{
	ER er;
	if ((er = m_com->open()) != 0) {
		return -1;
	}
	while ((m_com->status() & UART_INT_STATUS_TX_EMPTY) == 0) {
		;
	}
	m_com->init(m_BaudRate, UART_LEN_L8_S1, UART_PARITY_NONE);
	return er;
}

static int com_read(unsigned char *p, int size)
{
	int read_sum = 0;
	int read_size = 0;
	int remain = size;

	while (remain > 0) {
		read_size = m_com->read(p + read_sum, remain);
		if (read_size) {
			DBG_IND("read: %d\r\n", read_size);
		}
#if defined(__FREERTOS)
		if (read_size == 0) {
			//data abort for rtos
			return -2;
		}
#else
		if (!m_Loop) {
			return -2;
		}
#endif
		remain -= read_size;
		read_sum += read_size;
	}

	if (read_sum != size) {
		DBG_ERR("failed to com_read.\r\n");
		return -1;
	}
	return 0;
}

static int com_write(unsigned char *p, int size)
{
	int write_size = m_com->write(p, size);
	if (write_size != size) {
		DBG_ERR("failed to com_write.\r\n");
		return -1;
	}
	return 0;
}

static int check_valid(NVUSB_TRANS_INFO *p_info)
{
	if (p_info->fourcc != NVUSB_FOURCC) {
		DBG_ERR("error packet's fourcc: %08X.\r\n", p_info->fourcc);
		return -1;
	}
	if (p_info->trans_hdr_size != sizeof(NVUSB_TRANS_INFO)) {
		DBG_ERR("error packet's header size: %d.\r\n", p_info->trans_hdr_size);
		return -1;
	}
	return 0;
}

static int interpret_auth(NVUSB_TRANS_INFO *p_info)
{
	DBG_DUMP("auth, sn=%d\r\n", p_info->sn);
	return com_write((unsigned char *)p_info, sizeof(NVUSB_TRANS_INFO));
}

static int interpret_get(NVUSB_TRANS_INFO *p_info)
{
	int er;
	DBG_DUMP("get_cmd_id=%d, sn=%d\r\n", p_info->cmd_id, p_info->sn);

	if (p_info->cmd_id < m_SiHandle.uiGets) {
		m_TransSize = (UINT32)p_info->data_size;
		m_SiHandle.fpTblGet[p_info->cmd_id]();
	} else {
		DBG_ERR("get exceed id: %d\r\n", p_info->cmd_id);
	}

	//send back the result
	if ((er = com_write(m_shm, p_info->data_size)) != 0) {
		return er;
	}
	p_info->sum = check_sum((unsigned int)m_shm, p_info->data_size);
	return com_write((unsigned char *)p_info, sizeof(NVUSB_TRANS_INFO));
}

static int interpret_set(NVUSB_TRANS_INFO *p_info)
{
	int er;
	DBG_DUMP("set_cmd_id=%d, sn=%d\r\n", p_info->cmd_id, p_info->sn);

	if ((er = com_read(m_shm, p_info->data_size)) != 0) {
		return er;
	}
	if (p_info->sum != check_sum((unsigned int)m_shm, p_info->data_size)) {
		DBG_ERR("msdcnvt_cdc: interpret_set's check sum failed.\r\n");
		return -1;
	}

	if (p_info->cmd_id < m_SiHandle.uiSets) {
		m_TransSize = (UINT32)p_info->data_size;
		m_SiHandle.fpTblSet[p_info->cmd_id]();
	} else {
		DBG_ERR("set exceed id: %d\r\n", p_info->cmd_id);
	}

	return com_write((unsigned char *)p_info, sizeof(NVUSB_TRANS_INFO));
}

static int interpret_read(NVUSB_TRANS_INFO *p_info)
{
	int er;
	//DBG_DUMP("read memory: addr=0x%08X, size=0x%08X, sn=%d\r\n", p_info->addr, p_info->data_size, p_info->sn);
#if defined(__FREERTOS)
	unsigned char *p_mem = (unsigned char *)p_info->addr;
	if ((er = com_write(p_mem, p_info->data_size)) != 0) {
		return er;
	}
	p_info->sum = check_sum((unsigned int)p_mem, p_info->data_size);
#else
	UINT32 aligned_pa = ALIGN_FLOOR(p_info->addr, 4096);
	UINT32 aligned_size = ALIGN_CEIL(p_info->data_size, 4096*2); //2 page align for address front and rear align
	UINT32 ofs = p_info->addr - aligned_pa;
	unsigned char *p_mem = (unsigned char *)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, aligned_pa,aligned_size);
	if ((er = com_write(p_mem+ofs, p_info->data_size)) != 0) {
		hd_common_mem_munmap(p_mem, aligned_size);
		return er;
	}
	p_info->sum = check_sum((unsigned int)p_mem+ofs, p_info->data_size);
	hd_common_mem_munmap(p_mem, aligned_size);
#endif
	return com_write((unsigned char *)p_info, sizeof(NVUSB_TRANS_INFO));
}

static int initiate(void)
{
	int er;

	m_shm = (unsigned char *)MsdcNvtCbCom_GetTransAddr();

	if (MsdcNvt_GetCallback_Si(&m_SiHandle.fpTblGet, &m_SiHandle.uiGets, &m_SiHandle.fpTblSet, &m_SiHandle.uiSets) != TRUE) {
		return -1;
	}

	//open cdc com port
	if ((er = com_open()) != 0) {
		return er;
	}

#if 0 //debug, write fix pattern on pool app
	{
		int i;
		unsigned int *p = (unsigned int *)(0x02EE61C0);
		for (i=0; i<0x3200000/4;i++) {
			p[i] = i;
		}
	}
#endif
	return 0;
}

int interpret(NVUSB_TRANS_INFO *p_info)
{
	int er;
	if ((er = check_valid(p_info)) != 0) {
		return er;
	}
	switch (p_info->trans_type) {
	case NVUSB_TRANS_TYPE_AUTH:
		if ((er = interpret_auth(p_info)) != 0) {
			return er;
		}
		break;
	case NVUSB_TRANS_TYPE_GET:
		if ((er = interpret_get(p_info)) != 0) {
			return er;
		}
		break;
	case NVUSB_TRANS_TYPE_SET:
		if ((er = interpret_set(p_info)) != 0) {
			return er;
		}
		break;
	case NVUSB_TRANS_TYPE_READ:
		if ((er = interpret_read(p_info)) != 0) {
			return er;
		}
		break;
	default:
		DBG_ERR("unknown trans type: %d.\r\n", p_info->trans_type);
		return -1;
	}
	return er;
}

VOID MsdcNvtCbComTsk(VOID)
{
	int er;
	NVUSB_TRANS_INFO info = {0};

	set_flg(MSDCNVTCBCOM_FLG_ID, FLGMSDCNVTCBCOM_OPENED);

	if ((er = initiate()) != 0) {
		DBG_ERR("failed to initiate, please close and reopen msdcnvt via uart\r\n");
		m_Loop = FALSE;
	}

	while (m_Loop && com_read((unsigned char *)&info, sizeof(info)) == 0) {
		if ((er = interpret(&info)) != 0) {
			DBG_ERR("failed to interpret, please close and reopen msdcnvt via uart\r\n");
			break;
		}
		//DBG_DUMP("finish, sn=%d\r\n", info.sn);
	}

	FLGPTN FlgPtn;
	set_flg(MSDCNVTCBCOM_FLG_ID, FLGMSDCNVTCBCOM_STOPPED);
	wai_flg(&FlgPtn, MSDCNVTCBCOM_FLG_ID, FLGMSDCNVTCBCOM_WAIT_INFINATE, TWF_ORW | TWF_CLR);
}

static void MsdcNvtCbCom_InstallID(void)
{
	vos_flag_create(&MSDCNVTCBCOM_FLG_ID, NULL, "MSDCNVTCBCOM_FLG_ID");
}

static void FwSrv_UnInstallID(void)
{
	vos_flag_destroy(MSDCNVTCBCOM_FLG_ID);
}

BOOL MsdcNvtCbCom_Open(void)
{
	FLGPTN FlgPtn;
	m_pTransMem = (UINT8*)malloc(MSDCNVT_REQUIRE_MIN_SIZE_NO_USB);
	if (m_pTransMem == NULL) {
		return FALSE;
	}
	m_Loop = TRUE;
	MsdcNvtCbCom_InstallID();
	clr_flg(MSDCNVTCBCOM_FLG_ID, (FLGPTN) - 1);
	MSDCNVTCBCOM_TSK_ID = vos_task_create(MsdcNvtCbComTsk, NULL, "MsdcNvtCbComTsk", 10, 4096);
	vos_task_resume(MSDCNVTCBCOM_TSK_ID);
	wai_flg(&FlgPtn, MSDCNVTCBCOM_FLG_ID, FLGMSDCNVTCBCOM_OPENED, TWF_ORW);

	return TRUE;
}

BOOL MsdcNvtCbCom_Close(void)
{
	FLGPTN FlgPtn;

	m_Loop = FALSE;
	m_com->abort();
	wai_flg(&FlgPtn, MSDCNVTCBCOM_FLG_ID, FLGMSDCNVTCBCOM_STOPPED, TWF_ORW);
	m_com->close(); //close must be after wait FLGMSDCNVTCBCOM_STOPPED to avoid com_read hang up in task
	vos_task_destroy(MSDCNVTCBCOM_TSK_ID);
	FwSrv_UnInstallID();
	free(m_pTransMem);
	return TRUE;
}

BOOL MsdcNvtCbCom_SetupBaudRate(int BaudRate)
{
	switch(BaudRate) {
	case 115200:
		m_BaudRate = UART_BAUDRATE_115200;
		break;
	case 3000000:
		m_BaudRate = UART_BAUDRATE_3000000;
		break;
	default:
		DBG_ERR("only 115200 or 3000000 allowed, current is %d.", m_BaudRate);
		return FALSE;
	}
	return TRUE;
}