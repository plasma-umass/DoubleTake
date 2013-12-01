/*
 *	Random numbers
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2003 - 2008
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2.1 or 3 of the License.
 */
#include "config.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef __unix__
#	include <unistd.h>
#endif
#ifdef HAVE_GETTIMEOFDAY
#	include <sys/time.h>
#endif
#include <libHX/init.h>
#include <libHX/misc.h>
#include "internal.h"

static unsigned int HXrand_obtain_seed(void)
{
	unsigned int s;

#if defined(HAVE_GETTIMEOFDAY)
	struct timeval tv;

	gettimeofday(&tv, NULL);
	s  = tv.tv_sec;
	s ^= tv.tv_usec << 16;
#else
	s = time(NULL);
#endif
#ifdef HAVE_GETPID
	s ^= getpid() << 9;
#endif
#ifdef HAVE_GETPPID
	s ^= getppid() << 1;
#endif
#ifdef HAVE_GETEUID
	s ^= geteuid() << 13;
#endif
#ifdef HAVE_GETEGID
	s ^= getegid() << 5;
#endif
	return s;
}

static __attribute__((constructor)) void HXrand_init(void)
{
	unsigned int seed;
	int fd, ret = 0;

//  fprintf(stderr, "******inside the HXrand_init***********\n");
	if ((fd = open("/dev/urandom", O_RDONLY | O_BINARY)) >= 0) {
//    fprintf(stderr, "******inside the HXrand_init, open fd %d***********\n", fd);
		ret = read(fd, &seed, sizeof(seed));
//    fprintf(stderr, "******inside the HXrand_init, READ fd %d***********\n", fd);
		close(fd);
	}
	if (ret != sizeof(seed))
		seed = HXrand_obtain_seed();
	srand(seed);
}

static pthread_mutex_t HX_init_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned int HX_use_count;

static void __attribute__((constructor)) HX_ident(void)
{
	if (getenv("LIBHX_IDENTIFY") != NULL)
		fprintf(stderr, PACKAGE_NAME " " PACKAGE_VERSION " "
		        __DATE__ " " __TIME__ "\n");
}

EXPORT_SYMBOL int HX_init(void)
{
	pthread_mutex_lock(&HX_init_lock);
	if (HX_use_count == 0)
		HXrand_init();
	++HX_use_count;
	pthread_mutex_unlock(&HX_init_lock);
	return 1;
}

EXPORT_SYMBOL void HX_exit(void)
{
	pthread_mutex_lock(&HX_init_lock);
	if (HX_use_count == 0)
		fprintf(stderr, "%s: reference count is already zero!\n", __func__);
	else
		--HX_use_count;
	pthread_mutex_unlock(&HX_init_lock);
}

EXPORT_SYMBOL int HX_rand(void)
{
	/*
	 * If there is an overly broken system, we may need to use
	 * alternate methods again (/dev/urandom?)
	 */
	return rand();
}

EXPORT_SYMBOL double HX_drand(double lo, double hi)
{
	double delta = hi - lo;

	return static_cast(double, rand()) * delta / RAND_MAX + lo;
}

EXPORT_SYMBOL unsigned int HX_irand(unsigned int lo, unsigned int hi)
{
	unsigned int delta = hi - lo;

	if (delta <= RAND_MAX)
		return HX_rand() % delta + lo;
	else
		return static_cast(unsigned int,
		       static_cast(double, rand()) * delta / RAND_MAX) + lo;
}
