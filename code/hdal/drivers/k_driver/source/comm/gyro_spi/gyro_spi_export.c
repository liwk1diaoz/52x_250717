#include <linux/module.h>
#include "comm/gyro_spi.h"

EXPORT_SYMBOL(spi_open);
EXPORT_SYMBOL(spi_close);
EXPORT_SYMBOL(spi_setConfig);
EXPORT_SYMBOL(spi_startGyro);
EXPORT_SYMBOL(spi_stopGyro);
EXPORT_SYMBOL(spi_readGyroCounter);
EXPORT_SYMBOL(spi_changeGyro);
EXPORT_SYMBOL(spi_getGyroQueueCount);
EXPORT_SYMBOL(spi_getGyroData);
EXPORT_SYMBOL(spi_setBusWidth);
EXPORT_SYMBOL(spi_setCSActive);
EXPORT_SYMBOL(spi_setTransferLen);
EXPORT_SYMBOL(spi_writeSingle);
EXPORT_SYMBOL(spi_readSingle);
EXPORT_SYMBOL(spi_writeReadData);
EXPORT_SYMBOL(spi_writeReadSingle);

// Engineer usage
EXPORT_SYMBOL(spi_clrGyroStatus);
EXPORT_SYMBOL(spi_getGyroStatus);
EXPORT_SYMBOL(spi_getGyroFifoCnt);
EXPORT_SYMBOL(spi_readGyroFifo);
EXPORT_SYMBOL(spiTest_setPktDelay);


