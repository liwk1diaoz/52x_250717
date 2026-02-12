#include "EthCam/mdcmdio.h"
#include "io/gpio.h"
#include <kwrap/util.h>
#include "kwrap/semaphore.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          mdcmdio
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////// //////////////////////////////////////////////////////////

UINT32 smi_MDC;         /* GPIO used for SMI Clock Generation */
UINT32 smi_MDIO;       /* GPIO used for SMI Data signal */
#define CLK_DURATION(clk)   { UINT32 i; for (i=0; i< (clk); i++); }
#define DELAY 100
//#define CLK_DURATION(clk)   { vos_util_delay_us(clk);}
//#define DELAY 5
//void smiInit(void);
//INT32 smiWrite(UINT32 phyad, UINT32 regad, UINT32 data);
//INT32 smiRead(UINT32 phyad, UINT32 regad, UINT32 * data);
//INT32 smiReadBit(UINT32 phyad, UINT32 regad, UINT32 bit, UINT32 * pdata);
//INT32 smiWriteBit(UINT32 phyad, UINT32 regad, UINT32 bit, UINT32 data);

SEM_HANDLE SEMID_ETHCAM_SMI;
static UINT32 g_EthCamSMIOpen=0;

void smiInit(UINT32 MDC_Gpio, UINT32 MDIO_Gpio)
{
	if(g_EthCamSMIOpen){
		return;
	}
	/* Initialize GPIO smi_MDC  as SMI MDC signal */
	smi_MDC = MDC_Gpio;//P_GPIO_10;
	gpio_setDir(smi_MDC, GPIO_DIR_OUTPUT);
	gpio_setPin(smi_MDC);

	/* Initialize GPIO smi_MDIO  as SMI MDIO signal */
	smi_MDIO = MDIO_Gpio;//P_GPIO_4;
	gpio_setDir(smi_MDIO, GPIO_DIR_OUTPUT);
	gpio_setPin(smi_MDIO);

	OS_CONFIG_SEMPHORE(SEMID_ETHCAM_SMI, 0, 1, 1);

	g_EthCamSMIOpen=1;
}

static void smi_lock(void)
{
	vos_sem_wait(SEMID_ETHCAM_SMI);
}

static void smi_unlock(void)
{
	vos_sem_sig(SEMID_ETHCAM_SMI);
}

/*Generate  1 -> 0 transition and sampled at 1 to 0 transition time,
 *should not sample at 0->1 transition because some chips stop outputing
 *at the last bit at rising edge
 */
static void _smiReadBit(UINT32 * pdata)
{
	UINT32 u;

	//_rtl865x_initGpioPin(smi_MDIO, GPIO_PERI_GPIO, GPIO_DIR_IN, GPIO_INT_DISABLE);
	if (GPIO_DIR_INPUT != gpio_getDir(smi_MDIO)) {
		gpio_setDir(smi_MDIO, GPIO_DIR_INPUT);
	}

	//_rtl865x_setGpioDataBit(smi_MDC, 1);
	gpio_setPin(smi_MDC);
	CLK_DURATION(DELAY);
	//_rtl865x_setGpioDataBit(smi_MDC, 0);
	gpio_clearPin(smi_MDC);

	//_rtl865x_getGpioDataBit(smi_MDIO, &u);
	u=gpio_getPin(smi_MDIO);
	*pdata = u;
}



/* Generate  0 -> 1 transition and put data ready during 0 to 1 whole period */
static void _smiWriteBit(UINT32 data)
{
	//_rtl865x_initGpioPin(smi_MDIO, GPIO_PERI_GPIO, GPIO_DIR_OUT, GPIO_INT_DISABLE);
	if (GPIO_DIR_OUTPUT != gpio_getDir(smi_MDIO)) {
		gpio_setDir(smi_MDIO, GPIO_DIR_OUTPUT);
	}

	if (data)
	{  /*write 1*/
		//_rtl865x_setGpioDataBit(smi_MDIO, 1);
		gpio_setPin(smi_MDIO);

		CLK_DURATION(DELAY);

		//_rtl865x_setGpioDataBit(smi_MDC, 0);
		gpio_clearPin(smi_MDC);

		CLK_DURATION(DELAY);
		//_rtl865x_setGpioDataBit(smi_MDC, 1);
		gpio_setPin(smi_MDC);
	}
	else
	{
		//_rtl865x_setGpioDataBit(smi_MDIO, 0);
		gpio_clearPin(smi_MDIO);

		CLK_DURATION(DELAY);

		//_rtl865x_setGpioDataBit(smi_MDC, 0);
		gpio_clearPin(smi_MDC);

		CLK_DURATION(DELAY);
		//_rtl865x_setGpioDataBit(smi_MDC, 1);
		gpio_setPin(smi_MDC);
	}
}
/*
 *Low speed smi is a general MDC/MDIO interface, it is realized by call gpio api
 *function, could specified any gpio pin as MDC/MDIO
 */
static void _smiZbit(void)
{
	//_rtl865x_initGpioPin(smi_MDIO, GPIO_PERI_GPIO, GPIO_DIR_IN, GPIO_INT_DISABLE);
	if (GPIO_DIR_OUTPUT != gpio_getDir(smi_MDIO)) {
		gpio_setDir(smi_MDIO, GPIO_DIR_OUTPUT);
	}

	//_rtl865x_setGpioDataBit(smi_MDC, 0);
	gpio_clearPin(smi_MDC);

	//_rtl865x_setGpioDataBit(smi_MDIO, 0);
	gpio_clearPin(smi_MDIO);

	CLK_DURATION(DELAY);
}
/* Function Name:
 *      smiWrite
 * Description:
 *      Write data to Phy register
 * Input:
 *      phyad   - PHY address (0~31)
 *      regad   -  Register address (0 ~31)
 *      data    -  Data to be written into Phy register
 * Output:
 *      none
 * Return:
 *      SUCCESS         -  Success
 *      FAILED            -  Failure
 * Note:
 *     This function could read register through MDC/MDIO serial
 *     interface, and it is platform  related. It use two GPIO pins
 *     to simulate MDC/MDIO timing. MDC is sourced by the Station Management
 *     entity to the PHY as the timing reference for transfer of information
 *     on the MDIO signal. MDC is an aperiodic signal that has no maximum high
 *     or low times. The minimum high and low times for MDC shall be 160 ns each,
 *     and the minimum period for MDC shall be 400 ns. Obeying frame format defined
 *     by IEEE802.3 standard, you could access Phy registers. If you want to
 *     port it to other CPU, please modify static functions which are called
*      by this function.
 */

INT32 smiWrite(UINT32 phyad, UINT32 regad, UINT32 data)
{
    INT32 i;

    if ((phyad > 31) || (regad > 31) || (data > 0xFFFF))
        return -1;

    /*it lock the resource to ensure that SMI opertion is atomic,
      *the API is based on RTL865X, it is used to disable CPU interrupt,
      *if porting to other platform, please rewrite it to realize the same function
      */
    //rtlglue_drvMutexLock();
    smi_lock();

    /* 32 continuous 1 as preamble*/
    for(i=0; i<32; i++)
        _smiWriteBit(1);

    /* ST: Start of Frame, <01>*/
    _smiWriteBit(0);
    _smiWriteBit(1);

    /* OP: Operation code, write is <01>*/
    _smiWriteBit(0);
    _smiWriteBit(1);

    /* PHY Address */
    for(i=4; i>=0; i--)
        _smiWriteBit((phyad>>i)&0x1);

    /* Register Address */
    for(i=4; i>=0; i--)
        _smiWriteBit((regad>>i)&0x1);

    /* TA: Turnaround <10>*/
    _smiWriteBit(1);
    _smiWriteBit(0);

    /* Data */
    for(i=15; i>=0; i--)
        _smiWriteBit((data>>i)&0x1);
    _smiZbit();

    /*unlock the source, enable interrupt*/
    //rtlglue_drvMutexUnlock();
    smi_unlock();

    return 0;
}

/* Function Name:
 *      smiRead
 * Description:
 *      Read data from phy register
 * Input:
 *      phyad   - PHY address (0~31)
 *      regad   -  Register address (0 ~31)
 * Output:
 *      data    -  Register value
 * Return:
 *      SUCCESS         -  Success
 *      FAILED            -  Failure
 * Note:
 *     This function could read register through MDC/MDIO serial
 *     interface, and it is platform  related. It use two GPIO pins
 *     to simulate MDC/MDIO timing. MDC is sourced by the Station Management
 *     entity to the PHY as the timing reference for transfer of information
 *     on the MDIO signal. MDC is an aperiodic signal that has no maximum high
 *     or low times. The minimum high and low times for MDC shall be 160 ns each,
 *     and the minimum period for MDC shall be 400 ns. Obeying frame format defined
 *     by IEEE802.3 standard, you could access Phy registers. If you want to
 *     port it to other CPU, please modify static functions which are called
 *      by this function.
 */
INT32 smiRead(UINT32 phyad, UINT32 regad, UINT32 * data)
{

    INT32 i;
    UINT32 readBit;

    if ((phyad > 31) || (regad > 31) || (data == NULL))
        return -1;

    /*it lock the resource to ensure that SMI opertion is atomic,
     *the API is based on RTL865X, it is used to disable CPU interrupt,
     *if porting to other platform, please rewrite it to realize the same function
     */

    //rtlglue_drvMutexLock();
    smi_lock();

    /* 32 continuous 1 as preamble*/
    for(i=0; i<32; i++)
        _smiWriteBit(1);

    /* ST: Start of Frame, <01>*/
    _smiWriteBit(0);
    _smiWriteBit(1);

    /* OP: Operation code, read is <10>*/
    _smiWriteBit(1);
    _smiWriteBit(0);

    /* PHY Address */
    for(i=4; i>=0; i--)
        _smiWriteBit((phyad>>i)&0x1);

    /* Register Address */
    for(i=4; i>=0; i--)
        _smiWriteBit((regad>>i)&0x1);

    /* TA: Turnaround <z0>, zbit has no clock in order to steal a clock to
     *  sample data at clock falling edge
     */
    _smiZbit();
    _smiReadBit(&readBit);

    /* Data */
    *data = 0;
    for(i=15; i>=0; i--)
    {
        _smiReadBit(&readBit);
        *data = (*data<<1) | readBit;
    }

    /*add  an extra clock cycles for robust reading , ensure partner stop
     *output signal and not affect the next read operation, because TA
     *steal a clock*/
    _smiWriteBit(1);
    _smiZbit();

    /*unlock the source, enable interrupt*/
    //rtlglue_drvMutexUnlock();
    smi_unlock();

    return  0;
}
INT32 smiReadBit(UINT32 phyad, UINT32 regad, UINT32 bit, UINT32 * pdata)
{
    UINT32 regData;

    if ((phyad > 31) || (regad > 31) || (bit > 15) || (pdata == NULL) )
        return  -1;

    if(bit>=16)
        * pdata = 0;
    else
    {
        smiRead(phyad, regad, &regData);
        if(regData & (1<<bit))
            * pdata = 1;
        else
            * pdata = 0;
    }
    return 0;
}

/* Function Name:
 *      smiWriteBit
 * Description:
 *      Write one bit of PHY register
 * Input:
 *      phyad   - PHY address (0~31)
 *      regad   -  Register address (0 ~31)
 *      bit       -  Register bit (0~15)
 *      data     -  Bit value to be written
 * Output:
 *      none
 * Return:
 *      SUCCESS         -  Success
 *      FAILED            -  Failure
 * Note:
 */

INT32 smiWriteBit(UINT32 phyad, UINT32 regad, UINT32 bit, UINT32 data)
{
    UINT32 regData;

    if ((phyad > 31) || (regad > 31) || (bit > 15) || (data > 1) )
        return  -1;
    smiRead(phyad, regad, &regData);
    if(data)
        regData = regData | (1<<bit);
    else
        regData = regData & ~(1<<bit);
    smiWrite(phyad, regad, regData);
    return 0;
}
