#ifdef __KERNEL__
#include <linux/init.h>
#include <linux/spi/spi.h>
#endif

#define CTL_SEN_ID_MAX    8

static struct spi_device *sen_spi_device[CTL_SEN_ID_MAX] = { NULL };

static struct spi_board_info sen_spi_driver[CTL_SEN_ID_MAX] = {
	{
		.modalias     = "sensor_1",
		.max_speed_hz = 10000000,     //* SPI clock rate
		.bus_num      = SEN_SPI_ID_1, //* which SPI output signal in dts nvt-devname
		.chip_select  = 0,
		.mode         = SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST
	},
	{
		.modalias     = "sensor_2",
		.max_speed_hz = 10000000,
		.bus_num      = SEN_SPI_ID_2,
		.chip_select  = 0,
		.mode         = SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST
	},
	{
		.modalias     = "sensor_3",
		.max_speed_hz = 10000000,
		.bus_num      = SEN_SPI_ID_1,
		.chip_select  = 0,
		.mode         = SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST
	},
	{
		.modalias     = "sensor_4",
		.max_speed_hz = 10000000,
		.bus_num      = SEN_SPI_ID_1,
		.chip_select  = 0,
		.mode         = SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST
	},
	{
		.modalias     = "sensor_5",
		.max_speed_hz = 10000000,
		.bus_num      = SEN_SPI_ID_1,
		.chip_select  = 0,
		.mode         = SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST
	},
	{
		.modalias     = "sensor_6",
		.max_speed_hz = 10000000,
		.bus_num      = SEN_SPI_ID_1,
		.chip_select  = 0,
		.mode         = SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST
	},
	{
		.modalias     = "sensor_7",
		.max_speed_hz = 10000000,
		.bus_num      = SEN_SPI_ID_1,
		.chip_select  = 0,
		.mode         = SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST
	},
	{
		.modalias     = "sensor_8",
		.max_speed_hz = 10000000,
		.bus_num      = SEN_SPI_ID_1,
		.chip_select  = 0,
		.mode         = SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST
	}
};

struct spi_msg {
	UINT16 len;
	UINT8 *buf;
};

ER sen_spi_init_driver(UINT32 id, SEN_SPI *sen_spi)
{
	ER ret = E_OK;
	struct spi_master *master;

	sen_spi_driver[id].bus_num = sen_spi->id;
	sen_spi_driver[id].max_speed_hz = sen_spi->speed;
	sen_spi_driver[id].mode = sen_spi->mode;

	master = spi_busnum_to_master(sen_spi_driver[id].bus_num);

	if (master) {
		sen_spi_device[id] = spi_new_device(master, &sen_spi_driver[id]);

		if (sen_spi_device[id]) {
			master->max_dma_len = (sen_spi->fmt == SEN_SPI_FMT_16BIT) ? 2 : 3;    // NVT indicate for 24 bit tranfer
			sen_spi_device[id]->bits_per_word = 8;

			if (spi_setup(sen_spi_device[id]) != 0) {
				ret = -ENXIO;
				spi_unregister_device(sen_spi_device[id]);
				printk("fail to setup slave!\n");
			}
		} else {
			ret = -ENXIO;
			printk("fail to create slave!\n");
		}
	} else {
		ret = -ENXIO;
		printk("master not found!\n");
	}

	return ret;
}

void sen_spi_remove_driver(UINT32 id)
{
	if (sen_spi_device[id] != NULL) {
		spi_unregister_device(sen_spi_device[id]);
	}
}

INT32 sen_spi_transfer(UINT32 id, struct spi_msg *msgs, INT32 num)
{
	UINT32 spi_data, rd_addr;

	if (num == 2) {        // read register
		spi_data = (msgs->len == 2) ? ((((UINT32)msgs->buf[1] << 8) | (UINT32)msgs->buf[0]) & 0xFFFF) : ((UINT32)msgs->buf[0] & 0xFF);
	} else {
		if (num == 1) {    // write register
			spi_data = (msgs->len == 3) ? (((UINT32)msgs->buf[2] << 16) | ((UINT32)msgs->buf[1] << 8) | ((UINT32)msgs->buf[0])) : ((((UINT32)msgs->buf[1] << 8) | (UINT32)msgs->buf[0]) & 0xFFFF);
		} else {
			printk("spi_transfer special write cmd!\n");
			return -1;
		}
	}

	if (unlikely(spi_write(sen_spi_device[id], &spi_data, msgs->len) != 0)) {
		printk("%s fail: spi_transfer not OK!\n", __FUNCTION__);
		return -1;
	}

	if (num == 2) {    // read register
		spi_read(sen_spi_device[id], &rd_addr, 1);

		(msgs+1)->buf[0] = (UINT8)(rd_addr & 0xFF);
	}

	return 0;
}

