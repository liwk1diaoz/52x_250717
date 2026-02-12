#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int i = 0;
    float a[256],b[256],c[256];

    for (i=0;i<256;i++) {
	a[i] = b[i] + c[i];
	a[i] = b[i] * c[i];
	a[i] = b[i] / c[i];
    }

    printf("Hello World! \r\n");

    return 0;
}
