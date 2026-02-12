#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define DSIZE_6656K     (6656*1024/8) //6.5 Mbps = 6.5*1024*1024/8 Bytes/sec
#define DSIZE_6144K     (6144*1024/8) //6 Mbps = 6*1024*1024/8 Bytes/sec
#define RW_FPS          30
#define RW_DELAY_MS     (1/RW_FPS * 1000)

#define DEFAULT_DEV     "/dev/zero"
#define MAX_PTHREAD_NUM 32

#define max(a, b) ((a) > (b) ? (a) : (b))

//=========================
#if 0 //1 for debug, 0 is default
#define DBG_IND(fmtstr, args...) printf(fmtstr, ##args)
#else //release
#define DBG_IND(...)
#endif
#define DBG_ERR(fmtstr, args...) printf("\033[0;31mERR:%s() \033[0m" fmtstr, __func__, ##args)
#define DBG_WRN(fmtstr, args...) printf("\033[0;33mWRN:%s() \033[0m" fmtstr, __func__, ##args)
#define DBG_DUMP(fmtstr, args...) printf(fmtstr, ##args)
//=========================
typedef struct {
	unsigned long dur[RW_FPS];
	unsigned int count;
} PERF_STAT;

static int g_dev_fd = -1;
static int g_chans_r = 4;
static int g_chans_w = 8;
static int g_continue = 1;
//unsigned long g_perf_dur[MAX_PTHREAD_NUM][RW_FPS] = {0};
PERF_STAT g_perf_stat[MAX_PTHREAD_NUM] = {0};

typedef struct timeval PERF_TICK;

void perf_mark(PERF_TICK *p_tick)
{
	gettimeofday(p_tick, NULL);
}

unsigned long perf_duration(PERF_TICK t_begin, PERF_TICK t_end)
{
	return (t_end.tv_sec - t_begin.tv_sec) * 1000000 + (t_end.tv_usec - t_begin.tv_usec);
}

void perf_dump_array(void)
{
	int chan_idx;
	int frame_idx;
	PERF_STAT *p_stat;
	int dur_sum;

	for (chan_idx = 0; chan_idx < (g_chans_r + g_chans_w) ; chan_idx++) {
		p_stat = &g_perf_stat[chan_idx];
		DBG_DUMP("thread[%2d] %c ", chan_idx, (chan_idx < g_chans_r) ? 'R':'W');

		dur_sum = 0;
		for(frame_idx = 0; frame_idx < RW_FPS; frame_idx++){
			dur_sum += p_stat->dur[frame_idx];
			DBG_DUMP("%3d,", p_stat->dur[frame_idx]);
		}
		DBG_DUMP(" avg %.1f\r\n", (float)dur_sum/RW_FPS);
	}
}

int read_file(void *p_buf, size_t count, off_t offset)
{
	ssize_t ret = 0;

	if (count == 0 || p_buf == NULL)
		return -1;

	if((ret = pread(g_dev_fd, p_buf, count, offset)) == -1) {
		DBG_ERR("pread failed");
		return -1;
	}

	posix_fadvise(g_dev_fd, offset, count, POSIX_FADV_DONTNEED);

	return ret;
}

int write_file(void *p_buf, size_t count, off_t offset)
{
	ssize_t ret = 0;

	if (count == 0 || p_buf == NULL)
		return -1;

	if((ret = pwrite(g_dev_fd, p_buf, count, offset)) == -1) {
		DBG_ERR("pwrite failed");
		return -1;
	}

	sync();

	return ret;
}

static void *thread_r(void *ptr)
{
	int chan_idx = (int)ptr;
	void *p_buf;
	const int buf_size = DSIZE_6144K;

	int ret = 0;
	PERF_TICK t1, t2;
	PERF_STAT *p_stat = &g_perf_stat[chan_idx];

	int frame_idx = 0;
	size_t frame_size = buf_size/RW_FPS;
	off_t off_head = chan_idx * max(DSIZE_6144K, DSIZE_6656K);
	off_t off_frame;

	DBG_DUMP("thread[%2d]R created\r\n", chan_idx);

	//prepare buffer
	p_buf = malloc(buf_size);
	if (NULL == p_buf) {
		DBG_ERR("malloc %d failed\r\n", buf_size); 
		pthread_exit(NULL);
		return NULL;
	}
	memset(p_buf, 0, buf_size);

	//loop to read
	while(g_continue) {
		off_frame = off_head + frame_size * frame_idx;
		DBG_IND("%s[%d] read off=%d size=%d\r\n", __func__, chan_idx, off_frame, frame_size);

		perf_mark(&t1);
		ret = read_file(p_buf, frame_size, off_frame);
		perf_mark(&t2);
		if (ret < 0) {
			DBG_ERR("Error read in read thread\n");
			goto label_thread_r;
		}
		p_stat->dur[frame_idx] = perf_duration(t1, t2);
		p_stat->count++;

		usleep(1000*RW_DELAY_MS);

		frame_idx++;
		if (frame_idx >= RW_FPS) {
			frame_idx = 0;
		}
	}

label_thread_r:
	free(p_buf);
	pthread_exit(NULL);
	return NULL;
}

static void *thread_w(void *ptr)
{
	int chan_idx = (int)ptr;
	void *p_buf;
	const int buf_size = DSIZE_6656K;

	int ret = 0;
	PERF_TICK t1, t2;
	PERF_STAT *p_stat = &g_perf_stat[chan_idx];

	int frame_idx = 0;
	size_t frame_size = buf_size/RW_FPS;
	off_t off_head = chan_idx * max(DSIZE_6144K, DSIZE_6656K);
	off_t off_frame;

	DBG_DUMP("thread[%2d]W created\r\n", chan_idx);

	//prepare buffer
	p_buf = malloc(buf_size);
	if (NULL == p_buf) {
		DBG_ERR("malloc %d failed\r\n", buf_size); 
		pthread_exit(NULL);
		return NULL;
	}
	memset(p_buf, 0, buf_size);

	//loop to write
	while(g_continue) {
		off_frame = off_head + frame_size * frame_idx;
		DBG_IND("%s[%d] write off=%d size=%d\r\n", __func__, chan_idx, off_frame, frame_size);

		perf_mark(&t1);
		ret = write_file(p_buf, frame_size, off_frame);
		perf_mark(&t2);
		if (ret < 0) {
			DBG_ERR("Error read in read thread\n");
			goto label_thread_w;
		}
		p_stat->dur[frame_idx] = perf_duration(t1, t2);
		p_stat->count++;

		usleep(1000*RW_DELAY_MS);

		frame_idx++;
		if (frame_idx >= RW_FPS) {
			frame_idx = 0;
		}
	}

label_thread_w:
	free(p_buf);
	pthread_exit(NULL);
	return NULL;
}

int main(int argc, char **argv)
{
	char *dev_name = DEFAULT_DEV;

	pthread_t pth_hdl[MAX_PTHREAD_NUM] = {0};
	int chan_idx = 0;
	int pth_ret;
	int key;
	
	if (argc >= 2) {
		dev_name = argv[1];
	}

	if (argc >= 3) {
		g_chans_r = atoi(argv[2]);
	}

	if (argc >= 4) {
		g_chans_w = atoi(argv[3]);
	}

	DBG_DUMP("dev_name [%s], g_chans_r %d, g_chans_w %d\r\n", dev_name, g_chans_r, g_chans_w);

	//open the device
	g_dev_fd = open(dev_name, O_RDWR, 666);
	if (-1 == g_dev_fd) {
		DBG_ERR("%s not found\r\n", dev_name);
		return -1;
	}

	//create read thread
	for (chan_idx = 0; chan_idx < g_chans_r; chan_idx++) {
		pth_ret = pthread_create(&pth_hdl[chan_idx], NULL, thread_r, (void *)chan_idx);
		if (0 != pth_ret) {
			DBG_ERR("pthread_create r%d, ret %d\r\n", chan_idx, pth_ret);
			goto label_end;
		}
	}

	//create write thread
	for (chan_idx = g_chans_r; chan_idx < (g_chans_r + g_chans_w); chan_idx++) {
		pth_ret = pthread_create(&pth_hdl[chan_idx], NULL, thread_w, (void *)chan_idx);
		if (0 != pth_ret) {
			DBG_ERR("pthread_create w%d, ret %d\r\n", chan_idx, pth_ret);
			goto label_end;
		}
	}

	//wait for commands
	sleep(1);
	while (1) {
		printf("Enter q to exit, p to print\n");
		key = getchar();
		if (key == 'p') {
			perf_dump_array();
		} else if (key == 'q') {
			g_continue = 0;
			break;
		}
	}

	//wait all threads done
	for (chan_idx = 0; chan_idx < (g_chans_r + g_chans_w); chan_idx++) {
		pth_ret = pthread_join(pth_hdl[chan_idx], NULL);
		if (0 != pth_ret) {
			DBG_ERR("pthread_join failed, ret %d\r\n", pth_ret);
			goto label_end;
		}
		DBG_DUMP("thread[%2d]%c exit\r\n", chan_idx, (chan_idx < g_chans_r) ? 'R':'W');
	}

label_end:
	//close the device
	if (-1 == close(g_dev_fd)) {
		DBG_ERR("close %s failed\r\n", dev_name);
	}

    return 0;
}
