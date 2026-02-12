/*
    DIS module driver

    NT98520 DIS driver.

    @file       DIS.c
    @ingroup    mIDrvIPP_DIS

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#include    "dis_platform.h"
#include    "dis_lib.h"
#include    "dis_reg.h"
#include    "dis_int.h"


UINT32 g_uiOutAdd0 = 0;
UINT32 g_uiInAdd0  = 0;
UINT32 g_uiInAdd1  = 0;

/**
    @addtogroup mIDrvIPP_DIS
*/
//@{

STR_DIS_REG   g_ModeInfo;
static UINT32 guiDisStatus = DIS_ENGINE_IDLE;
static ID     gDisLockStatus = NO_TASK_LOCKED;

UINT16        g_uiDisFcnt = 0;
BOOL          g_bDebugPrint = FALSE;

// settings
#define DIS_APLY_SEARCH_RNG     DIS_SR_32
#define DIS_APLY_LUT            DIS_LUT_LIN

DIS_BLKSZ DIS_APLY_BLOCK_SZ = DIS_BLKSZ_32x32;
DIS_SUBIN DIS_APLY_SUBIN = DIS_SUBIN_1x;
static DIS_SUBIN_SEL DIS_APLY_SUBIN_SEL = DIS_SUBIN_CHOOSE_0;



#define DRV_SUPPORT_IST 0
static  SEM_HANDLE SEMID_DIS;
#define FLGPTN_DIS_FRAMEEND     FLGPTN_BIT(0)
UINT32 _DIS_REG_BASE_ADDR;
static void (*pDISIntCB)(UINT32 intstatus) = NULL;

//ISR
extern void dis_isr(void);
//control
static ER dis_chkStateMachine(UINT32 Operation, BOOL Update);
static void dis_clr(BOOL bReset);
void dis_enable(BOOL bStart);
static void dis_setLoad(DIS_LOAD_TYPE type);
static void dis_enableInt(BOOL bEnable, UINT32 uiIntr);
extern ER dis_setClockRate(UINT32 uiClock);
static ER   dis_lock(void);
static ER   dis_unlock(void);
static void dis_attach(void);
static void dis_detach(void);
//input/output
void dis_setDmaInAddr(UINT32 uiAddr0, UINT32 uiAddr1);
static void dis_setDmaInOffset(UINT16 uiLineOffset);
void dis_setDmaOutAddr(UINT32 uiAddr0);
//motion estimation
static void dis_setMotionEst(MOTION_EST Mot);
static void dis_setMDSDim(MDS_DIM Dim);
static void dis_setMDSAddr(MOTION_VECTOR *pStartAddr);
static void dis_setMDSOfs(MOTION_VECTOR *pOffset);
static STR_DIS_REG dis_allocateMotionArea(SIZE_CONF *pSizeConf);
//get info
UINT32 dis_getDmaOutAddr(DIS_MOTION_BUFID BufId);
UINT32 dis_getDmaInAddr(DIS_EDGE_BUFID BufId);

static void dis_resetFrameCnt(void);

//for FPGA verification only
static void dis_enableFrmEndLoad(BOOL bStart);
ER dis_start_withFrmEndLoad(void);
ER dis_start_withoutLoad(void);
ER dis_changeInterrupt_withoutLoad(UINT32 uiIntEn);

void dis_create_resource(UINT32 clk_freq)
{
	dis_platform_create_resource(clk_freq);
	OS_CONFIG_FLAG(FLG_ID_DIS);
	SEM_CREATE(SEMID_DIS,1);
}

void dis_release_resource(void)
{
	rel_flg(FLG_ID_DIS);
	SEM_DESTROY(SEMID_DIS);
}

void dis_setBaseAddr(UINT32 uiAddr)
{
	_DIS_REG_BASE_ADDR = uiAddr;
}

/*
    DIS State Machine

    DIS state machine

    @param[in] Operation to execute
    @param[in] Update if we will update state machine status

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER dis_chkStateMachine(UINT32 Operation, BOOL Update)
{
	//static UINT32 uiState = DIS_ENGINE_IDLE;
	ER ER_Code;

	switch (guiDisStatus) {
	case DIS_ENGINE_IDLE:
		switch (Operation) {
		case DIS_ENGINE_ATTACH:
			if (Update) {
				guiDisStatus = DIS_ENGINE_IDLE;
			}
			ER_Code = E_OK;
			break;
		case DIS_ENGINE_DETACH:
			if (Update) {
				guiDisStatus = DIS_ENGINE_IDLE;
			}
			ER_Code = E_OK;
			break;
		case DIS_ENGINE_OPEN:
			if (Update) {
				guiDisStatus = DIS_ENGINE_READY;
			}
			ER_Code = E_OK;
			break;
		default:
			ER_Code = E_OBJ;
			break;
		}
		break;

	case DIS_ENGINE_READY:
		switch (Operation) {
		case DIS_ENGINE_CLOSE:
			if (Update) {
				guiDisStatus = DIS_ENGINE_IDLE;
			}
			ER_Code = E_OK;
			break;
		case DIS_ENGINE_SET2PRV:
			//case DIS_ENGINE_SET2CAP:
			if (Update) {
				guiDisStatus = DIS_ENGINE_PAUSE;    // PAUSE first, and set DIS_ENGINE_SET2RUN to start
			}
			ER_Code = E_OK;
			break;
		case DIS_ENGINE_CHGINT:
			if (Update) {
				guiDisStatus = DIS_ENGINE_READY;
			}
			ER_Code = E_OK;
			break;
		case DIS_ENGINE_SET2CHGSIZE:
			if (Update) {
				guiDisStatus = DIS_ENGINE_READY;
			}
			ER_Code = E_OK;
			break;
		case DIS_ENGINE_SET2READY:
			if (Update) {
				guiDisStatus = DIS_ENGINE_READY;
			}
			ER_Code = E_OK;
			break;
#if 1 //for FPGA only
		case DIS_ENGINE_SET2PAUSE:
			if (Update) {
				guiDisStatus = DIS_ENGINE_PAUSE;
			}
			ER_Code = E_OK;
			break;
#endif
		default:
			ER_Code = E_OBJ;
			break;
		}
		break;

	case DIS_ENGINE_PAUSE:
		switch (Operation) {
		case DIS_ENGINE_SET2PRV:
			//case DIS_ENGINE_SET2CAP:
			if (Update) {
				guiDisStatus = DIS_ENGINE_PAUSE;    // PAUSE first, and set DIS_ENGINE_SET2RUN to start
			}
			ER_Code = E_OK;
			break;
		case DIS_ENGINE_CHGINT:
			if (Update) {
				guiDisStatus = DIS_ENGINE_PAUSE;
			}
			ER_Code = E_OK;
			break;
		case DIS_ENGINE_SET2RUN:
			if (Update) {
				guiDisStatus = DIS_ENGINE_RUN;
			}
			ER_Code = E_OK;
			break;
		case DIS_ENGINE_SET2READY:
			if (Update) {
				guiDisStatus = DIS_ENGINE_READY;
			}
			ER_Code = E_OK;
			break;
		case DIS_ENGINE_CLOSE:
			if (Update) {
				guiDisStatus = DIS_ENGINE_IDLE;
			}
			ER_Code = E_OK;
			break;
		case DIS_ENGINE_SET2PAUSE:
			DBG_WRN("DIS: multiple pause operations!\r\n");
			ER_Code = E_OK;
			break;
		default :
			ER_Code = E_OBJ;
			break;
		}
		break;


	case DIS_ENGINE_RUN:
		switch (Operation) {
		case DIS_ENGINE_SET2CHGSIZE:
			if (Update) {
				guiDisStatus = DIS_ENGINE_RUN;
			}
			ER_Code = E_OK;
			break;
		case DIS_ENGINE_CHGINT:
			if (Update) {
				guiDisStatus = DIS_ENGINE_RUN;
			}
			ER_Code = E_OK;
			break;
		case DIS_ENGINE_SET2PAUSE:
			if (Update) {
				guiDisStatus = DIS_ENGINE_PAUSE;
			}
			ER_Code = E_OK;
			break;
		default :
			ER_Code = E_OBJ;
			break;
		}
		break;
	default:
		ER_Code = E_OBJ;
	}
	if (ER_Code != E_OK) {
		DBG_ERR("State machine error! st %d op %d\r\n", guiDisStatus, Operation);
	}

	return ER_Code;

}


/**
    DIS Load

    Set DIS load bit to 1.

    @return None.
*/
void dis_setLoad(DIS_LOAD_TYPE type)
{
	T_DIS_CONTROL_REGISTER reg1;

	reg1.reg = DIS_GETREG(DIS_CONTROL_REGISTER_OFS);

	switch (type) {
	case DIS_START_LOAD:
		reg1.bit.DIS_LOAD_START = ENABLE;
		//HW clear load, force write zero to prevent multiple loads
		reg1.bit.DIS_LOAD_FD = DISABLE;
		break;
	case DIS_FRMEND_LOAD:
		if (guiDisStatus == DIS_ENGINE_RUN) {
			reg1.bit.DIS_LOAD_FD = ENABLE;
			//HW clear load, force write zero to prevent multiple loads
			reg1.bit.DIS_LOAD_START = DISABLE;
		} else {
			//HW clear load, force write zero to prevent multiple loads
			reg1.bit.DIS_LOAD_FD = DISABLE;
			reg1.bit.DIS_LOAD_START = DISABLE;
		}
		break;
	default :
		DBG_ERR("dis_setLoad() load type error!\r\n");
		//HW clear load, force write zero to prevent multiple loads
		reg1.bit.DIS_LOAD_FD = DISABLE;
		reg1.bit.DIS_LOAD_START = DISABLE;
		break;
	}

	DIS_SETREG(DIS_CONTROL_REGISTER_OFS, reg1.reg);
}

/**
    Enable/Disable DIS Interrupt

    DIS interrupt enable selection.

    @param[in] bEnable Decide selected funtions are to be enabled or disabled.
        \n-@b ENABLE: enable selected functions.
        \n-@b DISABLE: disable selected functions.
    @param[in] uiIntr Indicate the function(s)

    @return None.
*/
void dis_enableInt(BOOL bEnable, UINT32 uiIntr)
{
	T_DIS_INTERRUPT_ENABLE_REGISTER reg1;
	reg1.reg = DIS_GETREG(DIS_INTERRUPT_ENABLE_REGISTER_OFS);

	if (bEnable) {
		reg1.reg |= uiIntr;
	} else {
		reg1.reg &= (~uiIntr);
	}

	DIS_SETREG(DIS_INTERRUPT_ENABLE_REGISTER_OFS, reg1.reg);
}

/**
    DIS Get Interrupt Enable Status

    Get current DIS interrupt Enable status

    @return DIS interrupt Enable status.
*/
UINT32 dis_getIntEnable(void)
{
	T_DIS_INTERRUPT_ENABLE_REGISTER reg1;
	reg1.reg = DIS_GETREG(DIS_INTERRUPT_ENABLE_REGISTER_OFS);

	return reg1.reg;
}

/**
    DIS Get Interrupt Status

    Get current DIS interrupt status

    @return DIS interrupt status readout.
*/
UINT32 dis_getIntrStatus(void)
{
	T_DIS_INTERRUPT_STATUS_REGISTER reg1;
	reg1.reg = DIS_GETREG(DIS_INTERRUPT_STATUS_REGISTER_OFS);

	return reg1.reg;
}

/**
    DIS Clear Interrupt Status

    Clear DIS interrupt status.

    @param[in] uiStatus 1's in this word will clear corresponding interrupt status.

    @return None.
*/
void dis_clrIntrStatus(UINT32 uiStatus)
{
	T_DIS_INTERRUPT_STATUS_REGISTER reg1;
	reg1.reg = uiStatus;
	DIS_SETREG(DIS_INTERRUPT_STATUS_REGISTER_OFS, reg1.reg);
}

//@}

/**
    DIS Reset

    Enable/disable DIS SW reset

    @param[in] bReset
        \n-@b TRUE: enable reset,
        \n-@b FALSE: disable reset.

    @return None.
*/
void dis_clr(BOOL bReset)
{
	T_DIS_CONTROL_REGISTER reg1;
	reg1.reg = DIS_GETREG(DIS_CONTROL_REGISTER_OFS);

	reg1.bit.DIS_SW_RST = bReset;

	reg1.bit.DIS_LOAD_FD = 0;
	reg1.bit.DIS_LOAD_START = 0;

	DIS_SETREG(DIS_CONTROL_REGISTER_OFS, reg1.reg);
}

/**
    DIS Enable

    Trigger DIS HW start

    @param[in] bStart
        \n-@b TRUE: enable.
        \n-@b FALSE: disable.

    @return None.
*/
void dis_enable(BOOL bStart)
{
	T_DIS_CONTROL_REGISTER reg1;
	reg1.reg = DIS_GETREG(DIS_CONTROL_REGISTER_OFS);

	reg1.bit.DIS_START = bStart;

	reg1.bit.DIS_LOAD_FD = 0;
	reg1.bit.DIS_LOAD_START = 0;

	DIS_SETREG(DIS_CONTROL_REGISTER_OFS, reg1.reg);
}

//for FPGA verification only
void dis_enableFrmEndLoad(BOOL bStart)
{
	T_DIS_CONTROL_REGISTER reg1;
	reg1.reg = DIS_GETREG(DIS_CONTROL_REGISTER_OFS);

	reg1.bit.DIS_START = bStart;

	reg1.bit.DIS_LOAD_FD = ENABLE;
	reg1.bit.DIS_LOAD_START = 0;

	DIS_SETREG(DIS_CONTROL_REGISTER_OFS, reg1.reg);
}


/**
    DIS Set Input Starting Addresses

    Set three DMA input starting addresses

    @param[in] uiAddr0 DMA input channel (DMA to DIS) starting address 0.
    @param[in] uiAddr1 DMA input channel (DMA to DIS) starting address 1.
    @param[in] uiAddr2 DMA input channel (DMA to DIS) starting address 2.

    @return None.
*/
void dis_setDmaInAddr(UINT32 uiAddr0, UINT32 uiAddr1)
{
	T_DIS_INPUT_CHANNEL_REGISTER_0 LocalReg0;
	T_DIS_INPUT_CHANNEL_REGISTER_1 LocalReg1;

	LocalReg0.reg = DIS_GETREG(DIS_INPUT_CHANNEL_REGISTER_0_OFS);
	LocalReg0.bit.DRAM_E_SAI0 = uiAddr0 >> 2;

	LocalReg1.reg = DIS_GETREG(DIS_INPUT_CHANNEL_REGISTER_1_OFS);
	LocalReg1.bit.DRAM_E_SAI1 = uiAddr1 >> 2;

	DIS_SETREG(DIS_INPUT_CHANNEL_REGISTER_0_OFS, LocalReg0.reg);
	DIS_SETREG(DIS_INPUT_CHANNEL_REGISTER_1_OFS, LocalReg1.reg);
}

/**
    DIS Set Input Line Offset

    Set DMA input line offset

    @param[in] uiLineOffset DMA input channel (DMA to DIS) line offset.

    @return None.
*/
void dis_setDmaInOffset(UINT16 uiLineOffset)
{
	T_DIS_INPUT_CHANNEL_REGISTER_2 LocalReg;

	LocalReg.reg = DIS_GETREG(DIS_INPUT_CHANNEL_REGISTER_2_OFS);
	LocalReg.bit.DRAM_E_OFSI = uiLineOffset >> 2;

	DIS_SETREG(DIS_INPUT_CHANNEL_REGISTER_2_OFS, LocalReg.reg);
}

/**
    DIS Set Output Starting Addresses

    Set two DMA output starting addresses

    @param[in] uiAddr0 DMA output channel (DIS to DMA) starting address 0.

    @return None.
*/
void dis_setDmaOutAddr(UINT32 uiAddr0)
{
	T_DIS_OUTPUT_CHANNEL_REGISTER_0 LocalReg0;

	if(uiAddr0 == 0) {
		DBG_ERR("Output addresses cannot be 0x0!\r\n");
	} else {
		LocalReg0.reg = DIS_GETREG(DIS_OUTPUT_CHANNEL_REGISTER_0_OFS);
		LocalReg0.bit.DRAM_M_SAO = uiAddr0 >> 2;
		DIS_SETREG(DIS_OUTPUT_CHANNEL_REGISTER_0_OFS, LocalReg0.reg);
	}
}


/**
    DIS Set Motion Estimation

    Set DIS motion estimation parameters

    @param[in] mot Motion parameters including search range and block location

    @return None.
*/
void dis_setMotionEst(MOTION_EST Mot)
{
	T_DIS_ME_REGISTER1 reg1;
	T_DIS_CREDITS reg2;

    if(nvt_get_chip_id() == CHIP_NA51055) {
        T_DIS_ME_REGISTER0 reg0;
        reg0.reg = DIS_GETREG(DIS_ME_REGISTER0_OFS);
        reg0.bit.DIS_SR = Mot.SearchRagne;
    	reg0.bit.DIS_BLOCKSIZE = Mot.BlockSize;
    	reg0.bit.DIS_LUT = Mot.Lut;
        DIS_SETREG(DIS_ME_REGISTER0_OFS, reg0.reg);
    }else{
        T_DIS_ME_REGISTER0_528 reg0;
        reg0.reg = DIS_GETREG(DIS_ME_REGISTER0_OFS_528);
        reg0.bit.DIS_SR = Mot.SearchRagne;
    	reg0.bit.DIS_BLOCKSIZE = Mot.BlockSize;
    	reg0.bit.DIS_LUT = Mot.Lut;
        reg0.bit.ETH_SUBIN = Mot.SubIn;
        reg0.bit.ETH_SUBIN_SEL = Mot.SubInSel;
        DIS_SETREG(DIS_ME_REGISTER0_OFS_528, reg0.reg);
    }

	reg1.reg = DIS_GETREG(DIS_ME_REGISTER1_OFS);
	reg2.reg = DIS_GETREG(DIS_CREDITS_OFS);

	reg1.bit.DIS_SCROFFS   = Mot.Thrshs.uiScrOffs;
	reg1.bit.DIS_SCRTHR    = Mot.Thrshs.uiScrThrh;
	reg1.bit.CENTER_SCRTHR = Mot.Thrshs.uiCenScrThrh;
	reg1.bit.FTCNT_THR     = Mot.Thrshs.uiFeaCntThrh;

	reg2.bit.DIS_CREDIT0 = Mot.Credits.uiCredit0;
	reg2.bit.DIS_CREDIT1 = Mot.Credits.uiCredit1;
	reg2.bit.DIS_CREDIT2 = Mot.Credits.uiCredit2;

	DIS_SETREG(DIS_ME_REGISTER1_OFS, reg1.reg);
	DIS_SETREG(DIS_CREDITS_OFS, reg2.reg);
}

/**
    DIS Set MDS Dimension

    Set DIS MDS dimension parameters

    @param[in] dim Dimension information including block/MDS and total MDS number

    @return None.
*/
void dis_setMDSDim(MDS_DIM Dim)
{
	T_DIS_ME_REGISTER0 reg0;

	reg0.reg = DIS_GETREG(DIS_ME_REGISTER0_OFS);
	if (Dim.uiBlkNumH > BLKHMAX) {
		Dim.uiBlkNumH = BLKHMAX;
		DBG_WRN("Horizontal Block number over spec!!\r\n");
	} else if (Dim.uiBlkNumH < 1) {
		Dim.uiBlkNumH = 1;
		DBG_WRN("Horizontal Block number over spec!!\r\n");
	}

	if (Dim.uiBlkNumV > BLKVMAX) {
		Dim.uiBlkNumV = BLKVMAX;
		DBG_WRN("Vertical Block number over spec!!\r\n");
	} else if (Dim.uiBlkNumV < 1) {
		Dim.uiBlkNumV = 1;
		DBG_WRN("Vertical Block number over spec!!\r\n");
	}

	if (Dim.uiMdsNum > MDSNUMMAX) {
		Dim.uiMdsNum = MDSNUMMAX;
		DBG_WRN("MDS number over spec!!\r\n");
	} else if (Dim.uiMdsNum < 1) {
		Dim.uiMdsNum = 1;
		DBG_WRN("MDS number over spec!!\r\n");
	}

	reg0.bit.BLK_HORI = Dim.uiBlkNumH - 1;
	reg0.bit.BLK_VERT = Dim.uiBlkNumV - 1;
	reg0.bit.NUMMDS2DO = Dim.uiMdsNum - 1;

	DIS_SETREG(DIS_ME_REGISTER0_OFS, reg0.reg);
}

/**
    DIS Set MDS Starting Addresses

    Set DIS MDS starting addresses for detection area allocation

    @param[in] SA MDS starting addresses

    @return None.
*/
void dis_setMDSAddr(MOTION_VECTOR *pStartAddr)
{
	T_DIS_START_ADDRESS_REGISTER_0 reg0;
	UINT8 idx;

	for (idx = 0; idx < MDSNUMMAX; idx++) {
		reg0.reg = DIS_GETREG(DIS_START_ADDRESS_REGISTER_0_OFS);
		reg0.bit.DIS_R00_ADDRX = pStartAddr[idx].iX;
		reg0.bit.DIS_R00_ADDRY = pStartAddr[idx].iY;
		DIS_SETREG((DIS_START_ADDRESS_REGISTER_0_OFS + (idx << 2)), reg0.reg);
	}
}

/**
    DIS Set MDS offset

    Set DIS MDS offset from source to reference

    @param[in] OFS MDS offset

    @return None.
*/
void dis_setMDSOfs(MOTION_VECTOR *pOffset)
{
	T_DIS_OFFSET_REGISTER_00 reg0;
	UINT8 idx;

	for (idx = 0; idx < MDSNUMMAX; idx += 2) {
		reg0.reg = DIS_GETREG(DIS_OFFSET_REGISTER_00_OFS);
		reg0.bit.MDSOFS_X00 = pOffset[idx].iX + 128;
		reg0.bit.MDSOFS_Y00 = pOffset[idx].iY + 128;
		reg0.bit.MDSOFS_X01 = pOffset[idx + 1].iX + 128;
		reg0.bit.MDSOFS_Y01 = pOffset[idx + 1].iY + 128;
		DIS_SETREG((DIS_OFFSET_REGISTER_00_OFS + (idx << 1)), reg0.reg);
	}
}

/**
    DIS Get MDS Dimension

    Get DIS MDS dimension setting

    @return MDS dimension including total MDS number and blocks/MDS.
*/
MDS_DIM dis_getMDSDim(void)
{
	T_DIS_ME_REGISTER0 reg0;
	MDS_DIM Dim;

	reg0.reg = DIS_GETREG(DIS_ME_REGISTER0_OFS);
	Dim.uiBlkNumH = reg0.bit.BLK_HORI + 1;
	Dim.uiBlkNumV = reg0.bit.BLK_VERT + 1;
	Dim.uiMdsNum = reg0.bit.NUMMDS2DO + 1;

	return Dim;
}

/**
    DIS Get DMA output starting address

    Get specifed DMA output channel (DIS to DMA) starting address

    @param[in] bufId output ppb ID

    @return specifed DMA output channel (DIS to DMA) starting address
*/
UINT32 dis_getDmaOutAddr(DIS_MOTION_BUFID BufId)
{
	UINT32 uiAddr;
#if DIS_IOREMAP_IN_KERNEL
	UINT32 uiPhyAddr,uiOutSize;
#endif	
	
#if (DIS_DMA_CACHE_HANDLE == 1)
#if DIS_IOREMAP_IN_KERNEL
	uiPhyAddr = g_uiOutAdd0;
	uiOutSize = DIS_OUTPUT_BUF_SIZE;
	uiAddr = dis_platform_pa2va_remap(uiPhyAddr, uiOutSize, 2);
#else
	uiAddr = g_uiOutAdd0;
#endif

	if (uiAddr != 0) {
		dis_platform_dma_flush(uiAddr, DIS_OUTPUT_BUF_SIZE);
	}
#else
	UINT32 uiPhyAddr;
	T_DIS_OUTPUT_CHANNEL_REGISTER_0 LocalReg0;
	
	LocalReg0.reg = DIS_GETREG(DIS_OUTPUT_CHANNEL_REGISTER_0_OFS);
	uiPhyAddr = LocalReg0.bit.DRAM_M_SAO << 2;

	uiAddr = uiPhyAddr;
#endif

	return uiAddr;
}

/**
    DIS Get DMA input starting address

    Get specifed DMA input channel starting address

    @param[in] bufId input ppb ID

    @return specifed DMA input channel starting address
*/
UINT32 dis_getDmaInAddr(DIS_EDGE_BUFID BufId)
{
	UINT32 uiAddr;

	#if (DIS_DMA_CACHE_HANDLE == 1)
	if (BufId == DIS_EDGE_BUF0) {
		uiAddr = g_uiInAdd0;
	} else {
		uiAddr = g_uiInAdd1;
	}
	#else
	UINT32 uiPhyAddr;
	T_DIS_INPUT_CHANNEL_REGISTER_0 LocalReg0;
	T_DIS_INPUT_CHANNEL_REGISTER_1 LocalReg1;

	if (BufId == DIS_EDGE_BUF0) {
		LocalReg0.reg = DIS_GETREG(DIS_INPUT_CHANNEL_REGISTER_0_OFS);
		uiPhyAddr = LocalReg0.bit.DRAM_E_SAI0 << 2;
	} else {
		LocalReg1.reg = DIS_GETREG(DIS_INPUT_CHANNEL_REGISTER_1_OFS);
		uiPhyAddr = LocalReg1.bit.DRAM_E_SAI1 << 2;
	}
	uiAddr = uiPhyAddr;
	#endif

	return uiAddr;
}

STR_DIS_REG dis_allocateMotionArea(SIZE_CONF *pSizeConf)
{
	const UINT32 spare = 32;

	UINT32 uiMdsHori3, uiMdsHori4, uiBlkHori3, uiBlkHori4;
	UINT32  uiMdsHori, uiMdsVert, uiBlkHori, uiBlkVert;
	UINT32  uiCurWidth, uiCurHeight;
	UINT32  uiMdsHoriMax, uiMdsVertMax, uiBlkHoriMax, uiBlkVertMax;
	UINT32  uiAddH[32], uiAddV[1];
	UINT32  cnt, num, cntV;
	UINT32  uiSrc2RefX, uiSrc2RefY;
	UINT32  uiBlkHSize, uiBlkVSize;

	g_ModeInfo.PpbInfo.bPpbiEn = DISABLE;
	g_ModeInfo.PpbInfo.uiPpbiIni = 0;
	g_ModeInfo.PpbInfo.bPpboEn = DISABLE;

	g_ModeInfo.MotSet.SearchRagne = DIS_APLY_SEARCH_RNG;
	g_ModeInfo.MotSet.BlockSize   = DIS_APLY_BLOCK_SZ;
	g_ModeInfo.MotSet.Lut         = DIS_APLY_LUT;
    if(nvt_get_chip_id() == CHIP_NA51055) {
    }else{
        g_ModeInfo.MotSet.SubIn       = DIS_APLY_SUBIN;
        g_ModeInfo.MotSet.SubInSel    = DIS_APLY_SUBIN_SEL;
        if(DIS_APLY_SUBIN == DIS_SUBIN_2x) {
            pSizeConf->uiWidth /= 2;
            pSizeConf->uiHeight /= 2;
        }else if(DIS_APLY_SUBIN == DIS_SUBIN_4x) {
            pSizeConf->uiWidth /= 4;
            pSizeConf->uiHeight /= 4;
        }
    }

	g_ModeInfo.MotSet.Thrshs.uiScrOffs    = 0x0;
	g_ModeInfo.MotSet.Thrshs.uiScrThrh    = 0x0;
	g_ModeInfo.MotSet.Thrshs.uiCenScrThrh = 0x10;
	g_ModeInfo.MotSet.Thrshs.uiFeaCntThrh = 0x13;

	g_ModeInfo.MotSet.Credits.uiCredit0 = 0x14;
	g_ModeInfo.MotSet.Credits.uiCredit1 = 0x14;
	g_ModeInfo.MotSet.Credits.uiCredit2 = 0x14;

	uiSrc2RefX = 31;
	uiSrc2RefY = 31;
	if (g_ModeInfo.MotSet.SearchRagne == DIS_SR_16) {
		uiSrc2RefX = 15;
		uiSrc2RefY = 15;
	}

	uiBlkHSize = 32;
	uiBlkVSize = 32;
	if (g_ModeInfo.MotSet.BlockSize == DIS_BLKSZ_64x48) {
		uiBlkHSize = 64;
		uiBlkVSize = 48;
	}

	uiMdsHoriMax = MDSNUMMAX;
	uiMdsVertMax = 1;
	uiBlkHoriMax = BLKHMAX;
	uiBlkVertMax = BLKVMAX;

	// Calculate MDS and block numumbers
	if (pSizeConf->uiWidth  > (UINT32)((uiSrc2RefX + 1) * 2 + uiBlkHSize)) {
		uiCurWidth  = pSizeConf->uiWidth  - (uiSrc2RefX + 1) * 2;
		cnt = uiCurWidth / uiBlkHSize;
		if (cnt > (uiMdsHoriMax * uiBlkHoriMax)) {
			uiMdsHori = uiMdsHoriMax;
			uiBlkHori = uiBlkHoriMax;
		} else {
		    if(cnt == 5) {                        //cnt == 5 will bring bug, then DBG_WRN("Horizontal Block number over spec!!\r\n");
                cnt = 4;
		    }
			uiMdsHori3 = (cnt/3)==0 ? 1 : cnt/3;
			uiMdsHori4 = (cnt/4)==0 ? 1 : cnt/4;
			uiBlkHori3 = cnt / uiMdsHori3;
			uiBlkHori4 = cnt / uiMdsHori4;
			if((uiMdsHori4*uiBlkHori4 >= uiMdsHori3*uiBlkHori3) || (uiMdsHori3 > MDSNUMMAX)) {
				uiMdsHori = uiMdsHori4;
				uiBlkHori = uiBlkHori4;
			}
			else {
				uiMdsHori = uiMdsHori3;
				uiBlkHori = uiBlkHori3;
			}
			/*
			uiMdsHori = 1 + ((cnt - 1) / uiBlkHoriMax);
			uiBlkHori = cnt / uiMdsHori;
			*/
		}
	} else {
		//uiCurWidth = 0;
		uiMdsHori = 0;
		uiBlkHori = 0;
	}
	//DBG_ERR("uiMdsHori %d uiBlkHori %d\r\n",uiMdsHori, uiBlkHori);

	if (pSizeConf->uiHeight  > (UINT32)((uiSrc2RefY + 1) * 2 + uiBlkVSize)) {
		uiCurHeight = pSizeConf->uiHeight - (uiSrc2RefY + 1) * 2;
		cnt = uiCurHeight / uiBlkVSize;
		if (cnt > (uiMdsVertMax * uiBlkVertMax)) {
			uiMdsVert = uiMdsVertMax;
			uiBlkVert = uiBlkVertMax;
		} else {
			uiMdsVert = 1 + ((cnt - 1) / uiBlkVertMax);
			uiBlkVert = cnt / uiMdsVert;
		}
	} else {
		//uiCurHeight = 0;
		uiMdsVert = 0;
		uiBlkVert = 0;
	}
	//DBG_ERR("uiMdsVert %d uiBlkVert %d\r\n",uiMdsVert, uiBlkVert);

	// Allocate basic MDSs
//    spare = 0; //32;
	for (cnt = 0; cnt < uiMdsHoriMax; cnt++) {
		uiAddH[cnt] = spare;
	}
	//uiAddH[0] = uiCenterH - uiMdsHori*uiBlkHori*BLK_HSIZE/2 - (g_ModeInfo.MotSet.uiSrc2RefX + 1);
	for (cnt = 1; cnt < uiMdsHori; cnt++) {
		uiAddH[cnt] = uiAddH[cnt - 1] + uiBlkHori * uiBlkHSize;
	}
	//check
	for (cnt = 0; cnt < uiMdsHori; cnt++) {
		if (uiAddH[cnt] > pSizeConf->uiWidth - uiBlkHori * uiBlkHSize - (uiSrc2RefX + 1) * 2) {
			uiAddH[cnt] = pSizeConf->uiWidth - uiBlkHori * uiBlkHSize - (uiSrc2RefX + 1) * 2;
		}
		if (uiAddH[cnt] < spare) {
			uiAddH[cnt] = spare;
		}
	}

	for (cnt = 0; cnt < uiMdsVertMax; cnt++) {
		uiAddV[cnt] = spare;
	}
	//uiAddV[0] = uiCenterV - uiMdsVert*uiBlkVert*BLK_VSIZE/2 - (g_ModeInfo.MotSet.uiSrc2RefY + 1);

	//check
	for (cnt = 0; cnt < uiMdsVert; cnt++) {
		if (uiAddV[cnt] > pSizeConf->uiHeight - uiBlkVert * uiBlkVSize - (uiSrc2RefY + 1) * 2) {
			uiAddV[cnt] = pSizeConf->uiHeight - uiBlkVert * uiBlkVSize - (uiSrc2RefY + 1) * 2;
		}
		if (uiAddV[cnt] < spare) {
			uiAddV[cnt] = spare;
		}
	}

	// MDS addresses
	num = 0;
	for (cntV = 0; cntV < uiMdsVertMax; cntV++) {
		for (cnt = 0; cnt < uiMdsHoriMax; cnt++) {
			g_ModeInfo.MdsAddr[num].iX = uiAddH[cnt];
			g_ModeInfo.MdsAddr[num].iY = uiAddV[cntV];
			num++;
		}
	}
	for (cnt = num; cnt < MDSNUMMAX; cnt++) {
		g_ModeInfo.MdsAddr[cnt].iX = spare;
		g_ModeInfo.MdsAddr[cnt].iY = spare;
	}

	//MDS offset
	for (cnt = 0; cnt < MDSNUMMAX; cnt++) {
		g_ModeInfo.MdsOfs[cnt].iX = 0;
		g_ModeInfo.MdsOfs[cnt].iY = 0;
	}

	g_ModeInfo.MdsDim.uiBlkNumH = uiBlkHori;
	g_ModeInfo.MdsDim.uiBlkNumV = uiBlkVert;
	//g_ModeInfo.MdsDim.uiMdsNum = num;//uiMdsHoriMax*uiMdsVertMax;
	g_ModeInfo.MdsDim.uiMdsNum = uiMdsHori;

	return (g_ModeInfo);
}

#if 0
/**
    DIS check status is idle or busy now

    @return idle:0/busy:1
*/
BOOL dis_checkBusy(void)
{
	//T_DEBUG_STATUS_REGISTER_0 LocalReg;

	//LocalReg.reg = IPE_GETREG(DEBUG_STATUS_REGISTER_0_OFS);
	//if(LocalReg.bit.IPESTATUS)
	//    return 1;
	//else
	return 0;
}

/*
UINT dis_getSearchRange(void)
{
    T_DIS_ME_REGISTER reg0;

    reg0.reg = DIS_GETREG(DIS_ME_REGISTER_OFS);

    return reg0.bit.DIS_SR;
}
*/

/**
    DIS Get Single Motion Result

    Get DIS single motion estimation result

    @param[out] MotResult the specified motion vector result.
    @param[in] num the index of the motion vector to get.
    @param[in] MotionID the buffer ID of which the motion vector is extracted from.

    @return None.
*/
void dis_getSingleMotResult(MOTION_INFOR *MotResult, UINT32 num, DIS_MOTION_BUFID MotionID)
{
	UINT32 MVAddr;
	MOTION_INFOR MotInfo;

	MVAddr = dis_getDmaOutAddr(MotionID) << 2;
	MotInfo = dis_getSingleMotInfo(MVAddr, num);
	dis_convSAD(&MotInfo);
	dis_convMvRange(&MotInfo);
	MotResult = &MotInfo;
}

/**
    DIS SAD Conversion

    Convert SAD of motion vectors into a range of [0 ~ 100]

    @param MotInfo the motion vectors to be converted.

    @return None.
*/
void dis_convSAD(MOTION_INFOR *MotInfo)
{
	T_DIS_MODE_REGISTER_0 reg0;

	reg0.reg = DIS_GETREG(DIS_MODE_REGISTER_0_OFS);
	if (reg0.bit.DIS_MATCH_OP == DIS_MATCHOP_AND) {
		if ((MotInfo->valid == 1) && (MotInfo->cnt > 0)) {
			MotInfo->sad = MotInfo->sad * 100 / MotInfo->cnt;
		}
	}
}

/**
    DIS Motion Vector Range Conversion

    Convert the range of motion vectors from [0 ~ 63] to [-32 ~ +31]

    @param MotInfo the motion vectors to be converted.

    @return None.
*/
void dis_convMvRange(MOTION_INFOR *MotInfo)
{
	MotInfo->x -= 32;
	MotInfo->y -= 32;
}

/**
    DIS Get Single Motion Vector Information

    Get specifed single motion vector information

    @param[in] addr output buffer starting address
    @param[in] index specifed motion vector index

    @return motion information of the specified motion vector
*/
MOTION_INFOR dis_getSingleMotInfo(UINT32 addr, UINT16 index)
{
	MOTION_INFOR tmpMot;
	UINT32 motInfo0, motInfo1;
	UINT32 blocksz;

	blocksz = 64 * 48;
	motInfo0 = DIS_GETDATA(addr, (index << 3));
	motInfo1 = DIS_GETDATA(addr, ((index << 3) + 4));

	tmpMot.x = motInfo0 & DIS_MVX;         //5 bits
	tmpMot.y = (motInfo0 & DIS_MVY) >> 8;  //5 bits
	tmpMot.sad = (motInfo0 & DIS_SAD) >> 16; //12 bits
	tmpMot.cnt = motInfo1 & DIS_CNT;       //12 bits
	if (((tmpMot.cnt * 10) < blocksz) || ((tmpMot.cnt * 10) > (blocksz * 9))) { //<10% or >90%
		tmpMot.valid = 0;
	} else {
		tmpMot.valid = 1;
	}

	return tmpMot;
}

/**
    Get Integral motion vector, STILL mode
*/
void dis_getD3SIMV(MOTION_INFOR *disMV, MOTION_VECTOR *intMV, MOTION_INFOR *gv)
{
	int i, j, k;
	int max;
	//MOTION_INFOR ogmv;
	int sadPM[16]; // 16 blocks per MDS top
	MOTION_INFOR Mmv[MAXMDSNUM]; // Motion vectors of 8 MDSs //DIS_MV Mmv[4]; // Motion vectors of 4 MDSs
	//char fname[80];
	//int maxCIx, maxCIy;
	//int gmvScore=0;
	MOTION_INFOR wmv[MAXMDSNUM]; // weighted MV of Mmvs
	int lvec, upbound;
	UINT32 /*reg,*/ offs; //,numMDS2do;
	int cnt = 1;
	UINT8 blkpMds, numMDS2do;
	MDS_DIM mdsDim;

	mdsDim = dis_getMDSDim();
	blkpMds = mdsDim.blkNum_hori * mdsDim.blkNum_vert;
	numMDS2do = mdsDim.MDSNum;
	// FLOW STARTS
	{
		//printf("DIS is switched ON..............................frame# %d\n", sc);
		//DBG_ERR(("..............DIS is switched ON\r\n"));

		//get mv of each MDS --------------------------------------------
		DBG_ERR(("Mmv:\r\n"));//Print Mmv
		for (k = 0; k < (2 * numMDS2do); k++) { //for(k=0; k<(8+numMDS2do); k++){
			Mmv[k].valid = 0;
			offs = k * blkpMds;
			for (i = 0; i < 16; i++) {
				sadPM[i] = disMV[offs + i].sad;
			}
			for (i = 0; i < 15; i++)
				for (j = i + 1; j < 16; j++)
					if (disMV[offs + i].valid == 1 && disMV[offs + j].valid == 1)
						if (disMV[offs + i].x == disMV[offs + j].x && disMV[offs + i].y == disMV[offs + j].y) {
							sadPM[i] += sadPM[j];
							sadPM[j] = 0;
						}
			max = 0;
			for (i = 0; i < 16; i++) {
				if (disMV[offs + i].valid == 1 && sadPM[i] > max) {
					max = sadPM[i];
					Mmv[k].x = disMV[offs + i].x;
					Mmv[k].y = disMV[offs + i].y;
					Mmv[k].sad = max;
					Mmv[k].valid = 1;
				}
				//Print disMV
				if (disMV[offs + i].valid == 1) {
					DBG_ERR(("     %d %d %d\r\n", disMV[offs + i].x, disMV[offs + i].y, disMV[offs + i].sad));
				}
			}

			//Print Mmv
			if (Mmv[k].valid == 0) {
				DBG_ERR(("   n/a n/a n/a n/a\r\n"));
			} else {
				DBG_ERR(("   %d %d %d\r\n", Mmv[k].x, Mmv[k].y, Mmv[k].sad));
			}

		} //for(k)

		//get gmv --------------------------------------------------------
		DBG_ERR(("wmv:\r\n")); //Print wmv
		for (i = 0; i < (2 * numMDS2do); i++) {
			if (Mmv[i].valid == 1) {
				wmv[i] = Mmv[i];
				wmv[i].cnt = 1;
				for (j = 0; j < (2 * numMDS2do); j++) {
					// Accumulate sad of each other Mmv if (|x1-x2| + |y1-y2|) < upper_bound
					if ((i != j) && (Mmv[j].valid == 1)) {
						// Decide upper bound
						lvec = (DIS_ABS(Mmv[i].x) > DIS_ABS(Mmv[i].y)) ? DIS_ABS(Mmv[i].x) : DIS_ABS(Mmv[i].y);
						if (lvec < 8) {
							upbound = 2;
						} else if (lvec < 16) {
							upbound = 4;
						} else { //if (lvec<32)
							upbound = 6;
						}
						// Accumulation
						if ((DIS_ABS(Mmv[i].x - Mmv[j].x) + DIS_ABS(Mmv[i].y - Mmv[j].y)) < upbound) {
							wmv[i].sad += Mmv[j].sad;
							wmv[i].x += Mmv[j].x;
							wmv[i].y += Mmv[j].y;
							wmv[i].cnt++;
						}
					}
				}
				//DBG_ERR(("      x%d y%d %d\r\n",wmv[i].x,wmv[i].y,wmv[i].cnt));
				cnt = (int)wmv[i].cnt;
				// Average Mmv.x & Mmv.y
				if (((10 * DIS_ABS(wmv[i].x) / wmv[i].cnt) % 10) > 4) {
					if (wmv[i].x > 0) {
						wmv[i].x = wmv[i].x / wmv[i].cnt + 1;
					} else {
						wmv[i].x = wmv[i].x / wmv[i].cnt - 1;
					}
				} else {
					wmv[i].x = wmv[i].x / wmv[i].cnt;
				}
				if (((10 * DIS_ABS(wmv[i].y) / wmv[i].cnt) % 10) > 4) {
					if (wmv[i].y > 0) {
						wmv[i].y = wmv[i].y / wmv[i].cnt + 1;
					} else {
						wmv[i].y = wmv[i].y / wmv[i].cnt - 1;
					}
				} else {
					wmv[i].y = wmv[i].y / wmv[i].cnt;
				}
				// Weights favoring large MV
				lvec = DIS_ABS(wmv[i].x);
				if (lvec >= 8 && lvec < 16) {
					wmv[i].sad = wmv[i].sad + wmv[i].cnt * 32;
				} else if (lvec >= 16 && lvec < 24) {
					wmv[i].sad = wmv[i].sad + wmv[i].cnt * 64;
				} else if (lvec >= 24 && lvec < 32) {
					wmv[i].sad = wmv[i].sad + wmv[i].cnt * 96;
				}
				lvec = DIS_ABS(wmv[i].y);
				if (lvec >= 8 && lvec < 16) {
					wmv[i].sad = wmv[i].sad + wmv[i].cnt * 32;
				} else if (lvec >= 16 && lvec < 24) {
					wmv[i].sad = wmv[i].sad + wmv[i].cnt * 64;
				} else if (lvec >= 24 && lvec < 32) {
					wmv[i].sad = wmv[i].sad + wmv[i].cnt * 96;
				}

				//fprintf(fp, "idx %d\twsad %d\tcnt %d\tavgx %d\tavgy %d\n", i, wmv[i].sad, wmv[i].cnt, wmv[i].x, wmv[i].y);
				DBG_ERR(("   %d %d %d %d\r\n", wmv[i].sad, wmv[i].x, wmv[i].y, wmv[i].cnt));
			} else {
				wmv[i].valid = 0;
			}
		} //for(i)
		max = 0;
		for (i = 0; i < (2 * numMDS2do); i++) {
			if (wmv[i].valid == 1) {
				//Now combine identical ones
				for (j = 0; j < (2 * numMDS2do); j++) {
					if (i != j && wmv[i].x == wmv[j].x && wmv[i].y == wmv[j].y) {
						wmv[i].sad += wmv[j].sad;
						wmv[j].sad = 0;
					}
				}
				// pick the Mmv with the highest sad as GMV
				if (wmv[i].sad > max) {
					max = wmv[i].sad;
					gv->x = wmv[i].x;
					gv->y = wmv[i].y;
					gv->sad = max;
					//gmv.x = wmv[i].x;
					//gmv.y = wmv[i].y;
					//gmv.sad = max;
				}
			}
		} //for(i)

		//No Panning detection

		//get imv --------------------------------------------------------
		intMV->x += gv->x;
		intMV->y += gv->y;
		//intMV->x += gmv.x;
		//intMV->y += gmv.y;
		//DBG_ERR(("D3S: g %d %d  i %d %d\r\n",gmv.x,gmv.y,imv.x,imv.y));
	}
}

/**
    2's complement conversion
*/
void dis_sign_conv(MOTION_INFOR *MVarray)
{
	UINT32 mask_sign, mask_absv;
	int sign, value;
	int i;

	mask_sign = 0x00000010;
	mask_absv = 0x0000000F;

	for (i = 0; i < 4 * blkpMds; i++) {
		sign = (MVarray[i].x & mask_sign) >> 4;
		if (sign == 1) {
			value = (MVarray[i].x & mask_absv);
			value = ((~value)&mask_absv) + 1;
			MVarray[i].x = -(value & mask_absv);
			value = (MVarray[i].y & mask_absv);
			value = ((~value)&mask_absv) + 1;
			MVarray[i].y = -(value & mask_absv);
		}
	}

}
#endif

#if 0
UINT32 dis_accessOisOnOff(DIS_ACCESS_TYPE AccType, UINT32 uiIsOisOn)
{
	UINT32 retV = uiIsOisOn;
	DBG_MSG("AccOis:%d,onOff=%d\r\n", AccType, uiIsOisOn);

	if (DIS_ACCESS_SET == AccType) {
		g_uiDisIsOisOn = uiIsOisOn;
	} else if (DIS_ACCESS_GET) {
		retV = g_uiDisIsOisOn;
	} else {
		DBG_ERR("Unkown accType=%d\r\n", AccType);
		retV = DIS_ACCESS_MAX;
	}
	return retV;
}

UINT32 dis_setOisDetMotVec(INT32 iX, INT32 iY)
{
	DBG_MSG("SetOis x,y=%d,%d\r\n", iX, iY);
	if (FALSE == g_uiDisIsOisOn) {
		DBG_ERR("OIS is OFF\r\n");
		return FALSE;
	}
	g_iDisOisX = iX;
	g_iDisOisY = iY;
	return TRUE;
}

static void dis_getOisDetMotVec(DIS_SGM *pIV)
{
	DBG_MSG("OisOn:X=%d,Y=%d\r\n", g_iDisOisX, g_iDisOisY);
	pIV->pCurMv->iX = g_iDisOisX;
	pIV->pCurMv->iY = g_iDisOisY;
}
#endif

/*
    DIS Interrupt Service Routine

    DIS interrupt service routine

    @return None.
*/
#define DIS_EMU_SHOW_INTS   0

void dis_isr(void)
{
	UINT32 DisIntStatus;
	UINT32 Ptn;

	DisIntStatus = dis_getIntrStatus();
	DisIntStatus = DisIntStatus & dis_getIntEnable();

	if (DisIntStatus == 0) {
		return;
	}
#if DIS_EMU_SHOW_INTS
	debug_msg("\n\r*** DIS interrupt status 0x%x ***\n\r", DisIntStatus);
#else
	dis_clrIntrStatus(DisIntStatus);
#endif

	Ptn = 0;
	if (DisIntStatus & DIS_INT_FRM) {
		Ptn |= FLGPTN_DIS_FRAMEEND;
		g_uiDisFcnt++;
		//dis_clrIntrStatus(DIS_INT_FRM);
	}

	if (Ptn != 0) {
		iset_flg(FLG_ID_DIS, Ptn);
	}

	if (pDISIntCB != NULL) {
		if (DisIntStatus) {
			pDISIntCB(DisIntStatus);
		}
	}
}

/**
    DIS Lock Function

    Lock DIS engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER dis_lock(void)
{
	ER erReturn;

	erReturn = SEM_WAIT(SEMID_DIS);

	if (erReturn != E_OK) {
		return erReturn;
	}

	gDisLockStatus = TASK_LOCKED;

	return E_OK;
}

/**
    DIS Unlock Function

    Unlock DIS engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER dis_unlock(void)
{
	gDisLockStatus = NO_TASK_LOCKED;
	SEM_SIGNAL(SEMID_DIS);
	return E_OK;
}

/**
    DIS Attach Function

    Pre-initialize driver required HW before driver open
    \n This function should be called before calling any other functions

    @return None
*/
static void dis_attach(void)
{
	dis_platform_clk_enable();

}

/**
    DIS Detach Function

    De-initialize driver required HW after driver close
    \n This function should be called before calling any other functions

    @return None
*/
static void dis_detach(void)
{
	dis_platform_clk_disable();
}

ER dis_setClockRate(UINT32 uiClock)
{
	dis_platform_set_clock_rate(uiClock);

	return E_OK;
}

/**
    DIS get operation clock

    Get DIS clock selection.

    @param None.

    @return UINT32 DIS operation clock rate.
*/
static UINT32 dis_getClockRate(void)
{
	UINT32 clk_freq_mhz;

	clk_freq_mhz = dis_platform_get_clock_rate();
	
	return clk_freq_mhz;
}

/**
    DIS Open Driver

    Open DIS engine
    \n This function should be called after dis_attach()

    @param[in] pObjCB DIS open object

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER dis_open(DIS_OPENOBJ *pObjCB)
{
	ER erReturn;
	UINT32 clk_freq_mhz;
	//DBG_IND("DIS Ver=%s,Build Date=%s\r\n",gDisVerInfo,gDisBuildDate);

	// Lock semaphore
	erReturn = dis_lock();
	if (erReturn != E_OK) {
		return erReturn;
	}

	// Check state-machine
	erReturn = dis_chkStateMachine(DIS_ENGINE_OPEN, FALSE);
	if (erReturn != E_OK) {
		// release resource when stateMachine fail
		dis_unlock();
		return erReturn;
	}

	// Power on module
	//pmc_turnonPower(PMC_MODULE_DIS);
	
	// Select PLL clock source & enable PLL clock source	
	clk_freq_mhz = dis_getClockRate();
	if (clk_freq_mhz == 0) {
		dis_setClockRate(240);
	} else {
		dis_setClockRate(clk_freq_mhz);
	}

	// Attach
	dis_attach();

	// Disable Sram shut down
	//pll_enableSramShutDown(DIS_RSTN); // mismatch
	//pll_disableSramShutDown(DIS_RSTN);
	nvt_disable_sram_shutdown(DIS_SD);

	// Enable engine interrupt
	//drv_enableInt(DRV_INT_DIS);
	dis_enableInt(ENABLE, DIS_INT_FRM);

	// Clear engine interrupt status
	dis_clrIntrStatus(DIS_INT_ALL);

	// Clear SW flag
	erReturn = dis_clrFrameEnd();
	if (erReturn != E_OK) {
		return erReturn;
	}

	// Hook call-back function
	pDISIntCB   = pObjCB->FP_DISISR_CB;

	// SW reset
	dis_clr(1);
	dis_clr(0);

	// Update state-machine
	dis_chkStateMachine(DIS_ENGINE_OPEN, TRUE);

	dis_platform_request_irq((VOID *) dis_isr);

	return E_OK;
}

/*
    DIS Get Open Status

    Check if DIS engine is opened

    @return
        - @b FALSE: engine is not opened
        - @b TRUE: engine is opened
*/
BOOL dis_isOpened(void)
{
	if (gDisLockStatus == TASK_LOCKED) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
    DIS Close Driver

    Close DIS engine
    \n This function should be called before dis_detach()

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER dis_close(void)
{
	ER erReturn;

	// Check state-machine
	erReturn = dis_chkStateMachine(DIS_ENGINE_CLOSE, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	// Check semaphore
	if (gDisLockStatus != TASK_LOCKED) {
		return E_SYS;
	}

	// Diable engine interrupt
	//drv_disableInt(DRV_INT_DIS);

	// Enable Sram shut down
	//pll_disableSramShutDown(DIS_RSTN);//mismatch
	//pll_enableSramShutDown(DIS_RSTN);
	nvt_enable_sram_shutdown(DIS_SD);

	// Detach
	dis_detach();

	// Unhook call-back function
#if (DRV_SUPPORT_IST == ENABLE)
	pfIstCB_DIS = NULL;
#else
	pDISIntCB   = NULL;
#endif

	// Update state-machine
	dis_chkStateMachine(DIS_ENGINE_CLOSE, TRUE);

	// Unlock semaphore
	erReturn = dis_unlock();
	if (erReturn != E_OK) {
		return erReturn;
	}

	return E_OK;
}

/**
    DIS Pause Operation

    Pause DIS engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER dis_pause(void)
{
	ER erReturn;

	erReturn = dis_chkStateMachine(DIS_ENGINE_SET2PAUSE, FALSE);

	if (erReturn != E_OK) {
		return erReturn;
	}

	//dis_clrFrameEnd();
	//dis_waitFrameEnd();
	//dis_waitFrameEnd(TRUE);
	//dis_clrIntrStatus(DIS_INT_ALL);
	dis_enable(DISABLE);

	dis_resetFrameCnt();

	dis_chkStateMachine(DIS_ENGINE_SET2PAUSE, TRUE);
	return E_OK;
}

/**
    DIS Start Operation

    Start DIS engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER dis_start(void)
{
	ER erReturn;

	erReturn = dis_chkStateMachine(DIS_ENGINE_SET2RUN, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	//dis_clr(ENABLE);
	//dis_clr(DISABLE);
	dis_setLoad(DIS_START_LOAD);
	dis_enable(ENABLE);
	//dis_load();

	erReturn = dis_chkStateMachine(DIS_ENGINE_SET2RUN, TRUE);
	return erReturn;
}

//for FPGA verification only
ER dis_start_withFrmEndLoad(void)
{
	ER erReturn;

	erReturn = dis_chkStateMachine(DIS_ENGINE_SET2RUN, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	dis_enableFrmEndLoad(ENABLE);

	erReturn = dis_chkStateMachine(DIS_ENGINE_SET2RUN, TRUE);
	return erReturn;
}

//for FPGA verification only
ER dis_start_withoutLoad(void)
{
	ER erReturn;

	erReturn = dis_chkStateMachine(DIS_ENGINE_SET2RUN, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	dis_enable(ENABLE);

	erReturn = dis_chkStateMachine(DIS_ENGINE_SET2RUN, TRUE);
	return erReturn;
}

/**
    DIS Check If Engine Is Running

    Check if DIS engine is already started

    @return
        \n-@b ENABLE: busy.
        \n-@b DISABLE: idle.
*/
BOOL dis_isEnabled(void)
{
	T_DIS_CONTROL_REGISTER reg1;
	reg1.reg = DIS_GETREG(DIS_CONTROL_REGISTER_OFS);

	if (reg1.bit.DIS_START == 1) {
		return ENABLE;
	} else {
		return DISABLE;
	}
}

/**
    DIS Set Mode

    Set DIS engine mode

    @param pDisAddrSize DIS configuration.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER dis_setMode(DIS_PARAM *pDisAddrSize)
{
	ER ER_Code;
	static STR_DIS_REG ModeInfo;
	SIZE_CONF sizeConf;
#if (DIS_DMA_CACHE_HANDLE == 1)
	UINT32 uiAddr0, uiAddr1, uiInSize = 0;
	UINT32 uiPhyAddr0, uiPhyAddr1;
#endif
	ER_Code = dis_chkStateMachine(DIS_ENGINE_SET2PRV, FALSE);
	if (ER_Code != E_OK) {
		return ER_Code;
	}

	sizeConf.uiWidth = pDisAddrSize->uiWidth;
	sizeConf.uiHeight = pDisAddrSize->uiHeight;
	ModeInfo = dis_allocateMotionArea(&sizeConf);

	//--- Set to registers
	//dis_clr(ENABLE);
	//dis_setMatchOp(ModeInfo.matchOp);
#if (DIS_DMA_CACHE_HANDLE == 1)
	// Input Dram DMA/Cache Handle
	uiInSize = (pDisAddrSize->uiWidth*pDisAddrSize->uiHeight)>>2; // frame mode
#if DIS_IOREMAP_IN_KERNEL
	uiPhyAddr0 = pDisAddrSize->uiInAdd0;
	uiPhyAddr1 = pDisAddrSize->uiInAdd1;
	uiAddr0 = dis_platform_pa2va_remap(uiPhyAddr0, uiInSize, 2);
	uiAddr1 = dis_platform_pa2va_remap(uiPhyAddr1, uiInSize, 2);
	g_uiInAdd0 = uiAddr0;
	g_uiInAdd1 = uiAddr1;
#else
	uiAddr0 = pDisAddrSize->uiInAdd0;
	uiAddr1 = pDisAddrSize->uiInAdd1;
	uiPhyAddr0 = dis_platform_va2pa(uiAddr0);
	uiPhyAddr1 = dis_platform_va2pa(uiAddr1);
	g_uiInAdd0 = uiAddr0;
	g_uiInAdd1 = uiAddr1;	
#endif

	dis_platform_dma_flush_mem2device(uiAddr0, uiInSize);
	dis_platform_dma_flush_mem2device(uiAddr1, uiInSize);
	
#if DIS_IOREMAP_IN_KERNEL
	dis_platform_pa2va_unmap(uiAddr0, uiPhyAddr0);
	dis_platform_pa2va_unmap(uiAddr1, uiPhyAddr0);
#endif

	dis_setDmaInAddr(uiPhyAddr0, uiPhyAddr1);
	// Output Dram DMA/Cache Handle

#if DIS_IOREMAP_IN_KERNEL
	uiPhyAddr0 = pDisAddrSize->uiOutAdd0;
	g_uiOutAdd0 = uiPhyAddr0;
#else
	uiAddr0 = pDisAddrSize->uiOutAdd0;
	//uiAddr1 = pDisAddrSize->uiOutAdd1;
	uiPhyAddr0 = dis_platform_va2pa(uiAddr0);
	g_uiOutAdd0 = uiAddr0;
#endif
	
	dis_setDmaOutAddr(uiPhyAddr0);
#else
	dis_setDmaInAddr(pDisAddrSize->uiInAdd0, pDisAddrSize->uiInAdd1, pDisAddrSize->uiInAdd2);
	dis_setDmaOutAddr(pDisAddrSize->uiOutAdd0, pDisAddrSize->uiOutAdd1);
#endif

	dis_setDmaInOffset(pDisAddrSize->uiInOfs);
	dis_setMotionEst(ModeInfo.MotSet);
	dis_setMDSDim(ModeInfo.MdsDim);
	dis_setMDSAddr(ModeInfo.MdsAddr);
	dis_setMDSOfs(ModeInfo.MdsOfs);
	dis_enableInt(ENABLE, pDisAddrSize->uiIntEn);
	//dis_clr(DISABLE);
	dis_setLoad(DIS_FRMEND_LOAD); //dis_load();

	//dis_initAlgCal();

	dis_chkStateMachine(DIS_ENGINE_SET2PRV, TRUE);

	return E_OK;
}

ER dis_changeSize(DIS_PARAM *pDisAddrSize)
{
	ER ER_Code;
	static STR_DIS_REG ModeInfo;
	SIZE_CONF sizeConf;

	ER_Code = dis_chkStateMachine(DIS_ENGINE_SET2CHGSIZE, FALSE);
	if (ER_Code != E_OK) {
		return ER_Code;
	}

	sizeConf.uiWidth = pDisAddrSize->uiWidth;
	sizeConf.uiHeight = pDisAddrSize->uiHeight;

	ModeInfo = dis_allocateMotionArea(&sizeConf);
	dis_setMotionEst(ModeInfo.MotSet);
	dis_setMDSDim(ModeInfo.MdsDim);
	dis_setMDSAddr(ModeInfo.MdsAddr);
	dis_setDmaInOffset(pDisAddrSize->uiInOfs);
	dis_setLoad(DIS_FRMEND_LOAD); //dis_load();

	dis_chkStateMachine(DIS_ENGINE_SET2CHGSIZE, TRUE);

	return E_OK;
}

#if 0
/**
    DIS Set Mode

    Set DIS engine mode

    @param pDisAddrSize DIS configuration.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER dis_setMode(DIS_PARAM *pDisAddrSize)
{
	if (pDisAddrSize->DisOperation == DIS_ENGINE_SET2PRV) {
		if (dis_setPrvMode(pDisAddrSize) != E_OK) {
			return E_OBJ;
		}
	} else if (pDisAddrSize->DisOperation == DIS_ENGINE_SET2CHGSIZE) {
		if (dis_setSizeChange(pDisAddrSize) != E_OK) {
			return E_OBJ;
		}
	} else if (pDisAddrSize->DisOperation == DIS_ENGINE_SET2RUN) {
		if (dis_start() != E_OK) {
			return E_OBJ;
		}
	} else if (pDisAddrSize->DisOperation == DIS_ENGINE_SET2PAUSE) {
		if (dis_pause() != E_OK) {
			return E_OBJ;
		}
	} else {
		return E_OBJ;
	}
	return E_OK;
}
#endif

/**
    DIS change interrupt

    DIS interrupt configuration.

    @param IntEn Interrupt enable setting.

    @return ER DIS change interrupt status.
*/
ER dis_changeInterrupt(UINT32 uiIntEn)
{
	ER erReturn;

	erReturn = dis_chkStateMachine(DIS_ENGINE_CHGINT, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	dis_enableInt(ENABLE, uiIntEn);
	dis_setLoad(DIS_FRMEND_LOAD);

	erReturn = dis_chkStateMachine(DIS_ENGINE_CHGINT, TRUE);

	return erReturn;
}

//for FPGA verification only
ER dis_changeInterrupt_withoutLoad(UINT32 uiIntEn)
{
	ER erReturn;

	erReturn = dis_chkStateMachine(DIS_ENGINE_CHGINT, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	dis_enableInt(ENABLE, uiIntEn);

	erReturn = dis_chkStateMachine(DIS_ENGINE_CHGINT, TRUE);

	return erReturn;
}


/**
    DIS Clear Frame End

    Clear DIS frame end flag.

    @return None.
*/
ER dis_clrFrameEnd(void)
{
	return clr_flg(FLG_ID_DIS, FLGPTN_DIS_FRAMEEND);
}

/**
    DIS Wait Frame End

    Wait for DIS frame end flag.

    @param[in] bClrFlag
        \n-@b TRUE: clear flag and wait for next flag.
        \n-@b FALSE: wait for flag.

    @return None.
*/
void dis_waitFrameEnd(BOOL bClrFlag)
{
	FLGPTN uiFlag;
	if (bClrFlag == TRUE) {
		clr_flg(FLG_ID_DIS, FLGPTN_DIS_FRAMEEND);
	}
	wai_flg(&uiFlag, FLG_ID_DIS, FLGPTN_DIS_FRAMEEND, TWF_CLR | TWF_ORW);
}
//@}


/**
    DIS Get Motion Results

    Get DIS motion estimation results

    @param[out] pMotResult all valid motion vectors.

    @return None.
*/
VOID dis_getMotionVectors(MOTION_INFOR *pMotResult)
{
	UINT32  i, j, k, index, cnt;
	UINT32  MVAddr;
	MDS_DIM mdsDim;
	UINT32  blkpMds;
	UINT32  motInfo0, motInfo1;
	UINT32  blocksz = DIS_APLY_BLOCK_SZ == DIS_BLKSZ_32x32 ? 32 * 32 : 64 * 48;

	mdsDim = dis_getMDSDim();
	blkpMds = mdsDim.uiBlkNumH * mdsDim.uiBlkNumV;
	MVAddr = dis_getDmaOutAddr(DIS_MOTION_BUF0);
	cnt = 0;
	for (k = 0; k < mdsDim.uiBlkNumV; k++) {
		for (j = 0; j < mdsDim.uiMdsNum; j++) {
			for (i = 0; i < mdsDim.uiBlkNumH; i++) {
				//Get from DRAM
				index = j * blkpMds + k * mdsDim.uiBlkNumH + i;
				motInfo0 = DIS_GETDATA(MVAddr, (index  << 3));
				motInfo1 = DIS_GETDATA(MVAddr, ((index  << 3) + 4));
				pMotResult[cnt].iX    =  motInfo0 & DIS_MVX;         //6 bits
				pMotResult[cnt].iY    = (motInfo0 & DIS_MVY) >> 8;   //6 bits
				pMotResult[cnt].uiSad = (motInfo0 & DIS_SAD) >> 16;  //14 bits
				pMotResult[cnt].uiCnt  =  motInfo1 & DIS_CNT;         //14 bits
				pMotResult[cnt].uiIdx = (motInfo1 & DIS_IDX) >> 16;  //11 bits

				//count
				if (((pMotResult[cnt].uiCnt * 100) < (blocksz * MVCNT_MIN)) ||
					((pMotResult[cnt].uiCnt * 100) > (blocksz * MVCNT_MAX))) {
					pMotResult[cnt].bValid = 0;
				} else {
					pMotResult[cnt].bValid = 1;
				}
				//SAD
				if (pMotResult[cnt].uiCnt) {
					pMotResult[cnt].uiSad = pMotResult[cnt].uiSad * 1000 / pMotResult[cnt].uiCnt;
				} else {
					pMotResult[cnt].uiSad = 0;
				}
				//MV
				pMotResult[cnt].iX -= 32;
				pMotResult[cnt].iY -= 32;
				cnt++;
			}
		}
	}
#if DIS_IOREMAP_IN_KERNEL
	dis_platform_pa2va_unmap(MVAddr, g_uiOutAdd0);
#endif
}


/**
    DIS Reset Frame Counter

    Reset frame counter

    @return None.
*/
static void dis_resetFrameCnt(void)
{
	g_uiDisFcnt = 0;
}

/**
    DIS Set current Frame Counter

    Set current frame counter

    @param[in] uiCnt frame counter

    @return None.
*/
void dis_setFrameCnt(UINT16 uiCnt)
{
	g_uiDisFcnt = uiCnt;
}

/**
    DIS Get current Frame Counter

    Get current frame counter

    @return None.
*/
UINT16 dis_getFrameCnt(void)
{
	return g_uiDisFcnt;
}


