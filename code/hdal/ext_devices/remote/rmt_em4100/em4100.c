
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/spi/spi.h>

#include "em4100.h"
#include "em4100_core.h"
#include "em4100_proc.h"
#include "rmt_dbg.h"

struct em4100 *g_spidev;

static struct of_device_id spidev_match_table[] = {
	{	.compatible = "em4100-spi"},
	{}
};

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef __KERNEL__
char *rmt_cfg_path = "null";
module_param_named(rmt_cfg_path, rmt_cfg_path, charp, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rmt_cfg_path, "Path of cfg file");

#ifdef DEBUG
unsigned int rmt_debug_level = THIS_DBGLVL;
module_param_named(rmt_debug_level, rmt_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rmt_debug_level, "Debug message level");
#endif
#else
char *rmt_cfg_path = "null";
unsigned int rmt_debug_level = THIS_DBGLVL;
#endif
//=============================================================================

static int em4100_spi_read(struct em4100 *spidev, size_t len, char *buf)
{
    int r_count = 0;
    long ms, us, ns;
    struct spi_message msg;
    struct spi_transfer st[2];
    struct spi_device *spi;
    struct timespec ts_start,ts_end,test_of_time;

    DECLARE_COMPLETION_ONSTACK(done);

    DBG_FUNC( "%s was called\n", __func__ );

	spi_message_init( &msg );

	memset( st, 0, sizeof(st) );
	st[0].rx_buf = buf;
	st[0].len = len;
	spi_message_add_tail( &st[0], &msg );

    //spin_lock_irq(&spidev->spi_lock);
    spi = spidev->spi;
    //spin_unlock_irq(&spidev->spi_lock);

    getnstimeofday(&ts_start);
	mutex_lock( &spidev->lock );
	r_count = spi_sync( spi, &msg );
	if (r_count == 0){
	    r_count = msg.actual_length;
    }
	mutex_unlock( &spidev->lock );
    getnstimeofday(&ts_end);
	test_of_time = timespec_sub(ts_end,ts_start);
	us = (test_of_time.tv_nsec / 1000) % 1000;
	ms = (test_of_time.tv_sec * 1000) + ((test_of_time.tv_nsec / 1000000) % 1000);
	ns = test_of_time.tv_nsec % 1000;
	DBG_IND("SPI read timer: %4lu.%.3lu%.3lu ms\n", ms, us, ns);

    if (r_count < 0){
        pr_err("spi reading error\n");
    } else {
        DBG_IND( "in (%s): read %d bytes\n", __func__, r_count );
    }

	return r_count;
}

/*-------------------------------------------------------------------------*/

int em4100_api_read(char *buf, int len, int mode, int *id, int *data)
{
    int r_count = 0;
    char packet_buf[64];

    DBG_FUNC( "%s was called\n", __func__ );

    if (len < 1024) {
        pr_info("read length(%d) is too small.\n", len);
        return -1;
    } else if (len > 8192) {
        pr_info("read length(%d) is too long.\n", len);
        return -1;
    }

    *id = 0;
    *data = 0;
    memset(g_spidev->manchester_buf, 0, MANCHESTER_BUFFER_SIZE);
    memset(g_spidev->decode_buf, 0, DECODE_BUFFER_SIZE);

    r_count = g_spidev->read(g_spidev, len, buf);

    if (r_count < 0) {
        //dev_err(spidev->dev, "Read cmd 0x%04X failed\n", cmd);
        pr_err("em4100_spi_read() return error\n");
        return -1;
    }

    nvt_dbg(IND, "\n\nData reduction:\n");
    em4100_data_reduction(buf, len, g_spidev->manchester_buf);

    nvt_dbg(IND, "\n\nManchester decode:\n");
    em4100_manchester_decoding(g_spidev->manchester_buf, mode, g_spidev->decode_buf);

    nvt_dbg(IND, "\n\nSearch RFID Tag header:\n");
    em4100_search_header(g_spidev->decode_buf, packet_buf, NULL);

    nvt_dbg(IND, "\n\nGet Tag ID and Data:\n");
    em4100_getID_Data(packet_buf, id, data);

    return r_count;
}
EXPORT_SYMBOL_GPL(em4100_api_read);

/*-------------------------------------------------------------------------*/

static int em4100_spi_probe(struct spi_device *spi)
{
    struct em4100 *spidev;
    const struct of_device_id *match;

    match = of_match_device(spidev_match_table, &spi->dev);
    if (!match) {
        pr_err("Platform device not found \n");
		return -EINVAL;
	}

	spidev = kzalloc( sizeof(struct em4100), GFP_KERNEL );
	if( !spidev ) {
        pr_err("failed to allocate memory\n");
		return -ENOMEM;
	}
	spidev->spi = spi;
    mutex_init( &spidev->lock );
    spidev->read = em4100_spi_read;
    g_spidev = spidev;

    spi_set_drvdata(spi, spidev);

    // init proc interface
    em4100_proc_init();

    pr_info("em4100 reader probe done.\n");

	return 0;
}

static int em4100_spi_remove(struct spi_device *spi)
{
    struct em4100 *spidev;

    spidev = spi_get_drvdata(spi);
    kfree(spidev);
    spidev = NULL;
    spi_set_drvdata(spi, spidev);
    pr_info("em4100 reader remove. \n");

	return 0;
}

static struct spi_driver em4100_spi_driver = {
	.driver = {
		.name	= "nvt,spidev_em4100",
        .of_match_table = spidev_match_table,
	},
	.probe  = em4100_spi_probe,
	.remove = em4100_spi_remove,
};

/*-------------------------------------------------------------------------*/

int __init em4100_module_init(void)
{
    int ret ;

    pr_info("Start em4095/em4100 RFID Tag Reader driver. \n");

    ret = spi_register_driver(&em4100_spi_driver);
	if (ret < 0) {
        pr_info("spi register driver fail. \n");
	}

	return ret;
}

void __exit em4100_module_exit(void)
{
    pr_info("RFID Reader driver unloaded.\n");

    spi_unregister_driver(&em4100_spi_driver);

    // remove proc interface
    em4100_proc_exit();
}

module_init(em4100_module_init);
module_exit(em4100_module_exit);

MODULE_DESCRIPTION("Novatek em4095/4100 receiver driver");
MODULE_AUTHOR("Novatek Corp.");
MODULE_VERSION("1.00.02");
MODULE_LICENSE("GPL");


