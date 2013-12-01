/* This file is for testing the cumulative include */
#ifndef __cplusplus
#	include <stdlib.h>
#else
#	include <cstdlib>
#endif
#include <libHX.h>

#define ZZ 64

int main(void)
{
	unsigned long bitmap[HXbitmap_size(unsigned long, 64)];

	if (HX_init() <= 0)
		abort();
	printf("sizeof bitmap: %zu, array_size: %zu\n",
	       sizeof(bitmap), ARRAY_SIZE(bitmap));
	HXbitmap_set(bitmap, 0);
	printf(HX_STRINGIFY(1234+2 +2) "," HX_STRINGIFY(ZZ) "\n");
	HX_exit();
	return EXIT_SUCCESS;
}
