/*
 *	this program is released in the Public Domain
 */
#ifndef __cplusplus
#	include <stdlib.h>
#else
#	include <cstdlib>
#endif
#include <sys/stat.h>
#include <libHX/init.h>
#include <libHX/misc.h>

int main(int argc, const char **argv)
{
	unsigned int n;
	struct stat sa, sb;

	if (HX_init() <= 0)
		abort();
	printf("%d\n", HX_ffs(0));
	for (n = 1; ; n <<= 1) {
		printf("%08x = %d\n", n, HX_ffs(n));
		if (n & 0x80000000)
			break;
	}
	printf("---\n");
	for (n = 1; ; n <<= 1, n |= 1) {
		printf("%08x = %d\n", n, HX_ffs(n));
		if (n == ~0U)
			break;
	}

	if (argc >= 3) {
		if (stat(argv[1], &sa) < 0 ||
		    stat(argv[2], &sb) < 0)
			perror("stat");
		printf("Difference: %ld\n", HX_time_compare(&sa, &sb, 'm'));
	}

	HX_exit();
	return EXIT_SUCCESS;
}
