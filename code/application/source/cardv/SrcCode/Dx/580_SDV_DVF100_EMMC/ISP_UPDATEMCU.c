
#include "kwrap/type.h"
#include "DrvExt.h"
#include "IOCfg.h"
#include "IOInit.h"
#include <kwrap/util.h>
//#include "Customer.h"


///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          DxDrv
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////




char CheckMcuUpdateOK=2;


//=========================================================
#define	 I2C_WRITE_ADDR		0x24
#define	 I2C_READ_ADDR		0x25




#define __EN_ISP_DEBUG_PRINT__


//=========================================================
static UINT8  PinState;


#define	CST_ISP_SIZE 0x08
UINT16  IspMcuAddrOffset;
UINT16  IspMcuAddr; 


UINT16  IspLftSize;
UINT8   IspCurSize;
UINT8  * pIspDataBuf;


#define MCURTN_SoftWareUpdate_Succ  1
#define MCURTN_SoftWareUpdate_Fail     0


#define MCU_ISP_SDA  P_GPIO_7
#define MCU_ISP_SCL  P_GPIO_8


#define SDA_OUTPUT_1    gpio_setDir(MCU_ISP_SDA, GPIO_DIR_OUTPUT);gpio_setPin(MCU_ISP_SDA);
#define SDA_OUTPUT_0    gpio_setDir(MCU_ISP_SDA, GPIO_DIR_OUTPUT);gpio_clearPin(MCU_ISP_SDA);

#define SCL_OUTPUT_1     gpio_setDir(MCU_ISP_SCL, GPIO_DIR_OUTPUT); gpio_setPin(MCU_ISP_SCL);
#define SCL_OUTPUT_0    gpio_setDir(MCU_ISP_SCL, GPIO_DIR_OUTPUT);gpio_clearPin(MCU_ISP_SCL);



#define SDA_OUTPUT    gpio_setDir(MCU_ISP_SDA, GPIO_DIR_OUTPUT);
#define SCL_OUTPUT    gpio_setDir(MCU_ISP_SCL, GPIO_DIR_OUTPUT);

#define SDA_INPUT_1  gpio_setDir(MCU_ISP_SDA, GPIO_DIR_INPUT);

//=========================================================
//	Local function
//=========================================================

//---------------------------------------------------------
//---------------------------------------------------------
static void SDA_READ(void)
{
	
	gpio_setDir(MCU_ISP_SDA, GPIO_DIR_INPUT);
	PinState=gpio_getPin(MCU_ISP_SDA);

	//gpio_setDir(MCU_ISP_SDA, GPIO_DIR_OUTPUT);
}

static void I2C_Delay(void)
{

    UINT32	times =100;
    for( ; times>0; times--)
        ;
}

//---------------------------------------------------------
//---------------------------------------------------------
static void I2C_Start(void)
{
	
	SDA_OUTPUT
	SCL_OUTPUT

  	SDA_OUTPUT_1
  	I2C_Delay();
  	SCL_OUTPUT_1
  	I2C_Delay();
  	SDA_OUTPUT_0
  	I2C_Delay();   
  	SCL_OUTPUT_0
  	I2C_Delay();
}

//---------------------------------------------------------
//---------------------------------------------------------
static void I2C_Stop(void)
{
  	SDA_OUTPUT_0
  	I2C_Delay();
  	SCL_OUTPUT_1
  	I2C_Delay();
  	SDA_OUTPUT_1
  	I2C_Delay();
}

//---------------------------------------------------------
//---------------------------------------------------------
// return: 0__succ  other__fail
static UINT8 I2C_SendByte(UINT8 sndData)
{
  	UINT8 bitCnt;
 
  	for(bitCnt=0;bitCnt<8;bitCnt++)
  	{ 
		if( sndData & 0x80 )
		{
			SDA_OUTPUT_1
		}
		else
		{
			SDA_OUTPUT_0
		}
		sndData <<= 1;

		I2C_Delay();
		SCL_OUTPUT_1
		I2C_Delay();
		SCL_OUTPUT_0
  	}
	
	//SDA_OUTPUT_1
		
	SDA_INPUT_1
	I2C_Delay();
	SCL_OUTPUT_1
	I2C_Delay();
	SDA_READ();
	SCL_OUTPUT_0
	I2C_Delay();

	return	PinState;
}

//---------------------------------------------------------
//---------------------------------------------------------
static UINT8 I2C_RcvByte(void)
{
	UINT8 recData=0;
	UINT8 bitCnt;
	
		
	for(bitCnt=0; bitCnt<8; bitCnt++)
	{
		SCL_OUTPUT_1
		I2C_Delay();
		recData <<= 1;
		SDA_READ();
		if( PinState )
		{
			recData |= 1;
		}
		SCL_OUTPUT_0
		I2C_Delay();
	}

	return	recData;
}

//---------------------------------------------------------
//---------------------------------------------------------
static void I2C_SendAck(void)
{
	SDA_OUTPUT_0
	I2C_Delay();
	SCL_OUTPUT_1
	I2C_Delay();
	SCL_OUTPUT_0
	SDA_OUTPUT_1	
	I2C_Delay();
}

//---------------------------------------------------------
//---------------------------------------------------------
static void I2C_SendNack(void)
{
	SDA_OUTPUT_1
	I2C_Delay();
	SCL_OUTPUT_1
	I2C_Delay();
	SCL_OUTPUT_0
	SDA_OUTPUT_1
	I2C_Delay();
}



void ISP_Wait5ms(UINT8 t5ms)
{
	vos_util_delay_ms(5*t5ms);
}


//---------------------------------------------------------
//---------------------------------------------------------
// return:  0----succ    1----fail
UINT8 ISP_EnableISP(void)
{
	// S SLV+W NACK 'e' NACK 'd' NACK 'o' NACK 'M' NACK 'D' NACK 'R' ACK P
	UINT8 retinf = 1;

	I2C_Start();
	do{
		if( I2C_SendByte(I2C_WRITE_ADDR )==0 )
			break;
		if( I2C_SendByte('e' )==0 )
			break;
		if( I2C_SendByte('d' )==0 )
			break; 
		if( I2C_SendByte('o' )==0 )
			break;
		if( I2C_SendByte('M' )==0 )
			break;
		if( I2C_SendByte('D' )==0 )
			break;
		if( I2C_SendByte('R' )!=0 )
			break;
		retinf = 0;
	}while(0);
	I2C_Stop();

	return	retinf;
}

//---------------------------------------------------------
//---------------------------------------------------------
// return:  0----succ    1----fail
UINT8 ISP_EnterISPMode(void)
{
	// S SLV+W ¡°80H¡± ¡°20H¡± ¡°02H¡± P
	UINT8 retinf = 1;
	
	I2C_Start();
	do{
		if( I2C_SendByte(I2C_WRITE_ADDR )!=0 )
			break;
		if( I2C_SendByte(0x80 )!=0 )
			break;
		if( I2C_SendByte(0x20 )!=0 )
			break; 
		if( I2C_SendByte(0x02 )!=0 )
			break;
		retinf = 0;
	}while(0);
	I2C_Stop();

	return	retinf;
}

//---------------------------------------------------------
//---------------------------------------------------------
// return:  0----succ    1----fail
UINT8 ISP_FreeWriteProtect(void)
{
	// S SLV+W ¡°10H¡± ¡°10H¡± Lock Byte1[7:0] P
	// S SLV+W ¡°10H¡± ¡°11H¡± Lock Byte1[15:8] P
	// S SLV+W ¡°10H¡± ¡°12H¡± Lock Byte2[7:0] P
	// S SLV+W ¡°10H¡± ¡°13H¡± Lock Byte2[15:8] P
	UINT8 retinf = 1;

	I2C_Start();
	do{
		if( I2C_SendByte(I2C_WRITE_ADDR )!=0 )
			break;
		if( I2C_SendByte(0x10 )!=0 )
			break;
		if( I2C_SendByte(0x10 )!=0 )
			break; 
		if( I2C_SendByte(0x00 )!=0 )
			break;
		I2C_Stop();

		I2C_Start();
		if( I2C_SendByte(I2C_WRITE_ADDR )!=0 )
			break;
		if( I2C_SendByte(0x10 )!=0 )
			break;
		if( I2C_SendByte(0x11 )!=0 )
			break; 
		if( I2C_SendByte(0x00 )!=0 )
			break;
		I2C_Stop();

		I2C_Start();
		if( I2C_SendByte(I2C_WRITE_ADDR )!=0 )
			break;
		if( I2C_SendByte(0x10 )!=0 )
			break;
		if( I2C_SendByte(0x12 )!=0 )
			break; 
		if( I2C_SendByte(0xFF )!=0 )
			break;
		I2C_Stop();

		I2C_Start();
		if( I2C_SendByte(I2C_WRITE_ADDR )!=0 )
			break;
		if( I2C_SendByte(0x10 )!=0 )
			break;
		if( I2C_SendByte(0x13 )!=0 )
			break; 
		if( I2C_SendByte(0xFF )!=0 )
			break;
		
		retinf = 0;
	}while(0);
	I2C_Stop();

	return	retinf;	
}

//---------------------------------------------------------
//---------------------------------------------------------
// return:  0----succ    1----fail
UINT8 ISP_ChipErase(void)
{
	// S SLV+W ¡°20H¡± ¡°00H¡± ¡°00H¡± P
	UINT8 retinf = 1;

	I2C_Start();
	do{
		if( I2C_SendByte(I2C_WRITE_ADDR )!=0 )
			break;
		if( I2C_SendByte(0x20 )!=0 )
			break;
		if( I2C_SendByte(0x00 )!=0 )
			break; 
		if( I2C_SendByte(0x00 )!=0 )
			break;
		retinf = 0;
	}while(0);
	I2C_Stop();

	return	retinf;	
}

//---------------------------------------------------------
//---------------------------------------------------------
// return:  0----succ    1----fail
UINT8 ISP_FinishCmd(void)
{
	// S SLV+W ¡°00H¡± ¡°00H¡± ¡°00H¡± P
	UINT8 retinf = 1;
	
	I2C_Start();
	do{
		if( I2C_SendByte(I2C_WRITE_ADDR )!=0 )
			break;
		if( I2C_SendByte(0x00 )!=0 )
			break;
		if( I2C_SendByte(0x00 )!=0 )
			break; 
		if( I2C_SendByte(0x00 )!=0 )
			break;
		retinf = 0;
	}while(0);
	I2C_Stop();

	return	retinf;
}

//---------------------------------------------------------
//---------------------------------------------------------
// return:  0----succ    1----fail
UINT8 ISP_SetAddrHigh(void)
{
	// S SLV+W ¡°10H¡± ¡°01H¡± ADDR[13:8] P
	UINT8 retinf = 1;
	
	I2C_Start();
	do{
		if( I2C_SendByte(I2C_WRITE_ADDR )!=0 )
			break;
		if( I2C_SendByte(0x10 )!=0 )
			break;
		if( I2C_SendByte(0x01 )!=0 )
			break; 
		if( I2C_SendByte( (IspMcuAddr>>8) )!=0 )
			break;
		retinf = 0;
	}while(0);
	I2C_Stop();

	return	retinf;
}

//---------------------------------------------------------
//---------------------------------------------------------
// return:  0----succ    1----fail
//addr[0]=pIspDataBuf[0] pIspDataBuf[1] pIspDataBuf[3].....pIspDataBuf[7]
//addr[1]=pIspDataBuf[8] pIspDataBuf[9] pIspDataBuf[10].....pIspDataBuf[15]
//.....
UINT8 ISP_WriteData(void)
{
	// S SLV+W ¡°40H¡± ADDR[7:0] Data1 Data2 ¡­ Data64 P
	UINT8 retinf = 1;
	UINT8 localIdx;

	I2C_Start();
	do{
		if( I2C_SendByte(I2C_WRITE_ADDR )!=0 )
			break;
		if( I2C_SendByte(0x41 )!=0 )
			break;
		if( I2C_SendByte( IspMcuAddr ) !=0 )
			break; 
		for( localIdx=0; localIdx<IspCurSize; localIdx++ )
		{

		//	printf("IspMcuAddr=%d *(pIspDataBuf+localIdx)= %d\r\n",IspMcuAddr,*(pIspDataBuf+localIdx));
			if( I2C_SendByte( *(pIspDataBuf+localIdx) )!=0 )
			{
				break;
			}
		}
		if( localIdx != IspCurSize )
		{
			break;
		}
		retinf = 0;
	}while(0);
	I2C_Stop();

	return	retinf;
}

//---------------------------------------------------------
//---------------------------------------------------------
// return:  0----succ    1----fail
extern 	UINT8 ReadData_test;
extern 	UINT8 ReadData_test_buf;

UINT8 ISP_VerifyData(void)
{
	// S SLV+W ¡±61H¡± ADDR[7:0] reSTART SLV+R Data1 Data2 ¡­ Data256 P
	UINT8 retinf = 1;
	UINT8 localIdx;
	UINT8 ReadData;

	I2C_Start();
	do{
		if( I2C_SendByte(I2C_WRITE_ADDR )!=0 ){
			CHKPNT;
			break;
			}
		if( I2C_SendByte(0x61 )!=0 ){
			CHKPNT;
			break;
			}
		if( I2C_SendByte( IspMcuAddr ) !=0 ){
			CHKPNT;
			break; 
			}
		I2C_Stop();

		I2C_Start();
		if( I2C_SendByte(I2C_READ_ADDR )!=0 ){
			CHKPNT;
			break;
			}

		
		
		for( localIdx=0; localIdx<IspCurSize; localIdx++ )
		{	

			SDA_INPUT_1
			ReadData = I2C_RcvByte();
			if( localIdx < (IspCurSize-1) )
				I2C_SendAck();
			else
				I2C_SendNack();

			//ReadData_test=ReadData;
			//ReadData_test_buf=*(pIspDataBuf+localIdx);
			
			//printf("\r\IspMcuAddr=%d  ",IspMcuAddr);
			//printf("\r\nReadData=%d  *(pIspDataBuf+localIdx)=%d ",ReadData,*(pIspDataBuf+localIdx));
			if( ReadData != *(pIspDataBuf+localIdx) )
			{
				printf("NGlocalIdx=%d IspCurSize=%d\r\n",localIdx,IspCurSize);
				printf("NGReadData=%d  *(pIspDataBuf+localIdx)=%d\r\n ",ReadData,*(pIspDataBuf+localIdx));
				break;
			}else{
				//printf("OK localIdx=%d IspCurSize=%d\r\n",localIdx,IspCurSize);
			//	printf("OK ReadData=%d  *(pIspDataBuf+localIdx)=%d\r\n ",ReadData,*(pIspDataBuf+localIdx));
	
			}
		}

		
		if( localIdx != IspCurSize )
		{
			break;
		}
		retinf = 0;
	}while(0);
	I2C_Stop();

	return	retinf;
}
	
//---------------------------------------------------------
//---------------------------------------------------------
// return:  0----succ    1----fail
UINT8 ISP_EndCommand(void)
{
	// S SLV+W ¡°10H¡± ¡°02H¡± ¡°01H¡± P
	UINT8 retinf = 1;
	
	I2C_Start();
	do{
		if( I2C_SendByte(I2C_WRITE_ADDR )!=0 )
			break;
		if( I2C_SendByte(0x10 )!=0 )
			break;
		if( I2C_SendByte(0x02 )!=0 )
			break; 
		if( I2C_SendByte(0x01 )==0 )
			break;
		retinf = 0;
	}while(0);
	I2C_Stop();

	return	retinf;
}


//=========================================================
//	Global Function
//=========================================================

//---------------------------------------------------------
//---------------------------------------------------------
void ISP_PwrOnInit(void)
{
	//SDA_GPIO
	//SDA_PULLEN
	//SDA_PULLHIGH
	SDA_OUTPUT
	SDA_OUTPUT_1
	
	//SCL_GPIO
	//SCL_PULLEN
	//SCL_PULLHIGH
	SCL_OUTPUT
	SCL_OUTPUT_1

	I2C_Stop();

	//DebugPinInit
}
/*
ISP_Run
       ISP_EnISP_F
                  ISP_Run
                        
ISP_EnISP_F
           ISP_Run
                  ISP_EnISP_F
                             ISP_Fail
                            */
//---------------------------------------------------------
//---------------------------------------------------------

extern char CheckMcuUpdateOK;

UINT8 MCU_FwUpdateRun(void)
{
	UINT8 retinf = 1;
	UINT8 trycnt = 3;

	while(trycnt--)
	{
		#ifdef __EN_ISP_DEBUG_PRINT__
		printf("ISP_Run\r\n");
		#endif

		//-------------------------------------------------
		ISP_EndCommand();
		
		//-------------------------------------------------
		if( ISP_EnableISP() != 0 )
		{
			#ifdef __EN_ISP_DEBUG_PRINT__
			printf("ISP_EnISP_F\r\n");
			#endif
			continue;
		}
			
		//-------------------------------------------------
		if( ISP_EnterISPMode() != 0 )
		{
			#ifdef __EN_ISP_DEBUG_PRINT__
			printf("ISP_EnISPMod_F\r\n");
			#endif
			continue;
		}

		//-------------------------------------------------
		if( ISP_FreeWriteProtect() != 0 )
		{
			#ifdef __EN_ISP_DEBUG_PRINT__
			printf("ISP_FWP_F\r\n");
			#endif
			continue;
		}

		//-------------------------------------------------
		if( ISP_ChipErase() != 0 )
		{
			#ifdef __EN_ISP_DEBUG_PRINT__
			printf("ISP_E0_F\r\n");
			#endif
			continue;
		}
		ISP_Wait5ms(20);
		if( ISP_FinishCmd() != 0 )
		{
			#ifdef __EN_ISP_DEBUG_PRINT__
			printf("ISP_E1_F\r\n");
			#endif
			continue;
		}
		
		//-------------------------------------------------
		IspMcuAddrOffset = 0;
		retinf = 0;
		break;
	}

	if( retinf==0 )
	{
		#ifdef __EN_ISP_DEBUG_PRINT__
		printf("ISP_Succ\r\n");
		#endif
		return MCURTN_SoftWareUpdate_Succ;
	}
	else
	{
		#ifdef __EN_ISP_DEBUG_PRINT__
		printf("ISP_Fail\r\n");
		#endif
		CheckMcuUpdateOK=0;
		return MCURTN_SoftWareUpdate_Fail;
	}
}
//IspMcuAddr=0; pIspDataBuf-->MCU.bin   IspLftSize=13644

/*
 pBuf_McuUpdate_Addr[0]==2
 pBuf_McuUpdate_Addr[1]==20
 pBuf_McuUpdate_Addr[2]==115
 pBuf_McuUpdate_Addr[3]==2
 pBuf_McuUpdate_Addr[4]==38
 pBuf_McuUpdate_Addr[5]==12
 pBuf_McuUpdate_Addr[6]==194
 pBuf_McuUpdate_Addr[7]==140
 pBuf_McuUpdate_Addr[8]==194
 pBuf_McuUpdate_Addr[9]==169
 pBuf_McuUpdate_Addr[10]==34
 pBuf_McuUpdate_Addr[11]==2
 pBuf_McuUpdate_Addr[12]==31
 pBuf_McuUpdate_Addr[13]==243
 pBuf_McuUpdate_Addr[14]==228
 pBuf_McuUpdate_Addr[15]==120
 pBuf_McuUpdate_Addr[16]==179
 pBuf_McuUpdate_Addr[17]==246
 pBuf_McuUpdate_Addr[18]==34
 pBuf_McuUpdate_Addr[19]==2
 pBuf_McuUpdate_Addr[20]==39
 pBuf_McuUpdate_Addr[21]==92
 pBuf_McuUpdate_Addr[22]==194
 pBuf_McuUpdate_Addr[23]==44
 pBuf_McuUpdate_Addr[24]==194
 pBuf_McuUpdate_Addr[25]==39
 pBuf_McuUpdate_Addr[26]==34
 pBuf_McuUpdate_Addr[27]==2
 pBuf_McuUpdate_Addr[28]==52
 pBuf_McuUpdate_Addr[29]==45
 pBuf_McuUpdate_Addr[30]==120
 pBuf_McuUpdate_Addr[31]==200
 pBuf_McuUpdate_Addr[32]==230
 pBuf_McuUpdate_Addr[33]==255
 pBuf_McuUpdate_Addr[34]==34
 pBuf_McuUpdate_Addr[35]==2
 pBuf_McuUpdate_Addr[36]==32
 pBuf_McuUpdate_Addr[37]==148
 pBuf_McuUpdate_Addr[38]==120
 pBuf_McuUpdate_Addr[39]==206
 pBuf_McuUpdate_Addr[40]==166
 pBuf_McuUpdate_Addr[41]==7
 pBuf_McuUpdate_Addr[42]==34
 pBuf_McuUpdate_Addr[43]==2
 pBuf_McuUpdate_Addr[44]==52
 pBuf_McuUpdate_Addr[45]==48
 pBuf_McuUpdate_Addr[46]==120
 pBuf_McuUpdate_Addr[47]==206
 pBuf_McuUpdate_Addr[48]==230
 pBuf_McuUpdate_Addr[49]==255
 pBuf_McuUpdate_Addr[50]==34
 pBuf_McuUpdate_Addr[51]==2
 pBuf_McuUpdate_Addr[52]==52
 pBuf_McuUpdate_Addr[53]==51
 pBuf_McuUpdate_Addr[54]==32
 pBuf_McuUpdate_Addr[55]==48
 pBuf_McuUpdate_Addr[56]==6
 pBuf_McuUpdate_Addr[57]==32
 pBuf_McuUpdate_Addr[58]==49
 pBuf_McuUpdate_Addr[59]==3
 pBuf_McuUpdate_Addr[60]==194
 pBuf_McuUpdate_Addr[61]==50
 pBuf_McuUpdate_Addr[62]==34
 pBuf_McuUpdate_Addr[63]==228

*/
UINT8 MCU_FwUpdateWrite(UINT8 *pBuf, UINT16 bufSize )
{
	UINT8 retinf = 1;
	UINT8 trycnt = 3;

	while(trycnt--)
	{
		#ifdef __EN_ISP_DEBUG_PRINT__
		printf("ISP_W_Run\n");
		#endif

		//-------------------------------------------------
		IspMcuAddr = IspMcuAddrOffset;
		pIspDataBuf = pBuf;
		IspLftSize = bufSize;
		
		while(IspLftSize!=0)
		{
			if( IspLftSize >= CST_ISP_SIZE )
				IspCurSize = CST_ISP_SIZE;
			else
				IspCurSize = IspLftSize;

			if( ISP_SetAddrHigh() != 0 )
			{
				#ifdef __EN_ISP_DEBUG_PRINT__
				printf("ISP_W_SAH_F\n");
				#endif
				break;
			}
			if( ISP_WriteData() != 0 )
			{
				#ifdef __EN_ISP_DEBUG_PRINT__
				printf("ISP_W_WD0_F\n");
				#endif
				break;
			}
			if( ISP_FinishCmd() != 0 )
			{
				#ifdef __EN_ISP_DEBUG_PRINT__
				printf("ISP_W_WD1_F\n");
				#endif
				break;
			}
			//printf("before--IspLftSize=%d  pIspDataBuf=%x  IspMcuAddr=%d\r\n",IspLftSize,pIspDataBuf,IspMcuAddr);

			IspLftSize -= IspCurSize;
			pIspDataBuf += IspCurSize;
			IspMcuAddr += IspCurSize;
			//printf("After--IspLftSize=%d  pIspDataBuf=%x  IspMcuAddr=%d\r\n",IspLftSize,pIspDataBuf,IspMcuAddr);
		}
		if( IspLftSize != 0 )
		{
			continue;
		}
		
		//-------------------------------------------------
		IspMcuAddr = IspMcuAddrOffset;
		pIspDataBuf = pBuf;
		IspLftSize = bufSize;
		while(IspLftSize!=0)
		{
			if( IspLftSize >= CST_ISP_SIZE )
				IspCurSize = CST_ISP_SIZE;
			else
				IspCurSize = IspLftSize;

			if( ISP_SetAddrHigh() != 0 )
			{
				#ifdef __EN_ISP_DEBUG_PRINT__
				printf("ISP_R_SAH_F\n");
				#endif
				break;
			}
			if( ISP_VerifyData() != 0 )
			{
				#ifdef __EN_ISP_DEBUG_PRINT__
				printf("ISP_R_VD0_F\n");
				#endif
				break;
			}
			if( ISP_FinishCmd() != 0 )
			{
				#ifdef __EN_ISP_DEBUG_PRINT__
				printf("ISP_R_VD1_F\n");
				#endif
				break;
			}
			
			IspLftSize -= IspCurSize;
			pIspDataBuf += IspCurSize;
			IspMcuAddr += IspCurSize;
		}
		if( IspLftSize != 0 )
		{
			continue;
		}

		//-------------------------------------------------
		IspMcuAddrOffset += bufSize;
		retinf = 0;
		break;
	}

	if( retinf==0 )
	{
		#ifdef __EN_ISP_DEBUG_PRINT__
		printf("ISP_W_S\n");
		#endif
		return MCURTN_SoftWareUpdate_Succ;
	}
	else
	{
		#ifdef __EN_ISP_DEBUG_PRINT__
		printf("ISP_W_F\n");
		#endif
		CheckMcuUpdateOK=0;
		return MCURTN_SoftWareUpdate_Fail;
	}
}


UINT8 MCU_FwUpdateEnd(void)
{
	UINT8 retinf = 1;
	UINT8 trycnt = 3;

	while(trycnt--)
	{
		#ifdef __EN_ISP_DEBUG_PRINT__
		printf("ISP_End_Run\n");
		#endif
		
		//-------------------------------------------------
		if( ISP_EndCommand() != 0 )
		{
			#ifdef __EN_ISP_DEBUG_PRINT__
			printf("ISP_EndCmd_F\n");
			#endif
			continue;
		}

		//-------------------------------------------------
		retinf = 0;
		break;
	}

	if( retinf==0 )
	{
		#ifdef __EN_ISP_DEBUG_PRINT__
		printf("ISP_End_S\n");
		#endif
		CheckMcuUpdateOK=1;
		return MCURTN_SoftWareUpdate_Succ;
	}
	else
	{
		#ifdef __EN_ISP_DEBUG_PRINT__
		printf("ISP_End_F\n");
		#endif
		CheckMcuUpdateOK=0;
		return MCURTN_SoftWareUpdate_Fail;
	}
}



