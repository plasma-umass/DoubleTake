/*
 *	this program is released in the Public Domain
 */
#ifndef __cplusplus
#	include <stdio.h>
#	include <stdlib.h>
#else
#	include <cstdio>
#	include <cstdlib>
#endif
#include <libHX/init.h>
#include <libHX/misc.h>

int main(void)
{
	void *d;
	const char *n;

	if (HX_init() <= 0)
		abort();
	d = HXdir_open("/tmp");
	printf("Available files in /tmp:\n");
	while ((n = HXdir_read(d)) != NULL)
		printf("\t" "%s\n", n);

	HXdir_close(d);
	HX_exit();
	return EXIT_SUCCESS;
}
