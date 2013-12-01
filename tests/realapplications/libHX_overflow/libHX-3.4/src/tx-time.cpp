/*
 *	this program is released in the Public Domain
 */
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <libHX/defs.h>
#include <libHX/init.h>
#include <libHX/misc.h>
#include "internal.h"

static unsigned int sleep_amt = 1336670;

int main(int argc, const char **argv)
{
	struct timeval  m_past, m_future, m_delta;
	struct timespec n_past, n_future, n_delta;

	if (HX_init() <= 0)
		abort();

	if (argc >= 2)
		sleep_amt = strtoul(argv[1], NULL, 0);
	printf("µsec sleep: %u\n", sleep_amt);

	clock_gettime(CLOCK_REALTIME, &n_past);
	gettimeofday(&m_past, NULL);
	usleep(sleep_amt);
	clock_gettime(CLOCK_REALTIME, &n_future);
	gettimeofday(&m_future, NULL);

	HX_diff_timeval(&m_delta, &m_future, &m_past);
	printf("µsec: %ld.%06ld -> %ld.%06ld = %ld.%06ld\n",
	       static_cast(long, m_past.tv_sec),
	       static_cast(long, m_past.tv_usec),
	       static_cast(long, m_future.tv_sec),
	       static_cast(long, m_future.tv_usec),
	       static_cast(long, m_delta.tv_sec),
	       static_cast(long, m_delta.tv_usec));

	HX_diff_timespec(&n_delta, &n_future, &n_past);
	printf("nsec: %ld.%09ld -> %ld.%09ld = %ld.%09ld\n",
	       static_cast(long, n_past.tv_sec),
	       static_cast(long, n_past.tv_nsec),
	       static_cast(long, n_future.tv_sec),
	       static_cast(long, n_future.tv_nsec),
	       static_cast(long, n_delta.tv_sec),
	       static_cast(long, n_delta.tv_nsec));

	HX_diff_timespec(&n_delta, &n_past, &n_future);
	printf("ns-1: %ld.%09ld -> %ld.%09ld = %ld.%09ld\n",
	       static_cast(long, n_past.tv_sec),
	       static_cast(long, n_past.tv_nsec),
	       static_cast(long, n_future.tv_sec),
	       static_cast(long, n_future.tv_nsec),
	       static_cast(long, n_delta.tv_sec),
	       static_cast(long, n_delta.tv_nsec));

	HX_exit();
	return EXIT_SUCCESS;
}
