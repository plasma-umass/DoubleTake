/*
 *	libHX/assorted/tofrac.c
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 1999 - 2005
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2 or 3 of the License.
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <libHX.h>

int HX_tofrac(double arg, unsigned long num, unsigned long denom)
{
	size_t i, min_denom = *num, max_denom = *denom;
	char simple[32], rounded[32];
	double j;

	if (min_denom > max_denom) {
		for (i = max_denom; i < min_denom; --i) {
			j = arg * i;
			snprintf(simple, sizeof(simple), "%f", j);
			snprintf(rounded, sizeof(rounded), "%.0f", j);
			j = strtod(rounded, 0);
			if (strtod(simple, NULL) == j) {
				*num   = j;
				*denom = i;
				return 1;
			}
		}
	} else {
		for (i = min_denom; i < max_denom; ++i) {
			j = arg * i;
			snprintf(simple, sizeof(simple), "%f", j);
			snprintf(rounded, sizeof(rounded), "%.0f", j);
			j = strtod(rounded, 0);
			if (strtod(simple, NULL) == j) {
				*num   = j;
				*denom = i;
				return 1;
			}
		}
	}
	return 0;
}
