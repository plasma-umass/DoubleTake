/*
 *	libHX/assorted/pack.c
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 1999 - 2005
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2 or 3 of the License.
 */
#include <sys/types.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

size_t HX_pack(char *buf, size_t buflen, const char *fmt, ...)
{
	char *obp = buf;
	int run = 1;
	va_list ap;
	va_start(ap, fmt);

	while (run && *fmt != '\0') {
		switch (*fmt++) {
		case 'A': /* string with up to 255 chars */
		case 'a': {
			const char *src = va_arg(ap, const char *);
			size_t slen = strlen(src) & 0xFF;
			if (buflen < slen + 1) {
				run = 0;
				break;
			}
			*buf++ = slen;
			memcpy(buf, src, slen);
			buf    += slen;
			buflen -= slen + 1;
			break;
		}
		case 'C': /* unsigned char */
		case 'c':
			if (buflen-- < 1) {
				run = 0;
				break;
			}
			/*
			 * Minimum object's size being pushed on the stack is 2
			 * bytes since short int is promoted to int when passed
			 * thru va_arg and stays uncasted. (FIXME)
			 */
			*buf++ = va_arg(ap, unsigned int);
			break;
		case 'H': /* unsigned short */
		case 'h':
			if (buflen < sizeof(uint16_t)) {
				run = 0;
				break;
			}
			*((uint16_t *)buf) = va_arg(ap, uint16_t);
			buf    += sizeof(uint16_t);
			buflen -= sizeof(uint16_t);
			break;
		case 'L': /* unsigned long */
		case 'l':
			if (buflen < sizeof(uint32_t)) {
				run = 0;
				break;
			}
			*((uint32_t *)buf) = va_arg(ap, uint32_t);
			buf    += sizeof(uint32_t);
			buflen -= sizeof(uint32_t);
			break;
		case 'S': /* string with up to 65535 chars */
		case 's': {
			const char *src = va_arg(ap, const char *);
			size_t slen = strlen(src) & 0xFFFF;
			if (buflen < slen + 2) {
				run = 0;
				break;
			}
			*((unsigned short *)buf) = slen;
			memcpy(buf += 2, src, slen);
			buf    += slen;
			buflen -= slen + 2;
			break;
		}
		case 'V': /* fixed size string */
		case 'v': {
			void *src = va_arg(ap, void *);
			size_t slen = va_arg(ap, size_t);
			if (buflen < slen) {
				run = 0;
				break;
			}
			memcpy(buf, src, slen);
			buf    += slen;
			buflen -= slen;
			break;
		}
		} /* switch */
	}

	va_end(ap);
	return buf - obp;
}
