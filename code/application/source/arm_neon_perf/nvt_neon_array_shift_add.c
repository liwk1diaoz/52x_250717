/**
    NVT neon array testing
    To verify array calculation performance
    @file       nvt_array_shift_add.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arm_neon.h>

#define SIZE 2*1024*1024 // Arrays size

void routine(uint8_t *arr2, uint8_t *arr1, uint8_t *arr0);
void neon_routine(uint8_t *arr2, uint8_t *arr1, uint8_t *arr0);

int main(int argc, char **argv)
{
	uint32_t i = 0;
	uint8_t *arr0 = NULL;
	uint8_t *arr1 = NULL;
	uint8_t *arr2 = NULL;

	arr0 = (uint8_t *)malloc(SIZE);
	if (arr0 == NULL) {
		printf("Allocate failed at array 0\n");
		return 0;
	}
	arr1 = (uint8_t *)malloc(SIZE);
	if (arr1 == NULL) {
		free(arr0);
		printf("Allocate failed at array 1\n");
		return 0;
	}
	arr2 = (uint8_t *)malloc(SIZE);
	if (arr2  == NULL) {
		free(arr0);
		free(arr1);
		printf("Allocate failed at array 2\n");
		return 0;
	}

	struct timespec start, end;
	uint32_t diff;

	printf("\nStarting the NEON test...\n");

	for (i = 0; i < SIZE; i++) {
		arr0[i] = i;
		arr1[i] = i;
	}

	printf("Starting the test routine w/o NEON.\n");
	clock_gettime(CLOCK_REALTIME, &start);

	routine(arr2, arr1, arr0);

	clock_gettime(CLOCK_REALTIME, &end);
	diff = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
	printf("Elapsed time: %u ms\n", diff);

	printf("Starting the test routine with NEON.\n");
	clock_gettime(CLOCK_REALTIME, &start);

	neon_routine(arr2, arr1, arr0);

	clock_gettime(CLOCK_REALTIME, &end);
	diff = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
	printf("Elapsed time: %u ms\n\n", diff);

	free(arr0);
	free(arr1);
	free(arr2);

	return 0;
}

void neon_routine(uint8_t *arr2, uint8_t *arr1, uint8_t *arr0)
{

	uint32_t i;
	uint8x16_t arr0_;
	uint8x16_t arr1_;

	for (i = 0; i < SIZE; i += 16) {
		arr0_ = vld1q_u8(arr0 + i);     // arr0_ = arr0
		arr0_ = vshrq_n_u8(arr0_, 1);       // arr0_ = arr0_ >> 1

		arr1_ = vld1q_u8(arr1 + i);     // arr1_ = arr1
		arr0_ = vsraq_n_u8(arr0_, arr1_, 1);    // arr0_ = arr0_ + (arr1_ >> 1)

		vst1q_u8((arr2 + i), arr0_);    // arr2 = arr0_
	}
}

void routine(uint8_t *arr2, uint8_t *arr1, uint8_t *arr0)
{

	uint32_t i;

	for (i = 0; i < SIZE; i++) {
		arr2[i] = (arr0[i] >> 1) + (arr1[i] >> 1);
	}
}
