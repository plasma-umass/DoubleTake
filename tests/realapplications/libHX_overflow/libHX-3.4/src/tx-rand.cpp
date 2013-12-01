/*
 *	this program is released in the Public Domain
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libHX/init.h>
#include <libHX/misc.h>
#include "internal.h"

int main(void)
{
	struct timespec past, now, delta;
	unsigned int i;

	if (HX_init() <= 0)
		abort();
	for (i = 0; i < 15; ++i)
		printf("%08x ", HX_irand(0, RAND_MAX));
	printf("\n");

	clock_gettime(CLOCK_REALTIME, &past);
	for (i = 0; i < (1 << 25); ++i) {
		volatile unsigned int __attribute__((unused)) t =
			HX_irand(0, RAND_MAX);
	}
	clock_gettime(CLOCK_REALTIME, &now);
	HX_diff_timespec(&delta, &now, &past);
	printf("%% method: %ld.%06ld s\n",
	       static_cast(long, delta.tv_sec), delta.tv_nsec / 1000);

	clock_gettime(CLOCK_REALTIME, &past);
	for (i = 0; i < (1 << 25); ++i) {
		volatile unsigned int __attribute__((unused)) t =
			HX_irand(0, ~0U);
	}
	clock_gettime(CLOCK_REALTIME, &now);
	HX_diff_timespec(&delta, &now, &past);
	printf("/ method: %ld.%06ld s\n",
	       static_cast(long, delta.tv_sec), delta.tv_nsec / 1000);

	HX_exit();
	return EXIT_SUCCESS;
}
