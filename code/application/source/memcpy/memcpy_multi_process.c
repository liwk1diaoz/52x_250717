#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "util.h"

//#define NEON_MEMCPY

#define CHUNK_SZ   (960*540*4) //must 256 bytes align for neon copy
#define NR_CHUNK   10000
#define CACHE_LINE  64

int main(void)
{
	unsigned long elapsed;

	char *p1 = (char*)aligned_alloc(32, CHUNK_SZ);
	char *p2 = (char*)aligned_alloc(32, CHUNK_SZ);

	pid_t PID;
	int i;

	PID = fork();
	PID = fork();

	switch(PID){
	// PID == -1 is error
	case -1:
		perror("fork()");
		exit(-1);
		break;

	// PID == 0 child process
	case 0:
		printf("Running child memcpy process PID=%d\n", PID);
		break;

        // PID > 0 main process
        default:
		printf("memcpy v.1.2\n");
		printf("Running main memcpy process\n");
		break;
	}

	memset(p1, 0x55, CHUNK_SZ); // need to alloc physically
	memset(p2, 0xaa, CHUNK_SZ);

	start_watch();

	for(i=0; i<NR_CHUNK; i++)
	{
#if defined(NEON_MEMCPY)
		__asm__ __volatile__(
			"mov r0, %0 \n\t"
			"mov r1, %1 \n\t"
			"mov r2, %2 \n\t"
			"loop: \n\t"
			"subs r0, r0, #256 \n\t"
			"vld1.32	{d0-d3}, [r1 :256]! \n\t"
			"vld1.32	{d4-d7}, [r1 :256]! \n\t"
			"pld [r1, #512] \n\t"
			"vld1.32	{d8-d11}, [r1 :256]! \n\t"
			"vld1.32	{d12-d15}, [r1 :256]! \n\t"
			"pld [r1, #512] \n\t"
			"vld1.32	{d16-d19}, [r1 :256]! \n\t"
			"vld1.32	{d20-d23}, [r1 :256]! \n\t"
			"pld [r1, #512] \n\t"
			"vld1.32	{d24-d27}, [r1 :256]! \n\t"
			"vld1.32	{d28-d31}, [r1 :256]! \n\t"
			"pld [r1, #512] \n\t"
			"vst1.32	{d0-d3}, [r2]! \n\t"
			"vst1.32	{d4-d7}, [r2]! \n\t"
			"vst1.32	{d8-d11}, [r2]! \n\t"
			"vst1.32	{d12-d15}, [r2]! \n\t"
			"vst1.32	{d16-d19}, [r2]! \n\t"
			"vst1.32	{d20-d23}, [r2]! \n\t"
			"vst1.32	{d24-d27}, [r2]! \n\t"
			"vst1.32	{d28-d31}, [r2]! \n\t"
			"bgt loop \n\t"
			:
			: "r"(CHUNK_SZ), "r"(p1), "r"(p2)
			: "memory","r0","r1","r2"
		);
#else
		memcpy(p1, p2, CHUNK_SZ);
#endif
	}



	elapsed = stop_watch("memcpy");
	printf("BandWidth %fMB/s\n", 1.0*NR_CHUNK*CHUNK_SZ/elapsed/1000/*ms*/);

	free(p2);
	free(p1);

	return 0;
}
