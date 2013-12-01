/*
 *	C-string functions
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 1999 - 2008
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2.1 or 3 of the License.
 */
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libHX/ctype_helper.h>
#include <libHX/string.h>
#include "internal.h"

static inline unsigned int min_uint(unsigned int a, unsigned int b)
{
	return (a < b) ? a : b;
}

EXPORT_SYMBOL char *HX_basename(const char *s)
{
	const char *p;
	if ((p = strrchr(s, '/')) != NULL)
		return const_cast1(char *, p + 1);
	return const_cast1(char *, s);
}

EXPORT_SYMBOL char *HX_basename_exact(const char *s)
{
	const char *start, *end;
	char *ret;
	int len;

	if (*s == '\0')
		return HX_strdup(".");
	for (end = s + strlen(s) - 1; end >= s && *end == '/'; --end)
		;
	if (end < s)
		return HX_strdup("/");

	start = HX_strbchr(s, end, '/');
	if (start == NULL) {
		len = end - s + 1;
		ret = HX_memdup(s, len + 1);
	} else {
		++start;
		len = end - start + 1;
		ret = HX_memdup(start, len + 1);
	}
	if (ret == NULL)
		return NULL;
	ret[len] = '\0';
	return ret;
}

EXPORT_SYMBOL char *HX_chomp(char *s)
{
	char *p = s + strlen(s) - 1;
	while (p >= s) {
		if (*p != '\n' && *p != '\r')
			break;
		*p-- = '\0';
	}
	return s;
}

EXPORT_SYMBOL char *HX_dirname(const char *s)
{
	const char *last, *stop;
	char *p;

	if (*s == '\0')
		return HX_strdup(".");

	for (last = s + strlen(s) - 1; last > s && *last == '/'; --last)
		;

	if ((stop = HX_strbchr(s, last, '/')) == NULL)
		return HX_strdup(".");

	for (; stop > s && *stop == '/'; --stop)
		;

	p = HX_memdup(s, stop - s + 2);
	p[stop-s+1] = '\0';
	return p;
}

EXPORT_SYMBOL hxmc_t *HX_getl(hxmc_t **ptr, FILE *fp)
{
	/* Read a whole line into a dynamic buffer. */
	char temp[MAXLNLEN];

	if (fgets(temp, sizeof(temp), fp) == NULL)
		return NULL;

	if (*ptr == NULL)
		*ptr = HXmc_meminit(NULL, 0);
	else
		HXmc_trunc(ptr, 0);

	do {
		HXmc_strcat(ptr, temp);
		if (strchr(temp, '\n') != NULL)
			break;
	} while (fgets(temp, sizeof(temp), fp) != NULL);

	return *ptr;
}

EXPORT_SYMBOL void *HX_memmem(const void *space, size_t spacesize,
    const void *point, size_t pointsize)
{
	size_t i;

	if (pointsize > spacesize)
		return NULL;
	for (i = 0; i <= spacesize - pointsize; ++i)
		if (memcmp(space + i, point, pointsize) == 0)
			return const_cast1(void *, space + i);
	return NULL;
}

EXPORT_SYMBOL char **HX_split(const char *str, const char *delim,
    int *cp, int max)
{
	/*
	 * @countp can be NULL in case you are not interested in the number of
	 * fields. In either case, you can find out the number of fields by
	 * scanning through the resulting vector.
	 */
	int count = 0;
	char **ret;

	if (cp == NULL)
		cp = &count;
	*cp = 1;

	{
		const char *wp = str;
		while ((wp = strpbrk(wp, delim)) != NULL) {
			++*cp;
			++wp;
		}
	}
   // fprintf(stderr, "Initially max is %d, cp %p *cp %lx\n", max, cp, *cp); 

  // Tongping, should be (max == 0 || *cp < max)
	if (max == 0) {
		max = *cp;
  }
	else if (*cp > max)
		*cp = max;

  //fprintf(stderr, "max is %d, cp %p *cp %lx\n", max, cp, *cp); 
	ret = malloc(sizeof(char *) * (*cp + 1));
  fprintf(stderr, "malloc %p \n", ret);
	ret[*cp] = NULL;

	{
		char *seg, *wp = HX_strdup(str), *bg = wp;
		size_t i = 0;

		while (--max > 0) {
			seg      = HX_strsep(&wp, delim);
			ret[i++] = HX_strdup(seg);
		}

		ret[i++] = HX_strdup(wp);
		free(bg);
	}

	return ret;
}

EXPORT_SYMBOL char **HX_split4(char *s, const char *delim, int *fld, int max)
{
	char **stk;
	const char *p = s;
	int count = 1;

	for (p = strpbrk(p, delim); p != NULL; p = strpbrk(++p, delim))
		if (++count >= max && max > 0) {
			count = max;
			break;
		}

	stk = malloc(sizeof(char *) * (count + 1));
	if (stk == NULL)
		return NULL;
	stk[count] = NULL;
	count = HX_split5(s, delim, count, stk);
	if (fld != NULL)
		*fld = count;
	return stk;
}

EXPORT_SYMBOL int HX_split5(char *s, const char *delim, int max, char **stk)
{
	/*
	 * HX_split5 - the "stack split" (we try to avoid using the heap):
	 * Split @s (modifies it, so must be writable!) at @delim with at most
	 * @max fields and putting the results into @stk[0..@max-1].
	 *
	 * Example on @max:
	 *	char *stk[max];
	 *	HX_split5(s, delim, max, stk);
	 */
	int i = 0;
	char *p;

	while (--max > 0) {
		if ((p = strpbrk(s, delim)) == NULL)
			break;
		stk[i++] = s;
		*p = '\0';
		s  = p + 1;
	}

	stk[i++] = s;
	return i;
}

EXPORT_SYMBOL char *HX_strbchr(const char *start, const char *now, char d)
{
	/* Find the last occurrence of @d within @start and @now. */
	while (now >= start)
		if (*now-- == d)
			return const_cast1(char *, ++now);
	return NULL;
}

EXPORT_SYMBOL char *HX_strclone(char **pa, const char *pb)
{
	if (*pa == pb)
		return *pa;
	if (*pa != NULL) {
		free(*pa);
		*pa = NULL;
	}
	if (pb == NULL)
		return NULL;
	if ((*pa = malloc(strlen(pb) + 1)) == NULL)
		return NULL;
	strcpy(*pa, pb);
	return *pa;
}

EXPORT_SYMBOL char *HX_strlower(char *orig)
{
	char *expr;
	for (expr = orig; *expr != '\0'; ++expr)
		*expr = HX_tolower(*expr);
	return orig;
}

EXPORT_SYMBOL size_t HX_strltrim(char *expr)
{
	char *travp;
	size_t diff = 0;
	travp = expr;

	while (*travp != '\0' && HX_isspace(*travp))
		++travp;
	if ((diff = travp - expr) > 0)
		memmove(expr, travp, diff);
	return diff;
}

/* supports negative offsets like scripting languages */
EXPORT_SYMBOL char *HX_strmid(const char *expr, long offset, long length)
{
	char *buffer;

	if (offset < 0)
		offset = strlen(expr) + offset;
	if (length < 0)
		length = strlen(expr) - offset + length;
	if ((buffer = malloc(length + 1)) == NULL)
		return NULL;

	expr += offset;
	return HX_strlcpy(buffer, expr, length + 1);
}

EXPORT_SYMBOL size_t HX_strrcspn(const char *s, const char *rej)
{
	size_t n = strlen(s);
	const char *p = s + n;
	while (--p >= s)
		if (strchr(rej, *p) != NULL)
			return p - s;
	return n;
}

EXPORT_SYMBOL char *HX_strrev(char *s)
{
	size_t i, z = strlen(s)-1, z2 = z / 2;

	for (i = 0; i < z2; ++i) {
		char temp;
		temp = s[i];
		s[i] = s[z-i];
		s[z-i] = temp;
	}

	return s;
}

EXPORT_SYMBOL size_t HX_strrtrim(char *expr)
{
	int i = strlen(expr), s = 0;
	while (i-- && HX_isspace(expr[i]))
		++s;
	expr[++i] = '\0';
	return s;
}

EXPORT_SYMBOL char *HX_strsep(char **sp, const char *d)
{
	char *begin, *end;

	if (*sp == NULL || **sp == '\0')
		return NULL;
	begin = *sp;

	if (d[0] == '\0' || d[1] == '\0') {
		if (*begin == *d)
			end = begin;
		else if (*begin == '\0')
			end = NULL;
		else
			end = strchr(begin + 1, *d);
	} else {
		end = strpbrk(begin, d);
	}

	if (end == NULL) {
		*sp = NULL;
	} else {
		*end++ = '\0';
		*sp = end;
	}
	
	return begin;
}

EXPORT_SYMBOL char *HX_strsep2(char **wp, const char *str)
{
	char *ptr, *ret;
	if (*wp == NULL)
		return NULL;
	ret = *wp;
	if ((ptr = strstr(*wp, str)) == NULL) {
		*wp = NULL;
		return ret;
	}
	*ptr = '\0';
	*wp  = ptr + strlen(str);
	return ret;
}

static const char *const HX_quote_chars[] = {
	[HXQUOTE_SQUOTE] = "'\\",
	[HXQUOTE_DQUOTE] = "\"\\",
	[HXQUOTE_HTML]   = "\"&<>",
};

static size_t HX_qsize_backslash(const char *s, const char *qchars)
{
	const char *p = s;
	size_t n = strlen(s);

	while ((p = strpbrk(p, qchars)) != NULL) {
		++n;
		++p;
	}
	return n;
}

static char *HX_quote_backslash(char *dest, const char *src, const char *qc)
{
	char *ret = dest;
	size_t len;

	while (*src != '\0') {
		len = strcspn(src, qc);
		if (len > 0) {
			memcpy(dest, src, len);
			dest += len;
			src  += len;
			if (*src == '\0')
				break;
		}
		*dest++ = '\\';
		*dest++ = *src++;
	}

	*dest = '\0';
	return ret;
}

static size_t HX_qsize_html(const char *s)
{
	const char *p = s;
	size_t n = strlen(s);

	while ((p = strpbrk(p, HX_quote_chars[HXQUOTE_HTML])) != NULL) {
		switch (*p) {
		/* minus 2: \0 and the original char */
		case '"':
			n += sizeof("&quot;") - 2;
			break;
		case '&':
			n += sizeof("&amp;") - 2;
			break;
		case '<':
		case '>':
			n += sizeof("&lt;") - 2;
			break;
		}
		++p;
	}
	return n;
}

static char *HX_quote_html(char *dest, const char *src)
{
#define put(s) do { \
	memcpy(dest, (s), sizeof(s) - 1); \
	dest += sizeof(s) - 1; \
} while (false);

	char *ret = dest;

	while (*src != '\0') {
		size_t len = strcspn(src, HX_quote_chars[HXQUOTE_HTML]);
		if (len > 0) {
			memcpy(dest, src, len);
			dest += len;
			src  += len;
			if (*src == '\0')
				break;
		}
		switch (*src++) {
		case '"': put("&quot;"); break;
		case '&': put("&amp;"); break;
		case '<': put("&lt;"); break;
		case '>': put("&gt;"); break;
		}
	}
	*dest = '\0';
	return ret;
#undef put
}

/**
 * HX_quoted_size -
 * @s:		string to analyze
 * @type:	quoting method
 *
 * Returns the size of the string @s when quoted.
 */
static size_t HX_quoted_size(const char *s, unsigned int type)
{
	switch (type) {
	case HXQUOTE_SQUOTE:
	case HXQUOTE_DQUOTE:
		return HX_qsize_backslash(s, HX_quote_chars[type]);
	case HXQUOTE_HTML:
		return HX_qsize_html(s);
	default:
		return strlen(s);
	}
}

EXPORT_SYMBOL char *HX_strquote(const char *src, unsigned int type,
    char **free_me)
{
	bool do_quote;
	char *tmp;

	do_quote = type >= _HXQUOTE_MAX || (type < _HXQUOTE_MAX &&
	           strpbrk(src, HX_quote_chars[type]) != NULL);

	/*
	 * free_me == NULL implies that we always allocate, even if
	 * there is nothing to quote.
	 */
	if (free_me != NULL) {
		free(*free_me);
		*free_me = NULL;
		if (!do_quote)
			return const_cast1(char *, src);
	} else {
		if (!do_quote)
			return HX_strdup(src);
		free_me = &tmp;
	}

	*free_me = malloc(HX_quoted_size(src, type) + 1);
	if (*free_me == NULL)
		return NULL;

	switch (type) {
	case HXQUOTE_SQUOTE:
	case HXQUOTE_DQUOTE:
		return HX_quote_backslash(*free_me, src, HX_quote_chars[type]);
	case HXQUOTE_HTML:
		return HX_quote_html(*free_me, src);
	}
	return NULL;
}

EXPORT_SYMBOL char *HX_strupper(char *orig)
{
	char *expr;
	for (expr = orig; *expr != '\0'; ++expr)
		*expr = HX_toupper(*expr);
	return orig;
}
