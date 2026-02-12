#define MANCHESTER_BUFFER_SIZE 3072
#define DECODE_BUFFER_SIZE 1536

struct em4100 {
	struct spi_device *spi;
	struct mutex lock;
    //spinlock_t		spi_lock;

    char manchester_buf[MANCHESTER_BUFFER_SIZE];
    char decode_buf[DECODE_BUFFER_SIZE];

    int (*read)(struct em4100 *spidev, size_t len, char *buf);
};

int em4100_api_read(char *buf, int len, int mode, int *id, int *data);


