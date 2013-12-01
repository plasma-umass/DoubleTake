/*
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2009
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2.1 or 3 of the License.
 */
#include <sys/stat.h>
#include <sys/time.h>
#include <stdbool.h>
#include <time.h>
#include <libHX/misc.h>
#include "internal.h"

enum {
	MICROSECOND = 0xf4240,
	NANOSECOND  = 0x3b9aca00,
};

#ifdef HAVE_STRUCT_TIMESPEC_TV_NSEC
/**
 * Calculates @future - @past. You can also swap them and correctly
 * get a negative time.
 */
EXPORT_SYMBOL void HX_diff_timespec(struct timespec *delta,
    const struct timespec *future, const struct timespec *past)
{
	delta->tv_sec  = future->tv_sec  - past->tv_sec;
	delta->tv_nsec = future->tv_nsec - past->tv_nsec;
	if (future->tv_sec < past->tv_sec || (future->tv_sec == past->tv_sec &&
	    future->tv_nsec < past->tv_nsec)) {
		if (future->tv_nsec > past->tv_nsec) {
			delta->tv_nsec = -NANOSECOND + delta->tv_nsec;
			++delta->tv_sec;
		}
		if (delta->tv_sec < 0)
			delta->tv_nsec *= -1;
	} else if (delta->tv_nsec < 0) {
		delta->tv_nsec += NANOSECOND;
		--delta->tv_sec;
	}
}
#endif

#ifdef HAVE_STRUCT_TIMEVAL_TV_USEC
EXPORT_SYMBOL void HX_diff_timeval(struct timeval *delta,
    const struct timeval *future, const struct timeval *past)
{
	delta->tv_sec  = future->tv_sec  - past->tv_sec;
	delta->tv_usec = future->tv_usec - past->tv_usec;
	if (future->tv_sec < past->tv_sec || (future->tv_sec == past->tv_sec &&
	    future->tv_usec < past->tv_usec)) {
		if (future->tv_usec > past->tv_usec) {
			delta->tv_usec = -MICROSECOND + delta->tv_usec;
			++delta->tv_sec;
		}
		if (delta->tv_sec < 0)
			delta->tv_usec *= -1;
	} else if (delta->tv_usec < 0) {
		delta->tv_usec += MICROSECOND;
		--delta->tv_sec;
	}
}
#endif

EXPORT_SYMBOL long HX_time_compare(const struct stat *a,
    const struct stat *b, char sel)
{
	long r;

#if defined(HAVE_STRUCT_STAT_ST_MTIMENSEC)
	if (sel == 'm')
		return ((r = a->st_mtime - b->st_mtime) != 0) ?
		       r : a->st_mtimensec - b->st_mtimensec;
#ifdef HAVE_STRUCT_STAT_ST_OTIMENSEC
	else if (sel == 'o')
		return ((r = a->st_otime - b->st_otime) != 0) ?
		       r : a->st_otimensec - b->st_otimensec;
#endif
	else if (sel == 'a')
		return ((r = a->st_atime - b->st_atime) != 0) ?
		       r : a->st_atimensec - b->st_atimensec;
	else if (sel == 'c')
		return ((r = a->st_ctime - b->st_ctime) != 0) ?
		       r : a->st_ctimensec - b->st_ctimensec;
#elif defined(HAVE_STRUCT_STAT_ST_MTIM)
	if (sel == 'm')
		return ((r = a->st_mtim.tv_sec - b->st_mtim.tv_sec) != 0) ?
		       r : a->st_mtim.tv_nsec - b->st_mtim.tv_nsec;
#ifdef HAVE_STRUCT_STAT_ST_OTIM
	else if (sel == 'o')
		return ((r = a->st_otim.tv_sec - b->st_otim.tv_sec) != 0) ?
		       r : a->st_otim.tv_nsec - b->st_otim.tv_nsec;
#endif
	else if (sel == 'a')
		return ((r = a->st_atim.tv_sec - b->st_atim.tv_sec) != 0) ?
		       r : a->st_atim.tv_nsec - b->st_atim.tv_nsec;
	else if (sel == 'c')
		return ((r = a->st_ctim.tv_sec - b->st_ctim.tv_sec) != 0) ?
		       r : a->st_ctim.tv_nsec - b->st_ctim.tv_nsec;
#elif defined(HAVE_STRUCT_STAT_ST_MTIMESPEC)
	if (sel == 'm')
		return ((r = a->st_mtimespec.tv_sec - b->st_mtimespec.tv_sec) != 0) ?
		       r : a->st_mtimespec.tv_nsec - b->st_mtimespec.tv_nsec;
#ifdef HAVE_STRUCT_STAT_ST_OTIMESPEC
	else if (sel == 'o')
		return ((r = a->st_otimespec.tv_sec - b->st_otimespec.tv_sec) != 0) ?
		       r : a->st_otimespec.tv_nsec - b->st_otimespec.tv_nsec;
#endif
	else if (sel == 'a')
		return ((r = a->st_atimespec.tv_sec - b->st_atimespec.tv_sec) != 0) ?
		       r : a->st_atimespec.tv_nsec - b->st_atimespec.tv_nsec;
	else if (sel == 'c')
		return ((r = a->st_ctimespec.tv_sec - b->st_ctimespec.tv_sec) != 0) ?
		       r : a->st_ctimespec.tv_nsec - b->st_ctimespec.tv_nsec;
#elif defined(HAVE_STRUCT_STAT_ST_MTIME)
	if (sel == 'm')
		return a->st_mtime - b->st_mtime;
#ifdef HAVE_STRUCT_STAT_ST_OTIME
	else if (sel == 'o')
		return a->st_otime - b->st_otime;
#endif
	else if (sel == 'a')
		return a->st_atime - b->st_atime;
	else if (sel == 'c')
		return a->st_ctime - b->st_ctime;
#else
#	error Tis not ending well.
#endif
	return 0;
}
