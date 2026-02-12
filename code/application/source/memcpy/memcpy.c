#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "util.h"

#define CHUNK_SZ   (960*540*4)
#define NR_CHUNK   2000
#define CACHE_LINE  64

int main(void)
{
	unsigned long elapsed;
	
	char *p1 = (char*)malloc(CHUNK_SZ);
	char *p2 = (char*)malloc(CHUNK_SZ);
	volatile unsigned long *up1 = (unsigned long *)p1;
	volatile unsigned long *up2 = (unsigned long *)p2;
	volatile unsigned short *sp1 = (unsigned short *)p1;
	volatile unsigned short *sp2 = (unsigned short *)p2;

	int i, j;

	printf("memcpy v.1.2\n");

	memset(p1, 0x55, CHUNK_SZ); // need to alloc physically
	memset(p2, 0xaa, CHUNK_SZ);

	start_watch();

	for(i=0; i<NR_CHUNK; i++)
	{
		memcpy(p1, p2, CHUNK_SZ);
	}

	elapsed = stop_watch("memcpy");
	printf("BandWidth %fMB/s\n", 1.0*NR_CHUNK*CHUNK_SZ/elapsed/1000/*ms*/);

	start_watch();

	for(i=0; i<NR_CHUNK; i++)
	{
		for(j=0; j<CHUNK_SZ/4/2; j++)
		{
			*(up1 + j*2) = *(up2 + j*2);
			*(up2 + j*2+1) = *(up1 + j*2+1);
		}
	}
	
	elapsed = stop_watch("4byte copy");
	printf("BandWidth %fMB/s\n", 1.0*NR_CHUNK*CHUNK_SZ/elapsed/1000/*ms*/);

	start_watch();

	for(i=0; i<NR_CHUNK; i++)
	{
		for(j=0; j<CHUNK_SZ/2/2; j++)
		{
			*(sp1 + j*2) = *(sp2 + j*2);
			*(sp2 + j*2+1) = *(sp1 + j*2+1);
		}
	}
	
	elapsed = stop_watch("2byte copy");
	printf("BandWidth %fMB/s\n", 1.0*NR_CHUNK*CHUNK_SZ/elapsed/1000/*ms*/);

	start_watch();

	for(i=0; i<NR_CHUNK; i++)
	{
		for(j=0; j<CHUNK_SZ/2; j++)
		{
			*(p1 + j*2) = *(p2 + j*2);
			*(p2 + j*2+1) = *(p1 + j*2+1);
		}
	}
	
	elapsed = stop_watch("1byte copy");
	printf("BandWidth %fMB/s\n", 1.0*NR_CHUNK*CHUNK_SZ/elapsed/1000/*ms*/);

	start_watch();

	for(i=0; i<NR_CHUNK; i++)
	{
		for(j=0; j<CHUNK_SZ/4; j+=(CACHE_LINE/4))
		{
			*(up1 + j) = *(up2 + j);
		}
	}
	
	elapsed = stop_watch("Over cache line 4byte");
	printf("BandWidth %fMB/s\n", 1.0*NR_CHUNK*CHUNK_SZ/elapsed/1000/*ms*//CACHE_LINE*4/*copy bytes*/);

	free(p2);
	free(p1);
	
	return 0;
}
