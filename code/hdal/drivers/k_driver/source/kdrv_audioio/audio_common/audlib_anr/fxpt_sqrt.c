#ifdef __KERNEL__
#else
#include <stdint.h>
#include <stdio.h>
#endif
//=============================================================================
// fxpt_sqrt: Fixed-point square root approximation
// Parameter:
//      x       Qu31 format 32-bit unsigned integer value and <= 0x07ffffff
// Return:
//      y       Qu16 format
//=============================================================================
// fxpt_sqrt(0x7fffffff) = 0xb504
// fxpt_sqrt(0x40000000) = 0x8000
unsigned short fxpt_sqrt(unsigned int n);
unsigned short fxpt_sqrt(unsigned int n)
{
	unsigned int root = 0, ttry;

#define iter1(X) \
	ttry = root + (1 << (X)); \
	if (n >= ttry << (X))   \
	{   n -= ttry << (X);   \
		root |= 2 << (X); \
	}

	iter1(15);
	iter1(14);
	iter1(13);
	iter1(12);
	iter1(11);
	iter1(10);
	iter1(9);
	iter1(8);
	iter1(7);
	iter1(6);
	iter1(5);
	iter1(4);
	iter1(3);
	iter1(2);
	iter1(1);
	iter1(0);

	return (unsigned short)(root >> 1);
}

