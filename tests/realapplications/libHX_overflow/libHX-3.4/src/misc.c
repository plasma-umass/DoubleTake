/*
 *	Miscellaneous functions
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 1999 - 2009
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2.1 or 3 of the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libHX/ctype_helper.h>
#include <libHX/misc.h>
#include "internal.h"

EXPORT_SYMBOL int HX_ffs(unsigned long n)
{
	int s = 0;
	if (n == 0)
		return -1;
	while ((n >>= 1) >= 1)
		++s;
	return s;
}

EXPORT_SYMBOL int HX_fls(unsigned long n)
{
	int i;
	for (i = 31; i >= 0; --i)
		if (n & (1 << i))
			return i;
	return -1;
}

EXPORT_SYMBOL void HX_hexdump(FILE *fp, const void *vptr, unsigned int len)
{
	const unsigned char *ptr = vptr;
	unsigned int i, j;
	bool tty = isatty(fileno(fp));

	fprintf(fp, "Dumping %u bytes\n", len);
	for (i = 0; i < len / 16; ++i) {
		fprintf(fp, "%04x | ", i * 16);
		for (j = 0; j < 16; ++j)
			fprintf(fp, "%02x%c", *ptr++, (j == 7) ? '-' : ' ');
		ptr -= 16;
		fprintf(fp, "| ");
		for (j = 0; j < 16; ++j, ++ptr)
			if (HX_isprint(*ptr))
				fprintf(fp, "%c", *ptr);
			else if (tty)
				fprintf(fp, "\e[31m.\e[0m"); // ]]
			else
				fprintf(fp, ".");
		fprintf(fp, "\n");
	}
	fprintf(fp, "%04x | ", i * 16);
	len -= i * 16;
	for (i = 0; i < len; ++i)
		fprintf(fp, "%02x%c", ptr[i], (i == 7) ? '-' : ' ');
	for (; i < 16; ++i)
		fprintf(fp, "   ");
	fprintf(fp, "| ");
	for (i = 0; i < len; ++i)
		if (HX_isprint(ptr[i]))
			fprintf(fp, "%c", ptr[i]);
		else if (tty)
			fprintf(fp, "\e[31m.\e[0m"); // ]]
		else
			fprintf(fp, ".");
	fprintf(fp, "\n");
}

EXPORT_SYMBOL void HX_zvecfree(char **args)
{
	char **travp;
	for (travp = args; *travp != NULL; ++travp)
		free(*travp);
	free(args);
}
