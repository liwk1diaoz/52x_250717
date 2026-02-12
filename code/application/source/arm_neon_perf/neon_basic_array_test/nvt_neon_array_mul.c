#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arm_neon.h>

#define NUM 5

void test(uint32_t w, uint32_t h, uint32_t times);
void routine(uint8_t *arr, const uint8_t *num, uint32_t w, uint32_t h, uint32_t times);
void neon_routine(uint8_t *arr, const uint8_t *num, uint32_t w, uint32_t h, uint32_t times);

int main(void)
{

	test(128, 128,   51200);
	test(256, 256,   12800);
	test(512, 512,    1600);
	test(1024, 1024,   400);
	test(2048, 2048,   100);

	return 0;
}

void test(uint32_t w, uint32_t h, uint32_t times)
{
	uint32_t i;
	uint8_t *arr;
	struct timespec start, end;
	uint32_t diff;
	const uint8_t num = NUM;

	arr = (uint8_t *)malloc(w * h);
	if (arr == NULL)
		return;

	for (i = 0; i < (w * h); i++) { // Populates the input arrays
		arr[i] = (i & 0xFF) / 5;
	}

	clock_gettime(CLOCK_REALTIME, &start);
	routine(arr, &num, w, h, times);
	clock_gettime(CLOCK_REALTIME, &end);
	diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
	printf("multiplying %4ux%4u matrix  w/o NEON:  %5u us\n", w, h, diff / times);

	for (i = 0; i < (w * h); i++) { // Populates the input arrays
		arr[i] = (i & 0xFF) / 5;
	}

	clock_gettime(CLOCK_REALTIME, &start);
	neon_routine(arr, &num, w, h, times);
	clock_gettime(CLOCK_REALTIME, &end);
	diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
	printf("multiplying %4ux%4u matrix with NEON:  %5u us\n", w, h, diff / times);

	free(arr);
}

void neon_routine(uint8_t *arr, const uint8_t *num, uint32_t w, uint32_t h, uint32_t times)
{

	uint32_t i, j;
	uint8x16_t arr_;
	uint8x16_t num_ = vld1q_dup_u8(num);
	for (j = 0; j < times; j++) {
		for (i = 0; i < (w * h); i += 16) {
			arr_ = vld1q_u8(arr + i); // arr_ = arr
			arr_ = vmulq_u8(arr_, num_); // arr_ = arr_ + num_
			vst1q_u8((arr + i), arr_); // arr = arr_
		}
	}
}

void routine(uint8_t *arr, const uint8_t *num, uint32_t w, uint32_t h, uint32_t times)
{

	uint32_t i, j;

	if (arr == NULL || num == NULL) {
		printf("Your argument is null, please check it.");
		return;
	}

	for (j = 0; j < times; j++) {
		for (i = 0; i < (w * h); i++) {
			arr[i] = arr[i] * num[0];
		}
	}
}
