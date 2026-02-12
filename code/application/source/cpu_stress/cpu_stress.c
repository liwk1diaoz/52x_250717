#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sched.h> 

//#define DBG_PRINT
void cpu_stress_test1(void *ptr);
void cpu_stress_test2(void *ptr);
void power_loop(unsigned long loop, unsigned long output_addr);

char *output_buf1, *output_buf2;

int main(int argc, char **argv)
{
	int i = 1;
	unsigned long loop = 100;
	pthread_t thread1, thread2;  /* thread variables */

	printf("Starting to test cpu stress \r\n");
	if (argc == 2) {
		sscanf(argv[i], "%lx", &loop);
	}

	output_buf1 = malloc(loop * 16);
	memset(output_buf1, 0, loop * 16);
	output_buf2 = malloc(loop * 16);
	memset(output_buf2, 0, loop * 16);

	printf("Creating two thread to test cpu stress \r\n");
	pthread_create (&thread1, NULL, (void *) &cpu_stress_test1, (void *) &loop);
	pthread_create (&thread2, NULL, (void *) &cpu_stress_test2, (void *) &loop);
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	// to compare two output buffer value
	if (memcmp(output_buf1, output_buf2, loop * 16) != 0) {
		printf("ERROR!!!!!!!!! We can't pass this testing\n");
		return -1;
	} else {
		printf("GOOD!!! We pass this cpu testing.\n");
	}
	free(output_buf1);
	free(output_buf2);
	printf("Finish testing\r\n");
	return 0;
}

void cpu_stress_test1(void *ptr)
{
	unsigned long *loop;
	int i = 0;
	cpu_set_t	cpuset;

	
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);

	if(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) <0) {
		perror("cpu_stress_test1 can't set affinity\n");
	}

	loop = (unsigned long *) ptr;  /* type cast to a pointer to thdata */

	printf("%s %d create successfully\n", __func__, __LINE__);
	power_loop(*loop, (unsigned long)output_buf1);

	printf("Finish testing %s\r\n", __func__);
#ifdef	DBG_PRINT
	for (i=0;i<(int)*loop * 16;i++)
	{
		printf("0x%x ", output_buf1[i]);
	}
	printf("\n");
#endif
	pthread_exit(0); /* exit */
}

void cpu_stress_test2(void *ptr)
{
	unsigned long *loop;
	int i = 0;
	cpu_set_t	cpuset;

	CPU_ZERO(&cpuset);
	CPU_SET(1, &cpuset);

	if(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) <0) {
		perror("cpu_stress_test1 can't set affinity\n");
	}

	loop = (unsigned long *) ptr;  /* type cast to a pointer to thdata */

	printf("%s %d create successfully\n", __func__, __LINE__);
	power_loop(*loop, (unsigned long)output_buf2);

	printf("Finish testing %s\r\n", __func__);
#ifdef	DBG_PRINT
	for (i=0;i<(int)*loop * 16;i++)
	{
		printf("0x%x ", output_buf2[i]);
	}
	printf("\n");
#endif
	pthread_exit(0); /* exit */
}